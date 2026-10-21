/*
 * unused_screen.c: a screen set-up in the work-RAM code at $7E:F33C-F6E8
 * and the bank 02 helpers it calls, none of which the game calls: a mode 2
 * screen with the Super FX picture at $2C00, HDMA from $7E:BC28-C22E and a
 * raster IRQ, palettes from $03:A145, and a row of 32 tilemap words built
 * at $0866 and sent to VRAM $2FA0.
 */
#include "machine.h"
#include "native.h"
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

    cyc(44);
    for (r = 0x210D; r <= 0x2114; r++) {
        wr8(dbw(bank, r), cyc_then(24, 0x00));
        wr8(dbw(bank, r), cyc_then(24, 0x00));
    }
    cyc(72);
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

    ppu_wr(dbw(bank, 0x2121), cyc_then(206, 0x70));
    for (y = 0; y != 0x20; y = (uint16_t)(y + 2)) {
        cyc(y ? 166 : 88); /* wrap: INX INX INY INY CPY BNE, then LDA long */
        v = rd8(PALETTES + o);
        ppu_wr(dbw(bank, 0x2122), v);
        cyc(64);
        v = rd8(PALETTES + 1 + o);
        ppu_wr(dbw(bank, 0x2122), v);
        o = (uint16_t)(o + 2);
    }
    cyc(126);
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

    cyc(212);
    for (y = 0; y != 0x20; y = (uint16_t)(y + 2)) {
        cyc(180);
        w = rd16(PALETTES + o);
        wr16(0x7EE997u + y, w);
        cyc(30);
        wr16(0x7E0802u + y, w);
        o = (uint16_t)(o + 2);
    }
    *x = o;
    cyc(150);
    w = (uint16_t)((w & 0xFF00) | rd8(TIMEUP));
    cyc(94); /* REP, CLI, PLB, PLP, RTL */
    return w;
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

    cyc(162);
    for (y = 0; y != 0x100; y = (uint16_t)(y + 2)) {
        cyc(160);
        w = rd16(COLOURS + y);
        cyc(40);
        wr16(0x00003E, (uint16_t)(w & 0x7FE0));
        cyc(30);
        w = rd16(COLOURS + y);
        r = (uint16_t)(w & 0x001F);
        if (r <= 3) {
            cyc(r == 3 ? 8 : 18); /* BEQ taken, ORA; or BCS not taken, ORA */
            r |= 0x0003;
        }
        r = (uint16_t)(r << 1);
        if (r >= 0x001F)
            { cyc(18); r = 0x001F; }
        r &= 0x001F;
        cyc(186);
        r |= rd16(0x00003E);
        cyc(30);
        wr16(COLOURS + y, r);
    }
    cyc(142);
    wr16(0x000800, 0x0000);
    cyc(172); /* PLB, PLX, PLY, PLP, RTL */
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

    cyc(56);
    for (x = 0xDF; x >= 0; x--)
        wr8(dbw(bank, (uint16_t)(COLOURS + x)), cyc_then(66, 0x00));
    cyc(110);
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
    cyc(92);
    wr8(0x7EBC2A, 0x5F);
    cyc(48);
    wr8(0x7EBC2C, 0x2D);
    cyc(48);
    wr8(0x7EBC2E, 0x32);
    cyc(72); /* PLP, RTL */
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
    wr8(dbw(bank, 0x420C), cyc_then(90, 0x00));
    ppu_wr(dbw(bank, 0x2100), cyc_then(40, 0x80));
    cyc(62); /* JSL */
    Set7EBC2BTrio(0x80);
    wr8(dbw(bank, 0x06E3), cyc_then(40, 0x96));
    cyc(62); /* JSL */
    WaitScanline(bank);
    wr8(dbw(bank, 0x06E3), cyc_then(84, 0x8C));
    cyc(62); /* JSL */
    WaitScanline(bank);
    cyc(116); /* SEP, REP, PLP, RTL */
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
    {
        uint16_t r = writes[i].reg;
        /* STA abs is 24; an LDX/LDY #imm before the store makes it 40 */
        cyc(i == 0 ? 150
            : (r == 0x2101 || r == 0x2115 || (r == 0x211B && writes[i - 1].reg == 0x211A) ||
               r == 0x2130 || r == 0x2132 || r == 0x4201) ? 40 : 24);
        if ((r >= 0x4202 && r <= 0x4206) || r == 0x211B || r == 0x211C)
            io_wr8(r, writes[i].v);             /* the multiply units: as C */
        else
            reg_wr(r, writes[i].v);
    }
    ppu_mul_factor = 0x0100;                     /* M7A: $00, then $01 */
    cyc(70);
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
    (cyc(46), abi_HardwareRegsInit02());
    cyc(44); /* RTL */
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
 * second test of $08ED never runs (the test repeats one just passed); it
 * runs only as TileRowBuildZero.
 * HOW: 16-bit registers (left so); work RAM through the data bank; the
 * rows through direct page indexed by X (D = $0000, bank 0).
 * Entry: D = $0000. Exit: M, X 16-bit. Not built ($08ED = 0): A = 0; Z
 * set, N clear. Same row: A = the row address, X = the table offset; Z,
 * C set, N clear. Built: A = 1; X = the row address + $40 (forwards) or -
 * 2 (backwards), Y = $40; N, Z clear, C set, V from the last add.
 */
static uint16_t tile_row_pick(uint8_t bank, uint16_t a, int back, uint16_t *x, uint16_t *y, uint8_t *p);

uint16_t TileRowBuild(uint8_t bank, uint16_t *x, uint16_t *y, uint8_t *p)
{
    uint16_t a;
    int back;

    *p &= (uint8_t)~(P_M | P_X);
    wr16(dbw(bank, 0x08EA), cyc_then(46, 0x0000));
    a = (uint16_t)(rd16(cyc_then(24, dbw(bank, 0x08ED))) & 0x00FF);
    if (!a) {
        cyc(40 + 42); /* AND, BNE; RTS */
        *p = nz16(*p, a);
        return a;
    }
    a = rd16(cyc_then(116, dbw(bank, 0x0342)));
    back = (a & 0x8000) != 0;
    if (back)
        { cyc(44); a = (uint16_t)-a; }
    return tile_row_pick(bank, a, back, x, y, p);
}

/* TileRowBuild from its row pick on: the level word a (made positive),
   read backwards with `back`. */
static uint16_t tile_row_pick(uint8_t bank, uint16_t a, int back, uint16_t *x, uint16_t *y, uint8_t *p)
{
    uint16_t t, i, w;

    *x = (uint16_t)((a >> 8 & 0x0007) << 1);
    cyc(160);
    t = rd16(0x008BAEu + *x);
    if (t == rd16(dbw(bank, 0x08E8))) {
        cyc(16 + 42); /* BNE; RTS */
        *p = (uint8_t)((*p & ~P_N) | P_Z | P_C);
        return t;
    }
    wr16(dbw(bank, 0x08E8), cyc_then(46, t));
    i = back ? (cyc(38), (uint16_t)(t + 0x3E)) : t;
    for (*y = 0; *y != 0x40; *y = (uint16_t)(*y + 2)) {
        unsigned s;

        cyc(114);
        w = rd16((uint16_t)i);
        a = rd16(dbw(bank, 0x08E6));
        s = (unsigned)w + a;
        *p = (uint8_t)((*p & ~P_V) | ((~(w ^ a) & (w ^ s) & 0x8000) ? P_V : 0));
        wr16(dbw(bank, (uint16_t)(ROW + *y)), cyc_then(30, (uint16_t)s));
        i = back ? (cyc(64), (uint16_t)(i - 2)) : (cyc(64), (uint16_t)(i + 2));
    }
    *x = i;
    wr16(dbw(bank, 0x08EA), cyc_then(80, 0x0001));
    cyc(42); /* RTS */
    *p = (uint8_t)((*p & ~(P_N | P_Z)) | P_C);
    return 0x0001;
}

void abi_TileRowBuild(void)
{
    cpu.a = TileRowBuild(cpu.db, &cpu.x, &cpu.y, &cpu.p);
}

/*
 * @routine 02:D96E TileRowBuildZero
 * WHAT IT DOES: TileRowBuild's LDA #0 after its second $08ED test, which
 * never runs (the test repeats one just passed): the row for level 0 read
 * forwards, as TileRowBuild from its row pick.
 * HOW: LDA #0, BRA to the pick; then as TileRowBuild. No code reaches
 * these bytes; a direct call is their only way in.
 * Entry: M, X 16-bit; D = $0000. Exit: as TileRowBuild when it reaches its
 * row pick.
 */
void abi_TileRowBuildZero(void)
{
    cyc(30);
    cpu.a = tile_row_pick(cpu.db, 0x0000, 0, &cpu.x, &cpu.y, &cpu.p);
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
    (cyc(46), abi_TileRowBuild());
    cyc(44); /* RTL */
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
    ppu_wr(dbw(bank, 0x2115), cyc_then(40, 0x80));
    wr8(dbw(bank, 0x4301), cyc_then(40, 0x18));
    wr16(dbw(bank, 0x4302), cyc_then(48, ROW));
    wr8(dbw(bank, 0x4304), cyc_then(24, 0x00));
    ppu_wr16(dbw(bank, 0x2116), cyc_then(48, 0x2FA0));
    wr16(dbw(bank, 0x4305), cyc_then(48, 0x0040));
    wr8(dbw(bank, 0x4300), cyc_then(40, 0x01));
    wr8(dbw(bank, 0x420B), cyc_then(40, 0x01));
    cyc(42); /* RTS */
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
    cyc(112);
    TileRowUpload(cpu.db);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x01);
    cpu.x = 0x0040;
    cyc(30);
    op_plp(cpu.p);
}

void CrashWhite(uint8_t bank);
void abi_SpriteSlotFirstLong(void);
void abi_Set7EBC2BTrio(void);
void abi_WaitScanline(void);
void abi_GsuClearScreenIfSet(void);

/*
 * @routine 7E:F5EC UnusedScreenTiles
 * @alias 02:D313
 * WHAT IT DOES: Writes the unused screen's tilemaps to VRAM: from $2C40,
 * 16 rows of three $39A0 words, 26 words counting up by $10 and three more
 * $39A0 words (row r starting at $1C00 + r); when the Super FX RAM byte
 * $70:023A is set, ten single words at $2F03-$2F07 and $2F18-$2F1C; then
 * 16 words of $FFFF at $1A00 and 16 at $4A00. The work-RAM copy is called
 * at its ROM address $02:D313.
 * HOW: Data bank $00; VMAIN = $80; 16-bit stores to VMADDL and VMDATAL.
 * DB is saved and restored; the count of the inner loop is on the stack.
 * Entry: any widths. Exit: M 8-bit, X 16-bit; A = $2A86, or 0 when
 * $70:023A is 0; X = $FFFF, Y = 0; N, Z from DB; C set, V clear.
 */
uint16_t UnusedScreenTiles(void)
{
    static const uint16_t marks[10][2] = {
        {0x2F03, 0x282E}, {0x2F04, 0x2846}, {0x2F05, 0x285E}, {0x2F06, 0x2876},
        {0x2F07, 0x288E}, {0x2F18, 0x2A26}, {0x2F19, 0x2A3E}, {0x2F1A, 0x2A56},
        {0x2F1B, 0x2A6E}, {0x2F1C, 0x2A86}};
    uint16_t a = 0x1C00, y, x, r;
    unsigned i;

    cyc(150);
    ppu_wr(0x002115, 0x80);
    cyc(70);
    ppu_wr16(0x002116, 0x2C40);
    for (y = 0; y != 0x10; y++) {
        cyc(y ? 212 : 126); /* the first row follows LDA, LDY, PHA */
        ppu_wr16(0x002118, 0x39A0);
        cyc(24);
        ppu_wr16(0x002118, 0x39A0);
        cyc(24);
        ppu_wr16(0x002118, 0x39A0);
        for (x = 0; x != 0x1A; x++) {
            cyc(x ? 122 : 84); /* CLC, ADC, INX, CPX, BNE taken */
            ppu_wr16(0x002118, a);
            a = (uint16_t)(a + 0x10);
        }
        cyc(170);
        ppu_wr16(0x002118, 0x39A0);
        cyc(24);
        ppu_wr16(0x002118, 0x39A0);
        cyc(24);
        ppu_wr16(0x002118, 0x39A0);
        a = (uint16_t)(a - 0x019F);
    }
    cyc(160);
    r = (uint16_t)(rd16(0x70023A) & 0x00FF);
    if (r) {
        cyc(40);
        for (i = 0; i < 10; i++) {
            cyc(48);
            ppu_wr16(0x002116, marks[i][0]);
            cyc(48);
            ppu_wr16(0x002118, marks[i][1]);
        }
        r = 0x2A86;
    } else { cyc(46); }
    cyc(48);
    ppu_wr16(0x002116, 0x1A00);
    cyc(12);
    for (y = 0x10; y; y--)
        { cyc(60); ppu_wr16(0x002118, 0xFFFF); }
    cyc(78);
    ppu_wr16(0x002116, 0x4A00);
    cyc(12);
    for (y = 0x10; y; y--)
        { cyc(60); ppu_wr16(0x002118, 0xFFFF); }
    cyc(124);
    return r;
}

void abi_UnusedScreenTiles(void)
{
    cpu.a = UnusedScreenTiles();
    cpu.x = 0xFFFF;
    cpu.y = 0x0000;
    cpu.p = (uint8_t)((nz8(cpu.p, cpu.db) & ~(P_X | P_V)) | P_M | P_C);
}

/*
 * @routine 7E:F5DF UnusedScreenTilesLocked
 * WHAT IT DOES: UnusedScreenTiles with interrupts off, then clears any
 * pending IRQ and turns interrupts on.
 * HOW: PHP, SEI, JSL to the ROM copy $02:D313, a long read of TIMEUP,
 * CLI, PLP.
 * Entry: any widths. Exit: A = UnusedScreenTiles' high byte and TIMEUP's
 * value; X = $FFFF, Y = 0 (their low bytes when X is 8-bit); P unchanged.
 */
void abi_UnusedScreenTilesLocked(void)
{
    uint8_t p = cpu.p;

    (cyc(98), abi_UnusedScreenTiles());
    cyc(32);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | rd8(TIMEUP));
    cyc(44);
    op_plp(p);
}

/*
 * @routine 7E:F40E UnusedScreenSetup
 * WHAT IT DOES: Sets up the unused screen, interrupts off: the NMI off,
 * the frame flags $00 and $084C cleared, the Super FX's SCMR shadow $0374 =
 * $21; HDMA off, the screen blanked and two scanline waits
 * (ScreenBlankAndWait); all of VRAM cleared and the tilemap at $2C00
 * filled with $01A1, then UnusedScreenTiles; the Super FX picture at
 * $2C00 ($48 = $4A = $2C00, $50 = $3000, $4E = 0; SCBR and BG12NBA from
 * them); mode 2 with BG1 at $2C00 and BG2 at $7300; the tile row
 * (TileRowBuildLong, TileRowUploadLong); palette 1 to colours $70-$7F;
 * colours $80-$FF by DMA from $7F:FA17; windows for the sprites only;
 * BG1-3 and sprites on the main screen, BG1-3 on the sub; the $7E:BC2A
 * values; HDMA channels 1-4 from $7E:BC28, $BC50, $B7A0, $C22E; $0316 =
 * $1E and the HDMA pointers at $084D, $084F, $083E, $0840 (and $0842 =
 * 0); the scroll cleared; a raster IRQ at line $8F; then, with $80 at
 * $7E:BC2B-BC2F, the screen still blanked and HDMA channel 1 on, it waits
 * for lines $BE and $64 and turns on the NMI, the vertical IRQ and the
 * joypad read ($31), sets $05D9 = $FF and turns interrupts on.
 * HOW: Registers through the data bank (16-bit stores where M is 16-bit
 * write the next register too); direct page with D = $0000; reads TIMEUP,
 * the Super FX status and RDNMI to acknowledge them.
 * Entry: D = $0000. Exit: M 8-bit, X 16-bit; A = TIMEUP's value (high
 * byte 0); X = 0, Y = $20; I clear, N, Z from A; C and V as TileRowBuild
 * left them (C from bit 3 of $4F when it built nothing).
 */
void abi_UnusedScreenSetup(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint16_t a;
    uint8_t v;
    int i, j;

    op_sep(P_M | P_X);
    cpu.p |= P_I;
    cyc(38);
    wr8(b | 0x4200, 0x00);
    cyc(16);
    wr8(0x000000, 0x00);
    cyc(24);
    wr8(b | 0x084C, 0x00);
    cyc(24);
    rd8(b | 0x4211);
    op_rep(P_M);
    cyc(32);
    rd16(0x003030);
    cyc(48);
    wr16(b | 0x0374, 0x0021);
    (cyc(62), abi_ScreenBlankAndWait());
    cyc(24);
    ppu_wr16(b | 0x2121, 0x0000);
    cyc(24);
    ppu_wr16(b | 0x2116, 0x0000);
    cyc(24);
    ppu_wr16(b | 0x2117, 0x0000);
    cyc(48);
    ppu_wr16(b | 0x2100, 0x0080);
    cyc(24);
    ppu_wr16(b | 0x2115, 0x0080);
    for (i = 0x7F; i; i--)
        { cyc(i == 0x7F ? 20 : 46); /* later rounds: DEX, BNE, LDY after DEY, BNE */
          for (j = 0x100; j; j--) {
            cyc(60);
            ppu_wr16(b | 0x2118, 0x0000);
            cyc(24);
            ppu_wr16(b | 0x2119, 0x0000);
        } }
    op_rep(P_M | P_X);
    cyc(108);
    ppu_wr16(b | 0x2116, 0x2C00);
    cyc(12);
    for (i = 0x400; i; i--)
        { cyc(60); ppu_wr16(b | 0x2118, 0x01A1); }
    cyc(136);
    op_sep(P_M);
    abi_UnusedScreenTiles();
    cyc(62);
    wr16(0x00004E, 0x0000);
    cyc(40);
    wr16(0x000050, 0x3000);
    cyc(40);
    wr16(0x000048, 0x2C00);
    cyc(40);
    wr16(0x00004A, 0x2C00);
    cyc(16);
    v = rd8(0x000049);
    cyc(52);
    wr8(b | 0x3038, (uint8_t)(v >> 2));
    cyc(16);
    v = rd8(0x00004F);
    cpu.p = (uint8_t)((cpu.p & ~P_C) | (v & 0x08 ? P_C : 0));
    cyc(96);
    ppu_wr(b | 0x210B, (uint8_t)(v >> 4 | 0x50));
    cyc(40);
    ppu_wr(b | 0x2101, 0x03);
    cyc(40);
    ppu_wr(b | 0x2107, 0x2C);
    cyc(40);
    ppu_wr(b | 0x2108, 0x73);
    cyc(40);
    ppu_wr(b | 0x2105, 0x02);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x02);
    (cyc(62), abi_TileRowBuildLong());
    (cyc(62), abi_TileRowUploadLong());
    op_sep(P_M);
    op_rep(P_X);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x01);
    (cyc(100), abi_PaletteLoad70());
    cyc(40);
    wr8(b | 0x4301, 0x22);
    cyc(40);
    ppu_wr(b | 0x2121, 0x80);
    cyc(48);
    wr16(b | 0x4302, 0xFA17);
    cyc(40);
    wr8(b | 0x4304, 0x7F);
    cyc(48);
    wr16(b | 0x4305, 0x0100);
    cyc(40);
    wr8(b | 0x4300, 0x00);
    cyc(40);
    wr8(b | 0x420B, 0x01);
    cyc(24);
    ppu_wr(b | 0x2123, 0x00);
    cyc(24);
    ppu_wr(b | 0x2124, 0x00);
    cyc(40);
    ppu_wr(b | 0x2125, 0x20);
    cyc(24);
    ppu_wr(b | 0x212A, 0x00);
    cyc(40);
    ppu_wr(b | 0x212B, 0x0C);
    cyc(24);
    ppu_wr(b | 0x212E, 0x00);
    cyc(24);
    ppu_wr(b | 0x212F, 0x00);
    cyc(24);
    ppu_wr(b | 0x2126, 0x00);
    cyc(24);
    ppu_wr(b | 0x2127, 0x00);
    cyc(40);
    ppu_wr(b | 0x212C, 0x17);
    cyc(40);
    ppu_wr(b | 0x212D, 0x07);
    (cyc(62), abi_Set7EBC2AValues());
    op_sep(P_M);
    cyc(62);
    wr8(b | 0x4310, 0x00);
    cyc(40);
    wr8(b | 0x4311, 0x00);
    cyc(48);
    wr16(b | 0x4312, 0xBC28);
    cyc(40);
    wr8(b | 0x4314, 0x7E);
    cyc(24);
    wr8(b | 0x4317, 0x00);
    cyc(40);
    wr8(b | 0x4320, 0x02);
    cyc(40);
    wr8(b | 0x4321, 0x0F);
    cyc(48);
    wr16(b | 0x4322, 0xBC50);
    cyc(40);
    wr8(b | 0x4324, 0x7E);
    cyc(24);
    wr8(b | 0x4327, 0x00);
    cyc(40);
    wr8(b | 0x4330, 0x01);
    cyc(40);
    wr8(b | 0x4331, 0x26);
    cyc(48);
    wr16(b | 0x4332, 0xB7A0);
    cyc(40);
    wr8(b | 0x4334, 0x7E);
    cyc(24);
    wr8(b | 0x4337, 0x00);
    cyc(40);
    wr8(b | 0x4340, 0x02);
    cyc(40);
    wr8(b | 0x4341, 0x10);
    cyc(48);
    wr16(b | 0x4342, 0xC22E);
    cyc(40);
    wr8(b | 0x4344, 0x7E);
    cyc(24);
    wr8(b | 0x4347, 0x00);
    cyc(40);
    wr8(b | 0x0316, 0x1E);
    cyc(48);
    wr16(b | 0x084D, 0xBC50);
    cyc(48);
    wr16(b | 0x084F, 0xBF3F);
    cyc(48);
    wr16(b | 0x083E, 0xB7A0);
    cyc(48);
    wr16(b | 0x0840, 0xB9E4);
    cyc(24);
    wr8(b | 0x0842, 0x00);
    (cyc(62), abi_ScrollRegsClear());
    op_sep(P_M);
    cyc(70);
    wr16(b | 0x4209, 0x008F);
    cyc(48);
    wr16(b | 0x4207, 0x0000);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x80);
    (cyc(78), abi_Set7EBC2BTrio());
    cyc(24);
    ppu_wr(b | 0x2100, 0x80);
    cyc(24);
    wr8(b | 0x084C, 0x00);
    cyc(40);
    wr8(b | 0x420C, 0x02);
    cyc(40);
    wr8(b | 0x06E3, 0xBE);
    (cyc(62), abi_WaitScanline());
    cyc(84);
    wr8(b | 0x06E3, 0x64);
    (cyc(62), abi_WaitScanline());
    cyc(68);
    rd8(b | 0x4210);
    cyc(40);
    wr8(b | 0x4200, 0x31);
    cyc(40);
    wr8(b | 0x05D9, 0xFF);
    cyc(32);
    v = rd8(TIMEUP);
    a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.a = a;
    cpu.x = 0x0000;
    cpu.p = (uint8_t)(nz8(cpu.p, v) & ~P_I);
    cyc(58); /* CLI, RTL */
}

/*
 * @routine 7E:F3E3 UnusedScreenStart
 * WHAT IT DOES: Waits for the frame flag $00 to clear, sets it to 2 and
 * clears $0306; waits for $0854 to be set, runs GsuClearScreenIfSet,
 * waits for $0854 to be 2 and clears it; then SpriteSlotFirstLong and the
 * stop screen (CrashWhite). Nothing in the game calls it.
 * HOW: Polling loops on direct page $00 (D = $0000) and on $0854 through
 * the data bank; 8-bit A, 16-bit index registers.
 * Entry: D = $0000. Exit: never returns.
 */
void abi_UnusedScreenStart(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;

    op_sep(P_M);
    op_rep(P_X);
    while (rd8(cyc_then(60, 0x000000)))
        ;
    cyc(48);
    wr8(0x000000, 0x02);
    cyc(24);
    wr8(b | 0x0306, 0x00);
    while (!rd8(cyc_then(24, b | 0x0854)))
        ;
    cyc(78);
    abi_GsuClearScreenIfSet();
    while (rd8(cyc_then(24, b | 0x0854)) != 0x02)
        cyc(38); /* CMP, BNE taken */
    cyc(56);
    wr8(b | 0x0854, 0x00);
    (cyc(62), abi_SpriteSlotFirstLong());
    (cyc(62), CrashWhite(cpu.db));
}

void abi_ObjectLinkBobStart(void);

/*
 * @routine 7E:F6E5 ObjectLinkBobStartLong
 * WHAT IT DOES: A second copy of ViewParamsBobLong ($7E:F737), which
 * nothing calls: starts bob 1 with A as its kind ($1A1D = A, $1A1F =
 * $1A23 = 0, $1A21 = $FE).
 * HOW: JSR ObjectLinkBobStart, RTL.
 * Entry: M 8-bit (the code is 8-bit); data bank $7E. Exit: M and X 8-bit
 * (X and Y high bytes 0); A low byte = $FE; N set, Z clear.
 */
void abi_ObjectLinkBobStartLong(void)
{
    abi_ObjectLinkBobStart();
}
