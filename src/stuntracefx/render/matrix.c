/*
 * matrix.c: 3x3 matrices of signed words in Super FX RAM, nine elements
 * row by row, in 1.15 fixed point (FMULT keeps the high word of the
 * product and ROL doubles it, so each product is (a * b) >> 15). The
 * current one is at $00E4: rows at $E4, $EA, $F0.
 */
#include "gsu.h"

#define FX_MATRIX 0x00E4u

/*
 * @routine 01:972B FxMatrixTranspose
 * WHAT IT DOES: Transposes the 3x3 matrix at R9 in place.
 * HOW: Swaps the three pairs across the diagonal: elements 1 and 3, then
 * 2 and 6 together with 5 and 7, loading each with LDW and storing them
 * back crossed with STW, R9 stepping between them.
 * Entry: R9 = the matrix; R11 = return address. Exit: R9 = the matrix + 4
 * (element 2); R1 = the old element 5, R2 = the old element 7, R3 = the
 * old element 6; the last RAM address R9; OV, S, CY, Z from the last SUB #6.
 * *old5, *old7, *old6 = those three, as the registers keep them.
 */
void FxMatrixTranspose(uint16_t m, uint16_t *old5, uint16_t *old7, uint16_t *old6)
{
    uint16_t e1, e2, e3, e5, e6, e7;

    e1 = gsu_ldw((uint16_t)(m + 2));
    e2 = gsu_ldw((uint16_t)(m + 4));
    e3 = gsu_ldw((uint16_t)(m + 6));
    gsu_stw((uint16_t)(m + 6), e1);
    gsu_stw((uint16_t)(m + 2), e3);
    e5 = gsu_ldw((uint16_t)(m + 10));
    e6 = gsu_ldw((uint16_t)(m + 12));
    gsu_stw((uint16_t)(m + 12), e2);
    e7 = gsu_ldw((uint16_t)(m + 14));
    gsu_stw((uint16_t)(m + 14), e5);
    gsu_stw((uint16_t)(m + 10), e7);
    gsu_stw((uint16_t)(m + 4), e6);
    *old5 = e5;
    *old7 = e7;
    *old6 = e6;
}

void abi_FxMatrixTranspose(void)
{
    uint16_t m = gsu.r[9];

    FxMatrixTranspose(m, &gsu.r[1], &gsu.r[2], &gsu.r[3]);
    gsu.r[9] = gsu_sub((uint16_t)(m + 10), 6, 0);
}

/*
 * @routine 01:978B FxMatrixApply
 * WHAT IT DOES: Multiplies the row vector (R1, R2, R3) by the current
 * matrix and stores the three results as words at R9.
 * HOW: CACHE; for each column, three FMULTs by the column's elements (LMS
 * into R6), each doubled by ROL, summed in R5 then R0 and stored with STW;
 * R9 steps 2 after each.
 * Entry: R1, R2, R3 = the vector, R9 = where to store; R11 = return
 * address. Exit: R9 = R9 + 6; R0 = the last result, R5 = its first two
 * terms, R6 = the matrix's last element; the last RAM address the last
 * result's; OV and CY from the last ADD R5, S and Z from R9.
 * Returns the last result; *partial and *m22 = R5 and R6 as left.
 */
uint16_t FxMatrixApply(uint16_t x, uint16_t y, uint16_t z, uint16_t out, uint16_t *partial, uint16_t *m22)
{
    uint16_t v[3] = {x, y, z};
    uint16_t sum = 0, part = 0, e = 0;
    int col, row;

    for (col = 0; col < 3; col++) {
        sum = 0;
        for (row = 0; row < 3; row++) {
            e = gsu_ldw((uint16_t)(FX_MATRIX + row * 6 + col * 2));
            if (row == 2)
                part = sum;
            sum = (uint16_t)(sum + (uint16_t)((int32_t)(int16_t)v[row] * (int16_t)e >> 15));
        }
        gsu_stw((uint16_t)(out + col * 2), sum);
    }
    *partial = part;
    *m22 = e;
    return sum;
}

void abi_FxMatrixApply(void)
{
    uint16_t out = gsu.r[9], part, e;
    uint16_t sum = FxMatrixApply(gsu.r[1], gsu.r[2], gsu.r[3], out, &part, &e);

    gsu_add((uint16_t)(sum - part), part, 0);
    gsu.r[0] = sum;
    gsu.r[5] = part;
    gsu.r[6] = e;
    gsu.r[9] = (uint16_t)(out + 6);
    gsu_sz(gsu.r[9]);
}
