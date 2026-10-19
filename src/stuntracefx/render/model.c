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
    goto dispatch;
    /* FxModelDraw */
l8561:
    R[0] = gsu_ldw(FX_MODEL_1B4);
    R[0] = gsu_lob(R[0]);
    t = gsu.z;
    if (t) { gsu_code(0x018565, 3); gsu_code(0x018584, 3); goto l8584; }
    gsu_stw(R[10], R[11]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[11] = 0x8571;
    goto l8576;
l8571:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
    /* FxModelDrawFlagged */
l8576:
    R[2] = 0x1000;
    R[1] = 0x2000;
    R[0] = gsu_ldw(FX_MODEL_FLAGS);
    R[0] = gsu_and(R[0], R[2]);
    R[0] = gsu_or(R[0], R[1]);
    t = 1;
    if (t) goto l858F;
l8584:
    gsu_stw(R[10], R[11]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    R[11] = gsu_ldw(gsu_code_then(0x018587, 6, R[10]));
    R[0] = 0x8000;
l858F:
    gsu_code(0x01858D, 6);
    gsu_stw(FX_MODEL_FLAGS, R[0]);
    gsu_code(0x018593, 2);
    gsu_stw(R[10], R[11]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[0] = 0x0003;
    gsu_code(0x018595, 12);
    gsu_romb(R[0]);
    R[0] = 0x08AA;
    gsu_stw(FX_LIST_END, R[0]);
    R[0] = 0x0003;
    gsu_code(0x0185A1, 7);
    gsu_romb(R[0]);
    gsu_r14(gsu_ldw(FX_MODEL_LIST));
    R[0] = gsu_ldw(gsu_code_then(0x0185A8, 3, FX_MODEL_1B6));
    R[0] = gsu_lob(R[0]);
    t = gsu.z;
    if (t) { gsu_code(0x0185AB, 3); gsu_code(0x0185B6, 5); goto l85B6; }
    R[0] = gsu_dec(R[0]);
    R[0] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_add(R[0], R[0], 0);
    R[2] = 0xAE45;
    gsu_r14(gsu_add(R[0], R[2], 0));
l85B6:
    R[0] = gsu_ldw(FX_CAR_108C);
    R[0] = gsu_lob(R[0]);
    t = gsu.z;
    R[0] = gsu_sub(R[0], R[0], 0);
    if (t) { gsu_code(0x0185BB, 3); gsu_code(0x018609, 4); goto l8609; }
    R[1] = gsu_ldw(FX_OBJECT_Z);
    R[0] = gsu_getbh(R[0]);
    gsu_r14(gsu_inc(R[14]));
    R[0] = gsu_add(R[0], R[1], 0);
    t = !gsu.s;
    if (t) goto l85D2;
    R[0] = 0xA275;
    R[2] = 0x0000;
    goto l85FC;
l85D2:
    R[0] = gsu_sub(R[0], R[0], 0);
    R[0] = gsu_getbh(R[0]);
    gsu_r14(gsu_inc(R[14]));
    R[0] = gsu_add(R[0], R[1], 0);
    t = !gsu.s;
    if (t) goto l85E4;
    R[0] = 0xA2A1;
    R[2] = 0x0088;
    goto l85FC;
l85E4:
    R[0] = gsu_sub(R[0], R[0], 0);
    R[0] = gsu_getbh(R[0]);
    gsu_r14(gsu_inc(R[14]));
    R[0] = gsu_add(R[0], R[1], 0);
    t = !gsu.s;
    if (t) goto l85F6;
    R[0] = 0xA2CD;
    R[2] = 0x0110;
    goto l85FC;
l85F6:
    R[0] = 0xA2F9;
    R[2] = 0x0198;
l85FC:
    gsu_stw(FX_SHADE_TABLE, R[0]);
    R[0] = gsu_ldw(FX_SHADE_BASE);
    R[0] = gsu_add(R[0], R[2], 0);
    gsu_stw(FX_SHADE_AT, R[0]);
    t = 1;
    if (t) goto l8651;
l8609:
    R[1] = gsu_ldw(FX_OBJECT_Z);
    R[0] = gsu_getbh(gsu_code_then(0x01860D, 2, R[0]));
    gsu_code(0x01860F, 1);
    gsu_r14(gsu_inc(R[14]));
    R[0] = gsu_add(R[0], R[1], 0);
    t = !gsu.s;
    if (t) { gsu_code(0x018610, 3); gsu_code(0x01861D, 4); goto l861D; } else { gsu_code(0x018610, 13); }
    R[0] = 0xA1C5;
    R[2] = 0x0000;
    goto l8647;
l861D:
    R[0] = gsu_sub(R[0], R[0], 0);
    R[0] = gsu_getbh(R[0]);
    gsu_code(0x018621, 1);
    gsu_r14(gsu_inc(R[14]));
    R[0] = gsu_add(R[0], R[1], 0);
    t = !gsu.s;
    if (t) { gsu_code(0x018622, 3); gsu_code(0x01862F, 4); goto l862F; } else { gsu_code(0x018622, 13); }
    R[0] = 0xA1F1;
    R[2] = 0x0088;
    goto l8647;
l862F:
    R[0] = gsu_sub(R[0], R[0], 0);
    R[0] = gsu_getbh(R[0]);
    gsu_code(0x018633, 1);
    gsu_r14(gsu_inc(R[14]));
    R[0] = gsu_add(R[0], R[1], 0);
    t = !gsu.s;
    if (t) { gsu_code(0x018634, 3); gsu_code(0x018641, 6); goto l8641; } else { gsu_code(0x018634, 13); }
    R[0] = 0xA21D;
    R[2] = 0x0110;
    goto l8647;
l8641:
    R[0] = 0xA249;
    R[2] = 0x0198;
l8647:
    gsu_code(0x018647, 4);
    gsu_stw(FX_SHADE_TABLE, R[0]);
    R[0] = gsu_ldw(gsu_code_then(0x01864B, 3, FX_SHADE_BASE));
    R[0] = gsu_add(R[0], R[2], 0);
    gsu_code(0x01864E, 4);
    gsu_stw(FX_SHADE_AT, R[0]);
l8651:
    /* CACHE */
    R[0] = 0x0001;
    R[12] = gsu_ldw(FX_POINT_SHIFT);
    R[12] = R[12];
    gsu_moves(R[12]);
    t = gsu.z;
    if (t) goto l8660; else { gsu_code(0x01865C, 2); }
    R[13] = 0x865E;
l865E:
    gsu_code(0x01865E, 2);
    t = gsu_count_down(&R[12]);
    R[0] = gsu_add(R[0], R[0], 0);
    if (t) goto l865E;
l8660:
    gsu_code(0x018658, 4); gsu_code(0x018660, 4);
    gsu_stw(FX_POINT_SCALE, R[0]);
    R[1] = gsu_ldw(gsu_code_then(0x018664, 3, FX_OBJECT_ANGLE_X));
    R[0] = gsu_lob(R[1]);
    R[0] = gsu_swap(R[0]);
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_code(0x018667, 6);
    gsu_sbk(R[0]);
    R[2] = gsu_ldw(gsu_code_then(0x01866D, 3, FX_OBJECT_ANGLE_Y));
    R[0] = gsu_lob(R[2]);
    R[0] = gsu_swap(R[0]);
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_code(0x018670, 6);
    gsu_sbk(R[0]);
    R[3] = gsu_ldw(gsu_code_then(0x018676, 3, FX_OBJECT_ANGLE_Z));
    R[0] = gsu_lob(R[3]);
    R[0] = gsu_swap(R[0]);
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_code(0x018679, 6);
    gsu_sbk(R[0]);
    R[0] = gsu_or(R[1], R[3]);
    t = gsu.z;
    if (t) { gsu_code(0x01867F, 4); gsu_code(0x018687, 4); goto l8687; } else { gsu_code(0x01867F, 8); }
    goto l8741;
l8687:
    R[0] = gsu_or(R[0], R[2]);
    t = !gsu.z;
    if (t) { gsu_code(0x0186D8, 8); goto l86D8; } else { gsu_code(0x01868B, 4); }
    R[0] = gsu_ldw(FX_OBJECT_MATRIX_0);
    gsu_code(0x01868F, 3);
    gsu_stw(FX_VIEW_MATRIX_0, R[0]);
    R[0] = gsu_ldw(gsu_code_then(0x018692, 3, FX_OBJECT_MATRIX_1));
    gsu_code(0x018695, 3);
    gsu_stw(FX_VIEW_MATRIX_1, R[0]);
    R[0] = gsu_ldw(gsu_code_then(0x018698, 3, FX_OBJECT_MATRIX_2));
    gsu_code(0x01869B, 3);
    gsu_stw(FX_VIEW_MATRIX_2, R[0]);
    R[0] = gsu_ldw(gsu_code_then(0x01869E, 3, FX_OBJECT_MATRIX_3));
    gsu_code(0x0186A1, 3);
    gsu_stw(FX_VIEW_MATRIX_3, R[0]);
    R[0] = gsu_ldw(gsu_code_then(0x0186A4, 3, FX_OBJECT_MATRIX_4));
    gsu_code(0x0186A7, 3);
    gsu_stw(FX_VIEW_MATRIX_4, R[0]);
    R[0] = gsu_ldw(gsu_code_then(0x0186AA, 3, FX_OBJECT_MATRIX_5));
    gsu_code(0x0186AD, 3);
    gsu_stw(FX_VIEW_MATRIX_5, R[0]);
    R[0] = gsu_ldw(gsu_code_then(0x0186B0, 3, FX_OBJECT_MATRIX_6));
    gsu_code(0x0186B3, 3);
    gsu_stw(FX_VIEW_MATRIX_6, R[0]);
    R[0] = gsu_ldw(gsu_code_then(0x0186B6, 3, FX_OBJECT_MATRIX_7));
    gsu_code(0x0186B9, 3);
    gsu_stw(FX_VIEW_MATRIX_7, R[0]);
    R[0] = gsu_ldw(gsu_code_then(0x0186BC, 3, FX_OBJECT_MATRIX_8));
    gsu_code(0x0186BF, 3);
    gsu_stw(FX_VIEW_MATRIX_8, R[0]);
    R[0] = gsu_ldw(gsu_code_then(0x0186C2, 3, FX_MODEL_FLAGS));
    R[1] = 0x2000;
    R[0] = gsu_and(R[0], R[1]);
    t = gsu.z;
    R[0] = gsu_sub(R[0], R[0], 0);
    if (t) { gsu_code(0x0186C5, 6); gsu_code(0x0186D4, 4); goto l86D4; }
    gsu_stw(FX_VIEW_MATRIX_3, R[0]);
    gsu_stw(FX_VIEW_MATRIX_4, R[0]);
    gsu_stw(FX_VIEW_MATRIX_5, R[0]);
l86D4:
    goto l8778;
l86D8:
    R[4] = 0x0040;
    R[0] = gsu_sub(R[2], R[4], 0);
    t = !gsu.z;
    if (t) { gsu_code(0x0186E4, 4); goto l86E4; } else { gsu_code(0x0186E0, 4); }
    goto l8741;
l86E4:
    R[0] = gsu_sub(R[0], R[4], 0);
    t = !gsu.z;
    if (t) goto l8741; else { gsu_code(0x0186E8, 4); }
    R[0] = gsu_ldw(FX_OBJECT_MATRIX_0);
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_code(0x0186EC, 5);
    gsu_stw(FX_VIEW_MATRIX_0, R[0]);
    R[0] = gsu_ldw(gsu_code_then(0x0186F1, 3, FX_OBJECT_MATRIX_1));
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_code(0x0186F4, 5);
    gsu_stw(FX_VIEW_MATRIX_1, R[0]);
    R[0] = gsu_ldw(gsu_code_then(0x0186F9, 3, FX_OBJECT_MATRIX_2));
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_code(0x0186FC, 5);
    gsu_stw(FX_VIEW_MATRIX_2, R[0]);
    R[0] = gsu_ldw(gsu_code_then(0x018701, 3, FX_OBJECT_MATRIX_3));
    gsu_code(0x018704, 3);
    gsu_stw(FX_VIEW_MATRIX_3, R[0]);
    R[0] = gsu_ldw(gsu_code_then(0x018707, 3, FX_OBJECT_MATRIX_4));
    gsu_code(0x01870A, 3);
    gsu_stw(FX_VIEW_MATRIX_4, R[0]);
    R[0] = gsu_ldw(gsu_code_then(0x01870D, 3, FX_OBJECT_MATRIX_5));
    gsu_code(0x018710, 3);
    gsu_stw(FX_VIEW_MATRIX_5, R[0]);
    R[0] = gsu_ldw(gsu_code_then(0x018713, 3, FX_OBJECT_MATRIX_6));
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_code(0x018716, 5);
    gsu_stw(FX_VIEW_MATRIX_6, R[0]);
    R[0] = gsu_ldw(gsu_code_then(0x01871B, 3, FX_OBJECT_MATRIX_7));
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_code(0x01871E, 5);
    gsu_stw(FX_VIEW_MATRIX_7, R[0]);
    R[0] = gsu_ldw(gsu_code_then(0x018723, 3, FX_OBJECT_MATRIX_8));
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_code(0x018726, 5);
    gsu_stw(FX_VIEW_MATRIX_8, R[0]);
    R[0] = gsu_ldw(gsu_code_then(0x01872B, 3, FX_MODEL_FLAGS));
    R[1] = 0x2000;
    R[0] = gsu_and(R[0], R[1]);
    t = gsu.z;
    R[0] = gsu_sub(R[0], R[0], 0);
    if (t) { gsu_code(0x01872E, 6); gsu_code(0x01873D, 4); goto l873D; }
    gsu_stw(FX_VIEW_MATRIX_3, R[0]);
    gsu_stw(FX_VIEW_MATRIX_4, R[0]);
    gsu_stw(FX_VIEW_MATRIX_5, R[0]);
l873D:
    goto l8778;
l8741:
    R[9] = 0x0116;
    R[11] = 0x8749;
    (gsu_code(0x018741, 8), abi_FxMatrixFromAngles01());
l8749:
    R[9] = 0x0116;
    R[11] = 0x8751;
    abi_FxMatrixTranspose();
l8751:
    R[0] = gsu_ldw(FX_MODEL_FLAGS);
    R[1] = 0x2000;
    R[0] = gsu_and(R[0], R[1]);
    t = gsu.z;
    R[0] = gsu_sub(R[0], R[0], 0);
    if (t) goto l8770;
    gsu_stw(FX_MODEL_118, R[0]);
    gsu_stw(FX_MODEL_11E, R[0]);
    gsu_stw(FX_MODEL_124, R[0]);
    R[9] = 0x0132;
    R[11] = 0x876C;
    abi_FxMatrixApplyVectors();
l876C:
    goto l8778;
l8770:
    R[9] = 0x0132;
    R[11] = 0x8778;
    abi_FxMatrixApplyVectors();
l8778:
    R[0] = 0x0000;
    gsu_code(0x018778, 7);
    gsu_stw(FX_LIGHT_X, R[0]);
    R[0] = 0x393E;
    gsu_code(0x01877F, 6);
    gsu_stw(FX_LIGHT_Y, R[0]);
    R[0] = 0x727C;
    gsu_code(0x018785, 6);
    gsu_stw(FX_LIGHT_Z, R[0]);
    R[1] = gsu_ldw(gsu_code_then(0x01878B, 3, FX_LIGHT_X));
    R[2] = gsu_ldw(gsu_code_then(0x01878E, 3, FX_LIGHT_Y));
    R[3] = gsu_ldw(gsu_code_then(0x018791, 3, FX_LIGHT_Z));
    R[6] = gsu_ldw(gsu_code_then(0x018794, 3, FX_VIEW_MATRIX_0));
    R[0] = gsu_fmult(R[1], R[6]);
    R[5] = gsu_rol(R[0]);
    R[6] = gsu_ldw(gsu_code_then(0x018797, 7, FX_VIEW_MATRIX_1));
    R[0] = gsu_fmult(R[2], R[6]);
    R[0] = gsu_rol(R[0]);
    R[5] = gsu_add(R[5], R[0], 0);
    R[6] = gsu_ldw(gsu_code_then(0x01879E, 8, FX_VIEW_MATRIX_2));
    R[0] = gsu_fmult(R[3], R[6]);
    R[0] = gsu_rol(R[0]);
    R[0] = gsu_add(R[0], R[5], 0);
    R[0] = gsu_hib(R[0]);
    R[0] = gsu_sex(R[0]);
    gsu_code(0x0187A6, 9);
    gsu_stw(FX_LIGHT_LOCAL_X, R[0]);
    R[6] = gsu_ldw(gsu_code_then(0x0187AF, 3, FX_VIEW_MATRIX_3));
    R[0] = gsu_fmult(R[1], R[6]);
    R[5] = gsu_rol(R[0]);
    R[6] = gsu_ldw(gsu_code_then(0x0187B2, 7, FX_VIEW_MATRIX_4));
    R[0] = gsu_fmult(R[2], R[6]);
    R[0] = gsu_rol(R[0]);
    R[5] = gsu_add(R[5], R[0], 0);
    R[6] = gsu_ldw(gsu_code_then(0x0187B9, 8, FX_VIEW_MATRIX_5));
    R[0] = gsu_fmult(R[3], R[6]);
    R[0] = gsu_rol(R[0]);
    R[0] = gsu_add(R[0], R[5], 0);
    R[0] = gsu_hib(R[0]);
    R[0] = gsu_sex(R[0]);
    gsu_code(0x0187C1, 9);
    gsu_stw(FX_LIGHT_LOCAL_Y, R[0]);
    R[6] = gsu_ldw(gsu_code_then(0x0187CA, 3, FX_VIEW_MATRIX_6));
    R[0] = gsu_fmult(R[1], R[6]);
    R[5] = gsu_rol(R[0]);
    R[6] = gsu_ldw(gsu_code_then(0x0187CD, 7, FX_VIEW_MATRIX_7));
    R[0] = gsu_fmult(R[2], R[6]);
    R[0] = gsu_rol(R[0]);
    R[5] = gsu_add(R[5], R[0], 0);
    R[6] = gsu_ldw(gsu_code_then(0x0187D4, 8, FX_VIEW_MATRIX_8));
    R[0] = gsu_fmult(R[3], R[6]);
    R[0] = gsu_rol(R[0]);
    R[0] = gsu_add(R[0], R[5], 0);
    R[0] = gsu_hib(R[0]);
    R[0] = gsu_sex(R[0]);
    gsu_code(0x0187DC, 9);
    gsu_stw(FX_LIGHT_LOCAL_Z, R[0]);
    R[0] = gsu_ldw(gsu_code_then(0x0187E5, 3, FX_POINT_SHIFT));
    R[0] = gsu_sub(R[0], 3, 0);
    t = !gsu.s;
    if (t) { gsu_code(0x0187E8, 4); gsu_code(0x01882C, 1); goto l882C; } else { gsu_code(0x0187E8, 11); }
    R[2] = 0x0133;
    R[1] = gsu_ldb(R[2]);
    R[2] = gsu_inc(R[2]);
    R[2] = gsu_inc(R[2]);
    R[0] = gsu_ldb(gsu_code_then(0x0187F3, 4, R[2]));
    R[0] = gsu_swap(R[0]);
    R[0] = gsu_or(R[0], R[1]);
    gsu_code(0x0187F7, 5);
    gsu_stw(FX_MATRIX_PACKED_0, R[0]);
    R[2] = gsu_inc(R[2]);
    R[2] = gsu_inc(R[2]);
    R[1] = gsu_ldb(gsu_code_then(0x0187FC, 5, R[2]));
    R[2] = gsu_inc(R[2]);
    R[2] = gsu_inc(R[2]);
    R[0] = gsu_ldb(gsu_code_then(0x018801, 4, R[2]));
    R[0] = gsu_swap(R[0]);
    R[0] = gsu_or(R[0], R[1]);
    gsu_code(0x018805, 5);
    gsu_stw(FX_MATRIX_PACKED_1, R[0]);
    R[2] = gsu_inc(R[2]);
    R[2] = gsu_inc(R[2]);
    R[1] = gsu_ldb(gsu_code_then(0x01880A, 5, R[2]));
    R[2] = gsu_inc(R[2]);
    R[2] = gsu_inc(R[2]);
    R[0] = gsu_ldb(gsu_code_then(0x01880F, 4, R[2]));
    R[0] = gsu_swap(R[0]);
    R[0] = gsu_or(R[0], R[1]);
    gsu_code(0x018813, 5);
    gsu_stw(FX_MATRIX_PACKED_2, R[0]);
    R[2] = gsu_inc(R[2]);
    R[2] = gsu_inc(R[2]);
    R[1] = gsu_ldb(gsu_code_then(0x018818, 5, R[2]));
    R[2] = gsu_inc(R[2]);
    R[2] = gsu_inc(R[2]);
    R[0] = gsu_ldb(gsu_code_then(0x01881D, 4, R[2]));
    R[0] = gsu_swap(R[0]);
    R[0] = gsu_or(R[0], R[1]);
    gsu_code(0x018821, 5);
    gsu_stw(FX_MATRIX_PACKED_3, R[0]);
    R[2] = gsu_inc(R[2]);
    R[2] = gsu_inc(R[2]);
    R[0] = gsu_ldb(gsu_code_then(0x018826, 4, R[2]));
    gsu_code(0x01882A, 3);
    gsu_stw(FX_MATRIX_PACKED_4, R[0]);
l882C:
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_code(0x01882D, 4);
    gsu_stw(FX_POINTS_TOTAL, R[0]);
    R[0] = 0x0340;
    gsu_code(0x018831, 6);
    gsu_stw(FX_POINT_OUT, R[0]);
    R[0] = gsu_ldw(gsu_code_then(0x018837, 3, FX_ROM_BANK));
    gsu_code(0x01883A, 5);
    gsu_romb(R[0]);
    gsu_r14(gsu_ldw(FX_MODEL_AT));
    gsu_code(0x01883F, 3);
    gsu_stw(FX_ROM_R14, R[14]);
    R[0] = 0x9243;
    gsu_code(0x018842, 4);
    gsu_stw(R[10], R[0]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    nx = (uint16_t)((gsu_code(0x018846, 4), gsu_getb()));
    gsu_r14(gsu_inc(R[14]));
    at = model_command(nx);
    goto dispatch;
    /* FxModelReturn */
l884A:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
    /* FxModelFramePick */
l884F:
    R[1] = (gsu_code(0x01001C, 4), gsu_code(0x01884F, 7), gsu_getb());
    gsu_r14(gsu_inc(R[14]));
    R[0] = gsu_ldw(FX_MODEL_FRAME);
    R[2] = 0x003F;
    R[0] = gsu_and(R[0], R[2]);
l8858:
    gsu_code(0x018856, 7); gsu_code(0x018860, 4);
    gsu_sub(R[0], R[1], 0);   /* CMP */
    t = gsu.s;
    if (t) goto l8860;
    t = 1;
    R[0] = gsu_sub(R[0], R[1], 0);
    if (t) goto l8858;
l8860:
    R[0] = gsu_add(R[0], R[0], 0);
    gsu_r14(gsu_add(R[14], R[0], 0));
    /* FxModelJump */
l8863:
    R[0] = (gsu_code(0x018864, 1), gsu_getb());
    gsu_code(0x018865, 1);
    gsu_r14(gsu_inc(R[14]));
    R[0] = gsu_getbh(gsu_code_then(0x018866, 2, R[0]));
    gsu_code(0x018868, 2);
    gsu_r14(gsu_add(R[14], R[0], 0));
    nx = (uint16_t)((gsu_code(0x01886A, 2), gsu_getb()));
    gsu_r14(gsu_inc(R[14]));
    at = model_command(nx);
    goto dispatch;
    /* FxModelPointsFull */
l886C:
    R[12] = gsu_getb();
    gsu_r14(gsu_inc(R[14]));
    R[0] = gsu_ldw(FX_POINTS_TOTAL);
    R[0] = gsu_add(R[0], R[12], 0);
    gsu_sbk(R[0]);
    R[9] = gsu_ldw(FX_POINT_OUT);
    gsu_stw(FX_MODEL_STACK, R[10]);
    R[10] = gsu_ldw(FX_VIEW_MATRIX_0);
    R[11] = gsu_ldw(FX_VIEW_MATRIX_3);
    R[13] = gsu_ldw(FX_VIEW_MATRIX_6);
    R[4] = gsu_ldw(FX_VIEW_MATRIX_1);
    R[7] = gsu_ldw(FX_VIEW_MATRIX_4);
    R[8] = gsu_ldw(FX_VIEW_MATRIX_7);
    /* CACHE */
l888D:
    R[1] = gsu_getb();
    gsu_r14(gsu_inc(R[14]));
    R[1] = gsu_getbh(R[1]);
    gsu_r14(gsu_inc(R[14]));
    R[2] = gsu_getb();
    gsu_r14(gsu_inc(R[14]));
    R[2] = gsu_getbh(R[2]);
    gsu_r14(gsu_inc(R[14]));
    R[3] = gsu_getb();
    gsu_r14(gsu_inc(R[14]));
    R[3] = gsu_getbh(R[3]);
    gsu_r14(gsu_inc(R[14]));
    R[6] = R[10];
    R[0] = gsu_fmult(R[1], R[6]);
    R[5] = gsu_rol(R[0]);
    R[6] = R[11];
    R[0] = gsu_fmult(R[2], R[6]);
    R[0] = gsu_rol(R[0]);
    R[5] = gsu_add(R[5], R[0], 0);
    R[6] = R[13];
    R[0] = gsu_fmult(R[3], R[6]);
    R[0] = gsu_rol(R[0]);
    R[0] = gsu_add(R[0], R[5], 0);
    gsu_stw(R[9], R[0]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    R[6] = R[4];
    R[0] = gsu_fmult(R[1], R[6]);
    R[5] = gsu_rol(R[0]);
    R[6] = R[7];
    R[0] = gsu_fmult(R[2], R[6]);
    R[0] = gsu_rol(R[0]);
    R[5] = gsu_add(R[5], R[0], 0);
    R[6] = R[8];
    R[0] = gsu_fmult(R[3], R[6]);
    R[0] = gsu_rol(R[0]);
    R[0] = gsu_add(R[0], R[5], 0);
    gsu_stw(R[9], R[0]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    R[6] = gsu_ldw(FX_VIEW_MATRIX_2);
    R[0] = gsu_fmult(R[1], R[6]);
    R[5] = gsu_rol(R[0]);
    R[6] = gsu_ldw(FX_VIEW_MATRIX_5);
    R[0] = gsu_fmult(R[2], R[6]);
    R[0] = gsu_rol(R[0]);
    R[5] = gsu_add(R[5], R[0], 0);
    R[6] = gsu_ldw(FX_VIEW_MATRIX_8);
    R[0] = gsu_fmult(R[3], R[6]);
    R[0] = gsu_rol(R[0]);
    R[0] = gsu_add(R[0], R[5], 0);
    gsu_stw(R[9], R[0]);
    R[9] = gsu_inc(R[9]);
    R[12] = gsu_dec(R[12]);
    t = !gsu.z;
    R[9] = gsu_inc(R[9]);
    if (t) goto l888D;
    gsu_stw(FX_POINT_OUT, R[9]);
    R[10] = gsu_ldw(FX_MODEL_STACK);
    nx = (uint16_t)(gsu_getb());
    gsu_r14(gsu_inc(R[14]));
    at = model_command(nx);
    goto dispatch;
    /* FxModelPointsScaled */
l88F3:
    R[12] = gsu_getb();
    gsu_r14(gsu_inc(R[14]));
    R[0] = gsu_ldw(FX_POINTS_TOTAL);
    R[0] = gsu_add(R[0], R[12], 0);
    R[0] = gsu_add(R[0], R[12], 0);
    gsu_sbk(R[0]);
    R[9] = gsu_ldw(FX_POINT_OUT);
    /* CACHE */
    R[13] = 0x8902;
l8902:
    R[1] = gsu_getb();
    gsu_r14(gsu_inc(R[14]));
    R[1] = gsu_getbh(R[1]);
    gsu_r14(gsu_inc(R[14]));
    R[2] = gsu_getb();
    gsu_r14(gsu_inc(R[14]));
    R[2] = gsu_getbh(R[2]);
    gsu_r14(gsu_inc(R[14]));
    R[3] = gsu_getb();
    gsu_r14(gsu_inc(R[14]));
    R[3] = gsu_getbh(R[3]);
    gsu_r14(gsu_inc(R[14]));
    R[6] = gsu_ldw(FX_VIEW_MATRIX_0);
    R[0] = gsu_fmult(R[1], R[6]);
    R[8] = gsu_rol(R[0]);
    R[6] = gsu_ldw(FX_VIEW_MATRIX_3);
    R[0] = gsu_fmult(R[2], R[6]);
    R[5] = gsu_rol(R[0]);
    R[6] = gsu_ldw(FX_VIEW_MATRIX_6);
    R[0] = gsu_fmult(R[3], R[6]);
    R[0] = gsu_rol(R[0]);
    R[0] = gsu_add(R[0], R[5], 0);
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_stw(R[9], R[0]);
    R[0] = gsu_sub(R[0], R[8], 0);
    R[0] = gsu_sub(R[0], R[8], 0);
    R[9] = gsu_add(R[9], 6, 0);
    gsu_stw(R[9], R[0]);
    R[9] = gsu_sub(R[9], 4, 0);
    R[6] = gsu_ldw(FX_VIEW_MATRIX_1);
    R[0] = gsu_fmult(R[1], R[6]);
    R[8] = gsu_rol(R[0]);
    R[6] = gsu_ldw(FX_VIEW_MATRIX_4);
    R[0] = gsu_fmult(R[2], R[6]);
    R[5] = gsu_rol(R[0]);
    R[6] = gsu_ldw(FX_VIEW_MATRIX_7);
    R[0] = gsu_fmult(R[3], R[6]);
    R[0] = gsu_rol(R[0]);
    R[0] = gsu_add(R[0], R[5], 0);
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_stw(R[9], R[0]);
    R[0] = gsu_sub(R[0], R[8], 0);
    R[0] = gsu_sub(R[0], R[8], 0);
    R[9] = gsu_add(R[9], 6, 0);
    gsu_stw(R[9], R[0]);
    R[9] = gsu_sub(R[9], 4, 0);
    R[6] = gsu_ldw(FX_VIEW_MATRIX_2);
    R[0] = gsu_fmult(R[1], R[6]);
    R[8] = gsu_rol(R[0]);
    R[6] = gsu_ldw(FX_VIEW_MATRIX_5);
    R[0] = gsu_fmult(R[2], R[6]);
    R[5] = gsu_rol(R[0]);
    R[6] = gsu_ldw(FX_VIEW_MATRIX_8);
    R[0] = gsu_fmult(R[3], R[6]);
    R[0] = gsu_rol(R[0]);
    R[0] = gsu_add(R[0], R[5], 0);
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_stw(R[9], R[0]);
    R[0] = gsu_sub(R[0], R[8], 0);
    R[0] = gsu_sub(R[0], R[8], 0);
    R[9] = gsu_add(R[9], 6, 0);
    gsu_stw(R[9], R[0]);
    R[9] = gsu_sub(R[9], 4, 0);
    R[9] = gsu_add(R[9], 6, 0);
    t = gsu_count_down(&R[12]);
    if (t) goto l8902;
    gsu_stw(FX_POINT_OUT, R[9]);
    nx = (uint16_t)(gsu_getb());
    gsu_r14(gsu_inc(R[14]));
    at = model_command(nx);
    goto dispatch;
l8982:
    R[9] = gsu_ldw(gsu_code_then(0x018982, 4, gsu_code_then(0x018A9B, 8, FX_POINT_OUT)));
    gsu_code(0x018986, 3);
    gsu_stw(FX_MODEL_STACK, R[10]);
    R[11] = gsu_ldw(gsu_code_then(0x018989, 3, FX_VIEW_MATRIX_3));
    R[13] = gsu_ldw(gsu_code_then(0x01898C, 3, FX_VIEW_MATRIX_6));
    R[4] = gsu_ldw(gsu_code_then(0x01898F, 3, FX_VIEW_MATRIX_1));
    R[7] = gsu_ldw(gsu_code_then(0x018992, 3, FX_VIEW_MATRIX_4));
    R[8] = gsu_ldw(gsu_code_then(0x018995, 3, FX_VIEW_MATRIX_7));
    R[10] = gsu_ldw(gsu_code_then(0x018998, 3, FX_VIEW_MATRIX_2));
    R[12] = (gsu_code(0x01899B, 2), gsu_getb());
    gsu_code(0x01899D, 1);
    gsu_r14(gsu_inc(R[14]));
    R[0] = gsu_ldw(gsu_code_then(0x01899E, 3, FX_POINTS_TOTAL));
    R[0] = gsu_add(R[0], R[12], 0);
    gsu_code(0x0189A1, 2);
    gsu_sbk(R[0]);
    /* CACHE */
l89A3:
    R[0] = (gsu_code(0x0189A3, 1), gsu_cache(0x89A3), gsu_code(0x0189A4, 5), gsu_getb());
    gsu_r14(gsu_inc(R[14]));
    R[5] = gsu_ldw(FX_POINT_SCALE);
    R[1] = gsu_mult(R[0], R[5]);
    R[0] = (gsu_code(0x0189A9, 3), gsu_getb());
    gsu_code(0x0189AC, 1);
    gsu_r14(gsu_inc(R[14]));
    R[6] = gsu_ldw(gsu_code_then(0x0189AD, 3, FX_VIEW_MATRIX_0));
    R[2] = gsu_mult(R[0], R[5]);
    R[0] = (gsu_code(0x0189B0, 3), gsu_getb());
    gsu_code(0x0189B3, 1);
    gsu_r14(gsu_inc(R[14]));
    R[3] = gsu_mult(R[0], R[5]);
    R[0] = gsu_fmult(R[1], R[6]);
    R[5] = gsu_rol(R[0]);
    R[6] = R[11];
    R[0] = gsu_fmult(R[2], R[6]);
    R[0] = gsu_rol(R[0]);
    R[5] = gsu_add(R[5], R[0], 0);
    R[6] = R[13];
    R[0] = gsu_fmult(R[3], R[6]);
    R[0] = gsu_rol(R[0]);
    R[0] = gsu_add(R[0], R[5], 0);
    gsu_code(0x0189B4, 20);
    gsu_stw(R[9], R[0]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    R[6] = R[4];
    R[0] = gsu_fmult(R[1], R[6]);
    R[5] = gsu_rol(R[0]);
    R[6] = R[7];
    R[0] = gsu_fmult(R[2], R[6]);
    R[0] = gsu_rol(R[0]);
    R[5] = gsu_add(R[5], R[0], 0);
    R[6] = R[8];
    R[0] = gsu_fmult(R[3], R[6]);
    R[0] = gsu_rol(R[0]);
    R[0] = gsu_add(R[0], R[5], 0);
    gsu_code(0x0189C8, 22);
    gsu_stw(R[9], R[0]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    R[6] = R[10];
    R[0] = gsu_fmult(R[1], R[6]);
    R[5] = gsu_rol(R[0]);
    R[6] = gsu_ldw(gsu_code_then(0x0189DE, 11, FX_VIEW_MATRIX_5));
    R[0] = gsu_fmult(R[2], R[6]);
    R[0] = gsu_rol(R[0]);
    R[5] = gsu_add(R[5], R[0], 0);
    R[6] = gsu_ldw(gsu_code_then(0x0189E9, 8, FX_VIEW_MATRIX_8));
    R[0] = gsu_fmult(R[3], R[6]);
    R[0] = gsu_rol(R[0]);
    R[0] = gsu_add(R[0], R[5], 0);
    gsu_code(0x0189F1, 5);
    gsu_stw(R[9], R[0]);
    R[9] = gsu_inc(R[9]);
    R[12] = gsu_dec(R[12]);
    t = !gsu.z;
    R[9] = gsu_inc(R[9]);
    if (t) { gsu_code(0x0189F6, 4); goto l89A3; } else { gsu_code(0x0189F6, 8); }
    R[10] = gsu_ldw(FX_MODEL_STACK);
    gsu_code(0x0189FE, 3);
    gsu_stw(FX_POINT_OUT, R[9]);
    nx = (uint16_t)((gsu_code(0x018A01, 2), gsu_getb()));
    gsu_r14(gsu_inc(R[14]));
    at = model_command(nx);
    goto dispatch;
l8A03:
    R[9] = gsu_ldw(gsu_code_then(0x018A03, 4, gsu_code_then(0x018B12, 8, FX_POINT_OUT)));
    gsu_code(0x018A07, 3);
    gsu_stw(FX_MODEL_STACK, R[10]);
    R[11] = gsu_ldw(gsu_code_then(0x018A0A, 3, FX_VIEW_MATRIX_3));
    R[13] = gsu_ldw(gsu_code_then(0x018A0D, 3, FX_VIEW_MATRIX_6));
    R[4] = gsu_ldw(gsu_code_then(0x018A10, 3, FX_VIEW_MATRIX_1));
    R[7] = gsu_ldw(gsu_code_then(0x018A13, 3, FX_VIEW_MATRIX_4));
    R[10] = gsu_ldw(gsu_code_then(0x018A16, 3, FX_VIEW_MATRIX_2));
    R[12] = (gsu_code(0x018A19, 2), gsu_getb());
    gsu_code(0x018A1B, 1);
    gsu_r14(gsu_inc(R[14]));
    R[0] = gsu_ldw(gsu_code_then(0x018A1C, 3, FX_POINTS_TOTAL));
    R[0] = gsu_add(R[0], R[12], 0);
    R[0] = gsu_add(R[0], R[12], 0);
    gsu_code(0x018A1F, 3);
    gsu_sbk(R[0]);
    /* CACHE */
l8A22:
    R[0] = (gsu_code(0x018A22, 1), gsu_cache(0x8A22), gsu_code(0x018A23, 5), gsu_getb());
    gsu_r14(gsu_inc(R[14]));
    R[5] = gsu_ldw(FX_POINT_SCALE);
    R[1] = gsu_mult(R[0], R[5]);
    R[0] = (gsu_code(0x018A28, 3), gsu_getb());
    gsu_code(0x018A2B, 1);
    gsu_r14(gsu_inc(R[14]));
    R[6] = gsu_ldw(gsu_code_then(0x018A2C, 3, FX_VIEW_MATRIX_0));
    R[2] = gsu_mult(R[0], R[5]);
    R[0] = (gsu_code(0x018A2F, 3), gsu_getb());
    gsu_code(0x018A32, 1);
    gsu_r14(gsu_inc(R[14]));
    R[3] = gsu_mult(R[0], R[5]);
    R[0] = gsu_fmult(R[1], R[6]);
    R[8] = gsu_rol(R[0]);
    R[6] = R[11];
    R[0] = gsu_fmult(R[2], R[6]);
    R[5] = gsu_rol(R[0]);
    R[6] = R[13];
    R[0] = gsu_fmult(R[3], R[6]);
    R[0] = gsu_rol(R[0]);
    R[0] = gsu_add(R[0], R[5], 0);
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x018A33, 20);
    gsu_stw(R[9], R[0]);
    R[0] = gsu_sub(R[0], R[8], 0);
    R[0] = gsu_sub(R[0], R[8], 0);
    R[9] = gsu_add(R[9], 6, 0);
    gsu_code(0x018A47, 6);
    gsu_stw(R[9], R[0]);
    R[9] = gsu_sub(R[9], 4, 0);
    R[6] = R[4];
    R[0] = gsu_fmult(R[1], R[6]);
    R[8] = gsu_rol(R[0]);
    R[6] = R[7];
    R[0] = gsu_fmult(R[2], R[6]);
    R[5] = gsu_rol(R[0]);
    R[6] = gsu_ldw(gsu_code_then(0x018A4D, 18, FX_VIEW_MATRIX_7));
    R[0] = gsu_fmult(R[3], R[6]);
    R[0] = gsu_rol(R[0]);
    R[0] = gsu_add(R[0], R[5], 0);
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x018A5F, 6);
    gsu_stw(R[9], R[0]);
    R[0] = gsu_sub(R[0], R[8], 0);
    R[0] = gsu_sub(R[0], R[8], 0);
    R[9] = gsu_add(R[9], 6, 0);
    gsu_code(0x018A65, 6);
    gsu_stw(R[9], R[0]);
    R[9] = gsu_sub(R[9], 4, 0);
    R[6] = R[10];
    R[0] = gsu_fmult(R[1], R[6]);
    R[8] = gsu_rol(R[0]);
    R[6] = gsu_ldw(gsu_code_then(0x018A6B, 12, FX_VIEW_MATRIX_5));
    R[0] = gsu_fmult(R[2], R[6]);
    R[5] = gsu_rol(R[0]);
    R[6] = gsu_ldw(gsu_code_then(0x018A77, 7, FX_VIEW_MATRIX_8));
    R[0] = gsu_fmult(R[3], R[6]);
    R[0] = gsu_rol(R[0]);
    R[0] = gsu_add(R[0], R[5], 0);
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x018A7E, 6);
    gsu_stw(R[9], R[0]);
    R[0] = gsu_sub(R[0], R[8], 0);
    R[0] = gsu_sub(R[0], R[8], 0);
    R[9] = gsu_add(R[9], 6, 0);
    gsu_code(0x018A84, 6);
    gsu_stw(R[9], R[0]);
    R[9] = gsu_inc(R[9]);
    R[12] = gsu_dec(R[12]);
    t = !gsu.z;
    R[9] = gsu_inc(R[9]);
    if (t) { gsu_code(0x018A8A, 4); goto l8A22; } else { gsu_code(0x018A8A, 8); }
    R[10] = gsu_ldw(FX_MODEL_STACK);
    gsu_code(0x018A92, 3);
    gsu_stw(FX_POINT_OUT, R[9]);
    nx = (uint16_t)((gsu_code(0x018A95, 2), gsu_getb()));
    gsu_r14(gsu_inc(R[14]));
    at = model_command(nx);
    goto dispatch;
    /* FxModelPoints01 */
l8A97:
    R[0] = gsu_ldw(gsu_code_then(0x010005, 3, gsu_code_then(0x018A97, 4, FX_POINT_SHIFT)));
    R[0] = gsu_sub(R[0], 3, 0);
    t = gsu.s;
    if (t) { gsu_code(0x018A9B, 4); gsu_code(0x018AA3, 5); goto l8AA3; }
    goto l8982;
l8AA3:
    R[11] = gsu_ldw(FX_POINT_OUT);
    R[5] = gsu_ldw(gsu_code_then(0x018AA8, 3, FX_MATRIX_PACKED_0));
    R[6] = gsu_ldw(gsu_code_then(0x018AAB, 3, FX_MATRIX_PACKED_1));
    R[7] = gsu_ldw(gsu_code_then(0x018AAE, 3, FX_MATRIX_PACKED_2));
    R[8] = gsu_ldw(gsu_code_then(0x018AB1, 3, FX_MATRIX_PACKED_3));
    R[9] = gsu_ldw(gsu_code_then(0x018AB4, 3, FX_MATRIX_PACKED_4));
    R[1] = gsu_ldw(gsu_code_then(0x018AB7, 3, FX_POINT_SCALE));
    R[0] = gsu_lob(R[9]);
    R[0] = gsu_swap(R[0]);
    R[9] = gsu_or(R[0], R[1]);
    R[12] = (gsu_code(0x018ABA, 7), gsu_getb());
    gsu_code(0x018AC1, 1);
    gsu_r14(gsu_inc(R[14]));
    R[0] = gsu_ldw(gsu_code_then(0x018AC2, 3, FX_POINTS_TOTAL));
    R[0] = gsu_add(R[0], R[12], 0);
    gsu_code(0x018AC5, 2);
    gsu_sbk(R[0]);
    /* CACHE */
    R[13] = 0x8AC9;
l8AC9:
    R[1] = (gsu_code(0x018AC9, 25), gsu_getb());
    gsu_r14(gsu_inc(R[14]));
    R[4] = gsu_mult(R[5], R[1]);
    R[2] = gsu_getb();
    gsu_r14(gsu_inc(R[14]));
    R[0] = gsu_hib(R[6]);
    R[0] = gsu_mult(R[0], R[2]);
    R[4] = gsu_add(R[0], R[4], 0);
    R[3] = gsu_getb();
    gsu_r14(gsu_inc(R[14]));
    R[0] = gsu_mult(R[8], R[3]);
    R[0] = gsu_add(R[0], R[4], 0);
    R[0] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_hib(R[0]);
    R[0] = gsu_mult(R[0], R[9]);
    gsu_stw(R[11], R[0]);
    R[11] = gsu_inc(R[11]);
    R[11] = gsu_inc(R[11]);
    R[0] = gsu_hib(R[5]);
    R[4] = gsu_mult(R[0], R[1]);
    R[0] = gsu_mult(R[7], R[2]);
    R[4] = gsu_add(R[0], R[4], 0);
    R[0] = gsu_hib(R[8]);
    R[0] = gsu_mult(R[0], R[3]);
    R[0] = gsu_add(R[0], R[4], 0);
    R[0] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_hib(R[0]);
    R[0] = gsu_mult(R[0], R[9]);
    gsu_code(0x018AE2, 18);
    gsu_stw(R[11], R[0]);
    R[11] = gsu_inc(R[11]);
    R[11] = gsu_inc(R[11]);
    R[4] = gsu_mult(R[6], R[1]);
    R[0] = gsu_hib(R[7]);
    R[0] = gsu_mult(R[0], R[2]);
    R[4] = gsu_add(R[0], R[4], 0);
    R[0] = gsu_hib(R[9]);
    R[0] = gsu_mult(R[0], R[3]);
    R[0] = gsu_add(R[0], R[4], 0);
    R[0] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_hib(R[0]);
    R[0] = gsu_mult(R[0], R[9]);
    gsu_code(0x018AF4, 18);
    gsu_stw(R[11], R[0]);
    R[11] = gsu_inc(R[11]);
    t = gsu_count_down(&R[12]);
    R[11] = gsu_inc(R[11]);
    if (t) { gsu_code(0x018B06, 2); goto l8AC9; } else { gsu_code(0x018B06, 6); }
    gsu_stw(FX_POINT_OUT, R[11]);
    nx = (uint16_t)((gsu_code(0x018B0C, 2), gsu_getb()));
    gsu_r14(gsu_inc(R[14]));
    at = model_command(nx);
    goto dispatch;
    /* FxModelPointsMirrored */
l8B0E:
    R[0] = gsu_ldw(gsu_code_then(0x010038, 4, gsu_code_then(0x018B0E, 4, FX_POINT_SHIFT)));
    R[0] = gsu_sub(R[0], 3, 0);
    t = gsu.s;
    if (t) { gsu_code(0x018B12, 4); gsu_code(0x018B1A, 5); goto l8B1A; }
    goto l8A03;
l8B1A:
    R[11] = gsu_ldw(FX_POINT_OUT);
    R[5] = gsu_ldw(gsu_code_then(0x018B1F, 3, FX_MATRIX_PACKED_0));
    R[6] = gsu_ldw(gsu_code_then(0x018B22, 3, FX_MATRIX_PACKED_1));
    R[7] = gsu_ldw(gsu_code_then(0x018B25, 3, FX_MATRIX_PACKED_2));
    R[8] = gsu_ldw(gsu_code_then(0x018B28, 3, FX_MATRIX_PACKED_3));
    R[9] = gsu_ldw(gsu_code_then(0x018B2B, 3, FX_MATRIX_PACKED_4));
    R[1] = gsu_ldw(gsu_code_then(0x018B2E, 3, FX_POINT_SCALE));
    R[0] = gsu_lob(R[9]);
    R[0] = gsu_swap(R[0]);
    R[9] = gsu_or(R[0], R[1]);
    R[12] = (gsu_code(0x018B31, 7), gsu_getb());
    gsu_code(0x018B38, 1);
    gsu_r14(gsu_inc(R[14]));
    R[0] = gsu_ldw(gsu_code_then(0x018B39, 3, FX_POINTS_TOTAL));
    R[0] = gsu_add(R[0], R[12], 0);
    R[0] = gsu_add(R[0], R[12], 0);
    gsu_code(0x018B3C, 3);
    gsu_sbk(R[0]);
    /* CACHE */
l8B3F:
    R[1] = (gsu_code(0x018B3F, 1), gsu_cache(0x8B3F), gsu_code(0x018B40, 26), gsu_getb());
    gsu_r14(gsu_inc(R[14]));
    R[13] = gsu_mult(R[5], R[1]);
    R[2] = gsu_getb();
    gsu_r14(gsu_inc(R[14]));
    R[0] = gsu_hib(R[6]);
    R[4] = gsu_mult(R[0], R[2]);
    R[3] = gsu_getb();
    gsu_r14(gsu_inc(R[14]));
    R[0] = gsu_mult(R[8], R[3]);
    R[4] = gsu_add(R[0], R[4], 0);
    R[0] = gsu_add(R[4], R[13], 0);
    R[0] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_hib(R[0]);
    R[0] = gsu_mult(R[0], R[9]);
    gsu_stw(R[11], R[0]);
    R[11] = gsu_add(R[11], 6, 0);
    R[0] = gsu_sub(R[4], R[13], 0);
    R[0] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_hib(R[0]);
    R[0] = gsu_mult(R[0], R[9]);
    gsu_code(0x018B5A, 9);
    gsu_stw(R[11], R[0]);
    R[11] = gsu_sub(R[11], 4, 0);
    R[0] = gsu_hib(R[5]);
    R[13] = gsu_mult(R[0], R[1]);
    R[4] = gsu_mult(R[7], R[2]);
    R[0] = gsu_hib(R[8]);
    R[0] = gsu_mult(R[0], R[3]);
    R[4] = gsu_add(R[0], R[4], 0);
    R[0] = gsu_add(R[4], R[13], 0);
    R[0] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_hib(R[0]);
    R[0] = gsu_mult(R[0], R[9]);
    gsu_code(0x018B63, 21);
    gsu_stw(R[11], R[0]);
    R[11] = gsu_add(R[11], 6, 0);
    R[0] = gsu_sub(R[4], R[13], 0);
    R[0] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_hib(R[0]);
    R[0] = gsu_mult(R[0], R[9]);
    gsu_code(0x018B78, 9);
    gsu_stw(R[11], R[0]);
    R[11] = gsu_sub(R[11], 4, 0);
    R[13] = gsu_mult(R[6], R[1]);
    R[0] = gsu_hib(R[7]);
    R[4] = gsu_mult(R[0], R[2]);
    R[0] = gsu_hib(R[9]);
    R[0] = gsu_mult(R[0], R[3]);
    R[4] = gsu_add(R[0], R[4], 0);
    R[0] = gsu_add(R[4], R[13], 0);
    R[0] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_hib(R[0]);
    R[0] = gsu_mult(R[0], R[9]);
    gsu_code(0x018B81, 21);
    gsu_stw(R[11], R[0]);
    R[11] = gsu_add(R[11], 6, 0);
    R[0] = gsu_sub(R[4], R[13], 0);
    R[0] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_hib(R[0]);
    R[0] = gsu_mult(R[0], R[9]);
    gsu_code(0x018B96, 9);
    gsu_stw(R[11], R[0]);
    R[11] = gsu_inc(R[11]);
    R[11] = gsu_inc(R[11]);
    R[12] = gsu_dec(R[12]);
    t = !gsu.z;
    if (t) { gsu_code(0x018B9F, 5); goto l8B3F; } else { gsu_code(0x018B9F, 9); }
    gsu_stw(FX_POINT_OUT, R[11]);
    nx = (uint16_t)((gsu_code(0x018BA8, 2), gsu_getb()));
    gsu_r14(gsu_inc(R[14]));
    at = model_command(nx);
    goto dispatch;
    /* FxModelProject */
l8BAA:
    gsu_code(0x01000C, 4); gsu_code(0x018BAA, 4); gsu_code(0x018C89, 4);
    goto l8C89;
l8BAE:
    R[12] = gsu_ldw(FX_POINTS_TOTAL);
    R[0] = 0x0014;
    gsu_romb(R[0]);
    R[7] = 0x0340;
    gsu_stw(FX_ROM_R14, R[14]);
    R[8] = 0x04E4;
    R[0] = gsu_sub(R[0], R[0], 0);
l8BBF:
    gsu_code(0x018BBF, 4); gsu_code(0x018C86, 3);
    gsu_stw(FX_OUTCODES, R[0]);
    R[9] = gsu_ldw(gsu_code_then(0x018BC3, 3, FX_OBJECT_Z));
    R[3] = gsu_ldw(gsu_code_then(0x018BC6, 3, FX_OBJECT_X));
    R[13] = gsu_ldw(gsu_code_then(0x018BC9, 3, FX_OBJECT_Y));
    /* CACHE */
l8BCC:
    R[0] = gsu_ldw(gsu_code_then(0x018BCD, 1, R[7]));
    R[7] = gsu_inc(R[7]);
    R[7] = gsu_inc(R[7]);
    R[1] = gsu_add(R[0], R[3], 0);
    R[0] = gsu_ldw(gsu_code_then(0x018BCE, 5, R[7]));
    R[7] = gsu_inc(R[7]);
    R[7] = gsu_inc(R[7]);
    R[2] = gsu_add(R[0], R[13], 0);
    R[0] = gsu_ldw(gsu_code_then(0x018BD3, 5, R[7]));
    R[7] = gsu_inc(R[7]);
    R[7] = gsu_inc(R[7]);
    R[6] = gsu_add(R[0], R[9], 0);
    R[11] = 0x8BE0;
    (gsu_code(0x018BD8, 8), abi_FxProjectVertex());
l8BE0:
    gsu_code(0x018BE0, 3);
    gsu_stw(R[8], R[1]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    gsu_code(0x018BE3, 4);
    gsu_stw(R[8], R[2]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    gsu_code(0x018BE7, 3);
    gsu_stw(R[8], R[0]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    R[6] = gsu_ldw(gsu_code_then(0x018BEA, 5, FX_OUTCODES));
    R[0] = gsu_or(R[0], R[6]);
    R[12] = gsu_dec(R[12]);
    t = !gsu.z;
    gsu_sbk(R[0]);
    if (t) goto l8BCC; else { gsu_code(0x018BF4, 3); }
    R[0] = gsu_ldw(FX_ROM_BANK);
    gsu_code(0x018BF7, 5);
    gsu_romb(R[0]);
    gsu_r14(gsu_ldw(FX_ROM_R14));
    goto l8C79;
l8BFF:
    R[0] = 0x4400;
    R[0] = gsu_sub(R[0], R[6], 0);
    t = !gsu.s;
    if (t) goto l8C0E;
    R[1] = gsu_asr(R[1]);
    R[2] = gsu_asr(R[2]);
    t = 1;
    R[6] = gsu_asr(R[6]);
    if (t) goto l8BFF;
l8C0E:
    R[0] = 0x0100;
    R[0] = gsu_sub(R[0], R[6], 0);
    t = !gsu.s;
    if (t) goto l8C5C;
    R[6] = gsu_bic(R[6], 1);
    R[0] = 0x896C;
    gsu_r14(gsu_add(R[0], R[6], 0));
    R[6] = gsu_getb();
    gsu_r14(gsu_inc(R[14]));
    R[6] = gsu_getbh(R[6]);
    R[0] = gsu_fmult(R[1], R[6]);
    R[1] = gsu_ldw(FX_MODEL_34);
    R[0] = gsu_add(R[0], R[1], 0);
    R[1] = R[0];
    R[0] = gsu_fmult(R[2], R[6]);
    R[2] = gsu_ldw(FX_MODEL_36);
    R[0] = gsu_add(R[0], R[2], 0);
    R[2] = R[0];
    R[6] = 0x0000;
    R[0] = R[1];
    gsu_moves(R[1]);
    t = gsu.s;
    if (t) goto l8C42;
    R[5] = gsu_ldw(FX_SCREEN_RIGHT);
    R[0] = gsu_sub(R[0], R[5], 0);
    t = gsu.s;
    if (t) goto l8C46;
    R[6] = gsu_inc(R[6]);
l8C42:
    R[6] = gsu_inc(R[6]);
    R[6] = gsu_mult(R[6], 4);
l8C46:
    R[0] = R[2];
    gsu_moves(R[2]);
    t = gsu.s;
    if (t) goto l8C53;
    R[5] = gsu_ldw(FX_SCREEN_BOTTOM);
    R[0] = gsu_sub(R[0], R[5], 0);
    t = gsu.s;
    if (t) goto l8C54;
    R[6] = gsu_inc(R[6]);
l8C53:
    R[6] = gsu_inc(R[6]);
l8C54:
    R[0] = 0x001F;
    R[0] = gsu_xor(R[0], R[6]);
    R[0] = gsu_swap(R[0]);
    R[0] = gsu_or(R[0], R[6]);
    nx = (uint16_t)(R[11]);
    at = nx;
    goto dispatch;
l8C5C:
    gsu_r14(R[12]);
    R[3] = R[6];
    gsu_stw(R[10], R[11]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[11] = 0x8C69;
    abi_FxProjectPoint();
l8C69:
    R[12] = R[14];
    R[3] = gsu_ldw(FX_OBJECT_X);
    R[13] = gsu_ldw(FX_OBJECT_Y);
    R[9] = gsu_ldw(FX_OBJECT_Z);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
    /* FxModelProjectRest */
l8C79:
    gsu_code(0x018BFC, 3); gsu_code(0x018C79, 4); gsu_code(0x018D23, 1);
    goto l8D23;
l8C7D:
    R[7] = gsu_sub(R[7], 4, 0);
    R[0] = R[10];
    R[10] = gsu_ldw(gsu_code_then(0x018C7D, 9, gsu_code_then(0x018CB6, 11, FX_MODEL_STACK)));
    goto l8BBF;
l8C89:
    R[11] = gsu_ldw(FX_SCREEN_RIGHT);
    R[13] = gsu_ldw(gsu_code_then(0x018C8D, 3, FX_SCREEN_BOTTOM));
    R[1] = gsu_ldw(gsu_code_then(0x018C90, 3, FX_MODEL_34));
    R[9] = gsu_ldw(gsu_code_then(0x018C93, 3, FX_MODEL_36));
    R[3] = gsu_ldw(gsu_code_then(0x018C96, 3, FX_OBJECT_X));
    R[4] = gsu_ldw(gsu_code_then(0x018C99, 3, FX_OBJECT_Y));
    R[5] = gsu_ldw(gsu_code_then(0x018C9C, 3, FX_OBJECT_Z));
    R[12] = gsu_ldw(gsu_code_then(0x018C9F, 3, FX_POINTS_TOTAL));
    gsu_code(0x018CA2, 3);
    gsu_stw(FX_MODEL_STACK, R[10]);
    R[10] = 0x0000;
    R[0] = 0x0014;
    gsu_code(0x018CA5, 9);
    gsu_romb(R[0]);
    gsu_stw(FX_ROM_R14, R[14]);
    R[7] = 0x0344;
    R[8] = 0x04E4;
    /* CACHE */
l8CB4:
    R[0] = gsu_ldw(R[7]);
    R[0] = gsu_add(R[0], R[5], 0);
    R[6] = 0x0100;
    R[0] = gsu_sub(R[0], R[6], 0);
    t = !gsu.s;
    R[0] = gsu_add(R[0], R[6], 0);
    if (t) { gsu_code(0x018CB6, 7); gsu_code(0x018CC1, 7); gsu_code(0x018CCA, 8); goto l8CC1; }
    goto l8C7D;
l8CC1:
    R[6] = 0x43FE;
    R[0] = gsu_sub(R[0], R[6], 0);
    t = gsu.s;
    R[0] = gsu_add(R[0], R[6], 0);
    if (t) goto l8CCA; else { gsu_code(0x018CC8, 2); }
    R[0] = R[6];
l8CCA:
    R[0] = gsu_bic(R[0], 1);
    R[6] = 0x896C;
    gsu_r14(gsu_add(R[0], R[6], 0));
    R[7] = gsu_dec(R[7]);
    R[7] = gsu_dec(R[7]);
    R[0] = gsu_ldw(gsu_code_then(0x018CD2, 3, R[7]));
    R[2] = gsu_add(R[0], R[4], 0);
    R[6] = (gsu_code(0x018CD5, 4), gsu_getb());
    gsu_code(0x018CD9, 1);
    gsu_r14(gsu_inc(R[14]));
    R[7] = gsu_dec(R[7]);
    R[7] = gsu_dec(R[7]);
    R[0] = gsu_ldw(gsu_code_then(0x018CDA, 3, R[7]));
    R[0] = gsu_add(R[0], R[3], 0);
    R[6] = gsu_getbh(gsu_code_then(0x018CDD, 4, R[6]));
    R[0] = gsu_fmult(R[0], R[6]);
    R[0] = gsu_add(R[0], R[1], 0);
    gsu_code(0x018CE1, 3);
    gsu_stw(R[8], R[0]);
    gsu_code(0x018CE4, 2);
    gsu_r14(0x0000);
    t = gsu.s;
    R[0] = gsu_sub(R[0], R[11], 0);
    if (t) goto l8CEC; else { gsu_code(0x018CE8, 3); }
    t = gsu.s;
    if (t) goto l8CF0; else { gsu_code(0x018CEB, 1); }
    gsu_r14(gsu_inc(R[14]));
l8CEC:
    gsu_code(0x018CEC, 4);
    gsu_r14(gsu_inc(R[14]));
    gsu_r14(gsu_mult(R[14], 4));
l8CF0:
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    R[0] = gsu_fmult(R[2], R[6]);
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_code(0x018CE6, 2); gsu_code(0x018CF0, 7);
    gsu_stw(R[8], R[0]);
    t = gsu.s;
    R[0] = gsu_sub(R[0], R[13], 0);
    if (t) { gsu_code(0x018CF7, 2); gsu_code(0x018CFE, 1); goto l8CFD; } else { gsu_code(0x018CF7, 5); gsu_code(0x018CFE, 1); }
    t = gsu.s;
    if (t) goto l8CFE; else { gsu_code(0x018CFC, 1); }
    gsu_r14(gsu_inc(R[14]));
l8CFD:
    gsu_code(0x018CFD, 1);
    gsu_r14(gsu_inc(R[14]));
l8CFE:
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    R[0] = 0x001F;
    R[0] = gsu_xor(R[0], R[14]);
    R[0] = gsu_swap(R[0]);
    R[0] = gsu_or(R[0], R[14]);
    gsu_code(0x018CFF, 9);
    gsu_stw(R[8], R[0]);
    R[10] = gsu_or(R[0], R[10]);
    R[8] = gsu_inc(R[8]);
    R[7] = gsu_add(R[7], 10, 0);
    R[12] = gsu_dec(R[12]);
    t = !gsu.z;
    R[8] = gsu_inc(R[8]);
    if (t) { gsu_code(0x018D08, 9); goto l8CB4; } else { gsu_code(0x018D08, 17); }
    goto l8D15;
l8D15:
    gsu_stw(FX_OUTCODES, R[10]);
    R[10] = gsu_ldw(gsu_code_then(0x018D19, 3, FX_MODEL_STACK));
    R[0] = gsu_ldw(gsu_code_then(0x018D1C, 3, FX_ROM_BANK));
    gsu_code(0x018D1F, 5);
    gsu_romb(R[0]);
    gsu_r14(gsu_ldw(FX_ROM_R14));
l8D23:
    R[0] = gsu_ldw(gsu_code_then(0x018D24, 3, FX_OUTCODES));
    R[1] = 0x1F00;
    R[0] = gsu_and(R[0], R[1]);
    gsu_sub(R[0], R[1], 0);   /* CMP */
    t = gsu.z;
    if (t) { gsu_code(0x018D27, 8); gsu_code(0x018D33, 4); goto l8D33; } else { gsu_code(0x018D27, 12); gsu_code(0x01924D, 3); }
    goto l924D;
l8D33:
    R[1] = gsu_ldw(FX_OUTCODES);
    R[0] = 0x001F;
    R[0] = gsu_and(R[0], R[1]);
    t = !gsu.z;
    if (t) { gsu_code(0x018D37, 5); gsu_code(0x018D44, 1); goto l8D44; } else { gsu_code(0x018D37, 9); }
    R[0] = gsu_ldw(FX_MODEL_FLAGS);
    R[1] = 0x4000;
    R[0] = gsu_or(R[0], R[1]);
    gsu_code(0x018D40, 5);
    gsu_sbk(R[0]);
l8D44:
    gsu_code(0x018D45, 3);
    gsu_r14(gsu_ldw(FX_MODEL_18));
    gsu_code(0x018D48, 3);
    gsu_stw(FX_ROM_R14, R[14]);
    nx = (uint16_t)((gsu_code(0x018D4B, 2), gsu_getb()));
    gsu_code(0x010030, 4); gsu_code(0x0193A7, 4);
    gsu_r14(gsu_inc(R[14]));
    at = model_command(nx);
    goto dispatch;
    /* FxModelListStart */
l8D4D:
    R[1] = 0x0788;
    gsu_code(0x010058, 4); gsu_code(0x018D4D, 7);
    gsu_stw(FX_MODEL_RETURN_AT, R[1]);
    R[0] = 0x8E7D;
    gsu_code(0x018D54, 4);
    gsu_stw(R[10], R[0]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    /* CACHE */
    nx = (uint16_t)((gsu_code(0x018D58, 3), gsu_cache(0x8D5A), gsu_code(0x018D5B, 2), gsu_getb()));
    gsu_r14(gsu_inc(R[14]));
    at = model_command(nx);
    goto dispatch;
    /* FxModelCall */
l8D5D:
    gsu_code(0x010061, 3); gsu_code(0x018D5D, 2);
    gsu_r14(gsu_dec(R[14]));
    gsu_code(0x018D5F, 1);
    gsu_r14(gsu_inc(R[14]));
    R[0] = R[14];
    R[0] = gsu_inc(R[0]);
    R[0] = gsu_inc(R[0]);
    R[1] = gsu_ldw(gsu_code_then(0x018D60, 7, FX_MODEL_RETURN_AT));
    gsu_code(0x018D67, 1);
    gsu_stw(R[1], R[0]);
    R[1] = gsu_inc(R[1]);
    R[1] = gsu_inc(R[1]);
    gsu_code(0x018D68, 5);
    gsu_stw(FX_MODEL_RETURN_AT, R[1]);
    R[0] = (gsu_code(0x018D6D, 1), gsu_getb());
    gsu_code(0x018D6E, 1);
    gsu_r14(gsu_inc(R[14]));
    R[0] = gsu_getbh(gsu_code_then(0x018D6F, 2, R[0]));
    gsu_code(0x018D71, 4);
    gsu_r14(gsu_add(R[0], R[14], 0));
    nx = (uint16_t)(gsu_getb());
    gsu_r14(gsu_inc(R[14]));
    at = model_command(nx);
    goto dispatch;
    /* FxModelReturnPull */
l8D75:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x010064, 4); gsu_code(0x018D75, 5);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
    /* FxModelIfCall */
l8D7A:
    R[0] = (gsu_code(0x01005C, 4), gsu_code(0x018D7A, 9), gsu_getb());
    gsu_r14(gsu_inc(R[14]));
    R[1] = 0x07C6;
    R[0] = gsu_add(R[0], R[1], 0);
    R[0] = gsu_ldb(R[0]);
    R[0] = gsu_swap(R[0]);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.s;
    if (t) { gsu_code(0x018D83, 5); gsu_code(0x018DBD, 3); goto l8DBD; } else { gsu_code(0x018D83, 8); }
    gsu_stw(R[10], R[14]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[0] = (gsu_code(0x018D8B, 3), gsu_getb());
    gsu_code(0x018D8E, 1);
    gsu_r14(gsu_inc(R[14]));
    R[0] = gsu_getbh(gsu_code_then(0x018D8F, 8, R[0]));
    gsu_r14(gsu_add(R[0], R[14], 0));
    R[0] = gsu_add(0x8D95, 5, 0);
    gsu_stw(R[10], R[0]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    nx = (uint16_t)((gsu_code(0x018D97, 4), gsu_getb()));
    gsu_r14(gsu_inc(R[14]));
    at = model_command(nx);
    goto dispatch;
l8D9A:
    gsu_code(0x018D9A, 2);
    gsu_r14(gsu_inc(R[14]));
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x018D9C, 4);
    gsu_r14(gsu_ldw(R[10]));
    gsu_code(0x018DA0, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x018DA1, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x018DA2, 2);
    gsu_stw(R[10], R[14]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[0] = (gsu_code(0x018DA4, 3), gsu_getb());
    gsu_code(0x018DA7, 1);
    gsu_r14(gsu_inc(R[14]));
    R[0] = gsu_getbh(gsu_code_then(0x018DA8, 8, R[0]));
    gsu_r14(gsu_add(R[0], R[14], 0));
    R[0] = gsu_add(0x8DAE, 5, 0);
    gsu_stw(R[10], R[0]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    nx = (uint16_t)((gsu_code(0x018DB0, 4), gsu_getb()));
    gsu_r14(gsu_inc(R[14]));
    at = model_command(nx);
    goto dispatch;
l8DB3:
    gsu_code(0x018DB3, 2);
    gsu_r14(gsu_inc(R[14]));
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x018DB5, 4);
    gsu_r14(gsu_ldw(R[10]));
    gsu_code(0x018DB9, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x018DBA, 1);
    gsu_r14(gsu_inc(R[14]));
    nx = (uint16_t)((gsu_code(0x018DBB, 2), gsu_getb()));
    gsu_code(0x010060, 1);
    gsu_r14(gsu_inc(R[14]));
    at = model_command(nx);
    goto dispatch;
l8DBD:
    gsu_stw(R[10], R[14]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_code(0x018DC0, 3);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x018DC3, 1);
    gsu_r14(gsu_inc(R[14]));
    R[0] = (gsu_code(0x018DC4, 10), gsu_getb());
    gsu_r14(gsu_inc(R[14]));
    R[0] = gsu_getbh(R[0]);
    gsu_r14(gsu_add(R[0], R[14], 0));
    R[0] = gsu_add(0x8DCC, 5, 0);
    gsu_stw(R[10], R[0]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    nx = (uint16_t)((gsu_code(0x018DCE, 4), gsu_getb()));
    gsu_r14(gsu_inc(R[14]));
    at = model_command(nx);
    goto dispatch;
l8DD1:
    gsu_code(0x018DD1, 2);
    gsu_r14(gsu_inc(R[14]));
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x018DD3, 4);
    gsu_r14(gsu_ldw(R[10]));
    gsu_code(0x018DD7, 1);
    gsu_r14(gsu_dec(R[14]));
    gsu_code(0x018DD8, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x018DD9, 2);
    gsu_stw(R[10], R[14]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[0] = (gsu_code(0x018DDB, 3), gsu_getb());
    gsu_code(0x018DDE, 1);
    gsu_r14(gsu_inc(R[14]));
    R[0] = gsu_getbh(gsu_code_then(0x018DDF, 8, R[0]));
    gsu_r14(gsu_add(R[0], R[14], 0));
    R[0] = gsu_add(0x8DE5, 5, 0);
    gsu_stw(R[10], R[0]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    nx = (uint16_t)((gsu_code(0x018DE7, 4), gsu_getb()));
    gsu_r14(gsu_inc(R[14]));
    at = model_command(nx);
    goto dispatch;
l8DEA:
    gsu_code(0x018DEA, 2);
    gsu_r14(gsu_inc(R[14]));
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x018DEC, 4);
    gsu_r14(gsu_ldw(R[10]));
    gsu_code(0x018DF0, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x018DF1, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x018DF2, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x018DF3, 1);
    gsu_r14(gsu_inc(R[14]));
    nx = (uint16_t)((gsu_code(0x018DF4, 2), gsu_getb()));
    gsu_code(0x010060, 1);
    gsu_r14(gsu_inc(R[14]));
    at = model_command(nx);
    goto dispatch;
    /* FxModelListStart3C */
l8DF6:
    R[1] = 0x0788;
    gsu_code(0x01003C, 4); gsu_code(0x018DF6, 7);
    gsu_stw(FX_MODEL_RETURN_AT, R[1]);
    R[0] = 0x8E7D;
    gsu_code(0x018DFD, 4);
    gsu_stw(R[10], R[0]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    /* CACHE */
    nx = (uint16_t)((gsu_code(0x010028, 4), gsu_code(0x018E01, 3), gsu_cache(0x8E03), gsu_code(0x018E04, 2), gsu_code(0x018E19, 9), gsu_getb()));
    gsu_r14(gsu_inc(R[14]));
    at = model_command(nx);
    goto dispatch;
    /* FxModelPushReturn */
l8E06:
    R[0] = (gsu_code(0x010045, 3), gsu_code(0x018E06, 2), gsu_getb());
    gsu_code(0x018E08, 1);
    gsu_r14(gsu_inc(R[14]));
    R[0] = gsu_getbh(gsu_code_then(0x018E09, 2, R[0]));
    R[0] = gsu_add(R[0], R[14], 0);
    R[1] = gsu_ldw(gsu_code_then(0x018E0B, 4, FX_MODEL_RETURN_AT));
    gsu_code(0x018E0F, 1);
    gsu_stw(R[1], R[0]);
    R[1] = gsu_inc(R[1]);
    R[1] = gsu_inc(R[1]);
    gsu_code(0x018E10, 5);
    gsu_stw(FX_MODEL_RETURN_AT, R[1]);
    /* FxModelReturn40 */
l8E14:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x018E15, 4);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
    /* FxModelIfSkip */
l8E19:
    R[0] = gsu_getb();
    gsu_r14(gsu_inc(R[14]));
    R[1] = 0x07C6;
    R[0] = gsu_add(R[0], R[1], 0);
    R[0] = gsu_ldb(R[0]);
    R[0] = gsu_swap(R[0]);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.s;
    if (t) { gsu_code(0x018E22, 5); gsu_code(0x018E45, 2); goto l8E45; } else { gsu_code(0x018E22, 7); }
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x018E29, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x018E2A, 2);
    gsu_stw(R[10], R[14]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[0] = (gsu_code(0x018E2C, 3), gsu_getb());
    R[0] = gsu_lob(R[0]);
    t = gsu.z;
    if (t) goto l8E3D; else { gsu_code(0x018E2F, 10); }
    gsu_r14(gsu_add(R[0], R[14], 0));
    R[0] = gsu_add(0x8E37, 5, 0);
    gsu_stw(R[10], R[0]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    nx = (uint16_t)((gsu_code(0x018E39, 4), gsu_getb()));
    gsu_code(0x010044, 1);
    gsu_r14(gsu_inc(R[14]));
    at = model_command(nx);
    goto dispatch;
l8E3C:
    gsu_code(0x018E3C, 2);
    gsu_r14(gsu_inc(R[14]));
gsu_code(0x018E3E, 4);
l8E3D:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_r14(gsu_ldw(R[10]));
    gsu_code(0x018E42, 1);
    gsu_r14(gsu_inc(R[14]));
    nx = (uint16_t)((gsu_code(0x018E43, 2), gsu_getb()));
    gsu_code(0x010044, 1);
    gsu_r14(gsu_inc(R[14]));
    at = model_command(nx);
    goto dispatch;
l8E45:
    R[0] = gsu_getb();
    gsu_code(0x018E47, 1);
    gsu_r14(gsu_inc(R[14]));
    R[0] = gsu_getbh(gsu_code_then(0x018E48, 2, R[0]));
    R[12] = gsu_add(R[0], R[14], 0);
    gsu_code(0x018E4A, 3);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x018E4D, 2);
    gsu_stw(R[10], R[14]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_code(0x018E4F, 5);
    gsu_r14(gsu_inc(R[14]));
    gsu_stw(R[10], R[12]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[0] = gsu_add(0x8E58, 5, 0);
    gsu_code(0x018E54, 6);
    gsu_stw(R[10], R[0]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    nx = (uint16_t)((gsu_code(0x018E5A, 4), gsu_getb()));
    gsu_code(0x010044, 1);
    gsu_r14(gsu_inc(R[14]));
    at = model_command(nx);
    goto dispatch;
l8E5D:
    gsu_code(0x018E5D, 2);
    gsu_r14(gsu_inc(R[14]));
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    R[0] = gsu_ldw(gsu_code_then(0x018E5F, 3, R[10]));
    R[1] = gsu_ldw(gsu_code_then(0x018E62, 3, FX_MODEL_RETURN_AT));
    gsu_code(0x018E65, 1);
    gsu_stw(R[1], R[0]);
    R[1] = gsu_inc(R[1]);
    R[1] = gsu_inc(R[1]);
    gsu_code(0x018E66, 5);
    gsu_stw(FX_MODEL_RETURN_AT, R[1]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x018E6B, 4);
    gsu_r14(gsu_ldw(R[10]));
    R[0] = (gsu_code(0x018E6F, 1), gsu_getb());
    R[0] = gsu_lob(R[0]);
    t = gsu.z;
    if (t) goto l8E78; else { gsu_code(0x018E70, 6); }
    gsu_r14(gsu_add(R[0], R[14], 0));
    nx = (uint16_t)((gsu_code(0x018E76, 2), gsu_getb()));
    gsu_code(0x010044, 1);
    gsu_r14(gsu_inc(R[14]));
    at = model_command(nx);
    goto dispatch;
l8E78:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
l8E7D:
    R[0] = gsu_sub(R[0], R[0], 0);
    R[1] = gsu_ldw(gsu_code_then(0x018E7D, 5, FX_MODEL_RETURN_AT));
    gsu_code(0x018E82, 1);
    gsu_stw(R[1], R[0]);
    R[0] = 0x0788;
    gsu_code(0x018E83, 6);
    gsu_stw(FX_MODEL_RETURN_AT, R[0]);
l8E88:
    R[1] = gsu_ldw(gsu_code_then(0x018E89, 3, FX_MODEL_RETURN_AT));
    gsu_code(0x018E8C, 2);
    gsu_r14(gsu_ldw(R[1]));
    R[1] = gsu_inc(R[1]);
    R[1] = gsu_inc(R[1]);
    gsu_code(0x018E8E, 5);
    gsu_stw(FX_MODEL_RETURN_AT, R[1]);
    gsu_code(0x018E93, 2);
    gsu_r14(R[14]);
    gsu_moves(R[14]);
    t = gsu.z;
    if (t) { gsu_code(0x018E95, 2); gsu_code(0x018EA3, 4); goto l8EA3; } else { gsu_code(0x018E95, 7); }
    R[0] = gsu_add(0x8E9A, 5, 0);
    gsu_stw(R[10], R[0]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    nx = (uint16_t)((gsu_code(0x018E9C, 4), gsu_getb()));
    gsu_r14(gsu_inc(R[14]));
    at = model_command(nx);
    goto dispatch;
l8E9F:
    gsu_code(0x018E9F, 2);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x018E88, 1); gsu_code(0x018EA1, 2);
    t = 1;
    if (t) goto l8E88;
l8EA3:
    goto l9243;
    /* FxModelDepthSort */
l8EA7:
    R[12] = gsu_getb();
    gsu_r14(gsu_inc(R[14]));
    R[0] = 0xF850;
    R[0] = gsu_add(R[0], R[12], 0);
    R[0] = gsu_add(R[0], R[14], 0);
    gsu_stw(FX_MODEL_150, R[0]);
    gsu_stw(FX_MODEL_14E, R[12]);
    gsu_stw(FX_MODEL_148, R[12]);
    R[9] = 0x0344;
    R[5] = 0x07B0;
    R[13] = 0x8EC0;
l8EC0:
    R[0] = gsu_getb();
    gsu_r14(gsu_inc(R[14]));
    R[0] = gsu_umult(R[0], 6);
    R[0] = gsu_add(R[0], R[9], 0);
    R[0] = gsu_ldw(R[0]);
    gsu_stw(R[5], R[0]);
    R[5] = gsu_inc(R[5]);
    t = gsu_count_down(&R[12]);
    R[5] = gsu_inc(R[5]);
    if (t) goto l8EC0;
l8ECA:
    R[5] = 0x07B0;
    R[7] = 0x8000;
    R[12] = gsu_ldw(FX_MODEL_148);
    R[13] = 0x8ED5;
l8ED5:
    R[0] = gsu_ldw(R[5]);
    gsu_sub(R[0], R[7], 0);   /* CMP */
    t = gsu.s != gsu.ov;
    if (t) goto l8EDF;
    R[7] = R[0];
    R[6] = R[5];
l8EDF:
    R[5] = gsu_inc(R[5]);
    t = gsu_count_down(&R[12]);
    R[5] = gsu_inc(R[5]);
    if (t) goto l8ED5;
    R[0] = 0x8000;
    gsu_stw(R[6], R[0]);
    R[0] = gsu_ldw(FX_MODEL_150);
    gsu_r14(gsu_add(R[0], R[6], 0));
    R[0] = gsu_getb();
    gsu_r14(gsu_inc(R[14]));
    R[0] = gsu_getbh(R[0]);
    gsu_r14(R[0]);
    R[0] = gsu_add(0x8EF4, 5, 0);
    gsu_stw(R[10], R[0]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    nx = (uint16_t)(gsu_getb());
    gsu_r14(gsu_inc(R[14]));
    at = model_command(nx);
    goto dispatch;
l8EF9:
    gsu_r14(gsu_inc(R[14]));
    R[0] = gsu_ldw(FX_MODEL_14E);
    R[0] = gsu_dec(R[0]);
    t = !gsu.z;
    gsu_sbk(R[0]);
    if (t) goto l8ECA;
    goto l9243;
    /* FxModelIfSprite */
l8F05:
    R[0] = gsu_getb();
    gsu_r14(gsu_inc(R[14]));
    R[1] = 0x07C6;
    R[0] = gsu_add(R[0], R[1], 0);
    R[0] = gsu_ldb(R[0]);
    R[0] = gsu_swap(R[0]);
    R[0] = gsu_hib(R[0]);
    t = gsu.s;
    if (t) goto l8F18;
    gsu_r14(gsu_inc(R[14]));
    gsu_r14(gsu_inc(R[14]));
    gsu_r14(gsu_inc(R[14]));
    nx = (uint16_t)(gsu_getb());
    gsu_r14(gsu_inc(R[14]));
    at = model_command(nx);
    goto dispatch;
    /* FxModelSprite */
l8F18:
    R[0] = gsu_getb();
    gsu_r14(gsu_inc(R[14]));
    R[3] = gsu_umult(R[0], 6);
    R[1] = 0x04E4;
    R[2] = gsu_add(R[3], R[1], 0);
    R[0] = gsu_ldw(R[2]);
    R[2] = gsu_inc(R[2]);
    R[2] = gsu_inc(R[2]);
    gsu_stw(FX_MODEL_BA, R[0]);
    R[0] = gsu_ldw(R[2]);
    gsu_stw(FX_MODEL_BC, R[0]);
    R[1] = 0x04E4;
    R[1] = 0x0344;
    R[0] = gsu_add(R[3], R[1], 0);
    R[0] = gsu_ldw(R[0]);
    R[1] = gsu_ldw(FX_OBJECT_Z);
    R[0] = gsu_add(R[0], R[1], 0);
    gsu_stw(FX_MODEL_BE, R[0]);
    R[1] = 0x0020;
    R[0] = gsu_sub(R[0], R[1], 0);
    t = gsu.s;
    if (t) goto l8F78;
    R[11] = 0x8F49;
    goto l9250;
l8F49:
    gsu_r14(gsu_inc(R[14]));
    R[0] = gsu_getb();
    gsu_r14(gsu_inc(R[14]));
    R[0] = gsu_ldw(FX_TEX_MASK);
    R[0] = gsu_lob(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_stw(FX_MODEL_C8, R[0]);
    R[1] = 0x0100;
    R[0] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_swap(R[0]);
    t = !gsu.s;
    if (t) goto l8F5E;
    R[1] = gsu_lsr(R[1]);
l8F5E:
    gsu_stw(FX_MODEL_CE, R[1]);
    gsu_stw(FX_ROM_R14, R[14]);
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_stw(FX_MODEL_D2, R[0]);
    R[11] = 0x8F6D;
    goto lAB11;
l8F6D:
    R[0] = gsu_ldw(FX_ROM_BANK);
    gsu_romb(R[0]);
    gsu_r14(gsu_ldw(FX_ROM_R14));
    nx = (uint16_t)(gsu_getb());
    gsu_r14(gsu_inc(R[14]));
    at = model_command(nx);
    goto dispatch;
l8F78:
    gsu_r14(gsu_inc(R[14]));
    gsu_r14(gsu_inc(R[14]));
    nx = (uint16_t)(gsu_getb());
    gsu_r14(gsu_inc(R[14]));
    at = model_command(nx);
    goto dispatch;
    /* FxModelMaterial */
l8F7D:
    R[2] = R[14];
    R[0] = gsu_ldw(gsu_code_then(0x018F7D, 6, gsu_code_then(0x01910E, 4, FX_MODEL_FLAGS)));
    R[1] = 0x2000;
    R[0] = gsu_and(R[0], R[1]);
    t = gsu.z;
    if (t) { gsu_code(0x018F83, 6); gsu_code(0x018F98, 2); goto l8F98; }
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_stw(FX_FACE_KIND, R[0]);
    R[0] = 0x0002;
    gsu_cmode(R[0]);
    R[0] = 0x0009;
    goto l90B6;
l8F98:
    R[0] = gsu_getb();
    R[0] = gsu_add(R[0], R[0], 0);
    R[1] = 0x0003;
    gsu_code(0x018F9A, 9);
    gsu_romb(R[1]);
    R[1] = gsu_ldw(FX_MATERIAL_TABLE);
    gsu_code(0x018FA3, 2);
    gsu_r14(gsu_add(R[0], R[1], 0));
l8FA4:
    R[3] = (gsu_code(0x018FA5, 2), gsu_getb());
    gsu_code(0x018FA7, 1);
    gsu_r14(gsu_inc(R[14]));
    R[3] = gsu_getbh(gsu_code_then(0x018FA8, 3, R[3]));
    R[3] = R[3];
    gsu_moves(R[3]);
    t = !gsu.s;
    if (t) { gsu_code(0x018FAB, 4); gsu_code(0x018FE1, 8); goto l8FE1; } else { gsu_code(0x018FAB, 9); gsu_code(0x018FCF, 6); }
    R[0] = gsu_add(R[3], R[3], 0);
    t = !gsu.s;
    if (t) goto l8FCF;
    R[0] = 0xFFFE;
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
    gsu_stw(FX_FACE_COLOUR, R[0]);
    R[0] = 0xFFFE;
    goto l90B6;
l8FCF:
    R[0] = 0x1FFF;
    gsu_r14(gsu_and(R[0], R[3]));
    R[0] = gsu_ldw(gsu_code_then(0x018FD5, 3, FX_MATERIAL_MASK));
    R[3] = (gsu_code(0x018FD8, 2), gsu_getb());
    R[3] = gsu_dec(R[3]);
    R[0] = gsu_and(R[0], R[3]);
    R[0] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_inc(R[0]);
    t = 1;
    gsu_code(0x018FA4, 1); gsu_code(0x018FDA, 7);
    gsu_r14(gsu_add(R[0], R[14], 0));
    if (t) goto l8FA4;
l8FE1:
    R[0] = gsu_lob(R[3]);
    R[1] = 0x9FB1;
    gsu_r14(gsu_add(R[0], R[1], 0));
    R[1] = (gsu_code(0x018FE9, 2), gsu_getb());
    R[0] = 0x0800;
    R[0] = gsu_add(R[0], R[1], 0);
    gsu_code(0x018FEB, 7);
    gsu_stw(FX_FACE_COLOUR, R[0]);
    R[0] = gsu_add(R[3], R[3], 0);
    t = !gsu.s;
    R[0] = gsu_sub(R[0], R[0], 0);
    if (t) goto l8FF7; else { gsu_code(0x018FF6, 1); }
    R[0] = gsu_dec(R[0]);
l8FF7:
    gsu_code(0x018FF2, 4); gsu_code(0x018FF7, 4);
    gsu_stw(FX_FACE_KIND, R[0]);
    gsu_code(0x018FFB, 2);
    t = gsu.z;
    if (t) { gsu_code(0x019045, 14); goto l9045; } else { gsu_code(0x018FFD, 7); gsu_code(0x01900B, 1); }
    R[0] = 0x2000;
    R[0] = gsu_and(R[0], R[3]);
    t = gsu.z;
    if (t) goto l900B; else { gsu_code(0x019004, 7); }
    R[0] = 0x0C00;
    R[0] = gsu_add(R[0], R[1], 0);
    gsu_stw(FX_FACE_COLOUR, R[0]);
l900B:
    R[0] = gsu_lob(R[3]);
    R[0] = gsu_umult(R[0], 3);
    R[1] = 0x9D59;
    gsu_code(0x01900C, 17);
    gsu_r14(gsu_add(R[0], R[1], 0));
    R[0] = gsu_getb();
    gsu_r14(gsu_inc(R[14]));
    R[0] = gsu_getbh(R[0]);
    gsu_r14(gsu_inc(R[14]));
    gsu_stw(FX_TEX_BASE, R[0]);
    R[0] = (gsu_code(0x01901D, 4), gsu_getb());
    gsu_stw(FX_TEX_BANK, R[0]);
    R[0] = gsu_hib(R[3]);
    R[1] = 0x001F;
    R[0] = gsu_and(R[0], R[1]);
    R[0] = gsu_add(R[0], R[0], 0);
    R[1] = 0xA079;
    gsu_code(0x019021, 26);
    gsu_r14(gsu_add(R[0], R[1], 0));
    R[0] = gsu_getb();
    gsu_r14(gsu_inc(R[14]));
    R[0] = gsu_getbh(R[0]);
    gsu_r14(R[0]);
    R[1] = gsu_getb();
    gsu_r14(gsu_inc(R[14]));
    R[1] = gsu_getbh(R[1]);
    gsu_stw(FX_TEX_MASK, R[1]);
    R[0] = gsu_inc(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_code(0x01903B, 5);
    gsu_stw(FX_UV_AT, R[0]);
    R[0] = 0xFFFF;
    goto l90B6;
l9045:
    R[1] = 0x003F;
    R[0] = gsu_hib(R[3]);
    R[0] = gsu_inc(R[0]);
    R[0] = gsu_and(R[0], R[1]);
    t = gsu.z;
    if (t) goto l90B3;
    R[0] = gsu_inc(R[0]);
    R[0] = gsu_and(R[0], R[1]);
    t = !gsu.z;
    if (t) { gsu_code(0x01905E, 9); goto l905E; } else { gsu_code(0x019053, 6); }
    R[0] = gsu_lob(R[3]);
    R[3] = gsu_ldw(FX_SHADE_AT);
    gsu_code(0x019059, 2);
    gsu_r14(gsu_add(R[0], R[3], 0));
    R[0] = (gsu_code(0x01905B, 1), gsu_getb());
    t = 1;
    gsu_color((uint8_t)R[0]);
    if (t) { gsu_code(0x01905C, 2); goto l90B6; }
l905E:
    R[0] = gsu_dec(R[0]);
    R[0] = gsu_dec(R[0]);
    R[0] = gsu_and(R[0], R[1]);
    R[3] = R[0];
    R[0] = gsu_ldw(FX_SHADE_TABLE);
    R[0] = gsu_add(R[0], R[3], 0);
    gsu_code(0x019067, 3);
    gsu_r14(gsu_add(R[0], R[3], 0));
    R[6] = (gsu_code(0x01906A, 2), gsu_getb());
    gsu_code(0x01906C, 1);
    gsu_r14(gsu_inc(R[14]));
    R[6] = gsu_getbh(gsu_code_then(0x01906D, 3, R[6]));
    R[0] = gsu_ldw(gsu_code_then(0x019070, 3, FX_ROM_BANK));
    gsu_code(0x019073, 6);
    gsu_romb(R[0]);
    gsu_code(0x019079, 1);
    gsu_r14(R[2]);
    gsu_code(0x01907A, 3);
    gsu_r14(gsu_inc(R[14]));
    R[0] = gsu_getb();
    gsu_r14(gsu_inc(R[14]));
    R[4] = gsu_ldw(FX_LIGHT_LOCAL_X);
    R[5] = gsu_mult(R[0], R[4]);
    R[0] = (gsu_code(0x01907D, 3), gsu_getb());
    gsu_code(0x019080, 1);
    gsu_r14(gsu_inc(R[14]));
    R[4] = gsu_ldw(gsu_code_then(0x019081, 3, FX_LIGHT_LOCAL_Y));
    R[0] = gsu_mult(R[0], R[4]);
    R[5] = gsu_add(R[0], R[5], 0);
    R[0] = (gsu_code(0x019084, 4), gsu_getb());
    R[4] = gsu_ldw(gsu_code_then(0x019088, 3, FX_LIGHT_LOCAL_Z));
    R[0] = gsu_mult(R[0], R[4]);
    R[0] = gsu_add(R[0], R[5], 0);
    R[0] = gsu_asr(R[0]);
    R[0] = gsu_asr(R[0]);
    R[0] = gsu_hib(R[0]);
    R[0] = gsu_sex(R[0]);
    R[4] = 0x0006;
    gsu_sub(R[0], R[4], 0);   /* CMP */
    t = gsu.s == gsu.ov;
    if (t) { gsu_code(0x01909C, 7); goto l909C; } else { gsu_code(0x019097, 5); }
    R[0] = 0x0006;
    t = 1;
    if (t) goto l90A5;
l909C:
    R[4] = 0x0010;
    gsu_sub(R[0], R[4], 0);   /* CMP */
    t = gsu.s != gsu.ov;
    if (t) goto l90A5;
    R[0] = 0x000F;
l90A5:
    R[1] = gsu_sub(R[0], 6, 0);
    R[0] = 0x0003;
    gsu_code(0x01908B, 12); gsu_code(0x0190A5, 12);
    gsu_romb(R[0]);
    gsu_code(0x0190B1, 2);
    gsu_r14(gsu_add(R[6], R[1], 0));
    R[0] = gsu_getb();
    t = 1;
    gsu_color((uint8_t)R[0]);
    if (t) goto l90B6;
l90B3:
    R[0] = gsu_lob(R[3]);
    gsu_color((uint8_t)R[0]);
l90B6:
    R[1] = gsu_ldw(gsu_code_then(0x0190B6, 4, FX_ROM_BANK));
    gsu_code(0x0190BA, 5);
    gsu_romb(R[1]);
    gsu_r14(R[2]);
    nx = (uint16_t)(R[11]);
    at = nx;
    goto dispatch;
l90C0:
    R[0] = gsu_inc(R[0]);
    R[0] = gsu_lob(R[0]);
    t = gsu.z;
    if (t) { gsu_code(0x0190C8, 5); goto l90C8; } else { gsu_code(0x0190C5, 3); }
    nx = (uint16_t)(gsu_getb());
    gsu_code(0x010000, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x010001, 3);
    at = model_command(nx);
    goto dispatch;
l90C8:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
    /* FxModelFaces */
l90CD:
    /* CACHE */
    R[0] = (gsu_code(0x0190CE, 1), gsu_cache(0x90CE), gsu_code(0x0190CF, 1), gsu_getb());
    R[0] = gsu_lob(R[0]);
    t = gsu.s;
    gsu_code(0x0190CD, 1); gsu_code(0x0190D0, 3);
    gsu_r14(gsu_inc(R[14]));
    if (t) { gsu_code(0x0190C1, 4); goto l90C0; } else { gsu_code(0x0190D4, 1); }
    gsu_r14(gsu_inc(R[14]));
    R[2] = R[0];
    R[0] = gsu_sub(R[0], 2, 0);
    t = gsu.z;
    R[0] = (gsu_code(0x0190D3, 1), gsu_code(0x0190D5, 6), gsu_getb());
    if (t) { gsu_code(0x0190F0, 1); goto l90EF; } else { gsu_code(0x0190DC, 6); }
    R[1] = 0x07C6;
    R[0] = gsu_add(R[0], R[1], 0);
    R[0] = gsu_ldb(R[0]);
    R[0] = gsu_lob(R[0]);
    t = gsu.s;
    gsu_r14(gsu_inc(R[14]));
    if (t) goto l90F0; else { gsu_code(0x0190CD, 1); gsu_code(0x0190E6, 9); }
    t = gsu.z;
    if (t) goto l90F0;
    R[2] = gsu_add(R[2], 4, 0);
    t = 1;
    gsu_r14(gsu_add(R[14], R[2], 0));
    if (t) goto l90CD;
l90EF:
    gsu_r14(gsu_inc(R[14]));
l90F0:
    gsu_code(0x0190F1, 3);
    gsu_stw(FX_POINT_COUNT, R[2]);
    R[0] = 0x09AA;
    R[6] = gsu_ldw(gsu_code_then(0x0190F4, 6, FX_LIST_END));
    R[0] = gsu_sub(R[0], R[6], 0);
    t = !gsu.s;
    if (t) { gsu_code(0x0190FA, 3); gsu_code(0x01910D, 1); goto l910D; } else { gsu_code(0x0190FA, 6); }
    gsu_stw(R[10], R[14]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[11] = 0x9106;
    (gsu_code(0x019100, 6), abi_FxDrawList());
l9106:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x019106, 5);
    gsu_r14(gsu_ldw(R[10]));
    R[6] = gsu_ldw(gsu_code_then(0x01910B, 3, FX_LIST_END));
l910D:
    R[11] = 0x9112;
    goto l8F7D;
l9112:
    R[6] = gsu_ldw(gsu_code_then(0x0190BF, 1, gsu_code_then(0x019112, 4, FX_LIST_END)));
    R[3] = R[0];
    gsu_code(0x019116, 5);
    gsu_r14(gsu_add(R[14], 4, 0));
    R[12] = gsu_ldw(gsu_code_then(0x01911B, 3, FX_POINT_COUNT));
    R[0] = gsu_lob(R[3]);
    R[0] = gsu_swap(R[0]);
    R[0] = gsu_or(R[0], R[12]);
    R[3] = R[3];
    gsu_moves(R[3]);
    t = !gsu.s;
    if (t) { gsu_code(0x01911E, 8); gsu_code(0x019142, 1); goto l9142; } else { gsu_code(0x01911E, 16); }
    R[0] = gsu_not(R[12]);
    R[1] = gsu_lob(R[0]);
    R[0] = gsu_ldw(FX_TEX_BANK);
    R[0] = gsu_lob(R[0]);
    R[0] = gsu_swap(R[0]);
    R[0] = gsu_or(R[0], R[1]);
    gsu_code(0x01912E, 4);
    gsu_stw(R[6], R[0]);
    R[6] = gsu_inc(R[6]);
    R[6] = gsu_inc(R[6]);
    R[0] = gsu_ldw(gsu_code_then(0x019132, 5, FX_FACE_COLOUR));
    gsu_code(0x019137, 1);
    gsu_stw(R[6], R[0]);
    R[6] = gsu_inc(R[6]);
    R[6] = gsu_inc(R[6]);
    R[0] = gsu_ldw(gsu_code_then(0x019138, 5, FX_TEX_BASE));
    gsu_code(0x01913D, 1);
    gsu_stw(R[6], R[0]);
    R[6] = gsu_inc(R[6]);
    R[6] = gsu_inc(R[6]);
    R[0] = gsu_ldw(gsu_code_then(0x01913E, 5, FX_TEX_MASK));
l9142:
    gsu_code(0x019143, 1);
    gsu_stw(R[6], R[0]);
    R[6] = gsu_inc(R[6]);
    R[6] = gsu_inc(R[6]);
    R[7] = R[6];
    R[9] = 0x04E4;
    R[5] = 0x0000;
    R[13] = 0x914E;
l914E:
    R[4] = (gsu_code(0x01914E, 3), gsu_getb());
    gsu_code(0x019151, 1);
    gsu_r14(gsu_inc(R[14]));
    R[0] = gsu_umult(R[4], 6);
    R[8] = gsu_add(R[0], R[9], 0);
    R[0] = gsu_ldw(gsu_code_then(0x019152, 6, R[8]));
    gsu_code(0x019158, 1);
    gsu_stw(R[6], R[0]);
    R[6] = gsu_inc(R[6]);
    R[6] = gsu_inc(R[6]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    R[0] = gsu_ldw(gsu_code_then(0x019159, 5, R[8]));
    gsu_code(0x01915E, 1);
    gsu_stw(R[6], R[0]);
    R[6] = gsu_inc(R[6]);
    R[6] = gsu_inc(R[6]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    R[0] = gsu_ldw(gsu_code_then(0x01915F, 5, R[8]));
    R[5] = gsu_or(R[0], R[5]);
    R[3] = R[3];
    gsu_moves(R[3]);
    t = !gsu.s;
    if (t) { gsu_code(0x019164, 6); gsu_code(0x01918B, 1); goto l918B; } else { gsu_code(0x019164, 10); }
    gsu_stw(FX_ROM_R14, R[14]);
    R[0] = 0x0003;
    gsu_code(0x01916E, 7);
    gsu_romb(R[0]);
    gsu_r14(gsu_ldw(FX_UV_AT));
    R[0] = (gsu_code(0x019175, 1), gsu_getb());
    gsu_code(0x019176, 1);
    gsu_r14(gsu_inc(R[14]));
    R[0] = gsu_swap(R[0]);
    gsu_code(0x019177, 2);
    gsu_stw(R[6], R[0]);
    R[6] = gsu_inc(R[6]);
    R[6] = gsu_inc(R[6]);
    R[0] = (gsu_code(0x019179, 3), gsu_getb());
    gsu_code(0x01917C, 1);
    gsu_r14(gsu_inc(R[14]));
    R[0] = gsu_swap(R[0]);
    gsu_code(0x01917D, 2);
    gsu_stw(R[6], R[0]);
    R[6] = gsu_inc(R[6]);
    R[6] = gsu_inc(R[6]);
    R[0] = gsu_ldw(gsu_code_then(0x01917F, 5, FX_ROM_BANK));
    gsu_code(0x019184, 3);
    gsu_stw(FX_UV_AT, R[14]);
    gsu_code(0x019187, 5);
    gsu_romb(R[0]);
    gsu_r14(gsu_ldw(FX_ROM_R14));
l918B:
    gsu_code(0x01918C, 1);
    t = gsu_count_down(&R[12]);
    if (t) goto l914E; else { gsu_code(0x01918D, 5); }
    R[0] = gsu_lob(R[5]);
    t = !gsu.z;
    if (t) { gsu_code(0x019199, 9); goto l9199; } else { gsu_code(0x019192, 4); }
    gsu_stw(FX_LIST_END, R[6]);
    goto l90CD;
l9199:
    R[1] = 0x1F00;
    R[0] = gsu_and(R[1], R[5]);
    R[0] = gsu_sub(R[0], R[1], 0);
    t = gsu.z;
    if (t) { gsu_code(0x0191A6, 5); goto l91A6; } else { gsu_code(0x0191A2, 4); gsu_code(0x01923F, 1); }
    goto l923F;
l91A6:
    gsu_stw(FX_OUTCODES, R[5]);
    gsu_code(0x0191AB, 3);
    gsu_stw(FX_ROM_R14, R[14]);
    gsu_code(0x0191AE, 3);
    gsu_stw(FX_POLY_OUT, R[7]);
    R[0] = gsu_lob(R[5]);
    t = gsu.z;
    if (t) goto l91CA; else { gsu_code(0x0191B1, 8); }
    R[0] = gsu_ldw(FX_FACE_KIND);
    R[0] = gsu_hib(R[0]);
    t = gsu.z;
    if (t) { gsu_code(0x0191B9, 3); gsu_code(0x0191CA, 6); goto l91CA; } else { gsu_code(0x0191B9, 8); gsu_code(0x01C339, 3); }
    R[11] = 0x91C1;
    abi_FxPolygonDraw();
l91C1:
    R[0] = R[0];
    gsu_code(0x0191C1, 5);
    gsu_moves(R[0]);
    t = !gsu.z;
    if (t) goto l920F; else { gsu_code(0x0191C6, 4); }
    goto l923C;
l91CA:
    R[0] = 0x0010;
    R[0] = gsu_and(R[0], R[5]);
    t = gsu.z;
    if (t) { gsu_code(0x0191EB, 4); goto l91EB; } else { gsu_code(0x0191D0, 4); }
    R[0] = gsu_ldw(FX_POINT_COUNT);
    gsu_code(0x0191D4, 2);
    gsu_r14(gsu_sub(R[14], R[0], 0));
    R[11] = 0x91DA;
    (gsu_code(0x0191D6, 4), gsu_code(0x0198D7, 3), abi_FxPolygonGather());
l91DA:
    R[0] = R[0];
    gsu_code(0x0191DA, 5);
    gsu_moves(R[0]);
    t = gsu.z;
    if (t) goto l923C; else { gsu_code(0x0191DF, 4); }
    R[1] = gsu_ldw(FX_FACE_KIND);
    R[1] = gsu_hib(R[1]);
    t = gsu.z;
    if (t) { gsu_code(0x0191E3, 4); gsu_code(0x0191F5, 1); goto l91F5; }
l91E7:
    goto l9205;
l91EB:
    R[0] = gsu_ldw(FX_FACE_KIND);
    R[0] = gsu_hib(R[0]);
    t = !gsu.z;
    if (t) goto l9202; else { gsu_code(0x0191EF, 7); }
    R[0] = gsu_ldw(FX_POINT_COUNT);
l91F5:
    R[11] = 0x91FA;
    gsu_code(0x0191F6, 4); gsu_code(0x01A25F, 4);
    abi_FxPolygonClipScreen();
l91FA:
    R[0] = R[0];
    gsu_code(0x0191FA, 5);
    gsu_moves(R[0]);
    t = gsu.z;
    if (t) goto l923C; else { gsu_code(0x0191FF, 3); }
    t = 1;
    if (t) goto l920F;
l9202:
    R[0] = gsu_ldw(FX_POINT_COUNT);
l9205:
    R[11] = 0x920A;
    goto l9A52;
l920A:
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.z;
    if (t) goto l923C;
l920F:
    R[3] = gsu_ldw(gsu_code_then(0x01920F, 4, FX_FACE_KIND));
    R[6] = gsu_ldw(gsu_code_then(0x019213, 3, FX_POLY_OUT));
    R[2] = R[0];
    R[3] = R[3];
    gsu_moves(R[3]);
    t = !gsu.s;
    R[0] = gsu_add(R[0], R[0], 0);
    if (t) { gsu_code(0x019216, 6); gsu_code(0x01922C, 6); goto l922C; } else { gsu_code(0x019216, 13); }
    R[0] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_add(R[0], R[6], 0);
    gsu_stw(FX_LIST_END, R[0]);
    R[6] = gsu_sub(R[6], 8, 0);
    R[0] = gsu_not(R[2]);
    gsu_code(0x019223, 7);
    gsu_stb(R[6], (uint8_t)R[0]);
    t = 1;
    if (t) { gsu_code(0x01922A, 2); gsu_code(0x019237, 1); goto l9237; }
l922C:
    R[0] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_add(R[0], R[6], 0);
    gsu_stw(FX_LIST_END, R[0]);
    R[6] = gsu_sub(R[6], 2, 0);
    gsu_code(0x019232, 6);
    gsu_stb(R[6], (uint8_t)R[2]);
l9237:
    R[0] = gsu_ldw(gsu_code_then(0x019238, 3, FX_ROM_BANK));
    gsu_code(0x01923B, 1);
    gsu_romb(R[0]);
l923C:
    gsu_code(0x01923C, 4);
    gsu_r14(gsu_ldw(FX_ROM_R14));
l923F:
    gsu_code(0x0190CD, 1); gsu_code(0x019240, 3);
    goto l90CD;
    /* FxModelEnd */
l9243:
    R[11] = 0x9248;
    gsu_code(0x019243, 5);
    abi_FxDrawList();
l9248:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x019248, 5);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
l924D:
    t = 1;
    if (t) goto l9248;
    /* FxModelSpriteColour */
l9250:
    R[2] = R[14];
    R[0] = gsu_ldw(FX_MODEL_FLAGS);
    R[1] = 0x2000;
    R[1] = gsu_and(R[0], R[1]);
    t = gsu.z;
    if (t) goto l9275;
    R[1] = 0x1000;
    R[0] = gsu_and(R[0], R[1]);
    t = !gsu.z;
    if (t) goto l9275;
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_cmode(R[0]);
    R[0] = 0x0009;
    gsu_color((uint8_t)R[0]);
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_stw(FX_FACE_COLOUR, R[0]);
    gsu_stw(FX_FACE_KIND, R[0]);
    goto l939E;
l9275:
    R[0] = gsu_getb();
    /* FxSpriteMaterial */
l9276:
    R[0] = gsu_add(R[0], R[0], 0);
    R[1] = 0x0003;
    gsu_romb(R[1]);
    R[1] = gsu_ldw(FX_MATERIAL_TABLE);
    gsu_code(0x019280, 2);
    gsu_r14(gsu_add(R[0], R[1], 0));
l9281:
    R[3] = (gsu_code(0x019282, 2), gsu_getb());
    gsu_code(0x019284, 1);
    gsu_r14(gsu_inc(R[14]));
    R[3] = gsu_getbh(gsu_code_then(0x019285, 3, R[3]));
    R[3] = R[3];
    gsu_moves(R[3]);
    t = !gsu.s;
    if (t) { gsu_code(0x019288, 4); gsu_code(0x0192BC, 8); goto l92BC; } else { gsu_code(0x019288, 9); gsu_code(0x0192AA, 6); }
    R[0] = gsu_add(R[3], R[3], 0);
    t = !gsu.s;
    if (t) goto l92AA;
    R[0] = 0xFFFE;
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
    gsu_stw(FX_FACE_COLOUR, R[0]);
    goto l939E;
l92AA:
    R[0] = 0x1FFF;
    gsu_r14(gsu_and(R[0], R[3]));
    R[0] = gsu_ldw(gsu_code_then(0x0192B0, 3, FX_MATERIAL_MASK));
    R[3] = (gsu_code(0x0192B3, 2), gsu_getb());
    R[3] = gsu_dec(R[3]);
    R[0] = gsu_and(R[0], R[3]);
    R[0] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_inc(R[0]);
    t = 1;
    gsu_code(0x019281, 1); gsu_code(0x0192B5, 7);
    gsu_r14(gsu_add(R[0], R[14], 0));
    if (t) goto l9281;
l92BC:
    R[0] = gsu_lob(R[3]);
    R[1] = 0x9FB1;
    gsu_r14(gsu_add(R[0], R[1], 0));
    R[1] = (gsu_code(0x0192C4, 2), gsu_getb());
    R[0] = 0x0800;
    R[0] = gsu_add(R[0], R[1], 0);
    gsu_code(0x0192C6, 7);
    gsu_stw(FX_FACE_COLOUR, R[0]);
    R[0] = gsu_add(R[3], R[3], 0);
    t = !gsu.s;
    R[0] = gsu_sub(R[0], R[0], 0);
    if (t) goto l92D2; else { gsu_code(0x0192CD, 9); }
    R[0] = gsu_dec(R[0]);
l92D2:
    gsu_stw(FX_FACE_KIND, R[0]);
    gsu_code(0x0192D6, 9); gsu_code(0x0192E6, 1);
    t = gsu.z;
    if (t) goto l931E;
    R[0] = 0x2000;
    R[0] = gsu_and(R[0], R[3]);
    t = gsu.z;
    if (t) goto l92E6; else { gsu_code(0x0192DF, 7); }
    R[0] = 0x0C00;
    R[0] = gsu_add(R[0], R[1], 0);
    gsu_stw(FX_FACE_COLOUR, R[0]);
l92E6:
    R[0] = gsu_lob(R[3]);
    R[0] = gsu_umult(R[0], 3);
    R[1] = 0x9D59;
    gsu_code(0x0192E7, 17);
    gsu_r14(gsu_add(R[0], R[1], 0));
    R[0] = gsu_getb();
    gsu_r14(gsu_inc(R[14]));
    R[0] = gsu_getbh(R[0]);
    gsu_r14(gsu_inc(R[14]));
    gsu_stw(FX_TEX_BASE, R[0]);
    R[0] = (gsu_code(0x0192F8, 4), gsu_getb());
    gsu_stw(FX_TEX_BANK, R[0]);
    R[0] = gsu_hib(R[3]);
    R[1] = 0x001F;
    R[0] = gsu_and(R[0], R[1]);
    R[0] = gsu_add(R[0], R[0], 0);
    R[1] = 0xA079;
    gsu_code(0x0192FC, 26);
    gsu_r14(gsu_add(R[0], R[1], 0));
    R[0] = gsu_getb();
    gsu_r14(gsu_inc(R[14]));
    R[0] = gsu_getbh(R[0]);
    gsu_r14(R[0]);
    R[1] = gsu_getb();
    gsu_r14(gsu_inc(R[14]));
    R[1] = gsu_getbh(R[1]);
    gsu_stw(FX_TEX_MASK, R[1]);
    R[0] = gsu_inc(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_code(0x019316, 5);
    gsu_stw(FX_UV_AT, R[0]);
    goto l939E;
l931E:
    R[1] = 0x003F;
    R[0] = gsu_hib(R[3]);
    R[0] = gsu_inc(R[0]);
    R[0] = gsu_and(R[0], R[1]);
    t = !gsu.z;
    if (t) goto l932B;
    goto l939B;
l932B:
    R[0] = gsu_inc(R[0]);
    R[0] = gsu_and(R[0], R[1]);
    t = gsu.z;
    if (t) goto l9334;
    goto l9340;
l9334:
    R[0] = gsu_lob(R[3]);
    R[3] = gsu_ldw(FX_SHADE_AT);
    gsu_r14(gsu_add(R[0], R[3], 0));
    gsu_color(gsu_getb());   /* GETC */
    goto l939E;
l9340:
    R[0] = gsu_dec(R[0]);
    R[0] = gsu_dec(R[0]);
    R[0] = gsu_and(R[0], R[1]);
    R[3] = R[0];
    R[0] = gsu_ldw(FX_MODEL_FLAGS);
    R[0] = gsu_hib(R[0]);
    t = !gsu.s;
    if (t) goto l939B;
    R[0] = gsu_ldw(FX_SHADE_TABLE);
    R[0] = gsu_add(R[0], R[3], 0);
    gsu_r14(gsu_add(R[0], R[3], 0));
    R[6] = gsu_getb();
    gsu_r14(gsu_inc(R[14]));
    R[6] = gsu_getbh(R[6]);
    R[0] = gsu_ldw(FX_ROM_BANK);
    gsu_romb(R[0]);
    gsu_r14(R[2]);
    gsu_r14(gsu_inc(R[14]));
    R[0] = gsu_getb();
    gsu_r14(gsu_inc(R[14]));
    R[4] = gsu_ldw(FX_LIGHT_LOCAL_X);
    R[5] = gsu_mult(R[0], R[4]);
    R[0] = gsu_getb();
    gsu_r14(gsu_inc(R[14]));
    R[4] = gsu_ldw(FX_LIGHT_LOCAL_Y);
    R[0] = gsu_mult(R[0], R[4]);
    R[5] = gsu_add(R[0], R[5], 0);
    R[0] = gsu_getb();
    R[4] = gsu_ldw(FX_LIGHT_LOCAL_Z);
    R[0] = gsu_mult(R[0], R[4]);
    R[0] = gsu_add(R[0], R[5], 0);
    R[0] = gsu_asr(R[0]);
    R[0] = gsu_asr(R[0]);
    R[0] = gsu_hib(R[0]);
    R[0] = gsu_sex(R[0]);
    R[4] = 0x0006;
    gsu_sub(R[0], R[4], 0);   /* CMP */
    t = gsu.s == gsu.ov;
    if (t) goto l9385;
    R[0] = 0x0006;
    t = 1;
    if (t) goto l938E;
l9385:
    R[4] = 0x0010;
    gsu_sub(R[0], R[4], 0);   /* CMP */
    t = gsu.s != gsu.ov;
    if (t) goto l938E;
    R[0] = 0x000F;
l938E:
    R[1] = gsu_sub(R[0], 6, 0);
    R[0] = 0x0003;
    gsu_romb(R[0]);
    gsu_r14(gsu_add(R[6], R[1], 0));
    t = 1;
    gsu_color(gsu_getb());   /* GETC */
    if (t) goto l939E;
l939B:
    R[0] = gsu_lob(R[3]);
    gsu_color((uint8_t)R[0]);
l939E:
    R[0] = gsu_ldw(gsu_code_then(0x01931B, 3, gsu_code_then(0x01939E, 4, FX_ROM_BANK)));
    gsu_code(0x0193A2, 4);
    gsu_romb(R[0]);
    gsu_r14(R[2]);
    nx = (uint16_t)(R[11]);
    at = nx;
    goto dispatch;
    /* FxModelFacing */
l93A7:
    R[0] = gsu_ldw(FX_MODEL_FLAGS);
    R[1] = 0x4000;
    R[0] = gsu_and(R[0], R[1]);
    t = gsu.z;
    if (t) { gsu_code(0x0193AB, 6); gsu_code(0x0193B5, 3); goto l93B5; } else { gsu_code(0x0193AB, 10); gsu_code(0x01941B, 3); }
    goto l941B;
l93B5:
    R[12] = gsu_getb();
    gsu_code(0x0193B8, 1);
    gsu_r14(gsu_inc(R[14]));
    R[9] = 0x04E4;
    R[11] = 0x07C6;
    /* CACHE */
    R[13] = 0x93C1;
l93C1:
    R[0] = (gsu_code(0x0193C1, 2), gsu_getb());
    gsu_code(0x0193C3, 1);
    gsu_r14(gsu_inc(R[14]));
    R[0] = gsu_mult(R[0], 6);
    R[0] = gsu_add(R[0], R[9], 0);
    R[1] = gsu_ldw(gsu_code_then(0x0193C4, 5, R[0]));
    R[0] = gsu_inc(R[0]);
    R[0] = gsu_inc(R[0]);
    R[2] = gsu_ldw(gsu_code_then(0x0193C9, 4, R[0]));
    R[0] = gsu_inc(R[0]);
    R[0] = gsu_inc(R[0]);
    R[5] = gsu_ldb(gsu_code_then(0x0193CD, 5, R[0]));
    R[0] = (gsu_code(0x0193D2, 1), gsu_getb());
    gsu_code(0x0193D3, 1);
    gsu_r14(gsu_inc(R[14]));
    R[0] = gsu_mult(R[0], 6);
    R[8] = gsu_add(R[0], R[9], 0);
    R[0] = gsu_ldw(gsu_code_then(0x0193D4, 5, R[8]));
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    R[3] = gsu_sub(R[0], R[1], 0);
    R[0] = gsu_ldw(gsu_code_then(0x0193D9, 5, R[8]));
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    R[7] = gsu_sub(R[0], R[2], 0);
    R[0] = gsu_ldb(gsu_code_then(0x0193DE, 6, R[8]));
    R[5] = gsu_xor(R[0], R[5]);
    R[0] = (gsu_code(0x0193E4, 4), gsu_getb());
    gsu_code(0x0193E8, 1);
    gsu_r14(gsu_inc(R[14]));
    R[0] = gsu_mult(R[0], 6);
    R[8] = gsu_add(R[0], R[9], 0);
    R[0] = gsu_ldw(gsu_code_then(0x0193E9, 5, R[8]));
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    R[1] = gsu_sub(R[0], R[1], 0);
    R[0] = gsu_ldw(gsu_code_then(0x0193EE, 5, R[8]));
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    R[2] = gsu_sub(R[0], R[2], 0);
    R[0] = gsu_ldb(gsu_code_then(0x0193F3, 6, R[8]));
    R[5] = gsu_xor(R[0], R[5]);
    R[6] = R[1];
    { gsu_code(0x0193F9, 30); uint32_t p = gsu_lmult(R[7], R[6]); R[4] = (uint16_t)p; R[7] = (uint16_t)(p >> 16); }
    R[1] = R[4];
    R[6] = R[3];
    { uint32_t p = gsu_lmult(R[2], R[6]); R[4] = (uint16_t)p; R[2] = (uint16_t)(p >> 16); }
    R[4] = gsu_sub(R[4], R[1], 0);
    R[2] = gsu_sub(R[2], R[7], !gsu.cy);
    R[0] = gsu_add(R[5], R[5], 0);
    R[0] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_swap(R[0]);
    R[0] = gsu_xor(R[0], R[2]);
    R[0] = gsu_hib(R[0]);
    gsu_stb(R[11], (uint8_t)R[0]);
    t = gsu_count_down(&R[12]);
    R[11] = gsu_inc(R[11]);
    if (t) { gsu_code(0x019417, 1); goto l93C1; } else { gsu_code(0x019417, 4); }
    nx = (uint16_t)(gsu_getb());
    gsu_r14(gsu_inc(R[14]));
    at = model_command(nx);
    goto dispatch;
l941B:
    R[12] = gsu_getb();
    gsu_code(0x01941E, 1);
    gsu_r14(gsu_inc(R[14]));
    R[9] = 0x04E4;
    R[11] = 0x07C6;
    /* CACHE */
    R[13] = 0x9427;
l9427:
    R[0] = (gsu_code(0x019427, 2), gsu_getb());
    gsu_code(0x019429, 1);
    gsu_r14(gsu_inc(R[14]));
    R[0] = gsu_umult(R[0], 6);
    R[0] = gsu_add(R[0], R[9], 0);
    R[1] = gsu_ldb(gsu_code_then(0x01942A, 6, R[0]));
    R[0] = gsu_inc(R[0]);
    R[0] = gsu_inc(R[0]);
    R[2] = gsu_ldb(gsu_code_then(0x019430, 5, R[0]));
    R[0] = (gsu_code(0x019435, 1), gsu_getb());
    gsu_code(0x019436, 1);
    gsu_r14(gsu_inc(R[14]));
    R[0] = gsu_umult(R[0], 6);
    R[8] = gsu_add(R[0], R[9], 0);
    R[0] = gsu_ldb(gsu_code_then(0x019437, 6, R[8]));
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    R[3] = gsu_sub(R[0], R[1], 0);
    R[0] = gsu_ldb(gsu_code_then(0x01943D, 6, R[8]));
    R[7] = gsu_sub(R[0], R[2], 0);
    R[0] = (gsu_code(0x019443, 3), gsu_getb());
    gsu_code(0x019446, 1);
    gsu_r14(gsu_inc(R[14]));
    R[0] = gsu_umult(R[0], 6);
    R[8] = gsu_add(R[0], R[9], 0);
    R[0] = gsu_ldb(gsu_code_then(0x019447, 6, R[8]));
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    R[1] = gsu_sub(R[0], R[1], 0);
    R[0] = gsu_ldb(gsu_code_then(0x01944D, 6, R[8]));
    R[2] = gsu_sub(R[0], R[2], 0);
    R[6] = R[1];
    { gsu_code(0x019453, 22); uint32_t p = gsu_lmult(R[7], R[6]); R[4] = (uint16_t)p; R[7] = (uint16_t)(p >> 16); }
    R[1] = R[4];
    R[6] = R[3];
    { uint32_t p = gsu_lmult(R[2], R[6]); R[4] = (uint16_t)p; R[2] = (uint16_t)(p >> 16); }
    R[4] = gsu_sub(R[4], R[1], 0);
    R[0] = gsu_sub(R[2], R[7], !gsu.cy);
    R[0] = gsu_hib(R[0]);
    gsu_stb(R[11], (uint8_t)R[0]);
    t = gsu_count_down(&R[12]);
    R[11] = gsu_inc(R[11]);
    if (t) { gsu_code(0x019469, 1); goto l9427; } else { gsu_code(0x019469, 4); }
    nx = (uint16_t)(gsu_getb());
    gsu_r14(gsu_inc(R[14]));
    at = model_command(nx);
    goto dispatch;
    /* FxPolygonClipScreenUV */
l9A52:
    gsu_stw(R[10], R[11]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[1] = 0x0003;
    gsu_sub(R[0], R[1], 0);   /* CMP */
    t = !gsu.s;
    if (t) goto l9A61;
    goto l9EFB;
l9A61:
    gsu_stw(FX_VERTEX_COUNT_SAVE, R[0]);
    gsu_stw(FX_VERTEX_COUNT, R[0]);
    R[8] = gsu_ldw(FX_POLY_OUT);
    R[9] = 0x0688;
    R[1] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    R[2] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    R[3] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    R[6] = gsu_ldw(R[8]);
    R[8] = gsu_sub(R[8], 6, 0);
    R[0] = gsu_ldw(FX_VERTEX_COUNT);
    R[0] = gsu_mult(R[0], 8);
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_stw(R[0], R[1]);
    R[0] = gsu_inc(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_stw(R[0], R[2]);
    R[0] = gsu_inc(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_stw(R[0], R[3]);
    R[0] = gsu_inc(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_stw(R[0], R[6]);
l9A92:
    R[1] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    R[2] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_add(R[8], 4, 0);
    R[3] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    R[6] = gsu_ldw(R[8]);
    R[8] = gsu_sub(R[8], 2, 0);
    R[0] = gsu_ldw(FX_SCREEN_LEFT);
    R[0] = gsu_sub(R[1], R[0], 0);
    t = gsu.s != gsu.ov;
    if (t) goto l9AB2;
    goto l9AC7;
l9AB2:
    R[0] = gsu_ldw(FX_SCREEN_LEFT);
    R[0] = gsu_sub(R[3], R[0], 0);
    t = gsu.s != gsu.ov;
    if (t) goto l9ABE;
    goto l9AEF;
l9ABE:
    R[0] = gsu_ldw(FX_VERTEX_COUNT_SAVE);
    R[0] = gsu_dec(R[0]);
    gsu_sbk(R[0]);
    goto l9B68;
l9AC7:
    R[0] = gsu_ldw(FX_SCREEN_LEFT);
    R[0] = gsu_sub(R[3], R[0], 0);
    t = gsu.s != gsu.ov;
    if (t) goto l9AD3;
    goto l9B51;
l9AD3:
    R[0] = gsu_ldw(FX_VERTEX_COUNT_SAVE);
    R[0] = gsu_inc(R[0]);
    gsu_sbk(R[0]);
    gsu_stw(R[9], R[1]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    gsu_stw(R[9], R[2]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    R[8] = gsu_sub(R[8], 4, 0);
    R[0] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    gsu_stw(R[9], R[0]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    R[0] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    gsu_stw(R[9], R[0]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
l9AEF:
    R[3] = gsu_sub(R[3], R[1], 0);
    R[6] = gsu_sub(R[6], R[2], 0);
    R[0] = gsu_ldw(FX_SCREEN_LEFT);
    R[0] = gsu_sub(R[0], R[1], 0);
    { uint32_t p = gsu_lmult(R[0], R[6]); R[4] = (uint16_t)p; R[5] = (uint16_t)(p >> 16); }
    R[6] = R[3];
    R[11] = 0x9B01;
    abi_FxDivideSigned();
l9B01:
    R[0] = gsu_add(R[4], R[2], 0);
    R[6] = gsu_ldw(FX_SCREEN_LEFT);
    gsu_stw(R[9], R[6]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    gsu_stw(R[9], R[0]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    R[8] = gsu_add(R[8], 4, 0);
    R[6] = gsu_ldw(R[8]);
    R[8] = gsu_sub(R[8], 8, 0);
    R[2] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    R[6] = gsu_sub(R[6], R[2], 0);
    R[0] = gsu_ldw(FX_SCREEN_LEFT);
    R[0] = gsu_sub(R[0], R[1], 0);
    { uint32_t p = gsu_lmult(R[0], R[6]); R[4] = (uint16_t)p; R[5] = (uint16_t)(p >> 16); }
    R[6] = R[3];
    R[11] = 0x9B29;
    abi_FxDivideSigned();
l9B29:
    R[0] = gsu_add(R[4], R[2], 0);
    gsu_stw(R[9], R[0]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    R[2] = gsu_ldw(R[8]);
    R[8] = gsu_add(R[8], 8, 0);
    R[6] = gsu_ldw(R[8]);
    R[8] = gsu_sub(R[8], 6, 0);
    R[6] = gsu_sub(R[6], R[2], 0);
    R[0] = gsu_ldw(FX_SCREEN_LEFT);
    R[0] = gsu_sub(R[0], R[1], 0);
    { uint32_t p = gsu_lmult(R[0], R[6]); R[4] = (uint16_t)p; R[5] = (uint16_t)(p >> 16); }
    R[6] = R[3];
    R[11] = 0x9B48;
    abi_FxDivideSigned();
l9B48:
    R[0] = gsu_add(R[4], R[2], 0);
    gsu_stw(R[9], R[0]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    goto l9B68;
l9B51:
    gsu_stw(R[9], R[1]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    gsu_stw(R[9], R[2]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    R[8] = gsu_sub(R[8], 4, 0);
    R[0] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    gsu_stw(R[9], R[0]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    R[0] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    gsu_stw(R[9], R[0]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
l9B68:
    R[0] = gsu_ldw(FX_VERTEX_COUNT);
    R[0] = gsu_dec(R[0]);
    gsu_sbk(R[0]);
    t = gsu.z;
    if (t) goto l9B74;
    goto l9A92;
l9B74:
    R[0] = gsu_ldw(FX_VERTEX_COUNT_SAVE);
    R[0] = gsu_sub(R[0], 3, 0);
    t = !gsu.s;
    if (t) goto l9B80;
    goto l9EFB;
l9B80:
    R[0] = gsu_add(R[0], 3, 0);
    gsu_stw(FX_VERTEX_COUNT, R[0]);
    R[8] = 0x0688;
    R[9] = gsu_ldw(FX_POLY_OUT);
    R[1] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    R[2] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    R[3] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    R[6] = gsu_ldw(R[8]);
    R[8] = gsu_sub(R[8], 6, 0);
    R[0] = gsu_ldw(FX_VERTEX_COUNT);
    R[0] = gsu_mult(R[0], 8);
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_stw(R[0], R[1]);
    R[0] = gsu_inc(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_stw(R[0], R[2]);
    R[0] = gsu_inc(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_stw(R[0], R[3]);
    R[0] = gsu_inc(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_stw(R[0], R[6]);
l9BB0:
    R[1] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    R[2] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_add(R[8], 4, 0);
    R[3] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    R[6] = gsu_ldw(R[8]);
    R[8] = gsu_sub(R[8], 2, 0);
    R[0] = gsu_ldw(FX_SCREEN_RIGHT);
    R[0] = gsu_sub(R[1], R[0], 0);
    R[0] = gsu_sub(R[0], 1, 0);
    t = gsu.s == gsu.ov;
    if (t) goto l9BD2;
    goto l9BE9;
l9BD2:
    R[0] = gsu_ldw(FX_SCREEN_RIGHT);
    R[0] = gsu_sub(R[3], R[0], 0);
    R[0] = gsu_sub(R[0], 1, 0);
    t = gsu.s == gsu.ov;
    if (t) goto l9BE0;
    goto l9C13;
l9BE0:
    R[0] = gsu_ldw(FX_VERTEX_COUNT_SAVE);
    R[0] = gsu_dec(R[0]);
    gsu_sbk(R[0]);
    goto l9C8C;
l9BE9:
    R[0] = gsu_ldw(FX_SCREEN_RIGHT);
    R[0] = gsu_sub(R[3], R[0], 0);
    R[0] = gsu_sub(R[0], 1, 0);
    t = gsu.s == gsu.ov;
    if (t) goto l9BF7;
    goto l9C75;
l9BF7:
    R[0] = gsu_ldw(FX_VERTEX_COUNT_SAVE);
    R[0] = gsu_inc(R[0]);
    gsu_sbk(R[0]);
    gsu_stw(R[9], R[1]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    gsu_stw(R[9], R[2]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    R[8] = gsu_sub(R[8], 4, 0);
    R[0] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    gsu_stw(R[9], R[0]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    R[0] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    gsu_stw(R[9], R[0]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
l9C13:
    R[3] = gsu_sub(R[3], R[1], 0);
    R[6] = gsu_sub(R[6], R[2], 0);
    R[0] = gsu_ldw(FX_SCREEN_RIGHT);
    R[0] = gsu_sub(R[0], R[1], 0);
    { uint32_t p = gsu_lmult(R[0], R[6]); R[4] = (uint16_t)p; R[5] = (uint16_t)(p >> 16); }
    R[6] = R[3];
    R[11] = 0x9C25;
    abi_FxDivideSigned();
l9C25:
    R[0] = gsu_add(R[4], R[2], 0);
    R[6] = gsu_ldw(FX_SCREEN_RIGHT);
    gsu_stw(R[9], R[6]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    gsu_stw(R[9], R[0]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    R[8] = gsu_add(R[8], 4, 0);
    R[6] = gsu_ldw(R[8]);
    R[8] = gsu_sub(R[8], 8, 0);
    R[2] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    R[6] = gsu_sub(R[6], R[2], 0);
    R[0] = gsu_ldw(FX_SCREEN_RIGHT);
    R[0] = gsu_sub(R[0], R[1], 0);
    { uint32_t p = gsu_lmult(R[0], R[6]); R[4] = (uint16_t)p; R[5] = (uint16_t)(p >> 16); }
    R[6] = R[3];
    R[11] = 0x9C4D;
    abi_FxDivideSigned();
l9C4D:
    R[0] = gsu_add(R[4], R[2], 0);
    gsu_stw(R[9], R[0]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    R[2] = gsu_ldw(R[8]);
    R[8] = gsu_add(R[8], 8, 0);
    R[6] = gsu_ldw(R[8]);
    R[8] = gsu_sub(R[8], 6, 0);
    R[6] = gsu_sub(R[6], R[2], 0);
    R[0] = gsu_ldw(FX_SCREEN_RIGHT);
    R[0] = gsu_sub(R[0], R[1], 0);
    { uint32_t p = gsu_lmult(R[0], R[6]); R[4] = (uint16_t)p; R[5] = (uint16_t)(p >> 16); }
    R[6] = R[3];
    R[11] = 0x9C6C;
    abi_FxDivideSigned();
l9C6C:
    R[0] = gsu_add(R[4], R[2], 0);
    gsu_stw(R[9], R[0]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    goto l9C8C;
l9C75:
    gsu_stw(R[9], R[1]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    gsu_stw(R[9], R[2]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    R[8] = gsu_sub(R[8], 4, 0);
    R[0] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    gsu_stw(R[9], R[0]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    R[0] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    gsu_stw(R[9], R[0]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
l9C8C:
    R[0] = gsu_ldw(FX_VERTEX_COUNT);
    R[0] = gsu_dec(R[0]);
    gsu_sbk(R[0]);
    t = gsu.z;
    if (t) goto l9C98;
    goto l9BB0;
l9C98:
    R[0] = gsu_ldw(FX_VERTEX_COUNT_SAVE);
    R[0] = gsu_sub(R[0], 3, 0);
    t = !gsu.s;
    if (t) goto l9CA4;
    goto l9EFB;
l9CA4:
    R[0] = gsu_add(R[0], 3, 0);
    gsu_stw(FX_VERTEX_COUNT, R[0]);
    R[8] = gsu_ldw(FX_POLY_OUT);
    R[9] = 0x0688;
    R[1] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    R[2] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    R[3] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    R[6] = gsu_ldw(R[8]);
    R[8] = gsu_sub(R[8], 6, 0);
    R[0] = gsu_ldw(FX_VERTEX_COUNT);
    R[0] = gsu_mult(R[0], 8);
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_stw(R[0], R[1]);
    R[0] = gsu_inc(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_stw(R[0], R[2]);
    R[0] = gsu_inc(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_stw(R[0], R[3]);
    R[0] = gsu_inc(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_stw(R[0], R[6]);
l9CD4:
    R[1] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    R[2] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_add(R[8], 4, 0);
    R[3] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    R[6] = gsu_ldw(R[8]);
    R[8] = gsu_sub(R[8], 2, 0);
    R[0] = gsu_ldw(FX_SCREEN_TOP);
    R[0] = gsu_sub(R[2], R[0], 0);
    t = gsu.s != gsu.ov;
    if (t) goto l9CF4;
    goto l9D09;
l9CF4:
    R[0] = gsu_ldw(FX_SCREEN_TOP);
    R[0] = gsu_sub(R[6], R[0], 0);
    t = gsu.s != gsu.ov;
    if (t) goto l9D00;
    goto l9D31;
l9D00:
    R[0] = gsu_ldw(FX_VERTEX_COUNT_SAVE);
    R[0] = gsu_dec(R[0]);
    gsu_sbk(R[0]);
    goto l9DB1;
l9D09:
    R[0] = gsu_ldw(FX_SCREEN_TOP);
    R[0] = gsu_sub(R[6], R[0], 0);
    t = gsu.s != gsu.ov;
    if (t) goto l9D15;
    goto l9D9A;
l9D15:
    R[0] = gsu_ldw(FX_VERTEX_COUNT_SAVE);
    R[0] = gsu_inc(R[0]);
    gsu_sbk(R[0]);
    gsu_stw(R[9], R[1]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    gsu_stw(R[9], R[2]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    R[8] = gsu_sub(R[8], 4, 0);
    R[0] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    gsu_stw(R[9], R[0]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    R[0] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    gsu_stw(R[9], R[0]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
l9D31:
    R[3] = gsu_sub(R[3], R[1], 0);
    R[6] = gsu_sub(R[6], R[2], 0);
    R[0] = R[3];
    R[3] = R[6];
    R[6] = R[0];
    R[0] = gsu_ldw(FX_SCREEN_TOP);
    R[0] = gsu_sub(R[0], R[2], 0);
    gsu_stw(FX_MODEL_96, R[0]);
    { uint32_t p = gsu_lmult(R[0], R[6]); R[4] = (uint16_t)p; R[5] = (uint16_t)(p >> 16); }
    R[6] = R[3];
    R[11] = 0x9D4C;
    abi_FxDivideSigned();
l9D4C:
    R[0] = gsu_add(R[4], R[1], 0);
    R[6] = gsu_ldw(FX_SCREEN_TOP);
    gsu_stw(R[9], R[0]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    gsu_stw(R[9], R[6]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    R[8] = gsu_add(R[8], 4, 0);
    R[6] = gsu_ldw(R[8]);
    R[8] = gsu_sub(R[8], 8, 0);
    R[2] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    R[6] = gsu_sub(R[6], R[2], 0);
    R[0] = gsu_ldw(FX_MODEL_96);
    { uint32_t p = gsu_lmult(R[0], R[6]); R[4] = (uint16_t)p; R[5] = (uint16_t)(p >> 16); }
    R[6] = R[3];
    R[11] = 0x9D73;
    abi_FxDivideSigned();
l9D73:
    R[0] = gsu_add(R[4], R[2], 0);
    gsu_stw(R[9], R[0]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    R[2] = gsu_ldw(R[8]);
    R[8] = gsu_add(R[8], 8, 0);
    R[6] = gsu_ldw(R[8]);
    R[8] = gsu_sub(R[8], 6, 0);
    R[6] = gsu_sub(R[6], R[2], 0);
    R[0] = gsu_ldw(FX_MODEL_96);
    { uint32_t p = gsu_lmult(R[0], R[6]); R[4] = (uint16_t)p; R[5] = (uint16_t)(p >> 16); }
    R[6] = R[3];
    R[11] = 0x9D91;
    abi_FxDivideSigned();
l9D91:
    R[0] = gsu_add(R[4], R[2], 0);
    gsu_stw(R[9], R[0]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    goto l9DB1;
l9D9A:
    gsu_stw(R[9], R[1]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    gsu_stw(R[9], R[2]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    R[8] = gsu_sub(R[8], 4, 0);
    R[0] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    gsu_stw(R[9], R[0]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    R[0] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    gsu_stw(R[9], R[0]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
l9DB1:
    R[0] = gsu_ldw(FX_VERTEX_COUNT);
    R[0] = gsu_dec(R[0]);
    gsu_sbk(R[0]);
    t = gsu.z;
    if (t) goto l9DBD;
    goto l9CD4;
l9DBD:
    R[0] = gsu_ldw(FX_VERTEX_COUNT_SAVE);
    R[0] = gsu_sub(R[0], 3, 0);
    t = !gsu.s;
    if (t) goto l9DC9;
    goto l9EFB;
l9DC9:
    R[0] = gsu_add(R[0], 3, 0);
    gsu_stw(FX_VERTEX_COUNT, R[0]);
    R[8] = 0x0688;
    R[9] = gsu_ldw(FX_POLY_OUT);
    R[1] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    R[2] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    R[3] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    R[6] = gsu_ldw(R[8]);
    R[8] = gsu_sub(R[8], 6, 0);
    R[0] = gsu_ldw(FX_VERTEX_COUNT);
    R[0] = gsu_mult(R[0], 8);
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_stw(R[0], R[1]);
    R[0] = gsu_inc(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_stw(R[0], R[2]);
    R[0] = gsu_inc(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_stw(R[0], R[3]);
    R[0] = gsu_inc(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_stw(R[0], R[6]);
l9DF9:
    R[1] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    R[2] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_add(R[8], 4, 0);
    R[3] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    R[6] = gsu_ldw(R[8]);
    R[8] = gsu_sub(R[8], 2, 0);
    R[0] = gsu_ldw(FX_SCREEN_BOTTOM);
    R[0] = gsu_sub(R[2], R[0], 0);
    R[0] = gsu_sub(R[0], 1, 0);
    t = gsu.s == gsu.ov;
    if (t) goto l9E1B;
    goto l9E32;
l9E1B:
    R[0] = gsu_ldw(FX_SCREEN_BOTTOM);
    R[0] = gsu_sub(R[6], R[0], 0);
    R[0] = gsu_sub(R[0], 1, 0);
    t = gsu.s == gsu.ov;
    if (t) goto l9E29;
    goto l9E5C;
l9E29:
    R[0] = gsu_ldw(FX_VERTEX_COUNT_SAVE);
    R[0] = gsu_dec(R[0]);
    gsu_sbk(R[0]);
    goto l9EDC;
l9E32:
    R[0] = gsu_ldw(FX_SCREEN_BOTTOM);
    R[0] = gsu_sub(R[6], R[0], 0);
    R[0] = gsu_sub(R[0], 1, 0);
    t = gsu.s == gsu.ov;
    if (t) goto l9E40;
    goto l9EC5;
l9E40:
    R[0] = gsu_ldw(FX_VERTEX_COUNT_SAVE);
    R[0] = gsu_inc(R[0]);
    gsu_sbk(R[0]);
    gsu_stw(R[9], R[1]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    gsu_stw(R[9], R[2]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    R[8] = gsu_sub(R[8], 4, 0);
    R[0] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    gsu_stw(R[9], R[0]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    R[0] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    gsu_stw(R[9], R[0]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
l9E5C:
    R[3] = gsu_sub(R[3], R[1], 0);
    R[6] = gsu_sub(R[6], R[2], 0);
    R[0] = R[3];
    R[3] = R[6];
    R[6] = R[0];
    R[0] = gsu_ldw(FX_SCREEN_BOTTOM);
    R[0] = gsu_sub(R[0], R[2], 0);
    gsu_stw(FX_MODEL_96, R[0]);
    { uint32_t p = gsu_lmult(R[0], R[6]); R[4] = (uint16_t)p; R[5] = (uint16_t)(p >> 16); }
    R[6] = R[3];
    R[11] = 0x9E77;
    abi_FxDivideSigned();
l9E77:
    R[0] = gsu_add(R[4], R[1], 0);
    R[6] = gsu_ldw(FX_SCREEN_BOTTOM);
    gsu_stw(R[9], R[0]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    gsu_stw(R[9], R[6]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    R[8] = gsu_add(R[8], 4, 0);
    R[6] = gsu_ldw(R[8]);
    R[8] = gsu_sub(R[8], 8, 0);
    R[2] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    R[6] = gsu_sub(R[6], R[2], 0);
    R[0] = gsu_ldw(FX_MODEL_96);
    { uint32_t p = gsu_lmult(R[0], R[6]); R[4] = (uint16_t)p; R[5] = (uint16_t)(p >> 16); }
    R[6] = R[3];
    R[11] = 0x9E9E;
    abi_FxDivideSigned();
l9E9E:
    R[0] = gsu_add(R[4], R[2], 0);
    gsu_stw(R[9], R[0]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    R[2] = gsu_ldw(R[8]);
    R[8] = gsu_add(R[8], 8, 0);
    R[6] = gsu_ldw(R[8]);
    R[8] = gsu_sub(R[8], 6, 0);
    R[6] = gsu_sub(R[6], R[2], 0);
    R[0] = gsu_ldw(FX_MODEL_96);
    { uint32_t p = gsu_lmult(R[0], R[6]); R[4] = (uint16_t)p; R[5] = (uint16_t)(p >> 16); }
    R[6] = R[3];
    R[11] = 0x9EBC;
    abi_FxDivideSigned();
l9EBC:
    R[0] = gsu_add(R[4], R[2], 0);
    gsu_stw(R[9], R[0]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    goto l9EDC;
l9EC5:
    gsu_stw(R[9], R[1]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    gsu_stw(R[9], R[2]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    R[8] = gsu_sub(R[8], 4, 0);
    R[0] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    gsu_stw(R[9], R[0]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    R[0] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    gsu_stw(R[9], R[0]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
l9EDC:
    R[0] = gsu_ldw(FX_VERTEX_COUNT);
    R[0] = gsu_dec(R[0]);
    gsu_sbk(R[0]);
    t = gsu.z;
    if (t) goto l9EE8;
    goto l9DF9;
l9EE8:
    R[0] = gsu_ldw(FX_VERTEX_COUNT_SAVE);
    R[0] = gsu_sub(R[0], 3, 0);
    t = !gsu.s;
    if (t) goto l9EF4;
    goto l9EFB;
l9EF4:
    R[0] = gsu_add(R[0], 3, 0);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
l9EFB:
    R[0] = gsu_sub(R[0], R[0], 0);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
    /* FxReturnR11 */
lAB11:
    nx = (uint16_t)(R[11]);
    at = nx;
    goto dispatch;
dispatch:
    if (at == ret)
        return;
    /* An entry of the command table (01:8000): its IWT R15 goes to the
       command's code. */
    if (at >= 0x8000 && at < 0x8000 + 4 * 26 && !(at & 3))
        at = commands[(at - 0x8000) >> 2];
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
    case 0x8863: gsu_code(0x010020, 4); gsu_code(0x018863, 1); goto l8863;
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
    case 0x90CD: gsu_code(0x010014, 4); goto l90CD;
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
