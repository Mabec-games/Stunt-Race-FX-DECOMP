/*
 * fx_physics.c: the Super FX's physics program (04:8FCA) and what it runs:
 * the record handlers (falling, bouncing and dropped objects, touch
 * marks, the car), collisions of a record with what it touches (as a
 * box or against a model's faces), the twenty contact slots at $23AE and
 * how a contact pushes back, a car's parts (wheels on the ground, parts
 * pulled after the car) and keeping a car on its course.
 *
 * The C was drafted instruction for instruction from the ROM bytes and is
 * kept so: every register, flag and RAM access as the code makes them,
 * one per statement, a branch's delay slot after its decision. Jumps
 * through registers (record handlers by JMP R12, the push table, returns
 * pulled from the R10 stack, two levels at once in FxCarAimAlongCourse)
 * go through physics_run's dispatch on the address they reach.
 */
#include "gsu.h"
#include "gsu_ops.h"

#define R gsu.r

#define GSU_IN_20                0x0020u
#define GSU_IN_22                0x0022u
#define GSU_IN_24                0x0024u
#define GSU_DEST                 0x002Cu
#define GSU_IN_2E                0x002Eu
#define GSU_SPEED_44             0x0044u
#define GSU_IN_68                0x0068u
#define GSU_IN_6A                0x006Au
#define GSU_IN_A2                0x00A2u
#define GSU_IN_A4                0x00A4u
#define GSU_OUT_E4               0x00E4u
#define FX_OBJECT_MATRIX_1       0x00E6u
#define FX_OBJECT_MATRIX_2       0x00E8u
#define FX_OBJECT_MATRIX_3       0x00EAu
#define FX_OBJECT_MATRIX_4       0x00ECu
#define FX_OBJECT_MATRIX_5       0x00EEu
#define FX_OBJECT_MATRIX_6       0x00F0u
#define FX_OBJECT_MATRIX_7       0x00F2u
#define FX_OBJECT_MATRIX_8       0x00F4u
#define FX_VIEW_MATRIX_2         0x0136u
#define FX_VIEW_MATRIX_8         0x0142u
#define FX_RECORDS               0x2332u
#define FX_HIT                   0x2334u
#define FX_SPIN                  0x233Au
#define FX_WORLD_SUM             0x2340u
#define FX_CAR_POS               0x2354u
#define FX_RAM_TEST              0x235Cu
#define FX_PART_Z_SPEED          0x235Eu
#define FX_WHEEL_LOAD            0x2364u
#define FX_WHEEL_SIDE            0x2366u
#define FX_WHEEL_AHEAD           0x236Au
#define FX_SIZES                 0x236Cu
#define FX_FACE_NORMAL           0x2372u
#define FX_PUSH_AMOUNT           0x2378u
#define FX_FACE_PLANE            0x237Au
#define FX_FACE_FLAGS            0x237Cu
#define FX_CONTACT_DEPTH         0x237Eu
#define FX_INIT_2380             0x2380u
#define FX_INIT_2382             0x2382u
#define FX_FACE_SCALE            0x2384u
#define FX_CONTACT_FORCE         0x2386u
#define FX_CONTACT_SUM           0x2388u
#define FX_HIT_DEPTH             0x239Cu
#define FX_CONTACT_FORCE_SIDE    0x239Eu
#define FX_CONTACT_FORCE_UP      0x23A0u
#define FX_PUSHES                0x23AEu
#define FX_INIT_2470             0x2470u
#define FX_INIT_2472             0x2472u
#define FX_PUSH_TURN             0x247Eu
#define FX_PUSH_FLAGS            0x2480u
#define FX_WHEEL_FLAGS           0x2484u
#define FX_COLLIDE_MODEL         0x2486u
#define FX_HIT_ON                0x263Cu
#define FX_WHEEL_CONTACT         0x2640u
#define FX_PARAM_HIT             0x2652u
#define FX_PARAM_PUSH            0x2654u
#define FX_PARAM_BOUNCE          0x2656u
#define FX_PARAM_THRUST_BOOST    0x2658u
#define FX_PARAM_THRUST          0x265Au
#define FX_PARAM_REVERSE         0x265Cu
#define FX_PARAM_WHEEL_48        0x265Eu
#define FX_PARAM_TURN_GRIP       0x2660u
#define FX_PARAM_GRIP_FREE       0x266Cu
#define FX_PARAM_GRIP_SLIDE      0x266Eu
#define FX_PARAM_SPIN_DAMP       0x2670u
#define FX_PARAM_LIFT            0x2672u
#define FX_PARAM_GRIP_SIDE       0x2674u
#define FX_PARAM_GRIP_AHEAD      0x2676u
#define FX_PARAM_CURVE_A         0x267Au
#define FX_PARAM_CURVE_B         0x267Cu
#define FX_PARAM_SPEED_CAP       0x267Eu
#define FX_PARAM_SLIDE_LIMIT     0x2680u
#define FX_PARAM_JUMP            0x2682u
#define GSU_CONTROL_26F2         0x26F2u
#define FX_DRIVE_LEVEL           0x26F4u
#define FX_CLOSING               0x26F6u

void abi_FxAllTouches(void);
void abi_FxAngleVector(void);
void abi_FxCarCatchUp(void);
void abi_FxCarHit(void);
void abi_FxCarMoveParts(void);
void abi_FxCarPose(void);
void abi_FxCollidePush(void);
void abi_FxCourseEntryRead(void);
void abi_FxCourseNearest(void);
void abi_FxCourseNextDistance(void);
void abi_FxCourseNextEntry(void);
void abi_FxDriveDecide(void);
void abi_FxHitAddScaled(void);
void abi_FxHitAddUnit(void);
void abi_FxLapSegment(void);
void abi_FxMatrixFromAngles(void);
void abi_FxMatrixTranspose04(void);
void abi_FxModelPoints(void);
void abi_FxNearestCar(void);
void abi_FxNormalize(void);
void abi_FxObjectFree(void);
void abi_FxPushApply(void);
void abi_FxPushAt1(void);
void abi_FxPushAt2(void);
void abi_FxPushAt4(void);
void abi_FxPushAt4Twist(void);
void abi_FxPushAtHalf(void);
void abi_FxPushAtQuarter(void);
void abi_FxPushSet(void);
void abi_FxPushesApply(void);
void abi_FxRankCars(void);
void abi_FxRecordAlignment(void);
void abi_FxRecordHeadingAhead(void);
void abi_FxRecordHoldY(void);
void abi_FxRecordMove(void);
void abi_FxRecordSegment(void);
void abi_FxSteerFromInput(void);
void abi_FxSumsApply(void);
void abi_FxSumsClear(void);
void abi_FxSumsPushLocal(void);
void abi_FxSumsPushWorld(void);
void abi_FxSumsRotateLocal(void);
void abi_FxTableEntryWords(void);
void abi_FxTrafficSides(void);
void abi_FxTurn(void);

/* The code from address at until it jumps to ret (the caller's return;
   $FFFF for a program, which ends at its STOP), or, for a call, until it
   jumps with the R10 stack two words or more below where it began: it
   took its caller's frame and returned two levels up (04:C72F), and the
   caller's code is not its to run. */
static void physics_run(uint16_t at, uint16_t ret)
{
    gsu_code(0x048FCB, 12);
    int t;
    uint16_t nx;
    uint16_t base = R[10];

    (void)t;
    (void)nx;
    goto dispatch;
    /* FxPhysicsProgram */
l8FCA:
    R[10] = 0x0240;
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_romb(R[0]);
    gsu.rambr = (uint8_t)(R[0] & 1);
    R[0] = gsu_ldw((FX_PUSHES + 190));
    R[0] = R[0];
    gsu_moves(R[0]);
    t = !gsu.z;
    if (t) { gsu_code(0x048FD7, 4); gsu_code(0x048FDF, 5); goto l8FDF; }
    goto l9078;
l8FDF:
    R[0] = gsu_ldw(FX_INIT_2472);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.z;
    R[0] = gsu_dec(R[0]);
    if (t) { gsu_code(0x048FE4, 4); gsu_code(0x048FE9, 5); goto l8FE9; }
    gsu_sbk(R[0]);
l8FE9:
    R[0] = gsu_ldw(FX_INIT_2470);
    R[0] = gsu_inc(R[0]);
    gsu_code(0x048FEE, 2);
    gsu_sbk(R[0]);
    R[0] = 0xFFFF;
    gsu_code(0x048FF0, 7);
    gsu_stw((FX_PUSHES + 192), R[0]);
    R[0] = gsu_ldw(gsu_code_then(0x048FF7, 4, FX_INIT_2472));
    R[0] = R[0];
    gsu_moves(R[0]);
    t = !gsu.z;
    if (t) goto l9004;
    R[11] = 0x9004;
    abi_FxAllTouches();
l9004:
    R[0] = gsu_ldw(FX_RECORDS);
l9008:
    R[8] = R[0];
    gsu_code(0x049009, 4);
    gsu_moves(R[0]);
    t = gsu.z;
    if (t) { gsu_code(0x049069, 5); gsu_code(0x04CE1D, 3); goto l9069; } else { gsu_code(0x04900D, 5); }
    R[0] = 0x000A;
    R[0] = gsu_add(R[0], R[8], 0);
    R[0] = gsu_ldw(R[0]);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.z;
    if (t) { gsu_code(0x049012, 4); goto l903D; } else { gsu_code(0x049012, 11); }
    R[12] = R[0];
    R[0] = 0x001C;
    R[0] = gsu_add(R[0], R[8], 0);
    R[0] = gsu_ldw(R[0]);
    R[1] = 0x0008;
    R[0] = gsu_and(R[0], R[1]);
    t = gsu.z;
    if (t) { gsu_code(0x04901D, 6); gsu_code(0x049034, 1); goto l9034; } else { gsu_code(0x04901D, 9); }
    gsu_stw(R[10], R[12]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[0] = 0x0000;
    gsu_code(0x049026, 8);
    gsu_stw(FX_HIT_ON, R[0]);
    R[11] = 0x9030;
    nx = (uint16_t)(R[12]);
    at = nx;
    goto dispatch;
l9030:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    R[12] = gsu_ldw(gsu_code_then(0x049030, 5, R[10]));
l9034:
    R[0] = 0x0001;
    gsu_code(0x049035, 6);
    gsu_stw(FX_HIT_ON, R[0]);
    R[11] = 0x903D;
    nx = (uint16_t)(R[12]);
    at = nx;
    goto dispatch;
l903D:
    R[0] = 0x001C;
    R[0] = gsu_add(R[0], R[8], 0);
    R[0] = gsu_ldw(gsu_code_then(0x04903D, 5, R[0]));
    R[2] = R[0];
    R[1] = 0x0004;
    R[0] = gsu_and(R[0], R[1]);
    t = !gsu.z;
    if (t) { gsu_code(0x049042, 8); gsu_code(0x04905E, 5); goto l905E; } else { gsu_code(0x049042, 13); }
    R[0] = 0x0004;
    R[0] = gsu_add(R[0], R[8], 0);
    R[0] = gsu_ldw(R[0]);
    gsu_code(0x04904F, 1);
    gsu_stw(R[10], R[0]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[1] = R[8];
    R[11] = 0x9058;
    (gsu_code(0x048A08, 3), gsu_code(0x049050, 8), abi_FxObjectFree());
l9058:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    R[0] = gsu_ldw(gsu_code_then(0x049058, 4, R[10]));
    t = 1;
    if (t) { gsu_code(0x049008, 1); gsu_code(0x04905C, 2); goto l9008; }
l905E:
    R[0] = gsu_bic(R[2], R[1]);
    gsu_sbk(R[0]);
    R[0] = 0x0004;
    R[0] = gsu_add(R[0], R[8], 0);
    R[0] = gsu_ldw(gsu_code_then(0x049063, 4, R[0]));
    t = 1;
    if (t) { gsu_code(0x049008, 1); gsu_code(0x049067, 2); goto l9008; }
l9069:
    R[11] = 0x906E;
    abi_FxRankCars();
l906E:
    R[11] = 0x9073;
    gsu_code(0x04906E, 5); gsu_code(0x04CF08, 3);
    abi_FxNearestCar();
l9073:
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_code(0x049073, 6);
    gsu_stw((FX_PUSHES + 192), R[0]);
l9078:
    return;   /* the STOP: the program ends here */
    /* FxObjectBounce */
l907A:
    gsu_stw(R[10], R[11]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_stw(R[10], R[8]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[9] = R[8];
    R[1] = 0x0030;
    R[1] = gsu_add(R[1], R[9], 0);
    R[1] = gsu_ldw(R[1]);
    R[0] = 0x0078;
    R[1] = gsu_add(R[0], R[1], 0);
    R[0] = 0x0030;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_stw(R[0], R[1]);
    R[1] = 0x0003;
    R[11] = 0x909B;
    abi_FxRecordMove();
l909B:
    R[11] = 0x90A0;
    goto l91FF;
l90A0:
    R[0] = gsu_ldw(FX_HIT_DEPTH);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.z;
    if (t) goto l90DF;
    t = gsu.s;
    if (t) goto l90B9;
    R[1] = 0x0030;
    R[1] = gsu_add(R[1], R[9], 0);
    R[1] = gsu_ldw(R[1]);
    R[1] = gsu_sub(R[1], R[0], 0);
    R[0] = 0x0030;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_stw(R[0], R[1]);
l90B9:
    R[0] = 0x002E;
    R[0] = gsu_add(R[0], R[9], 0);
    R[0] = gsu_ldw(R[0]);
    R[1] = R[0];
    R[0] = gsu_div2(R[0]);
    R[0] = gsu_div2(R[0]);
    R[0] = gsu_div2(R[0]);
    R[1] = gsu_sub(R[1], R[0], 0);
    R[0] = 0x002E;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_stw(R[0], R[1]);
    R[0] = 0x0032;
    R[0] = gsu_add(R[0], R[9], 0);
    R[0] = gsu_ldw(R[0]);
    R[1] = R[0];
    R[0] = gsu_div2(R[0]);
    R[0] = gsu_div2(R[0]);
    R[0] = gsu_div2(R[0]);
    R[1] = gsu_sub(R[1], R[0], 0);
    R[0] = 0x0032;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_stw(R[0], R[1]);
l90DF:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    R[8] = gsu_ldw(R[10]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
    /* FxObjectMarkTouched */
l90E8:
    R[8] = R[8];
    R[1] = 0x0018;
    R[1] = gsu_add(R[1], R[8], 0);
    R[1] = gsu_ldw(gsu_code_then(0x04903B, 2, gsu_code_then(0x0490E8, 9, R[1])));
    R[1] = R[1];
    gsu_moves(R[1]);
    t = gsu.z;
    if (t) { gsu_code(0x0490F1, 4); goto l9107; } else { gsu_code(0x0490F1, 12); }
    R[2] = 0x0002;
l90F8:
    R[0] = 0x0010;
    R[0] = gsu_add(R[0], R[1], 0);
    R[0] = gsu_ldw(R[0]);
    R[0] = gsu_or(R[0], R[2]);
    gsu_code(0x0490FD, 2);
    gsu_sbk(R[0]);
    R[0] = 0x000E;
    R[0] = gsu_add(R[0], R[1], 0);
    R[0] = gsu_ldw(gsu_code_then(0x0490FF, 4, R[0]));
    R[1] = R[0];
    gsu_moves(R[0]);
    t = !gsu.z;
    if (t) goto l90F8; else { gsu_code(0x049103, 4); }
l9107:
    gsu_code(0x049107, 3);
    nx = (uint16_t)(R[11]);
    at = nx;
    goto dispatch;
    /* FxObjectFall */
l910A:
    gsu_stw(R[10], R[11]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_stw(R[10], R[8]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[9] = R[8];
    R[0] = 0x0018;
    R[0] = gsu_add(R[0], R[9], 0);
    R[0] = gsu_ldw(R[0]);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.z;
    if (t) goto l912D;
    R[1] = 0x0030;
    R[1] = gsu_add(R[1], R[9], 0);
    R[1] = gsu_ldw(R[1]);
    R[0] = 0x0078;
    R[1] = gsu_add(R[0], R[1], 0);
    R[0] = 0x0030;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_stw(R[0], R[1]);
l912D:
    R[1] = 0x0003;
    R[11] = 0x9134;
    abi_FxRecordMove();
l9134:
    R[11] = 0x9139;
    goto l91FF;
l9139:
    R[0] = gsu_ldw(FX_HIT_DEPTH);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.z;
    if (t) goto l9164;
    R[0] = 0x002E;
    R[0] = gsu_add(R[0], R[9], 0);
    R[0] = gsu_ldw(R[0]);
    R[1] = R[0];
    R[0] = gsu_div2(R[0]);
    R[0] = gsu_div2(R[0]);
    R[1] = gsu_sub(R[1], R[0], 0);
    R[0] = 0x002E;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_stw(R[0], R[1]);
    R[0] = 0x0032;
    R[0] = gsu_add(R[0], R[9], 0);
    R[0] = gsu_ldw(R[0]);
    R[1] = R[0];
    R[0] = gsu_div2(R[0]);
    R[0] = gsu_div2(R[0]);
    R[1] = gsu_sub(R[1], R[0], 0);
    R[0] = 0x0032;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_stw(R[0], R[1]);
l9164:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    R[8] = gsu_ldw(R[10]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
    /* FxObjectCollideOnly */
l916D:
    gsu_stw(R[10], R[11]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_stw(R[10], R[8]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[9] = R[8];
    R[11] = 0x917C;
    goto l91FF;
l917C:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    R[8] = gsu_ldw(R[10]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
    /* FxObjectDrop */
l9185:
    gsu_stw(R[10], R[11]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_stw(R[10], R[8]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[9] = R[8];
    R[1] = 0x0030;
    R[1] = gsu_add(R[1], R[9], 0);
    R[1] = gsu_ldw(R[1]);
    R[0] = 0x003C;
    R[1] = gsu_add(R[0], R[1], 0);
    R[0] = 0x0030;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_stw(R[0], R[1]);
    R[1] = 0x0000;
    R[0] = 0x0032;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_stw(R[0], R[1]);
    R[0] = 0x0032;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_stw(R[0], R[1]);
    R[1] = 0x0003;
    R[11] = 0x91B2;
    abi_FxRecordMove();
l91B2:
    R[1] = 0x001E;
    R[1] = gsu_add(R[1], R[9], 0);
    R[1] = gsu_ldw(R[1]);
    R[3] = 0x0022;
    R[3] = gsu_add(R[3], R[9], 0);
    R[3] = gsu_ldw(R[3]);
    gsu_stw(R[10], R[1]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_stw(R[10], R[3]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[11] = 0x91CB;
    goto l91FF;
l91CB:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    R[3] = gsu_ldw(R[10]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    R[1] = gsu_ldw(R[10]);
    R[0] = gsu_ldw(FX_HIT_DEPTH);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = !gsu.z;
    if (t) goto l91EF;
    R[0] = 0x001E;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_stw(R[0], R[1]);
    R[0] = 0x0022;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_stw(R[0], R[3]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    R[8] = gsu_ldw(R[10]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
l91EF:
    R[1] = 0x0000;
    R[0] = 0x000A;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_stw(R[0], R[1]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    R[8] = gsu_ldw(R[10]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
    /* FxRecordCollide */
l91FF:
    gsu_stw(R[10], R[11]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_stw(FX_PUSH_TURN, R[0]);
    gsu_stw(FX_HIT_DEPTH, R[0]);
    R[11] = 0x9211;
    abi_FxSumsClear();
l9211:
    R[3] = 0x0001;
    R[0] = 0x001C;
    R[0] = gsu_add(R[0], R[9], 0);
    R[0] = gsu_ldw(R[0]);
    R[0] = gsu_bic(R[0], R[3]);
    gsu_sbk(R[0]);
    R[0] = 0x0018;
    R[0] = gsu_add(R[0], R[9], 0);
    R[0] = gsu_ldw(R[0]);
    R[1] = R[0];
    gsu_moves(R[0]);
    t = !gsu.z;
    if (t) goto l9228;
    goto l92C5;
l9228:
l9229:
    gsu_stw(R[10], R[1]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[7] = 0x000C;
    R[7] = gsu_add(R[7], R[1], 0);
    R[7] = gsu_ldw(R[7]);
    gsu_stw(FX_COLLIDE_MODEL, R[7]);
    R[0] = 0x0010;
    R[0] = gsu_add(R[0], R[1], 0);
    R[0] = gsu_ldw(R[0]);
    R[3] = 0x0002;
    R[0] = gsu_bic(R[0], R[3]);
    gsu_sbk(R[0]);
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_stw(FX_PUSH_AMOUNT, R[0]);
    R[0] = 0x0016;
    R[0] = gsu_add(R[0], R[7], 0);
    R[0] = gsu_ldw(R[0]);
    R[2] = 0x0010;
    R[0] = gsu_and(R[0], R[2]);
    t = !gsu.z;
    if (t) goto l926A;
    R[11] = 0x9256;
    goto l9C3D;
l9256:
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.z;
    if (t) goto l92B4;
    R[11] = 0x9260;
    abi_FxPushesApply();
l9260:
    R[2] = gsu_ldw(FX_PUSH_AMOUNT);
    R[2] = gsu_not(R[2]);
    R[2] = gsu_inc(R[2]);
    t = 1;
    if (t) goto l927B;
l926A:
    R[11] = 0x926F;
    abi_FxCollidePush();
l926F:
    R[3] = 0x23A4;
    R[11] = 0x9277;
    abi_FxPushApply();
l9277:
    R[2] = gsu_ldw(FX_PUSH_AMOUNT);
l927B:
    R[0] = gsu_ldw(FX_HIT_DEPTH);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = !gsu.z;
    if (t) goto l9288;
    gsu_stw(FX_HIT_DEPTH, R[2]);
l9288:
    R[2] = R[2];
    gsu_moves(R[2]);
    t = gsu.z;
    if (t) goto l92B4;
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    R[1] = gsu_ldw(R[10]);
    R[0] = gsu_ldw(FX_PUSH_FLAGS);
    R[3] = 0x0020;
    R[0] = gsu_and(R[0], R[3]);
    t = gsu.z;
    if (t) goto l92A7;
    R[3] = 0x002C;
    R[3] = gsu_add(R[3], R[9], 0);
    R[3] = gsu_ldw(R[3]);
    R[0] = 0x0020;
    R[0] = gsu_or(R[0], R[3]);
    gsu_sbk(R[0]);
l92A7:
    R[3] = 0x0010;
    R[3] = gsu_add(R[3], R[1], 0);
    R[3] = gsu_ldw(R[3]);
    R[0] = 0x0002;
    R[0] = gsu_or(R[0], R[3]);
    t = 1;
    gsu_sbk(R[0]);
    if (t) goto l92B8;
l92B4:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    R[1] = gsu_ldw(R[10]);
l92B8:
    R[0] = 0x000E;
    R[0] = gsu_add(R[0], R[1], 0);
    R[0] = gsu_ldw(R[0]);
    R[1] = R[0];
    gsu_moves(R[0]);
    t = gsu.z;
    if (t) goto l92C5;
    goto l9229;
l92C5:
    R[2] = 0x0020;
    R[2] = gsu_add(R[2], R[9], 0);
    R[2] = gsu_ldw(R[2]);
    R[11] = 0x92D0;
    abi_FxPushSet();
l92D0:
    R[3] = 0x23A4;
    R[11] = 0x92D8;
    abi_FxPushApply();
l92D8:
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.z;
    if (t) goto l92F7;
    R[3] = 0x0001;
    R[0] = 0x001C;
    R[0] = gsu_add(R[0], R[9], 0);
    R[0] = gsu_ldw(R[0]);
    R[0] = gsu_or(R[0], R[3]);
    gsu_sbk(R[0]);
    R[0] = gsu_ldw(FX_HIT_DEPTH);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = !gsu.z;
    if (t) goto l92F7;
    R[0] = gsu_ldw(FX_PUSH_AMOUNT);
    gsu_stw(FX_HIT_DEPTH, R[0]);
l92F7:
    R[11] = 0x92FC;
    abi_FxSumsApply();
l92FC:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
    /* FxContactsAnyTouch */
l957C:
    /* CACHE */
    R[3] = 0x23AE;
    R[13] = 0x0000;
l9582:
    R[13] = gsu_inc(R[13]);
    R[0] = 0x0014;
    gsu_sub(R[0], R[13], 0);   /* CMP */
    t = gsu.z;
    if (t) goto l95B4;
    R[0] = gsu_ldw(R[3]);
    R[3] = gsu_inc(R[3]);
    R[3] = gsu_inc(R[3]);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.z;
    if (t) goto l95A6;
    R[4] = gsu_ldw(R[3]);
    R[3] = gsu_inc(R[3]);
    R[3] = gsu_inc(R[3]);
    R[0] = 0x7D00;
    gsu_sub(R[0], R[4], 0);   /* CMP */
    t = gsu.z;
    if (t) goto l95AD;
    R[4] = R[4];
    gsu_moves(R[4]);
    t = !gsu.s;
    if (t) goto l95B9;
    t = 1;
    if (t) goto l95AD;
l95A6:
    R[0] = 0x0008;
    R[3] = gsu_add(R[3], R[0], 0);
    t = 1;
    if (t) goto l9582;
l95AD:
    R[0] = 0x0006;
    R[3] = gsu_add(R[3], R[0], 0);
    t = 1;
    if (t) goto l9582;
l95B4:
    R[0] = 0x0000;
    nx = (uint16_t)(R[11]);
    at = nx;
    goto dispatch;
l95B9:
    R[0] = 0x0001;
    nx = (uint16_t)(R[11]);
    at = nx;
    goto dispatch;
    /* FxCollideModel */
l9C3D:
    gsu_code(0x049C3D, 3); gsu_code(0x04A9E8, 4);
    gsu_stw(R[10], R[11]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_code(0x049C40, 4);
    gsu_stw(R[10], R[8]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_code(0x049C44, 4);
    gsu_stw(R[10], R[9]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[0] = 0x0012;
    R[0] = gsu_add(R[0], R[7], 0);
    R[0] = gsu_ldw(gsu_code_then(0x049C48, 6, R[0]));
    gsu_code(0x049C4E, 3);
    gsu_stw(GSU_SPEED_44, R[0]);
    R[0] = 0x0000;
    R[1] = 0x7D00;
    gsu_code(0x049C51, 10);
    gsu_stw(FX_PUSHES, R[0]);
    gsu_code(0x049C5B, 4);
    gsu_stw((FX_PUSHES + 2), R[1]);
    gsu_code(0x049C5F, 4);
    gsu_stw((FX_PUSHES + 10), R[0]);
    gsu_code(0x049C63, 4);
    gsu_stw((FX_PUSHES + 12), R[1]);
    gsu_code(0x049C67, 4);
    gsu_stw((FX_PUSHES + 20), R[0]);
    gsu_code(0x049C6B, 4);
    gsu_stw((FX_PUSHES + 22), R[1]);
    gsu_code(0x049C6F, 4);
    gsu_stw((FX_PUSHES + 30), R[0]);
    gsu_code(0x049C73, 4);
    gsu_stw((FX_PUSHES + 32), R[1]);
    gsu_code(0x049C77, 4);
    gsu_stw((FX_PUSHES + 40), R[0]);
    gsu_code(0x049C7B, 4);
    gsu_stw((FX_PUSHES + 42), R[1]);
    gsu_code(0x049C7F, 4);
    gsu_stw((FX_PUSHES + 50), R[0]);
    gsu_code(0x049C83, 4);
    gsu_stw((FX_PUSHES + 52), R[1]);
    gsu_code(0x049C87, 4);
    gsu_stw((FX_PUSHES + 60), R[0]);
    gsu_code(0x049C8B, 4);
    gsu_stw((FX_PUSHES + 62), R[1]);
    gsu_code(0x049C8F, 4);
    gsu_stw((FX_PUSHES + 70), R[0]);
    gsu_code(0x049C93, 4);
    gsu_stw((FX_PUSHES + 72), R[1]);
    gsu_code(0x049C97, 4);
    gsu_stw((FX_PUSHES + 80), R[0]);
    gsu_code(0x049C9B, 4);
    gsu_stw((FX_PUSHES + 82), R[1]);
    gsu_code(0x049C9F, 4);
    gsu_stw((FX_PUSHES + 90), R[0]);
    gsu_code(0x049CA3, 4);
    gsu_stw((FX_PUSHES + 92), R[1]);
    gsu_code(0x049CA7, 4);
    gsu_stw((FX_PUSHES + 100), R[0]);
    gsu_code(0x049CAB, 4);
    gsu_stw((FX_PUSHES + 102), R[1]);
    gsu_code(0x049CAF, 4);
    gsu_stw((FX_PUSHES + 110), R[0]);
    gsu_code(0x049CB3, 4);
    gsu_stw((FX_PUSHES + 112), R[1]);
    gsu_code(0x049CB7, 4);
    gsu_stw((FX_PUSHES + 120), R[0]);
    gsu_code(0x049CBB, 4);
    gsu_stw((FX_PUSHES + 122), R[1]);
    gsu_code(0x049CBF, 4);
    gsu_stw((FX_PUSHES + 130), R[0]);
    gsu_code(0x049CC3, 4);
    gsu_stw((FX_PUSHES + 132), R[1]);
    gsu_code(0x049CC7, 4);
    gsu_stw((FX_PUSHES + 140), R[0]);
    gsu_code(0x049CCB, 4);
    gsu_stw((FX_PUSHES + 142), R[1]);
    gsu_code(0x049CCF, 4);
    gsu_stw((FX_PUSHES + 150), R[0]);
    gsu_code(0x049CD3, 4);
    gsu_stw((FX_PUSHES + 152), R[1]);
    gsu_code(0x049CD7, 4);
    gsu_stw((FX_PUSHES + 160), R[0]);
    gsu_code(0x049CDB, 4);
    gsu_stw((FX_PUSHES + 162), R[1]);
    gsu_code(0x049CDF, 4);
    gsu_stw((FX_PUSHES + 170), R[0]);
    gsu_code(0x049CE3, 4);
    gsu_stw((FX_PUSHES + 172), R[1]);
    gsu_code(0x049CE7, 4);
    gsu_stw((FX_PUSHES + 180), R[0]);
    gsu_code(0x049CEB, 4);
    gsu_stw((FX_PUSHES + 182), R[1]);
    R[0] = 0x0000;
    gsu_code(0x049CEF, 10);
    gsu_romb(R[0]);
    R[1] = 0x0014;
    R[1] = gsu_add(R[1], R[7], 0);
    R[1] = gsu_ldw(R[1]);
    R[1] = R[1];
    gsu_moves(R[1]);
    t = !gsu.z;
    if (t) { gsu_code(0x049CF9, 4); gsu_code(0x049D01, 3); goto l9D01; }
    goto lA145;
l9D01:
    gsu_r14(R[1]);
    R[0] = 0x000C;
    gsu_code(0x049D04, 5);
    gsu_r14(gsu_add(R[14], R[0], 0));
    R[0] = (gsu_code(0x049D09, 1), gsu_getb());
    gsu_code(0x049D0A, 1);
    gsu_r14(gsu_inc(R[14]));
    R[0] = gsu_getbh(gsu_code_then(0x049D0B, 2, R[0]));
    R[0] = R[0];
    gsu_moves(R[0]);
    t = !gsu.z;
    if (t) { gsu_code(0x049D0D, 4); gsu_code(0x049D15, 3); goto l9D15; }
    goto lA145;
l9D15:
    gsu_r14(R[1]);
    R[4] = 0x001E;
    R[4] = gsu_add(R[4], R[9], 0);
    R[4] = gsu_ldw(gsu_code_then(0x049D18, 6, R[4]));
    R[0] = 0x001E;
    R[0] = gsu_add(R[0], R[7], 0);
    R[0] = gsu_ldw(gsu_code_then(0x049D1E, 4, R[0]));
    R[4] = gsu_sub(R[4], R[0], 0);
    gsu_code(0x049D22, 5);
    gsu_stw(GSU_IN_20, R[4]);
    R[5] = 0x0020;
    R[5] = gsu_add(R[5], R[9], 0);
    R[5] = gsu_ldw(gsu_code_then(0x049D27, 6, R[5]));
    R[0] = 0x0020;
    R[0] = gsu_add(R[0], R[7], 0);
    R[0] = gsu_ldw(gsu_code_then(0x049D2D, 4, R[0]));
    R[5] = gsu_sub(R[5], R[0], 0);
    gsu_code(0x049D31, 5);
    gsu_stw(GSU_IN_22, R[5]);
    R[8] = 0x0022;
    R[8] = gsu_add(R[8], R[9], 0);
    R[8] = gsu_ldw(gsu_code_then(0x049D36, 6, R[8]));
    R[0] = 0x0022;
    R[0] = gsu_add(R[0], R[7], 0);
    R[0] = gsu_ldw(gsu_code_then(0x049D3C, 4, R[0]));
    R[8] = gsu_sub(R[8], R[0], 0);
    gsu_code(0x049D40, 5);
    gsu_stw(GSU_IN_24, R[8]);
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_code(0x049D45, 5);
    gsu_stw(FX_PUSH_TURN, R[0]);
    R[0] = 0x0024;
    R[0] = gsu_add(R[0], R[7], 0);
    R[0] = gsu_ldw(gsu_code_then(0x049D4A, 4, R[0]));
    R[0] = R[0];
    gsu_moves(R[0]);
    t = !gsu.z;
    if (t) { gsu_code(0x049D4E, 4); gsu_code(0x049D56, 6); goto l9D56; } else { gsu_code(0x049D4E, 8); }
    goto l9DEF;
l9D56:
    R[0] = gsu_hib(R[0]);
    gsu_stw(FX_PUSH_TURN, R[0]);
    R[7] = 0x00C0;
    gsu_sub(R[0], R[7], 0);   /* CMP */
    t = !gsu.z;
    if (t) { gsu_code(0x049D5C, 7); gsu_code(0x049D6F, 8); goto l9D6F; } else { gsu_code(0x049D5C, 19); }
    R[0] = R[8];
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
    R[8] = R[4];
    R[4] = R[0];
    goto l9DEF;
l9D6F:
    R[7] = 0x0040;
    gsu_sub(R[0], R[7], 0);   /* CMP */
    t = !gsu.z;
    if (t) { gsu_code(0x049D82, 17); goto l9D82; } else { gsu_code(0x049D77, 11); }
    R[0] = R[4];
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
    R[4] = R[8];
    R[8] = R[0];
    t = 1;
    if (t) goto l9DEF;
l9D82:
    R[7] = 0x0080;
    gsu_sub(R[0], R[7], 0);   /* CMP */
    t = !gsu.z;
    if (t) goto l9D93;
    R[4] = gsu_not(R[4]);
    R[4] = gsu_inc(R[4]);
    R[8] = gsu_not(R[8]);
    R[8] = gsu_inc(R[8]);
    t = 1;
    if (t) goto l9DEF;
l9D93:
    gsu_stw(R[10], R[14]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_stw(R[10], R[1]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_stw(R[10], R[2]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_stw(R[10], R[3]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_stw(R[10], R[5]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_stw(R[10], R[7]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_stw(R[10], R[9]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[1] = R[4];
    R[3] = R[8];
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
    R[2] = 0x00FF;
    R[2] = gsu_and(R[0], R[2]);
    R[0] = 0x0000;
    gsu_romb(R[0]);
    R[11] = 0x9DC3;
    abi_FxTurn();
l9DC3:
    R[1] = gsu_add(R[1], R[1], 0);
    R[1] = gsu_add(R[1], R[1], 0);
    R[3] = gsu_add(R[3], R[3], 0);
    R[3] = gsu_add(R[3], R[3], 0);
    R[4] = R[3];
    R[8] = R[1];
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    R[9] = gsu_ldw(R[10]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    R[7] = gsu_ldw(R[10]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    R[5] = gsu_ldw(R[10]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    R[3] = gsu_ldw(R[10]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    R[2] = gsu_ldw(R[10]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    R[1] = gsu_ldw(R[10]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_r14(gsu_ldw(R[10]));
    R[0] = 0x0000;
    gsu_romb(R[0]);
l9DEF:
    R[0] = 0x0003;
    gsu_code(0x049DEF, 6);
    gsu_r14(gsu_add(R[14], R[0], 0));
    R[2] = (gsu_code(0x049DF5, 2), gsu_getb());
    gsu_code(0x049DF7, 1);
    gsu_r14(gsu_inc(R[14]));
    R[2] = gsu_getbh(gsu_code_then(0x049DF8, 3, R[2]));
    R[0] = 0x0000;
    gsu_code(0x049DFB, 5);
    gsu_r14(R[1]);
    gsu_code(0x049E00, 2);
    gsu_r14(gsu_add(R[14], R[0], 0));
    R[9] = (gsu_code(0x049E02, 2), gsu_getb());
    gsu_code(0x049E04, 1);
    gsu_r14(gsu_inc(R[14]));
    R[9] = gsu_getbh(gsu_code_then(0x049E05, 3, R[9]));
    R[0] = 0x0007;
    gsu_code(0x049E08, 5);
    gsu_r14(R[1]);
    gsu_code(0x049E0D, 2);
    gsu_r14(gsu_add(R[14], R[0], 0));
    R[0] = (gsu_code(0x049E0F, 2), gsu_getbs());
    R[12] = R[0];
    R[0] = 0x0001;
    R[13] = 0x9E16;
l9E16:
    R[0] = gsu_add(R[0], R[0], 0);
    gsu_code(0x049E16, 3);
    t = gsu_count_down(&R[12]);
    if (t) goto l9E16; else { gsu_code(0x049E11, 5); gsu_code(0x049E19, 5); }
    gsu_stw(FX_FACE_SCALE, R[0]);
    R[0] = 0x0002;
    gsu_code(0x049E1E, 14);
    gsu_r14(R[1]);
    gsu_r14(gsu_add(R[14], R[0], 0));
    R[0] = gsu_getb();
    gsu_romb(R[0]);
    R[3] = gsu_ldw(GSU_SPEED_44);
    R[0] = gsu_ldw(gsu_code_then(0x049E2C, 4, FX_INIT_2380));
    gsu_sub(R[0], R[1], 0);   /* CMP */
    t = !gsu.z;
    if (t) { gsu_code(0x049E30, 4); gsu_code(0x049E3D, 5); goto l9E3D; } else { gsu_code(0x049E30, 9); }
    R[0] = gsu_ldw(FX_INIT_2382);
    gsu_sub(R[0], R[3], 0);   /* CMP */
    t = gsu.z;
    if (t) { gsu_code(0x049E39, 4); goto l9E4A; }
l9E3D:
    gsu_stw(FX_INIT_2380, R[1]);
    gsu_code(0x049E42, 4);
    gsu_stw(FX_INIT_2382, R[3]);
    R[11] = 0x9E4A;
    (gsu_code(0x049E46, 4), gsu_code(0x04A154, 3), abi_FxModelPoints());
l9E4A:
    R[9] = 0x2488;
    gsu_code(0x049E4A, 6);
    gsu_r14(R[2]);
    gsu_code(0x049E50, 1);
    gsu_r14(gsu_inc(R[14]));
    R[0] = (gsu_code(0x049E51, 1), gsu_getb());
    gsu_code(0x049E52, 1);
    gsu_r14(gsu_inc(R[14]));
    R[0] = gsu_umult(R[0], 3);
    gsu_code(0x049E53, 4);
    gsu_r14(gsu_add(R[14], R[0], 0));
    /* CACHE */
    R[0] = (gsu_code(0x049E57, 1), gsu_cache(0x9E57), gsu_code(0x049E58, 2), gsu_getbs());
    R[13] = 0x003C;
    gsu_sub(R[0], R[13], 0);   /* CMP */
    t = !gsu.z;
    gsu_code(0x049E5A, 6); gsu_code(0x049E92, 1); gsu_cache(0x9E92);
    gsu_r14(gsu_inc(R[14]));
    if (t) goto l9E92;
    R[1] = 0x0028;
    R[7] = 0x0005;
    R[2] = 0x0044;
    R[12] = 0x0003;
    R[3] = 0x0040;
l9E6A:
    R[0] = gsu_getbs();
    gsu_sub(R[0], R[3], 0);   /* CMP */
    t = !gsu.z;
    if (t) goto l9E74;
    t = 1;
    gsu_r14(gsu_inc(R[14]));
    if (t) goto l9E6A;
l9E74:
    gsu_sub(R[0], R[1], 0);   /* CMP */
    t = !gsu.z;
    if (t) goto l9E7D;
    t = 1;
    gsu_r14(gsu_add(R[14], R[7], 0));
    if (t) goto l9E6A;
l9E7D:
    gsu_sub(R[0], R[2], 0);   /* CMP */
    t = !gsu.z;
    if (t) goto l9E87;
    t = 1;
    gsu_r14(gsu_add(R[14], R[12], 0));
    if (t) goto l9E6A;
l9E85:
    R[0] = (gsu_code(0x049E86, 2), gsu_getbs());
gsu_code(0x049E88, 1);
l9E87:
    gsu_r14(gsu_inc(R[14]));
    R[0] = R[0];
    gsu_moves(R[0]);
    t = !gsu.z;
    if (t) goto l9E91; else { gsu_code(0x049E89, 8); gsu_code(0x04A0F8, 8); }
    goto lA0F8;
l9E91:
    /* CACHE */
l9E92:
    R[13] = (gsu_code(0x049E93, 3), gsu_getbs());
    R[13] = R[13];
    gsu_moves(R[13]);
    t = gsu.s;
    gsu_code(0x049E92, 1); gsu_cache(0x9E92); gsu_code(0x049E96, 4);
    gsu_r14(gsu_inc(R[14]));
    if (t) goto l9E85; else { gsu_code(0x049E9B, 4); }
    R[0] = gsu_sub(R[0], R[0], 0);
    R[11] = gsu_getbs();
    gsu_code(0x049E9F, 1);
    gsu_r14(gsu_inc(R[14]));
    R[11] = R[11];
    gsu_moves(R[11]);
    t = !gsu.z;
    if (t) { gsu_code(0x049EA0, 4); gsu_code(0x049EA8, 7); goto l9EA8; }
    goto lA0EF;
l9EA8:
    R[0] = R[11];
    gsu_stw(FX_FACE_FLAGS, R[0]);
    R[1] = 0x00E0;
    R[11] = gsu_bic(R[11], R[1]);
    R[0] = 0x000A;
    R[11] = gsu_mult(R[0], R[11]);
    R[0] = 0x23A4;
    R[11] = gsu_add(R[11], R[0], 0);
    R[0] = gsu_ldw(gsu_code_then(0x049EAF, 16, R[11]));
    R[0] = gsu_hib(R[0]);
    t = gsu.z;
    if (t) { gsu_code(0x049EBF, 3); gsu_code(0x049EC6, 2); goto l9EC6; } else { gsu_code(0x049EBF, 7); gsu_code(0x04A0EF, 3); }
    goto lA0EF;
l9EC6:
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x049EC8, 1);
    gsu_r14(gsu_inc(R[14]));
    R[0] = gsu_ldw(gsu_code_then(0x049EC9, 3, GSU_SPEED_44));
    R[0] = R[0];
    gsu_moves(R[0]);
    t = !gsu.z;
    if (t) goto l9ED4; else { gsu_code(0x049ECC, 8); gsu_code(0x04A013, 2); }
    goto lA013;
l9ED4:
    R[7] = gsu_ldw(FX_FACE_FLAGS);
    R[0] = 0x0040;
    R[0] = gsu_and(R[0], R[7]);
    t = !gsu.z;
    if (t) goto l9EE3;
    goto lA013;
l9EE3:
    gsu_r14(gsu_add(R[14], 3, 0));
    gsu_stw(R[10], R[14]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[12] = R[9];
    R[0] = gsu_getb();
    gsu_r14(gsu_inc(R[14]));
    R[0] = gsu_umult(R[0], 6);
    R[9] = gsu_add(R[0], R[12], 0);
    R[0] = gsu_ldw(R[9]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    gsu_stw(GSU_IN_68, R[0]);
    R[0] = gsu_ldw(R[9]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    gsu_stw(GSU_DEST, R[0]);
    R[0] = gsu_ldw(R[9]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    gsu_stw(GSU_IN_2E, R[0]);
    R[0] = gsu_getb();
    gsu_r14(gsu_inc(R[14]));
    R[0] = gsu_umult(R[0], 6);
    R[9] = gsu_add(R[0], R[12], 0);
    R[0] = gsu_ldw(R[9]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    gsu_stw(GSU_IN_6A, R[0]);
    R[0] = gsu_ldw(R[9]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    gsu_stw(GSU_IN_A2, R[0]);
    R[0] = gsu_ldw(R[9]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    gsu_stw(GSU_IN_A4, R[0]);
    R[0] = gsu_getb();
    gsu_r14(gsu_inc(R[14]));
    R[0] = gsu_umult(R[0], 6);
    R[9] = gsu_add(R[0], R[12], 0);
    R[1] = gsu_ldw(R[9]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    R[2] = gsu_ldw(R[9]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    R[3] = gsu_ldw(R[9]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    R[0] = gsu_ldw(GSU_IN_68);
    R[9] = gsu_ldw(GSU_IN_6A);
    R[0] = gsu_sub(R[0], R[9], 0);
    gsu_stw(GSU_IN_68, R[0]);
    R[0] = gsu_ldw(GSU_DEST);
    R[9] = gsu_ldw(GSU_IN_A2);
    R[0] = gsu_sub(R[0], R[9], 0);
    gsu_stw(GSU_DEST, R[0]);
    R[0] = gsu_ldw(GSU_IN_2E);
    R[9] = gsu_ldw(GSU_IN_A4);
    R[0] = gsu_sub(R[0], R[9], 0);
    gsu_stw(GSU_IN_2E, R[0]);
    R[0] = gsu_ldw(GSU_IN_6A);
    R[0] = gsu_sub(R[0], R[1], 0);
    gsu_stw(GSU_IN_6A, R[0]);
    R[0] = gsu_ldw(GSU_IN_A2);
    R[0] = gsu_sub(R[0], R[2], 0);
    gsu_stw(GSU_IN_A2, R[0]);
    R[0] = gsu_ldw(GSU_IN_A4);
    R[0] = gsu_sub(R[0], R[3], 0);
    gsu_stw(GSU_IN_A4, R[0]);
    gsu_stw(R[10], R[4]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_stw(R[10], R[5]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_stw(R[10], R[8]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_stw(R[10], R[12]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_stw(R[10], R[13]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_stw(R[10], R[11]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[1] = gsu_ldw(GSU_IN_68);
    R[2] = gsu_ldw(GSU_DEST);
    R[3] = gsu_ldw(GSU_IN_2E);
    R[11] = 0x9F87;
    abi_FxNormalize();
l9F87:
    gsu_stw(GSU_IN_68, R[1]);
    gsu_stw(GSU_DEST, R[2]);
    gsu_stw(GSU_IN_2E, R[3]);
    R[1] = gsu_ldw(GSU_IN_6A);
    R[2] = gsu_ldw(GSU_IN_A2);
    R[3] = gsu_ldw(GSU_IN_A4);
    R[11] = 0x9F9E;
    abi_FxNormalize();
l9F9E:
    R[6] = gsu_ldw(GSU_DEST);
    { uint32_t p = gsu_lmult(R[3], R[6]); R[4] = (uint16_t)p; R[0] = (uint16_t)(p >> 16); }
    R[7] = R[4];
    R[6] = gsu_ldw(GSU_IN_2E);
    { uint32_t p = gsu_lmult(R[2], R[6]); R[4] = (uint16_t)p; R[5] = (uint16_t)(p >> 16); }
    R[7] = gsu_sub(R[7], R[4], 0);
    R[0] = gsu_sub(R[0], R[5], !gsu.cy);
    R[7] = gsu_add(R[7], R[7], 0);
    R[0] = gsu_rol(R[0]);
    gsu_stw(GSU_IN_6A, R[0]);
    R[6] = gsu_ldw(GSU_IN_2E);
    { uint32_t p = gsu_lmult(R[1], R[6]); R[4] = (uint16_t)p; R[0] = (uint16_t)(p >> 16); }
    R[7] = R[4];
    R[6] = gsu_ldw(GSU_IN_68);
    { uint32_t p = gsu_lmult(R[3], R[6]); R[4] = (uint16_t)p; R[5] = (uint16_t)(p >> 16); }
    R[7] = gsu_sub(R[7], R[4], 0);
    R[0] = gsu_sub(R[0], R[5], !gsu.cy);
    R[7] = gsu_add(R[7], R[7], 0);
    R[0] = gsu_rol(R[0]);
    gsu_stw(GSU_IN_A2, R[0]);
    R[6] = gsu_ldw(GSU_IN_68);
    { uint32_t p = gsu_lmult(R[2], R[6]); R[4] = (uint16_t)p; R[0] = (uint16_t)(p >> 16); }
    R[7] = R[4];
    R[6] = gsu_ldw(GSU_DEST);
    { uint32_t p = gsu_lmult(R[1], R[6]); R[4] = (uint16_t)p; R[5] = (uint16_t)(p >> 16); }
    R[7] = gsu_sub(R[7], R[4], 0);
    R[0] = gsu_sub(R[0], R[5], !gsu.cy);
    R[7] = gsu_add(R[7], R[7], 0);
    R[0] = gsu_rol(R[0]);
    gsu_stw(GSU_IN_A4, R[0]);
    R[1] = gsu_ldw(GSU_IN_6A);
    R[2] = gsu_ldw(GSU_IN_A2);
    R[3] = gsu_ldw(GSU_IN_A4);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    R[11] = gsu_ldw(R[10]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    R[13] = gsu_ldw(R[10]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    R[12] = gsu_ldw(R[10]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    R[8] = gsu_ldw(R[10]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    R[5] = gsu_ldw(R[10]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    R[4] = gsu_ldw(R[10]);
    R[9] = R[12];
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_r14(gsu_ldw(R[10]));
    t = 1;
    if (t) goto lA028;
lA013:
    R[0] = gsu_getb();
    gsu_code(0x04A015, 1);
    gsu_r14(gsu_inc(R[14]));
    R[1] = gsu_swap(R[0]);
    R[1] = gsu_not(R[1]);
    R[1] = gsu_inc(R[1]);
    R[0] = (gsu_code(0x04A016, 6), gsu_getb());
    gsu_code(0x04A01C, 1);
    gsu_r14(gsu_inc(R[14]));
    R[2] = gsu_swap(R[0]);
    R[2] = gsu_not(R[2]);
    R[2] = gsu_inc(R[2]);
    R[0] = (gsu_code(0x04A01D, 6), gsu_getb());
    gsu_code(0x04A023, 1);
    gsu_r14(gsu_inc(R[14]));
    R[3] = gsu_swap(R[0]);
    R[3] = gsu_not(R[3]);
    R[3] = gsu_inc(R[3]);
lA028:
    R[7] = gsu_ldw(gsu_code_then(0x04A024, 9, FX_FACE_SCALE));
    R[0] = (gsu_code(0x04A02D, 2), gsu_getbs());
    gsu_code(0x04A02F, 2);
    gsu_r14(gsu_add(R[14], R[13], 0));
    R[13] = R[9];
    R[0] = gsu_umult(R[0], 6);
    R[9] = gsu_add(R[9], R[0], 0);
    R[0] = gsu_ldw(gsu_code_then(0x04A031, 7, R[9]));
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    R[0] = gsu_mult(R[0], R[7]);
    gsu_code(0x04A038, 7);
    gsu_stw(FX_FACE_NORMAL, R[0]);
    R[0] = gsu_ldw(gsu_code_then(0x04A03F, 1, R[9]));
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    R[0] = gsu_mult(R[0], R[7]);
    gsu_code(0x04A040, 7);
    gsu_stw((FX_FACE_NORMAL + 2), R[0]);
    R[0] = gsu_ldw(gsu_code_then(0x04A047, 1, R[9]));
    R[0] = gsu_mult(R[0], R[7]);
    gsu_code(0x04A048, 5);
    gsu_stw((FX_FACE_NORMAL + 4), R[0]);
    R[9] = R[13];
    R[0] = gsu_ldw(gsu_code_then(0x04A04D, 3, R[11]));
    R[0] = gsu_hib(R[0]);
    t = gsu.z;
    if (t) { gsu_code(0x04A050, 3); gsu_code(0x04A057, 21); goto lA057; }
lA053:
    goto l9E92;
lA057:
    R[6] = R[4];
    R[7] = gsu_fmult(R[1], R[6]);
    R[6] = R[5];
    R[0] = gsu_fmult(R[2], R[6]);
    R[7] = gsu_add(R[7], R[0], 0);
    R[6] = R[8];
    R[0] = gsu_fmult(R[3], R[6]);
    R[0] = gsu_add(R[0], R[7], 0);
    gsu_stw(FX_FACE_PLANE, R[0]);
    R[6] = gsu_ldw(gsu_code_then(0x04A06C, 4, FX_FACE_NORMAL));
    R[7] = gsu_fmult(R[1], R[6]);
    R[6] = gsu_ldw(gsu_code_then(0x04A070, 7, (FX_FACE_NORMAL + 2)));
    R[0] = gsu_fmult(R[2], R[6]);
    R[7] = gsu_add(R[7], R[0], 0);
    R[6] = gsu_ldw(gsu_code_then(0x04A077, 8, (FX_FACE_NORMAL + 4)));
    R[0] = gsu_fmult(R[3], R[6]);
    R[0] = gsu_add(R[0], R[7], 0);
    R[7] = gsu_ldw(gsu_code_then(0x04A07F, 7, FX_FACE_PLANE));
    R[0] = gsu_sub(R[0], R[7], 0);
    t = !gsu.s;
    if (t) { gsu_code(0x04A086, 3); gsu_code(0x04A09C, 5); goto lA09C; } else { gsu_code(0x04A086, 10); }
    R[12] = R[0];
    R[0] = gsu_ldw(FX_FACE_FLAGS);
    R[0] = gsu_swap(R[0]);
    R[7] = gsu_ldw(gsu_code_then(0x04A090, 3, R[11]));
    R[0] = gsu_or(R[0], R[7]);
    R[7] = 0xFF00;
    R[0] = gsu_and(R[0], R[7]);
    gsu_code(0x04A093, 6);
    gsu_sbk(R[0]);
    goto l9E92;
lA09C:
    R[7] = gsu_ldw(FX_FACE_FLAGS);
    R[12] = 0x0080;
    R[12] = gsu_and(R[12], R[7]);
    t = gsu.z;
    if (t) { gsu_code(0x04A0A1, 7); gsu_code(0x04A0B8, 5); goto lA0B8; } else { gsu_code(0x04A0A1, 12); }
    R[0] = gsu_ldw(FX_FACE_FLAGS);
    R[7] = 0x00FF;
    R[7] = gsu_and(R[0], R[7]);
    R[0] = gsu_ldw(gsu_code_then(0x04A0AD, 6, R[11]));
    R[0] = gsu_or(R[0], R[7]);
    gsu_code(0x04A0B3, 2);
    gsu_sbk(R[0]);
    goto l9E92;
lA0B8:
    R[11] = gsu_inc(R[11]);
    R[11] = gsu_inc(R[11]);
    R[12] = gsu_ldw(R[11]);
    R[7] = R[12];
    gsu_moves(R[12]);
    t = !gsu.s;
    if (t) { gsu_code(0x04A0BD, 4); gsu_code(0x04A0C4, 5); gsu_code(0x04A0CC, 6); goto lA0C4; }
    R[7] = gsu_not(R[7]);
    R[7] = gsu_inc(R[7]);
lA0C4:
    R[12] = R[0];
    gsu_moves(R[0]);
    t = !gsu.s;
    if (t) goto lA0CC;
    R[12] = gsu_not(R[12]);
    R[12] = gsu_inc(R[12]);
lA0CC:
    gsu_sub(R[7], R[12], 0);   /* CMP */
    t = !gsu.s;
    if (t) { gsu_code(0x04A0D6, 2); goto lA0D6; } else { gsu_code(0x049E92, 1); gsu_cache(0x9E92); gsu_code(0x04A0D2, 4); }
    goto l9E92;
lA0D6:
    gsu_stw(R[11], R[0]);
    R[11] = gsu_dec(R[11]);
    R[11] = gsu_dec(R[11]);
    R[0] = gsu_ldw(gsu_code_then(0x04A0D8, 6, FX_FACE_FLAGS));
    gsu_code(0x04A0DE, 1);
    gsu_stw(R[11], R[0]);
    R[11] = gsu_add(R[11], 4, 0);
    gsu_code(0x04A0DF, 5);
    gsu_stw(R[11], R[1]);
    R[11] = gsu_inc(R[11]);
    R[11] = gsu_inc(R[11]);
    gsu_code(0x04A0E4, 4);
    gsu_stw(R[11], R[2]);
    R[11] = gsu_inc(R[11]);
    R[11] = gsu_inc(R[11]);
    gsu_code(0x04A0E8, 4);
    gsu_stw(R[11], R[3]);
    goto l9E92;
lA0EF:
    gsu_r14(gsu_add(R[14], R[13], 0));
    gsu_code(0x04A0F2, 3);
    gsu_r14(gsu_add(R[14], 5, 0));
    goto l9E92;
lA0F8:
    R[11] = 0x23A4;
    R[12] = 0x0013;
    R[13] = 0xA100;
lA100:
    R[11] = gsu_add(R[11], 10, 0);
    R[0] = gsu_ldw(gsu_code_then(0x04A100, 5, R[11]));
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.z;
    if (t) goto lA10E; else { gsu_code(0x04A109, 5); }
    R[7] = gsu_hib(R[0]);
    t = gsu.z;
    if (t) { gsu_code(0x04A115, 2); goto lA115; }
lA10E:
    gsu_code(0x04A10E, 2);
    t = gsu_count_down(&R[12]);
    if (t) { gsu_code(0x04A105, 4); goto lA100; } else { gsu_code(0x04A110, 5); }
    R[0] = 0x0000;
    t = 1;
    if (t) goto lA117;
lA115:
    R[0] = 0x0001;
lA117:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    R[9] = gsu_ldw(gsu_code_then(0x04A105, 4, gsu_code_then(0x04A117, 5, R[10])));
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    R[8] = gsu_ldw(gsu_code_then(0x04A11C, 4, R[10]));
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x04A120, 4);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
    /* FxCollideModelUnused */
lA124:
    t = 1;
    if (t) goto lA0F8;
lA127:
    R[11] = 0x23A4;
    R[0] = 0x001E;
    R[0] = gsu_add(R[0], R[9], 0);
    R[0] = gsu_ldw(R[0]);
    R[2] = 0x0022;
    R[2] = gsu_add(R[2], R[9], 0);
    R[2] = gsu_ldw(R[2]);
    R[0] = gsu_add(R[0], R[2], 0);
    R[2] = 0x007F;
    R[0] = gsu_and(R[0], R[2]);
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
    R[2] = 0x0020;
    R[2] = gsu_add(R[2], R[9], 0);
    R[2] = gsu_ldw(R[2]);
    R[0] = gsu_add(R[0], R[2], 0);
    gsu_stw(R[11], R[0]);
    t = 1;
    if (t) goto lA0F8;
lA145:
    R[0] = 0x0000;
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    R[9] = gsu_ldw(R[10]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    R[8] = gsu_ldw(R[10]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
    /* FxPartsUpdate */
lA607:
    gsu_stw(R[10], R[11]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[1] = 0x0000;
    R[0] = 0x0044;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04A60A, 9);
    gsu_stw(R[0], R[1]);
    R[0] = 0x0046;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04A613, 5);
    gsu_stw(R[0], R[1]);
    R[0] = 0x0056;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04A618, 5);
    gsu_stw(R[0], R[1]);
    R[0] = 0x003E;
    R[0] = gsu_add(R[0], R[8], 0);
    R[0] = gsu_ldw(gsu_code_then(0x04A61D, 4, R[0]));
lA620:
    R[9] = R[0];
    gsu_code(0x04A621, 4);
    gsu_moves(R[0]);
    t = gsu.z;
    if (t) { gsu_code(0x04A651, 5); goto lA651; } else { gsu_code(0x04A625, 5); }
    R[0] = 0x0046;
    R[0] = gsu_add(R[0], R[9], 0);
    R[0] = gsu_ldw(R[0]);
    R[1] = 0x0800;
    R[0] = gsu_and(R[0], R[1]);
    t = gsu.z;
    if (t) { gsu_code(0x04A62A, 6); gsu_code(0x04A638, 5); gsu_code(0x04A893, 3); goto lA638; }
    R[11] = 0xA635;
    goto lA656;
lA635:
    t = 1;
    if (t) goto lA63D;
lA638:
    R[11] = 0xA63D;
    goto lA893;
lA63D:
    R[1] = 0x002C;
    R[1] = gsu_add(R[1], R[9], 0);
    R[1] = gsu_ldw(gsu_code_then(0x04A63D, 7, R[1]));
    R[0] = 0x0092;
    R[0] = gsu_add(R[0], R[8], 0);
    R[0] = gsu_ldw(gsu_code_then(0x04A644, 5, R[0]));
    R[0] = gsu_or(R[0], R[1]);
    gsu_code(0x04A649, 2);
    gsu_sbk(R[0]);
    R[0] = 0x0034;
    R[0] = gsu_add(R[0], R[9], 0);
    R[0] = gsu_ldw(gsu_code_then(0x04A64B, 4, R[0]));
    t = 1;
    if (t) { gsu_code(0x04A620, 1); gsu_code(0x04A64F, 2); goto lA620; }
lA651:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
    /* FxPartPull */
lA656:
    R[0] = 0x0064;
    R[0] = gsu_add(R[0], R[8], 0);
    R[0] = gsu_ldw(R[0]);
    R[4] = 0x0001;
    R[0] = gsu_and(R[0], R[4]);
    t = gsu.z;
    if (t) goto lA665;
    goto lA7B4;
lA665:
    gsu_stw(R[10], R[11]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[1] = 0x0046;
    R[1] = gsu_add(R[1], R[9], 0);
    R[1] = gsu_ldw(R[1]);
    R[0] = 0x0004;
    R[0] = gsu_and(R[0], R[1]);
    t = !gsu.z;
    if (t) goto lA681;
    R[0] = 0x0004;
    R[0] = gsu_or(R[0], R[1]);
    gsu_sbk(R[0]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
lA681:
    R[2] = 0x0800;
    R[7] = 0x008E;
    R[7] = gsu_add(R[7], R[8], 0);
    R[7] = gsu_ldw(R[7]);
    R[1] = 0x0028;
    R[1] = gsu_add(R[1], R[7], 0);
    R[1] = gsu_ldw(R[1]);
    R[0] = 0x0028;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_stw(R[0], R[1]);
    R[0] = 0x003E;
    R[0] = gsu_add(R[0], R[7], 0);
    R[0] = gsu_ldw(R[0]);
lA69A:
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.z;
    if (t) goto lA6B0;
    R[7] = R[0];
    R[0] = 0x0046;
    R[0] = gsu_add(R[0], R[7], 0);
    R[0] = gsu_ldw(R[0]);
    R[0] = gsu_and(R[0], R[2]);
    t = !gsu.z;
    if (t) goto lA6B0;
    R[0] = 0x0034;
    R[0] = gsu_add(R[0], R[7], 0);
    R[0] = gsu_ldw(R[0]);
    t = 1;
    if (t) goto lA69A;
lA6B0:
    R[1] = 0x001E;
    R[1] = gsu_add(R[1], R[7], 0);
    R[1] = gsu_ldw(R[1]);
    R[2] = 0x0020;
    R[2] = gsu_add(R[2], R[7], 0);
    R[2] = gsu_ldw(R[2]);
    R[3] = 0x0022;
    R[3] = gsu_add(R[3], R[7], 0);
    R[3] = gsu_ldw(R[3]);
    R[0] = 0x001E;
    R[0] = gsu_add(R[0], R[9], 0);
    R[0] = gsu_ldw(R[0]);
    R[1] = gsu_sub(R[1], R[0], 0);
    R[0] = 0x0020;
    R[0] = gsu_add(R[0], R[9], 0);
    R[0] = gsu_ldw(R[0]);
    R[2] = gsu_sub(R[2], R[0], 0);
    R[0] = 0x0022;
    R[0] = gsu_add(R[0], R[9], 0);
    R[0] = gsu_ldw(R[0]);
    R[3] = gsu_sub(R[3], R[0], 0);
    gsu_stw(FX_RAM_TEST, R[3]);
    R[4] = 0x00C8;
    R[0] = R[1];
    gsu_moves(R[1]);
    t = !gsu.s;
    if (t) goto lA6E2;
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
lA6E2:
    gsu_sub(R[0], R[4], 0);   /* CMP */
    t = !gsu.s;
    if (t) goto lA6E9;
    R[1] = 0x0000;
lA6E9:
    R[0] = R[2];
    gsu_moves(R[2]);
    t = !gsu.s;
    if (t) goto lA6F0;
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
lA6F0:
    gsu_sub(R[0], R[4], 0);   /* CMP */
    t = !gsu.s;
    if (t) goto lA6F7;
    R[2] = 0x0000;
lA6F7:
    R[0] = R[3];
    gsu_moves(R[3]);
    t = !gsu.s;
    if (t) goto lA6FE;
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
lA6FE:
    gsu_sub(R[0], R[4], 0);   /* CMP */
    t = !gsu.s;
    if (t) goto lA705;
    R[3] = 0x0000;
lA705:
    R[1] = gsu_add(R[1], R[1], 0);
    R[2] = gsu_add(R[2], R[2], 0);
    R[3] = gsu_add(R[3], R[3], 0);
    R[1] = gsu_add(R[1], R[1], 0);
    R[2] = gsu_add(R[2], R[2], 0);
    R[3] = gsu_add(R[3], R[3], 0);
    R[4] = 0x002E;
    R[4] = gsu_add(R[4], R[7], 0);
    R[4] = gsu_ldw(R[4]);
    R[5] = 0x0030;
    R[5] = gsu_add(R[5], R[7], 0);
    R[5] = gsu_ldw(R[5]);
    R[6] = 0x0032;
    R[6] = gsu_add(R[6], R[7], 0);
    R[6] = gsu_ldw(R[6]);
    R[5] = 0x0000;
    R[4] = gsu_add(R[4], R[4], 0);
    R[5] = gsu_add(R[5], R[5], 0);
    R[6] = gsu_add(R[6], R[6], 0);
    R[4] = gsu_add(R[4], R[4], 0);
    R[5] = gsu_add(R[5], R[5], 0);
    R[6] = gsu_add(R[6], R[6], 0);
    R[0] = 0x002E;
    R[0] = gsu_add(R[0], R[9], 0);
    R[0] = gsu_ldw(R[0]);
    R[7] = gsu_xor(R[0], R[1]);
    t = !gsu.s;
    if (t) goto lA73D;
    R[1] = gsu_sub(R[1], R[0], 0);
lA73D:
    R[0] = 0x0030;
    R[0] = gsu_add(R[0], R[9], 0);
    R[0] = gsu_ldw(R[0]);
    R[7] = gsu_xor(R[0], R[2]);
    t = !gsu.s;
    if (t) goto lA749;
    R[2] = gsu_sub(R[2], R[0], 0);
lA749:
    R[0] = 0x0032;
    R[0] = gsu_add(R[0], R[9], 0);
    R[0] = gsu_ldw(R[0]);
    gsu_stw(FX_PART_Z_SPEED, R[0]);
    R[7] = gsu_xor(R[0], R[3]);
    t = !gsu.s;
    if (t) goto lA759;
    R[2] = gsu_sub(R[2], R[0], 0);
lA759:
    R[1] = gsu_add(R[1], R[4], 0);
    R[2] = gsu_add(R[2], R[5], 0);
    R[3] = gsu_add(R[3], R[6], 0);
    R[4] = R[1];
    R[5] = R[2];
    R[6] = R[3];
    R[11] = 0xA76A;
    abi_FxSumsPushWorld();
lA76A:
    R[1] = 0x0036;
    R[1] = gsu_add(R[1], R[9], 0);
    R[1] = gsu_ldw(R[1]);
    R[2] = 0x0038;
    R[2] = gsu_add(R[2], R[9], 0);
    R[2] = gsu_ldw(R[2]);
    R[3] = 0x003A;
    R[3] = gsu_add(R[3], R[9], 0);
    R[3] = gsu_ldw(R[3]);
    R[1] = 0x0000;
    R[2] = 0x0000;
    R[3] = 0x0000;
    R[11] = 0xA787;
    abi_FxHitAddUnit();
lA787:
    R[1] = 0x003C;
    R[1] = gsu_add(R[1], R[9], 0);
    R[1] = gsu_ldw(R[1]);
    R[2] = 0x003E;
    R[2] = gsu_add(R[2], R[9], 0);
    R[2] = gsu_ldw(R[2]);
    R[3] = 0x0040;
    R[3] = gsu_add(R[3], R[9], 0);
    R[3] = gsu_ldw(R[3]);
    R[0] = 0x0004;
    gsu_romb(R[0]);
    R[0] = gsu_ldw(FX_PARAM_PUSH);
    gsu_r14(0x9BC6);
    gsu_r14(gsu_add(R[14], R[0], 0));
    R[0] = gsu_getb();
    gsu_r14(gsu_inc(R[14]));
    R[0] = gsu_getbh(R[0]);
    R[12] = R[0];
    R[11] = 0xA7AF;
    nx = (uint16_t)(R[12]);
    at = nx;
    goto dispatch;
lA7AF:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
lA7B4:
    R[1] = gsu_ldw(FX_HIT);
    R[0] = 0x002E;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_stw(R[0], R[1]);
    R[1] = gsu_ldw((FX_HIT + 2));
    R[0] = 0x0030;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_stw(R[0], R[1]);
    R[1] = gsu_ldw((FX_HIT + 4));
    R[0] = 0x0032;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_stw(R[0], R[1]);
    gsu_stw(R[10], R[11]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[1] = 0x0046;
    R[1] = gsu_add(R[1], R[9], 0);
    R[1] = gsu_ldw(R[1]);
    R[0] = 0x0004;
    R[0] = gsu_and(R[0], R[1]);
    t = !gsu.z;
    if (t) goto lA7EA;
    R[0] = 0x0004;
    R[0] = gsu_or(R[0], R[1]);
    gsu_sbk(R[0]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
lA7EA:
    R[2] = 0x0800;
    R[7] = 0x008E;
    R[7] = gsu_add(R[7], R[8], 0);
    R[7] = gsu_ldw(R[7]);
    R[0] = 0x003E;
    R[0] = gsu_add(R[0], R[7], 0);
    R[0] = gsu_ldw(R[0]);
lA7F8:
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.z;
    if (t) goto lA80E;
    R[7] = R[0];
    R[0] = 0x0046;
    R[0] = gsu_add(R[0], R[7], 0);
    R[0] = gsu_ldw(R[0]);
    R[0] = gsu_and(R[0], R[2]);
    t = !gsu.z;
    if (t) goto lA80E;
    R[0] = 0x0034;
    R[0] = gsu_add(R[0], R[7], 0);
    R[0] = gsu_ldw(R[0]);
    t = 1;
    if (t) goto lA7F8;
lA80E:
    R[1] = 0x001E;
    R[1] = gsu_add(R[1], R[7], 0);
    R[1] = gsu_ldw(R[1]);
    R[2] = 0x0020;
    R[2] = gsu_add(R[2], R[7], 0);
    R[2] = gsu_ldw(R[2]);
    R[3] = 0x0022;
    R[3] = gsu_add(R[3], R[7], 0);
    R[3] = gsu_ldw(R[3]);
    R[0] = 0x001E;
    R[0] = gsu_add(R[0], R[9], 0);
    R[0] = gsu_ldw(R[0]);
    R[1] = gsu_sub(R[1], R[0], 0);
    R[0] = 0x0020;
    R[0] = gsu_add(R[0], R[9], 0);
    R[0] = gsu_ldw(R[0]);
    R[2] = gsu_sub(R[2], R[0], 0);
    R[0] = 0x0022;
    R[0] = gsu_add(R[0], R[9], 0);
    R[0] = gsu_ldw(R[0]);
    R[3] = gsu_sub(R[3], R[0], 0);
    R[1] = gsu_add(R[1], R[1], 0);
    R[2] = gsu_add(R[2], R[2], 0);
    R[3] = gsu_add(R[3], R[3], 0);
    R[1] = gsu_add(R[1], R[1], 0);
    R[2] = gsu_add(R[2], R[2], 0);
    R[3] = gsu_add(R[3], R[3], 0);
    R[4] = R[1];
    R[5] = R[2];
    R[6] = R[3];
    R[11] = 0xA849;
    abi_FxSumsPushWorld();
lA849:
    R[1] = 0x0036;
    R[1] = gsu_add(R[1], R[9], 0);
    R[1] = gsu_ldw(R[1]);
    R[2] = 0x0038;
    R[2] = gsu_add(R[2], R[9], 0);
    R[2] = gsu_ldw(R[2]);
    R[3] = 0x003A;
    R[3] = gsu_add(R[3], R[9], 0);
    R[3] = gsu_ldw(R[3]);
    R[1] = 0x0000;
    R[2] = 0x0000;
    R[3] = 0x0000;
    R[11] = 0xA866;
    abi_FxHitAddUnit();
lA866:
    R[1] = 0x003C;
    R[1] = gsu_add(R[1], R[9], 0);
    R[1] = gsu_ldw(R[1]);
    R[2] = 0x003E;
    R[2] = gsu_add(R[2], R[9], 0);
    R[2] = gsu_ldw(R[2]);
    R[3] = 0x0040;
    R[3] = gsu_add(R[3], R[9], 0);
    R[3] = gsu_ldw(R[3]);
    R[0] = 0x0004;
    gsu_romb(R[0]);
    R[0] = gsu_ldw(FX_PARAM_PUSH);
    gsu_r14(0x9BC6);
    gsu_r14(gsu_add(R[14], R[0], 0));
    R[0] = gsu_getb();
    gsu_r14(gsu_inc(R[14]));
    R[0] = gsu_getbh(R[0]);
    R[12] = R[0];
    R[11] = 0xA88E;
    nx = (uint16_t)(R[12]);
    at = nx;
    goto dispatch;
lA88E:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
    /* FxWheelUpdate */
lA893:
    gsu_stw(R[10], R[11]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[0] = 0x0046;
    R[0] = gsu_add(R[0], R[9], 0);
    R[0] = gsu_ldw(gsu_code_then(0x04A896, 6, R[0]));
    gsu_code(0x04A89C, 4);
    gsu_stw(FX_WHEEL_FLAGS, R[0]);
    R[1] = 0x0000;
    R[0] = 0x002C;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_code(0x04A8A0, 7);
    gsu_stw(R[0], R[1]);
    R[3] = 0x0400;
    R[2] = 0x0040;
    R[2] = gsu_add(R[2], R[8], 0);
    R[2] = gsu_ldw(gsu_code_then(0x04A8A7, 9, R[2]));
    R[1] = gsu_ldw(gsu_code_then(0x04A8B0, 4, FX_WHEEL_FLAGS));
    R[1] = gsu_bic(R[1], R[3]);
    R[0] = 0x0200;
    R[0] = gsu_and(R[0], R[1]);
    t = gsu.z;
    if (t) { gsu_code(0x04A8B4, 9); gsu_code(0x04A8CC, 3); goto lA8CC; }
    R[0] = 0x0000;
    R[0] = gsu_and(R[0], R[2]);
    t = !gsu.z;
    if (t) goto lA8CC;
    R[1] = gsu_or(R[1], R[3]);
    gsu_sbk(R[1]);
    goto lAED7;
lA8CC:
    gsu_sbk(R[1]);
    R[1] = 0x0024;
    R[1] = gsu_add(R[1], R[8], 0);
    R[1] = gsu_ldw(gsu_code_then(0x04A8CF, 6, R[1]));
    R[0] = 0x0024;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_code(0x04A8D5, 5);
    gsu_stw(R[0], R[1]);
    R[1] = 0x0028;
    R[1] = gsu_add(R[1], R[8], 0);
    R[1] = gsu_ldw(gsu_code_then(0x04A8DA, 6, R[1]));
    R[0] = 0x0028;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_code(0x04A8E0, 5);
    gsu_stw(R[0], R[1]);
    R[2] = gsu_ldw(gsu_code_then(0x04A8E5, 4, FX_WHEEL_FLAGS));
    R[0] = 0x0002;
    R[0] = gsu_and(R[0], R[2]);
    t = gsu.z;
    if (t) { gsu_code(0x04A8E9, 6); gsu_code(0x04A8FB, 1); goto lA8FB; } else { gsu_code(0x04A8E9, 11); }
    R[0] = 0x003A;
    R[0] = gsu_add(R[0], R[8], 0);
    R[0] = gsu_ldw(R[0]);
    R[1] = 0x0024;
    R[1] = gsu_add(R[1], R[9], 0);
    R[1] = gsu_ldw(gsu_code_then(0x04A8F4, 6, R[1]));
    R[0] = gsu_add(R[0], R[1], 0);
    gsu_code(0x04A8FA, 2);
    gsu_sbk(R[0]);
lA8FB:
    R[0] = gsu_ldw(gsu_code_then(0x04A8FC, 4, FX_WHEEL_FLAGS));
    R[1] = 0x0080;
    R[0] = gsu_and(R[0], R[1]);
    t = gsu.z;
    if (t) { gsu_code(0x0496A7, 6); gsu_code(0x04A900, 6); gsu_code(0x04A90A, 5); goto lA90A; }
    goto lAEB9;
lA90A:
    R[11] = 0xA90F;
    abi_FxSumsClear();
lA90F:
    R[0] = 0x0000;
    R[1] = 0x7D00;
    gsu_code(0x0496C1, 2); gsu_code(0x04A90F, 11);
    gsu_stw(FX_PUSHES, R[0]);
    gsu_code(0x04A91A, 4);
    gsu_stw((FX_PUSHES + 2), R[1]);
    gsu_code(0x04A91E, 4);
    gsu_stw((FX_PUSHES + 10), R[0]);
    gsu_code(0x04A922, 4);
    gsu_stw((FX_PUSHES + 12), R[1]);
    gsu_code(0x04A926, 4);
    gsu_stw((FX_PUSHES + 20), R[0]);
    gsu_code(0x04A92A, 4);
    gsu_stw((FX_PUSHES + 22), R[1]);
    gsu_code(0x04A92E, 4);
    gsu_stw((FX_PUSHES + 30), R[0]);
    gsu_code(0x04A932, 4);
    gsu_stw((FX_PUSHES + 32), R[1]);
    gsu_code(0x04A936, 4);
    gsu_stw((FX_PUSHES + 40), R[0]);
    gsu_code(0x04A93A, 4);
    gsu_stw((FX_PUSHES + 42), R[1]);
    gsu_code(0x04A93E, 4);
    gsu_stw((FX_PUSHES + 50), R[0]);
    gsu_code(0x04A942, 4);
    gsu_stw((FX_PUSHES + 52), R[1]);
    gsu_code(0x04A946, 4);
    gsu_stw((FX_PUSHES + 60), R[0]);
    gsu_code(0x04A94A, 4);
    gsu_stw((FX_PUSHES + 62), R[1]);
    gsu_code(0x04A94E, 4);
    gsu_stw((FX_PUSHES + 70), R[0]);
    gsu_code(0x04A952, 4);
    gsu_stw((FX_PUSHES + 72), R[1]);
    gsu_code(0x04A956, 4);
    gsu_stw((FX_PUSHES + 80), R[0]);
    gsu_code(0x04A95A, 4);
    gsu_stw((FX_PUSHES + 82), R[1]);
    gsu_code(0x04A95E, 4);
    gsu_stw((FX_PUSHES + 90), R[0]);
    gsu_code(0x04A962, 4);
    gsu_stw((FX_PUSHES + 92), R[1]);
    gsu_code(0x04A966, 4);
    gsu_stw((FX_PUSHES + 100), R[0]);
    gsu_code(0x04A96A, 4);
    gsu_stw((FX_PUSHES + 102), R[1]);
    gsu_code(0x04A96E, 4);
    gsu_stw((FX_PUSHES + 110), R[0]);
    gsu_code(0x04A972, 4);
    gsu_stw((FX_PUSHES + 112), R[1]);
    gsu_code(0x04A976, 4);
    gsu_stw((FX_PUSHES + 120), R[0]);
    gsu_code(0x04A97A, 4);
    gsu_stw((FX_PUSHES + 122), R[1]);
    gsu_code(0x04A97E, 4);
    gsu_stw((FX_PUSHES + 130), R[0]);
    gsu_code(0x04A982, 4);
    gsu_stw((FX_PUSHES + 132), R[1]);
    gsu_code(0x04A986, 4);
    gsu_stw((FX_PUSHES + 140), R[0]);
    gsu_code(0x04A98A, 4);
    gsu_stw((FX_PUSHES + 142), R[1]);
    gsu_code(0x04A98E, 4);
    gsu_stw((FX_PUSHES + 150), R[0]);
    gsu_code(0x04A992, 4);
    gsu_stw((FX_PUSHES + 152), R[1]);
    gsu_code(0x04A996, 4);
    gsu_stw((FX_PUSHES + 160), R[0]);
    gsu_code(0x04A99A, 4);
    gsu_stw((FX_PUSHES + 162), R[1]);
    gsu_code(0x04A99E, 4);
    gsu_stw((FX_PUSHES + 170), R[0]);
    gsu_code(0x04A9A2, 4);
    gsu_stw((FX_PUSHES + 172), R[1]);
    gsu_code(0x04A9A6, 4);
    gsu_stw((FX_PUSHES + 180), R[0]);
    gsu_code(0x04A9AA, 4);
    gsu_stw((FX_PUSHES + 182), R[1]);
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_code(0x04A9AE, 5);
    gsu_stw(FX_CONTACT_SUM, R[0]);
    gsu_code(0x04A9B3, 4);
    gsu_stw((FX_CONTACT_SUM + 2), R[0]);
    gsu_code(0x04A9B7, 4);
    gsu_stw((FX_CONTACT_SUM + 4), R[0]);
    R[0] = 0x7D00;
    gsu_code(0x04A9BB, 7);
    gsu_stw(FX_CONTACT_DEPTH, R[0]);
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_code(0x04A9C2, 5);
    gsu_stw(FX_WHEEL_CONTACT, R[0]);
    R[0] = gsu_ldw(gsu_code_then(0x04A9C7, 4, FX_WHEEL_FLAGS));
    R[1] = 0x0030;
    R[0] = gsu_bic(R[0], R[1]);
    gsu_code(0x04A9CB, 6);
    gsu_sbk(R[0]);
    R[0] = 0x0018;
    R[0] = gsu_add(R[0], R[9], 0);
    R[0] = gsu_ldw(gsu_code_then(0x04A9D1, 4, R[0]));
    R[1] = R[0];
    gsu_moves(R[0]);
    t = !gsu.z;
    if (t) { gsu_code(0x04A9D5, 4); gsu_code(0x04A9DD, 7); goto lA9DD; }
    goto lAA98;
lA9DD:
    R[7] = 0x000C;
    R[7] = gsu_add(R[7], R[1], 0);
    R[7] = gsu_ldw(R[7]);
    gsu_code(0x04A9E4, 4);
    gsu_stw(FX_COLLIDE_MODEL, R[7]);
    R[11] = 0xA9EC;
    goto l9C3D;
lA9EC:
    R[0] = R[0];
    gsu_code(0x04A9EC, 5);
    gsu_moves(R[0]);
    t = gsu.z;
    if (t) { gsu_code(0x04AA0D, 5); goto lAA0D; } else { gsu_code(0x04A9F1, 5); gsu_code(0x04AEFF, 3); }
    R[11] = 0xA9F6;
    goto lAEFF;
lA9F6:
    R[1] = R[0];
    gsu_code(0x04A9F6, 12); gsu_code(0x04AA06, 7);
    gsu_moves(R[0]);
    t = gsu.z;
    if (t) goto lAA0D;
    R[0] = 0x001F;
    R[0] = gsu_and(R[0], R[1]);
    t = !gsu.z;
    if (t) goto lAA06;
lAA02:
    goto lAA98;
lAA06:
    R[0] = 0x0020;
    R[0] = gsu_and(R[0], R[1]);
    t = gsu.z;
    if (t) goto lAA5A;
lAA0D:
    R[2] = gsu_ldw(FX_COLLIDE_MODEL);
    R[1] = 0x0016;
    R[1] = gsu_add(R[1], R[2], 0);
    R[1] = gsu_ldw(gsu_code_then(0x04AA12, 6, R[1]));
    R[0] = 0x0040;
    R[0] = gsu_and(R[0], R[1]);
    t = !gsu.z;
    if (t) { gsu_code(0x04AA18, 6); goto lAA5A; } else { gsu_code(0x04AA18, 36); gsu_code(0x04AA52, 5); }
    R[0] = 0x3800;
    R[1] = gsu_and(R[0], R[1]);
    t = gsu.z;
    if (t) goto lAA8E;
    R[2] = 0x00A0;
    R[0] = 0x0800;
    gsu_sub(R[0], R[1], 0);   /* CMP */
    t = gsu.z;
    if (t) goto lAA52;
    R[2] = 0x2020;
    R[0] = 0x2000;
    gsu_sub(R[0], R[1], 0);   /* CMP */
    t = gsu.z;
    if (t) goto lAA52;
    R[2] = 0x1020;
    R[0] = 0x1000;
    gsu_sub(R[0], R[1], 0);   /* CMP */
    t = gsu.z;
    if (t) goto lAA52;
    R[2] = 0x0820;
    R[0] = 0x1800;
    gsu_sub(R[0], R[1], 0);   /* CMP */
    t = !gsu.z;
    if (t) goto lAA5A;
lAA52:
    R[0] = 0x002C;
    R[0] = gsu_add(R[0], R[9], 0);
    R[0] = gsu_ldw(R[0]);
    R[0] = gsu_or(R[0], R[2]);
    t = 1;
    gsu_code(0x04AA57, 3); gsu_code(0x04AA82, 1);
    gsu_sbk(R[0]);
    if (t) goto lAA82;
lAA5A:
    R[2] = gsu_ldw(gsu_code_then(0x04AA5A, 5, FX_COLLIDE_MODEL));
    R[1] = 0x0016;
    R[1] = gsu_add(R[1], R[2], 0);
    R[1] = gsu_ldw(gsu_code_then(0x04AA5F, 6, R[1]));
    R[0] = 0x4000;
    R[0] = gsu_and(R[0], R[1]);
    t = !gsu.z;
    if (t) goto lAA82; else { gsu_code(0x04AA65, 14); }
    R[1] = 0x008A;
    R[1] = gsu_add(R[1], R[8], 0);
    R[1] = gsu_ldw(R[1]);
    R[0] = 0x0040;
    R[0] = gsu_and(R[0], R[1]);
    t = gsu.z;
    if (t) { gsu_code(0x04AA73, 6); gsu_code(0x04AA82, 1); goto lAA82; }
    R[2] = 0x1000;
    R[0] = 0x002C;
    R[0] = gsu_add(R[0], R[9], 0);
    R[0] = gsu_ldw(R[0]);
    R[0] = gsu_or(R[0], R[2]);
    gsu_sbk(R[0]);
lAA82:
    R[0] = 0x002C;
    R[0] = gsu_add(R[0], R[8], 0);
    R[0] = gsu_ldw(gsu_code_then(0x04AA83, 4, R[0]));
    R[2] = 0x0040;
    R[0] = gsu_bic(R[0], R[2]);
    t = 1;
    gsu_code(0x04AA87, 7); gsu_code(0x04AAD0, 1);
    gsu_sbk(R[0]);
    if (t) { gsu_code(0x04AAD1, 6); goto lAAD0; }
lAA8E:
    R[0] = 0x002C;
    R[0] = gsu_add(R[0], R[8], 0);
    R[0] = gsu_ldw(R[0]);
    R[2] = 0x0040;
    R[0] = gsu_bic(R[0], R[2]);
    gsu_sbk(R[0]);
lAA98:
    R[1] = 0x008A;
    R[1] = gsu_add(R[1], R[8], 0);
    R[1] = gsu_ldw(R[1]);
    R[2] = 0x0040;
    R[0] = 0x0004;
    R[0] = gsu_and(R[0], R[1]);
    t = !gsu.z;
    if (t) goto lAACA;
    R[2] = 0x00C0;
    R[0] = 0x0020;
    R[0] = gsu_and(R[0], R[1]);
    t = !gsu.z;
    if (t) goto lAACA;
    R[2] = 0x1040;
    R[0] = 0x0040;
    R[0] = gsu_and(R[0], R[1]);
    t = !gsu.z;
    if (t) goto lAACA;
    R[2] = 0x0840;
    R[0] = 0x0002;
    R[0] = gsu_and(R[0], R[1]);
    t = !gsu.z;
    if (t) goto lAACA;
    R[2] = 0x0060;
lAACA:
    R[0] = 0x002C;
    R[0] = gsu_add(R[0], R[9], 0);
    R[0] = gsu_ldw(R[0]);
    R[0] = gsu_or(R[0], R[2]);
    gsu_sbk(R[0]);
lAAD0:
    R[2] = 0x0020;
    R[2] = gsu_add(R[2], R[9], 0);
    R[2] = gsu_ldw(R[2]);
    R[2] = R[2];
    gsu_moves(R[2]);
    t = gsu.s;
    if (t) { gsu_code(0x04AAD7, 4); gsu_code(0x04AB15, 5); goto lAB15; }
    R[0] = 0x002C;
    R[0] = gsu_add(R[0], R[9], 0);
    R[0] = gsu_ldw(R[0]);
    R[1] = 0x0020;
    R[0] = gsu_and(R[0], R[1]);
    t = gsu.z;
    if (t) goto lAB01;
    R[0] = 0x000A;
    R[2] = gsu_add(R[2], R[0], 0);
    R[0] = 0x001E;
    R[0] = gsu_add(R[0], R[9], 0);
    R[0] = gsu_ldw(R[0]);
    R[1] = 0x0022;
    R[1] = gsu_add(R[1], R[9], 0);
    R[1] = gsu_ldw(R[1]);
    R[0] = gsu_add(R[0], R[1], 0);
    R[0] = gsu_div2(R[0]);
    R[0] = gsu_div2(R[0]);
    R[0] = gsu_div2(R[0]);
    R[1] = 0x000F;
    R[0] = gsu_and(R[0], R[1]);
    R[2] = gsu_add(R[2], R[0], 0);
lAB01:
    R[11] = 0xAB06;
    abi_FxPushSet();
lAB06:
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.z;
    if (t) goto lAB15;
    R[3] = 0x23A4;
    R[2] = 0x0000;
    R[11] = 0xAB15;
    goto lAF54;
lAB15:
    R[0] = gsu_ldw(FX_CONTACT_FORCE_UP);
    gsu_code(0x04AB1A, 4);
    gsu_stw(FX_CONTACT_FORCE, R[0]);
    R[1] = 0x002C;
    R[1] = gsu_add(R[1], R[9], 0);
    R[1] = gsu_ldw(gsu_code_then(0x04AB1E, 6, R[1]));
    R[0] = 0x0020;
    R[0] = gsu_and(R[0], R[1]);
    t = gsu.z;
    if (t) { gsu_code(0x04AB24, 6); gsu_code(0x04AB4B, 1); goto lAB4B; } else { gsu_code(0x04AB24, 11); }
    R[0] = gsu_ldw(FX_CONTACT_FORCE_SIDE);
    gsu_code(0x04AB2F, 4);
    gsu_stw(FX_CONTACT_FORCE, R[0]);
    R[0] = 0x0046;
    R[0] = gsu_add(R[0], R[8], 0);
    R[0] = gsu_ldw(gsu_code_then(0x04AB33, 4, R[0]));
    R[0] = gsu_inc(R[0]);
    gsu_code(0x04AB37, 2);
    gsu_sbk(R[0]);
    R[0] = gsu_ldw(gsu_code_then(0x04AB39, 4, (FX_WORLD_SUM + 2)));
    R[0] = R[0];
    gsu_moves(R[0]);
    t = !gsu.z;
    if (t) goto lAB4B; else { gsu_code(0x04AB3D, 9); }
    R[0] = 0x002C;
    R[0] = gsu_add(R[0], R[9], 0);
    R[0] = gsu_ldw(R[0]);
    R[1] = 0x0020;
    R[0] = gsu_bic(R[0], R[1]);
    gsu_code(0x04AB46, 6);
    gsu_sbk(R[0]);
lAB4B:
    R[2] = gsu_ldw(gsu_code_then(0x04AB4C, 4, FX_WHEEL_FLAGS));
    R[0] = gsu_ldw(gsu_code_then(0x04AB50, 4, FX_CONTACT_DEPTH));
    R[1] = 0x7D00;
    gsu_sub(R[0], R[1], 0);   /* CMP */
    t = !gsu.z;
    if (t) { gsu_code(0x04AB54, 7); gsu_code(0x04AB5F, 5); goto lAB5F; } else { gsu_code(0x04AB54, 11); gsu_code(0x04AEE5, 8); }
    goto lAEE5;
lAB5F:
    R[0] = 0x0040;
    R[0] = gsu_add(R[0], R[8], 0);
    R[0] = gsu_ldw(R[0]);
    R[1] = 0x0100;
    R[0] = gsu_bic(R[0], R[1]);
    gsu_code(0x04AB64, 6);
    gsu_sbk(R[0]);
    R[0] = 0x0010;
    R[1] = gsu_or(R[0], R[2]);
    gsu_code(0x04AB6A, 9);
    gsu_stw(FX_WHEEL_FLAGS, R[1]);
    R[2] = gsu_ldw(gsu_code_then(0x04AB73, 4, FX_WHEEL_FLAGS));
    R[0] = 0x0008;
    R[0] = gsu_and(R[0], R[2]);
    t = gsu.z;
    if (t) { gsu_code(0x04AB77, 6); goto lABE5; } else { gsu_code(0x04AB77, 12); }
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_stw(FX_WHEEL_LOAD, R[0]);
    R[0] = 0x0050;
    R[0] = gsu_add(R[0], R[9], 0);
    R[0] = gsu_ldw(gsu_code_then(0x04AB83, 4, R[0]));
    R[0] = gsu_add(R[0], R[0], 0);
    R[6] = gsu_ldw(gsu_code_then(0x04AB87, 5, FX_CONTACT_FORCE));
    R[0] = gsu_fmult(R[0], R[6]);
    R[6] = 0x003C;
    R[6] = gsu_add(R[6], R[8], 0);
    R[6] = gsu_ldw(gsu_code_then(0x04AB8C, 7, R[6]));
    R[1] = 0x004E;
    R[1] = gsu_add(R[1], R[8], 0);
    R[1] = gsu_ldw(gsu_code_then(0x04AB93, 6, R[1]));
    R[1] = gsu_sub(R[1], 2, 0);
    t = !gsu.s;
    if (t) { gsu_code(0x04AB99, 5); gsu_code(0x04ABC8, 7); goto lABC8; }
    R[6] = R[6];
    gsu_moves(R[6]);
    t = gsu.z;
    if (t) goto lABE5;
    t = gsu.s;
    if (t) goto lABC8;
    gsu_sub(R[0], R[6], 0);   /* CMP */
    t = !gsu.s;
    if (t) goto lABC8;
    R[6] = R[0];
    R[0] = 0x0020;
    R[1] = gsu_or(R[0], R[2]);
    gsu_stw(FX_WHEEL_FLAGS, R[1]);
    R[1] = 0x0056;
    R[1] = gsu_add(R[1], R[8], 0);
    R[1] = gsu_ldw(R[1]);
    R[0] = 0x1F40;
    R[0] = gsu_add(R[0], R[1], 0);
    gsu_sbk(R[0]);
    R[0] = gsu_ldw(FX_CONTACT_FORCE);
    R[0] = gsu_div2(R[0]);
    gsu_sbk(R[0]);
lABC8:
    R[6] = gsu_add(R[6], R[6], 0);
    gsu_stw(FX_WHEEL_LOAD, R[6]);
    R[0] = gsu_ldw(gsu_code_then(0x04ABCF, 3, FX_OBJECT_MATRIX_6));
    R[1] = gsu_ldw(gsu_code_then(0x04ABD2, 3, FX_OBJECT_MATRIX_7));
    R[2] = gsu_ldw(gsu_code_then(0x04ABD5, 3, FX_OBJECT_MATRIX_8));
    R[0] = gsu_fmult(R[0], R[6]);
    R[4] = R[0];
    R[5] = gsu_fmult(R[1], R[6]);
    R[6] = gsu_fmult(R[2], R[6]);
    R[11] = 0xABE5;
    abi_FxSumsPushWorld();
lABE5:
    R[0] = 0x0000;
    gsu_code(0x04ABE5, 9);
    gsu_romb(R[0]);
    R[0] = 0x0024;
    R[0] = gsu_add(R[0], R[9], 0);
    R[0] = gsu_ldw(R[0]);
    R[1] = 0x0024;
    R[1] = gsu_add(R[1], R[8], 0);
    R[1] = gsu_ldw(gsu_code_then(0x04ABEE, 6, R[1]));
    R[0] = gsu_sub(R[0], R[1], 0);
    R[0] = gsu_hib(R[0]);
    R[1] = R[0];
    R[2] = 0xF060;
    R[3] = 0x0FA0;
    R[0] = gsu_lob(R[0]);
    gsu_code(0x04ABF4, 14);
    gsu_r14(0x96FE);
    gsu_code(0x04AC02, 2);
    gsu_r14(gsu_add(R[0], R[14], 0));
    R[0] = (gsu_code(0x04AC04, 1), gsu_getb());
    R[0] = gsu_swap(R[0]);
    R[0] = gsu_div2(R[0]);
    R[0] = gsu_div2(R[0]);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.s;
    if (t) { gsu_code(0x04AC18, 5); goto lAC18; } else { gsu_code(0x04AC0E, 5); }
    gsu_sub(R[0], R[3], 0);   /* CMP */
    t = gsu.s;
    if (t) goto lAC1F;
    R[0] = R[3];
    t = 1;
    if (t) goto lAC1F;
lAC18:
    gsu_sub(R[0], R[2], 0);   /* CMP */
    t = !gsu.s;
    if (t) goto lAC1F;
    R[0] = R[2];
lAC1F:
    gsu_code(0x04AC05, 9); gsu_code(0x04AC1F, 5);
    gsu_stw(FX_VIEW_MATRIX_2, R[0]);
    R[0] = R[1];
    R[0] = gsu_lob(R[0]);
    gsu_code(0x04AC24, 14); gsu_code(0x04AC34, 5);
    gsu_r14(0x973E);
    gsu_r14(gsu_add(R[0], R[14], 0));
    R[0] = gsu_getb();
    R[0] = gsu_swap(R[0]);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = !gsu.s;
    if (t) goto lAC34;
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
lAC34:
    gsu_stw(FX_VIEW_MATRIX_8, R[0]);
    R[1] = 0x002E;
    R[1] = gsu_add(R[1], R[9], 0);
    R[1] = gsu_ldw(gsu_code_then(0x04AC39, 6, R[1]));
    R[1] = gsu_not(R[1]);
    R[1] = gsu_inc(R[1]);
    R[2] = 0x0030;
    R[2] = gsu_add(R[2], R[9], 0);
    R[2] = gsu_ldw(gsu_code_then(0x04AC3F, 9, R[2]));
    R[2] = gsu_not(R[2]);
    R[2] = gsu_inc(R[2]);
    R[3] = 0x0032;
    R[3] = gsu_add(R[3], R[9], 0);
    R[3] = gsu_ldw(gsu_code_then(0x04AC48, 9, R[3]));
    R[3] = gsu_not(R[3]);
    R[3] = gsu_inc(R[3]);
    R[6] = gsu_ldw(gsu_code_then(0x04AC51, 6, GSU_OUT_E4));
    R[0] = gsu_fmult(R[1], R[6]);
    R[5] = gsu_rol(R[0]);
    R[6] = gsu_ldw(gsu_code_then(0x04AC57, 7, FX_OBJECT_MATRIX_1));
    R[0] = gsu_fmult(R[2], R[6]);
    R[0] = gsu_rol(R[0]);
    R[5] = gsu_add(R[5], R[0], 0);
    R[6] = gsu_ldw(gsu_code_then(0x04AC5E, 8, FX_OBJECT_MATRIX_2));
    R[0] = gsu_fmult(R[3], R[6]);
    R[0] = gsu_rol(R[0]);
    R[0] = gsu_add(R[0], R[5], 0);
    gsu_code(0x04AC66, 8);
    gsu_stw(FX_WHEEL_SIDE, R[0]);
    R[6] = gsu_ldw(gsu_code_then(0x04AC6E, 3, FX_OBJECT_MATRIX_6));
    R[0] = gsu_fmult(R[1], R[6]);
    R[5] = gsu_rol(R[0]);
    R[6] = gsu_ldw(gsu_code_then(0x04AC71, 7, FX_OBJECT_MATRIX_7));
    R[0] = gsu_fmult(R[2], R[6]);
    R[0] = gsu_rol(R[0]);
    R[5] = gsu_add(R[5], R[0], 0);
    R[6] = gsu_ldw(gsu_code_then(0x04AC78, 8, FX_OBJECT_MATRIX_8));
    R[0] = gsu_fmult(R[3], R[6]);
    R[0] = gsu_rol(R[0]);
    R[0] = gsu_add(R[0], R[5], 0);
    gsu_code(0x04AC80, 8);
    gsu_stw(FX_WHEEL_AHEAD, R[0]);
    R[1] = R[0];
    R[0] = 0x0026;
    R[0] = gsu_add(R[0], R[9], 0);
    R[0] = gsu_ldw(gsu_code_then(0x04AC88, 6, R[0]));
    R[1] = gsu_add(R[0], R[1], 0);
    R[0] = 0x0026;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_code(0x04AC8E, 7);
    gsu_stw(R[0], R[1]);
    R[1] = gsu_ldw(gsu_code_then(0x04AC95, 4, FX_WHEEL_SIDE));
    R[3] = gsu_ldw(gsu_code_then(0x04AC99, 4, FX_WHEEL_AHEAD));
    R[6] = gsu_ldw(gsu_code_then(0x04AC9D, 3, FX_VIEW_MATRIX_8));
    R[1] = gsu_fmult(R[1], R[6]);
    R[6] = gsu_ldw(gsu_code_then(0x04ACA0, 5, FX_VIEW_MATRIX_2));
    R[0] = gsu_add(R[3], R[3], 0);
    R[0] = gsu_fmult(R[0], R[6]);
    R[0] = gsu_add(R[0], R[1], 0);
    R[0] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_add(R[0], R[0], 0);
    R[4] = R[0];
    R[1] = R[0];
    R[5] = 0x0050;
    R[5] = gsu_add(R[5], R[9], 0);
    R[5] = gsu_ldw(gsu_code_then(0x04ACA5, 16, R[5]));
    R[0] = 0x003C;
    R[0] = gsu_add(R[0], R[8], 0);
    R[0] = gsu_ldw(gsu_code_then(0x04ACB5, 4, R[0]));
    R[0] = R[0];
    gsu_moves(R[0]);
    t = !gsu.z;
    if (t) { gsu_code(0x04ACB9, 4); goto lACC6; } else { gsu_code(0x04ACB9, 9); }
    R[6] = gsu_ldw(FX_PARAM_GRIP_FREE);
    R[0] = gsu_add(R[5], R[5], 0);
    R[0] = gsu_fmult(R[0], R[6]);
    R[5] = gsu_add(R[5], R[0], 0);
lACC6:
    R[1] = 0x003A;
    R[1] = gsu_add(R[1], R[8], 0);
    R[1] = gsu_ldw(gsu_code_then(0x04ACC6, 7, R[1]));
    R[1] = R[1];
    gsu_moves(R[1]);
    t = gsu.z;
    if (t) { gsu_code(0x04ACCD, 4); goto lACE9; } else { gsu_code(0x04ACCD, 11); }
    R[2] = 0x0034;
    R[2] = gsu_add(R[2], R[8], 0);
    R[2] = gsu_ldw(R[2]);
    R[2] = R[2];
    gsu_moves(R[2]);
    t = !gsu.s;
    if (t) { gsu_code(0x04ACD8, 4); gsu_code(0x04ACE4, 5); goto lACE4; } else { gsu_code(0x04ACD8, 9); }
    R[1] = R[1];
    gsu_moves(R[1]);
    t = !gsu.s;
    if (t) goto lACE9; else { gsu_code(0x04ACE1, 3); }
    t = 1;
    if (t) goto lACFC;
lACE4:
    R[1] = R[1];
    gsu_moves(R[1]);
    t = !gsu.s;
    if (t) goto lACFC;
lACE9:
    R[2] = gsu_ldw(gsu_code_then(0x04ACE9, 5, FX_WHEEL_FLAGS));
    R[0] = 0x0002;
    R[0] = gsu_and(R[0], R[2]);
    t = gsu.z;
    if (t) { gsu_code(0x04ACEE, 6); goto lACFC; } else { gsu_code(0x04ACEE, 11); }
    R[6] = gsu_ldw(FX_PARAM_GRIP_SLIDE);
    R[5] = gsu_add(R[5], R[5], 0);
    R[5] = gsu_fmult(R[5], R[6]);
lACFC:
    R[2] = gsu_ldw(gsu_code_then(0x04ACFC, 5, FX_WHEEL_FLAGS));
    R[1] = 0x0042;
    R[1] = gsu_add(R[1], R[8], 0);
    R[1] = gsu_ldw(gsu_code_then(0x04AD01, 6, R[1]));
    R[0] = 0x000C;
    R[0] = gsu_and(R[0], R[1]);
    t = gsu.z;
    if (t) { gsu_code(0x04AD07, 6); gsu_code(0x04AD2F, 7); gsu_code(0x04AD3D, 1); goto lAD2F; }
    R[0] = 0x0002;
    R[0] = gsu_and(R[0], R[2]);
    t = gsu.z;
    if (t) goto lAD1F;
    R[6] = gsu_ldw(FX_PARAM_GRIP_SIDE);
    R[0] = gsu_add(R[5], R[5], 0);
    R[0] = gsu_fmult(R[0], R[6]);
    t = 1;
    R[5] = gsu_add(R[5], R[0], 0);
    if (t) goto lAD2F;
lAD1F:
    R[0] = 0x0001;
    R[0] = gsu_and(R[0], R[2]);
    t = gsu.z;
    if (t) goto lAD2F;
    R[6] = gsu_ldw(FX_PARAM_GRIP_AHEAD);
    R[0] = gsu_add(R[5], R[5], 0);
    R[0] = gsu_fmult(R[0], R[6]);
    R[5] = gsu_add(R[5], R[0], 0);
lAD2F:
    R[0] = 0x0002;
    R[0] = gsu_and(R[0], R[2]);
    t = gsu.z;
    if (t) goto lAD3D; else { gsu_code(0x04AD36, 7); }
    R[0] = 0x4650;
    gsu_stw(FX_CONTACT_FORCE, R[0]);
lAD3D:
    R[6] = gsu_ldw(gsu_code_then(0x04AD3E, 4, FX_CONTACT_FORCE));
    R[5] = gsu_add(R[5], R[5], 0);
    R[5] = gsu_fmult(R[5], R[6]);
    R[0] = R[4];
    gsu_moves(R[4]);
    t = !gsu.s;
    if (t) goto lAD4C; else { gsu_code(0x04AD4A, 2); }
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
lAD4C:
    R[0] = gsu_sub(R[0], R[5], 0);
    gsu_code(0x04AD42, 8); gsu_code(0x04AD4C, 4);
    t = gsu.s;
    if (t) { gsu_code(0x04AD80, 1); goto lAD80; } else { gsu_code(0x04AD50, 7); gsu_code(0x04AD5A, 11); gsu_code(0x04AD68, 5); }
    R[2] = R[0];
    R[4] = R[4];
    gsu_moves(R[4]);
    t = !gsu.s;
    if (t) goto lAD5A; else { gsu_code(0x04AD57, 3); gsu_code(0x04AD65, 3); }
    R[5] = gsu_not(R[5]);
    R[5] = gsu_inc(R[5]);
lAD5A:
    R[1] = R[4];
    R[4] = R[5];
    R[0] = gsu_sub(R[1], R[5], 0);
    R[1] = R[0];
    gsu_moves(R[0]);
    t = !gsu.s;
    if (t) goto lAD68;
    R[1] = gsu_not(R[1]);
    R[1] = gsu_inc(R[1]);
lAD68:
    R[0] = gsu_ldw(FX_PARAM_SLIDE_LIMIT);
    gsu_code(0x04AD6D, 4);
    gsu_sub(R[0], R[1], 0);   /* CMP */
    t = !gsu.s;
    if (t) { gsu_code(0x04AD80, 1); goto lAD80; } else { gsu_code(0x04AD71, 5); }
    R[0] = 0x0056;
    R[0] = gsu_add(R[0], R[8], 0);
    R[0] = gsu_ldw(R[0]);
    R[0] = gsu_add(R[0], R[1], 0);
    gsu_code(0x04AD76, 2);
    gsu_sbk(R[0]);
    R[2] = gsu_ldw(gsu_code_then(0x04AD78, 4, FX_WHEEL_FLAGS));
    R[0] = 0x0020;
    R[0] = gsu_or(R[0], R[2]);
    gsu_code(0x04AD7C, 5);
    gsu_sbk(R[0]);
lAD80:
    R[5] = 0x0000;
    R[6] = 0x0000;
    R[11] = 0xAD8B;
    abi_FxSumsPushLocal();
lAD8B:
    R[6] = 0x0048;
    R[6] = gsu_add(R[6], R[8], 0);
    R[6] = gsu_ldw(R[6]);
    R[6] = R[6];
    gsu_moves(R[6]);
    t = gsu.z;
    if (t) { gsu_code(0x04AD92, 4); goto lADC4; } else { gsu_code(0x04AD92, 9); }
    R[0] = gsu_ldw(FX_WHEEL_AHEAD);
    R[0] = gsu_add(R[0], R[0], 0);
    R[6] = gsu_fmult(R[0], R[6]);
    R[4] = 0xFF38;
    R[5] = 0x00C8;
    R[6] = R[6];
    gsu_moves(R[6]);
    t = gsu.s;
    if (t) goto lADB3;
    gsu_sub(R[6], R[5], 0);   /* CMP */
    t = gsu.s;
    if (t) goto lADBB;
    R[6] = R[5];
    t = 1;
    if (t) goto lADBB;
lADB3:
    gsu_sub(R[6], R[4], 0);   /* CMP */
    t = !gsu.s;
    if (t) goto lADBB; else { gsu_code(0x04ADB9, 2); }
    R[6] = R[4];
lADBB:
    R[4] = 0x0000;
    R[5] = 0x0000;
    R[11] = 0xADC4;
    abi_FxSumsPushLocal();
lADC4:
    R[11] = 0xADC9;
    gsu_code(0x0497BB, 5); gsu_code(0x04ADC4, 5);
    abi_FxSumsRotateLocal();
lADC9:
    R[0] = gsu_ldw(gsu_code_then(0x04981E, 2, gsu_code_then(0x04ADC9, 5, FX_WORLD_SUM)));
    R[1] = gsu_ldw(gsu_code_then(0x04ADCE, 4, (FX_WORLD_SUM + 2)));
    R[2] = gsu_ldw(gsu_code_then(0x04ADD2, 4, (FX_WORLD_SUM + 4)));
    R[0] = gsu_or(R[0], R[1]);
    R[0] = gsu_or(R[0], R[2]);
    t = !gsu.z;
    if (t) { gsu_code(0x04ADD6, 4); gsu_code(0x04ADDE, 8); goto lADDE; }
    goto lAE39;
lADDE:
    R[1] = 0x0036;
    R[1] = gsu_add(R[1], R[9], 0);
    R[1] = gsu_ldw(R[1]);
    R[2] = 0x0038;
    R[2] = gsu_add(R[2], R[9], 0);
    R[2] = gsu_ldw(gsu_code_then(0x04ADE6, 6, R[2]));
    R[3] = 0x003A;
    R[3] = gsu_add(R[3], R[9], 0);
    R[3] = gsu_ldw(gsu_code_then(0x04ADEC, 6, R[3]));
    R[6] = gsu_ldw(gsu_code_then(0x04ADF2, 4, FX_PARAM_HIT));
    R[11] = 0xADFA;
    abi_FxHitAddScaled();
lADFA:
    R[2] = gsu_ldw(FX_WHEEL_FLAGS);
    R[0] = 0x0200;
    R[0] = gsu_and(R[0], R[2]);
    t = gsu.z;
    if (t) { gsu_code(0x04ADFF, 13); goto lAE05; }
lAE05:
    R[1] = 0x003C;
    R[1] = gsu_add(R[1], R[9], 0);
    R[1] = gsu_ldw(R[1]);
    R[2] = 0x003E;
    R[2] = gsu_add(R[2], R[9], 0);
    R[2] = gsu_ldw(gsu_code_then(0x04AE0C, 6, R[2]));
    R[3] = 0x0040;
    R[3] = gsu_add(R[3], R[9], 0);
    R[3] = gsu_ldw(gsu_code_then(0x04AE12, 6, R[3]));
    R[0] = 0x005A;
    R[0] = gsu_add(R[0], R[8], 0);
    R[0] = gsu_ldw(gsu_code_then(0x04AE18, 4, R[0]));
    R[2] = gsu_add(R[2], R[0], 0);
    R[0] = 0x005C;
    R[0] = gsu_add(R[0], R[8], 0);
    R[0] = gsu_ldw(gsu_code_then(0x04AE1C, 6, R[0]));
    R[3] = gsu_add(R[3], R[0], 0);
    R[0] = 0x0004;
    gsu_code(0x04AE22, 10);
    gsu_romb(R[0]);
    R[0] = gsu_ldw(FX_PARAM_PUSH);
    gsu_code(0x04AE2C, 3);
    gsu_r14(0x9BC6);
    gsu_code(0x04AE2F, 2);
    gsu_r14(gsu_add(R[14], R[0], 0));
    R[0] = (gsu_code(0x04AE31, 1), gsu_getb());
    gsu_code(0x04AE32, 1);
    gsu_r14(gsu_inc(R[14]));
    R[0] = gsu_getbh(gsu_code_then(0x04AE33, 2, R[0]));
    R[12] = R[0];
    R[11] = 0xAE39;
    nx = (uint16_t)(R[12]);
    at = nx;
    goto dispatch;
lAE39:
    R[3] = gsu_ldw(FX_PARAM_BOUNCE);
    R[2] = R[3];
    R[2] = gsu_not(R[2]);
    R[2] = gsu_inc(R[2]);
    R[0] = gsu_ldw(gsu_code_then(0x04AE3E, 9, FX_CONTACT_SUM));
    R[0] = gsu_div2(R[0]);
    R[0] = gsu_div2(R[0]);
    R[0] = gsu_div2(R[0]);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.s;
    if (t) goto lAE5B; else { gsu_code(0x04AE47, 15); gsu_code(0x04AE62, 8); }
    gsu_sub(R[0], R[3], 0);   /* CMP */
    t = gsu.s;
    if (t) goto lAE62;
    R[0] = R[3];
    t = 1;
    if (t) goto lAE62;
lAE5B:
    gsu_sub(R[0], R[2], 0);   /* CMP */
    t = !gsu.s;
    if (t) goto lAE62;
    R[0] = R[2];
lAE62:
    R[1] = R[0];
    R[0] = 0x004A;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_stw(R[0], R[1]);
    R[0] = gsu_ldw(gsu_code_then(0x04AE6A, 4, (FX_CONTACT_SUM + 4)));
    R[0] = gsu_div2(R[0]);
    R[0] = gsu_div2(R[0]);
    R[0] = gsu_div2(R[0]);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.s;
    if (t) { gsu_code(0x04AE82, 5); goto lAE82; } else { gsu_code(0x04AE78, 5); }
    gsu_sub(R[0], R[3], 0);   /* CMP */
    t = gsu.s;
    if (t) goto lAE89;
    R[0] = R[3];
    t = 1;
    if (t) goto lAE89;
lAE82:
    gsu_sub(R[0], R[2], 0);   /* CMP */
    t = !gsu.s;
    if (t) goto lAE89;
    R[0] = R[2];
lAE89:
    R[1] = R[0];
    R[0] = 0x004E;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_code(0x04AE6E, 10); gsu_code(0x04AE89, 8);
    gsu_stw(R[0], R[1]);
    R[0] = gsu_ldw(gsu_code_then(0x04AE91, 4, (FX_CONTACT_SUM + 2)));
    R[0] = gsu_div2(R[0]);
    R[0] = gsu_div2(R[0]);
    R[0] = gsu_div2(R[0]);
    R[4] = R[0];
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.s;
    if (t) { gsu_code(0x04AEAB, 5); goto lAEAB; } else { gsu_code(0x04AEA1, 5); }
    gsu_sub(R[0], R[3], 0);   /* CMP */
    t = gsu.s;
    if (t) goto lAEB2;
    R[0] = R[3];
    t = 1;
    if (t) goto lAEB2;
lAEAB:
    gsu_sub(R[0], R[2], 0);   /* CMP */
    t = !gsu.s;
    if (t) goto lAEB2; else { gsu_code(0x04AEB0, 2); }
    R[0] = R[2];
lAEB2:
    R[1] = R[0];
    R[0] = 0x004C;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_code(0x04AE95, 12); gsu_code(0x04AEB2, 8);
    gsu_stw(R[0], R[1]);
lAEB9:
    R[2] = 0x0046;
    R[2] = gsu_add(R[2], R[9], 0);
    R[2] = gsu_ldw(gsu_code_then(0x04AEBA, 6, R[2]));
    R[0] = 0x0002;
    R[0] = gsu_and(R[0], R[2]);
    t = gsu.z;
    if (t) { gsu_code(0x04AEC0, 6); gsu_code(0x04AED7, 1); goto lAED7; } else { gsu_code(0x04AEC0, 13); }
    R[1] = 0x0024;
    R[1] = gsu_add(R[1], R[9], 0);
    R[1] = gsu_ldw(R[1]);
    R[0] = 0x003A;
    R[0] = gsu_add(R[0], R[8], 0);
    R[0] = gsu_ldw(gsu_code_then(0x04AECD, 4, R[0]));
    R[1] = gsu_add(R[1], R[0], 0);
    R[0] = 0x0024;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_code(0x04AED1, 7);
    gsu_stw(R[0], R[1]);
lAED7:
    R[1] = gsu_ldw(gsu_code_then(0x04AED8, 4, FX_WHEEL_FLAGS));
    R[0] = 0x0046;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_code(0x04AEDC, 5);
    gsu_stw(R[0], R[1]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x04AEE1, 4);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
lAEE5:
    R[1] = 0x0000;
    R[0] = 0x004A;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_stw(R[0], R[1]);
    R[0] = 0x004C;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_code(0x04AEED, 5);
    gsu_stw(R[0], R[1]);
    R[0] = 0x004E;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_code(0x04AEF2, 5);
    gsu_stw(R[0], R[1]);
    R[0] = 0x0044;
    R[0] = gsu_add(R[0], R[8], 0);
    R[0] = gsu_ldw(gsu_code_then(0x04AEF7, 4, R[0]));
    R[0] = gsu_inc(R[0]);
    gsu_code(0x04AEFB, 2);
    gsu_sbk(R[0]);
    t = 1;
    if (t) { gsu_code(0x04AEB9, 1); gsu_code(0x04AEFD, 2); goto lAEB9; }
    /* FxContactsResolve */
lAEFF:
    gsu_stw(R[10], R[11]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[0] = 0x0000;
    gsu_code(0x04AF02, 11);
    gsu_romb(R[0]);
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_stw(FX_PUSH_FLAGS, R[0]);
    R[0] = 0x002C;
    R[0] = gsu_add(R[0], R[8], 0);
    R[0] = gsu_ldw(gsu_code_then(0x04AF0D, 4, R[0]));
    gsu_code(0x04AF11, 4);
    gsu_stw(FX_HIT_DEPTH, R[0]);
    R[3] = 0x23AE;
    R[13] = 0x0000;
    /* CACHE */
lAF1A:
    R[13] = gsu_inc(R[13]);
    R[0] = 0x0014;
    gsu_sub(R[0], R[13], 0);   /* CMP */
    t = !gsu.z;
    if (t) { gsu_code(0x04AF1A, 1); gsu_cache(0xAF1A); gsu_code(0x04AF1B, 7); gsu_code(0x04AF26, 2); goto lAF26; } else { gsu_code(0x04AF1A, 1); gsu_cache(0xAF1A); gsu_code(0x04AF1B, 11); gsu_code(0x04AF3C, 5); }
    goto lAF3C;
lAF26:
    R[0] = gsu_ldw(R[3]);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.z;
    if (t) { gsu_code(0x04AF28, 4); gsu_code(0x04AF4E, 6); goto lAF4E; } else { gsu_code(0x04AF28, 9); gsu_code(0x04AF54, 2); }
    R[11] = 0xAF31;
    goto lAF54;
lAF31:
    R[0] = gsu_ldw(gsu_code_then(0x04AF31, 5, FX_CONTACT_FORCE_UP));
    gsu_code(0x04AF36, 4);
    gsu_stw(FX_CONTACT_FORCE, R[0]);
    t = 1;
    if (t) { gsu_code(0x04AF3A, 2); goto lAF1A; }
lAF3C:
    R[1] = gsu_ldw(FX_HIT_DEPTH);
    R[0] = 0x002C;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04AF41, 5);
    gsu_stw(R[0], R[1]);
    R[0] = gsu_ldw(gsu_code_then(0x04AF46, 4, FX_PUSH_FLAGS));
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x04AF4A, 4);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
lAF4E:
    R[0] = 0x000A;
    t = 1;
    R[3] = gsu_add(R[3], R[0], 0);
    if (t) goto lAF1A;
    /* FxContactResolve */
lAF54:
    R[0] = gsu_ldw(R[3]);
    R[3] = gsu_inc(R[3]);
    R[3] = gsu_inc(R[3]);
    R[1] = R[0];
    gsu_moves(R[0]);
    t = !gsu.z;
    if (t) { gsu_code(0x04AF56, 6); gsu_code(0x04AF60, 5); goto lAF60; }
    goto lB19A;
lAF60:
    R[2] = gsu_ldw(FX_PUSH_FLAGS);
    R[0] = gsu_or(R[0], R[2]);
    gsu_code(0x04AF65, 2);
    gsu_sbk(R[0]);
    R[0] = gsu_hib(R[1]);
    t = gsu.z;
    if (t) { gsu_code(0x04AF67, 4); gsu_code(0x04AF6F, 3); goto lAF6F; } else { gsu_code(0x04AF67, 8); gsu_code(0x04B18A, 7); }
    goto lB18A;
lAF6F:
    gsu_stw(R[10], R[11]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[0] = gsu_ldw(gsu_code_then(0x04AF72, 3, R[3]));
    R[3] = gsu_inc(R[3]);
    R[3] = gsu_inc(R[3]);
    R[7] = R[0];
    gsu_moves(R[0]);
    t = !gsu.s;
    if (t) { gsu_code(0x04AF75, 6); gsu_code(0x04AF7F, 5); goto lAF7F; }
    goto lB191;
lAF7F:
    R[0] = gsu_ldw(FX_CONTACT_DEPTH);
    R[5] = R[0];
    gsu_moves(R[0]);
    t = !gsu.s;
    if (t) { gsu_code(0x04AF84, 4); gsu_code(0x04AF8B, 5); gsu_code(0x04AF93, 11); goto lAF8B; }
    R[5] = gsu_not(R[5]);
    R[5] = gsu_inc(R[5]);
lAF8B:
    R[4] = R[7];
    gsu_moves(R[7]);
    t = !gsu.s;
    if (t) goto lAF93;
lAF90:
    R[4] = gsu_not(R[4]);
    R[4] = gsu_inc(R[4]);
lAF93:
    gsu_sub(R[5], R[4], 0);   /* CMP */
    t = gsu.s;
    if (t) goto lAF9D;
    gsu_stw(FX_CONTACT_DEPTH, R[7]);
lAF9D:
    R[0] = 0x7D00;
    gsu_code(0x04AF9E, 7); gsu_code(0x04AFA9, 11);
    gsu_sub(R[0], R[7], 0);   /* CMP */
    t = !gsu.z;
    if (t) goto lAFA9;
    goto lB191;
lAFA9:
    R[7] = gsu_add(R[7], R[7], 0);
    R[7] = gsu_add(R[7], R[7], 0);
    R[7] = gsu_add(R[7], R[7], 0);
    R[7] = gsu_add(R[7], R[7], 0);
    R[4] = gsu_ldw(R[3]);
    R[3] = gsu_inc(R[3]);
    R[3] = gsu_inc(R[3]);
    R[5] = gsu_ldw(gsu_code_then(0x04AFB4, 4, R[3]));
    R[3] = gsu_inc(R[3]);
    R[3] = gsu_inc(R[3]);
    R[6] = gsu_ldw(gsu_code_then(0x04AFB8, 4, R[3]));
    R[3] = gsu_inc(R[3]);
    R[3] = gsu_inc(R[3]);
    R[0] = gsu_or(R[4], R[6]);
    t = !gsu.z;
    if (t) { gsu_code(0x04AFBC, 6); gsu_code(0x04AFD2, 1); goto lAFD2; } else { gsu_code(0x04AFBC, 11); }
    R[0] = gsu_ldw(FX_WHEEL_CONTACT);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.z;
    if (t) { gsu_code(0x04AFC7, 4); gsu_code(0x04AFCF, 4); goto lAFCF; }
    goto lB185;
lAFCF:
    R[0] = 0x0001;
    gsu_sbk(R[0]);
lAFD2:
    R[0] = gsu_ldw(gsu_code_then(0x04AFD3, 4, FX_PUSH_TURN));
    R[0] = R[0];
    gsu_moves(R[0]);
    t = !gsu.z;
    if (t) { gsu_code(0x04AFDF, 8); goto lAFDF; } else { gsu_code(0x04AFDB, 4); }
    goto lB06C;
lAFDF:
    R[1] = 0x0040;
    gsu_sub(R[0], R[1], 0);   /* CMP */
    t = !gsu.z;
    if (t) { gsu_code(0x04AFF3, 8); goto lAFF3; } else { gsu_code(0x04AFE7, 12); }
    R[0] = R[6];
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
    R[6] = R[4];
    R[4] = R[0];
    goto lB06C;
lAFF3:
    R[1] = 0x00C0;
    gsu_sub(R[0], R[1], 0);   /* CMP */
    t = !gsu.z;
    if (t) { gsu_code(0x04B006, 17); goto lB006; } else { gsu_code(0x04AFFB, 11); }
    R[0] = R[4];
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
    R[4] = R[6];
    R[6] = R[0];
    t = 1;
    if (t) goto lB06C;
lB006:
    R[1] = 0x0080;
    gsu_sub(R[0], R[1], 0);   /* CMP */
    t = !gsu.z;
    if (t) goto lB017;
    R[4] = gsu_not(R[4]);
    R[4] = gsu_inc(R[4]);
    R[6] = gsu_not(R[6]);
    R[6] = gsu_inc(R[6]);
    t = 1;
    if (t) goto lB06C;
lB017:
    gsu_stw(R[10], R[2]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_stw(R[10], R[3]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_stw(R[10], R[5]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_stw(R[10], R[7]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_stw(R[10], R[8]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_stw(R[10], R[9]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_stw(R[10], R[11]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[1] = R[4];
    R[3] = R[6];
    R[2] = R[0];
    R[0] = R[1];
    R[0] = gsu_or(R[0], R[3]);
    t = gsu.z;
    if (t) goto lB04C;
    R[11] = 0xB044;
    abi_FxTurn();
lB044:
    R[1] = gsu_add(R[1], R[1], 0);
    R[1] = gsu_add(R[1], R[1], 0);
    R[3] = gsu_add(R[3], R[3], 0);
    R[3] = gsu_add(R[3], R[3], 0);
lB04C:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    R[11] = gsu_ldw(R[10]);
    R[4] = R[3];
    R[6] = R[1];
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    R[9] = gsu_ldw(R[10]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    R[8] = gsu_ldw(R[10]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    R[7] = gsu_ldw(R[10]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    R[5] = gsu_ldw(R[10]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    R[3] = gsu_ldw(R[10]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    R[2] = gsu_ldw(R[10]);
lB06C:
    R[3] = gsu_sub(R[3], 6, 0);
    gsu_code(0x04AFD7, 4); gsu_code(0x04B06C, 6);
    gsu_stw(R[3], R[4]);
    R[3] = gsu_inc(R[3]);
    R[3] = gsu_inc(R[3]);
    gsu_code(0x04B072, 4);
    gsu_stw(R[3], R[5]);
    R[3] = gsu_inc(R[3]);
    R[3] = gsu_inc(R[3]);
    gsu_code(0x04B076, 4);
    gsu_stw(R[3], R[6]);
    R[3] = gsu_inc(R[3]);
    R[3] = gsu_inc(R[3]);
    gsu_code(0x04B07A, 6);
    gsu_stw(FX_SIZES, R[4]);
    gsu_code(0x04B080, 4);
    gsu_stw((FX_SIZES + 2), R[5]);
    gsu_code(0x04B084, 4);
    gsu_stw((FX_SIZES + 4), R[6]);
    R[0] = 0x002E;
    R[0] = gsu_add(R[0], R[9], 0);
    R[0] = gsu_ldw(gsu_code_then(0x04B088, 4, R[0]));
    R[0] = gsu_add(R[0], R[0], 0);
    R[6] = gsu_ldw(gsu_code_then(0x04B08C, 5, FX_SIZES));
    R[0] = gsu_fmult(R[0], R[6]);
    R[1] = R[0];
    R[0] = 0x0030;
    R[0] = gsu_add(R[0], R[9], 0);
    R[0] = gsu_ldw(gsu_code_then(0x04B091, 7, R[0]));
    R[0] = gsu_add(R[0], R[0], 0);
    R[6] = gsu_ldw(gsu_code_then(0x04B098, 5, (FX_SIZES + 2)));
    R[0] = gsu_fmult(R[0], R[6]);
    R[1] = gsu_add(R[1], R[0], 0);
    R[0] = 0x0032;
    R[0] = gsu_add(R[0], R[9], 0);
    R[0] = gsu_ldw(gsu_code_then(0x04B09D, 7, R[0]));
    R[0] = gsu_add(R[0], R[0], 0);
    R[6] = gsu_ldw(gsu_code_then(0x04B0A4, 5, (FX_SIZES + 4)));
    R[0] = gsu_fmult(R[0], R[6]);
    R[1] = gsu_add(R[1], R[0], 0);
    R[1] = gsu_not(R[1]);
    R[1] = gsu_inc(R[1]);
    R[2] = R[1];
    R[0] = 0x0042;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_code(0x04B0A9, 13);
    gsu_stw(R[0], R[1]);
    R[0] = R[1];
    R[6] = 0x0044;
    R[6] = gsu_add(R[6], R[9], 0);
    R[6] = gsu_ldw(gsu_code_then(0x04B0B6, 8, R[6]));
    { gsu_code(0x04B0BE, 11); uint32_t p = gsu_lmult(R[0], R[6]); R[4] = (uint16_t)p; R[0] = (uint16_t)(p >> 16); }
    R[0] = gsu_lob(R[0]);
    R[0] = gsu_swap(R[0]);
    R[4] = gsu_hib(R[4]);
    R[0] = gsu_or(R[0], R[4]);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = !gsu.s;
    if (t) { gsu_code(0x04B0DB, 9); goto lB0DB; } else { gsu_code(0x04B0C9, 8); }
    R[0] = gsu_div2(R[0]);
    R[0] = gsu_add(R[0], R[7], 0);
    R[1] = gsu_ldw(FX_WHEEL_FLAGS);
    R[4] = 0x0100;
    R[4] = gsu_and(R[4], R[1]);
    t = gsu.z;
    if (t) { gsu_code(0x04B0D1, 7); goto lB0E4; }
    t = 1;
    R[0] = gsu_sub(R[0], R[0], 0);
    if (t) goto lB0E4;
lB0DB:
    R[1] = 0x07D0;
    gsu_sub(R[0], R[1], 0);   /* CMP */
    t = !gsu.s;
    if (t) goto lB0E4;
    R[0] = gsu_add(R[0], R[7], 0);
lB0E4:
    R[1] = R[0];
    R[6] = gsu_ldw(gsu_code_then(0x04B0E4, 7, (FX_SIZES + 2)));
    R[0] = R[1];
    R[0] = gsu_fmult(R[0], R[6]);
    R[5] = R[0];
    R[6] = R[6];
    gsu_moves(R[6]);
    t = !gsu.z;
    if (t) { gsu_code(0x04B0EB, 9); gsu_code(0x04B133, 5); goto lB133; }
    R[0] = R[2];
    gsu_moves(R[2]);
    t = !gsu.s;
    if (t) goto lB0FA;
    R[0] = gsu_sub(R[0], R[0], 0);
lB0FA:
    R[0] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_add(R[0], R[7], 0);
    R[1] = R[0];
    R[6] = 0x1000;
    R[0] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_fmult(R[0], R[6]);
    R[1] = gsu_add(R[1], R[0], 0);
    R[6] = R[0];
    gsu_moves(R[0]);
    t = !gsu.s;
    if (t) goto lB10E;
    R[6] = gsu_not(R[6]);
    R[6] = gsu_inc(R[6]);
lB10E:
    R[0] = 0x0064;
    gsu_sub(R[0], R[6], 0);   /* CMP */
    t = !gsu.s;
    if (t) goto lB133;
    R[0] = 0x003C;
    R[0] = gsu_add(R[0], R[9], 0);
    R[0] = gsu_ldw(R[0]);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = !gsu.s;
    if (t) goto lB12A;
    R[0] = gsu_ldw(FX_HIT_DEPTH);
    R[6] = 0x0100;
    R[0] = gsu_or(R[0], R[6]);
    t = 1;
    gsu_sbk(R[0]);
    if (t) goto lB133;
lB12A:
    R[0] = gsu_ldw(FX_HIT_DEPTH);
    R[6] = 0x0200;
    R[0] = gsu_or(R[0], R[6]);
    gsu_sbk(R[0]);
lB133:
    R[0] = gsu_ldw(FX_SIZES);
    R[6] = gsu_ldw(gsu_code_then(0x04B138, 4, (FX_SIZES + 4)));
    R[0] = gsu_or(R[0], R[6]);
    t = gsu.z;
    if (t) { gsu_code(0x04B13C, 3); gsu_code(0x04B148, 1); goto lB148; } else { gsu_code(0x04B13C, 8); }
    R[0] = gsu_ldw(FX_HIT_DEPTH);
    R[6] = 0x0400;
    R[0] = gsu_or(R[0], R[6]);
    gsu_code(0x04B144, 5);
    gsu_sbk(R[0]);
lB148:
    R[6] = gsu_ldw(gsu_code_then(0x04B149, 4, FX_SIZES));
    R[0] = R[1];
    R[0] = gsu_fmult(R[0], R[6]);
    R[4] = R[0];
    R[6] = gsu_ldw(gsu_code_then(0x04B14D, 9, (FX_SIZES + 4)));
    R[0] = R[1];
    R[0] = gsu_fmult(R[0], R[6]);
    R[6] = R[0];
    R[11] = 0xB15F;
    abi_FxSumsPushWorld();
lB15F:
    R[6] = gsu_ldw(FX_SIZES);
    R[0] = gsu_fmult(R[7], R[6]);
    R[1] = gsu_ldw(gsu_code_then(0x04B164, 6, FX_CONTACT_SUM));
    R[0] = gsu_add(R[0], R[1], 0);
    gsu_code(0x04B16A, 2);
    gsu_sbk(R[0]);
    R[6] = gsu_ldw(gsu_code_then(0x04B16C, 4, (FX_SIZES + 2)));
    R[0] = gsu_fmult(R[7], R[6]);
    R[2] = R[0];
    R[1] = gsu_ldw(gsu_code_then(0x04B170, 8, (FX_CONTACT_SUM + 2)));
    R[0] = gsu_add(R[0], R[1], 0);
    gsu_code(0x04B178, 2);
    gsu_sbk(R[0]);
    R[6] = gsu_ldw(gsu_code_then(0x04B17A, 4, (FX_SIZES + 4)));
    R[0] = gsu_fmult(R[7], R[6]);
    R[1] = gsu_ldw(gsu_code_then(0x04B17E, 6, (FX_CONTACT_SUM + 4)));
    R[0] = gsu_add(R[0], R[1], 0);
    gsu_code(0x04B184, 2);
    gsu_sbk(R[0]);
lB185:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x04B186, 4);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
lB18A:
    R[0] = 0x0008;
    R[3] = gsu_add(R[3], R[0], 0);
    nx = (uint16_t)(R[11]);
    at = nx;
    goto dispatch;
lB191:
    R[0] = 0x0006;
    R[3] = gsu_add(R[3], R[0], 0);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
lB19A:
    R[0] = 0x0008;
    R[3] = gsu_add(R[3], R[0], 0);
    nx = (uint16_t)(R[11]);
    at = nx;
    goto dispatch;
    /* FxCarHandler */
lB1A1:
    gsu_stw(R[10], R[11]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[1] = 0x0004;
    R[0] = 0x001C;
    R[0] = gsu_add(R[0], R[8], 0);
    R[0] = gsu_ldw(gsu_code_then(0x04B1A4, 9, R[0]));
    R[0] = gsu_or(R[0], R[1]);
    gsu_code(0x04B1AD, 2);
    gsu_sbk(R[0]);
    R[0] = 0x003E;
    R[0] = gsu_add(R[0], R[8], 0);
    R[0] = gsu_ldw(gsu_code_then(0x04B1AF, 4, R[0]));
lB1B2:
    R[2] = R[0];
    gsu_code(0x04B1B3, 4);
    gsu_moves(R[0]);
    t = gsu.z;
    if (t) { gsu_code(0x04B1C4, 5); goto lB1C4; } else { gsu_code(0x04B1B7, 5); }
    R[0] = 0x001C;
    R[0] = gsu_add(R[0], R[2], 0);
    R[0] = gsu_ldw(R[0]);
    R[0] = gsu_or(R[0], R[1]);
    gsu_code(0x04B1BC, 2);
    gsu_sbk(R[0]);
    R[0] = 0x0034;
    R[0] = gsu_add(R[0], R[2], 0);
    R[0] = gsu_ldw(gsu_code_then(0x04B1BE, 4, R[0]));
    t = 1;
    if (t) { gsu_code(0x04B1B2, 1); gsu_code(0x04B1C2, 2); goto lB1B2; }
lB1C4:
    R[0] = gsu_ldw(FX_INIT_2472);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.z;
    if (t) { gsu_code(0x04B1C9, 4); gsu_code(0x04B1D7, 5); goto lB1D7; }
    R[11] = 0xB1D2;
    abi_FxCarPose();
lB1D2:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
lB1D7:
    R[0] = 0x001C;
    R[0] = gsu_add(R[0], R[8], 0);
    R[0] = gsu_ldw(R[0]);
    R[1] = 0x0002;
    R[0] = gsu_bic(R[0], R[1]);
    gsu_code(0x04B1DC, 6);
    gsu_sbk(R[0]);
    R[1] = 0x0000;
    R[0] = 0x0092;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04B1E2, 8);
    gsu_stw(R[0], R[1]);
    R[0] = 0x0009;
    gsu_code(0x04B1EA, 8);
    gsu_romb(R[0]);
    R[0] = 0x006E;
    R[0] = gsu_add(R[0], R[8], 0);
    R[0] = gsu_ldw(R[0]);
    gsu_code(0x04B1F2, 2);
    gsu_r14(R[0]);
    R[1] = 0x2652;
    R[12] = 0x0019;
    R[13] = 0xB1FB;
lB1FB:
    R[0] = (gsu_code(0x04B1FB, 2), gsu_getb());
    gsu_code(0x04B1FD, 1);
    gsu_r14(gsu_inc(R[14]));
    R[0] = gsu_getbh(gsu_code_then(0x04B1FE, 2, R[0]));
    gsu_code(0x04B200, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x04B201, 1);
    gsu_stw(R[1], R[0]);
    R[1] = gsu_inc(R[1]);
    R[1] = gsu_inc(R[1]);
    t = gsu_count_down(&R[12]);
    if (t) { gsu_code(0x04B202, 3); goto lB1FB; } else { gsu_code(0x04B202, 10); }
    R[1] = 0x001C;
    R[1] = gsu_add(R[1], R[8], 0);
    R[1] = gsu_ldw(R[1]);
    R[0] = 0x0020;
    R[0] = gsu_and(R[0], R[1]);
    t = gsu.z;
    if (t) goto lB216;
    goto lB990;
lB216:
    R[0] = 0x0010;
    R[0] = gsu_and(R[0], R[1]);
    t = gsu.z;
    if (t) goto lB23B;
    R[0] = 0x0064;
    R[0] = gsu_add(R[0], R[8], 0);
    R[0] = gsu_ldw(R[0]);
    R[1] = 0x0008;
    R[0] = gsu_and(R[0], R[1]);
    t = !gsu.z;
    if (t) goto lB22D;
    goto lB990;
lB22D:
    R[11] = 0xB232;
    goto lC3D5;
lB232:
    R[0] = R[0];
    gsu_moves(R[0]);
    t = !gsu.z;
    if (t) goto lB23B;
    goto lB990;
lB23B:
    R[1] = 0x0006;
    R[11] = 0xB242;
    abi_FxRecordMove();
lB242:
    gsu_stw(R[10], R[8]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[0] = 0x0026;
    R[0] = gsu_add(R[0], R[8], 0);
    R[0] = gsu_ldw(gsu_code_then(0x04B245, 6, R[0]));
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_code(0x04B24B, 5);
    gsu_stw(GSU_IN_20, R[0]);
    R[0] = 0x0024;
    R[0] = gsu_add(R[0], R[8], 0);
    R[0] = gsu_ldw(gsu_code_then(0x04B250, 4, R[0]));
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_code(0x04B254, 5);
    gsu_stw(GSU_IN_22, R[0]);
    R[0] = 0x0028;
    R[0] = gsu_add(R[0], R[8], 0);
    R[0] = gsu_ldw(gsu_code_then(0x04B259, 4, R[0]));
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_code(0x04B25D, 5);
    gsu_stw(GSU_IN_24, R[0]);
    R[9] = 0x00E4;
    R[11] = 0xB269;
    abi_FxMatrixFromAngles();
lB269:
    R[9] = 0x00E4;
    R[11] = 0xB271;
    abi_FxMatrixTranspose04();
lB271:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    R[8] = gsu_ldw(R[10]);
    R[0] = 0x0064;
    R[0] = gsu_add(R[0], R[8], 0);
    R[0] = gsu_ldw(gsu_code_then(0x04B276, 4, R[0]));
    R[1] = 0x0008;
    R[0] = gsu_and(R[0], R[1]);
    t = gsu.z;
    if (t) goto lB28E; else { gsu_code(0x04B27A, 11); gsu_code(0x04C3D5, 3); }
    R[11] = 0xB285;
    goto lC3D5;
lB285:
    R[0] = R[0];
    gsu_code(0x04B285, 5); gsu_code(0x04B29A, 6);
    gsu_moves(R[0]);
    t = !gsu.z;
    if (t) goto lB29A;
    goto lB990;
lB28E:
    R[11] = 0xB293;
    abi_FxRecordSegment();
lB293:
    R[7] = R[0];
    R[0] = 0x0058;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_stw(R[0], R[7]);
lB29A:
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_stw(FX_HIT, R[0]);
    gsu_code(0x04B2A0, 4);
    gsu_stw((FX_HIT + 2), R[0]);
    gsu_code(0x04B2A4, 4);
    gsu_stw((FX_HIT + 4), R[0]);
    gsu_code(0x04B2A8, 4);
    gsu_stw((FX_SPIN + 2), R[0]);
    gsu_code(0x04B2AC, 4);
    gsu_stw(FX_SPIN, R[0]);
    gsu_code(0x04B2B0, 4);
    gsu_stw((FX_SPIN + 4), R[0]);
    R[0] = 0x004A;
    R[0] = gsu_add(R[0], R[8], 0);
    R[0] = gsu_ldw(gsu_code_then(0x04B2B4, 4, R[0]));
    R[6] = gsu_ldw(gsu_code_then(0x04B2B8, 4, FX_PARAM_SPIN_DAMP));
    R[0] = gsu_fmult(R[0], R[6]);
    t = !gsu.s;
    if (t) { gsu_code(0x04B2BC, 3); gsu_code(0x04B2C1, 10); goto lB2C1; }
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
lB2C1:
    R[6] = 0x0078;
    R[6] = gsu_add(R[6], R[0], 0);
    R[0] = 0x001C;
    R[0] = gsu_add(R[0], R[8], 0);
    R[0] = gsu_ldw(R[0]);
    R[1] = 0x0008;
    R[0] = gsu_and(R[0], R[1]);
    t = !gsu.z;
    if (t) { gsu_code(0x04B2CB, 6); gsu_code(0x04B2D6, 5); goto lB2D6; }
    R[0] = 0x003C;
    R[6] = gsu_add(R[6], R[0], 0);
lB2D6:
    gsu_stw((FX_HIT + 2), R[6]);
    R[0] = 0x001E;
    R[0] = gsu_add(R[0], R[8], 0);
    R[0] = gsu_ldw(gsu_code_then(0x04B2DB, 4, R[0]));
    gsu_code(0x04B2DF, 4);
    gsu_stw(FX_CAR_POS, R[0]);
    R[0] = 0x0020;
    R[0] = gsu_add(R[0], R[8], 0);
    R[0] = gsu_ldw(gsu_code_then(0x04B2E3, 4, R[0]));
    gsu_code(0x04B2E7, 4);
    gsu_stw((FX_CAR_POS + 2), R[0]);
    R[0] = 0x0022;
    R[0] = gsu_add(R[0], R[8], 0);
    R[0] = gsu_ldw(gsu_code_then(0x04B2EB, 4, R[0]));
    gsu_code(0x04B2EF, 4);
    gsu_stw((FX_CAR_POS + 4), R[0]);
    R[0] = 0x008A;
    R[0] = gsu_add(R[0], R[8], 0);
    R[0] = gsu_ldw(gsu_code_then(0x04B2F3, 5, R[0]));
    gsu_code(0x04B2F8, 4);
    gsu_stw((FX_CAR_POS + 6), R[0]);
    R[0] = 0x0040;
    R[0] = gsu_add(R[0], R[8], 0);
    R[0] = gsu_ldw(gsu_code_then(0x04B2FC, 4, R[0]));
    R[1] = 0x0100;
    R[2] = 0x086A;
    R[0] = gsu_bic(R[0], R[2]);
    R[0] = gsu_or(R[0], R[1]);
    gsu_code(0x04B300, 10);
    gsu_sbk(R[0]);
    R[2] = 0x0042;
    R[2] = gsu_add(R[2], R[8], 0);
    R[2] = gsu_ldw(gsu_code_then(0x04B30A, 6, R[2]));
    R[1] = 0x0064;
    R[1] = gsu_add(R[1], R[8], 0);
    R[1] = gsu_ldw(gsu_code_then(0x04B310, 6, R[1]));
    R[0] = 0x0008;
    R[0] = gsu_and(R[0], R[1]);
    t = gsu.z;
    if (t) goto lB337; else { gsu_code(0x04B316, 11); }
    R[0] = 0x007C;
    R[0] = gsu_add(R[0], R[8], 0);
    R[0] = gsu_ldw(R[0]);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.z;
    if (t) { gsu_code(0x04B321, 4); gsu_code(0x04B32E, 9); gsu_code(0x04B37D, 5); goto lB32E; }
    R[3] = 0x3E80;
    R[7] = 0x3E80;
    t = 1;
    if (t) goto lB37D;
lB32E:
    R[3] = 0x7D00;
    R[7] = 0x7D00;
    t = 1;
    if (t) goto lB37D;
lB337:
    R[3] = 0x3E80;
    R[7] = 0x4650;
    R[0] = 0x0040;
    R[0] = gsu_add(R[0], R[8], 0);
    R[0] = gsu_ldw(R[0]);
    R[1] = 0x0010;
    R[0] = gsu_and(R[0], R[1]);
    t = gsu.z;
    if (t) goto lB34C;
    goto lB37D;
lB34C:
    R[1] = gsu_ldw((FX_CAR_POS + 6));
    R[0] = 0x0004;
    R[0] = gsu_and(R[0], R[1]);
    t = gsu.z;
    if (t) goto lB360;
    R[3] = 0x445C;
    R[7] = 0x4524;
    t = 1;
    if (t) goto lB37D;
lB360:
    R[0] = 0x0001;
    R[0] = gsu_and(R[0], R[1]);
    t = gsu.z;
    if (t) goto lB370;
    R[3] = 0x3E80;
    R[7] = 0x4074;
    t = 1;
    if (t) goto lB37D;
lB370:
    R[0] = 0x0042;
    R[0] = gsu_and(R[0], R[1]);
    t = gsu.z;
    if (t) goto lB37D;
    R[3] = 0x4268;
    R[7] = 0x3A98;
lB37D:
    gsu_stw(FX_CONTACT_FORCE_SIDE, R[3]);
    gsu_code(0x04B382, 4);
    gsu_stw(FX_CONTACT_FORCE_UP, R[7]);
    R[3] = 0x00F0;
    R[0] = 0x0400;
    R[0] = gsu_and(R[0], R[2]);
    t = gsu.z;
    if (t) goto lB392; else { gsu_code(0x04B38F, 3); }
    R[3] = 0x014A;
lB392:
    R[4] = 0x0040;
    R[4] = gsu_add(R[4], R[8], 0);
    R[4] = gsu_ldw(gsu_code_then(0x04B386, 9, gsu_code_then(0x04B392, 7, R[4])));
    R[0] = 0x2000;
    R[0] = gsu_and(R[0], R[4]);
    t = !gsu.z;
    if (t) goto lB3D5; else { gsu_code(0x04B399, 11); }
    R[0] = gsu_ldw((FX_CAR_POS + 6));
    R[1] = 0x0080;
    R[0] = gsu_and(R[0], R[1]);
    t = !gsu.z;
    if (t) goto lB3BA; else { gsu_code(0x04B3A4, 18); }
    R[0] = 0x0010;
    R[0] = gsu_and(R[0], R[4]);
    t = !gsu.z;
    if (t) goto lB3D5;
    R[0] = 0x0046;
    R[0] = gsu_add(R[0], R[8], 0);
    R[0] = gsu_ldw(R[0]);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.z;
    if (t) { gsu_code(0x04B3B6, 4); goto lB3D5; } else { gsu_code(0x04B3B6, 9); }
lB3BA:
    R[0] = 0x004A;
    R[0] = gsu_add(R[0], R[8], 0);
    R[0] = gsu_ldw(R[0]);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = !gsu.s;
    if (t) { gsu_code(0x04B3BF, 4); gsu_code(0x04B3C5, 16); goto lB3C5; }
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
lB3C5:
    gsu_sub(R[0], R[3], 0);   /* CMP */
    t = gsu.s;
    if (t) goto lB3D5;
    R[1] = 0x0040;
    R[2] = gsu_or(R[2], R[1]);
    R[1] = 0x0430;
    R[2] = gsu_bic(R[2], R[1]);
lB3D5:
    R[0] = 0x0044;
    R[0] = gsu_add(R[0], R[8], 0);
    R[0] = gsu_ldw(gsu_code_then(0x04B3D5, 5, R[0]));
    R[0] = gsu_sub(R[0], 3, 0);
    t = gsu.s;
    if (t) { gsu_code(0x04B3DA, 4); gsu_code(0x04B3ED, 1); goto lB3ED; } else { gsu_code(0x04B3DA, 15); }
    R[1] = 0x0430;
    R[2] = gsu_bic(R[2], R[1]);
    R[0] = 0x0040;
    R[0] = gsu_add(R[0], R[8], 0);
    R[0] = gsu_ldw(R[0]);
    R[1] = 0x0008;
    R[0] = gsu_or(R[0], R[1]);
    gsu_code(0x04B3E9, 5);
    gsu_sbk(R[0]);
lB3ED:
    R[0] = 0x0054;
    R[0] = gsu_add(R[0], R[8], 0);
    R[0] = gsu_ldw(gsu_code_then(0x04B3EE, 4, R[0]));
    R[0] = R[0];
    gsu_moves(R[0]);
    t = !gsu.z;
    if (t) goto lB401; else { gsu_code(0x04B3F2, 9); }
    R[0] = 0x0040;
    R[0] = gsu_add(R[0], R[8], 0);
    R[0] = gsu_ldw(R[0]);
    R[1] = 0x0004;
    R[0] = gsu_and(R[0], R[1]);
    t = gsu.z;
    if (t) { gsu_code(0x04B3FB, 6); gsu_code(0x04B407, 5); goto lB407; }
lB401:
    R[1] = 0x043F;
    R[2] = gsu_bic(R[2], R[1]);
lB407:
    R[0] = 0x005E;
    R[0] = gsu_add(R[0], R[8], 0);
    R[0] = gsu_ldw(R[0]);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.z;
    if (t) { gsu_code(0x04B40C, 4); gsu_code(0x04B415, 7); gsu_code(0x04B436, 5); gsu_code(0x04BA05, 5); goto lB415; }
    R[0] = gsu_dec(R[0]);
    gsu_sbk(R[0]);
    t = 1;
    if (t) goto lB436;
lB415:
    R[0] = 0x0080;
    R[0] = gsu_and(R[0], R[2]);
    t = gsu.z;
    if (t) goto lB436;
    R[1] = gsu_ldw(FX_PARAM_LIFT);
    R[1] = R[1];
    gsu_moves(R[1]);
    t = gsu.z;
    if (t) goto lB436;
    R[0] = gsu_ldw((FX_HIT + 2));
    R[0] = gsu_add(R[0], R[1], 0);
    gsu_stw((FX_HIT + 2), R[0]);
    R[1] = 0x0020;
    R[0] = 0x005E;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_stw(R[0], R[1]);
lB436:
    R[11] = 0xB43B;
    abi_FxSteerFromInput();
lB43B:
    R[1] = 0x0070;
    R[1] = gsu_add(R[1], R[8], 0);
    R[1] = gsu_ldw(gsu_code_then(0x04B43B, 7, gsu_code_then(0x04BB14, 3, R[1])));
    R[0] = 0x0008;
    R[0] = gsu_and(R[0], R[1]);
    t = gsu.z;
    if (t) { gsu_code(0x04B442, 6); gsu_code(0x04B44A, 6); goto lB44A; }
    R[3] = gsu_add(R[3], R[3], 0);
lB44A:
    R[0] = 0x003A;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_stw(R[0], R[3]);
    R[1] = 0x0000;
    R[0] = 0x0020;
    R[0] = gsu_and(R[0], R[2]);
    t = gsu.z;
    if (t) { gsu_code(0x04B450, 8); gsu_code(0x04B45F, 7); goto lB45F; }
    R[1] = gsu_ldw(FX_PARAM_REVERSE);
    R[1] = gsu_not(R[1]);
    R[1] = gsu_inc(R[1]);
lB45F:
    R[0] = 0x0400;
    R[0] = gsu_and(R[0], R[2]);
    t = gsu.z;
    if (t) { gsu_code(0x04B4A4, 7); gsu_code(0x04B4AF, 1); goto lB4A4; } else { gsu_code(0x04B466, 5); }
    R[1] = gsu_ldw(FX_PARAM_THRUST_BOOST);
    R[6] = 0x006A;
    R[6] = gsu_add(R[6], R[8], 0);
    R[6] = gsu_ldw(gsu_code_then(0x04B46B, 6, R[6]));
    R[6] = R[6];
    gsu_moves(R[6]);
    t = gsu.z;
    if (t) goto lB4AB; else { gsu_code(0x04B471, 11); }
    R[4] = 0x0040;
    R[4] = gsu_add(R[4], R[8], 0);
    R[4] = gsu_ldw(R[4]);
    R[0] = 0x2000;
    R[0] = gsu_and(R[0], R[4]);
    t = !gsu.z;
    if (t) goto lB49D; else { gsu_code(0x04B47C, 11); }
    R[0] = gsu_ldw(FX_PARAM_SPEED_CAP);
    R[6] = gsu_sub(R[6], R[0], 0);
    t = !gsu.s;
    if (t) { gsu_code(0x04B487, 4); gsu_code(0x04B48D, 5); goto lB48D; }
    R[6] = 0x0000;
lB48D:
    R[0] = gsu_ldw(GSU_CONTROL_26F2);
    R[1] = 0x0200;
    R[0] = gsu_and(R[0], R[1]);
    t = !gsu.z;
    if (t) goto lB49D; else { gsu_code(0x04B492, 12); }
    R[0] = 0x006A;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_stw(R[0], R[6]);
lB49D:
    R[1] = gsu_ldw(gsu_code_then(0x04B49E, 4, FX_PARAM_THRUST));
    gsu_code(0x04B4A2, 2); gsu_code(0x04B4AF, 1);
    t = 1;
    if (t) goto lB4AF;
lB4A4:
    R[0] = 0x0010;
    R[0] = gsu_and(R[0], R[2]);
    t = gsu.z;
    if (t) goto lB4AF; else { gsu_code(0x04B4AB, 4); }
lB4AB:
    R[1] = gsu_ldw(FX_PARAM_THRUST_BOOST);
lB4AF:
    R[4] = 0x0040;
    R[4] = gsu_add(R[4], R[8], 0);
    R[4] = gsu_ldw(gsu_code_then(0x04B4B0, 6, R[4]));
    R[0] = 0x2000;
    R[0] = gsu_and(R[0], R[4]);
    t = gsu.z;
    if (t) { gsu_code(0x04B4B6, 6); gsu_code(0x04B4C0, 6); goto lB4C0; }
    R[1] = gsu_add(R[1], R[1], 0);
    R[1] = gsu_add(R[1], R[1], 0);
lB4C0:
    R[0] = 0x0078;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_stw(R[0], R[1]);
    R[0] = 0x0009;
    gsu_code(0x04B4C6, 9);
    gsu_romb(R[0]);
    gsu_r14(gsu_ldw(FX_PARAM_CURVE_A));
    R[4] = 0x0001;
    R[5] = 0x0000;
    R[3] = 0x004A;
    R[3] = gsu_add(R[3], R[8], 0);
    R[3] = gsu_ldw(gsu_code_then(0x04B4CF, 10, R[3]));
    R[3] = R[3];
    gsu_moves(R[3]);
    t = !gsu.s;
    if (t) { gsu_code(0x04B4D9, 4); goto lB4F1; }
    R[0] = 0x0410;
    R[0] = gsu_and(R[0], R[2]);
    t = gsu.z;
    if (t) goto lB4EC;
    R[4] = 0x0001;
    R[5] = 0x000F;
    goto lB562;
lB4EC:
    R[4] = 0x0000;
    t = 1;
    if (t) goto lB506;
lB4F1:
    R[0] = (gsu_code(0x04B4F1, 2), gsu_getb());
    gsu_code(0x04B4F3, 1);
    gsu_r14(gsu_inc(R[14]));
    R[0] = gsu_getbh(gsu_code_then(0x04B4F4, 2, R[0]));
    gsu_code(0x04B4F6, 1);
    gsu_r14(gsu_inc(R[14]));
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.z;
    if (t) goto lB505; else { gsu_code(0x04B4F7, 9); }
    gsu_sub(R[0], R[3], 0);   /* CMP */
    t = !gsu.s;
    if (t) { gsu_code(0x04B506, 5); goto lB506; } else { gsu_code(0x04B500, 5); }
    R[5] = R[0];
    t = 1;
    R[4] = gsu_inc(R[4]);
    if (t) goto lB4F1;
lB505:
    R[4] = gsu_dec(R[4]);
lB506:
    R[0] = gsu_ldw(FX_INIT_2470);
    R[0] = gsu_and(R[0], 3);
    t = !gsu.z;
    if (t) { gsu_code(0x04B50B, 4); gsu_code(0x04B529, 1); goto lB529; } else { gsu_code(0x04B50B, 9); }
    R[0] = 0x004E;
    R[0] = gsu_add(R[0], R[8], 0);
    R[0] = gsu_ldw(R[0]);
    gsu_sub(R[0], R[4], 0);   /* CMP */
    t = gsu.z;
    if (t) { gsu_code(0x04B514, 4); gsu_code(0x04B529, 1); goto lB529; } else { gsu_code(0x04B514, 7); }
    gsu_sbk(R[4]);
    R[0] = gsu_sub(R[4], 4, 0);
    t = !gsu.z;
    if (t) { gsu_code(0x04B51B, 5); gsu_code(0x04B529, 1); goto lB529; } else { gsu_code(0x04B51B, 10); }
    R[0] = 0x0040;
    R[0] = gsu_add(R[0], R[8], 0);
    R[0] = gsu_ldw(R[0]);
    R[6] = 0x0002;
    R[0] = gsu_or(R[0], R[6]);
    gsu_code(0x04B525, 5);
    gsu_sbk(R[0]);
lB529:
    gsu_code(0x04B52A, 4);
    gsu_r14(gsu_ldw(FX_PARAM_CURVE_B));
    R[0] = 0x004E;
    R[0] = gsu_add(R[0], R[8], 0);
    R[0] = gsu_ldw(gsu_code_then(0x04B52E, 4, R[0]));
    R[0] = gsu_dec(R[0]);
    R[6] = 0x0011;
    R[0] = gsu_mult(R[0], R[6]);
    gsu_code(0x04B532, 7);
    gsu_r14(gsu_add(R[14], R[0], 0));
    R[0] = gsu_sub(R[0], R[0], 0);
    R[6] = gsu_getbh(gsu_code_then(0x04B539, 4, R[0]));
    R[4] = R[6];
    gsu_code(0x04B53D, 3);
    gsu_r14(gsu_inc(R[14]));
    R[0] = 0x004A;
    R[0] = gsu_add(R[0], R[8], 0);
    R[0] = gsu_ldw(gsu_code_then(0x04B540, 4, R[0]));
    R[0] = R[0];
    gsu_moves(R[0]);
    t = !gsu.s;
    if (t) { gsu_code(0x04B544, 4); gsu_code(0x04B54A, 10); gsu_code(0x04B556, 5); goto lB54A; }
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
lB54A:
    R[0] = gsu_sub(R[0], R[5], 0);
    R[0] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_fmult(R[0], R[6]);
    R[3] = 0x000F;
    gsu_sub(R[0], R[3], 0);   /* CMP */
    t = gsu.s;
    if (t) goto lB556; else { gsu_code(0x04B554, 2); }
    R[0] = R[3];
lB556:
    R[5] = R[0];
    gsu_r14(gsu_add(R[14], R[0], 0));
    R[0] = gsu_sub(R[0], R[0], 0);
    R[6] = gsu_getbh(gsu_code_then(0x04B55B, 4, R[0]));
    R[1] = gsu_add(R[1], R[1], 0);
    R[1] = gsu_fmult(R[1], R[6]);
lB562:
    R[0] = 0x003C;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04B55F, 9);
    gsu_stw(R[0], R[1]);
    R[1] = 0x0050;
    R[1] = gsu_add(R[1], R[8], 0);
    R[1] = gsu_ldw(gsu_code_then(0x04B568, 6, R[1]));
    R[0] = R[5];
    R[6] = 0x0040;
    { gsu_code(0x04B56E, 14); uint32_t p = gsu_lmult(R[0], R[6]); R[4] = (uint16_t)p; R[0] = (uint16_t)(p >> 16); }
    R[0] = gsu_sub(R[4], R[1], 0);
    R[0] = gsu_div2(R[0]);
    R[0] = gsu_div2(R[0]);
    R[0] = gsu_add(R[0], R[1], 0);
    gsu_sbk(R[0]);
    R[0] = 0x0800;
    R[0] = gsu_and(R[0], R[2]);
    t = gsu.z;
    if (t) { gsu_code(0x04B57C, 6); gsu_code(0x04B5AE, 10); goto lB5AE; }
    gsu_stw(R[10], R[2]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[4] = 0x0000;
    R[5] = 0x0000;
    R[6] = 0x0258;
    R[11] = 0xB594;
    abi_FxSumsPushLocal();
lB594:
    R[11] = 0xB599;
    abi_FxSumsRotateLocal();
lB599:
    R[1] = 0x0000;
    R[2] = 0x0000;
    R[3] = 0x0000;
    R[6] = 0x7FFF;
    R[11] = 0xB5AA;
    abi_FxHitAddScaled();
lB5AA:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    R[2] = gsu_ldw(R[10]);
lB5AE:
    R[1] = 0x0000;
    R[0] = 0x0410;
    R[0] = gsu_and(R[0], R[2]);
    t = !gsu.z;
    if (t) { gsu_code(0x04B5D6, 7); goto lB5D6; } else { gsu_code(0x04B5B8, 5); }
    R[0] = 0x004A;
    R[0] = gsu_add(R[0], R[8], 0);
    R[0] = gsu_ldw(R[0]);
    R[0] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_add(R[0], R[0], 0);
    t = !gsu.s;
    if (t) { gsu_code(0x04B5BD, 5); gsu_code(0x04B5C4, 5); goto lB5C4; }
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
lB5C4:
    R[6] = gsu_ldw(FX_PARAM_TURN_GRIP);
    R[0] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_fmult(R[0], R[6]);
    R[1] = 0x0800;
    gsu_sub(R[0], R[1], 0);   /* CMP */
    t = !gsu.s;
    if (t) goto lB5D4; else { gsu_code(0x04B5D2, 2); }
    R[0] = R[1];
lB5D4:
    R[1] = R[0];
gsu_code(0x04B5C9, 9); gsu_code(0x04B5D4, 9);
lB5D6:
    R[0] = 0x0040;
    R[0] = gsu_and(R[0], R[2]);
    t = gsu.z;
    if (t) { gsu_code(0x04B5FF, 1); goto lB5FF; } else { gsu_code(0x04B5DD, 5); }
    R[0] = 0x004A;
    R[0] = gsu_add(R[0], R[8], 0);
    R[0] = gsu_ldw(R[0]);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.s;
    if (t) goto lB5FF; else { gsu_code(0x04B5E2, 21); }
    R[3] = R[0];
    R[0] = 0x00C8;
    gsu_sub(R[0], R[3], 0);   /* CMP */
    t = !gsu.s;
    if (t) goto lB5FB;
    R[1] = 0x0040;
    R[1] = gsu_add(R[1], R[8], 0);
    R[1] = gsu_ldw(R[1]);
    R[0] = 0x0800;
    R[0] = gsu_or(R[0], R[1]);
    gsu_code(0x04B5F7, 5);
    gsu_sbk(R[0]);
lB5FB:
    R[1] = gsu_ldw(gsu_code_then(0x04B5FC, 4, FX_PARAM_WHEEL_48));
lB5FF:
    R[0] = 0x0048;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04B600, 5);
    gsu_stw(R[0], R[1]);
    R[0] = 0x0042;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04B605, 5);
    gsu_stw(R[0], R[2]);
    R[1] = 0x0040;
    R[0] = 0x002C;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04B60A, 8);
    gsu_stw(R[0], R[1]);
    R[11] = 0xB616;
    abi_FxCarMoveParts();
lB616:
    R[11] = 0xB61B;
    goto lA607;
lB61B:
    R[11] = 0xB620;
    gsu_code(0x04B61B, 5); gsu_code(0x04BC7B, 5);
    abi_FxRecordHoldY();
lB620:
    R[2] = 0x0042;
    R[2] = gsu_add(R[2], R[8], 0);
    R[2] = gsu_ldw(gsu_code_then(0x04B620, 7, gsu_code_then(0x04BC9C, 4, gsu_code_then(0x04BCA2, 3, R[2]))));
    R[0] = 0x0400;
    R[0] = gsu_and(R[0], R[2]);
    t = gsu.z;
    if (t) { gsu_code(0x04B627, 6); goto lB653; } else { gsu_code(0x04B627, 11); }
    R[2] = gsu_ldw(FX_PARAM_JUMP);
    R[2] = R[2];
    gsu_moves(R[2]);
    t = gsu.z;
    if (t) { gsu_code(0x04B632, 4); goto lB653; }
    R[0] = 0x004E;
    R[0] = gsu_add(R[0], R[8], 0);
    R[0] = gsu_ldw(R[0]);
    R[1] = 0x0002;
    gsu_sub(R[0], R[1], 0);   /* CMP */
    t = !gsu.s;
    if (t) goto lB653;
    R[0] = 0x0026;
    R[0] = gsu_add(R[0], R[8], 0);
    R[0] = gsu_ldw(R[0]);
    R[1] = 0xEAAB;
    gsu_sub(R[0], R[1], 0);   /* CMP */
    t = gsu.s;
    if (t) goto lB653;
    R[0] = gsu_ldw((FX_SPIN + 2));
    R[0] = gsu_sub(R[0], R[2], 0);
    gsu_sbk(R[0]);
lB653:
    R[11] = 0xB658;
    gsu_code(0x04B653, 5); gsu_code(0x04BD20, 3);
    goto lBD20;
lB658:
    R[11] = 0xB65D;
    gsu_code(0x04B658, 5); gsu_code(0x04BCBA, 3);
    abi_FxRecordAlignment();
lB65D:
    R[11] = 0xB662;
    abi_FxCarHit();
lB662:
    R[1] = 0x0070;
    R[1] = gsu_add(R[1], R[8], 0);
    R[1] = gsu_ldw(R[1]);
    R[0] = 0x0080;
    R[0] = gsu_and(R[0], R[1]);
    t = gsu.z;
    if (t) { gsu_code(0x04B669, 6); gsu_code(0x04B6C1, 11); goto lB6C1; }
    R[0] = 0x0058;
    R[0] = gsu_add(R[0], R[8], 0);
    R[0] = gsu_ldw(R[0]);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = !gsu.z;
    if (t) goto lB67C;
    goto lB6B7;
lB67C:
    R[1] = 0x008A;
    R[1] = gsu_add(R[1], R[8], 0);
    R[1] = gsu_ldw(R[1]);
    R[0] = 0x0100;
    R[0] = gsu_and(R[0], R[1]);
    t = !gsu.z;
    if (t) goto lB68E;
    goto lB6B7;
lB68E:
    R[11] = 0xB693;
    abi_FxRecordHeadingAhead();
lB693:
    R[2] = R[0];
    R[1] = 0x0024;
    R[1] = gsu_add(R[1], R[8], 0);
    R[1] = gsu_ldw(R[1]);
    R[0] = gsu_sub(R[0], R[1], 0);
    R[0] = gsu_div2(R[0]);
    R[0] = gsu_div2(R[0]);
    R[0] = gsu_add(R[0], R[1], 0);
    gsu_sbk(R[0]);
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_stw(FX_SPIN, R[0]);
    R[3] = R[0];
    R[0] = 0x0034;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_stw(R[0], R[3]);
    R[0] = gsu_sub(R[1], R[2], 0);
    R[0] = gsu_hib(R[0]);
    R[0] = gsu_sex(R[0]);
    R[0] = gsu_div2(R[0]);
    t = !gsu.z;
    if (t) goto lB6C1;
lB6B7:
    R[1] = 0x0080;
    R[0] = 0x0070;
    R[0] = gsu_add(R[0], R[8], 0);
    R[0] = gsu_ldw(R[0]);
    R[0] = gsu_bic(R[0], R[1]);
    gsu_sbk(R[0]);
lB6C1:
    R[1] = 0xF448;
    R[2] = 0x0BB8;
    R[0] = gsu_ldw((FX_HIT + 2));
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.s;
    if (t) { gsu_code(0x04B6DA, 5); goto lB6DA; } else { gsu_code(0x04B6D0, 5); }
    gsu_sub(R[0], R[2], 0);   /* CMP */
    t = gsu.s;
    if (t) goto lB6E1;
    R[0] = R[2];
    t = 1;
    if (t) goto lB6E1;
lB6DA:
    gsu_sub(R[0], R[1], 0);   /* CMP */
    t = !gsu.s;
    if (t) goto lB6E1;
    R[0] = R[1];
lB6E1:
    gsu_code(0x04B6CC, 4); gsu_code(0x04B6E1, 2);
    gsu_sbk(R[0]);
    R[0] = gsu_ldw(gsu_code_then(0x04B6E3, 4, (FX_SPIN + 4)));
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.s;
    if (t) { gsu_code(0x04B6F5, 5); goto lB6F5; } else { gsu_code(0x04B6EB, 5); }
    gsu_sub(R[0], R[2], 0);   /* CMP */
    t = gsu.s;
    if (t) goto lB6FC;
    R[0] = R[2];
    t = 1;
    if (t) goto lB6FC;
lB6F5:
    gsu_sub(R[0], R[1], 0);   /* CMP */
    t = !gsu.s;
    if (t) goto lB6FC;
    R[0] = R[1];
lB6FC:
    gsu_code(0x04B6E7, 4); gsu_code(0x04B6FC, 2);
    gsu_sbk(R[0]);
    R[0] = gsu_ldw(gsu_code_then(0x04B6FE, 4, (FX_SPIN + 2)));
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.s;
    if (t) { gsu_code(0x04B710, 5); goto lB710; } else { gsu_code(0x04B706, 5); }
    gsu_sub(R[0], R[2], 0);   /* CMP */
    t = gsu.s;
    if (t) goto lB717;
    R[0] = R[2];
    t = 1;
    if (t) goto lB717;
lB710:
    gsu_sub(R[0], R[1], 0);   /* CMP */
    t = !gsu.s;
    if (t) goto lB717;
    R[0] = R[1];
lB717:
    gsu_code(0x04B702, 4); gsu_code(0x04B717, 2);
    gsu_sbk(R[0]);
    R[0] = gsu_ldw(gsu_code_then(0x04B719, 4, FX_SPIN));
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.s;
    if (t) { gsu_code(0x04B72B, 5); goto lB72B; } else { gsu_code(0x04B721, 5); }
    gsu_sub(R[0], R[2], 0);   /* CMP */
    t = gsu.s;
    if (t) goto lB732;
    R[0] = R[2];
    t = 1;
    if (t) goto lB732;
lB72B:
    gsu_sub(R[0], R[1], 0);   /* CMP */
    t = !gsu.s;
    if (t) goto lB732;
    R[0] = R[1];
lB732:
    gsu_code(0x04B71D, 4); gsu_code(0x04B732, 2);
    gsu_sbk(R[0]);
    R[2] = 0x0000;
    R[0] = 0x0028;
    R[0] = gsu_add(R[0], R[8], 0);
    R[0] = gsu_ldw(gsu_code_then(0x04B734, 6, R[0]));
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.s;
    if (t) { gsu_code(0x04B750, 8); goto lB750; } else { gsu_code(0x04B73E, 8); }
    R[1] = 0x2000;
    gsu_sub(R[0], R[1], 0);   /* CMP */
    t = gsu.s;
    if (t) goto lB75F;
    gsu_sbk(R[1]);
    R[0] = 0x0038;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_stw(R[0], R[2]);
    t = 1;
    if (t) goto lB75F;
lB750:
    R[1] = 0xE000;
    gsu_sub(R[0], R[1], 0);   /* CMP */
    t = !gsu.s;
    if (t) goto lB75F;
    gsu_sbk(R[1]);
    R[0] = 0x0038;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_stw(R[0], R[2]);
lB75F:
    R[1] = 0x0040;
    R[1] = gsu_add(R[1], R[8], 0);
    R[1] = gsu_ldw(gsu_code_then(0x04B73A, 4, gsu_code_then(0x04B75F, 7, R[1])));
    R[0] = 0x0000;
    R[0] = gsu_and(R[0], R[1]);
    t = !gsu.z;
    if (t) goto lB799; else { gsu_code(0x04B766, 11); }
    R[0] = 0x0026;
    R[0] = gsu_add(R[0], R[8], 0);
    R[0] = gsu_ldw(R[0]);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.s;
    if (t) { gsu_code(0x04B787, 8); goto lB787; } else { gsu_code(0x04B775, 8); }
    R[1] = 0x2000;
    gsu_sub(R[0], R[1], 0);   /* CMP */
    t = gsu.s;
    if (t) goto lB7CA;
    gsu_sbk(R[1]);
    R[0] = 0x0036;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_stw(R[0], R[2]);
    t = 1;
    if (t) goto lB7CA;
lB787:
    R[1] = 0xE000;
    gsu_sub(R[0], R[1], 0);   /* CMP */
    t = !gsu.s;
    if (t) goto lB7CA;
    gsu_sbk(R[1]);
    R[0] = 0x0036;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_stw(R[0], R[2]);
    t = 1;
    if (t) goto lB7CA;
lB799:
    R[0] = 0x0026;
    R[0] = gsu_add(R[0], R[8], 0);
    R[0] = gsu_ldw(R[0]);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = !gsu.s;
    if (t) goto lB7A4;
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
lB7A4:
    R[1] = 0x4000;
    gsu_sub(R[0], R[1], 0);   /* CMP */
    t = gsu.s;
    if (t) goto lB7CA;
    R[0] = 0x0040;
    R[0] = gsu_add(R[0], R[8], 0);
    R[0] = gsu_ldw(R[0]);
    R[1] = 0x0000;
    R[0] = gsu_or(R[0], R[1]);
    gsu_sbk(R[0]);
    R[0] = 0x0076;
    R[0] = gsu_add(R[0], R[8], 0);
    R[0] = gsu_ldw(R[0]);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = !gsu.z;
    if (t) goto lB7C1;
    R[0] = 0x0028;
    gsu_sbk(R[0]);
lB7C1:
    R[0] = gsu_ldw(FX_SPIN);
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
    t = 1;
    gsu_sbk(R[0]);
    if (t) goto lB7D4;
lB7CA:
    R[0] = 0x0040;
    R[0] = gsu_add(R[0], R[8], 0);
    R[0] = gsu_ldw(gsu_code_then(0x04B771, 4, gsu_code_then(0x04B7CA, 5, R[0])));
    R[1] = 0x0000;
    R[0] = gsu_bic(R[0], R[1]);
    gsu_code(0x04B7CF, 6);
    gsu_sbk(R[0]);
lB7D4:
    R[0] = 0x004A;
    R[0] = gsu_add(R[0], R[8], 0);
    R[0] = gsu_ldw(gsu_code_then(0x04B7D5, 4, R[0]));
    R[0] = R[0];
    gsu_moves(R[0]);
    t = !gsu.s;
    if (t) { gsu_code(0x04B7D9, 4); gsu_code(0x04B7DF, 8); gsu_code(0x04B80F, 2); gsu_cache(0xB810); gsu_code(0x04B811, 11); goto lB7DF; }
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
lB7DF:
    R[1] = 0x0258;
    gsu_sub(R[0], R[1], 0);   /* CMP */
    t = gsu.s;
    R[0] = gsu_sub(R[0], R[0], 0);
    if (t) goto lB80F;
    gsu_stw(FX_HIT, R[0]);
    gsu_stw((FX_HIT + 4), R[0]);
    R[1] = 0x002E;
    R[1] = gsu_add(R[1], R[8], 0);
    R[1] = gsu_ldw(R[1]);
    R[0] = gsu_div2(R[1]);
    R[0] = gsu_div2(R[0]);
    R[0] = gsu_div2(R[0]);
    R[0] = gsu_sub(R[1], R[0], 0);
    gsu_sbk(R[0]);
    R[1] = 0x0032;
    R[1] = gsu_add(R[1], R[8], 0);
    R[1] = gsu_ldw(R[1]);
    R[0] = gsu_div2(R[1]);
    R[0] = gsu_div2(R[0]);
    R[0] = gsu_div2(R[0]);
    R[0] = gsu_sub(R[1], R[0], 0);
    gsu_sbk(R[0]);
lB80F:
    /* CACHE */
    R[4] = 0x2334;
    R[2] = 0x002E;
    R[2] = gsu_add(R[2], R[8], 0);
    R[12] = 0x0006;
    R[13] = 0xB81C;
lB81C:
    R[0] = gsu_ldw(gsu_code_then(0x04B81C, 2, R[4]));
    R[4] = gsu_inc(R[4]);
    R[4] = gsu_inc(R[4]);
    R[3] = gsu_ldw(gsu_code_then(0x04B81E, 4, R[2]));
    R[0] = gsu_add(R[0], R[3], 0);
    gsu_code(0x04B822, 2);
    gsu_stw(R[2], R[0]);
    R[2] = gsu_inc(R[2]);
    R[2] = gsu_inc(R[2]);
    t = gsu_count_down(&R[12]);
    if (t) { gsu_code(0x04B824, 3); goto lB81C; } else { gsu_code(0x04B824, 10); }
    R[2] = 0x0064;
    R[2] = gsu_add(R[2], R[8], 0);
    R[2] = gsu_ldw(R[2]);
    R[0] = 0x0008;
    R[0] = gsu_and(R[0], R[2]);
    t = gsu.z;
    if (t) goto lB838; else { gsu_code(0x04B82E, 10); gsu_code(0x04B920, 7); }
    goto lB920;
lB838:
    R[2] = 0x0042;
    R[2] = gsu_add(R[2], R[8], 0);
    R[2] = gsu_ldw(R[2]);
    R[0] = 0x0430;
    R[0] = gsu_and(R[0], R[2]);
    t = gsu.z;
    if (t) goto lB849;
    goto lB8BD;
lB849:
    R[2] = 0x002C;
    R[2] = gsu_add(R[2], R[8], 0);
    R[2] = gsu_ldw(R[2]);
    R[0] = 0x0400;
    R[0] = gsu_and(R[0], R[2]);
    t = gsu.z;
    if (t) goto lB85A;
    goto lB8BD;
lB85A:
    R[0] = 0x002E;
    R[0] = gsu_add(R[0], R[8], 0);
    R[0] = gsu_ldw(R[0]);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.s;
    if (t) goto lB870;
    R[2] = 0x00C8;
    gsu_sub(R[0], R[2], 0);   /* CMP */
    t = !gsu.s;
    if (t) goto lB87A;
    R[0] = 0x0000;
    t = 1;
    if (t) goto lB87A;
lB870:
    R[2] = 0xFF38;
    gsu_sub(R[0], R[2], 0);   /* CMP */
    t = gsu.s;
    if (t) goto lB87A;
    R[0] = 0x0000;
lB87A:
    gsu_sbk(R[0]);
    R[0] = 0x0032;
    R[0] = gsu_add(R[0], R[8], 0);
    R[0] = gsu_ldw(R[0]);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.s;
    if (t) goto lB891;
    R[2] = 0x00C8;
    gsu_sub(R[0], R[2], 0);   /* CMP */
    t = !gsu.s;
    if (t) goto lB89B;
    R[0] = 0x0000;
    t = 1;
    if (t) goto lB89B;
lB891:
    R[2] = 0xFF38;
    gsu_sub(R[0], R[2], 0);   /* CMP */
    t = gsu.s;
    if (t) goto lB89B;
    R[0] = 0x0000;
lB89B:
    gsu_sbk(R[0]);
    R[0] = 0x0030;
    R[0] = gsu_add(R[0], R[8], 0);
    R[0] = gsu_ldw(R[0]);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.s;
    if (t) goto lB8B2;
    R[2] = 0x0032;
    gsu_sub(R[0], R[2], 0);   /* CMP */
    t = !gsu.s;
    if (t) goto lB8BC;
    R[0] = 0x0000;
    t = 1;
    if (t) goto lB8BC;
lB8B2:
    R[2] = 0xFFCE;
    gsu_sub(R[0], R[2], 0);   /* CMP */
    t = gsu.s;
    if (t) goto lB8BC;
    R[0] = 0x0000;
lB8BC:
    gsu_sbk(R[0]);
lB8BD:
    R[1] = 0x0034;
    R[1] = gsu_add(R[1], R[8], 0);
    R[0] = gsu_ldw(R[1]);
    R[1] = gsu_inc(R[1]);
    R[1] = gsu_inc(R[1]);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.s;
    if (t) goto lB8D7;
    R[2] = 0x0040;
    gsu_sub(R[0], R[2], 0);   /* CMP */
    t = !gsu.s;
    if (t) goto lB8E1;
    R[0] = 0x0000;
    t = 1;
    if (t) goto lB8E1;
lB8D7:
    R[2] = 0xFFC0;
    gsu_sub(R[0], R[2], 0);   /* CMP */
    t = gsu.s;
    if (t) goto lB8E1;
    R[0] = 0x0000;
lB8E1:
    gsu_sbk(R[0]);
    R[0] = gsu_ldw(R[1]);
    R[1] = gsu_inc(R[1]);
    R[1] = gsu_inc(R[1]);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.s;
    if (t) goto lB8F7;
    R[2] = 0x0040;
    gsu_sub(R[0], R[2], 0);   /* CMP */
    t = !gsu.s;
    if (t) goto lB901;
    R[0] = 0x0000;
    t = 1;
    if (t) goto lB901;
lB8F7:
    R[2] = 0xFFC0;
    gsu_sub(R[0], R[2], 0);   /* CMP */
    t = gsu.s;
    if (t) goto lB901;
    R[0] = 0x0000;
lB901:
    gsu_sbk(R[0]);
    R[0] = gsu_ldw(R[1]);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.s;
    if (t) goto lB915;
    R[2] = 0x0040;
    gsu_sub(R[0], R[2], 0);   /* CMP */
    t = !gsu.s;
    if (t) goto lB91F;
    R[0] = 0x0000;
    t = 1;
    if (t) goto lB91F;
lB915:
    R[2] = 0xFFC0;
    gsu_sub(R[0], R[2], 0);   /* CMP */
    t = gsu.s;
    if (t) goto lB91F;
    R[0] = 0x0000;
lB91F:
    gsu_sbk(R[0]);
lB920:
    R[1] = 0x002E;
    R[1] = gsu_add(R[1], R[8], 0);
    R[1] = gsu_ldw(R[1]);
    R[2] = 0x0030;
    R[2] = gsu_add(R[2], R[8], 0);
    R[2] = gsu_ldw(gsu_code_then(0x04B927, 6, R[2]));
    R[3] = 0x0032;
    R[3] = gsu_add(R[3], R[8], 0);
    R[3] = gsu_ldw(gsu_code_then(0x04B92D, 6, R[3]));
    R[6] = gsu_ldw(gsu_code_then(0x04B933, 3, FX_OBJECT_MATRIX_6));
    R[0] = gsu_fmult(R[1], R[6]);
    R[5] = gsu_rol(R[0]);
    R[6] = gsu_ldw(gsu_code_then(0x04B936, 7, FX_OBJECT_MATRIX_7));
    R[0] = gsu_fmult(R[2], R[6]);
    R[0] = gsu_rol(R[0]);
    R[5] = gsu_add(R[5], R[0], 0);
    R[6] = gsu_ldw(gsu_code_then(0x04B93D, 8, FX_OBJECT_MATRIX_8));
    R[0] = gsu_fmult(R[3], R[6]);
    R[0] = gsu_rol(R[0]);
    R[0] = gsu_add(R[0], R[5], 0);
    R[0] = gsu_div2(R[0]);
    R[0] = gsu_div2(R[0]);
    R[0] = gsu_div2(R[0]);
    R[1] = R[0];
    R[0] = 0x004A;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04B945, 17);
    gsu_stw(R[0], R[1]);
    R[0] = 0x0004;
    gsu_code(0x04B956, 10);
    gsu_romb(R[0]);
    R[2] = 0x0050;
    R[2] = gsu_add(R[2], R[8], 0);
    R[2] = gsu_ldw(R[2]);
    R[0] = 0x0044;
    R[0] = gsu_add(R[0], R[8], 0);
    R[0] = gsu_ldw(gsu_code_then(0x04B960, 4, R[0]));
    R[1] = 0x0004;
    gsu_sub(R[0], R[1], 0);   /* CMP */
    t = gsu.s;
    if (t) { gsu_code(0x04B964, 6); gsu_code(0x04B97F, 1); goto lB97F; } else { gsu_code(0x04B964, 15); }
    R[2] = gsu_add(R[2], R[2], 0);
    R[1] = 0x0028;
    R[1] = gsu_add(R[1], R[8], 0);
    R[1] = gsu_ldw(R[1]);
    R[0] = R[1];
    R[0] = gsu_div2(R[0]);
    R[0] = gsu_div2(R[0]);
    R[1] = gsu_sub(R[1], R[0], 0);
    R[0] = 0x0028;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04B973, 13);
    gsu_stw(R[0], R[1]);
lB97F:
    R[1] = 0x0052;
    R[1] = gsu_add(R[1], R[8], 0);
    R[1] = gsu_ldw(gsu_code_then(0x04B980, 6, R[1]));
    R[0] = gsu_sub(R[2], R[1], 0);
    R[0] = gsu_div2(R[0]);
    R[1] = gsu_add(R[0], R[1], 0);
    R[0] = 0x0052;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04B986, 11);
    gsu_stw(R[0], R[1]);
lB990:
    R[11] = 0xB995;
    gsu_code(0x04B991, 4); gsu_code(0x04CDCA, 5);
    abi_FxLapSegment();
lB995:
    R[0] = 0x0086;
    R[0] = gsu_add(R[0], R[8], 0);
    R[0] = gsu_ldw(gsu_code_then(0x04B995, 6, gsu_code_then(0x04CE1A, 3, R[0])));
    R[11] = 0xB99F;
    (gsu_code(0x04B99B, 4), gsu_code(0x04CED6, 3), abi_FxCourseNextDistance());
lB99F:
    R[1] = R[0];
    R[0] = 0x0084;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04B99F, 9);
    gsu_stw(R[0], R[1]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x04B9A8, 4);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
    /* FxCarCollide */
lBD20:
    gsu_stw(R[10], R[11]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_code(0x04BD23, 7);
    gsu_stw(FX_PUSH_TURN, R[0]);
    gsu_code(0x04BD2A, 4);
    gsu_stw(FX_HIT_DEPTH, R[0]);
    gsu_code(0x04BD2E, 4);
    gsu_stw(FX_PUSH_AMOUNT, R[0]);
    gsu_code(0x04BD32, 4);
    gsu_stw(FX_CLOSING, R[0]);
    R[0] = gsu_ldw(gsu_code_then(0x04BD36, 4, FX_HIT_ON));
    R[0] = R[0];
    gsu_moves(R[0]);
    t = !gsu.z;
    if (t) { gsu_code(0x04BD3A, 4); gsu_code(0x04BD46, 1); goto lBD46; } else { gsu_code(0x04BD3A, 13); }
    R[1] = 0x0000;
    R[0] = 0x0094;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_stw(R[0], R[1]);
lBD46:
    R[9] = R[8];
    R[0] = 0x0018;
    R[0] = gsu_add(R[0], R[8], 0);
    R[0] = gsu_ldw(gsu_code_then(0x04BD47, 6, R[0]));
    R[1] = R[0];
    gsu_moves(R[0]);
    t = !gsu.z;
    if (t) { gsu_code(0x04BD4D, 4); gsu_code(0x04BD55, 5); goto lBD55; } else { gsu_code(0x04BD4D, 8); gsu_code(0x04BF42, 5); }
    goto lBF42;
lBD55:
    R[0] = 0x001C;
    R[0] = gsu_add(R[0], R[8], 0);
    R[0] = gsu_ldw(R[0]);
    R[2] = 0x0040;
    R[0] = gsu_and(R[0], R[2]);
    t = gsu.z;
    if (t) { gsu_code(0x04BD5A, 6); gsu_code(0x04BD64, 7); goto lBD64; }
    goto lBF3D;
lBD64:
    R[7] = 0x000C;
    R[7] = gsu_add(R[7], R[1], 0);
    R[7] = gsu_ldw(R[7]);
    gsu_code(0x04BD6B, 4);
    gsu_stw(FX_COLLIDE_MODEL, R[7]);
    R[2] = 0x001A;
    R[2] = gsu_add(R[2], R[7], 0);
    R[2] = gsu_ldw(gsu_code_then(0x04BD6F, 6, R[2]));
    R[0] = 0x0002;
    R[0] = gsu_and(R[0], R[2]);
    t = gsu.z;
    if (t) { gsu_code(0x04BD75, 6); gsu_code(0x04BD7F, 7); goto lBD7F; }
    goto lBF30;
lBD7F:
    R[2] = 0x0016;
    R[2] = gsu_add(R[2], R[7], 0);
    R[2] = gsu_ldw(R[2]);
    R[0] = 0x0020;
    R[0] = gsu_and(R[0], R[2]);
    t = gsu.z;
    if (t) goto lBD90; else { gsu_code(0x04BD86, 10); gsu_code(0x04BF30, 5); }
    goto lBF30;
lBD90:
    R[0] = 0x0002;
    R[0] = gsu_and(R[0], R[2]);
    t = gsu.z;
    if (t) goto lBDBD;
    R[3] = 0x0014;
    R[0] = gsu_ldw(FX_DRIVE_LEVEL);
    R[0] = gsu_add(R[0], R[0], 0);
    R[3] = gsu_sub(R[3], R[0], 0);
    t = !gsu.s;
    if (t) goto lBDA6;
    R[3] = 0x0000;
lBDA6:
    R[0] = 0x007C;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_stw(R[0], R[3]);
    R[0] = 0x005E;
    R[0] = gsu_add(R[0], R[8], 0);
    R[0] = gsu_ldw(R[0]);
    R[3] = 0x005E;
    R[3] = gsu_add(R[3], R[7], 0);
    R[3] = gsu_ldw(R[3]);
    R[0] = gsu_add(R[0], R[3], 0);
    t = gsu.z;
    if (t) goto lBDBD;
    goto lBF30;
lBDBD:
    gsu_stw(R[10], R[1]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_stw(R[10], R[8]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[11] = 0xBDCA;
    abi_FxSumsClear();
lBDCA:
    R[0] = 0x0002;
    R[0] = gsu_and(R[0], R[2]);
    t = gsu.z;
    if (t) goto lBDF0;
    R[0] = 0x0064;
    R[0] = gsu_add(R[0], R[8], 0);
    R[0] = gsu_ldw(R[0]);
    R[3] = 0x0010;
    R[0] = gsu_and(R[0], R[3]);
    t = gsu.z;
    if (t) goto lBDE7;
    R[0] = 0x0064;
    R[0] = gsu_add(R[0], R[7], 0);
    R[0] = gsu_ldw(R[0]);
    R[3] = 0x0003;
    R[0] = gsu_and(R[0], R[3]);
    t = gsu.z;
    if (t) goto lBDF0;
lBDE7:
    R[0] = 0x002C;
    R[0] = gsu_add(R[0], R[8], 0);
    R[0] = gsu_ldw(R[0]);
    R[3] = 0x0004;
    R[0] = gsu_or(R[0], R[3]);
    gsu_sbk(R[0]);
lBDF0:
    R[0] = 0x0016;
    R[0] = gsu_add(R[0], R[8], 0);
    R[0] = gsu_ldw(R[0]);
    R[2] = 0x0010;
    R[0] = gsu_and(R[0], R[2]);
    t = gsu.z;
    if (t) goto lBDFF;
    goto lBED1;
lBDFF:
    gsu_stw(R[10], R[1]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_stw(R[10], R[7]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_stw(R[10], R[9]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[0] = R[9];
    R[9] = R[7];
    R[7] = R[0];
    R[11] = 0xBE16;
    goto l9C3D;
lBE16:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    R[9] = gsu_ldw(R[10]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    R[7] = gsu_ldw(R[10]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    R[1] = gsu_ldw(R[10]);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = !gsu.z;
    if (t) goto lBE2B;
    goto lBF28;
lBE2B:
    R[11] = 0xBE30;
    goto l957C;
lBE30:
    R[0] = R[0];
    gsu_moves(R[0]);
    t = !gsu.z;
    if (t) goto lBE39;
    goto lBF28;
lBE39:
    R[11] = 0xBE3E;
    abi_FxPushesApply();
lBE3E:
    R[0] = gsu_ldw(FX_WORLD_SUM);
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
    R[0] = gsu_add(R[0], R[0], 0);
    gsu_sbk(R[0]);
    R[0] = gsu_ldw((FX_WORLD_SUM + 2));
    R[0] = 0x0000;
    gsu_sbk(R[0]);
    R[0] = gsu_ldw((FX_WORLD_SUM + 4));
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
    R[0] = gsu_add(R[0], R[0], 0);
    gsu_sbk(R[0]);
    R[1] = 0x0000;
    R[2] = 0x0000;
    R[3] = 0x0000;
    R[11] = 0xBE60;
    abi_FxHitAddUnit();
lBE60:
    R[1] = gsu_ldw(GSU_IN_20);
    R[1] = gsu_div2(R[1]);
    R[1] = gsu_div2(R[1]);
    R[2] = 0x0000;
    R[3] = gsu_ldw(GSU_IN_24);
    R[3] = gsu_div2(R[3]);
    R[3] = gsu_div2(R[3]);
    R[6] = gsu_ldw(GSU_OUT_E4);
    R[0] = gsu_fmult(R[1], R[6]);
    R[5] = gsu_rol(R[0]);
    R[6] = gsu_ldw(FX_OBJECT_MATRIX_1);
    R[0] = gsu_fmult(R[2], R[6]);
    R[0] = gsu_rol(R[0]);
    R[5] = gsu_add(R[5], R[0], 0);
    R[6] = gsu_ldw(FX_OBJECT_MATRIX_2);
    R[0] = gsu_fmult(R[3], R[6]);
    R[0] = gsu_rol(R[0]);
    R[0] = gsu_add(R[0], R[5], 0);
    gsu_stw(GSU_IN_20, R[0]);
    R[6] = gsu_ldw(FX_OBJECT_MATRIX_3);
    R[0] = gsu_fmult(R[1], R[6]);
    R[5] = gsu_rol(R[0]);
    R[6] = gsu_ldw(FX_OBJECT_MATRIX_4);
    R[0] = gsu_fmult(R[2], R[6]);
    R[0] = gsu_rol(R[0]);
    R[5] = gsu_add(R[5], R[0], 0);
    R[6] = gsu_ldw(FX_OBJECT_MATRIX_5);
    R[0] = gsu_fmult(R[3], R[6]);
    R[0] = gsu_rol(R[0]);
    R[0] = gsu_add(R[0], R[5], 0);
    gsu_stw(GSU_IN_22, R[0]);
    R[6] = gsu_ldw(FX_OBJECT_MATRIX_6);
    R[0] = gsu_fmult(R[1], R[6]);
    R[5] = gsu_rol(R[0]);
    R[6] = gsu_ldw(FX_OBJECT_MATRIX_7);
    R[0] = gsu_fmult(R[2], R[6]);
    R[0] = gsu_rol(R[0]);
    R[5] = gsu_add(R[5], R[0], 0);
    R[6] = gsu_ldw(FX_OBJECT_MATRIX_8);
    R[0] = gsu_fmult(R[3], R[6]);
    R[0] = gsu_rol(R[0]);
    R[0] = gsu_add(R[0], R[5], 0);
    gsu_stw(GSU_IN_24, R[0]);
    R[1] = gsu_ldw(GSU_IN_20);
    R[2] = gsu_ldw(GSU_IN_22);
    R[3] = gsu_ldw(GSU_IN_24);
    R[11] = 0xBECD;
    abi_FxPushAt4();
lBECD:
    goto lBF28;
lBED1:
    R[11] = 0xBED6;
    abi_FxCollidePush();
lBED6:
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_stw(FX_PUSH_TURN, R[0]);
    R[3] = 0x23A4;
    R[2] = gsu_ldw(FX_COLLIDE_MODEL);
    R[11] = 0xBEE7;
    abi_FxPushApply();
lBEE7:
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.z;
    if (t) goto lBEFC;
    R[1] = gsu_ldw(FX_CLOSING);
    R[1] = R[1];
    gsu_moves(R[1]);
    t = gsu.s;
    if (t) goto lBEFC;
    R[0] = 0x0094;
    R[0] = gsu_add(R[0], R[8], 0);
    R[0] = gsu_ldw(R[0]);
    R[0] = gsu_add(R[0], R[1], 0);
    gsu_sbk(R[0]);
lBEFC:
    R[0] = gsu_ldw(FX_WORLD_SUM);
    R[0] = gsu_add(R[0], R[0], 0);
    gsu_sbk(R[0]);
    R[1] = R[0];
    R[0] = gsu_ldw((FX_WORLD_SUM + 4));
    R[0] = gsu_add(R[0], R[0], 0);
    gsu_sbk(R[0]);
    R[1] = gsu_ldw(FX_WORLD_SUM);
    R[0] = gsu_ldw(FX_HIT);
    R[0] = gsu_add(R[0], R[1], 0);
    gsu_sbk(R[0]);
    R[1] = gsu_ldw((FX_WORLD_SUM + 4));
    R[0] = gsu_ldw((FX_HIT + 4));
    R[0] = gsu_add(R[0], R[1], 0);
    gsu_sbk(R[0]);
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_stw((FX_WORLD_SUM + 2), R[0]);
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_stw((FX_WORLD_SUM + 2), R[0]);
lBF28:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    R[8] = gsu_ldw(R[10]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    R[1] = gsu_ldw(R[10]);
lBF30:
    R[0] = 0x000E;
    R[0] = gsu_add(R[0], R[1], 0);
    R[0] = gsu_ldw(R[0]);
    R[1] = R[0];
    gsu_moves(R[0]);
    t = gsu.z;
    if (t) { gsu_code(0x04BF35, 4); gsu_code(0x04BF3D, 5); goto lBF3D; }
    goto lBD64;
lBF3D:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
lBF42:
    R[0] = 0x001C;
    R[0] = gsu_add(R[0], R[8], 0);
    R[0] = gsu_ldw(R[0]);
    R[1] = 0x0040;
    R[0] = gsu_bic(R[0], R[1]);
    gsu_code(0x04BF47, 6);
    gsu_sbk(R[0]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x04BF4D, 4);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
    /* FxCarFollowCourse */
lC3D5:
    gsu_stw(R[10], R[11]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[1] = 0x0016;
    R[1] = gsu_add(R[1], R[8], 0);
    R[1] = gsu_ldw(gsu_code_then(0x04C3D8, 8, R[1]));
    R[0] = 0x0400;
    R[0] = gsu_and(R[0], R[1]);
    t = !gsu.z;
    if (t) goto lC3EA; else { gsu_code(0x04C3E0, 10); gsu_code(0x04C43D, 5); }
    goto lC43D;
lC3EA:
    R[0] = 0x0018;
    R[0] = gsu_add(R[0], R[8], 0);
    R[0] = gsu_ldw(R[0]);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.z;
    if (t) goto lC3FD;
    R[0] = 0x001C;
    R[0] = gsu_add(R[0], R[8], 0);
    R[0] = gsu_ldw(R[0]);
    R[1] = 0x0040;
    R[0] = gsu_or(R[0], R[1]);
    gsu_sbk(R[0]);
lC3FD:
    R[0] = 0x001C;
    R[0] = gsu_add(R[0], R[8], 0);
    R[0] = gsu_ldw(R[0]);
    R[1] = 0x0010;
    R[2] = gsu_and(R[0], R[1]);
    t = !gsu.z;
    if (t) goto lC423;
    R[0] = gsu_or(R[0], R[1]);
    gsu_sbk(R[0]);
    R[1] = 0xFFFF;
    R[0] = 0x0072;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_stw(R[0], R[1]);
    R[1] = 0x0000;
    R[0] = 0x002E;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_stw(R[0], R[1]);
    R[0] = 0x0030;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_stw(R[0], R[1]);
    R[0] = 0x0032;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_stw(R[0], R[1]);
lC423:
    R[2] = 0x0070;
    R[2] = gsu_add(R[2], R[8], 0);
    R[2] = gsu_ldw(R[2]);
    R[0] = 0x0004;
    R[0] = gsu_and(R[0], R[2]);
    t = gsu.z;
    if (t) goto lC434;
    goto lC4BE;
lC434:
    R[11] = 0xC439;
    abi_FxCarCatchUp();
lC439:
    goto lC4B7;
lC43D:
    R[0] = 0x001C;
    R[0] = gsu_add(R[0], R[8], 0);
    R[0] = gsu_ldw(R[0]);
    R[1] = 0x0010;
    R[2] = gsu_and(R[0], R[1]);
    t = gsu.z;
    if (t) { gsu_code(0x04C442, 7); gsu_code(0x04C456, 7); goto lC456; }
    R[0] = gsu_bic(R[0], R[1]);
    gsu_sbk(R[0]);
    R[11] = 0xC451;
    abi_FxCarPose();
lC451:
    R[11] = 0xC456;
    goto lC72F;
lC456:
    R[2] = 0x0070;
    R[2] = gsu_add(R[2], R[8], 0);
    R[2] = gsu_ldw(R[2]);
    R[0] = 0x0004;
    R[0] = gsu_and(R[0], R[2]);
    t = gsu.z;
    if (t) { gsu_code(0x04C45D, 6); gsu_code(0x04C467, 7); goto lC467; }
    goto lC4BE;
lC467:
    R[2] = 0x002C;
    R[2] = gsu_add(R[2], R[8], 0);
    R[2] = gsu_ldw(R[2]);
    R[0] = 0x0040;
    R[0] = gsu_and(R[0], R[2]);
    t = gsu.z;
    if (t) { gsu_code(0x04C46E, 6); gsu_code(0x04C478, 7); goto lC478; }
    goto lC4AB;
lC478:
    R[1] = 0x001E;
    R[1] = gsu_add(R[1], R[8], 0);
    R[1] = gsu_ldw(R[1]);
    R[2] = 0x0020;
    R[2] = gsu_add(R[2], R[8], 0);
    R[2] = gsu_ldw(gsu_code_then(0x04C47F, 6, R[2]));
    R[3] = 0x0022;
    R[3] = gsu_add(R[3], R[8], 0);
    R[3] = gsu_ldw(gsu_code_then(0x04C485, 6, R[3]));
    R[11] = 0xC48F;
    (gsu_code(0x04C48B, 4), gsu_code(0x04CC06, 5), abi_FxCourseNearest());
lC48F:
    R[0] = R[0];
    gsu_moves(R[0]);
    t = !gsu.z;
    if (t) goto lC498;
    goto lC4AB;
lC498:
    R[7] = R[0];
    R[0] = 0x0058;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_stw(R[0], R[7]);
    R[0] = R[7];
    R[11] = 0xC4A6;
    (gsu_code(0x04C4A0, 6), gsu_code(0x04CCA4, 7), abi_FxCourseNextEntry());
lC4A6:
    R[11] = 0xC4AB;
    abi_FxDriveDecide();
lC4AB:
    R[11] = 0xC4B0;
    gsu_code(0x04C4AB, 5); gsu_code(0x04C933, 3);
    abi_FxTrafficSides();
lC4B0:
    R[0] = 0x0001;
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x04C4B0, 7);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
lC4B7:
    R[0] = 0x0000;
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
lC4BE:
    R[11] = 0xC4C3;
    abi_FxRecordSegment();
lC4C3:
    R[7] = R[0];
    R[0] = 0x0058;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_stw(R[0], R[7]);
    R[0] = 0x0001;
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
    /* FxCarAimAlongCourse */
lC72F:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    R[11] = gsu_ldw(R[10]);
    R[0] = 0x0058;
    R[0] = gsu_add(R[0], R[8], 0);
    R[0] = gsu_ldw(R[0]);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = !gsu.z;
    if (t) goto lC740;
    goto lC77C;
lC740:
    R[11] = 0xC745;
    abi_FxTableEntryWords();
lC745:
    R[11] = 0xC74A;
    abi_FxCourseEntryRead();
lC74A:
    R[0] = 0x0004;
    gsu_romb(R[0]);
    R[0] = gsu_ldw(FX_DRIVE_LEVEL);
    R[0] = gsu_lob(R[0]);
    R[0] = gsu_add(R[0], R[0], 0);
    gsu_r14(0xC781);
    gsu_r14(gsu_add(R[14], R[0], 0));
    R[6] = gsu_getb();
    gsu_r14(gsu_inc(R[14]));
    R[6] = gsu_getbh(R[6]);
    R[0] = gsu_add(R[4], R[4], 0);
    R[0] = gsu_fmult(R[0], R[6]);
    R[4] = R[0];
    R[0] = 0x0024;
    R[0] = gsu_add(R[0], R[8], 0);
    R[0] = gsu_ldw(R[0]);
    R[11] = 0xC76D;
    abi_FxAngleVector();
lC76D:
    R[0] = 0x002E;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_stw(R[0], R[1]);
    R[0] = 0x0030;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_stw(R[0], R[2]);
    R[0] = 0x0032;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_stw(R[0], R[3]);
lC77C:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
dispatch:
    if (at == ret || (ret != 0xFFFF && (int16_t)(base - R[10]) >= 4))
        return;
    switch (at) {
    case 0x8FCA: goto l8FCA;
    case 0x8FDF: goto l8FDF;
    case 0x8FE9: goto l8FE9;
    case 0x9004: goto l9004;
    case 0x9008: goto l9008;
    case 0x9030: goto l9030;
    case 0x9034: goto l9034;
    case 0x903D: goto l903D;
    case 0x9058: goto l9058;
    case 0x905E: goto l905E;
    case 0x9069: goto l9069;
    case 0x906E: goto l906E;
    case 0x9073: goto l9073;
    case 0x9078: goto l9078;
    case 0x907A: goto l907A;
    case 0x909B: goto l909B;
    case 0x90A0: goto l90A0;
    case 0x90B9: goto l90B9;
    case 0x90DF: goto l90DF;
    case 0x90E8: goto l90E8;
    case 0x90F8: goto l90F8;
    case 0x9107: goto l9107;
    case 0x910A: goto l910A;
    case 0x912D: goto l912D;
    case 0x9134: goto l9134;
    case 0x9139: goto l9139;
    case 0x9164: goto l9164;
    case 0x916D: goto l916D;
    case 0x917C: goto l917C;
    case 0x9185: goto l9185;
    case 0x91B2: goto l91B2;
    case 0x91CB: goto l91CB;
    case 0x91EF: goto l91EF;
    case 0x91FF: goto l91FF;
    case 0x9211: goto l9211;
    case 0x9228: goto l9228;
    case 0x9229: goto l9229;
    case 0x9256: goto l9256;
    case 0x9260: goto l9260;
    case 0x926A: goto l926A;
    case 0x926F: goto l926F;
    case 0x9277: goto l9277;
    case 0x927B: goto l927B;
    case 0x9288: goto l9288;
    case 0x92A7: goto l92A7;
    case 0x92B4: goto l92B4;
    case 0x92B8: goto l92B8;
    case 0x92C5: goto l92C5;
    case 0x92D0: goto l92D0;
    case 0x92D8: goto l92D8;
    case 0x92F7: goto l92F7;
    case 0x92FC: goto l92FC;
    case 0x957C: goto l957C;
    case 0x9582: goto l9582;
    case 0x95A6: goto l95A6;
    case 0x95AD: goto l95AD;
    case 0x95B4: goto l95B4;
    case 0x95B9: goto l95B9;
    case 0x9C3D: goto l9C3D;
    case 0x9D01: goto l9D01;
    case 0x9D15: goto l9D15;
    case 0x9D56: goto l9D56;
    case 0x9D6F: goto l9D6F;
    case 0x9D82: goto l9D82;
    case 0x9D93: goto l9D93;
    case 0x9DC3: goto l9DC3;
    case 0x9DEF: goto l9DEF;
    case 0x9E16: goto l9E16;
    case 0x9E3D: goto l9E3D;
    case 0x9E4A: goto l9E4A;
    case 0x9E6A: goto l9E6A;
    case 0x9E74: goto l9E74;
    case 0x9E7D: goto l9E7D;
    case 0x9E85: goto l9E85;
    case 0x9E87: goto l9E87;
    case 0x9E91: goto l9E91;
    case 0x9E92: goto l9E92;
    case 0x9EA8: goto l9EA8;
    case 0x9EC6: goto l9EC6;
    case 0x9ED4: goto l9ED4;
    case 0x9EE3: goto l9EE3;
    case 0x9F87: goto l9F87;
    case 0x9F9E: goto l9F9E;
    case 0xA013: goto lA013;
    case 0xA028: goto lA028;
    case 0xA053: goto lA053;
    case 0xA057: goto lA057;
    case 0xA09C: goto lA09C;
    case 0xA0B8: goto lA0B8;
    case 0xA0C4: goto lA0C4;
    case 0xA0CC: goto lA0CC;
    case 0xA0D6: goto lA0D6;
    case 0xA0EF: goto lA0EF;
    case 0xA0F8: goto lA0F8;
    case 0xA100: goto lA100;
    case 0xA10E: goto lA10E;
    case 0xA115: goto lA115;
    case 0xA117: goto lA117;
    case 0xA124: goto lA124;
    case 0xA127: goto lA127;
    case 0xA145: goto lA145;
    case 0xA607: goto lA607;
    case 0xA620: goto lA620;
    case 0xA635: goto lA635;
    case 0xA638: goto lA638;
    case 0xA63D: goto lA63D;
    case 0xA651: goto lA651;
    case 0xA656: goto lA656;
    case 0xA665: goto lA665;
    case 0xA681: goto lA681;
    case 0xA69A: goto lA69A;
    case 0xA6B0: goto lA6B0;
    case 0xA6E2: goto lA6E2;
    case 0xA6E9: goto lA6E9;
    case 0xA6F0: goto lA6F0;
    case 0xA6F7: goto lA6F7;
    case 0xA6FE: goto lA6FE;
    case 0xA705: goto lA705;
    case 0xA73D: goto lA73D;
    case 0xA749: goto lA749;
    case 0xA759: goto lA759;
    case 0xA76A: goto lA76A;
    case 0xA787: goto lA787;
    case 0xA7AF: goto lA7AF;
    case 0xA7B4: goto lA7B4;
    case 0xA7EA: goto lA7EA;
    case 0xA7F8: goto lA7F8;
    case 0xA80E: goto lA80E;
    case 0xA849: goto lA849;
    case 0xA866: goto lA866;
    case 0xA88E: goto lA88E;
    case 0xA893: goto lA893;
    case 0xA8CC: goto lA8CC;
    case 0xA8FB: goto lA8FB;
    case 0xA90A: goto lA90A;
    case 0xA90F: goto lA90F;
    case 0xA9DD: goto lA9DD;
    case 0xA9EC: goto lA9EC;
    case 0xA9F6: goto lA9F6;
    case 0xAA02: goto lAA02;
    case 0xAA06: goto lAA06;
    case 0xAA0D: goto lAA0D;
    case 0xAA52: goto lAA52;
    case 0xAA5A: goto lAA5A;
    case 0xAA82: goto lAA82;
    case 0xAA8E: goto lAA8E;
    case 0xAA98: goto lAA98;
    case 0xAACA: goto lAACA;
    case 0xAAD0: goto lAAD0;
    case 0xAB01: goto lAB01;
    case 0xAB06: goto lAB06;
    case 0xAB15: goto lAB15;
    case 0xAB4B: goto lAB4B;
    case 0xAB5F: goto lAB5F;
    case 0xABC8: goto lABC8;
    case 0xABE5: goto lABE5;
    case 0xAC18: goto lAC18;
    case 0xAC1F: goto lAC1F;
    case 0xAC34: goto lAC34;
    case 0xACC6: goto lACC6;
    case 0xACE4: goto lACE4;
    case 0xACE9: goto lACE9;
    case 0xACFC: goto lACFC;
    case 0xAD1F: goto lAD1F;
    case 0xAD2F: goto lAD2F;
    case 0xAD3D: goto lAD3D;
    case 0xAD4C: goto lAD4C;
    case 0xAD5A: goto lAD5A;
    case 0xAD68: goto lAD68;
    case 0xAD80: goto lAD80;
    case 0xAD8B: goto lAD8B;
    case 0xADB3: goto lADB3;
    case 0xADBB: goto lADBB;
    case 0xADC4: goto lADC4;
    case 0xADC9: goto lADC9;
    case 0xADDE: goto lADDE;
    case 0xADFA: goto lADFA;
    case 0xAE05: goto lAE05;
    case 0xAE39: goto lAE39;
    case 0xAE5B: goto lAE5B;
    case 0xAE62: goto lAE62;
    case 0xAE82: goto lAE82;
    case 0xAE89: goto lAE89;
    case 0xAEAB: goto lAEAB;
    case 0xAEB2: goto lAEB2;
    case 0xAEB9: goto lAEB9;
    case 0xAED7: goto lAED7;
    case 0xAEE5: goto lAEE5;
    case 0xAEFF: goto lAEFF;
    case 0xAF1A: goto lAF1A;
    case 0xAF26: goto lAF26;
    case 0xAF31: goto lAF31;
    case 0xAF3C: goto lAF3C;
    case 0xAF4E: goto lAF4E;
    case 0xAF54: goto lAF54;
    case 0xAF60: goto lAF60;
    case 0xAF6F: goto lAF6F;
    case 0xAF7F: goto lAF7F;
    case 0xAF8B: goto lAF8B;
    case 0xAF90: goto lAF90;
    case 0xAF93: goto lAF93;
    case 0xAF9D: goto lAF9D;
    case 0xAFA9: goto lAFA9;
    case 0xAFCF: goto lAFCF;
    case 0xAFD2: goto lAFD2;
    case 0xAFDF: goto lAFDF;
    case 0xAFF3: goto lAFF3;
    case 0xB006: goto lB006;
    case 0xB017: goto lB017;
    case 0xB044: goto lB044;
    case 0xB04C: goto lB04C;
    case 0xB06C: goto lB06C;
    case 0xB0DB: goto lB0DB;
    case 0xB0E4: goto lB0E4;
    case 0xB0FA: goto lB0FA;
    case 0xB10E: goto lB10E;
    case 0xB12A: goto lB12A;
    case 0xB133: goto lB133;
    case 0xB148: goto lB148;
    case 0xB15F: goto lB15F;
    case 0xB185: goto lB185;
    case 0xB18A: goto lB18A;
    case 0xB191: goto lB191;
    case 0xB19A: goto lB19A;
    case 0xB1A1: goto lB1A1;
    case 0xB1B2: goto lB1B2;
    case 0xB1C4: goto lB1C4;
    case 0xB1D2: goto lB1D2;
    case 0xB1D7: goto lB1D7;
    case 0xB1FB: goto lB1FB;
    case 0xB216: goto lB216;
    case 0xB22D: goto lB22D;
    case 0xB232: goto lB232;
    case 0xB23B: goto lB23B;
    case 0xB242: goto lB242;
    case 0xB269: goto lB269;
    case 0xB271: goto lB271;
    case 0xB285: goto lB285;
    case 0xB28E: goto lB28E;
    case 0xB293: goto lB293;
    case 0xB29A: goto lB29A;
    case 0xB2C1: goto lB2C1;
    case 0xB2D6: goto lB2D6;
    case 0xB32E: goto lB32E;
    case 0xB337: goto lB337;
    case 0xB34C: goto lB34C;
    case 0xB360: goto lB360;
    case 0xB370: goto lB370;
    case 0xB37D: goto lB37D;
    case 0xB392: goto lB392;
    case 0xB3BA: goto lB3BA;
    case 0xB3C5: goto lB3C5;
    case 0xB3D5: goto lB3D5;
    case 0xB3ED: goto lB3ED;
    case 0xB401: goto lB401;
    case 0xB407: goto lB407;
    case 0xB415: goto lB415;
    case 0xB436: goto lB436;
    case 0xB43B: goto lB43B;
    case 0xB44A: goto lB44A;
    case 0xB45F: goto lB45F;
    case 0xB48D: goto lB48D;
    case 0xB49D: goto lB49D;
    case 0xB4A4: goto lB4A4;
    case 0xB4AB: goto lB4AB;
    case 0xB4AF: goto lB4AF;
    case 0xB4C0: goto lB4C0;
    case 0xB4EC: goto lB4EC;
    case 0xB4F1: goto lB4F1;
    case 0xB505: goto lB505;
    case 0xB506: goto lB506;
    case 0xB529: goto lB529;
    case 0xB54A: goto lB54A;
    case 0xB556: goto lB556;
    case 0xB562: goto lB562;
    case 0xB594: goto lB594;
    case 0xB599: goto lB599;
    case 0xB5AA: goto lB5AA;
    case 0xB5AE: goto lB5AE;
    case 0xB5C4: goto lB5C4;
    case 0xB5D4: goto lB5D4;
    case 0xB5D6: goto lB5D6;
    case 0xB5FB: goto lB5FB;
    case 0xB5FF: goto lB5FF;
    case 0xB616: goto lB616;
    case 0xB61B: goto lB61B;
    case 0xB620: goto lB620;
    case 0xB653: goto lB653;
    case 0xB658: goto lB658;
    case 0xB65D: goto lB65D;
    case 0xB662: goto lB662;
    case 0xB67C: goto lB67C;
    case 0xB68E: goto lB68E;
    case 0xB693: goto lB693;
    case 0xB6B7: goto lB6B7;
    case 0xB6C1: goto lB6C1;
    case 0xB6DA: goto lB6DA;
    case 0xB6E1: goto lB6E1;
    case 0xB6F5: goto lB6F5;
    case 0xB6FC: goto lB6FC;
    case 0xB710: goto lB710;
    case 0xB717: goto lB717;
    case 0xB72B: goto lB72B;
    case 0xB732: goto lB732;
    case 0xB750: goto lB750;
    case 0xB75F: goto lB75F;
    case 0xB787: goto lB787;
    case 0xB799: goto lB799;
    case 0xB7A4: goto lB7A4;
    case 0xB7C1: goto lB7C1;
    case 0xB7CA: goto lB7CA;
    case 0xB7D4: goto lB7D4;
    case 0xB7DF: goto lB7DF;
    case 0xB80F: goto lB80F;
    case 0xB81C: goto lB81C;
    case 0xB838: goto lB838;
    case 0xB849: goto lB849;
    case 0xB85A: goto lB85A;
    case 0xB870: goto lB870;
    case 0xB87A: goto lB87A;
    case 0xB891: goto lB891;
    case 0xB89B: goto lB89B;
    case 0xB8B2: goto lB8B2;
    case 0xB8BC: goto lB8BC;
    case 0xB8BD: goto lB8BD;
    case 0xB8D7: goto lB8D7;
    case 0xB8E1: goto lB8E1;
    case 0xB8F7: goto lB8F7;
    case 0xB901: goto lB901;
    case 0xB915: goto lB915;
    case 0xB91F: goto lB91F;
    case 0xB920: goto lB920;
    case 0xB97F: goto lB97F;
    case 0xB990: goto lB990;
    case 0xB995: goto lB995;
    case 0xB99F: goto lB99F;
    case 0xBD20: goto lBD20;
    case 0xBD46: goto lBD46;
    case 0xBD55: goto lBD55;
    case 0xBD64: goto lBD64;
    case 0xBD7F: goto lBD7F;
    case 0xBD90: goto lBD90;
    case 0xBDA6: goto lBDA6;
    case 0xBDBD: goto lBDBD;
    case 0xBDCA: goto lBDCA;
    case 0xBDE7: goto lBDE7;
    case 0xBDF0: goto lBDF0;
    case 0xBDFF: goto lBDFF;
    case 0xBE16: goto lBE16;
    case 0xBE2B: goto lBE2B;
    case 0xBE30: goto lBE30;
    case 0xBE39: goto lBE39;
    case 0xBE3E: goto lBE3E;
    case 0xBE60: goto lBE60;
    case 0xBECD: goto lBECD;
    case 0xBED1: goto lBED1;
    case 0xBED6: goto lBED6;
    case 0xBEE7: goto lBEE7;
    case 0xBEFC: goto lBEFC;
    case 0xBF28: goto lBF28;
    case 0xBF30: goto lBF30;
    case 0xBF3D: goto lBF3D;
    case 0xBF42: goto lBF42;
    case 0xC3D5: goto lC3D5;
    case 0xC3EA: goto lC3EA;
    case 0xC3FD: goto lC3FD;
    case 0xC423: goto lC423;
    case 0xC434: goto lC434;
    case 0xC439: goto lC439;
    case 0xC43D: goto lC43D;
    case 0xC451: goto lC451;
    case 0xC456: goto lC456;
    case 0xC467: goto lC467;
    case 0xC478: goto lC478;
    case 0xC48F: goto lC48F;
    case 0xC498: goto lC498;
    case 0xC4A6: goto lC4A6;
    case 0xC4AB: goto lC4AB;
    case 0xC4B0: goto lC4B0;
    case 0xC4B7: goto lC4B7;
    case 0xC4BE: goto lC4BE;
    case 0xC4C3: goto lC4C3;
    case 0xC72F: goto lC72F;
    case 0xC740: goto lC740;
    case 0xC745: goto lC745;
    case 0xC74A: goto lC74A;
    case 0xC76D: goto lC76D;
    case 0xC77C: goto lC77C;
    /* the FxPushAt variants the table at $04:9BC6 picks (JMP R12),
       each returning to the R11 it was entered with */
    case 0x99A1: nx = R[11]; abi_FxPushAt2(); at = nx; goto dispatch;
    case 0x99C9: nx = R[11]; abi_FxPushAt1(); at = nx; goto dispatch;
    case 0x99EE: nx = R[11]; abi_FxPushAtHalf(); at = nx; goto dispatch;
    case 0x9A19: nx = R[11]; abi_FxPushAtQuarter(); at = nx; goto dispatch;
    case 0x9A4A: nx = R[11]; abi_FxPushAt4Twist(); at = nx; goto dispatch;
    case 0x9A9B: nx = R[11]; abi_FxPushAt4(); at = nx; goto dispatch;
    }
    return;   /* not an address the code jumps to */
}

/*
 * @routine 04:8FCA FxPhysicsProgram
 * WHAT IT DOES: A program the S-CPU starts once a race frame: steps
 * the race timers ($2472 down to 0, $2470 up), notes every touch
 * between records (FxAllTouches, unless $2472 is still running), then
 * runs each record of the list at $2332: one with a handler (+$0A)
 * has it run (twice when +$1C bit 3 is set, $263C 0 then 1); one
 * without is freed when its +$1C bit 2 is clear (FxObjectFree with
 * its +$04), else that bit is cleared. Then the cars are ranked
 * (FxRankCars) and the nearest found (FxNearestCar), and STOP.
 * Nothing runs when $246C is 0.
 * HOW: R10 = $0240; ROMB and RAMB 0; $246E = $FFFF while it runs; the
 * list walked by +$04 (R8 = the record); a handler is reached by LINK
 * #2; JMP R12, so it returns to 04:9030 or 04:903D.
 * Entry: started by the S-CPU.
 * Exit: (at the STOP) $246E = 0; the rest as the last record's
 * handler and FxNearestCar leave them.
 */
void abi_FxPhysicsProgram(void)
{
    physics_run(0x8FCA, 0xFFFF);
}

/*
 * @routine 04:907A FxObjectBounce
 * WHAT IT DOES: A record handler: an object that falls and bounces:
 * its y speed +$30 gains $78, it moves (FxRecordMove, R1 = 3) and
 * collides (FxRecordCollide); hit from below ($239C positive) its y
 * speed loses the hit, and on any hit its x and z speeds (+$2E, +$32)
 * lose an eighth. Not reached in the traces so far.
 * HOW: Pushes R11 and R8 on the R10 stack; R9 = R8; returns by
 * pulling R15.
 * Entry: R8 = the record; R10 = stack; R11 = return address.
 * Exit: R8 back; the rest as FxRecordCollide and the damping leave
 * them.
 */
void abi_FxObjectBounce(void)
{
    physics_run(0x907A, R[11]);
}

/*
 * @routine 04:90E8 FxObjectMarkTouched
 * WHAT IT DOES: A record handler: marks every record in this one's
 * touch list (+$18, linked by +$0E) touched (bit 1 of +$10).
 * HOW: A loop of LDW, OR, SBK; returns by JMP R11 (MOVE R15, R11).
 * Entry: R8 = the record; R11 = return address.
 * Exit: R0 = 0 at the list's end; R1 = 0; R2 = 2.
 */
void abi_FxObjectMarkTouched(void)
{
    physics_run(0x90E8, R[11]);
}

/*
 * @routine 04:910A FxObjectFall
 * WHAT IT DOES: A record handler: an object that falls while it
 * touches something: with a touch list (+$18) its y speed +$30 gains
 * $78; then it moves (FxRecordMove, R1 = 3) and collides
 * (FxRecordCollide); on a hit its x and z speeds lose a quarter.
 * HOW: Pushes R11 and R8 on the R10 stack; R9 = R8; returns by
 * pulling R15.
 * Entry: R8 = the record; R10 = stack; R11 = return address.
 * Exit: R8 back; the rest as FxRecordCollide and the damping leave
 * them.
 */
void abi_FxObjectFall(void)
{
    physics_run(0x910A, R[11]);
}

/*
 * @routine 04:916D FxObjectCollideOnly
 * WHAT IT DOES: A record handler: only collides the record with what
 * it touches (FxRecordCollide), without moving it. Not reached in the
 * traces so far.
 * HOW: Pushes R11 and R8 on the R10 stack; R9 = R8; returns by
 * pulling R15.
 * Entry: R8 = the record; R10 = stack; R11 = return address.
 * Exit: R8 back; the rest as FxRecordCollide leaves them.
 */
void abi_FxObjectCollideOnly(void)
{
    physics_run(0x916D, R[11]);
}

/*
 * @routine 04:9185 FxObjectDrop
 * WHAT IT DOES: A record handler: an object dropped straight down:
 * its y speed gains $3C, its x and z speeds are 0, it moves
 * (FxRecordMove, R1 = 3) and collides keeping x and z (+$1E, +$22)
 * from before; on a hit its handler (+$0A) is cleared, so it stops.
 * Not reached in the traces so far.
 * HOW: Pushes R11, R8 and the x and z on the R10 stack; returns by
 * pulling R15.
 * Entry: R8 = the record; R10 = stack; R11 = return address.
 * Exit: R8 back; the rest as FxRecordCollide leaves them.
 */
void abi_FxObjectDrop(void)
{
    physics_run(0x9185, R[11]);
}

/*
 * @routine 04:91FF FxRecordCollide
 * WHAT IT DOES: Collides record R9 with each record in its touch list
 * (+$18): with the other's model (FxCollideModel) unless the other's
 * flags (+$16) have bit 4, else as a box (FxCollidePush); the pushes
 * found are applied (FxPushesApply, FxPushApply), and $239C keeps the
 * first hit's depth. Clears bit 0 of R9's +$1C first and bit 1 of
 * each other's +$10.
 * HOW: Pushes R11 and each list entry on the R10 stack; $247E and
 * $239C cleared and the sums cleared (FxSumsClear) first; the next
 * entry by +$0E; returns by pulling R15.
 * Entry: R9 = the record; R10 = stack; R11 = return address.
 * Exit: $239C = the hit depth (0: none); the rest as the last
 * collision leaves them.
 */
void abi_FxRecordCollide(void)
{
    physics_run(0x91FF, R[11]);
}

/*
 * @routine 04:957C FxContactsAnyTouch
 * WHAT IT DOES: Whether any of the twenty contact slots at $23AE
 * holds a touch: R0 = 1 at the first slot in use whose depth word is
 * neither $7D00 nor negative, else 0. Not reached in the traces so
 * far.
 * HOW: CACHE; R13 counts the slots to $14, R3 walks them (8 past an
 * empty one, 6 past a counted one); returns by JMP R11 (MOVE R15,
 * R11).
 * Entry: R11 = return address.
 * Exit: R0 = 1 or 0; R3 at the slot reached; R4 its depth; R13 the
 * slot count.
 */
void abi_FxContactsAnyTouch(void)
{
    physics_run(0x957C, R[11]);
}

/*
 * @routine 04:9C3D FxCollideModel
 * WHAT IT DOES: Collides record R9 with the model of record R7 (its
 * shape's +$14 in ROM): R9's place relative to R7's is turned into
 * the model's axes (by quarter turns, or FxTurn for other headings),
 * then each face of the model's list is tested against it: a face the
 * place lies behind, by less than the face's best so far, is kept in
 * one of the twenty contact slots at $23AE (its flags, depth and
 * normal). R0 = 0 when the model has no collision list.
 * HOW: Pushes R11, R8 and R9; the slots' depth words set to $7D00
 * first; the face corners from the point table at $2488
 * (FxModelPoints when the model or its scale changed), the normals by
 * FxNormalize and LMULT cross products, the planes by FMULT; returns
 * by pulling R15.
 * Entry: R7 = the model's record, R9 = the record; R10 = stack; R11 =
 * return address.
 * Exit: R0 = 0 (no list) or as the face walk leaves it; R8, R9 back;
 * the slots filled.
 */
void abi_FxCollideModel(void)
{
    physics_run(0x9C3D, R[11]);
}

/*
 * @routine 04:A053 FxCollideModelSkipFace
 * WHAT IT DOES: FxCollideModel's skip of a face whose slot word has its
 * high byte set: on to the next face of the list (04:A0F8 at its end).
 * The game never runs it: the same slot word was tested just before
 * with nothing written between, so the BEQ at 04:A050 always branches
 * past it. A unit start runs it so the C is judged against the
 * original.
 * HOW: IWT R15 to 04:9E92 (NOP in the delay slot); then as
 * FxCollideModel's face walk.
 * Entry: R14 = the face list (ROM, the bank set); R10 = stack with the
 * return address, R8 and R9 pushed, as FxCollideModel leaves it.
 * Exit: as FxCollideModel's.
 */
void abi_FxCollideModelSkipFace(void)
{
    physics_run(0xA053, R[11]);
}

/*
 * @routine 04:A0B8 FxCollideModelDepthCompare
 * WHAT IT DOES: FxCollideModel's keep test for a face the place lies
 * behind: the absolute depth kept in the slot (R11 + 2) against the
 * absolute depth R0; a deeper one replaces the slot's flags, depth and
 * normal (R1-R3); then the next face. The game reaches it only with
 * both depths 0 or more (a slot's depth starts at $7D00 and only gets R0,
 * which the BPL before it keeps positive), so their negations at
 * 04:A0C1 and 04:A0C9 never run there; a unit start with both negative
 * runs them so the C is judged against the original.
 * HOW: LDW, MOVES and BPL with NOT, INC for each absolute value; CMP;
 * STW of R0, the flags at $237C and R1-R3 into the slot; IWT R15 to
 * 04:9E92.
 * Entry: R0 = the depth; R1-R3 = the normal; R11 = the slot; R14 = the
 * face list; R10 = stack with the return address, R8 and R9 pushed, as
 * FxCollideModel leaves it.
 * Exit: as FxCollideModel's.
 */
void abi_FxCollideModelDepthCompare(void)
{
    physics_run(0xA0B8, R[11]);
}

/*
 * @routine 04:A124 FxCollideModelUnused
 * WHAT IT DOES: Code no jump or branch reaches, after FxCollideModel's
 * return: a branch back to its slot scan (04:A0F8), which then ends as
 * FxCollideModel does (R0 = 1 when a slot holds a touch, else 0; R9, R8
 * and R15 pulled). The store after it is FxCollideModelUnusedStore
 * (04:A127). A unit start runs it so the C is judged against the
 * original.
 * HOW: BRA to 04:A0F8 (NOP in the delay slot).
 * Entry: R10 = stack with the return address, R8 and R9 pushed, as
 * FxCollideModel leaves it.
 * Exit: as FxCollideModel's.
 */
void abi_FxCollideModelUnused(void)
{
    physics_run(0xA124, R[11]);
}

/*
 * @routine 04:A127 FxCollideModelUnusedStore
 * WHAT IT DOES: Code no jump or branch reaches, after FxCollideModel's
 * return: stores record R9's y (+$20) minus the low seven bits of its x
 * plus z (+$1E + +$22) in the slot word at $23A4, then the slot scan
 * at 04:A0F8 as FxCollideModelUnused. A unit start runs it so the C is
 * judged against the original.
 * HOW: R11 = $23A4; ADD, AND #$7F, NOT, INC, ADD, STW (R11); BRA to
 * 04:A0F8.
 * Entry: R9 = the record; R10 = stack with the return address, R8 and
 * R9 pushed, as FxCollideModel leaves it.
 * Exit: as FxCollideModel's.
 */
void abi_FxCollideModelUnusedStore(void)
{
    physics_run(0xA127, R[11]);
}

/*
 * @routine 04:A607 FxPartsUpdate
 * WHAT IT DOES: Updates each part of car R8 (its list at +$3E, linked
 * by +$34): a part with bit 11 of its flags (+$46) is pulled toward
 * where it belongs (FxPartPull), any other is a wheel
 * (FxWheelUpdate); each part's contact flags (+$2C) are gathered into
 * the car's +$92. Clears the car's +$44, +$46 and +$56 first.
 * HOW: Pushes R11; R9 = the part; returns by pulling R15.
 * Entry: R8 = the car; R10 = stack; R11 = return address.
 * Exit: R0 = 0 at the list's end; R9 the last part; the rest as the
 * last part's update leaves them.
 */
void abi_FxPartsUpdate(void)
{
    physics_run(0xA607, R[11]);
}

/*
 * @routine 04:A656 FxPartPull
 * WHAT IT DOES: Pulls part R9 of car R8 toward its place on the car:
 * unless its flag bit 2 was clear (then it is only set), the part's
 * offset from the first wheel (by +$8E's list) is taken, each axis
 * under $C8 dropped, scaled by four and added to that wheel's speed,
 * the part's own speed subtracted where the signs differ; the result
 * is pushed on the car (FxSumsPushWorld, FxHitAddUnit) at the part's
 * place through the FxPushAt variant $2654 picks. With bit 0 of the
 * car's +$64 set it takes the car's speed ($2334-$2338) instead
 * (04:A7B4). Not reached in the traces so far.
 * HOW: Pushes R11; the push variant reached by LINK #2; JMP R12
 * through $04:9BC6; returns by pulling R15.
 * Entry: R8 = the car, R9 = the part; R10 = stack; R11 = return
 * address.
 * Exit: as the push leaves them.
 */
void abi_FxPartPull(void)
{
    physics_run(0xA656, R[11]);
}

/*
 * @routine 04:A893 FxWheelUpdate
 * WHAT IT DOES: Updates wheel R9 of car R8: copies the car's heading
 * and height, finds the ground under it (FxCollideModel with the
 * touched record, FxContactsResolve), takes the surface kind from the
 * model's flags into its contact flags (+$2C), and from the contact
 * depth and force works the suspension load, the grip along and
 * across the car (by the handling words $2652-$2682) and pushes the
 * car with them (FxSumsPushWorld, FxSumsPushLocal, FxSumsRotateLocal,
 * FxHitAddScaled). A wheel off the ground ($237E = $7D00) is handled
 * at 04:AEE5.
 * HOW: Pushes R11; $2484 = the wheel's flags while it runs; the
 * contact slots cleared and the sums cleared (FxSumsClear); returns
 * by pulling R15.
 * Entry: R8 = the car, R9 = the wheel; R10 = stack; R11 = return
 * address.
 * Exit: $2484 = the wheel's flags; the rest as the pushes leave them.
 */
void abi_FxWheelUpdate(void)
{
    physics_run(0xA893, R[11]);
}

/*
 * @routine 04:AA02 FxWheelUpdateNoSlot
 * WHAT IT DOES: FxWheelUpdate's way on when the contact flags from
 * FxContactsResolve (R1) are set but their slot bits (bits 0-4) are
 * clear: straight to the surface kind from the car's +$8A (04:AA98)
 * and the rest of the update. No play or unit start found so far makes
 * those flags (every slot word that is applied carries its slot
 * number), so a unit start runs it so the C is judged against the
 * original.
 * HOW: IWT R15 to 04:AA98 (NOP in the delay slot); then as
 * FxWheelUpdate from there.
 * Entry: R1 = the contact flags; R8 = the car, R9 = the wheel; R10 =
 * stack, the return address the word below it.
 * Exit: as FxWheelUpdate's.
 */
void abi_FxWheelUpdateNoSlot(void)
{
    physics_run(0xAA02, R[11]);
}

/*
 * @routine 04:AEFF FxContactsResolve
 * WHAT IT DOES: Resolves the contacts in the twenty slots at $23AE:
 * each slot in use goes through FxContactResolve and its upward force
 * ($23A0) is kept in $2386. R0 = $2480, the contact flags gathered.
 * HOW: Pushes R11; ROMB 0; $2480 = 0 and $239C = the wheel's +$2C
 * first; CACHE; R13 counts the slots to $14; returns by pulling R15.
 * Entry: R8 = the car; R10 = stack; R11 = return address.
 * Exit: R0 = $2480; the wheel's +$2C = $239C.
 */
void abi_FxContactsResolve(void)
{
    physics_run(0xAEFF, R[11]);
}

/*
 * @routine 04:AF54 FxContactResolve
 * WHAT IT DOES: Resolves one contact slot (R3): marks its flags in
 * $2480, notes the shallowest depth in $237E, and from the depth and
 * the face's normal works the force pushing the wheel out ($239E
 * across, $23A0 up). The code after it to 04:B9AB holds the car
 * handler (FxCarHandler, 04:B1A1) and its helpers. The bytes
 * 04:AF90-AF92 never run in the game (FxContactResolveNegate).
 * HOW: Pushes R11 for slots it resolves; R3 steps through the slot;
 * returns by pulling R15 (or JMP R11 for an empty or skipped slot,
 * 04:B185-B19A).
 * Entry: R3 = the slot; R8 = the car; R10 = stack; R11 = return
 * address.
 * Exit: R3 past the slot; the rest as the code leaves them.
 */
void abi_FxContactResolve(void)
{
    physics_run(0xAF54, R[11]);
}

/*
 * @routine 04:AF90 FxContactResolveNegate
 * WHAT IT DOES: FxContactResolve with the slot's depth (R7, copied to
 * R4) negative at its absolute value: R4 = -R4, then the rest of
 * FxContactResolve from its depth compare. The game never runs it: a
 * negative depth branches to 04:B191 before, so the BPL at 04:AF8D
 * always branches past it. A unit start runs it so the C is judged
 * against the original.
 * HOW: WITH R4; NOT; INC R4; then as FxContactResolve from 04:AF93.
 * Entry: R3 = the slot past its depth word; R4 = R7 = the depth; R5 =
 * the shallowest depth so far, made positive; R8 = the car; R10 =
 * stack, the return address the word below it.
 * Exit: as FxContactResolve's.
 */
void abi_FxContactResolveNegate(void)
{
    physics_run(0xAF90, R[11]);
}

/*
 * @routine 04:B1A1 FxCarHandler
 * WHAT IT DOES: The record handler of a car: steering from the input
 * (FxSteerFromInput), its pose (FxCarPose), its parts
 * (FxPartsUpdate), the move (FxRecordMove) and its collisions
 * (FxCarCollide, FxCarHit), holding it on the ground (FxRecordHoldY,
 * FxRecordAlignment), and its place on the course (FxCarFollowCourse,
 * FxLapSegment, FxCourseNextDistance).
 * HOW: Pushes R11 and R8; returns by pulling R15.
 * Entry: R8 = the car; R10 = stack; R11 = return address.
 * Exit: R8 back; the rest as the last call leaves them.
 */
void abi_FxCarHandler(void)
{
    physics_run(0xB1A1, R[11]);
}

/*
 * @routine 04:B6C1 FxCarHandlerClamp
 * WHAT IT DOES: FxCarHandler's clamp of its three sums ($2336, $233E,
 * $233C) to -$0BB8..$0BB8, entered on its own, then the rest of the
 * handler (its spin and flip steps, FxCarFollowCourse, the lap and
 * course distance). The game reaches it only by the handler's branches;
 * a unit start with each sum past its limit runs the clamps no play has
 * reached so far, so the C is judged against the original.
 * HOW: R1 = $F448 and R2 = $0BB8; each sum's sign picks which limit it
 * is compared with (CMP, BMI or BPL) and SBK puts the limit back; then
 * as FxCarHandler after them (04:B718).
 * Entry: R8 = the car; R10 = stack, the return address the word below
 * it.
 * Exit: as FxCarHandler's.
 */
void abi_FxCarHandlerClamp(void)
{
    physics_run(0xB6C1, R[11]);
}

/*
 * @routine 04:B799 FxCarHandlerFlip
 * WHAT IT DOES: FxCarHandler's flip step: when the car's angle +$26 is
 * $4000 or more either way, +$40 is ORed with 0, +$76 is set to $28
 * when it is 0, and the spin $233A is negated; then the rest of the
 * handler. The game never runs it: the test before it ANDs +$40 with 0
 * (IWT R0, #0), so the BNE at 04:B769 never branches here. A unit start
 * runs it so the C is judged against the original.
 * HOW: LDW, MOVES, BPL with NOT, INC for the absolute angle; CMP with
 * $4000; then as FxCarHandler from 04:B7D4.
 * Entry: R8 = the car; R10 = stack, the return address the word below
 * it.
 * Exit: as FxCarHandler's.
 */
void abi_FxCarHandlerFlip(void)
{
    physics_run(0xB799, R[11]);
}

/*
 * @routine 04:BD20 FxCarCollide
 * WHAT IT DOES: Collides car R8 with each record in its touch list
 * (+$18): another car's model (FxCollideModel with the roles
 * swapped), then if any slot holds a touch (FxContactsAnyTouch) the
 * pushes are applied (FxPushesApply), the world sum turned back and
 * doubled, and the car pushed at its place (FxHitAddUnit, FxPushAt4)
 * and the other record too (FxCollidePush, FxPushApply). Records
 * whose flags rule them out (+$1C bit 6, +$1A bit 1, +$16 bit 5) are
 * skipped.
 * HOW: Pushes R11 and each entry; $247E, $239C, $2378 and $26F6
 * cleared first; returns by pulling R15.
 * Entry: R8 = the car; R10 = stack; R11 = return address.
 * Exit: as the last collision leaves them.
 */
void abi_FxCarCollide(void)
{
    physics_run(0xBD20, R[11]);
}

/*
 * FxCarFollowCourse (04:C3D5), part of the program C, not tagged:
 * entered on its own it would end two levels up, past the R11 the
 * harness ends a call at.
 * WHAT IT DOES: Keeps car R8 on the course: off the course (+$16 bit
 * 10 clear) a car marked fallen (+$1C bit 4) is put back (FxCarPose,
 * FxCarAimAlongCourse); then, unless it is driven by the course data
 * (+$70 bit 2: FxRecordSegment), its nearest course entry is found
 * (FxCourseNearest, FxCourseNextEntry) and it decides its driving
 * (FxDriveDecide) and its sides (FxTrafficSides). R0 = 1, or 0 when
 * it is caught up (FxCarCatchUp).
 * HOW: Pushes R11; returns by pulling R15.
 * Entry: R8 = the car; R10 = stack; R11 = return address.
 * Exit: R0 = 1 or 0; the rest as the last call leaves them.
 */

/*
 * @routine 04:C72F FxCarVelocityFromSegment
 * WHAT IT DOES: Takes the saved R11 back off the R10 stack and, unless
 * car R8's word +$58 (its entry) is 0, sets the car's velocity
 * +$2E/+$30/+$32 to a vector along its heading +$24 whose length is
 * twice word 4 of the entry (FxTableEntryWords, FxCourseEntryRead)
 * times the factor for $26F4 from the table at $04:C781 (FMULT). It
 * aims a fallen car along the course.
 * HOW: Entered by LINK from 04:C451 (FxCarFollowCourse), but it never
 * returns to that R11: it pulls the R11 04:C3D5 saved and returns
 * through the word under it, two levels up, into its caller's caller.
 * Its C is the physics program's from here, to that return (the jump
 * with the R10 stack two words below its entry, where the checks end
 * such a call too). FxAngleVector makes the vector.
 * Entry: R8 = the car's record; R10 = stack (FxCarFollowCourse's saved
 * R11 on top of the return address).
 * Exit: R11 = $C76D (or the saved R11 for a 0 entry); R1-R3 the vector;
 * R10 two words below its entry; S, Z from R10; the last RAM address
 * R10.
 */
void abi_FxCarVelocityFromSegment(void)
{
    physics_run(0xC72F, R[11]);
}
