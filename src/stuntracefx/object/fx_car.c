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
/* gsuwalk: the code of each copy of script_word, read from the ROM */
static const GsuCode script_word_fetch[21][4] = {
    {{0x048dad, 2}, {0x048daf, 1}, {0x048db0, 3}, {0x048db3, 1}},
    {{0x048db4, 2}, {0x048db6, 1}, {0x048db7, 3}, {0x048dba, 1}},
    {{0x048dd1, 1}, {0x048dd2, 1}, {0x048dd3, 2}, {0x048dd5, 1}},
    {{0x048dd6, 2}, {0x048dd8, 1}, {0x048dd9, 3}, {0x048ddc, 1}},
    {{0x048de2, 2}, {0x048de4, 1}, {0x048de5, 3}, {0x048de8, 1}},
    {{0x048dee, 2}, {0x048df0, 1}, {0x048df1, 3}, {0x048df4, 1}},
    {{0x048dfa, 2}, {0x048dfc, 1}, {0x048dfd, 3}, {0x048e00, 1}},
    {{0x048e06, 2}, {0x048e08, 1}, {0x048e09, 3}, {0x048e0c, 1}},
    {{0x048e12, 2}, {0x048e14, 1}, {0x048e15, 3}, {0x048e18, 1}},
    {{0x048e1e, 2}, {0x048e20, 1}, {0x048e21, 3}, {0x048e24, 1}},
    {{0x048e2a, 2}, {0x048e2c, 1}, {0x048e2d, 3}, {0x048e30, 1}},
    {{0x048e42, 2}, {0x048e44, 1}, {0x048e45, 3}, {0x048e48, 1}},
    {{0x048e4e, 2}, {0x048e50, 1}, {0x048e51, 3}, {0x048e54, 1}},
    {{0x048f50, 2}, {0x048f52, 1}, {0x048f53, 3}, {0x048f56, 1}},
    {{0x048ea7, 2}, {0x048ea9, 1}, {0x048eaa, 3}, {0x048ead, 1}},
    {{0x048eb3, 2}, {0x048eb5, 1}, {0x048eb6, 3}, {0x048eb9, 1}},
    {{0x048ec2, 2}, {0x048ec4, 1}, {0x048ec5, 3}, {0x048ec8, 1}},
    {{0x048ece, 2}, {0x048ed0, 1}, {0x048ed1, 3}, {0x048ed4, 1}},
    {{0x048eda, 2}, {0x048edc, 1}, {0x048edd, 3}, {0x048ee0, 1}},
    {{0x048ee6, 2}, {0x048ee8, 1}, {0x048ee9, 3}, {0x048eec, 1}},
    {{0x048ef2, 2}, {0x048ef4, 1}, {0x048ef5, 3}, {0x048ef8, 1}},
};

static uint16_t script_word(uint16_t *r14, int copy)
{
    gsu_codes(script_word_fetch[copy], 1);
    uint16_t w = gsu_getb();

    gsu_codes(script_word_fetch[copy] + 1, 1);
    gsu_r14(gsu_inc(*r14));
    *r14 = gsu.r[14];
    gsu_codes(script_word_fetch[copy] + 2, 1);
    w = gsu_getbh(w);
    gsu_codes(script_word_fetch[copy] + 3, 1);
    gsu_r14(gsu_inc(*r14));
    *r14 = gsu.r[14];
    return w;
}

/* Field `field` of record `rec` = R3, through R0. */
/* gsuwalk: the code of each copy of field_set, read from the ROM */
static const GsuCode field_set_fetch[14][1] = {
    {{0x048eae, 5}},
    {{0x048eba, 8}},
    {{0x048ec9, 5}},
    {{0x048ed5, 5}},
    {{0x048ee1, 5}},
    {{0x048eed, 5}},
    {{0x048ef9, 5}},
    {{0x048f04, 5}},
    {{0x048f0e, 8}},
    {{0x048f16, 5}},
    {{0x048f1b, 5}},
    {{0x048f26, 10}},
    {{0x048f34, 5}},
    {{0x048f3f, 5}},
};

static void field_set(FxRegs *g, uint16_t field, uint16_t rec, int copy)
{
    g->r[0] = gsu_add(field, rec, 0);
    gsu_codes(field_set_fetch[copy], 1);
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
/* gsuwalk: the copy of script_word each copy of FxCarAddPart calls, by pass */
static const uint8_t script_word_in_FxCarAddPart[1][7] = {{14, 15, 16, 17, 18, 19, 20}};

/* gsuwalk: the copy of field_set each copy of FxCarAddPart calls, by pass */
static const uint8_t field_set_in_FxCarAddPart[1][7] = {{0, 1, 2, 3, 4, 5, 6}};

FxRegs FxCarAddPart(FxRegs g, int copy)
{
    static const uint8_t fields[7] = {0x12, 0x3C, 0x3E, 0x40, 0x44, 0x50, 0x46};
    uint16_t *sp = &g.r[10];
    int i;

    gsu_code(0x048e85, 2);
    gsu_push(sp, g.r[11]);
    gsu_code(0x048e87, 4);
    gsu_push(sp, g.r[1]);
    g.r[4] = PART_BYTES;
    g.r[3] = FX_POOL_FIRST;
    g.r[11] = 0x8E97;
    gsu_code(0x048e8b, 12); gsu_code(0x04893a, 1);
    g = FxPoolTake(g, GSU_COPY(0));
    g.r[2] = g.r[1];
    gsu_code(0x048e97, 7);
    g.r[1] = gsu_pull(sp);
    gsu_moves(g.r[2]);
    if (gsu.z) {
        gsu_code(0x048e9e, 8); gsu_code(0x048f43, 5);
        gsu_pull(sp);
        return g;
    } else { gsu_code(0x048e9e, 4); gsu_code(0x048ea6, 1); }
    for (i = 0; i < 7; i++) {
        g.r[3] = script_word(&g.r[14], GSU_COPY(script_word_in_FxCarAddPart[0][i]));
        if (fields[i] == 0x3C)
            { g.r[3] = gsu_inc(gsu_not(g.r[3])); }
        field_set(&g, fields[i], g.r[2], GSU_COPY(field_set_in_FxCarAddPart[0][i]));
    }
    gsu_code(0x048efe, 6);
    g.r[3] = gsu_ldw(gsu_add(0x3E, g.r[1], 0));     /* the car's first part */
    field_set(&g, 0x34, g.r[2], GSU_COPY(7));
    g.r[0] = gsu_add(0x3E, g.r[1], 0);
    gsu_code(0x048f09, 5);
    gsu_stw(g.r[0], g.r[2]);
    g.r[3] = 0x0014;
    field_set(&g, 0x0C, g.r[2], GSU_COPY(8));
    field_set(&g, 0x0E, g.r[2], GSU_COPY(9));
    field_set(&g, 0x10, g.r[2], GSU_COPY(10));
    gsu_code(0x048f20, 6);
    g.r[3] = gsu_ldw(gsu_add(0x16, g.r[2], 0));
    g.r[0] = 0x0004;
    g.r[3] = gsu_or(g.r[0], g.r[3]);
    field_set(&g, 0x16, g.r[2], GSU_COPY(11));
    gsu_code(0x048f30, 4);
    g.r[3] = gsu_ldw(FX_PART_MODE);
    field_set(&g, 0x1A, g.r[2], GSU_COPY(12));
    gsu_code(0x048f39, 6);
    g.r[3] = gsu_ldw(gsu_add(0x2A, g.r[1], 0));
    field_set(&g, 0x2A, g.r[2], GSU_COPY(13));
    gsu_code(0x048f44, 4);
    gsu_pull(sp);
    return g;
}

void abi_FxCarAddPart(void)
{
    FxRegs g;

    memcpy(g.r, gsu.r, sizeof g.r);
    g = FxCarAddPart(g, GSU_COPY(0));
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
/* gsuwalk: the code of each copy of FxCarFromScript, read from the ROM */
static const GsuCode FxCarFromScript_fetch[1][12] = {
    {{0, 0}, {0, 0}, {0x048fa3, 4}, {0, 0}, {0, 0}, {0, 0}, {0x048f92, 8}, {0x048f9a, 9}, {0x048fa7, 6}, {0x048fad, 8}, {0x048fb5, 9}, {0x048fbe, 8}},
};

FxRegs FxCarFromScript(FxRegs g)
{
    static const struct { uint8_t field; uint16_t value; } set[6] = {
        {0x6A, 0x0180}, {0x8C, 0x1000}, {0x8A, 0}, {0x7A, 0x0000}, {0x86, 0x0002}, {0x58, 0x0001},
    };
    uint16_t *sp = &g.r[10];
    uint32_t p;
    int i;

    gsu_code(0x048f49, 2);
    gsu_push(sp, g.r[11]);
    g.r[0] = gsu_sub(g.r[0], g.r[0], 0);
    gsu_code(0x048f4b, 4);
    g.r[0] = (gsu_getb());
    gsu_code(0x048f4f, 1);
    gsu_r14(gsu_inc(g.r[14]));
    g.r[14] = gsu.r[14];
    g.r[2] = script_word(&g.r[14], GSU_COPY(13));
    gsu_code(0x048f57, 4);
    gsu_stw(FX_PART_MODE, g.r[2]);
    g.r[12] = g.r[0];
    gsu_moves(g.r[12]);
    if (!gsu.z) {
        g.r[13] = 0x8F61;
        gsu_code(0x048f5b, 7);
        do {
            gsu_code(0x048f62, 2);
            gsu_push(sp, g.r[13]);
            gsu_code(0x048f64, 4);
            gsu_push(sp, g.r[12]);
            g.r[11] = 0x8F6E;
            
            gsu_code(0x048f68, 6); gsu_code(0x048e84, 1);
            g = FxCarAddPart(g, GSU_COPY(0));
            gsu_code(0x048f6e, 5);
            g.r[12] = gsu_pull(sp);
            gsu_code(0x048f73, 4);
            g.r[13] = gsu_pull(sp);
        } while (gsu_code_loop(0x048f61, 1, gsu_code_loop(0x048f77, 1, gsu_count_down(&g.r[12]))));
        gsu_code(0x048f77, 1);
        gsu_code(0x048f78, 1);
    } else { gsu_code(0x048f5b, 4); gsu_code(0x048f78, 1); }
    g.r[0] = 0x8090;
    g.r[0] = gsu_sub(g.r[14], g.r[0], 0);
    gsu_code(0x048f79, 9);
    gsu_stw(FX_SCRIPT_AT, g.r[0]);
    g.r[0] = gsu_sub(g.r[0], g.r[0], 0);
    gsu_code(0x048f82, 2);
    g.r[0] = (gsu_getb());
    gsu_code(0x048f84, 1);
    gsu_r14(gsu_inc(g.r[14]));
    g.r[14] = gsu.r[14];
    g.r[6] = 5;
    p = gsu_lmult(g.r[0], g.r[6]);
    g.r[0] = (uint16_t)(p >> 16);
    g.r[4] = (uint16_t)p;
    gsu_code(0x048f85, 6);
    gsu_r14(gsu_add(g.r[14], g.r[4], 0));
    g.r[14] = gsu.r[14];
    g.r[2] = g.r[14];
    g.r[0] = gsu_add(0x6E, g.r[1], 0);
    gsu_code(0x048f8b, 7);
    gsu_stw(g.r[0], g.r[2]);
    for (i = 0; i < 6; i++) {
        gsu_codes(FxCarFromScript_fetch[0] + i, 1);
        g.r[2] = set[i].field == 0x8A ? (gsu_ldw(FX_CAR_26F2)) : set[i].value;
        g.r[0] = gsu_add(set[i].field, g.r[1], 0);
        gsu_codes(FxCarFromScript_fetch[0] + 6 + i, 1);
        gsu_stw(g.r[0], g.r[2]);
    }
    gsu_code(0x048fc6, 4);
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
 * gets velocity 0 instead. The bytes after each jump back to the walk
 * are filed on their own (FxCarMovePartsAfterWalk, 04:A5E4, and
 * FxCarMovePartsAfterStill, 04:A601): no path reaches them.
 * HOW: R12 keeps R8 while R8 and R11 walk the offset and its turned copy;
 * the turn is three rows of FMULT, ROL; the change times 8 is LMULT by R6
 * = 8 (R4 takes the low word). The return address is kept on the R10
 * stack. The routine runs under CACHE (the code cache only; nothing the
 * C has to do).
 * Entry: R8 = the car's record; R10 = stack; R11 = return address. Exit:
 * R9 = 0; R8 = R12 = the car (R12 as on entry without parts); the others
 * from the last part; S, Z from R10; the last RAM address R10.
 */
/* gsuwalk: the code of each copy of FxCarMoveParts, read from the ROM */
static const GsuCode FxCarMoveParts_fetch[1][36] = {
    {{0x04a507, 10}, {0x04a5e8, 8}, {0x04a5f0, 5}, {0, 0}, {0x04a5f5, 5}, {0, 0}, {0x04a512, 14}, {0x04a520, 4}, {0x04a524, 4}, {0x04a528, 5}, {0x04a541, 5}, {0x04a55a, 5}, {0x04a52d, 7}, {0x04a546, 7}, {0x04a55f, 7}, {0x04a534, 8}, {0x04a54d, 8}, {0x04a566, 8}, {0x04a53c, 5}, {0x04a555, 5}, {0x04a56e, 5}, {0x04a573, 13}, {0x04a59b, 6}, {0x04a5bc, 6}, {0x04a580, 4}, {0x04a5a1, 4}, {0x04a5c2, 4}, {0x04a584, 6}, {0x04a5a5, 6}, {0x04a5c6, 6}, {0x04a58a, 7}, {0x04a5ab, 7}, {0x04a5cc, 7}, {0x04a591, 10}, {0x04a5b2, 10}, {0x04a5d3, 10}},
};

FxRegs FxCarMoveParts(FxRegs g, int copy)
{
    uint32_t p;
    int i;

    gsu_code(0x04a4ee, 2);
    gsu_push(&g.r[10], g.r[11]);
    gsu_code(0x04a4f0, 6); gsu_cache(0xa4f6);
    g.r[0] = gsu_ldw(gsu_add(CAR_PARTS, g.r[8], 0));
    gsu_code(0x04a4f6, 1);
    for (;;) {
        g.r[9] = g.r[0];
        gsu_moves(g.r[9]);
        if (gsu.z)
            { gsu_code(0x04a4f7, 4);
            break; } else { gsu_code(0x04a4f7, 4); gsu_code(0x04a4ff, 1); }
        gsu_code(0x04a500, 7);
        g.r[1] = gsu_ldw(gsu_add(PART_FLAGS, g.r[9], 0));
        g.r[0] = gsu_and(0x0400, g.r[1]);
        if (!gsu.z) {
            g.r[1] = 0;
            for (i = 0; i < 3; i++) {
                g.r[0] = gsu_add((uint16_t)(REC_VEL + 2 * i), g.r[9], 0);
                gsu_codes(FxCarMoveParts_fetch[0] + 2 * i, 2);
                gsu_stw(g.r[0], g.r[1]);
            }
            gsu_code(0x04a5fa, 4);
            g.r[0] = gsu_ldw(gsu_add(PART_NEXT, g.r[9], 0));
            gsu_code(0x04a5fe, 3); gsu_code(0x04a4f6, 1);
            continue;
        } else { gsu_code(0x04a507, 6); gsu_code(0x04a511, 1); }
        g.r[12] = g.r[8];
        g.r[8] = gsu_add(PART_OFFSET, g.r[9], 0);
        g.r[11] = gsu_add(PART_WORLD, g.r[9], 0);
        for (i = 1; i <= 3; i++) {
            gsu_codes(FxCarMoveParts_fetch[0] + 6 + (i - 1), 1);
            g.r[i] = gsu_ldw(g.r[8]);
            g.r[8] = gsu_inc(gsu_inc(g.r[8]));
        }
        for (i = 0; i < 3; i++) {
            uint16_t row = (uint16_t)(FX_MATRIX + 2 * i);

            gsu_codes(FxCarMoveParts_fetch[0] + 9 + i, 1);
            g.r[6] = gsu_ldw(row);
            g.r[5] = gsu_rol(gsu_fmult(g.r[1], g.r[6]));
            gsu_codes(FxCarMoveParts_fetch[0] + 12 + i, 1);
            g.r[6] = gsu_ldw((uint16_t)(row + 6));
            g.r[5] = gsu_add(g.r[5], gsu_rol(gsu_fmult(g.r[2], g.r[6])), 0);
            gsu_codes(FxCarMoveParts_fetch[0] + 15 + i, 1);
            g.r[6] = gsu_ldw((uint16_t)(row + 12));
            g.r[0] = gsu_add(gsu_rol(gsu_fmult(g.r[3], g.r[6])), g.r[5], 0);
            gsu_codes(FxCarMoveParts_fetch[0] + 18 + i, 1);
            gsu_stw(g.r[11], g.r[0]);
            g.r[11] = gsu_inc(gsu_inc(g.r[11]));
        }
        g.r[8] = g.r[12];
        g.r[6] = 8;
        for (i = 0; i < 3; i++) {
            gsu_codes(FxCarMoveParts_fetch[0] + 21 + i, 1);
            g.r[1] = gsu_ldw(gsu_add((uint16_t)(PART_WORLD + 2 * i), g.r[9], 0));
            gsu_codes(FxCarMoveParts_fetch[0] + 24 + i, 1);
            g.r[0] = gsu_ldw((uint16_t)(FX_CAR_POS + 2 * i));
            gsu_codes(FxCarMoveParts_fetch[0] + 27 + i, 1);
            g.r[2] = gsu_ldw(gsu_add((uint16_t)(REC_POS + 2 * i), g.r[9], 0));
            g.r[1] = gsu_add(g.r[1], g.r[0], 0);
            g.r[0] = gsu_add((uint16_t)(REC_POS + 2 * i), g.r[9], 0);
            gsu_codes(FxCarMoveParts_fetch[0] + 30 + i, 1);
            gsu_stw(g.r[0], g.r[1]);
            g.r[1] = gsu_sub(g.r[1], g.r[2], 0);
            p = gsu_lmult(g.r[1], g.r[6]);
            g.r[1] = (uint16_t)(p >> 16);
            g.r[4] = (uint16_t)p;
            g.r[0] = gsu_add((uint16_t)(REC_VEL + 2 * i), g.r[9], 0);
            gsu_codes(FxCarMoveParts_fetch[0] + 33 + i, 1);
            gsu_stw(g.r[0], g.r[4]);
        }
        gsu_code(0x04a5dd, 4);
        g.r[0] = gsu_ldw(gsu_add(PART_NEXT, g.r[9], 0));
        gsu_code(0x04a5e1, 3); gsu_code(0x04a4f6, 1);
    }
    gsu_code(0x04a4fb, 4); gsu_code(0x04a602, 5);
    gsu_pull(&g.r[10]);
    return g;
}

void abi_FxCarMoveParts(void)
{
    FxRegs g;

    memcpy(g.r, gsu.r, sizeof g.r);
    g = FxCarMoveParts(g, GSU_COPY(0));
    memcpy(gsu.r, g.r, 15 * sizeof g.r[0]);
}

/*
 * @routine 04:A5E4 FxCarMovePartsAfterWalk
 * WHAT IT DOES: The bytes after FxCarMoveParts' jump back to its walk (a
 * NOP, then a BRA to its return): no path reaches them, since the jump
 * before them always goes. A unit start runs them so the C is judged
 * against the original.
 * HOW: NOP; BRA to 04:A602 (the return, NOP in the delay slot); the
 * return pulls R15 from the R10 stack.
 * Entry: R10 = stack, the return address the word below it. Exit: R10
 * two lower; S, Z from R10.
 */
void abi_FxCarMovePartsAfterWalk(void)
{
    
    
    gsu_code(0x04a5e5, 3); gsu_code(0x04a602, 5);
    gsu_pull(&gsu.r[10]);
}

/*
 * @routine 04:A601 FxCarMovePartsAfterStill
 * WHAT IT DOES: The NOP after the still part's jump back to the walk in
 * FxCarMoveParts, which no path reaches (the jump before it always
 * goes); it runs on into the return. A unit start runs it so the C is
 * judged against the original.
 * HOW: NOP; then the return at 04:A602 pulls R15 from the R10 stack.
 * Entry: R10 = stack, the return address the word below it. Exit: R10
 * two lower; S, Z from R10.
 */
void abi_FxCarMovePartsAfterStill(void)
{
    gsu_code(0x04a602, 5);
    gsu_pull(&gsu.r[10]);
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
/* gsuwalk: the code of each copy of FxCarPose, read from the ROM */
static const GsuCode FxCarPose_fetch[1][12] = {
    {{0x04b9b3, 6}, {0x04b9be, 4}, {0x04b9c7, 4}, {0x04b9b9, 5}, {0x04b9c2, 5}, {0x04b9cb, 5}, {0x04b9e4, 4}, {0x04b9ec, 4}, {0x04b9f4, 4}, {0x04b9e8, 4}, {0x04b9f0, 4}, {0x04b9f8, 4}},
};

FxRegs FxCarPose(FxRegs g, int copy)
{
    static const uint8_t angle[3] = {CAR_YAW, CAR_PITCH, CAR_ROLL};
    int i;

    gsu_code(0x04b9ad, 2);
    gsu_push(&g.r[10], g.r[11]);
    gsu_code(0x04b9af, 4);
    gsu_push(&g.r[10], g.r[8]);
    for (i = 0; i < 3; i++) {
        gsu_codes(FxCarPose_fetch[0] + i, 1);
        g.r[0] = gsu_ldw(gsu_add(angle[i], g.r[8], 0));
        g.r[0] = gsu_inc(gsu_not(g.r[0]));
        gsu_codes(FxCarPose_fetch[0] + 3 + i, 1);
        gsu_stw((uint16_t)(FX_ANGLES_W + 2 * i), g.r[0]);
    }
    g.r[9] = FX_MATRIX;
    g.r[11] = 0xB9D7;
    memcpy(gsu.r, g.r, sizeof g.r);
    gsu_code(0x04b9d0, 7); gsu_code(0x04c1a7, 1);
    abi_FxMatrixFromAngles();
    memcpy(g.r, gsu.r, sizeof g.r);
    g.r[9] = FX_MATRIX;
    g.r[11] = 0xB9DF;
    memcpy(gsu.r, g.r, sizeof g.r);
    gsu_code(0x04b9d7, 8); gsu_code(0x04c2c8, 1);
    abi_FxMatrixTranspose04();
    memcpy(g.r, gsu.r, sizeof g.r);
    gsu_code(0x04b9df, 5);
    g.r[8] = gsu_pull(&g.r[10]);
    for (i = 0; i < 3; i++) {
        gsu_codes(FxCarPose_fetch[0] + 6 + i, 1);
        g.r[0] = gsu_ldw(gsu_add((uint16_t)(REC_POS + 2 * i), g.r[8], 0));
        gsu_codes(FxCarPose_fetch[0] + 9 + i, 1);
        gsu_stw((uint16_t)(FX_CAR_POS + 2 * i), g.r[0]);
    }
    g.r[11] = 0xBA00;
    gsu_code(0x04b9fc, 4); gsu_code(0x04a4ed, 1);
    g = FxCarMoveParts(g, GSU_COPY(0));
    gsu_code(0x04ba00, 5);
    gsu_pull(&g.r[10]);
    return g;
}

void abi_FxCarPose(void)
{
    FxRegs g;

    memcpy(g.r, gsu.r, sizeof g.r);
    g = FxCarPose(g, GSU_COPY(0));
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
/* gsuwalk: the code of each copy of car_set, read from the ROM */
static const GsuCode car_set_fetch[18][1] = {
    {{0x048d6b, 6}},
    {{0x048d75, 5}},
    {{0x048d7e, 5}},
    {{0x048d87, 5}},
    {{0x048d90, 5}},
    {{0x048d99, 5}},
    {{0x048d9e, 7}},
    {{0x048da5, 8}},
    {{0x048ddd, 5}},
    {{0x048de9, 5}},
    {{0x048df5, 5}},
    {{0x048e01, 5}},
    {{0x048e0d, 5}},
    {{0x048e19, 5}},
    {{0x048e25, 5}},
    {{0x048e31, 5}},
    {{0x048e49, 5}},
    {{0x048e55, 5}},
};

static void car_set(FxRegs *g, uint16_t f, uint16_t value, int copy)
{
    g->r[2] = value;
    g->r[0] = gsu_add(f, g->r[1], 0);
    gsu_codes(car_set_fetch[copy], 1);
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
 * Entry: the S-CPU starts it. Exit: (at its STOP) R1 = R8 = the car; R10
 * = $0240; the others as FxCarPose leaves them.
 */
/* gsuwalk: the code of each copy of FxCarNew, read from the ROM */
static const GsuCode FxCarNew_fetch[1][36] = {
    {{0x048cd3, 7}, {0x048cde, 1}, {0x048cdf, 8}, {0x048ceb, 1}, {0x048cec, 8}, {0x048cf8, 1}, {0x048cf9, 8}, {0x048d05, 1}, {0x048d06, 8}, {0x048d12, 1}, {0x048d13, 8}, {0x048d1f, 1}, {0x048d20, 8}, {0x048d2c, 1}, {0x048d2d, 8}, {0x048d39, 1}, {0x048d3a, 8}, {0x048d46, 1}, {0x048cd3, 11}, {0x048d4e, 1}, {0x048cdf, 12}, {0x048d4e, 1}, {0x048cec, 12}, {0x048d4e, 1}, {0x048cf9, 12}, {0x048d4e, 1}, {0x048d06, 12}, {0x048d4e, 1}, {0x048d13, 12}, {0x048d4e, 1}, {0x048d20, 12}, {0x048d4e, 1}, {0x048d2d, 12}, {0x048d4e, 1}, {0x048d3a, 12}, {0x048d4e, 1}},
};

/* gsuwalk: the copy of car_set each copy of FxCarNew calls, by pass */
static const uint8_t car_set_in_FxCarNew[1][8] = {{8, 9, 10, 11, 12, 13, 14, 15}};

/* gsuwalk: the copy of script_word each copy of FxCarNew calls, by pass */
static const uint8_t script_word_in_FxCarNew[1][8] = {{3, 4, 5, 6, 7, 8, 9, 10}};

FxRegs FxCarNew(FxRegs g)
{
    static const uint16_t known[9] = {0x81B9, 0x853D, 0x82E5, 0x8411, 0x8669, 0x876E, 0x8AA8, 0x8886, 0x8990};
    static const uint8_t first[8] = {0x5A, 0x5C, 0x16, 0x40, 0x1A, 0x0C, 0x0E, 0x10};
    int i;

    g.r[10] = 0x0240;
    g.r[0] = FX_SCRIPT_BANK;
    gsu_code(0x048cc7, 8);
    gsu_romb(FX_SCRIPT_BANK);
    gsu_code(0x048ccf, 4);
    gsu_r14(gsu_ldw(FX_NEW_RECORD));
    g.r[14] = gsu.r[14];
    for (i = 0; i < 9; i++) {
        g.r[0] = known[i];
        gsu_sub(g.r[0], g.r[14], 0);
        if (gsu.z)
            { gsu_codes(FxCarNew_fetch[0] + 18 + 2 * i, 2); goto known_car; } else { gsu_codes(FxCarNew_fetch[0] + 2 * i, 2); }                       /* IWT R15 on, from each test */
    }
    gsu_code(0x048d47, 5);
    gsu_stw(FX_CAR_BAD, g.r[14]);
    gsu_code(0x048d4c, 3);
    gsu_r14(FX_CAR_DEFAULT);
    g.r[14] = gsu.r[14];
known_car:
    g.r[4] = CAR_BYTES;
    g.r[3] = 0x1098;
    g.r[11] = 0x8D59;
    gsu_code(0x048d4f, 10); gsu_code(0x04893a, 1);
    g = FxPoolTake(g, GSU_COPY(0));
    gsu_code(0x048d59, 5);
    gsu_stw(FX_NEW_RECORD, g.r[1]);
    gsu_moves(g.r[1]);
    if (gsu.z)
        { gsu_code(0x048d5e, 8); gsu_code(0x048e82, 1);
        return g; } else { gsu_code(0x048d5e, 4); gsu_code(0x048d66, 1); }
    gsu_code(0x048d67, 4);
    car_set(&g, 0x96, gsu_ldw(FX_CAR_2392), GSU_COPY(0));
    gsu_code(0x048d71, 4);
    car_set(&g, 0x1E, gsu_ldw(0x0068), GSU_COPY(1));
    gsu_code(0x048d7a, 4);
    car_set(&g, 0x20, gsu_ldw(0x002C), GSU_COPY(2));
    gsu_code(0x048d83, 4);
    car_set(&g, 0x22, gsu_ldw(0x002E), GSU_COPY(3));
    gsu_code(0x048d8c, 4);
    car_set(&g, 0x24, gsu_ldw(0x0022), GSU_COPY(4));
    gsu_code(0x048d95, 4);
    car_set(&g, 0x64, gsu_ldw(FX_CAR_AMOUNT), GSU_COPY(5));
    car_set(&g, 0x66, g.r[14], GSU_COPY(6));
    car_set(&g, 0x0A, 0xB1A1, GSU_COPY(7));
    g.r[2] = script_word(&g.r[14], GSU_COPY(0));
    g.r[3] = script_word(&g.r[14], GSU_COPY(1));
    gsu_code(0x048dbb, 4);
    gsu_stw(FX_CAR_AMOUNT, g.r[2]);
    gsu_code(0x048dbf, 4);
    g.r[0] = gsu_lob(gsu_ldw(0x108C));
    if (!gsu.z)
        { gsu_code(0x048dc3, 8);
        gsu_stw(FX_CAR_AMOUNT, g.r[3]); } else { gsu_code(0x048dc3, 3); gsu_code(0x048dca, 1); }
    gsu_code(0x048dcb, 1);
    g.r[0] = gsu_getb();
    gsu_code(0x048dcc, 1);
    gsu_r14(gsu_inc(g.r[14]));
    g.r[14] = gsu.r[14];
    gsu_code(0x048dcd, 4);
    gsu_stw(FX_CAR_2394, g.r[0]);
    g.r[0] = script_word(&g.r[14], GSU_COPY(2));
    for (i = 0; i < 8; i++)
        car_set(&g, first[i], script_word(&g.r[14], GSU_COPY(script_word_in_FxCarNew[0][i])), GSU_COPY(car_set_in_FxCarNew[0][i]));
    g.r[0] = 0x8090;
    g.r[0] = gsu_sub(g.r[14], g.r[0], 0);
    gsu_code(0x048e36, 9);
    gsu_stw(FX_CAR_2396, g.r[0]);
    gsu_code(0x048e3f, 3);
    gsu_r14(gsu_add(g.r[14], 10, 0));
    g.r[14] = gsu.r[14];
    car_set(&g, 0x2A, script_word(&g.r[14], GSU_COPY(11)), GSU_COPY(16));
    car_set(&g, 0x14, script_word(&g.r[14], GSU_COPY(12)), GSU_COPY(17));
    gsu_code(0x048e5a, 4);
    g.r[0] = gsu_ldw(FX_CARS);
    g.r[2] = g.r[0];
    gsu_moves(g.r[2]);
    if (!gsu.z) {                              /* each way its own SM */
        g.r[0] = gsu_add(0x74, g.r[1], 0);
        gsu_code(0x048e5e, 4); gsu_code(0x048e69, 6);
        gsu_stw(g.r[0], g.r[2]);
        gsu_code(0x048e6f, 4);
        gsu_stw(FX_CARS, g.r[1]);
    } else {
        gsu_code(0x048e5e, 9);
        gsu_stw(FX_CARS, g.r[1]);
        gsu_code(0x048e67, 2); gsu_code(0x048e72, 1);
    }
    g.r[11] = 0x8E77;
    gsu_code(0x048e73, 4); gsu_code(0x048f48, 1);
    g = FxCarFromScript(g);
    gsu_code(0x048e77, 5);
    gsu_stw(FX_CAR_LAST, g.r[1]);
    g.r[8] = g.r[1];
    g.r[11] = 0x8E82;
    gsu_code(0x048e7c, 6); gsu_code(0x04b9ac, 1);
    g = FxCarPose(g, GSU_COPY(0));
    gsu_code(0x048e82, 1);
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
/* gsuwalk: the code of each copy of magnitude, read from the ROM */
static const GsuCode magnitude_fetch[3][3] = {
    {{0x04bb5b, 9}, {0x04bb5b, 6}, {0x04bb63, 1}},
    {{0x04bb88, 7}, {0x04bb88, 4}, {0x04bb8e, 1}},
    {{0x04bb9b, 7}, {0x04bb9b, 4}, {0x04bba1, 1}},
};

static uint16_t magnitude(uint16_t v, int copy)
{
    gsu_moves(v);
    return gsu.s ? (gsu_codes(magnitude_fetch[copy], 1), (gsu_inc(gsu_not(v)))) : (gsu_codes(magnitude_fetch[copy] + 1, 2), v);
}

/* Bits `bits` set in field `f` of car R8 (LDW, OR, SBK), through R2. */
/* gsuwalk: the code of each copy of car_or, read from the ROM */
static const GsuCode car_or_fetch[4][3] = {
    {{0x04bc22, 4}, {0x04bc26, 6}, {0x04bc35, 1}},
    {{0x04bc05, 4}, {0x04bc09, 5}, {0, 0}},
    {{0x04bc2d, 4}, {0x04bc31, 5}, {0, 0}},
    {{0x04bc17, 4}, {0x04bc1b, 6}, {0x04bc35, 1}},
};

static void car_or(FxRegs *g, uint16_t f, uint16_t bits, int copy)
{
    gsu_codes(car_or_fetch[copy], 1);
    g->r[0] = gsu_ldw(gsu_add(f, g->r[8], 0));
    g->r[2] = bits;
    g->r[0] = gsu_or(g->r[0], g->r[2]);
    gsu_codes(car_or_fetch[copy] + 1, 2);
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
/* gsuwalk: the code of each copy of FxCarHit, read from the ROM */
static const GsuCode FxCarHit_fetch[1][2] = {
    {{0x04bb84, 4}, {0x04bb8f, 12}},
};

/* gsuwalk: the copy of magnitude each copy of FxCarHit calls, by pass */
static const uint8_t magnitude_in_FxCarHit[1][2] = {{1, 2}};

FxRegs FxCarHit(FxRegs g, int copy)
{
    int i;

    gsu_code(0x04bb18, 6);
    g.r[1] = gsu_ldw(gsu_add(CAR_64, g.r[8], 0));
    g.r[0] = gsu_and(0x0008, g.r[1]);
    if (!gsu.z)
        { gsu_code(0x04bb1e, 10); gsu_code(0x04bc6e, 3);
        return g; } else { gsu_code(0x04bb1e, 6); gsu_code(0x04bb28, 1); }
    gsu_code(0x04bb29, 6);
    g.r[1] = gsu_ldw(gsu_add(CAR_FLAGS40, g.r[8], 0));
    g.r[0] = gsu_and(0x2201, g.r[1]);
    if (!gsu.z)
        { gsu_code(0x04bb2f, 10); gsu_code(0x04bc71, 1); goto cleared; } else { gsu_code(0x04bb2f, 6); gsu_code(0x04bb39, 1); }                             /* IWT R15 to the clearing */
    gsu_code(0x04bb3a, 7);
    g.r[1] = gsu_ldw(gsu_add(CAR_8A, g.r[8], 0));
    g.r[0] = gsu_and(0x0200, g.r[1]);
    if (!gsu.z)
        { gsu_code(0x04bb41, 10); gsu_code(0x04bc71, 1); goto cleared; } else { gsu_code(0x04bb41, 6); gsu_code(0x04bb4b, 1); }
    gsu_code(0x04bb4c, 4);
    g.r[0] = gsu_div2(gsu_div2(gsu_ldw(FX_HIT_X + 2)));
    g.r[1] = g.r[0];
    g.r[0] = gsu_add(CAR_IMPACT, g.r[8], 0);
    gsu_code(0x04bb50, 11);
    gsu_stw(g.r[0], g.r[1]);
    g.r[0] = magnitude(g.r[1], GSU_COPY(0));
    gsu_code(0x04bb64, 6);
    g.r[1] = gsu_ldw(gsu_add(CAR_44, g.r[8], 0));
    g.r[2] = 0x0002;
    gsu_sub(g.r[1], g.r[2], 0);
    if (gsu.s)
        { g.r[0] = gsu_div2(gsu_div2(g.r[0])); gsu_code(0x04bb6a, 13); } else { gsu_code(0x04bb6a, 8); gsu_code(0x04bb76, 1); }
    g.r[1] = g.r[0];
    gsu_code(0x04bb77, 6);
    g.r[0] = gsu_ldw(gsu_add(CAR_VY, g.r[8], 0));
    gsu_moves(g.r[0]);
    if (gsu.s)
        { g.r[1] = 0; gsu_code(0x04bb7d, 7); } else { gsu_code(0x04bb7d, 4); gsu_code(0x04bb83, 1); }
    for (i = 0; i < 2; i++) {
        gsu_codes(FxCarHit_fetch[0] + i, 1);
        g.r[0] = magnitude(gsu_ldw(i ? FX_HIT_X + 4 : FX_HIT_X), GSU_COPY(magnitude_in_FxCarHit[0][i]));
        g.r[0] = gsu_div2(gsu_div2(gsu_div2(g.r[0])));
        g.r[1] = gsu_add(g.r[1], g.r[0], 0);
    }
    gsu_code(0x04bba2, 12);
    g.r[6] = gsu_ldw(FX_HIT_SCALE);
    g.r[1] = gsu_fmult(g.r[1], g.r[6]);
    gsu_code(0x04bbae, 6);
    g.r[0] = gsu_ldw(gsu_add(REC_CONTACTS_OF_CAR, g.r[8], 0));
    gsu_moves(g.r[0]);
    if (!gsu.z)
        { g.r[1] = gsu_div2(g.r[1]); gsu_code(0x04bbb4, 8); } else { gsu_code(0x04bbb4, 4); gsu_code(0x04bbbb, 1); }
    gsu_code(0x04bbbc, 4);
    g.r[2] = gsu_ldw(FX_HIT_ON);
    gsu_moves(g.r[2]);
    {
        int on = !gsu.z;

        g.r[0] = gsu_sub(g.r[0], g.r[0], 0);      /* in the branch's delay slot */
        if (!on)
            { gsu_code(0x04bbc0, 9);
            gsu_stw(FX_HIT_SUM, g.r[0]); } else { gsu_code(0x04bbc0, 4); gsu_code(0x04bbc8, 1); }
    }
    gsu_code(0x04bbc9, 4);
    g.r[0] = gsu_add(gsu_ldw(FX_HIT_SUM), g.r[1], 0);
    gsu_code(0x04bbcd, 2);
    gsu_sbk(g.r[0]);
    g.r[1] = g.r[0];
    gsu_moves(g.r[2]);
    if (gsu.z)
        { gsu_code(0x04bbcf, 10); gsu_code(0x04bc6e, 3);
        return g; } else { gsu_code(0x04bbcf, 6); gsu_code(0x04bbd9, 1); }
    g.r[1] = gsu_div2(g.r[1]);
    g.r[2] = 0x0010;
    gsu_sub(g.r[1], g.r[2], 0);
    if (gsu.s)
        { gsu_code(0x04bbda, 15); gsu_code(0x04bc6e, 3);
        return g; } else { gsu_code(0x04bbda, 11); gsu_code(0x04bbe9, 1); }
    g.r[2] = 0x0060;
    gsu_sub(g.r[1], g.r[2], 0);
    if (!gsu.s)
        { g.r[1] = g.r[2]; gsu_code(0x04bbea, 11); } else { gsu_code(0x04bbea, 8); gsu_code(0x04bbf4, 1); }
    gsu_code(0x04bbf5, 5);
    g.r[0] = gsu_add(gsu_ldw(gsu_add(CAR_DAMAGE, g.r[8], 0)), g.r[1], 0);
    gsu_code(0x04bbfa, 2);
    gsu_sbk(g.r[0]);
    g.r[2] = 0x0018;
    gsu_sub(g.r[1], g.r[2], 0);
    if (gsu.s) {
        gsu_code(0x04bbfc, 8); gsu_code(0x04bc21, 1);
        car_or(&g, CAR_FLAGS40, 0x0080, GSU_COPY(0));
    } else {
        gsu_code(0x04bbfc, 9);
        car_or(&g, CAR_70, 0x0080, GSU_COPY(1));
        g.r[2] = 0x0038;
        gsu_sub(g.r[1], g.r[2], 0);
        if (gsu.s)
            { gsu_code(0x04bc0e, 8); gsu_code(0x04bc2c, 1);
            car_or(&g, CAR_FLAGS40, 0x0040, GSU_COPY(2)); }
        else
            { gsu_code(0x04bc0e, 9);
            car_or(&g, CAR_FLAGS40, 0x0020, GSU_COPY(3)); }
    }
    gsu_code(0x04bc36, 4);
    g.r[0] = gsu_ldw(gsu_add(CAR_FLAGS40, g.r[8], 0));
    g.r[2] = 0x0020;
    g.r[0] = gsu_and(g.r[0], g.r[2]);
    if (!gsu.z) {
        gsu_code(0x04bc3a, 11);
        g.r[0] = gsu_ldw(FX_PLAYER);
        gsu_sub(g.r[0], g.r[8], 0);
        if (gsu.z) {
            uint16_t ret = g.r[11];

            gsu_code(0x04bc45, 7);
            gsu_push(&g.r[10], ret);
            g.r[11] = 0xBC52;
            gsu_code(0x04bc4c, 6); gsu_code(0x04bf98, 1);
            g = FxSpreadCars(g);
            gsu_code(0x04bc52, 5);
            g.r[11] = gsu_pull(&g.r[10]);
        } else { gsu_code(0x04bc45, 4); gsu_code(0x04bc56, 1); }
    } else { gsu_code(0x04bc3a, 6); gsu_code(0x04bc56, 1); }
    gsu_code(0x04bc57, 5);
    g.r[0] = gsu_ldw(gsu_add(CAR_DAMAGE, g.r[8], 0));
    g.r[2] = 0x0200;
    gsu_sub(g.r[0], g.r[2], 0);
    if (!gsu.s) {
        gsu_code(0x04bc5c, 14);
        g.r[2] = gsu_ldw(gsu_add(CAR_FLAGS40, g.r[8], 0));
        g.r[0] = 0x0001;
        g.r[0] = gsu_or(g.r[0], g.r[2]);
        gsu_code(0x04bc6a, 5);
        gsu_sbk(g.r[0]);
    } else { gsu_code(0x04bc5c, 7); gsu_code(0x04bc6e, 1); }
    gsu_code(0x04bc6f, 2);
    return g;
cleared:
    g.r[1] = 0;
    g.r[0] = gsu_add(CAR_IMPACT, g.r[8], 0);
    gsu_code(0x04bc72, 7);
    gsu_stw(g.r[0], g.r[1]);
    gsu_code(0x04bc79, 2);
    return g;
}

void abi_FxCarHit(void)
{
    FxRegs g;

    memcpy(g.r, gsu.r, sizeof g.r);
    g = FxCarHit(g, GSU_COPY(0));
    memcpy(gsu.r, g.r, 15 * sizeof g.r[0]);
}
