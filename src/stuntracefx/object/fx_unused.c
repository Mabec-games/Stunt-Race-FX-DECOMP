/*
 * fx_unused.c: Super FX code of bank 04 that nothing in the game calls or
 * starts: a checksum program over the object pools' RAM, an object-free
 * program and an object-new program.
 *
 * The C was drafted instruction for instruction from the ROM bytes and is
 * kept so: every register, flag and RAM access as the code makes them,
 * one per statement, a branch's delay slot after its decision. Returns
 * through registers go through fx_unused_run's dispatch on the address
 * they reach.
 */
#include "gsu.h"
#include "gsu_ops.h"

#define R gsu.r

void abi_FxPoolTake(void);
void abi_FxObjectFree(void);
void abi_FxRecordSegment(void);
void abi_FxCourseNextEntry(void);
void abi_FxPoolChecksum(void);

/* The code from address at until it jumps to ret (the caller's return;
   $FFFF for a program, which ends at its STOP). */
static void fx_unused_run(uint16_t at, uint16_t ret)
{
    int t;
    uint16_t nx;

    (void)t;
    (void)nx;
    goto dispatch;
l8915:
    R[10] = 0x0240;
    R[11] = 0x891D;
    abi_FxPoolChecksum();
l891D:
    gsu_stw(0x2686, R[0]);
    return;   /* the STOP: the program ends here */
l8923:
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu.rambr = (uint8_t)(R[0] & 1);
    R[1] = 0x1090;
    R[12] = 0x0AF9;
    /* CACHE */
    R[13] = 0x892F;
l892F:
    R[2] = gsu_ldw(R[1]);
    R[1] = gsu_inc(R[1]);
    R[1] = gsu_inc(R[1]);
    R[0] = gsu_xor(R[0], R[2]);
    t = gsu_count_down(&R[12]);
    if (t) goto l892F;
    nx = (uint16_t)(R[11]);
    at = nx;
    goto dispatch;
l89FA:
    R[10] = 0x0240;
    R[1] = gsu_ldw(0x238E);
    R[11] = 0x8A06;
    abi_FxObjectFree();
l8A06:
    return;   /* the STOP: the program ends here */
l8AD0:
    R[10] = 0x0240;
    R[4] = 0x0028;
    R[3] = 0x1098;
    R[11] = 0x8ADE;
    abi_FxPoolTake();
l8ADE:
    R[1] = R[1];
    gsu_moves(R[1]);
    t = gsu.z;
    if (t) goto l8B2C;
    R[2] = gsu_ldw(0x238E);
    R[0] = 0x001E;
    R[0] = gsu_add(R[0], R[1], 0);
    gsu_stw(R[0], R[2]);
    R[2] = gsu_ldw(0x2390);
    R[0] = 0x0020;
    R[0] = gsu_add(R[0], R[1], 0);
    gsu_stw(R[0], R[2]);
    R[2] = gsu_ldw(0x2392);
    R[0] = 0x0022;
    R[0] = gsu_add(R[0], R[1], 0);
    gsu_stw(R[0], R[2]);
    R[2] = 0x0016;
    R[2] = gsu_add(R[2], R[1], 0);
    R[2] = gsu_ldw(R[2]);
    R[0] = 0x00A0;
    R[2] = gsu_or(R[0], R[2]);
    R[0] = 0x0016;
    R[0] = gsu_add(R[0], R[1], 0);
    gsu_stw(R[0], R[2]);
    R[2] = 0xFFFF;
    R[0] = 0x001A;
    R[0] = gsu_add(R[0], R[1], 0);
    gsu_stw(R[0], R[2]);
    gsu_stw(0x238E, R[1]);
    R[8] = R[1];
    R[11] = 0x8B21;
    abi_FxRecordSegment();
l8B21:
    R[11] = 0x8B26;
    abi_FxCourseNextEntry();
l8B26:
    gsu_stw(0x2390, R[0]);
    return;   /* the STOP: the program ends here */
l8B2C:
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_stw(0x238E, R[0]);
    return;   /* the STOP: the program ends here */
dispatch:
    if (at == ret)
        return;
    switch (at) {
    case 0x8915: goto l8915;
    case 0x891D: goto l891D;
    case 0x8923: goto l8923;
    case 0x892F: goto l892F;
    case 0x89FA: goto l89FA;
    case 0x8A06: goto l8A06;
    case 0x8AD0: goto l8AD0;
    case 0x8ADE: goto l8ADE;
    case 0x8B21: goto l8B21;
    case 0x8B26: goto l8B26;
    case 0x8B2C: goto l8B2C;
    }
    return;   /* not an address the code jumps to */
}

/*
 * @routine 04:8923 FxPoolChecksum
 * WHAT IT DOES: The XOR of the $AF9 words of Super FX RAM from $1090
 * (the object pools) into R0. Only FxPoolChecksumProgram calls it.
 * HOW: RAM bank 0; a LOOP of LDW, INC R1 twice, XOR.
 * Entry: R11 = return address. Exit: R0 = the checksum, S and Z from it;
 * R1 = $2682; R2 = the last word; R12 = 0; the cache base at 04:8920.
 */
void abi_FxPoolChecksum(void)
{
    fx_unused_run(0x8923, R[11]);
}

/*
 * @routine 04:8915 FxPoolChecksumProgram
 * WHAT IT DOES: A program: the object pools' checksum (FxPoolChecksum)
 * into $2686, then STOP. Nothing in the game starts it.
 * HOW: R10 = $0240, LINK to FxPoolChecksum, SM.
 * Entry: run as a program. Exit: (at the STOP) as FxPoolChecksum leaves
 * the registers, R11 = $891D.
 */
void abi_FxPoolChecksumProgram(void)
{
    fx_unused_run(0x8915, 0xFFFF);
}

/*
 * @routine 04:89FA FxObjectFreeProgram
 * WHAT IT DOES: A program: frees the Super FX object at $238E
 * (FxObjectFree), then STOP. Nothing in the game starts it.
 * HOW: R10 = $0240, R1 = $238E, LINK to FxObjectFree.
 * Entry: run as a program; $238E = the object. Exit: (at the STOP) as
 * FxObjectFree leaves the registers.
 */
void abi_FxObjectFreeProgram(void)
{
    fx_unused_run(0x89FA, 0xFFFF);
}

/*
 * @routine 04:8AD0 FxObjectNewProgram
 * WHAT IT DOES: A program: takes an object of $28 bytes from the pool at
 * $1098 (FxPoolTake); none: $238E = 0. Otherwise it is placed at
 * ($238E, $2390, $2392) (+$1E/+$20/+$22), its +$16 gets bits $A0,
 * +$1A = $FFFF, $238E = it, then its course segment (FxRecordSegment)
 * and FxCourseNextEntry, whose R0 goes to $2390; then STOP. Nothing in
 * the game starts it.
 * HOW: R10 = $0240; the object in R1, then R8 for the calls.
 * Entry: run as a program; $238E-$2392 = the place. Exit: (at the STOP)
 * as the last call leaves the registers.
 */
void abi_FxObjectNewProgram(void)
{
    fx_unused_run(0x8AD0, 0xFFFF);
}
