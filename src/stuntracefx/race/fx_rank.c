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

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode rank_walk_code[3][24] = {
    {{0, 0}, {0x04CE71, 1}, {0x04CE72, 5}, {0, 0}, {0x04CECD, 7}, {0x04CE77, 6}, {0x04CE7D, 11}, {0x04CE88, 7}, {0x04CE8F, 7}, {0x04CE96, 10}, {0x04CEA0, 5}, {0x04CE96, 4}, {0x04CEA6, 6}, {0x04CEAC, 3}, {0, 0}, {0x04CEAC, 9}, {0x04CEB5, 3}, {0x04CEB5, 9}, {0x04CEBE, 8}, {0x04CE7D, 4}, {0x04CEC6, 5}, {0x04CECB, 2}, {0, 0}, {0, 0}},
    {{0, 0}, {0x04CE71, 1}, {0x04CE72, 5}, {0, 0}, {0x04CECD, 7}, {0x04CE77, 6}, {0x04CE7D, 11}, {0x04CE88, 7}, {0x04CE8F, 7}, {0x04CE96, 10}, {0x04CEA0, 5}, {0x04CE96, 4}, {0x04CEA6, 6}, {0x04CEAC, 3}, {0x04CEBE, 8}, {0x04CEAC, 9}, {0x04CEB5, 3}, {0x04CEB5, 9}, {0, 0}, {0x04CE7D, 4}, {0x04CEC6, 5}, {0x04CECB, 2}, {0, 0}, {0, 0}},
    {{0x04CEA6, 24}, {0x04CE71, 1}, {0x04CE72, 5}, {0x04CECB, 2}, {0x04CECD, 7}, {0x04CE77, 6}, {0x04CE7D, 11}, {0x04CE88, 7}, {0x04CE8F, 7}, {0x04CE96, 10}, {0x04CEA0, 5}, {0x04CE96, 4}, {0, 0}, {0x04CEAC, 3}, {0, 0}, {0, 0}, {0x04CEB5, 3}, {0, 0}, {0x04CEBE, 8}, {0x04CE7D, 4}, {0x04CEC6, 5}, {0, 0}, {0x04888C, 1}, {0x04CED4, 2}},
};

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
static FxRegs rank_walk(FxRegs g, int at_dec, int copy)
{
    gsu_codes(rank_walk_code[copy], 1);
    if (at_dec)
        goto dec;
    g.r[0] = gsu_sub(g.r[0], g.r[0], 0);
    g.r[2] = g.r[0];
    g.r[3] = g.r[0];
    g.r[1] = 0xFFFF;
    g.r[0] = gsu_ldw(FX_CARS);
    g.r[4] = g.r[0];
    gsu_moves(g.r[4]);
    gsu_codes(rank_walk_code[copy] + 1, 1);
    for (;;) {
        gsu_codes(rank_walk_code[copy] + 2, 2);
        g.r[8] = g.r[0];
        gsu_moves(g.r[8]);
        if (gsu.z)
            { gsu_codes(rank_walk_code[copy] + 4, 1); break; } else { gsu_codes(rank_walk_code[copy] + 5, 1); }
        g.r[0] = gsu_ldw(gsu_add(REC_RANK, g.r[8], 0));
        gsu_moves(g.r[0]);
        if (gsu.z) {
            gsu_codes(rank_walk_code[copy] + 6, 1);
            int take;

            g.r[5] = gsu_ldw(gsu_add(REC_LAPS, g.r[8], 0));
            gsu_codes(rank_walk_code[copy] + 7, 1);
            g.r[6] = gsu_ldw(gsu_add(REC_COUNTED, g.r[8], 0));
            gsu_codes(rank_walk_code[copy] + 8, 1);
            g.r[7] = gsu_ldw(gsu_add(REC_TO_GO, g.r[8], 0));
            gsu_moves(g.r[5]);
            if (gsu.z) {
                gsu_codes(rank_walk_code[copy] + 9, 1);
                g.r[0] = gsu_ldw(gsu_add(REC_COUNTED, g.r[8], 0));
                g.r[0] = gsu_sub(g.r[0], g.r[6], 0);
                g.r[0] = gsu_add(g.r[0], 5, 0);
                if (gsu.s) {
dec:
                    g.r[5] = gsu_dec(g.r[5]);
                } else { gsu_codes(rank_walk_code[copy] + 10, 1); }
            } else { gsu_codes(rank_walk_code[copy] + 11, 1); }
            gsu_codes(rank_walk_code[copy] + 12, 1);
            gsu_sub(g.r[5], g.r[1], 0);
            if (gsu.s)
                take = 0;
            else if (!gsu.z)
                { gsu_codes(rank_walk_code[copy] + 13, 2); take = 1; }
            else {
                gsu_codes(rank_walk_code[copy] + 15, 1);
                gsu_sub(g.r[6], g.r[2], 0);
                if (gsu.s)
                    take = 0;
                else if (!gsu.z)
                    { gsu_codes(rank_walk_code[copy] + 16, 1); take = 1; }
                else {
                    gsu_codes(rank_walk_code[copy] + 17, 1);
                    gsu_sub(g.r[7], g.r[3], 0);
                    take = gsu.s;
                }
            }
            if (take) {
                gsu_codes(rank_walk_code[copy] + 18, 1);
                g.r[4] = g.r[8];
                g.r[1] = g.r[5];
                g.r[2] = g.r[6];
                g.r[3] = g.r[7];
            }
        } else { gsu_codes(rank_walk_code[copy] + 19, 1); }
        gsu_codes(rank_walk_code[copy] + 20, 1);
        g.r[0] = gsu_ldw(gsu_add(REC_NEXT_CAR, g.r[8], 0));
        gsu_codes(rank_walk_code[copy] + 21, 1);
    }
    g.r[0] = gsu_add(REC_RANK, g.r[4], 0);
    gsu_stw(g.r[0], g.r[9]);
    gsu_codes(rank_walk_code[copy] + 22, 2);
    return g;
}

/* cyccopy: the copy each of the caller's copies calls */
static const uint8_t rank_walk_from_FxRankNext[2] = {0, 1};

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode FxRankNext_code[2][4] = {
    {{0, 0}, {0, 0}, {0, 0}, {0, 0}},
    {{0x04CE4E, 6}, {0x04CE65, 12}, {0x04CE54, 5}, {0x04CED4, 2}},
};

FxRegs FxRankNext(FxRegs g, int copy)
{
    gsu_codes(FxRankNext_code[copy], 2);
    return rank_walk(g, 0, GSU_COPY(rank_walk_from_FxRankNext[copy]));
}

void abi_FxRankNext(void)
{
    FxRegs g;

    memcpy(g.r, gsu.r, sizeof g.r);
    g = FxRankNext(g, GSU_COPY(0));
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
    g = rank_walk(g, 1, GSU_COPY(2));
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
FxRegs FxRankCars(FxRegs g, int copy)
{
    gsu_code(0x04CE1E, 2);
    gsu_push(&g.r[10], g.r[11]);
    g.r[5] = 0;
    gsu_code(0x04CE20, 2); gsu_cache(0xCE22); gsu_code(0x04CE22, 7);
    g.r[0] = gsu_ldw(FX_CARS);
    gsu_moves(g.r[0]);
    if (!gsu.z) {
        gsu_code(0x04CE29, 4);
        for (;;) {
            gsu_code(0x04CE2D, 5);
            g.r[8] = g.r[0];
            gsu_moves(g.r[8]);
            if (gsu.z)
                { gsu_code(0x04CE3F, 5); break; } else { gsu_code(0x04CE32, 7); }
            g.r[0] = gsu_add(REC_RANK, g.r[8], 0);
            gsu_stw(g.r[0], g.r[5]);
            gsu_code(0x04CE39, 4);
            g.r[0] = gsu_ldw(gsu_add(REC_NEXT_CAR, g.r[8], 0));
            gsu_code(0x04CE3D, 2);
        }
        g.r[0] = gsu_ldw(FX_CARS);
        g.r[9] = 1;
        gsu_code(0x04CE44, 2); gsu_cache(0xCE46);
        for (;;) {
            gsu_code(0x04CE46, 5);
            g.r[8] = g.r[0];
            gsu_moves(g.r[8]);
            if (gsu.z)
                { gsu_code(0x04CE60, 5); break; } else { gsu_code(0x04CE4B, 3); }
            gsu_push(&g.r[10], g.r[8]);
            g.r[11] = 0xCE54;
            gsu_code(0x04CE4E, 6); gsu_code(0x04CE65, 12);
            g = FxRankNext(g, GSU_COPY(0));
            gsu_code(0x04CE54, 5); gsu_code(0x04CED4, 2);
            g.r[8] = gsu_pull(&g.r[10]);
            g.r[9] = gsu_inc(g.r[9]);
            gsu_code(0x04CE59, 5);
            g.r[0] = gsu_ldw(gsu_add(REC_NEXT_CAR, g.r[8], 0));
            gsu_code(0x04CE5E, 2);
        }
    } else { gsu_code(0x04CE29, 4); }
    gsu_code(0x04CE60, 5);
    gsu_pull(&g.r[10]);
    gsu_code(0x04906E, 1);
    return g;
}

void abi_FxRankCars(void)
{
    FxRegs g;

    memcpy(g.r, gsu.r, sizeof g.r);
    g = FxRankCars(g, GSU_COPY(0));
    memcpy(gsu.r, g.r, 15 * sizeof g.r[0]);
}
