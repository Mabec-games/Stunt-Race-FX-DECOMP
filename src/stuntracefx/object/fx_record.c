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
uint16_t FxPushSet(uint16_t amount)
{
    uint16_t on = gsu_sub(0, 0, 0);          /* SUB R0 from itself */

    gsu_moves(amount);
    if (!gsu.s)
        on = 1;
    gsu_stw(FX_PUSH, on);
    gsu_stw(FX_PUSH + 2, amount);
    gsu_stw(FX_PUSH + 4, 0);
    gsu_stw(FX_PUSH + 8, 0);
    gsu_stw(FX_PUSH + 6, 0x8100);
    return on;
}

void abi_FxPushSet(void)
{
    gsu.r[0] = FxPushSet(gsu.r[2]);
    gsu.r[2] = 0x8100;
}

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
void FxListClearBit0(uint16_t owner, int *any)
{
    uint16_t rec = gsu_ldw(gsu_add(REC_LIST, owner, 0));

    gsu_moves(rec);
    *any = !gsu.z;
    while (!gsu.z) {
        gsu_sbk(gsu_bic(gsu_ldw(gsu_add(REC_FLAGS10, rec, 0)), 1));
        rec = gsu_ldw(gsu_add(REC_NEXT, rec, 0));
        gsu_moves(rec);
    }
}

void abi_FxListClearBit0(void)
{
    int any;

    FxListClearBit0(gsu.r[6], &any);
    gsu.r[8] = 0;
    if (any) {
        gsu.r[0] = 0;
        gsu.r[12] = 1;
    }
}

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
uint16_t FxRecordHoldY(uint16_t rec, uint16_t *y_out)
{
    uint16_t y, held;

    gsu_and(gsu_ldw(gsu_add(REC_FLAGS40, rec, 0)), 0x0100);
    if (gsu.z) {
        y = gsu_ldw(gsu_add(REC_Y, rec, 0));
        gsu_stw(gsu_add(REC_Y_HELD, rec, 0), y);
    }
    y = gsu_ldw(gsu_add(REC_Y, rec, 0));
    held = gsu_ldw(gsu_add(REC_Y_HELD, rec, 0));
    gsu_sub(held, y, 0);
    if (gsu.s) {
        held = gsu_sub(held, held, 0);
        gsu_sbk(held);
    }
    *y_out = y;
    return held;
}

void abi_FxRecordHoldY(void)
{
    uint16_t y;

    gsu.r[0] = FxRecordHoldY(gsu.r[8], &y);
    gsu.r[1] = y;
}

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
FxRegs FxBearingTo(FxRegs g)
{
    gsu_stw(g.r[10], g.r[11]);
    g.r[10] = gsu_inc(gsu_inc(g.r[10]));
    g.r[0] = gsu_ldw(gsu_add(REC_X, g.r[8], 0));
    g.r[1] = gsu_sub(g.r[1], g.r[0], 0);
    g.r[0] = gsu_ldw(gsu_add(REC_Z, g.r[8], 0));
    g.r[3] = gsu_sub(g.r[3], g.r[0], 0);
    g.r[2] = g.r[3];
    g.r[11] = 0xCBFF;
    g = FxAngleOf(g);
    g.r[0] = gsu_inc(gsu_not(g.r[0]));
    g.r[10] = gsu_dec(gsu_dec(g.r[10]));
    gsu_ldw(g.r[10]);
    return g;
}

void abi_FxBearingTo(void)
{
    FxRegs g;

    memcpy(g.r, gsu.r, sizeof g.r);
    g = FxBearingTo(g);
    memcpy(gsu.r, g.r, 15 * sizeof g.r[0]);
}

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
FxRegs FxRecordAlignment(FxRegs g)
{
    uint32_t p;
    uint16_t rec = g.r[8];
    int i;

    gsu_stw(g.r[10], g.r[11]);
    g.r[10] = gsu_inc(gsu_inc(g.r[10]));
    gsu_stw(g.r[10], g.r[8]);
    g.r[10] = gsu_inc(gsu_inc(g.r[10]));
    for (i = 0; i < 3; i++)
        g.r[1 + i] = gsu_ldw(gsu_add((uint16_t)(REC_VEL + 2 * i), rec, 0));
    g.r[11] = 0xBCD9;
    g = FxNormalize(g);
    for (i = 0; i < 3; i++) {
        g.r[6] = gsu_ldw((uint16_t)(FX_MATRIX_Z + 2 * i));
        p = gsu_lmult(g.r[1 + i], g.r[6]);
        g.r[4] = (uint16_t)p;
        if (i == 0) {
            g.r[0] = (uint16_t)(p >> 16);
            g.r[7] = g.r[4];
        } else {
            g.r[5] = (uint16_t)(p >> 16);
            g.r[7] = gsu_add(g.r[7], g.r[4], 0);
            g.r[0] = gsu_add(g.r[0], g.r[5], gsu.cy);
        }
    }
    g.r[7] = gsu_add(g.r[7], g.r[7], 0);
    g.r[1] = g.r[0] = gsu_rol(g.r[0]);
    g.r[10] = gsu_dec(gsu_dec(g.r[10]));
    g.r[8] = gsu_ldw(g.r[10]);
    g.r[0] = gsu_ldw(gsu_add(REC_SPEED, g.r[8], 0));
    gsu_moves(g.r[0]);
    if (gsu.s)
        g.r[0] = gsu_inc(gsu_not(g.r[0]));
    g.r[2] = 10;
    gsu_sub(g.r[0], g.r[2], 0);
    if (gsu.s)
        g.r[1] = 0x7FFF;
    g.r[0] = gsu_add(REC_ALIGN, g.r[8], 0);
    gsu_stw(g.r[0], g.r[1]);
    g.r[10] = gsu_dec(gsu_dec(g.r[10]));
    gsu_ldw(g.r[10]);
    return g;
}

void abi_FxRecordAlignment(void)
{
    FxRegs g;

    memcpy(g.r, gsu.r, sizeof g.r);
    g = FxRecordAlignment(g);
    memcpy(gsu.r, g.r, 15 * sizeof g.r[0]);
}

typedef struct {
    uint16_t r0, r1, r2, r6, r8;
} FxAmountRegs;

/* The banks to 0 and the record and amount the S-CPU left; the field's
   address in the record is loaded last, so SBK stores there. */
static FxAmountRegs amount_start(uint16_t field, uint16_t scale_half)
{
    FxAmountRegs g;

    g.r0 = gsu_sub(0, 0, 0);               /* SUB R0, for ROMB and RAMB */
    gsu.rombr = 0;
    gsu.rambr = 0;
    g.r8 = gsu_ldw(FX_ARG_RECORD);
    g.r6 = gsu_ldw(FX_ARG_AMOUNT);
    g.r1 = gsu_ldw(gsu_add(field, g.r8, 0));
    g.r2 = scale_half;
    g.r0 = gsu_fmult(gsu_add(g.r2, g.r2, 0), g.r6);
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
 * Entry: the S-CPU starts it. Exit (at its STOP): R0 = the new +$6A; R1 =
 * the old; R2 = $0180; R6 = the amount; R8 = the record; ROMBR = RAMBR = 0;
 * OV, S, CY, Z from the CMP; the last RAM address R8 + $6A.
 */
FxAmountRegs FxRecordRaise6A(void)
{
    FxAmountRegs g = amount_start(REC_6A, 0x0180);

    g.r0 = gsu_add(g.r0, g.r1, 0);
    gsu_sub(g.r0, g.r2, 0);
    if (!gsu.s)
        g.r0 = g.r2;
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
 * Entry: the S-CPU starts it. Exit (at its STOP): R0 = the new +$82; R1 =
 * the old; R2 = $0200; R6 = the amount; R8 = the record; ROMBR = RAMBR = 0;
 * OV, S, CY, Z from the last SUB; the last RAM address R8 + $82.
 */
FxAmountRegs FxRecordLower82(void)
{
    FxAmountRegs g = amount_start(REC_82, 0x0200);

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
 * Entry: started by the S-CPU. Exit (at the STOP): R9 past the records;
 * R12 = 0; R10 = $0240; the other registers as the last record left them.
 */
void abi_FxTimersProgram(void)
{
    int bit0;

    gsu.r[10] = 0x0240;
    gsu.r[0] = 0x0014;
    gsu.rombr = 0x14;
    gsu.r[0] = gsu_sub(gsu.r[0], gsu.r[0], 0);
    gsu.rambr = 0x00;
    gsu.r[8] = 0x000E;
    gsu.r[9] = 0x273E;
    gsu.r[12] = 0x0014;
    gsu.r[5] = 0x0020;
    gsu.r[13] = 0xBBDF;
    do {
        gsu.r[0] = gsu_lob(gsu_ldb(gsu.r[9]));
        if (!gsu.z) {
            gsu.r[0] = gsu_dec(gsu.r[0]);
            gsu_stb(gsu.r[9], (uint8_t)gsu.r[0]);
            gsu.r[7] = gsu.r[0];
            gsu.r[0] = gsu_add(0x0001, gsu.r[9], 0);
            gsu.r[0] = gsu_ldb(gsu.r[0]);
            gsu.r[6] = 0x0010;
            gsu.r[0] = gsu_and(gsu.r[0], gsu.r[6]);
            if (!gsu.z) {
                gsu.r[0] = gsu.r[7];
                gsu_sub(gsu.r[0], gsu.r[5], 0);
                if (gsu.s) {
                    gsu_r14(0xD0BC);
                    gsu_r14(gsu_add(gsu.r[14], gsu.r[0], 0));
                    gsu.r[0] = gsu_getb();
                    gsu.r[7] = gsu_add(0x0002, gsu.r[9], 0);
                    gsu_stb(gsu.r[7], (uint8_t)gsu.r[0]);
                }
            }
            gsu.r[0] = gsu_add(0x0001, gsu.r[9], 0);
            gsu.r[0] = gsu_ldb(gsu.r[0]);
            gsu.r[6] = gsu.r[0];
            gsu.r[0] = gsu_and(gsu.r[0], 0x0001);
            bit0 = !gsu.z;
            gsu.r[0] = gsu_sub(gsu.r[0], gsu.r[0], 0);
            if (bit0) {
                gsu.r[0] = gsu.r[6];
                gsu.r[0] = gsu_bic(gsu.r[0], 0x0001);
                gsu.r[7] = gsu_add(0x0001, gsu.r[9], 0);
                gsu_stb(gsu.r[7], (uint8_t)gsu.r[0]);
                gsu.r[9] = gsu_add(gsu.r[9], gsu.r[8], 0);
                gsu.r[12] = gsu_dec(gsu.r[12]);
                continue;
            }
            gsu_stb(gsu.r[9], (uint8_t)gsu.r[0]);
        }
        gsu.r[7] = gsu_add(0x000C, gsu.r[9], 0);
        gsu_stw(gsu.r[7], gsu.r[0]);
        gsu.r[9] = gsu_add(gsu.r[9], gsu.r[8], 0);
        gsu.r[12] = gsu_dec(gsu.r[12]);
    } while (gsu.r[12] != 0);
}
