/*
 * joypad.c: reading the two joypads from the auto-read registers.
 */
#include "machine.h"
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
    uint16_t now1 = rd16(JOY1L);
    uint16_t new1 = (uint16_t)((now1 ^ rd16(JOY1_HELD)) & rd16(JOY1L));
    uint16_t now2;
    uint16_t new2;

    wr16(JOY1_PRESSED, new1);
    wr16(JOY1_PRESSED_COPY, new1);
    now2 = rd16(JOY2L);
    new2 = (uint16_t)((now2 ^ rd16(JOY2_HELD)) & rd16(JOY2L));
    wr16(JOY2_PRESSED, new2);
    wr16(JOY2_PRESSED_COPY, new2);
    wr16(JOY2_HELD, now2);
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

    now = rd16(JOY1L);
    wr16(JOY1_PRESSED_COPY, (uint16_t)(((now ^ rd16(JOY1_LAST)) & rd16(JOY1L)) | rd16(JOY1_PRESSED_COPY)));
    wr16(JOY1_LAST, now);
    now = rd16(JOY2L);
    wr16(JOY2_PRESSED_COPY, (uint16_t)(((now ^ rd16(JOY2_LAST)) & rd16(JOY2L)) | rd16(JOY2_PRESSED_COPY)));
    wr16(JOY2_LAST, now);

    now = rd16(JOY1L);
    wr16(JOY2_PRESSED, (uint16_t)((now ^ rd16(JOY2_HELD)) & rd16(JOY1L)));
    {
        uint16_t now2 = rd16(JOY2L);
        wr16(JOY2_PRESSED, (uint16_t)((now2 ^ rd16(JOY2_HELD)) & rd16(JOY2L)));
        wr16(JOY2_HELD, (uint16_t)(now2 | rd16(JOY2_HELD)));
    }
    held = (uint16_t)(now | rd16(JOY1_HELD));
    wr16(JOY1_HELD, held);
    return held;
}

void abi_JoypadAccumulate(void)
{
    cpu.a = JoypadAccumulate();
}

/* Eight serial reads of data line 2 (bit 1) of joypad port `port` into
   $1182, first bit highest; returns the byte. */
static uint8_t serial_byte(uint32_t b, uint8_t port)
{
    unsigned i;
    uint8_t v;

    wr8(b | 0x1182, 0x00);
    for (i = 0; i < 8; i++) {
        uint8_t bit = (rd8(b | (uint16_t)(0x4016 + port)) & 0x02) ? 1 : 0;

        v = rd8(b | 0x1182);
        wr8(b | 0x1182, (uint8_t)(v << 1 | bit));
    }
    return rd8(b | 0x1182);
}

/*
 * @routine 7F:10CD JoypadPortCheck
 * WHAT IT DOES: Checks joypad port A (0 or 1) for a device on its second
 * data line: with the latch set, eight reads of that line all 1 and then,
 * with the latch clear, eight reads not all 1 set $18FB to 1. The second
 * half is not reached in the traces so far.
 * HOW: The serial bits are shifted into $1182 (ROL); the latch is
 * JOYSER0 bit 0 and is left clear. P is saved and restored.
 * Entry: any widths; A low byte = the port. Exit: A low byte = 0 (A = 0
 * when $18FB was set); X = the port, Y = $FF (8-bit values); P unchanged.
 */
int JoypadPortCheck(uint8_t bank, uint8_t port)
{
    uint32_t b = (uint32_t)bank << 16;
    int found = 0;

    wr8(b | 0x4016, 0x01);
    if (serial_byte(b, port) == 0xFF) {
        wr8(b | 0x4016, 0x00);
        if (serial_byte(b, port) != 0xFF) {
            wr16(0x0018FB, 0x0001);
            found = 1;
        }
    }
    wr8(b | 0x4016, 0x00);
    return found;
}

void abi_JoypadPortCheck(void)
{
    uint8_t port = (uint8_t)cpu.a;

    cpu.a = JoypadPortCheck(cpu.db, port) ? 0x0000 : (uint16_t)(cpu.a & 0xFF00);
    cpu.x = port;
    cpu.y = 0x00FF;
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

    abi_SoundPortsSend();
    abi_JoypadRead();
    cpu.p &= (uint8_t)~P_M;
    for (i = 0; i < 2; i++) {
        cpu.a = rd16(b | (uint16_t)(0x118B + i * 2));
        cpu.a |= rd16(b | (uint16_t)(0x0311 + i * 2));
        cpu.p = nz16(cpu.p, cpu.a);
        wr16(b | (uint16_t)(0x118B + i * 2), cpu.a);
    }
}
