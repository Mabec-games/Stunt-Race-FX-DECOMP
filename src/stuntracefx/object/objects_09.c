/*
 * objects_09.c: the course objects' handlers of bank $09: the set-up a
 * course's object list points at (a long pointer per object kind in the
 * course data) and the per-frame handlers they install at +$1A/+$1C (run
 * by ObjectRunHandlerA). Most start with a NOP, as the course data's
 * pointers expect.
 *
 * Objects are at $2000 + X through the data bank (X 16-bit, M 8-bit).
 */
#include "machine.h"
#include "cpu_ops.h"

void abi_ObjectSpawn(void);
void abi_ObjectCopyPositionTo(void);
void abi_ObjectRebucket(void);
void abi_ObjectSyncFromRecord(void);
void abi_RecordContactSummary(void);
void abi_ObjectRelease(void);
void abi_GsuRecordNew42D(void);
void abi_GsuRecordFlag10(void);
void abi_GsuRecordPosition(void);
void abi_GsuRecordVelocity(void);
void abi_ApproachQuarterMin4Byte(void);
void abi_SoundPort0QueuePush(void);

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

/* LDA addr, AND #m, STA addr (8-bit). */
static void and8(uint32_t addr, uint8_t m)
{
    uint8_t v = (uint8_t)(ld8(addr) & m);

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

/* DEC addr (8-bit); returns the new value. */
static uint8_t dec8(uint32_t addr)
{
    uint8_t v = (uint8_t)(rd8(addr) - 1);

    wr8(addr, v);
    cpu.p = nz8(cpu.p, v);
    return v;
}

/* INC addr (8-bit). */
static void inc8(uint32_t addr)
{
    uint8_t v = (uint8_t)(rd8(addr) + 1);

    wr8(addr, v);
    cpu.p = nz8(cpu.p, v);
}

/* CMP #m (8-bit). */
static void cmp8(uint8_t m)
{
    uint8_t a = (uint8_t)cpu.a;

    cpu.p = (uint8_t)((nz8(cpu.p, (uint8_t)(a - m)) & ~P_C) | (a >= m ? P_C : 0));
}

/* The handler of object o: +$1A = h, +$1C = $09. */
static void handler(uint16_t o, uint16_t h)
{
    st16i(F(0x201A, o), h);
    st8i(F(0x201C, o), 0x09);
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
 * @routine 09:8B3B ObjectLampTower
 * WHAT IT DOES: Sets up a lamp on a tower: object X 116 units lower
 * ($FF74 on +$0E), its light ($5D = $9971, +$24 bit 3, +$3A = 0, +$5F =
 * $3C), shape $A104, +$09 bit 0 cleared; and, when one can be made, a
 * second light (kind $A120, $5D = $9973, +$5F = $64) at its position $578
 * higher, rebucketed; then no handler.
 * HOW: ObjectSpawn, ObjectCopyPositionTo, ObjectRebucket with X and Y
 * swapped round it. Without a free object the second light is not made.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; A low byte = 0.
 */
void abi_ObjectLampTower(void)
{
    add16(F(0x200E, cpu.x), 0xFF74);
    st16i(F(0x205D, cpu.x), 0x9971);
    lamp(cpu.x, 0x3C);
    st16i(F(0x2004, cpu.x), 0xA104);
    and8(F(0x2009, cpu.x), 0xFE);
    if (spawn(0xA120)) {
        lamp(cpu.y, 0x64);
        st16i(F(0x205D, cpu.y), 0x9973);
        abi_ObjectCopyPositionTo();
        add16(F(0x200E, cpu.y), 0xFA88);
        swap_xy();
        abi_ObjectRebucket();
        swap_xy();
    }
    handler_none(cpu.x);
}

/*
 * @routine 09:8BD4 ObjectLampLow
 * WHAT IT DOES: Sets up a lamp: its light ($5D = $9973, +$24 bit 3, +$3A =
 * 0, +$5F = $50), object X 380 units higher ($FE84 on +$0E); then no
 * handler.
 * HOW: Byte and word stores.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; A low byte = 0.
 */
void abi_ObjectLampLow(void)
{
    lamp(cpu.x, 0x50);
    st16i(F(0x205D, cpu.x), 0x9973);
    add16(F(0x200E, cpu.x), 0xFE84);
    handler_none(cpu.x);
}

/*
 * @routine 09:8C0F ObjectScale40
 * WHAT IT DOES: Sets object X's scale +$15 to $40.
 * HOW: One store.
 * Entry: M 8-bit; X = object. Exit: A low byte = $40.
 */
void abi_ObjectScale40(void)
{
    st8i(F(0x2015, cpu.x), 0x40);
}

/* 09:8CD6: the gate's frame: synced from its record; +$25 bit 0 = the
   sync's bit 0 and the record's contact summary's bit 0 together. */
static void gate_frame(void)
{
    abi_ObjectSyncFromRecord();
    lda8((uint8_t)(cpu.a & 0x01));
    if ((uint8_t)cpu.a) {
        abi_RecordContactSummary();
        lda8((uint8_t)(cpu.a & 0x01));
        if ((uint8_t)cpu.a) {
            or8(F(0x2025, cpu.x), 0x01);
            return;
        }
    }
    and8(F(0x2025, cpu.x), 0xFE);
}

/* 09:8CAF: the gate's record: a new one ($42D: GsuRecordNew42D) with flag
   bit 4, at (+$F00, +$F00, +$F00) (GsuRecordPosition); then its frame. */
static void gate_record(void)
{
    abi_GsuRecordNew42D();
    abi_GsuRecordFlag10();
    st16i(0x000002, 0x0F00);
    st16i(0x000008, 0x0F00);
    st16i(0x000094, 0x0F00);
    abi_GsuRecordPosition();
    gate_frame();
}

/*
 * @routine 09:8C16 ObjectScoreGate
 * WHAT IT DOES: Sets up a scoring gate: its restart handler (+$1D/+$1F =
 * $09:8CAF, ObjectScoreGateRecord) and frame handler ($09:8CD6,
 * ObjectScoreGateStep); a counter object (kind $A040) at its position
 * $6E0 left, $A0 lower and $F00 further on, linked back (+$27 = X), with
 * handler $09:8CF8 (ObjectScoreCounterStep), +$49 = 2, scale $20, +$5A =
 * $80, count +$48 = 0, +$0A = $50; then the gate's record and frame.
 * HOW: ObjectSpawn and ObjectCopyPositionTo; without a free object only
 * the gate is set up.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as
 * ObjectScoreGateStep.
 */
void abi_ObjectScoreGate(void)
{
    st16i(F(0x201D, cpu.x), 0x8CAF);
    st8i(F(0x201F, cpu.x), 0x09);
    handler(cpu.x, 0x8CD6);
    if (spawn(0xA040)) {
        uint8_t a;

        abi_ObjectCopyPositionTo();
        a = (uint8_t)cpu.a;                     /* PHA */
        cpu.p &= (uint8_t)~P_M;
        cpu.a = cpu.x;
        cpu.p = nz16(cpu.p, cpu.a);
        wr16(F(0x2027, cpu.y), cpu.x);
        cpu.p |= P_M;
        lda8(a);                                /* PLA */
        handler(cpu.y, 0x8CF8);
        st8i(F(0x2049, cpu.y), 0x02);
        add16(F(0x200C, cpu.y), 0xF920);
        add16(F(0x200E, cpu.y), 0x00A0);
        add16(F(0x2010, cpu.y), 0x0F00);
        st16i(F(0x2015, cpu.y), 0x0020);
        lda8(0x00);
        lda8(0x80);
        wr8(F(0x205A, cpu.y), 0x80);
        st8i(F(0x2048, cpu.y), 0x00);
        st8i(F(0x200A, cpu.y), 0x50);
    }
    gate_record();
}

/*
 * @routine 09:8CAF ObjectScoreGateRecord
 * WHAT IT DOES: The gate's restart: a new Super FX record ($42D) with
 * flag bit 4 at (+$F00, +$F00, +$F00), then ObjectScoreGateStep.
 * HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as
 * ObjectScoreGateStep.
 */
void abi_ObjectScoreGateRecord(void)
{
    gate_record();
}

/*
 * @routine 09:8CD6 ObjectScoreGateStep
 * WHAT IT DOES: The gate's frame: synced from its record; its +$25 bit 0
 * set while the record's sync and contact summary both have bit 0 set,
 * else cleared.
 * HOW: ObjectSyncFromRecord, RecordContactSummary.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; A low byte the new
 * +$25.
 */
void abi_ObjectScoreGateStep(void)
{
    gate_frame();
}

/* 09:8D94: the count +$48 down (not below 0), shown at +$5A with bit 7
   ($1F58 = the count). */
static void counter_show(uint16_t o)
{
    uint8_t v;

    v = ld8(F(0x2048, o));
    cmp8(0x00);
    if (v)
        dec8(F(0x2048, o));
    v = ld8(F(0x2048, o));
    wr8(db() | 0x1F58, v);
    v = (uint8_t)(ld8(db() | 0x1F58) | 0x80);
    lda8(v);
    wr8(F(0x205A, o), v);
}

/* 09:8D34: every third frame (+$49 counting down from 2), by its gate's
   +$25 bit 0 (+$27): passing it sets +$25 bit 0, sound $32 and counts
   +$48 up to 9; else clears it and counts down; shown at +$5A. */
static void counter_tick(void)
{
    uint16_t o = cpu.x;
    uint8_t v;

    v = ld8(F(0x2049, o));
    cmp8(0x00);
    if (v) {
        dec8(F(0x2049, o));
        return;
    }
    st8i(F(0x2049, o), 0x02);
    cpu.y = rd16(F(0x2027, o));
    cpu.p = nz16(cpu.p, cpu.y);
    lda8((uint8_t)(ld8(F(0x2025, cpu.y)) & 0x01));
    if (!(uint8_t)cpu.a) {
        and8(F(0x2025, o), 0xFE);
        counter_show(o);
        return;
    }
    lda8((uint8_t)(ld8(F(0x2025, o)) & 0x01));
    if (!(uint8_t)cpu.a) {
        or8(F(0x2025, o), 0x01);
        lda8(0x32);
        abi_SoundPort0QueuePush();
    }
    v = ld8(F(0x2048, o));
    cmp8(0x09);
    if (v != 0x09)
        inc8(F(0x2048, o));
    v = ld8(F(0x2048, o));
    wr8(db() | 0x1F58, v);
    v = (uint8_t)(ld8(db() | 0x1F58) | 0x80);
    lda8(v);
    wr8(F(0x205A, o), v);
}

/* 09:8DB1: while +$0A runs down, the scale +$15 eased toward $40
   (ApproachQuarterMin4Byte via $3E) and the object $5A further right;
   then the tick. */
static void counter_slide(void)
{
    uint16_t o = cpu.x;
    uint8_t v;

    v = ld8(F(0x200A, o));
    cmp8(0x00);
    if (v) {
        dec8(F(0x200A, o));
        v = ld8(F(0x2015, o));
        wr8(0x00003E, v);
        lda8(0x40);
        abi_ApproachQuarterMin4Byte();
        wr8(F(0x2015, o), (uint8_t)cpu.a);
        add16(F(0x200C, o), 0x005A);
    }
    counter_tick();
}

/*
 * @routine 09:8CF8 ObjectScoreCounterStep
 * WHAT IT DOES: The score counter's frame, by the lap count $0DEA: on the
 * first lap the first time its +$25 bit 1 and shape $B754, then its tick;
 * on the second it slides in (counter_slide); then rebucketed.
 * HOW: The tick (counter_tick) every third frame follows its gate's +$25
 * bit 0. Reached only by unit_objects_09.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as
 * ObjectRebucket.
 */
void abi_ObjectScoreCounterStep(void)
{
    uint8_t v;

    cpu.p |= P_M;
    v = ld8(db() | 0x0DEA);
    if (v) {
        cmp8(0x01);
        if (v == 0x01) {
            lda8((uint8_t)(ld8(F(0x2025, cpu.x)) & 0x02));
            if (!(uint8_t)cpu.a) {
                or8(F(0x2025, cpu.x), 0x02);
                st16i(F(0x2004, cpu.x), 0xB754);
            }
            counter_tick();
        } else {
            cmp8(0x02);
            if (v == 0x02)
                counter_slide();
        }
    }
    abi_ObjectRebucket();
}

/* 09:8DF8: synced from its record; released when +$0A has run down. */
static void piece_frame(void)
{
    abi_ObjectSyncFromRecord();
    if (dec8(F(0x200A, cpu.x)) == 0)
        abi_ObjectRelease();
}

/*
 * @routine 09:8DDF ObjectFlyingPiece
 * WHAT IT DOES: Sets up a flying piece: handler $09:8DF8
 * (ObjectFlyingPieceStep), a Super FX record from 08:CBCC (released at
 * once when there is none), its velocity (GsuRecordVelocity), then its
 * frame.
 * HOW: 08:CBCC as original code; its N flag tells the failure.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as the path leaves it.
 */
void abi_ObjectFlyingPiece(void)
{
    handler(cpu.x, 0x8DF8);
    call_routine(0x08CBCC, 1);
    if (cpu.p & P_N) {
        abi_ObjectRelease();
        return;
    }
    abi_GsuRecordVelocity();
    piece_frame();
}

/*
 * @routine 09:8DF8 ObjectFlyingPieceStep
 * WHAT IT DOES: A flying piece's frame: synced from its record
 * (ObjectSyncFromRecord); released once +$0A runs down.
 * HOW: DEC of the byte.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as the path leaves it.
 */
void abi_ObjectFlyingPieceStep(void)
{
    piece_frame();
}

void abi_GsuRecordSet2AHigh(void);

/* REP #$20, X/Y/Z to direct page $02, $08, $94 (each a word), SEP #$20,
   GsuRecordPosition. */
static void record_at(uint16_t x, uint16_t y, uint16_t z)
{
    st16i(0x000002, x);
    st16i(0x000008, y);
    st16i(0x000094, z);
    abi_GsuRecordPosition();
}

/* 09:8EAF: a spark: a new object of kind $A0CC lit by $996B (+$5F = t) at
   object X's position, handler $09:8DDF (ObjectFlyingPiece), velocity
   (vx, vy, vz), +$0A = $0F; nothing without a free object. */
static void spark(uint8_t t, uint16_t vx, uint16_t vy, uint16_t vz)
{
    if (!spawn(0xA0CC))
        return;
    st16i(F(0x205D, cpu.y), 0x996B);
    lamp(cpu.y, t);
    abi_ObjectCopyPositionTo();
    handler(cpu.y, 0x8DDF);
    st16i(F(0x202E, cpu.y), vx);
    st16i(F(0x2030, cpu.y), vy);
    st16i(F(0x2032, cpu.y), vz);
    st8i(F(0x200A, cpu.y), 0x0F);
}

/* 09:8E5A: the shell's frame: released when +$0A runs down (and goes on);
   its size bytes +$14-$16 grow ($0C added to the words at +$14, +$15 and
   +$16); synced from its record; on a hit (sync and contact bit 0) it is
   released with sound $56 and bursts into four sparks. */
static void shell_frame(void)
{
    if (dec8(F(0x200A, cpu.x)) == 0)
        abi_ObjectRelease();
    add16(F(0x2014, cpu.x), 0x000C);
    add16(F(0x2015, cpu.x), 0x000C);
    add16(F(0x2016, cpu.x), 0x000C);
    abi_ObjectSyncFromRecord();
    lda8((uint8_t)(cpu.a & 0x01));
    if (!(uint8_t)cpu.a)
        return;
    abi_RecordContactSummary();
    lda8((uint8_t)(cpu.a & 0x01));
    if (!(uint8_t)cpu.a)
        return;
    abi_ObjectRelease();
    lda8(0x56);
    abi_SoundPort0QueuePush();
    spark(0x14, 0x0028, 0xFFAB, 0x0000);
    spark(0x03, 0xFFD8, 0xFFAB, 0x0000);
    spark(0x0A, 0x0000, 0xFFA6, 0x0028);
    spark(0x01, 0x0000, 0xFFA6, 0xFFD8);
}

/* 09:8E2D: the shell's record (08:CBCC), velocity, +$2A high byte 8
   (GsuRecordSet2AHigh) at ($190, $190, $190); then its frame. */
static void shell_record(void)
{
    call_routine(0x08CBCC, 1);
    abi_GsuRecordVelocity();
    lda8(0x08);
    abi_GsuRecordSet2AHigh();
    record_at(0x0190, 0x0190, 0x0190);
    shell_frame();
}

/*
 * @routine 09:8E0F ObjectFireworkShell
 * WHAT IT DOES: Sets up a firework shell: frame handler $09:8E5A
 * (ObjectFireworkShellStep), restart $09:8E2D (ObjectFireworkShellRecord),
 * then its record and frame.
 * HOW: Word and byte stores; the record from 08:CBCC (original code).
 * Reached only by unit_objects_09.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void abi_ObjectFireworkShell(void)
{
    handler(cpu.x, 0x8E5A);
    st16i(F(0x201D, cpu.x), 0x8E2D);
    st8i(F(0x201F, cpu.x), 0x09);
    shell_record();
}

/*
 * @routine 09:8E2D ObjectFireworkShellRecord
 * WHAT IT DOES: The shell's restart: a Super FX record (08:CBCC) with its
 * velocity, +$2A's high byte 8, at ($190, $190, $190); then its frame.
 * HOW: GsuRecordVelocity, GsuRecordSet2AHigh, GsuRecordPosition. Reached
 * only by unit_objects_09.
 * Entry: as ObjectFireworkShell. Exit: as ObjectFireworkShell.
 */
void abi_ObjectFireworkShellRecord(void)
{
    shell_record();
}

/*
 * @routine 09:8E5A ObjectFireworkShellStep
 * WHAT IT DOES: The shell's frame: released when +$0A runs down; its size
 * bytes +$14-$16 grow; on a hit (its record's sync and contact bit 0) it is
 * released with sound $56 and bursts into four sparks (kind $A0CC, lit by
 * $996B, handler ObjectFlyingPiece, +$0A = $0F, flying out at four
 * velocities).
 * HOW: ObjectSyncFromRecord, RecordContactSummary; the sparks by
 * ObjectSpawn and ObjectCopyPositionTo. Reached only by unit_objects_09.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path
 * leaves it.
 */
void abi_ObjectFireworkShellStep(void)
{
    shell_frame();
}

/* 09:90C3: a firework shell: a new object of kind $B6E4 at object X's
   position, handler $09:8E0F (ObjectFireworkShell), moved (dx, -$500,
   -$1900), velocity (0, 0, $15E), size bytes (s14, s15, s16), +$0A = $1E. */
static void shell(uint16_t dx, uint8_t s14, uint8_t s15, uint8_t s16)
{
    if (!spawn(0xB6E4))
        return;
    abi_ObjectCopyPositionTo();
    handler(cpu.y, 0x8E0F);
    add16(F(0x200E, cpu.y), 0xFB00);
    add16(F(0x2010, cpu.y), 0xE700);
    add16(F(0x200C, cpu.y), dx);
    st16i(F(0x202E, cpu.y), 0x0000);
    st16i(F(0x2030, cpu.y), 0x0000);
    st16i(F(0x2032, cpu.y), 0x015E);
    st8i(F(0x2014, cpu.y), s14);
    st8i(F(0x2015, cpu.y), s15);
    st8i(F(0x2016, cpu.y), s16);
    st8i(F(0x200A, cpu.y), 0x1E);
}

/* 09:908D: the finish gate's frame: synced from its record; on a hit
   (sync and contact bit 0) on the second lap ($0DEA = 2), the first time
   (+$25 bit 0), sound $11 and two firework shells. */
static void finish_gate_frame(void)
{
    abi_ObjectSyncFromRecord();
    lda8((uint8_t)(cpu.a & 0x01));
    if (!(uint8_t)cpu.a)
        return;
    abi_RecordContactSummary();
    lda8((uint8_t)(cpu.a & 0x01));
    if (!(uint8_t)cpu.a)
        return;
    cpu.p |= P_M;
    ld8(db() | 0x0DEA);
    cmp8(0x02);
    if ((uint8_t)cpu.a != 0x02)
        return;
    lda8((uint8_t)(ld8(F(0x2025, cpu.x)) & 0x01));
    if ((uint8_t)cpu.a)
        return;
    or8(F(0x2025, cpu.x), 0x01);
    lda8(0x11);
    abi_SoundPort0QueuePush();
    shell(0x03C0, 0x3C, 0x32, 0x23);
    shell(0xFC40, 0x78, 0x3C, 0x0C);
}

/* 09:9066: the finish gate's record ($42D) with flag bit 4 at ($1680,
   $1680, $1680); then its frame. */
static void finish_gate_record(void)
{
    abi_GsuRecordNew42D();
    abi_GsuRecordFlag10();
    record_at(0x1680, 0x1680, 0x1680);
    finish_gate_frame();
}

/*
 * @routine 09:9048 ObjectFinishGate
 * WHAT IT DOES: Sets up the finish gate's fireworks: restart $09:9066
 * (ObjectFinishGateRecord) and frame handler $09:908D
 * (ObjectFinishGateStep), then its record and frame.
 * HOW: Word and byte stores. Reached only by unit_objects_09.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void abi_ObjectFinishGate(void)
{
    st16i(F(0x201D, cpu.x), 0x9066);
    st8i(F(0x201F, cpu.x), 0x09);
    handler(cpu.x, 0x908D);
    finish_gate_record();
}

/*
 * @routine 09:9066 ObjectFinishGateRecord
 * WHAT IT DOES: The finish gate's restart: a Super FX record ($42D) with
 * flag bit 4 at ($1680, $1680, $1680), then its frame.
 * HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition. Reached only
 * by unit_objects_09.
 * Entry: as ObjectFinishGate. Exit: as ObjectFinishGate.
 */
void abi_ObjectFinishGateRecord(void)
{
    finish_gate_record();
}

/*
 * @routine 09:908D ObjectFinishGateStep
 * WHAT IT DOES: The finish gate's frame: synced from its record; when it is
 * hit (sync and contact bit 0) on the second lap for the first time (+$25
 * bit 0), sound $11 and two firework shells (kind $B6E4, handler
 * ObjectFireworkShell) launched beside it.
 * HOW: ObjectSyncFromRecord, RecordContactSummary, ObjectSpawn. Reached
 * only by unit_objects_09.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
 * it.
 */
void abi_ObjectFinishGateStep(void)
{
    finish_gate_frame();
}

void abi_GsuRecordNew424(void);
void abi_GsuRecordNew424Alt(void);
void abi_GsuRecordMode2(void);
void abi_ApproachHalfMin2Byte(void);
void abi_ApproachQuarterMin4(void);
void abi_RecordNextContact(void);
void abi_GsuRecordRaise6A(void);

/* LDA addr, CLC, ADC #v, STA addr (8-bit). */
static void add8(uint32_t addr, uint8_t v)
{
    uint8_t r = ld8(addr);

    cpu.p &= (uint8_t)~P_C;
    r = op_adc8(r, v, &cpu.p);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | r);
    wr8(addr, r);
}

/* LDA addr, STA $3E, LDA #target, JSL to; STA addr: a byte eased toward
   the target by the routine at `to`. Returns C. */
static int ease8(uint32_t addr, uint8_t target, void (*to)(void))
{
    wr8(0x00003E, ld8(addr));
    lda8(target);
    to();
    wr8(addr, (uint8_t)cpu.a);
    return (cpu.p & P_C) != 0;
}

/* 03:D07C (not filed yet): a byte eased toward A. */
static void ease_d07c(void)
{
    call_routine(0x03D07C, 1);
}

/* The two set-up stores of a two-handler object: restart +$1D/+$1F = r,
   frame +$1A/+$1C = f. */
static void handlers2(uint16_t r, uint16_t f)
{
    st16i(F(0x201D, cpu.x), r);
    st8i(F(0x201F, cpu.x), 0x09);
    handler(cpu.x, f);
}

/* Hit by a car: the record's sync and contact summary both with bit 0
   (ObjectSyncFromRecord, RecordContactSummary, each AND #1). */
static int hit(void)
{
    abi_ObjectSyncFromRecord();
    lda8((uint8_t)(cpu.a & 0x01));
    if (!(uint8_t)cpu.a)
        return 0;
    abi_RecordContactSummary();
    lda8((uint8_t)(cpu.a & 0x01));
    return (uint8_t)cpu.a != 0;
}

/* 09:91F6: the bumper's frame: while +$48 counts down, it bulges (+$15 by
   6, +$14 and +$16 by $0A); after, +$14 eases back (ApproachHalfMin2Byte)
   and, once that holds, +$16 too while +$15 grows by $0C; then a hit sets
   +$25 bit 0 the first time (sound $54, +$48 = 6), no hit clears it. */
static void bumper_frame(void)
{
    uint8_t v = ld8(F(0x2048, cpu.x));

    if (v) {
        add8(F(0x2015, cpu.x), 0x06);
        add8(F(0x2014, cpu.x), 0x0A);
        add8(F(0x2016, cpu.x), 0x0A);
        add8(F(0x2048, cpu.x), 0xFF);
    } else if (!ease8(F(0x2014, cpu.x), 0x00, abi_ApproachHalfMin2Byte)) {
        ease8(F(0x2016, cpu.x), 0x00, abi_ApproachHalfMin2Byte);
        add8(F(0x2015, cpu.x), 0x0C);
    }
    if (!hit()) {
        and8(F(0x2025, cpu.x), 0xFE);
        return;
    }
    lda8((uint8_t)(ld8(F(0x2025, cpu.x)) & 0x01));
    if ((uint8_t)cpu.a)
        return;
    or8(F(0x2025, cpu.x), 0x01);
    lda8(0x54);
    abi_SoundPort0QueuePush();
    st8i(F(0x2048, cpu.x), 0x06);
}

/* 09:91E8: the bumper's record ($424) with flag bit 4 and +$2A high 1;
   then its frame. */
static void bumper_record(void)
{
    abi_GsuRecordNew424();
    abi_GsuRecordFlag10();
    lda8(0x01);
    abi_GsuRecordSet2AHigh();
    bumper_frame();
}

/*
 * @routine 09:91CA ObjectBumper
 * WHAT IT DOES: Sets up a bumper: restart $09:91E8 (ObjectBumperRecord),
 * frame $09:91F6 (ObjectBumperStep), then its record and frame.
 * HOW: Word and byte stores.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void abi_ObjectBumper(void)
{
    handlers2(0x91E8, 0x91F6);
    bumper_record();
}

/*
 * @routine 09:91E8 ObjectBumperRecord
 * WHAT IT DOES: The bumper's restart: a Super FX record ($424) with flag
 * bit 4 and +$2A's high byte 1, then its frame.
 * HOW: GsuRecordNew424, GsuRecordFlag10, GsuRecordSet2AHigh.
 * Entry: as ObjectBumper. Exit: as ObjectBumper.
 */
void abi_ObjectBumperRecord(void)
{
    bumper_record();
}

/*
 * @routine 09:91F6 ObjectBumperStep
 * WHAT IT DOES: The bumper's frame: while +$48 counts down it bulges
 * (+$15 by 6, +$14 and +$16 by $0A a frame); then it settles back (+$14
 * and +$16 eased to 0 by ApproachHalfMin2Byte, +$15 growing by $0C); a car
 * hitting it sets +$25 bit 0 the first time with sound $54 and a new
 * bulge (+$48 = 6); no hit clears the bit.
 * HOW: ObjectSyncFromRecord, RecordContactSummary.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: M 8-bit.
 */
void abi_ObjectBumperStep(void)
{
    bumper_frame();
}

/* 09:92BC: the switch's frame: when on (+$25 bit 0), +$48 eased toward 3
   (03:D07C); +$48 shown at +$5A with bit 7; off, a hit turns it on (+$15
   by $0C, sound $55). */
static void switch_frame(void)
{
    uint8_t v;

    lda8((uint8_t)(ld8(F(0x2025, cpu.x)) & 0x01));
    if ((uint8_t)cpu.a)
        ease8(F(0x2048, cpu.x), 0x03, ease_d07c);
    v = ld8(F(0x2048, cpu.x));
    wr8(db() | 0x1F58, v);
    v = (uint8_t)(ld8(db() | 0x1F58) | 0x80);
    lda8(v);
    wr8(F(0x205A, cpu.x), v);
    lda8((uint8_t)(ld8(F(0x2025, cpu.x)) & 0x01));
    if ((uint8_t)cpu.a)
        return;
    if (!hit())
        return;
    or8(F(0x2025, cpu.x), 0x01);
    add8(F(0x2015, cpu.x), 0x0C);
    lda8(0x55);
    abi_SoundPort0QueuePush();
}

/* 09:92AE: the switch's record ($424) with flag bit 4 and +$2A high 1;
   then its frame. */
static void switch_record(void)
{
    abi_GsuRecordNew424();
    abi_GsuRecordFlag10();
    lda8(0x01);
    abi_GsuRecordSet2AHigh();
    switch_frame();
}

/*
 * @routine 09:9289 ObjectSwitch
 * WHAT IT DOES: Sets up a switch: restart $09:92AE (ObjectSwitchRecord),
 * frame $09:92BC (ObjectSwitchStep), +$5A = $80; then its record and frame.
 * HOW: Word and byte stores.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void abi_ObjectSwitch(void)
{
    handlers2(0x92AE, 0x92BC);
    lda8(0x00);
    lda8(0x80);
    wr8(F(0x205A, cpu.x), 0x80);
    switch_record();
}

/*
 * @routine 09:92AE ObjectSwitchRecord
 * WHAT IT DOES: The switch's restart: a Super FX record ($424) with flag
 * bit 4 and +$2A's high byte 1, then its frame.
 * HOW: GsuRecordNew424, GsuRecordFlag10, GsuRecordSet2AHigh.
 * Entry: as ObjectSwitch. Exit: as ObjectSwitch.
 */
void abi_ObjectSwitchRecord(void)
{
    switch_record();
}

/*
 * @routine 09:92BC ObjectSwitchStep
 * WHAT IT DOES: The switch's frame: once on (+$25 bit 0), its count +$48
 * eases toward 3 (03:D07C); the count is shown at +$5A with bit 7; while
 * off, a car hitting it turns it on (+$15 by $0C, sound $55). The bytes at
 * 09:931C (clearing +$25 bit 0) are reached by no code.
 * HOW: ObjectSyncFromRecord, RecordContactSummary; 03:D07C as original
 * code.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: M 8-bit.
 */
void abi_ObjectSwitchStep(void)
{
    switch_frame();
}

/* 09:934D: the platform's record ($424, +$0A $90E8) in mode 2 with +$2A
   high $50 at ($500, $1180, $500); then its frame (synced). */
static void platform_record(void)
{
    abi_GsuRecordNew424Alt();
    abi_GsuRecordMode2();
    lda8(0x50);
    abi_GsuRecordSet2AHigh();
    record_at(0x0500, 0x1180, 0x0500);
    abi_ObjectSyncFromRecord();
}

/*
 * @routine 09:9325 ObjectPlatform
 * WHAT IT DOES: Sets up a platform: +$20 = $EC60, restart $09:934D
 * (ObjectPlatformRecord), frame $09:937A (ObjectPlatformStep); then its
 * record (mode 2, +$2A high $50, at ($500, $1180, $500)) and sync.
 * HOW: Word and byte stores. Reached only by unit_objects_09.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as
 * ObjectSyncFromRecord.
 */
void abi_ObjectPlatform(void)
{
    st16i(F(0x2020, cpu.x), 0xEC60);
    handlers2(0x934D, 0x937A);
    platform_record();
}

/*
 * @routine 09:934D ObjectPlatformRecord
 * WHAT IT DOES: The platform's restart: its Super FX record (GsuRecordNew424Alt,
 * GsuRecordMode2, +$2A high $50, at ($500, $1180, $500)), then synced.
 * HOW: As ObjectPlatform. Reached only by unit_objects_09.
 * Entry: as ObjectPlatform. Exit: as ObjectPlatform.
 */
void abi_ObjectPlatformRecord(void)
{
    platform_record();
}

/*
 * @routine 09:937A ObjectPlatformStep
 * WHAT IT DOES: The platform's frame: synced from its record.
 * HOW: ObjectSyncFromRecord. Reached only by unit_objects_09.
 * Entry: as ObjectSyncFromRecord. Exit: as ObjectSyncFromRecord.
 */
void abi_ObjectPlatformStep(void)
{
    abi_ObjectSyncFromRecord();
}

/* 09:93C4: the first switch post's frame: a hit squashes +$14 toward $33
   and sets bit 0 of $1A07. */
static void post1_frame(void)
{
    if (!hit())
        return;
    ease8(F(0x2014, cpu.x), 0x33, abi_ApproachHalfMin2Byte);
    cpu.p |= P_M;
    or8(db() | 0x1A07, 0x01);
}

/* 09:939D: a post's record ($42D) with flag bit 4 at (v, v, v). */
static void post_record(uint16_t v)
{
    abi_GsuRecordNew42D();
    abi_GsuRecordFlag10();
    record_at(v, v, v);
}

/*
 * @routine 09:937F ObjectPostA
 * WHAT IT DOES: Sets up the first of a gate's two posts: restart $09:939D
 * (ObjectPostARecord), frame $09:93C4 (ObjectPostAStep); its record ($42D,
 * flag bit 4, at ($1E0, $1E0, $1E0)) and frame.
 * HOW: Word and byte stores.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void abi_ObjectPostA(void)
{
    handlers2(0x939D, 0x93C4);
    post_record(0x01E0);
    post1_frame();
}

/*
 * @routine 09:939D ObjectPostARecord
 * WHAT IT DOES: The first post's restart: its record ($42D, flag bit 4, at
 * ($1E0, $1E0, $1E0)), then its frame.
 * HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition.
 * Entry: as ObjectPostA. Exit: as ObjectPostA.
 */
void abi_ObjectPostARecord(void)
{
    post_record(0x01E0);
    post1_frame();
}

/*
 * @routine 09:93C4 ObjectPostAStep
 * WHAT IT DOES: The first post's frame: when a car hits it, +$14 eases
 * toward $33 (ApproachHalfMin2Byte) and bit 0 of $1A07 is set.
 * HOW: ObjectSyncFromRecord, RecordContactSummary.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: M 8-bit.
 */
void abi_ObjectPostAStep(void)
{
    post1_frame();
}

void abi_ObjectSkyBallStep(void);

/* 09:9496: the sky ball: a new object of kind $AA34 linked to post X
   (+$27), handler $09:9520 (ObjectSkyBallStep), $C0 above and $2BC0 on,
   scale $80, +$48 = 0, +$29 and +$2B cleared, +$0A = +$49 = $0A, sound
   $0E; nothing without a free object. */
static void sky_ball(void)
{
    uint8_t a;

    if (!spawn(0xAA34))
        return;
    abi_ObjectCopyPositionTo();
    a = (uint8_t)cpu.a;
    cpu.p &= (uint8_t)~P_M;
    cpu.a = cpu.x;
    cpu.p = nz16(cpu.p, cpu.a);
    wr16(F(0x2027, cpu.y), cpu.x);
    cpu.p |= P_M;
    lda8(a);
    handler(cpu.y, 0x9520);
    add16(F(0x200C, cpu.y), 0x0000);
    add16(F(0x200E, cpu.y), 0xFF40);
    add16(F(0x2010, cpu.y), 0x2BC0);
    st8i(F(0x2015, cpu.y), 0x80);
    st8i(F(0x2048, cpu.y), 0x00);
    st16i(F(0x2029, cpu.y), 0x0000);
    st16i(F(0x202B, cpu.y), 0x0000);
    st8i(F(0x200A, cpu.y), 0x0A);
    st8i(F(0x2049, cpu.y), 0x0A);
    lda8(0x0E);
    abi_SoundPort0QueuePush();
}

/* 09:9432: the second post's frame: a hit sets its +$25 bit 0 and bit 1 of
   $1A07 the first time; with both posts hit ($1A07 = 3), the first time
   (+$25 bit 1) the sky ball, its target +$27 = this post's +$10 - $2300. */
static void post2_frame(void)
{
    if (!hit())
        return;
    lda8((uint8_t)(ld8(F(0x2025, cpu.x)) & 0x01));
    if (!(uint8_t)cpu.a) {
        or8(F(0x2025, cpu.x), 0x01);
        cpu.p |= P_M;
        or8(db() | 0x1A07, 0x02);
    }
    ld8(db() | 0x1A07);
    cmp8(0x03);
    if ((uint8_t)cpu.a != 0x03)
        return;
    lda8((uint8_t)(ld8(F(0x2025, cpu.x)) & 0x02));
    if ((uint8_t)cpu.a)
        return;
    or8(F(0x2025, cpu.x), 0x02);
    sky_ball();
    cpu.p &= (uint8_t)~P_M;
    op_lda16(F(0x2010, cpu.x));
    wr16(F(0x2027, cpu.y), cpu.a);
    cpu.p |= P_M;
    add16(F(0x2027, cpu.y), 0xDD00);
}

/*
 * @routine 09:93ED ObjectPostB
 * WHAT IT DOES: Sets up the second post: restart $09:940B
 * (ObjectPostBRecord), frame $09:9432 (ObjectPostBStep); its record ($42D,
 * flag bit 4, at ($F00, $F00, $F00)) and frame.
 * HOW: Word and byte stores.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void abi_ObjectPostB(void)
{
    handlers2(0x940B, 0x9432);
    post_record(0x0F00);
    post2_frame();
}

/*
 * @routine 09:940B ObjectPostBRecord
 * WHAT IT DOES: The second post's restart: its record ($42D, flag bit 4,
 * at ($F00, $F00, $F00)), then its frame.
 * HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition.
 * Entry: as ObjectPostB. Exit: as ObjectPostB.
 */
void abi_ObjectPostBRecord(void)
{
    post_record(0x0F00);
    post2_frame();
}

/*
 * @routine 09:9432 ObjectPostBStep
 * WHAT IT DOES: The second post's frame: when a car hits it, its +$25 bit
 * 0 and bit 1 of $1A07 the first time; once both posts are hit ($1A07 =
 * 3), the first time (its +$25 bit 1), a sky ball (kind $AA34, handler
 * ObjectSkyBallStep) whose target +$27 is this post's +$10 less $2300.
 * HOW: ObjectSyncFromRecord, RecordContactSummary, ObjectSpawn. The sky
 * ball's spawn is reached only by unit_objects_09.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path
 * leaves it.
 */
void abi_ObjectPostBStep(void)
{
    post2_frame();
}

/* 03:CDC1 (not filed yet): a word eased toward A. */
static void ease_cdc1(void)
{
    call_routine(0x03CDC1, 1);
}

/* REP #$20, LDA addr, STA $3E, LDA target, JSL to, STA addr, SEP #$20. */
static void ease16(uint32_t addr, uint32_t target, void (*to)(void))
{
    cpu.p &= (uint8_t)~P_M;
    op_lda16(addr);
    wr16(0x00003E, cpu.a);
    op_lda16(target);
    to();
    wr16(addr, cpu.a);
    cpu.p |= P_M;
}

/* 09:9599: the ball follows the player ($0346): its X eased toward the
   player's (ApproachQuarterMin4), its Z toward the player's (03:CDC1),
   then $380 back. */
static void ball_follow(void)
{
    cpu.y = rd16(db() | 0x0346);
    cpu.p = nz16(cpu.p, cpu.y);
    ease16(F(0x200C, cpu.x), F(0x200C, cpu.y), abi_ApproachQuarterMin4);
    ease16(F(0x2010, cpu.x), F(0x2010, cpu.y), ease_cdc1);
    add16(F(0x2010, cpu.x), 0xFC80);
}

/* 09:95D1 / 95FC: the ball drops (dz, dy) with +$14 eased toward t
   (03:D07C). */
static void ball_drop(uint16_t dz, uint16_t dy, uint8_t t)
{
    add16(F(0x2010, cpu.x), dz);
    add16(F(0x200E, cpu.x), dy);
    ease8(F(0x2014, cpu.x), t, ease_d07c);
}

/*
 * @routine 09:9520 ObjectSkyBallStep
 * WHAT IT DOES: The sky ball's frame: while its +$10 has not passed its
 * target +$27 it rises ($50 back, $F0 up, +$14 toward $C0) and is released
 * when +$0A runs out; past it, for +$49 frames it follows the player, then
 * the first time (+$25 bit 0) it drops a ball (kind $B7A8, handler
 * $09:9634, ObjectBallDrop) and keeps falling ($180 back, $1E up, +$14
 * toward $F8); rebucketed each frame.
 * HOW: A 16-bit compare (BEQ or BMI); ApproachQuarterMin4, 03:CDC1 and
 * 03:D07C (original code) ease the moves. Reached only by
 * unit_objects_09.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as ObjectRebucket.
 */
void abi_ObjectSkyBallStep(void)
{
    uint16_t z, t;

    cpu.p &= (uint8_t)~P_M;
    op_lda16(F(0x2010, cpu.x));
    wr16(db() | 0x1F4C, cpu.a);
    cpu.p |= P_M;
    cpu.p &= (uint8_t)~P_M;
    op_lda16(F(0x2027, cpu.x));
    t = cpu.a;
    z = rd16(db() | 0x1F4C);
    op_cmp16(z);
    cpu.p |= P_M;
    if (t == z || (cpu.p & P_N)) {
        uint8_t v = ld8(F(0x2049, cpu.x));

        if (v) {
            dec8(F(0x2049, cpu.x));
            ball_follow();
        } else {
            lda8((uint8_t)(ld8(F(0x2025, cpu.x)) & 0x01));
            if (!(uint8_t)cpu.a) {
                uint16_t sx = cpu.x, sy = cpu.y;

                or8(F(0x2025, cpu.x), 0x01);
                if (spawn(0xB7A8)) {
                    abi_ObjectCopyPositionTo();
                    handler(cpu.y, 0x9634);
                }
                cpu.y = sy;
                cpu.x = sx;
                cpu.p = nz16(cpu.p, sx);
            }
            ball_drop(0xFE80, 0xFFE2, 0xF8);
        }
    } else {
        ball_drop(0xFFB0, 0xFF10, 0xC0);
        if (dec8(F(0x200A, cpu.x)) == 0)
            abi_ObjectRelease();
    }
    abi_ObjectRebucket();
}

void abi_ObjectBallDropStep(void);

/* 09:967F: the dropped ball's frame: it grows (+$15, +$14 by $10); on a
   car's contact (the record's contacts, RecordNextContact, with bit 0) it
   raises that record's +$6A by $40 (GsuRecordRaise6A), sound $25, leaves
   a spark (kind $B7A8 at the player $0346's position, +$0A = $0A, +$27 =
   0, +$29 = the player, handler $09:AE60) and is released. */
static void drop_frame(void)
{
    uint16_t sx;

    add8(F(0x2015, cpu.x), 0x10);
    add8(F(0x2014, cpu.x), 0x10);
    abi_ObjectSyncFromRecord();
    lda8((uint8_t)(cpu.a & 0x01));
    if (!(uint8_t)cpu.a)
        return;
    for (;;) {
        abi_RecordNextContact();
        if (!(cpu.p & P_C))
            return;
        lda8((uint8_t)(cpu.a & 0x01));
        if ((uint8_t)cpu.a)
            break;
    }
    lda8(0x40);
    abi_GsuRecordRaise6A();
    cpu.p |= P_M;
    lda8(0x25);
    abi_SoundPort0QueuePush();
    sx = cpu.x;                                 /* PHX */
    cpu.x = cpu.y;                              /* TYX */
    cpu.p = nz16(cpu.p, cpu.x);
    if (spawn(0xB7A8)) {
        uint8_t a;

        cpu.x = rd16(db() | 0x0346);
        cpu.p = nz16(cpu.p, cpu.x);
        st8i(F(0x200A, cpu.y), 0x0A);
        abi_ObjectCopyPositionTo();
        st16i(F(0x2027, cpu.y), 0x0000);
        a = (uint8_t)cpu.a;
        cpu.p &= (uint8_t)~P_M;
        cpu.a = cpu.x;
        cpu.p = nz16(cpu.p, cpu.a);
        wr16(F(0x2029, cpu.y), cpu.x);
        cpu.p |= P_M;
        lda8(a);
        handler(cpu.y, 0xAE60);
    }
    cpu.x = sx;                                 /* PLX */
    cpu.p = nz16(cpu.p, sx);
    abi_ObjectRelease();
}

/* 09:9652: the dropped ball's record ($42D) with flag bit 4, +$2A high
   $14, at ($A0, $A0, $A0); then its frame. */
static void drop_record(void)
{
    abi_GsuRecordNew42D();
    abi_GsuRecordFlag10();
    lda8(0x14);
    abi_GsuRecordSet2AHigh();
    record_at(0x00A0, 0x00A0, 0x00A0);
    drop_frame();
}

/*
 * @routine 09:9634 ObjectBallDrop
 * WHAT IT DOES: Sets up a dropped ball: restart $09:9652
 * (ObjectBallDropRecord), frame $09:967F (ObjectBallDropStep); its record
 * and frame.
 * HOW: Word and byte stores. Reached only by unit_objects_09.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void abi_ObjectBallDrop(void)
{
    handlers2(0x9652, 0x967F);
    drop_record();
}

/*
 * @routine 09:9652 ObjectBallDropRecord
 * WHAT IT DOES: The dropped ball's restart: its record ($42D, flag bit 4,
 * +$2A high $14, at ($A0, $A0, $A0)), then its frame.
 * HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordSet2AHigh,
 * GsuRecordPosition. Reached only by unit_objects_09.
 * Entry: as ObjectBallDrop. Exit: as ObjectBallDrop.
 */
void abi_ObjectBallDropRecord(void)
{
    drop_record();
}

/*
 * @routine 09:967F ObjectBallDropStep
 * WHAT IT DOES: The dropped ball's frame: it grows (+$15 and +$14 by $10);
 * on a car's contact it raises that car's record +$6A by $40, plays sound
 * $25, leaves a spark (kind $B7A8 at the player's position, handler
 * $09:AE60) and is released.
 * HOW: ObjectSyncFromRecord, then RecordNextContact over the contacts.
 * Reached only by unit_objects_09.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
 * it.
 */
void abi_ObjectBallDropStep(void)
{
    drop_frame();
}
