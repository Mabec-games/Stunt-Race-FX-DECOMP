/*
 * fx_init.c: the Super FX program that sets up its RAM.
 */
#include <string.h>
#include "gsu_ops.h"
#include "fx_calls.h"

#define FX_STACK 0x0240u
#define FX_RAM_TEST 0x235Cu      /* $1234 is written and read back here */
#define FX_RAM_BAD 0x26F0u       /* $0F when the read back differs */
#define FX_POOL_FIRST 0x1098u    /* pool chains: link, then size */
#define FX_POOL_SECOND 0x1D70u
#define FX_CLEAR_WORDS 0x0B2Cu   /* words cleared from $1098 */

/*
 * @routine 04:8800 FxInit
 * WHAT IT DOES: A program: tests Super FX RAM (writes $1234 to $235C and
 * reads it back; a mismatch would set $26F0 to $0F: FxInitRamBad,
 * 04:8822, which the game never runs), clears $0B2C words from
 * $1098, makes each record pool one free block ($0CD8 bytes at $1098,
 * $05BE at $1D70), and sets $2636 = $0100, $2380 = $2382 = $2642 = -1,
 * $2472 = 3 and $2732, $26F0, $2B92, $2B94, $2684, $2686, $1090, $2470,
 * $273C, $235C = 0.
 * HOW: RAMB 0; the clearing is STW under LOOP with INC R1 in its delay
 * slot; the test reads the word twice (into R3, then R0) and compares.
 * Entry: the S-CPU starts it. Exit: (at its STOP) R0 = 0; R1 = $1D72; R2
 * = $1234; R3 = the word read back; R10 = $0240; R12 = 0; R13 = $8831;
 * RAMBR = 0; Z, CY set, S, OV clear; the last RAM address $235C.
 */
static FxRegs pools(FxRegs g);
static FxRegs ram_bad(FxRegs g);

FxRegs FxInit(FxRegs g)
{
    g.r[10] = FX_STACK;
    g.r[0] = gsu_sub(g.r[0], g.r[0], 0);
    gsu.rambr = 0;
    g.r[0] = gsu_sub(g.r[0], g.r[0], 0);
    gsu_stw(0x2732, g.r[0]);
    g.r[0] = gsu_sub(g.r[0], g.r[0], 0);
    gsu_stw(FX_RAM_BAD, g.r[0]);
    g.r[1] = FX_RAM_TEST;
    g.r[2] = g.r[0] = 0x1234;
    gsu_stw(g.r[1], g.r[0]);
    g.r[0] = gsu_sub(g.r[0], g.r[0], 0);
    g.r[3] = gsu_ldw(g.r[1]);
    g.r[0] = gsu_ldw(g.r[1]);
    gsu_sub(g.r[0], g.r[2], 0);
    if (!gsu.z)
        return ram_bad(g);
    return pools(g);
}

/* The rest of FxInit after the RAM test (04:8828): clears the pools and
   sets the starting words. */
static FxRegs pools(FxRegs g)
{
    static const uint16_t zeros[7] = {0x2B94, 0x2B92, 0x2684, 0x2686, 0x1090, 0x2470, 0x273C};
    int i;

    g.r[1] = FX_POOL_FIRST;
    g.r[0] = gsu_sub(g.r[0], g.r[0], 0);
    g.r[12] = FX_CLEAR_WORDS;
    g.r[13] = 0x8831;
    do {
        int again;

        gsu_stw(g.r[1], g.r[0]);
        g.r[1] = gsu_inc(g.r[1]);
        again = gsu_count_down(&g.r[12]);
        g.r[1] = gsu_inc(g.r[1]);
        if (!again)
            break;
    } while (1);
    g.r[0] = 0x0100;
    gsu_stw(0x2636, g.r[0]);
    g.r[0] = 0xFFFF;
    gsu_stw(0x2382, g.r[0]);
    gsu_stw(0x2380, g.r[0]);
    g.r[0] = 0xFFFF;
    gsu_stw(0x2642, g.r[0]);
    g.r[1] = FX_POOL_FIRST;
    g.r[0] = gsu_sub(g.r[0], g.r[0], 0);
    gsu_stw(g.r[1], g.r[0]);
    g.r[1] = gsu_inc(gsu_inc(g.r[1]));
    g.r[0] = 0x0CD8;
    gsu_stw(g.r[1], g.r[0]);
    g.r[1] = FX_POOL_SECOND;
    g.r[0] = gsu_sub(g.r[0], g.r[0], 0);
    gsu_stw(g.r[1], g.r[0]);
    g.r[1] = gsu_inc(gsu_inc(g.r[1]));
    g.r[0] = 0x05BE;
    gsu_stw(g.r[1], g.r[0]);
    g.r[0] = gsu_sub(g.r[0], g.r[0], 0);
    for (i = 0; i < 7; i++)
        gsu_stw(zeros[i], g.r[0]);
    g.r[0] = 3;
    gsu_stw(0x2472, g.r[0]);
    g.r[0] = 0;
    gsu_stw(FX_RAM_TEST, g.r[0]);
    return g;
}

/*
 * @routine 04:8822 FxInitRamBad
 * WHAT IT DOES: FxInit's RAM test failure: $26F0 = $0F, then the rest of
 * FxInit (the pools and the starting words). The game never runs it: the
 * word FxInit writes reads back the same on every machine it runs on, so
 * the BEQ at 04:881F always branches past it. A unit program start runs
 * it so the C is judged against the original.
 * HOW: IBT R0,#$0F; SM ($26F0); then as FxInit from 04:8828.
 * Entry: started here as a program (R10 as the start leaves it). Exit:
 * as FxInit's, with R2, R3 and R10 as at the start.
 */
static FxRegs ram_bad(FxRegs g)
{
    g.r[0] = 0x000F;
    gsu_stw(FX_RAM_BAD, g.r[0]);
    return pools(g);
}

void abi_FxInitRamBad(void)
{
    FxRegs g;

    memcpy(g.r, gsu.r, sizeof g.r);
    g = ram_bad(g);
    memcpy(gsu.r, g.r, 15 * sizeof g.r[0]);
}

void abi_FxInit(void)
{
    FxRegs g;

    memcpy(g.r, gsu.r, sizeof g.r);
    g = FxInit(g);
    memcpy(gsu.r, g.r, 15 * sizeof g.r[0]);
}
