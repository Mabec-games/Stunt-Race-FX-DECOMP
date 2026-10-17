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
void abi_ApproachSixteenthMin16Byte(void);
void abi_ApproachHalfMin2(void);
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

/*
 * @routine 09:8D34 ObjectScoreCounterTick
 * WHAT IT DOES: Every third frame (+$49 counting down from 2) follows its
 * gate (+$27): while the gate's +$25 bit 0 is set, sets its own (sound $32
 * the first time) and counts +$48 up to 9; else clears it and counts +$48
 * down to 0; shows the count at +$5A with bit 7 ($1F58 = the count).
 * HOW: Byte compares and JMLs inside the bank; ends RTS.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: M 8-bit; A low byte
 * = the shown count with bit 7, or the countdown's value when it only
 * counted down; N, Z from that.
 */
void abi_ObjectScoreCounterTick(void)
{
    counter_tick();
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
 * @routine 09:8DB1 ObjectScoreCounterSlide
 * WHAT IT DOES: While +$0A runs down, eases the scale +$15 toward $40
 * (ApproachQuarterMin4Byte via $3E) and moves the counter $5A right; then
 * ObjectScoreCounterTick.
 * HOW: 16-bit add for X; ends RTS.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as
 * ObjectScoreCounterTick.
 */
void abi_ObjectScoreCounterSlide(void)
{
    counter_slide();
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
 * HOW: GsuRecordNew424Piece (08:CBCC); its N flag tells the failure.
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

/*
 * @routine 09:8E0A ObjectFlyingPieceRelease
 * WHAT IT DOES: Releases object X (ObjectRelease): where a flying piece
 * goes when it gets no Super FX record.
 * HOW: JSL ObjectRelease, RTL.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectRelease.
 */
void abi_ObjectFlyingPieceRelease(void)
{
    abi_ObjectRelease();
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

/* 09:8EAF: the shell's burst: four sparks flying out left, right, back and
   on. */
static void firework_burst(void)
{
    spark(0x14, 0x0028, 0xFFAB, 0x0000);
    spark(0x03, 0xFFD8, 0xFFAB, 0x0000);
    spark(0x0A, 0x0000, 0xFFA6, 0x0028);
    spark(0x01, 0x0000, 0xFFA6, 0xFFD8);
}

/*
 * @routine 09:8EAF ObjectFireworkBurst
 * WHAT IT DOES: Bursts a firework into four sparks (kind $A0CC, lit by
 * $996B, handler ObjectFlyingPiece, +$0A = $0F) at object X, flying out at
 * ($28, -$55, 0), (-$28, -$55, 0), (0, -$5A, $28), (0, -$5A, -$28); a spark
 * without a free object is skipped.
 * HOW: ObjectSpawn, ObjectCopyPositionTo for each; ends RTS.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: M 8-bit; A, Y as
 * the last spark leaves them.
 */
void abi_ObjectFireworkBurst(void)
{
    firework_burst();
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
    firework_burst();
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
 * HOW: Word and byte stores; the record from GsuRecordNew424Piece (08:CBCC).
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

/* 09:90C3: the finish gate's two firework shells, $3C0 right and $3C0
   left. */
static void shell_pair(void)
{
    shell(0x03C0, 0x3C, 0x32, 0x23);
    shell(0xFC40, 0x78, 0x3C, 0x0C);
}

/*
 * @routine 09:90C3 ObjectFireworkShellPair
 * WHAT IT DOES: Fires the finish gate's two firework shells (kind $B6E4,
 * handler ObjectFireworkShell), $3C0 to each side, $500 up, $1900 back,
 * rising at $15E, with their sizes and +$0A = $1E.
 * HOW: ObjectSpawn, ObjectCopyPositionTo for each; ends RTS.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: M 8-bit; A, Y as
 * the second shell leaves them.
 */
void abi_ObjectFireworkShellPair(void)
{
    shell_pair();
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
    shell_pair();
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

/* 03:D07C (ApproachSixteenthMin16Byte): a byte eased toward A. */
static void ease_d07c(void)
{
    abi_ApproachSixteenthMin16Byte();
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
 * 09:931C (ObjectClear25Bit0) are reached by no code in it.
 * HOW: ObjectSyncFromRecord, RecordContactSummary,
 * ApproachSixteenthMin16Byte (03:D07C).
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: M 8-bit.
 */
void abi_ObjectSwitchStep(void)
{
    switch_frame();
}

/*
 * @routine 09:931C ObjectClear25Bit0
 * WHAT IT DOES: Clears bit 0 of object X's +$25 (the switch's "on").
 * HOW: LDA / AND #$FE / STA, 8-bit; RTL.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: A low byte = the new +$25;
 * N, Z from it.
 */
void abi_ObjectClear25Bit0(void)
{
    and8(F(0x2025, cpu.x), 0xFE);
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

/*
 * @routine 09:9496 ObjectSkyBallLaunch
 * WHAT IT DOES: Launches the sky ball: a new object of kind $AA34 linked to
 * object X (+$27), handler ObjectSkyBallStep, $C0 up and $2BC0 on, scale
 * $80, +$0A = +$49 = $0A, sound $0E; nothing without a free object.
 * HOW: ObjectSpawn, ObjectCopyPositionTo; ends RTS.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: M 8-bit; Y = the
 * new object; A low byte as the sound push leaves it.
 */
void abi_ObjectSkyBallLaunch(void)
{
    sky_ball();
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

/* 03:CDC1 (ApproachHalfMin2): a word eased toward A. */
static void ease_cdc1(void)
{
    abi_ApproachHalfMin2();
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

/*
 * @routine 09:9599 ObjectBallFollowPlayer
 * WHAT IT DOES: Moves object X toward the player ($0346): its X eased toward
 * the player's (ApproachQuarterMin4), its Z toward the player's
 * (ApproachHalfMin2), then $380 back.
 * HOW: 16-bit; ends RTS.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: M 8-bit; Y = the
 * player's object; A as the last add leaves it.
 */
void abi_ObjectBallFollowPlayer(void)
{
    ball_follow();
}

/* 09:95D1 / 95FC: the ball drops (dz, dy) with +$14 eased toward t
   (03:D07C). */
static void ball_drop(uint16_t dz, uint16_t dy, uint8_t t)
{
    add16(F(0x2010, cpu.x), dz);
    add16(F(0x200E, cpu.x), dy);
    ease8(F(0x2014, cpu.x), t, ease_d07c);
}

/* 09:95FC: the rising ball's step, its life +$0A counted down and the
   ball released when it runs out. */
static void ball_rise(void)
{
    ball_drop(0xFFB0, 0xFF10, 0xC0);
    if (dec8(F(0x200A, cpu.x)) == 0)
        abi_ObjectRelease();
}

/*
 * @routine 09:95FC ObjectSkyBallRise
 * WHAT IT DOES: The rising sky ball's step: $50 back, $F0 up, +$14 eased
 * toward $C0; released (ObjectRelease) when +$0A runs out.
 * HOW: As ObjectSkyBallFall, then a byte DEC.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as
 * ObjectSkyBallFall.
 */
void abi_ObjectSkyBallRise(void)
{
    ball_rise();
}

/*
 * @routine 09:95D1 ObjectSkyBallFall
 * WHAT IT DOES: The falling sky ball's step: $180 back, $1E up, +$14 eased
 * toward $F8 (ApproachSixteenthMin16Byte).
 * HOW: 16-bit adds, then the byte eased; ends RTS.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: M 8-bit; A low byte
 * = the new +$14.
 */
void abi_ObjectSkyBallFall(void)
{
    ball_drop(0xFE80, 0xFFE2, 0xF8);
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
        ball_rise();
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

/* REP #$20, LDA src, STA $1F4C, then LDA dst, CLC, ADC $1F4C, STA dst,
   SEP #$20: dst += src through $1F4C. */
static void add16_via(uint32_t dst, uint32_t src)
{
    cpu.p &= (uint8_t)~P_M;
    op_lda16(src);
    wr16(db() | 0x1F4C, cpu.a);
    cpu.p |= P_M;
    cpu.p &= (uint8_t)~P_M;
    op_lda16(dst);
    op_add16(rd16(db() | 0x1F4C));
    wr16(dst, cpu.a);
    cpu.p |= P_M;
}

/* A cannon ball: a new object of kind $AA34 linked to object X (+$27),
   handler h, moved (dx, dy, dz), scale s, +$48 = 0, its rise +$29 and
   drift +$2B, life +$0A, homing frames +$49, sound $0E. */
static void cannon_ball(uint16_t h, uint16_t dx, uint16_t dy, uint16_t dz, uint8_t s,
                        uint16_t rise, uint16_t drift, uint8_t life, uint8_t home)
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
    handler(cpu.y, h);
    add16(F(0x200C, cpu.y), dx);
    add16(F(0x200E, cpu.y), dy);
    add16(F(0x2010, cpu.y), dz);
    st8i(F(0x2015, cpu.y), s);
    st8i(F(0x2048, cpu.y), 0x00);
    st16i(F(0x2029, cpu.y), rise);
    st16i(F(0x202B, cpu.y), drift);
    st8i(F(0x200A, cpu.y), life);
    st8i(F(0x2049, cpu.y), home);
    lda8(0x0E);
    abi_SoundPort0QueuePush();
}

/* 09:9746: the lap gate's frame: a hit when the lap $0DEA differs from its
   +$0A takes the lap; lap 0 fires three balls to the left ($09:9ACB),
   lap 2 three to the right ($09:9AE8). */
/* 09:9793: the first lap's three cannon balls (ObjectCannonBallLeft),
   $3C00 to the left. */
static void cannon_balls_left(void)
{
    cannon_ball(0x9ACB, 0xC400, 0xF880, 0x0500, 0xC0, 0xFD60, 0x0000, 0x05, 0x09);
    cannon_ball(0x9ACB, 0xC400, 0xFC40, 0xF880, 0xC0, 0xFD40, 0xFE00, 0x05, 0x09);
    cannon_ball(0x9ACB, 0xC400, 0xF9C0, 0x0000, 0xC0, 0xFD20, 0x0200, 0x05, 0x09);
}

/*
 * @routine 09:9793 ObjectCannonBallsLeft
 * WHAT IT DOES: Fires the first lap's three cannon balls (kind via
 * ObjectSpawn, handler ObjectCannonBallLeft) from object X, $3C00 to the
 * left, at three heights and spreads.
 * HOW: ObjectSpawn, ObjectCopyPositionTo for each; ends RTS.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: M 8-bit; A, Y as
 * the third ball leaves them.
 */
void abi_ObjectCannonBallsLeft(void)
{
    cannon_balls_left();
}

/* 09:992F: the third lap's three cannon balls (ObjectCannonBallRight),
   $2580 to the right. */
static void cannon_balls_right(void)
{
    cannon_ball(0x9AE8, 0x2580, 0xF880, 0x0500, 0x40, 0xFDC0, 0x0000, 0x12, 0x06);
    cannon_ball(0x9AE8, 0x2580, 0xFC40, 0xF880, 0x40, 0xFDA0, 0xFF00, 0x12, 0x06);
    cannon_ball(0x9AE8, 0x2580, 0xF9C0, 0x0000, 0x40, 0xFDC0, 0x0100, 0x12, 0x06);
}

/*
 * @routine 09:992F ObjectCannonBallsRight
 * WHAT IT DOES: As ObjectCannonBallsLeft for the third lap: handler
 * ObjectCannonBallRight, $2580 to the right.
 * HOW: As ObjectCannonBallsLeft.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as
 * ObjectCannonBallsLeft.
 */
void abi_ObjectCannonBallsRight(void)
{
    cannon_balls_right();
}

static void lap_gate_frame(void)
{
    uint8_t lap;

    if (!hit())
        return;
    wr8(db() | 0x1F58, ld8(F(0x200A, cpu.x)));
    cpu.p |= P_M;
    lap = ld8(db() | 0x0DEA);
    {
        uint8_t m = rd8(db() | 0x1F58);

        cpu.p = (uint8_t)((nz8(cpu.p, (uint8_t)(lap - m)) & ~P_C) | (lap >= m ? P_C : 0));
        if (lap == m)
            return;
    }
    wr8(db() | 0x1F58, ld8(db() | 0x0DEA));
    wr8(F(0x200A, cpu.x), ld8(db() | 0x1F58));
    cpu.p |= P_M;
    lap = ld8(db() | 0x0DEA);
    cmp8(0x00);
    if (lap == 0x00) {
        cannon_balls_left();
        return;
    }
    cmp8(0x01);
    if (lap == 0x01)
        return;
    cmp8(0x02);
    if (lap != 0x02)
        return;
    cannon_balls_right();
}

/* 09:971F: the lap gate's record ($42D) with flag bit 4 at ($F00, $F00,
   $F00); then its frame. */
static void lap_gate_record(void)
{
    post_record(0x0F00);
    lap_gate_frame();
}

/*
 * @routine 09:96FC ObjectCannonGate
 * WHAT IT DOES: Sets up a cannon gate: restart $09:971F
 * (ObjectCannonGateRecord), frame $09:9746 (ObjectCannonGateStep), its lap
 * +$0A = 4; then its record and frame.
 * HOW: Word and byte stores.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void abi_ObjectCannonGate(void)
{
    handlers2(0x971F, 0x9746);
    st8i(F(0x200A, cpu.x), 0x04);
    lap_gate_record();
}

/*
 * @routine 09:971F ObjectCannonGateRecord
 * WHAT IT DOES: The cannon gate's restart: its record ($42D, flag bit 4, at
 * ($F00, $F00, $F00)), then its frame.
 * HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition.
 * Entry: as ObjectCannonGate. Exit: as ObjectCannonGate.
 */
void abi_ObjectCannonGateRecord(void)
{
    lap_gate_record();
}

/*
 * @routine 09:9746 ObjectCannonGateStep
 * WHAT IT DOES: The cannon gate's frame: when a car hits it on a lap ($0DEA)
 * other than the one it last fired on (+$0A), it takes that lap; on lap 0
 * it fires three cannon balls to one side (kind $AA34, handler
 * ObjectCannonBallLeft), on lap 2 three to the other (ObjectCannonBallRight),
 * each with its own drop, rise and drift, sound $0E.
 * HOW: ObjectSyncFromRecord, RecordContactSummary; the lap compared through
 * $1F58. The firing is reached only by unit_objects_09.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
 * it.
 */
void abi_ObjectCannonGateStep(void)
{
    lap_gate_frame();
}

/* 09:9B6D: the ball homes on its gate (Y): X and Z eased toward the gate's
   (ApproachQuarterMin4), Z += +$2B; Y eased toward the gate's (03:CDC1),
   Y += +$29. */
static void ball_home(void)
{
    ease16(F(0x200C, cpu.x), F(0x200C, cpu.y), abi_ApproachQuarterMin4);
    ease16(F(0x2010, cpu.x), F(0x2010, cpu.y), abi_ApproachQuarterMin4);
    add16_via(F(0x2010, cpu.x), F(0x202B, cpu.x));
    ease16(F(0x200E, cpu.x), F(0x200E, cpu.y), ease_cdc1);
    add16_via(F(0x200E, cpu.x), F(0x2029, cpu.x));
}

/*
 * @routine 09:9B6D ObjectCannonBallHome
 * WHAT IT DOES: The cannon ball homes on its gate (Y): X and Z eased toward
 * the gate's (ApproachQuarterMin4), Z += +$2B; Y eased toward the gate's
 * (ApproachHalfMin2), Y += +$29.
 * HOW: 16-bit eases and adds; ends RTS.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: M 8-bit; A as the
 * last add leaves it.
 */
void abi_ObjectCannonBallHome(void)
{
    ball_home();
}

/* 09:9B05 / 9B39: the ball flies: Z += +$2B, X += dx; released when +$0A
   runs out. */
static void ball_fly(uint16_t dx)
{
    add16_via(F(0x2010, cpu.x), F(0x202B, cpu.x));
    add16(F(0x200C, cpu.x), dx);
    if (dec8(F(0x200A, cpu.x)) == 0)
        abi_ObjectRelease();
}

/*
 * @routine 09:9B39 ObjectCannonBallFlyRight
 * WHAT IT DOES: As ObjectCannonBallFlyLeft with X -= $380.
 * HOW: As ObjectCannonBallFlyLeft.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
 * it.
 */
void abi_ObjectCannonBallFlyRight(void)
{
    ball_fly(0xFC80);
}

/*
 * @routine 09:9B05 ObjectCannonBallFlyLeft
 * WHAT IT DOES: The left cannon ball flies: Z += +$2B, X += $100; released
 * (ObjectRelease) when +$0A runs out.
 * HOW: 16-bit adds, a byte DEC; ends RTS.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
 * it.
 */
void abi_ObjectCannonBallFlyLeft(void)
{
    ball_fly(0x0100);
}

/* The ball's frame: Y = its gate; for +$49 frames it homes, then it flies
   (dx a frame); rebucketed. */
static void cannon_ball_frame(uint16_t dx)
{
    uint8_t v;

    cpu.y = rd16(F(0x2027, cpu.x));
    cpu.p = nz16(cpu.p, cpu.y);
    v = ld8(F(0x2049, cpu.x));
    if (v) {
        dec8(F(0x2049, cpu.x));
        ball_home();
    } else {
        ball_fly(dx);
    }
    abi_ObjectRebucket();
}

/*
 * @routine 09:9ACB ObjectCannonBallLeft
 * WHAT IT DOES: A cannon ball's frame: for +$49 frames it homes on its gate
 * (+$27; X and Z eased to the gate's, then its drift +$2B and rise +$29
 * added), then it flies off ($100 a frame along X, drifting) until its life
 * +$0A runs out; rebucketed.
 * HOW: ApproachQuarterMin4, 03:CDC1 (original code), ObjectRelease,
 * ObjectRebucket. Reached only by unit_objects_09.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as ObjectRebucket.
 */
void abi_ObjectCannonBallLeft(void)
{
    cannon_ball_frame(0x0100);
}

/*
 * @routine 09:9AE8 ObjectCannonBallRight
 * WHAT IT DOES: As ObjectCannonBallLeft, flying off the other way ($380 a
 * frame back along X).
 * HOW: As ObjectCannonBallLeft. Reached only by unit_objects_09.
 * Entry: as ObjectCannonBallLeft. Exit: as ObjectCannonBallLeft.
 */
void abi_ObjectCannonBallRight(void)
{
    cannon_ball_frame(0xFC80);
}

void abi_ObjectSyncToRecord(void);

/* As hit(), with the object's position given to its record
   (ObjectSyncToRecord) instead. */
static int hit_to(void)
{
    abi_ObjectSyncToRecord();
    lda8((uint8_t)(cpu.a & 0x01));
    if (!(uint8_t)cpu.a)
        return 0;
    abi_RecordContactSummary();
    lda8((uint8_t)(cpu.a & 0x01));
    return (uint8_t)cpu.a != 0;
}

/* A wave ball: a new object of kind $AA34 linked to gate X (+$27),
   handler h, moved (dx, dy, dz), scale s, rise +$29 = rise; the gate moves
   (gx, gz); life +$0A, sound $0E. Nothing without a free object. */
static void wave_ball(uint16_t h, uint16_t dx, uint16_t dy, uint16_t dz, uint8_t s,
                      uint16_t rise, uint16_t gx, uint16_t gz, uint8_t life)
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
    handler(cpu.y, h);
    add16(F(0x200C, cpu.y), dx);
    add16(F(0x200E, cpu.y), dy);
    add16(F(0x2010, cpu.y), dz);
    st8i(F(0x2015, cpu.y), s);
    st16i(F(0x2029, cpu.y), rise);
    add16(F(0x200C, cpu.x), gx);
    add16(F(0x2010, cpu.x), gz);
    st8i(F(0x200A, cpu.y), life);
    lda8(0x0E);
    abi_SoundPort0QueuePush();
}

/* 09:9CE4: the first gate's big wave ball (ObjectWaveBallBig). */
static void wave_ball_a(void)
{
    wave_ball(0x9F05, 0xDA80, 0xFE80, 0xDD00, 0xE0, 0xFF00, 0x0F00, 0x0F00, 0x0F);
}

/*
 * @routine 09:9CE4 ObjectWaveBallLaunchA
 * WHAT IT DOES: Launches the first wave gate's big ball (kind $AA34, handler
 * ObjectWaveBallBig) linked to gate X: $2580 left, $180 up, $2300 back,
 * scale $E0, rise -$100, life $0F; the gate moves $F00 on in X and Z; sound
 * $0E. Nothing without a free object.
 * HOW: ObjectSpawn, ObjectCopyPositionTo, 16-bit adds; ends RTS.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: M 8-bit; Y = the
 * new object; A low byte as the sound push leaves it.
 */
void abi_ObjectWaveBallLaunchA(void)
{
    wave_ball_a();
}

/* 09:9D76: the second gate's wave ball (ObjectWaveBallStep), $1E00 back. */
static void wave_ball_b(void)
{
    wave_ball(0x9E9A, 0x2300, 0xFC40, 0xE200, 0x20, 0xFF60, 0xF100, 0x0F00, 0x0A);
}

/*
 * @routine 09:9D76 ObjectWaveBallLaunchB
 * WHAT IT DOES: As ObjectWaveBallLaunchA for the second gate: handler
 * ObjectWaveBallStep, $2300 right, $3C0 up, $1E00 back, scale $20, rise
 * -$A0, life $0A; the gate moves $F00 back in X and on in Z.
 * HOW: As ObjectWaveBallLaunchA.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as
 * ObjectWaveBallLaunchA.
 */
void abi_ObjectWaveBallLaunchB(void)
{
    wave_ball_b();
}

/* 09:9E08: the third gate's wave ball, $1E00 on. */
static void wave_ball_c(void)
{
    wave_ball(0x9E9A, 0x2300, 0xFC40, 0x1E00, 0x60, 0xFF60, 0xF100, 0xF100, 0x0C);
}

/*
 * @routine 09:9E08 ObjectWaveBallLaunchC
 * WHAT IT DOES: As ObjectWaveBallLaunchB for the third gate: the ball $1E00
 * on, scale $60, life $0C; the gate moves $F00 back in X and Z.
 * HOW: As ObjectWaveBallLaunchA.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as
 * ObjectWaveBallLaunchA.
 */
void abi_ObjectWaveBallLaunchC(void)
{
    wave_ball_c();
}

/* A wave gate's frame: on a hit on lap 1, the first time (+$25 bit 0),
   its wave ball. */
static void wave_gate_frame(int which)
{
    if (!hit_to())
        return;
    ld8(db() | 0x0DEA);
    cmp8(0x01);
    if ((uint8_t)cpu.a != 0x01)
        return;
    lda8((uint8_t)(ld8(F(0x2025, cpu.x)) & 0x01));
    if ((uint8_t)cpu.a)
        return;
    or8(F(0x2025, cpu.x), 0x01);
    if (which == 0)
        wave_ball_a();
    else if (which == 1)
        wave_ball_b();
    else
        wave_ball_c();
}

/* 09:9BF5: the wave gate's record ($42D) with flag bit 4 at ($F00, $F00,
   $F00); then the first gate's frame. */
static void wave_gate_record(void)
{
    post_record(0x0F00);
    wave_gate_frame(0);
}

/*
 * @routine 09:9BD7 ObjectWaveGateA
 * WHAT IT DOES: Sets up the first wave gate: restart $09:9BF5
 * (ObjectWaveGateRecord), frame $09:9C1C (ObjectWaveGateAStep); its record
 * and frame.
 * HOW: Word and byte stores.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void abi_ObjectWaveGateA(void)
{
    handlers2(0x9BF5, 0x9C1C);
    wave_gate_record();
}

/*
 * @routine 09:9BF5 ObjectWaveGateRecord
 * WHAT IT DOES: A wave gate's restart (all three use it): its record
 * ($42D, flag bit 4, at ($F00, $F00, $F00)), then the first gate's frame
 * (ObjectWaveGateAStep).
 * HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition.
 * Entry: as ObjectWaveGateA. Exit: as ObjectWaveGateA.
 */
void abi_ObjectWaveGateRecord(void)
{
    wave_gate_record();
}

/*
 * @routine 09:9C1C ObjectWaveGateAStep
 * WHAT IT DOES: The first wave gate's frame: when a car hits it on lap 1
 * ($0DEA), the first time (+$25 bit 0), a wave ball (kind $AA34, handler
 * ObjectWaveBallBig) $2580 left, $180 up and $2300 back, and the gate moves
 * $F00 on in X and Z.
 * HOW: ObjectSyncToRecord, RecordContactSummary, ObjectSpawn.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
 * it.
 */
void abi_ObjectWaveGateAStep(void)
{
    wave_gate_frame(0);
}

/*
 * @routine 09:9C4A ObjectWaveGateB
 * WHAT IT DOES: Sets up the second wave gate: restart $09:9BF5
 * (ObjectWaveGateRecord), frame $09:9C69 (ObjectWaveGateBStep).
 * HOW: Word and byte stores.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; A low byte = 9.
 */
void abi_ObjectWaveGateB(void)
{
    handlers2(0x9BF5, 0x9C69);
}

/*
 * @routine 09:9C69 ObjectWaveGateBStep
 * WHAT IT DOES: As ObjectWaveGateAStep with a smaller ball (handler
 * ObjectWaveBallStep, $2300 right, $3C0 up, $1E00 back, scale $20, life
 * $A); the gate moves $F00 back in X and on in Z.
 * HOW: As ObjectWaveGateAStep.
 * Entry: as ObjectWaveGateAStep. Exit: as ObjectWaveGateAStep.
 */
void abi_ObjectWaveGateBStep(void)
{
    wave_gate_frame(1);
}

/*
 * @routine 09:9C97 ObjectWaveGateC
 * WHAT IT DOES: Sets up the third wave gate: restart $09:9BF5
 * (ObjectWaveGateRecord), frame $09:9CB6 (ObjectWaveGateCStep).
 * HOW: Word and byte stores.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; A low byte = 9.
 */
void abi_ObjectWaveGateC(void)
{
    handlers2(0x9BF5, 0x9CB6);
}

/*
 * @routine 09:9CB6 ObjectWaveGateCStep
 * WHAT IT DOES: As ObjectWaveGateBStep with the ball $1E00 on (scale $60,
 * life $C); the gate moves $F00 back in X and Z.
 * HOW: As ObjectWaveGateAStep.
 * Entry: as ObjectWaveGateAStep. Exit: as ObjectWaveGateAStep.
 */
void abi_ObjectWaveGateCStep(void)
{
    wave_gate_frame(2);
}

/* 09:9EA6: the ball homes on Y (X, Z, Y eased by ApproachQuarterMin4),
   rises by +$29, and is released when +$0A runs out. */
static void wave_home(void)
{
    ease16(F(0x200C, cpu.x), F(0x200C, cpu.y), abi_ApproachQuarterMin4);
    ease16(F(0x2010, cpu.x), F(0x2010, cpu.y), abi_ApproachQuarterMin4);
    ease16(F(0x200E, cpu.x), F(0x200E, cpu.y), abi_ApproachQuarterMin4);
    add16_via(F(0x200E, cpu.x), F(0x2029, cpu.x));
    if (dec8(F(0x200A, cpu.x)) == 0)
        abi_ObjectRelease();
}

/*
 * @routine 09:9EA6 ObjectWaveBallHome
 * WHAT IT DOES: The wave ball homes on Y: X, Z and Y eased toward Y's
 * (ApproachQuarterMin4), rises by +$29, and is released when +$0A runs out.
 * HOW: 16-bit eases and adds, a byte DEC; ends RTS.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
 * it.
 */
void abi_ObjectWaveBallHome(void)
{
    wave_home();
}

/*
 * @routine 09:9E9A ObjectWaveBallStep
 * WHAT IT DOES: A wave ball's frame: it homes on its gate (+$27; X, Z and
 * Y eased by ApproachQuarterMin4), rises by +$29, is released when its
 * life +$0A runs out; rebucketed.
 * HOW: The NOP first, as the course pointers expect. Reached only by
 * unit_objects_09.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as ObjectRebucket.
 */
void abi_ObjectWaveBallStep(void)
{
    cpu.y = rd16(F(0x2027, cpu.x));
    cpu.p = nz16(cpu.p, cpu.y);
    wave_home();
    abi_ObjectRebucket();
}

/* 09:A03F: a sparkle's frame: released when +$0A runs out; its frame
   +$18 steps through 0-5 (bit 7 kept); rebucketed. */
static void sparkle_frame(void)
{
    uint8_t v;

    if (dec8(F(0x200A, cpu.x)) == 0)
        abi_ObjectRelease();
    v = ld8(F(0x2018, cpu.x));
    cpu.p &= (uint8_t)~P_C;
    v = op_adc8(v, 0x01, &cpu.p);
    if (!(v & 0x80)) {
        cpu.p &= (uint8_t)~P_C;
        v = op_adc8(v, 0x06, &cpu.p);
    }
    v &= 0x7F;
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cmp8(0x06);
    if (v >= 0x06) {
        cpu.p |= P_C;
        v = op_sbc8(v, 0x06, &cpu.p);
    }
    v |= 0x80;
    lda8(v);
    wr8(F(0x2018, cpu.x), v);
    abi_ObjectRebucket();
}

/* 09:9FE1: a sparkle: a new object of kind $A158 lit by $997C (+$5F
   $64), frame +$18 = $82, at object X's position $C8 back, life 8,
   handler $09:A03F (ObjectStarSparkleStep). */
static void sparkle(void)
{
    if (!spawn(0xA158))
        return;
    st16i(F(0x205D, cpu.y), 0x997C);
    lda8(0x02);
    lda8(0x82);
    wr8(F(0x2018, cpu.y), 0x82);
    lamp(cpu.y, 0x64);
    abi_ObjectCopyPositionTo();
    add16(F(0x2010, cpu.y), 0xFF38);
    st8i(F(0x200A, cpu.y), 0x08);
    handler(cpu.y, 0xA03F);
}

/*
 * @routine 09:9FE1 ObjectStarSparkleSpawn
 * WHAT IT DOES: Leaves a sparkle: a new object of kind $A158 lit by $997C
 * (+$5F = $64), frame +$18 = $82, at object X's position $C8 back, life 8,
 * handler ObjectStarSparkleStep; nothing without a free object.
 * HOW: ObjectSpawn, ObjectCopyPositionTo; ends RTS.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: M 8-bit; Y = the
 * new object; A low byte = 9.
 */
void abi_ObjectStarSparkleSpawn(void)
{
    sparkle();
}

/* 09:9F63: the big wave ball's frame: once hit (+$25 bit 0) it grows
   (+$16, +$15 by $18), flies ($32 back, $28A right, $FA up) and leaves a
   sparkle each frame; before, it homes on its gate. A hit (its position
   given to the record) sets +$25 bit 0 the first time with sound $59. */
static void big_ball_frame(void)
{
    lda8((uint8_t)(ld8(F(0x2025, cpu.x)) & 0x01));
    if ((uint8_t)cpu.a) {
        add8(F(0x2016, cpu.x), 0x18);
        add8(F(0x2015, cpu.x), 0x18);
        add16(F(0x2010, cpu.x), 0xFFCE);
        add16(F(0x200C, cpu.x), 0x028A);
        add16(F(0x200E, cpu.x), 0xFF06);
        sparkle();
    } else {
        uint16_t sy = cpu.y;

        cpu.y = rd16(F(0x2027, cpu.x));
        cpu.p = nz16(cpu.p, cpu.y);
        wave_home();
        cpu.y = sy;
        cpu.p = nz16(cpu.p, sy);
    }
    if (!hit_to())
        return;
    lda8((uint8_t)(ld8(F(0x2025, cpu.x)) & 0x01));
    if ((uint8_t)cpu.a)
        return;
    or8(F(0x2025, cpu.x), 0x01);
    lda8(0x59);
    abi_SoundPort0QueuePush();
}

/* 09:9F3C: the big ball's record ($42D) with flag bit 4 at ($320, $320,
   $320); then its frame. */
static void big_ball_record(void)
{
    post_record(0x0320);
    big_ball_frame();
}

/* 09:9F14: restart $09:9F3C, frame $09:9F63, rise +$29 = -$50; then the
   record and frame. */
static void big_ball_setup(void)
{
    handlers2(0x9F3C, 0x9F63);
    st16i(F(0x2029, cpu.x), 0xFFB0);
    big_ball_record();
}

/*
 * @routine 09:9F05 ObjectWaveBallBig
 * WHAT IT DOES: The first wave gate's ball: its handler $09:9F14
 * (ObjectWaveBallBigSetup), then that set-up at once.
 * HOW: Word and byte stores. Reached only by unit_objects_09.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void abi_ObjectWaveBallBig(void)
{
    handler(cpu.x, 0x9F14);
    big_ball_setup();
}

/*
 * @routine 09:9F14 ObjectWaveBallBigSetup
 * WHAT IT DOES: The big wave ball's set-up: restart $09:9F3C
 * (ObjectWaveBallBigRecord), frame $09:9F63 (ObjectWaveBallBigStep), rise
 * +$29 = -$50; its record ($42D, flag bit 4, at ($320, $320, $320)) and
 * frame.
 * HOW: Word and byte stores. Reached only by unit_objects_09.
 * Entry: as ObjectWaveBallBig. Exit: as ObjectWaveBallBig.
 */
void abi_ObjectWaveBallBigSetup(void)
{
    big_ball_setup();
}

/*
 * @routine 09:9F3C ObjectWaveBallBigRecord
 * WHAT IT DOES: The big wave ball's restart: its record ($42D, flag bit 4,
 * at ($320, $320, $320)), then its frame.
 * HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition. Reached only
 * by unit_objects_09.
 * Entry: as ObjectWaveBallBig. Exit: as ObjectWaveBallBig.
 */
void abi_ObjectWaveBallBigRecord(void)
{
    big_ball_record();
}

/*
 * @routine 09:9F63 ObjectWaveBallBigStep
 * WHAT IT DOES: The big wave ball's frame: once a car has hit it (+$25 bit
 * 0) it grows (+$16, +$15 by $18), flies off ($32 back, $28A right, $FA
 * up) and leaves a sparkle (kind $A158, handler ObjectStarSparkleStep) each
 * frame; until then it homes on its gate. A hit sets +$25 bit 0 the first
 * time with sound $59.
 * HOW: ObjectSyncToRecord, RecordContactSummary. Reached only by
 * unit_objects_09.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
 * it.
 */
void abi_ObjectWaveBallBigStep(void)
{
    big_ball_frame();
}

/*
 * @routine 09:A03F ObjectStarSparkleStep
 * WHAT IT DOES: A sparkle's frame: released when its life +$0A runs out;
 * its picture frame +$18 steps through 0-5 (bit 7 kept); rebucketed.
 * HOW: Byte adds and compares. Reached only by unit_objects_09.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as ObjectRebucket.
 */
void abi_ObjectStarSparkleStep(void)
{
    sparkle_frame();
}

/*
 * @routine 09:A0FA ObjectLanternBurst
 * WHAT IT DOES: Bursts the lantern into four sparks (kind $A0CC, lit by
 * $996B, handler ObjectFlyingPiece, +$0A = $0F) flying out at ($28, -$2D,
 * 0), (-$28, -$2D, 0), (0, -$32, $28) and (0, -$32, -$28); a spark is
 * skipped without a free object.
 * HOW: Four ObjectSpawn blocks, each from object X's position.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the last spark
 * leaves it.
 */
void abi_ObjectLanternBurst(void)
{
    spark(0x14, 0x0028, 0xFFD3, 0x0000);
    spark(0x03, 0xFFD8, 0xFFD3, 0x0000);
    spark(0x0A, 0x0000, 0xFFCE, 0x0028);
    spark(0x01, 0x0000, 0xFFCE, 0xFFD8);
}

/* 09:A0C0: the lantern's frame: when a car hits it the first time (+$25
   bit 0), sound $10, it is released and bursts into four sparks; no hit
   clears the bit. */
static void lantern_frame(void)
{
    if (!hit()) {
        and8(F(0x2025, cpu.x), 0xFE);
        return;
    }
    lda8((uint8_t)(ld8(F(0x2025, cpu.x)) & 0x01));
    if ((uint8_t)cpu.a)
        return;
    or8(F(0x2025, cpu.x), 0x01);
    lda8(0x10);
    abi_SoundPort0QueuePush();
    abi_ObjectRelease();
    abi_ObjectLanternBurst();
}

/* 09:A0B2: the lantern's record ($424, +$0A $90E8) with flag bit 4 and
   +$2A high 2; then its frame. */
static void lantern_record(void)
{
    abi_GsuRecordNew424Alt();
    abi_GsuRecordFlag10();
    lda8(0x02);
    abi_GsuRecordSet2AHigh();
    lantern_frame();
}

/*
 * @routine 09:A06A ObjectLantern
 * WHAT IT DOES: Sets up a lantern: $78 higher, lit by $996D (+$24 bit 3,
 * +$3A = 0, +$5F = $64), restart $09:A0B2 (ObjectLanternRecord), frame
 * $09:A0C0 (ObjectLanternStep); its record and frame.
 * HOW: Word and byte stores.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void abi_ObjectLantern(void)
{
    add16(F(0x200E, cpu.x), 0xFF88);
    st16i(F(0x205D, cpu.x), 0x996D);
    lamp(cpu.x, 0x64);
    handlers2(0xA0B2, 0xA0C0);
    lantern_record();
}

/*
 * @routine 09:A0B2 ObjectLanternRecord
 * WHAT IT DOES: The lantern's restart: its record (GsuRecordNew424Alt, flag
 * bit 4, +$2A high 2), then its frame.
 * HOW: GsuRecordNew424Alt, GsuRecordFlag10, GsuRecordSet2AHigh.
 * Entry: as ObjectLantern. Exit: as ObjectLantern.
 */
void abi_ObjectLanternRecord(void)
{
    lantern_record();
}

/*
 * @routine 09:A0C0 ObjectLanternStep
 * WHAT IT DOES: The lantern's frame: when a car hits it the first time
 * (+$25 bit 0), sound $10, it is released and bursts into four sparks
 * (kind $A0CC, handler ObjectFlyingPiece); no hit clears the bit.
 * HOW: ObjectSyncFromRecord, RecordContactSummary, ObjectSpawn.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
 * it.
 */
void abi_ObjectLanternStep(void)
{
    lantern_frame();
}

/* ---- 09:A293-B0A0: the course gates and markers ---- */

void abi_SoundPort1QueuePush(void);
void abi_GsuRecordLower82(void);
void abi_ObjectCopyPositionFrom(void);
void abi_ObjectStep48To8(void);
void abi_HudCountMove19EECopy(void);
void abi_GsuRecordFlags8ASet(void);
void abi_GsuRecordFlags8AClear(void);

/* LDA #v, STA addr with M already 16-bit. */
static void st16w(uint32_t addr, uint16_t v)
{
    op_lda16_imm(v);
    wr16(addr, v);
}

/* LDA #v, JSL SoundPort1QueuePush. */
static void sound1(uint8_t v)
{
    lda8(v);
    abi_SoundPort1QueuePush();
}

/* LDA #v, JSL SoundPort0QueuePush. */
static void sound0(uint8_t v)
{
    lda8(v);
    abi_SoundPort0QueuePush();
}

/* LDA +$25, AND #m: whether bit m of the gate's +$25 is set. */
static int bit25(uint8_t m)
{
    lda8((uint8_t)(ld8(F(0x2025, cpu.x)) & m));
    return (uint8_t)cpu.a != 0;
}

/* SEP #$20, LDA $0DEA, CMP #lap: whether this is lap `lap`. */
static int lap_is(uint8_t lap)
{
    cpu.p |= P_M;
    ld8(db() | 0x0DEA);
    cmp8(lap);
    return (uint8_t)cpu.a == lap;
}

/* A gate's record: a new one ($42D) with flag bit 4 at (s, s, s). */
static void gate42d(uint16_t s)
{
    abi_GsuRecordNew42D();
    abi_GsuRecordFlag10();
    record_at(s, s, s);
}

/* ObjectSyncFromRecord, AND #1: whether the record has contacts. */
static int synced(void)
{
    abi_ObjectSyncFromRecord();
    lda8((uint8_t)(cpu.a & 0x01));
    return (uint8_t)cpu.a != 0;
}

/* RecordNextContact until a contact whose flags AND #1 are set: 1 when
   one is found, 0 when the list runs out (carry clear). */
static int next_contact(void)
{
    for (;;) {
        abi_RecordNextContact();
        if (!(cpu.p & P_C))
            return 0;
        lda8((uint8_t)(cpu.a & 0x01));
        if ((uint8_t)cpu.a)
            return 1;
    }
}

/* REP #$20, A = n (16-bit), JSL to: a car record's +$8A bits set or
   cleared (08:B72E / 08:B743). */
static void car_bits16(uint16_t n, void (*to)(void))
{
    cpu.p &= (uint8_t)~P_M;
    op_lda16_imm(n);
    to();
}

/* The slide's step, target and direction ($19F8, $19FA, $19FC), M
   16-bit. */
static void slide_set(uint16_t step, uint16_t target, uint16_t dir)
{
    st16w(db() | 0x19F8, step);
    st16w(db() | 0x19FA, target);
    st16w(db() | 0x19FC, dir);
}

/* The fade's step $1A00 and end $1A02, M 16-bit. */
static void fade_set(uint16_t step, uint16_t end)
{
    st16w(db() | 0x1A00, step);
    st16w(db() | 0x1A02, end);
}

/* SEP #$20, $1A04 = $FF, $1A05 = 0. */
static void slide_hold(void)
{
    cpu.p |= P_M;
    st8i(db() | 0x1A04, 0xFF);
    st8i(db() | 0x1A05, 0x00);
}

/* 09:A2D8: the chime gate's frame: a car's hit, the first time (+$25 bit
   0, clearing bit 1), sound $13 on port 1; no hit, the first time (+$25
   bit 1, clearing bit 0), sound $0E. */
static void chime_frame(void)
{
    if (hit()) {
        if (bit25(0x01))
            return;
        or8(F(0x2025, cpu.x), 0x01);
        and8(F(0x2025, cpu.x), 0xFD);
        sound1(0x13);
        return;
    }
    if (bit25(0x02))
        return;
    or8(F(0x2025, cpu.x), 0x02);
    and8(F(0x2025, cpu.x), 0xFE);
    sound1(0x0E);
}

/*
 * @routine 09:A293 ObjectChimeGate
 * WHAT IT DOES: Sets up a chime gate: restart $09:A2B1
 * (ObjectChimeGateRecord), frame $09:A2D8 (ObjectChimeGateStep); its record
 * and frame.
 * HOW: Word and byte stores. Reached only by unit_gates_09.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void abi_ObjectChimeGate(void)
{
    handlers2(0xA2B1, 0xA2D8);
    gate42d(0x0F00);
    chime_frame();
}

/*
 * @routine 09:A2B1 ObjectChimeGateRecord
 * WHAT IT DOES: The chime gate's restart: its record ($42D, flag bit 4, at
 * ($F00, $F00, $F00)), then its frame.
 * HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition. Reached only
 * by unit_gates_09.
 * Entry: as ObjectChimeGate. Exit: as ObjectChimeGate.
 */
void abi_ObjectChimeGateRecord(void)
{
    gate42d(0x0F00);
    chime_frame();
}

/*
 * @routine 09:A2D8 ObjectChimeGateStep
 * WHAT IT DOES: The chime gate's frame: when a car enters it (contact bit
 * 0) the first time, sound $13 on port 1; when it leaves, sound $0E; +$25
 * bit 0 marks inside, bit 1 outside.
 * HOW: ObjectSyncFromRecord, RecordContactSummary, SoundPort1QueuePush.
 * Reached only by unit_gates_09.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
 * it.
 */
void abi_ObjectChimeGateStep(void)
{
    chime_frame();
}

/* 09:A371 / A408: a flash gate's frame: a car's hit, the first time (+$25
   bit 0), flashes the screen ($19C5 = flash step) and starts a fade
   ($1A00 step, $1A02 end; $0DA5 bits 0 and 5); no hit clears the bit. */
static void flash_frame(uint16_t flash, uint16_t step, uint16_t end)
{
    if (hit()) {
        if (bit25(0x01))
            return;
        or8(F(0x2025, cpu.x), 0x01);
        cpu.p &= (uint8_t)~P_M;
        st16w(0x0019C5, flash);
        fade_set(step, end);
        cpu.p |= P_M;
        or8(db() | 0x0DA5, 0x21);
        return;
    }
    cpu.p |= P_M;
    and8(F(0x2025, cpu.x), 0xFE);
}

/*
 * @routine 09:A32C ObjectBrightGate
 * WHAT IT DOES: Sets up a bright gate: restart $09:A34A
 * (ObjectBrightGateRecord), frame $09:A371 (ObjectBrightGateStep); its
 * record and frame.
 * HOW: Word and byte stores. Reached only by unit_gates_09.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void abi_ObjectBrightGate(void)
{
    handlers2(0xA34A, 0xA371);
    gate42d(0x0780);
    flash_frame(0x0020, 0x0200, 0x2000);
}

/*
 * @routine 09:A34A ObjectBrightGateRecord
 * WHAT IT DOES: The bright gate's restart: its record ($42D, flag bit 4,
 * at ($780, $780, $780)), then its frame.
 * HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition. Reached only
 * by unit_gates_09.
 * Entry: as ObjectBrightGate. Exit: as ObjectBrightGate.
 */
void abi_ObjectBrightGateRecord(void)
{
    gate42d(0x0780);
    flash_frame(0x0020, 0x0200, 0x2000);
}

/*
 * @routine 09:A371 ObjectBrightGateStep
 * WHAT IT DOES: The bright gate's frame: a car's hit, the first time (+$25
 * bit 0), brightens the screen (flash step $19C5 = $20, fade by $200 up to
 * $2000, $0DA5 bits 0 and 5); no hit clears the bit.
 * HOW: ObjectSyncFromRecord, RecordContactSummary; the flash word by a long
 * store. Reached only by unit_gates_09.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
 * it.
 */
void abi_ObjectBrightGateStep(void)
{
    flash_frame(0x0020, 0x0200, 0x2000);
}

/*
 * @routine 09:A3C3 ObjectDarkGate
 * WHAT IT DOES: Sets up a dark gate: restart $09:A3E1
 * (ObjectDarkGateRecord), frame $09:A408 (ObjectDarkGateStep); its record
 * and frame.
 * HOW: Word and byte stores.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void abi_ObjectDarkGate(void)
{
    handlers2(0xA3E1, 0xA408);
    gate42d(0x0780);
    flash_frame(0xFFE0, 0xFE00, 0xFE00);
}

/*
 * @routine 09:A3E1 ObjectDarkGateRecord
 * WHAT IT DOES: The dark gate's restart: its record ($42D, flag bit 4, at
 * ($780, $780, $780)), then its frame.
 * HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition.
 * Entry: as ObjectDarkGate. Exit: as ObjectDarkGate.
 */
void abi_ObjectDarkGateRecord(void)
{
    gate42d(0x0780);
    flash_frame(0xFFE0, 0xFE00, 0xFE00);
}

/*
 * @routine 09:A408 ObjectDarkGateStep
 * WHAT IT DOES: The dark gate's frame: a car's hit, the first time (+$25
 * bit 0), darkens the screen (flash step $19C5 = -$20, fade by -$200 down
 * to $FE00, $0DA5 bits 0 and 5); no hit clears the bit.
 * HOW: ObjectSyncFromRecord, RecordContactSummary; the flash word by a long
 * store.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
 * it.
 */
void abi_ObjectDarkGateStep(void)
{
    flash_frame(0xFFE0, 0xFE00, 0xFE00);
}

/* 09:A49D / A52C: a car flag gate's frame: a car's contact (bit 0), the
   first time (+$25 bit 0), sets or clears bit 1 of the car's record +$8A
   (unless $7F:3460 is set) and plays `snd` on port 1; no contact clears
   the bit. `on` picks set (08:B72E) or clear (08:B743). */
static void car_flag_frame(int on, uint8_t snd)
{
    if (!synced() || !next_contact()) {
        and8(F(0x2025, cpu.x), 0xFE);
        return;
    }
    if (bit25(0x01))
        return;
    or8(F(0x2025, cpu.x), 0x01);
    cpu.p |= P_M;
    if (!ld8(0x7F3460))
        car_bits16(0x0002, on ? abi_GsuRecordFlags8ASet : abi_GsuRecordFlags8AClear);
    if (!on)
        cpu.p &= (uint8_t)~P_M;
    cpu.p |= P_M;
    sound1(snd);
}

/*
 * @routine 09:A458 ObjectCarFlagOffGate
 * WHAT IT DOES: Sets up a gate that takes bit 1 of a car's record +$8A
 * away: restart $09:A476 (ObjectCarFlagOffGateRecord), frame $09:A49D
 * (ObjectCarFlagOffGateStep); its record and frame.
 * HOW: Word and byte stores.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void abi_ObjectCarFlagOffGate(void)
{
    handlers2(0xA476, 0xA49D);
    gate42d(0x0780);
    car_flag_frame(0, 0x11);
}

/*
 * @routine 09:A476 ObjectCarFlagOffGateRecord
 * WHAT IT DOES: The gate's restart: its record ($42D, flag bit 4, at
 * ($780, $780, $780)), then its frame.
 * HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition.
 * Entry: as ObjectCarFlagOffGate. Exit: as ObjectCarFlagOffGate.
 */
void abi_ObjectCarFlagOffGateRecord(void)
{
    gate42d(0x0780);
    car_flag_frame(0, 0x11);
}

/*
 * @routine 09:A49D ObjectCarFlagOffGateStep
 * WHAT IT DOES: The gate's frame: the first car contact (contact flags bit
 * 0; +$25 bit 0 marks it) clears bit 1 of that car's record +$8A unless
 * $7F:3460 is set, and plays sound $11 on port 1; no contact clears the
 * mark.
 * HOW: ObjectSyncFromRecord, then RecordNextContact over the contacts;
 * 08:B743 with A = 2 (16-bit) and Y = the touched record.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
 * it.
 */
void abi_ObjectCarFlagOffGateStep(void)
{
    car_flag_frame(0, 0x11);
}

/*
 * @routine 09:A4E7 ObjectCarFlagOnGate
 * WHAT IT DOES: Sets up a gate that gives a car's record +$8A bit 1:
 * restart $09:A505 (ObjectCarFlagOnGateRecord), frame $09:A52C
 * (ObjectCarFlagOnGateStep); its record and frame.
 * HOW: Word and byte stores.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void abi_ObjectCarFlagOnGate(void)
{
    handlers2(0xA505, 0xA52C);
    gate42d(0x0780);
    car_flag_frame(1, 0x10);
}

/*
 * @routine 09:A505 ObjectCarFlagOnGateRecord
 * WHAT IT DOES: The gate's restart: its record ($42D, flag bit 4, at
 * ($780, $780, $780)), then its frame.
 * HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition.
 * Entry: as ObjectCarFlagOnGate. Exit: as ObjectCarFlagOnGate.
 */
void abi_ObjectCarFlagOnGateRecord(void)
{
    gate42d(0x0780);
    car_flag_frame(1, 0x10);
}

/*
 * @routine 09:A52C ObjectCarFlagOnGateStep
 * WHAT IT DOES: The gate's frame: the first car contact (contact flags bit
 * 0; +$25 bit 0 marks it) sets bit 1 of that car's record +$8A unless
 * $7F:3460 is set, and plays sound $10 on port 1; no contact clears the
 * mark.
 * HOW: ObjectSyncFromRecord, then RecordNextContact over the contacts;
 * 08:B72E with A = 2 (16-bit) and Y = the touched record.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
 * it.
 */
void abi_ObjectCarFlagOnGateStep(void)
{
    car_flag_frame(1, 0x10);
}

/* 09:A5B9: the lap 1 slide gate's frame: on lap 1 the first car contact
   (+$25 bit 0) clears bit 1 of the car's record +$8A and starts the slide
   down ($0DA5 bits 5 and 7: step $40 to -$40, direction $8000) and a fade
   ($1A00 = $20 up to $2000). */
static void slide1_frame(void)
{
    if (!synced() || !next_contact())
        return;
    if (!lap_is(0x01))
        return;
    if (bit25(0x01))
        return;
    or8(F(0x2025, cpu.x), 0x01);
    cpu.p |= P_M;
    or8(0x000DA5, 0xA0);
    car_bits16(0x0002, abi_GsuRecordFlags8AClear);
    cpu.p &= (uint8_t)~P_M;
    fade_set(0x0020, 0x2000);
    slide_set(0x0040, 0xFFC0, 0x8000);
    cpu.p |= P_M;
}

/*
 * @routine 09:A574 ObjectSlideGate
 * WHAT IT DOES: Sets up the lap 1 slide gate: restart $09:A592
 * (ObjectSlideGateRecord), frame $09:A5B9 (ObjectSlideGateStep); its record
 * and frame.
 * HOW: Word and byte stores.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void abi_ObjectSlideGate(void)
{
    handlers2(0xA592, 0xA5B9);
    gate42d(0x0780);
    slide1_frame();
}

/*
 * @routine 09:A592 ObjectSlideGateRecord
 * WHAT IT DOES: The slide gate's restart: its record ($42D, flag bit 4, at
 * ($780, $780, $780)), then its frame.
 * HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition.
 * Entry: as ObjectSlideGate. Exit: as ObjectSlideGate.
 */
void abi_ObjectSlideGateRecord(void)
{
    gate42d(0x0780);
    slide1_frame();
}

/*
 * @routine 09:A5B9 ObjectSlideGateStep
 * WHAT IT DOES: The slide gate's frame: on lap 1 ($0DEA) the first car
 * contact (+$25 bit 0 marks it) clears bit 1 of the car's record +$8A,
 * starts the slide down ($0DA5 bits 5 and 7; step $40 to target -$40,
 * direction $8000) and a fade (step $20 up to $2000).
 * HOW: ObjectSyncFromRecord, RecordNextContact; 08:B743 with A = 2;
 * $0DA5 by long accesses.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
 * it.
 */
void abi_ObjectSlideGateStep(void)
{
    slide1_frame();
}

/*
 * @routine 09:A6BE GateFlashOnClock
 * WHAT IT DOES: When bit 4 of the race clock's fraction $0E63 is clear,
 * sweeps the screen ($0DA5 bit 4) with sound $36.
 * HOW: Four LSRs and AND #1 (the carry is the fraction's bit 3); $0DA5 by
 * long accesses. Reached only by unit_gates_09.
 * Entry: X 16-bit. Exit: M 8-bit; carry = bit 3 of $0E63; bit 4 set: A
 * low byte = 1; else as SoundPort0QueuePush leaves it.
 */
void abi_GateFlashOnClock(void)
{
    uint8_t v;

    cpu.p |= P_M;
    v = ld8(db() | 0x0E63);
    cpu.p = (uint8_t)((cpu.p & ~P_C) | ((v & 0x08) ? P_C : 0));
    lda8((uint8_t)((v >> 4) & 0x01));
    if ((uint8_t)cpu.a)
        return;
    or8(0x000DA5, 0x10);
    sound0(0x36);
}

/*
 * @routine 09:A6DC GateFadeUp
 * WHAT IT DOES: Starts a fade ($0DA5 bit 5) by $80 up to $2000.
 * HOW: $0DA5 by long accesses; the words through the data bank.
 * Reached only by unit_gates_09.
 * Entry: X 16-bit. Exit: M 8-bit; A = $2000; N, Z from it.
 */
void abi_GateFadeUp(void)
{
    cpu.p |= P_M;
    or8(0x000DA5, 0x20);
    cpu.p &= (uint8_t)~P_M;
    fade_set(0x0080, 0x2000);
    cpu.p |= P_M;
}

/* 09:A666: the lap gate's frame: a car's hit on lap 0 or 1, the first time
   (+$25 bit 0), GateFlashOnClock; on lap 2, the first time (+$25 bit 1),
   GateFadeUp and the gate is released. */
static void lap_flash_frame(void)
{
    uint8_t lap;

    if (!hit())
        return;
    cpu.p |= P_M;
    lap = ld8(db() | 0x0DEA);
    cmp8(0x00);
    if (lap != 0x00) {
        cmp8(0x01);
        if (lap != 0x01) {
            cmp8(0x02);
            if (lap != 0x02)
                return;
            if (bit25(0x02))
                return;
            or8(F(0x2025, cpu.x), 0x02);
            abi_GateFadeUp();
            abi_ObjectRelease();
            return;
        }
    }
    if (bit25(0x01))
        return;
    or8(F(0x2025, cpu.x), 0x01);
    abi_GateFlashOnClock();
}

/*
 * @routine 09:A621 ObjectLapFlashGate
 * WHAT IT DOES: Sets up a gate that flashes on laps 0 and 1 and fades on
 * lap 2: restart $09:A63F (ObjectLapFlashGateRecord), frame $09:A666
 * (ObjectLapFlashGateStep); its record and frame.
 * HOW: Word and byte stores.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void abi_ObjectLapFlashGate(void)
{
    handlers2(0xA63F, 0xA666);
    gate42d(0x0780);
    lap_flash_frame();
}

/*
 * @routine 09:A63F ObjectLapFlashGateRecord
 * WHAT IT DOES: The gate's restart: its record ($42D, flag bit 4, at
 * ($780, $780, $780)), then its frame.
 * HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition.
 * Entry: as ObjectLapFlashGate. Exit: as ObjectLapFlashGate.
 */
void abi_ObjectLapFlashGateRecord(void)
{
    gate42d(0x0780);
    lap_flash_frame();
}

/*
 * @routine 09:A666 ObjectLapFlashGateStep
 * WHAT IT DOES: The gate's frame: a car's hit on lap 0 or 1 ($0DEA), the
 * first time (+$25 bit 0), sweeps the screen on even clock ticks
 * (GateFlashOnClock); on lap 2, the first time (+$25 bit 1), a fade
 * (GateFadeUp) and the gate is released.
 * HOW: ObjectSyncFromRecord, RecordContactSummary; the lap compared with
 * 0, 1 and 2.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
 * it.
 */
void abi_ObjectLapFlashGateStep(void)
{
    lap_flash_frame();
}

/* 09:A73E: a car's hit on lap 0, the first time (+$25 bit 0): a sweep
   ($0DA5 bit 4) with sound $36, and the gate is released; no hit clears
   the bit. */
static void sweep0_frame(void)
{
    if (!hit()) {
        and8(F(0x2025, cpu.x), 0xFE);
        return;
    }
    if (!lap_is(0x00))
        return;
    if (bit25(0x01))
        return;
    or8(F(0x2025, cpu.x), 0x01);
    or8(0x000DA5, 0x10);
    sound0(0x36);
    abi_ObjectRelease();
}

/*
 * @routine 09:A6F9 ObjectSweepGate
 * WHAT IT DOES: Sets up the lap 0 sweep gate: restart $09:A717
 * (ObjectSweepGateRecord), frame $09:A73E (ObjectSweepGateStep); its record
 * and frame.
 * HOW: Word and byte stores. Reached only by unit_gates_09.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void abi_ObjectSweepGate(void)
{
    handlers2(0xA717, 0xA73E);
    gate42d(0x0780);
    sweep0_frame();
}

/*
 * @routine 09:A717 ObjectSweepGateRecord
 * WHAT IT DOES: The sweep gate's restart: its record ($42D, flag bit 4, at
 * ($780, $780, $780)), then its frame.
 * HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition. Reached only
 * by unit_gates_09.
 * Entry: as ObjectSweepGate. Exit: as ObjectSweepGate.
 */
void abi_ObjectSweepGateRecord(void)
{
    gate42d(0x0780);
    sweep0_frame();
}

/*
 * @routine 09:A73E ObjectSweepGateStep
 * WHAT IT DOES: The sweep gate's frame: a car's hit on lap 0, the first
 * time (+$25 bit 0), sweeps the screen ($0DA5 bit 4) with sound $36 and
 * releases the gate; no hit clears the bit.
 * HOW: ObjectSyncFromRecord, RecordContactSummary; $0DA5 by long accesses.
 * Reached only by unit_gates_09.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
 * it.
 */
void abi_ObjectSweepGateStep(void)
{
    sweep0_frame();
}

/* 09:A7CD: the slide chime gate's frame: synced and summed (the summary's
   bit 0 is a hit): a hit, the first time (+$25 bit 1, clearing bit 2),
   sound $13 on port 1; then on lap 1 the first time (+$25 bit 0) the slide
   down ($0DA5 bits 5 and 7, step $80 to -$80, direction $8000, $1A04 =
   $FF) and a fade ($20 up to $2000). No hit: the first time (+$25 bit 2)
   sound $0E; +$25 bit 1 cleared. */
static void slide_chime_frame(void)
{
    abi_ObjectSyncFromRecord();
    abi_RecordContactSummary();
    lda8((uint8_t)(cpu.a & 0x01));
    if ((uint8_t)cpu.a) {
        if (!bit25(0x02)) {
            or8(F(0x2025, cpu.x), 0x02);
            and8(F(0x2025, cpu.x), 0xFB);
            sound1(0x13);
        }
        if (!lap_is(0x01))
            return;
        if (bit25(0x01))
            return;
        or8(F(0x2025, cpu.x), 0x01);
        cpu.p |= P_M;
        or8(0x000DA5, 0xA0);
        cpu.p &= (uint8_t)~P_M;
        slide_set(0x0080, 0xFF80, 0x8000);
        slide_hold();
        cpu.p &= (uint8_t)~P_M;
        fade_set(0x0020, 0x2000);
        cpu.p |= P_M;
        return;
    }
    cpu.p |= P_M;
    if (!bit25(0x04)) {
        or8(F(0x2025, cpu.x), 0x04);
        sound1(0x0E);
    }
    and8(F(0x2025, cpu.x), 0xFD);
}

/*
 * @routine 09:A788 ObjectSlideChimeGate
 * WHAT IT DOES: Sets up the slide chime gate: restart $09:A7A6
 * (ObjectSlideChimeGateRecord), frame $09:A7CD (ObjectSlideChimeGateStep);
 * its record and frame.
 * HOW: Word and byte stores.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void abi_ObjectSlideChimeGate(void)
{
    handlers2(0xA7A6, 0xA7CD);
    gate42d(0x0F00);
    slide_chime_frame();
}

/*
 * @routine 09:A7A6 ObjectSlideChimeGateRecord
 * WHAT IT DOES: The gate's restart: its record ($42D, flag bit 4, at
 * ($F00, $F00, $F00)), then its frame.
 * HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition.
 * Entry: as ObjectSlideChimeGate. Exit: as ObjectSlideChimeGate.
 */
void abi_ObjectSlideChimeGateRecord(void)
{
    gate42d(0x0F00);
    slide_chime_frame();
}

/*
 * @routine 09:A7CD ObjectSlideChimeGateStep
 * WHAT IT DOES: The gate's frame: a car entering (contact summary bit 0)
 * the first time plays sound $13 on port 1 (+$25 bit 1 marks inside); on
 * lap 1 the first entry (+$25 bit 0) also starts the slide down (step $80
 * to -$80, $1A04 = $FF) and a fade ($20 up to $2000). Leaving plays sound
 * $0E the first time (+$25 bit 2 marks outside).
 * HOW: ObjectSyncFromRecord, RecordContactSummary (not tested on the
 * sync); $0DA5 by long accesses.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
 * it.
 */
void abi_ObjectSlideChimeGateStep(void)
{
    slide_chime_frame();
}

/* 09:A8BC / A94C: on lap `lap` the first car contact (+$25 bit 0) starts
   a fade ($0DA5 bit 5, $1A00 = step, $1A02 = end). */
static void fade_frame(uint8_t lap, uint16_t step, uint16_t end)
{
    if (!synced() || !next_contact())
        return;
    if (!lap_is(lap))
        return;
    if (bit25(0x01))
        return;
    or8(F(0x2025, cpu.x), 0x01);
    cpu.p |= P_M;
    or8(0x000DA5, 0x20);
    cpu.p &= (uint8_t)~P_M;
    fade_set(step, end);
    cpu.p |= P_M;
}

/*
 * @routine 09:A877 ObjectFadeGate0
 * WHAT IT DOES: Sets up the lap 0 fade gate: restart $09:A895
 * (ObjectFadeGate0Record), frame $09:A8BC (ObjectFadeGate0Step); its record
 * and frame.
 * HOW: Word and byte stores. Reached only by unit_gates_09.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void abi_ObjectFadeGate0(void)
{
    handlers2(0xA895, 0xA8BC);
    gate42d(0x0780);
    fade_frame(0x00, 0x000A, 0x1FFE);
}

/*
 * @routine 09:A895 ObjectFadeGate0Record
 * WHAT IT DOES: The gate's restart: its record ($42D, flag bit 4, at
 * ($780, $780, $780)), then its frame.
 * HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition. Reached only
 * by unit_gates_09.
 * Entry: as ObjectFadeGate0. Exit: as ObjectFadeGate0.
 */
void abi_ObjectFadeGate0Record(void)
{
    gate42d(0x0780);
    fade_frame(0x00, 0x000A, 0x1FFE);
}

/*
 * @routine 09:A8BC ObjectFadeGate0Step
 * WHAT IT DOES: The gate's frame: on lap 0 the first car contact (+$25
 * bit 0 marks it) starts a slow fade ($0DA5 bit 5, step $0A up to $1FFE).
 * HOW: ObjectSyncFromRecord, RecordNextContact; $0DA5 by long accesses.
 * Reached only by unit_gates_09.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
 * it.
 */
void abi_ObjectFadeGate0Step(void)
{
    fade_frame(0x00, 0x000A, 0x1FFE);
}

/*
 * @routine 09:A907 ObjectFadeGate2
 * WHAT IT DOES: Sets up the lap 2 fade gate: restart $09:A925
 * (ObjectFadeGate2Record), frame $09:A94C (ObjectFadeGate2Step); its record
 * and frame.
 * HOW: Word and byte stores. Reached only by unit_gates_09.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void abi_ObjectFadeGate2(void)
{
    handlers2(0xA925, 0xA94C);
    gate42d(0x0780);
    fade_frame(0x02, 0x0020, 0x1F20);
}

/*
 * @routine 09:A925 ObjectFadeGate2Record
 * WHAT IT DOES: The gate's restart: its record ($42D, flag bit 4, at
 * ($780, $780, $780)), then its frame.
 * HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition. Reached only
 * by unit_gates_09.
 * Entry: as ObjectFadeGate2. Exit: as ObjectFadeGate2.
 */
void abi_ObjectFadeGate2Record(void)
{
    gate42d(0x0780);
    fade_frame(0x02, 0x0020, 0x1F20);
}

/*
 * @routine 09:A94C ObjectFadeGate2Step
 * WHAT IT DOES: The gate's frame: on lap 2 the first car contact (+$25
 * bit 0 marks it) starts a fade ($0DA5 bit 5, step $20 up to $1F20).
 * HOW: ObjectSyncFromRecord, RecordNextContact; $0DA5 by long accesses;
 * its no-contact exit is the RTL of 09:A8BC's code (09:A906). Reached only
 * by unit_gates_09.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
 * it.
 */
void abi_ObjectFadeGate2Step(void)
{
    fade_frame(0x02, 0x0020, 0x1F20);
}

/* 09:A9DC: while a car is in the zone, the slide runs up (step $100 to
   $1000, direction 0, $7F:3470 = $7FFF, $0DA5 bit 7); otherwise it runs
   back down (to -$100, direction $8000, $1A04 = $FF). */
static void slide_zone_frame(void)
{
    if (hit()) {
        cpu.p &= (uint8_t)~P_M;
        st16w(0x7F3470, 0x7FFF);
        slide_set(0x0100, 0x1000, 0x0000);
        cpu.p |= P_M;
        or8(db() | 0x0DA5, 0x80);
        return;
    }
    cpu.p &= (uint8_t)~P_M;
    slide_set(0x0100, 0xFF00, 0x8000);
    slide_hold();
}

/*
 * @routine 09:A997 ObjectSlideZone
 * WHAT IT DOES: Sets up a slide zone: restart $09:A9B5
 * (ObjectSlideZoneRecord), frame $09:A9DC (ObjectSlideZoneStep); its record
 * and frame.
 * HOW: Word and byte stores. Reached only by unit_gates_09.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void abi_ObjectSlideZone(void)
{
    handlers2(0xA9B5, 0xA9DC);
    gate42d(0x0F00);
    slide_zone_frame();
}

/*
 * @routine 09:A9B5 ObjectSlideZoneRecord
 * WHAT IT DOES: The zone's restart: its record ($42D, flag bit 4, at
 * ($F00, $F00, $F00)), then its frame.
 * HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition. Reached only
 * by unit_gates_09.
 * Entry: as ObjectSlideZone. Exit: as ObjectSlideZone.
 */
void abi_ObjectSlideZoneRecord(void)
{
    gate42d(0x0F00);
    slide_zone_frame();
}

/*
 * @routine 09:A9DC ObjectSlideZoneStep
 * WHAT IT DOES: The zone's frame: with a car's hit the slide runs up (step
 * $100 to $1000, direction 0, done mark $7F:3470 = $7FFF, $0DA5 bit 7);
 * without, back down (step $100 to -$100, direction $8000, $1A04 = $FF).
 * HOW: ObjectSyncFromRecord, RecordContactSummary; word stores.
 * Reached only by unit_gates_09.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
 * it.
 */
void abi_ObjectSlideZoneStep(void)
{
    slide_zone_frame();
}

/* 09:AA78: a car's contact on lap 0 counts $1A06 up, sets bit 0 of the
   car's record +$8A (08:B72E), sweeps the screen with sound $36 unless
   bits 4-5 of the clock fraction $0E63 are clear, and releases the
   marker. */
static void bonus_marker_frame(void)
{
    uint8_t v;

    if (!synced() || !next_contact())
        return;
    if (!lap_is(0x00))
        return;
    inc8(db() | 0x1A06);
    lda8(0x01);
    abi_GsuRecordFlags8ASet();
    cpu.p |= P_M;
    v = ld8(db() | 0x0E63);
    cpu.p = (uint8_t)((cpu.p & ~P_C) | ((v & 0x08) ? P_C : 0));
    lda8((uint8_t)((v >> 4) & 0x03));
    if ((uint8_t)cpu.a) {
        or8(db() | 0x0DA5, 0x10);
        sound0(0x36);
    }
    abi_ObjectRelease();
}

/*
 * @routine 09:AA33 ObjectBonusMarker
 * WHAT IT DOES: Sets up a bonus marker: restart $09:AA51
 * (ObjectBonusMarkerRecord), frame $09:AA78 (ObjectBonusMarkerStep); its
 * record and frame.
 * HOW: Word and byte stores. Reached only by unit_gates_09.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void abi_ObjectBonusMarker(void)
{
    handlers2(0xAA51, 0xAA78);
    gate42d(0x0780);
    bonus_marker_frame();
}

/*
 * @routine 09:AA51 ObjectBonusMarkerRecord
 * WHAT IT DOES: The marker's restart: its record ($42D, flag bit 4, at
 * ($780, $780, $780)), then its frame.
 * HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition. Reached only
 * by unit_gates_09.
 * Entry: as ObjectBonusMarker. Exit: as ObjectBonusMarker.
 */
void abi_ObjectBonusMarkerRecord(void)
{
    gate42d(0x0780);
    bonus_marker_frame();
}

/*
 * @routine 09:AA78 ObjectBonusMarkerStep
 * WHAT IT DOES: The marker's frame: a car's contact on lap 0 counts the
 * markers at $1A06 up, sets bit 0 of the car's record +$8A, sweeps the
 * screen ($0DA5 bit 4) with sound $36 unless bits 4-5 of the clock
 * fraction $0E63 are clear, and releases the marker.
 * HOW: ObjectSyncFromRecord, RecordNextContact; 08:B72E with A low byte 1.
 * Reached only by unit_gates_09.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
 * it.
 */
void abi_ObjectBonusMarkerStep(void)
{
    bonus_marker_frame();
}

/* 09:AB01: a car's contact sets the window slide's bit 2 ($0DA5, bit 3
   cleared); the first time (+$25 bit 0) sound $10 on port 1 and, on lap 0
   with markers taken ($1A06), bit 0 of the car's record +$8A set. No
   contact clears the mark. */
static void bonus_gate_frame(void)
{
    uint8_t v;

    if (!synced() || !next_contact()) {
        and8(F(0x2025, cpu.x), 0xFE);
        return;
    }
    cpu.p |= P_M;
    v = (uint8_t)(ld8(db() | 0x0DA5) & 0xF7);
    lda8((uint8_t)(v | 0x04));
    wr8(db() | 0x0DA5, (uint8_t)(v | 0x04));
    if (bit25(0x01))
        return;
    or8(F(0x2025, cpu.x), 0x01);
    sound1(0x10);
    if (!lap_is(0x00))
        return;
    if (!ld8(db() | 0x1A06))
        return;
    lda8(0x01);
    abi_GsuRecordFlags8ASet();
}

/*
 * @routine 09:AABC ObjectBonusGate
 * WHAT IT DOES: Sets up the bonus gate: restart $09:AADA
 * (ObjectBonusGateRecord), frame $09:AB01 (ObjectBonusGateStep); its record
 * and frame.
 * HOW: Word and byte stores. Reached only by unit_gates_09.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void abi_ObjectBonusGate(void)
{
    handlers2(0xAADA, 0xAB01);
    gate42d(0x0780);
    bonus_gate_frame();
}

/*
 * @routine 09:AADA ObjectBonusGateRecord
 * WHAT IT DOES: The gate's restart: its record ($42D, flag bit 4, at
 * ($780, $780, $780)), then its frame.
 * HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition. Reached only
 * by unit_gates_09.
 * Entry: as ObjectBonusGate. Exit: as ObjectBonusGate.
 */
void abi_ObjectBonusGateRecord(void)
{
    gate42d(0x0780);
    bonus_gate_frame();
}

/*
 * @routine 09:AB01 ObjectBonusGateStep
 * WHAT IT DOES: The gate's frame: a car's contact sets bit 2 of $0DA5
 * (clearing bit 3); the first contact (+$25 bit 0 marks it) plays sound
 * $10 on port 1 and, on lap 0 with markers taken ($1A06), sets bit 0 of
 * the car's record +$8A. No contact clears the mark.
 * HOW: ObjectSyncFromRecord, RecordNextContact; 08:B72E with A low byte 1.
 * Reached only by unit_gates_09.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
 * it.
 */
void abi_ObjectBonusGateStep(void)
{
    bonus_gate_frame();
}

/* 09:AB9B: a car's contact sets bits 1 and 3 of $0DA5; the first time
   (+$25 bit 0) sound $0F on port 1 and, unless on lap 2, with markers
   taken ($1A06), bit 0 of the car's record +$8A cleared. No contact clears
   the mark. */
static void bonus_end_frame(void)
{
    if (!synced() || !next_contact()) {
        and8(F(0x2025, cpu.x), 0xFE);
        return;
    }
    cpu.p |= P_M;
    or8(db() | 0x0DA5, 0x0A);
    if (bit25(0x01))
        return;
    or8(F(0x2025, cpu.x), 0x01);
    sound1(0x0F);
    if (lap_is(0x02))
        return;
    if (!ld8(db() | 0x1A06))
        return;
    car_bits16(0x0001, abi_GsuRecordFlags8AClear);
    cpu.p |= P_M;
}

/*
 * @routine 09:AB56 ObjectBonusEndGate
 * WHAT IT DOES: Sets up the bonus end gate: restart $09:AB74
 * (ObjectBonusEndGateRecord), frame $09:AB9B (ObjectBonusEndGateStep); its
 * record and frame.
 * HOW: Word and byte stores. Reached only by unit_gates_09.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void abi_ObjectBonusEndGate(void)
{
    handlers2(0xAB74, 0xAB9B);
    gate42d(0x0780);
    bonus_end_frame();
}

/*
 * @routine 09:AB74 ObjectBonusEndGateRecord
 * WHAT IT DOES: The gate's restart: its record ($42D, flag bit 4, at
 * ($780, $780, $780)), then its frame.
 * HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition. Reached only
 * by unit_gates_09.
 * Entry: as ObjectBonusEndGate. Exit: as ObjectBonusEndGate.
 */
void abi_ObjectBonusEndGateRecord(void)
{
    gate42d(0x0780);
    bonus_end_frame();
}

/*
 * @routine 09:AB9B ObjectBonusEndGateStep
 * WHAT IT DOES: The gate's frame: a car's contact sets bits 1 and 3 of
 * $0DA5; the first contact (+$25 bit 0 marks it) plays sound $0F on port 1
 * and, unless on lap 2, with markers taken ($1A06), clears bit 0 of the
 * car's record +$8A. No contact clears the mark.
 * HOW: ObjectSyncFromRecord, RecordNextContact; 08:B743 with A = 1.
 * Reached only by unit_gates_09.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
 * it.
 */
void abi_ObjectBonusEndGateStep(void)
{
    bonus_end_frame();
}

/* REP #$20, LDA $0D2B (the HUD mode), and with a mode (two players) SEP
   #$20 and ObjectRelease: 1 when released. */
static int released_by_hud(void)
{
    cpu.p &= (uint8_t)~P_M;
    op_lda16(db() | 0x0D2B);
    if (!cpu.a)
        return 0;
    cpu.p |= P_M;
    abi_ObjectRelease();
    return 1;
}

/*
 * @routine 09:ABF3 ObjectPopMarker
 * WHAT IT DOES: In a one-player race (HUD mode $0D2B = 0) puts a pop marker
 * (kind $B7FC) at object X's position $1E0 higher with its scale +$15,
 * handler $09:AD50 (ObjectPopMarkerStep), and clears X's handler; with a
 * HUD mode X is released.
 * HOW: ObjectSpawn, ObjectCopyPositionTo; starts with a NOP, as the course
 * data's pointers expect.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: M 8-bit; released:
 * as ObjectRelease leaves it; else A low byte = 0, Z set.
 */
void abi_ObjectPopMarker(void)
{
    if (released_by_hud())
        return;
    cpu.p |= P_M;
    if (spawn(0xB7FC)) {
        abi_ObjectCopyPositionTo();
        ld8(F(0x2015, cpu.x));
        wr8(F(0x2015, cpu.y), (uint8_t)cpu.a);
        add16(F(0x200E, cpu.y), 0xFE20);
        handler(cpu.y, 0xAD50);
    }
    handler_none(cpu.x);
}

/* 09:AD07: a car's hit on a lap other than the one in +$48 takes that lap
   in +$48 (through $1F58), plays sound $41 and adds $19E6 to the race
   time $0E70 in BCD. */
static void time_gate_frame(void)
{
    uint8_t v, r;

    if (!hit())
        return;
    v = ld8(F(0x2048, cpu.x));
    wr8(db() | 0x1F58, v);
    cpu.p |= P_M;
    v = ld8(db() | 0x0DEA);
    r = rd8(db() | 0x1F58);
    cmp8(r);
    if (v == r)
        return;
    cpu.p |= P_M;
    v = ld8(db() | 0x0DEA);
    wr8(db() | 0x1F58, v);
    v = ld8(db() | 0x1F58);
    wr8(F(0x2048, cpu.x), v);
    sound0(0x41);
    cpu.p |= P_D;
    v = ld8(db() | 0x0E70);
    cpu.p &= (uint8_t)~P_C;
    r = rd8(db() | 0x19E6);
    v = op_adc8(v, r, &cpu.p);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    wr8(db() | 0x0E70, v);
    v = ld8(db() | 0x0E71);
    v = op_adc8(v, 0x00, &cpu.p);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    wr8(db() | 0x0E71, v);
    cpu.p &= (uint8_t)~P_D;
}

/* A time gate's set-up in a one-player race: restart r, frame $09:AD07
   (ObjectTimeGateStep), +$48 = 4 (no lap taken yet). */
static void time_gate(uint16_t r)
{
    if (released_by_hud())
        return;
    cpu.p |= P_M;
    handlers2(r, 0xAD07);
    st8i(F(0x2048, cpu.x), 0x04);
}

/*
 * @routine 09:AC4F ObjectTimeGate
 * WHAT IT DOES: Sets up a time gate in a one-player race: restart $09:AC83
 * (ObjectTimeGateRecord), frame $09:AD07 (ObjectTimeGateStep), +$48 = 4;
 * with a HUD mode ($0D2B) it is released.
 * HOW: Word and byte stores.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: M 8-bit; A low
 * byte = 4, or as ObjectRelease leaves it.
 */
void abi_ObjectTimeGate(void)
{
    time_gate(0xAC83);
}

/*
 * @routine 09:AC83 ObjectTimeGateRecord
 * WHAT IT DOES: The time gate's restart: its record ($42D, flag bit 4, at
 * ($780, $780, $780)).
 * HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as
 * GsuRecordPosition leaves it, M 8-bit.
 */
void abi_ObjectTimeGateRecord(void)
{
    gate42d(0x0780);
}

/*
 * @routine 09:ACAB ObjectTimeGateSmall
 * WHAT IT DOES: Sets up a small time gate in a one-player race: restart
 * $09:ACDF (ObjectTimeGateSmallRecord), frame $09:AD07
 * (ObjectTimeGateStep), +$48 = 4; with a HUD mode ($0D2B) it is released.
 * HOW: Word and byte stores. Reached only by unit_gates_09.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: M 8-bit; A low
 * byte = 4, or as ObjectRelease leaves it.
 */
void abi_ObjectTimeGateSmall(void)
{
    time_gate(0xACDF);
}

/*
 * @routine 09:ACDF ObjectTimeGateSmallRecord
 * WHAT IT DOES: The small time gate's restart: its record ($42D, flag bit
 * 4, at ($3C0, $3C0, $3C0)).
 * HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition. Reached only
 * by unit_gates_09.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as
 * GsuRecordPosition leaves it, M 8-bit.
 */
void abi_ObjectTimeGateSmallRecord(void)
{
    gate42d(0x03C0);
}

/*
 * @routine 09:AD07 ObjectTimeGateStep
 * WHAT IT DOES: The time gates' frame: a car's hit on a lap ($0DEA) other
 * than the one it last counted (+$48) counts this one, plays sound $41 and
 * adds the bonus $19E6 to the race time $0E70-$0E71 (BCD).
 * HOW: ObjectSyncFromRecord, RecordContactSummary; the laps compared
 * through $1F58; SED, ADC, CLD.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
 * it; D flag clear.
 */
void abi_ObjectTimeGateStep(void)
{
    time_gate_frame();
}

/*
 * @routine 09:AD50 ObjectPopMarkerStep
 * WHAT IT DOES: The pop marker's frame: its size +$14 shrinks by 8.
 * HOW: ADC #$F8; ObjectRebucket.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectRebucket leaves it.
 */
void abi_ObjectPopMarkerStep(void)
{
    add8(F(0x2014, cpu.x), 0xF8);
    abi_ObjectRebucket();
}

/* +$0A against the lap $0DEA (through $1F58): 1 when equal. */
static int lap_matches_0a(void)
{
    uint8_t v, r;

    v = ld8(F(0x200A, cpu.x));
    wr8(db() | 0x1F58, v);
    cpu.p |= P_M;
    v = ld8(db() | 0x0DEA);
    r = rd8(db() | 0x1F58);
    cmp8(r);
    return v == r;
}

/* +$0A = the lap $0DEA (through $1F58), +$04 = $A040 (the lit model). */
static void lap_take_0a(void)
{
    uint8_t v;

    v = ld8(db() | 0x0DEA);
    wr8(db() | 0x1F58, v);
    v = ld8(db() | 0x1F58);
    wr8(F(0x200A, cpu.x), v);
    st16i(F(0x2004, cpu.x), 0xA040);
}

/* PHX, TYX: a spark of `kind` at the player $0346's position following
   it (+$29 = the player, +$27 = 0, +$0A = $0A, handler $09:AE60
   ObjectSparkFollow); PLX. */
static void spark_follow(uint16_t kind)
{
    uint16_t sx = cpu.x;

    cpu.x = cpu.y;
    cpu.p = nz16(cpu.p, cpu.x);
    if (spawn(kind)) {
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
    cpu.x = sx;
    cpu.p = nz16(cpu.p, sx);
}

/* A lap ball's record ($42D, flag bit 4, +$2A high $14); its model +$04
   lit ($A040) when +$0A is this lap, else `kind`. */
static void lap_ball_record(uint16_t kind)
{
    abi_GsuRecordNew42D();
    abi_GsuRecordFlag10();
    lda8(0x14);
    abi_GsuRecordSet2AHigh();
    if (lap_matches_0a())
        st16i(F(0x2004, cpu.x), 0xA040);
    else
        st16i(F(0x2004, cpu.x), kind);
}

/* 09:ADB6: the up ball's frame: it grows (+$15, +$14 by $10); on a car's
   contact with flags 1 or 2 (AND #$0B) on a lap it has not lit yet (+$0A)
   it takes the lap, lights up, raises that car's record +$6A by $40,
   plays sound $25 and leaves a following spark ($B7A8). */
static void ball_up_frame(void)
{
    add8(F(0x2015, cpu.x), 0x10);
    add8(F(0x2014, cpu.x), 0x10);
    if (!synced())
        return;
    for (;;) {
        abi_RecordNextContact();
        if (!(cpu.p & P_C))
            return;
        lda8((uint8_t)(cpu.a & 0x0B));
        cmp8(0x01);
        if ((uint8_t)cpu.a == 0x01)
            break;
        cmp8(0x02);
        if ((uint8_t)cpu.a == 0x02)
            break;
    }
    if (lap_matches_0a())
        return;
    lap_take_0a();
    lda8(0x40);
    abi_GsuRecordRaise6A();
    cpu.p |= P_M;
    sound0(0x25);
    spark_follow(0xB7A8);
}

/*
 * @routine 09:AD5E ObjectLapBallUp
 * WHAT IT DOES: Sets up an up ball: restart $09:AD81 (ObjectLapBallUpRecord),
 * frame $09:ADB6 (ObjectLapBallUpStep), +$0A = 4 (no lap lit); its record
 * and frame.
 * HOW: Word and byte stores.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void abi_ObjectLapBallUp(void)
{
    handlers2(0xAD81, 0xADB6);
    st8i(F(0x200A, cpu.x), 0x04);
    lap_ball_record(0xB7A8);
    ball_up_frame();
}

/*
 * @routine 09:AD81 ObjectLapBallUpRecord
 * WHAT IT DOES: The up ball's restart: its record ($42D, flag bit 4, +$2A
 * high $14), its model lit ($A040) when +$0A is the current lap, else
 * $B7A8; then its frame.
 * HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordSet2AHigh; the lap
 * compared through $1F58.
 * Entry: as ObjectLapBallUp. Exit: as ObjectLapBallUp.
 */
void abi_ObjectLapBallUpRecord(void)
{
    lap_ball_record(0xB7A8);
    ball_up_frame();
}

/*
 * @routine 09:ADB6 ObjectLapBallUpStep
 * WHAT IT DOES: The up ball's frame: it grows (+$15 and +$14 by $10); on a
 * car's contact with flags 1 or 2 (AND #$0B) on a lap it has not lit
 * (+$0A), it takes the lap, lights up ($A040), raises that car's record
 * +$6A by $40, plays sound $25 and leaves a spark (kind $B7A8) following
 * the player.
 * HOW: ObjectSyncFromRecord, RecordNextContact, GsuRecordRaise6A,
 * ObjectSpawn.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
 * it.
 */
void abi_ObjectLapBallUpStep(void)
{
    ball_up_frame();
}

/*
 * @routine 09:AE60 ObjectSparkFollow
 * WHAT IT DOES: A following spark's frame: at its object's (+$29) position,
 * released when +$0A runs out; it rises by $C0 and a speed +$27 that grows
 * by $28 each frame, and grows (+$15, +$14 by $10).
 * HOW: ObjectCopyPositionFrom; the speed added through $1F4C;
 * ObjectRebucket.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as ObjectRebucket
 * leaves it.
 */
void abi_ObjectSparkFollow(void)
{
    cpu.y = rd16(F(0x2029, cpu.x));
    cpu.p = nz16(cpu.p, cpu.y);
    abi_ObjectCopyPositionFrom();
    if (!dec8(F(0x200A, cpu.x)))
        abi_ObjectRelease();
    add16(F(0x200E, cpu.x), 0xFF40);
    add16(F(0x2027, cpu.x), 0xFFD8);
    add16_via(F(0x200E, cpu.x), F(0x2027, cpu.x));
    add8(F(0x2015, cpu.x), 0x10);
    add8(F(0x2014, cpu.x), 0x10);
    abi_ObjectRebucket();
}

/* 09:AF17: the down ball's frame: as the up ball's (contact flags bit 0),
   lowering the car's record +$82 by $40 (GsuRecordLower82) with sound $1B
   and a following spark $B7C4. */
static void ball_down_frame(void)
{
    add8(F(0x2015, cpu.x), 0x10);
    add8(F(0x2014, cpu.x), 0x10);
    if (!synced() || !next_contact())
        return;
    if (lap_matches_0a())
        return;
    lap_take_0a();
    lda8(0x40);
    abi_GsuRecordLower82();
    sound0(0x1B);
    cpu.p |= P_M;
    spark_follow(0xB7C4);
}

/*
 * @routine 09:AEBF ObjectLapBallDown
 * WHAT IT DOES: Sets up a down ball: restart $09:AEE2
 * (ObjectLapBallDownRecord), frame $09:AF17 (ObjectLapBallDownStep), +$0A
 * = 4 (no lap lit); its record and frame.
 * HOW: Word and byte stores.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void abi_ObjectLapBallDown(void)
{
    handlers2(0xAEE2, 0xAF17);
    st8i(F(0x200A, cpu.x), 0x04);
    lap_ball_record(0xB7C4);
    ball_down_frame();
}

/*
 * @routine 09:AEE2 ObjectLapBallDownRecord
 * WHAT IT DOES: The down ball's restart: its record ($42D, flag bit 4,
 * +$2A high $14), its model lit ($A040) when +$0A is the current lap, else
 * $B7C4; then its frame.
 * HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordSet2AHigh; the lap
 * compared through $1F58.
 * Entry: as ObjectLapBallDown. Exit: as ObjectLapBallDown.
 */
void abi_ObjectLapBallDownRecord(void)
{
    lap_ball_record(0xB7C4);
    ball_down_frame();
}

/*
 * @routine 09:AF17 ObjectLapBallDownStep
 * WHAT IT DOES: The down ball's frame: it grows (+$15 and +$14 by $10); on
 * a car's contact (flags bit 0) on a lap it has not lit (+$0A), it takes
 * the lap, lights up ($A040), lowers that car's record +$82 by $40, plays
 * sound $1B and leaves a spark (kind $B7C4) following the player.
 * HOW: ObjectSyncFromRecord, RecordNextContact, GsuRecordLower82,
 * ObjectSpawn.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
 * it.
 */
void abi_ObjectLapBallDownStep(void)
{
    ball_down_frame();
}

/* 09:AFFC: a car's hit, the first time (+$25 bit 0), moves the count
   $19EE to $19F0 (HudCountMove19EE's copy) and sets $19ED = 8. */
static void count_gate_frame(void)
{
    if (!hit())
        return;
    if (bit25(0x01))
        return;
    or8(F(0x2025, cpu.x), 0x01);
    abi_HudCountMove19EECopy();
    cpu.p |= P_M;
    st8i(db() | 0x19ED, 0x08);
}

/*
 * @routine 09:AFB7 ObjectCountGate
 * WHAT IT DOES: Sets up the count gate: restart $09:AFD5
 * (ObjectCountGateRecord), frame $09:AFFC (ObjectCountGateStep); its record
 * and frame.
 * HOW: Word and byte stores.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void abi_ObjectCountGate(void)
{
    handlers2(0xAFD5, 0xAFFC);
    gate42d(0x0780);
    count_gate_frame();
}

/*
 * @routine 09:AFD5 ObjectCountGateRecord
 * WHAT IT DOES: The count gate's restart: its record ($42D, flag bit 4, at
 * ($780, $780, $780)), then its frame.
 * HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition.
 * Entry: as ObjectCountGate. Exit: as ObjectCountGate.
 */
void abi_ObjectCountGateRecord(void)
{
    gate42d(0x0780);
    count_gate_frame();
}

/*
 * @routine 09:AFFC ObjectCountGateStep
 * WHAT IT DOES: The count gate's frame: a car's hit, the first time (+$25
 * bit 0), moves the count at $19EE to $19F0 (clearing $19EE, $19F2 = 1)
 * and sets $19ED to 8.
 * HOW: ObjectSyncFromRecord, RecordContactSummary; JSR into
 * HudCountMove19EE at its copy (09:C0AB).
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
 * it.
 */
void abi_ObjectCountGateStep(void)
{
    count_gate_frame();
}

/* 09:B065: the counting marker's record ($424, +$0A $90E8) in mode 2 with
   +$2A high $14 at ($500, $500, $500); then its frame. */
static void count_marker_record(void)
{
    abi_GsuRecordNew424Alt();
    abi_GsuRecordMode2();
    lda8(0x14);
    abi_GsuRecordSet2AHigh();
    record_at(0x0500, 0x0500, 0x0500);
}

/* 09:B092: while $19E8 is set, ObjectStep48To8; then synced. */
static void count_marker_frame(void)
{
    cpu.p |= P_M;
    if (ld8(db() | 0x19E8))
        abi_ObjectStep48To8();
    abi_ObjectSyncFromRecord();
}

/*
 * @routine 09:B02A ObjectCountMarker
 * WHAT IT DOES: Sets up the counting marker: restart $09:B065
 * (ObjectCountMarkerRecord), frame $09:B092 (ObjectCountMarkerStep), +$20
 * = $ECD0, shown as 0 (+$5A = $80, twice), +$48 = 4; its record and frame.
 * HOW: Word and byte stores; LDA #0, ORA #$80 for +$5A.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void abi_ObjectCountMarker(void)
{
    handlers2(0xB065, 0xB092);
    st16i(F(0x2020, cpu.x), 0xECD0);
    lda8(0x00);
    lda8(0x80);
    wr8(F(0x205A, cpu.x), 0x80);
    st8i(F(0x2048, cpu.x), 0x04);
    lda8(0x00);
    lda8(0x80);
    wr8(F(0x205A, cpu.x), 0x80);
    count_marker_record();
    count_marker_frame();
}

/*
 * @routine 09:B065 ObjectCountMarkerRecord
 * WHAT IT DOES: The marker's restart: its record (GsuRecordNew424Alt) in
 * mode 2 with +$2A high $14 at ($500, $500, $500), then its frame.
 * HOW: GsuRecordNew424Alt, GsuRecordMode2, GsuRecordSet2AHigh,
 * GsuRecordPosition.
 * Entry: as ObjectCountMarker. Exit: as ObjectCountMarker.
 */
void abi_ObjectCountMarkerRecord(void)
{
    count_marker_record();
    count_marker_frame();
}

/*
 * @routine 09:B092 ObjectCountMarkerStep
 * WHAT IT DOES: The marker's frame: while $19E8 is set its count +$48
 * steps (ObjectStep48To8); then it is synced from its record.
 * HOW: JSR ObjectStep48To8; ObjectSyncFromRecord.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as
 * ObjectSyncFromRecord leaves it.
 */
void abi_ObjectCountMarkerStep(void)
{
    count_marker_frame();
}
