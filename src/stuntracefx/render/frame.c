/*
 * frame.c: the Super FX's frame program (01:AB13) and what it runs: the
 * screen cleared, every object in the sorted draw list drawn (01:AB81) as
 * a model (model.c), a scaled sprite (01:B0D9) or a burst of effect
 * particles (01:B6D5, then the particles' draw), the weather (01:BC7D),
 * the effects and the text, then STOP.
 *
 * The C was drafted instruction for instruction from the ROM bytes and is
 * kept so: every register, flag and RAM access as the code makes them,
 * one per statement, a branch's delay slot after its decision. Jumps
 * through registers (returns pulled from the R10 stack) go through
 * frame_run's dispatch on the address they reach.
 */
#include "gsu.h"
#include "gsu_ops.h"

#define R gsu.r

#define FX_MODEL_AT              0x0016u
#define FX_MODEL_18              0x0018u
#define FX_ROM_BANK              0x001Cu
#define FX_OBJECT_ANGLE_X        0x0020u
#define FX_CAR_0022              0x0022u
#define FX_OBJECT_ANGLE_Z        0x0024u
#define FX_OBJECT_X              0x0026u
#define FX_OBJECT_Y              0x0028u
#define FX_OBJECT_Z              0x002Au
#define FX_POINT_SHIFT           0x0032u
#define FX_MODEL_34              0x0034u
#define FX_MODEL_36              0x0036u
#define FX_SCREEN_LEFT           0x0038u
#define FX_SCREEN_RIGHT          0x003Au
#define FX_SCREEN_TOP            0x003Cu
#define FX_SCREEN_BOTTOM         0x003Eu
#define FX_FACE_KIND             0x0042u
#define FX_MODEL_FRAME           0x0044u
#define FX_MATERIAL_MASK         0x0046u
#define FX_OBJECT_KIND           0x0048u
#define FX_MATERIAL_TABLE        0x004Au
#define FX_FACE_COLOUR           0x0058u
#define FX_TEX_BANK              0x00A6u
#define FX_TEX_BASE              0x00A8u
#define FX_SPRITE_SIZE           0x00AAu
#define FX_SPRITE_CLIP_X         0x00B2u
#define FX_SPRITE_CLIP_W         0x00B4u
#define FX_MODEL_BA              0x00BAu
#define FX_MODEL_BC              0x00BCu
#define FX_TEX_U_OFF             0x00C0u
#define FX_TEX_V_OFF             0x00C2u
#define FX_MODEL_C8              0x00C8u
#define FX_MODEL_CE              0x00CEu
#define FX_TEX_MASK              0x00D4u
#define FX_SPRITE_MATERIAL       0x00D6u
#define GSU_PARAM_00E0           0x00E0u
#define FX_POINTS_TOTAL          0x0144u
#define FX_MODEL_148             0x0148u
#define FX_MODEL_14E             0x014Eu
#define FX_MODEL_150             0x0150u
#define FX_MODEL_1B4             0x01B4u
#define FX_MODEL_1B6             0x01B6u
#define GSU_WORD_1C4             0x01C4u
#define FX_EFFECT_SEED           0x01D0u
#define GSU_DRAW_LIST            0x023Cu
#define GSU_PARAM_108A           0x108Au
#define GSU_TIED_22              0x247Au
#define FX_EFFECT_MAX            0x26EEu
#define FX_CLOSING               0x26F6u
#define FX_HAZE_X8               0x26F8u
#define FX_ME_Z                  0x26FAu
#define FX_WEATHER_LIFE          0x26FCu
#define FX_WEATHER_COLOUR        0x26FEu
#define FX_WEATHER_TURN          0x2B8Eu
#define FX_WEATHER_DRIFT         0x2B90u
#define FX_INIT_2B94             0x2B94u

void abi_FxDivide32By16Halving(void);
void abi_FxEffectsRun(void);
void abi_FxModelDraw(void);
void abi_FxParticlesDraw(void);
void abi_FxProjectScaled(void);
void abi_FxScreenClear(void);
void abi_FxSpriteMaterial(void);
void abi_FxTextNotice(void);

/* The code from address at until it jumps to ret (the caller's return;
   $FFFF for a program, which ends at its STOP). */
static void frame_run(uint16_t at, uint16_t ret)
{
    int t;
    uint16_t nx;

    (void)t;
    (void)nx;
    goto enter;   /* the caller fetched the first byte */
    /* FxFrameProgram */
lAB13:
    R[10] = 0x0240;
    gsu_code(0x01ab14, 6);
    R[0] = gsu_ldw(GSU_WORD_1C4);
    R[0] = gsu_lob(R[0]);
    t = gsu.z;
    gsu_code(0x01ab1a, 3);
    if (t) { gsu_code(0x01ab22, 1); goto lAB22; }
    R[11] = 0xAB22;
    gsu_code(0x01ab1d, 5); gsu_code(0x01aeb9, 1);
    abi_FxScreenClear();
    gsu_code(0x01ab22, 1);
lAB22:
    R[10] = 0x0240;
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_code(0x01ab23, 7);
    gsu_stw(FX_MODEL_1B4, R[0]);
    R[11] = 0xAB2E;
    gsu_code(0x01ab2a, 4); gsu_code(0x01ab81, 1);
    goto lAB81;
lAB2E:
    R[10] = 0x0240;
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_cmode(R[0]);
    gsu_code(0x01ab2f, 10);
    R[2] = gsu_ldw(GSU_TIED_22);
    R[2] = R[2];
    gsu_moves(R[2]);
    t = gsu.z;
    gsu_code(0x01ab39, 4);
    if (t) { gsu_code(0x01ab69, 1); goto lAB69; }
    R[1] = 0x008A;
    R[1] = gsu_add(R[1], R[2], 0);
    gsu_code(0x01ab3d, 8);
    R[1] = gsu_ldw(R[1]);
    R[0] = 0x0001;
    R[0] = gsu_and(R[0], R[1]);
    t = gsu.z;
    gsu_code(0x01ab45, 6);
    if (t) { gsu_code(0x01ab56, 1); goto lAB56; }
    R[0] = 0x0010;
    gsu_code(0x01ab4b, 7);
    gsu_stw(FX_EFFECT_MAX, R[0]);
    R[0] = 0x0001;
    t = 1;
    gsu_code(0x01ab52, 4);
    gsu_code(0x01ab5f, 1);
    if (t) goto lAB5F;
lAB56:
    R[0] = 0x0002;
    R[0] = gsu_and(R[0], R[1]);
    t = gsu.z;
    gsu_code(0x01ab57, 6);
    if (t) { gsu_code(0x01ab69, 1); goto lAB69; }
    R[0] = 0x0002;
    gsu_code(0x01ab5d, 3);
lAB5F:
    R[1] = 0x0050;
    R[11] = 0xAB66;
    gsu_code(0x01ab60, 6); gsu_code(0x01bc7d, 1);
    goto lBC7D;
lAB66:
    t = 1;
    gsu_code(0x01ab67, 2);
    gsu_code(0x01ab6e, 1);
    if (t) goto lAB6E;
lAB69:
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_code(0x01ab6a, 5);
    gsu_stw(FX_INIT_2B94, R[0]);
lAB6E:
    R[11] = 0xAB73;
    gsu_code(0x01ab6f, 4); gsu_code(0x01bf6d, 1);
    abi_FxEffectsRun();
lAB73:
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_code(0x01ab74, 5);
    gsu_stw(FX_EFFECT_MAX, R[0]);
    R[11] = 0xAB7D;
    gsu_code(0x01ab79, 4); gsu_code(0x01d12f, 1);
    abi_FxTextNotice();
lAB7D:
    gsu_code(0x01ab7e, 2);
    R[0] = gsu_rpix_r(R[1], R[2]);
    return;   /* the STOP: the program ends here */
    /* FxObjectsDraw */
lAB81:
    gsu_code(0x01ab82, 4);
    R[1] = gsu_ldw(GSU_DRAW_LIST);
    R[1] = R[1];
    gsu_moves(R[1]);
    t = !gsu.z;
    gsu_code(0x01ab86, 4);
    if (t) { gsu_code(0x01ab8c, 1); goto lAB8C; }
    nx = (uint16_t)(R[11]);
    at = nx;
    gsu_code(0x01ab8a, 2);
    goto dispatch;
lAB8C:
    gsu_code(0x01ab8d, 2);
    gsu_stw(R[10], R[11]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_code(0x01ab8f, 2);
lAB90:
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_code(0x01ab91, 3);
    gsu_romb(R[0]);
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_cmode(R[0]);
    gsu_code(0x01ab94, 6);
    gsu_stw(FX_POINTS_TOTAL, R[0]);
    gsu_code(0x01ab9a, 2);
    gsu_stw(R[10], R[1]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[1] = gsu_inc(R[1]);
    R[1] = gsu_inc(R[1]);
    R[1] = gsu_inc(R[1]);
    R[1] = gsu_inc(R[1]);
    gsu_code(0x01ab9c, 8);
    R[0] = gsu_ldb(R[1]);
    gsu_code(0x01aba4, 3);
    gsu_stw(FX_OBJECT_ANGLE_X, R[0]);
    R[1] = gsu_inc(R[1]);
    gsu_code(0x01aba7, 3);
    R[0] = gsu_ldb(R[1]);
    gsu_code(0x01abaa, 3);
    gsu_stw(FX_CAR_0022, R[0]);
    R[1] = gsu_inc(R[1]);
    gsu_code(0x01abad, 3);
    R[0] = gsu_ldb(R[1]);
    gsu_code(0x01abb0, 3);
    gsu_stw(FX_OBJECT_ANGLE_Z, R[0]);
    R[1] = gsu_inc(R[1]);
    gsu_code(0x01abb3, 4);
    R[4] = gsu_ldb(R[1]);
    R[1] = gsu_inc(R[1]);
    gsu_code(0x01abb7, 3);
    R[3] = gsu_ldw(R[1]);
    R[3] = R[3];
    gsu_moves(R[3]);
    t = !gsu.z;
    gsu_code(0x01abba, 4);
    if (t) { gsu_code(0x01abc2, 1); goto lABC2; }
    gsu_code(0x01abbe, 4); gsu_code(0x01ad53, 1);
    goto lAD53;
lABC2:
    R[1] = gsu_inc(R[1]);
    R[1] = gsu_inc(R[1]);
    R[1] = gsu_inc(R[1]);
    R[1] = gsu_inc(R[1]);
    R[1] = gsu_inc(R[1]);
    R[1] = gsu_inc(R[1]);
    R[1] = gsu_inc(R[1]);
    R[1] = gsu_inc(R[1]);
    gsu_code(0x01abc3, 9);
    R[0] = gsu_ldw(R[1]);
    gsu_code(0x01abcc, 3);
    gsu_stw(FX_OBJECT_Y, R[0]);
    R[1] = gsu_inc(R[1]);
    R[1] = gsu_inc(R[1]);
    gsu_code(0x01abcf, 3);
    R[0] = gsu_ldw(R[1]);
    gsu_code(0x01abd2, 3);
    gsu_stw(FX_OBJECT_X, R[0]);
    R[1] = gsu_inc(R[1]);
    R[1] = gsu_inc(R[1]);
    gsu_code(0x01abd5, 3);
    R[0] = gsu_ldw(R[1]);
    gsu_code(0x01abd8, 3);
    gsu_stw(FX_OBJECT_Z, R[0]);
    R[6] = R[0];
    R[1] = gsu_inc(R[1]);
    R[1] = gsu_inc(R[1]);
    R[5] = 0x0004;
    R[0] = gsu_and(R[4], R[5]);
    t = gsu.z;
    gsu_code(0x01abdb, 11);
    if (t) { gsu_code(0x01ac31, 1); goto lAC31; }
    R[0] = 0xFFF4;
    R[1] = gsu_add(R[0], R[1], 0);
    gsu_code(0x01abe6, 6);
    R[0] = gsu_ldw(R[1]);
    gsu_code(0x01abec, 3);
    gsu_stw(FX_MODEL_148, R[0]);
    R[1] = gsu_inc(R[1]);
    R[1] = gsu_inc(R[1]);
    gsu_code(0x01abef, 4);
    R[0] = gsu_ldb(R[1]);
    R[0] = gsu_lob(R[0]);
    gsu_code(0x01abf3, 4);
    gsu_stw(FX_MODEL_14E, R[0]);
    R[1] = gsu_inc(R[1]);
    gsu_code(0x01abf7, 3);
    R[0] = gsu_ldb(R[1]);
    R[0] = gsu_lob(R[0]);
    gsu_code(0x01abfa, 4);
    gsu_stw(FX_MODEL_150, R[0]);
    R[1] = gsu_inc(R[1]);
    gsu_code(0x01abfe, 3);
    R[0] = gsu_ldb(R[1]);
    R[0] = gsu_lob(R[0]);
    R[5] = 0x0007;
    R[0] = gsu_and(R[0], R[5]);
    t = !gsu.z;
    gsu_code(0x01ac01, 6);
    if (t) { gsu_code(0x01ac09, 1); goto lAC09; }
    R[0] = 0xFFFF;
    gsu_code(0x01ac07, 3);
lAC09:
    gsu_code(0x01ac0a, 3);
    gsu_stw(FX_POINTS_TOTAL, R[0]);
    gsu_code(0x01ac0d, 3);
    R[0] = gsu_ldw(FX_POINTS_TOTAL);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.z;
    gsu_code(0x01ac10, 4);
    if (t) { gsu_code(0x01ac2d, 1); goto lAC2D; }
    t = gsu.s;
    gsu_code(0x01ac14, 3);
    if (t) { gsu_code(0x01ac25, 1); goto lAC25; }
    gsu_code(0x01ac17, 4);
    R[1] = gsu_ldw(FX_MODEL_150);
    gsu_code(0x01ac1b, 3);
    R[2] = gsu_ldw(FX_MODEL_148);
    gsu_code(0x01ac1e, 3);
    R[3] = gsu_ldw(FX_MODEL_14E);
    R[11] = 0xAC25;
    gsu_code(0x01ac21, 4); gsu_code(0x01b6d5, 1);
    goto lB6D5;
lAC25:
    gsu_code(0x01ac26, 3);
    R[2] = gsu_ldw(FX_MODEL_148);
    R[11] = 0xAC2D;
    gsu_code(0x01ac29, 4); gsu_code(0x01ba13, 1);
    abi_FxParticlesDraw();
    gsu_code(0x01ac2d, 1);
lAC2D:
    gsu_code(0x01ac2e, 3); gsu_code(0x01ad53, 1);
    goto lAD53;
lAC31:
    R[5] = 0x0002;
    R[0] = gsu_and(R[4], R[5]);
    t = gsu.z;
    gsu_code(0x01ac32, 6);
    if (t) { gsu_code(0x01ac48, 1); goto lAC48; }
    R[5] = 0x0008;
    R[0] = gsu_and(R[4], R[5]);
    t = !gsu.z;
    gsu_code(0x01ac38, 7);
    if (t) { gsu_code(0x01ac48, 1); goto lAC48; }
    R[5] = 0x8D30;
    gsu_code(0x01ac3f, 7);
    gsu_stw(FX_MATERIAL_TABLE, R[5]);
    t = 1;
    gsu_code(0x01ac46, 2);
    gsu_code(0x01ac6d, 1);
    if (t) goto lAC6D;
lAC48:
    gsu_code(0x01ac49, 1);
    R[0] = gsu_ldw(R[1]);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = !gsu.z;
    gsu_code(0x01ac4a, 4);
    if (t) { gsu_code(0x01ac6a, 1); goto lAC6A; }
    gsu_code(0x01ac4e, 3);
    gsu_r14(0x0012);
    gsu_code(0x01ac51, 3);
    gsu_r14(gsu_add(R[3], R[14], 0));
    gsu_code(0x01ac54, 1);
    R[0] = gsu_getb();
    gsu_code(0x01ac55, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x01ac56, 2);
    R[0] = gsu_getbh(R[0]);
    R[5] = 0x8F91;
    gsu_sub(R[0], R[5], 0);   /* CMP */
    t = !gsu.z;
    gsu_code(0x01ac58, 7);
    if (t) { gsu_code(0x01ac6a, 1); goto lAC6A; }
    gsu_code(0x01ac5f, 5);
    R[5] = gsu_ldw(GSU_PARAM_108A);
    R[5] = R[5];
    gsu_moves(R[5]);
    t = gsu.z;
    gsu_code(0x01ac64, 4);
    if (t) { gsu_code(0x01ac6a, 1); goto lAC6A; }
    R[0] = R[5];
    gsu_code(0x01ac68, 3);
lAC6A:
    gsu_code(0x01ac6b, 3);
    gsu_stw(FX_MATERIAL_TABLE, R[0]);
lAC6D:
    R[1] = gsu_inc(R[1]);
    R[1] = gsu_inc(R[1]);
    gsu_code(0x01ac6e, 4);
    gsu_r14(0x0007);
    gsu_code(0x01ac72, 3);
    gsu_r14(gsu_add(R[3], R[14], 0));
    gsu_code(0x01ac75, 1);
    R[0] = gsu_getb();
    gsu_code(0x01ac76, 3);
    gsu_stw(FX_POINT_SHIFT, R[0]);
    gsu_code(0x01ac79, 2);
    gsu_stw(R[10], R[3]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[0] = R[6];
    R[5] = 0x0BB8;
    gsu_sub(R[0], R[5], 0);   /* CMP */
    t = gsu.cy;
    gsu_code(0x01ac7b, 11);
    if (t) { gsu_code(0x01ac99, 1); goto lAC99; }
    R[5] = 0x07D0;
    gsu_sub(R[0], R[5], 0);   /* CMP */
    t = gsu.cy;
    gsu_code(0x01ac86, 8);
    if (t) { gsu_code(0x01aca7, 1); goto lACA7; }
    R[5] = 0x03E8;
    gsu_sub(R[0], R[5], 0);   /* CMP */
    t = gsu.cy;
    gsu_code(0x01ac8e, 8);
    if (t) { gsu_code(0x01acb5, 1); goto lACB5; }
    t = 1;
    gsu_code(0x01ac96, 3);
    gsu_code(0x01acc0, 1);
    if (t) goto lACC0;
lAC99:
    gsu_code(0x01ac9a, 2);
    gsu_r14(0x001A);
    gsu_code(0x01ac9c, 3);
    gsu_r14(gsu_add(R[3], R[14], 0));
    gsu_code(0x01ac9f, 2);
    R[3] = gsu_getb();
    gsu_code(0x01aca1, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x01aca2, 3);
    R[3] = gsu_getbh(R[3]);
    t = 1;
    gsu_code(0x01aca5, 2);
    gsu_code(0x01acc0, 1);
    if (t) goto lACC0;
lACA7:
    gsu_code(0x01aca8, 2);
    gsu_r14(0x0018);
    gsu_code(0x01acaa, 3);
    gsu_r14(gsu_add(R[3], R[14], 0));
    gsu_code(0x01acad, 2);
    R[3] = gsu_getb();
    gsu_code(0x01acaf, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x01acb0, 3);
    R[3] = gsu_getbh(R[3]);
    t = 1;
    gsu_code(0x01acb3, 2);
    gsu_code(0x01acc0, 1);
    if (t) goto lACC0;
lACB5:
    gsu_code(0x01acb6, 2);
    gsu_r14(0x0016);
    gsu_code(0x01acb8, 3);
    gsu_r14(gsu_add(R[3], R[14], 0));
    gsu_code(0x01acbb, 2);
    R[3] = gsu_getb();
    gsu_code(0x01acbd, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x01acbe, 3);
    R[3] = gsu_getbh(R[3]);
lACC0:
    gsu_code(0x01acc1, 2);
    gsu_r14(0x0000);
    gsu_code(0x01acc3, 3);
    gsu_r14(gsu_add(R[3], R[14], 0));
    gsu_code(0x01acc6, 1);
    R[0] = gsu_getb();
    gsu_code(0x01acc7, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x01acc8, 2);
    R[0] = gsu_getbh(R[0]);
    gsu_code(0x01acca, 3);
    gsu_stw(FX_MODEL_AT, R[0]);
    gsu_code(0x01accd, 2);
    gsu_r14(0x0002);
    gsu_code(0x01accf, 3);
    gsu_r14(gsu_add(R[3], R[14], 0));
    gsu_code(0x01acd2, 1);
    R[0] = gsu_getb();
    gsu_code(0x01acd3, 3);
    gsu_stw(FX_ROM_BANK, R[0]);
    gsu_code(0x01acd6, 2);
    gsu_r14(0x0003);
    gsu_code(0x01acd8, 3);
    gsu_r14(gsu_add(R[3], R[14], 0));
    gsu_code(0x01acdb, 1);
    R[0] = gsu_getb();
    gsu_code(0x01acdc, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x01acdd, 2);
    R[0] = gsu_getbh(R[0]);
    gsu_code(0x01acdf, 3);
    gsu_stw(FX_MODEL_18, R[0]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x01ace2, 4);
    R[3] = gsu_ldw(R[10]);
    gsu_code(0x01ace6, 2);
    R[0] = gsu_ldb(R[1]);
    R[1] = gsu_inc(R[1]);
    gsu_code(0x01ace8, 4);
    gsu_stw(FX_OBJECT_KIND, R[0]);
    gsu_code(0x01acec, 2);
    R[0] = gsu_ldb(R[1]);
    gsu_code(0x01acee, 3);
    gsu_stw(FX_MODEL_FRAME, R[0]);
    R[1] = gsu_inc(R[1]);
    gsu_code(0x01acf1, 3);
    R[0] = gsu_ldb(R[1]);
    gsu_code(0x01acf4, 3);
    gsu_stw(FX_MATERIAL_MASK, R[0]);
    R[1] = gsu_inc(R[1]);
    R[5] = 0x0008;
    R[0] = gsu_and(R[4], R[5]);
    t = gsu.z;
    gsu_code(0x01acf7, 7);
    if (t) { gsu_code(0x01ad31, 1); goto lAD31; }
    gsu_code(0x01acfe, 4);
    R[0] = gsu_ldw(FX_MODEL_1B4);
    R[0] = gsu_lob(R[0]);
    t = !gsu.z;
    gsu_code(0x01ad02, 3);
    if (t) { gsu_code(0x01ad31, 1); goto lAD31; }
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_code(0x01ad05, 4);
    gsu_romb(R[0]);
    gsu_code(0x01ad09, 2);
    gsu_r14(0x0010);
    gsu_code(0x01ad0b, 3);
    gsu_r14(gsu_add(R[3], R[14], 0));
    gsu_code(0x01ad0e, 1);
    R[0] = gsu_getb();
    gsu_code(0x01ad0f, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x01ad10, 2);
    R[0] = gsu_getbh(R[0]);
    gsu_code(0x01ad12, 3);
    gsu_stw(FX_SPRITE_SIZE, R[0]);
    gsu_code(0x01ad15, 2);
    R[0] = gsu_ldb(R[1]);
    gsu_code(0x01ad17, 3);
    gsu_stw(FX_SPRITE_MATERIAL, R[0]);
    R[1] = gsu_inc(R[1]);
    gsu_code(0x01ad1a, 3);
    R[0] = gsu_ldb(R[1]);
    R[0] = gsu_sex(R[0]);
    gsu_code(0x01ad1d, 4);
    gsu_stw(FX_MODEL_CE, R[0]);
    gsu_code(0x01ad21, 3);
    R[0] = gsu_ldw(FX_MODEL_AT);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.z;
    gsu_code(0x01ad24, 4);
    if (t) { gsu_code(0x01ad2d, 1); goto lAD2D; }
    R[11] = 0xAD2D;
    gsu_code(0x01ad28, 5); gsu_code(0x01b0d9, 1);
    goto lB0D9;
lAD2D:
    gsu_code(0x01ad2e, 3); gsu_code(0x01ad53, 1);
    goto lAD53;
lAD31:
    gsu_code(0x01ad32, 2);
    R[0] = gsu_ldb(R[1]);
    gsu_code(0x01ad34, 3);
    gsu_stw(FX_MODEL_1B6, R[0]);
    R[1] = gsu_inc(R[1]);
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_code(0x01ad37, 4);
    R[0] = gsu_ldb(R[1]);
    R[0] = gsu_swap(R[0]);
    gsu_code(0x01ad3b, 4);
    gsu_stw(FX_TEX_U_OFF, R[0]);
    R[1] = gsu_inc(R[1]);
    gsu_code(0x01ad3f, 3);
    R[0] = gsu_ldb(R[1]);
    R[0] = gsu_swap(R[0]);
    gsu_code(0x01ad42, 4);
    gsu_stw(FX_TEX_V_OFF, R[0]);
    R[1] = gsu_inc(R[1]);
    gsu_code(0x01ad46, 4);
    R[0] = gsu_ldw(FX_MODEL_18);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.z;
    gsu_code(0x01ad4a, 4);
    if (t) { gsu_code(0x01ad53, 1); goto lAD53; }
    R[11] = 0xAD53;
    gsu_code(0x01ad4e, 5); gsu_code(0x018561, 1);
    abi_FxModelDraw();
lAD53:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x01ad54, 4);
    R[1] = gsu_ldw(R[10]);
    gsu_code(0x01ad58, 2);
    R[1] = gsu_ldw(R[1]);
    R[1] = R[1];
    gsu_moves(R[1]);
    t = gsu.z;
    gsu_code(0x01ad5a, 4);
    if (t) { gsu_code(0x01ad62, 1); goto lAD62; }
    gsu_code(0x01ad5e, 4); gsu_code(0x01ab90, 1);
    goto lAB90;
lAD62:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x01ad63, 5);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
    /* FxObjectSprite */
lB0D9:
    gsu_code(0x01b0da, 2);
    gsu_stw(R[10], R[11]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_code(0x01b0dc, 5);
    R[3] = gsu_ldw(FX_OBJECT_Z);
    R[0] = 0x0080;
    R[0] = gsu_sub(R[0], R[3], 0);
    t = !gsu.s;
    gsu_code(0x01b0e1, 6);
    if (t) { gsu_code(0x01b123, 1); goto lB123; }
    gsu_code(0x01b0e7, 4);
    R[0] = gsu_ldw(FX_MODEL_CE);
    R[1] = R[0];
    gsu_moves(R[0]);
    t = gsu.s;
    gsu_code(0x01b0eb, 4);
    if (t) { gsu_code(0x01b128, 1); goto lB128; }
    /* CACHE */
    gsu_code(0x01b0ef, 1); gsu_cache(0xb0f0); gsu_code(0x01b0f0, 4);
    R[12] = gsu_ldw(FX_POINT_SHIFT);
    R[12] = R[12];
    gsu_moves(R[12]);
    t = gsu.z;
    gsu_code(0x01b0f4, 4);
    if (t) { gsu_code(0x01b100, 1); goto lB100; }
    R[0] = R[1];
    R[13] = 0xB0FC;
    gsu_code(0x01b0f8, 5);
lB0FC:
    t = gsu_count_down(&R[12]);
    R[0] = gsu_add(R[0], R[0], 0);
    gsu_code(0x01b0fd, 1);
    if (t) { gsu_code(0x01b0fc, 1); goto lB0FC; }
    R[1] = R[0];
    gsu_code(0x01b0fe, 3);
lB100:
    gsu_code(0x01b101, 3);
    R[0] = gsu_ldw(FX_SPRITE_SIZE);
    R[0] = gsu_add(R[0], R[0], 0);
    R[1] = gsu_add(R[0], R[1], 0);
    t = !gsu.z;
    gsu_code(0x01b104, 5);
    if (t) { gsu_code(0x01b10b, 1); goto lB10B; }
    R[1] = 0x0001;
    gsu_code(0x01b109, 3);
lB10B:
    R[5] = R[1];
    gsu_code(0x01b10c, 5);
    R[1] = gsu_ldw(FX_OBJECT_X);
    gsu_code(0x01b111, 3);
    R[2] = gsu_ldw(FX_OBJECT_Y);
    R[11] = 0xB118;
    gsu_code(0x01b114, 4); gsu_code(0x01b2a4, 1);
    abi_FxProjectScaled();
    gsu_code(0x01b118, 1);
lB118:
    gsu_code(0x01b119, 3);
    gsu_stw(FX_MODEL_BA, R[1]);
    gsu_code(0x01b11c, 3);
    gsu_stw(FX_MODEL_BC, R[2]);
    R[1] = R[5];
    gsu_moves(R[5]);
    t = !gsu.z;
    gsu_code(0x01b11f, 4);
    if (t) { gsu_code(0x01b14c, 1); goto lB14C; }
    gsu_code(0x01b123, 1);
lB123:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x01b124, 4);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
lB128:
    gsu_code(0x01b129, 2);
    gsu_stw(R[10], R[1]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_code(0x01b12b, 5);
    R[1] = gsu_ldw(FX_OBJECT_X);
    gsu_code(0x01b130, 3);
    R[2] = gsu_ldw(FX_OBJECT_Y);
    R[11] = 0xB137;
    gsu_code(0x01b133, 4); gsu_code(0x01b2a4, 1);
    abi_FxProjectScaled();
    gsu_code(0x01b137, 1);
lB137:
    gsu_code(0x01b138, 3);
    gsu_stw(FX_MODEL_BA, R[1]);
    gsu_code(0x01b13b, 3);
    gsu_stw(FX_MODEL_BC, R[2]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x01b13e, 4);
    R[1] = gsu_ldw(R[10]);
    R[1] = gsu_not(R[1]);
    R[1] = gsu_inc(R[1]);
    t = !gsu.z;
    gsu_code(0x01b142, 5);
    if (t) { gsu_code(0x01b14c, 1); goto lB14C; }
    gsu_code(0x01b147, 1);
lB147:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x01b148, 4);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
lB14C:
    R[0] = 0x00F0;
    R[0] = gsu_sub(R[0], R[1], 0);
    t = !gsu.s;
    R[0] = gsu_add(R[0], R[1], 0);
    gsu_code(0x01b14d, 6);
    if (t) { gsu_code(0x01b155, 1); goto lB155; }
    R[1] = R[0];
    gsu_code(0x01b153, 3);
lB155:
    gsu_code(0x01b156, 3);
    gsu_stw(FX_MODEL_CE, R[1]);
    gsu_code(0x01b159, 3);
    R[0] = gsu_ldw(FX_SPRITE_MATERIAL);
    R[11] = 0xB160;
    gsu_code(0x01b15c, 4); gsu_code(0x019276, 1);
    abi_FxSpriteMaterial();
lB160:
    gsu_code(0x01b161, 3);
    R[0] = gsu_ldw(FX_FACE_KIND);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.z;
    gsu_code(0x01b164, 4);
    if (t) { gsu_code(0x01b123, 1); goto lB123; }
    gsu_code(0x01b168, 4);
    R[1] = gsu_ldw(FX_MODEL_CE);
    gsu_code(0x01b16c, 3);
    R[0] = gsu_ldw(FX_TEX_MASK);
    R[0] = gsu_lob(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_code(0x01b16f, 5);
    gsu_stw(FX_MODEL_C8, R[0]);
    gsu_code(0x01b174, 3);
    R[0] = gsu_ldw(FX_FACE_COLOUR);
    gsu_color((uint8_t)R[0]);
    R[0] = gsu_swap(R[0]);
    gsu_cmode(R[0]);
    t = 1;
    gsu_code(0x01b177, 6);
    gsu_code(0x01b182, 1);
    if (t) goto lB182;
    /* FxObjectSpriteAt */
lB17D:
    gsu_code(0x01b17e, 2);
    gsu_stw(R[10], R[11]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    /* CACHE */
    gsu_code(0x01b180, 2); gsu_cache(0xb182); gsu_code(0x01b182, 1);
lB182:
    R[3] = gsu_not(R[1]);
    R[3] = gsu_inc(R[3]);
    gsu_code(0x01b183, 7);
    gsu_stw(FX_SPRITE_CLIP_X, R[3]);
    gsu_code(0x01b18a, 3);
    R[3] = gsu_ldw(FX_SCREEN_RIGHT);
    R[3] = gsu_sub(R[3], R[1], 0);
    gsu_code(0x01b18d, 5);
    gsu_stw(FX_SPRITE_CLIP_W, R[3]);
    R[3] = 0xFFFF;
    R[0] = gsu_lsr(R[1]);
    R[0] = gsu_sub(R[0], R[3], !gsu.cy);
    gsu_code(0x01b192, 9);
    R[3] = gsu_ldw(FX_MODEL_BA);
    R[0] = gsu_sub(R[3], R[0], 0);
    gsu_code(0x01b19b, 3);
    gsu_sbk(R[0]);
    gsu_code(0x01b19e, 3);
    R[3] = gsu_ldw(FX_SPRITE_CLIP_X);
    R[3] = gsu_inc(R[3]);
    gsu_sub(R[0], R[3], 0);   /* CMP */
    t = !gsu.s;
    gsu_code(0x01b1a1, 5);
    if (t) { gsu_code(0x01b1ab, 1); goto lB1AB; }
    gsu_code(0x01b1a6, 1);
lB1A6:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x01b1a7, 4);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
lB1AB:
    gsu_code(0x01b1ac, 3);
    R[3] = gsu_ldw(FX_SCREEN_RIGHT);
    gsu_sub(R[0], R[3], 0);   /* CMP */
    t = !gsu.s;
    gsu_code(0x01b1af, 4);
    if (t) { gsu_code(0x01b1a6, 1); goto lB1A6; }
    R[3] = 0xFFFF;
    R[0] = gsu_lsr(R[1]);
    R[0] = gsu_sub(R[0], R[3], !gsu.cy);
    gsu_code(0x01b1b3, 10);
    R[3] = gsu_ldw(FX_MODEL_BC);
    R[0] = gsu_sub(R[3], R[0], 0);
    gsu_code(0x01b1bd, 3);
    gsu_sbk(R[0]);
    gsu_code(0x01b1c0, 3);
    R[6] = gsu_ldw(FX_MODEL_C8);
    R[0] = gsu_sub(R[1], R[6], 0);
    t = !gsu.cy;
    gsu_code(0x01b1c3, 4);
    if (t) { gsu_code(0x01b1cb, 1); goto lB1CB; }
    gsu_code(0x01b1c7, 4); gsu_code(0x01b2ed, 1);
    goto lB2ED;
lB1CB:
    R[4] = R[6];
    R[0] = 0x0000;
    gsu_code(0x01b1cc, 6);
    gsu_romb(R[0]);
    R[0] = 0x94A0;
    R[0] = gsu_add(R[0], R[1], 0);
    gsu_code(0x01b1d2, 6);
    gsu_r14(gsu_add(R[0], R[1], 0));
    gsu_code(0x01b1d8, 2);
    R[6] = gsu_getb();
    gsu_code(0x01b1da, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x01b1db, 3);
    R[6] = gsu_getbh(R[6]);
    R[0] = gsu_swap(R[4]);
    gsu_code(0x01b1de, 3);
    R[0] = gsu_fmult(R[0], R[6]);
    R[9] = gsu_rol(R[0]);
    /* CACHE */
    gsu_code(0x01b1e1, 2); gsu_cache(0xb1e3); gsu_code(0x01b1e3, 4);
    R[2] = gsu_ldw(FX_MODEL_BC);
    R[7] = 0x0000;
    R[8] = R[7];
    gsu_code(0x01b1e7, 7);
    R[4] = gsu_ldw(FX_MODEL_CE);
    R[6] = R[4];
    gsu_code(0x01b1ee, 5);
    R[0] = gsu_ldw(FX_TEX_BANK);
    gsu_code(0x01b1f3, 2);
    gsu_romb(R[0]);
    gsu_code(0x01b1f5, 3);
    R[5] = gsu_ldw(FX_TEX_BASE);
    gsu_code(0x01b1f8, 3);
    R[3] = gsu_ldw(FX_MODEL_BA);
    gsu_code(0x01b1fb, 3);
    R[0] = gsu_ldw(FX_TEX_MASK);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = !gsu.s;
    gsu_code(0x01b1fe, 4);
    if (t) { gsu_code(0x01b206, 1); goto lB206; }
    gsu_code(0x01b202, 4); gsu_code(0x01b251, 1);
    goto lB251;
lB206:
    R[3] = R[3];
    gsu_moves(R[3]);
    t = !gsu.s;
    gsu_code(0x01b207, 4);
    if (t) { gsu_code(0x01b220, 1); goto lB220; }
    R[12] = R[6];
    R[13] = 0xB20F;
    gsu_code(0x01b20b, 5);
lB20F:
    t = !gsu.s;
    gsu_code(0x01b210, 2);
    if (t) { gsu_code(0x01b21b, 1); goto lB21B; }
    R[8] = gsu_add(R[8], R[9], 0);
    t = gsu_count_down(&R[12]);
    R[3] = gsu_inc(R[3]);
    gsu_code(0x01b212, 4);
    if (t) { gsu_code(0x01b20f, 1); goto lB20F; }
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x01b216, 5);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
lB21B:
    R[6] = R[12];
    R[0] = gsu_merge();
    R[5] = gsu_add(R[0], R[5], 0);
    gsu_code(0x01b21c, 5);
lB220:
    gsu_code(0x01b221, 3);
    R[0] = gsu_ldw(FX_SPRITE_CLIP_W);
    R[0] = gsu_sub(R[0], R[3], 0);
    t = !gsu.s;
    gsu_code(0x01b224, 3);
    if (t) { gsu_code(0x01b22a, 1); goto lB22A; }
    R[6] = gsu_add(R[6], R[0], 0);
    R[6] = gsu_inc(R[6]);
    gsu_code(0x01b227, 4);
lB22A:
    R[13] = 0xB23E;
    gsu_code(0x01b22b, 3);
lB22D:
    gsu_code(0x01b22e, 3);
    R[0] = gsu_ldw(FX_SCREEN_BOTTOM);
    R[0] = gsu_dec(R[0]);
    R[0] = gsu_sub(R[0], R[2], 0);
    t = !gsu.cy;
    gsu_code(0x01b231, 4);
    if (t) { gsu_code(0x01b246, 1); goto lB246; }
    R[8] = 0x0000;
    R[0] = gsu_merge();
    gsu_code(0x01b235, 6);
    gsu_r14(gsu_add(R[0], R[5], 0));
    R[1] = R[3];
    R[12] = R[6];
    gsu_code(0x01b23b, 4);
lB23E:
    R[8] = gsu_add(R[8], R[9], 0);
    R[0] = gsu_merge();
    gsu_code(0x01b23f, 4);
    gsu_color(gsu_getb());   /* GETC */
    gsu_code(0x01b243, 2);
    gsu_r14(gsu_add(R[0], R[5], 0));
    t = gsu_count_down(&R[12]);
    gsu_code(0x01b245, 1);
    gsu_code(t ? 0x01b23e : 0x01b246, 1);   /* the delay slot runs after the next fetch */
    gsu_plot_r1();
    if (t) goto lB23E;
lB246:
    R[7] = gsu_add(R[7], R[9], 0);
    R[4] = gsu_dec(R[4]);
    t = !gsu.z;
    R[2] = gsu_inc(R[2]);
    gsu_code(0x01b247, 5);
    if (t) { gsu_code(0x01b22d, 1); goto lB22D; }
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x01b24c, 5);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
lB251:
    gsu_code(0x01b252, 3);
    R[0] = gsu_ldw(FX_MODEL_C8);
    R[5] = gsu_add(R[5], R[0], 0);
    R[5] = gsu_dec(R[5]);
    R[3] = R[3];
    gsu_moves(R[3]);
    t = !gsu.s;
    gsu_code(0x01b255, 7);
    if (t) { gsu_code(0x01b271, 1); goto lB271; }
    R[12] = R[6];
    R[13] = 0xB260;
    gsu_code(0x01b25c, 5);
lB260:
    t = !gsu.s;
    gsu_code(0x01b261, 2);
    if (t) { gsu_code(0x01b26c, 1); goto lB26C; }
    R[8] = gsu_sub(R[8], R[9], 0);
    t = gsu_count_down(&R[12]);
    R[3] = gsu_inc(R[3]);
    gsu_code(0x01b263, 4);
    if (t) { gsu_code(0x01b260, 1); goto lB260; }
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x01b267, 5);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
lB26C:
    R[6] = R[12];
    R[0] = gsu_merge();
    R[5] = gsu_add(R[0], R[5], 0);
    gsu_code(0x01b26d, 5);
lB271:
    R[1] = R[3];
    gsu_code(0x01b272, 5);
    R[0] = gsu_ldw(FX_SPRITE_CLIP_W);
    R[0] = gsu_sub(R[0], R[1], 0);
    t = !gsu.s;
    gsu_code(0x01b277, 3);
    if (t) { gsu_code(0x01b27d, 1); goto lB27D; }
    R[6] = gsu_add(R[6], R[0], 0);
    R[6] = gsu_inc(R[6]);
    gsu_code(0x01b27a, 4);
lB27D:
    R[13] = 0xB291;
    gsu_code(0x01b27e, 3);
lB280:
    gsu_code(0x01b281, 3);
    R[0] = gsu_ldw(FX_SCREEN_BOTTOM);
    R[0] = gsu_dec(R[0]);
    R[0] = gsu_sub(R[0], R[2], 0);
    t = !gsu.cy;
    gsu_code(0x01b284, 4);
    if (t) { gsu_code(0x01b299, 1); goto lB299; }
    R[8] = 0x0000;
    R[0] = gsu_merge();
    gsu_code(0x01b288, 6);
    gsu_r14(gsu_add(R[0], R[5], 0));
    R[1] = R[3];
    R[12] = R[6];
    gsu_code(0x01b28e, 4);
lB291:
    R[8] = gsu_sub(R[8], R[9], 0);
    R[0] = gsu_merge();
    gsu_code(0x01b292, 4);
    gsu_color(gsu_getb());   /* GETC */
    gsu_code(0x01b296, 2);
    gsu_r14(gsu_add(R[0], R[5], 0));
    t = gsu_count_down(&R[12]);
    gsu_code(0x01b298, 1);
    gsu_code(t ? 0x01b291 : 0x01b299, 1);   /* the delay slot runs after the next fetch */
    gsu_plot_r1();
    if (t) goto lB291;
lB299:
    R[7] = gsu_add(R[7], R[9], 0);
    R[4] = gsu_dec(R[4]);
    t = !gsu.z;
    R[2] = gsu_inc(R[2]);
    gsu_code(0x01b29a, 5);
    if (t) { gsu_code(0x01b280, 1); goto lB280; }
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x01b29f, 5);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
    /* FxObjectSpriteRows */
lB2ED:
    R[0] = R[6];
    R[6] = R[1];
    gsu_sub(R[0], R[6], 0);   /* CMP */
    t = !gsu.z;
    gsu_code(0x01b2ee, 8);
    if (t) { gsu_code(0x01b2fc, 1); goto lB2FC; }
    R[4] = 0xFFFF;
    t = 1;
    gsu_code(0x01b2f6, 6);
    gsu_code(0x01b303, 1);
    if (t) goto lB303;
lB2FC:
    R[4] = 0x0000;
    R[11] = 0xB303;
    gsu_code(0x01b2fd, 6); gsu_code(0x0181bc, 1);
    abi_FxDivide32By16Halving();
    gsu_code(0x01b303, 1);
lB303:
    R[9] = R[4];
    /* CACHE */
    gsu_code(0x01b304, 2); gsu_cache(0xb306); gsu_code(0x01b306, 4);
    R[6] = gsu_ldw(FX_MODEL_C8);
    gsu_code(0x01b30a, 3);
    R[3] = gsu_ldw(FX_MODEL_BA);
    gsu_code(0x01b30d, 3);
    R[2] = gsu_ldw(FX_MODEL_BC);
    R[7] = 0x0000;
    R[11] = 0x0000;
    gsu_code(0x01b310, 7);
    R[0] = gsu_ldw(FX_TEX_BANK);
    gsu_code(0x01b317, 2);
    gsu_romb(R[0]);
    gsu_code(0x01b319, 3);
    R[5] = gsu_ldw(FX_TEX_BASE);
    R[9] = gsu_bic(R[9], 1);
    gsu_code(0x01b31c, 6);
    R[0] = gsu_ldw(FX_TEX_MASK);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = !gsu.s;
    gsu_code(0x01b322, 4);
    if (t) { gsu_code(0x01b32a, 1); goto lB32A; }
    gsu_code(0x01b326, 4); gsu_code(0x01b3ab, 1);
    goto lB3AB;
lB32A:
    R[13] = R[6];
    gsu_code(0x01b32b, 5);
    R[0] = gsu_ldw(FX_SCREEN_RIGHT);
    R[6] = gsu_sub(R[0], R[3], 0);
    R[0] = gsu_lsr(R[9]);
    gsu_code(0x01b330, 5);
    R[0] = gsu_fmult(R[0], R[6]);
    R[4] = gsu_rol(R[0]);
    R[0] = gsu_sub(R[13], R[4], 0);
    R[0] = gsu_dec(R[0]);
    t = gsu.s;
    gsu_code(0x01b335, 7);
    if (t) { gsu_code(0x01b33e, 1); goto lB33E; }
    R[13] = R[4];
    gsu_code(0x01b33c, 3);
lB33E:
    R[6] = R[13];
    R[3] = R[3];
    gsu_moves(R[3]);
    t = !gsu.s;
    gsu_code(0x01b33f, 6);
    if (t) { gsu_code(0x01b35e, 1); goto lB35E; }
    R[6] = gsu_not(R[3]);
    R[6] = gsu_inc(R[6]);
    R[0] = gsu_lsr(R[9]);
    gsu_code(0x01b345, 9);
    { uint32_t p = gsu_lmult(R[0], R[6]); R[4] = (uint16_t)p; R[0] = (uint16_t)(p >> 16); }
    R[11] = gsu_add(R[4], R[4], 0);
    R[0] = gsu_rol(R[0]);
    t = gsu.s;
    gsu_code(0x01b34e, 6);
    if (t) { gsu_code(0x01b3a6, 1); goto lB3A6; }
    R[5] = gsu_add(R[5], R[0], 0);
    R[6] = gsu_sub(R[13], R[0], 0);
    t = gsu.s;
    gsu_code(0x01b354, 8);
    if (t) { gsu_code(0x01b3a6, 1); goto lB3A6; }
    R[3] = 0x0000;
    gsu_code(0x01b35c, 3);
lB35E:
    R[6] = R[6];
    gsu_moves(R[6]);
    t = gsu.z;
    gsu_code(0x01b35f, 4);
    if (t) { gsu_code(0x01b3a6, 1); goto lB3A6; }
    R[4] = 0x0000;
    gsu_code(0x01b363, 3);
lB365:
    R[2] = R[2];
    gsu_moves(R[2]);
    t = gsu.s;
    gsu_code(0x01b366, 4);
    if (t) { gsu_code(0x01b398, 1); goto lB398; }
    gsu_code(0x01b36a, 4);
    R[0] = gsu_ldw(FX_SCREEN_BOTTOM);
    R[0] = gsu_dec(R[0]);
    R[0] = gsu_sub(R[0], R[2], 0);
    t = gsu.s;
    gsu_code(0x01b36e, 4);
    if (t) { gsu_code(0x01b3a6, 1); goto lB3A6; }
    R[0] = gsu_swap(R[4]);
    gsu_code(0x01b372, 5);
    gsu_r14(gsu_add(R[0], R[5], 0));
    R[1] = R[3];
    R[0] = R[11];
    gsu_code(0x01b377, 5);
    gsu_color(gsu_getb());   /* GETC */
    R[12] = R[6];
    R[12] = gsu_dec(R[12]);
    t = gsu.z;
    gsu_code(0x01b37c, 5);
    if (t) { gsu_code(0x01b38a, 1); goto lB38A; }
    R[13] = 0xB383;
    gsu_code(0x01b381, 3);
lB383:
    gsu_code(0x01b384, 1);
    gsu_r14(gsu_inc(R[14]));
lB384:
    R[0] = gsu_add(R[0], R[9], 0);
    t = !gsu.cy;
    gsu_code(0x01b385, 3);
    gsu_code(t ? 0x01b384 : 0x01b388, 1);   /* the delay slot runs after the next fetch */
    gsu_plot_r1();
    if (t) goto lB384;
    t = gsu_count_down(&R[12]);
    gsu_code(0x01b389, 1);
    gsu_code(t ? 0x01b383 : 0x01b38a, 1);   /* the delay slot runs after the next fetch */
    gsu_color(gsu_getb());   /* GETC */
    if (t) goto lB383;
lB38A:
    R[8] = R[0];
    gsu_code(0x01b38b, 5);
    R[0] = gsu_ldw(FX_SCREEN_RIGHT);
lB38F:
    gsu_sub(R[0], R[1], 0);   /* CMP */
    t = gsu.s;
    gsu_code(0x01b390, 4);
    if (t) { gsu_code(0x01b398, 1); goto lB398; }
    R[8] = gsu_add(R[8], R[9], 0);
    t = !gsu.cy;
    gsu_code(0x01b394, 4);
    gsu_code(t ? 0x01b38f : 0x01b398, 1);   /* the delay slot runs after the next fetch */
    gsu_plot_r1();
    if (t) goto lB38F;
lB398:
    R[7] = gsu_sub(R[7], R[9], 0);
    t = gsu.cy;
    R[2] = gsu_inc(R[2]);
    gsu_code(0x01b399, 5);
    if (t) { gsu_code(0x01b365, 1); goto lB365; }
    R[4] = gsu_inc(R[4]);
    gsu_code(0x01b39e, 5);
    R[0] = gsu_ldw(FX_MODEL_C8);
    R[0] = gsu_sub(R[0], R[4], 0);
    t = !gsu.z;
    gsu_code(0x01b3a3, 3);
    if (t) { gsu_code(0x01b365, 1); goto lB365; }
    gsu_code(0x01b3a6, 1);
lB3A6:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x01b3a7, 4);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
lB3AB:
    R[5] = gsu_add(R[5], R[6], 0);
    R[5] = gsu_dec(R[5]);
    R[13] = R[6];
    gsu_code(0x01b3ac, 8);
    R[0] = gsu_ldw(FX_SCREEN_RIGHT);
    R[6] = gsu_sub(R[0], R[3], 0);
    R[0] = gsu_lsr(R[9]);
    gsu_code(0x01b3b4, 5);
    R[0] = gsu_fmult(R[0], R[6]);
    R[4] = gsu_rol(R[0]);
    R[0] = gsu_sub(R[13], R[4], 0);
    R[0] = gsu_dec(R[0]);
    t = gsu.s;
    gsu_code(0x01b3b9, 7);
    if (t) { gsu_code(0x01b3c2, 1); goto lB3C2; }
    R[13] = R[4];
    gsu_code(0x01b3c0, 3);
lB3C2:
    R[6] = R[13];
    R[3] = R[3];
    gsu_moves(R[3]);
    t = !gsu.s;
    gsu_code(0x01b3c3, 6);
    if (t) { gsu_code(0x01b3e2, 1); goto lB3E2; }
    R[6] = gsu_not(R[3]);
    R[6] = gsu_inc(R[6]);
    R[0] = gsu_lsr(R[9]);
    gsu_code(0x01b3c9, 9);
    { uint32_t p = gsu_lmult(R[0], R[6]); R[4] = (uint16_t)p; R[0] = (uint16_t)(p >> 16); }
    R[11] = gsu_add(R[4], R[4], 0);
    R[0] = gsu_rol(R[0]);
    t = gsu.s;
    gsu_code(0x01b3d2, 6);
    if (t) { gsu_code(0x01b42a, 1); goto lB42A; }
    R[5] = gsu_sub(R[5], R[0], 0);
    R[6] = gsu_sub(R[13], R[0], 0);
    t = gsu.s;
    gsu_code(0x01b3d8, 8);
    if (t) { gsu_code(0x01b42a, 1); goto lB42A; }
    R[3] = 0x0000;
    gsu_code(0x01b3e0, 3);
lB3E2:
    R[6] = R[6];
    gsu_moves(R[6]);
    t = gsu.z;
    gsu_code(0x01b3e3, 4);
    if (t) { gsu_code(0x01b42a, 1); goto lB42A; }
    R[4] = 0x0000;
    gsu_code(0x01b3e7, 3);
lB3E9:
    R[2] = R[2];
    gsu_moves(R[2]);
    t = gsu.s;
    gsu_code(0x01b3ea, 4);
    if (t) { gsu_code(0x01b41c, 1); goto lB41C; }
    gsu_code(0x01b3ee, 4);
    R[0] = gsu_ldw(FX_SCREEN_BOTTOM);
    R[0] = gsu_dec(R[0]);
    R[0] = gsu_sub(R[0], R[2], 0);
    t = gsu.s;
    gsu_code(0x01b3f2, 4);
    if (t) { gsu_code(0x01b42a, 1); goto lB42A; }
    R[0] = gsu_swap(R[4]);
    gsu_code(0x01b3f6, 5);
    gsu_r14(gsu_add(R[0], R[5], 0));
    R[1] = R[3];
    R[0] = R[11];
    gsu_code(0x01b3fb, 5);
    gsu_color(gsu_getb());   /* GETC */
    R[12] = R[6];
    R[12] = gsu_dec(R[12]);
    t = gsu.z;
    gsu_code(0x01b400, 5);
    if (t) { gsu_code(0x01b40e, 1); goto lB40E; }
    R[13] = 0xB407;
    gsu_code(0x01b405, 3);
lB407:
    gsu_code(0x01b408, 1);
    gsu_r14(gsu_dec(R[14]));
lB408:
    R[0] = gsu_add(R[0], R[9], 0);
    t = !gsu.cy;
    gsu_code(0x01b409, 3);
    gsu_code(t ? 0x01b408 : 0x01b40c, 1);   /* the delay slot runs after the next fetch */
    gsu_plot_r1();
    if (t) goto lB408;
    t = gsu_count_down(&R[12]);
    gsu_code(0x01b40d, 1);
    gsu_code(t ? 0x01b407 : 0x01b40e, 1);   /* the delay slot runs after the next fetch */
    gsu_color(gsu_getb());   /* GETC */
    if (t) goto lB407;
lB40E:
    R[8] = R[0];
    gsu_code(0x01b40f, 5);
    R[0] = gsu_ldw(FX_SCREEN_RIGHT);
lB413:
    gsu_sub(R[0], R[1], 0);   /* CMP */
    t = gsu.s;
    gsu_code(0x01b414, 4);
    if (t) { gsu_code(0x01b41c, 1); goto lB41C; }
    R[8] = gsu_add(R[8], R[9], 0);
    t = !gsu.cy;
    gsu_code(0x01b418, 4);
    gsu_code(t ? 0x01b413 : 0x01b41c, 1);   /* the delay slot runs after the next fetch */
    gsu_plot_r1();
    if (t) goto lB413;
lB41C:
    R[7] = gsu_sub(R[7], R[9], 0);
    t = gsu.cy;
    R[2] = gsu_inc(R[2]);
    gsu_code(0x01b41d, 5);
    if (t) { gsu_code(0x01b3e9, 1); goto lB3E9; }
    R[4] = gsu_inc(R[4]);
    gsu_code(0x01b422, 5);
    R[0] = gsu_ldw(FX_MODEL_C8);
    R[0] = gsu_sub(R[0], R[4], 0);
    t = !gsu.z;
    gsu_code(0x01b427, 3);
    if (t) { gsu_code(0x01b3e9, 1); goto lB3E9; }
    gsu_code(0x01b42a, 1);
lB42A:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x01b42b, 4);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
    /* FxEffectsAdd */
lB6D5:
    gsu_code(0x01b6d6, 2);
    gsu_stw(R[10], R[11]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[8] = R[0];
    R[6] = R[1];
    R[5] = R[2];
    R[9] = R[3];
    R[0] = 0x0014;
    gsu_code(0x01b6d8, 14);
    gsu_romb(R[0]);
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_code(0x01b6e6, 3);
    gsu_ramb(R[0]);
    R[3] = 0x273E;
    gsu_code(0x01b6e9, 6);
    R[2] = gsu_ldw(FX_EFFECT_SEED);
    R[12] = 0x0014;
    /* CACHE */
    R[13] = 0xB6F4;
    gsu_code(0x01b6ef, 3); gsu_cache(0xb6f2); gsu_code(0x01b6f2, 3);
lB6F4:
    gsu_code(0x01b6f5, 2);
    R[0] = gsu_ldb(R[3]);
    R[0] = gsu_lob(R[0]);
    t = !gsu.z;
    gsu_code(0x01b6f7, 3);
    if (t) { gsu_code(0x01b767, 1); goto lB767; }
    R[0] = gsu_sub(R[0], R[0], 0);
    R[7] = 0x0006;
    R[7] = gsu_add(R[7], R[3], 0);
    gsu_code(0x01b6fa, 8);
    gsu_stw(R[7], R[0]);
    R[7] = 0x0008;
    R[7] = gsu_add(R[7], R[3], 0);
    gsu_code(0x01b702, 6);
    gsu_stw(R[7], R[0]);
    R[7] = 0x000A;
    R[7] = gsu_add(R[7], R[3], 0);
    gsu_code(0x01b708, 6);
    gsu_stw(R[7], R[0]);
    R[7] = 0x000C;
    R[7] = gsu_add(R[7], R[3], 0);
    gsu_code(0x01b70e, 6);
    gsu_stw(R[7], R[5]);
    gsu_code(0x01b714, 3);
    gsu_stb(R[3], (uint8_t)R[6]);
    R[0] = 0x0001;
    gsu_sub(R[0], R[8], 0);   /* CMP */
    t = !gsu.z;
    gsu_code(0x01b717, 6);
    if (t) { gsu_code(0x01b721, 1); goto lB721; }
    gsu_code(0x01b71d, 4); gsu_code(0x01b99f, 1);
    goto lB99F;
lB721:
    R[0] = 0x0002;
    gsu_sub(R[0], R[8], 0);   /* CMP */
    t = !gsu.z;
    gsu_code(0x01b722, 6);
    if (t) { gsu_code(0x01b72c, 1); goto lB72C; }
    gsu_code(0x01b728, 4); gsu_code(0x01b8c3, 1);
    goto lB8C3;
lB72C:
    R[0] = 0x0003;
    gsu_sub(R[0], R[8], 0);   /* CMP */
    t = !gsu.z;
    gsu_code(0x01b72d, 6);
    if (t) { gsu_code(0x01b737, 1); goto lB737; }
    gsu_code(0x01b733, 4); gsu_code(0x01b856, 1);
    goto lB856;
lB737:
    R[0] = 0x0004;
    gsu_sub(R[0], R[8], 0);   /* CMP */
    t = !gsu.z;
    gsu_code(0x01b738, 6);
    if (t) { gsu_code(0x01b742, 1); goto lB742; }
    gsu_code(0x01b73e, 4); gsu_code(0x01b92f, 1);
    goto lB92F;
lB742:
    R[0] = 0x0005;
    gsu_sub(R[0], R[8], 0);   /* CMP */
    t = !gsu.z;
    gsu_code(0x01b743, 6);
    if (t) { gsu_code(0x01b74d, 1); goto lB74D; }
    gsu_code(0x01b749, 4); gsu_code(0x01b99b, 1);
    goto lB99B;
lB74D:
    R[0] = 0x0006;
    gsu_sub(R[0], R[8], 0);   /* CMP */
    t = !gsu.z;
    gsu_code(0x01b74e, 6);
    if (t) { gsu_code(0x01b758, 1); goto lB758; }
    gsu_code(0x01b754, 4); gsu_code(0x01b774, 1);
    goto lB774;
lB758:
    R[0] = 0x0007;
    gsu_sub(R[0], R[8], 0);   /* CMP */
    t = !gsu.z;
    gsu_code(0x01b759, 6);
    if (t) { gsu_code(0x01b763, 1); goto lB763; }
    gsu_code(0x01b75f, 4); gsu_code(0x01b9a3, 1);
    goto lB9A3;
lB763:
    R[9] = gsu_dec(R[9]);
    t = gsu.z;
    gsu_code(0x01b764, 3);
    if (t) { gsu_code(0x01b76c, 1); goto lB76C; }
    gsu_code(0x01b767, 1);
lB767:
    R[3] = gsu_add(R[3], 14, 0);
    t = gsu_count_down(&R[12]);
    gsu_code(0x01b768, 4);
    if (t) {
        at = R[13];   /* FxEffectsAdd's slot loop, 01:B6F4 */
        goto dispatch;
    }
    gsu_code(0x01b76c, 1);
lB76C:
    gsu_code(0x01b76d, 3);
    gsu_stw(FX_EFFECT_SEED, R[2]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x01b770, 4);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
lB774:
    R[0] = gsu_and(R[9], 15);
    R[0] = gsu_add(R[6], R[0], 0);
    R[0] = gsu_sub(R[0], 7, 0);
    R[7] = 0x0000;
    R[7] = gsu_add(R[7], R[3], 0);
    gsu_code(0x01b775, 14);
    gsu_stb(R[7], (uint8_t)R[0]);
    R[0] = 0x0010;
    R[7] = 0x0001;
    R[7] = gsu_add(R[7], R[3], 0);
    gsu_code(0x01b783, 9);
    gsu_stb(R[7], (uint8_t)R[0]);
    R[0] = R[9];
    R[0] = gsu_dec(R[0]);
    R[0] = gsu_add(R[0], R[0], 0);
    R[7] = 0xD0DC;
    gsu_code(0x01b78c, 9);
    gsu_r14(gsu_add(R[0], R[7], 0));
    gsu_code(0x01b795, 2);
    R[0] = gsu_getbs();
    gsu_code(0x01b797, 1);
    gsu_r14(gsu_inc(R[14]));
    R[0] = gsu_div2(R[0]);
    R[7] = 0x0003;
    R[7] = gsu_add(R[7], R[3], 0);
    gsu_code(0x01b798, 9);
    gsu_stb(R[7], (uint8_t)R[0]);
    R[7] = 0x0006;
    R[7] = gsu_add(R[7], R[3], 0);
    gsu_code(0x01b7a1, 6);
    gsu_stw(R[7], R[0]);
    gsu_code(0x01b7a7, 2);
    R[0] = gsu_getbs();
    R[0] = gsu_div2(R[0]);
    R[7] = 0x0004;
    R[7] = gsu_add(R[7], R[3], 0);
    gsu_code(0x01b7a9, 9);
    gsu_stb(R[7], (uint8_t)R[0]);
    R[7] = 0x0008;
    R[7] = gsu_add(R[7], R[3], 0);
    gsu_code(0x01b7b2, 6);
    gsu_stw(R[7], R[0]);
    R[0] = gsu_swap(R[2]);
    R[0] = gsu_ror(R[0]);
    R[0] = gsu_add(R[0], R[2], 0);
    R[2] = gsu_add(R[0], R[2], gsu.cy);
    R[0] = 0x001F;
    R[0] = gsu_and(R[0], R[2]);
    R[0] = gsu_sub(R[0], 15, 0);
    R[7] = 0x0005;
    R[7] = gsu_add(R[7], R[3], 0);
    gsu_code(0x01b7b8, 20);
    gsu_stb(R[7], (uint8_t)R[0]);
    R[7] = 0x000A;
    R[7] = gsu_add(R[7], R[3], 0);
    gsu_code(0x01b7cc, 6);
    gsu_stw(R[7], R[0]);
    R[4] = 0x0004;
    R[0] = 0x0001;
    R[0] = gsu_and(R[0], R[12]);
    t = gsu.z;
    gsu_code(0x01b7d2, 7);
    if (t) { gsu_code(0x01b7db, 1); goto lB7DB; }
    R[4] = 0x000E;
    gsu_code(0x01b7d9, 3);
lB7DB:
    R[7] = 0x0002;
    R[7] = gsu_add(R[7], R[3], 0);
    gsu_code(0x01b7dc, 7);
    gsu_stb(R[7], (uint8_t)R[4]);
    gsu_code(0x01b7e3, 3); gsu_code(0x01b763, 1);
    goto lB763;
    /* FxEffectsAddUnused */
lB7E6:
    R[0] = gsu_and(R[9], 15);
    R[0] = gsu_add(R[6], R[0], 0);
    R[0] = gsu_sub(R[0], 7, 0);
    R[7] = 0x0000;
    R[7] = gsu_add(R[7], R[3], 0);
    gsu_code(0x01b7e7, 14);
    gsu_stb(R[7], (uint8_t)R[0]);
    R[0] = 0x0010;
    R[7] = 0x0001;
    R[7] = gsu_add(R[7], R[3], 0);
    gsu_code(0x01b7f5, 9);
    gsu_stb(R[7], (uint8_t)R[0]);
    R[0] = R[9];
    R[0] = gsu_dec(R[0]);
    R[0] = gsu_add(R[0], R[0], 0);
    R[7] = 0xD0DC;
    gsu_code(0x01b7fe, 9);
    gsu_r14(gsu_add(R[0], R[7], 0));
    gsu_code(0x01b807, 2);
    R[0] = gsu_getbs();
    gsu_code(0x01b809, 1);
    gsu_r14(gsu_inc(R[14]));
    R[7] = 0x0003;
    R[7] = gsu_add(R[7], R[3], 0);
    gsu_code(0x01b80a, 7);
    gsu_stb(R[7], (uint8_t)R[0]);
    R[7] = 0x0006;
    R[7] = gsu_add(R[7], R[3], 0);
    gsu_code(0x01b811, 6);
    gsu_stw(R[7], R[0]);
    gsu_code(0x01b817, 2);
    R[0] = gsu_getbs();
    R[7] = 0x0004;
    R[7] = gsu_add(R[7], R[3], 0);
    gsu_code(0x01b819, 7);
    gsu_stb(R[7], (uint8_t)R[0]);
    R[7] = 0x0008;
    R[7] = gsu_add(R[7], R[3], 0);
    gsu_code(0x01b820, 6);
    gsu_stw(R[7], R[0]);
    R[0] = gsu_swap(R[2]);
    R[0] = gsu_ror(R[0]);
    R[0] = gsu_add(R[0], R[2], 0);
    R[2] = gsu_add(R[0], R[2], gsu.cy);
    R[0] = 0x003F;
    R[0] = gsu_and(R[0], R[2]);
    R[7] = 0x001F;
    R[0] = gsu_sub(R[0], R[7], 0);
    R[7] = 0x0005;
    R[7] = gsu_add(R[7], R[3], 0);
    gsu_code(0x01b826, 22);
    gsu_stb(R[7], (uint8_t)R[0]);
    R[7] = 0x000A;
    R[7] = gsu_add(R[7], R[3], 0);
    gsu_code(0x01b83c, 6);
    gsu_stw(R[7], R[0]);
    R[4] = 0x0004;
    R[0] = 0x0001;
    R[0] = gsu_and(R[0], R[12]);
    t = gsu.z;
    gsu_code(0x01b842, 7);
    if (t) { gsu_code(0x01b84b, 1); goto lB84B; }
    R[4] = 0x000E;
    gsu_code(0x01b849, 3);
lB84B:
    R[7] = 0x0002;
    R[7] = gsu_add(R[7], R[3], 0);
    gsu_code(0x01b84c, 7);
    gsu_stb(R[7], (uint8_t)R[4]);
    gsu_code(0x01b853, 3); gsu_code(0x01b763, 1);
    goto lB763;
    /* FxEffectsAddKinds */
lB856:
    R[0] = gsu_swap(R[2]);
    R[0] = gsu_ror(R[0]);
    R[0] = gsu_add(R[0], R[2], 0);
    R[2] = gsu_add(R[0], R[2], gsu.cy);
    R[7] = 0x0007;
    R[0] = gsu_and(R[0], R[7]);
    R[0] = gsu_sub(R[6], R[0], 0);
    gsu_code(0x01b857, 14);
    gsu_stb(R[3], (uint8_t)R[0]);
    R[0] = 0x0000;
    R[7] = 0x0001;
    R[7] = gsu_add(R[7], R[3], 0);
    gsu_code(0x01b865, 9);
    gsu_stb(R[7], (uint8_t)R[0]);
    R[4] = 0x0000;
    R[0] = gsu_swap(R[2]);
    R[0] = gsu_ror(R[0]);
    R[0] = gsu_add(R[0], R[2], 0);
    R[2] = gsu_add(R[0], R[2], gsu.cy);
    R[0] = 0x000F;
    R[0] = gsu_and(R[0], R[2]);
    R[7] = 0x0007;
    R[0] = gsu_sub(R[0], R[7], 0);
    R[7] = 0x0003;
    R[7] = gsu_add(R[7], R[3], 0);
    gsu_code(0x01b86e, 22);
    gsu_stb(R[7], (uint8_t)R[0]);
    R[0] = gsu_swap(R[2]);
    R[0] = gsu_ror(R[0]);
    R[0] = gsu_add(R[0], R[2], 0);
    R[2] = gsu_add(R[0], R[2], gsu.cy);
    R[0] = 0x000F;
    R[0] = gsu_and(R[0], R[2]);
    R[7] = 0x0007;
    R[0] = gsu_sub(R[0], R[7], 0);
    R[7] = 0x0004;
    R[7] = gsu_add(R[7], R[3], 0);
    gsu_code(0x01b884, 20);
    gsu_stb(R[7], (uint8_t)R[0]);
    R[0] = gsu_swap(R[2]);
    R[0] = gsu_ror(R[0]);
    R[0] = gsu_add(R[0], R[2], 0);
    R[2] = gsu_add(R[0], R[2], gsu.cy);
    R[0] = 0x007F;
    R[0] = gsu_and(R[0], R[2]);
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
    R[7] = 0x0005;
    R[7] = gsu_add(R[7], R[3], 0);
    gsu_code(0x01b898, 19);
    gsu_stb(R[7], (uint8_t)R[0]);
    R[0] = gsu_swap(R[2]);
    R[0] = gsu_ror(R[0]);
    R[0] = gsu_add(R[0], R[2], 0);
    R[2] = gsu_add(R[0], R[2], gsu.cy);
    R[0] = 0x0003;
    R[0] = gsu_and(R[0], R[2]);
    R[7] = 0x0083;
    R[0] = gsu_add(R[0], R[7], 0);
    R[7] = 0x0002;
    R[7] = gsu_add(R[7], R[3], 0);
    gsu_code(0x01b8ab, 21);
    gsu_stb(R[7], (uint8_t)R[0]);
    gsu_code(0x01b8c0, 3); gsu_code(0x01b763, 1);
    goto lB763;
lB8C3:
    R[0] = gsu_swap(R[2]);
    R[0] = gsu_ror(R[0]);
    R[0] = gsu_add(R[0], R[2], 0);
    R[2] = gsu_add(R[0], R[2], gsu.cy);
    R[7] = 0x0007;
    R[0] = gsu_and(R[0], R[7]);
    R[0] = gsu_sub(R[6], R[0], 0);
    gsu_code(0x01b8c4, 14);
    gsu_stb(R[3], (uint8_t)R[0]);
    R[0] = 0x0002;
    R[7] = 0x0001;
    R[7] = gsu_add(R[7], R[3], 0);
    gsu_code(0x01b8d2, 9);
    gsu_stb(R[7], (uint8_t)R[0]);
    R[4] = 0x0000;
    R[0] = gsu_swap(R[2]);
    R[0] = gsu_ror(R[0]);
    R[0] = gsu_add(R[0], R[2], 0);
    R[2] = gsu_add(R[0], R[2], gsu.cy);
    R[0] = 0x000F;
    R[0] = gsu_and(R[0], R[2]);
    R[7] = 0x0007;
    R[0] = gsu_sub(R[0], R[7], 0);
    R[7] = 0x0003;
    R[7] = gsu_add(R[7], R[3], 0);
    gsu_code(0x01b8db, 22);
    gsu_stb(R[7], (uint8_t)R[0]);
    R[0] = gsu_swap(R[2]);
    R[0] = gsu_ror(R[0]);
    R[0] = gsu_add(R[0], R[2], 0);
    R[2] = gsu_add(R[0], R[2], gsu.cy);
    R[0] = 0x0007;
    R[0] = gsu_and(R[0], R[2]);
    R[7] = 0x0003;
    R[0] = gsu_sub(R[0], R[7], 0);
    R[7] = 0x0004;
    R[7] = gsu_add(R[7], R[3], 0);
    gsu_code(0x01b8f1, 20);
    gsu_stb(R[7], (uint8_t)R[0]);
    R[0] = gsu_swap(R[2]);
    R[0] = gsu_ror(R[0]);
    R[0] = gsu_add(R[0], R[2], 0);
    R[2] = gsu_add(R[0], R[2], gsu.cy);
    R[0] = 0x001F;
    R[0] = gsu_and(R[0], R[2]);
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
    R[7] = 0x0005;
    R[7] = gsu_add(R[7], R[3], 0);
    gsu_code(0x01b905, 19);
    gsu_stb(R[7], (uint8_t)R[0]);
    R[0] = gsu_swap(R[2]);
    R[0] = gsu_ror(R[0]);
    R[0] = gsu_add(R[0], R[2], 0);
    R[2] = gsu_add(R[0], R[2], gsu.cy);
    R[0] = 0x0003;
    R[0] = gsu_and(R[0], R[2]);
    R[7] = 0x0068;
    R[0] = gsu_add(R[0], R[7], 0);
    R[7] = 0x0002;
    R[7] = gsu_add(R[7], R[3], 0);
    gsu_code(0x01b918, 20);
    gsu_stb(R[7], (uint8_t)R[0]);
    gsu_code(0x01b92c, 3); gsu_code(0x01b763, 1);
    goto lB763;
lB92F:
    R[0] = gsu_swap(R[2]);
    R[0] = gsu_ror(R[0]);
    R[0] = gsu_add(R[0], R[2], 0);
    R[2] = gsu_add(R[0], R[2], gsu.cy);
    R[7] = 0x0007;
    R[0] = gsu_and(R[0], R[7]);
    R[0] = gsu_sub(R[6], R[0], 0);
    gsu_code(0x01b930, 14);
    gsu_stb(R[3], (uint8_t)R[0]);
    R[0] = 0x0004;
    R[7] = 0x0001;
    R[7] = gsu_add(R[7], R[3], 0);
    gsu_code(0x01b93e, 9);
    gsu_stb(R[7], (uint8_t)R[0]);
    R[4] = 0x0000;
    R[0] = gsu_swap(R[2]);
    R[0] = gsu_ror(R[0]);
    R[0] = gsu_add(R[0], R[2], 0);
    R[2] = gsu_add(R[0], R[2], gsu.cy);
    R[0] = 0x000F;
    R[0] = gsu_and(R[0], R[2]);
    R[7] = 0x0007;
    R[0] = gsu_sub(R[0], R[7], 0);
    R[7] = 0x0003;
    R[7] = gsu_add(R[7], R[3], 0);
    gsu_code(0x01b947, 22);
    gsu_stb(R[7], (uint8_t)R[0]);
    R[0] = gsu_swap(R[2]);
    R[0] = gsu_ror(R[0]);
    R[0] = gsu_add(R[0], R[2], 0);
    R[2] = gsu_add(R[0], R[2], gsu.cy);
    R[0] = 0x0007;
    R[0] = gsu_and(R[0], R[2]);
    R[7] = 0x0003;
    R[0] = gsu_sub(R[0], R[7], 0);
    R[7] = 0x0004;
    R[7] = gsu_add(R[7], R[3], 0);
    gsu_code(0x01b95d, 20);
    gsu_stb(R[7], (uint8_t)R[0]);
    R[0] = gsu_swap(R[2]);
    R[0] = gsu_ror(R[0]);
    R[0] = gsu_add(R[0], R[2], 0);
    R[2] = gsu_add(R[0], R[2], gsu.cy);
    R[0] = 0x001F;
    R[0] = gsu_and(R[0], R[2]);
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
    R[7] = 0x0005;
    R[7] = gsu_add(R[7], R[3], 0);
    gsu_code(0x01b971, 19);
    gsu_stb(R[7], (uint8_t)R[0]);
    R[0] = gsu_swap(R[2]);
    R[0] = gsu_ror(R[0]);
    R[0] = gsu_add(R[0], R[2], 0);
    R[2] = gsu_add(R[0], R[2], gsu.cy);
    R[0] = 0x0003;
    R[0] = gsu_and(R[0], R[2]);
    R[7] = 0x0068;
    R[0] = gsu_add(R[0], R[7], 0);
    R[7] = 0x0002;
    R[7] = gsu_add(R[7], R[3], 0);
    gsu_code(0x01b984, 20);
    gsu_stb(R[7], (uint8_t)R[0]);
    gsu_code(0x01b998, 3); gsu_code(0x01b763, 1);
    goto lB763;
lB99B:
    gsu_code(0x01b99c, 3); gsu_code(0x01b763, 1);
    goto lB763;
lB99F:
    gsu_code(0x01b9a0, 3); gsu_code(0x01b763, 1);
    goto lB763;
lB9A3:
    R[0] = gsu_and(R[9], 15);
    R[0] = gsu_add(R[6], R[0], 0);
    R[0] = gsu_sub(R[0], 7, 0);
    R[7] = 0x0000;
    R[7] = gsu_add(R[7], R[3], 0);
    gsu_code(0x01b9a4, 14);
    gsu_stb(R[7], (uint8_t)R[0]);
    R[0] = 0x0010;
    R[7] = 0x0001;
    R[7] = gsu_add(R[7], R[3], 0);
    gsu_code(0x01b9b2, 9);
    gsu_stb(R[7], (uint8_t)R[0]);
    R[0] = R[9];
    R[0] = gsu_dec(R[0]);
    R[0] = gsu_add(R[0], R[0], 0);
    R[7] = 0xD0DC;
    gsu_code(0x01b9bb, 9);
    gsu_r14(gsu_add(R[0], R[7], 0));
    gsu_code(0x01b9c4, 2);
    R[0] = gsu_getbs();
    gsu_code(0x01b9c6, 1);
    gsu_r14(gsu_inc(R[14]));
    R[7] = 0x0003;
    R[7] = gsu_add(R[7], R[3], 0);
    gsu_code(0x01b9c7, 7);
    gsu_stb(R[7], (uint8_t)R[0]);
    R[7] = 0x0006;
    R[7] = gsu_add(R[7], R[3], 0);
    gsu_code(0x01b9ce, 6);
    gsu_stw(R[7], R[0]);
    gsu_code(0x01b9d4, 2);
    R[0] = gsu_getbs();
    R[7] = 0x0004;
    R[7] = gsu_add(R[7], R[3], 0);
    gsu_code(0x01b9d6, 7);
    gsu_stb(R[7], (uint8_t)R[0]);
    R[7] = 0x0008;
    R[7] = gsu_add(R[7], R[3], 0);
    gsu_code(0x01b9dd, 6);
    gsu_stw(R[7], R[0]);
    R[0] = gsu_swap(R[2]);
    R[0] = gsu_ror(R[0]);
    R[0] = gsu_add(R[0], R[2], 0);
    R[2] = gsu_add(R[0], R[2], gsu.cy);
    R[0] = 0x003F;
    R[0] = gsu_and(R[0], R[2]);
    R[7] = 0x001F;
    R[0] = gsu_sub(R[0], R[7], 0);
    R[7] = 0x0005;
    R[7] = gsu_add(R[7], R[3], 0);
    gsu_code(0x01b9e3, 22);
    gsu_stb(R[7], (uint8_t)R[0]);
    R[7] = 0x000A;
    R[7] = gsu_add(R[7], R[3], 0);
    gsu_code(0x01b9f9, 6);
    gsu_stw(R[7], R[0]);
    R[4] = 0x0004;
    R[0] = 0x0001;
    R[0] = gsu_and(R[0], R[12]);
    t = gsu.z;
    gsu_code(0x01b9ff, 7);
    if (t) { gsu_code(0x01ba08, 1); goto lBA08; }
    R[4] = 0x000E;
    gsu_code(0x01ba06, 3);
lBA08:
    R[7] = 0x0002;
    R[7] = gsu_add(R[7], R[3], 0);
    gsu_code(0x01ba09, 7);
    gsu_stb(R[7], (uint8_t)R[4]);
    gsu_code(0x01ba10, 3); gsu_code(0x01b763, 1);
    goto lB763;
    /* FxWeatherReset */
lBC35:
    gsu_code(0x01bc36, 4);
    gsu_stw(FX_INIT_2B94, R[0]);
    gsu_code(0x01bc3a, 2);
    gsu_stw(R[10], R[11]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[0] = gsu_sub(R[0], R[0], 0);
    R[2] = 0x0050;
    R[1] = 0x2864;
    R[3] = 0x0000;
    R[4] = 0x003F;
    /* CACHE */
    gsu_code(0x01bc3c, 14); gsu_cache(0xbc4a); gsu_code(0x01bc4a, 1);
lBC4A:
    R[0] = R[3];
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_code(0x01bc4b, 6);
    gsu_stb(R[1], (uint8_t)R[0]);
    R[1] = gsu_inc(R[1]);
    R[3] = gsu_inc(R[3]);
    R[3] = gsu_and(R[3], R[4]);
    R[0] = gsu_sub(R[0], R[0], 0);
    R[12] = 0x0009;
    R[13] = 0xBC5A;
    gsu_code(0x01bc51, 10);
lBC5A:
    gsu_code(0x01bc5b, 2);
    gsu_stb(R[1], (uint8_t)R[0]);
    t = gsu_count_down(&R[12]);
    R[1] = gsu_inc(R[1]);
    gsu_code(0x01bc5d, 1);
    if (t) { gsu_code(0x01bc5a, 1); goto lBC5A; }
    R[2] = gsu_dec(R[2]);
    t = !gsu.z;
    gsu_code(0x01bc5e, 4);
    if (t) { gsu_code(0x01bc4a, 1); goto lBC4A; }
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_code(0x01bc62, 6);
    gsu_stw(FX_WEATHER_TURN, R[0]);
    gsu_code(0x01bc68, 4);
    gsu_stw(FX_WEATHER_DRIFT, R[0]);
    R[1] = 0x2688;
    R[12] = 0x0030;
    R[13] = 0xBC73;
    gsu_code(0x01bc6c, 8);
lBC73:
    gsu_code(0x01bc74, 1);
    gsu_stw(R[1], R[0]);
    R[1] = gsu_inc(R[1]);
    R[1] = gsu_inc(R[1]);
    t = gsu_count_down(&R[12]);
    gsu_code(0x01bc75, 3);
    if (t) { gsu_code(0x01bc73, 1); goto lBC73; }
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x01bc78, 5);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
    /* FxWeatherDraw */
lBC7D:
    gsu_code(0x01bc7e, 4);
    R[3] = gsu_ldw(FX_INIT_2B94);
    gsu_sub(R[0], R[3], 0);   /* CMP */
    t = !gsu.z;
    gsu_code(0x01bc82, 4);
    if (t) { gsu_code(0x01bc35, 1); goto lBC35; }
    R[3] = 0x0001;
    gsu_sub(R[0], R[3], 0);   /* CMP */
    t = gsu.z;
    gsu_code(0x01bc86, 7);
    if (t) { gsu_code(0x01bc97, 1); goto lBC97; }
    R[3] = 0x0002;
    gsu_sub(R[0], R[3], 0);   /* CMP */
    t = gsu.z;
    gsu_code(0x01bc8d, 7);
    if (t) { gsu_code(0x01bc97, 1); goto lBC97; }
    nx = (uint16_t)(R[11]);
    at = nx;
    gsu_code(0x01bc94, 3);
    goto dispatch;
lBC97:
    gsu_code(0x01bc98, 2);
    gsu_stw(R[10], R[11]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_code(0x01bc9a, 5);
    R[7] = gsu_ldw(FX_EFFECT_SEED);
    R[12] = R[1];
    gsu_moves(R[1]);
    t = !gsu.z;
    gsu_code(0x01bc9f, 4);
    if (t) { gsu_code(0x01bca7, 1); goto lBCA7; }
    gsu_code(0x01bca3, 4); gsu_code(0x01bf65, 1);
    goto lBF65;
lBCA7:
    R[11] = R[0];
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_code(0x01bca8, 5);
    gsu_ramb(R[0]);
    R[0] = 0x0014;
    gsu_code(0x01bcad, 4);
    gsu_romb(R[0]);
    R[0] = 0x0001;
    gsu_cmode(R[0]);
    R[0] = gsu_sub(R[11], 1, 0);
    t = gsu.z;
    gsu_code(0x01bcb1, 9);
    if (t) { gsu_code(0x01bcd2, 1); goto lBCD2; }
    R[3] = 0x001F;
    R[4] = 0x000F;
    R[5] = 0x0007;
    R[6] = 0x0005;
    R[0] = 0x0020;
    gsu_code(0x01bcba, 16);
    gsu_stw(FX_WEATHER_COLOUR, R[0]);
    R[0] = 0x0064;
    gsu_code(0x01bcca, 6);
    gsu_stw(FX_WEATHER_LIFE, R[0]);
    t = 1;
    gsu_code(0x01bcd0, 2);
    gsu_code(0x01bce7, 1);
    if (t) goto lBCE7;
lBCD2:
    R[3] = 0x001F;
    R[4] = 0x003F;
    R[5] = 0x001F;
    R[6] = 0x0028;
    R[0] = 0x002A;
    gsu_code(0x01bcd3, 15);
    gsu_stw(FX_WEATHER_COLOUR, R[0]);
    R[0] = 0x001E;
    gsu_code(0x01bce2, 6);
    gsu_stw(FX_WEATHER_LIFE, R[0]);
lBCE7:
    gsu_code(0x01bce8, 4);
    R[2] = gsu_ldw(FX_WEATHER_DRIFT);
    R[0] = gsu_swap(R[7]);
    R[0] = gsu_ror(R[0]);
    R[0] = gsu_add(R[0], R[7], 0);
    R[7] = gsu_add(R[0], R[7], gsu.cy);
    R[1] = gsu_and(R[0], R[3]);
    t = !gsu.z;
    gsu_code(0x01bcec, 11);
    if (t) { gsu_code(0x01bcfb, 1); goto lBCFB; }
    R[0] = gsu_hib(R[0]);
    R[0] = gsu_and(R[0], R[4]);
    R[2] = gsu_sub(R[0], R[5], 0);
    gsu_code(0x01bcf7, 5);
lBCFB:
    gsu_code(0x01bcfc, 4);
    R[1] = gsu_ldw(FX_WEATHER_DRIFT);
    R[0] = gsu_sub(R[2], R[1], 0);
    R[0] = gsu_div2(R[0]);
    R[0] = gsu_div2(R[0]);
    R[0] = gsu_div2(R[0]);
    R[0] = gsu_add(R[0], R[1], 0);
    gsu_code(0x01bd00, 10);
    gsu_sbk(R[0]);
    gsu_code(0x01bd0a, 4);
    R[0] = gsu_ldw(GSU_PARAM_00E0);
    gsu_code(0x01bd0e, 4);
    R[1] = gsu_ldw(FX_WEATHER_TURN);
    gsu_code(0x01bd12, 4);
    gsu_stw(FX_WEATHER_TURN, R[0]);
    R[0] = gsu_sub(R[0], R[1], 0);
    R[0] = gsu_div2(R[0]);
    R[0] = gsu_div2(R[0]);
    R[0] = gsu_div2(R[0]);
    R[0] = gsu_div2(R[0]);
    R[0] = gsu_div2(R[0]);
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_code(0x01bd16, 17);
    R[1] = gsu_ldw(FX_WEATHER_DRIFT);
    R[0] = gsu_add(R[0], R[1], 0);
    gsu_code(0x01bd27, 5);
    gsu_stw(FX_CLOSING, R[0]);
    gsu_code(0x01bd2c, 4);
    gsu_stw(FX_HAZE_X8, R[6]);
    gsu_code(0x01bd30, 4);
    R[1] = gsu_ldw(GSU_TIED_22);
    R[0] = 0x004A;
    R[0] = gsu_add(R[0], R[1], 0);
    gsu_code(0x01bd34, 4);
    R[0] = gsu_ldw(R[0]);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = !gsu.s;
    gsu_code(0x01bd38, 4);
    if (t) { gsu_code(0x01bd40, 1); goto lBD40; }
    gsu_code(0x01bd3c, 4); gsu_code(0x01bf65, 1);
    goto lBF65;
lBD40:
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
    R[0] = gsu_div2(R[0]);
    R[0] = gsu_div2(R[0]);
    R[0] = gsu_div2(R[0]);
    gsu_code(0x01bd41, 12);
    gsu_stw(FX_ME_Z, R[0]);
    R[8] = 0x2864;
    /* CACHE */
    R[13] = 0xBD52;
    gsu_code(0x01bd4d, 3); gsu_cache(0xbd50); gsu_code(0x01bd50, 3);
lBD52:
    gsu_code(0x01bd53, 2);
    R[0] = gsu_ldb(R[8]);
    R[0] = gsu_sex(R[0]);
    t = gsu.z;
    R[0] = gsu_dec(R[0]);
    gsu_code(0x01bd55, 3);
    if (t) { gsu_code(0x01bd66, 1); goto lBD66; }
    t = !gsu.s;
    gsu_code(0x01bd58, 3);
    if (t) { gsu_code(0x01bdaa, 1); goto lBDAA; }
    R[0] = gsu_inc(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_code(0x01bd5b, 5);
    gsu_stb(R[8], (uint8_t)R[0]);
    R[8] = gsu_add(R[8], 10, 0);
    gsu_code(0x01bd60, 6); gsu_code(0x01bf2a, 1);
    goto lBF2A;
lBD66:
    gsu_code(0x01bd67, 4);
    R[0] = gsu_ldw(FX_WEATHER_LIFE);
    gsu_code(0x01bd6b, 2);
    gsu_stb(R[8], (uint8_t)R[0]);
    R[8] = gsu_inc(R[8]);
    R[0] = gsu_swap(R[7]);
    R[0] = gsu_ror(R[0]);
    R[0] = gsu_add(R[0], R[7], 0);
    R[7] = gsu_add(R[0], R[7], gsu.cy);
    gsu_code(0x01bd6d, 12);
    R[0] = gsu_ldw(FX_WEATHER_COLOUR);
    gsu_code(0x01bd79, 2);
    gsu_stb(R[8], (uint8_t)R[0]);
    R[8] = gsu_inc(R[8]);
    R[3] = 0x01FF;
    R[3] = gsu_and(R[0], R[3]);
    R[0] = gsu_swap(R[7]);
    R[0] = gsu_ror(R[0]);
    R[0] = gsu_add(R[0], R[7], 0);
    R[7] = gsu_add(R[0], R[7], gsu.cy);
    R[1] = 0x03FF;
    R[0] = gsu_and(R[0], R[1]);
    R[1] = 0x01FF;
    R[0] = gsu_sub(R[0], R[1], 0);
    gsu_code(0x01bd7b, 22);
    gsu_stw(R[8], R[0]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    gsu_code(0x01bd91, 5);
    R[1] = gsu_ldw(FX_MODEL_36);
    R[0] = gsu_div2(R[3]);
    R[0] = gsu_add(R[0], R[1], 0);
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_code(0x01bd96, 7);
    gsu_stw(R[8], R[0]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    gsu_code(0x01bd9d, 4);
    gsu_stw(R[8], R[3]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_code(0x01bda1, 4);
    gsu_stw(R[8], R[0]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    gsu_code(0x01bda5, 5); gsu_code(0x01bf2a, 1);
    goto lBF2A;
lBDAA:
    gsu_code(0x01bdab, 2);
    gsu_stb(R[8], (uint8_t)R[0]);
    R[8] = gsu_inc(R[8]);
    gsu_code(0x01bdad, 3);
    R[0] = gsu_ldb(R[8]);
    R[8] = gsu_inc(R[8]);
    gsu_color((uint8_t)R[0]);
    R[6] = 0x01FF;
    gsu_code(0x01bdb0, 9);
    R[1] = gsu_ldw(FX_CLOSING);
    gsu_code(0x01bdb9, 1);
    R[0] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    R[0] = gsu_add(R[0], R[1], 0);
    t = !gsu.s;
    gsu_code(0x01bdba, 5);
    if (t) { gsu_code(0x01bdc9, 1); goto lBDC9; }
    R[0] = gsu_add(R[0], R[6], 0);
    t = !gsu.s;
    R[0] = gsu_sub(R[0], R[6], 0);
    gsu_code(0x01bdbf, 4);
    if (t) { gsu_code(0x01bddd, 1); goto lBDDD; }
    R[0] = R[6];
    R[0] = gsu_dec(R[0]);
    t = 1;
    gsu_code(0x01bdc3, 6);
    gsu_code(0x01bdd1, 1);
    if (t) goto lBDD1;
lBDC9:
    R[0] = gsu_sub(R[0], R[6], 0);
    t = gsu.s;
    R[0] = gsu_add(R[0], R[6], 0);
    gsu_code(0x01bdca, 3);
    if (t) { gsu_code(0x01bddd, 1); goto lBDDD; }
    R[0] = R[6];
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_code(0x01bdcd, 5);
lBDD1:
    gsu_code(0x01bdd2, 1);
    gsu_sbk(R[0]);
    R[8] = gsu_add(R[8], 4, 0);
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_code(0x01bdd3, 5);
    gsu_stw(R[8], R[0]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    gsu_code(0x01bdd8, 5); gsu_code(0x01bf2a, 1);
    goto lBF2A;
lBDDD:
    gsu_code(0x01bdde, 1);
    gsu_sbk(R[0]);
    R[9] = R[0];
    R[6] = 0x00FF;
    gsu_code(0x01bddf, 9);
    R[1] = gsu_ldw(FX_HAZE_X8);
    gsu_code(0x01bde8, 1);
    R[0] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    R[0] = gsu_add(R[0], R[1], 0);
    t = !gsu.s;
    gsu_code(0x01bde9, 5);
    if (t) { gsu_code(0x01bdf8, 1); goto lBDF8; }
    R[0] = gsu_add(R[0], R[6], 0);
    t = !gsu.s;
    R[0] = gsu_sub(R[0], R[6], 0);
    gsu_code(0x01bdee, 4);
    if (t) { gsu_code(0x01be0c, 1); goto lBE0C; }
    R[0] = R[6];
    R[0] = gsu_dec(R[0]);
    t = 1;
    gsu_code(0x01bdf2, 6);
    gsu_code(0x01be00, 1);
    if (t) goto lBE00;
lBDF8:
    R[0] = gsu_sub(R[0], R[6], 0);
    t = gsu.s;
    R[0] = gsu_add(R[0], R[6], 0);
    gsu_code(0x01bdf9, 3);
    if (t) { gsu_code(0x01be0c, 1); goto lBE0C; }
    R[0] = R[6];
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_code(0x01bdfc, 5);
lBE00:
    gsu_code(0x01be01, 1);
    gsu_sbk(R[0]);
    R[8] = gsu_add(R[8], 2, 0);
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_code(0x01be02, 5);
    gsu_stw(R[8], R[0]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    gsu_code(0x01be07, 5); gsu_code(0x01bf2a, 1);
    goto lBF2A;
lBE0C:
    gsu_code(0x01be0d, 1);
    gsu_sbk(R[0]);
    R[5] = R[0];
    R[6] = 0x01FF;
    gsu_code(0x01be0e, 9);
    R[1] = gsu_ldw(FX_ME_Z);
    gsu_code(0x01be17, 1);
    R[0] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    R[0] = gsu_add(R[0], R[1], 0);
    t = !gsu.s;
    gsu_code(0x01be18, 5);
    if (t) { gsu_code(0x01be22, 1); goto lBE22; }
    R[0] = R[6];
    t = 1;
    gsu_code(0x01be1d, 5);
    gsu_code(0x01be28, 1);
    if (t) goto lBE28;
lBE22:
    gsu_sub(R[0], R[6], 0);   /* CMP */
    t = gsu.s;
    gsu_code(0x01be23, 4);
    if (t) { gsu_code(0x01be31, 1); goto lBE31; }
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_code(0x01be27, 2);
lBE28:
    gsu_code(0x01be29, 1);
    gsu_sbk(R[0]);
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_code(0x01be2a, 2);
    gsu_stw(R[8], R[0]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    gsu_code(0x01be2c, 5); gsu_code(0x01bf2a, 1);
    goto lBF2A;
lBE31:
    gsu_code(0x01be32, 1);
    gsu_sbk(R[0]);
    R[4] = R[0];
    R[6] = 0x012C;
    R[6] = gsu_add(R[6], R[4], 0);
    R[6] = gsu_add(R[6], R[6], 0);
    R[0] = 0x896C;
    gsu_code(0x01be33, 14);
    gsu_r14(gsu_add(R[0], R[6], 0));
    gsu_code(0x01be41, 2);
    R[6] = gsu_getb();
    gsu_code(0x01be43, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x01be44, 3);
    R[6] = gsu_getbh(R[6]);
    gsu_code(0x01be47, 2);
    R[9] = gsu_fmult(R[9], R[6]);
    R[9] = gsu_rol(R[9]);
    gsu_code(0x01be49, 5);
    R[1] = gsu_ldw(FX_MODEL_34);
    R[1] = gsu_add(R[1], R[9], 0);
    gsu_code(0x01be4e, 5);
    R[0] = gsu_ldw(FX_SCREEN_LEFT);
    gsu_sub(R[0], R[1], 0);   /* CMP */
    t = gsu.s;
    gsu_code(0x01be53, 4);
    if (t) { gsu_code(0x01be5b, 1); goto lBE5B; }
    gsu_code(0x01be57, 4); gsu_code(0x01bf4d, 1);
    goto lBF4D;
lBE5B:
    gsu_code(0x01be5c, 3);
    R[0] = gsu_ldw(FX_SCREEN_RIGHT);
    R[0] = gsu_sub(R[0], 2, 0);
    gsu_sub(R[0], R[1], 0);   /* CMP */
    t = !gsu.s;
    gsu_code(0x01be5f, 6);
    if (t) { gsu_code(0x01be69, 1); goto lBE69; }
    gsu_code(0x01be65, 4); gsu_code(0x01bf4d, 1);
    goto lBF4D;
lBE69:
    gsu_code(0x01be6a, 2);
    R[5] = gsu_fmult(R[5], R[6]);
    R[5] = gsu_rol(R[5]);
    gsu_code(0x01be6c, 5);
    R[2] = gsu_ldw(FX_MODEL_36);
    R[0] = 0x0014;
    R[2] = gsu_sub(R[2], R[0], 0);
    R[2] = gsu_add(R[2], R[5], 0);
    gsu_code(0x01be71, 10);
    R[0] = gsu_ldw(FX_SCREEN_TOP);
    gsu_sub(R[0], R[2], 0);   /* CMP */
    t = gsu.s;
    gsu_code(0x01be7b, 4);
    if (t) { gsu_code(0x01be83, 1); goto lBE83; }
    gsu_code(0x01be7f, 4); gsu_code(0x01bf4d, 1);
    goto lBF4D;
lBE83:
    gsu_code(0x01be84, 4);
    R[0] = gsu_ldw(FX_SCREEN_BOTTOM);
    R[0] = gsu_sub(R[0], 4, 0);
    gsu_sub(R[0], R[2], 0);   /* CMP */
    t = !gsu.s;
    gsu_code(0x01be88, 6);
    if (t) { gsu_code(0x01be92, 1); goto lBE92; }
    gsu_code(0x01be8e, 4); gsu_code(0x01bf53, 1);
    goto lBF53;
lBE92:
    R[0] = gsu_sub(R[11], 1, 0);
    t = gsu.z;
    gsu_code(0x01be93, 6);
    if (t) { gsu_code(0x01beb2, 1); goto lBEB2; }
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    R[0] = 0x0100;
    gsu_sub(R[0], R[4], 0);   /* CMP */
    t = !gsu.s;
    gsu_code(0x01be99, 10);
    if (t) { gsu_code(0x01bea8, 1); goto lBEA8; }
    gsu_code(0x01bea3, 2);
    gsu_plot_r1();
    gsu_code(0x01bea5, 3); gsu_code(0x01bf2a, 1);
    goto lBF2A;
lBEA8:
    gsu_code(0x01bea9, 1);
    gsu_plot_r1();
    gsu_code(0x01beaa, 1);
    gsu_plot_r1();
    R[2] = gsu_inc(R[2]);
    R[1] = gsu_dec(R[1]);
    R[1] = gsu_dec(R[1]);
    gsu_code(0x01beab, 4);
    gsu_plot_r1();
    gsu_code(0x01beaf, 1);
    gsu_plot_r1();
    t = 1;
    gsu_code(0x01beb0, 2);
    gsu_code(0x01bf2a, 1);
    if (t) goto lBF2A;
lBEB2:
    gsu_code(0x01beb3, 1);
    R[0] = gsu_ldw(R[8]);
    R[3] = gsu_lob(R[0]);
    R[9] = gsu_hib(R[0]);
    gsu_code(0x01beb4, 7);
    gsu_stb(R[8], (uint8_t)R[1]);
    R[8] = gsu_inc(R[8]);
    gsu_code(0x01bebb, 4);
    gsu_stb(R[8], (uint8_t)R[2]);
    R[8] = gsu_inc(R[8]);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.z;
    gsu_code(0x01bebf, 5);
    if (t) { gsu_code(0x01bf2a, 1); goto lBF2A; }
    gsu_code(0x01bec4, 3);
    gsu_stw(R[10], R[12]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_code(0x01bec7, 4);
    gsu_stw(R[10], R[13]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_code(0x01becb, 4);
    gsu_stw(R[10], R[8]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_code(0x01becf, 4);
    gsu_stw(R[10], R[7]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[5] = 0x0001;
    R[3] = gsu_sub(R[3], R[1], 0);
    t = !gsu.s;
    gsu_code(0x01bed3, 8);
    if (t) { gsu_code(0x01bee0, 1); goto lBEE0; }
    R[5] = 0xFFFF;
    R[3] = gsu_not(R[3]);
    R[3] = gsu_inc(R[3]);
    gsu_code(0x01bedb, 6);
lBEE0:
    R[6] = 0x0001;
    R[9] = gsu_sub(R[9], R[2], 0);
    t = !gsu.s;
    gsu_code(0x01bee1, 6);
    if (t) { gsu_code(0x01beec, 1); goto lBEEC; }
    R[6] = 0xFFFF;
    R[9] = gsu_not(R[9]);
    R[9] = gsu_inc(R[9]);
    gsu_code(0x01bee7, 6);
lBEEC:
    R[0] = gsu_sub(R[3], R[9], 0);
    t = gsu.s;
    gsu_code(0x01beed, 4);
    if (t) { gsu_code(0x01bf06, 1); goto lBF06; }
    R[12] = R[3];
    R[12] = gsu_inc(R[12]);
    R[0] = gsu_lsr(R[0]);
    R[0] = gsu_sub(R[0], R[9], 0);
    R[13] = 0xBEF8;
    gsu_code(0x01bef1, 8);
lBEF8:
    t = gsu.cy;
    gsu_code(0x01bef9, 2);
    gsu_code(t ? 0x01befe : 0x01befb, 1);   /* the delay slot runs after the next fetch */
    gsu_plot_r1();
    if (t) goto lBEFE;
    R[0] = gsu_add(R[0], R[3], 0);
    R[2] = gsu_add(R[2], R[6], 0);
    gsu_code(0x01befc, 3);
lBEFE:
    R[1] = gsu_dec(R[1]);
    R[1] = gsu_add(R[1], R[5], 0);
    t = gsu_count_down(&R[12]);
    R[0] = gsu_sub(R[0], R[9], 0);
    gsu_code(0x01beff, 4);
    if (t) { gsu_code(0x01bef8, 1); goto lBEF8; }
    t = 1;
    gsu_code(0x01bf03, 3);
    gsu_code(0x01bf1a, 1);
    if (t) goto lBF1A;
lBF06:
    R[12] = R[9];
    R[12] = gsu_inc(R[12]);
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
    R[0] = gsu_lsr(R[0]);
    R[0] = gsu_sub(R[0], R[3], 0);
    R[13] = 0xBF0F;
    gsu_code(0x01bf07, 9);
lBF0F:
    t = gsu.cy;
    gsu_code(0x01bf10, 2);
    gsu_code(t ? 0x01bf15 : 0x01bf12, 1);   /* the delay slot runs after the next fetch */
    gsu_plot_r1();
    if (t) goto lBF15;
    R[0] = gsu_add(R[0], R[9], 0);
    R[1] = gsu_add(R[1], R[5], 0);
    gsu_code(0x01bf13, 3);
lBF15:
    R[1] = gsu_dec(R[1]);
    R[2] = gsu_add(R[2], R[6], 0);
    t = gsu_count_down(&R[12]);
    R[0] = gsu_sub(R[0], R[3], 0);
    gsu_code(0x01bf16, 4);
    if (t) { gsu_code(0x01bf0f, 1); goto lBF0F; }
    gsu_code(0x01bf1a, 1);
lBF1A:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x01bf1b, 4);
    R[7] = gsu_ldw(R[10]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x01bf1f, 4);
    R[8] = gsu_ldw(R[10]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x01bf23, 4);
    R[13] = gsu_ldw(R[10]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x01bf27, 4);
    R[12] = gsu_ldw(R[10]);
lBF2A:
    t = gsu_count_down(&R[12]);
    gsu_code(0x01bf2b, 1);
    if (t) {
        at = R[13];   /* the drop loop's 01:BD52, pulled back after a streak */
        goto dispatch;
    }
    t = 1;
    gsu_code(0x01bf2c, 3);
    gsu_code(0x01bf65, 1);
    if (t) goto lBF65;
    /* FxWeatherUnused */
lBF2F:
    t = 1;
    gsu_code(0x01bf30, 2);
    gsu_code(0x01bf2a, 1);
    if (t) goto lBF2A;
lBF32:
    t = 1;
    gsu_code(0x01bf33, 2);
    gsu_code(0x01bf2a, 1);
    if (t) goto lBF2A;
lBF35:
    R[2] = 0x0000;
    t = 1;
    gsu_code(0x01bf36, 5);
    gsu_code(0x01bf3e, 1);
    if (t) goto lBF3E;
lBF3B:
    R[2] = 0x01FF;
    gsu_code(0x01bf3c, 3);
lBF3E:
    R[1] = R[8];
    R[8] = gsu_sub(R[8], 10, 0);
    R[8] = gsu_add(R[8], 6, 0);
    gsu_code(0x01bf3f, 10);
    gsu_stw(R[8], R[2]);
    R[8] = R[1];
    t = 1;
    gsu_code(0x01bf49, 4);
    gsu_code(0x01bf2a, 1);
    if (t) goto lBF2A;
    /* FxWeatherDropNext */
lBF4D:
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_code(0x01bf4e, 2);
    gsu_stw(R[8], R[0]);
    R[8] = gsu_inc(R[8]);
    t = 1;
    R[8] = gsu_inc(R[8]);
    gsu_code(0x01bf50, 3);
    gsu_code(0x01bf2a, 1);
    if (t) goto lBF2A;
lBF53:
    R[1] = R[8];
    R[8] = gsu_sub(R[8], 8, 0);
    R[8] = gsu_add(R[8], 0, 0);
    R[0] = 0x0001;
    gsu_code(0x01bf54, 12);
    gsu_stb(R[8], (uint8_t)R[0]);
    R[8] = R[1];
    R[8] = gsu_inc(R[8]);
    t = 1;
    R[8] = gsu_inc(R[8]);
    gsu_code(0x01bf60, 5);
    gsu_code(0x01bf2a, 1);
    if (t) goto lBF2A;
lBF65:
    gsu_code(0x01bf66, 3);
    gsu_stw(FX_EFFECT_SEED, R[7]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x01bf69, 4);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
dispatch:
    if (at == ret) {
        gsu_code(0x010000 | at, 1);
        return;
    }
    gsu_code(0x010000 | at, 1);   /* where it jumps: its first byte */
fetched:
enter:
    switch (at) {
    case 0xAB13: goto lAB13;
    case 0xAB22: goto lAB22;
    case 0xAB2E: goto lAB2E;
    case 0xAB56: goto lAB56;
    case 0xAB5F: goto lAB5F;
    case 0xAB66: goto lAB66;
    case 0xAB69: goto lAB69;
    case 0xAB6E: goto lAB6E;
    case 0xAB73: goto lAB73;
    case 0xAB7D: goto lAB7D;
    case 0xAB81: goto lAB81;
    case 0xAB8C: goto lAB8C;
    case 0xAB90: goto lAB90;
    case 0xABC2: goto lABC2;
    case 0xAC09: goto lAC09;
    case 0xAC25: goto lAC25;
    case 0xAC2D: goto lAC2D;
    case 0xAC31: goto lAC31;
    case 0xAC48: goto lAC48;
    case 0xAC6A: goto lAC6A;
    case 0xAC6D: goto lAC6D;
    case 0xAC99: goto lAC99;
    case 0xACA7: goto lACA7;
    case 0xACB5: goto lACB5;
    case 0xACC0: goto lACC0;
    case 0xAD2D: goto lAD2D;
    case 0xAD31: goto lAD31;
    case 0xAD53: goto lAD53;
    case 0xAD62: goto lAD62;
    case 0xB0D9: goto lB0D9;
    case 0xB0FC: goto lB0FC;
    case 0xB100: goto lB100;
    case 0xB10B: goto lB10B;
    case 0xB118: goto lB118;
    case 0xB123: goto lB123;
    case 0xB128: goto lB128;
    case 0xB137: goto lB137;
    case 0xB147: goto lB147;
    case 0xB14C: goto lB14C;
    case 0xB155: goto lB155;
    case 0xB160: goto lB160;
    case 0xB17D: goto lB17D;
    case 0xB182: goto lB182;
    case 0xB1A6: goto lB1A6;
    case 0xB1AB: goto lB1AB;
    case 0xB1CB: goto lB1CB;
    case 0xB206: goto lB206;
    case 0xB20F: goto lB20F;
    case 0xB21B: goto lB21B;
    case 0xB220: goto lB220;
    case 0xB22A: goto lB22A;
    case 0xB22D: goto lB22D;
    case 0xB23E: goto lB23E;
    case 0xB246: goto lB246;
    case 0xB251: goto lB251;
    case 0xB260: goto lB260;
    case 0xB26C: goto lB26C;
    case 0xB271: goto lB271;
    case 0xB27D: goto lB27D;
    case 0xB280: goto lB280;
    case 0xB291: goto lB291;
    case 0xB299: goto lB299;
    case 0xB2ED: goto lB2ED;
    case 0xB2FC: goto lB2FC;
    case 0xB303: goto lB303;
    case 0xB32A: goto lB32A;
    case 0xB33E: goto lB33E;
    case 0xB35E: goto lB35E;
    case 0xB365: goto lB365;
    case 0xB383: goto lB383;
    case 0xB384: goto lB384;
    case 0xB38A: goto lB38A;
    case 0xB38F: goto lB38F;
    case 0xB398: goto lB398;
    case 0xB3A6: goto lB3A6;
    case 0xB3AB: goto lB3AB;
    case 0xB3C2: goto lB3C2;
    case 0xB3E2: goto lB3E2;
    case 0xB3E9: goto lB3E9;
    case 0xB407: goto lB407;
    case 0xB408: goto lB408;
    case 0xB40E: goto lB40E;
    case 0xB413: goto lB413;
    case 0xB41C: goto lB41C;
    case 0xB42A: goto lB42A;
    case 0xB6D5: goto lB6D5;
    case 0xB6F4: goto lB6F4;
    case 0xB721: goto lB721;
    case 0xB72C: goto lB72C;
    case 0xB737: goto lB737;
    case 0xB742: goto lB742;
    case 0xB74D: goto lB74D;
    case 0xB758: goto lB758;
    case 0xB763: goto lB763;
    case 0xB767: goto lB767;
    case 0xB76C: goto lB76C;
    case 0xB774: goto lB774;
    case 0xB7DB: goto lB7DB;
    case 0xB7E6: goto lB7E6;
    case 0xB84B: goto lB84B;
    case 0xB856: goto lB856;
    case 0xB8C3: goto lB8C3;
    case 0xB92F: goto lB92F;
    case 0xB99B: goto lB99B;
    case 0xB99F: goto lB99F;
    case 0xB9A3: goto lB9A3;
    case 0xBA08: goto lBA08;
    case 0xBC35: goto lBC35;
    case 0xBC4A: goto lBC4A;
    case 0xBC5A: goto lBC5A;
    case 0xBC73: goto lBC73;
    case 0xBC7D: goto lBC7D;
    case 0xBC97: goto lBC97;
    case 0xBCA7: goto lBCA7;
    case 0xBCD2: goto lBCD2;
    case 0xBCE7: goto lBCE7;
    case 0xBCFB: goto lBCFB;
    case 0xBD40: goto lBD40;
    case 0xBD52: goto lBD52;
    case 0xBD66: goto lBD66;
    case 0xBDAA: goto lBDAA;
    case 0xBDC9: goto lBDC9;
    case 0xBDD1: goto lBDD1;
    case 0xBDDD: goto lBDDD;
    case 0xBDF8: goto lBDF8;
    case 0xBE00: goto lBE00;
    case 0xBE0C: goto lBE0C;
    case 0xBE22: goto lBE22;
    case 0xBE28: goto lBE28;
    case 0xBE31: goto lBE31;
    case 0xBE5B: goto lBE5B;
    case 0xBE69: goto lBE69;
    case 0xBE83: goto lBE83;
    case 0xBE92: goto lBE92;
    case 0xBEA8: goto lBEA8;
    case 0xBEB2: goto lBEB2;
    case 0xBEE0: goto lBEE0;
    case 0xBEEC: goto lBEEC;
    case 0xBEF8: goto lBEF8;
    case 0xBEFE: goto lBEFE;
    case 0xBF06: goto lBF06;
    case 0xBF0F: goto lBF0F;
    case 0xBF15: goto lBF15;
    case 0xBF1A: goto lBF1A;
    case 0xBF2A: goto lBF2A;
    case 0xBF2F: goto lBF2F;
    case 0xBF32: goto lBF32;
    case 0xBF35: goto lBF35;
    case 0xBF3B: goto lBF3B;
    case 0xBF3E: goto lBF3E;
    case 0xBF4D: goto lBF4D;
    case 0xBF53: goto lBF53;
    case 0xBF65: goto lBF65;
    }
    return;   /* not an address the code jumps to */

}

/*
 * @routine 01:AB13 FxFrameProgram
 * WHAT IT DOES: A program the S-CPU starts once a frame: clears the
 * screen (FxScreenClear, unless the byte at $01C4 is 0), draws the
 * objects of the sorted draw list (FxObjectsDraw), the weather
 * (FxWeatherDraw, by bits 0 and 1 of the word at +$8A of the record
 * $247A points to; none clears $2B94), the effects (FxEffectsRun) and
 * the text (FxTextNotice), then STOP.
 * HOW: R10 = $0240 before each part; $01B4 = 0; CMODE 0; $26EE = $10
 * for the first weather; RPIX before the STOP writes the pixel caches
 * out.
 * Entry: started by the S-CPU.
 * Exit: (at the STOP) as the last part leaves the registers, flags and
 * RAM; R14 and the ROM buffer move as on the Super FX.
 */
void abi_FxFrameProgram(void)
{
    frame_run(0xAB13, 0xFFFF);
}

/*
 * @routine 01:AB81 FxObjectsDraw
 * WHAT IT DOES: Draws every object in the draw list from $023C: its
 * angles, flags and shape from the slot; a slot whose flags have bit 2
 * adds a burst of effect particles (FxEffectsAdd) and draws them
 * (FxParticlesDraw); else the shape's header (in ROM bank $14) picks
 * the model by distance (+$16, +$18, +$1A at depths $3E8, $7D0, $BB8)
 * and the object is drawn as a sprite (FxObjectSprite) or a model
 * (FxModelDraw).
 * HOW: Pushes R11 on the R10 stack and each slot's address around its
 * drawing; the next slot from its link word (+$00) until 0; returns by
 * pulling R15.
 * Entry: R10 = stack; R11 = return address.
 * Exit: as the last object leaves the registers, flags and RAM; R14 and
 * the ROM buffer move as on the Super FX.
 */
void abi_FxObjectsDraw(void)
{
    frame_run(0xAB81, R[11]);
}

/*
 * @routine 01:B0D9 FxObjectSprite
 * WHAT IT DOES: Draws an object as a sprite scaled by its depth: its
 * screen position (FxProjectScaled), its colour from its material
 * (FxSpriteMaterial), and its rows of texels plotted, clipped to the
 * screen; nothing when its depth $002A is at most $0080.
 * HOW: Pushes R11 on the R10 stack; the size from $00CE and $00AA
 * shifted by $0032; the drawing goes on in FxObjectSpriteRows
 * (01:B2ED); returns by pulling R15.
 * Entry: R10 = stack; R11 = return address; the object in $0020-$002A,
 * $00AA, $00CE, $00D6.
 * Exit: as the code leaves the registers, flags and RAM; R14 and the
 * ROM buffer move as on the Super FX.
 */
void abi_FxObjectSprite(void)
{
    frame_run(0xB0D9, R[11]);
}

/*
 * @routine 01:B147 FxObjectSpritePull
 * WHAT IT DOES: FxObjectSprite's return when the size it pulled back at
 * 01:B13F negates to 0. The game never runs it: that size was pushed at
 * 01:B128 only when it is negative ($00CE), so its negation is never 0
 * and the BNE at 01:B144 always branches. A unit start runs it so the C
 * is judged against the original.
 * HOW: DEC R10 twice, then R15 = the word there (NOP in the delay slot).
 * Entry: R10 = stack, the return address the word below it. Exit: R10
 * two lower; S, Z from R10.
 */
void abi_FxObjectSpritePull(void)
{
    frame_run(0xB147, R[11]);
}

/*
 * @routine 01:B17D FxObjectSpriteAt
 * WHAT IT DOES: A second way into FxObjectSprite's drawing (the word
 * $B17D is at 01:B0D2, in code no script has run): pushes R11 on the
 * R10 stack and goes on as FxObjectSprite does once its colour is set
 * (not reached in the traces so far).
 * HOW: FROM R11; STW (R10); INC R10 twice; then 01:B182.
 * Entry: R10 = stack; R11 = return address; as FxObjectSprite has them
 * at 01:B182.
 * Exit: as FxObjectSprite.
 */
void abi_FxObjectSpriteAt(void)
{
    frame_run(0xB17D, R[11]);
}

/*
 * @routine 01:B2ED FxObjectSpriteRows
 * WHAT IT DOES: The rows of an object sprite: the texel step from the
 * size (FxDivide32By16Halving, or $FFFF when the size is the scale),
 * then each row of texels from the sprite's texture (bank $00A6,
 * address $00A8, mask $00D4) plotted between the screen's edges,
 * columns stepped by R9; mirrored when the mask's bit 15 is set
 * (01:B3AB).
 * HOW: Reached from FxObjectSprite (01:B1CA jumps here, after
 * FxProjectScaled's code ends at 01:B2EC). CACHE; GETC, PLOT in a LOOP
 * per row; returns by pulling R15 for FxObjectSprite.
 * Entry: as FxObjectSprite leaves them: R1 = the size, R6 = the scale;
 * R10 = stack.
 * Exit: as the code leaves the registers, flags and RAM; R14 and the
 * ROM buffer move as on the Super FX.
 */
void abi_FxObjectSpriteRows(void)
{
    frame_run(0xB2ED, R[11]);
}

/*
 * @routine 01:B6D5 FxEffectsAdd
 * WHAT IT DOES: Adds R3 effect particles of kind R0 (1 to 7) at R1 / R2
 * to the twenty 14-byte slots at $273E (a slot with byte 0 free), each
 * kind's speeds, life and colour from the random seed at $01D0.
 * HOW: Pushes R11 on the R10 stack; ROMB $14, RAMB 0; a LOOP of R12 =
 * 20 (R13 = $B6F4) over the slots; the seed stepped as SWAP, ROR, ADD,
 * ADC; returns by pulling R15.
 * Entry: R0 = kind, R1, R2 = place, R3 = count; R10 = stack; R11 =
 * return address.
 * Exit: R0-R9 as the last slot left them; $01D0 = the seed; R12, R13
 * the loop; the rest as the code leaves them.
 */
void abi_FxEffectsAdd(void)
{
    frame_run(0xB6D5, R[11]);
}

/*
 * @routine 01:B763 FxEffectsAddNext
 * WHAT IT DOES: FxEffectsAdd's step to the next particle: one fewer to
 * add (R9); when none are left, or after the last of the twenty slots,
 * the seed is kept at $01D0 and FxEffectsAdd returns.
 * HOW: DEC R9; the next slot R3 + 14 by LOOP (R13 = $B6F4); the return
 * pulled from the R10 stack. Every kind's set-up jumps here.
 * Entry: as FxEffectsAdd leaves them in its slot loop.
 * Exit: as FxEffectsAdd.
 */
void abi_FxEffectsAddNext(void)
{
    frame_run(0xB763, R[11]);
}

/*
 * @routine 01:B7E6 FxEffectsAddUnused
 * WHAT IT DOES: Code no jump or branch reaches: after kind 6's set-up
 * (which ends by jumping to 01:B763) a second copy of a kind's set-up,
 * its speeds halved differently; nothing in bank 01 jumps or branches
 * here, so a direct call is its only way in.
 * HOW: As kind 6 at 01:B774, ending at 01:B855 with the jump to the
 * next slot (01:B763).
 * Entry: as FxEffectsAdd at a kind's set-up: R3 = the slot, R9 = the
 * particles left, R2 = the seed, R6, R12, R10 = stack. Exit: as
 * FxEffectsAdd.
 */
void abi_FxEffectsAddUnused(void)
{
    frame_run(0xB7E6, R[11]);
}

/*
 * @routine 01:B856 FxEffectsAddKinds
 * WHAT IT DOES: FxEffectsAdd's set-ups for kinds 3, 4, 5 and 7
 * (01:B856, 01:B92F, 01:B99B, 01:B9A3) and kind 1's (01:B99F): each
 * particle's colour, life and speeds from the seed, then the next slot
 * (01:B763).
 * HOW: Reached by jump from FxEffectsAdd's dispatch on the kind; the
 * seed stepped as SWAP, ROR, ADD, ADC.
 * Entry: as FxEffectsAdd leaves them at the dispatch.
 * Exit: as FxEffectsAdd.
 */
void abi_FxEffectsAddKinds(void)
{
    frame_run(0xB856, R[11]);
}

/*
 * @routine 01:BC35 FxWeatherReset
 * WHAT IT DOES: Sets the weather afresh: $2B94 = R0 (the weather now
 * drawn), the 80 drops at $2864 (ten bytes each: a height from 0 down
 * by one, then nine zeros), $2B8E and $2B90 cleared and the 48 words at
 * $2688 cleared.
 * HOW: Pushes R11 on the R10 stack; CACHE; two LOOPs of STB and one of
 * STW; returns by pulling R15. FxWeatherDraw branches here (01:BC83)
 * when R0 is not $2B94.
 * Entry: R0 = the weather; R10 = stack; R11 = return address.
 * Exit: R0 = 0; R1 past the words at $2688; R2 = 0; R12 = 0; R13 the
 * last loop; S and Z from R10; the last RAM address R10.
 */
void abi_FxWeatherReset(void)
{
    frame_run(0xBC35, R[11]);
}

/*
 * @routine 01:BC7D FxWeatherDraw
 * WHAT IT DOES: Draws the weather: R0 = 1 (rain) or 2 (snow) with R1
 * drops; when R0 is not $2B94 the drops are set afresh instead
 * (FxWeatherReset); each drop moved by the view's turn ($00E0) and
 * drawn with PLOT.
 * HOW: Pushes R11 on the R10 stack; RAMB 0, ROMB $14, CMODE 1; the drop
 * list at $2864; the seed at $01D0; returns by pulling R15.
 * Entry: R0 = the weather, R1 = the drops; R10 = stack; R11 = return
 * address.
 * Exit: $01D0 = the seed; the rest as the code leaves them.
 */
void abi_FxWeatherDraw(void)
{
    frame_run(0xBC7D, R[11]);
}

/*
 * @routine 01:BF2F FxWeatherUnused
 * WHAT IT DOES: Code no jump or branch reaches: a branch back into
 * FxWeatherDraw's drop loop (01:BF2A); nothing in bank 01 jumps or
 * branches here, so a direct call is its only way in. The code after
 * it is three more such entries: FxWeatherUnusedLoop,
 * FxWeatherUnusedClear and FxWeatherUnusedFull.
 * HOW: BRA with a NOP in its delay slot; then as FxWeatherDraw.
 * Entry: as FxWeatherDraw's drop loop: R12 = the drops left, R13 = the
 * loop, R10 = stack. Exit: as FxWeatherDraw.
 */
void abi_FxWeatherUnused(void)
{
    frame_run(0xBF2F, R[11]);
}

/*
 * @routine 01:BF32 FxWeatherUnusedLoop
 * WHAT IT DOES: The second of FxWeatherUnused's branches back into
 * FxWeatherDraw's drop loop (01:BF2A), which nothing reaches; a direct
 * call is its only way in.
 * HOW: BRA with a NOP in its delay slot; then as FxWeatherDraw.
 * Entry: as FxWeatherDraw's drop loop: R12 = the drops left, R13 = the
 * loop, R10 = stack. Exit: as FxWeatherDraw.
 */
void abi_FxWeatherUnusedLoop(void)
{
    frame_run(0xBF32, R[11]);
}

/*
 * @routine 01:BF35 FxWeatherUnusedClear
 * WHAT IT DOES: Sets the word four bytes before R8 (a drop's) to 0 and
 * goes on with the drop loop (01:BF2A); nothing reaches it (it follows
 * an unconditional BRA), so a direct call is its only way in.
 * HOW: IWT R2,#0; BRA to the store at 01:BF3E (R8 - 10 + 6), R8 kept.
 * Entry: R8 = the drop; the drop loop's R12, R13, R10. Exit: R1 = R8;
 * R2 = 0; then as FxWeatherDraw.
 */
void abi_FxWeatherUnusedClear(void)
{
    frame_run(0xBF35, R[11]);
}

/*
 * @routine 01:BF3B FxWeatherUnusedFull
 * WHAT IT DOES: As FxWeatherUnusedClear with $01FF: the word four bytes
 * before R8 set to $01FF, then the drop loop (01:BF2A); nothing reaches
 * it, so a direct call is its only way in.
 * HOW: IWT R2,#$01FF; the store at 01:BF3E.
 * Entry: R8 = the drop; the drop loop's R12, R13, R10. Exit: R1 = R8;
 * R2 = $01FF; then as FxWeatherDraw.
 */
void abi_FxWeatherUnusedFull(void)
{
    frame_run(0xBF3B, R[11]);
}

/*
 * @routine 01:BF4D FxWeatherDropNext
 * WHAT IT DOES: The ends of FxWeatherDraw's drop loop: a drop's last
 * word cleared (01:BF4D) or its state byte set to 1 (01:BF53), then the
 * next drop (01:BF2A); after the last, the seed kept at $01D0 and the
 * return (01:BF65).
 * HOW: Reached by jump from the drop's moves; returns by pulling R15
 * for FxWeatherDraw.
 * Entry: as FxWeatherDraw leaves them in its drop loop.
 * Exit: as FxWeatherDraw.
 */
void abi_FxWeatherDropNext(void)
{
    frame_run(0xBF4D, R[11]);
}
