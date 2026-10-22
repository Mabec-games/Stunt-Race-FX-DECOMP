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
   and the shared tails (to their RTL): $3E; *moved (when asked) whether it
   moved; the add's overflow, for the routines' register exit (approach_v). */
static int approach_v;

static uint16_t approach(uint16_t target, int wide, uint16_t min, int shifts, int *moved)
{
    int m;
    uint16_t mask = wide ? 0xFFFF : 0x00FF, sign = wide ? 0x8000 : 0x0080;
    uint32_t imm = wide ? 24 : 16;
    uint16_t cur, d, s;
    int i;

    target &= mask;
    cyc(16);                                    /* to the CMP's read */
    cur = wide ? rd16(APPROACH_VALUE) : rd8(APPROACH_VALUE);
    m = target != cur;
    if (moved)
        *moved = m;
    if (!m) {
        cyc(64);                                /* BNE, JML, the tail's LDA */
        cur = wide ? rd16(APPROACH_VALUE) : rd8(APPROACH_VALUE);
        cyc(58);                                /* the rest of the tail */
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
    approach_v = (~(d ^ cur) & (d ^ s) & sign) != 0;
    cyc(16);                                    /* STA */
    if (wide)
        wr16(APPROACH_VALUE, s);
    else
        wr8(APPROACH_VALUE, (uint8_t)s);
    cyc(46);                                    /* BRL, the tail's LDA */
    s = wide ? rd16(APPROACH_VALUE) : rd8(APPROACH_VALUE);
    cyc(58);                                    /* the rest of the tail */
    return s;
}

/* The routine's register exit: N, Z from $3E; C clear when it moved, set
   when it was there; V from the add when it moved. */
static void approach_exit(uint16_t v, int wide, int moved)
{
    cpu.p = (uint8_t)(wide ? nz16(cpu.p, v) : nz8(cpu.p, (uint8_t)v));
    if (moved)
        cpu.p = (uint8_t)((cpu.p & ~(P_C | P_V)) | (approach_v ? P_V : 0));
    else
        cpu.p |= P_C;
}


/* A call by name from C that still works in registers: A in, the new $3E
   in A (its low byte for the byte forms), the flags as the routine leaves
   them. */
void approach_regs(uint16_t (*f)(uint16_t, int *))
{
    int moved;

    cpu.a = f(cpu.a, &moved);
    approach_exit(cpu.a, 1, moved);
}

void approach_regs_byte(uint8_t (*f)(uint8_t, int *))
{
    int moved;
    uint8_t v = f((uint8_t)cpu.a, &moved);

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    approach_exit(v, 0, moved);
}

/*
 * @routine 03:CDC1 ApproachHalfMin2
 * WHAT IT DOES: Moves $3E half of the way toward A, by at least 1.
 * HOW: The difference, at least 2 either way, halved once.
 * Entry: as ApproachQuarterMin4. Exit: as ApproachQuarterMin4.
 */
uint16_t ApproachHalfMin2(uint16_t target, int *moved)
{
    return approach(target, 1, 2, 1, moved);
}

void abi_ApproachHalfMin2(void)
{
    approach_regs(ApproachHalfMin2);
}

/*
 * @routine 03:CDF4 ApproachQuarterMin4
 * WHAT IT DOES: Moves $3E a quarter of the way toward A, by at least 1.
 * HOW: The difference, at least 4 either way, halved twice; the tails at
 * $03:D1AA / $03:D1AE.
 * Entry: M 16-bit, D = $0000; A = target. Exit: A = $3E; N, Z from it; C
 * clear when it moved (V from the add), set when it was already there.
 */
uint16_t ApproachQuarterMin4(uint16_t target, int *moved)
{
    cyc(62);                                    /* JSL */
    return approach(target, 1, 4, 2, moved);
}

void abi_ApproachQuarterMin4(void)
{
    approach_regs(ApproachQuarterMin4);
}

/*
 * @routine 03:CE30 ApproachEighthMin8
 * WHAT IT DOES: Moves $3E an eighth of the way toward A, by at least 1.
 * HOW: The difference, at least 8 either way, halved three times.
 * Entry: as ApproachQuarterMin4. Exit: as ApproachQuarterMin4.
 */
uint16_t ApproachEighthMin8(uint16_t target, int *moved)
{
    cyc(48);
    return approach(target, 1, 8, 3, moved);
}

void abi_ApproachEighthMin8(void)
{
    approach_regs(ApproachEighthMin8);
}

/*
 * @routine 03:CE75 ApproachSixteenthMin16
 * WHAT IT DOES: Moves $3E a sixteenth of the way toward A, by at least 1.
 * HOW: The difference, at least $10 either way, halved four times.
 * Entry: as ApproachQuarterMin4. Exit: as ApproachQuarterMin4.
 */
uint16_t ApproachSixteenthMin16(uint16_t target, int *moved)
{
    return approach(target, 1, 16, 4, moved);
}

void abi_ApproachSixteenthMin16(void)
{
    approach_regs(ApproachSixteenthMin16);
}

/*
 * @routine 03:CEC3 ApproachThirtySecondMin32
 * WHAT IT DOES: Moves $3E a thirty-second of the way toward A, by at least 1.
 * HOW: The difference, at least $20 either way, halved five times.
 * Entry: as ApproachQuarterMin4. Exit: as ApproachQuarterMin4.
 */
uint16_t ApproachThirtySecondMin32(uint16_t target, int *moved)
{
    return approach(target, 1, 32, 5, moved);
}

void abi_ApproachThirtySecondMin32(void)
{
    approach_regs(ApproachThirtySecondMin32);
}

/*
 * @routine 03:CF1A ApproachSixtyFourthMin64
 * WHAT IT DOES: Moves $3E a sixty-fourth of the way toward A, by at least 1.
 * HOW: The difference, at least $40 either way, halved six times.
 * Entry: as ApproachQuarterMin4. Exit: as ApproachQuarterMin4.
 */
uint16_t ApproachSixtyFourthMin64(uint16_t target, int *moved)
{
    return approach(target, 1, 64, 6, moved);
}

void abi_ApproachSixtyFourthMin64(void)
{
    approach_regs(ApproachSixtyFourthMin64);
}

/*
 * @routine 03:CF7A ApproachOneTwentyEighthMin128
 * WHAT IT DOES: Moves $3E a 128th of the way toward A, by at least 1.
 * HOW: The difference, at least $80 either way, halved seven times.
 * Entry: as ApproachQuarterMin4. Exit: as ApproachQuarterMin4.
 */
uint16_t ApproachOneTwentyEighthMin128(uint16_t target, int *moved)
{
    return approach(target, 1, 128, 7, moved);
}

void abi_ApproachOneTwentyEighthMin128(void)
{
    approach_regs(ApproachOneTwentyEighthMin128);
}

/*
 * @routine 03:CFE3 ApproachHalfMin2Byte
 * WHAT IT DOES: Moves the byte at $3E half of the way toward A, by at least 1.
 * HOW: The difference, at least 2 either way, halved once, with 8-bit arithmetic. Only the arrived case is reached in the traces so far.
 * Entry: M 8-bit, D = $0000; A = target. Exit: as ApproachQuarterMin4 (A low byte).
 */
uint8_t ApproachHalfMin2Byte(uint8_t target, int *moved)
{
    return (uint8_t)approach(target, 0, 2, 1, moved);
}

void abi_ApproachHalfMin2Byte(void)
{
    approach_regs_byte(ApproachHalfMin2Byte);
}

/*
 * @routine 03:D042 ApproachEighthMin8Byte
 * WHAT IT DOES: Moves the byte at $3E an eighth of the way toward A, by at least 1.
 * HOW: The difference, at least 8 either way, halved three times, with 8-bit arithmetic.
 * Entry: M 8-bit, D = $0000; A = target. Exit: as ApproachQuarterMin4 (A low byte).
 */
uint8_t ApproachEighthMin8Byte(uint8_t target, int *moved)
{
    return (uint8_t)approach(target, 0, 8, 3, moved);
}

void abi_ApproachEighthMin8Byte(void)
{
    approach_regs_byte(ApproachEighthMin8Byte);
}

/*
 * @routine 03:D00F ApproachQuarterMin4Byte
 * WHAT IT DOES: The 8-bit form of ApproachQuarterMin4, on the byte at $3E.
 * HOW: As ApproachQuarterMin4, with 8-bit arithmetic.
 * Entry: M 8-bit, D = $0000; A = target. Exit: as ApproachQuarterMin4 (A
 * low byte).
 */
uint8_t ApproachQuarterMin4Byte(uint8_t target, int *moved)
{
    return (uint8_t)approach(target, 0, 4, 2, moved);
}

void abi_ApproachQuarterMin4Byte(void)
{
    approach_regs_byte(ApproachQuarterMin4Byte);
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
uint8_t ApproachSixteenthMin16Byte(uint8_t target, int *moved)
{
    return (uint8_t)approach(target, 0, 16, 4, moved);
}

void abi_ApproachSixteenthMin16Byte(void)
{
    approach_regs_byte(ApproachSixteenthMin16Byte);
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
uint8_t ApproachThirtySecondMin32Byte(uint8_t target, int *moved)
{
    return (uint8_t)approach(target, 0, 32, 5, moved);
}

void abi_ApproachThirtySecondMin32Byte(void)
{
    approach_regs_byte(ApproachThirtySecondMin32Byte);
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
uint8_t ApproachSixtyFourthMin64Byte(uint8_t target, int *moved)
{
    return (uint8_t)approach(target, 0, 64, 6, moved);
}

void abi_ApproachSixtyFourthMin64Byte(void)
{
    approach_regs_byte(ApproachSixtyFourthMin64Byte);
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
uint8_t ApproachOneTwentyEighthMin128Byte(uint8_t target, int *moved)
{
    return (uint8_t)approach(target, 0, 128, 7, moved);
}

void abi_ApproachOneTwentyEighthMin128Byte(void)
{
    approach_regs_byte(ApproachOneTwentyEighthMin128Byte);
}
