/*
 * palette.c: colours the raster interrupt writes straight to CGRAM each
 * frame, and the colour cycling it steps. The palette buffer lives in work
 * RAM from $7E:FD65 (512 bytes); a few colours are kept apart at $7E:FF27.
 */
#include "machine.h"
#include "hw.h"
#include "cpu_ops.h"

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

/*
 * @routine 04:E1FE PaletteDarkenFour
 * WHAT IT DOES: Stores A at $7F:345C and sets colours 16-19 of the third
 * palette copy ($7F:39D7) to those of the second ($7F:37D7) minus one.
 * HOW: Four 16-bit decrements, long addressing.
 * Entry: M 16-bit; A = value. Exit: A = the last colour written; N, Z from
 * it.
 */
uint16_t PaletteDarkenFour(uint16_t v)
{
    unsigned i;

    wr16(0x7F345C, v);
    for (i = 0; i < 4; i++) {
        v = (uint16_t)(rd16(0x7F37F7 + 2 * i) - 1);
        wr16(0x7F39F7 + 2 * i, v);
    }
    return v;
}

void abi_PaletteDarkenFour(void)
{
    cpu.a = PaletteDarkenFour(cpu.a);
    cpu.p = nz16(cpu.p, cpu.a);
}

/*
 * @routine 03:EDC6 PaletteCopy16To7F39D7
 * WHAT IT DOES: Copies 16 colours from address A in the data bank to the
 * third palette copy at $7F:39D7.
 * HOW: A word loop counted in direct page $F1.
 * Entry: M 16-bit, X 16-bit, D = $0000; A = source. Exit: A = the last word;
 * X = $20; Y = source + $20; $F1 = 0; Z set, N clear.
 */
uint16_t PaletteCopy16To7F39D7(uint8_t bank, uint16_t src)
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

void abi_PaletteCopy16To7F39D7(void)
{
    uint16_t src = cpu.a;

    cpu.a = PaletteCopy16To7F39D7(cpu.db, src);
    cpu.y = (uint16_t)(src + 0x20);
    cpu.x = 0x0020;
    cpu.p = (uint8_t)((cpu.p & ~P_N) | P_Z);
}

/*
 * @routine 03:EDF4 PaletteCourseLoad
 * WHAT IT DOES: Loads the course's palettes: colours 16-255 of the first two
 * work copies and the palette buffer from the course palette, colours 16-255
 * of the third copy from its own palette, colours 0-15 from the course block
 * (PaletteLoad16) and the third copy's 0-15 from the second colour block
 * (PaletteCopy16To7F39D7). Then, colour by colour from 255 down, the third
 * copy is matched to the second where the bit table at $03:EF1F has the
 * colour's bit set and kept one step off it where clear (an equal colour
 * goes one down, or one up when its red is 0). Sets the six blended colours
 * at $0E21-$0E2B; with $10DC set, clears colours $B0-$BF of all four copies.
 * HOW: The pointer tables ($03:F649, $03:F69D, $03:F6F1, $03:F745) give each
 * source's bank, which becomes the data bank while it is read. The table's
 * word for 16 colours is shifted right by the colour's place in it, counted
 * in direct page $F1. The $10DC case is not reached in the traces so far.
 * Entry: M 16-bit, X 16-bit, D = $0000; DB = the bit table's bank ($03).
 * Exit: A = $10DC's word (0); X = $FFFE; Y = 0; $F1 = 0; M 16-bit; Z set, N
 * clear; C from the last compare of colour 0; V from the course index add;
 * DB unchanged. (With $10DC set: A low byte 0, X = $20, M 8-bit, Z, C set.)
 */
uint16_t PaletteCourseLoad(uint8_t bank, uint16_t *xo, uint16_t *yo, uint8_t *p)
{
    uint32_t b = (uint32_t)bank << 16, src;
    uint16_t c = rd16(b | 0x0E3D), i, x, y, w, d, s;
    uint8_t blk;
    unsigned k;

    s = (uint16_t)((uint16_t)(c << 1) + rd16(b | 0x0E3D));
    *p = (uint8_t)((*p & ~P_V) | ((~((uint16_t)(c << 1) ^ c) & ((uint16_t)(c << 1) ^ s) & 0x8000) ? P_V : 0));
    i = (uint16_t)(s & 0x00FF);
    blk = rd8(0x03F64B + i);
    y = rd16(0x03F649 + i);
    for (x = 0x0020; x < 0x0200; x = (uint16_t)(x + 2)) {
        src = ((uint32_t)blk << 16) + 0x0020 + y;
        w = rd16(src & 0xFFFFFF);
        wr16(0x7F35D7 + x, w);
        wr16(0x7F37D7 + x, w);
        wr16(0x7EFD65 + x, w);
        y = (uint16_t)(y + 2);
    }
    blk = rd8(0x03F69F + i);
    y = rd16(0x03F69D + i);
    for (x = 0x0020; x < 0x0200; x = (uint16_t)(x + 2)) {
        src = ((uint32_t)blk << 16) + 0x0020 + y;
        wr16(0x7F39D7 + x, rd16(src & 0xFFFFFF));
        y = (uint16_t)(y + 2);
    }
    blk = rd8(0x03F6F3 + i);
    PaletteLoad16(blk, rd16(0x03F6F1 + i));
    blk = rd8(0x03F747 + i);
    PaletteCopy16To7F39D7(blk, rd16(0x03F745 + i));
    x = 0x01FE;
    for (;;) {
        uint16_t n = (uint16_t)((x >> 1) & 0x000F), bits;

        y = (uint16_t)((x >> 4) & 0x001E);
        wr16(0x0000F1, n);
        bits = rd16(b | (uint16_t)(0xEF1F + y));
        while (n) {
            bits >>= 1;
            n = (uint16_t)(rd16(0x0000F1) - 1);
            wr16(0x0000F1, n);
        }
        w = rd16(0x7F37D7 + x);
        d = rd16(0x7F39D7 + x);
        *p = (uint8_t)((*p & ~P_C) | (w >= d ? P_C : 0));
        if (!(bits & 1)) {
            if (w == d)
                wr16(0x7F39D7 + x, (uint16_t)(w & 0x001F ? w - 1 : w + 1));
        } else if (w != d) {
            wr16(0x7F39D7 + x, w);
        }
        if (!x)
            break;
        x = (uint16_t)(x - 2);
    }
    wr16(b | 0x0E21, 0x45CC);
    wr16(b | 0x0E23, 0x2528);
    wr16(b | 0x0E25, 0x1CE5);
    wr16(b | 0x0E27, 0x739A);
    wr16(b | 0x0E29, 0x1CE5);
    wr16(b | 0x0E2B, 0x2528);
    w = rd16(b | 0x10DC);
    if (!w) {
        *xo = 0xFFFE;
        *yo = y;
        *p = (uint8_t)((nz16(*p, 0) & ~(P_M | P_X)));
        return 0;
    }
    for (k = 0; k < 0x20; k++) {
        wr8(0x7EFEC5 + k, 0x00);
        wr8(0x7F3937 + k, 0x00);
        wr8(0x7F3B37 + k, 0x00);
        wr8(0x7F3737 + k, 0x00);
    }
    *xo = 0x0020;
    *yo = y;
    *p = (uint8_t)((nz8(*p, 0) | P_M | P_C) & ~P_X);
    return (uint16_t)(w & 0xFF00);
}

void abi_PaletteCourseLoad(void)
{
    cpu.a = PaletteCourseLoad(cpu.db, &cpu.x, &cpu.y, &cpu.p);
}

uint16_t PpuMultiply(uint8_t v);

typedef struct {
    uint16_t a;
    uint8_t p;
} BlendEnd;

/* The body of ColourBlend; p carries the flags it leaves. */
static BlendEnd colour_blend(uint8_t bank, uint8_t p)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t from = rd16(0x0000F1), to = rd16(0x0000F3), r, g, bl, m, s;
    BlendEnd e;

    wr16(b | 0x344E, from & 0x1F);
    wr16(b | 0x3452, (from >> 5) & 0x1F);
    wr16(b | 0x3456, (from >> 10) & 0x1F);
    wr16(0x0000F1, (uint16_t)(from >> 15));
    wr16(b | 0x3450, to & 0x1F);
    wr16(b | 0x3454, (to >> 5) & 0x1F);
    wr16(b | 0x3458, (to >> 10) & 0x1F);
    wr16(0x0000F3, (uint16_t)(to >> 15));
    r = (uint16_t)((PpuMultiply((uint8_t)(rd16(b | 0x3450) - rd16(b | 0x344E))) + rd16(b | 0x344E)) & 0x1F);
    wr16(b | 0x344E, r);
    g = (uint16_t)(PpuMultiply((uint8_t)(rd16(b | 0x3454) - rd16(b | 0x3452))) + rd16(b | 0x3452));
    wr16(b | 0x3452, g);
    m = PpuMultiply((uint8_t)(rd16(b | 0x3458) - rd16(b | 0x3456)));
    bl = rd16(b | 0x3456);
    s = (uint16_t)(m + bl);
    p = (uint8_t)(p & ~(P_V | P_C));
    if (~(m ^ bl) & (m ^ s) & 0x8000)
        p |= P_V;
    bl = (uint16_t)((uint16_t)((uint16_t)(s >> 8 | s << 8) << 2) & 0x7C00);
    wr16(0x0000F1, bl);
    if (g & 0x0800)
        p |= P_C;
    e.a = (uint16_t)(((g << 5) & 0x03E0) | bl | r);
    e.p = (uint8_t)(nz16(p, e.a) & ~P_M);
    return e;
}

/*
 * @routine 7E:E6E9 ColourBlend
 * WHAT IT DOES: Blends two BGR555 colours, from direct page $F1 toward $F3,
 * by the fraction loaded in the PPU multiplier (factor t * 8 gives t / 32),
 * and returns the blend.
 * HOW: Splits both colours into 5-bit components at $344E-$3458 (from: red,
 * green, blue at $344E, $3452, $3456; to at $3450, $3454, $3458), shifting
 * each colour word right 5 at a time in place; each component becomes from
 * + PpuMultiply(to - from), red masked to 5 bits, and the three are packed
 * back; $F1 is left with the blue field. The components go through the
 * data bank.
 * Entry: M 16-bit, D = $0000; the multiplier's factor set. Exit: A = the
 * blended colour; N, Z from it; C = bit 11 of the green sum; V from the
 * blue add; X, Y unchanged.
 */
uint16_t ColourBlend(uint8_t bank)
{
    return colour_blend(bank, 0).a;
}

void abi_ColourBlend(void)
{
    BlendEnd e = colour_blend(cpu.db, cpu.p);

    cpu.a = e.a;
    cpu.p = e.p;
}

/* Steps a 13-entry cycle position by n, wrapping. */
static uint8_t cycle13(uint8_t v, uint8_t n)
{
    v = (uint8_t)(v + n);
    return v < 0x0D ? v : (n == 1 ? 0 : (uint8_t)(v - 0x0D));
}

typedef struct {
    uint16_t a, x;
    uint8_t p;
} KeptEnd;

/* The body of PaletteKeptColoursStep, with everything its exit leaves. */
static KeptEnd palette_kept_colours_step(uint8_t bank, uint16_t a, uint16_t x, uint16_t y, uint8_t p)
{
    uint32_t b = (uint32_t)bank << 16;
    uint8_t t, step = 0, v, d;
    unsigned s;
    KeptEnd e;
    enum { NONE, SKIP, ONE, TWO, ODD } go = NONE;

    if (!rd8(b | 0x0D53) && rd8(b | (uint16_t)(0x0E05 + y))
        && (a = rd16(b | (uint16_t)(0x0D1E + x)), a & 0x4000)) {
        t = rd8(b | (uint16_t)(0x0E01 + y));
        if (t == 0xFF) {
            go = TWO;
        } else {
            t++;
            wr8(b | (uint16_t)(0x0E01 + y), t);
            go = t >= 0xB4 ? TWO : t >= 0x78 ? ONE : t >= 0x3C ? ODD : SKIP;
        }
    } else {
        t = rd8(b | (uint16_t)(0x0E01 + y));
        if (!t) {
            a = 0x7E63;
            wr16(0x7EFF2B, a);
        } else if (t >= 0x78) {
            wr8(b | (uint16_t)(0x0E01 + y), (uint8_t)(t - 5));
            go = TWO;
        } else if (t >= 0x3C) {
            wr8(b | (uint16_t)(0x0E01 + y), (uint8_t)(t - 4));
            go = ONE;
        } else if (t >= 0x1E) {
            wr8(b | (uint16_t)(0x0E01 + y), (uint8_t)(t - 3));
            go = ODD;
        } else {
            wr8(b | (uint16_t)(0x0E01 + y), (uint8_t)(t - 1));
            go = SKIP;
        }
    }
    /* SKIP steps by one on every fourth frame, ODD on every other one. */
    if (go == SKIP)
        go = (rd8(b | 0x0E3B) & 0x03) ? NONE : ONE;
    else if (go == ODD)
        go = (rd8(b | 0x0E3B) & 0x01) ? NONE : ONE;
    if (go == ONE || go == TWO) {
        step = cycle13(rd8(b | (uint16_t)(0x0E03 + y)), go == TWO ? 2 : 1);
        wr8(b | (uint16_t)(0x0E03 + y), step);
        x = (uint8_t)(step << 1);
        a = rd16(0x7EB4E9 + x);
        wr16(0x7EFF2B, a);
    }
    v = rd8(b | (uint16_t)(0x0DFD + y));
    d = rd8(b | (uint16_t)(0x0DFB + y));
    s = (unsigned)v - d;
    p = (uint8_t)(p & ~(P_C | P_V));
    if (s < 0x100)
        p |= P_C;
    if ((v ^ d) & (v ^ (uint8_t)s) & 0x80)
        p |= P_V;
    wr8(b | (uint16_t)(0x0DFD + y), (uint8_t)s);
    if (p & P_C) {
        e.a = (uint16_t)((a & 0xFF00) | (uint8_t)s);
        e.p = (uint8_t)(nz8(p, (uint8_t)s) | P_M);
    } else {
        step = cycle13(rd8(b | (uint16_t)(0x0DFF + y)), 1);
        wr8(b | (uint16_t)(0x0DFF + y), step);
        x = (uint8_t)(step << 1);
        wr16(0x7EFF27, rd16(0x7EB505 + x));
        e.a = rd16(0x7EB521 + x);
        wr16(0x7EFF29, e.a);
        e.p = (uint8_t)(nz16(p, e.a) & ~(P_C | P_M));
    }
    e.x = x;
    return e;
}

/*
 * @routine 7E:B41F PaletteKeptColoursStep
 * WHAT IT DOES: Animates the kept colours at $7E:FF27-$7E:FF2C for player Y
 * (the timer at $0E01 + Y and the two 13-step cycles at $0E03 + Y and $0DFF
 * + Y). While the player's flag at $0E05 + Y is set, $0D53 is clear and
 * bit 14 of $0D1E + X is set, the timer counts up and the third colour's
 * cycle speeds up with it (every fourth frame below $3C, every other frame
 * below $78, every frame below $B4, two steps a frame from there); otherwise
 * the timer counts back down through the same speeds and at 0 the third
 * colour rests at $7E63. The first two colours step together whenever
 * $0DFD + Y, less $0DFB + Y each frame, borrows.
 * HOW: The third colour from the table at $7E:B4E9, the first two from
 * $7E:B505 and $7E:B521, each indexed by its cycle step * 2. Counting down
 * subtracts 5, 4, 3 or 1 by band ($78, $3C, $1E). Work RAM through the data
 * bank.
 * Entry: M 8-bit, X 8-bit; X = offset into $0D1E, Y = player offset.
 * Exit: when the first two colours stepped, A = the second colour, M 16-bit,
 * N, Z from it, C clear; otherwise A low byte = $0DFD + Y, M 8-bit, N, Z
 * from it, C set; V from that subtract; X = the last cycle step * 2 when a
 * step was made, else unchanged; Y unchanged.
 */
uint16_t PaletteKeptColoursStep(uint8_t bank, uint16_t x, uint16_t y)
{
    return palette_kept_colours_step(bank, 0, x, y, P_M | P_X).a;
}

void abi_PaletteKeptColoursStep(void)
{
    KeptEnd e = palette_kept_colours_step(cpu.db, cpu.a, cpu.x, cpu.y, cpu.p);

    cpu.a = e.a;
    cpu.x = e.x;
    cpu.p = e.p;
}

uint16_t PpuMultiplierSetLowByteTimes8(uint8_t v);

/*
 * @routine 7E:E6D1 ColourBlendBy
 * WHAT IT DOES: ColourBlend by the fraction in direct page $F5 (t / 32):
 * blends the colour in $F1 toward the one in $F3.
 * HOW: Loads the multiplier's factor with the low byte of $F5 times 8 (two
 * long writes to M7A, low then high), then falls into ColourBlend.
 * Entry: M 16-bit, D = $0000. Exit: as ColourBlend.
 */
uint16_t ColourBlendBy(uint8_t bank)
{
    PpuMultiplierSetLowByteTimes8(rd8(0x0000F5));
    return ColourBlend(bank);
}

void abi_ColourBlendBy(void)
{
    PpuMultiplierSetLowByteTimes8(rd8(0x0000F5));
    abi_ColourBlend();
}

void abi_ColourBlendBy(void);

/*
 * @routine 7E:E6A2 ColourBlendToTarget
 * WHAT IT DOES: Blends colour A toward the target colour at $3470 by the
 * amount at $3468 and stores the result at $0E21 + X; a negative target
 * leaves the colour as it is (blended toward itself).
 * HOW: Direct page $F1 = A, $F3 = the target (or A), $F5 = $3468, then
 * ColourBlendBy; a long store of the result. $3470 and $3468 go through
 * the data bank.
 * Entry: M 16-bit, X 16-bit, D = $0000; A = colour, X = offset. Exit: as
 * ColourBlend.
 */
uint16_t ColourBlendToTarget(uint8_t bank, uint16_t colour, uint16_t x)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t t = rd16(b | 0x3470), v;

    wr16(0x0000F1, colour);
    wr16(0x0000F3, (t & 0x8000) ? colour : t);
    wr16(0x0000F5, rd16(b | 0x3468));
    v = ColourBlendBy(bank);
    wr16(0x000E21 + x, v);
    return v;
}

void abi_ColourBlendToTarget(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint16_t t = rd16(b | 0x3470);

    wr16(0x0000F1, cpu.a);
    wr16(0x0000F3, (t & 0x8000) ? cpu.a : t);
    wr16(0x0000F5, rd16(b | 0x3468));
    abi_ColourBlendBy();
    wr16(0x000E21 + cpu.x, cpu.a);
}

typedef struct {
    uint16_t a, x, y;
    uint8_t p;
} FlashEnd;

/* The body of PlayerColoursFlash, with everything its exit leaves. */
static FlashEnd player_colours_flash(uint8_t bank, uint8_t v, uint16_t y, uint8_t p)
{
    uint32_t b = (uint32_t)bank << 16;
    uint8_t k, n, c;
    uint16_t src;
    FlashEnd e;

    if (v >= 0x08) {
        unsigned s = (unsigned)v + 0x60;

        p = (uint8_t)(p & ~P_V);
        if (~(v ^ 0x60) & (v ^ s) & 0x80)
            p |= P_V;
        v = s > 0xFF ? 0xFF : (uint8_t)s;
    }
    k = (uint8_t)(v >> 5);
    src = 0x0E21;
    if (!k) {
        wr8(b | (uint16_t)(0x0E1F + y), 0x00);
    } else {
        n = (uint8_t)(8 - k);
        p = (uint8_t)(p & ~P_V);
        wr8(0x0000F9, n);
        wr8(0x0000FA, (uint8_t)(n << 1));
        c = rd8(b | (uint16_t)(0x0E1F + y));
        if (c >= (uint8_t)(n << 1))
            c = 0xFF;
        c = (uint8_t)(c + 1);
        wr8(b | (uint16_t)(0x0E1F + y), c);
        if (c < n)
            src = 0x0E27;
    }
    p = (uint8_t)((p & ~P_C) | ((y & 0x8000) ? P_C : 0));
    e.y = (uint16_t)(y << 1);
    e.x = rd16(b | (uint16_t)(0x0D83 + e.y));
    wr16(0x7E01C8 + e.x, rd16(b | src));
    wr16(0x7E01CA + e.x, rd16(b | (uint16_t)(src + 2)));
    e.a = rd16(b | (uint16_t)(src + 4));
    wr16(0x7E01CC + e.x, e.a);
    e.p = (uint8_t)(nz16(p, e.a) & ~(P_M | P_X));
    return e;
}

/*
 * @routine 7E:B5C1 PlayerColoursFlash
 * WHAT IT DOES: Writes player Y's three colours into the palette at
 * $7E:01C8 + the player's offset ($0D83 + 2Y): the set at $0E21 normally;
 * for a level A of 8 or more, flashing between the sets at $0E27 and $0E21
 * faster the higher the level (on for n calls, off for n, where n = 8 - the
 * top three bits of A + $60, the sum held at $FF; no script has reached the
 * held case yet).
 * HOW: The flash counter is at $0E1F + Y and runs 1 to 2n; n and 2n are
 * kept in direct page $F9 and $FA; counts below n show $0E27. Work RAM
 * through the data bank; the palette with long stores.
 * Entry: M 8-bit, X 16-bit, D = $0000; A = level, Y = player. Exit: A = the
 * third colour written; X = the player's palette offset; Y = 2Y; M 16-bit;
 * N, Z from A; C = bit 15 of Y on entry; V clear after a flash step, from
 * the add of $60 when only that ran.
 */
void PlayerColoursFlash(uint8_t bank, uint8_t level, uint16_t y)
{
    player_colours_flash(bank, level, y, 0);
}

void abi_PlayerColoursFlash(void)
{
    FlashEnd e = player_colours_flash(cpu.db, (uint8_t)cpu.a, cpu.y, cpu.p);

    cpu.a = e.a;
    cpu.x = e.x;
    cpu.y = e.y;
    cpu.p = e.p;
}

/*
 * @routine 7E:B53F StatusColourPick
 * WHAT IT DOES: Picks the colour for item Y and stores it at $7E:FD8B: 0
 * while its timer $0DDA + Y is 0; with flag $0400 (of $0DF0/$0DF2 + Y) and
 * $0E05 + Y set, entry ($0E3B & 3) of the colours at $7E:B5B9; with flag
 * $0010 and the timer at $3C or more, entry ($0E3B & 3) of $7E:B5B1;
 * otherwise, on even frames only, entry (($0E3B >> 1) & 3) of $7E:B5B1
 * (odd frames store nothing).
 * HOW: The tables are read from the code's work RAM copy. The $0400 path
 * with $0E05 clear is not reached in the traces so far.
 * Entry: M 8-bit, X 16-bit; Y = item. Exit: M 16-bit; A = the colour (or
 * $0E3B on an odd frame, X unchanged); X = 0 after a store.
 */
void abi_StatusColourPick(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint16_t flags, t;
    uint8_t v;
    int table = 0;

    v = rd8(b | (uint16_t)(0x0DDA + cpu.y));
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, v);
    if (v == 0) {
        cpu.p &= (uint8_t)~P_M;
        cpu.a = 0x0000;
        cpu.p = nz16(cpu.p, 0);
        goto store;
    }
    v = rd8(b | (uint16_t)(0x0DF2 + cpu.y));
    flags = (uint16_t)(v << 8);
    flags |= rd8(b | (uint16_t)(0x0DF0 + cpu.y));
    cpu.a = flags;
    cpu.p &= (uint8_t)~P_M;
    if (flags & 0x0400) {
        v = rd8(b | (uint16_t)(0x0E05 + cpu.y));
        if (v) {
            cpu.a = flags;
            t = rd16(b | 0x0E3B);
            cpu.x = (uint16_t)((t & 0x0003) << 1);
            cpu.p = (uint8_t)(cpu.p & ~P_C);
            cpu.a = rd16((uint32_t)(0x7EB5B9 + cpu.x));
            cpu.p = nz16(cpu.p, cpu.a);
            goto store;
        }
    }
    if (flags & 0x0010) {
        v = rd8(b | (uint16_t)(0x0DDA + cpu.y));
        cpu.p = (uint8_t)((cpu.p & ~P_C) | (v >= 0x3C ? P_C : 0));
        if (v >= 0x3C) {
            t = rd16(b | 0x0E3B);
            table = 1;
        }
    }
    if (!table) {
        t = rd16(b | 0x0E3B);
        cpu.a = t;
        if (t & 0x0001) {
            cpu.p = nz16(cpu.p, t);
            return;
        }
        cpu.p = (uint8_t)((cpu.p & ~P_C) | (t & 0x0001));
        t = (uint16_t)(t >> 1);
    }
    cpu.x = (uint16_t)((t & 0x0003) << 1);
    cpu.p = (uint8_t)(cpu.p & ~P_C);
    cpu.a = rd16((uint32_t)(0x7EB5B1 + cpu.x));
    cpu.p = nz16(cpu.p, cpu.a);
store:
    cpu.x = 0x0000;
    wr16(0x7EFD8B, cpu.a);
    cpu.p = nz16(cpu.p, 0);
}

void abi_PlayerColoursFlash(void);
void abi_PaletteCycle(void);
void abi_PaletteKeptColoursStep(void);

/*
 * @routine 7E:B3F4 PalettesStep
 * WHAT IT DOES: While $0E5F is set, steps the frame's palette effects for
 * player 0: its colours (PlayerColoursFlash with A = $0DDA), its status
 * colour (StatusColourPick), the cycling colours (PaletteCycle) and the
 * kept colours (PaletteKeptColoursStep).
 * HOW: Y = 0 for each; M 8-bit, X 16-bit, then both 8-bit for the last.
 * Entry: any widths. Exit: P restored; X and Y as the last call left them.
 */
void abi_PalettesStep(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint8_t p0 = cpu.p, v;

    cpu.p = (uint8_t)((cpu.p | P_M) & ~P_X);
    v = rd8(b | 0x0E5F);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    if (v) {
        cpu.y = 0x0000;
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | rd8(b | 0x0DDA));
        abi_PlayerColoursFlash();
        cpu.p = (uint8_t)((cpu.p | P_M) & ~P_X);
        cpu.y = 0x0000;
        abi_StatusColourPick();
        abi_PaletteCycle();
        op_sep(P_M | P_X);
        cpu.y = 0x00;
        cpu.x = 0x00;
        abi_PaletteKeptColoursStep();
    }
    op_plp(p0);
}

/*
 * @routine 7E:E149 FadeValueStep
 * WHAT IT DOES: Unless bit 7 of $0DA5 is set, while bit 5 is set: steps
 * the word $19FE by $1A00 until it reaches $1A02 (then clears bit 5,
 * leaving $19FE as it was) and puts its high byte in $7F:3460.
 * HOW: The data bank is the code's ($7E) and restored; $0DA5 is read
 * long. The step is not reached in the traces so far.
 * Entry: any widths. Exit: P and the data bank restored; A as the last
 * step left it.
 */
void abi_FadeValueStep(void)
{
    uint8_t p0 = cpu.p, db0 = cpu.db, v;
    uint32_t b = 0x7E0000;
    uint16_t w;

    cpu.db = 0x7E;
    cpu.p |= P_M;
    v = (uint8_t)(rd8(0x000DA5) & 0x80);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    if (!v) {
        v = (uint8_t)(rd8(0x000DA5) & 0x20);
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
        if (v) {
            cpu.p &= (uint8_t)~(P_M | P_C);
            w = rd16(b | 0x19FE);
            w = op_adc16(w, rd16(b | 0x1A00), &cpu.p);
            cpu.a = w;
            op_cmp16(rd16(b | 0x1A02));
            if (!(cpu.p & P_Z))
                wr16(b | 0x19FE, w);
            else {
                v = (uint8_t)(rd8(0x000DA5) & 0xDF);
                wr8(0x000DA5, v);
            }
            cpu.p |= P_M;
            v = rd8(b | 0x19FF);
            cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
            wr8(0x7F3460, v);
        }
    }
    cpu.db = db0;
    op_plp(p0);
}

/* One colour part of PaletteFadeStep: the part (mask m) of the colour at
   $0722+Y moved one step toward the same part at $02:D65A+X. */
static uint16_t fade_part(uint32_t b, uint16_t m, uint16_t step)
{
    uint16_t c;

    op_lda16(b | (uint16_t)(0x0722 + cpu.y));
    op_and16(m);
    wr16(b | 0x1F2B, cpu.a);
    op_lda16(0x02D65A + (uint32_t)cpu.x);
    op_and16(m);
    c = rd16(b | 0x1F2B);
    op_cmp16(c);
    if (cpu.a == c)
        return cpu.a;
    if (cpu.p & P_C) {
        op_lda16(b | 0x1F2B);
        if (step == 1) {
            cpu.a++;
            cpu.p = nz16(cpu.p, cpu.a);
        } else {
            op_add16(step);
        }
    } else {
        op_lda16(b | 0x1F2B);
        if (step == 1) {
            cpu.a--;
            cpu.p = nz16(cpu.p, cpu.a);
        } else {
            op_sub16(step);
        }
    }
    return cpu.a;
}

/*
 * @routine 02:D55F PaletteFadeStep
 * WHAT IT DOES: While the fade count $0CFA is nonzero, decrements it and
 * moves $1167 colours of the buffer at $0722 + $1168 one step toward the
 * colours at $02:D65A + $1168: each of blue, green and red up or down by
 * one where it differs.
 * HOW: Each part masked ($7C00, $03E0, $001F) and compared through
 * $1F2B; blue and green kept at $3E and $40, then ORed with red. The count
 * kept on the stack. Runs only from unit_cpu.
 * Entry: D = $0000. Exit: with $0CFA 0, M 8-bit and A's low byte 0;
 * otherwise M 16-bit, X 16-bit, A = 0, X and Y past the last colour.
 */
void abi_PaletteFadeStep(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint16_t n, v;

    op_sep(P_M);
    op_lda8(b | 0x0CFA);
    if (!(uint8_t)cpu.a)
        return;
    v = (uint8_t)(cpu.a - 1);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, (uint8_t)v);
    wr8(b | 0x0CFA, (uint8_t)v);
    op_rep(P_M | P_X);
    cpu.x = rd16(b | 0x1168);
    cpu.p = nz16(cpu.p, cpu.x);
    cpu.y = rd16(b | 0x1168);
    cpu.p = nz16(cpu.p, cpu.y);
    op_lda16(b | 0x1167);
    do {
        n = cpu.a;
        wr16(0x00003E, fade_part(b, 0x7C00, 0x0400));
        wr16(0x000040, fade_part(b, 0x03E0, 0x0020));
        fade_part(b, 0x001F, 1);
        cpu.a |= rd16(0x00003E);
        cpu.a |= rd16(0x000040);
        cpu.p = nz16(cpu.p, cpu.a);
        wr16(b | (uint16_t)(0x0722 + cpu.y), cpu.a);
        cpu.y = (uint16_t)(cpu.y + 2);
        cpu.x = (uint16_t)(cpu.x + 2);
        cpu.p = nz16(cpu.p, cpu.x);
        cpu.a = (uint16_t)(n - 1);
        cpu.p = nz16(cpu.p, cpu.a);
    } while (cpu.a);
}
