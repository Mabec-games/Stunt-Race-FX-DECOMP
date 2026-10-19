/*
 * draw_list.c: the list of objects the Super FX draws this frame, built in
 * work RAM from the objects in the buckets the trigger list names.
 */
#include "machine.h"

#define DRAW_COUNT 0x7E0CEEu      /* records written (at most $31) */
#define TRIGGER_POS 0x7E1F2Bu     /* offset into the trigger list */
#define TRIGGER_WORD 0x7E1F2Du    /* the current trigger's first word */
#define TRIGGER_LIST 0x7E1AF5u    /* two words per trigger, 0 ends it */
#define DRAW_NEXT 0x00003Eu       /* direct page: the next record's address */
#define DRAW_LIST 0xF77Fu         /* the records, 30 bytes each, bank $7E */
#define DRAW_SIDE 0x7E1F07u       /* nonzero: the view is on the far side */
#define OBJ(o, f) (0x7E2000u + (uint16_t)((f) + (o)))
#define REC(r, f) (0x7E0000u + (uint16_t)((f) + (r)))

/* Object `o`'s draw record at `r`: position (relative to the view at
   $C5/$C7/$C9 unless bit 7 of +$24 says absolute) and its depth key at
   +$02, model +$08, angles +$04-$07, link fields or 0, flags, palette and
   frame bytes and the animation word. */
static uint16_t draw_record(uint16_t o, uint16_t r)
{
    uint16_t a, t;
    uint8_t v;

    if (rd16(OBJ(o, 0x24)) & 0x0080) {
        wr16(REC(r, 0x12), rd16(OBJ(o, 0x0C)));
        wr16(REC(r, 0x10), rd16(OBJ(o, 0x0E)));
        wr16(REC(r, 0x14), rd16(OBJ(o, 0x10)));
        wr16(REC(r, 0x02), 0xF060);
    } else {
        a = rd16(OBJ(o, 0x0E));
        a = (uint16_t)(a - rd16(0x0000C7));
        wr16(REC(r, 0x10), a);
        if (a & 0x8000)
            a = (uint16_t)(~a + 1);
        wr16(REC(r, 0x02), (uint16_t)(a << 1));
        t = (rd16(OBJ(o, 0x09)) & 0x0001) ? 0x1F40 : 0x0000;
        wr16(REC(r, 0x02), (uint16_t)(t + rd16(REC(r, 0x02))));
        a = rd16(OBJ(o, 0x0C));
        wr16(REC(r, 0x12), (uint16_t)(a - rd16(0x0000C5)));
        a = rd16(OBJ(o, 0x10));
        wr16(REC(r, 0x14), (uint16_t)(a - rd16(0x0000C9)));
    }
    wr16(REC(r, 0x08), rd16(OBJ(o, 0x04)));
    wr16(REC(r, 0x04), rd16(OBJ(o, 0x14)));
    wr16(REC(r, 0x05), rd16(OBJ(o, 0x15)));
    wr16(REC(r, 0x06), rd16(OBJ(o, 0x16)));
    if (rd16(OBJ(o, 0x24)) & 0x0004) {
        wr16(REC(r, 0x0A), o);
        wr8(REC(r, 0x0C), rd8(OBJ(o, 0x48)));
        wr8(REC(r, 0x0D), rd8(OBJ(o, 0x49)));
        wr8(REC(r, 0x0E), rd8(OBJ(o, 0x4A)));
    } else {
        wr8(REC(r, 0x18), 0x00);
    }
    v = rd8(OBJ(o, 0x24));
    wr8(REC(r, 0x07), v);
    wr8(OBJ(o, 0x24), (uint8_t)(v & 0xFD));
    v = rd8(OBJ(o, 0x5A));
    if (!(v & 0x80))
        v = rd8(0x7E05E9);
    wr8(REC(r, 0x19), (uint8_t)(v & 0x7F));
    if (rd8(DRAW_SIDE)) {
        v = rd8(OBJ(o, 0x19));
        if (!(v & 0x80))
            v = rd8(0x7E05E9);
    } else {
        v = rd8(OBJ(o, 0x18));
        if (!(v & 0x80))
            v = rd8(0x7E05E9);
    }
    wr8(REC(r, 0x1A), (uint8_t)(v & 0x7F));
    wr8(REC(r, 0x1B), rd8(OBJ(o, 0x3A)));
    wr8(REC(r, 0x1C), rd8(OBJ(o, 0x5F)));
    wr8(REC(r, 0x1D), rd8(OBJ(o, 0x60)));
    wr16(REC(r, 0x16), rd16(OBJ(o, 0x5D)));
    wr16(DRAW_COUNT, (uint16_t)(rd16(DRAW_COUNT) + 1));
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
            if (rd8(DRAW_SIDE))
                return 0;
        } else { cyc(56); if (!rd8(DRAW_SIDE)) {
            return 0;
        } }
    } else { cyc(48); }
    cyc(68);
    n = (int16_t)(rd16(DRAW_COUNT) - 0x0031);
    if (n >= 0)
        { cyc(8); return 0; }
    cyc(76);
    return rd16(OBJ(o, 0x04)) != 0xA040;
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
   drawn writing its record from r; then $3E = the next record and Y = 0. */
static void draw_bucket(uint16_t o, uint16_t r, int next)
{
    do {
        if (!next) {
            wr16(0x7E034C, o);
            if (drawn(o))
                r = draw_record(o, r);
        }
        next = 0;
        o = rd16(OBJ(o, 0x3C));
    } while (o);
    cpu.y = 0;
    wr16(DRAW_NEXT, r);
}

/* The triggers from $1F2B on, each bucket's objects drawn; A = 0 and X =
   the trigger position at the end. */
static void draw_triggers(void)
{
    uint16_t pos, a;

    for (;;) {
        pos = rd16(TRIGGER_POS);
        a = rd16(TRIGGER_LIST + pos);
        cpu.x = pos;
        cpu.a = a;
        if (!a)
            return;
        wr16(TRIGGER_WORD, a);
        a = rd16(TRIGGER_LIST + 2 + pos);
        pos = (uint16_t)(pos + 4);
        wr16(TRIGGER_POS, pos);
        a = rd16(0x7F0000 + (a & 0x07FE));
        if (!a)
            continue;
        draw_bucket(a, rd16(DRAW_NEXT), 0);
    }
}

void ObjectDrawListBuild(void)
{
    wr16(DRAW_COUNT, 0x0000);
    wr16(TRIGGER_POS, 0x0000);
    wr16(DRAW_NEXT, DRAW_LIST);
    draw_triggers();
}

void abi_ObjectDrawListBuild(void)
{
    uint8_t p = cpu.p;

    ObjectDrawListBuild();
    cpu.p = p;
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
void abi_ObjectDrawListClear08(void)
{
    uint8_t db, p;

    cyc(52);
    wr8(OBJ(cpu.y, 0x08), cyc_then(46, (uint8_t)(rd8(OBJ(cpu.y, 0x08)) & 0xFD)));
    cyc(44);
    draw_bucket(cpu.y, cpu.x, 1);
    (cyc(2), draw_triggers());
    cyc(66);
    db = rd8((uint16_t)(cpu.s + 1));
    cyc(20);
    p = rd8((uint16_t)(cpu.s + 2));
    cpu.db = db;
    cpu.p = p;
    if (p & P_X) {
        cpu.x &= 0x00FF;
        cpu.y &= 0x00FF;
    }
}
