/*
 * orient.c: records tied to an object (the camera's among them): turning
 * one to face, or to match, its object, and setting the view from one. The
 * record (X) holds flags at +$00, the tied object's offset in the object
 * table at $7E:2000 at +$02, a position at +$08/+$0A/+$0C and three angles
 * at +$0E/+$10/+$12. The tied object's angles are the bytes at its
 * +$14/+$15/+$16.
 */
#include "machine.h"
#include "gsu_programs_calls.h"
#include "object_calls.h"
#include "carried.h"
#include "wram_names.h"


/* An absolute indexed address: the data bank, plus the offset and index
   (the sum may carry into the next bank). */
static uint32_t absidx(uint8_t bank, uint16_t a, uint16_t i)
{
    return (((uint32_t)bank << 16) + a + i) & 0xFFFFFF;
}

/* Arithmetic shift right by one (CMP #$8000, ROR). */
static uint16_t asr16(uint16_t v)
{
    cyc(38);
    return (uint16_t)((v & 0x8000) | v >> 1);
}

/* -(b * 256) for the angle byte b, read as a word and masked. */
static uint16_t neg_angle(uint32_t addr)
{
    cyc(30);
    uint16_t b = rd16(addr) & 0x00FF;

    return (uint16_t)(0 - (uint16_t)(b << 8));
}

/* angle field = old + (v - old) / 2. */
static void ease_half(uint8_t bank, uint16_t rec, uint16_t off, uint16_t v)
{
    uint16_t old = rd16(cyc_then(126, absidx(bank, off, rec))), a;

    a = asr16((uint16_t)(v - old));
    old = rd16(cyc_then(44, absidx(bank, off, rec)));
    wr16(absidx(bank, off, rec), cyc_then(30, (uint16_t)(a + old)));
}

/*
 * @routine 02:DE45 ObjectFaceTied
 * WHAT IT DOES: Turns record X toward the object it is tied to (+$02).
 * With flag bit 0: the turn and the pitch toward it come from the Super FX
 * (ObjectGsu81E3 with the record's position in direct page $04/$0A and
 * $05F2; ObjectGsu80BCThen81E3): +$10 = the first + $8000, +$0E = the
 * second (with flag bit 6, a quarter of the way there each call), and +$12
 * eases halfway toward a quarter of -(the object's +$16 * 256). Otherwise,
 * with flag bit 14, each angle eases halfway toward -(the object's angle
 * byte * 256). Then +$12 is cleared unless flag bit 15 is set.
 * HOW: The record's flags and +$12 at the end are read as direct page
 * offsets indexed by X (D = $0000); everything else through the data bank
 * with Y = the record and X = the object. P is saved and restored.
 * Entry: D = $0000; X = the record. Exit: A = the flags AND $8000; X
 * unchanged; Y = the record when either flag was set, else unchanged; P
 * unchanged.
 */
static int face_y_set;                  /* the adapter's: Y = the record */

uint16_t ObjectFaceTied(uint8_t bank, uint16_t rec)
{
    uint16_t a, old, x;

    face_y_set = 0;
    cyc(66);
    if (rd16(rec) & 0x0001) {
        face_y_set = 1;
        cyc(104);
        x = rd16((uint16_t)(rec + 0x02));
        wr16(0x000004, cyc_then(16, rd16(cyc_then(44, absidx(bank, 0x0008, rec)))));
        wr16(0x00000A, cyc_then(16, rd16(cyc_then(30, absidx(bank, 0x000A, rec)))));
        wr16(absidx(bank, 0x05F2, 0), cyc_then(24, rd16(cyc_then(30, absidx(bank, 0x000C, rec)))));
        a = (cyc(62), ObjectGsu81E3(bank, x));
        wr16(absidx(bank, 0x0010, rec), cyc_then(68, (uint16_t)(a + 0x8000)));
        if (!(rd16(cyc_then(30, absidx(bank, 0x0000, rec))) & 0x0040)) {
            cyc(102);
            a = ObjectGsu80BCThen81E3(bank, x);
        } else {
            cyc(108);
            a = ObjectGsu80BCThen81E3(bank, x);
            old = rd16(cyc_then(22, absidx(bank, 0x000E, rec)));
            a = asr16(asr16((uint16_t)(a - old)));
            a = (uint16_t)(a + rd16(cyc_then(44, absidx(bank, 0x000E, rec))));
        }
        wr16(absidx(bank, 0x000E, rec), cyc_then(30, a));
        a = asr16(asr16(neg_angle(absidx(bank, 0x2016, x))));
        ease_half(bank, rec, 0x0012, a);
        cyc(22);                                       /* BRA */
    } else { cyc(114); if (rd16(rec) & 0x4000) {
        face_y_set = 1;
        cyc(76);
        x = rd16((uint16_t)(rec + 0x02));
        cyc(14);
        ease_half(bank, rec, 0x000E, neg_angle(absidx(bank, 0x2014, x)));
        ease_half(bank, rec, 0x0010, neg_angle(absidx(bank, 0x2015, x)));
        ease_half(bank, rec, 0x0012, neg_angle(absidx(bank, 0x2016, x)));
    } else { cyc(32); } }
    cyc(36);
    a = rd16(rec) & 0x8000;
    if (!a)
        { cyc(62); wr16((uint16_t)(rec + 0x12), 0x0000); } else { cyc(46); }
    cyc(72);
    return a;
}

void ObjectFaceTied_cr(Carried *cr);
void ViewFromRecord_cr(Carried *cr);

void ObjectFaceTied_cr(Carried *cr)
{
    cr->a = ObjectFaceTied(cr->db, cr->x);
    if (face_y_set)
        cr->y = cr->x;
}

void abi_ObjectFaceTied(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectFaceTied_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}


uint16_t ScaleDp02Dp94Long(uint8_t *p);

static uint32_t dbw(uint8_t bank, uint16_t a)
{
    return (uint32_t)bank << 16 | a;
}

/* ScaleDp02Dp94Long, its result and the rescaled $02 and $94 stored at
   `out`, `out` + 2 and `out` + 4; returns the last value stored. */
static uint16_t area_origin(uint8_t bank, uint16_t out, uint8_t *p)
{
    cyc(62);
    uint16_t v = ScaleDp02Dp94Long(p);

    wr16(dbw(bank, out), cyc_then(112, v));
    cyc(16);
    v = rd16(0x000002);
    wr16(dbw(bank, (uint16_t)(out + 2)), cyc_then(24, v));
    cyc(16);
    v = rd16(0x000094);
    wr16(dbw(bank, (uint16_t)(out + 4)), cyc_then(24, v));
    return v;
}

/*
 * @routine 02:DAD6 ViewFromRecord
 * WHAT IT DOES: Sets the view from record X: its position to direct page
 * $C5/$C7/$C9, its angles to $0664/$0666/$0668, the tied object's +$22 to
 * Super FX RAM $70:247A; the view's area origin $1EF5-$1EF9 from the
 * position (ScaleDp02Dp94Long on X and Z), and the fixed area origin
 * $1EFB-$1EFF from the tied object's X and Z (+$0C, +$10), or a copy of
 * the view's when flag bit 13 is set (not reached in the traces so far).
 * HOW: The record's fields as direct page offsets indexed by X (D =
 * $0000); the object's in bank $7E; the outputs through the data bank.
 * Entry: D = $0000; X = the record. Exit: A = the last value stored; X
 * unchanged; M 16-bit, X 16-bit; N, Z from X (from A with bit 13); C, V
 * from the last ScaleDp02Dp94Long.
 */
uint16_t ViewFromRecord(uint8_t bank, uint16_t rec, uint8_t *p)
{
    uint16_t obj, v;

    *p &= (uint8_t)~(P_M | P_X);
    cyc(44);
    wr16(W(dp_c5), cyc_then(16, rd16((uint16_t)(rec + 0x08))));
    cyc(22);
    wr16(W(dp_c7), cyc_then(16, rd16((uint16_t)(rec + 0x0A))));
    cyc(22);
    wr16(W(dp_c9), cyc_then(16, rd16((uint16_t)(rec + 0x0C))));
    wr16(dbw(bank, 0x0664), cyc_then(24, rd16(cyc_then(22, (uint16_t)(rec + 0x0E)))));
    wr16(dbw(bank, 0x0666), cyc_then(24, rd16(cyc_then(22, (uint16_t)(rec + 0x10)))));
    wr16(dbw(bank, 0x0668), cyc_then(24, rd16(cyc_then(22, (uint16_t)(rec + 0x12)))));
    cyc(52);
    obj = rd16((uint16_t)(rec + 0x02));
    cyc(46);
    wr16(0x70247A, cyc_then(32, rd16(W(obj_22_long) + obj)));
    cyc(82);
    wr16(0x000002, cyc_then(16, rd16(W(dp_c5))));
    cyc(16);
    wr16(0x000094, cyc_then(16, rd16(W(dp_c9))));
    area_origin(bank, 0x1EF5, p);
    cyc(58);
    if (rd16(rec) & 0x2000) {
        v = rd16(cyc_then(64, dbw(bank, 0x1EF5)));
        wr16(dbw(bank, 0x1EFB), cyc_then(24, v));
        v = rd16(cyc_then(24, dbw(bank, 0x1EF7)));
        wr16(dbw(bank, 0x1EFD), cyc_then(24, v));
        v = rd16(cyc_then(24, dbw(bank, 0x1EF9)));
        wr16(dbw(bank, 0x1EFF), cyc_then(24, v));
        *p = nz16(*p, v);
        cyc(66);                        /* BRA, RTL */
        return v;
    }
    cyc(98);
    obj = rd16((uint16_t)(rec + 0x02));
    cyc(46);
    wr16(0x000002, cyc_then(16, rd16(W(obj_pos_x_long) + obj)));
    cyc(32);
    wr16(0x000094, cyc_then(16, rd16(W(obj_pos_z_long) + obj)));
    v = area_origin(bank, 0x1EFB, p);
    *p = nz16(*p, rec);
    cyc(80);
    return v;
}

void ViewFromRecord_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = ViewFromRecord(cr->db, cr->x, &p);
    set_p(cr, p);
}

void abi_ViewFromRecord(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ViewFromRecord_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

