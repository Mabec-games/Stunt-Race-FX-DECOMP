/*
 * unused_screen.c: a screen set-up in the work-RAM code at $7E:F33C-F6E8
 * and the bank 02 helpers it calls, none of which the game calls: a mode 2
 * screen with the Super FX picture at $2C00, HDMA from $7E:BC28-C22E and a
 * raster IRQ, palettes from $03:A145, and a row of 32 tilemap words built
 * at $0866 and sent to VRAM $2FA0.
 */
#include "machine.h"
#include "hw.h"
#include "cpu_ops.h"

void Set7EBC2BTrio(uint8_t v);
void WaitScanline(uint8_t bank);

#define PALETTES 0x03A145u   /* 16 colours (32 bytes) per palette */
#define COLOURS 0x0722u      /* 128 colour words, through the data bank */
#define ROW 0x0866u          /* 32 tilemap words */

static uint32_t dbw(uint8_t bank, uint16_t a)
{
    return (uint32_t)bank << 16 | a;
}

/*
 * @routine 7E:F77C ScrollRegsClear
 * @alias 02:D4A3
 * WHAT IT DOES: Scrolls the four backgrounds to 0, 0. Called at its ROM
 * address $02:D4A3; its first three bytes are the last of the work-RAM
 * code's copy, so the routine is named by that copy's address.
 * HOW: Two zero stores to each of BG1HOFS-BG4VOFS ($210D-$2114: low byte,
 * then high), through the data bank. P is saved and restored.
 * Entry: any widths. Exit: every register unchanged.
 */
void ScrollRegsClear(uint8_t bank)
{
    uint16_t r;

    for (r = 0x210D; r <= 0x2114; r++) {
        wr8(dbw(bank, r), 0x00);
        wr8(dbw(bank, r), 0x00);
    }
}

void abi_ScrollRegsClear(void)
{
    ScrollRegsClear(cpu.db);
}

/*
 * @routine 02:D4D8 PaletteLoad70
 * WHAT IT DOES: Loads palette A (16 colours from the table at $03:A145)
 * into CGRAM colours $70-$7F.
 * HOW: X = A's low byte times 32 (XBA, three LSRs); CGADD = $70, then 32
 * long reads from $03:A145,X written to CGDATA. CGADD and CGDATA through
 * the data bank. P is saved and restored.
 * Entry: any widths. Exit: A = the palette's offset's high byte and the
 * last byte read; X = the offset + $20, Y = $20 (their low bytes when X
 * is 8-bit); P unchanged.
 */
uint8_t PaletteLoad70(uint8_t bank, uint8_t index, uint16_t *x)
{
    uint16_t o = (uint16_t)(index << 5), y;
    uint8_t v = 0;

    wr8(dbw(bank, 0x2121), 0x70);
    for (y = 0; y != 0x20; y = (uint16_t)(y + 2)) {
        v = rd8(PALETTES + o);
        wr8(dbw(bank, 0x2122), v);
        v = rd8(PALETTES + 1 + o);
        wr8(dbw(bank, 0x2122), v);
        o = (uint16_t)(o + 2);
    }
    *x = o;
    return v;
}

void abi_PaletteLoad70(void)
{
    uint16_t o = (uint16_t)((uint8_t)cpu.a << 5), x;
    uint8_t v = PaletteLoad70(cpu.db, (uint8_t)cpu.a, &x);

    cpu.a = (uint16_t)((o & 0xFF00) | v);
    cpu.x = x;
    cpu.y = 0x0020;
    op_plp(cpu.p);
}

/*
 * @routine 02:D506 PaletteToBuffers
 * WHAT IT DOES: Copies palette A (16 colours from the table at $03:A145)
 * into the colour buffers at $7E:E997 and $7E:0802, with interrupts off.
 * HOW: X = A's low byte times 32; data bank $7E; 16 word reads from
 * $03:A145,X, each stored to both buffers; then TIMEUP is read (clearing
 * a pending IRQ) and interrupts are enabled. DB and P are saved and
 * restored.
 * Entry: any widths. Exit: A = the last colour's high byte and TIMEUP's
 * value; X = the offset + $20, Y = $20 (their low bytes when X is 8-bit);
 * DB, P unchanged.
 */
uint16_t PaletteToBuffers(uint8_t index, uint16_t *x)
{
    uint16_t o = (uint16_t)(index << 5), y, w = 0;

    for (y = 0; y != 0x20; y = (uint16_t)(y + 2)) {
        w = rd16(PALETTES + o);
        wr16(0x7EE997u + y, w);
        wr16(0x7E0802u + y, w);
        o = (uint16_t)(o + 2);
    }
    *x = o;
    return (uint16_t)((w & 0xFF00) | rd8(TIMEUP));
}

void abi_PaletteToBuffers(void)
{
    uint16_t x;

    cpu.a = PaletteToBuffers((uint8_t)cpu.a, &x);
    cpu.x = x;
    cpu.y = 0x0020;
    op_plp(cpu.p);
}

/*
 * @routine 02:D5FD ColoursRedDouble
 * WHAT IT DOES: Doubles the red of the 128 colours at $0722 (raising any
 * red below 3 to 3 first, capping at 31) and clears the word at $0800.
 * HOW: Data bank $00; per colour, green and blue (AND $7FE0) go to $3E,
 * the colour is read again for red, and red is put back ORed with $3E.
 * The count is kept on the stack. P, Y, X and DB are saved and restored.
 * Entry: any widths; D = $0000. Exit: A = 0 (both bytes); X, Y, DB, P
 * unchanged.
 */
void ColoursRedDouble(void)
{
    uint16_t y, w, r;

    for (y = 0; y != 0x100; y = (uint16_t)(y + 2)) {
        w = rd16(COLOURS + y);
        wr16(0x00003E, (uint16_t)(w & 0x7FE0));
        w = rd16(COLOURS + y);
        r = (uint16_t)(w & 0x001F);
        if (r <= 3)
            r |= 0x0003;
        r = (uint16_t)(r << 1);
        if (r >= 0x001F)
            r = 0x001F;
        r &= 0x001F;
        r |= rd16(0x00003E);
        wr16(COLOURS + y, r);
    }
    wr16(0x000800, 0x0000);
}

void abi_ColoursRedDouble(void)
{
    ColoursRedDouble();
    cpu.a = 0x0000;
}

/*
 * @routine 02:D64A ColoursClear
 * WHAT IT DOES: Clears the 224 bytes at $0722-$0801.
 * HOW: 8-bit zero stores from X = $DF down to 0, through the data bank.
 * X is saved and restored; M is left 8-bit.
 * Entry: X 16-bit. Exit: A low byte = 0; X unchanged; M 8-bit; N, Z from
 * X.
 */
void ColoursClear(uint8_t bank)
{
    int x;

    for (x = 0xDF; x >= 0; x--)
        wr8(dbw(bank, (uint16_t)(COLOURS + x)), 0x00);
}

void abi_ColoursClear(void)
{
    ColoursClear(cpu.db);
    cpu.a = (uint16_t)(cpu.a & 0xFF00);
    cpu.p = nz16((uint8_t)(cpu.p | P_M), cpu.x);
}

/*
 * @routine 02:D66B Set7EBC2AValues
 * WHAT IT DOES: Stores $5F at $7E:BC2A, $2D at $7E:BC2C and $32 at
 * $7E:BC2E.
 * HOW: Three long byte stores. P is saved and restored.
 * Entry: any widths. Exit: A low byte = $32; P unchanged.
 */
void Set7EBC2AValues(void)
{
    wr8(0x7EBC2A, 0x5F);
    wr8(0x7EBC2C, 0x2D);
    wr8(0x7EBC2E, 0x32);
}

void abi_Set7EBC2AValues(void)
{
    Set7EBC2AValues();
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x32);
}

/*
 * @routine 02:D682 ScreenBlankAndWait
 * WHAT IT DOES: Turns HDMA off and blanks the screen, stores $80 at
 * $7E:BC2B, $BC2D and $BC2F, then waits for scanline $96 and then $8C.
 * HOW: HDMAEN = 0 and INIDISP = $80 through the data bank, Set7EBC2BTrio,
 * and WaitScanline twice with $06E3 = $96, then $8C. P is saved and
 * restored.
 * Entry: any widths. Exit: A low byte = $8C; P unchanged.
 */
void ScreenBlankAndWait(uint8_t bank)
{
    wr8(dbw(bank, 0x420C), 0x00);
    wr8(dbw(bank, 0x2100), 0x80);
    Set7EBC2BTrio(0x80);
    wr8(dbw(bank, 0x06E3), 0x96);
    WaitScanline(bank);
    wr8(dbw(bank, 0x06E3), 0x8C);
    WaitScanline(bank);
}

void abi_ScreenBlankAndWait(void)
{
    ScreenBlankAndWait(cpu.db);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x8C);
}

/*
 * @routine 02:D6B3 HardwareRegsInit02
 * WHAT IT DOES: Puts the video and CPU registers in a known state, as
 * HardwareRegsInit does, except that BG3HOFS is written once.
 * HOW: 8-bit registers, data bank $00 (left set); A = 0 for most stores,
 * X and Y for the others. P is saved and restored.
 * Entry: any widths. Exit: A low byte = 0; X = $FF, Y = $30 (high bytes
 * 0); DB = $00; P unchanged.
 */
void HardwareRegsInit02(void)
{
    static const struct { uint16_t reg; uint8_t v; } writes[] = {
        {0x2100, 0x8F}, {0x2101, 0x00}, {0x2102, 0x00}, {0x2103, 0x00},
        {0x2105, 0x00}, {0x2106, 0x00}, {0x2107, 0x00}, {0x2108, 0x00},
        {0x2109, 0x00}, {0x210A, 0x00}, {0x210B, 0x00}, {0x210C, 0x00},
        {0x210D, 0x00}, {0x210D, 0x00}, {0x210E, 0x00}, {0x210E, 0x00},
        {0x210F, 0x00}, {0x210F, 0x00}, {0x2110, 0x00}, {0x2110, 0x00},
        {0x2111, 0x00}, {0x2112, 0x00}, {0x2112, 0x00}, {0x2113, 0x00},
        {0x2113, 0x00}, {0x2114, 0x00}, {0x2114, 0x00}, {0x2115, 0x80},
        {0x2116, 0x00}, {0x2117, 0x00}, {0x211A, 0x00}, {0x211B, 0x00},
        {0x211B, 0x01}, {0x211C, 0x00}, {0x211C, 0x00}, {0x211D, 0x00},
        {0x211D, 0x00}, {0x211E, 0x00}, {0x211E, 0x01}, {0x211F, 0x00},
        {0x211F, 0x00}, {0x2120, 0x00}, {0x2120, 0x00}, {0x2121, 0x00},
        {0x2122, 0x00}, {0x2123, 0x00}, {0x2124, 0x00}, {0x2125, 0x00},
        {0x2126, 0x00}, {0x2127, 0x00}, {0x2128, 0x00}, {0x2129, 0x00},
        {0x212A, 0x00}, {0x212B, 0x00}, {0x212C, 0x00}, {0x212D, 0x00},
        {0x212E, 0x00}, {0x2130, 0x30}, {0x2131, 0x00}, {0x2132, 0xE0},
        {0x2133, 0x00}, {0x4200, 0x00}, {0x4201, 0xFF}, {0x4202, 0x00},
        {0x4203, 0x00}, {0x4204, 0x00}, {0x4205, 0x00}, {0x4206, 0x00},
        {0x4207, 0x00}, {0x4208, 0x00}, {0x4209, 0x00}, {0x420A, 0x00},
        {0x420B, 0x00}, {0x420C, 0x00}, {0x420D, 0x00}};
    unsigned i;

    for (i = 0; i < sizeof writes / sizeof writes[0]; i++)
        wr8(writes[i].reg, writes[i].v);
}

void abi_HardwareRegsInit02(void)
{
    HardwareRegsInit02();
    cpu.a = (uint16_t)(cpu.a & 0xFF00);
    cpu.x = 0x00FF;
    cpu.y = 0x0030;
    cpu.db = 0x00;
}

/*
 * @routine 02:D6AF HardwareRegsInit02Long
 * WHAT IT DOES: HardwareRegsInit02 for callers in other banks.
 * HOW: JSR, RTL.
 * Entry: any widths. Exit: as HardwareRegsInit02.
 */
void HardwareRegsInit02Long(void)
{
    HardwareRegsInit02();
}

void abi_HardwareRegsInit02Long(void)
{
    abi_HardwareRegsInit02();
}

/*
 * @routine 02:D95B TileRowBuild
 * WHAT IT DOES: Unless $08ED is 0, builds the 32 tilemap words at $0866
 * from one of eight 32-word rows in bank 0 picked by the high byte of the
 * word at $0342 (the table of row addresses at $00:8BAE, by its low three
 * bits), each word plus the word at $08E6. A negative $0342 is negated
 * first and its row is read backwards. Nothing is built when the row is
 * the one recorded at $08E8; otherwise $08E8 records it and $08EA is set
 * to 1 ($08EA is cleared first in every case). The LDA #0 after the
 * second test of $08ED can never run (the test repeats one just passed).
 * HOW: 16-bit registers (left so); work RAM through the data bank; the
 * rows through direct page indexed by X (D = $0000, bank 0).
 * Entry: D = $0000. Exit: M, X 16-bit. Not built ($08ED = 0): A = 0; Z
 * set, N clear. Same row: A = the row address, X = the table offset; Z,
 * C set, N clear. Built: A = 1; X = the row address + $40 (forwards) or -
 * 2 (backwards), Y = $40; N, Z clear, C set, V from the last add.
 */
uint16_t TileRowBuild(uint8_t bank, uint16_t *x, uint16_t *y, uint8_t *p)
{
    uint16_t a, t, i, w;
    int back;

    *p &= (uint8_t)~(P_M | P_X);
    wr16(dbw(bank, 0x08EA), 0x0000);
    a = (uint16_t)(rd16(dbw(bank, 0x08ED)) & 0x00FF);
    if (!a) {
        *p = nz16(*p, a);
        return a;
    }
    a = rd16(dbw(bank, 0x0342));
    back = (a & 0x8000) != 0;
    if (back)
        a = (uint16_t)-a;
    *x = (uint16_t)((a >> 8 & 0x0007) << 1);
    t = rd16(0x008BAEu + *x);
    if (t == rd16(dbw(bank, 0x08E8))) {
        *p = (uint8_t)((*p & ~P_N) | P_Z | P_C);
        return t;
    }
    wr16(dbw(bank, 0x08E8), t);
    i = back ? (uint16_t)(t + 0x3E) : t;
    for (*y = 0; *y != 0x40; *y = (uint16_t)(*y + 2)) {
        unsigned s;

        w = rd16((uint16_t)i);
        a = rd16(dbw(bank, 0x08E6));
        s = (unsigned)w + a;
        *p = (uint8_t)((*p & ~P_V) | ((~(w ^ a) & (w ^ s) & 0x8000) ? P_V : 0));
        wr16(dbw(bank, (uint16_t)(ROW + *y)), (uint16_t)s);
        i = back ? (uint16_t)(i - 2) : (uint16_t)(i + 2);
    }
    *x = i;
    wr16(dbw(bank, 0x08EA), 0x0001);
    *p = (uint8_t)((*p & ~(P_N | P_Z)) | P_C);
    return 0x0001;
}

void abi_TileRowBuild(void)
{
    cpu.a = TileRowBuild(cpu.db, &cpu.x, &cpu.y, &cpu.p);
}

/*
 * @routine 02:D957 TileRowBuildLong
 * WHAT IT DOES: TileRowBuild for callers in other banks.
 * HOW: JSR, RTL.
 * Entry: D = $0000. Exit: as TileRowBuild.
 */
uint16_t TileRowBuildLong(uint8_t bank, uint16_t *x, uint16_t *y, uint8_t *p)
{
    return TileRowBuild(bank, x, y, p);
}

void abi_TileRowBuildLong(void)
{
    abi_TileRowBuild();
}

/*
 * @routine 02:D9EA TileRowUpload
 * WHAT IT DOES: Sends the 32 tilemap words at $0866 to VRAM $2FA0.
 * HOW: DMA channel 0, mode 1 to VMDATA ($2118) with VMAIN = $80, 64 bytes
 * from $00:0866; the registers through the data bank.
 * Entry: M 8-bit, X 16-bit. Exit: A low byte = 1; X = $0040.
 */
void TileRowUpload(uint8_t bank)
{
    wr8(dbw(bank, 0x2115), 0x80);
    wr8(dbw(bank, 0x4301), 0x18);
    wr16(dbw(bank, 0x4302), ROW);
    wr8(dbw(bank, 0x4304), 0x00);
    wr16(dbw(bank, 0x2116), 0x2FA0);
    wr16(dbw(bank, 0x4305), 0x0040);
    wr8(dbw(bank, 0x4300), 0x01);
    wr8(dbw(bank, 0x420B), 0x01);
}

void abi_TileRowUpload(void)
{
    TileRowUpload(cpu.db);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x01);
    cpu.x = 0x0040;
}

/*
 * @routine 02:D9E0 TileRowUploadLong
 * WHAT IT DOES: TileRowUpload for callers in other banks.
 * HOW: 8-bit A, 16-bit index registers around the JSR; P is saved and
 * restored.
 * Entry: any widths. Exit: A low byte = 1; X = $0040 (its low byte when
 * X is 8-bit); P unchanged.
 */
void TileRowUploadLong(uint8_t bank)
{
    TileRowUpload(bank);
}

void abi_TileRowUploadLong(void)
{
    TileRowUpload(cpu.db);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x01);
    cpu.x = 0x0040;
    op_plp(cpu.p);
}
