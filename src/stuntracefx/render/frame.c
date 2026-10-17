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

/* RPIX: the pixel at (x, y), the pixel caches written out first; S and Z
   from it. */
static uint16_t gsu_rpix_r(uint16_t x, uint16_t y)
{
    uint16_t v = gsu_rpix((uint8_t)x, (uint8_t)y);

    gsu_sz(v);
    return v;
}

/* The code from address at until it jumps to ret (the caller's return;
   $FFFF for a program, which ends at its STOP). */
static void frame_run(uint16_t at, uint16_t ret)
{
    int t;
    uint16_t nx;

    (void)t;
    (void)nx;
    goto dispatch;
    /* FxFrameProgram */
lAB13:
    R[10] = 0x0240;
    R[0] = gsu_ldw(GSU_WORD_1C4);
    R[0] = gsu_lob(R[0]);
    t = gsu.z;
    if (t) goto lAB22;
    R[11] = 0xAB22;
    abi_FxScreenClear();
lAB22:
    R[10] = 0x0240;
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_stw(FX_MODEL_1B4, R[0]);
    R[11] = 0xAB2E;
    goto lAB81;
lAB2E:
    R[10] = 0x0240;
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_cmode(R[0]);
    R[2] = gsu_ldw(GSU_TIED_22);
    R[2] = R[2];
    gsu_moves(R[2]);
    t = gsu.z;
    if (t) goto lAB69;
    R[1] = 0x008A;
    R[1] = gsu_add(R[1], R[2], 0);
    R[1] = gsu_ldw(R[1]);
    R[0] = 0x0001;
    R[0] = gsu_and(R[0], R[1]);
    t = gsu.z;
    if (t) goto lAB56;
    R[0] = 0x0010;
    gsu_stw(FX_EFFECT_MAX, R[0]);
    R[0] = 0x0001;
    t = 1;
    if (t) goto lAB5F;
lAB56:
    R[0] = 0x0002;
    R[0] = gsu_and(R[0], R[1]);
    t = gsu.z;
    if (t) goto lAB69;
    R[0] = 0x0002;
lAB5F:
    R[1] = 0x0050;
    R[11] = 0xAB66;
    goto lBC7D;
lAB66:
    t = 1;
    if (t) goto lAB6E;
lAB69:
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_stw(FX_INIT_2B94, R[0]);
lAB6E:
    R[11] = 0xAB73;
    abi_FxEffectsRun();
lAB73:
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_stw(FX_EFFECT_MAX, R[0]);
    R[11] = 0xAB7D;
    abi_FxTextNotice();
lAB7D:
    R[0] = gsu_rpix_r(R[1], R[2]);
    return;   /* the STOP: the program ends here */
    /* FxObjectsDraw */
lAB81:
    R[1] = gsu_ldw(GSU_DRAW_LIST);
    R[1] = R[1];
    gsu_moves(R[1]);
    t = !gsu.z;
    if (t) goto lAB8C;
    nx = (uint16_t)(R[11]);
    at = nx;
    goto dispatch;
lAB8C:
    gsu_stw(R[10], R[11]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
lAB90:
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_romb(R[0]);
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_cmode(R[0]);
    gsu_stw(FX_POINTS_TOTAL, R[0]);
    gsu_stw(R[10], R[1]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[1] = gsu_inc(R[1]);
    R[1] = gsu_inc(R[1]);
    R[1] = gsu_inc(R[1]);
    R[1] = gsu_inc(R[1]);
    R[0] = gsu_ldb(R[1]);
    gsu_stw(FX_OBJECT_ANGLE_X, R[0]);
    R[1] = gsu_inc(R[1]);
    R[0] = gsu_ldb(R[1]);
    gsu_stw(FX_CAR_0022, R[0]);
    R[1] = gsu_inc(R[1]);
    R[0] = gsu_ldb(R[1]);
    gsu_stw(FX_OBJECT_ANGLE_Z, R[0]);
    R[1] = gsu_inc(R[1]);
    R[4] = gsu_ldb(R[1]);
    R[1] = gsu_inc(R[1]);
    R[3] = gsu_ldw(R[1]);
    R[3] = R[3];
    gsu_moves(R[3]);
    t = !gsu.z;
    if (t) goto lABC2;
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
    R[0] = gsu_ldw(R[1]);
    gsu_stw(FX_OBJECT_Y, R[0]);
    R[1] = gsu_inc(R[1]);
    R[1] = gsu_inc(R[1]);
    R[0] = gsu_ldw(R[1]);
    gsu_stw(FX_OBJECT_X, R[0]);
    R[1] = gsu_inc(R[1]);
    R[1] = gsu_inc(R[1]);
    R[0] = gsu_ldw(R[1]);
    gsu_stw(FX_OBJECT_Z, R[0]);
    R[6] = R[0];
    R[1] = gsu_inc(R[1]);
    R[1] = gsu_inc(R[1]);
    R[5] = 0x0004;
    R[0] = gsu_and(R[4], R[5]);
    t = gsu.z;
    if (t) goto lAC31;
    R[0] = 0xFFF4;
    R[1] = gsu_add(R[0], R[1], 0);
    R[0] = gsu_ldw(R[1]);
    gsu_stw(FX_MODEL_148, R[0]);
    R[1] = gsu_inc(R[1]);
    R[1] = gsu_inc(R[1]);
    R[0] = gsu_ldb(R[1]);
    R[0] = gsu_lob(R[0]);
    gsu_stw(FX_MODEL_14E, R[0]);
    R[1] = gsu_inc(R[1]);
    R[0] = gsu_ldb(R[1]);
    R[0] = gsu_lob(R[0]);
    gsu_stw(FX_MODEL_150, R[0]);
    R[1] = gsu_inc(R[1]);
    R[0] = gsu_ldb(R[1]);
    R[0] = gsu_lob(R[0]);
    R[5] = 0x0007;
    R[0] = gsu_and(R[0], R[5]);
    t = !gsu.z;
    if (t) goto lAC09;
    R[0] = 0xFFFF;
lAC09:
    gsu_stw(FX_POINTS_TOTAL, R[0]);
    R[0] = gsu_ldw(FX_POINTS_TOTAL);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.z;
    if (t) goto lAC2D;
    t = gsu.s;
    if (t) goto lAC25;
    R[1] = gsu_ldw(FX_MODEL_150);
    R[2] = gsu_ldw(FX_MODEL_148);
    R[3] = gsu_ldw(FX_MODEL_14E);
    R[11] = 0xAC25;
    goto lB6D5;
lAC25:
    R[2] = gsu_ldw(FX_MODEL_148);
    R[11] = 0xAC2D;
    abi_FxParticlesDraw();
lAC2D:
    goto lAD53;
lAC31:
    R[5] = 0x0002;
    R[0] = gsu_and(R[4], R[5]);
    t = gsu.z;
    if (t) goto lAC48;
    R[5] = 0x0008;
    R[0] = gsu_and(R[4], R[5]);
    t = !gsu.z;
    if (t) goto lAC48;
    R[5] = 0x8D30;
    gsu_stw(FX_MATERIAL_TABLE, R[5]);
    t = 1;
    if (t) goto lAC6D;
lAC48:
    R[0] = gsu_ldw(R[1]);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = !gsu.z;
    if (t) goto lAC6A;
    gsu_r14(0x0012);
    gsu_r14(gsu_add(R[3], R[14], 0));
    R[0] = gsu_getb();
    gsu_r14(gsu_inc(R[14]));
    R[0] = gsu_getbh(R[0]);
    R[5] = 0x8F91;
    gsu_sub(R[0], R[5], 0);   /* CMP */
    t = !gsu.z;
    if (t) goto lAC6A;
    R[5] = gsu_ldw(GSU_PARAM_108A);
    R[5] = R[5];
    gsu_moves(R[5]);
    t = gsu.z;
    if (t) goto lAC6A;
    R[0] = R[5];
lAC6A:
    gsu_stw(FX_MATERIAL_TABLE, R[0]);
lAC6D:
    R[1] = gsu_inc(R[1]);
    R[1] = gsu_inc(R[1]);
    gsu_r14(0x0007);
    gsu_r14(gsu_add(R[3], R[14], 0));
    R[0] = gsu_getb();
    gsu_stw(FX_POINT_SHIFT, R[0]);
    gsu_stw(R[10], R[3]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[0] = R[6];
    R[5] = 0x0BB8;
    gsu_sub(R[0], R[5], 0);   /* CMP */
    t = gsu.cy;
    if (t) goto lAC99;
    R[5] = 0x07D0;
    gsu_sub(R[0], R[5], 0);   /* CMP */
    t = gsu.cy;
    if (t) goto lACA7;
    R[5] = 0x03E8;
    gsu_sub(R[0], R[5], 0);   /* CMP */
    t = gsu.cy;
    if (t) goto lACB5;
    t = 1;
    if (t) goto lACC0;
lAC99:
    gsu_r14(0x001A);
    gsu_r14(gsu_add(R[3], R[14], 0));
    R[3] = gsu_getb();
    gsu_r14(gsu_inc(R[14]));
    R[3] = gsu_getbh(R[3]);
    t = 1;
    if (t) goto lACC0;
lACA7:
    gsu_r14(0x0018);
    gsu_r14(gsu_add(R[3], R[14], 0));
    R[3] = gsu_getb();
    gsu_r14(gsu_inc(R[14]));
    R[3] = gsu_getbh(R[3]);
    t = 1;
    if (t) goto lACC0;
lACB5:
    gsu_r14(0x0016);
    gsu_r14(gsu_add(R[3], R[14], 0));
    R[3] = gsu_getb();
    gsu_r14(gsu_inc(R[14]));
    R[3] = gsu_getbh(R[3]);
lACC0:
    gsu_r14(0x0000);
    gsu_r14(gsu_add(R[3], R[14], 0));
    R[0] = gsu_getb();
    gsu_r14(gsu_inc(R[14]));
    R[0] = gsu_getbh(R[0]);
    gsu_stw(FX_MODEL_AT, R[0]);
    gsu_r14(0x0002);
    gsu_r14(gsu_add(R[3], R[14], 0));
    R[0] = gsu_getb();
    gsu_stw(FX_ROM_BANK, R[0]);
    gsu_r14(0x0003);
    gsu_r14(gsu_add(R[3], R[14], 0));
    R[0] = gsu_getb();
    gsu_r14(gsu_inc(R[14]));
    R[0] = gsu_getbh(R[0]);
    gsu_stw(FX_MODEL_18, R[0]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    R[3] = gsu_ldw(R[10]);
    R[0] = gsu_ldb(R[1]);
    R[1] = gsu_inc(R[1]);
    gsu_stw(FX_OBJECT_KIND, R[0]);
    R[0] = gsu_ldb(R[1]);
    gsu_stw(FX_MODEL_FRAME, R[0]);
    R[1] = gsu_inc(R[1]);
    R[0] = gsu_ldb(R[1]);
    gsu_stw(FX_MATERIAL_MASK, R[0]);
    R[1] = gsu_inc(R[1]);
    R[5] = 0x0008;
    R[0] = gsu_and(R[4], R[5]);
    t = gsu.z;
    if (t) goto lAD31;
    R[0] = gsu_ldw(FX_MODEL_1B4);
    R[0] = gsu_lob(R[0]);
    t = !gsu.z;
    if (t) goto lAD31;
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_romb(R[0]);
    gsu_r14(0x0010);
    gsu_r14(gsu_add(R[3], R[14], 0));
    R[0] = gsu_getb();
    gsu_r14(gsu_inc(R[14]));
    R[0] = gsu_getbh(R[0]);
    gsu_stw(FX_SPRITE_SIZE, R[0]);
    R[0] = gsu_ldb(R[1]);
    gsu_stw(FX_SPRITE_MATERIAL, R[0]);
    R[1] = gsu_inc(R[1]);
    R[0] = gsu_ldb(R[1]);
    R[0] = gsu_sex(R[0]);
    gsu_stw(FX_MODEL_CE, R[0]);
    R[0] = gsu_ldw(FX_MODEL_AT);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.z;
    if (t) goto lAD2D;
    R[11] = 0xAD2D;
    goto lB0D9;
lAD2D:
    goto lAD53;
lAD31:
    R[0] = gsu_ldb(R[1]);
    gsu_stw(FX_MODEL_1B6, R[0]);
    R[1] = gsu_inc(R[1]);
    R[0] = gsu_sub(R[0], R[0], 0);
    R[0] = gsu_ldb(R[1]);
    R[0] = gsu_swap(R[0]);
    gsu_stw(FX_TEX_U_OFF, R[0]);
    R[1] = gsu_inc(R[1]);
    R[0] = gsu_ldb(R[1]);
    R[0] = gsu_swap(R[0]);
    gsu_stw(FX_TEX_V_OFF, R[0]);
    R[1] = gsu_inc(R[1]);
    R[0] = gsu_ldw(FX_MODEL_18);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.z;
    if (t) goto lAD53;
    R[11] = 0xAD53;
    abi_FxModelDraw();
lAD53:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    R[1] = gsu_ldw(R[10]);
    R[1] = gsu_ldw(R[1]);
    R[1] = R[1];
    gsu_moves(R[1]);
    t = gsu.z;
    if (t) goto lAD62;
    goto lAB90;
lAD62:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
    /* FxObjectSprite */
lB0D9:
    gsu_stw(R[10], R[11]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[3] = gsu_ldw(FX_OBJECT_Z);
    R[0] = 0x0080;
    R[0] = gsu_sub(R[0], R[3], 0);
    t = !gsu.s;
    if (t) goto lB123;
    R[0] = gsu_ldw(FX_MODEL_CE);
    R[1] = R[0];
    gsu_moves(R[0]);
    t = gsu.s;
    if (t) goto lB128;
    /* CACHE */
    R[12] = gsu_ldw(FX_POINT_SHIFT);
    R[12] = R[12];
    gsu_moves(R[12]);
    t = gsu.z;
    if (t) goto lB100;
    R[0] = R[1];
    R[13] = 0xB0FC;
lB0FC:
    t = gsu_count_down(&R[12]);
    R[0] = gsu_add(R[0], R[0], 0);
    if (t) goto lB0FC;
    R[1] = R[0];
lB100:
    R[0] = gsu_ldw(FX_SPRITE_SIZE);
    R[0] = gsu_add(R[0], R[0], 0);
    R[1] = gsu_add(R[0], R[1], 0);
    t = !gsu.z;
    if (t) goto lB10B;
    R[1] = 0x0001;
lB10B:
    R[5] = R[1];
    R[1] = gsu_ldw(FX_OBJECT_X);
    R[2] = gsu_ldw(FX_OBJECT_Y);
    R[11] = 0xB118;
    abi_FxProjectScaled();
lB118:
    gsu_stw(FX_MODEL_BA, R[1]);
    gsu_stw(FX_MODEL_BC, R[2]);
    R[1] = R[5];
    gsu_moves(R[5]);
    t = !gsu.z;
    if (t) goto lB14C;
lB123:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
lB128:
    gsu_stw(R[10], R[1]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[1] = gsu_ldw(FX_OBJECT_X);
    R[2] = gsu_ldw(FX_OBJECT_Y);
    R[11] = 0xB137;
    abi_FxProjectScaled();
lB137:
    gsu_stw(FX_MODEL_BA, R[1]);
    gsu_stw(FX_MODEL_BC, R[2]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    R[1] = gsu_ldw(R[10]);
    R[1] = gsu_not(R[1]);
    R[1] = gsu_inc(R[1]);
    t = !gsu.z;
    if (t) goto lB14C;
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
lB14C:
    R[0] = 0x00F0;
    R[0] = gsu_sub(R[0], R[1], 0);
    t = !gsu.s;
    R[0] = gsu_add(R[0], R[1], 0);
    if (t) goto lB155;
    R[1] = R[0];
lB155:
    gsu_stw(FX_MODEL_CE, R[1]);
    R[0] = gsu_ldw(FX_SPRITE_MATERIAL);
    R[11] = 0xB160;
    abi_FxSpriteMaterial();
lB160:
    R[0] = gsu_ldw(FX_FACE_KIND);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.z;
    if (t) goto lB123;
    R[1] = gsu_ldw(FX_MODEL_CE);
    R[0] = gsu_ldw(FX_TEX_MASK);
    R[0] = gsu_lob(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_stw(FX_MODEL_C8, R[0]);
    R[0] = gsu_ldw(FX_FACE_COLOUR);
    gsu_color((uint8_t)R[0]);
    R[0] = gsu_swap(R[0]);
    gsu_cmode(R[0]);
    t = 1;
    if (t) goto lB182;
    /* FxObjectSpriteAt */
lB17D:
    gsu_stw(R[10], R[11]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    /* CACHE */
lB182:
    R[3] = gsu_not(R[1]);
    R[3] = gsu_inc(R[3]);
    gsu_stw(FX_SPRITE_CLIP_X, R[3]);
    R[3] = gsu_ldw(FX_SCREEN_RIGHT);
    R[3] = gsu_sub(R[3], R[1], 0);
    gsu_stw(FX_SPRITE_CLIP_W, R[3]);
    R[3] = 0xFFFF;
    R[0] = gsu_lsr(R[1]);
    R[0] = gsu_sub(R[0], R[3], !gsu.cy);
    R[3] = gsu_ldw(FX_MODEL_BA);
    R[0] = gsu_sub(R[3], R[0], 0);
    gsu_sbk(R[0]);
    R[3] = gsu_ldw(FX_SPRITE_CLIP_X);
    R[3] = gsu_inc(R[3]);
    gsu_sub(R[0], R[3], 0);   /* CMP */
    t = !gsu.s;
    if (t) goto lB1AB;
lB1A6:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
lB1AB:
    R[3] = gsu_ldw(FX_SCREEN_RIGHT);
    gsu_sub(R[0], R[3], 0);   /* CMP */
    t = !gsu.s;
    if (t) goto lB1A6;
    R[3] = 0xFFFF;
    R[0] = gsu_lsr(R[1]);
    R[0] = gsu_sub(R[0], R[3], !gsu.cy);
    R[3] = gsu_ldw(FX_MODEL_BC);
    R[0] = gsu_sub(R[3], R[0], 0);
    gsu_sbk(R[0]);
    R[6] = gsu_ldw(FX_MODEL_C8);
    R[0] = gsu_sub(R[1], R[6], 0);
    t = !gsu.cy;
    if (t) goto lB1CB;
    goto lB2ED;
lB1CB:
    R[4] = R[6];
    R[0] = 0x0000;
    gsu_romb(R[0]);
    R[0] = 0x94A0;
    R[0] = gsu_add(R[0], R[1], 0);
    gsu_r14(gsu_add(R[0], R[1], 0));
    R[6] = gsu_getb();
    gsu_r14(gsu_inc(R[14]));
    R[6] = gsu_getbh(R[6]);
    R[0] = gsu_swap(R[4]);
    R[0] = gsu_fmult(R[0], R[6]);
    R[9] = gsu_rol(R[0]);
    /* CACHE */
    R[2] = gsu_ldw(FX_MODEL_BC);
    R[7] = 0x0000;
    R[8] = R[7];
    R[4] = gsu_ldw(FX_MODEL_CE);
    R[6] = R[4];
    R[0] = gsu_ldw(FX_TEX_BANK);
    gsu_romb(R[0]);
    R[5] = gsu_ldw(FX_TEX_BASE);
    R[3] = gsu_ldw(FX_MODEL_BA);
    R[0] = gsu_ldw(FX_TEX_MASK);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = !gsu.s;
    if (t) goto lB206;
    goto lB251;
lB206:
    R[3] = R[3];
    gsu_moves(R[3]);
    t = !gsu.s;
    if (t) goto lB220;
    R[12] = R[6];
    R[13] = 0xB20F;
lB20F:
    t = !gsu.s;
    if (t) goto lB21B;
    R[8] = gsu_add(R[8], R[9], 0);
    t = gsu_count_down(&R[12]);
    R[3] = gsu_inc(R[3]);
    if (t) goto lB20F;
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
lB21B:
    R[6] = R[12];
    R[0] = gsu_merge();
    R[5] = gsu_add(R[0], R[5], 0);
lB220:
    R[0] = gsu_ldw(FX_SPRITE_CLIP_W);
    R[0] = gsu_sub(R[0], R[3], 0);
    t = !gsu.s;
    if (t) goto lB22A;
    R[6] = gsu_add(R[6], R[0], 0);
    R[6] = gsu_inc(R[6]);
lB22A:
    R[13] = 0xB23E;
lB22D:
    R[0] = gsu_ldw(FX_SCREEN_BOTTOM);
    R[0] = gsu_dec(R[0]);
    R[0] = gsu_sub(R[0], R[2], 0);
    t = !gsu.cy;
    if (t) goto lB246;
    R[8] = 0x0000;
    R[0] = gsu_merge();
    gsu_r14(gsu_add(R[0], R[5], 0));
    R[1] = R[3];
    R[12] = R[6];
lB23E:
    R[8] = gsu_add(R[8], R[9], 0);
    R[0] = gsu_merge();
    gsu_color(gsu_getb());   /* GETC */
    gsu_r14(gsu_add(R[0], R[5], 0));
    t = gsu_count_down(&R[12]);
    gsu_plot_r1();
    if (t) goto lB23E;
lB246:
    R[7] = gsu_add(R[7], R[9], 0);
    R[4] = gsu_dec(R[4]);
    t = !gsu.z;
    R[2] = gsu_inc(R[2]);
    if (t) goto lB22D;
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
lB251:
    R[0] = gsu_ldw(FX_MODEL_C8);
    R[5] = gsu_add(R[5], R[0], 0);
    R[5] = gsu_dec(R[5]);
    R[3] = R[3];
    gsu_moves(R[3]);
    t = !gsu.s;
    if (t) goto lB271;
    R[12] = R[6];
    R[13] = 0xB260;
lB260:
    t = !gsu.s;
    if (t) goto lB26C;
    R[8] = gsu_sub(R[8], R[9], 0);
    t = gsu_count_down(&R[12]);
    R[3] = gsu_inc(R[3]);
    if (t) goto lB260;
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
lB26C:
    R[6] = R[12];
    R[0] = gsu_merge();
    R[5] = gsu_add(R[0], R[5], 0);
lB271:
    R[1] = R[3];
    R[0] = gsu_ldw(FX_SPRITE_CLIP_W);
    R[0] = gsu_sub(R[0], R[1], 0);
    t = !gsu.s;
    if (t) goto lB27D;
    R[6] = gsu_add(R[6], R[0], 0);
    R[6] = gsu_inc(R[6]);
lB27D:
    R[13] = 0xB291;
lB280:
    R[0] = gsu_ldw(FX_SCREEN_BOTTOM);
    R[0] = gsu_dec(R[0]);
    R[0] = gsu_sub(R[0], R[2], 0);
    t = !gsu.cy;
    if (t) goto lB299;
    R[8] = 0x0000;
    R[0] = gsu_merge();
    gsu_r14(gsu_add(R[0], R[5], 0));
    R[1] = R[3];
    R[12] = R[6];
lB291:
    R[8] = gsu_sub(R[8], R[9], 0);
    R[0] = gsu_merge();
    gsu_color(gsu_getb());   /* GETC */
    gsu_r14(gsu_add(R[0], R[5], 0));
    t = gsu_count_down(&R[12]);
    gsu_plot_r1();
    if (t) goto lB291;
lB299:
    R[7] = gsu_add(R[7], R[9], 0);
    R[4] = gsu_dec(R[4]);
    t = !gsu.z;
    R[2] = gsu_inc(R[2]);
    if (t) goto lB280;
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
    /* FxObjectSpriteRows */
lB2ED:
    R[0] = R[6];
    R[6] = R[1];
    gsu_sub(R[0], R[6], 0);   /* CMP */
    t = !gsu.z;
    if (t) goto lB2FC;
    R[4] = 0xFFFF;
    t = 1;
    if (t) goto lB303;
lB2FC:
    R[4] = 0x0000;
    R[11] = 0xB303;
    abi_FxDivide32By16Halving();
lB303:
    R[9] = R[4];
    /* CACHE */
    R[6] = gsu_ldw(FX_MODEL_C8);
    R[3] = gsu_ldw(FX_MODEL_BA);
    R[2] = gsu_ldw(FX_MODEL_BC);
    R[7] = 0x0000;
    R[11] = 0x0000;
    R[0] = gsu_ldw(FX_TEX_BANK);
    gsu_romb(R[0]);
    R[5] = gsu_ldw(FX_TEX_BASE);
    R[9] = gsu_bic(R[9], 1);
    R[0] = gsu_ldw(FX_TEX_MASK);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = !gsu.s;
    if (t) goto lB32A;
    goto lB3AB;
lB32A:
    R[13] = R[6];
    R[0] = gsu_ldw(FX_SCREEN_RIGHT);
    R[6] = gsu_sub(R[0], R[3], 0);
    R[0] = gsu_lsr(R[9]);
    R[0] = gsu_fmult(R[0], R[6]);
    R[4] = gsu_rol(R[0]);
    R[0] = gsu_sub(R[13], R[4], 0);
    R[0] = gsu_dec(R[0]);
    t = gsu.s;
    if (t) goto lB33E;
    R[13] = R[4];
lB33E:
    R[6] = R[13];
    R[3] = R[3];
    gsu_moves(R[3]);
    t = !gsu.s;
    if (t) goto lB35E;
    R[6] = gsu_not(R[3]);
    R[6] = gsu_inc(R[6]);
    R[0] = gsu_lsr(R[9]);
    { uint32_t p = gsu_lmult(R[0], R[6]); R[4] = (uint16_t)p; R[0] = (uint16_t)(p >> 16); }
    R[11] = gsu_add(R[4], R[4], 0);
    R[0] = gsu_rol(R[0]);
    t = gsu.s;
    if (t) goto lB3A6;
    R[5] = gsu_add(R[5], R[0], 0);
    R[6] = gsu_sub(R[13], R[0], 0);
    t = gsu.s;
    if (t) goto lB3A6;
    R[3] = 0x0000;
lB35E:
    R[6] = R[6];
    gsu_moves(R[6]);
    t = gsu.z;
    if (t) goto lB3A6;
    R[4] = 0x0000;
lB365:
    R[2] = R[2];
    gsu_moves(R[2]);
    t = gsu.s;
    if (t) goto lB398;
    R[0] = gsu_ldw(FX_SCREEN_BOTTOM);
    R[0] = gsu_dec(R[0]);
    R[0] = gsu_sub(R[0], R[2], 0);
    t = gsu.s;
    if (t) goto lB3A6;
    R[0] = gsu_swap(R[4]);
    gsu_r14(gsu_add(R[0], R[5], 0));
    R[1] = R[3];
    R[0] = R[11];
    gsu_color(gsu_getb());   /* GETC */
    R[12] = R[6];
    R[12] = gsu_dec(R[12]);
    t = gsu.z;
    if (t) goto lB38A;
    R[13] = 0xB383;
lB383:
    gsu_r14(gsu_inc(R[14]));
lB384:
    R[0] = gsu_add(R[0], R[9], 0);
    t = !gsu.cy;
    gsu_plot_r1();
    if (t) goto lB384;
    t = gsu_count_down(&R[12]);
    gsu_color(gsu_getb());   /* GETC */
    if (t) goto lB383;
lB38A:
    R[8] = R[0];
    R[0] = gsu_ldw(FX_SCREEN_RIGHT);
lB38F:
    gsu_sub(R[0], R[1], 0);   /* CMP */
    t = gsu.s;
    if (t) goto lB398;
    R[8] = gsu_add(R[8], R[9], 0);
    t = !gsu.cy;
    gsu_plot_r1();
    if (t) goto lB38F;
lB398:
    R[7] = gsu_sub(R[7], R[9], 0);
    t = gsu.cy;
    R[2] = gsu_inc(R[2]);
    if (t) goto lB365;
    R[4] = gsu_inc(R[4]);
    R[0] = gsu_ldw(FX_MODEL_C8);
    R[0] = gsu_sub(R[0], R[4], 0);
    t = !gsu.z;
    if (t) goto lB365;
lB3A6:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
lB3AB:
    R[5] = gsu_add(R[5], R[6], 0);
    R[5] = gsu_dec(R[5]);
    R[13] = R[6];
    R[0] = gsu_ldw(FX_SCREEN_RIGHT);
    R[6] = gsu_sub(R[0], R[3], 0);
    R[0] = gsu_lsr(R[9]);
    R[0] = gsu_fmult(R[0], R[6]);
    R[4] = gsu_rol(R[0]);
    R[0] = gsu_sub(R[13], R[4], 0);
    R[0] = gsu_dec(R[0]);
    t = gsu.s;
    if (t) goto lB3C2;
    R[13] = R[4];
lB3C2:
    R[6] = R[13];
    R[3] = R[3];
    gsu_moves(R[3]);
    t = !gsu.s;
    if (t) goto lB3E2;
    R[6] = gsu_not(R[3]);
    R[6] = gsu_inc(R[6]);
    R[0] = gsu_lsr(R[9]);
    { uint32_t p = gsu_lmult(R[0], R[6]); R[4] = (uint16_t)p; R[0] = (uint16_t)(p >> 16); }
    R[11] = gsu_add(R[4], R[4], 0);
    R[0] = gsu_rol(R[0]);
    t = gsu.s;
    if (t) goto lB42A;
    R[5] = gsu_sub(R[5], R[0], 0);
    R[6] = gsu_sub(R[13], R[0], 0);
    t = gsu.s;
    if (t) goto lB42A;
    R[3] = 0x0000;
lB3E2:
    R[6] = R[6];
    gsu_moves(R[6]);
    t = gsu.z;
    if (t) goto lB42A;
    R[4] = 0x0000;
lB3E9:
    R[2] = R[2];
    gsu_moves(R[2]);
    t = gsu.s;
    if (t) goto lB41C;
    R[0] = gsu_ldw(FX_SCREEN_BOTTOM);
    R[0] = gsu_dec(R[0]);
    R[0] = gsu_sub(R[0], R[2], 0);
    t = gsu.s;
    if (t) goto lB42A;
    R[0] = gsu_swap(R[4]);
    gsu_r14(gsu_add(R[0], R[5], 0));
    R[1] = R[3];
    R[0] = R[11];
    gsu_color(gsu_getb());   /* GETC */
    R[12] = R[6];
    R[12] = gsu_dec(R[12]);
    t = gsu.z;
    if (t) goto lB40E;
    R[13] = 0xB407;
lB407:
    gsu_r14(gsu_dec(R[14]));
lB408:
    R[0] = gsu_add(R[0], R[9], 0);
    t = !gsu.cy;
    gsu_plot_r1();
    if (t) goto lB408;
    t = gsu_count_down(&R[12]);
    gsu_color(gsu_getb());   /* GETC */
    if (t) goto lB407;
lB40E:
    R[8] = R[0];
    R[0] = gsu_ldw(FX_SCREEN_RIGHT);
lB413:
    gsu_sub(R[0], R[1], 0);   /* CMP */
    t = gsu.s;
    if (t) goto lB41C;
    R[8] = gsu_add(R[8], R[9], 0);
    t = !gsu.cy;
    gsu_plot_r1();
    if (t) goto lB413;
lB41C:
    R[7] = gsu_sub(R[7], R[9], 0);
    t = gsu.cy;
    R[2] = gsu_inc(R[2]);
    if (t) goto lB3E9;
    R[4] = gsu_inc(R[4]);
    R[0] = gsu_ldw(FX_MODEL_C8);
    R[0] = gsu_sub(R[0], R[4], 0);
    t = !gsu.z;
    if (t) goto lB3E9;
lB42A:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
    /* FxEffectsAdd */
lB6D5:
    gsu_stw(R[10], R[11]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[8] = R[0];
    R[6] = R[1];
    R[5] = R[2];
    R[9] = R[3];
    R[0] = 0x0014;
    gsu_romb(R[0]);
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu.rambr = (uint8_t)(R[0] & 1);
    R[3] = 0x273E;
    R[2] = gsu_ldw(FX_EFFECT_SEED);
    R[12] = 0x0014;
    /* CACHE */
    R[13] = 0xB6F4;
lB6F4:
    R[0] = gsu_ldb(R[3]);
    R[0] = gsu_lob(R[0]);
    t = !gsu.z;
    if (t) goto lB767;
    R[0] = gsu_sub(R[0], R[0], 0);
    R[7] = 0x0006;
    R[7] = gsu_add(R[7], R[3], 0);
    gsu_stw(R[7], R[0]);
    R[7] = 0x0008;
    R[7] = gsu_add(R[7], R[3], 0);
    gsu_stw(R[7], R[0]);
    R[7] = 0x000A;
    R[7] = gsu_add(R[7], R[3], 0);
    gsu_stw(R[7], R[0]);
    R[7] = 0x000C;
    R[7] = gsu_add(R[7], R[3], 0);
    gsu_stw(R[7], R[5]);
    gsu_stb(R[3], (uint8_t)R[6]);
    R[0] = 0x0001;
    gsu_sub(R[0], R[8], 0);   /* CMP */
    t = !gsu.z;
    if (t) goto lB721;
    goto lB99F;
lB721:
    R[0] = 0x0002;
    gsu_sub(R[0], R[8], 0);   /* CMP */
    t = !gsu.z;
    if (t) goto lB72C;
    goto lB8C3;
lB72C:
    R[0] = 0x0003;
    gsu_sub(R[0], R[8], 0);   /* CMP */
    t = !gsu.z;
    if (t) goto lB737;
    goto lB856;
lB737:
    R[0] = 0x0004;
    gsu_sub(R[0], R[8], 0);   /* CMP */
    t = !gsu.z;
    if (t) goto lB742;
    goto lB92F;
lB742:
    R[0] = 0x0005;
    gsu_sub(R[0], R[8], 0);   /* CMP */
    t = !gsu.z;
    if (t) goto lB74D;
    goto lB99B;
lB74D:
    R[0] = 0x0006;
    gsu_sub(R[0], R[8], 0);   /* CMP */
    t = !gsu.z;
    if (t) goto lB758;
    goto lB774;
lB758:
    R[0] = 0x0007;
    gsu_sub(R[0], R[8], 0);   /* CMP */
    t = !gsu.z;
    if (t) goto lB763;
    goto lB9A3;
lB763:
    R[9] = gsu_dec(R[9]);
    t = gsu.z;
    if (t) goto lB76C;
lB767:
    R[3] = gsu_add(R[3], 14, 0);
    t = gsu_count_down(&R[12]);
    if (t) goto lB6F4;
lB76C:
    gsu_stw(FX_EFFECT_SEED, R[2]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
lB774:
    R[0] = gsu_and(R[9], 15);
    R[0] = gsu_add(R[6], R[0], 0);
    R[0] = gsu_sub(R[0], 7, 0);
    R[7] = 0x0000;
    R[7] = gsu_add(R[7], R[3], 0);
    gsu_stb(R[7], (uint8_t)R[0]);
    R[0] = 0x0010;
    R[7] = 0x0001;
    R[7] = gsu_add(R[7], R[3], 0);
    gsu_stb(R[7], (uint8_t)R[0]);
    R[0] = R[9];
    R[0] = gsu_dec(R[0]);
    R[0] = gsu_add(R[0], R[0], 0);
    R[7] = 0xD0DC;
    gsu_r14(gsu_add(R[0], R[7], 0));
    R[0] = gsu_getbs();
    gsu_r14(gsu_inc(R[14]));
    R[0] = gsu_div2(R[0]);
    R[7] = 0x0003;
    R[7] = gsu_add(R[7], R[3], 0);
    gsu_stb(R[7], (uint8_t)R[0]);
    R[7] = 0x0006;
    R[7] = gsu_add(R[7], R[3], 0);
    gsu_stw(R[7], R[0]);
    R[0] = gsu_getbs();
    R[0] = gsu_div2(R[0]);
    R[7] = 0x0004;
    R[7] = gsu_add(R[7], R[3], 0);
    gsu_stb(R[7], (uint8_t)R[0]);
    R[7] = 0x0008;
    R[7] = gsu_add(R[7], R[3], 0);
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
    gsu_stb(R[7], (uint8_t)R[0]);
    R[7] = 0x000A;
    R[7] = gsu_add(R[7], R[3], 0);
    gsu_stw(R[7], R[0]);
    R[4] = 0x0004;
    R[0] = 0x0001;
    R[0] = gsu_and(R[0], R[12]);
    t = gsu.z;
    if (t) goto lB7DB;
    R[4] = 0x000E;
lB7DB:
    R[7] = 0x0002;
    R[7] = gsu_add(R[7], R[3], 0);
    gsu_stb(R[7], (uint8_t)R[4]);
    goto lB763;
    /* FxEffectsAddUnused */
lB7E6:
    R[0] = gsu_and(R[9], 15);
    R[0] = gsu_add(R[6], R[0], 0);
    R[0] = gsu_sub(R[0], 7, 0);
    R[7] = 0x0000;
    R[7] = gsu_add(R[7], R[3], 0);
    gsu_stb(R[7], (uint8_t)R[0]);
    R[0] = 0x0010;
    R[7] = 0x0001;
    R[7] = gsu_add(R[7], R[3], 0);
    gsu_stb(R[7], (uint8_t)R[0]);
    R[0] = R[9];
    R[0] = gsu_dec(R[0]);
    R[0] = gsu_add(R[0], R[0], 0);
    R[7] = 0xD0DC;
    gsu_r14(gsu_add(R[0], R[7], 0));
    R[0] = gsu_getbs();
    gsu_r14(gsu_inc(R[14]));
    R[7] = 0x0003;
    R[7] = gsu_add(R[7], R[3], 0);
    gsu_stb(R[7], (uint8_t)R[0]);
    R[7] = 0x0006;
    R[7] = gsu_add(R[7], R[3], 0);
    gsu_stw(R[7], R[0]);
    R[0] = gsu_getbs();
    R[7] = 0x0004;
    R[7] = gsu_add(R[7], R[3], 0);
    gsu_stb(R[7], (uint8_t)R[0]);
    R[7] = 0x0008;
    R[7] = gsu_add(R[7], R[3], 0);
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
    gsu_stb(R[7], (uint8_t)R[0]);
    R[7] = 0x000A;
    R[7] = gsu_add(R[7], R[3], 0);
    gsu_stw(R[7], R[0]);
    R[4] = 0x0004;
    R[0] = 0x0001;
    R[0] = gsu_and(R[0], R[12]);
    t = gsu.z;
    if (t) goto lB84B;
    R[4] = 0x000E;
lB84B:
    R[7] = 0x0002;
    R[7] = gsu_add(R[7], R[3], 0);
    gsu_stb(R[7], (uint8_t)R[4]);
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
    gsu_stb(R[3], (uint8_t)R[0]);
    R[0] = 0x0000;
    R[7] = 0x0001;
    R[7] = gsu_add(R[7], R[3], 0);
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
    gsu_stb(R[7], (uint8_t)R[0]);
    goto lB763;
lB8C3:
    R[0] = gsu_swap(R[2]);
    R[0] = gsu_ror(R[0]);
    R[0] = gsu_add(R[0], R[2], 0);
    R[2] = gsu_add(R[0], R[2], gsu.cy);
    R[7] = 0x0007;
    R[0] = gsu_and(R[0], R[7]);
    R[0] = gsu_sub(R[6], R[0], 0);
    gsu_stb(R[3], (uint8_t)R[0]);
    R[0] = 0x0002;
    R[7] = 0x0001;
    R[7] = gsu_add(R[7], R[3], 0);
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
    gsu_stb(R[7], (uint8_t)R[0]);
    goto lB763;
lB92F:
    R[0] = gsu_swap(R[2]);
    R[0] = gsu_ror(R[0]);
    R[0] = gsu_add(R[0], R[2], 0);
    R[2] = gsu_add(R[0], R[2], gsu.cy);
    R[7] = 0x0007;
    R[0] = gsu_and(R[0], R[7]);
    R[0] = gsu_sub(R[6], R[0], 0);
    gsu_stb(R[3], (uint8_t)R[0]);
    R[0] = 0x0004;
    R[7] = 0x0001;
    R[7] = gsu_add(R[7], R[3], 0);
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
    gsu_stb(R[7], (uint8_t)R[0]);
    goto lB763;
lB99B:
    goto lB763;
lB99F:
    goto lB763;
lB9A3:
    R[0] = gsu_and(R[9], 15);
    R[0] = gsu_add(R[6], R[0], 0);
    R[0] = gsu_sub(R[0], 7, 0);
    R[7] = 0x0000;
    R[7] = gsu_add(R[7], R[3], 0);
    gsu_stb(R[7], (uint8_t)R[0]);
    R[0] = 0x0010;
    R[7] = 0x0001;
    R[7] = gsu_add(R[7], R[3], 0);
    gsu_stb(R[7], (uint8_t)R[0]);
    R[0] = R[9];
    R[0] = gsu_dec(R[0]);
    R[0] = gsu_add(R[0], R[0], 0);
    R[7] = 0xD0DC;
    gsu_r14(gsu_add(R[0], R[7], 0));
    R[0] = gsu_getbs();
    gsu_r14(gsu_inc(R[14]));
    R[7] = 0x0003;
    R[7] = gsu_add(R[7], R[3], 0);
    gsu_stb(R[7], (uint8_t)R[0]);
    R[7] = 0x0006;
    R[7] = gsu_add(R[7], R[3], 0);
    gsu_stw(R[7], R[0]);
    R[0] = gsu_getbs();
    R[7] = 0x0004;
    R[7] = gsu_add(R[7], R[3], 0);
    gsu_stb(R[7], (uint8_t)R[0]);
    R[7] = 0x0008;
    R[7] = gsu_add(R[7], R[3], 0);
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
    gsu_stb(R[7], (uint8_t)R[0]);
    R[7] = 0x000A;
    R[7] = gsu_add(R[7], R[3], 0);
    gsu_stw(R[7], R[0]);
    R[4] = 0x0004;
    R[0] = 0x0001;
    R[0] = gsu_and(R[0], R[12]);
    t = gsu.z;
    if (t) goto lBA08;
    R[4] = 0x000E;
lBA08:
    R[7] = 0x0002;
    R[7] = gsu_add(R[7], R[3], 0);
    gsu_stb(R[7], (uint8_t)R[4]);
    goto lB763;
    /* FxWeatherReset */
lBC35:
    gsu_stw(FX_INIT_2B94, R[0]);
    gsu_stw(R[10], R[11]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[0] = gsu_sub(R[0], R[0], 0);
    R[2] = 0x0050;
    R[1] = 0x2864;
    R[3] = 0x0000;
    R[4] = 0x003F;
    /* CACHE */
lBC4A:
    R[0] = R[3];
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_stb(R[1], (uint8_t)R[0]);
    R[1] = gsu_inc(R[1]);
    R[3] = gsu_inc(R[3]);
    R[3] = gsu_and(R[3], R[4]);
    R[0] = gsu_sub(R[0], R[0], 0);
    R[12] = 0x0009;
    R[13] = 0xBC5A;
lBC5A:
    gsu_stb(R[1], (uint8_t)R[0]);
    t = gsu_count_down(&R[12]);
    R[1] = gsu_inc(R[1]);
    if (t) goto lBC5A;
    R[2] = gsu_dec(R[2]);
    t = !gsu.z;
    if (t) goto lBC4A;
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_stw(FX_WEATHER_TURN, R[0]);
    gsu_stw(FX_WEATHER_DRIFT, R[0]);
    R[1] = 0x2688;
    R[12] = 0x0030;
    R[13] = 0xBC73;
lBC73:
    gsu_stw(R[1], R[0]);
    R[1] = gsu_inc(R[1]);
    R[1] = gsu_inc(R[1]);
    t = gsu_count_down(&R[12]);
    if (t) goto lBC73;
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
    /* FxWeatherDraw */
lBC7D:
    R[3] = gsu_ldw(FX_INIT_2B94);
    gsu_sub(R[0], R[3], 0);   /* CMP */
    t = !gsu.z;
    if (t) goto lBC35;
    R[3] = 0x0001;
    gsu_sub(R[0], R[3], 0);   /* CMP */
    t = gsu.z;
    if (t) goto lBC97;
    R[3] = 0x0002;
    gsu_sub(R[0], R[3], 0);   /* CMP */
    t = gsu.z;
    if (t) goto lBC97;
    nx = (uint16_t)(R[11]);
    at = nx;
    goto dispatch;
lBC97:
    gsu_stw(R[10], R[11]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[7] = gsu_ldw(FX_EFFECT_SEED);
    R[12] = R[1];
    gsu_moves(R[1]);
    t = !gsu.z;
    if (t) goto lBCA7;
    goto lBF65;
lBCA7:
    R[11] = R[0];
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu.rambr = (uint8_t)(R[0] & 1);
    R[0] = 0x0014;
    gsu_romb(R[0]);
    R[0] = 0x0001;
    gsu_cmode(R[0]);
    R[0] = gsu_sub(R[11], 1, 0);
    t = gsu.z;
    if (t) goto lBCD2;
    R[3] = 0x001F;
    R[4] = 0x000F;
    R[5] = 0x0007;
    R[6] = 0x0005;
    R[0] = 0x0020;
    gsu_stw(FX_WEATHER_COLOUR, R[0]);
    R[0] = 0x0064;
    gsu_stw(FX_WEATHER_LIFE, R[0]);
    t = 1;
    if (t) goto lBCE7;
lBCD2:
    R[3] = 0x001F;
    R[4] = 0x003F;
    R[5] = 0x001F;
    R[6] = 0x0028;
    R[0] = 0x002A;
    gsu_stw(FX_WEATHER_COLOUR, R[0]);
    R[0] = 0x001E;
    gsu_stw(FX_WEATHER_LIFE, R[0]);
lBCE7:
    R[2] = gsu_ldw(FX_WEATHER_DRIFT);
    R[0] = gsu_swap(R[7]);
    R[0] = gsu_ror(R[0]);
    R[0] = gsu_add(R[0], R[7], 0);
    R[7] = gsu_add(R[0], R[7], gsu.cy);
    R[1] = gsu_and(R[0], R[3]);
    t = !gsu.z;
    if (t) goto lBCFB;
    R[0] = gsu_hib(R[0]);
    R[0] = gsu_and(R[0], R[4]);
    R[2] = gsu_sub(R[0], R[5], 0);
lBCFB:
    R[1] = gsu_ldw(FX_WEATHER_DRIFT);
    R[0] = gsu_sub(R[2], R[1], 0);
    R[0] = gsu_div2(R[0]);
    R[0] = gsu_div2(R[0]);
    R[0] = gsu_div2(R[0]);
    R[0] = gsu_add(R[0], R[1], 0);
    gsu_sbk(R[0]);
    R[0] = gsu_ldw(GSU_PARAM_00E0);
    R[1] = gsu_ldw(FX_WEATHER_TURN);
    gsu_stw(FX_WEATHER_TURN, R[0]);
    R[0] = gsu_sub(R[0], R[1], 0);
    R[0] = gsu_div2(R[0]);
    R[0] = gsu_div2(R[0]);
    R[0] = gsu_div2(R[0]);
    R[0] = gsu_div2(R[0]);
    R[0] = gsu_div2(R[0]);
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
    R[1] = gsu_ldw(FX_WEATHER_DRIFT);
    R[0] = gsu_add(R[0], R[1], 0);
    gsu_stw(FX_CLOSING, R[0]);
    gsu_stw(FX_HAZE_X8, R[6]);
    R[1] = gsu_ldw(GSU_TIED_22);
    R[0] = 0x004A;
    R[0] = gsu_add(R[0], R[1], 0);
    R[0] = gsu_ldw(R[0]);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = !gsu.s;
    if (t) goto lBD40;
    goto lBF65;
lBD40:
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
    R[0] = gsu_div2(R[0]);
    R[0] = gsu_div2(R[0]);
    R[0] = gsu_div2(R[0]);
    gsu_stw(FX_ME_Z, R[0]);
    R[8] = 0x2864;
    /* CACHE */
    R[13] = 0xBD52;
lBD52:
    R[0] = gsu_ldb(R[8]);
    R[0] = gsu_sex(R[0]);
    t = gsu.z;
    R[0] = gsu_dec(R[0]);
    if (t) goto lBD66;
    t = !gsu.s;
    if (t) goto lBDAA;
    R[0] = gsu_inc(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_stb(R[8], (uint8_t)R[0]);
    R[8] = gsu_add(R[8], 10, 0);
    goto lBF2A;
lBD66:
    R[0] = gsu_ldw(FX_WEATHER_LIFE);
    gsu_stb(R[8], (uint8_t)R[0]);
    R[8] = gsu_inc(R[8]);
    R[0] = gsu_swap(R[7]);
    R[0] = gsu_ror(R[0]);
    R[0] = gsu_add(R[0], R[7], 0);
    R[7] = gsu_add(R[0], R[7], gsu.cy);
    R[0] = gsu_ldw(FX_WEATHER_COLOUR);
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
    gsu_stw(R[8], R[0]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    R[1] = gsu_ldw(FX_MODEL_36);
    R[0] = gsu_div2(R[3]);
    R[0] = gsu_add(R[0], R[1], 0);
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_stw(R[8], R[0]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    gsu_stw(R[8], R[3]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_stw(R[8], R[0]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    goto lBF2A;
lBDAA:
    gsu_stb(R[8], (uint8_t)R[0]);
    R[8] = gsu_inc(R[8]);
    R[0] = gsu_ldb(R[8]);
    R[8] = gsu_inc(R[8]);
    gsu_color((uint8_t)R[0]);
    R[6] = 0x01FF;
    R[1] = gsu_ldw(FX_CLOSING);
    R[0] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    R[0] = gsu_add(R[0], R[1], 0);
    t = !gsu.s;
    if (t) goto lBDC9;
    R[0] = gsu_add(R[0], R[6], 0);
    t = !gsu.s;
    R[0] = gsu_sub(R[0], R[6], 0);
    if (t) goto lBDDD;
    R[0] = R[6];
    R[0] = gsu_dec(R[0]);
    t = 1;
    if (t) goto lBDD1;
lBDC9:
    R[0] = gsu_sub(R[0], R[6], 0);
    t = gsu.s;
    R[0] = gsu_add(R[0], R[6], 0);
    if (t) goto lBDDD;
    R[0] = R[6];
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
lBDD1:
    gsu_sbk(R[0]);
    R[8] = gsu_add(R[8], 4, 0);
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_stw(R[8], R[0]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    goto lBF2A;
lBDDD:
    gsu_sbk(R[0]);
    R[9] = R[0];
    R[6] = 0x00FF;
    R[1] = gsu_ldw(FX_HAZE_X8);
    R[0] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    R[0] = gsu_add(R[0], R[1], 0);
    t = !gsu.s;
    if (t) goto lBDF8;
    R[0] = gsu_add(R[0], R[6], 0);
    t = !gsu.s;
    R[0] = gsu_sub(R[0], R[6], 0);
    if (t) goto lBE0C;
    R[0] = R[6];
    R[0] = gsu_dec(R[0]);
    t = 1;
    if (t) goto lBE00;
lBDF8:
    R[0] = gsu_sub(R[0], R[6], 0);
    t = gsu.s;
    R[0] = gsu_add(R[0], R[6], 0);
    if (t) goto lBE0C;
    R[0] = R[6];
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
lBE00:
    gsu_sbk(R[0]);
    R[8] = gsu_add(R[8], 2, 0);
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_stw(R[8], R[0]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    goto lBF2A;
lBE0C:
    gsu_sbk(R[0]);
    R[5] = R[0];
    R[6] = 0x01FF;
    R[1] = gsu_ldw(FX_ME_Z);
    R[0] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    R[0] = gsu_add(R[0], R[1], 0);
    t = !gsu.s;
    if (t) goto lBE22;
    R[0] = R[6];
    t = 1;
    if (t) goto lBE28;
lBE22:
    gsu_sub(R[0], R[6], 0);   /* CMP */
    t = gsu.s;
    if (t) goto lBE31;
    R[0] = gsu_sub(R[0], R[0], 0);
lBE28:
    gsu_sbk(R[0]);
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_stw(R[8], R[0]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    goto lBF2A;
lBE31:
    gsu_sbk(R[0]);
    R[4] = R[0];
    R[6] = 0x012C;
    R[6] = gsu_add(R[6], R[4], 0);
    R[6] = gsu_add(R[6], R[6], 0);
    R[0] = 0x896C;
    gsu_r14(gsu_add(R[0], R[6], 0));
    R[6] = gsu_getb();
    gsu_r14(gsu_inc(R[14]));
    R[6] = gsu_getbh(R[6]);
    R[9] = gsu_fmult(R[9], R[6]);
    R[9] = gsu_rol(R[9]);
    R[1] = gsu_ldw(FX_MODEL_34);
    R[1] = gsu_add(R[1], R[9], 0);
    R[0] = gsu_ldw(FX_SCREEN_LEFT);
    gsu_sub(R[0], R[1], 0);   /* CMP */
    t = gsu.s;
    if (t) goto lBE5B;
    goto lBF4D;
lBE5B:
    R[0] = gsu_ldw(FX_SCREEN_RIGHT);
    R[0] = gsu_sub(R[0], 2, 0);
    gsu_sub(R[0], R[1], 0);   /* CMP */
    t = !gsu.s;
    if (t) goto lBE69;
    goto lBF4D;
lBE69:
    R[5] = gsu_fmult(R[5], R[6]);
    R[5] = gsu_rol(R[5]);
    R[2] = gsu_ldw(FX_MODEL_36);
    R[0] = 0x0014;
    R[2] = gsu_sub(R[2], R[0], 0);
    R[2] = gsu_add(R[2], R[5], 0);
    R[0] = gsu_ldw(FX_SCREEN_TOP);
    gsu_sub(R[0], R[2], 0);   /* CMP */
    t = gsu.s;
    if (t) goto lBE83;
    goto lBF4D;
lBE83:
    R[0] = gsu_ldw(FX_SCREEN_BOTTOM);
    R[0] = gsu_sub(R[0], 4, 0);
    gsu_sub(R[0], R[2], 0);   /* CMP */
    t = !gsu.s;
    if (t) goto lBE92;
    goto lBF53;
lBE92:
    R[0] = gsu_sub(R[11], 1, 0);
    t = gsu.z;
    if (t) goto lBEB2;
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    R[0] = 0x0100;
    gsu_sub(R[0], R[4], 0);   /* CMP */
    t = !gsu.s;
    if (t) goto lBEA8;
    gsu_plot_r1();
    goto lBF2A;
lBEA8:
    gsu_plot_r1();
    gsu_plot_r1();
    R[2] = gsu_inc(R[2]);
    R[1] = gsu_dec(R[1]);
    R[1] = gsu_dec(R[1]);
    gsu_plot_r1();
    gsu_plot_r1();
    t = 1;
    if (t) goto lBF2A;
lBEB2:
    R[0] = gsu_ldw(R[8]);
    R[3] = gsu_lob(R[0]);
    R[9] = gsu_hib(R[0]);
    gsu_stb(R[8], (uint8_t)R[1]);
    R[8] = gsu_inc(R[8]);
    gsu_stb(R[8], (uint8_t)R[2]);
    R[8] = gsu_inc(R[8]);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.z;
    if (t) goto lBF2A;
    gsu_stw(R[10], R[12]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_stw(R[10], R[13]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_stw(R[10], R[8]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_stw(R[10], R[7]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[5] = 0x0001;
    R[3] = gsu_sub(R[3], R[1], 0);
    t = !gsu.s;
    if (t) goto lBEE0;
    R[5] = 0xFFFF;
    R[3] = gsu_not(R[3]);
    R[3] = gsu_inc(R[3]);
lBEE0:
    R[6] = 0x0001;
    R[9] = gsu_sub(R[9], R[2], 0);
    t = !gsu.s;
    if (t) goto lBEEC;
    R[6] = 0xFFFF;
    R[9] = gsu_not(R[9]);
    R[9] = gsu_inc(R[9]);
lBEEC:
    R[0] = gsu_sub(R[3], R[9], 0);
    t = gsu.s;
    if (t) goto lBF06;
    R[12] = R[3];
    R[12] = gsu_inc(R[12]);
    R[0] = gsu_lsr(R[0]);
    R[0] = gsu_sub(R[0], R[9], 0);
    R[13] = 0xBEF8;
lBEF8:
    t = gsu.cy;
    gsu_plot_r1();
    if (t) goto lBEFE;
    R[0] = gsu_add(R[0], R[3], 0);
    R[2] = gsu_add(R[2], R[6], 0);
lBEFE:
    R[1] = gsu_dec(R[1]);
    R[1] = gsu_add(R[1], R[5], 0);
    t = gsu_count_down(&R[12]);
    R[0] = gsu_sub(R[0], R[9], 0);
    if (t) goto lBEF8;
    t = 1;
    if (t) goto lBF1A;
lBF06:
    R[12] = R[9];
    R[12] = gsu_inc(R[12]);
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
    R[0] = gsu_lsr(R[0]);
    R[0] = gsu_sub(R[0], R[3], 0);
    R[13] = 0xBF0F;
lBF0F:
    t = gsu.cy;
    gsu_plot_r1();
    if (t) goto lBF15;
    R[0] = gsu_add(R[0], R[9], 0);
    R[1] = gsu_add(R[1], R[5], 0);
lBF15:
    R[1] = gsu_dec(R[1]);
    R[2] = gsu_add(R[2], R[6], 0);
    t = gsu_count_down(&R[12]);
    R[0] = gsu_sub(R[0], R[3], 0);
    if (t) goto lBF0F;
lBF1A:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    R[7] = gsu_ldw(R[10]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    R[8] = gsu_ldw(R[10]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    R[13] = gsu_ldw(R[10]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    R[12] = gsu_ldw(R[10]);
lBF2A:
    t = gsu_count_down(&R[12]);
    if (t) {
        at = R[13];   /* the drop loop's 01:BD52, pulled back after a streak */
        goto dispatch;
    }
    t = 1;
    if (t) goto lBF65;
    /* FxWeatherUnused */
lBF2F:
    t = 1;
    if (t) goto lBF2A;
    t = 1;
    if (t) goto lBF2A;
    R[2] = 0x0000;
    t = 1;
    if (t) goto lBF3E;
    R[2] = 0x01FF;
lBF3E:
    R[1] = R[8];
    R[8] = gsu_sub(R[8], 10, 0);
    R[8] = gsu_add(R[8], 6, 0);
    gsu_stw(R[8], R[2]);
    R[8] = R[1];
    t = 1;
    if (t) goto lBF2A;
    /* FxWeatherDropNext */
lBF4D:
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_stw(R[8], R[0]);
    R[8] = gsu_inc(R[8]);
    t = 1;
    R[8] = gsu_inc(R[8]);
    if (t) goto lBF2A;
lBF53:
    R[1] = R[8];
    R[8] = gsu_sub(R[8], 8, 0);
    R[8] = gsu_add(R[8], 0, 0);
    R[0] = 0x0001;
    gsu_stb(R[8], (uint8_t)R[0]);
    R[8] = R[1];
    R[8] = gsu_inc(R[8]);
    t = 1;
    R[8] = gsu_inc(R[8]);
    if (t) goto lBF2A;
lBF65:
    gsu_stw(FX_EFFECT_SEED, R[7]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
dispatch:
    if (at == ret)
        return;
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
 * @routine 01:B7E6 FxEffectsAddUnused
 * WHAT IT DOES: Code no jump or branch reaches: after kind 6's set-up
 * (which ends by jumping to 01:B763) a second copy of a kind's set-up,
 * its speeds halved differently (not reached in the traces so far, and
 * never: nothing in bank 01 jumps or branches here).
 * HOW: As kind 6 at 01:B774, ending at 01:B855.
 * Entry: none (unreachable).
 * Exit: none.
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
 * WHAT IT DOES: Code no jump or branch reaches: branches back into
 * FxWeatherDraw's drop loop and the setting of a drop's word to 0 or
 * $01FF (not reached in the traces so far, and never: nothing in bank
 * 01 jumps or branches here).
 * HOW: BRA to 01:BF2A and two IWT R2 stores, ending at 01:BF4C.
 * Entry: none (unreachable).
 * Exit: none.
 */
void abi_FxWeatherUnused(void)
{
    frame_run(0xBF2F, R[11]);
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
