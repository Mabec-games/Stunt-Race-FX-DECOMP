/*
 * tilemap.c: BG tilemap buffers in work RAM and their uploads.
 */
#include "machine.h"
#include "hw.h"

/* Store a word count times at bank $7F, $3497 + X, stepping 2. */
static uint16_t fill_7f3497(uint16_t value, uint16_t x, uint16_t count)
{
    do {
        wr16(0x7F3497 + x, value);
        x = (uint16_t)(x + 2);
    } while (--count);
    return x;
}

/*
 * @routine 03:ED7F TilemapFill7F3497
 * WHAT IT DOES: Fills Y words of the tilemap buffer at $7F:3497 + X with A.
 * HOW: 16-bit stores, X stepping by 2.
 * Entry: M 16-bit, X 16-bit; Y = count (0 means 65536). Exit: X past the
 * last word; Y = 0; Z set, N clear.
 */
void TilemapFill7F3497(uint16_t value, uint16_t x, uint16_t count)
{
    fill_7f3497(value, x, count);
}

void abi_TilemapFill7F3497(void)
{
    cpu.x = fill_7f3497(cpu.a, cpu.x, cpu.y);
    cpu.y = 0;
    cpu.p = (uint8_t)((cpu.p & ~P_N) | P_Z);
}

/*
 * @routine 0B:FB1C TilemapFill7F3497B
 * WHAT IT DOES: The same fill as TilemapFill7F3497, in bank $0B.
 * HOW: As TilemapFill7F3497.
 * Entry, exit: as TilemapFill7F3497.
 */
void TilemapFill7F3497B(uint16_t value, uint16_t x, uint16_t count)
{
    fill_7f3497(value, x, count);
}

void abi_TilemapFill7F3497B(void)
{
    cpu.x = fill_7f3497(cpu.a, cpu.x, cpu.y);
    cpu.y = 0;
    cpu.p = (uint8_t)((cpu.p & ~P_N) | P_Z);
}

/*
 * @routine 03:EDC6 TilemapCopy16To7F39D7
 * WHAT IT DOES: Copies 16 words from address A in the data bank to $7F:39D7.
 * HOW: A word loop counted in direct page $F1.
 * Entry: M 16-bit, X 16-bit, D = $0000; A = source. Exit: A = the last word;
 * X = $20; Y = source + $20; $F1 = 0; Z set, N clear.
 */
uint16_t TilemapCopy16To7F39D7(uint8_t bank, uint16_t src)
{
    uint16_t w = 0;
    unsigned i;

    for (i = 0; i < 16; i++) {
        w = rd16((uint32_t)bank << 16 | (uint16_t)(src + 2 * i));
        wr16(0x7F39D7 + 2 * i, w);
    }
    wr16(0x0000F1, 0);
    return w;
}

void abi_TilemapCopy16To7F39D7(void)
{
    uint16_t src = cpu.a;

    cpu.a = TilemapCopy16To7F39D7(cpu.db, src);
    cpu.y = (uint16_t)(src + 0x20);
    cpu.x = 0x0020;
    cpu.p = (uint8_t)((cpu.p & ~P_N) | P_Z);
}

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
    uint16_t f1 = rd16(0x0000F1);
    uint32_t s = (uint32_t)a + f1;
    uint16_t v = 0;
    unsigned i;

    *p = (uint8_t)((*p & ~(P_C | P_V)) | (s > 0xFFFF ? P_C : 0)
                   | ((~(a ^ f1) & (a ^ (uint16_t)s) & 0x8000) ? P_V : 0));
    *x = (uint16_t)s;
    wr16(0x0000F3, 0x0020);
    for (i = 0; i < 32; i++) {
        v = (uint16_t)(rd16(0x703000 + *x) | 0x2000);
        wr16((uint32_t)bank << 16 | (uint16_t)(0x93D7 + *y), v);
        *x = (uint16_t)(*x + 2);
        *y = (uint16_t)(*y + 2);
        wr16(0x0000F3, (uint16_t)(0x1F - i));
    }
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

    wr16(0x0000F1, start);
    wr16(0x0000F3, 0x0010);
    for (r = 0; r < 16; r++) {
        a = rd16(0x0000F1);
        for (c = 0; c < 32; c++) {
            uint32_t s;
            wr16(0x002118, a);
            s = (uint32_t)a + 0x10;
            *p = (uint8_t)((*p & ~(P_C | P_V)) | (s > 0xFFFF ? P_C : 0)
                           | ((~(a ^ 0x10) & (a ^ (uint16_t)s) & 0x8000) ? P_V : 0));
            a = (uint16_t)s;
        }
        wr16(0x0000F1, (uint16_t)(rd16(0x0000F1) + 1));
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
    uint16_t a = (uint16_t)(tile << 5), f1 = rd16(0x0000F1);
    uint32_t s = (uint32_t)a + f1;

    *p = (uint8_t)((*p & ~(P_C | P_V)) | (s > 0xFFFF ? P_C : 0)
                   | ((~(a ^ f1) & (a ^ (uint16_t)s) & 0x8000) ? P_V : 0));
    wr16(A1T0L, (uint16_t)s);
    wr16(VMADDL, vram);
    wr16(DMAP0, 0x1801);
    wr16(DAS0L, 0x0020);
    wr8(VMAIN, 0x80);
    wr8(A1B0, 0x70);
    wr8(MDMAEN, 0x01);
}

void abi_VramDmaTileFromGsuRam(void)
{
    uint8_t p = cpu.p;

    VramDmaTileFromGsuRam(cpu.a, cpu.x, &p);
    cpu.a = 0x0001;
    cpu.p = (uint8_t)(p & ~(P_N | P_Z | P_M));
}
