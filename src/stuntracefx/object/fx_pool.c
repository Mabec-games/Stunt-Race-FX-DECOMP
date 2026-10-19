/*
 * fx_pool.c: the Super FX's two pools of record memory.
 *
 * Each pool is a chain of blocks: a block's first word links to the next
 * block (0 ends the chain), its second word is its size in bytes, negative
 * while the block is in use. The first pool's chain starts at the word at
 * $70:1098, the second's at $70:1D70 (the S-CPU's side: GsuRecordNew* in
 * object/gsu_record.c). Records in use sit on lists linked through +$0E,
 * with bit 0 of +$10 marking those still wanted.
 */
#include <string.h>
#include "gsu_ops.h"
#include "fx_calls.h"

#define FX_POOL_FIRST 0x1098u
#define FX_POOL_SECOND 0x1D70u
#define FX_STACK 0x0240u       /* the R10 stack programs start with */

#define REC_NEXT 0x0Eu
#define REC_FLAGS10 0x10u
#define REC_LIST 0x18u

typedef struct {
    uint16_t r0, r1, r2, r3;
} FxPoolWalk;

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode pool_merge_code[2][12] = {
    {{0x04889B, 3}, {0x04889E, 8}, {0x0488A6, 14}, {0x0488B4, 5}, {0x0488B9, 11}, {0x0488C4, 3}, {0x0488C7, 1}, {0x0488B9, 4}, {0x0488CE, 7}, {0x0488C9, 5}, {0x0488A6, 4}, {0x0488C9, 5}},
    {{0x0488D8, 3}, {0x0488DB, 8}, {0x0488E3, 14}, {0x0488F1, 5}, {0x0488F6, 11}, {0x048901, 3}, {0x048904, 1}, {0x0488F6, 4}, {0x04890B, 7}, {0x048906, 5}, {0x0488E3, 4}, {0x048906, 5}},
};

/* One pool: each free block whose end is the next block in the chain, and
   that next block free, takes it in: sizes added, its link taken over. The
   walk then goes on from the block taken in. */
static void pool_merge(uint16_t head, FxPoolWalk *w, int copy)
{
    w->r1 = head;
    for (;;) {
        gsu_codes(pool_merge_code[copy], 1);
        w->r2 = gsu_ldw(w->r1);                      /* the next block */
        w->r1 = gsu_inc(gsu_inc(w->r1));             /* this block + 2 */
        gsu_moves(w->r2);
        if (gsu.z)
            return; else { gsu_codes(pool_merge_code[copy] + 1, 1); }
        w->r0 = gsu_ldw(w->r1);                      /* this block's size */
        gsu_moves(w->r0);
        if (!gsu.s) {
            gsu_codes(pool_merge_code[copy] + 2, 1);
            w->r0 = gsu_sub(w->r0, 2, 0);
            w->r3 = gsu_add(w->r0, w->r1, 0);        /* this block's end */
            gsu_sub(w->r3, w->r2, 0);
            if (gsu.z) {
                gsu_codes(pool_merge_code[copy] + 3, 1);
                w->r2 = gsu_inc(gsu_inc(w->r2));
                w->r3 = gsu_ldw(w->r2);              /* the next block's size */
                gsu_moves(w->r3);
                if (!gsu.s) {
                    gsu_codes(pool_merge_code[copy] + 4, 1);
                    w->r0 = gsu_add(gsu_add(w->r0, w->r3, 0), 2, 0);
                    w->r2 = gsu_dec(gsu_dec(w->r2));
                    gsu_stw(w->r1, w->r0);
                    w->r1 = gsu_dec(gsu_dec(w->r1));
                    gsu_codes(pool_merge_code[copy] + 5, 1);
                    w->r0 = gsu_ldw(w->r2);          /* its link */
                    gsu_codes(pool_merge_code[copy] + 6, 1);
                    gsu_stw(w->r1, w->r0);
                    w->r1 = gsu_inc(gsu_inc(w->r1));
                } else {
                    gsu_codes(pool_merge_code[copy] + 7, 2);
                    w->r2 = gsu_dec(gsu_dec(w->r2));
                }
            } else { gsu_codes(pool_merge_code[copy] + 9, 1); }
        } else { gsu_codes(pool_merge_code[copy] + 10, 2); }
        w->r1 = w->r2;
    }
}

/*
 * @routine 04:8898 FxPoolsMerge
 * WHAT IT DOES: Merges neighbouring free blocks in both pools, the first
 * pool's chain and then the second's.
 * HOW: A walk of each chain (pool_merge): a free block (size not negative)
 * whose address plus size is the next block's address, the next block also
 * free, gets the sum of the sizes and the next block's link. The walk goes
 * on from the block merged in, so a run of three free blocks merges only
 * the first two in one pass.
 * Entry: R11 = return address. Exit: R2 = 0; R1 = the second chain's last
 * block + 2; R0 and R3 from the walk's last block; OV, S clear, Z set; the
 * last RAM address that block's link.
 */
FxPoolWalk FxPoolsMerge(FxPoolWalk w)
{
    gsu_code(0x04888F, 7); gsu_code(0x048898, 3);
    pool_merge(FX_POOL_FIRST, &w, GSU_COPY(0));
    gsu_code(0x04889E, 6); gsu_code(0x0488D5, 3);
    pool_merge(FX_POOL_SECOND, &w, GSU_COPY(1));
    gsu_code(0x048896, 1); gsu_code(0x0488DB, 6); gsu_code(0x048912, 3);
    return w;
}

void abi_FxPoolsMerge(void)
{
    FxPoolWalk w = {gsu.r[0], gsu.r[1], gsu.r[2], gsu.r[3]};

    gsu_code(0x048899, 3);
    w = FxPoolsMerge(w);
    gsu.r[0] = w.r0;
    gsu.r[1] = w.r1;
    gsu.r[2] = w.r2;
    gsu.r[3] = w.r3;
    gsu_code(0x0488db, 6); gsu_code(0x048912, 3);
}

/*
 * @routine 04:888E FxPoolsMergeProgram
 * WHAT IT DOES: A program: FxPoolsMerge.
 * HOW: R10 = $0240 for the stack, then the call.
 * Entry: the S-CPU starts it. Exit: (at its STOP) as FxPoolsMerge; R10 =
 * $0240; R11 = $8896.
 */
FxPoolWalk FxPoolsMergeProgram(FxPoolWalk w)
{
    return FxPoolsMerge(w);
}

void abi_FxPoolsMergeProgram(void)
{
    gsu.r[10] = FX_STACK;
    gsu.r[11] = 0x8896;
    gsu_code(0x04888f, 7); gsu_code(0x048898, 1);
    abi_FxPoolsMerge();
    gsu_code(0x048896, 1);
}

typedef struct {
    uint16_t r0, r2, r7, r8, r9, r12;
} FxSweepRegs;

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode FxListSweep_code[5][29] = {
    {{0x04A469, 14}, {0, 0}, {0x04A477, 8}, {0x04A47F, 6}, {0x04A477, 4}, {0x04A48C, 7}, {0x04A493, 10}, {0x04A493, 4}, {0x04A49F, 6}, {0, 0}, {0, 0}, {0x04A4A7, 5}, {0x04A4B0, 3}, {0, 0}, {0, 0}, {0x04A4AC, 4}, {0x04A4B3, 11}, {0x04A4BE, 8}, {0x04A4BE, 4}, {0x04A4C8, 2}, {0x04A4CA, 1}, {0x04A4CB, 2}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
    {{0x04A469, 9}, {0x04A472, 5}, {0x04A477, 8}, {0x04A47F, 6}, {0x04A477, 4}, {0x04A48C, 7}, {0x04A493, 10}, {0x04A493, 4}, {0x04A49F, 6}, {0x04A49D, 2}, {0x04A4A4, 3}, {0x04A4A7, 5}, {0x04A4B0, 3}, {0, 0}, {0, 0}, {0x04A4AC, 4}, {0x04A4B3, 11}, {0x04A4BE, 8}, {0x04A4BE, 4}, {0x04A4C8, 2}, {0x04A4CA, 1}, {0x04A4CB, 2}, {0, 0}, {0, 0}, {0x04A484, 5}, {0x04A4D1, 4}, {0, 0}, {0x048A25, 5}, {0x04A489, 3}},
    {{0x04A469, 14}, {0, 0}, {0x04A477, 8}, {0x04A47F, 6}, {0x04A477, 4}, {0x04A48C, 7}, {0x04A493, 10}, {0x04A493, 4}, {0x04A49F, 6}, {0x04A49D, 2}, {0x04A4A4, 3}, {0x04A4A7, 5}, {0x04A4AC, 18}, {0x04A4AC, 4}, {0x04A4B3, 11}, {0, 0}, {0, 0}, {0x04A4BE, 8}, {0x04A4BE, 4}, {0x04A4C8, 2}, {0x04A4CA, 1}, {0x04A4CB, 2}, {0x04A4C6, 2}, {0x04A4CC, 5}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
    {{0x04A469, 14}, {0, 0}, {0x04A477, 8}, {0x04A47F, 6}, {0x04A477, 4}, {0x04A48C, 7}, {0x04A493, 10}, {0x04A493, 4}, {0x04A49F, 6}, {0x04A49D, 2}, {0x04A4A4, 3}, {0x04A4A7, 5}, {0x04A4B0, 3}, {0x04A4AC, 4}, {0x04A4B3, 11}, {0, 0}, {0, 0}, {0x04A4BE, 8}, {0x04A4BE, 4}, {0x04A4C8, 2}, {0x04A4CA, 1}, {0x04A4CB, 2}, {0x04A4CD, 4}, {0, 0}, {0x048A25, 5}, {0x04A484, 8}, {0x04A4D1, 4}, {0, 0}, {0, 0}},
    {{0x04A469, 14}, {0, 0}, {0x04A477, 8}, {0x04A47F, 6}, {0x04A477, 4}, {0x04A48C, 7}, {0x04A493, 10}, {0x04A493, 4}, {0x04A49F, 6}, {0x04A49D, 2}, {0x04A4A4, 3}, {0x04A4A7, 5}, {0x04A4B0, 3}, {0x04A4AC, 4}, {0x04A4B3, 11}, {0, 0}, {0, 0}, {0x04A4BE, 8}, {0x04A4BE, 4}, {0x04A4C8, 2}, {0x04A4CA, 1}, {0x04A4CB, 2}, {0x04A4CD, 4}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
};

/*
 * @routine 04:A462 FxListSweep
 * WHAT IT DOES: Sweeps the list whose head is at +$18 of record R6: each
 * record with bit 0 of +$10 set stays and has the bit cleared; each with it
 * clear is unlinked and its memory given back. A block with a positive size
 * goes on the front of the first pool's chain ($1098); one with a negative
 * size has the size negated in place.
 * HOW: R2 follows the last record kept (0 before any), so an unlinked
 * record's successor goes to R2's +$0E or to the head. The successor is
 * pushed on the R10 stack around the freeing. The freeing first negates the
 * size into R7 (when negative), a value overwritten at once by $1098.
 * Entry: R6 = the record holding the list; R10 = stack; R11 = return
 * address. Exit: R8 = 0; with records: R0 = 0, R12 = 1, R2 = the last
 * record kept or 0, R9 and R7 from the last record seen; OV, S clear, Z
 * set; the last RAM address the last word read or written.
 */
FxSweepRegs FxListSweep(uint16_t owner, uint16_t stack, FxSweepRegs g, int copy)
{
    g.r8 = gsu_ldw(gsu_add(REC_LIST, owner, 0));
    gsu_moves(g.r8);
    if (gsu.z)
        return g; else { gsu_codes(FxListSweep_code[copy], 1); }
    g.r12 = 1;
    g.r2 = 0;
    do {
        gsu_codes(FxListSweep_code[copy] + 1, 1);
        g.r0 = gsu_ldw(gsu_add(REC_FLAGS10, g.r8, 0));
        g.r9 = gsu_and(g.r0, g.r12);
        if (!gsu.z) {
            gsu_codes(FxListSweep_code[copy] + 2, 1);
            g.r0 = gsu_bic(g.r0, g.r12);             /* kept: clear the mark */
            gsu_sbk(g.r0);
            g.r2 = g.r8;
            gsu_codes(FxListSweep_code[copy] + 3, 1);
            g.r0 = gsu_ldw(gsu_add(REC_NEXT, g.r8, 0));
        } else {
            gsu_codes(FxListSweep_code[copy] + 4, 2);
            g.r7 = gsu_ldw(gsu_add(REC_NEXT, g.r8, 0));
            gsu_moves(g.r2);
            if (gsu.z)
                { gsu_codes(FxListSweep_code[copy] + 6, 1); gsu_stw(gsu_add(REC_LIST, owner, 0), g.r7); }
            else
                { gsu_codes(FxListSweep_code[copy] + 7, 2); gsu_stw(gsu_add(REC_NEXT, g.r2, 0), g.r7); }
            gsu_codes(FxListSweep_code[copy] + 9, 2);
            gsu_stw(stack, g.r7);
            stack = gsu_inc(gsu_inc(stack));
            g.r8 = gsu_inc(gsu_inc(g.r8));
            gsu_codes(FxListSweep_code[copy] + 11, 1);
            g.r0 = gsu_ldw(g.r8);
            g.r7 = g.r0;
            gsu_moves(g.r7);
            if (gsu.s)
                { gsu_codes(FxListSweep_code[copy] + 12, 1); g.r7 = gsu_inc(gsu_not(g.r7)); } else { gsu_codes(FxListSweep_code[copy] + 13, 2); }
            gsu_codes(FxListSweep_code[copy] + 15, 2);
            g.r8 = gsu_dec(gsu_dec(g.r8));
            g.r7 = FX_POOL_FIRST;
            g.r9 = gsu_inc(gsu_inc(g.r8));
            g.r0 = gsu_ldw(g.r9);
            gsu_moves(g.r0);
            if (gsu.s) {
                gsu_codes(FxListSweep_code[copy] + 17, 1);
                g.r0 = gsu_inc(gsu_not(g.r0));
                gsu_stw(g.r9, g.r0);
            } else {
                gsu_codes(FxListSweep_code[copy] + 18, 2);
                g.r0 = gsu_ldw(g.r7);
                gsu_codes(FxListSweep_code[copy] + 20, 1);
                gsu_stw(g.r8, g.r0);
                gsu_codes(FxListSweep_code[copy] + 21, 1);
                gsu_stw(g.r7, g.r8);
            }
            gsu_codes(FxListSweep_code[copy] + 22, 2);
            stack = gsu_dec(gsu_dec(stack));
            g.r7 = gsu_ldw(stack);
            g.r0 = g.r7;
        }
        gsu_codes(FxListSweep_code[copy] + 24, 3);
        g.r8 = g.r0;
        gsu_moves(g.r8);
    } while (!gsu.z);
    gsu_codes(FxListSweep_code[copy] + 27, 2);
    return g;
}

/* FxListSweep on the whole register file, for other routines' C. */
FxRegs FxListSweepRegs(FxRegs g)
{
    FxSweepRegs s = {g.r[0], g.r[2], g.r[7], g.r[8], g.r[9], g.r[12]};

    s = FxListSweep(g.r[6], g.r[10], s, GSU_COPY(0));
    g.r[0] = s.r0;
    g.r[2] = s.r2;
    g.r[7] = s.r7;
    g.r[8] = s.r8;
    g.r[9] = s.r9;
    g.r[12] = s.r12;
    return g;
}

void abi_FxListSweep(void)
{
    FxSweepRegs g = {gsu.r[0], gsu.r[2], gsu.r[7], gsu.r[8], gsu.r[9], gsu.r[12]};

    g = FxListSweep(gsu.r[6], gsu.r[10], g, GSU_COPY(1));
    gsu.r[0] = g.r0;
    gsu.r[2] = g.r2;
    gsu.r[7] = g.r7;
    gsu.r[8] = g.r8;
    gsu.r[9] = g.r9;
    gsu.r[12] = g.r12;
}

#define FX_RECORDS 0x2332u      /* the list of all records, linked +$04/+$06 */
#define REC_ALL_NEXT 0x04u
#define REC_ALL_PREV 0x06u
#define REC_POOL 0x08u
#define REC_FLAGS1C 0x1Cu

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode FxPoolTake_code[9][49] = {
    {{0x04893A, 3}, {0x048D4E, 11}, {0x04893D, 4}, {0x048941, 2}, {0x8943, 0}, {0x048943, 5}, {0x048948, 1}, {0x048949, 3}, {0x04894C, 3}, {0x04894F, 3}, {0x04896E, 4}, {0x04894F, 8}, {0x048957, 5}, {0x04895C, 2}, {0, 0}, {0x04895E, 3}, {0x048961, 3}, {0x048978, 5}, {0x048986, 4}, {0x048961, 13}, {0, 0}, {0, 0}, {0x04895B, 1}, {0x04898A, 5}, {0, 0}, {0x04898F, 3}, {0x048992, 5}, {0x048997, 4}, {0x04899B, 4}, {0x0489F7, 3}, {0x04899B, 27}, {0x0489BC, 2}, {0x0489B6, 6}, {0x0489BE, 13}, {0, 0}, {0, 0}, {0x0489CB, 3}, {0x0489CE, 2}, {0x0489CE, 8}, {0x0489D6, 4}, {0x0489DA, 5}, {0x0489DF, 4}, {0x0489E3, 10}, {0x0489E3, 4}, {0x0489EC, 1}, {0x0489ED, 6}, {0x0489F3, 5}, {0x0489F8, 2}, {0, 0}},
    {{0, 0}, {0, 0}, {0x04893D, 4}, {0x048941, 2}, {0x8943, 0}, {0x048943, 5}, {0x048948, 1}, {0x048949, 3}, {0x04894C, 3}, {0x04894F, 3}, {0x04896E, 4}, {0x04894F, 8}, {0x048957, 5}, {0x04895C, 2}, {0, 0}, {0x04895E, 3}, {0x048961, 3}, {0x048978, 5}, {0x048986, 4}, {0x048961, 13}, {0, 0}, {0, 0}, {0x04895B, 1}, {0x04898A, 5}, {0, 0}, {0x04898F, 8}, {0, 0}, {0x048997, 4}, {0x04899B, 4}, {0x0489F7, 1}, {0x04899B, 27}, {0x0489BC, 2}, {0x0489B6, 6}, {0x0489BE, 13}, {0, 0}, {0, 0}, {0x0489CB, 3}, {0x0489CE, 2}, {0x0489CE, 8}, {0x0489D6, 4}, {0x0489DA, 5}, {0x0489DF, 4}, {0x0489E3, 10}, {0x0489E3, 4}, {0x0489EC, 1}, {0x0489ED, 6}, {0x0489F3, 5}, {0, 0}, {0, 0}},
    {{0, 0}, {0, 0}, {0x04893D, 4}, {0x048941, 2}, {0x8943, 0}, {0x048943, 5}, {0x048948, 1}, {0x048949, 3}, {0x04894C, 3}, {0x04894F, 3}, {0x04896E, 4}, {0x04894F, 8}, {0x048957, 5}, {0x04895C, 2}, {0, 0}, {0x04895E, 3}, {0x048961, 3}, {0x048978, 5}, {0x048986, 4}, {0x048961, 13}, {0, 0}, {0, 0}, {0x04895B, 1}, {0x04898A, 5}, {0, 0}, {0x04898F, 8}, {0, 0}, {0x048997, 4}, {0x04899B, 4}, {0x0489F7, 1}, {0x04899B, 27}, {0x0489BC, 2}, {0, 0}, {0, 0}, {0x0489B6, 6}, {0x0489BE, 13}, {0x0489CB, 3}, {0x0489CE, 2}, {0x0489CE, 8}, {0x0489D6, 4}, {0x0489DA, 5}, {0x0489DF, 4}, {0x0489E3, 10}, {0x0489E3, 4}, {0x0489EC, 1}, {0x0489ED, 6}, {0x0489F3, 5}, {0, 0}, {0, 0}},
    {{0, 0}, {0, 0}, {0x04893D, 4}, {0x048941, 2}, {0x8943, 0}, {0x048943, 5}, {0x048948, 1}, {0x048949, 3}, {0x04894C, 3}, {0x04894F, 3}, {0x04896E, 4}, {0x04894F, 8}, {0x048957, 5}, {0, 0}, {0x04895C, 2}, {0x04895E, 3}, {0x048961, 3}, {0x048978, 5}, {0x048986, 4}, {0x048961, 8}, {0, 0}, {0x048969, 5}, {0x04895B, 1}, {0x04898A, 5}, {0, 0}, {0x04898F, 3}, {0x048992, 5}, {0x048997, 4}, {0x04899B, 4}, {0x0489F7, 1}, {0x04899B, 27}, {0x0489BC, 2}, {0, 0}, {0, 0}, {0x0489B6, 6}, {0x0489BE, 13}, {0x0489CB, 3}, {0x0489CE, 2}, {0x0489CE, 8}, {0x0489D6, 4}, {0x0489DA, 5}, {0x0489DF, 4}, {0x0489E3, 10}, {0x0489E3, 4}, {0x0489EC, 1}, {0x0489ED, 6}, {0x0489F3, 5}, {0, 0}, {0, 0}},
    {{0x04893B, 2}, {0, 0}, {0x04893D, 4}, {0x048941, 2}, {0x8943, 0}, {0x048943, 5}, {0x048948, 1}, {0x048949, 3}, {0x04894C, 3}, {0x04894F, 3}, {0x04896E, 4}, {0x04894F, 8}, {0x048957, 5}, {0, 0}, {0x04895C, 2}, {0x04895E, 3}, {0x048961, 3}, {0x048978, 18}, {0, 0}, {0x04895B, 1}, {0x048961, 8}, {0, 0}, {0, 0}, {0x04898A, 5}, {0, 0}, {0x04898F, 8}, {0, 0}, {0x048997, 4}, {0x04899B, 4}, {0x0489F7, 1}, {0x04899B, 27}, {0x0489B6, 21}, {0x0489B6, 6}, {0x0489BE, 13}, {0, 0}, {0, 0}, {0x0489CB, 3}, {0x0489CE, 2}, {0x0489CE, 8}, {0x0489D6, 4}, {0x0489DA, 5}, {0x0489DF, 4}, {0x0489E3, 10}, {0x0489E3, 4}, {0x0489EC, 1}, {0x0489ED, 6}, {0x0489F3, 5}, {0x04888C, 1}, {0x0489F8, 2}},
    {{0, 0}, {0, 0}, {0x04893D, 4}, {0x048941, 2}, {0x8943, 0}, {0x048943, 5}, {0x048948, 4}, {0, 0}, {0x04894C, 3}, {0x04894F, 3}, {0x04896E, 4}, {0x04894F, 13}, {0, 0}, {0x04895C, 2}, {0, 0}, {0x04895E, 3}, {0x048961, 3}, {0x048978, 5}, {0x048986, 4}, {0x048961, 13}, {0x048992, 5}, {0, 0}, {0x04895B, 1}, {0x04898A, 5}, {0, 0}, {0x04898F, 3}, {0, 0}, {0x048997, 4}, {0, 0}, {0, 0}, {0x04899B, 27}, {0x0489BC, 2}, {0x0489B6, 6}, {0x0489BE, 13}, {0, 0}, {0, 0}, {0x0489CB, 3}, {0x0489CE, 2}, {0x0489CE, 8}, {0x0489D6, 4}, {0x0489DA, 5}, {0x0489DF, 4}, {0x0489E3, 10}, {0x0489E3, 4}, {0x0489EC, 1}, {0x0489ED, 6}, {0x0489F3, 5}, {0, 0}, {0, 0}},
    {{0, 0}, {0, 0}, {0x04893D, 4}, {0x048941, 2}, {0x8943, 0}, {0x048943, 5}, {0x048948, 1}, {0x048949, 3}, {0x04894C, 3}, {0x04894F, 3}, {0x04896E, 4}, {0x04894F, 8}, {0x048957, 5}, {0x04895C, 2}, {0, 0}, {0x04895E, 3}, {0x048961, 3}, {0x048978, 5}, {0x048986, 4}, {0x048961, 13}, {0, 0}, {0, 0}, {0x04895B, 1}, {0x04898A, 5}, {0, 0}, {0x04898F, 3}, {0x048992, 5}, {0x048997, 4}, {0, 0}, {0, 0}, {0x04899B, 27}, {0x0489BC, 2}, {0x0489B6, 6}, {0x0489BE, 13}, {0, 0}, {0, 0}, {0x0489CB, 3}, {0x0489CE, 2}, {0x0489CE, 8}, {0x0489D6, 4}, {0x0489DA, 5}, {0x0489DF, 4}, {0x0489E3, 10}, {0x0489E3, 4}, {0x0489EC, 1}, {0x0489ED, 6}, {0x0489F3, 5}, {0, 0}, {0, 0}},
    {{0, 0}, {0, 0}, {0x04893D, 4}, {0x048941, 2}, {0x8943, 0}, {0x048943, 5}, {0x048948, 1}, {0x048949, 3}, {0x04894C, 3}, {0x04894F, 3}, {0x04896E, 4}, {0x04894F, 8}, {0x048957, 5}, {0x04895C, 2}, {0, 0}, {0x04895E, 3}, {0x048961, 3}, {0x048978, 5}, {0x048986, 4}, {0x048961, 13}, {0, 0}, {0, 0}, {0x04895B, 1}, {0x04898A, 5}, {0, 0}, {0x04898F, 3}, {0x048992, 5}, {0x048997, 4}, {0x04899B, 4}, {0x0489F7, 1}, {0x04899B, 27}, {0x0489BC, 2}, {0x0489B6, 6}, {0x0489BE, 13}, {0, 0}, {0, 0}, {0x0489CB, 3}, {0x0489CE, 2}, {0x0489CE, 8}, {0x0489D6, 4}, {0x0489DA, 5}, {0x0489DF, 4}, {0x0489E3, 10}, {0x0489E3, 4}, {0x0489EC, 1}, {0x0489ED, 6}, {0x0489F3, 5}, {0, 0}, {0, 0}},
    {{0, 0}, {0, 0}, {0x04893D, 4}, {0x048941, 2}, {0x8943, 0}, {0x048943, 5}, {0x048948, 1}, {0x048949, 3}, {0x04894C, 3}, {0x04894F, 3}, {0x04896E, 4}, {0x04894F, 8}, {0x048957, 5}, {0x04895C, 2}, {0, 0}, {0x04895E, 3}, {0x048961, 3}, {0x048978, 5}, {0x048986, 4}, {0x048961, 13}, {0, 0}, {0, 0}, {0x04895B, 1}, {0x048972, 6}, {0x048989, 6}, {0x04898F, 3}, {0x048992, 5}, {0x048997, 4}, {0x04899B, 4}, {0x0489F7, 1}, {0x04899B, 27}, {0x0489B6, 21}, {0x0489B6, 6}, {0x0489BE, 13}, {0, 0}, {0, 0}, {0x0489CB, 3}, {0x0489CE, 2}, {0x0489CE, 8}, {0x0489D6, 4}, {0x0489DA, 5}, {0x0489DF, 4}, {0x0489E3, 10}, {0x0489E3, 4}, {0x0489EC, 1}, {0x0489ED, 6}, {0x0489F3, 5}, {0, 0}, {0, 0}},
};

/*
 * @routine 04:893A FxPoolTake
 * WHAT IT DOES: Takes a block of R4 bytes from the pool whose chain head is
 * at R3 ($1098 or $1D70) for a new record: R1 = the record, or 0 when no
 * block is big enough. The record's bytes from +4 to its end are cleared,
 * +$08 = the pool, it goes on the front of the list of all records at
 * $2332 (+$04 next, +$06 previous) and bit 2 of +$1C is set.
 * HOW: A first walk looks for a free block of exactly R4 bytes: it stays
 * on the chain, its size made -R4 (in use). Failing that, a second walk
 * takes the first block with more: the record is cut from its end, the
 * block keeping the rest (its size lowered), and the record's size word
 * is R4. A block with less than 4 bytes to spare is taken whole instead,
 * its size negated (decoded from the ROM bytes and not reached in the
 * traces so far). R4, R3 and the walk's start are kept on the R10 stack.
 * The clearing is STB of 0 under LOOP. Two compares of R3 with the pool
 * heads change nothing. The routine runs under CACHE (the code cache only;
 * nothing the C has to do).
 * Entry: R3 = the pool's chain head; R4 = bytes; R10 = stack; R11 =
 * return address. Exit: R1 = the record or 0; R3, R4 as on entry; with a
 * record: R0 = 4 | the old +$1C; R2 = that old word; R12 = 0 (when there
 * was anything to clear); R13 = $89CB; OV, S, Z from the OR; the last RAM
 * address the record + $1C. Without: S, Z from R1 (set), the last RAM
 * address R10.
 */
FxRegs FxPoolTake(FxRegs g, int copy)
{
    gsu_codes(FxPoolTake_code[copy], 2);
    uint16_t *sp = &g.r[10];

    gsu_push(sp, g.r[4]);
    gsu_codes(FxPoolTake_code[copy] + 2, 1);
    gsu_push(sp, g.r[3]);
    g.r[1] = g.r[4];
    gsu_codes(FxPoolTake_code[copy] + 3, 3);
    gsu_push(sp, g.r[3]);
    gsu_codes(FxPoolTake_code[copy] + 6, 1);
    for (;;) {                                        gsu_codes(FxPoolTake_code[copy] + 7, 1); /* an exact fit */
        g.r[2] = gsu_ldw(g.r[3]);
        g.r[3] = gsu_inc(gsu_inc(g.r[3]));
        gsu_codes(FxPoolTake_code[copy] + 8, 1);
        g.r[0] = gsu_sub(gsu_ldw(g.r[3]), g.r[1], 0);
        if (gsu.z) {
            gsu_codes(FxPoolTake_code[copy] + 9, 2);
            g.r[0] = gsu_pull(sp);
            g.r[1] = gsu_inc(gsu_not(g.r[1]));
            g.r[0] = gsu_sub(g.r[0], g.r[0], 0);
            goto mark;
        } else { gsu_codes(FxPoolTake_code[copy] + 11, 1); }
        g.r[3] = g.r[2];
        gsu_moves(g.r[3]);
        if (gsu.z)
            { gsu_codes(FxPoolTake_code[copy] + 12, 1); break; }
    }
    g.r[3] = gsu_pull(sp);
    gsu_codes(FxPoolTake_code[copy] + 13, 1);
    for (;;) {                                        gsu_codes(FxPoolTake_code[copy] + 14, 1); /* a bigger block */
        g.r[2] = gsu_ldw(g.r[3]);
        g.r[3] = gsu_inc(gsu_inc(g.r[3]));
        gsu_codes(FxPoolTake_code[copy] + 15, 1);
        g.r[0] = gsu_sub(gsu_ldw(g.r[3]), g.r[1], 0);
        if (!gsu.s) {
            gsu_codes(FxPoolTake_code[copy] + 16, 3);
            g.r[0] = gsu_sub(g.r[0], 4, 0);
            if (gsu.s) {                              /* too little to split */
                g.r[0] = gsu_add(g.r[0], 4, 0);
                g.r[1] = gsu_add(g.r[1], g.r[0], 0);
                g.r[1] = gsu_inc(gsu_not(g.r[1]));
                g.r[0] = 0xFFFC;
            }
            g.r[0] = gsu_add(g.r[0], 4, 0);
            gsu_stw(g.r[3], g.r[0]);
            goto mark;
        } else { gsu_codes(FxPoolTake_code[copy] + 19, 2); }
        g.r[3] = g.r[2];
        gsu_moves(g.r[3]);
        if (gsu.z)
            { gsu_codes(FxPoolTake_code[copy] + 21, 1); break; } else { gsu_codes(FxPoolTake_code[copy] + 22, 1); }
    }
    g.r[1] = 0;
    goto done;
mark:
    gsu_codes(FxPoolTake_code[copy] + 23, 2);
    g.r[2] = gsu_add(g.r[3], g.r[0], 0);
    gsu_stw(g.r[2], g.r[1]);
    g.r[2] = gsu_dec(gsu_dec(g.r[2]));
    g.r[1] = g.r[2];
gsu_codes(FxPoolTake_code[copy] + 25, 1);
done:
    gsu_codes(FxPoolTake_code[copy] + 26, 1);
    g.r[3] = gsu_pull(sp);
    gsu_codes(FxPoolTake_code[copy] + 27, 1);
    g.r[4] = gsu_pull(sp);
    gsu_moves(g.r[1]);
    if (gsu.z)
        { gsu_codes(FxPoolTake_code[copy] + 28, 2); return g; } else { gsu_codes(FxPoolTake_code[copy] + 30, 1); }
    g.r[0] = FX_POOL_FIRST;
    gsu_sub(g.r[0], g.r[3], 0);
    g.r[0] = FX_POOL_SECOND;
    gsu_sub(g.r[0], g.r[3], 0);
    g.r[2] = gsu_add(2, g.r[1], 0);
    g.r[0] = gsu_ldw(g.r[2]);
    g.r[2] = gsu_inc(gsu_inc(g.r[2]));
    gsu_moves(g.r[0]);
    if (gsu.s)
        { gsu_codes(FxPoolTake_code[copy] + 31, 1); g.r[0] = gsu_inc(gsu_not(g.r[0])); } else { gsu_codes(FxPoolTake_code[copy] + 32, 2); }
    gsu_codes(FxPoolTake_code[copy] + 34, 2);
    g.r[0] = gsu_sub(g.r[0], 4, 0);
    if (!gsu.z && !gsu.s) {
        g.r[12] = g.r[0];
        g.r[0] = gsu_sub(g.r[0], g.r[0], 0);
        g.r[13] = 0x89CB;
        do {
            gsu_codes(FxPoolTake_code[copy] + 36, 1);
            gsu_stb(g.r[2], (uint8_t)g.r[0]);
            g.r[2] = gsu_inc(g.r[2]);
        } while (gsu_codes_loop(FxPoolTake_code[copy] + 37, 1, gsu_count_down(&g.r[12])));
        gsu_codes(FxPoolTake_code[copy] + 38, 1);
    }
    g.r[0] = gsu_add(REC_POOL, g.r[1], 0);
    gsu_stw(g.r[0], g.r[3]);
    gsu_codes(FxPoolTake_code[copy] + 39, 1);
    g.r[2] = gsu_ldw(FX_RECORDS);
    g.r[0] = gsu_add(REC_ALL_NEXT, g.r[1], 0);
    gsu_codes(FxPoolTake_code[copy] + 40, 1);
    gsu_stw(g.r[0], g.r[2]);
    gsu_codes(FxPoolTake_code[copy] + 41, 1);
    gsu_stw(FX_RECORDS, g.r[1]);
    gsu_moves(g.r[2]);
    if (!gsu.z) {
        gsu_codes(FxPoolTake_code[copy] + 42, 1);
        g.r[0] = gsu_add(REC_ALL_PREV, g.r[2], 0);
        gsu_stw(g.r[0], g.r[1]);
    } else { gsu_codes(FxPoolTake_code[copy] + 43, 2); }
    gsu_codes(FxPoolTake_code[copy] + 45, 1);
    g.r[2] = gsu_ldw(gsu_add(REC_FLAGS1C, g.r[1], 0));
    g.r[0] = gsu_or(0x0004, g.r[2]);
    gsu_codes(FxPoolTake_code[copy] + 46, 1);
    gsu_sbk(g.r[0]);
    gsu_codes(FxPoolTake_code[copy] + 47, 2);
    return g;
}

void abi_FxPoolTake(void)
{
    FxRegs g;

    memcpy(g.r, gsu.r, sizeof g.r);
    g = FxPoolTake(g, GSU_COPY(4));
    memcpy(gsu.r, g.r, 15 * sizeof g.r[0]);
}

#define FX_NEW_RECORD 0x238Eu   /* the new record, for the S-CPU (0: none) */
#define FX_COUNT_422 0x2482u    /* the number of mode $0422 records */
#define FX_LIST_422 0x2398u     /* their list, linked back through +$26 */
#define REC_FLAGS16 0x16u
#define REC_MODE 0x1Au
#define REC_LINK_422 0x26u
#define REC_MASS 0x2Au

/* cyccopy: the copy each of the caller's copies calls */
static const uint8_t FxPoolTake_from_record_new[6] = {2, 2, 2, 3, 8, 8};

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode record_new_code[6][5] = {
    {{0x04893A, 3}, {0x048B65, 13}, {0x0489F8, 2}, {0x048B72, 5}, {0x048B77, 9}},
    {{0, 0}, {0, 0}, {0x0489F8, 2}, {0x048CA3, 5}, {0, 0}},
    {{0, 0}, {0, 0}, {0x0489F8, 2}, {0x048C18, 5}, {0, 0}},
    {{0, 0}, {0, 0}, {0x0489F8, 2}, {0x048B41, 5}, {0x048B46, 4}},
    {{0, 0}, {0, 0}, {0x0489F8, 2}, {0x048C49, 5}, {0x048C4E, 4}},
    {{0x04893A, 3}, {0x048C6D, 13}, {0x0489F8, 2}, {0x048C7A, 5}, {0x048C7F, 4}},
};

/* The start of each program below: R10 = the stack, a record of `bytes`
   from the pool `pool` (FxPoolTake, R11 = `ret`), its address left at
   $238E for the S-CPU. Returns whether there is one. */
static int record_new(FxRegs *g, uint16_t bytes, uint16_t pool, uint16_t ret, int copy)
{
    gsu_codes(record_new_code[copy], 2);
    g->r[10] = FX_STACK;
    g->r[4] = bytes;
    g->r[3] = pool;
    g->r[11] = ret;
    *g = FxPoolTake(*g, GSU_COPY(FxPoolTake_from_record_new[copy]));
    gsu_codes(record_new_code[copy] + 2, 2);
    gsu_stw(FX_NEW_RECORD, g->r[1]);
    gsu_moves(g->r[1]);
    gsu_codes(record_new_code[copy] + 4, 1);
    return !gsu.z;
}

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode record_set_code[3][2] = {
    {{0x048BA4, 8}, {0, 0}},
    {{0, 0}, {0, 0}},
    {{0x048C83, 9}, {0x048C8C, 8}},
};

/* Field `field` of the new record R1 = `value`, through R2 and R0. */
static void record_set(FxRegs *g, uint16_t field, uint16_t value, int copy)
{
    gsu_codes(record_set_code[copy], 1);
    g->r[2] = value;
    g->r[0] = gsu_add(field, g->r[1], 0);
    gsu_stw(g->r[0], g->r[2]);
    gsu_codes(record_set_code[copy] + 1, 1);
}

/* cyccopy: the copy each of the caller's copies calls */
static const uint8_t record_new_from_record_new_first[4] = {1, 2, 3, 4};

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode record_new_first_code[4][6] = {
    {{0x04893A, 3}, {0x048C96, 13}, {0x048CA8, 13}, {0x048CB5, 8}, {0x048CBD, 8}, {0x048B62, 1}},
    {{0x04893A, 3}, {0x048C0B, 13}, {0x048C1D, 13}, {0x048C2A, 8}, {0x048C32, 8}, {0x048B62, 1}},
    {{0x04893A, 3}, {0x048B34, 13}, {0x048B4A, 9}, {0x048B53, 8}, {0x048B5B, 8}, {0x048B62, 1}},
    {{0x04893A, 3}, {0x048C3C, 13}, {0x048C52, 9}, {0x048C5B, 8}, {0x048C63, 8}, {0x048C6A, 1}},
};

/* The first pool's programs: a $34-byte record with its mass (+$2A), flags
   (+$16) and mode (+$1A). */
static FxRegs record_new_first(FxRegs g, uint16_t ret, uint16_t mass, uint16_t flags, uint16_t mode, int copy)
{
    gsu_codes(record_new_first_code[copy], 2);
    if (record_new(&g, 0x0034, FX_POOL_FIRST, ret, GSU_COPY(record_new_from_record_new_first[copy]))) {
        gsu_codes(record_new_first_code[copy] + 2, 1);
        record_set(&g, REC_MASS, mass, GSU_COPY(1));
        gsu_codes(record_new_first_code[copy] + 3, 1);
        record_set(&g, REC_FLAGS16, flags, GSU_COPY(1));
        gsu_codes(record_new_first_code[copy] + 4, 1);
        record_set(&g, REC_MODE, mode, GSU_COPY(1));
    } else { gsu_codes(record_new_first_code[copy] + 5, 1); }
    return g;
}

#define ABI_REGS(call)                                   \
    do {                                                 \
        FxRegs g;                                        \
        memcpy(g.r, gsu.r, sizeof g.r);                  \
        g = call;                                        \
        memcpy(gsu.r, g.r, 15 * sizeof g.r[0]);          \
    } while (0)

/*
 * @routine 04:8B33 FxRecordNew426
 * WHAT IT DOES: A program: a new $34-byte record from the first pool with
 * mass +$2A = $40, flags +$16 = 8 and mode +$1A = $0426; its address (or
 * 0) to $238E. The S-CPU side is GsuRecordNew426 (object/gsu_record.c).
 * HOW: FxPoolTake with R10 = $0240, then three STWs through R2 and R0.
 * Entry: the S-CPU starts it. Exit: (at its STOP) R1 = the record; R0 = R1
 * + $1A; R2 = $0426; R11 = $8B41; OV, S, CY, Z from the last ADD; the last
 * RAM address R1 + $1A. No record: R1 = 0; Z set from R1.
 */
FxRegs FxRecordNew426(FxRegs g)
{
    return record_new_first(g, 0x8B41, 0x0040, 0x0008, 0x0426, GSU_COPY(2));
}

void abi_FxRecordNew426(void)
{
    ABI_REGS(FxRecordNew426(g));
}

/*
 * @routine 04:8C0A FxRecordNew424
 * WHAT IT DOES: A program: as FxRecordNew426 with mode $0424. The S-CPU
 * side is GsuRecordNew424 (object/gsu_record.c).
 * HOW: As FxRecordNew426.
 * Entry: the S-CPU starts it. Exit: (at its STOP) as FxRecordNew426; R2 =
 * $0424; R11 = $8C18.
 */
FxRegs FxRecordNew424(FxRegs g)
{
    return record_new_first(g, 0x8C18, 0x0040, 0x0008, 0x0424, GSU_COPY(1));
}

void abi_FxRecordNew424(void)
{
    ABI_REGS(FxRecordNew424(g));
}

/*
 * @routine 04:8C3B FxRecordNew426B
 * WHAT IT DOES: A program: as FxRecordNew426 with flags $20. The S-CPU
 * side is GsuRecordNew424B (object/gsu_record.c).
 * HOW: As FxRecordNew426.
 * Entry: the S-CPU starts it. Exit: (at its STOP) as FxRecordNew426; R2 =
 * $0426; R11 = $8C49.
 */
FxRegs FxRecordNew426B(FxRegs g)
{
    return record_new_first(g, 0x8C49, 0x0040, 0x0020, 0x0426, GSU_COPY(3));
}

void abi_FxRecordNew426B(void)
{
    ABI_REGS(FxRecordNew426B(g));
}

/*
 * @routine 04:8C6C FxRecordNewCamera
 * WHAT IT DOES: A program: a new $34-byte record from the first pool with
 * flags +$16 = $20 and mode +$1A = $042E, its mass left as the pool had
 * it; its address (or 0) to $238E. The S-CPU side is GsuRecordNewCamera
 * (object/gsu_record.c), the record of a replay camera's follower.
 * HOW: FxPoolTake with R10 = $0240, then two STWs through R2 and R0.
 * Entry: the S-CPU starts it. Exit: (at its STOP) R1 = the record; R0 = R1
 * + $1A; R2 = $042E; R11 = $8C7A; OV, S, CY, Z from the last ADD; the last
 * RAM address R1 + $1A. No record: R1 = 0; Z set from R1.
 */
FxRegs FxRecordNewCamera(FxRegs g)
{
    if (record_new(&g, 0x0034, FX_POOL_FIRST, 0x8C7A, GSU_COPY(5))) {
        record_set(&g, REC_FLAGS16, 0x0020, GSU_COPY(2));
        record_set(&g, REC_MODE, 0x042E, GSU_COPY(1));
    } else { gsu_code(0x048C93, 1); }
    return g;
}

void abi_FxRecordNewCamera(void)
{
    ABI_REGS(FxRecordNewCamera(g));
}

/*
 * @routine 04:8C95 FxRecordNew42D
 * WHAT IT DOES: A program: as FxRecordNew426 with mass 0, flags $20 and
 * mode $042D. The S-CPU side is GsuRecordNew42D (object/gsu_record.c).
 * HOW: As FxRecordNew426.
 * Entry: the S-CPU starts it. Exit: (at its STOP) as FxRecordNew426; R2 =
 * $042D; R11 = $8CA3.
 */
FxRegs FxRecordNew42D(FxRegs g)
{
    return record_new_first(g, 0x8CA3, 0x0000, 0x0020, 0x042D, GSU_COPY(0));
}

void abi_FxRecordNew42D(void)
{
    ABI_REGS(FxRecordNew42D(g));
}

/*
 * @routine 04:8B64 FxRecordNew422
 * WHAT IT DOES: A program: a new $2A-byte record from the second pool with
 * mode +$1A = $0422 and bits $0181 set in its flags +$16; it is counted at
 * $2482 and put at the head of the list at $2398 (the old head's +$26
 * pointing to it); its address (or 0) to $238E. The S-CPU side is
 * GsuRecordNew422 (object/gsu_record.c).
 * HOW: FxPoolTake with R10 = $0240; the count is LM, INC, SBK; the flags
 * an LDW, OR and STW.
 * Entry: the S-CPU starts it. Exit: (at its STOP) R1 = the record; R0 = R1
 * + $1A; R2 = $0422; R11 = $8B72; OV, S, CY, Z from the last ADD; the last
 * RAM address R1 + $1A. No record: R1 = 0; Z set from R1.
 */
FxRegs FxRecordNew422(FxRegs g)
{
    if (!record_new(&g, 0x002A, FX_POOL_SECOND, 0x8B72, GSU_COPY(0)))
        return g;
    g.r[0] = gsu_inc(gsu_ldw(FX_COUNT_422));
    gsu_code(0x048B80, 2);
    gsu_sbk(g.r[0]);
    gsu_code(0x048B82, 4);
    g.r[2] = gsu_ldw(FX_LIST_422);
    gsu_moves(g.r[2]);
    if (!gsu.z) {
        gsu_code(0x048B86, 10);
        g.r[0] = gsu_add(REC_LINK_422, g.r[2], 0);
        gsu_stw(g.r[0], g.r[1]);
    } else { gsu_code(0x048B86, 4); gsu_code(0x048B8F, 1); }
    gsu_code(0x048B90, 4);
    gsu_stw(FX_LIST_422, g.r[1]);
    gsu_code(0x048B94, 6);
    g.r[2] = gsu_ldw(gsu_add(REC_FLAGS16, g.r[1], 0));
    g.r[0] = 0x0181;
    g.r[2] = gsu_or(g.r[0], g.r[2]);
    g.r[0] = gsu_add(REC_FLAGS16, g.r[1], 0);
    gsu_code(0x048B9A, 10);
    gsu_stw(g.r[0], g.r[2]);
    record_set(&g, REC_MODE, 0x0422, GSU_COPY(0));
    return g;
}

void abi_FxRecordNew422(void)
{
    ABI_REGS(FxRecordNew422(g));
}

void abi_FxListClearBit0(void);
void abi_FxObjectFreeChildren(void);

/*
 * @routine 04:8A08 FxObjectFree
 * WHAT IT DOES: Frees Super FX object R1: sweeps its list when its word
 * +$18 is set (FxListClearBit0, FxListSweep), frees its children when bit
 * 1 of +$16 is set (FxObjectFreeChildren), takes it out of the used list
 * (previous +$06, next +$04; the head at $2332), then, when its word +$02
 * is negative, negates it, otherwise pushes the object on the free list
 * its +$08 points at (+$00 = the old first).
 * HOW: The return address (and R8 around the sweep) on the R10 stack.
 * Two compares of +$08 with $1098 and $1D70 branch to the next
 * instruction either way. The children are not freed in the traces so
 * far.
 * Entry: R1 = the object; R10 = stack; R11 = return address. Exit: R1 as
 * on entry; R2 = the object + 2 (R3 = +$08) or, negated, the object + 2
 * with R0 its new +$02; CY from the $1D70 compare; S, Z from R10; the
 * last RAM address R10.
 */
void abi_FxObjectFree(void)
{
    gsu_code(0x048A09, 2);
    gsu_push(&gsu.r[10], gsu.r[11]);
    gsu.r[0] = gsu_add(0x0018, gsu.r[1], 0);
    gsu_code(0x048A0B, 6);
    gsu.r[0] = gsu_ldw(gsu.r[0]);
    gsu_moves(gsu.r[0]);
    if (!gsu.z) {
        gsu_code(0x048A11, 7);
        gsu_push(&gsu.r[10], gsu.r[8]);
        gsu.r[6] = gsu.r[1];
        gsu.r[11] = 0x8A20;
        abi_FxListClearBit0();
        gsu.r[11] = 0x8A25;
        abi_FxListSweep();
        gsu.r[8] = gsu_pull(&gsu.r[10]);
    } else { gsu_code(0x048A11, 4); gsu_code(0x048A29, 1); }
    gsu_code(0x048A2A, 4);
    gsu.r[0] = gsu_add(0x0016, gsu.r[1], 0);
    gsu.r[0] = gsu_ldw(gsu.r[0]);
    gsu.r[2] = 0x0002;
    gsu.r[0] = gsu_and(gsu.r[0], gsu.r[2]);
    if (!gsu.z) {
        gsu_code(0x048A2E, 11); gsu_code(0x048AA2, 3);
        gsu.r[11] = 0x8A39;
        abi_FxObjectFreeChildren();
    } else { gsu_code(0x048A2E, 6); }
    gsu_code(0x048A39, 7);
    gsu.r[2] = gsu_add(0x0006, gsu.r[1], 0);
    gsu.r[2] = gsu_ldw(gsu.r[2]);
    gsu.r[3] = gsu_add(0x0004, gsu.r[1], 0);
    gsu_code(0x048A40, 6);
    gsu.r[3] = gsu_ldw(gsu.r[3]);
    gsu_moves(gsu.r[2]);
    if (!gsu.z) {
        gsu_code(0x048A46, 10);
        gsu.r[0] = gsu_add(0x0004, gsu.r[2], 0);
        gsu_stw(gsu.r[0], gsu.r[3]);
    } else
        gsu_stw(0x2332, gsu.r[3]);
    gsu_code(0x048A50, 2); gsu_code(0x048A56, 11);
    gsu_moves(gsu.r[3]);
    if (!gsu.z) {
        gsu.r[0] = gsu_add(0x0006, gsu.r[3], 0);
        gsu_stw(gsu.r[0], gsu.r[2]);
    }
    gsu_code(0x048A61, 6);
    gsu.r[3] = gsu_add(0x0008, gsu.r[1], 0);
    gsu.r[3] = gsu_ldw(gsu.r[3]);
    gsu.r[1] = gsu_inc(gsu_inc(gsu.r[1]));
    gsu_code(0x048A67, 3);
    gsu.r[0] = gsu_ldw(gsu.r[1]);
    gsu.r[2] = gsu.r[0];
    gsu_moves(gsu.r[2]);
    if (gsu.s)
        gsu.r[2] = gsu_inc(gsu_not(gsu.r[2])); else { gsu_code(0x048A6A, 4); gsu_code(0x048A71, 25); }
    gsu.r[0] = 0x1098;
    gsu_sub(gsu.r[0], gsu.r[3], 0);
    gsu.r[0] = 0x1D70;
    gsu_sub(gsu.r[0], gsu.r[3], 0);
    gsu.r[1] = gsu_dec(gsu_dec(gsu.r[1]));
    gsu.r[3] = gsu_add(0x0008, gsu.r[1], 0);
    gsu.r[3] = gsu_ldw(gsu.r[3]);
    gsu.r[2] = gsu_inc(gsu_inc(gsu.r[1]));
    gsu_code(0x048A8A, 5);
    gsu.r[0] = gsu_ldw(gsu.r[2]);
    gsu_moves(gsu.r[0]);
    if (gsu.s) {
        gsu.r[0] = gsu_inc(gsu_not(gsu.r[0]));
        gsu_stw(gsu.r[2], gsu.r[0]);
    } else {
        gsu_code(0x048A8F, 4); gsu_code(0x048A99, 2);
        gsu.r[0] = gsu_ldw(gsu.r[3]);
        gsu_code(0x048A9B, 1);
        gsu_stw(gsu.r[1], gsu.r[0]);
        gsu_code(0x048A9C, 2);
        gsu_stw(gsu.r[3], gsu.r[1]);
    }
    gsu_code(0x048A9E, 4);
    gsu_pull(&gsu.r[10]);
}

/*
 * @routine 04:8AA2 FxObjectFreeChildren
 * WHAT IT DOES: Frees each Super FX object linked from object R1's +$3E
 * (through each one's +$34) with FxObjectFree, which frees their own
 * children in turn.
 * HOW: The next link is read before each free; the return address and R1
 * are kept on the R10 stack. Not reached in the traces so far.
 * Entry: R1 = the object; R10 = stack; R11 = return address. Exit: R1 as
 * on entry; R4 = 0; R11 = $8AC2 when any was freed; S, Z from R10; the
 * last RAM address R10.
 */
void abi_FxObjectFreeChildren(void)
{
    gsu_push(&gsu.r[10], gsu.r[11]);
    gsu_code(0x048AA5, 4);
    gsu_push(&gsu.r[10], gsu.r[1]);
    gsu.r[4] = gsu_add(0x003E, gsu.r[1], 0);
    gsu_code(0x048AA9, 8);
    gsu.r[4] = gsu_ldw(gsu.r[4]);
    for (;;) {
        gsu_code(0x048AB1, 4);
        gsu_moves(gsu.r[4]);
        if (gsu.z)
            { gsu_code(0x048AC7, 5); break; } else { gsu_code(0x048AB5, 9); }
        gsu.r[1] = gsu.r[4];
        gsu.r[4] = gsu_add(0x0034, gsu.r[1], 0);
        gsu.r[4] = gsu_ldw(gsu.r[4]);
        gsu.r[11] = 0x8AC2;
        (gsu_code(0x048A08, 1), gsu_code(0x048ABE, 4), abi_FxObjectFree());
        gsu.r[1] = gsu.r[4];
        gsu_code(0x048AB0, 1); gsu_code(0x048AC2, 5);
    }
    gsu.r[1] = gsu_pull(&gsu.r[10]);
    gsu_code(0x048ACC, 4);
    gsu_pull(&gsu.r[10]);
}
