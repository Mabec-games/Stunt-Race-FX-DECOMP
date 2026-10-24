/*
 * joypad.c: reading the two joypads from the auto-read registers.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "hw.h"
#include "carried.h"
#include "wram_names.h"


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
    cyc(98);
    uint16_t now1 = io_rd16(JOY1L, input_pad(0));
    cyc(54);
    uint16_t new1 = (uint16_t)((now1 ^ rd16(W(joy1_held))) & io_rd16(cyc_then(32, JOY1L), input_pad(0)));
    uint16_t now2;
    uint16_t new2;

    cyc(24);
    wr16(W(joy1_pressed), new1);
    cyc(24);
    wr16(W(joy1_pressed_copy), new1);
    cyc(32);
    now2 = io_rd16(JOY2L, input_pad(1));
    cyc(54);
    new2 = (uint16_t)((now2 ^ rd16(W(joy2_held))) & io_rd16(cyc_then(32, JOY2L), input_pad(1)));
    cyc(24);
    wr16(W(joy2_pressed), new2);
    cyc(24);
    wr16(W(joy2_pressed_copy), new2);
    cyc(60);
    wr16(W(joy2_held), now2);
    cyc(60);
    wr16(W(joy1_held), now1);
    return now1;
}

void FrameSoundAndJoypads(Carried *cr);
void JoypadAccumulate_cr(Carried *cr);
void JoypadPortCheck_cr(Carried *cr);
void JoypadPortCheckBit(Carried *cr);
void JoypadPortCheckBit2(Carried *cr);
void JoypadPortCheckLatchOff(Carried *cr);
void JoypadRead_cr(Carried *cr);
static void port_check_entry(Carried *cr, int stage);
void SoundPortsSend_cr(Carried *cr);

void JoypadRead_cr(Carried *cr)
{
    cr->a = JoypadRead();
    cr->x &= 0x00FF;
    cr->y &= 0x00FF;
}

void abi_JoypadRead(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    JoypadRead_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
    now = io_rd16(JOY1L, input_pad(0));
    cyc(54);
    wr16(W(joy1_pressed_copy), cyc_then(24, (uint16_t)(((now ^ rd16(W(joy1_last))) & io_rd16(cyc_then(24, JOY1L), input_pad(0))) | rd16(cyc_then(24, W(joy1_pressed_copy))))));
    cyc(60);
    wr16(W(joy1_last), now);
    cyc(24);
    now = io_rd16(JOY2L, input_pad(1));
    cyc(54);
    wr16(W(joy2_pressed_copy), cyc_then(24, (uint16_t)(((now ^ rd16(W(joy2_last))) & io_rd16(cyc_then(24, JOY2L), input_pad(1))) | rd16(cyc_then(24, W(joy2_pressed_copy))))));
    cyc(60);
    wr16(W(joy2_last), now);

    cyc(24);
    now = io_rd16(JOY1L, input_pad(0));
    cyc(54);
    wr16(W(joy2_pressed), cyc_then(24, (uint16_t)((now ^ rd16(W(joy2_held))) & io_rd16(cyc_then(24, JOY1L), input_pad(0)))));
    {
        cyc(24);
        uint16_t now2 = io_rd16(JOY2L, input_pad(1));
        cyc(54);
        wr16(W(joy2_pressed), cyc_then(24, (uint16_t)((now2 ^ rd16(W(joy2_held))) & io_rd16(cyc_then(24, JOY2L), input_pad(1)))));
        cyc(60);
        wr16(W(joy2_held), cyc_then(24, (uint16_t)(now2 | rd16(W(joy2_held)))));
    }
    cyc(60);
    held = (uint16_t)(now | rd16(W(joy1_held)));
    cyc(24);
    wr16(W(joy1_held), held);
    cyc(70);
    return held;
}

void JoypadAccumulate_cr(Carried *cr)
{
    cr->a = JoypadAccumulate();
}

void abi_JoypadAccumulate(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    JoypadAccumulate_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* ROL $1182 with the carry `bit`. */
static void serial_shift(uint32_t b, uint8_t bit)
{
    cyc(38);
    uint8_t v = rd8(0x7E0000 | W(joy_serial_bits));

    cyc(6);
    wr8(0x7E0000 | W(joy_serial_bits), (uint8_t)(v << 1 | bit));
}

/* The serial loop from count y (7 down to 0): each pass one read of data
   line 2 (bit 1) of joypad port `port` shifted into $1182; returns the
   byte. */
static uint8_t serial_rest(uint32_t b, uint8_t port, int y, uint16_t lead)
{
    for (; y >= 0; y--, lead = 60)
        { cyc(lead); serial_shift(b, (io_rd8(b | (uint16_t)(0x4016 + port), 0x00) & 0x02) ? (cyc(32), 1) : (cyc(38), 0)); }
    cyc(54);
    return rd8(0x7E0000 | W(joy_serial_bits));
}

/* Eight serial reads into a cleared $1182, first bit highest. */
static uint8_t serial_byte(uint32_t b, uint8_t port)
{
    cyc(24);
    wr8(0x7E0000 | W(joy_serial_bits), 0x00);
    return serial_rest(b, port, 7, 40);
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
            cyc(198);
            io_wr8(b | 0x4016, 0x01);
            v = serial_byte(b, port);
        } else {
            serial_shift(b, 1);
            cyc(22);
            v = serial_rest(b, port, y - 1, 60);
        }
        cyc(38);
        if (v != 0xFF)
            goto done;
        stage = 2;
    }
    if (stage == 2) {
        cyc(40);
        io_wr8(b | 0x4016, 0x00);
        v = serial_byte(b, port);
    } else {
        serial_shift(b, 1);
        cyc(22);
        v = serial_rest(b, port, y - 1, 60);
    }
    if (v != 0xFF) {
        cyc(110);
        wr16(W(wram_18FB), 0x0001);
        found = 1;
    }
done:
    cyc(62);
    io_wr8(b | 0x4016, 0x00);
    cyc(42);
    return found;
}

/* An inner entry's end: A, X, Y as JoypadPortCheck leaves them, then P
   pulled as its PHP pushed it. */
static void port_check_entry(Carried *cr, int stage)
{
    uint8_t port = (uint8_t)cr->x;

    cr->a = port_check((uint32_t)cr->db << 16, port, stage, (uint8_t)cr->y) ? 0x0000
        : (uint16_t)(cr->a & 0xFF00);
    cr->x = port;
    cr->y = 0x00FF;
    r_plp(cr, rd8((uint16_t)(cpu.s + 1)));
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

void JoypadPortCheck_cr(Carried *cr)
{
    uint8_t port = (uint8_t)cr->a;

    cr->a = JoypadPortCheck(cr->db, port) ? 0x0000 : (uint16_t)(cr->a & 0xFF00);
    cr->x = port;
    cr->y = 0x00FF;
}

void abi_JoypadPortCheck(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    JoypadPortCheck_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
void JoypadPortCheckBit(Carried *cr)
{
    port_check_entry(cr, 1);
}

void abi_JoypadPortCheckBit(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    JoypadPortCheckBit(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
void JoypadPortCheckLatchOff(Carried *cr)
{
    port_check_entry(cr, 2);
}

void abi_JoypadPortCheckLatchOff(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    JoypadPortCheckLatchOff(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
void JoypadPortCheckBit2(Carried *cr)
{
    port_check_entry(cr, 3);
}

void abi_JoypadPortCheckBit2(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    JoypadPortCheckBit2(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
void FrameSoundAndJoypads(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;
    unsigned i;

    (cyc(46), SoundPortsSend_cr(cr));
    (cyc(46), JoypadRead_cr(cr));
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cyc(92);
    for (i = 0; i < 2; i++) {
        cyc(24);
        cr->a = rd16(0x7E0000 | (uint16_t)(W(menu_joy1_pressed) + i * 2));
        cyc(24);
        cr->a |= rd16(0x7E0000 | (uint16_t)(W(joy1_pressed) + i * 2));
        set_p(cr, nz16(cr->p, cr->a));
        cyc(24);
        wr16(0x7E0000 | (uint16_t)(W(menu_joy1_pressed) + i * 2), cr->a);
    }
    cyc(42);
}

void abi_FrameSoundAndJoypads(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    FrameSoundAndJoypads(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

