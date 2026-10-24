/*
 * slide.c: a 16-bit value at $19F6 that slides toward a target while bit 7
 * of $0DA5 is set; its high byte is copied to $7F:3468.
 */
#include "machine.h"
#include "system_calls.h"
#include "carried.h"
#include "wram_names.h"

/*
 * @routine 7E:E0E8 SlideStep
 * WHAT IT DOES: Moves the sliding value one step toward its target and
 * publishes its high byte; ends the slide when the target is reached going
 * down.
 * HOW: Direction by the sign of $19FC: adds or subtracts the step $19F8.
 * Reaching the target exactly: going up, nothing is stored; going down, the
 * slide flag is cleared, $7F:3470 = $FFFF and the high byte published.
 * Otherwise the new value is stored and its high byte published.
 * Entry: any widths. Exit: A depends on the path (see the abi wrapper);
 * P unchanged.
 */
uint16_t SlideStep(uint16_t a)
{
    uint16_t v;

    cyc(148);
    if (!(rd8(W(slide_flags)) & 0x80))
        { cyc(38); return (uint16_t)(a & 0xFF00); }
    cyc(78);
    if (!(rd16(W(slide_direction)) & 0x8000))
        { cyc(40); v = (uint16_t)(rd16(W(slide_value)) + rd16(cyc_then(38, W(slide_step)))); }
    else
        { cyc(68); v = (uint16_t)(rd16(W(slide_value)) - rd16(cyc_then(38, W(slide_step)))); }
    cyc(24);
    if (v == rd16(W(slide_target))) {
        cyc(46);
        uint16_t dir = rd16(W(slide_direction));
        if (!(dir & 0x8000))
            { cyc(22); return dir; }
        cyc(70);
        wr8(W(slide_flags), cyc_then(48, rd8(W(slide_flags)) & 0x7F));
        cyc(78);
        wr16(W(slide_done), 0xFFFF);
        a = 0xFFFF;
    } else {
        cyc(40);
        wr16(W(slide_value), v);
        a = v;
        cyc(24);                        /* JMP */
    }
    {
        uint8_t hi = (cyc(46), rd8(W(wram_19F7)));      /* SEP, LDA */
        cyc(32);
        wr8(W(slide_out), hi);
        return (uint16_t)((a & 0xFF00) | hi);
    }
}

/* SlideStep on carried registers. */
void SlideStep_cr(Carried *cr)
{
    cr->a = SlideStep(cr->a);
    cyc(100);
}

void abi_SlideStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    SlideStep_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* The flags SlideOutWobble leaves besides N and Z (those come from the
   data bank it pulls): C from the last compare or add, V from the last add
   (-1: none made, the caller's). For the adapter. */
static int wobble_c, wobble_v;

/* CLC / ADC (8-bit, binary): the sum, C and V in wobble_c, wobble_v. */
static uint8_t wobble_add(uint8_t a, uint8_t m)
{
    unsigned r = (unsigned)a + m;

    wobble_c = r > 0xFF;
    wobble_v = ((~(a ^ m) & (a ^ r) & 0x80) != 0);
    return (uint8_t)r;
}

/*
 * @routine 7E:E190 SlideOutWobble
 * WHAT IT DOES: Once the sliding value's high byte ($19F7) is $0F, every
 * second call moves the published byte $7F:3468 by $1A04 and turns $1A04
 * round when that byte reaches $0F or $0B: a wobble between the two.
 * Nothing reaches it (no JSL, jump or table; it follows DataBankTest's
 * RTL), so a direct call is its only way in.
 * HOW: SEP #$20; $1A05 counts calls to 2 and is cleared; CLC, ADC; EOR #$FF
 * and INC to negate. Ends PLB, RTL: the data bank its caller pushed is
 * pulled.
 * Entry: the caller's data bank pushed above the return (PHB); data bank
 * with $19F7, $1A04, $1A05. Exit: A low byte = the last value read or
 * made; M 8-bit; data bank pulled, N, Z from it; C, V from the last compare
 * or add. In C: the return value is A's low byte; *pulled the data bank
 * pulled; the flags are the adapter's.
 */
uint8_t SlideOutWobble(uint8_t bank, uint8_t *pulled)
{
    uint8_t a, m;

    wobble_v = -1;
    cyc(46);
    a = rd8(0x7E0000 | W(wram_19F7));
    wobble_c = a >= 0x0F;
    if (!wobble_c)
        cyc(38);
    if (a == 0x0F) {
        cyc(56);
        a = wobble_add(rd8(0x7E0000 | W(slide_wobble_count)), 0x01);
        cyc(54);
        wr8(0x7E0000 | W(slide_wobble_count), a);
        wobble_c = a >= 0x02;
        if (!wobble_c)
            cyc(38);
        if (a == 0x02) {
            cyc(56);
            wr8(0x7E0000 | W(slide_wobble_count), 0x00);
            cyc(32);
            a = rd8(W(slide_out));
            cyc(38);
            a = wobble_add(a, rd8(0x7E0000 | W(wram_1A04)));
            cyc(32);
            wr8(W(slide_out), a);
            m = a == 0x0F ? 0x0F : (cyc(32), 0x0B);
            wobble_c = a >= m;
            if (wobble_c)
                cyc(56);
            if (a == 0x0F || a == 0x0B) {
                cyc(6);
                a = (uint8_t)((rd8(0x7E0000 | W(wram_1A04)) ^ 0xFF) + 1);
                cyc(54);
                wr8(0x7E0000 | W(wram_1A04), a);
            }
        }
    }
    cyc(20);
    *pulled = rd8((uint16_t)(cpu.s + 1));
    cyc(44);                                    /* RTL */
    return a;
}

/* SlideOutWobble on carried registers. */
void SlideOutWobble_cr(Carried *cr)
{
    uint8_t pulled, a = SlideOutWobble(cr->db, &pulled);

    set_p(cr, (uint8_t)(cr->p | (P_M)));
    set_p(cr, (uint8_t)((cr->p & ~P_C) | (wobble_c ? P_C : 0)));
    if (wobble_v >= 0)
        set_p(cr, (uint8_t)((cr->p & ~P_V) | (wobble_v ? P_V : 0)));
    cr->a = (uint16_t)((cr->a & 0xFF00) | a);
    cr->db = pulled;
    set_p(cr, nz8(cr->p, pulled));
}

void abi_SlideOutWobble(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    SlideOutWobble_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}
