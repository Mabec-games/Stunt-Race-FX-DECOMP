/*
 * camera.c: the race camera, a record in bank 0 that follows an object
 * (its target) with a lag set by mode bits.
 */
#include "machine.h"

void abi_Gsu8325(void);
void abi_Gsu82CE(void);
void abi_ScaleFiveThirtySeconds(void);

#define CAM(r, f) ((uint16_t)((f) + (r)))      /* a camera field, bank 0 */
#define CAM_TARGET 0x1F33u                      /* the followed object */
#define CAM_FLAGS 0x05FAu                       /* the camera's mode bits */
#define CAM_LAG 0x1F2Bu                         /* the height still to go */

/* CMP #$8000 / ROR A: a signed halving; *c = the bit shifted out. */
static uint16_t asr(uint16_t v, unsigned *c)
{
    *c = v & 1u;
    return (uint16_t)((v >> 1) | (v & 0x8000));
}

/* `n` signed halvings, then (BPL / ADC #0) the last bit out added back to
   a negative result: rounding toward zero. */
static uint16_t asr_round(uint16_t v, unsigned n)
{
    unsigned c = 0;

    cyc(22);
    while (n--)
        { cyc(38); v = asr(v, &c); }
    return (v & 0x8000) ? (cyc(18), (uint16_t)(v + c)) : v;
}

static uint16_t asr_n(uint16_t v, unsigned n)
{
    unsigned c;

    while (n--)
        { cyc(32); v = asr(v, &c); }
    return v;
}

/* CLC / ADC in 16 bits: the sum, with N, Z, C, V in *p. */
static uint16_t add16(uint16_t a, uint16_t m, uint8_t *p)
{
    unsigned s = (unsigned)a + m;
    uint16_t r = (uint16_t)s;

    *p = (uint8_t)((nz16(*p, r) & ~(P_C | P_V)) | (s > 0xFFFF ? P_C : 0) | ((~(a ^ m) & (a ^ r) & 0x8000) ? P_V : 0));
    return r;
}

/* An angle byte of the target (+$15 or +$14) as the high byte. */
static uint16_t angle_hi(uint32_t b, uint16_t y, uint16_t field)
{
    cyc(30);
    uint16_t v = rd16(b | (uint16_t)(field + y));

    return (uint16_t)((uint16_t)((v << 8) | (v >> 8)) & 0xFF00);
}

/*
 * @routine 02:DB71 CameraFollowStep
 * WHAT IT DOES: One frame of the race camera X (a record in bank 0): its
 * mode bits ($05FA, from +$00) and target ($1F33, from +$02) are taken;
 * with bit 1 it does nothing more. With bit 2 its speed lead (+$26-$2A) is
 * minus the target's speed (+$4A of its Super FX record) scaled by 1/2 and
 * 1/8, else 0. It copies +$1A-$1E to direct page $02, $08, $94; then sets
 * its turn (+$16) and pitch (+$14) from the target's angles +$15 and +$14:
 * at once when bits 10-12 are 0, otherwise easing the turn by the
 * difference (whole in mode 0, a half or a quarter in mode 1 by the speed
 * of the record at $70:247A, a quarter in mode 2, 5/64 or 5/128 in modes
 * 3 and 4 through ScaleFiveThirtySeconds) and copying it to $0660, and the
 * pitch to $065E (a quarter); with bit 8 all three are 0. Then the view
 * angles ($065E-$0662, plus +$35-$39, $065E negated) go to the Super FX
 * (Gsu8325), the nine words $0608-$0618 to $064C-$065C, and Gsu82CE runs.
 * Last, its position (+$08-$0C) follows the target's (+$0C-$10 plus $B7,
 * $B9, $BB): at once, or with bit 4 the height by a half or a quarter of
 * the difference, or with bit 3 all three by a quarter, an eighth and a
 * quarter. With bits 2 and 3 of +$42 of the record at $70:247A set and
 * bit 9 clear, the turn eases by half the difference straight away.
 * HOW: Signed halvings as CMP #$8000 / ROR A, most of them rounded toward
 * zero; camera fields through direct page (D = 0), the target's through
 * the data bank. Bits 3 and 8, modes 0 and 3, the bit-9 case and the
 * rounding of the speed lead are not reached in the traces so far.
 * Entry: X 16-bit, D = $0000; X = camera. Exit: X = camera; M 16-bit,
 * X 16-bit; with bit 1: A = 2, N, Z from it; otherwise A = the last
 * position word, N, Z, C, V from its add, Y as the last step left it.
 */
void abi_CameraFollowStep(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint16_t r = cpu.x, a, t, f, y, g, w;
    uint8_t p = cpu.p;
    unsigned i;

    cpu.p &= (uint8_t)~(P_M | P_X);
    cyc(104);
    t = rd16(CAM(r, 0x02));
    cyc(38);
    wr16(b | CAM_TARGET, t);
    cyc(58);
    wr16(b | CAM_FLAGS, cyc_then(24, rd16(CAM(r, 0x00))));
    cyc(24);
    a = (uint16_t)(rd16(b | CAM_FLAGS) & 0x0002);
    if (a) {
        cyc(150);
        cpu.a = a;
        cpu.p = (uint8_t)(nz16(cpu.p, a) & ~(P_M | P_X));
        return;
    }
    cyc(92);
    if (rd16(b | CAM_FLAGS) & 0x0004) {
        cyc(100);
        g = rd16(cyc_then(32, 0x7E2022 + rd16(b | CAM_TARGET)));
        cyc(46);
        a = rd16(0x70004A + g);
        if (a & 0x8000)
            { cyc(32); a = (uint16_t)(~a + 1); }
        a = asr_round(a, 1);
        cyc(74);
        wr16(0x000094, a);
        a = asr_round(a, 2);
        cyc(16);
        wr16(0x000008, a);
        cyc(16);
        wr16(0x000002, 0x0000);
        cyc(16);
        wr16(CAM(r, 0x26), cyc_then(60, (uint16_t)(~rd16(0x000002) + 1)));
        cyc(16);
        wr16(CAM(r, 0x28), cyc_then(60, (uint16_t)(~rd16(0x000008) + 1)));
        cyc(16);
        wr16(CAM(r, 0x2A), cyc_then(60, (uint16_t)(~rd16(0x000094) + 1)));
        cyc(44);                        /* BRA to .lead_done, LDA dp,X */
    } else {
        cyc(92);
        wr16(CAM(r, 0x26), 0x0000);
        cyc(22);
        wr16(CAM(r, 0x28), 0x0000);
        cyc(22);
        wr16(CAM(r, 0x2A), 0x0000);
        cyc(22);                        /* LDA dp,X */
    }
    wr16(0x000002, cyc_then(16, rd16(CAM(r, 0x1A))));
    cyc(22);
    wr16(0x000008, cyc_then(16, rd16(CAM(r, 0x1C))));
    cyc(22);
    wr16(0x000094, cyc_then(16, rd16(CAM(r, 0x1E))));
    cyc(24);
    f = rd16(b | CAM_FLAGS);
    if (f & 0x0100) {
        cyc(64);
        wr16(b | 0x065E, 0x0000);
        cyc(24);
        wr16(b | 0x0660, 0x0000);
        cyc(24);
        wr16(b | 0x0662, 0x0000);
        cyc(78);                        /* JMP .to_super_fx, PHX, LDA */
    } else if (!(f & 0x1C00)) {
        cyc(110);
        y = rd16(b | CAM_TARGET);
        cpu.y = y;
        a = (uint16_t)(~angle_hi(b, y, 0x2015) + 1);
        cyc(104);
        wr16(CAM(r, 0x16), a);
        cyc(24);
        wr16(b | 0x0660, a);
        a = asr_n(angle_hi(b, y, 0x2014), 2);
        cyc(78);
        wr16(CAM(r, 0x14), a);
        cyc(24);
        wr16(b | 0x065E, a);
        cyc(24);
        wr16(b | 0x0662, 0x0000);
        cyc(78);                        /* JMP .to_super_fx, PHX, LDA */
    } else {
        cyc(116);
        y = rd16(b | CAM_TARGET);
        cpu.y = y;
        cyc(62);
        g = rd16(0x70247A);
        cyc(46);
        if ((rd16(0x700042 + g) & 0x000C) && !(rd16(cyc_then(100, b | CAM_FLAGS)) & 0x0200)) {
            cyc(40);
            a = (uint16_t)(~angle_hi(b, y, 0x2015) + 1);
            cyc(118);
            a = (uint16_t)(a - rd16(CAM(r, 0x16)));
            a = asr_n(a, 1);
            cyc(42);
            a = (uint16_t)(a + rd16(CAM(r, 0x16)));
        } else {
            cyc(82);
            a = (uint16_t)(~angle_hi(b, y, 0x2015) + 1);
            cyc(118);
            a = (uint16_t)(a - rd16(CAM(r, 0x16)));
            cyc(24);
            wr16(b | CAM_LAG, a);
            cyc(24);
            switch (rd16(b | CAM_FLAGS) & 0x1C00) {
            case 0x0400:
                cyc(172);
                g = rd16(0x70247A);
                cyc(46);
                w = rd16(0x70006C + g);
                /* BPL (taken 22, or not and EOR, INC 54), PLX 36, CMP #
                   24, BPL (taken 22: one halving; not 16: two), LDA 24 */
                cyc((w & 0x8000) ? 54 : 22);
                if (w & 0x8000)
                    w = (uint16_t)(~w + 1);
                cyc(36 + 24 + (((w - 0x7EF4) & 0x8000) ? 16 : 22) + 24);
                a = asr_round(rd16(b | CAM_LAG), (w - 0x7EF4) & 0x8000 ? 2 : 1);
                break;
            case 0x0800:
                cyc(174);
                a = asr_round(rd16(b | CAM_LAG), 2);
                break;
            case 0x0C00:
                cyc(214);
                cpu.a = rd16(b | CAM_LAG);
                abi_ScaleFiveThirtySeconds();
                cyc(44);
                a = asr_round(cpu.a, 1);
                break;
            case 0x1000:
                cyc(254);
                cpu.a = rd16(b | CAM_LAG);
                abi_ScaleFiveThirtySeconds();
                cyc(22);
                a = asr_round(cpu.a, 2);
                break;
            default:
                cyc(248);
                a = rd16(b | CAM_LAG);
                break;
            }
            cyc(58);
            a = (uint16_t)(a + rd16(CAM(r, 0x16)));
        }
        cyc(22);
        wr16(CAM(r, 0x16), a);
        cyc(24);
        wr16(b | 0x0660, a);
        a = asr_round(angle_hi(b, y, 0x2014), 2);
        cyc(66);
        wr16(CAM(r, 0x14), a);
        cyc(24);
        wr16(b | 0x065E, a);
        cyc(24);
        wr16(b | 0x0662, 0x0000);
        cyc(54);                        /* falls into .to_super_fx: PHX, LDA */
    }
    a = rd16(b | 0x065E);
    cyc(36);
    a = (uint16_t)(a + rd16(CAM(r, 0x35)));
    cyc(62);
    wr16(b | 0x065E, (uint16_t)(~a + 1));
    cyc(24);
    a = rd16(b | 0x0660);
    cyc(36);
    wr16(b | 0x0660, cyc_then(24, (uint16_t)(a + rd16(CAM(r, 0x37)))));
    cyc(24);
    a = rd16(b | 0x0662);
    cyc(36);
    wr16(b | 0x0662, cyc_then(24, (uint16_t)(a + rd16(CAM(r, 0x39)))));
    cpu.p |= P_M;
    (cyc(84), abi_Gsu8325());
    cpu.p &= (uint8_t)~P_M;
    cyc(22);
    for (i = 0; i < 18; i += 2)
        { cyc(24); wr16(b | (uint16_t)(0x064C + i), cyc_then(24, rd16(b | (uint16_t)(0x0608 + i)))); }
    cyc(84);
    cpu.p |= P_M;
    abi_Gsu82CE();
    cpu.p &= (uint8_t)~(P_M | P_X);
    cpu.x = r;
    cyc(82);
    f = rd16(b | CAM_FLAGS);
    p = cpu.p;
    if (f & 0x0008) {
        y = cpu.y;
        cyc(100);
        a = rd16(b | (uint16_t)(0x200C + y));
        cyc(30);
        a = (uint16_t)(a + rd16(0x0000B7));
        cyc(36);
        a = asr_n((uint16_t)(a - rd16(CAM(r, 0x08))), 2);
        cyc(48);
        a = add16(a, rd16(CAM(r, 0x08)), &p);
        cyc(22);
        wr16(CAM(r, 0x08), a);
        cyc(30);
        a = rd16(b | (uint16_t)(0x200E + y));
        cyc(30);
        a = (uint16_t)(a + rd16(0x0000B9));
        cyc(36);
        a = asr_n((uint16_t)(a - rd16(CAM(r, 0x0A))), 3);
        cyc(54);
        a = add16(a, rd16(CAM(r, 0x0A)), &p);
        cyc(22);
        wr16(CAM(r, 0x0A), a);
        cyc(30);
        a = rd16(b | (uint16_t)(0x2010 + y));
        cyc(30);
        a = (uint16_t)(a + rd16(0x0000BB));
        cyc(36);
        a = asr_n((uint16_t)(a - rd16(CAM(r, 0x0C))), 2);
        cyc(48);
        a = add16(a, rd16(CAM(r, 0x0C)), &p);
        cyc(22);
        wr16(CAM(r, 0x0C), a);
    } else {
        cyc(116);
        y = rd16(b | CAM_TARGET);
        cpu.y = y;
        cyc(30);
        a = rd16(b | (uint16_t)(0x200C + y));
        cyc(30);
        a = add16(a, rd16(0x0000B7), &p);
        cyc(22);
        wr16(CAM(r, 0x08), a);
        cyc(30);
        a = rd16(b | (uint16_t)(0x200E + y));
        cyc(30);
        a = add16(a, rd16(0x0000B9), &p);
        if (f & 0x0010) {
            cyc(36);
            a = (uint16_t)(a - rd16(CAM(r, 0x0A)));
            a = asr_n(a, (a & 0x8000) ? 1 : 2);
            cyc(64);
            a = add16(a, rd16(CAM(r, 0x0A)), &p);
        }
        cyc(22);
        wr16(CAM(r, 0x0A), a);
        cyc(30);
        a = rd16(b | (uint16_t)(0x2010 + y));
        cyc(30);
        a = add16(a, rd16(0x0000BB), &p);
        cyc(22);
        wr16(CAM(r, 0x0C), a);
    }
    cyc(102);
    cpu.a = a;
    cpu.p = (uint8_t)(p & ~(P_M | P_X));
}
