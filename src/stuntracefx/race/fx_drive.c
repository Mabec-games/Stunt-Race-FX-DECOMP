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

/* R9 = R0 op R9 (TO R9 with the operation). */
static void bits_and(FxRegs *g, uint16_t v, int copy) { g->r[0] = v; g->r[9] = gsu_and(g->r[0], g->r[9]); }
static void bits_or(FxRegs *g, uint16_t v, int copy) { g->r[0] = v; g->r[9] = gsu_or(g->r[0], g->r[9]); }

/* The field `f` of car R8 (R0 = f + R8, then LDW). */
/* gsuwalk: the code of each copy of car_field, read from the ROM */
static const GsuCode car_field_fetch[14][1] = {
    {{0x04c7d8, 6}},
    {{0x04c7f6, 6}},
    {{0x04c7fc, 6}},
    {{0x04c809, 4}},
    {{0x04c81b, 6}},
    {{0x04c867, 9}},
    {{0x04c894, 9}},
    {{0x04c8ac, 4}},
    {{0x04c8bd, 4}},
    {{0x04c8cc, 4}},
    {{0x04c8fd, 4}},
    {{0x04c909, 4}},
    {{0x04c912, 4}},
    {{0x04c91c, 4}},
};

static uint16_t car_field(FxRegs *g, uint16_t f, int copy)
{
    g->r[0] = gsu_add(f, g->r[8], 0);
    gsu_codes(car_field_fetch[copy], 1);
    return gsu_ldw(g->r[0]);
}

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
    uint16_t *sp = &g.r[10];

    gsu_code(0x04c7d6, 2);
    gsu_push(sp, g.r[11]);
    g.r[0] = car_field(&g, CAR_70, GSU_COPY(0));
    g.r[5] = 0x0020;
    g.r[0] = gsu_and(g.r[0], g.r[5]);
    if (!gsu.z) {
        g.r[4] = gsu_div2(g.r[4]);
        g.r[0] = gsu_div2(g.r[4]);
        g.r[4] = gsu_add(g.r[4], g.r[0], 0);
        gsu_code(0x04c7de, 15);
    } else { gsu_code(0x04c7de, 6); gsu_code(0x04c7ec, 1); }
    gsu_code(0x04c7ed, 2);
    gsu_push(sp, g.r[4]);
    g.r[11] = 0xC7F5;
    gsu_code(0x04c7ef, 6); gsu_code(0x04cbe8, 1);
    g = FxBearingTo(g, GSU_COPY(0));
    g.r[1] = g.r[0];
    gsu_code(0x04c7f5, 1);
    g.r[0] = car_field(&g, CAR_PITCH, GSU_COPY(1));
    g.r[1] = gsu_sub(g.r[1], g.r[0], 0);
    g.r[0] = car_field(&g, CAR_FLAGS40, GSU_COPY(2));
    g.r[2] = 0x0100;
    g.r[0] = gsu_and(g.r[0], g.r[2]);
    if (!gsu.z) {
        gsu_code(0x04c802, 7);
        g.r[0] = car_field(&g, CAR_YAW, GSU_COPY(3));
        g.r[2] = g.r[0];
        g.r[0] = gsu_div2(gsu_div2(g.r[0]));
        g.r[2] = gsu_sub(g.r[2], g.r[0], 0);
        gsu_code(0x04c80d, 10);
        gsu_sbk(g.r[2]);
    } else { gsu_code(0x04c802, 6); gsu_code(0x04c816, 1); }
    gsu_code(0x04c817, 4);
    g.r[4] = gsu_pull(sp);
    g.r[9] = 0;
    g.r[0] = car_field(&g, CAR_SPEED, GSU_COPY(4));
    g.r[3] = g.r[4];
    gsu_sub(g.r[0], g.r[3], 0);
    if (!gsu.s) {
        gsu_code(0x04c821, 6); gsu_code(0x04c840, 1);
        bits_or(&g, 0x0040, GSU_COPY(0));
        gsu_code(0x04c841, 5);
    } else {
        g.r[2] = 0x0227;
        gsu_sub(g.r[4], g.r[2], 0);
        bits_or(&g, gsu.s ? (gsu_code(0x04c821, 15), gsu_code(0x04c838, 8), gsu_code(0x04c845, 1), ((0x0010))) : (gsu_code(0x04c821, 23), gsu_code(0x04c845, 1), (((0x0400)))), GSU_COPY(1));
    }
    gsu_code(0x04c846, 6);
    g.r[2] = gsu_ldw(gsu_add(CAR_STEER_BIAS, g.r[8], 0));
    g.r[0] = gsu_sub(g.r[1], g.r[2], 0);
    if (gsu.s) {
        g.r[1] = 0xFC72;
        gsu_sub(g.r[0], g.r[1], 0);
        if (gsu.s) {
            g.r[1] = 0xF05C;
            gsu_sub(g.r[0], g.r[1], 0);
            if (gsu.s)
                { { gsu_code(0x04c84c, 22); bits_and(&g, 0xFFEF, GSU_COPY(0)); gsu_code(0x04c862, 5); } } else { gsu_code(0x04c84c, 21); gsu_code(0x04c866, 1); }
            bits_or(&g, 0x0002, GSU_COPY(2));
            g.r[0] = car_field(&g, CAR_FLAGS40, GSU_COPY(5));
            g.r[1] = 0x4000;
            g.r[0] = gsu_and(g.r[0], g.r[1]);
            if (!gsu.z)
                { { gsu_code(0x04c870, 7); bits_or(&g, 0x0008, GSU_COPY(3)); gsu_code(0x04c877, 7); gsu_code(0x04c8ab, 1); } } else { gsu_code(0x04c870, 6); gsu_code(0x04c8ab, 1); }
        } else { gsu_code(0x04c84c, 13); gsu_code(0x04c8ab, 1); }
    } else {
        g.r[1] = 0x038E;
        gsu_sub(g.r[0], g.r[1], 0);
        if (!gsu.s) {
            g.r[1] = 0x0FA4;
            gsu_sub(g.r[0], g.r[1], 0);
            if (!gsu.s)
                { { gsu_code(0x04c84c, 5); gsu_code(0x04c87e, 17); bits_and(&g, 0xFFEF, GSU_COPY(1)); gsu_code(0x04c88f, 5); } } else { gsu_code(0x04c84c, 5); gsu_code(0x04c87e, 16); gsu_code(0x04c893, 1); }
            bits_or(&g, 0x0001, GSU_COPY(4));
            g.r[0] = car_field(&g, CAR_FLAGS40, GSU_COPY(6));
            g.r[1] = 0x4000;
            g.r[0] = gsu_and(g.r[0], g.r[1]);
            if (!gsu.z)
                { { gsu_code(0x04c89d, 7); bits_or(&g, 0x0004, GSU_COPY(5)); gsu_code(0x04c8a4, 8); } } else { gsu_code(0x04c89d, 6); gsu_code(0x04c8ab, 1); }
        } else { gsu_code(0x04c84c, 5); gsu_code(0x04c87e, 8); gsu_code(0x04c8ab, 1); }
    }
    g.r[0] = car_field(&g, CAR_SPEED, GSU_COPY(7));
    g.r[3] = 0x00C8;
    gsu_sub(g.r[0], g.r[3], 0);
    if (gsu.s)
        { { gsu_code(0x04c8b0, 8); bits_or(&g, 0x0010, GSU_COPY(6)); gsu_code(0x04c8b8, 5); } } else { gsu_code(0x04c8b0, 7); gsu_code(0x04c8bc, 1); }
    g.r[0] = car_field(&g, CAR_SPREAD, GSU_COPY(8));
    g.r[1] = g.r[0];
    gsu_moves(g.r[1]);
    {
        int zero = gsu.z;

        g.r[0] = gsu_dec(g.r[0]);                /* in the branch's delay slot */
        if (!zero) {
            gsu_code(0x04c8c1, 6);
            gsu_sbk(g.r[0]);
            bits_and(&g, 0xFBEF, GSU_COPY(2));
            gsu_code(0x04c8c7, 5);
        } else { gsu_code(0x04c8c1, 4); gsu_code(0x04c8cb, 1); }
    }
    g.r[0] = car_field(&g, CAR_STUCK, GSU_COPY(9));
    gsu_moves(g.r[0]);
    if (!gsu.s) {                              /* BMI to the count, else BMI past it */
        g.r[1] = 0x0014;
        gsu_sub(g.r[0], g.r[1], 0);
        if (gsu.s)
            { gsu_code(0x04c8d0, 12); gsu_code(0x04c8fc, 1); goto not_counting; } else { gsu_code(0x04c8d0, 12); }
        g.r[0] = 0xFFD8;                         /* -40: start the turn */
        gsu_code(0x04c8dc, 4);
    } else { gsu_code(0x04c8d0, 4); gsu_code(0x04c8df, 1); }
    g.r[0] = gsu_inc(g.r[0]);
    gsu_code(0x04c8e0, 2);
    gsu_sbk(g.r[0]);
    g.r[1] = 0xFFEC;
    gsu_sub(g.r[0], g.r[1], 0);
    if (gsu.s) {
        gsu_code(0x04c8e2, 8);
        bits_and(&g, 0xFBAF, GSU_COPY(3));
        bits_or(&g, 0x0020, GSU_COPY(7));
        g.r[0] = 0x0003;
        g.r[9] = gsu_xor(g.r[0], g.r[9]);
        gsu_code(0x04c8ea, 18); gsu_code(0x04c91b, 1);
    } else { gsu_code(0x04c8e2, 7); gsu_code(0x04c91b, 1); }
    goto counted;
not_counting:
    g.r[0] = car_field(&g, CAR_SPEED, GSU_COPY(10));
    g.r[1] = 0x000A;
    gsu_sub(g.r[0], g.r[1], 0);
    if (gsu.s) {
        gsu_code(0x04c901, 8);
        g.r[0] = gsu_inc(car_field(&g, CAR_STUCK, GSU_COPY(11)));
        gsu_code(0x04c90d, 2);
        gsu_sbk(g.r[0]);
        gsu_code(0x04c90f, 2); gsu_code(0x04c91b, 1);
    } else {
        int zero;

        gsu_code(0x04c901, 7); gsu_code(0x04c911, 1);
        g.r[0] = car_field(&g, CAR_STUCK, GSU_COPY(12));
        gsu_moves(g.r[0]);
        zero = gsu.z;
        g.r[0] = gsu_dec(g.r[0]);                /* in the branch's delay slot */
        if (!zero)
            { gsu_code(0x04c916, 6); gsu_sbk(g.r[0]); } else { gsu_code(0x04c916, 4); gsu_code(0x04c91b, 1); }
    }
counted:
    g.r[0] = car_field(&g, CAR_44, GSU_COPY(13));
    gsu_moves(g.r[0]);
    if (!gsu.z)
        { { gsu_code(0x04c920, 5); bits_and(&g, 0xFBEF, GSU_COPY(4)); gsu_code(0x04c925, 5); } } else { gsu_code(0x04c920, 4); gsu_code(0x04c929, 1); }
    g.r[0] = gsu_add(CAR_SIDES, g.r[8], 0);
    gsu_code(0x04c92a, 5);
    gsu_stw(g.r[0], g.r[9]);
    gsu_code(0x04c92f, 4);
    gsu_pull(sp);
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
    gsu_moves(v);
    return gsu.s ? (gsu_code(0x04ba22, 7), gsu_inc(gsu_not(v))) : (gsu_code(0x04ba22, 4), gsu_code(0x04ba28, 1), ((v)));
}

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
    gsu_code(0x04ba06, 4);
    g.r[5] = gsu_ldw(FX_STEER_RATE);
    g.r[7] = 0;
    gsu_code(0x04ba0a, 8);
    g.r[1] = gsu_ldw(gsu_add(CAR_FLAGS40, g.r[8], 0));
    g.r[0] = gsu_and(0x0400, g.r[1]);
    if (!gsu.z) {
        g.r[7] = 1;
        g.r[6] = 0x007F;
        gsu_code(0x04ba12, 16);
        g.r[0] = magnitude(gsu_ldw(gsu_add(CAR_SPEED, g.r[8], 0)));
        gsu_sub(g.r[0], g.r[6], 0);
        if (!gsu.s)
            { g.r[0] = g.r[6]; gsu_code(0x04ba29, 7); } else { gsu_code(0x04ba29, 4); gsu_code(0x04ba2f, 1); }
        g.r[6] = gsu_swap(g.r[0]);
        g.r[5] = gsu_add(g.r[5], g.r[5], 0);
        g.r[5] = gsu_fmult(g.r[5], g.r[6]);
        gsu_code(0x04ba30, 6);
    } else { gsu_code(0x04ba12, 6); gsu_code(0x04ba35, 1); }
    gsu_code(0x04ba36, 6);
    g.r[3] = gsu_ldw(gsu_add(CAR_STEER_BIAS, g.r[8], 0));
    g.r[4] = gsu_inc(gsu_not(g.r[5]));
    g.r[0] = gsu_and(0x000F, g.r[2]);
    if (gsu.z) {                                     /* no bits: back toward 0 */
        gsu_moves(g.r[7]);
        if (!gsu.z)
            { gsu_code(0x04ba3c, 20); gsu_code(0x04bb14, 3); return g; } else { gsu_code(0x04ba3c, 16); gsu_code(0x04ba50, 1); }
        gsu_code(0x04ba51, 4);
        g.r[1] = gsu_ldw(FX_STEER_BACK);
        gsu_moves(g.r[3]);
        if (gsu.z)
            { gsu_code(0x04ba55, 8); gsu_code(0x04bb14, 3); return g; } else { gsu_code(0x04ba55, 4); gsu_code(0x04ba5d, 1); }
        if (gsu.s) {
            g.r[3] = gsu_add(g.r[3], g.r[1], 0);
            if (gsu.s)
                { gsu_code(0x04ba5e, 11); gsu_code(0x04bb14, 3); return g; } else { gsu_code(0x04ba5e, 7); gsu_code(0x04ba69, 1); }
            gsu_code(0x04ba6a, 5); gsu_code(0x04bb14, 1);
        } else {
            g.r[3] = gsu_sub(g.r[3], g.r[1], 0);
            if (!gsu.s)
                { gsu_code(0x04ba5e, 2); gsu_code(0x04ba6f, 9); gsu_code(0x04bb14, 3); return g; } else { gsu_code(0x04ba5e, 2); gsu_code(0x04ba6f, 5); gsu_code(0x04ba78, 1); }
            gsu_code(0x04ba79, 5); gsu_code(0x04bb14, 1);
        }
        g.r[3] = 0;
        gsu_code(0x04bb15, 2);
        return g;
    } else { gsu_code(0x04ba3c, 11); gsu_code(0x04ba7e, 1); }
    g.r[6] = 0;
    g.r[0] = gsu_and(0x0003, g.r[2]);
    if (!gsu.z) {
        gsu_code(0x04ba7f, 13);
        g.r[6] = gsu_ldw(FX_STEER_MAX);
        g.r[0] = gsu_and(0x0001, g.r[2]);
        if (!gsu.z) {
            gsu_moves(g.r[3]);
            if (gsu.s) {
                gsu_moves(g.r[7]);
                if (gsu.z)
                    { g.r[3] = 0; gsu_code(0x04ba8c, 19); } else { gsu_code(0x04ba8c, 16); gsu_code(0x04ba9e, 1); }
            } else { gsu_code(0x04ba8c, 11); gsu_code(0x04ba9e, 1); }
            gsu_moves(g.r[3]);
            if (!gsu.z) {
                g.r[0] = 0x0080;
                g.r[3] = gsu_add(g.r[3], g.r[0], 0);
                gsu_code(0x04ba9f, 11); gsu_code(0x04bacf, 1);
            } else {
                g.r[3] = gsu_add(g.r[3], g.r[5], 0);
                gsu_code(0x04ba9f, 4); gsu_code(0x04baaa, 4); gsu_code(0x04bacf, 1);
            }
        } else {
            g.r[0] = gsu_and(0x0002, g.r[2]);
            if (!gsu.z) {
                gsu_moves(g.r[3]);
                if (!gsu.s) {
                    gsu_moves(g.r[7]);
                    if (gsu.z)
                        { g.r[3] = 0; gsu_code(0x04ba8c, 6); gsu_code(0x04baae, 20); } else { gsu_code(0x04ba8c, 6); gsu_code(0x04baae, 17); gsu_code(0x04bac1, 1); }
                } else { gsu_code(0x04ba8c, 6); gsu_code(0x04baae, 12); gsu_code(0x04bac1, 1); }
                gsu_moves(g.r[3]);
                if (!gsu.z) {
                    g.r[0] = 0x0080;
                    g.r[3] = gsu_sub(g.r[3], g.r[0], 0);
                    gsu_code(0x04bac2, 11); gsu_code(0x04bacf, 1);
                } else {
                    g.r[3] = gsu_add(g.r[3], g.r[4], 0);
                    gsu_code(0x04bac2, 4); gsu_code(0x04bacd, 3);
                }
            } else { gsu_code(0x04ba8c, 6); gsu_code(0x04baae, 7); gsu_code(0x04bacf, 1); }
        }
    } else { gsu_code(0x04ba7f, 8); gsu_code(0x04bacf, 1); }
    g.r[0] = gsu_and(0x000C, g.r[2]);
    if (!gsu.z) {
        gsu_code(0x04bad0, 11);
        g.r[0] = gsu_ldw(FX_STEER_MAX2);
        g.r[6] = gsu_add(g.r[6], g.r[0], 0);
        gsu_code(0x04badb, 6);
        g.r[5] = gsu_ldw(FX_STEER_RATE2);
        g.r[4] = gsu_inc(gsu_not(g.r[5]));
        g.r[0] = gsu_and(0x0004, g.r[2]);
        if (!gsu.z)
            { g.r[3] = gsu_add(g.r[3], g.r[5], 0); gsu_code(0x04bae1, 14); } else { gsu_code(0x04bae1, 11); gsu_code(0x04baee, 1); }
        g.r[0] = gsu_and(0x0008, g.r[2]);
        if (!gsu.z)
            { g.r[3] = gsu_add(g.r[3], g.r[4], 0); gsu_code(0x04baef, 9); } else { gsu_code(0x04baef, 6); gsu_code(0x04baf7, 1); }
    } else { gsu_code(0x04bad0, 6); gsu_code(0x04baf7, 1); }
    g.r[4] = gsu_inc(gsu_not(g.r[6]));
    gsu_moves(g.r[3]);
    if (!gsu.s) {
        gsu_sub(g.r[3], g.r[6], 0);
        if (!gsu.s)
            { g.r[3] = g.r[6]; gsu_code(0x04baf8, 20); gsu_code(0x04bb14, 1); } else { gsu_code(0x04baf8, 15); gsu_code(0x04bb14, 1); }
    } else {
        gsu_sub(g.r[3], g.r[4], 0);
        if (gsu.s)
            { g.r[3] = g.r[4]; gsu_code(0x04baf8, 9); gsu_code(0x04bb0c, 9); } else { gsu_code(0x04baf8, 9); gsu_code(0x04bb0c, 6); gsu_code(0x04bb14, 1); }
    }
    gsu_code(0x04bb15, 2);
    return g;
}

void abi_FxSteerFromInput(void)
{
    FxRegs g;

    memcpy(g.r, gsu.r, sizeof g.r);
    g = FxSteerFromInput(g, GSU_COPY(0));
    memcpy(gsu.r, g.r, 15 * sizeof g.r[0]);
}
