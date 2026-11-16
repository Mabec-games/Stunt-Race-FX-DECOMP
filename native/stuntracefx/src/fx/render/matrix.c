/*
 * matrix.c: the Super FX code of asm/stuntracefx/render/matrix.asm, written from it by
 * the generator.
 *
 * matrix.asm: 3x3 matrices of signed words in Super FX RAM, nine elements
 * row by row, in 1.15 fixed point (FMULT keeps the high word of the
 * product and ROL doubles it, so each product is (a * b) >> 15). The
 * current one is at $00E4: rows at $E4, $EA, $F0.
 * C: src/stuntracefx/render/matrix.c.
 */

#include "fx/fx.h"
#include "fx/fx_face.h"

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
void FxMatrixTranspose(uint16_t matrix)
{
    uint16_t element;
    uint16_t element_2;
    uint16_t element_3;
    uint16_t element_4;
    uint16_t element_5;
    uint16_t element_6;
    uint16_t ptr_plus_2;
    uint16_t ptr_plus_4;
    uint16_t ptr_plus_6;
    uint16_t ptr_plus_10;
    uint16_t ptr_plus_12;
    uint16_t ptr_plus_14;
    uint16_t ptr_plus_2_again;
    uint16_t ptr_plus_10_again;

    ptr_plus_2 = (uint16_t)(matrix + 2);
    element = fx_ldw(ptr_plus_2);
    ptr_plus_4 = (uint16_t)(ptr_plus_2 + 2);
    element_2 = fx_ldw(ptr_plus_4);
    ptr_plus_6 = (uint16_t)(ptr_plus_4 + 2);
    element_3 = fx_ldw(ptr_plus_6);
    fx_stw(ptr_plus_6, element);
    ptr_plus_2_again = (uint16_t)(ptr_plus_6 - 4);
    fx_stw(ptr_plus_2_again, element_3);
    ptr_plus_10 = (uint16_t)(ptr_plus_2_again + 8);
    element_4 = fx_ldw(ptr_plus_10);
    ptr_plus_12 = (uint16_t)(ptr_plus_10 + 2);
    element_5 = fx_ldw(ptr_plus_12);
    fx_stw(ptr_plus_12, element_2);
    ptr_plus_14 = (uint16_t)(ptr_plus_12 + 2);
    element_6 = fx_ldw(ptr_plus_14);
    fx_stw(ptr_plus_14, element_4);
    ptr_plus_10_again = (uint16_t)(ptr_plus_14 - 4);
    fx_stw(ptr_plus_10_again, element_6);
    fx_stw((uint16_t)(ptr_plus_10_again - 6), element_5);
    return;
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
uint16_t FxMatrixApply(uint16_t vector_x, uint16_t vector_y, uint16_t vector_z, uint16_t where_store)
{
    uint32_t col0_x_prod;
    uint32_t col0_y_prod;
    uint32_t col0_z_prod;
    uint32_t col1_x_prod;
    uint32_t col1_y_prod;
    uint32_t col1_z_prod;
    uint32_t col2_x_prod;
    uint32_t col2_y_prod;
    uint32_t col2_z_prod;
    uint16_t matrix_elem_0;
    uint16_t col1_store_ptr;
    uint16_t col2_store_ptr;
    uint16_t object_matrix_1;
    uint16_t object_matrix_2;
    uint16_t object_matrix_3;
    uint16_t object_matrix_4;
    uint16_t object_matrix_5;
    uint16_t object_matrix_6;
    uint16_t object_matrix_7;
    uint16_t object_matrix_8;

    matrix_elem_0 = fx_ldw(0x00E4);
    col0_x_prod = (uint32_t)((int32_t)(int16_t)vector_x * (int16_t)matrix_elem_0);
    object_matrix_3 = fx_ldw(FX_OBJECT_MATRIX_3);
    col0_y_prod = (uint32_t)((int32_t)(int16_t)vector_y * (int16_t)object_matrix_3);
    object_matrix_6 = fx_ldw(FX_OBJECT_MATRIX_6);
    col0_z_prod = (uint32_t)((int32_t)(int16_t)vector_z * (int16_t)object_matrix_6);
    fx_stw(where_store, (uint16_t)((uint16_t)((uint16_t)((uint16_t)(col0_z_prod >> 16) << 1 | (int)(col0_z_prod >> 15 & 1)) + (uint16_t)((uint16_t)(col0_x_prod >> 16) << 1 | (int)(col0_x_prod >> 15 & 1))) + (uint16_t)((uint16_t)(col0_y_prod >> 16) << 1 | (int)(col0_y_prod >> 15 & 1))));
    col1_store_ptr = (uint16_t)(where_store + 2);
    object_matrix_1 = fx_ldw(FX_OBJECT_MATRIX_1);
    col1_x_prod = (uint32_t)((int32_t)(int16_t)vector_x * (int16_t)object_matrix_1);
    object_matrix_4 = fx_ldw(FX_OBJECT_MATRIX_4);
    col1_y_prod = (uint32_t)((int32_t)(int16_t)vector_y * (int16_t)object_matrix_4);
    object_matrix_7 = fx_ldw(FX_OBJECT_MATRIX_7);
    col1_z_prod = (uint32_t)((int32_t)(int16_t)vector_z * (int16_t)object_matrix_7);
    fx_stw(col1_store_ptr, (uint16_t)((uint16_t)((uint16_t)((uint16_t)(col1_z_prod >> 16) << 1 | (int)(col1_z_prod >> 15 & 1)) + (uint16_t)((uint16_t)(col1_x_prod >> 16) << 1 | (int)(col1_x_prod >> 15 & 1))) + (uint16_t)((uint16_t)(col1_y_prod >> 16) << 1 | (int)(col1_y_prod >> 15 & 1))));
    col2_store_ptr = (uint16_t)(col1_store_ptr + 2);
    object_matrix_2 = fx_ldw(FX_OBJECT_MATRIX_2);
    col2_x_prod = (uint32_t)((int32_t)(int16_t)vector_x * (int16_t)object_matrix_2);
    object_matrix_5 = fx_ldw(FX_OBJECT_MATRIX_5);
    col2_y_prod = (uint32_t)((int32_t)(int16_t)vector_y * (int16_t)object_matrix_5);
    object_matrix_8 = fx_ldw(FX_OBJECT_MATRIX_8);
    col2_z_prod = (uint32_t)((int32_t)(int16_t)vector_z * (int16_t)object_matrix_8);
    fx_stw(col2_store_ptr, (uint16_t)((uint16_t)((uint16_t)((uint16_t)(col2_z_prod >> 16) << 1 | (int)(col2_z_prod >> 15 & 1)) + (uint16_t)((uint16_t)(col2_x_prod >> 16) << 1 | (int)(col2_x_prod >> 15 & 1))) + (uint16_t)((uint16_t)(col2_y_prod >> 16) << 1 | (int)(col2_y_prod >> 15 & 1))));
    return (uint16_t)((uint16_t)(col2_store_ptr + 1) + 1);
}


/*
 * @routine 01:975B FxMatrixApplyVectors
 * WHAT IT DOES: Multiplies the three vectors at $0116, $011C and $0122 by
 * the current matrix, storing the nine results as words from R9 on.
 * HOW: Keeps R11 in R8, then for each vector loads it into R1, R2, R3 with
 * LMS and calls FxMatrixApply, which steps R9; restores R11 and returns.
 * Entry: R9 = where to store; R11 = return address. Exit: R9 = R9 + 18;
 * R1, R2, R3 = the third vector; R8 = R11; R0, R5, R6, the last RAM address
 * and the flags as the last FxMatrixApply leaves them.
 */
void FxMatrixApplyVectors(uint16_t where_store, uint16_t return_address)
{
    uint16_t vectors;
    uint16_t vector_x;
    uint16_t vector_z;
    uint16_t model_118;
    uint16_t model_11e;
    uint16_t model_124;
    uint16_t vector_x_2;
    uint16_t vector_z_2;
    uint16_t vector_z_3;
    uint16_t next_store_ptr;
    uint16_t next_store_ptr_2;

    vectors = fx_ldw(GSU_VECTORS);
    model_118 = fx_ldw(FX_MODEL_118);
    vector_z = fx_ldw(0x011A);
    next_store_ptr = FxMatrixApply(vectors, model_118, vector_z, where_store);
    vector_x = fx_ldw(0x011C);
    model_11e = fx_ldw(FX_MODEL_11E);
    vector_z_2 = fx_ldw(0x0120);
    next_store_ptr_2 = FxMatrixApply(vector_x, model_11e, vector_z_2, next_store_ptr);
    vector_x_2 = fx_ldw(0x0122);
    model_124 = fx_ldw(FX_MODEL_124);
    vector_z_3 = fx_ldw(0x0126);
    FxMatrixApply(vector_x_2, model_124, vector_z_3, next_store_ptr_2);
    return;
}


/*
 * @routine 01:8277 FxMatrixRotateBackProgram
 * WHAT IT DOES: A program the S-CPU starts: as FxMatrixRotateProgram with
 * the matrix transposed (the vector times its columns), back from the
 * object's axes: ($0068, $002C, $002E) to $0026, $0028, $002A.
 * HOW: As FxMatrixRotateProgram, output i summing the elements at $00E4 +
 * 2 * i, + 6 and + 12. Then STOP.
 * Entry: started by the S-CPU. Exit: as FxMatrixRotateProgram, R6 = the
 * element at $00F4.
 */
void FxMatrixRotateBackProgram(void)
{
    uint16_t vec_x;
    uint16_t vec_y;
    uint16_t vec_z;
    uint32_t prod_x_m0;
    uint32_t prod_x_m1;
    uint32_t prod_x_m2;
    uint32_t prod_y_m3;
    uint32_t prod_y_m4;
    uint32_t prod_y_m5;
    uint32_t prod_z_m6;
    uint32_t prod_z_m7;
    uint32_t prod_z_m8;
    uint16_t matrix_elem_0;
    uint16_t object_matrix_1;
    uint16_t object_matrix_2;
    uint16_t object_matrix_3;
    uint16_t object_matrix_4;
    uint16_t object_matrix_5;
    uint16_t object_matrix_6;
    uint16_t object_matrix_7;
    uint16_t object_matrix_8;

    vec_x = fx_ldw(0x0068);
    vec_y = fx_ldw(0x002C);
    vec_z = fx_ldw(0x002E);
    matrix_elem_0 = fx_ldw(0x00E4);
    prod_x_m0 = (uint32_t)((int32_t)(int16_t)vec_x * (int16_t)matrix_elem_0);
    object_matrix_3 = fx_ldw(FX_OBJECT_MATRIX_3);
    prod_y_m3 = (uint32_t)((int32_t)(int16_t)vec_y * (int16_t)object_matrix_3);
    object_matrix_6 = fx_ldw(FX_OBJECT_MATRIX_6);
    prod_z_m6 = (uint32_t)((int32_t)(int16_t)vec_z * (int16_t)object_matrix_6);
    fx_stw(0x0026, (uint16_t)((uint16_t)((uint16_t)((uint16_t)(prod_z_m6 >> 16) << 1 | (int)(prod_z_m6 >> 15 & 1)) + (uint16_t)((uint16_t)(prod_x_m0 >> 16) << 1 | (int)(prod_x_m0 >> 15 & 1))) + (uint16_t)((uint16_t)(prod_y_m3 >> 16) << 1 | (int)(prod_y_m3 >> 15 & 1))));
    object_matrix_1 = fx_ldw(FX_OBJECT_MATRIX_1);
    prod_x_m1 = (uint32_t)((int32_t)(int16_t)vec_x * (int16_t)object_matrix_1);
    object_matrix_4 = fx_ldw(FX_OBJECT_MATRIX_4);
    prod_y_m4 = (uint32_t)((int32_t)(int16_t)vec_y * (int16_t)object_matrix_4);
    object_matrix_7 = fx_ldw(FX_OBJECT_MATRIX_7);
    prod_z_m7 = (uint32_t)((int32_t)(int16_t)vec_z * (int16_t)object_matrix_7);
    fx_stw(0x0028, (uint16_t)((uint16_t)((uint16_t)((uint16_t)(prod_z_m7 >> 16) << 1 | (int)(prod_z_m7 >> 15 & 1)) + (uint16_t)((uint16_t)(prod_x_m1 >> 16) << 1 | (int)(prod_x_m1 >> 15 & 1))) + (uint16_t)((uint16_t)(prod_y_m4 >> 16) << 1 | (int)(prod_y_m4 >> 15 & 1))));
    object_matrix_2 = fx_ldw(FX_OBJECT_MATRIX_2);
    prod_x_m2 = (uint32_t)((int32_t)(int16_t)vec_x * (int16_t)object_matrix_2);
    object_matrix_5 = fx_ldw(FX_OBJECT_MATRIX_5);
    prod_y_m5 = (uint32_t)((int32_t)(int16_t)vec_y * (int16_t)object_matrix_5);
    object_matrix_8 = fx_ldw(FX_OBJECT_MATRIX_8);
    prod_z_m8 = (uint32_t)((int32_t)(int16_t)vec_z * (int16_t)object_matrix_8);
    fx_stw(0x002A, (uint16_t)((uint16_t)((uint16_t)((uint16_t)(prod_z_m8 >> 16) << 1 | (int)(prod_z_m8 >> 15 & 1)) + (uint16_t)((uint16_t)(prod_x_m2 >> 16) << 1 | (int)(prod_x_m2 >> 15 & 1))) + (uint16_t)((uint16_t)(prod_y_m5 >> 16) << 1 | (int)(prod_y_m5 >> 15 & 1))));
    return;
}


/*
 * @routine 01:82CE FxMatrixRotateProgram
 * WHAT IT DOES: A program the S-CPU starts: multiplies the current matrix
 * by the column vector ($0068, $002C, $002E) and stores the result at
 * $0026, $0028, $002A.
 * HOW: CACHE; loads the vector into R1, R2, R3 with LMS; for each row, three
 * FMULTs by the row's elements, each doubled by ROL, summed in R5 then R0
 * and stored with SMS. Then STOP.
 * Entry: started by the S-CPU. Exit: (at the STOP) R1, R2, R3 = the
 * vector; R0 = the last result, R5 = its first two terms, R6 = the
 * matrix's last element; the last RAM address $002A; OV, S, CY, Z from the
 * last ADD R5.
 * Returns the last result; *partial and *m22 = R5 and R6 as left.
 */
FxMatrixRotateProgramResult FxMatrixRotateProgram(void)
{
    uint16_t r_t_19;
    uint16_t r_t_22;
    uint32_t x_by_m0;
    uint32_t x_by_m3;
    uint32_t x_by_m6;
    uint32_t y_by_m1;
    uint32_t y_by_m4;
    uint32_t y_by_m7;
    uint32_t z_by_m2;
    uint32_t z_by_m5;
    uint32_t z_by_m8;
    uint16_t matrix_0;
    uint16_t vector_x;
    uint16_t vector_y;
    uint16_t vector_z;
    uint16_t object_matrix_1;
    uint16_t object_matrix_2;
    uint16_t object_matrix_3;
    uint16_t object_matrix_4;
    uint16_t object_matrix_5;
    uint16_t object_matrix_6;
    uint16_t object_matrix_7;
    uint16_t object_matrix_8;

    vector_x = fx_ldw(0x0068);
    vector_y = fx_ldw(0x002C);
    vector_z = fx_ldw(0x002E);
    matrix_0 = fx_ldw(0x00E4);
    x_by_m0 = (uint32_t)((int32_t)(int16_t)vector_x * (int16_t)matrix_0);
    object_matrix_1 = fx_ldw(FX_OBJECT_MATRIX_1);
    y_by_m1 = (uint32_t)((int32_t)(int16_t)vector_y * (int16_t)object_matrix_1);
    object_matrix_2 = fx_ldw(FX_OBJECT_MATRIX_2);
    z_by_m2 = (uint32_t)((int32_t)(int16_t)vector_z * (int16_t)object_matrix_2);
    fx_stw(0x0026, (uint16_t)((uint16_t)((uint16_t)((uint16_t)(z_by_m2 >> 16) << 1 | (int)(z_by_m2 >> 15 & 1)) + (uint16_t)((uint16_t)(x_by_m0 >> 16) << 1 | (int)(x_by_m0 >> 15 & 1))) + (uint16_t)((uint16_t)(y_by_m1 >> 16) << 1 | (int)(y_by_m1 >> 15 & 1))));
    object_matrix_3 = fx_ldw(FX_OBJECT_MATRIX_3);
    x_by_m3 = (uint32_t)((int32_t)(int16_t)vector_x * (int16_t)object_matrix_3);
    object_matrix_4 = fx_ldw(FX_OBJECT_MATRIX_4);
    y_by_m4 = (uint32_t)((int32_t)(int16_t)vector_y * (int16_t)object_matrix_4);
    object_matrix_5 = fx_ldw(FX_OBJECT_MATRIX_5);
    z_by_m5 = (uint32_t)((int32_t)(int16_t)vector_z * (int16_t)object_matrix_5);
    fx_stw(0x0028, (uint16_t)((uint16_t)((uint16_t)((uint16_t)(z_by_m5 >> 16) << 1 | (int)(z_by_m5 >> 15 & 1)) + (uint16_t)((uint16_t)(x_by_m3 >> 16) << 1 | (int)(x_by_m3 >> 15 & 1))) + (uint16_t)((uint16_t)(y_by_m4 >> 16) << 1 | (int)(y_by_m4 >> 15 & 1))));
    object_matrix_6 = fx_ldw(FX_OBJECT_MATRIX_6);
    x_by_m6 = (uint32_t)((int32_t)(int16_t)vector_x * (int16_t)object_matrix_6);
    object_matrix_7 = fx_ldw(FX_OBJECT_MATRIX_7);
    y_by_m7 = (uint32_t)((int32_t)(int16_t)vector_y * (int16_t)object_matrix_7);
    r_t_19 = (uint16_t)((uint16_t)((uint16_t)(x_by_m6 >> 16) << 1 | (int)(x_by_m6 >> 15 & 1)) + (uint16_t)((uint16_t)(y_by_m7 >> 16) << 1 | (int)(y_by_m7 >> 15 & 1)));
    object_matrix_8 = fx_ldw(FX_OBJECT_MATRIX_8);
    z_by_m8 = (uint32_t)((int32_t)(int16_t)vector_z * (int16_t)object_matrix_8);
    r_t_22 = (uint16_t)((uint16_t)((uint16_t)(z_by_m8 >> 16) << 1 | (int)(z_by_m8 >> 15 & 1)) + r_t_19);
    fx_stw(0x002A, r_t_22);
    return (FxMatrixRotateProgramResult){r_t_22, vector_x, vector_y, vector_z, r_t_19, object_matrix_8};
}


/*
 * @routine 01:8325 FxMatrixFromAnglesProgram
 * WHAT IT DOES: The Super FX program that builds the rotation matrix for the
 * angles at $0020-$0024 into $00E4 (FxMatrixFromAngles01).
 * HOW: R9 = $00E4, the call, then STOP.
 * Entry: started by the S-CPU. Exit: (at the STOP) as FxMatrixFromAngles01,
 * R9 = $00F4, R11 = $832D.
 */
FxMatrixFromAnglesProgramResult FxMatrixFromAnglesProgram(void)
{
    uint16_t where_store_out;

    fx_face_view_built(fx_ldw(0x0020), fx_ldw(0x0022), fx_ldw(0x0024));     /* the angles the view's matrix is made of (fx_face.c) */
    { FxMatrixFromAngles01Result r_ = FxMatrixFromAngles01(0x00E4); where_store_out = r_.where_store_out; };
    return (FxMatrixFromAnglesProgramResult){where_store_out, 0x832D};
}


/*
 * @routine 01:832F FxMatrixFromAngles01
 * WHAT IT DOES: Builds the rotation matrix for the three angles at $0020
 * (Z), $0022 (X) and $0024 (Y) and stores its nine words at R9.
 * HOW: Sets the RAM and ROM banks to 0. Each sine comes from the table at
 * $00:983E: the angle's high byte picks two neighbouring words (GETB,
 * GETBH), and the low byte << 7 times their difference (FMULT, ROL) is
 * added to the first; each cosine is the sine of the angle + $4000. With
 * products (a * b) >> 15 (FMULT, ROL) the rows are: sx sy sz + cx cy,
 * sx cy sz - cx sy, sx cz; sy cz, cy cz, -sz; cx sy sz - sx cy,
 * cx cy sz + sx sy, cx cz.
 * Entry: R9 = where to store; R11 = return address. Exit: R9 = R9 + 16;
 * R12, R8 = sin and cos of Z; R4, R3 = of X; R7, R5 = of Y; R1 = sx cy,
 * R2 = cx cy, R13 = sx sy, R14 = cx sy; R6 = cos Z; R0 = the last element;
 * the RAM and ROM banks 0; the last RAM address the last element's; CY,
 * S, Z from its ROL; OV from the ADD R13 of the element before.
 * R14 and the ROM buffer move as on the Super FX.
 */
FxMatrixFromAngles01Result FxMatrixFromAngles01(uint16_t where_store)
{
    uint16_t cos_x;
    uint16_t cos_y;
    uint16_t cos_z;
    uint16_t sin_x;
    uint16_t sin_y;
    uint16_t sin_z;
    uint16_t r_t_89;
    uint16_t angle_x;
    uint16_t angle_y;
    uint16_t angle_z;
    uint16_t cosx_cosy;
    uint16_t cosx_siny;
    uint16_t sinx_cosy;
    uint16_t sinx_siny;
    uint16_t cos_x_index;
    uint16_t cos_y_index;
    uint16_t cos_z_index;
    uint16_t sin_x_index;
    uint16_t sin_y_index;
    uint16_t sin_z_index;
    uint16_t x_cos_phase;
    uint16_t y_cos_phase;
    uint16_t z_cos_phase;
    uint16_t matrix_ptr_1;
    uint16_t matrix_ptr_2;
    uint16_t matrix_ptr_3;
    uint16_t matrix_ptr_4;
    uint16_t matrix_ptr_5;
    uint16_t matrix_ptr_6;
    uint16_t matrix_ptr_7;
    uint16_t cos_x_low_byte;
    uint16_t cos_x_next_ptr;
    uint16_t cos_y_low_byte;
    uint16_t cos_y_next_ptr;
    uint16_t cos_z_low_byte;
    uint16_t cos_z_next_ptr;
    uint32_t cosx_cosy_prod;
    uint32_t cosx_cosz_prod;
    uint32_t cosx_siny_prod;
    uint32_t cosy_cosz_prod;
    uint16_t sin_x_low_byte;
    uint16_t sin_x_next_ptr;
    uint16_t sin_y_low_byte;
    uint16_t sin_y_next_ptr;
    uint16_t sin_z_low_byte;
    uint16_t sin_z_next_ptr;
    uint32_t sinx_cosy_prod;
    uint32_t sinx_cosz_prod;
    uint32_t sinx_siny_prod;
    uint32_t siny_cosz_prod;
    uint16_t cos_x_entry_ptr;
    uint16_t cos_x_upper_low;
    uint16_t cos_x_upper_ptr;
    uint16_t cos_y_entry_ptr;
    uint16_t cos_y_upper_low;
    uint16_t cos_y_upper_ptr;
    uint16_t cos_z_entry_ptr;
    uint16_t cos_z_upper_low;
    uint16_t cos_z_upper_ptr;
    uint16_t sin_x_entry_ptr;
    uint16_t sin_x_upper_low;
    uint16_t sin_x_upper_ptr;
    uint16_t sin_y_entry_ptr;
    uint16_t sin_y_upper_low;
    uint16_t sin_y_upper_ptr;
    uint16_t sin_z_entry_ptr;
    uint16_t sin_z_upper_low;
    uint16_t sin_z_upper_ptr;
    uint32_t sinxy_sinz_prod;
    uint32_t cos_x_interp_prod;
    uint16_t cos_x_lower_value;
    uint16_t cos_x_upper_value;
    uint32_t cos_y_interp_prod;
    uint16_t cos_y_lower_value;
    uint16_t cos_y_upper_value;
    uint32_t cos_z_interp_prod;
    uint16_t cos_z_lower_value;
    uint16_t cos_z_upper_value;
    uint32_t sin_x_interp_prod;
    uint16_t sin_x_lower_value;
    uint16_t sin_x_upper_value;
    uint32_t sin_y_interp_prod;
    uint16_t sin_y_lower_value;
    uint16_t sin_y_upper_value;
    uint32_t sin_z_interp_prod;
    uint16_t sin_z_lower_value;
    uint16_t sin_z_upper_value;
    uint32_t cosxcosy_sinz_prod;
    uint32_t cosxsiny_sinz_prod;
    uint32_t sinxcosy_sinz_prod;

    fx_ramb(0);
    fx_romb(0);
    angle_z = fx_ldw(0x0020);
    sin_z_index = (uint16_t)(angle_z >> 8);
    sin_z_entry_ptr = (uint16_t)((uint16_t)(sin_z_index + sin_z_index) + FX_SINE_TABLE);
    fx_r14(sin_z_entry_ptr);
    sin_z_low_byte = fx_getb();
    sin_z_next_ptr = (uint16_t)(sin_z_entry_ptr + 1);
    fx_r14(sin_z_next_ptr);
    sin_z_lower_value = (uint16_t)((sin_z_low_byte & 0xFF) | fx_getb() << 8);
    sin_z_upper_ptr = (uint16_t)(sin_z_next_ptr + 1);
    fx_r14(sin_z_upper_ptr);
    sin_z_upper_low = fx_getb();
    fx_r14((uint16_t)(sin_z_upper_ptr + 1));
    sin_z_upper_value = (uint16_t)((sin_z_upper_low & 0xFF) | fx_getb() << 8);
    sin_z_interp_prod = (uint32_t)((int32_t)(int16_t)(uint16_t)((uint16_t)((uint16_t)(angle_z & 0xFF) << 8 | (uint16_t)(angle_z & 0xFF) >> 8) >> 1) * (int16_t)(uint16_t)(sin_z_upper_value - sin_z_lower_value));
    sin_z = (uint16_t)((uint16_t)((uint16_t)(sin_z_interp_prod >> 16) << 1 | (int)(sin_z_interp_prod >> 15 & 1)) + sin_z_lower_value);
    fx_r14(0x4000);
    z_cos_phase = (uint16_t)(angle_z + 0x4000);
    cos_z_index = (uint16_t)(z_cos_phase >> 8);
    cos_z_entry_ptr = (uint16_t)((uint16_t)(cos_z_index + cos_z_index) + FX_SINE_TABLE);
    fx_r14(cos_z_entry_ptr);
    cos_z_low_byte = fx_getb();
    cos_z_next_ptr = (uint16_t)(cos_z_entry_ptr + 1);
    fx_r14(cos_z_next_ptr);
    cos_z_lower_value = (uint16_t)((cos_z_low_byte & 0xFF) | fx_getb() << 8);
    cos_z_upper_ptr = (uint16_t)(cos_z_next_ptr + 1);
    fx_r14(cos_z_upper_ptr);
    cos_z_upper_low = fx_getb();
    fx_r14((uint16_t)(cos_z_upper_ptr + 1));
    cos_z_upper_value = (uint16_t)((cos_z_upper_low & 0xFF) | fx_getb() << 8);
    cos_z_interp_prod = (uint32_t)((int32_t)(int16_t)(uint16_t)((uint16_t)((uint16_t)(z_cos_phase & 0xFF) << 8 | (uint16_t)(z_cos_phase & 0xFF) >> 8) >> 1) * (int16_t)(uint16_t)(cos_z_upper_value - cos_z_lower_value));
    cos_z = (uint16_t)((uint16_t)((uint16_t)(cos_z_interp_prod >> 16) << 1 | (int)(cos_z_interp_prod >> 15 & 1)) + cos_z_lower_value);
    angle_x = fx_ldw(0x0022);
    sin_x_index = (uint16_t)(angle_x >> 8);
    sin_x_entry_ptr = (uint16_t)((uint16_t)(sin_x_index + sin_x_index) + FX_SINE_TABLE);
    fx_r14(sin_x_entry_ptr);
    sin_x_low_byte = fx_getb();
    sin_x_next_ptr = (uint16_t)(sin_x_entry_ptr + 1);
    fx_r14(sin_x_next_ptr);
    sin_x_lower_value = (uint16_t)((sin_x_low_byte & 0xFF) | fx_getb() << 8);
    sin_x_upper_ptr = (uint16_t)(sin_x_next_ptr + 1);
    fx_r14(sin_x_upper_ptr);
    sin_x_upper_low = fx_getb();
    fx_r14((uint16_t)(sin_x_upper_ptr + 1));
    sin_x_upper_value = (uint16_t)((sin_x_upper_low & 0xFF) | fx_getb() << 8);
    sin_x_interp_prod = (uint32_t)((int32_t)(int16_t)(uint16_t)((uint16_t)((uint16_t)(angle_x & 0xFF) << 8 | (uint16_t)(angle_x & 0xFF) >> 8) >> 1) * (int16_t)(uint16_t)(sin_x_upper_value - sin_x_lower_value));
    sin_x = (uint16_t)((uint16_t)((uint16_t)(sin_x_interp_prod >> 16) << 1 | (int)(sin_x_interp_prod >> 15 & 1)) + sin_x_lower_value);
    fx_r14(0x4000);
    x_cos_phase = (uint16_t)(angle_x + 0x4000);
    cos_x_index = (uint16_t)(x_cos_phase >> 8);
    cos_x_entry_ptr = (uint16_t)((uint16_t)(cos_x_index + cos_x_index) + FX_SINE_TABLE);
    fx_r14(cos_x_entry_ptr);
    cos_x_low_byte = fx_getb();
    cos_x_next_ptr = (uint16_t)(cos_x_entry_ptr + 1);
    fx_r14(cos_x_next_ptr);
    cos_x_lower_value = (uint16_t)((cos_x_low_byte & 0xFF) | fx_getb() << 8);
    cos_x_upper_ptr = (uint16_t)(cos_x_next_ptr + 1);
    fx_r14(cos_x_upper_ptr);
    cos_x_upper_low = fx_getb();
    fx_r14((uint16_t)(cos_x_upper_ptr + 1));
    cos_x_upper_value = (uint16_t)((cos_x_upper_low & 0xFF) | fx_getb() << 8);
    cos_x_interp_prod = (uint32_t)((int32_t)(int16_t)(uint16_t)((uint16_t)((uint16_t)(x_cos_phase & 0xFF) << 8 | (uint16_t)(x_cos_phase & 0xFF) >> 8) >> 1) * (int16_t)(uint16_t)(cos_x_upper_value - cos_x_lower_value));
    cos_x = (uint16_t)((uint16_t)((uint16_t)(cos_x_interp_prod >> 16) << 1 | (int)(cos_x_interp_prod >> 15 & 1)) + cos_x_lower_value);
    angle_y = fx_ldw(0x0024);
    sin_y_index = (uint16_t)(angle_y >> 8);
    sin_y_entry_ptr = (uint16_t)((uint16_t)(sin_y_index + sin_y_index) + FX_SINE_TABLE);
    fx_r14(sin_y_entry_ptr);
    sin_y_low_byte = fx_getb();
    sin_y_next_ptr = (uint16_t)(sin_y_entry_ptr + 1);
    fx_r14(sin_y_next_ptr);
    sin_y_lower_value = (uint16_t)((sin_y_low_byte & 0xFF) | fx_getb() << 8);
    sin_y_upper_ptr = (uint16_t)(sin_y_next_ptr + 1);
    fx_r14(sin_y_upper_ptr);
    sin_y_upper_low = fx_getb();
    fx_r14((uint16_t)(sin_y_upper_ptr + 1));
    sin_y_upper_value = (uint16_t)((sin_y_upper_low & 0xFF) | fx_getb() << 8);
    sin_y_interp_prod = (uint32_t)((int32_t)(int16_t)(uint16_t)((uint16_t)((uint16_t)(angle_y & 0xFF) << 8 | (uint16_t)(angle_y & 0xFF) >> 8) >> 1) * (int16_t)(uint16_t)(sin_y_upper_value - sin_y_lower_value));
    sin_y = (uint16_t)((uint16_t)((uint16_t)(sin_y_interp_prod >> 16) << 1 | (int)(sin_y_interp_prod >> 15 & 1)) + sin_y_lower_value);
    fx_r14(0x4000);
    y_cos_phase = (uint16_t)(angle_y + 0x4000);
    cos_y_index = (uint16_t)(y_cos_phase >> 8);
    cos_y_entry_ptr = (uint16_t)((uint16_t)(cos_y_index + cos_y_index) + FX_SINE_TABLE);
    fx_r14(cos_y_entry_ptr);
    cos_y_low_byte = fx_getb();
    cos_y_next_ptr = (uint16_t)(cos_y_entry_ptr + 1);
    fx_r14(cos_y_next_ptr);
    cos_y_lower_value = (uint16_t)((cos_y_low_byte & 0xFF) | fx_getb() << 8);
    cos_y_upper_ptr = (uint16_t)(cos_y_next_ptr + 1);
    fx_r14(cos_y_upper_ptr);
    cos_y_upper_low = fx_getb();
    fx_r14((uint16_t)(cos_y_upper_ptr + 1));
    cos_y_upper_value = (uint16_t)((cos_y_upper_low & 0xFF) | fx_getb() << 8);
    cos_y_interp_prod = (uint32_t)((int32_t)(int16_t)(uint16_t)((uint16_t)((uint16_t)(y_cos_phase & 0xFF) << 8 | (uint16_t)(y_cos_phase & 0xFF) >> 8) >> 1) * (int16_t)(uint16_t)(cos_y_upper_value - cos_y_lower_value));
    cos_y = (uint16_t)((uint16_t)((uint16_t)(cos_y_interp_prod >> 16) << 1 | (int)(cos_y_interp_prod >> 15 & 1)) + cos_y_lower_value);
    sinx_cosy_prod = (uint32_t)((int32_t)(int16_t)sin_x * (int16_t)cos_y);
    sinx_cosy = (uint16_t)((uint16_t)(sinx_cosy_prod >> 16) << 1 | (int)(sinx_cosy_prod >> 15 & 1));
    cosx_cosy_prod = (uint32_t)((int32_t)(int16_t)cos_x * (int16_t)cos_y);
    cosx_cosy = (uint16_t)((uint16_t)(cosx_cosy_prod >> 16) << 1 | (int)(cosx_cosy_prod >> 15 & 1));
    sinx_siny_prod = (uint32_t)((int32_t)(int16_t)sin_x * (int16_t)sin_y);
    sinx_siny = (uint16_t)((uint16_t)(sinx_siny_prod >> 16) << 1 | (int)(sinx_siny_prod >> 15 & 1));
    cosx_siny_prod = (uint32_t)((int32_t)(int16_t)cos_x * (int16_t)sin_y);
    cosx_siny = (uint16_t)((uint16_t)(cosx_siny_prod >> 16) << 1 | (int)(cosx_siny_prod >> 15 & 1));
    fx_r14(cosx_siny);
    sinxy_sinz_prod = (uint32_t)((int32_t)(int16_t)sinx_siny * (int16_t)sin_z);
    fx_stw(where_store, (uint16_t)((uint16_t)((uint16_t)(sinxy_sinz_prod >> 16) << 1 | (int)(sinxy_sinz_prod >> 15 & 1)) + cosx_cosy));
    matrix_ptr_1 = (uint16_t)(where_store + 2);
    sinxcosy_sinz_prod = (uint32_t)((int32_t)(int16_t)sinx_cosy * (int16_t)sin_z);
    fx_stw(matrix_ptr_1, (uint16_t)((uint16_t)((uint16_t)(sinxcosy_sinz_prod >> 16) << 1 | (int)(sinxcosy_sinz_prod >> 15 & 1)) - cosx_siny));
    matrix_ptr_2 = (uint16_t)(matrix_ptr_1 + 2);
    sinx_cosz_prod = (uint32_t)((int32_t)(int16_t)sin_x * (int16_t)cos_z);
    fx_stw(matrix_ptr_2, (uint16_t)((uint16_t)(sinx_cosz_prod >> 16) << 1 | (int)(sinx_cosz_prod >> 15 & 1)));
    matrix_ptr_3 = (uint16_t)(matrix_ptr_2 + 2);
    siny_cosz_prod = (uint32_t)((int32_t)(int16_t)sin_y * (int16_t)cos_z);
    fx_stw(matrix_ptr_3, (uint16_t)((uint16_t)(siny_cosz_prod >> 16) << 1 | (int)(siny_cosz_prod >> 15 & 1)));
    matrix_ptr_4 = (uint16_t)(matrix_ptr_3 + 2);
    cosy_cosz_prod = (uint32_t)((int32_t)(int16_t)cos_y * (int16_t)cos_z);
    fx_stw(matrix_ptr_4, (uint16_t)((uint16_t)(cosy_cosz_prod >> 16) << 1 | (int)(cosy_cosz_prod >> 15 & 1)));
    matrix_ptr_5 = (uint16_t)(matrix_ptr_4 + 2);
    fx_stw(matrix_ptr_5, (uint16_t)((uint16_t)~sin_z + 1));
    matrix_ptr_6 = (uint16_t)(matrix_ptr_5 + 2);
    cosxsiny_sinz_prod = (uint32_t)((int32_t)(int16_t)cosx_siny * (int16_t)sin_z);
    fx_stw(matrix_ptr_6, (uint16_t)((uint16_t)((uint16_t)(cosxsiny_sinz_prod >> 16) << 1 | (int)(cosxsiny_sinz_prod >> 15 & 1)) - sinx_cosy));
    matrix_ptr_7 = (uint16_t)(matrix_ptr_6 + 2);
    cosxcosy_sinz_prod = (uint32_t)((int32_t)(int16_t)cosx_cosy * (int16_t)sin_z);
    fx_stw(matrix_ptr_7, (uint16_t)((uint16_t)((uint16_t)(cosxcosy_sinz_prod >> 16) << 1 | (int)(cosxcosy_sinz_prod >> 15 & 1)) + sinx_siny));
    r_t_89 = (uint16_t)(matrix_ptr_7 + 2);
    cosx_cosz_prod = (uint32_t)((int32_t)(int16_t)cos_x * (int16_t)cos_z);
    fx_stw(r_t_89, (uint16_t)((uint16_t)(cosx_cosz_prod >> 16) << 1 | (int)(cosx_cosz_prod >> 15 & 1)));
    return (FxMatrixFromAngles01Result){sin_y, r_t_89};
}


/*
 * @routine 01:981A FxSegmentCrossZ
 * WHAT IT DOES: Finds where the segment from (R1, R2, R3) to (R4, R5, R6)
 * crosses z = 0: R0 = 0 when both ends have z >= 0, $FFFF when both are
 * negative; otherwise R0 = 1 with the end whose z was negative replaced by
 * the crossing point (the other end kept in its registers).
 * HOW: The z >= 0 end is saved at $9A/$9C/$9E (SMS) and put back at the
 * end (LMS); the ends are brought to the midpoint (ADD, DIV2) toward the
 * crossing until the midpoint's z is 0. When the first end is the one
 * with z >= 0 the ends are swapped while searching.
 * Entry: R1-R6 = the ends; R11 = return address. Exit: as above; R1-R6 as
 * the search left them; the last RAM address $9E; S, Z from the last
 * DIV2 (or the end tests).
 */
FxSegmentCrossZResult FxSegmentCrossZ(uint16_t ends_1, uint16_t ends_2, uint16_t ends_3, uint16_t ends_4, uint16_t ends_5, uint16_t ends_6)
{
    uint16_t mid_z;
    uint16_t cut_z_2;
    uint16_t kept_z_2;
    uint16_t clip_kept_x;
    uint16_t clip_kept_y;
    uint16_t clip_kept_z;
    uint16_t clip_kept_x_2;
    uint16_t clip_kept_y_2;
    uint16_t clip_kept_z_2;
    uint16_t mid_z_swapped;
    uint16_t swapped_end1_x;
    uint16_t swapped_end1_y;
    uint16_t swapped_end1_z;
    uint16_t swapped_end2_x;
    uint16_t swapped_end2_y;
    uint16_t swapped_end2_z;

    cut_z_2 = ends_3;
    if (ends_3 & 0x8000) {
        kept_z_2 = ends_6;
        if (!(ends_6 & 0x8000)) {
            fx_stw(FX_CLIP_KEPT_X, ends_4);
            fx_stw(FX_CLIP_KEPT_Y, ends_5);
            fx_stw(FX_CLIP_KEPT_Z, kept_z_2);
            for (;;) {
                mid_z = fx_div2((uint16_t)(cut_z_2 + kept_z_2));
                if (mid_z == 0) {
                    break;
                } else {
                    if (mid_z & 0x8000) {
                        cut_z_2 = mid_z;
                        ends_1 = fx_div2((uint16_t)(ends_1 + ends_4));
                        ends_2 = fx_div2((uint16_t)(ends_2 + ends_5));
                        continue;
                    } else {
                        kept_z_2 = mid_z;
                        ends_4 = fx_div2((uint16_t)(ends_1 + ends_4));
                        ends_5 = fx_div2((uint16_t)(ends_2 + ends_5));
                        continue;
                    }
                }
            }
            clip_kept_x = fx_ldw(FX_CLIP_KEPT_X);
            clip_kept_y = fx_ldw(FX_CLIP_KEPT_Y);
            clip_kept_z = fx_ldw(FX_CLIP_KEPT_Z);
            return (FxSegmentCrossZResult){fx_div2((uint16_t)(ends_1 + ends_4)), fx_div2((uint16_t)(ends_2 + ends_5)), mid_z, clip_kept_x, clip_kept_y, clip_kept_z};
        } else {
            return (FxSegmentCrossZResult){ends_1, ends_2, cut_z_2, ends_4, ends_5, kept_z_2};
        }
    } else {
        if (ends_6 & 0x8000) {
            fx_stw(FX_CLIP_KEPT_X, ends_1);
            fx_stw(FX_CLIP_KEPT_Y, ends_2);
            fx_stw(FX_CLIP_KEPT_Z, cut_z_2);
            swapped_end1_x = ends_4;
            swapped_end2_x = ends_1;
            swapped_end1_y = ends_5;
            swapped_end2_y = ends_2;
            swapped_end1_z = ends_6;
            swapped_end2_z = cut_z_2;
            for (;;) {
                mid_z_swapped = fx_div2((uint16_t)(swapped_end1_z + swapped_end2_z));
                if (mid_z_swapped == 0) {
                    break;
                } else {
                    if (mid_z_swapped & 0x8000) {
                        swapped_end1_z = mid_z_swapped;
                        swapped_end1_x = fx_div2((uint16_t)(swapped_end1_x + swapped_end2_x));
                        swapped_end1_y = fx_div2((uint16_t)(swapped_end1_y + swapped_end2_y));
                        continue;
                    } else {
                        swapped_end2_z = mid_z_swapped;
                        swapped_end2_x = fx_div2((uint16_t)(swapped_end1_x + swapped_end2_x));
                        swapped_end2_y = fx_div2((uint16_t)(swapped_end1_y + swapped_end2_y));
                        continue;
                    }
                }
            }
            clip_kept_x_2 = fx_ldw(FX_CLIP_KEPT_X);
            clip_kept_y_2 = fx_ldw(FX_CLIP_KEPT_Y);
            clip_kept_z_2 = fx_ldw(FX_CLIP_KEPT_Z);
            return (FxSegmentCrossZResult){clip_kept_x_2, clip_kept_y_2, clip_kept_z_2, fx_div2((uint16_t)(swapped_end1_x + swapped_end2_x)), fx_div2((uint16_t)(swapped_end1_y + swapped_end2_y)), mid_z_swapped};
        } else {
            return (FxSegmentCrossZResult){ends_1, ends_2, cut_z_2, ends_4, ends_5, ends_6};
        }
    }
}


/*
 * @routine 01:C4AA FxClipYPlane
 * WHAT IT DOES: Clips the segment from (R1, R2, R3) to (R4, R5, R7)
 * against the plane z = y * R6 * 8 (FMULT, ADC, two ADDs): R0 = 0 when
 * both ends have z at or above it, $FFFF when both are below; otherwise
 * R0 = 1 with the end below replaced by the crossing, found by halving
 * (at most 12 steps), its attributes halved along with it: the first
 * end's at $8A/$8E, the second's at $8C/$90 (the moved end's written
 * back there).
 * HOW: The end kept is saved at $9A-$9E and put back; R8, R9, R12, R13
 * and the return address and R14 are kept on the R10 stack, R10 itself at
 * $A0.
 * Entry: R1-R7 as above, R6 = the slope; R10 = stack; R11 = return
 * address. Exit: as above; R8, R9, R12-R14 restored; S, Z from R10; the
 * last RAM address R10.
 */
FxClipYPlaneResult FxClipYPlane(uint16_t a_x, uint16_t a_y, uint16_t a_z, uint16_t b_x, uint16_t b_y, uint16_t clip_slope, uint16_t b_z, uint16_t *stack, uint16_t rom_at)
{
    uint16_t count;
    uint16_t mid_y;
    uint16_t mid_z;
    uint16_t stack_2;
    uint16_t stack_3;
    uint16_t mid_side;
    uint16_t rom_at_2;
    uint16_t rom_at_3;
    uint16_t rom_at_4;
    uint16_t rom_at_5;
    uint16_t mid_limit;
    uint16_t mid_y_alt;
    uint16_t mid_z_alt;
    uint16_t end1_attr1;
    uint16_t end2_attr2;
    int steps_done;
    uint16_t steps_left;
    uint16_t clip_kept_x;
    uint16_t clip_kept_y;
    uint16_t clip_kept_z;
    uint32_t mid_product;
    uint16_t bisect_far_x;
    uint16_t bisect_far_y;
    uint16_t bisect_far_z;
    uint16_t mid_side_alt;
    uint16_t bisect_near_x;
    uint16_t bisect_near_y;
    uint16_t bisect_near_z;
    uint16_t clip_kept_x_2;
    uint16_t clip_kept_y_2;
    uint16_t clip_kept_z_2;
    uint16_t mid_limit_alt;
    uint16_t slope_limit_a;
    uint16_t slope_limit_b;
    uint16_t end2_attr2_alt;
    uint16_t mid_product_hi;
    int steps_done_alt;
    uint16_t steps_left_alt;
    uint16_t clip_end1_attr2;
    uint16_t clip_end2_attr1;
    uint32_t mid_product_alt;
    uint32_t slope_product_a;
    uint32_t slope_product_b;
    uint16_t clip_end1_attr2_2;
    uint16_t clip_end2_attr1_2;
    uint16_t mid_product_hi_alt;
    uint16_t slope_product_hi_a;
    uint16_t slope_product_hi_b;
    uint16_t mid_product_doubled;
    uint16_t slope_limit_b_recheck;
    uint16_t mid_product_doubled_alt;
    uint32_t slope_product_b_recheck;
    uint16_t slope_product_doubled_a;
    uint16_t slope_product_doubled_b;
    uint16_t slope_product_hi_b_recheck;
    uint16_t slope_product_doubled_b_recheck;

    fx_spill(stack);
    fx_push(stack, rom_at);
    slope_product_a = (uint32_t)((int32_t)(int16_t)a_y * (int16_t)clip_slope);
    slope_product_hi_a = (uint16_t)(slope_product_a >> 16);
    slope_product_doubled_a = (uint16_t)(slope_product_hi_a + slope_product_hi_a + (int)(slope_product_a >> 15 & 1));
    slope_limit_a = (uint16_t)(slope_product_doubled_a + slope_product_doubled_a);
    if ((uint16_t)((uint16_t)(a_z - slope_limit_a) - slope_limit_a) & 0x8000) {
        slope_product_b = (uint32_t)((int32_t)(int16_t)b_y * (int16_t)clip_slope);
        slope_product_hi_b = (uint16_t)(slope_product_b >> 16);
        slope_product_doubled_b = (uint16_t)(slope_product_hi_b + slope_product_hi_b + (int)(slope_product_b >> 15 & 1));
        slope_limit_b = (uint16_t)(slope_product_doubled_b + slope_product_doubled_b);
        if ((uint16_t)((uint16_t)(b_z - slope_limit_b) - slope_limit_b) & 0x8000) {
            rom_at_2 = fx_pull(stack);
            fx_r14(rom_at_2);
            fx_unspill(stack);
            return (FxClipYPlaneResult){a_x, a_y, a_z, b_x, b_y, b_z};
        } else {
            fx_stw(FX_CLIP_KEPT_X, b_x);
            fx_stw(FX_CLIP_KEPT_Y, b_y);
            fx_stw(FX_CLIP_KEPT_Z, b_z);
            fx_spill(stack);
            fx_spill(stack);
            fx_spill(stack);
            fx_spill(stack);
            fx_stw(FX_CLIP_STACK, (*stack));
            stack_2 = 0x000C;
            end1_attr1 = fx_ldw(0x008A);
            clip_end1_attr2 = fx_ldw(FX_CLIP_END1_ATTR2);
            clip_end2_attr1 = fx_ldw(FX_CLIP_END2_ATTR1);
            end2_attr2 = fx_ldw(0x0090);
            for (;;) {
                mid_y = fx_div2((uint16_t)(a_y + b_y));
                fx_r14(mid_y);
                mid_z = fx_div2((uint16_t)(a_z + b_z));
                mid_product = (uint32_t)((int32_t)(int16_t)mid_y * (int16_t)clip_slope);
                mid_product_hi = (uint16_t)(mid_product >> 16);
                mid_product_doubled = (uint16_t)(mid_product_hi + mid_product_hi + (int)(mid_product >> 15 & 1));
                mid_limit = (uint16_t)(mid_product_doubled + mid_product_doubled);
                mid_side = (uint16_t)((uint16_t)(mid_z - mid_limit) - mid_limit);
                if (mid_side == 0) {
                    break;
                } else {
                    steps_left = (uint16_t)(stack_2 - 1);
                    steps_done = (steps_left == 0);
                    stack_2 = steps_left;
                    if (mid_side & 0x8000) {
                        if (steps_done) {
                            break;
                        } else {
                            a_z = mid_z;
                            a_x = fx_div2((uint16_t)(a_x + b_x));
                            a_y = mid_y;
                            end1_attr1 = fx_div2((uint16_t)(end1_attr1 + clip_end2_attr1));
                            clip_end1_attr2 = fx_div2((uint16_t)(clip_end1_attr2 + end2_attr2));
                            continue;
                        }
                    } else {
                        if (steps_done) {
                            break;
                        } else {
                            b_z = mid_z;
                            b_x = fx_div2((uint16_t)(a_x + b_x));
                            b_y = mid_y;
                            clip_end2_attr1 = fx_div2((uint16_t)(end1_attr1 + clip_end2_attr1));
                            end2_attr2 = fx_div2((uint16_t)(clip_end1_attr2 + end2_attr2));
                            continue;
                        }
                    }
                }
            }
            clip_kept_x = fx_ldw(FX_CLIP_KEPT_X);
            clip_kept_y = fx_ldw(FX_CLIP_KEPT_Y);
            clip_kept_z = fx_ldw(FX_CLIP_KEPT_Z);
            fx_stw(0x008A, fx_div2((uint16_t)(end1_attr1 + clip_end2_attr1)));
            fx_stw(FX_CLIP_END1_ATTR2, fx_div2((uint16_t)(clip_end1_attr2 + end2_attr2)));
            (*stack) = fx_ldw(FX_CLIP_STACK);
            fx_unspill(stack);
            fx_unspill(stack);
            fx_unspill(stack);
            fx_unspill(stack);
            rom_at_3 = fx_pull(stack);
            fx_r14(rom_at_3);
            fx_unspill(stack);
            return (FxClipYPlaneResult){fx_div2((uint16_t)(a_x + b_x)), mid_y, mid_z, clip_kept_x, clip_kept_y, clip_kept_z};
        }
    } else {
        slope_product_b_recheck = (uint32_t)((int32_t)(int16_t)b_y * (int16_t)clip_slope);
        slope_product_hi_b_recheck = (uint16_t)(slope_product_b_recheck >> 16);
        slope_product_doubled_b_recheck = (uint16_t)(slope_product_hi_b_recheck + slope_product_hi_b_recheck + (int)(slope_product_b_recheck >> 15 & 1));
        slope_limit_b_recheck = (uint16_t)(slope_product_doubled_b_recheck + slope_product_doubled_b_recheck);
        if ((uint16_t)((uint16_t)(b_z - slope_limit_b_recheck) - slope_limit_b_recheck) & 0x8000) {
            fx_stw(FX_CLIP_KEPT_X, a_x);
            fx_stw(FX_CLIP_KEPT_Y, a_y);
            fx_stw(FX_CLIP_KEPT_Z, a_z);
            fx_spill(stack);
            fx_spill(stack);
            fx_spill(stack);
            fx_spill(stack);
            fx_stw(FX_CLIP_STACK, (*stack));
            stack_3 = 0x000C;
            bisect_near_x = b_x;
            bisect_far_x = a_x;
            bisect_near_y = b_y;
            bisect_far_y = a_y;
            bisect_near_z = b_z;
            bisect_far_z = a_z;
            clip_end2_attr1_2 = fx_ldw(FX_CLIP_END2_ATTR1);
            end2_attr2_alt = fx_ldw(0x0090);
            count = fx_ldw(0x008A);
            clip_end1_attr2_2 = fx_ldw(FX_CLIP_END1_ATTR2);
            for (;;) {
                mid_y_alt = fx_div2((uint16_t)(bisect_near_y + bisect_far_y));
                fx_r14(mid_y_alt);
                mid_z_alt = fx_div2((uint16_t)(bisect_near_z + bisect_far_z));
                mid_product_alt = (uint32_t)((int32_t)(int16_t)mid_y_alt * (int16_t)clip_slope);
                mid_product_hi_alt = (uint16_t)(mid_product_alt >> 16);
                mid_product_doubled_alt = (uint16_t)(mid_product_hi_alt + mid_product_hi_alt + (int)(mid_product_alt >> 15 & 1));
                mid_limit_alt = (uint16_t)(mid_product_doubled_alt + mid_product_doubled_alt);
                mid_side_alt = (uint16_t)((uint16_t)(mid_z_alt - mid_limit_alt) - mid_limit_alt);
                if (mid_side_alt == 0) {
                    break;
                } else {
                    steps_left_alt = (uint16_t)(stack_3 - 1);
                    steps_done_alt = (steps_left_alt == 0);
                    stack_3 = steps_left_alt;
                    if (mid_side_alt & 0x8000) {
                        if (steps_done_alt) {
                            break;
                        } else {
                            bisect_near_z = mid_z_alt;
                            bisect_near_x = fx_div2((uint16_t)(bisect_near_x + bisect_far_x));
                            bisect_near_y = mid_y_alt;
                            clip_end2_attr1_2 = fx_div2((uint16_t)(clip_end2_attr1_2 + count));
                            end2_attr2_alt = fx_div2((uint16_t)(end2_attr2_alt + clip_end1_attr2_2));
                            continue;
                        }
                    } else {
                        if (steps_done_alt) {
                            break;
                        } else {
                            bisect_far_z = mid_z_alt;
                            bisect_far_x = fx_div2((uint16_t)(bisect_near_x + bisect_far_x));
                            bisect_far_y = mid_y_alt;
                            count = fx_div2((uint16_t)(clip_end2_attr1_2 + count));
                            clip_end1_attr2_2 = fx_div2((uint16_t)(end2_attr2_alt + clip_end1_attr2_2));
                            continue;
                        }
                    }
                }
            }
            clip_kept_x_2 = fx_ldw(FX_CLIP_KEPT_X);
            clip_kept_y_2 = fx_ldw(FX_CLIP_KEPT_Y);
            clip_kept_z_2 = fx_ldw(FX_CLIP_KEPT_Z);
            fx_stw(FX_CLIP_END2_ATTR1, fx_div2((uint16_t)(clip_end2_attr1_2 + count)));
            fx_stw(0x0090, fx_div2((uint16_t)(end2_attr2_alt + clip_end1_attr2_2)));
            (*stack) = fx_ldw(FX_CLIP_STACK);
            fx_unspill(stack);
            fx_unspill(stack);
            fx_unspill(stack);
            fx_unspill(stack);
            rom_at_4 = fx_pull(stack);
            fx_r14(rom_at_4);
            fx_unspill(stack);
            return (FxClipYPlaneResult){clip_kept_x_2, clip_kept_y_2, clip_kept_z_2, fx_div2((uint16_t)(bisect_near_x + bisect_far_x)), mid_y_alt, mid_z_alt};
        } else {
            rom_at_5 = fx_pull(stack);
            fx_r14(rom_at_5);
            fx_unspill(stack);
            return (FxClipYPlaneResult){a_x, a_y, a_z, b_x, b_y, b_z};
        }
    }
}


/*
 * @routine 01:C77A FxClipXPlane
 * WHAT IT DOES: As FxClipYPlane against the plane z = x * R6 * 4 (FMULT,
 * ADC, one ADD), halving on x.
 * HOW: As FxClipYPlane.
 * Entry: as FxClipYPlane. Exit: as FxClipYPlane.
 */
FxClipXPlaneResult FxClipXPlane(uint16_t end1_x, uint16_t end1_y, uint16_t end1_z, uint16_t end2_x, uint16_t end2_y, uint16_t plane_slope, uint16_t end2_z, uint16_t *stack, uint16_t rom_at)
{
    uint16_t count;
    uint16_t mid_x;
    uint16_t mid_z;
    uint16_t stack_2;
    uint16_t stack_3;
    uint16_t mid_side;
    uint16_t rom_at_2;
    uint16_t rom_at_3;
    uint16_t rom_at_4;
    uint16_t rom_at_5;
    uint16_t end1_attr1;
    uint16_t end2_attr2;
    uint16_t steps_left;
    uint16_t clip_kept_x;
    uint16_t clip_kept_y;
    uint16_t clip_kept_z;
    uint16_t mid_plane_x;
    uint16_t end1_plane_x;
    uint16_t mid_slope_hi;
    uint16_t clip_kept_x_2;
    uint16_t clip_kept_y_2;
    uint16_t clip_kept_z_2;
    uint16_t end1_slope_hi;
    uint16_t mid_x_swapped;
    uint16_t mid_z_swapped;
    uint16_t end2_plane_x_a;
    uint16_t end2_plane_x_b;
    uint32_t mid_slope_prod;
    uint16_t swapped_end1_x;
    uint16_t swapped_end1_y;
    uint16_t swapped_end1_z;
    uint16_t swapped_end2_x;
    uint16_t swapped_end2_y;
    uint16_t swapped_end2_z;
    uint16_t clip_end1_attr2;
    uint16_t clip_end2_attr1;
    uint32_t end1_slope_prod;
    uint16_t end2_slope_hi_a;
    uint16_t end2_slope_hi_b;
    int steps_exhausted;
    uint16_t end1_attr1_start;
    uint16_t mid_side_swapped;
    uint16_t clip_end1_attr2_2;
    uint16_t clip_end2_attr1_2;
    uint32_t end2_slope_prod_a;
    uint32_t end2_slope_prod_b;
    uint16_t steps_left_swapped;
    uint16_t mid_plane_x_swapped;
    uint16_t mid_slope_hi_swapped;
    uint32_t mid_slope_prod_swapped;
    int steps_exhausted_swapped;

    fx_spill(stack);
    fx_push(stack, rom_at);
    end1_slope_prod = (uint32_t)((int32_t)(int16_t)end1_x * (int16_t)plane_slope);
    end1_slope_hi = (uint16_t)(end1_slope_prod >> 16);
    end1_plane_x = (uint16_t)(end1_slope_hi + end1_slope_hi + (int)(end1_slope_prod >> 15 & 1));
    if ((uint16_t)((uint16_t)(end1_z - end1_plane_x) - end1_plane_x) & 0x8000) {
        end2_slope_prod_b = (uint32_t)((int32_t)(int16_t)end2_x * (int16_t)plane_slope);
        end2_slope_hi_b = (uint16_t)(end2_slope_prod_b >> 16);
        end2_plane_x_b = (uint16_t)(end2_slope_hi_b + end2_slope_hi_b + (int)(end2_slope_prod_b >> 15 & 1));
        if ((uint16_t)((uint16_t)(end2_z - end2_plane_x_b) - end2_plane_x_b) & 0x8000) {
            rom_at_2 = fx_pull(stack);
            fx_r14(rom_at_2);
            fx_unspill(stack);
            return (FxClipXPlaneResult){end1_x, end1_y, end1_z, end2_x, end2_y, end2_z};
        } else {
            fx_stw(FX_CLIP_KEPT_X, end2_x);
            fx_stw(FX_CLIP_KEPT_Y, end2_y);
            fx_stw(FX_CLIP_KEPT_Z, end2_z);
            fx_spill(stack);
            fx_spill(stack);
            fx_spill(stack);
            fx_spill(stack);
            fx_stw(FX_CLIP_STACK, (*stack));
            stack_2 = 0x000C;
            end1_attr1_start = fx_ldw(0x008A);
            clip_end1_attr2 = fx_ldw(FX_CLIP_END1_ATTR2);
            clip_end2_attr1 = fx_ldw(FX_CLIP_END2_ATTR1);
            end2_attr2 = fx_ldw(0x0090);
            for (;;) {
                mid_x = fx_div2((uint16_t)(end1_x + end2_x));
                fx_r14(mid_x);
                mid_z = fx_div2((uint16_t)(end1_z + end2_z));
                mid_slope_prod = (uint32_t)((int32_t)(int16_t)mid_x * (int16_t)plane_slope);
                mid_slope_hi = (uint16_t)(mid_slope_prod >> 16);
                mid_plane_x = (uint16_t)(mid_slope_hi + mid_slope_hi + (int)(mid_slope_prod >> 15 & 1));
                mid_side = (uint16_t)((uint16_t)(mid_z - mid_plane_x) - mid_plane_x);
                if (mid_side == 0) {
                    break;
                } else {
                    steps_left = (uint16_t)(stack_2 - 1);
                    steps_exhausted = (steps_left == 0);
                    stack_2 = steps_left;
                    if (mid_side & 0x8000) {
                        if (steps_exhausted) {
                            break;
                        } else {
                            end1_z = mid_z;
                            end1_y = fx_div2((uint16_t)(end1_y + end2_y));
                            end1_x = mid_x;
                            end1_attr1_start = fx_div2((uint16_t)(end1_attr1_start + clip_end2_attr1));
                            clip_end1_attr2 = fx_div2((uint16_t)(clip_end1_attr2 + end2_attr2));
                            continue;
                        }
                    } else {
                        if (steps_exhausted) {
                            break;
                        } else {
                            end2_z = mid_z;
                            end2_y = fx_div2((uint16_t)(end1_y + end2_y));
                            end2_x = mid_x;
                            clip_end2_attr1 = fx_div2((uint16_t)(end1_attr1_start + clip_end2_attr1));
                            end2_attr2 = fx_div2((uint16_t)(clip_end1_attr2 + end2_attr2));
                            continue;
                        }
                    }
                }
            }
            clip_kept_x = fx_ldw(FX_CLIP_KEPT_X);
            clip_kept_y = fx_ldw(FX_CLIP_KEPT_Y);
            clip_kept_z = fx_ldw(FX_CLIP_KEPT_Z);
            fx_stw(0x008A, fx_div2((uint16_t)(end1_attr1_start + clip_end2_attr1)));
            fx_stw(FX_CLIP_END1_ATTR2, fx_div2((uint16_t)(clip_end1_attr2 + end2_attr2)));
            (*stack) = fx_ldw(FX_CLIP_STACK);
            fx_unspill(stack);
            fx_unspill(stack);
            fx_unspill(stack);
            fx_unspill(stack);
            rom_at_3 = fx_pull(stack);
            fx_r14(rom_at_3);
            fx_unspill(stack);
            return (FxClipXPlaneResult){mid_x, fx_div2((uint16_t)(end1_y + end2_y)), mid_z, clip_kept_x, clip_kept_y, clip_kept_z};
        }
    } else {
        end2_slope_prod_a = (uint32_t)((int32_t)(int16_t)end2_x * (int16_t)plane_slope);
        end2_slope_hi_a = (uint16_t)(end2_slope_prod_a >> 16);
        end2_plane_x_a = (uint16_t)(end2_slope_hi_a + end2_slope_hi_a + (int)(end2_slope_prod_a >> 15 & 1));
        if ((uint16_t)((uint16_t)(end2_z - end2_plane_x_a) - end2_plane_x_a) & 0x8000) {
            fx_stw(FX_CLIP_KEPT_X, end1_x);
            fx_stw(FX_CLIP_KEPT_Y, end1_y);
            fx_stw(FX_CLIP_KEPT_Z, end1_z);
            fx_spill(stack);
            fx_spill(stack);
            fx_spill(stack);
            fx_spill(stack);
            fx_stw(FX_CLIP_STACK, (*stack));
            stack_3 = 0x000C;
            swapped_end1_x = end2_x;
            swapped_end2_x = end1_x;
            swapped_end1_y = end2_y;
            swapped_end2_y = end1_y;
            swapped_end1_z = end2_z;
            swapped_end2_z = end1_z;
            clip_end2_attr1_2 = fx_ldw(FX_CLIP_END2_ATTR1);
            end1_attr1 = fx_ldw(0x0090);
            count = fx_ldw(0x008A);
            clip_end1_attr2_2 = fx_ldw(FX_CLIP_END1_ATTR2);
            for (;;) {
                mid_x_swapped = fx_div2((uint16_t)(swapped_end1_x + swapped_end2_x));
                fx_r14(mid_x_swapped);
                mid_z_swapped = fx_div2((uint16_t)(swapped_end1_z + swapped_end2_z));
                mid_slope_prod_swapped = (uint32_t)((int32_t)(int16_t)mid_x_swapped * (int16_t)plane_slope);
                mid_slope_hi_swapped = (uint16_t)(mid_slope_prod_swapped >> 16);
                mid_plane_x_swapped = (uint16_t)(mid_slope_hi_swapped + mid_slope_hi_swapped + (int)(mid_slope_prod_swapped >> 15 & 1));
                mid_side_swapped = (uint16_t)((uint16_t)(mid_z_swapped - mid_plane_x_swapped) - mid_plane_x_swapped);
                if (mid_side_swapped == 0) {
                    break;
                } else {
                    steps_left_swapped = (uint16_t)(stack_3 - 1);
                    steps_exhausted_swapped = (steps_left_swapped == 0);
                    stack_3 = steps_left_swapped;
                    if (mid_side_swapped & 0x8000) {
                        if (steps_exhausted_swapped) {
                            break;
                        } else {
                            swapped_end1_z = mid_z_swapped;
                            swapped_end1_y = fx_div2((uint16_t)(swapped_end1_y + swapped_end2_y));
                            swapped_end1_x = mid_x_swapped;
                            clip_end2_attr1_2 = fx_div2((uint16_t)(clip_end2_attr1_2 + count));
                            end1_attr1 = fx_div2((uint16_t)(end1_attr1 + clip_end1_attr2_2));
                            continue;
                        }
                    } else {
                        if (steps_exhausted_swapped) {
                            break;
                        } else {
                            swapped_end2_z = mid_z_swapped;
                            swapped_end2_y = fx_div2((uint16_t)(swapped_end1_y + swapped_end2_y));
                            swapped_end2_x = mid_x_swapped;
                            count = fx_div2((uint16_t)(clip_end2_attr1_2 + count));
                            clip_end1_attr2_2 = fx_div2((uint16_t)(end1_attr1 + clip_end1_attr2_2));
                            continue;
                        }
                    }
                }
            }
            clip_kept_x_2 = fx_ldw(FX_CLIP_KEPT_X);
            clip_kept_y_2 = fx_ldw(FX_CLIP_KEPT_Y);
            clip_kept_z_2 = fx_ldw(FX_CLIP_KEPT_Z);
            fx_stw(FX_CLIP_END2_ATTR1, fx_div2((uint16_t)(clip_end2_attr1_2 + count)));
            fx_stw(0x0090, fx_div2((uint16_t)(end1_attr1 + clip_end1_attr2_2)));
            (*stack) = fx_ldw(FX_CLIP_STACK);
            fx_unspill(stack);
            fx_unspill(stack);
            fx_unspill(stack);
            fx_unspill(stack);
            rom_at_4 = fx_pull(stack);
            fx_r14(rom_at_4);
            fx_unspill(stack);
            return (FxClipXPlaneResult){clip_kept_x_2, clip_kept_y_2, clip_kept_z_2, mid_x_swapped, fx_div2((uint16_t)(swapped_end1_y + swapped_end2_y)), mid_z_swapped};
        } else {
            rom_at_5 = fx_pull(stack);
            fx_r14(rom_at_5);
            fx_unspill(stack);
            return (FxClipXPlaneResult){end1_x, end1_y, end1_z, end2_x, end2_y, end2_z};
        }
    }
}


/*
 * @routine 01:C668 FxPolygonClipTop
 * WHAT IT DOES: Clips the polygon at $60 ($40 vertices of five words:
 * x, y, z and two attributes) against the plane z = y * slope ($94) * 8
 * into the list at $62 (FxClipYPlane for each crossing edge): R0 = the
 * new vertex count ($40), or 0 when fewer than 3 are left or there were 2.
 * HOW: Each edge from a vertex to the next (the last to the first); a
 * vertex inside is kept, a crossing adds the clipped point. R14 and the
 * return address on the R10 stack.
 * Entry: R10 = stack; R11 = return address. Exit: R0 as above; R9 past
 * the output; R14 restored; S, Z from R10; the last RAM address R10.
 */
FxPolygonClipTopResult FxPolygonClipTop(uint16_t next_vertex_z, uint16_t *stack, uint16_t rom_at)
{
    uint16_t slope;
    uint16_t rom_at_2;
    uint16_t rom_at_3;
    uint16_t rom_at_4;
    uint16_t vertex_x;
    uint16_t vertex_y;
    uint16_t vertex_z;
    uint16_t out_y_ptr;
    uint16_t out_z_ptr;
    uint16_t clip_attr2;
    uint16_t edges_left;
    uint16_t next_y_ptr;
    uint16_t next_z_ptr;
    uint16_t slope_y_x2;
    uint16_t slope_y_x4;
    uint16_t clipped_a_x;
    uint16_t clipped_a_y;
    uint16_t clipped_a_z;
    uint16_t clipped_b_x;
    uint16_t clipped_b_y;
    uint16_t saved_attr1;
    uint32_t slope_prod_y;
    uint16_t vertex_attr1;
    uint16_t vertex_y_ptr;
    uint16_t vertex_z_ptr;
    uint16_t in_vertex_ptr;
    uint16_t next_vertex_x;
    uint16_t next_vertex_y;
    uint16_t out_attr1_ptr;
    uint16_t out_attr2_ptr;
    uint16_t slope_prod_hi;
    uint16_t clip_out_y_ptr;
    uint16_t clip_out_z_ptr;
    uint16_t crossing_attr1;
    uint16_t crossing_y_ptr;
    uint16_t crossing_z_ptr;
    uint16_t next_attr1_ptr;
    uint16_t next_attr2_ptr;
    uint16_t out_vertex_ptr;
    uint16_t clip_end1_attr2;
    uint16_t clip_end2_attr1;
    uint16_t next_vertex_ptr;
    uint16_t slope_next_y_x2;
    uint16_t slope_next_y_x4;
    uint16_t vertex_attr1_ptr;
    uint16_t vertex_attr2_ptr;
    uint16_t clip_end1_attr2_2;
    uint16_t clip_end1_attr2_3;
    uint16_t clip_end2_attr1_2;
    uint16_t next_vertex_attr2;
    uint32_t slope_prod_next_y;
    uint16_t clip_out_attr1_ptr;
    uint16_t clip_out_attr2_ptr;
    uint16_t crossing_attr1_ptr;
    uint16_t crossing_attr2_ptr;
    uint16_t final_vertex_count;
    uint16_t slope_prod_next_hi;
    uint16_t out_count_before_clip;
    uint16_t out_count_before_kept;
    uint16_t slope_next_y_x2_outside;
    uint16_t slope_next_y_x4_outside;
    uint16_t out_count_before_crossing;
    uint32_t slope_prod_next_y_outside;
    uint16_t slope_prod_next_hi_outside;

    fx_spill(stack);
    fx_push(stack, rom_at);
    slope = fx_ldw(0x0094);
    rom_at_2 = fx_ldw(0x0040);
    fx_r14(rom_at_2);
    fx_stw(0x0040, 0);
    in_vertex_ptr = fx_ldw(0x0060);
    out_vertex_ptr = fx_ldw(0x0062);
    if (rom_at_2 != 2) {
        for (;;) {
            vertex_x = fx_ldw(in_vertex_ptr);
            vertex_y_ptr = (uint16_t)(in_vertex_ptr + 2);
            vertex_y = fx_ldw(vertex_y_ptr);
            vertex_z_ptr = (uint16_t)(vertex_y_ptr + 2);
            vertex_z = fx_ldw(vertex_z_ptr);
            vertex_attr1_ptr = (uint16_t)(vertex_z_ptr + 2);
            vertex_attr1 = fx_ldw(vertex_attr1_ptr);
            vertex_attr2_ptr = (uint16_t)(vertex_attr1_ptr + 2);
            fx_stw(0x008A, vertex_attr1);
            clip_end1_attr2 = fx_ldw(vertex_attr2_ptr);
            next_vertex_ptr = (uint16_t)(vertex_attr2_ptr + 2);
            fx_stw(FX_CLIP_END1_ATTR2, clip_end1_attr2);
            if (rom_at_2 != 1) {
            } else {
                next_vertex_ptr = fx_ldw(0x0060);
            }
            next_vertex_x = fx_ldw(next_vertex_ptr);
            next_y_ptr = (uint16_t)(next_vertex_ptr + 2);
            next_vertex_y = fx_ldw(next_y_ptr);
            next_z_ptr = (uint16_t)(next_y_ptr + 2);
            next_vertex_z = fx_ldw(next_z_ptr);
            next_attr1_ptr = (uint16_t)(next_z_ptr + 2);
            clip_end2_attr1 = fx_ldw(next_attr1_ptr);
            next_attr2_ptr = (uint16_t)(next_attr1_ptr + 2);
            fx_stw(FX_CLIP_END2_ATTR1, clip_end2_attr1);
            next_vertex_attr2 = fx_ldw(next_attr2_ptr);
            fx_stw(0x0090, next_vertex_attr2);
            in_vertex_ptr = (uint16_t)(next_attr2_ptr - 8);
            slope_prod_y = (uint32_t)((int32_t)(int16_t)vertex_y * (int16_t)slope);
            slope_prod_hi = (uint16_t)(slope_prod_y >> 16);
            slope_y_x2 = (uint16_t)(slope_prod_hi + slope_prod_hi + (int)(slope_prod_y >> 15 & 1));
            slope_y_x4 = (uint16_t)(slope_y_x2 + slope_y_x2);
            if (!((uint16_t)((uint16_t)(vertex_z - slope_y_x4) - slope_y_x4) & 0x8000)) {
                fx_stw(out_vertex_ptr, vertex_x);
                out_y_ptr = (uint16_t)(out_vertex_ptr + 2);
                fx_stw(out_y_ptr, vertex_y);
                out_z_ptr = (uint16_t)(out_y_ptr + 2);
                fx_stw(out_z_ptr, vertex_z);
                out_attr1_ptr = (uint16_t)(out_z_ptr + 2);
                saved_attr1 = fx_ldw(0x008A);
                fx_stw(out_attr1_ptr, saved_attr1);
                out_attr2_ptr = (uint16_t)(out_attr1_ptr + 2);
                clip_end1_attr2_2 = fx_ldw(FX_CLIP_END1_ATTR2);
                fx_stw(out_attr2_ptr, clip_end1_attr2_2);
                out_vertex_ptr = (uint16_t)(out_attr2_ptr + 2);
                out_count_before_kept = fx_ldw(0x0040);
                fx_sbk((uint16_t)(out_count_before_kept + 1));
                slope_prod_next_y = (uint32_t)((int32_t)(int16_t)next_vertex_y * (int16_t)slope);
                slope_prod_next_hi = (uint16_t)(slope_prod_next_y >> 16);
                slope_next_y_x2 = (uint16_t)(slope_prod_next_hi + slope_prod_next_hi + (int)(slope_prod_next_y >> 15 & 1));
                slope_next_y_x4 = (uint16_t)(slope_next_y_x2 + slope_next_y_x2);
                if (!((uint16_t)((uint16_t)(next_vertex_z - slope_next_y_x4) - slope_next_y_x4) & 0x8000)) {
                } else {
                    { FxClipYPlaneResult r_ = FxClipYPlane(vertex_x, vertex_y, vertex_z, next_vertex_x, next_vertex_y, slope, next_vertex_z, stack, rom_at_2); clipped_b_x = r_.clipped_b_x; clipped_b_y = r_.clipped_b_y; next_vertex_z = r_.clipped_b_z; };
                    fx_stw(out_vertex_ptr, clipped_b_x);
                    clip_out_y_ptr = (uint16_t)(out_vertex_ptr + 2);
                    fx_stw(clip_out_y_ptr, clipped_b_y);
                    clip_out_z_ptr = (uint16_t)(clip_out_y_ptr + 2);
                    fx_stw(clip_out_z_ptr, next_vertex_z);
                    clip_out_attr1_ptr = (uint16_t)(clip_out_z_ptr + 2);
                    clip_end2_attr1_2 = fx_ldw(FX_CLIP_END2_ATTR1);
                    fx_stw(clip_out_attr1_ptr, clip_end2_attr1_2);
                    clip_out_attr2_ptr = (uint16_t)(clip_out_attr1_ptr + 2);
                    clip_attr2 = fx_ldw(0x0090);
                    fx_stw(clip_out_attr2_ptr, clip_attr2);
                    out_vertex_ptr = (uint16_t)(clip_out_attr2_ptr + 2);
                    out_count_before_clip = fx_ldw(0x0040);
                    fx_sbk((uint16_t)(out_count_before_clip + 1));
                }
            } else {
                slope_prod_next_y_outside = (uint32_t)((int32_t)(int16_t)next_vertex_y * (int16_t)slope);
                slope_prod_next_hi_outside = (uint16_t)(slope_prod_next_y_outside >> 16);
                slope_next_y_x2_outside = (uint16_t)(slope_prod_next_hi_outside + slope_prod_next_hi_outside + (int)(slope_prod_next_y_outside >> 15 & 1));
                slope_next_y_x4_outside = (uint16_t)(slope_next_y_x2_outside + slope_next_y_x2_outside);
                if (!((uint16_t)((uint16_t)(next_vertex_z - slope_next_y_x4_outside) - slope_next_y_x4_outside) & 0x8000)) {
                    { FxClipYPlaneResult r_ = FxClipYPlane(vertex_x, vertex_y, vertex_z, next_vertex_x, next_vertex_y, slope, next_vertex_z, stack, rom_at_2); clipped_a_x = r_.clipped_a_x; clipped_a_y = r_.clipped_a_y; clipped_a_z = r_.clipped_a_z; next_vertex_z = r_.clipped_b_z; };
                    fx_stw(out_vertex_ptr, clipped_a_x);
                    crossing_y_ptr = (uint16_t)(out_vertex_ptr + 2);
                    fx_stw(crossing_y_ptr, clipped_a_y);
                    crossing_z_ptr = (uint16_t)(crossing_y_ptr + 2);
                    fx_stw(crossing_z_ptr, clipped_a_z);
                    crossing_attr1_ptr = (uint16_t)(crossing_z_ptr + 2);
                    crossing_attr1 = fx_ldw(0x008A);
                    fx_stw(crossing_attr1_ptr, crossing_attr1);
                    crossing_attr2_ptr = (uint16_t)(crossing_attr1_ptr + 2);
                    clip_end1_attr2_3 = fx_ldw(FX_CLIP_END1_ATTR2);
                    fx_stw(crossing_attr2_ptr, clip_end1_attr2_3);
                    out_vertex_ptr = (uint16_t)(crossing_attr2_ptr + 2);
                    out_count_before_crossing = fx_ldw(0x0040);
                    fx_sbk((uint16_t)(out_count_before_crossing + 1));
                } else {
                }
            }
            edges_left = (uint16_t)(rom_at_2 - 1);
            rom_at_2 = edges_left;
            fx_r14(rom_at_2);
            if (edges_left == 0) {
                break;
            } else {
                continue;
            }
        }
        final_vertex_count = fx_ldw(0x0040);
        if (!((uint16_t)(final_vertex_count - 3) & 0x8000)) {
            rom_at_3 = fx_pull(stack);
            fx_r14(rom_at_3);
            fx_unspill(stack);
            return (FxPolygonClipTopResult){final_vertex_count, slope, next_vertex_z};
        } else {
            rom_at_4 = fx_pull(stack);
            fx_r14(rom_at_4);
            fx_unspill(stack);
            return (FxPolygonClipTopResult){0, slope, next_vertex_z};
        }
    } else {
        rom_at_4 = fx_pull(stack);
        fx_r14(rom_at_4);
        fx_unspill(stack);
        return (FxPolygonClipTopResult){0, slope, next_vertex_z};
    }
}


/*
 * @routine 01:C933 FxPolygonClipRight
 * WHAT IT DOES: As FxPolygonClipTop against z = x * slope ($92) * 4
 * (FxClipXPlane).
 * HOW: As FxPolygonClipTop.
 * Entry: as FxPolygonClipTop. Exit: as FxPolygonClipTop.
 */
FxPolygonClipRightResult FxPolygonClipRight(uint16_t entry_end2_z, uint16_t *stack, uint16_t rom_at)
{
    uint16_t cut_x;
    uint16_t cut_y;
    uint16_t cut_z;
    uint16_t end1_x;
    uint16_t end1_y;
    uint16_t end1_z;
    uint16_t end2_x;
    uint16_t end2_y;
    uint16_t kept_x;
    uint16_t kept_y;
    uint16_t rom_at_2;
    uint16_t rom_at_3;
    uint16_t rom_at_4;
    uint16_t clip_slope;
    uint16_t edges_left;
    uint16_t end1_attr1;
    uint16_t end1_limit;
    uint16_t end1_y_ptr;
    uint16_t end1_z_ptr;
    uint16_t end2_attr2;
    uint16_t end2_limit;
    uint16_t end2_y_ptr;
    uint16_t end2_z_ptr;
    uint16_t out_count_a;
    uint16_t out_count_b;
    uint16_t out_count_c;
    uint16_t out_y_ptr_a;
    uint16_t out_y_ptr_b;
    uint16_t out_y_ptr_c;
    uint16_t out_z_ptr_a;
    uint16_t out_z_ptr_b;
    uint16_t out_z_ptr_c;
    uint16_t end2_limit_b;
    uint16_t end1_limit_hi;
    uint16_t end2_limit_hi;
    uint16_t dst_vertex_ptr;
    uint16_t end1_attr1_ptr;
    uint16_t end1_attr2_ptr;
    uint16_t end2_attr1_ptr;
    uint16_t end2_attr2_ptr;
    uint16_t src_vertex_ptr;
    uint16_t clip_end1_attr2;
    uint16_t clip_end2_attr1;
    uint32_t end1_limit_prod;
    uint16_t end2_attr2_copy;
    uint16_t end2_limit_hi_b;
    uint32_t end2_limit_prod;
    uint16_t end2_vertex_ptr;
    uint16_t out_attr1_ptr_a;
    uint16_t out_attr1_ptr_b;
    uint16_t out_attr1_ptr_c;
    uint16_t out_attr2_ptr_a;
    uint16_t out_attr2_ptr_b;
    uint16_t out_attr2_ptr_c;
    uint16_t clip_end1_attr2_2;
    uint16_t clip_end1_attr2_3;
    uint16_t clip_end2_attr1_2;
    uint16_t end1_attr1_copy_a;
    uint16_t end1_attr1_copy_b;
    uint32_t end2_limit_prod_b;
    uint16_t final_vertex_count;

    fx_spill(stack);
    fx_push(stack, rom_at);
    clip_slope = fx_ldw(0x0092);
    rom_at_2 = fx_ldw(0x0040);
    fx_r14(rom_at_2);
    fx_stw(0x0040, 0);
    src_vertex_ptr = fx_ldw(0x0060);
    dst_vertex_ptr = fx_ldw(0x0062);
    if (rom_at_2 != 2) {
        for (;;) {
            end1_x = fx_ldw(src_vertex_ptr);
            end1_y_ptr = (uint16_t)(src_vertex_ptr + 2);
            end1_y = fx_ldw(end1_y_ptr);
            end1_z_ptr = (uint16_t)(end1_y_ptr + 2);
            end1_z = fx_ldw(end1_z_ptr);
            end1_attr1_ptr = (uint16_t)(end1_z_ptr + 2);
            end1_attr1 = fx_ldw(end1_attr1_ptr);
            end1_attr2_ptr = (uint16_t)(end1_attr1_ptr + 2);
            fx_stw(0x008A, end1_attr1);
            clip_end1_attr2 = fx_ldw(end1_attr2_ptr);
            end2_vertex_ptr = (uint16_t)(end1_attr2_ptr + 2);
            fx_stw(FX_CLIP_END1_ATTR2, clip_end1_attr2);
            if (rom_at_2 != 1) {
            } else {
                end2_vertex_ptr = fx_ldw(0x0060);
            }
            end2_x = fx_ldw(end2_vertex_ptr);
            end2_y_ptr = (uint16_t)(end2_vertex_ptr + 2);
            end2_y = fx_ldw(end2_y_ptr);
            end2_z_ptr = (uint16_t)(end2_y_ptr + 2);
            entry_end2_z = fx_ldw(end2_z_ptr);
            end2_attr1_ptr = (uint16_t)(end2_z_ptr + 2);
            clip_end2_attr1 = fx_ldw(end2_attr1_ptr);
            end2_attr2_ptr = (uint16_t)(end2_attr1_ptr + 2);
            fx_stw(FX_CLIP_END2_ATTR1, clip_end2_attr1);
            end2_attr2 = fx_ldw(end2_attr2_ptr);
            fx_stw(0x0090, end2_attr2);
            src_vertex_ptr = (uint16_t)(end2_attr2_ptr - 8);
            end1_limit_prod = (uint32_t)((int32_t)(int16_t)end1_x * (int16_t)clip_slope);
            end1_limit_hi = (uint16_t)(end1_limit_prod >> 16);
            end1_limit = (uint16_t)(end1_limit_hi + end1_limit_hi + (int)(end1_limit_prod >> 15 & 1));
            if (!((uint16_t)((uint16_t)(end1_z - end1_limit) - end1_limit) & 0x8000)) {
                fx_stw(dst_vertex_ptr, end1_x);
                out_y_ptr_a = (uint16_t)(dst_vertex_ptr + 2);
                fx_stw(out_y_ptr_a, end1_y);
                out_z_ptr_a = (uint16_t)(out_y_ptr_a + 2);
                fx_stw(out_z_ptr_a, end1_z);
                out_attr1_ptr_a = (uint16_t)(out_z_ptr_a + 2);
                end1_attr1_copy_a = fx_ldw(0x008A);
                fx_stw(out_attr1_ptr_a, end1_attr1_copy_a);
                out_attr2_ptr_a = (uint16_t)(out_attr1_ptr_a + 2);
                clip_end1_attr2_2 = fx_ldw(FX_CLIP_END1_ATTR2);
                fx_stw(out_attr2_ptr_a, clip_end1_attr2_2);
                dst_vertex_ptr = (uint16_t)(out_attr2_ptr_a + 2);
                out_count_a = fx_ldw(0x0040);
                fx_sbk((uint16_t)(out_count_a + 1));
                end2_limit_prod = (uint32_t)((int32_t)(int16_t)end2_x * (int16_t)clip_slope);
                end2_limit_hi = (uint16_t)(end2_limit_prod >> 16);
                end2_limit = (uint16_t)(end2_limit_hi + end2_limit_hi + (int)(end2_limit_prod >> 15 & 1));
                if (!((uint16_t)((uint16_t)(entry_end2_z - end2_limit) - end2_limit) & 0x8000)) {
                } else {
                    { FxClipXPlaneResult r_ = FxClipXPlane(end1_x, end1_y, end1_z, end2_x, end2_y, clip_slope, entry_end2_z, stack, rom_at_2); kept_x = r_.kept_x; kept_y = r_.kept_y; entry_end2_z = r_.kept_z; };
                    fx_stw(dst_vertex_ptr, kept_x);
                    out_y_ptr_c = (uint16_t)(dst_vertex_ptr + 2);
                    fx_stw(out_y_ptr_c, kept_y);
                    out_z_ptr_c = (uint16_t)(out_y_ptr_c + 2);
                    fx_stw(out_z_ptr_c, entry_end2_z);
                    out_attr1_ptr_c = (uint16_t)(out_z_ptr_c + 2);
                    clip_end2_attr1_2 = fx_ldw(FX_CLIP_END2_ATTR1);
                    fx_stw(out_attr1_ptr_c, clip_end2_attr1_2);
                    out_attr2_ptr_c = (uint16_t)(out_attr1_ptr_c + 2);
                    end2_attr2_copy = fx_ldw(0x0090);
                    fx_stw(out_attr2_ptr_c, end2_attr2_copy);
                    dst_vertex_ptr = (uint16_t)(out_attr2_ptr_c + 2);
                    out_count_c = fx_ldw(0x0040);
                    fx_sbk((uint16_t)(out_count_c + 1));
                }
            } else {
                end2_limit_prod_b = (uint32_t)((int32_t)(int16_t)end2_x * (int16_t)clip_slope);
                end2_limit_hi_b = (uint16_t)(end2_limit_prod_b >> 16);
                end2_limit_b = (uint16_t)(end2_limit_hi_b + end2_limit_hi_b + (int)(end2_limit_prod_b >> 15 & 1));
                if (!((uint16_t)((uint16_t)(entry_end2_z - end2_limit_b) - end2_limit_b) & 0x8000)) {
                    { FxClipXPlaneResult r_ = FxClipXPlane(end1_x, end1_y, end1_z, end2_x, end2_y, clip_slope, entry_end2_z, stack, rom_at_2); cut_x = r_.cut_x; cut_y = r_.cut_y; cut_z = r_.cut_z; entry_end2_z = r_.kept_z; };
                    fx_stw(dst_vertex_ptr, cut_x);
                    out_y_ptr_b = (uint16_t)(dst_vertex_ptr + 2);
                    fx_stw(out_y_ptr_b, cut_y);
                    out_z_ptr_b = (uint16_t)(out_y_ptr_b + 2);
                    fx_stw(out_z_ptr_b, cut_z);
                    out_attr1_ptr_b = (uint16_t)(out_z_ptr_b + 2);
                    end1_attr1_copy_b = fx_ldw(0x008A);
                    fx_stw(out_attr1_ptr_b, end1_attr1_copy_b);
                    out_attr2_ptr_b = (uint16_t)(out_attr1_ptr_b + 2);
                    clip_end1_attr2_3 = fx_ldw(FX_CLIP_END1_ATTR2);
                    fx_stw(out_attr2_ptr_b, clip_end1_attr2_3);
                    dst_vertex_ptr = (uint16_t)(out_attr2_ptr_b + 2);
                    out_count_b = fx_ldw(0x0040);
                    fx_sbk((uint16_t)(out_count_b + 1));
                } else {
                }
            }
            edges_left = (uint16_t)(rom_at_2 - 1);
            rom_at_2 = edges_left;
            fx_r14(rom_at_2);
            if (edges_left == 0) {
                break;
            } else {
                continue;
            }
        }
        final_vertex_count = fx_ldw(0x0040);
        if (!((uint16_t)(final_vertex_count - 3) & 0x8000)) {
            rom_at_3 = fx_pull(stack);
            fx_r14(rom_at_3);
            fx_unspill(stack);
            return (FxPolygonClipRightResult){final_vertex_count, clip_slope, entry_end2_z};
        } else {
            rom_at_4 = fx_pull(stack);
            fx_r14(rom_at_4);
            fx_unspill(stack);
            return (FxPolygonClipRightResult){0, clip_slope, entry_end2_z};
        }
    } else {
        rom_at_4 = fx_pull(stack);
        fx_r14(rom_at_4);
        fx_unspill(stack);
        return (FxPolygonClipRightResult){0, clip_slope, entry_end2_z};
    }
}


/*
 * @routine 01:CA42 FxPolygonClipLeft
 * WHAT IT DOES: As FxPolygonClipRight against z = -x * slope * 4: x is
 * negated around each FxClipXPlane (the clipped point's back afterwards).
 * HOW: As FxPolygonClipTop, testing z + the scaled x.
 * Entry: as FxPolygonClipTop. Exit: as FxPolygonClipTop.
 */
FxPolygonClipLeftResult FxPolygonClipLeft(uint16_t entry_z, uint16_t *stack, uint16_t rom_at)
{
    uint16_t cut_x;
    uint16_t cut_y;
    uint16_t cut_z;
    uint16_t end_x;
    uint16_t end_y;
    uint16_t kept_x;
    uint16_t kept_y;
    uint16_t start_x;
    uint16_t start_y;
    uint16_t start_z;
    uint16_t rom_at_2;
    uint16_t rom_at_3;
    uint16_t rom_at_4;
    uint16_t end_attr2;
    uint16_t end_y_ptr;
    uint16_t end_z_ptr;
    uint16_t out_count;
    uint16_t out_y_ptr;
    uint16_t out_z_ptr;
    uint16_t edges_left;
    uint16_t final_count;
    uint16_t slope_scale;
    uint16_t start_attr1;
    uint16_t start_y_ptr;
    uint16_t start_z_ptr;
    uint16_t end_attr1_ptr;
    uint16_t end_attr2_ptr;
    uint16_t in_vertex_ptr;
    uint16_t out_attr1_ptr;
    uint16_t out_attr2_ptr;
    uint16_t prod_end_hi_a;
    uint16_t prod_end_hi_b;
    uint16_t prod_start_hi;
    uint16_t clip_out_y_ptr;
    uint16_t clip_out_z_ptr;
    uint16_t end_vertex_ptr;
    uint16_t end_x_scaled_a;
    uint16_t end_x_scaled_b;
    uint16_t out_vertex_ptr;
    uint16_t start_x_scaled;
    uint16_t clip_end1_attr2;
    uint16_t clip_end2_attr1;
    uint16_t entry_out_y_ptr;
    uint16_t entry_out_z_ptr;
    uint16_t saved_end_attr2;
    uint16_t start_attr1_ptr;
    uint16_t start_attr2_ptr;
    uint16_t clip_end1_attr2_2;
    uint16_t clip_end1_attr2_3;
    uint16_t clip_end2_attr1_2;
    uint16_t saved_start_attr1;
    uint16_t clip_out_attr1_ptr;
    uint16_t clip_out_attr2_ptr;
    uint32_t prod_end_x_slope_a;
    uint32_t prod_end_x_slope_b;
    uint32_t prod_start_x_slope;
    uint16_t entry_out_attr1_ptr;
    uint16_t entry_out_attr2_ptr;
    uint16_t out_count_after_end;
    uint16_t out_count_after_entry;
    uint16_t saved_start_attr1_entry;

    fx_spill(stack);
    fx_push(stack, rom_at);
    slope_scale = fx_ldw(0x0092);
    rom_at_2 = fx_ldw(0x0040);
    fx_r14(rom_at_2);
    fx_stw(0x0040, 0);
    in_vertex_ptr = fx_ldw(0x0060);
    out_vertex_ptr = fx_ldw(0x0062);
    if (rom_at_2 != 2) {
        for (;;) {
            start_x = fx_ldw(in_vertex_ptr);
            start_y_ptr = (uint16_t)(in_vertex_ptr + 2);
            start_y = fx_ldw(start_y_ptr);
            start_z_ptr = (uint16_t)(start_y_ptr + 2);
            start_z = fx_ldw(start_z_ptr);
            start_attr1_ptr = (uint16_t)(start_z_ptr + 2);
            start_attr1 = fx_ldw(start_attr1_ptr);
            start_attr2_ptr = (uint16_t)(start_attr1_ptr + 2);
            fx_stw(0x008A, start_attr1);
            clip_end1_attr2 = fx_ldw(start_attr2_ptr);
            end_vertex_ptr = (uint16_t)(start_attr2_ptr + 2);
            fx_stw(FX_CLIP_END1_ATTR2, clip_end1_attr2);
            if (rom_at_2 != 1) {
            } else {
                end_vertex_ptr = fx_ldw(0x0060);
            }
            end_x = fx_ldw(end_vertex_ptr);
            end_y_ptr = (uint16_t)(end_vertex_ptr + 2);
            end_y = fx_ldw(end_y_ptr);
            end_z_ptr = (uint16_t)(end_y_ptr + 2);
            entry_z = fx_ldw(end_z_ptr);
            end_attr1_ptr = (uint16_t)(end_z_ptr + 2);
            clip_end2_attr1 = fx_ldw(end_attr1_ptr);
            end_attr2_ptr = (uint16_t)(end_attr1_ptr + 2);
            fx_stw(FX_CLIP_END2_ATTR1, clip_end2_attr1);
            end_attr2 = fx_ldw(end_attr2_ptr);
            fx_stw(0x0090, end_attr2);
            in_vertex_ptr = (uint16_t)(end_attr2_ptr - 8);
            prod_start_x_slope = (uint32_t)((int32_t)(int16_t)start_x * (int16_t)slope_scale);
            prod_start_hi = (uint16_t)(prod_start_x_slope >> 16);
            start_x_scaled = (uint16_t)(prod_start_hi + prod_start_hi + (int)(prod_start_x_slope >> 15 & 1));
            if (!((uint16_t)((uint16_t)(start_z + start_x_scaled) + start_x_scaled) & 0x8000)) {
                fx_stw(out_vertex_ptr, start_x);
                out_y_ptr = (uint16_t)(out_vertex_ptr + 2);
                fx_stw(out_y_ptr, start_y);
                out_z_ptr = (uint16_t)(out_y_ptr + 2);
                fx_stw(out_z_ptr, start_z);
                out_attr1_ptr = (uint16_t)(out_z_ptr + 2);
                saved_start_attr1 = fx_ldw(0x008A);
                fx_stw(out_attr1_ptr, saved_start_attr1);
                out_attr2_ptr = (uint16_t)(out_attr1_ptr + 2);
                clip_end1_attr2_2 = fx_ldw(FX_CLIP_END1_ATTR2);
                fx_stw(out_attr2_ptr, clip_end1_attr2_2);
                out_vertex_ptr = (uint16_t)(out_attr2_ptr + 2);
                out_count = fx_ldw(0x0040);
                fx_sbk((uint16_t)(out_count + 1));
                prod_end_x_slope_b = (uint32_t)((int32_t)(int16_t)end_x * (int16_t)slope_scale);
                prod_end_hi_b = (uint16_t)(prod_end_x_slope_b >> 16);
                end_x_scaled_b = (uint16_t)(prod_end_hi_b + prod_end_hi_b + (int)(prod_end_x_slope_b >> 15 & 1));
                if (!((uint16_t)((uint16_t)(entry_z + end_x_scaled_b) + end_x_scaled_b) & 0x8000)) {
                } else {
                    { FxClipXPlaneResult r_ = FxClipXPlane((uint16_t)((uint16_t)~start_x + 1), start_y, start_z, (uint16_t)((uint16_t)~end_x + 1), end_y, slope_scale, entry_z, stack, rom_at_2); kept_x = r_.kept_x; kept_y = r_.kept_y; entry_z = r_.kept_z; };
                    fx_stw(out_vertex_ptr, (uint16_t)((uint16_t)~kept_x + 1));
                    clip_out_y_ptr = (uint16_t)(out_vertex_ptr + 2);
                    fx_stw(clip_out_y_ptr, kept_y);
                    clip_out_z_ptr = (uint16_t)(clip_out_y_ptr + 2);
                    fx_stw(clip_out_z_ptr, entry_z);
                    clip_out_attr1_ptr = (uint16_t)(clip_out_z_ptr + 2);
                    clip_end2_attr1_2 = fx_ldw(FX_CLIP_END2_ATTR1);
                    fx_stw(clip_out_attr1_ptr, clip_end2_attr1_2);
                    clip_out_attr2_ptr = (uint16_t)(clip_out_attr1_ptr + 2);
                    saved_end_attr2 = fx_ldw(0x0090);
                    fx_stw(clip_out_attr2_ptr, saved_end_attr2);
                    out_vertex_ptr = (uint16_t)(clip_out_attr2_ptr + 2);
                    out_count_after_end = fx_ldw(0x0040);
                    fx_sbk((uint16_t)(out_count_after_end + 1));
                }
            } else {
                prod_end_x_slope_a = (uint32_t)((int32_t)(int16_t)end_x * (int16_t)slope_scale);
                prod_end_hi_a = (uint16_t)(prod_end_x_slope_a >> 16);
                end_x_scaled_a = (uint16_t)(prod_end_hi_a + prod_end_hi_a + (int)(prod_end_x_slope_a >> 15 & 1));
                if (!((uint16_t)((uint16_t)(entry_z + end_x_scaled_a) + end_x_scaled_a) & 0x8000)) {
                    { FxClipXPlaneResult r_ = FxClipXPlane((uint16_t)((uint16_t)~start_x + 1), start_y, start_z, (uint16_t)((uint16_t)~end_x + 1), end_y, slope_scale, entry_z, stack, rom_at_2); cut_x = r_.cut_x; cut_y = r_.cut_y; cut_z = r_.cut_z; entry_z = r_.kept_z; };
                    fx_stw(out_vertex_ptr, (uint16_t)((uint16_t)~cut_x + 1));
                    entry_out_y_ptr = (uint16_t)(out_vertex_ptr + 2);
                    fx_stw(entry_out_y_ptr, cut_y);
                    entry_out_z_ptr = (uint16_t)(entry_out_y_ptr + 2);
                    fx_stw(entry_out_z_ptr, cut_z);
                    entry_out_attr1_ptr = (uint16_t)(entry_out_z_ptr + 2);
                    saved_start_attr1_entry = fx_ldw(0x008A);
                    fx_stw(entry_out_attr1_ptr, saved_start_attr1_entry);
                    entry_out_attr2_ptr = (uint16_t)(entry_out_attr1_ptr + 2);
                    clip_end1_attr2_3 = fx_ldw(FX_CLIP_END1_ATTR2);
                    fx_stw(entry_out_attr2_ptr, clip_end1_attr2_3);
                    out_vertex_ptr = (uint16_t)(entry_out_attr2_ptr + 2);
                    out_count_after_entry = fx_ldw(0x0040);
                    fx_sbk((uint16_t)(out_count_after_entry + 1));
                } else {
                }
            }
            edges_left = (uint16_t)(rom_at_2 - 1);
            rom_at_2 = edges_left;
            fx_r14(rom_at_2);
            if (edges_left == 0) {
                break;
            } else {
                continue;
            }
        }
        final_count = fx_ldw(0x0040);
        if (!((uint16_t)(final_count - 3) & 0x8000)) {
            rom_at_3 = fx_pull(stack);
            fx_r14(rom_at_3);
            fx_unspill(stack);
            return (FxPolygonClipLeftResult){final_count, slope_scale, entry_z};
        } else {
            rom_at_4 = fx_pull(stack);
            fx_r14(rom_at_4);
            fx_unspill(stack);
            return (FxPolygonClipLeftResult){0, slope_scale, entry_z};
        }
    } else {
        rom_at_4 = fx_pull(stack);
        fx_r14(rom_at_4);
        fx_unspill(stack);
        return (FxPolygonClipLeftResult){0, slope_scale, entry_z};
    }
}


/*
 * @routine 01:CB63 FxPolygonClipBottom
 * WHAT IT DOES: As FxPolygonClipTop against z = -y * slope * 8: y is
 * negated around each FxClipYPlane (the clipped point's back afterwards).
 * HOW: As FxPolygonClipTop, testing z + the scaled y.
 * Entry: as FxPolygonClipTop. Exit: as FxPolygonClipTop.
 */
FxPolygonClipBottomResult FxPolygonClipBottom(uint16_t entry_r7_word, uint16_t *stack, uint16_t rom_at)
{
    uint16_t end1_x;
    uint16_t end1_y;
    uint16_t end1_z;
    uint16_t end2_x;
    uint16_t end2_y;
    uint16_t rom_at_2;
    uint16_t rom_at_3;
    uint16_t rom_at_4;
    uint16_t out_y_ptr;
    uint16_t out_z_ptr;
    uint16_t edges_left;
    uint16_t end1_attr1;
    uint16_t end1_y_ptr;
    uint16_t end1_z_ptr;
    uint16_t end2_attr2;
    uint16_t end2_y_ptr;
    uint16_t end2_z_ptr;
    uint16_t clipped_a_x;
    uint16_t clipped_a_y;
    uint16_t clipped_a_z;
    uint16_t clipped_b_x;
    uint16_t clipped_b_y;
    uint16_t entry_y_ptr;
    uint16_t entry_z_ptr;
    uint16_t out_count_a;
    uint16_t out_count_b;
    uint16_t out_count_c;
    uint16_t slope_scale;
    uint16_t end1_prod_hi;
    uint16_t end2_prod_hi;
    uint16_t clipped_y_ptr;
    uint16_t clipped_z_ptr;
    uint16_t out_attr1_ptr;
    uint16_t out_attr2_ptr;
    uint16_t dst_vertex_ptr;
    uint16_t end1_attr1_ptr;
    uint16_t end1_attr2_ptr;
    uint16_t end2_attr1_ptr;
    uint16_t end2_attr2_ptr;
    uint16_t src_vertex_ptr;
    uint16_t clip_end1_attr2;
    uint16_t clip_end2_attr1;
    uint32_t end1_slope_prod;
    uint32_t end2_slope_prod;
    uint16_t end2_vertex_ptr;
    uint16_t entry_attr1_ptr;
    uint16_t entry_attr2_ptr;
    uint16_t out_count_final;
    uint16_t end1_attr1_saved;
    uint16_t end1_prod_scaled;
    uint16_t end2_attr2_saved;
    uint16_t end2_prod_hi_alt;
    uint16_t end2_prod_scaled;
    uint16_t clip_end1_attr2_2;
    uint16_t clip_end1_attr2_3;
    uint16_t clip_end2_attr1_2;
    uint16_t clipped_attr1_ptr;
    uint16_t clipped_attr2_ptr;
    uint16_t end1_attr1_reload;
    uint16_t end1_slope_offset;
    uint16_t end2_slope_offset;
    uint32_t end2_slope_prod_alt;
    uint16_t end2_prod_scaled_alt;
    uint16_t end2_slope_offset_alt;

    fx_spill(stack);
    fx_push(stack, rom_at);
    slope_scale = fx_ldw(0x0094);
    rom_at_2 = fx_ldw(0x0040);
    fx_r14(rom_at_2);
    fx_stw(0x0040, 0);
    src_vertex_ptr = fx_ldw(0x0060);
    dst_vertex_ptr = fx_ldw(0x0062);
    if (rom_at_2 != 2) {
        for (;;) {
            end1_x = fx_ldw(src_vertex_ptr);
            end1_y_ptr = (uint16_t)(src_vertex_ptr + 2);
            end1_y = fx_ldw(end1_y_ptr);
            end1_z_ptr = (uint16_t)(end1_y_ptr + 2);
            end1_z = fx_ldw(end1_z_ptr);
            end1_attr1_ptr = (uint16_t)(end1_z_ptr + 2);
            end1_attr1 = fx_ldw(end1_attr1_ptr);
            end1_attr2_ptr = (uint16_t)(end1_attr1_ptr + 2);
            fx_stw(0x008A, end1_attr1);
            clip_end1_attr2 = fx_ldw(end1_attr2_ptr);
            end2_vertex_ptr = (uint16_t)(end1_attr2_ptr + 2);
            fx_stw(FX_CLIP_END1_ATTR2, clip_end1_attr2);
            if (rom_at_2 != 1) {
            } else {
                end2_vertex_ptr = fx_ldw(0x0060);
            }
            end2_x = fx_ldw(end2_vertex_ptr);
            end2_y_ptr = (uint16_t)(end2_vertex_ptr + 2);
            end2_y = fx_ldw(end2_y_ptr);
            end2_z_ptr = (uint16_t)(end2_y_ptr + 2);
            entry_r7_word = fx_ldw(end2_z_ptr);
            end2_attr1_ptr = (uint16_t)(end2_z_ptr + 2);
            clip_end2_attr1 = fx_ldw(end2_attr1_ptr);
            end2_attr2_ptr = (uint16_t)(end2_attr1_ptr + 2);
            fx_stw(FX_CLIP_END2_ATTR1, clip_end2_attr1);
            end2_attr2 = fx_ldw(end2_attr2_ptr);
            fx_stw(0x0090, end2_attr2);
            src_vertex_ptr = (uint16_t)(end2_attr2_ptr - 8);
            end1_slope_prod = (uint32_t)((int32_t)(int16_t)end1_y * (int16_t)slope_scale);
            end1_prod_hi = (uint16_t)(end1_slope_prod >> 16);
            end1_prod_scaled = (uint16_t)(end1_prod_hi + end1_prod_hi + (int)(end1_slope_prod >> 15 & 1));
            end1_slope_offset = (uint16_t)(end1_prod_scaled + end1_prod_scaled);
            if (!((uint16_t)((uint16_t)(end1_z + end1_slope_offset) + end1_slope_offset) & 0x8000)) {
                fx_stw(dst_vertex_ptr, end1_x);
                out_y_ptr = (uint16_t)(dst_vertex_ptr + 2);
                fx_stw(out_y_ptr, end1_y);
                out_z_ptr = (uint16_t)(out_y_ptr + 2);
                fx_stw(out_z_ptr, end1_z);
                out_attr1_ptr = (uint16_t)(out_z_ptr + 2);
                end1_attr1_saved = fx_ldw(0x008A);
                fx_stw(out_attr1_ptr, end1_attr1_saved);
                out_attr2_ptr = (uint16_t)(out_attr1_ptr + 2);
                clip_end1_attr2_2 = fx_ldw(FX_CLIP_END1_ATTR2);
                fx_stw(out_attr2_ptr, clip_end1_attr2_2);
                dst_vertex_ptr = (uint16_t)(out_attr2_ptr + 2);
                out_count_a = fx_ldw(0x0040);
                fx_sbk((uint16_t)(out_count_a + 1));
                end2_slope_prod = (uint32_t)((int32_t)(int16_t)end2_y * (int16_t)slope_scale);
                end2_prod_hi = (uint16_t)(end2_slope_prod >> 16);
                end2_prod_scaled = (uint16_t)(end2_prod_hi + end2_prod_hi + (int)(end2_slope_prod >> 15 & 1));
                end2_slope_offset = (uint16_t)(end2_prod_scaled + end2_prod_scaled);
                if (!((uint16_t)((uint16_t)(entry_r7_word + end2_slope_offset) + end2_slope_offset) & 0x8000)) {
                } else {
                    { FxClipYPlaneResult r_ = FxClipYPlane(end1_x, (uint16_t)((uint16_t)~end1_y + 1), end1_z, end2_x, (uint16_t)((uint16_t)~end2_y + 1), slope_scale, entry_r7_word, stack, rom_at_2); clipped_b_x = r_.clipped_b_x; clipped_b_y = r_.clipped_b_y; entry_r7_word = r_.clipped_b_z; };
                    fx_stw(dst_vertex_ptr, clipped_b_x);
                    clipped_y_ptr = (uint16_t)(dst_vertex_ptr + 2);
                    fx_stw(clipped_y_ptr, (uint16_t)((uint16_t)~clipped_b_y + 1));
                    clipped_z_ptr = (uint16_t)(clipped_y_ptr + 2);
                    fx_stw(clipped_z_ptr, entry_r7_word);
                    clipped_attr1_ptr = (uint16_t)(clipped_z_ptr + 2);
                    clip_end2_attr1_2 = fx_ldw(FX_CLIP_END2_ATTR1);
                    fx_stw(clipped_attr1_ptr, clip_end2_attr1_2);
                    clipped_attr2_ptr = (uint16_t)(clipped_attr1_ptr + 2);
                    end2_attr2_saved = fx_ldw(0x0090);
                    fx_stw(clipped_attr2_ptr, end2_attr2_saved);
                    dst_vertex_ptr = (uint16_t)(clipped_attr2_ptr + 2);
                    out_count_b = fx_ldw(0x0040);
                    fx_sbk((uint16_t)(out_count_b + 1));
                }
            } else {
                end2_slope_prod_alt = (uint32_t)((int32_t)(int16_t)end2_y * (int16_t)slope_scale);
                end2_prod_hi_alt = (uint16_t)(end2_slope_prod_alt >> 16);
                end2_prod_scaled_alt = (uint16_t)(end2_prod_hi_alt + end2_prod_hi_alt + (int)(end2_slope_prod_alt >> 15 & 1));
                end2_slope_offset_alt = (uint16_t)(end2_prod_scaled_alt + end2_prod_scaled_alt);
                if (!((uint16_t)((uint16_t)(entry_r7_word + end2_slope_offset_alt) + end2_slope_offset_alt) & 0x8000)) {
                    { FxClipYPlaneResult r_ = FxClipYPlane(end1_x, (uint16_t)((uint16_t)~end1_y + 1), end1_z, end2_x, (uint16_t)((uint16_t)~end2_y + 1), slope_scale, entry_r7_word, stack, rom_at_2); clipped_a_x = r_.clipped_a_x; clipped_a_y = r_.clipped_a_y; clipped_a_z = r_.clipped_a_z; entry_r7_word = r_.clipped_b_z; };
                    fx_stw(dst_vertex_ptr, clipped_a_x);
                    entry_y_ptr = (uint16_t)(dst_vertex_ptr + 2);
                    fx_stw(entry_y_ptr, (uint16_t)((uint16_t)~clipped_a_y + 1));
                    entry_z_ptr = (uint16_t)(entry_y_ptr + 2);
                    fx_stw(entry_z_ptr, clipped_a_z);
                    entry_attr1_ptr = (uint16_t)(entry_z_ptr + 2);
                    end1_attr1_reload = fx_ldw(0x008A);
                    fx_stw(entry_attr1_ptr, end1_attr1_reload);
                    entry_attr2_ptr = (uint16_t)(entry_attr1_ptr + 2);
                    clip_end1_attr2_3 = fx_ldw(FX_CLIP_END1_ATTR2);
                    fx_stw(entry_attr2_ptr, clip_end1_attr2_3);
                    dst_vertex_ptr = (uint16_t)(entry_attr2_ptr + 2);
                    out_count_c = fx_ldw(0x0040);
                    fx_sbk((uint16_t)(out_count_c + 1));
                } else {
                }
            }
            edges_left = (uint16_t)(rom_at_2 - 1);
            rom_at_2 = edges_left;
            fx_r14(rom_at_2);
            if (edges_left == 0) {
                break;
            } else {
                continue;
            }
        }
        out_count_final = fx_ldw(0x0040);
        if (!((uint16_t)(out_count_final - 3) & 0x8000)) {
            rom_at_3 = fx_pull(stack);
            fx_r14(rom_at_3);
            fx_unspill(stack);
            return (FxPolygonClipBottomResult){out_count_final, slope_scale, entry_r7_word};
        } else {
            rom_at_4 = fx_pull(stack);
            fx_r14(rom_at_4);
            fx_unspill(stack);
            return (FxPolygonClipBottomResult){0, slope_scale, entry_r7_word};
        }
    } else {
        rom_at_4 = fx_pull(stack);
        fx_r14(rom_at_4);
        fx_unspill(stack);
        return (FxPolygonClipBottomResult){0, slope_scale, entry_r7_word};
    }
}


/*
 * @routine 01:CC87 FxClipNear
 * WHAT IT DOES: Clips the segment from (R1, R2, R3) to (R4, R5, R7)
 * against the near plane at z = R14: an end is in front when R14 - z (or
 * R14 + z for a negative R14) is not negative. R0 = 0 when both are, $FFFF
 * when neither is; otherwise R0 = 1 with the end behind replaced by the
 * crossing (halving x, y and z, at most 12 steps) and its attributes
 * written back ($8A/$8E for the first end, $8C/$90 for the second).
 * HOW: As FxClipYPlane, with R14 the plane (not saved).
 * Entry: R1-R5, R7 the ends; R14 = the near z; R10 = stack; R11 = return
 * address. Exit: as above; R8, R9, R12, R13 restored; S, Z from R10; the
 * last RAM address R10.
 */
FxClipNearResult FxClipNear(uint16_t end1_x, uint16_t end1_y, uint16_t end1_z, uint16_t end2_x, uint16_t end2_y, uint16_t end2_z, uint16_t *stack, uint16_t rom_at)
{
    uint16_t count;
    uint16_t mid_z;
    uint16_t mid_z_b;
    uint16_t stack_2;
    uint16_t stack_3;
    uint16_t mid_diff;
    uint16_t inside_x_b;
    uint16_t inside_y_b;
    uint16_t inside_z_b;
    uint16_t mid_diff_b;
    uint16_t steps_left;
    uint16_t clip_kept_x;
    uint16_t clip_kept_y;
    uint16_t clip_kept_z;
    uint16_t outside_x_b;
    uint16_t outside_y_b;
    uint16_t outside_z_b;
    uint16_t inside_attr1;
    uint16_t mid_diff_neg;
    int mid_in_front;
    int mid_on_plane;
    uint16_t steps_left_b;
    uint16_t clip_kept_x_2;
    uint16_t clip_kept_y_2;
    uint16_t clip_kept_z_2;
    int end1_in_front;
    int end2_in_front;
    uint16_t outside_attr2;
    uint16_t inside_attr2_b;
    uint16_t mid_diff_neg_b;
    int mid_in_front_b;
    int mid_on_plane_b;
    uint16_t clip_end1_attr2;
    uint16_t clip_end2_attr1;
    int end2_in_front_b;
    int steps_exhausted;
    uint16_t clip_end1_attr2_2;
    uint16_t clip_end2_attr1_2;
    int steps_exhausted_b;

    fx_spill(stack);
    end1_in_front = (((uint16_t)(rom_at - end1_z) & 0x8000) != 0);
    if (!(rom_at & 0x8000)) {
    } else {
        end1_in_front = (((uint16_t)(rom_at + end1_z) & 0x8000) != 0);
    }
    if (end1_in_front) {
        end2_in_front = (((uint16_t)(rom_at - end2_z) & 0x8000) != 0);
        if (!(rom_at & 0x8000)) {
        } else {
            end2_in_front = (((uint16_t)(rom_at + end2_z) & 0x8000) != 0);
        }
        if (end2_in_front) {
            fx_unspill(stack);
            return (FxClipNearResult){end1_x, end1_y, end1_z, end2_x, end2_y, end2_z};
        } else {
            fx_stw(FX_CLIP_KEPT_X, end2_x);
            fx_stw(FX_CLIP_KEPT_Y, end2_y);
            fx_stw(FX_CLIP_KEPT_Z, end2_z);
            fx_spill(stack);
            fx_spill(stack);
            fx_spill(stack);
            fx_spill(stack);
            fx_stw(FX_CLIP_STACK, (*stack));
            stack_2 = 0x000C;
            inside_attr1 = fx_ldw(0x008A);
            clip_end1_attr2 = fx_ldw(FX_CLIP_END1_ATTR2);
            clip_end2_attr1 = fx_ldw(FX_CLIP_END2_ATTR1);
            outside_attr2 = fx_ldw(0x0090);
            for (;;) {
                mid_z = fx_div2((uint16_t)(end1_z + end2_z));
                mid_diff = (uint16_t)(rom_at - mid_z);
                mid_in_front = ((mid_diff & 0x8000) != 0);
                mid_on_plane = (mid_diff == 0);
                if (!(rom_at & 0x8000)) {
                } else {
                    mid_diff_neg = (uint16_t)(rom_at + mid_z);
                    mid_in_front = ((mid_diff_neg & 0x8000) != 0);
                    mid_on_plane = (mid_diff_neg == 0);
                }
                if (mid_on_plane) {
                    break;
                } else {
                    steps_left = (uint16_t)(stack_2 - 1);
                    steps_exhausted = (steps_left == 0);
                    stack_2 = steps_left;
                    if (mid_in_front) {
                        if (steps_exhausted) {
                            break;
                        } else {
                            end1_z = mid_z;
                            end1_x = fx_div2((uint16_t)(end1_x + end2_x));
                            end1_y = fx_div2((uint16_t)(end1_y + end2_y));
                            inside_attr1 = fx_div2((uint16_t)(inside_attr1 + clip_end2_attr1));
                            clip_end1_attr2 = fx_div2((uint16_t)(clip_end1_attr2 + outside_attr2));
                            continue;
                        }
                    } else {
                        if (steps_exhausted) {
                            break;
                        } else {
                            end2_z = mid_z;
                            end2_x = fx_div2((uint16_t)(end1_x + end2_x));
                            end2_y = fx_div2((uint16_t)(end1_y + end2_y));
                            clip_end2_attr1 = fx_div2((uint16_t)(inside_attr1 + clip_end2_attr1));
                            outside_attr2 = fx_div2((uint16_t)(clip_end1_attr2 + outside_attr2));
                            continue;
                        }
                    }
                }
            }
            clip_kept_x = fx_ldw(FX_CLIP_KEPT_X);
            clip_kept_y = fx_ldw(FX_CLIP_KEPT_Y);
            clip_kept_z = fx_ldw(FX_CLIP_KEPT_Z);
            fx_stw(0x008A, fx_div2((uint16_t)(inside_attr1 + clip_end2_attr1)));
            fx_stw(FX_CLIP_END1_ATTR2, fx_div2((uint16_t)(clip_end1_attr2 + outside_attr2)));
            (*stack) = fx_ldw(FX_CLIP_STACK);
            fx_unspill(stack);
            fx_unspill(stack);
            fx_unspill(stack);
            fx_unspill(stack);
            fx_unspill(stack);
            return (FxClipNearResult){fx_div2((uint16_t)(end1_x + end2_x)), fx_div2((uint16_t)(end1_y + end2_y)), mid_z, clip_kept_x, clip_kept_y, clip_kept_z};
        }
    } else {
        end2_in_front_b = (((uint16_t)(rom_at - end2_z) & 0x8000) != 0);
        if (!(rom_at & 0x8000)) {
        } else {
            end2_in_front_b = (((uint16_t)(rom_at + end2_z) & 0x8000) != 0);
        }
        if (end2_in_front_b) {
            fx_stw(FX_CLIP_KEPT_X, end1_x);
            fx_stw(FX_CLIP_KEPT_Y, end1_y);
            fx_stw(FX_CLIP_KEPT_Z, end1_z);
            fx_spill(stack);
            fx_spill(stack);
            fx_spill(stack);
            fx_spill(stack);
            fx_stw(FX_CLIP_STACK, (*stack));
            stack_3 = 0x000C;
            inside_x_b = end2_x;
            outside_x_b = end1_x;
            inside_y_b = end2_y;
            outside_y_b = end1_y;
            inside_z_b = end2_z;
            outside_z_b = end1_z;
            clip_end2_attr1_2 = fx_ldw(FX_CLIP_END2_ATTR1);
            inside_attr2_b = fx_ldw(0x0090);
            count = fx_ldw(0x008A);
            clip_end1_attr2_2 = fx_ldw(FX_CLIP_END1_ATTR2);
            for (;;) {
                mid_z_b = fx_div2((uint16_t)(inside_z_b + outside_z_b));
                mid_diff_b = (uint16_t)(rom_at - mid_z_b);
                mid_in_front_b = ((mid_diff_b & 0x8000) != 0);
                mid_on_plane_b = (mid_diff_b == 0);
                if (!(rom_at & 0x8000)) {
                } else {
                    mid_diff_neg_b = (uint16_t)(rom_at + mid_z_b);
                    mid_in_front_b = ((mid_diff_neg_b & 0x8000) != 0);
                    mid_on_plane_b = (mid_diff_neg_b == 0);
                }
                if (mid_on_plane_b) {
                    break;
                } else {
                    steps_left_b = (uint16_t)(stack_3 - 1);
                    steps_exhausted_b = (steps_left_b == 0);
                    stack_3 = steps_left_b;
                    if (mid_in_front_b) {
                        if (steps_exhausted_b) {
                            break;
                        } else {
                            inside_z_b = mid_z_b;
                            inside_x_b = fx_div2((uint16_t)(inside_x_b + outside_x_b));
                            inside_y_b = fx_div2((uint16_t)(inside_y_b + outside_y_b));
                            clip_end2_attr1_2 = fx_div2((uint16_t)(clip_end2_attr1_2 + count));
                            inside_attr2_b = fx_div2((uint16_t)(inside_attr2_b + clip_end1_attr2_2));
                            continue;
                        }
                    } else {
                        if (steps_exhausted_b) {
                            break;
                        } else {
                            outside_z_b = mid_z_b;
                            outside_x_b = fx_div2((uint16_t)(inside_x_b + outside_x_b));
                            outside_y_b = fx_div2((uint16_t)(inside_y_b + outside_y_b));
                            count = fx_div2((uint16_t)(clip_end2_attr1_2 + count));
                            clip_end1_attr2_2 = fx_div2((uint16_t)(inside_attr2_b + clip_end1_attr2_2));
                            continue;
                        }
                    }
                }
            }
            clip_kept_x_2 = fx_ldw(FX_CLIP_KEPT_X);
            clip_kept_y_2 = fx_ldw(FX_CLIP_KEPT_Y);
            clip_kept_z_2 = fx_ldw(FX_CLIP_KEPT_Z);
            fx_stw(FX_CLIP_END2_ATTR1, fx_div2((uint16_t)(clip_end2_attr1_2 + count)));
            fx_stw(0x0090, fx_div2((uint16_t)(inside_attr2_b + clip_end1_attr2_2)));
            (*stack) = fx_ldw(FX_CLIP_STACK);
            fx_unspill(stack);
            fx_unspill(stack);
            fx_unspill(stack);
            fx_unspill(stack);
            fx_unspill(stack);
            return (FxClipNearResult){clip_kept_x_2, clip_kept_y_2, clip_kept_z_2, fx_div2((uint16_t)(inside_x_b + outside_x_b)), fx_div2((uint16_t)(inside_y_b + outside_y_b)), mid_z_b};
        } else {
            fx_unspill(stack);
            return (FxClipNearResult){end1_x, end1_y, end1_z, end2_x, end2_y, end2_z};
        }
    }
}


/*
 * @routine 01:CE34 FxPolygonClipNear
 * WHAT IT DOES: As FxPolygonClipTop against the near plane at z = R0
 * (FxClipNear): R0 = the new vertex count, or 0.
 * HOW: As FxPolygonClipTop, with R14 = the near z and R9 counting, R6
 * reading and R8 writing the vertices.
 * Entry: R0 = the near z; R10 = stack; R11 = return address. Exit: as
 * FxPolygonClipTop, R8 past the output.
 */
FxPolygonClipNearResult FxPolygonClipNear(uint16_t near_z, uint16_t entry_z, uint16_t *stack, uint16_t rom_at)
{
    uint16_t end_x;
    uint16_t end_y;
    uint16_t start_x;
    uint16_t start_y;
    uint16_t start_z;
    uint16_t rom_at_2;
    uint16_t rom_at_3;
    uint16_t end_attr2;
    uint16_t end_y_ptr;
    uint16_t end_z_ptr;
    uint16_t out_count;
    uint16_t out_y_ptr;
    uint16_t out_z_ptr;
    uint16_t edges_left;
    uint16_t final_count;
    uint16_t start_attr1;
    uint16_t start_y_ptr;
    uint16_t start_z_ptr;
    uint16_t end_attr1_ptr;
    uint16_t end_attr2_ptr;
    uint16_t in_vertex_ptr;
    uint16_t out_attr1_ptr;
    uint16_t out_attr2_ptr;
    uint16_t clip_out_y_ptr;
    uint16_t clip_out_z_ptr;
    uint16_t clipped_end1_x;
    uint16_t clipped_end1_y;
    uint16_t clipped_end1_z;
    uint16_t clipped_end2_x;
    uint16_t clipped_end2_y;
    uint16_t end_vertex_ptr;
    uint16_t out_vertex_ptr;
    uint16_t clip_end1_attr2;
    uint16_t clip_end2_attr1;
    uint16_t edges_left_next;
    uint16_t entry_out_y_ptr;
    uint16_t entry_out_z_ptr;
    uint16_t saved_end_attr2;
    uint16_t start_attr1_ptr;
    uint16_t start_attr2_ptr;
    uint16_t clip_end1_attr2_2;
    uint16_t clip_end1_attr2_3;
    uint16_t clip_end2_attr1_2;
    uint16_t saved_start_attr1;
    int start_behind_near;
    uint16_t clip_out_attr1_ptr;
    uint16_t clip_out_attr2_ptr;
    uint16_t entry_out_attr1_ptr;
    uint16_t entry_out_attr2_ptr;
    uint16_t out_count_after_end;
    uint16_t out_count_after_entry;
    uint16_t saved_start_attr1_entry;
    int end_behind_near_start_in;
    int end_behind_near_start_out;

    fx_spill(stack);
    fx_push(stack, rom_at);
    fx_r14(near_z);
    edges_left = fx_ldw(0x0040);
    fx_stw(0x0040, 0);
    in_vertex_ptr = fx_ldw(0x0060);
    out_vertex_ptr = fx_ldw(0x0062);
    if (edges_left != 2) {
        for (;;) {
            start_x = fx_ldw(in_vertex_ptr);
            start_y_ptr = (uint16_t)(in_vertex_ptr + 2);
            start_y = fx_ldw(start_y_ptr);
            start_z_ptr = (uint16_t)(start_y_ptr + 2);
            start_z = fx_ldw(start_z_ptr);
            start_attr1_ptr = (uint16_t)(start_z_ptr + 2);
            start_attr1 = fx_ldw(start_attr1_ptr);
            start_attr2_ptr = (uint16_t)(start_attr1_ptr + 2);
            fx_stw(0x008A, start_attr1);
            clip_end1_attr2 = fx_ldw(start_attr2_ptr);
            end_vertex_ptr = (uint16_t)(start_attr2_ptr + 2);
            fx_stw(FX_CLIP_END1_ATTR2, clip_end1_attr2);
            if (edges_left != 1) {
            } else {
                end_vertex_ptr = fx_ldw(0x0060);
            }
            end_x = fx_ldw(end_vertex_ptr);
            end_y_ptr = (uint16_t)(end_vertex_ptr + 2);
            end_y = fx_ldw(end_y_ptr);
            end_z_ptr = (uint16_t)(end_y_ptr + 2);
            entry_z = fx_ldw(end_z_ptr);
            end_attr1_ptr = (uint16_t)(end_z_ptr + 2);
            clip_end2_attr1 = fx_ldw(end_attr1_ptr);
            end_attr2_ptr = (uint16_t)(end_attr1_ptr + 2);
            fx_stw(FX_CLIP_END2_ATTR1, clip_end2_attr1);
            end_attr2 = fx_ldw(end_attr2_ptr);
            fx_stw(0x0090, end_attr2);
            in_vertex_ptr = (uint16_t)(end_attr2_ptr - 8);
            start_behind_near = (((uint16_t)(near_z - start_z) & 0x8000) != 0);
            if (!(near_z & 0x8000)) {
            } else {
                start_behind_near = (((uint16_t)(near_z + start_z) & 0x8000) != 0);
            }
            if (!start_behind_near) {
                fx_stw(out_vertex_ptr, start_x);
                out_y_ptr = (uint16_t)(out_vertex_ptr + 2);
                fx_stw(out_y_ptr, start_y);
                out_z_ptr = (uint16_t)(out_y_ptr + 2);
                fx_stw(out_z_ptr, start_z);
                out_attr1_ptr = (uint16_t)(out_z_ptr + 2);
                saved_start_attr1 = fx_ldw(0x008A);
                fx_stw(out_attr1_ptr, saved_start_attr1);
                out_attr2_ptr = (uint16_t)(out_attr1_ptr + 2);
                clip_end1_attr2_2 = fx_ldw(FX_CLIP_END1_ATTR2);
                fx_stw(out_attr2_ptr, clip_end1_attr2_2);
                out_vertex_ptr = (uint16_t)(out_attr2_ptr + 2);
                out_count = fx_ldw(0x0040);
                fx_sbk((uint16_t)(out_count + 1));
                end_behind_near_start_in = (((uint16_t)(near_z - entry_z) & 0x8000) != 0);
                if (!(near_z & 0x8000)) {
                } else {
                    end_behind_near_start_in = (((uint16_t)(near_z + entry_z) & 0x8000) != 0);
                }
                if (!end_behind_near_start_in) {
                } else {
                    { FxClipNearResult r_ = FxClipNear(start_x, start_y, start_z, end_x, end_y, entry_z, stack, near_z); clipped_end2_x = r_.clipped_end2_x; clipped_end2_y = r_.clipped_end2_y; entry_z = r_.clipped_end2_z; };
                    fx_stw(out_vertex_ptr, clipped_end2_x);
                    clip_out_y_ptr = (uint16_t)(out_vertex_ptr + 2);
                    fx_stw(clip_out_y_ptr, clipped_end2_y);
                    clip_out_z_ptr = (uint16_t)(clip_out_y_ptr + 2);
                    fx_stw(clip_out_z_ptr, entry_z);
                    clip_out_attr1_ptr = (uint16_t)(clip_out_z_ptr + 2);
                    clip_end2_attr1_2 = fx_ldw(FX_CLIP_END2_ATTR1);
                    fx_stw(clip_out_attr1_ptr, clip_end2_attr1_2);
                    clip_out_attr2_ptr = (uint16_t)(clip_out_attr1_ptr + 2);
                    saved_end_attr2 = fx_ldw(0x0090);
                    fx_stw(clip_out_attr2_ptr, saved_end_attr2);
                    out_vertex_ptr = (uint16_t)(clip_out_attr2_ptr + 2);
                    out_count_after_end = fx_ldw(0x0040);
                    fx_sbk((uint16_t)(out_count_after_end + 1));
                }
            } else {
                end_behind_near_start_out = (((uint16_t)(near_z - entry_z) & 0x8000) != 0);
                if (!(near_z & 0x8000)) {
                } else {
                    end_behind_near_start_out = (((uint16_t)(near_z + entry_z) & 0x8000) != 0);
                }
                if (!end_behind_near_start_out) {
                    { FxClipNearResult r_ = FxClipNear(start_x, start_y, start_z, end_x, end_y, entry_z, stack, near_z); clipped_end1_x = r_.clipped_end1_x; clipped_end1_y = r_.clipped_end1_y; clipped_end1_z = r_.clipped_end1_z; entry_z = r_.clipped_end2_z; };
                    fx_stw(out_vertex_ptr, clipped_end1_x);
                    entry_out_y_ptr = (uint16_t)(out_vertex_ptr + 2);
                    fx_stw(entry_out_y_ptr, clipped_end1_y);
                    entry_out_z_ptr = (uint16_t)(entry_out_y_ptr + 2);
                    fx_stw(entry_out_z_ptr, clipped_end1_z);
                    entry_out_attr1_ptr = (uint16_t)(entry_out_z_ptr + 2);
                    saved_start_attr1_entry = fx_ldw(0x008A);
                    fx_stw(entry_out_attr1_ptr, saved_start_attr1_entry);
                    entry_out_attr2_ptr = (uint16_t)(entry_out_attr1_ptr + 2);
                    clip_end1_attr2_3 = fx_ldw(FX_CLIP_END1_ATTR2);
                    fx_stw(entry_out_attr2_ptr, clip_end1_attr2_3);
                    out_vertex_ptr = (uint16_t)(entry_out_attr2_ptr + 2);
                    out_count_after_entry = fx_ldw(0x0040);
                    fx_sbk((uint16_t)(out_count_after_entry + 1));
                } else {
                }
            }
            edges_left_next = (uint16_t)(edges_left - 1);
            edges_left = edges_left_next;
            if (edges_left_next == 0) {
                break;
            } else {
                continue;
            }
        }
        final_count = fx_ldw(0x0040);
        if (!((uint16_t)(final_count - 3) & 0x8000)) {
            rom_at_2 = fx_pull(stack);
            fx_r14(rom_at_2);
            fx_unspill(stack);
            return (FxPolygonClipNearResult){final_count, in_vertex_ptr, entry_z};
        } else {
            rom_at_3 = fx_pull(stack);
            fx_r14(rom_at_3);
            fx_unspill(stack);
            return (FxPolygonClipNearResult){0, in_vertex_ptr, entry_z};
        }
    } else {
        rom_at_3 = fx_pull(stack);
        fx_r14(rom_at_3);
        fx_unspill(stack);
        return (FxPolygonClipNearResult){0, in_vertex_ptr, entry_z};
    }
}

