/*
 * slide.c: HUD panels that slide on and off the screen.
 */
#include "machine.h"
#include "cpu_ops.h"
#include "hud_calls.h"
#include "carried.h"
#include "wram_names.h"
#include "wram_struct.h"



/*
 * @routine 7E:AECE HudSlideIn
 * WHAT IT DOES: Moves the sliding HUD panel 4 pixels further in, stopping at
 * $10; a negative offset just moves.
 * HOW: Adds 4; when the result is not negative and is $10 or more, uses $10.
 * Entry: M 8-bit. Exit: A low byte = the new offset; flags from the add,
 * the compare or the load.
 */
static uint8_t hud_slide_in(uint8_t *p)
{
    cyc(70);
    uint8_t v = RD8(hud_slide);
    unsigned sum = v + 4;
    uint8_t r = (uint8_t)sum;

    *p = (uint8_t)((*p & ~(P_C | P_V)) | (sum > 0xFF ? P_C : 0)
                   | ((~(v ^ 4) & (v ^ r) & 0x80) ? P_V : 0));
    *p = nz8(*p, r);
    if (!(r & 0x80)) {
        cyc(42);
        *p = (uint8_t)(nz8(*p, (uint8_t)(r - 0x10)) & ~P_C) | (r >= 0x10 ? P_C : 0);
        if (r >= 0x10) {
            r = 0x10;
            *p = nz8(*p, r);
        }
    }
    cyc(76);
    WR8(hud_slide, r);
    cyc(42);
    return r;
}

void HudSlideIn(void)
{
    uint8_t p = 0;

    hud_slide_in(&p);
}

void HudPanelASlide_cr(Carried *cr);
void HudPanelBSlide_cr(Carried *cr);
void HudPanelsStep_cr(Carried *cr);
void HudSlideIn_cr(Carried *cr);
void HudSlideOut_cr(Carried *cr);
static void panel_exit(Carried *cr, uint8_t a, uint16_t x);

void HudSlideIn_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = (uint16_t)((cr->a & 0xFF00) | hud_slide_in(&p));
    set_p(cr, p);
}

void abi_HudSlideIn(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    HudSlideIn_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

uint8_t SpriteYOnScreen(uint8_t y);

/* The sprite positions that follow the slide ($0D57).
   The tails below are entered by jumps from routines that pushed P, so
   each ends with that routine's PLP (done by the caller here). */

/* LDA abs (8-bit) after n clocks of the code before it. */
static uint8_t lda8_after(uint32_t a, unsigned n)
{
    cyc(n);
    return rd8(a);
}

static uint8_t lda8(uint32_t a)
{
    return lda8_after(a, 48);
}

/* ADC #m (8-bit, binary: D is clear here) with the carry *c, which it
   leaves; 2 clocks more than the plain add. */
static uint8_t adc8i(uint8_t v, uint8_t m, int *c)
{
    unsigned r = (unsigned)v + m + (*c ? 1 : 0);

    cyc(2);
    *c = r > 0xFF;
    return (uint8_t)r;
}

/* SEC; SBC #m (8-bit): op_sbc8's stated time. */
static uint8_t sbc8i(uint8_t v, uint8_t m)
{
    cyc(16);
    return (uint8_t)(v - m);
}

/* CMP #imm, 8-bit: its time. */
static void cmp8(uint8_t v, uint8_t m)
{
    cyc(v >= m ? 38 : 76);
}

/* The slide plus $F9 as the panel's Y, and its sprites' Y bytes (x =
   the panel's offset) at $0405-$042D, each through SpriteYOnScreen.
   Several adds take the carry the clamp's compare left. Returns the last
   Y (A's low byte). */
static uint8_t hud_sprites_y(uint16_t d, uint16_t x)
{
    static const uint16_t row[4][2] = {{0x0411, 0x0421}, {0x0415, 0x0425}, {0x0419, 0x0429}, {0x041D, 0x042D}};
    static const struct { uint8_t add; int clc; uint16_t at; } tail[3] = {{0x0A, 0, 0x0405}, {0x18, 0, 0x0409}, {0x26, 1, 0x040D}};
    uint8_t v0, v, y = 0;
    int i, c = 0;

    cyc(92);
    v0 = RD8(hud_slide);
    cyc(30);
    v0 = adc8i(v0, rd8((uint16_t)(d + W(hud_panel_y))), &c);
    v = v0;
    for (i = 0; i < 4; i++) {
        if (i) {
            cyc(i == 3 ? 14 : 36);   /* PLA, CLC, ADC; PHA unless the last */
            c = 0;
            v = adc8i(v, 0x10, &c);
        }
        y = SpriteYOnScreen(v);
        c = v >= 0xB0;
        cyc(198);
        wr8(0x7E0000 | (uint16_t)(row[i][0] + x), y);
        cyc(30);
        wr8(0x7E0000 | (uint16_t)(row[i][1] + x), y);
    }
    for (i = 0; i < 3; i++) {
        uint8_t r;

        if (tail[i].clc)
            c = 0; else { cyc(8); }
        r = adc8i(v0, tail[i].add, &c);
        y = SpriteYOnScreen(r);
        c = r >= 0xB0;
        cyc(212);
        wr8(0x7E0000 | (uint16_t)(tail[i].at + x), y);
    }
    return y;
}

/* Panel B's Y ($0E0B - $0D) for its two sprite pairs, and + $10. */
static uint8_t hud_panel_b_y(void)
{
    uint8_t v = lda8(WA(map_b_y_w));
    int c = 0;

    v = sbc8i(v, 0x0D);
    cyc(38);
    WR8(wram_0535, v);
    cyc(24);
    WR8(wram_053D, v);
    v = adc8i(v, 0x10, &c);
    cyc(52);
    WR8(wram_0539, v);
    cyc(24);
    WR8(wram_0541, v);
    return v;
}

/* The map marker's Y ($0E16) and the column of Ys below it, $FF once a
   step carries. */
static uint8_t hud_marker_y(void)
{
    static const struct { uint8_t add; uint16_t at[2]; int n; } step[6] = {
        {0x08, {0x0545, 0}, 1}, {0x06, {0x055D, 0x0561}, 2}, {0x08, {0x0565, 0x0569}, 2},
        {0x08, {0x0549, 0}, 1}, {0x08, {0x054D, 0x0551}, 2}, {0x08, {0x0555, 0x0559}, 2}};
    uint8_t v = lda8(WA(wram_0E16));
    int i, j, c;

    cyc(24);
    WR8(wram_056D, v);
    cyc(24);
    WR8(wram_0571, v);
    for (i = 0; i < 6; i++) {
        cyc(50);
        c = 0;
        v = adc8i(v, step[i].add, &c);
        if (c)
            { cyc(10); v = 0xFF; }
        for (j = 0; j < step[i].n; j++)
            { cyc(24); wr8(0x7E0000 | step[i].at[j], v); }
    }
    return v;
}

/* Gauge B's X ($0E12) and the row of Xs after it: the first five skipped
   while negative, the rest carrying on (no CLC). */
static uint8_t hud_gauge_b_x(void)
{
    static const struct { uint8_t add; int sub, clc, skip; uint16_t at[2]; int n; } step[11] = {
        {0x00, 0, 0, 1, {0x04A1, 0}, 1}, {0x08, 1, 0, 1, {0x04BD, 0}, 1},
        {0x10, 0, 1, 1, {0x04A5, 0}, 1}, {0x08, 0, 1, 1, {0x04A9, 0}, 1},
        {0x08, 0, 1, 1, {0x04AD, 0}, 1}, {0x08, 0, 0, 0, {0x04B5, 0}, 1},
        {0x08, 0, 0, 0, {0x04B9, 0x04B1}, 2}, {0x08, 0, 0, 0, {0x04C1, 0}, 1},
        {0x04, 0, 0, 0, {0x04C5, 0}, 1}, {0x08, 0, 0, 0, {0x04C9, 0}, 1},
        {0x08, 0, 0, 0, {0x04CD, 0}, 1}};
    uint8_t v = lda8(WA(gauge_b6_x));
    int i, j, c = 0;

    for (i = 0; i < 11; i++) {
        if (i) {
            cyc(14);
            if (step[i].sub) {
                c = v >= step[i].add;   /* SEC; SBC: no borrow */
                v = sbc8i(v, step[i].add);
            } else {
                if (step[i].clc)
                    { cyc(14); c = 0; }
                v = adc8i(v, step[i].add, &c);
            }
        }
        if (step[i].skip && (cyc(16), (v & 0x80)))
            { cyc(6); continue; }
        for (j = 0; j < step[i].n; j++)
            { cyc(24); wr8(0x7E0000 | step[i].at[j], v); }
    }
    return v;
}

/*
 * @routine 7E:AEE0 HudSlideOut
 * WHAT IT DOES: Moves the HUD slide $0D57 back by 4, down to $D0 (once it
 * is negative).
 * HOW: 8-bit subtract and compare through the data bank.
 * Entry: M 8-bit. Exit: A low byte = the new slide; flags from it.
 */
static uint8_t out_cv, out_nz;          /* the adapter's: C and V, and the value N and Z come from */

uint8_t HudSlideOut(uint8_t bank)
{
    (void)bank;
    uint8_t v = lda8(WA(hud_slide)), r;

    r = sbc8i(v, 0x04);
    out_cv = (uint8_t)((v >= 0x04 ? P_C : 0) | (((v ^ 0x04) & (v ^ r) & 0x80) ? P_V : 0));
    v = r;
    out_nz = v;
    if (v & 0x80) {
        cmp8(v, 0xD0);
        out_cv = (uint8_t)((out_cv & P_V) | (v >= 0xD0 ? P_C : 0));
        out_nz = (uint8_t)(v - 0xD0);
        if (v < 0xD0)
            out_nz = v = 0xD0;
    }
    cyc(54);
    WR8(hud_slide, v);
    cyc(42);
    return v;
}

void HudSlideOut_cr(Carried *cr)
{
    uint8_t v = HudSlideOut(cr->db);

    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    set_p(cr, nz8((uint8_t)((cr->p & ~(P_C | P_V)) | out_cv), out_nz));
}

void abi_HudSlideOut(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    HudSlideOut_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* AE18 and AE30's body: $F9 = the panel's Y offset, x its sprites; by stage
   $0D54: below 8 nothing, 8 (slide in first for panel A) or above (out
   first, restarting the stage at $D0, for panel A) then the sprites.
   Returns A's low byte as it ends. */
static uint8_t hud_panel_slide(uint8_t bank, uint16_t d, uint8_t f9, uint16_t x, int panel_a)
{
    uint8_t v, p = 0;

    cyc(106);
    WR8(dp_f9, f9);
    v = lda8_after(WA(wram_0D54), panel_a ? 24 : 48);   /* A: STA, LDA; B: STA, LDX #, LDA */
    cmp8(v, 0x08);
    if (v < 0x08) {
        cyc(42);                        /* PLP */
        return v;
    }
    if (panel_a) {
        if (v == 0x08)
            hud_slide_in(&p);
        else {
            v = lda8(WA(hud_slide));
            cmp8(v, 0xD0);
            if (v == 0xD0)
                WR8(wram_0D54, 0x00);
            HudSlideOut(bank);
        }
    }
    v = hud_sprites_y(d, x);
    cyc(28);
    cyc(42);                            /* PLP */
    return v;
}

/*
 * @routine 7E:AE18 HudPanelASlide
 * WHAT IT DOES: In stage 8 and above ($0D54) slides panel A in (stage 8,
 * HudSlideIn) or out (above, HudSlideOut, the stage restarting when the
 * slide was at $D0) and places its sprites (Y offset $F8, sprite offset
 * $20).
 * HOW: The shared tail at $7E:AFA7.
 * Entry: any widths, D = $0000. Exit: P restored; X = $20 plus as the
 * tail left it; A as it left it.
 */
uint8_t HudPanelASlide(uint8_t bank, uint16_t d)
{
    cyc(24);
    return hud_panel_slide(bank, d, 0xF8, 0x0020, 1);
}

/* The panels' registers: A's low byte, X; P restored (8-bit index
   registers keep X's low byte). */
static void panel_exit(Carried *cr, uint8_t a, uint16_t x)
{
    cr->a = (uint16_t)((cr->a & 0xFF00) | a);
    cr->x = (cr->p & P_X) ? (uint8_t)x : x;
}

void HudPanelASlide_cr(Carried *cr)
{
    panel_exit(cr, HudPanelASlide(cr->db, cr->d), 0x0020);
}

void abi_HudPanelASlide(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    HudPanelASlide_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 7E:AE30 HudPanelBSlide
 * WHAT IT DOES: In stage 8 and above places panel B's sprites (Y offset
 * $4A, sprite offset $120) without moving the slide.
 * HOW: As HudPanelASlide.
 * Entry: as HudPanelASlide. Exit: as HudPanelASlide.
 */
uint8_t HudPanelBSlide(uint8_t bank, uint16_t d)
{
    return hud_panel_slide(bank, d, 0x4A, 0x0120, 0);
}

void HudPanelBSlide_cr(Carried *cr)
{
    panel_exit(cr, HudPanelBSlide(cr->db, cr->d), 0x0120);
}

void abi_HudPanelBSlide(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    HudPanelBSlide_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 7E:AE5E HudPanelsStep
 * WHAT IT DOES: The HUD panels' stage $0D54: 2 moves the map marker $0E16
 * up by 4 to $AC and places its column; 4 moves gauge B's X $0E12 right by
 * 4 to $1F and places its row; 6 moves panel B's Y $0E0B down by 4 to $2A
 * ($0E0C = $7C there) and places it; 8 slides in, 10 slides out (with the
 * panel A sprites, $F9 = 0, X = 0); 0 does nothing.
 * HOW: JMP ($AE67,X) on the stage; the placing tails at $7E:AF09, $AF5C,
 * $AEF2 and $AFA7. The marker clamp at $AC is not reached in the traces
 * so far.
 * Entry: any widths, D = $0000. Exit: P restored.
 */
static int step_a_set;                  /* the adapter's: A's low byte set (step_a), X the stage or 0 */
static uint8_t step_a, step_x;

void HudPanelsStep(uint8_t bank, uint16_t d)
{
    uint8_t v, p = 0;

    cyc(22);                            /* SEP #$30 */
    cyc(46);
    step_x = RD8(wram_0D54);
    step_a_set = 1;
    switch (step_x) {
    case 0x02:
        cyc(22);
        v = lda8(WA(wram_0E16));
        v = sbc8i(v, 0x04);
        /* CMP #$AC, BCS taken (38) or not, LDA #$AC (48) */
        cyc(v >= 0xAC ? 38 : 48);
        if (v < 0xAC)
            v = 0xAC;
        cyc(38);
        WR8(wram_0E16, v);
        step_a = hud_marker_y();
        break;
    case 0x04:
        cyc(22);
        v = lda8(WA(gauge_b6_x));
        v = (uint8_t)(v + 0x04);
        if (v & 0x80) {
            cyc(76);
        } else {
            cyc(v >= 0x1F ? 118 : 108);
            if (v >= 0x1F)
                v = 0x1F;
        }
        WR8(gauge_b6_x, v);
        step_a = hud_gauge_b_x();
        break;
    case 0x06:
        cyc(22);
        v = lda8(WA(map_b_y_w));
        v = (uint8_t)(v + 0x04);
        if (v & 0x80) {
            cyc(76);
        } else {
            if (v >= 0x2A) {
                cyc(118);
                WR8(wram_0E0C, 0x7C);
                v = 0x2A;
                cyc(40);
            } else { cyc(108); }
        }
        WR8(map_b_y_w, v);
        step_a = hud_panel_b_y();
        break;
    case 0x08:
    case 0x0A:
        cyc(46);
        if (step_x == 0x08)
            hud_slide_in(&p);
        else
            { cyc(22); HudSlideOut(bank); }
        step_x = 0x00;
        cyc(62);
        wr16((uint16_t)(d + W(hud_panel_y)), 0x0000);
        step_a = hud_sprites_y(d, 0x0000);
        break;
    default:
        step_a_set = 0;
        cyc(46);
        break;
    }
    cyc(28);
    cyc(42);                            /* PLP */
}

void HudPanelsStep_cr(Carried *cr)
{
    HudPanelsStep(cr->db, cr->d);
    if (step_a_set)
        cr->a = (uint16_t)((cr->a & 0xFF00) | step_a);
    cr->x = step_x;
    cr->y &= 0x00FF;
}

void abi_HudPanelsStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    HudPanelsStep_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

