/*
 * handlers.c: handlers of the menu screens' objects (the records described
 * in actors.c), each run once a frame through the record's +$08 with X =
 * the record, M 16-bit, X 16-bit and D = $0000, so the record's fields are
 * direct page offsets indexed by X. Fields used here: +$0A (a step count),
 * +$14/+$16/+$18 (the animation, see MenuActorAnimate) and +$1E (a frame
 * count).
 */
#include "machine.h"
#include "hw.h"
#include "cpu_ops.h"
#include "carried.h"
#include "wram_names.h"

uint16_t MenuActorAnimateFrame(uint8_t bank, uint16_t *x, uint8_t *p);
int MenuSelectConfirm(uint8_t bank, uint16_t *x, uint16_t *y, uint8_t *p);
void SoundPort0QueuePush(uint8_t command);
uint16_t FrameByteWritePush(uint8_t bank, uint16_t addr, uint8_t value);

static uint32_t dbw(uint8_t bank, uint16_t a)
{
    return (uint32_t)bank << 16 | a;
}

/* Compare as CMP does: C when a >= b; N, Z from a - b. */
static uint8_t cmp16(uint8_t p, uint16_t a, uint16_t b)
{
    return (uint8_t)((nz16(p, (uint16_t)(a - b)) & ~P_C) | (a >= b ? (cyc(46), P_C) : 0));
}

/* DEC of a record's word field: one read, one write; returns the result. */
static uint16_t dec_field(uint16_t rec, uint16_t off)
{
    cyc(22);
    uint16_t v = (uint16_t)(rd16((uint16_t)(rec + off)) - 1);

    cyc(14);
    wr16((uint16_t)(rec + off), v);
    return v;
}

/*
 * @routine 0B:D5EC MenuWaitThenAnimate
 * WHAT IT DOES: The handler of an object that waits +$1E + 1 frames and
 * then only animates: once the count is negative the handler becomes
 * MenuActorAnimateOnly ($D5F6), which it runs at once.
 * HOW: A memory decrement of +$1E; on the last frame it stores $D5F6 in
 * +$08 and falls into MenuActorAnimateOnly. The waiting return is not
 * reached in the traces so far.
 * Entry: M 16-bit, X 16-bit, D = $0000; X = record. Exit: waiting: A and X
 * unchanged; N, Z from the count. Done: as MenuActorAnimateFrame.
 */
uint16_t MenuWaitThenAnimate(uint8_t bank, uint16_t a, uint16_t *x, uint8_t *p)
{
    uint16_t c = dec_field(*x, 0x1E);

    if (!(c & 0x8000)) {
        cyc(58);
        *p = nz16(*p, c);
        return a;
    }
    cyc(68);
    wr16((uint16_t)(*x + 0x08), 0xD5F6);
    return cyc_then(42, MenuActorAnimateFrame(bank, x, p));
}

/* MenuWaitThenAnimate on carried registers. */
void MenuWaitThenAnimate_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = MenuWaitThenAnimate(cr->db, cr->a, &cr->x, &p);
    set_p(cr, p);
}

void abi_MenuWaitThenAnimate(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuWaitThenAnimate_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* The button sequence MenuButtonSequence watches for, one mask a step. */
#define BUTTON_SEQUENCE 0x0BD66Du

/*
 * @routine 0B:D67D MenuSequenceColourMath
 * WHAT IT DOES: The reward for the menu's button sequence: queues writes
 * for the next vertical blank that turn on HDMA channels 4 and 6 and set up
 * colour subtraction of a full-intensity fixed colour on BG1-3 and the
 * backdrop, through colour window 1, with the BG window masks cleared. Not
 * reached in the traces so far.
 * HOW: Eight FrameByteWritePush calls: HDMAEN $50, TSW $07, COLDATA $FF,
 * CGADSUB $A7, CGWSEL $20, W12SEL 0, W34SEL 0, WOBJSEL $30; X is pushed
 * around each.
 * Entry: X 16-bit or 8-bit; D any. Exit: A low byte = $30, A high byte
 * unchanged; X unchanged; M 16-bit, X 16-bit; N, Z from X; C, V unchanged.
 */
void MenuSequenceColourMath(uint8_t bank)
{
    cyc(198);
    FrameByteWritePush(bank, HDMAEN, 0x50);
    cyc(320);
    FrameByteWritePush(bank, TSW, 0x07);
    cyc(320);
    FrameByteWritePush(bank, COLDATA, 0xFF);
    cyc(320);
    FrameByteWritePush(bank, CGADSUB, 0xA7);
    cyc(320);
    FrameByteWritePush(bank, CGWSEL, 0x20);
    cyc(320);
    FrameByteWritePush(bank, W12SEL, 0x00);
    cyc(304);
    FrameByteWritePush(bank, W34SEL, 0x00);
    cyc(320);
    FrameByteWritePush(bank, WOBJSEL, 0x30);
}

/* MenuSequenceColourMath on carried registers. */
void MenuSequenceColourMath_cr(Carried *cr)
{
    MenuSequenceColourMath(cr->db);
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0x30);
    set_p(cr, nz16((uint8_t)(cr->p & ~(P_M | P_X)), cr->x));
}

void abi_MenuSequenceColourMath(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuSequenceColourMath_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:D606 MenuButtonSequence
 * WHAT IT DOES: The handler that animates the object and watches the
 * buttons newly pressed ($118B) for an eight-step sequence (the masks at
 * $0B:D66D; +$0A = steps matched). A press of X ($0800) restarts it and
 * clears $1913. A press off the sequence restarts it and is handled as a
 * selection (MenuSelectConfirm). The eighth step plays sound $46, turns on
 * the colour maths (MenuSequenceColourMath), moves the handler on to
 * $D71B and starts animation $152 unless it is already playing; that step
 * is not reached in the traces so far.
 * HOW: MenuActorAnimateFrame, then a 16-bit test of $118B against the
 * step's mask (kept at $11DF); a matched press is consumed (stz $118B).
 * $118B is read once more, unused, before the mask is fetched.
 * Entry: M 16-bit, X 16-bit, D = $0000; X = record. Exit: no press: A = 0;
 * Z set, N clear; X, C, V as MenuActorAnimateFrame. Off the sequence: as
 * MenuSelectConfirm. Matched: A = the steps matched, X = $11F9, P from
 * the compare with 8. Eighth step: A = +$14 when it was $152 (P from the
 * compare), else A = $152 (N, Z clear, C from the compare); X = $11F9.
 */
uint16_t MenuButtonSequence(uint8_t bank, uint16_t *x, uint16_t *y, uint8_t *p)
{
    uint16_t pressed, step, mask, a;

    MenuActorAnimateFrame(bank, x, p);
    pressed = rd16(cyc_then(24, dbw(bank, 0x118B)));
    if (!pressed) {
        cyc(64);
        *p = nz16(*p, pressed);
        return pressed;
    }
    if (pressed & 0x0800) {
        cyc(78);
        wr16((uint16_t)(*x + 0x0A), 0x0000);
        cyc(56);
        wr16(0x001913, 0x0000);
    } else { cyc(62); }
    rd16(cyc_then(24, dbw(bank, 0x118B)));
    cyc(22);
    step = rd16((uint16_t)(*x + 0x0A));
    cyc(100);
    mask = rd16(BUTTON_SEQUENCE + (uint16_t)(step << 1));
    wr16(dbw(bank, 0x11DF), mask);
    *x = rd16(cyc_then(24, dbw(bank, 0x11F9)));
    pressed = rd16(cyc_then(24, dbw(bank, 0x118B)));
    mask = rd16(cyc_then(24, dbw(bank, 0x11DF)));
    if (!(pressed & mask)) {
        cyc(38);
        wr16((uint16_t)(*x + 0x0A), 0x0000);
        return MenuSelectConfirm(bank, x, y, p) ? (cyc(42), 0xFF46) : (cyc(42), 0x0000);
    }
    wr16(dbw(bank, 0x118B), cyc_then(46, 0x0000));
    cyc(22);
    step = (uint16_t)(rd16((uint16_t)(*x + 0x0A)) + 1);
    cyc(36);
    wr16((uint16_t)(*x + 0x0A), step);
    *p = cmp16(*p, step, 0x0008);
    if (step != 0x0008)
        { cyc(88); return step; } else { cyc(40); }
    MenuSequenceColourMath(bank);
    cyc(308);
    SoundPort0QueuePush(0x46);
    cyc(68);
    wr16((uint16_t)(*x + 0x08), 0xD71B);
    cyc(22);
    wr16((uint16_t)(*x + 0x0A), 0x0000);
    cyc(22);
    a = rd16((uint16_t)(*x + 0x14));
    *p = cmp16(*p, a, 0x0152);
    if (a == 0x0152)
        { cyc(42); return a; }          /* RTS */
    a = 0x0152;
    cyc(86);
    wr16((uint16_t)(*x + 0x14), a);
    cyc(22);
    wr16((uint16_t)(*x + 0x16), 0x0000);
    cyc(22);
    wr16((uint16_t)(*x + 0x18), 0x0000);
    *p = nz16(*p, a);
    cyc(42);                            /* RTS */
    return a;
}

/* MenuButtonSequence on carried registers. */
void MenuButtonSequence_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = MenuButtonSequence(cr->db, &cr->x, &cr->y, &p);
    set_p(cr, p);
}

void abi_MenuButtonSequence(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuButtonSequence_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:D5FA MenuWaitThenSequence
 * WHAT IT DOES: The handler of an object that waits +$1E + 1 frames and
 * then watches for the button sequence: once the count is negative the
 * handler becomes MenuButtonSequence ($D606) with no steps matched, and
 * runs at once.
 * HOW: A memory decrement of +$1E; on the last frame +$08 = $D606, +$0A =
 * 0, then falls into MenuButtonSequence.
 * Entry: M 16-bit, X 16-bit, D = $0000; X = record. Exit: waiting: A and X
 * unchanged; N, Z from the count. Done: as MenuButtonSequence.
 */
uint16_t MenuWaitThenSequence(uint8_t bank, uint16_t a, uint16_t *x, uint16_t *y, uint8_t *p)
{
    uint16_t c = dec_field(*x, 0x1E);

    if (!(c & 0x8000)) {
        cyc(58);
        *p = nz16(*p, c);
        return a;
    }
    cyc(68);
    wr16((uint16_t)(*x + 0x08), 0xD606);
    cyc(22);
    wr16((uint16_t)(*x + 0x0A), 0x0000);
    return MenuButtonSequence(bank, x, y, p);
}

/* MenuWaitThenSequence on carried registers. */
void MenuWaitThenSequence_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = MenuWaitThenSequence(cr->db, cr->a, &cr->x, &cr->y, &p);
    set_p(cr, p);
}

void abi_MenuWaitThenSequence(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuWaitThenSequence_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

uint16_t MenuActorCopyGroup(Carried *cr, uint8_t bank, uint16_t *xo, uint8_t *p);
uint16_t MenuActorUploadFrames(uint8_t bank, uint16_t rec, uint16_t frame, uint16_t *xo, uint8_t *p);
void SoundPort0QueuePushRaw(uint8_t command);

/* A + b with carry in c, as ADC sets C, V, N and Z (16-bit). */
static uint16_t adc16(uint16_t a, uint16_t b, int c, uint8_t *p)
{
    unsigned s = (unsigned)a + b + (c ? 1 : 0);
    uint16_t r = (uint16_t)s;

    *p = (uint8_t)((nz16(*p, r) & ~(P_C | P_V)) | (s > 0xFFFF ? P_C : 0)
                   | ((~(a ^ b) & (a ^ r) & 0x8000) ? P_V : 0));
    return r;
}

uint16_t MenuChoiceObjectUpdate(uint16_t rec, uint8_t *p);
uint16_t MenuChoiceObjectFrames(uint8_t bank, uint16_t a, uint16_t *x, uint16_t *y, uint8_t *p);

/*
 * @routine 0B:CB42 MenuChoiceObject38
 * WHAT IT DOES: As MenuChoiceObject00 (actors.c), the frames starting at
 * offset $38: the fourth choice once it is open. Not reached in the traces
 * so far.
 * HOW: MenuChoiceObjectUpdate, $11E3 = $38, MenuChoiceObjectFrames with A
 * = $38.
 * Entry: M 16-bit, X 16-bit, D = $0000; X = record. Exit: as
 * MenuChoiceObjectFrames.
 */
uint16_t MenuChoiceObject38(uint8_t bank, uint16_t *x, uint16_t *y, uint8_t *p)
{
    cyc(46);
    uint16_t a;

    MenuChoiceObjectUpdate(*x, p);
    a = 0x0038;
    *p = nz16(*p, a);
    wr16(dbw(bank, 0x11E3), cyc_then(48, a));
    return cyc_then(42, MenuChoiceObjectFrames(bank, a, x, y, p));
}

/* MenuChoiceObject38 on carried registers. */
void MenuChoiceObject38_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = MenuChoiceObject38(cr->db, &cr->x, &cr->y, &p);
    set_p(cr, p);
}

void abi_MenuChoiceObject38(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuChoiceObject38_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:CB3D MenuChoiceObject38Start
 * WHAT IT DOES: The open fourth choice's first frame after
 * MenuChoiceObject48Start: makes MenuChoiceObject38 ($CB42) the handler
 * and runs it. Not reached in the traces so far.
 * HOW: +$08 = $CB42, then falls into MenuChoiceObject38.
 * Entry: as MenuChoiceObject38. Exit: as MenuChoiceObject38.
 */
uint16_t MenuChoiceObject38Start(uint8_t bank, uint16_t *x, uint16_t *y, uint8_t *p)
{
    cyc(46);
    wr16((uint16_t)(*x + 0x08), 0xCB42);
    return MenuChoiceObject38(bank, x, y, p);
}

/* MenuChoiceObject38Start on carried registers. */
void MenuChoiceObject38Start_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = MenuChoiceObject38Start(cr->db, &cr->x, &cr->y, &p);
    set_p(cr, p);
}

void abi_MenuChoiceObject38Start(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuChoiceObject38Start_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:CB28 MenuChoiceObject48Start
 * WHAT IT DOES: First frame of the choice object for the fourth choice:
 * copies its sprite group (MenuActorCopyGroup). While the saved progress
 * byte ($70:FC0B) is 0 it shows animation $FA with frame 0 of the frames
 * at table offset $48 and becomes MenuChoiceObject48 ($CB74); otherwise
 * it becomes MenuChoiceObject38Start ($CB3D), a path not reached in the
 * traces so far.
 * HOW: The animation is started only when +$14 is not already $FA (+$16
 * and +$18 cleared); then $11E3 = $48 and MenuActorUploadFrames with A = 0.
 * Entry: M 16-bit, X 16-bit, D = $0000; X = record. Exit: A = the new
 * handler; N set, Z clear; X = $11F9. Locked: Y = $FFFF; C, V as
 * MenuActorUploadFrames. Open: Y unchanged; C, V as MenuActorCopyGroup.
 */
uint16_t MenuChoiceObject48Start(Carried *cr, uint8_t bank, uint16_t *x, uint16_t *y, uint8_t *p)
{
    cyc(32);
    uint16_t a = rd16(0x70FC0B) & 0x00FF;

    if (a) {
        cyc(110);
        MenuActorCopyGroup(cr, bank, x, p);
        a = 0xCB3D;
        cyc(46);
        wr16((uint16_t)(*x + 0x08), a);
        *p = nz16(*p, a);
        return a;
    }
    cyc(92);
    a = rd16((uint16_t)(*x + 0x14));
    if (a != 0x00FA) {
        cyc(86);
        wr16((uint16_t)(*x + 0x14), 0x00FA);
        cyc(22);
        wr16((uint16_t)(*x + 0x16), 0x0000);
        cyc(22);
        wr16((uint16_t)(*x + 0x18), 0x0000);
    }
    cyc(46);
    MenuActorCopyGroup(cr, bank, x, p);
    wr16(dbw(bank, 0x11E3), cyc_then(48, 0x0048));
    cyc(70);
    MenuActorUploadFrames(bank, *x, 0x0000, x, p);
    *y = 0xFFFF;
    a = 0xCB74;
    cyc(46);
    wr16((uint16_t)(*x + 0x08), a);
    *p = nz16(*p, a);
    return a;
}

/* MenuChoiceObject48Start on carried registers. */
void MenuChoiceObject48Start_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = MenuChoiceObject48Start(cr, cr->db, &cr->x, &cr->y, &p);
    set_p(cr, p);
    cyc(42);
}

void abi_MenuChoiceObject48Start(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuChoiceObject48Start_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:CE8A MenuBg1RiseAway
 * WHAT IT DOES: The handler of the BG1 object leaving the screen upwards
 * after a selection: its speed (+$22) grows by 1 every second frame, its
 * position (+$20) moves by the speed, BG1's vertical scroll follows it
 * (-position) and +$06 = position + $4D. Once the position is below -$58
 * it plays sound $13 and hands over to MenuDropAway ($CF27). The path for
 * a position that is not negative is not reached in the traces so far.
 * HOW: MenuActorAnimateFrame first; +$0A toggles between 0 and 1. The
 * scroll is queued for the next vertical blank with two
 * FrameByteWritePush calls to BG1VOFS, high byte first.
 * Entry: M 16-bit, X 16-bit, D = $0000; X = record. Exit: A = +$06; N, Z,
 * C, V from the add; M 16-bit, X 16-bit; X as MenuActorAnimateFrame left
 * it.
 */
uint16_t MenuBg1RiseAway(uint8_t bank, uint16_t *x, uint8_t *p)
{
    uint16_t t, pos, v;

    MenuActorAnimateFrame(bank, x, p);
    cyc(22);
    t = (uint16_t)((rd16((uint16_t)(*x + 0x0A)) + 1) & 0x0001);
    cyc(60);
    wr16((uint16_t)(*x + 0x0A), t);
    if (!t) {
        cyc(38);
        v = adc16(rd16((uint16_t)(*x + 0x22)), 0x0001, 0, p);
        cyc(60);
        wr16((uint16_t)(*x + 0x22), v);
    } else { cyc(22); }
    cyc(22);
    pos = rd16((uint16_t)(*x + 0x20));
    cyc(36);
    v = rd16((uint16_t)(*x + 0x22));
    pos = adc16(pos, v, 0, p);
    cyc(22);
    wr16((uint16_t)(*x + 0x20), pos);
    cyc(22);
    pos = rd16((uint16_t)(*x + 0x20));
    if ((pos & 0x8000) && (cyc(10), pos < 0xFFA8)) {
        cyc(104);
        wr16((uint16_t)(*x + 0x08), 0xCF27);
        cyc(62);
        SoundPort0QueuePushRaw(0x13);
    }
    cyc(pos & 0x8000 ? 142 : 106);      /* BMI not taken, BRL */
    pos = rd16((uint16_t)(*x + 0x20));
    v = (uint16_t)(0 - pos);
    cyc(202);
    FrameByteWritePush(bank, BG1VOFS, (uint8_t)(v >> 8));
    cyc(324);
    FrameByteWritePush(bank, BG1VOFS, (uint8_t)v);
    cyc(188);
    pos = rd16((uint16_t)(*x + 0x20));
    v = adc16(pos, 0x004D, 0, p);
    cyc(60);
    wr16((uint16_t)(*x + 0x06), v);
    *p &= (uint8_t)~(P_M | P_X);
    cyc(42);
    return v;
}

/* MenuBg1RiseAway on carried registers. */
void MenuBg1RiseAway_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = MenuBg1RiseAway(cr->db, &cr->x, &p);
    set_p(cr, p);
}

void abi_MenuBg1RiseAway(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuBg1RiseAway_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:DF7B MenuActorNoStep
 * WHAT IT DOES: Nothing: the frame handler of an object that does nothing
 * on its own.
 * HOW: RTS.
 * Entry: any. Exit: every register and flag unchanged.
 */
/* MenuActorNoStep on carried registers: no register work. */
void MenuActorNoStep_cr(Carried *cr)
{
    (void)cr;
    cyc(42);
}

void abi_MenuActorNoStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuActorNoStep_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:E0DF MenuActorNoStepE0DF
 * WHAT IT DOES: Nothing: as MenuActorNoStep, a second copy.
 * HOW: RTS.
 * Entry: any. Exit: every register and flag unchanged.
 */
/* MenuActorNoStepE0DF on carried registers: no register work. */
void MenuActorNoStepE0DF_cr(Carried *cr)
{
    (void)cr;
    cyc(42);
}

void abi_MenuActorNoStepE0DF(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuActorNoStepE0DF_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

uint16_t VramDmaQueueAddTimed(uint16_t a);
uint8_t SaveUnlocksCheck(void);
void SoundPort0QueuePushW(uint8_t command, unsigned wide);

/*
 * @routine 0B:DF7C MenuSelectionFramesCycle
 * WHAT IT DOES: Object X's frame: every seventh frame (its count +$0A run
 * down from 6) steps its frame +$22 through 0-3 and queues that frame of the
 * picture for the selection $18CB to VRAM: $80 bytes from $70:3000 + the
 * frame's offset (0, $200, $400, $200; table $0B:DFCB) + the selection's
 * ($80, $100, 0, $180; table $0B:DFD3), to VRAM word $15C0.
 * HOW: The frame offset is kept at $11DF; the request goes through the
 * words at $16B1-$16B7 (data bank) and VramDmaQueueAdd; X is reloaded from
 * $11F9.
 * Entry: M 16-bit, X 16-bit, D = $0000; X = the object. Exit: counting:
 * A unchanged, N, Z from the count; a step: X = $11F9 with N, Z from it,
 * A as VramDmaQueueAdd leaves it, C and V from the address add.
 * From C: *x, *a, *p the registers in and out.
 */
void MenuSelectionFramesCycle(uint8_t bank, uint16_t *x, uint16_t *a, uint8_t *p)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t obj = *x, n, off, sel, v;

    n = dec_field(obj, 0x0A);
    *p = nz16(*p, n);
    if (!(n & 0x8000))
        { cyc(64); return; }
    cyc(62);
    wr16((uint16_t)(obj + 0x0A), 0x0006);
    cyc(22);
    n = (uint16_t)((rd16((uint16_t)(obj + 0x22)) + 1) & 0x0003);
    cyc(60);
    wr16((uint16_t)(obj + 0x22), n);
    cyc(100);
    off = rd16(0x0BDFCB + (uint32_t)(n << 1));
    wr16(b | W(scratch_11df), off);
    cyc(32);
    sel = (uint16_t)(rd16(W(selection)) << 1);
    cyc(114);
    v = rd16(0x0BDFD3 + (uint32_t)sel);
    v = adc16(v, rd16(b | W(scratch_11df)), 0, p);
    v = adc16(v, 0x3000, 0, p);
    cyc(84);
    wr16(b | 0x16B3, v);
    cyc(48);
    wr16(b | W(vram_queue_staged), 0x0080);
    cyc(48);
    wr16(b | 0x16B5, 0x0070);
    cyc(48);
    wr16(b | 0x16B7, 0x15C0);
    *a = (cyc(62), VramDmaQueueAddTimed(0x15C0));
    cyc(68);
    *x = rd16(b | W(actor_current));
    *p = nz16(*p, *x);
    cyc(42);
}

/* MenuSelectionFramesCycle on carried registers. */
void MenuSelectionFramesCycle_cr(Carried *cr)
{
    uint8_t p = cr->p;

    MenuSelectionFramesCycle(cr->db, &cr->x, &cr->a, &p);
    set_p(cr, p);
}

void abi_MenuSelectionFramesCycle(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuSelectionFramesCycle_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* The save area MenuEraseConfirm2 clears: $400 bytes of Super FX RAM. */
#define SAVE_AREA 0x70FC00u

/* SEP #$20, LDA #cmd, JSL SoundPort0QueuePush, REP #$20. */
static void menu_sound(uint8_t bank, uint16_t *a, uint8_t *p, uint8_t cmd)
{
    *p |= P_M;
    *a = (uint16_t)((*a & 0xFF00) | cmd);
    cyc(100);
    SoundPort0QueuePushW(cmd, (*p & P_X) ? 0 : 16);
    *p = (uint8_t)(nz8(*p, bank) & ~P_M);   /* the last pull restores DB */
}

/* LDA +$14,X, CMP #n, BEQ; else LDA #n, STA +$14,X, STZ +$16,X, STZ
   +$18,X: animation n started unless it is the one playing. */
static void menu_anim(uint16_t x, uint16_t *a, uint8_t *p, uint16_t n)
{
    cyc(22);
    uint16_t v = rd16((uint16_t)(x + 0x14));

    *a = v;
    *p = cmp16(*p, v, n);
    if (v == n)
        return;
    *a = n;
    *p = nz16(*p, n);
    cyc(86);
    wr16((uint16_t)(x + 0x14), n);
    cyc(22);
    wr16((uint16_t)(x + 0x16), 0x0000);
    cyc(22);
    wr16((uint16_t)(x + 0x18), 0x0000);
}

/* LDA $118B, AND #mask: the buttons newly pressed under mask. */
static uint16_t menu_pressed(uint8_t bank, uint16_t *a, uint8_t *p, uint16_t mask)
{
    *a = (uint16_t)(rd16(cyc_then(24, dbw(bank, 0x118B))) & mask);
    *p = nz16(*p, *a);
    return *a;
}

/* LDA +$0A,X: the choice (0 no, else yes). */
static uint16_t menu_choice(uint16_t x, uint16_t *a, uint8_t *p)
{
    cyc(62);
    *a = rd16((uint16_t)(x + 0x0A));
    *p = nz16(*p, *a);
    return *a;
}

/* LDA #h, STA +$08,X: the next handler. */
static void menu_handler(uint16_t x, uint16_t *a, uint8_t *p, uint16_t h)
{
    *a = h;
    *p = nz16(*p, h);
    cyc(46);
    wr16((uint16_t)(x + 0x08), h);
}

/*
 * @routine 0B:D6F8 MenuSequenceColourOff
 * WHAT IT DOES: Undoes MenuSequenceColourMath: queues writes for the next
 * vertical blank that leave only HDMA channel 4 on and clear TSW.
 * HOW: Two FrameByteWritePush calls: HDMAEN $10, TSW 0; X is pushed around
 * each.
 * Entry: X 16-bit or 8-bit; D any. Exit: A low byte = 0, A high byte
 * unchanged; X unchanged; M 16-bit, X 16-bit; N, Z from X; C, V unchanged.
 */
void MenuSequenceColourOff(uint8_t bank)
{
    cyc(198);
    FrameByteWritePush(bank, HDMAEN, 0x10);
    cyc(320);
    FrameByteWritePush(bank, TSW, 0x00);
    cyc(208);                           /* the push's end, PLX, REP, RTS */
}

/* MenuSequenceColourOff on carried registers. */
void MenuSequenceColourOff_cr(Carried *cr)
{
    MenuSequenceColourOff(cr->db);
    cr->a &= 0xFF00;
    set_p(cr, nz16((uint8_t)(cr->p & ~(P_M | P_X)), cr->x));
}

void abi_MenuSequenceColourOff(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuSequenceColourOff_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* B with "no" chosen: back to MenuButtonSequence ($D606) with the colour
   maths off, sound $47 and animation 2 ($0B:D79F, $0B:D84F, $0B:D879
   without the sound). */
static void menu_erase_cancel(uint8_t bank, uint16_t x, uint16_t *a, uint8_t *p, int sound)
{
    cyc(16);
    menu_handler(x, a, p, 0xD606);
    (cyc(46), MenuSequenceColourOff(bank));
    *a &= 0xFF00;
    *p = nz16((uint8_t)(*p & ~(P_M | P_X)), x);
    if (sound) {
        menu_sound(bank, a, p, 0x47);
        cyc(22);
    }
    menu_anim(x, a, p, 0x0002);
    cyc(42);                            /* RTS */
}

/*
 * @routine 0B:D71B MenuEraseConfirm
 * WHAT IT DOES: The handler after the menu's button sequence: the first
 * "erase?" question. Left ($0200) picks yes (+$0A = 1, animation $148,
 * sound $45), Right ($0100) picks no (+$0A = 0, sound $45, animation
 * $152); B ($8000) answers: yes asks again (animation $166, handler $D7C2
 * MenuEraseConfirm2, +$0A = 0, sound $46); no goes back to the sequence
 * (handler $D606, MenuSequenceColourOff, sound $47, animation 2).
 * HOW: MenuActorAnimateFrame, then three 16-bit tests of the buttons
 * newly pressed ($118B), each read again.
 * Entry: M 16-bit, X 16-bit, D = $0000; X = record. Exit: M 16-bit; A and
 * P from the last load, compare or sound (A low byte = the command, N, Z
 * from the data bank).
 * From C: *x, *a, *p the registers in and out.
 */
void MenuEraseConfirm(uint8_t bank, uint16_t *x, uint16_t *a, uint8_t *p)
{
    *a = MenuActorAnimateFrame(bank, x, p);
    if (!menu_pressed(bank, a, p, 0x0200)) {
        cyc(24);                        /* AND #imm */
    } else if (!menu_choice(*x, a, p)) {
        *a = 0x0001;
        *p = nz16(*p, 0x0001);
        cyc(62);
        wr16((uint16_t)(*x + 0x0A), 0x0001);
        menu_anim(*x, a, p, 0x0148);
        menu_sound(bank, a, p, 0x45);
    }
    cyc(22);
    if (menu_pressed(bank, a, p, 0x0100) && menu_choice(*x, a, p)) {
        cyc(16);
        menu_sound(bank, a, p, 0x45);
        cyc(44);
        wr16((uint16_t)(*x + 0x0A), 0x0000);
        menu_anim(*x, a, p, 0x0152);
    } else { cyc(46); }
    if (!menu_pressed(bank, a, p, 0x8000))
        { cyc(88); return; }            /* BEQ taken, RTS */
    if (!menu_choice(*x, a, p)) {
        cyc(6);
        menu_erase_cancel(bank, *x, a, p, 1);
        return;
    } else { cyc(16); }
    menu_anim(*x, a, p, 0x0166);
    menu_handler(*x, a, p, 0xD7C2);
    cyc(22);
    wr16((uint16_t)(*x + 0x0A), 0x0000);
    menu_sound(bank, a, p, 0x46);
    cyc(64);                            /* REP, RTS */
}

/* MenuEraseConfirm on carried registers. */
void MenuEraseConfirm_cr(Carried *cr)
{
    uint8_t p = cr->p;

    MenuEraseConfirm(cr->db, &cr->x, &cr->a, &p);
    set_p(cr, p);
}

void abi_MenuEraseConfirm(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuEraseConfirm_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:D892 MenuEraseSave
 * WHAT IT DOES: Erases the save: clears the $400 bytes at $70:FC00, then
 * lets SaveUnlocksCheck put back an empty, valid save.
 * HOW: 16-bit zero stores from X = 0 to $3FE; SaveUnlocksCheck.
 * Entry: M 16-bit, X 16-bit, D = $0000. Exit: as SaveUnlocksCheck; P from
 * the last CPX (Z and C set, N clear).
 * From C: returns A; *x, *y, *p as it leaves them.
 */
uint16_t MenuEraseSave(uint16_t *x, uint16_t *y, uint8_t *p)
{
    uint16_t i;

    for (i = 0; i != 0x0400; i = (uint16_t)(i + 2))
        { cyc(i == 0 ? 80 : 106); wr16(SAVE_AREA + i, 0x0000); }
    cyc(130);
    *p = (uint8_t)((*p & ~P_N) | P_Z | P_C);
    uint16_t a = (uint16_t)(0x0200 | SaveUnlocksCheck());

    *x = 20;
    *y = 4;
    cyc(42);                            /* RTS */
    return a;
}

/* MenuEraseSave on carried registers. */
void MenuEraseSave_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = MenuEraseSave(&cr->x, &cr->y, &p);
    set_p(cr, p);
}

void abi_MenuEraseSave(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuEraseSave_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:D7C2 MenuEraseConfirm2
 * WHAT IT DOES: The second "erase?" question: Left picks yes (+$0A = 1,
 * animation $15C, sound $45), Right picks no (+$0A = 0, animation $166,
 * sound $45); B answers: yes erases the save (MenuEraseSave), starts
 * animation $170 on the record at $11F9, plays sound $36 and waits $40
 * frames (handler $D872 MenuEraseDone, +$0A = $40); no goes back to the
 * sequence as MenuEraseConfirm's no.
 * HOW: As MenuEraseConfirm; X is reloaded from $11F9 after the erase.
 * Entry: M 16-bit, X 16-bit, D = $0000; X = record. Exit: M 16-bit; erase:
 * A = $40, N, Z clear; otherwise as MenuEraseConfirm.
 * From C: as MenuEraseConfirm, and *y (the erase leaves Y = 4).
 */
void MenuEraseConfirm2(uint8_t bank, uint16_t *x, uint16_t *y, uint16_t *a, uint8_t *p)
{
    *a = MenuActorAnimateFrame(bank, x, p);
    if (!menu_pressed(bank, a, p, 0x0200)) {
        cyc(24);                        /* AND #imm */
    } else if (!menu_choice(*x, a, p)) {
        *a = 0x0001;
        *p = nz16(*p, 0x0001);
        cyc(62);
        wr16((uint16_t)(*x + 0x0A), 0x0001);
        menu_anim(*x, a, p, 0x015C);
        menu_sound(bank, a, p, 0x45);
    }
    cyc(22);
    if (menu_pressed(bank, a, p, 0x0100) && menu_choice(*x, a, p)) {
        cyc(38);
        wr16((uint16_t)(*x + 0x0A), 0x0000);
        menu_anim(*x, a, p, 0x0166);
        menu_sound(bank, a, p, 0x45);
    } else { cyc(24); }
    cyc(22);
    if (!menu_pressed(bank, a, p, 0x8000))
        { cyc(88); return; }            /* BEQ taken, RTS */
    if (!menu_choice(*x, a, p)) {
        cyc(6);
        menu_erase_cancel(bank, *x, a, p, 1);
        return;
    } else { cyc(62); }
    *a = MenuEraseSave(x, y, p);
    *x = rd16(cyc_then(24, dbw(bank, 0x11F9)));
    *p = nz16(*p, *x);
    menu_anim(*x, a, p, 0x0170);
    menu_sound(bank, a, p, 0x36);
    cyc(22);
    menu_handler(*x, a, p, 0xD872);
    *a = 0x0040;
    *p = nz16(*p, 0x0040);
    cyc(46);
    wr16((uint16_t)(*x + 0x0A), 0x0040);
    cyc(42);                            /* RTS */
}

/* MenuEraseConfirm2 on carried registers. */
void MenuEraseConfirm2_cr(Carried *cr)
{
    uint8_t p = cr->p;

    MenuEraseConfirm2(cr->db, &cr->x, &cr->y, &cr->a, &p);
    set_p(cr, p);
}

void abi_MenuEraseConfirm2(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuEraseConfirm2_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:D872 MenuEraseDone
 * WHAT IT DOES: Waits out the count +$0A after the erase, then goes back
 * to the button sequence (handler $D606, MenuSequenceColourOff, animation
 * 2) with no sound.
 * HOW: MenuActorAnimateFrame, a memory decrement of +$0A; BPL waits.
 * Entry: M 16-bit, X 16-bit, D = $0000; X = record. Exit: waiting: A as
 * MenuActorAnimateFrame leaves it, N, Z from the count; done: as the
 * animation's compare or start.
 * From C: as MenuEraseConfirm.
 */
void MenuEraseDone(uint8_t bank, uint16_t *x, uint16_t *a, uint8_t *p)
{
    uint16_t c;

    *a = MenuActorAnimateFrame(bank, x, p);
    c = dec_field(*x, 0x0A);
    *p = nz16(*p, c);
    if (!(c & 0x8000))
        { cyc(64); return; }            /* BPL taken, RTS */
    menu_erase_cancel(bank, *x, a, p, 0);
}

/* MenuEraseDone on carried registers. */
void MenuEraseDone_cr(Carried *cr)
{
    uint8_t p = cr->p;

    MenuEraseDone(cr->db, &cr->x, &cr->a, &p);
    set_p(cr, p);
}

void abi_MenuEraseDone(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuEraseDone_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:D8A8 MenuActorNoStepD8A8
 * WHAT IT DOES: Nothing: a handler that only returns.
 * HOW: RTS.
 * Entry: any. Exit: unchanged.
 */
/* MenuActorNoStepD8A8 on carried registers: no register work. */
void MenuActorNoStepD8A8_cr(Carried *cr)
{
    (void)cr;
    cyc(42);
}

void abi_MenuActorNoStepD8A8(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuActorNoStepD8A8_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}
