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
