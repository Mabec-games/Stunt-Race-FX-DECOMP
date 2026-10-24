/*
 * draw_list.c: the list of objects the Super FX draws this frame, built in
 * work RAM from the objects in the buckets the trigger list names.
 */
#include "machine.h"
#include "object_calls.h"
#include "carried.h"
#include "wram_names.h"

#define DRAW_LIST 0xF77Fu         /* the records, 30 bytes each, bank $7E */
#define OBJ(o, f) (0x7E0000 | W(obj_base) + (uint16_t)((f) + (o)))
#define REC(r, f) (W(wram_bank_7e) + (uint16_t)((f) + (r)))

/* Object `o`'s draw record at `r`: position (relative to the view at
   $C5/$C7/$C9 unless bit 7 of +$24 says absolute) and its depth key at
   +$02, model +$08, angles +$04-$07, link fields or 0, flags, palette and
   frame bytes and the animation word. */
static uint16_t draw_record(uint16_t o, uint16_t r)
{
    uint16_t a, t;
    uint8_t v;

    cyc(68);
    if (rd16(OBJ(o, 0x24)) & 0x0080) {
        cyc(70);
        wr16(REC(r, 0x12), cyc_then(30, rd16(OBJ(o, 0x0C))));
        cyc(30);
        wr16(REC(r, 0x10), cyc_then(30, rd16(OBJ(o, 0x0E))));
        cyc(30);
        wr16(REC(r, 0x14), cyc_then(30, rd16(OBJ(o, 0x10))));
        cyc(54);
        wr16(REC(r, 0x02), 0xF060);
        cyc(22);                        /* BRA */
    } else {
        cyc(76);
        a = rd16(OBJ(o, 0x0E));
        cyc(30);
        a = (uint16_t)(a - rd16(W(dp_c7)));
        cyc(30);
        wr16(REC(r, 0x10), a);
        if (a & 0x8000)
            { cyc(32); a = (uint16_t)(~a + 1); }
        cyc(66);
        wr16(REC(r, 0x02), (uint16_t)(a << 1));
        cyc(30);
        t = (rd16(OBJ(o, 0x09)) & 0x0001) ? (cyc(16), 0x1F40) : 0x0000;
        cyc(114);
        wr16(REC(r, 0x02), cyc_then(30, (uint16_t)(t + rd16(REC(r, 0x02)))));
        cyc(30);
        a = rd16(OBJ(o, 0x0C));
        cyc(30);
        wr16(REC(r, 0x12), cyc_then(30, (uint16_t)(a - rd16(W(dp_c5)))));
        cyc(30);
        a = rd16(OBJ(o, 0x10));
        cyc(30);
        wr16(REC(r, 0x14), cyc_then(30, (uint16_t)(a - rd16(W(dp_c9)))));
    }
    cyc(30);
    wr16(REC(r, 0x08), cyc_then(30, rd16(OBJ(o, 0x04))));
    cyc(30);
    wr16(REC(r, 0x04), cyc_then(30, rd16(OBJ(o, 0x14))));
    cyc(30);
    wr16(REC(r, 0x05), cyc_then(30, rd16(OBJ(o, 0x15))));
    cyc(30);
    wr16(REC(r, 0x06), cyc_then(30, rd16(OBJ(o, 0x16))));
    cyc(30);
    if (rd16(OBJ(o, 0x24)) & 0x0004) {
        cyc(106);
        wr16(REC(r, 0x0A), o);
        cyc(52);
        wr8(REC(r, 0x0C), cyc_then(30, rd8(OBJ(o, 0x48))));
        cyc(30);
        wr8(REC(r, 0x0D), cyc_then(30, rd8(OBJ(o, 0x49))));
        cyc(30);
        wr8(REC(r, 0x0E), cyc_then(30, rd8(OBJ(o, 0x4A))));
        cyc(22);                        /* BRA */
    } else {
        cyc(114);
        wr8(REC(r, 0x18), 0x00);
    }
    cyc(30);
    v = rd8(OBJ(o, 0x24));
    cyc(30);
    wr8(REC(r, 0x07), v);
    cyc(46);
    wr8(OBJ(o, 0x24), (uint8_t)(v & 0xFD));
    cyc(30);
    v = rd8(OBJ(o, 0x5A));
    if (!(v & 0x80))
        { cyc(40); v = rd8(0x7E0000 | W(wram_05E9)); } else { cyc(22); }
    cyc(46);
    wr8(REC(r, 0x19), (uint8_t)(v & 0x7F));
    cyc(24);
    if (rd8((0x7E0000 | W(draw_side)))) {
        cyc(46);
        v = rd8(OBJ(o, 0x19));
        if (!(v & 0x80))
            { cyc(40); v = rd8(0x7E0000 | W(wram_05E9)); }
        cyc(22);
    } else {
        cyc(52);
        v = rd8(OBJ(o, 0x18));
        if (!(v & 0x80))
            { cyc(40); v = rd8(0x7E0000 | W(wram_05E9)); } else { cyc(22); }
    }
    cyc(46);
    wr8(REC(r, 0x1A), (uint8_t)(v & 0x7F));
    cyc(30);
    wr8(REC(r, 0x1B), cyc_then(30, rd8(OBJ(o, 0x3A))));
    cyc(30);
    wr8(REC(r, 0x1C), cyc_then(30, rd8(OBJ(o, 0x5F))));
    cyc(30);
    wr8(REC(r, 0x1D), cyc_then(30, rd8(OBJ(o, 0x60))));
    cyc(52);
    wr16(REC(r, 0x16), cyc_then(30, rd16(OBJ(o, 0x5D))));
    cyc(90);
    wr16((0x7E0000 | W(draw_count)), cyc_then(14, (uint16_t)(rd16((0x7E0000 | W(draw_count))) + 1)));
    return (uint16_t)(r + 0x1E);
}

/* Whether object `o` is drawn: not when bit 5 of +$24 is set; with bit 6
   set, kind 2 (+$26) only on the near side ($1F07 = 0) and the rest only
   on the far side; then at most $31 records, and never model $A040. */
static int drawn(uint16_t o)
{
    cyc(30);
    uint8_t f = rd8(OBJ(o, 0x24));
    int16_t n;

    if (f & 0x20)
        return 0;
    if (f & 0x40) {
        cyc(94);
        if (rd8(OBJ(o, 0x26)) == 0x02) {
            cyc(62);
            if (rd8((0x7E0000 | W(draw_side))))
                return 0;
        } else { cyc(56); if (!rd8((0x7E0000 | W(draw_side)))) {
            return 0;
        } }
    } else { cyc(48); }
    cyc(68);
    n = (int16_t)(rd16((0x7E0000 | W(draw_count))) - 0x0031);
    if (n >= 0)
        { cyc(8); return 0; }
    cyc(76);
    return cyc_then(8, rd16(OBJ(o, 0x04)) != 0xA040);
}

/*
 * @routine 7E:EF50 ObjectDrawListBuild
 * WHAT IT DOES: Builds the list of objects to draw: for each trigger (two
 * words at $1AF5, 0 ends the list), walks the objects of the bucket its
 * second word names ($7F:0000 + the word AND $7FE, linked by +$3C) and,
 * for each one drawn (not hidden by bit 5 of +$24; with bit 6, kind 2 only
 * on the near side and the others only on the far side, $1F07; at most
 * $31; never model $A040), writes a 30-byte draw record from $7E:F77F:
 * position relative to the view and a depth key, model, angles, link and
 * frame fields, flags (clearing bit 1 of +$24) and palette bytes. $0CEE
 * counts the records; $034C holds the object being looked at.
 * HOW: Data bank $7E; the record address is kept in direct page $3E
 * between buckets, the trigger position in $1F2B. The test of bit 6 for
 * kind-2 objects is not reached in the traces so far.
 * Entry: D = $0000. Exit: A = 0; X = the trigger position; Y = the last
 * object looked at (0 after a bucket's end); P and the data bank unchanged.
 */
/* The objects of a bucket from o (o itself first unless `next`), each
   drawn writing its record from r; then $3E = the next record (and Y = 0,
   which the callers note). */
static void draw_bucket(uint16_t o, uint16_t r, int next)
{
    do {
        cyc(44);
        if (!next) {
            cyc(2);
            wr16((0x7E0000 | W(draw_object)), o);
            if (drawn(o))
                r = draw_record(o, r); else { cyc(62); }
        }
        next = 0;
        cyc(52);
        o = rd16(OBJ(o, 0x3C));
    } while (cyc_loop(60, o));
    cyc(52);
    wr16(W(draw_next), r);
    cyc(24);
}

/* The triggers from $1F2B on, each bucket's objects drawn; A = 0, X (e->x)
   = the trigger position at the end and Y (e->y) = 0 once a bucket has been
   walked. `end` is the time from the last list word read to the end: the
   exit's PLB, PLP and RTS, or, for the arm that reads the pushed data bank
   itself, up to that read. */
static void draw_triggers(unsigned end, ObjectDrawListEnd *e)
{
    uint16_t pos, a;

    for (;;) {
        cyc(24);
        pos = rd16((0x7E0000 | W(trigger_pos)));
        cyc(30);
        a = rd16((0x7E0000 | W(group_trigger_list)) + pos);
        e->x = pos;
        if (!a)
            { cyc(end); return; }
        cyc(46);
        wr16((0x7E0000 | W(trigger_word)), a);
        cyc(30);
        a = rd16((0x7E0000 | W(group_trigger_list)) + 2 + pos);
        pos = (uint16_t)(pos + 4);
        cyc(80);
        wr16((0x7E0000 | W(trigger_pos)), pos);
        cyc(70);
        a = rd16(W(object_buckets) + (a & 0x07FE));
        if (!a)
            { cyc(22); continue; }
        cyc(46);
        draw_bucket(a, rd16(W(draw_next)), 0);
        e->y = 0;
    }
}

ObjectDrawListEnd ObjectDrawListBuild(uint16_t y)
{
    ObjectDrawListEnd e = {0, y};

    cyc(178);
    wr16((0x7E0000 | W(draw_count)), 0x0000);
    cyc(24);
    wr16((0x7E0000 | W(trigger_pos)), 0x0000);
    cyc(40);
    wr16(W(draw_next), DRAW_LIST);
    draw_triggers(144, &e);
    return e;
}

/* ObjectDrawListBuild on carried registers. */
void ObjectDrawListBuild_cr(Carried *cr)
{
    uint8_t p = cr->p;
    ObjectDrawListEnd e = ObjectDrawListBuild(cr->y);

    cr->a = 0;
    cr->x = e.x;
    cr->y = e.y;
    set_p(cr, p);
}

void abi_ObjectDrawListBuild(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectDrawListBuild_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 7E:F0D3 ObjectDrawListClear08
 * WHAT IT DOES: An arm after ObjectDrawListBuild's RTS that nothing
 * reaches: clears bit 1 of object Y's +$08, then goes on with
 * ObjectDrawListBuild's walk from the object after it in its bucket (+$3C)
 * and the rest of the trigger list ($1F2B), X the next draw record. A
 * direct call is its only way in.
 * HOW: SEP #$20 for the AND #$FD, REP #$20, BRA into the bucket loop
 * (7E:F0C0); ends with ObjectDrawListBuild's PLB, PLP, RTS.
 * Entry: M 16-bit, X 16-bit, D = $0000, data bank $7E; Y = object; X = the
 * next draw record; the caller's P and data bank pushed above the return
 * (PHP, PHB). Exit: as ObjectDrawListBuild, P and the data bank those
 * pushed.
 */
ObjectDrawListEnd ObjectDrawListClear08(uint16_t y, uint16_t x)
{
    ObjectDrawListEnd e = {0, 0};

    cyc(52);
    wr8(OBJ(y, 0x08), cyc_then(46, (uint8_t)(rd8(OBJ(y, 0x08)) & 0xFD)));
    draw_bucket(y, x, 1);
    draw_triggers(66, &e);
    return e;
}

/* ObjectDrawListClear08 on carried registers. */
void ObjectDrawListClear08_cr(Carried *cr)
{
    uint8_t db, p;
    ObjectDrawListEnd e = ObjectDrawListClear08(cr->y, cr->x);

    cr->a = 0;
    cr->x = e.x;
    cr->y = e.y;
    db = rd8((uint16_t)(cpu.s + 1));
    cyc(20);
    p = rd8((uint16_t)(cpu.s + 2));
    cr->db = db;
    set_p(cr, p);
    if (p & P_X) {
        cyc(42);
        cr->x &= 0x00FF;
        cr->y &= 0x00FF;
    }
}

void abi_ObjectDrawListClear08(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectDrawListClear08_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}
