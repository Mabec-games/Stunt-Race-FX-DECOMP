/*
 * fx_motion.c: the Super FX moves an object's record by its velocity.
 *
 * A record (R8) holds a velocity at +$2E/+$30/+$32 and the position it
 * moves at +$1E/+$20/+$22; an object of kind 6 (R1) also has a second pair,
 * a rate at +$34/+$36/+$38 for the three words at +$24/+$26/+$28. Bit 3 of
 * +$1C halves every step.
 */
#include "gsu_ops.h"

#define REC_POS 0x1Eu
#define REC_FLAGS1C 0x1Cu
#define REC_VEL 0x2Eu

typedef struct {
    uint16_t r0, r2, r3, r5;
} FxMoveRegs;

/* One word: the rate at *rate, halved `halvings` times with DIV2, added to
   the word at *word; both pointers step on. */
static void move_word(FxMoveRegs *g, int halvings)
{
    int i;

    g->r0 = gsu_ldw(g->r5);
    g->r5 = gsu_inc(gsu_inc(g->r5));
    for (i = 0; i < halvings; i++)
        g->r0 = gsu_div2(g->r0);
    g->r3 = gsu_ldw(g->r2);
    g->r0 = gsu_add(g->r0, g->r3, 0);
    gsu_stw(g->r2, g->r0);
    g->r2 = gsu_inc(gsu_inc(g->r2));
}

/*
 * @routine 04:95BE FxRecordMove
 * WHAT IT DOES: Adds record R8's velocity / 8 to its position (/ 16 when
 * bit 3 of +$1C is set); for kind 6 (R1) also adds the rate at +$34 to the
 * words at +$24 (halved when the bit is set).
 * HOW: R5 walks the rates and R2 the words; each word is LDW, DIV2s, LDW,
 * ADD, STW in that order. The halvings are DIV2 (rounding toward minus
 * infinity, -1 to 0) applied once per halving.
 * Entry: R1 = the object's kind; R8 = the record; R11 = return address.
 * Exit: R2, R5 just past the last word and rate; R3 = the last word before
 * the add; kind 6: R0 = the last new word, S, Z from R2, OV, CY from the
 * add; otherwise R0 = kind - 6 and the flags from that SUB. The last RAM
 * address the last word.
 */
FxMoveRegs FxRecordMove(uint16_t rec, uint16_t kind)
{
    FxMoveRegs g;
    int halve, i;

    g.r5 = gsu_add(REC_VEL, rec, 0);
    g.r2 = gsu_add(REC_POS, rec, 0);
    g.r3 = gsu_ldw(gsu_add(REC_FLAGS1C, rec, 0));
    g.r0 = gsu_and(0x0008, g.r3);
    halve = !gsu.z;
    for (i = 0; i < 3; i++)
        move_word(&g, halve ? 4 : 3);
    g.r0 = gsu_sub(kind, 6, 0);
    if (gsu.z)
        for (i = 0; i < 3; i++)
            move_word(&g, halve ? 1 : 0);
    return g;
}

void abi_FxRecordMove(void)
{
    FxMoveRegs g = FxRecordMove(gsu.r[8], gsu.r[1]);

    gsu.r[0] = g.r0;
    gsu.r[2] = g.r2;
    gsu.r[3] = g.r3;
    gsu.r[5] = g.r5;
}
