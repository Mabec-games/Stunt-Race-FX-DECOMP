/*
 * fx_far.c: the race's view distance (plan stage 10; the earlier work is
 * 218f69f1).
 *
 * The draw list comes from a block of map cells ahead of the view, at most
 * $31 records, so things showed up at the block's edge. Every other object
 * the list would draw, in every cell of the map, gets a far record from the
 * draw list (fx_far.h), with its place from the view in 32 bits. Here each
 * is turned to the view (the matrix at $00E4, as the sort turns the near
 * ones), left out behind the view or past its sides (the sort's tests, its
 * radius and the view's width), and halved until its place fits 14 bits;
 * every object, near and far (a parked tyre of a car in the near list,
 * FX_FAR_PARKED, among them), is drawn in one order by depth (merged_pass),
 * $31 at a time in the near slots' place, through FxObjectsDraw, with the
 * depth test deciding what is seen (docs/stuntracefx/native/DEPTH.md). While
 * a far one draws, its model's matrix and its sprite's size are halved
 * with its place (render/model.c, render/frame.c, fx_solid.c), so it lands
 * where it stands at its size, and its level of detail and colour tier are
 * taken from its true depth. The chip's first 64 KB and its state are kept
 * and put back after: only the picture changes. Which objects are active
 * stays the trigger list's. While the pass draws, a slot holds another
 * record than its own place says: fx_far_original gives the near record it
 * holds (none for a far one or a tyre), for the eyes' gaze and the angles
 * between steps.
 */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "data/cart.h"
#include "fx/fx.h"
#include "fx/fx_draw.h"
#include "fx/fx_far.h"
#include "fx/fx_ground.h"
#include "fx/fx_view.h"
#include "object/object.h"
#include "system/chip.h"
#include "system/state.h"

enum { SLOTS = 0x31, SLOT_SIZE = 0x1E, SLOT_FIRST = 0x0AAA };

typedef struct {
    int64_t key;
    int16_t at[3];
    uint8_t shift;
    uint16_t depth, rec;
} Far;

static Far parked[FX_FAR_MAX];          /* the far objects in view this picture */
static unsigned nparked;
static uint8_t slot_shift[SLOTS];
static uint16_t slot_depth[SLOTS];
static uint16_t slot_original[SLOTS];   /* the near record a slot holds in a pass; 0 none */
static uint16_t slot_rec[SLOTS];        /* the far record a slot holds in a pass (its address in the second 64 KB); 0 none */
static uint16_t slot_car[SLOTS];        /* the car a slot's record belongs to in the merged pass (draw_record_car); 0 none */
static uint8_t slot_tyre[SLOTS];        /* and whether it is one of its tyres */
static uint16_t slot_object[SLOTS];     /* and its object (draw_record_object); 0 none */

static int drawing, shift_now;
static uint16_t depth_now;
static uint8_t kept[0x10000];

/* words of the chip's RAM (the second 64 KB, the first) and of ROM bank 0,
   read and written without moving the chip's last RAM address */
static uint16_t far_word(uint16_t a)
{
    return (uint16_t)(fx_ram[FX_FAR_BANK + a] | fx_ram[FX_FAR_BANK + (uint16_t)(a + 1)] << 8);
}

static uint16_t ram_word(uint16_t a)
{
    return (uint16_t)(fx_ram[a] | fx_ram[(uint16_t)(a + 1)] << 8);
}

static void ram_put(uint16_t a, uint16_t v)
{
    fx_ram[a] = (uint8_t)v;
    fx_ram[(uint16_t)(a + 1)] = (uint8_t)(v >> 8);
}

static uint16_t rom_word(uint16_t a)
{
    return (uint16_t)(fx_rom_read(0, a) | fx_rom_read(0, (uint16_t)(a + 1)) << 8);
}

static int64_t magnitude(int64_t v) { return v < 0 ? -v : v; }

/* ---- the course ahead (fx_far_pass in a tunnel's zone) ----
   The course's segment table (the chip's $1092 in bank $1094: a count, then
   x, y, z and a word a segment): from the segment nearest the eye, the next
   ALONG_SEGMENTS of them; an object is along the course when it lies within
   ALONG_WIDE of one of their stretches across the ground. */
enum { ALONG_SEGMENTS = 12 };
#define ALONG_WIDE 2200.0

static int16_t along_x[ALONG_SEGMENTS + 1], along_z[ALONG_SEGMENTS + 1];
static int along_n;
static int32_t along_eye[2];

static int along_setup(void)
{
    uint16_t table = (uint16_t)(fx_ram[0x1092] | fx_ram[0x1093] << 8);
    uint8_t bank = fx_ram[0x1094];
    int n, i, best = 0;
    double bd = 1e30;

    along_n = 0;
    if (table < 0x8000)
        return 0;
    n = cart16(bank, table);
    if (n < 2)
        return 0;
    along_eye[0] = (int16_t)cameras[0].x;
    along_eye[1] = (int16_t)cameras[0].z;
    for (i = 0; i < n; i++) {
        double dx = (int16_t)cart16(bank, (uint16_t)(table + 2 + i * 8)) - along_eye[0];
        double dz = (int16_t)cart16(bank, (uint16_t)(table + 6 + i * 8)) - along_eye[1];
        if (dx * dx + dz * dz < bd)
            bd = dx * dx + dz * dz, best = i;
    }
    /* from the one before the nearest (the eye trails the car) */
    for (i = 0; i <= ALONG_SEGMENTS; i++) {
        int k = (best - 1 + i + n) % n;
        along_x[i] = (int16_t)cart16(bank, (uint16_t)(table + 2 + k * 8));
        along_z[i] = (int16_t)cart16(bank, (uint16_t)(table + 6 + k * 8));
    }
    along_n = ALONG_SEGMENTS + 1;
    return 1;
}

static int along_course(double x, double z)
{
    int i;

    for (i = 0; i + 1 < along_n; i++) {
        double ax = along_x[i], az = along_z[i], bx = along_x[i + 1] - ax, bz = along_z[i + 1] - az;
        double l = bx * bx + bz * bz, t = l > 0 ? ((x - ax) * bx + (z - az) * bz) / l : 0, px, pz;

        t = t < 0 ? 0 : t > 1 ? 1 : t;
        px = ax + t * bx - x;
        pz = az + t * bz - z;
        if (px * px + pz * pz < ALONG_WIDE * ALONG_WIDE)
            return 1;
    }
    return 0;
}

void fx_far_pass(void)
{
    unsigned count = far_word(FX_FAR_COUNT), i, k;
    int along;
    int32_t m[9], centre, half;

    nparked = 0;
    if (!count)
        return;
    if (count > FX_FAR_MAX)
        count = FX_FAR_MAX;
    fx_ram[FX_FAR_BANK + FX_FAR_COUNT] = fx_ram[FX_FAR_BANK + FX_FAR_COUNT + 1] = 0;
    /* in a sealed tunnel or a zone of the alternate backdrop (bits 3 and 6
       of $0DA5, which Aqua Tunnel's gates set under the water) only the far
       objects along the course ahead: the tube's next sections at full
       distance, not the lap's other sections seen across the water */
    along = (g.picture_effects & 0x48) && !getenv("SRFX_FAR_IN_TUNNEL") && along_setup();
    for (i = 0; i < 9; i++)
        m[i] = (int16_t)ram_word((uint16_t)(0x00E4 + 2 * i));
    /* the view's half-width, as the sort takes it */
    centre = (int16_t)ram_word(0x0034);
    half = centre - (int16_t)ram_word(0x0038);
    if ((int16_t)ram_word(0x003A) + 1 - centre > half)
        half = (int16_t)ram_word(0x003A) + 1 - centre;
    for (i = 0; i < count; i++) {
        uint16_t rec = (uint16_t)(FX_FAR_RECORDS + i * FX_FAR_SIZE), shape = far_word((uint16_t)(rec + 0x08));
        int32_t radius = (int16_t)rom_word((uint16_t)(shape + 0x10)), bias = (int16_t)rom_word((uint16_t)(shape + 0x05));
        int64_t d[3], v[3], big = 0, x, z;
        unsigned s = 0;
        int c;

        for (k = 0; k < 3; k++)
            d[k] = (int32_t)(far_word((uint16_t)(rec + FX_FAR_PLACE + 4 * k))
                             | (uint32_t)far_word((uint16_t)(rec + FX_FAR_PLACE + 4 * k + 2)) << 16);
        /* the slot's +$12, +$10, +$14 turned as the sort turns them: each
           product's top 16 bits doubled with bit 15 below, summed */
        for (c = 0; c < 3; c++)
            v[c] = (d[0] * m[c] >> 15) + (d[1] * m[3 + c] >> 15) + (d[2] * m[6 + c] >> 15);
        if (v[2] + radius < 0)
            continue;
        if (along && !along_course((double)along_eye[0] + (double)d[0], (double)along_eye[1] + (double)d[2]))
            continue;
        x = magnitude(v[0]);
        z = magnitude(v[2]);
        if (!(half > 128 ? x - radius < z * half / 128 : x - radius < z))
            continue;
        for (k = 0; k < 3; k++)
            if (magnitude(v[k]) > big)
                big = magnitude(v[k]);
        while ((big >> s) > 0x3FFF)
            s++;
        {
            Far *f = &parked[nparked++];

            /* its key whole, as merged_pass keys the near ones */
            f->key = (magnitude(d[1]) << 1) + (fx_ram[FX_FAR_BANK + (uint16_t)(rec + FX_FAR_LIFT)] ? 0x1F40 : 0) + bias + v[2];
            for (k = 0; k < 3; k++)
                f->at[k] = (int16_t)(v[k] >> s);
            f->shift = (uint8_t)s;
            f->depth = (uint16_t)(v[2] > 0x7FFF ? 0x7FFF : v[2] < 0 ? 0 : v[2]);
            f->rec = rec;
        }
    }
}

/* slot k filled with far record f: its record, its link, key and place */
static void far_slot(unsigned k, const Far *f, int last)
{
    uint16_t slot = (uint16_t)(SLOT_FIRST + k * SLOT_SIZE);
    int64_t key = f->key < 0 ? 0 : f->key > 0xFFFF ? 0xFFFF : f->key;

    memcpy(&fx_ram[slot], &fx_ram[FX_FAR_BANK + f->rec], SLOT_SIZE);
    ram_put(slot, last ? 0 : (uint16_t)(slot + SLOT_SIZE));
    ram_put((uint16_t)(slot + 0x02), (uint16_t)key);
    ram_put((uint16_t)(slot + 0x12), (uint16_t)f->at[0]);
    ram_put((uint16_t)(slot + 0x10), (uint16_t)f->at[1]);
    ram_put((uint16_t)(slot + 0x14), (uint16_t)f->at[2]);
    slot_shift[k] = f->shift;
    slot_depth[k] = f->depth;
    slot_original[k] = 0;
    slot_rec[k] = f->rec;
}

/*
 * Every object in one order by depth, the near list's among the far ones
 * (the depth test decides what is seen; the order, the layers of faces on
 * one plane and the cars after the course). A view other than the game's
 * (race/overhead.c) may lie far from the objects, so each near slot is
 * taken whole as
 * the far ones are: its place in view space (the sort's, exact in 16 bits
 * while its place from the eye is), its key as the sort made it less the
 * 16 bits it wrapped at (the record's own key, its shape's bias and its
 * depth; a record placed in view space keeps its own key's sign, $F060
 * the nearest), halved with its size while its place passes 14 bits. The
 * far objects (fx_far_pass, all among the parked here) come keyed the same
 * way. Drawn farthest first, $31 at a time; the chip's RAM put back after.
 */
typedef struct {
    int64_t key;
    unsigned seq;
    int far;                    /* a far object (parked[at]), else near slot `at` */
    unsigned at;
    int16_t place[3];
    uint8_t shift;
    uint16_t depth;
    uint16_t car;               /* its car (draw_record_car), 0 none */
    uint8_t tyre;
    uint16_t object;            /* its object (draw_record_object), 0 none */
    int64_t car_key;            /* its car's farthest part's key, the cars' order */
} Merged;

/* farthest first; of equal keys the near list's first, in its order. The
   cars come after the course, each car's parts together
   (the cars farthest first), its tyres first: the depth decides what is
   seen, and a car is seated on the course's faces (fx_draw.c) */
static int merged_order(const void *a, const void *b)
{
    const Merged *x = a, *y = b;

    if (!x->car != !y->car)
        return x->car ? 1 : -1;
    if (x->car && x->car != y->car)
        return x->car_key != y->car_key ? (x->car_key < y->car_key ? 1 : -1) : (x->car < y->car ? -1 : 1);
    if (x->car && x->tyre != y->tyre)
        return x->tyre ? -1 : 1;
    if (x->key != y->key)
        return x->key < y->key ? 1 : -1;
    return x->seq < y->seq ? -1 : x->seq > y->seq;
}

static void merged_pass(uint16_t *stack)
{
    static Merged e[SLOTS + FX_FAR_MAX];
    static uint8_t by_slot[SLOTS][SLOT_SIZE];
    FxState state = fxs;
    unsigned n = 0, i, j, b, k, near = 0;
    uint16_t slot;

    for (slot = ram_word(GSU_DRAW_LIST); slot && near < SLOTS; slot = ram_word(slot), near++) {
        uint16_t shape = ram_word((uint16_t)(slot + 0x08));
        int32_t bias = (int16_t)rom_word((uint16_t)(shape + 0x05));
        int32_t v[3] = { (int16_t)ram_word((uint16_t)(slot + 0x12)), (int16_t)ram_word((uint16_t)(slot + 0x10)),
                         (int16_t)ram_word((uint16_t)(slot + 0x14)) };
        uint16_t own = (uint16_t)(ram_word((uint16_t)(slot + 0x02)) - (uint16_t)bias - (uint16_t)v[2]);
        int32_t big = 0;
        unsigned s = 0;

        for (k = 0; k < 3; k++)
            if ((v[k] < 0 ? -v[k] : v[k]) > big)
                big = v[k] < 0 ? -v[k] : v[k];
        while ((big >> s) > 0x3FFF)
            s++;
        e[n].key = ((fx_ram[(uint16_t)(slot + 0x07)] & 0x80) ? (int64_t)(int16_t)own : (int64_t)own) + bias + v[2];
        {
            int tyre = 0;

            e[n].car = draw_record_car(0, (uint16_t)(slot - SLOT_FIRST) / SLOT_SIZE, &tyre);
            e[n].tyre = (uint8_t)tyre;
            e[n].object = draw_record_object(0, (uint16_t)(slot - SLOT_FIRST) / SLOT_SIZE);
        }
        e[n].far = 0;
        e[n].at = (unsigned)(uint16_t)(slot - SLOT_FIRST) / SLOT_SIZE;
        for (k = 0; k < 3; k++)
            e[n].place[k] = (int16_t)(v[k] >> s);
        e[n].shift = (uint8_t)s;
        e[n].depth = (uint16_t)(v[2] > 0x7FFF ? 0x7FFF : v[2] < 0 ? 0 : v[2]);
        e[n].seq = n;
        n++;
    }
    for (j = 0; j < nparked; j++) {
        int tyre = 0;

        e[n].key = parked[j].key;
        e[n].car = draw_record_car(1, (uint16_t)(parked[j].rec - FX_FAR_RECORDS) / FX_FAR_SIZE, &tyre);
        e[n].tyre = (uint8_t)tyre;
        e[n].object = draw_record_object(1, (uint16_t)(parked[j].rec - FX_FAR_RECORDS) / FX_FAR_SIZE);
        e[n].far = 1;
        e[n].at = j;
        e[n].seq = n;
        n++;
    }
    {                                   /* each car's order: its farthest part's key */
        static int64_t car_far[OBJECT_COUNT + 1];

        for (i = 0; i < n; i++)
            if (e[i].car && e[i].car <= OBJECT_COUNT)
                car_far[e[i].car] = INT64_MIN;
        for (i = 0; i < n; i++)
            if (e[i].car && e[i].car <= OBJECT_COUNT && e[i].key > car_far[e[i].car])
                car_far[e[i].car] = e[i].key;
        for (i = 0; i < n; i++)
            e[i].car_key = e[i].car && e[i].car <= OBJECT_COUNT ? car_far[e[i].car] : e[i].key;
    }
    qsort(e, n, sizeof e[0], merged_order);
    memcpy(kept, fx_ram, sizeof kept);
    for (i = 0; i < SLOTS; i++)
        memcpy(by_slot[i], &kept[SLOT_FIRST + i * SLOT_SIZE], SLOT_SIZE);
    drawing = 1;
    for (b = 0; b < n; b += SLOTS) {
        unsigned in = n - b < SLOTS ? n - b : SLOTS;

        for (k = 0; k < in; k++) {
            slot_car[k] = e[b + k].car;
            slot_tyre[k] = e[b + k].tyre;
            slot_object[k] = e[b + k].object;
            if (e[b + k].far) {
                far_slot(k, &parked[e[b + k].at], k + 1 == in);
                continue;
            }
            slot = (uint16_t)(SLOT_FIRST + k * SLOT_SIZE);
            memcpy(&fx_ram[slot], by_slot[e[b + k].at], SLOT_SIZE);
            ram_put(slot, k + 1 < in ? (uint16_t)(slot + SLOT_SIZE) : 0);
            ram_put((uint16_t)(slot + 0x12), (uint16_t)e[b + k].place[0]);
            ram_put((uint16_t)(slot + 0x10), (uint16_t)e[b + k].place[1]);
            ram_put((uint16_t)(slot + 0x14), (uint16_t)e[b + k].place[2]);
            slot_shift[k] = e[b + k].shift;
            slot_depth[k] = e[b + k].depth;
            slot_rec[k] = 0;
            slot_original[k] = (uint16_t)(SLOT_FIRST + e[b + k].at * SLOT_SIZE);
        }
        ram_put(GSU_DRAW_LIST, SLOT_FIRST);
        *stack = 0x0240;
        fx_stw(FX_MODEL_1B4, 0);
        (void)FxObjectsDraw(0, stack);
        fx_ground_object_end();
    }
    fx_draw_flush();
    drawing = 0;
    shift_now = 0;
    nparked = 0;
    memcpy(fx_ram, kept, sizeof kept);
    fxs = state;
}

void fx_far_parked(uint16_t *stack)
{
    merged_pass(stack);
}

uint16_t fx_far_original(uint16_t record)
{
    unsigned k;

    if (!drawing)
        return record;
    if ((uint16_t)(record - SLOT_FIRST) % SLOT_SIZE)
        return 0;
    k = (uint16_t)(record - SLOT_FIRST) / SLOT_SIZE;
    return k < SLOTS ? slot_original[k] : 0;
}

int fx_far_record_number(uint16_t record)
{
    unsigned k;

    if (!drawing || (uint16_t)(record - SLOT_FIRST) % SLOT_SIZE)
        return -1;
    k = (uint16_t)(record - SLOT_FIRST) / SLOT_SIZE;
    if (k >= SLOTS || !slot_rec[k])
        return -1;
    return (slot_rec[k] - FX_FAR_RECORDS) / FX_FAR_SIZE;
}

static void fx_far_record_state(uint16_t record);

void fx_far_record(uint16_t record)
{
    fx_ground_object_end();             /* the object before's tunnel rock, at its own depth */
    fx_far_record_state(record);
    fx_ground_object_begin(record, shift_now);
}

static void fx_far_record_state(uint16_t record)
{
    unsigned k;

    if (!drawing) {
        shift_now = 0;
        return;
    }
    k = (uint16_t)(record - SLOT_FIRST) / SLOT_SIZE % SLOTS;
    shift_now = slot_shift[k];
    depth_now = slot_depth[k];
}

uint16_t fx_far_object(uint16_t record)
{
    unsigned k;

    if (!drawing || (uint16_t)(record - SLOT_FIRST) % SLOT_SIZE)
        return 0;
    k = (uint16_t)(record - SLOT_FIRST) / SLOT_SIZE;
    return k < SLOTS ? slot_object[k] : 0;
}

uint16_t fx_far_car(uint16_t record, int *tyre)
{
    unsigned k;

    *tyre = 0;
    if (!drawing || (uint16_t)(record - SLOT_FIRST) % SLOT_SIZE)
        return 0;
    k = (uint16_t)(record - SLOT_FIRST) / SLOT_SIZE;
    if (k >= SLOTS)
        return 0;
    *tyre = slot_tyre[k];
    return slot_car[k];
}

/* near record i's place as the main CPU wrote it (+$12, +$10, +$14, from
   the view in 16 bits), when the chip's slot `slot` was uploaded from it */
static int near_place(unsigned i, uint16_t slot, double rel[3], int *in_view)
{
    const uint8_t *r = g.frame_records + 30u * i;

    if (i >= g.frame_record_count || i >= 64 || (uint16_t)(r[0x08] | r[0x09] << 8) != ram_word((uint16_t)(slot + 0x08)))
        return 0;
    rel[0] = (int16_t)(uint16_t)(r[0x12] | r[0x13] << 8);
    rel[1] = (int16_t)(uint16_t)(r[0x10] | r[0x11] << 8);
    rel[2] = (int16_t)(uint16_t)(r[0x14] | r[0x15] << 8);
    *in_view = (r[0x07] & 0x80) != 0;
    return 1;
}

int fx_far_exact_place(uint16_t record, double rel[3], int *in_view)
{
    unsigned k;

    *in_view = 0;
    if ((uint16_t)(record - SLOT_FIRST) % SLOT_SIZE)
        return 0;
    k = (uint16_t)(record - SLOT_FIRST) / SLOT_SIZE;
    if (k >= SLOTS)
        return 0;
    if (!drawing)
        return 0;
    if (slot_rec[k]) {
        unsigned c;

        for (c = 0; c < 3; c++)
            rel[c] = (int32_t)(far_word((uint16_t)(slot_rec[k] + FX_FAR_PLACE + 4 * c))
                               | (uint32_t)far_word((uint16_t)(slot_rec[k] + FX_FAR_PLACE + 4 * c + 2)) << 16);
        return 1;
    }
    return slot_original[k] && near_place((uint16_t)(slot_original[k] - SLOT_FIRST) / SLOT_SIZE, record, rel, in_view);
}

int fx_far_shift(void) { return shift_now; }
uint16_t fx_far_depth(uint16_t depth) { return fx_view_depth(shift_now ? depth_now : depth); }
int fx_far_drawing(void) { return drawing; }
