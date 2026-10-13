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
FxEntryWords FxCourseEntryRead(uint16_t r0, uint16_t entry, uint16_t stack)
{
    FxEntryWords o;
    uint16_t r14, w;
    int i;

    gsu_stw(stack, r0);
    stack = gsu_inc(gsu_inc(stack));
    gsu.rombr = (uint8_t)(gsu_ldw(FX_TABLE_BANK) & 0x7F);
    gsu_r14(entry);
    r14 = gsu.r[14];
    for (i = 0; i < 4; i++) {
        w = gsu_getb();
        gsu_r14(gsu_inc(r14));
        r14 = gsu.r[14];
        o.w[i] = gsu_getbh(w);
        gsu_r14(gsu_inc(r14));
        r14 = gsu.r[14];
    }
    o.r14 = r14;
    stack = gsu_dec(gsu_dec(stack));
    gsu_ldw(stack);
    return o;
}

void abi_FxCourseEntryRead(void)
{
    FxEntryWords o = FxCourseEntryRead(gsu.r[0], gsu.r[5], gsu.r[10]);
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
FxLapLeft FxLapSegment(uint16_t rec, FxLapLeft o)
{
    uint16_t count;

    o.r0 = o.r3 = gsu_ldw(gsu_add(REC_SEGMENT, rec, 0));
    o.r1 = gsu_ldw(gsu_add(REC_COUNTED, rec, 0));
    gsu_sub(o.r3, o.r1, 0);
    if (gsu.z)
        return o;
    o.r0 = gsu_ldw(FX_TABLE_BANK);
    gsu.rombr = (uint8_t)(o.r0 & 0x7F);
    o.r0 = gsu_ldw(FX_TABLE_ADDR);
    gsu_r14(o.r0);
    o.r14 = gsu.r[14];
    gsu_moves(o.r14);
    if (gsu.z)
        return o;
    count = gsu_getb();
    gsu_r14(gsu_inc(o.r14));
    o.r14 = gsu.r[14];
    o.r4 = count = gsu_getbh(count);
    o.r0 = gsu_sub(o.r3, o.r1, 0);
    if (gsu.s) {
        gsu_sub(o.r1, count, 0);           /* wrapped: from the last segment */
        if (!gsu.z)
            return o;
    } else {
        o.r2 = 5;
        gsu_sub(o.r0, o.r2, 0);
        if (!gsu.s)
            return o;
    }
    o.r0 = gsu_add(REC_COUNTED, rec, 0);
    gsu_stw(o.r0, o.r3);
    o.r2 = 1;
    gsu_sub(o.r3, o.r2, 0);
    if (!gsu.z)
        return o;
    o.r0 = gsu_ldw(gsu_add(REC_LAPS, rec, 0));
    o.r0 = gsu_inc(o.r0);
    gsu_sbk(o.r0);
    return o;
}

void abi_FxLapSegment(void)
{
    FxLapLeft o = {gsu.r[0], gsu.r[1], gsu.r[2], gsu.r[3], gsu.r[4], gsu.r[14]};

    o = FxLapSegment(gsu.r[8], o);
    gsu.r[0] = o.r0;
    gsu.r[1] = o.r1;
    gsu.r[2] = o.r2;
    gsu.r[3] = o.r3;
    gsu.r[4] = o.r4;
    gsu.r[14] = o.r14;
}

/* A word of the table through the ROM buffer (GETB, INC, GETBH, INC). */
static uint16_t table_word(uint16_t *r14)
{
    uint16_t w = gsu_getb();

    gsu_r14(gsu_inc(*r14));
    *r14 = gsu.r[14];
    w = gsu_getbh(w);
    gsu_r14(gsu_inc(*r14));
    *r14 = gsu.r[14];
    return w;
}

/* |v| by NOT and INC when the flags say negative. */
static uint16_t magnitude(uint16_t v)
{
    return gsu.s ? gsu_inc(gsu_not(v)) : v;
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
FxRegs FxCourseNearest(FxRegs g)
{
    uint16_t *r14 = &g.r[14];

    g.r[0] = gsu_ldw(FX_TABLE_BANK);
    gsu.rombr = (uint8_t)(g.r[0] & 0x7F);
    g.r[0] = gsu_ldw(FX_TABLE_ADDR);
    gsu_r14(g.r[0]);
    *r14 = gsu.r[14];
    gsu_moves(*r14);
    if (gsu.z) {
        g.r[0] = gsu_sub(g.r[0], g.r[0], 0);
        return g;
    }
    g.r[0] = table_word(r14);
    g.r[12] = g.r[0];
    gsu_moves(g.r[12]);
    if (gsu.z) {
        g.r[0] = gsu_sub(g.r[0], g.r[0], 0);
        return g;
    }
    g.r[5] = 0;
    g.r[6] = 0x7FFF;
    g.r[4] = 0;
    g.r[7] = 0;
    g.r[13] = 0xCC33;
    do {
        g.r[4] = gsu_inc(g.r[4]);
        g.r[0] = gsu_sub(table_word(r14), g.r[1], 0);
        if (gsu.ov) {
            gsu_r14(gsu_add(*r14, 4, 0)); /* past y and z */
            *r14 = gsu.r[14];
        } else {
            g.r[9] = magnitude(g.r[0]);
            g.r[0] = gsu_sub(table_word(r14), g.r[2], 0);
            g.r[0] = magnitude(g.r[0]);
            g.r[0] = gsu_add(g.r[0], g.r[0], 0);
            g.r[0] = gsu_add(g.r[0], g.r[0], 0);
            g.r[9] = gsu_add(g.r[9], g.r[0], 0);
            if (gsu.ov) {
                gsu_r14(gsu_inc(*r14));  /* past z */
                *r14 = gsu.r[14];
                gsu_r14(gsu_inc(*r14));
                *r14 = gsu.r[14];
            } else {
                g.r[0] = gsu_sub(table_word(r14), g.r[3], 0);
                if (!gsu.ov) {
                    g.r[0] = gsu_add(magnitude(g.r[0]), g.r[9], 0);
                    if (!gsu.ov) {
                        gsu_sub(g.r[0], g.r[6], 0);
                        if (!gsu.cy) {
                            g.r[6] = g.r[0];
                            g.r[5] = *r14;
                            g.r[7] = g.r[4];
                        }
                    }
                }
            }
        }
        gsu_r14(gsu_inc(*r14));
        *r14 = gsu.r[14];
        {
            int again = gsu_count_down(&g.r[12]);

            gsu_r14(gsu_inc(*r14)); /* the fourth word */
            *r14 = gsu.r[14];
            if (!again)
                break;
        }
    } while (1);
    g.r[0] = g.r[7];
    g.r[5] = gsu_sub(g.r[5], 6, 0);
    gsu_r14(g.r[5]);
    *r14 = gsu.r[14];
    g.r[1] = table_word(r14);
    g.r[2] = table_word(r14);
    g.r[3] = table_word(r14);
    return g;
}

void abi_FxCourseNearest(void)
{
    FxRegs g;

    memcpy(g.r, gsu.r, sizeof g.r);
    g = FxCourseNearest(g);
    memcpy(gsu.r, g.r, 15 * sizeof g.r[0]);
}

/* The four words of the entry at R14 into R1-R4, R5 = the entry; R0 = R6. */
static void entry_words(FxRegs *g)
{
    int i;

    g->r[5] = g->r[14];
    for (i = 0; i < 4; i++)
        g->r[1 + i] = table_word(&g->r[14]);
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
FxRegs FxCoursePrevEntry(FxRegs g)
{
    uint32_t p;

    g.r[6] = g.r[0];
    g.r[0] = gsu_ldw(FX_TABLE_BANK);
    gsu.rombr = (uint8_t)(g.r[0] & 0x7F);
    g.r[6] = gsu_dec(g.r[6]);
    if (!gsu.z) {
        gsu_r14(gsu_sub(g.r[5], 8, 0));
        g.r[14] = gsu.r[14];
    } else {
        gsu_r14(gsu_ldw(FX_TABLE_ADDR));
        g.r[14] = gsu.r[14];
        g.r[0] = gsu_getb();
        gsu_r14(gsu_inc(g.r[14]));
        g.r[14] = gsu.r[14];
        g.r[0] = gsu_getbh(g.r[0]);
        g.r[1] = g.r[0];                       /* the count */
        g.r[0] = gsu_dec(g.r[0]);
        g.r[6] = 8;
        p = gsu_lmult(g.r[0], g.r[6]);
        g.r[4] = (uint16_t)p;
        g.r[0] = gsu_ldw(FX_TABLE_ADDR);
        g.r[0] = gsu_add(g.r[0], 2, 0);
        gsu_r14(gsu_add(g.r[4], g.r[0], 0));
        g.r[14] = gsu.r[14];
        g.r[6] = g.r[1];
    }
    entry_words(&g);
    return g;
}

void abi_FxCoursePrevEntry(void)
{
    FxRegs g;

    memcpy(g.r, gsu.r, sizeof g.r);
    g = FxCoursePrevEntry(g);
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
FxRegs FxCourseNextEntry(FxRegs g)
{
    g.r[6] = g.r[0];
    g.r[0] = gsu_ldw(FX_TABLE_BANK);
    gsu.rombr = (uint8_t)(g.r[0] & 0x7F);
    g.r[1] = gsu_ldw(gsu_add(REC_DIR, g.r[8], 0));
    g.r[0] = gsu_and(0x0002, g.r[1]);
    g.r[0] = g.r[6];
    if (!gsu.z)
        return FxCoursePrevEntry(g);
    g.r[6] = gsu_inc(g.r[0]);
    gsu_r14(gsu_ldw(FX_TABLE_ADDR));
    g.r[14] = gsu.r[14];
    g.r[1] = gsu_getb();
    gsu_r14(gsu_inc(g.r[14]));
    g.r[14] = gsu.r[14];
    g.r[1] = gsu_getbh(g.r[1]);
    gsu_sub(g.r[0], g.r[1], 0);
    if (gsu.z) {
        g.r[0] = gsu_ldw(FX_TABLE_ADDR);
        g.r[0] = gsu_add(g.r[0], 2, 0);
        gsu_r14(g.r[0]);
        g.r[14] = gsu.r[14];
        g.r[6] = 1;
    } else {
        gsu_r14(gsu_add(g.r[5], 8, 0));
        g.r[14] = gsu.r[14];
    }
    entry_words(&g);
    return g;
}

void abi_FxCourseNextEntry(void)
{
    FxRegs g;

    memcpy(g.r, gsu.r, sizeof g.r);
    g = FxCourseNextEntry(g);
    memcpy(gsu.r, g.r, 15 * sizeof g.r[0]);
}

/* R1-R3 = record R8's position (+$1E, +$20, +$22). */
static void position(FxRegs *g)
{
    int i;

    for (i = 0; i < 3; i++)
        g->r[1 + i] = gsu_ldw(gsu_add((uint16_t)(REC_X + 2 * i), g->r[8], 0));
}

/*
 * @routine 04:C3B5 FxRecordSegment
 * WHAT IT DOES: FxCourseNearest for record R8's position.
 * HOW: The position (+$1E, +$20, +$22) into R1-R3, then the call; the
 * return address is kept on the R10 stack.
 * Entry: R8 = the record; R10 = stack; R11 = return address. Exit: as
 * FxCourseNearest; R11 = $C3D0; S, Z from R10; the last RAM address R10.
 */
FxRegs FxRecordSegment(FxRegs g)
{
    gsu_push(&g.r[10], g.r[11]);
    position(&g);
    g.r[11] = 0xC3D0;
    g = FxCourseNearest(g);
    gsu_pull(&g.r[10]);
    return g;
}

void abi_FxRecordSegment(void)
{
    FxRegs g;

    memcpy(g.r, gsu.r, sizeof g.r);
    g = FxRecordSegment(g);
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
    gsu_push(&g.r[10], g.r[11]);
    position(&g);
    g.r[11] = 0xC393;
    g = FxCourseNearest(g);
    g.r[11] = 0xC398;
    g = FxCourseNextEntry(g);
    g.r[11] = 0xC39D;
    g = FxBearingTo(g);
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
static uint16_t quarter_apart(uint16_t a, uint16_t b)
{
    uint16_t d = gsu_sub(a, b, 0);

    if (gsu.s)
        d = gsu_inc(gsu_not(d));
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
    gsu_push(&gsu.r[10], gsu.r[11]);
    gsu.r[11] = 0xCEDF;
    abi_FxTableEntryWords();
    gsu.r[11] = 0xCEE4;
    abi_FxCourseNextEntry();
    gsu.r[0] = gsu_add(0x001E, gsu.r[8], 0);
    gsu.r[0] = quarter_apart(gsu_ldw(gsu.r[0]), gsu.r[1]);
    gsu.r[4] = gsu.r[0];
    gsu.r[0] = gsu_add(0x0022, gsu.r[8], 0);
    gsu.r[0] = quarter_apart(gsu_ldw(gsu.r[0]), gsu.r[3]);
    gsu.r[0] = gsu_add(gsu.r[0], gsu.r[4], 0);
    gsu_pull(&gsu.r[10]);
}

void abi_FxAngleVector(void);

/*
 * @routine 04:C72F FxCarVelocityFromSegment
 * WHAT IT DOES: Takes the saved R11 back off the R10 stack and, unless car
 * R8's word +$58 (its entry) is 0, sets the car's velocity +$2E/+$30/+$32
 * to a vector along its heading +$24 whose length is twice word 4 of the
 * entry (FxTableEntryWords, FxCourseEntryRead) times the factor for $26F4
 * from the table at $04:C781 (FMULT).
 * HOW: Entered by a jump with the caller's R11 saved on top of the return
 * address; FxAngleVector makes the vector. The 0 entry exit is not reached
 * in the traces so far.
 * Entry: R8 = the car's record; R10 = stack (saved R11 on top of the
 * return address). Exit: R11 = $C76D (or the saved R11 for a 0 entry); R1-
 * R3 the vector; S, Z from R10; the last RAM address R10.
 */
void abi_FxCarVelocityFromSegment(void)
{
    static const uint8_t field[3] = {0x2E, 0x30, 0x32};
    int i;

    gsu.r[11] = gsu_pull(&gsu.r[10]);
    gsu.r[0] = gsu_add(0x0058, gsu.r[8], 0);
    gsu.r[0] = gsu_ldw(gsu.r[0]);
    gsu_moves(gsu.r[0]);
    if (gsu.z) {
        gsu_pull(&gsu.r[10]);
        return;
    }
    gsu.r[11] = 0xC745;
    abi_FxTableEntryWords();
    gsu.r[11] = 0xC74A;
    abi_FxCourseEntryRead();
    gsu.r[0] = 0x0004;
    gsu.rombr = 0x04;
    gsu.r[0] = gsu_lob(gsu_ldw(0x26F4));
    gsu.r[0] = gsu_add(gsu.r[0], gsu.r[0], 0);
    gsu_r14(0xC781);
    gsu_r14(gsu_add(gsu.r[14], gsu.r[0], 0));
    gsu.r[6] = gsu_getb();
    gsu_r14(gsu_inc(gsu.r[14]));
    gsu.r[6] = gsu_getbh(gsu.r[6]);
    gsu.r[0] = gsu_add(gsu.r[4], gsu.r[4], 0);
    gsu.r[4] = gsu.r[0] = gsu_fmult(gsu.r[0], gsu.r[6]);
    gsu.r[0] = gsu_add(0x0024, gsu.r[8], 0);
    gsu.r[0] = gsu_ldw(gsu.r[0]);
    gsu.r[11] = 0xC76D;
    abi_FxAngleVector();
    for (i = 0; i < 3; i++) {
        gsu.r[0] = gsu_add(field[i], gsu.r[8], 0);
        gsu_stw(gsu.r[0], gsu.r[1 + i]);
    }
    gsu_pull(&gsu.r[10]);
}
