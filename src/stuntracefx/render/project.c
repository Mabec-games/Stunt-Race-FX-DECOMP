/*
 * project.c: the Super FX projecting a point onto the screen: X and Y over
 * the depth (FxRatioClamp, clamped to $3FFF), moved to the screen centre,
 * with an outcode of which screen edges the point is beyond.
 */
#include "gsu.h"
#include "gsu_ops.h"

#define FX_CENTRE 0x0034u   /* the screen centre: X at $34, Y at $36 */
#define FX_EDGES 0x0038u    /* the edges: X at $38 and $3A, Y at $3C and $3E */

void abi_FxRatioClamp(void);

/* The LINK return address of each of the sixteen FxRatioClamp calls, by
   depth negative, X negative, Y negative, |X| below |Y|. */
static const uint16_t clamp_ret[16] = {
    0x94A2, 0x94B1, 0x94C8, 0x94DA, 0x94F9, 0x950B, 0x9525, 0x953A,
    0x9563, 0x9578, 0x9595, 0x95A7, 0x95C6, 0x95D8, 0x95F2, 0x9601,
};

/* NOT then INC. */
static uint16_t negate(uint16_t v, int copy)
{
    return gsu_inc(gsu_not(v));
}

/* One axis of the outcode: from R5, one more for a point past the low
   edge and two more past the high edge (SUB and BGE against each), the
   two bits swapped (XOR #3) when the point is behind (the bit in mask). */
/* gsuwalk: the code of each copy of outcode_axis, read from the ROM */
static const GsuCode outcode_axis_fetch[2][12] = {
    {{0x019611, 5}, {0x019616, 3}, {0x019616, 3}, {0x019621, 1}, {0x019619, 4}, {0x01961d, 3}, {0x01961d, 3}, {0x01962b, 1}, {0x019620, 2}, {0x019622, 6}, {0x01962b, 1}, {0x019622, 10}},
    {{0x01962f, 3}, {0x019632, 3}, {0x019632, 3}, {0x01963d, 1}, {0x019635, 4}, {0x019639, 3}, {0x019639, 3}, {0x019647, 1}, {0x01963c, 2}, {0x01963e, 6}, {0x019647, 1}, {0x01963e, 10}},
};

static void outcode_axis(uint16_t v, uint16_t low_edge, uint16_t high_edge, uint16_t mask, int copy)
{
    gsu_codes(outcode_axis_fetch[copy], 1);
    gsu.r[0] = gsu_sub(gsu_ldw(low_edge), v, 0);
    if (gsu.s == gsu.ov)
        { 
        gsu_codes(outcode_axis_fetch[copy] + 2, 2);
        goto one; } else { gsu_codes(outcode_axis_fetch[copy] + 1, 1); }
    gsu_codes(outcode_axis_fetch[copy] + 4, 1);
    gsu.r[0] = gsu_sub(gsu_ldw(high_edge), v, 0);
    if (gsu.s == gsu.ov)
        { 
        gsu_codes(outcode_axis_fetch[copy] + 6, 2);
        return; } else { gsu_codes(outcode_axis_fetch[copy] + 5, 1); }
    gsu.r[5] = gsu_inc(gsu.r[5]);
gsu_codes(outcode_axis_fetch[copy] + 8, 1);
one:
    ;
    gsu.r[5] = gsu_inc(gsu.r[5]);
    gsu.r[0] = gsu_and((uint16_t)mask, gsu.r[5]);
    if (!gsu.z)
        { gsu.r[5] = gsu_xor(gsu.r[5], 3); gsu_codes(outcode_axis_fetch[copy] + 11, 1); } else { gsu_codes(outcode_axis_fetch[copy] + 9, 2); }
}

/* One of FxProjectPoint's sixteen cases (the original has each in its own
   code): the pair swapped around FxRatioClamp when |X| is below |Y|, then
   the signs back, each reversed behind the view. */
/* gsuwalk: the code of each copy of project_case, read from the ROM */
static const GsuCode project_case_fetch[16][4] = {
    {{0x0195f7, 10}, {0x019658, 1}, {0, 0}, {0x019601, 1}},
    {{0x0195ee, 4}, {0x019658, 1}, {0x0195f2, 1}, {0, 0}},
    {{0x0195ce, 10}, {0x019658, 1}, {0, 0}, {0x0195d8, 1}},
    {{0x0195c2, 4}, {0x019658, 1}, {0x0195c6, 1}, {0, 0}},
    {{0x01959d, 10}, {0x019658, 1}, {0, 0}, {0x0195a7, 1}},
    {{0x019591, 4}, {0x019658, 1}, {0x019595, 1}, {0, 0}},
    {{0x01956e, 10}, {0x019658, 1}, {0, 0}, {0x019578, 1}},
    {{0x01955f, 4}, {0x019658, 1}, {0x019563, 1}, {0, 0}},
    {{0x019530, 10}, {0x019658, 1}, {0, 0}, {0x01953a, 1}},
    {{0x019521, 4}, {0x019658, 1}, {0x019525, 1}, {0, 0}},
    {{0x019501, 10}, {0x019658, 1}, {0, 0}, {0x01950b, 1}},
    {{0x0194f5, 4}, {0x019658, 1}, {0x0194f9, 1}, {0, 0}},
    {{0x0194d0, 10}, {0x019658, 1}, {0, 0}, {0x0194da, 1}},
    {{0x0194c4, 4}, {0x019658, 1}, {0x0194c8, 1}, {0, 0}},
    {{0x0194a7, 10}, {0x019658, 1}, {0, 0}, {0x0194b1, 1}},
    {{0x01949e, 4}, {0x019658, 1}, {0x0194a2, 1}, {0, 0}},
};

/* gsuwalk: the copy of negate each copy of project_case calls */
static const uint8_t negate_in_project_case[16] = {0, 0, 3, 4, 0, 0, 8, 9, 12, 13, 0, 0, 17, 18, 0, 0};

/* gsuwalk: the copy of negate each copy of project_case calls */
static const uint8_t negate_in_project_case_2[16] = {0, 0, 0, 0, 6, 7, 8, 9, 12, 13, 14, 15, 0, 0, 0, 0};

static void project_case(int behind, int xneg, int yneg, int swap, int copy)
{
    if (swap) {
        gsu.r[0] = gsu.r[1];
        gsu.r[1] = gsu.r[2];
        gsu.r[2] = gsu.r[0];
    }
    gsu.r[11] = clamp_ret[behind << 3 | xneg << 2 | yneg << 1 | swap];
    gsu_codes(project_case_fetch[copy], 2);
    abi_FxRatioClamp();
    if (swap) {
        gsu.r[0] = gsu.r[1];
        gsu.r[1] = gsu.r[2];
        gsu.r[2] = gsu.r[0];
        gsu_codes(project_case_fetch[copy] + 3, 1);
    } else { gsu_codes(project_case_fetch[copy] + 2, 1); }
    if (yneg != behind)
        gsu.r[2] = negate(gsu.r[2], GSU_COPY(negate_in_project_case[copy]));
    if (xneg != behind)
        gsu.r[1] = negate(gsu.r[1], GSU_COPY(negate_in_project_case_2[copy]));
}

/* Y's sign (MOVES, BMI; NOT, INC when negative), then |X| against |Y|
   (SUB, BCC): each way in its own code. */
/* gsuwalk: the code of each copy of project_y, read from the ROM */
static const GsuCode project_y_fetch[4][20] = {
    {{0x0195b5, 7}, {0x0195e5, 1}, {0x0195f3, 3}, {0x019607, 1}, {0x9608, 0}, {0x019602, 6}, {0x9608, 0}, {0, 0}, {0x0195e6, 7}, {0x0195f6, 1}, {0x0195e6, 8}, {0x0195c7, 6}, {0x019607, 1}, {0x9608, 0}, {0x0195d9, 12}, {0x019607, 1}, {0x9608, 0}, {0x0195b5, 12}, {0x0195cd, 1}, {0x0195b5, 13}},
    {{0x019555, 4}, {0x019588, 1}, {0x019596, 6}, {0x019607, 1}, {0x9608, 0}, {0x0195a8, 12}, {0x019607, 1}, {0x9608, 0}, {0x019589, 7}, {0x01959c, 1}, {0x019589, 8}, {0x019564, 9}, {0x019607, 1}, {0x9608, 0}, {0x019579, 15}, {0x019607, 1}, {0x9608, 0}, {0x019555, 9}, {0x01956d, 1}, {0x019555, 10}},
    {{0x0194e8, 7}, {0x019518, 1}, {0x019526, 9}, {0x019607, 1}, {0x9608, 0}, {0x01953b, 15}, {0x019607, 1}, {0x9608, 0}, {0x019519, 7}, {0x01952f, 1}, {0x019519, 8}, {0x0194fa, 6}, {0x019607, 1}, {0x9608, 0}, {0x01950c, 12}, {0x019607, 1}, {0x9608, 0}, {0x0194e8, 12}, {0x019500, 1}, {0x0194e8, 13}},
    {{0x019494, 4}, {0x0194bb, 1}, {0x0194c9, 6}, {0x019607, 1}, {0x9608, 0}, {0x0194db, 12}, {0x019607, 1}, {0x9608, 0}, {0x0194bc, 7}, {0x0194cf, 1}, {0x0194bc, 8}, {0x0194a3, 3}, {0x019607, 1}, {0x9608, 0}, {0x0194b2, 9}, {0x019607, 1}, {0x9608, 0}, {0x019494, 9}, {0x0194a6, 1}, {0x019494, 10}},
};

/* gsuwalk: the copy of negate each copy of project_y calls */
static const uint8_t negate_in_project_y[4] = {2, 5, 11, 16};

/* gsuwalk: the copy of project_case each copy of project_y calls */
static const uint8_t project_case_in_project_y[4] = {0, 4, 8, 12};

/* gsuwalk: the copy of project_case each copy of project_y calls */
static const uint8_t project_case_in_project_y_2[4] = {1, 5, 9, 13};

/* gsuwalk: the copy of project_case each copy of project_y calls */
static const uint8_t project_case_in_project_y_3[4] = {2, 6, 10, 14};

/* gsuwalk: the copy of project_case each copy of project_y calls */
static const uint8_t project_case_in_project_y_4[4] = {3, 7, 11, 15};

static void project_y(int behind, int xneg, int copy)
{
    gsu_moves(gsu.r[2]);
    if (gsu.s) {
        gsu_codes(project_y_fetch[copy], 2);
        gsu.r[2] = negate(gsu.r[2], GSU_COPY(negate_in_project_y[copy]));
        gsu.r[0] = gsu_sub(gsu.r[1], gsu.r[2], 0);
        if (!gsu.cy)
            { 
            gsu_codes(project_y_fetch[copy] + 8, 2);
            project_case(behind, xneg, 1, 1, GSU_COPY(project_case_in_project_y[copy])); gsu_codes(project_y_fetch[copy] + 5, 3); }
        else
            { 
            gsu_codes(project_y_fetch[copy] + 10, 1);
            project_case(behind, xneg, 1, 0, GSU_COPY(project_case_in_project_y_2[copy])); gsu_codes(project_y_fetch[copy] + 2, 3); }
    } else {
        gsu.r[0] = gsu_sub(gsu.r[1], gsu.r[2], 0);
        if (!gsu.cy)
            { 
            gsu_codes(project_y_fetch[copy] + 17, 2);
            project_case(behind, xneg, 0, 1, GSU_COPY(project_case_in_project_y_3[copy])); gsu_codes(project_y_fetch[copy] + 14, 3); }
        else
            { 
            gsu_codes(project_y_fetch[copy] + 19, 1);
            project_case(behind, xneg, 0, 0, GSU_COPY(project_case_in_project_y_4[copy])); gsu_codes(project_y_fetch[copy] + 11, 3); }
    }
}

/* X's sign (MOVES, BMI; NOT, INC when negative): each way in its own code. */
/* gsuwalk: the code of each copy of project_x, read from the ROM */
static const GsuCode project_x_fetch[2][5] = {
    {{0x019484, 3}, {0x01954a, 10}, {0x0195b4, 1}, {0x019484, 3}, {0x01954a, 11}},
    {{0x01948d, 6}, {0x0194e7, 1}, {0, 0}, {0x01948d, 7}, {0, 0}},
};

/* gsuwalk: the copy of negate each copy of project_x calls */
static const uint8_t negate_in_project_x[2] = {1, 10};

/* gsuwalk: the copy of project_y each copy of project_x calls */
static const uint8_t project_y_in_project_x[2] = {0, 2};

/* gsuwalk: the copy of project_y each copy of project_x calls */
static const uint8_t project_y_in_project_x_2[2] = {1, 3};

static void project_x(int behind, int copy)
{
    gsu_moves(gsu.r[1]);
    if (gsu.s) {
        gsu_codes(project_x_fetch[copy], 3);
        gsu.r[1] = negate(gsu.r[1], GSU_COPY(negate_in_project_x[copy]));
        project_y(behind, 1, GSU_COPY(project_y_in_project_x[copy]));
    } else {
        gsu_codes(project_x_fetch[copy] + 3, 2);
        project_y(behind, 0, GSU_COPY(project_y_in_project_x_2[copy]));
    }
}

/*
 * @routine 01:947A FxProjectPoint
 * WHAT IT DOES: Projects the point (R1, R2) at depth R3 to the screen,
 * moved to the centre at $0034, and returns its outcode: bits 0-1 past
 * the X edges, bits 2-3 past the Y edges, bit 4 behind the view.
 * HOW: Pushes R11 on the R10 stack. A negative depth is negated and sets
 * R5 = 4 (else R5 = 0; a zero depth counts as 1). FxRatioClamp divides the
 * larger of |R1|, |R2| first (the pair swapped around the call when |R1|
 * is the smaller), then the signs go back, each one reversed when the
 * depth was negative: sixteen copies of the code, one per case. Then
 * CACHE, the centre is added, and against the edges at $0038-$003E
 * (SUB, BGE) R5 counts 1 for past the low edge and 2 for past the high
 * one, its two bits swapped when behind, X then (after MULT #4) Y. R0 =
 * the outcode's complement in its low 5 bits << 8, OR the outcode.
 * Returns by pulling R11's value into R15.
 * Entry: R1, R2 = the point, R3 = its depth; R10 = the stack; R11 =
 * return address. Exit: R1, R2 = the screen position; R5 = the outcode;
 * R0 = ((R5 XOR $1F) << 8) | R5; R3 = |depth| (1 for 0); R4, R6, R11,
 * R12, R13 as FxRatioClamp leaves them; R10 as at entry; the last RAM
 * address R10; S and Z from R10; OV and CY from the last SUB R2.
 */
void FxProjectPoint(int copy)
{
    gsu_code(0x01947b, 2);
    gsu_push(&gsu.r[10], gsu.r[11]);
    gsu_moves(gsu.r[3]);
    if (gsu.s) {
        gsu_code(0x01947d, 7);
        gsu.r[3] = negate(gsu.r[3], GSU_COPY(0));
        gsu.r[5] = 4;
        project_x(1, GSU_COPY(0));
    } else {
        if (gsu.z)
            { gsu.r[3] = gsu_inc(gsu.r[3]); gsu_code(0x01947d, 6); gsu_code(0x019487, 6); } else { gsu_code(0x01947d, 6); gsu_code(0x019487, 4); gsu_code(0x01948c, 1); }
        gsu.r[5] = 0;
        project_x(0, GSU_COPY(1));
    }

    gsu_code(0x019608, 4);
    gsu.r[0] = gsu_ldw(FX_CENTRE);
    gsu.r[1] = gsu_add(gsu.r[1], gsu.r[0], 0);
    gsu_code(0x01960c, 5);
    gsu.r[0] = gsu_ldw((uint16_t)(FX_CENTRE + 2));
    gsu.r[2] = gsu_add(gsu.r[2], gsu.r[0], 0);
    outcode_axis(gsu.r[1], FX_EDGES, (uint16_t)(FX_EDGES + 2), 0x04, GSU_COPY(0));
    gsu.r[5] = gsu_mult(gsu.r[5], 4);                 /* MULT #4 */
    gsu_code(0x01962c, 3);
    outcode_axis(gsu.r[2], (uint16_t)(FX_EDGES + 4), (uint16_t)(FX_EDGES + 6), 0x10, GSU_COPY(1));
    gsu.r[0] = gsu_or(gsu_swap(gsu_xor(0x1F, gsu.r[5])), gsu.r[5]);
    gsu_code(0x019648, 10);
    gsu_pull(&gsu.r[10]);       /* the return address, into R15 */
}

void abi_FxProjectPoint(void)
{
    FxProjectPoint(GSU_COPY(0));
}

#define FX_SCALE_TABLE 0x896Cu  /* $14:896C: the scale for each even depth */

/*
 * @routine 01:B2A4 FxProjectScaled
 * WHAT IT DOES: Projects a point and a size at a depth: X (R1), Y (R2) and
 * the size (R5) times the scale for the depth (R3), X and Y moved to the
 * screen centre.
 * HOW: ROM bank $14. While the depth is above $4400 (SUB, BPL), ASR halves
 * X, Y, the size and the depth; while $100 - the depth is not negative,
 * ADD doubles them, so the depth ends from $101 to $4400. The scale is
 * the word at $14:896C + the depth AND NOT 1 (GETB, GETBH); FMULT by it
 * gives each, X plus $34 and Y plus $36.
 * Entry: R1 = X, R2 = Y, R3 = the depth (positive), R5 = the size; R11 =
 * return address. Exit: R1, R2 = the screen position; R5 = R0 = the size;
 * R3 = the depth as scaled, bit 0 clear; R6 = the scale; R14 = the
 * table's address + 1; the ROM bank $14; flags from the last FMULT.
 * R14 and the ROM buffer move as on the Super FX.
 */
void FxProjectScaled(int copy)
{
    gsu.r[0] = 0x0014;
    gsu_code(0x01b2a5, 4);
    gsu_romb(0x14);
    for (;;) {
        gsu.r[0] = gsu_sub(0x4400, gsu.r[3], 0);
        if (!gsu.s)
            { gsu_code(0x01b2a9, 6); gsu_code(0x01b2b9, 1); break; } else { gsu_code(0x01b2a9, 6); }
        gsu.r[1] = gsu_asr(gsu.r[1]);
        gsu.r[2] = gsu_asr(gsu.r[2]);
        gsu.r[5] = gsu_asr(gsu.r[5]);
        gsu.r[3] = gsu_asr(gsu.r[3]);       /* BRA's delay slot */
        gsu_code(0x01b2af, 10); gsu_code(0x01b2a8, 1);
    }
    for (;;) {
        gsu.r[0] = gsu_sub(0x0100, gsu.r[3], 0);
        if (gsu.s)
            { gsu_code(0x01b2ba, 6); gsu_code(0x01b2ca, 1); break; } else { gsu_code(0x01b2ba, 6); }
        gsu.r[5] = gsu_add(gsu.r[5], gsu.r[5], 0);
        gsu.r[1] = gsu_add(gsu.r[1], gsu.r[1], 0);
        gsu.r[2] = gsu_add(gsu.r[2], gsu.r[2], 0);
        gsu.r[3] = gsu_add(gsu.r[3], gsu.r[3], 0);  /* BRA's delay slot */
        gsu_code(0x01b2c0, 10); gsu_code(0x01b2b9, 1);
    }
    gsu.r[3] = gsu_bic(gsu.r[3], 0x0001);
    gsu.r[0] = FX_SCALE_TABLE;
    gsu_code(0x01b2cb, 8);
    gsu_r14(gsu_add(gsu.r[0], gsu.r[3], 0));
    gsu_code(0x01b2d3, 2);
    gsu.r[6] = (gsu_getb());
    gsu_code(0x01b2d5, 1);
    gsu_r14(gsu_inc(gsu.r[14]));
    gsu_code(0x01b2d6, 3);
    gsu.r[6] = gsu_getbh(gsu.r[6]);
    gsu.r[0] = gsu_fmult(gsu.r[1], gsu.r[6]);
    gsu_code(0x01b2d9, 5);
    gsu.r[1] = gsu_ldw(FX_CENTRE);
    gsu.r[1] = gsu_add(gsu.r[1], gsu.r[0], 0);
    gsu.r[0] = gsu_fmult(gsu.r[2], gsu.r[6]);
    gsu_code(0x01b2de, 7);
    gsu.r[2] = gsu_ldw(FX_CENTRE + 2);
    gsu.r[2] = gsu_add(gsu.r[2], gsu.r[0], 0);
    gsu.r[0] = gsu_fmult(gsu.r[5], gsu.r[6]);
    gsu.r[5] = gsu.r[0];
    gsu_code(0x01b2e5, 8);
}

void abi_FxProjectScaled(void)
{
    FxProjectScaled(GSU_COPY(0));
}

/* MULT #4: the low byte, signed, times 4; S and Z from the product. */
static uint16_t mult4(uint16_t v, int copy)
{
    (void)copy;
    return gsu_mult(v, 4);                   /* MULT #4 */
}

/*
 * @routine 01:8BFF FxProjectVertex
 * WHAT IT DOES: Projects the point (R1, R2) at depth R6 to the screen and
 * returns its outcode. A point farther than $100 is scaled by the table at
 * $14:896C (as FxProjectScaled) and moved to the centre at $0034; its
 * outcode has bit 2 for X below 0, bit 3 for X at or past $003A, bit 0 for
 * Y below 0, bit 1 for Y at or past $003E. A nearer point goes through
 * FxProjectPoint, which divides by the depth instead.
 * HOW: While $4400 - the depth is negative (SUB, BPL), ASR halves X, Y
 * and the depth (the last in the BRA's delay slot). For a far point the
 * scale is the word at $896C + the depth AND NOT 1 (GETB, GETBH), FMULT
 * by it gives X and Y, each plus the centre. R6 counts the outcode: X
 * first (MOVES, then SUB against the right edge; MULT #4 moves it up),
 * then Y. R0 = ((R6 XOR $1F) << 8) | R6. For a near point: R14 = R12,
 * R3 = the depth, R11 pushed on the R10 stack, FxProjectPoint, then R12 =
 * R14 and R3, R13, R9 reloaded from $0026-$002A; return by pulling R15.
 * Entry: R1 = X, R2 = Y, R6 = the depth; R12 = the next index byte's
 * address (near points only); R10 = stack; R11 = return address; the ROM
 * bank $14. Exit: (far) R1, R2 = the screen position; R6 = the outcode, R0
 * as above; R5 = the last edge read (when one was); R14 = the scale's
 * address + 1; S and Z from the OR; OV and CY from the last SUB or ADD;
 * the last RAM address the last word loaded. Exit (near): as FxProjectPoint
 * with R12 = R14, R3, R13, R9 from $0026, $0028, $002A; S, Z from R10;
 * the last RAM address R10. R14 and the ROM buffer move as on the Super
 * FX.
 */
void abi_FxProjectVertex(void)
{
    for (;;) {
        gsu.r[0] = gsu_sub(0x4400, gsu.r[6], 0);
        if (!gsu.s)
            { gsu_code(0x018c00, 6); gsu_code(0x018c0e, 1);
            break; } else { gsu_code(0x018c00, 6); }
        gsu.r[1] = gsu_asr(gsu.r[1]);
        gsu.r[2] = gsu_asr(gsu.r[2]);
        gsu.r[6] = gsu_asr(gsu.r[6]);       /* BRA's delay slot */
        
        gsu_code(0x018c06, 8); gsu_code(0x018bff, 1);
    }
    gsu.r[0] = gsu_sub(0x0100, gsu.r[6], 0);
    if (!gsu.s) {
        
        gsu_code(0x018c0f, 6); gsu_code(0x018c5c, 3);
        gsu_r14(gsu.r[12]);
        gsu.r[3] = gsu.r[6];
        gsu_code(0x018c5f, 4);
        gsu_push(&gsu.r[10], gsu.r[11]);
        gsu.r[11] = 0x8C69;
        
        gsu_code(0x018c63, 6); gsu_code(0x01947a, 1);
        abi_FxProjectPoint();
        gsu.r[12] = gsu.r[14];
        gsu_code(0x018c69, 6);
        gsu.r[3] = gsu_ldw(0x0026);
        gsu_code(0x018c6f, 3);
        gsu.r[13] = gsu_ldw(0x0028);
        gsu_code(0x018c72, 3);
        gsu.r[9] = gsu_ldw(0x002A);
        gsu_code(0x018c75, 4);
        gsu_pull(&gsu.r[10]);       /* the return address, into R15 */
        return;
    } else { gsu_code(0x018c0f, 6); }
    gsu.r[6] = gsu_bic(gsu.r[6], 0x0001);
    gsu.r[0] = FX_SCALE_TABLE;
    gsu_code(0x018c15, 9);
    gsu_r14(gsu_add(gsu.r[0], gsu.r[6], 0));
    gsu_code(0x018c1e, 2);
    gsu.r[6] = (gsu_getb());
    gsu_code(0x018c20, 1);
    gsu_r14(gsu_inc(gsu.r[14]));
    gsu_code(0x018c21, 3);
    gsu.r[6] = gsu_getbh(gsu.r[6]);
    gsu.r[0] = gsu_fmult(gsu.r[1], gsu.r[6]);
    gsu_code(0x018c24, 5);
    gsu.r[1] = gsu_ldw(FX_CENTRE);
    gsu.r[0] = gsu_add(gsu.r[0], gsu.r[1], 0);
    gsu.r[1] = gsu.r[0];
    gsu.r[0] = gsu_fmult(gsu.r[2], gsu.r[6]);
    gsu_code(0x018c29, 8);
    gsu.r[2] = gsu_ldw(FX_CENTRE + 2);
    gsu.r[0] = gsu_add(gsu.r[0], gsu.r[2], 0);
    gsu.r[2] = gsu.r[0];

    gsu.r[6] = 0;
    gsu.r[0] = gsu.r[1];
    gsu_moves(gsu.r[0]);
    if (!gsu.s) {
        gsu_code(0x018c31, 13);
        gsu.r[5] = gsu_ldw(FX_EDGES + 2);
        gsu.r[0] = gsu_sub(gsu.r[0], gsu.r[5], 0);
        if (gsu.s)
            { 
            gsu_code(0x018c3e, 3); gsu_code(0x018c46, 1);
            goto y_axis; } else { gsu_code(0x018c3e, 3); }
        gsu.r[6] = gsu_inc(gsu.r[6]);
        gsu_code(0x018c41, 2);
    } else { gsu_code(0x018c31, 9); gsu_code(0x018c42, 1); }
    gsu.r[6] = gsu_inc(gsu.r[6]);
    gsu.r[6] = mult4(gsu.r[6], GSU_COPY(0));
gsu_code(0x018c43, 4);
y_axis:
    ;
    gsu.r[0] = gsu.r[2];
    gsu_moves(gsu.r[0]);
    if (!gsu.s) {
        gsu_code(0x018c47, 8);
        gsu.r[5] = gsu_ldw(FX_EDGES + 6);
        gsu.r[0] = gsu_sub(gsu.r[0], gsu.r[5], 0);
        if (gsu.s)
            { 
            gsu_code(0x018c4f, 3); gsu_code(0x018c54, 1);
            goto code; } else { gsu_code(0x018c4f, 3); }
        gsu.r[6] = gsu_inc(gsu.r[6]);
        gsu_code(0x018c52, 2);
    } else { gsu_code(0x018c47, 4); gsu_code(0x018c53, 1); }
    gsu.r[6] = gsu_inc(gsu.r[6]);
gsu_code(0x018c54, 1);
code:
    ;
    gsu.r[0] = gsu_or(gsu_swap(gsu_xor(0x1F, gsu.r[6])), gsu.r[6]);
    gsu_code(0x018c55, 7);
}
