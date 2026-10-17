/*
 * lamps_08.c: course object set-ups of bank $08 that no course in the
 * scripts places (08:D61B-D83C): a burst of fifteen break-off pieces,
 * lamps and stacks of lamps lit by their own light (+$5D) as the lamps of
 * bank $09 are, and a falling object with a Super FX record. Each is
 * entered with X = the object; unit_lamps_08 calls them in a race.
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

static uint32_t db(void)
{
    return (uint32_t)cpu.db << 16;
}

/* Object field `f` ($20xx) of object `o`, through the data bank. */
#define F(f, o) (db() | (uint16_t)((f) + (o)))

/* LDA #v with M 8-bit. */
static void lda8(uint8_t v)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, v);
}

/* LDA addr, 8-bit; returns the byte. */
static uint8_t ld8(uint32_t addr)
{
    op_lda8(addr);
    return (uint8_t)cpu.a;
}

/* LDA #v, STA addr, 8-bit. */
static void st8i(uint32_t addr, uint8_t v)
{
    lda8(v);
    wr8(addr, v);
}

/* REP #$20, LDA #v, STA addr, SEP #$20. */
static void st16i(uint32_t addr, uint16_t v)
{
    cpu.p &= (uint8_t)~P_M;
    op_lda16_imm(v);
    wr16(addr, v);
    cpu.p |= P_M;
}

/* LDA addr, ORA #m, STA addr (8-bit). */
static void or8(uint32_t addr, uint8_t m)
{
    uint8_t v = (uint8_t)(ld8(addr) | m);

    lda8(v);
    wr8(addr, v);
}

/* REP #$20, LDA addr, CLC, ADC #v, STA addr, SEP #$20. */
static void add16(uint32_t addr, uint16_t v)
{
    cpu.p &= (uint8_t)~P_M;
    op_lda16(addr);
    op_add16(v);
    wr16(addr, cpu.a);
    cpu.p |= P_M;
}

/* The handler of object o: +$1A = h, +$1C = $08. */
static void handler(uint16_t o, uint16_t h)
{
    st16i(F(0x201A, o), h);
    st8i(F(0x201C, o), 0x08);
}

/* The handler cleared: +$1A = 0, +$1C = 0. */
static void handler_none(uint16_t o)
{
    st16i(F(0x201A, o), 0x0000);
    st8i(F(0x201C, o), 0x00);
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
        if (spawn(0xA1AC)) {
            abi_ObjectCopyPositionTo();
            lo = (uint8_t)cpu.a;
            cpu.p &= (uint8_t)~P_M;
            cpu.a = cpu.x;
            wr16(F(0x2006, cpu.y), cpu.x);
            cpu.p |= P_M;
            cpu.a = (uint16_t)((cpu.a & 0xFF00) | lo);
            handler(cpu.y, 0xD337);
        }
        n--;
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | n);
        cpu.p = nz8(cpu.p, n);
    } while (n);
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
    add16(F(0x200E, cpu.x), 0xFF88);
    lamp(cpu.x, 0x28);
    st16i(F(0x205D, cpu.x), 0x9971);
    if (spawn(0xA078)) {
        lamp(cpu.y, 0x50);
        st16i(F(0x205D, cpu.y), 0x9973);
        abi_ObjectCopyPositionTo();
        add16(F(0x200E, cpu.y), 0xFEC0);
    }
    handler_none(cpu.x);
}

/* A light of the lamp stack (kind $A104, $5D = $9975, +$5F = $32) `up`
   on +$0E of object X's position, rebucketed; skipped without a free
   object. */
static void stack_light(uint16_t up)
{
    if (!spawn(0xA104))
        return;
    lamp(cpu.y, 0x32);
    st16i(F(0x205D, cpu.y), 0x9975);
    abi_ObjectCopyPositionTo();
    add16(F(0x200E, cpu.y), up);
    swap_xy();
    abi_ObjectRebucket();
    swap_xy();
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
    add16(F(0x200E, cpu.x), 0xFFBA);
    st16i(F(0x205D, cpu.x), 0x9975);
    lamp(cpu.x, 0x1E);
    stack_light(0xFD44);
    stack_light(0xFA88);
    handler_none(cpu.x);
}

/* +$24 |= 8, +$3A = 0, +$5F = $14, then no handler ($08:D81B). */
static void lamp_still(void)
{
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
    st16i(F(0x205D, cpu.x), 0x9B8F);
    add16(F(0x200E, cpu.x), 0xFFE2);
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
    abi_ObjectSyncFromRecord();
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
    abi_ObjectFallerStep();
}

/* The falling object's set-up ($08:D7C0): $12C higher ($FED4 on +$0E),
   +$1D = $08:D808 (ObjectFallerRecord), lit (+$5F = $14), falling at
   -$64 (+$30), handler $08:D816 (ObjectFallerStep); then its record. */
static void faller(void)
{
    add16(F(0x200E, cpu.x), 0xFED4);
    st16i(F(0x201D, cpu.x), 0xD808);
    st8i(F(0x201F, cpu.x), 0x08);
    lamp(cpu.x, 0x14);
    st16i(F(0x2030, cpu.x), 0xFF9C);
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
