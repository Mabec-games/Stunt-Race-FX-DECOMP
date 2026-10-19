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

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode move_word_code[4][3] = {
    {{0, 0}, {0, 0}, {0x049643, 10}},
    {{0, 0}, {0, 0}, {0, 0}},
    {{0x0495CF, 6}, {0, 0}, {0, 0}},
    {{0x0495CF, 6}, {0x049632, 2}, {0, 0}},
};

/* One word: the rate at *rate, halved `halvings` times with DIV2, added to
   the word at *word; both pointers step on. */
static void move_word(FxMoveRegs *g, int halvings, int copy)
{
    gsu_codes(move_word_code[copy], 2);
    int i;

    g->r0 = gsu_ldw(g->r5);
    g->r5 = gsu_inc(gsu_inc(g->r5));
    gsu_codes(move_word_code[copy] + 2, 1);
    for (i = 0; i < halvings; i++)
        g->r0 = gsu_div2(g->r0);
    g->r3 = gsu_ldw(g->r2);
    g->r0 = gsu_add(g->r0, g->r3, 0);
    gsu_stw(g->r2, g->r0);
    g->r2 = gsu_inc(gsu_inc(g->r2));
}

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode FxRecordMove_code[8][15] = {
    {{0, 0}, {0, 0}, {0x0495CF, 6}, {0, 0}, {0x0495D5, 2}, {0x049632, 2}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x049607, 9}, {0x04965E, 9}, {0, 0}, {0, 0}, {0, 0}},
    {{0x04912E, 6}, {0x0495BE, 17}, {0x0495CF, 6}, {0x049632, 2}, {0, 0}, {0, 0}, {0x049134, 5}, {0x0491FF, 3}, {0x04965E, 7}, {0x049680, 3}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
    {{0, 0}, {0, 0}, {0x0495CF, 6}, {0x049632, 2}, {0, 0}, {0, 0}, {0x04965E, 9}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x04967F, 4}, {0x04B242, 3}, {0, 0}},
    {{0, 0}, {0, 0}, {0x0495CF, 6}, {0, 0}, {0x0495D5, 2}, {0x049632, 2}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x049607, 9}, {0x04965E, 9}, {0, 0}, {0, 0}, {0, 0}},
    {{0, 0}, {0, 0}, {0x0495CF, 8}, {0, 0}, {0, 0}, {0, 0}, {0x049607, 9}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x04962E, 4}, {0x049680, 3}, {0x04B242, 3}},
    {{0, 0}, {0, 0}, {0x0495CF, 6}, {0x049632, 2}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
    {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
    {{0, 0}, {0, 0}, {0x0495CF, 6}, {0x049632, 2}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
};

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
FxMoveRegs FxRecordMove(uint16_t rec, uint16_t kind, int copy)
{
    gsu_codes(FxRecordMove_code[copy], 2);
    FxMoveRegs g;
    int halve, i;

    g.r5 = gsu_add(REC_VEL, rec, 0);
    g.r2 = gsu_add(REC_POS, rec, 0);
    g.r3 = gsu_ldw(gsu_add(REC_FLAGS1C, rec, 0));
    g.r0 = gsu_and(0x0008, g.r3);
    halve = !gsu.z;
    gsu_codes(FxRecordMove_code[copy] + 2, 2);
    for (i = 0; i < 3; i++)
        { move_word(&g, halve ? (gsu_codes(FxRecordMove_code[copy] + 4, 1), 4) : (gsu_codes(FxRecordMove_code[copy] + 5, 1), 3), GSU_COPY(0)); gsu_codes(FxRecordMove_code[copy] + 6, 4); }
    g.r0 = gsu_sub(kind, 6, 0);
    if (gsu.z)
        for (i = 0; i < 3; i++)
            { move_word(&g, halve ? (gsu_codes(FxRecordMove_code[copy] + 10, 1), 1) : (gsu_codes(FxRecordMove_code[copy] + 11, 1), 0), GSU_COPY(1)); gsu_codes(FxRecordMove_code[copy] + 12, 3); }
    return g;
}

void abi_FxRecordMove(void)
{
    FxMoveRegs g = FxRecordMove(gsu.r[8], gsu.r[1], GSU_COPY(0));

    gsu.r[0] = g.r0;
    gsu.r[2] = g.r2;
    gsu.r[3] = g.r3;
    gsu.r[5] = g.r5;
}
