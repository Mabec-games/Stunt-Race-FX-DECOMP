/*
 * table_2702.c: a Super FX program that writes a fixed table of twelve
 * four-byte entries at $70:2702: a zero, then three bytes.
 */
#include "gsu.h"
#include "gsu_ops.h"

#define FX_TABLE_2702 0x2702u

static const uint8_t table_2702[48] = {
    0x00, 0x1F, 0x1B, 0x1E,
    0x00, 0xBF, 0xB7, 0xB6,
    0x00, 0xBF, 0xBC, 0xBD,
    0x00, 0xBF, 0xB5, 0xBB,
    0x00, 0x1F, 0x5B, 0x58,
    0x00, 0xBF, 0xB2, 0xB3,
    0x00, 0x1E, 0x15, 0x14,
    0x00, 0x17, 0x19, 0x1A,
    0x00, 0x17, 0x18, 0x1E,
    0x00, 0x17, 0x19, 0x1A,
    0x00, 0x17, 0x18, 0x15,
    0x00, 0x14, 0x15, 0x1E
};

/*
 * @routine 01:D1EB FxTable2702Program
 * WHAT IT DOES: A program the S-CPU starts: writes the fixed 48-byte table
 * at $70:2702.
 * HOW: Sets the RAM bank to 0, then stores each byte with STB through R1,
 * loading it with IBT (zero) or IWT (the rest), INC R1 after each. Then
 * STOP.
 * Entry: started by the S-CPU. Exit: (at the STOP) R1 = $2732; R0 = the
 * last byte; the RAM bank 0; the last RAM address $2731; CY set and OV
 * clear (SUB R0); S and Z from R1.
 */
void FxTable2702Program(void)
{
    gsu_code(0x01D1EC, 10);
    int i;

    gsu_ramb(0);
    for (i = 0; i < 48; i++)
        { gsu_stb((uint16_t)(FX_TABLE_2702 + i), table_2702[i]); gsu_code(0x01D305, 1); }
}

void abi_FxTable2702Program(void)
{
    FxTable2702Program();
    gsu.r[0] = table_2702[47];
    gsu.r[1] = (uint16_t)(FX_TABLE_2702 + 48);
    gsu.cy = 1;
    gsu.ov = 0;
    gsu_sz(gsu.r[1]);
}
