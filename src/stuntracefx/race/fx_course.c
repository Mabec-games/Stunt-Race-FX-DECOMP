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

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode FxCourseEntryRead_code[2][6] = {
    {{0x04CD58, 6}, {0x04CD5E, 4}, {0x04CD62, 2}, {0x04CD7E, 3}, {0, 0}, {0, 0}},
    {{0x04CD58, 6}, {0x04CD5E, 4}, {0x04CD62, 2}, {0x04CD7E, 3}, {0x04C675, 2}, {0x04CD81, 2}},
};

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
FxEntryWords FxCourseEntryRead(uint16_t r0, uint16_t entry, uint16_t stack, int copy)
{
    FxEntryWords o;
    uint16_t r14, w;
    int i;

    gsu_stw(stack, r0);
    stack = gsu_inc(gsu_inc(stack));
    gsu_codes(FxCourseEntryRead_code[copy], 1);
    gsu.rombr = (uint8_t)(gsu_ldw(FX_TABLE_BANK) & 0x7F);
    gsu_codes(FxCourseEntryRead_code[copy] + 1, 1);
    gsu_r14(entry);
    r14 = gsu.r[14];
    gsu_codes(FxCourseEntryRead_code[copy] + 2, 1);
    for (i = 0; i < 4; i++) {
        w = gsu_getb();
        gsu_r14(gsu_inc(r14));
        r14 = gsu.r[14];
        o.w[i] = gsu_getbh(w);
        gsu_r14(gsu_inc(r14));
        r14 = gsu.r[14];
        gsu_codes(FxCourseEntryRead_code[copy] + 3, 1);
    }
    o.r14 = r14;
    stack = gsu_dec(gsu_dec(stack));
    gsu_ldw(stack);
    gsu_codes(FxCourseEntryRead_code[copy] + 4, 2);
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

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode FxLapSegment_code[4][26] = {
    {{0, 0}, {0x04CDCF, 7}, {0x04CDD6, 6}, {0, 0}, {0, 0}, {0x04CDD6, 11}, {0x04CDE1, 6}, {0x04CDE7, 2}, {0x04CDE9, 2}, {0x04CDE9, 5}, {0x04CDEE, 1}, {0x04CDEF, 3}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x04CDF2, 4}, {0x04CDFF, 7}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
    {{0, 0}, {0x04CDCF, 7}, {0x04CDD6, 6}, {0x04CE1A, 1}, {0, 0}, {0x04CDD6, 11}, {0x04CDE1, 6}, {0x04CDE7, 2}, {0, 0}, {0x04CDE9, 5}, {0x04CDEE, 1}, {0x04CDEF, 3}, {0x04CDF2, 13}, {0x04CE1A, 1}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x04CDF2, 4}, {0x04CDFF, 14}, {0, 0}, {0, 0}, {0x04CE0D, 7}, {0x04CE1A, 1}, {0, 0}, {0, 0}},
    {{0x04CDCB, 4}, {0x04CDCF, 7}, {0x04B995, 1}, {0x04CDD6, 6}, {0x04CE1A, 3}, {0x04CDD6, 11}, {0x04CDE1, 6}, {0x04CDE7, 2}, {0, 0}, {0x04CDE9, 5}, {0x04CDEE, 1}, {0x04CDEF, 3}, {0x04B995, 1}, {0x04CDF2, 13}, {0x04CE1A, 3}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
    {{0, 0}, {0x04CDCF, 7}, {0x04CDD6, 6}, {0x04CE1A, 1}, {0, 0}, {0x04CDD6, 11}, {0x04CDE1, 6}, {0x04CDE7, 2}, {0, 0}, {0x04CDE9, 5}, {0x04CDEE, 1}, {0x04CDEF, 3}, {0x04CDF6, 6}, {0, 0}, {0, 0}, {0x04CDF2, 4}, {0x04CDFC, 3}, {0x04CE1A, 1}, {0x04CDFF, 7}, {0, 0}, {0x04CDF2, 4}, {0x04CE06, 7}, {0x04CE0D, 7}, {0x04CE1A, 1}, {0x04CE0D, 12}, {0x04CE19, 2}},
};

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
    gsu_codes(FxLapSegment_code[copy], 1);
    uint16_t count;

    o.r0 = o.r3 = gsu_ldw(gsu_add(REC_SEGMENT, rec, 0));
    gsu_codes(FxLapSegment_code[copy] + 1, 1);
    o.r1 = gsu_ldw(gsu_add(REC_COUNTED, rec, 0));
    gsu_sub(o.r3, o.r1, 0);
    if (gsu.z)
        { gsu_codes(FxLapSegment_code[copy] + 2, 3); return o; } else { gsu_codes(FxLapSegment_code[copy] + 5, 1); }
    o.r0 = gsu_ldw(FX_TABLE_BANK);
    gsu.rombr = (uint8_t)(o.r0 & 0x7F);
    gsu_codes(FxLapSegment_code[copy] + 6, 1);
    o.r0 = gsu_ldw(FX_TABLE_ADDR);
    gsu_codes(FxLapSegment_code[copy] + 7, 1);
    gsu_r14(o.r0);
    o.r14 = gsu.r[14];
    gsu_moves(o.r14);
    if (gsu.z)
        { gsu_codes(FxLapSegment_code[copy] + 8, 1); return o; } else { gsu_codes(FxLapSegment_code[copy] + 9, 1); }
    count = gsu_getb();
    gsu_codes(FxLapSegment_code[copy] + 10, 1);
    gsu_r14(gsu_inc(o.r14));
    o.r14 = gsu.r[14];
    gsu_codes(FxLapSegment_code[copy] + 11, 1);
    o.r4 = count = gsu_getbh(count);
    o.r0 = gsu_sub(o.r3, o.r1, 0);
    if (gsu.s) {
        gsu_codes(FxLapSegment_code[copy] + 12, 3);
        gsu_sub(o.r1, count, 0);           /* wrapped: from the last segment */
        if (!gsu.z)
            { gsu_codes(FxLapSegment_code[copy] + 15, 3); return o; }
    } else {
        gsu_codes(FxLapSegment_code[copy] + 18, 2);
        o.r2 = 5;
        gsu_sub(o.r0, o.r2, 0);
        if (!gsu.s)
            { gsu_code(0x04CE1A, 1); return o; }
    }
    gsu_codes(FxLapSegment_code[copy] + 20, 2);
    o.r0 = gsu_add(REC_COUNTED, rec, 0);
    gsu_stw(o.r0, o.r3);
    o.r2 = 1;
    gsu_sub(o.r3, o.r2, 0);
    if (!gsu.z)
        { gsu_codes(FxLapSegment_code[copy] + 22, 2); return o; } else { gsu_codes(FxLapSegment_code[copy] + 24, 1); }
    o.r0 = gsu_ldw(gsu_add(REC_LAPS, rec, 0));
    o.r0 = gsu_inc(o.r0);
    gsu_codes(FxLapSegment_code[copy] + 25, 1);
    gsu_sbk(o.r0);
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

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode table_word_code[16][7] = {
    {{0x04CC13, 2}, {0x04CC19, 2}, {0x04CC1B, 1}, {0x04CC1C, 2}, {0x04CC1E, 1}, {0x04CC1F, 4}, {0, 0}},
    {{0x04CC34, 2}, {0, 0}, {0x04CC36, 1}, {0x04CC37, 2}, {0x04CC39, 1}, {0x04CC3A, 3}, {0, 0}},
    {{0, 0}, {0, 0}, {0x04CC46, 1}, {0x04CC47, 2}, {0x04CC49, 1}, {0x04CC4A, 3}, {0x04CC4F, 7}},
    {{0x04CC56, 2}, {0, 0}, {0x04CC58, 1}, {0x04CC59, 2}, {0x04CC5B, 1}, {0x04CC5C, 3}, {0x04CC7D, 1}},
    {{0x04CC89, 2}, {0, 0}, {0x04CC8B, 1}, {0x04CC8C, 3}, {0x04CC8F, 1}, {0x04CC90, 2}, {0, 0}},
    {{0, 0}, {0, 0}, {0x04CC92, 1}, {0x04CC93, 3}, {0x04CC96, 1}, {0x04CC97, 2}, {0, 0}},
    {{0, 0}, {0, 0}, {0x04CC99, 1}, {0x04CC9A, 3}, {0x04CC9D, 1}, {0x04CC9E, 2}, {0, 0}},
    {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
    {{0, 0}, {0, 0}, {0x04CC1B, 1}, {0x04CC1C, 2}, {0x04CC1E, 1}, {0, 0}, {0, 0}},
    {{0, 0}, {0, 0}, {0x04CC36, 1}, {0x04CC37, 2}, {0x04CC39, 1}, {0x04CC3A, 3}, {0, 0}},
    {{0, 0}, {0, 0}, {0x04CC46, 1}, {0x04CC47, 2}, {0x04CC49, 1}, {0, 0}, {0, 0}},
    {{0, 0}, {0, 0}, {0x04CC58, 1}, {0x04CC59, 2}, {0x04CC5B, 1}, {0, 0}, {0, 0}},
    {{0, 0}, {0, 0}, {0x04CC8B, 1}, {0x04CC8C, 3}, {0x04CC8F, 1}, {0, 0}, {0, 0}},
    {{0, 0}, {0, 0}, {0x04CC92, 1}, {0x04CC93, 3}, {0x04CC96, 1}, {0, 0}, {0, 0}},
    {{0, 0}, {0, 0}, {0x04CC99, 1}, {0x04CC9A, 3}, {0x04CC9D, 1}, {0, 0}, {0, 0}},
    {{0x04CCE3, 4}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
};

/* A word of the table through the ROM buffer (GETB, INC, GETBH, INC). */
static uint16_t table_word(uint16_t *r14, int copy)
{
    gsu_codes(table_word_code[copy], 2);
    uint16_t w = gsu_getb();

    gsu_codes(table_word_code[copy] + 2, 1);
    gsu_r14(gsu_inc(*r14));
    *r14 = gsu.r[14];
    gsu_codes(table_word_code[copy] + 3, 1);
    w = gsu_getbh(w);
    gsu_codes(table_word_code[copy] + 4, 1);
    gsu_r14(gsu_inc(*r14));
    *r14 = gsu.r[14];
    gsu_codes(table_word_code[copy] + 5, 2);
    return w;
}

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode magnitude_code[5][3] = {
    {{0x04CC3D, 3}, {0x04CC42, 4}, {0x04CC40, 2}},
    {{0, 0}, {0, 0}, {0x04CC4D, 2}},
    {{0x04CC5F, 3}, {0x04CC64, 4}, {0x04CC62, 2}},
    {{0, 0}, {0, 0}, {0x04CC40, 2}},
    {{0, 0}, {0, 0}, {0x04CC62, 2}},
};

/* |v| by NOT and INC when the flags say negative. */
static uint16_t magnitude(uint16_t v, int copy)
{
    gsu_codes(magnitude_code[copy], 2);
    return gsu.s ? (gsu_codes(magnitude_code[copy] + 2, 1), gsu_inc(gsu_not(v))) : v;
}

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode FxCourseNearest_code[7][38] = {
    {{0x04CC0B, 6}, {0x04CC11, 2}, {0, 0}, {0, 0}, {0, 0}, {0x04CC13, 2}, {0x04CC19, 2}, {0, 0}, {0, 0}, {0x04CC1F, 4}, {0x04CC27, 10}, {0xCC31, 0}, {0x04CC31, 3}, {0x04CC34, 2}, {0, 0}, {0x04CC3A, 6}, {0x04CC42, 4}, {0x04CC4A, 3}, {0x04CC4F, 9}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x04CC5C, 6}, {0x04CC64, 4}, {0x04CC7D, 2}, {0x04CC68, 5}, {0x04CC6D, 9}, {0, 0}, {0, 0}, {0, 0}, {0x04CC81, 8}, {0x04CC89, 2}, {0x04CC90, 2}, {0x04CC97, 2}, {0, 0}, {0, 0}, {0, 0}},
    {{0x04CC0B, 6}, {0x04CC11, 2}, {0, 0}, {0, 0}, {0, 0}, {0x04CC13, 2}, {0x04CC19, 2}, {0, 0}, {0, 0}, {0x04CC1F, 4}, {0x04CC27, 10}, {0xCC31, 0}, {0x04CC31, 3}, {0x04CC34, 2}, {0x04CC7A, 4}, {0x04CC3D, 3}, {0x04CC42, 4}, {0x04CC4A, 3}, {0x04CC4F, 7}, {0x04CC76, 2}, {0x04CC78, 2}, {0x04CC7D, 1}, {0x04CC56, 2}, {0x04CC5F, 3}, {0x04CC64, 4}, {0, 0}, {0x04CC68, 5}, {0x04CC6D, 9}, {0x04CC5C, 3}, {0x04CC7D, 1}, {0x04CC7E, 1}, {0x04CC81, 8}, {0x04CC89, 2}, {0x04CC90, 2}, {0x04CC97, 2}, {0, 0}, {0, 0}, {0, 0}},
    {{0x04CC0B, 6}, {0x04CC11, 2}, {0, 0}, {0, 0}, {0, 0}, {0x04CC13, 2}, {0x04CC19, 2}, {0x04CC1F, 8}, {0x04CCA0, 4}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
    {{0x04CC0B, 6}, {0x04CC11, 2}, {0, 0}, {0, 0}, {0, 0}, {0x04CC13, 2}, {0x04CC19, 2}, {0, 0}, {0, 0}, {0x04CC1F, 4}, {0x04CC27, 10}, {0xCC31, 0}, {0x04CC31, 3}, {0x04CC34, 2}, {0x04CC7A, 4}, {0x04CC3D, 3}, {0x04CC42, 4}, {0x04CC4A, 3}, {0x04CC4F, 7}, {0x04CC76, 2}, {0x04CC78, 2}, {0x04CC7D, 1}, {0x04CC56, 2}, {0x04CC5F, 3}, {0x04CC64, 4}, {0, 0}, {0x04CC68, 5}, {0x04CC6D, 9}, {0x04CC5C, 3}, {0x04CC7D, 1}, {0x04CC7E, 1}, {0x04CC81, 8}, {0x04CC89, 2}, {0x04CC90, 2}, {0x04CC97, 2}, {0x04CC9E, 2}, {0, 0}, {0, 0}},
    {{0x04CC0B, 6}, {0x04CC11, 2}, {0x04888C, 1}, {0x04CC13, 6}, {0x04CCA0, 4}, {0x04CC13, 2}, {0x04CC19, 2}, {0, 0}, {0, 0}, {0x04CC1F, 4}, {0x04CC27, 10}, {0xCC31, 0}, {0x04CC31, 3}, {0x04CC34, 2}, {0x04CC7A, 4}, {0x04CC3D, 3}, {0x04CC42, 4}, {0x04CC4A, 3}, {0x04CC4F, 7}, {0x04CC76, 2}, {0x04CC78, 2}, {0x04CC7D, 1}, {0x04CC56, 2}, {0x04CC5F, 3}, {0x04CC64, 4}, {0, 0}, {0x04CC68, 5}, {0x04CC6D, 9}, {0x04CC5C, 3}, {0x04CC7D, 1}, {0x04CC7E, 1}, {0x04CC81, 8}, {0x04CC89, 2}, {0x04CC90, 2}, {0x04CC97, 2}, {0x04CC9E, 2}, {0, 0}, {0, 0}},
    {{0x04CC0B, 6}, {0x04CC11, 2}, {0, 0}, {0, 0}, {0, 0}, {0x04CC13, 2}, {0x04CC19, 2}, {0, 0}, {0, 0}, {0x04CC1F, 4}, {0x04CC27, 10}, {0xCC31, 0}, {0x04CC31, 3}, {0x04CC34, 2}, {0x04CC7A, 4}, {0x04CC3D, 3}, {0x04CC42, 4}, {0x04CC4A, 3}, {0x04CC4F, 7}, {0x04CC76, 2}, {0x04CC78, 2}, {0x04CC7D, 1}, {0x04CC56, 2}, {0x04CC5F, 3}, {0x04CC64, 4}, {0, 0}, {0x04CC68, 5}, {0x04CC6D, 9}, {0x04CC5C, 3}, {0x04CC7D, 1}, {0x04CC7E, 1}, {0x04CC81, 8}, {0x04CC89, 2}, {0x04CC90, 2}, {0x04CC97, 2}, {0x04C393, 5}, {0x04CC9E, 2}, {0x04CCA4, 7}},
    {{0x04CC0B, 6}, {0x04CC11, 2}, {0, 0}, {0, 0}, {0, 0}, {0x04CC13, 2}, {0x04CC19, 2}, {0, 0}, {0, 0}, {0x04CC1F, 4}, {0x04CC27, 10}, {0xCC31, 0}, {0x04CC31, 3}, {0x04CC34, 2}, {0x04CC7A, 4}, {0x04CC3D, 3}, {0x04CC42, 4}, {0x04CC4A, 3}, {0x04CC4F, 9}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x04CC5C, 6}, {0x04CC64, 4}, {0x04CC7D, 1}, {0x04CC68, 5}, {0x04CC6D, 9}, {0, 0}, {0, 0}, {0x04CC7E, 1}, {0x04CC81, 8}, {0x04CC89, 2}, {0x04CC90, 2}, {0x04CC97, 2}, {0, 0}, {0, 0}, {0, 0}},
};

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

    g.r[0] = gsu_ldw(FX_TABLE_BANK);
    gsu.rombr = (uint8_t)(g.r[0] & 0x7F);
    gsu_codes(FxCourseNearest_code[copy], 1);
    g.r[0] = gsu_ldw(FX_TABLE_ADDR);
    gsu_codes(FxCourseNearest_code[copy] + 1, 1);
    gsu_r14(g.r[0]);
    *r14 = gsu.r[14];
    gsu_moves(*r14);
    if (gsu.z) {
        gsu_codes(FxCourseNearest_code[copy] + 2, 3);
        g.r[0] = gsu_sub(g.r[0], g.r[0], 0);
        return g;
    } else { gsu_codes(FxCourseNearest_code[copy] + 5, 2); }
    g.r[0] = table_word(r14, GSU_COPY(0));
    g.r[12] = g.r[0];
    gsu_moves(g.r[12]);
    if (gsu.z) {
        gsu_codes(FxCourseNearest_code[copy] + 7, 2);
        g.r[0] = gsu_sub(g.r[0], g.r[0], 0);
        return g;
    } else { gsu_codes(FxCourseNearest_code[copy] + 9, 4); }
    g.r[5] = 0;
    g.r[6] = 0x7FFF;
    g.r[4] = 0;
    g.r[7] = 0;
    g.r[13] = 0xCC33;
    do {
        gsu_codes(FxCourseNearest_code[copy] + 13, 1);
        g.r[4] = gsu_inc(g.r[4]);
        g.r[0] = gsu_sub(table_word(r14, GSU_COPY(1)), g.r[1], 0);
        if (gsu.ov) {
            gsu_codes(FxCourseNearest_code[copy] + 14, 1);
            gsu_r14(gsu_add(*r14, 4, 0)); /* past y and z */
            *r14 = gsu.r[14];
        } else {
            gsu_codes(FxCourseNearest_code[copy] + 15, 2);
            g.r[9] = magnitude(g.r[0], GSU_COPY(0));
            g.r[0] = gsu_sub(table_word(r14, GSU_COPY(2)), g.r[2], 0);
            gsu_codes(FxCourseNearest_code[copy] + 17, 2);
            g.r[0] = magnitude(g.r[0], GSU_COPY(1));
            g.r[0] = gsu_add(g.r[0], g.r[0], 0);
            g.r[0] = gsu_add(g.r[0], g.r[0], 0);
            g.r[9] = gsu_add(g.r[9], g.r[0], 0);
            if (gsu.ov) {
                gsu_codes(FxCourseNearest_code[copy] + 19, 1);
                gsu_r14(gsu_inc(*r14));  /* past z */
                *r14 = gsu.r[14];
                gsu_codes(FxCourseNearest_code[copy] + 20, 2);
                gsu_r14(gsu_inc(*r14));
                *r14 = gsu.r[14];
            } else {
                gsu_codes(FxCourseNearest_code[copy] + 22, 1);
                g.r[0] = gsu_sub(table_word(r14, GSU_COPY(3)), g.r[3], 0);
                if (!gsu.ov) {
                    gsu_codes(FxCourseNearest_code[copy] + 23, 3);
                    g.r[0] = gsu_add(magnitude(g.r[0], GSU_COPY(2)), g.r[9], 0);
                    if (!gsu.ov) {
                        gsu_codes(FxCourseNearest_code[copy] + 26, 1);
                        gsu_sub(g.r[0], g.r[6], 0);
                        if (!gsu.cy) {
                            gsu_codes(FxCourseNearest_code[copy] + 27, 1);
                            g.r[6] = g.r[0];
                            g.r[5] = *r14;
                            g.r[7] = g.r[4];
                        }
                    }
                }
                gsu_codes(FxCourseNearest_code[copy] + 28, 2);
            }
        }
        gsu_codes(FxCourseNearest_code[copy] + 30, 1);
        gsu_r14(gsu_inc(*r14));
        *r14 = gsu.r[14];
        {
            int again = gsu_count_down(&g.r[12]);

            gsu_r14(gsu_inc(*r14)); /* the fourth word */
            *r14 = gsu.r[14];
            if (!again)
                { gsu_codes(FxCourseNearest_code[copy] + 31, 1); break; }
        }
    } while (1);
    g.r[0] = g.r[7];
    g.r[5] = gsu_sub(g.r[5], 6, 0);
    gsu_r14(g.r[5]);
    *r14 = gsu.r[14];
    gsu_codes(FxCourseNearest_code[copy] + 32, 1);
    g.r[1] = table_word(r14, GSU_COPY(4));
    gsu_codes(FxCourseNearest_code[copy] + 33, 1);
    g.r[2] = table_word(r14, GSU_COPY(5));
    gsu_codes(FxCourseNearest_code[copy] + 34, 1);
    g.r[3] = table_word(r14, GSU_COPY(6));
    gsu_codes(FxCourseNearest_code[copy] + 35, 3);
    return g;
}

void abi_FxCourseNearest(void)
{
    gsu_code(0x04CC07, 4);
    FxRegs g;

    memcpy(g.r, gsu.r, sizeof g.r);
    g = FxCourseNearest(g, GSU_COPY(1));
    memcpy(gsu.r, g.r, 15 * sizeof g.r[0]);
}

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode entry_words_code[10][4] = {
    {{0, 0}, {0x04CD01, 4}, {0, 0}, {0, 0}},
    {{0, 0}, {0, 0}, {0, 0}, {0, 0}},
    {{0x04CCE3, 4}, {0, 0}, {0, 0}, {0, 0}},
    {{0x04CCE3, 4}, {0, 0}, {0, 0}, {0, 0}},
    {{0x04CCE3, 4}, {0x04CD01, 4}, {0x04CEE4, 5}, {0, 0}},
    {{0x04CCE3, 4}, {0x04C5FF, 8}, {0x04CD01, 4}, {0, 0}},
    {{0x04CCE3, 4}, {0x04C6D6, 5}, {0x04CBE8, 3}, {0x04CD01, 4}},
    {{0x04CCE3, 4}, {0x04C5FF, 8}, {0x04CD01, 4}, {0, 0}},
    {{0, 0}, {0, 0}, {0, 0}, {0, 0}},
    {{0x04CCE3, 4}, {0x04C398, 5}, {0x04CBE8, 3}, {0x04CD01, 4}},
};

/* The four words of the entry at R14 into R1-R4, R5 = the entry; R0 = R6. */
static void entry_words(FxRegs *g, int copy)
{
    gsu_codes(entry_words_code[copy], 1);
    int i;

    g->r[5] = g->r[14];
    for (i = 0; i < 4; i++)
        { g->r[1 + i] = table_word(&g->r[14], GSU_COPY(7)); gsu_codes(entry_words_code[copy] + 1, 3); }
    g->r[0] = g->r[6];
}

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode FxCoursePrevEntry_code[2][12] = {
    {{0, 0}, {0x04CD0C, 5}, {0x04CD2F, 5}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x04CD34, 4}, {0, 0}, {0, 0}},
    {{0x04CD06, 6}, {0x04CD0C, 5}, {0x04CD2F, 5}, {0x04CD0C, 10}, {0x04CD16, 1}, {0x04CD17, 1}, {0x04CD18, 2}, {0x04CD1A, 12}, {0x04CD26, 5}, {0, 0}, {0x04888C, 1}, {0x04CD52, 4}},
};

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
    gsu_codes(FxCoursePrevEntry_code[copy], 1);
    uint32_t p;

    g.r[6] = g.r[0];
    g.r[0] = gsu_ldw(FX_TABLE_BANK);
    gsu.rombr = (uint8_t)(g.r[0] & 0x7F);
    g.r[6] = gsu_dec(g.r[6]);
    if (!gsu.z) {
        gsu_codes(FxCoursePrevEntry_code[copy] + 1, 2);
        gsu_r14(gsu_sub(g.r[5], 8, 0));
        g.r[14] = gsu.r[14];
    } else {
        gsu_codes(FxCoursePrevEntry_code[copy] + 3, 1);
        gsu_r14(gsu_ldw(FX_TABLE_ADDR));
        g.r[14] = gsu.r[14];
        g.r[0] = (gsu_codes(FxCoursePrevEntry_code[copy] + 4, 1), gsu_getb());
        gsu_codes(FxCoursePrevEntry_code[copy] + 5, 1);
        gsu_r14(gsu_inc(g.r[14]));
        g.r[14] = gsu.r[14];
        gsu_codes(FxCoursePrevEntry_code[copy] + 6, 1);
        g.r[0] = gsu_getbh(g.r[0]);
        g.r[1] = g.r[0];                       /* the count */
        g.r[0] = gsu_dec(g.r[0]);
        g.r[6] = 8;
        p = gsu_lmult(g.r[0], g.r[6]);
        g.r[4] = (uint16_t)p;
        gsu_codes(FxCoursePrevEntry_code[copy] + 7, 1);
        g.r[0] = gsu_ldw(FX_TABLE_ADDR);
        g.r[0] = gsu_add(g.r[0], 2, 0);
        gsu_codes(FxCoursePrevEntry_code[copy] + 8, 1);
        gsu_r14(gsu_add(g.r[4], g.r[0], 0));
        g.r[14] = gsu.r[14];
        g.r[6] = g.r[1];
    }
    gsu_codes(FxCoursePrevEntry_code[copy] + 9, 1);
    entry_words(&g, GSU_COPY(1));
    gsu_codes(FxCoursePrevEntry_code[copy] + 10, 2);
    return g;
}

void abi_FxCoursePrevEntry(void)
{
    FxRegs g;

    memcpy(g.r, gsu.r, sizeof g.r);
    g = FxCoursePrevEntry(g, GSU_COPY(1));
    memcpy(gsu.r, g.r, 15 * sizeof g.r[0]);
}

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode FxCourseNextEntry_code[7][18] = {
    {{0x04CCA4, 7}, {0, 0}, {0x04CCAB, 8}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x04CCB3, 8}, {0x04CCBF, 8}, {0x04CCC7, 2}, {0x04CCC9, 1}, {0x04CCCA, 3}, {0x04CCCD, 9}, {0x04CCD6, 4}, {0x04CCCD, 4}, {0x04CCDE, 5}, {0, 0}, {0, 0}},
    {{0, 0}, {0, 0}, {0x04CCAB, 8}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x04CCB3, 8}, {0x04CCBF, 8}, {0x04CCC7, 2}, {0x04CCC9, 1}, {0x04CCCA, 3}, {0x04CCCD, 9}, {0x04CCD6, 4}, {0x04CCCD, 4}, {0x04CCDE, 5}, {0x04CCE3, 4}, {0, 0}},
    {{0, 0}, {0, 0}, {0x04CCAB, 8}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x04CCB3, 8}, {0x04CCBF, 8}, {0x04CCC7, 2}, {0x04CCC9, 1}, {0x04CCCA, 3}, {0, 0}, {0, 0}, {0x04CCCD, 4}, {0x04CCDE, 5}, {0, 0}, {0, 0}},
    {{0, 0}, {0, 0}, {0x04CCAB, 8}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x04CCB3, 8}, {0x04CCBF, 8}, {0x04CCC7, 2}, {0x04CCC9, 1}, {0x04CCCA, 3}, {0x04CCCD, 9}, {0x04CCD6, 4}, {0x04CCCD, 4}, {0x04CCDE, 5}, {0, 0}, {0, 0}},
    {{0x04C6D2, 4}, {0x04CCA4, 7}, {0x04CCAB, 8}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x04CCB3, 8}, {0x04CCBF, 8}, {0x04CCC7, 2}, {0x04CCC9, 1}, {0x04CCCA, 3}, {0, 0}, {0, 0}, {0x04CCCD, 4}, {0x04CCDE, 5}, {0, 0}, {0, 0}},
    {{0x04C6D2, 4}, {0x04CCA4, 7}, {0x04CCAB, 8}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x04CCB3, 8}, {0x04CCBF, 8}, {0x04CCC7, 2}, {0x04CCC9, 1}, {0x04CCCA, 3}, {0x04CCCD, 9}, {0x04CCD6, 4}, {0x04CCCD, 4}, {0x04CCDE, 5}, {0x04CCDA, 4}, {0x04CCE2, 5}},
    {{0x04CCA5, 6}, {0, 0}, {0x04CCAB, 8}, {0x04CCB3, 12}, {0x04CD05, 7}, {0x04888C, 1}, {0x04CD52, 4}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
};

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
    gsu_codes(FxCourseNextEntry_code[copy], 2);
    g.r[6] = g.r[0];
    g.r[0] = gsu_ldw(FX_TABLE_BANK);
    gsu.rombr = (uint8_t)(g.r[0] & 0x7F);
    gsu_codes(FxCourseNextEntry_code[copy] + 2, 1);
    g.r[1] = gsu_ldw(gsu_add(REC_DIR, g.r[8], 0));
    g.r[0] = gsu_and(0x0002, g.r[1]);
    g.r[0] = g.r[6];
    if (!gsu.z)
        { gsu_codes(FxCourseNextEntry_code[copy] + 3, 2); return FxCoursePrevEntry(g, GSU_COPY(0)); } else { gsu_codes(FxCourseNextEntry_code[copy] + 7, 2); }
    g.r[6] = gsu_inc(g.r[0]);
    gsu_r14(gsu_ldw(FX_TABLE_ADDR));
    g.r[14] = gsu.r[14];
    g.r[1] = (gsu_codes(FxCourseNextEntry_code[copy] + 9, 1), gsu_getb());
    gsu_codes(FxCourseNextEntry_code[copy] + 10, 1);
    gsu_r14(gsu_inc(g.r[14]));
    g.r[14] = gsu.r[14];
    gsu_codes(FxCourseNextEntry_code[copy] + 11, 1);
    g.r[1] = gsu_getbh(g.r[1]);
    gsu_sub(g.r[0], g.r[1], 0);
    if (gsu.z) {
        gsu_codes(FxCourseNextEntry_code[copy] + 12, 1);
        g.r[0] = gsu_ldw(FX_TABLE_ADDR);
        g.r[0] = gsu_add(g.r[0], 2, 0);
        gsu_codes(FxCourseNextEntry_code[copy] + 13, 1);
        gsu_r14(g.r[0]);
        g.r[14] = gsu.r[14];
        g.r[6] = 1;
    } else {
        gsu_codes(FxCourseNextEntry_code[copy] + 14, 2);
        gsu_r14(gsu_add(g.r[5], 8, 0));
        g.r[14] = gsu.r[14];
    }
    gsu_codes(FxCourseNextEntry_code[copy] + 16, 2);
    entry_words(&g, GSU_COPY(1));
    return g;
}

void abi_FxCourseNextEntry(void)
{
    FxRegs g;

    memcpy(g.r, gsu.r, sizeof g.r);
    g = FxCourseNextEntry(g, GSU_COPY(0));
    memcpy(gsu.r, g.r, 15 * sizeof g.r[0]);
}

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode position_code[3][3] = {
    {{0, 0}, {0, 0}, {0, 0}},
    {{0x04C37B, 8}, {0x04C38F, 4}, {0x04CC06, 5}},
    {{0x04C3B8, 8}, {0x04C3CC, 4}, {0x04CC06, 5}},
};

/* R1-R3 = record R8's position (+$1E, +$20, +$22). */
static void position(FxRegs *g, int copy)
{
    gsu_codes(position_code[copy], 1);
    int i;

    for (i = 0; i < 3; i++)
        { g->r[1 + i] = gsu_ldw(gsu_add((uint16_t)(REC_X + 2 * i), g->r[8], 0)); gsu_codes(position_code[copy] + 1, 2); }
}

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode FxRecordSegment_code[3][6] = {
    {{0, 0}, {0x04C3B8, 8}, {0x04C3CC, 4}, {0x04CC06, 5}, {0x04C3D0, 5}, {0x04CC9E, 2}},
    {{0, 0}, {0x04C3B8, 8}, {0x04C3CC, 4}, {0x04CC06, 5}, {0x04C3D0, 5}, {0, 0}},
    {{0x04C3B6, 2}, {0, 0}, {0, 0}, {0, 0}, {0x04C3D0, 5}, {0x04CC9E, 2}},
};

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
    gsu_codes(FxRecordSegment_code[copy], 1);
    gsu_push(&g.r[10], g.r[11]);
    gsu_codes(FxRecordSegment_code[copy] + 1, 1);
    position(&g, GSU_COPY(0));
    g.r[11] = 0xC3D0;
    gsu_codes(FxRecordSegment_code[copy] + 2, 2);
    g = FxCourseNearest(g, GSU_COPY(1));
    gsu_codes(FxRecordSegment_code[copy] + 4, 2);
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
    gsu_push(&g.r[10], g.r[11]);
    gsu_code(0x04C37B, 8);
    position(&g, GSU_COPY(0));
    g.r[11] = 0xC393;
    gsu_code(0x04C38F, 4); gsu_code(0x04CC06, 5);
    g = FxCourseNearest(g, GSU_COPY(1));
    g.r[11] = 0xC398;
    gsu_code(0x04C393, 5); gsu_code(0x04CC9E, 2); gsu_code(0x04CCA4, 7);
    g = FxCourseNextEntry(g, GSU_COPY(2));
    g.r[11] = 0xC39D;
    gsu_code(0x04C398, 5); gsu_code(0x04CBE8, 3); gsu_code(0x04CD01, 4);
    g = FxBearingTo(g, GSU_COPY(0));
    gsu_code(0x04C39D, 5);
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

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode quarter_apart_code[2][3] = {
    {{0x04CEE9, 3}, {0x04CEEE, 11}, {0x04CEEC, 2}},
    {{0x04CEF9, 3}, {0x04CEFE, 10}, {0x04CEFC, 2}},
};

/* |R0 - b| / 4 with the Super FX's flags: SUB, then NOT and INC when
   negative, then two DIV2s. */
static uint16_t quarter_apart(uint16_t a, uint16_t b, int copy)
{
    gsu_codes(quarter_apart_code[copy], 2);
    uint16_t d = gsu_sub(a, b, 0);

    if (gsu.s)
        { gsu_codes(quarter_apart_code[copy] + 2, 1); d = gsu_inc(gsu_not(d)); }
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
    gsu_code(0x04CED7, 2);
    gsu_push(&gsu.r[10], gsu.r[11]);
    gsu.r[11] = 0xCEDF;
    (gsu_code(0x04CD83, 13), gsu_code(0x04CED9, 6), abi_FxTableEntryWords());
    gsu.r[11] = 0xCEE4;
    (gsu_code(0x04CCA4, 7), gsu_code(0x04CD90, 8), gsu_code(0x04CEDF, 5), abi_FxCourseNextEntry());
    gsu.r[0] = gsu_add(0x001E, gsu.r[8], 0);
    gsu_code(0x04CD01, 4); gsu_code(0x04CEE4, 5);
    gsu.r[0] = quarter_apart(gsu_ldw(gsu.r[0]), gsu.r[1], GSU_COPY(0));
    gsu.r[4] = gsu.r[0];
    gsu.r[0] = gsu_add(0x0022, gsu.r[8], 0);
    gsu.r[0] = quarter_apart(gsu_ldw(gsu.r[0]), gsu.r[3], GSU_COPY(1));
    gsu.r[0] = gsu_add(gsu.r[0], gsu.r[4], 0);
    gsu_pull(&gsu.r[10]);
    gsu_code(0x04B99F, 1);
}

void abi_FxBearingTo(void);
void abi_FxListClearBit0(void);
void abi_FxListSweep(void);

/* R0 = the car's word +off - from, negated (SUB, NOT, INC). */
static uint16_t back_from(uint16_t off, uint16_t from)
{
    gsu_code(0x04C68A, 4);
    gsu.r[0] = gsu_add(off, gsu.r[8], 0);
    gsu.r[0] = gsu_sub(gsu_ldw(gsu.r[0]), from, 0);
    return gsu_inc(gsu_not(gsu.r[0]));
}

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode sweep_if_listed_code[4][10] = {
    {{0x04C6F1, 5}, {0x04C6F6, 4}, {0x04C704, 5}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
    {{0x04C712, 6}, {0x04C718, 4}, {0x04C726, 5}, {0x04A441, 7}, {0x04C718, 9}, {0x04A45B, 14}, {0x04C721, 5}, {0x04A484, 8}, {0x04A4D1, 4}, {0x04C726, 5}},
    {{0x04C6F1, 5}, {0x04C6F6, 4}, {0x04C704, 5}, {0x04A441, 7}, {0x04C6F6, 9}, {0x04A45B, 14}, {0x04C6FF, 5}, {0x04A484, 8}, {0x04A4D1, 4}, {0x04C704, 5}},
    {{0x04C712, 6}, {0x04C718, 4}, {0x04C726, 5}, {0x04A441, 7}, {0x04C718, 9}, {0x04A45B, 14}, {0x04C721, 5}, {0x04A484, 8}, {0x04A4D1, 4}, {0x04C726, 5}},
};

/* For list member R6 with a word +$18, FxListClearBit0 and FxListSweep. */
static void sweep_if_listed(uint16_t r11a, uint16_t r11b, int copy)
{
    gsu_codes(sweep_if_listed_code[copy], 1);
    gsu.r[0] = gsu_add(0x0018, gsu.r[6], 0);
    gsu.r[0] = gsu_ldw(gsu.r[0]);
    gsu_moves(gsu.r[0]);
    if (gsu.z)
        { gsu_codes(sweep_if_listed_code[copy] + 1, 2); return; } else { gsu_codes(sweep_if_listed_code[copy] + 3, 2); }
    gsu.r[11] = r11a;
    abi_FxListClearBit0();
    gsu.r[11] = r11b;
    (gsu_codes(sweep_if_listed_code[copy] + 5, 2), abi_FxListSweep());
    gsu_codes(sweep_if_listed_code[copy] + 7, 3);
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
void abi_FxCarPlaceOnSegment(void)
{
    gsu_code(0x04C607, 4); gsu_code(0x04C65A, 3);
    static const uint8_t at[3] = {0x1E, 0x20, 0x22}, vel[3] = {0x2E, 0x30, 0x32};
    static const uint8_t zero[5] = {0x26, 0x28, 0x36, 0x34, 0x38};
    int i;

    gsu_push(&gsu.r[10], gsu.r[11]);
    gsu.r[0] = gsu_add(0x0058, gsu.r[8], 0);
    gsu_code(0x04C65D, 6);
    gsu.r[0] = gsu_ldw(gsu.r[0]);
    gsu_moves(gsu.r[0]);
    if (gsu.z) {
        gsu_code(0x04C663, 8); gsu_code(0x04C72A, 5);
        gsu_pull(&gsu.r[10]);
        return;
    }
    gsu.r[11] = 0xC670;
    abi_FxTableEntryWords();
    gsu.r[11] = 0xC675;
    abi_FxCourseEntryRead();
    gsu_code(0x04C675, 2); gsu_code(0x04CD81, 2);
    gsu_push(&gsu.r[10], gsu.r[0]);
    gsu.r[0] = 0xFF60;
    gsu.r[2] = gsu_add(gsu.r[2], gsu.r[0], 0);
    gsu_code(0x04C677, 11);
    gsu_stw(0x26F6, gsu.r[1]);
    gsu_code(0x04C682, 4);
    gsu_stw(0x26F8, gsu.r[2]);
    gsu_code(0x04C686, 4);
    gsu_stw(0x26FA, gsu.r[3]);
    for (i = 0; i < 3; i++) {
        gsu.r[4] = gsu.r[0] = back_from(at[i], gsu.r[1 + i]);
        gsu.r[0] = gsu_add(vel[i], gsu.r[8], 0);
        gsu_stw(gsu.r[0], gsu.r[4]);
        gsu_code(0x04C6B4, 7);
    }
    gsu.r[1] = 0x0000;
    for (i = 0; i < 5; i++) {
        gsu.r[0] = gsu_add(zero[i], gsu.r[8], 0);
        gsu_stw(gsu.r[0], gsu.r[1]);
        gsu_code(0x04C6CF, 3);
    }
    gsu.r[0] = gsu_pull(&gsu.r[10]);
    gsu.r[11] = 0xC6D6;
    abi_FxCourseNextEntry();
    gsu.r[11] = 0xC6DB;
    abi_FxBearingTo();
    gsu.r[1] = gsu.r[0];
    gsu.r[0] = gsu_add(0x0024, gsu.r[8], 0);
    gsu_stw(gsu.r[0], gsu.r[1]);
    gsu.r[0] = gsu.r[1];
    gsu_code(0x04C6E3, 4);
    gsu_push(&gsu.r[10], gsu.r[8]);
    gsu.r[0] = gsu_add(0x003E, gsu.r[8], 0);
    gsu_code(0x04C6E7, 6);
    gsu.r[0] = gsu_ldw(gsu.r[0]);
    for (;;) {
        gsu_code(0x04C6ED, 4);
        gsu.r[6] = gsu.r[0];
        gsu_moves(gsu.r[6]);
        if (gsu.z)
            { gsu_code(0x04C70B, 5); break; }
        sweep_if_listed(0xC6FF, 0xC704, GSU_COPY(0));
        gsu.r[0] = gsu_add(0x0034, gsu.r[6], 0);
        gsu.r[0] = gsu_ldw(gsu.r[0]);
        gsu_code(0x04C6EC, 1); gsu_code(0x04C709, 2);
    }
    gsu.r[6] = gsu_pull(&gsu.r[10]);
    gsu_code(0x04C710, 2);
    gsu_push(&gsu.r[10], gsu.r[6]);
    sweep_if_listed(0xC721, 0xC726, GSU_COPY(1));
    gsu.r[8] = gsu_pull(&gsu.r[10]);
    gsu_code(0x04C72B, 4);
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
void abi_FxCarCatchUp(void)
{
    gsu_code(0x04C596, 2);
    static const uint8_t pos[3] = {0x1E, 0x20, 0x22}, vel[3] = {0x2E, 0x30, 0x32};
    int i, place = 1;

    gsu_push(&gsu.r[10], gsu.r[11]);
    gsu.r[0] = 0x0004;
    gsu.rombr = 0x04;
    gsu_code(0x04C598, 13);
    gsu_r14(0xC4D1);
    gsu.r[0] = gsu_lob(gsu_ldw(0x26F4));
    gsu.r[0] = gsu_add(gsu.r[0], gsu.r[0], 0);
    gsu_code(0x04C5A5, 4);
    gsu_r14(gsu_add(gsu.r[14], gsu.r[0], 0));
    gsu.r[0] = (gsu_code(0x04C5A9, 1), gsu_getb());
    gsu_code(0x04C5AA, 1);
    gsu_r14(gsu_inc(gsu.r[14]));
    gsu_code(0x04C5AB, 2);
    gsu.r[0] = gsu_getbh(gsu.r[0]);
    gsu_code(0x04C5AD, 2);
    gsu_r14(gsu.r[0]);
    gsu_code(0x04C5AF, 4);
    gsu.r[2] = gsu_ldw(0x2476);
    gsu.r[1] = 0x0088;
    gsu.r[1] = gsu_add(gsu.r[1], gsu.r[2], 0);
    gsu_code(0x04C5B3, 7);
    gsu.r[1] = gsu_ldw(gsu.r[1]);
    gsu.r[0] = gsu_add(0x0088, gsu.r[8], 0);
    gsu_code(0x04C5BA, 5);
    gsu.r[0] = gsu_sub(gsu_ldw(gsu.r[0]), gsu.r[1], 0);
    gsu.r[0] = gsu_add(gsu.r[0], gsu.r[0], 0);
    gsu.r[0] = gsu_add(gsu.r[0], gsu.r[0], 0);
    gsu_code(0x04C5BF, 5);
    gsu_r14(gsu_add(gsu.r[14], gsu.r[0], 0));
    gsu.r[3] = (gsu_code(0x04C5C4, 2), gsu_getb());
    gsu_code(0x04C5C6, 1);
    gsu_r14(gsu_inc(gsu.r[14]));
    gsu_code(0x04C5C7, 3);
    gsu.r[3] = gsu_getbh(gsu.r[3]);
    gsu_code(0x04C5CA, 1);
    gsu_r14(gsu_inc(gsu.r[14]));
    gsu.r[6] = (gsu_code(0x04C5CB, 2), gsu_getb());
    gsu_code(0x04C5CD, 1);
    gsu_r14(gsu_inc(gsu.r[14]));
    gsu_code(0x04C5CE, 3);
    gsu.r[6] = gsu_getbh(gsu.r[6]);
    gsu_code(0x04C5D1, 1);
    gsu_r14(gsu_inc(gsu.r[14]));
    gsu_code(0x04C5D2, 2);
    gsu_push(&gsu.r[10], gsu.r[6]);
    gsu.r[0] = gsu_add(0x0072, gsu.r[8], 0);
    gsu_code(0x04C5D4, 6);
    gsu.r[0] = gsu_inc(gsu_ldw(gsu.r[0]));
    gsu_sbk(gsu.r[0]);
    if (!gsu.z) {
        gsu_code(0x04C5DE, 4);
        gsu_sub(gsu.r[0], gsu.r[3], 0);
        if (gsu.s)
            { gsu_code(0x04C5E2, 4); place = 0; }
        else {
            gsu_code(0x04C5E6, 3);
            gsu.r[0] = gsu_sub(gsu.r[0], gsu.r[0], 0);
            gsu_sbk(gsu.r[0]);
        }
    }
    if (place) {
        gsu_code(0x04C5E9, 4);
        gsu.r[0] = gsu_add(0x0058, gsu.r[8], 0);
        gsu.r[0] = gsu_ldw(gsu.r[0]);
        gsu_moves(gsu.r[0]);
        if (!gsu.z) {
            gsu.r[11] = 0xC5FA;
            abi_FxTableEntryWords();
            gsu.r[11] = 0xC5FF;
            abi_FxCourseNextEntry();
            gsu.r[6] = gsu.r[0];
            gsu.r[0] = gsu_add(0x0058, gsu.r[8], 0);
            gsu_stw(gsu.r[0], gsu.r[6]);
            gsu.r[11] = 0xC60B;
            (gsu_code(0x04C607, 4), gsu_code(0x04C65A, 1), abi_FxCarPlaceOnSegment());
        } else { gsu_code(0x04C5ED, 8); }
    }
    gsu_code(0x04C60B, 5);
    gsu.r[6] = gsu_pull(&gsu.r[10]);
    gsu_code(0x04C610, 4);
    for (i = 0; i < 3; i++) {
        gsu.r[0] = gsu_add(vel[i], gsu.r[8], 0);
        gsu.r[0] = gsu_ldw(gsu.r[0]);
        gsu.r[1 + i] = pos[i];
        gsu.r[1 + i] = gsu_add(gsu.r[1 + i], gsu.r[8], 0);
        gsu.r[1 + i] = gsu_ldw(gsu.r[1 + i]);
        gsu.r[0] = gsu_fmult(gsu.r[0], gsu.r[6]);
        gsu.r[0] = gsu_add(gsu.r[0], gsu.r[1 + i], 0);
        gsu_sbk(gsu.r[0]);
        gsu_code(0x04C637, 4);
    }
    gsu.r[0] = gsu_add(0x003E, gsu.r[8], 0);
    gsu.r[0] = gsu_ldw(gsu.r[0]);
    for (;;) {
        gsu_code(0x04C63B, 4);
        gsu.r[6] = gsu.r[0];
        gsu_moves(gsu.r[6]);
        if (gsu.z)
            { gsu_code(0x04C655, 5); break; } else { gsu_code(0x04C63F, 6); }
        for (i = 0; i < 3; i++) {
            gsu.r[0] = gsu_add(pos[i], gsu.r[6], 0);
            gsu_stw(gsu.r[0], gsu.r[1 + i]);
            gsu_code(0x04C64F, 4);
        }
        gsu.r[0] = gsu_add(0x0034, gsu.r[6], 0);
        gsu.r[0] = gsu_ldw(gsu.r[0]);
        gsu_code(0x04C63A, 1); gsu_code(0x04C653, 2);
    }
    gsu_pull(&gsu.r[10]);
}
