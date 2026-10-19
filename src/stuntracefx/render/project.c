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

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode outcode_axis_code[10][8] = {
    {{0x019616, 3}, {0, 0}, {0x019616, 7}, {0x01961D, 3}, {0x01961D, 4}, {0, 0}, {0x019621, 7}, {0x019628, 3}},
    {{0x019632, 3}, {0, 0}, {0x019632, 7}, {0x019639, 3}, {0x019639, 4}, {0, 0}, {0x01963D, 7}, {0x019644, 3}},
    {{0x019616, 3}, {0, 0}, {0x019616, 7}, {0x01961D, 3}, {0x01961D, 4}, {0, 0}, {0x019621, 7}, {0x019628, 3}},
    {{0x019632, 3}, {0, 0}, {0x019632, 7}, {0x019639, 3}, {0x019639, 4}, {0, 0}, {0x01963D, 7}, {0x019628, 3}},
    {{0x019632, 3}, {0, 0}, {0x019632, 7}, {0x019639, 3}, {0x019639, 4}, {0x019644, 3}, {0x01963D, 7}, {0, 0}},
    {{0x019616, 3}, {0, 0}, {0x019616, 7}, {0x01961D, 3}, {0x01961D, 4}, {0, 0}, {0, 0}, {0x019628, 3}},
    {{0x019632, 3}, {0x019644, 3}, {0x019632, 7}, {0x019639, 3}, {0x019639, 4}, {0, 0}, {0x01963D, 7}, {0, 0}},
    {{0x019616, 3}, {0, 0}, {0x019632, 7}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x019628, 3}},
    {{0x019616, 3}, {0, 0}, {0x019616, 7}, {0, 0}, {0x01961D, 4}, {0, 0}, {0x019621, 7}, {0x019628, 3}},
    {{0x019616, 3}, {0, 0}, {0x019632, 7}, {0, 0}, {0x01963C, 8}, {0, 0}, {0, 0}, {0x019628, 3}},
};

/* One axis of the outcode: from R5, one more for a point past the low
   edge and two more past the high edge (SUB and BGE against each), the
   two bits swapped (XOR #3) when the point is behind (the bit in mask). */
static void outcode_axis(uint16_t v, uint16_t low_edge, uint16_t high_edge, uint16_t mask, int copy)
{
    gsu.r[0] = gsu_sub(gsu_ldw(low_edge), v, 0);
    if (gsu.s == gsu.ov)
        { gsu_codes(outcode_axis_code[copy], 2); goto one; } else { gsu_codes(outcode_axis_code[copy] + 2, 1); }
    gsu.r[0] = gsu_sub(gsu_ldw(high_edge), v, 0);
    if (gsu.s == gsu.ov)
        { gsu_codes(outcode_axis_code[copy] + 3, 1); return; } else { gsu_codes(outcode_axis_code[copy] + 4, 2); }
    gsu.r[5] = gsu_inc(gsu.r[5]);
one:
    gsu_codes(outcode_axis_code[copy] + 6, 1);
    gsu.r[5] = gsu_inc(gsu.r[5]);
    gsu.r[0] = gsu_and((uint16_t)mask, gsu.r[5]);
    if (!gsu.z)
        { gsu_codes(outcode_axis_code[copy] + 7, 1); gsu.r[5] = gsu_xor(gsu.r[5], 3); }
}

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode project_case_code[54][2] = {
    {{0x0194B1, 10}, {0x0195C6, 7}},
    {{0x019578, 16}, {0, 0}},
    {{0x0194B1, 10}, {0x0194F9, 7}},
    {{0x0194B1, 10}, {0x0194A2, 4}},
    {{0x0194B1, 10}, {0x019563, 10}},
    {{0x0194B1, 10}, {0x0194C8, 7}},
    {{0x0195D8, 13}, {0, 0}},
    {{0x0194B1, 10}, {0x019525, 10}},
    {{0x01950B, 13}, {0, 0}},
    {{0x0194B1, 10}, {0x0194A2, 4}},
    {{0x0194B1, 10}, {0x019525, 10}},
    {{0x0194B1, 10}, {0x0195F2, 4}},
    {{0x0195A7, 13}, {0, 0}},
    {{0x0194B1, 10}, {0x019595, 7}},
    {{0x0194B1, 10}, {0x0194F9, 7}},
    {{0x01953A, 16}, {0, 0}},
    {{0x01953A, 16}, {0, 0}},
    {{0x019601, 6}, {0, 0}},
    {{0x01950B, 13}, {0, 0}},
    {{0x0194B1, 10}, {0, 0}},
    {{0x0194DA, 13}, {0, 0}},
    {{0x0194DA, 13}, {0, 0}},
    {{0x0194B1, 10}, {0x0194A2, 4}},
    {{0x0194B1, 10}, {0x0194F9, 7}},
    {{0x0194B1, 10}, {0, 0}},
    {{0x0194B1, 10}, {0, 0}},
    {{0x0194B1, 10}, {0, 0}},
    {{0x01950B, 13}, {0, 0}},
    {{0x0194B1, 10}, {0, 0}},
    {{0x0194B1, 10}, {0, 0}},
    {{0x0194B1, 10}, {0x019563, 10}},
    {{0x0194B1, 10}, {0, 0}},
    {{0x019578, 16}, {0, 0}},
    {{0x0194B1, 10}, {0x0195C6, 7}},
    {{0x0195D8, 13}, {0, 0}},
    {{0x0194B1, 10}, {0, 0}},
    {{0x01950B, 13}, {0, 0}},
    {{0x0194B1, 10}, {0x0194F9, 7}},
    {{0x019578, 16}, {0, 0}},
    {{0x0194B1, 10}, {0, 0}},
    {{0x0194B1, 10}, {0, 0}},
    {{0x0194B1, 10}, {0, 0}},
    {{0x0194B1, 10}, {0, 0}},
    {{0x0194B1, 10}, {0, 0}},
    {{0x0194B1, 10}, {0x0194A2, 4}},
    {{0x0194B1, 10}, {0x0194A2, 4}},
    {{0x0194B1, 10}, {0x019563, 10}},
    {{0x0194B1, 10}, {0, 0}},
    {{0x0194DA, 13}, {0, 0}},
    {{0x0194B1, 10}, {0x019525, 10}},
    {{0x01953A, 16}, {0, 0}},
    {{0x0194B1, 10}, {0x0194C8, 7}},
    {{0x0194B1, 10}, {0, 0}},
    {{0x01950B, 13}, {0, 0}},
};

/* One of FxProjectPoint's sixteen cases (the original has each in its own
   code): the pair swapped around FxRatioClamp when |X| is below |Y|, then
   the signs back, each reversed behind the view. */
static void project_case(int behind, int xneg, int yneg, int swap, int copy)
{
    if (swap) {
        gsu.r[0] = gsu.r[1];
        gsu.r[1] = gsu.r[2];
        gsu.r[2] = gsu.r[0];
    }
    gsu.r[11] = clamp_ret[behind << 3 | xneg << 2 | yneg << 1 | swap];
    abi_FxRatioClamp();
    if (swap) {
        gsu_codes(project_case_code[copy], 1);
        gsu.r[0] = gsu.r[1];
        gsu.r[1] = gsu.r[2];
        gsu.r[2] = gsu.r[0];
    } else { gsu_codes(project_case_code[copy] + 1, 1); }
    if (yneg != behind)
        gsu.r[2] = negate(gsu.r[2], GSU_COPY(0));
    if (xneg != behind)
        gsu.r[1] = negate(gsu.r[1], GSU_COPY(0));
}

/* cyccopy: the copy each of the caller's copies calls */
static const uint8_t project_case_from_project_y[25] = {17, 12, 15, 21, 16, 20, 24, 25, 29, 32, 31, 34, 35, 36, 39, 40, 41, 42, 48, 26, 53, 26, 46, 50, 52};

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode project_y_code[25][8] = {
    {{0x0195E5, 8}, {0x0195F6, 11}, {0x0195ED, 5}, {0, 0}, {0x0195BC, 5}, {0x0195CD, 11}, {0x0195C1, 5}, {0, 0}},
    {{0x019588, 8}, {0x01959C, 11}, {0x019590, 5}, {0, 0}, {0x019559, 5}, {0x01956D, 11}, {0x01955E, 5}, {0, 0}},
    {{0x019518, 8}, {0x01952F, 11}, {0x019520, 5}, {0, 0}, {0x0194EF, 5}, {0x019500, 11}, {0x0194F4, 5}, {0, 0}},
    {{0x0194BB, 8}, {0x0194CF, 11}, {0x0194C3, 5}, {0, 0}, {0x019498, 5}, {0x0194A6, 11}, {0x01949D, 5}, {0, 0}},
    {{0x019518, 8}, {0x01952F, 11}, {0x019520, 5}, {0, 0}, {0x0194EF, 5}, {0x019500, 11}, {0x0194F4, 5}, {0, 0}},
    {{0x0194BB, 8}, {0x0194CF, 11}, {0x0194C3, 5}, {0, 0}, {0x019498, 5}, {0x0194A6, 11}, {0x01949D, 5}, {0, 0}},
    {{0x0194BB, 13}, {0x01952F, 11}, {0, 0}, {0x0194C8, 7}, {0x019498, 5}, {0x0194A6, 11}, {0x01949D, 5}, {0, 0}},
    {{0x019518, 13}, {0x01952F, 11}, {0, 0}, {0x019525, 10}, {0x0194EF, 5}, {0x019500, 11}, {0x0194F4, 5}, {0, 0}},
    {{0x0194BB, 13}, {0x01952F, 11}, {0, 0}, {0x0194C8, 7}, {0x019498, 10}, {0x0194A6, 11}, {0, 0}, {0x0194A2, 4}},
    {{0x019588, 8}, {0x01959C, 11}, {0x019590, 5}, {0, 0}, {0x019559, 5}, {0x01956D, 11}, {0x01955E, 5}, {0, 0}},
    {{0x0194BB, 8}, {0x0194CF, 11}, {0x0194C3, 5}, {0, 0}, {0x019498, 10}, {0x0194A6, 11}, {0, 0}, {0x0194A2, 4}},
    {{0x0195E5, 13}, {0x01952F, 11}, {0, 0}, {0x0195F2, 4}, {0x0195BC, 5}, {0x0195CD, 11}, {0x0195C1, 5}, {0, 0}},
    {{0x019518, 13}, {0x01952F, 11}, {0, 0}, {0x019525, 10}, {0x0194EF, 10}, {0x0194A6, 11}, {0, 0}, {0x0194F9, 7}},
    {{0x019518, 8}, {0x01952F, 11}, {0x019520, 5}, {0, 0}, {0x0194EF, 5}, {0x019500, 11}, {0x0194F4, 5}, {0, 0}},
    {{0x0195E5, 13}, {0x01952F, 11}, {0, 0}, {0x0195F2, 4}, {0x0195BC, 5}, {0x0195CD, 11}, {0x0195C1, 5}, {0, 0}},
    {{0x019588, 13}, {0x01952F, 11}, {0, 0}, {0x019595, 7}, {0x019559, 5}, {0x01956D, 11}, {0x01955E, 5}, {0, 0}},
    {{0x019518, 8}, {0x01952F, 11}, {0, 0}, {0, 0}, {0, 0}, {0x0194A6, 11}, {0, 0}, {0x0194F9, 7}},
    {{0x019518, 8}, {0x01952F, 11}, {0, 0}, {0, 0}, {0, 0}, {0x0194A6, 11}, {0, 0}, {0x0194A2, 4}},
    {{0x0194BB, 8}, {0x0194CF, 11}, {0x0194C3, 5}, {0, 0}, {0x019498, 5}, {0x0194A6, 11}, {0x01949D, 5}, {0, 0}},
    {{0x019518, 8}, {0x01952F, 11}, {0, 0}, {0, 0}, {0, 0}, {0x0194A6, 11}, {0x01949D, 5}, {0, 0}},
    {{0x019518, 13}, {0x01952F, 11}, {0, 0}, {0x019525, 10}, {0x0194EF, 5}, {0x019500, 11}, {0x0194F4, 5}, {0, 0}},
    {{0x019518, 8}, {0x01952F, 11}, {0, 0}, {0, 0}, {0, 0}, {0x0194A6, 11}, {0x01949D, 5}, {0, 0}},
    {{0x019588, 13}, {0x01952F, 11}, {0, 0}, {0x019595, 7}, {0x019559, 5}, {0x01956D, 11}, {0x01955E, 5}, {0, 0}},
    {{0x019518, 8}, {0x01952F, 11}, {0x019520, 5}, {0, 0}, {0x0194EF, 10}, {0x0194A6, 11}, {0, 0}, {0x0194F9, 7}},
    {{0x019518, 8}, {0x01952F, 11}, {0, 0}, {0, 0}, {0, 0}, {0x0194A6, 11}, {0, 0}, {0, 0}},
};

/* Y's sign (MOVES, BMI; NOT, INC when negative), then |X| against |Y|
   (SUB, BCC): each way in its own code. */
static void project_y(int behind, int xneg, int copy)
{
    gsu_moves(gsu.r[2]);
    if (gsu.s) {
        gsu_codes(project_y_code[copy], 1);
        gsu.r[2] = negate(gsu.r[2], GSU_COPY(0));
        gsu.r[0] = gsu_sub(gsu.r[1], gsu.r[2], 0);
        if (!gsu.cy)
            { gsu_codes(project_y_code[copy] + 1, 1); project_case(behind, xneg, 1, 1, GSU_COPY(project_case_from_project_y[copy])); }
        else
            { gsu_codes(project_y_code[copy] + 2, 1); project_case(behind, xneg, 1, 0, GSU_COPY(project_case_from_project_y[copy])); }
        gsu_codes(project_y_code[copy] + 3, 1);
    } else {
        gsu_codes(project_y_code[copy] + 4, 1);
        gsu.r[0] = gsu_sub(gsu.r[1], gsu.r[2], 0);
        if (!gsu.cy)
            { gsu_codes(project_y_code[copy] + 5, 1); project_case(behind, xneg, 0, 1, GSU_COPY(project_case_from_project_y[copy])); }
        else
            { gsu_codes(project_y_code[copy] + 6, 1); project_case(behind, xneg, 0, 0, GSU_COPY(project_case_from_project_y[copy])); }
        gsu_codes(project_y_code[copy] + 7, 1);
    }
}

/* cyccopy: the copy each of the caller's copies calls */
static const uint8_t project_y_from_project_x[14] = {1, 3, 5, 6, 8, 9, 10, 15, 17, 18, 19, 21, 22, 24};

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode project_x_code[14][2] = {
    {{0x0195B4, 8}, {0x019554, 5}},
    {{0x0194E7, 8}, {0x019493, 5}},
    {{0x0194E7, 8}, {0x019493, 5}},
    {{0x0194E7, 8}, {0x019493, 5}},
    {{0x0194E7, 8}, {0x019493, 5}},
    {{0x0195B4, 8}, {0x019554, 5}},
    {{0x0194E7, 8}, {0x019493, 5}},
    {{0x0195B4, 8}, {0x019554, 5}},
    {{0x0194E7, 18}, {0x019493, 15}},
    {{0x0194E7, 8}, {0x019493, 5}},
    {{0x0194E7, 8}, {0, 0}},
    {{0x0194E7, 8}, {0x019493, 10}},
    {{0x0195B4, 8}, {0x019554, 5}},
    {{0x0194E7, 8}, {0, 0}},
};

/* X's sign (MOVES, BMI; NOT, INC when negative): each way in its own code. */
static void project_x(int behind, int copy)
{
    gsu_moves(gsu.r[1]);
    if (gsu.s) {
        gsu_codes(project_x_code[copy], 1);
        gsu.r[1] = negate(gsu.r[1], GSU_COPY(0));
        project_y(behind, 1, GSU_COPY(project_y_from_project_x[copy]));
    } else {
        gsu_codes(project_x_code[copy] + 1, 1);
        project_y(behind, 0, GSU_COPY(project_y_from_project_x[copy]));
    }
}

/* cyccopy: the copy each of the caller's copies calls */
static const uint8_t project_x_from_FxProjectPoint[12] = {2, 1, 3, 4, 6, 7, 8, 9, 10, 11, 12, 13};

/* cyccopy: the copy each of the caller's copies calls */
static const uint8_t outcode_axis_from_FxProjectPoint[12] = {1, 3, 4, 1, 3, 6, 7, 3, 9, 7, 1, 7};

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode FxProjectPoint_code[12][23] = {
    {{0x018C63, 6}, {0x01947A, 3}, {0x01947D, 10}, {0x01954A, 10}, {0x019658, 3}, {0x01947D, 6}, {0x019487, 4}, {0x01948C, 7}, {0x019658, 3}, {0, 0}, {0, 0}, {0x01948B, 1}, {0x019607, 1}, {0x9608, 0}, {0x019608, 4}, {0, 0}, {0x01960C, 5}, {0x019611, 5}, {0x01962B, 7}, {0, 0}, {0x019647, 11}, {0, 0}, {0x018C69, 6}},
    {{0x018C63, 6}, {0x01947A, 3}, {0x01947D, 10}, {0x01954A, 10}, {0x019658, 3}, {0x01947D, 6}, {0x019487, 4}, {0x01948C, 7}, {0x019658, 3}, {0, 0}, {0, 0}, {0x01948B, 1}, {0x019607, 1}, {0x9608, 0}, {0x019608, 4}, {0, 0}, {0x01960C, 5}, {0x019611, 5}, {0x01962B, 7}, {0, 0}, {0x019647, 11}, {0, 0}, {0x018C69, 6}},
    {{0x018C63, 6}, {0x01947A, 3}, {0x01947D, 10}, {0x01954A, 10}, {0x019658, 3}, {0x01947D, 6}, {0x019487, 4}, {0x01948C, 7}, {0x019658, 3}, {0, 0}, {0, 0}, {0x01948B, 1}, {0x019607, 1}, {0x9608, 0}, {0x019608, 4}, {0, 0}, {0x01960C, 5}, {0x019611, 5}, {0x01962B, 7}, {0, 0}, {0x019647, 11}, {0, 0}, {0x018C69, 6}},
    {{0x018C63, 6}, {0x01947A, 3}, {0x01947D, 10}, {0x01954A, 10}, {0x019658, 3}, {0x01947D, 6}, {0x019487, 4}, {0x01948C, 7}, {0x019658, 3}, {0, 0}, {0, 0}, {0x01948B, 1}, {0x019607, 1}, {0x9608, 0}, {0x019608, 4}, {0, 0}, {0x01960C, 5}, {0x019611, 5}, {0x01962B, 7}, {0, 0}, {0x019647, 11}, {0, 0}, {0x018C69, 6}},
    {{0x018C63, 6}, {0x01947A, 3}, {0x01947D, 10}, {0x01954A, 10}, {0x019658, 3}, {0x01947D, 6}, {0x019487, 4}, {0x01948C, 7}, {0x019658, 3}, {0, 0}, {0, 0}, {0x01948B, 1}, {0x019607, 1}, {0x9608, 0}, {0x019608, 4}, {0, 0}, {0x01960C, 5}, {0x019611, 5}, {0x019621, 7}, {0x01962B, 7}, {0x019647, 11}, {0, 0}, {0x018C69, 6}},
    {{0x018C63, 6}, {0x01947A, 3}, {0x01947D, 10}, {0x01954A, 10}, {0x019658, 3}, {0x01947D, 6}, {0x019487, 4}, {0x01948C, 7}, {0x0194E7, 13}, {0x019500, 11}, {0x019658, 3}, {0x01948B, 1}, {0x019607, 1}, {0x9608, 0}, {0x019608, 4}, {0, 0}, {0x01960C, 5}, {0x019611, 5}, {0x01962B, 7}, {0, 0}, {0x019647, 11}, {0, 0}, {0x018C69, 6}},
    {{0x018C63, 6}, {0x01947A, 3}, {0x01947D, 10}, {0x01954A, 10}, {0x019658, 3}, {0x01947D, 6}, {0x019487, 12}, {0x019658, 3}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x019607, 1}, {0x9608, 0}, {0x019608, 4}, {0, 0}, {0x01960C, 5}, {0x019611, 5}, {0x019621, 7}, {0x01962B, 7}, {0x019639, 11}, {0x019647, 11}, {0x018C69, 6}},
    {{0, 0}, {0, 0}, {0x01947D, 10}, {0x01954A, 10}, {0x019658, 3}, {0x01947D, 6}, {0x019487, 4}, {0x01948C, 7}, {0x019658, 3}, {0, 0}, {0, 0}, {0x01948B, 1}, {0x019607, 1}, {0x9608, 0}, {0x019608, 4}, {0, 0}, {0x01960C, 5}, {0x019611, 5}, {0x01962B, 7}, {0, 0}, {0x019647, 11}, {0, 0}, {0, 0}},
    {{0, 0}, {0, 0}, {0x01947D, 10}, {0x01954A, 10}, {0x019658, 3}, {0x01947D, 6}, {0x019487, 4}, {0x01948C, 17}, {0x019658, 3}, {0, 0}, {0, 0}, {0x01948B, 1}, {0x019607, 1}, {0x9608, 0}, {0x019608, 4}, {0, 0}, {0x01960C, 5}, {0x019611, 5}, {0x01961D, 3}, {0x01962B, 7}, {0x019639, 3}, {0x019647, 11}, {0, 0}},
    {{0x018C63, 6}, {0x01947A, 3}, {0x01947D, 10}, {0x01954A, 10}, {0x019658, 3}, {0x01947D, 6}, {0x019487, 4}, {0x01948C, 7}, {0x019658, 3}, {0, 0}, {0, 0}, {0x01948B, 1}, {0x019607, 1}, {0x9608, 0}, {0x019608, 4}, {0, 0}, {0x01960C, 5}, {0x019611, 5}, {0x01962B, 7}, {0, 0}, {0x019639, 3}, {0x019647, 11}, {0x018C69, 6}},
    {{0x018C63, 6}, {0x01947A, 3}, {0x01947D, 10}, {0x01954A, 10}, {0x019658, 3}, {0x01947D, 6}, {0x019487, 4}, {0x01948C, 7}, {0x019658, 3}, {0, 0}, {0, 0}, {0x01948B, 1}, {0x019607, 1}, {0x9608, 0}, {0x019608, 4}, {0, 0}, {0x01960C, 5}, {0x019611, 5}, {0x01962B, 7}, {0, 0}, {0x019647, 11}, {0, 0}, {0x018C69, 6}},
    {{0, 0}, {0, 0}, {0x01947D, 10}, {0x01954A, 10}, {0x019658, 3}, {0x01947D, 6}, {0x019487, 4}, {0x01948C, 22}, {0x019658, 3}, {0, 0}, {0, 0}, {0x01948B, 1}, {0x0194A2, 4}, {0x019607, 1}, {0x9608, 0}, {0x019608, 4}, {0x01960C, 5}, {0x019611, 5}, {0x01961D, 3}, {0x01962B, 7}, {0x019639, 3}, {0x019647, 11}, {0, 0}},
};

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
    gsu_codes(FxProjectPoint_code[copy], 2);
    gsu_push(&gsu.r[10], gsu.r[11]);
    gsu_moves(gsu.r[3]);
    if (gsu.s) {
        gsu_codes(FxProjectPoint_code[copy] + 2, 3);
        gsu.r[3] = negate(gsu.r[3], GSU_COPY(0));
        gsu.r[5] = 4;
        project_x(1, GSU_COPY(project_x_from_FxProjectPoint[copy]));
    } else {
        gsu_codes(FxProjectPoint_code[copy] + 5, 6);
        if (gsu.z)
            { gsu_codes(FxProjectPoint_code[copy] + 11, 1); gsu.r[3] = gsu_inc(gsu.r[3]); }
        gsu.r[5] = 0;
        project_x(0, GSU_COPY(project_x_from_FxProjectPoint[copy]));
    }

    gsu_codes(FxProjectPoint_code[copy] + 12, 4);
    gsu.r[0] = gsu_ldw(FX_CENTRE);
    gsu.r[1] = gsu_add(gsu.r[1], gsu.r[0], 0);
    gsu_codes(FxProjectPoint_code[copy] + 16, 1);
    gsu.r[0] = gsu_ldw((uint16_t)(FX_CENTRE + 2));
    gsu.r[2] = gsu_add(gsu.r[2], gsu.r[0], 0);
    gsu_codes(FxProjectPoint_code[copy] + 17, 1);
    outcode_axis(gsu.r[1], FX_EDGES, (uint16_t)(FX_EDGES + 2), 0x04, GSU_COPY(outcode_axis_from_FxProjectPoint[copy]));
    gsu.r[5] = (uint16_t)((int8_t)gsu.r[5] * 4);      /* MULT #4 */
    gsu_sz(gsu.r[5]);
    gsu_codes(FxProjectPoint_code[copy] + 18, 2);
    outcode_axis(gsu.r[2], (uint16_t)(FX_EDGES + 4), (uint16_t)(FX_EDGES + 6), 0x10, GSU_COPY(outcode_axis_from_FxProjectPoint[copy]));
    gsu.r[0] = gsu_or(gsu_swap(gsu_xor(0x1F, gsu.r[5])), gsu.r[5]);
    gsu_codes(FxProjectPoint_code[copy] + 20, 2);
    gsu_pull(&gsu.r[10]);       /* the return address, into R15 */
    gsu_codes(FxProjectPoint_code[copy] + 22, 1);
}

void abi_FxProjectPoint(void)
{
    FxProjectPoint(GSU_COPY(0));
}

#define FX_SCALE_TABLE 0x896Cu  /* $14:896C: the scale for each even depth */

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode FxProjectScaled_code[2][9] = {
    {{0x01B2A8, 7}, {0x01B2AF, 10}, {0x01B2B9, 7}, {0x01B2C0, 10}, {0x01B2D3, 2}, {0x01B2D5, 1}, {0x01B2D6, 3}, {0x01B2D9, 5}, {0x01B2DE, 7}},
    {{0, 0}, {0x01B2AF, 10}, {0x01B2B9, 7}, {0x01B2C0, 10}, {0x01B2D3, 2}, {0x01B2D5, 1}, {0x01B2D6, 3}, {0x01B2D9, 5}, {0x01B2DE, 7}},
};

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
    gsu_romb(0x14);
    for (;;) {
        gsu_codes(FxProjectScaled_code[copy], 1);
        gsu.r[0] = gsu_sub(0x4400, gsu.r[3], 0);
        if (!gsu.s)
            break; else { gsu_codes(FxProjectScaled_code[copy] + 1, 1); }
        gsu.r[1] = gsu_asr(gsu.r[1]);
        gsu.r[2] = gsu_asr(gsu.r[2]);
        gsu.r[5] = gsu_asr(gsu.r[5]);
        gsu.r[3] = gsu_asr(gsu.r[3]);       /* BRA's delay slot */
    }
    for (;;) {
        gsu_codes(FxProjectScaled_code[copy] + 2, 1);
        gsu.r[0] = gsu_sub(0x0100, gsu.r[3], 0);
        if (gsu.s)
            break; else { gsu_codes(FxProjectScaled_code[copy] + 3, 1); }
        gsu.r[5] = gsu_add(gsu.r[5], gsu.r[5], 0);
        gsu.r[1] = gsu_add(gsu.r[1], gsu.r[1], 0);
        gsu.r[2] = gsu_add(gsu.r[2], gsu.r[2], 0);
        gsu.r[3] = gsu_add(gsu.r[3], gsu.r[3], 0);  /* BRA's delay slot */
    }
    gsu.r[3] = gsu_bic(gsu.r[3], 0x0001);
    gsu.r[0] = FX_SCALE_TABLE;
    gsu_r14(gsu_add(gsu.r[0], gsu.r[3], 0));
    gsu.r[6] = (gsu_codes(FxProjectScaled_code[copy] + 4, 1), gsu_getb());
    gsu_codes(FxProjectScaled_code[copy] + 5, 1);
    gsu_r14(gsu_inc(gsu.r[14]));
    gsu_codes(FxProjectScaled_code[copy] + 6, 1);
    gsu.r[6] = gsu_getbh(gsu.r[6]);
    gsu.r[0] = gsu_fmult(gsu.r[1], gsu.r[6]);
    gsu_codes(FxProjectScaled_code[copy] + 7, 1);
    gsu.r[1] = gsu_ldw(FX_CENTRE);
    gsu.r[1] = gsu_add(gsu.r[1], gsu.r[0], 0);
    gsu.r[0] = gsu_fmult(gsu.r[2], gsu.r[6]);
    gsu_codes(FxProjectScaled_code[copy] + 8, 1);
    gsu.r[2] = gsu_ldw(FX_CENTRE + 2);
    gsu.r[2] = gsu_add(gsu.r[2], gsu.r[0], 0);
    gsu.r[0] = gsu_fmult(gsu.r[5], gsu.r[6]);
    gsu.r[5] = gsu.r[0];
}

void abi_FxProjectScaled(void)
{
    FxProjectScaled(GSU_COPY(0));
}

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode mult4_code[2][2] = {
    {{0x018C42, 4}, {0, 0}},
    {{0x018C31, 9}, {0x018C42, 4}},
};

/* MULT #4: the low byte, signed, times 4; S and Z from the product. */
static uint16_t mult4(uint16_t v, int copy)
{
    gsu_codes(mult4_code[copy], 2);
    uint16_t r = (uint16_t)((int8_t)v * 4);

    gsu_sz(r);
    return r;
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
        gsu_code(0x018BFF, 7);
        gsu.r[0] = gsu_sub(0x4400, gsu.r[6], 0);
        if (!gsu.s)
            break; else { gsu_code(0x018C06, 8); }
        gsu.r[1] = gsu_asr(gsu.r[1]);
        gsu.r[2] = gsu_asr(gsu.r[2]);
        gsu.r[6] = gsu_asr(gsu.r[6]);       /* BRA's delay slot */
    }
    gsu.r[0] = gsu_sub(0x0100, gsu.r[6], 0);
    if (!gsu.s) {
        gsu_code(0x018C5C, 3);
        gsu_r14(gsu.r[12]);
        gsu.r[3] = gsu.r[6];
        gsu_code(0x018C5F, 4);
        gsu_push(&gsu.r[10], gsu.r[11]);
        gsu.r[11] = 0x8C69;
        abi_FxProjectPoint();
        gsu.r[12] = gsu.r[14];
        gsu.r[3] = gsu_ldw(0x0026);
        gsu_code(0x018C6F, 3);
        gsu.r[13] = gsu_ldw(0x0028);
        gsu_code(0x018C72, 3);
        gsu.r[9] = gsu_ldw(0x002A);
        gsu_code(0x018C75, 4);
        gsu_pull(&gsu.r[10]);       /* the return address, into R15 */
        return;
    } else { gsu_code(0x018C15, 9); }
    gsu.r[6] = gsu_bic(gsu.r[6], 0x0001);
    gsu.r[0] = FX_SCALE_TABLE;
    gsu_r14(gsu_add(gsu.r[0], gsu.r[6], 0));
    gsu.r[6] = (gsu_code(0x018C1E, 2), gsu_getb());
    gsu_code(0x018C20, 1);
    gsu_r14(gsu_inc(gsu.r[14]));
    gsu_code(0x018C21, 3);
    gsu.r[6] = gsu_getbh(gsu.r[6]);
    gsu.r[0] = gsu_fmult(gsu.r[1], gsu.r[6]);
    gsu_code(0x018C24, 5);
    gsu.r[1] = gsu_ldw(FX_CENTRE);
    gsu.r[0] = gsu_add(gsu.r[0], gsu.r[1], 0);
    gsu.r[1] = gsu.r[0];
    gsu.r[0] = gsu_fmult(gsu.r[2], gsu.r[6]);
    gsu_code(0x018C29, 8);
    gsu.r[2] = gsu_ldw(FX_CENTRE + 2);
    gsu.r[0] = gsu_add(gsu.r[0], gsu.r[2], 0);
    gsu.r[2] = gsu.r[0];

    gsu.r[6] = 0;
    gsu.r[0] = gsu.r[1];
    gsu_moves(gsu.r[0]);
    if (!gsu.s) {
        gsu_code(0x018C31, 13);
        gsu.r[5] = gsu_ldw(FX_EDGES + 2);
        gsu.r[0] = gsu_sub(gsu.r[0], gsu.r[5], 0);
        if (gsu.s)
            { gsu_code(0x018C3E, 3); goto y_axis; } else { gsu_code(0x018C3E, 4); }
        gsu.r[6] = gsu_inc(gsu.r[6]);
    } else { gsu_code(0x018C31, 9); }
    gsu.r[6] = gsu_inc(gsu.r[6]);
    gsu.r[6] = mult4(gsu.r[6], GSU_COPY(0));
y_axis:
    gsu_code(0x018C46, 5);
    gsu.r[0] = gsu.r[2];
    gsu_moves(gsu.r[0]);
    if (!gsu.s) {
        gsu_code(0x018C4B, 4);
        gsu.r[5] = gsu_ldw(FX_EDGES + 6);
        gsu.r[0] = gsu_sub(gsu.r[0], gsu.r[5], 0);
        if (gsu.s)
            { gsu_code(0x018C4F, 3); goto code; } else { gsu_code(0x018C4F, 4); }
        gsu.r[6] = gsu_inc(gsu.r[6]);
    }
    gsu_code(0x018C53, 1);
    gsu.r[6] = gsu_inc(gsu.r[6]);
code:
    gsu_code(0x018C54, 8);
    gsu.r[0] = gsu_or(gsu_swap(gsu_xor(0x1F, gsu.r[6])), gsu.r[6]);
}
