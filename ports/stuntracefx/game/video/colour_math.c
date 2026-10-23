/*
 * colour_math.c: the colour maths settings the raster interrupt copies to the
 * PPU: $0D47 for CGADSUB, and the fixed colour's intensity at $7F:340D.
 */
#include "machine.h"
#include "native.h"
#include "video_calls.h"
#include "carried.h"

void SoundPort1QueuePushW(uint8_t command, unsigned wide);

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
    cyc(42);
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
    cyc(100);
    SoundPort1QueuePushW(0x04, 0);      /* after SEP #$30 */
    cyc(40);
    wr8(0x000D47, 0xA7);
    cyc(78);
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
static uint8_t fixed_step;              /* the step read, for the adapter's X */

uint8_t FixedColourFromStep(void)
{
    uint8_t v;

    cyc(54);
    fixed_step = rd8(0x7F3408);
    cyc(86);
    v = rd8(0x03FE7B + fixed_step);
    wr8(0x7F340D, v);
    cyc(44);
    return v;
}

void abi_FixedColourFromStep(void)
{
    uint8_t v = FixedColourFromStep();

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.x = fixed_step;
    cpu.y &= 0x00FF;
    cpu.p = (uint8_t)(nz8(cpu.p, v) | P_M | P_X);
}

/* FixedColourFromStep on carried registers, as abi_FixedColourFromStep leaves them. */
void FixedColourFromStep_cr(Carried *cr)
{
    uint8_t v = FixedColourFromStep();

    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    cr->x = fixed_step;
    cr->y &= 0x00FF;
    cr->p = (uint8_t)(nz8(cr->p, v) | P_M | P_X);
}

void WindowsColourWindowOnly(void);

/*
 * @routine 7E:E011 ColourMathSub83Windowed
 * WHAT IT DOES: Sets colour maths to $83 (subtract, on BG1 and BG2) and the
 * windows to colour only.
 * HOW: CGADSUB = $83 (a long store), then WindowsColourWindowOnly.
 * Entry: any widths. Exit: A low byte = $03; N, Z clear; M 8-bit.
 */
void ColourMathSub83Windowed(void)
{
    cyc(70);
    ppu_wr(0x002131, 0x83);
    (cyc(46), WindowsColourWindowOnly());
    cyc(42);
}

void abi_ColourMathSub83Windowed(void)
{
    ColourMathSub83Windowed();
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x03);
    cpu.p = (uint8_t)((cpu.p & ~(P_N | P_Z)) | P_M);
}

void abi_ColourMathA3(void);

/*
 * @routine 03:FE53 ColourMathA3Long
 * WHAT IT DOES: ColourMathA3 for callers in other banks.
 * HOW: JSR to it, then RTL.
 * Entry: as ColourMathA3. Exit: as ColourMathA3.
 */
void ColourMathA3Long(void)
{
    ColourMathA3();
}

void abi_ColourMathA3Long(void)
{
    (cyc(108), abi_ColourMathA3());
    cyc(44);
}

/*
 * @routine 03:FE63 ColourMathOff
 * WHAT IT DOES: Sets the colour maths mode at $0D47 to 0 (none).
 * HOW: One byte store through the data bank.
 * Entry: any widths. Exit: A low byte = 0; M 8-bit; Z set, N clear.
 */
void ColourMathOff(uint8_t bank)
{
    cyc(62);
    wr8((uint32_t)bank << 16 | 0x0D47, 0x00);
    cyc(42);
}

void abi_ColourMathOff(void)
{
    ColourMathOff(cpu.db);
    cpu.a &= 0xFF00;
    cpu.p = (uint8_t)((cpu.p & ~P_N) | P_Z | P_M);
}

void abi_ColourMathOff(void);

/*
 * @routine 03:FE5F ColourMathOffLong
 * WHAT IT DOES: ColourMathOff for callers in other banks.
 * HOW: JSR to it, then RTL.
 * Entry: as ColourMathOff. Exit: as ColourMathOff.
 */
void ColourMathOffLong(uint8_t bank)
{
    ColourMathOff(bank);
}

void abi_ColourMathOffLong(void)
{
    cyc(46);
    abi_ColourMathOff();
    cyc(44);
}
