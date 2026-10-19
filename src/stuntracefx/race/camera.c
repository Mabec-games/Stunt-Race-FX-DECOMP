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
    t = rd16(CAM(r, 0x02));
    wr16(b | CAM_TARGET, t);
    wr16(b | CAM_FLAGS, rd16(CAM(r, 0x00)));
    a = (uint16_t)(rd16(b | CAM_FLAGS) & 0x0002);
    if (a) {
        cpu.a = a;
        cpu.p = (uint8_t)(nz16(cpu.p, a) & ~(P_M | P_X));
        return;
    }
    if (rd16(b | CAM_FLAGS) & 0x0004) {
        g = rd16(0x7E2022 + rd16(b | CAM_TARGET));
        a = rd16(0x70004A + g);
        if (a & 0x8000)
            a = (uint16_t)(~a + 1);
        a = asr_round(a, 1);
        wr16(0x000094, a);
        a = asr_round(a, 2);
        wr16(0x000008, a);
        wr16(0x000002, 0x0000);
        wr16(CAM(r, 0x26), (uint16_t)(~rd16(0x000002) + 1));
        wr16(CAM(r, 0x28), (uint16_t)(~rd16(0x000008) + 1));
        wr16(CAM(r, 0x2A), (uint16_t)(~rd16(0x000094) + 1));
    } else {
        wr16(CAM(r, 0x26), 0x0000);
        wr16(CAM(r, 0x28), 0x0000);
        wr16(CAM(r, 0x2A), 0x0000);
    }
    wr16(0x000002, rd16(CAM(r, 0x1A)));
    wr16(0x000008, rd16(CAM(r, 0x1C)));
    wr16(0x000094, rd16(CAM(r, 0x1E)));
    f = rd16(b | CAM_FLAGS);
    if (f & 0x0100) {
        wr16(b | 0x065E, 0x0000);
        wr16(b | 0x0660, 0x0000);
        wr16(b | 0x0662, 0x0000);
    } else if (!(f & 0x1C00)) {
        y = rd16(b | CAM_TARGET);
        cpu.y = y;
        a = (uint16_t)(~angle_hi(b, y, 0x2015) + 1);
        wr16(CAM(r, 0x16), a);
        wr16(b | 0x0660, a);
        a = asr_n(angle_hi(b, y, 0x2014), 2);
        wr16(CAM(r, 0x14), a);
        wr16(b | 0x065E, a);
        wr16(b | 0x0662, 0x0000);
    } else {
        y = rd16(b | CAM_TARGET);
        cpu.y = y;
        g = rd16(0x70247A);
        if ((rd16(0x700042 + g) & 0x000C) && !(rd16(b | CAM_FLAGS) & 0x0200)) {
            a = (uint16_t)(~angle_hi(b, y, 0x2015) + 1);
            a = (uint16_t)(a - rd16(CAM(r, 0x16)));
            a = asr_n(a, 1);
            a = (uint16_t)(a + rd16(CAM(r, 0x16)));
        } else {
            a = (uint16_t)(~angle_hi(b, y, 0x2015) + 1);
            a = (uint16_t)(a - rd16(CAM(r, 0x16)));
            wr16(b | CAM_LAG, a);
            switch (rd16(b | CAM_FLAGS) & 0x1C00) {
            case 0x0400:
                g = rd16(0x70247A);
                w = rd16(0x70006C + g);
                if (w & 0x8000)
                    w = (uint16_t)(~w + 1);
                a = asr_round(rd16(b | CAM_LAG), (w - 0x7EF4) & 0x8000 ? 2 : 1);
                break;
            case 0x0800:
                a = asr_round(rd16(b | CAM_LAG), 2);
                break;
            case 0x0C00:
                cpu.a = rd16(b | CAM_LAG);
                abi_ScaleFiveThirtySeconds();
                a = asr_round(cpu.a, 1);
                break;
            case 0x1000:
                cpu.a = rd16(b | CAM_LAG);
                abi_ScaleFiveThirtySeconds();
                a = asr_round(cpu.a, 2);
                break;
            default:
                a = rd16(b | CAM_LAG);
                break;
            }
            a = (uint16_t)(a + rd16(CAM(r, 0x16)));
        }
        wr16(CAM(r, 0x16), a);
        wr16(b | 0x0660, a);
        a = asr_round(angle_hi(b, y, 0x2014), 2);
        wr16(CAM(r, 0x14), a);
        wr16(b | 0x065E, a);
        wr16(b | 0x0662, 0x0000);
    }
    a = rd16(b | 0x065E);
    a = (uint16_t)(a + rd16(CAM(r, 0x35)));
    wr16(b | 0x065E, (uint16_t)(~a + 1));
    a = rd16(b | 0x0660);
    wr16(b | 0x0660, (uint16_t)(a + rd16(CAM(r, 0x37))));
    a = rd16(b | 0x0662);
    wr16(b | 0x0662, (uint16_t)(a + rd16(CAM(r, 0x39))));
    cpu.p |= P_M;
    abi_Gsu8325();
    cpu.p &= (uint8_t)~P_M;
    for (i = 0; i < 18; i += 2)
        wr16(b | (uint16_t)(0x064C + i), rd16(b | (uint16_t)(0x0608 + i)));
    cpu.p |= P_M;
    abi_Gsu82CE();
    cpu.p &= (uint8_t)~(P_M | P_X);
    cpu.x = r;
    f = rd16(b | CAM_FLAGS);
    p = cpu.p;
    if (f & 0x0008) {
        y = cpu.y;
        a = rd16(b | (uint16_t)(0x200C + y));
        a = (uint16_t)(a + rd16(0x0000B7));
        a = asr_n((uint16_t)(a - rd16(CAM(r, 0x08))), 2);
        a = add16(a, rd16(CAM(r, 0x08)), &p);
        wr16(CAM(r, 0x08), a);
        a = rd16(b | (uint16_t)(0x200E + y));
        a = (uint16_t)(a + rd16(0x0000B9));
        a = asr_n((uint16_t)(a - rd16(CAM(r, 0x0A))), 3);
        a = add16(a, rd16(CAM(r, 0x0A)), &p);
        wr16(CAM(r, 0x0A), a);
        a = rd16(b | (uint16_t)(0x2010 + y));
        a = (uint16_t)(a + rd16(0x0000BB));
        a = asr_n((uint16_t)(a - rd16(CAM(r, 0x0C))), 2);
        a = add16(a, rd16(CAM(r, 0x0C)), &p);
        wr16(CAM(r, 0x0C), a);
    } else {
        y = rd16(b | CAM_TARGET);
        cpu.y = y;
        a = rd16(b | (uint16_t)(0x200C + y));
        a = add16(a, rd16(0x0000B7), &p);
        wr16(CAM(r, 0x08), a);
        a = rd16(b | (uint16_t)(0x200E + y));
        a = add16(a, rd16(0x0000B9), &p);
        if (f & 0x0010) {
            a = (uint16_t)(a - rd16(CAM(r, 0x0A)));
            a = asr_n(a, (a & 0x8000) ? 1 : 2);
            a = add16(a, rd16(CAM(r, 0x0A)), &p);
        }
        wr16(CAM(r, 0x0A), a);
        a = rd16(b | (uint16_t)(0x2010 + y));
        a = add16(a, rd16(0x0000BB), &p);
        wr16(CAM(r, 0x0C), a);
    }
    cpu.a = a;
    cpu.p = (uint8_t)(p & ~(P_M | P_X));
}
