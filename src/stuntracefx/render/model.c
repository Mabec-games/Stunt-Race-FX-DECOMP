/*
 * model.c: the Super FX's 3D model interpreter (01:8561): a model is a list
 * of commands in ROM, each a byte that indexes the IWT R15 table at 01:8000
 * (the code runs it through the bank's mirror at 01:00xx: TO R15; GETB
 * jumps to 01:00cc, whose IWT R15 goes to the command's code). Commands
 * transform the model's points (into words at $0340 on), project them,
 * queue faces in the draw list (raster.c) and draw it, and steer the list:
 * frame choice, jumps, calls and returns, tests of the face-visibility
 * flags at $07C6, and a back-to-front sort. Command code returns by
 * pulling a command address from the R10 stack ($0056 keeps a second
 * stack of list returns).
 *
 * The C was drafted instruction for instruction from the ROM bytes and is
 * kept so: every register, flag and RAM access as the code makes them,
 * one per statement, a branch's delay slot after its decision. The code's
 * jumps through registers (the command table, pulled return addresses)
 * go through model_run's dispatch on the address they reach; at the
 * caller's return address it returns. Labels are the code's addresses.
 */
#include "gsu.h"
#include "gsu_ops.h"

#define R gsu.r

#define FX_MODEL_STACK         0x0012u  /* R10 kept while a point loop uses it */
#define FX_MODEL_AT            0x0016u  /* the model's commands in ROM (R14) */
#define FX_MODEL_18            0x0018u
#define FX_ROM_R14             0x001Au
#define FX_ROM_BANK            0x001Cu
#define FX_POINT_OUT           0x001Eu  /* where the next transformed point goes ($0340 on) */
#define FX_OBJECT_ANGLE_X      0x0020u  /* the object's angles (bytes), negated in place */
#define FX_OBJECT_ANGLE_Y      0x0022u
#define FX_OBJECT_ANGLE_Z      0x0024u
#define FX_OBJECT_X            0x0026u
#define FX_OBJECT_Y            0x0028u
#define FX_OBJECT_Z            0x002Au
#define FX_POINT_SCALE         0x0030u  /* 2 to the power $0032: the model's point scale */
#define FX_POINT_SHIFT         0x0032u
#define FX_MODEL_34            0x0034u
#define FX_MODEL_36            0x0036u
#define FX_SCREEN_LEFT         0x0038u
#define FX_SCREEN_RIGHT        0x003Au
#define FX_SCREEN_TOP          0x003Cu
#define FX_SCREEN_BOTTOM       0x003Eu
#define FX_VERTEX_COUNT        0x0040u
#define FX_FACE_KIND           0x0042u  /* the face being drawn: 0 flat, negative textured */
#define FX_MODEL_FRAME         0x0044u  /* the animation frame the model's frame lists pick from */
#define FX_MATERIAL_MASK       0x0046u
#define FX_MATERIAL_TABLE      0x004Au
#define FX_SHADE_TABLE         0x004Cu
#define FX_SHADE_BASE          0x004Eu
#define FX_MODEL_LIST          0x0050u
#define FX_SHADE_AT            0x0052u
#define FX_MODEL_FLAGS         0x0054u
#define FX_MODEL_RETURN_AT     0x0056u  /* the stack of sub-list returns ($0788 on) */
#define FX_FACE_COLOUR         0x0058u
#define FX_POLY_OUT            0x005Au
#define FX_LIST_END            0x005Cu
#define FX_UV_AT               0x0088u  /* the next u, v pair for a textured face */
#define FX_MODEL_96            0x0096u
#define FX_VERTEX_COUNT_SAVE   0x0098u
#define FX_TEX_BANK            0x00A6u
#define FX_TEX_BASE            0x00A8u
#define FX_MODEL_BA            0x00BAu
#define FX_MODEL_BC            0x00BCu
#define FX_MODEL_BE            0x00BEu
#define FX_MODEL_C8            0x00C8u
#define FX_MODEL_CE            0x00CEu
#define FX_MODEL_D2            0x00D2u
#define FX_TEX_MASK            0x00D4u
#define FX_OBJECT_MATRIX_0     0x00E4u  /* the object's rotation, row by row */
#define FX_OBJECT_MATRIX_1     0x00E6u
#define FX_OBJECT_MATRIX_2     0x00E8u
#define FX_OBJECT_MATRIX_3     0x00EAu
#define FX_OBJECT_MATRIX_4     0x00ECu
#define FX_OBJECT_MATRIX_5     0x00EEu
#define FX_OBJECT_MATRIX_6     0x00F0u
#define FX_OBJECT_MATRIX_7     0x00F2u
#define FX_OBJECT_MATRIX_8     0x00F4u
#define FX_LIGHT_X             0x0100u  /* the light direction */
#define FX_LIGHT_Y             0x0102u
#define FX_LIGHT_Z             0x0104u
#define FX_LIGHT_LOCAL_X       0x0106u  /* the light turned into the object */
#define FX_LIGHT_LOCAL_Y       0x0108u
#define FX_LIGHT_LOCAL_Z       0x010Au
#define FX_MODEL_118           0x0118u
#define FX_MODEL_11E           0x011Eu
#define FX_MODEL_124           0x0124u
#define FX_MATRIX_PACKED_0     0x0128u  /* the view matrix high bytes, two to a word */
#define FX_MATRIX_PACKED_1     0x012Au
#define FX_MATRIX_PACKED_2     0x012Cu
#define FX_MATRIX_PACKED_3     0x012Eu
#define FX_MATRIX_PACKED_4     0x0130u
#define FX_VIEW_MATRIX_0       0x0132u  /* the rotation the points are turned by */
#define FX_VIEW_MATRIX_1       0x0134u
#define FX_VIEW_MATRIX_2       0x0136u
#define FX_VIEW_MATRIX_3       0x0138u
#define FX_VIEW_MATRIX_4       0x013Au
#define FX_VIEW_MATRIX_5       0x013Cu
#define FX_VIEW_MATRIX_6       0x013Eu
#define FX_VIEW_MATRIX_7       0x0140u
#define FX_VIEW_MATRIX_8       0x0142u
#define FX_POINTS_TOTAL        0x0144u
#define FX_POINT_COUNT         0x0146u
#define FX_MODEL_148           0x0148u
#define FX_OUTCODES            0x014Au
#define FX_MODEL_14E           0x014Eu
#define FX_MODEL_150           0x0150u
#define FX_MODEL_1B4           0x01B4u
#define FX_MODEL_1B6           0x01B6u
#define FX_CAR_108C            0x108Cu

void abi_FxDivideSigned(void);
void abi_FxSegmentCrossZ(void);
void abi_FxProjectVertex(void);
void abi_FxProjectPoint(void);
void abi_FxMatrixFromAngles01(void);
void abi_FxMatrixTranspose(void);
void abi_FxMatrixApplyVectors(void);
void abi_FxPolygonGather(void);
void abi_FxPolygonClipScreen(void);
void abi_FxPolygonDraw(void);
void abi_FxDrawList(void);

/* The command table at 01:8000: IWT R15, #code; NOP, four bytes a
   command. */
static const uint16_t commands[26] = {
    0x9243, 0x8A97, 0x886C, 0x8BAA, 0x8EA7, 0x90CD, 0x884A, 0x884F,
    0x8863, 0x884A, 0x8E19, 0x884A, 0x93A7, 0x88F3, 0x8B0E, 0x8DF6,
    0x8E14, 0x8E06, 0x884A, 0x886C, 0x8F18, 0x8F05, 0x8D4D, 0x8D7A,
    0x8D5D, 0x8D75,
};

/* Where command byte cmd goes: the table's entry; 0 for a byte that is
   not a command (the code would run into the middle of the table). */
static uint16_t model_command(uint16_t cmd)
{
    if ((cmd & 3) || cmd >= 4 * 26)
        return 0;
    return commands[cmd >> 2];
}

/* The code from address at until it jumps to ret (the caller's return). */
static void model_run(uint16_t at, uint16_t ret)
{
    int t;
    uint16_t nx;

    (void)t;
    (void)nx;
    goto enter;   /* the caller fetched the first byte */
    /* FxModelDraw */
l8561:
    gsu_code(0x018562, 3);
    R[0] = gsu_ldw(FX_MODEL_1B4);
    R[0] = gsu_lob(R[0]);
    t = gsu.z;
    gsu_code(0x018565, 3);
    if (t) { gsu_code(0x018584, 1); goto l8584; }
    gsu_code(0x018568, 3);
    gsu_stw(R[10], R[11]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[11] = 0x8571;
    gsu_code(0x01856b, 6); gsu_code(0x018576, 1);
    goto l8576;
l8571:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x018572, 4);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
    /* FxModelDrawFlagged */
l8576:
    R[2] = 0x1000;
    R[1] = 0x2000;
    gsu_code(0x018577, 9);
    R[0] = gsu_ldw(FX_MODEL_FLAGS);
    R[0] = gsu_and(R[0], R[2]);
    R[0] = gsu_or(R[0], R[1]);
    t = 1;
    gsu_code(0x018580, 4);
    gsu_code(0x01858f, 1);
    if (t) goto l858F;
l8584:
    gsu_code(0x018585, 2);
    gsu_stw(R[10], R[11]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x018587, 6);
    R[11] = gsu_ldw(R[10]);
    R[0] = 0x8000;
    gsu_code(0x01858d, 3);
l858F:
    gsu_code(0x018590, 3);
    gsu_stw(FX_MODEL_FLAGS, R[0]);
    gsu_code(0x018593, 2);
    gsu_stw(R[10], R[11]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[0] = 0x0003;
    gsu_code(0x018595, 6);
    gsu_romb(R[0]);
    R[0] = 0x08AA;
    gsu_code(0x01859b, 6);
    gsu_stw(FX_LIST_END, R[0]);
    R[0] = 0x0003;
    gsu_code(0x0185a1, 4);
    gsu_romb(R[0]);
    gsu_code(0x0185a5, 3);
    gsu_r14(gsu_ldw(FX_MODEL_LIST));
    gsu_code(0x0185a8, 3);
    R[0] = gsu_ldw(FX_MODEL_1B6);
    R[0] = gsu_lob(R[0]);
    t = gsu.z;
    gsu_code(0x0185ab, 3);
    if (t) { gsu_code(0x0185b6, 1); goto l85B6; }
    R[0] = gsu_dec(R[0]);
    R[0] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_add(R[0], R[0], 0);
    R[2] = 0xAE45;
    gsu_code(0x0185ae, 9);
    gsu_r14(gsu_add(R[0], R[2], 0));
l85B6:
    gsu_code(0x0185b7, 4);
    R[0] = gsu_ldw(FX_CAR_108C);
    R[0] = gsu_lob(R[0]);
    t = gsu.z;
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_code(0x0185bb, 3);
    if (t) { gsu_code(0x018609, 1); goto l8609; }
    gsu_code(0x0185be, 4);
    R[1] = gsu_ldw(FX_OBJECT_Z);
    gsu_code(0x0185c2, 2);
    R[0] = gsu_getbh(R[0]);
    gsu_code(0x0185c4, 1);
    gsu_r14(gsu_inc(R[14]));
    R[0] = gsu_add(R[0], R[1], 0);
    t = !gsu.s;
    gsu_code(0x0185c5, 3);
    if (t) { gsu_code(0x0185d2, 1); goto l85D2; }
    R[0] = 0xA275;
    R[2] = 0x0000;
    gsu_code(0x0185c8, 10); gsu_code(0x0185fc, 1);
    goto l85FC;
l85D2:
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_code(0x0185d3, 3);
    R[0] = gsu_getbh(R[0]);
    gsu_code(0x0185d6, 1);
    gsu_r14(gsu_inc(R[14]));
    R[0] = gsu_add(R[0], R[1], 0);
    t = !gsu.s;
    gsu_code(0x0185d7, 3);
    if (t) { gsu_code(0x0185e4, 1); goto l85E4; }
    R[0] = 0xA2A1;
    R[2] = 0x0088;
    gsu_code(0x0185da, 10); gsu_code(0x0185fc, 1);
    goto l85FC;
l85E4:
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_code(0x0185e5, 3);
    R[0] = gsu_getbh(R[0]);
    gsu_code(0x0185e8, 1);
    gsu_r14(gsu_inc(R[14]));
    R[0] = gsu_add(R[0], R[1], 0);
    t = !gsu.s;
    gsu_code(0x0185e9, 3);
    if (t) { gsu_code(0x0185f6, 1); goto l85F6; }
    R[0] = 0xA2CD;
    R[2] = 0x0110;
    gsu_code(0x0185ec, 10); gsu_code(0x0185fc, 1);
    goto l85FC;
l85F6:
    R[0] = 0xA2F9;
    R[2] = 0x0198;
    gsu_code(0x0185f7, 6);
l85FC:
    gsu_code(0x0185fd, 3);
    gsu_stw(FX_SHADE_TABLE, R[0]);
    gsu_code(0x018600, 3);
    R[0] = gsu_ldw(FX_SHADE_BASE);
    R[0] = gsu_add(R[0], R[2], 0);
    gsu_code(0x018603, 4);
    gsu_stw(FX_SHADE_AT, R[0]);
    t = 1;
    gsu_code(0x018607, 2);
    gsu_code(0x018651, 1); gsu_cache(0x8652);
    if (t) goto l8651;
l8609:
    gsu_code(0x01860a, 3);
    R[1] = gsu_ldw(FX_OBJECT_Z);
    gsu_code(0x01860d, 2);
    R[0] = gsu_getbh(R[0]);
    gsu_code(0x01860f, 1);
    gsu_r14(gsu_inc(R[14]));
    R[0] = gsu_add(R[0], R[1], 0);
    t = !gsu.s;
    gsu_code(0x018610, 3);
    if (t) { gsu_code(0x01861d, 1); goto l861D; }
    R[0] = 0xA1C5;
    R[2] = 0x0000;
    gsu_code(0x018613, 10); gsu_code(0x018647, 1);
    goto l8647;
l861D:
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_code(0x01861e, 3);
    R[0] = gsu_getbh(R[0]);
    gsu_code(0x018621, 1);
    gsu_r14(gsu_inc(R[14]));
    R[0] = gsu_add(R[0], R[1], 0);
    t = !gsu.s;
    gsu_code(0x018622, 3);
    if (t) { gsu_code(0x01862f, 1); goto l862F; }
    R[0] = 0xA1F1;
    R[2] = 0x0088;
    gsu_code(0x018625, 10); gsu_code(0x018647, 1);
    goto l8647;
l862F:
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_code(0x018630, 3);
    R[0] = gsu_getbh(R[0]);
    gsu_code(0x018633, 1);
    gsu_r14(gsu_inc(R[14]));
    R[0] = gsu_add(R[0], R[1], 0);
    t = !gsu.s;
    gsu_code(0x018634, 3);
    if (t) { gsu_code(0x018641, 1); goto l8641; }
    R[0] = 0xA21D;
    R[2] = 0x0110;
    gsu_code(0x018637, 10); gsu_code(0x018647, 1);
    goto l8647;
l8641:
    R[0] = 0xA249;
    R[2] = 0x0198;
    gsu_code(0x018642, 6);
l8647:
    gsu_code(0x018648, 3);
    gsu_stw(FX_SHADE_TABLE, R[0]);
    gsu_code(0x01864b, 3);
    R[0] = gsu_ldw(FX_SHADE_BASE);
    R[0] = gsu_add(R[0], R[2], 0);
    gsu_code(0x01864e, 4); gsu_cache(0x8652);
    gsu_stw(FX_SHADE_AT, R[0]);
l8651:
    /* CACHE */
    R[0] = 0x0001;
    gsu_code(0x018652, 6);
    R[12] = gsu_ldw(FX_POINT_SHIFT);
    R[12] = R[12];
    gsu_moves(R[12]);
    t = gsu.z;
    gsu_code(0x018658, 4);
    if (t) { gsu_code(0x018660, 1); goto l8660; }
    R[13] = 0x865E;
    gsu_code(0x01865c, 3);
l865E:
    t = gsu_count_down(&R[12]);
    R[0] = gsu_add(R[0], R[0], 0);
    gsu_code(0x01865f, 1);
    if (t) { gsu_code(0x01865e, 1); goto l865E; }
    gsu_code(0x018660, 1);
l8660:
    gsu_code(0x018661, 3);
    gsu_stw(FX_POINT_SCALE, R[0]);
    gsu_code(0x018664, 3);
    R[1] = gsu_ldw(FX_OBJECT_ANGLE_X);
    R[0] = gsu_lob(R[1]);
    R[0] = gsu_swap(R[0]);
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_code(0x018667, 6);
    gsu_sbk(R[0]);
    gsu_code(0x01866d, 3);
    R[2] = gsu_ldw(FX_OBJECT_ANGLE_Y);
    R[0] = gsu_lob(R[2]);
    R[0] = gsu_swap(R[0]);
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_code(0x018670, 6);
    gsu_sbk(R[0]);
    gsu_code(0x018676, 3);
    R[3] = gsu_ldw(FX_OBJECT_ANGLE_Z);
    R[0] = gsu_lob(R[3]);
    R[0] = gsu_swap(R[0]);
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_code(0x018679, 6);
    gsu_sbk(R[0]);
    R[0] = gsu_or(R[1], R[3]);
    t = gsu.z;
    gsu_code(0x01867f, 4);
    if (t) { gsu_code(0x018687, 1); goto l8687; }
    gsu_code(0x018683, 4); gsu_code(0x018741, 1);
    goto l8741;
l8687:
    R[0] = gsu_or(R[0], R[2]);
    t = !gsu.z;
    gsu_code(0x018688, 3);
    if (t) { gsu_code(0x0186d8, 1); goto l86D8; }
    gsu_code(0x01868b, 4);
    R[0] = gsu_ldw(FX_OBJECT_MATRIX_0);
    gsu_code(0x01868f, 3);
    gsu_stw(FX_VIEW_MATRIX_0, R[0]);
    gsu_code(0x018692, 3);
    R[0] = gsu_ldw(FX_OBJECT_MATRIX_1);
    gsu_code(0x018695, 3);
    gsu_stw(FX_VIEW_MATRIX_1, R[0]);
    gsu_code(0x018698, 3);
    R[0] = gsu_ldw(FX_OBJECT_MATRIX_2);
    gsu_code(0x01869b, 3);
    gsu_stw(FX_VIEW_MATRIX_2, R[0]);
    gsu_code(0x01869e, 3);
    R[0] = gsu_ldw(FX_OBJECT_MATRIX_3);
    gsu_code(0x0186a1, 3);
    gsu_stw(FX_VIEW_MATRIX_3, R[0]);
    gsu_code(0x0186a4, 3);
    R[0] = gsu_ldw(FX_OBJECT_MATRIX_4);
    gsu_code(0x0186a7, 3);
    gsu_stw(FX_VIEW_MATRIX_4, R[0]);
    gsu_code(0x0186aa, 3);
    R[0] = gsu_ldw(FX_OBJECT_MATRIX_5);
    gsu_code(0x0186ad, 3);
    gsu_stw(FX_VIEW_MATRIX_5, R[0]);
    gsu_code(0x0186b0, 3);
    R[0] = gsu_ldw(FX_OBJECT_MATRIX_6);
    gsu_code(0x0186b3, 3);
    gsu_stw(FX_VIEW_MATRIX_6, R[0]);
    gsu_code(0x0186b6, 3);
    R[0] = gsu_ldw(FX_OBJECT_MATRIX_7);
    gsu_code(0x0186b9, 3);
    gsu_stw(FX_VIEW_MATRIX_7, R[0]);
    gsu_code(0x0186bc, 3);
    R[0] = gsu_ldw(FX_OBJECT_MATRIX_8);
    gsu_code(0x0186bf, 3);
    gsu_stw(FX_VIEW_MATRIX_8, R[0]);
    gsu_code(0x0186c2, 3);
    R[0] = gsu_ldw(FX_MODEL_FLAGS);
    R[1] = 0x2000;
    R[0] = gsu_and(R[0], R[1]);
    t = gsu.z;
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_code(0x0186c5, 6);
    if (t) { gsu_code(0x0186d4, 1); goto l86D4; }
    gsu_code(0x0186cb, 4);
    gsu_stw(FX_VIEW_MATRIX_3, R[0]);
    gsu_code(0x0186cf, 3);
    gsu_stw(FX_VIEW_MATRIX_4, R[0]);
    gsu_code(0x0186d2, 3);
    gsu_stw(FX_VIEW_MATRIX_5, R[0]);
l86D4:
    gsu_code(0x0186d5, 3); gsu_code(0x018778, 1);
    goto l8778;
l86D8:
    R[4] = 0x0040;
    R[0] = gsu_sub(R[2], R[4], 0);
    t = !gsu.z;
    gsu_code(0x0186d9, 7);
    if (t) { gsu_code(0x0186e4, 1); goto l86E4; }
    gsu_code(0x0186e0, 4); gsu_code(0x018741, 1);
    goto l8741;
l86E4:
    R[0] = gsu_sub(R[0], R[4], 0);
    t = !gsu.z;
    gsu_code(0x0186e5, 3);
    if (t) { gsu_code(0x018741, 1); goto l8741; }
    gsu_code(0x0186e8, 4);
    R[0] = gsu_ldw(FX_OBJECT_MATRIX_0);
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_code(0x0186ec, 5);
    gsu_stw(FX_VIEW_MATRIX_0, R[0]);
    gsu_code(0x0186f1, 3);
    R[0] = gsu_ldw(FX_OBJECT_MATRIX_1);
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_code(0x0186f4, 5);
    gsu_stw(FX_VIEW_MATRIX_1, R[0]);
    gsu_code(0x0186f9, 3);
    R[0] = gsu_ldw(FX_OBJECT_MATRIX_2);
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_code(0x0186fc, 5);
    gsu_stw(FX_VIEW_MATRIX_2, R[0]);
    gsu_code(0x018701, 3);
    R[0] = gsu_ldw(FX_OBJECT_MATRIX_3);
    gsu_code(0x018704, 3);
    gsu_stw(FX_VIEW_MATRIX_3, R[0]);
    gsu_code(0x018707, 3);
    R[0] = gsu_ldw(FX_OBJECT_MATRIX_4);
    gsu_code(0x01870a, 3);
    gsu_stw(FX_VIEW_MATRIX_4, R[0]);
    gsu_code(0x01870d, 3);
    R[0] = gsu_ldw(FX_OBJECT_MATRIX_5);
    gsu_code(0x018710, 3);
    gsu_stw(FX_VIEW_MATRIX_5, R[0]);
    gsu_code(0x018713, 3);
    R[0] = gsu_ldw(FX_OBJECT_MATRIX_6);
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_code(0x018716, 5);
    gsu_stw(FX_VIEW_MATRIX_6, R[0]);
    gsu_code(0x01871b, 3);
    R[0] = gsu_ldw(FX_OBJECT_MATRIX_7);
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_code(0x01871e, 5);
    gsu_stw(FX_VIEW_MATRIX_7, R[0]);
    gsu_code(0x018723, 3);
    R[0] = gsu_ldw(FX_OBJECT_MATRIX_8);
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_code(0x018726, 5);
    gsu_stw(FX_VIEW_MATRIX_8, R[0]);
    gsu_code(0x01872b, 3);
    R[0] = gsu_ldw(FX_MODEL_FLAGS);
    R[1] = 0x2000;
    R[0] = gsu_and(R[0], R[1]);
    t = gsu.z;
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_code(0x01872e, 6);
    if (t) { gsu_code(0x01873d, 1); goto l873D; }
    gsu_code(0x018734, 4);
    gsu_stw(FX_VIEW_MATRIX_3, R[0]);
    gsu_code(0x018738, 3);
    gsu_stw(FX_VIEW_MATRIX_4, R[0]);
    gsu_code(0x01873b, 3);
    gsu_stw(FX_VIEW_MATRIX_5, R[0]);
l873D:
    gsu_code(0x01873e, 3); gsu_code(0x018778, 1);
    goto l8778;
l8741:
    R[9] = 0x0116;
    R[11] = 0x8749;
    gsu_code(0x018742, 7); gsu_code(0x01832f, 1);
    abi_FxMatrixFromAngles01();
    gsu_code(0x018749, 1);
l8749:
    R[9] = 0x0116;
    R[11] = 0x8751;
    gsu_code(0x01874a, 7); gsu_code(0x01972b, 1);
    abi_FxMatrixTranspose();
    gsu_code(0x018751, 1);
l8751:
    gsu_code(0x018752, 3);
    R[0] = gsu_ldw(FX_MODEL_FLAGS);
    R[1] = 0x2000;
    R[0] = gsu_and(R[0], R[1]);
    t = gsu.z;
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_code(0x018755, 6);
    if (t) { gsu_code(0x018770, 1); goto l8770; }
    gsu_code(0x01875b, 4);
    gsu_stw(FX_MODEL_118, R[0]);
    gsu_code(0x01875f, 3);
    gsu_stw(FX_MODEL_11E, R[0]);
    gsu_code(0x018762, 3);
    gsu_stw(FX_MODEL_124, R[0]);
    R[9] = 0x0132;
    R[11] = 0x876C;
    gsu_code(0x018765, 7); gsu_code(0x01975b, 1);
    abi_FxMatrixApplyVectors();
    gsu_code(0x01876c, 1);
l876C:
    gsu_code(0x01876d, 3); gsu_code(0x018778, 1);
    goto l8778;
l8770:
    R[9] = 0x0132;
    R[11] = 0x8778;
    gsu_code(0x018771, 7); gsu_code(0x01975b, 1);
    abi_FxMatrixApplyVectors();
    gsu_code(0x018778, 1);
l8778:
    R[0] = 0x0000;
    gsu_code(0x018779, 6);
    gsu_stw(FX_LIGHT_X, R[0]);
    R[0] = 0x393E;
    gsu_code(0x01877f, 6);
    gsu_stw(FX_LIGHT_Y, R[0]);
    R[0] = 0x727C;
    gsu_code(0x018785, 6);
    gsu_stw(FX_LIGHT_Z, R[0]);
    gsu_code(0x01878b, 3);
    R[1] = gsu_ldw(FX_LIGHT_X);
    gsu_code(0x01878e, 3);
    R[2] = gsu_ldw(FX_LIGHT_Y);
    gsu_code(0x018791, 3);
    R[3] = gsu_ldw(FX_LIGHT_Z);
    gsu_code(0x018794, 3);
    R[6] = gsu_ldw(FX_VIEW_MATRIX_0);
    gsu_code(0x018797, 2);
    R[0] = gsu_fmult(R[1], R[6]);
    R[5] = gsu_rol(R[0]);
    gsu_code(0x018799, 5);
    R[6] = gsu_ldw(FX_VIEW_MATRIX_1);
    gsu_code(0x01879e, 2);
    R[0] = gsu_fmult(R[2], R[6]);
    R[0] = gsu_rol(R[0]);
    R[5] = gsu_add(R[5], R[0], 0);
    gsu_code(0x0187a0, 6);
    R[6] = gsu_ldw(FX_VIEW_MATRIX_2);
    gsu_code(0x0187a6, 2);
    R[0] = gsu_fmult(R[3], R[6]);
    R[0] = gsu_rol(R[0]);
    R[0] = gsu_add(R[0], R[5], 0);
    R[0] = gsu_hib(R[0]);
    R[0] = gsu_sex(R[0]);
    gsu_code(0x0187a8, 7);
    gsu_stw(FX_LIGHT_LOCAL_X, R[0]);
    gsu_code(0x0187af, 3);
    R[6] = gsu_ldw(FX_VIEW_MATRIX_3);
    gsu_code(0x0187b2, 2);
    R[0] = gsu_fmult(R[1], R[6]);
    R[5] = gsu_rol(R[0]);
    gsu_code(0x0187b4, 5);
    R[6] = gsu_ldw(FX_VIEW_MATRIX_4);
    gsu_code(0x0187b9, 2);
    R[0] = gsu_fmult(R[2], R[6]);
    R[0] = gsu_rol(R[0]);
    R[5] = gsu_add(R[5], R[0], 0);
    gsu_code(0x0187bb, 6);
    R[6] = gsu_ldw(FX_VIEW_MATRIX_5);
    gsu_code(0x0187c1, 2);
    R[0] = gsu_fmult(R[3], R[6]);
    R[0] = gsu_rol(R[0]);
    R[0] = gsu_add(R[0], R[5], 0);
    R[0] = gsu_hib(R[0]);
    R[0] = gsu_sex(R[0]);
    gsu_code(0x0187c3, 7);
    gsu_stw(FX_LIGHT_LOCAL_Y, R[0]);
    gsu_code(0x0187ca, 3);
    R[6] = gsu_ldw(FX_VIEW_MATRIX_6);
    gsu_code(0x0187cd, 2);
    R[0] = gsu_fmult(R[1], R[6]);
    R[5] = gsu_rol(R[0]);
    gsu_code(0x0187cf, 5);
    R[6] = gsu_ldw(FX_VIEW_MATRIX_7);
    gsu_code(0x0187d4, 2);
    R[0] = gsu_fmult(R[2], R[6]);
    R[0] = gsu_rol(R[0]);
    R[5] = gsu_add(R[5], R[0], 0);
    gsu_code(0x0187d6, 6);
    R[6] = gsu_ldw(FX_VIEW_MATRIX_8);
    gsu_code(0x0187dc, 2);
    R[0] = gsu_fmult(R[3], R[6]);
    R[0] = gsu_rol(R[0]);
    R[0] = gsu_add(R[0], R[5], 0);
    R[0] = gsu_hib(R[0]);
    R[0] = gsu_sex(R[0]);
    gsu_code(0x0187de, 7);
    gsu_stw(FX_LIGHT_LOCAL_Z, R[0]);
    gsu_code(0x0187e5, 3);
    R[0] = gsu_ldw(FX_POINT_SHIFT);
    R[0] = gsu_sub(R[0], 3, 0);
    t = !gsu.s;
    gsu_code(0x0187e8, 4);
    if (t) { gsu_code(0x01882c, 1); goto l882C; }
    R[2] = 0x0133;
    gsu_code(0x0187ec, 7);
    R[1] = gsu_ldb(R[2]);
    R[2] = gsu_inc(R[2]);
    R[2] = gsu_inc(R[2]);
    gsu_code(0x0187f3, 4);
    R[0] = gsu_ldb(R[2]);
    R[0] = gsu_swap(R[0]);
    R[0] = gsu_or(R[0], R[1]);
    gsu_code(0x0187f7, 5);
    gsu_stw(FX_MATRIX_PACKED_0, R[0]);
    R[2] = gsu_inc(R[2]);
    R[2] = gsu_inc(R[2]);
    gsu_code(0x0187fc, 5);
    R[1] = gsu_ldb(R[2]);
    R[2] = gsu_inc(R[2]);
    R[2] = gsu_inc(R[2]);
    gsu_code(0x018801, 4);
    R[0] = gsu_ldb(R[2]);
    R[0] = gsu_swap(R[0]);
    R[0] = gsu_or(R[0], R[1]);
    gsu_code(0x018805, 5);
    gsu_stw(FX_MATRIX_PACKED_1, R[0]);
    R[2] = gsu_inc(R[2]);
    R[2] = gsu_inc(R[2]);
    gsu_code(0x01880a, 5);
    R[1] = gsu_ldb(R[2]);
    R[2] = gsu_inc(R[2]);
    R[2] = gsu_inc(R[2]);
    gsu_code(0x01880f, 4);
    R[0] = gsu_ldb(R[2]);
    R[0] = gsu_swap(R[0]);
    R[0] = gsu_or(R[0], R[1]);
    gsu_code(0x018813, 5);
    gsu_stw(FX_MATRIX_PACKED_2, R[0]);
    R[2] = gsu_inc(R[2]);
    R[2] = gsu_inc(R[2]);
    gsu_code(0x018818, 5);
    R[1] = gsu_ldb(R[2]);
    R[2] = gsu_inc(R[2]);
    R[2] = gsu_inc(R[2]);
    gsu_code(0x01881d, 4);
    R[0] = gsu_ldb(R[2]);
    R[0] = gsu_swap(R[0]);
    R[0] = gsu_or(R[0], R[1]);
    gsu_code(0x018821, 5);
    gsu_stw(FX_MATRIX_PACKED_3, R[0]);
    R[2] = gsu_inc(R[2]);
    R[2] = gsu_inc(R[2]);
    gsu_code(0x018826, 4);
    R[0] = gsu_ldb(R[2]);
    gsu_code(0x01882a, 3);
    gsu_stw(FX_MATRIX_PACKED_4, R[0]);
l882C:
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_code(0x01882d, 4);
    gsu_stw(FX_POINTS_TOTAL, R[0]);
    R[0] = 0x0340;
    gsu_code(0x018831, 6);
    gsu_stw(FX_POINT_OUT, R[0]);
    gsu_code(0x018837, 3);
    R[0] = gsu_ldw(FX_ROM_BANK);
    gsu_code(0x01883a, 2);
    gsu_romb(R[0]);
    gsu_code(0x01883c, 3);
    gsu_r14(gsu_ldw(FX_MODEL_AT));
    gsu_code(0x01883f, 3);
    gsu_stw(FX_ROM_R14, R[14]);
    R[0] = 0x9243;
    gsu_code(0x018842, 4);
    gsu_stw(R[10], R[0]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_code(0x018846, 4);
    nx = (uint16_t)(gsu_getb());
    at = model_command(nx);
    gsu_code(0x010000 | at, 1);   /* where it jumps, fetched before the delay slot runs */
    gsu_r14(gsu_inc(R[14]));
    goto fetched;
    /* FxModelReturn */
l884A:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x01884b, 4);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
    /* FxModelFramePick */
l884F:
    gsu_code(0x018850, 2);
    R[1] = gsu_getb();
    gsu_code(0x018852, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x018853, 3);
    R[0] = gsu_ldw(FX_MODEL_FRAME);
    R[2] = 0x003F;
    R[0] = gsu_and(R[0], R[2]);
    gsu_code(0x018856, 3);
l8858:
    gsu_sub(R[0], R[1], 0);   /* CMP */
    t = gsu.s;
    gsu_code(0x018859, 4);
    if (t) { gsu_code(0x018860, 1); goto l8860; }
    t = 1;
    R[0] = gsu_sub(R[0], R[1], 0);
    gsu_code(0x01885d, 3);
    gsu_code(0x018858, 1);
    if (t) goto l8858;
l8860:
    R[0] = gsu_add(R[0], R[0], 0);
    gsu_code(0x018861, 3);
    gsu_r14(gsu_add(R[14], R[0], 0));
    /* FxModelJump */
l8863:
    gsu_code(0x018864, 1);
    R[0] = gsu_getb();
    gsu_code(0x018865, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x018866, 2);
    R[0] = gsu_getbh(R[0]);
    gsu_code(0x018868, 2);
    gsu_r14(gsu_add(R[14], R[0], 0));
    gsu_code(0x01886a, 2);
    nx = (uint16_t)(gsu_getb());
    at = model_command(nx);
    gsu_code(0x010000 | at, 1);   /* where it jumps, fetched before the delay slot runs */
    gsu_r14(gsu_inc(R[14]));
    goto fetched;
    /* FxModelPointsFull */
l886C:
    gsu_code(0x01886d, 2);
    R[12] = gsu_getb();
    gsu_code(0x01886f, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x018870, 3);
    R[0] = gsu_ldw(FX_POINTS_TOTAL);
    R[0] = gsu_add(R[0], R[12], 0);
    gsu_code(0x018873, 2);
    gsu_sbk(R[0]);
    gsu_code(0x018875, 3);
    R[9] = gsu_ldw(FX_POINT_OUT);
    gsu_code(0x018878, 3);
    gsu_stw(FX_MODEL_STACK, R[10]);
    gsu_code(0x01887b, 3);
    R[10] = gsu_ldw(FX_VIEW_MATRIX_0);
    gsu_code(0x01887e, 3);
    R[11] = gsu_ldw(FX_VIEW_MATRIX_3);
    gsu_code(0x018881, 3);
    R[13] = gsu_ldw(FX_VIEW_MATRIX_6);
    gsu_code(0x018884, 3);
    R[4] = gsu_ldw(FX_VIEW_MATRIX_1);
    gsu_code(0x018887, 3);
    R[7] = gsu_ldw(FX_VIEW_MATRIX_4);
    gsu_code(0x01888a, 3); gsu_cache(0x888d);
    R[8] = gsu_ldw(FX_VIEW_MATRIX_7);
    /* CACHE */
    gsu_code(0x01888d, 1);
l888D:
    gsu_code(0x01888e, 2);
    R[1] = gsu_getb();
    gsu_code(0x018890, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x018891, 3);
    R[1] = gsu_getbh(R[1]);
    gsu_code(0x018894, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x018895, 2);
    R[2] = gsu_getb();
    gsu_code(0x018897, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x018898, 3);
    R[2] = gsu_getbh(R[2]);
    gsu_code(0x01889b, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x01889c, 2);
    R[3] = gsu_getb();
    gsu_code(0x01889e, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x01889f, 3);
    R[3] = gsu_getbh(R[3]);
    gsu_code(0x0188a2, 1);
    gsu_r14(gsu_inc(R[14]));
    R[6] = R[10];
    gsu_code(0x0188a3, 4);
    R[0] = gsu_fmult(R[1], R[6]);
    R[5] = gsu_rol(R[0]);
    R[6] = R[11];
    gsu_code(0x0188a7, 6);
    R[0] = gsu_fmult(R[2], R[6]);
    R[0] = gsu_rol(R[0]);
    R[5] = gsu_add(R[5], R[0], 0);
    R[6] = R[13];
    gsu_code(0x0188ad, 7);
    R[0] = gsu_fmult(R[3], R[6]);
    R[0] = gsu_rol(R[0]);
    R[0] = gsu_add(R[0], R[5], 0);
    gsu_code(0x0188b4, 3);
    gsu_stw(R[9], R[0]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    R[6] = R[4];
    gsu_code(0x0188b7, 6);
    R[0] = gsu_fmult(R[1], R[6]);
    R[5] = gsu_rol(R[0]);
    R[6] = R[7];
    gsu_code(0x0188bd, 6);
    R[0] = gsu_fmult(R[2], R[6]);
    R[0] = gsu_rol(R[0]);
    R[5] = gsu_add(R[5], R[0], 0);
    R[6] = R[8];
    gsu_code(0x0188c3, 7);
    R[0] = gsu_fmult(R[3], R[6]);
    R[0] = gsu_rol(R[0]);
    R[0] = gsu_add(R[0], R[5], 0);
    gsu_code(0x0188ca, 3);
    gsu_stw(R[9], R[0]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    gsu_code(0x0188cd, 5);
    R[6] = gsu_ldw(FX_VIEW_MATRIX_2);
    gsu_code(0x0188d2, 2);
    R[0] = gsu_fmult(R[1], R[6]);
    R[5] = gsu_rol(R[0]);
    gsu_code(0x0188d4, 5);
    R[6] = gsu_ldw(FX_VIEW_MATRIX_5);
    gsu_code(0x0188d9, 2);
    R[0] = gsu_fmult(R[2], R[6]);
    R[0] = gsu_rol(R[0]);
    R[5] = gsu_add(R[5], R[0], 0);
    gsu_code(0x0188db, 6);
    R[6] = gsu_ldw(FX_VIEW_MATRIX_8);
    gsu_code(0x0188e1, 2);
    R[0] = gsu_fmult(R[3], R[6]);
    R[0] = gsu_rol(R[0]);
    R[0] = gsu_add(R[0], R[5], 0);
    gsu_code(0x0188e3, 3);
    gsu_stw(R[9], R[0]);
    R[9] = gsu_inc(R[9]);
    R[12] = gsu_dec(R[12]);
    t = !gsu.z;
    R[9] = gsu_inc(R[9]);
    gsu_code(0x0188e6, 4);
    if (t) { gsu_code(0x01888d, 1); goto l888D; }
    gsu_code(0x0188ea, 4);
    gsu_stw(FX_POINT_OUT, R[9]);
    gsu_code(0x0188ee, 3);
    R[10] = gsu_ldw(FX_MODEL_STACK);
    gsu_code(0x0188f1, 2);
    nx = (uint16_t)(gsu_getb());
    at = model_command(nx);
    gsu_code(0x010000 | at, 1);   /* where it jumps, fetched before the delay slot runs */
    gsu_r14(gsu_inc(R[14]));
    goto fetched;
    /* FxModelPointsScaled */
l88F3:
    gsu_code(0x0188f4, 2);
    R[12] = gsu_getb();
    gsu_code(0x0188f6, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x0188f7, 3);
    R[0] = gsu_ldw(FX_POINTS_TOTAL);
    R[0] = gsu_add(R[0], R[12], 0);
    R[0] = gsu_add(R[0], R[12], 0);
    gsu_code(0x0188fa, 3);
    gsu_sbk(R[0]);
    gsu_code(0x0188fd, 3); gsu_cache(0x8900);
    R[9] = gsu_ldw(FX_POINT_OUT);
    /* CACHE */
    R[13] = 0x8902;
    gsu_code(0x018900, 3);
l8902:
    gsu_code(0x018903, 2);
    R[1] = gsu_getb();
    gsu_code(0x018905, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x018906, 3);
    R[1] = gsu_getbh(R[1]);
    gsu_code(0x018909, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x01890a, 2);
    R[2] = gsu_getb();
    gsu_code(0x01890c, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x01890d, 3);
    R[2] = gsu_getbh(R[2]);
    gsu_code(0x018910, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x018911, 2);
    R[3] = gsu_getb();
    gsu_code(0x018913, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x018914, 3);
    R[3] = gsu_getbh(R[3]);
    gsu_code(0x018917, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x018918, 3);
    R[6] = gsu_ldw(FX_VIEW_MATRIX_0);
    gsu_code(0x01891b, 2);
    R[0] = gsu_fmult(R[1], R[6]);
    R[8] = gsu_rol(R[0]);
    gsu_code(0x01891d, 5);
    R[6] = gsu_ldw(FX_VIEW_MATRIX_3);
    gsu_code(0x018922, 2);
    R[0] = gsu_fmult(R[2], R[6]);
    R[5] = gsu_rol(R[0]);
    gsu_code(0x018924, 5);
    R[6] = gsu_ldw(FX_VIEW_MATRIX_6);
    gsu_code(0x018929, 2);
    R[0] = gsu_fmult(R[3], R[6]);
    R[0] = gsu_rol(R[0]);
    R[0] = gsu_add(R[0], R[5], 0);
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x01892b, 4);
    gsu_stw(R[9], R[0]);
    R[0] = gsu_sub(R[0], R[8], 0);
    R[0] = gsu_sub(R[0], R[8], 0);
    R[9] = gsu_add(R[9], 6, 0);
    gsu_code(0x01892f, 6);
    gsu_stw(R[9], R[0]);
    R[9] = gsu_sub(R[9], 4, 0);
    gsu_code(0x018935, 6);
    R[6] = gsu_ldw(FX_VIEW_MATRIX_1);
    gsu_code(0x01893b, 2);
    R[0] = gsu_fmult(R[1], R[6]);
    R[8] = gsu_rol(R[0]);
    gsu_code(0x01893d, 5);
    R[6] = gsu_ldw(FX_VIEW_MATRIX_4);
    gsu_code(0x018942, 2);
    R[0] = gsu_fmult(R[2], R[6]);
    R[5] = gsu_rol(R[0]);
    gsu_code(0x018944, 5);
    R[6] = gsu_ldw(FX_VIEW_MATRIX_7);
    gsu_code(0x018949, 2);
    R[0] = gsu_fmult(R[3], R[6]);
    R[0] = gsu_rol(R[0]);
    R[0] = gsu_add(R[0], R[5], 0);
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x01894b, 4);
    gsu_stw(R[9], R[0]);
    R[0] = gsu_sub(R[0], R[8], 0);
    R[0] = gsu_sub(R[0], R[8], 0);
    R[9] = gsu_add(R[9], 6, 0);
    gsu_code(0x01894f, 6);
    gsu_stw(R[9], R[0]);
    R[9] = gsu_sub(R[9], 4, 0);
    gsu_code(0x018955, 6);
    R[6] = gsu_ldw(FX_VIEW_MATRIX_2);
    gsu_code(0x01895b, 2);
    R[0] = gsu_fmult(R[1], R[6]);
    R[8] = gsu_rol(R[0]);
    gsu_code(0x01895d, 5);
    R[6] = gsu_ldw(FX_VIEW_MATRIX_5);
    gsu_code(0x018962, 2);
    R[0] = gsu_fmult(R[2], R[6]);
    R[5] = gsu_rol(R[0]);
    gsu_code(0x018964, 5);
    R[6] = gsu_ldw(FX_VIEW_MATRIX_8);
    gsu_code(0x018969, 2);
    R[0] = gsu_fmult(R[3], R[6]);
    R[0] = gsu_rol(R[0]);
    R[0] = gsu_add(R[0], R[5], 0);
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x01896b, 4);
    gsu_stw(R[9], R[0]);
    R[0] = gsu_sub(R[0], R[8], 0);
    R[0] = gsu_sub(R[0], R[8], 0);
    R[9] = gsu_add(R[9], 6, 0);
    gsu_code(0x01896f, 6);
    gsu_stw(R[9], R[0]);
    R[9] = gsu_sub(R[9], 4, 0);
    R[9] = gsu_add(R[9], 6, 0);
    t = gsu_count_down(&R[12]);
    gsu_code(0x018975, 7);
    if (t) { gsu_code(0x018902, 1); goto l8902; }
    gsu_code(0x01897c, 4);
    gsu_stw(FX_POINT_OUT, R[9]);
    gsu_code(0x018980, 2);
    nx = (uint16_t)(gsu_getb());
    at = model_command(nx);
    gsu_code(0x010000 | at, 1);   /* where it jumps, fetched before the delay slot runs */
    gsu_r14(gsu_inc(R[14]));
    goto fetched;
l8982:
    gsu_code(0x018983, 3);
    R[9] = gsu_ldw(FX_POINT_OUT);
    gsu_code(0x018986, 3);
    gsu_stw(FX_MODEL_STACK, R[10]);
    gsu_code(0x018989, 3);
    R[11] = gsu_ldw(FX_VIEW_MATRIX_3);
    gsu_code(0x01898c, 3);
    R[13] = gsu_ldw(FX_VIEW_MATRIX_6);
    gsu_code(0x01898f, 3);
    R[4] = gsu_ldw(FX_VIEW_MATRIX_1);
    gsu_code(0x018992, 3);
    R[7] = gsu_ldw(FX_VIEW_MATRIX_4);
    gsu_code(0x018995, 3);
    R[8] = gsu_ldw(FX_VIEW_MATRIX_7);
    gsu_code(0x018998, 3);
    R[10] = gsu_ldw(FX_VIEW_MATRIX_2);
    gsu_code(0x01899b, 2);
    R[12] = gsu_getb();
    gsu_code(0x01899d, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x01899e, 3);
    R[0] = gsu_ldw(FX_POINTS_TOTAL);
    R[0] = gsu_add(R[0], R[12], 0);
    gsu_code(0x0189a1, 2); gsu_cache(0x89a3);
    gsu_sbk(R[0]);
    /* CACHE */
    gsu_code(0x0189a3, 1);
l89A3:
    gsu_code(0x0189a4, 1);
    R[0] = gsu_getb();
    gsu_code(0x0189a5, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x0189a6, 3);
    R[5] = gsu_ldw(FX_POINT_SCALE);
    gsu_code(0x0189a9, 2);
    R[1] = gsu_mult(R[0], R[5]);
    gsu_code(0x0189ab, 1);
    R[0] = gsu_getb();
    gsu_code(0x0189ac, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x0189ad, 3);
    R[6] = gsu_ldw(FX_VIEW_MATRIX_0);
    gsu_code(0x0189b0, 2);
    R[2] = gsu_mult(R[0], R[5]);
    gsu_code(0x0189b2, 1);
    R[0] = gsu_getb();
    gsu_code(0x0189b3, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x0189b4, 2);
    R[3] = gsu_mult(R[0], R[5]);
    gsu_code(0x0189b6, 2);
    R[0] = gsu_fmult(R[1], R[6]);
    R[5] = gsu_rol(R[0]);
    R[6] = R[11];
    gsu_code(0x0189b8, 6);
    R[0] = gsu_fmult(R[2], R[6]);
    R[0] = gsu_rol(R[0]);
    R[5] = gsu_add(R[5], R[0], 0);
    R[6] = R[13];
    gsu_code(0x0189be, 7);
    R[0] = gsu_fmult(R[3], R[6]);
    R[0] = gsu_rol(R[0]);
    R[0] = gsu_add(R[0], R[5], 0);
    gsu_code(0x0189c5, 3);
    gsu_stw(R[9], R[0]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    R[6] = R[4];
    gsu_code(0x0189c8, 6);
    R[0] = gsu_fmult(R[1], R[6]);
    R[5] = gsu_rol(R[0]);
    R[6] = R[7];
    gsu_code(0x0189ce, 6);
    R[0] = gsu_fmult(R[2], R[6]);
    R[0] = gsu_rol(R[0]);
    R[5] = gsu_add(R[5], R[0], 0);
    R[6] = R[8];
    gsu_code(0x0189d4, 7);
    R[0] = gsu_fmult(R[3], R[6]);
    R[0] = gsu_rol(R[0]);
    R[0] = gsu_add(R[0], R[5], 0);
    gsu_code(0x0189db, 3);
    gsu_stw(R[9], R[0]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    R[6] = R[10];
    gsu_code(0x0189de, 6);
    R[0] = gsu_fmult(R[1], R[6]);
    R[5] = gsu_rol(R[0]);
    gsu_code(0x0189e4, 5);
    R[6] = gsu_ldw(FX_VIEW_MATRIX_5);
    gsu_code(0x0189e9, 2);
    R[0] = gsu_fmult(R[2], R[6]);
    R[0] = gsu_rol(R[0]);
    R[5] = gsu_add(R[5], R[0], 0);
    gsu_code(0x0189eb, 6);
    R[6] = gsu_ldw(FX_VIEW_MATRIX_8);
    gsu_code(0x0189f1, 2);
    R[0] = gsu_fmult(R[3], R[6]);
    R[0] = gsu_rol(R[0]);
    R[0] = gsu_add(R[0], R[5], 0);
    gsu_code(0x0189f3, 3);
    gsu_stw(R[9], R[0]);
    R[9] = gsu_inc(R[9]);
    R[12] = gsu_dec(R[12]);
    t = !gsu.z;
    R[9] = gsu_inc(R[9]);
    gsu_code(0x0189f6, 4);
    if (t) { gsu_code(0x0189a3, 1); goto l89A3; }
    gsu_code(0x0189fa, 4);
    R[10] = gsu_ldw(FX_MODEL_STACK);
    gsu_code(0x0189fe, 3);
    gsu_stw(FX_POINT_OUT, R[9]);
    gsu_code(0x018a01, 2);
    nx = (uint16_t)(gsu_getb());
    at = model_command(nx);
    gsu_code(0x010000 | at, 1);   /* where it jumps, fetched before the delay slot runs */
    gsu_r14(gsu_inc(R[14]));
    goto fetched;
l8A03:
    gsu_code(0x018a04, 3);
    R[9] = gsu_ldw(FX_POINT_OUT);
    gsu_code(0x018a07, 3);
    gsu_stw(FX_MODEL_STACK, R[10]);
    gsu_code(0x018a0a, 3);
    R[11] = gsu_ldw(FX_VIEW_MATRIX_3);
    gsu_code(0x018a0d, 3);
    R[13] = gsu_ldw(FX_VIEW_MATRIX_6);
    gsu_code(0x018a10, 3);
    R[4] = gsu_ldw(FX_VIEW_MATRIX_1);
    gsu_code(0x018a13, 3);
    R[7] = gsu_ldw(FX_VIEW_MATRIX_4);
    gsu_code(0x018a16, 3);
    R[10] = gsu_ldw(FX_VIEW_MATRIX_2);
    gsu_code(0x018a19, 2);
    R[12] = gsu_getb();
    gsu_code(0x018a1b, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x018a1c, 3);
    R[0] = gsu_ldw(FX_POINTS_TOTAL);
    R[0] = gsu_add(R[0], R[12], 0);
    R[0] = gsu_add(R[0], R[12], 0);
    gsu_code(0x018a1f, 3); gsu_cache(0x8a22);
    gsu_sbk(R[0]);
    /* CACHE */
    gsu_code(0x018a22, 1);
l8A22:
    gsu_code(0x018a23, 1);
    R[0] = gsu_getb();
    gsu_code(0x018a24, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x018a25, 3);
    R[5] = gsu_ldw(FX_POINT_SCALE);
    gsu_code(0x018a28, 2);
    R[1] = gsu_mult(R[0], R[5]);
    gsu_code(0x018a2a, 1);
    R[0] = gsu_getb();
    gsu_code(0x018a2b, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x018a2c, 3);
    R[6] = gsu_ldw(FX_VIEW_MATRIX_0);
    gsu_code(0x018a2f, 2);
    R[2] = gsu_mult(R[0], R[5]);
    gsu_code(0x018a31, 1);
    R[0] = gsu_getb();
    gsu_code(0x018a32, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x018a33, 2);
    R[3] = gsu_mult(R[0], R[5]);
    gsu_code(0x018a35, 2);
    R[0] = gsu_fmult(R[1], R[6]);
    R[8] = gsu_rol(R[0]);
    R[6] = R[11];
    gsu_code(0x018a37, 6);
    R[0] = gsu_fmult(R[2], R[6]);
    R[5] = gsu_rol(R[0]);
    R[6] = R[13];
    gsu_code(0x018a3d, 6);
    R[0] = gsu_fmult(R[3], R[6]);
    R[0] = gsu_rol(R[0]);
    R[0] = gsu_add(R[0], R[5], 0);
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x018a43, 4);
    gsu_stw(R[9], R[0]);
    R[0] = gsu_sub(R[0], R[8], 0);
    R[0] = gsu_sub(R[0], R[8], 0);
    R[9] = gsu_add(R[9], 6, 0);
    gsu_code(0x018a47, 6);
    gsu_stw(R[9], R[0]);
    R[9] = gsu_sub(R[9], 4, 0);
    R[6] = R[4];
    gsu_code(0x018a4d, 7);
    R[0] = gsu_fmult(R[1], R[6]);
    R[8] = gsu_rol(R[0]);
    R[6] = R[7];
    gsu_code(0x018a54, 6);
    R[0] = gsu_fmult(R[2], R[6]);
    R[5] = gsu_rol(R[0]);
    gsu_code(0x018a5a, 5);
    R[6] = gsu_ldw(FX_VIEW_MATRIX_7);
    gsu_code(0x018a5f, 2);
    R[0] = gsu_fmult(R[3], R[6]);
    R[0] = gsu_rol(R[0]);
    R[0] = gsu_add(R[0], R[5], 0);
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x018a61, 4);
    gsu_stw(R[9], R[0]);
    R[0] = gsu_sub(R[0], R[8], 0);
    R[0] = gsu_sub(R[0], R[8], 0);
    R[9] = gsu_add(R[9], 6, 0);
    gsu_code(0x018a65, 6);
    gsu_stw(R[9], R[0]);
    R[9] = gsu_sub(R[9], 4, 0);
    R[6] = R[10];
    gsu_code(0x018a6b, 7);
    R[0] = gsu_fmult(R[1], R[6]);
    R[8] = gsu_rol(R[0]);
    gsu_code(0x018a72, 5);
    R[6] = gsu_ldw(FX_VIEW_MATRIX_5);
    gsu_code(0x018a77, 2);
    R[0] = gsu_fmult(R[2], R[6]);
    R[5] = gsu_rol(R[0]);
    gsu_code(0x018a79, 5);
    R[6] = gsu_ldw(FX_VIEW_MATRIX_8);
    gsu_code(0x018a7e, 2);
    R[0] = gsu_fmult(R[3], R[6]);
    R[0] = gsu_rol(R[0]);
    R[0] = gsu_add(R[0], R[5], 0);
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x018a80, 4);
    gsu_stw(R[9], R[0]);
    R[0] = gsu_sub(R[0], R[8], 0);
    R[0] = gsu_sub(R[0], R[8], 0);
    R[9] = gsu_add(R[9], 6, 0);
    gsu_code(0x018a84, 6);
    gsu_stw(R[9], R[0]);
    R[9] = gsu_inc(R[9]);
    R[12] = gsu_dec(R[12]);
    t = !gsu.z;
    R[9] = gsu_inc(R[9]);
    gsu_code(0x018a8a, 4);
    if (t) { gsu_code(0x018a22, 1); goto l8A22; }
    gsu_code(0x018a8e, 4);
    R[10] = gsu_ldw(FX_MODEL_STACK);
    gsu_code(0x018a92, 3);
    gsu_stw(FX_POINT_OUT, R[9]);
    gsu_code(0x018a95, 2);
    nx = (uint16_t)(gsu_getb());
    at = model_command(nx);
    gsu_code(0x010000 | at, 1);   /* where it jumps, fetched before the delay slot runs */
    gsu_r14(gsu_inc(R[14]));
    goto fetched;
    /* FxModelPoints01 */
l8A97:
    gsu_code(0x018a98, 3);
    R[0] = gsu_ldw(FX_POINT_SHIFT);
    R[0] = gsu_sub(R[0], 3, 0);
    t = gsu.s;
    gsu_code(0x018a9b, 4);
    if (t) { gsu_code(0x018aa3, 1); goto l8AA3; }
    gsu_code(0x018a9f, 4); gsu_code(0x018982, 1);
    goto l8982;
l8AA3:
    gsu_code(0x018aa4, 4);
    R[11] = gsu_ldw(FX_POINT_OUT);
    gsu_code(0x018aa8, 3);
    R[5] = gsu_ldw(FX_MATRIX_PACKED_0);
    gsu_code(0x018aab, 3);
    R[6] = gsu_ldw(FX_MATRIX_PACKED_1);
    gsu_code(0x018aae, 3);
    R[7] = gsu_ldw(FX_MATRIX_PACKED_2);
    gsu_code(0x018ab1, 3);
    R[8] = gsu_ldw(FX_MATRIX_PACKED_3);
    gsu_code(0x018ab4, 3);
    R[9] = gsu_ldw(FX_MATRIX_PACKED_4);
    gsu_code(0x018ab7, 3);
    R[1] = gsu_ldw(FX_POINT_SCALE);
    R[0] = gsu_lob(R[9]);
    R[0] = gsu_swap(R[0]);
    R[9] = gsu_or(R[0], R[1]);
    gsu_code(0x018aba, 7);
    R[12] = gsu_getb();
    gsu_code(0x018ac1, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x018ac2, 3);
    R[0] = gsu_ldw(FX_POINTS_TOTAL);
    R[0] = gsu_add(R[0], R[12], 0);
    gsu_code(0x018ac5, 2); gsu_cache(0x8ac7);
    gsu_sbk(R[0]);
    /* CACHE */
    R[13] = 0x8AC9;
    gsu_code(0x018ac7, 3);
l8AC9:
    gsu_code(0x018aca, 2);
    R[1] = gsu_getb();
    gsu_code(0x018acc, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x018acd, 3);
    R[4] = gsu_mult(R[5], R[1]);
    gsu_code(0x018ad0, 2);
    R[2] = gsu_getb();
    gsu_code(0x018ad2, 1);
    gsu_r14(gsu_inc(R[14]));
    R[0] = gsu_hib(R[6]);
    gsu_code(0x018ad3, 3);
    R[0] = gsu_mult(R[0], R[2]);
    R[4] = gsu_add(R[0], R[4], 0);
    gsu_code(0x018ad6, 4);
    R[3] = gsu_getb();
    gsu_code(0x018ada, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x018adb, 2);
    R[0] = gsu_mult(R[8], R[3]);
    R[0] = gsu_add(R[0], R[4], 0);
    R[0] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_hib(R[0]);
    gsu_code(0x018add, 4);
    R[0] = gsu_mult(R[0], R[9]);
    gsu_code(0x018ae1, 1);
    gsu_stw(R[11], R[0]);
    R[11] = gsu_inc(R[11]);
    R[11] = gsu_inc(R[11]);
    R[0] = gsu_hib(R[5]);
    gsu_code(0x018ae2, 6);
    R[4] = gsu_mult(R[0], R[1]);
    gsu_code(0x018ae8, 2);
    R[0] = gsu_mult(R[7], R[2]);
    R[4] = gsu_add(R[0], R[4], 0);
    R[0] = gsu_hib(R[8]);
    gsu_code(0x018aea, 5);
    R[0] = gsu_mult(R[0], R[3]);
    R[0] = gsu_add(R[0], R[4], 0);
    R[0] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_hib(R[0]);
    gsu_code(0x018aef, 4);
    R[0] = gsu_mult(R[0], R[9]);
    gsu_code(0x018af3, 1);
    gsu_stw(R[11], R[0]);
    R[11] = gsu_inc(R[11]);
    R[11] = gsu_inc(R[11]);
    gsu_code(0x018af4, 5);
    R[4] = gsu_mult(R[6], R[1]);
    R[0] = gsu_hib(R[7]);
    gsu_code(0x018af9, 3);
    R[0] = gsu_mult(R[0], R[2]);
    R[4] = gsu_add(R[0], R[4], 0);
    R[0] = gsu_hib(R[9]);
    gsu_code(0x018afc, 5);
    R[0] = gsu_mult(R[0], R[3]);
    R[0] = gsu_add(R[0], R[4], 0);
    R[0] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_hib(R[0]);
    gsu_code(0x018b01, 4);
    R[0] = gsu_mult(R[0], R[9]);
    gsu_code(0x018b05, 1);
    gsu_stw(R[11], R[0]);
    R[11] = gsu_inc(R[11]);
    t = gsu_count_down(&R[12]);
    R[11] = gsu_inc(R[11]);
    gsu_code(0x018b06, 2);
    if (t) { gsu_code(0x018ac9, 1); goto l8AC9; }
    gsu_code(0x018b08, 4);
    gsu_stw(FX_POINT_OUT, R[11]);
    gsu_code(0x018b0c, 2);
    nx = (uint16_t)(gsu_getb());
    at = model_command(nx);
    gsu_code(0x010000 | at, 1);   /* where it jumps, fetched before the delay slot runs */
    gsu_r14(gsu_inc(R[14]));
    goto fetched;
    /* FxModelPointsMirrored */
l8B0E:
    gsu_code(0x018b0f, 3);
    R[0] = gsu_ldw(FX_POINT_SHIFT);
    R[0] = gsu_sub(R[0], 3, 0);
    t = gsu.s;
    gsu_code(0x018b12, 4);
    if (t) { gsu_code(0x018b1a, 1); goto l8B1A; }
    gsu_code(0x018b16, 4); gsu_code(0x018a03, 1);
    goto l8A03;
l8B1A:
    gsu_code(0x018b1b, 4);
    R[11] = gsu_ldw(FX_POINT_OUT);
    gsu_code(0x018b1f, 3);
    R[5] = gsu_ldw(FX_MATRIX_PACKED_0);
    gsu_code(0x018b22, 3);
    R[6] = gsu_ldw(FX_MATRIX_PACKED_1);
    gsu_code(0x018b25, 3);
    R[7] = gsu_ldw(FX_MATRIX_PACKED_2);
    gsu_code(0x018b28, 3);
    R[8] = gsu_ldw(FX_MATRIX_PACKED_3);
    gsu_code(0x018b2b, 3);
    R[9] = gsu_ldw(FX_MATRIX_PACKED_4);
    gsu_code(0x018b2e, 3);
    R[1] = gsu_ldw(FX_POINT_SCALE);
    R[0] = gsu_lob(R[9]);
    R[0] = gsu_swap(R[0]);
    R[9] = gsu_or(R[0], R[1]);
    gsu_code(0x018b31, 7);
    R[12] = gsu_getb();
    gsu_code(0x018b38, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x018b39, 3);
    R[0] = gsu_ldw(FX_POINTS_TOTAL);
    R[0] = gsu_add(R[0], R[12], 0);
    R[0] = gsu_add(R[0], R[12], 0);
    gsu_code(0x018b3c, 3); gsu_cache(0x8b3f);
    gsu_sbk(R[0]);
    /* CACHE */
    gsu_code(0x018b3f, 1);
l8B3F:
    gsu_code(0x018b40, 2);
    R[1] = gsu_getb();
    gsu_code(0x018b42, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x018b43, 3);
    R[13] = gsu_mult(R[5], R[1]);
    gsu_code(0x018b46, 2);
    R[2] = gsu_getb();
    gsu_code(0x018b48, 1);
    gsu_r14(gsu_inc(R[14]));
    R[0] = gsu_hib(R[6]);
    gsu_code(0x018b49, 4);
    R[4] = gsu_mult(R[0], R[2]);
    gsu_code(0x018b4d, 2);
    R[3] = gsu_getb();
    gsu_code(0x018b4f, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x018b50, 2);
    R[0] = gsu_mult(R[8], R[3]);
    R[4] = gsu_add(R[0], R[4], 0);
    R[0] = gsu_add(R[4], R[13], 0);
    R[0] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_hib(R[0]);
    gsu_code(0x018b52, 7);
    R[0] = gsu_mult(R[0], R[9]);
    gsu_code(0x018b59, 1);
    gsu_stw(R[11], R[0]);
    R[11] = gsu_add(R[11], 6, 0);
    R[0] = gsu_sub(R[4], R[13], 0);
    R[0] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_hib(R[0]);
    gsu_code(0x018b5a, 8);
    R[0] = gsu_mult(R[0], R[9]);
    gsu_code(0x018b62, 1);
    gsu_stw(R[11], R[0]);
    R[11] = gsu_sub(R[11], 4, 0);
    R[0] = gsu_hib(R[5]);
    gsu_code(0x018b63, 7);
    R[13] = gsu_mult(R[0], R[1]);
    gsu_code(0x018b6a, 3);
    R[4] = gsu_mult(R[7], R[2]);
    R[0] = gsu_hib(R[8]);
    gsu_code(0x018b6d, 3);
    R[0] = gsu_mult(R[0], R[3]);
    R[4] = gsu_add(R[0], R[4], 0);
    R[0] = gsu_add(R[4], R[13], 0);
    R[0] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_hib(R[0]);
    gsu_code(0x018b70, 7);
    R[0] = gsu_mult(R[0], R[9]);
    gsu_code(0x018b77, 1);
    gsu_stw(R[11], R[0]);
    R[11] = gsu_add(R[11], 6, 0);
    R[0] = gsu_sub(R[4], R[13], 0);
    R[0] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_hib(R[0]);
    gsu_code(0x018b78, 8);
    R[0] = gsu_mult(R[0], R[9]);
    gsu_code(0x018b80, 1);
    gsu_stw(R[11], R[0]);
    R[11] = gsu_sub(R[11], 4, 0);
    gsu_code(0x018b81, 6);
    R[13] = gsu_mult(R[6], R[1]);
    R[0] = gsu_hib(R[7]);
    gsu_code(0x018b87, 4);
    R[4] = gsu_mult(R[0], R[2]);
    R[0] = gsu_hib(R[9]);
    gsu_code(0x018b8b, 3);
    R[0] = gsu_mult(R[0], R[3]);
    R[4] = gsu_add(R[0], R[4], 0);
    R[0] = gsu_add(R[4], R[13], 0);
    R[0] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_hib(R[0]);
    gsu_code(0x018b8e, 7);
    R[0] = gsu_mult(R[0], R[9]);
    gsu_code(0x018b95, 1);
    gsu_stw(R[11], R[0]);
    R[11] = gsu_add(R[11], 6, 0);
    R[0] = gsu_sub(R[4], R[13], 0);
    R[0] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_hib(R[0]);
    gsu_code(0x018b96, 8);
    R[0] = gsu_mult(R[0], R[9]);
    gsu_code(0x018b9e, 1);
    gsu_stw(R[11], R[0]);
    R[11] = gsu_inc(R[11]);
    R[11] = gsu_inc(R[11]);
    R[12] = gsu_dec(R[12]);
    t = !gsu.z;
    gsu_code(0x018b9f, 5);
    if (t) { gsu_code(0x018b3f, 1); goto l8B3F; }
    gsu_code(0x018ba4, 4);
    gsu_stw(FX_POINT_OUT, R[11]);
    gsu_code(0x018ba8, 2);
    nx = (uint16_t)(gsu_getb());
    at = model_command(nx);
    gsu_code(0x010000 | at, 1);   /* where it jumps, fetched before the delay slot runs */
    gsu_r14(gsu_inc(R[14]));
    goto fetched;
    /* FxModelProject */
l8BAA:
    gsu_code(0x018bab, 3); gsu_code(0x018c89, 1);
    goto l8C89;
l8BAE:
    gsu_code(0x018baf, 3);
    R[12] = gsu_ldw(FX_POINTS_TOTAL);
    R[0] = 0x0014;
    gsu_code(0x018bb2, 4);
    gsu_romb(R[0]);
    R[7] = 0x0340;
    gsu_code(0x018bb6, 6);
    gsu_stw(FX_ROM_R14, R[14]);
    R[8] = 0x04E4;
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_code(0x018bbc, 4);
l8BBF:
    gsu_code(0x018bc0, 3);
    gsu_stw(FX_OUTCODES, R[0]);
    gsu_code(0x018bc3, 3);
    R[9] = gsu_ldw(FX_OBJECT_Z);
    gsu_code(0x018bc6, 3);
    R[3] = gsu_ldw(FX_OBJECT_X);
    gsu_code(0x018bc9, 3); gsu_cache(0x8bcc);
    R[13] = gsu_ldw(FX_OBJECT_Y);
    /* CACHE */
    gsu_code(0x018bcc, 1);
l8BCC:
    gsu_code(0x018bcd, 1);
    R[0] = gsu_ldw(R[7]);
    R[7] = gsu_inc(R[7]);
    R[7] = gsu_inc(R[7]);
    R[1] = gsu_add(R[0], R[3], 0);
    gsu_code(0x018bce, 5);
    R[0] = gsu_ldw(R[7]);
    R[7] = gsu_inc(R[7]);
    R[7] = gsu_inc(R[7]);
    R[2] = gsu_add(R[0], R[13], 0);
    gsu_code(0x018bd3, 5);
    R[0] = gsu_ldw(R[7]);
    R[7] = gsu_inc(R[7]);
    R[7] = gsu_inc(R[7]);
    R[6] = gsu_add(R[0], R[9], 0);
    R[11] = 0x8BE0;
    gsu_code(0x018bd8, 8); gsu_code(0x018bff, 1);
    abi_FxProjectVertex();
    gsu_code(0x018be0, 1);
l8BE0:
    gsu_code(0x018be1, 2);
    gsu_stw(R[8], R[1]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    gsu_code(0x018be3, 4);
    gsu_stw(R[8], R[2]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    gsu_code(0x018be7, 3);
    gsu_stw(R[8], R[0]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    gsu_code(0x018bea, 5);
    R[6] = gsu_ldw(FX_OUTCODES);
    R[0] = gsu_or(R[0], R[6]);
    R[12] = gsu_dec(R[12]);
    t = !gsu.z;
    gsu_code(0x018bef, 4);
    gsu_code(t ? 0x018bcc : 0x018bf3, 1);   /* the delay slot runs after the next fetch */
    gsu_sbk(R[0]);
    if (t) goto l8BCC;
    gsu_code(0x018bf4, 3);
    R[0] = gsu_ldw(FX_ROM_BANK);
    gsu_code(0x018bf7, 2);
    gsu_romb(R[0]);
    gsu_code(0x018bf9, 3);
    gsu_r14(gsu_ldw(FX_ROM_R14));
    gsu_code(0x018bfc, 3); gsu_code(0x018c79, 1);
    goto l8C79;
l8BFF:
    R[0] = 0x4400;
    R[0] = gsu_sub(R[0], R[6], 0);
    t = !gsu.s;
    gsu_code(0x018c00, 6);
    if (t) { gsu_code(0x018c0e, 1); goto l8C0E; }
    R[1] = gsu_asr(R[1]);
    R[2] = gsu_asr(R[2]);
    t = 1;
    R[6] = gsu_asr(R[6]);
    gsu_code(0x018c06, 8);
    gsu_code(0x018bff, 1);
    if (t) goto l8BFF;
l8C0E:
    R[0] = 0x0100;
    R[0] = gsu_sub(R[0], R[6], 0);
    t = !gsu.s;
    gsu_code(0x018c0f, 6);
    if (t) { gsu_code(0x018c5c, 1); goto l8C5C; }
    R[6] = gsu_bic(R[6], 1);
    R[0] = 0x896C;
    gsu_code(0x018c15, 9);
    gsu_r14(gsu_add(R[0], R[6], 0));
    gsu_code(0x018c1e, 2);
    R[6] = gsu_getb();
    gsu_code(0x018c20, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x018c21, 3);
    R[6] = gsu_getbh(R[6]);
    gsu_code(0x018c24, 2);
    R[0] = gsu_fmult(R[1], R[6]);
    gsu_code(0x018c26, 3);
    R[1] = gsu_ldw(FX_MODEL_34);
    R[0] = gsu_add(R[0], R[1], 0);
    R[1] = R[0];
    gsu_code(0x018c29, 5);
    R[0] = gsu_fmult(R[2], R[6]);
    gsu_code(0x018c2e, 3);
    R[2] = gsu_ldw(FX_MODEL_36);
    R[0] = gsu_add(R[0], R[2], 0);
    R[2] = R[0];
    R[6] = 0x0000;
    R[0] = R[1];
    gsu_moves(R[1]);
    t = gsu.s;
    gsu_code(0x018c31, 9);
    if (t) { gsu_code(0x018c42, 1); goto l8C42; }
    gsu_code(0x018c3a, 4);
    R[5] = gsu_ldw(FX_SCREEN_RIGHT);
    R[0] = gsu_sub(R[0], R[5], 0);
    t = gsu.s;
    gsu_code(0x018c3e, 3);
    if (t) { gsu_code(0x018c46, 1); goto l8C46; }
    R[6] = gsu_inc(R[6]);
    gsu_code(0x018c41, 2);
l8C42:
    R[6] = gsu_inc(R[6]);
    gsu_code(0x018c43, 4);
    R[6] = gsu_mult(R[6], 4);
l8C46:
    R[0] = R[2];
    gsu_moves(R[2]);
    t = gsu.s;
    gsu_code(0x018c47, 4);
    if (t) { gsu_code(0x018c53, 1); goto l8C53; }
    gsu_code(0x018c4b, 4);
    R[5] = gsu_ldw(FX_SCREEN_BOTTOM);
    R[0] = gsu_sub(R[0], R[5], 0);
    t = gsu.s;
    gsu_code(0x018c4f, 3);
    if (t) { gsu_code(0x018c54, 1); goto l8C54; }
    R[6] = gsu_inc(R[6]);
    gsu_code(0x018c52, 2);
l8C53:
    R[6] = gsu_inc(R[6]);
    gsu_code(0x018c54, 1);
l8C54:
    R[0] = 0x001F;
    R[0] = gsu_xor(R[0], R[6]);
    R[0] = gsu_swap(R[0]);
    R[0] = gsu_or(R[0], R[6]);
    nx = (uint16_t)(R[11]);
    at = nx;
    gsu_code(0x018c55, 7);
    goto dispatch;
l8C5C:
    gsu_code(0x018c5d, 2);
    gsu_r14(R[12]);
    R[3] = R[6];
    gsu_code(0x018c5f, 4);
    gsu_stw(R[10], R[11]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[11] = 0x8C69;
    gsu_code(0x018c63, 6); gsu_code(0x01947a, 1);
    abi_FxProjectPoint();
    gsu_code(0x018c69, 1);
l8C69:
    R[12] = R[14];
    gsu_code(0x018c6a, 5);
    R[3] = gsu_ldw(FX_OBJECT_X);
    gsu_code(0x018c6f, 3);
    R[13] = gsu_ldw(FX_OBJECT_Y);
    gsu_code(0x018c72, 3);
    R[9] = gsu_ldw(FX_OBJECT_Z);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x018c75, 4);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
    /* FxModelProjectRest */
l8C79:
    gsu_code(0x018c7a, 3); gsu_code(0x018d23, 1);
    goto l8D23;
l8C7D:
    R[7] = gsu_sub(R[7], 4, 0);
    R[0] = R[10];
    gsu_code(0x018c7e, 8);
    R[10] = gsu_ldw(FX_MODEL_STACK);
    gsu_code(0x018c86, 3); gsu_code(0x018bbf, 1);
    goto l8BBF;
l8C89:
    gsu_code(0x018c8a, 3);
    R[11] = gsu_ldw(FX_SCREEN_RIGHT);
    gsu_code(0x018c8d, 3);
    R[13] = gsu_ldw(FX_SCREEN_BOTTOM);
    gsu_code(0x018c90, 3);
    R[1] = gsu_ldw(FX_MODEL_34);
    gsu_code(0x018c93, 3);
    R[9] = gsu_ldw(FX_MODEL_36);
    gsu_code(0x018c96, 3);
    R[3] = gsu_ldw(FX_OBJECT_X);
    gsu_code(0x018c99, 3);
    R[4] = gsu_ldw(FX_OBJECT_Y);
    gsu_code(0x018c9c, 3);
    R[5] = gsu_ldw(FX_OBJECT_Z);
    gsu_code(0x018c9f, 3);
    R[12] = gsu_ldw(FX_POINTS_TOTAL);
    gsu_code(0x018ca2, 3);
    gsu_stw(FX_MODEL_STACK, R[10]);
    R[10] = 0x0000;
    R[0] = 0x0014;
    gsu_code(0x018ca5, 6);
    gsu_romb(R[0]);
    gsu_code(0x018cab, 3);
    gsu_stw(FX_ROM_R14, R[14]);
    R[7] = 0x0344;
    R[8] = 0x04E4;
    /* CACHE */
    gsu_code(0x018cae, 6); gsu_cache(0x8cb4); gsu_code(0x018cb4, 1);
l8CB4:
    gsu_code(0x018cb5, 1);
    R[0] = gsu_ldw(R[7]);
    R[0] = gsu_add(R[0], R[5], 0);
    R[6] = 0x0100;
    R[0] = gsu_sub(R[0], R[6], 0);
    t = !gsu.s;
    R[0] = gsu_add(R[0], R[6], 0);
    gsu_code(0x018cb6, 7);
    if (t) { gsu_code(0x018cc1, 1); goto l8CC1; }
    gsu_code(0x018cbd, 4); gsu_code(0x018c7d, 1);
    goto l8C7D;
l8CC1:
    R[6] = 0x43FE;
    R[0] = gsu_sub(R[0], R[6], 0);
    t = gsu.s;
    R[0] = gsu_add(R[0], R[6], 0);
    gsu_code(0x018cc2, 6);
    if (t) { gsu_code(0x018cca, 1); goto l8CCA; }
    R[0] = R[6];
    gsu_code(0x018cc8, 3);
l8CCA:
    R[0] = gsu_bic(R[0], 1);
    R[6] = 0x896C;
    gsu_code(0x018ccb, 7);
    gsu_r14(gsu_add(R[0], R[6], 0));
    R[7] = gsu_dec(R[7]);
    R[7] = gsu_dec(R[7]);
    gsu_code(0x018cd2, 3);
    R[0] = gsu_ldw(R[7]);
    R[2] = gsu_add(R[0], R[4], 0);
    gsu_code(0x018cd5, 4);
    R[6] = gsu_getb();
    gsu_code(0x018cd9, 1);
    gsu_r14(gsu_inc(R[14]));
    R[7] = gsu_dec(R[7]);
    R[7] = gsu_dec(R[7]);
    gsu_code(0x018cda, 3);
    R[0] = gsu_ldw(R[7]);
    R[0] = gsu_add(R[0], R[3], 0);
    gsu_code(0x018cdd, 4);
    R[6] = gsu_getbh(R[6]);
    gsu_code(0x018ce1, 1);
    R[0] = gsu_fmult(R[0], R[6]);
    R[0] = gsu_add(R[0], R[1], 0);
    gsu_code(0x018ce2, 2);
    gsu_stw(R[8], R[0]);
    gsu_code(0x018ce4, 2);
    gsu_r14(0x0000);
    t = gsu.s;
    R[0] = gsu_sub(R[0], R[11], 0);
    gsu_code(0x018ce6, 2);
    if (t) { gsu_code(0x018cec, 1); goto l8CEC; }
    t = gsu.s;
    gsu_code(0x018ce8, 3);
    if (t) { gsu_code(0x018cf0, 1); goto l8CF0; }
    gsu_code(0x018ceb, 2);
    gsu_r14(gsu_inc(R[14]));
l8CEC:
    gsu_code(0x018ced, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x018cee, 3);
    gsu_r14(gsu_mult(R[14], 4));
l8CF0:
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    gsu_code(0x018cf1, 4);
    R[0] = gsu_fmult(R[2], R[6]);
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_code(0x018cf5, 2);
    gsu_stw(R[8], R[0]);
    t = gsu.s;
    R[0] = gsu_sub(R[0], R[13], 0);
    gsu_code(0x018cf7, 2);
    if (t) { gsu_code(0x018cfd, 1); goto l8CFD; }
    t = gsu.s;
    gsu_code(0x018cf9, 3);
    if (t) { gsu_code(0x018cfe, 1); goto l8CFE; }
    gsu_code(0x018cfc, 2);
    gsu_r14(gsu_inc(R[14]));
l8CFD:
    gsu_code(0x018cfe, 1);
    gsu_r14(gsu_inc(R[14]));
l8CFE:
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    R[0] = 0x001F;
    R[0] = gsu_xor(R[0], R[14]);
    R[0] = gsu_swap(R[0]);
    R[0] = gsu_or(R[0], R[14]);
    gsu_code(0x018cff, 9);
    gsu_stw(R[8], R[0]);
    R[10] = gsu_or(R[0], R[10]);
    R[8] = gsu_inc(R[8]);
    R[7] = gsu_add(R[7], 10, 0);
    R[12] = gsu_dec(R[12]);
    t = !gsu.z;
    R[8] = gsu_inc(R[8]);
    gsu_code(0x018d08, 9);
    if (t) { gsu_code(0x018cb4, 1); goto l8CB4; }
    gsu_code(0x018d11, 5);
    goto l8D15;
l8D15:
    gsu_code(0x018d16, 3);
    gsu_stw(FX_OUTCODES, R[10]);
    gsu_code(0x018d19, 3);
    R[10] = gsu_ldw(FX_MODEL_STACK);
    gsu_code(0x018d1c, 3);
    R[0] = gsu_ldw(FX_ROM_BANK);
    gsu_code(0x018d1f, 2);
    gsu_romb(R[0]);
    gsu_code(0x018d21, 3);
    gsu_r14(gsu_ldw(FX_ROM_R14));
l8D23:
    gsu_code(0x018d24, 3);
    R[0] = gsu_ldw(FX_OUTCODES);
    R[1] = 0x1F00;
    R[0] = gsu_and(R[0], R[1]);
    gsu_sub(R[0], R[1], 0);   /* CMP */
    t = gsu.z;
    gsu_code(0x018d27, 8);
    if (t) { gsu_code(0x018d33, 1); goto l8D33; }
    gsu_code(0x018d2f, 4); gsu_code(0x01924d, 1);
    goto l924D;
l8D33:
    gsu_code(0x018d34, 3);
    R[1] = gsu_ldw(FX_OUTCODES);
    R[0] = 0x001F;
    R[0] = gsu_and(R[0], R[1]);
    t = !gsu.z;
    gsu_code(0x018d37, 5);
    if (t) { gsu_code(0x018d44, 1); goto l8D44; }
    gsu_code(0x018d3c, 4);
    R[0] = gsu_ldw(FX_MODEL_FLAGS);
    R[1] = 0x4000;
    R[0] = gsu_or(R[0], R[1]);
    gsu_code(0x018d40, 5);
    gsu_sbk(R[0]);
l8D44:
    gsu_code(0x018d45, 3);
    gsu_r14(gsu_ldw(FX_MODEL_18));
    gsu_code(0x018d48, 3);
    gsu_stw(FX_ROM_R14, R[14]);
    gsu_code(0x018d4b, 2);
    nx = (uint16_t)(gsu_getb());
    at = model_command(nx);
    gsu_code(0x010000 | at, 1);   /* where it jumps, fetched before the delay slot runs */
    gsu_r14(gsu_inc(R[14]));
    goto fetched;
    /* FxModelListStart */
l8D4D:
    R[1] = 0x0788;
    gsu_code(0x018d4e, 6);
    gsu_stw(FX_MODEL_RETURN_AT, R[1]);
    R[0] = 0x8E7D;
    gsu_code(0x018d54, 4);
    gsu_stw(R[10], R[0]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    /* CACHE */
    gsu_code(0x018d58, 2); gsu_cache(0x8d5a); gsu_code(0x018d5a, 3);
    nx = (uint16_t)(gsu_getb());
    at = model_command(nx);
    gsu_code(0x010000 | at, 1);   /* where it jumps, fetched before the delay slot runs */
    gsu_r14(gsu_inc(R[14]));
    goto fetched;
    /* FxModelCall */
l8D5D:
    gsu_code(0x018d5e, 1);
    gsu_r14(gsu_dec(R[14]));
    gsu_code(0x018d5f, 1);
    gsu_r14(gsu_inc(R[14]));
    R[0] = R[14];
    R[0] = gsu_inc(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_code(0x018d60, 7);
    R[1] = gsu_ldw(FX_MODEL_RETURN_AT);
    gsu_code(0x018d67, 1);
    gsu_stw(R[1], R[0]);
    R[1] = gsu_inc(R[1]);
    R[1] = gsu_inc(R[1]);
    gsu_code(0x018d68, 5);
    gsu_stw(FX_MODEL_RETURN_AT, R[1]);
    gsu_code(0x018d6d, 1);
    R[0] = gsu_getb();
    gsu_code(0x018d6e, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x018d6f, 2);
    R[0] = gsu_getbh(R[0]);
    gsu_code(0x018d71, 2);
    gsu_r14(gsu_add(R[0], R[14], 0));
    gsu_code(0x018d73, 2);
    nx = (uint16_t)(gsu_getb());
    at = model_command(nx);
    gsu_code(0x010000 | at, 1);   /* where it jumps, fetched before the delay slot runs */
    gsu_r14(gsu_inc(R[14]));
    goto fetched;
    /* FxModelReturnPull */
l8D75:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x018d76, 4);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
    /* FxModelIfCall */
l8D7A:
    gsu_code(0x018d7b, 1);
    R[0] = gsu_getb();
    gsu_code(0x018d7c, 1);
    gsu_r14(gsu_inc(R[14]));
    R[1] = 0x07C6;
    R[0] = gsu_add(R[0], R[1], 0);
    gsu_code(0x018d7d, 6);
    R[0] = gsu_ldb(R[0]);
    R[0] = gsu_swap(R[0]);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.s;
    gsu_code(0x018d83, 5);
    if (t) { gsu_code(0x018dbd, 1); goto l8DBD; }
    gsu_code(0x018d88, 3);
    gsu_stw(R[10], R[14]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_code(0x018d8b, 3);
    R[0] = gsu_getb();
    gsu_code(0x018d8e, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x018d8f, 2);
    R[0] = gsu_getbh(R[0]);
    gsu_code(0x018d91, 2);
    gsu_r14(gsu_add(R[0], R[14], 0));
    R[0] = gsu_add(0x8D95, 5, 0);
    gsu_code(0x018d93, 4);
    gsu_stw(R[10], R[0]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_code(0x018d97, 4);
    nx = (uint16_t)(gsu_getb());
    at = model_command(nx);
    gsu_code(0x010000 | at, 1);   /* where it jumps, fetched before the delay slot runs */
    gsu_r14(gsu_inc(R[14]));
    goto fetched;
l8D9A:
    gsu_code(0x018d9b, 1);
    gsu_r14(gsu_inc(R[14]));
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x018d9c, 4);
    gsu_r14(gsu_ldw(R[10]));
    gsu_code(0x018da0, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x018da1, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x018da2, 2);
    gsu_stw(R[10], R[14]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_code(0x018da4, 3);
    R[0] = gsu_getb();
    gsu_code(0x018da7, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x018da8, 2);
    R[0] = gsu_getbh(R[0]);
    gsu_code(0x018daa, 2);
    gsu_r14(gsu_add(R[0], R[14], 0));
    R[0] = gsu_add(0x8DAE, 5, 0);
    gsu_code(0x018dac, 4);
    gsu_stw(R[10], R[0]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_code(0x018db0, 4);
    nx = (uint16_t)(gsu_getb());
    at = model_command(nx);
    gsu_code(0x010000 | at, 1);   /* where it jumps, fetched before the delay slot runs */
    gsu_r14(gsu_inc(R[14]));
    goto fetched;
l8DB3:
    gsu_code(0x018db4, 1);
    gsu_r14(gsu_inc(R[14]));
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x018db5, 4);
    gsu_r14(gsu_ldw(R[10]));
    gsu_code(0x018db9, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x018dba, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x018dbb, 2);
    nx = (uint16_t)(gsu_getb());
    at = model_command(nx);
    gsu_code(0x010000 | at, 1);   /* where it jumps, fetched before the delay slot runs */
    gsu_r14(gsu_inc(R[14]));
    goto fetched;
l8DBD:
    gsu_code(0x018dbe, 2);
    gsu_stw(R[10], R[14]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_code(0x018dc0, 3);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x018dc3, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x018dc4, 1);
    R[0] = gsu_getb();
    gsu_code(0x018dc5, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x018dc6, 2);
    R[0] = gsu_getbh(R[0]);
    gsu_code(0x018dc8, 2);
    gsu_r14(gsu_add(R[0], R[14], 0));
    R[0] = gsu_add(0x8DCC, 5, 0);
    gsu_code(0x018dca, 4);
    gsu_stw(R[10], R[0]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_code(0x018dce, 4);
    nx = (uint16_t)(gsu_getb());
    at = model_command(nx);
    gsu_code(0x010000 | at, 1);   /* where it jumps, fetched before the delay slot runs */
    gsu_r14(gsu_inc(R[14]));
    goto fetched;
l8DD1:
    gsu_code(0x018dd2, 1);
    gsu_r14(gsu_inc(R[14]));
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x018dd3, 4);
    gsu_r14(gsu_ldw(R[10]));
    gsu_code(0x018dd7, 1);
    gsu_r14(gsu_dec(R[14]));
    gsu_code(0x018dd8, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x018dd9, 2);
    gsu_stw(R[10], R[14]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_code(0x018ddb, 3);
    R[0] = gsu_getb();
    gsu_code(0x018dde, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x018ddf, 2);
    R[0] = gsu_getbh(R[0]);
    gsu_code(0x018de1, 2);
    gsu_r14(gsu_add(R[0], R[14], 0));
    R[0] = gsu_add(0x8DE5, 5, 0);
    gsu_code(0x018de3, 4);
    gsu_stw(R[10], R[0]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_code(0x018de7, 4);
    nx = (uint16_t)(gsu_getb());
    at = model_command(nx);
    gsu_code(0x010000 | at, 1);   /* where it jumps, fetched before the delay slot runs */
    gsu_r14(gsu_inc(R[14]));
    goto fetched;
l8DEA:
    gsu_code(0x018deb, 1);
    gsu_r14(gsu_inc(R[14]));
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x018dec, 4);
    gsu_r14(gsu_ldw(R[10]));
    gsu_code(0x018df0, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x018df1, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x018df2, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x018df3, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x018df4, 2);
    nx = (uint16_t)(gsu_getb());
    at = model_command(nx);
    gsu_code(0x010000 | at, 1);   /* where it jumps, fetched before the delay slot runs */
    gsu_r14(gsu_inc(R[14]));
    goto fetched;
    /* FxModelListStart3C */
l8DF6:
    R[1] = 0x0788;
    gsu_code(0x018df7, 6);
    gsu_stw(FX_MODEL_RETURN_AT, R[1]);
    R[0] = 0x8E7D;
    gsu_code(0x018dfd, 4);
    gsu_stw(R[10], R[0]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    /* CACHE */
    gsu_code(0x018e01, 2); gsu_cache(0x8e03); gsu_code(0x018e03, 3);
    nx = (uint16_t)(gsu_getb());
    at = model_command(nx);
    gsu_code(0x010000 | at, 1);   /* where it jumps, fetched before the delay slot runs */
    gsu_r14(gsu_inc(R[14]));
    goto fetched;
    /* FxModelPushReturn */
l8E06:
    gsu_code(0x018e07, 1);
    R[0] = gsu_getb();
    gsu_code(0x018e08, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x018e09, 2);
    R[0] = gsu_getbh(R[0]);
    R[0] = gsu_add(R[0], R[14], 0);
    gsu_code(0x018e0b, 4);
    R[1] = gsu_ldw(FX_MODEL_RETURN_AT);
    gsu_code(0x018e0f, 1);
    gsu_stw(R[1], R[0]);
    R[1] = gsu_inc(R[1]);
    R[1] = gsu_inc(R[1]);
    gsu_code(0x018e10, 5);
    gsu_stw(FX_MODEL_RETURN_AT, R[1]);
    /* FxModelReturn40 */
l8E14:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x018e15, 4);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
    /* FxModelIfSkip */
l8E19:
    gsu_code(0x018e1a, 1);
    R[0] = gsu_getb();
    gsu_code(0x018e1b, 1);
    gsu_r14(gsu_inc(R[14]));
    R[1] = 0x07C6;
    R[0] = gsu_add(R[0], R[1], 0);
    gsu_code(0x018e1c, 6);
    R[0] = gsu_ldb(R[0]);
    R[0] = gsu_swap(R[0]);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.s;
    gsu_code(0x018e22, 5);
    if (t) { gsu_code(0x018e45, 1); goto l8E45; }
    gsu_code(0x018e27, 2);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x018e29, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x018e2a, 2);
    gsu_stw(R[10], R[14]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_code(0x018e2c, 3);
    R[0] = gsu_getb();
    R[0] = gsu_lob(R[0]);
    t = gsu.z;
    gsu_code(0x018e2f, 3);
    if (t) { gsu_code(0x018e3d, 1); goto l8E3D; }
    gsu_code(0x018e32, 3);
    gsu_r14(gsu_add(R[0], R[14], 0));
    R[0] = gsu_add(0x8E37, 5, 0);
    gsu_code(0x018e35, 4);
    gsu_stw(R[10], R[0]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_code(0x018e39, 4);
    nx = (uint16_t)(gsu_getb());
    at = model_command(nx);
    gsu_code(0x010000 | at, 1);   /* where it jumps, fetched before the delay slot runs */
    gsu_r14(gsu_inc(R[14]));
    goto fetched;
l8E3C:
    gsu_code(0x018e3d, 1);
    gsu_r14(gsu_inc(R[14]));
l8E3D:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x018e3e, 4);
    gsu_r14(gsu_ldw(R[10]));
    gsu_code(0x018e42, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x018e43, 2);
    nx = (uint16_t)(gsu_getb());
    at = model_command(nx);
    gsu_code(0x010000 | at, 1);   /* where it jumps, fetched before the delay slot runs */
    gsu_r14(gsu_inc(R[14]));
    goto fetched;
l8E45:
    gsu_code(0x018e46, 1);
    R[0] = gsu_getb();
    gsu_code(0x018e47, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x018e48, 2);
    R[0] = gsu_getbh(R[0]);
    R[12] = gsu_add(R[0], R[14], 0);
    gsu_code(0x018e4a, 3);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x018e4d, 2);
    gsu_stw(R[10], R[14]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_code(0x018e4f, 3);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x018e52, 2);
    gsu_stw(R[10], R[12]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[0] = gsu_add(0x8E58, 5, 0);
    gsu_code(0x018e54, 6);
    gsu_stw(R[10], R[0]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_code(0x018e5a, 4);
    nx = (uint16_t)(gsu_getb());
    at = model_command(nx);
    gsu_code(0x010000 | at, 1);   /* where it jumps, fetched before the delay slot runs */
    gsu_r14(gsu_inc(R[14]));
    goto fetched;
l8E5D:
    gsu_code(0x018e5e, 1);
    gsu_r14(gsu_inc(R[14]));
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x018e5f, 3);
    R[0] = gsu_ldw(R[10]);
    gsu_code(0x018e62, 3);
    R[1] = gsu_ldw(FX_MODEL_RETURN_AT);
    gsu_code(0x018e65, 1);
    gsu_stw(R[1], R[0]);
    R[1] = gsu_inc(R[1]);
    R[1] = gsu_inc(R[1]);
    gsu_code(0x018e66, 5);
    gsu_stw(FX_MODEL_RETURN_AT, R[1]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x018e6b, 4);
    gsu_r14(gsu_ldw(R[10]));
    gsu_code(0x018e6f, 1);
    R[0] = gsu_getb();
    R[0] = gsu_lob(R[0]);
    t = gsu.z;
    gsu_code(0x018e70, 3);
    if (t) { gsu_code(0x018e78, 1); goto l8E78; }
    gsu_code(0x018e73, 3);
    gsu_r14(gsu_add(R[0], R[14], 0));
    gsu_code(0x018e76, 2);
    nx = (uint16_t)(gsu_getb());
    at = model_command(nx);
    gsu_code(0x010000 | at, 1);   /* where it jumps, fetched before the delay slot runs */
    gsu_r14(gsu_inc(R[14]));
    goto fetched;
l8E78:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x018e79, 4);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
l8E7D:
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_code(0x018e7e, 4);
    R[1] = gsu_ldw(FX_MODEL_RETURN_AT);
    gsu_code(0x018e82, 1);
    gsu_stw(R[1], R[0]);
    R[0] = 0x0788;
    gsu_code(0x018e83, 6);
    gsu_stw(FX_MODEL_RETURN_AT, R[0]);
l8E88:
    gsu_code(0x018e89, 3);
    R[1] = gsu_ldw(FX_MODEL_RETURN_AT);
    gsu_code(0x018e8c, 2);
    gsu_r14(gsu_ldw(R[1]));
    R[1] = gsu_inc(R[1]);
    R[1] = gsu_inc(R[1]);
    gsu_code(0x018e8e, 5);
    gsu_stw(FX_MODEL_RETURN_AT, R[1]);
    gsu_code(0x018e93, 2);
    gsu_r14(R[14]);
    gsu_moves(R[14]);
    t = gsu.z;
    gsu_code(0x018e95, 2);
    if (t) { gsu_code(0x018ea3, 1); goto l8EA3; }
    R[0] = gsu_add(0x8E9A, 5, 0);
    gsu_code(0x018e97, 5);
    gsu_stw(R[10], R[0]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_code(0x018e9c, 4);
    nx = (uint16_t)(gsu_getb());
    at = model_command(nx);
    gsu_code(0x010000 | at, 1);   /* where it jumps, fetched before the delay slot runs */
    gsu_r14(gsu_inc(R[14]));
    goto fetched;
l8E9F:
    gsu_code(0x018ea0, 1);
    gsu_r14(gsu_inc(R[14]));
    t = 1;
    gsu_code(0x018ea1, 2);
    gsu_code(0x018e88, 1);
    if (t) goto l8E88;
l8EA3:
    gsu_code(0x018ea4, 3); gsu_code(0x019243, 1);
    goto l9243;
    /* FxModelDepthSort */
l8EA7:
    gsu_code(0x018ea8, 2);
    R[12] = gsu_getb();
    gsu_code(0x018eaa, 1);
    gsu_r14(gsu_inc(R[14]));
    R[0] = 0xF850;
    R[0] = gsu_add(R[0], R[12], 0);
    R[0] = gsu_add(R[0], R[14], 0);
    gsu_code(0x018eab, 8);
    gsu_stw(FX_MODEL_150, R[0]);
    gsu_code(0x018eb3, 3);
    gsu_stw(FX_MODEL_14E, R[12]);
    gsu_code(0x018eb6, 3);
    gsu_stw(FX_MODEL_148, R[12]);
    R[9] = 0x0344;
    R[5] = 0x07B0;
    R[13] = 0x8EC0;
    gsu_code(0x018eb9, 8);
l8EC0:
    gsu_code(0x018ec1, 1);
    R[0] = gsu_getb();
    gsu_code(0x018ec2, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x018ec3, 2);
    R[0] = gsu_umult(R[0], 6);
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_code(0x018ec5, 2);
    R[0] = gsu_ldw(R[0]);
    gsu_code(0x018ec7, 1);
    gsu_stw(R[5], R[0]);
    R[5] = gsu_inc(R[5]);
    t = gsu_count_down(&R[12]);
    R[5] = gsu_inc(R[5]);
    gsu_code(0x018ec8, 2);
    if (t) { gsu_code(0x018ec0, 1); goto l8EC0; }
    gsu_code(0x018eca, 1);
l8ECA:
    R[5] = 0x07B0;
    R[7] = 0x8000;
    gsu_code(0x018ecb, 9);
    R[12] = gsu_ldw(FX_MODEL_148);
    R[13] = 0x8ED5;
    gsu_code(0x018ed4, 2);
l8ED5:
    gsu_code(0x018ed6, 1);
    R[0] = gsu_ldw(R[5]);
    gsu_sub(R[0], R[7], 0);   /* CMP */
    t = gsu.s != gsu.ov;
    gsu_code(0x018ed7, 4);
    if (t) { gsu_code(0x018edf, 1); goto l8EDF; }
    R[7] = R[0];
    R[6] = R[5];
    gsu_code(0x018edb, 5);
l8EDF:
    R[5] = gsu_inc(R[5]);
    t = gsu_count_down(&R[12]);
    R[5] = gsu_inc(R[5]);
    gsu_code(0x018ee0, 2);
    if (t) { gsu_code(0x018ed5, 1); goto l8ED5; }
    R[0] = 0x8000;
    gsu_code(0x018ee2, 5);
    gsu_stw(R[6], R[0]);
    gsu_code(0x018ee7, 3);
    R[0] = gsu_ldw(FX_MODEL_150);
    gsu_code(0x018eea, 2);
    gsu_r14(gsu_add(R[0], R[6], 0));
    gsu_code(0x018eec, 1);
    R[0] = gsu_getb();
    gsu_code(0x018eed, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x018eee, 2);
    R[0] = gsu_getbh(R[0]);
    gsu_code(0x018ef0, 2);
    gsu_r14(R[0]);
    R[0] = gsu_add(0x8EF4, 5, 0);
    gsu_code(0x018ef2, 4);
    gsu_stw(R[10], R[0]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_code(0x018ef6, 4);
    nx = (uint16_t)(gsu_getb());
    at = model_command(nx);
    gsu_code(0x010000 | at, 1);   /* where it jumps, fetched before the delay slot runs */
    gsu_r14(gsu_inc(R[14]));
    goto fetched;
l8EF9:
    gsu_code(0x018efa, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x018efb, 3);
    R[0] = gsu_ldw(FX_MODEL_14E);
    R[0] = gsu_dec(R[0]);
    t = !gsu.z;
    gsu_code(0x018efe, 3);
    gsu_code(t ? 0x018eca : 0x018f01, 1);   /* the delay slot runs after the next fetch */
    gsu_sbk(R[0]);
    if (t) goto l8ECA;
    gsu_code(0x018f02, 3); gsu_code(0x019243, 1);
    goto l9243;
    /* FxModelIfSprite */
l8F05:
    gsu_code(0x018f06, 1);
    R[0] = gsu_getb();
    gsu_code(0x018f07, 1);
    gsu_r14(gsu_inc(R[14]));
    R[1] = 0x07C6;
    R[0] = gsu_add(R[0], R[1], 0);
    gsu_code(0x018f08, 6);
    R[0] = gsu_ldb(R[0]);
    R[0] = gsu_swap(R[0]);
    R[0] = gsu_hib(R[0]);
    t = gsu.s;
    gsu_code(0x018f0e, 4);
    if (t) { gsu_code(0x018f18, 1); goto l8F18; }
    gsu_code(0x018f12, 2);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x018f14, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x018f15, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x018f16, 2);
    nx = (uint16_t)(gsu_getb());
    at = model_command(nx);
    gsu_code(0x010000 | at, 1);   /* where it jumps, fetched before the delay slot runs */
    gsu_r14(gsu_inc(R[14]));
    goto fetched;
    /* FxModelSprite */
l8F18:
    gsu_code(0x018f19, 1);
    R[0] = gsu_getb();
    gsu_code(0x018f1a, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x018f1b, 3);
    R[3] = gsu_umult(R[0], 6);
    R[1] = 0x04E4;
    R[2] = gsu_add(R[3], R[1], 0);
    gsu_code(0x018f1e, 7);
    R[0] = gsu_ldw(R[2]);
    R[2] = gsu_inc(R[2]);
    R[2] = gsu_inc(R[2]);
    gsu_code(0x018f25, 5);
    gsu_stw(FX_MODEL_BA, R[0]);
    gsu_code(0x018f2a, 1);
    R[0] = gsu_ldw(R[2]);
    gsu_code(0x018f2b, 3);
    gsu_stw(FX_MODEL_BC, R[0]);
    R[1] = 0x04E4;
    R[1] = 0x0344;
    R[0] = gsu_add(R[3], R[1], 0);
    gsu_code(0x018f2e, 9);
    R[0] = gsu_ldw(R[0]);
    gsu_code(0x018f37, 3);
    R[1] = gsu_ldw(FX_OBJECT_Z);
    R[0] = gsu_add(R[0], R[1], 0);
    gsu_code(0x018f3a, 4);
    gsu_stw(FX_MODEL_BE, R[0]);
    R[1] = 0x0020;
    R[0] = gsu_sub(R[0], R[1], 0);
    t = gsu.s;
    gsu_code(0x018f3e, 6);
    if (t) { gsu_code(0x018f78, 1); goto l8F78; }
    R[11] = 0x8F49;
    gsu_code(0x018f44, 5); gsu_code(0x019250, 1);
    goto l9250;
l8F49:
    gsu_code(0x018f4a, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x018f4b, 1);
    R[0] = gsu_getb();
    gsu_code(0x018f4c, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x018f4d, 3);
    R[0] = gsu_ldw(FX_TEX_MASK);
    R[0] = gsu_lob(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_code(0x018f50, 5);
    gsu_stw(FX_MODEL_C8, R[0]);
    R[1] = 0x0100;
    R[0] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_swap(R[0]);
    t = !gsu.s;
    gsu_code(0x018f55, 7);
    if (t) { gsu_code(0x018f5e, 1); goto l8F5E; }
    R[1] = gsu_lsr(R[1]);
    gsu_code(0x018f5c, 3);
l8F5E:
    gsu_code(0x018f5f, 3);
    gsu_stw(FX_MODEL_CE, R[1]);
    gsu_code(0x018f62, 3);
    gsu_stw(FX_ROM_R14, R[14]);
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_code(0x018f65, 4);
    gsu_stw(FX_MODEL_D2, R[0]);
    R[11] = 0x8F6D;
    gsu_code(0x018f69, 4); gsu_code(0x01ab11, 1);
    goto lAB11;
l8F6D:
    gsu_code(0x018f6e, 3);
    R[0] = gsu_ldw(FX_ROM_BANK);
    gsu_code(0x018f71, 2);
    gsu_romb(R[0]);
    gsu_code(0x018f73, 3);
    gsu_r14(gsu_ldw(FX_ROM_R14));
    gsu_code(0x018f76, 2);
    nx = (uint16_t)(gsu_getb());
    at = model_command(nx);
    gsu_code(0x010000 | at, 1);   /* where it jumps, fetched before the delay slot runs */
    gsu_r14(gsu_inc(R[14]));
    goto fetched;
l8F78:
    gsu_code(0x018f79, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x018f7a, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x018f7b, 2);
    nx = (uint16_t)(gsu_getb());
    at = model_command(nx);
    gsu_code(0x010000 | at, 1);   /* where it jumps, fetched before the delay slot runs */
    gsu_r14(gsu_inc(R[14]));
    goto fetched;
    /* FxModelMaterial */
l8F7D:
    R[2] = R[14];
    gsu_code(0x018f7e, 5);
    R[0] = gsu_ldw(FX_MODEL_FLAGS);
    R[1] = 0x2000;
    R[0] = gsu_and(R[0], R[1]);
    t = gsu.z;
    gsu_code(0x018f83, 6);
    if (t) { gsu_code(0x018f98, 1); goto l8F98; }
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_code(0x018f89, 5);
    gsu_stw(FX_FACE_KIND, R[0]);
    R[0] = 0x0002;
    gsu_cmode(R[0]);
    R[0] = 0x0009;
    gsu_code(0x018f8e, 10); gsu_code(0x0190b6, 1);
    goto l90B6;
l8F98:
    gsu_code(0x018f99, 1);
    R[0] = gsu_getb();
    R[0] = gsu_add(R[0], R[0], 0);
    R[1] = 0x0003;
    gsu_code(0x018f9a, 6);
    gsu_romb(R[1]);
    gsu_code(0x018fa0, 3);
    R[1] = gsu_ldw(FX_MATERIAL_TABLE);
    gsu_code(0x018fa3, 2);
    gsu_r14(gsu_add(R[0], R[1], 0));
l8FA4:
    gsu_code(0x018fa5, 2);
    R[3] = gsu_getb();
    gsu_code(0x018fa7, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x018fa8, 3);
    R[3] = gsu_getbh(R[3]);
    R[3] = R[3];
    gsu_moves(R[3]);
    t = !gsu.s;
    gsu_code(0x018fab, 4);
    if (t) { gsu_code(0x018fe1, 1); goto l8FE1; }
    R[0] = gsu_add(R[3], R[3], 0);
    t = !gsu.s;
    gsu_code(0x018faf, 5);
    if (t) { gsu_code(0x018fcf, 1); goto l8FCF; }
    R[0] = 0xFFFE;
    gsu_code(0x018fb4, 7);
    gsu_stw(FX_FACE_KIND, R[0]);
    R[0] = gsu_hib(R[3]);
    R[0] = gsu_and(R[0], 15);
    R[0] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_swap(R[0]);
    R[0] = gsu_or(R[0], 8);
    R[0] = gsu_swap(R[0]);
    gsu_code(0x018fbb, 15);
    gsu_stw(FX_FACE_COLOUR, R[0]);
    R[0] = 0xFFFE;
    gsu_code(0x018fca, 5); gsu_code(0x0190b6, 1);
    goto l90B6;
l8FCF:
    R[0] = 0x1FFF;
    gsu_code(0x018fd0, 5);
    gsu_r14(gsu_and(R[0], R[3]));
    gsu_code(0x018fd5, 3);
    R[0] = gsu_ldw(FX_MATERIAL_MASK);
    gsu_code(0x018fd8, 2);
    R[3] = gsu_getb();
    R[3] = gsu_dec(R[3]);
    R[0] = gsu_and(R[0], R[3]);
    R[0] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_inc(R[0]);
    t = 1;
    gsu_code(0x018fda, 7); gsu_code(0x018fa4, 1);
    gsu_r14(gsu_add(R[0], R[14], 0));
    if (t) goto l8FA4;
l8FE1:
    R[0] = gsu_lob(R[3]);
    R[1] = 0x9FB1;
    gsu_code(0x018fe2, 7);
    gsu_r14(gsu_add(R[0], R[1], 0));
    gsu_code(0x018fe9, 2);
    R[1] = gsu_getb();
    R[0] = 0x0800;
    R[0] = gsu_add(R[0], R[1], 0);
    gsu_code(0x018feb, 7);
    gsu_stw(FX_FACE_COLOUR, R[0]);
    R[0] = gsu_add(R[3], R[3], 0);
    t = !gsu.s;
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_code(0x018ff2, 4);
    if (t) { gsu_code(0x018ff7, 1); goto l8FF7; }
    R[0] = gsu_dec(R[0]);
    gsu_code(0x018ff6, 2);
l8FF7:
    gsu_code(0x018ff8, 3);
    gsu_stw(FX_FACE_KIND, R[0]);
    t = gsu.z;
    gsu_code(0x018ffb, 2);
    if (t) { gsu_code(0x019045, 1); goto l9045; }
    R[0] = 0x2000;
    R[0] = gsu_and(R[0], R[3]);
    t = gsu.z;
    gsu_code(0x018ffd, 7);
    if (t) { gsu_code(0x01900b, 1); goto l900B; }
    R[0] = 0x0C00;
    R[0] = gsu_add(R[0], R[1], 0);
    gsu_code(0x019004, 8);
    gsu_stw(FX_FACE_COLOUR, R[0]);
l900B:
    R[0] = gsu_lob(R[3]);
    gsu_code(0x01900c, 4);
    R[0] = gsu_umult(R[0], 3);
    R[1] = 0x9D59;
    gsu_code(0x019010, 5);
    gsu_r14(gsu_add(R[0], R[1], 0));
    gsu_code(0x019015, 1);
    R[0] = gsu_getb();
    gsu_code(0x019016, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x019017, 2);
    R[0] = gsu_getbh(R[0]);
    gsu_code(0x019019, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x01901a, 3);
    gsu_stw(FX_TEX_BASE, R[0]);
    gsu_code(0x01901d, 1);
    R[0] = gsu_getb();
    gsu_code(0x01901e, 3);
    gsu_stw(FX_TEX_BANK, R[0]);
    R[0] = gsu_hib(R[3]);
    R[1] = 0x001F;
    R[0] = gsu_and(R[0], R[1]);
    R[0] = gsu_add(R[0], R[0], 0);
    R[1] = 0xA079;
    gsu_code(0x019021, 11);
    gsu_r14(gsu_add(R[0], R[1], 0));
    gsu_code(0x01902c, 1);
    R[0] = gsu_getb();
    gsu_code(0x01902d, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x01902e, 2);
    R[0] = gsu_getbh(R[0]);
    gsu_code(0x019030, 2);
    gsu_r14(R[0]);
    gsu_code(0x019032, 2);
    R[1] = gsu_getb();
    gsu_code(0x019034, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x019035, 3);
    R[1] = gsu_getbh(R[1]);
    gsu_code(0x019038, 3);
    gsu_stw(FX_TEX_MASK, R[1]);
    R[0] = gsu_inc(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_code(0x01903b, 5);
    gsu_stw(FX_UV_AT, R[0]);
    R[0] = 0xFFFF;
    gsu_code(0x019040, 5); gsu_code(0x0190b6, 1);
    goto l90B6;
l9045:
    R[1] = 0x003F;
    R[0] = gsu_hib(R[3]);
    R[0] = gsu_inc(R[0]);
    R[0] = gsu_and(R[0], R[1]);
    t = gsu.z;
    gsu_code(0x019046, 8);
    if (t) { gsu_code(0x0190b3, 1); goto l90B3; }
    R[0] = gsu_inc(R[0]);
    R[0] = gsu_and(R[0], R[1]);
    t = !gsu.z;
    gsu_code(0x01904e, 5);
    if (t) { gsu_code(0x01905e, 1); goto l905E; }
    R[0] = gsu_lob(R[3]);
    gsu_code(0x019053, 6);
    R[3] = gsu_ldw(FX_SHADE_AT);
    gsu_code(0x019059, 2);
    gsu_r14(gsu_add(R[0], R[3], 0));
    gsu_code(0x01905b, 1);
    R[0] = gsu_getb();
    t = 1;
    gsu_color((uint8_t)R[0]);
    gsu_code(0x01905c, 2);
    gsu_code(0x0190b6, 1);
    if (t) goto l90B6;
l905E:
    R[0] = gsu_dec(R[0]);
    R[0] = gsu_dec(R[0]);
    R[0] = gsu_and(R[0], R[1]);
    R[3] = R[0];
    gsu_code(0x01905f, 8);
    R[0] = gsu_ldw(FX_SHADE_TABLE);
    R[0] = gsu_add(R[0], R[3], 0);
    gsu_code(0x019067, 3);
    gsu_r14(gsu_add(R[0], R[3], 0));
    gsu_code(0x01906a, 2);
    R[6] = gsu_getb();
    gsu_code(0x01906c, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x01906d, 3);
    R[6] = gsu_getbh(R[6]);
    gsu_code(0x019070, 3);
    R[0] = gsu_ldw(FX_ROM_BANK);
    gsu_code(0x019073, 2);
    gsu_romb(R[0]);
    gsu_code(0x019075, 2);
    gsu_r14(R[2]);
    gsu_code(0x019077, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x019078, 1);
    R[0] = gsu_getb();
    gsu_code(0x019079, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x01907a, 3);
    R[4] = gsu_ldw(FX_LIGHT_LOCAL_X);
    gsu_code(0x01907d, 2);
    R[5] = gsu_mult(R[0], R[4]);
    gsu_code(0x01907f, 1);
    R[0] = gsu_getb();
    gsu_code(0x019080, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x019081, 3);
    R[4] = gsu_ldw(FX_LIGHT_LOCAL_Y);
    gsu_code(0x019084, 1);
    R[0] = gsu_mult(R[0], R[4]);
    R[5] = gsu_add(R[0], R[5], 0);
    gsu_code(0x019085, 3);
    R[0] = gsu_getb();
    gsu_code(0x019088, 3);
    R[4] = gsu_ldw(FX_LIGHT_LOCAL_Z);
    gsu_code(0x01908b, 1);
    R[0] = gsu_mult(R[0], R[4]);
    R[0] = gsu_add(R[0], R[5], 0);
    R[0] = gsu_asr(R[0]);
    R[0] = gsu_asr(R[0]);
    R[0] = gsu_hib(R[0]);
    R[0] = gsu_sex(R[0]);
    R[4] = 0x0006;
    gsu_sub(R[0], R[4], 0);   /* CMP */
    t = gsu.s == gsu.ov;
    gsu_code(0x01908c, 11);
    if (t) { gsu_code(0x01909c, 1); goto l909C; }
    R[0] = 0x0006;
    t = 1;
    gsu_code(0x019097, 5);
    gsu_code(0x0190a5, 1);
    if (t) goto l90A5;
l909C:
    R[4] = 0x0010;
    gsu_sub(R[0], R[4], 0);   /* CMP */
    t = gsu.s != gsu.ov;
    gsu_code(0x01909d, 6);
    if (t) { gsu_code(0x0190a5, 1); goto l90A5; }
    R[0] = 0x000F;
    gsu_code(0x0190a3, 3);
l90A5:
    R[1] = gsu_sub(R[0], 6, 0);
    R[0] = 0x0003;
    gsu_code(0x0190a6, 7);
    gsu_romb(R[0]);
    gsu_code(0x0190ad, 3);
    gsu_r14(gsu_add(R[6], R[1], 0));
    gsu_code(0x0190b0, 1);
    R[0] = gsu_getb();
    t = 1;
    gsu_color((uint8_t)R[0]);
    gsu_code(0x0190b1, 2);
    gsu_code(0x0190b6, 1);
    if (t) goto l90B6;
l90B3:
    R[0] = gsu_lob(R[3]);
    gsu_color((uint8_t)R[0]);
    gsu_code(0x0190b4, 3);
l90B6:
    gsu_code(0x0190b7, 3);
    R[1] = gsu_ldw(FX_ROM_BANK);
    gsu_code(0x0190ba, 3);
    gsu_romb(R[1]);
    gsu_code(0x0190bd, 2);
    gsu_r14(R[2]);
    nx = (uint16_t)(R[11]);
    at = nx;
    gsu_code(0x0190bf, 1);
    goto dispatch;
l90C0:
    R[0] = gsu_inc(R[0]);
    R[0] = gsu_lob(R[0]);
    t = gsu.z;
    gsu_code(0x0190c1, 4);
    if (t) { gsu_code(0x0190c8, 1); goto l90C8; }
    gsu_code(0x0190c5, 3);
    nx = (uint16_t)(gsu_getb());
    at = model_command(nx);
    gsu_code(0x010000 | at, 1);   /* where it jumps, fetched before the delay slot runs */
    gsu_r14(gsu_inc(R[14]));
    goto fetched;
l90C8:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x0190c9, 4);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
    /* FxModelFaces */
l90CD:
    /* CACHE */
    gsu_code(0x0190ce, 2);
    R[0] = gsu_getb();
    R[0] = gsu_lob(R[0]);
    t = gsu.s;
    gsu_code(0x0190d0, 3);
    gsu_code(t ? 0x0190c0 : 0x0190d3, 1);   /* the delay slot runs after the next fetch */
    gsu_r14(gsu_inc(R[14]));
    if (t) goto l90C0;
    gsu_code(0x0190d4, 1);
    gsu_r14(gsu_inc(R[14]));
    R[2] = R[0];
    R[0] = gsu_sub(R[0], 2, 0);
    t = gsu.z;
    gsu_code(0x0190d5, 6);
    gsu_code(t ? 0x0190ef : 0x0190db, 1);   /* the delay slot runs after the next fetch */
    R[0] = gsu_getb();
    if (t) goto l90EF;
    R[1] = 0x07C6;
    R[0] = gsu_add(R[0], R[1], 0);
    gsu_code(0x0190dc, 6);
    R[0] = gsu_ldb(R[0]);
    R[0] = gsu_lob(R[0]);
    t = gsu.s;
    gsu_code(0x0190e2, 3);
    gsu_code(t ? 0x0190f0 : 0x0190e5, 1);   /* the delay slot runs after the next fetch */
    gsu_r14(gsu_inc(R[14]));
    if (t) goto l90F0;
    t = gsu.z;
    gsu_code(0x0190e6, 2);
    if (t) { gsu_code(0x0190f0, 1); goto l90F0; }
    R[2] = gsu_add(R[2], 4, 0);
    t = 1;
    gsu_code(0x0190e8, 7); gsu_code(0x0190cd, 1); gsu_cache(0x90ce);
    gsu_r14(gsu_add(R[14], R[2], 0));
    if (t) goto l90CD;
l90EF:
    gsu_code(0x0190f0, 1);
    gsu_r14(gsu_inc(R[14]));
l90F0:
    gsu_code(0x0190f1, 3);
    gsu_stw(FX_POINT_COUNT, R[2]);
    R[0] = 0x09AA;
    gsu_code(0x0190f4, 6);
    R[6] = gsu_ldw(FX_LIST_END);
    R[0] = gsu_sub(R[0], R[6], 0);
    t = !gsu.s;
    gsu_code(0x0190fa, 3);
    if (t) { gsu_code(0x01910d, 1); goto l910D; }
    gsu_code(0x0190fd, 3);
    gsu_stw(R[10], R[14]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[11] = 0x9106;
    gsu_code(0x019100, 6); gsu_code(0x01a931, 1);
    abi_FxDrawList();
    gsu_code(0x019106, 1);
l9106:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x019107, 4);
    gsu_r14(gsu_ldw(R[10]));
    gsu_code(0x01910b, 3);
    R[6] = gsu_ldw(FX_LIST_END);
l910D:
    R[11] = 0x9112;
    gsu_code(0x01910e, 4); gsu_code(0x018f7d, 1);
    goto l8F7D;
l9112:
    gsu_code(0x019113, 3);
    R[6] = gsu_ldw(FX_LIST_END);
    R[3] = R[0];
    gsu_code(0x019116, 5);
    gsu_r14(gsu_add(R[14], 4, 0));
    gsu_code(0x01911b, 3);
    R[12] = gsu_ldw(FX_POINT_COUNT);
    R[0] = gsu_lob(R[3]);
    R[0] = gsu_swap(R[0]);
    R[0] = gsu_or(R[0], R[12]);
    R[3] = R[3];
    gsu_moves(R[3]);
    t = !gsu.s;
    gsu_code(0x01911e, 8);
    if (t) { gsu_code(0x019142, 1); goto l9142; }
    R[0] = gsu_not(R[12]);
    R[1] = gsu_lob(R[0]);
    gsu_code(0x019126, 8);
    R[0] = gsu_ldw(FX_TEX_BANK);
    R[0] = gsu_lob(R[0]);
    R[0] = gsu_swap(R[0]);
    R[0] = gsu_or(R[0], R[1]);
    gsu_code(0x01912e, 4);
    gsu_stw(R[6], R[0]);
    R[6] = gsu_inc(R[6]);
    R[6] = gsu_inc(R[6]);
    gsu_code(0x019132, 5);
    R[0] = gsu_ldw(FX_FACE_COLOUR);
    gsu_code(0x019137, 1);
    gsu_stw(R[6], R[0]);
    R[6] = gsu_inc(R[6]);
    R[6] = gsu_inc(R[6]);
    gsu_code(0x019138, 5);
    R[0] = gsu_ldw(FX_TEX_BASE);
    gsu_code(0x01913d, 1);
    gsu_stw(R[6], R[0]);
    R[6] = gsu_inc(R[6]);
    R[6] = gsu_inc(R[6]);
    gsu_code(0x01913e, 5);
    R[0] = gsu_ldw(FX_TEX_MASK);
l9142:
    gsu_code(0x019143, 1);
    gsu_stw(R[6], R[0]);
    R[6] = gsu_inc(R[6]);
    R[6] = gsu_inc(R[6]);
    R[7] = R[6];
    R[9] = 0x04E4;
    R[5] = 0x0000;
    R[13] = 0x914E;
    gsu_code(0x019144, 11);
l914E:
    gsu_code(0x01914f, 2);
    R[4] = gsu_getb();
    gsu_code(0x019151, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x019152, 3);
    R[0] = gsu_umult(R[4], 6);
    R[8] = gsu_add(R[0], R[9], 0);
    gsu_code(0x019155, 3);
    R[0] = gsu_ldw(R[8]);
    gsu_code(0x019158, 1);
    gsu_stw(R[6], R[0]);
    R[6] = gsu_inc(R[6]);
    R[6] = gsu_inc(R[6]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    gsu_code(0x019159, 5);
    R[0] = gsu_ldw(R[8]);
    gsu_code(0x01915e, 1);
    gsu_stw(R[6], R[0]);
    R[6] = gsu_inc(R[6]);
    R[6] = gsu_inc(R[6]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    gsu_code(0x01915f, 5);
    R[0] = gsu_ldw(R[8]);
    R[5] = gsu_or(R[0], R[5]);
    R[3] = R[3];
    gsu_moves(R[3]);
    t = !gsu.s;
    gsu_code(0x019164, 6);
    if (t) { gsu_code(0x01918b, 1); goto l918B; }
    gsu_code(0x01916a, 4);
    gsu_stw(FX_ROM_R14, R[14]);
    R[0] = 0x0003;
    gsu_code(0x01916e, 4);
    gsu_romb(R[0]);
    gsu_code(0x019172, 3);
    gsu_r14(gsu_ldw(FX_UV_AT));
    gsu_code(0x019175, 1);
    R[0] = gsu_getb();
    gsu_code(0x019176, 1);
    gsu_r14(gsu_inc(R[14]));
    R[0] = gsu_swap(R[0]);
    gsu_code(0x019177, 2);
    gsu_stw(R[6], R[0]);
    R[6] = gsu_inc(R[6]);
    R[6] = gsu_inc(R[6]);
    gsu_code(0x019179, 3);
    R[0] = gsu_getb();
    gsu_code(0x01917c, 1);
    gsu_r14(gsu_inc(R[14]));
    R[0] = gsu_swap(R[0]);
    gsu_code(0x01917d, 2);
    gsu_stw(R[6], R[0]);
    R[6] = gsu_inc(R[6]);
    R[6] = gsu_inc(R[6]);
    gsu_code(0x01917f, 5);
    R[0] = gsu_ldw(FX_ROM_BANK);
    gsu_code(0x019184, 3);
    gsu_stw(FX_UV_AT, R[14]);
    gsu_code(0x019187, 2);
    gsu_romb(R[0]);
    gsu_code(0x019189, 3);
    gsu_r14(gsu_ldw(FX_ROM_R14));
l918B:
    t = gsu_count_down(&R[12]);
    gsu_code(0x01918c, 1);
    if (t) { gsu_code(0x01914e, 1); goto l914E; }
    R[0] = gsu_lob(R[5]);
    t = !gsu.z;
    gsu_code(0x01918d, 5);
    if (t) { gsu_code(0x019199, 1); goto l9199; }
    gsu_code(0x019192, 4);
    gsu_stw(FX_LIST_END, R[6]);
    gsu_code(0x019196, 3); gsu_code(0x0190cd, 1); gsu_cache(0x90ce);
    goto l90CD;
l9199:
    R[1] = 0x1F00;
    R[0] = gsu_and(R[1], R[5]);
    R[0] = gsu_sub(R[0], R[1], 0);
    t = gsu.z;
    gsu_code(0x01919a, 8);
    if (t) { gsu_code(0x0191a6, 1); goto l91A6; }
    gsu_code(0x0191a2, 4); gsu_code(0x01923f, 1);
    goto l923F;
l91A6:
    gsu_code(0x0191a7, 4);
    gsu_stw(FX_OUTCODES, R[5]);
    gsu_code(0x0191ab, 3);
    gsu_stw(FX_ROM_R14, R[14]);
    gsu_code(0x0191ae, 3);
    gsu_stw(FX_POLY_OUT, R[7]);
    R[0] = gsu_lob(R[5]);
    t = gsu.z;
    gsu_code(0x0191b1, 4);
    if (t) { gsu_code(0x0191ca, 1); goto l91CA; }
    gsu_code(0x0191b5, 4);
    R[0] = gsu_ldw(FX_FACE_KIND);
    R[0] = gsu_hib(R[0]);
    t = gsu.z;
    gsu_code(0x0191b9, 3);
    if (t) { gsu_code(0x0191ca, 1); goto l91CA; }
    R[11] = 0x91C1;
    gsu_code(0x0191bc, 5); gsu_code(0x01c339, 1);
    abi_FxPolygonDraw();
    gsu_code(0x0191c1, 1);
l91C1:
    R[0] = R[0];
    gsu_moves(R[0]);
    t = !gsu.z;
    gsu_code(0x0191c2, 4);
    if (t) { gsu_code(0x01920f, 1); goto l920F; }
    gsu_code(0x0191c6, 4); gsu_code(0x01923c, 1);
    goto l923C;
l91CA:
    R[0] = 0x0010;
    R[0] = gsu_and(R[0], R[5]);
    t = gsu.z;
    gsu_code(0x0191cb, 5);
    if (t) { gsu_code(0x0191eb, 1); goto l91EB; }
    gsu_code(0x0191d0, 4);
    R[0] = gsu_ldw(FX_POINT_COUNT);
    gsu_code(0x0191d4, 2);
    gsu_r14(gsu_sub(R[14], R[0], 0));
    R[11] = 0x91DA;
    gsu_code(0x0191d6, 4); gsu_code(0x0198d7, 1);
    abi_FxPolygonGather();
    gsu_code(0x0191da, 1);
l91DA:
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.z;
    gsu_code(0x0191db, 4);
    if (t) { gsu_code(0x01923c, 1); goto l923C; }
    gsu_code(0x0191df, 4);
    R[1] = gsu_ldw(FX_FACE_KIND);
    R[1] = gsu_hib(R[1]);
    t = gsu.z;
    gsu_code(0x0191e3, 4);
    if (t) { gsu_code(0x0191f5, 1); goto l91F5; }
    gsu_code(0x0191e7, 1);
l91E7:
    gsu_code(0x0191e8, 3); gsu_code(0x019205, 1);
    goto l9205;
l91EB:
    gsu_code(0x0191ec, 3);
    R[0] = gsu_ldw(FX_FACE_KIND);
    R[0] = gsu_hib(R[0]);
    t = !gsu.z;
    gsu_code(0x0191ef, 3);
    if (t) { gsu_code(0x019202, 1); goto l9202; }
    gsu_code(0x0191f2, 4);
    R[0] = gsu_ldw(FX_POINT_COUNT);
l91F5:
    R[11] = 0x91FA;
    gsu_code(0x0191f6, 4); gsu_code(0x01a25f, 1);
    abi_FxPolygonClipScreen();
    gsu_code(0x0191fa, 1);
l91FA:
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.z;
    gsu_code(0x0191fb, 4);
    if (t) { gsu_code(0x01923c, 1); goto l923C; }
    t = 1;
    gsu_code(0x0191ff, 3);
    gsu_code(0x01920f, 1);
    if (t) goto l920F;
l9202:
    gsu_code(0x019203, 3);
    R[0] = gsu_ldw(FX_POINT_COUNT);
l9205:
    R[11] = 0x920A;
    gsu_code(0x019206, 4); gsu_code(0x019a52, 1);
    goto l9A52;
l920A:
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.z;
    gsu_code(0x01920b, 4);
    if (t) { gsu_code(0x01923c, 1); goto l923C; }
    gsu_code(0x01920f, 1);
l920F:
    gsu_code(0x019210, 3);
    R[3] = gsu_ldw(FX_FACE_KIND);
    gsu_code(0x019213, 3);
    R[6] = gsu_ldw(FX_POLY_OUT);
    R[2] = R[0];
    R[3] = R[3];
    gsu_moves(R[3]);
    t = !gsu.s;
    R[0] = gsu_add(R[0], R[0], 0);
    gsu_code(0x019216, 6);
    if (t) { gsu_code(0x01922c, 1); goto l922C; }
    R[0] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_add(R[0], R[6], 0);
    gsu_code(0x01921c, 7);
    gsu_stw(FX_LIST_END, R[0]);
    R[6] = gsu_sub(R[6], 8, 0);
    R[0] = gsu_not(R[2]);
    gsu_code(0x019223, 7);
    gsu_stb(R[6], (uint8_t)R[0]);
    t = 1;
    gsu_code(0x01922a, 2);
    gsu_code(0x019237, 1);
    if (t) goto l9237;
l922C:
    R[0] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_add(R[0], R[6], 0);
    gsu_code(0x01922d, 5);
    gsu_stw(FX_LIST_END, R[0]);
    R[6] = gsu_sub(R[6], 2, 0);
    gsu_code(0x019232, 6);
    gsu_stb(R[6], (uint8_t)R[2]);
l9237:
    gsu_code(0x019238, 3);
    R[0] = gsu_ldw(FX_ROM_BANK);
    gsu_code(0x01923b, 2);
    gsu_romb(R[0]);
l923C:
    gsu_code(0x01923d, 3);
    gsu_r14(gsu_ldw(FX_ROM_R14));
l923F:
    gsu_code(0x019240, 3); gsu_code(0x0190cd, 1); gsu_cache(0x90ce);
    goto l90CD;
    /* FxModelEnd */
l9243:
    R[11] = 0x9248;
    gsu_code(0x019244, 4); gsu_code(0x01a931, 1);
    abi_FxDrawList();
    gsu_code(0x019248, 1);
l9248:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x019249, 4);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
l924D:
    t = 1;
    gsu_code(0x01924e, 2);
    gsu_code(0x019248, 1);
    if (t) goto l9248;
    /* FxModelSpriteColour */
l9250:
    R[2] = R[14];
    gsu_code(0x019251, 5);
    R[0] = gsu_ldw(FX_MODEL_FLAGS);
    R[1] = 0x2000;
    R[1] = gsu_and(R[0], R[1]);
    t = gsu.z;
    gsu_code(0x019256, 7);
    if (t) { gsu_code(0x019275, 1); goto l9275; }
    R[1] = 0x1000;
    R[0] = gsu_and(R[0], R[1]);
    t = !gsu.z;
    gsu_code(0x01925d, 7);
    if (t) { gsu_code(0x019275, 1); goto l9275; }
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_cmode(R[0]);
    R[0] = 0x0009;
    gsu_color((uint8_t)R[0]);
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_code(0x019264, 11);
    gsu_stw(FX_FACE_COLOUR, R[0]);
    gsu_code(0x01926f, 3);
    gsu_stw(FX_FACE_KIND, R[0]);
    gsu_code(0x019272, 3); gsu_code(0x01939e, 1);
    goto l939E;
l9275:
    gsu_code(0x019276, 1);
    R[0] = gsu_getb();
    /* FxSpriteMaterial */
l9276:
    R[0] = gsu_add(R[0], R[0], 0);
    R[1] = 0x0003;
    gsu_code(0x019277, 6);
    gsu_romb(R[1]);
    gsu_code(0x01927d, 3);
    R[1] = gsu_ldw(FX_MATERIAL_TABLE);
    gsu_code(0x019280, 2);
    gsu_r14(gsu_add(R[0], R[1], 0));
l9281:
    gsu_code(0x019282, 2);
    R[3] = gsu_getb();
    gsu_code(0x019284, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x019285, 3);
    R[3] = gsu_getbh(R[3]);
    R[3] = R[3];
    gsu_moves(R[3]);
    t = !gsu.s;
    gsu_code(0x019288, 4);
    if (t) { gsu_code(0x0192bc, 1); goto l92BC; }
    R[0] = gsu_add(R[3], R[3], 0);
    t = !gsu.s;
    gsu_code(0x01928c, 5);
    if (t) { gsu_code(0x0192aa, 1); goto l92AA; }
    R[0] = 0xFFFE;
    gsu_code(0x019291, 7);
    gsu_stw(FX_FACE_KIND, R[0]);
    R[0] = gsu_hib(R[3]);
    R[0] = gsu_and(R[0], 15);
    R[0] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_swap(R[0]);
    R[0] = gsu_or(R[0], 8);
    R[0] = gsu_swap(R[0]);
    gsu_code(0x019298, 15);
    gsu_stw(FX_FACE_COLOUR, R[0]);
    gsu_code(0x0192a7, 3); gsu_code(0x01939e, 1);
    goto l939E;
l92AA:
    R[0] = 0x1FFF;
    gsu_code(0x0192ab, 5);
    gsu_r14(gsu_and(R[0], R[3]));
    gsu_code(0x0192b0, 3);
    R[0] = gsu_ldw(FX_MATERIAL_MASK);
    gsu_code(0x0192b3, 2);
    R[3] = gsu_getb();
    R[3] = gsu_dec(R[3]);
    R[0] = gsu_and(R[0], R[3]);
    R[0] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_inc(R[0]);
    t = 1;
    gsu_code(0x0192b5, 7); gsu_code(0x019281, 1);
    gsu_r14(gsu_add(R[0], R[14], 0));
    if (t) goto l9281;
l92BC:
    R[0] = gsu_lob(R[3]);
    R[1] = 0x9FB1;
    gsu_code(0x0192bd, 7);
    gsu_r14(gsu_add(R[0], R[1], 0));
    gsu_code(0x0192c4, 2);
    R[1] = gsu_getb();
    R[0] = 0x0800;
    R[0] = gsu_add(R[0], R[1], 0);
    gsu_code(0x0192c6, 7);
    gsu_stw(FX_FACE_COLOUR, R[0]);
    R[0] = gsu_add(R[3], R[3], 0);
    t = !gsu.s;
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_code(0x0192cd, 4);
    if (t) { gsu_code(0x0192d2, 1); goto l92D2; }
    R[0] = gsu_dec(R[0]);
    gsu_code(0x0192d1, 2);
l92D2:
    gsu_code(0x0192d3, 3);
    gsu_stw(FX_FACE_KIND, R[0]);
    t = gsu.z;
    gsu_code(0x0192d6, 2);
    if (t) { gsu_code(0x01931e, 1); goto l931E; }
    R[0] = 0x2000;
    R[0] = gsu_and(R[0], R[3]);
    t = gsu.z;
    gsu_code(0x0192d8, 7);
    if (t) { gsu_code(0x0192e6, 1); goto l92E6; }
    R[0] = 0x0C00;
    R[0] = gsu_add(R[0], R[1], 0);
    gsu_code(0x0192df, 8);
    gsu_stw(FX_FACE_COLOUR, R[0]);
l92E6:
    R[0] = gsu_lob(R[3]);
    gsu_code(0x0192e7, 4);
    R[0] = gsu_umult(R[0], 3);
    R[1] = 0x9D59;
    gsu_code(0x0192eb, 5);
    gsu_r14(gsu_add(R[0], R[1], 0));
    gsu_code(0x0192f0, 1);
    R[0] = gsu_getb();
    gsu_code(0x0192f1, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x0192f2, 2);
    R[0] = gsu_getbh(R[0]);
    gsu_code(0x0192f4, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x0192f5, 3);
    gsu_stw(FX_TEX_BASE, R[0]);
    gsu_code(0x0192f8, 1);
    R[0] = gsu_getb();
    gsu_code(0x0192f9, 3);
    gsu_stw(FX_TEX_BANK, R[0]);
    R[0] = gsu_hib(R[3]);
    R[1] = 0x001F;
    R[0] = gsu_and(R[0], R[1]);
    R[0] = gsu_add(R[0], R[0], 0);
    R[1] = 0xA079;
    gsu_code(0x0192fc, 11);
    gsu_r14(gsu_add(R[0], R[1], 0));
    gsu_code(0x019307, 1);
    R[0] = gsu_getb();
    gsu_code(0x019308, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x019309, 2);
    R[0] = gsu_getbh(R[0]);
    gsu_code(0x01930b, 2);
    gsu_r14(R[0]);
    gsu_code(0x01930d, 2);
    R[1] = gsu_getb();
    gsu_code(0x01930f, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x019310, 3);
    R[1] = gsu_getbh(R[1]);
    gsu_code(0x019313, 3);
    gsu_stw(FX_TEX_MASK, R[1]);
    R[0] = gsu_inc(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_code(0x019316, 5);
    gsu_stw(FX_UV_AT, R[0]);
    gsu_code(0x01931b, 3); gsu_code(0x01939e, 1);
    goto l939E;
l931E:
    R[1] = 0x003F;
    R[0] = gsu_hib(R[3]);
    R[0] = gsu_inc(R[0]);
    R[0] = gsu_and(R[0], R[1]);
    t = !gsu.z;
    gsu_code(0x01931f, 8);
    if (t) { gsu_code(0x01932b, 1); goto l932B; }
    gsu_code(0x019327, 4); gsu_code(0x01939b, 1);
    goto l939B;
l932B:
    R[0] = gsu_inc(R[0]);
    R[0] = gsu_and(R[0], R[1]);
    t = gsu.z;
    gsu_code(0x01932c, 4);
    if (t) { gsu_code(0x019334, 1); goto l9334; }
    gsu_code(0x019330, 4); gsu_code(0x019340, 1);
    goto l9340;
l9334:
    R[0] = gsu_lob(R[3]);
    gsu_code(0x019335, 5);
    R[3] = gsu_ldw(FX_SHADE_AT);
    gsu_code(0x01933a, 2);
    gsu_r14(gsu_add(R[0], R[3], 0));
    gsu_code(0x01933c, 1);
    gsu_color(gsu_getb());   /* GETC */
    gsu_code(0x01933d, 3); gsu_code(0x01939e, 1);
    goto l939E;
l9340:
    R[0] = gsu_dec(R[0]);
    R[0] = gsu_dec(R[0]);
    R[0] = gsu_and(R[0], R[1]);
    R[3] = R[0];
    gsu_code(0x019341, 8);
    R[0] = gsu_ldw(FX_MODEL_FLAGS);
    R[0] = gsu_hib(R[0]);
    t = !gsu.s;
    gsu_code(0x019349, 3);
    if (t) { gsu_code(0x01939b, 1); goto l939B; }
    gsu_code(0x01934c, 4);
    R[0] = gsu_ldw(FX_SHADE_TABLE);
    R[0] = gsu_add(R[0], R[3], 0);
    gsu_code(0x019350, 3);
    gsu_r14(gsu_add(R[0], R[3], 0));
    gsu_code(0x019353, 2);
    R[6] = gsu_getb();
    gsu_code(0x019355, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x019356, 3);
    R[6] = gsu_getbh(R[6]);
    gsu_code(0x019359, 3);
    R[0] = gsu_ldw(FX_ROM_BANK);
    gsu_code(0x01935c, 2);
    gsu_romb(R[0]);
    gsu_code(0x01935e, 2);
    gsu_r14(R[2]);
    gsu_code(0x019360, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x019361, 1);
    R[0] = gsu_getb();
    gsu_code(0x019362, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x019363, 3);
    R[4] = gsu_ldw(FX_LIGHT_LOCAL_X);
    gsu_code(0x019366, 2);
    R[5] = gsu_mult(R[0], R[4]);
    gsu_code(0x019368, 1);
    R[0] = gsu_getb();
    gsu_code(0x019369, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x01936a, 3);
    R[4] = gsu_ldw(FX_LIGHT_LOCAL_Y);
    gsu_code(0x01936d, 1);
    R[0] = gsu_mult(R[0], R[4]);
    R[5] = gsu_add(R[0], R[5], 0);
    gsu_code(0x01936e, 3);
    R[0] = gsu_getb();
    gsu_code(0x019371, 3);
    R[4] = gsu_ldw(FX_LIGHT_LOCAL_Z);
    gsu_code(0x019374, 1);
    R[0] = gsu_mult(R[0], R[4]);
    R[0] = gsu_add(R[0], R[5], 0);
    R[0] = gsu_asr(R[0]);
    R[0] = gsu_asr(R[0]);
    R[0] = gsu_hib(R[0]);
    R[0] = gsu_sex(R[0]);
    R[4] = 0x0006;
    gsu_sub(R[0], R[4], 0);   /* CMP */
    t = gsu.s == gsu.ov;
    gsu_code(0x019375, 11);
    if (t) { gsu_code(0x019385, 1); goto l9385; }
    R[0] = 0x0006;
    t = 1;
    gsu_code(0x019380, 5);
    gsu_code(0x01938e, 1);
    if (t) goto l938E;
l9385:
    R[4] = 0x0010;
    gsu_sub(R[0], R[4], 0);   /* CMP */
    t = gsu.s != gsu.ov;
    gsu_code(0x019386, 6);
    if (t) { gsu_code(0x01938e, 1); goto l938E; }
    R[0] = 0x000F;
    gsu_code(0x01938c, 3);
l938E:
    R[1] = gsu_sub(R[0], 6, 0);
    R[0] = 0x0003;
    gsu_code(0x01938f, 7);
    gsu_romb(R[0]);
    gsu_code(0x019396, 3);
    gsu_r14(gsu_add(R[6], R[1], 0));
    t = 1;
    gsu_code(0x019399, 2); gsu_code(0x01939e, 1);
    gsu_color(gsu_getb());   /* GETC */
    if (t) goto l939E;
l939B:
    R[0] = gsu_lob(R[3]);
    gsu_color((uint8_t)R[0]);
    gsu_code(0x01939c, 3);
l939E:
    gsu_code(0x01939f, 3);
    R[0] = gsu_ldw(FX_ROM_BANK);
    gsu_code(0x0193a2, 2);
    gsu_romb(R[0]);
    gsu_code(0x0193a4, 2);
    gsu_r14(R[2]);
    nx = (uint16_t)(R[11]);
    at = nx;
    gsu_code(0x0193a6, 1);
    goto dispatch;
    /* FxModelFacing */
l93A7:
    gsu_code(0x0193a8, 3);
    R[0] = gsu_ldw(FX_MODEL_FLAGS);
    R[1] = 0x4000;
    R[0] = gsu_and(R[0], R[1]);
    t = gsu.z;
    gsu_code(0x0193ab, 6);
    if (t) { gsu_code(0x0193b5, 1); goto l93B5; }
    gsu_code(0x0193b1, 4); gsu_code(0x01941b, 1);
    goto l941B;
l93B5:
    gsu_code(0x0193b6, 2);
    R[12] = gsu_getb();
    gsu_code(0x0193b8, 1);
    gsu_r14(gsu_inc(R[14]));
    R[9] = 0x04E4;
    R[11] = 0x07C6;
    /* CACHE */
    R[13] = 0x93C1;
    gsu_code(0x0193b9, 6); gsu_cache(0x93bf); gsu_code(0x0193bf, 3);
l93C1:
    gsu_code(0x0193c2, 1);
    R[0] = gsu_getb();
    gsu_code(0x0193c3, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x0193c4, 2);
    R[0] = gsu_mult(R[0], 6);
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_code(0x0193c6, 3);
    R[1] = gsu_ldw(R[0]);
    R[0] = gsu_inc(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_code(0x0193c9, 4);
    R[2] = gsu_ldw(R[0]);
    R[0] = gsu_inc(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_code(0x0193cd, 5);
    R[5] = gsu_ldb(R[0]);
    gsu_code(0x0193d2, 1);
    R[0] = gsu_getb();
    gsu_code(0x0193d3, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x0193d4, 2);
    R[0] = gsu_mult(R[0], 6);
    R[8] = gsu_add(R[0], R[9], 0);
    gsu_code(0x0193d6, 3);
    R[0] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    R[3] = gsu_sub(R[0], R[1], 0);
    gsu_code(0x0193d9, 5);
    R[0] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    R[7] = gsu_sub(R[0], R[2], 0);
    gsu_code(0x0193de, 6);
    R[0] = gsu_ldb(R[8]);
    R[5] = gsu_xor(R[0], R[5]);
    gsu_code(0x0193e4, 4);
    R[0] = gsu_getb();
    gsu_code(0x0193e8, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x0193e9, 2);
    R[0] = gsu_mult(R[0], 6);
    R[8] = gsu_add(R[0], R[9], 0);
    gsu_code(0x0193eb, 3);
    R[0] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    R[1] = gsu_sub(R[0], R[1], 0);
    gsu_code(0x0193ee, 5);
    R[0] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    R[2] = gsu_sub(R[0], R[2], 0);
    gsu_code(0x0193f3, 6);
    R[0] = gsu_ldb(R[8]);
    R[5] = gsu_xor(R[0], R[5]);
    R[6] = R[1];
    gsu_code(0x0193f9, 8);
    { uint32_t p = gsu_lmult(R[7], R[6]); R[4] = (uint16_t)p; R[7] = (uint16_t)(p >> 16); }
    R[1] = R[4];
    R[6] = R[3];
    gsu_code(0x019401, 7);
    { uint32_t p = gsu_lmult(R[2], R[6]); R[4] = (uint16_t)p; R[2] = (uint16_t)(p >> 16); }
    R[4] = gsu_sub(R[4], R[1], 0);
    R[2] = gsu_sub(R[2], R[7], !gsu.cy);
    R[0] = gsu_add(R[5], R[5], 0);
    R[0] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_swap(R[0]);
    R[0] = gsu_xor(R[0], R[2]);
    R[0] = gsu_hib(R[0]);
    gsu_code(0x019408, 15);
    gsu_stb(R[11], (uint8_t)R[0]);
    t = gsu_count_down(&R[12]);
    R[11] = gsu_inc(R[11]);
    gsu_code(0x019417, 1);
    if (t) { gsu_code(0x0193c1, 1); goto l93C1; }
    gsu_code(0x019418, 3);
    nx = (uint16_t)(gsu_getb());
    at = model_command(nx);
    gsu_code(0x010000 | at, 1);   /* where it jumps, fetched before the delay slot runs */
    gsu_r14(gsu_inc(R[14]));
    goto fetched;
l941B:
    gsu_code(0x01941c, 2);
    R[12] = gsu_getb();
    gsu_code(0x01941e, 1);
    gsu_r14(gsu_inc(R[14]));
    R[9] = 0x04E4;
    R[11] = 0x07C6;
    /* CACHE */
    R[13] = 0x9427;
    gsu_code(0x01941f, 6); gsu_cache(0x9425); gsu_code(0x019425, 3);
l9427:
    gsu_code(0x019428, 1);
    R[0] = gsu_getb();
    gsu_code(0x019429, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x01942a, 2);
    R[0] = gsu_umult(R[0], 6);
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_code(0x01942c, 4);
    R[1] = gsu_ldb(R[0]);
    R[0] = gsu_inc(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_code(0x019430, 5);
    R[2] = gsu_ldb(R[0]);
    gsu_code(0x019435, 1);
    R[0] = gsu_getb();
    gsu_code(0x019436, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x019437, 2);
    R[0] = gsu_umult(R[0], 6);
    R[8] = gsu_add(R[0], R[9], 0);
    gsu_code(0x019439, 4);
    R[0] = gsu_ldb(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    R[3] = gsu_sub(R[0], R[1], 0);
    gsu_code(0x01943d, 6);
    R[0] = gsu_ldb(R[8]);
    R[7] = gsu_sub(R[0], R[2], 0);
    gsu_code(0x019443, 3);
    R[0] = gsu_getb();
    gsu_code(0x019446, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x019447, 2);
    R[0] = gsu_umult(R[0], 6);
    R[8] = gsu_add(R[0], R[9], 0);
    gsu_code(0x019449, 4);
    R[0] = gsu_ldb(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    R[1] = gsu_sub(R[0], R[1], 0);
    gsu_code(0x01944d, 6);
    R[0] = gsu_ldb(R[8]);
    R[2] = gsu_sub(R[0], R[2], 0);
    R[6] = R[1];
    gsu_code(0x019453, 7);
    { uint32_t p = gsu_lmult(R[7], R[6]); R[4] = (uint16_t)p; R[7] = (uint16_t)(p >> 16); }
    R[1] = R[4];
    R[6] = R[3];
    gsu_code(0x01945a, 7);
    { uint32_t p = gsu_lmult(R[2], R[6]); R[4] = (uint16_t)p; R[2] = (uint16_t)(p >> 16); }
    R[4] = gsu_sub(R[4], R[1], 0);
    R[0] = gsu_sub(R[2], R[7], !gsu.cy);
    R[0] = gsu_hib(R[0]);
    gsu_code(0x019461, 8);
    gsu_stb(R[11], (uint8_t)R[0]);
    t = gsu_count_down(&R[12]);
    R[11] = gsu_inc(R[11]);
    gsu_code(0x019469, 1);
    if (t) { gsu_code(0x019427, 1); goto l9427; }
    gsu_code(0x01946a, 3);
    nx = (uint16_t)(gsu_getb());
    at = model_command(nx);
    gsu_code(0x010000 | at, 1);   /* where it jumps, fetched before the delay slot runs */
    gsu_r14(gsu_inc(R[14]));
    goto fetched;
    /* FxPolygonClipScreenUV */
l9A52:
    gsu_code(0x019a53, 2);
    gsu_stw(R[10], R[11]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[1] = 0x0003;
    gsu_sub(R[0], R[1], 0);   /* CMP */
    t = !gsu.s;
    gsu_code(0x019a55, 8);
    if (t) { gsu_code(0x019a61, 1); goto l9A61; }
    gsu_code(0x019a5d, 4); gsu_code(0x019efb, 1);
    goto l9EFB;
l9A61:
    gsu_code(0x019a62, 3);
    gsu_stw(FX_VERTEX_COUNT_SAVE, R[0]);
    gsu_code(0x019a65, 3);
    gsu_stw(FX_VERTEX_COUNT, R[0]);
    gsu_code(0x019a68, 3);
    R[8] = gsu_ldw(FX_POLY_OUT);
    R[9] = 0x0688;
    gsu_code(0x019a6b, 5);
    R[1] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    gsu_code(0x019a70, 4);
    R[2] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    gsu_code(0x019a74, 4);
    R[3] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    gsu_code(0x019a78, 4);
    R[6] = gsu_ldw(R[8]);
    R[8] = gsu_sub(R[8], 6, 0);
    gsu_code(0x019a7c, 6);
    R[0] = gsu_ldw(FX_VERTEX_COUNT);
    gsu_code(0x019a82, 2);
    R[0] = gsu_mult(R[0], 8);
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x019a84, 3);
    gsu_stw(R[0], R[1]);
    R[0] = gsu_inc(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_code(0x019a87, 4);
    gsu_stw(R[0], R[2]);
    R[0] = gsu_inc(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_code(0x019a8b, 4);
    gsu_stw(R[0], R[3]);
    R[0] = gsu_inc(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_code(0x019a8f, 4);
    gsu_stw(R[0], R[6]);
l9A92:
    gsu_code(0x019a93, 2);
    R[1] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    gsu_code(0x019a95, 4);
    R[2] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_add(R[8], 4, 0);
    gsu_code(0x019a99, 7);
    R[3] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    gsu_code(0x019aa0, 4);
    R[6] = gsu_ldw(R[8]);
    R[8] = gsu_sub(R[8], 2, 0);
    gsu_code(0x019aa4, 6);
    R[0] = gsu_ldw(FX_SCREEN_LEFT);
    R[0] = gsu_sub(R[1], R[0], 0);
    t = gsu.s != gsu.ov;
    gsu_code(0x019aaa, 4);
    if (t) { gsu_code(0x019ab2, 1); goto l9AB2; }
    gsu_code(0x019aae, 4); gsu_code(0x019ac7, 1);
    goto l9AC7;
l9AB2:
    gsu_code(0x019ab3, 3);
    R[0] = gsu_ldw(FX_SCREEN_LEFT);
    R[0] = gsu_sub(R[3], R[0], 0);
    t = gsu.s != gsu.ov;
    gsu_code(0x019ab6, 4);
    if (t) { gsu_code(0x019abe, 1); goto l9ABE; }
    gsu_code(0x019aba, 4); gsu_code(0x019aef, 1);
    goto l9AEF;
l9ABE:
    gsu_code(0x019abf, 3);
    R[0] = gsu_ldw(FX_VERTEX_COUNT_SAVE);
    R[0] = gsu_dec(R[0]);
    gsu_code(0x019ac2, 2);
    gsu_sbk(R[0]);
    gsu_code(0x019ac4, 3); gsu_code(0x019b68, 1);
    goto l9B68;
l9AC7:
    gsu_code(0x019ac8, 3);
    R[0] = gsu_ldw(FX_SCREEN_LEFT);
    R[0] = gsu_sub(R[3], R[0], 0);
    t = gsu.s != gsu.ov;
    gsu_code(0x019acb, 4);
    if (t) { gsu_code(0x019ad3, 1); goto l9AD3; }
    gsu_code(0x019acf, 4); gsu_code(0x019b51, 1);
    goto l9B51;
l9AD3:
    gsu_code(0x019ad4, 3);
    R[0] = gsu_ldw(FX_VERTEX_COUNT_SAVE);
    R[0] = gsu_inc(R[0]);
    gsu_code(0x019ad7, 2);
    gsu_sbk(R[0]);
    gsu_code(0x019ad9, 2);
    gsu_stw(R[9], R[1]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    gsu_code(0x019adb, 4);
    gsu_stw(R[9], R[2]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    R[8] = gsu_sub(R[8], 4, 0);
    gsu_code(0x019adf, 6);
    R[0] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    gsu_code(0x019ae5, 3);
    gsu_stw(R[9], R[0]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    gsu_code(0x019ae8, 3);
    R[0] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    gsu_code(0x019aeb, 3);
    gsu_stw(R[9], R[0]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    gsu_code(0x019aee, 2);
l9AEF:
    R[3] = gsu_sub(R[3], R[1], 0);
    R[6] = gsu_sub(R[6], R[2], 0);
    gsu_code(0x019af0, 7);
    R[0] = gsu_ldw(FX_SCREEN_LEFT);
    R[0] = gsu_sub(R[0], R[1], 0);
    gsu_code(0x019af7, 4);
    { uint32_t p = gsu_lmult(R[0], R[6]); R[4] = (uint16_t)p; R[5] = (uint16_t)(p >> 16); }
    R[6] = R[3];
    R[11] = 0x9B01;
    gsu_code(0x019afb, 6); gsu_code(0x018186, 1); gsu_cache(0x8187);
    abi_FxDivideSigned();
    gsu_code(0x019b01, 1);
l9B01:
    R[0] = gsu_add(R[4], R[2], 0);
    gsu_code(0x019b02, 5);
    R[6] = gsu_ldw(FX_SCREEN_LEFT);
    gsu_code(0x019b07, 2);
    gsu_stw(R[9], R[6]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    gsu_code(0x019b09, 3);
    gsu_stw(R[9], R[0]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    R[8] = gsu_add(R[8], 4, 0);
    gsu_code(0x019b0c, 7);
    R[6] = gsu_ldw(R[8]);
    R[8] = gsu_sub(R[8], 8, 0);
    gsu_code(0x019b13, 5);
    R[2] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    R[6] = gsu_sub(R[6], R[2], 0);
    gsu_code(0x019b18, 7);
    R[0] = gsu_ldw(FX_SCREEN_LEFT);
    R[0] = gsu_sub(R[0], R[1], 0);
    gsu_code(0x019b1f, 4);
    { uint32_t p = gsu_lmult(R[0], R[6]); R[4] = (uint16_t)p; R[5] = (uint16_t)(p >> 16); }
    R[6] = R[3];
    R[11] = 0x9B29;
    gsu_code(0x019b23, 6); gsu_code(0x018186, 1); gsu_cache(0x8187);
    abi_FxDivideSigned();
    gsu_code(0x019b29, 1);
l9B29:
    R[0] = gsu_add(R[4], R[2], 0);
    gsu_code(0x019b2a, 3);
    gsu_stw(R[9], R[0]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    gsu_code(0x019b2d, 4);
    R[2] = gsu_ldw(R[8]);
    R[8] = gsu_add(R[8], 8, 0);
    gsu_code(0x019b31, 5);
    R[6] = gsu_ldw(R[8]);
    R[8] = gsu_sub(R[8], 6, 0);
    R[6] = gsu_sub(R[6], R[2], 0);
    gsu_code(0x019b36, 8);
    R[0] = gsu_ldw(FX_SCREEN_LEFT);
    R[0] = gsu_sub(R[0], R[1], 0);
    gsu_code(0x019b3e, 4);
    { uint32_t p = gsu_lmult(R[0], R[6]); R[4] = (uint16_t)p; R[5] = (uint16_t)(p >> 16); }
    R[6] = R[3];
    R[11] = 0x9B48;
    gsu_code(0x019b42, 6); gsu_code(0x018186, 1); gsu_cache(0x8187);
    abi_FxDivideSigned();
    gsu_code(0x019b48, 1);
l9B48:
    R[0] = gsu_add(R[4], R[2], 0);
    gsu_code(0x019b49, 3);
    gsu_stw(R[9], R[0]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    gsu_code(0x019b4c, 5); gsu_code(0x019b68, 1);
    goto l9B68;
l9B51:
    gsu_code(0x019b52, 2);
    gsu_stw(R[9], R[1]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    gsu_code(0x019b54, 4);
    gsu_stw(R[9], R[2]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    R[8] = gsu_sub(R[8], 4, 0);
    gsu_code(0x019b58, 6);
    R[0] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    gsu_code(0x019b5e, 3);
    gsu_stw(R[9], R[0]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    gsu_code(0x019b61, 3);
    R[0] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    gsu_code(0x019b64, 3);
    gsu_stw(R[9], R[0]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    gsu_code(0x019b67, 2);
l9B68:
    gsu_code(0x019b69, 3);
    R[0] = gsu_ldw(FX_VERTEX_COUNT);
    R[0] = gsu_dec(R[0]);
    gsu_code(0x019b6c, 2);
    gsu_sbk(R[0]);
    t = gsu.z;
    gsu_code(0x019b6e, 2);
    if (t) { gsu_code(0x019b74, 1); goto l9B74; }
    gsu_code(0x019b70, 4); gsu_code(0x019a92, 1);
    goto l9A92;
l9B74:
    gsu_code(0x019b75, 3);
    R[0] = gsu_ldw(FX_VERTEX_COUNT_SAVE);
    R[0] = gsu_sub(R[0], 3, 0);
    t = !gsu.s;
    gsu_code(0x019b78, 4);
    if (t) { gsu_code(0x019b80, 1); goto l9B80; }
    gsu_code(0x019b7c, 4); gsu_code(0x019efb, 1);
    goto l9EFB;
l9B80:
    R[0] = gsu_add(R[0], 3, 0);
    gsu_code(0x019b81, 5);
    gsu_stw(FX_VERTEX_COUNT, R[0]);
    R[8] = 0x0688;
    gsu_code(0x019b86, 6);
    R[9] = gsu_ldw(FX_POLY_OUT);
    gsu_code(0x019b8c, 2);
    R[1] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    gsu_code(0x019b8e, 4);
    R[2] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    gsu_code(0x019b92, 4);
    R[3] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    gsu_code(0x019b96, 4);
    R[6] = gsu_ldw(R[8]);
    R[8] = gsu_sub(R[8], 6, 0);
    gsu_code(0x019b9a, 6);
    R[0] = gsu_ldw(FX_VERTEX_COUNT);
    gsu_code(0x019ba0, 2);
    R[0] = gsu_mult(R[0], 8);
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x019ba2, 3);
    gsu_stw(R[0], R[1]);
    R[0] = gsu_inc(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_code(0x019ba5, 4);
    gsu_stw(R[0], R[2]);
    R[0] = gsu_inc(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_code(0x019ba9, 4);
    gsu_stw(R[0], R[3]);
    R[0] = gsu_inc(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_code(0x019bad, 4);
    gsu_stw(R[0], R[6]);
l9BB0:
    gsu_code(0x019bb1, 2);
    R[1] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    gsu_code(0x019bb3, 4);
    R[2] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_add(R[8], 4, 0);
    gsu_code(0x019bb7, 7);
    R[3] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    gsu_code(0x019bbe, 4);
    R[6] = gsu_ldw(R[8]);
    R[8] = gsu_sub(R[8], 2, 0);
    gsu_code(0x019bc2, 6);
    R[0] = gsu_ldw(FX_SCREEN_RIGHT);
    R[0] = gsu_sub(R[1], R[0], 0);
    R[0] = gsu_sub(R[0], 1, 0);
    t = gsu.s == gsu.ov;
    gsu_code(0x019bc8, 6);
    if (t) { gsu_code(0x019bd2, 1); goto l9BD2; }
    gsu_code(0x019bce, 4); gsu_code(0x019be9, 1);
    goto l9BE9;
l9BD2:
    gsu_code(0x019bd3, 3);
    R[0] = gsu_ldw(FX_SCREEN_RIGHT);
    R[0] = gsu_sub(R[3], R[0], 0);
    R[0] = gsu_sub(R[0], 1, 0);
    t = gsu.s == gsu.ov;
    gsu_code(0x019bd6, 6);
    if (t) { gsu_code(0x019be0, 1); goto l9BE0; }
    gsu_code(0x019bdc, 4); gsu_code(0x019c13, 1);
    goto l9C13;
l9BE0:
    gsu_code(0x019be1, 3);
    R[0] = gsu_ldw(FX_VERTEX_COUNT_SAVE);
    R[0] = gsu_dec(R[0]);
    gsu_code(0x019be4, 2);
    gsu_sbk(R[0]);
    gsu_code(0x019be6, 3); gsu_code(0x019c8c, 1);
    goto l9C8C;
l9BE9:
    gsu_code(0x019bea, 3);
    R[0] = gsu_ldw(FX_SCREEN_RIGHT);
    R[0] = gsu_sub(R[3], R[0], 0);
    R[0] = gsu_sub(R[0], 1, 0);
    t = gsu.s == gsu.ov;
    gsu_code(0x019bed, 6);
    if (t) { gsu_code(0x019bf7, 1); goto l9BF7; }
    gsu_code(0x019bf3, 4); gsu_code(0x019c75, 1);
    goto l9C75;
l9BF7:
    gsu_code(0x019bf8, 3);
    R[0] = gsu_ldw(FX_VERTEX_COUNT_SAVE);
    R[0] = gsu_inc(R[0]);
    gsu_code(0x019bfb, 2);
    gsu_sbk(R[0]);
    gsu_code(0x019bfd, 2);
    gsu_stw(R[9], R[1]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    gsu_code(0x019bff, 4);
    gsu_stw(R[9], R[2]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    R[8] = gsu_sub(R[8], 4, 0);
    gsu_code(0x019c03, 6);
    R[0] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    gsu_code(0x019c09, 3);
    gsu_stw(R[9], R[0]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    gsu_code(0x019c0c, 3);
    R[0] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    gsu_code(0x019c0f, 3);
    gsu_stw(R[9], R[0]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    gsu_code(0x019c12, 2);
l9C13:
    R[3] = gsu_sub(R[3], R[1], 0);
    R[6] = gsu_sub(R[6], R[2], 0);
    gsu_code(0x019c14, 7);
    R[0] = gsu_ldw(FX_SCREEN_RIGHT);
    R[0] = gsu_sub(R[0], R[1], 0);
    gsu_code(0x019c1b, 4);
    { uint32_t p = gsu_lmult(R[0], R[6]); R[4] = (uint16_t)p; R[5] = (uint16_t)(p >> 16); }
    R[6] = R[3];
    R[11] = 0x9C25;
    gsu_code(0x019c1f, 6); gsu_code(0x018186, 1); gsu_cache(0x8187);
    abi_FxDivideSigned();
    gsu_code(0x019c25, 1);
l9C25:
    R[0] = gsu_add(R[4], R[2], 0);
    gsu_code(0x019c26, 5);
    R[6] = gsu_ldw(FX_SCREEN_RIGHT);
    gsu_code(0x019c2b, 2);
    gsu_stw(R[9], R[6]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    gsu_code(0x019c2d, 3);
    gsu_stw(R[9], R[0]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    R[8] = gsu_add(R[8], 4, 0);
    gsu_code(0x019c30, 7);
    R[6] = gsu_ldw(R[8]);
    R[8] = gsu_sub(R[8], 8, 0);
    gsu_code(0x019c37, 5);
    R[2] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    R[6] = gsu_sub(R[6], R[2], 0);
    gsu_code(0x019c3c, 7);
    R[0] = gsu_ldw(FX_SCREEN_RIGHT);
    R[0] = gsu_sub(R[0], R[1], 0);
    gsu_code(0x019c43, 4);
    { uint32_t p = gsu_lmult(R[0], R[6]); R[4] = (uint16_t)p; R[5] = (uint16_t)(p >> 16); }
    R[6] = R[3];
    R[11] = 0x9C4D;
    gsu_code(0x019c47, 6); gsu_code(0x018186, 1); gsu_cache(0x8187);
    abi_FxDivideSigned();
    gsu_code(0x019c4d, 1);
l9C4D:
    R[0] = gsu_add(R[4], R[2], 0);
    gsu_code(0x019c4e, 3);
    gsu_stw(R[9], R[0]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    gsu_code(0x019c51, 4);
    R[2] = gsu_ldw(R[8]);
    R[8] = gsu_add(R[8], 8, 0);
    gsu_code(0x019c55, 5);
    R[6] = gsu_ldw(R[8]);
    R[8] = gsu_sub(R[8], 6, 0);
    R[6] = gsu_sub(R[6], R[2], 0);
    gsu_code(0x019c5a, 8);
    R[0] = gsu_ldw(FX_SCREEN_RIGHT);
    R[0] = gsu_sub(R[0], R[1], 0);
    gsu_code(0x019c62, 4);
    { uint32_t p = gsu_lmult(R[0], R[6]); R[4] = (uint16_t)p; R[5] = (uint16_t)(p >> 16); }
    R[6] = R[3];
    R[11] = 0x9C6C;
    gsu_code(0x019c66, 6); gsu_code(0x018186, 1); gsu_cache(0x8187);
    abi_FxDivideSigned();
    gsu_code(0x019c6c, 1);
l9C6C:
    R[0] = gsu_add(R[4], R[2], 0);
    gsu_code(0x019c6d, 3);
    gsu_stw(R[9], R[0]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    gsu_code(0x019c70, 5); gsu_code(0x019c8c, 1);
    goto l9C8C;
l9C75:
    gsu_code(0x019c76, 2);
    gsu_stw(R[9], R[1]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    gsu_code(0x019c78, 4);
    gsu_stw(R[9], R[2]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    R[8] = gsu_sub(R[8], 4, 0);
    gsu_code(0x019c7c, 6);
    R[0] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    gsu_code(0x019c82, 3);
    gsu_stw(R[9], R[0]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    gsu_code(0x019c85, 3);
    R[0] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    gsu_code(0x019c88, 3);
    gsu_stw(R[9], R[0]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    gsu_code(0x019c8b, 2);
l9C8C:
    gsu_code(0x019c8d, 3);
    R[0] = gsu_ldw(FX_VERTEX_COUNT);
    R[0] = gsu_dec(R[0]);
    gsu_code(0x019c90, 2);
    gsu_sbk(R[0]);
    t = gsu.z;
    gsu_code(0x019c92, 2);
    if (t) { gsu_code(0x019c98, 1); goto l9C98; }
    gsu_code(0x019c94, 4); gsu_code(0x019bb0, 1);
    goto l9BB0;
l9C98:
    gsu_code(0x019c99, 3);
    R[0] = gsu_ldw(FX_VERTEX_COUNT_SAVE);
    R[0] = gsu_sub(R[0], 3, 0);
    t = !gsu.s;
    gsu_code(0x019c9c, 4);
    if (t) { gsu_code(0x019ca4, 1); goto l9CA4; }
    gsu_code(0x019ca0, 4); gsu_code(0x019efb, 1);
    goto l9EFB;
l9CA4:
    R[0] = gsu_add(R[0], 3, 0);
    gsu_code(0x019ca5, 5);
    gsu_stw(FX_VERTEX_COUNT, R[0]);
    gsu_code(0x019caa, 3);
    R[8] = gsu_ldw(FX_POLY_OUT);
    R[9] = 0x0688;
    gsu_code(0x019cad, 5);
    R[1] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    gsu_code(0x019cb2, 4);
    R[2] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    gsu_code(0x019cb6, 4);
    R[3] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    gsu_code(0x019cba, 4);
    R[6] = gsu_ldw(R[8]);
    R[8] = gsu_sub(R[8], 6, 0);
    gsu_code(0x019cbe, 6);
    R[0] = gsu_ldw(FX_VERTEX_COUNT);
    gsu_code(0x019cc4, 2);
    R[0] = gsu_mult(R[0], 8);
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x019cc6, 3);
    gsu_stw(R[0], R[1]);
    R[0] = gsu_inc(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_code(0x019cc9, 4);
    gsu_stw(R[0], R[2]);
    R[0] = gsu_inc(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_code(0x019ccd, 4);
    gsu_stw(R[0], R[3]);
    R[0] = gsu_inc(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_code(0x019cd1, 4);
    gsu_stw(R[0], R[6]);
l9CD4:
    gsu_code(0x019cd5, 2);
    R[1] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    gsu_code(0x019cd7, 4);
    R[2] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_add(R[8], 4, 0);
    gsu_code(0x019cdb, 7);
    R[3] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    gsu_code(0x019ce2, 4);
    R[6] = gsu_ldw(R[8]);
    R[8] = gsu_sub(R[8], 2, 0);
    gsu_code(0x019ce6, 6);
    R[0] = gsu_ldw(FX_SCREEN_TOP);
    R[0] = gsu_sub(R[2], R[0], 0);
    t = gsu.s != gsu.ov;
    gsu_code(0x019cec, 4);
    if (t) { gsu_code(0x019cf4, 1); goto l9CF4; }
    gsu_code(0x019cf0, 4); gsu_code(0x019d09, 1);
    goto l9D09;
l9CF4:
    gsu_code(0x019cf5, 3);
    R[0] = gsu_ldw(FX_SCREEN_TOP);
    R[0] = gsu_sub(R[6], R[0], 0);
    t = gsu.s != gsu.ov;
    gsu_code(0x019cf8, 4);
    if (t) { gsu_code(0x019d00, 1); goto l9D00; }
    gsu_code(0x019cfc, 4); gsu_code(0x019d31, 1);
    goto l9D31;
l9D00:
    gsu_code(0x019d01, 3);
    R[0] = gsu_ldw(FX_VERTEX_COUNT_SAVE);
    R[0] = gsu_dec(R[0]);
    gsu_code(0x019d04, 2);
    gsu_sbk(R[0]);
    gsu_code(0x019d06, 3); gsu_code(0x019db1, 1);
    goto l9DB1;
l9D09:
    gsu_code(0x019d0a, 3);
    R[0] = gsu_ldw(FX_SCREEN_TOP);
    R[0] = gsu_sub(R[6], R[0], 0);
    t = gsu.s != gsu.ov;
    gsu_code(0x019d0d, 4);
    if (t) { gsu_code(0x019d15, 1); goto l9D15; }
    gsu_code(0x019d11, 4); gsu_code(0x019d9a, 1);
    goto l9D9A;
l9D15:
    gsu_code(0x019d16, 3);
    R[0] = gsu_ldw(FX_VERTEX_COUNT_SAVE);
    R[0] = gsu_inc(R[0]);
    gsu_code(0x019d19, 2);
    gsu_sbk(R[0]);
    gsu_code(0x019d1b, 2);
    gsu_stw(R[9], R[1]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    gsu_code(0x019d1d, 4);
    gsu_stw(R[9], R[2]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    R[8] = gsu_sub(R[8], 4, 0);
    gsu_code(0x019d21, 6);
    R[0] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    gsu_code(0x019d27, 3);
    gsu_stw(R[9], R[0]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    gsu_code(0x019d2a, 3);
    R[0] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    gsu_code(0x019d2d, 3);
    gsu_stw(R[9], R[0]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    gsu_code(0x019d30, 2);
l9D31:
    R[3] = gsu_sub(R[3], R[1], 0);
    R[6] = gsu_sub(R[6], R[2], 0);
    R[0] = R[3];
    R[3] = R[6];
    R[6] = R[0];
    gsu_code(0x019d32, 13);
    R[0] = gsu_ldw(FX_SCREEN_TOP);
    R[0] = gsu_sub(R[0], R[2], 0);
    gsu_code(0x019d3f, 4);
    gsu_stw(FX_MODEL_96, R[0]);
    gsu_code(0x019d43, 3);
    { uint32_t p = gsu_lmult(R[0], R[6]); R[4] = (uint16_t)p; R[5] = (uint16_t)(p >> 16); }
    R[6] = R[3];
    R[11] = 0x9D4C;
    gsu_code(0x019d46, 6); gsu_code(0x018186, 1); gsu_cache(0x8187);
    abi_FxDivideSigned();
    gsu_code(0x019d4c, 1);
l9D4C:
    R[0] = gsu_add(R[4], R[1], 0);
    gsu_code(0x019d4d, 5);
    R[6] = gsu_ldw(FX_SCREEN_TOP);
    gsu_code(0x019d52, 1);
    gsu_stw(R[9], R[0]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    gsu_code(0x019d53, 4);
    gsu_stw(R[9], R[6]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    R[8] = gsu_add(R[8], 4, 0);
    gsu_code(0x019d57, 7);
    R[6] = gsu_ldw(R[8]);
    R[8] = gsu_sub(R[8], 8, 0);
    gsu_code(0x019d5e, 5);
    R[2] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    R[6] = gsu_sub(R[6], R[2], 0);
    gsu_code(0x019d63, 7);
    R[0] = gsu_ldw(FX_MODEL_96);
    gsu_code(0x019d6a, 3);
    { uint32_t p = gsu_lmult(R[0], R[6]); R[4] = (uint16_t)p; R[5] = (uint16_t)(p >> 16); }
    R[6] = R[3];
    R[11] = 0x9D73;
    gsu_code(0x019d6d, 6); gsu_code(0x018186, 1); gsu_cache(0x8187);
    abi_FxDivideSigned();
    gsu_code(0x019d73, 1);
l9D73:
    R[0] = gsu_add(R[4], R[2], 0);
    gsu_code(0x019d74, 3);
    gsu_stw(R[9], R[0]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    gsu_code(0x019d77, 4);
    R[2] = gsu_ldw(R[8]);
    R[8] = gsu_add(R[8], 8, 0);
    gsu_code(0x019d7b, 5);
    R[6] = gsu_ldw(R[8]);
    R[8] = gsu_sub(R[8], 6, 0);
    R[6] = gsu_sub(R[6], R[2], 0);
    gsu_code(0x019d80, 8);
    R[0] = gsu_ldw(FX_MODEL_96);
    gsu_code(0x019d88, 3);
    { uint32_t p = gsu_lmult(R[0], R[6]); R[4] = (uint16_t)p; R[5] = (uint16_t)(p >> 16); }
    R[6] = R[3];
    R[11] = 0x9D91;
    gsu_code(0x019d8b, 6); gsu_code(0x018186, 1); gsu_cache(0x8187);
    abi_FxDivideSigned();
    gsu_code(0x019d91, 1);
l9D91:
    R[0] = gsu_add(R[4], R[2], 0);
    gsu_code(0x019d92, 3);
    gsu_stw(R[9], R[0]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    gsu_code(0x019d95, 5); gsu_code(0x019db1, 1);
    goto l9DB1;
l9D9A:
    gsu_code(0x019d9b, 2);
    gsu_stw(R[9], R[1]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    gsu_code(0x019d9d, 4);
    gsu_stw(R[9], R[2]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    R[8] = gsu_sub(R[8], 4, 0);
    gsu_code(0x019da1, 6);
    R[0] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    gsu_code(0x019da7, 3);
    gsu_stw(R[9], R[0]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    gsu_code(0x019daa, 3);
    R[0] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    gsu_code(0x019dad, 3);
    gsu_stw(R[9], R[0]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    gsu_code(0x019db0, 2);
l9DB1:
    gsu_code(0x019db2, 3);
    R[0] = gsu_ldw(FX_VERTEX_COUNT);
    R[0] = gsu_dec(R[0]);
    gsu_code(0x019db5, 2);
    gsu_sbk(R[0]);
    t = gsu.z;
    gsu_code(0x019db7, 2);
    if (t) { gsu_code(0x019dbd, 1); goto l9DBD; }
    gsu_code(0x019db9, 4); gsu_code(0x019cd4, 1);
    goto l9CD4;
l9DBD:
    gsu_code(0x019dbe, 3);
    R[0] = gsu_ldw(FX_VERTEX_COUNT_SAVE);
    R[0] = gsu_sub(R[0], 3, 0);
    t = !gsu.s;
    gsu_code(0x019dc1, 4);
    if (t) { gsu_code(0x019dc9, 1); goto l9DC9; }
    gsu_code(0x019dc5, 4); gsu_code(0x019efb, 1);
    goto l9EFB;
l9DC9:
    R[0] = gsu_add(R[0], 3, 0);
    gsu_code(0x019dca, 5);
    gsu_stw(FX_VERTEX_COUNT, R[0]);
    R[8] = 0x0688;
    gsu_code(0x019dcf, 6);
    R[9] = gsu_ldw(FX_POLY_OUT);
    gsu_code(0x019dd5, 2);
    R[1] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    gsu_code(0x019dd7, 4);
    R[2] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    gsu_code(0x019ddb, 4);
    R[3] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    gsu_code(0x019ddf, 4);
    R[6] = gsu_ldw(R[8]);
    R[8] = gsu_sub(R[8], 6, 0);
    gsu_code(0x019de3, 6);
    R[0] = gsu_ldw(FX_VERTEX_COUNT);
    gsu_code(0x019de9, 2);
    R[0] = gsu_mult(R[0], 8);
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x019deb, 3);
    gsu_stw(R[0], R[1]);
    R[0] = gsu_inc(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_code(0x019dee, 4);
    gsu_stw(R[0], R[2]);
    R[0] = gsu_inc(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_code(0x019df2, 4);
    gsu_stw(R[0], R[3]);
    R[0] = gsu_inc(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_code(0x019df6, 4);
    gsu_stw(R[0], R[6]);
l9DF9:
    gsu_code(0x019dfa, 2);
    R[1] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    gsu_code(0x019dfc, 4);
    R[2] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_add(R[8], 4, 0);
    gsu_code(0x019e00, 7);
    R[3] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    gsu_code(0x019e07, 4);
    R[6] = gsu_ldw(R[8]);
    R[8] = gsu_sub(R[8], 2, 0);
    gsu_code(0x019e0b, 6);
    R[0] = gsu_ldw(FX_SCREEN_BOTTOM);
    R[0] = gsu_sub(R[2], R[0], 0);
    R[0] = gsu_sub(R[0], 1, 0);
    t = gsu.s == gsu.ov;
    gsu_code(0x019e11, 6);
    if (t) { gsu_code(0x019e1b, 1); goto l9E1B; }
    gsu_code(0x019e17, 4); gsu_code(0x019e32, 1);
    goto l9E32;
l9E1B:
    gsu_code(0x019e1c, 3);
    R[0] = gsu_ldw(FX_SCREEN_BOTTOM);
    R[0] = gsu_sub(R[6], R[0], 0);
    R[0] = gsu_sub(R[0], 1, 0);
    t = gsu.s == gsu.ov;
    gsu_code(0x019e1f, 6);
    if (t) { gsu_code(0x019e29, 1); goto l9E29; }
    gsu_code(0x019e25, 4); gsu_code(0x019e5c, 1);
    goto l9E5C;
l9E29:
    gsu_code(0x019e2a, 3);
    R[0] = gsu_ldw(FX_VERTEX_COUNT_SAVE);
    R[0] = gsu_dec(R[0]);
    gsu_code(0x019e2d, 2);
    gsu_sbk(R[0]);
    gsu_code(0x019e2f, 3); gsu_code(0x019edc, 1);
    goto l9EDC;
l9E32:
    gsu_code(0x019e33, 3);
    R[0] = gsu_ldw(FX_SCREEN_BOTTOM);
    R[0] = gsu_sub(R[6], R[0], 0);
    R[0] = gsu_sub(R[0], 1, 0);
    t = gsu.s == gsu.ov;
    gsu_code(0x019e36, 6);
    if (t) { gsu_code(0x019e40, 1); goto l9E40; }
    gsu_code(0x019e3c, 4); gsu_code(0x019ec5, 1);
    goto l9EC5;
l9E40:
    gsu_code(0x019e41, 3);
    R[0] = gsu_ldw(FX_VERTEX_COUNT_SAVE);
    R[0] = gsu_inc(R[0]);
    gsu_code(0x019e44, 2);
    gsu_sbk(R[0]);
    gsu_code(0x019e46, 2);
    gsu_stw(R[9], R[1]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    gsu_code(0x019e48, 4);
    gsu_stw(R[9], R[2]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    R[8] = gsu_sub(R[8], 4, 0);
    gsu_code(0x019e4c, 6);
    R[0] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    gsu_code(0x019e52, 3);
    gsu_stw(R[9], R[0]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    gsu_code(0x019e55, 3);
    R[0] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    gsu_code(0x019e58, 3);
    gsu_stw(R[9], R[0]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    gsu_code(0x019e5b, 2);
l9E5C:
    R[3] = gsu_sub(R[3], R[1], 0);
    R[6] = gsu_sub(R[6], R[2], 0);
    R[0] = R[3];
    R[3] = R[6];
    R[6] = R[0];
    gsu_code(0x019e5d, 13);
    R[0] = gsu_ldw(FX_SCREEN_BOTTOM);
    R[0] = gsu_sub(R[0], R[2], 0);
    gsu_code(0x019e6a, 4);
    gsu_stw(FX_MODEL_96, R[0]);
    gsu_code(0x019e6e, 3);
    { uint32_t p = gsu_lmult(R[0], R[6]); R[4] = (uint16_t)p; R[5] = (uint16_t)(p >> 16); }
    R[6] = R[3];
    R[11] = 0x9E77;
    gsu_code(0x019e71, 6); gsu_code(0x018186, 1); gsu_cache(0x8187);
    abi_FxDivideSigned();
    gsu_code(0x019e77, 1);
l9E77:
    R[0] = gsu_add(R[4], R[1], 0);
    gsu_code(0x019e78, 5);
    R[6] = gsu_ldw(FX_SCREEN_BOTTOM);
    gsu_code(0x019e7d, 1);
    gsu_stw(R[9], R[0]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    gsu_code(0x019e7e, 4);
    gsu_stw(R[9], R[6]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    R[8] = gsu_add(R[8], 4, 0);
    gsu_code(0x019e82, 7);
    R[6] = gsu_ldw(R[8]);
    R[8] = gsu_sub(R[8], 8, 0);
    gsu_code(0x019e89, 5);
    R[2] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    R[6] = gsu_sub(R[6], R[2], 0);
    gsu_code(0x019e8e, 7);
    R[0] = gsu_ldw(FX_MODEL_96);
    gsu_code(0x019e95, 3);
    { uint32_t p = gsu_lmult(R[0], R[6]); R[4] = (uint16_t)p; R[5] = (uint16_t)(p >> 16); }
    R[6] = R[3];
    R[11] = 0x9E9E;
    gsu_code(0x019e98, 6); gsu_code(0x018186, 1); gsu_cache(0x8187);
    abi_FxDivideSigned();
    gsu_code(0x019e9e, 1);
l9E9E:
    R[0] = gsu_add(R[4], R[2], 0);
    gsu_code(0x019e9f, 3);
    gsu_stw(R[9], R[0]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    gsu_code(0x019ea2, 4);
    R[2] = gsu_ldw(R[8]);
    R[8] = gsu_add(R[8], 8, 0);
    gsu_code(0x019ea6, 5);
    R[6] = gsu_ldw(R[8]);
    R[8] = gsu_sub(R[8], 6, 0);
    R[6] = gsu_sub(R[6], R[2], 0);
    gsu_code(0x019eab, 8);
    R[0] = gsu_ldw(FX_MODEL_96);
    gsu_code(0x019eb3, 3);
    { uint32_t p = gsu_lmult(R[0], R[6]); R[4] = (uint16_t)p; R[5] = (uint16_t)(p >> 16); }
    R[6] = R[3];
    R[11] = 0x9EBC;
    gsu_code(0x019eb6, 6); gsu_code(0x018186, 1); gsu_cache(0x8187);
    abi_FxDivideSigned();
    gsu_code(0x019ebc, 1);
l9EBC:
    R[0] = gsu_add(R[4], R[2], 0);
    gsu_code(0x019ebd, 3);
    gsu_stw(R[9], R[0]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    gsu_code(0x019ec0, 5); gsu_code(0x019edc, 1);
    goto l9EDC;
l9EC5:
    gsu_code(0x019ec6, 2);
    gsu_stw(R[9], R[1]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    gsu_code(0x019ec8, 4);
    gsu_stw(R[9], R[2]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    R[8] = gsu_sub(R[8], 4, 0);
    gsu_code(0x019ecc, 6);
    R[0] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    gsu_code(0x019ed2, 3);
    gsu_stw(R[9], R[0]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    gsu_code(0x019ed5, 3);
    R[0] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    gsu_code(0x019ed8, 3);
    gsu_stw(R[9], R[0]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    gsu_code(0x019edb, 2);
l9EDC:
    gsu_code(0x019edd, 3);
    R[0] = gsu_ldw(FX_VERTEX_COUNT);
    R[0] = gsu_dec(R[0]);
    gsu_code(0x019ee0, 2);
    gsu_sbk(R[0]);
    t = gsu.z;
    gsu_code(0x019ee2, 2);
    if (t) { gsu_code(0x019ee8, 1); goto l9EE8; }
    gsu_code(0x019ee4, 4); gsu_code(0x019df9, 1);
    goto l9DF9;
l9EE8:
    gsu_code(0x019ee9, 3);
    R[0] = gsu_ldw(FX_VERTEX_COUNT_SAVE);
    R[0] = gsu_sub(R[0], 3, 0);
    t = !gsu.s;
    gsu_code(0x019eec, 4);
    if (t) { gsu_code(0x019ef4, 1); goto l9EF4; }
    gsu_code(0x019ef0, 4); gsu_code(0x019efb, 1);
    goto l9EFB;
l9EF4:
    R[0] = gsu_add(R[0], 3, 0);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x019ef5, 6);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
l9EFB:
    R[0] = gsu_sub(R[0], R[0], 0);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x019efc, 5);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
    /* FxReturnR11 */
lAB11:
    nx = (uint16_t)(R[11]);
    at = nx;
    gsu_code(0x01ab12, 1);
    goto dispatch;
dispatch:
    if (at == ret)
        return;
    /* An entry of the command table (01:8000): its IWT R15 goes to the
       command's code. */
    if (at >= 0x8000 && at < 0x8000 + 4 * 26 && !(at & 3))
        at = commands[(at - 0x8000) >> 2];
    gsu_code(0x010000 | at, 1);   /* where it jumps: its first byte */
fetched:
    if (at == 0x8651 || at == 0x90CD)
        gsu_cache((uint16_t)(at + 1));   /* the code there starts with a CACHE */
enter:
    switch (at) {
    case 0x8561: goto l8561;
    case 0x8571: goto l8571;
    case 0x8576: goto l8576;
    case 0x8584: goto l8584;
    case 0x858F: goto l858F;
    case 0x85B6: goto l85B6;
    case 0x85D2: goto l85D2;
    case 0x85E4: goto l85E4;
    case 0x85F6: goto l85F6;
    case 0x85FC: goto l85FC;
    case 0x8609: goto l8609;
    case 0x861D: goto l861D;
    case 0x862F: goto l862F;
    case 0x8641: goto l8641;
    case 0x8647: goto l8647;
    case 0x8651: goto l8651;
    case 0x865E: goto l865E;
    case 0x8660: goto l8660;
    case 0x8687: goto l8687;
    case 0x86D4: goto l86D4;
    case 0x86D8: goto l86D8;
    case 0x86E4: goto l86E4;
    case 0x873D: goto l873D;
    case 0x8741: goto l8741;
    case 0x8749: goto l8749;
    case 0x8751: goto l8751;
    case 0x876C: goto l876C;
    case 0x8770: goto l8770;
    case 0x8778: goto l8778;
    case 0x882C: goto l882C;
    case 0x884A: goto l884A;
    case 0x884F: goto l884F;
    case 0x8858: goto l8858;
    case 0x8860: goto l8860;
    case 0x8863: goto l8863;
    case 0x886C: goto l886C;
    case 0x888D: goto l888D;
    case 0x88F3: goto l88F3;
    case 0x8902: goto l8902;
    case 0x8982: goto l8982;
    case 0x89A3: goto l89A3;
    case 0x8A03: goto l8A03;
    case 0x8A22: goto l8A22;
    case 0x8A97: goto l8A97;
    case 0x8AA3: goto l8AA3;
    case 0x8AC9: goto l8AC9;
    case 0x8B0E: goto l8B0E;
    case 0x8B1A: goto l8B1A;
    case 0x8B3F: goto l8B3F;
    case 0x8BAA: goto l8BAA;
    case 0x8BAE: goto l8BAE;
    case 0x8BBF: goto l8BBF;
    case 0x8BCC: goto l8BCC;
    case 0x8BE0: goto l8BE0;
    case 0x8BFF: goto l8BFF;
    case 0x8C0E: goto l8C0E;
    case 0x8C42: goto l8C42;
    case 0x8C46: goto l8C46;
    case 0x8C53: goto l8C53;
    case 0x8C54: goto l8C54;
    case 0x8C5C: goto l8C5C;
    case 0x8C69: goto l8C69;
    case 0x8C79: goto l8C79;
    case 0x8C7D: goto l8C7D;
    case 0x8C89: goto l8C89;
    case 0x8CB4: goto l8CB4;
    case 0x8CC1: goto l8CC1;
    case 0x8CCA: goto l8CCA;
    case 0x8CEC: goto l8CEC;
    case 0x8CF0: goto l8CF0;
    case 0x8CFD: goto l8CFD;
    case 0x8CFE: goto l8CFE;
    case 0x8D15: goto l8D15;
    case 0x8D23: goto l8D23;
    case 0x8D33: goto l8D33;
    case 0x8D44: goto l8D44;
    case 0x8D4D: goto l8D4D;
    case 0x8D5D: goto l8D5D;
    case 0x8D75: goto l8D75;
    case 0x8D7A: goto l8D7A;
    case 0x8D9A: goto l8D9A;
    case 0x8DB3: goto l8DB3;
    case 0x8DBD: goto l8DBD;
    case 0x8DD1: goto l8DD1;
    case 0x8DEA: goto l8DEA;
    case 0x8DF6: goto l8DF6;
    case 0x8E06: goto l8E06;
    case 0x8E14: goto l8E14;
    case 0x8E19: goto l8E19;
    case 0x8E3C: goto l8E3C;
    case 0x8E3D: goto l8E3D;
    case 0x8E45: goto l8E45;
    case 0x8E5D: goto l8E5D;
    case 0x8E78: goto l8E78;
    case 0x8E7D: goto l8E7D;
    case 0x8E88: goto l8E88;
    case 0x8E9F: goto l8E9F;
    case 0x8EA3: goto l8EA3;
    case 0x8EA7: goto l8EA7;
    case 0x8EC0: goto l8EC0;
    case 0x8ECA: goto l8ECA;
    case 0x8ED5: goto l8ED5;
    case 0x8EDF: goto l8EDF;
    case 0x8EF9: goto l8EF9;
    case 0x8F05: goto l8F05;
    case 0x8F18: goto l8F18;
    case 0x8F49: goto l8F49;
    case 0x8F5E: goto l8F5E;
    case 0x8F6D: goto l8F6D;
    case 0x8F78: goto l8F78;
    case 0x8F7D: goto l8F7D;
    case 0x8F98: goto l8F98;
    case 0x8FA4: goto l8FA4;
    case 0x8FCF: goto l8FCF;
    case 0x8FE1: goto l8FE1;
    case 0x8FF7: goto l8FF7;
    case 0x900B: goto l900B;
    case 0x9045: goto l9045;
    case 0x905E: goto l905E;
    case 0x909C: goto l909C;
    case 0x90A5: goto l90A5;
    case 0x90B3: goto l90B3;
    case 0x90B6: goto l90B6;
    case 0x90C0: goto l90C0;
    case 0x90C8: goto l90C8;
    case 0x90CD: goto l90CD;
    case 0x90EF: goto l90EF;
    case 0x90F0: goto l90F0;
    case 0x9106: goto l9106;
    case 0x910D: goto l910D;
    case 0x9112: goto l9112;
    case 0x9142: goto l9142;
    case 0x914E: goto l914E;
    case 0x918B: goto l918B;
    case 0x9199: goto l9199;
    case 0x91A6: goto l91A6;
    case 0x91C1: goto l91C1;
    case 0x91CA: goto l91CA;
    case 0x91DA: goto l91DA;
    case 0x91E7: goto l91E7;
    case 0x91EB: goto l91EB;
    case 0x91F5: goto l91F5;
    case 0x91FA: goto l91FA;
    case 0x9202: goto l9202;
    case 0x9205: goto l9205;
    case 0x920A: goto l920A;
    case 0x920F: goto l920F;
    case 0x922C: goto l922C;
    case 0x9237: goto l9237;
    case 0x923C: goto l923C;
    case 0x923F: goto l923F;
    case 0x9243: goto l9243;
    case 0x9248: goto l9248;
    case 0x924D: goto l924D;
    case 0x9250: goto l9250;
    case 0x9275: goto l9275;
    case 0x9276: goto l9276;
    case 0x9281: goto l9281;
    case 0x92AA: goto l92AA;
    case 0x92BC: goto l92BC;
    case 0x92D2: goto l92D2;
    case 0x92E6: goto l92E6;
    case 0x931E: goto l931E;
    case 0x932B: goto l932B;
    case 0x9334: goto l9334;
    case 0x9340: goto l9340;
    case 0x9385: goto l9385;
    case 0x938E: goto l938E;
    case 0x939B: goto l939B;
    case 0x939E: goto l939E;
    case 0x93A7: goto l93A7;
    case 0x93B5: goto l93B5;
    case 0x93C1: goto l93C1;
    case 0x941B: goto l941B;
    case 0x9427: goto l9427;
    case 0x9A52: goto l9A52;
    case 0x9A61: goto l9A61;
    case 0x9A92: goto l9A92;
    case 0x9AB2: goto l9AB2;
    case 0x9ABE: goto l9ABE;
    case 0x9AC7: goto l9AC7;
    case 0x9AD3: goto l9AD3;
    case 0x9AEF: goto l9AEF;
    case 0x9B01: goto l9B01;
    case 0x9B29: goto l9B29;
    case 0x9B48: goto l9B48;
    case 0x9B51: goto l9B51;
    case 0x9B68: goto l9B68;
    case 0x9B74: goto l9B74;
    case 0x9B80: goto l9B80;
    case 0x9BB0: goto l9BB0;
    case 0x9BD2: goto l9BD2;
    case 0x9BE0: goto l9BE0;
    case 0x9BE9: goto l9BE9;
    case 0x9BF7: goto l9BF7;
    case 0x9C13: goto l9C13;
    case 0x9C25: goto l9C25;
    case 0x9C4D: goto l9C4D;
    case 0x9C6C: goto l9C6C;
    case 0x9C75: goto l9C75;
    case 0x9C8C: goto l9C8C;
    case 0x9C98: goto l9C98;
    case 0x9CA4: goto l9CA4;
    case 0x9CD4: goto l9CD4;
    case 0x9CF4: goto l9CF4;
    case 0x9D00: goto l9D00;
    case 0x9D09: goto l9D09;
    case 0x9D15: goto l9D15;
    case 0x9D31: goto l9D31;
    case 0x9D4C: goto l9D4C;
    case 0x9D73: goto l9D73;
    case 0x9D91: goto l9D91;
    case 0x9D9A: goto l9D9A;
    case 0x9DB1: goto l9DB1;
    case 0x9DBD: goto l9DBD;
    case 0x9DC9: goto l9DC9;
    case 0x9DF9: goto l9DF9;
    case 0x9E1B: goto l9E1B;
    case 0x9E29: goto l9E29;
    case 0x9E32: goto l9E32;
    case 0x9E40: goto l9E40;
    case 0x9E5C: goto l9E5C;
    case 0x9E77: goto l9E77;
    case 0x9E9E: goto l9E9E;
    case 0x9EBC: goto l9EBC;
    case 0x9EC5: goto l9EC5;
    case 0x9EDC: goto l9EDC;
    case 0x9EE8: goto l9EE8;
    case 0x9EF4: goto l9EF4;
    case 0x9EFB: goto l9EFB;
    case 0xAB11: goto lAB11;
    }
    return;   /* no command or return address of the code */

}

/*
 * @routine 01:8561 FxModelDraw
 * WHAT IT DOES: Draws a 3D model: sets up the object's rotation, light
 * and point scale, then runs the model's command list from ROM ($0016,
 * bank $001C) until its end command, each command transforming points,
 * projecting them, or queueing and drawing faces.
 * HOW: Pushes R11 on the R10 stack; a non-zero $01B4 first (01:8568)
 * and $01B6 pick other set-ups. $005C = $08AA (the draw list empty).
 * The object's angles ($0020-$0024) become negated 8.8 words; with all
 * three zero the matrix at $00E4 is copied to $0132 (or negated in
 * parts when the y angle is $40 or $80), else FxMatrixFromAngles01,
 * FxMatrixTranspose and FxMatrixApplyVectors build it. The light $0000,
 * $393E, $727C is turned into $0106-$010A. Points start at $0340
 * ($001E); the end command (9243) is pushed on the stack, and the first
 * command byte is dispatched through the IWT R15 table at 01:8000, as
 * every command's last GETB is.
 * Entry: R10 = stack; R11 = return address; the object's set-up in RAM.
 * Exit: as the code leaves the registers, flags and RAM; R14 and the
 * ROM buffer move as on the Super FX.
 */
void abi_FxModelDraw(void)
{
    model_run(0x8561, R[11]);
}

/*
 * @routine 01:8576 FxModelDrawFlagged
 * WHAT IT DOES: FxModelDraw's set-up when $01B4 is set (01:8561 calls
 * it with R11 = $8571): the model flags $0054 become bit 12 of them
 * with bit 13 set, then the model is drawn as FxModelDraw draws it.
 * HOW: R2 = $1000, R1 = $2000: ($0054 AND R2) OR R1 into $0054
 * (01:858F), the rest as in FxModelDraw; its end command's pull returns
 * to R11.
 * Entry: R10 = stack; R11 = return address; the object's set-up in RAM.
 * Exit: as the code leaves the registers, flags and RAM; R14 and the
 * ROM buffer move as on the Super FX.
 */
void abi_FxModelDrawFlagged(void)
{
    model_run(0x8576, R[11]);
}

/*
 * @routine 01:8584 FxModelDrawFlags8000
 * WHAT IT DOES: An entry inside FxModelDrawFlagged's set-up: draws the
 * model with the model flags $0054 = $8000. Only the uncalled program
 * FxModelMirrorProgram (01:84DA) calls it.
 * HOW: Pushes R11 and pulls it straight back, R0 = $8000, then on as at
 * 01:858F.
 * Entry: as FxModelDrawFlagged. Exit: as FxModelDrawFlagged.
 */
void abi_FxModelDrawFlags8000(void)
{
    model_run(0x8584, R[11]);
}

/*
 * @routine 01:858F FxModelDrawWithFlags
 * WHAT IT DOES: The draw both set-ups lead to: the model flags $0054 = R0,
 * then the model drawn as FxModelDraw draws it. Called directly only by
 * the uncalled program FxModelRecordProgram (01:8515).
 * HOW: SMS $0054, R0, then on in the model interpreter.
 * Entry: R0 = the model flags; otherwise as FxModelDrawFlagged. Exit: as
 * FxModelDrawFlagged.
 */
void abi_FxModelDrawWithFlags(void)
{
    model_run(0x858F, R[11]);
}

/*
 * @routine 01:8000 FxModelCommands
 * WHAT IT DOES: The model interpreter's command table: command byte cc
 * (0, 4, ... $64) runs the entry at 01:80cc, which jumps to that
 * command's code (commands[] above). The code reaches an entry through
 * the bank's mirror (TO R15; GETB jumps to 01:00cc).
 * HOW: 26 entries of IWT R15,#code; NOP; model_run's dispatch takes an
 * entry's address to its command's code.
 * Entry: R15 = 01:00cc (a command's last GETB); R10 = stack; R11 =
 * return address; R14 at the command's bytes (bank $001C).
 * Exit: as the command's code leaves the registers, flags and RAM; R14
 * and the ROM buffer move as on the Super FX.
 */
void abi_FxModelCommands(void)
{
    model_run(0x8000, R[11]);
}

/*
 * @routine 01:884A FxModelReturn
 * WHAT IT DOES: Commands $18, $24, $2C and $48: return, pulling the
 * next command address from the R10 stack.
 * HOW: DEC R10 twice, R15 from (R10).
 * Entry: R10 = stack; R11 = return address; R14 at the command's bytes
 * (bank $001C).
 * Exit: as the code leaves the registers, flags and RAM; R14 and the
 * ROM buffer move as on the Super FX.
 */
void abi_FxModelReturn(void)
{
    model_run(0x884A, R[11]);
}

/*
 * @routine 01:884F FxModelFramePick
 * WHAT IT DOES: Command $1C: of the n lists that follow (n the next
 * byte), runs the one the model frame $0044 picks ($0044 AND $3F, less
 * n until below it).
 * HOW: R14 skips to the picked list's word offset and the list's first
 * command is dispatched.
 * Entry: R10 = stack; R11 = return address; R14 at the command's bytes
 * (bank $001C).
 * Exit: as the code leaves the registers, flags and RAM; R14 and the
 * ROM buffer move as on the Super FX.
 */
void abi_FxModelFramePick(void)
{
    model_run(0x884F, R[11]);
}

/*
 * @routine 01:8863 FxModelJump
 * WHAT IT DOES: Command $20: jumps within the commands by the signed
 * word that follows.
 * HOW: GETB, GETBH, R14 += it, dispatch.
 * Entry: R10 = stack; R11 = return address; R14 at the command's bytes
 * (bank $001C).
 * Exit: as the code leaves the registers, flags and RAM; R14 and the
 * ROM buffer move as on the Super FX.
 */
void abi_FxModelJump(void)
{
    model_run(0x8863, R[11]);
}

/*
 * @routine 01:886C FxModelPointsFull
 * WHAT IT DOES: Commands $08 and $4C: n points (the next byte) turned
 * by the view matrix ($0132-$0142, held in R10, R11, R13, R4, R7, R8
 * and two RAM words) into x, y, z words at $001E (not reached in the
 * traces so far).
 * HOW: R10 kept at $0012 while it holds a matrix element; a count of
 * DEC R12; $0144 counts the points.
 * Entry: R10 = stack; R11 = return address; R14 at the command's bytes
 * (bank $001C).
 * Exit: as the code leaves the registers, flags and RAM; R14 and the
 * ROM buffer move as on the Super FX.
 */
void abi_FxModelPointsFull(void)
{
    model_run(0x886C, R[11]);
}

/*
 * @routine 01:88F3 FxModelPointsScaled
 * WHAT IT DOES: Command $34: n points of three words (GETB, GETBH),
 * each turned by the view matrix (FMULT, ROL) and written twice at
 * $001E: as given and with x negated (the x term subtracted twice);
 * $0144 counts 2n (not reached in the traces so far).
 * HOW: As FxModelPointsFull, with FMULT and ROL by the matrix
 * registers.
 * Entry: R10 = stack; R11 = return address; R14 at the command's bytes
 * (bank $001C).
 * Exit: as the code leaves the registers, flags and RAM; R14 and the
 * ROM buffer move as on the Super FX.
 */
void abi_FxModelPointsScaled(void)
{
    model_run(0x88F3, R[11]);
}

/*
 * @routine 01:8A97 FxModelPoints01
 * WHAT IT DOES: Command $04: n points of three signed bytes each,
 * scaled by $0030 and turned by the packed matrix ($0128-$0130),
 * written as x, y, z words at $001E.
 * HOW: A LOOP of R12 = n; MULT of each byte by the packed matrix bytes;
 * $0144 counts the points; models of fewer than 3 points per $0032 take
 * 01:8982.
 * Entry: R10 = stack; R11 = return address; R14 at the command's bytes
 * (bank $001C).
 * Exit: as the code leaves the registers, flags and RAM; R14 and the
 * ROM buffer move as on the Super FX.
 */
void abi_FxModelPoints01(void)
{
    model_run(0x8A97, R[11]);
}

/*
 * @routine 01:8B0E FxModelPointsMirrored
 * WHAT IT DOES: Command $38: as command $04, each point also written
 * mirrored in x (two points per entry).
 * HOW: The x term added and subtracted (ADD R8, SUB R8 twice) for the
 * pair.
 * Entry: R10 = stack; R11 = return address; R14 at the command's bytes
 * (bank $001C).
 * Exit: as the code leaves the registers, flags and RAM; R14 and the
 * ROM buffer move as on the Super FX.
 */
void abi_FxModelPointsMirrored(void)
{
    model_run(0x8B0E, R[11]);
}

/*
 * @routine 01:8BAA FxModelProject
 * WHAT IT DOES: Command $0C: projects the points written so far
 * (FxProjectVertex, or FxProjectPoint for near ones) for the faces that
 * follow.
 * HOW: Its rest runs at 01:8C79 (FxModelProjectRest).
 * Entry: R10 = stack; R11 = return address; R14 at the command's bytes
 * (bank $001C).
 * Exit: as the code leaves the registers, flags and RAM; R14 and the
 * ROM buffer move as on the Super FX.
 */
void abi_FxModelProject(void)
{
    model_run(0x8BAA, R[11]);
}

/*
 * @routine 01:8BAE FxModelProjectEach
 * WHAT IT DOES: A projection of the points written so far that command
 * $0C does not use: each point ($0340 on, $0144 of them) moved by the
 * object's place and projected by FxProjectVertex into the table at
 * $04E4, the outcodes ORed into $014A; then on as FxModelProjectRest.
 * The game never runs it: FxModelProject jumps over it (to 01:8C89)
 * with its first instruction. A unit start runs it so the C is judged
 * against the original.
 * HOW: ROMB $14 for the loop, R14 kept in $001A; the ROM bank ($001C)
 * and R14 put back after it, then IWT R15 to 01:8C79.
 * Entry: R10 = stack; R11 = return address; R14 at the command's bytes;
 * the ROM bank the model's.
 * Exit: as the code leaves the registers, flags and RAM; R14 and the
 * ROM buffer move as on the Super FX.
 */
void abi_FxModelProjectEach(void)
{
    model_run(0x8BAE, R[11]);
}

/*
 * @routine 01:8C79 FxModelProjectRest
 * WHAT IT DOES: The rest of command $0C's projection: each point's
 * screen position and outcode, then the next command.
 * HOW: Reached by jump from 01:8BFE.
 * Entry: R10 = stack; R11 = return address; R14 at the command's bytes
 * (bank $001C).
 * Exit: as the code leaves the registers, flags and RAM; R14 and the
 * ROM buffer move as on the Super FX.
 */
void abi_FxModelProjectRest(void)
{
    model_run(0x8C79, R[11]);
}

/*
 * @routine 01:8D4D FxModelListStart
 * WHAT IT DOES: Command $58: starts the sub-list returns: $0056 = $0788
 * and 01:8E7D pushed on the R10 stack, then the next command.
 * HOW: CACHE; dispatch.
 * Entry: R10 = stack; R11 = return address; R14 at the command's bytes
 * (bank $001C).
 * Exit: as the code leaves the registers, flags and RAM; R14 and the
 * ROM buffer move as on the Super FX.
 */
void abi_FxModelListStart(void)
{
    model_run(0x8D4D, R[11]);
}

/*
 * @routine 01:8D5D FxModelCall
 * WHAT IT DOES: Command $60: calls the commands at the signed word
 * offset that follows, the return (past the word) kept at the $0056
 * stack.
 * HOW: R14 refilled (DEC, INC); the return stored and $0056 stepped;
 * R14 += the offset; dispatch.
 * Entry: R10 = stack; R11 = return address; R14 at the command's bytes
 * (bank $001C).
 * Exit: as the code leaves the registers, flags and RAM; R14 and the
 * ROM buffer move as on the Super FX.
 */
void abi_FxModelCall(void)
{
    model_run(0x8D5D, R[11]);
}

/*
 * @routine 01:8D75 FxModelReturnPull
 * WHAT IT DOES: Command $64: return, pulling the next command address
 * from the R10 stack.
 * HOW: DEC R10 twice, R15 from (R10).
 * Entry: R10 = stack; R11 = return address; R14 at the command's bytes
 * (bank $001C).
 * Exit: as the code leaves the registers, flags and RAM; R14 and the
 * ROM buffer move as on the Super FX.
 */
void abi_FxModelReturnPull(void)
{
    model_run(0x8D75, R[11]);
}

/*
 * @routine 01:8D7A FxModelIfCall
 * WHAT IT DOES: Command $5C: the flag byte at $07C6 plus the next byte
 * picks, by its bit 7, one of two lists at the word offsets that
 * follow; the one picked runs with R14 pushed on the R10 stack and
 * comes back past both.
 * HOW: R14 pushed on the R10 stack around the call; two forms (8D7A,
 * 8DBD).
 * Entry: R10 = stack; R11 = return address; R14 at the command's bytes
 * (bank $001C).
 * Exit: as the code leaves the registers, flags and RAM; R14 and the
 * ROM buffer move as on the Super FX.
 */
void abi_FxModelIfCall(void)
{
    model_run(0x8D7A, R[11]);
}

/*
 * @routine 01:8DF6 FxModelListStart3C
 * WHAT IT DOES: Command $3C: as command $58.
 * HOW: $0056 = $0788, 01:8E7D pushed, dispatch.
 * Entry: R10 = stack; R11 = return address; R14 at the command's bytes
 * (bank $001C).
 * Exit: as the code leaves the registers, flags and RAM; R14 and the
 * ROM buffer move as on the Super FX.
 */
void abi_FxModelListStart3C(void)
{
    model_run(0x8DF6, R[11]);
}

/*
 * @routine 01:8E06 FxModelPushReturn
 * WHAT IT DOES: Command $44: keeps the address at the word offset that
 * follows on the $0056 stack, then returns as command $40.
 * HOW: Falls into 01:8E14.
 * Entry: R10 = stack; R11 = return address; R14 at the command's bytes
 * (bank $001C).
 * Exit: as the code leaves the registers, flags and RAM; R14 and the
 * ROM buffer move as on the Super FX.
 */
void abi_FxModelPushReturn(void)
{
    model_run(0x8E06, R[11]);
}

/*
 * @routine 01:8E14 FxModelReturn40
 * WHAT IT DOES: Command $40: return, pulling the next command address
 * from the R10 stack.
 * HOW: DEC R10 twice, R15 from (R10).
 * Entry: R10 = stack; R11 = return address; R14 at the command's bytes
 * (bank $001C).
 * Exit: as the code leaves the registers, flags and RAM; R14 and the
 * ROM buffer move as on the Super FX.
 */
void abi_FxModelReturn40(void)
{
    model_run(0x8E14, R[11]);
}

/*
 * @routine 01:8E19 FxModelIfSkip
 * WHAT IT DOES: Command $28: the flag byte at $07C6 plus the next byte
 * picks, by its bit 7, whether the list that follows runs (R14 pushed
 * on the R10 stack) or is passed by its length byte.
 * HOW: R14 pushed on the R10 stack around each list.
 * Entry: R10 = stack; R11 = return address; R14 at the command's bytes
 * (bank $001C).
 * Exit: as the code leaves the registers, flags and RAM; R14 and the
 * ROM buffer move as on the Super FX.
 */
void abi_FxModelIfSkip(void)
{
    model_run(0x8E19, R[11]);
}

/*
 * @routine 01:8EA7 FxModelDepthSort
 * WHAT IT DOES: Command $10: n sub-lists (n the next byte) run back to
 * front: the depth words of the n points the following bytes pick
 * ($0344 + 6 * index) copied to $07B0, then n times the deepest left
 * marked $8000 and its list (a word offset in the table after the
 * bytes) run, coming back to 01:8EF9 (not reached in the traces so
 * far).
 * HOW: $0148 = n, $014E counts down, $0150 = the offset table; the end
 * command when done.
 * Entry: R10 = stack; R11 = return address; R14 at the command's bytes
 * (bank $001C).
 * Exit: as the code leaves the registers, flags and RAM; R14 and the
 * ROM buffer move as on the Super FX.
 */
void abi_FxModelDepthSort(void)
{
    model_run(0x8EA7, R[11]);
}

/*
 * @routine 01:8F05 FxModelIfSprite
 * WHAT IT DOES: Command $54: the flag byte at $07C6 plus the next byte;
 * with bit 7 set, command $50 follows (01:8F18), else its bytes are
 * skipped (not reached in the traces so far).
 * HOW: SWAP, HIB, BMI.
 * Entry: R10 = stack; R11 = return address; R14 at the command's bytes
 * (bank $001C).
 * Exit: as the code leaves the registers, flags and RAM; R14 and the
 * ROM buffer move as on the Super FX.
 */
void abi_FxModelIfSprite(void)
{
    model_run(0x8F05, R[11]);
}

/*
 * @routine 01:8F18 FxModelSprite
 * WHAT IT DOES: Command $50: a shape at the projected point ($04E4 + 6
 * * the next byte) whose depth, against $002A, sets its size; its
 * colour from FxModelSpriteColour (01:9250) and a LINK to 01:AB11 (not
 * reached in the traces so far).
 * HOW: $00BA-$00D2 set for it; ROMB and R14 back from $001C and $001A.
 * Entry: R10 = stack; R11 = return address; R14 at the command's bytes
 * (bank $001C).
 * Exit: as the code leaves the registers, flags and RAM; R14 and the
 * ROM buffer move as on the Super FX.
 */
void abi_FxModelSprite(void)
{
    model_run(0x8F18, R[11]);
}

/*
 * @routine 01:8F7D FxModelMaterial
 * WHAT IT DOES: Sets the face colour (and for textured faces the
 * texture's bank, address and mask) from the face's material word in
 * ROM: a shade from the light ($0106-$010A) for shaded materials.
 * HOW: R2 keeps R14; ROMB $03; the material table at $004A, looked up
 * through $0046 for animated ones; the shade table at $004C; ROMB and
 * R14 back. Returns with JMP R11.
 * Entry: R14 at the face's material word; R11 = return address.
 * Exit: as the code leaves the registers, flags and RAM; R14 and the
 * ROM buffer move as on the Super FX.
 */
void abi_FxModelMaterial(void)
{
    model_run(0x8F7D, R[11]);
}

/*
 * @routine 01:90CD FxModelFaces
 * WHAT IT DOES: Command $14: the faces that follow, each with its
 * material (FxModelMaterial), its points gathered and clipped
 * (FxPolygonGather or FxPolygonDraw, FxPolygonClipScreen) and queued in
 * the draw list, which is drawn (FxDrawList) when nearly full.
 * HOW: CACHE; a face count byte less than 0 ends it.
 * Entry: R10 = stack; R11 = return address; R14 at the command's bytes
 * (bank $001C).
 * Exit: as the code leaves the registers, flags and RAM; R14 and the
 * ROM buffer move as on the Super FX.
 */
void abi_FxModelFaces(void)
{
    model_run(0x90CD, R[11]);
}

/*
 * @routine 01:91E7 FxModelFacesGatheredUV
 * WHAT IT DOES: FxModelFaces' way to the textured clip (01:9205,
 * FxPolygonClipScreenUV) for a face gathered by FxPolygonGather whose
 * kind ($0042) has its high byte set, then the face queued and the next
 * one. The game never runs it: a face with that high byte and points off
 * the screen was drawn by FxPolygonDraw before it instead, and the
 * gather does not change the kind, so the BEQ at 01:91E4 always
 * branches. A unit start runs it so the C is judged against the
 * original.
 * HOW: IWT R15 to 01:9205 (NOP in the delay slot); then as FxModelFaces.
 * Entry: R0 = the gathered point count; R10 = stack; R14 at the face
 * list; the ROM bank the model's.
 * Exit: as the code leaves the registers, flags and RAM; R14 and the
 * ROM buffer move as on the Super FX.
 */
void abi_FxModelFacesGatheredUV(void)
{
    model_run(0x91E7, R[11]);
}

/*
 * @routine 01:9202 FxModelFacesOnScreenUV
 * WHAT IT DOES: FxModelFaces' textured clip (FxPolygonClipScreenUV) of a
 * face whose kind ($0042) has its high byte set and whose points are
 * all on the screen (outcodes 0), then the face queued and the next one.
 * No play has reached it so far. A unit start runs it so the C is judged
 * against the original.
 * HOW: R0 = the point count ($0146); LINK to FxPolygonClipScreenUV; a
 * kept polygon is queued at 01:920F.
 * Entry: R10 = stack; R14 at the face list; the polygon's points in
 * place; the ROM bank the model's.
 * Exit: as the code leaves the registers, flags and RAM; R14 and the
 * ROM buffer move as on the Super FX.
 */
void abi_FxModelFacesOnScreenUV(void)
{
    model_run(0x9202, R[11]);
}

/*
 * @routine 01:9243 FxModelEnd
 * WHAT IT DOES: Command $00: draws the queued faces (FxDrawList) and
 * returns from the model, pulling R15.
 * HOW: LINK to FxDrawList, then DEC R10 twice and R15 from (R10).
 * Entry: R10 = stack; R11 = return address; R14 at the command's bytes
 * (bank $001C).
 * Exit: as the code leaves the registers, flags and RAM; R14 and the
 * ROM buffer move as on the Super FX.
 */
void abi_FxModelEnd(void)
{
    model_run(0x9243, R[11]);
}

/*
 * @routine 01:9250 FxModelSpriteColour
 * WHAT IT DOES: The sprite's colour: as FxModelMaterial for the flat
 * case (not reached in the traces so far).
 * HOW: Shares FxSpriteMaterial's ending (01:939E).
 * Entry: R14 at the sprite's bytes; R11 = return address.
 * Exit: as the code leaves the registers, flags and RAM; R14 and the
 * ROM buffer move as on the Super FX.
 */
void abi_FxModelSpriteColour(void)
{
    model_run(0x9250, R[11]);
}

/*
 * @routine 01:9276 FxSpriteMaterial
 * WHAT IT DOES: Sets the colour of a sprite from its material word: as
 * FxModelMaterial without the textured faces.
 * HOW: R2 keeps R14; ends at 939E: ROMB from $001C, R14 = R2, JMP R11.
 * Entry: R0 = the material index; R11 = return address.
 * Exit: as the code leaves the registers, flags and RAM; R14 and the
 * ROM buffer move as on the Super FX.
 */
void abi_FxSpriteMaterial(void)
{
    model_run(0x9276, R[11]);
}

/*
 * @routine 01:93A7 FxModelFacing
 * WHAT IT DOES: Command $30: for n triangles (n the next byte) of three
 * projected points ($04E4 + 6 * index), the sign of their cross product
 * XORed with the points' attribute bytes, stored as a flag byte at
 * $07C6 on (skipped to 01:941B when bit 14 of $0054 is set).
 * HOW: A LOOP of R12 = n (R13 = $93C1); LMULT twice, SUB and SBC.
 * Entry: R10 = stack; R11 = return address; R14 at the command's bytes
 * (bank $001C).
 * Exit: as the code leaves the registers, flags and RAM; R14 and the
 * ROM buffer move as on the Super FX.
 */
void abi_FxModelFacing(void)
{
    model_run(0x93A7, R[11]);
}

/*
 * @routine 01:9A52 FxPolygonClipScreenUV
 * WHAT IT DOES: Clips a textured polygon (x, y, u, v words a vertex) at
 * $005A against the four screen edges, as FxPolygonClipScreen does for
 * flat ones (not reached in the traces so far).
 * HOW: Pushes R11; the crossings by FxDivideSigned; pulls R15.
 * Entry: R0 = the vertex count; R10 = stack; R11 = return address.
 * Exit: as the code leaves the registers, flags and RAM; R14 and the
 * ROM buffer move as on the Super FX.
 */
void abi_FxPolygonClipScreenUV(void)
{
    model_run(0x9A52, R[11]);
}

/*
 * @routine 01:AB11 FxReturnR11
 * WHAT IT DOES: A routine of one JMP R11: returns at once (not reached
 * in the traces so far).
 * HOW: JMP R11; NOP.
 * Entry: R11 = return address.
 * Exit: as the code leaves the registers, flags and RAM; R14 and the
 * ROM buffer move as on the Super FX.
 */
void abi_FxReturnR11(void)
{
    
    model_run(0xAB11, R[11]);
}
