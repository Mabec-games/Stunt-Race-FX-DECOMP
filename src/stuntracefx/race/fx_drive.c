/*
 * fx_drive.c: the Super FX drives a computer car.
 *
 * The decision goes to the car's +$42 as bits: 0 steer one way, 1 the
 * other, 2 and 3 their harder forms, 4 accelerate, 5 a reversing turn, 6
 * brake, 10 a boost. +$7C counts down a period of not accelerating (set
 * when cars are pushed apart, race/fx_traffic.c); +$7E counts how long the
 * car has been stuck.
 */
#include <string.h>
#include "gsu_ops.h"
#include "fx_calls.h"

#define CAR_PITCH 0x24u
#define CAR_YAW 0x26u
#define CAR_STEER_BIAS 0x3Au
#define CAR_FLAGS40 0x40u
#define CAR_SIDES 0x42u
#define CAR_44 0x44u
#define CAR_SPEED 0x4Au
#define CAR_70 0x70u
#define CAR_SPREAD 0x7Cu
#define CAR_STUCK 0x7Eu

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode bits_and_code[4][1] = {
    {{0, 0}},
    {{0x04C84C, 36}},
    {{0x04C886, 23}},
    {{0x04C8E2, 26}},
};

/* R9 = R0 op R9 (TO R9 with the operation). */
static void bits_and(FxRegs *g, uint16_t v, int copy) { gsu_codes(bits_and_code[copy], 1); g->r[0] = v; g->r[9] = gsu_and(g->r[0], g->r[9]); }
static void bits_or(FxRegs *g, uint16_t v, int copy) { g->r[0] = v; g->r[9] = gsu_or(g->r[0], g->r[9]); }

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode car_field_code[17][3] = {
    {{0, 0}, {0, 0}, {0, 0}},
    {{0, 0}, {0x04C802, 6}, {0, 0}},
    {{0, 0}, {0x04C901, 7}, {0, 0}},
    {{0, 0}, {0x04C916, 4}, {0x04C91B, 1}},
    {{0x04C7D8, 6}, {0x04C7DE, 6}, {0x04C7EC, 3}},
    {{0, 0}, {0x04C7FC, 6}, {0, 0}},
    {{0, 0}, {0x04C802, 11}, {0, 0}},
    {{0, 0}, {0x04C80D, 10}, {0, 0}},
    {{0x04C81B, 6}, {0x04C821, 23}, {0x04C845, 7}},
    {{0, 0}, {0x04C870, 14}, {0, 0}},
    {{0x04C8AB, 5}, {0x04C8B0, 17}, {0, 0}},
    {{0, 0}, {0x04C8C1, 4}, {0x04C8CB, 5}},
    {{0, 0}, {0x04C8D0, 12}, {0, 0}},
    {{0x04C8FC, 5}, {0x04C901, 12}, {0, 0}},
    {{0, 0}, {0x04C90D, 2}, {0, 0}},
    {{0x04C91B, 5}, {0x04C920, 15}, {0, 0}},
    {{0, 0}, {0x04C89D, 14}, {0, 0}},
};

/* The field `f` of car R8 (R0 = f + R8, then LDW). */
static uint16_t car_field(FxRegs *g, uint16_t f, int copy)
{
    gsu_codes(car_field_code[copy], 1);
    g->r[0] = gsu_add(f, g->r[8], 0);
    return gsu_codes_then(car_field_code[copy] + 1, 2, gsu_ldw(g->r[0]));
}

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode FxDriveDecide_code[2][56] = {
    {{0x04C7D8, 6}, {0x04C7E4, 8}, {0, 0}, {0, 0}, {0x04C7DE, 6}, {0x04C7EC, 3}, {0x04C7EF, 6}, {0x04CBE8, 3}, {0x04C7F5, 7}, {0x04C7FC, 6}, {0x04C808, 5}, {0x04C80D, 10}, {0x04C816, 1}, {0x04C817, 4}, {0x04C81B, 6}, {0x04C840, 5}, {0x04C827, 9}, {0, 0}, {0x04C838, 8}, {0x04C830, 8}, {0x04C821, 6}, {0x04C845, 7}, {0x04C84C, 13}, {0, 0}, {0x04C859, 8}, {0x04C866, 10}, {0x04C861, 5}, {0x04C870, 6}, {0x04C84C, 5}, {0x04C87E, 8}, {0, 0}, {0x04C886, 8}, {0x04C893, 10}, {0x04C88E, 5}, {0x04C89D, 6}, {0x04C8AB, 5}, {0x04C8B7, 5}, {0x04C8B0, 7}, {0x04C8BC, 5}, {0x04C8C1, 4}, {0, 0}, {0x04C8C5, 2}, {0x04C8C7, 4}, {0x04C8CB, 5}, {0x04C8D0, 12}, {0x04C8FC, 5}, {0x04C908, 5}, {0x04C90D, 2}, {0x04C911, 5}, {0x04C91A, 1}, {0, 0}, {0x04C91C, 4}, {0x04C924, 5}, {0x04C920, 4}, {0x04C929, 6}, {0x04C92F, 4}},
    {{0x04C7D8, 6}, {0x04C7E4, 8}, {0x04C7DE, 6}, {0x04C7EC, 3}, {0, 0}, {0, 0}, {0x04C7EF, 6}, {0x04CBE8, 3}, {0x04C7F5, 7}, {0x04C7FC, 6}, {0x04C808, 5}, {0x04C80D, 10}, {0x04C816, 1}, {0x04C817, 4}, {0x04C81B, 6}, {0x04C840, 5}, {0x04C821, 23}, {0x04C845, 7}, {0x04C838, 8}, {0, 0}, {0, 0}, {0, 0}, {0x04C84C, 21}, {0x04C866, 10}, {0, 0}, {0, 0}, {0x04C861, 5}, {0x04C870, 6}, {0x04C84C, 5}, {0x04C87E, 16}, {0x04C893, 10}, {0, 0}, {0, 0}, {0x04C88E, 5}, {0x04C89D, 6}, {0x04C8AB, 5}, {0x04C8B7, 5}, {0x04C8B0, 7}, {0x04C8BC, 5}, {0x04C8C1, 4}, {0x04C8CB, 5}, {0x04C8C5, 2}, {0x04C8C7, 4}, {0, 0}, {0x04C8D0, 12}, {0x04C8FC, 5}, {0x04C908, 5}, {0x04C90D, 2}, {0x04C911, 5}, {0x04C91A, 1}, {0x04C916, 4}, {0x04C91B, 5}, {0x04C924, 5}, {0x04C920, 4}, {0x04C929, 6}, {0x04C92F, 4}},
};

/*
 * @routine 04:C7D5 FxDriveDecide
 * WHAT IT DOES: Decides computer car R8's controls toward the point (R1,
 * R3) at speed R4 into +$42 (bits above). The heading error is the bearing
 * (FxBearingTo) less +$24, less +$3A: from -$38E to $38E nothing, beyond
 * it steer (bit 1 to the right, bit 0 to the left), and beyond $FA4 also
 * no accelerating; with bit 14 of +$40 the harder form too (decoded from
 * the ROM bytes and not reached in the traces so far). Bit 6 when at or
 * above R4, else bit 10 when R4 is $227 or more, else bit 4; bit 4 also
 * below speed 200. A running +$7C counts down and clears bits 4 and 10.
 * +$7E counts up from -40 to -20 for a reversing turn (bits 0, 1 swapped,
 * bit 5) once it has reached 20 while slower than 10, and down otherwise.
 * Bits 4 and 10 are cleared while +$44 is not 0. Bit 8 of +$40 takes a
 * quarter off +$26; bit 5 of +$70 makes R4 three quarters (decoded from
 * the ROM bytes and not reached in the traces so far).
 * HOW: R9 collects the bits; limits are CMP and the sign. R4 and the
 * return address are kept on the R10 stack.
 * Entry: R1, R3 = the point; R4 = the speed wanted; R8 = the car's record;
 * R10 = stack; R11 = return address. Exit: R9 = the bits; R4 as used; R1
 * the heading error or the last limit; the others from the tests and
 * FxBearingTo; S, Z from R10; the last RAM address R10.
 */
FxRegs FxDriveDecide(FxRegs g, int copy)
{
    gsu_code(0x04C7D6, 2);
    uint16_t *sp = &g.r[10];

    gsu_push(sp, g.r[11]);
    gsu_codes(FxDriveDecide_code[copy], 1);
    g.r[0] = car_field(&g, CAR_70, GSU_COPY(0));
    g.r[5] = 0x0020;
    g.r[0] = gsu_and(g.r[0], g.r[5]);
    if (!gsu.z) {
        gsu_codes(FxDriveDecide_code[copy] + 1, 1);
        g.r[4] = gsu_div2(g.r[4]);
        g.r[0] = gsu_div2(g.r[4]);
        g.r[4] = gsu_add(g.r[4], g.r[0], 0);
    } else { gsu_codes(FxDriveDecide_code[copy] + 2, 2); }
    gsu_codes(FxDriveDecide_code[copy] + 4, 2);
    gsu_push(sp, g.r[4]);
    g.r[11] = 0xC7F5;
    gsu_codes(FxDriveDecide_code[copy] + 6, 2);
    g = FxBearingTo(g, GSU_COPY(0));
    g.r[1] = g.r[0];
    gsu_codes(FxDriveDecide_code[copy] + 8, 1);
    g.r[0] = car_field(&g, CAR_PITCH, GSU_COPY(0));
    g.r[1] = gsu_sub(g.r[1], g.r[0], 0);
    gsu_codes(FxDriveDecide_code[copy] + 9, 1);
    g.r[0] = car_field(&g, CAR_FLAGS40, GSU_COPY(1));
    g.r[2] = 0x0100;
    g.r[0] = gsu_and(g.r[0], g.r[2]);
    if (!gsu.z) {
        gsu_codes(FxDriveDecide_code[copy] + 10, 1);
        g.r[0] = car_field(&g, CAR_YAW, GSU_COPY(0));
        g.r[2] = g.r[0];
        g.r[0] = gsu_div2(gsu_div2(g.r[0]));
        g.r[2] = gsu_sub(g.r[2], g.r[0], 0);
        gsu_codes(FxDriveDecide_code[copy] + 11, 1);
        gsu_sbk(g.r[2]);
    } else { gsu_codes(FxDriveDecide_code[copy] + 12, 1); }
    gsu_codes(FxDriveDecide_code[copy] + 13, 1);
    g.r[4] = gsu_pull(sp);
    g.r[9] = 0;
    gsu_codes(FxDriveDecide_code[copy] + 14, 1);
    g.r[0] = car_field(&g, CAR_SPEED, GSU_COPY(0));
    g.r[3] = g.r[4];
    gsu_sub(g.r[0], g.r[3], 0);
    if (!gsu.s) {
        gsu_codes(FxDriveDecide_code[copy] + 15, 1);
        bits_or(&g, 0x0040, GSU_COPY(4));
    } else {
        gsu_codes(FxDriveDecide_code[copy] + 16, 2);
        g.r[2] = 0x0227;
        gsu_sub(g.r[4], g.r[2], 0);
        bits_or(&g, gsu.s ? (gsu_codes(FxDriveDecide_code[copy] + 18, 1), 0x0010) : (gsu_codes(FxDriveDecide_code[copy] + 19, 1), 0x0400), GSU_COPY(0));
    }
    gsu_codes(FxDriveDecide_code[copy] + 20, 2);
    g.r[2] = gsu_ldw(gsu_add(CAR_STEER_BIAS, g.r[8], 0));
    g.r[0] = gsu_sub(g.r[1], g.r[2], 0);
    if (gsu.s) {
        gsu_codes(FxDriveDecide_code[copy] + 22, 2);
        g.r[1] = 0xFC72;
        gsu_sub(g.r[0], g.r[1], 0);
        if (gsu.s) {
            gsu_codes(FxDriveDecide_code[copy] + 24, 2);
            g.r[1] = 0xF05C;
            gsu_sub(g.r[0], g.r[1], 0);
            if (gsu.s)
                { gsu_codes(FxDriveDecide_code[copy] + 26, 1); bits_and(&g, 0xFFEF, GSU_COPY(0)); }
            bits_or(&g, 0x0002, GSU_COPY(3));
            g.r[0] = car_field(&g, CAR_FLAGS40, GSU_COPY(0));
            g.r[1] = 0x4000;
            g.r[0] = gsu_and(g.r[0], g.r[1]);
            if (!gsu.z)
                bits_or(&g, 0x0008, GSU_COPY(6)); else { gsu_codes(FxDriveDecide_code[copy] + 27, 1); }
        }
    } else {
        gsu_codes(FxDriveDecide_code[copy] + 28, 3);
        g.r[1] = 0x038E;
        gsu_sub(g.r[0], g.r[1], 0);
        if (!gsu.s) {
            gsu_codes(FxDriveDecide_code[copy] + 31, 2);
            g.r[1] = 0x0FA4;
            gsu_sub(g.r[0], g.r[1], 0);
            if (!gsu.s)
                { gsu_codes(FxDriveDecide_code[copy] + 33, 1); bits_and(&g, 0xFFEF, GSU_COPY(0)); }
            bits_or(&g, 0x0001, GSU_COPY(1));
            g.r[0] = car_field(&g, CAR_FLAGS40, GSU_COPY(0));
            g.r[1] = 0x4000;
            g.r[0] = gsu_and(g.r[0], g.r[1]);
            if (!gsu.z)
                bits_or(&g, 0x0004, GSU_COPY(7)); else { gsu_codes(FxDriveDecide_code[copy] + 34, 1); }
        }
    }
    gsu_codes(FxDriveDecide_code[copy] + 35, 1);
    g.r[0] = car_field(&g, CAR_SPEED, GSU_COPY(0));
    g.r[3] = 0x00C8;
    gsu_sub(g.r[0], g.r[3], 0);
    if (gsu.s)
        { gsu_codes(FxDriveDecide_code[copy] + 36, 1); bits_or(&g, 0x0010, GSU_COPY(2)); }
    gsu_codes(FxDriveDecide_code[copy] + 37, 2);
    g.r[0] = car_field(&g, CAR_SPREAD, GSU_COPY(0));
    g.r[1] = g.r[0];
    gsu_moves(g.r[1]);
    {
        gsu_codes(FxDriveDecide_code[copy] + 39, 2);
        int zero = gsu.z;

        g.r[0] = gsu_dec(g.r[0]);                /* in the branch's delay slot */
        if (!zero) {
            gsu_codes(FxDriveDecide_code[copy] + 41, 1);
            gsu_sbk(g.r[0]);
            gsu_codes(FxDriveDecide_code[copy] + 42, 1);
            bits_and(&g, 0xFBEF, GSU_COPY(0));
        }
        gsu_codes(FxDriveDecide_code[copy] + 43, 1);
    }
    g.r[0] = car_field(&g, CAR_STUCK, GSU_COPY(0));
    gsu_moves(g.r[0]);
    {
        gsu_codes(FxDriveDecide_code[copy] + 44, 2);
        int counting = 1;

        if (!gsu.s) {
            g.r[1] = 0x0014;
            gsu_sub(g.r[0], g.r[1], 0);
            if (gsu.s)
                counting = 0;
            else
                { gsu_code(0x04C8DC, 6); g.r[0] = 0xFFD8; }                 /* -40: start the turn */
        }
        if (counting) {
            g.r[0] = gsu_inc(g.r[0]);
            gsu_sbk(g.r[0]);
            g.r[1] = 0xFFEC;
            gsu_sub(g.r[0], g.r[1], 0);
            if (gsu.s) {
                bits_and(&g, 0xFBAF, GSU_COPY(3));
                bits_or(&g, 0x0020, GSU_COPY(5));
                g.r[0] = 0x0003;
                g.r[9] = gsu_xor(g.r[0], g.r[9]);
            }
        } else {
            g.r[0] = car_field(&g, CAR_SPEED, GSU_COPY(2));
            g.r[1] = 0x000A;
            gsu_sub(g.r[0], g.r[1], 0);
            if (gsu.s) {
                gsu_codes(FxDriveDecide_code[copy] + 46, 1);
                g.r[0] = gsu_inc(car_field(&g, CAR_STUCK, GSU_COPY(0)));
                gsu_codes(FxDriveDecide_code[copy] + 47, 1);
                gsu_sbk(g.r[0]);
            } else {
                gsu_codes(FxDriveDecide_code[copy] + 48, 1);
                int zero;

                g.r[0] = car_field(&g, CAR_STUCK, GSU_COPY(3));
                gsu_moves(g.r[0]);
                zero = gsu.z;
                g.r[0] = gsu_dec(g.r[0]);        /* in the branch's delay slot */
                if (!zero)
                    { gsu_codes(FxDriveDecide_code[copy] + 49, 1); gsu_sbk(g.r[0]); } else { gsu_codes(FxDriveDecide_code[copy] + 50, 1); }
            }
            gsu_codes(FxDriveDecide_code[copy] + 51, 1);
        }
    }
    g.r[0] = car_field(&g, CAR_44, GSU_COPY(0));
    gsu_moves(g.r[0]);
    if (!gsu.z)
        { gsu_codes(FxDriveDecide_code[copy] + 52, 1); bits_and(&g, 0xFBEF, GSU_COPY(0)); }
    gsu_codes(FxDriveDecide_code[copy] + 53, 2);
    g.r[0] = gsu_add(CAR_SIDES, g.r[8], 0);
    gsu_stw(g.r[0], g.r[9]);
    gsu_codes(FxDriveDecide_code[copy] + 55, 1);
    gsu_pull(sp);
    gsu_code(0x04888C, 1);
    return g;
}

void abi_FxDriveDecide(void)
{
    FxRegs g;

    memcpy(g.r, gsu.r, sizeof g.r);
    g = FxDriveDecide(g, GSU_COPY(0));
    memcpy(gsu.r, g.r, 15 * sizeof g.r[0]);
}

#define FX_STEER_RATE 0x2662u
#define FX_STEER_RATE2 0x2664u
#define FX_STEER_MAX 0x2666u
#define FX_STEER_MAX2 0x2668u
#define FX_STEER_BACK 0x266Au

/* |v| by MOVES, then NOT and INC when negative. */
static uint16_t magnitude(uint16_t v)
{
    gsu_code(0x04BA22, 4); gsu_code(0x04BA28, 5); gsu_code(0x04BA2F, 6);
    gsu_moves(v);
    return gsu.s ? gsu_inc(gsu_not(v)) : v;
}

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode FxSteerFromInput_code[2][47] = {
    {{0x04BA0A, 8}, {0x04BA12, 16}, {0x04BA2D, 2}, {0x04BA12, 6}, {0, 0}, {0x04BA35, 7}, {0x04BA3C, 16}, {0, 0}, {0x04BA4C, 4}, {0x04BA50, 5}, {0x04BA55, 8}, {0x04BA55, 4}, {0x04BA5D, 3}, {0x04BA60, 5}, {0x04BA69, 6}, {0x04BA6F, 5}, {0x04BA78, 6}, {0x04BA3C, 11}, {0x04BA7E, 9}, {0x04BA87, 5}, {0x04BA92, 5}, {0x04BA9E, 5}, {0, 0}, {0x04BA97, 7}, {0x04BAA3, 7}, {0x04BAAA, 4}, {0x04BAAE, 12}, {0x04BAC1, 5}, {0, 0}, {0x04BABA, 7}, {0x04BACD, 2}, {0x04BAC6, 7}, {0x04BA8C, 6}, {0, 0}, {0, 0}, {0x04BACF, 7}, {0x04BAD6, 5}, {0x04BADB, 6}, {0x04BAEC, 2}, {0x04BAF5, 2}, {0x04BAE1, 11}, {0x04BAEE, 7}, {0x04BAF7, 10}, {0x04BB01, 6}, {0x04BB07, 5}, {0x04BB0C, 6}, {0x04BB12, 2}},
    {{0x04BA0A, 8}, {0x04BA12, 16}, {0x04BA2D, 2}, {0x04BA12, 6}, {0x04BA35, 7}, {0, 0}, {0x04BA3C, 16}, {0x04BA50, 5}, {0x04BA4C, 4}, {0, 0}, {0x04BA55, 8}, {0x04BA55, 4}, {0x04BA5D, 3}, {0x04BA60, 9}, {0, 0}, {0x04BA6F, 9}, {0, 0}, {0x04BA3C, 11}, {0x04BA7E, 14}, {0, 0}, {0x04BA92, 5}, {0x04BA9E, 12}, {0x04BB01, 6}, {0x04BA97, 7}, {0, 0}, {0x04BAAA, 4}, {0x04BAAE, 24}, {0x04BACD, 2}, {0x04BB0C, 6}, {0, 0}, {0, 0}, {0x04BAC6, 7}, {0x04BA8C, 6}, {0x04BACF, 7}, {0x04BAF7, 10}, {0, 0}, {0x04BAD6, 5}, {0x04BADB, 6}, {0x04BAEC, 2}, {0x04BAF5, 2}, {0x04BAE1, 11}, {0x04BAEE, 7}, {0, 0}, {0, 0}, {0x04BB07, 5}, {0, 0}, {0x04BB12, 2}},
};

/*
 * @routine 04:BA05 FxSteerFromInput
 * WHAT IT DOES: The car R8's new steering value R3 from its old one
 * (+$3A) and the control bits R2: bit 0 steers one way and bit 1 the
 * other by $2662 a step (by $80 when already turned, and from the
 * opposite side straight back to 0), up to $2666 either way; bits 2 and 3
 * steer harder by $2664, the limit raised by $2668. With no bits it
 * returns toward 0 by $266A a step. With bit 10 of +$40 set the step is
 * scaled by the speed (twice $2662 times |+$4A|, $7F at most, / 256) and
 * a value is kept as it is when no bit is held (decoded from the ROM
 * bytes and not reached in the traces so far, as are the returns where
 * the value has not yet reached 0).
 * HOW: R5 = the step and R4 = its negative; R7 = 1 for the speed-scaled
 * case; limits by CMP and the sign. Three adds are WITH R3 before a BRA,
 * done in its delay slot.
 * Entry: R2 = the control bits; R8 = the car's record; R11 = return
 * address. Exit: R3 = the new value; R4, R5, R6 from the limits; R0 and
 * the flags from the last test; the last RAM address the last value read.
 */
FxRegs FxSteerFromInput(FxRegs g, int copy)
{
    gsu_code(0x04BA06, 4);
    g.r[5] = gsu_ldw(FX_STEER_RATE);
    g.r[7] = 0;
    gsu_codes(FxSteerFromInput_code[copy], 1);
    g.r[1] = gsu_ldw(gsu_add(CAR_FLAGS40, g.r[8], 0));
    g.r[0] = gsu_and(0x0400, g.r[1]);
    if (!gsu.z) {
        gsu_codes(FxSteerFromInput_code[copy] + 1, 1);
        g.r[7] = 1;
        g.r[6] = 0x007F;
        g.r[0] = magnitude(gsu_ldw(gsu_add(CAR_SPEED, g.r[8], 0)));
        gsu_sub(g.r[0], g.r[6], 0);
        if (!gsu.s)
            { gsu_codes(FxSteerFromInput_code[copy] + 2, 1); g.r[0] = g.r[6]; }
        g.r[6] = gsu_swap(g.r[0]);
        g.r[5] = gsu_add(g.r[5], g.r[5], 0);
        g.r[5] = gsu_fmult(g.r[5], g.r[6]);
    } else { gsu_codes(FxSteerFromInput_code[copy] + 3, 2); }
    gsu_codes(FxSteerFromInput_code[copy] + 5, 1);
    g.r[3] = gsu_ldw(gsu_add(CAR_STEER_BIAS, g.r[8], 0));
    g.r[4] = gsu_inc(gsu_not(g.r[5]));
    g.r[0] = gsu_and(0x000F, g.r[2]);
    if (gsu.z) {                                     gsu_codes(FxSteerFromInput_code[copy] + 6, 2); /* no bits: back toward 0 */
        gsu_moves(g.r[7]);
        if (!gsu.z)
            { gsu_codes(FxSteerFromInput_code[copy] + 8, 1); return g; } else { gsu_codes(FxSteerFromInput_code[copy] + 9, 1); }
        g.r[1] = gsu_ldw(FX_STEER_BACK);
        gsu_moves(g.r[3]);
        if (gsu.z)
            { gsu_codes(FxSteerFromInput_code[copy] + 10, 1); return g; } else { gsu_codes(FxSteerFromInput_code[copy] + 11, 2); }
        if (gsu.s) {
            gsu_codes(FxSteerFromInput_code[copy] + 13, 2);
            g.r[3] = gsu_add(g.r[3], g.r[1], 0);
            if (gsu.s)
                return g;
        } else {
            gsu_codes(FxSteerFromInput_code[copy] + 15, 2);
            g.r[3] = gsu_sub(g.r[3], g.r[1], 0);
            if (!gsu.s)
                return g;
        }
        g.r[3] = 0;
        return g;
    } else { gsu_codes(FxSteerFromInput_code[copy] + 17, 2); }
    g.r[6] = 0;
    g.r[0] = gsu_and(0x0003, g.r[2]);
    if (!gsu.z) {
        gsu_codes(FxSteerFromInput_code[copy] + 19, 1);
        g.r[6] = gsu_ldw(FX_STEER_MAX);
        g.r[0] = gsu_and(0x0001, g.r[2]);
        if (!gsu.z) {
            gsu_codes(FxSteerFromInput_code[copy] + 20, 3);
            gsu_moves(g.r[3]);
            if (gsu.s) {
                gsu_codes(FxSteerFromInput_code[copy] + 23, 1);
                gsu_moves(g.r[7]);
                if (gsu.z)
                    g.r[3] = 0;
            }
            gsu_moves(g.r[3]);
            if (!gsu.z) {
                gsu_codes(FxSteerFromInput_code[copy] + 24, 1);
                g.r[0] = 0x0080;
                g.r[3] = gsu_add(g.r[3], g.r[0], 0);
            } else {
                gsu_codes(FxSteerFromInput_code[copy] + 25, 1);
                g.r[3] = gsu_add(g.r[3], g.r[5], 0);
            }
        } else {
            gsu_codes(FxSteerFromInput_code[copy] + 26, 3);
            g.r[0] = gsu_and(0x0002, g.r[2]);
            if (!gsu.z) {
                gsu_moves(g.r[3]);
                if (!gsu.s) {
                    gsu_codes(FxSteerFromInput_code[copy] + 29, 2);
                    gsu_moves(g.r[7]);
                    if (gsu.z)
                        g.r[3] = 0;
                } else { gsu_codes(FxSteerFromInput_code[copy] + 31, 1); }
                gsu_moves(g.r[3]);
                if (!gsu.z) {
                    g.r[0] = 0x0080;
                    g.r[3] = gsu_sub(g.r[3], g.r[0], 0);
                } else {
                    g.r[3] = gsu_add(g.r[3], g.r[4], 0);
                }
            }
        }
        gsu_codes(FxSteerFromInput_code[copy] + 32, 3);
    }
    gsu_codes(FxSteerFromInput_code[copy] + 35, 1);
    g.r[0] = gsu_and(0x000C, g.r[2]);
    if (!gsu.z) {
        gsu_codes(FxSteerFromInput_code[copy] + 36, 1);
        g.r[0] = gsu_ldw(FX_STEER_MAX2);
        g.r[6] = gsu_add(g.r[6], g.r[0], 0);
        gsu_codes(FxSteerFromInput_code[copy] + 37, 1);
        g.r[5] = gsu_ldw(FX_STEER_RATE2);
        g.r[4] = gsu_inc(gsu_not(g.r[5]));
        g.r[0] = gsu_and(0x0004, g.r[2]);
        if (!gsu.z)
            { gsu_codes(FxSteerFromInput_code[copy] + 38, 1); g.r[3] = gsu_add(g.r[3], g.r[5], 0); } else { gsu_codes(FxSteerFromInput_code[copy] + 39, 1); }
        gsu_codes(FxSteerFromInput_code[copy] + 40, 2);
        g.r[0] = gsu_and(0x0008, g.r[2]);
        if (!gsu.z)
            g.r[3] = gsu_add(g.r[3], g.r[4], 0);
    }
    gsu_codes(FxSteerFromInput_code[copy] + 42, 1);
    g.r[4] = gsu_inc(gsu_not(g.r[6]));
    gsu_moves(g.r[3]);
    if (!gsu.s) {
        gsu_codes(FxSteerFromInput_code[copy] + 43, 1);
        gsu_sub(g.r[3], g.r[6], 0);
        if (!gsu.s)
            { gsu_codes(FxSteerFromInput_code[copy] + 44, 1); g.r[3] = g.r[6]; }
    } else {
        gsu_codes(FxSteerFromInput_code[copy] + 45, 1);
        gsu_sub(g.r[3], g.r[4], 0);
        if (gsu.s)
            { gsu_codes(FxSteerFromInput_code[copy] + 46, 1); g.r[3] = g.r[4]; }
    }
    return g;
}

void abi_FxSteerFromInput(void)
{
    FxRegs g;

    memcpy(g.r, gsu.r, sizeof g.r);
    g = FxSteerFromInput(g, GSU_COPY(0));
    memcpy(gsu.r, g.r, 15 * sizeof g.r[0]);
    gsu_code(0x04BB14, 3);
}
