/*
 * fx_record.c: small Super FX routines on object records in Super FX RAM.
 *
 * R8 or R9 is the record; records in a list are linked through +$0E (0 ends
 * the list). The push at $70:23A4 is five words: on (1) or off (0), an
 * amount, and a vector.
 */
#include <string.h>
#include "gsu_ops.h"
#include "fx_calls.h"

#define REC_NEXT 0x0Eu
#define REC_X 0x1Eu
#define REC_Z 0x22u
#define REC_FLAGS10 0x10u
#define REC_Y 0x20u
#define REC_FLAGS40 0x40u
#define REC_Y_HELD 0x60u
#define REC_VEL 0x2Eu
#define REC_SPEED 0x4Au
#define REC_ALIGN 0x6Cu

#define FX_MATRIX_Z 0x00F0u   /* the matrix's third column */
#define REC_LIST 0x18u

#define FX_PUSH 0x23A4u    /* on, amount, x, y, z */
#define FX_ARG_RECORD 0x238Eu  /* a program's record, from the S-CPU */
#define FX_ARG_AMOUNT 0x2390u  /* and its amount */
#define REC_6A 0x6Au
#define REC_82 0x82u

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode FxPushSet_code[2][5] = {
    {{0, 0}, {0x049690, 4}, {0x049694, 6}, {0x04969A, 4}, {0x04969E, 7}},
    {{0x049689, 2}, {0x049690, 4}, {0x049694, 6}, {0x04969A, 4}, {0x04969E, 7}},
};

/*
 * @routine 04:9683 FxPushSet
 * WHAT IT DOES: Sets the push to amount R2, on when R2 is not negative, with
 * the vector (0, $8100, 0).
 * HOW: SUB R0 and MOVES R2 for the sign, then five SMs in field order with
 * R2 reused for the zeros and $8100.
 * Entry: R2 = amount; R11 = return address. Exit: R0 = 1 or 0; R2 = $8100;
 * OV, S, Z from R2's sign test, CY from the SUB (set); the last RAM
 * address $23AA.
 */
uint16_t FxPushSet(uint16_t amount, int copy)
{
    uint16_t on = gsu_sub(0, 0, 0);          /* SUB R0 from itself */

    gsu_moves(amount);
    if (!gsu.s)
        { gsu_codes(FxPushSet_code[copy], 1); on = 1; }
    gsu_stw(FX_PUSH, on);
    gsu_codes(FxPushSet_code[copy] + 1, 1);
    gsu_stw(FX_PUSH + 2, amount);
    gsu_codes(FxPushSet_code[copy] + 2, 1);
    gsu_stw(FX_PUSH + 4, 0);
    gsu_codes(FxPushSet_code[copy] + 3, 1);
    gsu_stw(FX_PUSH + 8, 0);
    gsu_codes(FxPushSet_code[copy] + 4, 1);
    gsu_stw(FX_PUSH + 6, 0x8100);
    return on;
}

void abi_FxPushSet(void)
{
    gsu.r[0] = FxPushSet(gsu.r[2], GSU_COPY(0));
    gsu.r[2] = 0x8100;
}

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode FxListClearBit0_code[3][10] = {
    {{0x048A18, 8}, {0x04A441, 7}, {0x04A448, 7}, {0x04A44F, 5}, {0x04A454, 3}, {0x04A457, 4}, {0x04A45B, 4}, {0, 0}, {0x048A20, 5}, {0x04A45F, 10}},
    {{0, 0}, {0, 0}, {0x04A448, 12}, {0, 0}, {0x04A454, 3}, {0x04A457, 4}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
    {{0x048A18, 8}, {0x04A441, 7}, {0x04A448, 12}, {0, 0}, {0x04A454, 3}, {0x04A457, 4}, {0x048A20, 5}, {0x04A45B, 14}, {0, 0}, {0, 0}},
};

/*
 * @routine 04:A441 FxListClearBit0
 * WHAT IT DOES: Clears bit 0 of +$10 in every record of the list whose head
 * is at +$18 of record R6.
 * HOW: LDW, BIC R12 (= 1) and SBK per record, then the link at +$0E with
 * MOVES until it is 0.
 * Entry: R6 = the record holding the list; R11 = return address. Exit: R8 =
 * 0; with records: R12 = 1, R0 = 0; OV, S clear, Z set; the last RAM
 * address the last record's +$0E (else R6 + $18).
 */
void FxListClearBit0(uint16_t owner, int *any, int copy)
{
    gsu_codes(FxListClearBit0_code[copy], 2);
    uint16_t rec = gsu_ldw(gsu_add(REC_LIST, owner, 0));

    gsu_moves(rec);
    *any = !gsu.z;
    gsu_codes(FxListClearBit0_code[copy] + 2, 1);
    while (gsu_codes_loop(FxListClearBit0_code[copy] + 3, 1, !gsu.z)) {
        gsu_sbk(gsu_codes_then(FxListClearBit0_code[copy] + 4, 1, gsu_bic(gsu_ldw(gsu_add(REC_FLAGS10, rec, 0)), 1)));
        gsu_codes(FxListClearBit0_code[copy] + 5, 1);
        rec = gsu_ldw(gsu_add(REC_NEXT, rec, 0));
        gsu_moves(rec);
        gsu_codes(FxListClearBit0_code[copy] + 6, 2);
    }
    gsu_codes(FxListClearBit0_code[copy] + 8, 2);
}

void abi_FxListClearBit0(void)
{
    int any;

    FxListClearBit0(gsu.r[6], &any, GSU_COPY(0));
    gsu.r[8] = 0;
    if (any) {
        gsu.r[0] = 0;
        gsu.r[12] = 1;
    }
}

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode FxRecordHoldY_code[2][9] = {
    {{0x04BC80, 13}, {0x04BC8D, 5}, {0x04BC80, 6}, {0x04BC91, 1}, {0x04BC92, 6}, {0x04BC98, 4}, {0x04BC9C, 7}, {0x04BC9C, 4}, {0x04BCA2, 1}},
    {{0x04BC80, 13}, {0x04BC8D, 5}, {0x04BC80, 6}, {0x04BC91, 1}, {0x04BC92, 6}, {0x04BC98, 4}, {0x04BC9C, 7}, {0, 0}, {0, 0}},
};

/*
 * @routine 04:BC7B FxRecordHoldY
 * WHAT IT DOES: Keeps record R8's +$60 as its height +$20 while bit 8 of
 * +$40 is clear; while it is set, +$60 holds, and becomes 0 once it is
 * below +$20.
 * HOW: AND #$0100 of +$40; the copy is an LDW and STW; "below" is the sign
 * of +$60 - +$20 (CMP); SUB R0 and SBK clear it.
 * Entry: R8 = the record; R11 = return address. Exit: R1 = +$20; R0 = +$60
 * (0 when cleared); OV, S, CY, Z from the CMP or the SUB; the last RAM
 * address R8 + $60.
 */
uint16_t FxRecordHoldY(uint16_t rec, uint16_t *y_out, int copy)
{
    uint16_t y, held;

    gsu_and(gsu_ldw(gsu_add(REC_FLAGS40, rec, 0)), 0x0100);
    if (gsu.z) {
        gsu_codes(FxRecordHoldY_code[copy], 1);
        y = gsu_ldw(gsu_add(REC_Y, rec, 0));
        gsu_codes(FxRecordHoldY_code[copy] + 1, 1);
        gsu_stw(gsu_add(REC_Y_HELD, rec, 0), y);
    } else { gsu_codes(FxRecordHoldY_code[copy] + 2, 2); }
    gsu_codes(FxRecordHoldY_code[copy] + 4, 1);
    y = gsu_ldw(gsu_add(REC_Y, rec, 0));
    gsu_codes(FxRecordHoldY_code[copy] + 5, 1);
    held = gsu_ldw(gsu_add(REC_Y_HELD, rec, 0));
    gsu_sub(held, y, 0);
    if (gsu.s) {
        gsu_codes(FxRecordHoldY_code[copy] + 6, 1);
        held = gsu_sub(held, held, 0);
        gsu_sbk(held);
    } else { gsu_codes(FxRecordHoldY_code[copy] + 7, 2); }
    *y_out = y;
    return held;
}

void abi_FxRecordHoldY(void)
{
    uint16_t y;

    gsu.r[0] = FxRecordHoldY(gsu.r[8], &y, GSU_COPY(0));
    gsu.r[1] = y;
}

/* cyccopy: the copy each of the caller's copies calls */
static const uint8_t FxAngleOf_from_FxBearingTo[6] = {0, 1, 2, 3, 4, 5};

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode FxBearingTo_code[6][8] = {
    {{0, 0}, {0, 0}, {0x04CBEB, 6}, {0x04CBF1, 6}, {0x04C2F8, 3}, {0x04CBF7, 8}, {0x04CBFF, 7}, {0, 0}},
    {{0, 0}, {0, 0}, {0x04CBEB, 6}, {0x04CBF1, 6}, {0x04C2F8, 3}, {0x04CBF7, 8}, {0x04CBFF, 7}, {0x04C6DB, 8}},
    {{0, 0}, {0, 0}, {0x04CBEB, 6}, {0x04CBF1, 6}, {0x04C2F8, 3}, {0x04CBF7, 8}, {0x04CBFF, 7}, {0, 0}},
    {{0, 0}, {0, 0}, {0x04CBEB, 6}, {0x04CBF1, 6}, {0x04C2F8, 3}, {0x04CBF7, 8}, {0x04CBFF, 7}, {0x04C6DB, 8}},
    {{0x04C7EF, 6}, {0x04CBE8, 3}, {0x04CBEB, 6}, {0x04CBF1, 6}, {0x04C2F8, 3}, {0x04CBF7, 8}, {0x04CBFF, 7}, {0x04C7F5, 7}},
    {{0, 0}, {0, 0}, {0x04CBEB, 6}, {0x04CBF1, 6}, {0x04C2F8, 3}, {0x04CBF7, 8}, {0x04CBFF, 7}, {0, 0}},
};

/*
 * @routine 04:CBE8 FxBearingTo
 * WHAT IT DOES: The angle from record R8's position (+$1E, +$22) to the
 * point (R1, R3), negated: FxAngleOf of the difference, then NOT and INC.
 * HOW: The differences replace R1 and R3 (SUB under WITH), R2 = the z
 * difference for FxAngleOf; the return address is kept on the R10 stack.
 * Entry: R1 = x, R3 = z; R8 = the record; R10 = stack; R11 = return
 * address. Exit: R0 = the angle; R1 = the x difference; R2 = R3 = the z
 * difference; R4-R6, R11-R14 as FxAngleOf leaves them (R11 = $CBFF when it
 * did not divide); S, Z from R10; the last RAM address R10.
 */
FxRegs FxBearingTo(FxRegs g, int copy)
{
    gsu_codes(FxBearingTo_code[copy], 2);
    gsu_stw(g.r[10], g.r[11]);
    g.r[10] = gsu_inc(gsu_inc(g.r[10]));
    gsu_codes(FxBearingTo_code[copy] + 2, 1);
    g.r[0] = gsu_ldw(gsu_add(REC_X, g.r[8], 0));
    g.r[1] = gsu_sub(g.r[1], g.r[0], 0);
    gsu_codes(FxBearingTo_code[copy] + 3, 1);
    g.r[0] = gsu_ldw(gsu_add(REC_Z, g.r[8], 0));
    g.r[3] = gsu_sub(g.r[3], g.r[0], 0);
    g.r[2] = g.r[3];
    g.r[11] = 0xCBFF;
    gsu_codes(FxBearingTo_code[copy] + 4, 2);
    g = FxAngleOf(g, GSU_COPY(FxAngleOf_from_FxBearingTo[copy]));
    g.r[0] = gsu_inc(gsu_not(g.r[0]));
    g.r[10] = gsu_dec(gsu_dec(g.r[10]));
    gsu_codes(FxBearingTo_code[copy] + 6, 1);
    gsu_ldw(g.r[10]);
    gsu_codes(FxBearingTo_code[copy] + 7, 1);
    return g;
}

void abi_FxBearingTo(void)
{
    FxRegs g;

    memcpy(g.r, gsu.r, sizeof g.r);
    g = FxBearingTo(g, GSU_COPY(1));
    memcpy(gsu.r, g.r, 15 * sizeof g.r[0]);
}

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode FxRecordAlignment_code[2][14] = {
    {{0x04BCBD, 4}, {0x04BCC1, 8}, {0x04BCD5, 4}, {0x04C087, 3}, {0x04BCD9, 4}, {0x04BCDD, 8}, {0x04BCF0, 17}, {0x04BD01, 4}, {0x04BD09, 2}, {0x04BD05, 4}, {0x04BD0B, 8}, {0x04BD16, 6}, {0x04BD13, 3}, {0x04BD1C, 4}},
    {{0x04BCBD, 4}, {0x04BCC1, 8}, {0x04BCD5, 4}, {0x04C087, 3}, {0x04BCD9, 4}, {0x04BCDD, 8}, {0x04BCF0, 17}, {0x04BD01, 4}, {0x04BD09, 2}, {0x04BD05, 4}, {0x04BD0B, 8}, {0x04BD16, 6}, {0x04BD13, 3}, {0x04BD1C, 4}},
};

/*
 * @routine 04:BCBA FxRecordAlignment
 * WHAT IT DOES: Sets record R8's +$6C to how closely its velocity points
 * along the third axis of the matrix at $00E4 (the column at $00F0): the
 * dot product of the unit velocity with it, $7FFF for straight along; or
 * $7FFF when its speed +$4A is below 10 either way.
 * HOW: FxNormalize of the velocity (+$2E, +$30, +$32), three LMULTs summed
 * as 32 bits and the result doubled (ADD, ROL) to its high word; R8 and the
 * return address are kept on the R10 stack.
 * Entry: R8 = the record; R10 = stack; R11 = return address. Exit: R1 =
 * the value stored; R0 = R8 + $6C; R2 = 10; R3, R9, R12, R13 as
 * FxNormalize leaves them; R4, R5 = the last product's low and high
 * words; R6 = the word at $00F4; R7 = the low word of the sum, doubled; R11
 * = $C0E3 ($BCD9 for a zero velocity); S, Z from R10, OV, CY from the add
 * for +$6C; the last RAM address R10.
 */
FxRegs FxRecordAlignment(FxRegs g, int copy)
{
    uint32_t p;
    uint16_t rec = g.r[8];
    int i;

    gsu_stw(g.r[10], g.r[11]);
    g.r[10] = gsu_inc(gsu_inc(g.r[10]));
    gsu_codes(FxRecordAlignment_code[copy], 1);
    gsu_stw(g.r[10], g.r[8]);
    g.r[10] = gsu_inc(gsu_inc(g.r[10]));
    gsu_codes(FxRecordAlignment_code[copy] + 1, 1);
    for (i = 0; i < 3; i++)
        { g.r[1 + i] = gsu_ldw(gsu_add((uint16_t)(REC_VEL + 2 * i), rec, 0)); gsu_codes(FxRecordAlignment_code[copy] + 2, 2); }
    g.r[11] = 0xBCD9;
    g = FxNormalize(g, GSU_COPY(0));
    gsu_codes(FxRecordAlignment_code[copy] + 4, 1);
    for (i = 0; i < 3; i++) {
        g.r[6] = gsu_ldw((uint16_t)(FX_MATRIX_Z + 2 * i));
        p = gsu_lmult(g.r[1 + i], g.r[6]);
        g.r[4] = (uint16_t)p;
        if (i == 0) {
            gsu_codes(FxRecordAlignment_code[copy] + 5, 1);
            g.r[0] = (uint16_t)(p >> 16);
            g.r[7] = g.r[4];
        } else {
            g.r[5] = (uint16_t)(p >> 16);
            g.r[7] = gsu_add(g.r[7], g.r[4], 0);
            g.r[0] = gsu_add(g.r[0], g.r[5], gsu.cy);
        }
    }
    gsu_codes(FxRecordAlignment_code[copy] + 6, 1);
    g.r[7] = gsu_add(g.r[7], g.r[7], 0);
    g.r[1] = g.r[0] = gsu_rol(g.r[0]);
    g.r[10] = gsu_dec(gsu_dec(g.r[10]));
    g.r[8] = gsu_ldw(g.r[10]);
    gsu_codes(FxRecordAlignment_code[copy] + 7, 1);
    g.r[0] = gsu_ldw(gsu_add(REC_SPEED, g.r[8], 0));
    gsu_moves(g.r[0]);
    if (gsu.s)
        { gsu_codes(FxRecordAlignment_code[copy] + 8, 1); g.r[0] = gsu_inc(gsu_not(g.r[0])); }
    gsu_codes(FxRecordAlignment_code[copy] + 9, 3);
    g.r[2] = 10;
    gsu_sub(g.r[0], g.r[2], 0);
    if (gsu.s)
        { gsu_codes(FxRecordAlignment_code[copy] + 12, 1); g.r[1] = 0x7FFF; }
    g.r[0] = gsu_add(REC_ALIGN, g.r[8], 0);
    gsu_stw(g.r[0], g.r[1]);
    g.r[10] = gsu_dec(gsu_dec(g.r[10]));
    gsu_codes(FxRecordAlignment_code[copy] + 13, 1);
    gsu_ldw(g.r[10]);
    return g;
}

void abi_FxRecordAlignment(void)
{
    FxRegs g;

    memcpy(g.r, gsu.r, sizeof g.r);
    g = FxRecordAlignment(g, GSU_COPY(0));
    memcpy(gsu.r, g.r, 15 * sizeof g.r[0]);
}

typedef struct {
    uint16_t r0, r1, r2, r6, r8;
} FxAmountRegs;

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode amount_start_code[2][5] = {
    {{0x04BF52, 9}, {0x04BF5B, 4}, {0x04BF5F, 6}, {0x04BF65, 11}, {0x04BF72, 2}},
    {{0x04BF76, 9}, {0x04BF7F, 4}, {0x04BF83, 7}, {0x04BF8A, 13}, {0, 0}},
};

/* The banks to 0 and the record and amount the S-CPU left; the field's
   address in the record is loaded last, so SBK stores there. */
static FxAmountRegs amount_start(uint16_t field, uint16_t scale_half, int copy)
{
    gsu_codes(amount_start_code[copy], 1);
    FxAmountRegs g;

    g.r0 = gsu_sub(0, 0, 0);               /* SUB R0, for ROMB and RAMB */
    gsu_romb(0);
    gsu_ramb(0);
    g.r8 = gsu_ldw(FX_ARG_RECORD);
    gsu_codes(amount_start_code[copy] + 1, 1);
    g.r6 = gsu_ldw(FX_ARG_AMOUNT);
    gsu_codes(amount_start_code[copy] + 2, 1);
    g.r1 = gsu_ldw(gsu_add(field, g.r8, 0));
    g.r2 = scale_half;
    g.r0 = gsu_fmult(gsu_add(g.r2, g.r2, 0), g.r6);
    gsu_codes(amount_start_code[copy] + 3, 2);
    return g;
}

static void abi_amount(FxAmountRegs g)
{
    gsu.r[0] = g.r0;
    gsu.r[1] = g.r1;
    gsu.r[2] = g.r2;
    gsu.r[6] = g.r6;
    gsu.r[8] = g.r8;
}

/*
 * @routine 04:BF51 FxRecordRaise6A
 * WHAT IT DOES: A program: raises +$6A of the record whose address the
 * S-CPU left at $238E by the amount at $2390 times $300 / 65536, to at most
 * $0180.
 * HOW: ROMB, RAMB 0; FMULT of $0300 (R2 doubled) by the amount, ADD, and
 * the cap by CMP and the sign; SBK stores to the field's address, the last
 * loaded. The S-CPU side is GsuRecordRaise6A (object/gsu_record.c).
 * Entry: the S-CPU starts it. Exit: (at its STOP) R0 = the new +$6A; R1 =
 * the old; R2 = $0180; R6 = the amount; R8 = the record; ROMBR = RAMBR = 0;
 * OV, S, CY, Z from the CMP; the last RAM address R8 + $6A.
 */
FxAmountRegs FxRecordRaise6A(void)
{
    FxAmountRegs g = amount_start(REC_6A, 0x0180, GSU_COPY(0));

    g.r0 = gsu_add(g.r0, g.r1, 0);
    gsu_sub(g.r0, g.r2, 0);
    if (!gsu.s)
        { gsu_code(0x04BF70, 2); g.r0 = g.r2; }
    gsu_sbk(g.r0);
    return g;
}

void abi_FxRecordRaise6A(void)
{
    abi_amount(FxRecordRaise6A());
}

/*
 * @routine 04:BF75 FxRecordLower82
 * WHAT IT DOES: A program: lowers +$82 of the record whose address the
 * S-CPU left at $238E by the amount at $2390 times $400 / 65536, to no less
 * than 0.
 * HOW: ROMB, RAMB 0; FMULT of $0400 (R2 doubled) by the amount, SUB from
 * the field, and SUB R0 for 0 when negative; SBK stores to the field's
 * address, the last loaded. The S-CPU side is GsuRecordLower82
 * (object/gsu_record.c).
 * Entry: the S-CPU starts it. Exit: (at its STOP) R0 = the new +$82; R1 =
 * the old; R2 = $0200; R6 = the amount; R8 = the record; ROMBR = RAMBR = 0;
 * OV, S, CY, Z from the last SUB; the last RAM address R8 + $82.
 */
FxAmountRegs FxRecordLower82(void)
{
    FxAmountRegs g = amount_start(REC_82, 0x0200, GSU_COPY(1));

    g.r0 = gsu_sub(g.r1, g.r0, 0);
    if (gsu.s)
        g.r0 = gsu_sub(g.r0, g.r0, 0);
    gsu_sbk(g.r0);
    return g;
}

void abi_FxRecordLower82(void)
{
    abi_amount(FxRecordLower82());
}

/*
 * @routine 01:BBC7 FxTimersProgram
 * WHAT IT DOES: The Super FX program that steps the 20 timed records at
 * $273E (14 bytes each): a record whose timer byte (+0) is set counts it
 * down; while its flags (+1) have bit 4 and the timer is below $20, +2
 * takes entry (timer) of the byte table at $14:D0BC (ROM); then a record
 * with bit 0 of +1 has it cleared, any other ends: its timer is cleared
 * and its word +$0C set to 0 (also for a record whose timer was already
 * 0).
 * HOW: ROM bank $14, RAM bank 0; CACHE and LOOP over R12 = 20 records, R9
 * stepping by R8 = 14; WITH then TO is a MOVE; the BNE's delay slot
 * clears R0 either way.
 * Entry: started by the S-CPU. Exit: (at the STOP) R9 past the records;
 * R12 = 0; R10 = $0240; the other registers as the last record left them.
 */
void abi_FxTimersProgram(void)
{
    gsu_code(0x01BBC8, 21); gsu_cache(0xBBDD); gsu_code(0x01BBDD, 2);
    int bit0;

    gsu.r[10] = 0x0240;
    gsu.r[0] = 0x0014;
    gsu_romb(0x14);
    gsu.r[0] = gsu_sub(gsu.r[0], gsu.r[0], 0);
    gsu_ramb(0x00);
    gsu.r[8] = 0x000E;
    gsu.r[9] = 0x273E;
    gsu.r[12] = 0x0014;
    gsu.r[5] = 0x0020;
    gsu.r[13] = 0xBBDF;
    do {
        gsu_code(0x01BBDF, 3);
        gsu.r[0] = gsu_lob(gsu_ldb(gsu.r[9]));
        if (!gsu.z) {
            gsu_code(0x01BBE2, 7);
            gsu.r[0] = gsu_dec(gsu.r[0]);
            gsu_stb(gsu.r[9], (uint8_t)gsu.r[0]);
            gsu.r[7] = gsu.r[0];
            gsu.r[0] = gsu_add(0x0001, gsu.r[9], 0);
            gsu_code(0x01BBE9, 7);
            gsu.r[0] = gsu_ldb(gsu.r[0]);
            gsu.r[6] = 0x0010;
            gsu.r[0] = gsu_and(gsu.r[0], gsu.r[6]);
            if (!gsu.z) {
                gsu_code(0x01BBF0, 16);
                gsu.r[0] = gsu.r[7];
                gsu_sub(gsu.r[0], gsu.r[5], 0);
                if (gsu.s) {
                    gsu_r14(0xD0BC);
                    gsu_code(0x01BC00, 2);
                    gsu_r14(gsu_add(gsu.r[14], gsu.r[0], 0));
                    gsu.r[0] = (gsu_code(0x01BC02, 1), gsu_getb());
                    gsu.r[7] = gsu_add(0x0002, gsu.r[9], 0);
                    gsu_code(0x01BC03, 7);
                    gsu_stb(gsu.r[7], (uint8_t)gsu.r[0]);
                }
            } else { gsu_code(0x01BBF0, 5); gsu_code(0x01BC09, 1); }
            gsu_code(0x01BC0A, 5);
            gsu.r[0] = gsu_add(0x0001, gsu.r[9], 0);
            gsu.r[0] = gsu_ldb(gsu.r[0]);
            gsu.r[6] = gsu.r[0];
            gsu.r[0] = gsu_and(gsu.r[0], 0x0001);
            bit0 = !gsu.z;
            gsu.r[0] = gsu_sub(gsu.r[0], gsu.r[0], 0);
            if (bit0) {
                gsu_code(0x01BC0F, 6); gsu_code(0x01BC24, 12);
                gsu.r[0] = gsu.r[6];
                gsu.r[0] = gsu_bic(gsu.r[0], 0x0001);
                gsu.r[7] = gsu_add(0x0001, gsu.r[9], 0);
                gsu_stb(gsu.r[7], (uint8_t)gsu.r[0]);
                gsu.r[9] = gsu_add(gsu.r[9], gsu.r[8], 0);
                gsu.r[12] = gsu_dec(gsu.r[12]);
                continue;
            } else { gsu_code(0x01BC0F, 9); }
            gsu_stb(gsu.r[9], (uint8_t)gsu.r[0]);
        } else { gsu_code(0x01BBE2, 3); gsu_code(0x01BC17, 1); }
        gsu_code(0x01BC18, 6);
        gsu.r[7] = gsu_add(0x000C, gsu.r[9], 0);
        gsu_stw(gsu.r[7], gsu.r[0]);
        gsu.r[9] = gsu_add(gsu.r[9], gsu.r[8], 0);
        gsu.r[12] = gsu_dec(gsu.r[12]);
    } while (gsu.r[12] != 0);
    gsu_code(0x01BC33, 1);
}

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode fx_plot_code[7][3] = {
    {{0, 0}, {0, 0}, {0, 0}},
    {{0x01BB02, 6}, {0, 0}, {0x01BB08, 1}},
    {{0, 0}, {0, 0}, {0x01BB09, 4}},
    {{0, 0}, {0, 0}, {0x01BB0D, 1}},
    {{0, 0}, {0, 0}, {0x01BB0E, 6}},
    {{0x01BB9A, 3}, {0x01BBA0, 1}, {0x01BBA1, 4}},
    {{0x01BBB5, 3}, {0x01BBBB, 1}, {0x01BBBC, 4}},
};

/* PLOT: the pixel at (R1, R2), R1 moving on by one. */
static void fx_plot(int copy)
{
    gsu_codes(fx_plot_code[copy], 2);
    gsu_plot((uint8_t)gsu.r[1], (uint8_t)gsu.r[2]);
    gsu.r[1] = (uint16_t)(gsu.r[1] + 1);
    gsu_codes(fx_plot_code[copy] + 2, 1);
}

/* COLOR, with the plot options' nibble modes. */
static void fx_color(uint16_t v)
{
    gsu_code(0x01BAF8, 10);
    uint8_t c = (uint8_t)v;

    if (gsu.por & 0x04)
        c = (uint8_t)((gsu.colr & 0xF0) | c >> 4);
    else if (gsu.por & 0x08)
        c = (uint8_t)((gsu.colr & 0xF0) | (c & 0x0F));
    gsu.colr = c;
}

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode fx_scale_for_code[3][12] = {
    {{0, 0}, {0, 0}, {0, 0}, {0x01BAA8, 3}, {0x01BAB2, 3}, {0, 0}, {0x01BAB5, 4}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
    {{0x01BA9D, 11}, {0x01BAAB, 7}, {0x01BAB9, 20}, {0x01BAA8, 3}, {0x01BAB2, 3}, {0, 0}, {0x01BAB5, 4}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
    {{0x01BB22, 10}, {0x01BB2F, 7}, {0, 0}, {0x01BB2C, 3}, {0x01BB0D, 7}, {0x01BB36, 7}, {0, 0}, {0x01BB3D, 9}, {0x01BB46, 2}, {0x01BB48, 1}, {0x01BB49, 3}, {0x01BB4C, 5}},
};

/* R6 = z for the scale: clamped to $43FF; returns 0 (skip) when below
   $100. Then R14 = $896C + (z AND NOT 1) and R6 = the scale there. */
static int fx_scale_for(int copy)
{
    gsu_codes(fx_scale_for_code[copy], 3);
    gsu.r[0] = gsu_sub(0x4400, gsu.r[6], 0);
    if (gsu.s)
        { gsu_codes(fx_scale_for_code[copy] + 3, 1); gsu.r[6] = 0x43FF; }
    gsu.r[0] = gsu_sub(0x0100, gsu.r[6], 0);
    if (!gsu.s && (gsu_codes(fx_scale_for_code[copy] + 4, 2), !gsu.z))
        { gsu_codes(fx_scale_for_code[copy] + 6, 1); return 0; } else { gsu_codes(fx_scale_for_code[copy] + 7, 1); }
    gsu.r[6] = gsu_bic(gsu.r[6], 0x0001);
    gsu.r[0] = 0x896C;
    gsu_r14(gsu_add(gsu.r[0], gsu.r[6], 0));
    gsu.r[6] = (gsu_codes(fx_scale_for_code[copy] + 8, 1), gsu_getb());
    gsu_codes(fx_scale_for_code[copy] + 9, 1);
    gsu_r14(gsu_inc(gsu.r[14]));
    gsu_codes(fx_scale_for_code[copy] + 10, 1);
    gsu.r[6] = gsu_getbh(gsu.r[6]);
    gsu_codes(fx_scale_for_code[copy] + 11, 1);
    return 1;
}

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode fx_project_code[6][5] = {
    {{0x01BACD, 5}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
    {{0x01BAE1, 5}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
    {{0x01BACD, 5}, {0x01BAD2, 8}, {0x01BB0D, 7}, {0x01BAD2, 4}, {0x01BADA, 7}},
    {{0x01BAE1, 5}, {0x01BAE6, 8}, {0x01BB0D, 7}, {0x01BAE6, 4}, {0x01BAEE, 10}},
    {{0x01BB51, 5}, {0x01BB56, 8}, {0, 0}, {0x01BB56, 4}, {0x01BB5E, 7}},
    {{0x01BB65, 5}, {0x01BB6A, 8}, {0, 0}, {0x01BB6A, 4}, {0x01BB72, 5}},
};

/* Projects R[c]: FMULT by the scale, plus the centre at `centre` (into
   R5), then checks it against the bound at `bound` (into R[rb]; SUB, carry
   clear = inside). */
static int fx_project(int c, uint16_t centre, uint16_t bound, int rb, int copy)
{
    gsu.r[0] = gsu_fmult(gsu.r[c], gsu.r[6]);
    gsu.r[5] = gsu_ldw(centre);
    gsu.r[c] = gsu_add(gsu.r[0], gsu.r[5], 0);
    gsu_codes(fx_project_code[copy], 1);
    gsu.r[rb] = gsu_ldw(bound);
    gsu.r[0] = gsu_sub(gsu.r[c], gsu.r[rb], 0);
    if (gsu.cy)
        { gsu_codes(fx_project_code[copy] + 1, 2); return 0; } else { gsu_codes(fx_project_code[copy] + 3, 2); }
    gsu.r[0] = gsu_add(gsu.r[0], gsu.r[rb], 0);
    return 1;
}

/* A line from (R3, R9) to (R1, R2): Bresenham with PLOT in the branch's
   delay slot and the error step in the LOOP's. R12 is kept at $40. */
static void fx_line(void)
{
    gsu_stw(0x0040, gsu.r[12]);
    gsu.r[5] = 0x0001;
    gsu.r[3] = gsu_sub(gsu.r[3], gsu.r[1], 0);
    if (gsu.s) {
        gsu_code(0x01BB7D, 5);
        gsu.r[5] = 0xFFFF;
        gsu.r[3] = gsu_inc(gsu_not(gsu.r[3]));
    }
    gsu_code(0x01BB77, 6); gsu_code(0x01BB82, 7); gsu_code(0x01BB8E, 5);
    gsu.r[6] = 0x0001;
    gsu.r[9] = gsu_sub(gsu.r[9], gsu.r[2], 0);
    if (gsu.s) {
        gsu_code(0x01BB89, 5);
        gsu.r[6] = 0xFFFF;
        gsu.r[9] = gsu_inc(gsu_not(gsu.r[9]));
    }
    gsu.r[0] = gsu_sub(gsu.r[3], gsu.r[9], 0);
    if (!gsu.s) {
        gsu_code(0x01BB93, 7);
        gsu.r[12] = gsu_inc(gsu.r[3]);
        gsu.r[0] = gsu_lsr(gsu.r[0]);
        gsu.r[0] = gsu_sub(gsu.r[0], gsu.r[9], 0);
        gsu.r[13] = 0xBB9A;
        do {
            int cs = gsu.cy;

            fx_plot(GSU_COPY(5));
            if (!cs) {
                gsu_code(0x01BB9E, 2);
                gsu.r[0] = gsu_add(gsu.r[0], gsu.r[3], 0);
                gsu.r[2] = gsu_add(gsu.r[2], gsu.r[6], 0);
            }
            gsu.r[1] = gsu_dec(gsu.r[1]);
            gsu.r[1] = gsu_add(gsu.r[1], gsu.r[5], 0);
            gsu.r[12] = gsu_dec(gsu.r[12]);
            gsu.r[0] = gsu_sub(gsu.r[0], gsu.r[9], 0);
        } while (gsu.r[12] != 0);
        gsu_code(0x01BBA5, 4);
    } else {
        gsu_code(0x01BBAC, 9);
        gsu.r[12] = gsu_inc(gsu.r[9]);
        gsu.r[0] = gsu_inc(gsu_not(gsu.r[0]));
        gsu.r[0] = gsu_lsr(gsu.r[0]);
        gsu.r[0] = gsu_sub(gsu.r[0], gsu.r[3], 0);
        gsu.r[13] = 0xBBB5;
        do {
            int cs = gsu.cy;

            fx_plot(GSU_COPY(6));
            if (!cs) {
                gsu_code(0x01BBB9, 3);
                gsu.r[0] = gsu_add(gsu.r[0], gsu.r[9], 0);
                gsu.r[1] = gsu_add(gsu.r[1], gsu.r[5], 0);
            }
            gsu.r[1] = gsu_dec(gsu.r[1]);
            gsu.r[2] = gsu_add(gsu.r[2], gsu.r[6], 0);
            gsu.r[12] = gsu_dec(gsu.r[12]);
            gsu.r[0] = gsu_sub(gsu.r[0], gsu.r[3], 0);
        } while (gsu.r[12] != 0);
        gsu_code(0x01BBC0, 4);
    }
    gsu.r[12] = gsu_ldw(0x0040);
}

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode fx_particle_code[2][24] = {
    {{0x01BA26, 3}, {0x01BA35, 9}, {0x01BA76, 9}, {0x01BA7F, 10}, {0x01BA89, 11}, {0x01BA94, 9}, {0x01BA89, 5}, {0x01BA9C, 1}, {0x01BA9D, 4}, {0x01BAA1, 7}, {0x01BAAB, 7}, {0x01BB0D, 7}, {0x01BAB9, 20}, {0x01BAD2, 4}, {0x01BADA, 7}, {0x01BAE6, 8}, {0x01BB0D, 7}, {0x01BAE6, 4}, {0x01BAEE, 10}, {0x01BB02, 6}, {0x01BB08, 1}, {0x01BB09, 4}, {0x01BB0D, 1}, {0x01BB0E, 6}},
    {{0x01BA26, 3}, {0x01BA35, 9}, {0x01BA76, 9}, {0x01BA7F, 10}, {0x01BA89, 11}, {0x01BA94, 9}, {0x01BA89, 5}, {0x01BA9C, 5}, {0, 0}, {0x01BAA1, 7}, {0x01BAAB, 7}, {0x01BB0D, 7}, {0x01BAB9, 20}, {0x01BAD2, 4}, {0x01BADA, 7}, {0x01BAE6, 8}, {0x01BB0D, 7}, {0x01BAE6, 4}, {0x01BAEE, 10}, {0x01BB02, 6}, {0x01BB08, 5}, {0, 0}, {0x01BB0D, 7}, {0, 0}},
};

/* Particle R11 (its record): moved by its deltas, projected and drawn. */
static void fx_particle(int copy)
{
    gsu_codes(fx_particle_code[copy], 2);
    static const struct { uint8_t delta, word, at; } axis[3] = {{3, 6, 0x26}, {4, 8, 0x28}, {5, 0x0A, 0x2A}};
    static const int keep[3] = {3, 9, 13}, to[3] = {1, 2, 6}, rw[3] = {1, 2, 6};
    int i, big;

    for (i = 0; i < 3; i++) {
        gsu.r[0] = gsu_add(axis[i].delta, gsu.r[11], 0);
        gsu.r[0] = gsu_ldb(gsu.r[0]);
        gsu.r[0] = (uint16_t)(int16_t)(int8_t)gsu.r[0];
        gsu_sz(gsu.r[0]);
        gsu.r[rw[i]] = axis[i].word;
        gsu.r[rw[i]] = gsu_add(gsu.r[rw[i]], gsu.r[11], 0);
        gsu.r[rw[i]] = gsu_ldw(gsu.r[rw[i]]);
        gsu.r[keep[i]] = gsu.r[rw[i]];
        gsu.r[0] = gsu_add(gsu.r[0], gsu.r[rw[i]], 0);
        gsu_sbk(gsu.r[0]);
        gsu.r[4] = gsu_ldw(axis[i].at);
        gsu.r[to[i]] = gsu_add(gsu.r[0], gsu.r[4], 0);
        gsu_codes(fx_particle_code[copy] + 2, 1);
    }
    gsu.r[5] = gsu_add(0x0001, gsu.r[11], 0);
    gsu.r[5] = gsu_ldb(gsu.r[5]);
    gsu.r[0] = gsu_or(gsu.r[5], 0x0001);
    gsu.r[4] = gsu_add(0x0001, gsu.r[11], 0);
    gsu_codes(fx_particle_code[copy] + 3, 1);
    gsu_stb(gsu.r[4], (uint8_t)gsu.r[0]);
    gsu.r[0] = gsu_and(gsu.r[5], 0x0002);
    if (!gsu.z) {
        gsu_codes(fx_particle_code[copy] + 4, 1);
        gsu.r[0] = gsu_add(0x0004, gsu.r[11], 0);
        gsu.r[0] = gsu_ldb(gsu.r[0]);
        gsu.r[0] = gsu_add(gsu.r[0], 0x0001, 0);
        gsu.r[4] = gsu_add(0x0004, gsu.r[11], 0);
        gsu_codes(fx_particle_code[copy] + 5, 1);
        gsu_stb(gsu.r[4], (uint8_t)gsu.r[0]);
    } else { gsu_codes(fx_particle_code[copy] + 6, 2); }
    gsu_codes(fx_particle_code[copy] + 8, 1);
    gsu_moves(gsu.r[6]);
    if (gsu.s || (gsu_codes(fx_particle_code[copy] + 9, 2), !fx_scale_for(GSU_COPY(0))))
        { gsu_codes(fx_particle_code[copy] + 11, 1); return; } else { gsu_codes(fx_particle_code[copy] + 12, 1); }
    if (!fx_project(1, 0x0034, 0x003A, 4, GSU_COPY(0)) || (gsu_codes(fx_particle_code[copy] + 13, 2), !fx_project(2, 0x0036, 0x003E, 5, GSU_COPY(1))))
        { gsu_codes(fx_particle_code[copy] + 15, 2); return; } else { gsu_codes(fx_particle_code[copy] + 17, 2); }
    gsu.r[0] = gsu_sub(gsu.r[0], gsu.r[0], 0);
    gsu.por = (uint8_t)gsu.r[0];
    gsu.r[0] = gsu_add(0x0002, gsu.r[11], 0);
    gsu.r[0] = gsu_ldb(gsu.r[0]);
    fx_color(gsu.r[0]);
    gsu.r[0] = 0x0008;
    gsu.por = 0x08;
    gsu.r[0] = gsu_add(0x0001, gsu.r[11], 0);
    gsu.r[0] = gsu_ldb(gsu.r[0]);
    gsu.r[0] = gsu_and(gsu.r[0], 0x0004);
    big = !gsu.z;
    if (!big) {
        gsu_codes(fx_particle_code[copy] + 19, 1);
        fx_plot(GSU_COPY(0));
        gsu_codes(fx_particle_code[copy] + 20, 1);
        fx_plot(GSU_COPY(0));
        gsu.r[2] = gsu_inc(gsu.r[2]);
        gsu.r[1] = gsu_dec(gsu.r[1]);
        gsu.r[1] = gsu_dec(gsu.r[1]);
        gsu_codes(fx_particle_code[copy] + 21, 1);
        fx_plot(GSU_COPY(0));
        gsu_codes(fx_particle_code[copy] + 22, 1);
        fx_plot(GSU_COPY(0));
        gsu_codes(fx_particle_code[copy] + 23, 1);
        return;
    } else { gsu_code(0x01BB02, 4); gsu_code(0x01BB14, 4); }
    for (i = 0; i < 3; i++) {
        static const uint8_t at[3] = {0x26, 0x28, 0x2A};

        gsu.r[0] = gsu_ldw(at[i]);
        gsu.r[keep[i]] = gsu_add(gsu.r[0], gsu.r[keep[i]], 0);
    }
    gsu.r[6] = gsu.r[13];
    if (!fx_scale_for(GSU_COPY(2)))
        return;
    if (!fx_project(3, 0x0034, 0x003A, 4, GSU_COPY(4)) || !fx_project(9, 0x0036, 0x003E, 5, GSU_COPY(5)))
        { gsu_code(0x01BB0D, 7); return; }
    fx_line();
}

/*
 * @routine 01:BA13 FxParticlesDraw
 * WHAT IT DOES: Draws the particles of key R2: for each of the 20 records
 * at $273E (14 bytes) whose word +$0C is R2, moves it by its signed deltas
 * (+3, +4, +5 into the words +6, +8, +$0A), sets bit 0 of its flags (+1)
 * and, with flag bit 1, steps +4; then, when its z (plus $2A) is from
 * $100 up, projects it (the scale for z from the table at $14:896C,
 * FMULT, plus the centre $34/$36), and inside the bounds ($3A, $3E) plots
 * it in colour +2: a 2 x 2 dot, or with flag bit 2 a line from where it
 * was (projected the same way).
 * HOW: ROM bank $14; the view offsets at $26/$28/$2A; R12 counts the
 * records and is kept at $40 while a line is drawn; plot options 0 for
 * the colour, then 8. The line is not reached in the traces so far.
 * Entry: R2 = the key; R10 = stack; R11 = return address. Exit: R7 = the
 * key; R12 = 0; R11 past the last record + 12; the plot registers as the
 * last particle left them; S, Z from R10; the last RAM address R10.
 */
void abi_FxParticlesDraw(void)
{
    gsu_code(0x01BA14, 2);
    gsu_push(&gsu.r[10], gsu.r[11]);
    gsu.r[7] = gsu.r[2];
    gsu.r[0] = 0x0014;
    gsu_code(0x01BA16, 14); gsu_cache(0xBA24);
    gsu_romb(0x14);
    gsu.r[11] = 0x274A;
    gsu.r[12] = 0x0014;
    for (;;) {
        gsu_code(0x01BA24, 2);
        gsu.r[0] = gsu_ldw(gsu.r[11]);
        gsu.r[0] = gsu_sub(gsu.r[0], gsu.r[7], 0);
        if (gsu.z) {
            gsu.r[11] = gsu_sub(gsu.r[11], 0x000C, 0);
            fx_particle(GSU_COPY(0));
            gsu.r[11] = gsu_add(gsu.r[11], 0x000C, 0);
        } else { gsu_code(0x01BA26, 3); }
        gsu_code(0x01BA29, 7);
        gsu.r[11] = gsu_add(gsu.r[11], 0x000E, 0);
        gsu.r[12] = gsu_dec(gsu.r[12]);
        if (gsu.z)
            { gsu_code(0x01BA30, 5); break; }
    }
    gsu_pull(&gsu.r[10]);
}
