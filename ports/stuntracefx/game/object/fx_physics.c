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
/* The port scales only boost consumption when physics runs at 60 Hz. */
uint16_t interp_boost_cost(uint16_t rec, uint16_t cost);
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
    int t;
    uint16_t nx;
    uint16_t base = R[10];

    (void)t;
    (void)nx;
    goto enter;   /* the caller fetched the first byte */
    /* FxPhysicsProgram */
l8FCA:
    R[10] = 0x0240;
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_code(0x048fcb, 6);
    gsu_romb(R[0]);
    gsu_code(0x048fd1, 2);
    gsu_ramb(R[0]);
    gsu_code(0x048fd3, 4);
    R[0] = gsu_ldw((FX_PUSHES + 190));
    R[0] = R[0];
    gsu_moves(R[0]);
    t = !gsu.z;
    gsu_code(0x048fd7, 4);
    if (t) { gsu_code(0x048fdf, 1); goto l8FDF; }
    gsu_code(0x048fdb, 4); gsu_code(0x049078, 1);
    goto l9078;
l8FDF:
    gsu_code(0x048fe0, 4);
    R[0] = gsu_ldw(FX_INIT_2472);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.z;
    R[0] = gsu_dec(R[0]);
    gsu_code(0x048fe4, 4);
    if (t) { gsu_code(0x048fe9, 1); goto l8FE9; }
    gsu_code(0x048fe8, 2);
    gsu_sbk(R[0]);
l8FE9:
    gsu_code(0x048fea, 4);
    R[0] = gsu_ldw(FX_INIT_2470);
    R[0] = gsu_inc(R[0]);
    gsu_code(0x048fee, 2);
    gsu_sbk(R[0]);
    R[0] = 0xFFFF;
    gsu_code(0x048ff0, 7);
    gsu_stw((FX_PUSHES + 192), R[0]);
    gsu_code(0x048ff7, 4);
    R[0] = gsu_ldw(FX_INIT_2472);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = !gsu.z;
    gsu_code(0x048ffb, 4);
    if (t) { gsu_code(0x049004, 1); goto l9004; }
    R[11] = 0x9004;
    gsu_code(0x048fff, 5); gsu_code(0x04a1ec, 1);
    abi_FxAllTouches();
    gsu_code(0x049004, 1);
l9004:
    gsu_code(0x049005, 4);
    R[0] = gsu_ldw(FX_RECORDS);
l9008:
    R[8] = R[0];
    gsu_moves(R[0]);
    t = gsu.z;
    gsu_code(0x049009, 4);
    if (t) { gsu_code(0x049069, 1); goto l9069; }
    R[0] = 0x000A;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04900d, 5);
    R[0] = gsu_ldw(R[0]);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.z;
    gsu_code(0x049012, 4);
    if (t) { gsu_code(0x04903d, 1); goto l903D; }
    R[12] = R[0];
    R[0] = 0x001C;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x049016, 7);
    R[0] = gsu_ldw(R[0]);
    R[1] = 0x0008;
    R[0] = gsu_and(R[0], R[1]);
    t = gsu.z;
    gsu_code(0x04901d, 6);
    if (t) { gsu_code(0x049034, 1); goto l9034; }
    gsu_code(0x049023, 3);
    gsu_stw(R[10], R[12]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[0] = 0x0000;
    gsu_code(0x049026, 8);
    gsu_stw(FX_HIT_ON, R[0]);
    R[11] = 0x9030;
    nx = (uint16_t)(R[12]);
    at = nx;
    gsu_code(0x04902e, 2);
    goto dispatch;
l9030:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x049031, 4);
    R[12] = gsu_ldw(R[10]);
l9034:
    R[0] = 0x0001;
    gsu_code(0x049035, 6);
    gsu_stw(FX_HIT_ON, R[0]);
    R[11] = 0x903D;
    nx = (uint16_t)(R[12]);
    at = nx;
    gsu_code(0x04903b, 2);
    goto dispatch;
l903D:
    R[0] = 0x001C;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04903e, 4);
    R[0] = gsu_ldw(R[0]);
    R[2] = R[0];
    R[1] = 0x0004;
    R[0] = gsu_and(R[0], R[1]);
    t = !gsu.z;
    gsu_code(0x049042, 8);
    if (t) { gsu_code(0x04905e, 1); goto l905E; }
    R[0] = 0x0004;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04904a, 5);
    R[0] = gsu_ldw(R[0]);
    gsu_code(0x04904f, 1);
    gsu_stw(R[10], R[0]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[1] = R[8];
    R[11] = 0x9058;
    gsu_code(0x049050, 8); gsu_code(0x048a08, 1);
    abi_FxObjectFree();
    gsu_code(0x049058, 1);
l9058:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x049059, 3);
    R[0] = gsu_ldw(R[10]);
    t = 1;
    gsu_code(0x04905c, 2);
    gsu_code(0x049008, 1);
    if (t) goto l9008;
l905E:
    R[0] = gsu_bic(R[2], R[1]);
    gsu_code(0x04905f, 4);
    gsu_sbk(R[0]);
    R[0] = 0x0004;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x049063, 4);
    R[0] = gsu_ldw(R[0]);
    t = 1;
    gsu_code(0x049067, 2);
    gsu_code(0x049008, 1);
    if (t) goto l9008;
l9069:
    R[11] = 0x906E;
    gsu_code(0x04906a, 4); gsu_code(0x04ce1d, 1);
    abi_FxRankCars();
l906E:
    R[11] = 0x9073;
    gsu_code(0x04906f, 4); gsu_code(0x04cf08, 1);
    abi_FxNearestCar();
l9073:
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_code(0x049074, 5);
    gsu_stw((FX_PUSHES + 192), R[0]);
l9078:
    return;   /* the STOP: the program ends here */
    /* FxObjectBounce */
l907A:
    gsu_code(0x04907b, 2);
    gsu_stw(R[10], R[11]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_code(0x04907d, 4);
    gsu_stw(R[10], R[8]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[9] = R[8];
    R[1] = 0x0030;
    R[1] = gsu_add(R[1], R[9], 0);
    gsu_code(0x049081, 10);
    R[1] = gsu_ldw(R[1]);
    R[0] = 0x0078;
    R[1] = gsu_add(R[0], R[1], 0);
    R[0] = 0x0030;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_code(0x04908b, 10);
    gsu_stw(R[0], R[1]);
    R[1] = 0x0003;
    R[11] = 0x909B;
    gsu_code(0x049095, 6); gsu_code(0x0495be, 1);
    abi_FxRecordMove();
l909B:
    R[11] = 0x90A0;
    gsu_code(0x04909c, 4); gsu_code(0x0491ff, 1);
    goto l91FF;
l90A0:
    gsu_code(0x0490a1, 4);
    R[0] = gsu_ldw(FX_HIT_DEPTH);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.z;
    gsu_code(0x0490a5, 4);
    if (t) { gsu_code(0x0490df, 1); goto l90DF; }
    t = gsu.s;
    gsu_code(0x0490a9, 3);
    if (t) { gsu_code(0x0490b9, 1); goto l90B9; }
    R[1] = 0x0030;
    R[1] = gsu_add(R[1], R[9], 0);
    gsu_code(0x0490ac, 7);
    R[1] = gsu_ldw(R[1]);
    R[1] = gsu_sub(R[1], R[0], 0);
    R[0] = 0x0030;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_code(0x0490b3, 7);
    gsu_stw(R[0], R[1]);
l90B9:
    R[0] = 0x002E;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_code(0x0490ba, 4);
    R[0] = gsu_ldw(R[0]);
    R[1] = R[0];
    R[0] = gsu_div2(R[0]);
    R[0] = gsu_div2(R[0]);
    R[0] = gsu_div2(R[0]);
    R[1] = gsu_sub(R[1], R[0], 0);
    R[0] = 0x002E;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_code(0x0490be, 15);
    gsu_stw(R[0], R[1]);
    R[0] = 0x0032;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_code(0x0490cd, 4);
    R[0] = gsu_ldw(R[0]);
    R[1] = R[0];
    R[0] = gsu_div2(R[0]);
    R[0] = gsu_div2(R[0]);
    R[0] = gsu_div2(R[0]);
    R[1] = gsu_sub(R[1], R[0], 0);
    R[0] = 0x0032;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_code(0x0490d1, 15);
    gsu_stw(R[0], R[1]);
l90DF:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x0490e0, 4);
    R[8] = gsu_ldw(R[10]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x0490e4, 4);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
    /* FxObjectMarkTouched */
l90E8:
    R[8] = R[8];
    R[1] = 0x0018;
    R[1] = gsu_add(R[1], R[8], 0);
    gsu_code(0x0490e9, 8);
    R[1] = gsu_ldw(R[1]);
    R[1] = R[1];
    gsu_moves(R[1]);
    t = gsu.z;
    gsu_code(0x0490f1, 4);
    if (t) { gsu_code(0x049107, 1); goto l9107; }
    R[2] = 0x0002;
    gsu_code(0x0490f5, 4);
l90F8:
    R[0] = 0x0010;
    R[0] = gsu_add(R[0], R[1], 0);
    gsu_code(0x0490f9, 4);
    R[0] = gsu_ldw(R[0]);
    R[0] = gsu_or(R[0], R[2]);
    gsu_code(0x0490fd, 2);
    gsu_sbk(R[0]);
    R[0] = 0x000E;
    R[0] = gsu_add(R[0], R[1], 0);
    gsu_code(0x0490ff, 4);
    R[0] = gsu_ldw(R[0]);
    R[1] = R[0];
    gsu_moves(R[0]);
    t = !gsu.z;
    gsu_code(0x049103, 4);
    if (t) { gsu_code(0x0490f8, 1); goto l90F8; }
    gsu_code(0x049107, 1);
l9107:
    nx = (uint16_t)(R[11]);
    at = nx;
    gsu_code(0x049108, 2);
    goto dispatch;
    /* FxObjectFall */
l910A:
    gsu_code(0x04910b, 2);
    gsu_stw(R[10], R[11]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_code(0x04910d, 4);
    gsu_stw(R[10], R[8]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[9] = R[8];
    R[0] = 0x0018;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_code(0x049111, 8);
    R[0] = gsu_ldw(R[0]);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.z;
    gsu_code(0x049119, 4);
    if (t) { gsu_code(0x04912d, 1); goto l912D; }
    R[1] = 0x0030;
    R[1] = gsu_add(R[1], R[9], 0);
    gsu_code(0x04911d, 7);
    R[1] = gsu_ldw(R[1]);
    R[0] = 0x0078;
    R[1] = gsu_add(R[0], R[1], 0);
    R[0] = 0x0030;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_code(0x049124, 10);
    gsu_stw(R[0], R[1]);
l912D:
    R[1] = 0x0003;
    R[11] = 0x9134;
    gsu_code(0x04912e, 6); gsu_code(0x0495be, 1);
    abi_FxRecordMove();
l9134:
    R[11] = 0x9139;
    gsu_code(0x049135, 4); gsu_code(0x0491ff, 1);
    goto l91FF;
l9139:
    gsu_code(0x04913a, 4);
    R[0] = gsu_ldw(FX_HIT_DEPTH);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.z;
    gsu_code(0x04913e, 4);
    if (t) { gsu_code(0x049164, 1); goto l9164; }
    R[0] = 0x002E;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_code(0x049142, 5);
    R[0] = gsu_ldw(R[0]);
    R[1] = R[0];
    R[0] = gsu_div2(R[0]);
    R[0] = gsu_div2(R[0]);
    R[1] = gsu_sub(R[1], R[0], 0);
    R[0] = 0x002E;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_code(0x049147, 13);
    gsu_stw(R[0], R[1]);
    R[0] = 0x0032;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_code(0x049154, 4);
    R[0] = gsu_ldw(R[0]);
    R[1] = R[0];
    R[0] = gsu_div2(R[0]);
    R[0] = gsu_div2(R[0]);
    R[1] = gsu_sub(R[1], R[0], 0);
    R[0] = 0x0032;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_code(0x049158, 13);
    gsu_stw(R[0], R[1]);
l9164:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x049165, 4);
    R[8] = gsu_ldw(R[10]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x049169, 4);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
    /* FxObjectCollideOnly */
l916D:
    gsu_code(0x04916e, 2);
    gsu_stw(R[10], R[11]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_code(0x049170, 4);
    gsu_stw(R[10], R[8]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[9] = R[8];
    R[11] = 0x917C;
    gsu_code(0x049174, 8); gsu_code(0x0491ff, 1);
    goto l91FF;
l917C:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x04917d, 4);
    R[8] = gsu_ldw(R[10]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x049181, 4);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
    /* FxObjectDrop */
l9185:
    gsu_code(0x049186, 2);
    gsu_stw(R[10], R[11]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_code(0x049188, 4);
    gsu_stw(R[10], R[8]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[9] = R[8];
    R[1] = 0x0030;
    R[1] = gsu_add(R[1], R[9], 0);
    gsu_code(0x04918c, 10);
    R[1] = gsu_ldw(R[1]);
    R[0] = 0x003C;
    R[1] = gsu_add(R[0], R[1], 0);
    R[0] = 0x0030;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_code(0x049196, 10);
    gsu_stw(R[0], R[1]);
    R[1] = 0x0000;
    R[0] = 0x0032;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_code(0x0491a0, 7);
    gsu_stw(R[0], R[1]);
    R[0] = 0x0032;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_code(0x0491a7, 5);
    gsu_stw(R[0], R[1]);
    R[1] = 0x0003;
    R[11] = 0x91B2;
    gsu_code(0x0491ac, 6); gsu_code(0x0495be, 1);
    abi_FxRecordMove();
l91B2:
    R[1] = 0x001E;
    R[1] = gsu_add(R[1], R[9], 0);
    gsu_code(0x0491b3, 6);
    R[1] = gsu_ldw(R[1]);
    R[3] = 0x0022;
    R[3] = gsu_add(R[3], R[9], 0);
    gsu_code(0x0491b9, 6);
    R[3] = gsu_ldw(R[3]);
    gsu_code(0x0491bf, 2);
    gsu_stw(R[10], R[1]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_code(0x0491c1, 4);
    gsu_stw(R[10], R[3]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[11] = 0x91CB;
    gsu_code(0x0491c5, 6); gsu_code(0x0491ff, 1);
    goto l91FF;
l91CB:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x0491cc, 4);
    R[3] = gsu_ldw(R[10]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x0491d0, 4);
    R[1] = gsu_ldw(R[10]);
    gsu_code(0x0491d4, 4);
    R[0] = gsu_ldw(FX_HIT_DEPTH);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = !gsu.z;
    gsu_code(0x0491d8, 4);
    if (t) { gsu_code(0x0491ef, 1); goto l91EF; }
    R[0] = 0x001E;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_code(0x0491dc, 6);
    gsu_stw(R[0], R[1]);
    R[0] = 0x0022;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_code(0x0491e2, 5);
    gsu_stw(R[0], R[3]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x0491e7, 4);
    R[8] = gsu_ldw(R[10]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x0491eb, 4);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
l91EF:
    R[1] = 0x0000;
    R[0] = 0x000A;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_code(0x0491f0, 7);
    gsu_stw(R[0], R[1]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x0491f7, 4);
    R[8] = gsu_ldw(R[10]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x0491fb, 4);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
    /* FxRecordCollide */
l91FF:
    gsu_code(0x049200, 2);
    gsu_stw(R[10], R[11]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_code(0x049202, 7);
    gsu_stw(FX_PUSH_TURN, R[0]);
    gsu_code(0x049209, 4);
    gsu_stw(FX_HIT_DEPTH, R[0]);
    R[11] = 0x9211;
    gsu_code(0x04920d, 4); gsu_code(0x0496a7, 1);
    abi_FxSumsClear();
    gsu_code(0x049211, 1);
l9211:
    R[3] = 0x0001;
    R[0] = 0x001C;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_code(0x049212, 7);
    R[0] = gsu_ldw(R[0]);
    R[0] = gsu_bic(R[0], R[3]);
    gsu_code(0x049219, 3);
    gsu_sbk(R[0]);
    R[0] = 0x0018;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_code(0x04921c, 4);
    R[0] = gsu_ldw(R[0]);
    R[1] = R[0];
    gsu_moves(R[0]);
    t = !gsu.z;
    gsu_code(0x049220, 4);
    if (t) { gsu_code(0x049228, 1); goto l9228; }
    gsu_code(0x049224, 4); gsu_code(0x0492c5, 1);
    goto l92C5;
l9228:
    gsu_code(0x049229, 1);
l9229:
    gsu_code(0x04922a, 2);
    gsu_stw(R[10], R[1]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[7] = 0x000C;
    R[7] = gsu_add(R[7], R[1], 0);
    gsu_code(0x04922c, 8);
    R[7] = gsu_ldw(R[7]);
    gsu_code(0x049234, 4);
    gsu_stw(FX_COLLIDE_MODEL, R[7]);
    R[0] = 0x0010;
    R[0] = gsu_add(R[0], R[1], 0);
    gsu_code(0x049238, 4);
    R[0] = gsu_ldw(R[0]);
    R[3] = 0x0002;
    R[0] = gsu_bic(R[0], R[3]);
    gsu_code(0x04923c, 6);
    gsu_sbk(R[0]);
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_code(0x049242, 5);
    gsu_stw(FX_PUSH_AMOUNT, R[0]);
    R[0] = 0x0016;
    R[0] = gsu_add(R[0], R[7], 0);
    gsu_code(0x049247, 4);
    R[0] = gsu_ldw(R[0]);
    R[2] = 0x0010;
    R[0] = gsu_and(R[0], R[2]);
    t = !gsu.z;
    gsu_code(0x04924b, 6);
    if (t) { gsu_code(0x04926a, 1); goto l926A; }
    R[11] = 0x9256;
    gsu_code(0x049251, 5); gsu_code(0x049c3d, 1);
    goto l9C3D;
l9256:
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.z;
    gsu_code(0x049257, 4);
    if (t) { gsu_code(0x0492b4, 1); goto l92B4; }
    R[11] = 0x9260;
    gsu_code(0x04925b, 5); gsu_code(0x04941c, 1);
    abi_FxPushesApply();
    gsu_code(0x049260, 1);
l9260:
    gsu_code(0x049261, 4);
    R[2] = gsu_ldw(FX_PUSH_AMOUNT);
    R[2] = gsu_not(R[2]);
    R[2] = gsu_inc(R[2]);
    t = 1;
    gsu_code(0x049265, 5);
    gsu_code(0x04927b, 1);
    if (t) goto l927B;
l926A:
    R[11] = 0x926F;
    gsu_code(0x04926b, 4); gsu_code(0x049301, 1);
    abi_FxCollidePush();
    gsu_code(0x04926f, 1);
l926F:
    R[3] = 0x23A4;
    R[11] = 0x9277;
    gsu_code(0x049270, 7); gsu_code(0x04944c, 1);
    abi_FxPushApply();
    gsu_code(0x049277, 1);
l9277:
    gsu_code(0x049278, 4);
    R[2] = gsu_ldw(FX_PUSH_AMOUNT);
l927B:
    gsu_code(0x04927c, 4);
    R[0] = gsu_ldw(FX_HIT_DEPTH);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = !gsu.z;
    gsu_code(0x049280, 4);
    if (t) { gsu_code(0x049288, 1); goto l9288; }
    gsu_code(0x049284, 5);
    gsu_stw(FX_HIT_DEPTH, R[2]);
l9288:
    R[2] = R[2];
    gsu_moves(R[2]);
    t = gsu.z;
    gsu_code(0x049289, 4);
    if (t) { gsu_code(0x0492b4, 1); goto l92B4; }
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x04928d, 5);
    R[1] = gsu_ldw(R[10]);
    gsu_code(0x049292, 4);
    R[0] = gsu_ldw(FX_PUSH_FLAGS);
    R[3] = 0x0020;
    R[0] = gsu_and(R[0], R[3]);
    t = gsu.z;
    gsu_code(0x049296, 6);
    if (t) { gsu_code(0x0492a7, 1); goto l92A7; }
    R[3] = 0x002C;
    R[3] = gsu_add(R[3], R[9], 0);
    gsu_code(0x04929c, 7);
    R[3] = gsu_ldw(R[3]);
    R[0] = 0x0020;
    R[0] = gsu_or(R[0], R[3]);
    gsu_code(0x0492a3, 5);
    gsu_sbk(R[0]);
l92A7:
    R[3] = 0x0010;
    R[3] = gsu_add(R[3], R[1], 0);
    gsu_code(0x0492a8, 6);
    R[3] = gsu_ldw(R[3]);
    R[0] = 0x0002;
    R[0] = gsu_or(R[0], R[3]);
    t = 1;
    gsu_code(0x0492ae, 6); gsu_code(0x0492b8, 1);
    gsu_sbk(R[0]);
    if (t) goto l92B8;
l92B4:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x0492b5, 4);
    R[1] = gsu_ldw(R[10]);
l92B8:
    R[0] = 0x000E;
    R[0] = gsu_add(R[0], R[1], 0);
    gsu_code(0x0492b9, 4);
    R[0] = gsu_ldw(R[0]);
    R[1] = R[0];
    gsu_moves(R[0]);
    t = gsu.z;
    gsu_code(0x0492bd, 4);
    if (t) { gsu_code(0x0492c5, 1); goto l92C5; }
    gsu_code(0x0492c1, 4); gsu_code(0x049229, 1);
    goto l9229;
l92C5:
    R[2] = 0x0020;
    R[2] = gsu_add(R[2], R[9], 0);
    gsu_code(0x0492c6, 6);
    R[2] = gsu_ldw(R[2]);
    R[11] = 0x92D0;
    gsu_code(0x0492cc, 4); gsu_code(0x049683, 1);
    abi_FxPushSet();
l92D0:
    R[3] = 0x23A4;
    R[11] = 0x92D8;
    gsu_code(0x0492d1, 7); gsu_code(0x04944c, 1);
    abi_FxPushApply();
    gsu_code(0x0492d8, 1);
l92D8:
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.z;
    gsu_code(0x0492d9, 4);
    if (t) { gsu_code(0x0492f7, 1); goto l92F7; }
    R[3] = 0x0001;
    R[0] = 0x001C;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_code(0x0492dd, 8);
    R[0] = gsu_ldw(R[0]);
    R[0] = gsu_or(R[0], R[3]);
    gsu_code(0x0492e5, 2);
    gsu_sbk(R[0]);
    gsu_code(0x0492e7, 4);
    R[0] = gsu_ldw(FX_HIT_DEPTH);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = !gsu.z;
    gsu_code(0x0492eb, 4);
    if (t) { gsu_code(0x0492f7, 1); goto l92F7; }
    gsu_code(0x0492ef, 5);
    R[0] = gsu_ldw(FX_PUSH_AMOUNT);
    gsu_code(0x0492f4, 4);
    gsu_stw(FX_HIT_DEPTH, R[0]);
l92F7:
    R[11] = 0x92FC;
    gsu_code(0x0492f8, 4); gsu_code(0x049bd2, 1);
    abi_FxSumsApply();
    gsu_code(0x0492fc, 1);
l92FC:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x0492fd, 4);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
    /* FxContactsAnyTouch */
l957C:
    /* CACHE */
    R[3] = 0x23AE;
    R[13] = 0x0000;
    gsu_code(0x04957d, 6);
l9582:
    R[13] = gsu_inc(R[13]);
    R[0] = 0x0014;
    gsu_sub(R[0], R[13], 0);   /* CMP */
    t = gsu.z;
    gsu_code(0x049583, 7);
    if (t) { gsu_code(0x0495b4, 1); goto l95B4; }
    gsu_code(0x04958a, 2);
    R[0] = gsu_ldw(R[3]);
    R[3] = gsu_inc(R[3]);
    R[3] = gsu_inc(R[3]);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.z;
    gsu_code(0x04958c, 6);
    if (t) { gsu_code(0x0495a6, 1); goto l95A6; }
    gsu_code(0x049592, 3);
    R[4] = gsu_ldw(R[3]);
    R[3] = gsu_inc(R[3]);
    R[3] = gsu_inc(R[3]);
    R[0] = 0x7D00;
    gsu_sub(R[0], R[4], 0);   /* CMP */
    t = gsu.z;
    gsu_code(0x049595, 9);
    if (t) { gsu_code(0x0495ad, 1); goto l95AD; }
    R[4] = R[4];
    gsu_moves(R[4]);
    t = !gsu.s;
    gsu_code(0x04959e, 5);
    if (t) { gsu_code(0x0495b9, 1); goto l95B9; }
    t = 1;
    gsu_code(0x0495a3, 3);
    gsu_code(0x0495ad, 1);
    if (t) goto l95AD;
l95A6:
    R[0] = 0x0008;
    R[3] = gsu_add(R[3], R[0], 0);
    t = 1;
    gsu_code(0x0495a7, 6);
    gsu_code(0x049582, 1);
    if (t) goto l9582;
l95AD:
    R[0] = 0x0006;
    R[3] = gsu_add(R[3], R[0], 0);
    t = 1;
    gsu_code(0x0495ae, 6);
    gsu_code(0x049582, 1);
    if (t) goto l9582;
l95B4:
    R[0] = 0x0000;
    nx = (uint16_t)(R[11]);
    at = nx;
    gsu_code(0x0495b5, 4);
    goto dispatch;
l95B9:
    R[0] = 0x0001;
    nx = (uint16_t)(R[11]);
    at = nx;
    gsu_code(0x0495ba, 4);
    goto dispatch;
    /* FxCollideModel */
l9C3D:
    gsu_code(0x049c3e, 2);
    gsu_stw(R[10], R[11]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_code(0x049c40, 4);
    gsu_stw(R[10], R[8]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_code(0x049c44, 4);
    gsu_stw(R[10], R[9]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[0] = 0x0012;
    R[0] = gsu_add(R[0], R[7], 0);
    gsu_code(0x049c48, 6);
    R[0] = gsu_ldw(R[0]);
    gsu_code(0x049c4e, 3);
    gsu_stw(GSU_SPEED_44, R[0]);
    R[0] = 0x0000;
    R[1] = 0x7D00;
    gsu_code(0x049c51, 10);
    gsu_stw(FX_PUSHES, R[0]);
    gsu_code(0x049c5b, 4);
    gsu_stw((FX_PUSHES + 2), R[1]);
    gsu_code(0x049c5f, 4);
    gsu_stw((FX_PUSHES + 10), R[0]);
    gsu_code(0x049c63, 4);
    gsu_stw((FX_PUSHES + 12), R[1]);
    gsu_code(0x049c67, 4);
    gsu_stw((FX_PUSHES + 20), R[0]);
    gsu_code(0x049c6b, 4);
    gsu_stw((FX_PUSHES + 22), R[1]);
    gsu_code(0x049c6f, 4);
    gsu_stw((FX_PUSHES + 30), R[0]);
    gsu_code(0x049c73, 4);
    gsu_stw((FX_PUSHES + 32), R[1]);
    gsu_code(0x049c77, 4);
    gsu_stw((FX_PUSHES + 40), R[0]);
    gsu_code(0x049c7b, 4);
    gsu_stw((FX_PUSHES + 42), R[1]);
    gsu_code(0x049c7f, 4);
    gsu_stw((FX_PUSHES + 50), R[0]);
    gsu_code(0x049c83, 4);
    gsu_stw((FX_PUSHES + 52), R[1]);
    gsu_code(0x049c87, 4);
    gsu_stw((FX_PUSHES + 60), R[0]);
    gsu_code(0x049c8b, 4);
    gsu_stw((FX_PUSHES + 62), R[1]);
    gsu_code(0x049c8f, 4);
    gsu_stw((FX_PUSHES + 70), R[0]);
    gsu_code(0x049c93, 4);
    gsu_stw((FX_PUSHES + 72), R[1]);
    gsu_code(0x049c97, 4);
    gsu_stw((FX_PUSHES + 80), R[0]);
    gsu_code(0x049c9b, 4);
    gsu_stw((FX_PUSHES + 82), R[1]);
    gsu_code(0x049c9f, 4);
    gsu_stw((FX_PUSHES + 90), R[0]);
    gsu_code(0x049ca3, 4);
    gsu_stw((FX_PUSHES + 92), R[1]);
    gsu_code(0x049ca7, 4);
    gsu_stw((FX_PUSHES + 100), R[0]);
    gsu_code(0x049cab, 4);
    gsu_stw((FX_PUSHES + 102), R[1]);
    gsu_code(0x049caf, 4);
    gsu_stw((FX_PUSHES + 110), R[0]);
    gsu_code(0x049cb3, 4);
    gsu_stw((FX_PUSHES + 112), R[1]);
    gsu_code(0x049cb7, 4);
    gsu_stw((FX_PUSHES + 120), R[0]);
    gsu_code(0x049cbb, 4);
    gsu_stw((FX_PUSHES + 122), R[1]);
    gsu_code(0x049cbf, 4);
    gsu_stw((FX_PUSHES + 130), R[0]);
    gsu_code(0x049cc3, 4);
    gsu_stw((FX_PUSHES + 132), R[1]);
    gsu_code(0x049cc7, 4);
    gsu_stw((FX_PUSHES + 140), R[0]);
    gsu_code(0x049ccb, 4);
    gsu_stw((FX_PUSHES + 142), R[1]);
    gsu_code(0x049ccf, 4);
    gsu_stw((FX_PUSHES + 150), R[0]);
    gsu_code(0x049cd3, 4);
    gsu_stw((FX_PUSHES + 152), R[1]);
    gsu_code(0x049cd7, 4);
    gsu_stw((FX_PUSHES + 160), R[0]);
    gsu_code(0x049cdb, 4);
    gsu_stw((FX_PUSHES + 162), R[1]);
    gsu_code(0x049cdf, 4);
    gsu_stw((FX_PUSHES + 170), R[0]);
    gsu_code(0x049ce3, 4);
    gsu_stw((FX_PUSHES + 172), R[1]);
    gsu_code(0x049ce7, 4);
    gsu_stw((FX_PUSHES + 180), R[0]);
    gsu_code(0x049ceb, 4);
    gsu_stw((FX_PUSHES + 182), R[1]);
    R[0] = 0x0000;
    gsu_code(0x049cef, 4);
    gsu_romb(R[0]);
    R[1] = 0x0014;
    R[1] = gsu_add(R[1], R[7], 0);
    gsu_code(0x049cf3, 6);
    R[1] = gsu_ldw(R[1]);
    R[1] = R[1];
    gsu_moves(R[1]);
    t = !gsu.z;
    gsu_code(0x049cf9, 4);
    if (t) { gsu_code(0x049d01, 1); goto l9D01; }
    gsu_code(0x049cfd, 4); gsu_code(0x04a145, 1);
    goto lA145;
l9D01:
    gsu_code(0x049d02, 2);
    gsu_r14(R[1]);
    R[0] = 0x000C;
    gsu_code(0x049d04, 5);
    gsu_r14(gsu_add(R[14], R[0], 0));
    gsu_code(0x049d09, 1);
    R[0] = gsu_getb();
    gsu_code(0x049d0a, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x049d0b, 2);
    R[0] = gsu_getbh(R[0]);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = !gsu.z;
    gsu_code(0x049d0d, 4);
    if (t) { gsu_code(0x049d15, 1); goto l9D15; }
    gsu_code(0x049d11, 4); gsu_code(0x04a145, 1);
    goto lA145;
l9D15:
    gsu_code(0x049d16, 2);
    gsu_r14(R[1]);
    R[4] = 0x001E;
    R[4] = gsu_add(R[4], R[9], 0);
    gsu_code(0x049d18, 6);
    R[4] = gsu_ldw(R[4]);
    R[0] = 0x001E;
    R[0] = gsu_add(R[0], R[7], 0);
    gsu_code(0x049d1e, 4);
    R[0] = gsu_ldw(R[0]);
    R[4] = gsu_sub(R[4], R[0], 0);
    gsu_code(0x049d22, 5);
    gsu_stw(GSU_IN_20, R[4]);
    R[5] = 0x0020;
    R[5] = gsu_add(R[5], R[9], 0);
    gsu_code(0x049d27, 6);
    R[5] = gsu_ldw(R[5]);
    R[0] = 0x0020;
    R[0] = gsu_add(R[0], R[7], 0);
    gsu_code(0x049d2d, 4);
    R[0] = gsu_ldw(R[0]);
    R[5] = gsu_sub(R[5], R[0], 0);
    gsu_code(0x049d31, 5);
    gsu_stw(GSU_IN_22, R[5]);
    R[8] = 0x0022;
    R[8] = gsu_add(R[8], R[9], 0);
    gsu_code(0x049d36, 6);
    R[8] = gsu_ldw(R[8]);
    R[0] = 0x0022;
    R[0] = gsu_add(R[0], R[7], 0);
    gsu_code(0x049d3c, 4);
    R[0] = gsu_ldw(R[0]);
    R[8] = gsu_sub(R[8], R[0], 0);
    gsu_code(0x049d40, 5);
    gsu_stw(GSU_IN_24, R[8]);
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_code(0x049d45, 5);
    gsu_stw(FX_PUSH_TURN, R[0]);
    R[0] = 0x0024;
    R[0] = gsu_add(R[0], R[7], 0);
    gsu_code(0x049d4a, 4);
    R[0] = gsu_ldw(R[0]);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = !gsu.z;
    gsu_code(0x049d4e, 4);
    if (t) { gsu_code(0x049d56, 1); goto l9D56; }
    gsu_code(0x049d52, 4); gsu_code(0x049def, 1);
    goto l9DEF;
l9D56:
    R[0] = gsu_hib(R[0]);
    gsu_code(0x049d57, 5);
    gsu_stw(FX_PUSH_TURN, R[0]);
    R[7] = 0x00C0;
    gsu_sub(R[0], R[7], 0);   /* CMP */
    t = !gsu.z;
    gsu_code(0x049d5c, 7);
    if (t) { gsu_code(0x049d6f, 1); goto l9D6F; }
    R[0] = R[8];
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
    R[8] = R[4];
    R[4] = R[0];
    gsu_code(0x049d63, 12); gsu_code(0x049def, 1);
    goto l9DEF;
l9D6F:
    R[7] = 0x0040;
    gsu_sub(R[0], R[7], 0);   /* CMP */
    t = !gsu.z;
    gsu_code(0x049d70, 7);
    if (t) { gsu_code(0x049d82, 1); goto l9D82; }
    R[0] = R[4];
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
    R[4] = R[8];
    R[8] = R[0];
    t = 1;
    gsu_code(0x049d77, 11);
    gsu_code(0x049def, 1);
    if (t) goto l9DEF;
l9D82:
    R[7] = 0x0080;
    gsu_sub(R[0], R[7], 0);   /* CMP */
    t = !gsu.z;
    gsu_code(0x049d83, 7);
    if (t) { gsu_code(0x049d93, 1); goto l9D93; }
    R[4] = gsu_not(R[4]);
    R[4] = gsu_inc(R[4]);
    R[8] = gsu_not(R[8]);
    R[8] = gsu_inc(R[8]);
    t = 1;
    gsu_code(0x049d8a, 9);
    gsu_code(0x049def, 1);
    if (t) goto l9DEF;
l9D93:
    gsu_code(0x049d94, 2);
    gsu_stw(R[10], R[14]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_code(0x049d96, 4);
    gsu_stw(R[10], R[1]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_code(0x049d9a, 4);
    gsu_stw(R[10], R[2]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_code(0x049d9e, 4);
    gsu_stw(R[10], R[3]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_code(0x049da2, 4);
    gsu_stw(R[10], R[5]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_code(0x049da6, 4);
    gsu_stw(R[10], R[7]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_code(0x049daa, 4);
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
    gsu_code(0x049dae, 17);
    gsu_romb(R[0]);
    R[11] = 0x9DC3;
    gsu_code(0x049dbf, 4); gsu_code(0x04c15c, 1);
    abi_FxTurn();
    gsu_code(0x049dc3, 1);
l9DC3:
    R[1] = gsu_add(R[1], R[1], 0);
    R[1] = gsu_add(R[1], R[1], 0);
    R[3] = gsu_add(R[3], R[3], 0);
    R[3] = gsu_add(R[3], R[3], 0);
    R[4] = R[3];
    R[8] = R[1];
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x049dc4, 16);
    R[9] = gsu_ldw(R[10]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x049dd4, 4);
    R[7] = gsu_ldw(R[10]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x049dd8, 4);
    R[5] = gsu_ldw(R[10]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x049ddc, 4);
    R[3] = gsu_ldw(R[10]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x049de0, 4);
    R[2] = gsu_ldw(R[10]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x049de4, 4);
    R[1] = gsu_ldw(R[10]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x049de8, 4);
    gsu_r14(gsu_ldw(R[10]));
    R[0] = 0x0000;
    gsu_code(0x049dec, 4);
    gsu_romb(R[0]);
l9DEF:
    R[0] = 0x0003;
    gsu_code(0x049df0, 5);
    gsu_r14(gsu_add(R[14], R[0], 0));
    gsu_code(0x049df5, 2);
    R[2] = gsu_getb();
    gsu_code(0x049df7, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x049df8, 3);
    R[2] = gsu_getbh(R[2]);
    R[0] = 0x0000;
    gsu_code(0x049dfb, 5);
    gsu_r14(R[1]);
    gsu_code(0x049e00, 2);
    gsu_r14(gsu_add(R[14], R[0], 0));
    gsu_code(0x049e02, 2);
    R[9] = gsu_getb();
    gsu_code(0x049e04, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x049e05, 3);
    R[9] = gsu_getbh(R[9]);
    R[0] = 0x0007;
    gsu_code(0x049e08, 5);
    gsu_r14(R[1]);
    gsu_code(0x049e0d, 2);
    gsu_r14(gsu_add(R[14], R[0], 0));
    gsu_code(0x049e0f, 2);
    R[0] = gsu_getbs();
    R[12] = R[0];
    R[0] = 0x0001;
    R[13] = 0x9E16;
    gsu_code(0x049e11, 6);
l9E16:
    R[0] = gsu_add(R[0], R[0], 0);
    t = gsu_count_down(&R[12]);
    gsu_code(0x049e17, 2);
    if (t) { gsu_code(0x049e16, 1); goto l9E16; }
    gsu_code(0x049e19, 5);
    gsu_stw(FX_FACE_SCALE, R[0]);
    R[0] = 0x0002;
    gsu_code(0x049e1e, 5);
    gsu_r14(R[1]);
    gsu_code(0x049e23, 2);
    gsu_r14(gsu_add(R[14], R[0], 0));
    gsu_code(0x049e25, 1);
    R[0] = gsu_getb();
    gsu_code(0x049e26, 2);
    gsu_romb(R[0]);
    gsu_code(0x049e28, 4);
    R[3] = gsu_ldw(GSU_SPEED_44);
    gsu_code(0x049e2c, 4);
    R[0] = gsu_ldw(FX_INIT_2380);
    gsu_sub(R[0], R[1], 0);   /* CMP */
    t = !gsu.z;
    gsu_code(0x049e30, 4);
    if (t) { gsu_code(0x049e3d, 1); goto l9E3D; }
    gsu_code(0x049e34, 5);
    R[0] = gsu_ldw(FX_INIT_2382);
    gsu_sub(R[0], R[3], 0);   /* CMP */
    t = gsu.z;
    gsu_code(0x049e39, 4);
    if (t) { gsu_code(0x049e4a, 1); goto l9E4A; }
    gsu_code(0x049e3d, 1);
l9E3D:
    gsu_code(0x049e3e, 4);
    gsu_stw(FX_INIT_2380, R[1]);
    gsu_code(0x049e42, 4);
    gsu_stw(FX_INIT_2382, R[3]);
    R[11] = 0x9E4A;
    gsu_code(0x049e46, 4); gsu_code(0x04a154, 1);
    abi_FxModelPoints();
    gsu_code(0x049e4a, 1);
l9E4A:
    R[9] = 0x2488;
    gsu_code(0x049e4b, 5);
    gsu_r14(R[2]);
    gsu_code(0x049e50, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x049e51, 1);
    R[0] = gsu_getb();
    gsu_code(0x049e52, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x049e53, 2);
    R[0] = gsu_umult(R[0], 3);
    gsu_code(0x049e55, 2); gsu_cache(0x9e57);
    gsu_r14(gsu_add(R[14], R[0], 0));
    /* CACHE */
    gsu_code(0x049e57, 3);
    R[0] = gsu_getbs();
    R[13] = 0x003C;
    gsu_sub(R[0], R[13], 0);   /* CMP */
    t = !gsu.z;
    gsu_code(0x049e5a, 6);
    gsu_code(t ? 0x049e92 : 0x049e60, 1);   /* the delay slot runs after the next fetch */
    gsu_r14(gsu_inc(R[14]));
    if (t) goto l9E92;
    R[1] = 0x0028;
    R[7] = 0x0005;
    R[2] = 0x0044;
    R[12] = 0x0003;
    R[3] = 0x0040;
    gsu_code(0x049e61, 10);
l9E6A:
    gsu_code(0x049e6b, 2);
    R[0] = gsu_getbs();
    gsu_sub(R[0], R[3], 0);   /* CMP */
    t = !gsu.z;
    gsu_code(0x049e6d, 4);
    if (t) { gsu_code(0x049e74, 1); goto l9E74; }
    t = 1;
    gsu_code(0x049e71, 3); gsu_code(0x049e6a, 1);
    gsu_r14(gsu_inc(R[14]));
    if (t) goto l9E6A;
l9E74:
    gsu_sub(R[0], R[1], 0);   /* CMP */
    t = !gsu.z;
    gsu_code(0x049e75, 4);
    if (t) { gsu_code(0x049e7d, 1); goto l9E7D; }
    t = 1;
    gsu_code(0x049e79, 4); gsu_code(0x049e6a, 1);
    gsu_r14(gsu_add(R[14], R[7], 0));
    if (t) goto l9E6A;
l9E7D:
    gsu_sub(R[0], R[2], 0);   /* CMP */
    t = !gsu.z;
    gsu_code(0x049e7e, 4);
    if (t) { gsu_code(0x049e87, 1); goto l9E87; }
    t = 1;
    gsu_code(0x049e82, 3); gsu_code(0x049e6a, 1);
    gsu_r14(gsu_add(R[14], R[12], 0));
    if (t) goto l9E6A;
l9E85:
    gsu_code(0x049e86, 2);
    R[0] = gsu_getbs();
l9E87:
    gsu_code(0x049e88, 1);
    gsu_r14(gsu_inc(R[14]));
    R[0] = R[0];
    gsu_moves(R[0]);
    t = !gsu.z;
    gsu_code(0x049e89, 4);
    if (t) { gsu_code(0x049e91, 1); gsu_cache(0x9e92); goto l9E91; }
    gsu_code(0x049e8d, 4); gsu_code(0x04a0f8, 1);
    goto lA0F8;
l9E91:
    /* CACHE */
    gsu_code(0x049e92, 1);
l9E92:
    gsu_code(0x049e93, 3);
    R[13] = gsu_getbs();
    R[13] = R[13];
    gsu_moves(R[13]);
    t = gsu.s;
    gsu_code(0x049e96, 4);
    gsu_code(t ? 0x049e85 : 0x049e9a, 1);   /* the delay slot runs after the next fetch */
    gsu_r14(gsu_inc(R[14]));
    if (t) goto l9E85;
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_code(0x049e9b, 4);
    R[11] = gsu_getbs();
    gsu_code(0x049e9f, 1);
    gsu_r14(gsu_inc(R[14]));
    R[11] = R[11];
    gsu_moves(R[11]);
    t = !gsu.z;
    gsu_code(0x049ea0, 4);
    if (t) { gsu_code(0x049ea8, 1); goto l9EA8; }
    gsu_code(0x049ea4, 4); gsu_code(0x04a0ef, 1);
    goto lA0EF;
l9EA8:
    R[0] = R[11];
    gsu_code(0x049ea9, 6);
    gsu_stw(FX_FACE_FLAGS, R[0]);
    R[1] = 0x00E0;
    R[11] = gsu_bic(R[11], R[1]);
    R[0] = 0x000A;
    gsu_code(0x049eaf, 10);
    R[11] = gsu_mult(R[0], R[11]);
    R[0] = 0x23A4;
    R[11] = gsu_add(R[11], R[0], 0);
    gsu_code(0x049eb9, 6);
    R[0] = gsu_ldw(R[11]);
    R[0] = gsu_hib(R[0]);
    t = gsu.z;
    gsu_code(0x049ebf, 3);
    if (t) { gsu_code(0x049ec6, 1); goto l9EC6; }
    gsu_code(0x049ec2, 4); gsu_code(0x04a0ef, 1);
    goto lA0EF;
l9EC6:
    gsu_code(0x049ec7, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x049ec8, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x049ec9, 3);
    R[0] = gsu_ldw(GSU_SPEED_44);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = !gsu.z;
    gsu_code(0x049ecc, 4);
    if (t) { gsu_code(0x049ed4, 1); goto l9ED4; }
    gsu_code(0x049ed0, 4); gsu_code(0x04a013, 1);
    goto lA013;
l9ED4:
    gsu_code(0x049ed5, 4);
    R[7] = gsu_ldw(FX_FACE_FLAGS);
    R[0] = 0x0040;
    R[0] = gsu_and(R[0], R[7]);
    t = !gsu.z;
    gsu_code(0x049ed9, 6);
    if (t) { gsu_code(0x049ee3, 1); goto l9EE3; }
    gsu_code(0x049edf, 4); gsu_code(0x04a013, 1);
    goto lA013;
l9EE3:
    gsu_code(0x049ee4, 3);
    gsu_r14(gsu_add(R[14], 3, 0));
    gsu_code(0x049ee7, 2);
    gsu_stw(R[10], R[14]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[12] = R[9];
    gsu_code(0x049ee9, 5);
    R[0] = gsu_getb();
    gsu_code(0x049eee, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x049eef, 2);
    R[0] = gsu_umult(R[0], 6);
    R[9] = gsu_add(R[0], R[12], 0);
    gsu_code(0x049ef1, 3);
    R[0] = gsu_ldw(R[9]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    gsu_code(0x049ef4, 5);
    gsu_stw(GSU_IN_68, R[0]);
    gsu_code(0x049ef9, 1);
    R[0] = gsu_ldw(R[9]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    gsu_code(0x049efa, 5);
    gsu_stw(GSU_DEST, R[0]);
    gsu_code(0x049eff, 1);
    R[0] = gsu_ldw(R[9]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    gsu_code(0x049f00, 5);
    gsu_stw(GSU_IN_2E, R[0]);
    gsu_code(0x049f05, 1);
    R[0] = gsu_getb();
    gsu_code(0x049f06, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x049f07, 2);
    R[0] = gsu_umult(R[0], 6);
    R[9] = gsu_add(R[0], R[12], 0);
    gsu_code(0x049f09, 3);
    R[0] = gsu_ldw(R[9]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    gsu_code(0x049f0c, 5);
    gsu_stw(GSU_IN_6A, R[0]);
    gsu_code(0x049f11, 1);
    R[0] = gsu_ldw(R[9]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    gsu_code(0x049f12, 5);
    gsu_stw(GSU_IN_A2, R[0]);
    gsu_code(0x049f17, 1);
    R[0] = gsu_ldw(R[9]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    gsu_code(0x049f18, 5);
    gsu_stw(GSU_IN_A4, R[0]);
    gsu_code(0x049f1d, 1);
    R[0] = gsu_getb();
    gsu_code(0x049f1e, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x049f1f, 2);
    R[0] = gsu_umult(R[0], 6);
    R[9] = gsu_add(R[0], R[12], 0);
    gsu_code(0x049f21, 4);
    R[1] = gsu_ldw(R[9]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    gsu_code(0x049f25, 4);
    R[2] = gsu_ldw(R[9]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    gsu_code(0x049f29, 4);
    R[3] = gsu_ldw(R[9]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    gsu_code(0x049f2d, 5);
    R[0] = gsu_ldw(GSU_IN_68);
    gsu_code(0x049f32, 3);
    R[9] = gsu_ldw(GSU_IN_6A);
    R[0] = gsu_sub(R[0], R[9], 0);
    gsu_code(0x049f35, 4);
    gsu_stw(GSU_IN_68, R[0]);
    gsu_code(0x049f39, 3);
    R[0] = gsu_ldw(GSU_DEST);
    gsu_code(0x049f3c, 3);
    R[9] = gsu_ldw(GSU_IN_A2);
    R[0] = gsu_sub(R[0], R[9], 0);
    gsu_code(0x049f3f, 4);
    gsu_stw(GSU_DEST, R[0]);
    gsu_code(0x049f43, 3);
    R[0] = gsu_ldw(GSU_IN_2E);
    gsu_code(0x049f46, 3);
    R[9] = gsu_ldw(GSU_IN_A4);
    R[0] = gsu_sub(R[0], R[9], 0);
    gsu_code(0x049f49, 4);
    gsu_stw(GSU_IN_2E, R[0]);
    gsu_code(0x049f4d, 3);
    R[0] = gsu_ldw(GSU_IN_6A);
    R[0] = gsu_sub(R[0], R[1], 0);
    gsu_code(0x049f50, 4);
    gsu_stw(GSU_IN_6A, R[0]);
    gsu_code(0x049f54, 3);
    R[0] = gsu_ldw(GSU_IN_A2);
    R[0] = gsu_sub(R[0], R[2], 0);
    gsu_code(0x049f57, 4);
    gsu_stw(GSU_IN_A2, R[0]);
    gsu_code(0x049f5b, 3);
    R[0] = gsu_ldw(GSU_IN_A4);
    R[0] = gsu_sub(R[0], R[3], 0);
    gsu_code(0x049f5e, 4);
    gsu_stw(GSU_IN_A4, R[0]);
    gsu_code(0x049f62, 2);
    gsu_stw(R[10], R[4]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_code(0x049f64, 4);
    gsu_stw(R[10], R[5]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_code(0x049f68, 4);
    gsu_stw(R[10], R[8]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_code(0x049f6c, 4);
    gsu_stw(R[10], R[12]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_code(0x049f70, 4);
    gsu_stw(R[10], R[13]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_code(0x049f74, 4);
    gsu_stw(R[10], R[11]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_code(0x049f78, 5);
    R[1] = gsu_ldw(GSU_IN_68);
    gsu_code(0x049f7d, 3);
    R[2] = gsu_ldw(GSU_DEST);
    gsu_code(0x049f80, 3);
    R[3] = gsu_ldw(GSU_IN_2E);
    R[11] = 0x9F87;
    gsu_code(0x049f83, 4); gsu_code(0x04c087, 1);
    abi_FxNormalize();
    gsu_code(0x049f87, 1);
l9F87:
    gsu_code(0x049f88, 3);
    gsu_stw(GSU_IN_68, R[1]);
    gsu_code(0x049f8b, 3);
    gsu_stw(GSU_DEST, R[2]);
    gsu_code(0x049f8e, 3);
    gsu_stw(GSU_IN_2E, R[3]);
    gsu_code(0x049f91, 3);
    R[1] = gsu_ldw(GSU_IN_6A);
    gsu_code(0x049f94, 3);
    R[2] = gsu_ldw(GSU_IN_A2);
    gsu_code(0x049f97, 3);
    R[3] = gsu_ldw(GSU_IN_A4);
    R[11] = 0x9F9E;
    gsu_code(0x049f9a, 4); gsu_code(0x04c087, 1);
    abi_FxNormalize();
    gsu_code(0x049f9e, 1);
l9F9E:
    gsu_code(0x049f9f, 3);
    R[6] = gsu_ldw(GSU_DEST);
    gsu_code(0x049fa2, 3);
    { uint32_t p = gsu_lmult(R[3], R[6]); R[4] = (uint16_t)p; R[0] = (uint16_t)(p >> 16); }
    R[7] = R[4];
    gsu_code(0x049fa5, 5);
    R[6] = gsu_ldw(GSU_IN_2E);
    gsu_code(0x049faa, 4);
    { uint32_t p = gsu_lmult(R[2], R[6]); R[4] = (uint16_t)p; R[5] = (uint16_t)(p >> 16); }
    R[7] = gsu_sub(R[7], R[4], 0);
    R[0] = gsu_sub(R[0], R[5], !gsu.cy);
    R[7] = gsu_add(R[7], R[7], 0);
    R[0] = gsu_rol(R[0]);
    gsu_code(0x049fae, 10);
    gsu_stw(GSU_IN_6A, R[0]);
    gsu_code(0x049fb8, 3);
    R[6] = gsu_ldw(GSU_IN_2E);
    gsu_code(0x049fbb, 3);
    { uint32_t p = gsu_lmult(R[1], R[6]); R[4] = (uint16_t)p; R[0] = (uint16_t)(p >> 16); }
    R[7] = R[4];
    gsu_code(0x049fbe, 5);
    R[6] = gsu_ldw(GSU_IN_68);
    gsu_code(0x049fc3, 4);
    { uint32_t p = gsu_lmult(R[3], R[6]); R[4] = (uint16_t)p; R[5] = (uint16_t)(p >> 16); }
    R[7] = gsu_sub(R[7], R[4], 0);
    R[0] = gsu_sub(R[0], R[5], !gsu.cy);
    R[7] = gsu_add(R[7], R[7], 0);
    R[0] = gsu_rol(R[0]);
    gsu_code(0x049fc7, 10);
    gsu_stw(GSU_IN_A2, R[0]);
    gsu_code(0x049fd1, 3);
    R[6] = gsu_ldw(GSU_IN_68);
    gsu_code(0x049fd4, 3);
    { uint32_t p = gsu_lmult(R[2], R[6]); R[4] = (uint16_t)p; R[0] = (uint16_t)(p >> 16); }
    R[7] = R[4];
    gsu_code(0x049fd7, 5);
    R[6] = gsu_ldw(GSU_DEST);
    gsu_code(0x049fdc, 4);
    { uint32_t p = gsu_lmult(R[1], R[6]); R[4] = (uint16_t)p; R[5] = (uint16_t)(p >> 16); }
    R[7] = gsu_sub(R[7], R[4], 0);
    R[0] = gsu_sub(R[0], R[5], !gsu.cy);
    R[7] = gsu_add(R[7], R[7], 0);
    R[0] = gsu_rol(R[0]);
    gsu_code(0x049fe0, 10);
    gsu_stw(GSU_IN_A4, R[0]);
    gsu_code(0x049fea, 3);
    R[1] = gsu_ldw(GSU_IN_6A);
    gsu_code(0x049fed, 3);
    R[2] = gsu_ldw(GSU_IN_A2);
    gsu_code(0x049ff0, 3);
    R[3] = gsu_ldw(GSU_IN_A4);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x049ff3, 4);
    R[11] = gsu_ldw(R[10]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x049ff7, 4);
    R[13] = gsu_ldw(R[10]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x049ffb, 4);
    R[12] = gsu_ldw(R[10]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x049fff, 4);
    R[8] = gsu_ldw(R[10]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x04a003, 4);
    R[5] = gsu_ldw(R[10]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x04a007, 4);
    R[4] = gsu_ldw(R[10]);
    R[9] = R[12];
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x04a00b, 6);
    gsu_r14(gsu_ldw(R[10]));
    t = 1;
    gsu_code(0x04a011, 2);
    gsu_code(0x04a028, 1);
    if (t) goto lA028;
lA013:
    gsu_code(0x04a014, 1);
    R[0] = gsu_getb();
    gsu_code(0x04a015, 1);
    gsu_r14(gsu_inc(R[14]));
    R[1] = gsu_swap(R[0]);
    R[1] = gsu_not(R[1]);
    R[1] = gsu_inc(R[1]);
    gsu_code(0x04a016, 6);
    R[0] = gsu_getb();
    gsu_code(0x04a01c, 1);
    gsu_r14(gsu_inc(R[14]));
    R[2] = gsu_swap(R[0]);
    R[2] = gsu_not(R[2]);
    R[2] = gsu_inc(R[2]);
    gsu_code(0x04a01d, 6);
    R[0] = gsu_getb();
    gsu_code(0x04a023, 1);
    gsu_r14(gsu_inc(R[14]));
    R[3] = gsu_swap(R[0]);
    R[3] = gsu_not(R[3]);
    R[3] = gsu_inc(R[3]);
    gsu_code(0x04a024, 5);
lA028:
    gsu_code(0x04a029, 4);
    R[7] = gsu_ldw(FX_FACE_SCALE);
    gsu_code(0x04a02d, 2);
    R[0] = gsu_getbs();
    gsu_code(0x04a02f, 2);
    gsu_r14(gsu_add(R[14], R[13], 0));
    R[13] = R[9];
    gsu_code(0x04a031, 4);
    R[0] = gsu_umult(R[0], 6);
    R[9] = gsu_add(R[9], R[0], 0);
    gsu_code(0x04a035, 3);
    R[0] = gsu_ldw(R[9]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    gsu_code(0x04a038, 3);
    R[0] = gsu_mult(R[0], R[7]);
    gsu_code(0x04a03b, 4);
    gsu_stw(FX_FACE_NORMAL, R[0]);
    gsu_code(0x04a03f, 1);
    R[0] = gsu_ldw(R[9]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    gsu_code(0x04a040, 3);
    R[0] = gsu_mult(R[0], R[7]);
    gsu_code(0x04a043, 4);
    gsu_stw((FX_FACE_NORMAL + 2), R[0]);
    gsu_code(0x04a047, 1);
    R[0] = gsu_ldw(R[9]);
    gsu_code(0x04a048, 1);
    R[0] = gsu_mult(R[0], R[7]);
    gsu_code(0x04a049, 4);
    gsu_stw((FX_FACE_NORMAL + 4), R[0]);
    R[9] = R[13];
    gsu_code(0x04a04d, 3);
    R[0] = gsu_ldw(R[11]);
    R[0] = gsu_hib(R[0]);
    t = gsu.z;
    gsu_code(0x04a050, 3);
    if (t) { gsu_code(0x04a057, 1); goto lA057; }
    gsu_code(0x04a053, 1);
lA053:
    gsu_code(0x04a054, 3); gsu_code(0x049e92, 1);
    goto l9E92;
lA057:
    R[6] = R[4];
    gsu_code(0x04a058, 5);
    R[7] = gsu_fmult(R[1], R[6]);
    R[6] = R[5];
    gsu_code(0x04a05d, 4);
    R[0] = gsu_fmult(R[2], R[6]);
    R[7] = gsu_add(R[7], R[0], 0);
    R[6] = R[8];
    gsu_code(0x04a061, 6);
    R[0] = gsu_fmult(R[3], R[6]);
    R[0] = gsu_add(R[0], R[7], 0);
    gsu_code(0x04a067, 5);
    gsu_stw(FX_FACE_PLANE, R[0]);
    gsu_code(0x04a06c, 4);
    R[6] = gsu_ldw(FX_FACE_NORMAL);
    gsu_code(0x04a070, 3);
    R[7] = gsu_fmult(R[1], R[6]);
    gsu_code(0x04a073, 4);
    R[6] = gsu_ldw((FX_FACE_NORMAL + 2));
    gsu_code(0x04a077, 2);
    R[0] = gsu_fmult(R[2], R[6]);
    R[7] = gsu_add(R[7], R[0], 0);
    gsu_code(0x04a079, 6);
    R[6] = gsu_ldw((FX_FACE_NORMAL + 4));
    gsu_code(0x04a07f, 2);
    R[0] = gsu_fmult(R[3], R[6]);
    R[0] = gsu_add(R[0], R[7], 0);
    gsu_code(0x04a081, 5);
    R[7] = gsu_ldw(FX_FACE_PLANE);
    R[0] = gsu_sub(R[0], R[7], 0);
    t = !gsu.s;
    gsu_code(0x04a086, 3);
    if (t) { gsu_code(0x04a09c, 1); goto lA09C; }
    R[12] = R[0];
    gsu_code(0x04a089, 7);
    R[0] = gsu_ldw(FX_FACE_FLAGS);
    R[0] = gsu_swap(R[0]);
    gsu_code(0x04a090, 3);
    R[7] = gsu_ldw(R[11]);
    R[0] = gsu_or(R[0], R[7]);
    R[7] = 0xFF00;
    R[0] = gsu_and(R[0], R[7]);
    gsu_code(0x04a093, 6);
    gsu_sbk(R[0]);
    gsu_code(0x04a099, 3); gsu_code(0x049e92, 1);
    goto l9E92;
lA09C:
    gsu_code(0x04a09d, 4);
    R[7] = gsu_ldw(FX_FACE_FLAGS);
    R[12] = 0x0080;
    R[12] = gsu_and(R[12], R[7]);
    t = gsu.z;
    gsu_code(0x04a0a1, 7);
    if (t) { gsu_code(0x04a0b8, 1); goto lA0B8; }
    gsu_code(0x04a0a8, 5);
    R[0] = gsu_ldw(FX_FACE_FLAGS);
    R[7] = 0x00FF;
    R[7] = gsu_and(R[0], R[7]);
    gsu_code(0x04a0ad, 6);
    R[0] = gsu_ldw(R[11]);
    R[0] = gsu_or(R[0], R[7]);
    gsu_code(0x04a0b3, 2);
    gsu_sbk(R[0]);
    gsu_code(0x04a0b5, 3); gsu_code(0x049e92, 1);
    goto l9E92;
lA0B8:
    R[11] = gsu_inc(R[11]);
    R[11] = gsu_inc(R[11]);
    gsu_code(0x04a0b9, 4);
    R[12] = gsu_ldw(R[11]);
    R[7] = R[12];
    gsu_moves(R[12]);
    t = !gsu.s;
    gsu_code(0x04a0bd, 4);
    if (t) { gsu_code(0x04a0c4, 1); goto lA0C4; }
    R[7] = gsu_not(R[7]);
    R[7] = gsu_inc(R[7]);
    gsu_code(0x04a0c1, 4);
lA0C4:
    R[12] = R[0];
    gsu_moves(R[0]);
    t = !gsu.s;
    gsu_code(0x04a0c5, 4);
    if (t) { gsu_code(0x04a0cc, 1); goto lA0CC; }
    R[12] = gsu_not(R[12]);
    R[12] = gsu_inc(R[12]);
    gsu_code(0x04a0c9, 4);
lA0CC:
    gsu_sub(R[7], R[12], 0);   /* CMP */
    t = !gsu.s;
    gsu_code(0x04a0cd, 5);
    if (t) { gsu_code(0x04a0d6, 1); goto lA0D6; }
    gsu_code(0x04a0d2, 4); gsu_code(0x049e92, 1);
    goto l9E92;
lA0D6:
    gsu_code(0x04a0d7, 1);
    gsu_stw(R[11], R[0]);
    R[11] = gsu_dec(R[11]);
    R[11] = gsu_dec(R[11]);
    gsu_code(0x04a0d8, 6);
    R[0] = gsu_ldw(FX_FACE_FLAGS);
    gsu_code(0x04a0de, 1);
    gsu_stw(R[11], R[0]);
    R[11] = gsu_add(R[11], 4, 0);
    gsu_code(0x04a0df, 5);
    gsu_stw(R[11], R[1]);
    R[11] = gsu_inc(R[11]);
    R[11] = gsu_inc(R[11]);
    gsu_code(0x04a0e4, 4);
    gsu_stw(R[11], R[2]);
    R[11] = gsu_inc(R[11]);
    R[11] = gsu_inc(R[11]);
    gsu_code(0x04a0e8, 4);
    gsu_stw(R[11], R[3]);
    gsu_code(0x04a0ec, 3); gsu_code(0x049e92, 1);
    goto l9E92;
lA0EF:
    gsu_code(0x04a0f0, 2);
    gsu_r14(gsu_add(R[14], R[13], 0));
    gsu_code(0x04a0f2, 3);
    gsu_r14(gsu_add(R[14], 5, 0));
    gsu_code(0x04a0f5, 3); gsu_code(0x049e92, 1);
    goto l9E92;
lA0F8:
    R[11] = 0x23A4;
    R[12] = 0x0013;
    R[13] = 0xA100;
    gsu_code(0x04a0f9, 8);
lA100:
    R[11] = gsu_add(R[11], 10, 0);
    gsu_code(0x04a101, 4);
    R[0] = gsu_ldw(R[11]);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.z;
    gsu_code(0x04a105, 4);
    if (t) { gsu_code(0x04a10e, 1); goto lA10E; }
    R[7] = gsu_hib(R[0]);
    t = gsu.z;
    gsu_code(0x04a109, 5);
    if (t) { gsu_code(0x04a115, 1); goto lA115; }
    gsu_code(0x04a10e, 1);
lA10E:
    t = gsu_count_down(&R[12]);
    gsu_code(0x04a10f, 1);
    if (t) { gsu_code(0x04a100, 1); goto lA100; }
    R[0] = 0x0000;
    t = 1;
    gsu_code(0x04a110, 5);
    gsu_code(0x04a117, 1);
    if (t) goto lA117;
lA115:
    R[0] = 0x0001;
    gsu_code(0x04a116, 2);
lA117:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x04a118, 4);
    R[9] = gsu_ldw(R[10]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x04a11c, 4);
    R[8] = gsu_ldw(R[10]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x04a120, 4);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
    /* FxCollideModelUnused */
lA124:
    t = 1;
    gsu_code(0x04a125, 2);
    gsu_code(0x04a0f8, 1);
    if (t) goto lA0F8;
lA127:
    R[11] = 0x23A4;
    R[0] = 0x001E;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_code(0x04a128, 7);
    R[0] = gsu_ldw(R[0]);
    R[2] = 0x0022;
    R[2] = gsu_add(R[2], R[9], 0);
    gsu_code(0x04a12f, 6);
    R[2] = gsu_ldw(R[2]);
    R[0] = gsu_add(R[0], R[2], 0);
    R[2] = 0x007F;
    R[0] = gsu_and(R[0], R[2]);
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
    R[2] = 0x0020;
    R[2] = gsu_add(R[2], R[9], 0);
    gsu_code(0x04a135, 12);
    R[2] = gsu_ldw(R[2]);
    R[0] = gsu_add(R[0], R[2], 0);
    gsu_code(0x04a141, 2);
    gsu_stw(R[11], R[0]);
    t = 1;
    gsu_code(0x04a143, 2);
    gsu_code(0x04a0f8, 1);
    if (t) goto lA0F8;
lA145:
    R[0] = 0x0000;
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x04a146, 6);
    R[9] = gsu_ldw(R[10]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x04a14c, 4);
    R[8] = gsu_ldw(R[10]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x04a150, 4);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
    /* FxPartsUpdate */
lA607:
    gsu_code(0x04a608, 2);
    gsu_stw(R[10], R[11]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[1] = 0x0000;
    R[0] = 0x0044;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04a60a, 9);
    gsu_stw(R[0], R[1]);
    R[0] = 0x0046;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04a613, 5);
    gsu_stw(R[0], R[1]);
    R[0] = 0x0056;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04a618, 5);
    gsu_stw(R[0], R[1]);
    R[0] = 0x003E;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04a61d, 4);
    R[0] = gsu_ldw(R[0]);
lA620:
    R[9] = R[0];
    gsu_moves(R[0]);
    t = gsu.z;
    gsu_code(0x04a621, 4);
    if (t) { gsu_code(0x04a651, 1); goto lA651; }
    R[0] = 0x0046;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_code(0x04a625, 5);
    R[0] = gsu_ldw(R[0]);
    R[1] = 0x0800;
    R[0] = gsu_and(R[0], R[1]);
    t = gsu.z;
    gsu_code(0x04a62a, 6);
    if (t) { gsu_code(0x04a638, 1); goto lA638; }
    R[11] = 0xA635;
    gsu_code(0x04a630, 5); gsu_code(0x04a656, 1);
    goto lA656;
lA635:
    t = 1;
    gsu_code(0x04a636, 2);
    gsu_code(0x04a63d, 1);
    if (t) goto lA63D;
lA638:
    R[11] = 0xA63D;
    gsu_code(0x04a639, 4); gsu_code(0x04a893, 1);
    goto lA893;
lA63D:
    R[1] = 0x002C;
    R[1] = gsu_add(R[1], R[9], 0);
    gsu_code(0x04a63e, 6);
    R[1] = gsu_ldw(R[1]);
    R[0] = 0x0092;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04a644, 5);
    R[0] = gsu_ldw(R[0]);
    R[0] = gsu_or(R[0], R[1]);
    gsu_code(0x04a649, 2);
    gsu_sbk(R[0]);
    R[0] = 0x0034;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_code(0x04a64b, 4);
    R[0] = gsu_ldw(R[0]);
    t = 1;
    gsu_code(0x04a64f, 2);
    gsu_code(0x04a620, 1);
    if (t) goto lA620;
lA651:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x04a652, 4);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
    /* FxPartPull */
lA656:
    R[0] = 0x0064;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04a657, 4);
    R[0] = gsu_ldw(R[0]);
    R[4] = 0x0001;
    R[0] = gsu_and(R[0], R[4]);
    t = gsu.z;
    gsu_code(0x04a65b, 6);
    if (t) { gsu_code(0x04a665, 1); goto lA665; }
    gsu_code(0x04a661, 4); gsu_code(0x04a7b4, 1);
    goto lA7B4;
lA665:
    gsu_code(0x04a666, 3);
    gsu_stw(R[10], R[11]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[1] = 0x0046;
    R[1] = gsu_add(R[1], R[9], 0);
    gsu_code(0x04a669, 8);
    R[1] = gsu_ldw(R[1]);
    R[0] = 0x0004;
    R[0] = gsu_and(R[0], R[1]);
    t = !gsu.z;
    gsu_code(0x04a671, 6);
    if (t) { gsu_code(0x04a681, 1); goto lA681; }
    R[0] = 0x0004;
    R[0] = gsu_or(R[0], R[1]);
    gsu_code(0x04a677, 6);
    gsu_sbk(R[0]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x04a67d, 4);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
lA681:
    R[2] = 0x0800;
    R[7] = 0x008E;
    R[7] = gsu_add(R[7], R[8], 0);
    gsu_code(0x04a682, 10);
    R[7] = gsu_ldw(R[7]);
    R[1] = 0x0028;
    R[1] = gsu_add(R[1], R[7], 0);
    gsu_code(0x04a68c, 6);
    R[1] = gsu_ldw(R[1]);
    R[0] = 0x0028;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04a692, 5);
    gsu_stw(R[0], R[1]);
    R[0] = 0x003E;
    R[0] = gsu_add(R[0], R[7], 0);
    gsu_code(0x04a697, 4);
    R[0] = gsu_ldw(R[0]);
lA69A:
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.z;
    gsu_code(0x04a69b, 4);
    if (t) { gsu_code(0x04a6b0, 1); goto lA6B0; }
    R[7] = R[0];
    R[0] = 0x0046;
    R[0] = gsu_add(R[0], R[7], 0);
    gsu_code(0x04a69f, 7);
    R[0] = gsu_ldw(R[0]);
    R[0] = gsu_and(R[0], R[2]);
    t = !gsu.z;
    gsu_code(0x04a6a6, 3);
    if (t) { gsu_code(0x04a6b0, 1); goto lA6B0; }
    R[0] = 0x0034;
    R[0] = gsu_add(R[0], R[7], 0);
    gsu_code(0x04a6a9, 5);
    R[0] = gsu_ldw(R[0]);
    t = 1;
    gsu_code(0x04a6ae, 2);
    gsu_code(0x04a69a, 1);
    if (t) goto lA69A;
lA6B0:
    R[1] = 0x001E;
    R[1] = gsu_add(R[1], R[7], 0);
    gsu_code(0x04a6b1, 6);
    R[1] = gsu_ldw(R[1]);
    R[2] = 0x0020;
    R[2] = gsu_add(R[2], R[7], 0);
    gsu_code(0x04a6b7, 6);
    R[2] = gsu_ldw(R[2]);
    R[3] = 0x0022;
    R[3] = gsu_add(R[3], R[7], 0);
    gsu_code(0x04a6bd, 6);
    R[3] = gsu_ldw(R[3]);
    R[0] = 0x001E;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_code(0x04a6c3, 4);
    R[0] = gsu_ldw(R[0]);
    R[1] = gsu_sub(R[1], R[0], 0);
    R[0] = 0x0020;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_code(0x04a6c7, 6);
    R[0] = gsu_ldw(R[0]);
    R[2] = gsu_sub(R[2], R[0], 0);
    R[0] = 0x0022;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_code(0x04a6cd, 6);
    R[0] = gsu_ldw(R[0]);
    R[3] = gsu_sub(R[3], R[0], 0);
    gsu_code(0x04a6d3, 6);
    gsu_stw(FX_RAM_TEST, R[3]);
    R[4] = 0x00C8;
    R[0] = R[1];
    gsu_moves(R[1]);
    t = !gsu.s;
    gsu_code(0x04a6d9, 7);
    if (t) { gsu_code(0x04a6e2, 1); goto lA6E2; }
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_code(0x04a6e0, 3);
lA6E2:
    gsu_sub(R[0], R[4], 0);   /* CMP */
    t = !gsu.s;
    gsu_code(0x04a6e3, 4);
    if (t) { gsu_code(0x04a6e9, 1); goto lA6E9; }
    R[1] = 0x0000;
    gsu_code(0x04a6e7, 3);
lA6E9:
    R[0] = R[2];
    gsu_moves(R[2]);
    t = !gsu.s;
    gsu_code(0x04a6ea, 4);
    if (t) { gsu_code(0x04a6f0, 1); goto lA6F0; }
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_code(0x04a6ee, 3);
lA6F0:
    gsu_sub(R[0], R[4], 0);   /* CMP */
    t = !gsu.s;
    gsu_code(0x04a6f1, 4);
    if (t) { gsu_code(0x04a6f7, 1); goto lA6F7; }
    R[2] = 0x0000;
    gsu_code(0x04a6f5, 3);
lA6F7:
    R[0] = R[3];
    gsu_moves(R[3]);
    t = !gsu.s;
    gsu_code(0x04a6f8, 4);
    if (t) { gsu_code(0x04a6fe, 1); goto lA6FE; }
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_code(0x04a6fc, 3);
lA6FE:
    gsu_sub(R[0], R[4], 0);   /* CMP */
    t = !gsu.s;
    gsu_code(0x04a6ff, 4);
    if (t) { gsu_code(0x04a705, 1); goto lA705; }
    R[3] = 0x0000;
    gsu_code(0x04a703, 3);
lA705:
    R[1] = gsu_add(R[1], R[1], 0);
    R[2] = gsu_add(R[2], R[2], 0);
    R[3] = gsu_add(R[3], R[3], 0);
    R[1] = gsu_add(R[1], R[1], 0);
    R[2] = gsu_add(R[2], R[2], 0);
    R[3] = gsu_add(R[3], R[3], 0);
    R[4] = 0x002E;
    R[4] = gsu_add(R[4], R[7], 0);
    gsu_code(0x04a706, 18);
    R[4] = gsu_ldw(R[4]);
    R[5] = 0x0030;
    R[5] = gsu_add(R[5], R[7], 0);
    gsu_code(0x04a718, 6);
    R[5] = gsu_ldw(R[5]);
    R[6] = 0x0032;
    R[6] = gsu_add(R[6], R[7], 0);
    gsu_code(0x04a71e, 6);
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
    gsu_code(0x04a724, 18);
    R[0] = gsu_ldw(R[0]);
    R[7] = gsu_xor(R[0], R[1]);
    t = !gsu.s;
    gsu_code(0x04a736, 5);
    if (t) { gsu_code(0x04a73d, 1); goto lA73D; }
    R[1] = gsu_sub(R[1], R[0], 0);
    gsu_code(0x04a73b, 3);
lA73D:
    R[0] = 0x0030;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_code(0x04a73e, 4);
    R[0] = gsu_ldw(R[0]);
    R[7] = gsu_xor(R[0], R[2]);
    t = !gsu.s;
    gsu_code(0x04a742, 5);
    if (t) { gsu_code(0x04a749, 1); goto lA749; }
    R[2] = gsu_sub(R[2], R[0], 0);
    gsu_code(0x04a747, 3);
lA749:
    R[0] = 0x0032;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_code(0x04a74a, 4);
    R[0] = gsu_ldw(R[0]);
    gsu_code(0x04a74e, 4);
    gsu_stw(FX_PART_Z_SPEED, R[0]);
    R[7] = gsu_xor(R[0], R[3]);
    t = !gsu.s;
    gsu_code(0x04a752, 5);
    if (t) { gsu_code(0x04a759, 1); goto lA759; }
    R[2] = gsu_sub(R[2], R[0], 0);
    gsu_code(0x04a757, 3);
lA759:
    R[1] = gsu_add(R[1], R[4], 0);
    R[2] = gsu_add(R[2], R[5], 0);
    R[3] = gsu_add(R[3], R[6], 0);
    R[4] = R[1];
    R[5] = R[2];
    R[6] = R[3];
    R[11] = 0xA76A;
    gsu_code(0x04a75a, 16); gsu_code(0x0496c3, 1);
    abi_FxSumsPushWorld();
lA76A:
    R[1] = 0x0036;
    R[1] = gsu_add(R[1], R[9], 0);
    gsu_code(0x04a76b, 6);
    R[1] = gsu_ldw(R[1]);
    R[2] = 0x0038;
    R[2] = gsu_add(R[2], R[9], 0);
    gsu_code(0x04a771, 6);
    R[2] = gsu_ldw(R[2]);
    R[3] = 0x003A;
    R[3] = gsu_add(R[3], R[9], 0);
    gsu_code(0x04a777, 6);
    R[3] = gsu_ldw(R[3]);
    R[1] = 0x0000;
    R[2] = 0x0000;
    R[3] = 0x0000;
    R[11] = 0xA787;
    gsu_code(0x04a77d, 10); gsu_code(0x0498ab, 1);
    abi_FxHitAddUnit();
    gsu_code(0x04a787, 1);
lA787:
    R[1] = 0x003C;
    R[1] = gsu_add(R[1], R[9], 0);
    gsu_code(0x04a788, 6);
    R[1] = gsu_ldw(R[1]);
    R[2] = 0x003E;
    R[2] = gsu_add(R[2], R[9], 0);
    gsu_code(0x04a78e, 6);
    R[2] = gsu_ldw(R[2]);
    R[3] = 0x0040;
    R[3] = gsu_add(R[3], R[9], 0);
    gsu_code(0x04a794, 6);
    R[3] = gsu_ldw(R[3]);
    R[0] = 0x0004;
    gsu_code(0x04a79a, 4);
    gsu_romb(R[0]);
    gsu_code(0x04a79e, 4);
    R[0] = gsu_ldw(FX_PARAM_PUSH);
    gsu_code(0x04a7a2, 3);
    gsu_r14(0x9BC6);
    gsu_code(0x04a7a5, 2);
    gsu_r14(gsu_add(R[14], R[0], 0));
    gsu_code(0x04a7a7, 1);
    R[0] = gsu_getb();
    gsu_code(0x04a7a8, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x04a7a9, 2);
    R[0] = gsu_getbh(R[0]);
    R[12] = R[0];
    R[11] = 0xA7AF;
    nx = (uint16_t)(R[12]);
    at = nx;
    gsu_code(0x04a7ab, 4);
    goto dispatch;
lA7AF:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x04a7b0, 4);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
lA7B4:
    gsu_code(0x04a7b5, 4);
    R[1] = gsu_ldw(FX_HIT);
    R[0] = 0x002E;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_code(0x04a7b9, 5);
    gsu_stw(R[0], R[1]);
    gsu_code(0x04a7be, 4);
    R[1] = gsu_ldw((FX_HIT + 2));
    R[0] = 0x0030;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_code(0x04a7c2, 5);
    gsu_stw(R[0], R[1]);
    gsu_code(0x04a7c7, 4);
    R[1] = gsu_ldw((FX_HIT + 4));
    R[0] = 0x0032;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_code(0x04a7cb, 5);
    gsu_stw(R[0], R[1]);
    gsu_code(0x04a7d0, 2);
    gsu_stw(R[10], R[11]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[1] = 0x0046;
    R[1] = gsu_add(R[1], R[9], 0);
    gsu_code(0x04a7d2, 8);
    R[1] = gsu_ldw(R[1]);
    R[0] = 0x0004;
    R[0] = gsu_and(R[0], R[1]);
    t = !gsu.z;
    gsu_code(0x04a7da, 6);
    if (t) { gsu_code(0x04a7ea, 1); goto lA7EA; }
    R[0] = 0x0004;
    R[0] = gsu_or(R[0], R[1]);
    gsu_code(0x04a7e0, 6);
    gsu_sbk(R[0]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x04a7e6, 4);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
lA7EA:
    R[2] = 0x0800;
    R[7] = 0x008E;
    R[7] = gsu_add(R[7], R[8], 0);
    gsu_code(0x04a7eb, 10);
    R[7] = gsu_ldw(R[7]);
    R[0] = 0x003E;
    R[0] = gsu_add(R[0], R[7], 0);
    gsu_code(0x04a7f5, 4);
    R[0] = gsu_ldw(R[0]);
lA7F8:
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.z;
    gsu_code(0x04a7f9, 4);
    if (t) { gsu_code(0x04a80e, 1); goto lA80E; }
    R[7] = R[0];
    R[0] = 0x0046;
    R[0] = gsu_add(R[0], R[7], 0);
    gsu_code(0x04a7fd, 7);
    R[0] = gsu_ldw(R[0]);
    R[0] = gsu_and(R[0], R[2]);
    t = !gsu.z;
    gsu_code(0x04a804, 3);
    if (t) { gsu_code(0x04a80e, 1); goto lA80E; }
    R[0] = 0x0034;
    R[0] = gsu_add(R[0], R[7], 0);
    gsu_code(0x04a807, 5);
    R[0] = gsu_ldw(R[0]);
    t = 1;
    gsu_code(0x04a80c, 2);
    gsu_code(0x04a7f8, 1);
    if (t) goto lA7F8;
lA80E:
    R[1] = 0x001E;
    R[1] = gsu_add(R[1], R[7], 0);
    gsu_code(0x04a80f, 6);
    R[1] = gsu_ldw(R[1]);
    R[2] = 0x0020;
    R[2] = gsu_add(R[2], R[7], 0);
    gsu_code(0x04a815, 6);
    R[2] = gsu_ldw(R[2]);
    R[3] = 0x0022;
    R[3] = gsu_add(R[3], R[7], 0);
    gsu_code(0x04a81b, 6);
    R[3] = gsu_ldw(R[3]);
    R[0] = 0x001E;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_code(0x04a821, 4);
    R[0] = gsu_ldw(R[0]);
    R[1] = gsu_sub(R[1], R[0], 0);
    R[0] = 0x0020;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_code(0x04a825, 6);
    R[0] = gsu_ldw(R[0]);
    R[2] = gsu_sub(R[2], R[0], 0);
    R[0] = 0x0022;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_code(0x04a82b, 6);
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
    gsu_code(0x04a831, 24); gsu_code(0x0496c3, 1);
    abi_FxSumsPushWorld();
lA849:
    R[1] = 0x0036;
    R[1] = gsu_add(R[1], R[9], 0);
    gsu_code(0x04a84a, 6);
    R[1] = gsu_ldw(R[1]);
    R[2] = 0x0038;
    R[2] = gsu_add(R[2], R[9], 0);
    gsu_code(0x04a850, 6);
    R[2] = gsu_ldw(R[2]);
    R[3] = 0x003A;
    R[3] = gsu_add(R[3], R[9], 0);
    gsu_code(0x04a856, 6);
    R[3] = gsu_ldw(R[3]);
    R[1] = 0x0000;
    R[2] = 0x0000;
    R[3] = 0x0000;
    R[11] = 0xA866;
    gsu_code(0x04a85c, 10); gsu_code(0x0498ab, 1);
    abi_FxHitAddUnit();
    gsu_code(0x04a866, 1);
lA866:
    R[1] = 0x003C;
    R[1] = gsu_add(R[1], R[9], 0);
    gsu_code(0x04a867, 6);
    R[1] = gsu_ldw(R[1]);
    R[2] = 0x003E;
    R[2] = gsu_add(R[2], R[9], 0);
    gsu_code(0x04a86d, 6);
    R[2] = gsu_ldw(R[2]);
    R[3] = 0x0040;
    R[3] = gsu_add(R[3], R[9], 0);
    gsu_code(0x04a873, 6);
    R[3] = gsu_ldw(R[3]);
    R[0] = 0x0004;
    gsu_code(0x04a879, 4);
    gsu_romb(R[0]);
    gsu_code(0x04a87d, 4);
    R[0] = gsu_ldw(FX_PARAM_PUSH);
    gsu_code(0x04a881, 3);
    gsu_r14(0x9BC6);
    gsu_code(0x04a884, 2);
    gsu_r14(gsu_add(R[14], R[0], 0));
    gsu_code(0x04a886, 1);
    R[0] = gsu_getb();
    gsu_code(0x04a887, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x04a888, 2);
    R[0] = gsu_getbh(R[0]);
    R[12] = R[0];
    R[11] = 0xA88E;
    nx = (uint16_t)(R[12]);
    at = nx;
    gsu_code(0x04a88a, 4);
    goto dispatch;
lA88E:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x04a88f, 4);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
    /* FxWheelUpdate */
lA893:
    gsu_code(0x04a894, 2);
    gsu_stw(R[10], R[11]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[0] = 0x0046;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_code(0x04a896, 6);
    R[0] = gsu_ldw(R[0]);
    gsu_code(0x04a89c, 4);
    gsu_stw(FX_WHEEL_FLAGS, R[0]);
    R[1] = 0x0000;
    R[0] = 0x002C;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_code(0x04a8a0, 7);
    gsu_stw(R[0], R[1]);
    R[3] = 0x0400;
    R[2] = 0x0040;
    R[2] = gsu_add(R[2], R[8], 0);
    gsu_code(0x04a8a7, 9);
    R[2] = gsu_ldw(R[2]);
    gsu_code(0x04a8b0, 4);
    R[1] = gsu_ldw(FX_WHEEL_FLAGS);
    R[1] = gsu_bic(R[1], R[3]);
    R[0] = 0x0200;
    R[0] = gsu_and(R[0], R[1]);
    t = gsu.z;
    gsu_code(0x04a8b4, 9);
    if (t) { gsu_code(0x04a8cc, 1); goto lA8CC; }
    R[0] = 0x0000;
    R[0] = gsu_and(R[0], R[2]);
    t = !gsu.z;
    gsu_code(0x04a8bd, 7);
    if (t) { gsu_code(0x04a8cc, 1); goto lA8CC; }
    R[1] = gsu_or(R[1], R[3]);
    gsu_code(0x04a8c4, 5);
    gsu_sbk(R[1]);
    gsu_code(0x04a8c9, 3); gsu_code(0x04aed7, 1);
    goto lAED7;
lA8CC:
    gsu_code(0x04a8cd, 2);
    gsu_sbk(R[1]);
    R[1] = 0x0024;
    R[1] = gsu_add(R[1], R[8], 0);
    gsu_code(0x04a8cf, 6);
    R[1] = gsu_ldw(R[1]);
    R[0] = 0x0024;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_code(0x04a8d5, 5);
    gsu_stw(R[0], R[1]);
    R[1] = 0x0028;
    R[1] = gsu_add(R[1], R[8], 0);
    gsu_code(0x04a8da, 6);
    R[1] = gsu_ldw(R[1]);
    R[0] = 0x0028;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_code(0x04a8e0, 5);
    gsu_stw(R[0], R[1]);
    gsu_code(0x04a8e5, 4);
    R[2] = gsu_ldw(FX_WHEEL_FLAGS);
    R[0] = 0x0002;
    R[0] = gsu_and(R[0], R[2]);
    t = gsu.z;
    gsu_code(0x04a8e9, 6);
    if (t) { gsu_code(0x04a8fb, 1); goto lA8FB; }
    R[0] = 0x003A;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04a8ef, 5);
    R[0] = gsu_ldw(R[0]);
    R[1] = 0x0024;
    R[1] = gsu_add(R[1], R[9], 0);
    gsu_code(0x04a8f4, 6);
    R[1] = gsu_ldw(R[1]);
    R[0] = gsu_add(R[0], R[1], 0);
    gsu_code(0x04a8fa, 2);
    gsu_sbk(R[0]);
lA8FB:
    gsu_code(0x04a8fc, 4);
    R[0] = gsu_ldw(FX_WHEEL_FLAGS);
    R[1] = 0x0080;
    R[0] = gsu_and(R[0], R[1]);
    t = gsu.z;
    gsu_code(0x04a900, 6);
    if (t) { gsu_code(0x04a90a, 1); goto lA90A; }
    gsu_code(0x04a906, 4); gsu_code(0x04aeb9, 1);
    goto lAEB9;
lA90A:
    R[11] = 0xA90F;
    gsu_code(0x04a90b, 4); gsu_code(0x0496a7, 1);
    abi_FxSumsClear();
    gsu_code(0x04a90f, 1);
lA90F:
    R[0] = 0x0000;
    R[1] = 0x7D00;
    gsu_code(0x04a910, 10);
    gsu_stw(FX_PUSHES, R[0]);
    gsu_code(0x04a91a, 4);
    gsu_stw((FX_PUSHES + 2), R[1]);
    gsu_code(0x04a91e, 4);
    gsu_stw((FX_PUSHES + 10), R[0]);
    gsu_code(0x04a922, 4);
    gsu_stw((FX_PUSHES + 12), R[1]);
    gsu_code(0x04a926, 4);
    gsu_stw((FX_PUSHES + 20), R[0]);
    gsu_code(0x04a92a, 4);
    gsu_stw((FX_PUSHES + 22), R[1]);
    gsu_code(0x04a92e, 4);
    gsu_stw((FX_PUSHES + 30), R[0]);
    gsu_code(0x04a932, 4);
    gsu_stw((FX_PUSHES + 32), R[1]);
    gsu_code(0x04a936, 4);
    gsu_stw((FX_PUSHES + 40), R[0]);
    gsu_code(0x04a93a, 4);
    gsu_stw((FX_PUSHES + 42), R[1]);
    gsu_code(0x04a93e, 4);
    gsu_stw((FX_PUSHES + 50), R[0]);
    gsu_code(0x04a942, 4);
    gsu_stw((FX_PUSHES + 52), R[1]);
    gsu_code(0x04a946, 4);
    gsu_stw((FX_PUSHES + 60), R[0]);
    gsu_code(0x04a94a, 4);
    gsu_stw((FX_PUSHES + 62), R[1]);
    gsu_code(0x04a94e, 4);
    gsu_stw((FX_PUSHES + 70), R[0]);
    gsu_code(0x04a952, 4);
    gsu_stw((FX_PUSHES + 72), R[1]);
    gsu_code(0x04a956, 4);
    gsu_stw((FX_PUSHES + 80), R[0]);
    gsu_code(0x04a95a, 4);
    gsu_stw((FX_PUSHES + 82), R[1]);
    gsu_code(0x04a95e, 4);
    gsu_stw((FX_PUSHES + 90), R[0]);
    gsu_code(0x04a962, 4);
    gsu_stw((FX_PUSHES + 92), R[1]);
    gsu_code(0x04a966, 4);
    gsu_stw((FX_PUSHES + 100), R[0]);
    gsu_code(0x04a96a, 4);
    gsu_stw((FX_PUSHES + 102), R[1]);
    gsu_code(0x04a96e, 4);
    gsu_stw((FX_PUSHES + 110), R[0]);
    gsu_code(0x04a972, 4);
    gsu_stw((FX_PUSHES + 112), R[1]);
    gsu_code(0x04a976, 4);
    gsu_stw((FX_PUSHES + 120), R[0]);
    gsu_code(0x04a97a, 4);
    gsu_stw((FX_PUSHES + 122), R[1]);
    gsu_code(0x04a97e, 4);
    gsu_stw((FX_PUSHES + 130), R[0]);
    gsu_code(0x04a982, 4);
    gsu_stw((FX_PUSHES + 132), R[1]);
    gsu_code(0x04a986, 4);
    gsu_stw((FX_PUSHES + 140), R[0]);
    gsu_code(0x04a98a, 4);
    gsu_stw((FX_PUSHES + 142), R[1]);
    gsu_code(0x04a98e, 4);
    gsu_stw((FX_PUSHES + 150), R[0]);
    gsu_code(0x04a992, 4);
    gsu_stw((FX_PUSHES + 152), R[1]);
    gsu_code(0x04a996, 4);
    gsu_stw((FX_PUSHES + 160), R[0]);
    gsu_code(0x04a99a, 4);
    gsu_stw((FX_PUSHES + 162), R[1]);
    gsu_code(0x04a99e, 4);
    gsu_stw((FX_PUSHES + 170), R[0]);
    gsu_code(0x04a9a2, 4);
    gsu_stw((FX_PUSHES + 172), R[1]);
    gsu_code(0x04a9a6, 4);
    gsu_stw((FX_PUSHES + 180), R[0]);
    gsu_code(0x04a9aa, 4);
    gsu_stw((FX_PUSHES + 182), R[1]);
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_code(0x04a9ae, 5);
    gsu_stw(FX_CONTACT_SUM, R[0]);
    gsu_code(0x04a9b3, 4);
    gsu_stw((FX_CONTACT_SUM + 2), R[0]);
    gsu_code(0x04a9b7, 4);
    gsu_stw((FX_CONTACT_SUM + 4), R[0]);
    R[0] = 0x7D00;
    gsu_code(0x04a9bb, 7);
    gsu_stw(FX_CONTACT_DEPTH, R[0]);
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_code(0x04a9c2, 5);
    gsu_stw(FX_WHEEL_CONTACT, R[0]);
    gsu_code(0x04a9c7, 4);
    R[0] = gsu_ldw(FX_WHEEL_FLAGS);
    R[1] = 0x0030;
    R[0] = gsu_bic(R[0], R[1]);
    gsu_code(0x04a9cb, 6);
    gsu_sbk(R[0]);
    R[0] = 0x0018;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_code(0x04a9d1, 4);
    R[0] = gsu_ldw(R[0]);
    R[1] = R[0];
    gsu_moves(R[0]);
    t = !gsu.z;
    gsu_code(0x04a9d5, 4);
    if (t) { gsu_code(0x04a9dd, 1); goto lA9DD; }
    gsu_code(0x04a9d9, 4); gsu_code(0x04aa98, 1);
    goto lAA98;
lA9DD:
    R[7] = 0x000C;
    R[7] = gsu_add(R[7], R[1], 0);
    gsu_code(0x04a9de, 6);
    R[7] = gsu_ldw(R[7]);
    gsu_code(0x04a9e4, 4);
    gsu_stw(FX_COLLIDE_MODEL, R[7]);
    R[11] = 0xA9EC;
    gsu_code(0x04a9e8, 4); gsu_code(0x049c3d, 1);
    goto l9C3D;
lA9EC:
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.z;
    gsu_code(0x04a9ed, 4);
    if (t) { gsu_code(0x04aa0d, 1); goto lAA0D; }
    R[11] = 0xA9F6;
    gsu_code(0x04a9f1, 5); gsu_code(0x04aeff, 1);
    goto lAEFF;
lA9F6:
    R[1] = R[0];
    gsu_moves(R[0]);
    t = gsu.z;
    gsu_code(0x04a9f7, 4);
    if (t) { gsu_code(0x04aa0d, 1); goto lAA0D; }
    R[0] = 0x001F;
    R[0] = gsu_and(R[0], R[1]);
    t = !gsu.z;
    gsu_code(0x04a9fb, 7);
    if (t) { gsu_code(0x04aa06, 1); goto lAA06; }
    gsu_code(0x04aa02, 1);
lAA02:
    gsu_code(0x04aa03, 3); gsu_code(0x04aa98, 1);
    goto lAA98;
lAA06:
    R[0] = 0x0020;
    R[0] = gsu_and(R[0], R[1]);
    t = gsu.z;
    gsu_code(0x04aa07, 6);
    if (t) { gsu_code(0x04aa5a, 1); goto lAA5A; }
    gsu_code(0x04aa0d, 1);
lAA0D:
    gsu_code(0x04aa0e, 4);
    R[2] = gsu_ldw(FX_COLLIDE_MODEL);
    R[1] = 0x0016;
    R[1] = gsu_add(R[1], R[2], 0);
    gsu_code(0x04aa12, 6);
    R[1] = gsu_ldw(R[1]);
    R[0] = 0x0040;
    R[0] = gsu_and(R[0], R[1]);
    t = !gsu.z;
    gsu_code(0x04aa18, 6);
    if (t) { gsu_code(0x04aa5a, 1); goto lAA5A; }
    R[0] = 0x3800;
    R[1] = gsu_and(R[0], R[1]);
    t = gsu.z;
    gsu_code(0x04aa1e, 8);
    if (t) { gsu_code(0x04aa8e, 1); goto lAA8E; }
    R[2] = 0x00A0;
    R[0] = 0x0800;
    gsu_sub(R[0], R[1], 0);   /* CMP */
    t = gsu.z;
    gsu_code(0x04aa26, 11);
    if (t) { gsu_code(0x04aa52, 1); goto lAA52; }
    R[2] = 0x2020;
    R[0] = 0x2000;
    gsu_sub(R[0], R[1], 0);   /* CMP */
    t = gsu.z;
    gsu_code(0x04aa31, 11);
    if (t) { gsu_code(0x04aa52, 1); goto lAA52; }
    R[2] = 0x1020;
    R[0] = 0x1000;
    gsu_sub(R[0], R[1], 0);   /* CMP */
    t = gsu.z;
    gsu_code(0x04aa3c, 11);
    if (t) { gsu_code(0x04aa52, 1); goto lAA52; }
    R[2] = 0x0820;
    R[0] = 0x1800;
    gsu_sub(R[0], R[1], 0);   /* CMP */
    t = !gsu.z;
    gsu_code(0x04aa47, 11);
    if (t) { gsu_code(0x04aa5a, 1); goto lAA5A; }
    gsu_code(0x04aa52, 1);
lAA52:
    R[0] = 0x002C;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_code(0x04aa53, 4);
    R[0] = gsu_ldw(R[0]);
    R[0] = gsu_or(R[0], R[2]);
    t = 1;
    gsu_code(0x04aa57, 3); gsu_code(0x04aa82, 1);
    gsu_sbk(R[0]);
    if (t) goto lAA82;
lAA5A:
    gsu_code(0x04aa5b, 4);
    R[2] = gsu_ldw(FX_COLLIDE_MODEL);
    R[1] = 0x0016;
    R[1] = gsu_add(R[1], R[2], 0);
    gsu_code(0x04aa5f, 6);
    R[1] = gsu_ldw(R[1]);
    R[0] = 0x4000;
    R[0] = gsu_and(R[0], R[1]);
    t = !gsu.z;
    gsu_code(0x04aa65, 6);
    if (t) { gsu_code(0x04aa82, 1); goto lAA82; }
    R[1] = 0x008A;
    R[1] = gsu_add(R[1], R[8], 0);
    gsu_code(0x04aa6b, 8);
    R[1] = gsu_ldw(R[1]);
    R[0] = 0x0040;
    R[0] = gsu_and(R[0], R[1]);
    t = gsu.z;
    gsu_code(0x04aa73, 6);
    if (t) { gsu_code(0x04aa82, 1); goto lAA82; }
    R[2] = 0x1000;
    R[0] = 0x002C;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_code(0x04aa79, 8);
    R[0] = gsu_ldw(R[0]);
    R[0] = gsu_or(R[0], R[2]);
    gsu_code(0x04aa81, 2);
    gsu_sbk(R[0]);
lAA82:
    R[0] = 0x002C;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04aa83, 4);
    R[0] = gsu_ldw(R[0]);
    R[2] = 0x0040;
    R[0] = gsu_bic(R[0], R[2]);
    t = 1;
    gsu_code(0x04aa87, 7); gsu_code(0x04aad0, 1);
    gsu_sbk(R[0]);
    if (t) goto lAAD0;
lAA8E:
    R[0] = 0x002C;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04aa8f, 4);
    R[0] = gsu_ldw(R[0]);
    R[2] = 0x0040;
    R[0] = gsu_bic(R[0], R[2]);
    gsu_code(0x04aa93, 6);
    gsu_sbk(R[0]);
lAA98:
    R[1] = 0x008A;
    R[1] = gsu_add(R[1], R[8], 0);
    gsu_code(0x04aa99, 7);
    R[1] = gsu_ldw(R[1]);
    R[2] = 0x0040;
    R[0] = 0x0004;
    R[0] = gsu_and(R[0], R[1]);
    t = !gsu.z;
    gsu_code(0x04aaa0, 9);
    if (t) { gsu_code(0x04aaca, 1); goto lAACA; }
    R[2] = 0x00C0;
    R[0] = 0x0020;
    R[0] = gsu_and(R[0], R[1]);
    t = !gsu.z;
    gsu_code(0x04aaa9, 10);
    if (t) { gsu_code(0x04aaca, 1); goto lAACA; }
    R[2] = 0x1040;
    R[0] = 0x0040;
    R[0] = gsu_and(R[0], R[1]);
    t = !gsu.z;
    gsu_code(0x04aab3, 10);
    if (t) { gsu_code(0x04aaca, 1); goto lAACA; }
    R[2] = 0x0840;
    R[0] = 0x0002;
    R[0] = gsu_and(R[0], R[1]);
    t = !gsu.z;
    gsu_code(0x04aabd, 10);
    if (t) { gsu_code(0x04aaca, 1); goto lAACA; }
    R[2] = 0x0060;
    gsu_code(0x04aac7, 4);
lAACA:
    R[0] = 0x002C;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_code(0x04aacb, 4);
    R[0] = gsu_ldw(R[0]);
    R[0] = gsu_or(R[0], R[2]);
    gsu_code(0x04aacf, 2);
    gsu_sbk(R[0]);
lAAD0:
    R[2] = 0x0020;
    R[2] = gsu_add(R[2], R[9], 0);
    gsu_code(0x04aad1, 6);
    R[2] = gsu_ldw(R[2]);
    R[2] = R[2];
    gsu_moves(R[2]);
    t = gsu.s;
    gsu_code(0x04aad7, 4);
    if (t) { gsu_code(0x04ab15, 1); goto lAB15; }
    R[0] = 0x002C;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_code(0x04aadb, 5);
    R[0] = gsu_ldw(R[0]);
    R[1] = 0x0020;
    R[0] = gsu_and(R[0], R[1]);
    t = gsu.z;
    gsu_code(0x04aae0, 6);
    if (t) { gsu_code(0x04ab01, 1); goto lAB01; }
    R[0] = 0x000A;
    R[2] = gsu_add(R[2], R[0], 0);
    R[0] = 0x001E;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_code(0x04aae6, 10);
    R[0] = gsu_ldw(R[0]);
    R[1] = 0x0022;
    R[1] = gsu_add(R[1], R[9], 0);
    gsu_code(0x04aaf0, 6);
    R[1] = gsu_ldw(R[1]);
    R[0] = gsu_add(R[0], R[1], 0);
    R[0] = gsu_div2(R[0]);
    R[0] = gsu_div2(R[0]);
    R[0] = gsu_div2(R[0]);
    R[1] = 0x000F;
    R[0] = gsu_and(R[0], R[1]);
    R[2] = gsu_add(R[2], R[0], 0);
    gsu_code(0x04aaf6, 12);
lAB01:
    R[11] = 0xAB06;
    gsu_code(0x04ab02, 4); gsu_code(0x049683, 1);
    abi_FxPushSet();
lAB06:
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.z;
    gsu_code(0x04ab07, 4);
    if (t) { gsu_code(0x04ab15, 1); goto lAB15; }
    R[3] = 0x23A4;
    R[2] = 0x0000;
    R[11] = 0xAB15;
    gsu_code(0x04ab0b, 10); gsu_code(0x04af54, 1);
    goto lAF54;
lAB15:
    gsu_code(0x04ab16, 4);
    R[0] = gsu_ldw(FX_CONTACT_FORCE_UP);
    gsu_code(0x04ab1a, 4);
    gsu_stw(FX_CONTACT_FORCE, R[0]);
    R[1] = 0x002C;
    R[1] = gsu_add(R[1], R[9], 0);
    gsu_code(0x04ab1e, 6);
    R[1] = gsu_ldw(R[1]);
    R[0] = 0x0020;
    R[0] = gsu_and(R[0], R[1]);
    t = gsu.z;
    gsu_code(0x04ab24, 6);
    if (t) { gsu_code(0x04ab4b, 1); goto lAB4B; }
    gsu_code(0x04ab2a, 5);
    R[0] = gsu_ldw(FX_CONTACT_FORCE_SIDE);
    gsu_code(0x04ab2f, 4);
    gsu_stw(FX_CONTACT_FORCE, R[0]);
    R[0] = 0x0046;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04ab33, 4);
    R[0] = gsu_ldw(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_code(0x04ab37, 2);
    gsu_sbk(R[0]);
    gsu_code(0x04ab39, 4);
    R[0] = gsu_ldw((FX_WORLD_SUM + 2));
    R[0] = R[0];
    gsu_moves(R[0]);
    t = !gsu.z;
    gsu_code(0x04ab3d, 4);
    if (t) { gsu_code(0x04ab4b, 1); goto lAB4B; }
    R[0] = 0x002C;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_code(0x04ab41, 5);
    R[0] = gsu_ldw(R[0]);
    R[1] = 0x0020;
    R[0] = gsu_bic(R[0], R[1]);
    gsu_code(0x04ab46, 6);
    gsu_sbk(R[0]);
lAB4B:
    gsu_code(0x04ab4c, 4);
    R[2] = gsu_ldw(FX_WHEEL_FLAGS);
    gsu_code(0x04ab50, 4);
    R[0] = gsu_ldw(FX_CONTACT_DEPTH);
    R[1] = 0x7D00;
    gsu_sub(R[0], R[1], 0);   /* CMP */
    t = !gsu.z;
    gsu_code(0x04ab54, 7);
    if (t) { gsu_code(0x04ab5f, 1); goto lAB5F; }
    gsu_code(0x04ab5b, 4); gsu_code(0x04aee5, 1);
    goto lAEE5;
lAB5F:
    R[0] = 0x0040;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04ab60, 4);
    R[0] = gsu_ldw(R[0]);
    R[1] = 0x0100;
    R[0] = gsu_bic(R[0], R[1]);
    gsu_code(0x04ab64, 6);
    gsu_sbk(R[0]);
    R[0] = 0x0010;
    R[1] = gsu_or(R[0], R[2]);
    gsu_code(0x04ab6a, 9);
    gsu_stw(FX_WHEEL_FLAGS, R[1]);
    gsu_code(0x04ab73, 4);
    R[2] = gsu_ldw(FX_WHEEL_FLAGS);
    R[0] = 0x0008;
    R[0] = gsu_and(R[0], R[2]);
    t = gsu.z;
    gsu_code(0x04ab77, 6);
    if (t) { gsu_code(0x04abe5, 1); goto lABE5; }
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_code(0x04ab7d, 6);
    gsu_stw(FX_WHEEL_LOAD, R[0]);
    R[0] = 0x0050;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_code(0x04ab83, 4);
    R[0] = gsu_ldw(R[0]);
    R[0] = gsu_add(R[0], R[0], 0);
    gsu_code(0x04ab87, 5);
    R[6] = gsu_ldw(FX_CONTACT_FORCE);
    gsu_code(0x04ab8c, 1);
    R[0] = gsu_fmult(R[0], R[6]);
    R[6] = 0x003C;
    R[6] = gsu_add(R[6], R[8], 0);
    gsu_code(0x04ab8d, 6);
    R[6] = gsu_ldw(R[6]);
    R[1] = 0x004E;
    R[1] = gsu_add(R[1], R[8], 0);
    gsu_code(0x04ab93, 6);
    R[1] = gsu_ldw(R[1]);
    R[1] = gsu_sub(R[1], 2, 0);
    t = !gsu.s;
    gsu_code(0x04ab99, 5);
    if (t) { gsu_code(0x04abc8, 1); goto lABC8; }
    R[6] = R[6];
    gsu_moves(R[6]);
    t = gsu.z;
    gsu_code(0x04ab9e, 5);
    if (t) { gsu_code(0x04abe5, 1); goto lABE5; }
    t = gsu.s;
    gsu_code(0x04aba3, 3);
    if (t) { gsu_code(0x04abc8, 1); goto lABC8; }
    gsu_sub(R[0], R[6], 0);   /* CMP */
    t = !gsu.s;
    gsu_code(0x04aba6, 5);
    if (t) { gsu_code(0x04abc8, 1); goto lABC8; }
    R[6] = R[0];
    R[0] = 0x0020;
    R[1] = gsu_or(R[0], R[2]);
    gsu_code(0x04abab, 12);
    gsu_stw(FX_WHEEL_FLAGS, R[1]);
    R[1] = 0x0056;
    R[1] = gsu_add(R[1], R[8], 0);
    gsu_code(0x04abb7, 6);
    R[1] = gsu_ldw(R[1]);
    R[0] = 0x1F40;
    R[0] = gsu_add(R[0], R[1], 0);
    gsu_code(0x04abbd, 5);
    gsu_sbk(R[0]);
    gsu_code(0x04abc2, 4);
    R[0] = gsu_ldw(FX_CONTACT_FORCE);
    R[0] = gsu_div2(R[0]);
    gsu_code(0x04abc6, 3);
    gsu_sbk(R[0]);
lABC8:
    R[6] = gsu_add(R[6], R[6], 0);
    gsu_code(0x04abc9, 6);
    gsu_stw(FX_WHEEL_LOAD, R[6]);
    gsu_code(0x04abcf, 3);
    R[0] = gsu_ldw(FX_OBJECT_MATRIX_6);
    gsu_code(0x04abd2, 3);
    R[1] = gsu_ldw(FX_OBJECT_MATRIX_7);
    gsu_code(0x04abd5, 3);
    R[2] = gsu_ldw(FX_OBJECT_MATRIX_8);
    gsu_code(0x04abd8, 1);
    R[0] = gsu_fmult(R[0], R[6]);
    R[4] = R[0];
    gsu_code(0x04abd9, 5);
    R[5] = gsu_fmult(R[1], R[6]);
    gsu_code(0x04abde, 3);
    R[6] = gsu_fmult(R[2], R[6]);
    R[11] = 0xABE5;
    gsu_code(0x04abe1, 4); gsu_code(0x0496c3, 1);
    abi_FxSumsPushWorld();
lABE5:
    R[0] = 0x0000;
    gsu_code(0x04abe6, 4);
    gsu_romb(R[0]);
    R[0] = 0x0024;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_code(0x04abea, 4);
    R[0] = gsu_ldw(R[0]);
    R[1] = 0x0024;
    R[1] = gsu_add(R[1], R[8], 0);
    gsu_code(0x04abee, 6);
    R[1] = gsu_ldw(R[1]);
    R[0] = gsu_sub(R[0], R[1], 0);
    R[0] = gsu_hib(R[0]);
    R[1] = R[0];
    R[2] = 0xF060;
    R[3] = 0x0FA0;
    R[0] = gsu_lob(R[0]);
    gsu_code(0x04abf4, 14);
    gsu_r14(0x96FE);
    gsu_code(0x04ac02, 2);
    gsu_r14(gsu_add(R[0], R[14], 0));
    gsu_code(0x04ac04, 1);
    R[0] = gsu_getb();
    R[0] = gsu_swap(R[0]);
    R[0] = gsu_div2(R[0]);
    R[0] = gsu_div2(R[0]);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.s;
    gsu_code(0x04ac05, 9);
    if (t) { gsu_code(0x04ac18, 1); goto lAC18; }
    gsu_sub(R[0], R[3], 0);   /* CMP */
    t = gsu.s;
    gsu_code(0x04ac0e, 5);
    if (t) { gsu_code(0x04ac1f, 1); goto lAC1F; }
    R[0] = R[3];
    t = 1;
    gsu_code(0x04ac13, 5);
    gsu_code(0x04ac1f, 1);
    if (t) goto lAC1F;
lAC18:
    gsu_sub(R[0], R[2], 0);   /* CMP */
    t = !gsu.s;
    gsu_code(0x04ac19, 4);
    if (t) { gsu_code(0x04ac1f, 1); goto lAC1F; }
    R[0] = R[2];
    gsu_code(0x04ac1d, 3);
lAC1F:
    gsu_code(0x04ac20, 4);
    gsu_stw(FX_VIEW_MATRIX_2, R[0]);
    R[0] = R[1];
    R[0] = gsu_lob(R[0]);
    gsu_code(0x04ac24, 6);
    gsu_r14(0x973E);
    gsu_code(0x04ac2a, 2);
    gsu_r14(gsu_add(R[0], R[14], 0));
    gsu_code(0x04ac2c, 1);
    R[0] = gsu_getb();
    R[0] = gsu_swap(R[0]);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = !gsu.s;
    gsu_code(0x04ac2d, 5);
    if (t) { gsu_code(0x04ac34, 1); goto lAC34; }
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_code(0x04ac32, 3);
lAC34:
    gsu_code(0x04ac35, 4);
    gsu_stw(FX_VIEW_MATRIX_8, R[0]);
    R[1] = 0x002E;
    R[1] = gsu_add(R[1], R[9], 0);
    gsu_code(0x04ac39, 6);
    R[1] = gsu_ldw(R[1]);
    R[1] = gsu_not(R[1]);
    R[1] = gsu_inc(R[1]);
    R[2] = 0x0030;
    R[2] = gsu_add(R[2], R[9], 0);
    gsu_code(0x04ac3f, 9);
    R[2] = gsu_ldw(R[2]);
    R[2] = gsu_not(R[2]);
    R[2] = gsu_inc(R[2]);
    R[3] = 0x0032;
    R[3] = gsu_add(R[3], R[9], 0);
    gsu_code(0x04ac48, 9);
    R[3] = gsu_ldw(R[3]);
    R[3] = gsu_not(R[3]);
    R[3] = gsu_inc(R[3]);
    gsu_code(0x04ac51, 6);
    R[6] = gsu_ldw(GSU_OUT_E4);
    gsu_code(0x04ac57, 2);
    R[0] = gsu_fmult(R[1], R[6]);
    R[5] = gsu_rol(R[0]);
    gsu_code(0x04ac59, 5);
    R[6] = gsu_ldw(FX_OBJECT_MATRIX_1);
    gsu_code(0x04ac5e, 2);
    R[0] = gsu_fmult(R[2], R[6]);
    R[0] = gsu_rol(R[0]);
    R[5] = gsu_add(R[5], R[0], 0);
    gsu_code(0x04ac60, 6);
    R[6] = gsu_ldw(FX_OBJECT_MATRIX_2);
    gsu_code(0x04ac66, 2);
    R[0] = gsu_fmult(R[3], R[6]);
    R[0] = gsu_rol(R[0]);
    R[0] = gsu_add(R[0], R[5], 0);
    gsu_code(0x04ac68, 6);
    gsu_stw(FX_WHEEL_SIDE, R[0]);
    gsu_code(0x04ac6e, 3);
    R[6] = gsu_ldw(FX_OBJECT_MATRIX_6);
    gsu_code(0x04ac71, 2);
    R[0] = gsu_fmult(R[1], R[6]);
    R[5] = gsu_rol(R[0]);
    gsu_code(0x04ac73, 5);
    R[6] = gsu_ldw(FX_OBJECT_MATRIX_7);
    gsu_code(0x04ac78, 2);
    R[0] = gsu_fmult(R[2], R[6]);
    R[0] = gsu_rol(R[0]);
    R[5] = gsu_add(R[5], R[0], 0);
    gsu_code(0x04ac7a, 6);
    R[6] = gsu_ldw(FX_OBJECT_MATRIX_8);
    gsu_code(0x04ac80, 2);
    R[0] = gsu_fmult(R[3], R[6]);
    R[0] = gsu_rol(R[0]);
    R[0] = gsu_add(R[0], R[5], 0);
    gsu_code(0x04ac82, 6);
    gsu_stw(FX_WHEEL_AHEAD, R[0]);
    R[1] = R[0];
    R[0] = 0x0026;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_code(0x04ac88, 6);
    R[0] = gsu_ldw(R[0]);
    R[1] = gsu_add(R[0], R[1], 0);
    R[0] = 0x0026;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_code(0x04ac8e, 7);
    gsu_stw(R[0], R[1]);
    gsu_code(0x04ac95, 4);
    R[1] = gsu_ldw(FX_WHEEL_SIDE);
    gsu_code(0x04ac99, 4);
    R[3] = gsu_ldw(FX_WHEEL_AHEAD);
    gsu_code(0x04ac9d, 3);
    R[6] = gsu_ldw(FX_VIEW_MATRIX_8);
    gsu_code(0x04aca0, 2);
    R[1] = gsu_fmult(R[1], R[6]);
    gsu_code(0x04aca2, 3);
    R[6] = gsu_ldw(FX_VIEW_MATRIX_2);
    R[0] = gsu_add(R[3], R[3], 0);
    gsu_code(0x04aca5, 3);
    R[0] = gsu_fmult(R[0], R[6]);
    R[0] = gsu_add(R[0], R[1], 0);
    R[0] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_add(R[0], R[0], 0);
    R[4] = R[0];
    R[1] = R[0];
    R[5] = 0x0050;
    R[5] = gsu_add(R[5], R[9], 0);
    gsu_code(0x04aca8, 13);
    R[5] = gsu_ldw(R[5]);
    R[0] = 0x003C;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04acb5, 4);
    R[0] = gsu_ldw(R[0]);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = !gsu.z;
    gsu_code(0x04acb9, 4);
    if (t) { gsu_code(0x04acc6, 1); goto lACC6; }
    gsu_code(0x04acbd, 5);
    R[6] = gsu_ldw(FX_PARAM_GRIP_FREE);
    R[0] = gsu_add(R[5], R[5], 0);
    gsu_code(0x04acc2, 3);
    R[0] = gsu_fmult(R[0], R[6]);
    R[5] = gsu_add(R[5], R[0], 0);
    gsu_code(0x04acc5, 2);
lACC6:
    R[1] = 0x003A;
    R[1] = gsu_add(R[1], R[8], 0);
    gsu_code(0x04acc7, 6);
    R[1] = gsu_ldw(R[1]);
    R[1] = R[1];
    gsu_moves(R[1]);
    t = gsu.z;
    gsu_code(0x04accd, 4);
    if (t) { gsu_code(0x04ace9, 1); goto lACE9; }
    R[2] = 0x0034;
    R[2] = gsu_add(R[2], R[8], 0);
    gsu_code(0x04acd1, 7);
    R[2] = gsu_ldw(R[2]);
    R[2] = R[2];
    gsu_moves(R[2]);
    t = !gsu.s;
    gsu_code(0x04acd8, 4);
    if (t) { gsu_code(0x04ace4, 1); goto lACE4; }
    R[1] = R[1];
    gsu_moves(R[1]);
    t = !gsu.s;
    gsu_code(0x04acdc, 5);
    if (t) { gsu_code(0x04ace9, 1); goto lACE9; }
    t = 1;
    gsu_code(0x04ace1, 3);
    gsu_code(0x04acfc, 1);
    if (t) goto lACFC;
lACE4:
    R[1] = R[1];
    gsu_moves(R[1]);
    t = !gsu.s;
    gsu_code(0x04ace5, 4);
    if (t) { gsu_code(0x04acfc, 1); goto lACFC; }
    gsu_code(0x04ace9, 1);
lACE9:
    gsu_code(0x04acea, 4);
    R[2] = gsu_ldw(FX_WHEEL_FLAGS);
    R[0] = 0x0002;
    R[0] = gsu_and(R[0], R[2]);
    t = gsu.z;
    gsu_code(0x04acee, 6);
    if (t) { gsu_code(0x04acfc, 1); goto lACFC; }
    gsu_code(0x04acf4, 5);
    R[6] = gsu_ldw(FX_PARAM_GRIP_SLIDE);
    R[5] = gsu_add(R[5], R[5], 0);
    gsu_code(0x04acf9, 4);
    R[5] = gsu_fmult(R[5], R[6]);
lACFC:
    gsu_code(0x04acfd, 4);
    R[2] = gsu_ldw(FX_WHEEL_FLAGS);
    R[1] = 0x0042;
    R[1] = gsu_add(R[1], R[8], 0);
    gsu_code(0x04ad01, 6);
    R[1] = gsu_ldw(R[1]);
    R[0] = 0x000C;
    R[0] = gsu_and(R[0], R[1]);
    t = gsu.z;
    gsu_code(0x04ad07, 6);
    if (t) { gsu_code(0x04ad2f, 1); goto lAD2F; }
    R[0] = 0x0002;
    R[0] = gsu_and(R[0], R[2]);
    t = gsu.z;
    gsu_code(0x04ad0d, 7);
    if (t) { gsu_code(0x04ad1f, 1); goto lAD1F; }
    gsu_code(0x04ad14, 5);
    R[6] = gsu_ldw(FX_PARAM_GRIP_SIDE);
    R[0] = gsu_add(R[5], R[5], 0);
    gsu_code(0x04ad19, 3);
    R[0] = gsu_fmult(R[0], R[6]);
    t = 1;
    R[5] = gsu_add(R[5], R[0], 0);
    gsu_code(0x04ad1c, 3);
    gsu_code(0x04ad2f, 1);
    if (t) goto lAD2F;
lAD1F:
    R[0] = 0x0001;
    R[0] = gsu_and(R[0], R[2]);
    t = gsu.z;
    gsu_code(0x04ad20, 6);
    if (t) { gsu_code(0x04ad2f, 1); goto lAD2F; }
    gsu_code(0x04ad26, 5);
    R[6] = gsu_ldw(FX_PARAM_GRIP_AHEAD);
    R[0] = gsu_add(R[5], R[5], 0);
    gsu_code(0x04ad2b, 3);
    R[0] = gsu_fmult(R[0], R[6]);
    R[5] = gsu_add(R[5], R[0], 0);
    gsu_code(0x04ad2e, 2);
lAD2F:
    R[0] = 0x0002;
    R[0] = gsu_and(R[0], R[2]);
    t = gsu.z;
    gsu_code(0x04ad30, 6);
    if (t) { gsu_code(0x04ad3d, 1); goto lAD3D; }
    R[0] = 0x4650;
    gsu_code(0x04ad36, 8);
    gsu_stw(FX_CONTACT_FORCE, R[0]);
lAD3D:
    gsu_code(0x04ad3e, 4);
    R[6] = gsu_ldw(FX_CONTACT_FORCE);
    R[5] = gsu_add(R[5], R[5], 0);
    gsu_code(0x04ad42, 4);
    R[5] = gsu_fmult(R[5], R[6]);
    R[0] = R[4];
    gsu_moves(R[4]);
    t = !gsu.s;
    gsu_code(0x04ad46, 4);
    if (t) { gsu_code(0x04ad4c, 1); goto lAD4C; }
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_code(0x04ad4a, 3);
lAD4C:
    R[0] = gsu_sub(R[0], R[5], 0);
    t = gsu.s;
    gsu_code(0x04ad4d, 3);
    if (t) { gsu_code(0x04ad80, 1); goto lAD80; }
    R[2] = R[0];
    R[4] = R[4];
    gsu_moves(R[4]);
    t = !gsu.s;
    gsu_code(0x04ad50, 7);
    if (t) { gsu_code(0x04ad5a, 1); goto lAD5A; }
    R[5] = gsu_not(R[5]);
    R[5] = gsu_inc(R[5]);
    gsu_code(0x04ad57, 4);
lAD5A:
    R[1] = R[4];
    R[4] = R[5];
    R[0] = gsu_sub(R[1], R[5], 0);
    R[1] = R[0];
    gsu_moves(R[0]);
    t = !gsu.s;
    gsu_code(0x04ad5b, 10);
    if (t) { gsu_code(0x04ad68, 1); goto lAD68; }
    R[1] = gsu_not(R[1]);
    R[1] = gsu_inc(R[1]);
    gsu_code(0x04ad65, 4);
lAD68:
    gsu_code(0x04ad69, 4);
    R[0] = gsu_ldw(FX_PARAM_SLIDE_LIMIT);
    gsu_sub(R[0], R[1], 0);   /* CMP */
    t = !gsu.s;
    gsu_code(0x04ad6d, 4);
    if (t) { gsu_code(0x04ad80, 1); goto lAD80; }
    R[0] = 0x0056;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04ad71, 5);
    R[0] = gsu_ldw(R[0]);
    R[0] = gsu_add(R[0], R[1], 0);
    gsu_code(0x04ad76, 2);
    gsu_sbk(R[0]);
    gsu_code(0x04ad78, 4);
    R[2] = gsu_ldw(FX_WHEEL_FLAGS);
    R[0] = 0x0020;
    R[0] = gsu_or(R[0], R[2]);
    gsu_code(0x04ad7c, 5);
    gsu_sbk(R[0]);
lAD80:
    R[5] = 0x0000;
    R[6] = 0x0000;
    R[11] = 0xAD8B;
    gsu_code(0x04ad81, 10); gsu_code(0x04973f, 1);
    abi_FxSumsPushLocal();
lAD8B:
    R[6] = 0x0048;
    R[6] = gsu_add(R[6], R[8], 0);
    gsu_code(0x04ad8c, 6);
    R[6] = gsu_ldw(R[6]);
    R[6] = R[6];
    gsu_moves(R[6]);
    t = gsu.z;
    gsu_code(0x04ad92, 4);
    if (t) { gsu_code(0x04adc4, 1); goto lADC4; }
    gsu_code(0x04ad96, 5);
    R[0] = gsu_ldw(FX_WHEEL_AHEAD);
    R[0] = gsu_add(R[0], R[0], 0);
    gsu_code(0x04ad9b, 3);
    R[6] = gsu_fmult(R[0], R[6]);
    R[4] = 0xFF38;
    R[5] = 0x00C8;
    R[6] = R[6];
    gsu_moves(R[6]);
    t = gsu.s;
    gsu_code(0x04ad9e, 10);
    if (t) { gsu_code(0x04adb3, 1); goto lADB3; }
    gsu_sub(R[6], R[5], 0);   /* CMP */
    t = gsu.s;
    gsu_code(0x04ada8, 6);
    if (t) { gsu_code(0x04adbb, 1); goto lADBB; }
    R[6] = R[5];
    t = 1;
    gsu_code(0x04adae, 5);
    gsu_code(0x04adbb, 1);
    if (t) goto lADBB;
lADB3:
    gsu_sub(R[6], R[4], 0);   /* CMP */
    t = !gsu.s;
    gsu_code(0x04adb4, 5);
    if (t) { gsu_code(0x04adbb, 1); goto lADBB; }
    R[6] = R[4];
    gsu_code(0x04adb9, 3);
lADBB:
    R[4] = 0x0000;
    R[5] = 0x0000;
    R[11] = 0xADC4;
    gsu_code(0x04adbc, 8); gsu_code(0x04973f, 1);
    abi_FxSumsPushLocal();
lADC4:
    R[11] = 0xADC9;
    gsu_code(0x04adc5, 4); gsu_code(0x0497bb, 1);
    abi_FxSumsRotateLocal();
lADC9:
    gsu_code(0x04adca, 4);
    R[0] = gsu_ldw(FX_WORLD_SUM);
    gsu_code(0x04adce, 4);
    R[1] = gsu_ldw((FX_WORLD_SUM + 2));
    gsu_code(0x04add2, 4);
    R[2] = gsu_ldw((FX_WORLD_SUM + 4));
    R[0] = gsu_or(R[0], R[1]);
    R[0] = gsu_or(R[0], R[2]);
    t = !gsu.z;
    gsu_code(0x04add6, 4);
    if (t) { gsu_code(0x04adde, 1); goto lADDE; }
    gsu_code(0x04adda, 4); gsu_code(0x04ae39, 1);
    goto lAE39;
lADDE:
    R[1] = 0x0036;
    R[1] = gsu_add(R[1], R[9], 0);
    gsu_code(0x04addf, 7);
    R[1] = gsu_ldw(R[1]);
    R[2] = 0x0038;
    R[2] = gsu_add(R[2], R[9], 0);
    gsu_code(0x04ade6, 6);
    R[2] = gsu_ldw(R[2]);
    R[3] = 0x003A;
    R[3] = gsu_add(R[3], R[9], 0);
    gsu_code(0x04adec, 6);
    R[3] = gsu_ldw(R[3]);
    gsu_code(0x04adf2, 4);
    R[6] = gsu_ldw(FX_PARAM_HIT);
    R[11] = 0xADFA;
    gsu_code(0x04adf6, 4); gsu_code(0x04987d, 1);
    abi_FxHitAddScaled();
lADFA:
    gsu_code(0x04adfb, 4);
    R[2] = gsu_ldw(FX_WHEEL_FLAGS);
    R[0] = 0x0200;
    R[0] = gsu_and(R[0], R[2]);
    t = gsu.z;
    gsu_code(0x04adff, 6);
    if (t) { gsu_code(0x04ae05, 1); goto lAE05; }
    gsu_code(0x04ae05, 1);
lAE05:
    R[1] = 0x003C;
    R[1] = gsu_add(R[1], R[9], 0);
    gsu_code(0x04ae06, 6);
    R[1] = gsu_ldw(R[1]);
    R[2] = 0x003E;
    R[2] = gsu_add(R[2], R[9], 0);
    gsu_code(0x04ae0c, 6);
    R[2] = gsu_ldw(R[2]);
    R[3] = 0x0040;
    R[3] = gsu_add(R[3], R[9], 0);
    gsu_code(0x04ae12, 6);
    R[3] = gsu_ldw(R[3]);
    R[0] = 0x005A;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04ae18, 4);
    R[0] = gsu_ldw(R[0]);
    R[2] = gsu_add(R[2], R[0], 0);
    R[0] = 0x005C;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04ae1c, 6);
    R[0] = gsu_ldw(R[0]);
    R[3] = gsu_add(R[3], R[0], 0);
    R[0] = 0x0004;
    gsu_code(0x04ae22, 6);
    gsu_romb(R[0]);
    gsu_code(0x04ae28, 4);
    R[0] = gsu_ldw(FX_PARAM_PUSH);
    gsu_code(0x04ae2c, 3);
    gsu_r14(0x9BC6);
    gsu_code(0x04ae2f, 2);
    gsu_r14(gsu_add(R[14], R[0], 0));
    gsu_code(0x04ae31, 1);
    R[0] = gsu_getb();
    gsu_code(0x04ae32, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x04ae33, 2);
    R[0] = gsu_getbh(R[0]);
    R[12] = R[0];
    R[11] = 0xAE39;
    nx = (uint16_t)(R[12]);
    at = nx;
    gsu_code(0x04ae35, 4);
    goto dispatch;
lAE39:
    gsu_code(0x04ae3a, 4);
    R[3] = gsu_ldw(FX_PARAM_BOUNCE);
    R[2] = R[3];
    R[2] = gsu_not(R[2]);
    R[2] = gsu_inc(R[2]);
    gsu_code(0x04ae3e, 9);
    R[0] = gsu_ldw(FX_CONTACT_SUM);
    R[0] = gsu_div2(R[0]);
    R[0] = gsu_div2(R[0]);
    R[0] = gsu_div2(R[0]);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.s;
    gsu_code(0x04ae47, 10);
    if (t) { gsu_code(0x04ae5b, 1); goto lAE5B; }
    gsu_sub(R[0], R[3], 0);   /* CMP */
    t = gsu.s;
    gsu_code(0x04ae51, 5);
    if (t) { gsu_code(0x04ae62, 1); goto lAE62; }
    R[0] = R[3];
    t = 1;
    gsu_code(0x04ae56, 5);
    gsu_code(0x04ae62, 1);
    if (t) goto lAE62;
lAE5B:
    gsu_sub(R[0], R[2], 0);   /* CMP */
    t = !gsu.s;
    gsu_code(0x04ae5c, 4);
    if (t) { gsu_code(0x04ae62, 1); goto lAE62; }
    R[0] = R[2];
    gsu_code(0x04ae60, 3);
lAE62:
    R[1] = R[0];
    R[0] = 0x004A;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_code(0x04ae63, 7);
    gsu_stw(R[0], R[1]);
    gsu_code(0x04ae6a, 4);
    R[0] = gsu_ldw((FX_CONTACT_SUM + 4));
    R[0] = gsu_div2(R[0]);
    R[0] = gsu_div2(R[0]);
    R[0] = gsu_div2(R[0]);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.s;
    gsu_code(0x04ae6e, 10);
    if (t) { gsu_code(0x04ae82, 1); goto lAE82; }
    gsu_sub(R[0], R[3], 0);   /* CMP */
    t = gsu.s;
    gsu_code(0x04ae78, 5);
    if (t) { gsu_code(0x04ae89, 1); goto lAE89; }
    R[0] = R[3];
    t = 1;
    gsu_code(0x04ae7d, 5);
    gsu_code(0x04ae89, 1);
    if (t) goto lAE89;
lAE82:
    gsu_sub(R[0], R[2], 0);   /* CMP */
    t = !gsu.s;
    gsu_code(0x04ae83, 4);
    if (t) { gsu_code(0x04ae89, 1); goto lAE89; }
    R[0] = R[2];
    gsu_code(0x04ae87, 3);
lAE89:
    R[1] = R[0];
    R[0] = 0x004E;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_code(0x04ae8a, 7);
    gsu_stw(R[0], R[1]);
    gsu_code(0x04ae91, 4);
    R[0] = gsu_ldw((FX_CONTACT_SUM + 2));
    R[0] = gsu_div2(R[0]);
    R[0] = gsu_div2(R[0]);
    R[0] = gsu_div2(R[0]);
    R[4] = R[0];
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.s;
    gsu_code(0x04ae95, 12);
    if (t) { gsu_code(0x04aeab, 1); goto lAEAB; }
    gsu_sub(R[0], R[3], 0);   /* CMP */
    t = gsu.s;
    gsu_code(0x04aea1, 5);
    if (t) { gsu_code(0x04aeb2, 1); goto lAEB2; }
    R[0] = R[3];
    t = 1;
    gsu_code(0x04aea6, 5);
    gsu_code(0x04aeb2, 1);
    if (t) goto lAEB2;
lAEAB:
    gsu_sub(R[0], R[2], 0);   /* CMP */
    t = !gsu.s;
    gsu_code(0x04aeac, 4);
    if (t) { gsu_code(0x04aeb2, 1); goto lAEB2; }
    R[0] = R[2];
    gsu_code(0x04aeb0, 3);
lAEB2:
    R[1] = R[0];
    R[0] = 0x004C;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_code(0x04aeb3, 7);
    gsu_stw(R[0], R[1]);
lAEB9:
    R[2] = 0x0046;
    R[2] = gsu_add(R[2], R[9], 0);
    gsu_code(0x04aeba, 6);
    R[2] = gsu_ldw(R[2]);
    R[0] = 0x0002;
    R[0] = gsu_and(R[0], R[2]);
    t = gsu.z;
    gsu_code(0x04aec0, 6);
    if (t) { gsu_code(0x04aed7, 1); goto lAED7; }
    R[1] = 0x0024;
    R[1] = gsu_add(R[1], R[9], 0);
    gsu_code(0x04aec6, 7);
    R[1] = gsu_ldw(R[1]);
    R[0] = 0x003A;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04aecd, 4);
    R[0] = gsu_ldw(R[0]);
    R[1] = gsu_add(R[1], R[0], 0);
    R[0] = 0x0024;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_code(0x04aed1, 7);
    gsu_stw(R[0], R[1]);
lAED7:
    gsu_code(0x04aed8, 4);
    R[1] = gsu_ldw(FX_WHEEL_FLAGS);
    R[0] = 0x0046;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_code(0x04aedc, 5);
    gsu_stw(R[0], R[1]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x04aee1, 4);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
lAEE5:
    R[1] = 0x0000;
    R[0] = 0x004A;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_code(0x04aee6, 7);
    gsu_stw(R[0], R[1]);
    R[0] = 0x004C;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_code(0x04aeed, 5);
    gsu_stw(R[0], R[1]);
    R[0] = 0x004E;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_code(0x04aef2, 5);
    gsu_stw(R[0], R[1]);
    R[0] = 0x0044;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04aef7, 4);
    R[0] = gsu_ldw(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_code(0x04aefb, 2);
    gsu_sbk(R[0]);
    t = 1;
    gsu_code(0x04aefd, 2);
    gsu_code(0x04aeb9, 1);
    if (t) goto lAEB9;
    /* FxContactsResolve */
lAEFF:
    gsu_code(0x04af00, 2);
    gsu_stw(R[10], R[11]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[0] = 0x0000;
    gsu_code(0x04af02, 6);
    gsu_romb(R[0]);
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_code(0x04af08, 5);
    gsu_stw(FX_PUSH_FLAGS, R[0]);
    R[0] = 0x002C;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04af0d, 4);
    R[0] = gsu_ldw(R[0]);
    gsu_code(0x04af11, 4);
    gsu_stw(FX_HIT_DEPTH, R[0]);
    R[3] = 0x23AE;
    R[13] = 0x0000;
    /* CACHE */
    gsu_code(0x04af15, 5); gsu_cache(0xaf1a); gsu_code(0x04af1a, 1);
lAF1A:
    R[13] = gsu_inc(R[13]);
    R[0] = 0x0014;
    gsu_sub(R[0], R[13], 0);   /* CMP */
    t = !gsu.z;
    gsu_code(0x04af1b, 7);
    if (t) { gsu_code(0x04af26, 1); goto lAF26; }
    gsu_code(0x04af22, 4); gsu_code(0x04af3c, 1);
    goto lAF3C;
lAF26:
    gsu_code(0x04af27, 1);
    R[0] = gsu_ldw(R[3]);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.z;
    gsu_code(0x04af28, 4);
    if (t) { gsu_code(0x04af4e, 1); goto lAF4E; }
    R[11] = 0xAF31;
    gsu_code(0x04af2c, 5); gsu_code(0x04af54, 1);
    goto lAF54;
lAF31:
    gsu_code(0x04af32, 4);
    R[0] = gsu_ldw(FX_CONTACT_FORCE_UP);
    gsu_code(0x04af36, 4);
    gsu_stw(FX_CONTACT_FORCE, R[0]);
    t = 1;
    gsu_code(0x04af3a, 2);
    gsu_code(0x04af1a, 1);
    if (t) goto lAF1A;
lAF3C:
    gsu_code(0x04af3d, 4);
    R[1] = gsu_ldw(FX_HIT_DEPTH);
    R[0] = 0x002C;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04af41, 5);
    gsu_stw(R[0], R[1]);
    gsu_code(0x04af46, 4);
    R[0] = gsu_ldw(FX_PUSH_FLAGS);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x04af4a, 4);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
lAF4E:
    R[0] = 0x000A;
    t = 1;
    R[3] = gsu_add(R[3], R[0], 0);
    gsu_code(0x04af4f, 5);
    gsu_code(0x04af1a, 1);
    if (t) goto lAF1A;
    /* FxContactResolve */
lAF54:
    gsu_code(0x04af55, 1);
    R[0] = gsu_ldw(R[3]);
    R[3] = gsu_inc(R[3]);
    R[3] = gsu_inc(R[3]);
    R[1] = R[0];
    gsu_moves(R[0]);
    t = !gsu.z;
    gsu_code(0x04af56, 6);
    if (t) { gsu_code(0x04af60, 1); goto lAF60; }
    gsu_code(0x04af5c, 4); gsu_code(0x04b19a, 1);
    goto lB19A;
lAF60:
    gsu_code(0x04af61, 4);
    R[2] = gsu_ldw(FX_PUSH_FLAGS);
    R[0] = gsu_or(R[0], R[2]);
    gsu_code(0x04af65, 2);
    gsu_sbk(R[0]);
    R[0] = gsu_hib(R[1]);
    t = gsu.z;
    gsu_code(0x04af67, 4);
    if (t) { gsu_code(0x04af6f, 1); goto lAF6F; }
    gsu_code(0x04af6b, 4); gsu_code(0x04b18a, 1);
    goto lB18A;
lAF6F:
    gsu_code(0x04af70, 2);
    gsu_stw(R[10], R[11]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_code(0x04af72, 3);
    R[0] = gsu_ldw(R[3]);
    R[3] = gsu_inc(R[3]);
    R[3] = gsu_inc(R[3]);
    R[7] = R[0];
    gsu_moves(R[0]);
    t = !gsu.s;
    gsu_code(0x04af75, 6);
    if (t) { gsu_code(0x04af7f, 1); goto lAF7F; }
    gsu_code(0x04af7b, 4); gsu_code(0x04b191, 1);
    goto lB191;
lAF7F:
    gsu_code(0x04af80, 4);
    R[0] = gsu_ldw(FX_CONTACT_DEPTH);
    R[5] = R[0];
    gsu_moves(R[0]);
    t = !gsu.s;
    gsu_code(0x04af84, 4);
    if (t) { gsu_code(0x04af8b, 1); goto lAF8B; }
    R[5] = gsu_not(R[5]);
    R[5] = gsu_inc(R[5]);
    gsu_code(0x04af88, 4);
lAF8B:
    R[4] = R[7];
    gsu_moves(R[7]);
    t = !gsu.s;
    gsu_code(0x04af8c, 4);
    if (t) { gsu_code(0x04af93, 1); goto lAF93; }
    gsu_code(0x04af90, 1);
lAF90:
    R[4] = gsu_not(R[4]);
    R[4] = gsu_inc(R[4]);
    gsu_code(0x04af91, 3);
lAF93:
    gsu_sub(R[5], R[4], 0);   /* CMP */
    t = gsu.s;
    gsu_code(0x04af94, 5);
    if (t) { gsu_code(0x04af9d, 1); goto lAF9D; }
    gsu_code(0x04af99, 5);
    gsu_stw(FX_CONTACT_DEPTH, R[7]);
lAF9D:
    R[0] = 0x7D00;
    gsu_sub(R[0], R[7], 0);   /* CMP */
    t = !gsu.z;
    gsu_code(0x04af9e, 7);
    if (t) { gsu_code(0x04afa9, 1); goto lAFA9; }
    gsu_code(0x04afa5, 4); gsu_code(0x04b191, 1);
    goto lB191;
lAFA9:
    R[7] = gsu_add(R[7], R[7], 0);
    R[7] = gsu_add(R[7], R[7], 0);
    R[7] = gsu_add(R[7], R[7], 0);
    R[7] = gsu_add(R[7], R[7], 0);
    gsu_code(0x04afaa, 10);
    R[4] = gsu_ldw(R[3]);
    R[3] = gsu_inc(R[3]);
    R[3] = gsu_inc(R[3]);
    gsu_code(0x04afb4, 4);
    R[5] = gsu_ldw(R[3]);
    R[3] = gsu_inc(R[3]);
    R[3] = gsu_inc(R[3]);
    gsu_code(0x04afb8, 4);
    R[6] = gsu_ldw(R[3]);
    R[3] = gsu_inc(R[3]);
    R[3] = gsu_inc(R[3]);
    R[0] = gsu_or(R[4], R[6]);
    t = !gsu.z;
    gsu_code(0x04afbc, 6);
    if (t) { gsu_code(0x04afd2, 1); goto lAFD2; }
    gsu_code(0x04afc2, 5);
    R[0] = gsu_ldw(FX_WHEEL_CONTACT);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.z;
    gsu_code(0x04afc7, 4);
    if (t) { gsu_code(0x04afcf, 1); goto lAFCF; }
    gsu_code(0x04afcb, 4); gsu_code(0x04b185, 1);
    goto lB185;
lAFCF:
    R[0] = 0x0001;
    gsu_code(0x04afd0, 3);
    gsu_sbk(R[0]);
lAFD2:
    gsu_code(0x04afd3, 4);
    R[0] = gsu_ldw(FX_PUSH_TURN);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = !gsu.z;
    gsu_code(0x04afd7, 4);
    if (t) { gsu_code(0x04afdf, 1); goto lAFDF; }
    gsu_code(0x04afdb, 4); gsu_code(0x04b06c, 1);
    goto lB06C;
lAFDF:
    R[1] = 0x0040;
    gsu_sub(R[0], R[1], 0);   /* CMP */
    t = !gsu.z;
    gsu_code(0x04afe0, 7);
    if (t) { gsu_code(0x04aff3, 1); goto lAFF3; }
    R[0] = R[6];
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
    R[6] = R[4];
    R[4] = R[0];
    gsu_code(0x04afe7, 12); gsu_code(0x04b06c, 1);
    goto lB06C;
lAFF3:
    R[1] = 0x00C0;
    gsu_sub(R[0], R[1], 0);   /* CMP */
    t = !gsu.z;
    gsu_code(0x04aff4, 7);
    if (t) { gsu_code(0x04b006, 1); goto lB006; }
    R[0] = R[4];
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
    R[4] = R[6];
    R[6] = R[0];
    t = 1;
    gsu_code(0x04affb, 11);
    gsu_code(0x04b06c, 1);
    if (t) goto lB06C;
lB006:
    R[1] = 0x0080;
    gsu_sub(R[0], R[1], 0);   /* CMP */
    t = !gsu.z;
    gsu_code(0x04b007, 7);
    if (t) { gsu_code(0x04b017, 1); goto lB017; }
    R[4] = gsu_not(R[4]);
    R[4] = gsu_inc(R[4]);
    R[6] = gsu_not(R[6]);
    R[6] = gsu_inc(R[6]);
    t = 1;
    gsu_code(0x04b00e, 9);
    gsu_code(0x04b06c, 1);
    if (t) goto lB06C;
lB017:
    gsu_code(0x04b018, 2);
    gsu_stw(R[10], R[2]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_code(0x04b01a, 4);
    gsu_stw(R[10], R[3]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_code(0x04b01e, 4);
    gsu_stw(R[10], R[5]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_code(0x04b022, 4);
    gsu_stw(R[10], R[7]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_code(0x04b026, 4);
    gsu_stw(R[10], R[8]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_code(0x04b02a, 4);
    gsu_stw(R[10], R[9]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_code(0x04b02e, 4);
    gsu_stw(R[10], R[11]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[1] = R[4];
    R[3] = R[6];
    R[2] = R[0];
    R[0] = R[1];
    R[0] = gsu_or(R[0], R[3]);
    t = gsu.z;
    gsu_code(0x04b032, 13);
    if (t) { gsu_code(0x04b04c, 1); goto lB04C; }
    R[11] = 0xB044;
    gsu_code(0x04b03f, 5); gsu_code(0x04c15c, 1);
    abi_FxTurn();
    gsu_code(0x04b044, 1);
lB044:
    R[1] = gsu_add(R[1], R[1], 0);
    R[1] = gsu_add(R[1], R[1], 0);
    R[3] = gsu_add(R[3], R[3], 0);
    R[3] = gsu_add(R[3], R[3], 0);
    gsu_code(0x04b045, 8);
lB04C:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x04b04d, 4);
    R[11] = gsu_ldw(R[10]);
    R[4] = R[3];
    R[6] = R[1];
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x04b051, 8);
    R[9] = gsu_ldw(R[10]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x04b059, 4);
    R[8] = gsu_ldw(R[10]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x04b05d, 4);
    R[7] = gsu_ldw(R[10]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x04b061, 4);
    R[5] = gsu_ldw(R[10]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x04b065, 4);
    R[3] = gsu_ldw(R[10]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x04b069, 4);
    R[2] = gsu_ldw(R[10]);
lB06C:
    R[3] = gsu_sub(R[3], 6, 0);
    gsu_code(0x04b06d, 5);
    gsu_stw(R[3], R[4]);
    R[3] = gsu_inc(R[3]);
    R[3] = gsu_inc(R[3]);
    gsu_code(0x04b072, 4);
    gsu_stw(R[3], R[5]);
    R[3] = gsu_inc(R[3]);
    R[3] = gsu_inc(R[3]);
    gsu_code(0x04b076, 4);
    gsu_stw(R[3], R[6]);
    R[3] = gsu_inc(R[3]);
    R[3] = gsu_inc(R[3]);
    gsu_code(0x04b07a, 6);
    gsu_stw(FX_SIZES, R[4]);
    gsu_code(0x04b080, 4);
    gsu_stw((FX_SIZES + 2), R[5]);
    gsu_code(0x04b084, 4);
    gsu_stw((FX_SIZES + 4), R[6]);
    R[0] = 0x002E;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_code(0x04b088, 4);
    R[0] = gsu_ldw(R[0]);
    R[0] = gsu_add(R[0], R[0], 0);
    gsu_code(0x04b08c, 5);
    R[6] = gsu_ldw(FX_SIZES);
    gsu_code(0x04b091, 1);
    R[0] = gsu_fmult(R[0], R[6]);
    R[1] = R[0];
    R[0] = 0x0030;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_code(0x04b092, 6);
    R[0] = gsu_ldw(R[0]);
    R[0] = gsu_add(R[0], R[0], 0);
    gsu_code(0x04b098, 5);
    R[6] = gsu_ldw((FX_SIZES + 2));
    gsu_code(0x04b09d, 1);
    R[0] = gsu_fmult(R[0], R[6]);
    R[1] = gsu_add(R[1], R[0], 0);
    R[0] = 0x0032;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_code(0x04b09e, 6);
    R[0] = gsu_ldw(R[0]);
    R[0] = gsu_add(R[0], R[0], 0);
    gsu_code(0x04b0a4, 5);
    R[6] = gsu_ldw((FX_SIZES + 4));
    gsu_code(0x04b0a9, 1);
    R[0] = gsu_fmult(R[0], R[6]);
    R[1] = gsu_add(R[1], R[0], 0);
    R[1] = gsu_not(R[1]);
    R[1] = gsu_inc(R[1]);
    R[2] = R[1];
    R[0] = 0x0042;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_code(0x04b0aa, 12);
    gsu_stw(R[0], R[1]);
    R[0] = R[1];
    R[6] = 0x0044;
    R[6] = gsu_add(R[6], R[9], 0);
    gsu_code(0x04b0b6, 8);
    R[6] = gsu_ldw(R[6]);
    gsu_code(0x04b0be, 2);
    { uint32_t p = gsu_lmult(R[0], R[6]); R[4] = (uint16_t)p; R[0] = (uint16_t)(p >> 16); }
    R[0] = gsu_lob(R[0]);
    R[0] = gsu_swap(R[0]);
    R[4] = gsu_hib(R[4]);
    R[0] = gsu_or(R[0], R[4]);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = !gsu.s;
    gsu_code(0x04b0c0, 9);
    if (t) { gsu_code(0x04b0db, 1); goto lB0DB; }
    R[0] = gsu_div2(R[0]);
    R[0] = gsu_add(R[0], R[7], 0);
    gsu_code(0x04b0c9, 8);
    R[1] = gsu_ldw(FX_WHEEL_FLAGS);
    R[4] = 0x0100;
    R[4] = gsu_and(R[4], R[1]);
    t = gsu.z;
    gsu_code(0x04b0d1, 7);
    if (t) { gsu_code(0x04b0e4, 1); goto lB0E4; }
    t = 1;
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_code(0x04b0d8, 3);
    gsu_code(0x04b0e4, 1);
    if (t) goto lB0E4;
lB0DB:
    R[1] = 0x07D0;
    gsu_sub(R[0], R[1], 0);   /* CMP */
    t = !gsu.s;
    gsu_code(0x04b0dc, 7);
    if (t) { gsu_code(0x04b0e4, 1); goto lB0E4; }
    R[0] = gsu_add(R[0], R[7], 0);
    gsu_code(0x04b0e3, 2);
lB0E4:
    R[1] = R[0];
    gsu_code(0x04b0e5, 6);
    R[6] = gsu_ldw((FX_SIZES + 2));
    R[0] = R[1];
    gsu_code(0x04b0eb, 3);
    R[0] = gsu_fmult(R[0], R[6]);
    R[5] = R[0];
    R[6] = R[6];
    gsu_moves(R[6]);
    t = !gsu.z;
    gsu_code(0x04b0ee, 6);
    if (t) { gsu_code(0x04b133, 1); goto lB133; }
    R[0] = R[2];
    gsu_moves(R[2]);
    t = !gsu.s;
    gsu_code(0x04b0f4, 5);
    if (t) { gsu_code(0x04b0fa, 1); goto lB0FA; }
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_code(0x04b0f9, 2);
lB0FA:
    R[0] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_add(R[0], R[7], 0);
    R[1] = R[0];
    R[6] = 0x1000;
    R[0] = gsu_add(R[0], R[0], 0);
    gsu_code(0x04b0fb, 10);
    R[0] = gsu_fmult(R[0], R[6]);
    R[1] = gsu_add(R[1], R[0], 0);
    R[6] = R[0];
    gsu_moves(R[0]);
    t = !gsu.s;
    gsu_code(0x04b105, 6);
    if (t) { gsu_code(0x04b10e, 1); goto lB10E; }
    R[6] = gsu_not(R[6]);
    R[6] = gsu_inc(R[6]);
    gsu_code(0x04b10b, 4);
lB10E:
    R[0] = 0x0064;
    gsu_sub(R[0], R[6], 0);   /* CMP */
    t = !gsu.s;
    gsu_code(0x04b10f, 7);
    if (t) { gsu_code(0x04b133, 1); goto lB133; }
    R[0] = 0x003C;
    R[0] = gsu_add(R[0], R[9], 0);
    gsu_code(0x04b116, 5);
    R[0] = gsu_ldw(R[0]);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = !gsu.s;
    gsu_code(0x04b11b, 4);
    if (t) { gsu_code(0x04b12a, 1); goto lB12A; }
    gsu_code(0x04b11f, 5);
    R[0] = gsu_ldw(FX_HIT_DEPTH);
    R[6] = 0x0100;
    R[0] = gsu_or(R[0], R[6]);
    t = 1;
    gsu_code(0x04b124, 6); gsu_code(0x04b133, 1);
    gsu_sbk(R[0]);
    if (t) goto lB133;
lB12A:
    gsu_code(0x04b12b, 4);
    R[0] = gsu_ldw(FX_HIT_DEPTH);
    R[6] = 0x0200;
    R[0] = gsu_or(R[0], R[6]);
    gsu_code(0x04b12f, 5);
    gsu_sbk(R[0]);
lB133:
    gsu_code(0x04b134, 4);
    R[0] = gsu_ldw(FX_SIZES);
    gsu_code(0x04b138, 4);
    R[6] = gsu_ldw((FX_SIZES + 4));
    R[0] = gsu_or(R[0], R[6]);
    t = gsu.z;
    gsu_code(0x04b13c, 3);
    if (t) { gsu_code(0x04b148, 1); goto lB148; }
    gsu_code(0x04b13f, 5);
    R[0] = gsu_ldw(FX_HIT_DEPTH);
    R[6] = 0x0400;
    R[0] = gsu_or(R[0], R[6]);
    gsu_code(0x04b144, 5);
    gsu_sbk(R[0]);
lB148:
    gsu_code(0x04b149, 4);
    R[6] = gsu_ldw(FX_SIZES);
    R[0] = R[1];
    gsu_code(0x04b14d, 3);
    R[0] = gsu_fmult(R[0], R[6]);
    R[4] = R[0];
    gsu_code(0x04b150, 6);
    R[6] = gsu_ldw((FX_SIZES + 4));
    R[0] = R[1];
    gsu_code(0x04b156, 3);
    R[0] = gsu_fmult(R[0], R[6]);
    R[6] = R[0];
    R[11] = 0xB15F;
    gsu_code(0x04b159, 6); gsu_code(0x0496c3, 1);
    abi_FxSumsPushWorld();
lB15F:
    gsu_code(0x04b160, 4);
    R[6] = gsu_ldw(FX_SIZES);
    gsu_code(0x04b164, 2);
    R[0] = gsu_fmult(R[7], R[6]);
    gsu_code(0x04b166, 4);
    R[1] = gsu_ldw(FX_CONTACT_SUM);
    R[0] = gsu_add(R[0], R[1], 0);
    gsu_code(0x04b16a, 2);
    gsu_sbk(R[0]);
    gsu_code(0x04b16c, 4);
    R[6] = gsu_ldw((FX_SIZES + 2));
    gsu_code(0x04b170, 2);
    R[0] = gsu_fmult(R[7], R[6]);
    R[2] = R[0];
    gsu_code(0x04b172, 6);
    R[1] = gsu_ldw((FX_CONTACT_SUM + 2));
    R[0] = gsu_add(R[0], R[1], 0);
    gsu_code(0x04b178, 2);
    gsu_sbk(R[0]);
    gsu_code(0x04b17a, 4);
    R[6] = gsu_ldw((FX_SIZES + 4));
    gsu_code(0x04b17e, 2);
    R[0] = gsu_fmult(R[7], R[6]);
    gsu_code(0x04b180, 4);
    R[1] = gsu_ldw((FX_CONTACT_SUM + 4));
    R[0] = gsu_add(R[0], R[1], 0);
    gsu_code(0x04b184, 2);
    gsu_sbk(R[0]);
lB185:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x04b186, 4);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
lB18A:
    R[0] = 0x0008;
    R[3] = gsu_add(R[3], R[0], 0);
    nx = (uint16_t)(R[11]);
    at = nx;
    gsu_code(0x04b18b, 6);
    goto dispatch;
lB191:
    R[0] = 0x0006;
    R[3] = gsu_add(R[3], R[0], 0);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x04b192, 8);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
lB19A:
    R[0] = 0x0008;
    R[3] = gsu_add(R[3], R[0], 0);
    nx = (uint16_t)(R[11]);
    at = nx;
    gsu_code(0x04b19b, 6);
    goto dispatch;
    /* FxCarHandler */
lB1A1:
    gsu_code(0x04b1a2, 2);
    gsu_stw(R[10], R[11]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[1] = 0x0004;
    R[0] = 0x001C;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04b1a4, 9);
    R[0] = gsu_ldw(R[0]);
    R[0] = gsu_or(R[0], R[1]);
    gsu_code(0x04b1ad, 2);
    gsu_sbk(R[0]);
    R[0] = 0x003E;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04b1af, 4);
    R[0] = gsu_ldw(R[0]);
lB1B2:
    R[2] = R[0];
    gsu_moves(R[0]);
    t = gsu.z;
    gsu_code(0x04b1b3, 4);
    if (t) { gsu_code(0x04b1c4, 1); goto lB1C4; }
    R[0] = 0x001C;
    R[0] = gsu_add(R[0], R[2], 0);
    gsu_code(0x04b1b7, 5);
    R[0] = gsu_ldw(R[0]);
    R[0] = gsu_or(R[0], R[1]);
    gsu_code(0x04b1bc, 2);
    gsu_sbk(R[0]);
    R[0] = 0x0034;
    R[0] = gsu_add(R[0], R[2], 0);
    gsu_code(0x04b1be, 4);
    R[0] = gsu_ldw(R[0]);
    t = 1;
    gsu_code(0x04b1c2, 2);
    gsu_code(0x04b1b2, 1);
    if (t) goto lB1B2;
lB1C4:
    gsu_code(0x04b1c5, 4);
    R[0] = gsu_ldw(FX_INIT_2472);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.z;
    gsu_code(0x04b1c9, 4);
    if (t) { gsu_code(0x04b1d7, 1); goto lB1D7; }
    R[11] = 0xB1D2;
    gsu_code(0x04b1cd, 5); gsu_code(0x04b9ac, 1);
    abi_FxCarPose();
    gsu_code(0x04b1d2, 1);
lB1D2:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x04b1d3, 4);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
lB1D7:
    R[0] = 0x001C;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04b1d8, 4);
    R[0] = gsu_ldw(R[0]);
    R[1] = 0x0002;
    R[0] = gsu_bic(R[0], R[1]);
    gsu_code(0x04b1dc, 6);
    gsu_sbk(R[0]);
    R[1] = 0x0000;
    R[0] = 0x0092;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04b1e2, 8);
    gsu_stw(R[0], R[1]);
    R[0] = 0x0009;
    gsu_code(0x04b1ea, 4);
    gsu_romb(R[0]);
    R[0] = 0x006E;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04b1ee, 4);
    R[0] = gsu_ldw(R[0]);
    gsu_code(0x04b1f2, 2);
    gsu_r14(R[0]);
    R[1] = 0x2652;
    R[12] = 0x0019;
    R[13] = 0xB1FB;
    gsu_code(0x04b1f4, 8);
lB1FB:
    gsu_code(0x04b1fc, 1);
    R[0] = gsu_getb();
    gsu_code(0x04b1fd, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x04b1fe, 2);
    R[0] = gsu_getbh(R[0]);
    gsu_code(0x04b200, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x04b201, 1);
    gsu_stw(R[1], R[0]);
    R[1] = gsu_inc(R[1]);
    R[1] = gsu_inc(R[1]);
    t = gsu_count_down(&R[12]);
    gsu_code(0x04b202, 3);
    if (t) { gsu_code(0x04b1fb, 1); goto lB1FB; }
    R[1] = 0x001C;
    R[1] = gsu_add(R[1], R[8], 0);
    gsu_code(0x04b205, 7);
    R[1] = gsu_ldw(R[1]);
    R[0] = 0x0020;
    R[0] = gsu_and(R[0], R[1]);
    t = gsu.z;
    gsu_code(0x04b20c, 6);
    if (t) { gsu_code(0x04b216, 1); goto lB216; }
    gsu_code(0x04b212, 4); gsu_code(0x04b990, 1);
    goto lB990;
lB216:
    R[0] = 0x0010;
    R[0] = gsu_and(R[0], R[1]);
    t = gsu.z;
    gsu_code(0x04b217, 7);
    if (t) { gsu_code(0x04b23b, 1); goto lB23B; }
    R[0] = 0x0064;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04b21e, 5);
    R[0] = gsu_ldw(R[0]);
    R[1] = 0x0008;
    R[0] = gsu_and(R[0], R[1]);
    t = !gsu.z;
    gsu_code(0x04b223, 6);
    if (t) { gsu_code(0x04b22d, 1); goto lB22D; }
    gsu_code(0x04b229, 4); gsu_code(0x04b990, 1);
    goto lB990;
lB22D:
    R[11] = 0xB232;
    gsu_code(0x04b22e, 4); gsu_code(0x04c3d5, 1);
    goto lC3D5;
lB232:
    R[0] = R[0];
    gsu_moves(R[0]);
    t = !gsu.z;
    gsu_code(0x04b233, 4);
    if (t) { gsu_code(0x04b23b, 1); goto lB23B; }
    gsu_code(0x04b237, 4); gsu_code(0x04b990, 1);
    goto lB990;
lB23B:
    R[1] = 0x0006;
    R[11] = 0xB242;
    gsu_code(0x04b23c, 6); gsu_code(0x0495be, 1);
    abi_FxRecordMove();
lB242:
    gsu_code(0x04b243, 2);
    gsu_stw(R[10], R[8]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[0] = 0x0026;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04b245, 6);
    R[0] = gsu_ldw(R[0]);
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_code(0x04b24b, 5);
    gsu_stw(GSU_IN_20, R[0]);
    R[0] = 0x0024;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04b250, 4);
    R[0] = gsu_ldw(R[0]);
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_code(0x04b254, 5);
    gsu_stw(GSU_IN_22, R[0]);
    R[0] = 0x0028;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04b259, 4);
    R[0] = gsu_ldw(R[0]);
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_code(0x04b25d, 5);
    gsu_stw(GSU_IN_24, R[0]);
    R[9] = 0x00E4;
    R[11] = 0xB269;
    gsu_code(0x04b262, 7); gsu_code(0x04c1a7, 1);
    abi_FxMatrixFromAngles();
lB269:
    R[9] = 0x00E4;
    R[11] = 0xB271;
    gsu_code(0x04b26a, 7); gsu_code(0x04c2c8, 1);
    abi_FxMatrixTranspose04();
lB271:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x04b272, 4);
    R[8] = gsu_ldw(R[10]);
    R[0] = 0x0064;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04b276, 4);
    R[0] = gsu_ldw(R[0]);
    R[1] = 0x0008;
    R[0] = gsu_and(R[0], R[1]);
    t = gsu.z;
    gsu_code(0x04b27a, 6);
    if (t) { gsu_code(0x04b28e, 1); goto lB28E; }
    R[11] = 0xB285;
    gsu_code(0x04b280, 5); gsu_code(0x04c3d5, 1);
    goto lC3D5;
lB285:
    R[0] = R[0];
    gsu_moves(R[0]);
    t = !gsu.z;
    gsu_code(0x04b286, 4);
    if (t) { gsu_code(0x04b29a, 1); goto lB29A; }
    gsu_code(0x04b28a, 4); gsu_code(0x04b990, 1);
    goto lB990;
lB28E:
    R[11] = 0xB293;
    gsu_code(0x04b28f, 4); gsu_code(0x04c3b5, 1);
    abi_FxRecordSegment();
lB293:
    R[7] = R[0];
    R[0] = 0x0058;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04b294, 7);
    gsu_stw(R[0], R[7]);
lB29A:
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_code(0x04b29b, 5);
    gsu_stw(FX_HIT, R[0]);
    gsu_code(0x04b2a0, 4);
    gsu_stw((FX_HIT + 2), R[0]);
    gsu_code(0x04b2a4, 4);
    gsu_stw((FX_HIT + 4), R[0]);
    gsu_code(0x04b2a8, 4);
    gsu_stw((FX_SPIN + 2), R[0]);
    gsu_code(0x04b2ac, 4);
    gsu_stw(FX_SPIN, R[0]);
    gsu_code(0x04b2b0, 4);
    gsu_stw((FX_SPIN + 4), R[0]);
    R[0] = 0x004A;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04b2b4, 4);
    R[0] = gsu_ldw(R[0]);
    gsu_code(0x04b2b8, 4);
    R[6] = gsu_ldw(FX_PARAM_SPIN_DAMP);
    gsu_code(0x04b2bc, 1);
    R[0] = gsu_fmult(R[0], R[6]);
    t = !gsu.s;
    gsu_code(0x04b2bd, 2);
    if (t) { gsu_code(0x04b2c1, 1); goto lB2C1; }
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_code(0x04b2bf, 3);
lB2C1:
    R[6] = 0x0078;
    R[6] = gsu_add(R[6], R[0], 0);
    R[0] = 0x001C;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04b2c2, 9);
    R[0] = gsu_ldw(R[0]);
    R[1] = 0x0008;
    R[0] = gsu_and(R[0], R[1]);
    t = !gsu.z;
    gsu_code(0x04b2cb, 6);
    if (t) { gsu_code(0x04b2d6, 1); goto lB2D6; }
    R[0] = 0x003C;
    R[6] = gsu_add(R[6], R[0], 0);
    gsu_code(0x04b2d1, 6);
lB2D6:
    gsu_code(0x04b2d7, 4);
    gsu_stw((FX_HIT + 2), R[6]);
    R[0] = 0x001E;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04b2db, 4);
    R[0] = gsu_ldw(R[0]);
    gsu_code(0x04b2df, 4);
    gsu_stw(FX_CAR_POS, R[0]);
    R[0] = 0x0020;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04b2e3, 4);
    R[0] = gsu_ldw(R[0]);
    gsu_code(0x04b2e7, 4);
    gsu_stw((FX_CAR_POS + 2), R[0]);
    R[0] = 0x0022;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04b2eb, 4);
    R[0] = gsu_ldw(R[0]);
    gsu_code(0x04b2ef, 4);
    gsu_stw((FX_CAR_POS + 4), R[0]);
    R[0] = 0x008A;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04b2f3, 5);
    R[0] = gsu_ldw(R[0]);
    gsu_code(0x04b2f8, 4);
    gsu_stw((FX_CAR_POS + 6), R[0]);
    R[0] = 0x0040;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04b2fc, 4);
    R[0] = gsu_ldw(R[0]);
    R[1] = 0x0100;
    R[2] = 0x086A;
    R[0] = gsu_bic(R[0], R[2]);
    R[0] = gsu_or(R[0], R[1]);
    gsu_code(0x04b300, 10);
    gsu_sbk(R[0]);
    R[2] = 0x0042;
    R[2] = gsu_add(R[2], R[8], 0);
    gsu_code(0x04b30a, 6);
    R[2] = gsu_ldw(R[2]);
    R[1] = 0x0064;
    R[1] = gsu_add(R[1], R[8], 0);
    gsu_code(0x04b310, 6);
    R[1] = gsu_ldw(R[1]);
    R[0] = 0x0008;
    R[0] = gsu_and(R[0], R[1]);
    t = gsu.z;
    gsu_code(0x04b316, 6);
    if (t) { gsu_code(0x04b337, 1); goto lB337; }
    R[0] = 0x007C;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04b31c, 5);
    R[0] = gsu_ldw(R[0]);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.z;
    gsu_code(0x04b321, 4);
    if (t) { gsu_code(0x04b32e, 1); goto lB32E; }
    R[3] = 0x3E80;
    R[7] = 0x3E80;
    t = 1;
    gsu_code(0x04b325, 9);
    gsu_code(0x04b37d, 1);
    if (t) goto lB37D;
lB32E:
    R[3] = 0x7D00;
    R[7] = 0x7D00;
    t = 1;
    gsu_code(0x04b32f, 8);
    gsu_code(0x04b37d, 1);
    if (t) goto lB37D;
lB337:
    R[3] = 0x3E80;
    R[7] = 0x4650;
    R[0] = 0x0040;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04b338, 10);
    R[0] = gsu_ldw(R[0]);
    R[1] = 0x0010;
    R[0] = gsu_and(R[0], R[1]);
    t = gsu.z;
    gsu_code(0x04b342, 6);
    if (t) { gsu_code(0x04b34c, 1); goto lB34C; }
    gsu_code(0x04b348, 4); gsu_code(0x04b37d, 1);
    goto lB37D;
lB34C:
    gsu_code(0x04b34d, 4);
    R[1] = gsu_ldw((FX_CAR_POS + 6));
    R[0] = 0x0004;
    R[0] = gsu_and(R[0], R[1]);
    t = gsu.z;
    gsu_code(0x04b351, 6);
    if (t) { gsu_code(0x04b360, 1); goto lB360; }
    R[3] = 0x445C;
    R[7] = 0x4524;
    t = 1;
    gsu_code(0x04b357, 9);
    gsu_code(0x04b37d, 1);
    if (t) goto lB37D;
lB360:
    R[0] = 0x0001;
    R[0] = gsu_and(R[0], R[1]);
    t = gsu.z;
    gsu_code(0x04b361, 6);
    if (t) { gsu_code(0x04b370, 1); goto lB370; }
    R[3] = 0x3E80;
    R[7] = 0x4074;
    t = 1;
    gsu_code(0x04b367, 9);
    gsu_code(0x04b37d, 1);
    if (t) goto lB37D;
lB370:
    R[0] = 0x0042;
    R[0] = gsu_and(R[0], R[1]);
    t = gsu.z;
    gsu_code(0x04b371, 6);
    if (t) { gsu_code(0x04b37d, 1); goto lB37D; }
    R[3] = 0x4268;
    R[7] = 0x3A98;
    gsu_code(0x04b377, 7);
lB37D:
    gsu_code(0x04b37e, 4);
    gsu_stw(FX_CONTACT_FORCE_SIDE, R[3]);
    gsu_code(0x04b382, 4);
    gsu_stw(FX_CONTACT_FORCE_UP, R[7]);
    R[3] = 0x00F0;
    R[0] = 0x0400;
    R[0] = gsu_and(R[0], R[2]);
    t = gsu.z;
    gsu_code(0x04b386, 9);
    if (t) { gsu_code(0x04b392, 1); goto lB392; }
    R[3] = 0x014A;
    gsu_code(0x04b38f, 4);
lB392:
    R[4] = 0x0040;
    R[4] = gsu_add(R[4], R[8], 0);
    gsu_code(0x04b393, 6);
    R[4] = gsu_ldw(R[4]);
    R[0] = 0x2000;
    R[0] = gsu_and(R[0], R[4]);
    t = !gsu.z;
    gsu_code(0x04b399, 6);
    if (t) { gsu_code(0x04b3d5, 1); goto lB3D5; }
    gsu_code(0x04b39f, 5);
    R[0] = gsu_ldw((FX_CAR_POS + 6));
    R[1] = 0x0080;
    R[0] = gsu_and(R[0], R[1]);
    t = !gsu.z;
    gsu_code(0x04b3a4, 6);
    if (t) { gsu_code(0x04b3ba, 1); goto lB3BA; }
    R[0] = 0x0010;
    R[0] = gsu_and(R[0], R[4]);
    t = !gsu.z;
    gsu_code(0x04b3aa, 7);
    if (t) { gsu_code(0x04b3d5, 1); goto lB3D5; }
    R[0] = 0x0046;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04b3b1, 5);
    R[0] = gsu_ldw(R[0]);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.z;
    gsu_code(0x04b3b6, 4);
    if (t) { gsu_code(0x04b3d5, 1); goto lB3D5; }
    gsu_code(0x04b3ba, 1);
lB3BA:
    R[0] = 0x004A;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04b3bb, 4);
    R[0] = gsu_ldw(R[0]);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = !gsu.s;
    gsu_code(0x04b3bf, 4);
    if (t) { gsu_code(0x04b3c5, 1); goto lB3C5; }
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_code(0x04b3c3, 3);
lB3C5:
    gsu_sub(R[0], R[3], 0);   /* CMP */
    t = gsu.s;
    gsu_code(0x04b3c6, 4);
    if (t) { gsu_code(0x04b3d5, 1); goto lB3D5; }
    R[1] = 0x0040;
    R[2] = gsu_or(R[2], R[1]);
    R[1] = 0x0430;
    R[2] = gsu_bic(R[2], R[1]);
    gsu_code(0x04b3ca, 12);
lB3D5:
    R[0] = 0x0044;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04b3d6, 4);
    R[0] = gsu_ldw(R[0]);
    R[0] = gsu_sub(R[0], 3, 0);
    t = gsu.s;
    gsu_code(0x04b3da, 4);
    if (t) { gsu_code(0x04b3ed, 1); goto lB3ED; }
    R[1] = 0x0430;
    R[2] = gsu_bic(R[2], R[1]);
    R[0] = 0x0040;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04b3de, 11);
    R[0] = gsu_ldw(R[0]);
    R[1] = 0x0008;
    R[0] = gsu_or(R[0], R[1]);
    gsu_code(0x04b3e9, 5);
    gsu_sbk(R[0]);
lB3ED:
    R[0] = 0x0054;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04b3ee, 4);
    R[0] = gsu_ldw(R[0]);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = !gsu.z;
    gsu_code(0x04b3f2, 4);
    if (t) { gsu_code(0x04b401, 1); goto lB401; }
    R[0] = 0x0040;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04b3f6, 5);
    R[0] = gsu_ldw(R[0]);
    R[1] = 0x0004;
    R[0] = gsu_and(R[0], R[1]);
    t = gsu.z;
    gsu_code(0x04b3fb, 6);
    if (t) { gsu_code(0x04b407, 1); goto lB407; }
    gsu_code(0x04b401, 1);
lB401:
    R[1] = 0x043F;
    R[2] = gsu_bic(R[2], R[1]);
    gsu_code(0x04b402, 6);
lB407:
    R[0] = 0x005E;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04b408, 4);
    R[0] = gsu_ldw(R[0]);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.z;
    gsu_code(0x04b40c, 4);
    if (t) { gsu_code(0x04b415, 1); goto lB415; }
    R[0] = gsu_dec(R[0]);
    gsu_code(0x04b410, 3);
    gsu_sbk(R[0]);
    t = 1;
    gsu_code(0x04b413, 2);
    gsu_code(0x04b436, 1);
    if (t) goto lB436;
lB415:
    R[0] = 0x0080;
    R[0] = gsu_and(R[0], R[2]);
    t = gsu.z;
    gsu_code(0x04b416, 6);
    if (t) { gsu_code(0x04b436, 1); goto lB436; }
    gsu_code(0x04b41c, 5);
    R[1] = gsu_ldw(FX_PARAM_LIFT);
    R[1] = R[1];
    gsu_moves(R[1]);
    t = gsu.z;
    gsu_code(0x04b421, 4);
    if (t) { gsu_code(0x04b436, 1); goto lB436; }
    gsu_code(0x04b425, 5);
    R[0] = gsu_ldw((FX_HIT + 2));
    R[0] = gsu_add(R[0], R[1], 0);
    gsu_code(0x04b42a, 5);
    gsu_stw((FX_HIT + 2), R[0]);
    R[1] = 0x0020;
    R[0] = 0x005E;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04b42f, 8);
    gsu_stw(R[0], R[1]);
lB436:
    R[11] = 0xB43B;
    gsu_code(0x04b437, 4); gsu_code(0x04ba05, 1);
    abi_FxSteerFromInput();
lB43B:
    R[1] = 0x0070;
    R[1] = gsu_add(R[1], R[8], 0);
    gsu_code(0x04b43c, 6);
    R[1] = gsu_ldw(R[1]);
    R[0] = 0x0008;
    R[0] = gsu_and(R[0], R[1]);
    t = gsu.z;
    gsu_code(0x04b442, 6);
    if (t) { gsu_code(0x04b44a, 1); goto lB44A; }
    R[3] = gsu_add(R[3], R[3], 0);
    gsu_code(0x04b448, 3);
lB44A:
    R[0] = 0x003A;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04b44b, 5);
    gsu_stw(R[0], R[3]);
    R[1] = 0x0000;
    R[0] = 0x0020;
    R[0] = gsu_and(R[0], R[2]);
    t = gsu.z;
    gsu_code(0x04b450, 8);
    if (t) { gsu_code(0x04b45f, 1); goto lB45F; }
    gsu_code(0x04b458, 5);
    R[1] = gsu_ldw(FX_PARAM_REVERSE);
    R[1] = gsu_not(R[1]);
    R[1] = gsu_inc(R[1]);
    gsu_code(0x04b45d, 3);
lB45F:
    R[0] = 0x0400;
    R[0] = gsu_and(R[0], R[2]);
    t = gsu.z;
    gsu_code(0x04b460, 6);
    if (t) { gsu_code(0x04b4a4, 1); goto lB4A4; }
    gsu_code(0x04b466, 5);
    R[1] = gsu_ldw(FX_PARAM_THRUST_BOOST);
    R[6] = 0x006A;
    R[6] = gsu_add(R[6], R[8], 0);
    gsu_code(0x04b46b, 6);
    R[6] = gsu_ldw(R[6]);
    R[6] = R[6];
    gsu_moves(R[6]);
    t = gsu.z;
    gsu_code(0x04b471, 4);
    if (t) { gsu_code(0x04b4ab, 1); goto lB4AB; }
    R[4] = 0x0040;
    R[4] = gsu_add(R[4], R[8], 0);
    gsu_code(0x04b475, 7);
    R[4] = gsu_ldw(R[4]);
    R[0] = 0x2000;
    R[0] = gsu_and(R[0], R[4]);
    t = !gsu.z;
    gsu_code(0x04b47c, 6);
    if (t) { gsu_code(0x04b49d, 1); goto lB49D; }
    gsu_code(0x04b482, 5);
    R[0] = gsu_ldw(FX_PARAM_SPEED_CAP);
    R[6] = gsu_sub(R[6], interp_boost_cost(R[8], R[0]), 0);
    t = !gsu.s;
    gsu_code(0x04b487, 4);
    if (t) { gsu_code(0x04b48d, 1); goto lB48D; }
    R[6] = 0x0000;
    gsu_code(0x04b48b, 3);
lB48D:
    gsu_code(0x04b48e, 4);
    R[0] = gsu_ldw(GSU_CONTROL_26F2);
    R[1] = 0x0200;
    R[0] = gsu_and(R[0], R[1]);
    t = !gsu.z;
    gsu_code(0x04b492, 6);
    if (t) { gsu_code(0x04b49d, 1); goto lB49D; }
    R[0] = 0x006A;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04b498, 6);
    gsu_stw(R[0], R[6]);
lB49D:
    gsu_code(0x04b49e, 4);
    R[1] = gsu_ldw(FX_PARAM_THRUST);
    t = 1;
    gsu_code(0x04b4a2, 2);
    gsu_code(0x04b4af, 1);
    if (t) goto lB4AF;
lB4A4:
    R[0] = 0x0010;
    R[0] = gsu_and(R[0], R[2]);
    t = gsu.z;
    gsu_code(0x04b4a5, 6);
    if (t) { gsu_code(0x04b4af, 1); goto lB4AF; }
    gsu_code(0x04b4ab, 1);
lB4AB:
    gsu_code(0x04b4ac, 4);
    R[1] = gsu_ldw(FX_PARAM_THRUST_BOOST);
lB4AF:
    R[4] = 0x0040;
    R[4] = gsu_add(R[4], R[8], 0);
    gsu_code(0x04b4b0, 6);
    R[4] = gsu_ldw(R[4]);
    R[0] = 0x2000;
    R[0] = gsu_and(R[0], R[4]);
    t = gsu.z;
    gsu_code(0x04b4b6, 6);
    if (t) { gsu_code(0x04b4c0, 1); goto lB4C0; }
    R[1] = gsu_add(R[1], R[1], 0);
    R[1] = gsu_add(R[1], R[1], 0);
    gsu_code(0x04b4bc, 5);
lB4C0:
    R[0] = 0x0078;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04b4c1, 5);
    gsu_stw(R[0], R[1]);
    R[0] = 0x0009;
    gsu_code(0x04b4c6, 5);
    gsu_romb(R[0]);
    gsu_code(0x04b4cb, 4);
    gsu_r14(gsu_ldw(FX_PARAM_CURVE_A));
    R[4] = 0x0001;
    R[5] = 0x0000;
    R[3] = 0x004A;
    R[3] = gsu_add(R[3], R[8], 0);
    gsu_code(0x04b4cf, 10);
    R[3] = gsu_ldw(R[3]);
    R[3] = R[3];
    gsu_moves(R[3]);
    t = !gsu.s;
    gsu_code(0x04b4d9, 4);
    if (t) { gsu_code(0x04b4f1, 1); goto lB4F1; }
    R[0] = 0x0410;
    R[0] = gsu_and(R[0], R[2]);
    t = gsu.z;
    gsu_code(0x04b4dd, 7);
    if (t) { gsu_code(0x04b4ec, 1); goto lB4EC; }
    R[4] = 0x0001;
    R[5] = 0x000F;
    gsu_code(0x04b4e4, 8); gsu_code(0x04b562, 1);
    goto lB562;
lB4EC:
    R[4] = 0x0000;
    t = 1;
    gsu_code(0x04b4ed, 4);
    gsu_code(0x04b506, 1);
    if (t) goto lB506;
lB4F1:
    gsu_code(0x04b4f2, 1);
    R[0] = gsu_getb();
    gsu_code(0x04b4f3, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x04b4f4, 2);
    R[0] = gsu_getbh(R[0]);
    gsu_code(0x04b4f6, 1);
    gsu_r14(gsu_inc(R[14]));
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.z;
    gsu_code(0x04b4f7, 4);
    if (t) { gsu_code(0x04b505, 1); goto lB505; }
    gsu_sub(R[0], R[3], 0);   /* CMP */
    t = !gsu.s;
    gsu_code(0x04b4fb, 5);
    if (t) { gsu_code(0x04b506, 1); goto lB506; }
    R[5] = R[0];
    t = 1;
    R[4] = gsu_inc(R[4]);
    gsu_code(0x04b500, 5);
    gsu_code(0x04b4f1, 1);
    if (t) goto lB4F1;
lB505:
    R[4] = gsu_dec(R[4]);
    gsu_code(0x04b506, 1);
lB506:
    gsu_code(0x04b507, 4);
    R[0] = gsu_ldw(FX_INIT_2470);
    R[0] = gsu_and(R[0], 3);
    t = !gsu.z;
    gsu_code(0x04b50b, 4);
    if (t) { gsu_code(0x04b529, 1); goto lB529; }
    R[0] = 0x004E;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04b50f, 5);
    R[0] = gsu_ldw(R[0]);
    gsu_sub(R[0], R[4], 0);   /* CMP */
    t = gsu.z;
    gsu_code(0x04b514, 4);
    if (t) { gsu_code(0x04b529, 1); goto lB529; }
    gsu_code(0x04b518, 3);
    gsu_sbk(R[4]);
    R[0] = gsu_sub(R[4], 4, 0);
    t = !gsu.z;
    gsu_code(0x04b51b, 5);
    if (t) { gsu_code(0x04b529, 1); goto lB529; }
    R[0] = 0x0040;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04b520, 5);
    R[0] = gsu_ldw(R[0]);
    R[6] = 0x0002;
    R[0] = gsu_or(R[0], R[6]);
    gsu_code(0x04b525, 5);
    gsu_sbk(R[0]);
lB529:
    gsu_code(0x04b52a, 4);
    gsu_r14(gsu_ldw(FX_PARAM_CURVE_B));
    R[0] = 0x004E;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04b52e, 4);
    R[0] = gsu_ldw(R[0]);
    R[0] = gsu_dec(R[0]);
    R[6] = 0x0011;
    gsu_code(0x04b532, 5);
    R[0] = gsu_mult(R[0], R[6]);
    gsu_code(0x04b537, 2);
    gsu_r14(gsu_add(R[14], R[0], 0));
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_code(0x04b539, 4);
    R[6] = gsu_getbh(R[0]);
    R[4] = R[6];
    gsu_code(0x04b53d, 3);
    gsu_r14(gsu_inc(R[14]));
    R[0] = 0x004A;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04b540, 4);
    R[0] = gsu_ldw(R[0]);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = !gsu.s;
    gsu_code(0x04b544, 4);
    if (t) { gsu_code(0x04b54a, 1); goto lB54A; }
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_code(0x04b548, 3);
lB54A:
    R[0] = gsu_sub(R[0], R[5], 0);
    R[0] = gsu_add(R[0], R[0], 0);
    gsu_code(0x04b54b, 3);
    R[0] = gsu_fmult(R[0], R[6]);
    R[3] = 0x000F;
    gsu_sub(R[0], R[3], 0);   /* CMP */
    t = gsu.s;
    gsu_code(0x04b54e, 6);
    if (t) { gsu_code(0x04b556, 1); goto lB556; }
    R[0] = R[3];
    gsu_code(0x04b554, 3);
lB556:
    R[5] = R[0];
    gsu_code(0x04b557, 4);
    gsu_r14(gsu_add(R[14], R[0], 0));
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_code(0x04b55b, 4);
    R[6] = gsu_getbh(R[0]);
    R[1] = gsu_add(R[1], R[1], 0);
    gsu_code(0x04b55f, 4);
    R[1] = gsu_fmult(R[1], R[6]);
lB562:
    R[0] = 0x003C;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04b563, 5);
    gsu_stw(R[0], R[1]);
    R[1] = 0x0050;
    R[1] = gsu_add(R[1], R[8], 0);
    gsu_code(0x04b568, 6);
    R[1] = gsu_ldw(R[1]);
    R[0] = R[5];
    R[6] = 0x0040;
    gsu_code(0x04b56e, 6);
    { uint32_t p = gsu_lmult(R[0], R[6]); R[4] = (uint16_t)p; R[0] = (uint16_t)(p >> 16); }
    R[0] = gsu_sub(R[4], R[1], 0);
    R[0] = gsu_div2(R[0]);
    R[0] = gsu_div2(R[0]);
    R[0] = gsu_add(R[0], R[1], 0);
    gsu_code(0x04b574, 8);
    gsu_sbk(R[0]);
    R[0] = 0x0800;
    R[0] = gsu_and(R[0], R[2]);
    t = gsu.z;
    gsu_code(0x04b57c, 6);
    if (t) { gsu_code(0x04b5ae, 1); goto lB5AE; }
    gsu_code(0x04b582, 3);
    gsu_stw(R[10], R[2]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[4] = 0x0000;
    R[5] = 0x0000;
    R[6] = 0x0258;
    R[11] = 0xB594;
    gsu_code(0x04b585, 15); gsu_code(0x04973f, 1);
    abi_FxSumsPushLocal();
lB594:
    R[11] = 0xB599;
    gsu_code(0x04b595, 4); gsu_code(0x0497bb, 1);
    abi_FxSumsRotateLocal();
lB599:
    R[1] = 0x0000;
    R[2] = 0x0000;
    R[3] = 0x0000;
    R[6] = 0x7FFF;
    R[11] = 0xB5AA;
    gsu_code(0x04b59a, 16); gsu_code(0x04987d, 1);
    abi_FxHitAddScaled();
lB5AA:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x04b5ab, 4);
    R[2] = gsu_ldw(R[10]);
lB5AE:
    R[1] = 0x0000;
    R[0] = 0x0410;
    R[0] = gsu_and(R[0], R[2]);
    t = !gsu.z;
    gsu_code(0x04b5af, 9);
    if (t) { gsu_code(0x04b5d6, 1); goto lB5D6; }
    R[0] = 0x004A;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04b5b8, 5);
    R[0] = gsu_ldw(R[0]);
    R[0] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_add(R[0], R[0], 0);
    t = !gsu.s;
    gsu_code(0x04b5bd, 5);
    if (t) { gsu_code(0x04b5c4, 1); goto lB5C4; }
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_code(0x04b5c2, 3);
lB5C4:
    gsu_code(0x04b5c5, 4);
    R[6] = gsu_ldw(FX_PARAM_TURN_GRIP);
    R[0] = gsu_add(R[0], R[0], 0);
    gsu_code(0x04b5c9, 2);
    R[0] = gsu_fmult(R[0], R[6]);
    R[1] = 0x0800;
    gsu_sub(R[0], R[1], 0);   /* CMP */
    t = !gsu.s;
    gsu_code(0x04b5cb, 7);
    if (t) { gsu_code(0x04b5d4, 1); goto lB5D4; }
    R[0] = R[1];
    gsu_code(0x04b5d2, 3);
lB5D4:
    R[1] = R[0];
    gsu_code(0x04b5d5, 2);
lB5D6:
    R[0] = 0x0040;
    R[0] = gsu_and(R[0], R[2]);
    t = gsu.z;
    gsu_code(0x04b5d7, 6);
    if (t) { gsu_code(0x04b5ff, 1); goto lB5FF; }
    R[0] = 0x004A;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04b5dd, 5);
    R[0] = gsu_ldw(R[0]);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.s;
    gsu_code(0x04b5e2, 4);
    if (t) { gsu_code(0x04b5ff, 1); goto lB5FF; }
    R[3] = R[0];
    R[0] = 0x00C8;
    gsu_sub(R[0], R[3], 0);   /* CMP */
    t = !gsu.s;
    gsu_code(0x04b5e6, 10);
    if (t) { gsu_code(0x04b5fb, 1); goto lB5FB; }
    R[1] = 0x0040;
    R[1] = gsu_add(R[1], R[8], 0);
    gsu_code(0x04b5f0, 7);
    R[1] = gsu_ldw(R[1]);
    R[0] = 0x0800;
    R[0] = gsu_or(R[0], R[1]);
    gsu_code(0x04b5f7, 5);
    gsu_sbk(R[0]);
lB5FB:
    gsu_code(0x04b5fc, 4);
    R[1] = gsu_ldw(FX_PARAM_WHEEL_48);
lB5FF:
    R[0] = 0x0048;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04b600, 5);
    gsu_stw(R[0], R[1]);
    R[0] = 0x0042;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04b605, 5);
    gsu_stw(R[0], R[2]);
    R[1] = 0x0040;
    R[0] = 0x002C;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04b60a, 8);
    gsu_stw(R[0], R[1]);
    R[11] = 0xB616;
    gsu_code(0x04b612, 4); gsu_code(0x04a4ed, 1);
    abi_FxCarMoveParts();
lB616:
    R[11] = 0xB61B;
    gsu_code(0x04b617, 4); gsu_code(0x04a607, 1);
    goto lA607;
lB61B:
    R[11] = 0xB620;
    gsu_code(0x04b61c, 4); gsu_code(0x04bc7b, 1);
    abi_FxRecordHoldY();
lB620:
    R[2] = 0x0042;
    R[2] = gsu_add(R[2], R[8], 0);
    gsu_code(0x04b621, 6);
    R[2] = gsu_ldw(R[2]);
    R[0] = 0x0400;
    R[0] = gsu_and(R[0], R[2]);
    t = gsu.z;
    gsu_code(0x04b627, 6);
    if (t) { gsu_code(0x04b653, 1); goto lB653; }
    gsu_code(0x04b62d, 5);
    R[2] = gsu_ldw(FX_PARAM_JUMP);
    R[2] = R[2];
    gsu_moves(R[2]);
    t = gsu.z;
    gsu_code(0x04b632, 4);
    if (t) { gsu_code(0x04b653, 1); goto lB653; }
    R[0] = 0x004E;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04b636, 5);
    R[0] = gsu_ldw(R[0]);
    R[1] = 0x0002;
    gsu_sub(R[0], R[1], 0);   /* CMP */
    t = !gsu.s;
    gsu_code(0x04b63b, 6);
    if (t) { gsu_code(0x04b653, 1); goto lB653; }
    R[0] = 0x0026;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04b641, 5);
    R[0] = gsu_ldw(R[0]);
    R[1] = 0xEAAB;
    gsu_sub(R[0], R[1], 0);   /* CMP */
    t = gsu.s;
    gsu_code(0x04b646, 7);
    if (t) { gsu_code(0x04b653, 1); goto lB653; }
    gsu_code(0x04b64d, 5);
    R[0] = gsu_ldw((FX_SPIN + 2));
    R[0] = gsu_sub(R[0], R[2], 0);
    gsu_code(0x04b652, 2);
    gsu_sbk(R[0]);
lB653:
    R[11] = 0xB658;
    gsu_code(0x04b654, 4); gsu_code(0x04bd20, 1);
    goto lBD20;
lB658:
    R[11] = 0xB65D;
    gsu_code(0x04b659, 4); gsu_code(0x04bcba, 1);
    abi_FxRecordAlignment();
lB65D:
    R[11] = 0xB662;
    gsu_code(0x04b65e, 4); gsu_code(0x04bb17, 1);
    abi_FxCarHit();
lB662:
    R[1] = 0x0070;
    R[1] = gsu_add(R[1], R[8], 0);
    gsu_code(0x04b663, 6);
    R[1] = gsu_ldw(R[1]);
    R[0] = 0x0080;
    R[0] = gsu_and(R[0], R[1]);
    t = gsu.z;
    gsu_code(0x04b669, 6);
    if (t) { gsu_code(0x04b6c1, 1); goto lB6C1; }
    R[0] = 0x0058;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04b66f, 5);
    R[0] = gsu_ldw(R[0]);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = !gsu.z;
    gsu_code(0x04b674, 4);
    if (t) { gsu_code(0x04b67c, 1); goto lB67C; }
    gsu_code(0x04b678, 4); gsu_code(0x04b6b7, 1);
    goto lB6B7;
lB67C:
    R[1] = 0x008A;
    R[1] = gsu_add(R[1], R[8], 0);
    gsu_code(0x04b67d, 7);
    R[1] = gsu_ldw(R[1]);
    R[0] = 0x0100;
    R[0] = gsu_and(R[0], R[1]);
    t = !gsu.z;
    gsu_code(0x04b684, 6);
    if (t) { gsu_code(0x04b68e, 1); goto lB68E; }
    gsu_code(0x04b68a, 4); gsu_code(0x04b6b7, 1);
    goto lB6B7;
lB68E:
    R[11] = 0xB693;
    gsu_code(0x04b68f, 4); gsu_code(0x04c378, 1);
    abi_FxRecordHeadingAhead();
lB693:
    R[2] = R[0];
    R[1] = 0x0024;
    R[1] = gsu_add(R[1], R[8], 0);
    gsu_code(0x04b694, 8);
    R[1] = gsu_ldw(R[1]);
    R[0] = gsu_sub(R[0], R[1], 0);
    R[0] = gsu_div2(R[0]);
    R[0] = gsu_div2(R[0]);
    R[0] = gsu_add(R[0], R[1], 0);
    gsu_code(0x04b69c, 7);
    gsu_sbk(R[0]);
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_code(0x04b6a3, 5);
    gsu_stw(FX_SPIN, R[0]);
    R[3] = R[0];
    R[0] = 0x0034;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04b6a8, 7);
    gsu_stw(R[0], R[3]);
    R[0] = gsu_sub(R[1], R[2], 0);
    R[0] = gsu_hib(R[0]);
    R[0] = gsu_sex(R[0]);
    R[0] = gsu_div2(R[0]);
    t = !gsu.z;
    gsu_code(0x04b6af, 8);
    if (t) { gsu_code(0x04b6c1, 1); goto lB6C1; }
    gsu_code(0x04b6b7, 1);
lB6B7:
    R[1] = 0x0080;
    R[0] = 0x0070;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04b6b8, 7);
    R[0] = gsu_ldw(R[0]);
    R[0] = gsu_bic(R[0], R[1]);
    gsu_code(0x04b6bf, 3);
    gsu_sbk(R[0]);
lB6C1:
    R[1] = 0xF448;
    R[2] = 0x0BB8;
    gsu_code(0x04b6c2, 10);
    R[0] = gsu_ldw((FX_HIT + 2));
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.s;
    gsu_code(0x04b6cc, 4);
    if (t) { gsu_code(0x04b6da, 1); goto lB6DA; }
    gsu_sub(R[0], R[2], 0);   /* CMP */
    t = gsu.s;
    gsu_code(0x04b6d0, 5);
    if (t) { gsu_code(0x04b6e1, 1); goto lB6E1; }
    R[0] = R[2];
    t = 1;
    gsu_code(0x04b6d5, 5);
    gsu_code(0x04b6e1, 1);
    if (t) goto lB6E1;
lB6DA:
    gsu_sub(R[0], R[1], 0);   /* CMP */
    t = !gsu.s;
    gsu_code(0x04b6db, 4);
    if (t) { gsu_code(0x04b6e1, 1); goto lB6E1; }
    R[0] = R[1];
    gsu_code(0x04b6df, 3);
lB6E1:
    gsu_code(0x04b6e2, 1);
    gsu_sbk(R[0]);
    gsu_code(0x04b6e3, 4);
    R[0] = gsu_ldw((FX_SPIN + 4));
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.s;
    gsu_code(0x04b6e7, 4);
    if (t) { gsu_code(0x04b6f5, 1); goto lB6F5; }
    gsu_sub(R[0], R[2], 0);   /* CMP */
    t = gsu.s;
    gsu_code(0x04b6eb, 5);
    if (t) { gsu_code(0x04b6fc, 1); goto lB6FC; }
    R[0] = R[2];
    t = 1;
    gsu_code(0x04b6f0, 5);
    gsu_code(0x04b6fc, 1);
    if (t) goto lB6FC;
lB6F5:
    gsu_sub(R[0], R[1], 0);   /* CMP */
    t = !gsu.s;
    gsu_code(0x04b6f6, 4);
    if (t) { gsu_code(0x04b6fc, 1); goto lB6FC; }
    R[0] = R[1];
    gsu_code(0x04b6fa, 3);
lB6FC:
    gsu_code(0x04b6fd, 1);
    gsu_sbk(R[0]);
    gsu_code(0x04b6fe, 4);
    R[0] = gsu_ldw((FX_SPIN + 2));
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.s;
    gsu_code(0x04b702, 4);
    if (t) { gsu_code(0x04b710, 1); goto lB710; }
    gsu_sub(R[0], R[2], 0);   /* CMP */
    t = gsu.s;
    gsu_code(0x04b706, 5);
    if (t) { gsu_code(0x04b717, 1); goto lB717; }
    R[0] = R[2];
    t = 1;
    gsu_code(0x04b70b, 5);
    gsu_code(0x04b717, 1);
    if (t) goto lB717;
lB710:
    gsu_sub(R[0], R[1], 0);   /* CMP */
    t = !gsu.s;
    gsu_code(0x04b711, 4);
    if (t) { gsu_code(0x04b717, 1); goto lB717; }
    R[0] = R[1];
    gsu_code(0x04b715, 3);
lB717:
    gsu_code(0x04b718, 1);
    gsu_sbk(R[0]);
    gsu_code(0x04b719, 4);
    R[0] = gsu_ldw(FX_SPIN);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.s;
    gsu_code(0x04b71d, 4);
    if (t) { gsu_code(0x04b72b, 1); goto lB72B; }
    gsu_sub(R[0], R[2], 0);   /* CMP */
    t = gsu.s;
    gsu_code(0x04b721, 5);
    if (t) { gsu_code(0x04b732, 1); goto lB732; }
    R[0] = R[2];
    t = 1;
    gsu_code(0x04b726, 5);
    gsu_code(0x04b732, 1);
    if (t) goto lB732;
lB72B:
    gsu_sub(R[0], R[1], 0);   /* CMP */
    t = !gsu.s;
    gsu_code(0x04b72c, 4);
    if (t) { gsu_code(0x04b732, 1); goto lB732; }
    R[0] = R[1];
    gsu_code(0x04b730, 3);
lB732:
    gsu_code(0x04b733, 1);
    gsu_sbk(R[0]);
    R[2] = 0x0000;
    R[0] = 0x0028;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04b734, 6);
    R[0] = gsu_ldw(R[0]);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.s;
    gsu_code(0x04b73a, 4);
    if (t) { gsu_code(0x04b750, 1); goto lB750; }
    R[1] = 0x2000;
    gsu_sub(R[0], R[1], 0);   /* CMP */
    t = gsu.s;
    gsu_code(0x04b73e, 8);
    if (t) { gsu_code(0x04b75f, 1); goto lB75F; }
    gsu_code(0x04b746, 3);
    gsu_sbk(R[1]);
    R[0] = 0x0038;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04b749, 5);
    gsu_stw(R[0], R[2]);
    t = 1;
    gsu_code(0x04b74e, 2);
    gsu_code(0x04b75f, 1);
    if (t) goto lB75F;
lB750:
    R[1] = 0xE000;
    gsu_sub(R[0], R[1], 0);   /* CMP */
    t = !gsu.s;
    gsu_code(0x04b751, 7);
    if (t) { gsu_code(0x04b75f, 1); goto lB75F; }
    gsu_code(0x04b758, 3);
    gsu_sbk(R[1]);
    R[0] = 0x0038;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04b75b, 5);
    gsu_stw(R[0], R[2]);
lB75F:
    R[1] = 0x0040;
    R[1] = gsu_add(R[1], R[8], 0);
    gsu_code(0x04b760, 6);
    R[1] = gsu_ldw(R[1]);
    R[0] = 0x0000;
    R[0] = gsu_and(R[0], R[1]);
    t = !gsu.z;
    gsu_code(0x04b766, 6);
    if (t) { gsu_code(0x04b799, 1); goto lB799; }
    R[0] = 0x0026;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04b76c, 5);
    R[0] = gsu_ldw(R[0]);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.s;
    gsu_code(0x04b771, 4);
    if (t) { gsu_code(0x04b787, 1); goto lB787; }
    R[1] = 0x2000;
    gsu_sub(R[0], R[1], 0);   /* CMP */
    t = gsu.s;
    gsu_code(0x04b775, 8);
    if (t) { gsu_code(0x04b7ca, 1); goto lB7CA; }
    gsu_code(0x04b77d, 3);
    gsu_sbk(R[1]);
    R[0] = 0x0036;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04b780, 5);
    gsu_stw(R[0], R[2]);
    t = 1;
    gsu_code(0x04b785, 2);
    gsu_code(0x04b7ca, 1);
    if (t) goto lB7CA;
lB787:
    R[1] = 0xE000;
    gsu_sub(R[0], R[1], 0);   /* CMP */
    t = !gsu.s;
    gsu_code(0x04b788, 7);
    if (t) { gsu_code(0x04b7ca, 1); goto lB7CA; }
    gsu_code(0x04b78f, 3);
    gsu_sbk(R[1]);
    R[0] = 0x0036;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04b792, 5);
    gsu_stw(R[0], R[2]);
    t = 1;
    gsu_code(0x04b797, 2);
    gsu_code(0x04b7ca, 1);
    if (t) goto lB7CA;
lB799:
    R[0] = 0x0026;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04b79a, 4);
    R[0] = gsu_ldw(R[0]);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = !gsu.s;
    gsu_code(0x04b79e, 4);
    if (t) { gsu_code(0x04b7a4, 1); goto lB7A4; }
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_code(0x04b7a2, 3);
lB7A4:
    R[1] = 0x4000;
    gsu_sub(R[0], R[1], 0);   /* CMP */
    t = gsu.s;
    gsu_code(0x04b7a5, 7);
    if (t) { gsu_code(0x04b7ca, 1); goto lB7CA; }
    R[0] = 0x0040;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04b7ac, 5);
    R[0] = gsu_ldw(R[0]);
    R[1] = 0x0000;
    R[0] = gsu_or(R[0], R[1]);
    gsu_code(0x04b7b1, 5);
    gsu_sbk(R[0]);
    R[0] = 0x0076;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04b7b6, 4);
    R[0] = gsu_ldw(R[0]);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = !gsu.z;
    gsu_code(0x04b7ba, 4);
    if (t) { gsu_code(0x04b7c1, 1); goto lB7C1; }
    R[0] = 0x0028;
    gsu_code(0x04b7be, 4);
    gsu_sbk(R[0]);
lB7C1:
    gsu_code(0x04b7c2, 4);
    R[0] = gsu_ldw(FX_SPIN);
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
    t = 1;
    gsu_code(0x04b7c6, 4); gsu_code(0x04b7d4, 1);
    gsu_sbk(R[0]);
    if (t) goto lB7D4;
lB7CA:
    R[0] = 0x0040;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04b7cb, 4);
    R[0] = gsu_ldw(R[0]);
    R[1] = 0x0000;
    R[0] = gsu_bic(R[0], R[1]);
    gsu_code(0x04b7cf, 6);
    gsu_sbk(R[0]);
lB7D4:
    R[0] = 0x004A;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04b7d5, 4);
    R[0] = gsu_ldw(R[0]);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = !gsu.s;
    gsu_code(0x04b7d9, 4);
    if (t) { gsu_code(0x04b7df, 1); goto lB7DF; }
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_code(0x04b7dd, 3);
lB7DF:
    R[1] = 0x0258;
    gsu_sub(R[0], R[1], 0);   /* CMP */
    t = gsu.s;
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_code(0x04b7e0, 7);
    if (t) { gsu_code(0x04b80f, 1); gsu_cache(0xb810); goto lB80F; }
    gsu_code(0x04b7e7, 5);
    gsu_stw(FX_HIT, R[0]);
    gsu_code(0x04b7ec, 4);
    gsu_stw((FX_HIT + 4), R[0]);
    R[1] = 0x002E;
    R[1] = gsu_add(R[1], R[8], 0);
    gsu_code(0x04b7f0, 6);
    R[1] = gsu_ldw(R[1]);
    R[0] = gsu_div2(R[1]);
    R[0] = gsu_div2(R[0]);
    R[0] = gsu_div2(R[0]);
    R[0] = gsu_sub(R[1], R[0], 0);
    gsu_code(0x04b7f6, 10);
    gsu_sbk(R[0]);
    R[1] = 0x0032;
    R[1] = gsu_add(R[1], R[8], 0);
    gsu_code(0x04b800, 6);
    R[1] = gsu_ldw(R[1]);
    R[0] = gsu_div2(R[1]);
    R[0] = gsu_div2(R[0]);
    R[0] = gsu_div2(R[0]);
    R[0] = gsu_sub(R[1], R[0], 0);
    gsu_code(0x04b806, 10); gsu_cache(0xb810);
    gsu_sbk(R[0]);
lB80F:
    /* CACHE */
    R[4] = 0x2334;
    R[2] = 0x002E;
    R[2] = gsu_add(R[2], R[8], 0);
    R[12] = 0x0006;
    R[13] = 0xB81C;
    gsu_code(0x04b810, 13);
lB81C:
    gsu_code(0x04b81d, 1);
    R[0] = gsu_ldw(R[4]);
    R[4] = gsu_inc(R[4]);
    R[4] = gsu_inc(R[4]);
    gsu_code(0x04b81e, 4);
    R[3] = gsu_ldw(R[2]);
    R[0] = gsu_add(R[0], R[3], 0);
    gsu_code(0x04b822, 2);
    gsu_stw(R[2], R[0]);
    R[2] = gsu_inc(R[2]);
    R[2] = gsu_inc(R[2]);
    t = gsu_count_down(&R[12]);
    gsu_code(0x04b824, 3);
    if (t) { gsu_code(0x04b81c, 1); goto lB81C; }
    R[2] = 0x0064;
    R[2] = gsu_add(R[2], R[8], 0);
    gsu_code(0x04b827, 7);
    R[2] = gsu_ldw(R[2]);
    R[0] = 0x0008;
    R[0] = gsu_and(R[0], R[2]);
    t = gsu.z;
    gsu_code(0x04b82e, 6);
    if (t) { gsu_code(0x04b838, 1); goto lB838; }
    gsu_code(0x04b834, 4); gsu_code(0x04b920, 1);
    goto lB920;
lB838:
    R[2] = 0x0042;
    R[2] = gsu_add(R[2], R[8], 0);
    gsu_code(0x04b839, 6);
    R[2] = gsu_ldw(R[2]);
    R[0] = 0x0430;
    R[0] = gsu_and(R[0], R[2]);
    t = gsu.z;
    gsu_code(0x04b83f, 6);
    if (t) { gsu_code(0x04b849, 1); goto lB849; }
    gsu_code(0x04b845, 4); gsu_code(0x04b8bd, 1);
    goto lB8BD;
lB849:
    R[2] = 0x002C;
    R[2] = gsu_add(R[2], R[8], 0);
    gsu_code(0x04b84a, 6);
    R[2] = gsu_ldw(R[2]);
    R[0] = 0x0400;
    R[0] = gsu_and(R[0], R[2]);
    t = gsu.z;
    gsu_code(0x04b850, 6);
    if (t) { gsu_code(0x04b85a, 1); goto lB85A; }
    gsu_code(0x04b856, 4); gsu_code(0x04b8bd, 1);
    goto lB8BD;
lB85A:
    R[0] = 0x002E;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04b85b, 4);
    R[0] = gsu_ldw(R[0]);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.s;
    gsu_code(0x04b85f, 4);
    if (t) { gsu_code(0x04b870, 1); goto lB870; }
    R[2] = 0x00C8;
    gsu_sub(R[0], R[2], 0);   /* CMP */
    t = !gsu.s;
    gsu_code(0x04b863, 8);
    if (t) { gsu_code(0x04b87a, 1); goto lB87A; }
    R[0] = 0x0000;
    t = 1;
    gsu_code(0x04b86b, 5);
    gsu_code(0x04b87a, 1);
    if (t) goto lB87A;
lB870:
    R[2] = 0xFF38;
    gsu_sub(R[0], R[2], 0);   /* CMP */
    t = gsu.s;
    gsu_code(0x04b871, 7);
    if (t) { gsu_code(0x04b87a, 1); goto lB87A; }
    R[0] = 0x0000;
    gsu_code(0x04b878, 3);
lB87A:
    gsu_code(0x04b87b, 1);
    gsu_sbk(R[0]);
    R[0] = 0x0032;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04b87c, 4);
    R[0] = gsu_ldw(R[0]);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.s;
    gsu_code(0x04b880, 4);
    if (t) { gsu_code(0x04b891, 1); goto lB891; }
    R[2] = 0x00C8;
    gsu_sub(R[0], R[2], 0);   /* CMP */
    t = !gsu.s;
    gsu_code(0x04b884, 8);
    if (t) { gsu_code(0x04b89b, 1); goto lB89B; }
    R[0] = 0x0000;
    t = 1;
    gsu_code(0x04b88c, 5);
    gsu_code(0x04b89b, 1);
    if (t) goto lB89B;
lB891:
    R[2] = 0xFF38;
    gsu_sub(R[0], R[2], 0);   /* CMP */
    t = gsu.s;
    gsu_code(0x04b892, 7);
    if (t) { gsu_code(0x04b89b, 1); goto lB89B; }
    R[0] = 0x0000;
    gsu_code(0x04b899, 3);
lB89B:
    gsu_code(0x04b89c, 1);
    gsu_sbk(R[0]);
    R[0] = 0x0030;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04b89d, 4);
    R[0] = gsu_ldw(R[0]);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.s;
    gsu_code(0x04b8a1, 4);
    if (t) { gsu_code(0x04b8b2, 1); goto lB8B2; }
    R[2] = 0x0032;
    gsu_sub(R[0], R[2], 0);   /* CMP */
    t = !gsu.s;
    gsu_code(0x04b8a5, 8);
    if (t) { gsu_code(0x04b8bc, 1); goto lB8BC; }
    R[0] = 0x0000;
    t = 1;
    gsu_code(0x04b8ad, 5);
    gsu_code(0x04b8bc, 1);
    if (t) goto lB8BC;
lB8B2:
    R[2] = 0xFFCE;
    gsu_sub(R[0], R[2], 0);   /* CMP */
    t = gsu.s;
    gsu_code(0x04b8b3, 7);
    if (t) { gsu_code(0x04b8bc, 1); goto lB8BC; }
    R[0] = 0x0000;
    gsu_code(0x04b8ba, 3);
lB8BC:
    gsu_code(0x04b8bd, 1);
    gsu_sbk(R[0]);
lB8BD:
    R[1] = 0x0034;
    R[1] = gsu_add(R[1], R[8], 0);
    gsu_code(0x04b8be, 6);
    R[0] = gsu_ldw(R[1]);
    R[1] = gsu_inc(R[1]);
    R[1] = gsu_inc(R[1]);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.s;
    gsu_code(0x04b8c4, 6);
    if (t) { gsu_code(0x04b8d7, 1); goto lB8D7; }
    R[2] = 0x0040;
    gsu_sub(R[0], R[2], 0);   /* CMP */
    t = !gsu.s;
    gsu_code(0x04b8ca, 8);
    if (t) { gsu_code(0x04b8e1, 1); goto lB8E1; }
    R[0] = 0x0000;
    t = 1;
    gsu_code(0x04b8d2, 5);
    gsu_code(0x04b8e1, 1);
    if (t) goto lB8E1;
lB8D7:
    R[2] = 0xFFC0;
    gsu_sub(R[0], R[2], 0);   /* CMP */
    t = gsu.s;
    gsu_code(0x04b8d8, 7);
    if (t) { gsu_code(0x04b8e1, 1); goto lB8E1; }
    R[0] = 0x0000;
    gsu_code(0x04b8df, 3);
lB8E1:
    gsu_code(0x04b8e2, 1);
    gsu_sbk(R[0]);
    gsu_code(0x04b8e3, 1);
    R[0] = gsu_ldw(R[1]);
    R[1] = gsu_inc(R[1]);
    R[1] = gsu_inc(R[1]);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.s;
    gsu_code(0x04b8e4, 6);
    if (t) { gsu_code(0x04b8f7, 1); goto lB8F7; }
    R[2] = 0x0040;
    gsu_sub(R[0], R[2], 0);   /* CMP */
    t = !gsu.s;
    gsu_code(0x04b8ea, 8);
    if (t) { gsu_code(0x04b901, 1); goto lB901; }
    R[0] = 0x0000;
    t = 1;
    gsu_code(0x04b8f2, 5);
    gsu_code(0x04b901, 1);
    if (t) goto lB901;
lB8F7:
    R[2] = 0xFFC0;
    gsu_sub(R[0], R[2], 0);   /* CMP */
    t = gsu.s;
    gsu_code(0x04b8f8, 7);
    if (t) { gsu_code(0x04b901, 1); goto lB901; }
    R[0] = 0x0000;
    gsu_code(0x04b8ff, 3);
lB901:
    gsu_code(0x04b902, 1);
    gsu_sbk(R[0]);
    gsu_code(0x04b903, 1);
    R[0] = gsu_ldw(R[1]);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.s;
    gsu_code(0x04b904, 4);
    if (t) { gsu_code(0x04b915, 1); goto lB915; }
    R[2] = 0x0040;
    gsu_sub(R[0], R[2], 0);   /* CMP */
    t = !gsu.s;
    gsu_code(0x04b908, 8);
    if (t) { gsu_code(0x04b91f, 1); goto lB91F; }
    R[0] = 0x0000;
    t = 1;
    gsu_code(0x04b910, 5);
    gsu_code(0x04b91f, 1);
    if (t) goto lB91F;
lB915:
    R[2] = 0xFFC0;
    gsu_sub(R[0], R[2], 0);   /* CMP */
    t = gsu.s;
    gsu_code(0x04b916, 7);
    if (t) { gsu_code(0x04b91f, 1); goto lB91F; }
    R[0] = 0x0000;
    gsu_code(0x04b91d, 3);
lB91F:
    gsu_code(0x04b920, 1);
    gsu_sbk(R[0]);
lB920:
    R[1] = 0x002E;
    R[1] = gsu_add(R[1], R[8], 0);
    gsu_code(0x04b921, 6);
    R[1] = gsu_ldw(R[1]);
    R[2] = 0x0030;
    R[2] = gsu_add(R[2], R[8], 0);
    gsu_code(0x04b927, 6);
    R[2] = gsu_ldw(R[2]);
    R[3] = 0x0032;
    R[3] = gsu_add(R[3], R[8], 0);
    gsu_code(0x04b92d, 6);
    R[3] = gsu_ldw(R[3]);
    gsu_code(0x04b933, 3);
    R[6] = gsu_ldw(FX_OBJECT_MATRIX_6);
    gsu_code(0x04b936, 2);
    R[0] = gsu_fmult(R[1], R[6]);
    R[5] = gsu_rol(R[0]);
    gsu_code(0x04b938, 5);
    R[6] = gsu_ldw(FX_OBJECT_MATRIX_7);
    gsu_code(0x04b93d, 2);
    R[0] = gsu_fmult(R[2], R[6]);
    R[0] = gsu_rol(R[0]);
    R[5] = gsu_add(R[5], R[0], 0);
    gsu_code(0x04b93f, 6);
    R[6] = gsu_ldw(FX_OBJECT_MATRIX_8);
    gsu_code(0x04b945, 2);
    R[0] = gsu_fmult(R[3], R[6]);
    R[0] = gsu_rol(R[0]);
    R[0] = gsu_add(R[0], R[5], 0);
    R[0] = gsu_div2(R[0]);
    R[0] = gsu_div2(R[0]);
    R[0] = gsu_div2(R[0]);
    R[1] = R[0];
    R[0] = 0x004A;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04b947, 15);
    gsu_stw(R[0], R[1]);
    R[0] = 0x0004;
    gsu_code(0x04b956, 4);
    gsu_romb(R[0]);
    R[2] = 0x0050;
    R[2] = gsu_add(R[2], R[8], 0);
    gsu_code(0x04b95a, 6);
    R[2] = gsu_ldw(R[2]);
    R[0] = 0x0044;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04b960, 4);
    R[0] = gsu_ldw(R[0]);
    R[1] = 0x0004;
    gsu_sub(R[0], R[1], 0);   /* CMP */
    t = gsu.s;
    gsu_code(0x04b964, 6);
    if (t) { gsu_code(0x04b97f, 1); goto lB97F; }
    R[2] = gsu_add(R[2], R[2], 0);
    R[1] = 0x0028;
    R[1] = gsu_add(R[1], R[8], 0);
    gsu_code(0x04b96a, 9);
    R[1] = gsu_ldw(R[1]);
    R[0] = R[1];
    R[0] = gsu_div2(R[0]);
    R[0] = gsu_div2(R[0]);
    R[1] = gsu_sub(R[1], R[0], 0);
    R[0] = 0x0028;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04b973, 13);
    gsu_stw(R[0], R[1]);
lB97F:
    R[1] = 0x0052;
    R[1] = gsu_add(R[1], R[8], 0);
    gsu_code(0x04b980, 6);
    R[1] = gsu_ldw(R[1]);
    R[0] = gsu_sub(R[2], R[1], 0);
    R[0] = gsu_div2(R[0]);
    R[1] = gsu_add(R[0], R[1], 0);
    R[0] = 0x0052;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04b986, 11);
    gsu_stw(R[0], R[1]);
lB990:
    R[11] = 0xB995;
    gsu_code(0x04b991, 4); gsu_code(0x04cdca, 1);
    abi_FxLapSegment();
lB995:
    R[0] = 0x0086;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04b996, 5);
    R[0] = gsu_ldw(R[0]);
    R[11] = 0xB99F;
    gsu_code(0x04b99b, 4); gsu_code(0x04ced6, 1);
    abi_FxCourseNextDistance();
lB99F:
    R[1] = R[0];
    R[0] = 0x0084;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04b9a0, 8);
    gsu_stw(R[0], R[1]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x04b9a8, 4);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
    /* FxCarCollide */
lBD20:
    gsu_code(0x04bd21, 2);
    gsu_stw(R[10], R[11]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_code(0x04bd23, 7);
    gsu_stw(FX_PUSH_TURN, R[0]);
    gsu_code(0x04bd2a, 4);
    gsu_stw(FX_HIT_DEPTH, R[0]);
    gsu_code(0x04bd2e, 4);
    gsu_stw(FX_PUSH_AMOUNT, R[0]);
    gsu_code(0x04bd32, 4);
    gsu_stw(FX_CLOSING, R[0]);
    gsu_code(0x04bd36, 4);
    R[0] = gsu_ldw(FX_HIT_ON);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = !gsu.z;
    gsu_code(0x04bd3a, 4);
    if (t) { gsu_code(0x04bd46, 1); goto lBD46; }
    R[1] = 0x0000;
    R[0] = 0x0094;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04bd3e, 9);
    gsu_stw(R[0], R[1]);
lBD46:
    R[9] = R[8];
    R[0] = 0x0018;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04bd47, 6);
    R[0] = gsu_ldw(R[0]);
    R[1] = R[0];
    gsu_moves(R[0]);
    t = !gsu.z;
    gsu_code(0x04bd4d, 4);
    if (t) { gsu_code(0x04bd55, 1); goto lBD55; }
    gsu_code(0x04bd51, 4); gsu_code(0x04bf42, 1);
    goto lBF42;
lBD55:
    R[0] = 0x001C;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04bd56, 4);
    R[0] = gsu_ldw(R[0]);
    R[2] = 0x0040;
    R[0] = gsu_and(R[0], R[2]);
    t = gsu.z;
    gsu_code(0x04bd5a, 6);
    if (t) { gsu_code(0x04bd64, 1); goto lBD64; }
    gsu_code(0x04bd60, 4); gsu_code(0x04bf3d, 1);
    goto lBF3D;
lBD64:
    R[7] = 0x000C;
    R[7] = gsu_add(R[7], R[1], 0);
    gsu_code(0x04bd65, 6);
    R[7] = gsu_ldw(R[7]);
    gsu_code(0x04bd6b, 4);
    gsu_stw(FX_COLLIDE_MODEL, R[7]);
    R[2] = 0x001A;
    R[2] = gsu_add(R[2], R[7], 0);
    gsu_code(0x04bd6f, 6);
    R[2] = gsu_ldw(R[2]);
    R[0] = 0x0002;
    R[0] = gsu_and(R[0], R[2]);
    t = gsu.z;
    gsu_code(0x04bd75, 6);
    if (t) { gsu_code(0x04bd7f, 1); goto lBD7F; }
    gsu_code(0x04bd7b, 4); gsu_code(0x04bf30, 1);
    goto lBF30;
lBD7F:
    R[2] = 0x0016;
    R[2] = gsu_add(R[2], R[7], 0);
    gsu_code(0x04bd80, 6);
    R[2] = gsu_ldw(R[2]);
    R[0] = 0x0020;
    R[0] = gsu_and(R[0], R[2]);
    t = gsu.z;
    gsu_code(0x04bd86, 6);
    if (t) { gsu_code(0x04bd90, 1); goto lBD90; }
    gsu_code(0x04bd8c, 4); gsu_code(0x04bf30, 1);
    goto lBF30;
lBD90:
    R[0] = 0x0002;
    R[0] = gsu_and(R[0], R[2]);
    t = gsu.z;
    gsu_code(0x04bd91, 6);
    if (t) { gsu_code(0x04bdbd, 1); goto lBDBD; }
    R[3] = 0x0014;
    gsu_code(0x04bd97, 8);
    R[0] = gsu_ldw(FX_DRIVE_LEVEL);
    R[0] = gsu_add(R[0], R[0], 0);
    R[3] = gsu_sub(R[3], R[0], 0);
    t = !gsu.s;
    gsu_code(0x04bd9f, 5);
    if (t) { gsu_code(0x04bda6, 1); goto lBDA6; }
    R[3] = 0x0000;
    gsu_code(0x04bda4, 3);
lBDA6:
    R[0] = 0x007C;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04bda7, 5);
    gsu_stw(R[0], R[3]);
    R[0] = 0x005E;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04bdac, 4);
    R[0] = gsu_ldw(R[0]);
    R[3] = 0x005E;
    R[3] = gsu_add(R[3], R[7], 0);
    gsu_code(0x04bdb0, 6);
    R[3] = gsu_ldw(R[3]);
    R[0] = gsu_add(R[0], R[3], 0);
    t = gsu.z;
    gsu_code(0x04bdb6, 3);
    if (t) { gsu_code(0x04bdbd, 1); goto lBDBD; }
    gsu_code(0x04bdb9, 4); gsu_code(0x04bf30, 1);
    goto lBF30;
lBDBD:
    gsu_code(0x04bdbe, 2);
    gsu_stw(R[10], R[1]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_code(0x04bdc0, 4);
    gsu_stw(R[10], R[8]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[11] = 0xBDCA;
    gsu_code(0x04bdc4, 6); gsu_code(0x0496a7, 1);
    abi_FxSumsClear();
    gsu_code(0x04bdca, 1);
lBDCA:
    R[0] = 0x0002;
    R[0] = gsu_and(R[0], R[2]);
    t = gsu.z;
    gsu_code(0x04bdcb, 6);
    if (t) { gsu_code(0x04bdf0, 1); goto lBDF0; }
    R[0] = 0x0064;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04bdd1, 5);
    R[0] = gsu_ldw(R[0]);
    R[3] = 0x0010;
    R[0] = gsu_and(R[0], R[3]);
    t = gsu.z;
    gsu_code(0x04bdd6, 6);
    if (t) { gsu_code(0x04bde7, 1); goto lBDE7; }
    R[0] = 0x0064;
    R[0] = gsu_add(R[0], R[7], 0);
    gsu_code(0x04bddc, 5);
    R[0] = gsu_ldw(R[0]);
    R[3] = 0x0003;
    R[0] = gsu_and(R[0], R[3]);
    t = gsu.z;
    gsu_code(0x04bde1, 6);
    if (t) { gsu_code(0x04bdf0, 1); goto lBDF0; }
    gsu_code(0x04bde7, 1);
lBDE7:
    R[0] = 0x002C;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04bde8, 4);
    R[0] = gsu_ldw(R[0]);
    R[3] = 0x0004;
    R[0] = gsu_or(R[0], R[3]);
    gsu_code(0x04bdec, 5);
    gsu_sbk(R[0]);
lBDF0:
    R[0] = 0x0016;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04bdf1, 4);
    R[0] = gsu_ldw(R[0]);
    R[2] = 0x0010;
    R[0] = gsu_and(R[0], R[2]);
    t = gsu.z;
    gsu_code(0x04bdf5, 6);
    if (t) { gsu_code(0x04bdff, 1); goto lBDFF; }
    gsu_code(0x04bdfb, 4); gsu_code(0x04bed1, 1);
    goto lBED1;
lBDFF:
    gsu_code(0x04be00, 2);
    gsu_stw(R[10], R[1]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_code(0x04be02, 4);
    gsu_stw(R[10], R[7]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_code(0x04be06, 4);
    gsu_stw(R[10], R[9]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[0] = R[9];
    R[9] = R[7];
    R[7] = R[0];
    R[11] = 0xBE16;
    gsu_code(0x04be0a, 12); gsu_code(0x049c3d, 1);
    goto l9C3D;
lBE16:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x04be17, 4);
    R[9] = gsu_ldw(R[10]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x04be1b, 4);
    R[7] = gsu_ldw(R[10]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x04be1f, 4);
    R[1] = gsu_ldw(R[10]);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = !gsu.z;
    gsu_code(0x04be23, 4);
    if (t) { gsu_code(0x04be2b, 1); goto lBE2B; }
    gsu_code(0x04be27, 4); gsu_code(0x04bf28, 1);
    goto lBF28;
lBE2B:
    R[11] = 0xBE30;
    gsu_code(0x04be2c, 4); gsu_code(0x04957c, 1); gsu_cache(0x957d);
    goto l957C;
lBE30:
    R[0] = R[0];
    gsu_moves(R[0]);
    t = !gsu.z;
    gsu_code(0x04be31, 4);
    if (t) { gsu_code(0x04be39, 1); goto lBE39; }
    gsu_code(0x04be35, 4); gsu_code(0x04bf28, 1);
    goto lBF28;
lBE39:
    R[11] = 0xBE3E;
    gsu_code(0x04be3a, 4); gsu_code(0x04941c, 1);
    abi_FxPushesApply();
    gsu_code(0x04be3e, 1);
lBE3E:
    gsu_code(0x04be3f, 4);
    R[0] = gsu_ldw(FX_WORLD_SUM);
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
    R[0] = gsu_add(R[0], R[0], 0);
    gsu_code(0x04be43, 4);
    gsu_sbk(R[0]);
    gsu_code(0x04be47, 4);
    R[0] = gsu_ldw((FX_WORLD_SUM + 2));
    R[0] = 0x0000;
    gsu_code(0x04be4b, 3);
    gsu_sbk(R[0]);
    gsu_code(0x04be4e, 4);
    R[0] = gsu_ldw((FX_WORLD_SUM + 4));
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
    R[0] = gsu_add(R[0], R[0], 0);
    gsu_code(0x04be52, 4);
    gsu_sbk(R[0]);
    R[1] = 0x0000;
    R[2] = 0x0000;
    R[3] = 0x0000;
    R[11] = 0xBE60;
    gsu_code(0x04be56, 10); gsu_code(0x0498ab, 1);
    abi_FxHitAddUnit();
    gsu_code(0x04be60, 1);
lBE60:
    gsu_code(0x04be61, 3);
    R[1] = gsu_ldw(GSU_IN_20);
    R[1] = gsu_div2(R[1]);
    R[1] = gsu_div2(R[1]);
    R[2] = 0x0000;
    gsu_code(0x04be64, 11);
    R[3] = gsu_ldw(GSU_IN_24);
    R[3] = gsu_div2(R[3]);
    R[3] = gsu_div2(R[3]);
    gsu_code(0x04be6f, 9);
    R[6] = gsu_ldw(GSU_OUT_E4);
    gsu_code(0x04be78, 2);
    R[0] = gsu_fmult(R[1], R[6]);
    R[5] = gsu_rol(R[0]);
    gsu_code(0x04be7a, 5);
    R[6] = gsu_ldw(FX_OBJECT_MATRIX_1);
    gsu_code(0x04be7f, 2);
    R[0] = gsu_fmult(R[2], R[6]);
    R[0] = gsu_rol(R[0]);
    R[5] = gsu_add(R[5], R[0], 0);
    gsu_code(0x04be81, 6);
    R[6] = gsu_ldw(FX_OBJECT_MATRIX_2);
    gsu_code(0x04be87, 2);
    R[0] = gsu_fmult(R[3], R[6]);
    R[0] = gsu_rol(R[0]);
    R[0] = gsu_add(R[0], R[5], 0);
    gsu_code(0x04be89, 5);
    gsu_stw(GSU_IN_20, R[0]);
    gsu_code(0x04be8e, 3);
    R[6] = gsu_ldw(FX_OBJECT_MATRIX_3);
    gsu_code(0x04be91, 2);
    R[0] = gsu_fmult(R[1], R[6]);
    R[5] = gsu_rol(R[0]);
    gsu_code(0x04be93, 5);
    R[6] = gsu_ldw(FX_OBJECT_MATRIX_4);
    gsu_code(0x04be98, 2);
    R[0] = gsu_fmult(R[2], R[6]);
    R[0] = gsu_rol(R[0]);
    R[5] = gsu_add(R[5], R[0], 0);
    gsu_code(0x04be9a, 6);
    R[6] = gsu_ldw(FX_OBJECT_MATRIX_5);
    gsu_code(0x04bea0, 2);
    R[0] = gsu_fmult(R[3], R[6]);
    R[0] = gsu_rol(R[0]);
    R[0] = gsu_add(R[0], R[5], 0);
    gsu_code(0x04bea2, 5);
    gsu_stw(GSU_IN_22, R[0]);
    gsu_code(0x04bea7, 3);
    R[6] = gsu_ldw(FX_OBJECT_MATRIX_6);
    gsu_code(0x04beaa, 2);
    R[0] = gsu_fmult(R[1], R[6]);
    R[5] = gsu_rol(R[0]);
    gsu_code(0x04beac, 5);
    R[6] = gsu_ldw(FX_OBJECT_MATRIX_7);
    gsu_code(0x04beb1, 2);
    R[0] = gsu_fmult(R[2], R[6]);
    R[0] = gsu_rol(R[0]);
    R[5] = gsu_add(R[5], R[0], 0);
    gsu_code(0x04beb3, 6);
    R[6] = gsu_ldw(FX_OBJECT_MATRIX_8);
    gsu_code(0x04beb9, 2);
    R[0] = gsu_fmult(R[3], R[6]);
    R[0] = gsu_rol(R[0]);
    R[0] = gsu_add(R[0], R[5], 0);
    gsu_code(0x04bebb, 5);
    gsu_stw(GSU_IN_24, R[0]);
    gsu_code(0x04bec0, 3);
    R[1] = gsu_ldw(GSU_IN_20);
    gsu_code(0x04bec3, 3);
    R[2] = gsu_ldw(GSU_IN_22);
    gsu_code(0x04bec6, 3);
    R[3] = gsu_ldw(GSU_IN_24);
    R[11] = 0xBECD;
    gsu_code(0x04bec9, 4); gsu_code(0x049a9b, 1);
    abi_FxPushAt4();
lBECD:
    gsu_code(0x04bece, 3); gsu_code(0x04bf28, 1);
    goto lBF28;
lBED1:
    R[11] = 0xBED6;
    gsu_code(0x04bed2, 4); gsu_code(0x049301, 1);
    abi_FxCollidePush();
    gsu_code(0x04bed6, 1);
lBED6:
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_code(0x04bed7, 5);
    gsu_stw(FX_PUSH_TURN, R[0]);
    R[3] = 0x23A4;
    gsu_code(0x04bedc, 7);
    R[2] = gsu_ldw(FX_COLLIDE_MODEL);
    R[11] = 0xBEE7;
    gsu_code(0x04bee3, 4); gsu_code(0x04944c, 1);
    abi_FxPushApply();
    gsu_code(0x04bee7, 1);
lBEE7:
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.z;
    gsu_code(0x04bee8, 4);
    if (t) { gsu_code(0x04befc, 1); goto lBEFC; }
    gsu_code(0x04beec, 5);
    R[1] = gsu_ldw(FX_CLOSING);
    R[1] = R[1];
    gsu_moves(R[1]);
    t = gsu.s;
    gsu_code(0x04bef1, 4);
    if (t) { gsu_code(0x04befc, 1); goto lBEFC; }
    R[0] = 0x0094;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04bef5, 6);
    R[0] = gsu_ldw(R[0]);
    R[0] = gsu_add(R[0], R[1], 0);
    gsu_code(0x04befb, 2);
    gsu_sbk(R[0]);
lBEFC:
    gsu_code(0x04befd, 4);
    R[0] = gsu_ldw(FX_WORLD_SUM);
    R[0] = gsu_add(R[0], R[0], 0);
    gsu_code(0x04bf01, 2);
    gsu_sbk(R[0]);
    R[1] = R[0];
    gsu_code(0x04bf03, 6);
    R[0] = gsu_ldw((FX_WORLD_SUM + 4));
    R[0] = gsu_add(R[0], R[0], 0);
    gsu_code(0x04bf09, 2);
    gsu_sbk(R[0]);
    gsu_code(0x04bf0b, 4);
    R[1] = gsu_ldw(FX_WORLD_SUM);
    gsu_code(0x04bf0f, 4);
    R[0] = gsu_ldw(FX_HIT);
    R[0] = gsu_add(R[0], R[1], 0);
    gsu_code(0x04bf13, 2);
    gsu_sbk(R[0]);
    gsu_code(0x04bf15, 4);
    R[1] = gsu_ldw((FX_WORLD_SUM + 4));
    gsu_code(0x04bf19, 4);
    R[0] = gsu_ldw((FX_HIT + 4));
    R[0] = gsu_add(R[0], R[1], 0);
    gsu_code(0x04bf1d, 2);
    gsu_sbk(R[0]);
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_code(0x04bf1f, 5);
    gsu_stw((FX_WORLD_SUM + 2), R[0]);
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_code(0x04bf24, 5);
    gsu_stw((FX_WORLD_SUM + 2), R[0]);
lBF28:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x04bf29, 4);
    R[8] = gsu_ldw(R[10]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x04bf2d, 4);
    R[1] = gsu_ldw(R[10]);
lBF30:
    R[0] = 0x000E;
    R[0] = gsu_add(R[0], R[1], 0);
    gsu_code(0x04bf31, 4);
    R[0] = gsu_ldw(R[0]);
    R[1] = R[0];
    gsu_moves(R[0]);
    t = gsu.z;
    gsu_code(0x04bf35, 4);
    if (t) { gsu_code(0x04bf3d, 1); goto lBF3D; }
    gsu_code(0x04bf39, 4); gsu_code(0x04bd64, 1);
    goto lBD64;
lBF3D:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x04bf3e, 4);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
lBF42:
    R[0] = 0x001C;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04bf43, 4);
    R[0] = gsu_ldw(R[0]);
    R[1] = 0x0040;
    R[0] = gsu_bic(R[0], R[1]);
    gsu_code(0x04bf47, 6);
    gsu_sbk(R[0]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x04bf4d, 4);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
    /* FxCarFollowCourse */
lC3D5:
    gsu_code(0x04c3d6, 2);
    gsu_stw(R[10], R[11]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[1] = 0x0016;
    R[1] = gsu_add(R[1], R[8], 0);
    gsu_code(0x04c3d8, 8);
    R[1] = gsu_ldw(R[1]);
    R[0] = 0x0400;
    R[0] = gsu_and(R[0], R[1]);
    t = !gsu.z;
    gsu_code(0x04c3e0, 6);
    if (t) { gsu_code(0x04c3ea, 1); goto lC3EA; }
    gsu_code(0x04c3e6, 4); gsu_code(0x04c43d, 1);
    goto lC43D;
lC3EA:
    R[0] = 0x0018;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04c3eb, 5);
    R[0] = gsu_ldw(R[0]);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = gsu.z;
    gsu_code(0x04c3f0, 4);
    if (t) { gsu_code(0x04c3fd, 1); goto lC3FD; }
    R[0] = 0x001C;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04c3f4, 5);
    R[0] = gsu_ldw(R[0]);
    R[1] = 0x0040;
    R[0] = gsu_or(R[0], R[1]);
    gsu_code(0x04c3f9, 5);
    gsu_sbk(R[0]);
lC3FD:
    R[0] = 0x001C;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04c3fe, 4);
    R[0] = gsu_ldw(R[0]);
    R[1] = 0x0010;
    R[2] = gsu_and(R[0], R[1]);
    t = !gsu.z;
    gsu_code(0x04c402, 7);
    if (t) { gsu_code(0x04c423, 1); goto lC423; }
    R[0] = gsu_or(R[0], R[1]);
    gsu_code(0x04c409, 3);
    gsu_sbk(R[0]);
    R[1] = 0xFFFF;
    R[0] = 0x0072;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04c40c, 7);
    gsu_stw(R[0], R[1]);
    R[1] = 0x0000;
    R[0] = 0x002E;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04c413, 7);
    gsu_stw(R[0], R[1]);
    R[0] = 0x0030;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04c41a, 5);
    gsu_stw(R[0], R[1]);
    R[0] = 0x0032;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04c41f, 5);
    gsu_stw(R[0], R[1]);
lC423:
    R[2] = 0x0070;
    R[2] = gsu_add(R[2], R[8], 0);
    gsu_code(0x04c424, 6);
    R[2] = gsu_ldw(R[2]);
    R[0] = 0x0004;
    R[0] = gsu_and(R[0], R[2]);
    t = gsu.z;
    gsu_code(0x04c42a, 6);
    if (t) { gsu_code(0x04c434, 1); goto lC434; }
    gsu_code(0x04c430, 4); gsu_code(0x04c4be, 1);
    goto lC4BE;
lC434:
    R[11] = 0xC439;
    gsu_code(0x04c435, 4); gsu_code(0x04c595, 1);
    abi_FxCarCatchUp();
    gsu_code(0x04c439, 1);
lC439:
    gsu_code(0x04c43a, 3); gsu_code(0x04c4b7, 1);
    goto lC4B7;
lC43D:
    R[0] = 0x001C;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04c43e, 4);
    R[0] = gsu_ldw(R[0]);
    R[1] = 0x0010;
    R[2] = gsu_and(R[0], R[1]);
    t = gsu.z;
    gsu_code(0x04c442, 7);
    if (t) { gsu_code(0x04c456, 1); goto lC456; }
    R[0] = gsu_bic(R[0], R[1]);
    gsu_code(0x04c449, 4);
    gsu_sbk(R[0]);
    R[11] = 0xC451;
    gsu_code(0x04c44d, 4); gsu_code(0x04b9ac, 1);
    abi_FxCarPose();
    gsu_code(0x04c451, 1);
lC451:
    R[11] = 0xC456;
    gsu_code(0x04c452, 4); gsu_code(0x04c72f, 1);
    goto lC72F;
lC456:
    R[2] = 0x0070;
    R[2] = gsu_add(R[2], R[8], 0);
    gsu_code(0x04c457, 6);
    R[2] = gsu_ldw(R[2]);
    R[0] = 0x0004;
    R[0] = gsu_and(R[0], R[2]);
    t = gsu.z;
    gsu_code(0x04c45d, 6);
    if (t) { gsu_code(0x04c467, 1); goto lC467; }
    gsu_code(0x04c463, 4); gsu_code(0x04c4be, 1);
    goto lC4BE;
lC467:
    R[2] = 0x002C;
    R[2] = gsu_add(R[2], R[8], 0);
    gsu_code(0x04c468, 6);
    R[2] = gsu_ldw(R[2]);
    R[0] = 0x0040;
    R[0] = gsu_and(R[0], R[2]);
    t = gsu.z;
    gsu_code(0x04c46e, 6);
    if (t) { gsu_code(0x04c478, 1); goto lC478; }
    gsu_code(0x04c474, 4); gsu_code(0x04c4ab, 1);
    goto lC4AB;
lC478:
    R[1] = 0x001E;
    R[1] = gsu_add(R[1], R[8], 0);
    gsu_code(0x04c479, 6);
    R[1] = gsu_ldw(R[1]);
    R[2] = 0x0020;
    R[2] = gsu_add(R[2], R[8], 0);
    gsu_code(0x04c47f, 6);
    R[2] = gsu_ldw(R[2]);
    R[3] = 0x0022;
    R[3] = gsu_add(R[3], R[8], 0);
    gsu_code(0x04c485, 6);
    R[3] = gsu_ldw(R[3]);
    R[11] = 0xC48F;
    gsu_code(0x04c48b, 4); gsu_code(0x04cc06, 1);
    abi_FxCourseNearest();
    gsu_code(0x04c48f, 1);
lC48F:
    R[0] = R[0];
    gsu_moves(R[0]);
    t = !gsu.z;
    gsu_code(0x04c490, 4);
    if (t) { gsu_code(0x04c498, 1); goto lC498; }
    gsu_code(0x04c494, 4); gsu_code(0x04c4ab, 1);
    goto lC4AB;
lC498:
    R[7] = R[0];
    R[0] = 0x0058;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04c499, 7);
    gsu_stw(R[0], R[7]);
    R[0] = R[7];
    R[11] = 0xC4A6;
    gsu_code(0x04c4a0, 6); gsu_code(0x04cca4, 1);
    abi_FxCourseNextEntry();
lC4A6:
    R[11] = 0xC4AB;
    gsu_code(0x04c4a7, 4); gsu_code(0x04c7d5, 1);
    abi_FxDriveDecide();
    gsu_code(0x04c4ab, 1);
lC4AB:
    R[11] = 0xC4B0;
    gsu_code(0x04c4ac, 4); gsu_code(0x04c933, 1);
    abi_FxTrafficSides();
    gsu_code(0x04c4b0, 1);
lC4B0:
    R[0] = 0x0001;
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x04c4b1, 6);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
lC4B7:
    R[0] = 0x0000;
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x04c4b8, 6);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
lC4BE:
    R[11] = 0xC4C3;
    gsu_code(0x04c4bf, 4); gsu_code(0x04c3b5, 1);
    abi_FxRecordSegment();
lC4C3:
    R[7] = R[0];
    R[0] = 0x0058;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04c4c4, 7);
    gsu_stw(R[0], R[7]);
    R[0] = 0x0001;
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x04c4cb, 6);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
    /* FxCarAimAlongCourse */
lC72F:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x04c730, 4);
    R[11] = gsu_ldw(R[10]);
    R[0] = 0x0058;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04c734, 4);
    R[0] = gsu_ldw(R[0]);
    R[0] = R[0];
    gsu_moves(R[0]);
    t = !gsu.z;
    gsu_code(0x04c738, 4);
    if (t) { gsu_code(0x04c740, 1); goto lC740; }
    gsu_code(0x04c73c, 4); gsu_code(0x04c77c, 1);
    goto lC77C;
lC740:
    R[11] = 0xC745;
    gsu_code(0x04c741, 4); gsu_code(0x04cd83, 1);
    abi_FxTableEntryWords();
    gsu_code(0x04c745, 1);
lC745:
    R[11] = 0xC74A;
    gsu_code(0x04c746, 4); gsu_code(0x04cd56, 1);
    abi_FxCourseEntryRead();
    gsu_code(0x04c74a, 1);
lC74A:
    R[0] = 0x0004;
    gsu_code(0x04c74b, 4);
    gsu_romb(R[0]);
    gsu_code(0x04c74f, 4);
    R[0] = gsu_ldw(FX_DRIVE_LEVEL);
    R[0] = gsu_lob(R[0]);
    R[0] = gsu_add(R[0], R[0], 0);
    gsu_code(0x04c753, 5);
    gsu_r14(0xC781);
    gsu_code(0x04c758, 2);
    gsu_r14(gsu_add(R[14], R[0], 0));
    gsu_code(0x04c75a, 2);
    R[6] = gsu_getb();
    gsu_code(0x04c75c, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_code(0x04c75d, 3);
    R[6] = gsu_getbh(R[6]);
    R[0] = gsu_add(R[4], R[4], 0);
    gsu_code(0x04c760, 3);
    R[0] = gsu_fmult(R[0], R[6]);
    R[4] = R[0];
    R[0] = 0x0024;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04c763, 6);
    R[0] = gsu_ldw(R[0]);
    R[11] = 0xC76D;
    gsu_code(0x04c769, 4); gsu_code(0x04cd98, 1);
    abi_FxAngleVector();
    gsu_code(0x04c76d, 1);
lC76D:
    R[0] = 0x002E;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04c76e, 5);
    gsu_stw(R[0], R[1]);
    R[0] = 0x0030;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04c773, 5);
    gsu_stw(R[0], R[2]);
    R[0] = 0x0032;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x04c778, 5);
    gsu_stw(R[0], R[3]);
lC77C:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_code(0x04c77d, 4);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
stated:   /* the code returned to has its first byte stated */
    if (at == ret || (ret != 0xFFFF && (int16_t)(base - R[10]) >= 4))
        return;
    goto fetched;
dispatch:
    if (at == ret) {
        gsu_code(0x040000 | at, 1);   /* the code returned to: its first byte */
        return;
    }
    if (ret != 0xFFFF && (int16_t)(base - R[10]) >= 4) {
        gsu_code(0x040000 | at, 1);   /* where it returns, two levels up: its first byte */
        return;
    }
    gsu_code(0x040000 | at, 1);   /* where it jumps: its first byte */
fetched:
    if (at == 0x957C || at == 0x9E91 || at == 0xB80F)
        gsu_cache((uint16_t)(at + 1));   /* the code there starts with a CACHE */
enter:
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
    case 0x9A9B: nx = R[11]; abi_FxPushAt4(); at = nx; goto stated;
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
