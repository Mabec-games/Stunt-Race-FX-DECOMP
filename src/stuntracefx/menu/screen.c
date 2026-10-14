/*
 * screen.c: menu screen setup: scroll registers, the palette buffer and its
 * saved copy, sprite lists and pictures drawn by the Super FX.
 */
#include "machine.h"
#include "cpu_ops.h"
#include "hw.h"

uint8_t GsuCF42To3000(uint16_t in68, uint16_t in6a);

/*
 * @routine 0B:F4B2 MenuScrollReset
 * WHAT IT DOES: Sets the horizontal and vertical scroll of BG1-BG3 to 0.
 * HOW: Two zero writes to each scroll register (low then high byte), the
 * horizontal ones first.
 * Entry: any widths. Exit: X and Y high bytes cleared (the routine switches
 * to 8-bit index registers); A and P unchanged.
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
    cpu.x &= 0x00FF;
    cpu.y &= 0x00FF;
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

/*
 * @routine 0B:F50C MenuHdmaSetup
 * WHAT IT DOES: Points HDMA channels 7, 5 and 4 at TM (one byte per line,
 * tables at $7F:1315, $7F:131A, $7F:131F) and channel 6 at WH0-WH1 (two
 * bytes, table at $7F:1324). Enabling them is left to the caller.
 * HOW: Mode, B-bus register, table address (16-bit store) and bank for each
 * channel, in the order 7, 6, 5, 4; the registers are reached through the
 * data bank.
 * Entry: any widths. Exit: A low byte = $7F; X = $131F; P unchanged.
 */
void MenuHdmaSetup(uint8_t bank)
{
    static const struct { uint16_t base; uint8_t mode, reg; uint16_t table; } ch[4] = {
        {0x4370, 0x00, 0x2C, 0x1315}, {0x4360, 0x01, 0x26, 0x1324},
        {0x4350, 0x00, 0x2C, 0x131A}, {0x4340, 0x00, 0x2C, 0x131F}};
    uint32_t b = (uint32_t)bank << 16;
    unsigned i;

    for (i = 0; i < 4; i++) {
        wr8(b | ch[i].base, ch[i].mode);
        wr8(b | (uint16_t)(ch[i].base + 1), ch[i].reg);
        wr16(b | (uint16_t)(ch[i].base + 2), ch[i].table);
        wr8(b | (uint16_t)(ch[i].base + 4), 0x7F);
    }
}

void abi_MenuHdmaSetup(void)
{
    MenuHdmaSetup(cpu.db);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x7F);
    cpu.x = 0x131F;
}

/*
 * @routine 0B:F5E6 MenuHdmaSetupB
 * WHAT IT DOES: Points HDMA channel 7 at BG2HOFS (indirect, table $7F:133A,
 * data bank 0), 6 at CGWSEL ($7F:1341), 5 at TMW ($7F:1348) and 4 at WH0
 * ($7F:1353), sets $7F:1354 to $FF and the positions at $18BB/$18BF to 0 and
 * $18BD/$18C1 to $A8.
 * HOW: Register stores through the data bank in channel order 7, 6, 5, 4,
 * then the bank $7F byte and four 16-bit long stores.
 * Entry: any widths. Exit: A = $00A8; X = $1353; P unchanged.
 */
void MenuHdmaSetupB(uint8_t bank)
{
    static const struct { uint16_t base; uint8_t mode, reg; uint16_t table; } ch[4] = {
        {0x4370, 0x42, 0x0F, 0x133A}, {0x4360, 0x00, 0x30, 0x1341},
        {0x4350, 0x00, 0x2E, 0x1348}, {0x4340, 0x00, 0x26, 0x1353}};
    uint32_t b = (uint32_t)bank << 16;
    unsigned i;

    for (i = 0; i < 4; i++) {
        wr8(b | ch[i].base, ch[i].mode);
        wr8(b | (uint16_t)(ch[i].base + 1), ch[i].reg);
        wr16(b | (uint16_t)(ch[i].base + 2), ch[i].table);
        wr8(b | (uint16_t)(ch[i].base + 4), 0x7F);
        if (i == 0)
            wr8(b | 0x4377, 0x00);
    }
    wr8(0x7F1354, 0xFF);
    wr16(0x0018BB, 0x0000);
    wr16(0x0018BF, 0x0000);
    wr16(0x0018BD, 0x00A8);
    wr16(0x0018C1, 0x00A8);
}

void abi_MenuHdmaSetupB(void)
{
    MenuHdmaSetupB(cpu.db);
    cpu.a = 0x00A8;
    cpu.x = 0x1353;
}

/*
 * @routine 0B:E303 MenuPaletteScale
 * WHAT IT DOES: Writes Y + 1 colours into the palette buffer from X on, each
 * the saved colour ($7E:FF65 + X) with its red, green and blue scaled by
 * the factor at $11DF / 32.
 * HOW: Three 8 x 8 hardware multiplies per colour (component x factor, the
 * product's bits 5-9 kept); $11E3 holds the colour and $11E1 the result
 * as it is built (red, then green, then blue).
 * Entry: X 16-bit; X = byte offset of the first colour, Y = count - 1.
 * Exit: A = the last colour; X past it; Y = $FFFF; P unchanged.
 */
static uint16_t scale_component(uint32_t b, uint8_t v)
{
    wr8(WRMPYA, v);
    wr8(WRMPYB, rd8(b | 0x11DF));
    return rd16(RDMPYL);
}

uint16_t MenuPaletteScale(uint8_t bank, uint16_t *x, uint16_t *y)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t c, r = 0, prod;

    do {
        c = rd16(0x7EFF65 + *x);
        wr16(b | 0x11E3, c);
        prod = scale_component(b, (uint8_t)(c & 0x1F));
        wr16(b | 0x11E1, (uint16_t)(((prod << 3) >> 8) & 0x1F));
        c = rd16(b | 0x11E3);
        prod = scale_component(b, (uint8_t)((c & 0x03E0) >> 5));
        wr16(b | 0x11E1, (uint16_t)((prod & 0x03E0) | rd16(b | 0x11E1)));
        c = rd16(b | 0x11E3);
        prod = scale_component(b, (uint8_t)((c & 0x7C00) >> 10));
        r = (uint16_t)((((prod & 0x03E0) >> 3) << 8) | rd16(b | 0x11E1));
        wr16(b | 0x11E1, r);
        wr16(0x7EFD65 + *x, r);
        *x = (uint16_t)(*x + 2);
        *y = (uint16_t)(*y - 1);
    } while (!(*y & 0x8000));
    return r;
}

void abi_MenuPaletteScale(void)
{
    cpu.a = MenuPaletteScale(cpu.db, &cpu.x, &cpu.y);
}

/* A picture word with its tile number raised by $100: kept in the picture
   and copied to the second picture at $70:3800. */
static uint16_t raise_tile(uint16_t x)
{
    uint16_t w = (uint16_t)(rd16(0x703000 + x) + 0x0100);

    wr16(0x703000 + x, w);
    wr16(0x703800 + x, w);
    return w;
}

/*
 * @routine 0B:F6EA MenuPictureInsetGrid
 * WHAT IT DOES: Raises the tile numbers of the $380-word picture at
 * $70:3000 by $100 into a copy at $70:3800, and in the picture itself puts
 * an 11-row by 13-column block of tiles numbered from $11E1 (priority set),
 * rows one tile apart and columns $0B apart.
 * HOW: Rows of 32 words from word $180: 4 raised, 13 replaced in the picture
 * (the copy still gets the raised original), 15 raised; $11DF counts the
 * rows and $11E1 the first tile of each row. The words before and after
 * the block are only raised.
 * Entry: any widths. Exit: A = the last word; X = $700; Y = $FFFF; P
 * unchanged.
 */
uint16_t MenuPictureInsetGrid(uint8_t bank)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t x = 0, a = 0, rows, w;
    int i;

    for (i = 0; i < 0x180; i++, x = (uint16_t)(x + 2))
        a = raise_tile(x);
    wr16(b | 0x11DF, 0x000A);
    do {
        for (i = 0; i < 4; i++, x = (uint16_t)(x + 2))
            raise_tile(x);
        a = (uint16_t)(rd16(b | 0x11E1) | 0x2000);
        for (i = 0; i < 13; i++, x = (uint16_t)(x + 2)) {
            wr16(0x703800 + x, (uint16_t)(rd16(0x703000 + x) + 0x0100));
            wr16(0x703000 + x, a);
            a = (uint16_t)(a + 0x0B);
        }
        for (i = 0; i < 15; i++, x = (uint16_t)(x + 2))
            a = raise_tile(x);
        wr16(b | 0x11E1, (uint16_t)(rd16(b | 0x11E1) + 1));
        rows = (uint16_t)(rd16(b | 0x11DF) - 1);
        wr16(b | 0x11DF, rows);
    } while (!(rows & 0x8000));
    for (i = 0; i < 0xA0; i++, x = (uint16_t)(x + 2))
        a = w = raise_tile(x);
    return a;
}

void abi_MenuPictureInsetGrid(void)
{
    cpu.a = MenuPictureInsetGrid(cpu.db);
    cpu.x = 0x0700;
    cpu.y = 0xFFFF;
}

/* Draw a picture with GsuCF42To3000 and write `bytes` of it to VRAM word
   address `vram` (VMAIN as the caller left it). */
static uint16_t picture_to_vram(uint8_t bank, uint16_t in68, uint16_t in6a, uint16_t vram, uint16_t bytes, int draw)
{
    uint16_t x, w = 0;

    (void)bank;
    if (draw)
        GsuCF42To3000(in68, in6a);
    wr16(VMADDL, vram);
    for (x = 0; x != bytes; x = (uint16_t)(x + 2)) {
        w = rd16(0x703000 + x);
        wr16(VMDATAL, w);
    }
    return w;
}

/* The two words of a course's picture entry: input $68 and input $6A. */
static void course_picture(uint8_t bank, uint32_t table, uint16_t *in68, uint16_t *in6a)
{
    uint16_t c = rd16((uint32_t)bank << 16 | 0x0E3D), i = (uint16_t)(((uint16_t)(c << 1) + c) & 0x00FF);

    *in68 = rd16(table + i);
    *in6a = rd16(table + 2 + i) & 0x00FF;
}

/*
 * @routine 0B:F94A MenuCourseGraphics
 * WHAT IT DOES: Draws the course screen's graphics into VRAM: the course's
 * two pictures (tables $03:F4F9 and $03:F5A1, three bytes per course) to
 * $4C00 ($1800 bytes) and $3C00 ($2000 bytes), picture $FB84 to both $1680
 * and $3680 ($600 bytes each) and picture $FFAC to $1C00 ($800 bytes).
 * HOW: Each picture is drawn by the Super FX (GsuCF42To3000) and written a
 * word at a time to VMDATA.
 * Entry: any widths, D = $0000. Exit: A = the last word; X = $800; P
 * unchanged.
 */
uint16_t MenuCourseGraphics(uint8_t bank)
{
    uint16_t a, x;

    course_picture(bank, 0x03F4F9, &a, &x);
    picture_to_vram(bank, a, x, 0x4C00, 0x1800, 1);
    course_picture(bank, 0x03F5A1, &a, &x);
    picture_to_vram(bank, a, x, 0x3C00, 0x2000, 1);
    picture_to_vram(bank, 0xFB84, 0x0017, 0x1680, 0x0600, 1);
    picture_to_vram(bank, 0, 0, 0x3680, 0x0600, 0);
    return picture_to_vram(bank, 0xFFAC, 0x0017, 0x1C00, 0x0800, 1);
}

void abi_MenuCourseGraphics(void)
{
    cpu.a = MenuCourseGraphics(cpu.db);
    cpu.x = 0x0800;
}

/*
 * @routine 0B:C9DD MenuSpritePalettesScale
 * WHAT IT DOES: Scales sprite palettes 5 and 1 (colours 208-215 and
 * 144-151) from their saved copy by A / 32.
 * HOW: MenuPaletteScale twice, from byte offsets $1A0 and $120, with the
 * factor stored at $11DF before each.
 * Entry: M 16-bit, X 16-bit; A = factor (0-32). Exit: A = the last colour;
 * X = $11F9; Y = $FFFF; N, Z from X.
 */
uint16_t MenuSpritePalettesScale(uint8_t bank, uint16_t factor)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t x = 0x01A0, y = 0x0007;

    wr16(b | 0x11DF, factor);
    MenuPaletteScale(bank, &x, &y);
    wr16(b | 0x11DF, factor);
    x = 0x0120;
    y = 0x0007;
    return MenuPaletteScale(bank, &x, &y);
}

void abi_MenuSpritePalettesScale(void)
{
    cpu.a = MenuSpritePalettesScale(cpu.db, cpu.a);
    cpu.x = rd16((uint32_t)cpu.db << 16 | 0x11F9);
    cpu.y = 0xFFFF;
    cpu.p = nz16(cpu.p, cpu.x);
}

/* One cross-fade frame for count n: colours 64-127 scaled by f / 32 and
   colours 12-15 by (32 - f) / 32, f = $0B:CFE5[n with bit 0 clear], the
   second factor made as ((f - 1) EOR $1F) + 1; both through $11DF. Returns
   the last colour; *factor = the second factor. */
static uint16_t cross_fade(uint8_t bank, uint16_t n, uint16_t *factor)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t x = 0x0080, y = 0x003F;

    wr16(b | 0x11DF, rd16(0x0BCFE5 + (uint16_t)(n & 0xFFFE)));
    MenuPaletteScale(bank, &x, &y);
    *factor = (uint16_t)(((uint16_t)(rd16(b | 0x11DF) - 1) ^ 0x001F) + 1);
    wr16(b | 0x11DF, *factor);
    x = 0x0018;
    y = 0x0003;
    return MenuPaletteScale(bank, &x, &y);
}

/*
 * @routine 0B:CFAB MenuCrossFadeStep
 * WHAT IT DOES: One frame of a 64-frame cross-fade, as object X's handler:
 * colours 64-127 are scaled from their saved copy by f / 32 and colours
 * 12-15 by (32 - f) / 32, f from the table at $0B:CFE5 for the frame
 * count +$0A; after the last frame the object's handler becomes $CE89.
 * HOW: The count is incremented first; at $40 only the handler is changed.
 * The table is read at the count with bit 0 cleared; the second factor is
 * ((f - 1) EOR $1F) + 1. Both go through $11DF to MenuPaletteScale.
 * Entry: M 16-bit, X 16-bit, D = $0000; X = record. Exit: last frame: A =
 * $CE89; N set, Z clear; C set. Otherwise: A = the last colour; X = $0020;
 * Y = $FFFF; N, Z from the second factor; C clear.
 */
int MenuCrossFadeStep(uint8_t bank, uint16_t rec, uint16_t *a, uint16_t *factor)
{
    uint16_t n = (uint16_t)(rd16((uint16_t)(rec + 0x0A)) + 1);

    if (n == 0x0040) {
        *a = 0xCE89;
        wr16((uint16_t)(rec + 0x08), *a);
        return 0;
    }
    wr16((uint16_t)(rec + 0x0A), n);
    *a = cross_fade(bank, n, factor);
    return 1;
}

void abi_MenuCrossFadeStep(void)
{
    uint16_t f;

    if (MenuCrossFadeStep(cpu.db, cpu.x, &cpu.a, &f)) {
        cpu.x = 0x0020;
        cpu.y = 0xFFFF;
        cpu.p = (uint8_t)(nz16(cpu.p, f) & ~P_C);
    } else {
        cpu.p = (uint8_t)((nz16(cpu.p, cpu.a)) | P_C);
    }
}

/* Draws picture $DC8C (input $1C) into $70:3000 with the Super FX, then
   lays the tile grid from tile `first` over it (MenuPictureInsetGrid). */
static uint16_t picture_dc8c_grid(uint8_t bank, uint16_t first)
{
    GsuCF42To3000(0xDC8C, 0x001C);
    wr16((uint32_t)bank << 16 | 0x11E1, first);
    return MenuPictureInsetGrid(bank);
}

/*
 * @routine 0B:F6B6 MenuPictureDC8CGrid0
 * WHAT IT DOES: Draws picture $DC8C and lays over it the 11 x 13 tile grid
 * numbered from tile 0.
 * HOW: GsuCF42To3000 with $DC8C and $1C, $11E1 = 0, MenuPictureInsetGrid;
 * P saved and restored around it.
 * Entry: X 16-bit. Exit: A = the last picture word; X = $0700; Y = $FFFF;
 * P unchanged.
 */
uint16_t MenuPictureDC8CGrid0(uint8_t bank)
{
    return picture_dc8c_grid(bank, 0x0000);
}

void abi_MenuPictureDC8CGrid0(void)
{
    cpu.a = MenuPictureDC8CGrid0(cpu.db);
    cpu.x = 0x0700;
    cpu.y = 0xFFFF;
}

/*
 * @routine 0B:F6D0 MenuPictureDC8CGrid58
 * WHAT IT DOES: As MenuPictureDC8CGrid0, the grid numbered from tile $58.
 * HOW: As MenuPictureDC8CGrid0.
 * Entry: X 16-bit. Exit: as MenuPictureDC8CGrid0.
 */
uint16_t MenuPictureDC8CGrid58(uint8_t bank)
{
    return picture_dc8c_grid(bank, 0x0058);
}

void abi_MenuPictureDC8CGrid58(void)
{
    cpu.a = MenuPictureDC8CGrid58(cpu.db);
    cpu.x = 0x0700;
    cpu.y = 0xFFFF;
}

uint16_t FrameByteWritePush(uint8_t bank, uint16_t addr, uint8_t value);
void VramDmaQueueAdd(void);

/*
 * @routine 0B:D14A MenuCrossFadeOut
 * WHAT IT DOES: The cross-fade of MenuCrossFadeStep run backwards, as object
 * X's handler: +$0A counts down by 2 and picks the factors; when it would
 * go below 0, sets the screen up for the menu's graphics loader and makes
 * MenuLoadPictureCC78 ($D1F7) the handler: $80 bytes from $0B:D56C queued
 * to VRAM $7000, and for the next vertical blank BG1 tiles at $0000 and
 * BG2 tiles at $7000, the BG2 map at $7C00 (64 x 32), mode 3, only BG2 on
 * the main screen, HDMA off.
 * HOW: cross_fade with the new count; the setup through VramDmaQueueAdd and
 * FrameByteWritePush.
 * Entry: M 16-bit, X 16-bit, D = $0000; X = record. Exit: fading: as
 * MenuCrossFadeStep. Done: A = $D1F7; X unchanged; N set, Z clear; M
 * 16-bit, X 16-bit; C, V unchanged.
 */
int MenuCrossFadeOut(uint8_t bank, uint16_t rec, uint16_t *a, uint16_t *factor)
{
    static const struct { uint32_t reg; uint8_t v; } set[] = {
        {BG12NBA, 0x70}, {BG2SC, 0x7D}, {BGMODE, 0x03}, {TM, 0x02}, {HDMAEN, 0x00}};
    uint16_t n = (uint16_t)(rd16((uint16_t)(rec + 0x0A)) - 2);
    unsigned i;

    if (!(n & 0x8000)) {
        wr16((uint16_t)(rec + 0x0A), n);
        *a = cross_fade(bank, n, factor);
        return 1;
    }
    wr16(0x0016B1, 0x0080);
    wr16(0x0016B5, 0x000B);
    wr16(0x0016B3, 0xD56C);
    wr16(0x0016B7, 0x7000);
    VramDmaQueueAdd();
    for (i = 0; i < sizeof set / sizeof set[0]; i++)
        FrameByteWritePush(bank, (uint16_t)set[i].reg, set[i].v);
    *a = 0xD1F7;
    wr16((uint16_t)(rec + 0x08), *a);
    return 0;
}

void abi_MenuCrossFadeOut(void)
{
    uint16_t f;

    if (MenuCrossFadeOut(cpu.db, cpu.x, &cpu.a, &f)) {
        cpu.x = 0x0020;
        cpu.y = 0xFFFF;
        cpu.p = (uint8_t)(nz16(cpu.p, f) & ~P_C);
    } else {
        cpu.p = (uint8_t)((nz16(cpu.p, cpu.a) & ~(P_M | P_X)));
    }
}

void SoundReset(void);
void SoundPort0QueuePush(uint8_t command);
void MenuWaitFrame(uint8_t bank);
void MenuActorPoolInit(uint8_t bank);
uint16_t MenuActorSpawnList(uint8_t bank, uint16_t list, uint16_t *a, uint16_t *y);
void MenuActorsRun(void);
uint16_t MenuOamBuild(uint16_t *x, uint16_t *y);

/* The end of a menu screen's opening, with DB = 0: an empty object pool,
   the objects of the spawn list made, run once and drawn into the sprite
   table; then $0D3F = $1E, the IRQ at line $E0 with automatic joypad
   reading, frame updates 2 and 3 asked for, and one frame waited. The
   objects run with M 16-bit, X 16-bit and C set, as the copy loops before
   leave them. */
static void menu_screen_start(uint16_t list, uint8_t p)
{
    uint16_t a = 0x2802, y = 0x002C;  /* as MenuActorPoolInit leaves A and Y */

    MenuActorPoolInit(0x00);
    cpu.x = MenuActorSpawnList(0x00, list, &a, &y);
    cpu.a = a;
    cpu.y = y;
    cpu.p = (uint8_t)((p & ~(P_M | P_X)) | P_C);
    MenuActorsRun();
    cpu.a = MenuOamBuild(&cpu.x, &cpu.y);
    wr8(0x000D3F, 0x1E);
    wr8(VTIMEL, 0xE0);
    wr8(VTIMEH, 0x00);
    wr8(0x0011D7, 0x0C);
    wr8(NMITIMEN, 0x21);
    MenuWaitFrame(0x00);
}

/*
 * @routine 0B:F33C MenuScreenOpen0
 * WHAT IT DOES: Opens the menu screen built from spawn list 0: with the
 * display off, sets mode 1 (BG3 priority) with BG1-BG3 maps at $6000,
 * $6400 and $7C00, tiles at 0, $2000 and $7000 (BG3 and BG4), colour math
 * and the sub screen off; resets the sound and the scroll; loads the
 * palette from $1C:989C (colour 0 then black) and $720 bytes of tiles from
 * $1C:DC8C to VRAM $4000; clears the menu's lists and state; makes the
 * objects of spawn list 0, runs them once and builds the sprite table; then
 * turns on the IRQ at line $E0 and automatic joypad reading, plays sound
 * $39, waits three frames and shows only sprites, with $11DA = $11DB =
 * $0F.
 * HOW: DB = 0 throughout; the display and interrupts are written directly,
 * the frame updates are requested through $11D7 = $0C. The palette source
 * goes through direct page $FB.
 * Entry: D = $0000. Exit: A = $0010; X, Y as MenuOamBuild left them; P and
 * DB unchanged.
 */
void MenuScreenOpen0(void)
{
    static const struct { uint32_t reg; uint8_t v; } ppu[] = {
        {BGMODE, 0x09}, {BG1SC, 0x60}, {BG2SC, 0x65}, {BG3SC, 0x7C},
        {BG12NBA, 0x20}, {BG34NBA, 0x77}, {CGADSUB, 0x00}, {CGWSEL, 0x00}, {TS, 0x00}};
    uint8_t p = cpu.p, db = cpu.db;
    uint16_t x;
    unsigned i;

    cpu.db = 0x00;
    wr8(INIDISP, 0x80);
    wr8(HDMAEN, 0x00);
    wr8(MDMAEN, 0x00);
    wr8(0x0011DA, 0x00);
    wr8(NMITIMEN, 0x00);
    for (i = 0; i < sizeof ppu / sizeof ppu[0]; i++)
        wr8(ppu[i].reg, ppu[i].v);
    SoundReset();
    MenuScrollReset();
    wr16(0x0000FB, 0x989C);
    wr16(0x0000FD, 0x001C);
    MenuPaletteLoad();
    wr16(0x7EFD65, 0x0000);
    wr16(VMADDL, 0x4000);
    for (x = 0; x != 0x0720; x = (uint16_t)(x + 2))
        wr16(VMDATAL, rd16(0x1CDC8C + x));
    wr16(0x00162D, 0x0000);
    wr16(0x0016AF, 0x0000);
    wr16(0x000D7D, 0x0006);
    wr16(0x0018FD, 0x0000);
    wr16(0x001903, 0x0001);
    wr16(0x0018B9, 0x0000);
    wr16(0x001189, 0x1234);
    wr16(0x00118F, 0x0000);
    wr16(0x0011EB, 0x0000);
    wr16(0x0011ED, 0x0000);
    wr16(0x0011EF, 0x0000);
    menu_screen_start(0x0000, p);
    SoundPort0QueuePush(0x39);
    MenuWaitFrame(0x00);
    MenuWaitFrame(0x00);
    wr8(0x0011DA, 0x0F);
    wr8(0x0011DB, 0x0F);
    wr8(TM, 0x10);
    cpu.a = 0x0010;
    cpu.db = db;
    cpu.p = p;
}

void abi_MenuScreenOpen0(void)
{
    MenuScreenOpen0();
}

void SoundDataLoad(uint16_t which, uint16_t input);
void ApuUpload(uint8_t bank, uint16_t addr);

/* A sound block upload as the menu does it: port 1 (and 2) = 5, a delay,
   the upload. The delays are counted loops; they cost time, not state. */
static void menu_apu_upload(uint8_t bank, uint16_t addr)
{
    wr16(APUIO1, 0x0005);
    ApuUpload(bank, addr);
}

/* Picture `in68` (Super FX input $1C) to VRAM `vram`; its byte count is
   the big-endian word before it in bank $1C. */
static void menu_picture(uint16_t vram, uint16_t in68)
{
    uint16_t x, n = rd16(0x1C0000 | (uint16_t)(in68 - 2));

    wr16(VMADDL, vram);
    wr16(0x0011D5, (uint16_t)((n << 8) | (n >> 8)));
    MenuPictureToVram(0x00, in68, 0x001C, &x);
}

/*
 * @routine 0B:EE24 MenuScreenOpen6
 * WHAT IT DOES: Opens the menu screen built from spawn list 6: with the
 * display off, sets mode 1 (BG3 priority) with BG1-BG3 maps at $6000,
 * $6400 and $7C00, tiles at 0, $2000 and $7000, colour math adding BG2 (on
 * the sub screen) to BG1, no fixed colour and no window masks; loads three
 * sound blocks (SoundDataLoad $F3B5 with input $1E, then $1B:85F8 and
 * $19:9A98), resets the sound and sends 1 on port 0; resets the scroll;
 * loads the palette from $1C:989C, saves it, and scales colours 64-127 to
 * 1/32 and 12-15 to full; sets up the menu HDMA; draws seven pictures from
 * bank $1C into VRAM ($DC38 at $4000, $C9DC at $6800, $BABC at 0, $BE58 at
 * $6000, $C664 at $2000, $C818 at $6400, $BECC at $7C00) and copies $40
 * bytes from $1C:BE58 to $7000; clears the menu's lists, state and input;
 * then menu_screen_start with spawn list 6 and BG1-BG3 and sprites on the
 * main screen.
 * HOW: DB = 0 throughout. Each sound upload is preceded by port 1 = 5 and
 * a delay loop of $4000 steps (the last two followed by another).
 * Entry: D = $0000. Exit: A = $0017; X, Y as MenuOamBuild left them; P and
 * DB unchanged.
 */
void MenuScreenOpen6(void)
{
    static const struct { uint32_t reg; uint8_t v; } ppu[] = {
        {BGMODE, 0x09}, {BG1SC, 0x60}, {BG2SC, 0x65}, {BG3SC, 0x7C},
        {BG12NBA, 0x20}, {BG34NBA, 0x77}, {CGADSUB, 0x01}, {CGWSEL, 0x02}, {TS, 0x02},
        {COLDATA, 0x00}, {TSW, 0x00}, {TMW, 0x00}};
    uint8_t p = cpu.p, db = cpu.db;
    uint16_t x, y;
    unsigned i;

    cpu.db = 0x00;
    wr8(INIDISP, 0x80);
    wr8(HDMAEN, 0x00);
    wr8(MDMAEN, 0x00);
    wr8(0x0011DA, 0x00);
    wr8(NMITIMEN, 0x00);
    for (i = 0; i < sizeof ppu / sizeof ppu[0]; i++)
        wr8(ppu[i].reg, ppu[i].v);
    wr16(APUIO1, 0x0005);
    SoundDataLoad(0xF3B5, 0x001E);
    menu_apu_upload(0x1B, 0x85F8);
    menu_apu_upload(0x19, 0x9A98);
    SoundReset();
    wr8(APUIO0, 0x01);
    MenuScrollReset();
    wr16(0x0000FB, 0x989C);
    wr16(0x0000FD, 0x001C);
    MenuPaletteLoad();
    MenuPaletteSave();
    wr16(0x0011DF, 0x0001);
    x = 0x0080;
    y = 0x003F;
    MenuPaletteScale(0x00, &x, &y);
    wr16(0x0011DF, 0x0020);
    x = 0x0018;
    y = 0x0003;
    MenuPaletteScale(0x00, &x, &y);
    MenuHdmaSetup(0x00);
    menu_picture(0x4000, 0xDC38);
    menu_picture(0x6800, 0xC9DC);
    wr16(VMADDL, 0x7000);
    for (x = 0; x != 0x0040; x = (uint16_t)(x + 2))
        wr16(VMDATAL, rd16(0x1CBE58 + x));
    menu_picture(0x0000, 0xBABC);
    menu_picture(0x6000, 0xBE58);
    menu_picture(0x2000, 0xC664);
    menu_picture(0x6400, 0xC818);
    menu_picture(0x7C00, 0xBECC);
    wr16(0x00162D, 0x0000);
    wr16(0x0016AF, 0x0000);
    wr16(0x000D7D, 0x0006);
    wr16(0x0018FD, 0x0000);
    wr16(0x00118B, 0x0000);
    wr16(0x000311, 0x0000);
    wr16(0x001903, 0x0000);
    wr16(0x0018B9, 0x0000);
    wr16(0x001189, 0x1234);
    wr16(0x00118F, 0x0000);
    wr16(0x0011EB, 0x0000);
    wr16(0x0011ED, 0x0000);
    wr16(0x0011EF, 0x0000);
    menu_screen_start(0x0006, p);
    wr8(TM, 0x17);
    cpu.a = 0x0017;
    cpu.db = db;
    cpu.p = p;
}

void abi_MenuScreenOpen6(void)
{
    MenuScreenOpen6();
}

/*
 * @routine 0B:EDE7 PictureCellAttributes
 * WHAT IT DOES: Gives the 2 x 2 cell at X in the Super FX picture map
 * ($70:3000 + X, + 2, + $40, + $42) the attribute bits $11DF over each
 * tile number (its low 9 bits kept).
 * HOW: AND #$01FF, ORA $11DF per word; P saved and restored.
 * Entry: X 16-bit; X = the cell. Exit: P restored; A = the last word.
 */
void abi_PictureCellAttributes(void)
{
    static const uint16_t at[4] = {0x0000, 0x0002, 0x0040, 0x0042};
    uint32_t b;
    uint8_t p0 = cpu.p;
    int i;

    op_rep(P_M | P_X);
    b = (uint32_t)cpu.db << 16;
    for (i = 0; i < 4; i++) {
        uint32_t a = 0x703000 + (uint32_t)(uint16_t)(cpu.x + at[i]);

        op_lda16(a);
        op_and16(0x01FF);
        cpu.a |= rd16(b | 0x11DF);
        wr16(a, cpu.a);
    }
    op_plp(p0);
}

void abi_SoundDataLoad(void);
void abi_ApuUpload(void);
void abi_SoundReset(void);
void abi_SaveChecksum(void);
void abi_WindowHdmaChannelsSetup(void);
void abi_GsuCF42To3000(void);
void abi_MenuActorPoolInit(void);
void abi_MenuActorSpawnList(void);
void abi_MenuActorsRun(void);
void abi_MenuOamBuild(void);
void abi_MenuWaitFrame(void);

/* PHP, REP #$30, port 1 = 5 (a word store), a delay, the load with A and
   Y, (REP #$20 and a delay after an upload), PLP. */
static void champion_sound(void (*load)(void), uint16_t a, uint16_t y, int delay_after)
{
    uint8_t p = cpu.p;

    cpu.p &= (uint8_t)~(P_M | P_X);
    cpu.a = 0x0005;
    wr16(APUIO1, 0x0005);
    cpu.a = 0x0000;
    cpu.p = (uint8_t)((cpu.p & ~P_N) | P_Z);
    cpu.a = a;
    cpu.y = y;
    cpu.p = nz16(cpu.p, y);
    load();
    if (delay_after) {
        cpu.p &= (uint8_t)~P_M;
        cpu.a = 0x0000;
        cpu.p = (uint8_t)((cpu.p & ~P_N) | P_Z);
    }
    op_plp(p);
}

/* LDX #vram, STX VMADDL, LDA.L size (big-endian), XBA, STA $11D5, then
   MenuPictureToVram with A and X. */
static void champion_picture(uint16_t vram, uint32_t size, uint16_t in68, uint16_t in6a)
{
    cpu.x = vram;
    cpu.p = nz16(cpu.p, vram);
    wr16(VMADDL, vram);
    cpu.a = rd16(size);
    op_xba();
    wr16(0x0011D5, cpu.a);
    cpu.a = in68;
    cpu.x = in6a;
    cpu.p = nz16(cpu.p, in6a);
    abi_MenuPictureToVram();
}

/* CLC, ADC #m (16-bit). */
static uint16_t champion_add(uint16_t a, uint16_t m)
{
    cpu.p &= (uint8_t)~P_C;
    return op_adc16(a, m, &cpu.p);
}

/*
 * @routine 0B:EA3B ChampionScreenOpen
 * WHAT IT DOES: Opens the championship's closing screen: with the display
 * off, mode 3 with BG1 and BG2 maps at $6000 and $6400, no colour math, the
 * windows for sprites only; three sound blocks (SoundDataLoad $F884 with
 * input $1D, ApuUpload $1B:85F8 and $19:9A98), the sound reset and 1 on
 * port 0; the scroll reset. When the phase $0DD6 is 8 (won) the class
 * $0D4A (0-2) sets its bit (1, 2 or 4) in $70:FC0B; the save's checksum
 * renewed. The palette from $04:EBCB loaded and saved, the window HDMA set
 * up; pictures drawn by the Super FX: $1D:9068 copied to $7F:A7D7 ($2000
 * bytes), $1C:DC8C to VRAM $6000 (each word + $100), $18:C770, $18:998C
 * and $1C:989C to VRAM $4000, $2000 and 0; $04:E87B coloured for the
 * class, row by row from the tables at $0B:ED9F, $0BEDD7 and $0BEDCF, the
 * two cells at $0628 and $052E given the selection's attributes ($0B:EDDF)
 * and written to VRAM $6400 ($1000 bytes); then $04:EBCB drawn. The menu's
 * lists and state cleared ($1905 = 3, $11EF = $66, $1919 = class * 32,
 * $1927 = 0 when won, else $20), the objects of spawn list $36 made, run
 * and drawn; HDMA channels 4-7, $0D3F = $20, the IRQ at line $E0, frame
 * updates 2 and 3, a frame waited and BG1 and BG2 on the main screen.
 * HOW: DB = 0 throughout; P and DB pushed. Each sound load is preceded by
 * port 1 = 5 and a delay. Classes 1 and 2 when won, and the phase other
 * than 8, are not reached in the traces so far.
 * Entry: any widths; D = $0000. Exit: P and DB restored; A, X, Y as
 * MenuWaitFrame leaves them.
 */
void abi_ChampionScreenOpen(void)
{
    static const struct { uint32_t reg; uint8_t v; } ppu[] = {
        {BGMODE, 0x03}, {BG1SC, 0x60}, {BG2SC, 0x65}, {BG12NBA, 0x00}};
    static const uint32_t off[] = {CGADSUB, CGWSEL, TS, COLDATA, TSW};
    uint8_t p0 = cpu.p, db0 = cpu.db, v, bit;
    uint16_t x, a, yo, y2, y3, x2, sx;
    unsigned i;

    op_sep(P_M);
    op_rep(P_X);
    cpu.a = (uint16_t)(cpu.a & 0xFF00);
    cpu.db = 0x00;
    cpu.p = nz8(cpu.p, 0x00);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x80);
    wr8(INIDISP, 0x80);
    wr8(HDMAEN, 0x00);
    wr8(MDMAEN, 0x00);
    wr8(0x0011DA, 0x00);
    wr8(NMITIMEN, 0x00);
    for (i = 0; i < sizeof ppu / sizeof ppu[0]; i++)
        wr8(ppu[i].reg, ppu[i].v);
    for (i = 0; i < sizeof off / sizeof off[0]; i++)
        wr8(off[i], 0x00);
    wr8(WOBJSEL, 0x03);
    wr8(WH0, 0x00);
    wr8(WH1, 0x00);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x10);
    cpu.p = nz8(cpu.p, 0x10);
    wr8(TMW, 0x10);
    champion_sound(abi_SoundDataLoad, 0xF884, 0x001D, 0);
    champion_sound(abi_ApuUpload, 0x001B, 0x85F8, 1);
    champion_sound(abi_ApuUpload, 0x0019, 0x9A98, 1);
    abi_SoundReset();
    op_sep(P_M);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x01);
    cpu.p = nz8(cpu.p, 0x01);
    wr8(APUIO0, 0x01);
    abi_MenuScrollReset();
    op_sep(P_M);
    op_lda8(0x000DD6);
    v = (uint8_t)cpu.a;
    cpu.p = (uint8_t)((nz8(cpu.p, (uint8_t)(v - 0x08)) & ~P_C) | (v >= 0x08 ? P_C : 0));
    if (v == 0x08) {
        op_lda8(0x000D4A);
        v = (uint8_t)cpu.a;
        bit = v == 0x00 ? 0x01 : v == 0x01 ? 0x02 : v == 0x02 ? 0x04 : 0x00;
        cpu.p = (uint8_t)((nz8(cpu.p, (uint8_t)(v - (v < 0x02 ? v : 0x02))) & ~P_C) | P_C);
        if (v > 0x02)
            cpu.p = (uint8_t)((nz8(cpu.p, (uint8_t)(v - 0x02)) & ~P_C) | P_C);
        if (bit) {
            v = (uint8_t)(rd8(0x70FC0B) | bit);
            cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
            cpu.p = nz8(cpu.p, v);
            wr8(0x70FC0B, v);
        }
    }
    abi_SaveChecksum();
    cpu.p &= (uint8_t)~(P_M | P_X);
    cpu.a = 0xEBCB;
    wr16(0x0000FB, 0xEBCB);
    cpu.a = 0x0004;
    cpu.p = nz16(cpu.p, 0x0004);
    wr16(0x0000FD, 0x0004);
    abi_MenuPaletteLoad();
    abi_MenuPaletteSave();
    abi_WindowHdmaChannelsSetup();
    cpu.p &= (uint8_t)~(P_M | P_X);
    cpu.a = 0x9068;
    cpu.x = 0x001D;
    cpu.p = nz16(cpu.p, 0x001D);
    abi_GsuCF42To3000();
    cpu.p &= (uint8_t)~(P_M | P_X);
    for (x = 0x0000; x != 0x2000; x = (uint16_t)(x + 2)) {
        cpu.a = rd16(0x703000 + x);
        wr16(0x7FA7D7 + x, cpu.a);
    }
    cpu.x = x;
    cpu.p = (uint8_t)((cpu.p & ~P_N) | P_Z | P_C);
    cpu.a = 0xDC8C;
    cpu.x = 0x001C;
    cpu.p = nz16(cpu.p, 0x001C);
    abi_GsuCF42To3000();
    cpu.p &= (uint8_t)~(P_M | P_X);
    cpu.a = 0x6000;
    wr16(VMADDL, 0x6000);
    for (x = 0x0000; x != 0x0800; x = (uint16_t)(x + 2)) {
        a = rd16(0x703000 + x);
        cpu.a = champion_add(a, 0x0100);
        wr16(VMDATAL, cpu.a);
    }
    cpu.x = x;
    cpu.p = (uint8_t)((cpu.p & ~P_N) | P_Z | P_C);
    champion_picture(0x4000, 0x18C76E, 0xC770, 0x0018);
    champion_picture(0x2000, 0x18998A, 0x998C, 0x0018);
    champion_picture(0x0000, 0x1C989A, 0x989C, 0x001C);
    cpu.p &= (uint8_t)~(P_M | P_X);
    cpu.a = 0xE87B;
    cpu.x = 0x0004;
    cpu.p = nz16(cpu.p, 0x0004);
    abi_GsuCF42To3000();
    cpu.p &= (uint8_t)~(P_M | P_X);
    a = (uint16_t)(rd16(0x000D4A) & 0x00FF);
    for (i = 0; i < 4; i++) {
        cpu.p = (uint8_t)((cpu.p & ~P_C) | (a >> 15));
        a = (uint16_t)(a << 1);
    }
    x = a;
    yo = 0x0003;
    for (;;) {
        cpu.a = rd16(0x0BED9F + x);
        wr16(0x0011E1, cpu.a);
        cpu.a = rd16(0x0BEDA1 + x);
        wr16(0x0011E3, cpu.a);
        sx = x;
        cpu.a = rd16(0x0BEDD7 + (uint16_t)(yo << 1));
        wr16(0x0011E5, cpu.a);
        for (y2 = 0x0004;; y2--) {
            x2 = rd16(0x0011E5);
            for (y3 = 0x000B;; y3--) {
                a = (uint16_t)(rd16(0x703000 + x2) & 0x03FF);
                a |= rd16(0x0011E3);
                wr16(0x703000 + x2, a);
                x2 = (uint16_t)(x2 + 2);
                if (!y3)
                    break;
            }
            a = rd16(0x0011E5);
            a = champion_add(a, 0x0040);
            wr16(0x0011E5, a);
            if (!y2)
                break;
        }
        x2 = rd16(0x0BEDCF + (uint16_t)(yo << 1));
        a = rd16(0x0011E1);
        a |= rd16(0x0011E3);
        for (i = 0; i < 4; i++) {
            uint16_t c = a;

            wr16(0x703000 + x2, c);
            c = champion_add(c, 0x0010);
            wr16(0x703040 + x2, c);
            c = champion_add(c, 0x0010);
            wr16(0x703080 + x2, c);
            a = (uint16_t)(a + 1);
            x2 = (uint16_t)(x2 + 2);
        }
        cpu.a = a;
        x = (uint16_t)(sx + 4);
        if (!yo)
            break;
        yo--;
    }
    cpu.x = x;
    cpu.y = 0xFFFF;
    cpu.p = nz16(cpu.p, 0xFFFF);
    a = (uint16_t)(rd16(0x0018CB) & 0x0003);
    cpu.p = (uint8_t)((cpu.p & ~P_C) | (a >> 15));
    a = (uint16_t)(a << 1);
    cpu.a = rd16(0x0BEDDF + a);
    wr16(0x0011DF, cpu.a);
    cpu.x = 0x0628;
    cpu.p = nz16(cpu.p, 0x0628);
    abi_PictureCellAttributes();
    cpu.x = 0x052E;
    cpu.p = nz16(cpu.p, 0x052E);
    abi_PictureCellAttributes();
    cpu.a = 0x6400;
    wr16(VMADDL, 0x6400);
    for (x = 0x0000; x != 0x1000; x = (uint16_t)(x + 2)) {
        cpu.a = rd16(0x703000 + x);
        wr16(VMDATAL, cpu.a);
    }
    cpu.x = x;
    cpu.p = (uint8_t)((cpu.p & ~P_N) | P_Z | P_C);
    cpu.a = 0xEBCB;
    cpu.x = 0x0004;
    cpu.p = nz16(cpu.p, 0x0004);
    abi_GsuCF42To3000();
    cpu.p &= (uint8_t)~(P_M | P_X);
    wr16(0x00162D, 0x0000);
    wr16(0x0016AF, 0x0000);
    cpu.a = 0x0006;
    wr16(0x000D7D, 0x0006);
    cpu.a = 0x0000;
    wr16(0x0018FD, 0x0000);
    wr16(0x00118B, 0x0000);
    wr16(0x000311, 0x0000);
    wr16(0x001903, 0x0000);
    wr16(0x0018B9, 0x0000);
    cpu.a = 0x0003;
    wr16(0x001905, 0x0003);
    cpu.a = 0x0000;
    wr16(0x001921, 0x0000);
    wr16(0x001925, 0x0000);
    cpu.a = 0x0066;
    wr16(0x0011EF, 0x0066);
    a = (uint16_t)(rd16(0x000D4A) & 0x00FF);
    for (i = 0; i < 5; i++) {
        cpu.p = (uint8_t)((cpu.p & ~P_C) | (a >> 15));
        a = (uint16_t)(a << 1);
    }
    cpu.a = a;
    wr16(0x001919, a);
    a = (uint16_t)(rd16(0x000DD6) & 0x00FF);
    cpu.p = (uint8_t)((nz16(cpu.p, (uint16_t)(a - 0x0008)) & ~P_C) | (a >= 0x0008 ? P_C : 0));
    cpu.a = a == 0x0008 ? 0x0000 : 0x0020;
    wr16(0x001927, cpu.a);
    cpu.a = 0x1234;
    wr16(0x001189, 0x1234);
    wr16(0x00118F, 0x0000);
    wr16(0x0011EB, 0x0000);
    cpu.a = 0x0000;
    cpu.p = nz16(cpu.p, 0x0000);
    wr16(0x0011ED, 0x0000);
    abi_MenuActorPoolInit();
    cpu.x = 0x0036;
    cpu.p = nz16(cpu.p, 0x0036);
    abi_MenuActorSpawnList();
    abi_MenuActorsRun();
    abi_MenuOamBuild();
    op_sep(P_M);
    wr8(HDMAEN, 0xF0);
    wr8(0x000D3F, 0x20);
    wr8(VTIMEL, 0xE0);
    wr8(VTIMEH, 0x00);
    wr8(0x0011D7, 0x0C);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x21);
    cpu.p = nz8(cpu.p, 0x21);
    wr8(NMITIMEN, 0x21);
    abi_MenuWaitFrame();
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x03);
    cpu.p = nz8(cpu.p, 0x03);
    wr8(TM, 0x03);
    cpu.db = db0;
    cpu.p = nz8(cpu.p, db0);
    op_plp(p0);
}

