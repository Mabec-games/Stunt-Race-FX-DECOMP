/*
 * screen.c: menu screen setup: scroll registers, the palette buffer and its
 * saved copy, sprite lists and pictures drawn by the Super FX.
 */
#include "machine.h"
#include "hw.h"

uint8_t GsuCF42To3000(uint16_t in68, uint16_t in6a);

/*
 * @routine 0B:F4B2 MenuScrollReset
 * WHAT IT DOES: Sets the horizontal and vertical scroll of BG1-BG3 to 0.
 * HOW: Two zero writes to each scroll register (low then high byte), the
 * horizontal ones first.
 * Entry: any widths. Exit: every register unchanged.
 */
void MenuScrollReset(void)
{
    static const uint32_t reg[6] = {BG1HOFS, BG2HOFS, BG3HOFS, BG1VOFS, BG2VOFS, BG3VOFS};
    unsigned i;

    for (i = 0; i < 6; i++) {
        wr8(reg[i], 0x00);
        wr8(reg[i], 0x00);
    }
}

void abi_MenuScrollReset(void)
{
    MenuScrollReset();
}

/*
 * @routine 0B:F4DB MenuPaletteLoad
 * WHAT IT DOES: Copies a full 256-colour palette into the palette buffer
 * from the long address in direct page $FB.
 * HOW: 256 word copies.
 * Entry: D = $0000. Exit: A = the last colour; X = Y = $200; P unchanged.
 */
uint16_t MenuPaletteLoad(void)
{
    uint32_t src = rd24(0x0000FB);
    uint16_t c = 0;
    unsigned i;

    for (i = 0; i < 0x200; i += 2) {
        c = rd16((src + i) & 0xFFFFFF);
        wr16(0x7EFD65 + i, c);
    }
    return c;
}

void abi_MenuPaletteLoad(void)
{
    cpu.a = MenuPaletteLoad();
    cpu.x = cpu.y = 0x0200;
}

/*
 * @routine 0B:F4F5 MenuPaletteSave
 * WHAT IT DOES: Saves the palette buffer to the copy that follows it
 * ($7E:FF65).
 * HOW: 256 word copies.
 * Entry: any widths. Exit: A = the last colour; X = $200; P unchanged.
 */
uint16_t MenuPaletteSave(void)
{
    uint16_t c = 0;
    unsigned i;

    for (i = 0; i < 0x200; i += 2) {
        c = rd16(0x7EFD65 + i);
        wr16(0x7EFF65 + i, c);
    }
    return c;
}

void abi_MenuPaletteSave(void)
{
    cpu.a = MenuPaletteSave();
    cpu.x = 0x0200;
}

/*
 * @routine 0B:FAE5 MenuOamFromList
 * WHAT IT DOES: Copies words from the list at $FCA2 + Y (data bank) into the
 * sprite table copy at $0375 + X, until a $FFFF word.
 * HOW: A word loop stepping X and Y by 2.
 * Entry: X = sprite table offset, Y = list offset. Exit: X, Y past the last
 * word copied and the end mark; A = $FFFF; M 16-bit, X 16-bit; Z, C set, N
 * clear.
 */
void MenuOamFromList(uint8_t bank, uint16_t *x, uint16_t *y)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t w;

    while ((w = rd16(b | (uint16_t)(0xFCA2 + *y))) != 0xFFFF) {
        wr16(b | (uint16_t)(0x0375 + *x), w);
        *x = (uint16_t)(*x + 2);
        *y = (uint16_t)(*y + 2);
    }
}

void abi_MenuOamFromList(void)
{
    MenuOamFromList(cpu.db, &cpu.x, &cpu.y);
    cpu.a = 0xFFFF;
    cpu.p = (uint8_t)((cpu.p & ~(P_N | P_M | P_X)) | P_Z | P_C);
}

/*
 * @routine 0B:E3D2 MenuPictureToVram
 * WHAT IT DOES: Has the Super FX draw a picture into $70:3000 (program
 * $01:CF42 with inputs A and X) and writes it to VRAM at the current
 * address, a word at a time, up to the byte count at $11D5.
 * HOW: GsuCF42To3000, VMAIN = $80, then VMDATA writes until X reaches the
 * count (read each time through the data bank).
 * Entry: D = $0000; A, X = the picture's inputs. Exit: A = the last word; X
 * = the count; P unchanged.
 */
uint16_t MenuPictureToVram(uint8_t bank, uint16_t in68, uint16_t in6a, uint16_t *x)
{
    uint16_t w;

    GsuCF42To3000(in68, in6a);
    wr8(VMAIN, 0x80);
    *x = 0;
    do {
        w = rd16(0x703000 + *x);
        wr16(VMDATAL, w);
        *x = (uint16_t)(*x + 2);
    } while (*x != rd16((uint32_t)bank << 16 | 0x11D5));
    return w;
}

void abi_MenuPictureToVram(void)
{
    cpu.a = MenuPictureToVram(cpu.db, cpu.a, cpu.x, &cpu.x);
}
