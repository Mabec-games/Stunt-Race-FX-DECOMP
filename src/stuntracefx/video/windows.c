/*
 * windows.c: the PPU window and colour maths setups the frame code switches
 * between.
 */
#include "machine.h"
#include "cpu_ops.h"
#include "hw.h"

/*
 * @routine 7E:DFE4 WindowsColourAddBg12
 * WHAT IT DOES: Sets the windows and colour maths for adding the sub screen
 * to BG1 and BG2 inside window 1.
 * HOW: W12SEL $99, WOBJSEL $30, CGWSEL $22, CGADSUB $03, WBGLOG $00,
 * WOBJLOG $01, TMW $17, in that order.
 * Entry: any widths. Exit: A low byte = $17; N, Z clear; M 8-bit.
 */
void WindowsColourAddBg12(void)
{
    wr8(W12SEL, 0x99);
    wr8(WOBJSEL, 0x30);
    wr8(CGWSEL, 0x22);
    wr8(CGADSUB, 0x03);
    wr8(WBGLOG, 0x00);
    wr8(WOBJLOG, 0x01);
    wr8(TMW, 0x17);
}

void abi_WindowsColourAddBg12(void)
{
    WindowsColourAddBg12();
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x17);
    cpu.p = (uint8_t)((cpu.p & ~(P_N | P_Z)) | P_M);
}

/*
 * @routine 7E:E01D WindowsColourWindowOnly
 * WHAT IT DOES: Turns the BG windows off and keeps window 1 for colour only.
 * HOW: W12SEL $00, W34SEL $00, WOBJSEL $20, CGWSEL $10, WBGLOG $00,
 * WOBJLOG $00, TMW $03, in that order.
 * Entry: any widths. Exit: A low byte = $03; N, Z clear; M 8-bit.
 */
void WindowsColourWindowOnly(void)
{
    wr8(W12SEL, 0x00);
    wr8(W34SEL, 0x00);
    wr8(WOBJSEL, 0x20);
    wr8(CGWSEL, 0x10);
    wr8(WBGLOG, 0x00);
    wr8(WOBJLOG, 0x00);
    wr8(TMW, 0x03);
}

void abi_WindowsColourWindowOnly(void)
{
    WindowsColourWindowOnly();
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x03);
    cpu.p = (uint8_t)((cpu.p & ~(P_N | P_Z)) | P_M);
}

/*
 * @routine 7E:E085 WindowsAllInverted
 * WHAT IT DOES: Sets every layer's window 1 inverted and colour maths
 * subtracting on all layers.
 * HOW: CGADSUB $BF, W12SEL $AA, W34SEL $AA, WOBJSEL $AA, CGWSEL $10,
 * WBGLOG $00, WOBJLOG $00, TMW $1F, in that order.
 * Entry: any widths. Exit: A low byte = $1F; N, Z clear; M 8-bit.
 */
void WindowsAllInverted(void)
{
    wr8(CGADSUB, 0xBF);
    wr8(W12SEL, 0xAA);
    wr8(W34SEL, 0xAA);
    wr8(WOBJSEL, 0xAA);
    wr8(CGWSEL, 0x10);
    wr8(WBGLOG, 0x00);
    wr8(WOBJLOG, 0x00);
    wr8(TMW, 0x1F);
}

void abi_WindowsAllInverted(void)
{
    WindowsAllInverted();
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x1F);
    cpu.p = (uint8_t)((cpu.p & ~(P_N | P_Z)) | P_M);
}

void ColourMathSub83Windowed(void);

/* Setup 2 ($7E:E048), which no script selects: colour maths $03 (add, on
   BG1 and BG2), then the windows for colour only. Leaves A low byte $03. */
static void windows_colour_add03(void)
{
    wr8(CGADSUB, 0x03);
    WindowsColourWindowOnly();
}

/* Setup 3 ($7E:E054), which no script selects: colour maths $81, no BG
   windows, sprites window 1, colour window only, BG1 on the main screen
   inside the window. Leaves A low byte $01. */
static void windows_colour_sub81(void)
{
    wr8(CGADSUB, 0x81);
    wr8(W12SEL, 0x00);
    wr8(W34SEL, 0x00);
    wr8(WOBJSEL, 0x20);
    wr8(CGWSEL, 0x10);
    wr8(WBGLOG, 0x00);
    wr8(WOBJLOG, 0x00);
    wr8(TMW, 0x01);
}

/*
 * @routine 7E:DFCA WindowsSetupSelect
 * WHAT IT DOES: Applies the window and colour maths setup chosen by $19A9:
 * 0 WindowsColourAddBg12, 2 ColourMathSub83Windowed, 4 colour maths $03
 * with the windows for colour only, 6 colour maths $81 with BG1 alone
 * inside the window, 8 WindowsAllInverted.
 * HOW: An indexed indirect call through the five-entry table at $7E:DFDA,
 * with the data bank set to $7E. Only 0, 2 and 8 are seen in play.
 * Entry: any widths; $19A9 = 0, 2, 4, 6 or 8. Exit: A low byte as the
 * setup leaves it; X = $19A9 (high byte clear); P and the data bank
 * unchanged.
 */
uint8_t WindowsSetupSelect(void)
{
    switch (rd8(0x7E19A9)) {
    case 0: WindowsColourAddBg12(); return 0x17;
    case 2: ColourMathSub83Windowed(); return 0x03;
    case 4: windows_colour_add03(); return 0x03;
    case 6: windows_colour_sub81(); return 0x01;
    default: WindowsAllInverted(); return 0x1F;
    }
}

void abi_WindowsSetupSelect(void)
{
    uint8_t a = WindowsSetupSelect();

    cpu.x = rd8(0x7E19A9);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | a);
}

/*
 * @routine 7E:DF61 BobStep
 * WHAT IT DOES: Steps bob X (0 or 1) at half rate: on every second call
 * ($1A1E + X toggling) its count $1A22 + X goes up; at 10 the count
 * restarts and bit 7 of $0DA4 (bob 0) or $0DA6 (bob 1) is cleared;
 * otherwise its position $1A1C + X moves by its velocity $1A20 + X, which
 * turns round at $1C and $16.
 * HOW: 8-bit; long accesses to bank 0 except the velocity add and the
 * flag bytes (data bank). Bob 1's flag is not reached in the traces so
 * far.
 * Entry: X = bob. Exit: M 8-bit, X 8-bit (high bytes cleared); A low byte
 * and flags from the last step taken.
 */
void abi_BobStep(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint8_t v, c;

    op_sep(P_M | P_X);
    cpu.p &= (uint8_t)~P_C;
    v = op_adc8(rd8(0x001A1E + cpu.x), 0x01, &cpu.p);
    cpu.p = (uint8_t)((nz8(cpu.p, (uint8_t)(v - 0x02)) & ~P_C) | (v >= 0x02 ? P_C : 0));
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    if (v != 0x02) {
        wr8(0x001A1E + cpu.x, v);
        return;
    }
    cpu.p &= (uint8_t)~P_C;
    c = op_adc8(rd8(0x001A22 + cpu.x), 0x01, &cpu.p);
    wr8(0x001A22 + cpu.x, c);
    cpu.p = (uint8_t)((nz8(cpu.p, (uint8_t)(c - 0x0A)) & ~P_C) | (c >= 0x0A ? P_C : 0));
    if (c == 0x0A) {
        uint16_t at = cpu.x == 0x01 ? 0x0DA6 : 0x0DA4;

        wr8(0x001A22 + cpu.x, 0x00);
        cpu.p = (uint8_t)((nz8(cpu.p, (uint8_t)(cpu.x - 0x01)) & ~P_C) | (cpu.x >= 0x01 ? P_C : 0));
        v = (uint8_t)(rd8(b | at) & 0x7F);
        wr8(b | at, v);
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
        cpu.p = nz8(cpu.p, v);
        return;
    }
    wr8(0x001A1E + cpu.x, 0x00);
    v = rd8(0x001A1C + cpu.x);
    cpu.p &= (uint8_t)~P_C;
    v = op_adc8(v, rd8(b | (uint16_t)(0x1A20 + cpu.x)), &cpu.p);
    wr8(0x001A1C + cpu.x, v);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = (uint8_t)((nz8(cpu.p, (uint8_t)(v - 0x1C)) & ~P_C) | (v >= 0x1C ? P_C : 0));
    if (v != 0x1C) {
        cpu.p = (uint8_t)((nz8(cpu.p, (uint8_t)(v - 0x16)) & ~P_C) | (v >= 0x16 ? P_C : 0));
        if (v != 0x16)
            return;
    }
    v = (uint8_t)((rd8(0x001A20 + cpu.x) ^ 0xFF) + 1);
    wr8(0x001A20 + cpu.x, v);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, v);
}
