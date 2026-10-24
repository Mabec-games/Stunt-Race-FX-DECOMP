/*
 * programs_vector.c: two more S-CPU wrappers of Super FX programs on the
 * vector in direct page $02, $08, $94, beside those in programs.c. No play
 * runs them; unit_gsu_vector calls them.
 */
#include "machine.h"
#include "cpu_ops.h"
#include "gsu_programs_calls.h"
#include "carried.h"

uint8_t GsuRun(uint8_t bank, uint16_t start);

/*
 * @routine 03:AF9D Gsu8277
 * WHAT IT DOES: Runs Super FX program $01:8277 on the vector in direct
 * page $02, $08, $94 and puts its three results in direct page $B7, $B9,
 * $BB.
 * HOW: As Gsu82CE: inputs to $70:0068, $70:002C, $70:002E; GsuRun; results
 * from $70:0026, $70:0028, $70:002A. X, Y and P saved and restored.
 * Entry: any widths, D = $0000. Exit: A = the third result (16 bits); X, Y
 * unchanged; P unchanged but N, Z, which the final pull of X sets.
 */
uint16_t Gsu8277(int x8)
{
    uint16_t v;
    unsigned i;

    cyc(x8 ? 104 : 120);                /* PHP, REP, PHX, PHY, the vector's first load */
    v = rd16(0x000002);
    cyc(32);
    wr16(0x700068, v);
    cyc(16);
    v = rd16(0x000008);
    cyc(32);
    wr16(0x70002C, v);
    cyc(16);
    v = rd16(0x000094);
    cyc(32);
    wr16(0x70002E, v);
    cyc(124);
    GsuRun(0x01, 0x8277);
    cyc(94);
    for (i = 0; i < 3; i++) {
        cyc(32);
        v = rd16(0x700026 + 2 * i);
        cyc(16);
        wr16(0x0000B7 + 2 * i, v);
    }
    cyc(128);
    if (!x8)
        cyc(16);                        /* the 16-bit PLX */
    return v;
}

/* Gsu8277 on carried registers. */
void Gsu8277_cr(Carried *cr)
{
    int x8 = (cr->p & P_X) != 0;

    cr->a = Gsu8277(x8);
    set_p(cr, x8 ? nz8(cr->p, (uint8_t)cr->x) : nz16(cr->p, cr->x));
}

void abi_Gsu8277(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    Gsu8277_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:C9D2 VectorGsu81E3
 * WHAT IT DOES: Puts direct page $04 - $02 in $02 and $05F2 - $94 in $08,
 * then runs Gsu81E3Long: ObjectGsu81E3 for a point in $02, $94 instead of
 * an object.
 * HOW: 16-bit subtracts, $05F2 through the data bank; X and Y saved and
 * restored.
 * Entry: X 16-bit, D = $0000. Exit: A as Gsu81E3Long leaves it; X, Y
 * unchanged; M 16-bit; N, Z from X.
 */
static uint8_t vector_cv;               /* the adapter's: C and V of the second subtract */

uint16_t VectorGsu81E3(uint8_t bank)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t u, v, r;

    cyc(98);
    u = rd16(0x000004);
    cyc(30);
    v = rd16(0x000002);
    cyc(16);
    wr16(0x000002, (uint16_t)(u - v));
    cyc(24);
    u = rd16(b | 0x05F2);
    cyc(30);
    v = rd16(0x000094);
    r = (uint16_t)(u - v);
    vector_cv = (uint8_t)((u >= v ? P_C : 0) | (((u ^ v) & (u ^ r) & 0x8000) ? P_V : 0));
    cyc(16);
    wr16(0x000008, r);
    r = (cyc(62), Gsu81E3Long());
    cyc(138);                           /* REP, PLY, PLX, RTL */
    return r;
}

/* VectorGsu81E3 on carried registers. */
void VectorGsu81E3_cr(Carried *cr)
{
    cr->a = VectorGsu81E3(cr->db);
    set_p(cr, nz16((uint8_t)((cr->p & ~(P_M | P_C | P_V)) | vector_cv), cr->x));
}

void abi_VectorGsu81E3(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    VectorGsu81E3_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* ADC and SBC (8-bit, binary: D is clear here) with the carry in *p; N,
   Z, C, V to *p. The SBC's 16 clocks are op_sbc8's stated time. */
static uint8_t adc8(uint8_t a, uint8_t m, uint8_t *p)
{
    unsigned r = (unsigned)a + m + (*p & P_C);

    *p = (uint8_t)((*p & ~(P_V | P_C)) | ((~(a ^ m) & (a ^ r) & 0x80) ? P_V : 0) | (r > 0xFF ? P_C : 0));
    *p = nz8(*p, (uint8_t)r);
    return (uint8_t)r;
}

static uint8_t sbc8(uint8_t a, uint8_t m, uint8_t *p)
{
    cyc(16);
    return adc8(a, (uint8_t)~m, p);
}

/* 8-bit shifts and compares as the 65816 sets N, Z and C. */
static uint8_t lsr8(uint8_t v, uint8_t *p)
{
    *p = (uint8_t)((*p & ~P_C) | (v & 1));
    v >>= 1;
    *p = nz8(*p, v);
    return v;
}

static uint8_t asl8(uint8_t v, uint8_t *p)
{
    *p = (uint8_t)((*p & ~P_C) | (v >> 7));
    v = (uint8_t)(v << 1);
    *p = nz8(*p, v);
    return v;
}

static uint8_t ror8(uint8_t v, uint8_t *p)
{
    uint8_t c = (uint8_t)(*p & P_C);

    *p = (uint8_t)((*p & ~P_C) | (v & 1));
    v = (uint8_t)(v >> 1 | (c ? (cyc(6), 0x80) : (cyc(6), 0)));
    *p = nz8(*p, v);
    return v;
}

static uint8_t rol8(uint8_t v, uint8_t *p)
{
    uint8_t c = (uint8_t)(*p & P_C);

    *p = (uint8_t)((*p & ~P_C) | (v >> 7));
    v = (uint8_t)(v << 1 | (c ? 1 : 0));
    *p = nz8(*p, v);
    return v;
}

static void cmp8(uint8_t a, uint8_t m, uint8_t *p)
{
    *p = (uint8_t)((nz8(*p, (uint8_t)(a - m)) & ~P_C) | (a >= m ? P_C : 0));
}

/* Read-modify-write shifts of a direct page byte (D = $0000). */
static void ror_dp(uint16_t a, uint8_t *p)
{
    cyc(60);
    uint8_t v = rd8(a);

    wr8(a, ror8(v, p));
}

static void rol_dp(uint16_t a, uint8_t *p)
{
    cyc(62);
    uint8_t v = rd8(a);

    wr8(a, cyc_then(6, rol8(v, p)));
}

static void lsr_dp(uint16_t a, uint8_t *p)
{
    cyc(38);
    uint8_t v = rd8(a);

    wr8(a, cyc_then(6, lsr8(v, p)));
}

/*
 * @routine 03:B0CC ScaledOffset
 * WHAT IT DOES: Sets the word at direct page $8C to the word at $CE plus
 * the signed word at $88 times 256 over the word at $8A (only its top
 * bits count), or to $CE when $88 is 0: a scaled offset from a base,
 * worked with log and exponent tables. Nothing in the game calls it.
 * HOW: $8A:$8B and the magnitude of $88:$89 are each brought to one byte
 * by shifting right (X counts the shifts; $8B's count is kept at $96);
 * a byte $88 below $80 is normalised by shifting left. The quotient's
 * byte is the difference of the two bytes' logs from the table at $8368
 * in the data bank, and its value comes from the tables at $8268 (high
 * byte) and $8168 (low byte), shifted by the difference of the counts.
 * The sign of $89 (kept at $8F) picks add or subtract. Work bytes $88,
 * $8A, $8E, $8F and $96 are left changed. 8-bit throughout.
 * Entry: M 8-bit, X 8-bit, D = $0000; data bank $03. Exit: A = $8D's new
 * value (or $CF's when $88 was 0); X = the shift count left; N, Z, C, V
 * from the last add or subtract (from $CF's load when $88 was 0).
 */
void ScaledOffset(uint8_t bank, uint8_t *ra, uint8_t *rx, uint8_t *p)
{
    uint32_t b = (uint32_t)bank << 16;
    uint8_t a, x = 0, v;
    uint32_t pre = 40;                  /* the JMP out of the shift loop, to the LDA */

    *p = nz8(*p, x);
    cyc(32);
    a = rd8(0x00008B);
    *p = nz8(*p, a);
    if (a) {
        cyc(10);
        x++;
        a = lsr8(a, p);
        while (a) {
            cyc(6);
            ror_dp(0x8A, p);
            x++;
            a = lsr8(a, p);
        }
        ror_dp(0x8A, p);
    } else { cyc(22); }
    cyc(16);
    wr8(0x000096, x);
    x = 0;
    cyc(32);
    a = rd8(0x000089);
    cyc(16);
    wr8(0x00008F, a);
    *p = nz8(*p, a);
    if (a & 0x80) {
        *p |= P_C;
        cyc(62);
        v = rd8(0x000088);
        a = sbc8(0x00, v, p);
        wr8(0x000088, a);
        cyc(32);
        v = rd8(0x000089);
        a = sbc8(0x00, v, p);
        wr8(0x000089, a);
    } else { cyc(22); }
    cyc(16);
    if (a) {
        int again = 0;

        x++;
        a = lsr8(a, p);
        while (a) {
            if (again)
                cyc(6);                 /* BNE taken, not BEQ not taken */
            again = 1;
            ror_dp(0x88, p);
            x++;
            a = lsr8(a, p);
        }
        if (!again)
            cyc(6);                     /* BEQ taken */
        ror_dp(0x88, p);
    } else {
        cyc(22);
        a = rd8(0x000088);
        *p = nz8(*p, a);
        pre = 38;                       /* BMI taken */
        if (!(a & 0x80)) {
            if (!a) {
                cyc(48);
                a = rd8(0x0000CE);
                cyc(16);
                wr8(0x00008C, a);
                cyc(16);
                a = rd8(0x0000CF);
                cyc(16);
                wr8(0x00008D, a);
                *p = nz8(*p, a);
                cyc(24);                /* JMP */
                *ra = a;
                *rx = x;
                return;
            }
            do {
                x--;
                a = asl8(a, p);
            } while (!(a & 0x80));
            cyc(148);
            wr8(0x000088, a);
            pre = 16;
        }
    }
    cyc(pre);
    a = rd8(0x00008A);
    cyc(16);
    v = rd8(0x000088);
    cmp8(a, v, p);
    if (*p & P_C) {
        do {
            x--;
            a = lsr8(a, p);
            cyc(60);
            v = rd8(0x000088);
            cmp8(a, v, p);
        } while (*p & P_C);
        cyc(32);
        wr8(0x00008A, a);
    } else { cyc(22); }
    *p |= P_C;
    cyc(44);
    v = rd8(0x000096);
    a = sbc8(x, v, p);
    wr8(0x00008E, a);
    cyc(16);
    x = rd8(0x000088);
    cyc(48 + (x > 0x97 ? 6 : 0));       /* LDA table,x (a page crossing costs 6), LDX */
    a = rd8(b | (uint16_t)(0x8368 + x));
    x = rd8(0x00008A);
    *p |= P_C;
    cyc(92 + (x > 0x97 ? 6 : 0));
    v = rd8(b | (uint16_t)(0x8368 + x));
    a = sbc8(a, v, p);
    x = a;
    cyc(x > 0x97 ? 6 : 0);
    a = rd8(b | (uint16_t)(0x8268 + x));
    wr8(0x00008D, a);
    cyc(48 + (x > 0x97 ? 6 : 0));
    a = rd8(b | (uint16_t)(0x8168 + x));
    x = rd8(0x00008E);
    if (x & 0x80) {
        do {
            lsr_dp(0x8D, p);
            cyc(22);
            a = ror8(a, p);
            x++;
        } while (x);
    } else {
        if (x) {
            int again = 0;

            while (x) {
                if (again)
                    cyc(4);             /* DEX, BNE taken */
                again = 1;
                a = asl8(a, p);
                rol_dp(0x8D, p);
                x--;
            }
            cyc(38);                    /* DEX, BNE, JMP */
        } else { cyc(22); }
    }
    cyc(32);
    wr8(0x00008C, a);
    cyc(16);
    v = rd8(0x00008F);
    *p = (uint8_t)((*p & ~(P_N | P_V | P_Z)) | (v & 0xC0) | ((a & v) ? 0 : P_Z));
    if (v & 0x80) {
        *p |= P_C;
        cyc(46);
        a = rd8(0x0000CE);
        cyc(16);
        v = rd8(0x00008C);
        a = sbc8(a, v, p);
        wr8(0x00008C, a);
        cyc(16);
        a = rd8(0x0000CF);
        cyc(16);
        v = rd8(0x00008D);
        a = sbc8(a, v, p);
        wr8(0x00008D, a);
        cyc(24);                        /* JMP */
    } else {
        *p &= (uint8_t)~P_C;
        cyc(52);
        a = rd8(0x00008C);
        cyc(16);
        v = rd8(0x0000CE);
        a = adc8(a, v, p);
        cyc(16);
        wr8(0x00008C, a);
        cyc(16);
        a = rd8(0x00008D);
        cyc(16);
        v = rd8(0x0000CF);
        a = adc8(a, v, p);
        cyc(16);
        wr8(0x00008D, a);
    }
    *ra = a;
    *rx = x;
}

/* ScaledOffset on carried registers. */
void ScaledOffset_cr(Carried *cr)
{
    uint8_t a = (uint8_t)cr->a, x = (uint8_t)cr->x;

    ScaledOffset(cr->db, &a, &x, &cr->p);
    cr->a = (uint16_t)((cr->a & 0xFF00) | a);
    cr->x = x;
    cyc(44);                            /* RTL */
}

void abi_ScaledOffset(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ScaledOffset_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:B079 ScaledOffsetPair
 * WHAT IT DOES: Two ScaledOffsets from the base $CE over the divisor in
 * direct page $94: of $02 into $76, and of $08 into $78, the second then
 * plus $D0 and minus $CE. Nothing in the game calls it.
 * HOW: Byte copies of the inputs to $88-$8B and of the results from $8C,
 * 8-bit adds and subtracts.
 * Entry: M 8-bit, X 8-bit, D = $0000; data bank $03. Exit: A = $79's new
 * value; X as the second ScaledOffset left it; N, Z, C, V from the last
 * subtract.
 */
void ScaledOffsetPair(uint8_t bank, uint8_t *ra, uint8_t *rx, uint8_t *p)
{
    uint8_t a, v;
    unsigned i;

    for (i = 0; i < 2; i++) {
        static const uint16_t in[2] = {0x02, 0x08}, out[2] = {0x76, 0x78};

        cyc(16);
        a = rd8(in[i]);
        cyc(16);
        wr8(0x000088, a);
        cyc(16);
        a = rd8((uint16_t)(in[i] + 1));
        cyc(16);
        wr8(0x000089, a);
        cyc(16);
        a = rd8(0x000094);
        cyc(16);
        wr8(0x00008A, a);
        cyc(16);
        a = rd8(0x000095);
        cyc(16);
        wr8(0x00008B, a);
        (cyc(62), ScaledOffset(bank, &a, rx, p));
        cyc(44);                        /* its RTL */
        cyc(16);
        a = rd8(0x00008C);
        cyc(16);
        wr8(out[i], a);
        cyc(16);
        a = rd8(0x00008D);
        cyc(16);
        wr8((uint16_t)(out[i] + 1), a);
    }
    *p &= (uint8_t)~P_C;
    cyc(30);
    a = rd8(0x0000D0);
    cyc(16);
    v = rd8(0x000078);
    a = adc8(a, v, p);
    cyc(16);
    wr8(0x000078, a);
    cyc(16);
    a = rd8(0x0000D1);
    cyc(16);
    v = rd8(0x000079);
    a = adc8(a, v, p);
    cyc(16);
    wr8(0x000079, a);
    *p |= P_C;
    cyc(30);
    a = rd8(0x000078);
    cyc(16);
    v = rd8(0x0000CE);
    a = sbc8(a, v, p);
    wr8(0x000078, a);
    cyc(16);
    a = rd8(0x000079);
    cyc(16);
    v = rd8(0x0000CF);
    a = sbc8(a, v, p);
    wr8(0x000079, a);
    *ra = a;
    cyc(44);                            /* RTL */
}

/* ScaledOffsetPair on carried registers. */
void ScaledOffsetPair_cr(Carried *cr)
{
    uint8_t a, x = (uint8_t)cr->x;

    ScaledOffsetPair(cr->db, &a, &x, &cr->p);
    cr->a = (uint16_t)((cr->a & 0xFF00) | a);
    cr->x = x;
}

void abi_ScaledOffsetPair(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ScaledOffsetPair_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* LDA / CMP #$8000 / ROR seven times: an arithmetic shift right by 7. */
static uint16_t asr7(uint16_t w, uint8_t *p)
{
    unsigned i;

    for (i = 0; i < 7; i++) {
        uint16_t c = (w >= 0x8000) ? 0x8000 : 0;

        *p = (uint8_t)((*p & ~P_C) | (w & 1));
        w = (uint16_t)(c | w >> 1);
    }
    *p = nz16(*p, w);
    return w;
}

/*
 * @routine 03:B17E ViewFromAnglesGsu8277
 * WHAT IT DOES: Builds a view from the angle bytes in direct page $C5,
 * $C7, $C9: the vector ($FF - $C5's low byte - $780, -$C7, $FF - $C9's
 * low byte - $780) into $02, $08, $94, the high bytes of $C5 and $C9 into
 * $067C and $067D; runs Gsu8277 on it and takes its results back into
 * $02, $08, $94; the nine words at $064C over 128 (kept to the sign) into
 * $066A-$067A; then puts the vector, six of those words and the two
 * angle bytes in Super FX RAM: $70:0068, $002C, $002E, $0154-$015E,
 * $0162, $0164. Nothing in the game calls it.
 * HOW: 16-bit direct page and work RAM (through the data bank), long
 * stores; the division is CMP #$8000, ROR seven times.
 * Entry: D = $0000. Exit: M 8-bit, X 8-bit (X and Y high bytes 0); A =
 * $067B and $067D (high, low); N, Z from $067D; C = bit 6 of $065C.
 */
static uint8_t view_c;                  /* the adapter's: C from the last division */

uint16_t ViewFromAnglesGsu8277(uint8_t bank)
{
    uint32_t b = (uint32_t)bank << 16;
    static const uint16_t gsu_words[6] = {0x066A, 0x066C, 0x066E, 0x0676, 0x0678, 0x067A};
    uint16_t w;
    uint8_t p = 0;
    unsigned i;

    cyc(22);                            /* REP #$30 */
    cyc(16);
    w = rd16(0x0000C5);
    w = (uint16_t)((w & 0x00FF) ^ 0x00FF);
    cyc(102);
    wr16(0x000002, (uint16_t)(w - 0x0780));
    cyc(16);
    w = rd16(0x0000C9);
    w = (uint16_t)((w & 0x00FF) ^ 0x00FF);
    cyc(102);
    wr16(0x000094, (uint16_t)(w - 0x0780));
    cyc(54);
    w = rd16(0x0000C7);
    cyc(16);
    wr16(0x000008, (uint16_t)(0 - w));
    cyc(16);
    w = rd16(0x0000C5);
    cyc(182);
    wr8(b | 0x067C, (uint8_t)(w >> 8));
    cyc(38);
    w = rd16(0x0000C9);
    cyc(182);
    wr8(b | 0x067D, (uint8_t)(w >> 8));
    (cyc(62), Gsu8277(0));
    cyc(22);                            /* REP #$20 */
    cyc(16);
    w = rd16(0x0000B7);
    cyc(16);
    wr16(0x000002, w);
    cyc(16);
    w = rd16(0x0000B9);
    cyc(16);
    wr16(0x000008, w);
    cyc(16);
    w = rd16(0x0000BB);
    cyc(16);
    wr16(0x000094, w);
    cyc(44);
    for (i = 0; i < 9; i++) {
        cyc(24);
        w = rd16(b | (uint16_t)(0x064C + 2 * i));
        w = asr7(w, &p);
        cyc(290);
        wr16(b | (uint16_t)(0x066A + 2 * i), w);
    }
    view_c = (uint8_t)(p & P_C);
    cyc(60);
    w = rd16(0x000002);
    cyc(32);
    wr16(0x700068, w);
    cyc(16);
    w = rd16(0x000008);
    cyc(32);
    wr16(0x70002C, w);
    cyc(16);
    w = rd16(0x000094);
    cyc(32);
    wr16(0x70002E, w);
    for (i = 0; i < 6; i++) {
        cyc(24);
        w = rd16(b | gsu_words[i]);
        cyc(32);
        wr16(0x700154 + 2 * i, w);
    }
    cyc(68);
    w = (uint16_t)((w & 0xFF00) | rd8(b | 0x067C));
    cyc(32);
    wr8(0x700162, (uint8_t)w);
    cyc(24);
    w = (uint16_t)((w & 0xFF00) | rd8(b | 0x067D));
    cyc(32);
    wr8(0x700164, (uint8_t)w);
    cyc(22);                            /* SEP #$30 */
    cyc(44);                            /* RTL */
    return w;
}

/* ViewFromAnglesGsu8277 on carried registers. */
void ViewFromAnglesGsu8277_cr(Carried *cr)
{
    cr->a = ViewFromAnglesGsu8277(cr->db);
    cr->x &= 0x00FF;
    cr->y &= 0x00FF;
    set_p(cr, nz8((uint8_t)((cr->p & ~P_C) | view_c | P_M | P_X), (uint8_t)cr->a));
}

void abi_ViewFromAnglesGsu8277(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ViewFromAnglesGsu8277_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:B367 ViewToGsuParams
 * WHAT IT DOES: Puts the view in Super FX RAM: the angle words in direct
 * page $C5, $C7, $C9 at $70:00D8-00DC (first the word at $1168 in all
 * three when $1167 is set) and the nine words at $064C at $70:00E4-00F4.
 * Nothing in the game calls it.
 * HOW: 16-bit loads (work RAM through the data bank) and long stores.
 * Entry: D = $0000. Exit: M 8-bit, X 8-bit (X and Y high bytes 0); A =
 * the word at $065C; N, Z from it.
 */
uint16_t ViewToGsuParams(uint8_t bank)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t w;
    unsigned i;

    cyc(22);                            /* REP #$20 */
    cyc(24);
    w = (uint16_t)(rd16(b | 0x1167) & 0x00FF);
    if (w) {
        cyc(40);
        for (i = 0; i < 3; i++) {
            cyc(24);
            w = rd16(b | 0x1168);
            cyc(32);
            wr16(0x7000D8 + 2 * i, w);
        }
    } else { cyc(46); }
    for (i = 0; i < 3; i++) {
        cyc(16);
        w = rd16((uint16_t)(0x00C5 + 2 * i));
        cyc(32);
        wr16(0x7000D8 + 2 * i, w);
    }
    for (i = 0; i < 9; i++) {
        cyc(24);
        w = rd16(b | (uint16_t)(0x064C + 2 * i));
        cyc(32);
        wr16(0x7000E4 + 2 * i, w);
    }
    cyc(44);
    cyc(22);                            /* SEP #$30 */
    return w;
}

/* ViewToGsuParams on carried registers. */
void ViewToGsuParams_cr(Carried *cr)
{
    cr->a = ViewToGsuParams(cr->db);
    cr->x &= 0x00FF;
    cr->y &= 0x00FF;
    set_p(cr, (uint8_t)(nz16(cr->p, cr->a) | P_M | P_X));
}

void abi_ViewToGsuParams(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ViewToGsuParams_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}
