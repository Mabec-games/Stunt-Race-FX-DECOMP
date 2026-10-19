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
    goto enter;   /* the caller fetched the first byte */
    /* FxGaugeRight */
lAD68:
    gsu_code(0x01ad69, 2);
    gsu_stw(R[10], R[11]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[1] = 0x00B0;
    R[2] = 0x00B0;
    R[3] = 0x0028;
    R[4] = 0x0008;
    R[0] = 0x000D;
    R[11] = 0xAD7D;
    gsu_code(0x01ad6b, 18); gsu_code(0x01ae5f, 1); gsu_cache(0xae60);
    goto lAE5F;
lAD7D:
    gsu_code(0x01ad7e, 3);
    R[3] = gsu_ldw(FX_GAUGE_RIGHT);
    R[3] = gsu_lob(R[3]);
    R[0] = 0x0024;
    gsu_sub(R[0], R[3], 0);   /* CMP */
    t = gsu.s == gsu.ov;
    gsu_code(0x01ad81, 8);
    if (t) { gsu_code(0x01ad8b, 1); goto lAD8B; }
    R[3] = R[0];
    gsu_code(0x01ad89, 3);
lAD8B:
    R[1] = 0x00B2;
    R[2] = 0x00B2;
    R[4] = 0x0004;
    R[0] = 0x0007;
    R[11] = 0xAD9A;
    gsu_code(0x01ad8c, 14); gsu_code(0x01ae3a, 1);
    goto lAE3A;
lAD9A:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x01ad9b, 4);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
    /* FxGaugeLeft */
lAD9F:
    gsu_code(0x01ada0, 2);
    gsu_stw(R[10], R[11]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[1] = 0x0008;
    R[2] = 0x00B0;
    R[3] = 0x0028;
    R[4] = 0x0008;
    R[0] = 0x000D;
    R[11] = 0xADB3;
    gsu_code(0x01ada2, 17); gsu_code(0x01ae5f, 1); gsu_cache(0xae60);
    goto lAE5F;
lADB3:
    gsu_code(0x01adb4, 3);
    R[3] = gsu_ldw(FX_GAUGE_LEFT);
    R[3] = gsu_lob(R[3]);
    R[0] = 0x0024;
    gsu_sub(R[0], R[3], 0);   /* CMP */
    t = gsu.s == gsu.ov;
    gsu_code(0x01adb7, 8);
    if (t) { gsu_code(0x01adc1, 1); goto lADC1; }
    R[3] = R[0];
    gsu_code(0x01adbf, 3);
lADC1:
    R[1] = 0x000A;
    R[2] = 0x00B2;
    R[4] = 0x0004;
    R[0] = 0x0004;
    R[11] = 0xADCF;
    gsu_code(0x01adc2, 13); gsu_code(0x01ae3a, 1);
    goto lAE3A;
lADCF:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x01add0, 4);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
    /* FxGaugeTop */
lADD4:
    gsu_code(0x01add5, 2);
    gsu_stw(R[10], R[11]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_code(0x01add7, 5);
    R[0] = gsu_ldw(FX_GAUGE_TOP);
    R[0] = gsu_add(R[0], 10, 0);
    R[1] = gsu_lob(R[0]);
    gsu_code(0x01addc, 7);
    R[0] = gsu_ldw(FX_GAUGE_TOP_INNER);
    R[0] = gsu_lob(R[0]);
    gsu_sub(R[0], R[1], 0);   /* CMP */
    t = !gsu.cy;
    gsu_code(0x01ade3, 5);
    if (t) { gsu_code(0x01aded, 1); goto lADED; }
    R[0] = 0x0000;
    gsu_code(0x01ade8, 6);
    gsu_stw(FX_GAUGE_TOP_INNER, R[0]);
lADED:
    gsu_code(0x01adee, 3);
    R[0] = gsu_ldw(FX_GAUGE_TOP);
    R[3] = gsu_lob(R[0]);
    t = gsu.z;
    gsu_code(0x01adf1, 4);
    if (t) { gsu_code(0x01ae31, 1); goto lAE31; }
    t = !gsu.s;
    gsu_code(0x01adf5, 3);
    if (t) { gsu_code(0x01adfa, 1); goto lADFA; }
    R[3] = gsu_lsr(R[3]);
    gsu_code(0x01adf8, 3);
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
    gsu_code(0x01adfb, 23); gsu_code(0x01ae5f, 1); gsu_cache(0xae60);
    goto lAE5F;
lAE12:
    R[1] = R[5];
    R[2] = R[6];
    R[1] = gsu_inc(R[1]);
    R[1] = gsu_inc(R[1]);
    R[2] = gsu_inc(R[2]);
    R[2] = gsu_inc(R[2]);
    gsu_code(0x01ae13, 11);
    R[0] = gsu_ldw(FX_GAUGE_TOP_INNER);
    R[3] = gsu_lob(R[0]);
    gsu_code(0x01ae1e, 5);
    R[0] = gsu_ldw(FX_GAUGE_TOP);
    R[0] = gsu_lob(R[0]);
    t = !gsu.s;
    gsu_code(0x01ae23, 3);
    if (t) { gsu_code(0x01ae28, 1); goto lAE28; }
    R[3] = gsu_lsr(R[3]);
    gsu_code(0x01ae26, 3);
lAE28:
    R[4] = 0x0002;
    R[0] = 0x0002;
    R[11] = 0xAE31;
    gsu_code(0x01ae29, 8); gsu_code(0x01ae3a, 1);
    goto lAE3A;
lAE31:
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_code(0x01ae32, 4);
    gsu_stw(FX_GAUGE_TOP_INNER, R[0]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x01ae36, 4);
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
    gsu_code(0x01ae3b, 16);
    if (t) { gsu_code(0x01ae5d, 1); goto lAE5D; }
    R[4] = R[4];
    gsu_moves(R[4]);
    t = gsu.z;
    gsu_code(0x01ae4b, 5);
    if (t) { gsu_code(0x01ae5d, 1); goto lAE5D; }
    /* CACHE */
    gsu_code(0x01ae50, 1); gsu_cache(0xae51); gsu_code(0x01ae51, 1);
lAE51:
    R[12] = R[3];
    gsu_moves(R[3]);
    R[13] = 0xAE55;
    gsu_code(0x01ae52, 4);
lAE55:
    t = gsu_count_down(&R[12]);
    gsu_code(0x01ae56, 1);
    gsu_code(t ? 0x01ae55 : 0x01ae57, 1);   /* the delay slot runs after the next fetch */
    gsu_plot_r1();
    if (t) goto lAE55;
    R[1] = gsu_sub(R[1], R[3], 0);
    R[4] = gsu_dec(R[4]);
    t = !gsu.z;
    R[2] = gsu_inc(R[2]);
    gsu_code(0x01ae58, 5);
    if (t) { gsu_code(0x01ae51, 1); goto lAE51; }
    gsu_code(0x01ae5d, 1);
lAE5D:
    nx = (uint16_t)(R[11]);
    at = nx;
    gsu_code(0x01ae5e, 1);
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
    gsu_code(0x01ae60, 17);
lAE70:
    t = gsu_count_down(&R[12]);
    gsu_code(0x01ae71, 1);
    gsu_code(t ? 0x01ae70 : 0x01ae72, 1);   /* the delay slot runs after the next fetch */
    gsu_plot_r1();
    if (t) goto lAE70;
    R[12] = R[4];
    R[12] = gsu_dec(R[12]);
    R[12] = gsu_dec(R[12]);
    R[13] = 0xAE78;
    gsu_code(0x01ae73, 6);
lAE78:
    R[2] = gsu_inc(R[2]);
    R[1] = gsu_sub(R[1], R[3], 0);
    gsu_code(0x01ae79, 4);
    gsu_plot_r1();
    R[1] = gsu_add(R[1], R[3], 0);
    R[1] = gsu_dec(R[1]);
    R[1] = gsu_dec(R[1]);
    t = gsu_count_down(&R[12]);
    gsu_code(0x01ae7d, 5);
    gsu_code(t ? 0x01ae78 : 0x01ae82, 1);   /* the delay slot runs after the next fetch */
    gsu_plot_r1();
    if (t) goto lAE78;
    R[1] = gsu_sub(R[1], R[3], 0);
    R[2] = gsu_inc(R[2]);
    R[12] = R[3];
    R[13] = 0xAE89;
    gsu_code(0x01ae83, 7);
lAE89:
    t = gsu_count_down(&R[12]);
    gsu_code(0x01ae8a, 1);
    gsu_code(t ? 0x01ae89 : 0x01ae8b, 1);   /* the delay slot runs after the next fetch */
    gsu_plot_r1();
    if (t) goto lAE89;
    nx = (uint16_t)(R[11]);
    at = nx;
    gsu_code(0x01ae8c, 1);
    goto dispatch;
    /* FxPixelRun */
lAE8D:
    /* CACHE */
    R[12] = R[3];
    gsu_moves(R[3]);
    t = gsu.z;
    gsu_color((uint8_t)R[0]);
    gsu_code(0x01ae8e, 5);
    if (t) { gsu_code(0x01ae97, 1); goto lAE97; }
    R[13] = 0xAE95;
    gsu_code(0x01ae93, 3);
lAE95:
    t = gsu_count_down(&R[12]);
    gsu_code(0x01ae96, 1);
    gsu_code(t ? 0x01ae95 : 0x01ae97, 1);   /* the delay slot runs after the next fetch */
    gsu_plot_r1();
    if (t) goto lAE95;
lAE97:
    nx = (uint16_t)(R[11]);
    at = nx;
    gsu_code(0x01ae98, 1);
    goto dispatch;
    /* FxClearScreenLong */
lAE99:
    R[0] = gsu_sub(R[0], R[0], 0);
    R[1] = 0x2C00;
    R[12] = 0x2000;
    /* CACHE */
    R[13] = 0xAEA3;
    gsu_code(0x01ae9a, 7); gsu_cache(0xaea1); gsu_code(0x01aea1, 3);
lAEA3:
    gsu_code(0x01aea4, 1);
    gsu_stw(R[1], R[0]);
    R[1] = gsu_inc(R[1]);
    t = gsu_count_down(&R[12]);
    R[1] = gsu_inc(R[1]);
    gsu_code(0x01aea5, 2);
    if (t) { gsu_code(0x01aea3, 1); goto lAEA3; }
    gsu_code(0x01aea7, 1);
    return;   /* the STOP: the program ends here */
    /* FxClearScreenShort */
lAEA9:
    R[0] = gsu_sub(R[0], R[0], 0);
    R[1] = 0x2C00;
    R[12] = 0x1800;
    /* CACHE */
    R[13] = 0xAEB3;
    gsu_code(0x01aeaa, 7); gsu_cache(0xaeb1); gsu_code(0x01aeb1, 3);
lAEB3:
    gsu_code(0x01aeb4, 1);
    gsu_stw(R[1], R[0]);
    R[1] = gsu_inc(R[1]);
    t = gsu_count_down(&R[12]);
    R[1] = gsu_inc(R[1]);
    gsu_code(0x01aeb5, 2);
    if (t) { gsu_code(0x01aeb3, 1); goto lAEB3; }
    gsu_code(0x01aeb7, 1);
    return;   /* the STOP: the program ends here */
    /* FxClearScreenIfSet */
lAECA:
    gsu_code(0x01aecb, 3);
    R[0] = gsu_ldw(GSU_WORD_1C4);
    R[0] = gsu_lob(R[0]);
    t = gsu.z;
    gsu_code(0x01aece, 3);
    if (t) { gsu_code(0x01aedf, 1); goto lAEDF; }
    R[0] = gsu_sub(R[0], R[0], 0);
    R[1] = 0x2C00;
    R[12] = 0x1A00;
    /* CACHE */
    R[13] = 0xAEDB;
    gsu_code(0x01aed1, 8); gsu_cache(0xaed9); gsu_code(0x01aed9, 3);
lAEDB:
    gsu_code(0x01aedc, 1);
    gsu_stw(R[1], R[0]);
    R[1] = gsu_inc(R[1]);
    t = gsu_count_down(&R[12]);
    R[1] = gsu_inc(R[1]);
    gsu_code(0x01aedd, 2);
    if (t) { gsu_code(0x01aedb, 1); goto lAEDB; }
    gsu_code(0x01aedf, 1);
lAEDF:
    return;   /* the STOP: the program ends here */
    /* FxClear6000IfSet */
lAEE1:
    gsu_code(0x01aee2, 3);
    R[0] = gsu_ldw(GSU_WORD_1C4);
    R[0] = gsu_lob(R[0]);
    t = gsu.z;
    gsu_code(0x01aee5, 3);
    if (t) { gsu_code(0x01aef6, 1); goto lAEF6; }
    R[0] = gsu_sub(R[0], R[0], 0);
    R[1] = 0x6000;
    R[12] = 0x1A00;
    /* CACHE */
    R[13] = 0xAEF2;
    gsu_code(0x01aee8, 8); gsu_cache(0xaef0); gsu_code(0x01aef0, 3);
lAEF2:
    gsu_code(0x01aef3, 1);
    gsu_stw(R[1], R[0]);
    R[1] = gsu_inc(R[1]);
    t = gsu_count_down(&R[12]);
    R[1] = gsu_inc(R[1]);
    gsu_code(0x01aef4, 2);
    if (t) { gsu_code(0x01aef2, 1); goto lAEF2; }
    gsu_code(0x01aef6, 1);
lAEF6:
    return;   /* the STOP: the program ends here */
    /* FxSprite32Program */
lB027:
    R[0] = 0x0003;
    gsu_code(0x01b028, 4);
    gsu_romb(R[0]);
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_cmode(R[0]);
    R[1] = 0x0010;
    gsu_code(0x01b02c, 8);
    R[0] = gsu_ldw(FX_SPRITE_COLOUR);
    gsu_code(0x01b034, 1);
    R[0] = gsu_mult(R[0], R[1]);
    gsu_color((uint8_t)R[0]);
    R[2] = 0x0020;
    gsu_code(0x01b035, 7);
    R[1] = gsu_ldw(FX_SPRITE_SIZE);
    R[0] = gsu_lob(R[1]);
    R[0] = gsu_and(R[0], R[2]);
    t = gsu.z;
    gsu_code(0x01b03c, 5);
    if (t) { gsu_code(0x01b043, 1); goto lB043; }
    R[0] = 0x0004;
    gsu_code(0x01b041, 3);
lB043:
    R[0] = gsu_or(R[0], 8);
    gsu_cmode(R[0]);
    gsu_code(0x01b044, 7);
    R[0] = gsu_umult(R[1], 3);
    R[1] = 0x9D59;
    gsu_code(0x01b04b, 5);
    gsu_r14(gsu_add(R[0], R[1], 0));
    gsu_code(0x01b050, 1);
    R[0] = gsu_getb();
    gsu_code(0x01b051, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x01b052, 2);
    R[0] = gsu_getbh(R[0]);
    gsu_code(0x01b054, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x01b055, 2);
    R[1] = gsu_getb();
    gsu_code(0x01b057, 3);
    gsu_romb(R[1]);
    gsu_code(0x01b05a, 2);
    gsu_r14(R[0]);
    R[0] = 0xFFF0;
    gsu_code(0x01b05c, 5);
    R[1] = gsu_ldw(FX_SPRITE_X);
    R[1] = gsu_add(R[0], R[1], 0);
    gsu_code(0x01b061, 5);
    R[2] = gsu_ldw(FX_SPRITE_Y);
    R[2] = gsu_add(R[0], R[2], 0);
    R[9] = R[1];
    R[11] = 0x0020;
    R[10] = 0x00E0;
    R[7] = 0xB079;
    R[8] = 0x0020;
    /* CACHE */
    gsu_code(0x01b066, 15); gsu_cache(0xb075); gsu_code(0x01b075, 1);
lB075:
    R[12] = R[8];
    R[13] = 0xB079;
    gsu_code(0x01b076, 4);
lB079:
    gsu_code(0x01b07a, 1);
    gsu_color(gsu_getb());   /* GETC */
    gsu_code(0x01b07b, 1);
    gsu_r14(gsu_inc(R[14]));
    t = gsu_count_down(&R[12]);
    gsu_code(0x01b07c, 1);
    gsu_code(t ? 0x01b079 : 0x01b07d, 1);   /* the delay slot runs after the next fetch */
    gsu_plot_r1();
    if (t) goto lB079;
    gsu_code(0x01b07e, 3);
    gsu_r14(gsu_add(R[14], R[10], 0));
    R[1] = R[9];
    R[11] = gsu_dec(R[11]);
    t = !gsu.z;
    R[2] = gsu_inc(R[2]);
    gsu_code(0x01b081, 5);
    if (t) { gsu_code(0x01b075, 1); goto lB075; }
    gsu_code(0x01b086, 3);
    R[0] = gsu_rpix_r(R[1], R[2]);
    return;   /* the STOP: the program ends here */
    /* FxObjectSpriteProgram */
lB08A:
    R[10] = 0x0240;
    R[0] = 0x0003;
    gsu_code(0x01b08b, 7);
    gsu_romb(R[0]);
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_cmode(R[0]);
    gsu_code(0x01b092, 6);
    R[0] = gsu_ldw(FX_SPRITE_X);
    gsu_code(0x01b098, 3);
    gsu_stw(FX_MODEL_BA, R[0]);
    gsu_code(0x01b09b, 3);
    R[0] = gsu_ldw(FX_SPRITE_Y);
    gsu_code(0x01b09e, 3);
    gsu_stw(FX_MODEL_BC, R[0]);
    R[1] = 0x0010;
    gsu_code(0x01b0a1, 5);
    R[0] = gsu_ldw(FX_SPRITE_COLOUR);
    gsu_code(0x01b0a6, 1);
    R[0] = gsu_mult(R[0], R[1]);
    gsu_color((uint8_t)R[0]);
    R[2] = 0x0020;
    gsu_code(0x01b0a7, 7);
    R[1] = gsu_ldw(FX_SPRITE_SIZE);
    R[0] = gsu_lob(R[1]);
    R[0] = gsu_and(R[0], R[2]);
    t = gsu.z;
    gsu_code(0x01b0ae, 5);
    if (t) { gsu_code(0x01b0b5, 1); goto lB0B5; }
    R[0] = 0x0004;
    gsu_code(0x01b0b3, 3);
lB0B5:
    R[0] = gsu_or(R[0], 8);
    gsu_cmode(R[0]);
    gsu_code(0x01b0b6, 7);
    R[0] = gsu_umult(R[1], 3);
    R[1] = 0x9D59;
    gsu_code(0x01b0bd, 5);
    gsu_r14(gsu_add(R[0], R[1], 0));
    gsu_code(0x01b0c2, 1);
    R[0] = gsu_getb();
    gsu_code(0x01b0c3, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x01b0c4, 2);
    R[0] = gsu_getbh(R[0]);
    gsu_code(0x01b0c6, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x01b0c7, 3);
    gsu_stw(FX_TEX_BASE, R[0]);
    gsu_code(0x01b0ca, 1);
    R[0] = gsu_getb();
    gsu_code(0x01b0cb, 3);
    gsu_stw(FX_TEX_BANK, R[0]);
    gsu_code(0x01b0ce, 3);
    R[1] = gsu_ldw(FX_MODEL_CE);
    R[11] = 0xB0D5;
    gsu_code(0x01b0d1, 4); gsu_code(0x01b17d, 1);
    abi_FxObjectSpriteAt();
    gsu_code(0x01b0d5, 1);
lB0D5:
    gsu_code(0x01b0d6, 2);
    R[0] = gsu_rpix_r(R[1], R[2]);
    return;   /* the STOP: the program ends here */
dispatch:
    if (at == ret)
        return;
    gsu_code(0x010000 | at, 1);   /* where it jumps: its first byte */
fetched:
    if (at == 0xAE5F || at == 0xAE8D)
        gsu_cache((uint16_t)(at + 1));   /* the code there starts with a CACHE */
enter:
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
    goto enter;   /* the caller fetched the first byte */
l8450:
    R[1] = 0x973E;
    gsu_code(0x018451, 6);
    gsu_r14(gsu_add(R[1], R[1], 0));
    gsu_code(0x018457, 2);
    R[4] = gsu_getb();
    gsu_code(0x018459, 3);
    gsu_r14(gsu_add(R[1], R[2], 0));
    gsu_code(0x01845c, 2);
    R[6] = gsu_getb();
    gsu_code(0x01845e, 3);
    gsu_r14(gsu_add(R[1], R[3], 0));
    gsu_code(0x018461, 2);
    R[8] = gsu_getb();
    R[1] = 0x96FE;
    gsu_code(0x018463, 6);
    gsu_r14(gsu_add(R[1], R[1], 0));
    gsu_code(0x018469, 2);
    R[5] = gsu_getb();
    gsu_code(0x01846b, 3);
    gsu_r14(gsu_add(R[1], R[2], 0));
    gsu_code(0x01846e, 2);
    R[7] = gsu_getb();
    gsu_code(0x018470, 3);
    gsu_r14(gsu_add(R[1], R[3], 0));
    gsu_code(0x018473, 2);
    R[3] = gsu_getb();
    gsu_code(0x018475, 2);
    R[0] = gsu_mult(R[8], R[7]);
    R[1] = gsu_add(R[0], R[0], 0);
    gsu_code(0x018477, 4);
    R[0] = gsu_mult(R[8], R[6]);
    R[2] = gsu_add(R[0], R[0], 0);
    gsu_code(0x01847b, 4);
    R[0] = gsu_mult(R[3], R[7]);
    R[12] = gsu_add(R[0], R[0], 0);
    gsu_code(0x01847f, 4);
    R[0] = gsu_mult(R[3], R[6]);
    R[13] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_hib(R[12]);
    gsu_code(0x018483, 5);
    R[0] = gsu_mult(R[0], R[5]);
    R[0] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_add(R[0], R[2], 0);
    R[0] = gsu_hib(R[0]);
    gsu_code(0x018488, 5);
    gsu_stb(R[9], (uint8_t)R[0]);
    R[9] = gsu_inc(R[9]);
    R[0] = gsu_hib(R[1]);
    gsu_code(0x01848d, 4);
    R[0] = gsu_mult(R[0], R[5]);
    R[0] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_sub(R[0], R[13], 0);
    R[0] = gsu_hib(R[0]);
    gsu_code(0x018491, 5);
    gsu_stb(R[9], (uint8_t)R[0]);
    R[9] = gsu_inc(R[9]);
    gsu_code(0x018496, 3);
    R[0] = gsu_mult(R[7], R[4]);
    R[0] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_hib(R[0]);
    gsu_code(0x018499, 4);
    gsu_stb(R[9], (uint8_t)R[0]);
    R[9] = gsu_inc(R[9]);
    gsu_code(0x01849d, 3);
    R[0] = gsu_mult(R[4], R[3]);
    R[0] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_hib(R[0]);
    gsu_code(0x0184a0, 4);
    gsu_stb(R[9], (uint8_t)R[0]);
    R[9] = gsu_inc(R[9]);
    gsu_code(0x0184a4, 3);
    R[0] = gsu_mult(R[4], R[8]);
    R[0] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_hib(R[0]);
    gsu_code(0x0184a7, 4);
    gsu_stb(R[9], (uint8_t)R[0]);
    R[9] = gsu_inc(R[9]);
    R[0] = gsu_not(R[5]);
    R[0] = gsu_inc(R[0]);
    gsu_code(0x0184ab, 6);
    gsu_stb(R[9], (uint8_t)R[0]);
    R[9] = gsu_inc(R[9]);
    R[0] = gsu_hib(R[13]);
    gsu_code(0x0184b1, 4);
    R[0] = gsu_mult(R[0], R[5]);
    R[0] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_sub(R[0], R[1], 0);
    R[0] = gsu_hib(R[0]);
    gsu_code(0x0184b5, 5);
    gsu_stb(R[9], (uint8_t)R[0]);
    R[9] = gsu_inc(R[9]);
    R[0] = gsu_hib(R[2]);
    gsu_code(0x0184ba, 4);
    R[0] = gsu_mult(R[0], R[5]);
    R[0] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_add(R[0], R[12], 0);
    R[0] = gsu_hib(R[0]);
    gsu_code(0x0184be, 5);
    gsu_stb(R[9], (uint8_t)R[0]);
    R[9] = gsu_inc(R[9]);
    gsu_code(0x0184c3, 3);
    R[0] = gsu_mult(R[6], R[4]);
    R[0] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_hib(R[0]);
    gsu_code(0x0184c6, 4);
    gsu_stb(R[9], (uint8_t)R[0]);
    nx = (uint16_t)(R[11]);
    at = nx;
    gsu_code(0x0184ca, 1);
    goto dispatch;
l84CB:
    R[0] = 0xFFFF;
    gsu_color((uint8_t)R[0]);
    R[1] = 0x0000;
    R[2] = 0x0040;
    R[12] = 0x0020;
    R[13] = 0x84D6;
    gsu_code(0x0184cc, 11);
l84D6:
    t = gsu_count_down(&R[12]);
    gsu_code(0x0184d7, 1);
    gsu_code(t ? 0x0184d6 : 0x0184d8, 1);   /* the delay slot runs after the next fetch */
    gsu_plot_r1();
    if (t) goto l84D6;
    return;   /* the STOP: the program ends here */
l84DA:
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_code(0x0184db, 3);
    gsu_ramb(R[0]);
    gsu_code(0x0184de, 2);
    gsu_romb(R[0]);
    R[10] = 0x0240;
    gsu_code(0x0184e0, 6);
    R[0] = gsu_ldw(0x0028);
    gsu_code(0x0184e6, 1);
    gsu_stw(R[10], R[0]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[1] = 0x0080;
    R[0] = gsu_add(R[0], R[1], 0);
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_code(0x0184e7, 8);
    gsu_sbk(R[0]);
    R[11] = 0x84F3;
    gsu_code(0x0184ef, 4); gsu_code(0x018576, 1);
    abi_FxModelDrawFlagged();
    gsu_code(0x0184f3, 1);
l84F3:
    gsu_code(0x0184f4, 3);
    R[0] = gsu_ldw(0x0020);
    R[0] = gsu_swap(R[0]);
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_code(0x0184f7, 4);
    gsu_sbk(R[0]);
    gsu_code(0x0184fb, 3);
    R[0] = gsu_ldw(0x0022);
    R[0] = gsu_swap(R[0]);
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_code(0x0184fe, 4);
    gsu_sbk(R[0]);
    gsu_code(0x018502, 3);
    R[0] = gsu_ldw(0x0024);
    R[0] = gsu_swap(R[0]);
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_code(0x018505, 4);
    gsu_sbk(R[0]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x018509, 3);
    R[0] = gsu_ldw(R[10]);
    gsu_code(0x01850c, 3);
    gsu_stw(0x0028, R[0]);
    R[11] = 0x8513;
    gsu_code(0x01850f, 4); gsu_code(0x018584, 1);
    abi_FxModelDrawFlags8000();
    gsu_code(0x018513, 1);
l8513:
    return;   /* the STOP: the program ends here */
l8515:
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_code(0x018516, 3);
    gsu_ramb(R[0]);
    gsu_code(0x018519, 2);
    gsu_romb(R[0]);
    R[10] = 0x0240;
    gsu_code(0x01851b, 7);
    R[4] = gsu_ldw(0x001A);
    gsu_code(0x018522, 2);
    gsu_r14(0x0012);
    gsu_code(0x018524, 3);
    gsu_r14(gsu_add(R[4], R[14], 0));
    gsu_code(0x018527, 1);
    R[0] = gsu_getb();
    gsu_code(0x018528, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x018529, 2);
    R[0] = gsu_getbh(R[0]);
    gsu_code(0x01852b, 3);
    gsu_stw(0x004A, R[0]);
    gsu_code(0x01852e, 2);
    gsu_r14(0x0007);
    gsu_code(0x018530, 3);
    gsu_r14(gsu_add(R[4], R[14], 0));
    gsu_code(0x018533, 1);
    R[0] = gsu_getb();
    gsu_code(0x018534, 3);
    gsu_stw(0x0032, R[0]);
    gsu_code(0x018537, 2);
    gsu_r14(0x0000);
    gsu_code(0x018539, 3);
    gsu_r14(gsu_add(R[4], R[14], 0));
    gsu_code(0x01853c, 1);
    R[0] = gsu_getb();
    gsu_code(0x01853d, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x01853e, 2);
    R[0] = gsu_getbh(R[0]);
    gsu_code(0x018540, 3);
    gsu_stw(0x0016, R[0]);
    gsu_code(0x018543, 2);
    gsu_r14(0x0002);
    gsu_code(0x018545, 3);
    gsu_r14(gsu_add(R[4], R[14], 0));
    gsu_code(0x018548, 1);
    R[0] = gsu_getb();
    gsu_code(0x018549, 3);
    gsu_stw(0x001C, R[0]);
    gsu_code(0x01854c, 2);
    gsu_r14(0x0003);
    gsu_code(0x01854e, 3);
    gsu_r14(gsu_add(R[4], R[14], 0));
    gsu_code(0x018551, 1);
    R[0] = gsu_getb();
    gsu_code(0x018552, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x018553, 2);
    R[0] = gsu_getbh(R[0]);
    gsu_code(0x018555, 3);
    gsu_stw(0x0018, R[0]);
    R[0] = 0x8000;
    R[11] = 0x855F;
    gsu_code(0x018558, 7); gsu_code(0x01858f, 1);
    abi_FxModelDrawWithFlags();
    gsu_code(0x01855f, 1);
l855F:
    return;   /* the STOP: the program ends here */
dispatch:
    if (at == ret)
        return;
    gsu_code(0x010000 | at, 1);   /* where it jumps: its first byte */
fetched:
enter:
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
    goto enter;   /* the caller fetched the first byte */
lB42F:
    gsu_code(0x01b430, 2);
    gsu_stw(R[10], R[11]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_color((uint8_t)R[0]);
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_code(0x01b432, 6);
    gsu_ramb(R[0]);
    gsu_cmode(R[0]);
    R[0] = 0x0080;
    gsu_code(0x01b438, 8);
    R[1] = gsu_ldw(0x0026);
    gsu_code(0x01b440, 3);
    R[2] = gsu_ldw(0x0028);
    gsu_code(0x01b443, 3);
    R[3] = gsu_ldw(0x002A);
    gsu_sub(R[0], R[3], 0);   /* CMP */
    t = gsu.s;
    gsu_code(0x01b446, 4);
    if (t) { gsu_code(0x01b44f, 1); goto lB44F; }
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x01b44a, 5);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
lB44F:
    R[11] = 0xB454;
    gsu_code(0x01b450, 4); gsu_code(0x01947a, 1);
    abi_FxProjectPoint();
    gsu_code(0x01b454, 1);
lB454:
    gsu_code(0x01b455, 3);
    gsu_stw(0x0026, R[1]);
    gsu_code(0x01b458, 3);
    gsu_stw(0x0028, R[2]);
    gsu_code(0x01b45b, 3);
    R[0] = gsu_ldw(0x001C);
    gsu_code(0x01b45e, 2); gsu_cache(0xb460);
    gsu_romb(R[0]);
    /* CACHE */
    gsu_code(0x01b460, 4);
    gsu_r14(gsu_ldw(0x001A));
    R[12] = 0xFFFF;
    gsu_code(0x01b464, 2);
lB465:
    gsu_code(0x01b466, 1);
    R[0] = gsu_getb();
    gsu_code(0x01b467, 1);
    gsu_r14(gsu_inc(R[14]));
    R[0] = gsu_lob(R[0]);
    t = !gsu.z;
    R[12] = gsu_inc(R[12]);
    gsu_code(0x01b468, 3);
    if (t) { gsu_code(0x01b465, 1); goto lB465; }
    R[3] = 0x0010;
    gsu_code(0x01b46b, 5);
    R[0] = gsu_mult(R[12], R[3]);
    gsu_code(0x01b470, 1);
    gsu_stw(R[10], R[0]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[2] = 0x007F;
    gsu_code(0x01b471, 7);
    R[1] = gsu_ldw(0x00C8);
    R[1] = gsu_add(R[1], R[2], 0);
    gsu_code(0x01b478, 6);
    R[2] = gsu_umult(R[1], R[12]);
    gsu_code(0x01b47e, 3);
    R[3] = gsu_ldw(0x002A);
    R[11] = 0xB485;
    gsu_code(0x01b481, 4); gsu_code(0x01947a, 1);
    abi_FxProjectPoint();
    gsu_code(0x01b485, 1);
lB485:
    R[0] = 0x0065;
    R[1] = gsu_sub(R[1], R[0], 0);
    R[0] = 0x0033;
    R[2] = gsu_sub(R[2], R[0], 0);
    R[1] = gsu_lob(R[1]);
    gsu_code(0x01b486, 15);
    gsu_stw(0x00C8, R[1]);
    R[3] = 0xFFFF;
    R[0] = gsu_add(R[0], 0, 0);
    R[0] = gsu_ror(R[2]);
    R[0] = gsu_sub(R[0], R[3], !gsu.cy);
    gsu_code(0x01b495, 11);
    R[3] = gsu_ldw(0x0026);
    R[0] = gsu_sub(R[3], R[0], 0);
    gsu_code(0x01b4a0, 3);
    gsu_sbk(R[0]);
    R[3] = 0xFFFF;
    R[0] = gsu_add(R[0], 0, 0);
    R[0] = gsu_ror(R[1]);
    R[0] = gsu_sub(R[0], R[3], !gsu.cy);
    gsu_code(0x01b4a3, 11);
    R[3] = gsu_ldw(0x0028);
    R[0] = gsu_sub(R[3], R[0], 0);
    gsu_code(0x01b4ae, 3);
    gsu_sbk(R[0]);
    R[3] = gsu_not(R[1]);
    R[3] = gsu_inc(R[3]);
    gsu_code(0x01b4b1, 7);
    gsu_stw(0x00B2, R[3]);
    R[3] = 0x00E0;
    R[3] = gsu_sub(R[3], R[1], 0);
    gsu_code(0x01b4b8, 8);
    gsu_stw(0x00B4, R[3]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x01b4c0, 4);
    R[6] = gsu_ldw(R[10]);
    R[0] = gsu_sub(R[2], R[6], 0);
    t = !gsu.cy;
    gsu_code(0x01b4c4, 4);
    if (t) { gsu_code(0x01b4cc, 1); goto lB4CC; }
    gsu_code(0x01b4c8, 4); gsu_code(0x01b57d, 1);
    goto lB57D;
lB4CC:
    R[0] = R[2];
    R[4] = 0x0000;
    R[11] = 0xB4D5;
    gsu_code(0x01b4cd, 8); gsu_code(0x0181bc, 1);
    abi_FxDivide32By16Halving();
    gsu_code(0x01b4d5, 1);
lB4D5:
    R[9] = R[4];
    R[8] = R[4];
    R[3] = 0x0000;
    gsu_code(0x01b4d6, 9); gsu_cache(0xb4df);
    gsu_stw(0x00B2, R[3]);
    /* CACHE */
    gsu_code(0x01b4df, 4);
    R[2] = gsu_ldw(0x0028);
    R[6] = 0x0000;
    R[4] = 0x0000;
    gsu_code(0x01b4e3, 4);
lB4E6:
    R[2] = R[2];
    gsu_moves(R[2]);
    t = !gsu.s;
    gsu_code(0x01b4e7, 4);
    if (t) { gsu_code(0x01b4ef, 1); goto lB4EF; }
    gsu_code(0x01b4eb, 4); gsu_code(0x01b55a, 1);
    goto lB55A;
lB4EF:
    R[0] = 0x00BF;
    gsu_sub(R[0], R[2], 0);   /* CMP */
    t = !gsu.s;
    gsu_code(0x01b4f0, 7);
    if (t) { gsu_code(0x01b4fb, 1); goto lB4FB; }
    gsu_code(0x01b4f7, 4); gsu_code(0x01b55a, 1);
    goto lB55A;
lB4FB:
    gsu_code(0x01b4fc, 3);
    R[1] = gsu_ldw(0x0026);
    R[7] = 0x0000;
    gsu_code(0x01b4ff, 5);
    R[5] = gsu_ldw(0x001A);
lB503:
    gsu_code(0x01b504, 3);
    R[0] = gsu_ldw(0x001C);
    gsu_code(0x01b507, 2);
    gsu_romb(R[0]);
    gsu_code(0x01b509, 2);
    gsu_r14(R[5]);
    gsu_code(0x01b50b, 1);
    R[0] = gsu_getb();
    R[5] = gsu_inc(R[5]);
    R[0] = gsu_add(R[0], R[0], 0);
    t = gsu.z;
    gsu_code(0x01b50c, 4);
    if (t) { gsu_code(0x01b55a, 1); goto lB55A; }
    R[3] = 0x0014;
    gsu_code(0x01b510, 6);
    gsu_romb(R[3]);
    R[3] = 0x0010;
    gsu_code(0x01b516, 4);
    R[0] = gsu_umult(R[0], R[3]);
    R[3] = 0xD2BA;
    R[0] = gsu_add(R[0], R[4], 0);
    R[0] = gsu_add(R[0], R[4], 0);
    gsu_code(0x01b51a, 7);
    gsu_r14(gsu_add(R[0], R[3], 0));
    gsu_code(0x01b521, 3);
    R[0] = gsu_ldw(0x00B2);
    gsu_sub(R[0], R[1], 0);   /* CMP */
    t = gsu.s;
    gsu_code(0x01b524, 4);
    if (t) { gsu_code(0x01b530, 1); goto lB530; }
    gsu_code(0x01b528, 4);
    R[0] = gsu_ldw(0x00C8);
    R[1] = gsu_add(R[0], R[1], 0);
    t = 1;
    gsu_code(0x01b52c, 4);
    gsu_code(0x01b503, 1);
    if (t) goto lB503;
lB530:
    gsu_code(0x01b531, 3);
    R[3] = gsu_ldw(0x00B4);
    R[3] = gsu_dec(R[3]);
    R[3] = gsu_dec(R[3]);
    gsu_sub(R[3], R[1], 0);   /* CMP */
    t = gsu.s;
    gsu_code(0x01b534, 7);
    if (t) { gsu_code(0x01b55a, 1); goto lB55A; }
    gsu_code(0x01b53b, 2);
    R[0] = gsu_getb();
    gsu_code(0x01b53d, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x01b53e, 2);
    R[0] = gsu_getbh(R[0]);
    R[12] = 0x0010;
    R[13] = 0xB543;
    gsu_code(0x01b540, 4);
lB543:
    R[7] = gsu_sub(R[7], R[9], 0);
    t = !gsu.cy;
    R[0] = gsu_rol(R[0]);
    gsu_code(0x01b544, 4);
    if (t) { gsu_code(0x01b54d, 1); goto lB54D; }
    t = gsu_count_down(&R[12]);
    gsu_code(0x01b548, 2);
    if (t) { gsu_code(0x01b543, 1); goto lB543; }
    t = 1;
    gsu_code(0x01b54a, 3);
    gsu_code(0x01b503, 1);
    if (t) goto lB503;
lB54D:
    t = gsu.cy;
    gsu_code(0x01b54e, 2);
    if (t) { gsu_code(0x01b555, 1); goto lB555; }
    t = gsu_count_down(&R[12]);
    R[1] = gsu_inc(R[1]);
    gsu_code(0x01b550, 2);
    if (t) { gsu_code(0x01b543, 1); goto lB543; }
    t = 1;
    gsu_code(0x01b552, 3);
    gsu_code(0x01b503, 1);
    if (t) goto lB503;
lB555:
    t = gsu_count_down(&R[12]);
    gsu_code(0x01b556, 1);
    gsu_code(t ? 0x01b543 : 0x01b557, 1);   /* the delay slot runs after the next fetch */
    gsu_plot_r1();
    if (t) goto lB543;
    t = 1;
    gsu_code(0x01b558, 2);
    gsu_code(0x01b503, 1);
    if (t) goto lB503;
lB55A:
    R[6] = gsu_sub(R[6], R[8], 0);
    t = !gsu.cy;
    gsu_code(0x01b55b, 4);
    if (t) { gsu_code(0x01b56a, 1); goto lB56A; }
    R[4] = gsu_inc(R[4]);
    R[3] = 0x0010;
    R[0] = gsu_sub(R[4], R[3], 0);
    t = !gsu.z;
    gsu_code(0x01b55f, 8);
    if (t) { gsu_code(0x01b55a, 1); goto lB55A; }
    t = 1;
    gsu_code(0x01b567, 3);
    gsu_code(0x01b578, 1);
    if (t) goto lB578;
lB56A:
    R[2] = gsu_inc(R[2]);
    R[4] = gsu_inc(R[4]);
    R[3] = 0x0010;
    R[0] = gsu_sub(R[4], R[3], 0);
    t = gsu.z;
    gsu_code(0x01b56b, 8);
    if (t) { gsu_code(0x01b577, 1); goto lB577; }
    gsu_code(0x01b573, 4); gsu_code(0x01b4e6, 1);
    goto lB4E6;
lB577:
    gsu_code(0x01b578, 1);
lB578:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x01b579, 4);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
lB57D:
    R[0] = R[6];
    R[6] = R[2];
    gsu_sub(R[0], R[6], 0);   /* CMP */
    t = !gsu.z;
    gsu_code(0x01b57e, 8);
    if (t) { gsu_code(0x01b58c, 1); goto lB58C; }
    R[4] = 0xFFFF;
    t = 1;
    gsu_code(0x01b586, 6);
    gsu_code(0x01b593, 1);
    if (t) goto lB593;
lB58C:
    R[4] = 0x0000;
    R[11] = 0xB593;
    gsu_code(0x01b58d, 6); gsu_code(0x0181bc, 1);
    abi_FxDivide32By16Halving();
    gsu_code(0x01b593, 1);
lB593:
    R[9] = R[4];
    R[8] = R[4];
    /* CACHE */
    gsu_code(0x01b594, 4); gsu_cache(0xb598); gsu_code(0x01b598, 4);
    R[2] = gsu_ldw(0x0028);
    R[6] = 0x0000;
    R[4] = 0x0000;
    gsu_code(0x01b59c, 4);
lB59F:
    R[2] = R[2];
    gsu_moves(R[2]);
    t = !gsu.s;
    gsu_code(0x01b5a0, 4);
    if (t) { gsu_code(0x01b5a8, 1); goto lB5A8; }
    gsu_code(0x01b5a4, 4); gsu_code(0x01b67e, 1);
    goto lB67E;
lB5A8:
    R[0] = 0x00BF;
    gsu_sub(R[0], R[2], 0);   /* CMP */
    t = !gsu.s;
    gsu_code(0x01b5a9, 7);
    if (t) { gsu_code(0x01b5b4, 1); goto lB5B4; }
    gsu_code(0x01b5b0, 4); gsu_code(0x01b67e, 1);
    goto lB67E;
lB5B4:
    gsu_code(0x01b5b5, 3);
    R[1] = gsu_ldw(0x0026);
    R[7] = 0x0000;
    gsu_code(0x01b5b8, 5);
    R[5] = gsu_ldw(0x001A);
lB5BC:
    gsu_code(0x01b5bd, 3);
    R[0] = gsu_ldw(0x001C);
    gsu_code(0x01b5c0, 2);
    gsu_romb(R[0]);
    gsu_code(0x01b5c2, 2);
    gsu_r14(R[5]);
    gsu_code(0x01b5c4, 1);
    R[0] = gsu_getb();
    R[5] = gsu_inc(R[5]);
    R[0] = gsu_add(R[0], R[0], 0);
    t = !gsu.z;
    gsu_code(0x01b5c5, 4);
    if (t) { gsu_code(0x01b5cd, 1); goto lB5CD; }
    gsu_code(0x01b5c9, 4); gsu_code(0x01b67e, 1);
    goto lB67E;
lB5CD:
    R[3] = 0x0014;
    gsu_code(0x01b5ce, 5);
    gsu_romb(R[3]);
    R[3] = 0x0010;
    gsu_code(0x01b5d3, 4);
    R[0] = gsu_umult(R[0], R[3]);
    R[3] = 0xD2BA;
    R[0] = gsu_add(R[0], R[4], 0);
    R[0] = gsu_add(R[0], R[4], 0);
    gsu_code(0x01b5d7, 7);
    gsu_r14(gsu_add(R[0], R[3], 0));
    gsu_code(0x01b5de, 3);
    R[0] = gsu_ldw(0x00B2);
    gsu_sub(R[0], R[1], 0);   /* CMP */
    t = gsu.s;
    gsu_code(0x01b5e1, 4);
    if (t) { gsu_code(0x01b5e9, 1); goto lB5E9; }
    gsu_code(0x01b5e5, 4); gsu_code(0x01b66e, 1);
    goto lB66E;
lB5E9:
    R[1] = R[1];
    gsu_moves(R[1]);
    t = !gsu.s;
    gsu_code(0x01b5ea, 4);
    gsu_code(t ? 0x01b618 : 0x01b5ee, 1);   /* the delay slot runs after the next fetch */
    R[0] = gsu_getb();
    if (t) goto lB618;
    gsu_code(0x01b5ef, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x01b5f0, 2);
    R[0] = gsu_getbh(R[0]);
    R[12] = 0x0010;
    R[13] = 0xB5F5;
    gsu_code(0x01b5f2, 4);
lB5F5:
    R[0] = gsu_rol(R[0]);
    t = gsu.cy;
    gsu_code(0x01b5f6, 3);
    if (t) { gsu_code(0x01b603, 1); goto lB603; }
    gsu_code(0x01b5f9, 1);
lB5F9:
    R[7] = gsu_sub(R[7], R[9], 0);
    t = gsu.cy;
    R[1] = gsu_inc(R[1]);
    gsu_code(0x01b5fa, 4);
    if (t) { gsu_code(0x01b5f9, 1); goto lB5F9; }
    t = gsu_count_down(&R[12]);
    gsu_code(0x01b5fe, 2);
    if (t) { gsu_code(0x01b5f5, 1); goto lB5F5; }
    t = 1;
    gsu_code(0x01b600, 3);
    gsu_code(0x01b5bc, 1);
    if (t) goto lB5BC;
lB603:
    R[7] = gsu_sub(R[7], R[9], 0);
    t = gsu.cy;
    R[1] = gsu_inc(R[1]);
    gsu_code(0x01b604, 4);
    if (t) { gsu_code(0x01b603, 1); goto lB603; }
    t = gsu.s;
    gsu_code(0x01b608, 3);
    if (t) { gsu_code(0x01b613, 1); goto lB613; }
    R[13] = 0xB629;
    t = gsu_count_down(&R[12]);
    gsu_code(0x01b60b, 5);
    if (t) { gsu_code(0x01b629, 1); goto lB629; }
    t = 1;
    gsu_code(0x01b610, 3);
    gsu_code(0x01b5bc, 1);
    if (t) goto lB5BC;
lB613:
    t = gsu_count_down(&R[12]);
    gsu_code(0x01b614, 1);
    if (t) { gsu_code(0x01b5f5, 1); goto lB5F5; }              /* R13 is still $B5F5 on this path */
    t = 1;
    gsu_code(0x01b615, 3);
    gsu_code(0x01b5bc, 1);
    if (t) goto lB5BC;
lB618:
    gsu_code(0x01b619, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x01b61a, 3);
    R[3] = gsu_ldw(0x00B4);
    R[3] = gsu_dec(R[3]);
    gsu_sub(R[3], R[1], 0);   /* CMP */
    t = gsu.s;
    gsu_code(0x01b61d, 6);
    if (t) { gsu_code(0x01b642, 1); goto lB642; }
    gsu_code(0x01b623, 3);
    R[0] = gsu_getbh(R[0]);
    R[12] = 0x0010;
    R[13] = 0xB629;
    gsu_code(0x01b626, 4);
lB629:
    R[0] = gsu_rol(R[0]);
    t = gsu.cy;
    gsu_code(0x01b62a, 3);
    if (t) { gsu_code(0x01b637, 1); goto lB637; }
    gsu_code(0x01b62d, 1);
lB62D:
    R[7] = gsu_sub(R[7], R[9], 0);
    t = gsu.cy;
    R[1] = gsu_inc(R[1]);
    gsu_code(0x01b62e, 4);
    if (t) { gsu_code(0x01b62d, 1); goto lB62D; }
    t = gsu_count_down(&R[12]);
    gsu_code(0x01b632, 2);
    if (t) { gsu_code(0x01b629, 1); goto lB629; }
    t = 1;
    gsu_code(0x01b634, 3);
    gsu_code(0x01b5bc, 1);
    if (t) goto lB5BC;
lB637:
    R[7] = gsu_sub(R[7], R[9], 0);
    t = gsu.cy;
    gsu_code(0x01b638, 4);
    gsu_code(t ? 0x01b637 : 0x01b63c, 1);   /* the delay slot runs after the next fetch */
    gsu_plot_r1();
    if (t) goto lB637;
    t = gsu_count_down(&R[12]);
    gsu_code(0x01b63d, 1);
    if (t) { gsu_code(0x01b629, 1); goto lB629; }
    gsu_code(0x01b63e, 4); gsu_code(0x01b5bc, 1);
    goto lB5BC;
lB642:
    gsu_code(0x01b643, 2);
    R[0] = gsu_getbh(R[0]);
    R[3] = 0x00DF;
    R[12] = 0x0010;
    R[13] = 0xB64B;
    gsu_code(0x01b645, 7);
lB64B:
    R[0] = gsu_rol(R[0]);
    t = gsu.cy;
    gsu_code(0x01b64c, 3);
    if (t) { gsu_code(0x01b65f, 1); goto lB65F; }
    gsu_code(0x01b64f, 1);
lB64F:
    R[7] = gsu_sub(R[7], R[9], 0);
    t = gsu.cy;
    R[1] = gsu_inc(R[1]);
    gsu_code(0x01b650, 4);
    if (t) { gsu_code(0x01b64f, 1); goto lB64F; }
    gsu_sub(R[1], R[3], 0);   /* CMP */
    t = !gsu.s;
    gsu_code(0x01b654, 6);
    if (t) { gsu_code(0x01b67e, 1); goto lB67E; }
    t = gsu_count_down(&R[12]);
    gsu_code(0x01b65a, 2);
    if (t) { gsu_code(0x01b64b, 1); goto lB64B; }
    t = 1;
    gsu_code(0x01b65c, 3);
    gsu_code(0x01b67e, 1);
    if (t) goto lB67E;
lB65F:
    gsu_sub(R[1], R[3], 0);   /* CMP */
    t = !gsu.s;
    gsu_code(0x01b660, 5);
    if (t) { gsu_code(0x01b67e, 1); goto lB67E; }
    R[7] = gsu_sub(R[7], R[9], 0);
    t = gsu.cy;
    gsu_code(0x01b665, 4);
    gsu_code(t ? 0x01b65f : 0x01b669, 1);   /* the delay slot runs after the next fetch */
    gsu_plot_r1();
    if (t) goto lB65F;
    t = gsu_count_down(&R[12]);
    gsu_code(0x01b66a, 1);
    if (t) { gsu_code(0x01b64b, 1); goto lB64B; }
    t = 1;
    gsu_code(0x01b66b, 3);
    gsu_code(0x01b67e, 1);
    if (t) goto lB67E;
lB66E:
    gsu_code(0x01b66f, 3);
    R[3] = gsu_ldw(0x00C8);
    R[1] = gsu_add(R[1], R[3], 0);
    R[7] = gsu_sub(R[7], R[9], 0);
    t = !gsu.cy;
    gsu_code(0x01b672, 6);
    if (t) { gsu_code(0x01b679, 1); goto lB679; }
    R[1] = gsu_inc(R[1]);
    gsu_code(0x01b678, 2);
lB679:
    gsu_code(0x01b67a, 3); gsu_code(0x01b5bc, 1);
    goto lB5BC;
lB67D:
    gsu_code(0x01b67e, 1);
lB67E:
    R[2] = gsu_inc(R[2]);
    R[6] = gsu_sub(R[6], R[8], 0);
    t = !gsu.cy;
    gsu_code(0x01b67f, 6);
    if (t) { gsu_code(0x01b689, 1); goto lB689; }
    gsu_code(0x01b685, 4); gsu_code(0x01b59f, 1);
    goto lB59F;
lB689:
    R[4] = gsu_inc(R[4]);
    R[3] = 0x0010;
    R[0] = gsu_sub(R[4], R[3], 0);
    t = gsu.z;
    gsu_code(0x01b68a, 7);
    if (t) { gsu_code(0x01b695, 1); goto lB695; }
    gsu_code(0x01b691, 4); gsu_code(0x01b59f, 1);
    goto lB59F;
lB695:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x01b696, 4);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
lB69A:
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_code(0x01b69b, 3);
    gsu_ramb(R[0]);
    gsu_code(0x01b69e, 2); gsu_cache(0xb6a0);
    gsu_romb(R[0]);
    /* CACHE */
    gsu_code(0x01b6a0, 4);
    R[4] = gsu_ldw(0x0026);
    R[0] = 0x0100;
    gsu_code(0x01b6a4, 5);
    R[0] = gsu_umult(R[0], R[4]);
    R[0] = gsu_add(R[0], R[0], 0);
    R[4] = gsu_add(R[0], R[0], 0);
    gsu_code(0x01b6a9, 6);
    R[0] = gsu_ldw(0x0028);
    gsu_code(0x01b6af, 2);
    R[0] = gsu_umult(R[0], 4);
    R[0] = gsu_add(R[0], R[4], 0);
    R[4] = 0x2C00;
    R[4] = gsu_add(R[0], R[4], 0);
    R[5] = 0x0400;
    gsu_code(0x01b6b1, 12);
    gsu_r14(gsu_ldw(0x0018));
    R[3] = 0x0004;
    gsu_code(0x01b6bd, 2);
lB6BE:
    R[1] = R[4];
    R[12] = 0x0050;
    R[13] = 0xB6C4;
    gsu_code(0x01b6bf, 6);
lB6C4:
    gsu_code(0x01b6c5, 1);
    R[0] = gsu_getb();
    gsu_code(0x01b6c6, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x01b6c7, 2);
    R[0] = gsu_getbh(R[0]);
    gsu_code(0x01b6c9, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x01b6ca, 1);
    gsu_stw(R[1], R[0]);
    R[1] = gsu_inc(R[1]);
    t = gsu_count_down(&R[12]);
    R[1] = gsu_inc(R[1]);
    gsu_code(0x01b6cb, 2);
    if (t) { gsu_code(0x01b6c4, 1); goto lB6C4; }
    R[4] = gsu_add(R[4], R[5], 0);
    R[3] = gsu_dec(R[3]);
    t = !gsu.z;
    gsu_code(0x01b6cd, 6);
    if (t) { gsu_code(0x01b6be, 1); goto lB6BE; }
    gsu_code(0x01b6d3, 1);
    return;   /* the STOP: the program ends here */
dispatch:
    if (at == ret)
        return;
    gsu_code(0x010000 | at, 1);   /* where it jumps: its first byte */
fetched:
enter:
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
    goto enter;   /* the caller fetched the first byte */
lC0E0:
    gsu_code(0x01c0e1, 70); gsu_cache(0xc127);
lC126:
    /* CACHE */
    nx = (uint16_t)(R[11]);
    at = nx;
    gsu_code(0x01c127, 9);
    goto dispatch;
l96E7:
    gsu_code(0x0196e8, 2);
    gsu_stw(R[10], R[11]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_code(0x0196ea, 5);
    gsu_stw(0x0040, R[0]);
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_code(0x0196ef, 4);
    gsu_stw(0x014A, R[0]);
l96F2:
    gsu_code(0x0196f3, 1);
    R[0] = gsu_ldw(R[7]);
    R[7] = gsu_inc(R[7]);
    R[7] = gsu_inc(R[7]);
    gsu_code(0x0196f4, 5);
    R[9] = gsu_ldw(0x0026);
    R[1] = gsu_add(R[0], R[9], 0);
    gsu_code(0x0196f9, 3);
    R[0] = gsu_ldw(R[7]);
    R[7] = gsu_inc(R[7]);
    R[7] = gsu_inc(R[7]);
    gsu_code(0x0196fc, 5);
    R[9] = gsu_ldw(0x0028);
    R[2] = gsu_add(R[0], R[9], 0);
    gsu_code(0x019701, 3);
    R[0] = gsu_ldw(R[7]);
    R[7] = gsu_inc(R[7]);
    R[7] = gsu_inc(R[7]);
    gsu_code(0x019704, 5);
    R[9] = gsu_ldw(0x002A);
    R[3] = gsu_add(R[0], R[9], 0);
    R[11] = 0x970F;
    gsu_code(0x019709, 6); gsu_code(0x01947a, 1);
    abi_FxProjectPoint();
    gsu_code(0x01970f, 1);
l970F:
    gsu_code(0x019710, 2);
    gsu_stw(R[8], R[1]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    gsu_code(0x019712, 4);
    gsu_stw(R[8], R[2]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    gsu_code(0x019716, 3);
    gsu_stw(R[8], R[0]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    gsu_code(0x019719, 5);
    R[3] = gsu_ldw(0x014A);
    R[0] = gsu_or(R[0], R[3]);
    gsu_code(0x01971e, 2);
    gsu_sbk(R[0]);
    gsu_code(0x019720, 3);
    R[0] = gsu_ldw(0x0040);
    R[0] = gsu_dec(R[0]);
    t = !gsu.z;
    gsu_code(0x019723, 3);
    gsu_code(t ? 0x0196f2 : 0x019726, 1);   /* the delay slot runs after the next fetch */
    gsu_sbk(R[0]);
    if (t) goto l96F2;
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x019727, 4);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
l97D9:
    gsu_code(0x0197da, 2);
    gsu_stw(R[10], R[11]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[0] = 0x973E;
    gsu_code(0x0197dc, 7);
    gsu_r14(gsu_add(R[0], R[2], 0));
    gsu_code(0x0197e3, 1);
    R[0] = gsu_getb();
    R[5] = gsu_swap(R[0]);
    R[0] = 0x96FE;
    gsu_code(0x0197e4, 7);
    gsu_r14(gsu_add(R[0], R[2], 0));
    gsu_code(0x0197eb, 1);
    R[0] = gsu_getb();
    R[9] = gsu_swap(R[0]);
    R[6] = R[1];
    gsu_code(0x0197ec, 8);
    { uint32_t p = gsu_lmult(R[9], R[6]); R[4] = (uint16_t)p; R[8] = (uint16_t)(p >> 16); }
    R[7] = R[4];
    R[6] = R[3];
    gsu_code(0x0197f4, 7);
    { uint32_t p = gsu_lmult(R[5], R[6]); R[4] = (uint16_t)p; R[0] = (uint16_t)(p >> 16); }
    R[7] = gsu_add(R[7], R[4], 0);
    R[0] = gsu_add(R[0], R[8], gsu.cy);
    R[2] = gsu_asr(R[0]);
    R[6] = R[3];
    gsu_code(0x0197fb, 12);
    { uint32_t p = gsu_lmult(R[9], R[6]); R[4] = (uint16_t)p; R[8] = (uint16_t)(p >> 16); }
    R[7] = R[4];
    R[6] = R[1];
    gsu_code(0x019807, 7);
    { uint32_t p = gsu_lmult(R[5], R[6]); R[4] = (uint16_t)p; R[0] = (uint16_t)(p >> 16); }
    R[7] = gsu_sub(R[7], R[4], 0);
    R[0] = gsu_sub(R[0], R[8], !gsu.cy);
    R[3] = gsu_asr(R[0]);
    R[1] = R[2];
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x01980e, 12);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
lD120:
    R[1] = 0x0200;
    R[12] = 0x3F00;
    R[13] = 0xD128;
    gsu_code(0x01d121, 8);
lD128:
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_code(0x01d129, 2);
    gsu_stw(R[1], R[0]);
    R[1] = gsu_inc(R[1]);
    t = gsu_count_down(&R[12]);
    R[1] = gsu_inc(R[1]);
    gsu_code(0x01d12b, 2);
    if (t) { gsu_code(0x01d128, 1); goto lD128; }
    gsu_code(0x01d12d, 1);
    return;   /* the STOP: the program ends here */
lC4A5:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x01c4a6, 4);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
dispatch:
    if (at == ret)
        return;
    gsu_code(0x010000 | at, 1);   /* where it jumps: its first byte */
fetched:
    if (at == 0xC126)
        gsu_cache((uint16_t)(at + 1));   /* the code there starts with a CACHE */
enter:
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
    goto enter;   /* the caller fetched the first byte */
l8068:
    R[6] = 0x0000;
    R[4] = 0x0000;
    R[12] = 0x0008;
    R[13] = 0x8072;
    gsu_code(0x018069, 10);
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
    gsu_code(0x018073, 15);
    if (t) { gsu_code(0x018085, 1); goto l8085; }
    R[4] = gsu_sub(R[4], R[0], !gsu.cy);
    R[6] = gsu_inc(R[6]);
    gsu_code(0x018082, 4);
l8085:
    t = gsu_count_down(&R[12]);
    gsu_code(0x018086, 1);
    if (t) { gsu_code(0x018072, 1); goto l8072; }
    nx = (uint16_t)(R[11]);
    at = nx;
    gsu_code(0x018087, 2);
    goto dispatch;
l8089:
    R[5] = 0xFFFF;
    R[4] = 0x7FFF;
    R[11] = 0x8094;
    gsu_code(0x01808a, 10); gsu_code(0x018096, 1);
    abi_FxSqrt32();
    gsu_code(0x018094, 1);
l8094:
    return;   /* the STOP: the program ends here */
l80E5:
    R[1] = 0x0100;
    R[2] = 0x0100;
    R[3] = 0x7D00;
    R[11] = 0x80F3;
    gsu_code(0x0180e6, 13); gsu_code(0x0180fe, 1);
    goto l80FE;
l80F3:
    return;   /* the STOP: the program ends here */
l80F5:
    R[1] = gsu_sub(R[4], R[1], 0);
    R[2] = gsu_sub(R[5], R[2], 0);
    R[3] = gsu_sub(R[6], R[3], 0);
    gsu_code(0x0180f6, 9);
l80FE:
    gsu_code(0x0180ff, 2);
    gsu_stw(R[10], R[11]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[6] = R[1];
    gsu_code(0x018101, 7);
    { uint32_t p = gsu_lmult(R[1], R[6]); R[4] = (uint16_t)p; R[0] = (uint16_t)(p >> 16); }
    R[5] = R[4];
    R[6] = R[2];
    gsu_code(0x018108, 8);
    { uint32_t p = gsu_lmult(R[2], R[6]); R[4] = (uint16_t)p; R[6] = (uint16_t)(p >> 16); }
    R[5] = gsu_add(R[5], R[4], 0);
    R[0] = gsu_add(R[0], R[6], gsu.cy);
    R[6] = R[3];
    gsu_code(0x018110, 10);
    { uint32_t p = gsu_lmult(R[3], R[6]); R[4] = (uint16_t)p; R[6] = (uint16_t)(p >> 16); }
    R[5] = gsu_add(R[5], R[4], 0);
    R[0] = gsu_add(R[0], R[6], gsu.cy);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = !gsu.z;
    gsu_code(0x01811a, 8);
    if (t) { gsu_code(0x018132, 1); goto l8132; }
    R[5] = R[5];
    gsu_moves(R[5]);
    t = !gsu.z;
    gsu_code(0x018122, 5);
    if (t) { gsu_code(0x018132, 1); goto l8132; }
    R[1] = 0x0000;
    R[2] = 0x0000;
    R[3] = 0x0000;
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x018127, 11);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
l8132:
    R[4] = R[0];
    R[11] = 0x8139;
    gsu_code(0x018133, 6); gsu_code(0x018096, 1);
    abi_FxSqrt32();
    gsu_code(0x018139, 1);
l8139:
    R[0] = R[6];
    R[9] = 0x0000;
    gsu_code(0x01813a, 4);
l813D:
    R[0] = gsu_lsr(R[0]);
    t = gsu.z;
    gsu_code(0x01813e, 3);
    if (t) { gsu_code(0x018144, 1); goto l8144; }
    t = 1;
    R[9] = gsu_inc(R[9]);
    gsu_code(0x018141, 3);
    gsu_code(0x01813d, 1);
    if (t) goto l813D;
l8144:
    R[0] = gsu_sub(R[0], R[0], 0);
    R[4] = 0x7FFF;
    R[12] = R[9];
    gsu_moves(R[9]);
    t = gsu.z;
    gsu_code(0x018145, 8);
    if (t) { gsu_code(0x018153, 1); goto l8153; }
    R[13] = 0x814F;
    gsu_code(0x01814d, 3);
l814F:
    R[4] = gsu_add(R[4], R[4], 0);
    t = gsu_count_down(&R[12]);
    R[0] = gsu_rol(R[0]);
    gsu_code(0x018150, 3);
    if (t) { gsu_code(0x01814f, 1); goto l814F; }
    gsu_code(0x018153, 1);
l8153:
    R[6] = R[6];
    R[11] = 0x815A;
    gsu_code(0x018154, 6); gsu_code(0x0181bc, 1);
    abi_FxDivide32By16Halving();
    gsu_code(0x01815a, 1);
l815A:
    R[6] = R[4];
    gsu_code(0x01815b, 6);
    { uint32_t p = gsu_lmult(R[1], R[6]); R[4] = (uint16_t)p; R[7] = (uint16_t)(p >> 16); }
    R[1] = R[4];
    gsu_code(0x018161, 6);
    { uint32_t p = gsu_lmult(R[2], R[6]); R[4] = (uint16_t)p; R[8] = (uint16_t)(p >> 16); }
    R[2] = R[4];
    gsu_code(0x018167, 5);
    { uint32_t p = gsu_lmult(R[3], R[6]); R[4] = (uint16_t)p; R[0] = (uint16_t)(p >> 16); }
    R[3] = R[4];
    R[12] = R[9];
    gsu_moves(R[9]);
    t = gsu.z;
    gsu_code(0x01816c, 6);
    if (t) { gsu_code(0x018181, 1); goto l8181; }
    R[13] = 0x8174;
    gsu_code(0x018172, 3);
l8174:
    R[7] = gsu_asr(R[7]);
    R[1] = gsu_ror(R[1]);
    R[8] = gsu_asr(R[8]);
    R[2] = gsu_ror(R[2]);
    R[0] = gsu_asr(R[0]);
    R[3] = gsu_ror(R[3]);
    t = gsu_count_down(&R[12]);
    gsu_code(0x018175, 12);
    if (t) { gsu_code(0x018174, 1); goto l8174; }
    gsu_code(0x018181, 1);
l8181:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x018182, 4);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
l946D:
    gsu_code(0x01946e, 1);
    gsu_r14(gsu_dec(R[14]));
    gsu_code(0x01946f, 1);
    R[0] = gsu_getb();
    gsu_code(0x019470, 3);
    gsu_r14(gsu_add(R[14], 7, 0));
    R[1] = 0x07C6;
    R[0] = gsu_add(R[0], R[1], 0);
    gsu_code(0x019473, 6);
    R[0] = gsu_ldb(R[0]);
    nx = (uint16_t)(R[11]);
    R[0] = gsu_swap(R[0]);
    at = nx;
    gsu_code(0x019479, 1);
    goto dispatch;
l9652:
    R[0] = R[1];
    R[1] = R[2];
    R[2] = R[0];
    gsu_code(0x019653, 6);
    abi_FxRatioClamp();   /* runs on into it */
    return;
dispatch:
    if (at == ret)
        return;
    gsu_code(0x010000 | at, 1);   /* where it jumps: its first byte */
fetched:
enter:
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
