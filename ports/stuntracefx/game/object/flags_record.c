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
#include "object_calls.h"
#include "carried.h"


uint8_t GsuRecordNew422(uint8_t bank, uint16_t obj);
uint16_t GsuRecordFlag4(uint8_t bank, uint16_t obj, int *has);

/* Object field `f` ($20xx) of object `o`, through the data bank. */
#define F(f, o) ((uint32_t)b << 16 | (uint16_t)((f) + (o)))
/* A work RAM byte through the data bank. */
#define W(a) ((uint32_t)b << 16 | (a))

/* How these routines end, for the adapters: no room in the record pool
   (A = $00FF), GsuRecordFlag4 with a record (A = fr_a, Y = the object) or
   without one (A = fr_a, $FF in its low byte); C and V of the frame
   steps' last compare or add (fr_cv: bit 0 C, bit 1 V). */
enum { FR_NO_ROOM, FR_RECORD, FR_NO_RECORD };
static int fr_kind, fr_cv, fr_y_obj;
static uint16_t fr_a;
static uint8_t fr_c, fr_v;

/* LDA #v with M 8-bit: its time. */
static void lda8(void)
{
    cyc(30);
}

/* LDA addr, 8-bit. */
static uint8_t ld8(uint32_t addr)
{
    cyc(24);
    return reg_rd(addr);
}

/* CMP with M 8-bit: its time; C to fr_c. */
static void cmp8(uint8_t a, uint8_t m)
{
    cyc(a >= m ? 18 : 16);
    fr_c = a >= m ? P_C : 0;
    fr_cv |= 1;
}

/* ADC and SBC (8-bit, binary: D is clear here); C and V to fr_c, fr_v.
   The SBC's 16 clocks are op_sbc8's stated time. */
static uint8_t adc8(uint8_t a, uint8_t m, int c)
{
    unsigned r = (unsigned)a + m + (c ? 1 : 0);

    fr_c = r > 0xFF ? P_C : 0;
    fr_v = (~(a ^ m) & (a ^ r) & 0x80) ? P_V : 0;
    fr_cv |= 3;
    return (uint8_t)r;
}

static uint8_t sbc8(uint8_t a, uint8_t m)
{
    cyc(16);
    return adc8(a, (uint8_t)~m, 1);
}

/* INC or DEC addr, 8-bit. */
static void step8(uint32_t addr, int d)
{
    cyc(30);
    uint8_t v = rd8(addr);

    v = (uint8_t)(v + d);
    cyc(6);
    wr8(addr, v);
}

/* The handler of object x: +$1A = h, +$1C = $08 (16-bit then 8-bit
   immediate stores). */
static void handler08(uint8_t b, uint16_t x, uint16_t h)
{
    cyc(76);
    wr16(F(0x201A, x), h);
    lda8();
    cyc(38);
    wr8(F(0x201C, x), 0x08);
}

/* LDA +f, STA $1F58, LDA $1F58, ORA #$80, STA +$5A: the frame from field
   f, shown. */
static void frame_from(uint8_t b, uint16_t x, uint16_t f)
{
    uint8_t v;

    cyc(6);
    v = ld8(F(f, x));
    cyc(24);
    wr8(W(0x1F58), v);
    v = ld8(W(0x1F58));
    v |= 0x80;
    lda8();
    cyc(16);
    wr8(F(0x205A, x), v);
}

/* LDA #0, ORA #$80, STA +$5A: frame 0, shown. */
static void frame_zero(uint8_t b, uint16_t x)
{
    lda8();
    lda8();
    cyc(2);
    wr8(F(0x205A, x), 0x80);
}

/*
 * @routine 08:CD7E ObjectRecordFlag4Step
 * WHAT IT DOES: A handler that only marks object X's record
 * (GsuRecordFlag4) each frame; also the tail other steps jump to.
 * HOW: JSL to GsuRecordFlag4.
 * Entry: as GsuRecordFlag4. Exit: as GsuRecordFlag4.
 */
int ObjectRecordFlag4Step(uint8_t b, uint16_t obj)
{
    int has;
    uint16_t a = (cyc(62), GsuRecordFlag4(b, obj, &has));

    cyc(8);
    cyc(88);
    cyc(44);
    fr_kind = has ? FR_RECORD : FR_NO_RECORD;
    if (has)
        fr_a = a;
    return has;
}

void ObjectFlagsRecord_cr(Carried *cr);
void ObjectFlagsRecordFrame_cr(Carried *cr);
void ObjectFlagsRecordSetup_cr(Carried *cr);
void ObjectFlagsRecordSetup20_cr(Carried *cr);
void ObjectFrameCycleBackSetup_cr(Carried *cr);
void ObjectFrameCycleBackStep_cr(Carried *cr);
void ObjectFrameCycleSetup_cr(Carried *cr);
void ObjectFrameCycleStep_cr(Carried *cr);
void ObjectFrameSwingSetup_cr(Carried *cr);
void ObjectFrameSwingStep_cr(Carried *cr);
void ObjectRecordFlag4Step_cr(Carried *cr);
void ObjectStopMarked_cr(Carried *cr);
static void flags_record_exit(Carried *cr);
static void flags_record_run(Carried *cr, void (*f)(uint8_t, uint16_t));

/* The registers these routines leave. */
static void flags_record_exit(Carried *cr)
{
    switch (fr_kind) {
    case FR_NO_ROOM:
        cr->a = 0x00FF;
        set_p(cr, (uint8_t)((cr->p & ~P_Z) | P_N | P_M));
        break;
    case FR_RECORD:
        cr->a = fr_a;
        cr->y = cr->x;
        set_p(cr, (uint8_t)((cr->p & ~P_N) | P_Z | P_M));
        break;
    default:
        cr->a = (uint16_t)((fr_a & 0xFF00) | 0xFF);
        if (fr_y_obj)
            cr->y = cr->x;
        set_p(cr, (uint8_t)((cr->p & ~P_Z) | P_N | P_M));
        break;
    }
    if (fr_cv & 1)
        set_p(cr, (uint8_t)((cr->p & ~P_C) | fr_c));
    if (fr_cv & 2)
        set_p(cr, (uint8_t)((cr->p & ~P_V) | fr_v));
}

void ObjectRecordFlag4Step_cr(Carried *cr)
{
    fr_cv = fr_y_obj = 0;
    fr_a = cr->a;
    ObjectRecordFlag4Step(cr->db, cr->x);
    flags_record_exit(cr);
}

void abi_ObjectRecordFlag4Step(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectRecordFlag4Step_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* 08:CB7E: the shown frame (+$5A, low seven bits, a 16-bit read) to the
   record's +$12 when the object has one, then ObjectRecordFlag4Step. The
   object in Y and the record in X while copying. */
static void frames_to_record(uint8_t b, uint16_t obj)
{
    uint16_t rec, v;

    cyc(96);
    rec = rd16(F(0x2022, obj));
    fr_a = rec;
    if (rec) {
        cyc(60);
        v = (uint16_t)(rd16(F(0x205A, obj)) & 0x007F);
        fr_a = v;
        cyc(56);
        wr16(0x700012 + rec, v);
    } else { cyc(22); }
    cyc(68);
    fr_y_obj = 1;
    ObjectRecordFlag4Step(b, obj);
}

/* Every +$49 frames (+$4A counts them) the frame +$5A steps by d (1 or
   $FF) round the +$48 frames: ADC d, and unless that is negative ADC +$48;
   AND #$7F; at +$48 or more, SBC +$48; shown (ORA #$80). Then
   frames_to_record. */
static void frame_cycle(uint8_t b, uint16_t x, uint8_t d)
{
    uint8_t v, m;

    step8(F(0x204A, x), 1);
    cyc(6);
    v = ld8(F(0x204A, x));
    cyc(30);
    m = rd8(F(0x2049, x));
    cmp8(v, m);
    if (v != m) {
        cyc(6);
        frames_to_record(b, x);
        return;
    }
    cyc(28);
    wr8(F(0x204A, x), 0x00);
    cyc(6);
    v = ld8(F(0x2048, x));
    cyc(24);
    wr8(W(0x1F58), v);
    cyc(6);
    v = ld8(F(0x205A, x));
    v = adc8(v, d, 0);
    if (!(v & 0x80)) {
        cyc(84);
        m = rd8(W(0x1F58));
        v = adc8(v, m, 0);
    } else { cyc(52); }
    v &= 0x7F;
    lda8();
    cyc(10);
    m = rd8(W(0x1F58));
    cmp8(v, m);
    if (v >= m) {
        cyc(36);
        m = rd8(W(0x1F58));
        v = sbc8(v, m);
    } else { cyc(22); }
    v |= 0x80;
    lda8();
    wr8(F(0x205A, x), v);
    frames_to_record(b, x);
}

/*
 * @routine 08:CA2D ObjectStopMarked
 * WHAT IT DOES: Sets bit 0 of object X's +$09 and clears its handler
 * (+$1A = 0, +$1C = 0). Nothing in the game refers to it.
 * HOW: An 8-bit OR, then 16-bit and 8-bit zero stores.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; A = 0, Z set.
 */
void ObjectStopMarked(uint8_t b, uint16_t x)
{
    uint8_t v;

    cyc(6);
    v = (uint8_t)(ld8(F(0x2009, x)) | 0x01);
    lda8();
    cyc(16);
    wr8(F(0x2009, x), v);
    cyc(76);
    wr16(F(0x201A, x), 0x0000);
    lda8();
    cyc(38);
    wr8(F(0x201C, x), 0x00);
    cyc(44); /* RTL */
}

void ObjectStopMarked_cr(Carried *cr)
{
    ObjectStopMarked(cr->db, cr->x);
    cr->a = 0x0000;
    set_p(cr, (uint8_t)((cr->p & ~P_N) | P_Z | P_M));
}

void abi_ObjectStopMarked(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectStopMarked_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
void ObjectFlagsRecord(uint8_t b, uint16_t obj)
{
    uint16_t rec, v, w;

    cyc(62);
    if ((cyc(62), GsuRecordNew422(b, obj))) {
        cyc(60);
        fr_kind = FR_NO_ROOM;
        cyc(46 + 44);                   /* BRL; RTL */
        return;
    }
    cyc(60);
    cyc(88);
    rec = rd16(F(0x2022, obj));
    cyc(44);
    v = rd16(F(0x2006, obj));
    cyc(32);
    w = rd16(0x700016 + rec);
    v |= w;
    cyc(32);
    wr16(0x700016 + rec, v);
    cyc(32);
    v = rd16(0x700016 + rec);
    cyc(30);
    w = rd16(F(0x2027, obj));
    v |= w;
    cyc(32);
    wr16(0x700016 + rec, v);
    fr_a = v;
    fr_y_obj = 1;
    (cyc(36), ObjectRecordFlag4Step(b, obj));
}

void ObjectFlagsRecord_cr(Carried *cr)
{
    fr_cv = fr_y_obj = 0;
    ObjectFlagsRecord(cr->db, cr->x);
    flags_record_exit(cr);
}

void abi_ObjectFlagsRecord(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectFlagsRecord_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* 08:CD46, the end of ObjectFlagsRecordSetup that other set-ups branch
   to: +$1D = $08:CD55 (ObjectFlagsRecord), then the record
   (ObjectFlagsRecord). */
static void flags_record_tail(uint8_t b, uint16_t x)
{
    cyc(76);
    wr16(F(0x201D, x), 0xCD55);
    lda8();
    cyc(38);
    wr8(F(0x201F, x), 0x08);
    ObjectFlagsRecord(b, x);
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
void ObjectFlagsRecordSetup(uint8_t b, uint16_t x)
{
    cyc(76);
    wr16(F(0x2027, x), 0x0000);
    cyc(22);
    handler08(b, x, 0xCD7E);
    flags_record_tail(b, x);
}

/* The adapters of the set-ups and steps. */
static void flags_record_run(Carried *cr, void (*f)(uint8_t, uint16_t))
{
    fr_cv = fr_y_obj = 0;
    f(cr->db, cr->x);
    flags_record_exit(cr);
}

void ObjectFlagsRecordSetup_cr(Carried *cr)
{
    flags_record_run(cr, ObjectFlagsRecordSetup);
}

void abi_ObjectFlagsRecordSetup(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectFlagsRecordSetup_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
void ObjectFlagsRecordFrame(uint8_t b, uint16_t x)
{
    frame_from(b, x, 0x2048);
    (cyc(30), ObjectFlagsRecordSetup(b, x));
}

void ObjectFlagsRecordFrame_cr(Carried *cr)
{
    flags_record_run(cr, ObjectFlagsRecordFrame);
}

void abi_ObjectFlagsRecordFrame(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectFlagsRecordFrame_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
void ObjectFrameCycleSetup(uint8_t b, uint16_t x)
{
    handler08(b, x, 0xCA6F);
    frame_zero(b, x);
    (cyc(30), flags_record_tail(b, x));
}

void ObjectFrameCycleSetup_cr(Carried *cr)
{
    flags_record_run(cr, ObjectFrameCycleSetup);
}

void abi_ObjectFrameCycleSetup(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectFrameCycleSetup_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
void ObjectFrameCycleStep(uint8_t b, uint16_t x)
{
    frame_cycle(b, x, 0x01);
}

void ObjectFrameCycleStep_cr(Carried *cr)
{
    flags_record_run(cr, ObjectFrameCycleStep);
}

void abi_ObjectFrameCycleStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectFrameCycleStep_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
void ObjectFrameCycleBackSetup(uint8_t b, uint16_t x)
{
    uint8_t v;

    handler08(b, x, 0xCAC3);
    cyc(6);
    v = (uint8_t)(ld8(F(0x2048, x)) - 1);
    lda8();
    cyc(8);
    wr8(W(0x1F58), v);
    v = (uint8_t)(ld8(W(0x1F58)) | 0x80);
    lda8();
    cyc(16);
    wr8(F(0x205A, x), v);
    (cyc(30), flags_record_tail(b, x));
}

void ObjectFrameCycleBackSetup_cr(Carried *cr)
{
    flags_record_run(cr, ObjectFrameCycleBackSetup);
}

void abi_ObjectFrameCycleBackSetup(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectFrameCycleBackSetup_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:CAC3 ObjectFrameCycleBackStep
 * WHAT IT DOES: As ObjectFrameCycleStep with the frame stepping backwards
 * (ADC #$FF).
 * HOW: As ObjectFrameCycleStep.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as GsuRecordFlag4.
 */
void ObjectFrameCycleBackStep(uint8_t b, uint16_t x)
{
    frame_cycle(b, x, 0xFF);
}

void ObjectFrameCycleBackStep_cr(Carried *cr)
{
    flags_record_run(cr, ObjectFrameCycleBackStep);
}

void abi_ObjectFrameCycleBackStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectFrameCycleBackStep_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
void ObjectFrameSwingSetup(uint8_t b, uint16_t x)
{
    uint8_t v;

    handler08(b, x, 0xCB15);
    frame_zero(b, x);
    cyc(6);
    v = ld8(F(0x204A, x));
    cyc(30);
    wr8(F(0x202E, x), v);
    (cyc(30), flags_record_tail(b, x));
}

void ObjectFrameSwingSetup_cr(Carried *cr)
{
    flags_record_run(cr, ObjectFrameSwingSetup);
}

void abi_ObjectFrameSwingSetup(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectFrameSwingSetup_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
void ObjectFrameSwingStep(uint8_t b, uint16_t x)
{
    uint8_t v, m;

    cyc(6);
    v = ld8(F(0x202E, x));
    if (v) {
        cyc(16);
        step8(F(0x202E, x), -1);
        (cyc(22), frames_to_record(b, x));
        return;
    } else { cyc(22); }
    frame_from(b, x, 0x2030);
    cyc(6);
    v = ld8(F(0x2048, x));
    cyc(24);
    wr8(W(0x1F58), v);
    v--;
    lda8();
    cyc(8);
    wr8(W(0x1F59), v);
    step8(F(0x204B, x), 1);
    cyc(6);
    v = ld8(F(0x204B, x));
    cyc(30);
    m = rd8(F(0x2049, x));
    cmp8(v, m);
    if (v != m) {
        cyc(6);
        frames_to_record(b, x);
        return;
    }
    cyc(28);
    wr8(F(0x204B, x), 0x00);
    cyc(6);
    v = (uint8_t)(ld8(F(0x2025, x)) & 0x01);
    lda8();
    if (v) {
        cyc(40);
        v = (uint8_t)(ld8(F(0x205A, x)) & 0x7F);
        lda8();
        cmp8(v, 0x00);
        if (v) {
            step8(F(0x2030, x), -1);
            frames_to_record(b, x);
            return;
        } else { cyc(2); }
    } else {
        cyc(14);
        v = (uint8_t)(ld8(F(0x205A, x)) & 0x7F);
        lda8();
        cyc(10);
        m = rd8(W(0x1F59));
        cmp8(v, m);
        if (v != m) {
            step8(F(0x2030, x), 1);
            (cyc(22), frames_to_record(b, x));
            return;
        }
    }
    cyc(10);
    v = (uint8_t)(ld8(F(0x2025, x)) ^ 0x01);
    lda8();
    cyc(16);
    wr8(F(0x2025, x), v);
    cyc(6);
    v = ld8(F(0x204A, x));
    cyc(30);
    wr8(F(0x202E, x), v);
    (cyc(22), frames_to_record(b, x));
}

void ObjectFrameSwingStep_cr(Carried *cr)
{
    flags_record_run(cr, ObjectFrameSwingStep);
}

void abi_ObjectFrameSwingStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectFrameSwingStep_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:CD25 ObjectFlagsRecordSetup20
 * WHAT IT DOES: As ObjectFlagsRecordSetup, with bit 5 of object X's +$24
 * set first. Nothing in the game calls it.
 * HOW: An 8-bit OR into +$24, then falls into ObjectFlagsRecordSetup.
 * Entry: as ObjectFlagsRecordSetup. Exit: as ObjectFlagsRecordSetup.
 */
void ObjectFlagsRecordSetup20(uint8_t b, uint16_t x)
{
    uint8_t v;

    cyc(30);
    v = (uint8_t)(rd8(F(0x2024, x)) | 0x20);
    cyc(46);
    wr8(F(0x2024, x), v);
    ObjectFlagsRecordSetup(b, x);
}

void ObjectFlagsRecordSetup20_cr(Carried *cr)
{
    flags_record_run(cr, ObjectFlagsRecordSetup20);
}

void abi_ObjectFlagsRecordSetup20(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectFlagsRecordSetup20_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

