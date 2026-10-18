/*
 * fx_rank.c: the Super FX ranks the cars into race positions.
 *
 * The cars' Super FX records are a list from the word at $70:262E, linked
 * through +$74. A car's rank goes to +$88 (1 = leading, 0 = not ranked
 * yet); it is ordered by laps (+$7A), then the last segment counted
 * (+$86), both higher first, then +$84, lower first.
 */
#include <string.h>
#include "gsu_ops.h"
#include "fx_calls.h"

#define FX_CARS 0x262Eu

#define REC_NEXT_CAR 0x74u
#define REC_LAPS 0x7Au
#define REC_TO_GO 0x84u
#define REC_COUNTED 0x86u
#define REC_RANK 0x88u

/*
 * @routine 04:CE65 FxRankNext
 * WHAT IT DOES: Gives rank R9 to the leading car not ranked yet (the first
 * car when none is).
 * HOW: A walk of the list keeping the best keys in R1-R3 (starting at -1,
 * 0, 0) and its car in R4; each unranked car's keys go to R5-R7 and are
 * compared in turn with CMP and the sign (BMI). A car on lap 0 has a
 * correction (its lap count lowered when +$86 - +$86 + 5 is negative,
 * which it never is: FxRankNextLapZero, 04:CEA5).
 * Entry: R9 = the rank; R11 = return address. Exit: R0 = the car + $88; R4
 * = the car; R1-R3 = its keys; R5-R7 = the last unranked car's keys; R8 =
 * 0; OV, S, CY, Z from the ADD of R0; the last RAM address the car + $88.
 */
static FxRegs rank_walk(FxRegs g, int at_dec)
{
    if (at_dec)
        goto dec;
    g.r[0] = gsu_sub(g.r[0], g.r[0], 0);
    g.r[2] = g.r[0];
    g.r[3] = g.r[0];
    g.r[1] = 0xFFFF;
    g.r[0] = gsu_ldw(FX_CARS);
    g.r[4] = g.r[0];
    gsu_moves(g.r[4]);
    for (;;) {
        g.r[8] = g.r[0];
        gsu_moves(g.r[8]);
        if (gsu.z)
            break;
        g.r[0] = gsu_ldw(gsu_add(REC_RANK, g.r[8], 0));
        gsu_moves(g.r[0]);
        if (gsu.z) {
            int take;

            g.r[5] = gsu_ldw(gsu_add(REC_LAPS, g.r[8], 0));
            g.r[6] = gsu_ldw(gsu_add(REC_COUNTED, g.r[8], 0));
            g.r[7] = gsu_ldw(gsu_add(REC_TO_GO, g.r[8], 0));
            gsu_moves(g.r[5]);
            if (gsu.z) {
                g.r[0] = gsu_ldw(gsu_add(REC_COUNTED, g.r[8], 0));
                g.r[0] = gsu_sub(g.r[0], g.r[6], 0);
                g.r[0] = gsu_add(g.r[0], 5, 0);
                if (gsu.s) {
dec:
                    g.r[5] = gsu_dec(g.r[5]);
                }
            }
            gsu_sub(g.r[5], g.r[1], 0);
            if (gsu.s)
                take = 0;
            else if (!gsu.z)
                take = 1;
            else {
                gsu_sub(g.r[6], g.r[2], 0);
                if (gsu.s)
                    take = 0;
                else if (!gsu.z)
                    take = 1;
                else {
                    gsu_sub(g.r[7], g.r[3], 0);
                    take = gsu.s;
                }
            }
            if (take) {
                g.r[4] = g.r[8];
                g.r[1] = g.r[5];
                g.r[2] = g.r[6];
                g.r[3] = g.r[7];
            }
        }
        g.r[0] = gsu_ldw(gsu_add(REC_NEXT_CAR, g.r[8], 0));
    }
    g.r[0] = gsu_add(REC_RANK, g.r[4], 0);
    gsu_stw(g.r[0], g.r[9]);
    return g;
}

FxRegs FxRankNext(FxRegs g)
{
    return rank_walk(g, 0);
}

void abi_FxRankNext(void)
{
    FxRegs g;

    memcpy(g.r, gsu.r, sizeof g.r);
    g = FxRankNext(g);
    memcpy(gsu.r, g.r, 15 * sizeof g.r[0]);
}

/*
 * @routine 04:CEA5 FxRankNextLapZero
 * WHAT IT DOES: FxRankNext's lap 0 correction, entered on its own: the
 * lap count R5 lowered by one, then FxRankNext's compare of this car's
 * keys and the rest of its walk. The game never runs it: the test before
 * it subtracts a car's +$86 from itself, so +5 is never negative and the
 * BPL at 04:CEA2 always branches past it. A unit start runs it so the C
 * is judged against the original.
 * HOW: DEC R5; then as FxRankNext from its compare (04:CEA6).
 * Entry: R1-R3 = the best keys so far, R4 = their car; R5-R7 = car R8's
 * keys; R9 = the rank; R11 = return address. Exit: as FxRankNext's.
 */
void abi_FxRankNextLapZero(void)
{
    FxRegs g;

    memcpy(g.r, gsu.r, sizeof g.r);
    g = rank_walk(g, 1);
    memcpy(gsu.r, g.r, 15 * sizeof g.r[0]);
}

/*
 * @routine 04:CE1D FxRankCars
 * WHAT IT DOES: Ranks all the cars: clears every +$88, then FxRankNext once
 * per car with ranks 1, 2, 3, ...
 * HOW: Two walks of the list; the second keeps the walking car on the R10
 * stack across each call, as the return address is kept across the
 * routine. It runs under CACHE (the code cache only; nothing the C has to
 * do).
 * Entry: R10 = stack; R11 = return address. Exit: R0 = R8 = 0; R5 = 0;
 * with cars: R9 = their number + 1, R1-R7 from the last FxRankNext, R11 =
 * $CE54; S, Z from R10; the last RAM address R10.
 */
FxRegs FxRankCars(FxRegs g)
{
    gsu_push(&g.r[10], g.r[11]);
    g.r[5] = 0;
    g.r[0] = gsu_ldw(FX_CARS);
    gsu_moves(g.r[0]);
    if (!gsu.z) {
        for (;;) {
            g.r[8] = g.r[0];
            gsu_moves(g.r[8]);
            if (gsu.z)
                break;
            g.r[0] = gsu_add(REC_RANK, g.r[8], 0);
            gsu_stw(g.r[0], g.r[5]);
            g.r[0] = gsu_ldw(gsu_add(REC_NEXT_CAR, g.r[8], 0));
        }
        g.r[0] = gsu_ldw(FX_CARS);
        g.r[9] = 1;
        for (;;) {
            g.r[8] = g.r[0];
            gsu_moves(g.r[8]);
            if (gsu.z)
                break;
            gsu_push(&g.r[10], g.r[8]);
            g.r[11] = 0xCE54;
            g = FxRankNext(g);
            g.r[8] = gsu_pull(&g.r[10]);
            g.r[9] = gsu_inc(g.r[9]);
            g.r[0] = gsu_ldw(gsu_add(REC_NEXT_CAR, g.r[8], 0));
        }
    }
    gsu_pull(&g.r[10]);
    return g;
}

void abi_FxRankCars(void)
{
    FxRegs g;

    memcpy(g.r, gsu.r, sizeof g.r);
    g = FxRankCars(g);
    memcpy(gsu.r, g.r, 15 * sizeof g.r[0]);
}
