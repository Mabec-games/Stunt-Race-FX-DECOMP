/*
 * effects.c: the Super FX's short-lived screen effects. Sixteen records of
 * six bytes at $2688: +$00 the time left (0: the record is free), +$01 a
 * byte, +$02 X and +$04 Y on the screen (words).
 */
#include "gsu.h"
#include "gsu_ops.h"

#define FX_EFFECTS 0x2688u
#define FX_EFFECT_SIZE 6u
#define FX_EFFECT_COUNT 0x10u
#define FX_EDGES 0x0038u        /* the screen's edges: X at $38 and $3A, Y at $3C and $3E */
#define FX_CENTRE_X 0x0034u
#define FX_TIED_RECORD 0x247Au  /* the address of the tied object's record */

/*
 * @routine 01:C060 FxEffectsAge
 * WHAT IT DOES: Counts down the time of each effect in use and frees the
 * ones outside the screen less a margin of 8: X below $38 + 8 or not below
 * $3A - 8, Y below $3C + 8 or not below $3E - 8.
 * HOW: The margins into R8, R7, R4 and R5 (ADD #8, SUB #8). R6 = 8 times
 * the word at +$4A of the record at $247A, or 0 when that is negative
 * (nothing here uses it). A LOOP over the sixteen records, R13 = its
 * start ($C095), R3 stepping by 6: a record whose time (LDB, LOB) is not
 * 0 has it counted down (DEC, STB); then R2 = its byte, R9 = $0034
 * (unused), R1 = X, R2 = Y, and CMP against each margin; one outside has
 * its first word set to 0. WITH then TO is a MOVE, WITH then FROM a
 * MOVES. Returns by a MOVE of R11 to R15.
 * Entry: R11 = return address. Exit: R3 = $26E8; R12 = 0; R13 = $C095;
 * R8, R7, R4, R5 the margins; R6 as above; R0, R1, R2, R9 and the OV and
 * CY flags as the last record in use, or the last free record, left them;
 * S clear, Z set (LOOP).
 */
void FxEffectsAge(void)
{
    gsu.r[0] = gsu_ldw(FX_EDGES);
    gsu.r[8] = gsu_add(gsu.r[0], 8, 0);
    gsu.r[0] = gsu_ldw(FX_EDGES + 2);
    gsu.r[7] = gsu_sub(gsu.r[0], 8, 0);
    gsu.r[0] = gsu_ldw(FX_EDGES + 4);
    gsu.r[4] = gsu_add(gsu.r[0], 8, 0);
    gsu.r[0] = gsu_ldw(FX_EDGES + 6);
    gsu.r[5] = gsu_sub(gsu.r[0], 8, 0);
    gsu.r[6] = 0x0000;
    gsu.r[1] = gsu_ldw(FX_TIED_RECORD);
    gsu.r[0] = gsu_add(0x004A, gsu.r[1], 0);
    gsu.r[0] = gsu_ldw(gsu.r[0]);
    gsu_moves(gsu.r[0]);
    if (!gsu.s) {
        gsu.r[0] = gsu_add(gsu.r[0], gsu.r[0], 0);
        gsu.r[0] = gsu_add(gsu.r[0], gsu.r[0], 0);
        gsu.r[0] = gsu_add(gsu.r[0], gsu.r[0], 0);
        gsu.r[6] = gsu.r[0];
    }
    gsu.r[3] = FX_EFFECTS;
    gsu.r[12] = FX_EFFECT_COUNT;
    gsu.r[13] = 0xC095;
    do {
        int out;

        gsu.r[0] = gsu_lob(gsu_ldb(gsu.r[3]));
        gsu_moves(gsu.r[0]);
        if (gsu.z) {
            gsu.r[3] = gsu_add(gsu.r[3], FX_EFFECT_SIZE, 0);
            continue;
        }
        gsu.r[0] = gsu_dec(gsu.r[0]);
        gsu_stb(gsu.r[3], (uint8_t)gsu.r[0]);
        gsu.r[3] = gsu_inc(gsu.r[3]);
        gsu.r[2] = gsu_ldb(gsu.r[3]);
        gsu.r[3] = gsu_inc(gsu.r[3]);
        gsu.r[9] = gsu_ldw(FX_CENTRE_X);
        gsu.r[1] = gsu_ldw(gsu.r[3]);
        gsu.r[3] = gsu_inc(gsu_inc(gsu.r[3]));
        gsu.r[0] = gsu_lob(gsu.r[2]);
        gsu.r[2] = gsu_ldw(gsu.r[3]);
        gsu.r[3] = gsu_inc(gsu_inc(gsu.r[3]));
        gsu.r[0] = gsu.r[1];
        gsu_sub(gsu.r[0], gsu.r[8], 0);
        out = gsu.s;
        if (!out) {
            gsu_sub(gsu.r[0], gsu.r[7], 0);
            out = !gsu.s;
        }
        if (!out) {
            gsu.r[0] = gsu.r[2];
            gsu_sub(gsu.r[0], gsu.r[4], 0);
            out = gsu.s;
            if (!out) {
                gsu_sub(gsu.r[0], gsu.r[5], 0);
                out = !gsu.s;
            }
        }
        if (out) {
            gsu.r[3] = gsu_sub(gsu.r[3], FX_EFFECT_SIZE, 0);
            gsu.r[0] = gsu_sub(gsu.r[0], gsu.r[0], 0);
            gsu_stw(gsu.r[3], gsu.r[0]);
            gsu.r[3] = gsu_add(gsu.r[3], FX_EFFECT_SIZE, 0);
        }
    } while (gsu_count_down(&gsu.r[12]));
}

void abi_FxEffectsAge(void)
{
    FxEffectsAge();
}
