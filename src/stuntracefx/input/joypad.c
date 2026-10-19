/*
 * joypad.c: reading the two joypads from the auto-read registers.
 */
#include "machine.h"
#include "cpu_ops.h"
#include "hw.h"
#include "wram.h"

/*
 * @routine 7E:B1AE JoypadRead
 * WHAT IT DOES: Reads both joypads: the buttons held now, and the buttons
 * pressed since the last read.
 * HOW: For each pad, newly pressed = (now XOR held last time) AND now, stored
 * to the pressed byte and its copy; then now becomes the held state. The
 * auto-read register is read twice per pad, as the original does.
 * Entry: any M, X 8-bit. Exit: A = joypad 1 now; X and Y high bytes clear;
 * P unchanged.
 */
uint16_t JoypadRead(void)
{
    cyc(144);
    uint16_t now1 = rd16(JOY1L);
    cyc(54);
    uint16_t new1 = (uint16_t)((now1 ^ rd16(JOY1_HELD)) & rd16(cyc_then(32, JOY1L)));
    uint16_t now2;
    uint16_t new2;

    cyc(24);
    wr16(JOY1_PRESSED, new1);
    cyc(24);
    wr16(JOY1_PRESSED_COPY, new1);
    cyc(32);
    now2 = rd16(JOY2L);
    cyc(54);
    new2 = (uint16_t)((now2 ^ rd16(JOY2_HELD)) & rd16(cyc_then(32, JOY2L)));
    cyc(24);
    wr16(JOY2_PRESSED, new2);
    cyc(24);
    wr16(JOY2_PRESSED_COPY, new2);
    cyc(60);
    wr16(JOY2_HELD, now2);
    cyc(60);
    wr16(JOY1_HELD, now1);
    return now1;
}

void abi_JoypadRead(void)
{
    cpu.a = JoypadRead();
    cpu.x &= 0x00FF;
    cpu.y &= 0x00FF;
}

/*
 * @routine 7E:B1E1 JoypadAccumulate
 * WHAT IT DOES: Folds this frame's joypad state into the frame code's
 * accumulated presses and held buttons, for frames where the game logic runs
 * less often than the interrupt.
 * HOW: Per pad: presses since its own last sample are ORed into the pressed
 * copy, and now becomes that last sample. Then, as the original does, both
 * pads' presses are computed against joypad 2's held state and written to
 * joypad 2's pressed byte (the second overwrites the first), and both held
 * states have now ORed in.
 * Entry: any widths. Exit: A = joypad 1 now OR its held state; P unchanged.
 */
uint16_t JoypadAccumulate(void)
{
    uint16_t now, held;

    cyc(68);
    now = rd16(JOY1L);
    cyc(54);
    wr16(JOY1_PRESSED_COPY, cyc_then(24, (uint16_t)(((now ^ rd16(JOY1_LAST)) & rd16(cyc_then(24, JOY1L))) | rd16(cyc_then(24, JOY1_PRESSED_COPY)))));
    cyc(60);
    wr16(JOY1_LAST, now);
    cyc(24);
    now = rd16(JOY2L);
    cyc(54);
    wr16(JOY2_PRESSED_COPY, cyc_then(24, (uint16_t)(((now ^ rd16(JOY2_LAST)) & rd16(cyc_then(24, JOY2L))) | rd16(cyc_then(24, JOY2_PRESSED_COPY)))));
    cyc(60);
    wr16(JOY2_LAST, now);

    cyc(24);
    now = rd16(JOY1L);
    cyc(54);
    wr16(JOY2_PRESSED, cyc_then(24, (uint16_t)((now ^ rd16(JOY2_HELD)) & rd16(cyc_then(24, JOY1L)))));
    {
        cyc(24);
        uint16_t now2 = rd16(JOY2L);
        cyc(54);
        wr16(JOY2_PRESSED, cyc_then(24, (uint16_t)((now2 ^ rd16(JOY2_HELD)) & rd16(cyc_then(24, JOY2L)))));
        cyc(60);
        wr16(JOY2_HELD, cyc_then(24, (uint16_t)(now2 | rd16(JOY2_HELD))));
    }
    cyc(60);
    held = (uint16_t)(now | rd16(JOY1_HELD));
    cyc(24);
    wr16(JOY1_HELD, held);
    return held;
}

void abi_JoypadAccumulate(void)
{
    cpu.a = JoypadAccumulate();
}

/* ROL $1182 with the carry `bit`. */
static void serial_shift(uint32_t b, uint8_t bit)
{
    uint8_t v = rd8(b | 0x1182);

    cyc(6);
    wr8(b | 0x1182, (uint8_t)(v << 1 | bit));
}

/* The serial loop from count y (7 down to 0): each pass one read of data
   line 2 (bit 1) of joypad port `port` shifted into $1182; returns the
   byte. */
static uint8_t serial_rest(uint32_t b, uint8_t port, int y)
{
    for (; y >= 0; y--)
        serial_shift(b, (rd8(b | (uint16_t)(0x4016 + port)) & 0x02) ? 1 : 0);
    return rd8(b | 0x1182);
}

/* Eight serial reads into a cleared $1182, first bit highest. */
static uint8_t serial_byte(uint32_t b, uint8_t port)
{
    wr8(b | 0x1182, 0x00);
    return serial_rest(b, port, 7);
}

/* JoypadPortCheck from `stage`: 0 the start (latch set, first eight
   reads); 1 the first loop's set-bit arm at count y; 2 the second half
   (latch clear, eight more reads); 3 the second loop's set-bit arm at
   count y. Returns 1 when $18FB was set. */
static int port_check(uint32_t b, uint8_t port, int stage, int y)
{
    int found = 0;
    uint8_t v;

    if (stage <= 1) {
        if (stage == 0) {
            wr8(b | 0x4016, 0x01);
            cyc(24);
            v = serial_byte(b, port);
        } else {
            cyc(38);
            serial_shift(b, 1);
            cyc(22);
            v = serial_rest(b, port, y - 1);
        }
        cyc(38);
        if (v != 0xFF)
            goto done;
        stage = 2;
    }
    if (stage == 2) {
        cyc(40);
        wr8(b | 0x4016, 0x00);
        cyc(24);
        v = serial_byte(b, port);
    } else {
        cyc(38);
        serial_shift(b, 1);
        cyc(22);
        v = serial_rest(b, port, y - 1);
    }
    if (v != 0xFF) {
        cyc(110);
        wr16(0x0018FB, 0x0001);
        found = 1;
    }
done:
    cyc(62);
    wr8(b | 0x4016, 0x00);
    cyc(42);
    return found;
}

/* An inner entry's end: A, X, Y as JoypadPortCheck leaves them, then P
   pulled as its PHP pushed it. */
static void port_check_entry(int stage)
{
    uint8_t port = (uint8_t)cpu.x;

    cpu.a = port_check((uint32_t)cpu.db << 16, port, stage, (uint8_t)cpu.y) ? 0x0000
        : (uint16_t)(cpu.a & 0xFF00);
    cpu.x = port;
    cpu.y = 0x00FF;
    op_plp(rd8((uint16_t)(cpu.s + 1)));
    cpu.s = (uint16_t)(cpu.s + 1);
}

/*
 * @routine 7F:10CD JoypadPortCheck
 * WHAT IT DOES: Checks joypad port A (0 or 1) for a device on its second
 * data line: with the latch set, eight reads of that line all 1 and then,
 * with the latch clear, eight reads not all 1 set $18FB to 1 (a Super
 * Multitap answers so). With a plain joypad the line reads 0, so the
 * set-bit arms and the second half run only as the inner entries
 * JoypadPortCheckBit, JoypadPortCheckLatchOff and JoypadPortCheckBit2.
 * HOW: The serial bits are shifted into $1182 (ROL); the latch is
 * JOYSER0 bit 0 and is left clear. P is saved and restored.
 * Entry: any widths; A low byte = the port. Exit: A low byte = 0 (A = 0
 * when $18FB was set); X = the port, Y = $FF (8-bit values); P unchanged.
 */
int JoypadPortCheck(uint8_t bank, uint8_t port)
{
    return port_check((uint32_t)bank << 16, port, 0, 0);
}

void abi_JoypadPortCheck(void)
{
    uint8_t port = (uint8_t)cpu.a;

    cpu.a = JoypadPortCheck(cpu.db, port) ? 0x0000 : (uint16_t)(cpu.a & 0xFF00);
    cpu.x = port;
    cpu.y = 0x00FF;
}

/*
 * @routine 7F:10E2 JoypadPortCheckBit
 * WHAT IT DOES: JoypadPortCheck's first loop at a read of 1 on the second
 * data line: that bit shifted into $1182, the loop's remaining reads, and
 * the rest of the check.
 * HOW: SEC, ROL $1182, then the loop (DEY, BPL); as JoypadPortCheck.
 * Entry: M, X 8-bit; X = the port, Y = the loop count (0-7), $1182 the
 * bits so far; JoypadPortCheck's P on the stack. Exit: as
 * JoypadPortCheck.
 */
void abi_JoypadPortCheckBit(void)
{
    port_check_entry(1);
}

/*
 * @routine 7F:10F6 JoypadPortCheckLatchOff
 * WHAT IT DOES: JoypadPortCheck's second half, after eight reads of 1 with
 * the latch set: the latch cleared, eight more reads of the second data
 * line into $1182, and $18FB = 1 unless they are all 1.
 * HOW: STA JOYSER0, STZ $1182, the serial loop; a 16-bit store.
 * Entry: M, X 8-bit; X = the port; JoypadPortCheck's P on the stack.
 * Exit: as JoypadPortCheck.
 */
void abi_JoypadPortCheckLatchOff(void)
{
    port_check_entry(2);
}

/*
 * @routine 7F:1107 JoypadPortCheckBit2
 * WHAT IT DOES: JoypadPortCheck's second loop at a read of 1 on the second
 * data line: that bit shifted into $1182, the remaining reads, and the
 * end of the check.
 * HOW: SEC, ROL $1182, then the loop; as JoypadPortCheck.
 * Entry: M, X 8-bit; X = the port, Y = the loop count (0-7), $1182 the
 * bits so far; JoypadPortCheck's P on the stack. Exit: as
 * JoypadPortCheck.
 */
void abi_JoypadPortCheckBit2(void)
{
    port_check_entry(3);
}

void abi_SoundPortsSend(void);
void abi_JoypadRead(void);

/*
 * @routine 7E:A7A3 FrameSoundAndJoypads
 * WHAT IT DOES: The frame's sound output (SoundPortsSend) and joypad read
 * (JoypadRead), then adds this frame's presses ($0311, $0313) to the ones
 * the menus collect at $118B and $118D.
 * HOW: 16-bit ORs through the data bank.
 * Entry: as JoypadRead. Exit: A = $118D; M 16-bit; N, Z from it.
 */
void abi_FrameSoundAndJoypads(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    unsigned i;

    (cyc(46), abi_SoundPortsSend());
    abi_JoypadRead();
    cpu.p &= (uint8_t)~P_M;
    cyc(92);
    for (i = 0; i < 2; i++) {
        cyc(24);
        cpu.a = rd16(b | (uint16_t)(0x118B + i * 2));
        cyc(24);
        cpu.a |= rd16(b | (uint16_t)(0x0311 + i * 2));
        cpu.p = nz16(cpu.p, cpu.a);
        cyc(24);
        wr16(b | (uint16_t)(0x118B + i * 2), cpu.a);
    }
}
