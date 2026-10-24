/*
 * palette.c: colours the raster interrupt writes straight to CGRAM each
 * frame, and the colour cycling it steps. The palette buffer lives in work
 * RAM from $7E:FD65 (512 bytes); a few colours are kept apart at $7E:FF27.
 */
#include <stddef.h>

#include "machine.h"
#include "native.h"
#include "hw.h"
#include "video_palette_calls.h"
#include "carried.h"
#include "wram_names.h"


void CgramDma(uint16_t src, uint8_t colour, uint16_t size);


/* Write n bytes from src to CGRAM starting at colour c; return the last. */
static uint8_t cgram_put(uint8_t c, uint32_t src, unsigned n)
{
    uint8_t v = 0;
    unsigned i;

    cyc(40);
    ppu_wr(CGADD, c);
    for (i = 0; i < n; i++) {
        cyc(32);
        v = rd8(src + i);
        cyc(24);
        ppu_wr(CGDATA, v);
    }
    cyc(24);
    return v;
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
uint8_t RasterColours(void)
{
    cyc(46);
    if (rd8(W(palette_animate)) == 0)
        { cyc(22); return cgram_put(0x00, W(palette_buffer), 2); } else { cyc(16); }
    return cgram_put(0xE1, W(palette_kept), 6);
}

void ColourBlend_cr(Carried *cr);
void ColourBlendBy_cr(Carried *cr);
void ColourBlendToTarget_cr(Carried *cr);
void CoursePaletteCycle_cr(Carried *cr);
void FadeValueStep_cr(Carried *cr);
void HudColourRowsRest_cr(Carried *cr);
void HudColourStep_cr(Carried *cr);
void HudColourStepFrom14_cr(Carried *cr);
void PaletteCopy16To7F39D7_cr(Carried *cr);
void PaletteCourseLoad_cr(Carried *cr);
void PaletteCycle_cr(Carried *cr);
void PaletteCycleFourth_cr(Carried *cr);
void PaletteDarkenFour_cr(Carried *cr);
void PaletteFadeStep_cr(Carried *cr);
void PaletteKeptColoursStep_cr(Carried *cr);
void PaletteLoad16_cr(Carried *cr);
void PalettesStep_cr(Carried *cr);
void PlayerColoursFlash_cr(Carried *cr);
void RasterColours_cr(Carried *cr);
void RasterColoursCycling_cr(Carried *cr);
void RasterColoursE4_cr(Carried *cr);
void RasterColoursUnless0D22_cr(Carried *cr);
void RasterPaletteUpload_cr(Carried *cr);
void StatusColourPick_cr(Carried *cr);

void RasterColours_cr(Carried *cr)
{
    uint8_t v = RasterColours();

    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    cr->x &= 0x00FF;
    cr->y &= 0x00FF;
    set_p(cr, (uint8_t)(nz8(cr->p, v) | P_M | P_X));
    cyc(18);
}

void abi_RasterColours(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RasterColours_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
uint8_t RasterPaletteUpload(int *animating)
{
    *animating = 0;
    cyc(46);
    if (rd8(W(palette_animate)) == 0)
        { cyc(22); return cgram_put(0x00, W(palette_buffer), 2); } else { cyc(156); }
    CgramDma(0xFD65, 0x00, 0x0060);
    cyc(88);
    if (rd8(W(palette_animate)) == 0)
        return cgram_put(0x00, W(palette_buffer), 2); else { cyc(156); }
    *animating = 1;
    CgramDma(0xFEC5, 0xB0, 0x0020);
    return (cyc(42), RasterColours());
}

void RasterPaletteUpload_cr(Carried *cr)
{
    int animating;
    uint8_t v = RasterPaletteUpload(&animating);

    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    set_p(cr, (uint8_t)(nz8(cr->p, v) | P_M));
    if (animating) {
        cr->x = 0xB0;
        cr->y = 0x20;
        set_p(cr, (uint8_t)(cr->p | (P_X)));
    }
    cyc(18);
}

void abi_RasterPaletteUpload(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RasterPaletteUpload_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
    cyc(22);
    cgram_put(0xE1, W(palette_kept), 6);
    if (!(rd8(W(frame_count)) & 0x01))
        { cyc(32); return cyc_then(18, cgram_put(0x10, (0x7E0000 | W(cycle_colour_a) + x) & 0xFFFFFF, 4)); } else { cyc(38); }
    return cyc_then(18, cgram_put(0x12, (0x7E0000 | W(cycle_colour_c) + x) & 0xFFFFFF, 4));
}

void RasterColoursCycling_cr(Carried *cr)
{
    uint8_t v = RasterColoursCycling(cr->x);

    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    set_p(cr, (uint8_t)(nz8(cr->p, v) | P_M));
}

void abi_RasterColoursCycling(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RasterColoursCycling_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
    return cyc_then(18, cgram_put(0xE4, (W(wram_7EFF2D) + x) & 0xFFFFFF, 12));
}

void RasterColoursE4_cr(Carried *cr)
{
    uint8_t v = RasterColoursE4(cr->x);

    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    set_p(cr, nz8(cr->p, v));
}

void abi_RasterColoursE4(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RasterColoursE4_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* What PaletteCycle's exit leaves in C and V, for the adapter only: C from
   the last phase test (set when the phase wrapped); V from the last step
   that ran the backwards table index, -1 when none did. */
static int cycle_c, cycle_v;

/* One colour-cycling step: the phase counts 0..15; phases 8-15 run the table
   backwards (15 - phase), so the colour swings up and down. Returns the new
   phase; *x the table offset used. */
static uint16_t cycle_step(uint32_t phase_addr, uint32_t table, uint32_t dest, uint16_t *x)
{
    cyc(24);
    uint16_t phase = rd16(phase_addr), i = phase, next;

    if (phase >= 8) {
        cyc(72);
        i = (uint16_t)(0x000F - phase);
        cycle_v = ((0x000F ^ phase) & (0x000F ^ i) & 0x8000) != 0;
    }
    *x = (uint16_t)(i << 1);
    cyc(106);
    wr16(dest, cyc_then(30, rd16(table + *x)));
    next = (uint16_t)(phase + 1);
    if (next >= 0x0010) {
        cyc(18);
        next = 0;
        cycle_c = 1;
    } else {
        cycle_c = 0;
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
uint16_t PaletteCycle(uint16_t *x)
{
    cycle_v = -1;
    cyc(188);
    if (!(rd8(W(frame_count)) & 0x01)) {
        cyc(54);
        cycle_step(W(wram_7EFD5B), W(wram_7EB71B), (0x7E0000 | W(cycle_colour_a)), x);
        return cycle_step(W(wram_7EFD5D), W(wram_7EB6FB), (0x7E0000 | W(cycle_colour_b)), x);
    } else { cyc(60); }
    return cycle_step(W(wram_7EFD5F), W(wram_7EB70B), (0x7E0000 | W(cycle_colour_c)), x);
}

void PaletteCycle_cr(Carried *cr)
{
    uint16_t x;

    cr->a = PaletteCycle(&x);
    cr->x = x;
    cr->y = 0;
    set_p(cr, (uint8_t)(nz8(cr->p, cr->db) & ~(P_M | P_X | P_C)));
    if (cycle_c)
        set_p(cr, (uint8_t)(cr->p | (P_C)));
    if (cycle_v >= 0)
        set_p(cr, (uint8_t)((cr->p & ~P_V) | (cycle_v ? P_V : 0)));
}

void abi_PaletteCycle(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PaletteCycle_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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

    cyc(78);
    wr16(W(dp_f1), 0x0010);
    for (i = 0; i < 16; i++) {
        cyc(78);
        c = rd16((uint32_t)bank << 16 | (uint16_t)(src + 2 * i));
        wr16(W(palette_work_a) + 2 * i, c);
        cyc(32);
        wr16(W(palette_work_b) + 2 * i, c);
        cyc(32);
        wr16(W(palette_buffer) + 2 * i, c);
        cyc(72);
        n = rd16(W(dp_f1));
        cyc(14);
        wr16(W(dp_f1), (uint16_t)(n - 1));
        cyc(22);
    }
    cyc(36);
    return c;
}

void PaletteLoad16_cr(Carried *cr)
{
    uint16_t src = cr->a;

    cr->a = PaletteLoad16(cr->db, src);
    cr->x = 0x0020;
    cr->y = (uint16_t)(src + 0x20);
    set_p(cr, (uint8_t)((cr->p & ~P_N) | P_Z));
}

void abi_PaletteLoad16(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PaletteLoad16_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
    uint8_t v = rd8(0x7E0000 | W(wram_0D22));

    *ran = !v;
    if (v)
        { cyc(22); return v; } else { cyc(108); }
    RasterColoursE4(0);
    cyc(46);
    return RasterColoursCycling(0);
}

void RasterColoursUnless0D22_cr(Carried *cr)
{
    int ran;
    uint8_t v = RasterColoursUnless0D22(cr->db, &ran);

    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    if (ran)
        cr->x = 0;
    cyc(70);
}

void abi_RasterColoursUnless0D22(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RasterColoursUnless0D22_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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

    cyc(32);
    wr16(W(wram_7F345C), v);
    for (i = 0; i < 4; i++) {
        cyc(32);
        v = (uint16_t)(rd16(W(wram_7F37F7) + 2 * i) - 1);
        cyc(46);
        wr16(W(wram_7F39F7) + 2 * i, v);
    }
    return v;
}

void PaletteDarkenFour_cr(Carried *cr)
{
    cr->a = PaletteDarkenFour(cr->a);
    set_p(cr, nz16(cr->p, cr->a));
    cyc(44);
}

void abi_PaletteDarkenFour(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PaletteDarkenFour_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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

    cyc(78);
    wr16(W(dp_f1), 0x0010);
    for (i = 0; i < 16; i++) {
        cyc(78);
        w = rd16((uint32_t)bank << 16 | (uint16_t)(src + 2 * i));
        wr16(W(palette_work_c) + 2 * i, w);
        cyc(72);
        n = rd16(W(dp_f1));
        cyc(14);
        wr16(W(dp_f1), (uint16_t)(n - 1));
        cyc(22);
    }
    cyc(36);
    return w;
}

void PaletteCopy16To7F39D7_cr(Carried *cr)
{
    uint16_t src = cr->a;

    cr->a = PaletteCopy16To7F39D7(cr->db, src);
    cr->y = (uint16_t)(src + 0x20);
    cr->x = 0x0020;
    set_p(cr, (uint8_t)((cr->p & ~P_N) | P_Z));
}

void abi_PaletteCopy16To7F39D7(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PaletteCopy16To7F39D7_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
    uint16_t c = rd16(0x7E0000 | W(course)), i, x, y, w, d, s;
    uint8_t blk;
    unsigned k;

    s = (uint16_t)((uint16_t)(c << 1) + rd16(cyc_then(52, 0x7E0000 | W(course))));
    *p = (uint8_t)((*p & ~P_V) | ((~((uint16_t)(c << 1) ^ c) & ((uint16_t)(c << 1) ^ s) & 0x8000) ? P_V : 0));
    i = (uint16_t)(s & 0x00FF);
    cyc(208);
    blk = rd8(0x03F64B + i);
    y = rd16(0x03F649 + i);
    for (x = 0x0020; x < 0x0200; x = (uint16_t)(x + 2)) {
        src = ((uint32_t)blk << 16) + 0x0020 + y;
        cyc(180);
        w = rd16(src & 0xFFFFFF);
        wr16(W(palette_work_a) + x, w);
        cyc(32);
        wr16(W(palette_work_b) + x, w);
        cyc(32);
        wr16(W(palette_buffer) + x, w);
        y = (uint16_t)(y + 2);
    }
    cyc(330);
    blk = rd8(0x03F69F + i);
    y = rd16(0x03F69D + i);
    for (x = 0x0020; x < 0x0200; x = (uint16_t)(x + 2)) {
        src = ((uint32_t)blk << 16) + 0x0020 + y;
        cyc(180);
        wr16(W(palette_work_c) + x, rd16(src & 0xFFFFFF));
        y = (uint16_t)(y + 2);
    }
    blk = rd8(0x03F6F3 + i);
    PaletteLoad16(blk, cyc_then(462, rd16(0x03F6F1 + i)));
    blk = rd8(0x03F747 + i);
    PaletteCopy16To7F39D7(blk, cyc_then(314, rd16(0x03F745 + i)));
    x = 0x01FE;
    cyc(24);
    for (;;) {
        uint16_t n = (uint16_t)((x >> 1) & 0x000F), bits;
        int shifted;

        y = (uint16_t)((x >> 4) & 0x001E);
        cyc(226);
        wr16(W(dp_f1), n);
        cyc(70);
        bits = rd16(b | (uint16_t)(0xEF1F + y));
        shifted = n != 0;
        while (n) {
            bits >>= 1;
            cyc(52);
            n = (uint16_t)(rd16(W(dp_f1)) - 1);
            cyc(14);
            wr16(W(dp_f1), n);
        }
        /* to the LDA long: BIT and BNE (6 more taken), and with no shift
           loop the table's ROM read and BRA before them */
        cyc((shifted ? 88 : 110) + (bits & 1 ? 6 : 0));
        w = rd16(W(palette_work_b) + x);
        cyc(32);
        d = rd16(W(palette_work_c) + x);
        *p = (uint8_t)((*p & ~P_C) | (w >= d ? (cyc(22), P_C) : (cyc(22), 0)));
        if (!(bits & 1)) {
            if (w == d)
                { cyc(168); wr16(W(palette_work_c) + x, (uint16_t)(w & 0x001F ? w - 1 : (cyc(6), w + 1))); }
        } else if (w != d) {
            cyc(26);
            wr16(W(palette_work_c) + x, w);
        }
        if (!x)
            break;
        x = (uint16_t)(x - 2);
    }
    cyc(92);
    wr16(0x7E0000 | W(blend_set_a), 0x45CC);
    cyc(48);
    wr16(0x7E0000 | W(wram_0E23), 0x2528);
    cyc(48);
    wr16(0x7E0000 | W(wram_0E25), 0x1CE5);
    cyc(48);
    wr16(0x7E0000 | W(blend_set_b), 0x739A);
    cyc(48);
    wr16(0x7E0000 | W(wram_0E29), 0x1CE5);
    cyc(48);
    wr16(0x7E0000 | W(wram_0E2B), 0x2528);
    cyc(24);
    w = rd16(0x7E0000 | W(wram_10DC));
    if (!w) {
        *xo = 0xFFFE;
        *yo = y;
        *p = (uint8_t)((nz16(*p, 0) & ~(P_M | P_X)));
        return 0;
    } else { cyc(40); }
    for (k = 0; k < 0x20; k++) {
        cyc(92);
        wr8(W(wram_7EFEC5) + k, 0x00);
        cyc(32);
        wr8(W(wram_7F3937) + k, 0x00);
        cyc(32);
        wr8(W(wram_7F3B37) + k, 0x00);
        cyc(32);
        wr8(W(wram_7F3737) + k, 0x00);
    }
    cyc(32);
    *xo = 0x0020;
    *yo = y;
    *p = (uint8_t)((nz8(*p, 0) | P_M | P_C) & ~P_X);
    return (uint16_t)(w & 0xFF00);
}

void PaletteCourseLoad_cr(Carried *cr)
{
    cr->a = PaletteCourseLoad(cr->db, &cr->x, &cr->y, &cr->p);
    cyc(64);
}

void abi_PaletteCourseLoad(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PaletteCourseLoad_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

uint16_t PpuMultiply(uint8_t v);

/* The flags ColourBlend's exit leaves in C and V (both clear first), for the
   adapters only. */
static uint8_t blend_cv;

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
    uint16_t r, g, bl, m, s, t, f, a;
    uint8_t cv = 0;

    blend_split(0x7F0000 | W(colour_from_r), W(dp_f1));
    blend_split(0x7F0000 | W(colour_from_g), W(dp_f1));
    blend_split(0x7F0000 | W(colour_from_b), W(dp_f1));
    blend_split(0x7F0000 | W(colour_to_r), W(dp_f3));
    blend_split(0x7F0000 | W(colour_to_g), W(dp_f3));
    blend_split(0x7F0000 | W(colour_to_b), W(dp_f3));
    cyc(24);
    t = rd16(0x7F0000 | W(colour_to_r));
    cyc(38);
    f = rd16(0x7F0000 | W(colour_from_r));
    cyc(46);
    m = PpuMultiply((uint8_t)(t - f));
    cyc(80);
    f = rd16(0x7F0000 | W(colour_from_r));
    r = (uint16_t)((m + f) & 0x1F);
    cyc(48);
    wr16(0x7F0000 | W(colour_from_r), r);
    cyc(24);
    t = rd16(0x7F0000 | W(colour_to_g));
    cyc(38);
    f = rd16(0x7F0000 | W(colour_from_g));
    cyc(46);
    m = PpuMultiply((uint8_t)(t - f));
    cyc(80);
    f = rd16(0x7F0000 | W(colour_from_g));
    g = (uint16_t)(m + f);
    cyc(24);
    wr16(0x7F0000 | W(colour_from_g), g);
    cyc(24);
    t = rd16(0x7F0000 | W(colour_to_b));
    cyc(38);
    f = rd16(0x7F0000 | W(colour_from_b));
    cyc(46);
    m = PpuMultiply((uint8_t)(t - f));
    cyc(80);
    bl = rd16(0x7F0000 | W(colour_from_b));
    s = (uint16_t)(m + bl);
    if (~(m ^ bl) & (m ^ s) & 0x8000)
        cv |= P_V;
    bl = (uint16_t)((uint16_t)((uint16_t)(s >> 8 | s << 8) << 2) & 0x7C00);
    cyc(88);
    wr16(W(dp_f1), bl);
    cyc(24);
    g = rd16(0x7F0000 | W(colour_from_g));
    if (g & 0x0800)
        cv |= P_C;
    cyc(110);
    bl = rd16(W(dp_f1));
    cyc(24);
    r = rd16(0x7F0000 | W(colour_from_r));
    a = (uint16_t)(((g << 5) & 0x03E0) | bl | r);
    blend_cv = cv;
    cyc(42);                                    /* RTS */
    return a;
}

void ColourBlend_cr(Carried *cr)
{
    uint16_t a = ColourBlend(cr->db);

    cr->a = a;
    set_p(cr, (uint8_t)(nz16((uint8_t)((cr->p & ~(P_V | P_C)) | blend_cv), a) & ~P_M));
}

void abi_ColourBlend(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ColourBlend_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* Steps a 13-entry cycle position by n, wrapping. */
static uint8_t cycle13(uint8_t v, uint8_t n)
{
    v = (uint8_t)(v + n);
    return v < 0x0D ? v : (cyc(10), (n == 1 ? 0 : (uint8_t)(v - 0x0D)));
}

/* What PaletteKeptColoursStep's exit leaves in V (the subtract's overflow),
   for the adapter only. */
static int kept_v;

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
 * In C: a is A on entry (the no-step case keeps its high byte); *xo the last
 * cycle step * 2 when a step was made, else x; *stepped whether the first two
 * colours stepped. Returns A.
 */
uint16_t PaletteKeptColoursStep(uint8_t bank, uint16_t a, uint16_t x, uint16_t y, uint16_t *xo, int *stepped)
{
    uint8_t t, step = 0, v, d;
    unsigned s;
    int up;
    uint8_t cf = 0;
    enum { NONE, SKIP, ONE, TWO, ODD } go = NONE;

    cyc(102);
    if (rd8(0x7E0000 | W(wram_0D53)))
        up = 0, cyc(68);                /* BNE taken, SEP, LDA */
    else if (!rd8(cyc_then(40, 0x7E0000 | (uint16_t)(W(kept_enable) + y))))
        up = 0, cyc(68);                /* BEQ taken, SEP, LDA */
    else {
        a = rd16(cyc_then(62, 0x7E0000 | (uint16_t)(W(joy1_last) + x)));
        up = (a & 0x4000) != 0;
        if (!up)
            cyc(92);                    /* BIT, BEQ taken, SEP, LDA */
    }
    if (up) {
        cyc(86);
        t = rd8(0x7E0000 | (uint16_t)(W(kept_timer) + y));
        if (t == 0xFF) {
            go = TWO;
        } else {
            t++;
            cyc(76);
            wr8(0x7E0000 | (uint16_t)(W(kept_timer) + y), t);
            go = t >= 0xB4 ? TWO : t >= 0x78 ? ONE : t >= 0x3C ? ODD : SKIP;
        }
    } else {
        t = rd8(0x7E0000 | (uint16_t)(W(kept_timer) + y));
        if (!t) {
            a = 0x7E63;
            cyc(100);
            wr16(W(kept_colour_c), a);
        } else if (t >= 0x78) {
            wr8(0x7E0000 | (uint16_t)(W(kept_timer) + y), (uint8_t)(t - 5));
            go = TWO;
        } else if (t >= 0x3C) {
            wr8(0x7E0000 | (uint16_t)(W(kept_timer) + y), (uint8_t)(t - 4));
            go = ONE;
        } else if (t >= 0x1E) {
            wr8(0x7E0000 | (uint16_t)(W(kept_timer) + y), (uint8_t)(t - 3));
            go = ODD;
        } else {
            wr8(0x7E0000 | (uint16_t)(W(kept_timer) + y), (uint8_t)(t - 1));
            go = SKIP;
        }
    }
    /* SKIP steps by one on every fourth frame, ODD on every other one. */
    if (go == SKIP)
        { cyc(120); go = (rd8(0x7E0000 | W(frame_count)) & 0x03) ? (cyc(38), NONE) : ONE; }
    else if (go == ODD)
        go = (rd8(0x7E0000 | W(frame_count)) & 0x01) ? NONE : ONE;
    if (go == ONE || go == TWO) {
        step = cycle13(rd8(0x7E0000 | (uint16_t)(W(kept_cycle_c) + y)), go == TWO ? 2 : 1);
        wr8(0x7E0000 | (uint16_t)(W(kept_cycle_c) + y), step);
        x = (uint8_t)(step << 1);
        a = rd16(W(kept_colours_c) + x);
        wr16(W(kept_colour_c), a);
    }
    cyc(46);
    v = rd8(0x7E0000 | (uint16_t)(W(kept_acc) + y));
    cyc(38);
    d = rd8(0x7E0000 | (uint16_t)(W(kept_sub) + y));
    s = (unsigned)v - d;
    cf = s < 0x100;
    kept_v = ((v ^ d) & (v ^ (uint8_t)s) & 0x80) != 0;
    cyc(30);
    wr8(0x7E0000 | (uint16_t)(W(kept_acc) + y), (uint8_t)s);
    *stepped = !cf;
    if (cf) {
        cyc(22);
        a = (uint16_t)((a & 0xFF00) | (uint8_t)s);
    } else {
        cyc(40 + (((0x0DFF + y) ^ 0x0DFF) & 0xFF00 ? 6 : 0));  /* BCS, LDA abs,Y */
        step = cycle13(rd8(0x7E0000 | (uint16_t)(W(kept_cycle_ab) + y)), 1);
        cyc(82);
        wr8(0x7E0000 | (uint16_t)(W(kept_cycle_ab) + y), step);
        x = (uint8_t)(step << 1);
        cyc(82);
        wr16(W(palette_kept), cyc_then(32, rd16(W(kept_colours_a) + x)));
        cyc(32);
        a = rd16(W(kept_colours_b) + x);
        cyc(32);
        wr16(W(kept_colour_b), a);
    }
    cyc(42);
    *xo = x;
    return a;
}

void PaletteKeptColoursStep_cr(Carried *cr)
{
    int stepped;
    uint16_t x;
    uint16_t a = PaletteKeptColoursStep(cr->db, cr->a, cr->x, cr->y, &x, &stepped);
    uint8_t p = (uint8_t)(cr->p & ~(P_C | P_V));

    if (kept_v)
        p |= P_V;
    if (stepped)
        set_p(cr, (uint8_t)(nz16(p, a) & ~(P_C | P_M)));
    else
        set_p(cr, (uint8_t)(nz8((uint8_t)(p | P_C), (uint8_t)a) | P_M));
    cr->a = a;
    cr->x = x;
}

void abi_PaletteKeptColoursStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PaletteKeptColoursStep_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
    cyc(16);
    PpuMultiplierSetLowByteTimes8((uint8_t)rd16(W(dp_f5)));
    cyc(22);
    return ColourBlend(bank);
}

void ColourBlendBy_cr(Carried *cr)
{
    uint16_t a = ColourBlendBy(cr->db);

    cr->a = a;
    set_p(cr, (uint8_t)(nz16((uint8_t)((cr->p & ~(P_V | P_C)) | blend_cv), a) & ~P_M));
}

void abi_ColourBlendBy(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ColourBlendBy_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

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
    uint16_t t, v;

    cyc(16);
    wr16(W(dp_f1), colour);
    cyc(24);
    t = rd16(0x7F0000 | W(blend_target));
    if (t & 0x8000)
        { cyc(32); t = rd16(W(dp_f1)); } else { cyc(22); }
    cyc(16);
    wr16(W(dp_f3), t);
    cyc(24);
    t = rd16(0x7F0000 | W(blend_target_amount));
    cyc(16);
    wr16(W(dp_f5), t);
    cyc(46);
    v = ColourBlendBy(bank);
    cyc(32);
    wr16(W(blend_set_a) + x, v);
    cyc(42);
    return v;
}

void ColourBlendToTarget_cr(Carried *cr)
{
    uint16_t a = ColourBlendToTarget(cr->db, cr->a, cr->x);

    cr->a = a;
    set_p(cr, (uint8_t)(nz16((uint8_t)((cr->p & ~(P_V | P_C)) | blend_cv), a) & ~P_M));
}

void abi_ColourBlendToTarget(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ColourBlendToTarget_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* What PlayerColoursFlash's exit leaves in V, for the adapter only: -1 the
   entry's, else 0 or 1. */
static int flash_v;

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
 * In C: returns A; *xo is X.
 */
uint16_t PlayerColoursFlash(uint8_t bank, uint8_t level, uint16_t y, uint16_t *xo)
{
    uint32_t b = (uint32_t)bank << 16;
    uint8_t k, n, c, v = level;
    uint16_t src, x, a;

    flash_v = -1;
    if (v >= 0x08) {
        unsigned s = (unsigned)v + 0x60;

        flash_v = (~(v ^ 0x60) & (v ^ s) & 0x80) != 0;
        v = s > 0xFF ? (cyc(10), 0xFF) : (uint8_t)s;
    }
    k = (uint8_t)(v >> 5);
    src = 0x0E21;
    if (!k) {
        cyc(178);
        wr8(0x7E0000 | (uint16_t)(W(flash_count) + y), 0x00);
    } else {
        n = (uint8_t)(8 - k);
        flash_v = 0;
        cyc(292);
        wr8(W(dp_f9), n);
        cyc(30);
        wr8(W(flash_2n), (uint8_t)(n << 1));
        cyc(30);
        c = rd8(0x7E0000 | (uint16_t)(W(flash_count) + y));
        if (c >= (uint8_t)(n << 1))
            { cyc(10); c = 0xFF; }
        c = (uint8_t)(c + 1);
        cyc(90);
        wr8(0x7E0000 | (uint16_t)(W(flash_count) + y), c);
        if (c < n)
            { cyc(6); src = 0x0E27; }
        cyc(18);
    }
    cyc(116);
    x = rd16(0x7E0000 | (uint16_t)(W(player_pal_offset) + (uint16_t)(y << 1)));
    cyc(38);
    wr16(W(player_palettes) + x, cyc_then(32, rd16(b | src)));
    cyc(24);
    wr16((0x7E0000 | W(wram_01CA)) + x, cyc_then(32, rd16(b | (uint16_t)(src + 2))));
    cyc(24);
    a = rd16(b | (uint16_t)(src + 4));
    cyc(32);
    wr16((0x7E0000 | W(wram_01CC)) + x, a);
    cyc(42);
    *xo = x;
    return a;
}

void PlayerColoursFlash_cr(Carried *cr)
{
    uint16_t y = cr->y, x;
    uint16_t a = PlayerColoursFlash(cr->db, (uint8_t)cr->a, y, &x);
    uint8_t p = cr->p;

    if (flash_v >= 0)
        p = (uint8_t)((p & ~P_V) | (flash_v ? P_V : 0));
    p = (uint8_t)((p & ~P_C) | ((y & 0x8000) ? P_C : 0));
    cr->a = a;
    cr->x = x;
    cr->y = (uint16_t)(y << 1);
    set_p(cr, (uint8_t)(nz16(p, a) & ~(P_M | P_X)));
}

void abi_PlayerColoursFlash(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PlayerColoursFlash_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 7E:B5E3 PlayerColoursFlashRts
 * WHAT IT DOES: An RTS inside PlayerColoursFlash that nothing branches to.
 * HOW: Returns.
 * Entry: any. Exit: all unchanged.
 */
void PlayerColoursFlashRts(void)
{
    cyc(42);
}

/* PlayerColoursFlashRts on carried registers: no register work. */
void PlayerColoursFlashRts_cr(Carried *cr)
{
    (void)cr;
    PlayerColoursFlashRts();
}

void abi_PlayerColoursFlashRts(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PlayerColoursFlashRts_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* What StatusColourPick's exit leaves in C, for the adapter only: whether
   the entry's. */
static int status_c_kept;

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
 * In C: returns A; *stored is whether the colour was stored (then X = 0).
 */
uint16_t StatusColourPick(uint8_t bank, uint16_t y, int *stored)
{
    uint16_t flags, t = 0, a, x;
    uint8_t v;
    int table = 0;

    *stored = 1;
    status_c_kept = 1;
    cyc(30);
    v = rd8(0x7E0000 | (uint16_t)(W(wram_0DDA) + y));
    if (v == 0) {
        cyc(68);
        a = 0x0000;
        goto store;
    }
    cyc(46);
    v = rd8(0x7E0000 | (uint16_t)(W(player_rec_43) + y));
    flags = (uint16_t)(v << 8);
    cyc(50);
    flags |= rd8(0x7E0000 | (uint16_t)(W(player_rec_42) + y));
    if (flags & 0x0400) {
        cyc(150);
        v = rd8(0x7E0000 | (uint16_t)(W(kept_enable) + y));
        if (v) {
            cyc(104);
            t = rd16(0x7E0000 | W(frame_count));
            x = (uint16_t)((t & 0x0003) << 1);
            status_c_kept = 0;
            cyc(84);
            a = rd16((uint32_t)(W(status_colours_b) + x));
            goto store;
        } else { cyc(12); }
    }
    cyc(86);
    if (flags & 0x0010) {
        cyc(74);
        v = rd8(0x7E0000 | (uint16_t)(W(wram_0DDA) + y));
        status_c_kept = 0;
        if (v >= 0x3C) {
            cyc(78);
            t = rd16(0x7E0000 | W(frame_count));
            table = 1;
        }
    }
    if (!table) {
        cyc(84);
        t = rd16(0x7E0000 | W(frame_count));
        if (t & 0x0001) {
            cyc(88);
            *stored = 0;
            return t;
        } else { cyc(32); }
        status_c_kept = 0;
        t = (uint16_t)(t >> 1);
    }
    x = (uint16_t)((t & 0x0003) << 1);
    status_c_kept = 0;
    cyc(106);
    a = rd16((uint32_t)(W(status_colours_a) + x));
store:
    cyc(78);
    wr16(W(status_colour), a);
    cyc(42);
    return a;
}

void StatusColourPick_cr(Carried *cr)
{
    int stored;
    uint16_t a = StatusColourPick(cr->db, cr->y, &stored);
    uint8_t p = (uint8_t)(cr->p & ~P_M);

    if (!status_c_kept)
        p &= (uint8_t)~P_C;
    cr->a = a;
    if (stored)
        cr->x = 0x0000;
    set_p(cr, nz16(p, stored ? 0 : a));
}

void abi_StatusColourPick(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    StatusColourPick_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 7E:B3F4 PalettesStep
 * WHAT IT DOES: While $0E5F is set, steps the frame's palette effects for
 * player 0: its colours (PlayerColoursFlash with A = $0DDA), its status
 * colour (StatusColourPick), the cycling colours (PaletteCycle) and the
 * kept colours (PaletteKeptColoursStep).
 * HOW: Y = 0 for each; M 8-bit, X 16-bit, then both 8-bit for the last.
 * Entry: any widths. Exit: P restored; X and Y as the last call left them.
 * In C: returns A as the last call left it (its low byte 0 when *ran is 0,
 * the rest of A as on entry); *xo is X; X and Y are as on entry when *ran
 * is 0, else Y is 0.
 */
uint16_t PalettesStep(uint8_t bank, int *ran, uint16_t *xo)
{
    uint16_t a = 0, x = 0;
    int stored, stepped;
    uint8_t level;

    cyc(90);
    *ran = rd8(0x7E0000 | W(palette_animate)) != 0;
    if (*ran) {
        cyc(64);
        level = rd8(0x7E0000 | W(wram_0DDA));
        cyc(46);
        PlayerColoursFlash(bank, level, 0, &x);
        cyc(114);
        StatusColourPick(bank, 0, &stored);
        cyc(46);
        a = PaletteCycle(&x);
        cyc(22);                                /* SEP #$30 */
        cyc(70);
        a = PaletteKeptColoursStep(bank, a, 0, 0, &x, &stepped);
    } else { cyc(22); }
    cyc(28);
    cyc(42);                                    /* PLP */
    *xo = x;
    return a;
}

void PalettesStep_cr(Carried *cr)
{
    uint8_t p0 = cr->p;
    uint16_t a0 = cr->a, x;
    int ran;
    uint16_t a = PalettesStep(cr->db, &ran, &x);

    if (ran) {
        cr->a = a;
        cr->x = x;
        cr->y = 0x0000;
    } else {
        cr->a = (uint16_t)(a0 & 0xFF00);
    }
    set_p(cr, p0);
    if (p0 & P_X) {
        cr->x &= 0x00FF;
        cr->y &= 0x00FF;
    }
}

void abi_PalettesStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PalettesStep_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* CLC / ADC and SEC / SBC (16-bit, binary): the result, with the overflow
   in *v (and for the subtract the carry, set when it did not borrow, in
   *c) when they are not NULL. The subtract's clocks follow the 65816
   core's: 6 more than the add. */
static uint16_t add16(uint16_t a, uint16_t m, int *v)
{
    unsigned r = (unsigned)a + m;

    if (v)
        *v = (~(a ^ m) & (a ^ r) & 0x8000) != 0;
    return (uint16_t)r;
}

static uint16_t sub16(uint16_t a, uint16_t m, int *c, int *v)
{
    uint16_t n = (uint16_t)~m;
    unsigned r = (unsigned)a + n + 1;

    if (v)
        *v = (~(a ^ n) & (a ^ r) & 0x8000) != 0;
    if (c)
        *c = r > 0xFFFF;
    cyc(6);
    return (uint16_t)r;
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
 * In C: a is A on entry; returns A.
 */
uint16_t FadeValueStep(uint16_t a)
{
    uint16_t w, m;
    uint8_t v;

    cyc(148);
    v = (uint8_t)(rd8(W(slide_flags)) & 0x80);
    a = (uint16_t)((a & 0xFF00) | v);
    if (!v) {
        cyc(64);
        v = (uint8_t)(rd8(W(slide_flags)) & 0x20);
        a = (uint16_t)((a & 0xFF00) | v);
        if (v) {
            cyc(78);
            w = rd16(0x7E0000 | W(wram_19FE));
            cyc(38);
            w = add16(w, rd16(0x7E0000 | W(wram_1A00)), NULL);
            a = w;
            cyc(24);
            m = rd16(0x7E0000 | W(wram_1A02));
            cyc(0);
            if (w != m)
                { cyc(40); wr16(0x7E0000 | W(wram_19FE), w); cyc(24); }  /* JMP .show */
            else {
                cyc(76);
                v = (uint8_t)(rd8(W(slide_flags)) & 0xDF);
                cyc(48);
                wr8(W(slide_flags), v);
            }
            cyc(46);                    /* SEP, LDA abs */
            v = rd8(0x7E0000 | W(wram_19FF));
            a = (uint16_t)((a & 0xFF00) | v);
            cyc(32);
            wr8(W(wram_7F3460), v);
        } else { cyc(38); }
    } else { cyc(38); }
    cyc(58);
    cyc(42);                            /* PLP */
    return a;
}

void FadeValueStep_cr(Carried *cr)
{
    cr->a = FadeValueStep(cr->a);
    if (cr->p & P_X) {
        cr->x &= 0x00FF;
        cr->y &= 0x00FF;
    }
}

void abi_FadeValueStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    FadeValueStep_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* What PaletteFadeStep's exit leaves in C and V, for the adapter only: C
   from the last part's compare; V from the last add or subtract of a step,
   -1 when none ran. */
static int fade_c, fade_v;

/* One colour part of PaletteFadeStep: the part (mask m) of the colour at
   $0722+Y moved one step toward the same part at $02:D65A+X. */
static uint16_t fade_part(uint32_t b, uint16_t m, uint16_t step, uint16_t x, uint16_t y)
{
    uint16_t a, c;

    cyc(30);
    a = rd16(0x7E0000 | (uint16_t)(W(crash_scroll) + 9 + y));
    cyc(2);
    a &= m;
    cyc(46);
    wr16(0x7E0000 | W(wram_1F2B), a);
    cyc(94);
    a = rd16(0x02D65A + (uint32_t)x);
    cyc(2);
    a &= m;
    c = rd16(0x7E0000 | W(wram_1F2B));
    cyc(0);
    fade_c = a >= c;
    if (a == c)
        { cyc(8); return a; }
    if (a > c) {
        cyc(62);
        a = rd16(0x7E0000 | W(wram_1F2B));
        if (step == 1) {
            a++;
        } else {
            cyc(24);
            a = add16(a, step, &fade_v);
        }
    } else {
        cyc(56);
        a = rd16(0x7E0000 | W(wram_1F2B));
        if (step == 1) {
            a--;
        } else {
            cyc(18);
            a = sub16(a, step, NULL, &fade_v);
        }
        cyc(22);
    }
    return a;
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
 * In C: returns whether the count was nonzero (the colours were stepped);
 * then *xo and *yo are X and Y at the end.
 */
int PaletteFadeStep(uint8_t bank, uint16_t *xo, uint16_t *yo)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t n, a, x, y, t;
    uint8_t v;

    fade_v = -1;
    cyc(46);
    v = rd8(0x7E0000 | W(wram_0CFA));
    if (v == 0)
        { cyc(60); return 0; }
    v = (uint8_t)(v - 1);
    cyc(60);
    wr8(0x7E0000 | W(wram_0CFA), v);
    cyc(22);                            /* REP #$30 */
    cyc(24);
    x = rd16(0x7E0000 | W(frame_step));
    cyc(24);
    y = rd16(0x7E0000 | W(frame_step));
    cyc(24);
    a = rd16(0x7E0000 | W(wram_1167));
    for (;;) {
        cyc(30);
        n = a;
        t = fade_part(b, 0x7C00, 0x0400, x, y);
        cyc(30);
        wr16(W(dp_3e), t);
        t = fade_part(b, 0x03E0, 0x0020, x, y);
        cyc(30);
        wr16(W(dp_40), t);
        a = fade_part(b, 0x001F, 1, x, y);
        cyc(30);
        a |= rd16(W(dp_3e));
        cyc(16);
        a |= rd16(W(dp_40));
        cyc(30);
        wr16(0x7E0000 | (uint16_t)(W(crash_scroll) + 9 + y), a);
        y = (uint16_t)(y + 2);
        x = (uint16_t)(x + 2);
        a = (uint16_t)(n - 1);
        if (!a)
            break;
        cyc(152);               /* INY, INY, INX, INX, PLA, DEC, BEQ, BRL */
    }
    cyc(172);
    *xo = x;
    *yo = y;
    return 1;
}

void PaletteFadeStep_cr(Carried *cr)
{
    uint16_t x, y;
    uint8_t p;

    if (!PaletteFadeStep(cr->db, &x, &y)) {
        cr->a &= 0xFF00;
        set_p(cr, (uint8_t)((cr->p | P_M | P_Z) & ~P_N));
        return;
    }
    p = (uint8_t)(cr->p & ~(P_N | P_M | P_X | P_C | P_V));
    p |= P_Z;
    if (fade_c)
        p |= P_C;
    if (fade_v > 0 || (fade_v < 0 && (cr->p & P_V)))
        p |= P_V;
    cr->a = 0x0000;
    cr->x = x;
    cr->y = y;
    set_p(cr, p);
}

void abi_PaletteFadeStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PaletteFadeStep_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* LDA $0D30, INC, back to 0 at $16, STA $0D30, ASL, TAX (M 8-bit): the
   cycle's next step and its table offset (with X 16-bit, TAX also takes
   A's high byte). Returns A; *x is the offset. */
static uint16_t palette_cycle_step(uint32_t b, uint16_t a, int index8, uint16_t *x)
{
    cyc(56);
    uint8_t v = (uint8_t)(rd8(0x7E0000 | W(wram_0D30)) + 1);

    if (v >= 0x16)
        { cyc(10); v = 0x00; }
    cyc(76);
    wr8(0x7E0000 | W(wram_0D30), v);
    v = (uint8_t)(v << 1);
    a = (uint16_t)((a & 0xFF00) | v);
    cyc(28);
    *x = index8 ? v : a;
    return a;
}

/* Whether a read at `a` is of work RAM, a register or the Super FX's RAM
   (the low half of the banks below $40, or banks $70, $71, $7E, $7F), which
   the lockstep places on the original's own access: the clocks of its bytes
   are then not part of the interval, as a ROM read's are. */
static int placed_read(uint32_t a)
{
    uint32_t bank = a >> 16 & 0xFF, lo = a & 0xFFFF;

    return bank == 0x70 || bank == 0x71 || bank == 0x7E || bank == 0x7F || ((bank & 0x40) == 0 && lo < 0x8000);
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
 * In C: a is A on entry; index8 whether X is 8-bit; returns A; *xo is set
 * to the step's offset when the course has one (else left as it was).
 */
uint16_t CoursePaletteCycle(uint16_t a, int index8, uint16_t *xo)
{
    const uint32_t b = 0x040000;
    uint8_t c, v;
    uint16_t x = 0;
    static const uint16_t c3[4] = {0xE45F, 0xE48B, 0xE4B7, 0xE4E3};
    static const uint16_t c7[4] = {0xE50F, 0xE53B, 0xE4B7, 0xE4E3};
    static const uint16_t fixed7[4] = {0x14A7, 0x1086, 0x15FF, 0x269F};
    unsigned i;
    int bright = 0;

    cyc(140);
    c = rd8(0x7E0000 | W(course));
    a = (uint16_t)((a & 0xFF00) | c);
    if (c == 0x03) {
        a = palette_cycle_step(b, a, index8, &x);
        *xo = x;
        cyc(22);                        /* REP #$20 */
        for (i = 0; i < 4; i++) {
            uint32_t at = (b + c3[i] + x) & 0xFFFFFF;

            cyc(32);
            a = rd16(at);
            cyc(placed_read(at) ? 32 : 48);        /* a ROM read's 16 clocks are stated */
            wr16(W(wram_7EFD7D) + 2 * i, a);
        }
        cyc(24);                        /* JMP */
    } else if (c == 0x07) {
        cyc(44);
        a = palette_cycle_step(b, a, index8, &x);
        *xo = x;
        cyc(22);                        /* REP #$20 */
        for (i = 0; i < 4; i++)
            { cyc(56); wr16(W(wram_7F37EF) + 2 * i, fixed7[i]); }
        for (i = 0; i < 4; i++) {
            cyc(80);
            a = rd16(b + c7[i] + x);
            wr16(W(wram_7F39EF) + 2 * i, a);
        }
        cyc(24);                        /* JMP */
    } else if (c == 0x0B) {
        cyc(100);
        a = palette_cycle_step(b, a, index8, &x);
        *xo = x;
        cyc(32);
        v = rd8(W(wram_7F3460));
        bright = v >= 0x18;
        cyc(104);
        v = rd8(b + (bright ? 0xE433 : (cyc(6), 0xE407)) + x);
        a = (uint16_t)((a & 0xFF00) | v);
        wr8(W(wram_7EFD83), v);
        cyc(32);
        wr8(W(wram_7F37F5), v);
        cyc(32);
        wr8(W(wram_7F39F5), v);
        if (bright)
            cyc(22);                    /* BRA */
    } else { cyc(138); }
    cyc(80);
    cyc(42);                            /* PLP */
    return a;
}

void CoursePaletteCycle_cr(Carried *cr)
{
    uint16_t x = cr->x;

    cr->a = CoursePaletteCycle(cr->a, (cr->p & P_X) != 0, &x);
    cr->x = x;
    if (cr->p & P_X) {
        cr->x &= 0x00FF;
        cr->y &= 0x00FF;
    }
}

void abi_CoursePaletteCycle(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CoursePaletteCycle_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* HudColourStep's work registers in bank $7F. */
#define HC_TO(o) (W(wram_7F3458) + (o))      /* $3458: +2 band end, +4 request, +8 blend, +12 last blend */
#define HC_AMT(o) (W(slide_out) + (o))     /* $3468: amount, +4 last amount */
#define HC_TGT(o) (W(slide_done) + (o))     /* $3470: target, +4 last, +8 gradient flag, +12 band */

/* The multiplier's factor loaded from the low byte of v (PpuMultiplierSetLowByteTimes8
   as the colour step calls it, with its own clocks). */
static void hc_factor(uint8_t v)
{
    cyc(0);
    PpuMultiplierSetLowByteTimes8(v);
    cyc(64);
}

enum { HC_START, HC_ROWS_REST, HC_FROM14 };

/* HudColourStep's body, entered at its start or at one of its two inner
   entries. The start keeps the P and data bank it pushes (and the colour
   the band loop pushes and pulls) here, not on the stack: called from C
   there is no return address under them, so stack writes would land where
   the original writes other things. An inner entry finds P and the data
   bank on the stack above its return address, as the start leaves them. */
static HudColourEnd hud_colour_step(int from, uint8_t bank, uint16_t a, uint16_t x, uint16_t y)
{
    uint16_t pushed = 0, m;
    int first, c;
    HudColourEnd e;

    e.popped = 0;
    e.db = e.p = 0;
    switch (from) {
    case HC_ROWS_REST:
        goto rows_rest;
    case HC_FROM14:
        x = 0x0014;
        goto buffer;
    }
    cyc(22);                            /* REP #$30 */
    cyc(156);
    a = rd16(HC_TO(4));
    if (a) {
        if (a & 0x8000) {
            cyc(2);
            a &= 0x7FFF;
            cyc(90);
            wr16(HC_TGT(0), a);
            cyc(24);
            a = rd16(HC_AMT(0));
            a = add16(a, 0x0002, NULL);
            cyc(0);
            if (a >= 0x001F)
                { cyc(38); a = 0x001F; cyc(4); } else { cyc(24); }
            cyc(38);
            goto store_amount;
        }
        cyc(70);
        a = rd16(W(course));
        cyc(2);
        a &= 0x00FF;
        cyc(0);
        if (a == 0x0006) {
            cyc(92);
            a = rd16(HC_AMT(0));
            a = sub16(a, 0x0003, NULL, NULL);
            cyc(0);
            if (a >= 0x000A)
                { cyc(56); goto store_amount; }
            cyc(38);
            a = 0x000A;
            goto floor;
        } else { cyc(8); }
        cyc(0);
        if (a == 0x0004) {
            cyc(118);
            a = rd16(HC_AMT(0));
            a = sub16(a, 0x0003, NULL, NULL);
            cyc(0);
            if (a >= 0x000F)
                { cyc(56); goto store_amount; } else { cyc(22); }
            cyc(38);
            a = 0x000F;
            goto floor;
        }
        cyc(132);
        a = rd16(W(fade_level));
        cyc(0);
        if (a != 0x0F00) {
            cyc(64);
            a = rd16(HC_AMT(0));
            goto store_amount;
        }
        cyc(70);
        a = rd16(HC_AMT(0));
        if (!a)
            goto gradient;
        a = sub16(a, 0x0003, &c, NULL);
        if (c)
            { cyc(48); goto store_amount; }
        cyc(38);
        a = 0xFFFF;
        cyc(74);
        wr16(HC_TGT(0), 0xFFFF);
        cyc(38);
        a = 0x0000;
        cyc(10);
        wr16(HC_TO(4), 0x0000);
        goto store_amount;
floor:
        cyc(82);
        wr16(HC_AMT(0), a);
        cyc(38);
        a = 0x0000;
        cyc(10);
        wr16(HC_TO(4), 0x0000);
        goto gradient;
store_amount:
        cyc(46);
        wr16(HC_AMT(0), a);
        cyc(32);
        goto gradient_load;
    } else { cyc(24); }
gradient:
    cyc(54);
gradient_load:
    a = rd16(W(wram_0DA4));
    if (a & 0x0800) {
        cyc(64);
        wr16(HC_TGT(8), a);
        y = 0x0080;
        x = 0x00FE;
        cyc(80);
        a = rd16(W(gsu_buffer_select));
        if (a) {
            cyc(18);
            x = 0x01FE;
        }
        cyc(46);
        a = rd16(HC_TGT(0));
        if (!(a & 0x8000)) {
            cyc(32);
            wr16(W(dp_f3), a);
            cyc(38);
            a = 0x0000;
            cyc(2);
            wr16(W(dp_f1), 0x0000);
            cyc(24);
            a = rd16(HC_AMT(0));
            cyc(16);
            wr16(W(dp_f5), a);
            cyc(46);
            a = ColourBlendBy(bank);
            cyc(32);
        } else { cyc(54); }
        a = rd16(W(gsu_buffer_select));
        a = (uint16_t)(a << 1);
        cyc(30);
        x = a;
        cyc(38);
        a = 0xF757;
        cyc(16);
        wr16(W(wram_0D9A) + x, a);
        cyc(38);
        a = 0x0000;
        x = 0x0004;
        do {
            cyc(42);
            wr16(W(palette_buffer) + x, a);
            cyc(36);
            x = (uint16_t)(x + 2);
            cyc(28);
        } while (x != 0x0020);
        cyc(28);
        goto end;
    }
    cyc(70);
    a = rd16(HC_TGT(8));
    if (a) {
        cyc(40);
        a = rd16(HC_TGT(0));
        cyc(24);
        wr16(HC_TGT(4), a);
        cyc(38);
        a = 0x0000;
        cyc(10);
        wr16(HC_TGT(8), 0x0000);
        x = 0x0004;
        goto blend_rows;
    }
    cyc(46);
    a = rd16(HC_TGT(12));
    a++;
    cyc(0);
    if (a >= 0x0003)
        { cyc(38); a = 0x0000; } else { cyc(20); }
    cyc(64);
    wr16(HC_TGT(12), a);
    a = (uint16_t)(a << 1);
    cyc(30);
    x = a;
    cyc(30);
    a = rd16(W(wram_7EE69C) + x);
    y = a;
    a = add16(a, 0x0100, NULL);
    cyc(76);
    wr16(HC_TO(2), a);
    cyc(32);
    a = rd16(W(gsu_buffer_select));
    a = (uint16_t)(a << 1);
    cyc(30);
    x = a;
    a = y;
    a = add16(a, 0xEC57, NULL);
    cyc(82);
    wr16(W(wram_0D9A) + x, a);
    cyc(32);
    a = rd16(W(wram_0D96));
    cyc(30);
    x = a;
    cyc(8);
    a = rd16(HC_TGT(0));
    if (a & 0x8000) {
        cyc(40);
        a = rd16(HC_TO(8));
        cyc(46);
        hc_factor((uint8_t)a);
        cyc(30);
        a = rd16(W(gradient_colours) + x);
        cyc(16);
        wr16(W(dp_f1), a);
        cyc(52);                                /* BRA, LDA abs,X */
        goto a_to;
        for (;;) {
            cyc(24);
            a = rd16(W(gradient_colours) + x);
            cyc(30);
            m = rd16(W(wram_7FEF55) + x);
            cyc(0);
            if (a != m) {
                cyc(4);
                cyc(34);
                wr16(W(dp_f1), a);
                cyc(30);                        /* LDA abs,X */
                goto a_to;
            }
            cyc(32);
            wr16(W(dp_f1), a);
            cyc(30);
            a = rd16(W(wram_7FF357) + x);
            cyc(30);
            m = rd16(W(wram_7FF355) + x);
            cyc(0);
            if (a != m)
                { cyc(22); goto a_blend; }
            cyc(46);
            a = rd16(W(wram_7FEC55) + y);
            cyc(52);                            /* BRA, STA abs,Y */
            goto a_store;
a_to:
            a = rd16(W(wram_7FF357) + x);
a_blend:
            cyc(16);
            wr16(W(dp_f3), a);
            cyc(46);
            a = ColourBlend(bank);
            cyc(30);                            /* RTS, STA abs,Y */
a_store:
            wr16(W(wram_7FEC57) + y, a);
            x = (uint16_t)(x + 2);
            cyc(28);
            cyc(52);
            m = rd16(HC_TO(2));
            y = (uint16_t)(y + 2);
            cyc(28);
            if (y >= m)
                { cyc(22); break; }
        }
        goto buffer4;
    }
    cyc(52);
    a = rd16(W(gradient_colours) + x);
    cyc(16);
    wr16(W(dp_f1), a);
    cyc(52);                                    /* BRA, LDA abs,X */
    goto b_to;
    for (;;) {
        cyc(24);
        a = rd16(W(gradient_colours) + x);
        cyc(30);
        m = rd16(W(wram_7FEF55) + x);
        cyc(0);
        if (a != m) {
            cyc(38);
            wr16(W(dp_f1), a);
            cyc(30);                            /* LDA abs,X */
            goto b_to;
        }
        cyc(32);
        wr16(W(dp_f1), a);
        cyc(30);
        a = rd16(W(wram_7FF357) + x);
        cyc(30);
        m = rd16(W(wram_7FF355) + x);
        cyc(0);
        if (a != m)
            { cyc(22); goto b_blend; }
        cyc(46);
        a = rd16(W(wram_7FEC55) + y);
        cyc(52);                                /* BRA, STA abs,Y */
        goto b_store;
b_to:
        a = rd16(W(wram_7FF357) + x);
b_blend:
        cyc(16);
        wr16(W(dp_f3), a);
        cyc(24);
        a = rd16(HC_TO(8));
        cyc(16);
        wr16(W(dp_f5), a);
        cyc(46);
        a = ColourBlendBy(bank);
        pushed = a;
        cyc(54);
        goto rows_tail;
rows_rest:
        pushed = a;
        cyc(54);
rows_tail:
        a = rd16(HC_TGT(0));
        if (a & 0x8000) {
            a = pushed;
            cyc(88);                            /* BMI, PLA, STA abs,Y */
            goto b_store;
        }
        cyc(32);
        wr16(W(dp_f3), a);
        a = pushed;
        cyc(52);
        wr16(W(dp_f1), a);
        cyc(24);
        a = rd16(HC_AMT(0));
        cyc(16);
        wr16(W(dp_f5), a);
        cyc(46);
        a = ColourBlendBy(bank);
        cyc(52);
b_store:
        wr16(W(wram_7FEC57) + y, a);
        x = (uint16_t)(x + 2);
        cyc(28);
        cyc(52);
        m = rd16(HC_TO(2));
        y = (uint16_t)(y + 2);
        cyc(28);
        if (y >= m)
            break;
    }
buffer4:
    cyc(10);
    x = 0x0004;
buffer:
    cyc(48);
    a = rd16(HC_TO(8));
    cyc(24);
    m = rd16(HC_TO(12));
    cyc(0);
    if (a != m) {
        cyc(46);
        a = rd16(HC_TGT(0));
        if (a & 0x8000)
            goto blend_both;
        goto blend_rows;
    }
    cyc(40);
    a = rd16(HC_TGT(0));
    if (a & 0x8000) {
        cyc(40);
        a = rd16(HC_TGT(4));
        if (a & 0x8000)
            { cyc(40); goto hud; }
        do {
            cyc(54);
            a = rd16(W(palette_work_a) + x);
            cyc(32);
            wr16(W(palette_buffer) + x, a);
            cyc(24);
            x = (uint16_t)(x + 2);
            cyc(28);
        } while (!(x >= 0x0200));
        cyc(40);
        goto hud;
    }
    cyc(46);
    m = rd16(HC_TGT(4));
    cyc(0);
    if (a == m) {
        cyc(46);
        a = rd16(HC_AMT(0));
        cyc(24);
        m = rd16(HC_AMT(4));
        cyc(0);
        if (a == m)
            { cyc(18); goto keep; }
    } else { cyc(18); }
    cyc(46);
    a = rd16(HC_AMT(0));
    cyc(46);
    hc_factor((uint8_t)a);
    cyc(18);
    do {
        cyc(14);
        a = rd16(W(palette_work_b) + x);
        cyc(32);
        m = rd16(W(palette_work_c) + x);
        cyc(0);
        if (a != m) {
            cyc(40);
            a = rd16(HC_TGT(0));
            cyc(16);
            wr16(W(dp_f3), a);
            cyc(32);
            a = rd16(W(palette_work_a) + x);
            cyc(16);
            wr16(W(dp_f1), a);
            cyc(46);
            a = ColourBlend(bank);
            cyc(32);
            wr16(W(palette_buffer) + x, a);
        } else { cyc(22); }
        cyc(64);
        x = (uint16_t)(x + 2);
        cyc(28);
    } while (!(x >= 0x0200));
    cyc(0);
    goto hud;
blend_rows:
    cyc(2);
    do {
        cyc(78);
        a = rd16(W(palette_work_b) + x);
        cyc(46);
        m = rd16(W(palette_work_c) + x);
        a = sub16(a, m, NULL, NULL);
        if (a) {
            cyc(34);
            cyc(0);
            if (a == 0x0001)
                goto from_b; else { cyc(36); }
            cyc(4);
            cyc(0);
            if (a == 0xFFFF)
                goto from_b;
            cyc(40);
            a = rd16(HC_TO(8));
            if (!a)
                goto from_b;
            cyc(0);
            if (a == 0x001F) {
                cyc(84);
                a = rd16(W(palette_work_c) + x);
                goto work;
            }
            cyc(90);
            a = rd16(W(palette_work_b) + x);
            cyc(16);
            wr16(W(dp_f1), a);
            cyc(32);
            a = rd16(W(palette_work_c) + x);
            cyc(16);
            wr16(W(dp_f3), a);
            cyc(24);
            a = rd16(HC_TO(8));
            cyc(16);
            wr16(W(dp_f5), a);
            cyc(46);
            a = ColourBlendBy(bank);
            goto work;
from_b:
            cyc(54);
            a = rd16(W(palette_work_b) + x);
work:
            cyc(54);
            wr16(W(palette_work_a) + x, a);
            cyc(16);
            wr16(W(dp_f1), a);
            cyc(24);
            a = rd16(HC_TGT(0));
            cyc(16);
            wr16(W(dp_f3), a);
            cyc(24);
            a = rd16(HC_AMT(0));
            cyc(16);
            wr16(W(dp_f5), a);
            cyc(46);
            a = ColourBlendBy(bank);
            cyc(32);
            wr16(W(palette_buffer) + x, a);
        } else { cyc(16); }
        x = (uint16_t)(x + 2);
        cyc(28);
    } while (!(x >= 0x0200));
    cyc(62);
    goto hud;
blend_both:
    cyc(46);
    a = rd16(HC_TO(8));
    cyc(16);
    wr16(W(dp_f5), a);
    cyc(46);
    hc_factor((uint8_t)a);
    first = 1;
    do {
        cyc(first ? 32 : 78);           /* LDA long; after the first, INX, INX, CPX, BCC too */
        first = 0;
        a = rd16(W(palette_work_b) + x);
        cyc(46);
        m = rd16(W(palette_work_c) + x);
        a = sub16(a, m, NULL, NULL);
        if (a) {
            cyc(52);
            cyc(0);
            if (a == 0x0001)
                { cyc(4); } else {
                cyc(40);
                cyc(0);
                if (a == 0xFFFF)
                    { cyc(4); } else {
                    cyc(30);
                    a = rd16(W(palette_work_b) + x);
                    cyc(16);
                    wr16(W(dp_f1), a);
                    cyc(32);
                    a = rd16(W(palette_work_c) + x);
                    cyc(16);
                    wr16(W(dp_f3), a);
                    cyc(46);
                    a = ColourBlend(bank);
                    cyc(32);
                    wr16(W(palette_work_a) + x, a);
                    cyc(32);
                    wr16(W(palette_buffer) + x, a);
                }
            }
        } else { cyc(16); }
        x = (uint16_t)(x + 2);
        cyc(28);
    } while (!(x >= 0x0200));
    cyc(40);                            /* CPX, BCC not taken */
hud:
    {
        static const struct { uint16_t colour, x; } hud[6] = {
            {0x45CC, 0x0000}, {0x2528, 0x0002}, {0x1CE5, 0x0004},
            {0x739A, 0x0006}, {0x1CE5, 0x0008}, {0x2528, 0x000A}};
        int i;

        for (i = 0; i < 6; i++) {
            cyc(56);
            cyc(38);
            x = hud[i].x;
            a = ColourBlendToTarget(bank, hud[i].colour, x);
        }
    }
    cyc(22);                            /* REP #$30 */
    cyc(32);
    a = rd16(W(course));
    cyc(2);
    a &= 0x00FF;
    cyc(0);
    if (a == 0x0007) {
        cyc(86);
        a = rd16(HC_TO(4));
        if (!a) {
            cyc(40);
            a = rd16(HC_TO(8));
            cyc(16);
            wr16(W(dp_f5), a);
            cyc(46);
            hc_factor((uint8_t)a);
            x = 0x0018;
            cyc(42);
            do {
                cyc(14);
                a = rd16(W(palette_work_b) + x);
                cyc(16);
                wr16(W(dp_f1), a);
                cyc(32);
                a = rd16(W(palette_work_c) + x);
                cyc(16);
                wr16(W(dp_f3), a);
                cyc(46);
                a = ColourBlend(bank);
                cyc(32);
                wr16(W(palette_work_a) + x, a);
                cyc(32);
                wr16(W(palette_buffer) + x, a);
                x = (uint16_t)(x + 2);
                cyc(28);
            } while (cyc_loop(64, !(x >= 0x0020)));
            cyc(18);
        }
    } else { cyc(46); }
keep:
    cyc(46);
    a = rd16(HC_TO(8));
    cyc(24);
    wr16(HC_TO(12), a);
    cyc(24);
    a = rd16(HC_AMT(0));
    cyc(24);
    wr16(HC_AMT(4), a);
    cyc(24);
    a = rd16(HC_TGT(0));
    cyc(24);
    wr16(HC_TGT(4), a);
end:
    cyc(20);
    cyc(22);                            /* SEP #$30 */
    x &= 0x00FF;
    y &= 0x00FF;
    if (from != HC_START) {
        cpu.s++;
        e.db = rd8((uint16_t)cpu.s);
        cpu.s++;
        cyc(20);
        e.p = rd8((uint16_t)cpu.s);
        e.popped = 1;
    } else { cyc(36); }
    cyc(2);
    cyc(42);                            /* PLP */
    e.a = a;
    e.x = x;
    e.y = y;
    return e;
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
 * In C: x and y are X and Y on entry (Y stays as it is on some paths); the
 * result is A, X and Y on exit. The inner entries take the data bank, which
 * ColourBlend and its callers read.
 */
HudColourEnd HudColourStep(uint16_t x, uint16_t y)
{
    return hud_colour_step(HC_START, 0x7F, 0, x, y);
}

void HudColourStep_cr(Carried *cr)
{
    HudColourEnd e = HudColourStep(cr->x, cr->y);

    cr->a = e.a;
    cr->x = e.x;
    cr->y = e.y;
    if (cr->p & P_X) {
        cr->x &= 0x00FF;
        cr->y &= 0x00FF;
    }
}

void abi_HudColourStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    HudColourStep_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
 * In C: the result also carries the data bank and P pulled from the stack.
 */
HudColourEnd HudColourRowsRest(uint8_t bank, uint16_t a, uint16_t x, uint16_t y)
{
    return hud_colour_step(HC_ROWS_REST, bank, a, x, y);
}

void HudColourRowsRest_cr(Carried *cr)
{
    HudColourEnd e = HudColourRowsRest(cr->db, cr->a, cr->x, cr->y);

    cr->a = e.a;
    cr->x = e.x;
    cr->y = e.y;
    cr->db = e.db;
    set_p(cr, e.p);
    if (cr->p & P_X) {
        cr->x &= 0x00FF;
        cr->y &= 0x00FF;
    }
}

void abi_HudColourRowsRest(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    HudColourRowsRest_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
 * In C: as HudColourRowsRest.
 */
HudColourEnd HudColourStepFrom14(uint8_t bank, uint16_t y)
{
    return hud_colour_step(HC_FROM14, bank, 0, 0, y);
}

void HudColourStepFrom14_cr(Carried *cr)
{
    HudColourEnd e = HudColourStepFrom14(cr->db, cr->y);

    cr->a = e.a;
    cr->x = e.x;
    cr->y = e.y;
    cr->db = e.db;
    set_p(cr, e.p);
    if (cr->p & P_X) {
        cr->x &= 0x00FF;
        cr->y &= 0x00FF;
    }
}

void abi_HudColourStepFrom14(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    HudColourStepFrom14_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
 * In C: returns the colour's high byte; *xo is the new phase, *db the pulled
 * data bank, *wrapped whether the phase wrapped.
 */
uint8_t PaletteCycleFourth(uint8_t bank, uint16_t y, uint16_t *xo, uint8_t *db, int *wrapped)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t x;
    uint8_t v;

    cyc(24);
    x = rd16(b | 0xFD61);
    cyc(32);
    v = rd8(W(cycle_colours_d) + x);
    cyc(30);
    wr8(b | (uint16_t)(0xFD8B + y), v);
    cyc(32);
    v = rd8(W(wram_7EB6EC) + x);
    cyc(30);
    wr8(b | (uint16_t)(0xFD8C + y), v);
    x = (uint16_t)(x + 2);
    *wrapped = x >= 0x0010;
    if (*wrapped) {
        cyc(18);
        x = 0x0000;
    }
    cyc(98);
    wr16(b | 0xFD61, x);
    *xo = x;
    cpu.s++;
    cyc(20);
    *db = rd8((uint16_t)cpu.s);
    cyc(42);                            /* RTS */
    return v;
}

void PaletteCycleFourth_cr(Carried *cr)
{
    uint16_t x;
    uint8_t db;
    int wrapped;
    uint8_t v = PaletteCycleFourth(cr->db, cr->y, &x, &db, &wrapped);

    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    cr->x = x;
    cr->db = db;
    set_p(cr, (uint8_t)((cr->p & ~P_C) | (wrapped ? P_C : 0)));
    set_p(cr, nz8(cr->p, db));
}

void abi_PaletteCycleFourth(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PaletteCycleFourth_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

