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
static uint16_t script_byte(FxRegs *g)
{
    uint16_t b = gsu_getbs();

    gsu_r14(gsu_inc(g->r[14]));
    g->r[14] = gsu.r[14];
    return b;
}

/* A word to R9's list, R9 stepped on (STW, INC, INC). */
static void put(FxRegs *g, uint16_t v)
{
    gsu_stw(g->r[9], v);
    g->r[9] = gsu_inc(gsu_inc(g->r[9]));
}

/* The count byte for LOOP (GETB to R12) and R13 = the loop's start. */
static void count(FxRegs *g, uint16_t start)
{
    g->r[12] = gsu_getb();
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
FxRegs FxModelPoints(FxRegs g)
{
    uint16_t *sp = &g.r[10];
    uint16_t w;

    gsu_push(sp, g.r[3]);
    gsu_push(sp, g.r[1]);
    gsu_push(sp, g.r[2]);
    gsu_r14(g.r[9]);
    g.r[14] = gsu.r[14];
    g.r[9] = FX_POINTS;
    for (;;) {
        g.r[0] = script_byte(&g);
        g.r[3] = 0x04;
        gsu_sub(g.r[0], g.r[3], 0);
        if (gsu.z) {
            count(&g, 0xA199);
            do {
                put(&g, script_byte(&g));
                put(&g, script_byte(&g));
                g.r[0] = script_byte(&g);
                gsu_stw(g.r[9], g.r[0]);
                g.r[9] = gsu_inc(g.r[9]);
                {
                    int again = gsu_count_down(&g.r[12]);

                    g.r[9] = gsu_inc(g.r[9]);
                    if (!again)
                        break;
                }
            } while (1);
            continue;
        }
        g.r[3] = 0x38;
        gsu_sub(g.r[0], g.r[3], 0);
        if (gsu.z) {
            count(&g, 0xA1B4);
            do {
                g.r[0] = script_byte(&g);
                put(&g, g.r[0]);
                g.r[1] = script_byte(&g);
                put(&g, g.r[1]);
                g.r[2] = script_byte(&g);
                put(&g, g.r[2]);
                g.r[0] = gsu_inc(gsu_not(g.r[0]));
                put(&g, g.r[0]);
                put(&g, g.r[1]);
                gsu_stw(g.r[9], g.r[2]);
                g.r[9] = gsu_inc(g.r[9]);
                {
                    int again = gsu_count_down(&g.r[12]);

                    g.r[9] = gsu_inc(g.r[9]);
                    if (!again)
                        break;
                }
            } while (1);
            continue;
        }
        g.r[3] = 0x1C;
        gsu_sub(g.r[0], g.r[3], 0);
        if (gsu.z) {
            gsu_r14(gsu_inc(g.r[14]));
            g.r[0] = gsu_ldw(FX_VARIANT);
            g.r[0] = gsu_add(g.r[0], g.r[0], 0);
            gsu_r14(gsu_add(gsu.r[14], g.r[0], 0));
            g.r[14] = gsu.r[14];
        } else {
            g.r[3] = 0x20;
            gsu_sub(g.r[0], g.r[3], 0);
            if (!gsu.z)
                break;
        }
        w = gsu_getb();
        gsu_r14(gsu_inc(g.r[14]));
        g.r[14] = gsu.r[14];
        g.r[0] = gsu_getbh(w);
        gsu_r14(gsu_add(g.r[14], g.r[0], 0));
        g.r[14] = gsu.r[14];
    }
    g.r[2] = gsu_pull(sp);
    g.r[1] = gsu_pull(sp);
    g.r[3] = gsu_pull(sp);
    return g;
}

void abi_FxModelPoints(void)
{
    FxRegs g;

    memcpy(g.r, gsu.r, sizeof g.r);
    g = FxModelPoints(g);
    memcpy(gsu.r, g.r, 15 * sizeof g.r[0]);
}
