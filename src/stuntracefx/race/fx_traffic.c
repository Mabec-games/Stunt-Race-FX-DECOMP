/*
 * fx_traffic.c: the Super FX looks for cars about to be caught up with.
 *
 * For car R8, every other car on the cars' list ($70:262E, linked through
 * +$74) that is not flagged out (bit 10 of +$16) and not faster (+$4A) is
 * tested: it counts when the closing speed along the line between them,
 * plus 20, is at least their distance (|dx| + |dz|) / 32, and it is less
 * than $200 to the side (along the first column of the matrix at $00E4).
 * Bits 0 and 1 of R8's +$42 then say on which side it is.
 */
#include <string.h>
#include "gsu_ops.h"
#include "fx_calls.h"

#define FX_CARS 0x262Eu
#define FX_ME_X 0x26F6u
#define FX_ME_Z 0x26FAu
#define FX_ME_VX 0x26FCu
#define FX_ME_VZ 0x2700u
#define FX_DIST 0x263Au
#define FX_CLOSE 0x2638u
#define FX_D_X 0x0068u
#define FX_D_Y 0x002Cu
#define FX_D_Z 0x002Eu
#define FX_MATRIX 0x00E4u

#define REC_X 0x1Eu
#define REC_Z 0x22u
#define REC_VX 0x2Eu
#define REC_VZ 0x32u
#define REC_FLAGS16 0x16u
#define REC_SIDES 0x42u
#define REC_SPEED 0x4Au
#define REC_NEXT_CAR 0x74u
#define REC_Y 0x20u
#define REC_YAW_RATE 0x34u
#define REC_SPREAD 0x7Cu
#define NEAR 0x07D0u           /* 2000 */
#define FX_PLAYER 0x2476u
#define FX_NEAR_LEVEL 0x2642u   /* smoothed closing level, -1 for none */
#define FX_NEAR_SIDE 0x2644u
#define FX_NEAR_BAND 0x2646u
#define FX_NEAR_CAR 0x2648u
#define FX_NEAR_Z 0x264Au
#define FX_NEAR_X 0x264Cu
#define FX_VIEW_X 0x00D8u
#define FX_VIEW_Z 0x00DCu
#define FX_VIEW_YAW 0x00E0u
#define REC_64 0x64u
#define REC_NEAR 0x8Cu

/* The field `field` of record `rec` (R0 = field + rec, then LDW). */
/* gsuwalk: the code of each copy of field, read from the ROM */
static const GsuCode field_fetch[14][1] = {
    {{0x04c936, 8}},
    {{0x04c93e, 4}},
    {{0x04c946, 4}},
    {{0x04c94e, 4}},
    {{0x04c956, 4}},
    {{0x04c974, 6}},
    {{0x04c986, 6}},
    {{0x04c98c, 4}},
    {{0x04c998, 4}},
    {{0x04c9ac, 4}},
    {{0x04c9d4, 4}},
    {{0x04c9dc, 6}},
    {{0x04ca4f, 6}},
    {{0x04ca8d, 4}},
};

static uint16_t field(uint16_t f, uint16_t rec, int copy)
{
    gsu_codes(field_fetch[copy], 1);
    return gsu_ldw(gsu_add(f, rec, 0));
}

/* |v| into a register: MOVES, then NOT and INC when negative. */
/* gsuwalk: the code of each copy of magnitude, read from the ROM */
static const GsuCode magnitude_fetch[4][3] = {
    {{0x04c00f, 9}, {0x04c00f, 6}, {0x04c017, 1}},
    {{0x04c9a4, 8}, {0x04c9a4, 4}, {0x04c9ab, 1}},
    {{0x04c9b8, 8}, {0x04c9b8, 4}, {0x04c9bf, 1}},
    {{0x04ca38, 9}, {0x04ca38, 6}, {0x04ca40, 1}},
};

static uint16_t magnitude(uint16_t v, int copy)
{
    gsu_moves(v);
    return gsu.s ? (gsu_codes(magnitude_fetch[copy], 1), (((gsu_inc(gsu_not(v)))))) : (gsu_codes(magnitude_fetch[copy] + 1, 2), ((v)));
}

/* The dot product of (a, b, c) with the words at $0068, $002C, $002E, each
   FMULT then ROL; R5 = the first two summed, R6 = the last word read. */
/* gsuwalk: the code of each copy of dot, read from the ROM */
static const GsuCode dot_fetch[2][3] = {
    {{0x04c9e6, 7}, {0x04c9ed, 7}, {0x04c9f4, 8}},
    {{0x04ca1e, 3}, {0x04ca21, 7}, {0x04ca28, 8}},
};

static uint16_t dot(FxRegs *g, uint16_t a, uint16_t b, uint16_t c, int copy)
{
    gsu_codes(dot_fetch[copy], 1);
    g->r[6] = gsu_ldw(FX_D_X);
    g->r[5] = gsu_rol(gsu_fmult(a, g->r[6]));
    gsu_codes(dot_fetch[copy] + 1, 1);
    g->r[6] = gsu_ldw(FX_D_Y);
    g->r[5] = gsu_add(g->r[5], gsu_rol(gsu_fmult(b, g->r[6])), 0);
    gsu_codes(dot_fetch[copy] + 2, 1);
    g->r[6] = gsu_ldw(FX_D_Z);
    return gsu_add(gsu_rol(gsu_fmult(c, g->r[6])), g->r[5], 0);
}

/*
 * @routine 04:C933 FxTrafficSides
 * WHAT IT DOES: As described above: for each car that counts, bit 1 of R8's
 * +$42 is set when it is to the left (its offset negative) and bit 0
 * cleared if it was set when the routine started; to the right, bit 0 set
 * and bit 1 cleared likewise. $2638 = -1 for a car $200 or more to the
 * side.
 * HOW: R8's position and velocity are copied to $26F6, $26FA, $26FC,
 * $2700; per car, the difference goes to $0068/$002C/$002E, the distance
 * / 32 (DIV2 five times) to $263A, the closing speed (FMULT, ROL) plus 20
 * to $2638, then the side offset to $263A. R9 holds +$42 as it was at the
 * start; the bits are changed in R6, read again per car. The return
 * address is kept on the R10 stack.
 * Entry: R8 = the car's record; R10 = stack; R11 = return address. Exit:
 * R7 = 0; R9 = the old +$42; the other registers from the last car tested;
 * S, Z from R10; the last RAM address R10.
 */
/* gsuwalk: the code of each copy of FxTrafficSides, read from the ROM */
static const GsuCode FxTrafficSides_fetch[1][4] = {
    {{0x04c942, 4}, {0x04c94a, 4}, {0x04c952, 4}, {0x04c95a, 4}},
};

/* gsuwalk: the copy of field each copy of FxTrafficSides calls, by pass */
static const uint8_t field_in_FxTrafficSides[1][4] = {{1, 2, 3, 4}};

FxRegs FxTrafficSides(FxRegs g, int copy)
{
    static const uint16_t me[4] = {FX_ME_X, FX_ME_Z, FX_ME_VX, FX_ME_VZ};
    static const uint8_t from[4] = {REC_X, REC_Z, REC_VX, REC_VZ};
    int i;

    gsu_code(0x04c934, 2);
    gsu_push(&g.r[10], g.r[11]);
    g.r[9] = field(REC_SIDES, g.r[8], GSU_COPY(0));
    for (i = 0; i < 4; i++) {
        g.r[0] = field(from[i], g.r[8], GSU_COPY(field_in_FxTrafficSides[0][i]));
        gsu_codes(FxTrafficSides_fetch[0] + i, 1);
        gsu_stw(me[i], g.r[0]);
    }
    gsu_code(0x04c95e, 4);
    g.r[0] = gsu_ldw(FX_CARS);
    for (;; g.r[0] = field(REC_NEXT_CAR, g.r[7], GSU_COPY(13)), gsu_code(0x04ca91, 3), gsu_code(0x04c961, 1), gsu_code(0x04ca91, 3), gsu_code(0x04c961, 1), gsu_code(0x04ca91, 3), gsu_code(0x04c961, 1)) {
        g.r[7] = g.r[0];
        gsu_moves(g.r[7]);
        if (gsu.z)
            { 
            gsu_code(0x04c962, 4);
            break; } else { gsu_code(0x04c962, 4); gsu_code(0x04c96a, 1); }
        gsu_sub(g.r[0], g.r[8], 0);
        if (gsu.z)
            { 
            
            gsu_code(0x04c96b, 8); gsu_code(0x04ca8c, 1);
            continue; } else { gsu_code(0x04c96b, 4); gsu_code(0x04c973, 1); }                                /* itself */
        g.r[1] = field(REC_FLAGS16, g.r[7], GSU_COPY(5));
        g.r[0] = gsu_and(0x0400, g.r[1]);
        if (!gsu.z)
            { 
            
            gsu_code(0x04c97a, 10); gsu_code(0x04ca8c, 1);
            continue; } else { gsu_code(0x04c97a, 6); gsu_code(0x04c984, 1); }
        gsu_code(0x04c985, 1);
        g.r[1] = field(REC_SPEED, g.r[7], GSU_COPY(6));
        g.r[0] = gsu_sub(field(REC_SPEED, g.r[8], GSU_COPY(7)), g.r[1], 0);
        if (gsu.s)
            { 
            
            gsu_code(0x04c990, 7); gsu_code(0x04ca8c, 1);
            continue; } else { gsu_code(0x04c990, 3); gsu_code(0x04c997, 1); }                                /* faster */
        g.r[0] = field(REC_X, g.r[7], GSU_COPY(8));
        gsu_code(0x04c99c, 4);
        g.r[1] = gsu_ldw(FX_ME_X);
        g.r[0] = gsu_sub(g.r[0], g.r[1], 0);
        gsu_code(0x04c9a0, 4);
        gsu_stw(FX_D_X, g.r[0]);
        g.r[1] = magnitude(g.r[0], GSU_COPY(1));
        g.r[0] = field(REC_Z, g.r[7], GSU_COPY(9));
        gsu_code(0x04c9b0, 4);
        g.r[3] = gsu_ldw(FX_ME_Z);
        g.r[0] = gsu_sub(g.r[0], g.r[3], 0);
        gsu_code(0x04c9b4, 4);
        gsu_stw(FX_D_Z, g.r[0]);
        g.r[3] = magnitude(g.r[0], GSU_COPY(2));
        g.r[0] = gsu_sub(g.r[0], g.r[0], 0);
        gsu_code(0x04c9c0, 4);
        gsu_stw(FX_D_Y, g.r[0]);
        g.r[0] = gsu_add(g.r[1], g.r[3], 0);
        gsu_code(0x04c9c4, 12);
        for (i = 0; i < 5; i++)
            g.r[0] = gsu_div2(g.r[0]);
        gsu_code(0x04c9d0, 4);
        gsu_stw(FX_DIST, g.r[0]);
        g.r[0] = field(REC_VX, g.r[7], GSU_COPY(10));
        gsu_code(0x04c9d8, 4);
        g.r[1] = gsu_sub(gsu_ldw(FX_ME_VX), g.r[0], 0);
        g.r[0] = field(REC_VZ, g.r[7], GSU_COPY(11));
        gsu_code(0x04c9e2, 4);
        g.r[3] = gsu_sub(gsu_ldw(FX_ME_VZ), g.r[0], 0);
        g.r[2] = 0;
        g.r[0] = dot(&g, g.r[1], g.r[2], g.r[3], GSU_COPY(0));
        g.r[1] = 0x0014;
        g.r[0] = gsu_add(g.r[0], g.r[1], 0);
        gsu_code(0x04c9fc, 12);
        gsu_stw(FX_CLOSE, g.r[0]);
        gsu_code(0x04ca08, 4);
        g.r[1] = gsu_ldw(FX_DIST);
        gsu_sub(g.r[0], g.r[1], 0);
        if (gsu.s)
            { 
            
            gsu_code(0x04ca0c, 8); gsu_code(0x04ca8c, 1);
            continue; } else { gsu_code(0x04ca0c, 4); gsu_code(0x04ca14, 1); }                                /* not closing in time */
        gsu_code(0x04ca15, 3);
        g.r[1] = gsu_ldw(FX_MATRIX);
        gsu_code(0x04ca18, 3);
        g.r[2] = gsu_ldw(FX_MATRIX + 2);
        gsu_code(0x04ca1b, 3);
        g.r[3] = gsu_ldw(FX_MATRIX + 4);
        g.r[0] = dot(&g, g.r[1], g.r[2], g.r[3], GSU_COPY(1));
        gsu_code(0x04ca30, 8);
        gsu_stw(FX_DIST, g.r[0]);
        g.r[1] = g.r[0];
        g.r[0] = magnitude(g.r[0], GSU_COPY(3));
        g.r[2] = 0x0200;
        gsu_sub(g.r[0], g.r[2], 0);
        if (!gsu.s) {
            g.r[2] = 0xFFFF;
            gsu_code(0x04ca41, 14);
            gsu_stw(FX_CLOSE, g.r[2]);
        } else { gsu_code(0x04ca41, 7); gsu_code(0x04ca4e, 1); }
        g.r[6] = field(REC_SIDES, g.r[8], GSU_COPY(12));
        gsu_code(0x04ca55, 4);
        g.r[0] = gsu_ldw(FX_CLOSE);
        gsu_moves(g.r[0]);
        if (!gsu.s) {
            gsu_moves(g.r[1]);
            if (gsu.s) {                             /* to the left */
                g.r[0] = gsu_and(0x0001, g.r[9]);
                if (!gsu.z) {
                    g.r[0] = 0xFFFE;
                    g.r[6] = gsu_and(g.r[0], g.r[6]);
                    gsu_code(0x04ca59, 22);
                } else { gsu_code(0x04ca59, 16); gsu_code(0x04ca6e, 1); }
                g.r[0] = 0x0002;
                g.r[6] = gsu_or(g.r[0], g.r[6]);
                
                
                gsu_code(0x04ca6f, 7); gsu_code(0x04ca87, 1);
            } else {
                g.r[0] = gsu_and(0x0002, g.r[9]);
                if (!gsu.z) {
                    g.r[0] = 0xFFFD;
                    g.r[6] = gsu_and(g.r[0], g.r[6]);
                    
                    
                    gsu_code(0x04ca59, 9); gsu_code(0x04ca76, 13);
                } else { gsu_code(0x04ca59, 9); gsu_code(0x04ca76, 7); gsu_code(0x04ca82, 1); }
                g.r[0] = 0x0001;
                g.r[6] = gsu_or(g.r[0], g.r[6]);
                gsu_code(0x04ca83, 5);
            }
        } else { gsu_code(0x04ca59, 4); gsu_code(0x04ca87, 1); }
        g.r[0] = gsu_add(REC_SIDES, g.r[8], 0);
        gsu_code(0x04ca88, 5);
        gsu_stw(g.r[0], g.r[6]);
    }
    
    
    gsu_code(0x04c966, 4); gsu_code(0x04ca94, 5);
    gsu_pull(&g.r[10]);
    return g;
}

void abi_FxTrafficSides(void)
{
    FxRegs g;

    memcpy(g.r, gsu.r, sizeof g.r);
    g = FxTrafficSides(g, GSU_COPY(0));
    memcpy(gsu.r, g.r, 15 * sizeof g.r[0]);
}

/* |v| / 2 by DIV2 (S from it), then NOT and INC when negative. */
/* gsuwalk: the code of each copy of half_magnitude, read from the ROM */
static const GsuCode half_magnitude_fetch[3][3] = {
    {{0x04bfd0, 10}, {0x04bfd0, 7}, {0x04bfd9, 1}},
    {{0x04bfe0, 8}, {0x04bfe0, 5}, {0x04bfe7, 1}},
    {{0x04bfee, 10}, {0x04bfee, 7}, {0x04bff7, 1}},
};

static uint16_t half_magnitude(uint16_t v, int copy)
{
    v = gsu_div2(v);
    return gsu.s ? (gsu_codes(half_magnitude_fetch[copy], 1), (((gsu_inc(gsu_not(v)))))) : (gsu_codes(half_magnitude_fetch[copy] + 1, 2), ((v)));
}

/*
 * @routine 04:BF98 FxSpreadCars
 * WHAT IT DOES: Pushes the other cars near car R8 away from it: for each
 * car on the cars' list (not R8) whose distance (|dx| + |dy| + |dz|) / 2
 * is below 2000, its velocity x and z gain twice (2000 - that) along the
 * unit vector from R8 to it, its +$34 gains or loses a share of it (by the
 * sign of +$34: times $C180 or $3E80, scaled), and its +$7C = 30. A
 * negative half z distance is made positive by code decoded from the ROM
 * bytes and not reached in the traces so far.
 * HOW: R8's position in R6, R5, R7 and R2 = 2000; per car, the differences
 * (x into R1, z into R3) halved with DIV2; for a near car R8, R6, R5, R7,
 * R2, the push and the car are kept on the R10 stack around FxNormalize of
 * (dx, 0, dz), and each push is FMULT then ADD and SBK. R8 and the return
 * address are kept on the R10 stack.
 * Entry: R8 = the car's record; R10 = stack; R11 = return address. Exit:
 * R9 = 0; R8, R6, R5, R7, R2 as set up; the others from the last car; S,
 * Z from R10; the last RAM address R10.
 */
FxRegs FxSpreadCars(FxRegs g)
{
    uint16_t *sp = &g.r[10];

    gsu_code(0x04bf99, 2);
    gsu_push(sp, g.r[11]);
    gsu_code(0x04bf9b, 4);
    gsu_push(sp, g.r[8]);
    gsu_code(0x04bf9f, 8);
    g.r[6] = gsu_ldw(gsu_add(REC_X, g.r[8], 0));
    gsu_code(0x04bfa7, 6);
    g.r[5] = gsu_ldw(gsu_add(REC_Y, g.r[8], 0));
    gsu_code(0x04bfad, 6);
    g.r[7] = gsu_ldw(gsu_add(REC_Z, g.r[8], 0));
    g.r[2] = NEAR;
    gsu_code(0x04bfb3, 7);
    g.r[0] = gsu_ldw(FX_CARS);
    for (;; g.r[0] = (((gsu_code(0x04c077, 4), gsu_ldw(gsu_add(REC_NEXT_CAR, g.r[9], 0))))), gsu_code(0x04c07b, 3), gsu_code(0x04bfb9, 1), gsu_code(0x04c07b, 3), gsu_code(0x04bfb9, 1), gsu_code(0x04c07b, 3), gsu_code(0x04bfb9, 1)) {
        g.r[9] = g.r[0];
        gsu_moves(g.r[9]);
        if (gsu.z)
            { 
            gsu_code(0x04bfba, 4);
            break; } else { gsu_code(0x04bfba, 4); gsu_code(0x04bfc2, 1); }
        gsu_sub(g.r[0], g.r[8], 0);
        if (gsu.z)
            { 
            
            gsu_code(0x04bfc3, 8); gsu_code(0x04c076, 1);
            continue; } else { gsu_code(0x04bfc3, 4); gsu_code(0x04bfcb, 1); }
        gsu_code(0x04bfcc, 4);
        g.r[0] = gsu_sub(gsu_ldw(gsu_add(REC_X, g.r[9], 0)), g.r[6], 0);
        g.r[1] = g.r[0];
        
        g.r[4] = half_magnitude(g.r[0], GSU_COPY(0));
        gsu_code(0x04bfda, 6);
        g.r[0] = gsu_sub(gsu_ldw(gsu_add(REC_Y, g.r[9], 0)), g.r[5], 0);
        
        g.r[4] = gsu_add(g.r[4], half_magnitude(g.r[0], GSU_COPY(1)), 0);
        gsu_code(0x04bfe8, 6);
        g.r[0] = gsu_sub(gsu_ldw(gsu_add(REC_Z, g.r[9], 0)), g.r[7], 0);
        g.r[3] = g.r[0];
        
        g.r[0] = half_magnitude(g.r[0], GSU_COPY(2));
        g.r[0] = gsu_add(g.r[0], g.r[4], 0);
        g.r[0] = gsu_sub(g.r[0], g.r[2], 0);
        if (!gsu.s)
            { 
            
            gsu_code(0x04bff8, 4); gsu_code(0x04c076, 1);
            continue; } else { gsu_code(0x04bff8, 4); }                                /* not near */
        gsu_code(0x04bffc, 3);
        gsu_push(sp, g.r[8]);
        gsu_code(0x04bfff, 4);
        gsu_push(sp, g.r[6]);
        gsu_code(0x04c003, 4);
        gsu_push(sp, g.r[5]);
        gsu_code(0x04c007, 4);
        gsu_push(sp, g.r[7]);
        gsu_code(0x04c00b, 4);
        gsu_push(sp, g.r[2]);
        g.r[0] = magnitude(g.r[0], GSU_COPY(0));
        g.r[0] = gsu_add(g.r[0], g.r[0], 0);
        gsu_code(0x04c018, 2);
        gsu_push(sp, g.r[0]);
        gsu_code(0x04c01a, 4);
        gsu_push(sp, g.r[9]);
        g.r[2] = 0;
        g.r[11] = 0xC026;
        
        
        
        gsu_code(0x04c01e, 8); gsu_code(0x04c087, 1);
        g = FxNormalize(g, GSU_COPY(0));
        gsu_code(0x04c026, 5);
        g.r[9] = gsu_pull(sp);
        gsu_code(0x04c02b, 4);
        g.r[6] = gsu_pull(sp);
        g.r[0] = gsu_fmult(g.r[1], g.r[6]);
        gsu_code(0x04c02f, 8);
        g.r[4] = gsu_ldw(gsu_add(REC_VX, g.r[9], 0));
        g.r[0] = gsu_add(g.r[0], g.r[4], 0);
        gsu_code(0x04c037, 2);
        gsu_sbk(g.r[0]);
        g.r[0] = gsu_fmult(g.r[3], g.r[6]);
        gsu_code(0x04c039, 8);
        g.r[4] = gsu_ldw(gsu_add(REC_VZ, g.r[9], 0));
        g.r[0] = gsu_add(g.r[0], g.r[4], 0);
        gsu_code(0x04c041, 2);
        gsu_sbk(g.r[0]);
        gsu_code(0x04c043, 4);
        g.r[0] = gsu_ldw(gsu_add(REC_YAW_RATE, g.r[9], 0));
        g.r[1] = g.r[0];
        g.r[0] = 0xC180;
        gsu_moves(g.r[1]);
        if (!gsu.s)
            { g.r[0] = 0x3E80; gsu_code(0x04c047, 13); } else { gsu_code(0x04c047, 9); gsu_code(0x04c053, 1); }
        g.r[6] = gsu_add(g.r[6], g.r[6], 0);
        g.r[6] = gsu_add(g.r[6], g.r[6], 0);
        g.r[0] = gsu_add(gsu_fmult(g.r[0], g.r[6]), g.r[1], 0);
        gsu_code(0x04c054, 7);
        gsu_sbk(g.r[0]);
        g.r[4] = 0x001E;
        g.r[0] = gsu_add(REC_SPREAD, g.r[9], 0);
        gsu_code(0x04c05b, 8);
        gsu_stw(g.r[0], g.r[4]);
        gsu_code(0x04c063, 4);
        g.r[2] = gsu_pull(sp);
        gsu_code(0x04c067, 4);
        g.r[7] = gsu_pull(sp);
        gsu_code(0x04c06b, 4);
        g.r[5] = gsu_pull(sp);
        gsu_code(0x04c06f, 4);
        g.r[6] = gsu_pull(sp);
        gsu_code(0x04c073, 4);
        g.r[8] = gsu_pull(sp);
    }
    
    
    gsu_code(0x04bfbe, 4); gsu_code(0x04c07e, 5);
    g.r[8] = gsu_pull(sp);
    gsu_code(0x04c083, 4);
    gsu_pull(sp);
    return g;
}

void abi_FxSpreadCars(void)
{
    FxRegs g;

    memcpy(g.r, gsu.r, sizeof g.r);
    g = FxSpreadCars(g);
    memcpy(gsu.r, g.r, 15 * sizeof g.r[0]);
}

/* The early exit: no car near; $2642 = -1, $2648 = 0. */
/* gsuwalk: the code of each copy of near_none, read from the ROM */
static const GsuCode near_none_fetch[3][4] = {
    {{0x04cf16, 3}, {0x04d030, 7}, {0x04d037, 5}, {0x04d03c, 4}},
    {{0x04cf49, 3}, {0x04d030, 7}, {0x04d037, 5}, {0x04d03c, 4}},
    {{0x04cf96, 3}, {0x04d030, 7}, {0x04d037, 5}, {0x04d03c, 4}},
};

static FxRegs near_none(FxRegs g, int copy)
{
    g.r[0] = 0xFFFF;
    gsu_codes(near_none_fetch[copy], 2);
    gsu_stw(FX_NEAR_LEVEL, g.r[0]);
    g.r[0] = gsu_sub(g.r[0], g.r[0], 0);
    gsu_codes(near_none_fetch[copy] + 2, 1);
    gsu_stw(FX_NEAR_CAR, g.r[0]);
    gsu_codes(near_none_fetch[copy] + 3, 1);
    gsu_pull(&g.r[10]);
    return g;
}

/*
 * @routine 04:CF08 FxNearestCar
 * WHAT IT DOES: Finds the car nearest the player's car ($2476; from the
 * view's position $00D8/$00DC instead when bit 3 of its +$64 is set) by
 * |dx| + |dz|, keeping each car's distance in its +$8C, and for the
 * nearest: $2648 = the car; $2642 = a smoothed level from how fast it
 * closed ((old distance - new) / 2 + $80, 0 at least, 255 at most, a
 * quarter of the way from the old level each step); $2646 = its distance
 * / 512, 7 at most; $264A/$264C = its offset turned into the view's axes
 * (FxTurn by the high byte of $00E0), x negated; $2644 = 7 + that z / 32,
 * 0 to 15. With no player car, no other car or no old distance: $2642 =
 * -1, $2648 = 0. The case of an empty cars' list is decoded from the ROM
 * bytes and not reached in the traces so far.
 * HOW: R5 = the best distance (from $1000), R7 its car, R6 its old +$8C;
 * the sums are checked for overflow (BVS). The return address is kept on
 * the R10 stack. The walk runs under CACHE (the code cache only; nothing
 * the C has to do).
 * Entry: R10 = stack; R11 = return address. Exit: R0 = the side value or
 * 0; R7 = the nearest car; R11 = $D002 when it got that far; S, Z from
 * R10; the last RAM address R10.
 */
FxRegs FxNearestCar(FxRegs g, int copy)
{
    int i;

    gsu_code(0x04cf09, 2);
    gsu_push(&g.r[10], g.r[11]);
    gsu_code(0x04cf0b, 6);
    g.r[3] = gsu_ldw(FX_PLAYER);
    gsu_moves(g.r[3]);
    if (gsu.z)
        { 
        gsu_code(0x04cf11, 5);
        return near_none(g, GSU_COPY(0)); } else { gsu_code(0x04cf11, 4); gsu_code(0x04cf19, 1); }
    gsu_code(0x04cf1a, 6);
    g.r[1] = gsu_ldw(gsu_add(REC_X, g.r[3], 0));
    gsu_code(0x04cf20, 6);
    g.r[2] = gsu_ldw(gsu_add(REC_Z, g.r[3], 0));
    gsu_code(0x04cf26, 6);
    g.r[8] = gsu_ldw(gsu_add(REC_64, g.r[3], 0));
    g.r[0] = gsu_and(0x0008, g.r[8]);
    if (!gsu.z) {
        g.r[3] = 0;
        gsu_code(0x04cf2c, 12);
        g.r[1] = gsu_ldw(FX_VIEW_X);
        gsu_code(0x04cf38, 3);
        g.r[2] = gsu_ldw(FX_VIEW_Z);
    } else { gsu_code(0x04cf2c, 6); gsu_code(0x04cf3a, 1); }
    g.r[5] = 0x1000;
    g.r[6] = 0;
    gsu_code(0x04cf3b, 9);
    g.r[0] = gsu_ldw(FX_CARS);
    gsu_moves(g.r[0]);
    if (gsu.z)
        { 
        gsu_code(0x04cf44, 5);
        return near_none(g, GSU_COPY(1)); } else { gsu_code(0x04cf44, 4); gsu_code(0x04cf4c, 1); gsu_cache(0xcf4d); }
    gsu_code(0x04cf4d, 1);
    for (;; g.r[0] = (((gsu_code(0x04cf8a, 4), gsu_ldw(gsu_add(REC_NEXT_CAR, g.r[8], 0))))), gsu_code(0x04cf8e, 2), gsu_code(0x04cf4d, 1), gsu_code(0x04cf8e, 2), gsu_code(0x04cf4d, 1), gsu_code(0x04cf8e, 2), gsu_code(0x04cf4d, 1)) {
        g.r[8] = g.r[0];
        gsu_moves(g.r[8]);
        if (gsu.z)
            { 
            
            gsu_code(0x04cf4e, 4); gsu_code(0x04cf90, 1);
            break; } else { gsu_code(0x04cf4e, 4); }
        gsu_sub(g.r[0], g.r[3], 0);
        if (gsu.z)
            { 
            
            gsu_code(0x04cf52, 5); gsu_code(0x04cf89, 1);
            continue; } else { gsu_code(0x04cf52, 5); }
        gsu_code(0x04cf57, 5);
        g.r[0] = gsu_sub(gsu_ldw(gsu_add(REC_X, g.r[8], 0)), g.r[1], 0);
        if (gsu.s)
            { g.r[0] = gsu_inc(gsu_not(g.r[0])); gsu_code(0x04cf5c, 6); } else { gsu_code(0x04cf5c, 3); gsu_code(0x04cf61, 1); }
        
        g.r[4] = g.r[0];
        gsu_code(0x04cf62, 6);
        g.r[0] = gsu_sub(gsu_ldw(gsu_add(REC_Z, g.r[8], 0)), g.r[2], 0);
        if (gsu.s)
            { g.r[0] = gsu_inc(gsu_not(g.r[0])); gsu_code(0x04cf68, 6); } else { gsu_code(0x04cf68, 3); gsu_code(0x04cf6d, 1); }
        
        g.r[4] = gsu_add(g.r[4], g.r[0], 0);
        if (gsu.ov)
            { 
            
            gsu_code(0x04cf6e, 4); gsu_code(0x04cf89, 1);
            continue; } else { gsu_code(0x04cf6e, 4); }
        gsu_sub(g.r[4], g.r[5], 0);
        if (gsu.s) {
            gsu_code(0x04cf72, 14);
            g.r[6] = gsu_ldw(gsu_add(REC_NEAR, g.r[8], 0));
            g.r[5] = g.r[4];
            g.r[7] = g.r[8];
            gsu_code(0x04cf80, 4);
        } else { gsu_code(0x04cf72, 6); gsu_code(0x04cf83, 1); }
        g.r[0] = gsu_add(REC_NEAR, g.r[8], 0);
        gsu_code(0x04cf84, 6);
        gsu_stw(g.r[0], g.r[4]);
    }
    g.r[0] = g.r[6];
    gsu_moves(g.r[0]);
    if (gsu.z)
        { 
        gsu_code(0x04cf91, 5);
        return near_none(g, GSU_COPY(2)); } else { gsu_code(0x04cf91, 4); gsu_code(0x04cf99, 1); }
    g.r[0] = gsu_sub(g.r[6], g.r[5], 0);
    g.r[6] = gsu_div2(g.r[0]);
    g.r[0] = 0x0080;
    g.r[6] = gsu_add(g.r[6], g.r[0], 0);
    if (gsu.s)
        { g.r[6] = 0; gsu_code(0x04cf9a, 15); } else { gsu_code(0x04cf9a, 12); gsu_code(0x04cfa8, 1); }
    gsu_code(0x04cfa9, 4);
    g.r[0] = gsu_ldw(FX_NEAR_LEVEL);
    gsu_moves(g.r[0]);
    if (gsu.s) {
        g.r[0] = g.r[6];
        gsu_code(0x04cfad, 11);
        gsu_stw(FX_NEAR_LEVEL, g.r[0]);
    } else { gsu_code(0x04cfad, 4); gsu_code(0x04cfb7, 1); }
    g.r[0] = gsu_sub(g.r[0], g.r[6], 0);
    g.r[0] = gsu_div2(gsu_div2(g.r[0]));
    gsu_code(0x04cfb8, 9);
    g.r[8] = gsu_ldw(FX_NEAR_LEVEL);
    g.r[0] = gsu_sub(g.r[8], g.r[0], 0);
    gsu_code(0x04cfc1, 3);
    gsu_sbk(g.r[0]);
    g.r[6] = 0x0080;
    g.r[0] = gsu_fmult(g.r[5], g.r[6]);
    g.r[8] = 0x0007;
    gsu_sub(g.r[0], g.r[8], 0);
    if (!gsu.s)
        { g.r[0] = g.r[8]; gsu_code(0x04cfc4, 15); } else { gsu_code(0x04cfc4, 12); gsu_code(0x04cfd2, 1); }
    
    gsu_code(0x04cfd3, 4);
    gsu_stw(FX_NEAR_BAND, g.r[0]);
    g.r[8] = 0x00FF;
    gsu_code(0x04cfd7, 7);
    g.r[0] = gsu_ldw(FX_NEAR_LEVEL);
    gsu_sub(g.r[0], g.r[8], 0);
    if (!gsu.s)
        { 
        gsu_code(0x04cfde, 7);
        gsu_sbk(g.r[8]); } else { gsu_code(0x04cfde, 4); gsu_code(0x04cfe4, 1); }
    gsu_code(0x04cfe5, 4);
    gsu_stw(FX_NEAR_CAR, g.r[7]);
    gsu_code(0x04cfe9, 4);
    g.r[0] = gsu_ldw(gsu_add(REC_X, g.r[7], 0));
    g.r[1] = gsu_sub(g.r[0], g.r[1], 0);
    gsu_code(0x04cfed, 6);
    g.r[0] = gsu_ldw(gsu_add(REC_Z, g.r[7], 0));
    g.r[3] = gsu_sub(g.r[0], g.r[2], 0);
    gsu_code(0x04cff3, 5);
    g.r[0] = gsu_ldw(FX_VIEW_YAW);
    g.r[2] = gsu_hib(g.r[0]);
    g.r[0] = 0;
    
    gsu_code(0x04cff8, 6);
    gsu_romb(0);
    g.r[11] = 0xD002;
    g = FxTurnRegs(g, GSU_COPY(1));
    gsu_code(0x04d002, 5);
    gsu_stw(FX_NEAR_Z, g.r[3]);
    g.r[1] = gsu_inc(gsu_not(g.r[1]));
    gsu_code(0x04d007, 7);
    gsu_stw(FX_NEAR_X, g.r[1]);
    g.r[0] = g.r[3];
    
    gsu_code(0x04d00e, 13);
    for (i = 0; i < 5; i++)
        g.r[0] = gsu_div2(g.r[0]);
    g.r[0] = gsu_add(g.r[0], 7, 0);
    if (gsu.s)
        { g.r[0] = gsu_sub(g.r[0], g.r[0], 0); gsu_code(0x04d01b, 4); } else { gsu_code(0x04d01b, 2); gsu_code(0x04d01e, 1); }
    g.r[3] = 0x000F;
    gsu_sub(g.r[0], g.r[3], 0);
    if (!gsu.s)
        { g.r[0] = g.r[3]; gsu_code(0x04d01f, 9); } else { gsu_code(0x04d01f, 6); gsu_code(0x04d027, 1); }
    gsu_code(0x04d028, 4);
    gsu_stw(FX_NEAR_SIDE, g.r[0]);
    gsu_code(0x04d02c, 4);
    gsu_pull(&g.r[10]);
    return g;
}

void abi_FxNearestCar(void)
{
    FxRegs g;

    memcpy(g.r, gsu.r, sizeof g.r);
    g = FxNearestCar(g, GSU_COPY(0));
    memcpy(gsu.r, g.r, 15 * sizeof g.r[0]);
}
