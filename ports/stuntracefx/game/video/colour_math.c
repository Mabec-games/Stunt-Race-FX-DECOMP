/*
 * colour_math.c: the colour maths settings the raster interrupt copies to the
 * PPU: $0D47 for CGADSUB, and the fixed colour's intensity at $7F:340D.
 */
#include "machine.h"
#include "native.h"
#include "video_calls.h"
#include "carried.h"
#include "wram_names.h"


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
    wr8(W(colour_math_mode), 0xA3);
    cyc(42);
}

void ColourMathA3_cr(Carried *cr);
void ColourMathA3Long_cr(Carried *cr);
void ColourMathA7Start_cr(Carried *cr);
void ColourMathOff_cr(Carried *cr);
void ColourMathOffLong_cr(Carried *cr);
void ColourMathSub83Windowed_cr(Carried *cr);
void FixedColourFromStep_cr(Carried *cr);

void ColourMathA3_cr(Carried *cr)
{
    ColourMathA3();
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0xA3);
    set_p(cr, (uint8_t)((cr->p & ~P_Z) | P_N | P_M));
}

void abi_ColourMathA3(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ColourMathA3_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
    wr8(W(colour_math_mode), 0xA7);
    cyc(78);
    wr16(W(fixed_colour_step), 0x0000);
}

void ColourMathA7Start_cr(Carried *cr)
{
    ColourMathA7Start();
    cr->a = 0;
    cr->x &= 0x00FF;
    cr->y &= 0x00FF;
    set_p(cr, (uint8_t)(((cr->p & ~(P_N | P_M)) | P_Z | P_X)));
}

void abi_ColourMathA7Start(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ColourMathA7Start_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
    fixed_step = rd8(W(fixed_colour_step));
    cyc(86);
    v = rd8(0x03FE7B + fixed_step);
    wr8(W(fixed_colour), v);
    cyc(44);
    return v;
}

void FixedColourFromStep_cr(Carried *cr)
{
    uint8_t v = FixedColourFromStep();

    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    cr->x = fixed_step;
    cr->y &= 0x00FF;
    set_p(cr, (uint8_t)(nz8(cr->p, v) | P_M | P_X));
}

void abi_FixedColourFromStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    FixedColourFromStep_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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

void ColourMathSub83Windowed_cr(Carried *cr)
{
    ColourMathSub83Windowed();
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0x03);
    set_p(cr, (uint8_t)((cr->p & ~(P_N | P_Z)) | P_M));
}

void abi_ColourMathSub83Windowed(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ColourMathSub83Windowed_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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

void ColourMathA3Long_cr(Carried *cr)
{
    (cyc(108), ColourMathA3_cr(cr));
    cyc(44);
}

void abi_ColourMathA3Long(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ColourMathA3Long_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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

void ColourMathOff_cr(Carried *cr)
{
    ColourMathOff(cr->db);
    cr->a &= 0xFF00;
    set_p(cr, (uint8_t)((cr->p & ~P_N) | P_Z | P_M));
}

void abi_ColourMathOff(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ColourMathOff_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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

void ColourMathOffLong_cr(Carried *cr)
{
    cyc(46);
    ColourMathOff_cr(cr);
    cyc(44);
}

void abi_ColourMathOffLong(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ColourMathOffLong_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

