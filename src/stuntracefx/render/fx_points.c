/*
 * fx_points.c: the Super FX unpacks a model's points from a byte script in
 * ROM into the list at $70:2488, three words (x, y, z) a point.
 *
 * The script is read through the ROM buffer. Commands: 4, a count, then
 * that many points as three signed bytes; $38, a count, then that many
 * points each stored twice, as (x, y, z) and (-x, y, z); $20, a word
 * added to R14 (a jump, counted from the word's second byte); $1C, a skipped
 * byte, then a jump through a table of such words indexed by the word at
 * $0044. Anything else ends the script.
 */
#include <string.h>
#include "gsu_ops.h"
#include "fx_calls.h"

#define FX_POINTS 0x2488u
#define FX_VARIANT 0x0044u

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode script_byte_code[3][2] = {
    {{0x04A169, 1}, {0x04A16A, 6}},
    {{0x04A1B7, 1}, {0, 0}},
    {{0, 0}, {0, 0}},
};

/* A signed script byte (GETBS, INC R14). */
static uint16_t script_byte(FxRegs *g, int copy)
{
    uint16_t b = gsu_getbs();

    gsu_codes(script_byte_code[copy], 1);
    gsu_r14(gsu_inc(g->r[14]));
    g->r[14] = gsu.r[14];
    gsu_codes(script_byte_code[copy] + 1, 1);
    return b;
}

/* A word to R9's list, R9 stepped on (STW, INC, INC). */
static void put(FxRegs *g, uint16_t v, int copy)
{
    gsu_stw(g->r[9], v);
    g->r[9] = gsu_inc(gsu_inc(g->r[9]));
}

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode count_code[2][1] = {
    {{0x04A1B2, 1}},
    {{0, 0}},
};

/* The count byte for LOOP (GETB to R12) and R13 = the loop's start. */
static void count(FxRegs *g, uint16_t start, int copy)
{
    g->r[12] = gsu_getb();
    gsu_codes(count_code[copy], 1);
    gsu_r14(gsu_inc(g->r[14]));
    g->r[14] = gsu.r[14];
    g->r[13] = start;
}

/* cyccopy: the copy each of the caller's copies calls */
static const uint8_t script_byte_from_FxModelPoints[4] = {1, 2, 0, 2};

/* cyccopy: the copy each of the caller's copies calls */
static const uint8_t count_from_FxModelPoints[4] = {0, 1, 0, 1};

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode FxModelPoints_code[4][44] = {
    {{0x04A157, 4}, {0x04A15B, 4}, {0x04A15F, 7}, {0xA166, 0}, {0, 0}, {0x04A166, 3}, {0x04A194, 5}, {0, 0}, {0, 0}, {0x04A199, 5}, {0, 0}, {0x04A19E, 4}, {0x04A1A2, 2}, {0x04A1A4, 4}, {0x04A1A8, 2}, {0x04A1AA, 2}, {0x04A1AC, 3}, {0x04A170, 7}, {0x04A1AF, 3}, {0x04A1B3, 1}, {0x04A1B4, 3}, {0x04A1B8, 1}, {0x04A1B9, 8}, {0x04A1C1, 8}, {0x04A1C9, 5}, {0x04A1CE, 4}, {0x04A1D2, 4}, {0x04A1D6, 2}, {0x04A1D8, 3}, {0, 0}, {0, 0}, {0x04A177, 7}, {0x04A1DB, 5}, {0x04A1E0, 3}, {0x04A17E, 7}, {0x04A185, 5}, {0x04A1E2, 1}, {0x04A1E3, 1}, {0x04A1E4, 1}, {0x04A1E5, 2}, {0x04A1E7, 2}, {0x04A1E9, 3}, {0x04A18A, 4}, {0x04A18E, 4}},
    {{0x04A157, 4}, {0x04A15B, 4}, {0x04A15F, 7}, {0xA166, 0}, {0, 0}, {0x04A166, 10}, {0x04A194, 5}, {0, 0}, {0, 0}, {0x04A199, 5}, {0, 0}, {0x04A19E, 4}, {0x04A1A2, 2}, {0x04A1A4, 4}, {0x04A1A8, 2}, {0x04A1AA, 2}, {0x04A1AC, 3}, {0x04A170, 7}, {0x04A1AF, 5}, {0, 0}, {0x04A1B4, 5}, {0, 0}, {0x04A1B9, 8}, {0x04A1C1, 8}, {0x04A1C9, 5}, {0x04A1CE, 4}, {0x04A1D2, 4}, {0x04A1D6, 2}, {0x04A1D8, 3}, {0, 0}, {0, 0}, {0x04A177, 19}, {0x04A1DB, 5}, {0x04A1E0, 3}, {0, 0}, {0, 0}, {0x04A1E2, 1}, {0x04A1E3, 1}, {0x04A1E4, 1}, {0x04A1E5, 2}, {0x04A1E7, 2}, {0x04A1E9, 3}, {0x04A18A, 4}, {0x04A18E, 4}},
    {{0x04A157, 4}, {0x04A15B, 4}, {0x04A15F, 7}, {0xA166, 0}, {0x04A1DB, 5}, {0x04A166, 3}, {0x04A16A, 6}, {0x04A194, 3}, {0x04A198, 1}, {0x04A199, 3}, {0x04A19D, 1}, {0x04A19E, 4}, {0x04A1A2, 2}, {0x04A1A4, 4}, {0x04A1A8, 2}, {0x04A1AA, 2}, {0x04A1AC, 8}, {0x04A16A, 13}, {0, 0}, {0, 0}, {0x04A1B4, 5}, {0, 0}, {0x04A1B9, 8}, {0x04A1C1, 8}, {0x04A1C9, 5}, {0x04A1CE, 4}, {0x04A1D2, 4}, {0x04A1D6, 2}, {0x04A185, 5}, {0x04A1D8, 3}, {0x04A1E2, 1}, {0x04A177, 7}, {0, 0}, {0x04A1E0, 3}, {0x04A17E, 7}, {0, 0}, {0, 0}, {0x04A1E3, 1}, {0x04A1E4, 1}, {0x04A1E5, 2}, {0x04A1E7, 2}, {0x04A1E9, 3}, {0x04A18A, 4}, {0x04A18E, 4}},
    {{0x04A157, 4}, {0x04A15B, 4}, {0x04A15F, 7}, {0xA166, 0}, {0x04A1AF, 5}, {0x04A166, 17}, {0x04A194, 5}, {0, 0}, {0, 0}, {0x04A199, 5}, {0, 0}, {0x04A19E, 4}, {0x04A1A2, 2}, {0x04A1A4, 4}, {0x04A1A8, 2}, {0x04A1AA, 2}, {0x04A1AC, 3}, {0, 0}, {0, 0}, {0, 0}, {0x04A1B4, 5}, {0, 0}, {0x04A1B9, 8}, {0x04A1C1, 8}, {0x04A1C9, 5}, {0x04A1CE, 4}, {0x04A1D2, 4}, {0x04A1D6, 2}, {0x04A177, 19}, {0x04A1D8, 3}, {0, 0}, {0, 0}, {0x04A1DB, 5}, {0x04A1E0, 3}, {0, 0}, {0, 0}, {0x04A1E2, 1}, {0x04A1E3, 1}, {0x04A1E4, 1}, {0x04A1E5, 2}, {0x04A1E7, 2}, {0x04A1E9, 3}, {0x04A18A, 4}, {0x04A18E, 4}},
};

/*
 * @routine 04:A154 FxModelPoints
 * WHAT IT DOES: Unpacks the point script at R9 (in ROMBR's bank) into the
 * list at $2488, as described above.
 * HOW: R14 takes the script and R9 the list; each command byte is GETBS
 * and compared with 4, $38, $1C and $20 in turn. Points go out with STW
 * under LOOP (the last INC R9 in its delay slot); the jumps add to R14.
 * R3, R1 and R2 are kept on the R10 stack. The routine runs under CACHE
 * (the code cache only; nothing the C has to do).
 * Entry: R9 = the script; R10 = stack; R11 = return address. Exit: R9 =
 * just past the last point; R14 = just past the ending byte; R0 = that
 * byte (sign-extended); R1, R2, R3 as on entry; R12, R13 from the last
 * count; S, Z from R10, OV, CY from the compare with $20; the last RAM
 * address R10.
 */
FxRegs FxModelPoints(FxRegs g, int copy)
{
    uint16_t *sp = &g.r[10];
    uint16_t w;

    gsu_push(sp, g.r[3]);
    gsu_codes(FxModelPoints_code[copy], 1);
    gsu_push(sp, g.r[1]);
    gsu_codes(FxModelPoints_code[copy] + 1, 1);
    gsu_push(sp, g.r[2]);
    gsu_codes(FxModelPoints_code[copy] + 2, 3);
    gsu_r14(g.r[9]);
    g.r[14] = gsu.r[14];
    g.r[9] = FX_POINTS;
    for (;;) {
        gsu_codes(FxModelPoints_code[copy] + 5, 1);
        g.r[0] = script_byte(&g, GSU_COPY(script_byte_from_FxModelPoints[copy]));
        g.r[3] = 0x04;
        gsu_sub(g.r[0], g.r[3], 0);
        if (gsu.z) {
            gsu_codes(FxModelPoints_code[copy] + 6, 2);
            count(&g, 0xA199, GSU_COPY(1));
            gsu_codes(FxModelPoints_code[copy] + 8, 1);
            do {
                gsu_codes(FxModelPoints_code[copy] + 9, 1);
                put(&g, script_byte(&g, GSU_COPY(2)), gsu_codes_then(FxModelPoints_code[copy] + 10, 1, GSU_COPY(5)));
                gsu_codes(FxModelPoints_code[copy] + 11, 1);
                put(&g, script_byte(&g, GSU_COPY(2)), gsu_codes_then(FxModelPoints_code[copy] + 12, 1, GSU_COPY(6)));
                gsu_codes(FxModelPoints_code[copy] + 13, 1);
                g.r[0] = script_byte(&g, GSU_COPY(2));
                gsu_codes(FxModelPoints_code[copy] + 14, 1);
                gsu_stw(g.r[9], g.r[0]);
                g.r[9] = gsu_inc(g.r[9]);
                {
                    gsu_codes(FxModelPoints_code[copy] + 15, 1);
                    int again = gsu_count_down(&g.r[12]);

                    g.r[9] = gsu_inc(g.r[9]);
                    if (!again)
                        { gsu_codes(FxModelPoints_code[copy] + 16, 1); break; }
                }
            } while (1);
            continue;
        } else { gsu_codes(FxModelPoints_code[copy] + 17, 1); }
        g.r[3] = 0x38;
        gsu_sub(g.r[0], g.r[3], 0);
        if (gsu.z) {
            gsu_codes(FxModelPoints_code[copy] + 18, 1);
            count(&g, 0xA1B4, GSU_COPY(count_from_FxModelPoints[copy]));
            gsu_codes(FxModelPoints_code[copy] + 19, 1);
            do {
                gsu_codes(FxModelPoints_code[copy] + 20, 1);
                g.r[0] = script_byte(&g, GSU_COPY(script_byte_from_FxModelPoints[copy]));
                gsu_codes(FxModelPoints_code[copy] + 21, 1);
                put(&g, g.r[0], GSU_COPY(0));
                gsu_codes(FxModelPoints_code[copy] + 22, 1);
                g.r[1] = script_byte(&g, GSU_COPY(2));
                put(&g, g.r[1], GSU_COPY(1));
                gsu_codes(FxModelPoints_code[copy] + 23, 1);
                g.r[2] = script_byte(&g, GSU_COPY(2));
                put(&g, g.r[2], GSU_COPY(2));
                g.r[0] = gsu_inc(gsu_not(g.r[0]));
                gsu_codes(FxModelPoints_code[copy] + 24, 1);
                put(&g, g.r[0], GSU_COPY(3));
                gsu_codes(FxModelPoints_code[copy] + 25, 1);
                put(&g, g.r[1], GSU_COPY(4));
                gsu_codes(FxModelPoints_code[copy] + 26, 1);
                gsu_stw(g.r[9], g.r[2]);
                g.r[9] = gsu_inc(g.r[9]);
                {
                    gsu_codes(FxModelPoints_code[copy] + 27, 1);
                    int again = gsu_count_down(&g.r[12]);

                    g.r[9] = gsu_inc(g.r[9]);
                    if (!again)
                        { gsu_codes(FxModelPoints_code[copy] + 28, 3); break; }
                }
            } while (1);
            continue;
        } else { gsu_codes(FxModelPoints_code[copy] + 31, 1); }
        g.r[3] = 0x1C;
        gsu_sub(g.r[0], g.r[3], 0);
        if (gsu.z) {
            gsu_codes(FxModelPoints_code[copy] + 32, 1);
            gsu_r14(gsu_inc(g.r[14]));
            g.r[0] = gsu_ldw(FX_VARIANT);
            g.r[0] = gsu_add(g.r[0], g.r[0], 0);
            gsu_codes(FxModelPoints_code[copy] + 33, 1);
            gsu_r14(gsu_add(gsu.r[14], g.r[0], 0));
            g.r[14] = gsu.r[14];
        } else {
            gsu_codes(FxModelPoints_code[copy] + 34, 1);
            g.r[3] = 0x20;
            gsu_sub(g.r[0], g.r[3], 0);
            if (!gsu.z)
                { gsu_codes(FxModelPoints_code[copy] + 35, 1); break; } else { gsu_codes(FxModelPoints_code[copy] + 36, 1); }
        }
        gsu_codes(FxModelPoints_code[copy] + 37, 1);
        w = gsu_getb();
        gsu_codes(FxModelPoints_code[copy] + 38, 1);
        gsu_r14(gsu_inc(g.r[14]));
        g.r[14] = gsu.r[14];
        gsu_codes(FxModelPoints_code[copy] + 39, 1);
        g.r[0] = gsu_getbh(w);
        gsu_codes(FxModelPoints_code[copy] + 40, 1);
        gsu_r14(gsu_add(g.r[14], g.r[0], 0));
        g.r[14] = gsu.r[14];
        gsu_codes(FxModelPoints_code[copy] + 41, 1);
    }
    g.r[2] = gsu_pull(sp);
    gsu_codes(FxModelPoints_code[copy] + 42, 1);
    g.r[1] = gsu_pull(sp);
    gsu_codes(FxModelPoints_code[copy] + 43, 1);
    g.r[3] = gsu_pull(sp);
    return g;
}

void abi_FxModelPoints(void)
{
    FxRegs g;

    memcpy(g.r, gsu.r, sizeof g.r);
    g = FxModelPoints(g, GSU_COPY(0));
    memcpy(gsu.r, g.r, 15 * sizeof g.r[0]);
}
