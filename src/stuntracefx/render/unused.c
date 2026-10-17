/*
 * unused.c: Super FX code at 01:AD68-B0D8 that nothing in the game calls
 * or starts: two gauges at the screen's foot and one at its top right,
 * the rectangle fill, rectangle frame and pixel run they plot with, four
 * programs that clear stretches of Super FX RAM, and two sprite programs
 * (a 32 by 32 texture and a scaled object sprite).
 *
 * The C was drafted instruction for instruction from the ROM bytes and is
 * kept so: every register, flag and RAM access as the code makes them,
 * one per statement, a branch's delay slot after its decision. Returns
 * through registers go through unused_run's dispatch on the address they
 * reach.
 */
#include "gsu.h"
#include "gsu_ops.h"

#define R gsu.r

#define FX_TEX_BANK          0x00A6u
#define FX_TEX_BASE          0x00A8u
#define FX_SPRITE_SIZE       0x00AAu
#define FX_SPRITE_COLOUR     0x00ACu
#define FX_MODEL_BA          0x00BAu
#define FX_MODEL_BC          0x00BCu
#define FX_MODEL_CE          0x00CEu
#define FX_GAUGE_TOP         0x01ACu
#define FX_GAUGE_TOP_INNER   0x01AEu
#define GSU_WORD_1C4         0x01C4u
#define FX_GAUGE_LEFT        0x01CAu
#define FX_GAUGE_RIGHT       0x01CEu
#define FX_SPRITE_X          0x01D2u
#define FX_SPRITE_Y          0x01D4u

void abi_FxObjectSpriteAt(void);

/* The code from address at until it jumps to ret (the caller's return;
   $FFFF for a program, which ends at its STOP). */
static void unused_run(uint16_t at, uint16_t ret)
{
    int t;
    uint16_t nx;

    (void)t;
    (void)nx;
    goto dispatch;
    /* FxGaugeRight */
lAD68:
    gsu_stw(R[10], R[11]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[1] = 0x00B0;
    R[2] = 0x00B0;
    R[3] = 0x0028;
    R[4] = 0x0008;
    R[0] = 0x000D;
    R[11] = 0xAD7D;
    goto lAE5F;
lAD7D:
    R[3] = gsu_ldw(FX_GAUGE_RIGHT);
    R[3] = gsu_lob(R[3]);
    R[0] = 0x0024;
    gsu_sub(R[0], R[3], 0);   /* CMP */
    t = gsu.s == gsu.ov;
    if (t) goto lAD8B;
    R[3] = R[0];
lAD8B:
    R[1] = 0x00B2;
    R[2] = 0x00B2;
    R[4] = 0x0004;
    R[0] = 0x0007;
    R[11] = 0xAD9A;
    goto lAE3A;
lAD9A:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
    /* FxGaugeLeft */
lAD9F:
    gsu_stw(R[10], R[11]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[1] = 0x0008;
    R[2] = 0x00B0;
    R[3] = 0x0028;
    R[4] = 0x0008;
    R[0] = 0x000D;
    R[11] = 0xADB3;
    goto lAE5F;
lADB3:
    R[3] = gsu_ldw(FX_GAUGE_LEFT);
    R[3] = gsu_lob(R[3]);
    R[0] = 0x0024;
    gsu_sub(R[0], R[3], 0);   /* CMP */
    t = gsu.s == gsu.ov;
    if (t) goto lADC1;
    R[3] = R[0];
lADC1:
    R[1] = 0x000A;
    R[2] = 0x00B2;
    R[4] = 0x0004;
    R[0] = 0x0004;
    R[11] = 0xADCF;
    goto lAE3A;
lADCF:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
    /* FxGaugeTop */
lADD4:
    gsu_stw(R[10], R[11]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[0] = gsu_ldw(FX_GAUGE_TOP);
    R[0] = gsu_add(R[0], 10, 0);
    R[1] = gsu_lob(R[0]);
    R[0] = gsu_ldw(FX_GAUGE_TOP_INNER);
    R[0] = gsu_lob(R[0]);
    gsu_sub(R[0], R[1], 0);   /* CMP */
    t = !gsu.cy;
    if (t) goto lADED;
    R[0] = 0x0000;
    gsu_stw(FX_GAUGE_TOP_INNER, R[0]);
lADED:
    R[0] = gsu_ldw(FX_GAUGE_TOP);
    R[3] = gsu_lob(R[0]);
    t = gsu.z;
    if (t) goto lAE31;
    t = !gsu.s;
    if (t) goto lADFA;
    R[3] = gsu_lsr(R[3]);
lADFA:
    R[3] = gsu_inc(R[3]);
    R[3] = gsu_inc(R[3]);
    R[3] = gsu_inc(R[3]);
    R[3] = gsu_inc(R[3]);
    R[4] = 0x0006;
    R[0] = 0x00CE;
    R[1] = gsu_sub(R[0], R[3], 0);
    R[2] = 0x0002;
    R[5] = R[1];
    R[6] = R[2];
    R[0] = 0x000E;
    R[11] = 0xAE12;
    goto lAE5F;
lAE12:
    R[1] = R[5];
    R[2] = R[6];
    R[1] = gsu_inc(R[1]);
    R[1] = gsu_inc(R[1]);
    R[2] = gsu_inc(R[2]);
    R[2] = gsu_inc(R[2]);
    R[0] = gsu_ldw(FX_GAUGE_TOP_INNER);
    R[3] = gsu_lob(R[0]);
    R[0] = gsu_ldw(FX_GAUGE_TOP);
    R[0] = gsu_lob(R[0]);
    t = !gsu.s;
    if (t) goto lAE28;
    R[3] = gsu_lsr(R[3]);
lAE28:
    R[4] = 0x0002;
    R[0] = 0x0002;
    R[11] = 0xAE31;
    goto lAE3A;
lAE31:
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_stw(FX_GAUGE_TOP_INNER, R[0]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
    /* FxRectFill */
lAE3A:
    R[12] = R[0];
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_cmode(R[0]);
    R[0] = R[12];
    gsu_color((uint8_t)R[0]);
    R[0] = 0x0009;
    gsu_cmode(R[0]);
    R[3] = R[3];
    gsu_moves(R[3]);
    t = gsu.z;
    if (t) goto lAE5D;
    R[4] = R[4];
    gsu_moves(R[4]);
    t = gsu.z;
    if (t) goto lAE5D;
    /* CACHE */
lAE51:
    R[12] = R[3];
    gsu_moves(R[3]);
    R[13] = 0xAE55;
lAE55:
    t = gsu_count_down(&R[12]);
    gsu_plot_r1();
    if (t) goto lAE55;
    R[1] = gsu_sub(R[1], R[3], 0);
    R[4] = gsu_dec(R[4]);
    t = !gsu.z;
    R[2] = gsu_inc(R[2]);
    if (t) goto lAE51;
lAE5D:
    nx = (uint16_t)(R[11]);
    at = nx;
    goto dispatch;
    /* FxRectFrame */
lAE5F:
    /* CACHE */
    R[12] = R[0];
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_cmode(R[0]);
    R[0] = R[12];
    gsu_color((uint8_t)R[0]);
    R[0] = 0x0009;
    gsu_cmode(R[0]);
    R[12] = R[3];
    R[13] = 0xAE70;
lAE70:
    t = gsu_count_down(&R[12]);
    gsu_plot_r1();
    if (t) goto lAE70;
    R[12] = R[4];
    R[12] = gsu_dec(R[12]);
    R[12] = gsu_dec(R[12]);
    R[13] = 0xAE78;
lAE78:
    R[2] = gsu_inc(R[2]);
    R[1] = gsu_sub(R[1], R[3], 0);
    gsu_plot_r1();
    R[1] = gsu_add(R[1], R[3], 0);
    R[1] = gsu_dec(R[1]);
    R[1] = gsu_dec(R[1]);
    t = gsu_count_down(&R[12]);
    gsu_plot_r1();
    if (t) goto lAE78;
    R[1] = gsu_sub(R[1], R[3], 0);
    R[2] = gsu_inc(R[2]);
    R[12] = R[3];
    R[13] = 0xAE89;
lAE89:
    t = gsu_count_down(&R[12]);
    gsu_plot_r1();
    if (t) goto lAE89;
    nx = (uint16_t)(R[11]);
    at = nx;
    goto dispatch;
    /* FxPixelRun */
lAE8D:
    /* CACHE */
    R[12] = R[3];
    gsu_moves(R[3]);
    t = gsu.z;
    gsu_color((uint8_t)R[0]);
    if (t) goto lAE97;
    R[13] = 0xAE95;
lAE95:
    t = gsu_count_down(&R[12]);
    gsu_plot_r1();
    if (t) goto lAE95;
lAE97:
    nx = (uint16_t)(R[11]);
    at = nx;
    goto dispatch;
    /* FxClearScreenLong */
lAE99:
    R[0] = gsu_sub(R[0], R[0], 0);
    R[1] = 0x2C00;
    R[12] = 0x2000;
    /* CACHE */
    R[13] = 0xAEA3;
lAEA3:
    gsu_stw(R[1], R[0]);
    R[1] = gsu_inc(R[1]);
    t = gsu_count_down(&R[12]);
    R[1] = gsu_inc(R[1]);
    if (t) goto lAEA3;
    return;   /* the STOP: the program ends here */
    /* FxClearScreenShort */
lAEA9:
    R[0] = gsu_sub(R[0], R[0], 0);
    R[1] = 0x2C00;
    R[12] = 0x1800;
    /* CACHE */
    R[13] = 0xAEB3;
lAEB3:
    gsu_stw(R[1], R[0]);
    R[1] = gsu_inc(R[1]);
    t = gsu_count_down(&R[12]);
    R[1] = gsu_inc(R[1]);
    if (t) goto lAEB3;
    return;   /* the STOP: the program ends here */
    /* FxClearScreenIfSet */
lAECA:
    R[0] = gsu_ldw(GSU_WORD_1C4);
    R[0] = gsu_lob(R[0]);
    t = gsu.z;
    if (t) goto lAEDF;
    R[0] = gsu_sub(R[0], R[0], 0);
    R[1] = 0x2C00;
    R[12] = 0x1A00;
    /* CACHE */
    R[13] = 0xAEDB;
lAEDB:
    gsu_stw(R[1], R[0]);
    R[1] = gsu_inc(R[1]);
    t = gsu_count_down(&R[12]);
    R[1] = gsu_inc(R[1]);
    if (t) goto lAEDB;
lAEDF:
    return;   /* the STOP: the program ends here */
    /* FxClear6000IfSet */
lAEE1:
    R[0] = gsu_ldw(GSU_WORD_1C4);
    R[0] = gsu_lob(R[0]);
    t = gsu.z;
    if (t) goto lAEF6;
    R[0] = gsu_sub(R[0], R[0], 0);
    R[1] = 0x6000;
    R[12] = 0x1A00;
    /* CACHE */
    R[13] = 0xAEF2;
lAEF2:
    gsu_stw(R[1], R[0]);
    R[1] = gsu_inc(R[1]);
    t = gsu_count_down(&R[12]);
    R[1] = gsu_inc(R[1]);
    if (t) goto lAEF2;
lAEF6:
    return;   /* the STOP: the program ends here */
    /* FxSprite32Program */
lB027:
    R[0] = 0x0003;
    gsu_romb(R[0]);
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_cmode(R[0]);
    R[1] = 0x0010;
    R[0] = gsu_ldw(FX_SPRITE_COLOUR);
    R[0] = gsu_mult(R[0], R[1]);
    gsu_color((uint8_t)R[0]);
    R[2] = 0x0020;
    R[1] = gsu_ldw(FX_SPRITE_SIZE);
    R[0] = gsu_lob(R[1]);
    R[0] = gsu_and(R[0], R[2]);
    t = gsu.z;
    if (t) goto lB043;
    R[0] = 0x0004;
lB043:
    R[0] = gsu_or(R[0], 8);
    gsu_cmode(R[0]);
    R[0] = gsu_umult(R[1], 3);
    R[1] = 0x9D59;
    gsu_r14(gsu_add(R[0], R[1], 0));
    R[0] = gsu_getb();
    gsu_r14(gsu_inc(R[14]));
    R[0] = gsu_getbh(R[0]);
    gsu_r14(gsu_inc(R[14]));
    R[1] = gsu_getb();
    gsu_romb(R[1]);
    gsu_r14(R[0]);
    R[0] = 0xFFF0;
    R[1] = gsu_ldw(FX_SPRITE_X);
    R[1] = gsu_add(R[0], R[1], 0);
    R[2] = gsu_ldw(FX_SPRITE_Y);
    R[2] = gsu_add(R[0], R[2], 0);
    R[9] = R[1];
    R[11] = 0x0020;
    R[10] = 0x00E0;
    R[7] = 0xB079;
    R[8] = 0x0020;
    /* CACHE */
lB075:
    R[12] = R[8];
    R[13] = 0xB079;
lB079:
    gsu_color(gsu_getb());   /* GETC */
    gsu_r14(gsu_inc(R[14]));
    t = gsu_count_down(&R[12]);
    gsu_plot_r1();
    if (t) goto lB079;
    gsu_r14(gsu_add(R[14], R[10], 0));
    R[1] = R[9];
    R[11] = gsu_dec(R[11]);
    t = !gsu.z;
    R[2] = gsu_inc(R[2]);
    if (t) goto lB075;
    R[0] = gsu_rpix_r(R[1], R[2]);
    return;   /* the STOP: the program ends here */
    /* FxObjectSpriteProgram */
lB08A:
    R[10] = 0x0240;
    R[0] = 0x0003;
    gsu_romb(R[0]);
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_cmode(R[0]);
    R[0] = gsu_ldw(FX_SPRITE_X);
    gsu_stw(FX_MODEL_BA, R[0]);
    R[0] = gsu_ldw(FX_SPRITE_Y);
    gsu_stw(FX_MODEL_BC, R[0]);
    R[1] = 0x0010;
    R[0] = gsu_ldw(FX_SPRITE_COLOUR);
    R[0] = gsu_mult(R[0], R[1]);
    gsu_color((uint8_t)R[0]);
    R[2] = 0x0020;
    R[1] = gsu_ldw(FX_SPRITE_SIZE);
    R[0] = gsu_lob(R[1]);
    R[0] = gsu_and(R[0], R[2]);
    t = gsu.z;
    if (t) goto lB0B5;
    R[0] = 0x0004;
lB0B5:
    R[0] = gsu_or(R[0], 8);
    gsu_cmode(R[0]);
    R[0] = gsu_umult(R[1], 3);
    R[1] = 0x9D59;
    gsu_r14(gsu_add(R[0], R[1], 0));
    R[0] = gsu_getb();
    gsu_r14(gsu_inc(R[14]));
    R[0] = gsu_getbh(R[0]);
    gsu_r14(gsu_inc(R[14]));
    gsu_stw(FX_TEX_BASE, R[0]);
    R[0] = gsu_getb();
    gsu_stw(FX_TEX_BANK, R[0]);
    R[1] = gsu_ldw(FX_MODEL_CE);
    R[11] = 0xB0D5;
    abi_FxObjectSpriteAt();
lB0D5:
    R[0] = gsu_rpix_r(R[1], R[2]);
    return;   /* the STOP: the program ends here */
dispatch:
    if (at == ret)
        return;
    switch (at) {
    case 0xAD68: goto lAD68;
    case 0xAD7D: goto lAD7D;
    case 0xAD8B: goto lAD8B;
    case 0xAD9A: goto lAD9A;
    case 0xAD9F: goto lAD9F;
    case 0xADB3: goto lADB3;
    case 0xADC1: goto lADC1;
    case 0xADCF: goto lADCF;
    case 0xADD4: goto lADD4;
    case 0xADED: goto lADED;
    case 0xADFA: goto lADFA;
    case 0xAE12: goto lAE12;
    case 0xAE28: goto lAE28;
    case 0xAE31: goto lAE31;
    case 0xAE3A: goto lAE3A;
    case 0xAE51: goto lAE51;
    case 0xAE55: goto lAE55;
    case 0xAE5D: goto lAE5D;
    case 0xAE5F: goto lAE5F;
    case 0xAE70: goto lAE70;
    case 0xAE78: goto lAE78;
    case 0xAE89: goto lAE89;
    case 0xAE8D: goto lAE8D;
    case 0xAE95: goto lAE95;
    case 0xAE97: goto lAE97;
    case 0xAE99: goto lAE99;
    case 0xAEA3: goto lAEA3;
    case 0xAEA9: goto lAEA9;
    case 0xAEB3: goto lAEB3;
    case 0xAECA: goto lAECA;
    case 0xAEDB: goto lAEDB;
    case 0xAEDF: goto lAEDF;
    case 0xAEE1: goto lAEE1;
    case 0xAEF2: goto lAEF2;
    case 0xAEF6: goto lAEF6;
    case 0xB027: goto lB027;
    case 0xB043: goto lB043;
    case 0xB075: goto lB075;
    case 0xB079: goto lB079;
    case 0xB08A: goto lB08A;
    case 0xB0B5: goto lB0B5;
    case 0xB0D5: goto lB0D5;
    }
    return;   /* not an address the code jumps to */
}

/*
 * @routine 01:AD68 FxGaugeRight
 * WHAT IT DOES: Draws a gauge at the screen's lower right: a frame 40
 * by 8 at ($B0, $B0) in colour $0D (FxRectFrame), then a bar inside it
 * at ($B2, $B2), 4 rows of the byte $01CE's pixels (at most $24) in
 * colour 7 (FxRectFill). Nothing in the game calls it.
 * HOW: Pushes R11 on the R10 stack around the two calls; the length
 * capped with CMP / BGE; returns by pulling R15.
 * Entry: R10 = stack; R11 = return address; $01CE = the bar's length.
 * Exit: as FxRectFill leaves them; CMODE 9.
 */
void abi_FxGaugeRight(void)
{
    unused_run(0xAD68, R[11]);
}

/*
 * @routine 01:AD9F FxGaugeLeft
 * WHAT IT DOES: FxGaugeRight's twin at the lower left: the frame at
 * (8, $B0), the bar at ($0A, $B2) from the byte $01CA in colour 4.
 * Nothing in the game calls it.
 * HOW: As FxGaugeRight.
 * Entry: R10 = stack; R11 = return address; $01CA = the bar's length.
 * Exit: as FxRectFill leaves them; CMODE 9.
 */
void abi_FxGaugeLeft(void)
{
    unused_run(0xAD9F, R[11]);
}

/*
 * @routine 01:ADD4 FxGaugeTop
 * WHAT IT DOES: A gauge at the top right whose width is its value:
 * with the byte $01AC not 0, a frame 6 rows high and 4 + $01AC pixels
 * wide (4 + half when bit 7 is set) ending at x = $CE, y = 2, in
 * colour $0E, and inside it a bar 2 rows high of the byte $01AE's
 * pixels (halved likewise) in colour 2. $01AE is cleared first when it
 * is at least $01AC + 10 (low bytes), and always at the end. Nothing
 * in the game calls it.
 * HOW: Pushes R11 on the R10 stack; R5/R6 keep the frame's corner over
 * FxRectFrame; returns by pulling R15.
 * Entry: R10 = stack; R11 = return address; $01AC, $01AE.
 * Exit: $01AE = 0; R0 = 0; the rest as the last call leaves them.
 */
void abi_FxGaugeTop(void)
{
    unused_run(0xADD4, R[11]);
}

/*
 * @routine 01:AE3A FxRectFill
 * WHAT IT DOES: Fills a rectangle: R4 rows of R3 pixels in colour R0
 * from (R1, R2) down; nothing when R3 or R4 is 0.
 * HOW: CMODE 0 for the COLOR, then CMODE 9; per row a LOOP of PLOT
 * (R12 = R3, R13 = $AE55), R1 back by R3 and R2 one down; returns by
 * JMP R11.
 * Entry: R0 = colour, R1, R2 = top left, R3 = width, R4 = height; R11
 * = return address.
 * Exit: R1 back at the left, R2 below the last row, R4 = 0 (unless R3
 * was 0), R12 = 0; CMODE 9.
 */
void abi_FxRectFill(void)
{
    unused_run(0xAE3A, R[11]);
}

/*
 * @routine 01:AE5F FxRectFrame
 * WHAT IT DOES: Draws a rectangle's outline: R3 pixels across from
 * (R1, R2), the sides R4 - 2 rows down, the bottom row, in colour R0.
 * HOW: CMODE 0 for the COLOR, then CMODE 9; three LOOPs of PLOT (top,
 * both sides a row at a time, bottom); returns by JMP R11.
 * Entry: R0 = colour, R1, R2 = top left, R3 = width, R4 = height (at
 * least 3); R11 = return address.
 * Exit: R1 one past the bottom row, R2 on it, R12 = 0; CMODE 9.
 */
void abi_FxRectFrame(void)
{
    unused_run(0xAE5F, R[11]);
}

/*
 * @routine 01:AE8D FxPixelRun
 * WHAT IT DOES: Plots R3 pixels in colour R0 from (R1, R2) across;
 * nothing when R3 is 0.
 * HOW: COLOR in the branch's delay slot; a LOOP of PLOT; returns by
 * JMP R11.
 * Entry: R0 = colour, R1, R2 = start, R3 = count; R11 = return
 * address.
 * Exit: R1 past the run, R12 = 0.
 */
void abi_FxPixelRun(void)
{
    unused_run(0xAE8D, R[11]);
}

/*
 * @routine 01:AE99 FxClearScreenLong
 * WHAT IT DOES: A program: clears $2000 words of Super FX RAM from
 * $2C00 (to $6C00), then STOP.
 * HOW: A LOOP of STW, INC R1 twice (R12 = $2000).
 * Entry: run as a program (nothing in the game starts it).
 * Exit: (at the STOP) R0 = 0, R1 = $6C00, R12 = 0.
 */
void abi_FxClearScreenLong(void)
{
    unused_run(0xAE99, 0xFFFF);
}

/*
 * @routine 01:AEA9 FxClearScreenShort
 * WHAT IT DOES: A program: clears $1800 words from $2C00 (to $5C00),
 * then STOP.
 * HOW: As FxClearScreenLong, R12 = $1800.
 * Entry: run as a program (nothing in the game starts it).
 * Exit: (at the STOP) R0 = 0, R1 = $5C00, R12 = 0.
 */
void abi_FxClearScreenShort(void)
{
    unused_run(0xAEA9, 0xFFFF);
}

/*
 * @routine 01:AECA FxClearScreenIfSet
 * WHAT IT DOES: A program: when the byte $01C4 is not 0, clears $1A00
 * words from $2C00 (to $6000); then STOP.
 * HOW: LOB of $01C4; a LOOP of STW (R12 = $1A00).
 * Entry: run as a program (nothing in the game starts it); $01C4.
 * Exit: (at the STOP) R0 = 0; R1 = $6000 and R12 = 0 when it cleared.
 */
void abi_FxClearScreenIfSet(void)
{
    unused_run(0xAECA, 0xFFFF);
}

/*
 * @routine 01:AEE1 FxClear6000IfSet
 * WHAT IT DOES: A program: when the byte $01C4 is not 0, clears $1A00
 * words from $6000 (to $9400); then STOP.
 * HOW: As FxClearScreenIfSet from $6000.
 * Entry: run as a program (nothing in the game starts it); $01C4.
 * Exit: (at the STOP) R0 = 0; R1 = $9400 and R12 = 0 when it cleared.
 */
void abi_FxClear6000IfSet(void)
{
    unused_run(0xAEE1, 0xFFFF);
}

/*
 * @routine 01:B027 FxSprite32Program
 * WHAT IT DOES: A program: draws a 32 by 32 sprite centred on ($01D2,
 * $01D4): texture $00AA's address from the table at $03:9D59 (three
 * bytes each), its texels plotted with GETC row by row, colour $00AC
 * times 16 (CMODE 8, or $0C when bit 5 of $00AA is set); then STOP.
 * HOW: ROMB 3 for the table, then the texture's bank; rows of 32 by
 * LOOP (R13 = $B079), R14 past the row's $E0 others; R11 counts the
 * rows; RPIX writes the pixel caches out before the STOP.
 * Entry: run as a program (nothing in the game starts it); $00AA, $00AC,
 * $01D2, $01D4.
 * Exit: (at the STOP) as the drawing leaves them.
 */
void abi_FxSprite32Program(void)
{
    unused_run(0xB027, 0xFFFF);
}

/*
 * @routine 01:B08A FxObjectSpriteProgram
 * WHAT IT DOES: A program: draws texture $00AA as a scaled object
 * sprite at ($01D2, $01D4) (into $00BA, $00BC) with size $00CE: colour
 * and CMODE as FxSprite32Program, the texture's address and bank into
 * $00A8 and $00A6, then FxObjectSpriteAt; then STOP.
 * HOW: R10 = $0240; ROMB 3; LINK to 01:B17D; RPIX before the STOP.
 * Entry: run as a program (nothing in the game starts it); $00AA, $00AC,
 * $00CE, $01D2, $01D4 and
 * the object's set-up FxObjectSprite reads.
 * Exit: (at the STOP) as FxObjectSpriteAt leaves them.
 */
void abi_FxObjectSpriteProgram(void)
{
    unused_run(0xB08A, 0xFFFF);
}
