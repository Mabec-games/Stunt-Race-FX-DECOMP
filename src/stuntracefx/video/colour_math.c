/*
 * colour_math.c: the colour maths settings the raster interrupt copies to the
 * PPU: $0D47 for CGADSUB, and the fixed colour's intensity at $7F:340D.
 */
#include "machine.h"

void SoundPort1QueuePush(uint8_t command);

/*
 * @routine 03:FE57 ColourMathA3
 * WHAT IT DOES: Sets the colour maths mode to $A3 (subtract, half, BG1 and
 * BG2).
 * HOW: Stores $A3 to $0D47.
 * Entry: any widths. Exit: A low byte = $A3; M 8-bit; N set, Z clear.
 */
void ColourMathA3(void)
{
    wr8(0x000D47, 0xA3);
}

void abi_ColourMathA3(void)
{
    ColourMathA3();
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0xA3);
    cpu.p = (uint8_t)((cpu.p & ~P_Z) | P_N | P_M);
}

/*
 * @routine 03:FCC9 ColourMathA7Start
 * WHAT IT DOES: Sends sound $04 on port 1, sets the colour maths mode to $A7
 * and the fixed colour step at $7F:3408 to 0.
 * HOW: SoundPort1QueuePush, then two stores.
 * Entry: any widths. Exit: A = 0; M 16-bit, X 8-bit (X, Y high bytes clear);
 * Z set, N clear.
 */
void ColourMathA7Start(void)
{
    SoundPort1QueuePush(0x04);
    wr8(0x000D47, 0xA7);
    wr16(0x7F3408, 0x0000);
}

void abi_ColourMathA7Start(void)
{
    ColourMathA7Start();
    cpu.a = 0;
    cpu.x &= 0x00FF;
    cpu.y &= 0x00FF;
    cpu.p = (uint8_t)(((cpu.p & ~(P_N | P_M)) | P_Z | P_X));
}

/*
 * @routine 03:FE6B FixedColourFromStep
 * WHAT IT DOES: Sets the fixed colour's intensity from the step at $7F:3408,
 * through the table after this routine.
 * HOW: $7F:340D = table[$7F:3408].
 * Entry: any widths. Exit: A low byte = the intensity; X = the step; M and X
 * 8-bit; N, Z from A.
 */
uint8_t FixedColourFromStep(void)
{
    uint8_t step = rd8(0x7F3408), v = rd8(0x03FE7B + step);

    wr8(0x7F340D, v);
    return v;
}

void abi_FixedColourFromStep(void)
{
    uint8_t step = rd8(0x7F3408), v = FixedColourFromStep();

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.x = step;
    cpu.y &= 0x00FF;
    cpu.p = (uint8_t)(nz8(cpu.p, v) | P_M | P_X);
}
