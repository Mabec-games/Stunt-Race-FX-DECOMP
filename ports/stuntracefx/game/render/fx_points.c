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

/* A signed script byte (GETBS, INC R14). */
/* gsuwalk: the code of each copy of script_byte, read from the ROM */
static const GsuCode script_byte_fetch[7][2] = {
    {{0x04a167, 2}, {0x04a169, 1}},
    {{0x04a19a, 2}, {0x04a19c, 1}},
    {{0x04a19e, 4}, {0x04a1a2, 1}},
    {{0x04a1a4, 4}, {0x04a1a8, 1}},
    {{0x04a1b5, 2}, {0x04a1b7, 1}},
    {{0x04a1b9, 5}, {0x04a1be, 1}},
    {{0x04a1c1, 5}, {0x04a1c6, 1}},
};

static uint16_t script_byte(FxRegs *g, int copy)
{
    gsu_codes(script_byte_fetch[copy], 1);
    uint16_t b = gsu_getbs();

    gsu_codes(script_byte_fetch[copy] + 1, 1);
    gsu_r14(gsu_inc(g->r[14]));
    g->r[14] = gsu.r[14];
    return b;
}

/* A word to R9's list, R9 stepped on (STW, INC, INC). */
/* gsuwalk: the code of each copy of put, read from the ROM */
static const GsuCode put_fetch[7][1] = {
    {{0x04a19d, 1}},
    {{0x04a1a3, 1}},
    {{0x04a1b8, 1}},
    {{0x04a1bf, 2}},
    {{0x04a1c7, 2}},
    {{0x04a1c9, 5}},
    {{0x04a1ce, 4}},
};

static void put(FxRegs *g, uint16_t v, int copy)
{
    gsu_codes(put_fetch[copy], 1);
    gsu_stw(g->r[9], v);
    g->r[9] = gsu_inc(gsu_inc(g->r[9]));
}

/* The count byte for LOOP (GETB to R12) and R13 = the loop's start. */
/* gsuwalk: the code of each copy of count, read from the ROM */
static const GsuCode count_fetch[2][2] = {
    {{0x04a195, 2}, {0x04a197, 1}},
    {{0x04a1b0, 2}, {0x04a1b2, 1}},
};

static void count(FxRegs *g, uint16_t start, int copy)
{
    gsu_codes(count_fetch[copy], 1);
    g->r[12] = gsu_getb();
    gsu_codes(count_fetch[copy] + 1, 1);
    gsu_r14(gsu_inc(g->r[14]));
    g->r[14] = gsu.r[14];
    g->r[13] = start;
}

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

    gsu_code(0x04a155, 2);
    gsu_push(sp, g.r[3]);
    gsu_code(0x04a157, 4);
    gsu_push(sp, g.r[1]);
    gsu_code(0x04a15b, 4);
    gsu_push(sp, g.r[2]);
    gsu_code(0x04a15f, 4);
    gsu_r14(g.r[9]);
    g.r[14] = gsu.r[14];
    g.r[9] = FX_POINTS;
    gsu_code(0x04a163, 3); gsu_cache(0xa166); gsu_code(0x04a166, 1);
    for (;;) {
        g.r[0] = script_byte(&g, GSU_COPY(0));
        g.r[3] = 0x04;
        gsu_sub(g.r[0], g.r[3], 0);
        if (gsu.z) {
            gsu_code(0x04a16a, 6); gsu_code(0x04a194, 1);
            count(&g, 0xA199, GSU_COPY(0));
            gsu_code(0x04a198, 2);
            do {
                put(&g, script_byte(&g, GSU_COPY(1)), GSU_COPY(0));
                put(&g, script_byte(&g, GSU_COPY(2)), GSU_COPY(1));
                g.r[0] = script_byte(&g, GSU_COPY(3));
                gsu_code(0x04a1a9, 1);
                gsu_stw(g.r[9], g.r[0]);
                g.r[9] = gsu_inc(g.r[9]);
                {
                    int again = gsu_count_down(&g.r[12]);

                    g.r[9] = gsu_inc(g.r[9]);
                    if (!again)
                        { gsu_code(0x04a1aa, 2); break; } else { gsu_code(0x04a1aa, 2); gsu_code(0x04a199, 1); }
                }
            } while (1);
            gsu_code(0x04a1ac, 3); gsu_code(0x04a166, 1);
            continue;
        } else { gsu_code(0x04a16a, 6); }
        g.r[3] = 0x38;
        gsu_sub(g.r[0], g.r[3], 0);
        if (gsu.z) {
            gsu_code(0x04a170, 7); gsu_code(0x04a1af, 1);
            count(&g, 0xA1B4, GSU_COPY(1));
            gsu_code(0x04a1b3, 2);
            do {
                g.r[0] = script_byte(&g, GSU_COPY(4));
                put(&g, g.r[0], GSU_COPY(2));
                g.r[1] = script_byte(&g, GSU_COPY(5));
                put(&g, g.r[1], GSU_COPY(3));
                g.r[2] = script_byte(&g, GSU_COPY(6));
                put(&g, g.r[2], GSU_COPY(4));
                g.r[0] = gsu_inc(gsu_not(g.r[0]));
                put(&g, g.r[0], GSU_COPY(5));
                put(&g, g.r[1], GSU_COPY(6));
                gsu_code(0x04a1d2, 4);
                gsu_stw(g.r[9], g.r[2]);
                g.r[9] = gsu_inc(g.r[9]);
                {
                    int again = gsu_count_down(&g.r[12]);

                    g.r[9] = gsu_inc(g.r[9]);
                    if (!again)
                        { gsu_code(0x04a1d6, 2); break; } else { gsu_code(0x04a1d6, 2); gsu_code(0x04a1b4, 1); }
                }
            } while (1);
            gsu_code(0x04a1d8, 3); gsu_code(0x04a166, 1);
            continue;
        } else { gsu_code(0x04a170, 7); }
        g.r[3] = 0x1C;
        gsu_sub(g.r[0], g.r[3], 0);
        if (gsu.z) {
            gsu_code(0x04a177, 7); gsu_code(0x04a1db, 2);
            gsu_r14(gsu_inc(g.r[14]));
            gsu_code(0x04a1dd, 3);
            g.r[0] = gsu_ldw(FX_VARIANT);
            g.r[0] = gsu_add(g.r[0], g.r[0], 0);
            gsu_code(0x04a1e0, 3);
            gsu_r14(gsu_add(gsu.r[14], g.r[0], 0));
            g.r[14] = gsu.r[14];
        } else {
            g.r[3] = 0x20;
            gsu_sub(g.r[0], g.r[3], 0);
            if (!gsu.z)
                { gsu_code(0x04a177, 14); break; } else { gsu_code(0x04a177, 14); gsu_code(0x04a1e2, 1); }
        }
        gsu_code(0x04a1e3, 1);
        w = gsu_getb();
        gsu_code(0x04a1e4, 1);
        gsu_r14(gsu_inc(g.r[14]));
        g.r[14] = gsu.r[14];
        gsu_code(0x04a1e5, 2);
        g.r[0] = gsu_getbh(w);
        gsu_code(0x04a1e7, 2);
        gsu_r14(gsu_add(g.r[14], g.r[0], 0));
        g.r[14] = gsu.r[14];
        gsu_code(0x04a1e9, 3); gsu_code(0x04a166, 1);
    }
    gsu_code(0x04a185, 5);
    g.r[2] = gsu_pull(sp);
    gsu_code(0x04a18a, 4);
    g.r[1] = gsu_pull(sp);
    gsu_code(0x04a18e, 4);
    g.r[3] = gsu_pull(sp);
    gsu_code(0x04a192, 2);
    return g;
}

void abi_FxModelPoints(void)
{
    FxRegs g;

    memcpy(g.r, gsu.r, sizeof g.r);
    g = FxModelPoints(g, GSU_COPY(0));
    memcpy(gsu.r, g.r, 15 * sizeof g.r[0]);
}
