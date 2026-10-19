/*
 * approach.c: moving the value in direct page $3E toward a target. The
 * step is the difference, raised to a minimum size, then divided by a power
 * of two (each halving rounds toward zero). Every routine ends in one of two
 * shared tails: $03:D1AA returns $3E with carry clear (it moved), $03:D1AE
 * returns it with carry set (it was already there).
 */
#include "machine.h"

#define APPROACH_VALUE 0x00003Eu

/* The time is the assembly's, every routine the same shape (master
   clocks: a ROM byte 8, an internal cycle 6). I is an immediate
   instruction's (CMP #, LDA #, ADC #): 24 with a 16-bit A, 16 with 8. */

/* One halving as CMP #$8000 (or #$80) / ROR A / BPL / ADC #0 does it: an
   arithmetic shift whose negative results round toward zero. CMP, ROR,
   then BPL taken (I + 36), or not and the ADC (2I + 30). */
static uint16_t halve(uint16_t v, uint16_t sign, uint32_t imm)
{
    uint16_t r = (uint16_t)((v >> 1) | (v & sign));

    if (r & sign) {
        cyc(2 * imm + 30);
        r = (uint16_t)(r + (v & 1));
    } else {
        cyc(imm + 36);
    }
    return r;
}

/* The step as each routine makes it, reading and writing $3E as often and
   in the order the routine does (CMP, SBC, ADC, STA, then the tail's LDA),
   and the shared tails: A = $3E; N, Z from it; C clear when it moved; when
   it moved, V from the add of the step. */
static uint16_t approach(uint16_t target, int wide, uint16_t min, int shifts, int *moved, uint8_t *p)
{
    uint16_t mask = wide ? 0xFFFF : 0x00FF, sign = wide ? 0x8000 : 0x0080;
    uint32_t imm = wide ? 24 : 16;
    uint16_t cur, d, s;
    int i;

    target &= mask;
    cyc(wide ? 48 : 16);                        /* to the CMP's read */
    cur = wide ? rd16(APPROACH_VALUE) : rd8(APPROACH_VALUE);
    *moved = target != cur;
    if (!*moved) {
        cyc(64);                                /* BNE, JML, the tail's LDA */
        cur = wide ? rd16(APPROACH_VALUE) : rd8(APPROACH_VALUE);
        *p = (uint8_t)(wide ? nz16(*p, cur) : nz8(*p, (uint8_t)cur)) | P_C;
        return cur;
    }
    cyc(52);                                    /* BNE taken, SEC, SBC */
    cur = wide ? rd16(APPROACH_VALUE) : rd8(APPROACH_VALUE);
    d = (uint16_t)(target - cur) & mask;
    if (!(d & sign)) {
        if (d < min) {
            cyc(3 * imm + 54);                  /* CMP, BMI, CMP, BPL, LDA #, BRA */
            d = min;
        } else {
            cyc(2 * imm + 38);                  /* CMP, BMI, CMP, BPL taken */
        }
    } else if (!((uint16_t)(d + min) & sign)) {
        cyc(3 * imm + 38);                      /* CMP, BMI taken, CMP, BMI, LDA # */
        d = (uint16_t)(-min) & mask;
    } else {
        cyc(2 * imm + 44);                      /* CMP, BMI taken, CMP, BMI taken */
    }
    for (i = 0; i < shifts; i++)
        d = halve(d, sign, imm) & mask;
    cyc(30);                                    /* CLC, ADC */
    cur = wide ? rd16(APPROACH_VALUE) : rd8(APPROACH_VALUE);
    s = (uint16_t)(d + cur) & mask;
    *p = (uint8_t)((*p & ~(P_C | P_V)) | ((~(d ^ cur) & (d ^ s) & sign) ? P_V : 0));
    cyc(16);                                    /* STA */
    if (wide)
        wr16(APPROACH_VALUE, s);
    else
        wr8(APPROACH_VALUE, (uint8_t)s);
    cyc(46);                                    /* BRL, the tail's LDA */
    s = wide ? rd16(APPROACH_VALUE) : rd8(APPROACH_VALUE);
    *p = wide ? nz16(*p, s) : nz8(*p, (uint8_t)s);
    return s;
}

static void abi_approach(int wide, uint16_t min, int shifts)
{
    int moved;
    uint8_t p = cpu.p;
    uint16_t v = approach(cpu.a, wide, min, shifts, &moved, &p);

    cpu.a = wide ? (cyc(58), v) : (cyc(58), (uint16_t)((cpu.a & 0xFF00) | v));
    cpu.p = p;
}

/*
 * @routine 03:CDC1 ApproachHalfMin2
 * WHAT IT DOES: Moves $3E half of the way toward A, by at least 1.
 * HOW: The difference, at least 2 either way, halved once.
 * Entry: as ApproachQuarterMin4. Exit: as ApproachQuarterMin4.
 */
uint16_t ApproachHalfMin2(uint16_t target, int *moved, uint8_t *p)
{
    return approach(target, 1, 2, 1, moved, p);
}

void abi_ApproachHalfMin2(void)
{
    abi_approach(1, 2, 1);
}

/*
 * @routine 03:CDF4 ApproachQuarterMin4
 * WHAT IT DOES: Moves $3E a quarter of the way toward A, by at least 1.
 * HOW: The difference, at least 4 either way, halved twice; the tails at
 * $03:D1AA / $03:D1AE.
 * Entry: M 16-bit, D = $0000; A = target. Exit: A = $3E; N, Z from it; C
 * clear when it moved (V from the add), set when it was already there.
 */
uint16_t ApproachQuarterMin4(uint16_t target, int *moved, uint8_t *p)
{
    return approach(target, 1, 4, 2, moved, p);
}

void abi_ApproachQuarterMin4(void)
{
    cyc(30);
    abi_approach(1, 4, 2);
}

/*
 * @routine 03:CE30 ApproachEighthMin8
 * WHAT IT DOES: Moves $3E an eighth of the way toward A, by at least 1.
 * HOW: The difference, at least 8 either way, halved three times.
 * Entry: as ApproachQuarterMin4. Exit: as ApproachQuarterMin4.
 */
uint16_t ApproachEighthMin8(uint16_t target, int *moved, uint8_t *p)
{
    return approach(target, 1, 8, 3, moved, p);
}

void abi_ApproachEighthMin8(void)
{
    cyc(16);
    abi_approach(1, 8, 3);
}

/*
 * @routine 03:CE75 ApproachSixteenthMin16
 * WHAT IT DOES: Moves $3E a sixteenth of the way toward A, by at least 1.
 * HOW: The difference, at least $10 either way, halved four times.
 * Entry: as ApproachQuarterMin4. Exit: as ApproachQuarterMin4.
 */
uint16_t ApproachSixteenthMin16(uint16_t target, int *moved, uint8_t *p)
{
    return approach(target, 1, 16, 4, moved, p);
}

void abi_ApproachSixteenthMin16(void)
{
    abi_approach(1, 16, 4);
}

/*
 * @routine 03:CEC3 ApproachThirtySecondMin32
 * WHAT IT DOES: Moves $3E a thirty-second of the way toward A, by at least 1.
 * HOW: The difference, at least $20 either way, halved five times.
 * Entry: as ApproachQuarterMin4. Exit: as ApproachQuarterMin4.
 */
uint16_t ApproachThirtySecondMin32(uint16_t target, int *moved, uint8_t *p)
{
    return approach(target, 1, 32, 5, moved, p);
}

void abi_ApproachThirtySecondMin32(void)
{
    abi_approach(1, 32, 5);
}

/*
 * @routine 03:CF1A ApproachSixtyFourthMin64
 * WHAT IT DOES: Moves $3E a sixty-fourth of the way toward A, by at least 1.
 * HOW: The difference, at least $40 either way, halved six times.
 * Entry: as ApproachQuarterMin4. Exit: as ApproachQuarterMin4.
 */
uint16_t ApproachSixtyFourthMin64(uint16_t target, int *moved, uint8_t *p)
{
    return approach(target, 1, 64, 6, moved, p);
}

void abi_ApproachSixtyFourthMin64(void)
{
    abi_approach(1, 64, 6);
}

/*
 * @routine 03:CF7A ApproachOneTwentyEighthMin128
 * WHAT IT DOES: Moves $3E a 128th of the way toward A, by at least 1.
 * HOW: The difference, at least $80 either way, halved seven times.
 * Entry: as ApproachQuarterMin4. Exit: as ApproachQuarterMin4.
 */
uint16_t ApproachOneTwentyEighthMin128(uint16_t target, int *moved, uint8_t *p)
{
    return approach(target, 1, 128, 7, moved, p);
}

void abi_ApproachOneTwentyEighthMin128(void)
{
    abi_approach(1, 128, 7);
}

/*
 * @routine 03:CFE3 ApproachHalfMin2Byte
 * WHAT IT DOES: Moves the byte at $3E half of the way toward A, by at least 1.
 * HOW: The difference, at least 2 either way, halved once, with 8-bit arithmetic. Only the arrived case is reached in the traces so far.
 * Entry: M 8-bit, D = $0000; A = target. Exit: as ApproachQuarterMin4 (A low byte).
 */
uint16_t ApproachHalfMin2Byte(uint8_t target, int *moved, uint8_t *p)
{
    return approach(target, 0, 2, 1, moved, p);
}

void abi_ApproachHalfMin2Byte(void)
{
    abi_approach(0, 2, 1);
}

/*
 * @routine 03:D042 ApproachEighthMin8Byte
 * WHAT IT DOES: Moves the byte at $3E an eighth of the way toward A, by at least 1.
 * HOW: The difference, at least 8 either way, halved three times, with 8-bit arithmetic.
 * Entry: M 8-bit, D = $0000; A = target. Exit: as ApproachQuarterMin4 (A low byte).
 */
uint16_t ApproachEighthMin8Byte(uint8_t target, int *moved, uint8_t *p)
{
    return approach(target, 0, 8, 3, moved, p);
}

void abi_ApproachEighthMin8Byte(void)
{
    abi_approach(0, 8, 3);
}

/*
 * @routine 03:D00F ApproachQuarterMin4Byte
 * WHAT IT DOES: The 8-bit form of ApproachQuarterMin4, on the byte at $3E.
 * HOW: As ApproachQuarterMin4, with 8-bit arithmetic.
 * Entry: M 8-bit, D = $0000; A = target. Exit: as ApproachQuarterMin4 (A
 * low byte).
 */
uint16_t ApproachQuarterMin4Byte(uint8_t target, int *moved, uint8_t *p)
{
    return approach(target, 0, 4, 2, moved, p);
}

void abi_ApproachQuarterMin4Byte(void)
{
    cyc(58);
    abi_approach(0, 4, 2);
}

/*
 * @routine 03:D07C ApproachSixteenthMin16Byte
 * WHAT IT DOES: Moves the byte at $3E a sixteenth of the way toward A, by at
 * least 1.
 * HOW: The difference, at least $10 either way, halved four times, with
 * 8-bit arithmetic.
 * Entry: M 8-bit, D = $0000; A = target. Exit: as ApproachQuarterMin4 (A
 * low byte).
 */
uint16_t ApproachSixteenthMin16Byte(uint8_t target, int *moved, uint8_t *p)
{
    return approach(target, 0, 16, 4, moved, p);
}

void abi_ApproachSixteenthMin16Byte(void)
{
    abi_approach(0, 16, 4);
}

/*
 * @routine 03:D0BD ApproachThirtySecondMin32Byte
 * WHAT IT DOES: Moves the byte at $3E a thirty-second of the way toward A, by
 * at least 1.
 * HOW: The difference, at least $20 either way, halved five times, with
 * 8-bit arithmetic.
 * Entry: M 8-bit, D = $0000; A = target. Exit: as ApproachQuarterMin4 (A
 * low byte).
 */
uint16_t ApproachThirtySecondMin32Byte(uint8_t target, int *moved, uint8_t *p)
{
    return approach(target, 0, 32, 5, moved, p);
}

void abi_ApproachThirtySecondMin32Byte(void)
{
    abi_approach(0, 32, 5);
}

/*
 * @routine 03:D105 ApproachSixtyFourthMin64Byte
 * WHAT IT DOES: Moves the byte at $3E a sixty-fourth of the way toward A, by
 * at least 1.
 * HOW: The difference, at least $40 either way, halved six times, with
 * 8-bit arithmetic.
 * Entry: M 8-bit, D = $0000; A = target. Exit: as ApproachQuarterMin4 (A
 * low byte).
 */
uint16_t ApproachSixtyFourthMin64Byte(uint8_t target, int *moved, uint8_t *p)
{
    return approach(target, 0, 64, 6, moved, p);
}

void abi_ApproachSixtyFourthMin64Byte(void)
{
    abi_approach(0, 64, 6);
}

/*
 * @routine 03:D154 ApproachOneTwentyEighthMin128Byte
 * WHAT IT DOES: Meant to move the byte at $3E a 128th of the way toward A;
 * unless it is already there, it always steps it down by 1.
 * HOW: The difference is raised to $80 either way, and $80 is -128 as a
 * byte: halved seven times it is -1 whichever side the target is on.
 * Entry: M 8-bit, D = $0000; A = target. Exit: as ApproachQuarterMin4 (A
 * low byte).
 */
uint16_t ApproachOneTwentyEighthMin128Byte(uint8_t target, int *moved, uint8_t *p)
{
    return approach(target, 0, 128, 7, moved, p);
}

void abi_ApproachOneTwentyEighthMin128Byte(void)
{
    abi_approach(0, 128, 7);
}
