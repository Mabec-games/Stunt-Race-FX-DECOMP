/*
 * slide.c: a 16-bit value at $19F6 that slides toward a target while bit 7
 * of $0DA5 is set; its high byte is copied to $7F:3468.
 */
#include "machine.h"
#include "cpu_ops.h"

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
static uint16_t slide_step(uint16_t a)
{
    uint16_t v;

    cyc(256);
    if (!(rd8(0x000DA5) & 0x80))
        { cyc(38); return (uint16_t)(a & 0xFF00); }
    cyc(78);
    if (!(rd16(0x0019FC) & 0x8000))
        { cyc(40); v = (uint16_t)(rd16(0x0019F6) + rd16(cyc_then(38, 0x0019F8))); }
    else
        { cyc(68); v = (uint16_t)(rd16(0x0019F6) - rd16(cyc_then(38, 0x0019F8))); }
    cyc(24);
    if (v == rd16(0x0019FA)) {
        cyc(46);
        uint16_t dir = rd16(0x0019FC);
        if (!(dir & 0x8000))
            { cyc(22); return dir; }
        cyc(70);
        wr8(0x000DA5, cyc_then(48, rd8(0x000DA5) & 0x7F));
        cyc(78);
        wr16(0x7F3470, 0xFFFF);
        a = 0xFFFF;
    } else {
        cyc(40);
        wr16(0x0019F6, v);
        a = v;
    }
    {
        uint8_t hi = rd8(0x0019F7);
        cyc(32);
        wr8(0x7F3468, hi);
        return (uint16_t)((a & 0xFF00) | hi);
    }
}

void SlideStep(void)
{
    slide_step(0);
}

void abi_SlideStep(void)
{
    cpu.a = slide_step(cpu.a);
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
 * or add.
 */
void abi_SlideOutWobble(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint8_t a, m;

    cpu.p |= P_M;
    cyc(46);
    a = rd8(b | 0x19F7);
    cpu.p = (uint8_t)((nz8(cpu.p, (uint8_t)(a - 0x0F)) & ~P_C) | (a >= 0x0F ? P_C : (cyc(38), 0)));
    if (a == 0x0F) {
        cpu.p &= (uint8_t)~P_C;
        cyc(56);
        a = op_adc8(rd8(b | 0x1A05), 0x01, &cpu.p);
        cyc(54);
        wr8(b | 0x1A05, a);
        cpu.p = (uint8_t)((nz8(cpu.p, (uint8_t)(a - 0x02)) & ~P_C) | (a >= 0x02 ? P_C : (cyc(38), 0)));
        if (a == 0x02) {
            cyc(56);
            wr8(b | 0x1A05, 0x00);
            cyc(32);
            a = rd8(0x7F3468);
            cpu.p &= (uint8_t)~P_C;
            cyc(38);
            a = op_adc8(a, rd8(b | 0x1A04), &cpu.p);
            cyc(32);
            wr8(0x7F3468, a);
            m = a == 0x0F ? 0x0F : (cyc(32), 0x0B);
            cpu.p = (uint8_t)((nz8(cpu.p, (uint8_t)(a - m)) & ~P_C) | (a >= m ? (cyc(56), P_C) : 0));
            if (a == 0x0F || a == 0x0B) {
                cyc(6);
                a = (uint8_t)((rd8(b | 0x1A04) ^ 0xFF) + 1);
                cpu.p = nz8(cpu.p, a);
                cyc(54);
                wr8(b | 0x1A04, a);
            }
        }
    }
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | a);
    cyc(20);
    cpu.db = rd8((uint16_t)(cpu.s + 1));
    cpu.p = nz8(cpu.p, cpu.db);
}
