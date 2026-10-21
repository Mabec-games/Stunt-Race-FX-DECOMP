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
/* gsuwalk: the code of each copy of rank_walk, read from the ROM */
static const GsuCode rank_walk_fetch[2][41] = {
    {{0x04ce66, 11}, {0x04cecb, 2}, {0x04ce72, 1}, {0x04ce71, 2}, {0x04ce73, 4}, {0, 0}, {0x04ce73, 4}, {0x04cecd, 1}, {0, 0}, {0x04ce77, 6}, {0x04ce7d, 4}, {0x04cec6, 1}, {0x04cebf, 8}, {0x04ce7d, 11}, {0x04ce88, 7}, {0x04ce8f, 7}, {0x04ce96, 4}, {0x04cea6, 1}, {0x04ce96, 10}, {0x04cea0, 5}, {0x04cea6, 1}, {0x04cea5, 2}, {0x04cea0, 5}, {0x04cea7, 5}, {0x04cea7, 5}, {0x04cec6, 1}, {0x04ceac, 3}, {0x04ceac, 3}, {0x04cebe, 1}, {0x04ceaf, 6}, {0x04ceaf, 6}, {0x04cec6, 1}, {0x04ceb5, 3}, {0x04ceb5, 3}, {0x04cebe, 1}, {0x04ceb8, 6}, {0x04ceb8, 6}, {0x04cec6, 1}, {0x04cebe, 1}, {0x04cec7, 4}, {0x04cece, 6}},
    {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x04cecb, 2}, {0x04ce72, 5}, {0x04cecb, 2}, {0x04ce72, 5}, {0x04cecd, 1}, {0x04ce77, 6}, {0x04ce7d, 4}, {0x04cec6, 1}, {0x04cebf, 8}, {0x04ce7d, 11}, {0x04ce88, 7}, {0x04ce8f, 7}, {0x04ce96, 4}, {0x04cea6, 1}, {0x04ce96, 10}, {0x04cea0, 5}, {0x04cea6, 1}, {0x04cea6, 1}, {0x04cea0, 6}, {0x04cea7, 5}, {0x04cea7, 5}, {0x04cec6, 1}, {0x04ceac, 3}, {0x04ceac, 3}, {0x04cebe, 1}, {0x04ceaf, 6}, {0x04ceaf, 6}, {0x04cec6, 1}, {0x04ceb5, 3}, {0x04ceb5, 3}, {0x04cebe, 1}, {0x04ceb8, 6}, {0x04ceb8, 6}, {0x04cec6, 1}, {0x04cebe, 1}, {0x04cec7, 4}, {0x04cece, 6}},
};

static FxRegs rank_walk(FxRegs g, int at_dec, int copy)
{
    if (at_dec)
        goto dec;
    g.r[0] = gsu_sub(g.r[0], g.r[0], 0);
    g.r[2] = g.r[0];
    g.r[3] = g.r[0];
    g.r[1] = 0xFFFF;
    gsu_codes(rank_walk_fetch[copy], 1);
    g.r[0] = gsu_ldw(FX_CARS);
    g.r[4] = g.r[0];
    gsu_moves(g.r[4]);
    gsu_codes(rank_walk_fetch[copy] + 3, 1);
    for (;;) {
        g.r[8] = g.r[0];
        gsu_moves(g.r[8]);
        if (gsu.z)
            { gsu_codes(rank_walk_fetch[copy] + 6, 3); break; } else { gsu_codes(rank_walk_fetch[copy] + 4, 2); }
        gsu_codes(rank_walk_fetch[copy] + 9, 1);
        g.r[0] = gsu_ldw(gsu_add(REC_RANK, g.r[8], 0));
        gsu_moves(g.r[0]);
        if (gsu.z) {
            gsu_codes(rank_walk_fetch[copy] + 13, 1);
            g.r[5] = gsu_ldw(gsu_add(REC_LAPS, g.r[8], 0));
            gsu_codes(rank_walk_fetch[copy] + 14, 1);
            g.r[6] = gsu_ldw(gsu_add(REC_COUNTED, g.r[8], 0));
            gsu_codes(rank_walk_fetch[copy] + 15, 1);
            g.r[7] = gsu_ldw(gsu_add(REC_TO_GO, g.r[8], 0));
            gsu_moves(g.r[5]);
            if (gsu.z) {
                gsu_codes(rank_walk_fetch[copy] + 18, 1);
                g.r[0] = gsu_ldw(gsu_add(REC_COUNTED, g.r[8], 0));
                g.r[0] = gsu_sub(g.r[0], g.r[6], 0);
                g.r[0] = gsu_add(g.r[0], 5, 0);
                if (gsu.s) {
gsu_codes(rank_walk_fetch[copy] + 22, 1);
dec:
                    g.r[5] = gsu_dec(g.r[5]);
                    gsu_codes(rank_walk_fetch[copy] + 21, 1);
                } else { gsu_codes(rank_walk_fetch[copy] + 19, 2); }
            } else { gsu_codes(rank_walk_fetch[copy] + 16, 2); }
            gsu_sub(g.r[5], g.r[1], 0);
            if (gsu.s)
                { gsu_codes(rank_walk_fetch[copy] + 24, 2); goto next; } else { gsu_codes(rank_walk_fetch[copy] + 23, 1); }
            if (!gsu.z)
                { gsu_codes(rank_walk_fetch[copy] + 27, 2); goto take; } else { gsu_codes(rank_walk_fetch[copy] + 26, 1); }
            gsu_sub(g.r[6], g.r[2], 0);
            if (gsu.s)
                { gsu_codes(rank_walk_fetch[copy] + 30, 2); goto next; } else { gsu_codes(rank_walk_fetch[copy] + 29, 1); }
            if (!gsu.z)
                { gsu_codes(rank_walk_fetch[copy] + 33, 2); goto take; } else { gsu_codes(rank_walk_fetch[copy] + 32, 1); }
            gsu_sub(g.r[7], g.r[3], 0);
            if (!gsu.s)
                { gsu_codes(rank_walk_fetch[copy] + 36, 2); goto next; } else { gsu_codes(rank_walk_fetch[copy] + 35, 1); }
gsu_codes(rank_walk_fetch[copy] + 38, 1);
take:
            g.r[4] = g.r[8];
            g.r[1] = g.r[5];
            g.r[2] = g.r[6];
            g.r[3] = g.r[7];
            gsu_codes(rank_walk_fetch[copy] + 12, 1);
        } else { gsu_codes(rank_walk_fetch[copy] + 10, 2); }
next:
        ;
        gsu_codes(rank_walk_fetch[copy] + 39, 1);
        g.r[0] = gsu_ldw(gsu_add(REC_NEXT_CAR, g.r[8], 0));
        gsu_codes(rank_walk_fetch[copy] + 1, 2);
    }
    g.r[0] = gsu_add(REC_RANK, g.r[4], 0);
    gsu_codes(rank_walk_fetch[copy] + 40, 1);
    gsu_stw(g.r[0], g.r[9]);
    return g;
}

FxRegs FxRankNext(FxRegs g, int copy)
{
    { FxRegs ret_ = rank_walk(g, 0, GSU_COPY(0)); gsu_code(0x04ced4, 2); return ret_; }
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
    g = rank_walk(g, 1, GSU_COPY(1));
    memcpy(gsu.r, g.r, 15 * sizeof g.r[0]);
    gsu_code(0x04ced4, 2);
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
    gsu_code(0x04ce1e, 2);
    gsu_push(&g.r[10], g.r[11]);
    g.r[5] = 0;
    gsu_code(0x04ce20, 2); gsu_cache(0xce22); gsu_code(0x04ce22, 7);
    g.r[0] = gsu_ldw(FX_CARS);
    gsu_moves(g.r[0]);
    if (!gsu.z) {
        gsu_code(0x04ce29, 5);
        for (;;) {
            g.r[8] = g.r[0];
            gsu_moves(g.r[8]);
            if (gsu.z)
                { gsu_code(0x04ce2e, 4); gsu_code(0x04ce3f, 1); break; } else { gsu_code(0x04ce2e, 4); }
            g.r[0] = gsu_add(REC_RANK, g.r[8], 0);
            gsu_code(0x04ce32, 7);
            gsu_stw(g.r[0], g.r[5]);
            gsu_code(0x04ce39, 4);
            g.r[0] = gsu_ldw(gsu_add(REC_NEXT_CAR, g.r[8], 0));
            gsu_code(0x04ce3d, 2); gsu_code(0x04ce2d, 1);
        }
        gsu_code(0x04ce40, 4);
        g.r[0] = gsu_ldw(FX_CARS);
        g.r[9] = 1;
        gsu_code(0x04ce44, 2); gsu_cache(0xce46); gsu_code(0x04ce46, 1);
        for (;;) {
            g.r[8] = g.r[0];
            gsu_moves(g.r[8]);
            if (gsu.z)
                { gsu_code(0x04ce47, 4); gsu_code(0x04ce60, 1); break; } else { gsu_code(0x04ce47, 4); }
            gsu_code(0x04ce4b, 3);
            gsu_push(&g.r[10], g.r[8]);
            g.r[11] = 0xCE54;
            gsu_code(0x04ce4e, 6); gsu_code(0x04ce65, 1);
            g = FxRankNext(g, GSU_COPY(0));
            gsu_code(0x04ce54, 5);
            g.r[8] = gsu_pull(&g.r[10]);
            g.r[9] = gsu_inc(g.r[9]);
            gsu_code(0x04ce59, 5);
            g.r[0] = gsu_ldw(gsu_add(REC_NEXT_CAR, g.r[8], 0));
            gsu_code(0x04ce5e, 2); gsu_code(0x04ce46, 1);
        }
    } else { gsu_code(0x04ce29, 4); gsu_code(0x04ce60, 1); }
    gsu_code(0x04ce61, 4);
    gsu_pull(&g.r[10]);
    return g;
}

void abi_FxRankCars(void)
{
    uint16_t ret = gsu.r[11];
    FxRegs g;

    memcpy(g.r, gsu.r, sizeof g.r);
    g = FxRankCars(g, GSU_COPY(0));
    memcpy(gsu.r, g.r, 15 * sizeof g.r[0]);
    gsu_code(0x040000 | ret, 1);
}
