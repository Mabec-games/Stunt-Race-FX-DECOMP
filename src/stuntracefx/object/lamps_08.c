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
#include "cpu_ops.h"

void abi_ObjectSpawn(void);
void abi_ObjectCopyPositionTo(void);
void abi_ObjectRebucket(void);
void abi_ObjectSyncFromRecord(void);
void abi_GsuRecordNew424(void);
void abi_GsuRecordFlag10(void);
void abi_GsuRecordSet2AHigh(void);
void abi_RandomStepLong(void);
void abi_ApproachSixteenthMin16Byte(void);
void abi_ApproachEighthMin8(void);
void abi_ObjectCopyPositionFrom(void);
void abi_ObjectApplyVelocity(void);
void abi_GsuRecordNew42D(void);
void abi_GsuRecordPosition(void);
void abi_RecordNextContact(void);
void abi_ObjectContactsCameraPoint(void);

static uint32_t db(void)
{
    return (uint32_t)cpu.db << 16;
}

/* Object field `f` ($20xx) of object `o`, through the data bank. */
#define F(f, o) (db() | (uint16_t)((f) + (o)))

/* LDA #v with M 8-bit. */
static void lda8(uint8_t v)
{
    cyc(20);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, v);
}

/* LDA addr, 8-bit; returns the byte. */
static uint8_t ld8(uint32_t addr)
{
    cyc(30);
    op_lda8(addr);
    return (uint8_t)cpu.a;
}

/* LDA #v, STA addr, 8-bit. */
static void st8i(uint32_t addr, uint8_t v)
{
    lda8(v);
    cyc(26);
    wr8(addr, v);
}

/* REP #$20, LDA #v, STA addr, SEP #$20. */
static void st16i(uint32_t addr, uint16_t v)
{
    cpu.p &= (uint8_t)~P_M;
    op_lda16_imm(v);
    cyc(32);
    wr16(addr, v);
    cpu.p |= P_M;
}

/* LDA addr, ORA #m, STA addr (8-bit). */
static void or8(uint32_t addr, uint8_t m)
{
    uint8_t v = (uint8_t)(ld8(addr) | m);

    lda8(v);
    cyc(26);
    wr8(addr, v);
}

/* REP #$20, LDA addr, CLC, ADC #v, STA addr, SEP #$20. */
static void add16(uint32_t addr, uint16_t v)
{
    cpu.p &= (uint8_t)~P_M;
    cyc(30);
    op_lda16(addr);
    op_add16(v);
    cyc(68);
    wr16(addr, cpu.a);
    cpu.p |= P_M;
}

/* The handler of object o: +$1A = h, +$1C = $08. */
static void handler(uint16_t o, uint16_t h)
{
    st16i(F(0x201A, o), cyc_then(6, h));
    st8i(F(0x201C, o), cyc_then(22, 0x08));
}

/* The handler cleared: +$1A = 0, +$1C = 0. */
static void handler_none(uint16_t o)
{
    st16i(F(0x201A, o), cyc_then(6, 0x0000));
    st8i(F(0x201C, o), cyc_then(22, 0x00));
cyc(44);
    }

/* A new object of `kind` ($1F2B, ObjectSpawn): 1 when one was made (C
   set), Y = it. */
static int spawn(uint16_t kind)
{
    st16i(db() | 0x1F2B, kind);
    abi_ObjectSpawn();
    return (cpu.p & P_C) != 0;
}

/* PHX, TYX, PLY: X and Y swapped, N and Z from Y. */
static void swap_xy(void)
{
    cyc(164);
    uint16_t t = cpu.x;

    cpu.x = cpu.y;
    cpu.y = t;
    cpu.p = nz16(cpu.p, cpu.y);
}

/* +$24 |= 8, +$3A = 0, +$5F = t: an object lit by its own light ($5D). */
static void lamp(uint16_t o, uint8_t t)
{
    or8(F(0x2024, o), 0x08);
    st8i(F(0x203A, o), 0x00);
    st8i(F(0x205F, o), t);
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
void abi_ObjectBreakOffBurst15(void)
{
    uint8_t n = 0x0F, lo;

    lda8(n);
    do {
        cyc(18);
        if (spawn(0xA1AC)) {
            abi_ObjectCopyPositionTo();
            lo = (uint8_t)cpu.a;
            cpu.p &= (uint8_t)~P_M;
            cpu.a = cpu.x;
            wr16(F(0x2006, cpu.y), cyc_then(66, cpu.x));
            cpu.p |= P_M;
            cpu.a = (uint16_t)((cpu.a & 0xFF00) | lo);
            cyc(50);
            handler(cpu.y, 0xD337);
        } else { cyc(48); }
        cyc(68);
        n--;
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | n);
        cpu.p = nz8(cpu.p, n);
    } while (n);
cyc(34);
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
void abi_ObjectLampPair08(void)
{
    add16(F(0x200E, cpu.x), cyc_then(22, 0xFF88));
    cyc(22);
    lamp(cpu.x, 0x28);
    st16i(F(0x205D, cpu.x), cyc_then(6, 0x9971));
    cyc(22);
    if (spawn(0xA078)) {
        lamp(cpu.y, 0x50);
        st16i(F(0x205D, cpu.y), cyc_then(6, 0x9973));
        (cyc(22), abi_ObjectCopyPositionTo());
        add16(F(0x200E, cpu.y), 0xFEC0);
    } else { cyc(26); }
    cyc(22);
    handler_none(cpu.x);
}

/* A light of the lamp stack (kind $A104, $5D = $9975, +$5F = $32) `up`
   on +$0E of object X's position, rebucketed; skipped without a free
   object. */
static void stack_light(uint16_t up)
{
    if (!spawn(0xA104))
        { cyc(48); return; }
    lamp(cpu.y, 0x32);
    st16i(F(0x205D, cpu.y), cyc_then(6, 0x9975));
    (cyc(22), abi_ObjectCopyPositionTo());
    add16(F(0x200E, cpu.y), up);
    swap_xy();
    abi_ObjectRebucket();
    (cyc(132), swap_xy());
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
void abi_ObjectLampStack(void)
{
    add16(F(0x200E, cpu.x), cyc_then(22, 0xFFBA));
    st16i(F(0x205D, cpu.x), cyc_then(28, 0x9975));
    cyc(22);
    lamp(cpu.x, 0x1E);
    stack_light(0xFD44);
    stack_light(0xFA88);
    handler_none(cpu.x);
}

/* +$24 |= 8, +$3A = 0, +$5F = $14, then no handler ($08:D81B). */
static void lamp_still(void)
{
    cyc(44);
    lamp(cpu.x, 0x14);
    handler_none(cpu.x);
}

/*
 * @routine 08:D7A2 ObjectLampLow08
 * WHAT IT DOES: Sets up a lamp lit by $9B8F: object X $1E higher ($FFE2
 * on +$0E), +$5F = $14; then no handler.
 * HOW: Word and byte stores; BRA to the shared tail at $08:D81B.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; A low byte = 0.
 */
void abi_ObjectLampLow08(void)
{
    st16i(F(0x205D, cpu.x), cyc_then(6, 0x9B8F));
    add16(F(0x200E, cpu.x), cyc_then(44, 0xFFE2));
    lamp_still();
}

/*
 * @routine 08:D816 ObjectFallerStep
 * WHAT IT DOES: The falling object's handler: the object follows its Super
 * FX record (ObjectSyncFromRecord).
 * HOW: JSL ObjectSyncFromRecord.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectSyncFromRecord.
 */
void abi_ObjectFallerStep(void)
{
    (cyc(62), abi_ObjectSyncFromRecord());
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
void abi_ObjectFallerRecord(void)
{
    abi_GsuRecordNew424();
    abi_GsuRecordFlag10();
    lda8(0x20);
    abi_GsuRecordSet2AHigh();
    (cyc(102), abi_ObjectFallerStep());
}

/* The falling object's set-up ($08:D7C0): $12C higher ($FED4 on +$0E),
   +$1D = $08:D808 (ObjectFallerRecord), lit (+$5F = $14), falling at
   -$64 (+$30), handler $08:D816 (ObjectFallerStep); then its record. */
static void faller(void)
{
    add16(F(0x200E, cpu.x), cyc_then(44, 0xFED4));
    st16i(F(0x201D, cpu.x), cyc_then(28, 0xD808));
    st8i(F(0x201F, cpu.x), cyc_then(22, 0x08));
    lamp(cpu.x, 0x14);
    st16i(F(0x2030, cpu.x), cyc_then(6, 0xFF9C));
    cyc(22);
    handler(cpu.x, 0xD816);
    abi_ObjectFallerRecord();
}

/*
 * @routine 08:D7BC ObjectFaller
 * WHAT IT DOES: Sets up a falling object: $12C higher, +$1D = $08:D808,
 * lit by its own light (+$24 bit 3, +$3A = 0, +$5F = $14), Y speed -$64,
 * handler ObjectFallerStep; then ObjectFallerRecord.
 * HOW: BRA to the set-up at $08:D7C0.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectSyncFromRecord.
 */
void abi_ObjectFaller(void)
{
    faller();
}

/*
 * @routine 08:D7BE ObjectFallerB
 * WHAT IT DOES: As ObjectFaller (a second entry for another kind).
 * HOW: BRA to the set-up at $08:D7C0.
 * Entry: as ObjectFaller. Exit: as ObjectFaller.
 */
void abi_ObjectFallerB(void)
{
    faller();
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
void abi_ObjectSpray8(void)
{
    uint16_t x0 = cpu.x, y0 = cpu.y;
    unsigned i;
    uint8_t lo;

    cpu.x = cpu.y;
    cpu.p = nz16(cpu.p, cpu.x);
    cyc(74);
    for (i = 0; i < 8; i++) {
        if (!spawn(spray8[i].kind))
            { cyc(48); continue; }
        handler(cpu.y, 0xDB7A);
        cpu.p &= (uint8_t)~P_M;
        op_lda16_imm(spray8[i].vx);
        wr16(F(0x202E, cpu.y), cyc_then(38, spray8[i].vx));
        op_lda16_imm(0x03E8);
        wr16(F(0x2030, cpu.y), cyc_then(16, 0x03E8));
        op_lda16_imm(0x1770);
        wr16(F(0x2032, cpu.y), cyc_then(16, 0x1770));
        cpu.p |= P_M;
        st16i(F(0x2029, cpu.y), cyc_then(28, spray8[i].rise));
        lo = (uint8_t)cpu.a;
        cpu.p &= (uint8_t)~P_M;
        op_txa16();
        wr16(F(0x2027, cpu.y), cyc_then(80, cpu.x));
        cpu.p |= P_M;
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | lo);
        cpu.p = nz8(cpu.p, lo);
        st8i(F(0x2015, cpu.y), cyc_then(50, spray8[i].turn));
    }
    cyc(114);
    cpu.y = y0;
    cpu.x = x0;
    cpu.p = nz16(cpu.p, cpu.x);
}

/* ObjectSprayPieceStep's work on object X. */
static void spray_piece_step(void)
{
    uint8_t v;

    op_lda8(cyc_then(30, F(0x2014, cpu.x)));
    cyc(16);
    wr8(0x00003E, (uint8_t)cpu.a);
    cyc(58);
    lda8(0x00);
    abi_ApproachSixteenthMin16Byte();
    wr8(F(0x2014, cpu.x), cyc_then(30, (uint8_t)cpu.a));
    op_lda8(cyc_then(30, F(0x2015, cpu.x)));
    cpu.p &= (uint8_t)~P_C;
    v = op_adc8((uint8_t)cpu.a, 0x08, &cpu.p);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    wr8(F(0x2015, cpu.x), cyc_then(60, v));
    cpu.y = rd16(cyc_then(30, F(0x2027, cpu.x)));
    cpu.p = nz16(cpu.p, cpu.y);
    abi_ObjectCopyPositionFrom();
    abi_ObjectApplyVelocity();
    cpu.p &= (uint8_t)~P_M;
    op_lda16(cyc_then(74, F(0x2030, cpu.x)));
    cyc(16);
    wr16(0x00003E, cpu.a);
    op_lda16(cyc_then(30, F(0x2029, cpu.x)));
    (cyc(14), abi_ApproachEighthMin8());
    wr16(F(0x2030, cpu.x), cyc_then(30, cpu.a));
    cpu.p |= P_M;
    cpu.p &= (uint8_t)~P_M;
    op_lda16(cyc_then(74, F(0x2032, cpu.x)));
    cyc(16);
    wr16(0x00003E, cpu.a);
    op_lda16_imm(0x05DC);
    abi_ApproachEighthMin8();
    wr16(F(0x2032, cpu.x), cyc_then(30, cpu.a));
    cpu.p |= P_M;
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
void abi_ObjectSprayPieceStart(void)
{
    handler(cpu.x, 0xDB90);
    abi_RandomStepLong();
    wr8(F(0x2014, cpu.x), cyc_then(144, (uint8_t)cpu.a));
    spray_piece_step();
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
void abi_ObjectSprayPieceStep(void)
{
    spray_piece_step();
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
void abi_ObjectPlayerPassCount(void)
{
    uint8_t v;
    uint16_t d;

    op_lda8(cyc_then(30, F(0x2025, cpu.x)));
    v = (uint8_t)(cpu.a & 0x01);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, v);
    if (v)
        { cyc(64); return; }
    cpu.y = rd16(cyc_then(62, db() | 0x0346));
    cpu.p = nz16(cpu.p, cpu.y);
    cpu.p &= (uint8_t)~P_M;
    op_lda16(cyc_then(52, F(0x2010, cpu.y)));
    op_sub16(rd16(cyc_then(44, F(0x2010, cpu.x))));
    if (cpu.a & 0x8000) {
        cyc(32);
        d = (uint16_t)(cpu.a ^ 0xFFFF);
        cpu.a = (uint16_t)(d + 1);
        cpu.p = nz16(cpu.p, cpu.a);
    }
    cyc(80);
    op_cmp16(0x01F4);
    cpu.p |= P_M;
    if (!(cpu.p & P_N))
        return;
    op_lda8(cyc_then(24, F(0x204C, cpu.y)));
    v = (uint8_t)(cpu.a + 1);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, v);
    wr8(F(0x204C, cpu.y), cyc_then(44, v));
    cpu.p = (uint8_t)((nz8(cpu.p, (uint8_t)(v - 0x01)) & ~P_C) | (v >= 0x01 ? P_C : 0));
    op_lda8(cyc_then(68, F(0x2025, cpu.x)));
    v = (uint8_t)(cpu.a | 0x01);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, v);
    wr8(F(0x2025, cpu.x), cyc_then(46, v));
}

/* LDA #v, STA d (direct page, D = $0000), 16-bit, between REP and SEP. */
static void dp16i(uint16_t d, uint16_t v)
{
    cpu.p &= (uint8_t)~P_M;
    op_lda16_imm(v);
    cyc(24);
    wr16(d, v);
    cpu.p |= P_M;
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
void abi_ObjectCameraPadRecord(void)
{
    abi_GsuRecordNew42D();
    abi_GsuRecordFlag10();
    dp16i(0x000002, 0x0780);
    cyc(22);
    dp16i(0x000008, 0x0780);
    cyc(22);
    dp16i(0x000094, 0x0780);
    abi_GsuRecordPosition();
}

/*
 * @routine 08:84A8 ObjectCameraPadSetup
 * WHAT IT DOES: The set-up both camera pads jump to: the record handler
 * (+$1D, +$1F) becomes $08:84B7 (ObjectCameraPadRecord), which then runs.
 * HOW: Runs on into ObjectCameraPadRecord.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as
 * ObjectCameraPadRecord.
 */
void abi_ObjectCameraPadSetup(void)
{
    st16i(F(0x201D, cpu.x), cyc_then(6, 0x84B7));
    st8i(F(0x201F, cpu.x), cyc_then(22, 0x08));
    abi_ObjectCameraPadRecord();
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
void abi_ObjectCameraPad(void)
{
    handler(cpu.x, 0x844F);
    (cyc(32), abi_ObjectCameraPadSetup());
}

/*
 * @routine 08:8472 ObjectCameraPadB
 * WHAT IT DOES: ObjectCameraPad with its own copy of the handler,
 * $08:8485 (ObjectCameraPadStepB). Nothing in the game calls it.
 * HOW: As ObjectCameraPad.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as
 * ObjectCameraPadSetup.
 */
void abi_ObjectCameraPadB(void)
{
    handler(cpu.x, 0x8485);
    (cyc(32), abi_ObjectCameraPadSetup());
}

/* The camera pads' handler: the offsets $1F4C, $1F4E, $1F50 all 0, then
   ObjectContactsCameraPoint. */
static void camera_pad_step(void)
{
    st16i(db() | 0x1F4C, 0x0000);
    st16i((cyc(22), db()) | 0x1F4E, 0x0000);
    st16i((cyc(22), db()) | 0x1F50, 0x0000);
    (cyc(146), abi_ObjectContactsCameraPoint());
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
void abi_ObjectCameraPadStep(void)
{
    camera_pad_step();
}

/*
 * @routine 08:8485 ObjectCameraPadStepB
 * WHAT IT DOES: ObjectCameraPadB's copy of ObjectCameraPadStep.
 * HOW: As ObjectCameraPadStep.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as
 * ObjectContactsCameraPoint.
 */
void abi_ObjectCameraPadStepB(void)
{
    camera_pad_step();
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
void abi_ObjectContactsCameraPoint(void)
{
    uint16_t x0;
    uint8_t f;
    static const uint16_t to[3] = {0x002C, 0x002E, 0x0030}, from[3] = {0x1F4C, 0x1F4E, 0x1F50};
    unsigned i;

    abi_ObjectSyncFromRecord();
    f = (uint8_t)(cpu.a & 0x01);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | f);
    cpu.p = nz8(cpu.p, f);
    if (!f)
        { cyc(206); return; } else { cyc(32); }
    for (;;) {
        cyc(142);
        abi_RecordNextContact();
        if (!(cpu.p & P_C))
            { cyc(116); return; } else { cyc(132); }
        f = (uint8_t)cpu.a;
        cpu.p = (uint8_t)((cpu.p & ~P_Z) | ((f & 0x04) ? 0 : P_Z));
        if (!(f & 0x04))
            continue;
        x0 = cpu.x;
        cpu.p = (uint8_t)((cpu.p & ~P_Z) | ((f & 0x01) ? 0 : P_Z));
        if (f & 0x01) {
            cpu.x = rd16(cyc_then(160, db() | 0x0346));
        } else {
            cpu.p = (uint8_t)((cpu.p & ~P_Z) | ((f & 0x02) ? 0 : (cyc(158), P_Z)));
            if (!(f & 0x02))
                goto next;
            cpu.x = rd16(cyc_then(198, db() | 0x034A));
        }
        cpu.p = nz16(cpu.p, cpu.x);
        cpu.y = rd16(F(0x204F, cpu.x));
        cpu.p = nz16(cpu.p, cpu.y);
        if (!cpu.y)
            goto next;
        cpu.x = cpu.y;
        cpu.y = rd16(cyc_then(60, F(0x204F, cpu.x)));
        cpu.p = nz16(cpu.p, cpu.y);
        cpu.p &= (uint8_t)~P_M;
        cyc(22);
        for (i = 0; i < 3; i++) {
            op_lda16(cyc_then(24, db() | from[i]));
            wr16(db() | (uint16_t)(to[i] + cpu.y), cyc_then(30, cpu.a));
        }
        cpu.p |= P_M;
next:
        cpu.x = x0;
        cpu.p = nz16(cpu.p, cpu.x);
    }
}

/* ObjectSpin8Step's work: the turn +$15 grows by 8. */
static void spin_step(void)
{
    uint8_t v;

    op_lda8(cyc_then(30, F(0x2015, cpu.x)));
    cpu.p &= (uint8_t)~P_C;
    v = op_adc8((uint8_t)cpu.a, 0x08, &cpu.p);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    wr8(F(0x2015, cpu.x), cyc_then(60, v));
cyc(44);
    }

/* A spinning set-up of kind k: +$18 = k | $80 and (unless k is 3) the
   turn +$15 = turn; then the handler $08:D0DE and its first tick. */
static void spin(uint8_t k, int has_turn, uint8_t turn)
{
    lda8(k);
    lda8((uint8_t)(k | 0x80));
    wr8(F(0x2018, cpu.x), cyc_then(22, (uint8_t)(k | 0x80)));
    if (has_turn)
        st8i(F(0x2015, cpu.x), turn);
    handler(cpu.x, 0xD0DE);
    spin_step();
}

/*
 * @routine 08:D09E ObjectSpin8Kind0
 * WHAT IT DOES: Sets up a spinning object of kind 0: +$18 = $80, the turn
 * +$15 = $30, the handler $08:D0DE (ObjectSpin8Step) and its first tick.
 * Nothing in the game calls it; a direct call is its only way in.
 * HOW: LDA #0, ORA #$80; a BRA to the shared handler set-up at 08:D0CF.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectSpin8Step.
 */
void abi_ObjectSpin8Kind0(void)
{
    spin(0x00, 1, 0x30);
}

/*
 * @routine 08:D0AC ObjectSpin8Kind1
 * WHAT IT DOES: ObjectSpin8Kind0 for kind 1: +$18 = $81, the turn $20.
 * HOW: As ObjectSpin8Kind0.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectSpin8Step.
 */
void abi_ObjectSpin8Kind1(void)
{
    spin(0x01, 1, 0x20);
}

/*
 * @routine 08:D0BA ObjectSpin8Kind2
 * WHAT IT DOES: ObjectSpin8Kind0 for kind 2: +$18 = $82, the turn $10.
 * HOW: As ObjectSpin8Kind0.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectSpin8Step.
 */
void abi_ObjectSpin8Kind2(void)
{
    spin(0x02, 1, 0x10);
}

/*
 * @routine 08:D0C8 ObjectSpin8Kind3
 * WHAT IT DOES: ObjectSpin8Kind0 for kind 3: +$18 = $83, the turn left as it
 * is; it holds the shared handler set-up the other three branch to.
 * HOW: Runs on into the handler set-up and ObjectSpin8Step.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectSpin8Step.
 */
void abi_ObjectSpin8Kind3(void)
{
    spin(0x03, 0, 0x00);
}

/*
 * @routine 08:D0DE ObjectSpin8Step
 * WHAT IT DOES: A tick of a spinning object: its turn +$15 grows by 8.
 * HOW: CLC, ADC #8 (8-bit).
 * Entry: M 8-bit, X 16-bit; X = object. Exit: A low byte = the new turn,
 * the flags from the ADC.
 */
void abi_ObjectSpin8Step(void)
{
    spin_step();
}

/* ObjectRecordMatchStep's work on object X. */
static void record_match_step(void)
{
    uint16_t v, c;
    uint8_t f;
    unsigned i;

    (cyc(62), abi_ObjectSyncFromRecord());
    op_lda8(cyc_then(110, F(0x2024, cpu.x)));
    f = (uint8_t)(cpu.a & 0xFD);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | f);
    cpu.p = nz8(cpu.p, f);
    wr8(F(0x2024, cpu.x), cyc_then(46, f));
    cpu.y = cpu.x;
    cpu.p = nz16(cpu.p, cpu.y);
    cpu.x = rd16(cyc_then(38, db() | 0x0346));
    cpu.p = nz16(cpu.p, cpu.x);
    cpu.p &= (uint8_t)~P_M;
    op_lda16(cyc_then(52, F(0x2022, cpu.x)));
    op_tax16();
    cyc(16);
    op_lda16(0x700058 + cpu.x);
    op_cmp16(rd16(cyc_then(30, F(0x2029, cpu.y))));
    cpu.p |= P_M;
    if (cpu.p & P_Z) {
        op_lda8(cyc_then(64, F(0x2024, cpu.y)));
        f = (uint8_t)(cpu.a | 0x02);
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | f);
        cpu.p = nz8(cpu.p, f);
        wr8(F(0x2024, cpu.y), cyc_then(46, f));
        cpu.p &= (uint8_t)~P_M;
        op_lda16(cyc_then(52, F(0x200E, cpu.y)));
        v = (uint16_t)((cpu.a ^ 0xFFFF) + 1);
        for (i = 0; i < 5; i++) {
            c = v >= 0x8000;
            cpu.p = (uint8_t)((cpu.p & ~P_C) | ((v & 1) ? P_C : 0));
            v = (uint16_t)((v >> 1) | (c << 15));
        }
        cpu.a = v;
        cpu.p = nz16(cpu.p, v);
        wr16(db() | 0x1F41, cyc_then(252, v));
        op_lda16(cyc_then(30, F(0x2029, cpu.y)));
        wr16(db() | 0x1F43, cyc_then(24, cpu.a));
        cpu.p |= P_M;
    } else { cyc(18); }
    cyc(80);
    cpu.x = cpu.y;
    cpu.p = nz16(cpu.p, cpu.x);
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
void abi_ObjectRecordMatchStep(void)
{
    record_match_step();
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
void abi_ObjectRecordMatch(void)
{
    handler(cpu.x, 0xD0F7);
    record_match_step();
}
