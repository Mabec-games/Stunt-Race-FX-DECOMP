/*
 * screen.c: the Super FX's screen buffers. The renderer draws into one of
 * two buffers while the other is shown; $0D7F selects which, and $0D7D holds
 * the matching offset (0 or 8).
 */
#include "machine.h"
#include "hw.h"

/*
 * @routine 03:B83C GsuScreenFlip
 * WHAT IT DOES: Swaps the Super FX's draw buffer and resets its screen base.
 * HOW: Toggles $0D7F between 0 and 1 and sets $0D7D to 8 or 0 to match;
 * gives RAM back to the S-CPU (screen mode value without bit 3), stores the
 * buffer start $2C00 at $70:021A and direct page $48, hands ROM and RAM to
 * the Super FX again (bits 4 and 3) and sets the screen base register to
 * $0B. Work RAM is reached through the data bank; the screen mode stores
 * are 16-bit.
 * Entry: D = $0000. Exit: A = the high byte of the last screen mode value
 * and $0B; M 8-bit; N, Z clear.
 */
uint16_t GsuScreenFlip(uint8_t bank)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t mode;

    if (rd16(b | 0x0D7F)) {
        wr16(b | 0x0D7F, 0x0000);
        wr16(b | 0x0D7D, 0x0008);
    } else {
        wr16(b | 0x0D7F, (uint16_t)(rd16(b | 0x0D7F) + 1));
        wr16(b | 0x0D7D, 0x0000);
    }
    wr16(b | (GSU_SCMR & 0xFFFF), (uint16_t)(rd16(b | 0x0374) & 0xFFF7));
    wr16(0x70021A, 0x2C00);
    wr16(0x000048, 0x2C00);
    mode = (uint16_t)(rd16(b | 0x0374) | 0x0018);
    wr16(b | (GSU_SCMR & 0xFFFF), mode);
    wr8(GSU_SCBR, 0x0B);
    return (uint16_t)((mode & 0xFF00) | 0x0B);
}

void abi_GsuScreenFlip(void)
{
    cpu.a = GsuScreenFlip(cpu.db);
    cpu.p = (uint8_t)((cpu.p & ~(P_N | P_Z)) | P_M);
}
