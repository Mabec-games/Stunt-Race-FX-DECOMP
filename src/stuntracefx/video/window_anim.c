/*
 * window_anim.c: three values at $3435, $3437, $3439 in the data bank (bank
 * $7F when called) stepped toward their limits; each step function counts a
 * finished value by incrementing X.
 */
#include "machine.h"

/*
 * @routine 03:FE06 WindowStep3437
 * WHAT IT DOES: Adds 2 to $3437 in the data bank, stopping at $FF; X counts
 * up by one when it stops.
 * HOW: 8-bit add; on carry X is incremented and $FF stored instead.
 * Entry: M 8-bit, X 8-bit. Exit: A low byte = the stored value; flags from
 * the add or, when it stopped, N set, Z clear, C set.
 */
uint8_t WindowStep3437(uint8_t bank, uint8_t *x, uint8_t *p)
{
    uint8_t v = rd8((uint32_t)bank << 16 | 0x3437);
    unsigned s = v + 2u;
    uint8_t r = (uint8_t)s;

    *p = (uint8_t)((nz8(*p, r) & ~(P_C | P_V)) | (s > 0xFF ? P_C : 0)
                   | ((~(v ^ 2) & (v ^ r) & 0x80) ? P_V : 0));
    if (s > 0xFF) {
        (*x)++;
        r = 0xFF;
        *p = nz8(*p, r);
    }
    wr8((uint32_t)bank << 16 | 0x3437, r);
    return r;
}

void abi_WindowStep3437(void)
{
    uint8_t x = (uint8_t)cpu.x, p = cpu.p;

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | WindowStep3437(cpu.db, &x, &p));
    cpu.x = x;
    cpu.p = p;
}

/*
 * @routine 03:FE44 WindowStep3435
 * WHAT IT DOES: Subtracts 3 from $3435 in the data bank, stopping at 1; X
 * counts up by one when it stops.
 * HOW: 8-bit subtract; on borrow X is incremented and 1 stored instead.
 * Entry: M 8-bit, X 8-bit. Exit: A low byte = the stored value; flags from
 * the subtract or, when it stopped, from the load of 1 (C clear).
 */
uint8_t WindowStep3435(uint8_t bank, uint8_t *x, uint8_t *p)
{
    uint8_t v = rd8((uint32_t)bank << 16 | 0x3435), r = (uint8_t)(v - 3);

    *p = (uint8_t)((nz8(*p, r) & ~(P_C | P_V)) | (v >= 3 ? P_C : 0)
                   | (((v ^ 3) & (v ^ r) & 0x80) ? P_V : 0));
    if (v < 3) {
        (*x)++;
        r = 0x01;
        *p = nz8(*p, r);
    }
    wr8((uint32_t)bank << 16 | 0x3435, r);
    return r;
}

void abi_WindowStep3435(void)
{
    uint8_t x = (uint8_t)cpu.x, p = cpu.p;

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | WindowStep3435(cpu.db, &x, &p));
    cpu.x = x;
    cpu.p = p;
}

/*
 * @routine 03:FE15 WindowStep3439
 * WHAT IT DOES: Adds 1 to $3439 in the data bank, stopping at $E0; X counts
 * up by one when it reaches $E0.
 * HOW: 8-bit add, compare with $E0.
 * Entry: M 8-bit, X 8-bit. Exit: A low byte = the stored value; V from the
 * add; N, Z, C from the compare or, when it stopped, from the load of $E0.
 */
uint8_t WindowStep3439(uint8_t bank, uint8_t *x, uint8_t *p)
{
    uint8_t v = rd8((uint32_t)bank << 16 | 0x3439), r = (uint8_t)(v + 1);

    *p = (uint8_t)((*p & ~P_V) | ((~(v ^ 1) & (v ^ r) & 0x80) ? P_V : 0));
    if (r >= 0xE0) {
        (*x)++;
        r = 0xE0;
        *p = (uint8_t)(nz8(*p, r) | P_C);
    } else {
        *p = (uint8_t)(nz8(*p, (uint8_t)(r - 0xE0)) & ~P_C);
    }
    wr8((uint32_t)bank << 16 | 0x3439, r);
    return r;
}

void abi_WindowStep3439(void)
{
    uint8_t x = (uint8_t)cpu.x, p = cpu.p;

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | WindowStep3439(cpu.db, &x, &p));
    cpu.x = x;
    cpu.p = p;
}
