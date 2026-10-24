/*
 * window_anim.c: three values at $3435, $3437, $3439 in the data bank (bank
 * $7F when called) stepped toward their limits; each step function counts a
 * finished value by incrementing X.
 */
#include "machine.h"
#include "cpu_ops.h"
#include "video_calls.h"
#include "carried.h"
#include "wram_names.h"


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
    cyc(24);
    uint8_t v = rd8((uint32_t)bank << 16 | 0x3437);
    unsigned s = v + 2u;
    uint8_t r = (uint8_t)s;

    *p = (uint8_t)((nz8(*p, r) & ~(P_C | P_V)) | (s > 0xFF ? (cyc(24), P_C) : 0)
                   | ((~(v ^ 2) & (v ^ r) & 0x80) ? P_V : 0));
    if (s > 0xFF) {
        (*x)++;
        r = 0xFF;
        *p = nz8(*p, r);
    }
    cyc(76);
    wr8((uint32_t)bank << 16 | 0x3437, r);
    cyc(42);
    return r;
}

void WindowEdgesStep_cr(Carried *cr);
void WindowStep3433By2_cr(Carried *cr);
void WindowStep3433By3_cr(Carried *cr);
void WindowStep3435_cr(Carried *cr);
void WindowStep3437_cr(Carried *cr);
void WindowStep3437By3_cr(Carried *cr);
void WindowStep3439_cr(Carried *cr);

void WindowStep3437_cr(Carried *cr)
{
    uint8_t x = (uint8_t)cr->x, p = cr->p;

    cr->a = (uint16_t)((cr->a & 0xFF00) | WindowStep3437(cr->db, &x, &p));
    cr->x = x;
    set_p(cr, p);
}

void abi_WindowStep3437(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    WindowStep3437_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
    cyc(24);
    uint8_t v = rd8((uint32_t)bank << 16 | 0x3435), r = (uint8_t)(v - 3);

    *p = (uint8_t)((nz8(*p, r) & ~(P_C | P_V)) | (v >= 3 ? P_C : (cyc(24), 0))
                   | (((v ^ 3) & (v ^ r) & 0x80) ? P_V : 0));
    if (v < 3) {
        (*x)++;
        r = 0x01;
        *p = nz8(*p, r);
    }
    cyc(76);
    wr8((uint32_t)bank << 16 | 0x3435, r);
    cyc(42);
    return r;
}

void WindowStep3435_cr(Carried *cr)
{
    uint8_t x = (uint8_t)cr->x, p = cr->p;

    cr->a = (uint16_t)((cr->a & 0xFF00) | WindowStep3435(cr->db, &x, &p));
    cr->x = x;
    set_p(cr, p);
}

void abi_WindowStep3435(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    WindowStep3435_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
    cyc(70);
    uint8_t v = rd8((uint32_t)bank << 16 | 0x3439), r = (uint8_t)(v + 1);

    *p = (uint8_t)((*p & ~P_V) | ((~(v ^ 1) & (v ^ r) & 0x80) ? P_V : 0));
    if (r >= 0xE0) {
        cyc(24);
        (*x)++;
        r = 0xE0;
        *p = (uint8_t)(nz8(*p, r) | P_C);
    } else {
        *p = (uint8_t)(nz8(*p, (uint8_t)(r - 0xE0)) & ~P_C);
    }
    cyc(92);
    wr8((uint32_t)bank << 16 | 0x3439, r);
    cyc(42);
    return r;
}

void WindowStep3439_cr(Carried *cr)
{
    uint8_t x = (uint8_t)cr->x, p = cr->p;

    cr->a = (uint16_t)((cr->a & 0xFF00) | WindowStep3439(cr->db, &x, &p));
    cr->x = x;
    set_p(cr, p);
}

void abi_WindowStep3439(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    WindowStep3439_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* Subtract n from the byte at addr in the bank, stopping at 1 (*x
   counting the stop), as WindowStep3435; the stored byte. */
static uint8_t window_step_down(uint8_t bank, uint16_t addr, uint8_t n, uint8_t *x, uint8_t *p)
{
    uint32_t a = (uint32_t)bank << 16 | addr;
    cyc(24);
    uint8_t v = rd8(a), r = (uint8_t)(v - n);

    *p = (uint8_t)((nz8(*p, r) & ~(P_C | P_V)) | (v >= n ? P_C : (cyc(24), 0))
                   | (((v ^ n) & (v ^ r) & 0x80) ? P_V : 0));
    if (v < n) {
        *x = (uint8_t)(*x + 1);
        r = 0x01;
        *p = nz8(*p, r);
    }
    cyc(76);
    wr8(a, r);
    cyc(42);
    return r;
}

/*
 * @routine 03:FE26 WindowStep3433By2
 * WHAT IT DOES: Subtracts 2 from $3433 in the data bank, stopping at 1; X
 * counts up by one when it stops.
 * HOW: As WindowStep3435. The stop is not reached in the traces so far.
 * Entry: M 8-bit, X 8-bit; bank = DB; *x = X; *p = P. Exit: as
 * WindowStep3435 (the stored byte is the result).
 */
uint8_t WindowStep3433By2(uint8_t bank, uint8_t *x, uint8_t *p)
{
    cyc(0);
    return window_step_down(bank, 0x3433, 2, x, p);
}

void WindowStep3433By2_cr(Carried *cr)
{
    uint8_t x = (uint8_t)cr->x, p = cr->p;

    cr->a = (uint16_t)((cr->a & 0xFF00) | WindowStep3433By2(cr->db, &x, &p));
    if (x != (uint8_t)cr->x)
        cr->x = x;
    set_p(cr, p);
}

void abi_WindowStep3433By2(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    WindowStep3433By2_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:FE35 WindowStep3433By3
 * WHAT IT DOES: Subtracts 3 from $3433 in the data bank, stopping at 1; X
 * counts up by one when it stops.
 * HOW: As WindowStep3435. The stop is not reached in the traces so far.
 * Entry: M 8-bit, X 8-bit. Exit: as WindowStep3435.
 */
uint8_t WindowStep3433By3(uint8_t bank, uint8_t *x, uint8_t *p)
{
    return window_step_down(bank, 0x3433, 3, x, p);
}

void WindowStep3433By3_cr(Carried *cr)
{
    uint8_t x = (uint8_t)cr->x, p = cr->p;

    cr->a = (uint16_t)((cr->a & 0xFF00) | WindowStep3433By3(cr->db, &x, &p));
    if (x != (uint8_t)cr->x)
        cr->x = x;
    set_p(cr, p);
}

void abi_WindowStep3433By3(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    WindowStep3433By3_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:FDF7 WindowStep3437By3
 * WHAT IT DOES: Adds 3 to $3437 in the data bank, stopping at $FF; X
 * counts up by one when it stops.
 * HOW: As WindowStep3437. Not reached in the traces so far.
 * Entry: M 8-bit, X 8-bit. Exit: as WindowStep3437.
 */
uint8_t WindowStep3437By3(uint8_t bank, uint8_t *x, uint8_t *p)
{
    uint32_t a = (uint32_t)bank << 16 | 0x3437;
    cyc(24);
    uint8_t v = rd8(a), r;

    *p &= (uint8_t)~P_C;
    r = alu_adc8(v, 0x03, p);
    if (*p & P_C) {
        cyc(24);
        *x = (uint8_t)(*x + 1);
        r = 0xFF;
        *p = nz8(*p, r);
    }
    cyc(76);
    wr8(a, r);
    cyc(42);                            /* RTS */
    return r;
}

void WindowStep3437By3_cr(Carried *cr)
{
    uint8_t x = (uint8_t)cr->x, p = cr->p;

    cr->a = (uint16_t)((cr->a & 0xFF00) | WindowStep3437By3(cr->db, &x, &p));
    if (x != (uint8_t)cr->x)
        cr->x = x;
    set_p(cr, p);
}

void abi_WindowStep3437By3(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    WindowStep3437By3_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:FDA9 WindowEdgesStep
 * WHAT IT DOES: One step of the window edges in bank $7F: $340D counts up
 * to $1F; then by how $3433 compares with -$3437: equal, $3437 + 2 and
 * $3433 - 2; below, $3437 + 2 and $3433 - 3; above, $3437 + 3 and $3433 -
 * 2; then $3439 + 1 and $3435 - 3. When two or more of the steps have
 * stopped (X), the fade starts ($0D61 = $80, $0E3A = 1).
 * HOW: The data bank is $7F for the steps; X counts the stops. The above
 * case is not reached in the traces so far.
 * Entry: any widths; db0 = DB, *p = P with M and X already set. Exit: M
 * 8-bit, X 8-bit; *x = the stops (X); the data bank restored; the result is
 * A's low byte as the last step left it.
 */
uint8_t WindowEdgesStep(uint8_t db0, uint8_t *p, uint8_t *x)
{
    uint8_t v, m, a;
    uint32_t b = 0x7F0000;

    cyc(112);
    v = rd8(b | W(wram_340D));
    *p = (uint8_t)((*p & ~P_C) | (v >= 0x1F ? (cyc(38), P_C) : 0));
    if (v < 0x1F)
        { cyc(56); wr8(b | W(wram_340D), cyc_then(6, (uint8_t)(rd8(b | W(wram_340D)) + 1))); }
    *x = 0x00;
    cyc(40);
    v = (uint8_t)((rd8(b | W(wram_3437)) ^ 0xFF) + 1);
    cyc(54);
    m = rd8(b | W(wram_3433));
    *p = (uint8_t)((nz8(*p, (uint8_t)(v - m)) & ~P_C) | (v >= m ? P_C : (cyc(84), 0)));
    if (v == m) {
        (cyc(68), WindowStep3437(0x7F, x, p));
        (cyc(46), WindowStep3433By2(0x7F, x, p));
    } else if (v < m) {
        WindowStep3437(0x7F, x, p);
        (cyc(46), WindowStep3433By3(0x7F, x, p));
        cyc(22);                    /* BRA */
    } else {
        cyc(78);
        WindowStep3437By3(0x7F, x, p);
        (cyc(46), WindowStep3433By2(0x7F, x, p));
        cyc(22);                    /* BRA */
    }
    WindowStep3439(0x7F, x, p);
    a = (cyc(46), WindowStep3435(0x7F, x, p));
    *p = (uint8_t)((*p & ~P_C) | (*x >= 0x02 ? P_C : (cyc(38), 0)));
    if (*x >= 0x02) {
        cyc(80);
        wr8(W(fade_level_hi), 0x80);
        cyc(48);
        wr8(W(fade_out), 0x01);
        a = 0x01;
    }
    cyc(72);
    *p = nz8(*p, db0);
    return a;
}

void WindowEdgesStep_cr(Carried *cr)
{
    uint8_t x, p, a;

    r_sep(cr, P_M | P_X);
    p = cr->p;
    a = WindowEdgesStep(cr->db, &p, &x);
    cr->a = (uint16_t)((cr->a & 0xFF00) | a);
    cr->x = x;
    set_p(cr, p);
}

void abi_WindowEdgesStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    WindowEdgesStep_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

