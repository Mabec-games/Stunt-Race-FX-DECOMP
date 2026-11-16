/*
 * fx_solid.c: sprites that show solid things drawn as solid shapes (plan
 * stage 10, solids; the design of src/stuntracefx/remaster/solids.c, as
 * approved there). Some of the race's objects are pictures facing the
 * view, drawn by FxObjectSprite with the picture picked for the angle they
 * are seen at: every car's tyres (shape $A1C8), the bonus course truck's
 * wheels ($A1E4), every car's eyes ($A190, near or far) and the trees' trunks and
 * canopies ($A104, $A120; Night Cruise's bushes $A0E8). Each is drawn in
 * its place as a solid instead:
 *
 *   tyres, wheels   a shape turned about its axle (a profile of rings, the
 *                   ends capped and ringed), at the object's place in view
 *                   space, turned as the model drawer turns a model with
 *                   the object's angles, its faces flat in the colours its
 *                   pictures use, a side's shade by how far it faces up;
 *   eyes            a half dome, its flat face ahead, turned by the byte
 *                   its sprite's picture is picked by (the objects' gaze:
 *                   the lens to the view at 119.5, side on at 55.5, 64 a
 *                   quarter turn), level; its face its look's picture
 *                   facing the view drawn as marks: plain ($9AAF),
 *                   startled ($9AB1), lidded ($9AB3, $9AB7) or shut
 *                   ($9AB5); an eye with no gaze (its fixed picture) stays
 *                   its sprite;
 *   trees           the trunk and the canopy each turned about the world's
 *                   upright, their widths from their paintings, the
 *                   canopy's points moved a little in and out and up and
 *                   down (the same for every tree), so its facets take the
 *                   light each its own way; each face's colour one of its
 *                   painting's by how far it faces the light the painting
 *                   is lit from; Night Cruise's bushes ($A0E8) a canopy
 *                   alone, to the ground (Jeff, 2026-11-12: every course's
 *                   2D trees our 3D trees; a census of the sprites every
 *                   course leaves flat and of the objects its records
 *                   place found no other tree: $A104/$A120 on Easy Ride
 *                   and King's Forest, $A0E8 on Night Cruise).
 *
 * The sprite's code runs as ever (its state, its clipping, its colour);
 * only its quads are left out (fx_solid_hiding). The solid's faces are
 * points in view space, projected and clipped by fx_face_triangles and
 * filled by the GPU. Effects stay sprites. In two players' pictures (four
 * bits a pixel, their colours one palette row's) the tyres and wheels take
 * one player's colours, entries of palette row 14, which the battle palette
 * has the same; the eyes take the battle row's colours at the same places
 * (its body blue, its white and black are one player's at those places);
 * the trees keep their sprites.
 *
 * In a shape's own coordinates x is across, y down and z ahead.
 */
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "fx/fx.h"
#include "fx/fx_draw.h"
#include "fx/fx_face.h"
#include "fx/fx_far.h"
#include "fx/fx_rt.h"
#include "object/object.h"
#include "system/state.h"
#include "video/picture.h"

#define PI_F 3.14159265f
#define SEGMENTS_MAX 60

/* a profile's ring: its place along the axis and radius (texels); a band
   from it to the next ring in one colour (nib) or shaded by how far it
   faces up: top (nearly straight up, when nonzero), up, across, down */
typedef struct {
    float a, r;
    uint8_t nib, top, up, across, down;
} Ring;
/* a cap's ring, drawn over the cap outward in: radius, colour */
typedef struct {
    float r;
    uint8_t nib;
} Disc;
typedef struct {
    uint16_t shape;
    int texels, axis;                   /* the picture's width; the axis (0 x, 1 y, 2 z) */
    const Ring *rings;
    int nrings;
    const Disc *front, *back;           /* the caps at the last ring (+) and the first (-) */
    int nfront, nback;
    int tree;                           /* a tree's: 1 its trunk, 2 its canopy (shaped by its painting) */
} Solid;

/* a tyre: 28 texels of its 32 across, 18 wide, shoulders a texel; its
   side 9; its hub 4 inside a ring of B round a centre of 9; its tread 5
   up, 6 across, A down; its shoulders A */
static const Ring tyre_rings[] = {
    { -9, 13, 0xA, 0, 0, 0, 0 }, { -8, 14, 0, 0, 0x5, 0x6, 0xA }, { 8, 14, 0xA, 0, 0, 0, 0 }, { 9, 13, 0, 0, 0, 0, 0 },
};
static const Disc tyre_side[] = { { 13, 0x9 }, { 6, 0x4 }, { 4, 0xB }, { 2, 0x9 } };

/* an eye: a half dome 14 of its 16 texels across, its flat face ahead
   (the eye) and its dome behind: the dome shaded B nearly straight up, 8
   up, 7 across, F down; its face's ground the body colour (7), its marks
   over it; its back's last cap 7 */
#define LAMP_SHADE 0, 0xB, 0x8, 0x7, 0xF
static const Ring lamp_rings[] = {      /* round the dome from its back: 15, 30, ... 90 degrees */
    { -3.261f, 1.812f, LAMP_SHADE }, { -2.562f, 3.5f, LAMP_SHADE }, { -1.450f, 4.950f, LAMP_SHADE },
    { 0.0f, 6.062f, LAMP_SHADE }, { 1.688f, 6.761f, LAMP_SHADE }, { 3.5f, 7.0f, 0, 0, 0, 0, 0 },
};
static const Disc lamp_lens[] = { { 7, 0x7 } };
static const Disc lamp_back[] = { { 1.812f, 0x7 } };

/* An eye's face, as its look's picture facing the view has it (16 x 16
   texels): marks in order, each a disc (r0 and t0, t1 0) or a band of a
   ring (r0 to r1, over turns t0 to t1 in degrees from across, down +),
   round (cx, cy) in texels from the face's middle, down +. The rim: the
   body colour (7) a texel wide, dark (F) round its bottom and lower left;
   inside it a ring of B, grey (C) round its upper left when plain. The
   white (E) a little left of the middle. Plain, the pupil black in a ring
   of B: a dark arch round its top, a grey fleck and a white glint, a grey
   crescent along its bottom. Startled, the ring of B round a grey one:
   inside, a dark arch round a grey middle, white below. The lids: the
   body colour above the middle, its crease a dark arc with a light shine
   inside it, the lash dark along its edge; shut, the whole face 7, lighter
   (8) inside, the crease and the shine as the lid's, the rim dark round
   its lower left. */
typedef struct {
    float cx, cy, r0, r1, t0, t1;
    uint8_t nib;
} Mark;
#define DISC(x, y, r, c) { x, y, 0, r, 0, 0, c }
#define BAND(x, y, r0, r1, t0, t1, c) { x, y, r0, r1, t0, t1, c }
#define RIM BAND(0, 0, 6.0f, 7.0f, 45, 200, 0xF), DISC(0, 0, 6.0f, 0xB)
#define RIM_PLAIN RIM, BAND(0, 0, 5.0f, 6.0f, 200, 300, 0xC)
#define PUPIL_PLAIN \
    DISC(-0.4f, 0.5f, 4.6f, 0xE), DISC(0, 0, 3.0f, 0xB), DISC(0, 0, 2.2f, 0xF), \
    BAND(0, 0.3f, 1.4f, 2.3f, 40, 140, 0xC), DISC(0, -1.0f, 1.0f, 0xC), DISC(0.5f, -1.4f, 0.5f, 0xE)
#define PUPIL_STARTLED \
    DISC(-0.1f, 0.2f, 5.0f, 0xE), DISC(0, 0.5f, 2.9f, 0xB), DISC(0, 0.5f, 2.3f, 0xC), DISC(0, 2.0f, 1.0f, 0xE), \
    BAND(0, 0.5f, 0.9f, 1.6f, 180, 360, 0xF), DISC(0, 0.5f, 0.8f, 0xC)
#define LID \
    BAND(0, 0, 0, 7.0f, 180, 360, 0x7), BAND(0, 0, 4.7f, 5.6f, 200, 340, 0xF), \
    DISC(0, -3.0f, 1.5f, 0x8), DISC(0, -3.0f, 0.8f, 0xB), BAND(0, 49.2f, 49.2f, 50.0f, 262, 278, 0xF)
static const Mark eye_plain[] = { RIM_PLAIN, PUPIL_PLAIN };
static const Mark eye_startled[] = { RIM, PUPIL_STARTLED };
static const Mark eye_half[] = { RIM_PLAIN, PUPIL_PLAIN, LID };
static const Mark eye_half_startled[] = { RIM, PUPIL_STARTLED, LID };
static const Mark eye_shut[] = { DISC(0, 0, 7.0f, 0x7), DISC(0, 0.5f, 5.0f, 0x8), BAND(0, 0, 4.7f, 5.6f, 200, 340, 0xF),
                                 BAND(0, 0, 2.2f, 3.0f, 210, 330, 0xB), BAND(0, 0, 6.0f, 7.0f, 90, 200, 0xF) };

#define N(a) (int)(sizeof a / sizeof a[0])
static const Solid solids[] = {
    { 0xA1C8, 32, 0, tyre_rings, N(tyre_rings), tyre_side, tyre_side, N(tyre_side), N(tyre_side), 0 },
    { 0xA1E4, 32, 0, tyre_rings, N(tyre_rings), tyre_side, tyre_side, N(tyre_side), N(tyre_side), 0 },
    { 0xA190, 16, 2, lamp_rings, N(lamp_rings), lamp_lens, lamp_back, N(lamp_lens), N(lamp_back), 0 },
    { 0xA104, 32, 1, NULL, 0, NULL, NULL, 0, 0, 1 },
    { 0xA120, 64, 1, NULL, 0, NULL, NULL, 0, 0, 2 },
    { 0xA0E8, 64, 1, NULL, 0, NULL, NULL, 0, 0, 2 },     /* Night Cruise's bush: a canopy to the ground, no trunk */
};

static const Solid *object, *armed;
static const Mark *face;                /* an eye's face for its look */
static int nface, record_index, cap_seen;
static int far_index;                   /* the far record a far object's slot holds (its gaze: draw_record_gaze_far), else -1 */
static uint16_t far_slot;               /* that slot's address: its look is in the slot's own record */
static int coloured, two, tyre_row, size_halved;     /* size_halved: its size halved with its place (a far object's) */
/* the object's place and the view's turn made exactly (fx_face_exact_object):
   its place whole, never halved; else the chip's, halved `halving` times */
static int exact_view, halving;
static double exact_turn[3][3];
static uint8_t hi, tex_bank, tex_nib;
static uint16_t tex_base;
static float pos[3], view[3][3], scale;

static uint16_t word(uint16_t a)
{
    return (uint16_t)(fx_ram[fx_ram_at(a)] | fx_ram[fx_ram_at((uint16_t)(a ^ 1))] << 8);
}

/* the output's pixels a picture pixel: how fine a solid's round must be */
static float output_focal(void)
{
    float sx, sy;

    fx_draw_world_scale(&sx, &sy);
    return 128.0f * (sx > sy ? sx : sy);
}

/* a face of n view-space points in colour c, into the picture */
static void emit(const float (*p)[3], int n, uint8_t c)
{
    static FxCorner corners[3 * (SEGMENTS_MAX + 8)];
    uint32_t k = fx_face_triangles(p, n, corners, N(corners));

    if (!k)
        return;
    if (tyre_row)
        fx_draw_flat_entry(corners, k, (uint8_t)(0xE0 | (c & 0x0F)));  /* a tyre's: row 14 */
    else
        fx_draw_flat(corners, k, c);
}

/* FxObjectsDraw's object: its shape (the draw record's model) and its draw
   record's address (the n-th record from $0AAA, 30 bytes each) */
void fx_solid_object(uint16_t shape, uint16_t record)
{
    int i;

    object = NULL;
    far_index = fx_far_record_number(record);
    far_slot = record;
    record = fx_far_original(record);   /* in a far or parked pass, the near record the slot holds */
    record_index = !record || (uint16_t)(record - 0x0AAA) % 30u ? -1 : (int)((uint16_t)(record - 0x0AAA) / 30u);
    {
        /* the vehicles as the original drew them (a launcher's choice,
           SRFX_ORIGINAL_CARS=1): their tyres, wheels and eyes stay sprites */
        static int original = -1;
        if (original < 0)
            original = getenv("SRFX_ORIGINAL_CARS") && getenv("SRFX_ORIGINAL_CARS")[0] == '1';
        fx_draw_sprite_lift(0);
        if (original && (shape == 0xA1C8 || shape == 0xA1E4 || shape == 0xA190)) {
            fx_draw_sprite_lift(80.0f);  /* (two tyres' radii: clear of the road, short of a car's length) */
            return;
        }
    }
    for (i = 0; i < N(solids); i++)
        if (solids[i].shape == shape)
            object = &solids[i];
}

/* The rotation the model drawer turns a model's points by (model.c): the
   matrix of the angles at $0020 (Z), $0022 (X), $0024 (Y), each the byte
   negated as a turn's high byte with its fraction between steps
   (FxMatrixFromAngles01's rows), transposed, times the view's at $00E4
   (FxMatrixApplyVectors). A point p in the model is p . view in view
   space. */
static void turn(void)
{
    float a[3], s[3], c[3], m[3][3], e[3][3];
    int i, j, k;

    for (i = 0; i < 3; i++) {
        uint16_t w = (uint16_t)-(int)((word((uint16_t)(0x0020 + 2 * i)) & 0xFF) << 8 | fx_angle_fraction(i));

        a[i] = (float)w / 65536.0f * 2 * PI_F;
        s[i] = sinf(a[i]);
        c[i] = cosf(a[i]);
    }
    {
        float sz = s[0], cz = c[0], sx = s[1], cx = c[1], sy = s[2], cy = c[2];

        m[0][0] = sx * sy * sz + cx * cy; m[0][1] = sx * cy * sz - cx * sy; m[0][2] = sx * cz;
        m[1][0] = sy * cz;                m[1][1] = cy * cz;                m[1][2] = -sz;
        m[2][0] = cx * sy * sz - sx * cy; m[2][1] = cx * cy * sz + sx * sy; m[2][2] = cx * cz;
    }
    for (i = 0; i < 3; i++)
        for (j = 0; j < 3; j++)
            e[i][j] = exact_view ? (float)exact_turn[i][j] : (int16_t)word((uint16_t)(0x00E4 + 6 * i + 2 * j)) / 32768.0f;
    for (i = 0; i < 3; i++)
        for (j = 0; j < 3; j++)
            for (view[i][j] = 0, k = 0; k < 3; k++)
                view[i][j] += m[k][i] * e[k][j];
}

/* FxObjectSprite past its depth test: a solid's place, size and turn kept,
   its sprite's quads left out */
void fx_solid_begin(void)
{
    int32_t own, size;
    int i;

    armed = NULL;
    coloured = 0;
    face = NULL;
    if (!object)
        return;
    two = !fx_draw_full_colour();
    if (two && (object->tree || !fx_draw_two_players()))
        return;
    tyre_row = two && object->shape != 0xA190;
    if (object->shape == 0xA190) {      /* an eye: by its gaze, in its look */
        uint16_t look, gz = record_index >= 0 ? draw_record_gaze(record_index) : far_index >= 0 ? draw_record_gaze_far((unsigned)far_index) : 0;
        float phi, d[3];

        if (!(gz & 0x100))
            return;
        /* the record's +$16: the eye's anim word, from the slot being drawn, which holds the
           eye's own record: the near record's place holds another object's once the merged
           pass has filled the slots in depth order */
        look = word((uint16_t)(far_slot + 0x16));
        switch (look) {
        case 0x9AAF: face = eye_plain, nface = N(eye_plain); break;
        case 0x9AB1: face = eye_startled, nface = N(eye_startled); break;
        case 0x9AB3: face = eye_half, nface = N(eye_half); break;
        case 0x9AB5: face = eye_shut, nface = N(eye_shut); break;
        case 0x9AB7: face = eye_half_startled, nface = N(eye_half_startled); break;
        default: return;
        }
        phi = (119.5f - (float)(gz & 0xFF)) * 2 * PI_F / 256;
        d[0] = -sinf(phi), d[1] = 0, d[2] = -cosf(phi);
        view[0][0] = d[2], view[0][1] = 0, view[0][2] = -d[0];      /* across: down x ahead */
        view[1][0] = 0, view[1][1] = 1, view[1][2] = 0;             /* down: the view's */
        view[2][0] = d[0], view[2][1] = d[1], view[2][2] = d[2];    /* the lens */
    }
    pos[0] = (int16_t)word(0x0026);
    pos[1] = (int16_t)word(0x0028);
    pos[2] = (int16_t)word(0x002A);
    {                                   /* the place made exactly, whole (fx_face.c) */
        double place[3];

        exact_view = fx_face_exact_object(place, exact_turn);
        if (exact_view)
            for (i = 0; i < 3; i++)
                pos[i] = (float)place[i];
    }
    halving = exact_view ? 0 : fx_far_shift();
    /* the sprite's size (01:B100, B128): a negative own size alone, else
       the shape's twice and the own shifted by $0032; 0 as 1 */
    own = (int16_t)word(FX_MODEL_CE);
    size = own < 0 ? -own : 2 * word(FX_SPRITE_SIZE) + (own << (word(0x0032) & 15));
    size_halved = own >= 0 && halving;
    if (size_halved)                    /* a far object's, halved with its place (fx_far.c) */
        size >>= halving;
    scale = (float)(size ? size : 1) / object->texels;
    if (exact_view && !object->tree && object->rings) {     /* a car's tyre seated on the road (fx_face.c) */
        float rmax = 0;

        for (i = 0; i < object->nrings; i++)
            rmax = object->rings[i].r > rmax ? object->rings[i].r : rmax;
        fx_face_seat_tyre(pos, rmax * scale);
    }
    if (face) {
        /* turned by its gaze above */
    } else if (object->tree) {          /* upright in the world: the view's matrix alone */
        int i, j;

        for (i = 0; i < 3; i++)
            for (j = 0; j < 3; j++)
                view[i][j] = exact_view ? (float)exact_turn[i][j] : (int16_t)word((uint16_t)(0x00E4 + 6 * i + 2 * j)) / 32768.0f;
    } else {
        turn();
    }
    armed = object;
}

int fx_solid_hiding(void)
{
    return armed != NULL;
}

/* its material read (FxSpriteMaterial, COLOR and CMODE): its colour's high
   nibble, and its painting, which shapes a tree */
void fx_solid_colour(uint8_t colr, uint8_t por)
{
    if (!armed)
        return;
    hi = (uint8_t)(colr & 0xF0);
    tex_bank = (uint8_t)word(FX_TEX_BANK);
    tex_base = word(FX_TEX_BASE);
    tex_nib = (por & 0x04) ? 1 : 0;     /* COLOR's: the high nibble, else the low */
    coloured = 1;
}

/* ---- tyres and wheels ---- */

/* a point of the solid, along its axis a, round it at turn t, radius r, in
   view space */
static void place(const Solid *s, float a, float t, float r, float *out)
{
    float m[3];
    int j;

    m[s->axis] = a;
    m[s->axis ? 0 : 1] = r * cosf(t);
    m[s->axis ? 1 : 2] = r * sinf(t);
    for (j = 0; j < 3; j++)
        out[j] = pos[j] + scale * (m[0] * view[0][j] + m[1] * view[1][j] + m[2] * view[2][j]);
}

/* a face's outward normal (from its first three points, turned away from
   the solid's centre) and whether it is seen: it faces the eye */
static int face_seen(const float (*p)[3], int n, float *nv)
{
    float u[3], v[3], c[3], d = 0, e = 0;
    int i, j;

    for (j = 0; j < 3; j++) {
        u[j] = p[1][j] - p[0][j];
        v[j] = p[n - 1][j] - p[0][j];
        for (c[j] = 0, i = 0; i < n; i++)
            c[j] += p[i][j];
        c[j] /= n;
    }
    nv[0] = u[1] * v[2] - u[2] * v[1];
    nv[1] = u[2] * v[0] - u[0] * v[2];
    nv[2] = u[0] * v[1] - u[1] * v[0];
    for (j = 0; j < 3; j++)
        d += nv[j] * (c[j] - pos[j]);
    if (d < 0)
        for (j = 0; j < 3; j++)
            nv[j] = -nv[j];
    for (j = 0; j < 3; j++)
        e += nv[j] * c[j];
    return e < 0;
}

static uint8_t shade(const Ring *g, const float *n)
{
    float up = -n[1] / sqrtf(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);   /* view y is down */

    if (!g->up)
        return g->nib;
    if (g->top && up > 0.8f)
        return g->top;
    return up > 0.45f ? g->up : up < -0.45f ? g->down : g->across;
}

/* a cap at a, its rings over it outward in, when it faces the eye; with
   the depth test each disc one layer in front of the one under it, on the
   same plane (fx_draw_layer) */
static void cap(const Solid *s, float a, const Disc *d, int nd, int n)
{
    static float p[SEGMENTS_MAX][3];
    float nv[3];
    int i, k;

    for (k = 0; k < nd; k++) {
        int m = k ? (n / 2 < 8 ? 8 : n / 2) : n;

        for (i = 0; i < m; i++)
            place(s, a, 2 * PI_F * i / m, d[k].r, p[i]);
        if (!k && !(cap_seen = face_seen((const float (*)[3])p, m, nv)))
            return;
        fx_draw_layer(k);
        emit((const float (*)[3])p, m, (uint8_t)(hi | d[k].nib));
    }
    fx_draw_layer(0);
}

/* an eye's face's marks on its face plane at a, in order: with the depth
   test each a layer in front of the last, the first in front of the cap's
   `base` discs under them */
static void eye_marks(const Solid *s, float a, const Mark *mk, int nm, int n, int base)
{
    static float p[SEGMENTS_MAX][3];
    float q[4][3];
    int k, i;

    for (k = 0; k < nm; k++) {
        const Mark *m = &mk[k];

        fx_draw_layer(base + k);
        if (m->t0 == m->t1) {           /* a disc */
            int sides = m->r1 < 1.5f ? 12 : n;

            for (i = 0; i < sides; i++) {
                float t = 2 * PI_F * i / sides, x = m->cx + m->r1 * cosf(t), y = m->cy + m->r1 * sinf(t);

                place(s, a, atan2f(y, x), hypotf(x, y), p[i]);
            }
            emit((const float (*)[3])p, sides, (uint8_t)(hi | m->nib));
            continue;
        }
        {                               /* a band of a ring */
            float t0 = m->t0 * PI_F / 180, t1 = m->t1 * PI_F / 180;
            int steps = (int)ceilf(n * (t1 - t0) / (2 * PI_F));

            if (steps < 2)
                steps = 2;
            for (i = 0; i < steps; i++) {
                float u0 = t0 + (t1 - t0) * i / steps, u1 = t0 + (t1 - t0) * (i + 1) / steps;
                float xy[4][2] = { { m->cx + m->r0 * cosf(u0), m->cy + m->r0 * sinf(u0) },
                                   { m->cx + m->r0 * cosf(u1), m->cy + m->r0 * sinf(u1) },
                                   { m->cx + m->r1 * cosf(u1), m->cy + m->r1 * sinf(u1) },
                                   { m->cx + m->r1 * cosf(u0), m->cy + m->r1 * sinf(u0) } };
                int j, np = m->r0 > 0 ? 4 : 3;  /* from the middle: a wedge, its first two corners one */

                for (j = 0; j < np; j++) {
                    int jj = m->r0 > 0 ? j : j + 1;

                    place(s, a, atan2f(xy[jj][1], xy[jj][0]), hypotf(xy[jj][0], xy[jj][1]), q[j]);
                }
                emit((const float (*)[3])q, np, (uint8_t)(hi | m->nib));
            }
        }
    }
    fx_draw_layer(0);
}

static void draw(const Solid *s)
{
    float rmax = 0, pr, p[4][3], nv[3];
    int n, i, k;

    /* segments enough that the rim's flats are within half an output pixel
       of the circle: n >= pi / acos(1 - 0.5 / r), r its radius on the
       output */
    for (k = 0; k < s->nrings; k++)
        if (s->rings[k].r > rmax)
            rmax = s->rings[k].r;
    pr = pos[2] > 0 ? rmax * scale * output_focal() / pos[2] : rmax;
    n = pr > 0.5f ? (int)ceilf(PI_F / acosf(1 - 0.5f / pr)) : 8;
    n = (n + 3) & ~3;
    if (n < 8)
        n = 8;
    if (n > SEGMENTS_MAX)
        n = SEGMENTS_MAX;
    /* the bands: a convex solid, so the faces it shows never overlap */
    for (k = 0; k + 1 < s->nrings; k++) {
        const Ring *g = &s->rings[k], *h = &s->rings[k + 1];

        for (i = 0; i < n; i++) {
            float t0 = 2 * PI_F * i / n, t1 = 2 * PI_F * (i + 1) / n;

            place(s, g->a, t0, g->r, p[0]);
            place(s, g->a, t1, g->r, p[1]);
            place(s, h->a, t1, h->r, p[2]);
            place(s, h->a, t0, h->r, p[3]);
            if (face_seen((const float (*)[3])p, 4, nv))
                emit((const float (*)[3])p, 4, (uint8_t)(hi | shade(g, nv)));
        }
    }
    cap(s, s->rings[s->nrings - 1].a, s->front, s->nfront, n);
    if (face && cap_seen)
        eye_marks(s, s->rings[s->nrings - 1].a, face, nface, n, s->nfront);
    cap(s, s->rings[0].a, s->back, s->nback, n);
}

/* ---- trees ---- */
/*
 * A tree in the race's polygon style, of its paintings' size and shape:
 * its trunk and its canopy each turned about the world's upright, their
 * half widths from their paintings (the trunk every two rows in ten
 * sides, the canopy every three in 44, the widest of each), the canopy's
 * outline eased ring to ring and every other ring turned half a side, so
 * its facets interlock. Flat faces in the paintings' colours by how far
 * each faces the light, from where the painting is lit (where its lighter
 * colours lie): each colour from a light on, set once for the painting by
 * drawing the part from the front (its faces seen, least lit first, taking
 * the painting's colours darkest first, each for its share of the painting
 * by area on the screen), the same every picture after. A painting faces
 * the view whatever the tree's angles, so a tree stands upright.
 */
#define PAINT_ROWS 64
typedef struct {
    int used, kind, ready;              /* the painting it is, measured once; its colours' lights set */
    uint8_t bank, nib;
    uint16_t base;
    float half[PAINT_ROWS], light[3], share[16], from[16];
    int top, bottom, ncolours;
    uint8_t colour[16];                 /* darkest (the higher nibble) first */
} Paint;

static uint8_t texel(int u, int v)
{
    return (uint8_t)((fx_rom_read(tex_bank, (uint16_t)(tex_base + (v << 8) + u)) >> (4 * tex_nib)) & 15);
}

static void paint_measure(const Solid *s, Paint *p)
{
    float count[16] = { 0 }, total = 0, mean = 0, cx = 0, cy = 0, lx = 0, ly = 0, l;
    int u, v, k, T = s->texels;
    uint8_t c, rank[16] = { 0 };

    memset(p, 0, sizeof *p);
    p->top = -1;
    for (v = 0; v < T; v++) {
        int left = -1, right = -1;

        for (u = 0; u < T; u++)
            if ((c = texel(u, v))) {
                if (left < 0)
                    left = u;
                right = u;
                count[c]++, total++;
            }
        p->half[v] = left < 0 ? 0 : (right + 1 - left) / 2.0f;
        if (left >= 0) {
            if (p->top < 0)
                p->top = v;
            p->bottom = v;
        }
    }
    for (c = 15; c >= 1; c--)
        if (count[c])
            rank[c] = (uint8_t)p->ncolours, p->colour[p->ncolours++] = c;
    if (!total)
        return;
    for (k = 0; k < p->ncolours; k++)
        p->share[k] = count[p->colour[k]] / total;
    for (v = 0; v < T; v++)
        for (u = 0; u < T; u++)
            if ((c = texel(u, v)))
                mean += rank[c], cx += u + 0.5f, cy += v + 0.5f;
    mean /= total, cx /= total, cy /= total;
    for (v = 0; v < T; v++)
        for (u = 0; u < T; u++)
            if ((c = texel(u, v)))
                lx += (rank[c] - mean) * (u + 0.5f - cx), ly += (rank[c] - mean) * (v + 0.5f - cy);
    l = sqrtf(lx * lx + ly * ly);
    p->light[0] = l ? 0.75f * lx / l : 0, p->light[1] = l ? 0.75f * ly / l : 0, p->light[2] = -0.66f;
    l = sqrtf(p->light[0] * p->light[0] + p->light[1] * p->light[1] + p->light[2] * p->light[2]);
    for (k = 0; k < 3; k++)
        p->light[k] /= l;
}

/* a point of the tree (x and z across, y the world's down, texels from its
   painting's middle) in view space */
static void tree_point(float x, float y, float z, float *out)
{
    int j;

    for (j = 0; j < 3; j++)
        out[j] = pos[j] + scale * (x * view[0][j] + y * view[1][j] + z * view[2][j]);
}

#define TREE_FACES 4096
typedef struct {
    float p[4][3], key, lit, area;
    int n;
    uint8_t c;
} TreeFace;
static TreeFace faces[TREE_FACES];
static TreeFace *by_lit[TREE_FACES];

static int face_cmp(const void *a, const void *b)
{
    float x = ((const TreeFace *)a)->key, y = ((const TreeFace *)b)->key;

    return x > y ? -1 : x < y;
}

static int lit_cmp(const void *a, const void *b)
{
    float x = (*(TreeFace *const *)a)->lit, y = (*(TreeFace *const *)b)->lit;

    return x < y ? -1 : x > y;
}

#define TREE_SIDES_MAX 48
#define RINGS_MAX (PAINT_ROWS / 2 + 4)
static int nf, calibrating;
static float total_area;

/* a face of m points kept when seen, outward away from the upright at down
   ay; its light against the painting's and its area on the screen */
static void keep_face(const float (*q)[3], int m, float ay, const Paint *pt)
{
    float u[3], v[3], nv[3], c[3] = { 0, 0, 0 }, axis[3], d = 0, e = 0, l;
    TreeFace *f;
    int i, j;

    if (nf >= TREE_FACES)
        return;
    f = &faces[nf];
    for (j = 0; j < 3; j++) {
        u[j] = q[1][j] - q[0][j];
        v[j] = q[m - 1][j] - q[0][j];
        for (i = 0; i < m; i++)
            c[j] += q[i][j] / m;
    }
    nv[0] = u[1] * v[2] - u[2] * v[1];
    nv[1] = u[2] * v[0] - u[0] * v[2];
    nv[2] = u[0] * v[1] - u[1] * v[0];
    tree_point(0, ay, 0, axis);
    for (j = 0; j < 3; j++)
        d += nv[j] * (c[j] - axis[j]);
    if (d < 0)                          /* outward: away from the upright */
        for (j = 0; j < 3; j++)
            nv[j] = -nv[j];
    for (j = 0; j < 3; j++)
        e += nv[j] * c[j];
    if (e >= 0)
        return;
    l = sqrtf(nv[0] * nv[0] + nv[1] * nv[1] + nv[2] * nv[2]);
    f->lit = l ? (nv[0] * pt->light[0] + nv[1] * pt->light[1] + nv[2] * pt->light[2]) / l : -1;
    for (i = 0; i < m; i++)
        memcpy(f->p[i], q[i], sizeof f->p[i]);
    f->n = m;
    f->key = c[0] * c[0] + c[1] * c[1] + c[2] * c[2];
    f->area = 0;
    if (calibrating) {                  /* its area on the screen, for the painting's colours */
        for (j = 0; j < m; j++) {
            const float *a = q[j], *b = q[(j + 1) % m];

            if (a[2] > 0 && b[2] > 0)
                f->area += (a[0] / a[2]) * (b[1] / b[2]) - (b[0] / b[2]) * (a[1] / a[2]);
        }
        f->area = fabsf(f->area) * 128.0f * 128.0f / 2;
    }
    total_area += f->area;
    nf++;
}

/* The tree's points: ring k's point c (0 to sides, point sides point 0 one
   turn on) at radius r[k] times jag[k][c mod sides], angle 2 pi (c + o) /
   sides (o half a side on a turned ring), down y[k] + rise[k][c mod sides] */
static void tree_model(float (*m)[TREE_SIDES_MAX + 1][3], const float *y, const float *r, int nr, int sides,
                       const float (*jag)[TREE_SIDES_MAX], const float (*rise)[TREE_SIDES_MAX], int stagger)
{
    int k, c;

    for (k = 0; k < nr; k++) {
        float o = stagger && (k & 1) ? 0.5f : 0;

        for (c = 0; c <= sides; c++) {
            int w = c % sides;
            float ra = r[k] * jag[k][w], t = 2 * PI_F * (c + o) / sides;

            m[k][c][0] = ra * cosf(t);
            m[k][c][1] = y[k] + rise[k][w];
            m[k][c][2] = ra * sinf(t);
        }
    }
}

static float placed[RINGS_MAX][TREE_SIDES_MAX + 1][3];

/* a body turned about the upright, its seen faces drawn farthest first (it
   need not be convex), every other ring turned half a side when staggered */
static void tree_body(const float *y, int nr, int sides, float (*m)[TREE_SIDES_MAX + 1][3], const float *r,
                      int stagger, Paint *pt)
{
    int k, i;
    float at;

    nf = 0, total_area = 0;
    for (k = 0; k < nr; k++)
        for (i = 0; i <= sides; i++)
            tree_point(m[k][i][0], m[k][i][1], m[k][i][2], placed[k][i]);
    for (k = 0; k + 1 < nr; k++) {
        float o0 = stagger && (k & 1) ? 0.5f : 0, o1 = stagger && ((k + 1) & 1) ? 0.5f : 0, ay = (y[k] + y[k + 1]) / 2;

        for (i = 0; i < sides; i++) {
            const float *a0 = placed[k][i], *a1 = placed[k][i + 1], *b0 = placed[k + 1][i], *b1 = placed[k + 1][i + 1];
            float q[4][3];

            if (!r[k]) {                /* a closed end: triangles to its point */
                memcpy(q[0], a0, sizeof q[0]), memcpy(q[1], b1, sizeof q[0]), memcpy(q[2], b0, sizeof q[0]);
                keep_face((const float (*)[3])q, 3, ay, pt);
            } else if (!r[k + 1]) {
                memcpy(q[0], a0, sizeof q[0]), memcpy(q[1], a1, sizeof q[0]), memcpy(q[2], b0, sizeof q[0]);
                keep_face((const float (*)[3])q, 3, ay, pt);
            } else if (o0 == o1) {      /* a band of four-sided faces */
                memcpy(q[0], a0, sizeof q[0]), memcpy(q[1], a1, sizeof q[0]), memcpy(q[2], b1, sizeof q[0]), memcpy(q[3], b0, sizeof q[0]);
                keep_face((const float (*)[3])q, 4, ay, pt);
            } else if (o1 > o0) {       /* the next ring's point beside this edge: b0 */
                memcpy(q[0], a0, sizeof q[0]), memcpy(q[1], a1, sizeof q[0]), memcpy(q[2], b0, sizeof q[0]);
                keep_face((const float (*)[3])q, 3, ay, pt);
                memcpy(q[0], b0, sizeof q[0]), memcpy(q[1], a1, sizeof q[0]), memcpy(q[2], b1, sizeof q[0]);
                keep_face((const float (*)[3])q, 3, ay, pt);
            } else {                    /* this ring turned: the next ring's b1 beside this edge */
                memcpy(q[0], a0, sizeof q[0]), memcpy(q[1], a1, sizeof q[0]), memcpy(q[2], b1, sizeof q[0]);
                keep_face((const float (*)[3])q, 3, ay, pt);
                memcpy(q[0], a0, sizeof q[0]), memcpy(q[1], b1, sizeof q[0]), memcpy(q[2], b0, sizeof q[0]);
                keep_face((const float (*)[3])q, 3, ay, pt);
            }
        }
    }
    if (calibrating) {                  /* each colour's light: where the faces, least lit first, pass its share */
        float edge;

        for (i = 0; i < nf; i++)
            by_lit[i] = &faces[i];
        qsort(by_lit, (size_t)nf, sizeof by_lit[0], lit_cmp);
        pt->from[0] = -2;
        for (k = 1, i = 0, at = 0, edge = pt->share[0] * total_area; k < pt->ncolours; k++, edge += pt->share[k - 1] * total_area) {
            while (i < nf && at + by_lit[i]->area / 2 <= edge)
                at += by_lit[i++]->area;
            pt->from[k] = i < nf ? by_lit[i]->lit : 2;
        }
        return;
    }
    for (i = 0; i < nf; i++) {          /* each face's colour by its light */
        for (k = pt->ncolours - 1; k > 0 && faces[i].lit < pt->from[k]; k--)
            ;
        faces[i].c = pt->colour[k];
    }
    qsort(faces, (size_t)nf, sizeof faces[0], face_cmp);
    for (i = 0; i < nf; i++)
        emit((const float (*)[3])faces[i].p, faces[i].n, (uint8_t)(hi | faces[i].c));
}

/* A tree drawn at the detail its size on the output asks: sides enough that
   its flats are within half an output pixel of round at its widest, on four
   levels (the full count, a half, a quarter, an eighth, at least 5); its
   rings merged two, four or eight painting rows at a time while they would
   come closer than 1.5 output pixels apart. Each level made once for its
   painting; the colours' lights set from the full detail, so a face's
   colour means the same light at every level. */
typedef struct {
    int ok, nr, sides;
    float T, y[RINGS_MAX], r[RINGS_MAX];
    float (*m)[TREE_SIDES_MAX + 1][3];
} TreeLevel;
static TreeLevel levels[4][4][4];       /* painting, sides level, rings level */

static TreeLevel *tree_level(const Solid *s, Paint *pp, int paint, int sl, int rl)
{
    static float jag[RINGS_MAX][TREE_SIDES_MAX], rise[RINGS_MAX][TREE_SIDES_MAX];
    TreeLevel *L = &levels[paint][sl][rl];
    float T = (float)s->texels, *y = L->y, *r = L->r;
    int full = s->tree == 1 ? 10 : 44, sides = full >> sl, step = (s->tree == 1 ? 2 : 3) << rl, nr = 0, v, k, i;

    if (L->ok && L->T == T)
        return L;
    if (sides < 5)
        sides = 5;
    y[nr] = pp->top - T / 2, r[nr++] = 0;
    for (v = pp->top; v <= pp->bottom && nr < RINGS_MAX - 1; v += step) {
        float h = 0;

        for (k = v; k < v + step && k <= pp->bottom; k++)
            if (pp->half[k] > h)
                h = pp->half[k];
        y[nr] = v + step / 2.0f - T / 2, r[nr++] = h;
    }
    if (s->tree == 2) {                 /* the canopy's outline eased: each ring with its neighbours */
        float was[RINGS_MAX];

        memcpy(was, r, sizeof was);
        for (k = 1; k + 1 < nr; k++)
            r[k] = 0.25f * was[k - 1] + 0.5f * was[k] + 0.25f * was[k + 1];
    }
    y[nr] = pp->bottom + 1 - T / 2, r[nr++] = 0;
    for (k = 0; k < nr; k++)            /* the canopy's points in and out and up and down a little, the same for every tree */
        for (i = 0; i < sides; i++) {
            unsigned hsh = (unsigned)(k * 7919 + i * 104729) * 2654435761u;

            jag[k][i] = s->tree == 2 ? 0.91f + 0.18f * ((hsh >> 16) & 0xFF) / 255.0f : 1.0f;
            rise[k][i] = s->tree == 2 && r[k] ? 1.6f * (((hsh >> 8) & 0xFF) / 255.0f - 0.5f) : 0;
        }
    if (!L->m && !(L->m = malloc(sizeof *L->m * RINGS_MAX)))
        return NULL;
    tree_model(L->m, y, r, nr, sides, (const float (*)[TREE_SIDES_MAX])jag, (const float (*)[TREE_SIDES_MAX])rise, s->tree == 2);
    L->nr = nr;
    L->sides = sides;
    L->T = T;
    L->ok = 1;
    return L;
}

static Paint paints[4];

/* the painting the tree's sprite shows (the bank, base and nibble its
   material set), measured the first time it is seen; NULL when it has no
   texels */
static Paint *tree_paint(const Solid *s)
{
    Paint *pp = NULL;
    int i, sl, rl;

    for (i = 0; i < 4 && !pp; i++)
        if (paints[i].used && paints[i].kind == s->tree && paints[i].bank == tex_bank && paints[i].base == tex_base
            && paints[i].nib == tex_nib)
            pp = &paints[i];
    if (!pp) {
        for (i = 0; i < 4 && paints[i].used; i++)
            ;
        pp = &paints[i < 4 ? i : 0];
        paint_measure(s, pp);
        pp->used = 1, pp->kind = s->tree, pp->bank = tex_bank, pp->base = tex_base, pp->nib = tex_nib;
        for (sl = 0; sl < 4; sl++)
            for (rl = 0; rl < 4; rl++)
                levels[pp - paints][sl][rl].ok = 0;
    }
    return pp->ncolours && pp->top >= 0 ? pp : NULL;
}

/* the painting's widest half width, texels */
static float paint_half(const Paint *pp)
{
    float h = 0;
    int i;

    for (i = pp->top; i <= pp->bottom; i++)
        if (pp->half[i] > h)
            h = pp->half[i];
    return h;
}

/* the detail a part asks at px output pixels a painting texel: its sides
   level sl and its rings level rl (tree_level) */
static void tree_detail(const Solid *s, const Paint *pp, float px, int *sl, int *rl)
{
    float pr = paint_half(pp) * px;
    int full = s->tree == 1 ? 10 : 44, step = s->tree == 1 ? 2 : 3;
    int n = pr > 0.5f ? (int)ceilf(PI_F / acosf(1 - 0.5f / pr)) : 5;

    for (*sl = 3; *sl > 0 && (full >> *sl < 5 ? 5 : full >> *sl) < n; (*sl)--)
        ;
    for (*rl = 0; *rl < 3 && step * (1 << *rl) * px < 1.5f; (*rl)++)
        ;
}

/* the tree's part s in its painting pp at pos, scale and view, at the
   detail its size on the output asks */
static void tree_draw(const Solid *s, Paint *pp)
{
    TreeLevel *L;
    float px;
    int sl, rl, paint = (int)(pp - paints);

    if (!pp->ready) {                   /* its colours' lights, once, at the full detail: the part from the front, far off */
        float kp[3], kv[3][3], ks = scale;
        int a, b;

        if (!(L = tree_level(s, pp, paint, 0, 0)))
            return;
        memcpy(kp, pos, sizeof kp), memcpy(kv, view, sizeof kv);
        for (a = 0; a < 3; a++)
            for (b = 0; b < 3; b++)
                view[a][b] = a == b;
        pos[0] = pos[1] = 0, pos[2] = 1e5f, scale = 1;
        calibrating = 1;
        tree_body(L->y, L->nr, L->sides, L->m, L->r, s->tree == 2, pp);
        calibrating = 0;
        memcpy(pos, kp, sizeof kp), memcpy(view, kv, sizeof kv), scale = ks;
        pp->ready = 1;
    }
    /* its detail: output pixels a painting texel, at its depth */
    px = pos[2] > 0 ? scale * output_focal() / pos[2] : 1e9f;
    tree_detail(s, pp, px, &sl, &rl);
    if (!(L = tree_level(s, pp, paint, sl, rl)))
        return;
    tree_body(L->y, L->nr, L->sides, L->m, L->r, s->tree == 2, pp);
}

/* ---- the land's forest (fx_ground.c): copies of the course's tree ----

   Jeff (2026-11-12): King's Forest's land with "a ton of the trees", the
   course's own. A course's tree is two sprites, its trunk ($A104) and its
   canopy ($A120), each its own object at its own place. The first tree
   the race draws whose trunk
   and canopy stand together in one picture (the canopy's middle above the
   trunk's and within a quarter of the canopy's width of the trunk's
   upright, the nearest such) gives the land's tree: the two paintings and
   their colours' row, each part's size (world units a painting texel) and
   where the canopy's middle is from the trunk's in the world. A land tree
   is that, k times the size, at the detail a course tree of that size
   there would be drawn at (tree_detail): its model at that level (the
   course's own, tree_level) is an instanced mesh (nshvc 6.2), made once a
   level, its faces' colours set for the picture's view as tree_body sets
   a face's (its normal in view space against the painting's light), and
   every tree at that level one instance of it, placed and sized on the
   GPU (fx_solid_forest_end). A far tree's parts (their places halved for
   the far list, fx_far.c) pair as a near one's, their places and sizes
   doubled back, unless its size was its own, not halved with its place. */
#define FOREST_SEEN 16
static struct {
    int ready, course;
    int paint[3];                       /* by part (1 the trunk, 2 the canopy): its painting in paints[] */
    uint8_t bank[3], nib[3], hi[3];
    uint16_t base[3];
    float scale[3];                     /* world units a texel */
    float off[3];                       /* the canopy's middle from the trunk's, in the world's axes */
    uint8_t frame;                      /* the picture its parts so far were seen in */
    int seen[3];
    struct {
        float at[3], scale;
        int paint;
        uint8_t hi;
    } was[3][FOREST_SEEN];
} forest;

static int forest_paints_kept(void)
{
    int k;

    for (k = 1; k <= 2; k++) {
        const Paint *p = &paints[forest.paint[k]];

        if (!p->used || p->kind != k || p->bank != forest.bank[k] || p->base != forest.base[k] || p->nib != forest.nib[k])
            return 0;
    }
    return 1;
}

/* a near tree's part drawn: kept for the forest, or paired with the other
   part seen in this picture */
static void forest_see(const Solid *s, Paint *pp)
{
    int k = s->tree, o = 3 - k, i, j, best = -1;
    float best_across = 0, t2c[3] = { 0, 0, 0 }, at[3], unshift, sc;

    if (forest.course != g.course) {
        forest.ready = 0;
        forest.course = g.course;
        forest.seen[1] = forest.seen[2] = 0;
    }
    if (forest.ready || (halving && !size_halved))
        return;
    unshift = (float)(1 << halving);
    if (forest.frame != g.frame_count) {
        forest.frame = g.frame_count;
        forest.seen[1] = forest.seen[2] = 0;
    }
    for (j = 0; j < 3; j++)
        at[j] = pos[j] * unshift;
    sc = scale * unshift;
    for (i = 0; i < forest.seen[o]; i++) {
        float d[3], down = 0, all = 0, across, width;

        for (j = 0; j < 3; j++) {       /* trunk to canopy, in view space */
            d[j] = k == 2 ? at[j] - forest.was[o][i].at[j] : forest.was[o][i].at[j] - at[j];
            down += d[j] * view[1][j];
            all += d[j] * d[j];
        }
        across = sqrtf(fmaxf(all - down * down, 0));
        width = 2 * (k == 2 ? paint_half(pp) * sc : paint_half(&paints[forest.was[o][i].paint]) * forest.was[o][i].scale);
        if (down >= 0 || across > width / 4)
            continue;
        if (best < 0 || across < best_across) {
            best = i, best_across = across;
            memcpy(t2c, d, sizeof t2c);
        }
    }
    if (best < 0) {
        if (forest.seen[k] < FOREST_SEEN) {
            i = forest.seen[k]++;
            memcpy(forest.was[k][i].at, at, sizeof at);
            forest.was[k][i].scale = sc;
            forest.was[k][i].paint = (int)(pp - paints);
            forest.was[k][i].hi = hi;
        }
        return;
    }
    forest.paint[k] = (int)(pp - paints), forest.hi[k] = hi, forest.scale[k] = sc;
    forest.paint[o] = forest.was[o][best].paint, forest.hi[o] = forest.was[o][best].hi, forest.scale[o] = forest.was[o][best].scale;
    for (k = 1; k <= 2; k++) {
        const Paint *p = &paints[forest.paint[k]];

        forest.bank[k] = p->bank, forest.base[k] = p->base, forest.nib[k] = p->nib;
    }
    for (i = 0; i < 3; i++)             /* world axis i is the view's row i */
        forest.off[i] = t2c[0] * view[i][0] + t2c[1] * view[i][1] + t2c[2] * view[i][2];
    forest.ready = 1;
}

static void tree(const Solid *s)
{
    Paint *pp = tree_paint(s);

    if (!pp)
        return;
    forest_see(s, pp);
    tree_draw(s, pp);
}

int fx_solid_forest_ready(void)
{
    if (!forest.ready || forest.course != g.course || !forest_paints_kept())
        return 0;
    return 1;
}

static const Solid *tree_solid(int part)
{
    int i;

    for (i = 0; i < N(solids); i++)
        if (solids[i].tree == part)
            return &solids[i];
    return NULL;
}

/* the trunk's middle over its foot, world units (its painting's lowest
   row's bottom from its middle) */
static float trunk_rise(void)
{
    const Paint *pt = &paints[forest.paint[1]];

    return (pt->bottom + 1 - tree_solid(1)->texels / 2.0f) * forest.scale[1];
}

/* the canopy's middle (its painting's rows') from its painting's middle,
   down, world units */
static float canopy_mid(void)
{
    const Paint *pc = &paints[forest.paint[2]];

    return ((pc->top + pc->bottom + 1) / 2.0f - tree_solid(2)->texels / 2.0f) * forest.scale[2];
}

void fx_solid_forest_shape(FxForestShape *shape)
{
    const Paint *pt = &paints[forest.paint[1]], *pc = &paints[forest.paint[2]];

    shape->half = paint_half(pc) * forest.scale[2];
    shape->height = trunk_rise() - forest.off[1] + (tree_solid(2)->texels / 2.0f - pc->top) * forest.scale[2];
    shape->trunk_half = paint_half(pt) * forest.scale[1];
    shape->canopy_rise = trunk_rise() - forest.off[1] - canopy_mid();
    shape->canopy_half_height = (pc->bottom + 1 - pc->top) / 2.0f * forest.scale[2];
}

/* ---- the forest's meshes ----

   Each part's (1 the trunk, 2 the canopy) model at each detail level (its
   sides level and rings level, tree_level's), in its painting's texels
   about its middle along the world's axes (x and z across, y down, as
   tree_point places it), is a mesh: one for each picture being made
   (fx_draw_mesh_slot), so a picture shown keeps the colours written for its
   own view. A mesh's room is its level's most: (rings - 1) x sides x 2
   triangles, the rings at most a painting's rows by the level's step, two
   ends more. A picture's trees are gathered by part and level
   (fx_solid_forest_add) and each level used is written and drawn once
   (fx_solid_forest_end).

   Its cost on the GPU is its triangles and the pixels they cover, so a
   land tree is no finer than it shows (Jeff, 2026-11-12, on the forest at
   the course trees' detail: "framerate is pretty heavily hit ... heavily
   optimize"): the course trees' detail keeps a part's outline within half
   an output pixel of round and its rings 1.5 pixels apart, which made a
   far tree's triangles a pixel or two each, about a million of them a
   picture; a land tree's level keeps its outline within FOREST_FLAT_PX and
   its rings FOREST_RING_PX apart (forest_detail), a part under
   FOREST_LEAST_PX across its half width is left out (a far tree's trunk
   first), a part whose sphere is wholly outside the view's rectangle or
   behind the eye is left out, and each level's instances are drawn
   nearest first, the canopies before the trunks, so the GPU's depth test
   throws away what lies behind them before shading it. */
#define FOREST_FLAT_PX 2.0f
#define FOREST_RING_PX 6.0f
#define FOREST_LEAST_PX 0.6f
#define FOREST_TREES 4096
#define FOREST_SLOTS 5
static struct {
    nshvc_mesh mesh;
    uint32_t room;
} forest_mesh[FOREST_SLOTS][2][4][4];
typedef struct {
    float z;                            /* its depth, for the order */
    nshvc_instance in;
} ForestInstance;
static ForestInstance forest_inst[2][4][4][FOREST_TREES];
static nshvc_instance forest_sorted[FOREST_TREES];
static uint32_t forest_n[2][4][4];
static nshvc_view forest_view;
static float forest_plane[4][3];        /* the view's sides in view space, inward, unit */
static int forest_on;

/* the level a land tree's part takes at px output pixels a painting
   texel: its outline within FOREST_FLAT_PX of round, its rings at least
   FOREST_RING_PX apart */
static void forest_detail(const Solid *s, const Paint *pp, float px, int *sl, int *rl)
{
    float pr = paint_half(pp) * px;
    int full = s->tree == 1 ? 10 : 44, step = s->tree == 1 ? 2 : 3;
    int n = pr > FOREST_FLAT_PX ? (int)ceilf(PI_F / acosf(1 - FOREST_FLAT_PX / pr)) : 5;

    for (*sl = 3; *sl > 0 && (full >> *sl < 5 ? 5 : full >> *sl) < n; (*sl)--)
        ;
    for (*rl = 0; *rl < 3 && step * (1 << *rl) * px < FOREST_RING_PX; (*rl)++)
        ;
}

static int instance_nearer(const void *a, const void *b)
{
    float x = ((const ForestInstance *)a)->z, y = ((const ForestInstance *)b)->z;

    return x < y ? -1 : x > y;
}

static uint32_t level_room(const Solid *s, int sl, int rl)
{
    int full = s->tree == 1 ? 10 : 44, sides = full >> sl, step = (s->tree == 1 ? 2 : 3) << rl;
    int rings = PAINT_ROWS / step + 3;

    if (sides < 5)
        sides = 5;
    if (rings > RINGS_MAX)
        rings = RINGS_MAX;
    return (uint32_t)(rings - 1) * (uint32_t)sides * 6u;
}

/* a face of m (3 or 4) model points, outward away from the upright at down
   ay: its colour the painting's for its normal in view space against the
   painting's light (as keep_face and tree_body), its triangles into out */
static uint32_t mesh_face(nshvc_mesh_vertex *out, uint32_t n, const float *const *q, int m, float ay, const Paint *pt,
                          const float rot[9], uint8_t colour_hi)
{
    float u[3], v[3], nm[3], nv[3], c[3] = { 0, 0, 0 }, d, l, lit;
    uint32_t argb;
    uint16_t bgr;
    int i, j, k, t;
    static const int tri[2][3] = { { 0, 1, 2 }, { 0, 2, 3 } };

    for (j = 0; j < 3; j++) {
        u[j] = q[1][j] - q[0][j];
        v[j] = q[m - 1][j] - q[0][j];
        for (i = 0; i < m; i++)
            c[j] += q[i][j] / m;
    }
    nm[0] = u[1] * v[2] - u[2] * v[1];
    nm[1] = u[2] * v[0] - u[0] * v[2];
    nm[2] = u[0] * v[1] - u[1] * v[0];
    d = nm[0] * c[0] + nm[1] * (c[1] - ay) + nm[2] * c[2];
    if (d < 0)                          /* outward: away from the upright */
        for (j = 0; j < 3; j++)
            nm[j] = -nm[j];
    for (j = 0; j < 3; j++)
        nv[j] = rot[3 * j] * nm[0] + rot[3 * j + 1] * nm[1] + rot[3 * j + 2] * nm[2];
    l = sqrtf(nv[0] * nv[0] + nv[1] * nv[1] + nv[2] * nv[2]);
    lit = l ? (nv[0] * pt->light[0] + nv[1] * pt->light[1] + nv[2] * pt->light[2]) / l : -1;
    for (k = pt->ncolours - 1; k > 0 && lit < pt->from[k]; k--)
        ;
    bgr = pic.palette[(uint8_t)(colour_hi | pt->colour[k])];
    {
        unsigned r = bgr & 31, gg = bgr >> 5 & 31, b = bgr >> 10 & 31;

        argb = 0xFF000000u | (r << 3 | r >> 2) << 16 | (gg << 3 | gg >> 2) << 8 | (b << 3 | b >> 2);
    }
    for (t = 0; t < (m == 4 ? 2 : 1); t++) {
        /* wound clockwise on the screen seen from outside, as the game's
           instanced meshes are (fx_ground.c mesh_wound): (b - a) x (c - a)
           against the outward normal, else its last two the other way */
        const float *a = q[tri[t][0]], *b = q[tri[t][1]], *e = q[tri[t][2]];
        float e1[3] = { b[0] - a[0], b[1] - a[1], b[2] - a[2] }, e2[3] = { e[0] - a[0], e[1] - a[1], e[2] - a[2] };
        float tn[3] = { e1[1] * e2[2] - e1[2] * e2[1], e1[2] * e2[0] - e1[0] * e2[2], e1[0] * e2[1] - e1[1] * e2[0] };
        int flip = tn[0] * nm[0] + tn[1] * nm[1] + tn[2] * nm[2] > 0;

        for (i = 0; i < 3; i++) {
            const float *p = q[tri[t][flip && i ? 3 - i : i]];

            out[n].x = p[0], out[n].y = p[1], out[n].z = p[2];
            out[n].u = out[n].v = 0;
            out[n].colour = argb;
            n++;
        }
    }
    return n;
}

/* the level's model's faces, as tree_body makes them, into out */
static uint32_t mesh_faces(const Solid *s, const Paint *pt, const TreeLevel *L, const float rot[9], uint8_t colour_hi,
                           nshvc_mesh_vertex *out)
{
    int k, i, stagger = s->tree == 2;
    uint32_t n = 0;

    for (k = 0; k + 1 < L->nr; k++) {
        float o0 = stagger && (k & 1) ? 0.5f : 0, o1 = stagger && ((k + 1) & 1) ? 0.5f : 0, ay = (L->y[k] + L->y[k + 1]) / 2;

        for (i = 0; i < L->sides; i++) {
            const float *a0 = L->m[k][i], *a1 = L->m[k][i + 1], *b0 = L->m[k + 1][i], *b1 = L->m[k + 1][i + 1];

            if (!L->r[k]) {             /* a closed end: triangles to its point */
                const float *q[3] = { a0, b1, b0 };

                n = mesh_face(out, n, q, 3, ay, pt, rot, colour_hi);
            } else if (!L->r[k + 1]) {
                const float *q[3] = { a0, a1, b0 };

                n = mesh_face(out, n, q, 3, ay, pt, rot, colour_hi);
            } else if (o0 == o1) {      /* a band of four-sided faces */
                const float *q[4] = { a0, a1, b1, b0 };

                n = mesh_face(out, n, q, 4, ay, pt, rot, colour_hi);
            } else if (o1 > o0) {       /* the next ring's point beside this edge: b0 */
                const float *q[3] = { a0, a1, b0 }, *r2[3] = { b0, a1, b1 };

                n = mesh_face(out, n, q, 3, ay, pt, rot, colour_hi);
                n = mesh_face(out, n, r2, 3, ay, pt, rot, colour_hi);
            } else {                    /* this ring turned: the next ring's b1 beside this edge */
                const float *q[3] = { a0, a1, b1 }, *r2[3] = { a0, b1, b0 };

                n = mesh_face(out, n, q, 3, ay, pt, rot, colour_hi);
                n = mesh_face(out, n, r2, 3, ay, pt, rot, colour_hi);
            }
        }
    }
    return n;
}

void fx_solid_forest_begin(const nshvc_view *v, float left, float right, float top, float bottom)
{
    /* x = cx + f X / Z within [left, right]: f X - (left - cx) Z >= 0 and
       -f X + (right - cx) Z >= 0; y likewise */
    float p[4][3] = { { v->f, 0, -(left - v->cx) }, { -v->f, 0, right - v->cx },
                      { 0, v->f, -(top - v->cy) }, { 0, -v->f, bottom - v->cy } };
    int i, j;

    forest_view = *v;
    for (i = 0; i < 4; i++) {
        float l = sqrtf(p[i][0] * p[i][0] + p[i][1] * p[i][1] + p[i][2] * p[i][2]);

        for (j = 0; j < 3; j++)
            forest_plane[i][j] = l > 0 ? p[i][j] / l : 0;
    }
    memset(forest_n, 0, sizeof forest_n);
    forest_on = fx_solid_forest_ready() && fx_draw_mesh_slot() >= 0;
}

/* a tree, its trunk's foot at the world point foot, k times the course's
   tree's size: each part one instance of the level its size at its depth
   asks */
/* a forest part's sphere's radius by its size: about its painting's
   middle, to its widest half and its farthest row, its points moved out a
   little (jag 1.09, rise 0.8 of a texel) */
static float part_radius(const Solid *s, const Paint *pp)
{
    float t = s->texels / 2.0f, up = fabsf(pp->top - t), down = fabsf(pp->bottom + 1 - t), h = paint_half(pp);
    float far = up > down ? up : down;

    return 1.1f * sqrtf(h * h + far * far) + 1;
}

void fx_solid_forest_add(const double foot[3], float k)
{
    const float *rot = forest_view.rot;
    double place[3][3];
    float rise = trunk_rise() * k, focal = output_focal();
    int part, j;

    if (!forest_on)
        return;
    for (j = 0; j < 3; j++) {
        place[1][j] = foot[j] - (j == 1 ? rise : 0);    /* the trunk's middle, over its foot (y is down) */
        place[2][j] = place[1][j] + k * forest.off[j];  /* the canopy's middle from it */
    }
    for (part = 1; part <= 2; part++) {
        const Solid *s = tree_solid(part);
        const Paint *pp = &paints[forest.paint[part]];
        float size = forest.scale[part] * k, px, r, c[3], d[3];
        int sl, rl, out = 0;
        uint32_t *n;
        ForestInstance *fi;

        for (j = 0; j < 3; j++)
            d[j] = (float)(place[part][j] - forest_view.eye[j]);
        for (j = 0; j < 3; j++)
            c[j] = rot[3 * j] * d[0] + rot[3 * j + 1] * d[1] + rot[3 * j + 2] * d[2];
        r = size * part_radius(s, pp);      /* its sphere about its painting's middle */
        if (c[2] < forest_view.near - r)        /* wholly behind the eye */
            continue;
        for (j = 0; j < 4 && !out; j++)         /* wholly outside the view */
            out = forest_plane[j][0] * c[0] + forest_plane[j][1] * c[1] + forest_plane[j][2] * c[2] < -r;
        if (out)
            continue;
        px = c[2] > r ? size * focal / c[2] : 1e9f;
        if (paint_half(pp) * px < FOREST_LEAST_PX)
            continue;
        forest_detail(s, pp, px, &sl, &rl);
        n = &forest_n[part - 1][sl][rl];
        if (*n >= FOREST_TREES)
            continue;
        fi = &forest_inst[part - 1][sl][rl][(*n)++];
        fi->z = c[2];
        fi->in.x = (float)place[part][0];
        fi->in.y = (float)place[part][1];
        fi->in.z = (float)place[part][2];
        fi->in.size = size;
    }
}

void fx_solid_forest_end(void)
{
    static nshvc_mesh_vertex v[(RINGS_MAX - 1) * TREE_SIDES_MAX * 6];
    int slot = fx_draw_mesh_slot(), part, sl, rl;

    if (!forest_on || slot < 0 || slot >= FOREST_SLOTS)
        return;
    for (part = 2; part >= 1; part--) {         /* the canopies first: they hide the trunks */
        const Solid *s = tree_solid(part);
        Paint *pp = &paints[forest.paint[part]];

        if (!pp->ready)
            continue;
        for (sl = 0; sl < 4; sl++)
            for (rl = 0; rl < 4; rl++) {
                uint32_t count = forest_n[part - 1][sl][rl], nv, i;
                TreeLevel *L;

                if (!count || !(L = tree_level(s, pp, forest.paint[part], sl, rl)))
                    continue;
                if (!forest_mesh[slot][part - 1][sl][rl].mesh.id) {
                    forest_mesh[slot][part - 1][sl][rl].room = level_room(s, sl, rl);
                    forest_mesh[slot][part - 1][sl][rl].mesh = nshvc_mesh_create(forest_mesh[slot][part - 1][sl][rl].room);
                    if (!forest_mesh[slot][part - 1][sl][rl].mesh.id)
                        continue;
                }
                nv = mesh_faces(s, pp, L, forest_view.rot, forest.hi[part], v);
                if (nv > forest_mesh[slot][part - 1][sl][rl].room)
                    continue;
                nshvc_mesh_write(forest_mesh[slot][part - 1][sl][rl].mesh, v, nv);
                qsort(forest_inst[part - 1][sl][rl], count, sizeof forest_inst[0][0][0][0], instance_nearer);
                for (i = 0; i < count; i++)
                    forest_sorted[i] = forest_inst[part - 1][sl][rl][i].in;
                /* each part seen through about the player's car where it hides
                   it: its sphere as fx_solid_forest_add's, about its middle */
                fx_draw_mesh_seen(forest_mesh[slot][part - 1][sl][rl].mesh, &forest_view, forest_sorted, count, 0,
                                  part_radius(s, pp), FX_PIECE_KIND_FOREST + (uint32_t)part);
            }
    }
}

/* FxObjectSprite returned: the solid drawn in the sprite's place in the
   order */
void fx_solid_end(void)
{
    const Solid *s = armed;

    armed = NULL;
    if (!s || !coloured)
        return;
    coloured = 0;
    if (s->tree)
        tree(s);
    else
        draw(s);
}
