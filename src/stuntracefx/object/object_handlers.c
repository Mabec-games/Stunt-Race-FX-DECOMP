/*
 * object_handlers.c: object handlers of bank $08 (the long pointer at
 * +$1A/+$1C that ObjectRunHandlerA runs each frame): bursts of pieces, a
 * blinking child, and the Speed Trax car and signal pair. A handler that
 * becomes another stores the next one's address and runs it at once
 * (falls into it).
 *
 * Objects are at $2000 + X through the data bank; the Super FX record of
 * the object in hand is $70:0000 + ($70:2476).
 */
#include "machine.h"
#include "cpu_ops.h"

void abi_ObjectSpawn(void);
void abi_ObjectCopyPositionTo(void);
void abi_RandomStepLong(void);
void abi_GsuRecordNew426(void);
void abi_GsuRecordNew422(void);
void abi_GsuRecordVelocity(void);
void abi_GsuRecordPosition(void);
void abi_GsuRecordSetFlags(void);
void abi_ObjectSyncFromRecord(void);
void abi_RecordContactSummary(void);
void abi_ObjectReleaseLong(void);
void abi_ObjectReleaseY(void);
void abi_ObjectRelease(void);
void abi_ObjectBreakOffContact(void);
void abi_ObjectBreakOffChance(void);
void abi_ObjectRebucket(void);
void abi_ObjectCarSpawn(void);
void abi_ObjectNearFocus(void);
void abi_ObjectPoseFromRecord(void);
void abi_GsuRecordFlag4(void);

typedef void (*AbiFn)(void);

static uint32_t db(void)
{
    return (uint32_t)cpu.db << 16;
}

/* LDA #v with M 8-bit. */
static void lda8(uint8_t v)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, v);
}

/* REP #$20, LDA #v, STA addr, SEP #$20. */
static void sta16_imm(uint32_t addr, uint16_t v)
{
    cpu.p &= (uint8_t)~P_M;
    op_lda16_imm(v);
    wr16(addr, v);
    cpu.p |= P_M;
}

/* The handler pointer of object `obj`: +$1A = `h`, +$1C = $08. */
static void set_handler(uint16_t obj, uint16_t h)
{
    uint32_t b = db();

    sta16_imm(b | (uint16_t)(0x201A + obj), h);
    lda8(0x08);
    wr8(b | (uint16_t)(0x201C + obj), 0x08);
}

/* A new object of `kind` ($1F2B, ObjectSpawn): returns whether one was
   made (C), Y = it. */
static int spawn(uint16_t kind)
{
    sta16_imm(db() | 0x1F2B, kind);
    abi_ObjectSpawn();
    return (cpu.p & P_C) != 0;
}

/* PHX, TYX, PLY: X and Y swapped, N and Z from the pulled Y. */
static void swap_xy(void)
{
    uint16_t t = cpu.x;

    cpu.x = cpu.y;
    cpu.y = t;
    cpu.p = nz16(cpu.p, cpu.y);
}

/* PHA, REP #$20, TYA, STA +$06,X, SEP #$20, PLA: the new object Y as
   object X's child. */
static void set_child(void)
{
    uint8_t lo = (uint8_t)cpu.a;

    wr16(db() | (uint16_t)(0x2006 + cpu.x), cpu.y);
    cpu.a = (uint16_t)((cpu.y & 0xFF00) | lo);
    cpu.p = nz8(cpu.p, lo);
}

/* The record's position (direct page $02, $08, $94: X, Y, Z) set
   (GsuRecordPosition), then its flags from $02 (GsuRecordSetFlags). */
static void record_place(uint16_t x, uint16_t y, uint16_t z, uint16_t flags)
{
    sta16_imm(0x000002, x);
    sta16_imm(0x000008, y);
    sta16_imm(0x000094, z);
    abi_GsuRecordPosition();
    sta16_imm(0x000002, flags);
    abi_GsuRecordSetFlags();
}

/* PHX, REP #$20, X = $70:2476 (the record): its +$42 = v; SEP #$20, PLX. */
static void record_42_set(uint16_t v)
{
    uint16_t x = cpu.x;

    cpu.p &= (uint8_t)~P_M;
    op_lda16(0x702476);
    op_tax16();
    op_lda16_imm(v);
    wr16(0x700042 + cpu.x, v);
    cpu.p |= P_M;
    cpu.x = x;
    cpu.p = nz16(cpu.p, x);
}

/* ObjectSyncFromRecord, then +$36 = 3. */
static void sync_36(void)
{
    abi_ObjectSyncFromRecord();
    lda8(0x03);
    wr8(db() | (uint16_t)(0x2036 + cpu.x), 0x03);
}

/* LDA +$48,X: whether the count has run out (0); otherwise it is counted
   down (DEC +$48,X). */
static int count_down(void)
{
    uint32_t b = db();
    uint8_t v = rd8(b | (uint16_t)(0x2048 + cpu.x));

    lda8(v);
    if (!v)
        return 1;
    v = (uint8_t)(rd8(b | (uint16_t)(0x2048 + cpu.x)) - 1);
    wr8(b | (uint16_t)(0x2048 + cpu.x), v);
    cpu.p = nz8(cpu.p, v);
    return 0;
}

/* +off,X 8-bit = random & $3F, then the word there + add (16-bit, the
   high byte as it was). Returns the word. */
static uint16_t random_word(uint16_t off, uint16_t add)
{
    uint32_t b = db();
    uint16_t w;
    uint8_t v;

    abi_RandomStepLong();
    v = (uint8_t)(cpu.a & 0x3F);
    lda8(v);
    wr8(b | (uint16_t)(off + cpu.x), v);
    cpu.p &= (uint8_t)~P_M;
    w = rd16(b | (uint16_t)(off + cpu.x));
    cpu.a = w;
    if (add & 0x8000)
        op_sub16((uint16_t)-add);
    else
        op_add16(add);
    wr16(b | (uint16_t)(off + cpu.x), cpu.a);
    cpu.p |= P_M;
    return cpu.a;
}

/* One burst piece: a new object of `kind` with handler $08:ED39
   (ObjectBurstPieceStart) at object X's position; none when the spawn
   fails. */
static void burst_piece(uint16_t kind)
{
    if (!spawn(kind))
        return;
    set_handler(cpu.y, 0xED39);
    abi_ObjectCopyPositionTo();
}

/*
 * @routine 08:EC9C ObjectBurstSpawn
 * WHAT IT DOES: Object X bursts into four pieces: new objects of kinds
 * $A5F0, $A60C, $A628 and $A644 at its position, each with handler $08:ED39
 * (ObjectBurstPieceStart).
 * HOW: ObjectSpawn and ObjectCopyPositionTo for each; a failed spawn skips
 * its piece.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; Y = the last piece
 * (or as ObjectSpawn left it); A and flags as the last call left them.
 */
void abi_ObjectBurstSpawn(void)
{
    burst_piece(0xA5F0);
    burst_piece(0xA60C);
    burst_piece(0xA628);
    burst_piece(0xA644);
}

/*
 * @routine 08:EDB6 ObjectBurstPieceStep
 * WHAT IT DOES: A burst piece's frame: synced from its Super FX record
 * (ObjectSyncFromRecord); released when that says so (C set), or after
 * the code at $08:D337 when the record's contact summary has bit 7 and
 * the sync's bit 0 is clear; otherwise released once its count +$48 runs
 * out, else counted down.
 * HOW: The releases are jumps (ObjectReleaseLong; $08:D337 as original
 * code).
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; A low byte the
 * count; as the release leaves it otherwise.
 */
void abi_ObjectBurstPieceStep(void)
{
    abi_ObjectSyncFromRecord();
    if (cpu.p & P_C) {
        abi_ObjectReleaseLong();
        return;
    }
    lda8((uint8_t)(cpu.a & 0x01));
    if (!(cpu.a & 0x01)) {
        abi_RecordContactSummary();
        lda8((uint8_t)(cpu.a & 0x80));
        if (cpu.a & 0x80) {
            abi_ObjectBreakOffContact();
            return;
        }
    }
    if (count_down())
        abi_ObjectReleaseLong();
}

/*
 * @routine 08:ED39 ObjectBurstPieceStart
 * WHAT IT DOES: A burst piece's first frame: handler $08:EDB6
 * (ObjectBurstPieceStep), a random velocity (X and Z -$1F to +$20, Y up
 * -$1E to -$5D), count +$48 = $28, random +$15 and +$14, a Super FX record
 * (GsuRecordNew426) given the velocity (GsuRecordVelocity); then its first
 * step.
 * HOW: Each random part is RandomStepLong & $3F stored to the low byte and
 * then adjusted as a word (the high byte as it was). Falls into
 * ObjectBurstPieceStep.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectBurstPieceStep.
 */
void abi_ObjectBurstPieceStart(void)
{
    uint32_t b = db();
    uint16_t w;

    set_handler(cpu.x, 0xEDB6);
    random_word(0x202E, (uint16_t)-0x001F);
    random_word(0x2030, 0x001E);
    cpu.p &= (uint8_t)~P_M;
    w = (uint16_t)((rd16(b | (uint16_t)(0x2030 + cpu.x)) ^ 0xFFFF) + 1);
    cpu.a = w;
    cpu.p = nz16(cpu.p, w);
    wr16(b | (uint16_t)(0x2030 + cpu.x), w);
    cpu.p |= P_M;
    random_word(0x2032, (uint16_t)-0x001F);
    lda8(0x28);
    wr8(b | (uint16_t)(0x2048 + cpu.x), 0x28);
    abi_RandomStepLong();
    wr8(b | (uint16_t)(0x2015 + cpu.x), (uint8_t)cpu.a);
    abi_RandomStepLong();
    wr8(b | (uint16_t)(0x2014 + cpu.x), (uint8_t)cpu.a);
    abi_GsuRecordNew426();
    abi_GsuRecordVelocity();
    abi_ObjectBurstPieceStep();
}

/*
 * @routine 08:EDDB ObjectHandlerEmptyA
 * WHAT IT DOES: Nothing: an empty object handler.
 * HOW: RTL. Not reached in the traces so far.
 * Entry: any. Exit: every register unchanged.
 */
void abi_ObjectHandlerEmptyA(void)
{
}

/*
 * @routine 08:EDDC ObjectHandlerEmptyB
 * WHAT IT DOES: Nothing: an empty object handler.
 * HOW: RTL. Not reached in the traces so far.
 * Entry: any. Exit: every register unchanged.
 */
void abi_ObjectHandlerEmptyB(void)
{
}

/* The blinker's record: +$42 = $1000 (shown), synced, +$36 = 3. */
static void blinker_show(void)
{
    record_42_set(0x1000);
    sync_36();
}

void abi_ObjectBlinkerFade(void);

/*
 * @routine 08:EE63 ObjectBlinkerRise
 * WHAT IT DOES: The blinker's first phase: while its count +$48 runs, its
 * record shown (+$42 = $1000) and synced (+$36 = 3); then count $28 and
 * handler $08:EE9F (ObjectBlinkerFade), run at once.
 * HOW: Falls into ObjectBlinkerFade.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; as
 * ObjectSyncFromRecord leaves it, A low byte 3.
 */
void abi_ObjectBlinkerRise(void)
{
    uint32_t b = db();

    if (!count_down()) {
        blinker_show();
        return;
    }
    lda8(0x28);
    wr8(b | (uint16_t)(0x2048 + cpu.x), 0x28);
    set_handler(cpu.x, 0xEE9F);
    abi_ObjectBlinkerFade();
}

/*
 * @routine 08:EE9F ObjectBlinkerFade
 * WHAT IT DOES: The blinker's second phase: while its count +$48 runs, its
 * colour +$5A (low seven bits, 0-$0F) steps down one a frame with the
 * record shown, and at 0 the record gets +$42 = $1010; when the count runs
 * out, the record's bit 12 of +$42 is cleared and both its child (+$06,
 * ObjectReleaseY) and it are released.
 * HOW: The colour steps by ADC #$FF with a wrap to $10 kept in its low
 * seven bits, bit 7 set; the release is a jump (ObjectReleaseLong).
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; as the last call
 * leaves it.
 */
void abi_ObjectBlinkerFade(void)
{
    uint32_t b = db();
    uint16_t x;
    uint8_t v, p;

    if (count_down()) {
        x = cpu.x;
        cpu.p &= (uint8_t)~P_M;
        op_lda16(0x702476);
        op_tax16();
        op_lda16(0x700042 + cpu.x);
        op_and16(0xEFFF);
        wr16(0x700042 + cpu.x, cpu.a);
        cpu.p |= P_M;
        cpu.x = x;
        cpu.p = nz16(cpu.p, x);
        cpu.y = rd16(b | (uint16_t)(0x2006 + cpu.x));
        cpu.p = nz16(cpu.p, cpu.y);
        abi_ObjectReleaseY();
        abi_ObjectReleaseLong();
        return;
    }
    v = rd8(b | (uint16_t)(0x205A + cpu.x));
    lda8((uint8_t)(v & 0x7F));
    cpu.p |= P_C;
    if (!(v & 0x7F)) {
        record_42_set(0x1010);
        sync_36();
        return;
    }
    v = rd8(b | (uint16_t)(0x205A + cpu.x));
    p = (uint8_t)(cpu.p & ~P_C);
    v = op_adc8(v, 0xFF, &p);
    if (!(v & 0x80)) {
        p &= (uint8_t)~P_C;
        v = op_adc8(v, 0x10, &p);
    }
    v &= 0x7F;
    if (v >= 0x10)
        v = (uint8_t)(v - 0x10);
    v |= 0x80;
    cpu.p = p;
    lda8(v);
    wr8(b | (uint16_t)(0x205A + cpu.x), v);
    blinker_show();
}

/*
 * @routine 08:EDDD ObjectBlinkerStart
 * WHAT IT DOES: Makes object X a blinker: a child object of kind $A91C at
 * its position (its +$06, put in its bucket), handler $08:EE63
 * (ObjectBlinkerRise), count +$48 = $32, colour +$5A = $8F, +$20 =
 * $D354, a Super FX record (GsuRecordNew422) at $02D0, $0190, $02D0 with
 * flags $4000; then the first phase at once. With no record the handler
 * just returns ($08:A033).
 * HOW: Falls into ObjectBlinkerRise; a failed spawn skips the child.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectBlinkerRise.
 */
void abi_ObjectBlinkerStart(void)
{
    uint32_t b = db();

    if (spawn(0xA91C)) {
        abi_ObjectCopyPositionTo();
        set_child();
        swap_xy();
        abi_ObjectRebucket();
        swap_xy();
    }
    set_handler(cpu.x, 0xEE63);
    lda8(0x32);
    wr8(b | (uint16_t)(0x2048 + cpu.x), 0x32);
    lda8(0x8F);
    wr8(b | (uint16_t)(0x205A + cpu.x), 0x8F);
    sta16_imm(b | (uint16_t)(0x2020 + cpu.x), 0xD354);
    abi_GsuRecordNew422();
    if (cpu.p & P_N) {
        return;                     /* JMP to the RTL at $08:A033 */
        return;
    }
    record_place(0x02D0, 0x0190, 0x02D0, 0x4000);
    abi_ObjectBlinkerRise();
}

/*
 * @routine 08:EF19 ObjectPaceCarStep
 * WHAT IT DOES: The Speed Trax pace car's frame: near the focus
 * (ObjectNearFocus), rebucketed, posed from its record
 * (ObjectPoseFromRecord). Until $1F71 is 2: once its record's Z ($70:0022)
 * + $258 is past $1F73, $1F71 = 1; its record's bit 11 of +$42 is set
 * while its speed ($70:004A) is below $1E, cleared otherwise; then the
 * code at $08:CD7E. At $1F71 = 2: the record's +$1C gets bit 4, its +$24
 * bits 5 and 3, its Y + $B1E0, and its handler cleared.
 * HOW: 16-bit record words through $70:0000 + its +$22; the code at
 * $08:CD7E is a JSL to GsuRecordFlag4 and an RTL, called directly here.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; Y = the object; as
 * the code at $08:CD7E leaves it, or A low byte 0 after the clear.
 */
void abi_ObjectPaceCarStep(void)
{
    uint32_t b = db();
    uint16_t x, w;
    uint8_t v;

    abi_ObjectNearFocus();
    abi_ObjectRebucket();
    abi_ObjectPoseFromRecord();
    x = cpu.x;
    cpu.p &= (uint8_t)~P_M;
    cpu.y = cpu.x;
    op_lda16(b | (uint16_t)(0x2022 + cpu.x));
    op_tax16();
    op_lda16(b | 0x1F71);
    op_and16(0x00FF);
    op_cmp16(0x0002);
    if (cpu.a != 0x0002) {
        op_lda16(0x700022 + cpu.x);
        op_add16(0x0258);
        w = rd16(b | 0x1F73);
        op_cmp16(w);
        if (!(cpu.p & P_N)) {
            cpu.p |= P_M;
            lda8(0x01);
            wr8(b | 0x1F71, 0x01);
            cpu.p &= (uint8_t)~P_M;
        }
        op_lda16(0x700042 + cpu.x);
        op_and16(0xF7FF);
        wr16(0x700042 + cpu.x, cpu.a);
        op_lda16(0x70004A + cpu.x);
        op_cmp16(0x001E);
        if (cpu.p & P_N) {
            op_lda16(0x700042 + cpu.x);
            cpu.a |= 0x0800;
            cpu.p = nz16(cpu.p, cpu.a);
            wr16(0x700042 + cpu.x, cpu.a);
        }
        cpu.p |= P_M;
        cpu.x = x;
        cpu.p = nz16(cpu.p, x);
        abi_GsuRecordFlag4();
        return;
    }
    op_lda16(0x70001C + cpu.x);
    cpu.a |= 0x0010;
    cpu.p = nz16(cpu.p, cpu.a);
    wr16(0x70001C + cpu.x, cpu.a);
    cpu.p |= P_M;
    cpu.x = x;
    cpu.p = nz16(cpu.p, x);
    v = (uint8_t)(rd8(b | (uint16_t)(0x2024 + cpu.x)) | 0x20);
    lda8(v);
    wr8(b | (uint16_t)(0x2024 + cpu.x), v);
    v = (uint8_t)(rd8(b | (uint16_t)(0x2024 + cpu.x)) | 0x08);
    lda8(v);
    wr8(b | (uint16_t)(0x2024 + cpu.x), v);
    cpu.p &= (uint8_t)~P_M;
    w = rd16(b | (uint16_t)(0x200E + cpu.x));
    cpu.a = w;
    op_add16(0xB1E0);
    wr16(b | (uint16_t)(0x200E + cpu.x), cpu.a);
    cpu.p |= P_M;
    sta16_imm(b | (uint16_t)(0x201A + cpu.x), 0x0000);
    lda8(0x00);
    wr8(b | (uint16_t)(0x201C + cpu.x), 0x00);
}

/*
 * @routine 08:EF06 ObjectPaceCarStart
 * WHAT IT DOES: The Speed Trax pace car's first frame: its car set up
 * (ObjectCarSpawn), handler $08:EF19 (ObjectPaceCarStep), run at once.
 * HOW: Falls into ObjectPaceCarStep.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectPaceCarStep.
 */
void abi_ObjectPaceCarStart(void)
{
    abi_ObjectCarSpawn();
    set_handler(cpu.x, 0xEF19);
    abi_ObjectPaceCarStep();
}

/*
 * @routine 08:F0B1 ObjectSignalStep
 * WHAT IT DOES: The pace car's signal: synced (+$36 = 3) each frame; once
 * $1F71 is set, its child (+$06) gets +$04 = $A794 and its light +$5A
 * (low seven bits, 0-7) steps up one a frame, and at 7 the object $0346
 * gets +$04 = $A75C, the child and the signal are released and $1F71 =
 * 2.
 * HOW: The light steps by ADC #1 with a wrap to 8 in its low seven bits,
 * bit 7 set.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; as the last call
 * leaves it.
 */
void abi_ObjectSignalStep(void)
{
    uint32_t b = db();
    uint8_t v, p;

    op_lda8(b | 0x1F71);
    if ((uint8_t)cpu.a) {
        cpu.y = rd16(b | (uint16_t)(0x2006 + cpu.x));
        cpu.p = nz16(cpu.p, cpu.y);
        sta16_imm(b | (uint16_t)(0x2004 + cpu.y), 0xA794);
        v = rd8(b | (uint16_t)(0x205A + cpu.y));
        lda8((uint8_t)(v & 0x7F));
        if ((v & 0x7F) == 0x07) {
            cpu.y = rd16(b | 0x0346);
            cpu.p = nz16(cpu.p, cpu.y);
            sta16_imm(b | (uint16_t)(0x2004 + cpu.y), 0xA75C);
            cpu.y = rd16(b | (uint16_t)(0x2006 + cpu.x));
            cpu.p = nz16(cpu.p, cpu.y);
            abi_ObjectReleaseY();
            abi_ObjectRelease();
            lda8(0x02);
            wr8(b | 0x1F71, 0x02);
            return;
        }
        v = rd8(b | (uint16_t)(0x205A + cpu.y));
        p = (uint8_t)(cpu.p & ~P_C);
        v = op_adc8(v, 0x01, &p);
        if (!(v & 0x80)) {
            p &= (uint8_t)~P_C;
            v = op_adc8(v, 0x08, &p);
        }
        v &= 0x7F;
        if (v >= 0x08)
            v = (uint8_t)(v - 0x08);
        v |= 0x80;
        cpu.p = p;
        lda8(v);
        wr8(b | (uint16_t)(0x205A + cpu.y), v);
    }
    sync_36();
}

/*
 * @routine 08:EFAF ObjectPaceCarSetup
 * WHAT IT DOES: In Speed Trax (HUD mode 0; otherwise released): $1F73 =
 * its Z, $1F71 = 0, its Y - $64; a pace car (kind $A040, handler $08:EF06
 * ObjectPaceCarStart) at its position, Z - $DAC, Y 0, +$27 = the selection
 * handler $18DB; a signal (kind $A7B0) at its position, Y + $32, light
 * +$5A = $80, put in its bucket and made its child; then it becomes the
 * signal's driver: handler $08:F0B1 (ObjectSignalStep), +$20 = $C6DC, a
 * Super FX record (GsuRecordNew422) at $0640, $0640, $0640 with flags
 * $C100, count +$48 = $14, and its first step. A failed spawn or record
 * returns ($08:A033).
 * HOW: Falls into ObjectSignalStep.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectSignalStep.
 */
void abi_ObjectPaceCarSetup(void)
{
    uint32_t b = db();

    op_lda8(b | 0x0D2B);
    if ((uint8_t)cpu.a) {
        abi_ObjectReleaseLong();
        return;
    }
    cpu.p &= (uint8_t)~P_M;
    op_lda16(b | (uint16_t)(0x2010 + cpu.x));
    wr16(b | 0x1F73, cpu.a);
    cpu.p |= P_M;
    lda8(0x00);
    wr8(b | 0x1F71, 0x00);
    cpu.p &= (uint8_t)~P_M;
    op_lda16(b | (uint16_t)(0x200E + cpu.x));
    op_add16(0xFF9C);
    wr16(b | (uint16_t)(0x200E + cpu.x), cpu.a);
    cpu.p |= P_M;
    if (!spawn(0xA040)) {
        return;                     /* JMP to the RTL at $08:A033 */
        return;
    }
    abi_ObjectCopyPositionTo();
    cpu.p &= (uint8_t)~P_M;
    op_lda16(b | (uint16_t)(0x2010 + cpu.y));
    op_add16(0xF254);
    wr16(b | (uint16_t)(0x2010 + cpu.y), cpu.a);
    cpu.p |= P_M;
    sta16_imm(b | (uint16_t)(0x200E + cpu.y), 0x0000);
    cpu.p &= (uint8_t)~P_M;
    op_lda16(b | 0x18DB);
    wr16(b | (uint16_t)(0x2027 + cpu.y), cpu.a);
    cpu.p |= P_M;
    set_handler(cpu.y, 0xEF06);
    if (!spawn(0xA7B0)) {
        return;                     /* JMP to the RTL at $08:A033 */
        return;
    }
    abi_ObjectCopyPositionTo();
    cpu.p &= (uint8_t)~P_M;
    op_lda16(b | (uint16_t)(0x200E + cpu.y));
    op_add16(0x0032);
    wr16(b | (uint16_t)(0x200E + cpu.y), cpu.a);
    cpu.p |= P_M;
    lda8(0x80);
    wr8(b | (uint16_t)(0x205A + cpu.y), 0x80);
    swap_xy();
    abi_ObjectRebucket();
    swap_xy();
    set_child();
    set_handler(cpu.x, 0xF0B1);
    sta16_imm(b | (uint16_t)(0x2020 + cpu.x), 0xC6DC);
    abi_GsuRecordNew422();
    if (cpu.p & P_N) {
        return;                     /* JMP to the RTL at $08:A033 */
        return;
    }
    record_place(0x0640, 0x0640, 0x0640, 0xC100);
    lda8(0x14);
    wr8(b | (uint16_t)(0x2048 + cpu.x), 0x14);
    abi_ObjectSignalStep();
}

void abi_ObjectTurnFromRecord(void);

/* |w| as the original takes it (EOR #$FFFF, INC A when negative). */
static uint16_t abs16(uint16_t w)
{
    return (w & 0x8000) ? (uint16_t)((w ^ 0xFFFF) + 1) : w;
}

/* With record X: whether |+$2E| + |+$32| (kept in $1F2B) is at least $64,
   signed (BPL after CMP #$0064). */
static int child_fast(uint32_t b)
{
    uint16_t w;

    w = abs16(rd16(0x70002E + cpu.x));
    wr16(b | 0x1F2B, w);
    w = abs16(rd16(0x700032 + cpu.x));
    cpu.a = (uint16_t)(w + rd16(b | 0x1F2B));
    op_cmp16(0x0064);
    return !(cpu.p & P_N);
}

/* ObjectBreakOffChance with X = the record, Y = the child: whether a
   piece was made (C). */
static int child_d1bf(void)
{
    abi_ObjectBreakOffChance();
    return (cpu.p & P_C) != 0;
}

/* The child Y's next handler `h` (bank $08). */
static void child_handler(uint16_t h)
{
    set_handler(cpu.y, h);
}

/*
 * @routine 08:BD22 ObjectChildFollow
 * WHAT IT DOES: Keeps object X (a child of +$06) on its parent: rebucketed;
 * when the parent's record has flag $0400, its own record gets $0480 and
 * its +$24 bit 5, and nothing more. Otherwise its +$24 bit 5 and its
 * record's $0480 are cleared; unless the parent's +$24 has bit 6 or its
 * record's +$64 has neither bit 0 nor 1, and when its own record's +$46
 * has bits 0 and 4: by its record's +$2C (bit 7, bit 13, bit 11, bit 12,
 * bits 5-6) or, failing those, the parent's +$40 bit 11 or its +$46 bit 5,
 * a new handler for it ($D1F7, $D249, $D39F, $D2CF; $D33F or $D303 by
 * speed; $D3D3) when it moves fast enough (|+$2E| + |+$32| >= $64, where
 * asked) and the code at $08:D1BF allows it (C). Then its position is its
 * record's (+$1E + $4A, +$20 + $4C + $FFEC - its +$5F, +$22 + $4E) and
 * its turn (ObjectTurnFromRecord).
 * HOW: 16-bit record reads at $70:0000 + +$22; the object kept on the
 * stack (PHX) and in Y.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; X = the object; Y
 * as the path leaves it.
 */
void abi_ObjectChildFollow(void)
{
    uint32_t b = db();
    uint16_t obj = cpu.x, me, w, d;

    abi_ObjectRebucket();
    me = cpu.x;
    cpu.y = me;
    cpu.p &= (uint8_t)~P_M;
    cpu.x = rd16(b | (uint16_t)(0x2006 + cpu.x));
    cpu.x = rd16(b | (uint16_t)(0x2022 + cpu.x));
    op_lda16(0x700016 + cpu.x);
    if (cpu.a & 0x0400) {
        cpu.x = rd16(b | (uint16_t)(0x2022 + cpu.y));
        op_lda16(0x700016 + cpu.x);
        cpu.a |= 0x0480;
        cpu.p = nz16(cpu.p, cpu.a);
        wr16(0x700016 + cpu.x, cpu.a);
        cpu.p |= P_M;
        w = (uint8_t)(rd8(b | (uint16_t)(0x2024 + cpu.y)) | 0x20);
        lda8((uint8_t)w);
        wr8(b | (uint16_t)(0x2024 + cpu.y), (uint8_t)w);
        cpu.x = obj;
        cpu.p = nz16(cpu.p, obj);
        return;
    }
    cpu.x = cpu.y;
    cpu.p |= P_M;
    w = (uint8_t)(rd8(b | (uint16_t)(0x2024 + cpu.x)) & 0xDF);
    lda8((uint8_t)w);
    wr8(b | (uint16_t)(0x2024 + cpu.x), (uint8_t)w);
    cpu.p &= (uint8_t)~P_M;
    cpu.x = rd16(b | (uint16_t)(0x2022 + cpu.x));
    op_lda16(0x700016 + cpu.x);
    op_and16(0xFB7F);
    wr16(0x700016 + cpu.x, cpu.a);
    cpu.x = rd16(b | (uint16_t)(0x2006 + cpu.y));
    op_lda16(b | (uint16_t)(0x2024 + cpu.x));
    if (cpu.a & 0x0040)
        goto place;
    cpu.x = rd16(b | (uint16_t)(0x2022 + cpu.x));
    op_lda16(0x700040 + cpu.x);
    wr16(b | 0x1F4C, cpu.a);
    op_lda16(0x700064 + cpu.x);
    if (!(cpu.a & 0x0003))
        goto place;
    cpu.x = rd16(b | (uint16_t)(0x2022 + me));
    op_lda16(0x700046 + cpu.x);
    if (!(cpu.a & 0x0001) || !(cpu.a & 0x0010))
        goto place;
    d = rd16(0x70002C + cpu.x);
    cpu.a = d;
    if (d & 0x0080) {
        if (!child_d1bf())
            goto place;
        cpu.p &= (uint8_t)~P_M;
        child_handler(child_fast(b) ? 0xD33F : 0xD303);
        goto place;
    }
    if (d & 0x2000) {
        if (child_fast(b) && child_d1bf())
            child_handler(0xD2CF);
        goto place;
    }
    if (d & 0x0800) {
        if (child_fast(b) && child_d1bf())
            child_handler(0xD249);
        goto place;
    }
    if (d & 0x1000) {
        if (child_fast(b) && child_d1bf())
            child_handler(0xD1F7);
        goto place;
    }
    if (d & 0x0060) {
        if (child_fast(b) && child_d1bf())
            child_handler(0xD39F);
        goto place;
    }
    op_lda16(b | 0x1F4C);
    if (!(cpu.a & 0x0800)) {
        op_lda16(0x700046 + cpu.x);
        if (!(cpu.a & 0x0020))
            goto place;
    }
    if (child_d1bf())
        child_handler(0xD3D3);
place:
    /* $08:BF53 */
    cpu.y = me;
    cpu.p &= (uint8_t)~P_M;
    cpu.x = rd16(b | (uint16_t)(0x2022 + cpu.y));
    op_lda16(0x70001E + cpu.x);
    op_add16(rd16(0x70004A + cpu.x));
    wr16(b | (uint16_t)(0x200C + cpu.y), cpu.a);
    op_lda16(b | (uint16_t)(0x205F + cpu.y));
    cpu.a = (uint16_t)(((cpu.a & 0x00FF) ^ 0xFFFF) + 1);
    op_add16(0xFFEC);
    op_add16(rd16(0x700020 + cpu.x));
    op_add16(rd16(0x70004C + cpu.x));
    wr16(b | (uint16_t)(0x200E + cpu.y), cpu.a);
    op_lda16(0x700022 + cpu.x);
    op_add16(rd16(0x70004E + cpu.x));
    wr16(b | (uint16_t)(0x2010 + cpu.y), cpu.a);
    abi_ObjectTurnFromRecord();
    cpu.p |= P_M;
    cpu.x = obj;
    cpu.p = nz16(cpu.p, obj);
}

void abi_ObjectVelocityFromRecordOf(void);
void abi_ObjectSpawnChanceD460(void);
void abi_ApproachQuarterMin4(void);
void abi_ObjectApplyVelocity(void);

/*
 * @routine 08:BA57 ObjectChildrenSpawn
 * WHAT IT DOES: Gives object X a child object for each record in its
 * record's chain (its record's +$3E, then each one's +$34, to 0), except
 * records with +$46 bit 9: the child's kind, +$29 and +$5F from the three
 * words at $09:8000 + the record's +$12; with a nonzero +$29 the child
 * gets +$24 bit 3, +$3A = 0, +$5F and +$29 (+ 2 for two players);
 * handler $08:BB16 (ObjectChildStart), +$22 = the record, +$06 = object
 * X. With no record of its own it just returns ($08:A033).
 * HOW: $1F50 keeps the parent; $1F4C, $1F52, $1F4E the table's words; a
 * failed spawn skips the record.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; X = the object, N
 * and Z from it (PLX); Y the last child (or as ObjectSpawn left it).
 */
void abi_ObjectChildrenSpawn(void)
{
    uint32_t b = db();
    uint16_t obj = cpu.x, rec, t;

    wr16(b | 0x1F50, obj);
    cpu.p &= (uint8_t)~P_M;
    op_lda16(b | (uint16_t)(0x2022 + obj));
    if (!cpu.a) {
        return;                     /* JMP to the RTL at $08:A033 */
        return;
    }
    cpu.x = cpu.a;
    op_lda16(0x70003E + cpu.x);
    for (;;) {
        op_tax16();
        if (!cpu.x)
            break;
        rec = cpu.x;
        op_lda16(0x700046 + rec);
        if (!(cpu.a & 0x0200)) {
            t = rd16(0x700012 + rec);
            wr16(b | 0x1F4C, rd16(0x098000 + t));
            wr16(b | 0x1F52, rd16(0x098002 + t));
            op_lda16(0x098004 + t);
            wr16(b | 0x1F4E, cpu.a);
            cpu.x = rec;
            op_lda16(b | 0x1F4C);
            wr16(b | 0x1F2B, cpu.a);
            cpu.p |= P_M;
            abi_ObjectSpawn();
            if (cpu.p & P_C) {
                op_lda8(b | 0x1F52);
                cpu.a |= rd8(b | 0x1F53);
                cpu.p = nz8(cpu.p, (uint8_t)cpu.a);
                if ((uint8_t)cpu.a) {
                    lda8((uint8_t)(rd8(b | (uint16_t)(0x2024 + cpu.y)) | 0x08));
                    wr8(b | (uint16_t)(0x2024 + cpu.y), (uint8_t)cpu.a);
                    lda8(0x00);
                    wr8(b | (uint16_t)(0x203A + cpu.y), 0x00);
                    op_lda8(b | 0x1F4E);
                    wr8(b | (uint16_t)(0x205F + cpu.y), (uint8_t)cpu.a);
                    cpu.p &= (uint8_t)~P_M;
                    op_lda16(b | 0x1F52);
                    wr16(b | (uint16_t)(0x2029 + cpu.y), cpu.a);
                    cpu.p |= P_M;
                    op_lda8(b | 0x0D62);
                    if ((uint8_t)cpu.a) {
                        cpu.p &= (uint8_t)~P_M;
                        op_lda16(b | (uint16_t)(0x2029 + cpu.y));
                        op_add16(0x0002);
                        wr16(b | (uint16_t)(0x2029 + cpu.y), cpu.a);
                        cpu.p |= P_M;
                    }
                }
                set_handler(cpu.y, 0xBB16);
                wr16(b | (uint16_t)(0x2022 + cpu.y), cpu.x);
                cpu.p &= (uint8_t)~P_M;
                op_lda16(b | 0x1F50);
                wr16(b | (uint16_t)(0x2006 + cpu.y), cpu.a);
                cpu.p |= P_M;
            }
        }
        cpu.p &= (uint8_t)~P_M;
        op_lda16(0x700034 + cpu.x);
    }
    cpu.x = obj;
    cpu.p = (uint8_t)(nz16(cpu.p, obj) | P_M);
}

/* $08:BB4E: object X's +$24 bit 6 cleared, then its bit 5 set when its
   parent's +$24 has bit 3, or its bit 6 when the parent's has bit 6;
   +$36 = 3. */
static void child_flags(void)
{
    uint32_t b = db();
    uint8_t v, pv;

    cpu.y = rd16(b | (uint16_t)(0x2006 + cpu.x));
    cpu.p = nz16(cpu.p, cpu.y);
    v = (uint8_t)(rd8(b | (uint16_t)(0x2024 + cpu.x)) & 0xBF);
    lda8(v);
    wr8(b | (uint16_t)(0x2024 + cpu.x), v);
    pv = rd8(b | (uint16_t)(0x2024 + cpu.y));
    lda8(pv);
    if (pv & 0x08) {
        v = (uint8_t)(rd8(b | (uint16_t)(0x2024 + cpu.x)) | 0x20);
        lda8(v);
        wr8(b | (uint16_t)(0x2024 + cpu.x), v);
    } else if (pv & 0x40) {
        v = (uint8_t)(rd8(b | (uint16_t)(0x2024 + cpu.x)) | 0x40);
        lda8(v);
        wr8(b | (uint16_t)(0x2024 + cpu.x), v);
    }
    lda8(0x03);
    wr8(b | (uint16_t)(0x2036 + cpu.x), 0x03);
}

/*
 * @routine 08:BB2E ObjectChildStep
 * WHAT IT DOES: A child object's frame on its parent: follows it
 * (ObjectChildFollow); when the parent's +$25 has bit 2 its handler
 * becomes $08:BB7C (ObjectChildLoose, next frame); then its +$24 flags
 * from the parent's and +$36 = 3.
 * HOW: Byte tests through the data bank.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; A low byte 3; Y =
 * the parent.
 */
void abi_ObjectChildStep(void)
{
    uint32_t b = db();

    abi_ObjectChildFollow();
    cpu.y = rd16(b | (uint16_t)(0x2006 + cpu.x));
    cpu.p = nz16(cpu.p, cpu.y);
    lda8((uint8_t)(rd8(b | (uint16_t)(0x2025 + cpu.y)) & 0x04));
    if ((uint8_t)cpu.a)
        set_handler(cpu.x, 0xBB7C);
    child_flags();
}

/*
 * @routine 08:BB16 ObjectChildStart
 * WHAT IT DOES: A child object's first frame: handler $08:BB2E
 * (ObjectChildStep), its +$26 from its parent's; then its first step.
 * HOW: Falls into ObjectChildStep.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectChildStep.
 */
void abi_ObjectChildStart(void)
{
    uint32_t b = db();

    set_handler(cpu.x, 0xBB2E);
    cpu.y = rd16(b | (uint16_t)(0x2006 + cpu.x));
    cpu.p = nz16(cpu.p, cpu.y);
    op_lda8(b | (uint16_t)(0x2026 + cpu.y));
    wr8(b | (uint16_t)(0x2026 + cpu.x), (uint8_t)cpu.a);
    abi_ObjectChildStep();
}

/* $08:BD06: back on the parent: +$22 = +$27 (its record again), handler
   $08:BB2E (ObjectChildStep), then the flags. */
static void child_back(void)
{
    uint32_t b = db();

    cpu.p &= (uint8_t)~P_M;
    op_lda16(b | (uint16_t)(0x2027 + cpu.x));
    wr16(b | (uint16_t)(0x2022 + cpu.x), cpu.a);
    cpu.p |= P_M;
    set_handler(cpu.x, 0xBB2E);
    child_flags();
}

/* LDA +$5F (low byte), negated, + $FFEC, + a: a Y offset below the
   parent by the child's +$5F and $14. */
static uint16_t child_y(uint16_t f5, uint16_t a)
{
    cpu.a = (uint16_t)(((f5 & 0x00FF) ^ 0xFFFF) + 1);
    op_add16(0xFFEC);
    op_add16(a);
    return cpu.a;
}

/*
 * @routine 08:BC73 ObjectChildReturn
 * WHAT IT DOES: A loose child going back: its velocity from its parent's
 * record (ObjectVelocityFromRecordOf); its position a step (a quarter, at
 * least 4: ApproachQuarterMin4) toward its place on the parent's record
 * (+$1E + $4A, +$20 + $4C + $FFEC - its +$5F, +$22 + $4E, its turn by
 * ObjectTurnFromRecord), moved (ObjectApplyVelocity) and rebucketed; once
 * the parent's +$25 bit 2 is clear it is back (+$22 = +$27, handler
 * $08:BB2E). Then its +$24 flags and +$36 = 3.
 * HOW: The target kept in direct page $04, $0A and $05F2.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; A low byte 3.
 */
void abi_ObjectChildReturn(void)
{
    uint32_t b = db();
    uint16_t obj, w;

    cpu.y = rd16(b | (uint16_t)(0x2006 + cpu.x));
    cpu.p = nz16(cpu.p, cpu.y);
    abi_ObjectVelocityFromRecordOf();
    cpu.p &= (uint8_t)~P_M;
    obj = cpu.x;
    op_lda16(b | (uint16_t)(0x2027 + cpu.x));
    cpu.y = cpu.x;
    op_tax16();
    op_lda16(0x70001E + cpu.x);
    op_add16(rd16(0x70004A + cpu.x));
    wr16(0x000004, cpu.a);
    w = rd16(b | (uint16_t)(0x205F + cpu.y));
    child_y(w, rd16(0x700020 + cpu.x));
    op_add16(rd16(0x70004C + cpu.x));
    wr16(0x00000A, cpu.a);
    op_lda16(0x700022 + cpu.x);
    op_add16(rd16(0x70004E + cpu.x));
    wr16(b | 0x05F2, cpu.a);
    abi_ObjectTurnFromRecord();
    cpu.x = obj;
    cpu.p |= P_M;
    cpu.y = rd16(b | (uint16_t)(0x2006 + cpu.x));
    cpu.p = nz16(cpu.p, cpu.y);
    cpu.p &= (uint8_t)~P_M;
    wr16(0x00003E, rd16(b | (uint16_t)(0x200C + cpu.x)));
    op_lda16(0x000004);
    abi_ApproachQuarterMin4();
    wr16(b | (uint16_t)(0x200C + cpu.x), cpu.a);
    wr16(0x00003E, rd16(b | (uint16_t)(0x200E + cpu.x)));
    op_lda16(0x00000A);
    abi_ApproachQuarterMin4();
    wr16(b | (uint16_t)(0x200E + cpu.x), cpu.a);
    wr16(0x00003E, rd16(b | (uint16_t)(0x2010 + cpu.x)));
    op_lda16(b | 0x05F2);
    abi_ApproachQuarterMin4();
    wr16(b | (uint16_t)(0x2010 + cpu.x), cpu.a);
    cpu.p |= P_M;
    abi_ObjectApplyVelocity();
    abi_ObjectRebucket();
    lda8((uint8_t)(rd8(b | (uint16_t)(0x2025 + cpu.y)) & 0x04));
    if (!(uint8_t)cpu.a) {
        child_back();
        return;
    }
    child_flags();
}

/*
 * @routine 08:BBE2 ObjectChildFlying
 * WHAT IT DOES: A loose child's flight: synced from its record
 * (ObjectSyncFromRecord); with the parent's +$25 bit 4 and a contact
 * (RecordContactSummary bit 7, the sync's bit 0 clear) the code at
 * $08:D337. While its count +$4B runs it drops ($14 + its +$5F lower)
 * and may spawn (ObjectSpawnChanceD460); at 0 with the parent's +$25 bit
 * 4 it is released if the parent's record +$64 has bit 4, else may spawn;
 * at 0 otherwise it goes back (handler $08:BC73, ObjectChildReturn, at
 * once). Then its +$24 flags and +$36 = 3.
 * HOW: Byte tests through the data bank; the release is a jump.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; as the path
 * leaves it.
 */
void abi_ObjectChildFlying(void)
{
    uint32_t b = db();
    uint16_t x;
    uint8_t v;

    abi_ObjectSyncFromRecord();
    lda8((uint8_t)(cpu.a & 0x01));
    if (!(uint8_t)cpu.a) {
        cpu.y = rd16(b | (uint16_t)(0x2006 + cpu.x));
        cpu.p = nz16(cpu.p, cpu.y);
        lda8((uint8_t)(rd8(b | (uint16_t)(0x2025 + cpu.y)) & 0x10));
        if ((uint8_t)cpu.a) {
            abi_RecordContactSummary();
            lda8((uint8_t)(cpu.a & 0x80));
            if ((uint8_t)cpu.a) {
                abi_ObjectBreakOffContact();
                return;
            }
        }
    }
    cpu.y = rd16(b | (uint16_t)(0x2006 + cpu.x));
    cpu.p = nz16(cpu.p, cpu.y);
    lda8((uint8_t)(rd8(b | (uint16_t)(0x2025 + cpu.y)) & 0x10));
    if ((uint8_t)cpu.a) {
        op_lda8(b | (uint16_t)(0x204B + cpu.x));
        if (!(uint8_t)cpu.a) {
            x = cpu.x;
            cpu.p &= (uint8_t)~P_M;
            cpu.y = rd16(b | (uint16_t)(0x2006 + cpu.x));
            cpu.x = rd16(b | (uint16_t)(0x2022 + cpu.y));
            op_lda16(0x700064 + cpu.x);
            cpu.x = x;
            op_and16(0x0010);
            cpu.p |= P_M;
            if (cpu.a) {
                abi_ObjectReleaseLong();
                return;
            }
            abi_ObjectSpawnChanceD460();
            child_flags();
            return;
        }
    }
    op_lda8(b | (uint16_t)(0x204B + cpu.x));
    if (!(uint8_t)cpu.a) {
        set_handler(cpu.x, 0xBC73);
        abi_ObjectChildReturn();
        return;
    }
    v = (uint8_t)(rd8(b | (uint16_t)(0x204B + cpu.x)) - 1);
    wr8(b | (uint16_t)(0x204B + cpu.x), v);
    cpu.p &= (uint8_t)~P_M;
    op_lda16(b | (uint16_t)(0x205F + cpu.x));
    child_y(cpu.a, rd16(b | (uint16_t)(0x200E + cpu.x)));
    wr16(b | (uint16_t)(0x200E + cpu.x), cpu.a);
    cpu.p |= P_M;
    abi_ObjectSpawnChanceD460();
    child_flags();
}

/*
 * @routine 08:BB7C ObjectChildLoose
 * WHAT IT DOES: A child comes loose from its parent: handler $08:BBE2
 * (ObjectChildFlying), +$27 = its record (+$22), a record of its own
 * (GsuRecordNew426) with the parent's record's velocity
 * (ObjectVelocityFromRecordOf), then X and Z halved (+$36, +$3A of the
 * old record, arithmetic shift) and Y $FFCE (GsuRecordVelocity); count
 * +$4B = random 0-$0F + $0A; then its first flight.
 * HOW: Falls into ObjectChildFlying; the halving is CMP #$8000, ROR A.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectChildFlying.
 */
void abi_ObjectChildLoose(void)
{
    uint32_t b = db();
    uint16_t x, w;
    uint8_t v;

    set_handler(cpu.x, 0xBBE2);
    cpu.p &= (uint8_t)~P_M;
    op_lda16(b | (uint16_t)(0x2022 + cpu.x));
    wr16(b | (uint16_t)(0x2027 + cpu.x), cpu.a);
    cpu.p |= P_M;
    abi_GsuRecordNew426();
    cpu.y = rd16(b | (uint16_t)(0x2006 + cpu.x));
    cpu.p = nz16(cpu.p, cpu.y);
    abi_ObjectVelocityFromRecordOf();
    if (cpu.p & P_N) {
        child_back();
        return;
    }
    cpu.p &= (uint8_t)~P_M;
    x = cpu.x;
    cpu.y = cpu.x;
    cpu.x = rd16(b | (uint16_t)(0x2027 + cpu.x));
    w = rd16(0x700036 + cpu.x);
    wr16(b | (uint16_t)(0x202E + cpu.y), (uint16_t)((w >> 1) | (w & 0x8000)));
    wr16(b | (uint16_t)(0x2030 + cpu.y), 0xFFCE);
    w = rd16(0x70003A + cpu.x);
    cpu.a = (uint16_t)((w >> 1) | (w & 0x8000));
    cpu.p = (uint8_t)(nz16(cpu.p, cpu.a) & ~P_C) | (w & 1);
    wr16(b | (uint16_t)(0x2032 + cpu.y), cpu.a);
    cpu.x = x;
    cpu.p |= P_M;
    abi_GsuRecordVelocity();
    abi_RandomStepLong();
    v = (uint8_t)(cpu.a & 0x0F);
    lda8(v);
    wr8(b | (uint16_t)(0x204B + cpu.x), v);
    v = (uint8_t)(rd8(b | (uint16_t)(0x204B + cpu.x)) + 0x0A);
    lda8(v);
    wr8(b | (uint16_t)(0x204B + cpu.x), v);
    abi_ObjectChildFlying();
}

void abi_ObjectVelocityFromRecord(void);
void abi_ObjectVelocityFromRecordScaled(void);
void abi_ObjectAdvance5DIfFlag(void);
void abi_RotateXZFrom08(void);
void abi_RotateXZ(void);

/*
 * @routine 08:D1BF ObjectBreakOffChance
 * WHAT IT DOES: For child Y breaking off: on a random byte of $7F or more,
 * a new object (kind $A1AC) at Y's position with Y's parent (+$06) as its
 * +$27; C set when one was made, clear otherwise.
 * HOW: X kept (PHX, PLX); ObjectSpawn and ObjectCopyPositionTo from Y
 * (TYX).
 * Entry: X 16-bit; Y = the child. Exit: M 8-bit; X kept; C as above; Y =
 * the new object when there is one.
 */
void abi_ObjectBreakOffChance(void)
{
    uint32_t b = db();
    uint16_t x = cpu.x;

    cpu.p |= P_M;
    cpu.x = cpu.y;
    cpu.p = nz16(cpu.p, cpu.x);
    abi_RandomStepLong();
    cpu.p = (uint8_t)((nz8(cpu.p, (uint8_t)((uint8_t)cpu.a - 0x7F)) & ~P_C) | ((uint8_t)cpu.a >= 0x7F ? P_C : 0));
    if ((uint8_t)cpu.a >= 0x7F && spawn(0xA1AC)) {
        abi_ObjectCopyPositionTo();
        cpu.p &= (uint8_t)~P_M;
        op_lda16(b | (uint16_t)(0x2006 + cpu.x));
        wr16(b | (uint16_t)(0x2027 + cpu.y), cpu.a);
        cpu.p |= P_M;
        cpu.x = x;
        cpu.p = nz16(cpu.p, x) | P_C;
        return;
    }
    cpu.x = x;
    cpu.p = (uint8_t)(nz16(cpu.p, x) & ~P_C);
}

/*
 * @routine 08:D549 ObjectPieceFall
 * WHAT IT DOES: A broken-off piece's frame: rebucketed; its +$18 and +$19
 * = its frame count +$48 with bit 7; released once the count reaches its
 * end +$49, otherwise counted up and moved (ObjectApplyVelocity); +$36 =
 * 3.
 * HOW: $1F58 and $1F59 hold the count and the end.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; A low byte 3, or as
 * ObjectReleaseLong leaves it.
 */
void abi_ObjectPieceFall(void)
{
    uint32_t b = db();
    uint8_t v;

    abi_ObjectRebucket();
    op_lda8(b | (uint16_t)(0x2048 + cpu.x));
    wr8(b | 0x1F58, (uint8_t)cpu.a);
    v = (uint8_t)(rd8(b | 0x1F58) | 0x80);
    lda8(v);
    wr8(b | (uint16_t)(0x2018 + cpu.x), v);
    op_lda8(b | (uint16_t)(0x2018 + cpu.x));
    wr8(b | (uint16_t)(0x2019 + cpu.x), (uint8_t)cpu.a);
    op_lda8(b | (uint16_t)(0x2049 + cpu.x));
    wr8(b | 0x1F59, (uint8_t)cpu.a);
    op_lda8(b | (uint16_t)(0x2048 + cpu.x));
    v = rd8(b | 0x1F59);
    cpu.p = (uint8_t)((nz8(cpu.p, (uint8_t)((uint8_t)cpu.a - v)) & ~P_C) | ((uint8_t)cpu.a >= v ? P_C : 0));
    if ((uint8_t)cpu.a == v) {
        abi_ObjectReleaseLong();
        return;
    }
    v = (uint8_t)(rd8(b | (uint16_t)(0x2048 + cpu.x)) + 1);
    wr8(b | (uint16_t)(0x2048 + cpu.x), v);
    cpu.p = nz8(cpu.p, v);
    abi_ObjectApplyVelocity();
    lda8(0x03);
    wr8(b | (uint16_t)(0x2036 + cpu.x), 0x03);
}

/* Handler $08:D549 (ObjectPieceFall), count +$48 = 0, end +$49. */
static void piece_handler(uint8_t end)
{
    uint32_t b = db();

    set_handler(cpu.x, 0xD549);
    lda8(0x00);
    wr8(b | (uint16_t)(0x2048 + cpu.x), 0x00);
    if (end == 0x06)
        return;
    lda8((uint8_t)(rd8(b | (uint16_t)(0x2025 + cpu.x)) | 0x01));
    wr8(b | (uint16_t)(0x2025 + cpu.x), (uint8_t)cpu.a);
    lda8(end);
    wr8(b | (uint16_t)(0x2049 + cpu.x), end);
}

/* The break-off look: +$5D = `look`, ObjectAdvance5DIfFlag when `adv`,
   +$24 bit 3, +$3A = 0, +$5F = `f5`. */
static void piece_look(uint16_t look, int adv, uint8_t f5)
{
    uint32_t b = db();

    sta16_imm(b | (uint16_t)(0x205D + cpu.x), look);
    if (adv)
        abi_ObjectAdvance5DIfFlag();
    lda8((uint8_t)(rd8(b | (uint16_t)(0x2024 + cpu.x)) | 0x08));
    wr8(b | (uint16_t)(0x2024 + cpu.x), (uint8_t)cpu.a);
    lda8(0x00);
    wr8(b | (uint16_t)(0x203A + cpu.x), 0x00);
    lda8(f5);
    wr8(b | (uint16_t)(0x205F + cpu.x), f5);
}

/* +$0E (Y) + $28. */
static void piece_drop(void)
{
    uint32_t b = db();

    cpu.p &= (uint8_t)~P_M;
    op_lda16(b | (uint16_t)(0x200E + cpu.x));
    op_add16(0x0028);
    wr16(b | (uint16_t)(0x200E + cpu.x), cpu.a);
    cpu.p |= P_M;
}

/* $08:D403: handler ObjectPieceFall, count 0, end 4, run at once. */
static void piece_fall4(void)
{
    piece_handler(0x04);
    abi_ObjectPieceFall();
}

/*
 * @routine 08:D1F7 ObjectBreakOffA
 * WHAT IT DOES: A child broken off (look $998D): +$24 bit 3, +$3A = 0,
 * +$5F = $14, its parent's record velocity scaled
 * (ObjectVelocityFromRecordScaled), $28 lower; then falls
 * (ObjectPieceFall, 4 frames).
 * HOW: Ends in ObjectPieceFall with +$25 bit 0 set.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectPieceFall.
 */
void abi_ObjectBreakOffA(void)
{
    piece_look(0x998D, 0, 0x14);
    abi_ObjectVelocityFromRecordScaled();
    piece_drop();
    piece_fall4();
}

/*
 * @routine 08:D249 ObjectBreakOffB
 * WHAT IT DOES: As ObjectBreakOffA with look $9B96, the record's velocity
 * unscaled (ObjectVelocityFromRecord) and 3 frames.
 * HOW: As ObjectBreakOffA. Not reached in the traces so far.
 * Entry: as ObjectBreakOffA. Exit: as ObjectBreakOffA.
 */
void abi_ObjectBreakOffB(void)
{
    piece_look(0x9B96, 0, 0x14);
    abi_ObjectVelocityFromRecord();
    piece_drop();
    piece_handler(0x03);
    abi_ObjectPieceFall();
}

/*
 * @routine 08:D29B ObjectBreakOffC
 * WHAT IT DOES: As ObjectBreakOffB with look $9B9F (ObjectAdvance5DIfFlag)
 * and 4 frames.
 * HOW: As ObjectBreakOffA. Not reached in the traces so far.
 * Entry: as ObjectBreakOffA. Exit: as ObjectBreakOffA.
 */
void abi_ObjectBreakOffC(void)
{
    piece_look(0x9B9F, 1, 0x14);
    abi_ObjectVelocityFromRecord();
    piece_drop();
    piece_fall4();
}

/*
 * @routine 08:D2CF ObjectBreakOffD
 * WHAT IT DOES: As ObjectBreakOffC with look $9B8F.
 * HOW: As ObjectBreakOffA.
 * Entry: as ObjectBreakOffA. Exit: as ObjectBreakOffA.
 */
void abi_ObjectBreakOffD(void)
{
    piece_look(0x9B8F, 1, 0x14);
    abi_ObjectVelocityFromRecord();
    piece_drop();
    piece_fall4();
}

/*
 * @routine 08:D303 ObjectBreakOffE
 * WHAT IT DOES: As ObjectBreakOffC with look $9B79.
 * HOW: As ObjectBreakOffA.
 * Entry: as ObjectBreakOffA. Exit: as ObjectBreakOffA.
 */
void abi_ObjectBreakOffE(void)
{
    piece_look(0x9B79, 1, 0x14);
    abi_ObjectVelocityFromRecord();
    piece_drop();
    piece_fall4();
}

/* $08:D36D: look $9B63 (advanced), +$5F = $1F58, $28 lower, falls. */
static void break_off_f(void)
{
    uint32_t b = db();

    sta16_imm(b | (uint16_t)(0x205D + cpu.x), 0x9B63);
    abi_ObjectAdvance5DIfFlag();
    lda8((uint8_t)(rd8(b | (uint16_t)(0x2024 + cpu.x)) | 0x08));
    wr8(b | (uint16_t)(0x2024 + cpu.x), (uint8_t)cpu.a);
    lda8(0x00);
    wr8(b | (uint16_t)(0x203A + cpu.x), 0x00);
    op_lda8(b | 0x1F58);
    wr8(b | (uint16_t)(0x205F + cpu.x), (uint8_t)cpu.a);
    piece_drop();
    piece_fall4();
}

/*
 * @routine 08:D337 ObjectBreakOffContact
 * WHAT IT DOES: A piece hit on contact: look $9B63 with +$5F = $6E, $28
 * lower, falls (4 frames).
 * HOW: $1F58 = $6E, then the tail of ObjectBreakOffF.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectPieceFall.
 */
void abi_ObjectBreakOffContact(void)
{
    lda8(0x6E);
    wr8(db() | 0x1F58, 0x6E);
    break_off_f();
}

/*
 * @routine 08:D33F ObjectBreakOffF
 * WHAT IT DOES: As ObjectBreakOffC with look $9B63 and +$5F = half the
 * speed of its parent's (+$27) record ($4A, its size) + $14, at most $78.
 * HOW: The halving is CMP #$8000, ROR A; $1F58 holds +$5F.
 * Entry: as ObjectBreakOffA. Exit: as ObjectBreakOffA.
 */
void abi_ObjectBreakOffF(void)
{
    uint32_t b = db();
    uint16_t x = cpu.x, w;

    cpu.p &= (uint8_t)~P_M;
    cpu.x = rd16(b | (uint16_t)(0x2027 + cpu.x));
    cpu.x = rd16(b | (uint16_t)(0x2022 + cpu.x));
    w = abs16(rd16(0x70004A + cpu.x));
    cpu.a = (uint16_t)((w >> 1) | (w & 0x8000));
    op_add16(0x0014);
    op_cmp16(0x0078);
    if (!(cpu.p & P_N))
        op_lda16_imm(0x0078);
    cpu.p |= P_M;
    wr8(b | 0x1F58, (uint8_t)cpu.a);
    cpu.x = x;
    cpu.p = nz16(cpu.p, x);
    abi_ObjectVelocityFromRecord();
    break_off_f();
}

/*
 * @routine 08:D39F ObjectBreakOffG
 * WHAT IT DOES: As ObjectBreakOffD with look $9B56, $28 lower before the
 * scaled velocity (ObjectVelocityFromRecordScaled).
 * HOW: As ObjectBreakOffA.
 * Entry: as ObjectBreakOffA. Exit: as ObjectBreakOffA.
 */
void abi_ObjectBreakOffG(void)
{
    piece_look(0x9B56, 1, 0x14);
    piece_drop();
    abi_ObjectVelocityFromRecordScaled();
    piece_fall4();
}

/*
 * @routine 08:D3D3 ObjectBreakOffH
 * WHAT IT DOES: As ObjectBreakOffG with look $9B40 and +$5F = $0A.
 * HOW: As ObjectBreakOffA.
 * Entry: as ObjectBreakOffA. Exit: as ObjectBreakOffA.
 */
void abi_ObjectBreakOffH(void)
{
    piece_look(0x9B40, 1, 0x0A);
    piece_drop();
    abi_ObjectVelocityFromRecordScaled();
    piece_fall4();
}

/*
 * @routine 08:D428 ObjectPieceFallStart
 * WHAT IT DOES: A piece's first frame: falls for 6 frames
 * (ObjectPieceFall), look $997C, +$24 bit 3, +$3A = 0, +$5F = $0A.
 * HOW: Falls into ObjectPieceFall. No caller found: not reached in the
 * traces so far.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectPieceFall.
 */
void abi_ObjectPieceFallStart(void)
{
    uint32_t b = db();

    piece_handler(0x06);
    lda8(0x06);
    wr8(b | (uint16_t)(0x2049 + cpu.x), 0x06);
    piece_look(0x997C, 0, 0x0A);
    abi_ObjectPieceFall();
}

/*
 * @routine 08:D460 ObjectPieceSpray
 * WHAT IT DOES: A spray piece's first frame (from ObjectSpawnChanceD460):
 * falls for 6 frames (ObjectPieceFall), look $997C, +$24 bit 3, +$3A = 0,
 * +$5F = $0A; its offset ($00, $00, random -$28 to -$19) turned by its
 * parent's (+$27) angles +$14 (RotateXZFrom08) and +$15 (RotateXZ),
 * doubled and added to its position; its parent's record velocity scaled
 * (ObjectVelocityFromRecordScaled), Y - $14, ObjectAdvance5DIfFlag; then
 * its first fall.
 * HOW: The offset in direct page $02/$04, $08/$0A, $94/$05F2 with their
 * sign bytes; the copies between the turns are 8-bit.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectPieceFall.
 */
void abi_ObjectPieceSpray(void)
{
    uint32_t b = db();
    uint16_t w;
    uint8_t v;

    piece_handler(0x06);
    lda8(0x06);
    wr8(b | (uint16_t)(0x2049 + cpu.x), 0x06);
    abi_RandomStepLong();
    v = (uint8_t)(cpu.a & 0x0F);
    lda8(v);
    wr8(b | 0x1F4C, v);
    cpu.p &= (uint8_t)~P_M;
    op_lda16(b | 0x1F4C);
    op_add16(0xFFD8);
    wr16(b | 0x1F4C, cpu.a);
    cpu.p |= P_M;
    piece_look(0x997C, 0, 0x0A);
    cpu.y = rd16(b | (uint16_t)(0x2027 + cpu.x));
    cpu.p = nz16(cpu.p, cpu.y);
    lda8(0x00);
    wr8(0x000002, 0x00);
    wr8(0x000004, 0x00);
    wr8(0x000005, 0x00);
    lda8(0x00);
    wr8(0x000008, 0x00);
    wr8(0x00000A, 0x00);
    wr8(0x00000B, 0x00);
    op_lda8(b | 0x1F4C);
    v = (uint8_t)cpu.a;
    wr8(0x000094, v);
    wr8(b | 0x05F2, v);
    if (v & 0x80) {
        lda8(0xFF);
        wr8(b | 0x05F3, 0xFF);
    } else {
        wr8(b | 0x05F3, 0x00);
    }
    op_lda8(b | (uint16_t)(0x2014 + cpu.y));
    abi_RotateXZFrom08();
    op_lda8(0x000004);
    wr8(0x000002, (uint8_t)cpu.a);
    op_lda8(0x00000A);
    wr8(0x000008, (uint8_t)cpu.a);
    op_lda8(b | 0x05F2);
    wr8(0x000094, (uint8_t)cpu.a);
    op_lda8(b | (uint16_t)(0x2015 + cpu.y));
    abi_RotateXZ();
    cpu.p &= (uint8_t)~P_M;
    op_lda16(0x000004);
    op_asl16();
    wr16(0x000004, cpu.a);
    op_lda16(0x00000A);
    op_asl16();
    wr16(0x00000A, cpu.a);
    op_lda16(b | 0x05F2);
    op_asl16();
    wr16(b | 0x05F2, cpu.a);
    op_lda16(b | (uint16_t)(0x200C + cpu.x));
    op_add16(rd16(0x000004));
    wr16(b | (uint16_t)(0x200C + cpu.x), cpu.a);
    op_lda16(b | (uint16_t)(0x2010 + cpu.x));
    op_add16(rd16(b | 0x05F2));
    wr16(b | (uint16_t)(0x2010 + cpu.x), cpu.a);
    op_lda16(b | (uint16_t)(0x200E + cpu.x));
    op_add16(rd16(0x00000A));
    wr16(b | (uint16_t)(0x200E + cpu.x), cpu.a);
    cpu.p |= P_M;
    abi_ObjectVelocityFromRecordScaled();
    cpu.p &= (uint8_t)~P_M;
    w = rd16(b | (uint16_t)(0x2030 + cpu.x));
    cpu.a = w;
    op_add16(0xFFEC);
    wr16(b | (uint16_t)(0x2030 + cpu.x), cpu.a);
    cpu.p |= P_M;
    abi_ObjectAdvance5DIfFlag();
    abi_ObjectPieceFall();
}

void abi_RotateXY(void);
void abi_ObjectCopy14From(void);
void abi_ObjectBillboardTilt(void);
void abi_ObjectCopyPositionFrom(void);
void abi_ObjectCarRecordNew(void);
void abi_GsuRecordWord58(void);
void abi_GsuRecordWord88(void);
void abi_SoundPort0QueuePush(void);

/* LDA.L word,X (16-bit) with its low byte sign-extended (BIT #$80, ORA
   #$FF00 or AND #$00FF). */
static uint16_t sext_word(uint32_t at)
{
    uint16_t w = rd16(at);

    w = (w & 0x0080) ? (uint16_t)(w | 0xFF00) : (uint16_t)(w & 0x00FF);
    cpu.a = w;
    cpu.p = nz16(cpu.p, w);
    return w;
}

/* $08:E13C: +$24 bit 5 (hidden), +$36 = 3. */
static void attach_hide(void)
{
    uint32_t b = db();

    lda8((uint8_t)(rd8(b | (uint16_t)(0x2024 + cpu.x)) | 0x20));
    wr8(b | (uint16_t)(0x2024 + cpu.x), (uint8_t)cpu.a);
    lda8(0x03);
    wr8(b | (uint16_t)(0x2036 + cpu.x), 0x03);
}

/* $08:E14A: +$24 bit 6, +$36 = 3. */
static void attach_bit6(void)
{
    uint32_t b = db();

    lda8((uint8_t)(rd8(b | (uint16_t)(0x2024 + cpu.x)) | 0x40));
    wr8(b | (uint16_t)(0x2024 + cpu.x), (uint8_t)cpu.a);
    lda8(0x03);
    wr8(b | (uint16_t)(0x2036 + cpu.x), 0x03);
}

/* LDY +$06,X; whether the parent is gone or loose (its +$24 bit 3 or +$25
   bit 2): then the object hides ($08:E13C). */
static int attach_parent_gone(void)
{
    uint32_t b = db();

    cpu.y = rd16(b | (uint16_t)(0x2006 + cpu.x));
    cpu.p = nz16(cpu.p, cpu.y);
    lda8((uint8_t)(rd8(b | (uint16_t)(0x2024 + cpu.y)) & 0x08));
    if ((uint8_t)cpu.a) {
        attach_hide();
        return 1;
    }
    lda8((uint8_t)(rd8(b | (uint16_t)(0x2025 + cpu.y)) & 0x04));
    if ((uint8_t)cpu.a) {
        attach_hide();
        return 1;
    }
    return 0;
}

/* The offset in object X's velocity bytes (+$2E, +$30, +$32, via $1F58-
   $1F5A) as signed direct page words $04, $0A and $05F2, turned by parent
   Y's +$16 (RotateXY), +$14 (RotateXZFrom08) and +$15 (RotateXZ), shifted
   left `sh` times and added to the parent's position into object X's. */
static void offset_on_parent(int sh)
{
    uint32_t b = db();
    uint8_t v;
    int i;

    op_lda8(b | (uint16_t)(0x202E + cpu.x));
    wr8(b | 0x1F58, (uint8_t)cpu.a);
    op_lda8(b | (uint16_t)(0x2030 + cpu.x));
    wr8(b | 0x1F59, (uint8_t)cpu.a);
    op_lda8(b | (uint16_t)(0x2032 + cpu.x));
    wr8(b | 0x1F5A, (uint8_t)cpu.a);
    op_lda8(b | 0x1F58);
    v = (uint8_t)cpu.a;
    wr8(0x000002, v);
    wr8(0x000004, v);
    if (v & 0x80) {
        lda8(0xFF);
        wr8(0x000005, 0xFF);
    } else {
        wr8(0x000005, 0x00);
    }
    op_lda8(b | 0x1F59);
    v = (uint8_t)cpu.a;
    wr8(0x000008, v);
    wr8(0x00000A, v);
    if (v & 0x80) {
        lda8(0xFF);
        wr8(0x00000B, 0xFF);
    } else {
        wr8(0x00000B, 0x00);
    }
    op_lda8(b | 0x1F5A);
    v = (uint8_t)cpu.a;
    wr8(0x000094, v);
    wr8(b | 0x05F2, v);
    if (v & 0x80) {
        lda8(0xFF);
        wr8(b | 0x05F3, 0xFF);
    } else {
        wr8(b | 0x05F3, 0x00);
    }
    op_lda8(b | (uint16_t)(0x2016 + cpu.y));
    abi_RotateXY();
    op_lda8(0x000004);
    wr8(0x000002, (uint8_t)cpu.a);
    op_lda8(0x00000A);
    wr8(0x000008, (uint8_t)cpu.a);
    op_lda8(b | 0x05F2);
    wr8(0x000094, (uint8_t)cpu.a);
    op_lda8(b | (uint16_t)(0x2014 + cpu.y));
    abi_RotateXZFrom08();
    op_lda8(0x000004);
    wr8(0x000002, (uint8_t)cpu.a);
    op_lda8(0x00000A);
    wr8(0x000008, (uint8_t)cpu.a);
    op_lda8(b | 0x05F2);
    wr8(0x000094, (uint8_t)cpu.a);
    op_lda8(b | (uint16_t)(0x2015 + cpu.y));
    abi_RotateXZ();
    cpu.p &= (uint8_t)~P_M;
    op_lda16(0x000004);
    for (i = 0; i < sh; i++)
        op_asl16();
    wr16(0x000004, cpu.a);
    op_lda16(0x00000A);
    for (i = 0; i < sh; i++)
        op_asl16();
    wr16(0x00000A, cpu.a);
    op_lda16(b | 0x05F2);
    for (i = 0; i < sh; i++)
        op_asl16();
    wr16(b | 0x05F2, cpu.a);
    op_lda16(b | (uint16_t)(0x200C + cpu.y));
    op_add16(rd16(0x000004));
    wr16(b | (uint16_t)(0x200C + cpu.x), cpu.a);
    op_lda16(b | (uint16_t)(0x2010 + cpu.y));
    op_add16(rd16(b | 0x05F2));
    wr16(b | (uint16_t)(0x2010 + cpu.x), cpu.a);
    op_lda16(b | (uint16_t)(0x200E + cpu.y));
    op_add16(rd16(0x00000A));
    wr16(b | (uint16_t)(0x200E + cpu.x), cpu.a);
    cpu.p |= P_M;
}

/*
 * @routine 08:DCB4 ObjectAttachSpawn
 * WHAT IT DOES: Gives object X the attachments listed for the course
 * script's entry ($70:2392 indexes the list at $09:8090: a count, then five
 * bytes each): for each, a new object (kind $A040) with object X as its
 * +$06, its +$04, look, handler and bank from the five words at $09:8000 +
 * the entry's first word (+$04, $1F50, $1F52, +$1A, +$1C); with a look
 * ($1F50 nonzero) +$24 bit 3, +$3A = 0, +$5F = $1F52, +$5D = $1F50; and
 * the entry's three signed bytes as its velocity (+$2E, +$30, +$32).
 * HOW: The list index in X, the count on the stack; $1F4C keeps object X;
 * a failed spawn skips its entry.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; X = the object; Y
 * the last attachment (or as ObjectSpawn left it).
 */
void abi_ObjectAttachSpawn(void)
{
    uint32_t b = db();
    uint16_t t, i;
    uint8_t n;

    wr16(b | 0x1F4C, cpu.x);
    cpu.p &= (uint8_t)~P_M;
    op_lda16(0x702392);
    op_tax16();
    cpu.p |= P_M;
    op_lda8(0x098090 + cpu.x);
    n = (uint8_t)cpu.a;
    if (n) {
        cpu.x++;
        do {
            if (spawn(0xA040)) {
                i = cpu.x;
                cpu.p &= (uint8_t)~P_M;
                op_lda16(b | 0x1F4C);
                wr16(b | (uint16_t)(0x2006 + cpu.y), cpu.a);
                t = rd16(0x098090 + i);
                wr16(b | (uint16_t)(0x2004 + cpu.y), rd16(0x098000 + t));
                wr16(b | 0x1F50, rd16(0x098002 + t));
                wr16(b | 0x1F52, rd16(0x098004 + t));
                wr16(b | (uint16_t)(0x201A + cpu.y), rd16(0x098006 + t));
                cpu.p |= P_M;
                op_lda8(0x098008 + t);
                wr8(b | (uint16_t)(0x201C + cpu.y), (uint8_t)cpu.a);
                cpu.x = i;
                op_lda8(b | 0x1F50);
                cpu.a |= rd8(b | 0x1F51);
                cpu.p = nz8(cpu.p, (uint8_t)cpu.a);
                if ((uint8_t)cpu.a) {
                    lda8((uint8_t)(rd8(b | (uint16_t)(0x2024 + cpu.y)) | 0x08));
                    wr8(b | (uint16_t)(0x2024 + cpu.y), (uint8_t)cpu.a);
                    lda8(0x00);
                    wr8(b | (uint16_t)(0x203A + cpu.y), 0x00);
                    op_lda8(b | 0x1F52);
                    wr8(b | (uint16_t)(0x205F + cpu.y), (uint8_t)cpu.a);
                    cpu.p &= (uint8_t)~P_M;
                    op_lda16(b | 0x1F50);
                    wr16(b | (uint16_t)(0x205D + cpu.y), cpu.a);
                    cpu.p |= P_M;
                }
                cpu.p &= (uint8_t)~P_M;
                wr16(b | (uint16_t)(0x202E + cpu.y), sext_word(0x098092 + cpu.x));
                wr16(b | (uint16_t)(0x2030 + cpu.y), sext_word(0x098093 + cpu.x));
                wr16(b | (uint16_t)(0x2032 + cpu.y), sext_word(0x098094 + cpu.x));
                cpu.p |= P_M;
            }
            cpu.x = (uint16_t)(cpu.x + 5);
            n--;
        } while (n);
    }
    cpu.x = rd16(b | 0x1F4C);
    cpu.p = nz16(cpu.p, cpu.x);
}

/*
 * @routine 08:DDB4 ObjectAttachFollow
 * WHAT IT DOES: An attachment's frame: hidden ($08:E13C) when its parent
 * is gone or loose, or when the parent's record has +$1C bit 4; otherwise
 * placed at the parent's record position (+$1E, +$60, +$22) with its
 * heading (+$27 to +$14, +$25 to +$15), moved by its velocity
 * (ObjectApplyVelocity), rebucketed, +$24 bits 5 and 6 cleared, and bit 6
 * set again ($08:E14A) when the parent's +$24 has it.
 * HOW: Y = the attachment and X = the parent's record while copying.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; as the path leaves
 * it.
 */
void abi_ObjectAttachFollow(void)
{
    uint32_t b = db();
    uint16_t obj = cpu.x;

    if (attach_parent_gone())
        return;
    cpu.p &= (uint8_t)~P_M;
    op_lda16(b | (uint16_t)(0x2022 + cpu.y));
    cpu.y = obj;
    op_tax16();
    op_lda16(0x70001C + cpu.x);
    op_and16(0x0010);
    if (cpu.a) {
        cpu.p |= P_M;
        cpu.x = obj;
        cpu.p = nz16(cpu.p, obj);
        attach_hide();
        return;
    }
    op_lda16(0x70001E + cpu.x);
    wr16(b | (uint16_t)(0x200C + cpu.y), cpu.a);
    op_lda16(0x700022 + cpu.x);
    wr16(b | (uint16_t)(0x2010 + cpu.y), cpu.a);
    op_lda16(0x700060 + cpu.x);
    wr16(b | (uint16_t)(0x200E + cpu.y), cpu.a);
    cpu.p |= P_M;
    op_lda8(0x700027 + cpu.x);
    wr8(b | (uint16_t)(0x2014 + cpu.y), (uint8_t)cpu.a);
    op_lda8(0x700025 + cpu.x);
    wr8(b | (uint16_t)(0x2015 + cpu.y), (uint8_t)cpu.a);
    cpu.x = obj;
    cpu.p = nz16(cpu.p, obj);
    abi_ObjectApplyVelocity();
    abi_ObjectRebucket();
    lda8((uint8_t)(rd8(b | (uint16_t)(0x2024 + cpu.x)) & 0xDF));
    wr8(b | (uint16_t)(0x2024 + cpu.x), (uint8_t)cpu.a);
    lda8((uint8_t)(rd8(b | (uint16_t)(0x2024 + cpu.x)) & 0xBF));
    wr8(b | (uint16_t)(0x2024 + cpu.x), (uint8_t)cpu.a);
    cpu.y = rd16(b | (uint16_t)(0x2006 + cpu.x));
    cpu.p = nz16(cpu.p, cpu.y);
    lda8((uint8_t)(rd8(b | (uint16_t)(0x2024 + cpu.y)) & 0x40));
    if ((uint8_t)cpu.a)
        attach_bit6();
}

/*
 * @routine 08:DD8D ObjectAttachStart
 * WHAT IT DOES: An attachment's first frame: handler $08:DDB4
 * (ObjectAttachFollow), its Y velocity's low byte sign-extended and
 * doubled; then its first frame.
 * HOW: Falls into ObjectAttachFollow.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectAttachFollow.
 */
void abi_ObjectAttachStart(void)
{
    uint32_t b = db();

    set_handler(cpu.x, 0xDDB4);
    cpu.p &= (uint8_t)~P_M;
    sext_word(b | (uint16_t)(0x2030 + cpu.x));
    op_asl16();
    wr16(b | (uint16_t)(0x2030 + cpu.x), cpu.a);
    cpu.p |= P_M;
    abi_ObjectAttachFollow();
}

/*
 * @routine 08:DF4C ObjectAttachTurned
 * WHAT IT DOES: An attachment turned with its parent: hidden ($08:E13C)
 * when the parent is gone or loose; its heading +$15 = the parent's +$15 +
 * the high byte of the parent's record +$3A, +$14 and +$16 the parent's;
 * its position the parent's plus its velocity bytes turned by the
 * parent's angles, times 8; rebucketed; +$36 = 3.
 * HOW: RotateXY, RotateXZFrom08 and RotateXZ on the offset in direct page.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; A low byte 3.
 */
void abi_ObjectAttachTurned(void)
{
    uint32_t b = db();
    uint16_t x, y;
    uint8_t p;

    if (attach_parent_gone())
        return;
    x = cpu.x;
    y = cpu.y;
    cpu.p &= (uint8_t)~P_M;
    cpu.x = rd16(b | (uint16_t)(0x2022 + y));
    op_lda16(0x70003A + cpu.x);
    op_xba();
    cpu.p |= P_M;
    cpu.y = y;
    cpu.x = x;
    cpu.p = nz16(cpu.p, x);
    p = (uint8_t)(cpu.p & ~P_C);
    lda8(op_adc8((uint8_t)cpu.a, rd8(b | (uint16_t)(0x2015 + cpu.y)), &p));
    cpu.p = (uint8_t)((cpu.p & ~(P_C | P_V)) | (p & (P_C | P_V)));
    wr8(b | (uint16_t)(0x2015 + cpu.x), (uint8_t)cpu.a);
    op_lda8(b | (uint16_t)(0x2014 + cpu.y));
    wr8(b | (uint16_t)(0x2014 + cpu.x), (uint8_t)cpu.a);
    op_lda8(b | (uint16_t)(0x2016 + cpu.y));
    wr8(b | (uint16_t)(0x2016 + cpu.x), (uint8_t)cpu.a);
    offset_on_parent(3);
    abi_ObjectRebucket();
    lda8(0x03);
    wr8(b | (uint16_t)(0x2036 + cpu.x), 0x03);
}

/* PHA, REP #$20, TXA, STA +$06,Y, SEP #$20, PLA: object X as the new
   object's parent. */
static void set_child_of_x(void)
{
    wr16(db() | (uint16_t)(0x2006 + cpu.y), cpu.x);
}

/* One spark (kind $A1E4, handler $08:E048, parent X) with velocity
   (vx, $0A, vz); none when the spawn fails. */
static void spark(uint16_t vx, uint16_t vz)
{
    uint32_t b = db();

    if (!spawn(0xA1E4))
        return;
    set_child_of_x();
    set_handler(cpu.y, 0xE048);
    sta16_imm(b | (uint16_t)(0x202E + cpu.y), vx);
    sta16_imm(b | (uint16_t)(0x2030 + cpu.y), 0x000A);
    sta16_imm(b | (uint16_t)(0x2032 + cpu.y), vz);
}

/*
 * @routine 08:DE31 ObjectSparksStart
 * WHAT IT DOES: Gives object X four sparks (kind $A1E4, handler $08:E048
 * ObjectSparkStart) at offsets (-$1E or $1E, $0A, $3C or -$14), its own
 * handler $08:DF4C (ObjectAttachTurned); then its first frame.
 * HOW: Falls into ObjectAttachTurned; a failed spawn skips its spark.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectAttachTurned.
 */
void abi_ObjectSparksStart(void)
{
    set_handler(cpu.x, 0xDF4C);
    spark(0xFFE2, 0x003C);
    spark(0x001E, 0x003C);
    spark(0xFFE2, 0xFFEC);
    spark(0x001E, 0xFFEC);
    abi_ObjectAttachTurned();
}

/*
 * @routine 08:E073 ObjectSparkStep
 * WHAT IT DOES: A spark's frame: its position its parent's plus its
 * velocity bytes turned by the parent's angles, times 8; the parent's
 * +$14-$16 copied (ObjectCopy14From); tilted toward the camera
 * (ObjectBillboardTilt); rebucketed; +$36 = 3.
 * HOW: As ObjectAttachTurned without the heading's add.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; A low byte 3.
 */
void abi_ObjectSparkStep(void)
{
    uint32_t b = db();

    cpu.y = rd16(b | (uint16_t)(0x2006 + cpu.x));
    cpu.p = nz16(cpu.p, cpu.y);
    offset_on_parent(3);
    abi_ObjectCopy14From();
    cpu.y = cpu.x;
    cpu.p = nz16(cpu.p, cpu.y);
    abi_ObjectBillboardTilt();
    abi_ObjectRebucket();
    lda8(0x03);
    wr8(b | (uint16_t)(0x2036 + cpu.x), 0x03);
}

/*
 * @routine 08:E048 ObjectSparkStart
 * WHAT IT DOES: A spark's first frame: +$24 bit 3, +$3A = 0, +$5F = $6E,
 * handler $08:E073 (ObjectSparkStep), +$29 = $9BBF; then its first step.
 * HOW: Falls into ObjectSparkStep.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectSparkStep.
 */
void abi_ObjectSparkStart(void)
{
    uint32_t b = db();

    lda8((uint8_t)(rd8(b | (uint16_t)(0x2024 + cpu.x)) | 0x08));
    wr8(b | (uint16_t)(0x2024 + cpu.x), (uint8_t)cpu.a);
    lda8(0x00);
    wr8(b | (uint16_t)(0x203A + cpu.x), 0x00);
    lda8(0x6E);
    wr8(b | (uint16_t)(0x205F + cpu.x), 0x6E);
    set_handler(cpu.x, 0xE073);
    sta16_imm(b | (uint16_t)(0x2029 + cpu.x), 0x9BBF);
    abi_ObjectSparkStep();
}

/*
 * @routine 08:E1C7 ObjectCarAttachedStep
 * WHAT IT DOES: A car carried on another: rebucketed, posed from its
 * record (ObjectPoseFromRecord) and its record marked (GsuRecordFlag4, the
 * code at $08:CD7E).
 * HOW: Three calls. Not reached in the traces so far.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as GsuRecordFlag4.
 */
void abi_ObjectCarAttachedStep(void)
{
    abi_ObjectRebucket();
    abi_ObjectPoseFromRecord();
    abi_GsuRecordFlag4();
}

/*
 * @routine 08:E158 ObjectCarAttachedStart
 * WHAT IT DOES: A car carried on its parent (+$06): at the parent's
 * position, $640 back and $1E0 up, a car record of its own
 * (ObjectCarRecordNew with A = 0) marked carried (+$1C bit 3), each
 * record's +$8E naming the other's record; handler $08:E1C7
 * (ObjectCarAttachedStep), run at once.
 * HOW: The records through X with the objects swapped in X and Y. Not
 * reached in the traces so far.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectCarAttachedStep.
 */
void abi_ObjectCarAttachedStart(void)
{
    uint32_t b = db();
    uint16_t obj = cpu.x, par;

    cpu.y = rd16(b | (uint16_t)(0x2006 + cpu.x));
    cpu.p = nz16(cpu.p, cpu.y);
    abi_ObjectCopyPositionFrom();
    cpu.p &= (uint8_t)~P_M;
    op_lda16(b | (uint16_t)(0x2010 + cpu.x));
    op_sub16(0x0640);
    wr16(b | (uint16_t)(0x2010 + cpu.x), cpu.a);
    op_lda16(b | (uint16_t)(0x200E + cpu.x));
    op_add16(0xFE20);
    wr16(b | (uint16_t)(0x200E + cpu.x), cpu.a);
    op_lda16_imm(0x0000);
    abi_ObjectCarRecordNew();
    cpu.p |= P_M;
    par = rd16(b | (uint16_t)(0x2006 + cpu.x));
    cpu.y = par;
    cpu.p &= (uint8_t)~P_M;
    cpu.x = rd16(b | (uint16_t)(0x2022 + obj));
    op_lda16(b | (uint16_t)(0x2022 + par));
    wr16(0x70008E + cpu.x, cpu.a);
    op_lda16(0x70001C + cpu.x);
    cpu.a |= 0x0008;
    cpu.p = nz16(cpu.p, cpu.a);
    wr16(0x70001C + cpu.x, cpu.a);
    cpu.x = rd16(b | (uint16_t)(0x2022 + par));
    op_lda16(b | (uint16_t)(0x2022 + obj));
    wr16(0x70008E + cpu.x, cpu.a);
    cpu.x = obj;
    cpu.y = par;
    cpu.p = (uint8_t)(nz16(cpu.p, par) | P_M);
    set_handler(cpu.x, 0xE1C7);
    abi_ObjectCarAttachedStep();
}

/*
 * @routine 08:E426 ObjectParentRecordBit8
 * WHAT IT DOES: Whether object X's parent's (+$06) record has +$40 bit 8,
 * in C.
 * HOW: 16-bit through the parent's +$22; X kept.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; Y = the parent; C
 * as above; X kept, N and Z from it.
 */
void abi_ObjectParentRecordBit8(void)
{
    uint32_t b = db();
    uint16_t x = cpu.x;

    cpu.y = rd16(b | (uint16_t)(0x2006 + cpu.x));
    cpu.p &= (uint8_t)~P_M;
    cpu.x = rd16(b | (uint16_t)(0x2022 + cpu.y));
    op_lda16(0x700040 + cpu.x);
    op_and16(0x0100);
    cpu.p |= P_M;
    cpu.x = x;
    cpu.p = (uint8_t)((nz16(cpu.p, x) & ~P_C) | (cpu.a ? P_C : 0));
}

/*
 * @routine 08:E244 ObjectExhaustStep
 * WHAT IT DOES: An exhaust flame's frame by its state +$39: after its
 * start count +$4B, +$48 = the parent's record +$88 - 1; hidden (+$24 bit
 * 5) while the parent is gone or loose (+$25 bits 3, 4) or its record has
 * +$40 bit 8 (ObjectParentRecordBit8). State 1: a changed +$88 starts
 * state 2 for 8 frames; a drop in the parent's record +$58 of 2 to 9 since
 * the last ($1F2B, +$4A) starts state 3 for 8 frames with sound $3B.
 * State 2 shows frame +$88 - 1, state 3 frame 4 (+$18 with bit 7). While
 * +$49 counts down, every fourth frame it is at the parent's position,
 * moved by its velocity, +$24 bit 7 set when the parent's link chain's
 * end has bit 14 (there it is rebucketed at the link's +$08, +$0A, +$0C
 * offset from (0, -$64, $1F4)), else rebucketed; back to state 1 at 0.
 * +$36 = 3.
 * HOW: GsuRecordWord88 and GsuRecordWord58 read the parent's record; 8-bit
 * with 16-bit position adds.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; A low byte 3.
 */
void abi_ObjectExhaustStep(void)
{
    uint32_t b = db();
    uint8_t v, w, p;
    uint16_t t;

    op_lda8(b | (uint16_t)(0x204B + cpu.x));
    if ((uint8_t)cpu.a) {
        v = (uint8_t)(rd8(b | (uint16_t)(0x204B + cpu.x)) - 1);
        wr8(b | (uint16_t)(0x204B + cpu.x), v);
        abi_GsuRecordWord88();
        wr8(b | (uint16_t)(0x2048 + cpu.x), (uint8_t)cpu.a);
    }
    cpu.y = rd16(b | (uint16_t)(0x2006 + cpu.x));
    cpu.p = nz16(cpu.p, cpu.y);
    lda8((uint8_t)(rd8(b | (uint16_t)(0x2025 + cpu.y)) & 0x08));
    if ((uint8_t)cpu.a)
        goto hide;
    lda8((uint8_t)(rd8(b | (uint16_t)(0x2025 + cpu.y)) & 0x10));
    if ((uint8_t)cpu.a)
        goto hide;
    abi_ObjectParentRecordBit8();
    if (cpu.p & P_C)
        goto hide;
    op_lda8(b | (uint16_t)(0x2039 + cpu.x));
    if ((uint8_t)cpu.a == 0x01) {
        abi_GsuRecordWord88();
        v = (uint8_t)cpu.a;
        w = rd8(b | (uint16_t)(0x2048 + cpu.x));
        cpu.p = (uint8_t)((nz8(cpu.p, (uint8_t)(v - w)) & ~P_C) | (v >= w ? P_C : 0));
        if (v != w) {
            wr8(b | (uint16_t)(0x2048 + cpu.x), v);
            lda8(0x02);
            wr8(b | (uint16_t)(0x2039 + cpu.x), 0x02);
            lda8(0x08);
            wr8(b | (uint16_t)(0x2049 + cpu.x), 0x08);
        }
        abi_GsuRecordWord58();
        v = (uint8_t)cpu.a;
        wr8(b | 0x1F2B, v);
        if (!(cpu.p & P_C)) {
            p = (uint8_t)(cpu.p | P_C);
            w = op_sbc8(v, rd8(b | (uint16_t)(0x204A + cpu.x)), &p);
            cpu.p = p;
            if (w & 0x80) {
                w = (uint8_t)((w ^ 0xFF) + 1);
                cpu.p = (uint8_t)((nz8(cpu.p, (uint8_t)(w - 0x02)) & ~P_C) | (w >= 0x02 ? P_C : 0));
                if ((uint8_t)(w - 0x02) & 0x80)
                    goto state2;
                cpu.p = (uint8_t)((nz8(cpu.p, (uint8_t)(w - 0x0A)) & ~P_C) | (w >= 0x0A ? P_C : 0));
                if ((uint8_t)(w - 0x0A) & 0x80) {
                    lda8(0x03);
                    wr8(b | (uint16_t)(0x2039 + cpu.x), 0x03);
                    lda8(0x08);
                    wr8(b | (uint16_t)(0x2049 + cpu.x), 0x08);
                    lda8(0x3B);
                    abi_SoundPort0QueuePush();
                }
            }
        }
        op_lda8(b | 0x1F2B);
        wr8(b | (uint16_t)(0x204A + cpu.x), (uint8_t)cpu.a);
    }
state2:
    op_lda8(b | (uint16_t)(0x2039 + cpu.x));
    if ((uint8_t)cpu.a == 0x02) {
        abi_GsuRecordWord88();
        wr8(b | 0x1F58, (uint8_t)cpu.a);
        v = (uint8_t)(rd8(b | 0x1F58) | 0x80);
        lda8(v);
        wr8(b | (uint16_t)(0x2018 + cpu.x), v);
    }
    op_lda8(b | (uint16_t)(0x2039 + cpu.x));
    if ((uint8_t)cpu.a == 0x03) {
        lda8(0x84);
        wr8(b | (uint16_t)(0x2018 + cpu.x), 0x84);
        abi_GsuRecordWord58();
        wr8(b | (uint16_t)(0x204A + cpu.x), (uint8_t)cpu.a);
    }
    op_lda8(b | (uint16_t)(0x2049 + cpu.x));
    v = (uint8_t)cpu.a;
    if (!v)
        goto hide;
    v--;
    lda8(v);
    if (!v) {
        wr8(b | (uint16_t)(0x2049 + cpu.x), 0x00);
        lda8(0x01);
        wr8(b | (uint16_t)(0x2039 + cpu.x), 0x01);
        goto hide;
    }
    wr8(b | (uint16_t)(0x2049 + cpu.x), v);
    lda8((uint8_t)(v & 0x03));
    if (!(uint8_t)cpu.a)
        goto hide;
    lda8((uint8_t)(rd8(b | (uint16_t)(0x2024 + cpu.x)) & 0xDF));
    wr8(b | (uint16_t)(0x2024 + cpu.x), (uint8_t)cpu.a);
    cpu.y = rd16(b | (uint16_t)(0x2006 + cpu.x));
    cpu.p = nz16(cpu.p, cpu.y);
    abi_ObjectCopyPositionFrom();
    abi_ObjectApplyVelocity();
    lda8((uint8_t)(rd8(b | (uint16_t)(0x2024 + cpu.x)) & 0x7F));
    wr8(b | (uint16_t)(0x2024 + cpu.x), (uint8_t)cpu.a);
    cpu.p &= (uint8_t)~P_M;
    cpu.y = rd16(b | (uint16_t)(0x204F + cpu.y));
    cpu.y = rd16(b | (uint16_t)(0x204F + cpu.y));
    wr16(b | 0x1F4C, rd16(b | (uint16_t)(0x0008 + cpu.y)));
    wr16(b | 0x1F4E, rd16(b | (uint16_t)(0x000A + cpu.y)));
    wr16(b | 0x1F50, rd16(b | (uint16_t)(0x000C + cpu.y)));
    op_lda16(b | cpu.y);
    op_and16(0x4000);
    if (!cpu.a) {
        cpu.p |= P_M;
        abi_ObjectRebucket();
    } else {
        sta16_imm(b | (uint16_t)(0x200C + cpu.x), 0x0000);
        cpu.p &= (uint8_t)~P_M;
        op_lda16_imm(0xFF9C);
        wr16(b | (uint16_t)(0x200E + cpu.x), 0xFF9C);
        op_lda16_imm(0x01F4);
        wr16(b | (uint16_t)(0x2010 + cpu.x), 0x01F4);
        cpu.p |= P_M;
        lda8((uint8_t)(rd8(b | (uint16_t)(0x2024 + cpu.x)) | 0x80));
        wr8(b | (uint16_t)(0x2024 + cpu.x), (uint8_t)cpu.a);
        for (t = 0; t < 3; t++) {
            static const uint16_t f[3] = {0x200C, 0x200E, 0x2010}, s[3] = {0x1F4C, 0x1F4E, 0x1F50};

            cpu.p &= (uint8_t)~P_M;
            op_lda16(b | (uint16_t)(f[t] + cpu.x));
            op_add16(rd16(b | s[t]));
            wr16(b | (uint16_t)(f[t] + cpu.x), cpu.a);
            cpu.p |= P_M;
        }
        abi_ObjectRebucket();
        for (t = 0; t < 3; t++) {
            static const uint16_t f[3] = {0x200C, 0x200E, 0x2010}, s[3] = {0x1F4C, 0x1F4E, 0x1F50};

            cpu.p &= (uint8_t)~P_M;
            op_lda16(b | (uint16_t)(f[t] + cpu.x));
            op_sub16(rd16(b | s[t]));
            wr16(b | (uint16_t)(f[t] + cpu.x), cpu.a);
            cpu.p |= P_M;
        }
    }
    lda8(0x03);
    wr8(b | (uint16_t)(0x2036 + cpu.x), 0x03);
    return;
hide:
    attach_hide();
}

/*
 * @routine 08:E1D2 ObjectExhaustStart
 * WHAT IT DOES: An exhaust flame's first frame: released when its
 * parent's record has +$70 bit 3, or $70:26F2 lacks bit 8, or in HUD
 * modes 1-3; otherwise handler $08:E244 (ObjectExhaustStep), +$4A = the
 * parent's record +$58, +$49 = 0, its Y velocity's low byte
 * sign-extended and doubled, start count +$4B = $0A, state +$39 = 1; then
 * its first step.
 * HOW: Falls into ObjectExhaustStep; the release is ObjectRelease.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectExhaustStep or
 * ObjectRelease.
 */
void abi_ObjectExhaustStart(void)
{
    uint32_t b = db();
    uint16_t x = cpu.x;
    uint8_t v;

    cpu.p &= (uint8_t)~P_M;
    cpu.x = rd16(b | (uint16_t)(0x2006 + cpu.x));
    cpu.x = rd16(b | (uint16_t)(0x2022 + cpu.x));
    op_lda16(0x700070 + cpu.x);
    cpu.x = x;
    op_and16(0x0008);
    if (!cpu.a) {
        op_lda16(0x7026F2);
        op_and16(0x0100);
        if (cpu.a) {
            cpu.p |= P_M;
            op_lda8(b | 0x0D2B);
            v = (uint8_t)cpu.a;
            if (v != 0x01 && v != 0x02 && v != 0x03) {
                set_handler(cpu.x, 0xE244);
                abi_GsuRecordWord58();
                wr8(b | (uint16_t)(0x204A + cpu.x), (uint8_t)cpu.a);
                lda8(0x00);
                wr8(b | (uint16_t)(0x2049 + cpu.x), 0x00);
                cpu.p &= (uint8_t)~P_M;
                sext_word(b | (uint16_t)(0x2030 + cpu.x));
                op_asl16();
                wr16(b | (uint16_t)(0x2030 + cpu.x), cpu.a);
                cpu.p |= P_M;
                lda8(0x0A);
                wr8(b | (uint16_t)(0x204B + cpu.x), 0x0A);
                lda8(0x01);
                wr8(b | (uint16_t)(0x2039 + cpu.x), 0x01);
                abi_ObjectExhaustStep();
                return;
            }
        }
    }
    cpu.p |= P_M;
    abi_ObjectRelease();
}

void abi_SoundAtRateForLevel(void);
void abi_PlayerSound12(void);
void abi_PlayerSound0C(void);
void abi_PlayerSound23(void);
void abi_PlayerSound24(void);
void abi_PlayerSound0D(void);
void abi_PlayerSound0B(void);
void abi_PlayerStopHeldSound(void);
void abi_PlayerHoldSound32(void);
void abi_PlayerHoldSound33(void);
void abi_PlayerHoldSound34(void);
void abi_PlayerHoldSound35(void);
void abi_ObjectSpawnSparklePair(void);
void abi_ObjectSpawnAttached(void);
void abi_ObjectSpawnTrail(void);
void abi_ObjectSpawnD460UnlessFlag(void);

/* LDA $1A5D, AND #$FF (16-bit): the player in hand's sound latches are at
   $1A60 for player 2, $1A5E for player 1. */
static uint16_t latch_addr(void)
{
    uint32_t b = db();

    op_lda16(b | 0x1A5D);
    op_and16(0x00FF);
    return cpu.a ? 0x1A60 : 0x1A5E;
}

/* A latch bit and its sound: when `on` and the bit is clear, the sound
   (`snd`) and the bit set; when off and the bit is set, the sound and the
   bit cleared. 16-bit, the latch word re-chosen before the store. */
static void latch(uint16_t bit, int on, AbiFn snd)
{
    uint32_t b = db();
    uint16_t w;

    op_lda16(b | latch_addr());
    cpu.p = (uint8_t)((cpu.p & ~P_Z) | ((cpu.a & bit) ? 0 : P_Z));
    if (((cpu.a & bit) ? 1 : 0) == on)
        return;
    w = cpu.a;
    snd();
    w = on ? (uint16_t)(w | bit) : (uint16_t)(w & ~bit);
    cpu.p &= (uint8_t)~P_M;
    wr16(b | latch_addr(), w);
    cpu.a = w;
    cpu.p = nz16(cpu.p, w);
}

/* CMP #k / BMI on A (16-bit): whether A - k is negative. */
static int below16(uint16_t a, uint16_t k)
{
    cpu.p = (uint8_t)((nz16(cpu.p, (uint16_t)(a - k)) & ~P_C) | (a >= k ? P_C : 0));
    return ((uint16_t)(a - k) & 0x8000) != 0;
}

/* LDA #s, JSL SoundPort0QueuePush (16-bit). */
static void snd16(uint16_t s)
{
    op_lda16_imm(s);
    abi_SoundPort0QueuePush();
}

/*
 * @routine 08:C124 PlayerCarEvents
 * WHAT IT DOES: The player car X's per-frame events from its Super FX
 * record: unless its +$25 has bit 4 or the record +$40 bit 8: with $1AEC
 * bit 5, the engine sound at its speed (SoundAtRateForLevel) when one of
 * its parts' records (the +$3E/+$34 chain) is a loose wheel (+$2C bit 7,
 * +$46 bits 0 and 4), else the latch for record +$92 bit 5 (sounds $12 /
 * $0C). Then the latches for +$42 bit 10 (sounds $23 / $24) and for a
 * landing (+$42 bit 13 with +$40 bit 11, not bit 8: sounds $0D / $0B); the
 * held sound by +$57 (stop, $32-$35); a hit by +$94 (sounds $54-$57 by
 * size, sparks from $50 on); sounds $0A / $0D for +$2C bits 8 / 9; a
 * scrape by -$62 (sounds $16-$19, from $46 on +$25 bit 5 and an attached
 * object); a crash by +$40 bit 0 (the code at $08:A034 with A = 1, then
 * unless it says so (C) +$25 bit 4 and a burst for one player), bit 6
 * (+$25 bit 5, sparks, $08:A034 with A = 0) or bit 5 ($08:A034 with A =
 * 1, a burst); unless the car's +$24 has bit 6, trails for +$40 bit 1
 * (and a spray chance) or, 14 in 256, for +$42 bit 10; a record +$82 of
 * $199 or more latches bit 5 once ($00:1FFE = $11, sound $3D) and may
 * spray. Last its +$24 bit 5 follows +$25 bits 2 and 7 and bit 6 the
 * link chain's end (bit 14).
 * HOW: The car in $1F18; the record in X; the latches through $1A5D; the
 * code at $08:A034 runs as original code.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; X = the object.
 */
void abi_PlayerCarEvents(void)
{
    uint32_t b = db();
    uint16_t obj = cpu.x, rec, w, x;
    uint8_t v;
    int found;

    wr16(b | 0x1F18, obj);
    cpu.p &= (uint8_t)~P_M;
    cpu.y = obj;
    op_lda16(b | (uint16_t)(0x2022 + obj));
    op_tax16();
    rec = cpu.x;
    op_lda16(b | (uint16_t)(0x2025 + cpu.y));
    if (cpu.a & 0x0010)
        goto tail;
    op_lda16(0x700040 + cpu.x);
    if (!(cpu.a & 0x0100)) {
        op_lda16(b | 0x1AEC);
        if (cpu.a & 0x0020) {
            wr16(b | 0x1F4C, 0x0000);
            op_lda16(0x70003E + cpu.x);
            found = 0;
            do {
                op_tax16();
                op_lda16(0x70002C + cpu.x);
                if (cpu.a & 0x0080) {
                    op_lda16(0x700046 + cpu.x);
                    if ((cpu.a & 0x0001) && (cpu.a & 0x0010)) {
                        op_lda16_imm(0x0001);
                        wr16(b | 0x1F4C, 0x0001);
                    }
                }
                op_lda16(0x700034 + cpu.x);
            } while (cpu.a);
            (void)found;
            cpu.x = rec;
            op_lda16(b | 0x1F4C);
            if (cpu.a) {
                op_lda16(b | 0x0D62);
                op_and16(0x00FF);
                if (!cpu.a) {
                    op_lda16_imm(0x0010);
                    wr16(0x7026EE, 0x0010);
                }
                op_lda16(0x70004A + cpu.x);
                cpu.a = abs16(cpu.a);
                abi_SoundAtRateForLevel();
            }
        } else {
            op_lda16(0x700092 + cpu.x);
            if (cpu.a & 0x0020)
                latch(0x0004, 1, abi_PlayerSound12);
            else
                latch(0x0004, 0, abi_PlayerSound0C);
        }
    }
    cpu.p &= (uint8_t)~P_M;
    op_lda16(0x700042 + cpu.x);
    if (cpu.a & 0x0400)
        latch(0x0001, 1, abi_PlayerSound23);
    else
        latch(0x0001, 0, abi_PlayerSound24);
    op_lda16(0x700042 + cpu.x);
    op_lda16(0x700042 + cpu.x);
    found = 0;
    if (cpu.a & 0x2000) {
        op_lda16(0x700040 + cpu.x);
        if ((cpu.a & 0x0800) && !(cpu.a & 0x0100))
            found = 1;
    }
    latch(0x0002, found, found ? abi_PlayerSound0D : abi_PlayerSound0B);
    /* $08:C304: the held sound by the record's +$57. */
    cpu.p |= P_M;
    op_lda8(0x700057 + cpu.x);
    v = (uint8_t)cpu.a;
    wr8(b | 0x1F5E, v);
    if (!(v & 0x80)) {
        if ((uint8_t)(v - 0x01) & 0x80)
            abi_PlayerStopHeldSound();
        else if ((uint8_t)(v - 0x02) & 0x80)
            abi_PlayerHoldSound32();
        else if ((uint8_t)(v - 0x04) & 0x80)
            abi_PlayerHoldSound33();
        else if ((uint8_t)(v - 0x05) & 0x80)
            abi_PlayerHoldSound34();
        else if ((uint8_t)(v - 0x64) & 0x80)
            abi_PlayerHoldSound35();
    }
    lda8(v);
    cpu.p &= (uint8_t)~P_M;
    op_lda16(0x700094 + cpu.x);
    w = cpu.a;
    if (w && !(w & 0x8000)) {
        if (!below16(w, 0x0032)) {
            if (below16(w, 0x01F4))
                snd16(0x0054);
            else if (below16(w, 0x0320))
                snd16(0x0055);
            else if (below16(w, 0x044C))
                snd16(0x0056);
            else if (below16(w, 0x07D0))
                snd16(0x0057);
        }
        cpu.a = w;
        if (!below16(w, 0x0032)) {
            x = cpu.x;
            cpu.p |= P_M;
            cpu.x = rd16(b | 0x1F18);
            abi_ObjectSpawnSparklePair();
            cpu.p &= (uint8_t)~P_M;
            cpu.x = x;
        }
    }
    op_lda16(0x70002C + cpu.x);
    if (cpu.a & 0x0100)
        snd16(0x000A);
    else if (cpu.a & 0x0200)
        snd16(0x000D);
    op_lda16(0x700062 + cpu.x);
    if (cpu.a & 0x8000) {
        w = (uint16_t)((cpu.a ^ 0xFFFF) + 1);
        if (!below16(w, 0x0041)) {
            if (below16(w, 0x0050))
                snd16(0x0016);
            else if (below16(w, 0x005A))
                snd16(0x0017);
            else if (below16(w, 0x006E))
                snd16(0x0018);
            else if (below16(w, 0x03E8))
                snd16(0x0019);
        }
        if (!below16(w, 0x0046)) {
            uint16_t ysave = cpu.y;

            x = cpu.x;
            cpu.x = cpu.y;
            cpu.p |= P_M;
            lda8((uint8_t)(rd8(b | (uint16_t)(0x2025 + cpu.x)) | 0x20));
            wr8(b | (uint16_t)(0x2025 + cpu.x), (uint8_t)cpu.a);
            lda8(0x0A);
            wr8(b | 0x1F59, 0x0A);
            lda8(0x00);
            wr8(b | 0x1F5A, 0x00);
            abi_ObjectSpawnAttached();
            cpu.p &= (uint8_t)~P_M;
            cpu.y = ysave;
            cpu.x = x;
        }
    }
    op_lda16(0x700040 + cpu.x);
    if (cpu.a & 0x0001) {
        x = cpu.x;
        cpu.p |= P_M;
        cpu.x = rd16(b | 0x1F18);
        lda8(0x01);
        call_routine(0x08A034, 1);
        if (!(cpu.p & P_C)) {
            lda8((uint8_t)(rd8(b | (uint16_t)(0x2025 + cpu.x)) | 0x10));
            wr8(b | (uint16_t)(0x2025 + cpu.x), (uint8_t)cpu.a);
            op_lda8(b | 0x0D62);
            if (!(uint8_t)cpu.a)
                abi_ObjectBurstSpawn();
        }
        cpu.p &= (uint8_t)~P_M;
        cpu.x = x;
    } else {
        op_lda16(0x700040 + cpu.x);
        if (cpu.a & 0x0040) {
            x = cpu.x;
            cpu.p |= P_M;
            cpu.x = rd16(b | 0x1F18);
            lda8((uint8_t)(rd8(b | (uint16_t)(0x2025 + cpu.x)) | 0x20));
            wr8(b | (uint16_t)(0x2025 + cpu.x), (uint8_t)cpu.a);
            abi_ObjectSpawnSparklePair();
            lda8(0x00);
            call_routine(0x08A034, 1);
            cpu.p &= (uint8_t)~P_M;
            cpu.x = x;
        } else {
            op_lda16(0x700040 + cpu.x);
            if (cpu.a & 0x0020) {
                x = cpu.x;
                cpu.p |= P_M;
                cpu.x = rd16(b | 0x1F18);
                lda8(0x01);
                call_routine(0x08A034, 1);
                op_lda8(b | 0x0D62);
                if (!(uint8_t)cpu.a)
                    abi_ObjectBurstSpawn();
                cpu.p &= (uint8_t)~P_M;
                cpu.x = x;
            }
        }
    }
    /* $08:C4A2 */
    cpu.y = rd16(b | 0x1F18);
    op_lda16(b | (uint16_t)(0x2024 + cpu.y));
    if (cpu.a & 0x0040)
        goto tail;
    op_lda16(0x700040 + cpu.x);
    if (cpu.a & 0x0002) {
        x = cpu.x;
        cpu.p |= P_M;
        cpu.x = rd16(b | 0x1F18);
        abi_ObjectSpawnTrail();
        abi_ObjectSpawnD460UnlessFlag();
        cpu.p &= (uint8_t)~P_M;
        cpu.x = x;
    } else {
        op_lda16(0x700042 + cpu.x);
        if (cpu.a & 0x0400) {
            cpu.p |= P_M;
            abi_RandomStepLong();
            if ((uint8_t)cpu.a >= 0xF2) {
                x = cpu.x;
                cpu.x = rd16(b | 0x1F18);
                abi_ObjectSpawnTrail();
                cpu.x = x;
            }
            cpu.p &= (uint8_t)~P_M;
        }
    }
    cpu.p &= (uint8_t)~P_M;
    op_lda16(0x700082 + cpu.x);
    if (!below16(cpu.a, 0x0199)) {
        uint16_t at;

        cpu.p |= P_M;
        op_lda8(b | 0x1A5D);
        at = (uint8_t)cpu.a ? 0x1A60 : 0x1A5E;
        op_lda8(b | at);
        v = (uint8_t)cpu.a;
        if (!(v & 0x20)) {
            lda8(0x11);
            wr8(0x001FFE, 0x11);
            lda8(0x3D);
            abi_SoundPort0QueuePush();
            v |= 0x20;
            op_lda8(b | 0x1A5D);
            at = (uint8_t)cpu.a ? 0x1A60 : 0x1A5E;
            lda8(v);
            wr8(b | at, v);
        }
        x = cpu.x;
        cpu.x = rd16(b | 0x1F18);
        lda8((uint8_t)(rd8(b | (uint16_t)(0x2025 + cpu.x)) & 0x04));
        if (!(uint8_t)cpu.a) {
            abi_RandomStepLong();
            if ((uint8_t)cpu.a >= 0x4C)
                abi_ObjectSpawnD460UnlessFlag();
        }
        cpu.x = x;
    }
tail:
    /* $08:C554 */
    cpu.x = obj;
    cpu.p = (uint8_t)(nz16(cpu.p, obj) | P_M);
    v = rd8(b | (uint16_t)(0x2025 + cpu.x));
    lda8((uint8_t)(v & 0x04));
    if (!(v & 0x04)) {
        lda8((uint8_t)(rd8(b | (uint16_t)(0x2025 + cpu.x)) & 0x80));
        if (!(uint8_t)cpu.a) {
            lda8((uint8_t)(rd8(b | (uint16_t)(0x2024 + cpu.x)) & 0xDF));
            wr8(b | (uint16_t)(0x2024 + cpu.x), (uint8_t)cpu.a);
            goto link;
        }
    }
    lda8((uint8_t)(rd8(b | (uint16_t)(0x2024 + cpu.x)) | 0x20));
    wr8(b | (uint16_t)(0x2024 + cpu.x), (uint8_t)cpu.a);
link:
    cpu.y = rd16(b | (uint16_t)(0x204F + cpu.x));
    cpu.p &= (uint8_t)~P_M;
    cpu.y = rd16(b | (uint16_t)(0x204F + cpu.y));
    op_lda16(b | cpu.y);
    op_and16(0x4000);
    cpu.p |= P_M;
    if (cpu.a) {
        lda8((uint8_t)(rd8(b | (uint16_t)(0x2024 + cpu.x)) | 0x40));
        wr8(b | (uint16_t)(0x2024 + cpu.x), (uint8_t)cpu.a);
    } else {
        lda8((uint8_t)(rd8(b | (uint16_t)(0x2024 + cpu.x)) & 0xBF));
        wr8(b | (uint16_t)(0x2024 + cpu.x), (uint8_t)cpu.a);
    }
}

void abi_ObjectPlaceOnParent(void);
void abi_ObjectCountStep5F(void);

/*
 * @routine 08:E443 ObjectOffsetOnParent2
 * WHAT IT DOES: An attachment placed at its velocity bytes turned by its
 * parent's angles, times 2: hidden ($08:E13C) when the parent's +$24 has
 * bit 3 or 5; +$18 = $80; then +$24 bit 5 cleared and rebucketed.
 * HOW: As ObjectAttachTurned's placement with one shift. No caller found:
 * not reached in the traces so far.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; as ObjectRebucket.
 */
void abi_ObjectOffsetOnParent2(void)
{
    uint32_t b = db();

    cpu.y = rd16(b | (uint16_t)(0x2006 + cpu.x));
    cpu.p = nz16(cpu.p, cpu.y);
    lda8((uint8_t)(rd8(b | (uint16_t)(0x2024 + cpu.y)) & 0x08));
    if ((uint8_t)cpu.a) {
        attach_hide();
        return;
    }
    lda8((uint8_t)(rd8(b | (uint16_t)(0x2024 + cpu.y)) & 0x20));
    if ((uint8_t)cpu.a) {
        attach_hide();
        return;
    }
    lda8(0x80);
    wr8(b | (uint16_t)(0x2018 + cpu.x), 0x80);
    offset_on_parent(1);
    lda8((uint8_t)(rd8(b | (uint16_t)(0x2024 + cpu.x)) & 0xDF));
    wr8(b | (uint16_t)(0x2024 + cpu.x), (uint8_t)cpu.a);
    abi_ObjectRebucket();
}

/* $08:E682: on its parent (ObjectPlaceOnParent with X = the parent, Y =
   the lamp), rebucketed, +$24 bits 5 and 6 cleared, bit 6 again when the
   parent has it ($08:E14A); +$36 = 3. */
static void lamp_place(void)
{
    uint32_t b = db();

    cpu.y = rd16(b | (uint16_t)(0x2006 + cpu.x));
    swap_xy();
    abi_ObjectPlaceOnParent();
    cpu.x = cpu.y;
    cpu.p = nz16(cpu.p, cpu.x);
    abi_ObjectRebucket();
    lda8((uint8_t)(rd8(b | (uint16_t)(0x2024 + cpu.x)) & 0xDF));
    wr8(b | (uint16_t)(0x2024 + cpu.x), (uint8_t)cpu.a);
    lda8((uint8_t)(rd8(b | (uint16_t)(0x2024 + cpu.x)) & 0xBF));
    wr8(b | (uint16_t)(0x2024 + cpu.x), (uint8_t)cpu.a);
    cpu.y = rd16(b | (uint16_t)(0x2006 + cpu.x));
    cpu.p = nz16(cpu.p, cpu.y);
    lda8((uint8_t)(rd8(b | (uint16_t)(0x2024 + cpu.y)) & 0x40));
    if ((uint8_t)cpu.a) {
        attach_bit6();
        return;
    }
    lda8(0x03);
    wr8(b | (uint16_t)(0x2036 + cpu.x), 0x03);
}

void abi_ObjectLampDrop(void);
void abi_ObjectLampDropB(void);
void abi_ObjectLampDropBStart(void);

/* $08:E6BE: with the parent's +$25 bit 4 released; otherwise handler
   $08:E6DE (ObjectLampDrop), +$49 = 0, and its first frame. */
static void lamp_drop_start(void)
{
    uint32_t b = db();

    cpu.y = rd16(b | (uint16_t)(0x2006 + cpu.x));
    cpu.p = nz16(cpu.p, cpu.y);
    lda8((uint8_t)(rd8(b | (uint16_t)(0x2025 + cpu.y)) & 0x10));
    if ((uint8_t)cpu.a) {
        abi_ObjectReleaseLong();
        return;
    }
    set_handler(cpu.x, 0xE6DE);
    wr8(b | (uint16_t)(0x2049 + cpu.x), 0x00);
    abi_ObjectLampDrop();
}

/*
 * @routine 08:E653 ObjectLampOnParent
 * WHAT IT DOES: A lamp's frame on its parent (+$06): hidden ($08:E13C)
 * when the parent is gone (+$24 bit 3); when the parent is loose (+$25 bit
 * 2) it drops ($08:E6BE); with +$25 bit 5 it bounces ($08:E712,
 * ObjectLampDropBStart); with bit 6 it takes look $9AB1 and drops;
 * otherwise placed on the parent ($08:E682).
 * HOW: Byte tests through the data bank.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; as the path leaves
 * it.
 */
void abi_ObjectLampOnParent(void)
{
    uint32_t b = db();

    cpu.y = rd16(b | (uint16_t)(0x2006 + cpu.x));
    cpu.p = nz16(cpu.p, cpu.y);
    lda8((uint8_t)(rd8(b | (uint16_t)(0x2024 + cpu.y)) & 0x08));
    if ((uint8_t)cpu.a) {
        attach_hide();
        return;
    }
    lda8((uint8_t)(rd8(b | (uint16_t)(0x2025 + cpu.y)) & 0x04));
    if ((uint8_t)cpu.a) {
        lamp_drop_start();
        return;
    }
    lda8((uint8_t)(rd8(b | (uint16_t)(0x2025 + cpu.y)) & 0x20));
    if ((uint8_t)cpu.a) {
        abi_ObjectLampDropBStart();
        return;
    }
    lda8((uint8_t)(rd8(b | (uint16_t)(0x2025 + cpu.y)) & 0x40));
    if ((uint8_t)cpu.a) {
        sta16_imm(b | (uint16_t)(0x205D + cpu.x), 0x9AB1);
        lamp_drop_start();
        return;
    }
    lamp_place();
}

/* One step of a lamp's look animation: unless its count +$4B is 4, the
   count up one and its look +$5D the word for it in the table at
   $08:table (X 8-bit for the index). */
static void lamp_look_step(uint16_t table)
{
    uint32_t b = db();
    uint8_t n;
    uint16_t x = cpu.x;

    op_lda8(b | (uint16_t)(0x204B + cpu.x));
    n = (uint8_t)cpu.a;
    if (n == 0x04)
        return;
    n++;
    lda8(n);
    wr8(b | (uint16_t)(0x204B + cpu.x), n);
    cpu.x = (uint8_t)(n << 1);
    cpu.p &= (uint8_t)~P_M;
    op_lda16(0x080000 | (uint16_t)(table + cpu.x));
    cpu.x = x;
    wr16(b | (uint16_t)(0x205D + cpu.x), cpu.a);
    cpu.p |= P_M;
}

/*
 * @routine 08:E51E ObjectLampAnimate
 * WHAT IT DOES: A lamp's look by $1A5B bits 2-4: 4, 8, $0C, $1C step its
 * count +$4B up to 4 through the looks at $08:E57F, $08:E5AD, $08:E5DB,
 * $08:E609; $10 sets look $9AAF; anything else clears the count; then its
 * frame on the parent (ObjectLampOnParent).
 * HOW: Compare chain; the paths for $0C and $1C are not reached in the
 * traces so far.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectLampOnParent.
 */
void abi_ObjectLampAnimate(void)
{
    uint32_t b = db();
    uint8_t v;

    op_lda8(b | 0x1A5B);
    v = (uint8_t)(cpu.a & 0x1C);
    lda8(v);
    if (v == 0x08)
        lamp_look_step(0xE5AD);
    else if (v == 0x04)
        lamp_look_step(0xE57F);
    else if (v == 0x0C)
        lamp_look_step(0xE5DB);
    else if (v == 0x1C)
        lamp_look_step(0xE609);
    else if (v == 0x10)
        sta16_imm(b | (uint16_t)(0x205D + cpu.x), 0x9AAF);
    else {
        lda8(0x00);
        wr8(b | (uint16_t)(0x204B + cpu.x), 0x00);
    }
    abi_ObjectLampOnParent();
}

/*
 * @routine 08:E61C ObjectLampWait
 * WHAT IT DOES: A lamp waiting: with $1A5B bit 5 its count +$4B = 0 and
 * handler $08:E51E (ObjectLampAnimate), run at once; otherwise look $9AAF,
 * handler $08:E653 (ObjectLampOnParent), run at once.
 * HOW: Falls into the chosen handler.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as the handler leaves it.
 */
void abi_ObjectLampWait(void)
{
    uint32_t b = db();

    op_lda8(b | 0x1A5B);
    lda8((uint8_t)(cpu.a & 0x20));
    if ((uint8_t)cpu.a) {
        lda8(0x00);
        wr8(b | (uint16_t)(0x204B + cpu.x), 0x00);
        set_handler(cpu.x, 0xE51E);
        abi_ObjectLampAnimate();
        return;
    }
    sta16_imm(b | (uint16_t)(0x205D + cpu.x), 0x9AAF);
    set_handler(cpu.x, 0xE653);
    abi_ObjectLampOnParent();
}

/*
 * @routine 08:E613 ObjectLampStart
 * WHAT IT DOES: A lamp's first frame: its +$26 from its parent's; then as
 * ObjectLampWait.
 * HOW: Falls into ObjectLampWait.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectLampWait.
 */
void abi_ObjectLampStart(void)
{
    uint32_t b = db();

    cpu.y = rd16(b | (uint16_t)(0x2006 + cpu.x));
    cpu.p = nz16(cpu.p, cpu.y);
    op_lda8(b | (uint16_t)(0x2026 + cpu.y));
    wr8(b | (uint16_t)(0x2026 + cpu.x), (uint8_t)cpu.a);
    abi_ObjectLampWait();
}

/*
 * @routine 08:E6DE ObjectLampDrop
 * WHAT IT DOES: A dropping lamp: +$48 up $10; the count +$5F stepped
 * (ObjectCountStep5F with $1F58 = 8, $1F59 = $10, $1F5A = 5); when done,
 * +$48 = 0 and handler $08:E61C (ObjectLampWait); placed on its parent
 * either way.
 * HOW: Ends in the placement at $08:E682.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as the placement leaves it.
 */
void abi_ObjectLampDrop(void)
{
    uint32_t b = db();
    uint8_t p, v;

    v = rd8(b | (uint16_t)(0x2048 + cpu.x));
    p = (uint8_t)(cpu.p & ~P_C);
    v = op_adc8(v, 0x10, &p);
    cpu.p = p;
    lda8(v);
    wr8(b | (uint16_t)(0x2048 + cpu.x), v);
    lda8(0x08);
    wr8(b | 0x1F58, 0x08);
    lda8(0x10);
    wr8(b | 0x1F59, 0x10);
    lda8(0x05);
    wr8(b | 0x1F5A, 0x05);
    abi_ObjectCountStep5F();
    if (cpu.p & P_C) {
        lda8(0x00);
        wr8(b | (uint16_t)(0x2048 + cpu.x), 0x00);
        set_handler(cpu.x, 0xE61C);
    }
    lamp_place();
}

/*
 * @routine 08:E72C ObjectLampDropB
 * WHAT IT DOES: A bouncing lamp: the count +$5F stepped
 * (ObjectCountStep5F with $1F58 = 3, $1F59 = 6, $1F5A = $0A); when done,
 * +$49 = 0, handler $08:E61C (ObjectLampWait) and +$25 bit 0 cleared;
 * placed on its parent either way.
 * HOW: Ends in the placement at $08:E682. Not reached in the traces so
 * far.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as the placement leaves it.
 */
void abi_ObjectLampDropB(void)
{
    uint32_t b = db();

    lda8(0x03);
    wr8(b | 0x1F58, 0x03);
    lda8(0x06);
    wr8(b | 0x1F59, 0x06);
    lda8(0x0A);
    wr8(b | 0x1F5A, 0x0A);
    abi_ObjectCountStep5F();
    if (cpu.p & P_C) {
        wr8(b | (uint16_t)(0x2049 + cpu.x), 0x00);
        set_handler(cpu.x, 0xE61C);
        lda8((uint8_t)(rd8(b | (uint16_t)(0x2025 + cpu.x)) & 0xFE));
        wr8(b | (uint16_t)(0x2025 + cpu.x), (uint8_t)cpu.a);
    }
    lamp_place();
}

/*
 * @routine 08:E712 ObjectLampDropBStart
 * WHAT IT DOES: A lamp starts bouncing: handler $08:E72C
 * (ObjectLampDropB), +$49 = 0, +$25 bit 0; then its first frame.
 * HOW: Falls into ObjectLampDropB. Not reached in the traces so far.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectLampDropB.
 */
void abi_ObjectLampDropBStart(void)
{
    uint32_t b = db();

    set_handler(cpu.x, 0xE72C);
    wr8(b | (uint16_t)(0x2049 + cpu.x), 0x00);
    lda8((uint8_t)(rd8(b | (uint16_t)(0x2025 + cpu.x)) | 0x01));
    wr8(b | (uint16_t)(0x2025 + cpu.x), (uint8_t)cpu.a);
    abi_ObjectLampDropB();
}
