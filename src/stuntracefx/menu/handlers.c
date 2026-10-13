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

uint16_t MenuActorAnimateFrame(uint8_t bank, uint16_t *x, uint8_t *p);
int MenuSelectConfirm(uint8_t bank, uint16_t *x, uint16_t *y, uint8_t *p);
void SoundPort0QueuePush(uint8_t command);
uint16_t FrameByteWritePush(uint8_t bank, uint16_t addr, uint8_t value);
void abi_MenuActorAnimateFrame(void);
void abi_MenuSelectConfirm(void);

static uint32_t dbw(uint8_t bank, uint16_t a)
{
    return (uint32_t)bank << 16 | a;
}

/* Compare as CMP does: C when a >= b; N, Z from a - b. */
static uint8_t cmp16(uint8_t p, uint16_t a, uint16_t b)
{
    return (uint8_t)((nz16(p, (uint16_t)(a - b)) & ~P_C) | (a >= b ? P_C : 0));
}

/* DEC of a record's word field: one read, one write; returns the result. */
static uint16_t dec_field(uint16_t rec, uint16_t off)
{
    uint16_t v = (uint16_t)(rd16((uint16_t)(rec + off)) - 1);

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
        *p = nz16(*p, c);
        return a;
    }
    wr16((uint16_t)(*x + 0x08), 0xD5F6);
    return MenuActorAnimateFrame(bank, x, p);
}

void abi_MenuWaitThenAnimate(void)
{
    uint8_t p = cpu.p;

    cpu.a = MenuWaitThenAnimate(cpu.db, cpu.a, &cpu.x, &p);
    cpu.p = p;
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
    FrameByteWritePush(bank, HDMAEN, 0x50);
    FrameByteWritePush(bank, TSW, 0x07);
    FrameByteWritePush(bank, COLDATA, 0xFF);
    FrameByteWritePush(bank, CGADSUB, 0xA7);
    FrameByteWritePush(bank, CGWSEL, 0x20);
    FrameByteWritePush(bank, W12SEL, 0x00);
    FrameByteWritePush(bank, W34SEL, 0x00);
    FrameByteWritePush(bank, WOBJSEL, 0x30);
}

void abi_MenuSequenceColourMath(void)
{
    MenuSequenceColourMath(cpu.db);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x30);
    cpu.p = nz16((uint8_t)(cpu.p & ~(P_M | P_X)), cpu.x);
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
    pressed = rd16(dbw(bank, 0x118B));
    if (!pressed) {
        *p = nz16(*p, pressed);
        return pressed;
    }
    if (pressed & 0x0800) {
        wr16((uint16_t)(*x + 0x0A), 0x0000);
        wr16(0x001913, 0x0000);
    }
    rd16(dbw(bank, 0x118B));
    step = rd16((uint16_t)(*x + 0x0A));
    mask = rd16(BUTTON_SEQUENCE + (uint16_t)(step << 1));
    wr16(dbw(bank, 0x11DF), mask);
    *x = rd16(dbw(bank, 0x11F9));
    pressed = rd16(dbw(bank, 0x118B));
    mask = rd16(dbw(bank, 0x11DF));
    if (!(pressed & mask)) {
        wr16((uint16_t)(*x + 0x0A), 0x0000);
        return MenuSelectConfirm(bank, x, y, p) ? 0xFF46 : 0x0000;
    }
    wr16(dbw(bank, 0x118B), 0x0000);
    step = (uint16_t)(rd16((uint16_t)(*x + 0x0A)) + 1);
    wr16((uint16_t)(*x + 0x0A), step);
    *p = cmp16(*p, step, 0x0008);
    if (step != 0x0008)
        return step;
    MenuSequenceColourMath(bank);
    SoundPort0QueuePush(0x46);
    wr16((uint16_t)(*x + 0x08), 0xD71B);
    wr16((uint16_t)(*x + 0x0A), 0x0000);
    a = rd16((uint16_t)(*x + 0x14));
    *p = cmp16(*p, a, 0x0152);
    if (a == 0x0152)
        return a;
    a = 0x0152;
    wr16((uint16_t)(*x + 0x14), a);
    wr16((uint16_t)(*x + 0x16), 0x0000);
    wr16((uint16_t)(*x + 0x18), 0x0000);
    *p = nz16(*p, a);
    return a;
}

void abi_MenuButtonSequence(void)
{
    uint8_t p = cpu.p;

    cpu.a = MenuButtonSequence(cpu.db, &cpu.x, &cpu.y, &p);
    cpu.p = p;
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
        *p = nz16(*p, c);
        return a;
    }
    wr16((uint16_t)(*x + 0x08), 0xD606);
    wr16((uint16_t)(*x + 0x0A), 0x0000);
    return MenuButtonSequence(bank, x, y, p);
}

void abi_MenuWaitThenSequence(void)
{
    uint8_t p = cpu.p;

    cpu.a = MenuWaitThenSequence(cpu.db, cpu.a, &cpu.x, &cpu.y, &p);
    cpu.p = p;
}

uint16_t MenuActorCopyGroup(uint8_t bank, uint16_t *xo, uint8_t *p);
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
    uint16_t a;

    MenuChoiceObjectUpdate(*x, p);
    a = 0x0038;
    *p = nz16(*p, a);
    wr16(dbw(bank, 0x11E3), a);
    return MenuChoiceObjectFrames(bank, a, x, y, p);
}

void abi_MenuChoiceObject38(void)
{
    uint8_t p = cpu.p;

    cpu.a = MenuChoiceObject38(cpu.db, &cpu.x, &cpu.y, &p);
    cpu.p = p;
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
    wr16((uint16_t)(*x + 0x08), 0xCB42);
    return MenuChoiceObject38(bank, x, y, p);
}

void abi_MenuChoiceObject38Start(void)
{
    uint8_t p = cpu.p;

    cpu.a = MenuChoiceObject38Start(cpu.db, &cpu.x, &cpu.y, &p);
    cpu.p = p;
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
uint16_t MenuChoiceObject48Start(uint8_t bank, uint16_t *x, uint16_t *y, uint8_t *p)
{
    uint16_t a = rd16(0x70FC0B) & 0x00FF;

    if (a) {
        MenuActorCopyGroup(bank, x, p);
        a = 0xCB3D;
        wr16((uint16_t)(*x + 0x08), a);
        *p = nz16(*p, a);
        return a;
    }
    a = rd16((uint16_t)(*x + 0x14));
    if (a != 0x00FA) {
        wr16((uint16_t)(*x + 0x14), 0x00FA);
        wr16((uint16_t)(*x + 0x16), 0x0000);
        wr16((uint16_t)(*x + 0x18), 0x0000);
    }
    MenuActorCopyGroup(bank, x, p);
    wr16(dbw(bank, 0x11E3), 0x0048);
    MenuActorUploadFrames(bank, *x, 0x0000, x, p);
    *y = 0xFFFF;
    a = 0xCB74;
    wr16((uint16_t)(*x + 0x08), a);
    *p = nz16(*p, a);
    return a;
}

void abi_MenuChoiceObject48Start(void)
{
    uint8_t p = cpu.p;

    cpu.a = MenuChoiceObject48Start(cpu.db, &cpu.x, &cpu.y, &p);
    cpu.p = p;
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
    t = (uint16_t)((rd16((uint16_t)(*x + 0x0A)) + 1) & 0x0001);
    wr16((uint16_t)(*x + 0x0A), t);
    if (!t) {
        v = adc16(rd16((uint16_t)(*x + 0x22)), 0x0001, 0, p);
        wr16((uint16_t)(*x + 0x22), v);
    }
    pos = rd16((uint16_t)(*x + 0x20));
    v = rd16((uint16_t)(*x + 0x22));
    pos = adc16(pos, v, 0, p);
    wr16((uint16_t)(*x + 0x20), pos);
    pos = rd16((uint16_t)(*x + 0x20));
    if ((pos & 0x8000) && pos < 0xFFA8) {
        wr16((uint16_t)(*x + 0x08), 0xCF27);
        SoundPort0QueuePushRaw(0x13);
    }
    pos = rd16((uint16_t)(*x + 0x20));
    v = (uint16_t)(0 - pos);
    FrameByteWritePush(bank, BG1VOFS, (uint8_t)(v >> 8));
    FrameByteWritePush(bank, BG1VOFS, (uint8_t)v);
    pos = rd16((uint16_t)(*x + 0x20));
    v = adc16(pos, 0x004D, 0, p);
    wr16((uint16_t)(*x + 0x06), v);
    *p &= (uint8_t)~(P_M | P_X);
    return v;
}

void abi_MenuBg1RiseAway(void)
{
    uint8_t p = cpu.p;

    cpu.a = MenuBg1RiseAway(cpu.db, &cpu.x, &p);
    cpu.p = p;
}

/*
 * @routine 0B:DF7B MenuActorNoStep
 * WHAT IT DOES: Nothing: the frame handler of an object that does nothing
 * on its own.
 * HOW: RTS.
 * Entry: any. Exit: every register and flag unchanged.
 */
void abi_MenuActorNoStep(void)
{
}

/*
 * @routine 0B:E0DF MenuActorNoStepE0DF
 * WHAT IT DOES: Nothing: as MenuActorNoStep, a second copy.
 * HOW: RTS.
 * Entry: any. Exit: every register and flag unchanged.
 */
void abi_MenuActorNoStepE0DF(void)
{
}

void abi_VramDmaQueueAdd(void);

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
 */
void abi_MenuSelectionFramesCycle(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint16_t obj = cpu.x, n, off, sel;

    n = dec_field(obj, 0x0A);
    cpu.p = nz16(cpu.p, n);
    if (!(n & 0x8000))
        return;
    wr16((uint16_t)(obj + 0x0A), 0x0006);
    n = (uint16_t)((rd16((uint16_t)(obj + 0x22)) + 1) & 0x0003);
    wr16((uint16_t)(obj + 0x22), n);
    off = rd16(0x0BDFCB + (uint32_t)(n << 1));
    wr16(b | 0x11DF, off);
    sel = (uint16_t)(rd16(0x0018CB) << 1);
    cpu.x = sel;
    cpu.a = rd16(0x0BDFD3 + (uint32_t)sel);
    off = rd16(b | 0x11DF);
    cpu.p &= (uint8_t)~P_C;
    cpu.a = op_adc16(cpu.a, off, &cpu.p);
    cpu.p &= (uint8_t)~P_C;
    cpu.a = op_adc16(cpu.a, 0x3000, &cpu.p);
    wr16(b | 0x16B3, cpu.a);
    wr16(b | 0x16B1, 0x0080);
    wr16(b | 0x16B5, 0x0070);
    cpu.a = 0x15C0;
    wr16(b | 0x16B7, cpu.a);
    abi_VramDmaQueueAdd();
    cpu.x = rd16(b | 0x11F9);
    cpu.p = nz16(cpu.p, cpu.x);
}
