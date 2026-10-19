/*
 * unused.c: Super FX code of bank 01 that nothing in the game calls or
 * starts (01:AD68-B0D8, 01:8450-8560, 01:B42F-B6D4 and smaller pieces):
 * two gauges at the screen's foot and one at its top right, the rectangle
 * fill, rectangle frame and pixel run they plot with, four programs that
 * clear stretches of Super FX RAM, two sprite programs (a 32 by 32
 * texture and a scaled object sprite), a byte rotation matrix, a pixel
 * test, two model-drawing programs, a text drawer at a 3D point, a
 * row-copy program, a point-list projector, a byte-angle turn, a
 * RAM-clear program, a NOP run and a spare return; and a 16-bit square
 * root, a vector normalizer with its test programs, a table byte and a
 * swapped ratio clamp (01:8068-8185, 01:946D, 01:9652).
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
void abi_FxRatioClamp(void);
void abi_FxSqrt32(void);

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

void abi_FxModelDrawFlagged(void);
void abi_FxModelDrawFlags8000(void);
void abi_FxModelDrawWithFlags(void);

/* The code of 01:8450-8560 from address at until it jumps to ret (as
   unused_run). */
static void unused84_run(uint16_t at, uint16_t ret)
{
    int t;
    uint16_t nx;

    (void)t;
    (void)nx;
    goto dispatch;
l8450:
    R[1] = 0x973E;
    gsu_r14(gsu_add(R[1], R[1], 0));
    R[4] = gsu_getb();
    gsu_r14(gsu_add(R[1], R[2], 0));
    R[6] = gsu_getb();
    gsu_r14(gsu_add(R[1], R[3], 0));
    R[8] = gsu_getb();
    R[1] = 0x96FE;
    gsu_r14(gsu_add(R[1], R[1], 0));
    R[5] = gsu_getb();
    gsu_r14(gsu_add(R[1], R[2], 0));
    R[7] = gsu_getb();
    gsu_r14(gsu_add(R[1], R[3], 0));
    R[3] = gsu_getb();
    R[0] = gsu_mult(R[8], R[7]);
    R[1] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_mult(R[8], R[6]);
    R[2] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_mult(R[3], R[7]);
    R[12] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_mult(R[3], R[6]);
    R[13] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_hib(R[12]);
    R[0] = gsu_mult(R[0], R[5]);
    R[0] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_add(R[0], R[2], 0);
    R[0] = gsu_hib(R[0]);
    gsu_stb(R[9], (uint8_t)R[0]);
    R[9] = gsu_inc(R[9]);
    R[0] = gsu_hib(R[1]);
    R[0] = gsu_mult(R[0], R[5]);
    R[0] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_sub(R[0], R[13], 0);
    R[0] = gsu_hib(R[0]);
    gsu_stb(R[9], (uint8_t)R[0]);
    R[9] = gsu_inc(R[9]);
    R[0] = gsu_mult(R[7], R[4]);
    R[0] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_hib(R[0]);
    gsu_stb(R[9], (uint8_t)R[0]);
    R[9] = gsu_inc(R[9]);
    R[0] = gsu_mult(R[4], R[3]);
    R[0] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_hib(R[0]);
    gsu_stb(R[9], (uint8_t)R[0]);
    R[9] = gsu_inc(R[9]);
    R[0] = gsu_mult(R[4], R[8]);
    R[0] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_hib(R[0]);
    gsu_stb(R[9], (uint8_t)R[0]);
    R[9] = gsu_inc(R[9]);
    R[0] = gsu_not(R[5]);
    R[0] = gsu_inc(R[0]);
    gsu_stb(R[9], (uint8_t)R[0]);
    R[9] = gsu_inc(R[9]);
    R[0] = gsu_hib(R[13]);
    R[0] = gsu_mult(R[0], R[5]);
    R[0] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_sub(R[0], R[1], 0);
    R[0] = gsu_hib(R[0]);
    gsu_stb(R[9], (uint8_t)R[0]);
    R[9] = gsu_inc(R[9]);
    R[0] = gsu_hib(R[2]);
    R[0] = gsu_mult(R[0], R[5]);
    R[0] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_add(R[0], R[12], 0);
    R[0] = gsu_hib(R[0]);
    gsu_stb(R[9], (uint8_t)R[0]);
    R[9] = gsu_inc(R[9]);
    R[0] = gsu_mult(R[6], R[4]);
    R[0] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_hib(R[0]);
    gsu_stb(R[9], (uint8_t)R[0]);
    nx = (uint16_t)(R[11]);
    at = nx;
    goto dispatch;
l84CB:
    R[0] = 0xFFFF;
    gsu_color((uint8_t)R[0]);
    R[1] = 0x0000;
    R[2] = 0x0040;
    R[12] = 0x0020;
    R[13] = 0x84D6;
l84D6:
    t = gsu_count_down(&R[12]);
    gsu_plot_r1();
    if (t) goto l84D6;
    return;   /* the STOP: the program ends here */
l84DA:
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu.rambr = (uint8_t)(R[0] & 1);
    gsu_romb(R[0]);
    R[10] = 0x0240;
    R[0] = gsu_ldw(0x0028);
    gsu_stw(R[10], R[0]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[1] = 0x0080;
    R[0] = gsu_add(R[0], R[1], 0);
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_sbk(R[0]);
    R[11] = 0x84F3;
    abi_FxModelDrawFlagged();
l84F3:
    R[0] = gsu_ldw(0x0020);
    R[0] = gsu_swap(R[0]);
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_sbk(R[0]);
    R[0] = gsu_ldw(0x0022);
    R[0] = gsu_swap(R[0]);
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_sbk(R[0]);
    R[0] = gsu_ldw(0x0024);
    R[0] = gsu_swap(R[0]);
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_sbk(R[0]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    R[0] = gsu_ldw(R[10]);
    gsu_stw(0x0028, R[0]);
    R[11] = 0x8513;
    abi_FxModelDrawFlags8000();
l8513:
    return;   /* the STOP: the program ends here */
l8515:
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu.rambr = (uint8_t)(R[0] & 1);
    gsu_romb(R[0]);
    R[10] = 0x0240;
    R[4] = gsu_ldw(0x001A);
    gsu_r14(0x0012);
    gsu_r14(gsu_add(R[4], R[14], 0));
    R[0] = gsu_getb();
    gsu_r14(gsu_inc(R[14]));
    R[0] = gsu_getbh(R[0]);
    gsu_stw(0x004A, R[0]);
    gsu_r14(0x0007);
    gsu_r14(gsu_add(R[4], R[14], 0));
    R[0] = gsu_getb();
    gsu_stw(0x0032, R[0]);
    gsu_r14(0x0000);
    gsu_r14(gsu_add(R[4], R[14], 0));
    R[0] = gsu_getb();
    gsu_r14(gsu_inc(R[14]));
    R[0] = gsu_getbh(R[0]);
    gsu_stw(0x0016, R[0]);
    gsu_r14(0x0002);
    gsu_r14(gsu_add(R[4], R[14], 0));
    R[0] = gsu_getb();
    gsu_stw(0x001C, R[0]);
    gsu_r14(0x0003);
    gsu_r14(gsu_add(R[4], R[14], 0));
    R[0] = gsu_getb();
    gsu_r14(gsu_inc(R[14]));
    R[0] = gsu_getbh(R[0]);
    gsu_stw(0x0018, R[0]);
    R[0] = 0x8000;
    R[11] = 0x855F;
    abi_FxModelDrawWithFlags();
l855F:
    return;   /* the STOP: the program ends here */
dispatch:
    if (at == ret)
        return;
    switch (at) {
    case 0x8450: goto l8450;
    case 0x84CB: goto l84CB;
    case 0x84D6: goto l84D6;
    case 0x84DA: goto l84DA;
    case 0x84F3: goto l84F3;
    case 0x8513: goto l8513;
    case 0x8515: goto l8515;
    case 0x855F: goto l855F;
    }
    return;   /* not an address the code jumps to */
}

/*
 * @routine 01:8450 FxMatrixFromAnglesBytes
 * WHAT IT DOES: Meant to build a rotation matrix of signed bytes at R9
 * from the angle bytes in R1, R2 and R3 (the sines at $01:973E and the
 * cosines at $01:96FE, a byte an angle): nine bytes, the rows as
 * FxMatrixFromAngles01's. The first angle's two lookups do not use R1:
 * IWT R1 overwrites it with the table address before it is added, so they
 * read the bytes at twice the table addresses. Nothing in the game calls
 * it.
 * HOW: GETB from each table at its address + the angle; products with
 * MULT (signed 8 by 8), doubled (ADD R0), the high byte (HIB) kept; each
 * element stored with STB, R9 stepping by one.
 * Entry: R2, R3 = the second and third angles; R9 = where to store; R11 =
 * return address. Exit: R9 = R9 + 8; R0 = the last element; R1-R8, R12,
 * R13 as the code leaves them; R14 and the ROM buffer move as on the Super
 * FX.
 */
void abi_FxMatrixFromAnglesBytes(void)
{
    unused84_run(0x8450, R[11]);
}

/*
 * @routine 01:84CB FxPlotTestProgram
 * WHAT IT DOES: A program: plots 32 pixels in colour $FF along the row at
 * Y $40 from X 0, then STOP.
 * HOW: COLOR, then a LOOP of PLOT (R12 = $20, R13 the loop).
 * Entry: run as a program (nothing in the game starts it). Exit: (at the
 * STOP) R1 = $20, R2 = $40, R12 = 0.
 */
void abi_FxPlotTestProgram(void)
{
    unused84_run(0x84CB, 0xFFFF);
}

/*
 * @routine 01:84DA FxModelMirrorProgram
 * WHAT IT DOES: A program: draws the object as FxModelDrawFlagged does,
 * then once more with its three angles ($0020-$0024) negated in place
 * (each word's bytes swapped, then the two's complement) and the model
 * flags $8000 (FxModelDrawFlags8000), with $0028 kept on the stack across
 * the first draw and put back; then STOP. On the way it stores $0028 +
 * $80 and clears it again into the last RAM address (SBK of 0).
 * HOW: The RAM and ROM banks 0, the stack R10 at $0240; two LINK calls.
 * Entry: run as a program (nothing in the game starts it); the object's
 * set-up in RAM. Exit: (at the STOP) as the second draw leaves the
 * registers.
 */
void abi_FxModelMirrorProgram(void)
{
    unused84_run(0x84DA, 0xFFFF);
}

/*
 * @routine 01:8515 FxModelRecordProgram
 * WHAT IT DOES: A program: takes an object's set-up from its record (the
 * address at $001A, in ROM bank 0): +$12 (word) to $004A, +$07 (byte) to
 * $0032, +$00 (word) to $0016, +$02 (byte) to $001C and +$03 (word) to
 * $0018; then draws it with model flags $8000 (FxModelDrawWithFlags);
 * then STOP.
 * HOW: The RAM and ROM banks 0, the stack R10 at $0240; GETB, GETBH from
 * R14 = the record + the field.
 * Entry: run as a program (nothing in the game starts it); $001A = the
 * record. Exit: (at the STOP) as the draw leaves the registers.
 */
void abi_FxModelRecordProgram(void)
{
    unused84_run(0x8515, 0xFFFF);
}

void abi_FxProjectPoint(void);
void abi_FxDivide32By16Halving(void);

/* The code of 01:B42F-B6D4 from address at until it jumps to ret (as
   unused_run). */
static void unusedb4_run(uint16_t at, uint16_t ret)
{
    int t;
    uint16_t nx;

    (void)t;
    (void)nx;
    goto dispatch;
lB42F:
    gsu_stw(R[10], R[11]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_color((uint8_t)R[0]);
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu.rambr = (uint8_t)(R[0] & 1);
    gsu_cmode(R[0]);
    R[0] = 0x0080;
    R[1] = gsu_ldw(0x0026);
    R[2] = gsu_ldw(0x0028);
    R[3] = gsu_ldw(0x002A);
    gsu_sub(R[0], R[3], 0);   /* CMP */
    t = gsu.s;
    if (t) goto lB44F;
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
lB44F:
    R[11] = 0xB454;
    abi_FxProjectPoint();
lB454:
    gsu_stw(0x0026, R[1]);
    gsu_stw(0x0028, R[2]);
    R[0] = gsu_ldw(0x001C);
    gsu_romb(R[0]);
    /* CACHE */
    gsu_r14(gsu_ldw(0x001A));
    R[12] = 0xFFFF;
lB465:
    R[0] = gsu_getb();
    gsu_r14(gsu_inc(R[14]));
    R[0] = gsu_lob(R[0]);
    t = !gsu.z;
    R[12] = gsu_inc(R[12]);
    if (t) goto lB465;
    R[3] = 0x0010;
    R[0] = gsu_mult(R[12], R[3]);
    gsu_stw(R[10], R[0]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[2] = 0x007F;
    R[1] = gsu_ldw(0x00C8);
    R[1] = gsu_add(R[1], R[2], 0);
    R[2] = gsu_umult(R[1], R[12]);
    R[3] = gsu_ldw(0x002A);
    R[11] = 0xB485;
    abi_FxProjectPoint();
lB485:
    R[0] = 0x0065;
    R[1] = gsu_sub(R[1], R[0], 0);
    R[0] = 0x0033;
    R[2] = gsu_sub(R[2], R[0], 0);
    R[1] = gsu_lob(R[1]);
    gsu_stw(0x00C8, R[1]);
    R[3] = 0xFFFF;
    R[0] = gsu_add(R[0], 0, 0);
    R[0] = gsu_ror(R[2]);
    R[0] = gsu_sub(R[0], R[3], !gsu.cy);
    R[3] = gsu_ldw(0x0026);
    R[0] = gsu_sub(R[3], R[0], 0);
    gsu_sbk(R[0]);
    R[3] = 0xFFFF;
    R[0] = gsu_add(R[0], 0, 0);
    R[0] = gsu_ror(R[1]);
    R[0] = gsu_sub(R[0], R[3], !gsu.cy);
    R[3] = gsu_ldw(0x0028);
    R[0] = gsu_sub(R[3], R[0], 0);
    gsu_sbk(R[0]);
    R[3] = gsu_not(R[1]);
    R[3] = gsu_inc(R[3]);
    gsu_stw(0x00B2, R[3]);
    R[3] = 0x00E0;
    R[3] = gsu_sub(R[3], R[1], 0);
    gsu_stw(0x00B4, R[3]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    R[6] = gsu_ldw(R[10]);
    R[0] = gsu_sub(R[2], R[6], 0);
    t = !gsu.cy;
    if (t) goto lB4CC;
    goto lB57D;
lB4CC:
    R[0] = R[2];
    R[4] = 0x0000;
    R[11] = 0xB4D5;
    abi_FxDivide32By16Halving();
lB4D5:
    R[9] = R[4];
    R[8] = R[4];
    R[3] = 0x0000;
    gsu_stw(0x00B2, R[3]);
    /* CACHE */
    R[2] = gsu_ldw(0x0028);
    R[6] = 0x0000;
    R[4] = 0x0000;
lB4E6:
    R[2] = R[2];
    gsu_moves(R[2]);
    t = !gsu.s;
    if (t) goto lB4EF;
    goto lB55A;
lB4EF:
    R[0] = 0x00BF;
    gsu_sub(R[0], R[2], 0);   /* CMP */
    t = !gsu.s;
    if (t) goto lB4FB;
    goto lB55A;
lB4FB:
    R[1] = gsu_ldw(0x0026);
    R[7] = 0x0000;
    R[5] = gsu_ldw(0x001A);
lB503:
    R[0] = gsu_ldw(0x001C);
    gsu_romb(R[0]);
    gsu_r14(R[5]);
    R[0] = gsu_getb();
    R[5] = gsu_inc(R[5]);
    R[0] = gsu_add(R[0], R[0], 0);
    t = gsu.z;
    if (t) goto lB55A;
    R[3] = 0x0014;
    gsu_romb(R[3]);
    R[3] = 0x0010;
    R[0] = gsu_umult(R[0], R[3]);
    R[3] = 0xD2BA;
    R[0] = gsu_add(R[0], R[4], 0);
    R[0] = gsu_add(R[0], R[4], 0);
    gsu_r14(gsu_add(R[0], R[3], 0));
    R[0] = gsu_ldw(0x00B2);
    gsu_sub(R[0], R[1], 0);   /* CMP */
    t = gsu.s;
    if (t) goto lB530;
    R[0] = gsu_ldw(0x00C8);
    R[1] = gsu_add(R[0], R[1], 0);
    t = 1;
    if (t) goto lB503;
lB530:
    R[3] = gsu_ldw(0x00B4);
    R[3] = gsu_dec(R[3]);
    R[3] = gsu_dec(R[3]);
    gsu_sub(R[3], R[1], 0);   /* CMP */
    t = gsu.s;
    if (t) goto lB55A;
    R[0] = gsu_getb();
    gsu_r14(gsu_inc(R[14]));
    R[0] = gsu_getbh(R[0]);
    R[12] = 0x0010;
    R[13] = 0xB543;
lB543:
    R[7] = gsu_sub(R[7], R[9], 0);
    t = !gsu.cy;
    R[0] = gsu_rol(R[0]);
    if (t) goto lB54D;
    t = gsu_count_down(&R[12]);
    if (t) goto lB543;
    t = 1;
    if (t) goto lB503;
lB54D:
    t = gsu.cy;
    if (t) goto lB555;
    t = gsu_count_down(&R[12]);
    R[1] = gsu_inc(R[1]);
    if (t) goto lB543;
    t = 1;
    if (t) goto lB503;
lB555:
    t = gsu_count_down(&R[12]);
    gsu_plot_r1();
    if (t) goto lB543;
    t = 1;
    if (t) goto lB503;
lB55A:
    R[6] = gsu_sub(R[6], R[8], 0);
    t = !gsu.cy;
    if (t) goto lB56A;
    R[4] = gsu_inc(R[4]);
    R[3] = 0x0010;
    R[0] = gsu_sub(R[4], R[3], 0);
    t = !gsu.z;
    if (t) goto lB55A;
    t = 1;
    if (t) goto lB578;
lB56A:
    R[2] = gsu_inc(R[2]);
    R[4] = gsu_inc(R[4]);
    R[3] = 0x0010;
    R[0] = gsu_sub(R[4], R[3], 0);
    t = gsu.z;
    if (t) goto lB577;
    goto lB4E6;
lB577:
lB578:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
lB57D:
    R[0] = R[6];
    R[6] = R[2];
    gsu_sub(R[0], R[6], 0);   /* CMP */
    t = !gsu.z;
    if (t) goto lB58C;
    R[4] = 0xFFFF;
    t = 1;
    if (t) goto lB593;
lB58C:
    R[4] = 0x0000;
    R[11] = 0xB593;
    abi_FxDivide32By16Halving();
lB593:
    R[9] = R[4];
    R[8] = R[4];
    /* CACHE */
    R[2] = gsu_ldw(0x0028);
    R[6] = 0x0000;
    R[4] = 0x0000;
lB59F:
    R[2] = R[2];
    gsu_moves(R[2]);
    t = !gsu.s;
    if (t) goto lB5A8;
    goto lB67E;
lB5A8:
    R[0] = 0x00BF;
    gsu_sub(R[0], R[2], 0);   /* CMP */
    t = !gsu.s;
    if (t) goto lB5B4;
    goto lB67E;
lB5B4:
    R[1] = gsu_ldw(0x0026);
    R[7] = 0x0000;
    R[5] = gsu_ldw(0x001A);
lB5BC:
    R[0] = gsu_ldw(0x001C);
    gsu_romb(R[0]);
    gsu_r14(R[5]);
    R[0] = gsu_getb();
    R[5] = gsu_inc(R[5]);
    R[0] = gsu_add(R[0], R[0], 0);
    t = !gsu.z;
    if (t) goto lB5CD;
    goto lB67E;
lB5CD:
    R[3] = 0x0014;
    gsu_romb(R[3]);
    R[3] = 0x0010;
    R[0] = gsu_umult(R[0], R[3]);
    R[3] = 0xD2BA;
    R[0] = gsu_add(R[0], R[4], 0);
    R[0] = gsu_add(R[0], R[4], 0);
    gsu_r14(gsu_add(R[0], R[3], 0));
    R[0] = gsu_ldw(0x00B2);
    gsu_sub(R[0], R[1], 0);   /* CMP */
    t = gsu.s;
    if (t) goto lB5E9;
    goto lB66E;
lB5E9:
    R[1] = R[1];
    gsu_moves(R[1]);
    t = !gsu.s;
    R[0] = gsu_getb();
    if (t) goto lB618;
    gsu_r14(gsu_inc(R[14]));
    R[0] = gsu_getbh(R[0]);
    R[12] = 0x0010;
    R[13] = 0xB5F5;
lB5F5:
    R[0] = gsu_rol(R[0]);
    t = gsu.cy;
    if (t) goto lB603;
lB5F9:
    R[7] = gsu_sub(R[7], R[9], 0);
    t = gsu.cy;
    R[1] = gsu_inc(R[1]);
    if (t) goto lB5F9;
    t = gsu_count_down(&R[12]);
    if (t) goto lB5F5;
    t = 1;
    if (t) goto lB5BC;
lB603:
    R[7] = gsu_sub(R[7], R[9], 0);
    t = gsu.cy;
    R[1] = gsu_inc(R[1]);
    if (t) goto lB603;
    t = gsu.s;
    if (t) goto lB613;
    R[13] = 0xB629;
    t = gsu_count_down(&R[12]);
    if (t) goto lB629;
    t = 1;
    if (t) goto lB5BC;
lB613:
    t = gsu_count_down(&R[12]);
    if (t) goto lB5F5;              /* R13 is still $B5F5 on this path */
    t = 1;
    if (t) goto lB5BC;
lB618:
    gsu_r14(gsu_inc(R[14]));
    R[3] = gsu_ldw(0x00B4);
    R[3] = gsu_dec(R[3]);
    gsu_sub(R[3], R[1], 0);   /* CMP */
    t = gsu.s;
    if (t) goto lB642;
    R[0] = gsu_getbh(R[0]);
    R[12] = 0x0010;
    R[13] = 0xB629;
lB629:
    R[0] = gsu_rol(R[0]);
    t = gsu.cy;
    if (t) goto lB637;
lB62D:
    R[7] = gsu_sub(R[7], R[9], 0);
    t = gsu.cy;
    R[1] = gsu_inc(R[1]);
    if (t) goto lB62D;
    t = gsu_count_down(&R[12]);
    if (t) goto lB629;
    t = 1;
    if (t) goto lB5BC;
lB637:
    R[7] = gsu_sub(R[7], R[9], 0);
    t = gsu.cy;
    gsu_plot_r1();
    if (t) goto lB637;
    t = gsu_count_down(&R[12]);
    if (t) goto lB629;
    goto lB5BC;
lB642:
    R[0] = gsu_getbh(R[0]);
    R[3] = 0x00DF;
    R[12] = 0x0010;
    R[13] = 0xB64B;
lB64B:
    R[0] = gsu_rol(R[0]);
    t = gsu.cy;
    if (t) goto lB65F;
lB64F:
    R[7] = gsu_sub(R[7], R[9], 0);
    t = gsu.cy;
    R[1] = gsu_inc(R[1]);
    if (t) goto lB64F;
    gsu_sub(R[1], R[3], 0);   /* CMP */
    t = !gsu.s;
    if (t) goto lB67E;
    t = gsu_count_down(&R[12]);
    if (t) goto lB64B;
    t = 1;
    if (t) goto lB67E;
lB65F:
    gsu_sub(R[1], R[3], 0);   /* CMP */
    t = !gsu.s;
    if (t) goto lB67E;
    R[7] = gsu_sub(R[7], R[9], 0);
    t = gsu.cy;
    gsu_plot_r1();
    if (t) goto lB65F;
    t = gsu_count_down(&R[12]);
    if (t) goto lB64B;
    t = 1;
    if (t) goto lB67E;
lB66E:
    R[3] = gsu_ldw(0x00C8);
    R[1] = gsu_add(R[1], R[3], 0);
    R[7] = gsu_sub(R[7], R[9], 0);
    t = !gsu.cy;
    if (t) goto lB679;
    R[1] = gsu_inc(R[1]);
lB679:
    goto lB5BC;
lB67D:
lB67E:
    R[2] = gsu_inc(R[2]);
    R[6] = gsu_sub(R[6], R[8], 0);
    t = !gsu.cy;
    if (t) goto lB689;
    goto lB59F;
lB689:
    R[4] = gsu_inc(R[4]);
    R[3] = 0x0010;
    R[0] = gsu_sub(R[4], R[3], 0);
    t = gsu.z;
    if (t) goto lB695;
    goto lB59F;
lB695:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
lB69A:
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu.rambr = (uint8_t)(R[0] & 1);
    gsu_romb(R[0]);
    /* CACHE */
    R[4] = gsu_ldw(0x0026);
    R[0] = 0x0100;
    R[0] = gsu_umult(R[0], R[4]);
    R[0] = gsu_add(R[0], R[0], 0);
    R[4] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_ldw(0x0028);
    R[0] = gsu_umult(R[0], 4);
    R[0] = gsu_add(R[0], R[4], 0);
    R[4] = 0x2C00;
    R[4] = gsu_add(R[0], R[4], 0);
    R[5] = 0x0400;
    gsu_r14(gsu_ldw(0x0018));
    R[3] = 0x0004;
lB6BE:
    R[1] = R[4];
    R[12] = 0x0050;
    R[13] = 0xB6C4;
lB6C4:
    R[0] = gsu_getb();
    gsu_r14(gsu_inc(R[14]));
    R[0] = gsu_getbh(R[0]);
    gsu_r14(gsu_inc(R[14]));
    gsu_stw(R[1], R[0]);
    R[1] = gsu_inc(R[1]);
    t = gsu_count_down(&R[12]);
    R[1] = gsu_inc(R[1]);
    if (t) goto lB6C4;
    R[4] = gsu_add(R[4], R[5], 0);
    R[3] = gsu_dec(R[3]);
    t = !gsu.z;
    if (t) goto lB6BE;
    return;   /* the STOP: the program ends here */
dispatch:
    if (at == ret)
        return;
    switch (at) {
    case 0xB42F: goto lB42F;
    case 0xB44F: goto lB44F;
    case 0xB454: goto lB454;
    case 0xB465: goto lB465;
    case 0xB485: goto lB485;
    case 0xB4CC: goto lB4CC;
    case 0xB4D5: goto lB4D5;
    case 0xB4E6: goto lB4E6;
    case 0xB4EF: goto lB4EF;
    case 0xB4FB: goto lB4FB;
    case 0xB503: goto lB503;
    case 0xB530: goto lB530;
    case 0xB543: goto lB543;
    case 0xB54D: goto lB54D;
    case 0xB555: goto lB555;
    case 0xB55A: goto lB55A;
    case 0xB56A: goto lB56A;
    case 0xB577: goto lB577;
    case 0xB578: goto lB578;
    case 0xB57D: goto lB57D;
    case 0xB58C: goto lB58C;
    case 0xB593: goto lB593;
    case 0xB59F: goto lB59F;
    case 0xB5A8: goto lB5A8;
    case 0xB5B4: goto lB5B4;
    case 0xB5BC: goto lB5BC;
    case 0xB5CD: goto lB5CD;
    case 0xB5E9: goto lB5E9;
    case 0xB5F5: goto lB5F5;
    case 0xB5F9: goto lB5F9;
    case 0xB603: goto lB603;
    case 0xB613: goto lB613;
    case 0xB618: goto lB618;
    case 0xB629: goto lB629;
    case 0xB62D: goto lB62D;
    case 0xB637: goto lB637;
    case 0xB642: goto lB642;
    case 0xB64B: goto lB64B;
    case 0xB64F: goto lB64F;
    case 0xB65F: goto lB65F;
    case 0xB66E: goto lB66E;
    case 0xB679: goto lB679;
    case 0xB67D: goto lB67D;
    case 0xB67E: goto lB67E;
    case 0xB689: goto lB689;
    case 0xB695: goto lB695;
    case 0xB69A: goto lB69A;
    case 0xB6BE: goto lB6BE;
    case 0xB6C4: goto lB6C4;
    }
    return;   /* not an address the code jumps to */
}

/*
 * @routine 01:B42F FxText3D
 * WHAT IT DOES: Draws the zero-terminated string at $001A (ROM bank
 * $001C) at the 3D point ($0026, $0028, $002A) in colour R0: the point
 * is projected (FxProjectPoint) and kept at $0026/$0028, the string's
 * width (16 a character) and the scale from $00C8 projected again, and
 * each character drawn from the 16 by 16 font at $14:D2BA, scaled
 * (FxDivide32By16Halving) and clipped to the screen ($00B2/$00B4, rows
 * up to $BF, columns up to $DF). Nothing is drawn when the point is at
 * depth $80 or nearer. Nothing in the game calls it.
 * HOW: R11 kept on the R10 stack; RAM bank 0, CMODE 0; two drawing loops
 * (a character narrower or wider than its scaled size) of GETB/GETBH rows,
 * ROL out of each bit, LOOP and PLOT; returns by pulling R15.
 * Entry: R0 = colour; R10 = stack; R11 = return address; $001A/$001C =
 * the string; $0026-$002A = the point; $00C8 = the size. Exit: as the
 * code leaves the registers, flags and RAM; R14 and the ROM buffer move
 * as on the Super FX.
 */
void abi_FxText3D(void)
{
    unusedb4_run(0xB42F, R[11]);
}

/*
 * @routine 01:B603 FxText3DLeftColumn
 * WHAT IT DOES: FxText3D's wide drawing loop at a set font bit of a
 * character that starts left of the screen: the columns the bit covers
 * are counted (R1) without plotting while R1 is negative, and once R1
 * reaches the screen the rest of the row is drawn by the loop at
 * 01:B629. A unit start on the row's last bit (R12 = 1) with R1 just
 * left of the screen and further left runs its two ends of row, which
 * no play or unit start has reached so far, so the C is judged against
 * the original.
 * HOW: WITH R7; SUB R9 and BCS for each column, INC R1 in the delay
 * slot; BMI to a LOOP back to the bit loop; else R13 = 01:B629 and LOOP;
 * at the row's end a BRA to the next character (01:B5BC).
 * Entry: as FxText3D has them in its wide loop: R1 = the column, R7 the
 * step sum, R9 the step, R12 the bits left, R13 = 01:B5F5, R2 the row,
 * R4 the font row, R5 the string, R6 and R8 the row sum and step;
 * R10 = stack.
 * Exit: as FxText3D's.
 */
void abi_FxText3DLeftColumn(void)
{
    unusedb4_run(0xB603, R[11]);
}

/*
 * @routine 01:B67D FxText3DRowEndNop
 * WHAT IT DOES: The NOP after the wide loop's jump back to its next
 * character, which no path reaches (the jump before it always goes);
 * it runs on into the row's end at 01:B67E (the next row, or the
 * return after 16). A unit start runs it so the C is judged against
 * the original.
 * HOW: NOP; then as FxText3D from 01:B67E.
 * Entry: as FxText3D has them at a row's end: R2 the row, R4 the font
 * row, R6 and R8 the row sum and step; R10 = stack.
 * Exit: as FxText3D's.
 */
void abi_FxText3DRowEndNop(void)
{
    unusedb4_run(0xB67D, R[11]);
}

/*
 * @routine 01:B69A FxRowsCopyProgram
 * WHAT IT DOES: A program: copies four rows of $50 words from the ROM at
 * $0018 (bank 0) into the screen in Super FX RAM at $2C00 + $0026 * $400
 * + $0028 * 4, a row every $400 bytes; then STOP. Nothing in the game
 * starts it.
 * HOW: RAM and ROM banks 0; GETB, GETBH and STW in a LOOP (R12 = $50),
 * the destination stepping two bytes a word.
 * Entry: run as a program; $0018 = the source, $0026/$0028 = the place.
 * Exit: (at the STOP) R3 = 0, R12 = 0; R14 past the last word read.
 */
void abi_FxRowsCopyProgram(void)
{
    unusedb4_run(0xB69A, 0xFFFF);
}

/* The smaller uncalled pieces of bank 01 (01:96E7, 97D9, C0E0, C4A5,
   D120) from address at until they jump to ret (as unused_run). */
static void unused_misc_run(uint16_t at, uint16_t ret)
{
    int t;
    uint16_t nx;

    (void)t;
    (void)nx;
    goto dispatch;
lC0E0:
lC126:
    /* CACHE */
    nx = (uint16_t)(R[11]);
    at = nx;
    goto dispatch;
l96E7:
    gsu_stw(R[10], R[11]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_stw(0x0040, R[0]);
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_stw(0x014A, R[0]);
l96F2:
    R[0] = gsu_ldw(R[7]);
    R[7] = gsu_inc(R[7]);
    R[7] = gsu_inc(R[7]);
    R[9] = gsu_ldw(0x0026);
    R[1] = gsu_add(R[0], R[9], 0);
    R[0] = gsu_ldw(R[7]);
    R[7] = gsu_inc(R[7]);
    R[7] = gsu_inc(R[7]);
    R[9] = gsu_ldw(0x0028);
    R[2] = gsu_add(R[0], R[9], 0);
    R[0] = gsu_ldw(R[7]);
    R[7] = gsu_inc(R[7]);
    R[7] = gsu_inc(R[7]);
    R[9] = gsu_ldw(0x002A);
    R[3] = gsu_add(R[0], R[9], 0);
    R[11] = 0x970F;
    abi_FxProjectPoint();
l970F:
    gsu_stw(R[8], R[1]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    gsu_stw(R[8], R[2]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    gsu_stw(R[8], R[0]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    R[3] = gsu_ldw(0x014A);
    R[0] = gsu_or(R[0], R[3]);
    gsu_sbk(R[0]);
    R[0] = gsu_ldw(0x0040);
    R[0] = gsu_dec(R[0]);
    t = !gsu.z;
    gsu_sbk(R[0]);
    if (t) goto l96F2;
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
l97D9:
    gsu_stw(R[10], R[11]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[0] = 0x973E;
    gsu_r14(gsu_add(R[0], R[2], 0));
    R[0] = gsu_getb();
    R[5] = gsu_swap(R[0]);
    R[0] = 0x96FE;
    gsu_r14(gsu_add(R[0], R[2], 0));
    R[0] = gsu_getb();
    R[9] = gsu_swap(R[0]);
    R[6] = R[1];
    { uint32_t p = gsu_lmult(R[9], R[6]); R[4] = (uint16_t)p; R[8] = (uint16_t)(p >> 16); }
    R[7] = R[4];
    R[6] = R[3];
    { uint32_t p = gsu_lmult(R[5], R[6]); R[4] = (uint16_t)p; R[0] = (uint16_t)(p >> 16); }
    R[7] = gsu_add(R[7], R[4], 0);
    R[0] = gsu_add(R[0], R[8], gsu.cy);
    R[2] = gsu_asr(R[0]);
    R[6] = R[3];
    { uint32_t p = gsu_lmult(R[9], R[6]); R[4] = (uint16_t)p; R[8] = (uint16_t)(p >> 16); }
    R[7] = R[4];
    R[6] = R[1];
    { uint32_t p = gsu_lmult(R[5], R[6]); R[4] = (uint16_t)p; R[0] = (uint16_t)(p >> 16); }
    R[7] = gsu_sub(R[7], R[4], 0);
    R[0] = gsu_sub(R[0], R[8], !gsu.cy);
    R[3] = gsu_asr(R[0]);
    R[1] = R[2];
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
lD120:
    R[1] = 0x0200;
    R[12] = 0x3F00;
    R[13] = 0xD128;
lD128:
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_stw(R[1], R[0]);
    R[1] = gsu_inc(R[1]);
    t = gsu_count_down(&R[12]);
    R[1] = gsu_inc(R[1]);
    if (t) goto lD128;
    return;   /* the STOP: the program ends here */
lC4A5:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
dispatch:
    if (at == ret)
        return;
    switch (at) {
    case 0x96E7: goto l96E7;
    case 0x96F2: goto l96F2;
    case 0x970F: goto l970F;
    case 0x97D9: goto l97D9;
    case 0xC0E0: goto lC0E0;
    case 0xC126: goto lC126;
    case 0xC4A5: goto lC4A5;
    case 0xD120: goto lD120;
    case 0xD128: goto lD128;
    }
    return;   /* not an address the code jumps to */
}

/*
 * @routine 01:C0E0 FxNopReturn
 * WHAT IT DOES: Returns to R11 after a run of 70 NOPs (and a CACHE at
 * 01:C126): padding with a return at its end. Nothing in the game calls
 * it.
 * HOW: NOPs, CACHE, MOVE R15, R11.
 * Entry: R11 = return address. Exit: registers and flags unchanged; the
 * cache base at 01:C120.
 */
void abi_FxNopReturn(void)
{
    unused_misc_run(0xC0E0, R[11]);
}

/*
 * @routine 01:C126 FxNopReturnCache
 * WHAT IT DOES: The end of FxNopReturn: CACHE, six NOPs, return to R11.
 * HOW: As FxNopReturn.
 * Entry: R11 = return address. Exit: as FxNopReturn.
 */
void abi_FxNopReturnCache(void)
{
    unused_misc_run(0xC126, R[11]);
}

/*
 * @routine 01:96E7 FxProjectPoints
 * WHAT IT DOES: Projects R0 points: each three words at R7 (X, Y, Z),
 * moved by ($0026, $0028, $002A), goes through FxProjectPoint, and its
 * screen X, Y and result word are stored at R8; the results are ORed
 * together into $014A. Nothing in the game calls it.
 * HOW: The count kept at $0040 (DEC, SBK); R11 on the R10 stack;
 * returns by pulling R15.
 * Entry: R0 = count (1 or more); R7 = the points; R8 = where to store;
 * R10 = stack; R11 = return address. Exit: R7, R8 past the lists; $0040 =
 * 0; $014A = the OR of the results; the rest as the last FxProjectPoint
 * leaves them.
 */
void abi_FxProjectPoints(void)
{
    unused_misc_run(0x96E7, R[11]);
}

/*
 * @routine 01:97D9 FxRotateXZByte
 * WHAT IT DOES: Turns the point (X, Z) = (R1, R3) by the angle byte R2,
 * with s and c the signed bytes at $01:973E + R2 and $01:96FE + R2:
 * R1 = R2 = (X c + Z s) / 512 and R3 = (X s - Z c) / 512, each the high
 * word of a 32-bit sum, halved. Nothing in the game calls it.
 * HOW: GETB from each table, SWAP into the high byte; LMULT products
 * summed with ADD/ADC and SUB/SBC, then ASR; R11 on the R10 stack;
 * returns by pulling R15.
 * Entry: R1, R3 = the point; R2 = the angle; R10 = stack; R11 = return
 * address. Exit: R1 = R2 = the new X, R3 = the new Z; R4-R9 as the code
 * leaves them; R14 and the ROM buffer move as on the Super FX.
 */
void abi_FxRotateXZByte(void)
{
    unused_misc_run(0x97D9, R[11]);
}

/*
 * @routine 01:D120 FxClearRam200Program
 * WHAT IT DOES: A program: clears $3F00 words of Super FX RAM from
 * $0200 (to $7E00), then STOP. Nothing in the game starts it.
 * HOW: A LOOP of STW, INC R1 twice (R12 = $3F00).
 * Entry: run as a program. Exit: (at the STOP) R0 = 0, R1 = $7E00, R12
 * = 0.
 */
void abi_FxClearRam200Program(void)
{
    unused_misc_run(0xD120, 0xFFFF);
}

/*
 * @routine 01:C4A5 FxPopReturnC4A5
 * WHAT IT DOES: Returns to the address on top of the R10 stack: a second
 * copy of the return just before it at 01:C4A0, which nothing reaches.
 * HOW: DEC R10 twice, TO R15, LDW (R10).
 * Entry: R10 = stack, the return address on its top. Exit: R10 two
 * lower.
 */
void abi_FxPopReturnC4A5(void)
{
    unused_misc_run(0xC4A5, R[11]);
}

/* Bank 01 code nothing reaches around FxSqrt32 and the projection code
   (01:8068-8095, 01:80E5-8185, 01:946D-9479, 01:9652-9657): the code from
   address at until it jumps to ret. */
static void unused80_run(uint16_t at, uint16_t ret)
{
    int t;
    uint16_t nx;

    (void)t;
    (void)nx;
    goto dispatch;
l8068:
    R[6] = 0x0000;
    R[4] = 0x0000;
    R[12] = 0x0008;
    R[13] = 0x8072;
l8072:
    /* reached with the WITH R3 of 01:8071 or of the LOOP's delay slot */
    R[3] = gsu_add(R[3], R[3], 0);
    R[4] = gsu_rol(R[4]);
    R[3] = gsu_add(R[3], R[3], 0);
    R[4] = gsu_rol(R[4]);
    R[6] = gsu_add(R[6], R[6], 0);
    R[0] = gsu_add(R[6], R[6], 0);
    gsu_sub(R[0], R[4], 0);   /* CMP */
    t = gsu.cy;
    if (t) goto l8085;
    R[4] = gsu_sub(R[4], R[0], !gsu.cy);
    R[6] = gsu_inc(R[6]);
l8085:
    t = gsu_count_down(&R[12]);
    if (t) goto l8072;
    nx = (uint16_t)(R[11]);
    at = nx;
    goto dispatch;
l8089:
    R[5] = 0xFFFF;
    R[4] = 0x7FFF;
    R[11] = 0x8094;
    abi_FxSqrt32();
l8094:
    return;   /* the STOP: the program ends here */
l80E5:
    R[1] = 0x0100;
    R[2] = 0x0100;
    R[3] = 0x7D00;
    R[11] = 0x80F3;
    goto l80FE;
l80F3:
    return;   /* the STOP: the program ends here */
l80F5:
    R[1] = gsu_sub(R[4], R[1], 0);
    R[2] = gsu_sub(R[5], R[2], 0);
    R[3] = gsu_sub(R[6], R[3], 0);
l80FE:
    gsu_stw(R[10], R[11]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[6] = R[1];
    { uint32_t p = gsu_lmult(R[1], R[6]); R[4] = (uint16_t)p; R[0] = (uint16_t)(p >> 16); }
    R[5] = R[4];
    R[6] = R[2];
    { uint32_t p = gsu_lmult(R[2], R[6]); R[4] = (uint16_t)p; R[6] = (uint16_t)(p >> 16); }
    R[5] = gsu_add(R[5], R[4], 0);
    R[0] = gsu_add(R[0], R[6], gsu.cy);
    R[6] = R[3];
    { uint32_t p = gsu_lmult(R[3], R[6]); R[4] = (uint16_t)p; R[6] = (uint16_t)(p >> 16); }
    R[5] = gsu_add(R[5], R[4], 0);
    R[0] = gsu_add(R[0], R[6], gsu.cy);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = !gsu.z;
    if (t) goto l8132;
    R[5] = R[5];
    gsu_moves(R[5]);
    t = !gsu.z;
    if (t) goto l8132;
    R[1] = 0x0000;
    R[2] = 0x0000;
    R[3] = 0x0000;
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
l8132:
    R[4] = R[0];
    R[11] = 0x8139;
    abi_FxSqrt32();
l8139:
    R[0] = R[6];
    R[9] = 0x0000;
l813D:
    R[0] = gsu_lsr(R[0]);
    t = gsu.z;
    if (t) goto l8144;
    t = 1;
    R[9] = gsu_inc(R[9]);
    if (t) goto l813D;
l8144:
    R[0] = gsu_sub(R[0], R[0], 0);
    R[4] = 0x7FFF;
    R[12] = R[9];
    gsu_moves(R[9]);
    t = gsu.z;
    if (t) goto l8153;
    R[13] = 0x814F;
l814F:
    R[4] = gsu_add(R[4], R[4], 0);
    t = gsu_count_down(&R[12]);
    R[0] = gsu_rol(R[0]);
    if (t) goto l814F;
l8153:
    R[6] = R[6];
    R[11] = 0x815A;
    abi_FxDivide32By16Halving();
l815A:
    R[6] = R[4];
    { uint32_t p = gsu_lmult(R[1], R[6]); R[4] = (uint16_t)p; R[7] = (uint16_t)(p >> 16); }
    R[1] = R[4];
    { uint32_t p = gsu_lmult(R[2], R[6]); R[4] = (uint16_t)p; R[8] = (uint16_t)(p >> 16); }
    R[2] = R[4];
    { uint32_t p = gsu_lmult(R[3], R[6]); R[4] = (uint16_t)p; R[0] = (uint16_t)(p >> 16); }
    R[3] = R[4];
    R[12] = R[9];
    gsu_moves(R[9]);
    t = gsu.z;
    if (t) goto l8181;
    R[13] = 0x8174;
l8174:
    R[7] = gsu_asr(R[7]);
    R[1] = gsu_ror(R[1]);
    R[8] = gsu_asr(R[8]);
    R[2] = gsu_ror(R[2]);
    R[0] = gsu_asr(R[0]);
    R[3] = gsu_ror(R[3]);
    t = gsu_count_down(&R[12]);
    if (t) goto l8174;
l8181:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
l946D:
    gsu_r14(gsu_dec(R[14]));
    R[0] = gsu_getb();
    gsu_r14(gsu_add(R[14], 7, 0));
    R[1] = 0x07C6;
    R[0] = gsu_add(R[0], R[1], 0);
    R[0] = gsu_ldb(R[0]);
    nx = (uint16_t)(R[11]);
    R[0] = gsu_swap(R[0]);
    at = nx;
    goto dispatch;
l9652:
    R[0] = R[1];
    R[1] = R[2];
    R[2] = R[0];
    abi_FxRatioClamp();   /* runs on into it */
    return;
dispatch:
    if (at == ret)
        return;
    switch (at) {
    case 0x8068: goto l8068;
    case 0x8072: goto l8072;
    case 0x8085: goto l8085;
    case 0x8089: goto l8089;
    case 0x8094: goto l8094;
    case 0x80E5: goto l80E5;
    case 0x80F3: goto l80F3;
    case 0x80F5: goto l80F5;
    case 0x80FE: goto l80FE;
    case 0x8132: goto l8132;
    case 0x8139: goto l8139;
    case 0x813D: goto l813D;
    case 0x8144: goto l8144;
    case 0x814F: goto l814F;
    case 0x8153: goto l8153;
    case 0x815A: goto l815A;
    case 0x8174: goto l8174;
    case 0x8181: goto l8181;
    case 0x946D: goto l946D;
    case 0x9652: goto l9652;
    }
    return;   /* not an address the code jumps to */
}

/*
 * @routine 01:8068 FxSqrt16
 * WHAT IT DOES: The integer square root of R3: R6 = the root, R4 = the
 * remainder. Nothing calls it; a direct start is its only way in.
 * HOW: 8 passes, each shifting the next two bits of R3 into R4 and
 * doubling R6; when R4 is above twice R6 (R0), R4 loses R0 plus 1 and R6
 * gains 1 (CMP, BCS, SBC). As in FxSqrt32, the pass loop starts at the
 * ADD R3 after the first WITH R3 ($8072), the LOOP's delay slot being the
 * WITH R3 of the next pass.
 * Entry: R3 = value; R11 = return address. Exit: R6 = root, R4 =
 * remainder; R3 = 0; R0 = twice the root before its last bit; R12 = 0;
 * R13 = $8072.
 */
void abi_FxSqrt16(void)
{
    unused80_run(0x8068, R[11]);
}

/*
 * @routine 01:8089 FxSqrt32Test
 * WHAT IT DOES: A program nothing starts: FxSqrt32 of $7FFF:FFFF, then
 * STOP.
 * HOW: IWT R5, IWT R4, LINK, IWT R15.
 * Entry: the S-CPU starts it. Exit: (at its STOP) as FxSqrt32.
 */
void abi_FxSqrt32Test(void)
{
    unused80_run(0x8089, 0xFFFF);
}

/*
 * @routine 01:80E5 FxVectorNormalizeTest
 * WHAT IT DOES: A program nothing starts: the vector ($100, $100, $7D00)
 * normalized (FxVectorNormalize), then STOP.
 * HOW: IWT R1-R3, LINK, IWT R15.
 * Entry: the S-CPU starts it; R10 = stack. Exit: (at its STOP) as
 * FxVectorNormalize.
 */
void abi_FxVectorNormalizeTest(void)
{
    unused80_run(0x80E5, 0xFFFF);
}

/*
 * @routine 01:80F5 FxVectorNormalizeDelta
 * WHAT IT DOES: The vector from (R1, R2, R3) to (R4, R5, R6), normalized:
 * (R1, R2, R3) = (R4 - R1, R5 - R2, R6 - R3), then FxVectorNormalize.
 * Nothing calls it.
 * HOW: Three FROM/TO SUBs, then on into FxVectorNormalize.
 * Entry: R1-R6 as above; R10 = stack; R11 = return address. Exit: as
 * FxVectorNormalize.
 */
void abi_FxVectorNormalizeDelta(void)
{
    unused80_run(0x80F5, R[11]);
}

/*
 * @routine 01:80FE FxVectorNormalize
 * WHAT IT DOES: Scales the vector (R1, R2, R3) to length $7FFF: its squared
 * length (R0:R5, from three LMULTs) square-rooted by FxSqrt32, $7FFF << n
 * divided by that root (FxDivide32By16Halving, n the root's bit count),
 * each component multiplied by the quotient and shifted back right n
 * times. The zero vector stays zero. Nothing calls it except its test and
 * FxVectorNormalizeDelta.
 * HOW: Pushes R11 on the R10 stack; the 32-bit shifts as ASR/ROR pairs in
 * a LOOP; returns by pulling R15.
 * Entry: R1, R2, R3 = vector; R10 = stack; R11 = return address. Exit:
 * R1, R2, R3 = the scaled vector; R9 = n; the rest as the code leaves
 * them.
 */
void abi_FxVectorNormalize(void)
{
    unused80_run(0x80FE, R[11]);
}

/*
 * @routine 01:946D FxTable7C6Byte
 * WHAT IT DOES: The byte of the RAM table at $07C6 that the ROM byte before
 * R14 indexes, as the high byte of R0; R14 then 7 past that byte. Nothing
 * calls it.
 * HOW: DEC R14, GETB, ADD #7 to R14; LDB through R0; SWAP in the JMP R11's
 * delay slot.
 * Entry: R14 = ROM pointer; R11 = return address. Exit: R0 = the byte <<
 * 8; R1 = $07C6; R14 as above.
 */
void abi_FxTable7C6Byte(void)
{
    unused80_run(0x946D, R[11]);
}

/*
 * @routine 01:9652 FxRatioClampSwapped
 * WHAT IT DOES: FxRatioClamp with R1 and R2 swapped first. Nothing calls
 * it.
 * HOW: Three MOVEs through R0, then on into FxRatioClamp.
 * Entry: as FxRatioClamp, the pair swapped. Exit: as FxRatioClamp.
 */
void abi_FxRatioClampSwapped(void)
{
    unused80_run(0x9652, R[11]);
}
