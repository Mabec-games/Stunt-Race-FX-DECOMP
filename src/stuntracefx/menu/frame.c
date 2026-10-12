/*
 * frame.c: the menu's frame handshake. The menu code sets bit 0 of $11D8
 * when it has finished a frame; the menu's vertical blank code clears $11D8
 * once it has applied the frame's updates.
 */
#include "machine.h"

/*
 * @routine 0B:E390 MenuWaitFrame
 * WHAT IT DOES: Marks the frame finished and waits for the vertical blank
 * code to take it.
 * HOW: Sets bit 0 of $11D8 (16-bit, through the data bank) and polls it
 * until it is zero.
 * Entry: any widths. Exit: A = 0; P unchanged.
 */
void MenuWaitFrame(uint8_t bank)
{
    uint32_t a = (uint32_t)bank << 16 | 0x11D8;

    wr16(a, (uint16_t)(rd16(a) | 0x0001));
    while (rd16(a))
        ;
}

void abi_MenuWaitFrame(void)
{
    MenuWaitFrame(cpu.db);
    cpu.a = 0x0000;
}
