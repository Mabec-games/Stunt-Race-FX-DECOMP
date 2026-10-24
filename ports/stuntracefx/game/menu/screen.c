/*
 * screen.c: menu screen setup: scroll registers, the palette buffer and its
 * saved copy, sprite lists and pictures drawn by the Super FX.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "hw.h"
#include "carried.h"

void ApproachTargetFull(uint8_t bank);
void ApproachTargetZero(uint8_t bank);
#include "audio_calls.h"
#include "system_calls.h"
#include "menu_calls.h"
uint8_t SaveChecksum(void);

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

    cyc(44);
    for (i = 0; i < 6; i++) {
        cyc(24);
        reg_wr(reg[i], 0x00);
        cyc(24);
        reg_wr(reg[i], 0x00);
    }
    cyc(70);
}

void ChampionScreenOpen(Carried *cr);
void ChampionScreenRun(Carried *cr);
void MenuColourMathSetup(Carried *cr);
void MenuCourseGraphics_cr(Carried *cr);
void MenuCrossFadeOut_cr(Carried *cr);
void MenuCrossFadeStep_cr(Carried *cr);
void MenuHdmaSetup_cr(Carried *cr);
void MenuHdmaSetupB_cr(Carried *cr);
void MenuOamFromList_cr(Carried *cr);
void MenuPaletteLoad_cr(Carried *cr);
void MenuPaletteSave_cr(Carried *cr);
void MenuPaletteScale_cr(Carried *cr);
void MenuPictureDC8CGrid0_cr(Carried *cr);
void MenuPictureDC8CGrid58_cr(Carried *cr);
void MenuPictureInsetGrid_cr(Carried *cr);
void MenuPictureToVram_cr(Carried *cr);
void MenuScrollReset_cr(Carried *cr);
void MenuScrollZeroBg12(Carried *cr);
void MenuSpritePalettesScale_cr(Carried *cr);
void MenuVramClear(Carried *cr);
void MenuWindowSetup(Carried *cr);
void PictureCellAttributes(Carried *cr);
void TitleScreenLoop(Carried *cr);
void TitleScreenRun(Carried *cr);
void MenuScreenOpen6(Carried *cr);
static uint16_t champion_add(Carried *cr, uint16_t a, uint16_t m);
static void champion_picture(Carried *cr, uint16_t vram, uint32_t size, uint16_t in68, uint16_t in6a);
static void champion_sound(Carried *cr, void (*load)(Carried *), uint16_t a, uint16_t y, int delay_after);
static void menu_screen_start(Carried *cr, uint16_t list, uint8_t p);
static void w_ApproachTargetFull(Carried *cr);
static void w_ApproachTargetZero(Carried *cr);
static void w_ApuUpload(Carried *cr);
static void w_MenuActorsRun(Carried *cr);
static void w_MenuScreenOpen6(Carried *cr);
static void w_SaveChecksum(Carried *cr);
void DemoRunFirst(Carried *cr);
void DemoRunFourth(Carried *cr);
void DemoRunSecond(Carried *cr);
void GsuCF42To3000_cr(Carried *cr);
void MenuActorFind_cr(Carried *cr);
void MenuActorPoolInit_cr(Carried *cr);
void MenuActorSpawnList_cr(Carried *cr);
void MenuOamBuild_cr(Carried *cr);
void MenuRunFrames_cr(Carried *cr);
void MenuWaitFrame_cr(Carried *cr);
void SoundDataLoad_cr(Carried *cr);
void SoundReset_cr(Carried *cr);
void WindowHdmaChannelsSetup_cr(Carried *cr);

void MenuScrollReset_cr(Carried *cr)
{
    MenuScrollReset();
    cr->x &= 0x00FF;
    cr->y &= 0x00FF;
}

void abi_MenuScrollReset(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuScrollReset_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
    cyc(108);
    uint32_t src = rd24(0x0000FB);
    uint16_t c = 0;
    unsigned i;

    for (i = 0; i < 0x200; i += 2) {
        cyc(48);
        c = rd16((src + i) & 0xFFFFFF);
        wr16(0x7EFD65 + i, c);
        cyc(142);
    }
    cyc(24);
    return c;
}

void MenuPaletteLoad_cr(Carried *cr)
{
    cr->a = MenuPaletteLoad();
    cr->x = cr->y = 0x0200;
}

void abi_MenuPaletteLoad(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuPaletteLoad_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
        cyc(100);
        c = rd16(0x7EFD65 + i);
        cyc(32);
        wr16(0x7EFF65 + i, c);
        cyc(6);
    }
    cyc(132);
    return c;
}

void MenuPaletteSave_cr(Carried *cr)
{
    cr->a = MenuPaletteSave();
    cr->x = 0x0200;
}

void abi_MenuPaletteSave(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuPaletteSave_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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

    while ((w = rd16(cyc_then(138, b | (uint16_t)(0xFCA2 + *y)))) != 0xFFFF) {
        wr16(b | (uint16_t)(0x0375 + *x), w);
        *x = (uint16_t)(*x + 2);
        *y = (uint16_t)(*y + 2);
        cyc(56);
    }
    cyc(18);
}

void MenuOamFromList_cr(Carried *cr)
{
    MenuOamFromList(cr->db, &cr->x, &cr->y);
    cr->a = 0xFFFF;
    set_p(cr, (uint8_t)((cr->p & ~(P_N | P_M | P_X)) | P_Z | P_C));
}

void abi_MenuOamFromList(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuOamFromList_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
    cyc(106);
    uint16_t w;

    GsuCF42To3000(in68, in6a);
    cyc(102);
    ppu_wr(VMAIN, 0x80);
    *x = 0;
    cyc(24);
    do {
        cyc(54);
        w = rd16(0x703000 + *x);
        cyc(24);
        ppu_wr16(VMDATAL, w);
        *x = (uint16_t)(*x + 2);
    } while (*x != rd16(cyc_then(52, (uint32_t)bank << 16 | 0x11D5)));
    cyc(86);
    return w;
}

void MenuPictureToVram_cr(Carried *cr)
{
    cr->a = MenuPictureToVram(cr->db, cr->a, cr->x, &cr->x);
}

void abi_MenuPictureToVram(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuPictureToVram_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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

    cyc(112);
    for (i = 0; i < 4; i++) {
        cyc(40);
        dma_wr(b | ch[i].base, ch[i].mode);
        cyc(40);
        dma_wr(b | (uint16_t)(ch[i].base + 1), ch[i].reg);
        cyc(48);
        dma_wr16(b | (uint16_t)(ch[i].base + 2), ch[i].table);
        cyc(40);
        dma_wr(b | (uint16_t)(ch[i].base + 4), 0x7F);
    }
    cyc(70);
}

void MenuHdmaSetup_cr(Carried *cr)
{
    MenuHdmaSetup(cr->db);
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0x7F);
    cr->x = 0x131F;
}

void abi_MenuHdmaSetup(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuHdmaSetup_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:F664 MenuVramClear
 * WHAT IT DOES: Clears all of VRAM: the word address set to 0, then $8000
 * zero words written. Nothing calls it, so a direct call is its only way
 * in.
 * HOW: REP #$30; X counts down by 2 from 0 to 0; 16-bit stores to VMADDL
 * and VMDATAL through the data bank.
 * Entry: any widths; data bank with the PPU registers. Exit: X = Y = 0; A
 * unchanged; M and X 16-bit; Z set, N clear.
 */
void MenuVramClear(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;
    uint16_t x = 0;

    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~(P_M | P_X))));
    cyc(94);
    ppu_wr16(b | 0x2116, 0x0000);
    cyc(24);
    do {
        ppu_wr16(b | 0x2118, 0x0000);
        x = (uint16_t)(x - 2);
    } while (cyc_loop(74, x));
    cyc(86);
    cr->x = 0;
    cr->y = 0;
    set_p(cr, nz16(cr->p, 0));
}

void abi_MenuVramClear(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuVramClear(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:F677 MenuWindowSetup
 * WHAT IT DOES: Sets the windows: W12SEL = $22, W34SEL = 0, WOBJSEL = $30,
 * window 1 from $20 to $88, window masking on BG1 on both screens (TMW =
 * TSW = 1) and BG1 on the sub screen (TS = 1). Nothing calls it, so a
 * direct call is its only way in.
 * HOW: 8-bit stores through the data bank between PHP and PLP.
 * Entry: any widths; data bank with the PPU registers. Exit: A low byte =
 * 1; P unchanged.
 */
void MenuWindowSetup(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;

    cyc(84);
    ppu_wr(b | 0x2123, 0x22);
    cyc(40);
    ppu_wr(b | 0x2124, 0x00);
    cyc(40);
    ppu_wr(b | 0x2125, 0x30);
    cyc(40);
    ppu_wr(b | 0x2126, 0x20);
    cyc(40);
    ppu_wr(b | 0x2127, 0x88);
    cyc(40);
    ppu_wr(b | 0x212E, 0x01);
    cyc(24);
    ppu_wr(b | 0x212F, 0x01);
    cyc(24);
    ppu_wr(b | 0x212D, 0x01);
    cyc(70);
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0x01);
}

void abi_MenuWindowSetup(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuWindowSetup(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:F6A0 MenuColourMathSetup
 * WHAT IT DOES: Sets colour maths to add the sub screen on BG1 and the
 * backdrop at half (CGADSUB = $41, CGWSEL = 2) and empties the sub screen
 * (TS = 0). Nothing calls it, so a direct call is its only way in.
 * HOW: Long stores to CGADSUB and CGWSEL, then a store to TS through the
 * data bank; between PHP and PLP.
 * Entry: any widths. Exit: A low byte = 0; P unchanged.
 */
void MenuColourMathSetup(Carried *cr)
{
    cyc(92);
    ppu_wr(CGADSUB, 0x41);
    cyc(48);
    ppu_wr(CGWSEL, 0x02);
    cyc(40);
    ppu_wr((uint32_t)cr->db << 16 | 0x212D, 0x00);
    cyc(70);
    cr->a = (uint16_t)(cr->a & 0xFF00);
}

void abi_MenuColourMathSetup(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuColourMathSetup(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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

    cyc(66);
    for (i = 0; i < 4; i++) {
        cyc(40);
        dma_wr(b | ch[i].base, ch[i].mode);
        cyc(40);
        dma_wr(b | (uint16_t)(ch[i].base + 1), ch[i].reg);
        cyc(48);
        dma_wr16(b | (uint16_t)(ch[i].base + 2), ch[i].table);
        cyc(40);
        dma_wr(b | (uint16_t)(ch[i].base + 4), 0x7F);
        if (i == 0)
            { cyc(40); dma_wr(b | 0x4377, 0x00); }
    }
    cyc(48);
    wr8(0x7F1354, 0xFF);
    cyc(78);
    wr16(0x0018BB, 0x0000);
    cyc(32);
    wr16(0x0018BF, 0x0000);
    cyc(56);
    wr16(0x0018BD, 0x00A8);
    cyc(32);
    wr16(0x0018C1, 0x00A8);
    cyc(70);
}

void MenuHdmaSetupB_cr(Carried *cr)
{
    MenuHdmaSetupB(cr->db);
    cr->a = 0x00A8;
    cr->x = 0x1353;
}

void abi_MenuHdmaSetupB(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuHdmaSetupB_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
    uint8_t f;

    cyc(70);
    io_wr8(WRMPYA, v);
    cyc(24);
    io_wr8(WRMPYB, f = cyc_then(24, rd8(b | 0x11DF)));
    cyc(102);
    return io_rd16(RDMPYL, cpu_mul(v, f));
}

uint16_t MenuPaletteScale(uint8_t bank, uint16_t *x, uint16_t *y)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t c, r = 0, prod;
    int first = 1;

    do {
        cyc(first ? 76 : 120);          /* PHP, REP, LDA long, or the loop's end and LDA long */
        first = 0;
        c = rd16(0x7EFF65 + *x);
        cyc(24);
        wr16(b | 0x11E3, c);
        prod = scale_component(b, (uint8_t)(c & 0x1F));
        cyc(110);
        wr16(b | 0x11E1, (uint16_t)(((prod << 3) >> 8) & 0x1F));
        cyc(24);
        c = rd16(b | 0x11E3);
        cyc(62);
        prod = scale_component(b, (uint8_t)((c & 0x03E0) >> 5));
        cyc(48);
        wr16(b | 0x11E1, cyc_then(24, (uint16_t)((prod & 0x03E0) | rd16(b | 0x11E1))));
        cyc(24);
        c = rd16(b | 0x11E3);
        cyc(72);
        prod = scale_component(b, (uint8_t)((c & 0x7C00) >> 10));
        cyc(110);
        r = (uint16_t)((((prod & 0x03E0) >> 3) << 8) | rd16(b | 0x11E1));
        cyc(24);
        wr16(b | 0x11E1, r);
        cyc(32);
        wr16(0x7EFD65 + *x, r);
        *x = (uint16_t)(*x + 2);
        *y = (uint16_t)(*y - 1);
    } while (!(*y & 0x8000));
    cyc(136);
    return r;
}

void MenuPaletteScale_cr(Carried *cr)
{
    cr->a = MenuPaletteScale(cr->db, &cr->x, &cr->y);
}

void abi_MenuPaletteScale(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuPaletteScale_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* A picture word with its tile number raised by $100: kept in the picture
   and copied to the second picture at $70:3800. */
static uint16_t raise_tile(uint16_t x)
{
    cyc(54);
    uint16_t w = (uint16_t)(rd16(0x703000 + x) + 0x0100);

    cyc(70);
    wr16(0x703000 + x, w);
    cyc(32);
    wr16(0x703800 + x, w);
    cyc(24);
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

    cyc(52);
    for (i = 0; i < 0x180; i++, x = (uint16_t)(x + 2))
        { cyc(18); a = raise_tile(x); }
    cyc(82);
    wr16(b | 0x11DF, 0x000A);
    do {
        for (i = 0; cyc_loop(2, i < 4); i++, x = (uint16_t)(x + 2))
            { raise_tile(x); cyc(16); }
        cyc(42);
        a = (uint16_t)(rd16(b | 0x11E1) | 0x2000);
        for (i = 0; i < 13; i++, x = (uint16_t)(x + 2)) {
            cyc(110);
            wr16(0x703800 + x, cyc_then(70, (uint16_t)(rd16(0x703000 + x) + 0x0100)));
            cyc(68);
            wr16(0x703000 + x, a);
            a = (uint16_t)(a + 0x0B);
            cyc(54);
        }
        cyc(26);
        for (i = 0; i < 15; i++, x = (uint16_t)(x + 2))
            { cyc(18); a = raise_tile(x); }
        cyc(58);
        wr16(b | 0x11E1, cyc_then(14, (uint16_t)(rd16(b | 0x11E1) + 1)));
        cyc(24);
        rows = (uint16_t)(rd16(b | 0x11DF) - 1);
        cyc(14);
        wr16(b | 0x11DF, rows);
    } while (cyc_loop(22, !(rows & 0x8000)));
    for (i = 0; cyc_loop(18, i < 0xA0); i++, x = (uint16_t)(x + 2))
        a = w = raise_tile(x);
    cyc(104);
    return a;
}

void MenuPictureInsetGrid_cr(Carried *cr)
{
    cr->a = MenuPictureInsetGrid(cr->db);
    cr->x = 0x0700;
    cr->y = 0xFFFF;
}

void abi_MenuPictureInsetGrid(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuPictureInsetGrid_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* Draw a picture with GsuCF42To3000 and write `bytes` of it to VRAM word
   address `vram` (VMAIN as the caller left it). */
static uint16_t picture_to_vram(uint8_t bank, uint16_t in68, uint16_t in6a, uint16_t vram, uint16_t bytes, int draw)
{
    uint16_t x, w = 0;

    (void)bank;
    if (draw)
        { cyc(150); GsuCF42To3000(in68, in6a); }
    cyc(88);
    ppu_wr16(VMADDL, vram);
    for (x = 0; x != bytes; x = (uint16_t)(x + 2)) {
        cyc(56);
        w = rd16(0x703000 + x);
        cyc(24);
        ppu_wr16(VMDATAL, w);
        cyc(50);
    }
    return w;
}

/* The two words of a course's picture entry: input $68 and input $6A. */
static void course_picture(uint8_t bank, uint32_t table, uint16_t *in68, uint16_t *in6a)
{
    cyc(64);
    uint16_t c = rd16((uint32_t)bank << 16 | 0x0E3D), i = (uint16_t)(((uint16_t)(c << 1) + c) & 0x00FF);

    cyc(168);
    *in68 = rd16(table + i);
    cyc(50);
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
    cyc(4);
    uint16_t a, x;

    course_picture(bank, 0x03F4F9, &a, &x);
    picture_to_vram(bank, a, x, 0x4C00, 0x1800, 1);
    course_picture(bank, 0x03F5A1, &a, &x);
    picture_to_vram(bank, a, x, 0x3C00, 0x2000, 1);
    picture_to_vram(bank, 0xFB84, 0x0017, 0x1680, 0x0600, 1);
    picture_to_vram(bank, 0, 0, 0x3680, 0x0600, 0);
    return picture_to_vram(bank, 0xFFAC, 0x0017, 0x1C00, 0x0800, 1);
}

void MenuCourseGraphics_cr(Carried *cr)
{
    cr->a = MenuCourseGraphics(cr->db);
    cr->x = 0x0800;
    cyc(112);
}

void abi_MenuCourseGraphics(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuCourseGraphics_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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

    cyc(24);
    wr16(b | 0x11DF, factor);
    cyc(140);
    MenuPaletteScale(bank, &x, &y);
    cyc(60);
    wr16(b | 0x11DF, factor);
    x = 0x0120;
    y = 0x0007;
    cyc(110);
    return MenuPaletteScale(bank, &x, &y);
}

void MenuSpritePalettesScale_cr(Carried *cr)
{
    cr->a = MenuSpritePalettesScale(cr->db, cr->a);
    cr->x = rd16((uint32_t)cr->db << 16 | 0x11F9);
    cr->y = 0xFFFF;
    set_p(cr, nz16(cr->p, cr->x));
}

void abi_MenuSpritePalettesScale(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuSpritePalettesScale_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* One cross-fade frame for count n: colours 64-127 scaled by f / 32 and
   colours 12-15 by (32 - f) / 32, f = $0B:CFE5[n with bit 0 clear], the
   second factor made as ((f - 1) EOR $1F) + 1; both through $11DF. Returns
   the last colour; *factor = the second factor. */
static uint16_t cross_fade(uint8_t bank, uint16_t n, uint16_t *factor)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t x = 0x0080, y = 0x003F;

    cyc(114);
    wr16(b | 0x11DF, rd16(0x0BCFE5 + (uint16_t)(n & 0xFFFE)));
    cyc(110);
    MenuPaletteScale(bank, &x, &y);
    cyc(24);
    *factor = (uint16_t)(((uint16_t)(rd16(b | 0x11DF) - 1) ^ 0x001F) + 1);
    cyc(76);
    wr16(b | 0x11DF, *factor);
    x = 0x0018;
    y = 0x0003;
    cyc(110);
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
    cyc(22);
    uint16_t n = (uint16_t)(rd16((uint16_t)(rec + 0x0A)) + 1);

    if (n == 0x0040) {
        *a = 0xCE89;
        cyc(106);
        wr16((uint16_t)(rec + 0x08), *a);
        return 0;
    }
    cyc(76);
    wr16((uint16_t)(rec + 0x0A), n);
    *a = cross_fade(bank, n, factor);
    return 1;
}

void MenuCrossFadeStep_cr(Carried *cr)
{
    uint16_t f;

    if (MenuCrossFadeStep(cr->db, cr->x, &cr->a, &f)) {
        cr->x = 0x0020;
        cr->y = 0xFFFF;
        set_p(cr, (uint8_t)(nz16(cr->p, f) & ~P_C));
    } else {
        set_p(cr, (uint8_t)((nz16(cr->p, cr->a)) | P_C));
    }
    cyc(42);
}

void abi_MenuCrossFadeStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuCrossFadeStep_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* Draws picture $DC8C (input $1C) into $70:3000 with the Super FX, then
   lays the tile grid from tile `first` over it (MenuPictureInsetGrid). */
static uint16_t picture_dc8c_grid(uint8_t bank, uint16_t first)
{
    cyc(154);
    GsuCF42To3000(0xDC8C, 0x001C);
    cyc(88);
    wr16((uint32_t)bank << 16 | 0x11E1, first);
    cyc(46);
    return cyc_then(70, MenuPictureInsetGrid(bank));
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

void MenuPictureDC8CGrid0_cr(Carried *cr)
{
    cr->a = MenuPictureDC8CGrid0(cr->db);
    cr->x = 0x0700;
    cr->y = 0xFFFF;
}

void abi_MenuPictureDC8CGrid0(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuPictureDC8CGrid0_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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

void MenuPictureDC8CGrid58_cr(Carried *cr)
{
    cr->a = MenuPictureDC8CGrid58(cr->db);
    cr->x = 0x0700;
    cr->y = 0xFFFF;
}

void abi_MenuPictureDC8CGrid58(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuPictureDC8CGrid58_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

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
    cyc(22);
    uint16_t n = (uint16_t)(rd16((uint16_t)(rec + 0x0A)) - 2);
    unsigned i;

    if (!(n & 0x8000)) {
        cyc(66);
        wr16((uint16_t)(rec + 0x0A), n);
        *a = cross_fade(bank, n, factor);
        return 1;
    }
    cyc(150);
    wr16(0x0016B1, 0x0080);
    cyc(56);
    wr16(0x0016B5, 0x000B);
    cyc(56);
    wr16(0x0016B3, 0xD56C);
    cyc(56);
    wr16(0x0016B7, 0x7000);
    (cyc(62), VramDmaQueueAdd());
    cyc(8);
    for (i = 0; i < sizeof set / sizeof set[0]; i++)
        { cyc(212); FrameByteWritePush(bank, (uint16_t)set[i].reg, set[i].v); cyc(108); }
    *a = 0xD1F7;
    cyc(104);
    wr16((uint16_t)(rec + 0x08), *a);
    return 0;
}

void MenuCrossFadeOut_cr(Carried *cr)
{
    uint16_t f;

    if (MenuCrossFadeOut(cr->db, cr->x, &cr->a, &f)) {
        cr->x = 0x0020;
        cr->y = 0xFFFF;
        set_p(cr, (uint8_t)(nz16(cr->p, f) & ~P_C));
    } else {
        set_p(cr, (uint8_t)((nz16(cr->p, cr->a) & ~(P_M | P_X))));
    }
    cyc(42);
}

void abi_MenuCrossFadeOut(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuCrossFadeOut_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void SoundPort0QueuePush(uint8_t command);
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
static void menu_screen_start(Carried *cr, uint16_t list, uint8_t p)
{
    cyc(62);
    uint16_t a = 0x2802, y = 0x002C;  /* as MenuActorPoolInit leaves A and Y */

    MenuActorPoolInit(0x00);
    cyc(86);
    cr->x = MenuActorSpawnList(0x00, list, &a, &y);
    cr->a = a;
    cr->y = y;
    set_p(cr, (uint8_t)((p & ~(P_M | P_X)) | P_C));
    (cyc(62), w_MenuActorsRun(cr));
    cyc(62);
    cr->a = MenuOamBuild(&cr->x, &cr->y);
    cyc(62);
    wr8(0x000D3F, 0x1E);
    cyc(40);
    hw_wr(VTIMEL, 0xE0);
    cyc(24);
    hw_wr(VTIMEH, 0x00);
    cyc(40);
    wr8(0x0011D7, 0x0C);
    cyc(40);
    hw_wr(NMITIMEN, 0x21);
    cyc(46);
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
void MenuScreenOpen0(Carried *cr)
{
    /* cost: LDA # and STA (40) or STZ (24). */
    static const struct { uint32_t reg; uint8_t v; uint8_t cost; } ppu[] = {
        {BGMODE, 0x09, 40}, {BG1SC, 0x60, 40}, {BG2SC, 0x65, 40}, {BG3SC, 0x7C, 40},
        {BG12NBA, 0x20, 40}, {BG34NBA, 0x77, 40}, {CGADSUB, 0x00, 24}, {CGWSEL, 0x00, 24}, {TS, 0x00, 24}};
    uint8_t p = cr->p, db = cr->db;
    uint16_t x;
    unsigned i;

    cr->db = 0x00;
    cyc(306);
    ppu_wr(INIDISP, 0x80);
    cyc(24);
    hdmaen_wr(HDMAEN, 0x00);
    cyc(24);
    mdmaen_wr(MDMAEN, 0x00);
    cyc(24);
    wr8(0x0011DA, 0x00);
    cyc(24);
    hw_wr(NMITIMEN, 0x00);
    for (i = 0; i < sizeof ppu / sizeof ppu[0]; i++)
        { cyc(ppu[i].cost); reg_wr(ppu[i].reg, ppu[i].v); }
    cyc(62);
    SoundReset();
    (cyc(46), MenuScrollReset());
    cyc(62);
    wr16(0x0000FB, 0x989C);
    cyc(40);
    wr16(0x0000FD, 0x001C);
    (cyc(90), MenuPaletteLoad());
    cyc(78);
    wr16(0x7EFD65, 0x0000);
    cyc(48);
    ppu_wr16(VMADDL, 0x4000);
    for (x = 0; x != 0x0720; x = (uint16_t)(x + 2))
        { cyc(96); ppu_wr16(VMDATAL, rd16(0x1CDC8C + x)); cyc(50); }
    cyc(64);
    wr16(0x00162D, 0x0000);
    cyc(24);
    wr16(0x0016AF, 0x0000);
    cyc(48);
    wr16(0x000D7D, 0x0006);
    cyc(56);
    wr16(0x0018FD, 0x0000);
    cyc(56);
    wr16(0x001903, 0x0001);
    cyc(56);
    wr16(0x0018B9, 0x0000);
    cyc(48);
    wr16(0x001189, 0x1234);
    cyc(24);
    wr16(0x00118F, 0x0000);
    cyc(24);
    wr16(0x0011EB, 0x0000);
    cyc(48);
    wr16(0x0011ED, 0x0000);
    cyc(24);
    wr16(0x0011EF, 0x0000);
    menu_screen_start(cr, 0x0000, p);
    cyc(100);
    SoundPort0QueuePush(0x39);
    cyc(46);
    MenuWaitFrame(0x00);
    cyc(46);
    MenuWaitFrame(0x00);
    cyc(62);
    wr8(0x0011DA, 0x0F);
    cyc(24);
    wr8(0x0011DB, 0x0F);
    cyc(40);
    ppu_wr(TM, 0x10);
    cr->a = 0x0010;
    cr->db = db;
    set_p(cr, p);
}

void abi_MenuScreenOpen0(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuScreenOpen0(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}


/* A sound block upload as the menu does it: port 1 (and 2) = 5, a delay,
   the upload, a delay. The delays are counted loops: time, no state. */
static void menu_apu_upload(uint8_t bank, uint16_t addr)
{
    cyc(38);
    hw_wr16(APUIO1, 0x0005);
    cyc(134);
    delay_turns(0x4000);
    ApuUpload(bank, addr);
    cyc(136);
    delay_turns(0x4000);
}

/* Picture `in68` (Super FX input $1C) to VRAM `vram`; its byte count is
   the big-endian word before it in bank $1C. */
static void menu_picture(uint16_t vram, uint16_t in68)
{
    cyc(70);
    uint16_t x, n = rd16(0x1C0000 | (uint16_t)(in68 - 2));

    ppu_wr16(VMADDL, vram);
    cyc(114);
    wr16(0x0011D5, (uint16_t)((n << 8) | (n >> 8)));
    cyc(138);
    MenuPictureToVram(0x00, in68, 0x001C, &x);
    cyc(44);
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
void MenuScreenOpen6(Carried *cr)
{
    /* cost: LDA # and STA (40) or STZ (24). */
    static const struct { uint32_t reg; uint8_t v; uint8_t cost; } ppu[] = {
        {BGMODE, 0x09, 40}, {BG1SC, 0x60, 40}, {BG2SC, 0x65, 40}, {BG3SC, 0x7C, 40},
        {BG12NBA, 0x20, 40}, {BG34NBA, 0x77, 40}, {CGADSUB, 0x01, 40}, {CGWSEL, 0x02, 40}, {TS, 0x02, 40},
        {COLDATA, 0x00, 24}, {TSW, 0x00, 24}, {TMW, 0x00, 24}};
    uint8_t p = cr->p, db = cr->db;
    uint16_t x, y;
    unsigned i;

    cr->db = 0x00;
    cyc(318);
    ppu_wr(INIDISP, 0x80);
    cyc(24);
    hdmaen_wr(HDMAEN, 0x00);
    cyc(24);
    mdmaen_wr(MDMAEN, 0x00);
    cyc(24);
    wr8(0x0011DA, 0x00);
    cyc(24);
    hw_wr(NMITIMEN, 0x00);
    for (i = 0; i < sizeof ppu / sizeof ppu[0]; i++)
        { cyc(ppu[i].cost); reg_wr(ppu[i].reg, ppu[i].v); }
    cyc(100);
    hw_wr16(APUIO1, 0x0005);
    cyc(134);
    delay_turns(0x4000);
    SoundDataLoad(0xF3B5, 0x001E);
    cyc(90);
    menu_apu_upload(0x1B, 0x85F8);
    menu_apu_upload(0x19, 0x9A98);
    SoundReset();
    cyc(62);
    hw_wr(APUIO0, 0x01);
    (cyc(46), MenuScrollReset());
    cyc(84);
    wr16(0x0000FB, 0x989C);
    cyc(40);
    wr16(0x0000FD, 0x001C);
    (cyc(90), MenuPaletteLoad());
    (cyc(46), MenuPaletteSave());
    cyc(48);
    wr16(0x0011DF, 0x0001);
    x = 0x0080;
    y = 0x003F;
    cyc(110);
    MenuPaletteScale(0x00, &x, &y);
    cyc(48);
    wr16(0x0011DF, 0x0020);
    x = 0x0018;
    y = 0x0003;
    cyc(110);
    MenuPaletteScale(0x00, &x, &y);
    MenuHdmaSetup(0x00);
    menu_picture(0x4000, 0xDC38);
    menu_picture(0x6800, 0xC9DC);
    cyc(70);
    ppu_wr16(VMADDL, 0x7000);
    for (x = 0; x != 0x0040; x = (uint16_t)(x + 2))
        { cyc(96); ppu_wr16(VMDATAL, rd16(0x1CBE58 + x)); cyc(50); }
    cyc(18);
    menu_picture(0x0000, 0xBABC);
    menu_picture(0x6000, 0xBE58);
    menu_picture(0x2000, 0xC664);
    menu_picture(0x6400, 0xC818);
    menu_picture(0x7C00, 0xBECC);
    cyc(46);
    wr16(0x00162D, 0x0000);
    cyc(24);
    wr16(0x0016AF, 0x0000);
    cyc(48);
    wr16(0x000D7D, 0x0006);
    cyc(56);
    wr16(0x0018FD, 0x0000);
    cyc(24);
    wr16(0x00118B, 0x0000);
    cyc(24);
    wr16(0x000311, 0x0000);
    cyc(56);
    wr16(0x001903, 0x0000);
    cyc(32);
    wr16(0x0018B9, 0x0000);
    cyc(48);
    wr16(0x001189, 0x1234);
    cyc(24);
    wr16(0x00118F, 0x0000);
    cyc(24);
    wr16(0x0011EB, 0x0000);
    cyc(48);
    wr16(0x0011ED, 0x0000);
    cyc(24);
    wr16(0x0011EF, 0x0000);
    menu_screen_start(cr, 0x0006, p);
    cyc(40);
    ppu_wr(TM, 0x17);
    cr->a = 0x0017;
    cr->db = db;
    set_p(cr, p);
    cyc(98);
}

void abi_MenuScreenOpen6(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuScreenOpen6(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:EDE7 PictureCellAttributes
 * WHAT IT DOES: Gives the 2 x 2 cell at X in the Super FX picture map
 * ($70:3000 + X, + 2, + $40, + $42) the attribute bits $11DF over each
 * tile number (its low 9 bits kept).
 * HOW: AND #$01FF, ORA $11DF per word; P saved and restored.
 * Entry: X 16-bit; X = the cell. Exit: P restored; A = the last word.
 */
void PictureCellAttributes(Carried *cr)
{
    static const uint16_t at[4] = {0x0000, 0x0002, 0x0040, 0x0042};
    uint32_t b;
    uint8_t p0 = cr->p;
    int i;

    r_rep(cr, P_M | P_X);
    b = (uint32_t)cr->db << 16;
    cyc(92);
    for (i = 0; i < 4; i++) {
        uint32_t a = 0x703000 + (uint32_t)(uint16_t)(cr->x + at[i]);

        cyc(32);
        r_lda16(cr, a);
        r_and16(cr, 0x01FF);
        cyc(46);
        cr->a |= rd16(b | 0x11DF);
        cyc(32);
        wr16(a, cr->a);
    }
    cyc(28);
    r_plp(cr, p0);
}

void abi_PictureCellAttributes(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PictureCellAttributes(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
static void champion_sound(Carried *cr, void (*load)(Carried *), uint16_t a, uint16_t y, int delay_after)
{
    uint8_t p = cr->p;

    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~(P_M | P_X))));
    cr->a = 0x0005;
    cyc(86);
    hw_wr16(APUIO1, 0x0005);
    cyc(134);
    delay_turns(0x4000);
    cr->a = 0x0000;
    set_p(cr, (uint8_t)((cr->p & ~P_N) | P_Z));
    cr->a = a;
    cr->y = y;
    set_p(cr, nz16(cr->p, y));
    load(cr);
    if (delay_after) {
        set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
        cyc(46);
        delay_turns(0x4000);
        cr->a = 0x0000;
        set_p(cr, (uint8_t)((cr->p & ~P_N) | P_Z));
    }
    r_plp(cr, p);
}

/* LDX #vram, STX VMADDL, LDA.L size (big-endian), XBA, STA $11D5, then
   MenuPictureToVram with A and X. */
static void champion_picture(Carried *cr, uint16_t vram, uint32_t size, uint16_t in68, uint16_t in6a)
{
    cr->x = vram;
    set_p(cr, nz16(cr->p, vram));
    cyc(88);
    ppu_wr16(VMADDL, vram);
    cyc(114);
    cr->a = rd16(size);
    r_xba(cr);
    wr16(0x0011D5, cr->a);
    cr->a = in68;
    cr->x = in6a;
    set_p(cr, nz16(cr->p, in6a));
    (cyc(138), MenuPictureToVram_cr(cr));
    cyc(26);
}

/* CLC, ADC #m (16-bit). */
static uint16_t champion_add(Carried *cr, uint16_t a, uint16_t m)
{
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_C)));
    return alu_adc16(a, m, &cr->p);
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
void ChampionScreenOpen(Carried *cr)
{
    static const struct { uint32_t reg; uint8_t v; } ppu[] = {
        {BGMODE, 0x03}, {BG1SC, 0x60}, {BG2SC, 0x65}, {BG12NBA, 0x00}};
    static const uint32_t off[] = {CGADSUB, CGWSEL, TS, COLDATA, TSW};
    uint8_t p0 = cr->p, db0 = cr->db, v, bit;
    uint16_t x, a, yo, y2, y3, x2, sx;
    unsigned i;

    r_sep(cr, P_M);
    r_rep(cr, P_X);
    cr->a = (uint16_t)(cr->a & 0xFF00);
    cr->db = 0x00;
    set_p(cr, nz8(cr->p, 0x00));
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0x80);
    cyc(324);
    ppu_wr(INIDISP, 0x80);
    cyc(24);
    hdmaen_wr(HDMAEN, 0x00);
    cyc(24);
    mdmaen_wr(MDMAEN, 0x00);
    cyc(24);
    wr8(0x0011DA, 0x00);
    cyc(24);
    hw_wr(NMITIMEN, 0x00);
    for (i = 0; i < sizeof ppu / sizeof ppu[0]; i++)
        { cyc(40); reg_wr(ppu[i].reg, ppu[i].v); }
    cyc(16);
    for (i = 0; i < sizeof off / sizeof off[0]; i++)
        { cyc(24); wr8(off[i], 0x00); }
    cyc(40);
    ppu_wr(WOBJSEL, 0x03);
    cyc(40);
    ppu_wr(WH0, 0x00);
    cyc(40);
    ppu_wr(WH1, 0x00);
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0x10);
    set_p(cr, nz8(cr->p, 0x10));
    cyc(40);
    ppu_wr(TMW, 0x10);
    cyc(14);
    champion_sound(cr, SoundDataLoad_cr, 0xF884, 0x001D, 0);
    champion_sound(cr, w_ApuUpload, 0x001B, 0x85F8, 1);
    champion_sound(cr, w_ApuUpload, 0x0019, 0x9A98, 1);
    (cyc(48), SoundReset_cr(cr));
    r_sep(cr, P_M);
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0x01);
    set_p(cr, nz8(cr->p, 0x01));
    cyc(62);
    hw_wr(APUIO0, 0x01);
    (cyc(46), MenuScrollReset_cr(cr));
    r_sep(cr, P_M);
    cyc(54);
    r_lda8(cr, 0x000DD6);
    v = (uint8_t)cr->a;
    set_p(cr, (uint8_t)((nz8(cr->p, (uint8_t)(v - 0x08)) & ~P_C) | (v >= 0x08 ? P_C : (cyc(100), 0))));
    if (v == 0x08) {
        cyc(64);
        r_lda8(cr, 0x000D4A);
        v = (uint8_t)cr->a;
        bit = v == 0x00 ? 0x01 : (cyc(38), v == 0x01 ? 0x02 : (cyc(38), v == 0x02 ? 0x04 : 0x00));
        set_p(cr, (uint8_t)((nz8(cr->p, (uint8_t)(v - (v < 0x02 ? v : 0x02))) & ~P_C) | P_C));
        if (v > 0x02)
            set_p(cr, (uint8_t)((nz8(cr->p, (uint8_t)(v - 0x02)) & ~P_C) | P_C));
        if (bit) {
            cyc(64);
            v = (uint8_t)(rd8(0x70FC0B) | bit);
            cr->a = (uint16_t)((cr->a & 0xFF00) | v);
            set_p(cr, nz8(cr->p, v));
            cyc(48);
            wr8(0x70FC0B, v);
            cyc(bit == 0x04 ? 62 : 84);     /* [BRA,] JSL */
        }
    }
    w_SaveChecksum(cr);
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~(P_M | P_X))));
    cr->a = 0xEBCB;
    cyc(84);
    wr16(0x0000FB, 0xEBCB);
    cr->a = 0x0004;
    set_p(cr, nz16(cr->p, 0x0004));
    cyc(40);
    wr16(0x0000FD, 0x0004);
    (cyc(90), MenuPaletteLoad_cr(cr));
    (cyc(46), MenuPaletteSave_cr(cr));
    WindowHdmaChannelsSetup_cr(cr);
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~(P_M | P_X))));
    cr->a = 0x9068;
    cr->x = 0x001D;
    set_p(cr, nz16(cr->p, 0x001D));
    (cyc(132), GsuCF42To3000_cr(cr));
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~(P_M | P_X))));
    cyc(88);
    for (x = 0x0000; x != 0x2000; x = (uint16_t)(x + 2)) {
        cyc(106);
        cr->a = rd16(0x703000 + x);
        cyc(32);
        wr16(0x7FA7D7 + x, cr->a);
    }
    cyc(200);
    cr->x = x;
    set_p(cr, (uint8_t)((cr->p & ~P_N) | P_Z | P_C));
    cr->a = 0xDC8C;
    cr->x = 0x001C;
    set_p(cr, nz16(cr->p, 0x001C));
    GsuCF42To3000_cr(cr);
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~(P_M | P_X))));
    cr->a = 0x6000;
    cyc(186);
    ppu_wr16(VMADDL, 0x6000);
    for (x = 0x0000; x != 0x0800; x = (uint16_t)(x + 2)) {
        cyc(56);
        a = rd16(0x703000 + x);
        cr->a = champion_add(cr, a, 0x0100);
        cyc(62);
        ppu_wr16(VMDATAL, cr->a);
        cyc(50);
    }
    cyc(0);
    cr->x = x;
    set_p(cr, (uint8_t)((cr->p & ~P_N) | P_Z | P_C));
    champion_picture(cr, 0x4000, 0x18C76E, 0xC770, 0x0018);
    champion_picture(cr, 0x2000, 0x18998A, 0x998C, 0x0018);
    champion_picture(cr, 0x0000, 0x1C989A, 0x989C, 0x001C);
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~(P_M | P_X))));
    cr->a = 0xE87B;
    cr->x = 0x0004;
    set_p(cr, nz16(cr->p, 0x0004));
    (cyc(150), GsuCF42To3000_cr(cr));
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~(P_M | P_X))));
    cyc(170);
    a = (uint16_t)(rd16(0x000D4A) & 0x00FF);
    for (i = 0; i < 4; i++) {
        set_p(cr, (uint8_t)((cr->p & ~P_C) | (a >> 15)));
        a = (uint16_t)(a << 1);
    }
    x = a;
    yo = 0x0003;
    for (;;) {
        cyc(190);
        cr->a = rd16(0x0BED9F + x);
        wr16(0x0011E1, cr->a);
        cyc(72);
        cr->a = rd16(0x0BEDA1 + x);
        wr16(0x0011E3, cr->a);
        sx = x;
        cyc(174);
        cr->a = rd16(0x0BEDD7 + (uint16_t)(yo << 1));
        wr16(0x0011E5, cr->a);
        cyc(48);
        for (y2 = 0x0004;; y2--) {
            cyc(90);
            x2 = rd16(0x0011E5);
            for (y3 = 0x000B;; y3--) {
                cyc(56);
                a = (uint16_t)(rd16(0x703000 + x2) & 0x03FF);
                cyc(48);
                a |= rd16(0x0011E3);
                cyc(32);
                wr16(0x703000 + x2, a);
                x2 = (uint16_t)(x2 + 2);
                if (!y3)
                    break; else { cyc(40); }
            }
            cyc(118);
            a = rd16(0x0011E5);
            a = champion_add(cr, a, 0x0040);
            cyc(62);
            wr16(0x0011E5, a);
            if (!y2)
                break;
        }
        cyc(254);
        x2 = rd16(0x0BEDCF + (uint16_t)(yo << 1));
        a = rd16(0x0011E1);
        cyc(24);
        a |= rd16(0x0011E3);
        for (i = 0; i < 4; i++) {
            uint16_t c = a;

            cyc(62);
            wr16(0x703000 + x2, c);
            c = champion_add(cr, c, 0x0010);
            cyc(70);
            wr16(0x703040 + x2, c);
            c = champion_add(cr, c, 0x0010);
            cyc(70);
            wr16(0x703080 + x2, c);
            a = (uint16_t)(a + 1);
            x2 = (uint16_t)(x2 + 2);
            cyc(114);
        }
        cr->a = a;
        x = (uint16_t)(sx + 4);
        if (!yo)
            break; else { cyc(40); }
        yo--;
    }
    cr->x = x;
    cr->y = 0xFFFF;
    set_p(cr, nz16(cr->p, 0xFFFF));
    cyc(184);
    a = (uint16_t)(rd16(0x0018CB) & 0x0003);
    set_p(cr, (uint8_t)((cr->p & ~P_C) | (a >> 15)));
    a = (uint16_t)(a << 1);
    cyc(124);
    cr->a = rd16(0x0BEDDF + a);
    wr16(0x0011DF, cr->a);
    cr->x = 0x0628;
    set_p(cr, nz16(cr->p, 0x0628));
    PictureCellAttributes(cr);
    cr->x = 0x052E;
    set_p(cr, nz16(cr->p, 0x052E));
    PictureCellAttributes(cr);
    cr->a = 0x6400;
    cyc(48);
    ppu_wr16(VMADDL, 0x6400);
    for (x = 0x0000; x != 0x1000; x = (uint16_t)(x + 2)) {
        cyc(56);
        cr->a = rd16(0x703000 + x);
        cyc(24);
        ppu_wr16(VMDATAL, cr->a);
        cyc(50);
    }
    cyc(150);
    cr->x = x;
    set_p(cr, (uint8_t)((cr->p & ~P_N) | P_Z | P_C));
    cr->a = 0xEBCB;
    cr->x = 0x0004;
    set_p(cr, nz16(cr->p, 0x0004));
    GsuCF42To3000_cr(cr);
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~(P_M | P_X))));
    cyc(162);
    wr16(0x00162D, 0x0000);
    cyc(24);
    wr16(0x0016AF, 0x0000);
    cr->a = 0x0006;
    cyc(48);
    wr16(0x000D7D, 0x0006);
    cr->a = 0x0000;
    cyc(56);
    wr16(0x0018FD, 0x0000);
    cyc(24);
    wr16(0x00118B, 0x0000);
    cyc(24);
    wr16(0x000311, 0x0000);
    cyc(56);
    wr16(0x001903, 0x0000);
    cyc(32);
    wr16(0x0018B9, 0x0000);
    cr->a = 0x0003;
    cyc(56);
    wr16(0x001905, 0x0003);
    cr->a = 0x0000;
    cyc(56);
    wr16(0x001921, 0x0000);
    cyc(32);
    wr16(0x001925, 0x0000);
    cr->a = 0x0066;
    cyc(56);
    wr16(0x0011EF, 0x0066);
    cyc(32);
    a = (uint16_t)(rd16(0x000D4A) & 0x00FF);
    for (i = 0; i < 5; i++) {
        set_p(cr, (uint8_t)((cr->p & ~P_C) | (a >> 15)));
        a = (uint16_t)(a << 1);
    }
    cr->a = a;
    cyc(126);
    wr16(0x001919, a);
    cyc(32);
    a = (uint16_t)(rd16(0x000DD6) & 0x00FF);
    set_p(cr, (uint8_t)((nz16(cr->p, (uint16_t)(a - 0x0008)) & ~P_C) | (a >= 0x0008 ? (cyc(6), P_C) : 0)));
    cr->a = a == 0x0008 ? 0x0000 : 0x0020;
    cyc(120);
    wr16(0x001927, cr->a);
    if (a != 0x0008)
        cyc(22);   /* BRA */
    cr->a = 0x1234;
    cyc(48);
    wr16(0x001189, 0x1234);
    cyc(24);
    wr16(0x00118F, 0x0000);
    cyc(24);
    wr16(0x0011EB, 0x0000);
    cr->a = 0x0000;
    set_p(cr, nz16(cr->p, 0x0000));
    cyc(48);
    wr16(0x0011ED, 0x0000);
    (cyc(62), MenuActorPoolInit_cr(cr));
    cr->x = 0x0036;
    set_p(cr, nz16(cr->p, 0x0036));
    (cyc(86), MenuActorSpawnList_cr(cr));
    (cyc(62), w_MenuActorsRun(cr));
    (cyc(62), MenuOamBuild_cr(cr));
    r_sep(cr, P_M);
    cyc(62);
    hdmaen_wr(HDMAEN, 0xF0);
    cyc(40);
    wr8(0x000D3F, 0x20);
    cyc(40);
    hw_wr(VTIMEL, 0xE0);
    cyc(24);
    hw_wr(VTIMEH, 0x00);
    cyc(40);
    wr8(0x0011D7, 0x0C);
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0x21);
    set_p(cr, nz8(cr->p, 0x21));
    cyc(40);
    hw_wr(NMITIMEN, 0x21);
    (cyc(46), MenuWaitFrame_cr(cr));
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0x03);
    set_p(cr, nz8(cr->p, 0x03));
    cyc(40);
    ppu_wr(TM, 0x03);
    cr->db = db0;
    set_p(cr, nz8(cr->p, db0));
    r_plp(cr, p0);
    cyc(56);
}

void abi_ChampionScreenOpen(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ChampionScreenOpen(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void abi_ApproachTargetFull(void);
void abi_ApproachTargetZero(void);
void abi_MenuRunFrames(void);

/*
 * @routine 0B:ADD4 ChampionScreenRun
 * WHAT IT DOES: Runs the championship's closing screen: opens it
 * (ChampionScreenOpen), waits a frame, sets the fade target to full
 * (ApproachTargetFull); then each frame (MenuWaitFrame) runs the objects,
 * builds the sprite table, clears both players' presses ($118B, $118D) and
 * asks for frame updates 0-3 ($11D7), until the menu asks to leave
 * ($11EB); then the fade target to 0 (ApproachTargetZero) and the menu
 * frames run out (MenuRunFrames).
 * HOW: 16-bit, with 8-bit for the $11D7 update.
 * Entry: D = $0000. Exit: as MenuRunFrames leaves it.
 */
void ChampionScreenRun(Carried *cr)
{
    uint32_t b;
    int first;

    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~(P_M | P_X))));
    ChampionScreenOpen(cr);
    (cyc(46), MenuWaitFrame_cr(cr));
    (cyc(46), w_ApproachTargetFull(cr));
    first = 1;
    do {
        cyc(first ? 46 : 68);           /* JSR; later rounds also the BEQ back */
        first = 0;
        MenuWaitFrame_cr(cr);
        (cyc(62), w_MenuActorsRun(cr));
        (cyc(62), MenuOamBuild_cr(cr));
        b = (uint32_t)cr->db << 16;
        set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
        cyc(46);
        wr16(b | 0x118B, 0x0000);
        cyc(24);
        wr16(b | 0x118D, 0x0000);
        set_p(cr, (uint8_t)(cr->p | (P_M)));
        cyc(46);
        cr->a = (uint16_t)((cr->a & 0xFF00) | (uint8_t)(rd8(b | 0x11D7) | 0x0F));
        set_p(cr, nz8(cr->p, (uint8_t)cr->a));
        cyc(40);
        wr8(b | 0x11D7, (uint8_t)cr->a);
        set_p(cr, (uint8_t)(cr->p & ((uint8_t)~(P_M | P_X))));
        cyc(46);
        r_lda16(cr, b | 0x11EB);
    } while (!cr->a);
    cyc(62);
    w_ApproachTargetZero(cr);
    (cyc(46), MenuRunFrames_cr(cr));
    cyc(44);
}

void abi_ChampionScreenRun(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ChampionScreenRun(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void abi_MenuScreenOpen6(void);
void abi_MenuWaitFrame(void);
void abi_ApproachTargetFull(void);
void abi_ApproachTargetZero(void);
void abi_MenuRunFrames(void);
void abi_MenuActorsRun(void);
void abi_MenuOamBuild(void);
void abi_MenuActorFind(void);
void abi_DemoRunFirst(void);
void abi_DemoRunSecond(void);
void abi_DemoRunFourth(void);

void abi_BootWarm(void);

/*
 * @routine 0B:AF10 TitleScreenLoop
 * WHAT IT DOES: The title screen's frames: each runs the objects and builds
 * the sprites; while object kind $12 is idle (handler $D606, +$0A at 0) the
 * count $1913 goes up, and at $400 the screen fades out, $10DA = $FF and the
 * next of three attract demos runs ($1915 counts 0, 1, 2: DemoRunFirst,
 * DemoRunSecond, DemoRunFourth); a demo returning C set leaves with A =
 * $FFFF, C clear restarts the session (BootWarm). Both players' presses are
 * cleared and frame updates 0-3 asked for each frame; the menu's exit
 * ($11EB) leaves with A = 0.
 * HOW: The idle object through direct page + X; a branch back for the next
 * frame. BootWarm resets the stack (cpu_restart).
 * Entry: M 16-bit, X 16-bit; D = $0000. Exit: A = 0 or $FFFF; M 16-bit, X
 * 16-bit; or BootWarm, which never returns.
 */
void TitleScreenLoop(Carried *cr)
{
    uint32_t b;
    uint16_t v;

    for (;;) {
        cyc(46);
        MenuWaitFrame_cr(cr);
        (cyc(62), w_MenuActorsRun(cr));
        (cyc(62), MenuOamBuild_cr(cr));
        r_rep(cr, P_M);
        r_lda16_imm(cr, 0x0012);
        (cyc(32), MenuActorFind_cr(cr));
        cyc(22);
        r_lda16(cr, (uint16_t)(cr->d + 0x08 + cr->x));
        r_cmp16(cr, 0xD606);
        if (cr->p & P_Z) {
            cyc(62);
            r_lda16(cr, (uint16_t)(cr->d + 0x0A + cr->x));
            if (!cr->a) {
                cyc(48);
                r_lda16(cr, 0x001913);
                cr->a++;
                set_p(cr, nz16(cr->p, cr->a));
                cyc(46);
                wr16(0x001913, cr->a);
                r_cmp16(cr, 0x0400);
                if (cr->p & P_Z) {
                    cyc(86);
                    w_ApproachTargetZero(cr);
                    (cyc(46), MenuRunFrames_cr(cr));
                    r_sep(cr, P_M);
                    cr->a = (uint16_t)((cr->a & 0xFF00) | 0xFF);
                    set_p(cr, nz8(cr->p, 0xFF));
                    cyc(70);
                    wr8(0x0010DA, 0xFF);
                    r_rep(cr, P_M);
                    cyc(32);
                    r_lda16(cr, 0x001915);
                    cr->a++;
                    set_p(cr, nz16(cr->p, cr->a));
                    r_and16(cr, 0x0003);
                    r_cmp16(cr, 0x0003);
                    if (cr->p & P_Z)
                        r_lda16_imm(cr, 0x0000);
                    v = cr->a;
                    cyc(114);
                    wr16(0x001915, v);
                    r_cmp16(cr, 0x0000);
                    if (cr->p & P_Z)
                        { cyc(108); DemoRunFirst(cr); }
                    else {
                        r_cmp16(cr, 0x0001);
                        if (cr->p & P_Z)
                            { cyc(148); DemoRunSecond(cr); }
                        else
                            { cyc(142); DemoRunFourth(cr); }
                    }
                    if (cr->p & P_C) {
                        r_rep(cr, P_M | P_X);
                        r_lda16_imm(cr, 0xFFFF);
                        return;
                    }
                    cyc(48); /* BCS, JML */
                    carried_restart(cr, 0x038BAA, abi_BootWarm);
                    return;
                }
            }
        }
        b = (uint32_t)cr->db << 16;
        cyc(70);
        wr16(b | 0x118B, 0x0000);
        cyc(24);
        wr16(b | 0x118D, 0x0000);
        r_sep(cr, P_M);
        cyc(46);
        r_lda8(cr, b | 0x11D7);
        cr->a = (uint16_t)(cr->a | 0x0F);
        set_p(cr, nz8(cr->p, (uint8_t)cr->a));
        cyc(40);
        wr8(b | 0x11D7, (uint8_t)cr->a);
        r_rep(cr, P_M | P_X);
        cyc(24);
        r_lda16(cr, b | 0x11EB);
        if (cr->a) {
            cyc(52);
            r_lda16_imm(cr, 0x0000);
            return;
        } else { cyc(46); }
    }
}

void abi_TitleScreenLoop(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    TitleScreenLoop(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:AEF2 TitleScreenRun
 * WHAT IT DOES: The title screen: opens it (MenuScreenOpen6), $10DA = 0,
 * waits a frame and sets the fade target to full, then its loop
 * (TitleScreenLoop) with the idle count $1913 at 0.
 * HOW: 16-bit registers; $10DA and $1913 by long accesses.
 * Entry: any widths; D = $0000. Exit: as TitleScreenLoop leaves it.
 */
void TitleScreenRun(Carried *cr)
{
    r_rep(cr, P_M | P_X);
    w_MenuScreenOpen6(cr);
    r_sep(cr, P_M);
    cr->a = (uint16_t)(cr->a & 0xFF00);
    set_p(cr, nz8(cr->p, 0x00));
    cyc(70);
    wr8(0x0010DA, 0x00);
    r_rep(cr, P_M);
    (cyc(46), MenuWaitFrame_cr(cr));
    (cyc(46), w_ApproachTargetFull(cr));
    r_rep(cr, P_M | P_X);
    r_lda16_imm(cr, 0x0000);
    cyc(18);
    wr16(0x001913, 0x0000);
    TitleScreenLoop(cr);
}

void abi_TitleScreenRun(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    TitleScreenRun(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:F443 MenuScrollZeroBg12
 * WHAT IT DOES: Queues writes for the next vertical blank that put BG1 and
 * BG2 back at scroll 0 (both bytes of BG1HOFS, BG2HOFS, BG1VOFS and
 * BG2VOFS). Nothing in the game calls it.
 * HOW: Eight FrameByteWritePush calls with 0; X is pushed around each.
 * SEP #$30 first, so X and Y keep only their low bytes.
 * Entry: any widths. Exit: A low byte = 0, A high byte unchanged; X and Y
 * their low bytes (high bytes 0); P unchanged.
 */
void MenuScrollZeroBg12(Carried *cr)
{
    static const uint16_t reg[8] = {BG1HOFS, BG1HOFS, BG2HOFS, BG2HOFS, BG1VOFS, BG1VOFS, BG2VOFS, BG2VOFS};
    unsigned i;

    cr->x &= 0x00FF;
    cr->y &= 0x00FF;
    cr->a &= 0xFF00;
    for (i = 0; i < 8; i++)
        { cyc(i ? 304 : 220); FrameByteWritePush(cr->db, reg[i], 0x00); }
    cyc(214);
}

void abi_MenuScrollZeroBg12(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuScrollZeroBg12(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* MenuActorsRun, still in registers: its call with the registers loaded and taken
   back. */
static void w_MenuActorsRun(Carried *cr)
{
    cpu.a = cr->a;
    cpu.x = cr->x;
    cpu.y = cr->y;
    cpu.d = cr->d;
    cpu.p = cr->p;
    cpu.db = cr->db;
    abi_MenuActorsRun();
    cr->a = cpu.a;
    cr->x = cpu.x;
    cr->y = cpu.y;
    cr->d = cpu.d;
    cr->p = cpu.p;
    cr->db = cpu.db;
}

/* SaveChecksum as its adapter leaves the registers. */
static void w_SaveChecksum(Carried *cr)
{
    cr->a = (uint16_t)((cr->a & 0xFF00) | SaveChecksum());
}

/* ApuUpload as its adapter leaves the registers. */
static void w_ApuUpload(Carried *cr)
{
    ApuUploadEnd end = ApuUpload((uint8_t)cr->a, cr->y);

    cr->a = end.kick;
    cr->x = 0;
    cr->y = end.offset;
    set_p(cr, (uint8_t)((cr->p & ~(P_N | P_V)) | P_Z | P_C | P_M));
}

/* ApproachTargetFull as its adapter leaves the registers. */
static void w_ApproachTargetFull(Carried *cr)
{
    ApproachTargetFull(cr->db);
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0x0F);
}

/* ApproachTargetZero as its adapter leaves the registers. */
static void w_ApproachTargetZero(Carried *cr)
{
    ApproachTargetZero(cr->db);
}

/* MenuScreenOpen6 as its adapter leaves the registers. */
static void w_MenuScreenOpen6(Carried *cr)
{
    MenuScreenOpen6(cr);
}
