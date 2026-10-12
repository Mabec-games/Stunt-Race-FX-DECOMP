/*
 * fx_car.c: the Super FX sets up a car's record from a script in ROM.
 *
 * The script is read through the ROM buffer from R14 (in ROMBR's bank): a
 * byte giving the number of parts, a word for the parts' mode, seven words
 * per part, then a byte choosing one of the 5-byte entries that follow.
 * Each part is a $52-byte record of its own on the car's list of parts:
 * the car's +$3E is the first part, a part's +$34 the next.
 */
#include <string.h>
#include "gsu_ops.h"
#include "fx_calls.h"

#define FX_POOL_FIRST 0x1098u
#define FX_PART_MODE 0x0068u     /* the parts' mode word from the script */
#define FX_SCRIPT_AT 0x2392u     /* where the entries start, from $8090 */
#define FX_CAR_26F2 0x26F2u

#define PART_BYTES 0x0052u

/* A word of the script (GETB, INC R14, GETBH, INC R14). */
static uint16_t script_word(uint16_t *r14)
{
    uint16_t w = gsu_getb();

    gsu_r14(gsu_inc(*r14));
    *r14 = gsu.r[14];
    w = gsu_getbh(w);
    gsu_r14(gsu_inc(*r14));
    *r14 = gsu.r[14];
    return w;
}

/* Field `field` of record `rec` = R3, through R0. */
static void field_set(FxRegs *g, uint16_t field, uint16_t rec)
{
    g->r[0] = gsu_add(field, rec, 0);
    gsu_stw(g->r[0], g->r[3]);
}

/*
 * @routine 04:8E84 FxCarAddPart
 * WHAT IT DOES: Adds a part to car R1: a new $52-byte record from the first
 * pool whose +$12, +$3C (negated), +$3E, +$40, +$44, +$50 and +$46 are the
 * next seven script words; its +$0C, +$0E, +$10 = $14, bit 2 of +$16 set,
 * +$1A = the parts' mode ($0068), +$2A = the car's +$2A; it goes on the
 * front of the car's parts list. With no room the script is not read
 * (decoded from the ROM bytes and not reached in the traces so far).
 * HOW: FxPoolTake; each field is STW through R0 = field + R2 with R3 the
 * value. R1 and the return address are kept on the R10 stack.
 * Entry: R1 = the car's record; R14 = the script; R10 = stack; R11 =
 * return address. Exit: R1 = the car; R2 = the part (0 without); R3 = the
 * car's +$2A; R0 = the part + $2A; R14 past the seven words; R11 = $8E97;
 * R4, R12, R13 as FxPoolTake leaves them; S, Z from R10, OV, CY from the
 * last ADD; the last RAM address R10.
 */
FxRegs FxCarAddPart(FxRegs g)
{
    static const uint8_t fields[7] = {0x12, 0x3C, 0x3E, 0x40, 0x44, 0x50, 0x46};
    uint16_t *sp = &g.r[10];
    int i;

    gsu_push(sp, g.r[11]);
    gsu_push(sp, g.r[1]);
    g.r[4] = PART_BYTES;
    g.r[3] = FX_POOL_FIRST;
    g.r[11] = 0x8E97;
    g = FxPoolTake(g);
    g.r[2] = g.r[1];
    g.r[1] = gsu_pull(sp);
    gsu_moves(g.r[2]);
    if (gsu.z) {
        gsu_pull(sp);
        return g;
    }
    for (i = 0; i < 7; i++) {
        g.r[3] = script_word(&g.r[14]);
        if (fields[i] == 0x3C)
            g.r[3] = gsu_inc(gsu_not(g.r[3]));
        field_set(&g, fields[i], g.r[2]);
    }
    g.r[3] = gsu_ldw(gsu_add(0x3E, g.r[1], 0));     /* the car's first part */
    field_set(&g, 0x34, g.r[2]);
    g.r[0] = gsu_add(0x3E, g.r[1], 0);
    gsu_stw(g.r[0], g.r[2]);
    g.r[3] = 0x0014;
    field_set(&g, 0x0C, g.r[2]);
    field_set(&g, 0x0E, g.r[2]);
    field_set(&g, 0x10, g.r[2]);
    g.r[3] = gsu_ldw(gsu_add(0x16, g.r[2], 0));
    g.r[0] = 0x0004;
    g.r[3] = gsu_or(g.r[0], g.r[3]);
    field_set(&g, 0x16, g.r[2]);
    g.r[3] = gsu_ldw(FX_PART_MODE);
    field_set(&g, 0x1A, g.r[2]);
    g.r[3] = gsu_ldw(gsu_add(0x2A, g.r[1], 0));
    field_set(&g, 0x2A, g.r[2]);
    gsu_pull(sp);
    return g;
}

void abi_FxCarAddPart(void)
{
    FxRegs g;

    memcpy(g.r, gsu.r, sizeof g.r);
    g = FxCarAddPart(g);
    memcpy(gsu.r, g.r, 15 * sizeof g.r[0]);
}

/*
 * @routine 04:8F48 FxCarFromScript
 * WHAT IT DOES: Sets up car R1 from the script at R14: its parts
 * (FxCarAddPart for each), $2392 = where the entries start less $8090, +$6E
 * = the entry the next byte chooses (5 bytes each), and +$6A = $0180, +$8C
 * = $1000, +$8A = the word at $26F2, +$7A = 0 (laps), +$86 = 2 and +$58 = 1
 * (segments).
 * HOW: The part count goes to R12 for LOOP, with R13 and R12 kept on the
 * R10 stack around each call; the entry is LMULT of the byte by 5 added to
 * R14. The return address is kept on the R10 stack.
 * Entry: R1 = the car's record; R14 = the script; R10 = stack; R11 =
 * return address. Exit: R14 = R2 = the chosen entry; R0 = R1 + $58; R4 =
 * the byte times 5; R6 = 5; R11 = $8E97 (with parts); R12 = 0; R13 =
 * $8F61; S, Z from R10, OV, CY from the last ADD; the last RAM address R10.
 */
FxRegs FxCarFromScript(FxRegs g)
{
    static const struct { uint8_t field; uint16_t value; } set[6] = {
        {0x6A, 0x0180}, {0x8C, 0x1000}, {0x8A, 0}, {0x7A, 0x0000}, {0x86, 0x0002}, {0x58, 0x0001},
    };
    uint16_t *sp = &g.r[10];
    uint32_t p;
    int i;

    gsu_push(sp, g.r[11]);
    g.r[0] = gsu_sub(g.r[0], g.r[0], 0);
    g.r[0] = gsu_getb();
    gsu_r14(gsu_inc(g.r[14]));
    g.r[14] = gsu.r[14];
    g.r[2] = script_word(&g.r[14]);
    gsu_stw(FX_PART_MODE, g.r[2]);
    g.r[12] = g.r[0];
    gsu_moves(g.r[12]);
    if (!gsu.z) {
        g.r[13] = 0x8F61;
        do {
            gsu_push(sp, g.r[13]);
            gsu_push(sp, g.r[12]);
            g.r[11] = 0x8F6E;
            g = FxCarAddPart(g);
            g.r[12] = gsu_pull(sp);
            g.r[13] = gsu_pull(sp);
        } while (gsu_count_down(&g.r[12]));
    }
    g.r[0] = 0x8090;
    g.r[0] = gsu_sub(g.r[14], g.r[0], 0);
    gsu_stw(FX_SCRIPT_AT, g.r[0]);
    g.r[0] = gsu_sub(g.r[0], g.r[0], 0);
    g.r[0] = gsu_getb();
    gsu_r14(gsu_inc(g.r[14]));
    g.r[14] = gsu.r[14];
    g.r[6] = 5;
    p = gsu_lmult(g.r[0], g.r[6]);
    g.r[0] = (uint16_t)(p >> 16);
    g.r[4] = (uint16_t)p;
    gsu_r14(gsu_add(g.r[14], g.r[4], 0));
    g.r[14] = gsu.r[14];
    g.r[2] = g.r[14];
    g.r[0] = gsu_add(0x6E, g.r[1], 0);
    gsu_stw(g.r[0], g.r[2]);
    for (i = 0; i < 6; i++) {
        g.r[2] = set[i].field == 0x8A ? gsu_ldw(FX_CAR_26F2) : set[i].value;
        g.r[0] = gsu_add(set[i].field, g.r[1], 0);
        gsu_stw(g.r[0], g.r[2]);
    }
    gsu_pull(sp);
    return g;
}

void abi_FxCarFromScript(void)
{
    FxRegs g;

    memcpy(g.r, gsu.r, sizeof g.r);
    g = FxCarFromScript(g);
    memcpy(gsu.r, g.r, 15 * sizeof g.r[0]);
}
