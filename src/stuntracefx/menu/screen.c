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
   the last colour. */
static uint16_t cross_fade(uint8_t bank, uint16_t n)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t x = 0x0080, y = 0x003F;

    wr16(b | 0x11DF, rd16(0x0BCFE5 + (uint16_t)(n & 0xFFFE)));
    MenuPaletteScale(bank, &x, &y);
    wr16(b | 0x11DF, (uint16_t)(((uint16_t)(rd16(b | 0x11DF) - 1) ^ 0x001F) + 1));
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
int MenuCrossFadeStep(uint8_t bank, uint16_t rec, uint16_t *a)
{
    uint16_t n = (uint16_t)(rd16((uint16_t)(rec + 0x0A)) + 1);

    if (n == 0x0040) {
        *a = 0xCE89;
        wr16((uint16_t)(rec + 0x08), *a);
        return 0;
    }
    wr16((uint16_t)(rec + 0x0A), n);
    *a = cross_fade(bank, n);
    return 1;
}

void abi_MenuCrossFadeStep(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;

    if (MenuCrossFadeStep(cpu.db, cpu.x, &cpu.a)) {
        cpu.x = 0x0020;
        cpu.y = 0xFFFF;
        cpu.p = (uint8_t)(nz16(cpu.p, rd16(b | 0x11DF)) & ~P_C);
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
int MenuCrossFadeOut(uint8_t bank, uint16_t rec, uint16_t *a)
{
    static const struct { uint32_t reg; uint8_t v; } set[] = {
        {BG12NBA, 0x70}, {BG2SC, 0x7D}, {BGMODE, 0x03}, {TM, 0x02}, {HDMAEN, 0x00}};
    uint16_t n = (uint16_t)(rd16((uint16_t)(rec + 0x0A)) - 2);
    unsigned i;

    if (!(n & 0x8000)) {
        wr16((uint16_t)(rec + 0x0A), n);
        *a = cross_fade(bank, n);
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
    uint32_t b = (uint32_t)cpu.db << 16;

    if (MenuCrossFadeOut(cpu.db, cpu.x, &cpu.a)) {
        cpu.x = 0x0020;
        cpu.y = 0xFFFF;
        cpu.p = (uint8_t)(nz16(cpu.p, rd16(b | 0x11DF)) & ~P_C);
    } else {
        cpu.p = (uint8_t)((nz16(cpu.p, cpu.a) & ~(P_M | P_X)));
    }
}
