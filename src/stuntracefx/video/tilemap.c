/*
 * tilemap.c: BG tilemap buffers in work RAM and their uploads.
 */
#include "machine.h"
#include "hw.h"
#include "cpu_ops.h"

/*
 * @routine 03:ED89 TilemapRowFromGsuRam
 * WHAT IT DOES: Copies 32 tilemap words from Super FX RAM ($70:3000 + A +
 * $F1) to the buffer at $93D7 + Y in the data bank, setting bit 13 (priority)
 * on each.
 * HOW: A word loop counted in direct page $F3.
 * Entry: M 16-bit, X 16-bit, D = $0000. Exit: A = the last word written; X
 * and Y advanced by $40; $F3 = 0; Z set, N clear; C, V from the first add.
 */
static uint16_t tilemap_row(uint8_t bank, uint16_t a, uint16_t *x, uint16_t *y, uint8_t *p)
{
    cyc(142);
    uint16_t f1 = rd16(0x0000F1);
    uint32_t s = (uint32_t)a + f1;
    uint16_t v = 0;
    unsigned i;

    *p = (uint8_t)((*p & ~(P_C | P_V)) | (s > 0xFFFF ? P_C : 0)
                   | ((~(a ^ f1) & (a ^ (uint16_t)s) & 0x8000) ? P_V : 0));
    *x = (uint16_t)s;
    cyc(54);
    wr16(0x0000F3, 0x0020);
    for (i = 0; i < 32; i++) {
        cyc(54);
        v = (uint16_t)(rd16(0x703000 + *x) | 0x2000);
        cyc(54);
        wr16((uint32_t)bank << 16 | (uint16_t)(0x93D7 + *y), v);
        *x = (uint16_t)(*x + 2);
        *y = (uint16_t)(*y + 2);
        cyc(102);
        wr16(0x0000F3, (uint16_t)(0x1F - i));
    }
    cyc(16);
    return v;
}

void TilemapRowFromGsuRam(uint8_t bank, uint16_t a, uint16_t y)
{
    uint16_t x;
    uint8_t p = 0;

    tilemap_row(bank, a, &x, &y, &p);
}

void abi_TilemapRowFromGsuRam(void)
{
    uint8_t p = cpu.p;

    cpu.a = tilemap_row(cpu.db, cpu.a, &cpu.x, &cpu.y, &p);
    cpu.p = (uint8_t)((p & ~P_N) | P_Z);
}

/*
 * @routine 03:EAF3 VramFillGradient
 * WHAT IT DOES: Writes 16 rows of 32 words to VRAM: row r holds A + r, A + r
 * + 16, A + r + 32, ... (each word 16 more than the last).
 * HOW: Nested loops writing VMDATA; the row start lives in direct page $F1,
 * the step ($10) in $F3.
 * Entry: M 16-bit, X 8-bit, D = $0000. Exit: A = the last word written + 16;
 * X = 0, Y = 0; $F1 = A + 16; $F3 = $10; Z set, N clear; C, V from the last
 * add.
 */
static uint16_t vram_fill_gradient(uint16_t start, uint8_t *p)
{
    uint16_t a = 0;
    unsigned r, c;

    cyc(238);
    wr16(0x0000F1, start);
    cyc(40);
    wr16(0x0000F3, 0x0010);
    for (r = 0; r < 16; r++) {
        cyc(68);
        a = rd16(0x0000F1);
        for (c = 0; c < 32; c++) {
            uint32_t s;
            cyc(24);
            wr16(0x002118, a);
            s = (uint32_t)a + 0x10;
            *p = (uint8_t)((*p & ~(P_C | P_V)) | (s > 0xFFFF ? P_C : (cyc(82), 0))
                           | ((~(a ^ 0x10) & (a ^ (uint16_t)s) & 0x8000) ? P_V : 0));
            a = (uint16_t)s;
        }
        cyc(10);
        wr16(0x0000F1, cyc_then(14, (uint16_t)(rd16(0x0000F1) + 1)));
    }
    return a;
}

void VramFillGradient(uint16_t start)
{
    uint8_t p = 0;

    vram_fill_gradient(start, &p);
}

void abi_VramFillGradient(void)
{
    uint8_t p = cpu.p;

    cpu.a = vram_fill_gradient(cpu.a, &p);
    cpu.x = 0;
    cpu.y = 0;
    cpu.p = (uint8_t)((p & ~P_N) | P_Z);
}

/*
 * @routine 03:DCC0 VramDmaTileFromGsuRam
 * WHAT IT DOES: Copies one 32-byte tile (number A, counted from the offset
 * at $00F1) from Super FX RAM to VRAM word address X.
 * HOW: Channel 0 in mode 1, bank $70, 32 bytes.
 * Entry: M 16-bit, X 16-bit; DB a bank where $00F1 is work RAM. Exit: A = 1;
 * M 16-bit; N, Z clear; C, V from the add.
 */
void VramDmaTileFromGsuRam(uint16_t tile, uint16_t vram, uint8_t *p)
{
    cyc(178);
    uint16_t a = (uint16_t)(tile << 5), f1 = rd16(0x0000F1);
    uint32_t s = (uint32_t)a + f1;

    *p = (uint8_t)((*p & ~(P_C | P_V)) | (s > 0xFFFF ? P_C : 0)
                   | ((~(a ^ f1) & (a ^ (uint16_t)s) & 0x8000) ? P_V : 0));
    cyc(24);
    wr16(A1T0L, (uint16_t)s);
    cyc(24);
    wr16(VMADDL, vram);
    cyc(48);
    wr16(DMAP0, 0x1801);
    cyc(48);
    wr16(DAS0L, 0x0020);
    cyc(62);
    wr8(VMAIN, 0x80);
    cyc(40);
    wr8(A1B0, 0x70);
    cyc(40);
    wr8(MDMAEN, 0x01);
}

void abi_VramDmaTileFromGsuRam(void)
{
    uint8_t p = cpu.p;

    VramDmaTileFromGsuRam(cpu.a, cpu.x, &p);
    cpu.a = 0x0001;
    cpu.p = (uint8_t)(p & ~(P_N | P_Z | P_M));
}

/*
 * The frame rows: 32 words of tile $2001 + base, then 35 words of tile $2002
 * + base, or $2010 + base when the HUD mode ($0D2B) is 1 or 3 and the byte
 * at $10CD is 0. Written to VMDATA a word at a time; $F3 holds the base.
 * Returns the flags of the last add.
 */
static uint8_t frame_rows(uint8_t bank, uint16_t base, uint8_t p)
{
    uint32_t b = (uint32_t)bank << 16;
    unsigned i, s = 0;
    uint16_t t, mode;

    cyc(160);
    wr16(0x0000F3, base);
    for (i = 0; i < 0x20; i++)
        { cyc(118); wr16(VMDATAL, (uint16_t)(0x2001 + base)); }
    cyc(18);
    for (i = 0; i < 0x23; i++) {
        cyc(60);
        mode = rd16(b | 0x0D2B) & 0x00FF;
        t = 0x2002;
        if ((mode == 1 || (cyc(40), mode == 3)) && !(rd16(cyc_then(94, b | 0x10CD)) & 0x00FF))
            t = 0x2010; else { cyc(28); }
        s = (unsigned)t + base;
        cyc(134);
        wr16(VMDATAL, (uint16_t)s);
    }
    return (uint8_t)((p & ~(P_C | P_V)) | (s > 0xFFFF ? P_C : 0)
                     | ((~(t ^ base) & (t ^ (uint16_t)s) & 0x8000) ? P_V : 0));
}

/*
 * The picture rows: the 192 words of the Super FX picture at $70:3000, each
 * plus base, to VMDATA. Returns the flags of the last add, with C set by the
 * final compare.
 */
static uint8_t picture_rows(uint16_t base, uint8_t p)
{
    unsigned x, s = 0;
    uint16_t t = 0;

    cyc(184);
    wr16(0x0000F3, base);
    for (x = 0; x < 0x180; x += 2) {
        cyc(56);
        t = rd16(0x703000 + x);
        s = (unsigned)t + base;
        cyc(70);
        wr16(VMDATAL, (uint16_t)s);
    }
    return (uint8_t)((p & ~P_V) | P_C | ((~(t ^ base) & (t ^ (uint16_t)s) & 0x8000) ? P_V : 0));
}

/*
 * @routine 03:EA76 VramFrameRows2B0
 * WHAT IT DOES: Writes the frame rows with tile base $02B0 at the current
 * VRAM address.
 * HOW: frame_rows; A is saved and restored around it.
 * Entry: M 16-bit, D = $0000; the VRAM address and VMAIN set. Exit: A
 * unchanged; X = 0; M 16-bit, X 16-bit; N, Z from A; C, V from the last add.
 */
void VramFrameRows2B0(uint8_t bank)
{
    frame_rows(bank, 0x02B0, 0);
}

void abi_VramFrameRows2B0(void)
{
    cpu.p = (uint8_t)(nz16(frame_rows(cpu.db, 0x02B0, cpu.p), cpu.a) & ~(P_M | P_X));
    cpu.x = 0;
}

/*
 * @routine 03:EA7E VramFrameRows1B0
 * WHAT IT DOES: As VramFrameRows2B0, with tile base $01B0.
 * HOW: As VramFrameRows2B0.
 * Entry: as VramFrameRows2B0. Exit: as VramFrameRows2B0.
 */
void VramFrameRows1B0(uint8_t bank)
{
    frame_rows(bank, 0x01B0, 0);
}

void abi_VramFrameRows1B0(void)
{
    cpu.p = (uint8_t)(nz16(frame_rows(cpu.db, 0x01B0, cpu.p), cpu.a) & ~(P_M | P_X));
    cpu.x = 0;
}

/*
 * @routine 03:EA86 VramPictureRows2B0
 * WHAT IT DOES: Writes the Super FX picture as tilemap words with tile base
 * $02B0 at the current VRAM address.
 * HOW: picture_rows; A is saved and restored around it.
 * Entry: M 16-bit, D = $0000; the VRAM address and VMAIN set. Exit: A
 * unchanged; X = $180; M 16-bit, X 16-bit; N, Z from A; C set; V from the
 * last add.
 */
void VramPictureRows2B0(void)
{
    picture_rows(0x02B0, 0);
}

void abi_VramPictureRows2B0(void)
{
    cpu.p = (uint8_t)(nz16(picture_rows(0x02B0, cpu.p), cpu.a) & ~(P_M | P_X));
    cpu.x = 0x0180;
}

/*
 * @routine 03:EA91 VramPictureRows1B0
 * WHAT IT DOES: As VramPictureRows2B0, with tile base $01B0.
 * HOW: As VramPictureRows2B0.
 * Entry: as VramPictureRows2B0. Exit: as VramPictureRows2B0.
 */
void VramPictureRows1B0(void)
{
    picture_rows(0x01B0, 0);
}

void abi_VramPictureRows1B0(void)
{
    cpu.p = (uint8_t)(nz16(picture_rows(0x01B0, cpu.p), cpu.a) & ~(P_M | P_X));
    cpu.x = 0x0180;
}

/* Step X by $40 as TXA / CLC / ADC #$40 / TAX does; A = the new X. */
static uint16_t step_40(uint16_t *x, uint8_t *p)
{
    uint16_t s = (uint16_t)(*x + 0x40);

    *p = (uint8_t)((nz16(*p, s) & ~(P_C | P_V)) | ((unsigned)*x + 0x40 > 0xFFFF ? P_C : (cyc(24), 0)) | ((~*x & s & 0x8000) ? P_V : 0));
    *x = s;
    return s;
}

/*
 * @routine 02:FB27 TilemapRowFrom7FBBD7
 * WHAT IT DOES: Copies four words from $7F:BBD7 + X to the tilemap buffer at
 * $7F:93D7 + X, and moves X on by $40 (one row of 32 words).
 * HOW: Four 16-bit long copies.
 * Entry: M 16-bit, X 16-bit; X = byte offset. Exit: A = X = the offset + $40;
 * N, Z, C, V from the add.
 */
uint16_t TilemapRowFrom7FBBD7(uint16_t *x, uint8_t *p)
{
    unsigned i;

    cyc(130);
    for (i = 0; i < 4; i++)
        { cyc(32); wr16(0x7F93D7 + 2 * i + *x, cyc_then(32, rd16(0x7FBBD7 + 2 * i + *x))); }
    return step_40(x, p);
}

void abi_TilemapRowFrom7FBBD7(void)
{
    uint8_t p = cpu.p;

    cpu.a = TilemapRowFrom7FBBD7(&cpu.x, &p);
    cpu.p = p;
}

/*
 * @routine 02:FAFE TilemapRowFromFB4E
 * WHAT IT DOES: Copies four words from $FB4E + Y in the data bank to the
 * tilemap buffer at $7F:93D7 + X; Y moves on by 8, X by $40.
 * HOW: Four 16-bit copies.
 * Entry: M 16-bit, X 16-bit; X = byte offset, Y = source offset. Exit: A = X
 * = the offset + $40; N, Z, C, V from that add.
 */
uint16_t TilemapRowFromFB4E(uint8_t bank, uint16_t *x, uint16_t *y, uint8_t *p)
{
    unsigned i;

    cyc(108);
    for (i = 0; i < 4; i++)
        { cyc(78); wr16(0x7F93D7 + 2 * i + *x, rd16((uint32_t)bank << 16 | (uint16_t)(0xFB4E + 2 * i + *y))); }
    cyc(88);
    *y = (uint16_t)(*y + 8);
    return step_40(x, p);
}

void abi_TilemapRowFromFB4E(void)
{
    uint8_t p = cpu.p;

    cpu.a = TilemapRowFromFB4E(cpu.db, &cpu.x, &cpu.y, &p);
    cpu.p = p;
}

/*
 * @routine 07:CF14 TilemapPaletteFill
 * WHAT IT DOES: Sets the palette field (bits 2-4 of the attribute byte) of
 * Y tilemap entries in the buffer at $7F:93D6, from entry offset X, to the
 * bits in direct page $F1.
 * HOW: Read, mask with $E3, OR in $F1, write back; X steps by 2. It is the
 * tail of TilemapPaletteCycle, also called on its own.
 * Entry: M 8-bit, X 16-bit, D = $0000; X = entry offset (bytes), Y = count
 * (1 or more), $F1 = the palette bits. Exit: A low byte = the last
 * attribute byte; X past the last entry; Y = 0; Z set, N clear.
 */
void TilemapPaletteFill(uint16_t *x, uint16_t *y, uint16_t *a)
{
    do {
        cyc(78);
        uint8_t v = (uint8_t)((rd8(0x7F93D7 + *x) & 0xE3) | rd8(cyc_then(32, 0x0000F1)));

        cyc(32);
        wr8(0x7F93D7 + *x, v);
        *a = (uint16_t)((*a & 0xFF00) | v);
        *x += 2;
    } while (--*y);
cyc(100);
    }

void abi_TilemapPaletteFill(void)
{
    TilemapPaletteFill(&cpu.x, &cpu.y, &cpu.a);
    cpu.p = (uint8_t)((cpu.p & ~P_N) | P_Z);
}

/*
 * @routine 07:CEDF TilemapPaletteCycle
 * WHAT IT DOES: Every fourth frame, steps the phase at $0DD5 (0 to 5) and
 * sets the palette field of Y tilemap entries in the buffer at $7F:93D6,
 * starting at entry offset X, to the phase's palette: from the table at
 * $07:CF2C when the HUD mode word is 2, else from $07:CF26.
 * HOW: Skips unless the low two bits of the frame counter are 0. The
 * palette bits go through direct page $F1; each entry's attribute byte keeps
 * its bits other than 2-4.
 * Entry: M 8-bit, X 16-bit, D = $0000; X = entry offset (bytes), Y = count
 * (1 or more). Exit: when skipped, A low byte = the frame counter's low two
 * bits; N, Z from them; X, Y unchanged. Otherwise: A = the table byte after
 * the phase's and the last attribute byte; X past the last entry; Y = 0; C
 * set when the HUD mode word is 2 or more; Z set, N clear. M 8-bit.
 */
int TilemapPaletteCycle(uint8_t bank, uint16_t *x, uint16_t *y, uint16_t *a)
{
    uint32_t b = (uint32_t)bank << 16;
    uint8_t phase, pal, frame;
    uint16_t mode, table;

    cyc(140);
    frame = rd8(b | 0x0E3B);
    if (frame & 0x03) {
        cyc(74);
        *a = (uint16_t)((*a & 0xFF00) | (frame & 0x03));
        return 0;
    }
    cyc(62);
    phase = (uint8_t)(rd8(b | 0x0DD5) + 1);
    if (phase >= 6)
        { cyc(10); phase = 0; }
    cyc(76);
    wr8(b | 0x0DD5, phase);
    cyc(114);
    mode = rd16(b | 0x0D2B);
    table = mode == 2 ? 0xCF2C : (cyc(16), 0xCF26);
    cyc(132);
    pal = rd8(0x070000 | (uint16_t)(table + phase));
    wr8(0x0000F1, pal);
    *a = (uint16_t)(rd8(0x070000 | (uint16_t)(table + phase + 1)) << 8);
    TilemapPaletteFill(x, y, a);
    return mode >= 2 ? 2 : 1;
}

void abi_TilemapPaletteCycle(void)
{
    int r = TilemapPaletteCycle(cpu.db, &cpu.x, &cpu.y, &cpu.a);

    if (r == 0)
        cpu.p = nz8(cpu.p, (uint8_t)cpu.a);
    else
        cpu.p = (uint8_t)((cpu.p & ~(P_N | P_C)) | P_Z | (r == 2 ? P_C : 0));
    cpu.p |= P_M;
}

/*
 * @routine 03:EA2E ScreenFrameAndPictures
 * WHAT IT DOES: Writes the two framed picture panels to VRAM: at $7800 the
 * frame rows (base $02B0), the gradient from $2000 and at $7A40 the picture
 * (base $02B0); at $7C00 the frame rows (base $01B0), the gradient from
 * $2010 and at $7E40 the picture (base $01B0).
 * HOW: VMAIN = $80; VramFrameRows, VramFillGradient and VramPictureRows for
 * each panel, setting the VRAM address between them.
 * Entry: D = $0000. Exit: A = $7E40; X = $180; P unchanged.
 */
void ScreenFrameAndPictures(uint8_t bank)
{
    wr16(VMADDL, 0x7800);
    wr8(VMAIN, 0x80);
    VramFrameRows2B0(bank);
    VramFillGradient(0x2000);
    wr16(VMADDL, 0x7A40);
    VramPictureRows2B0();
    wr16(VMADDL, 0x7C00);
    VramFrameRows1B0(bank);
    VramFillGradient(0x2010);
    wr16(VMADDL, 0x7E40);
    VramPictureRows1B0();
}

void abi_ScreenFrameAndPictures(void)
{
    uint8_t p = cpu.p;

    cpu.p = (uint8_t)((cpu.p & ~P_M) | P_X);
    cpu.a = 0x7800;
    cyc(190);
    wr16(VMADDL, cpu.a);
    cpu.x = 0x80;
    cyc(40);
    wr8(VMAIN, 0x80);
    abi_VramFrameRows2B0();
    cpu.p = (uint8_t)((cpu.p & ~P_M) | P_X);
    cpu.x &= 0x00FF;
    cpu.y &= 0x00FF;
    cpu.a = 0x2000;
    abi_VramFillGradient();
    cpu.p &= (uint8_t)~P_M;
    cpu.a = 0x7A40;
    cyc(142);
    wr16(VMADDL, cpu.a);
    abi_VramPictureRows2B0();
    cpu.a = 0x7C00;
    cyc(194);
    wr16(VMADDL, cpu.a);
    abi_VramFrameRows1B0();
    cpu.a = 0x2010;
    cpu.p = (uint8_t)((cpu.p & ~P_M) | P_X);
    cpu.x &= 0x00FF;
    cpu.y &= 0x00FF;
    abi_VramFillGradient();
    cpu.p &= (uint8_t)~P_M;
    cpu.a = 0x7E40;
    cyc(142);
    wr16(VMADDL, cpu.a);
    abi_VramPictureRows1B0();
    cpu.p = p;
}

uint16_t RandomWordStep(uint8_t bank);

/*
 * @routine 03:F47B VramNoiseTiles
 * WHAT IT DOES: Fills $400 words of VRAM from address 0 with random tiles
 * $20-$3F (from RandomWordStep); first stores $8000 as a word at BG4VOFS,
 * which leaves BG4VOFS 0 and sets VMAIN to $80.
 * HOW: A 16-bit store to $2114/$2115, then VMDATA writes; $0D18 is the
 * random word.
 * Entry: any widths. Exit: A = the last word written; X = 0; M 16-bit, X
 * 16-bit; Z set, N clear.
 */
uint16_t VramNoiseTiles(uint8_t bank)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t v = 0;
    unsigned i;

    cyc(70);
    wr16(b | 0x2114, 0x8000);
    cyc(48);
    wr16(b | 0x2116, 0x0000);
    for (i = 0; i < 0x400; i++) {
        cyc(86);
        RandomWordStep(bank);
        cyc(24);
        v = (uint16_t)((rd16(b | 0x0D18) & 0x001F) | 0x0020);
        cyc(72);
        wr16(b | 0x2118, v);
    }
    cyc(72);
    return v;
}

void abi_VramNoiseTiles(void)
{
    cpu.a = VramNoiseTiles(cpu.db);
    cpu.x = 0;
    cpu.p = (uint8_t)((cpu.p & ~(P_N | P_M | P_X)) | P_Z);
}

uint8_t Bg2ScrollFromTable(uint16_t band);

/*
 * @routine 7E:B24C TilemapRowsTo5C00
 * WHAT IT DOES: Writes 17 tilemap rows of 26 entries from the table at
 * $7F:93D7 to VRAM from $5C00 (one 32-entry row apart), then sets BG2's
 * scroll from screen band 0.
 * HOW: VMAIN $80; for each row the VRAM address, then 26 words to VMDATA
 * from $7F:93D7 plus the row offset in direct page $F9 (steps of $40 up to
 * $440); the row address rides on the stack. Then Bg2ScrollFromTable with
 * X = 0. The PPU is reached through the data bank.
 * Entry: X 16-bit, D = $0000. Exit: A low byte = BG2's vertical scroll high
 * byte, high byte $5E; X = Y = 0; M 8-bit; N, Z from A low byte; C set, V
 * clear.
 */
uint8_t TilemapRowsTo5C00(uint8_t bank)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t vram = 0x5C00, off;
    unsigned i;

    cyc(152);
    wr8(b | 0x2115, 0x80);
    cyc(38);
    wr16(0x0000F9, 0x0000);
    cyc(8);
    do {
        cyc(106);
        wr16(b | 0x2116, vram);
        vram = (uint16_t)(vram + 0x0020);
        cyc(84);
        off = rd16(0x0000F9);
        for (i = 0; i < 26; i++)
            { cyc(96); wr16(b | 0x2118, cyc_then(24, rd16(0x7F93D7 + (uint16_t)(off + 2 * i)))); }
        cyc(74);
        off = rd16(0x0000F9);
        off = (uint16_t)(off + 0x0040);
        cyc(54);
        wr16(0x0000F9, off);
    } while (off != 0x0440);
    cyc(174);
    return Bg2ScrollFromTable(0);
}

void abi_TilemapRowsTo5C00(void)
{
    uint8_t v = TilemapRowsTo5C00(cpu.db);

    cpu.a = (uint16_t)(0x5E00 | v);
    cpu.x = 0;
    cpu.y = 0;
    cpu.p = (uint8_t)(((nz8(cpu.p, v) & ~(P_V | P_X)) | P_C | P_M));
}

/*
 * @routine 02:FA91 TilemapLevelColumns
 * WHAT IT DOES: Draws twelve 4-row columns into the tilemap buffer, each
 * filled to the level (0-4, more counts as 4) in $0F05-$0F10: the filled
 * rows come from $02:FB4E, the empty ones above them from $7F:BBD7.
 * HOW: For column i the buffer offset is the word at $02:FB6E + 2i; level n
 * takes n rows from TilemapRowFromFB4E starting at source offset (4 - n) *
 * 8, then 4 - n rows from TilemapRowFrom7FBBD7. Data bank 2 inside.
 * Entry: any widths. Exit: A = the last offset + $40 (16-bit); X = its low
 * byte, Y = $0C (with the 8-bit index registers P restores); P and the data
 * bank unchanged.
 */
uint16_t TilemapLevelColumns(void)
{
    uint16_t x = 0, y, a = 0;
    uint8_t p = 0, n;
    unsigned i, k;

    cyc(6);
    for (i = 0; i < 12; i++) {
        cyc(312);
        x = rd16(0x02FB6E + 2 * i);
        n = rd8(0x020F05 + i);
        if (n > 4)
            n = 4;
        y = (uint16_t)((4 - n) * 8);
        for (k = 0; k < n; k++)
            a = TilemapRowFromFB4E(0x02, &x, &y, &p);
        for (k = n; k < 4; k++)
            a = TilemapRowFrom7FBBD7(&x, &p);
    }
    return a;
}

void abi_TilemapLevelColumns(void)
{
    cpu.a = TilemapLevelColumns();
    cpu.x = cpu.a;
    cpu.y = 0x000C;
    if (cpu.p & P_X)
        cpu.x &= 0x00FF;
}

/*
 * @routine 7E:E8F4 TilemapRowToE3D7
 * WHAT IT DOES: Copies 32 tilemap entries from $93D7 + X to $E3D7 + Y, or
 * from the alternate buffer at $BBD7 + X when bit 14 of $0DA4 is set (no
 * script has reached that case yet).
 * HOW: A word loop counted down in direct page $F3; buffers through the
 * data bank, $0DA4 with a long load.
 * Entry: M 16-bit, X 16-bit, D = $0000; X = source offset, Y = destination
 * offset. Exit: A = the last word copied; X, Y + $40; N clear, Z set; C, V
 * unchanged.
 */
uint16_t TilemapRowToE3D7(uint8_t bank, uint16_t *x, uint16_t *y)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t src, v = 0, n;

    cyc(40);
    wr16(0x0000F3, 0x0020);
    cyc(32);
    src = (rd16(0x000DA4) & 0x4000) ? (cyc(6), 0xBBD7) : 0x93D7;
    cyc(18);
    do {
        cyc(52);
        v = rd16(b | (uint16_t)(src + *x));
        cyc(30);
        wr16(b | (uint16_t)(0xE3D7 + *y), v);
        *x = (uint16_t)(*x + 2);
        *y = (uint16_t)(*y + 2);
        cyc(72);
        n = (uint16_t)(rd16(0x0000F3) - 1);
        cyc(14);
        wr16(0x0000F3, n);
    } while (n);
    cyc(58);
    return v;
}

void abi_TilemapRowToE3D7(void)
{
    cpu.a = TilemapRowToE3D7(cpu.db, &cpu.x, &cpu.y);
    cpu.p = (uint8_t)((cpu.p & ~P_N) | P_Z);
}

uint16_t ObjectLinkFlag4000Step(void);

/* The body of BackdropRowsUpdate; returns A as the code leaves it. */
static uint16_t backdrop_rows_update(uint8_t bank, uint16_t *xo, uint16_t *yo)
{
    uint32_t b = (uint32_t)bank << 16;
    cyc(164);
    uint16_t sel = rd16(0x000D7F), x = (uint16_t)(sel << 1), y, h, s, a, end;
    unsigned n;

    (cyc(90), ObjectLinkFlag4000Step());
    cyc(140);
    h = rd16(0x0006CD);
    s = (uint16_t)((h << 1) + 0x42 + (h >> 15));
    cyc(138);
    wr16(b | 0x0D96, s < 0x0300 ? s : (cyc(18), 0x0300));
    cyc(144);
    wr16(b | (uint16_t)(0x0D8E + x), cyc_then(30, (uint16_t)((h & 0x0007) - 0x20 + rd16(0x7EE811 + x))));
    cyc(24);
    wr16(b | (uint16_t)(0x0D92 + x), cyc_then(106, (uint16_t)(((rd16(b | 0x06CF) >> 1) & 0x0007) - 0x18)));
    cyc(70);
    wr8(0x00211B, 0x40);
    cyc(48);
    wr8(0x00211B, 0x01);
    cyc(46);
    wr16(0x00211C, cyc_then(98, (uint16_t)(((uint16_t)(rd16(b | 0x06CD) + rd16(cyc_then(38, b | 0x0DA2))) >> 3) & 0x003F)));
    cyc(32);
    wr16(0x0000F1, cyc_then(16, rd16(0x002134)));
    cyc(24);
    x = (uint16_t)(((rd16(b | 0x06CF) >> 3) & 0x00FE) + rd16(cyc_then(96, 0x0000F1)));
    cyc(186);
    wr16(0x0000F1, 0x0011);
    cyc(32);
    sel = rd16(0x000D7F);
    y = sel ? (cyc(6), 0x0440) : 0x0000;
    end = sel ? 0x0880 : 0x0440;
    cyc(18);
    for (;;) {
        cyc(18);
        if (x >= 0x2800) {
            a = 0x20C0;
            do {
                cyc(104);
                wr16(0x7F0000 | (uint16_t)(0xE3D7 + y), a);
                y = (uint16_t)(y + 2);
            } while (y < end);
            cyc(52);
            break;
        } else { cyc(90); }
        TilemapRowToE3D7(0x7F, &x, &y);
        x = (uint16_t)(x + 0x0100);
        a = x;
        cyc(82);
        n = (uint16_t)(rd16(0x0000F1) - 1);
        cyc(14);
        wr16(0x0000F1, (uint16_t)n);
        if (!n)
            break;
    }
    cyc(182);
    *xo = x;
    *yo = y;
    return a;
}

/*
 * @routine 7E:E815 BackdropRowsUpdate
 * WHAT IT DOES: Updates the scrolling backdrop for the Super FX buffer
 * $0D7F selects: sets $0D96 to twice the view angle $06CD plus $42 (at most
 * $300), BG2's scroll pair for that buffer from the angle and $06CF, and
 * copies 17 rows of 32 entries from the backdrop map at $7F:93D7 into that
 * buffer's tilemap ($7F:E3D7, or + $440 for buffer 1), filling
 * with $20C0 once the map ends ($2800 bytes).
 * HOW: ObjectLinkFlag4000Step first. The map row is ((($06CD + $0DA2) / 8)
 * AND $3F) * $140 by the PPU multiplier ($0140 in M7A, the row in M7B and
 * M7C), plus ($06CF / 8) AND $FE; rows go through TilemapRowToE3D7 with the
 * data bank at $7F and step $140 (17 counted in direct page $F1). The
 * vertical scroll is (angle AND 7) - $20 plus the word at $7E:E811 + 2 *
 * buffer, the horizontal (($06CF / 2) AND 7) - $18. The clamp of $0D96 has
 * not been reached by any script yet.
 * Entry: any widths, D = $0000. Exit: A = the next map offset, or $20C0
 * after a fill; X, Y past the copy (low bytes when P restores 8-bit index
 * registers); P and the data bank unchanged.
 */
void BackdropRowsUpdate(uint8_t bank)
{
    uint16_t x, y;

    backdrop_rows_update(bank, &x, &y);
}

void abi_BackdropRowsUpdate(void)
{
    cpu.a = backdrop_rows_update(cpu.db, &cpu.x, &cpu.y);
    if (cpu.p & P_X) {
        cpu.x &= 0x00FF;
        cpu.y &= 0x00FF;
    }
}

/*
 * @routine 03:D306 BackdropAnglesFromView
 * WHAT IT DOES: Sets the backdrop's scroll from the view position: $06CD
 * (the angle BackdropRowsUpdate scrolls by) = -($0664 / 64), kept below $E8
 * and at least -$38, plus $98, modulo $200; $06CF = ($0666 / 32) AND $7FF.
 * HOW: Arithmetic shifts as CMP #$8000 / ROR A; the two limits are tested
 * with CMP and the sign of the difference, as the original does. Work RAM
 * through the data bank. The upper limit is not reached in the traces so
 * far.
 * Entry: any widths. Exit: A = the $06CF value; X, Y unchanged; P
 * unchanged except N, Z, which the final pull of Y sets from Y.
 */
uint16_t BackdropAnglesFromView(uint8_t bank)
{
    uint32_t b = (uint32_t)bank << 16;
    cyc(98);
    uint16_t v = rd16(b | 0x0664), w;
    int i;

    for (i = 0; i < 6; i++)
        v = (uint16_t)((v >> 1) | (v & 0x8000));
    v = (uint16_t)-v;
    if (!((uint16_t)(v - 0x00E8) & 0x8000))
        { cyc(18); v = 0x00E8; }
    if ((uint16_t)(v - 0xFFC8) & 0x8000)
        { cyc(18); v = 0xFFC8; }
    cyc(444);
    wr16(b | 0x06CD, (uint16_t)((v + 0x0098) & 0x01FF));
    cyc(24);
    w = rd16(b | 0x0666);
    for (i = 0; i < 5; i++)
        w = (uint16_t)((w >> 1) | (w & 0x8000));
    w &= 0x07FF;
    cyc(238);
    wr16(b | 0x06CF, w);
    return w;
}

void abi_BackdropAnglesFromView(void)
{
    cpu.a = BackdropAnglesFromView(cpu.db);
    cpu.p = (cpu.p & P_X) ? nz8(cpu.p, (uint8_t)cpu.y) : (cyc(130), nz16(cpu.p, cpu.y));
}

/*
 * @routine 07:C293 TilemapPaletteCycle87
 * WHAT IT DOES: TilemapPaletteCycle on nine entries from entry offset $87.
 * HOW: Sets X and Y and calls it.
 * Entry: M 8-bit, D = $0000. Exit: as TilemapPaletteCycle, with X 16-bit;
 * when skipped, X = $87 and Y = 9.
 */
void TilemapPaletteCycle87(uint8_t bank, uint16_t *a)
{
    uint16_t x = 0x0087, y = 0x0009;

    TilemapPaletteCycle(bank, &x, &y, a);
}

void abi_TilemapPaletteCycle87(void)
{
    cpu.x = 0x0087;
    cpu.y = 0x0009;
    cpu.p &= (uint8_t)~P_X;
    abi_TilemapPaletteCycle();
}

/*
 * @routine 07:C29F TilemapPaletteCycle107
 * WHAT IT DOES: TilemapPaletteCycle on 13 entries from entry offset $107.
 * HOW: Sets X and Y and calls it.
 * Entry: M 8-bit, D = $0000. Exit: as TilemapPaletteCycle, with X 16-bit;
 * when skipped, X = $107 and Y = 13.
 */
void TilemapPaletteCycle107(uint8_t bank, uint16_t *a)
{
    uint16_t x = 0x0107, y = 0x000D;

    TilemapPaletteCycle(bank, &x, &y, a);
}

void abi_TilemapPaletteCycle107(void)
{
    cpu.x = 0x0107;
    cpu.y = 0x000D;
    cpu.p &= (uint8_t)~P_X;
    (cyc(46), abi_TilemapPaletteCycle());
}

/*
 * @routine 07:C2AB TilemapPaletteCycle187
 * WHAT IT DOES: TilemapPaletteCycle on 10 entries from entry offset $187.
 * HOW: Sets X and Y and calls it.
 * Entry: M 8-bit, D = $0000. Exit: as TilemapPaletteCycle, with X 16-bit;
 * when skipped, X = $187 and Y = 10.
 */
void TilemapPaletteCycle187(uint8_t bank, uint16_t *a)
{
    uint16_t x = 0x0187, y = 0x000A;

    TilemapPaletteCycle(bank, &x, &y, a);
}

void abi_TilemapPaletteCycle187(void)
{
    cpu.x = 0x0187;
    cpu.y = 0x000A;
    cpu.p &= (uint8_t)~P_X;
    abi_TilemapPaletteCycle();
}

/* Fill a row with palette `bits`, then give the entry at `mark` (an
   attribute byte address) palette `mark_bits`. */
static uint8_t row_mark(uint16_t x, uint16_t count, uint8_t bits, uint32_t mark, uint8_t mark_bits,
                        uint16_t *xp, uint16_t *yp, uint16_t *a)
{
    uint8_t v;

    cyc(126);
    wr8(0x0000F1, bits);
    *xp = x;
    *yp = count;
    TilemapPaletteFill(xp, yp, a);
    cyc(32);
    v = (uint8_t)((rd8(mark) & 0xE3) | mark_bits);
    wr8(mark, v);
    *a = (uint16_t)((*a & 0xFF00) | v);
    return v;
}

static void abi_row_mark(uint16_t x, uint16_t count, uint8_t bits, uint32_t mark, uint8_t mark_bits)
{
    uint8_t v = row_mark(x, count, bits, mark, mark_bits, &cpu.x, &cpu.y, &cpu.a);

    cpu.p = (uint8_t)(nz8(cpu.p, v) & ~P_X);
}

/*
 * @routine 07:C2B7 TilemapRow87Palette5
 * WHAT IT DOES: Gives 9 tilemap entries from offset $87 palette 5, then
 * the fourth of them ($7F:9464) palette 4.
 * HOW: TilemapPaletteFill with $F1 = $14, then one masked store.
 * Entry: M 8-bit, D = $0000. Exit: A low byte = the marked attribute byte;
 * N, Z from it; X = $0099; Y = 0; X 16-bit.
 */
void TilemapRow87Palette5(void)
{
    uint16_t x, y, a = 0;

    row_mark(0x0087, 0x0009, 0x14, 0x7F9464, 0x10, &x, &y, &a);
}

void abi_TilemapRow87Palette5(void)
{
    abi_row_mark(0x0087, 0x0009, 0x14, 0x7F9464, 0x10);
}

/*
 * @routine 07:C2D3 TilemapRow107Palette5
 * WHAT IT DOES: Gives 13 tilemap entries from offset $107 palette 5, then
 * the seventh of them ($7F:94EA) palette 4.
 * HOW: TilemapPaletteFill with $F1 = $14, then one masked store.
 * Entry: M 8-bit, D = $0000. Exit: A low byte = the marked attribute byte;
 * N, Z from it; X = $0121; Y = 0; X 16-bit.
 */
void TilemapRow107Palette5(void)
{
    uint16_t x, y, a = 0;

    row_mark(0x0107, 0x000D, 0x14, 0x7F94EA, 0x10, &x, &y, &a);
}

void abi_TilemapRow107Palette5(void)
{
    abi_row_mark(0x0107, 0x000D, 0x14, 0x7F94EA, 0x10);
}

/*
 * @routine 07:C2EF TilemapRow187Palette5
 * WHAT IT DOES: Gives 10 tilemap entries from offset $187 palette 5, then
 * the seventh of them ($7F:956A) palette 4.
 * HOW: As TilemapRow107Palette5.
 * Entry: M 8-bit, D = $0000. Exit: A low byte = the marked attribute byte;
 * N, Z from it; X = $019B; Y = 0; X 16-bit.
 */
void TilemapRow187Palette5(void)
{
    uint16_t x, y, a = 0;

    row_mark(0x0187, 0x000A, 0x14, 0x7F956A, 0x10, &x, &y, &a);
}

void abi_TilemapRow187Palette5(void)
{
    abi_row_mark(0x0187, 0x000A, 0x14, 0x7F956A, 0x10);
}

/*
 * @routine 07:CE97 TilemapRow187Palette4
 * WHAT IT DOES: Gives 9 tilemap entries from offset $187 palette 4, then
 * the fourth of them ($7F:9564) palette 0.
 * HOW: TilemapPaletteFill with $F1 = $10, then one masked store.
 * Entry: M 8-bit, X 16-bit, D = $0000. Exit: A low byte = the marked
 * attribute byte; N, Z from it; X = $0199; Y = 0.
 */
void TilemapRow187Palette4(void)
{
    uint16_t x, y, a = 0;

    row_mark(0x0187, 0x0009, 0x10, 0x7F9564, 0x00, &x, &y, &a);
}

void abi_TilemapRow187Palette4(void)
{
    abi_row_mark(0x0187, 0x0009, 0x10, 0x7F9564, 0x00);
}

/*
 * @routine 07:CEAF TilemapRow207Palette4
 * WHAT IT DOES: Gives 13 tilemap entries from offset $207 palette 4, then
 * the seventh of them ($7F:95EA) palette 0.
 * HOW: As TilemapRow187Palette4.
 * Entry: M 8-bit, X 16-bit, D = $0000. Exit: A low byte = the marked
 * attribute byte; N, Z from it; X = $0221; Y = 0.
 */
void TilemapRow207Palette4(void)
{
    uint16_t x, y, a = 0;

    row_mark(0x0207, 0x000D, 0x10, 0x7F95EA, 0x00, &x, &y, &a);
}

void abi_TilemapRow207Palette4(void)
{
    abi_row_mark(0x0207, 0x000D, 0x10, 0x7F95EA, 0x00);
}

/*
 * @routine 07:CEC7 TilemapRow287Palette4
 * WHAT IT DOES: Gives 10 tilemap entries from offset $287 palette 4, then
 * the seventh of them ($7F:966A) palette 0.
 * HOW: As TilemapRow187Palette4.
 * Entry: M 8-bit, X 16-bit, D = $0000. Exit: A low byte = the marked
 * attribute byte; N, Z from it; X = $029B; Y = 0.
 */
void TilemapRow287Palette4(void)
{
    uint16_t x, y, a = 0;

    row_mark(0x0287, 0x000A, 0x10, 0x7F966A, 0x00, &x, &y, &a);
}

void abi_TilemapRow287Palette4(void)
{
    abi_row_mark(0x0287, 0x000A, 0x10, 0x7F966A, 0x00);
}

/*
 * @routine 07:CFA3 DigitTiles1184
 * WHAT IT DOES: Writes the three digits at $1184-$1186 as tilemap words
 * at $7F:9481-$7F:9486: tile $CA + digit, priority bit set ($2000).
 * HOW: Each digit read 8-bit, then a 16-bit add and OR.
 * Entry: any widths. Exit: A = $20CA + the last digit; M 8-bit; N, Z from
 * the OR; C, V from the add.
 */
void abi_DigitTiles1184(void)
{
    uint8_t v;
    unsigned i;

    cyc(72);
    for (i = 0; i < 3; i++) {
        cpu.p |= P_M;
        cyc(54);
        v = rd8(0x001184 + i);
        cpu.p &= (uint8_t)~(P_M | P_C);
        cpu.a = op_adc16(v, 0x00CA, &cpu.p);
        cpu.a |= 0x2000;
        cpu.p = nz16(cpu.p, cpu.a);
        cyc(140);
        wr16(0x7F9481 + i * 2, cpu.a);
    }
    cyc(22);
    cpu.p |= P_M;
}

/*
 * @routine 07:D029 TimesKeepAndAdvance
 * WHAT IT DOES: Keeps $0EE7-$0EE9 at $0EEA-$0EEC, clears $0E3F, moves the
 * stage $0D59 on, sets $0D23 = 2, clears $19DF and starts the window
 * closing ($19D8 = 2).
 * HOW: 8-bit stores through the data bank.
 * Entry: any widths. Exit: A low byte = 2; M 8-bit; N, Z clear.
 */
void abi_TimesKeepAndAdvance(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint8_t v;
    unsigned i;

    cpu.p |= P_M;
    cyc(22);
    for (i = 0; i < 3; i++)
        { cyc(24); wr8(b | (uint16_t)(0x0EEA + i), cyc_then(24, rd8(b | (uint16_t)(0x0EE7 + i)))); }
    cyc(40);
    wr8(b | 0x0E3F, 0x00);
    cyc(24);
    v = (uint8_t)(rd8(b | 0x0D59) + 1);
    cyc(6);
    wr8(b | 0x0D59, v);
    cpu.p = nz8(cpu.p, v);
    cyc(40);
    wr8(b | 0x0D23, 0x02);
    cyc(46);
    wr8(b | 0x19DF, 0x00);
    cyc(40);
    wr8(b | 0x19D8, 0x02);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x02);
    cpu.p = nz8(cpu.p, 0x02);
}

/*
 * @routine 07:C30B WindowClosedDigitStep
 * WHAT IT DOES: When the split window is in state 5 ($19D8): with the
 * override $0D4C set, clears it, sets $0D48 = 2 and goes on at $07:EC6B;
 * without it at $07:EBDD (decoded from the ROM bytes and not reached in the
 * traces so far). Otherwise nothing (not reached either).
 * HOW: 8-bit tests through the data bank; the two continuations are jumps
 * (jump_routine: RaceRestart, RaceEndLeave); RaceRestart never returns.
 * Entry: any widths. Exit: M 8-bit; as the continuation leaves it.
 */
void abi_WindowClosedDigitStep(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;

    op_sep(P_M);
    cyc(46);
    op_lda8(b | 0x19D8);
    cpu.p = (uint8_t)((nz8(cpu.p, (uint8_t)((uint8_t)cpu.a - 0x05)) & ~P_C) | ((uint8_t)cpu.a >= 0x05 ? P_C : (cyc(122), 0)));
    if ((uint8_t)cpu.a != 0x05)
        return;
    cyc(56);
    op_lda8(b | 0x0D4C);
    if (!(uint8_t)cpu.a) {
        cyc(46);
        jump_routine(0x07EBDD);
        return;
    }
    cyc(40);
    wr8(b | 0x0D4C, 0x00);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x02);
    cpu.p = nz8(cpu.p, 0x02);
    cyc(40);
    wr8(b | 0x0D48, 0x02);
    cyc(24);
    jump_routine(0x07EC6B);
}

void abi_WindowSlideUpSound(void);
void abi_OamHide36And96(void);
void abi_GsuCF42To3000(void);

/* SEP #$20 / REP #$20 around: BG2's scroll to (-24, -32) for the
   championship screens ($0D92 = $FFE8, $0D8E = $FFE0). */
static void bg2_scroll_corner(uint32_t b)
{
    op_rep(P_M);
    op_lda16_imm(0xFFE0);
    cyc(10);
    wr16(b | 0x0D8E, 0xFFE0);
    op_lda16_imm(0xFFE8);
    cyc(10);
    wr16(b | 0x0D92, 0xFFE8);
}

/* INC $0D59 (8-bit). */
static void stage_next(uint32_t b)
{
    cyc(24);
    uint8_t v = (uint8_t)(rd8(b | 0x0D59) + 1);

    cyc(6);
    wr8(b | 0x0D59, v);
    cpu.p = nz8(cpu.p, v);
}

/*
 * @routine 07:BA15 ChampionshipScreenStep
 * WHAT IT DOES: One frame of the championship screen's set-up by its stage
 * $0D59 (the table at $07:BA26), or, with the HUD mode $0D2B 0,
 * WindowClosedDigitStep. The stages here: 0 (BA71) clears $0E5F, $19DF and
 * $10A2, sets $0D22 = 6, $0D23 = 0, the slide-up sound and hides sprites
 * 36 and 96 (OamHide36And96); 1 (BA56) waits for the split window's
 * state 5, then BG2's scroll (-24, -32); 4 (BA9A) has the Super FX picture
 * made (GsuCF42To3000 with $FB99, $1F) and copied from $70:3000 to
 * $7F:93D7 ($440 bytes), the digit tiles (DigitTiles1184), bit 1 of $0D45
 * set and bits 2-3 cleared, and the palette buffer from $1F:8600; 7 (BA3A)
 * BG2's scroll and map ($5C), $19DF cleared; 8 (BA8F) waits for the split
 * window to close. The others are elsewhere in bank $07 ($CC83, $CC9B,
 * $CF6F, TimesKeepAndAdvance, $CCBF). Each finished stage moves $0D59 on.
 * HOW: JMP (table,X); the stages outside this routine are reached through
 * jump_routine (one leaves through the caller's frame). The RTS after WindowClosedDigitStep runs only when that
 * returns (unit_cpu).
 * Entry: M 8-bit, X 8-bit, D = $0000. Exit: as the stage leaves it.
 */
void abi_ChampionshipScreenStep(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint16_t t;
    uint8_t v;
    int i;

    cyc(24);
    op_lda8(b | 0x0D2B);
    if (!(uint8_t)cpu.a) {
        cyc(62);
        abi_WindowClosedDigitStep();
        return;
    }
    cyc(46);
    op_lda8(b | 0x0D59);
    v = (uint8_t)cpu.a;
    cpu.p = (uint8_t)((cpu.p & ~P_C) | (v >> 7));
    v = (uint8_t)(v << 1);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.x = v;
    cpu.p = nz8(cpu.p, v);
    cyc(74);
    t = rd16(0x070000 | (uint16_t)(0xBA26 + v));
    switch (t) {
    case 0xBA71:
        cyc(24);
        wr8(b | 0x0E5F, 0x00);
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x06);
        cyc(40);
        wr8(b | 0x0D22, 0x06);
        cpu.a = (uint16_t)(cpu.a & 0xFF00);
        cpu.p = nz8(cpu.p, 0);
        cyc(40);
        wr8(b | 0x0D23, 0x00);
        cyc(24);
        wr8(b | 0x19DF, 0x00);
        cyc(24);
        wr8(b | 0x10A2, 0x00);
        (cyc(62), abi_WindowSlideUpSound());
        stage_next(b);
        abi_OamHide36And96();
        break;
    case 0xBA56:
        op_sep(P_M);
        cyc(46);
        op_lda8(b | 0x19D8);
        v = (uint8_t)cpu.a;
        cpu.p = (uint8_t)((nz8(cpu.p, (uint8_t)(v - 0x05)) & ~P_C) | (v >= 0x05 ? (cyc(32), P_C) : (cyc(38), 0)));
        if (v != 0x05)
            break;
        stage_next(b);
        bg2_scroll_corner(b);
        break;
    case 0xBA3A:
        bg2_scroll_corner(b);
        op_sep(P_M);
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x5C);
        cpu.p = nz8(cpu.p, 0x5C);
        cyc(62);
        wr8(b | 0x2108, 0x5C);
        stage_next(b);
        cyc(24);
        wr8(b | 0x19DF, 0x00);
        break;
    case 0xBA8F:
        op_sep(P_M);
        cyc(46);
        op_lda8(b | 0x19D8);
        if (!(uint8_t)cpu.a)
            { cyc(16); stage_next(b); } else { cyc(22); }
        break;
    case 0xBA9A: {
        uint8_t p1, db1;

        op_sep(P_M);
        cpu.a = (uint16_t)(cpu.a & 0xFF00);
        cpu.p = nz8(cpu.p, 0);
        cyc(62);
        wr8(b | 0x0DD4, 0x00);
        cyc(24);
        wr8(b | 0x0DD5, 0x00);
        op_rep(P_M | P_X);
        cpu.x = 0x001F;
        op_lda16_imm(0xFB99);
        abi_GsuCF42To3000();
        db1 = cpu.db;
        p1 = cpu.p;
        op_rep(P_M | P_X);
        cyc(220);
        for (i = 0; i < 0x440; i++)
            { cyc(36); wr8(0x7F93D7 + (uint32_t)i, rd8(0x703000 + (uint32_t)i)); }
        cpu.a = 0xFFFF;
        cpu.x = 0x3440;
        cpu.y = 0x9817;
        op_plp(p1);
        cpu.db = db1;
        cpu.p = nz8(cpu.p, db1);
        abi_DigitTiles1184();
        op_sep(P_M);
        cyc(64);
        stage_next((uint32_t)cpu.db << 16);
        b = (uint32_t)cpu.db << 16;
        cyc(24);
        op_lda8(b | 0x0D45);
        v = (uint8_t)((cpu.a | 0x02) & 0xF3);
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
        cpu.p = nz8(cpu.p, v);
        cyc(56);
        wr8(b | 0x0D45, v);
        op_rep(P_M | P_X);
        cpu.x = 0x01FE;
        do {
            cyc(130);
            op_lda16(0x1F8600 + (uint32_t)cpu.x);
            wr16(0x7EFD65 + (uint32_t)cpu.x, cpu.a);
            cpu.x = (uint16_t)(cpu.x - 2);
            cpu.p = nz16(cpu.p, cpu.x);
        } while (!(cpu.x & 0x8000));
        cyc(44);
        break;
    }
    case 0xD029:
        abi_TimesKeepAndAdvance();
        break;
    default:
        jump_routine(0x070000 | t);
        break;
    }
cyc(42);
    }

void abi_OverlaySpritesStep(void);

/*
 * @routine 07:B9EE ChampionshipScreenLoop
 * WHAT IT DOES: Runs the championship screen a frame at a time: waits for
 * the vertical blank's flag $0D52, then ChampionshipScreenStep and
 * OverlaySpritesStep, and clears both pads' presses and held buttons;
 * until the fade level's high byte $0D61 goes negative.
 * HOW: 8-bit, the pads 16-bit. A stage of ChampionshipScreenStep can leave
 * through this routine's frame.
 * Entry: D = $0000. Exit: M 8-bit, X 8-bit; A low byte = $0D61.
 */
void abi_ChampionshipScreenLoop(void)
{
    uint32_t b;

    cyc(8);
    do {
        op_sep(P_M | P_X);
        b = (uint32_t)cpu.db << 16;
        do
            { cyc(46); op_lda8(b | 0x0D52); }
        while (!(uint8_t)cpu.a);
        cyc(40);
        wr8(b | 0x0D52, 0x00);
        (cyc(46), abi_ChampionshipScreenStep());
        (cyc(62), abi_OverlaySpritesStep());
        b = (uint32_t)cpu.db << 16;
        op_rep(P_M);
        cyc(82);
        wr16(b | 0x0D1A, 0x0000);
        cyc(24);
        wr16(b | 0x0D1C, 0x0000);
        cyc(24);
        wr16(b | 0x0309, 0x0000);
        cyc(24);
        wr16(b | 0x030D, 0x0000);
        op_sep(P_M);
        cyc(46);
        op_lda8(b | 0x0D61);
    } while (!(cpu.a & 0x80));
cyc(58);
    }
