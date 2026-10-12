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
