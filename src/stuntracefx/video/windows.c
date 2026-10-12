/*
 * windows.c: the PPU window and colour maths setups the frame code switches
 * between.
 */
#include "machine.h"
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
