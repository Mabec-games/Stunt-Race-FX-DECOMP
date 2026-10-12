/*
 * fx_car.c: the Super FX sets up a car's record from a script in ROM.
 *
 * The script is read through the ROM buffer from R14 (in ROMBR's bank): a
 * byte giving the number of parts, a word for the parts' mode, seven words
 * per part, then a byte choosing one of the 5-byte entries that follow.
 * Each part is a $52-byte record of its own on the car's list of parts:
 * the car's +$3E is the first part, a part's +$34 the next.
 */
#include <string.h>
#include "gsu_ops.h"
#include "fx_calls.h"

#define FX_POOL_FIRST 0x1098u
#define FX_PART_MODE 0x0068u     /* the parts' mode word from the script */
#define FX_SCRIPT_AT 0x2392u     /* where the entries start, from $8090 */
#define FX_CAR_26F2 0x26F2u

#define PART_BYTES 0x0052u
#define FX_CAR_POS 0x2354u       /* the car's position, for its parts */
#define FX_ANGLES_W 0x0020u
#define FX_MATRIX 0x00E4u

#define CAR_PITCH 0x24u
#define CAR_YAW 0x26u
#define CAR_ROLL 0x28u
#define CAR_PARTS 0x3Eu
#define PART_OFFSET 0x3Cu       /* +$3C/+$3E/+$40: offset in the car's axes */
#define PART_WORLD 0x36u        /* +$36/+$38/+$3A: the offset turned */
#define PART_NEXT 0x34u
#define PART_FLAGS 0x46u
#define REC_POS 0x1Eu
#define REC_VEL 0x2Eu

/* A word of the script (GETB, INC R14, GETBH, INC R14). */
static uint16_t script_word(uint16_t *r14)
{
    uint16_t w = gsu_getb();

    gsu_r14(gsu_inc(*r14));
    *r14 = gsu.r[14];
    w = gsu_getbh(w);
    gsu_r14(gsu_inc(*r14));
    *r14 = gsu.r[14];
    return w;
}

/* Field `field` of record `rec` = R3, through R0. */
static void field_set(FxRegs *g, uint16_t field, uint16_t rec)
{
    g->r[0] = gsu_add(field, rec, 0);
    gsu_stw(g->r[0], g->r[3]);
}

/*
 * @routine 04:8E84 FxCarAddPart
 * WHAT IT DOES: Adds a part to car R1: a new $52-byte record from the first
 * pool whose +$12, +$3C (negated), +$3E, +$40, +$44, +$50 and +$46 are the
 * next seven script words; its +$0C, +$0E, +$10 = $14, bit 2 of +$16 set,
 * +$1A = the parts' mode ($0068), +$2A = the car's +$2A; it goes on the
 * front of the car's parts list. With no room the script is not read
 * (decoded from the ROM bytes and not reached in the traces so far).
 * HOW: FxPoolTake; each field is STW through R0 = field + R2 with R3 the
 * value. R1 and the return address are kept on the R10 stack.
 * Entry: R1 = the car's record; R14 = the script; R10 = stack; R11 =
 * return address. Exit: R1 = the car; R2 = the part (0 without); R3 = the
 * car's +$2A; R0 = the part + $2A; R14 past the seven words; R11 = $8E97;
 * R4, R12, R13 as FxPoolTake leaves them; S, Z from R10, OV, CY from the
 * last ADD; the last RAM address R10.
 */
FxRegs FxCarAddPart(FxRegs g)
{
    static const uint8_t fields[7] = {0x12, 0x3C, 0x3E, 0x40, 0x44, 0x50, 0x46};
    uint16_t *sp = &g.r[10];
    int i;

    gsu_push(sp, g.r[11]);
    gsu_push(sp, g.r[1]);
    g.r[4] = PART_BYTES;
    g.r[3] = FX_POOL_FIRST;
    g.r[11] = 0x8E97;
    g = FxPoolTake(g);
    g.r[2] = g.r[1];
    g.r[1] = gsu_pull(sp);
    gsu_moves(g.r[2]);
    if (gsu.z) {
        gsu_pull(sp);
        return g;
    }
    for (i = 0; i < 7; i++) {
        g.r[3] = script_word(&g.r[14]);
        if (fields[i] == 0x3C)
            g.r[3] = gsu_inc(gsu_not(g.r[3]));
        field_set(&g, fields[i], g.r[2]);
    }
    g.r[3] = gsu_ldw(gsu_add(0x3E, g.r[1], 0));     /* the car's first part */
    field_set(&g, 0x34, g.r[2]);
    g.r[0] = gsu_add(0x3E, g.r[1], 0);
    gsu_stw(g.r[0], g.r[2]);
    g.r[3] = 0x0014;
    field_set(&g, 0x0C, g.r[2]);
    field_set(&g, 0x0E, g.r[2]);
    field_set(&g, 0x10, g.r[2]);
    g.r[3] = gsu_ldw(gsu_add(0x16, g.r[2], 0));
    g.r[0] = 0x0004;
    g.r[3] = gsu_or(g.r[0], g.r[3]);
    field_set(&g, 0x16, g.r[2]);
    g.r[3] = gsu_ldw(FX_PART_MODE);
    field_set(&g, 0x1A, g.r[2]);
    g.r[3] = gsu_ldw(gsu_add(0x2A, g.r[1], 0));
    field_set(&g, 0x2A, g.r[2]);
    gsu_pull(sp);
    return g;
}

void abi_FxCarAddPart(void)
{
    FxRegs g;

    memcpy(g.r, gsu.r, sizeof g.r);
    g = FxCarAddPart(g);
    memcpy(gsu.r, g.r, 15 * sizeof g.r[0]);
}

/*
 * @routine 04:8F48 FxCarFromScript
 * WHAT IT DOES: Sets up car R1 from the script at R14: its parts
 * (FxCarAddPart for each), $2392 = where the entries start less $8090, +$6E
 * = the entry the next byte chooses (5 bytes each), and +$6A = $0180, +$8C
 * = $1000, +$8A = the word at $26F2, +$7A = 0 (laps), +$86 = 2 and +$58 = 1
 * (segments).
 * HOW: The part count goes to R12 for LOOP, with R13 and R12 kept on the
 * R10 stack around each call; the entry is LMULT of the byte by 5 added to
 * R14. The return address is kept on the R10 stack.
 * Entry: R1 = the car's record; R14 = the script; R10 = stack; R11 =
 * return address. Exit: R14 = R2 = the chosen entry; R0 = R1 + $58; R4 =
 * the byte times 5; R6 = 5; R11 = $8E97 (with parts); R12 = 0; R13 =
 * $8F61; S, Z from R10, OV, CY from the last ADD; the last RAM address R10.
 */
FxRegs FxCarFromScript(FxRegs g)
{
    static const struct { uint8_t field; uint16_t value; } set[6] = {
        {0x6A, 0x0180}, {0x8C, 0x1000}, {0x8A, 0}, {0x7A, 0x0000}, {0x86, 0x0002}, {0x58, 0x0001},
    };
    uint16_t *sp = &g.r[10];
    uint32_t p;
    int i;

    gsu_push(sp, g.r[11]);
    g.r[0] = gsu_sub(g.r[0], g.r[0], 0);
    g.r[0] = gsu_getb();
    gsu_r14(gsu_inc(g.r[14]));
    g.r[14] = gsu.r[14];
    g.r[2] = script_word(&g.r[14]);
    gsu_stw(FX_PART_MODE, g.r[2]);
    g.r[12] = g.r[0];
    gsu_moves(g.r[12]);
    if (!gsu.z) {
        g.r[13] = 0x8F61;
        do {
            gsu_push(sp, g.r[13]);
            gsu_push(sp, g.r[12]);
            g.r[11] = 0x8F6E;
            g = FxCarAddPart(g);
            g.r[12] = gsu_pull(sp);
            g.r[13] = gsu_pull(sp);
        } while (gsu_count_down(&g.r[12]));
    }
    g.r[0] = 0x8090;
    g.r[0] = gsu_sub(g.r[14], g.r[0], 0);
    gsu_stw(FX_SCRIPT_AT, g.r[0]);
    g.r[0] = gsu_sub(g.r[0], g.r[0], 0);
    g.r[0] = gsu_getb();
    gsu_r14(gsu_inc(g.r[14]));
    g.r[14] = gsu.r[14];
    g.r[6] = 5;
    p = gsu_lmult(g.r[0], g.r[6]);
    g.r[0] = (uint16_t)(p >> 16);
    g.r[4] = (uint16_t)p;
    gsu_r14(gsu_add(g.r[14], g.r[4], 0));
    g.r[14] = gsu.r[14];
    g.r[2] = g.r[14];
    g.r[0] = gsu_add(0x6E, g.r[1], 0);
    gsu_stw(g.r[0], g.r[2]);
    for (i = 0; i < 6; i++) {
        g.r[2] = set[i].field == 0x8A ? gsu_ldw(FX_CAR_26F2) : set[i].value;
        g.r[0] = gsu_add(set[i].field, g.r[1], 0);
        gsu_stw(g.r[0], g.r[2]);
    }
    gsu_pull(sp);
    return g;
}

void abi_FxCarFromScript(void)
{
    FxRegs g;

    memcpy(g.r, gsu.r, sizeof g.r);
    g = FxCarFromScript(g);
    memcpy(gsu.r, g.r, 15 * sizeof g.r[0]);
}

/*
 * @routine 04:A4ED FxCarMoveParts
 * WHAT IT DOES: Moves each part of car R8 with it: the part's offset
 * (+$3C/+$3E/+$40, in the car's axes) turned into world axes by the
 * matrix at $00E4 goes to +$36/+$38/+$3A; its position +$1E/+$20/+$22
 * becomes the car's ($2354/$2356/$2358) plus that, and its velocity
 * +$2E/+$30/+$32 eight times the change. A part with bit 10 of +$46 set
 * gets velocity 0 instead (decoded from the ROM bytes, with four bytes
 * after the walk's jump and one after this path's that nothing reaches,
 * and not reached in the traces so far).
 * HOW: R12 keeps R8 while R8 and R11 walk the offset and its turned copy;
 * the turn is three rows of FMULT, ROL; the change times 8 is LMULT by R6
 * = 8 (R4 takes the low word). The return address is kept on the R10
 * stack. The routine runs under CACHE (the code cache only; nothing the
 * C has to do).
 * Entry: R8 = the car's record; R10 = stack; R11 = return address. Exit:
 * R9 = 0; R8 = R12 = the car (R12 as on entry without parts); the others
 * from the last part; S, Z from R10; the last RAM address R10.
 */
FxRegs FxCarMoveParts(FxRegs g)
{
    uint32_t p;
    int i;

    gsu_push(&g.r[10], g.r[11]);
    g.r[0] = gsu_ldw(gsu_add(CAR_PARTS, g.r[8], 0));
    for (;;) {
        g.r[9] = g.r[0];
        gsu_moves(g.r[9]);
        if (gsu.z)
            break;
        g.r[1] = gsu_ldw(gsu_add(PART_FLAGS, g.r[9], 0));
        g.r[0] = gsu_and(0x0400, g.r[1]);
        if (!gsu.z) {
            g.r[1] = 0;
            for (i = 0; i < 3; i++) {
                g.r[0] = gsu_add((uint16_t)(REC_VEL + 2 * i), g.r[9], 0);
                gsu_stw(g.r[0], g.r[1]);
            }
            g.r[0] = gsu_ldw(gsu_add(PART_NEXT, g.r[9], 0));
            continue;
        }
        g.r[12] = g.r[8];
        g.r[8] = gsu_add(PART_OFFSET, g.r[9], 0);
        g.r[11] = gsu_add(PART_WORLD, g.r[9], 0);
        for (i = 1; i <= 3; i++) {
            g.r[i] = gsu_ldw(g.r[8]);
            g.r[8] = gsu_inc(gsu_inc(g.r[8]));
        }
        for (i = 0; i < 3; i++) {
            uint16_t row = (uint16_t)(FX_MATRIX + 2 * i);

            g.r[6] = gsu_ldw(row);
            g.r[5] = gsu_rol(gsu_fmult(g.r[1], g.r[6]));
            g.r[6] = gsu_ldw((uint16_t)(row + 6));
            g.r[5] = gsu_add(g.r[5], gsu_rol(gsu_fmult(g.r[2], g.r[6])), 0);
            g.r[6] = gsu_ldw((uint16_t)(row + 12));
            g.r[0] = gsu_add(gsu_rol(gsu_fmult(g.r[3], g.r[6])), g.r[5], 0);
            gsu_stw(g.r[11], g.r[0]);
            g.r[11] = gsu_inc(gsu_inc(g.r[11]));
        }
        g.r[8] = g.r[12];
        g.r[6] = 8;
        for (i = 0; i < 3; i++) {
            g.r[1] = gsu_ldw(gsu_add((uint16_t)(PART_WORLD + 2 * i), g.r[9], 0));
            g.r[0] = gsu_ldw((uint16_t)(FX_CAR_POS + 2 * i));
            g.r[2] = gsu_ldw(gsu_add((uint16_t)(REC_POS + 2 * i), g.r[9], 0));
            g.r[1] = gsu_add(g.r[1], g.r[0], 0);
            g.r[0] = gsu_add((uint16_t)(REC_POS + 2 * i), g.r[9], 0);
            gsu_stw(g.r[0], g.r[1]);
            g.r[1] = gsu_sub(g.r[1], g.r[2], 0);
            p = gsu_lmult(g.r[1], g.r[6]);
            g.r[1] = (uint16_t)(p >> 16);
            g.r[4] = (uint16_t)p;
            g.r[0] = gsu_add((uint16_t)(REC_VEL + 2 * i), g.r[9], 0);
            gsu_stw(g.r[0], g.r[4]);
        }
        g.r[0] = gsu_ldw(gsu_add(PART_NEXT, g.r[9], 0));
    }
    gsu_pull(&g.r[10]);
    return g;
}

void abi_FxCarMoveParts(void)
{
    FxRegs g;

    memcpy(g.r, gsu.r, sizeof g.r);
    g = FxCarMoveParts(g);
    memcpy(gsu.r, g.r, 15 * sizeof g.r[0]);
}

/*
 * @routine 04:B9AC FxCarPose
 * WHAT IT DOES: Sets up the matrix at $00E4 for car R8 (from its angles,
 * negated: +$26, +$24, +$28 as angles 0-2, by FxMatrixFromAngles, then
 * FxMatrixTranspose04) and moves its parts with it (FxCarMoveParts from its
 * position, copied to $2354).
 * HOW: The negated angles go to $0020/$0022/$0024 (NOT, INC, SMS); R8 and
 * the return address are kept on the R10 stack.
 * Entry: R8 = the car's record; R10 = stack; R11 = return address. Exit:
 * as FxCarMoveParts leaves them (R11 = $BA00 for a car without parts); S,
 * Z from R10; the last RAM address R10.
 */
FxRegs FxCarPose(FxRegs g)
{
    static const uint8_t angle[3] = {CAR_YAW, CAR_PITCH, CAR_ROLL};
    int i;

    gsu_push(&g.r[10], g.r[11]);
    gsu_push(&g.r[10], g.r[8]);
    for (i = 0; i < 3; i++) {
        g.r[0] = gsu_ldw(gsu_add(angle[i], g.r[8], 0));
        g.r[0] = gsu_inc(gsu_not(g.r[0]));
        gsu_stw((uint16_t)(FX_ANGLES_W + 2 * i), g.r[0]);
    }
    g.r[9] = FX_MATRIX;
    g.r[11] = 0xB9D7;
    g = fx_via_abi(g, abi_FxMatrixFromAngles);
    g.r[9] = FX_MATRIX;
    g.r[11] = 0xB9DF;
    g = fx_via_abi(g, abi_FxMatrixTranspose04);
    g.r[8] = gsu_pull(&g.r[10]);
    for (i = 0; i < 3; i++) {
        g.r[0] = gsu_ldw(gsu_add((uint16_t)(REC_POS + 2 * i), g.r[8], 0));
        gsu_stw((uint16_t)(FX_CAR_POS + 2 * i), g.r[0]);
    }
    g.r[11] = 0xBA00;
    g = FxCarMoveParts(g);
    gsu_pull(&g.r[10]);
    return g;
}

void abi_FxCarPose(void)
{
    FxRegs g;

    memcpy(g.r, gsu.r, sizeof g.r);
    g = FxCarPose(g);
    memcpy(gsu.r, g.r, 15 * sizeof g.r[0]);
}

#define FX_NEW_RECORD 0x238Eu    /* in: the car's script; out: its record */
#define FX_CAR_AMOUNT 0x2390u
#define FX_CAR_2392 0x2392u
#define FX_CAR_2394 0x2394u
#define FX_CAR_2396 0x2396u
#define FX_CAR_LAST 0x2474u
#define FX_CAR_BAD 0x2650u       /* an unknown script is noted here */
#define FX_CARS 0x262Eu
#define FX_SCRIPT_BANK 0x09u
#define FX_CAR_DEFAULT 0x81B9u
#define CAR_BYTES 0x0098u

/* Field `field` of the new car R1 = `value`, through R2 and R0. */
static void car_set(FxRegs *g, uint16_t f, uint16_t value)
{
    g->r[2] = value;
    g->r[0] = gsu_add(f, g->r[1], 0);
    gsu_stw(g->r[0], g->r[2]);
}

/*
 * @routine 04:8CC6 FxCarNew
 * WHAT IT DOES: A program: makes a car from the script whose address (in
 * bank 9) the S-CPU left at $238E: a $98-byte record from the first pool
 * (its address back to $238E, 0 when there is no room: decoded from the
 * ROM bytes and not reached in the traces so far), set up from $2392,
 * $0068/$002C/$002E (the position), $0022, $2390 and the script's own
 * words, put on the front of the cars' list ($262E, +$74) and noted at
 * $2474; then FxCarFromScript and FxCarPose. A script not among the nine
 * known ones is noted at $2650 and $81B9 used instead (decoded from the
 * ROM bytes and not reached in the traces so far, as are the matches of
 * scripts $8669 and $8886).
 * HOW: ROMB 9 and R14 = the script; nine CMPs with the known addresses.
 * The script gives two words, of which the second is used for $2390 when
 * the low byte of $108C is not 0, a byte for $2394, a word skipped, words
 * for +$5A, +$5C, +$16, +$40, +$1A, +$0C, +$0E, +$10, then (after $2396 =
 * the script offset from $8090 and 10 bytes skipped) +$2A and +$14. The
 * record also gets +$96 = $2392, +$24 = $0022, +$64 = the old $2390, +$66 =
 * the script, +$0A = $B1A1.
 * Entry: the S-CPU starts it. Exit (at its STOP): R1 = R8 = the car; R10
 * = $0240; the others as FxCarPose leaves them.
 */
FxRegs FxCarNew(FxRegs g)
{
    static const uint16_t known[9] = {0x81B9, 0x853D, 0x82E5, 0x8411, 0x8669, 0x876E, 0x8AA8, 0x8886, 0x8990};
    static const uint8_t first[8] = {0x5A, 0x5C, 0x16, 0x40, 0x1A, 0x0C, 0x0E, 0x10};
    int i;

    g.r[10] = 0x0240;
    g.r[0] = FX_SCRIPT_BANK;
    gsu.rombr = FX_SCRIPT_BANK;
    gsu_r14(gsu_ldw(FX_NEW_RECORD));
    g.r[14] = gsu.r[14];
    for (i = 0; i < 9; i++) {
        g.r[0] = known[i];
        gsu_sub(g.r[0], g.r[14], 0);
        if (gsu.z)
            break;
    }
    if (i == 9) {
        gsu_stw(FX_CAR_BAD, g.r[14]);
        gsu_r14(FX_CAR_DEFAULT);
        g.r[14] = gsu.r[14];
    }
    g.r[4] = CAR_BYTES;
    g.r[3] = 0x1098;
    g.r[11] = 0x8D59;
    g = FxPoolTake(g);
    gsu_stw(FX_NEW_RECORD, g.r[1]);
    gsu_moves(g.r[1]);
    if (gsu.z)
        return g;
    car_set(&g, 0x96, gsu_ldw(FX_CAR_2392));
    car_set(&g, 0x1E, gsu_ldw(0x0068));
    car_set(&g, 0x20, gsu_ldw(0x002C));
    car_set(&g, 0x22, gsu_ldw(0x002E));
    car_set(&g, 0x24, gsu_ldw(0x0022));
    car_set(&g, 0x64, gsu_ldw(FX_CAR_AMOUNT));
    car_set(&g, 0x66, g.r[14]);
    car_set(&g, 0x0A, 0xB1A1);
    g.r[2] = script_word(&g.r[14]);
    g.r[3] = script_word(&g.r[14]);
    gsu_stw(FX_CAR_AMOUNT, g.r[2]);
    g.r[0] = gsu_lob(gsu_ldw(0x108C));
    if (!gsu.z)
        gsu_stw(FX_CAR_AMOUNT, g.r[3]);
    g.r[0] = gsu_getb();
    gsu_r14(gsu_inc(g.r[14]));
    g.r[14] = gsu.r[14];
    gsu_stw(FX_CAR_2394, g.r[0]);
    g.r[0] = script_word(&g.r[14]);
    for (i = 0; i < 8; i++)
        car_set(&g, first[i], script_word(&g.r[14]));
    g.r[0] = 0x8090;
    g.r[0] = gsu_sub(g.r[14], g.r[0], 0);
    gsu_stw(FX_CAR_2396, g.r[0]);
    gsu_r14(gsu_add(g.r[14], 10, 0));
    g.r[14] = gsu.r[14];
    car_set(&g, 0x2A, script_word(&g.r[14]));
    car_set(&g, 0x14, script_word(&g.r[14]));
    g.r[0] = gsu_ldw(FX_CARS);
    g.r[2] = g.r[0];
    gsu_moves(g.r[2]);
    if (!gsu.z) {
        g.r[0] = gsu_add(0x74, g.r[1], 0);
        gsu_stw(g.r[0], g.r[2]);
    }
    gsu_stw(FX_CARS, g.r[1]);
    g.r[11] = 0x8E77;
    g = FxCarFromScript(g);
    gsu_stw(FX_CAR_LAST, g.r[1]);
    g.r[8] = g.r[1];
    g.r[11] = 0x8E82;
    g = FxCarPose(g);
    return g;
}

void abi_FxCarNew(void)
{
    FxRegs g;

    memcpy(g.r, gsu.r, sizeof g.r);
    g = FxCarNew(g);
    memcpy(gsu.r, g.r, 15 * sizeof g.r[0]);
}

#define FX_HIT_X 0x2334u          /* the hit: x, y, z */
#define FX_HIT_SCALE 0x2678u
#define FX_HIT_ON 0x263Cu
#define FX_HIT_SUM 0x263Eu
#define FX_PLAYER 0x2476u
#define CAR_64 0x64u
#define CAR_FLAGS40 0x40u
#define CAR_IMPACT 0x62u
#define CAR_44 0x44u
#define CAR_VY 0x30u
#define CAR_DAMAGE 0x82u
#define CAR_70 0x70u
#define CAR_8A 0x8Au
#define REC_CONTACTS_OF_CAR 0x18u

/* |v| by MOVES, then NOT and INC when negative. */
static uint16_t magnitude(uint16_t v)
{
    gsu_moves(v);
    return gsu.s ? gsu_inc(gsu_not(v)) : v;
}

/* Bits `bits` set in field `f` of car R8 (LDW, OR, SBK), through R2. */
static void car_or(FxRegs *g, uint16_t f, uint16_t bits)
{
    g->r[0] = gsu_ldw(gsu_add(f, g->r[8], 0));
    g->r[2] = bits;
    g->r[0] = gsu_or(g->r[0], g->r[2]);
    gsu_sbk(g->r[0]);
}

/*
 * @routine 04:BB17 FxCarHit
 * WHAT IT DOES: The damage from car R8's hit this step. Nothing for a car
 * with bit 3 of +$64 set; +$62 = 0 for one with any of bits $2201 of +$40
 * (or bit 9 of +$8A, decoded from the ROM bytes and not reached in the
 * traces so far). Otherwise +$62 = the hit's y ($2336) / 4, and the impact
 * is |that| (a quarter of it when +$44 is below 2; 0 when +$30 is negative)
 * plus |hit x| / 8 and |hit z| / 8, scaled by $2678 (FMULT), halved for a
 * car with contacts; it is added to $263E (cleared first when $263C is 0).
 * With $263C set, half the sum, when 16 or more (96 at most, decoded from
 * the ROM bytes and not reached in the traces so far), is added to the
 * damage +$82 and sets bit 7 of +$40 (below 24), bit 7 of +$70 and bit 6 of
 * +$40 (below 56) or bit 5 (else); bit 5 for the player's car ($2476) also
 * pushes the cars near it apart (FxSpreadCars). Damage of $200 or more sets
 * bit 0 of +$40.
 * HOW: DIV2s for the halvings, CMP and the sign for the limits; the hit
 * sum is LM, ADD, SBK. R11 is kept on the R10 stack around FxSpreadCars.
 * Entry: R8 = the car's record; R10 = stack; R11 = return address. Exit:
 * R1 = the impact (halved); R2, R0 and the flags from the last test; the
 * last RAM address the last field changed.
 */
FxRegs FxCarHit(FxRegs g)
{
    int i;

    g.r[1] = gsu_ldw(gsu_add(CAR_64, g.r[8], 0));
    g.r[0] = gsu_and(0x0008, g.r[1]);
    if (!gsu.z)
        return g;
    g.r[1] = gsu_ldw(gsu_add(CAR_FLAGS40, g.r[8], 0));
    g.r[0] = gsu_and(0x2201, g.r[1]);
    if (gsu.z) {
        g.r[1] = gsu_ldw(gsu_add(CAR_8A, g.r[8], 0));
        g.r[0] = gsu_and(0x0200, g.r[1]);
    }
    if (!gsu.z) {
        g.r[1] = 0;
        g.r[0] = gsu_add(CAR_IMPACT, g.r[8], 0);
        gsu_stw(g.r[0], g.r[1]);
        return g;
    }
    g.r[0] = gsu_div2(gsu_div2(gsu_ldw(FX_HIT_X + 2)));
    g.r[1] = g.r[0];
    g.r[0] = gsu_add(CAR_IMPACT, g.r[8], 0);
    gsu_stw(g.r[0], g.r[1]);
    g.r[0] = magnitude(g.r[1]);
    g.r[1] = gsu_ldw(gsu_add(CAR_44, g.r[8], 0));
    g.r[2] = 0x0002;
    gsu_sub(g.r[1], g.r[2], 0);
    if (gsu.s)
        g.r[0] = gsu_div2(gsu_div2(g.r[0]));
    g.r[1] = g.r[0];
    g.r[0] = gsu_ldw(gsu_add(CAR_VY, g.r[8], 0));
    gsu_moves(g.r[0]);
    if (gsu.s)
        g.r[1] = 0;
    for (i = 0; i < 2; i++) {
        g.r[0] = magnitude(gsu_ldw(i ? FX_HIT_X + 4 : FX_HIT_X));
        g.r[0] = gsu_div2(gsu_div2(gsu_div2(g.r[0])));
        g.r[1] = gsu_add(g.r[1], g.r[0], 0);
    }
    g.r[6] = gsu_ldw(FX_HIT_SCALE);
    g.r[1] = gsu_fmult(g.r[1], g.r[6]);
    g.r[0] = gsu_ldw(gsu_add(REC_CONTACTS_OF_CAR, g.r[8], 0));
    gsu_moves(g.r[0]);
    if (!gsu.z)
        g.r[1] = gsu_div2(g.r[1]);
    g.r[2] = gsu_ldw(FX_HIT_ON);
    gsu_moves(g.r[2]);
    {
        int on = !gsu.z;

        g.r[0] = gsu_sub(g.r[0], g.r[0], 0);      /* in the branch's delay slot */
        if (!on)
            gsu_stw(FX_HIT_SUM, g.r[0]);
    }
    g.r[0] = gsu_add(gsu_ldw(FX_HIT_SUM), g.r[1], 0);
    gsu_sbk(g.r[0]);
    g.r[1] = g.r[0];
    gsu_moves(g.r[2]);
    if (gsu.z)
        return g;
    g.r[1] = gsu_div2(g.r[1]);
    g.r[2] = 0x0010;
    gsu_sub(g.r[1], g.r[2], 0);
    if (gsu.s)
        return g;
    g.r[2] = 0x0060;
    gsu_sub(g.r[1], g.r[2], 0);
    if (!gsu.s)
        g.r[1] = g.r[2];
    g.r[0] = gsu_add(gsu_ldw(gsu_add(CAR_DAMAGE, g.r[8], 0)), g.r[1], 0);
    gsu_sbk(g.r[0]);
    g.r[2] = 0x0018;
    gsu_sub(g.r[1], g.r[2], 0);
    if (gsu.s) {
        car_or(&g, CAR_FLAGS40, 0x0080);
    } else {
        car_or(&g, CAR_70, 0x0080);
        g.r[2] = 0x0038;
        gsu_sub(g.r[1], g.r[2], 0);
        if (gsu.s)
            car_or(&g, CAR_FLAGS40, 0x0040);
        else
            car_or(&g, CAR_FLAGS40, 0x0020);
    }
    g.r[0] = gsu_ldw(gsu_add(CAR_FLAGS40, g.r[8], 0));
    g.r[2] = 0x0020;
    g.r[0] = gsu_and(g.r[0], g.r[2]);
    if (!gsu.z) {
        g.r[0] = gsu_ldw(FX_PLAYER);
        gsu_sub(g.r[0], g.r[8], 0);
        if (gsu.z) {
            uint16_t ret = g.r[11];

            gsu_push(&g.r[10], ret);
            g.r[11] = 0xBC52;
            g = FxSpreadCars(g);
            g.r[11] = gsu_pull(&g.r[10]);
        }
    }
    g.r[0] = gsu_ldw(gsu_add(CAR_DAMAGE, g.r[8], 0));
    g.r[2] = 0x0200;
    gsu_sub(g.r[0], g.r[2], 0);
    if (!gsu.s) {
        g.r[2] = gsu_ldw(gsu_add(CAR_FLAGS40, g.r[8], 0));
        g.r[0] = 0x0001;
        g.r[0] = gsu_or(g.r[0], g.r[2]);
        gsu_sbk(g.r[0]);
    }
    return g;
}

void abi_FxCarHit(void)
{
    FxRegs g;

    memcpy(g.r, gsu.r, sizeof g.r);
    g = FxCarHit(g);
    memcpy(gsu.r, g.r, 15 * sizeof g.r[0]);
}
