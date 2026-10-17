/*
 * programs_vector.c: two more S-CPU wrappers of Super FX programs on the
 * vector in direct page $02, $08, $94, beside those in programs.c. No play
 * runs them; unit_gsu_vector calls them.
 */
#include "machine.h"
#include "cpu_ops.h"

uint8_t GsuRun(uint8_t bank, uint16_t start);
void abi_Gsu81E3Long(void);

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
uint16_t Gsu8277(void)
{
    uint16_t v;
    unsigned i;

    v = rd16(0x000002);
    wr16(0x700068, v);
    v = rd16(0x000008);
    wr16(0x70002C, v);
    v = rd16(0x000094);
    wr16(0x70002E, v);
    GsuRun(0x01, 0x8277);
    for (i = 0; i < 3; i++) {
        v = rd16(0x700026 + 2 * i);
        wr16(0x0000B7 + 2 * i, v);
    }
    return v;
}

void abi_Gsu8277(void)
{
    uint8_t p = cpu.p;
    uint16_t x = cpu.x, y = cpu.y;

    cpu.a = Gsu8277();
    cpu.x = x;
    cpu.y = y;
    cpu.p = (p & P_X) ? nz8(p, (uint8_t)x) : nz16(p, x);
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
void VectorGsu81E3(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint16_t x = cpu.x, y = cpu.y, u, v;

    cpu.p &= (uint8_t)~P_M;
    u = rd16(0x000004);
    v = rd16(0x000002);
    wr16(0x000002, (uint16_t)(u - v));
    u = rd16(b | 0x05F2);
    v = rd16(0x000094);
    cpu.a = (uint16_t)(u - v);
    cpu.p = (uint8_t)((cpu.p & ~(P_C | P_V)) | (u >= v ? P_C : 0) | (((u ^ v) & (u ^ cpu.a) & 0x8000) ? P_V : 0));
    wr16(0x000008, cpu.a);
    abi_Gsu81E3Long();
    cpu.y = y;
    cpu.x = x;
    cpu.p = nz16(cpu.p & ~P_M, x);
}

void abi_VectorGsu81E3(void)
{
    VectorGsu81E3();
}

void abi_Gsu8277(void);

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
    v = (uint8_t)(v >> 1 | (c ? 0x80 : 0));
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
    uint8_t v = rd8(a);

    wr8(a, ror8(v, p));
}

static void rol_dp(uint16_t a, uint8_t *p)
{
    uint8_t v = rd8(a);

    wr8(a, rol8(v, p));
}

static void lsr_dp(uint16_t a, uint8_t *p)
{
    uint8_t v = rd8(a);

    wr8(a, lsr8(v, p));
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

    *p = nz8(*p, x);
    a = rd8(0x00008B);
    *p = nz8(*p, a);
    if (a) {
        x++;
        a = lsr8(a, p);
        while (a) {
            ror_dp(0x8A, p);
            x++;
            a = lsr8(a, p);
        }
        ror_dp(0x8A, p);
    }
    wr8(0x000096, x);
    x = 0;
    a = rd8(0x000089);
    wr8(0x00008F, a);
    *p = nz8(*p, a);
    if (a & 0x80) {
        *p |= P_C;
        v = rd8(0x000088);
        a = op_sbc8(0x00, v, p);
        wr8(0x000088, a);
        v = rd8(0x000089);
        a = op_sbc8(0x00, v, p);
        wr8(0x000089, a);
    }
    if (a) {
        x++;
        a = lsr8(a, p);
        while (a) {
            ror_dp(0x88, p);
            x++;
            a = lsr8(a, p);
        }
        ror_dp(0x88, p);
    } else {
        a = rd8(0x000088);
        *p = nz8(*p, a);
        if (!(a & 0x80)) {
            if (!a) {
                a = rd8(0x0000CE);
                wr8(0x00008C, a);
                a = rd8(0x0000CF);
                wr8(0x00008D, a);
                *p = nz8(*p, a);
                *ra = a;
                *rx = x;
                return;
            }
            do {
                x--;
                a = asl8(a, p);
            } while (!(a & 0x80));
            wr8(0x000088, a);
        }
    }
    a = rd8(0x00008A);
    v = rd8(0x000088);
    cmp8(a, v, p);
    if (*p & P_C) {
        do {
            x--;
            a = lsr8(a, p);
            v = rd8(0x000088);
            cmp8(a, v, p);
        } while (*p & P_C);
        wr8(0x00008A, a);
    }
    *p |= P_C;
    v = rd8(0x000096);
    a = op_sbc8(x, v, p);
    wr8(0x00008E, a);
    x = rd8(0x000088);
    a = rd8(b | (uint16_t)(0x8368 + x));
    x = rd8(0x00008A);
    *p |= P_C;
    v = rd8(b | (uint16_t)(0x8368 + x));
    a = op_sbc8(a, v, p);
    x = a;
    a = rd8(b | (uint16_t)(0x8268 + x));
    wr8(0x00008D, a);
    a = rd8(b | (uint16_t)(0x8168 + x));
    x = rd8(0x00008E);
    if (x & 0x80) {
        do {
            lsr_dp(0x8D, p);
            a = ror8(a, p);
            x++;
        } while (x);
    } else {
        while (x) {
            a = asl8(a, p);
            rol_dp(0x8D, p);
            x--;
        }
    }
    wr8(0x00008C, a);
    v = rd8(0x00008F);
    *p = (uint8_t)((*p & ~(P_N | P_V | P_Z)) | (v & 0xC0) | ((a & v) ? 0 : P_Z));
    if (v & 0x80) {
        *p |= P_C;
        a = rd8(0x0000CE);
        v = rd8(0x00008C);
        a = op_sbc8(a, v, p);
        wr8(0x00008C, a);
        a = rd8(0x0000CF);
        v = rd8(0x00008D);
        a = op_sbc8(a, v, p);
        wr8(0x00008D, a);
    } else {
        *p &= (uint8_t)~P_C;
        a = rd8(0x00008C);
        v = rd8(0x0000CE);
        a = op_adc8(a, v, p);
        wr8(0x00008C, a);
        a = rd8(0x00008D);
        v = rd8(0x0000CF);
        a = op_adc8(a, v, p);
        wr8(0x00008D, a);
    }
    *ra = a;
    *rx = x;
}

void abi_ScaledOffset(void)
{
    uint8_t a = (uint8_t)cpu.a, x = (uint8_t)cpu.x;

    ScaledOffset(cpu.db, &a, &x, &cpu.p);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | a);
    cpu.x = x;
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
void abi_ScaledOffsetPair(void)
{
    uint8_t a, v;
    unsigned i;

    for (i = 0; i < 2; i++) {
        static const uint16_t in[2] = {0x02, 0x08}, out[2] = {0x76, 0x78};

        a = rd8(in[i]);
        wr8(0x000088, a);
        a = rd8((uint16_t)(in[i] + 1));
        wr8(0x000089, a);
        a = rd8(0x000094);
        wr8(0x00008A, a);
        a = rd8(0x000095);
        wr8(0x00008B, a);
        abi_ScaledOffset();
        a = rd8(0x00008C);
        wr8(out[i], a);
        a = rd8(0x00008D);
        wr8((uint16_t)(out[i] + 1), a);
    }
    cpu.p &= (uint8_t)~P_C;
    a = rd8(0x0000D0);
    v = rd8(0x000078);
    a = op_adc8(a, v, &cpu.p);
    wr8(0x000078, a);
    a = rd8(0x0000D1);
    v = rd8(0x000079);
    a = op_adc8(a, v, &cpu.p);
    wr8(0x000079, a);
    cpu.p |= P_C;
    a = rd8(0x000078);
    v = rd8(0x0000CE);
    a = op_sbc8(a, v, &cpu.p);
    wr8(0x000078, a);
    a = rd8(0x000079);
    v = rd8(0x0000CF);
    a = op_sbc8(a, v, &cpu.p);
    wr8(0x000079, a);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | a);
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
void abi_ViewFromAnglesGsu8277(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    static const uint16_t gsu_words[6] = {0x066A, 0x066C, 0x066E, 0x0676, 0x0678, 0x067A};
    uint16_t w;
    unsigned i;

    op_rep(P_M | P_X);
    w = rd16(0x0000C5);
    w = (uint16_t)((w & 0x00FF) ^ 0x00FF);
    wr16(0x000002, (uint16_t)(w - 0x0780));
    w = rd16(0x0000C9);
    w = (uint16_t)((w & 0x00FF) ^ 0x00FF);
    wr16(0x000094, (uint16_t)(w - 0x0780));
    w = rd16(0x0000C7);
    wr16(0x000008, (uint16_t)(0 - w));
    w = rd16(0x0000C5);
    wr8(b | 0x067C, (uint8_t)(w >> 8));
    w = rd16(0x0000C9);
    wr8(b | 0x067D, (uint8_t)(w >> 8));
    cpu.a = (uint16_t)(w >> 8);
    op_sep(P_M);
    abi_Gsu8277();
    op_rep(P_M);
    w = rd16(0x0000B7);
    wr16(0x000002, w);
    w = rd16(0x0000B9);
    wr16(0x000008, w);
    w = rd16(0x0000BB);
    wr16(0x000094, w);
    for (i = 0; i < 9; i++) {
        w = rd16(b | (uint16_t)(0x064C + 2 * i));
        w = asr7(w, &cpu.p);
        wr16(b | (uint16_t)(0x066A + 2 * i), w);
    }
    w = rd16(0x000002);
    wr16(0x700068, w);
    w = rd16(0x000008);
    wr16(0x70002C, w);
    w = rd16(0x000094);
    wr16(0x70002E, w);
    for (i = 0; i < 6; i++) {
        w = rd16(b | gsu_words[i]);
        wr16(0x700154 + 2 * i, w);
    }
    op_sep(P_M);
    w = (uint16_t)((w & 0xFF00) | rd8(b | 0x067C));
    wr8(0x700162, (uint8_t)w);
    w = (uint16_t)((w & 0xFF00) | rd8(b | 0x067D));
    wr8(0x700164, (uint8_t)w);
    cpu.a = w;
    cpu.p = nz8(cpu.p, (uint8_t)w);
    op_sep(P_M | P_X);
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
void abi_ViewToGsuParams(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint16_t w;
    unsigned i;

    op_rep(P_M);
    w = (uint16_t)(rd16(b | 0x1167) & 0x00FF);
    if (w) {
        for (i = 0; i < 3; i++) {
            w = rd16(b | 0x1168);
            wr16(0x7000D8 + 2 * i, w);
        }
    }
    for (i = 0; i < 3; i++) {
        w = rd16((uint16_t)(0x00C5 + 2 * i));
        wr16(0x7000D8 + 2 * i, w);
    }
    for (i = 0; i < 9; i++) {
        w = rd16(b | (uint16_t)(0x064C + 2 * i));
        wr16(0x7000E4 + 2 * i, w);
    }
    cpu.a = w;
    cpu.p = nz16(cpu.p, w);
    op_sep(P_M | P_X);
}
