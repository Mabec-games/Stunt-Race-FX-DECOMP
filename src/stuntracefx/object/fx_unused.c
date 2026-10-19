/*
 * fx_unused.c: Super FX code of bank 04 that nothing in the game calls or
 * starts: a checksum program over the object pools' RAM, an object-free
 * program, an object-new program, a matrix program and eight small
 * routines (a turned push, a clamp, a record's aim, two scalings, a
 * record step, a spare exit); and three programs making records from
 * the second pool (04:8BAD-8C09).
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
    goto enter;   /* the caller fetched the first byte */
l8915:
    R[10] = 0x0240;
    R[11] = 0x891D;
    gsu_code(0x048916, 7); gsu_code(0x048923, 1);
    abi_FxPoolChecksum();
    gsu_code(0x04891d, 1);
l891D:
    gsu_code(0x04891e, 4);
    gsu_stw(0x2686, R[0]);
    return;   /* the STOP: the program ends here */
l8923:
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_code(0x048924, 3);
    gsu_ramb(R[0]);
    R[1] = 0x1090;
    R[12] = 0x0AF9;
    /* CACHE */
    R[13] = 0x892F;
    gsu_code(0x048927, 6); gsu_cache(0x892d); gsu_code(0x04892d, 3);
l892F:
    gsu_code(0x048930, 2);
    R[2] = gsu_ldw(R[1]);
    R[1] = gsu_inc(R[1]);
    R[1] = gsu_inc(R[1]);
    R[0] = gsu_xor(R[0], R[2]);
    t = gsu_count_down(&R[12]);
    gsu_code(0x048932, 5);
    if (t) { gsu_code(0x04892f, 1); goto l892F; }
    nx = (uint16_t)(R[11]);
    at = nx;
    gsu_code(0x048937, 3);
    goto dispatch;
l89FA:
    R[10] = 0x0240;
    gsu_code(0x0489fb, 7);
    R[1] = gsu_ldw(0x238E);
    R[11] = 0x8A06;
    gsu_code(0x048a02, 4); gsu_code(0x048a08, 1);
    abi_FxObjectFree();
    gsu_code(0x048a06, 1);
l8A06:
    return;   /* the STOP: the program ends here */
l8AD0:
    R[10] = 0x0240;
    R[4] = 0x0028;
    R[3] = 0x1098;
    R[11] = 0x8ADE;
    gsu_code(0x048ad1, 13); gsu_code(0x04893a, 1);
    abi_FxPoolTake();
    gsu_code(0x048ade, 1);
l8ADE:
    R[1] = R[1];
    gsu_moves(R[1]);
    t = gsu.z;
    gsu_code(0x048adf, 4);
    if (t) { gsu_code(0x048b2c, 1); goto l8B2C; }
    gsu_code(0x048ae3, 5);
    R[2] = gsu_ldw(0x238E);
    R[0] = 0x001E;
    R[0] = gsu_add(R[0], R[1], 0);
    gsu_code(0x048ae8, 5);
    gsu_stw(R[0], R[2]);
    gsu_code(0x048aed, 4);
    R[2] = gsu_ldw(0x2390);
    R[0] = 0x0020;
    R[0] = gsu_add(R[0], R[1], 0);
    gsu_code(0x048af1, 5);
    gsu_stw(R[0], R[2]);
    gsu_code(0x048af6, 4);
    R[2] = gsu_ldw(0x2392);
    R[0] = 0x0022;
    R[0] = gsu_add(R[0], R[1], 0);
    gsu_code(0x048afa, 5);
    gsu_stw(R[0], R[2]);
    R[2] = 0x0016;
    R[2] = gsu_add(R[2], R[1], 0);
    gsu_code(0x048aff, 6);
    R[2] = gsu_ldw(R[2]);
    R[0] = 0x00A0;
    R[2] = gsu_or(R[0], R[2]);
    R[0] = 0x0016;
    R[0] = gsu_add(R[0], R[1], 0);
    gsu_code(0x048b05, 10);
    gsu_stw(R[0], R[2]);
    R[2] = 0xFFFF;
    R[0] = 0x001A;
    R[0] = gsu_add(R[0], R[1], 0);
    gsu_code(0x048b0f, 8);
    gsu_stw(R[0], R[2]);
    gsu_code(0x048b17, 4);
    gsu_stw(0x238E, R[1]);
    R[8] = R[1];
    R[11] = 0x8B21;
    gsu_code(0x048b1b, 6); gsu_code(0x04c3b5, 1);
    abi_FxRecordSegment();
    gsu_code(0x048b21, 1);
l8B21:
    R[11] = 0x8B26;
    gsu_code(0x048b22, 4); gsu_code(0x04cca4, 1);
    abi_FxCourseNextEntry();
    gsu_code(0x048b26, 1);
l8B26:
    gsu_code(0x048b27, 4);
    gsu_stw(0x2390, R[0]);
    return;   /* the STOP: the program ends here */
l8B2C:
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_code(0x048b2d, 5);
    gsu_stw(0x238E, R[0]);
    return;   /* the STOP: the program ends here */
dispatch:
    if (at == ret)
        return;
    gsu_code(0x040000 | at, 1);   /* where it jumps: its first byte */
fetched:
enter:
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

void abi_FxHitAddUnit(void);
void abi_FxMatrixFromAngles(void);

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode fx_unused2_run_code[9][72] = {
    {{0x049821, 3}, {0, 0}, {0x049824, 7}, {0x04982B, 8}, {0x049833, 8}, {0x04983B, 3}, {0x04983E, 7}, {0x049845, 8}, {0x04984D, 8}, {0x049855, 3}, {0x049858, 7}, {0x04985F, 8}, {0x049867, 8}, {0x04986F, 4}, {0x049873, 4}, {0x049877, 4}, {0x04987B, 2}, {0x0498AB, 3}, {0x04888C, 1}, {0x04CB81, 4}, {0x04CB85, 8}, {0x04CB8D, 6}, {0x04CB93, 6}, {0x04CB99, 4}, {0x04CBDD, 4}, {0x04CB99, 11}, {0x04CBA8, 2}, {0x04CBA4, 4}, {0x04CBAA, 6}, {0x04CBB0, 6}, {0x04CBB6, 6}, {0x04CBD0, 6}, {0x04CBC5, 6}, {0x04CBCB, 5}, {0x04CBD6, 2}, {0x04CBBC, 9}, {0x04CBD8, 5}, {0x04CBE1, 7}, {0x04A304, 8}, {0x04A30C, 4}, {0x04A328, 5}, {0x04A30C, 9}, {0x04A315, 4}, {0x04A319, 4}, {0x04A31D, 4}, {0x04A321, 5}, {0x04A326, 2}, {0x04A340, 1}, {0x04A32D, 4}, {0x04A331, 4}, {0x04A335, 4}, {0x04A339, 4}, {0x04A33D, 4}, {0x04A341, 4}, {0x049405, 7}, {0x04940C, 2}, {0x04940E, 1}, {0x04940F, 3}, {0x04BCB7, 3}, {0x04BCAC, 11}, {0x04C3A5, 6}, {0x04C3B5, 3}, {0x04C3AB, 5}, {0x04CCA4, 7}, {0x04C3B0, 5}, {0x04CD01, 4}, {0x049C33, 4}, {0x049C37, 4}, {0x04C1A5, 1}, {0x04C2C7, 1}, {0x04888C, 1}, {0, 0}},
    {{0x04CB7F, 2}, {0, 0}, {0x049824, 7}, {0x04982B, 8}, {0x049833, 8}, {0x04983B, 3}, {0x04983E, 7}, {0x049845, 8}, {0x04984D, 8}, {0x049855, 3}, {0x049858, 7}, {0x04985F, 8}, {0x049867, 8}, {0x04986F, 4}, {0x049873, 4}, {0x049877, 4}, {0x04987B, 2}, {0x0498AB, 3}, {0x04888C, 1}, {0x04CB81, 4}, {0x04CB85, 8}, {0x04CB8D, 6}, {0x04CB93, 6}, {0x04CB99, 4}, {0x04CBDD, 4}, {0x04CB99, 11}, {0x04CBA8, 2}, {0x04CBA4, 4}, {0x04CBAA, 6}, {0x04CBB0, 6}, {0x04CBB6, 6}, {0x04CBD0, 6}, {0x04CBC5, 6}, {0x04CBCB, 5}, {0x04CBD6, 2}, {0x04CBBC, 9}, {0x04CBD8, 5}, {0x04CBE1, 7}, {0x04A304, 8}, {0x04A30C, 4}, {0x04A328, 5}, {0x04A30C, 9}, {0x04A315, 4}, {0x04A319, 4}, {0x04A31D, 4}, {0x04A321, 5}, {0x04A326, 2}, {0x04A340, 1}, {0x04A32D, 4}, {0x04A331, 4}, {0x04A335, 4}, {0x04A339, 4}, {0x04A33D, 4}, {0x04A341, 4}, {0x049405, 7}, {0x04940C, 2}, {0x04940E, 1}, {0x04940F, 3}, {0x04BCB7, 3}, {0x04BCAC, 11}, {0x04C3A5, 6}, {0x04C3B5, 3}, {0x04C3AB, 5}, {0x04CCA4, 7}, {0x04C3B0, 5}, {0x04CD01, 4}, {0x049C33, 4}, {0x049C37, 4}, {0x04C1A5, 1}, {0x04C2C7, 1}, {0x04888C, 1}, {0, 0}},
    {{0x04A302, 2}, {0, 0}, {0x049824, 7}, {0x04982B, 8}, {0x049833, 8}, {0x04983B, 3}, {0x04983E, 7}, {0x049845, 8}, {0x04984D, 8}, {0x049855, 3}, {0x049858, 7}, {0x04985F, 8}, {0x049867, 8}, {0x04986F, 4}, {0x049873, 4}, {0x049877, 4}, {0x04987B, 2}, {0x0498AB, 3}, {0x04888C, 1}, {0x04CB81, 4}, {0x04CB85, 8}, {0x04CB8D, 6}, {0x04CB93, 6}, {0x04CB99, 4}, {0x04CBDD, 4}, {0x04CB99, 11}, {0x04CBA8, 2}, {0x04CBA4, 4}, {0x04CBAA, 6}, {0x04CBB0, 6}, {0x04CBB6, 6}, {0x04CBD0, 6}, {0x04CBC5, 6}, {0x04CBCB, 5}, {0x04CBD6, 2}, {0x04CBBC, 9}, {0x04CBD8, 5}, {0x04CBE1, 7}, {0x04A304, 8}, {0x04A30C, 4}, {0x04A328, 5}, {0x04A30C, 9}, {0x04A315, 4}, {0x04A319, 4}, {0x04A31D, 4}, {0x04A321, 5}, {0x04A326, 2}, {0x04A340, 1}, {0x04A32D, 4}, {0x04A331, 4}, {0x04A335, 4}, {0x04A339, 4}, {0x04A33D, 4}, {0x04A341, 4}, {0x049405, 7}, {0x04940C, 2}, {0x04940E, 1}, {0x04940F, 3}, {0x04BCB7, 3}, {0x04BCAC, 11}, {0x04C3A5, 6}, {0x04C3B5, 3}, {0x04C3AB, 5}, {0x04CCA4, 7}, {0x04C3B0, 5}, {0x04CD01, 4}, {0x049C33, 4}, {0x049C37, 4}, {0x04C1A5, 1}, {0x04C2C7, 1}, {0x04888C, 1}, {0, 0}},
    {{0x0493FB, 10}, {0, 0}, {0x049824, 7}, {0x04982B, 8}, {0x049833, 8}, {0x04983B, 3}, {0x04983E, 7}, {0x049845, 8}, {0x04984D, 8}, {0x049855, 3}, {0x049858, 7}, {0x04985F, 8}, {0x049867, 8}, {0x04986F, 4}, {0x049873, 4}, {0x049877, 4}, {0x04987B, 2}, {0x0498AB, 3}, {0x04888C, 1}, {0x04CB81, 4}, {0x04CB85, 8}, {0x04CB8D, 6}, {0x04CB93, 6}, {0x04CB99, 4}, {0x04CBDD, 4}, {0x04CB99, 11}, {0x04CBA8, 2}, {0x04CBA4, 4}, {0x04CBAA, 6}, {0x04CBB0, 6}, {0x04CBB6, 6}, {0x04CBD0, 6}, {0x04CBC5, 6}, {0x04CBCB, 5}, {0x04CBD6, 2}, {0x04CBBC, 9}, {0x04CBD8, 5}, {0x04CBE1, 7}, {0x04A304, 8}, {0x04A30C, 4}, {0x04A328, 5}, {0x04A30C, 9}, {0x04A315, 4}, {0x04A319, 4}, {0x04A31D, 4}, {0x04A321, 5}, {0x04A326, 2}, {0x04A340, 1}, {0x04A32D, 4}, {0x04A331, 4}, {0x04A335, 4}, {0x04A339, 4}, {0x04A33D, 4}, {0x04A341, 4}, {0x049405, 7}, {0x04940C, 2}, {0x04940E, 1}, {0x04940F, 3}, {0x04BCB7, 3}, {0x04BCAC, 11}, {0x04C3A5, 6}, {0x04C3B5, 3}, {0x04C3AB, 5}, {0x04CCA4, 7}, {0x04C3B0, 5}, {0x04CD01, 4}, {0x049C33, 4}, {0x049C37, 4}, {0x04C1A5, 1}, {0x04C2C7, 1}, {0x04888C, 1}, {0x049412, 5}},
    {{0x04888C, 1}, {0x04BCA6, 6}, {0x049824, 7}, {0x04982B, 8}, {0x049833, 8}, {0x04983B, 3}, {0x04983E, 7}, {0x049845, 8}, {0x04984D, 8}, {0x049855, 3}, {0x049858, 7}, {0x04985F, 8}, {0x049867, 8}, {0x04986F, 4}, {0x049873, 4}, {0x049877, 4}, {0x04987B, 2}, {0x0498AB, 3}, {0x04888C, 1}, {0x04CB81, 4}, {0x04CB85, 8}, {0x04CB8D, 6}, {0x04CB93, 6}, {0x04CB99, 4}, {0x04CBDD, 4}, {0x04CB99, 11}, {0x04CBA8, 2}, {0x04CBA4, 4}, {0x04CBAA, 6}, {0x04CBB0, 6}, {0x04CBB6, 6}, {0x04CBD0, 6}, {0x04CBC5, 6}, {0x04CBCB, 5}, {0x04CBD6, 2}, {0x04CBBC, 9}, {0x04CBD8, 5}, {0x04CBE1, 7}, {0x04A304, 8}, {0x04A30C, 4}, {0x04A328, 5}, {0x04A30C, 9}, {0x04A315, 4}, {0x04A319, 4}, {0x04A31D, 4}, {0x04A321, 5}, {0x04A326, 2}, {0x04A340, 1}, {0x04A32D, 4}, {0x04A331, 4}, {0x04A335, 4}, {0x04A339, 4}, {0x04A33D, 4}, {0x04A341, 4}, {0x049405, 7}, {0x04940C, 2}, {0x04940E, 1}, {0x04940F, 3}, {0x04BCB7, 3}, {0x04BCAC, 11}, {0x04C3A5, 6}, {0x04C3B5, 3}, {0x04C3AB, 5}, {0x04CCA4, 7}, {0x04C3B0, 5}, {0x04CD01, 4}, {0x049C33, 4}, {0x049C37, 4}, {0x04C1A5, 1}, {0x04C2C7, 1}, {0, 0}, {0, 0}},
    {{0x04C3A3, 2}, {0, 0}, {0x049824, 7}, {0x04982B, 8}, {0x049833, 8}, {0x04983B, 3}, {0x04983E, 7}, {0x049845, 8}, {0x04984D, 8}, {0x049855, 3}, {0x049858, 7}, {0x04985F, 8}, {0x049867, 8}, {0x04986F, 4}, {0x049873, 4}, {0x049877, 4}, {0x04987B, 2}, {0x0498AB, 3}, {0x04888C, 1}, {0x04CB81, 4}, {0x04CB85, 8}, {0x04CB8D, 6}, {0x04CB93, 6}, {0x04CB99, 4}, {0x04CBDD, 4}, {0x04CB99, 11}, {0x04CBA8, 2}, {0x04CBA4, 4}, {0x04CBAA, 6}, {0x04CBB0, 6}, {0x04CBB6, 6}, {0x04CBD0, 6}, {0x04CBC5, 6}, {0x04CBCB, 5}, {0x04CBD6, 2}, {0x04CBBC, 9}, {0x04CBD8, 5}, {0x04CBE1, 7}, {0x04A304, 8}, {0x04A30C, 4}, {0x04A328, 5}, {0x04A30C, 9}, {0x04A315, 4}, {0x04A319, 4}, {0x04A31D, 4}, {0x04A321, 5}, {0x04A326, 2}, {0x04A340, 1}, {0x04A32D, 4}, {0x04A331, 4}, {0x04A335, 4}, {0x04A339, 4}, {0x04A33D, 4}, {0x04A341, 4}, {0x049405, 7}, {0x04940C, 2}, {0x04940E, 1}, {0x04940F, 3}, {0x04BCB7, 3}, {0x04BCAC, 11}, {0x04C3A5, 6}, {0x04C3B5, 3}, {0x04C3AB, 5}, {0x04CCA4, 7}, {0x04C3B0, 5}, {0x04CD01, 4}, {0x049C33, 4}, {0x049C37, 4}, {0x04C1A5, 1}, {0x04C2C7, 1}, {0x04888C, 1}, {0, 0}},
    {{0x049C2D, 6}, {0, 0}, {0x049824, 7}, {0x04982B, 8}, {0x049833, 8}, {0x04983B, 3}, {0x04983E, 7}, {0x049845, 8}, {0x04984D, 8}, {0x049855, 3}, {0x049858, 7}, {0x04985F, 8}, {0x049867, 8}, {0x04986F, 4}, {0x049873, 4}, {0x049877, 4}, {0x04987B, 2}, {0x0498AB, 3}, {0x04888C, 1}, {0x04CB81, 4}, {0x04CB85, 8}, {0x04CB8D, 6}, {0x04CB93, 6}, {0x04CB99, 4}, {0x04CBDD, 4}, {0x04CB99, 11}, {0x04CBA8, 2}, {0x04CBA4, 4}, {0x04CBAA, 6}, {0x04CBB0, 6}, {0x04CBB6, 6}, {0x04CBD0, 6}, {0x04CBC5, 6}, {0x04CBCB, 5}, {0x04CBD6, 2}, {0x04CBBC, 9}, {0x04CBD8, 5}, {0x04CBE1, 7}, {0x04A304, 8}, {0x04A30C, 4}, {0x04A328, 5}, {0x04A30C, 9}, {0x04A315, 4}, {0x04A319, 4}, {0x04A31D, 4}, {0x04A321, 5}, {0x04A326, 2}, {0x04A340, 1}, {0x04A32D, 4}, {0x04A331, 4}, {0x04A335, 4}, {0x04A339, 4}, {0x04A33D, 4}, {0x04A341, 4}, {0x049405, 7}, {0x04940C, 2}, {0x04940E, 1}, {0x04940F, 3}, {0x04BCB7, 3}, {0x04BCAC, 11}, {0x04C3A5, 6}, {0x04C3B5, 3}, {0x04C3AB, 5}, {0x04CCA4, 7}, {0x04C3B0, 5}, {0x04CD01, 4}, {0x049C33, 4}, {0x049C37, 4}, {0x04C1A5, 1}, {0x04C2C7, 1}, {0x04888C, 1}, {0x049C3B, 2}},
    {{0x04C19E, 7}, {0x04C1A7, 9}, {0x049824, 7}, {0x04982B, 8}, {0x049833, 8}, {0x04983B, 3}, {0x04983E, 7}, {0x049845, 8}, {0x04984D, 8}, {0x049855, 3}, {0x049858, 7}, {0x04985F, 8}, {0x049867, 8}, {0x04986F, 4}, {0x049873, 4}, {0x049877, 4}, {0x04987B, 2}, {0x0498AB, 3}, {0x04888C, 1}, {0x04CB81, 4}, {0x04CB85, 8}, {0x04CB8D, 6}, {0x04CB93, 6}, {0x04CB99, 4}, {0x04CBDD, 4}, {0x04CB99, 11}, {0x04CBA8, 2}, {0x04CBA4, 4}, {0x04CBAA, 6}, {0x04CBB0, 6}, {0x04CBB6, 6}, {0x04CBD0, 6}, {0x04CBC5, 6}, {0x04CBCB, 5}, {0x04CBD6, 2}, {0x04CBBC, 9}, {0x04CBD8, 5}, {0x04CBE1, 7}, {0x04A304, 8}, {0x04A30C, 4}, {0x04A328, 5}, {0x04A30C, 9}, {0x04A315, 4}, {0x04A319, 4}, {0x04A31D, 4}, {0x04A321, 5}, {0x04A326, 2}, {0x04A340, 1}, {0x04A32D, 4}, {0x04A331, 4}, {0x04A335, 4}, {0x04A339, 4}, {0x04A33D, 4}, {0x04A341, 4}, {0x049405, 7}, {0x04940C, 2}, {0x04940E, 1}, {0x04940F, 3}, {0x04BCB7, 3}, {0x04BCAC, 11}, {0x04C3A5, 6}, {0x04C3B5, 3}, {0x04C3AB, 5}, {0x04CCA4, 7}, {0x04C3B0, 5}, {0x04CD01, 4}, {0x049C33, 4}, {0x049C37, 4}, {0x04C1A5, 1}, {0x04C2C7, 1}, {0x04888C, 1}, {0, 0}},
    {{0x049418, 4}, {0, 0}, {0x049824, 7}, {0x04982B, 8}, {0x049833, 8}, {0x04983B, 3}, {0x04983E, 7}, {0x049845, 8}, {0x04984D, 8}, {0x049855, 3}, {0x049858, 7}, {0x04985F, 8}, {0x049867, 8}, {0x04986F, 4}, {0x049873, 4}, {0x049877, 4}, {0x04987B, 2}, {0x0498AB, 3}, {0x04888C, 1}, {0x04CB81, 4}, {0x04CB85, 8}, {0x04CB8D, 6}, {0x04CB93, 6}, {0x04CB99, 4}, {0x04CBDD, 4}, {0x04CB99, 11}, {0x04CBA8, 2}, {0x04CBA4, 4}, {0x04CBAA, 6}, {0x04CBB0, 6}, {0x04CBB6, 6}, {0x04CBD0, 6}, {0x04CBC5, 6}, {0x04CBCB, 5}, {0x04CBD6, 2}, {0x04CBBC, 9}, {0x04CBD8, 5}, {0x04CBE1, 7}, {0x04A304, 8}, {0x04A30C, 4}, {0x04A328, 5}, {0x04A30C, 9}, {0x04A315, 4}, {0x04A319, 4}, {0x04A31D, 4}, {0x04A321, 5}, {0x04A326, 2}, {0x04A340, 1}, {0x04A32D, 4}, {0x04A331, 4}, {0x04A335, 4}, {0x04A339, 4}, {0x04A33D, 4}, {0x04A341, 4}, {0x049405, 7}, {0x04940C, 2}, {0x04940E, 1}, {0x04940F, 3}, {0x04BCB7, 3}, {0x04BCAC, 11}, {0x04C3A5, 6}, {0x04C3B5, 3}, {0x04C3AB, 5}, {0x04CCA4, 7}, {0x04C3B0, 5}, {0x04CD01, 4}, {0x049C33, 4}, {0x049C37, 4}, {0x04C1A5, 1}, {0x04C2C7, 1}, {0x04888C, 1}, {0, 0}},
};

/* The other uncalled pieces of bank 04 (04:93FA, 9820, 9C2C, A301, BCA5,
   C19D, C3A2, CB7E) from address at until they jump to ret. */
static void fx_unused2_run(uint16_t at, uint16_t ret, int copy)
{
    gsu_codes(fx_unused2_run_code[copy], 2);
    int t;
    uint16_t nx;

    (void)t;
    (void)nx;
    goto dispatch;
l9820:
    R[6] = gsu_ldw(0x00E4);
    R[0] = gsu_fmult(R[1], R[6]);
    R[5] = gsu_rol(R[0]);
    R[6] = gsu_ldw(gsu_codes_then(fx_unused2_run_code[copy] + 2, 1, 0x00EA));
    R[0] = gsu_fmult(R[2], R[6]);
    R[0] = gsu_rol(R[0]);
    R[5] = gsu_add(R[5], R[0], 0);
    R[6] = gsu_ldw(gsu_codes_then(fx_unused2_run_code[copy] + 3, 1, 0x00F0));
    R[0] = gsu_fmult(R[3], R[6]);
    R[0] = gsu_rol(R[0]);
    R[0] = gsu_add(R[0], R[5], 0);
    gsu_codes(fx_unused2_run_code[copy] + 4, 1);
    gsu_stw(0x26F6, R[0]);
    R[6] = gsu_ldw(gsu_codes_then(fx_unused2_run_code[copy] + 5, 1, 0x00E6));
    R[0] = gsu_fmult(R[1], R[6]);
    R[5] = gsu_rol(R[0]);
    R[6] = gsu_ldw(gsu_codes_then(fx_unused2_run_code[copy] + 6, 1, 0x00EC));
    R[0] = gsu_fmult(R[2], R[6]);
    R[0] = gsu_rol(R[0]);
    R[5] = gsu_add(R[5], R[0], 0);
    R[6] = gsu_ldw(gsu_codes_then(fx_unused2_run_code[copy] + 7, 1, 0x00F2));
    R[0] = gsu_fmult(R[3], R[6]);
    R[0] = gsu_rol(R[0]);
    R[0] = gsu_add(R[0], R[5], 0);
    gsu_codes(fx_unused2_run_code[copy] + 8, 1);
    gsu_stw(0x26F8, R[0]);
    R[6] = gsu_ldw(gsu_codes_then(fx_unused2_run_code[copy] + 9, 1, 0x00E8));
    R[0] = gsu_fmult(R[1], R[6]);
    R[5] = gsu_rol(R[0]);
    R[6] = gsu_ldw(gsu_codes_then(fx_unused2_run_code[copy] + 10, 1, 0x00EE));
    R[0] = gsu_fmult(R[2], R[6]);
    R[0] = gsu_rol(R[0]);
    R[5] = gsu_add(R[5], R[0], 0);
    R[6] = gsu_ldw(gsu_codes_then(fx_unused2_run_code[copy] + 11, 1, 0x00F4));
    R[0] = gsu_fmult(R[3], R[6]);
    R[0] = gsu_rol(R[0]);
    R[0] = gsu_add(R[0], R[5], 0);
    gsu_codes(fx_unused2_run_code[copy] + 12, 1);
    gsu_stw(0x26FA, R[0]);
    R[1] = gsu_ldw(gsu_codes_then(fx_unused2_run_code[copy] + 13, 1, 0x26F6));
    R[2] = gsu_ldw(gsu_codes_then(fx_unused2_run_code[copy] + 14, 1, 0x26F8));
    R[3] = gsu_ldw(gsu_codes_then(fx_unused2_run_code[copy] + 15, 1, 0x26FA));
    (gsu_codes(fx_unused2_run_code[copy] + 16, 2), abi_FxHitAddUnit());           /* BRA into it: its return is ours */
    gsu_codes(fx_unused2_run_code[copy] + 18, 1);
    return;
lCB7E:
    gsu_stw(R[10], R[11]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    gsu_codes(fx_unused2_run_code[copy] + 19, 1);
    gsu_stw(R[10], R[1]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[1] = 0x001E;
    R[1] = gsu_add(R[1], R[8], 0);
    R[1] = gsu_ldw(gsu_codes_then(fx_unused2_run_code[copy] + 20, 1, R[1]));
    R[2] = 0x0020;
    R[2] = gsu_add(R[2], R[8], 0);
    R[2] = gsu_ldw(gsu_codes_then(fx_unused2_run_code[copy] + 21, 1, R[2]));
    R[3] = 0x0022;
    R[3] = gsu_add(R[3], R[8], 0);
    R[3] = gsu_ldw(gsu_codes_then(fx_unused2_run_code[copy] + 22, 1, R[3]));
    R[5] = R[5];
    gsu_moves(R[5]);
    t = gsu.z;
    if (t) { gsu_codes(fx_unused2_run_code[copy] + 23, 2); goto lCBDD; } else { gsu_codes(fx_unused2_run_code[copy] + 25, 1); }
    R[1] = 0x0026;
    R[1] = gsu_add(R[1], R[5], 0);
    R[1] = gsu_ldw(R[1]);
    R[1] = R[1];
    gsu_moves(R[1]);
    t = gsu.z;
    if (t) goto lCBAA; else { gsu_codes(fx_unused2_run_code[copy] + 26, 1); }
    R[5] = R[1];
lCBAA:
    R[0] = 0x0058;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_codes(fx_unused2_run_code[copy] + 27, 2);
    gsu_stw(R[0], R[5]);
    R[1] = R[0];
    R[0] = 0x0024;
    R[0] = gsu_add(R[0], R[8], 0);
    R[0] = gsu_ldw(gsu_codes_then(fx_unused2_run_code[copy] + 29, 1, R[0]));
    R[1] = gsu_sub(R[1], R[0], 0);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    R[3] = gsu_ldw(gsu_codes_then(fx_unused2_run_code[copy] + 30, 1, R[10]));
    R[2] = R[3];
    R[2] = gsu_not(R[2]);
    R[2] = gsu_inc(R[2]);
    R[1] = R[1];
    gsu_moves(R[1]);
    t = gsu.s;
    if (t) { gsu_codes(fx_unused2_run_code[copy] + 31, 1); goto lCBD0; } else { gsu_codes(fx_unused2_run_code[copy] + 32, 1); }
    gsu_sub(R[1], R[3], 0);   /* CMP */
    t = gsu.s;
    if (t) goto lCBD8; else { gsu_codes(fx_unused2_run_code[copy] + 33, 1); }
    R[1] = R[3];
    t = 1;
    if (t) goto lCBD8;
lCBD0:
    gsu_sub(R[1], R[2], 0);   /* CMP */
    t = !gsu.s;
    if (t) goto lCBD8; else { gsu_codes(fx_unused2_run_code[copy] + 34, 1); }
    R[1] = R[2];
lCBD8:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_codes(fx_unused2_run_code[copy] + 35, 2);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
lCBDD:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    R[0] = gsu_ldw(R[10]);
    R[1] = 0x0000;
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_codes(fx_unused2_run_code[copy] + 37, 1);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
lA301:
    gsu_stw(R[10], R[11]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[1] = 0x0018;
    R[1] = gsu_add(R[1], R[7], 0);
    R[1] = gsu_ldw(gsu_codes_then(fx_unused2_run_code[copy] + 38, 1, R[1]));
    R[1] = R[1];
    gsu_moves(R[1]);
    t = gsu.z;
    if (t) { gsu_codes(fx_unused2_run_code[copy] + 39, 2); goto lA328; } else { gsu_codes(fx_unused2_run_code[copy] + 41, 1); }
    R[0] = 0x0004;
    R[0] = gsu_add(R[0], R[1], 0);
    R[0] = gsu_ldw(R[0]);
    gsu_codes(fx_unused2_run_code[copy] + 42, 1);
    gsu_stw(0x0068, R[0]);
    R[0] = 0x0006;
    R[0] = gsu_add(R[0], R[1], 0);
    R[0] = gsu_ldw(gsu_codes_then(fx_unused2_run_code[copy] + 43, 1, R[0]));
    gsu_codes(fx_unused2_run_code[copy] + 44, 1);
    gsu_stw(0x002E, R[0]);
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_codes(fx_unused2_run_code[copy] + 45, 1);
    gsu_stw(0x002C, R[0]);
    t = 1;
    if (t) { gsu_codes(fx_unused2_run_code[copy] + 46, 2); goto lA340; }
lA328:
    R[0] = 0x002E;
    R[0] = gsu_add(R[0], R[7], 0);
    R[0] = gsu_ldw(R[0]);
    gsu_codes(fx_unused2_run_code[copy] + 48, 1);
    gsu_stw(0x0068, R[0]);
    R[0] = 0x0030;
    R[0] = gsu_add(R[0], R[7], 0);
    R[0] = gsu_ldw(gsu_codes_then(fx_unused2_run_code[copy] + 49, 1, R[0]));
    gsu_codes(fx_unused2_run_code[copy] + 50, 1);
    gsu_stw(0x002C, R[0]);
    R[0] = 0x0032;
    R[0] = gsu_add(R[0], R[7], 0);
    R[0] = gsu_ldw(gsu_codes_then(fx_unused2_run_code[copy] + 51, 1, R[0]));
    gsu_codes(fx_unused2_run_code[copy] + 52, 1);
    gsu_stw(0x002E, R[0]);
lA340:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    gsu_codes(fx_unused2_run_code[copy] + 53, 1);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
l93FA:
    R[1] = 0x0100;
    R[0] = 0x0014;
    gsu_romb(R[0]);
    gsu_r14(0x896C);
    R[0] = 0x0200;
    R[0] = gsu_bic(R[0], 1);
    gsu_codes(fx_unused2_run_code[copy] + 54, 1);
    gsu_r14(gsu_add(R[14], R[0], 0));
    R[6] = (gsu_codes(fx_unused2_run_code[copy] + 55, 1), gsu_getb());
    gsu_codes(fx_unused2_run_code[copy] + 56, 1);
    gsu_r14(gsu_inc(R[14]));
    R[6] = gsu_getbh(gsu_codes_then(fx_unused2_run_code[copy] + 57, 1, R[6]));
    R[0] = gsu_fmult(R[1], R[6]);
    R[0] = gsu_add(R[0], R[0], 0);
    nx = (uint16_t)(R[11]);
    at = nx;
    goto dispatch;
l9417:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
lBCA5:
    R[1] = 0x0100;
    R[0] = gsu_sub(R[0], R[1], 0);
    t = gsu.s;
    if (t) { gsu_codes(fx_unused2_run_code[copy] + 58, 1); goto lBCB7; } else { gsu_codes(fx_unused2_run_code[copy] + 59, 1); }
    R[6] = 0x0080;
    { uint32_t p = gsu_lmult(R[0], R[6]); R[4] = (uint16_t)p; R[0] = (uint16_t)(p >> 16); }
    R[0] = gsu_swap(R[4]);
    R[0] = gsu_lob(R[0]);
    nx = (uint16_t)(R[11]);
    at = nx;
    goto dispatch;
lBCB7:
    nx = (uint16_t)(R[11]);
    at = nx;
    goto dispatch;
lC3A2:
    gsu_stw(R[10], R[11]);
    R[10] = gsu_inc(R[10]);
    R[10] = gsu_inc(R[10]);
    R[11] = 0xC3AB;
    (gsu_codes(fx_unused2_run_code[copy] + 60, 2), abi_FxRecordSegment());
gsu_codes(fx_unused2_run_code[copy] + 62, 2);
lC3AB:
    R[11] = 0xC3B0;
    abi_FxCourseNextEntry();
gsu_codes(fx_unused2_run_code[copy] + 64, 2);
lC3B0:
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    nx = (uint16_t)(gsu_ldw(R[10]));
    at = nx;
    goto dispatch;
l9C2C:
    R[0] = 0x0001;
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    R[9] = gsu_ldw(R[10]);
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    R[8] = gsu_ldw(gsu_codes_then(fx_unused2_run_code[copy] + 66, 1, R[10]));
    R[10] = gsu_dec(R[10]);
    R[10] = gsu_dec(R[10]);
    R[11] = gsu_ldw(gsu_codes_then(fx_unused2_run_code[copy] + 67, 1, R[10]));
    nx = (uint16_t)(R[11]);
    at = nx;
    goto dispatch;
lC19D:
    R[9] = 0x00E4;
    R[11] = 0xC1A5;
    abi_FxMatrixFromAngles();
gsu_codes(fx_unused2_run_code[copy] + 68, 2);
lC1A5:
    return;   /* the STOP: the program ends here */
dispatch:
    if (at == ret)
        { gsu_codes(fx_unused2_run_code[copy] + 70, 2); return; }
    switch (at) {
    case 0x93FA: goto l93FA;
    case 0x9417: goto l9417;
    case 0x9820: goto l9820;
    case 0x9C2C: goto l9C2C;
    case 0xA301: goto lA301;
    case 0xA328: goto lA328;
    case 0xA340: goto lA340;
    case 0xBCA5: goto lBCA5;
    case 0xBCB7: goto lBCB7;
    case 0xC19D: goto lC19D;
    case 0xC1A5: goto lC1A5;
    case 0xC3A2: goto lC3A2;
    case 0xC3AB: goto lC3AB;
    case 0xC3B0: goto lC3B0;
    case 0xCB7E: goto lCB7E;
    case 0xCBAA: goto lCBAA;
    case 0xCBD0: goto lCBD0;
    case 0xCBD8: goto lCBD8;
    case 0xCBDD: goto lCBDD;
    }
    return;   /* not an address the code jumps to */
}

/*
 * @routine 04:9820 FxHitAddRotated
 * WHAT IT DOES: Turns the push (R1, R2, R3) by the current matrix at
 * $00E4 (each result the sum of the three products with one column:
 * $E4/$EA/$F0, $E6/$EC/$F2, $E8/$EE/$F4) into $26F6-$26FA, then
 * adds it as FxHitAddUnit does. Nothing in the game calls it.
 * HOW: FMULT, ROL for each product (1.15); the sums stored with SM and
 * read back into R1-R3; BRA into FxHitAddUnit.
 * Entry: R1-R3 = the push; R10 = stack; R11 = return address. Exit: as
 * FxHitAddUnit.
 */
void abi_FxHitAddRotated(void)
{
    fx_unused2_run(0x9820, R[11], GSU_COPY(0));
}

/*
 * @routine 04:CB7E FxObjectSlideClamp
 * WHAT IT DOES: For the object at R8 and the one at R5: none (R5 = 0)
 * gives R1 = 0; otherwise R5 is replaced by the one at its +$26 when that
 * is set, stored at R8's +$58, and R1 = (R8 + $58) - R8's +$24, kept
 * within plus or minus R1 as it came in (pushed at the start, pulled into
 * R3, its negative in R2). R8's position (+$1E/+$20/+$22) is loaded into
 * R1-R3 first. Nothing in the game calls it.
 * HOW: R11 and R1 on the R10 stack; signed compares (CMP, BMI); returns by
 * pulling R15.
 * Entry: R1 = the limit; R5, R8 = the objects; R10 = stack; R11 = return
 * address. Exit: R1 as above; R0 = R8 + $58, or the limit with no R5.
 */
void abi_FxObjectSlideClamp(void)
{
    fx_unused2_run(0xCB7E, R[11], GSU_COPY(1));
}

/*
 * @routine 04:A301 FxRecordAimFromLink
 * WHAT IT DOES: Sets the aim words $0068, $002C, $002E for the record at
 * R7: from the record linked at its +$18 ($68 = its +$04, $2E = its +$06,
 * $2C = 0) when there is one, else from the record's own +$2E, +$30 and
 * +$32. Nothing in the game calls it.
 * HOW: LDW from R7 + each field; R11 on the R10 stack; returns by pulling
 * R15.
 * Entry: R7 = the record; R10 = stack; R11 = return address. Exit: R0 the
 * last word stored; R1 = the link.
 */
void abi_FxRecordAimFromLink(void)
{
    fx_unused2_run(0xA301, R[11], GSU_COPY(2));
}

/*
 * @routine 04:93FA FxScaleFromTable14
 * WHAT IT DOES: R0 = 2 * FMULT($0100, the word at $14:896C + $0200):
 * an entry of the bank $14 table at $896C halved, ROM bank $14 left set.
 * Its last five bytes, a pull return after the JMP R11's delay slot that
 * nothing reaches, are filed on their own (FxScaleFromTable14Pull,
 * 04:9417). Nothing in the game calls it.
 * HOW: ROMB $14, R14 = the table + ($0200 BIC 1), GETB, GETBH, FMULT, ADD
 * R0.
 * Entry: R11 = return address. Exit: R0 as above; R1 = $0100; R6 = the
 * table word; ROM bank $14; R14 and the ROM buffer move as on the Super
 * FX.
 */
void abi_FxScaleFromTable14(void)
{
    fx_unused2_run(0x93FA, R[11], GSU_COPY(3));
}

/*
 * @routine 04:9417 FxScaleFromTable14Pull
 * WHAT IT DOES: The pull return after FxScaleFromTable14's JMP R11 and
 * its delay slot, which no path reaches. A unit start runs it so the C
 * is judged against the original.
 * HOW: DEC R10 twice, then R15 = the word there (NOP in the delay slot).
 * Entry: R10 = stack, the return address the word below it. Exit: R10
 * two lower; S, Z from R10.
 */
void abi_FxScaleFromTable14Pull(void)
{
    fx_unused2_run(0x9417, R[11], GSU_COPY(8));
}

/*
 * @routine 04:BCA5 FxAbove100Scaled
 * WHAT IT DOES: When R0 is $0100 or more, R0 = the low byte of the middle
 * bytes of (R0 - $0100) * $80 (that is ((R0 - $100) >> 1) & $FF); below
 * $0100 R0 is left R0 - $0100. Nothing in the game calls it.
 * HOW: SUB, BMI; LMULT by $80, SWAP of the low word, LOB.
 * Entry: R0 = the value; R11 = return address. Exit: R0 as above; R1 =
 * $0100; R4, R6 as the LMULT leaves them.
 */
void abi_FxAbove100Scaled(void)
{
    fx_unused2_run(0xBCA5, R[11], GSU_COPY(4));
}

/*
 * @routine 04:C3A2 FxRecordSegmentNext
 * WHAT IT DOES: FxRecordSegment, then FxCourseNextEntry. Nothing in the
 * game calls it.
 * HOW: R11 on the R10 stack around the two LINK calls; returns by pulling
 * R15.
 * Entry: as FxRecordSegment; R10 = stack; R11 = return address. Exit: as
 * FxCourseNextEntry.
 */
void abi_FxRecordSegmentNext(void)
{
    fx_unused2_run(0xC3A2, R[11], GSU_COPY(5));
}

/*
 * @routine 04:9C2C FxReturnOneRestore
 * WHAT IT DOES: Returns R0 = 1 to the caller whose R11, R8 and R9 are
 * pushed on the R10 stack: pulls R9, R8 and R11, then jumps to R11. A spare
 * exit nothing reaches.
 * HOW: IBT R0, three pulls, MOVE R15, R11.
 * Entry: R10 = stack (R11, R8, R9 pushed in that order). Exit: R0 = 1;
 * R8, R9, R11 as pulled; R10 six lower.
 */
void abi_FxReturnOneRestore(void)
{
    fx_unused2_run(0x9C2C, R[11], GSU_COPY(6));
}

/*
 * @routine 04:C19D FxMatrixFromAnglesProgram04
 * WHAT IT DOES: A program: the rotation matrix for the angles into $00E4
 * (FxMatrixFromAngles, R9 = $00E4), then STOP. Nothing in the game starts
 * it.
 * HOW: IWT R9, LINK, STOP.
 * Entry: run as a program. Exit: (at the STOP) as FxMatrixFromAngles
 * leaves the registers, R11 = $C1A5.
 */
void abi_FxMatrixFromAnglesProgram04(void)
{
    fx_unused2_run(0xC19D, 0xFFFF, GSU_COPY(7));
}

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode fx_unused8b_run_code[3][15] = {
    {{0x04893A, 3}, {0x048BAE, 13}, {0x0489F8, 2}, {0x048BBB, 5}, {0x048BC0, 4}, {0x048BE0, 1}, {0x048BC0, 9}, {0x048BC9, 6}, {0x048BCF, 10}, {0x048BD9, 8}, {0x0489F8, 2}, {0x048BF7, 5}, {0x048BFC, 4}, {0x048C08, 1}, {0x048BFC, 13}},
    {{0x048BE3, 8}, {0, 0}, {0x0489F8, 2}, {0x048BBB, 5}, {0x048BC0, 4}, {0x048BE0, 1}, {0x048BC0, 9}, {0x048BC9, 6}, {0x048BCF, 10}, {0x048BD9, 8}, {0x0489F8, 2}, {0x048BF7, 5}, {0x048BFC, 4}, {0x048C08, 1}, {0x048BFC, 13}},
    {{0x04893A, 3}, {0x048BED, 10}, {0x0489F8, 2}, {0x048BBB, 5}, {0x048BC0, 4}, {0x048BE0, 1}, {0x048BC0, 9}, {0x048BC9, 6}, {0x048BCF, 10}, {0x048BD9, 8}, {0x0489F8, 2}, {0x048BF7, 5}, {0x048BFC, 4}, {0x048C08, 1}, {0x048BFC, 13}},
};

/* Bank 04 programs nothing starts (04:8BAD-8C09), new records from the
   second pool: the code from address at until its STOP. */
static void fx_unused8b_run(uint16_t at, uint16_t ret, int copy)
{
    gsu_codes(fx_unused8b_run_code[copy], 2);
    int t;
    uint16_t nx;

    (void)t;
    (void)nx;
    goto dispatch;
l8BAD:
    R[10] = 0x0240;
    R[4] = 0x002A;
    R[3] = 0x1098;
    R[11] = 0x8BBB;
    abi_FxPoolTake();
gsu_codes(fx_unused8b_run_code[copy] + 2, 2);
l8BBB:
    gsu_stw(0x238E, R[1]);
    R[1] = R[1];
    gsu_moves(R[1]);
    t = gsu.z;
    if (t) { gsu_codes(fx_unused8b_run_code[copy] + 4, 2); goto l8BE0; } else { gsu_codes(fx_unused8b_run_code[copy] + 6, 1); }
    gsu_stw(0x239A, R[1]);
    R[2] = 0x0016;
    R[2] = gsu_add(R[2], R[1], 0);
    R[2] = gsu_ldw(gsu_codes_then(fx_unused8b_run_code[copy] + 7, 1, R[2]));
    R[0] = 0x0020;
    R[2] = gsu_or(R[0], R[2]);
    R[0] = 0x0016;
    R[0] = gsu_add(R[0], R[1], 0);
    gsu_codes(fx_unused8b_run_code[copy] + 8, 1);
    gsu_stw(R[0], R[2]);
    R[2] = 0xFFFF;
    R[0] = 0x001A;
    R[0] = gsu_add(R[0], R[1], 0);
    gsu_codes(fx_unused8b_run_code[copy] + 9, 1);
    gsu_stw(R[0], R[2]);
l8BE0:
    return;   /* the STOP: the program ends here */
l8BE2:
    R[10] = 0x0240;
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_stw(0x238E, R[0]);
    return;   /* the STOP: the program ends here */
l8BEC:
    R[4] = 0x002A;
    R[3] = 0x1098;
    R[11] = 0x8BF7;
    abi_FxPoolTake();
gsu_codes(fx_unused8b_run_code[copy] + 10, 2);
l8BF7:
    gsu_stw(0x238E, R[1]);
    R[1] = R[1];
    gsu_moves(R[1]);
    t = gsu.z;
    if (t) { gsu_codes(fx_unused8b_run_code[copy] + 12, 2); goto l8C08; } else { gsu_codes(fx_unused8b_run_code[copy] + 14, 1); }
    R[2] = 0x0001;
    R[0] = 0x0016;
    R[0] = gsu_add(R[0], R[1], 0);
    gsu_stw(R[0], R[2]);
l8C08:
    return;   /* the STOP: the program ends here */
dispatch:
    if (at == ret)
        return;
    switch (at) {
    case 0x8BAD: goto l8BAD;
    case 0x8BE2: goto l8BE2;
    case 0x8BEC: goto l8BEC;
    case 0x8BBB: goto l8BBB;
    case 0x8BE0: goto l8BE0;
    case 0x8BF7: goto l8BF7;
    case 0x8C08: goto l8C08;
    }
    return;   /* not an address the code jumps to */
}

/*
 * @routine 04:8BAD FxRecordNewMarkedProgram
 * WHAT IT DOES: A program nothing starts: a new $2A-byte record from the
 * second pool (FxPoolTake, pool $1098), its address (or 0) to $238E; when
 * there is one, also to $239A, with bit 5 set in its flags (+$16) and its
 * mode (+$1A) $FFFF.
 * HOW: R10 = $0240; LINK, IWT R15; then STOP.
 * Entry: the S-CPU starts it. Exit: (at its STOP) as FxPoolTake leaves
 * them; R1 = the record or 0.
 */
void abi_FxRecordNewMarkedProgram(void)
{
    fx_unused8b_run(0x8BAD, 0xFFFF, GSU_COPY(0));
}

/*
 * @routine 04:8BE2 FxRecordNoneProgram
 * WHAT IT DOES: A program nothing starts: $238E = 0.
 * HOW: R10 = $0240; SUB R0, SM; then STOP.
 * Entry: the S-CPU starts it. Exit: (at its STOP) R0 = 0; R10 = $0240.
 */
void abi_FxRecordNoneProgram(void)
{
    fx_unused8b_run(0x8BE2, 0xFFFF, GSU_COPY(1));
}

/*
 * @routine 04:8BEC FxRecordNewFlag1Program
 * WHAT IT DOES: A program nothing starts: a new $2A-byte record from the
 * second pool (FxPoolTake, pool $1098), its address (or 0) to $238E; when
 * there is one, its flags (+$16) = 1. It sets no stack: R10 is as the
 * S-CPU left it.
 * HOW: LINK, IWT R15; then STOP.
 * Entry: the S-CPU starts it; R10 = stack. Exit: (at its STOP) as
 * FxPoolTake leaves them; R1 = the record or 0.
 */
void abi_FxRecordNewFlag1Program(void)
{
    fx_unused8b_run(0x8BEC, 0xFFFF, GSU_COPY(2));
}
