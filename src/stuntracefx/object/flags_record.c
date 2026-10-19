/*
 * flags_record.c: the objects of bank $08 whose Super FX record takes the
 * object's own flags (08:CA2D-CD83): their set-ups, the frame steps that
 * cycle or swing a picture frame (+$5A) and copy it to the record's +$12,
 * the record step that ORs the object's +$06 and +$27 words into the
 * record's +$16, and the step that only marks the record. Kept apart from
 * gsu_record.c so a change here reruns only the scripts that reach these
 * routines.
 */
#include "machine.h"
#include "cpu_ops.h"

void abi_GsuRecordNew422(void);
void abi_GsuRecordFlag4(void);
void abi_ObjectRecordFlag4Step(void);
void abi_ObjectFlagsRecordSetup(void);
static void flags_record_tail(void);

/* Object field `f` ($20xx) of object `o`, through the data bank. */
#define F(f, o) ((uint32_t)cpu.db << 16 | (uint16_t)((f) + (o)))
/* A work RAM byte through the data bank. */
#define W(a) ((uint32_t)cpu.db << 16 | (a))

/* LDA #v with M 8-bit. */
static void lda8(uint8_t v)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, v);
}

/* LDA addr, 8-bit; returns the byte. */
static uint8_t ld8(uint32_t addr)
{
    cyc(24);
    op_lda8(addr);
    return (uint8_t)cpu.a;
}

/* CMP with M 8-bit: N, Z, C from A - m. */
static void cmp8(uint8_t m)
{
    uint8_t a = (uint8_t)cpu.a;

    cpu.p = nz8(cpu.p, (uint8_t)(a - m));
    cpu.p = (uint8_t)(a >= m ? cpu.p | P_C : cpu.p & ~P_C);
}

/* INC or DEC addr, 8-bit: N, Z from the new byte. */
static void step8(uint32_t addr, int d)
{
    uint8_t v = rd8(addr);

    v = (uint8_t)(v + d);
    cyc(6);
    wr8(addr, v);
    cpu.p = nz8(cpu.p, v);
}

/* The handler of object X: +$1A = h, +$1C = $08 (16-bit then 8-bit
   immediate stores). */
static void handler08(uint16_t h)
{
    cpu.p &= (uint8_t)~P_M;
    cpu.a = h;
    cpu.p = nz16(cpu.p, h);
    cyc(76);
    wr16(F(0x201A, cpu.x), h);
    cpu.p |= P_M;
    cyc(68);
    lda8(0x08);
    wr8(F(0x201C, cpu.x), 0x08);
}

/* LDA +f, STA $1F58, LDA $1F58, ORA #$80, STA +$5A: the frame from field
   f, shown. */
static void frame_from(uint16_t f)
{
    uint8_t v;

    v = ld8(F(f, cpu.x));
    cyc(24);
    wr8(W(0x1F58), v);
    v = ld8(W(0x1F58));
    v |= 0x80;
    cyc(46);
    lda8(v);
    wr8(F(0x205A, cpu.x), v);
}

/* LDA #0, ORA #$80, STA +$5A: frame 0, shown. */
static void frame_zero(void)
{
    lda8(0x00);
    lda8(0x80);
    wr8(F(0x205A, cpu.x), 0x80);
}

/* 08:CB7E: the shown frame (+$5A, low seven bits, a 16-bit read) to the
   record's +$12 when the object has one, then ObjectRecordFlag4Step. The
   object in Y and the record in X while copying. */
static void frames_to_record(void)
{
    uint16_t obj = cpu.x, rec, v;

    cpu.y = obj;
    cpu.p = nz16(cpu.p, obj);
    cpu.p &= (uint8_t)~P_M;
    cyc(96);
    rec = rd16(F(0x2022, obj));
    cpu.a = rec;
    cpu.p = nz16(cpu.p, rec);
    if (rec) {
        cpu.x = rec;
        cyc(60);
        v = rd16(F(0x205A, obj));
        v &= 0x007F;
        cpu.a = v;
        cpu.p = nz16(cpu.p, v);
        cyc(56);
        wr16(0x700012 + rec, v);
    } else { cyc(22); }
    cpu.x = obj;
    cpu.p = nz16(cpu.p, obj);
    cpu.p |= P_M;
    (cyc(68), abi_ObjectRecordFlag4Step());
}

/* Every +$49 frames (+$4A counts them) the frame +$5A steps by d (1 or
   $FF) round the +$48 frames: ADC d, and unless that is negative ADC +$48;
   AND #$7F; at +$48 or more, SBC +$48; shown (ORA #$80). Then
   frames_to_record. */
static void frame_cycle(uint8_t d)
{
    uint8_t v, m;

    step8(F(0x204A, cpu.x), 1);
    cyc(6);
    ld8(F(0x204A, cpu.x));
    cyc(30);
    m = rd8(F(0x2049, cpu.x));
    cmp8(m);
    if (!(cpu.p & P_Z)) {
        (cyc(22), frames_to_record());
        return;
    }
    cyc(46);
    wr8(F(0x204A, cpu.x), 0x00);
    cyc(6);
    v = ld8(F(0x2048, cpu.x));
    cyc(24);
    wr8(W(0x1F58), v);
    cyc(6);
    v = ld8(F(0x205A, cpu.x));
    cpu.p &= (uint8_t)~P_C;
    v = op_adc8(v, d, &cpu.p);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    if (!(cpu.p & P_N)) {
        cpu.p &= (uint8_t)~P_C;
        cyc(84);
        m = rd8(W(0x1F58));
        v = op_adc8(v, m, &cpu.p);
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    } else { cyc(52); }
    v &= 0x7F;
    cyc(40);
    lda8(v);
    m = rd8(W(0x1F58));
    cyc(38);
    cmp8(m);
    if (cpu.p & P_C) {
        cpu.p |= P_C;
        cyc(16);
        m = rd8(W(0x1F58));
        v = op_sbc8(v, m, &cpu.p);
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    }
    v |= 0x80;
    cyc(30);
    lda8(v);
    wr8(F(0x205A, cpu.x), v);
    frames_to_record();
}

/*
 * @routine 08:CA2D ObjectStopMarked
 * WHAT IT DOES: Sets bit 0 of object X's +$09 and clears its handler
 * (+$1A = 0, +$1C = 0). Nothing in the game refers to it.
 * HOW: An 8-bit OR, then 16-bit and 8-bit zero stores.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; A = 0, Z set.
 */
void abi_ObjectStopMarked(void)
{
    uint8_t v;

    cyc(6);
    v = ld8(F(0x2009, cpu.x));
    v |= 0x01;
    cyc(46);
    lda8(v);
    wr8(F(0x2009, cpu.x), v);
    cpu.p &= (uint8_t)~P_M;
    cpu.a = 0x0000;
    cpu.p = nz16(cpu.p, 0x0000);
    cyc(76);
    wr16(F(0x201A, cpu.x), 0x0000);
    cpu.p |= P_M;
    cyc(68);
    lda8(0x00);
    wr8(F(0x201C, cpu.x), 0x00);
}

/*
 * @routine 08:CA45 ObjectFlagsRecordFrame
 * WHAT IT DOES: Shows frame +$48 (+$5A = it with bit 7, through $1F58),
 * then sets the object up as ObjectFlagsRecordSetup does. Placed by course
 * data in bank $0D.
 * HOW: Byte copies, then BRL to ObjectFlagsRecordSetup.
 * Entry: M 8-bit, X 16-bit, D = $0000; X = object. Exit: as
 * ObjectFlagsRecordSetup.
 */
void abi_ObjectFlagsRecordFrame(void)
{
    cyc(6);
    frame_from(0x2048);
    (cyc(30), abi_ObjectFlagsRecordSetup());
}

/*
 * @routine 08:CA56 ObjectFrameCycleSetup
 * WHAT IT DOES: Sets up an object whose picture cycles forwards through its
 * +$48 frames, one step every +$49 frames: handler $08:CA6F
 * (ObjectFrameCycleStep), frame 0 shown, then the flags record
 * (ObjectFlagsRecordSetup's end at $08:CD46). Nothing in the game refers
 * to it.
 * HOW: Stores, then BRL to $08:CD46.
 * Entry: M 8-bit, X 16-bit, D = $0000; X = object. Exit: as
 * ObjectFlagsRecord.
 */
void abi_ObjectFrameCycleSetup(void)
{
    handler08(0xCA6F);
    (cyc(62), frame_zero());
    (cyc(106), flags_record_tail());
}

/*
 * @routine 08:CA6F ObjectFrameCycleStep
 * WHAT IT DOES: The cycling object's frame: every +$49 frames (counted in
 * +$4A) its frame +$5A steps forwards round its +$48 frames; then the
 * shown frame goes to its record's +$12 and the record is marked
 * (ObjectRecordFlag4Step).
 * HOW: INC/CMP on the count; CLC, ADC #1, and unless negative CLC, ADC
 * +$48 (through $1F58); AND #$7F; SBC +$48 when not below it; the record
 * copy at $08:CB7E.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as GsuRecordFlag4.
 */
void abi_ObjectFrameCycleStep(void)
{
    cyc(30);
    frame_cycle(0x01);
}

/*
 * @routine 08:CAA2 ObjectFrameCycleBackSetup
 * WHAT IT DOES: As ObjectFrameCycleSetup for a picture that cycles
 * backwards: handler $08:CAC3 (ObjectFrameCycleBackStep), the last frame
 * (+$48 - 1) shown, then the flags record. Nothing in the game refers to
 * it.
 * HOW: Stores (DEC A for the last frame), then BRL to $08:CD46.
 * Entry: M 8-bit, X 16-bit, D = $0000; X = object. Exit: as
 * ObjectFlagsRecord.
 */
void abi_ObjectFrameCycleBackSetup(void)
{
    uint8_t v;

    handler08(0xCAC3);
    cyc(6);
    v = ld8(F(0x2048, cpu.x));
    v--;
    cyc(38);
    lda8(v);
    wr8(W(0x1F58), v);
    v = ld8(W(0x1F58));
    v |= 0x80;
    cyc(46);
    lda8(v);
    wr8(F(0x205A, cpu.x), v);
    (cyc(106), flags_record_tail());
}

/*
 * @routine 08:CAC3 ObjectFrameCycleBackStep
 * WHAT IT DOES: As ObjectFrameCycleStep with the frame stepping backwards
 * (ADC #$FF).
 * HOW: As ObjectFrameCycleStep.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as GsuRecordFlag4.
 */
void abi_ObjectFrameCycleBackStep(void)
{
    cyc(30);
    frame_cycle(0xFF);
}

/*
 * @routine 08:CAF6 ObjectFrameSwingSetup
 * WHAT IT DOES: Sets up an object whose picture swings between its first
 * and last frames: handler $08:CB15 (ObjectFrameSwingStep), frame 0 shown,
 * its pause (+$2E) from +$4A, then the flags record. Placed by course
 * data in banks $05 and $0D.
 * HOW: Stores, then BRL to $08:CD46.
 * Entry: M 8-bit, X 16-bit, D = $0000; X = object. Exit: as
 * ObjectFlagsRecord.
 */
void abi_ObjectFrameSwingSetup(void)
{
    uint8_t v;

    handler08(0xCB15);
    (cyc(62), frame_zero());
    cyc(6);
    v = ld8(F(0x204A, cpu.x));
    cyc(30);
    wr8(F(0x202E, cpu.x), v);
    (cyc(106), flags_record_tail());
}

/*
 * @routine 08:CB15 ObjectFrameSwingStep
 * WHAT IT DOES: The swinging object's frame: while its pause +$2E is not 0
 * it counts down; otherwise frame +$30 is shown and every +$49 frames
 * (counted in +$4B) it steps one frame towards the last (+$48 - 1, kept
 * in $1F59) or, with +$25 bit 0 set, towards frame 0; at the end the
 * direction (+$25 bit 0) flips and the pause restarts from +$4A. Then the
 * shown frame goes to its record's +$12 and the record is marked.
 * HOW: Byte INC/DEC/CMP; the set direction's case is reached by a JML to
 * $08:CB5F; the record copy at $08:CB7E is shared with the cycling steps.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as GsuRecordFlag4.
 */
void abi_ObjectFrameSwingStep(void)
{
    uint8_t v, m;

    cyc(6);
    v = ld8(F(0x202E, cpu.x));
    if (v) {
        cyc(46);
        step8(F(0x202E, cpu.x), -1);
        (cyc(22), frames_to_record());
        return;
    }
    cyc(28);
    frame_from(0x2030);
    cyc(6);
    v = ld8(F(0x2048, cpu.x));
    cyc(24);
    wr8(W(0x1F58), v);
    v--;
    cyc(38);
    lda8(v);
    wr8(W(0x1F59), v);
    cyc(30);
    step8(F(0x204B, cpu.x), 1);
    cyc(6);
    ld8(F(0x204B, cpu.x));
    cyc(30);
    m = rd8(F(0x2049, cpu.x));
    cmp8(m);
    if (!(cpu.p & P_Z)) {
        (cyc(22), frames_to_record());
        return;
    }
    cyc(46);
    wr8(F(0x204B, cpu.x), 0x00);
    cyc(6);
    v = ld8(F(0x2025, cpu.x));
    v &= 0x01;
    lda8(v);
    if (v) {
        cyc(70);
        v = ld8(F(0x205A, cpu.x));
        v &= 0x7F;
        cyc(60);
        lda8(v);
        cmp8(0x00);
        if (!(cpu.p & P_Z)) {
            cyc(18);
            step8(F(0x2030, cpu.x), -1);
            frames_to_record();
            return;
        }
    } else {
        cyc(44);
        v = ld8(F(0x205A, cpu.x));
        v &= 0x7F;
        cyc(40);
        lda8(v);
        m = rd8(W(0x1F59));
        cyc(28);
        cmp8(m);
        if (!(cpu.p & P_Z)) {
            cyc(18);
            step8(F(0x2030, cpu.x), 1);
            (cyc(22), frames_to_record());
            return;
        }
    }
    v = ld8(F(0x2025, cpu.x));
    v ^= 0x01;
    cyc(46);
    lda8(v);
    wr8(F(0x2025, cpu.x), v);
    cyc(6);
    v = ld8(F(0x204A, cpu.x));
    cyc(30);
    wr8(F(0x202E, cpu.x), v);
    (cyc(22), frames_to_record());
}

/*
 * @routine 08:CD7E ObjectRecordFlag4Step
 * WHAT IT DOES: A handler that only marks object X's record
 * (GsuRecordFlag4) each frame; also the tail other steps jump to.
 * HOW: JSL to GsuRecordFlag4.
 * Entry: as GsuRecordFlag4. Exit: as GsuRecordFlag4.
 */
void abi_ObjectRecordFlag4Step(void)
{
    (cyc(62), abi_GsuRecordFlag4());
}

/*
 * @routine 08:CD55 ObjectFlagsRecord
 * WHAT IT DOES: Gives object X a record from the second pool
 * (GsuRecordNew422), ORs the object's +$06 word and then its +$27 word into
 * the record's flags (+$16), and marks it (ObjectRecordFlag4Step). With no
 * room in the pool it returns at once. Also the +$1D pointer
 * ObjectFlagsRecordSetup leaves.
 * HOW: BPL on GsuRecordNew422's result (BRL to an RTL when negative); the
 * object in Y and the record in X while ORing, two read-OR-write passes
 * over +$16.
 * Entry: M 8-bit, X 16-bit, D = $0000; X = object. Exit: no room: as
 * GsuRecordNew422 (A = $00FF, N set); otherwise as GsuRecordFlag4.
 */
void abi_ObjectFlagsRecord(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint16_t obj, rec, v, w;

    abi_GsuRecordNew422();
    if (cpu.p & P_N)
        { cyc(32); return; }
    cpu.p &= (uint8_t)~P_M;
    obj = cpu.x;
    cpu.y = obj;
    cyc(88);
    rec = rd16(b | (uint16_t)(0x2022 + obj));
    cpu.x = rec;
    cyc(44);
    v = rd16(b | (uint16_t)(0x2006 + obj));
    cyc(32);
    w = rd16(0x700016 + rec);
    v |= w;
    cyc(32);
    wr16(0x700016 + rec, v);
    cyc(32);
    v = rd16(0x700016 + rec);
    cyc(30);
    w = rd16(b | (uint16_t)(0x2027 + obj));
    v |= w;
    cyc(32);
    wr16(0x700016 + rec, v);
    cpu.a = v;
    cpu.x = obj;
    cpu.p = (uint8_t)(nz16(cpu.p, obj) | P_M);
    (cyc(36), abi_ObjectRecordFlag4Step());
}

/* 08:CD46, the end of ObjectFlagsRecordSetup that other set-ups branch
   to: +$1D = $08:CD55 (ObjectFlagsRecord), then the record
   (ObjectFlagsRecord). */
static void flags_record_tail(void)
{
    cpu.p &= (uint8_t)~P_M;
    cpu.a = 0xCD55;
    cpu.p = nz16(cpu.p, 0xCD55);
    wr16(F(0x201D, cpu.x), 0xCD55);
    cpu.p |= P_M;
    cyc(68);
    lda8(0x08);
    wr8(F(0x201F, cpu.x), 0x08);
    abi_ObjectFlagsRecord();
}

/*
 * @routine 08:CD2D ObjectFlagsRecordSetup
 * WHAT IT DOES: Sets up an object whose record takes the object's flags:
 * +$27 = 0, handler $08:CD7E (ObjectRecordFlag4Step), +$1D = $08:CD55
 * (ObjectFlagsRecord), then its record (ObjectFlagsRecord). Entered as an
 * object's handler.
 * HOW: Stores (16-bit then 8-bit), then falls into ObjectFlagsRecord;
 * the +$1D stores at $08:CD46 are also where other set-ups branch to.
 * Entry: M 8-bit, X 16-bit, D = $0000; X = object. Exit: as
 * ObjectFlagsRecord.
 */
void abi_ObjectFlagsRecordSetup(void)
{
    cpu.p &= (uint8_t)~P_M;
    cpu.a = 0x0000;
    cpu.p = nz16(cpu.p, 0x0000);
    cyc(76);
    wr16(F(0x2027, cpu.x), 0x0000);
    cpu.p |= P_M;
    cyc(22);
    handler08(0xCD7E);
    (cyc(76), flags_record_tail());
}

/*
 * @routine 08:CD25 ObjectFlagsRecordSetup20
 * WHAT IT DOES: As ObjectFlagsRecordSetup, with bit 5 of object X's +$24
 * set first. Nothing in the game calls it.
 * HOW: An 8-bit OR into +$24, then falls into ObjectFlagsRecordSetup.
 * Entry: as ObjectFlagsRecordSetup. Exit: as ObjectFlagsRecordSetup.
 */
void abi_ObjectFlagsRecordSetup20(void)
{
    uint32_t a = (uint32_t)cpu.db << 16 | (uint16_t)(0x2024 + cpu.x);
    uint8_t v;

    cyc(30);
    v = rd8(a);
    v |= 0x20;
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, v);
    cyc(46);
    wr8(a, v);
    abi_ObjectFlagsRecordSetup();
}
