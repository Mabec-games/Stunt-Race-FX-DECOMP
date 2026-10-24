/*
 * lamps_08.c: course object set-ups of bank $08 that no course in the
 * scripts places (08:D61B-D83C): a burst of fifteen break-off pieces,
 * lamps and stacks of lamps lit by their own light (+$5D) as the lamps of
 * bank $09 are, and a falling object with a Super FX record. Each is
 * entered with X = the object; unit_lamps_08 calls them in a race. Also
 * bank $08 code nothing in the game calls (unit_dead_08_*): two pads that
 * reset a touching player's camera point (08:843C-8524), four spinning
 * set-ups and a player record match (08:D09E-D149), and a spray of eight
 * pieces with its handler (08:D8E4-DC12).
 *
 * Objects are at $2000 + X through the data bank (X 16-bit, M 8-bit).
 */
#include "machine.h"
#include "math_calls.h"
#include "cpu_ops.h"

#include "alu.h"
#include "carried.h"

int ObjectSpawn(uint8_t bank, uint16_t *a, uint16_t x, uint16_t *y);
uint16_t ObjectCopyPositionTo(uint8_t bank, uint16_t from, uint16_t to);
uint16_t ObjectCopyPositionFrom(uint8_t bank, uint16_t to, uint16_t from);
uint16_t ObjectApplyVelocity(uint8_t bank, uint16_t obj, uint8_t *p);
uint16_t ObjectRebucket(uint16_t obj);
uint16_t ObjectSyncFromRecord(uint8_t bank, uint16_t obj, uint16_t *y, uint8_t *p);
uint8_t GsuRecordNew424(uint8_t bank, uint16_t obj);
uint8_t GsuRecordNew42D(uint8_t bank, uint16_t obj);
uint16_t GsuRecordFlag10(uint8_t bank, uint16_t obj, uint16_t *a);
uint16_t GsuRecordPosition(uint8_t bank, uint16_t obj, uint16_t *a);
uint16_t GsuRecordSet2AHigh(uint8_t bank, uint16_t obj, uint8_t value, uint8_t *p);
uint8_t RandomStepLong(void);
int RecordNextContact(uint8_t bank, uint16_t *a, uint16_t *y, uint8_t *p, uint16_t x);

/* The registers these set-ups leave for their callers (they follow every
   instruction), carried through their steps. */
typedef struct {
    uint16_t a, x, y;
    uint8_t p;
} LRegs;

/* LDA (8-bit), LDA (16-bit) and LDA #, CLC/ADC #, SEC/SBC, CMP, TAX, TXA
   on the carried registers. */
static void lr_lda8(LRegs *r, uint32_t addr)
{
    uint8_t v = rd8(addr);

    r->a = (uint16_t)((r->a & 0xFF00) | v);
    r->p = nz8(r->p, v);
}

static void lr_lda(LRegs *r, uint32_t addr)
{
    r->a = rd16(addr);
    r->p = nz16(r->p, r->a);
}

static void lr_lda_imm(LRegs *r, uint16_t v)
{
    cyc(38);
    r->a = v;
    r->p = nz16(r->p, v);
}

static void lr_add(LRegs *r, uint16_t m)
{
    r->p &= (uint8_t)~P_C;
    r->a = alu_adc16(r->a, m, &r->p);
}

static void lr_sub(LRegs *r, uint16_t m)
{
    r->p |= P_C;
    r->a = alu_sbc16(r->a, m, &r->p);
}

static void lr_cmp(LRegs *r, uint16_t m)
{
    r->p = (uint8_t)((nz16(r->p, (uint16_t)(r->a - m)) & ~P_C) | (r->a >= m ? P_C : 0));
}

static void lr_tax(LRegs *r)
{
    cyc(30);
    r->x = r->a;
    r->p = nz16(r->p, r->x);
}

static void lr_txa(LRegs *r)
{
    cyc(30);
    r->a = r->x;
    r->p = nz16(r->p, r->a);
}

/* An approach of $3E toward A (approach.c), with the flags it leaves. */
static void lr_approach(LRegs *r, uint16_t (*f)(uint16_t, int *))
{
    int moved;

    r->a = f(r->a, &moved);
    r->p = ApproachExitP(r->p, r->a, 1, moved);
}

static void lr_approach_byte(LRegs *r, uint8_t (*f)(uint8_t, int *))
{
    int moved;
    uint8_t v = f((uint8_t)r->a, &moved);

    r->a = (uint16_t)((r->a & 0xFF00) | v);
    r->p = ApproachExitP(r->p, v, 0, moved);
}

void ObjectContactsCameraPoint(uint8_t bank, LRegs *r);

/* The calls, each with the registers its adapter leaves. */
static void l_ObjectSpawn(uint8_t bank, LRegs *r)
{
    if (ObjectSpawn(bank, &r->a, r->x, &r->y))
        { cyc(22); r->p = (uint8_t)(nz16(r->p, r->a) | P_C | P_M); }
    else
        r->p = (uint8_t)((nz16(r->p, r->x) & ~P_C) | P_M);
    cyc(58);
}

static void l_ObjectCopyPositionTo(uint8_t bank, LRegs *r)
{
    r->a = ObjectCopyPositionTo(bank, r->x, r->y);
    r->p = (uint8_t)(nz16(r->p, r->a) | P_M);
}

static void l_ObjectCopyPositionFrom(uint8_t bank, LRegs *r)
{
    r->a = ObjectCopyPositionFrom(bank, r->x, r->y);
    r->p = (uint8_t)(nz16(r->p, r->a) | P_M);
}

static void l_ObjectApplyVelocity(uint8_t bank, LRegs *r)
{
    r->a = ObjectApplyVelocity(bank, r->x, &r->p);
    r->p |= P_M;
}

static void l_ObjectRebucket(uint8_t bank, LRegs *r)
{
    (void)bank;
    r->a = ObjectRebucket(r->x);
}

static void l_ObjectSyncFromRecord(uint8_t bank, LRegs *r)
{
    r->a = ObjectSyncFromRecord(bank, r->x, &r->y, &r->p);
    r->p |= P_M;
}

/* A new record's exit: A = 0 (made) or $FF, Y = the object when made. */
static void record_new_exit(LRegs *r, uint8_t made)
{
    if (!made)
        r->y = r->x;
    cyc(60);
    r->a = made;
    r->p = (uint8_t)(nz8(r->p, made) | P_M);
}

static void l_GsuRecordNew424(uint8_t bank, LRegs *r)
{
    cyc(62);
    record_new_exit(r, GsuRecordNew424(bank, r->x));
}

static void l_GsuRecordNew42D(uint8_t bank, LRegs *r)
{
    cyc(62);
    record_new_exit(r, GsuRecordNew42D(bank, r->x));
    cyc(62);
}

/* GsuRecordFlag10 and GsuRecordPosition: no record, A = 0, Z set;
   otherwise A as left, N, Z from X. */
static void record_exit(LRegs *r, uint16_t rec, uint16_t a)
{
    if (!rec) {
        r->a = 0x0000;
        r->p = (uint8_t)((r->p & ~P_N) | P_Z | P_M);
        return;
    }
    cyc(102);
    r->a = a;
    r->p = (uint8_t)(nz16(r->p, r->x) | P_M);
}

static void l_GsuRecordFlag10(uint8_t bank, LRegs *r)
{
    uint16_t a = 0, rec = GsuRecordFlag10(bank, r->x, &a);

    record_exit(r, rec, a);
}

static void l_GsuRecordPosition(uint8_t bank, LRegs *r)
{
    uint16_t a = 0, rec = GsuRecordPosition(bank, r->x, &a);

    record_exit(r, rec, a);
}

static void l_GsuRecordSet2AHigh(uint8_t bank, LRegs *r)
{
    r->a = GsuRecordSet2AHigh(bank, r->x, (uint8_t)r->a, &r->p);
    r->p |= P_M;
}

static void l_RandomStepLong(uint8_t bank, LRegs *r)
{
    (void)bank;
    r->a = (uint16_t)((r->a & 0xFF00) | (cyc(46), RandomStepLong()));
}

static void l_RecordNextContact(uint8_t bank, LRegs *r)
{
    RecordNextContact(bank, &r->a, &r->y, &r->p, r->x);
    r->p |= P_M;
}

/* Object field `f` ($20xx) of object `o`, through the data bank. */
#define F(f, o) ((uint32_t)bank << 16 | (uint16_t)((f) + (o)))

/* LDA #v with M 8-bit. */
static void lda8(uint8_t bank, LRegs *r, uint8_t v)
{
    cyc(20);
    r->a = (uint16_t)((r->a & 0xFF00) | v);
    r->p = nz8(r->p, v);
}

/* LDA addr, 8-bit; returns the byte. */
static uint8_t ld8(uint8_t bank, LRegs *r, uint32_t addr)
{
    cyc(30);
    lr_lda8(r, addr);
    return (uint8_t)r->a;
}

/* LDA #v, STA addr, 8-bit. */
static void st8i(uint8_t bank, LRegs *r, uint32_t addr, uint8_t v)
{
    lda8(bank, r, v);
    cyc(26);
    wr8(addr, v);
}

/* REP #$20, LDA #v, STA addr, SEP #$20. */
static void st16i(uint8_t bank, LRegs *r, uint32_t addr, uint16_t v)
{
    r->p &= (uint8_t)~P_M;
    lr_lda_imm(r, v);
    cyc(32);
    wr16(addr, v);
    r->p |= P_M;
}

/* LDA addr, ORA #m, STA addr (8-bit). */
static void or8(uint8_t bank, LRegs *r, uint32_t addr, uint8_t m)
{
    uint8_t v = (uint8_t)(ld8(bank, r, addr) | m);

    lda8(bank, r, v);
    cyc(26);
    wr8(addr, v);
}

/* REP #$20, LDA addr, CLC, ADC #v, STA addr, SEP #$20. */
static void add16(uint8_t bank, LRegs *r, uint32_t addr, uint16_t v)
{
    r->p &= (uint8_t)~P_M;
    cyc(30);
    lr_lda(r, addr);
    lr_add(r, v);
    cyc(68);
    wr16(addr, r->a);
    r->p |= P_M;
}

/* The handler of object o: +$1A = h, +$1C = $08. */
static void handler(uint8_t bank, LRegs *r, uint16_t o, uint16_t h)
{
    st16i(bank, r, F(0x201A, o), cyc_then(6, h));
    st8i(bank, r, F(0x201C, o), cyc_then(22, 0x08));
}

/* The handler cleared: +$1A = 0, +$1C = 0. */
static void handler_none(uint8_t bank, LRegs *r, uint16_t o)
{
    st16i(bank, r, F(0x201A, o), cyc_then(6, 0x0000));
    st8i(bank, r, F(0x201C, o), cyc_then(22, 0x00));
    cyc(44);
}

/* A new object of `kind` ($1F2B, ObjectSpawn): 1 when one was made (C
   set), Y = it. */
static int spawn(uint8_t bank, LRegs *r, uint16_t kind)
{
    st16i(bank, r, ((uint32_t)bank << 16) | 0x1F2B, kind);
    (cyc(84), l_ObjectSpawn(bank, r));
    return (r->p & P_C) != 0;
}

/* PHX, TYX, PLY: X and Y swapped, N and Z from Y. */
static void swap_xy(uint8_t bank, LRegs *r, uint32_t clocks)
{
    cyc(clocks);
    uint16_t t = r->x;

    r->x = r->y;
    r->y = t;
    r->p = nz16(r->p, r->y);
}

/* +$24 |= 8, +$3A = 0, +$5F = t: an object lit by its own light ($5D). */
static void lamp(uint8_t bank, LRegs *r, uint16_t o, uint8_t t)
{
    or8(bank, r, F(0x2024, o), 0x08);
    st8i(bank, r, F(0x203A, o), 0x00);
    st8i(bank, r, F(0x205F, o), t);
}

/*
 * @routine 08:D61B ObjectBreakOffBurst15
 * WHAT IT DOES: Makes fifteen break-off pieces (kind $A1AC) at object X's
 * position, each with X as its parent (+$06) and the handler $08:D337
 * (ObjectBreakOffContact); a spawn that fails only skips that piece.
 * HOW: The count kept on the stack (PHA, PLA, 8-bit) across ObjectSpawn
 * and ObjectCopyPositionTo; A's low byte is pulled back between the
 * parent's store (TXA, 16-bit) and the handler's.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; A low byte = 0, Z
 * set.
 */
void ObjectBreakOffBurst15(uint8_t bank, LRegs *r)
{
    uint8_t n = 0x0F, lo;

    lda8(bank, r, n);
    do {
        cyc(18);
        if (spawn(bank, r, 0xA1AC)) {
            (cyc(84), l_ObjectCopyPositionTo(bank, r));
            lo = (uint8_t)r->a;
            r->p &= (uint8_t)~P_M;
            r->a = r->x;
            wr16(F(0x2006, r->y), cyc_then(88, r->x));
            r->p |= P_M;
            r->a = (uint16_t)((r->a & 0xFF00) | lo);
            cyc(50);
            handler(bank, r, r->y, 0xD337);
        } else { cyc(48); }
        cyc(68);
        n--;
        r->a = (uint16_t)((r->a & 0xFF00) | n);
        r->p = nz8(r->p, n);
    } while (n);
    cyc(34);
}

/* ObjectBreakOffBurst15 on carried registers. */
void ObjectBreakOffBurst15_cr(Carried *cr)
{
    LRegs r = {cr->a, cr->x, cr->y, cr->p};

    ObjectBreakOffBurst15(cr->db, &r);
    cr->a = r.a;
    cr->x = r.x;
    cr->y = r.y;
    set_p(cr, r.p);
}

void abi_ObjectBreakOffBurst15(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectBreakOffBurst15_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:D654 ObjectLampPair08
 * WHAT IT DOES: Sets up a lamp: object X $78 higher ($FF88 on +$0E), lit
 * by $9971 (+$24 bit 3, +$3A = 0, +$5F = $28); and, when one can be made,
 * a second light (kind $A078, $5D = $9973, +$5F = $50) at its position
 * $140 higher; then no handler.
 * HOW: ObjectSpawn, ObjectCopyPositionTo; the second light is not
 * rebucketed.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; A low byte = 0.
 */
void ObjectLampPair08(uint8_t bank, LRegs *r)
{
    add16(bank, r, F(0x200E, r->x), cyc_then(22, 0xFF88));
    cyc(22);
    lamp(bank, r, r->x, 0x28);
    st16i(bank, r, F(0x205D, r->x), cyc_then(6, 0x9971));
    cyc(22);
    if (spawn(bank, r, 0xA078)) {
        cyc(22);
        lamp(bank, r, r->y, 0x50);
        st16i(bank, r, F(0x205D, r->y), cyc_then(6, 0x9973));
        (cyc(84), l_ObjectCopyPositionTo(bank, r));
        add16(bank, r, F(0x200E, r->y), cyc_then(22, 0xFEC0));
    } else { cyc(26); }
    cyc(22);
    handler_none(bank, r, r->x);
}

/* ObjectLampPair08 on carried registers. */
void ObjectLampPair08_cr(Carried *cr)
{
    LRegs r = {cr->a, cr->x, cr->y, cr->p};

    ObjectLampPair08(cr->db, &r);
    cr->a = r.a;
    cr->x = r.x;
    cr->y = r.y;
    set_p(cr, r.p);
}

void abi_ObjectLampPair08(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectLampPair08_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* A light of the lamp stack (kind $A104, $5D = $9975, +$5F = $32) `up`
   on +$0E of object X's position, rebucketed; skipped without a free
   object. */
static void stack_light(uint8_t bank, LRegs *r, uint16_t up)
{
    if (!spawn(bank, r, 0xA104))
        { cyc(48); return; }
    cyc(22);
    lamp(bank, r, r->y, 0x32);
    st16i(bank, r, F(0x205D, r->y), cyc_then(6, 0x9975));
    (cyc(84), l_ObjectCopyPositionTo(bank, r));
    add16(bank, r, F(0x200E, r->y), cyc_then(22, up));
    swap_xy(bank, r, 164);
    l_ObjectRebucket(bank, r);
    swap_xy(bank, r, 80);
}

/*
 * @routine 08:D6D0 ObjectLampStack
 * WHAT IT DOES: Sets up a stack of three lights lit by $9975: object X
 * $46 higher ($FFBA on +$0E, +$5F = $1E) and two more (kind $A104, +$5F =
 * $32) at its position $2BC and $578 higher, each made only when an
 * object is free; then no handler.
 * HOW: ObjectSpawn, ObjectCopyPositionTo; each light rebucketed with X
 * and Y swapped round ObjectRebucket.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; A low byte = 0.
 */
void ObjectLampStack(uint8_t bank, LRegs *r)
{
    add16(bank, r, F(0x200E, r->x), cyc_then(22, 0xFFBA));
    st16i(bank, r, F(0x205D, r->x), cyc_then(28, 0x9975));
    cyc(22);
    lamp(bank, r, r->x, 0x1E);
    stack_light(bank, r, 0xFD44);
    stack_light(bank, r, 0xFA88);
    handler_none(bank, r, r->x);
}

/* ObjectLampStack on carried registers. */
void ObjectLampStack_cr(Carried *cr)
{
    LRegs r = {cr->a, cr->x, cr->y, cr->p};

    ObjectLampStack(cr->db, &r);
    cr->a = r.a;
    cr->x = r.x;
    cr->y = r.y;
    set_p(cr, r.p);
}

void abi_ObjectLampStack(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectLampStack_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* +$24 |= 8, +$3A = 0, +$5F = $14, then no handler ($08:D81B). */
static void lamp_still(uint8_t bank, LRegs *r)
{
    cyc(44);
    lamp(bank, r, r->x, 0x14);
    handler_none(bank, r, r->x);
}

/*
 * @routine 08:D7A2 ObjectLampLow08
 * WHAT IT DOES: Sets up a lamp lit by $9B8F: object X $1E higher ($FFE2
 * on +$0E), +$5F = $14; then no handler.
 * HOW: Word and byte stores; BRA to the shared tail at $08:D81B.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; A low byte = 0.
 */
void ObjectLampLow08(uint8_t bank, LRegs *r)
{
    st16i(bank, r, F(0x205D, r->x), cyc_then(6, 0x9B8F));
    add16(bank, r, F(0x200E, r->x), cyc_then(44, 0xFFE2));
    lamp_still(bank, r);
}

/* ObjectLampLow08 on carried registers. */
void ObjectLampLow08_cr(Carried *cr)
{
    LRegs r = {cr->a, cr->x, cr->y, cr->p};

    ObjectLampLow08(cr->db, &r);
    cr->a = r.a;
    cr->x = r.x;
    cr->y = r.y;
    set_p(cr, r.p);
}

void abi_ObjectLampLow08(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectLampLow08_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:D816 ObjectFallerStep
 * WHAT IT DOES: The falling object's handler: the object follows its Super
 * FX record (ObjectSyncFromRecord).
 * HOW: JSL ObjectSyncFromRecord.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectSyncFromRecord.
 */
void ObjectFallerStep(uint8_t bank, LRegs *r)
{
    (cyc(62), l_ObjectSyncFromRecord(bank, r));
    cyc(44);
}

/* ObjectFallerStep on carried registers. */
void ObjectFallerStep_cr(Carried *cr)
{
    LRegs r = {cr->a, cr->x, cr->y, cr->p};

    ObjectFallerStep(cr->db, &r);
    cr->a = r.a;
    cr->x = r.x;
    cr->y = r.y;
    set_p(cr, r.p);
}

void abi_ObjectFallerStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectFallerStep_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:D808 ObjectFallerRecord
 * WHAT IT DOES: Gives the falling object its Super FX record
 * (GsuRecordNew424) with flag $10 and $20 in the high byte of +$2A, then
 * its first step (ObjectFallerStep). Also its +$1D pointer.
 * HOW: GsuRecordNew424, GsuRecordFlag10, LDA #$20, GsuRecordSet2AHigh;
 * falls into ObjectFallerStep. A failed record is not checked.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectSyncFromRecord.
 */
void ObjectFallerRecord(uint8_t bank, LRegs *r)
{
    (cyc(62), l_GsuRecordNew424(bank, r));
    (cyc(62), l_GsuRecordFlag10(bank, r));
    lda8(bank, r, 0x20);
    (cyc(58), l_GsuRecordSet2AHigh(bank, r));
    (cyc(102), ObjectFallerStep(bank, r));
}

/* ObjectFallerRecord on carried registers. */
void ObjectFallerRecord_cr(Carried *cr)
{
    LRegs r = {cr->a, cr->x, cr->y, cr->p};

    ObjectFallerRecord(cr->db, &r);
    cr->a = r.a;
    cr->x = r.x;
    cr->y = r.y;
    set_p(cr, r.p);
}

void abi_ObjectFallerRecord(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectFallerRecord_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* The falling object's set-up ($08:D7C0): $12C higher ($FED4 on +$0E),
   +$1D = $08:D808 (ObjectFallerRecord), lit (+$5F = $14), falling at
   -$64 (+$30), handler $08:D816 (ObjectFallerStep); then its record. */
static void faller(uint8_t bank, LRegs *r)
{
    add16(bank, r, F(0x200E, r->x), cyc_then(44, 0xFED4));
    st16i(bank, r, F(0x201D, r->x), cyc_then(28, 0xD808));
    st8i(bank, r, F(0x201F, r->x), cyc_then(22, 0x08));
    lamp(bank, r, r->x, 0x14);
    st16i(bank, r, F(0x2030, r->x), cyc_then(6, 0xFF9C));
    cyc(22);
    handler(bank, r, r->x, 0xD816);
    ObjectFallerRecord(bank, r);
}

/*
 * @routine 08:D7BC ObjectFaller
 * WHAT IT DOES: Sets up a falling object: $12C higher, +$1D = $08:D808,
 * lit by its own light (+$24 bit 3, +$3A = 0, +$5F = $14), Y speed -$64,
 * handler ObjectFallerStep; then ObjectFallerRecord.
 * HOW: BRA to the set-up at $08:D7C0.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectSyncFromRecord.
 */
void ObjectFaller(uint8_t bank, LRegs *r)
{
    faller(bank, r);
}

/* ObjectFaller on carried registers. */
void ObjectFaller_cr(Carried *cr)
{
    LRegs r = {cr->a, cr->x, cr->y, cr->p};

    ObjectFaller(cr->db, &r);
    cr->a = r.a;
    cr->x = r.x;
    cr->y = r.y;
    set_p(cr, r.p);
}

void abi_ObjectFaller(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectFaller_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:D7BE ObjectFallerB
 * WHAT IT DOES: As ObjectFaller (a second entry for another kind).
 * HOW: BRA to the set-up at $08:D7C0.
 * Entry: as ObjectFaller. Exit: as ObjectFaller.
 */
void ObjectFallerB(uint8_t bank, LRegs *r)
{
    faller(bank, r);
}

/* ObjectFallerB on carried registers. */
void ObjectFallerB_cr(Carried *cr)
{
    LRegs r = {cr->a, cr->x, cr->y, cr->p};

    ObjectFallerB(cr->db, &r);
    cr->a = r.a;
    cr->x = r.x;
    cr->y = r.y;
    set_p(cr, r.p);
}

void abi_ObjectFallerB(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectFallerB_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* The eight pieces of ObjectSpray8: kind, +$2E, the rise +$29 the
   vertical speed eases to, and the starting turn +$15. */
static const struct { uint16_t kind, vx, rise; uint8_t turn; } spray8[8] = {
    {0xA660, 0xFE3E, 0xFC7C, 0x00},
    {0xA67C, 0xFF6A, 0xFC7C, 0xFC},
    {0xA698, 0x0096, 0xFC7C, 0xF8},
    {0xA6B4, 0x01C2, 0xFC7C, 0xF4},
    {0xA6D0, 0xFE3E, 0xFE70, 0xF0},
    {0xA6EC, 0xFF6A, 0xFE70, 0xEC},
    {0xA6B4, 0x0096, 0xFE70, 0xE8},
    {0xA708, 0x01C2, 0xFE70, 0xE4},
};

/*
 * @routine 08:D8E4 ObjectSpray8
 * WHAT IT DOES: Sprays eight pieces from object Y: each a new object of
 * its kind (the table above) with the handler $08:DB7A
 * (ObjectSprayPieceStart), Y as its parent (+$27), the speeds +$2E (four
 * sideways values, twice), +$30 = $03E8 and +$32 = $1770, the rise +$29
 * ($FC7C for the first four, $FE70 for the rest) and the turn +$15 from 0
 * down by 4. A spawn that fails skips only that piece. Nothing in the
 * game calls it; a direct call is its only way in.
 * HOW: Eight copies of one block, each jumping to the next when
 * ObjectSpawn fails; X and Y saved on the stack (PHX, PHY) and X = Y for
 * the parent; A's low byte kept on the stack (PHA, PLA) across the
 * parent's store (TXA, 16-bit).
 * Entry: M 8-bit, X 16-bit; Y = the parent; D = $0000. Exit: X and Y
 * unchanged, N and Z from X; M 8-bit; A as the last piece left it.
 */
void ObjectSpray8(uint8_t bank, LRegs *r)
{
    uint16_t x0 = r->x, y0 = r->y;
    unsigned i;
    uint8_t lo;

    r->x = r->y;
    r->p = nz16(r->p, r->x);
    cyc(74);
    for (i = 0; i < 8; i++) {
        if (!spawn(bank, r, spray8[i].kind))
            { cyc(48); continue; }
        cyc(22);
        handler(bank, r, r->y, 0xDB7A);
        r->p &= (uint8_t)~P_M;
        lr_lda_imm(r, spray8[i].vx);
        wr16(F(0x202E, r->y), cyc_then(38, spray8[i].vx));
        lr_lda_imm(r, 0x03E8);
        wr16(F(0x2030, r->y), cyc_then(16, 0x03E8));
        lr_lda_imm(r, 0x1770);
        wr16(F(0x2032, r->y), cyc_then(16, 0x1770));
        r->p |= P_M;
        st16i(bank, r, F(0x2029, r->y), cyc_then(28, spray8[i].rise));
        lo = (uint8_t)r->a;
        r->p &= (uint8_t)~P_M;
        lr_txa(r);
        wr16(F(0x2027, r->y), cyc_then(80, r->x));
        r->p |= P_M;
        r->a = (uint16_t)((r->a & 0xFF00) | lo);
        r->p = nz8(r->p, lo);
        st8i(bank, r, F(0x2015, r->y), cyc_then(50, spray8[i].turn));
    }
    cyc(114);
    r->y = y0;
    r->x = x0;
    r->p = nz16(r->p, r->x);
}

/* ObjectSpray8 on carried registers. */
void ObjectSpray8_cr(Carried *cr)
{
    LRegs r = {cr->a, cr->x, cr->y, cr->p};

    ObjectSpray8(cr->db, &r);
    cr->a = r.a;
    cr->x = r.x;
    cr->y = r.y;
    set_p(cr, r.p);
}

void abi_ObjectSpray8(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSpray8_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* ObjectSprayPieceStep's work on object X. */
static void spray_piece_step(uint8_t bank, LRegs *r)
{
    uint8_t v;

    lr_lda8(r, cyc_then(30, F(0x2014, r->x)));
    cyc(16);
    wr8(0x00003E, (uint8_t)r->a);
    cyc(58);
    lda8(bank, r, 0x00);
    lr_approach_byte(r, ApproachSixteenthMin16Byte);
    wr8(F(0x2014, r->x), cyc_then(30, (uint8_t)r->a));
    lr_lda8(r, cyc_then(30, F(0x2015, r->x)));
    r->p &= (uint8_t)~P_C;
    v = alu_adc8((uint8_t)r->a, 0x08, &r->p);
    r->a = (uint16_t)((r->a & 0xFF00) | v);
    wr8(F(0x2015, r->x), cyc_then(60, v));
    r->y = rd16(cyc_then(30, F(0x2027, r->x)));
    r->p = nz16(r->p, r->y);
    (cyc(62), l_ObjectCopyPositionFrom(bank, r));
    (cyc(62), l_ObjectApplyVelocity(bank, r));
    r->p &= (uint8_t)~P_M;
    lr_lda(r, cyc_then(52, F(0x2030, r->x)));
    cyc(16);
    wr16(0x00003E, r->a);
    lr_lda(r, cyc_then(30, F(0x2029, r->x)));
    (cyc(14), lr_approach(r, ApproachEighthMin8));
    wr16(F(0x2030, r->x), cyc_then(30, r->a));
    r->p |= P_M;
    r->p &= (uint8_t)~P_M;
    lr_lda(r, cyc_then(74, F(0x2032, r->x)));
    cyc(16);
    wr16(0x00003E, r->a);
    lr_lda_imm(r, 0x05DC);
    lr_approach(r, ApproachEighthMin8);
    wr16(F(0x2032, r->x), cyc_then(30, r->a));
    r->p |= P_M;
    cyc(66);
}

/*
 * @routine 08:DB7A ObjectSprayPieceStart
 * WHAT IT DOES: The first tick of a piece of ObjectSpray8: its handler
 * becomes ObjectSprayPieceStep ($08:DB90) and +$14 a random byte
 * (RandomStepLong); then the step itself.
 * HOW: Runs on into ObjectSprayPieceStep.
 * Entry: M 8-bit, X 16-bit; X = the piece; D = $0000. Exit: as
 * ObjectSprayPieceStep.
 */
void ObjectSprayPieceStart(uint8_t bank, LRegs *r)
{
    handler(bank, r, r->x, 0xDB90);
    (cyc(62), l_RandomStepLong(bank, r));
    wr8(F(0x2014, r->x), cyc_then(30, (uint8_t)r->a));
    spray_piece_step(bank, r);
}

/* ObjectSprayPieceStart on carried registers. */
void ObjectSprayPieceStart_cr(Carried *cr)
{
    LRegs r = {cr->a, cr->x, cr->y, cr->p};

    ObjectSprayPieceStart(cr->db, &r);
    cr->a = r.a;
    cr->x = r.x;
    cr->y = r.y;
    set_p(cr, r.p);
}

void abi_ObjectSprayPieceStart(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSprayPieceStart_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:DB90 ObjectSprayPieceStep
 * WHAT IT DOES: A tick of a piece of ObjectSpray8: +$14 eases a sixteenth
 * of the way to 0 (at least 1), the turn +$15 grows by 8, the piece takes
 * its parent's (+$27) position (ObjectCopyPositionFrom) and moves by its
 * speeds (ObjectApplyVelocity); its speed +$30 eases an eighth of the way
 * (at least 8) to its rise +$29, and +$32 to $05DC.
 * HOW: ApproachSixteenthMin16Byte and ApproachEighthMin8 with the start
 * in direct page $3E.
 * Entry: M 8-bit, X 16-bit; X = the piece; D = $0000. Exit: M 8-bit; A =
 * +$32; Y = the parent.
 */
void ObjectSprayPieceStep(uint8_t bank, LRegs *r)
{
    spray_piece_step(bank, r);
}

/* ObjectSprayPieceStep on carried registers. */
void ObjectSprayPieceStep_cr(Carried *cr)
{
    LRegs r = {cr->a, cr->x, cr->y, cr->p};

    ObjectSprayPieceStep(cr->db, &r);
    cr->a = r.a;
    cr->x = r.x;
    cr->y = r.y;
    set_p(cr, r.p);
}

void abi_ObjectSprayPieceStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSprayPieceStep_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:DBD9 ObjectPlayerPassCount
 * WHAT IT DOES: Counts the player ($0346) past object X once: unless the
 * object's +$25 bit 0 is set already, when the player is less than $1F4
 * from it along +$10 the player's +$4C goes up by one and the object's
 * +$25 bit 0 is set. Nothing in the game calls it; a direct call is its
 * only way in.
 * HOW: The distance as |player - object| (EOR #$FFFF, INC), compared
 * signed (BPL); a CMP #1 and both its branches to the same place.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; Y = the player
 * unless the bit was set; A and the flags as the last step left them.
 */
void ObjectPlayerPassCount(uint8_t bank, LRegs *r)
{
    uint8_t v;
    uint16_t d;

    lr_lda8(r, cyc_then(30, F(0x2025, r->x)));
    v = (uint8_t)(r->a & 0x01);
    r->a = (uint16_t)((r->a & 0xFF00) | v);
    r->p = nz8(r->p, v);
    if (v)
        { cyc(108); return; }
    r->y = rd16(cyc_then(62, ((uint32_t)bank << 16) | 0x0346));
    r->p = nz16(r->p, r->y);
    r->p &= (uint8_t)~P_M;
    lr_lda(r, cyc_then(52, F(0x2010, r->y)));
    lr_sub(r, rd16(cyc_then(44, F(0x2010, r->x))));
    if (r->a & 0x8000) {
        cyc(32);
        d = (uint16_t)(r->a ^ 0xFFFF);
        r->a = (uint16_t)(d + 1);
        r->p = nz16(r->p, r->a);
    }
    cyc(84);
    lr_cmp(r, 0x01F4);
    r->p |= P_M;
    if (!(r->p & P_N))
        { cyc(44); return; }
    lr_lda8(r, cyc_then(24, F(0x204C, r->y)));
    v = (uint8_t)(r->a + 1);
    r->a = (uint16_t)((r->a & 0xFF00) | v);
    r->p = nz8(r->p, v);
    wr8(F(0x204C, r->y), cyc_then(44, v));
    r->p = (uint8_t)((nz8(r->p, (uint8_t)(v - 0x01)) & ~P_C) | (v >= 0x01 ? P_C : 0));
    lr_lda8(r, cyc_then(v == 0x01 ? 68 : 84, F(0x2025, r->x)));
    v = (uint8_t)(r->a | 0x01);
    r->a = (uint16_t)((r->a & 0xFF00) | v);
    r->p = nz8(r->p, v);
    wr8(F(0x2025, r->x), cyc_then(46, v));
    cyc(44);
}

/* ObjectPlayerPassCount on carried registers. */
void ObjectPlayerPassCount_cr(Carried *cr)
{
    LRegs r = {cr->a, cr->x, cr->y, cr->p};

    ObjectPlayerPassCount(cr->db, &r);
    cr->a = r.a;
    cr->x = r.x;
    cr->y = r.y;
    set_p(cr, r.p);
}

void abi_ObjectPlayerPassCount(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectPlayerPassCount_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* LDA #v, STA d (direct page, D = $0000), 16-bit, between REP and SEP. */
static void dp16i(uint8_t bank, LRegs *r, uint16_t d, uint16_t v)
{
    r->p &= (uint8_t)~P_M;
    lr_lda_imm(r, v);
    cyc(24);
    wr16(d, v);
    r->p |= P_M;
}

/*
 * @routine 08:84B7 ObjectCameraPadRecord
 * WHAT IT DOES: The Super FX record of a camera pad (ObjectCameraPad,
 * ObjectCameraPadB): a new record (GsuRecordNew42D) with bit 4 of its
 * flags (GsuRecordFlag10), the three sizes in direct page $02, $08 and
 * $94 all $0780, and its position (GsuRecordPosition). It is the record
 * handler ObjectCameraPadSetup stores at +$1D.
 * HOW: Three 16-bit stores of $0780 between the calls.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as
 * GsuRecordPosition.
 */
void ObjectCameraPadRecord(uint8_t bank, LRegs *r)
{
    (cyc(62), l_GsuRecordNew42D(bank, r));
    l_GsuRecordFlag10(bank, r);
    dp16i(bank, r, 0x000002, 0x0780);
    cyc(22);
    dp16i(bank, r, 0x000008, 0x0780);
    cyc(22);
    dp16i(bank, r, 0x000094, 0x0780);
    (cyc(84), l_GsuRecordPosition(bank, r));
    cyc(44);
}

/* ObjectCameraPadRecord on carried registers. */
void ObjectCameraPadRecord_cr(Carried *cr)
{
    LRegs r = {cr->a, cr->x, cr->y, cr->p};

    ObjectCameraPadRecord(cr->db, &r);
    cr->a = r.a;
    cr->x = r.x;
    cr->y = r.y;
    set_p(cr, r.p);
}

void abi_ObjectCameraPadRecord(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectCameraPadRecord_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:84A8 ObjectCameraPadSetup
 * WHAT IT DOES: The set-up both camera pads jump to: the record handler
 * (+$1D, +$1F) becomes $08:84B7 (ObjectCameraPadRecord), which then runs.
 * HOW: Runs on into ObjectCameraPadRecord.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as
 * ObjectCameraPadRecord.
 */
void ObjectCameraPadSetup(uint8_t bank, LRegs *r)
{
    st16i(bank, r, F(0x201D, r->x), cyc_then(6, 0x84B7));
    st8i(bank, r, F(0x201F, r->x), cyc_then(22, 0x08));
    ObjectCameraPadRecord(bank, r);
}

/* ObjectCameraPadSetup on carried registers. */
void ObjectCameraPadSetup_cr(Carried *cr)
{
    LRegs r = {cr->a, cr->x, cr->y, cr->p};

    ObjectCameraPadSetup(cr->db, &r);
    cr->a = r.a;
    cr->x = r.x;
    cr->y = r.y;
    set_p(cr, r.p);
}

void abi_ObjectCameraPadSetup(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectCameraPadSetup_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:843C ObjectCameraPad
 * WHAT IT DOES: Sets up a pad that resets the camera of a player touching
 * it: its handler becomes $08:844F (ObjectCameraPadStep), then the shared
 * set-up (ObjectCameraPadSetup). Nothing in the game calls it; a direct
 * call is its only way in.
 * HOW: A JML to the set-up.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as
 * ObjectCameraPadSetup.
 */
void ObjectCameraPad(uint8_t bank, LRegs *r)
{
    handler(bank, r, r->x, 0x844F);
    (cyc(32), ObjectCameraPadSetup(bank, r));
}

/* ObjectCameraPad on carried registers. */
void ObjectCameraPad_cr(Carried *cr)
{
    LRegs r = {cr->a, cr->x, cr->y, cr->p};

    ObjectCameraPad(cr->db, &r);
    cr->a = r.a;
    cr->x = r.x;
    cr->y = r.y;
    set_p(cr, r.p);
}

void abi_ObjectCameraPad(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectCameraPad_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:8472 ObjectCameraPadB
 * WHAT IT DOES: ObjectCameraPad with its own copy of the handler,
 * $08:8485 (ObjectCameraPadStepB). Nothing in the game calls it.
 * HOW: As ObjectCameraPad.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as
 * ObjectCameraPadSetup.
 */
void ObjectCameraPadB(uint8_t bank, LRegs *r)
{
    handler(bank, r, r->x, 0x8485);
    (cyc(32), ObjectCameraPadSetup(bank, r));
}

/* ObjectCameraPadB on carried registers. */
void ObjectCameraPadB_cr(Carried *cr)
{
    LRegs r = {cr->a, cr->x, cr->y, cr->p};

    ObjectCameraPadB(cr->db, &r);
    cr->a = r.a;
    cr->x = r.x;
    cr->y = r.y;
    set_p(cr, r.p);
}

void abi_ObjectCameraPadB(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectCameraPadB_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* The camera pads' handler: the offsets $1F4C, $1F4E, $1F50 all 0, then
   ObjectContactsCameraPoint. */
static void camera_pad_step(uint8_t bank, LRegs *r)
{
    st16i(bank, r, ((uint32_t)bank << 16) | 0x1F4C, 0x0000);
    st16i(bank, r, (cyc(22), ((uint32_t)bank << 16)) | 0x1F4E, 0x0000);
    st16i(bank, r, (cyc(22), ((uint32_t)bank << 16)) | 0x1F50, 0x0000);
    (cyc(146), ObjectContactsCameraPoint(bank, r));
}

/*
 * @routine 08:844F ObjectCameraPadStep
 * WHAT IT DOES: A tick of ObjectCameraPad: $1F4C, $1F4E and $1F50 set to
 * 0 and given to the camera point of every player touching it
 * (ObjectContactsCameraPoint).
 * HOW: Three 16-bit stores of 0, then the call.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as
 * ObjectContactsCameraPoint.
 */
void ObjectCameraPadStep(uint8_t bank, LRegs *r)
{
    camera_pad_step(bank, r);
}

/* ObjectCameraPadStep on carried registers. */
void ObjectCameraPadStep_cr(Carried *cr)
{
    LRegs r = {cr->a, cr->x, cr->y, cr->p};

    ObjectCameraPadStep(cr->db, &r);
    cr->a = r.a;
    cr->x = r.x;
    cr->y = r.y;
    set_p(cr, r.p);
}

void abi_ObjectCameraPadStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectCameraPadStep_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:8485 ObjectCameraPadStepB
 * WHAT IT DOES: ObjectCameraPadB's copy of ObjectCameraPadStep.
 * HOW: As ObjectCameraPadStep.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as
 * ObjectContactsCameraPoint.
 */
void ObjectCameraPadStepB(uint8_t bank, LRegs *r)
{
    camera_pad_step(bank, r);
}

/* ObjectCameraPadStepB on carried registers. */
void ObjectCameraPadStepB_cr(Carried *cr)
{
    LRegs r = {cr->a, cr->x, cr->y, cr->p};

    ObjectCameraPadStepB(cr->db, &r);
    cr->a = r.a;
    cr->x = r.x;
    cr->y = r.y;
    set_p(cr, r.p);
}

void abi_ObjectCameraPadStepB(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectCameraPadStepB_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:84DF ObjectContactsCameraPoint
 * WHAT IT DOES: Takes object X's position back from its record
 * (ObjectSyncFromRecord) and, when the record has contacts, walks them
 * (RecordNextContact): for each with bit 2 of its flags, the player its
 * bit 0 or bit 1 names ($0346 or $034A) gets $1F4C, $1F4E and $1F50 as
 * +$2C, +$2E and +$30 of its camera point (the link, +$4F, of its link),
 * when it has a link.
 * HOW: X kept on the stack (PHX, PLX) while it is the player and then
 * its link; BIT #imm tests of the flags in A; the stores 16-bit through
 * the data bank with Y.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: M 8-bit; X = the
 * object; carry clear; A and the rest as the last call left them.
 */
void ObjectContactsCameraPoint(uint8_t bank, LRegs *r)
{
    uint16_t x0;
    uint8_t f;
    static const uint16_t to[3] = {0x002C, 0x002E, 0x0030}, from[3] = {0x1F4C, 0x1F4E, 0x1F50};
    unsigned i;

    l_ObjectSyncFromRecord(bank, r);
    f = (uint8_t)(r->a & 0x01);
    r->a = (uint16_t)((r->a & 0xFF00) | f);
    r->p = nz8(r->p, f);
    if (!f)
        { cyc(126); return; } else { cyc(32); }
    for (;;) {
        cyc(62);
        l_RecordNextContact(bank, r);
        if (!(r->p & P_C))
            { cyc(116); return; } else { cyc(48); }
        f = (uint8_t)r->a;
        r->p = (uint8_t)((r->p & ~P_Z) | ((f & 0x04) ? 0 : P_Z));
        if (!(f & 0x04))
            { cyc(6); continue; }
        x0 = r->x;
        cyc(30);
        r->p = (uint8_t)((r->p & ~P_Z) | ((f & 0x01) ? 0 : P_Z));
        if (f & 0x01) {
            /* BIT, BEQ not taken, LDX; BRA */
            r->x = rd16(cyc_then(56, ((uint32_t)bank << 16) | 0x0346));
            cyc(22);
        } else {
            r->p = (uint8_t)((r->p & ~P_Z) | ((f & 0x02) ? 0 : (cyc(76), P_Z)));
            if (!(f & 0x02))
                goto next;
            r->x = rd16(cyc_then(94, ((uint32_t)bank << 16) | 0x034A));
        }
        r->p = nz16(r->p, r->x);
        cyc(30);
        r->y = rd16(F(0x204F, r->x));
        r->p = nz16(r->p, r->y);
        if (!r->y)
            { cyc(22); goto next; }
        r->x = r->y;
        r->y = rd16(cyc_then(60, F(0x204F, r->x)));
        r->p = nz16(r->p, r->y);
        r->p &= (uint8_t)~P_M;
        cyc(22);
        for (i = 0; i < 3; i++) {
            lr_lda(r, cyc_then(24, ((uint32_t)bank << 16) | from[i]));
            wr16(((uint32_t)bank << 16) | (uint16_t)(to[i] + r->y), cyc_then(30, r->a));
        }
        r->p |= P_M;
        cyc(22);
next:
        cyc(58);
        r->x = x0;
        r->p = nz16(r->p, r->x);
    }
}

/* ObjectContactsCameraPoint on carried registers. */
void ObjectContactsCameraPoint_cr(Carried *cr)
{
    LRegs r = {cr->a, cr->x, cr->y, cr->p};

    ObjectContactsCameraPoint(cr->db, &r);
    cr->a = r.a;
    cr->x = r.x;
    cr->y = r.y;
    set_p(cr, r.p);
}

void abi_ObjectContactsCameraPoint(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectContactsCameraPoint_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* ObjectSpin8Step's work: the turn +$15 grows by 8. */
static void spin_step(uint8_t bank, LRegs *r)
{
    uint8_t v;

    lr_lda8(r, cyc_then(30, F(0x2015, r->x)));
    r->p &= (uint8_t)~P_C;
    v = alu_adc8((uint8_t)r->a, 0x08, &r->p);
    r->a = (uint16_t)((r->a & 0xFF00) | v);
    wr8(F(0x2015, r->x), cyc_then(60, v));
    cyc(44);
}

/* A spinning set-up of kind k: +$18 = k | $80 and (unless k is 3) the
   turn +$15 = turn; then the handler $08:D0DE and its first tick. */
static void spin(uint8_t bank, LRegs *r, uint8_t k, int has_turn, uint8_t turn)
{
    lda8(bank, r, k);
    lda8(bank, r, (uint8_t)(k | 0x80));
    wr8(F(0x2018, r->x), cyc_then(22, (uint8_t)(k | 0x80)));
    if (has_turn) {
        st8i(bank, r, F(0x2015, r->x), turn);
        cyc(22);
    }
    handler(bank, r, r->x, 0xD0DE);
    spin_step(bank, r);
}

/*
 * @routine 08:D09E ObjectSpin8Kind0
 * WHAT IT DOES: Sets up a spinning object of kind 0: +$18 = $80, the turn
 * +$15 = $30, the handler $08:D0DE (ObjectSpin8Step) and its first tick.
 * Nothing in the game calls it; a direct call is its only way in.
 * HOW: LDA #0, ORA #$80; a BRA to the shared handler set-up at 08:D0CF.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectSpin8Step.
 */
void ObjectSpin8Kind0(uint8_t bank, LRegs *r)
{
    spin(bank, r, 0x00, 1, 0x30);
}

/* ObjectSpin8Kind0 on carried registers. */
void ObjectSpin8Kind0_cr(Carried *cr)
{
    LRegs r = {cr->a, cr->x, cr->y, cr->p};

    ObjectSpin8Kind0(cr->db, &r);
    cr->a = r.a;
    cr->x = r.x;
    cr->y = r.y;
    set_p(cr, r.p);
}

void abi_ObjectSpin8Kind0(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSpin8Kind0_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:D0AC ObjectSpin8Kind1
 * WHAT IT DOES: ObjectSpin8Kind0 for kind 1: +$18 = $81, the turn $20.
 * HOW: As ObjectSpin8Kind0.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectSpin8Step.
 */
void ObjectSpin8Kind1(uint8_t bank, LRegs *r)
{
    spin(bank, r, 0x01, 1, 0x20);
}

/* ObjectSpin8Kind1 on carried registers. */
void ObjectSpin8Kind1_cr(Carried *cr)
{
    LRegs r = {cr->a, cr->x, cr->y, cr->p};

    ObjectSpin8Kind1(cr->db, &r);
    cr->a = r.a;
    cr->x = r.x;
    cr->y = r.y;
    set_p(cr, r.p);
}

void abi_ObjectSpin8Kind1(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSpin8Kind1_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:D0BA ObjectSpin8Kind2
 * WHAT IT DOES: ObjectSpin8Kind0 for kind 2: +$18 = $82, the turn $10.
 * HOW: As ObjectSpin8Kind0.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectSpin8Step.
 */
void ObjectSpin8Kind2(uint8_t bank, LRegs *r)
{
    spin(bank, r, 0x02, 1, 0x10);
}

/* ObjectSpin8Kind2 on carried registers. */
void ObjectSpin8Kind2_cr(Carried *cr)
{
    LRegs r = {cr->a, cr->x, cr->y, cr->p};

    ObjectSpin8Kind2(cr->db, &r);
    cr->a = r.a;
    cr->x = r.x;
    cr->y = r.y;
    set_p(cr, r.p);
}

void abi_ObjectSpin8Kind2(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSpin8Kind2_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:D0C8 ObjectSpin8Kind3
 * WHAT IT DOES: ObjectSpin8Kind0 for kind 3: +$18 = $83, the turn left as it
 * is; it holds the shared handler set-up the other three branch to.
 * HOW: Runs on into the handler set-up and ObjectSpin8Step.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectSpin8Step.
 */
void ObjectSpin8Kind3(uint8_t bank, LRegs *r)
{
    spin(bank, r, 0x03, 0, 0x00);
}

/* ObjectSpin8Kind3 on carried registers. */
void ObjectSpin8Kind3_cr(Carried *cr)
{
    LRegs r = {cr->a, cr->x, cr->y, cr->p};

    ObjectSpin8Kind3(cr->db, &r);
    cr->a = r.a;
    cr->x = r.x;
    cr->y = r.y;
    set_p(cr, r.p);
}

void abi_ObjectSpin8Kind3(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSpin8Kind3_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:D0DE ObjectSpin8Step
 * WHAT IT DOES: A tick of a spinning object: its turn +$15 grows by 8.
 * HOW: CLC, ADC #8 (8-bit).
 * Entry: M 8-bit, X 16-bit; X = object. Exit: A low byte = the new turn,
 * the flags from the ADC.
 */
void ObjectSpin8Step(uint8_t bank, LRegs *r)
{
    spin_step(bank, r);
}

/* ObjectSpin8Step on carried registers. */
void ObjectSpin8Step_cr(Carried *cr)
{
    LRegs r = {cr->a, cr->x, cr->y, cr->p};

    ObjectSpin8Step(cr->db, &r);
    cr->a = r.a;
    cr->x = r.x;
    cr->y = r.y;
    set_p(cr, r.p);
}

void abi_ObjectSpin8Step(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSpin8Step_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* ObjectRecordMatchStep's work on object X. */
static void record_match_step(uint8_t bank, LRegs *r)
{
    uint16_t v, c;
    uint8_t f;
    unsigned i;

    (cyc(62), l_ObjectSyncFromRecord(bank, r));
    lr_lda8(r, cyc_then(30, F(0x2024, r->x)));
    f = (uint8_t)(r->a & 0xFD);
    r->a = (uint16_t)((r->a & 0xFF00) | f);
    r->p = nz8(r->p, f);
    wr8(F(0x2024, r->x), cyc_then(46, f));
    r->y = r->x;
    r->p = nz16(r->p, r->y);
    r->x = rd16(cyc_then(38, ((uint32_t)bank << 16) | 0x0346));
    r->p = nz16(r->p, r->x);
    r->p &= (uint8_t)~P_M;
    lr_lda(r, cyc_then(52, F(0x2022, r->x)));
    lr_tax(r);
    cyc(16);
    lr_lda(r, 0x700058 + r->x);
    lr_cmp(r, rd16(cyc_then(30, F(0x2029, r->y))));
    r->p |= P_M;
    if (r->p & P_Z) {
        lr_lda8(r, cyc_then(68, F(0x2024, r->y)));
        f = (uint8_t)(r->a | 0x02);
        r->a = (uint16_t)((r->a & 0xFF00) | f);
        r->p = nz8(r->p, f);
        wr8(F(0x2024, r->y), cyc_then(46, f));
        r->p &= (uint8_t)~P_M;
        lr_lda(r, cyc_then(52, F(0x200E, r->y)));
        v = (uint16_t)((r->a ^ 0xFFFF) + 1);
        for (i = 0; i < 5; i++) {
            c = v >= 0x8000;
            r->p = (uint8_t)((r->p & ~P_C) | ((v & 1) ? P_C : 0));
            v = (uint16_t)((v >> 1) | (c << 15));
        }
        r->a = v;
        r->p = nz16(r->p, v);
        wr16(((uint32_t)bank << 16) | 0x1F41, cyc_then(252, v));
        lr_lda(r, cyc_then(30, F(0x2029, r->y)));
        wr16(((uint32_t)bank << 16) | 0x1F43, cyc_then(24, r->a));
        r->p |= P_M;
    } else { cyc(22); }
    cyc(80);
    r->x = r->y;
    r->p = nz16(r->p, r->x);
}

/*
 * @routine 08:D0F7 ObjectRecordMatchStep
 * WHAT IT DOES: A tick of the record-match object: it takes its position
 * back from its record (ObjectSyncFromRecord) with bit 1 of +$24 cleared;
 * when the player's ($0346) record's +$58 equals its +$29, bit 1 is set
 * again, $1F41 = minus its +$0E divided by 32 and $1F43 = its +$29.
 * HOW: The object in Y while X is the player and then its record; the
 * division as five arithmetic shifts (CMP #$8000, ROR).
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: M 8-bit; X = the
 * object, N and Z from it; Y = the object; carry from the last shift or
 * the compare.
 */
void ObjectRecordMatchStep(uint8_t bank, LRegs *r)
{
    record_match_step(bank, r);
}

/* ObjectRecordMatchStep on carried registers. */
void ObjectRecordMatchStep_cr(Carried *cr)
{
    LRegs r = {cr->a, cr->x, cr->y, cr->p};

    ObjectRecordMatchStep(cr->db, &r);
    cr->a = r.a;
    cr->x = r.x;
    cr->y = r.y;
    set_p(cr, r.p);
}

void abi_ObjectRecordMatchStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectRecordMatchStep_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:D0E8 ObjectRecordMatch
 * WHAT IT DOES: Sets up the record-match object: its handler becomes
 * $08:D0F7 (ObjectRecordMatchStep), whose first tick runs. Nothing in the
 * game calls it; a direct call is its only way in.
 * HOW: Runs on into ObjectRecordMatchStep.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as
 * ObjectRecordMatchStep.
 */
void ObjectRecordMatch(uint8_t bank, LRegs *r)
{
    handler(bank, r, r->x, 0xD0F7);
    record_match_step(bank, r);
}

/* ObjectRecordMatch on carried registers. */
void ObjectRecordMatch_cr(Carried *cr)
{
    LRegs r = {cr->a, cr->x, cr->y, cr->p};

    ObjectRecordMatch(cr->db, &r);
    cr->a = r.a;
    cr->x = r.x;
    cr->y = r.y;
    set_p(cr, r.p);
}

void abi_ObjectRecordMatch(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectRecordMatch_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}
