/*
 * palette.c: colours the raster interrupt writes straight to CGRAM each
 * frame, and the colour cycling it steps. The palette buffer lives in work
 * RAM from $7E:FD65 (512 bytes); a few colours are kept apart at $7E:FF27.
 */
#include "machine.h"
#include "hw.h"

void CgramDma(uint16_t src, uint8_t colour, uint16_t size);

#define PALETTE_ANIMATE 0x000E5Fu  /* nonzero: the animated colours are in use */

/* Write n bytes from src to CGRAM starting at colour c; return the last. */
static uint8_t cgram_put(uint8_t c, uint32_t src, unsigned n)
{
    uint8_t v = 0;
    unsigned i;

    wr8(CGADD, c);
    for (i = 0; i < n; i++) {
        v = rd8(src + i);
        wr8(CGDATA, v);
    }
    return v;
}

/* Colours $E1-$E3 when animating, else just colour 0. */
static uint8_t raster_colours(void)
{
    if (rd8(PALETTE_ANIMATE) == 0)
        return cgram_put(0x00, 0x7EFD65, 2);
    return cgram_put(0xE1, 0x7EFF27, 6);
}

/*
 * @routine 7E:B755 RasterColours
 * WHAT IT DOES: Writes the animated colours $E1-$E3 when they are in use,
 * otherwise colour 0, from the palette buffer to CGRAM.
 * HOW: With $0E5F set, six bytes from $7E:FF27 at colour $E1; else two bytes
 * from $7E:FD65 at colour 0.
 * Entry: any widths. Exit: A low byte = the last byte written; N, Z from it;
 * M and X 8-bit.
 */
void RasterColours(void)
{
    raster_colours();
}

void abi_RasterColours(void)
{
    uint8_t v = raster_colours();

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.x &= 0x00FF;
    cpu.y &= 0x00FF;
    cpu.p = (uint8_t)(nz8(cpu.p, v) | P_M | P_X);
}

/*
 * @routine 7E:B72B RasterPaletteUpload
 * WHAT IT DOES: When the animated colours are in use, copies colours 0-$2F
 * and $B0-$BF from the palette buffer to CGRAM and then writes the animated
 * colours (RasterColours); otherwise writes colour 0 only.
 * HOW: Two CgramDma transfers ($60 bytes from $7E:FD65 at colour 0, $20
 * bytes from $7E:FEC5 at colour $B0), then falls into RasterColours.
 * Entry: M 8-bit, X 8-bit. Exit: A low byte = the last byte written; N, Z
 * from it; when animating X = $B0, Y = $20, M and X 8-bit.
 */
static uint8_t raster_palette_upload(int *animating)
{
    *animating = 0;
    if (rd8(PALETTE_ANIMATE) == 0)
        return cgram_put(0x00, 0x7EFD65, 2);
    CgramDma(0xFD65, 0x00, 0x0060);
    if (rd8(PALETTE_ANIMATE) == 0)
        return cgram_put(0x00, 0x7EFD65, 2);
    *animating = 1;
    CgramDma(0xFEC5, 0xB0, 0x0020);
    return raster_colours();
}

void RasterPaletteUpload(void)
{
    int animating;

    raster_palette_upload(&animating);
}

void abi_RasterPaletteUpload(void)
{
    int animating;
    uint8_t v = raster_palette_upload(&animating);

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = (uint8_t)(nz8(cpu.p, v) | P_M);
    if (animating) {
        cpu.x = 0xB0;
        cpu.y = 0x20;
        cpu.p |= P_X;
    }
}

/*
 * @routine 7E:B31E RasterColoursCycling
 * WHAT IT DOES: Writes the animated colours $E1-$E3, then two cycling colours
 * for this frame: colours $10-$11 on even frames, $12-$13 on odd ones, from
 * the buffer at $7E:FD85 + X.
 * HOW: Six bytes at colour $E1 from $7E:FF27; then four bytes at colour $10
 * (or $12) from $7E:FD85+X (or $7E:FD89+X).
 * Entry: M 8-bit, X 16-bit. Exit: A low byte = the last byte written; N, Z
 * from it.
 */
uint8_t RasterColoursCycling(uint16_t x)
{
    cgram_put(0xE1, 0x7EFF27, 6);
    if (!(rd8(0x000E3B) & 0x01))
        return cgram_put(0x10, (0x7EFD85 + x) & 0xFFFFFF, 4);
    return cgram_put(0x12, (0x7EFD89 + x) & 0xFFFFFF, 4);
}

void abi_RasterColoursCycling(void)
{
    uint8_t v = RasterColoursCycling(cpu.x);

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = (uint8_t)(nz8(cpu.p, v) | P_M);
}

/*
 * @routine 7E:B39A RasterColoursE4
 * WHAT IT DOES: Writes six colours ($E4-$E9) from the buffer at $7E:FF2D + X.
 * HOW: Twelve bytes to CGRAM from colour $E4.
 * Entry: M 8-bit, X 16-bit. Exit: A low byte = the last byte written; N, Z
 * from it.
 */
uint8_t RasterColoursE4(uint16_t x)
{
    return cgram_put(0xE4, (0x7EFF2D + x) & 0xFFFFFF, 12);
}

void abi_RasterColoursE4(void)
{
    uint8_t v = RasterColoursE4(cpu.x);

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, v);
}

/* One colour-cycling step: the phase counts 0..15; phases 8-15 run the table
   backwards (15 - phase), so the colour swings up and down. Returns the new
   phase; *x the table offset used; tracks C and V as the original does. */
static uint16_t cycle_step(uint32_t phase_addr, uint32_t table, uint32_t dest, uint16_t *x, uint8_t *p)
{
    uint16_t phase = rd16(phase_addr), i = phase, next;

    if (phase >= 8) {
        i = (uint16_t)(0x000F - phase);
        *p = (uint8_t)((*p & ~P_V) | (((0x000F ^ phase) & (0x000F ^ i) & 0x8000) ? P_V : 0));
    }
    *x = (uint16_t)(i << 1);
    wr16(dest, rd16(table + *x));
    next = (uint16_t)(phase + 1);
    if (next >= 0x0010) {
        next = 0;
        *p |= P_C;
    } else {
        *p &= (uint8_t)~P_C;
    }
    wr16(phase_addr, next);
    return next;
}

/*
 * @routine 7E:B63A PaletteCycle
 * WHAT IT DOES: Steps the palette's cycling colours: two on even frames
 * (into $7E:FD85 and $7E:FD87), one on odd frames (into $7E:FD89).
 * HOW: Each cycling colour has a phase 0-15 ($7E:FD5B, FD5D, FD5F) and a
 * table of eight colours; the phase picks the colour, running back down the
 * table for phases 8-15, then steps and wraps at 16.
 * Entry: M 16-bit. Exit: A = the last phase stored; X = the last table
 * offset; Y = 0; M 16-bit; N, Z from the data bank pulled back on return; C
 * from the phase wrap test.
 */
static uint16_t palette_cycle(uint16_t *x, uint8_t *p)
{
    if (!(rd8(0x000E3B) & 0x01)) {
        cycle_step(0x7EFD5B, 0x7EB71B, 0x7EFD85, x, p);
        return cycle_step(0x7EFD5D, 0x7EB6FB, 0x7EFD87, x, p);
    }
    return cycle_step(0x7EFD5F, 0x7EB70B, 0x7EFD89, x, p);
}

void PaletteCycle(void)
{
    uint16_t x;
    uint8_t p = 0;

    palette_cycle(&x, &p);
}

void abi_PaletteCycle(void)
{
    uint16_t x;
    uint8_t p = cpu.p;

    cpu.a = palette_cycle(&x, &p);
    cpu.x = x;
    cpu.y = 0;
    cpu.p = (uint8_t)(nz8(p, cpu.db) & ~(P_M | P_X));
}

/*
 * @routine 03:EDA5 PaletteLoad16
 * WHAT IT DOES: Loads 16 colours from address A in the data bank into both
 * palette work copies in bank $7F ($7F:35D7, $7F:37D7) and the palette
 * buffer ($7E:FD65), from colour 0.
 * HOW: A word loop counted in direct page $F1; each word is stored to the
 * three copies in that order.
 * Entry: M 16-bit, X 16-bit, D = $0000; A = source. Exit: A = the last
 * colour; X = $20; Y = source + $20; $F1 = 0; Z set, N clear.
 */
uint16_t PaletteLoad16(uint8_t bank, uint16_t src)
{
    uint16_t c = 0;
    unsigned i;

    wr16(0x0000F1, 0x0010);
    for (i = 0; i < 16; i++) {
        c = rd16((uint32_t)bank << 16 | (uint16_t)(src + 2 * i));
        wr16(0x7F35D7 + 2 * i, c);
        wr16(0x7F37D7 + 2 * i, c);
        wr16(0x7EFD65 + 2 * i, c);
        wr16(0x0000F1, (uint16_t)(0x000F - i));
    }
    return c;
}

void abi_PaletteLoad16(void)
{
    uint16_t src = cpu.a;

    cpu.a = PaletteLoad16(cpu.db, src);
    cpu.x = 0x0020;
    cpu.y = (uint16_t)(src + 0x20);
    cpu.p = (uint8_t)((cpu.p & ~P_N) | P_Z);
}

/*
 * @routine 7E:B307 RasterColoursUnless0D22
 * WHAT IT DOES: Unless the byte at $0D22 is set, writes the raster colours
 * from the start of their buffers: RasterColoursE4, then RasterColoursCycling.
 * HOW: X = 0 for both; $0D22 is read through the data bank.
 * Entry: any widths. Exit: skipped: A low byte = $0D22. Written: A low byte
 * as RasterColoursCycling leaves it, X = 0. P unchanged.
 */
uint8_t RasterColoursUnless0D22(uint8_t bank, int *ran)
{
    uint8_t v = rd8((uint32_t)bank << 16 | 0x0D22);

    *ran = !v;
    if (v)
        return v;
    RasterColoursE4(0);
    return RasterColoursCycling(0);
}

void abi_RasterColoursUnless0D22(void)
{
    int ran;
    uint8_t v = RasterColoursUnless0D22(cpu.db, &ran);

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    if (ran)
        cpu.x = 0;
}
