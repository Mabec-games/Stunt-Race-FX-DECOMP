/*
 * fx_course.c: the Super FX's reads of the course's segment table and the
 * lap count it keeps from them.
 *
 * The table is in ROM: its bank is the byte at $70:1094 and its address
 * the word at $70:1092. Its first word is the number of segments; 8-byte
 * entries follow (see render/table.c). A car's Super FX record (R8) holds
 * its current segment at +$58, the last segment it was counted through at
 * +$86, and its laps at +$7A.
 */
#include <string.h>
#include "gsu_ops.h"
#include "fx_calls.h"

#define FX_TABLE_ADDR 0x1092u
#define FX_TABLE_BANK 0x1094u

#define REC_SEGMENT 0x58u
#define REC_LAPS 0x7Au
#define REC_COUNTED 0x86u
#define REC_X 0x1Eu
#define REC_DIR 0x70u        /* bit 1: going the wrong way round */

typedef struct {
    uint16_t w[4];
    uint16_t r14;
} FxEntryWords;

/*
 * @routine 04:CD56 FxCourseEntryRead
 * WHAT IT DOES: Reads the four words of the segment table entry at R5 into
 * R1-R4, keeping R0.
 * HOW: R0 is pushed on the R10 stack, ROMB from the table bank byte, then
 * GETB and GETBH through R14 = R5 and pulled back.
 * Entry: R0 = any; R5 = entry address; R10 = stack; R11 = return address.
 * Exit: R1-R4 = the words; R14 = R5 + 8; ROMBR = the table bank; R0 as on
 * entry; the last RAM address R10; S, Z from R10.
 */
/* gsuwalk: the code of each copy of FxCourseEntryRead, read from the ROM */
static const GsuCode FxCourseEntryRead_fetch[1][16] = {
    {{0x04cd62, 2}, {0x04cd69, 2}, {0x04cd70, 2}, {0x04cd77, 2}, {0x04cd64, 1}, {0x04cd6b, 1}, {0x04cd72, 1}, {0x04cd79, 1}, {0x04cd65, 3}, {0x04cd6c, 3}, {0x04cd73, 3}, {0x04cd7a, 3}, {0x04cd68, 1}, {0x04cd6f, 1}, {0x04cd76, 1}, {0x04cd7d, 1}},
};

FxEntryWords FxCourseEntryRead(uint16_t r0, uint16_t entry, uint16_t stack, int copy)
{
    FxEntryWords o;
    uint16_t r14, w;
    int i;

    gsu_code(0x04cd57, 1);
    gsu_stw(stack, r0);
    stack = gsu_inc(gsu_inc(stack));
    gsu_code(0x04cd58, 6);
    (gsu_code(0x04cd5e, 2), gsu_romb(gsu_ldw(FX_TABLE_BANK)));
    gsu_code(0x04cd60, 2);
    gsu_r14(entry);
    r14 = gsu.r[14];
    for (i = 0; i < 4; i++) {
        gsu_codes(FxCourseEntryRead_fetch[0] + i, 1);
        w = gsu_getb();
        gsu_codes(FxCourseEntryRead_fetch[0] + 4 + i, 1);
        gsu_r14(gsu_inc(r14));
        r14 = gsu.r[14];
        gsu_codes(FxCourseEntryRead_fetch[0] + 8 + i, 1);
        o.w[i] = gsu_getbh(w);
        gsu_codes(FxCourseEntryRead_fetch[0] + 12 + i, 1);
        gsu_r14(gsu_inc(r14));
        r14 = gsu.r[14];
    }
    o.r14 = r14;
    stack = gsu_dec(gsu_dec(stack));
    gsu_code(0x04cd7e, 3);
    gsu_ldw(stack);
    gsu_code(0x04cd81, 2);
    return o;
}

void abi_FxCourseEntryRead(void)
{
    FxEntryWords o = FxCourseEntryRead(gsu.r[0], gsu.r[5], gsu.r[10], GSU_COPY(0));
    int i;

    for (i = 0; i < 4; i++)
        gsu.r[1 + i] = o.w[i];
    gsu.r[14] = o.r14;
}

typedef struct {
    uint16_t r0, r1, r2, r3, r4, r14;
} FxLapLeft;

/*
 * @routine 04:CDCA FxLapSegment
 * WHAT IT DOES: Counts car R8 through its current segment: when +$58 has
 * moved on from +$86 by 1 to 4 segments, or has wrapped from the last
 * segment (+$86 = the table's count) to a lower one, +$86 takes it; when
 * the segment counted is 1, the lap count +$7A goes up by one.
 * HOW: Returns early when the two are equal or the table address is 0. The
 * count is the table's first word (GETB, GETBH with ROMBR = the table
 * bank); the step forward is +$58 - +$86 compared with 5 by sign.
 * Entry: R8 = the car's record; R11 = return address. Exit: R3 = +$58; R1
 * = the old +$86; when they differed and the table is there: R14 = table +
 * 1, R4 = the count, R2 = 5 or 1; R0 and the flags from the last compare,
 * or from the INC of the laps. The last RAM address is the last field or
 * table word read or written.
 */
FxLapLeft FxLapSegment(uint16_t rec, FxLapLeft o, int copy)
{
    uint16_t count;

    gsu_code(0x04cdcb, 4);
    o.r0 = o.r3 = gsu_ldw(gsu_add(REC_SEGMENT, rec, 0));
    gsu_code(0x04cdcf, 7);
    o.r1 = gsu_ldw(gsu_add(REC_COUNTED, rec, 0));
    gsu_sub(o.r3, o.r1, 0);
    if (gsu.z)
        { gsu_code(0x04cdd6, 6); gsu_code(0x04ce1a, 3);
        return o; } else { gsu_code(0x04cdd6, 6); }
    gsu_code(0x04cddc, 5);
    o.r0 = gsu_ldw(FX_TABLE_BANK);
    gsu_code(0x04cde1, 2);
    gsu_romb(o.r0);
    gsu_code(0x04cde3, 4);
    o.r0 = gsu_ldw(FX_TABLE_ADDR);
    gsu_code(0x04cde7, 2);
    gsu_r14(o.r0);
    o.r14 = gsu.r[14];
    gsu_moves(o.r14);
    if (gsu.z)
        { gsu_code(0x04cde9, 2); gsu_code(0x04ce1a, 3);
        return o; } else { gsu_code(0x04cde9, 2); }
    gsu_code(0x04cdeb, 3);
    count = gsu_getb();
    gsu_code(0x04cdee, 1);
    gsu_r14(gsu_inc(o.r14));
    o.r14 = gsu.r[14];
    gsu_code(0x04cdef, 3);
    o.r4 = count = gsu_getbh(count);
    o.r0 = gsu_sub(o.r3, o.r1, 0);
    if (gsu.s) {
        gsu_sub(o.r1, count, 0);           /* wrapped: from the last segment */
        if (!gsu.z)
            { gsu_code(0x04cdf2, 13); gsu_code(0x04ce1a, 3);
            return o; } else { gsu_code(0x04cdf2, 10); gsu_code(0x04ce06, 1); }
    } else {
        o.r2 = 5;
        gsu_sub(o.r0, o.r2, 0);
        if (!gsu.s)
            { gsu_code(0x04cdf2, 4); gsu_code(0x04cdff, 7); gsu_code(0x04ce1a, 3);
            return o; } else { gsu_code(0x04cdf2, 4); gsu_code(0x04cdff, 7); }
        gsu_code(0x04ce06, 1);
    }
    o.r0 = gsu_add(REC_COUNTED, rec, 0);
    gsu_code(0x04ce07, 6);
    gsu_stw(o.r0, o.r3);
    o.r2 = 1;
    gsu_sub(o.r3, o.r2, 0);
    if (!gsu.z)
        { gsu_code(0x04ce0d, 7); gsu_code(0x04ce1a, 3);
        return o; } else { gsu_code(0x04ce0d, 7); }
    gsu_code(0x04ce14, 5);
    o.r0 = gsu_ldw(gsu_add(REC_LAPS, rec, 0));
    o.r0 = gsu_inc(o.r0);
    gsu_code(0x04ce19, 2);
    gsu_sbk(o.r0);
    gsu_code(0x04ce1b, 2);
    return o;
}

void abi_FxLapSegment(void)
{
    FxLapLeft o = {gsu.r[0], gsu.r[1], gsu.r[2], gsu.r[3], gsu.r[4], gsu.r[14]};

    o = FxLapSegment(gsu.r[8], o, GSU_COPY(0));
    gsu.r[0] = o.r0;
    gsu.r[1] = o.r1;
    gsu.r[2] = o.r2;
    gsu.r[3] = o.r3;
    gsu.r[4] = o.r4;
    gsu.r[14] = o.r14;
    
}

/* A word of the table through the ROM buffer (GETB, INC, GETBH, INC). */
/* gsuwalk: the code of each copy of table_word, read from the ROM */
static const GsuCode table_word_fetch[15][4] = {
    {{0x04cc1a, 1}, {0x04cc1b, 1}, {0x04cc1c, 2}, {0x04cc1e, 1}},
    {{0x04cc34, 2}, {0x04cc36, 1}, {0x04cc37, 2}, {0x04cc39, 1}},
    {{0x04cc43, 3}, {0x04cc46, 1}, {0x04cc47, 2}, {0x04cc49, 1}},
    {{0x04cc57, 1}, {0x04cc58, 1}, {0x04cc59, 2}, {0x04cc5b, 1}},
    {{0x04cc89, 2}, {0x04cc8b, 1}, {0x04cc8c, 3}, {0x04cc8f, 1}},
    {{0x04cc90, 2}, {0x04cc92, 1}, {0x04cc93, 3}, {0x04cc96, 1}},
    {{0x04cc97, 2}, {0x04cc99, 1}, {0x04cc9a, 3}, {0x04cc9d, 1}},
    {{0x04cd34, 4}, {0x04cd38, 1}, {0x04cd39, 3}, {0x04cd3c, 1}},
    {{0x04cd3d, 2}, {0x04cd3f, 1}, {0x04cd40, 3}, {0x04cd43, 1}},
    {{0x04cd44, 2}, {0x04cd46, 1}, {0x04cd47, 3}, {0x04cd4a, 1}},
    {{0x04cd4b, 2}, {0x04cd4d, 1}, {0x04cd4e, 3}, {0x04cd51, 1}},
    {{0x04cce3, 4}, {0x04cce7, 1}, {0x04cce8, 3}, {0x04cceb, 1}},
    {{0x04ccec, 2}, {0x04ccee, 1}, {0x04ccef, 3}, {0x04ccf2, 1}},
    {{0x04ccf3, 2}, {0x04ccf5, 1}, {0x04ccf6, 3}, {0x04ccf9, 1}},
    {{0x04ccfa, 2}, {0x04ccfc, 1}, {0x04ccfd, 3}, {0x04cd00, 1}},
};

static uint16_t table_word(uint16_t *r14, int copy)
{
    gsu_codes(table_word_fetch[copy], 1);
    uint16_t w = gsu_getb();

    gsu_codes(table_word_fetch[copy] + 1, 1);
    gsu_r14(gsu_inc(*r14));
    *r14 = gsu.r[14];
    gsu_codes(table_word_fetch[copy] + 2, 1);
    w = gsu_getbh(w);
    gsu_codes(table_word_fetch[copy] + 3, 1);
    gsu_r14(gsu_inc(*r14));
    *r14 = gsu.r[14];
    return w;
}

/* |v| by NOT and INC when the flags say negative. */
/* gsuwalk: the code of each copy of magnitude, read from the ROM */
static const GsuCode magnitude_fetch[3][3] = {
    {{0x04cc3e, 5}, {0x04cc3e, 2}, {0x04cc42, 1}},
    {{0x04cc4a, 6}, {0x04cc4a, 3}, {0x04cc4f, 1}},
    {{0x04cc60, 5}, {0x04cc60, 2}, {0x04cc64, 1}},
};

static uint16_t magnitude(uint16_t v, int copy)
{
    return gsu.s ? (gsu_codes(magnitude_fetch[copy], 1), (gsu_inc(gsu_not(v)))) : (gsu_codes(magnitude_fetch[copy] + 1, 2), v);
}

/*
 * @routine 04:CC06 FxCourseNearest
 * WHAT IT DOES: Finds the segment table entry nearest the point (R1, R2,
 * R3) by |dx| + 4|dy| + |dz|: R0 = its number (from 1), R1-R3 = its first
 * three words, R5 its address. An entry whose sums overflow is passed
 * over. With no table (address 0) or no entries, R0 = 0.
 * HOW: A LOOP over the count with the ROM buffer (ROMBR = the table bank)
 * reading each entry's x, y, z; SUB with BVS for the overflow, NOT and INC
 * for the magnitudes; a sum below the best so far (CMP, BCS) takes its
 * place, R6 holding the best sum, R7 its number and R5 the address just
 * past its z. The routine runs under CACHE (the code cache only; nothing
 * the C has to do). The not-reached-yet path for a zero table address is
 * decoded from the ROM bytes and not reached in the traces so far.
 * Entry: R1, R2, R3 = the point; R11 = return address. Exit: R0 = R7 = the
 * number; R1-R3 = the entry's words; R4 = the count; R5 = the entry; R6 =
 * its sum ($7FFF when none was nearer); R9 from the last entry; R12 = 0;
 * R13 = $CC33; R14 = R5 + 6; ROMBR = the table bank; S, Z from R14, OV, CY
 * from R5's SUB; the last RAM address $1092. No entries: R0 = 0; R12 = 0;
 * R14 = table + 2; the flags from SUB R0.
 */
FxRegs FxCourseNearest(FxRegs g, int copy)
{
    uint16_t *r14 = &g.r[14];

    gsu_code(0x04cc07, 4);
    g.r[0] = gsu_ldw(FX_TABLE_BANK);
    gsu_code(0x04cc0b, 2);
    gsu_romb(g.r[0]);
    gsu_code(0x04cc0d, 4);
    g.r[0] = gsu_ldw(FX_TABLE_ADDR);
    gsu_code(0x04cc11, 2);
    gsu_r14(g.r[0]);
    *r14 = gsu.r[14];
    gsu_moves(*r14);
    if (gsu.z) {
        g.r[0] = gsu_sub(g.r[0], g.r[0], 0);
        gsu_code(0x04cc13, 6); gsu_code(0x04cca0, 4);
        return g;
    } else { gsu_code(0x04cc13, 2); gsu_code(0x04cc19, 1); }
    g.r[0] = table_word(r14, GSU_COPY(0));
    g.r[12] = g.r[0];
    gsu_moves(g.r[12]);
    if (gsu.z) {
        g.r[0] = gsu_sub(g.r[0], g.r[0], 0);
        gsu_code(0x04cc1f, 8); gsu_code(0x04cca0, 4);
        return g;
    } else { gsu_code(0x04cc1f, 4); gsu_code(0x04cc27, 1); }
    g.r[5] = 0;
    g.r[6] = 0x7FFF;
    g.r[4] = 0;
    g.r[7] = 0;
    g.r[13] = 0xCC33;
    gsu_code(0x04cc28, 9); gsu_cache(0xcc31); gsu_code(0x04cc31, 3);
    do {
        g.r[4] = gsu_inc(g.r[4]);
        g.r[0] = gsu_sub(table_word(r14, GSU_COPY(1)), g.r[1], 0);
        if (gsu.ov) {
            gsu_code(0x04cc3a, 3); gsu_code(0x04cc7a, 4);
            gsu_r14(gsu_add(*r14, 4, 0)); /* past y and z */
            *r14 = gsu.r[14];
        } else {
            gsu_code(0x04cc3a, 4);
            g.r[9] = magnitude(g.r[0], GSU_COPY(0));
            g.r[0] = gsu_sub(table_word(r14, GSU_COPY(2)), g.r[2], 0);
            g.r[0] = magnitude(g.r[0], GSU_COPY(1));
            g.r[0] = gsu_add(g.r[0], g.r[0], 0);
            g.r[0] = gsu_add(g.r[0], g.r[0], 0);
            g.r[9] = gsu_add(g.r[9], g.r[0], 0);
            if (gsu.ov) {
                gsu_code(0x04cc50, 6); gsu_code(0x04cc76, 2);
                gsu_r14(gsu_inc(*r14));  /* past z */
                *r14 = gsu.r[14];
                gsu_code(0x04cc78, 2); gsu_code(0x04cc7d, 1);
                gsu_r14(gsu_inc(*r14));
                *r14 = gsu.r[14];
            } else {
                gsu_code(0x04cc50, 7);
                g.r[0] = gsu_sub(table_word(r14, GSU_COPY(3)), g.r[3], 0);
                if (!gsu.ov) {
                    gsu_code(0x04cc5c, 4);
                    g.r[0] = gsu_add(magnitude(g.r[0], GSU_COPY(2)), g.r[9], 0);
                    if (!gsu.ov) {
                        gsu_sub(g.r[0], g.r[6], 0);
                        if (!gsu.cy) {
                            g.r[6] = g.r[0];
                            g.r[5] = *r14;
                            g.r[7] = g.r[4];
                            gsu_code(0x04cc65, 17); gsu_code(0x04cc7d, 1);
                        } else { gsu_code(0x04cc65, 8); gsu_code(0x04cc7d, 1); }
                    } else { gsu_code(0x04cc65, 3); gsu_code(0x04cc7d, 1); }
                } else { gsu_code(0x04cc5c, 3); gsu_code(0x04cc7d, 1); }
            }
        }
        gsu_code(0x04cc7e, 1);
        gsu_r14(gsu_inc(*r14));
        *r14 = gsu.r[14];
        {
            int again = gsu_count_down(&g.r[12]);

            gsu_code(0x04cc7f, 1); gsu_code((again) ? 0x04cc33 : 0x04cc80, 1);   /* the branch's delay slot */
            gsu_r14(gsu_inc(*r14)); /* the fourth word */
            *r14 = gsu.r[14];
            if (!again)
                { break; }
        }
    } while (1);
    g.r[0] = g.r[7];
    g.r[5] = gsu_sub(g.r[5], 6, 0);
    gsu_code(0x04cc81, 8);
    gsu_r14(g.r[5]);
    *r14 = gsu.r[14];
    g.r[1] = table_word(r14, GSU_COPY(4));
    g.r[2] = table_word(r14, GSU_COPY(5));
    g.r[3] = table_word(r14, GSU_COPY(6));
    gsu_code(0x04cc9e, 2);
    return g;
}

void abi_FxCourseNearest(void)
{
    FxRegs g;

    memcpy(g.r, gsu.r, sizeof g.r);
    g = FxCourseNearest(g, GSU_COPY(0));
    memcpy(gsu.r, g.r, 15 * sizeof g.r[0]);
}

/* The four words of the entry at R14 into R1-R4, R5 = the entry; R0 = R6. */
/* gsuwalk: the copy of table_word each copy of entry_words calls, by pass */
static const uint8_t table_word_in_entry_words[2][4] = {{7, 8, 9, 10}, {11, 12, 13, 14}};

static void entry_words(FxRegs *g, int copy)
{
    int i;

    g->r[5] = g->r[14];
    for (i = 0; i < 4; i++)
        { g->r[1 + i] = table_word(&g->r[14], GSU_COPY(table_word_in_entry_words[copy][i])); }
    g->r[0] = g->r[6];
}

/*
 * @routine 04:CD05 FxCoursePrevEntry
 * WHAT IT DOES: Steps back one segment: from number R0 at entry R5 to the
 * one before (from the first to the last); R0 = R6 = the new number, R5 its
 * entry, R1-R4 its words.
 * HOW: The last entry is table + 2 + (count - 1) * 8 (LMULT by R6 = 8),
 * the count read through the ROM buffer; otherwise R5 - 8. It is reached
 * only by FxCourseNextEntry's jump for a car going the wrong way; decoded
 * from the ROM bytes and not reached in the traces so far.
 * Entry: R0 = number; R5 = its entry; R11 = return address. Exit: R0 = R6 =
 * the number; R1-R4 = the words; R5 = the entry; R14 = R5 + 8; ROMBR = the
 * table bank; S, Z from R14; OV, CY from the address SUB or ADD; the last
 * RAM address $1094 or $1092.
 */
FxRegs FxCoursePrevEntry(FxRegs g, int copy)
{
    uint32_t p;

    g.r[6] = g.r[0];
    gsu_code(0x04cd06, 6);
    g.r[0] = gsu_ldw(FX_TABLE_BANK);
    gsu_code(0x04cd0c, 2);
    gsu_romb(g.r[0]);
    g.r[6] = gsu_dec(g.r[6]);
    if (!gsu.z) {
        gsu_code(0x04cd0e, 3); gsu_code(0x04cd2f, 5);
        gsu_r14(gsu_sub(g.r[5], 8, 0));
        g.r[14] = gsu.r[14];
    } else {
        gsu_code(0x04cd0e, 8);
        gsu_r14(gsu_ldw(FX_TABLE_ADDR));
        g.r[14] = gsu.r[14];
        gsu_code(0x04cd16, 1);
        g.r[0] = (gsu_getb());
        gsu_code(0x04cd17, 1);
        gsu_r14(gsu_inc(g.r[14]));
        g.r[14] = gsu.r[14];
        gsu_code(0x04cd18, 2);
        g.r[0] = gsu_getbh(g.r[0]);
        g.r[1] = g.r[0];                       /* the count */
        g.r[0] = gsu_dec(g.r[0]);
        g.r[6] = 8;
        p = gsu_lmult(g.r[0], g.r[6]);
        g.r[4] = (uint16_t)p;
        gsu_code(0x04cd1a, 12);
        g.r[0] = gsu_ldw(FX_TABLE_ADDR);
        g.r[0] = gsu_add(g.r[0], 2, 0);
        gsu_code(0x04cd26, 5);
        gsu_r14(gsu_add(g.r[4], g.r[0], 0));
        g.r[14] = gsu.r[14];
        g.r[6] = g.r[1];
        gsu_code(0x04cd2b, 4); gsu_code(0x04cd33, 1);
    }
    entry_words(&g, GSU_COPY(0));
    gsu_code(0x04cd52, 4);
    return g;
}

void abi_FxCoursePrevEntry(void)
{
    FxRegs g;

    memcpy(g.r, gsu.r, sizeof g.r);
    g = FxCoursePrevEntry(g, GSU_COPY(0));
    memcpy(gsu.r, g.r, 15 * sizeof g.r[0]);
}

/*
 * @routine 04:CCA4 FxCourseNextEntry
 * WHAT IT DOES: Steps car R8 on one segment: from number R0 at entry R5 to
 * the next (from the last to the first); R0 = R6 = the new number, R5 its
 * entry, R1-R4 its words. A car going the wrong way (bit 1 of +$70) steps
 * back instead (FxCoursePrevEntry).
 * HOW: The count is read through the ROM buffer (ROMBR = the table bank)
 * and compared with the number; the first entry is table + 2, otherwise R5
 * + 8. The wrong-way jump is decoded from the ROM bytes and not reached in
 * the traces so far.
 * Entry: R0 = number; R5 = its entry; R8 = the car's record; R11 = return
 * address. Exit: R0 = R6 = the number; R1-R4 = the words; R5 = the entry;
 * R14 = R5 + 8; ROMBR = the table bank; S, Z from R14; OV, CY from the
 * address ADD (or the CMP with the count); the last RAM address $1092.
 */
FxRegs FxCourseNextEntry(FxRegs g, int copy)
{
    g.r[6] = g.r[0];
    gsu_code(0x04cca5, 6);
    g.r[0] = gsu_ldw(FX_TABLE_BANK);
    gsu_code(0x04ccab, 2);
    gsu_romb(g.r[0]);
    gsu_code(0x04ccad, 6);
    g.r[1] = gsu_ldw(gsu_add(REC_DIR, g.r[8], 0));
    g.r[0] = gsu_and(0x0002, g.r[1]);
    g.r[0] = g.r[6];
    if (!gsu.z)
        { gsu_code(0x04ccb3, 12); gsu_code(0x04cd05, 1);
        return FxCoursePrevEntry(g, GSU_COPY(0)); } else { gsu_code(0x04ccb3, 8); gsu_code(0x04ccbf, 1); }
    g.r[6] = gsu_inc(g.r[0]);
    gsu_code(0x04ccc0, 7);
    gsu_r14(gsu_ldw(FX_TABLE_ADDR));
    g.r[14] = gsu.r[14];
    gsu_code(0x04ccc7, 2);
    g.r[1] = (gsu_getb());
    gsu_code(0x04ccc9, 1);
    gsu_r14(gsu_inc(g.r[14]));
    g.r[14] = gsu.r[14];
    gsu_code(0x04ccca, 3);
    g.r[1] = gsu_getbh(g.r[1]);
    gsu_sub(g.r[0], g.r[1], 0);
    if (gsu.z) {
        gsu_code(0x04cccd, 9);
        g.r[0] = gsu_ldw(FX_TABLE_ADDR);
        g.r[0] = gsu_add(g.r[0], 2, 0);
        gsu_code(0x04ccd6, 4);
        gsu_r14(g.r[0]);
        g.r[14] = gsu.r[14];
        g.r[6] = 1;
        gsu_code(0x04ccda, 4); gsu_code(0x04cce2, 1);
    } else {
        gsu_code(0x04cccd, 4); gsu_code(0x04ccde, 5);
        gsu_r14(gsu_add(g.r[5], 8, 0));
        g.r[14] = gsu.r[14];
    }
    entry_words(&g, GSU_COPY(1));
    gsu_code(0x04cd01, 4);
    return g;
}

void abi_FxCourseNextEntry(void)
{
    FxRegs g;

    memcpy(g.r, gsu.r, sizeof g.r);
    g = FxCourseNextEntry(g, GSU_COPY(0));
    memcpy(gsu.r, g.r, 15 * sizeof g.r[0]);
}

/* R1-R3 = record R8's position (+$1E, +$20, +$22). */
/* gsuwalk: the code of each copy of position, read from the ROM */
static const GsuCode position_fetch[2][3] = {
    {{0x04c37b, 8}, {0x04c383, 6}, {0x04c389, 6}},
    {{0x04c3b8, 8}, {0x04c3c0, 6}, {0x04c3c6, 6}},
};

static void position(FxRegs *g, int copy)
{
    int i;

    for (i = 0; i < 3; i++)
        { gsu_codes(position_fetch[copy] + i, 1);
        g->r[1 + i] = gsu_ldw(gsu_add((uint16_t)(REC_X + 2 * i), g->r[8], 0)); }
}

/*
 * @routine 04:C3B5 FxRecordSegment
 * WHAT IT DOES: FxCourseNearest for record R8's position.
 * HOW: The position (+$1E, +$20, +$22) into R1-R3, then the call; the
 * return address is kept on the R10 stack.
 * Entry: R8 = the record; R10 = stack; R11 = return address. Exit: as
 * FxCourseNearest; R11 = $C3D0; S, Z from R10; the last RAM address R10.
 */
FxRegs FxRecordSegment(FxRegs g, int copy)
{
    gsu_code(0x04c3b6, 2);
    gsu_push(&g.r[10], g.r[11]);
    position(&g, GSU_COPY(1));
    g.r[11] = 0xC3D0;
    gsu_code(0x04c3cc, 4); gsu_code(0x04cc06, 1);
    g = FxCourseNearest(g, GSU_COPY(0));
    gsu_code(0x04c3d0, 5);
    gsu_pull(&g.r[10]);
    return g;
}

void abi_FxRecordSegment(void)
{
    FxRegs g;

    memcpy(g.r, gsu.r, sizeof g.r);
    g = FxRecordSegment(g, GSU_COPY(0));
    memcpy(gsu.r, g.r, 15 * sizeof g.r[0]);
}

/*
 * @routine 04:C378 FxRecordHeadingAhead
 * WHAT IT DOES: The bearing (FxBearingTo) from record R8 to the segment
 * after the one nearest it (FxCourseNearest, FxCourseNextEntry): R0 = the
 * negated angle.
 * HOW: The position into R1-R3, then the three calls in a row; the return
 * address is kept on the R10 stack.
 * Entry: R8 = the record; R10 = stack; R11 = return address. Exit: as
 * FxBearingTo leaves them (R11 = $C39D when it did not divide); S, Z from
 * R10; the last RAM address R10.
 */
FxRegs FxRecordHeadingAhead(FxRegs g)
{
    gsu_code(0x04c379, 2);
    gsu_push(&g.r[10], g.r[11]);
    position(&g, GSU_COPY(0));
    g.r[11] = 0xC393;
    
    gsu_code(0x04c38f, 4); gsu_code(0x04cc06, 1);
    g = FxCourseNearest(g, GSU_COPY(0));
    g.r[11] = 0xC398;
    
    gsu_code(0x04c393, 5); gsu_code(0x04cca4, 1);
    g = FxCourseNextEntry(g, GSU_COPY(0));
    g.r[11] = 0xC39D;
    
    gsu_code(0x04c398, 5); gsu_code(0x04cbe8, 1);
    g = FxBearingTo(g, GSU_COPY(0));
    gsu_code(0x04c39d, 5);
    gsu_pull(&g.r[10]);
    return g;
}

void abi_FxRecordHeadingAhead(void)
{
    FxRegs g;

    memcpy(g.r, gsu.r, sizeof g.r);
    g = FxRecordHeadingAhead(g);
    memcpy(gsu.r, g.r, 15 * sizeof g.r[0]);
}

void abi_FxTableEntryWords(void);

/* |R0 - b| / 4 with the Super FX's flags: SUB, then NOT and INC when
   negative, then two DIV2s. */
/* gsuwalk: the code of each copy of quarter_apart, read from the ROM */
static const GsuCode quarter_apart_fetch[2][3] = {
    {{0x04cee9, 3}, {0x04ceee, 1}, {0x04cee9, 6}},
    {{0x04cef9, 3}, {0x04cefe, 1}, {0x04cef9, 6}},
};

static uint16_t quarter_apart(uint16_t a, uint16_t b, int copy)
{
    uint16_t d = gsu_sub(a, b, 0);

    if (gsu.s)
        { d = gsu_inc(gsu_not(d)); gsu_codes(quarter_apart_fetch[copy] + 2, 1); } else { gsu_codes(quarter_apart_fetch[copy], 2); }
    return gsu_div2(gsu_div2(d));
}

/*
 * @routine 04:CED6 FxCourseNextDistance
 * WHAT IT DOES: Finds the segment after entry R0 for car R8
 * (FxTableEntryWords, FxCourseNextEntry) and returns how far the car is
 * from it: R0 = |car x (+$1E) - R1| / 4 + |car z (+$22) - R3| / 4.
 * HOW: The return address is kept on the R10 stack; each difference is
 * made positive with NOT and INC and halved twice with DIV2.
 * Entry: R0 = entry; R8 = the car's record; R10 = stack; R11 = return
 * address. Exit: R0 = the distance; R4 = the x part; R1-R3, R5, R6 as
 * FxCourseNextEntry leaves them; R11 = $CEE4; OV, CY from the final add;
 * S, Z from R10; the last RAM address R10.
 */
void abi_FxCourseNextDistance(void)
{
    gsu_code(0x04ced7, 2);
    gsu_push(&gsu.r[10], gsu.r[11]);
    gsu.r[11] = 0xCEDF;
    gsu_code(0x04ced9, 6); gsu_code(0x04cd83, 1);
    abi_FxTableEntryWords();
    gsu.r[11] = 0xCEE4;
    gsu_code(0x04cedf, 5); gsu_code(0x04cca4, 1);
    abi_FxCourseNextEntry();
    gsu.r[0] = gsu_add(0x001E, gsu.r[8], 0);
    gsu_code(0x04cee4, 5);
    gsu.r[0] = quarter_apart(gsu_ldw(gsu.r[0]), gsu.r[1], GSU_COPY(0));
    gsu.r[4] = gsu.r[0];
    gsu.r[0] = gsu_add(0x0022, gsu.r[8], 0);
    gsu_code(0x04ceef, 10);
    gsu.r[0] = quarter_apart(gsu_ldw(gsu.r[0]), gsu.r[3], GSU_COPY(1));
    gsu.r[0] = gsu_add(gsu.r[0], gsu.r[4], 0);
    gsu_code(0x04ceff, 9);
    gsu_pull(&gsu.r[10]);
}

void abi_FxBearingTo(void);
void abi_FxListClearBit0(void);
void abi_FxListSweep(void);

/* R0 = the car's word +off - from, negated (SUB, NOT, INC). */
/* gsuwalk: the code of each copy of back_from, read from the ROM */
static const GsuCode back_from_fetch[3][1] = {
    {{0x04c68a, 4}},
    {{0x04c698, 4}},
    {{0x04c6a6, 4}},
};

static uint16_t back_from(uint16_t off, uint16_t from, int copy)
{
    gsu.r[0] = gsu_add(off, gsu.r[8], 0);
    gsu_codes(back_from_fetch[copy], 1);
    gsu.r[0] = gsu_sub(gsu_ldw(gsu.r[0]), from, 0);
    return gsu_inc(gsu_not(gsu.r[0]));
}

/* For list member R6 with a word +$18, FxListClearBit0 and FxListSweep. */
/* gsuwalk: the code of each copy of sweep_if_listed, read from the ROM */
static const GsuCode sweep_if_listed_fetch[2][9] = {
    {{0x04c6f2, 4}, {0x04c6f6, 4}, {0x04c6f6, 4}, {0x04c704, 1}, {0x04c6fa, 5}, {0x04a441, 1}, {0x04c6ff, 5}, {0x04a462, 1}, {0x04c704, 1}},
    {{0x04c712, 6}, {0x04c718, 4}, {0x04c718, 4}, {0x04c726, 1}, {0x04c71c, 5}, {0x04a441, 1}, {0x04c721, 5}, {0x04a462, 1}, {0x04c726, 1}},
};

static void sweep_if_listed(uint16_t r11a, uint16_t r11b, int copy)
{
    gsu.r[0] = gsu_add(0x0018, gsu.r[6], 0);
    gsu_codes(sweep_if_listed_fetch[copy], 1);
    gsu.r[0] = gsu_ldw(gsu.r[0]);
    gsu_moves(gsu.r[0]);
    if (gsu.z)
        { gsu_codes(sweep_if_listed_fetch[copy] + 2, 2);
        return; } else { gsu_codes(sweep_if_listed_fetch[copy] + 1, 1); }
    gsu.r[11] = r11a;
    gsu_codes(sweep_if_listed_fetch[copy] + 4, 2);
    abi_FxListClearBit0();
    gsu.r[11] = r11b;
    gsu_codes(sweep_if_listed_fetch[copy] + 6, 2);
    (abi_FxListSweep());
    gsu_codes(sweep_if_listed_fetch[copy] + 8, 1);
}

/*
 * @routine 04:C65A FxCarPlaceOnSegment
 * WHAT IT DOES: Puts car R8 back on its segment entry +$58 (unless that is
 * 0): reads the entry (FxTableEntryWords, FxCourseEntryRead), keeps its
 * point at $26F6/$26F8/$26FA with y lowered by $A0, sets the car's
 * velocity +$2E/+$30/+$32 to point minus position (+$1E/+$20/+$22), clears
 * +$26, +$28, +$36, +$34 and +$38, steps to the next entry
 * (FxCourseNextEntry) and turns the car to face it (FxBearingTo into
 * +$24); then, for each object linked from +$3E (through +$34) and for
 * the car itself, when its word +$18 is set, FxListClearBit0 and
 * FxListSweep.
 * HOW: The return address, the entry number and R8 are kept on the R10
 * stack. The 0 entry exit and the sweep inside the loop are not reached in
 * the traces so far.
 * Entry: R8 = the car's record; R10 = stack; R11 = return address. Exit:
 * R8 restored; R6 = the car; R11 = $C726 or as before the last sweep; S, Z
 * from R10; the last RAM address R10.
 */
/* gsuwalk: the code of each copy of abi_FxCarPlaceOnSegment, read from the ROM */
static const GsuCode abi_FxCarPlaceOnSegment_fetch[1][8] = {
    {{0x04c68e, 10}, {0x04c69c, 10}, {0x04c6aa, 10}, {0x04c6b4, 7}, {0x04c6bb, 5}, {0x04c6c0, 5}, {0x04c6c5, 5}, {0x04c6ca, 5}},
};

/* gsuwalk: the copy of back_from each copy of abi_FxCarPlaceOnSegment calls, by pass */
static const uint8_t back_from_in_abi_FxCarPlaceOnSegment[1][3] = {{0, 1, 2}};

void abi_FxCarPlaceOnSegment(void)
{
    
    static const uint8_t at[3] = {0x1E, 0x20, 0x22}, vel[3] = {0x2E, 0x30, 0x32};
    static const uint8_t zero[5] = {0x26, 0x28, 0x36, 0x34, 0x38};
    int i;

    gsu_code(0x04c65b, 2);
    gsu_push(&gsu.r[10], gsu.r[11]);
    gsu.r[0] = gsu_add(0x0058, gsu.r[8], 0);
    gsu_code(0x04c65d, 6);
    gsu.r[0] = gsu_ldw(gsu.r[0]);
    gsu_moves(gsu.r[0]);
    if (gsu.z) {
        
        gsu_code(0x04c663, 8); gsu_code(0x04c72a, 5);
        gsu_pull(&gsu.r[10]);
        return;
    } else { gsu_code(0x04c663, 4); gsu_code(0x04c66b, 1); }
    gsu.r[11] = 0xC670;
    gsu_code(0x04c66c, 4); gsu_code(0x04cd83, 1);
    abi_FxTableEntryWords();
    gsu.r[11] = 0xC675;
    gsu_code(0x04c670, 5); gsu_code(0x04cd56, 1);
    abi_FxCourseEntryRead();
    gsu_code(0x04c675, 2);
    gsu_push(&gsu.r[10], gsu.r[0]);
    gsu.r[0] = 0xFF60;
    gsu.r[2] = gsu_add(gsu.r[2], gsu.r[0], 0);
    gsu_code(0x04c677, 11);
    gsu_stw(0x26F6, gsu.r[1]);
    gsu_code(0x04c682, 4);
    gsu_stw(0x26F8, gsu.r[2]);
    gsu_code(0x04c686, 4);
    gsu_stw(0x26FA, gsu.r[3]);
    for (i = 0; i < 3; i++) {
        gsu.r[4] = gsu.r[0] = back_from(at[i], gsu.r[1 + i], GSU_COPY(back_from_in_abi_FxCarPlaceOnSegment[0][i]));
        gsu.r[0] = gsu_add(vel[i], gsu.r[8], 0);
        gsu_codes(abi_FxCarPlaceOnSegment_fetch[0] + i, 1);
        gsu_stw(gsu.r[0], gsu.r[4]);
    }
    gsu.r[1] = 0x0000;
    for (i = 0; i < 5; i++) {
        gsu.r[0] = gsu_add(zero[i], gsu.r[8], 0);
        gsu_codes(abi_FxCarPlaceOnSegment_fetch[0] + 3 + i, 1);
        gsu_stw(gsu.r[0], gsu.r[1]);
    }
    gsu_code(0x04c6cf, 3);
    gsu.r[0] = gsu_pull(&gsu.r[10]);
    gsu.r[11] = 0xC6D6;
    gsu_code(0x04c6d2, 4); gsu_code(0x04cca4, 1);
    abi_FxCourseNextEntry();
    gsu.r[11] = 0xC6DB;
    gsu_code(0x04c6d6, 5); gsu_code(0x04cbe8, 1);
    abi_FxBearingTo();
    gsu.r[1] = gsu.r[0];
    gsu.r[0] = gsu_add(0x0024, gsu.r[8], 0);
    gsu_code(0x04c6db, 8);
    gsu_stw(gsu.r[0], gsu.r[1]);
    gsu.r[0] = gsu.r[1];
    gsu_code(0x04c6e3, 4);
    gsu_push(&gsu.r[10], gsu.r[8]);
    gsu.r[0] = gsu_add(0x003E, gsu.r[8], 0);
    gsu_code(0x04c6e7, 6);
    gsu.r[0] = gsu_ldw(gsu.r[0]);
    for (;;) {
        gsu.r[6] = gsu.r[0];
        gsu_moves(gsu.r[6]);
        if (gsu.z)
            { gsu_code(0x04c6ed, 4); gsu_code(0x04c70b, 1);
            break; } else { gsu_code(0x04c6ed, 4); }
        gsu_code(0x04c6f1, 1);
        sweep_if_listed(0xC6FF, 0xC704, GSU_COPY(0));
        gsu.r[0] = gsu_add(0x0034, gsu.r[6], 0);
        gsu_code(0x04c705, 4);
        gsu.r[0] = gsu_ldw(gsu.r[0]);
        
        gsu_code(0x04c709, 2); gsu_code(0x04c6ec, 1);
    }
    gsu_code(0x04c70c, 4);
    gsu.r[6] = gsu_pull(&gsu.r[10]);
    gsu_code(0x04c710, 2);
    gsu_push(&gsu.r[10], gsu.r[6]);
    sweep_if_listed(0xC721, 0xC726, GSU_COPY(1));
    gsu_code(0x04c727, 4);
    gsu.r[8] = gsu_pull(&gsu.r[10]);
    gsu_code(0x04c72b, 4);
    gsu_pull(&gsu.r[10]);
}

/*
 * @routine 04:C595 FxCarCatchUp
 * WHAT IT DOES: The rubber band for car R8: picks a pair of words from the
 * table for $26F4 (at $04:C4D1, pointing into bank 4) by how far the car
 * is ahead of the player (+$88 against the player's, $2476). The car's
 * count +$72 goes up by one; when it reaches the first word (or wraps
 * from $FFFF to 0) it restarts at 0 and the car is moved on to its next
 * segment entry (FxTableEntryWords, FxCourseNextEntry, into +$58) and put
 * there (FxCarPlaceOnSegment).
 * Then its position +$1E/+$20/+$22 gets its velocity +$2E/+$30/+$32
 * times the second word (FMULT) added, and every object linked from +$3E
 * (through +$34) is put at the same position.
 * HOW: ROM bank 4 for the GETBs; the second word is kept on the R10 stack
 * with the return address. The 0 entry jump is not reached in the traces
 * so far.
 * Entry: R8 = the car's record; R10 = stack; R11 = return address. Exit:
 * R1-R3 the new position; R6 = 0 (the end of the links); R14 past the
 * pair; S, Z from R10; the last RAM address R10.
 */
/* gsuwalk: the code of each copy of abi_FxCarCatchUp, read from the ROM */
static const GsuCode abi_FxCarCatchUp_fetch[1][12] = {
    {{0x04c610, 4}, {0x04c61d, 4}, {0x04c62a, 4}, {0x04c614, 6}, {0x04c621, 6}, {0x04c62e, 6}, {0x04c61a, 3}, {0x04c627, 3}, {0x04c634, 3}, {0x04c63f, 6}, {0x04c645, 5}, {0x04c64a, 5}},
};

void abi_FxCarCatchUp(void)
{
    static const uint8_t pos[3] = {0x1E, 0x20, 0x22}, vel[3] = {0x2E, 0x30, 0x32};
    int i;

    gsu_code(0x04c596, 2);
    gsu_push(&gsu.r[10], gsu.r[11]);
    gsu.r[0] = 0x0004;
    gsu_code(0x04c598, 6);
    gsu_romb(0x04);
    gsu_code(0x04c59e, 3);
    gsu_r14(0xC4D1);
    gsu_code(0x04c5a1, 4);
    gsu.r[0] = gsu_lob(gsu_ldw(0x26F4));
    gsu.r[0] = gsu_add(gsu.r[0], gsu.r[0], 0);
    gsu_code(0x04c5a5, 4);
    gsu_r14(gsu_add(gsu.r[14], gsu.r[0], 0));
    gsu_code(0x04c5a9, 1);
    gsu.r[0] = (gsu_getb());
    gsu_code(0x04c5aa, 1);
    gsu_r14(gsu_inc(gsu.r[14]));
    gsu_code(0x04c5ab, 2);
    gsu.r[0] = gsu_getbh(gsu.r[0]);
    gsu_code(0x04c5ad, 2);
    gsu_r14(gsu.r[0]);
    gsu_code(0x04c5af, 4);
    gsu.r[2] = gsu_ldw(0x2476);
    gsu.r[1] = 0x0088;
    gsu.r[1] = gsu_add(gsu.r[1], gsu.r[2], 0);
    gsu_code(0x04c5b3, 7);
    gsu.r[1] = gsu_ldw(gsu.r[1]);
    gsu.r[0] = gsu_add(0x0088, gsu.r[8], 0);
    gsu_code(0x04c5ba, 5);
    gsu.r[0] = gsu_sub(gsu_ldw(gsu.r[0]), gsu.r[1], 0);
    gsu.r[0] = gsu_add(gsu.r[0], gsu.r[0], 0);
    gsu.r[0] = gsu_add(gsu.r[0], gsu.r[0], 0);
    gsu_code(0x04c5bf, 5);
    gsu_r14(gsu_add(gsu.r[14], gsu.r[0], 0));
    gsu_code(0x04c5c4, 2);
    gsu.r[3] = (gsu_getb());
    gsu_code(0x04c5c6, 1);
    gsu_r14(gsu_inc(gsu.r[14]));
    gsu_code(0x04c5c7, 3);
    gsu.r[3] = gsu_getbh(gsu.r[3]);
    gsu_code(0x04c5ca, 1);
    gsu_r14(gsu_inc(gsu.r[14]));
    gsu_code(0x04c5cb, 2);
    gsu.r[6] = (gsu_getb());
    gsu_code(0x04c5cd, 1);
    gsu_r14(gsu_inc(gsu.r[14]));
    gsu_code(0x04c5ce, 3);
    gsu.r[6] = gsu_getbh(gsu.r[6]);
    gsu_code(0x04c5d1, 1);
    gsu_r14(gsu_inc(gsu.r[14]));
    gsu_code(0x04c5d2, 2);
    gsu_push(&gsu.r[10], gsu.r[6]);
    gsu.r[0] = gsu_add(0x0072, gsu.r[8], 0);
    gsu_code(0x04c5d4, 6);
    gsu.r[0] = gsu_inc(gsu_ldw(gsu.r[0]));
    gsu_code(0x04c5da, 3); gsu_code((gsu.z) ? 0x04c5e8 : 0x04c5dd, 1);   /* the branch's delay slot */
    gsu_sbk(gsu.r[0]);
    if (!gsu.z) {
        gsu_sub(gsu.r[0], gsu.r[3], 0);
        if (gsu.s)
            { gsu_code(0x04c5de, 8); gsu_code(0x04c60b, 1); goto placed; } else { gsu_code(0x04c5de, 4); gsu_code(0x04c5e6, 1); }                          /* IWT R15 past the placing */
        gsu.r[0] = gsu_sub(gsu.r[0], gsu.r[0], 0);
        gsu_code(0x04c5e7, 2);
        gsu_sbk(gsu.r[0]);
    }
    {
        gsu.r[0] = gsu_add(0x0058, gsu.r[8], 0);
        gsu_code(0x04c5e9, 4);
        gsu.r[0] = gsu_ldw(gsu.r[0]);
        gsu_moves(gsu.r[0]);
        if (!gsu.z) {
            gsu.r[11] = 0xC5FA;
            gsu_code(0x04c5ed, 4); gsu_code(0x04c5f5, 5); gsu_code(0x04cd83, 1);
            abi_FxTableEntryWords();
            gsu.r[11] = 0xC5FF;
            gsu_code(0x04c5fa, 5); gsu_code(0x04cca4, 1);
            abi_FxCourseNextEntry();
            gsu.r[6] = gsu.r[0];
            gsu.r[0] = gsu_add(0x0058, gsu.r[8], 0);
            gsu_code(0x04c5ff, 8);
            gsu_stw(gsu.r[0], gsu.r[6]);
            gsu.r[11] = 0xC60B;
            gsu_code(0x04c607, 4); gsu_code(0x04c65a, 1);
            abi_FxCarPlaceOnSegment();
            gsu_code(0x04c60b, 1);
        } else { gsu_code(0x04c5ed, 8); gsu_code(0x04c60b, 1); }
    }
placed:
    gsu_code(0x04c60c, 4);
    gsu.r[6] = gsu_pull(&gsu.r[10]);
    for (i = 0; i < 3; i++) {
        gsu.r[0] = gsu_add(vel[i], gsu.r[8], 0);
        gsu_codes(abi_FxCarCatchUp_fetch[0] + i, 1);
        gsu.r[0] = gsu_ldw(gsu.r[0]);
        gsu.r[1 + i] = pos[i];
        gsu.r[1 + i] = gsu_add(gsu.r[1 + i], gsu.r[8], 0);
        gsu_codes(abi_FxCarCatchUp_fetch[0] + 3 + i, 1);
        gsu.r[1 + i] = gsu_ldw(gsu.r[1 + i]);
        gsu.r[0] = gsu_fmult(gsu.r[0], gsu.r[6]);
        gsu.r[0] = gsu_add(gsu.r[0], gsu.r[1 + i], 0);
        gsu_codes(abi_FxCarCatchUp_fetch[0] + 6 + i, 1);
        gsu_sbk(gsu.r[0]);
    }
    gsu.r[0] = gsu_add(0x003E, gsu.r[8], 0);
    gsu_code(0x04c637, 4);
    gsu.r[0] = gsu_ldw(gsu.r[0]);
    for (;;) {
        gsu.r[6] = gsu.r[0];
        gsu_moves(gsu.r[6]);
        if (gsu.z)
            { gsu_code(0x04c63b, 4); gsu_code(0x04c655, 1);
            break; } else { gsu_code(0x04c63b, 4); }
        for (i = 0; i < 3; i++) {
            gsu.r[0] = gsu_add(pos[i], gsu.r[6], 0);
            gsu_codes(abi_FxCarCatchUp_fetch[0] + 9 + i, 1);
            gsu_stw(gsu.r[0], gsu.r[1 + i]);
        }
        gsu.r[0] = gsu_add(0x0034, gsu.r[6], 0);
        gsu_code(0x04c64f, 4);
        gsu.r[0] = gsu_ldw(gsu.r[0]);
        
        gsu_code(0x04c653, 2); gsu_code(0x04c63a, 1);
    }
    gsu_code(0x04c656, 4);
    gsu_pull(&gsu.r[10]);
}
