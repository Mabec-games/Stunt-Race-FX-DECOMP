/*
 * slide.c: HUD panels that slide on and off the screen.
 */
#include "machine.h"
#include "cpu_ops.h"

/* The sliding panel's offset; $10 is fully shown. */
#define HUD_SLIDE 0x000D57u

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
    uint8_t v = rd8(HUD_SLIDE);
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
    wr8(HUD_SLIDE, r);
    cyc(42);
    return r;
}

void HudSlideIn(void)
{
    uint8_t p = 0;

    hud_slide_in(&p);
}

void abi_HudSlideIn(void)
{
    uint8_t p = cpu.p;

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | hud_slide_in(&p));
    cpu.p = p;
}

void abi_SpriteYOnScreen(void);

/* The sprite positions that follow the slide ($0D57).
   The tails below are entered by jumps from routines that pushed P, so
   each ends with that routine's PLP (done by the caller here). */

/* LDA abs after n clocks of the code before it. */
static uint8_t lda8_after(uint32_t a, unsigned n)
{
    cyc(n);
    uint8_t v = rd8(a);

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, v);
    return v;
}

static uint8_t lda8(uint32_t a)
{
    return lda8_after(a, 48);
}

static uint8_t adc8i(uint8_t v, uint8_t m)
{
    cyc(2);
    v = op_adc8(v, m, &cpu.p);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    return v;
}

/* CMP #imm, 8-bit. */
static void cmp8(uint8_t v, uint8_t m)
{
    cpu.p = (uint8_t)((nz8(cpu.p, (uint8_t)(v - m)) & ~P_C) | (v >= m ? (cyc(38), P_C) : (cyc(76), 0)));
}


/* The slide plus $F9 as the panel's Y, and its sprites' Y bytes (X =
   the panel's offset) at $0405-$042D, each through SpriteYOnScreen.
   Several adds take the carry the clamp's compare left. */
static void hud_sprites_y(uint32_t b)
{
    static const uint16_t row[4][2] = {{0x0411, 0x0421}, {0x0415, 0x0425}, {0x0419, 0x0429}, {0x041D, 0x042D}};
    static const struct { uint8_t add; int clc; uint16_t at; } tail[3] = {{0x0A, 0, 0x0405}, {0x18, 0, 0x0409}, {0x26, 1, 0x040D}};
    uint8_t v0, v;
    int i;

    op_sep(P_M);
    cpu.p &= (uint8_t)~P_X;
    cyc(92);
    v0 = rd8(b | 0x0D57);
    cpu.p &= (uint8_t)~P_C;
    cyc(30);
    v0 = adc8i(v0, rd8((uint16_t)(cpu.d + 0xF9)));
    v = v0;
    for (i = 0; i < 4; i++) {
        if (i) {
            cyc(i == 3 ? 14 : 36);   /* PLA, CLC, ADC; PHA unless the last */
            cpu.p &= (uint8_t)~P_C;
            v = adc8i(v, 0x10);
        }
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
        abi_SpriteYOnScreen();
        cyc(198);
        wr8(b | (uint16_t)(row[i][0] + cpu.x), (uint8_t)cpu.a);
        cyc(30);
        wr8(b | (uint16_t)(row[i][1] + cpu.x), (uint8_t)cpu.a);
        cpu.p = nz8(cpu.p, v);
    }
    for (i = 0; i < 3; i++) {
        cpu.p = nz8(cpu.p, v0);
        if (tail[i].clc)
            cpu.p &= (uint8_t)~P_C; else { cyc(8); }
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | v0);
        adc8i(v0, tail[i].add);
        abi_SpriteYOnScreen();
        cyc(212);
        wr8(b | (uint16_t)(tail[i].at + cpu.x), (uint8_t)cpu.a);
    }
}

/* Panel B's Y ($0E0B - $0D) for its two sprite pairs, and + $10. */
static void hud_panel_b_y(uint32_t b)
{
    uint8_t v = lda8(b | 0x0E0B);

    cpu.p |= P_C;
    v = op_sbc8(v, 0x0D, &cpu.p);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cyc(38);
    wr8(b | 0x0535, v);
    cyc(24);
    wr8(b | 0x053D, v);
    cpu.p &= (uint8_t)~P_C;
    v = adc8i(v, 0x10);
    cyc(52);
    wr8(b | 0x0539, v);
    cyc(24);
    wr8(b | 0x0541, v);
}

/* The map marker's Y ($0E16) and the column of Ys below it, $FF once a
   step carries. */
static void hud_marker_y(uint32_t b)
{
    static const struct { uint8_t add; uint16_t at[2]; int n; } step[6] = {
        {0x08, {0x0545, 0}, 1}, {0x06, {0x055D, 0x0561}, 2}, {0x08, {0x0565, 0x0569}, 2},
        {0x08, {0x0549, 0}, 1}, {0x08, {0x054D, 0x0551}, 2}, {0x08, {0x0555, 0x0559}, 2}};
    uint8_t v = lda8(b | 0x0E16);
    int i, j;

    cyc(24);
    wr8(b | 0x056D, v);
    cyc(24);
    wr8(b | 0x0571, v);
    for (i = 0; i < 6; i++) {
        cyc(50);
        cpu.p &= (uint8_t)~P_C;
        v = adc8i(v, step[i].add);
        if (!(cpu.p & P_C)) {
        } else {
            cyc(10);
            v = 0xFF;
            cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
            cpu.p = nz8(cpu.p, v);
        }
        for (j = 0; j < step[i].n; j++)
            { cyc(24); wr8(b | step[i].at[j], v); }
    }
}

/* Gauge B's X ($0E12) and the row of Xs after it: the first five skipped
   while negative, the rest carrying on (no CLC). */
static void hud_gauge_b_x(uint32_t b)
{
    static const struct { uint8_t add; int sub, clc, skip; uint16_t at[2]; int n; } step[11] = {
        {0x00, 0, 0, 1, {0x04A1, 0}, 1}, {0x08, 1, 0, 1, {0x04BD, 0}, 1},
        {0x10, 0, 1, 1, {0x04A5, 0}, 1}, {0x08, 0, 1, 1, {0x04A9, 0}, 1},
        {0x08, 0, 1, 1, {0x04AD, 0}, 1}, {0x08, 0, 0, 0, {0x04B5, 0}, 1},
        {0x08, 0, 0, 0, {0x04B9, 0x04B1}, 2}, {0x08, 0, 0, 0, {0x04C1, 0}, 1},
        {0x04, 0, 0, 0, {0x04C5, 0}, 1}, {0x08, 0, 0, 0, {0x04C9, 0}, 1},
        {0x08, 0, 0, 0, {0x04CD, 0}, 1}};
    uint8_t v = lda8(b | 0x0E12);
    int i, j;

    for (i = 0; i < 11; i++) {
        if (i) {
            cyc(14);
            if (step[i].sub) {
                cpu.p |= P_C;
                v = op_sbc8(v, step[i].add, &cpu.p);
                cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
            } else {
                if (step[i].clc)
                    { cyc(14); cpu.p &= (uint8_t)~P_C; }
                v = adc8i(v, step[i].add);
            }
        }
        if (step[i].skip && (cyc(16), (v & 0x80)))
            { cyc(6); continue; }
        for (j = 0; j < step[i].n; j++)
            { cyc(24); wr8(b | step[i].at[j], v); }
    }
}


/*
 * @routine 7E:AEE0 HudSlideOut
 * WHAT IT DOES: Moves the HUD slide $0D57 back by 4, down to $D0 (once it
 * is negative).
 * HOW: 8-bit subtract and compare through the data bank.
 * Entry: M 8-bit. Exit: A low byte = the new slide; flags from it.
 */
void abi_HudSlideOut(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint8_t v = lda8(b | 0x0D57);

    cpu.p |= P_C;
    v = op_sbc8(v, 0x04, &cpu.p);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    if (v & 0x80) {
        cmp8(v, 0xD0);
        if (v < 0xD0) {
            v = 0xD0;
            cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
            cpu.p = nz8(cpu.p, v);
        }
    }
    cyc(54);
    wr8(b | 0x0D57, v);
    cyc(42);
}

/* AE18 and AE30's body: $F9 = the panel's Y offset, X its sprites; by stage
   $0D54: below 8 nothing, 8 (slide in first for panel A) or above (out
   first, restarting the stage at $D0, for panel A) then the sprites. */
static void hud_panel_slide(uint8_t f9, uint16_t x, int panel_a)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint8_t p0 = cpu.p, v;

    cpu.p = (uint8_t)((cpu.p | P_M) & ~P_X);
    cpu.x = x;
    cyc(106);
    wr8(b | 0x00F9, f9);
    v = lda8_after(b | 0x0D54, panel_a ? 24 : 48);   /* A: STA, LDA; B: STA, LDX #, LDA */
    cmp8(v, 0x08);
    if (v < 0x08) {
        op_plp(p0);
        return;
    }
    if (panel_a) {
        if (v == 0x08)
            abi_HudSlideIn();
        else {
            v = lda8(b | 0x0D57);
            cmp8(v, 0xD0);
            if (v == 0xD0)
                wr8(b | 0x0D54, 0x00);
            abi_HudSlideOut();
        }
    }
    hud_sprites_y(b);
    cyc(28);
    op_plp(p0);
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
void abi_HudPanelASlide(void)
{
    cyc(24);
    hud_panel_slide(0xF8, 0x0020, 1);
}

/*
 * @routine 7E:AE30 HudPanelBSlide
 * WHAT IT DOES: In stage 8 and above places panel B's sprites (Y offset
 * $4A, sprite offset $120) without moving the slide.
 * HOW: As HudPanelASlide.
 * Entry: as HudPanelASlide. Exit: as HudPanelASlide.
 */
void abi_HudPanelBSlide(void)
{
    cyc(0);
    hud_panel_slide(0x4A, 0x0120, 0);
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
void abi_HudPanelsStep(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint8_t p0 = cpu.p, v;

    op_sep(P_M | P_X);
    cyc(46);
    cpu.x = rd8(b | 0x0D54);
    switch (cpu.x) {
    case 0x02:
        cyc(22);
        v = lda8(b | 0x0E16);
        cpu.p |= P_C;
        v = op_sbc8(v, 0x04, &cpu.p);
        /* CMP #$AC, BCS taken (38) or not, LDA #$AC (48) */
        cpu.p = (uint8_t)((nz8(cpu.p, (uint8_t)(v - 0xAC)) & ~P_C) | (v >= 0xAC ? P_C : 0));
        cyc(v >= 0xAC ? 38 : 48);
        if (v < 0xAC)
            v = 0xAC;
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
        cyc(38);
        wr8(b | 0x0E16, v);
        hud_marker_y(b);
        break;
    case 0x04:
        cyc(22);
        v = lda8(b | 0x0E12);
        cpu.p &= (uint8_t)~P_C;
        v = op_adc8(v, 0x04, &cpu.p);
        if (v & 0x80) {
            cyc(76);
        } else {
            cpu.p = (uint8_t)((nz8(cpu.p, (uint8_t)(v - 0x1F)) & ~P_C) | (v >= 0x1F ? P_C : 0));
            cyc(v >= 0x1F ? 118 : 108);
            if (v >= 0x1F)
                v = 0x1F;
        }
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
        wr8(b | 0x0E12, v);
        hud_gauge_b_x(b);
        break;
    case 0x06:
        cyc(22);
        v = lda8(b | 0x0E0B);
        cpu.p &= (uint8_t)~P_C;
        v = op_adc8(v, 0x04, &cpu.p);
        if (v & 0x80) {
            cyc(76);
        } else {
            cpu.p = (uint8_t)((nz8(cpu.p, (uint8_t)(v - 0x2A)) & ~P_C) | (v >= 0x2A ? P_C : 0));
            if (v >= 0x2A) {
                cyc(118);
                wr8(b | 0x0E0C, 0x7C);
                v = 0x2A;
                cyc(40);
            } else { cyc(108); }
        }
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
        wr8(b | 0x0E0B, v);
        hud_panel_b_y(b);
        break;
    case 0x08:
    case 0x0A:
        cyc(46);
        if (cpu.x == 0x08)
            abi_HudSlideIn();
        else
            { cyc(22); abi_HudSlideOut(); }
        cpu.p &= (uint8_t)~P_X;
        cpu.x = 0x0000;
        cyc(62);
        wr16((uint16_t)(cpu.d + 0xF9), 0x0000);
        hud_sprites_y(b);
        break;
    default:
        cyc(46);
        break;
    }
    cyc(28);
    op_plp(p0);
}
