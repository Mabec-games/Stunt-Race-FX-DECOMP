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
static void bits_and(FxRegs *g, uint16_t v) { g->r[0] = v; g->r[9] = gsu_and(g->r[0], g->r[9]); }
static void bits_or(FxRegs *g, uint16_t v) { g->r[0] = v; g->r[9] = gsu_or(g->r[0], g->r[9]); }

/* The field `f` of car R8 (R0 = f + R8, then LDW). */
static uint16_t car_field(FxRegs *g, uint16_t f)
{
    g->r[0] = gsu_add(f, g->r[8], 0);
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
FxRegs FxDriveDecide(FxRegs g)
{
    uint16_t *sp = &g.r[10];

    gsu_push(sp, g.r[11]);
    g.r[0] = car_field(&g, CAR_70);
    g.r[5] = 0x0020;
    g.r[0] = gsu_and(g.r[0], g.r[5]);
    if (!gsu.z) {
        g.r[4] = gsu_div2(g.r[4]);
        g.r[0] = gsu_div2(g.r[4]);
        g.r[4] = gsu_add(g.r[4], g.r[0], 0);
    }
    gsu_push(sp, g.r[4]);
    g.r[11] = 0xC7F5;
    g = FxBearingTo(g);
    g.r[1] = g.r[0];
    g.r[0] = car_field(&g, CAR_PITCH);
    g.r[1] = gsu_sub(g.r[1], g.r[0], 0);
    g.r[0] = car_field(&g, CAR_FLAGS40);
    g.r[2] = 0x0100;
    g.r[0] = gsu_and(g.r[0], g.r[2]);
    if (!gsu.z) {
        g.r[0] = car_field(&g, CAR_YAW);
        g.r[2] = g.r[0];
        g.r[0] = gsu_div2(gsu_div2(g.r[0]));
        g.r[2] = gsu_sub(g.r[2], g.r[0], 0);
        gsu_sbk(g.r[2]);
    }
    g.r[4] = gsu_pull(sp);
    g.r[9] = 0;
    g.r[0] = car_field(&g, CAR_SPEED);
    g.r[3] = g.r[4];
    gsu_sub(g.r[0], g.r[3], 0);
    if (!gsu.s) {
        bits_or(&g, 0x0040);
    } else {
        g.r[2] = 0x0227;
        gsu_sub(g.r[4], g.r[2], 0);
        bits_or(&g, gsu.s ? 0x0010 : 0x0400);
    }
    g.r[2] = gsu_ldw(gsu_add(CAR_STEER_BIAS, g.r[8], 0));
    g.r[0] = gsu_sub(g.r[1], g.r[2], 0);
    if (gsu.s) {
        g.r[1] = 0xFC72;
        gsu_sub(g.r[0], g.r[1], 0);
        if (gsu.s) {
            g.r[1] = 0xF05C;
            gsu_sub(g.r[0], g.r[1], 0);
            if (gsu.s)
                bits_and(&g, 0xFFEF);
            bits_or(&g, 0x0002);
            g.r[0] = car_field(&g, CAR_FLAGS40);
            g.r[1] = 0x4000;
            g.r[0] = gsu_and(g.r[0], g.r[1]);
            if (!gsu.z)
                bits_or(&g, 0x0008);
        }
    } else {
        g.r[1] = 0x038E;
        gsu_sub(g.r[0], g.r[1], 0);
        if (!gsu.s) {
            g.r[1] = 0x0FA4;
            gsu_sub(g.r[0], g.r[1], 0);
            if (!gsu.s)
                bits_and(&g, 0xFFEF);
            bits_or(&g, 0x0001);
            g.r[0] = car_field(&g, CAR_FLAGS40);
            g.r[1] = 0x4000;
            g.r[0] = gsu_and(g.r[0], g.r[1]);
            if (!gsu.z)
                bits_or(&g, 0x0004);
        }
    }
    g.r[0] = car_field(&g, CAR_SPEED);
    g.r[3] = 0x00C8;
    gsu_sub(g.r[0], g.r[3], 0);
    if (gsu.s)
        bits_or(&g, 0x0010);
    g.r[0] = car_field(&g, CAR_SPREAD);
    g.r[1] = g.r[0];
    gsu_moves(g.r[1]);
    {
        int zero = gsu.z;

        g.r[0] = gsu_dec(g.r[0]);                /* in the branch's delay slot */
        if (!zero) {
            gsu_sbk(g.r[0]);
            bits_and(&g, 0xFBEF);
        }
    }
    g.r[0] = car_field(&g, CAR_STUCK);
    gsu_moves(g.r[0]);
    {
        int counting = 1;

        if (!gsu.s) {
            g.r[1] = 0x0014;
            gsu_sub(g.r[0], g.r[1], 0);
            if (gsu.s)
                counting = 0;
            else
                g.r[0] = 0xFFD8;                 /* -40: start the turn */
        }
        if (counting) {
            g.r[0] = gsu_inc(g.r[0]);
            gsu_sbk(g.r[0]);
            g.r[1] = 0xFFEC;
            gsu_sub(g.r[0], g.r[1], 0);
            if (gsu.s) {
                bits_and(&g, 0xFBAF);
                bits_or(&g, 0x0020);
                g.r[0] = 0x0003;
                g.r[9] = gsu_xor(g.r[0], g.r[9]);
            }
        } else {
            g.r[0] = car_field(&g, CAR_SPEED);
            g.r[1] = 0x000A;
            gsu_sub(g.r[0], g.r[1], 0);
            if (gsu.s) {
                g.r[0] = gsu_inc(car_field(&g, CAR_STUCK));
                gsu_sbk(g.r[0]);
            } else {
                int zero;

                g.r[0] = car_field(&g, CAR_STUCK);
                gsu_moves(g.r[0]);
                zero = gsu.z;
                g.r[0] = gsu_dec(g.r[0]);        /* in the branch's delay slot */
                if (!zero)
                    gsu_sbk(g.r[0]);
            }
        }
    }
    g.r[0] = car_field(&g, CAR_44);
    gsu_moves(g.r[0]);
    if (!gsu.z)
        bits_and(&g, 0xFBEF);
    g.r[0] = gsu_add(CAR_SIDES, g.r[8], 0);
    gsu_stw(g.r[0], g.r[9]);
    gsu_pull(sp);
    return g;
}

void abi_FxDriveDecide(void)
{
    FxRegs g;

    memcpy(g.r, gsu.r, sizeof g.r);
    g = FxDriveDecide(g);
    memcpy(gsu.r, g.r, 15 * sizeof g.r[0]);
}
