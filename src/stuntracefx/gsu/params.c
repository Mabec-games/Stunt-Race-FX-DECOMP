/*
 * params.c: the parameter block in Super FX RAM that the renderer reads.
 */
#include "machine.h"

/*
 * @routine 7E:E2C9 GsuParamsUpdate
 * WHAT IT DOES: Copies the view parameters the renderer needs from work RAM
 * into its parameter block in Super FX RAM.
 * HOW: Eight 16-bit copies: $0D12 to $70:108A; direct page $C5, $C7, $C9 to
 * $70:00D8, 00DA, 00DC; $0664, $0666, $0668 to $70:00DE, 00E0, 00E2; $05FA to
 * $70:2736.
 * Entry: any widths, D = $0000. Exit: A = the last value copied; P unchanged.
 */
uint16_t GsuParamsUpdate(void)
{
    static const uint32_t from[8] = {0x000D12, 0x0000C5, 0x0000C7, 0x0000C9, 0x000664, 0x000666, 0x000668, 0x0005FA};
    static const uint32_t to[8] = {0x70108A, 0x7000D8, 0x7000DA, 0x7000DC, 0x7000DE, 0x7000E0, 0x7000E2, 0x702736};
    uint16_t v = 0;
    unsigned i;

    for (i = 0; i < 8; i++) {
        v = rd16(from[i]);
        wr16(to[i], v);
    }
    return v;
}

void abi_GsuParamsUpdate(void)
{
    cpu.a = GsuParamsUpdate();
}

/*
 * @routine 03:AEB6 GsuParamsFromDp
 * WHAT IT DOES: Marks $05EF with $FF and copies eight direct-page words into
 * the renderer's inputs at $70:0034-$70:003E, $70:0092 and $70:0094.
 * HOW: Word copies from $CE, $D0, $2A, $2E, $2C, $30, $32, $34, in that
 * order; $05EF is reached through the data bank.
 * Entry: M 8-bit, D = $0000. Exit: A = the last word; M 16-bit; N, Z from A.
 */
uint16_t GsuParamsFromDp(uint8_t bank)
{
    static const uint8_t from[8] = {0xCE, 0xD0, 0x2A, 0x2E, 0x2C, 0x30, 0x32, 0x34};
    static const uint32_t to[8] = {0x700034, 0x700036, 0x700038, 0x70003A, 0x70003C, 0x70003E, 0x700092, 0x700094};
    uint16_t v = 0;
    unsigned i;

    wr8((uint32_t)bank << 16 | 0x05EF, 0xFF);
    for (i = 0; i < 8; i++) {
        v = rd16(from[i]);
        wr16(to[i], v);
    }
    return v;
}

void abi_GsuParamsFromDp(void)
{
    cpu.a = GsuParamsFromDp(cpu.db);
    cpu.p = (uint8_t)(nz16(cpu.p, cpu.a) & ~P_M);
}
