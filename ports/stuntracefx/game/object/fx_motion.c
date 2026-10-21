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
/* gsuwalk: the code of each copy of move_word, read from the ROM */
static const GsuCode move_word_fetch[12][4] = {
    {{0x0495d6, 1}, {0x0495d7, 10}, {0x0495e1, 2}, {0x0495e3, 2}},
    {{0x0495e5, 3}, {0x0495e8, 10}, {0x0495f2, 2}, {0x0495f4, 2}},
    {{0x0495f6, 3}, {0x0495f9, 10}, {0x049603, 2}, {0x049605, 2}},
    {{0x04960f, 1}, {0x049610, 4}, {0x049614, 2}, {0x049616, 2}},
    {{0x049618, 3}, {0x04961b, 4}, {0x04961f, 2}, {0x049621, 2}},
    {{0x049623, 3}, {0x049626, 4}, {0x04962a, 2}, {0x04962c, 2}},
    {{0x049633, 1}, {0x049634, 8}, {0x04963c, 2}, {0x04963e, 2}},
    {{0x049640, 3}, {0x049643, 8}, {0x04964b, 2}, {0x04964d, 2}},
    {{0x04964f, 3}, {0x049652, 8}, {0x04965a, 2}, {0x04965c, 2}},
    {{0x049666, 1}, {0, 0}, {0x049667, 4}, {0x04966b, 2}},
    {{0x04966d, 3}, {0, 0}, {0x049670, 4}, {0x049674, 2}},
    {{0x049676, 3}, {0, 0}, {0x049679, 4}, {0x04967d, 2}},
};

static void move_word(FxMoveRegs *g, int halvings, int copy)
{
    int i;

    gsu_codes(move_word_fetch[copy], 1);
    g->r0 = gsu_ldw(g->r5);
    g->r5 = gsu_inc(gsu_inc(g->r5));
    gsu_codes(move_word_fetch[copy] + 1, 1);
    for (i = 0; i < halvings; i++)
        g->r0 = gsu_div2(g->r0);
    gsu_codes(move_word_fetch[copy] + 2, 1);
    g->r3 = gsu_ldw(g->r2);
    g->r0 = gsu_add(g->r0, g->r3, 0);
    gsu_codes(move_word_fetch[copy] + 3, 1);
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
/* gsuwalk: the code of each copy of FxRecordMove, read from the ROM */
static const GsuCode FxRecordMove_fetch[1][15] = {
    {{0x0495cf, 7}, {0, 0}, {0, 0}, {0x049607, 8}, {0, 0}, {0, 0}, {0x0495cf, 6}, {0x049632, 1}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x04965e, 8}, {0, 0}, {0, 0}},
};

/* gsuwalk: the copy of move_word each copy of FxRecordMove calls, by pass */
static const uint8_t move_word_in_FxRecordMove[1][3] = {{0, 1, 2}};

/* gsuwalk: the copy of move_word each copy of FxRecordMove calls, by pass */
static const uint8_t move_word_in_FxRecordMove_2[1][3] = {{3, 4, 5}};

/* gsuwalk: the copy of move_word each copy of FxRecordMove calls, by pass */
static const uint8_t move_word_in_FxRecordMove_3[1][3] = {{6, 7, 8}};

/* gsuwalk: the copy of move_word each copy of FxRecordMove calls, by pass */
static const uint8_t move_word_in_FxRecordMove_4[1][3] = {{9, 10, 11}};

FxMoveRegs FxRecordMove(uint16_t rec, uint16_t kind, int copy)
{
    FxMoveRegs g;
    int i;

    g.r5 = gsu_add(REC_VEL, rec, 0);
    g.r2 = gsu_add(REC_POS, rec, 0);
    gsu_code(0x0495bf, 16);
    g.r3 = gsu_ldw(gsu_add(REC_FLAGS1C, rec, 0));
    g.r0 = gsu_and(0x0008, g.r3);
    /* halved or not decides first, each way with its own copies */
    if (!gsu.z) {
        for (i = 0; i < 3; i++)
            { gsu_codes(FxRecordMove_fetch[0] + i, 1); move_word(&g, 4, GSU_COPY(move_word_in_FxRecordMove[0][i])); }
        g.r0 = gsu_sub(kind, 6, 0);
        if (gsu.z)
            { for (i = 0; i < 3; i++)
                { gsu_codes(FxRecordMove_fetch[0] + 3 + i, 1); move_word(&g, 1, GSU_COPY(move_word_in_FxRecordMove_2[0][i])); } gsu_code(0x04962e, 2); } else { gsu_code(0x049607, 7); gsu_code(0x04962f, 1); }
        gsu_code(0x049630, 2); gsu_code(0x049680, 1);
    } else {
        for (i = 0; i < 3; i++)
            { gsu_codes(FxRecordMove_fetch[0] + 6 + 2 * i, 2); move_word(&g, 3, GSU_COPY(move_word_in_FxRecordMove_3[0][i])); }
        g.r0 = gsu_sub(kind, 6, 0);
        if (gsu.z)
            { for (i = 0; i < 3; i++)
                { gsu_codes(FxRecordMove_fetch[0] + 12 + i, 1); move_word(&g, 0, GSU_COPY(move_word_in_FxRecordMove_4[0][i])); } gsu_code(0x04967f, 2); } else { gsu_code(0x04965e, 7); gsu_code(0x049680, 1); }
    }
    gsu_code(0x049681, 2);
    return g;
}

void abi_FxRecordMove(void)
{
    uint16_t ret = gsu.r[11];
    FxMoveRegs g = FxRecordMove(gsu.r[8], gsu.r[1], GSU_COPY(0));

    gsu.r[0] = g.r0;
    gsu.r[2] = g.r2;
    gsu.r[3] = g.r3;
    gsu.r[5] = g.r5;
    gsu_code(0x040000 | ret, 1);
}
