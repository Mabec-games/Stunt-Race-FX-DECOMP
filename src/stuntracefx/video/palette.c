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

    cyc(56);
    wr8(CGADD, c);
    for (i = 0; i < n; i++) {
        cyc(32);
        v = rd8(src + i);
        cyc(24);
        wr8(CGDATA, v);
    }
    cyc(42);
    return v;
}

/* Colours $E1-$E3 when animating, else just colour 0. */
static uint8_t raster_colours(void)
{
    cyc(88);
    if (rd8(PALETTE_ANIMATE) == 0)
        { cyc(6); return cgram_put(0x00, 0x7EFD65, 2); }
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
    cyc(46);
    if (rd8(PALETTE_ANIMATE) == 0)
        { cyc(6); return cgram_put(0x00, 0x7EFD65, 2); } else { cyc(62); }
    CgramDma(0xFD65, 0x00, 0x0060);
    cyc(88);
    if (rd8(PALETTE_ANIMATE) == 0)
        return cgram_put(0x00, 0x7EFD65, 2); else { cyc(62); }
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
    cyc(52);
    cgram_put(0xE1, 0x7EFF27, 6);
    if (!(rd8(0x000E3B) & 0x01))
        { cyc(16); return cgram_put(0x10, (0x7EFD85 + x) & 0xFFFFFF, 4); } else { cyc(22); }
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
    cyc(24);
    uint16_t phase = rd16(phase_addr), i = phase, next;

    if (phase >= 8) {
        cyc(72);
        i = (uint16_t)(0x000F - phase);
        *p = (uint8_t)((*p & ~P_V) | (((0x000F ^ phase) & (0x000F ^ i) & 0x8000) ? P_V : 0));
    }
    *x = (uint16_t)(i << 1);
    cyc(106);
    wr16(dest, cyc_then(30, rd16(table + *x)));
    next = (uint16_t)(phase + 1);
    if (next >= 0x0010) {
        cyc(18);
        next = 0;
        *p |= P_C;
    } else {
        *p &= (uint8_t)~P_C;
    }
    cyc(124);
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
    cyc(276);
    if (!(rd8(0x000E3B) & 0x01)) {
        cyc(54);
        cycle_step(0x7EFD5B, 0x7EB71B, 0x7EFD85, x, p);
        return cycle_step(0x7EFD5D, 0x7EB6FB, 0x7EFD87, x, p);
    } else { cyc(60); }
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
    uint16_t c = 0, n;
    unsigned i;

    cyc(540);
    wr16(0x0000F1, 0x0010);
    for (i = 0; i < 16; i++) {
        cyc(78);
        c = rd16((uint32_t)bank << 16 | (uint16_t)(src + 2 * i));
        wr16(0x7F35D7 + 2 * i, c);
        cyc(32);
        wr16(0x7F37D7 + 2 * i, c);
        cyc(32);
        wr16(0x7EFD65 + 2 * i, c);
        cyc(72);
        n = rd16(0x0000F1);
        cyc(14);
        wr16(0x0000F1, (uint16_t)(n - 1));
        cyc(22);
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
    cyc(90);
    uint8_t v = rd8((uint32_t)bank << 16 | 0x0D22);

    *ran = !v;
    if (v)
        { cyc(22); return v; } else { cyc(92); }
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
    cyc(70);
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

    cyc(102);
    wr16(0x7F345C, v);
    for (i = 0; i < 4; i++) {
        cyc(32);
        v = (uint16_t)(rd16(0x7F37F7 + 2 * i) - 1);
        cyc(46);
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
    uint16_t w = 0, n;
    unsigned i;

    cyc(450);
    wr16(0x0000F1, 0x0010);
    for (i = 0; i < 16; i++) {
        cyc(78);
        w = rd16((uint32_t)bank << 16 | (uint16_t)(src + 2 * i));
        wr16(0x7F39D7 + 2 * i, w);
        cyc(72);
        n = rd16(0x0000F1);
        cyc(14);
        wr16(0x0000F1, (uint16_t)(n - 1));
        cyc(22);
    }
    cyc(82);
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
    cyc(46);
    uint16_t c = rd16(b | 0x0E3D), i, x, y, w, d, s;
    uint8_t blk;
    unsigned k;

    s = (uint16_t)((uint16_t)(c << 1) + rd16(cyc_then(52, b | 0x0E3D)));
    *p = (uint8_t)((*p & ~P_V) | ((~((uint16_t)(c << 1) ^ c) & ((uint16_t)(c << 1) ^ s) & 0x8000) ? P_V : 0));
    i = (uint16_t)(s & 0x00FF);
    cyc(208);
    blk = rd8(0x03F64B + i);
    y = rd16(0x03F649 + i);
    for (x = 0x0020; x < 0x0200; x = (uint16_t)(x + 2)) {
        src = ((uint32_t)blk << 16) + 0x0020 + y;
        cyc(180);
        w = rd16(src & 0xFFFFFF);
        wr16(0x7F35D7 + x, w);
        cyc(32);
        wr16(0x7F37D7 + x, w);
        cyc(32);
        wr16(0x7EFD65 + x, w);
        y = (uint16_t)(y + 2);
    }
    cyc(330);
    blk = rd8(0x03F69F + i);
    y = rd16(0x03F69D + i);
    for (x = 0x0020; x < 0x0200; x = (uint16_t)(x + 2)) {
        src = ((uint32_t)blk << 16) + 0x0020 + y;
        cyc(180);
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
        int shifted;

        y = (uint16_t)((x >> 4) & 0x001E);
        cyc(226);
        wr16(0x0000F1, n);
        cyc(70);
        bits = rd16(b | (uint16_t)(0xEF1F + y));
        shifted = n != 0;
        while (n) {
            bits >>= 1;
            cyc(52);
            n = (uint16_t)(rd16(0x0000F1) - 1);
            cyc(14);
            wr16(0x0000F1, n);
        }
        /* to the LDA long: BIT and BNE (6 more taken), and with no shift
           loop the table's ROM read and BRA before them */
        cyc((shifted ? 88 : 110) + (bits & 1 ? 6 : 0));
        w = rd16(0x7F37D7 + x);
        cyc(32);
        d = rd16(0x7F39D7 + x);
        *p = (uint8_t)((*p & ~P_C) | (w >= d ? (cyc(22), P_C) : (cyc(22), 0)));
        if (!(bits & 1)) {
            if (w == d)
                { cyc(168); wr16(0x7F39D7 + x, (uint16_t)(w & 0x001F ? w - 1 : (cyc(6), w + 1))); }
        } else if (w != d) {
            cyc(26);
            wr16(0x7F39D7 + x, w);
        }
        if (!x)
            break;
        x = (uint16_t)(x - 2);
    }
    cyc(92);
    wr16(b | 0x0E21, 0x45CC);
    cyc(48);
    wr16(b | 0x0E23, 0x2528);
    cyc(48);
    wr16(b | 0x0E25, 0x1CE5);
    cyc(48);
    wr16(b | 0x0E27, 0x739A);
    cyc(48);
    wr16(b | 0x0E29, 0x1CE5);
    cyc(48);
    wr16(b | 0x0E2B, 0x2528);
    cyc(24);
    w = rd16(b | 0x10DC);
    if (!w) {
        *xo = 0xFFFE;
        *yo = y;
        *p = (uint8_t)((nz16(*p, 0) & ~(P_M | P_X)));
        return 0;
    } else { cyc(40); }
    for (k = 0; k < 0x20; k++) {
        cyc(92);
        wr8(0x7EFEC5 + k, 0x00);
        cyc(32);
        wr8(0x7F3937 + k, 0x00);
        cyc(32);
        wr8(0x7F3B37 + k, 0x00);
        cyc(32);
        wr8(0x7F3737 + k, 0x00);
    }
    cyc(32);
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
/* One 5-bit field of the colour word at dp (the low five bits), stored at
   `at`; the word is then shifted right 5 in place (five LSRs). */
static void blend_split(uint32_t at, uint32_t dp)
{
    cyc(16);
    uint16_t v = rd16(dp), w;
    int i;

    cyc(24);
    for (i = 0; i < 5; i++) {
        cyc(16);
        w = rd16(dp);
        cyc(14);
        wr16(dp, (uint16_t)(w >> 1));
    }
    cyc(24);
    wr16(at, (uint16_t)(v & 0x1F));
}

static BlendEnd colour_blend(uint8_t bank, uint8_t p)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t r, g, bl, m, s, t, f;
    BlendEnd e;

    blend_split(b | 0x344E, 0x0000F1);
    blend_split(b | 0x3452, 0x0000F1);
    blend_split(b | 0x3456, 0x0000F1);
    blend_split(b | 0x3450, 0x0000F3);
    blend_split(b | 0x3454, 0x0000F3);
    blend_split(b | 0x3458, 0x0000F3);
    cyc(24);
    t = rd16(b | 0x3450);
    cyc(38);
    f = rd16(b | 0x344E);
    cyc(46);
    m = PpuMultiply((uint8_t)(t - f));
    cyc(80);
    f = rd16(b | 0x344E);
    r = (uint16_t)((m + f) & 0x1F);
    cyc(48);
    wr16(b | 0x344E, r);
    cyc(24);
    t = rd16(b | 0x3454);
    cyc(38);
    f = rd16(b | 0x3452);
    cyc(46);
    m = PpuMultiply((uint8_t)(t - f));
    cyc(80);
    f = rd16(b | 0x3452);
    g = (uint16_t)(m + f);
    cyc(24);
    wr16(b | 0x3452, g);
    cyc(24);
    t = rd16(b | 0x3458);
    cyc(38);
    f = rd16(b | 0x3456);
    cyc(46);
    m = PpuMultiply((uint8_t)(t - f));
    cyc(80);
    bl = rd16(b | 0x3456);
    s = (uint16_t)(m + bl);
    p = (uint8_t)(p & ~(P_V | P_C));
    if (~(m ^ bl) & (m ^ s) & 0x8000)
        p |= P_V;
    bl = (uint16_t)((uint16_t)((uint16_t)(s >> 8 | s << 8) << 2) & 0x7C00);
    cyc(88);
    wr16(0x0000F1, bl);
    cyc(24);
    g = rd16(b | 0x3452);
    if (g & 0x0800)
        p |= P_C;
    cyc(110);
    bl = rd16(0x0000F1);
    cyc(24);
    r = rd16(b | 0x344E);
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
    return v < 0x0D ? v : (cyc(10), (n == 1 ? 0 : (uint8_t)(v - 0x0D)));
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

    cyc(102);
    if (!rd8(b | 0x0D53) && rd8(cyc_then(40, b | (uint16_t)(0x0E05 + y)))
        && (a = rd16(cyc_then(62, b | (uint16_t)(0x0D1E + x))), a & 0x4000)) {
        cyc(86);
        t = rd8(b | (uint16_t)(0x0E01 + y));
        if (t == 0xFF) {
            go = TWO;
        } else {
            t++;
            cyc(76);
            wr8(b | (uint16_t)(0x0E01 + y), t);
            go = t >= 0xB4 ? TWO : t >= 0x78 ? ONE : t >= 0x3C ? ODD : SKIP;
        }
    } else {
        cyc(68);
        t = rd8(b | (uint16_t)(0x0E01 + y));
        if (!t) {
            a = 0x7E63;
            cyc(100);
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
        { cyc(120); go = (rd8(b | 0x0E3B) & 0x03) ? (cyc(38), NONE) : ONE; }
    else if (go == ODD)
        go = (rd8(b | 0x0E3B) & 0x01) ? NONE : ONE;
    if (go == ONE || go == TWO) {
        step = cycle13(rd8(b | (uint16_t)(0x0E03 + y)), go == TWO ? 2 : 1);
        wr8(b | (uint16_t)(0x0E03 + y), step);
        x = (uint8_t)(step << 1);
        a = rd16(0x7EB4E9 + x);
        wr16(0x7EFF2B, a);
    }
    cyc(46);
    v = rd8(b | (uint16_t)(0x0DFD + y));
    cyc(38);
    d = rd8(b | (uint16_t)(0x0DFB + y));
    s = (unsigned)v - d;
    p = (uint8_t)(p & ~(P_C | P_V));
    if (s < 0x100)
        p |= P_C;
    if ((v ^ d) & (v ^ (uint8_t)s) & 0x80)
        p |= P_V;
    cyc(30);
    wr8(b | (uint16_t)(0x0DFD + y), (uint8_t)s);
    if (p & P_C) {
        cyc(22);
        e.a = (uint16_t)((a & 0xFF00) | (uint8_t)s);
        e.p = (uint8_t)(nz8(p, (uint8_t)s) | P_M);
    } else {
        step = cycle13(rd8(b | (uint16_t)(0x0DFF + y)), 1);
        cyc(82);
        wr8(b | (uint16_t)(0x0DFF + y), step);
        x = (uint8_t)(step << 1);
        cyc(82);
        wr16(0x7EFF27, cyc_then(32, rd16(0x7EB505 + x)));
        cyc(32);
        e.a = rd16(0x7EB521 + x);
        cyc(32);
        wr16(0x7EFF29, e.a);
        e.p = (uint8_t)(nz16(p, e.a) & ~(P_C | P_M));
    }
    cyc(42);
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
    PpuMultiplierSetLowByteTimes8((uint8_t)rd16(0x0000F5));
    return ColourBlend(bank);
}

void abi_ColourBlendBy(void)
{
    cyc(16);
    PpuMultiplierSetLowByteTimes8((uint8_t)rd16(0x0000F5));
    (cyc(22), abi_ColourBlend());
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
    uint16_t t, v;

    wr16(0x0000F1, colour);
    t = rd16(b | 0x3470);
    if (t & 0x8000)
        t = rd16(0x0000F1);
    wr16(0x0000F3, t);
    t = rd16(b | 0x3468);
    wr16(0x0000F5, t);
    v = ColourBlendBy(bank);
    wr16(0x000E21 + x, v);
    return v;
}

void abi_ColourBlendToTarget(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint16_t t;

    cyc(16);
    wr16(0x0000F1, cpu.a);
    cyc(24);
    t = rd16(b | 0x3470);
    if (t & 0x8000)
        { cyc(32); t = rd16(0x0000F1); } else { cyc(22); }
    cyc(16);
    wr16(0x0000F3, t);
    cyc(24);
    t = rd16(b | 0x3468);
    cyc(16);
    wr16(0x0000F5, t);
    (cyc(46), abi_ColourBlendBy());
    cyc(74);
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
        v = s > 0xFF ? (cyc(10), 0xFF) : (uint8_t)s;
    }
    k = (uint8_t)(v >> 5);
    src = 0x0E21;
    if (!k) {
        cyc(178);
        wr8(b | (uint16_t)(0x0E1F + y), 0x00);
    } else {
        n = (uint8_t)(8 - k);
        p = (uint8_t)(p & ~P_V);
        cyc(292);
        wr8(0x0000F9, n);
        cyc(30);
        wr8(0x0000FA, (uint8_t)(n << 1));
        cyc(30);
        c = rd8(b | (uint16_t)(0x0E1F + y));
        if (c >= (uint8_t)(n << 1))
            { cyc(10); c = 0xFF; }
        c = (uint8_t)(c + 1);
        cyc(90);
        wr8(b | (uint16_t)(0x0E1F + y), c);
        if (c < n)
            { cyc(6); src = 0x0E27; }
        cyc(18);
    }
    p = (uint8_t)((p & ~P_C) | ((y & 0x8000) ? P_C : 0));
    e.y = (uint16_t)(y << 1);
    cyc(116);
    e.x = rd16(b | (uint16_t)(0x0D83 + e.y));
    cyc(38);
    wr16(0x7E01C8 + e.x, cyc_then(32, rd16(b | src)));
    cyc(24);
    wr16(0x7E01CA + e.x, cyc_then(32, rd16(b | (uint16_t)(src + 2))));
    cyc(24);
    e.a = rd16(b | (uint16_t)(src + 4));
    cyc(32);
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
 * @routine 7E:B5E3 PlayerColoursFlashRts
 * WHAT IT DOES: An RTS inside PlayerColoursFlash that nothing branches to.
 * HOW: Returns.
 * Entry: any. Exit: all unchanged.
 */
void abi_PlayerColoursFlashRts(void)
{
    cyc(42);
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

    cyc(30);
    v = rd8(b | (uint16_t)(0x0DDA + cpu.y));
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, v);
    if (v == 0) {
        cyc(68);
        cpu.p &= (uint8_t)~P_M;
        cpu.a = 0x0000;
        cpu.p = nz16(cpu.p, 0);
        goto store;
    }
    cyc(46);
    v = rd8(b | (uint16_t)(0x0DF2 + cpu.y));
    flags = (uint16_t)(v << 8);
    cyc(50);
    flags |= rd8(b | (uint16_t)(0x0DF0 + cpu.y));
    cpu.a = flags;
    cpu.p &= (uint8_t)~P_M;
    if (flags & 0x0400) {
        cyc(150);
        v = rd8(b | (uint16_t)(0x0E05 + cpu.y));
        if (v) {
            cpu.a = flags;
            cyc(104);
            t = rd16(b | 0x0E3B);
            cpu.x = (uint16_t)((t & 0x0003) << 1);
            cpu.p = (uint8_t)(cpu.p & ~P_C);
            cyc(84);
            cpu.a = rd16((uint32_t)(0x7EB5B9 + cpu.x));
            cpu.p = nz16(cpu.p, cpu.a);
            goto store;
        } else { cyc(12); }
    }
    cyc(86);
    if (flags & 0x0010) {
        cyc(74);
        v = rd8(b | (uint16_t)(0x0DDA + cpu.y));
        cpu.p = (uint8_t)((cpu.p & ~P_C) | (v >= 0x3C ? P_C : 0));
        if (v >= 0x3C) {
            cyc(78);
            t = rd16(b | 0x0E3B);
            table = 1;
        }
    }
    if (!table) {
        cyc(84);
        t = rd16(b | 0x0E3B);
        cpu.a = t;
        if (t & 0x0001) {
            cyc(46);
            cpu.p = nz16(cpu.p, t);
            return;
        } else { cyc(32); }
        cpu.p = (uint8_t)((cpu.p & ~P_C) | (t & 0x0001));
        t = (uint16_t)(t >> 1);
    }
    cpu.x = (uint16_t)((t & 0x0003) << 1);
    cpu.p = (uint8_t)(cpu.p & ~P_C);
    cyc(106);
    cpu.a = rd16((uint32_t)(0x7EB5B1 + cpu.x));
    cpu.p = nz16(cpu.p, cpu.a);
store:
    cpu.x = 0x0000;
    cyc(78);
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
    cyc(136);
    v = rd8(b | 0x0E5F);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    if (v) {
        cpu.y = 0x0000;
        cyc(64);
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | rd8(b | 0x0DDA));
        (cyc(46), abi_PlayerColoursFlash());
        cpu.p = (uint8_t)((cpu.p | P_M) & ~P_X);
        cpu.y = 0x0000;
        (cyc(156), abi_StatusColourPick());
        abi_PaletteCycle();
        op_sep(P_M | P_X);
        cpu.y = 0x00;
        cpu.x = 0x00;
        (cyc(70), abi_PaletteKeptColoursStep());
    } else { cyc(22); }
    cyc(28);
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
    cyc(148);
    v = (uint8_t)(rd8(0x000DA5) & 0x80);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    if (!v) {
        cyc(64);
        v = (uint8_t)(rd8(0x000DA5) & 0x20);
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
        if (v) {
            cpu.p &= (uint8_t)~(P_M | P_C);
            cyc(78);
            w = rd16(b | 0x19FE);
            cyc(38);
            w = op_adc16(w, rd16(b | 0x1A00), &cpu.p);
            cpu.a = w;
            cyc(24);
            op_cmp16(rd16(b | 0x1A02));
            if (!(cpu.p & P_Z))
                { cyc(36); wr16(b | 0x19FE, w); }
            else {
                cyc(72);
                v = (uint8_t)(rd8(0x000DA5) & 0xDF);
                cyc(48);
                wr8(0x000DA5, v);
            }
            cpu.p |= P_M;
            v = rd8(b | 0x19FF);
            cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
            cyc(32);
            wr8(0x7F3460, v);
        } else { cyc(38); }
    } else { cyc(38); }
    cyc(58);
    cpu.db = db0;
    op_plp(p0);
}

/* One colour part of PaletteFadeStep: the part (mask m) of the colour at
   $0722+Y moved one step toward the same part at $02:D65A+X. */
static uint16_t fade_part(uint32_t b, uint16_t m, uint16_t step)
{
    uint16_t c;

    cyc(30);
    op_lda16(b | (uint16_t)(0x0722 + cpu.y));
    op_and16(m);
    cyc(46);
    wr16(b | 0x1F2B, cpu.a);
    cyc(94);
    op_lda16(0x02D65A + (uint32_t)cpu.x);
    op_and16(m);
    c = rd16(b | 0x1F2B);
    op_cmp16(c);
    if (cpu.a == c)
        { cyc(4); return cpu.a; }
    if (cpu.p & P_C) {
        cyc(58);
        op_lda16(b | 0x1F2B);
        if (step == 1) {
            cpu.a++;
            cpu.p = nz16(cpu.p, cpu.a);
        } else {
            cyc(24);
            op_add16(step);
        }
    } else {
        cyc(52);
        op_lda16(b | 0x1F2B);
        if (step == 1) {
            cpu.a--;
            cpu.p = nz16(cpu.p, cpu.a);
        } else {
            cyc(18);
            op_sub16(step);
        }
        cyc(22);
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
    cyc(46);
    op_lda8(b | 0x0CFA);
    if (!(uint8_t)cpu.a)
        { cyc(60); return; }
    v = (uint8_t)(cpu.a - 1);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, (uint8_t)v);
    cyc(60);
    wr8(b | 0x0CFA, (uint8_t)v);
    op_rep(P_M | P_X);
    cyc(24);
    cpu.x = rd16(b | 0x1168);
    cpu.p = nz16(cpu.p, cpu.x);
    cyc(24);
    cpu.y = rd16(b | 0x1168);
    cpu.p = nz16(cpu.p, cpu.y);
    cyc(24);
    op_lda16(b | 0x1167);
    do {
        cyc(30);
        n = cpu.a;
        wr16(0x00003E, cyc_then(30, fade_part(b, 0x7C00, 0x0400)));
        wr16(0x000040, cyc_then(30, fade_part(b, 0x03E0, 0x0020)));
        fade_part(b, 0x001F, 1);
        cyc(30);
        cpu.a |= rd16(0x00003E);
        cyc(16);
        cpu.a |= rd16(0x000040);
        cpu.p = nz16(cpu.p, cpu.a);
        cyc(30);
        wr16(b | (uint16_t)(0x0722 + cpu.y), cpu.a);
        cpu.y = (uint16_t)(cpu.y + 2);
        cpu.x = (uint16_t)(cpu.x + 2);
        cpu.p = nz16(cpu.p, cpu.x);
        cpu.a = (uint16_t)(n - 1);
        cpu.p = nz16(cpu.p, cpu.a);
    } while (cpu.a);
    cyc(172);
}

/* LDA $0D30, INC, back to 0 at $16, STA $0D30, ASL, TAX (M 8-bit): the
   cycle's next step and its table offset (with X 16-bit, TAX also takes
   A's high byte). */
static void palette_cycle_step(uint32_t b)
{
    cyc(56);
    uint8_t v = (uint8_t)(rd8(b | 0x0D30) + 1);

    if (v >= 0x16)
        { cyc(10); v = 0x00; }
    cyc(76);
    wr8(b | 0x0D30, v);
    v = (uint8_t)(v << 1);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.x = (cpu.p & P_X) ? (cyc(28), v) : (cyc(28), cpu.a);
    cpu.p = nz8(cpu.p, v);
}

/*
 * @routine 04:E32D CoursePaletteCycle
 * WHAT IT DOES: Steps the colour cycle of courses 3, 7 and $0B: the step
 * $0D30 goes round 0 to $15, and its entry in word tables in bank 04 sets
 * colours 12-15 of the palette: for course 3 in $7E:FD7D from the tables
 * at $04:E45F, $E48B, $E4B7 and $E4E3; for course 7 in $7F:39EF from those
 * at $E50F, $E53B, $E4B7 and $E4E3, with fixed colours $14A7, $1086,
 * $15FF and $269F in $7F:37EF; for course $0B only the low byte of colour
 * 15, in all three copies, from the byte tables at $04:E433 (when
 * $7F:3460 is $18 or more) or $04:E407. Other courses: nothing.
 * HOW: P and the data bank ($04) pushed; 8-bit step, 16-bit colours (8-bit
 * for course $0B). Course $0B is not reached in the traces so far.
 * Entry: any widths. Exit: P and the data bank restored; A the last value
 * loaded; X the step's offset.
 */
void abi_CoursePaletteCycle(void)
{
    const uint32_t b = 0x040000;
    uint8_t p0 = cpu.p, db0 = cpu.db, c, v;
    static const uint16_t c3[4] = {0xE45F, 0xE48B, 0xE4B7, 0xE4E3};
    static const uint16_t c7[4] = {0xE50F, 0xE53B, 0xE4B7, 0xE4E3};
    static const uint16_t fixed7[4] = {0x14A7, 0x1086, 0x15FF, 0x269F};
    unsigned i;

    op_sep(P_M);
    cpu.db = 0x04;
    cyc(140);
    op_lda8(b | 0x0E3D);
    c = (uint8_t)cpu.a;
    if (c == 0x03) {
        palette_cycle_step(b);
        op_rep(P_M);
        for (i = 0; i < 4; i++) {
            cyc(32);
            cpu.a = rd16(b + c3[i] + cpu.x);
            cyc(48);
            wr16(0x7EFD7D + 2 * i, cpu.a);
        }
    } else if (c == 0x07) {
        cyc(44);
        palette_cycle_step(b);
        op_rep(P_M);
        for (i = 0; i < 4; i++)
            { cyc(56); wr16(0x7F37EF + 2 * i, fixed7[i]); }
        for (i = 0; i < 4; i++) {
            cyc(80);
            cpu.a = rd16(b + c7[i] + cpu.x);
            wr16(0x7F39EF + 2 * i, cpu.a);
        }
    } else if (c == 0x0B) {
        cyc(100);
        palette_cycle_step(b);
        cyc(32);
        v = rd8(0x7F3460);
        cyc(104);
        v = rd8(b + (v >= 0x18 ? 0xE433 : (cyc(6), 0xE407)) + cpu.x);
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
        wr8(0x7EFD83, v);
        cyc(32);
        wr8(0x7F37F5, v);
        cyc(32);
        wr8(0x7F39F5, v);
    } else { cyc(114); }
    cyc(82);
    op_sep(P_M);
    cpu.db = db0;
    op_plp(p0);
}

void abi_ColourBlend(void);
void abi_PpuMultiplierSetLowByteTimes8(void);

/* HudColourStep's work registers in bank $7F. */
#define HC_TO(o) (0x7F3458u + (o))      /* $3458: +2 band end, +4 request, +8 blend, +12 last blend */
#define HC_AMT(o) (0x7F3468u + (o))     /* $3468: amount, +4 last amount */
#define HC_TGT(o) (0x7F3470u + (o))     /* $3470: target, +4 last, +8 gradient flag, +12 band */
#define HC_WORK_A 0x7F35D7u
#define HC_WORK_B 0x7F37D7u
#define HC_WORK_C 0x7F39D7u
#define HC_BUFFER 0x7EFD65u

static uint32_t hc_dp(uint8_t off)
{
    return (uint16_t)(cpu.d + off);
}

/* BIT # (16-bit): Z only. */
static void hc_bit(uint16_t m)
{
    cpu.p = (uint8_t)((cpu.p & ~P_Z) | ((cpu.a & m) ? 0 : P_Z));
}

/* INX, INX (or INY, INY) and CPX/CPY: the next colour, and whether the
   loop goes on (C clear). */
static void hc_step(uint16_t *r, uint16_t end)
{
    *r = (uint16_t)(*r + 2);
    cpu.p = (uint8_t)((nz16(cpu.p, (uint16_t)(*r - end)) & ~P_C) | (*r >= end ? (cyc(28), P_C) : (cyc(28), 0)));
}

enum { HC_START, HC_ROWS_REST, HC_FROM14 };

/* HudColourStep's body, entered at its start or at one of its two inner
   entries. The start keeps the P and data bank it pushes (and the colour
   the band loop pushes and pulls) here, not on the stack: called from C
   there is no return address under them, so stack writes would land where
   the original writes other things. An inner entry finds P and the data
   bank on the stack above its return address, as the start leaves them. */
static void hud_colour_step(int from)
{
    uint8_t p0 = cpu.p, db0 = cpu.db;
    uint16_t pushed;

    switch (from) {
    case HC_ROWS_REST:
        goto rows_rest;
    case HC_FROM14:
        cpu.x = 0x0014;
        cpu.p = nz16(cpu.p, cpu.x);
        goto buffer;
    }
    op_sep(P_M);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x7F);
    cpu.db = 0x7F;
    cpu.p = nz8(cpu.p, 0x7F);
    op_rep(P_M | P_X);
    cyc(156);
    op_lda16(HC_TO(4));
    if (cpu.a) {
        if (cpu.a & 0x8000) {
            op_and16(0x7FFF);
            cyc(90);
            wr16(HC_TGT(0), cpu.a);
            cyc(24);
            op_lda16(HC_AMT(0));
            op_add16(0x0002);
            op_cmp16(0x001F);
            if (cpu.p & P_C)
                op_lda16_imm(0x001F); else { cyc(20); }
            cyc(38);
            goto store_amount;
        }
        cyc(70);
        op_lda16(0x000E3D);
        op_and16(0x00FF);
        op_cmp16(0x0006);
        if (cpu.p & P_Z) {
            cyc(88);
            op_lda16(HC_AMT(0));
            op_sub16(0x0003);
            op_cmp16(0x000A);
            if (cpu.p & P_C)
                { cyc(52); goto store_amount; }
            op_lda16_imm(0x000A);
            goto floor;
        }
        op_cmp16(0x0004);
        if (cpu.p & P_Z) {
            cyc(118);
            op_lda16(HC_AMT(0));
            op_sub16(0x0003);
            op_cmp16(0x000F);
            if (cpu.p & P_C)
                { cyc(52); goto store_amount; } else { cyc(22); }
            op_lda16_imm(0x000F);
            goto floor;
        }
        cyc(132);
        op_lda16(0x000D60);
        op_cmp16(0x0F00);
        if (!(cpu.p & P_Z)) {
            cyc(60);
            op_lda16(HC_AMT(0));
            goto store_amount;
        }
        cyc(66);
        op_lda16(HC_AMT(0));
        if (!cpu.a)
            goto gradient;
        op_sub16(0x0003);
        if (cpu.p & P_C)
            { cyc(48); goto store_amount; }
        op_lda16_imm(0xFFFF);
        cyc(74);
        wr16(HC_TGT(0), 0xFFFF);
        op_lda16_imm(0x0000);
        cyc(10);
        wr16(HC_TO(4), 0x0000);
        goto store_amount;
floor:
        cyc(78);
        wr16(HC_AMT(0), cpu.a);
        op_lda16_imm(0x0000);
        cyc(10);
        wr16(HC_TO(4), 0x0000);
        goto gradient;
store_amount:
        cyc(46);
        wr16(HC_AMT(0), cpu.a);
    } else { cyc(24); }
gradient:
    cyc(54);
    op_lda16(0x000DA4);
    hc_bit(0x0800);
    if (cpu.a & 0x0800) {
        cyc(64);
        wr16(HC_TGT(8), cpu.a);
        cpu.y = 0x0080;
        cpu.p = nz16(cpu.p, cpu.y);
        cpu.x = 0x00FE;
        cpu.p = nz16(cpu.p, cpu.x);
        cyc(80);
        op_lda16(0x000D7F);
        if (cpu.a) {
            cyc(18);
            cpu.x = 0x01FE;
            cpu.p = nz16(cpu.p, cpu.x);
        }
        cyc(46);
        op_lda16(HC_TGT(0));
        if (!(cpu.a & 0x8000)) {
            wr16(hc_dp(0xF3), cyc_then(32, cpu.a));
            op_lda16_imm(0x0000);
            wr16(hc_dp(0xF1), cyc_then(2, 0x0000));
            cyc(24);
            op_lda16(HC_AMT(0));
            wr16(hc_dp(0xF5), cyc_then(16, cpu.a));
            (cyc(46), abi_ColourBlendBy());
        }
        cyc(54);
        op_lda16(0x000D7F);
        op_asl16();
        op_tax16();
        op_lda16_imm(0xF757);
        cyc(16);
        wr16(0x000D9A + cpu.x, cpu.a);
        op_lda16_imm(0x0000);
        cpu.x = 0x0004;
        cpu.p = nz16(cpu.p, cpu.x);
        do {
            cyc(42);
            wr16(HC_BUFFER + cpu.x, cpu.a);
            cyc(36);
            hc_step(&cpu.x, 0x0020);
        } while (cpu.x != 0x0020);
        cyc(28);
        goto end;
    }
    cyc(70);
    op_lda16(HC_TGT(8));
    if (cpu.a) {
        cyc(40);
        op_lda16(HC_TGT(0));
        cyc(24);
        wr16(HC_TGT(4), cpu.a);
        op_lda16_imm(0x0000);
        cyc(10);
        wr16(HC_TGT(8), 0x0000);
        cpu.x = 0x0004;
        cpu.p = nz16(cpu.p, cpu.x);
        goto blend_rows;
    }
    cyc(46);
    op_lda16(HC_TGT(12));
    cpu.a++;
    cpu.p = nz16(cpu.p, cpu.a);
    op_cmp16(0x0003);
    if (cpu.p & P_C)
        op_lda16_imm(0x0000); else { cyc(20); }
    cyc(60);
    wr16(HC_TGT(12), cpu.a);
    op_asl16();
    op_tax16();
    cyc(30);
    op_lda16(0x7EE69C + cpu.x);
    op_tay16();
    op_add16(0x0100);
    cyc(76);
    wr16(HC_TO(2), cpu.a);
    cyc(32);
    op_lda16(0x000D7F);
    op_asl16();
    op_tax16();
    cpu.a = cpu.y;
    cpu.p = nz16(cpu.p, cpu.a);
    op_add16(0xEC57);
    cyc(82);
    wr16(0x000D9A + cpu.x, cpu.a);
    cyc(32);
    op_lda16(0x000D96);
    op_tax16();
    cyc(8);
    op_lda16(HC_TGT(0));
    if (cpu.a & 0x8000) {
        cyc(40);
        op_lda16(HC_TO(8));
        abi_PpuMultiplierSetLowByteTimes8();
        cyc(94);
        op_lda16(0x7FEF57 + cpu.x);
        wr16(hc_dp(0xF1), cyc_then(16, cpu.a));
        cyc(52);                                /* BRA, LDA abs,X */
        goto a_to;
        for (;;) {
            cyc(24);
            op_lda16(0x7FEF57 + cpu.x);
            cyc(30);
            op_cmp16(rd16(0x7FEF55 + cpu.x));
            if (!(cpu.p & P_Z)) {
                wr16(hc_dp(0xF1), cyc_then(34, cpu.a));
                cyc(30);                        /* LDA abs,X */
                goto a_to;
            }
            wr16(hc_dp(0xF1), cyc_then(28, cpu.a));
            cyc(30);
            op_lda16(0x7FF357 + cpu.x);
            cyc(30);
            op_cmp16(rd16(0x7FF355 + cpu.x));
            if (!(cpu.p & P_Z))
                { cyc(18); goto a_blend; }
            cyc(42);
            op_lda16(0x7FEC55 + cpu.y);
            cyc(52);                            /* BRA, STA abs,Y */
            goto a_store;
a_to:
            op_lda16(0x7FF357 + cpu.x);
a_blend:
            wr16(hc_dp(0xF3), cyc_then(16, cpu.a));
            cyc(46);
            abi_ColourBlend();
            cyc(72);                            /* RTS, STA abs,Y */
a_store:
            wr16(0x7FEC57 + cpu.y, cpu.a);
            hc_step(&cpu.x, 0);
            cyc(52);
            hc_step(&cpu.y, rd16(HC_TO(2)));
            if (cpu.p & P_C)
                { cyc(22); break; }
        }
        goto buffer4;
    }
    cyc(52);
    op_lda16(0x7FEF57 + cpu.x);
    wr16(hc_dp(0xF1), cyc_then(16, cpu.a));
    cyc(52);                                    /* BRA, LDA abs,X */
    goto b_to;
    for (;;) {
        cyc(24);
        op_lda16(0x7FEF57 + cpu.x);
        cyc(30);
        op_cmp16(rd16(0x7FEF55 + cpu.x));
        if (!(cpu.p & P_Z)) {
            wr16(hc_dp(0xF1), cyc_then(34, cpu.a));
            cyc(30);                            /* LDA abs,X */
            goto b_to;
        }
        wr16(hc_dp(0xF1), cyc_then(28, cpu.a));
        cyc(30);
        op_lda16(0x7FF357 + cpu.x);
        cyc(30);
        op_cmp16(rd16(0x7FF355 + cpu.x));
        if (!(cpu.p & P_Z))
            { cyc(18); goto b_blend; }
        cyc(42);
        op_lda16(0x7FEC55 + cpu.y);
        cyc(52);                                /* BRA, STA abs,Y */
        goto b_store;
b_to:
        op_lda16(0x7FF357 + cpu.x);
b_blend:
        wr16(hc_dp(0xF3), cyc_then(16, cpu.a));
        cyc(24);
        op_lda16(HC_TO(8));
        wr16(hc_dp(0xF5), cyc_then(16, cpu.a));
        (cyc(46), abi_ColourBlendBy());
rows_rest:
        pushed = cpu.a;
        cyc(96);
        op_lda16(HC_TGT(0));
        if (cpu.a & 0x8000) {
            cpu.a = pushed;
            cpu.p = nz16(cpu.p, cpu.a);
            cyc(88);                            /* BMI, PLA, STA abs,Y */
            goto b_store;
        }
        wr16(hc_dp(0xF3), cyc_then(32, cpu.a));
        cpu.a = pushed;
        cpu.p = nz16(cpu.p, cpu.a);
        wr16(hc_dp(0xF1), cyc_then(52, cpu.a));
        cyc(24);
        op_lda16(HC_AMT(0));
        wr16(hc_dp(0xF5), cyc_then(16, cpu.a));
        (cyc(46), abi_ColourBlendBy());
        cyc(94);
b_store:
        wr16(0x7FEC57 + cpu.y, cpu.a);
        hc_step(&cpu.x, 0);
        cyc(52);
        hc_step(&cpu.y, rd16(HC_TO(2)));
        if (cpu.p & P_C)
            break;
    }
buffer4:
    cyc(10);
    cpu.x = 0x0004;
    cpu.p = nz16(cpu.p, cpu.x);
buffer:
    cyc(48);
    op_lda16(HC_TO(8));
    cyc(24);
    op_cmp16(rd16(HC_TO(12)));
    if (!(cpu.p & P_Z)) {
        cyc(42);
        op_lda16(HC_TGT(0));
        if (cpu.a & 0x8000)
            goto blend_both;
        goto blend_rows;
    }
    cyc(36);
    op_lda16(HC_TGT(0));
    if (cpu.a & 0x8000) {
        cyc(40);
        op_lda16(HC_TGT(4));
        if (cpu.a & 0x8000)
            goto hud;
        do {
            cyc(54);
            op_lda16(HC_WORK_A + cpu.x);
            cyc(32);
            wr16(HC_BUFFER + cpu.x, cpu.a);
            cyc(24);
            hc_step(&cpu.x, 0x0200);
        } while (!(cpu.p & P_C));
        goto hud;
    }
    cyc(46);
    op_cmp16(rd16(HC_TGT(4)));
    if (cpu.p & P_Z) {
        cyc(42);
        op_lda16(HC_AMT(0));
        cyc(24);
        op_cmp16(rd16(HC_AMT(4)));
        if (cpu.p & P_Z)
            { cyc(14); goto keep; }
    } else { cyc(18); }
    cyc(42);
    op_lda16(HC_AMT(0));
    abi_PpuMultiplierSetLowByteTimes8();
    cyc(18);
    do {
        cyc(78);
        op_lda16(HC_WORK_B + cpu.x);
        cyc(32);
        op_cmp16(rd16(HC_WORK_C + cpu.x));
        if (!(cpu.p & P_Z)) {
            cyc(36);
            op_lda16(HC_TGT(0));
            wr16(hc_dp(0xF3), cyc_then(16, cpu.a));
            cyc(32);
            op_lda16(HC_WORK_A + cpu.x);
            wr16(hc_dp(0xF1), cyc_then(16, cpu.a));
            (cyc(46), abi_ColourBlend());
            cyc(74);
            wr16(HC_BUFFER + cpu.x, cpu.a);
        } else { cyc(18); }
        hc_step(&cpu.x, 0x0200);
    } while (!(cpu.p & P_C));
    cyc(24);
    goto hud;
blend_rows:
    cyc(2);
    do {
        cyc(78);
        op_lda16(HC_WORK_B + cpu.x);
        cyc(46);
        op_sub16(rd16(HC_WORK_C + cpu.x));
        if (cpu.a) {
            cyc(30);
            op_cmp16(0x0001);
            if (cpu.p & P_Z)
                goto from_b; else { cyc(36); }
            op_cmp16(0xFFFF);
            if (cpu.p & P_Z)
                goto from_b;
            cyc(40);
            op_lda16(HC_TO(8));
            if (!cpu.a)
                goto from_b;
            op_cmp16(0x001F);
            if (cpu.p & P_Z) {
                cyc(84);
                op_lda16(HC_WORK_C + cpu.x);
                goto work;
            }
            cyc(90);
            op_lda16(HC_WORK_B + cpu.x);
            wr16(hc_dp(0xF1), cyc_then(16, cpu.a));
            cyc(32);
            op_lda16(HC_WORK_C + cpu.x);
            wr16(hc_dp(0xF3), cyc_then(16, cpu.a));
            cyc(24);
            op_lda16(HC_TO(8));
            wr16(hc_dp(0xF5), cyc_then(16, cpu.a));
            (cyc(46), abi_ColourBlendBy());
            goto work;
from_b:
            cyc(54);
            op_lda16(HC_WORK_B + cpu.x);
work:
            cyc(54);
            wr16(HC_WORK_A + cpu.x, cpu.a);
            wr16(hc_dp(0xF1), cyc_then(16, cpu.a));
            cyc(24);
            op_lda16(HC_TGT(0));
            wr16(hc_dp(0xF3), cyc_then(16, cpu.a));
            cyc(24);
            op_lda16(HC_AMT(0));
            wr16(hc_dp(0xF5), cyc_then(16, cpu.a));
            (cyc(46), abi_ColourBlendBy());
            cyc(74);
            wr16(HC_BUFFER + cpu.x, cpu.a);
        } else { cyc(16); }
        hc_step(&cpu.x, 0x0200);
    } while (!(cpu.p & P_C));
    cyc(22);
    goto hud;
blend_both:
    cyc(46);
    op_lda16(HC_TO(8));
    wr16(hc_dp(0xF5), cyc_then(16, cpu.a));
    abi_PpuMultiplierSetLowByteTimes8();
    cyc(18);
    do {
        cyc(78);
        op_lda16(HC_WORK_B + cpu.x);
        cyc(46);
        op_sub16(rd16(HC_WORK_C + cpu.x));
        if (cpu.a) {
            cyc(52);
            op_cmp16(0x0001);
            if (!(cpu.p & P_Z)) {
                cyc(36);
                op_cmp16(0xFFFF);
                if (!(cpu.p & P_Z)) {
                    cyc(26);
                    op_lda16(HC_WORK_B + cpu.x);
                    wr16(hc_dp(0xF1), cyc_then(16, cpu.a));
                    cyc(32);
                    op_lda16(HC_WORK_C + cpu.x);
                    wr16(hc_dp(0xF3), cyc_then(16, cpu.a));
                    (cyc(46), abi_ColourBlend());
                    cyc(74);
                    wr16(HC_WORK_A + cpu.x, cpu.a);
                    cyc(32);
                    wr16(HC_BUFFER + cpu.x, cpu.a);
                }
            }
        } else { cyc(16); }
        hc_step(&cpu.x, 0x0200);
    } while (!(cpu.p & P_C));
hud:
    {
        static const struct { uint16_t colour, x; } hud[6] = {
            {0x45CC, 0x0000}, {0x2528, 0x0002}, {0x1CE5, 0x0004},
            {0x739A, 0x0006}, {0x1CE5, 0x0008}, {0x2528, 0x000A}};
        int i;

        for (i = 0; i < 6; i++) {
            cyc(96);
            op_lda16_imm(hud[i].colour);
            cpu.x = hud[i].x;
            cpu.p = nz16(cpu.p, cpu.x);
            abi_ColourBlendToTarget();
        }
    }
    op_rep(P_M | P_X);
    cyc(74);
    op_lda16(0x000E3D);
    op_and16(0x00FF);
    op_cmp16(0x0007);
    if (cpu.p & P_Z) {
        cyc(82);
        op_lda16(HC_TO(4));
        if (!cpu.a) {
            cyc(40);
            op_lda16(HC_TO(8));
            wr16(hc_dp(0xF5), cyc_then(16, cpu.a));
            abi_PpuMultiplierSetLowByteTimes8();
            cpu.x = 0x0018;
            cpu.p = nz16(cpu.p, cpu.x);
            cyc(42);
            do {
                cyc(78);
                op_lda16(HC_WORK_B + cpu.x);
                wr16(hc_dp(0xF1), cyc_then(16, cpu.a));
                cyc(32);
                op_lda16(HC_WORK_C + cpu.x);
                wr16(hc_dp(0xF3), cyc_then(16, cpu.a));
                (cyc(46), abi_ColourBlend());
                cyc(74);
                wr16(HC_WORK_A + cpu.x, cpu.a);
                cyc(32);
                wr16(HC_BUFFER + cpu.x, cpu.a);
                hc_step(&cpu.x, 0x0020);
            } while (!(cpu.p & P_C));
            cyc(18);
        }
    } else { cyc(42); }
keep:
    cyc(46);
    op_lda16(HC_TO(8));
    cyc(24);
    wr16(HC_TO(12), cpu.a);
    cyc(24);
    op_lda16(HC_AMT(0));
    cyc(24);
    wr16(HC_AMT(4), cpu.a);
    cyc(24);
    op_lda16(HC_TGT(0));
    cyc(24);
    wr16(HC_TGT(4), cpu.a);
end:
    cyc(20);
    op_sep(P_M | P_X);
    if (from != HC_START) {
        cpu.s++;
        db0 = rd8((uint16_t)cpu.s);
        cpu.s++;
        cyc(20);
        p0 = rd8((uint16_t)cpu.s);
    } else { cyc(36); }
    cyc(2);
    cpu.db = db0;
    cpu.p = nz8(cpu.p, db0);
    op_plp(p0);
}

/*
 * @routine 7E:E34F HudColourStep
 * WHAT IT DOES: The race's colour work each frame, with the data bank $7F:
 * steps the blend toward the target colour $3470 by $3468 ($345C asks: a
 * negative value sets the target to its low 15 bits and adds 2 to the
 * amount, at most $1F; a positive one takes 3 off it, down to $0A on course
 * 6 and $0F on course 4, elsewhere to nothing once the fade $0D60 is $0F00,
 * the target then $FFFF); with $0DA4 bit 11 the HUD's gradient goes to $F757
 * and the first 16 colours of the buffer at $7E:FD65 to 0, the target (when
 * not negative) blended from black at $F1 first. Otherwise it rebuilds one
 * of three gradient bands (the table at $7E:E69C, counter $347C) at
 * $7F:EC57 from the colours at $EF57/$F357 + $0D96, blended by $3460 and
 * toward the target; then the buffer from $7F:35D7, $37D7 and $39D7 as the
 * blend, target and amount have changed, five HUD colours at $0E21
 * (ColourBlendToTarget), on course 7 colours $0C-$0F again, and keeps the
 * blend, amount and target at $3464, $346C and $3474.
 * HOW: P and the data bank pushed at the start and pulled at the end; 16-bit
 * registers; colours through ColourBlend and ColourBlendBy with direct page
 * $F1, $F3 and $F5.
 * Entry: any widths; D = $0000. Exit: M 8-bit, X 8-bit inside, then P and
 * the data bank as they were; A, X, Y as the last step left them.
 */
void abi_HudColourStep(void)
{
    hud_colour_step(HC_START);
}

/*
 * @routine 7E:E4E4 HudColourRowsRest
 * WHAT IT DOES: HudColourStep from inside its second band loop, with the
 * blended colour in A: a negative target ($3470) keeps it as the row's
 * colour; otherwise it is blended toward the target by $3468. Then the
 * loop's other rows and the rest of HudColourStep. Nothing reaches it with a
 * negative target: the loop runs only when the target is positive.
 * HOW: Pushes A and pulls it back, as the loop does; ends as HudColourStep,
 * pulling the data bank and P the start pushed.
 * Entry: as inside HudColourStep: M 16-bit, X 16-bit, D = $0000, DB = $7F;
 * X = colour offset, Y = row offset; the data bank and P on the stack above
 * the return address. Exit: as HudColourStep.
 */
void abi_HudColourRowsRest(void)
{
    hud_colour_step(HC_ROWS_REST);
}

/*
 * @routine 7E:E50B HudColourStepFrom14
 * WHAT IT DOES: HudColourStep's buffer step from colour $14 (offset $14 into
 * the buffers) instead of 4. Nothing reaches it: the code before branches
 * over it.
 * HOW: X = $14, then as HudColourStep from its buffer step.
 * Entry: as inside HudColourStep: M 16-bit, X 16-bit, D = $0000, DB = $7F;
 * the data bank and P on the stack above the return address. Exit: as
 * HudColourStep.
 */
void abi_HudColourStepFrom14(void)
{
    hud_colour_step(HC_FROM14);
}

/*
 * @routine 7E:B6CB PaletteCycleFourth
 * WHAT IT DOES: A fourth colour-cycle step after PaletteCycle's code that
 * nothing reaches: the phase at $FD61 (a byte offset stepping by 2 and
 * wrapping at $10) picks one of the eight colours at $7E:B6EB, copied a
 * byte at a time to $FD8B + Y and $FD8C + Y.
 * HOW: 8-bit loads and stores (the phase as a 16-bit index), work RAM
 * through the data bank; it ends with PaletteCycle's PLB, RTS, so the
 * data bank comes back from the stack.
 * Entry: M 8-bit, X 16-bit; Y = offset; a data bank byte on the stack
 * above the return address. Exit: A low byte = the colour's high byte; X =
 * the new phase; DB = the pulled byte, N, Z from it; C set when the phase
 * wrapped.
 */
void abi_PaletteCycleFourth(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint16_t x;
    uint8_t v, db;

    cyc(24);
    x = rd16(b | 0xFD61);
    cyc(32);
    v = rd8(0x7EB6EB + x);
    cyc(30);
    wr8(b | (uint16_t)(0xFD8B + cpu.y), v);
    cyc(32);
    v = rd8(0x7EB6EC + x);
    cyc(30);
    wr8(b | (uint16_t)(0xFD8C + cpu.y), v);
    x = (uint16_t)(x + 2);
    if (x >= 0x0010) {
        cyc(18);
        cpu.p |= P_C;
        x = 0x0000;
    } else {
        cpu.p &= (uint8_t)~P_C;
    }
    cyc(98);
    wr16(b | 0xFD61, x);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.x = x;
    cpu.s++;
    cyc(20);
    db = rd8((uint16_t)cpu.s);
    cpu.db = db;
    cpu.p = nz8(cpu.p, db);
}
