/*
 * heading.c: an object's velocity from its speed and heading, with the
 * S-CPU's hardware multiplier, and moving an object by such a vector. The
 * speed is a signed byte (an object's +$17, or A); the angles are bytes
 * indexing the sine and cosine tables at $00:96FE and $00:973E. Each
 * product is a signed 8 x 8 multiply kept to its high byte: the vector's
 * components land in direct page $02 (x), $08 (y) and $94 (z), each
 * sign-extended to a word. X and Y are kept in direct page $7A and $7C,
 * the sines and cosines in $7E/$7F and $86/$87, the speed in $82. No play
 * runs these; unit_heading_03 calls them.
 */
#include "machine.h"
#include "hw.h"
#include "cpu_ops.h"

#define H_X 0x000002u
#define H_Y 0x000008u
#define H_Z 0x000094u
#define H_SAVE_X 0x00007Au
#define H_SAVE_Y 0x00007Cu
#define H_SIN_A 0x00007Eu
#define H_COS_A 0x00007Fu
#define H_SIN_B 0x000086u
#define H_COS_B 0x000087u
#define H_SPEED 0x000082u
#define SIN_TABLE 0x0096FEu
#define COS_TABLE 0x00973Eu

/*
 * Signed 8 x 8 multiply of the bytes at a_at and b_at, as each routine
 * here does it inline: the magnitude of the first, doubled, to WRMPYA; the
 * magnitude of the second to WRMPYB; the high byte of the product from
 * RDMPYH, subtracted from 0 when the signs differ (the subtract sets V).
 */
static uint8_t smul_at(uint32_t a_at, uint32_t b_at, uint8_t *p)
{
    cyc(16);
    uint8_t a = rd8(a_at), b, h, neg = 0, a_neg;

    if (a & 0x80) {
        cyc(36);
        a = (uint8_t)(~a + 1);
        neg ^= 1;
    }
    a_neg = neg;
    cyc(54);
    wr8(WRMPYA, (uint8_t)(a << 1));
    cyc(16);
    b = rd8(b_at);
    if (b & 0x80) {
        cyc(36);
        b = (uint8_t)(~b + 1);
        neg ^= 1;
    }
    cyc(40);
    wr8(WRMPYB, b);
    /* four NOPs and LDA RDMPYH; or NOP, JMP, then LDA #0 / SEC / SBC (82
       when only b was negated, 92 when only a was) */
    cyc(!neg ? 80 : a_neg ? 92 : 82);
    h = rd8(RDMPYH);
    if (neg) {
        *p |= P_C;
        h = op_sbc8(0x00, h, p);
    } else { cyc(40); }
    return h;
}

/* A product stored with its sign byte written after it (LDX #0 / DEX /
   STX), as the flat vectors store x and z. */
static void store_extended(uint32_t at, uint8_t v)
{
    cyc(30);
    wr8(at, v);
    cyc(54);
    wr8(at + 1, (v & 0x80) ? (cyc(8), 0xFF) : 0x00);
}

/* A product stored over a sign byte cleared earlier, DEC'd when negative,
   as the vectors with a pitch store theirs. */
static void store_dec(uint32_t at, uint8_t v)
{
    uint8_t hi;

    cyc(14);
    wr8(at, v);
    if (v & 0x80) {
        cyc(48);
        hi = rd8(at + 1);
        cyc(6);
        wr8(at + 1, (uint8_t)(hi - 1));
    } else { cyc(38); }
}

/* The extra 6 clocks of an indexed read that crosses a page. */
static uint32_t crossed(uint32_t table, uint8_t i)
{
    return ((table & 0xFF) + i) > 0xFF ? 6 : 0;
}

/* The flat vector from the sine and cosine at table index i: x = speed
   sin, z = speed cos; y was cleared by the caller. */
static uint8_t flat(uint8_t i, uint8_t *p)
{
    uint8_t v;

    cyc(150 + crossed(SIN_TABLE, i));
    wr8(H_SIN_A, rd8(SIN_TABLE + i));
    cyc(48 + crossed(COS_TABLE, i));
    wr8(H_COS_A, rd8(COS_TABLE + i));
    v = smul_at(H_SPEED, H_SIN_A, p);
    store_extended(H_X, v);
    v = smul_at(H_SPEED, H_COS_A, p);
    store_extended(H_Z, v);
    return v;
}

/* The vector with a pitch, from the tables at index ia (into $7E/$7F) and
   ib (into $86/$87): x = (speed sin a) cos b, z = (speed cos a) cos b, y =
   speed sin b; each first product read back from its place before the
   second multiply. */
static uint8_t pitched(uint8_t ia, uint8_t ib, uint8_t *p)
{
    uint8_t v;

    cyc(84 + crossed(SIN_TABLE, ia));
    wr8(H_SIN_A, rd8(SIN_TABLE + ia));
    cyc(48 + crossed(COS_TABLE, ia));
    wr8(H_COS_A, rd8(COS_TABLE + ia));
    cyc(48 + crossed(SIN_TABLE, ib));
    wr8(H_SIN_B, rd8(SIN_TABLE + ib));
    cyc(48 + crossed(COS_TABLE, ib));
    wr8(H_COS_B, rd8(COS_TABLE + ib));
    wr8(H_X, cyc_then(14, smul_at(H_SPEED, H_SIN_A, p)));
    v = smul_at(H_X, H_COS_B, p);
    store_dec(H_X, v);
    wr8(H_Z, cyc_then(14, smul_at(H_SPEED, H_COS_A, p)));
    v = smul_at(H_Z, H_COS_B, p);
    store_dec(H_Z, v);
    v = smul_at(H_SPEED, H_SIN_B, p);
    store_dec(H_Y, v);
    return v;
}

/* Every routine here saves X and Y at its start and ends with X 16-bit,
   X and Y read back, and the PLB of the caller's data bank (N, Z from it);
   C is set by the last sign test (CMP #0); A keeps its high byte. */
static void save_xy(void)
{
    cyc(16);
    wr16(H_SAVE_X, cpu.x);
    cyc(16);
    wr16(H_SAVE_Y, cpu.y);
}

static void finish(uint8_t v, uint8_t p)
{
    cpu.p = (uint8_t)((p & ~P_X) | P_C);
    cyc(38);
    cpu.x = rd16(H_SAVE_X);
    cyc(16);
    cpu.y = rd16(H_SAVE_Y);
    cyc(72);
    cpu.p = nz8(cpu.p, cpu.db);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
}

/*
 * @routine 03:B93E HeadingFlat
 * WHAT IT DOES: Object X's flat velocity from its speed (+$17) and its
 * heading (+$15): $02 = speed sin, $94 = speed cos, $08 = 0.
 * HOW: The tables read with the data bank 0; two signed multiplies.
 * Entry: M 8-bit, X 16-bit, D = $0000; X = object. Exit: A low byte = the
 * z component's low byte; X, Y unchanged, X 16-bit; C set; V from the
 * last negating subtract; N, Z from the caller's data bank.
 */
void HeadingFlat(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint8_t p = cpu.p, i, v;

    cyc(30);
    wr8(H_SPEED, cyc_then(16, rd8(b | (uint16_t)(0x2017 + cpu.x))));
    (cyc(22), save_xy());
    cyc(16);
    wr8(H_Y, 0x00);
    cyc(16);
    wr8(H_Y + 1, 0x00);
    cyc(30);
    i = rd8(b | (uint16_t)(0x2015 + cpu.x));
    v = flat(i, &p);
    finish(v, p);
}

void abi_HeadingFlat(void)
{
    HeadingFlat();
}

/*
 * @routine 03:BA12 HeadingFlatMirrored
 * WHAT IT DOES: As HeadingFlat, for the heading 1 - (+$15): the mirror of
 * the object's heading, one step on.
 * HOW: The heading negated, then INX; the INY it makes is undone when Y is
 * read back.
 * Entry: as HeadingFlat. Exit: as HeadingFlat.
 */
void HeadingFlatMirrored(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint8_t p = cpu.p, i, v;

    cyc(30);
    wr8(H_SPEED, cyc_then(16, rd8(b | (uint16_t)(0x2017 + cpu.x))));
    cyc(30);
    i = rd8(b | (uint16_t)(0x2015 + cpu.x));
    save_xy();
    cyc(16);
    wr8(H_Y, 0x00);
    cyc(16);
    wr8(H_Y + 1, 0x00);
    cyc(80);
    v = flat((uint8_t)(~i + 2), &p);
    finish(v, p);
}

void abi_HeadingFlatMirrored(void)
{
    HeadingFlatMirrored();
}

/*
 * @routine 03:BAEB HeadingPitched
 * WHAT IT DOES: Object X's velocity from its speed (+$17), heading (+$15)
 * and pitch (+$14): $02 = (speed sin h) cos p, $94 = (speed cos h) cos p,
 * $08 = speed sin p, where p is the pitch negated.
 * HOW: pitched(); the high bytes of $02, $08, $94 cleared first and
 * decremented for a negative product.
 * Entry: M 8-bit, X 16-bit, D = $0000; X = object. Exit: as HeadingFlat (A
 * low byte = the y component).
 */
void HeadingPitched(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint8_t p = cpu.p, h, pitch, v;

    cyc(30);
    wr8(H_SPEED, cyc_then(16, rd8(b | (uint16_t)(0x2017 + cpu.x))));
    cyc(16);
    wr8(H_X + 1, 0x00);
    cyc(16);
    wr8(H_Y + 1, 0x00);
    cyc(16);
    wr8(H_Z + 1, 0x00);
    save_xy();
    cyc(30);
    h = rd8(b | (uint16_t)(0x2015 + cpu.x));
    cyc(44);
    pitch = rd8(b | (uint16_t)(0x2014 + cpu.x));
    cyc(118);
    v = pitched(h, (uint8_t)(~pitch + 1), &p);
    finish(v, p);
}

void abi_HeadingPitched(void)
{
    HeadingPitched();
}

/* HeadingPitchedFrom1F7C from its second entry on: the angles from $1F7C
   (pitch) and $1F7D (heading) through data bank 0. */
static void pitched_1f7c(void)
{
    uint8_t p = cpu.p, h, pitch, v;

    cyc(16);
    wr8(H_X + 1, 0x00);
    cyc(16);
    wr8(H_Y + 1, 0x00);
    cyc(16);
    wr8(H_Z + 1, 0x00);
    save_xy();
    cyc(112);
    h = rd8(0x001F7D);
    cyc(68);
    pitch = rd8(0x001F7C);
    v = pitched((uint8_t)(~h + 1), pitch, &p);
    finish(v, p);
}

/*
 * @routine 03:BCB6 HeadingPitchedVia1F7C
 * WHAT IT DOES: As HeadingPitched with the heading negated instead of the
 * pitch: the object's +$15 and +$14 are copied to $1F7D and $1F7C first,
 * then HeadingPitchedFrom1F7C.
 * HOW: The copies through the caller's data bank, then falls into
 * HeadingPitchedFrom1F7C.
 * Entry: as HeadingPitched. Exit: as HeadingPitched.
 */
void HeadingPitchedVia1F7C(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;

    cyc(30);
    wr8(H_SPEED, cyc_then(16, rd8(b | (uint16_t)(0x2017 + cpu.x))));
    cyc(30);
    wr8(b | 0x1F7D, cyc_then(24, rd8(b | (uint16_t)(0x2015 + cpu.x))));
    cyc(30);
    wr8(b | 0x1F7C, cyc_then(24, rd8(b | (uint16_t)(0x2014 + cpu.x))));
    pitched_1f7c();
}

void abi_HeadingPitchedVia1F7C(void)
{
    HeadingPitchedVia1F7C();
}

/*
 * @routine 03:BCC7 HeadingPitchedFrom1F7C
 * WHAT IT DOES: The velocity for the speed in $82, heading -($1F7D) and
 * pitch $1F7C: $02 = (speed sin h) cos p, $94 = (speed cos h) cos p, $08 =
 * speed sin p.
 * HOW: As HeadingPitched, the angles read with the data bank already 0.
 * Entry: M 8-bit, X 16-bit, D = $0000; $82 = speed. Exit: as HeadingPitched.
 */
void HeadingPitchedFrom1F7C(void)
{
    pitched_1f7c();
}

void abi_HeadingPitchedFrom1F7C(void)
{
    pitched_1f7c();
}

/*
 * @routine 03:BE8D HeadingFlatQuarterTurn
 * WHAT IT DOES: As HeadingFlat for the speed A and the heading $41 - (+$15)
 * of object X: a quarter turn from its mirror, one step on.
 * HOW: SEC / LDA #$40 / SBC (the subtract sets V), then INX.
 * Entry: M 8-bit, X 16-bit, D = $0000; A = speed, X = object. Exit: as
 * HeadingFlat.
 */
void HeadingFlatQuarterTurn(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint8_t p = cpu.p, i, v;

    cyc(16);
    wr8(H_SPEED, (uint8_t)cpu.a);
    save_xy();
    cyc(16);
    wr8(H_Y, 0x00);
    cyc(16);
    wr8(H_Y + 1, 0x00);
    p |= P_C;
    cyc(60);
    i = op_sbc8(0x40, rd8(b | (uint16_t)(0x2015 + cpu.x)), &p);
    cyc(20);
    v = flat((uint8_t)(i + 1), &p);
    finish(v, p);
}

void abi_HeadingFlatQuarterTurn(void)
{
    HeadingFlatQuarterTurn();
}

/*
 * @routine 03:BF62 HeadingFlatSpeedA
 * WHAT IT DOES: As HeadingFlat for the speed A and the heading (+$15) + 1
 * of object X.
 * HOW: As HeadingFlat, with INX.
 * Entry: as HeadingFlatQuarterTurn. Exit: as HeadingFlat.
 */
void HeadingFlatSpeedA(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint8_t p = cpu.p, i, v;

    cyc(16);
    wr8(H_SPEED, (uint8_t)cpu.a);
    save_xy();
    cyc(16);
    wr8(H_Y, 0x00);
    cyc(16);
    wr8(H_Y + 1, 0x00);
    cyc(30);
    i = rd8(b | (uint16_t)(0x2015 + cpu.x));
    cyc(36);
    v = flat((uint8_t)(i + 1), &p);
    finish(v, p);
}

void abi_HeadingFlatSpeedA(void)
{
    HeadingFlatSpeedA();
}

/* Object X's position (+$0C, +$0E, +$10) plus the words at $02, $08 and
   $94, in that order; N, Z, C, V from the last add. */
static void move_by(uint8_t *p)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint16_t v;

    cyc(52);
    v = rd16(b | (uint16_t)(0x200C + cpu.x));
    *p &= (uint8_t)~P_C;
    cyc(30);
    v = op_adc16(v, rd16(H_X), p);
    cyc(30);
    wr16(b | (uint16_t)(0x200C + cpu.x), v);
    cyc(30);
    v = rd16(b | (uint16_t)(0x200E + cpu.x));
    *p &= (uint8_t)~P_C;
    cyc(30);
    v = op_adc16(v, rd16(H_Y), p);
    cyc(30);
    wr16(b | (uint16_t)(0x200E + cpu.x), v);
    cyc(30);
    v = rd16(b | (uint16_t)(0x2010 + cpu.x));
    *p &= (uint8_t)~P_C;
    cyc(30);
    v = op_adc16(v, rd16(H_Z), p);
    cyc(30);
    wr16(b | (uint16_t)(0x2010 + cpu.x), v);
    cpu.a = v;
}

/* ASL of the three words at $02, $08, $94 in that order. */
static void double_vector(void)
{
    static const uint32_t at[3] = {H_X, H_Y, H_Z};
    unsigned i;
    uint16_t v;

    for (i = 0; i < 3; i++) {
        cyc(16);
        v = rd16(at[i]);
        cyc(14);
        wr16(at[i], (uint16_t)(v << 1));
    }
}

/*
 * @routine 03:C034 ObjectMoveByVectorTimes4
 * WHAT IT DOES: Moves object X by four times the vector at $02, $08, $94
 * (16-bit), which is left doubled twice.
 * HOW: Each word shifted left twice, all three once and then again, then
 * the adds of ObjectMoveByVector.
 * Entry: X 16-bit, D = $0000; X = object. Exit: A = the new +$10; M 8-bit;
 * N, Z, C, V from its add.
 */
void ObjectMoveByVectorTimes4(void)
{
    uint8_t p = cpu.p;

    (cyc(22), double_vector());
    double_vector();
    move_by(&p);
    cyc(66);
    cpu.p = (uint8_t)(p | P_M);
}

void abi_ObjectMoveByVectorTimes4(void)
{
    ObjectMoveByVectorTimes4();
}

/*
 * @routine 03:C044 ObjectMoveByVectorTimes2
 * WHAT IT DOES: Moves object X by twice the vector at $02, $08, $94, which
 * is left doubled.
 * HOW: Each word shifted left once, then ObjectMoveByVector.
 * Entry: as ObjectMoveByVectorTimes4. Exit: as ObjectMoveByVectorTimes4.
 */
void ObjectMoveByVectorTimes2(void)
{
    uint8_t p = cpu.p;

    (cyc(22), double_vector());
    move_by(&p);
    cyc(66);
    cpu.p = (uint8_t)(p | P_M);
}

void abi_ObjectMoveByVectorTimes2(void)
{
    ObjectMoveByVectorTimes2();
}

/*
 * @routine 03:C04C ObjectMoveByVector
 * WHAT IT DOES: Moves object X by the vector at $02, $08, $94: adds them
 * to its +$0C, +$0E and +$10.
 * HOW: Three 16-bit adds, through the caller's data bank.
 * Entry: as ObjectMoveByVectorTimes4. Exit: as ObjectMoveByVectorTimes4.
 */
void ObjectMoveByVector(void)
{
    uint8_t p = cpu.p;

    move_by(&p);
    cyc(66);
    cpu.p = (uint8_t)(p | P_M);
}

void abi_ObjectMoveByVector(void)
{
    ObjectMoveByVector();
}

/* Object X's velocity (+$2E, +$30, +$32) from the vector at $02, $08, $94
   after HeadingPitchedFrom1F7C, each word shifted left `shift` times (each
   shift a 16-bit ASL of A). */
static void velocity_from_vector(int shift)
{
    static const uint32_t from[3] = {H_X, H_Y, H_Z};
    uint32_t b;
    uint16_t v = 0;
    unsigned i;
    int k;

    (cyc(62), abi_HeadingPitchedFrom1F7C());
    b = (uint32_t)cpu.db << 16;
    cyc(22);
    for (i = 0; i < 3; i++) {
        cyc(16);
        v = rd16(from[i]);
        cpu.p = nz16(cpu.p, v);
        for (k = 0; k < shift; k++) {
            cyc(14);
            cpu.p = (uint8_t)((cpu.p & ~P_C) | (v >> 15));
            v = (uint16_t)(v << 1);
            cpu.p = nz16(cpu.p, v);
        }
        cyc(30);
        wr16(b | (uint16_t)(0x202E + 2 * i + cpu.x), v);
    }
    cyc(66);
    cpu.a = v;
    cpu.p |= P_M;
}

/*
 * @routine 03:CD1D ObjectVelocityFromHeading
 * WHAT IT DOES: Sets object X's velocity (+$2E, +$30, +$32) to the vector
 * HeadingPitchedFrom1F7C makes for the speed in $82 and the angles at
 * $1F7C/$1F7D.
 * HOW: The vector's words copied, 16-bit, through the caller's data bank.
 * Entry: M 8-bit, X 16-bit, D = $0000; X = object. Exit: A = the z word;
 * M 8-bit; N, Z from it; C, V as HeadingPitchedFrom1F7C leaves them.
 */
void ObjectVelocityFromHeading(void)
{
    velocity_from_vector(0);
}

void abi_ObjectVelocityFromHeading(void)
{
    velocity_from_vector(0);
}

/*
 * @routine 03:CD35 ObjectVelocityFromHeadingTimes2
 * WHAT IT DOES: As ObjectVelocityFromHeading with each word doubled.
 * HOW: One ASL A on each word before it is stored.
 * Entry: as ObjectVelocityFromHeading. Exit: as ObjectVelocityFromHeading,
 * C from the last shift.
 */
void ObjectVelocityFromHeadingTimes2(void)
{
    velocity_from_vector(1);
}

void abi_ObjectVelocityFromHeadingTimes2(void)
{
    velocity_from_vector(1);
}

/*
 * @routine 03:CD50 ObjectVelocityFromHeadingTimes4
 * WHAT IT DOES: As ObjectVelocityFromHeading with each word times 4.
 * HOW: Two ASL A on each word.
 * Entry: as ObjectVelocityFromHeading. Exit: as
 * ObjectVelocityFromHeadingTimes2.
 */
void ObjectVelocityFromHeadingTimes4(void)
{
    velocity_from_vector(2);
}

void abi_ObjectVelocityFromHeadingTimes4(void)
{
    velocity_from_vector(2);
}

/*
 * @routine 03:CD6E ObjectVelocityFromHeadingTimes8
 * WHAT IT DOES: As ObjectVelocityFromHeading with each word times 8.
 * HOW: Three ASL A on each word.
 * Entry: as ObjectVelocityFromHeading. Exit: as
 * ObjectVelocityFromHeadingTimes2.
 */
void ObjectVelocityFromHeadingTimes8(void)
{
    velocity_from_vector(3);
}

void abi_ObjectVelocityFromHeadingTimes8(void)
{
    velocity_from_vector(3);
}

/*
 * @routine 03:D1C2 ObjectVelocityLevelFromDp
 * WHAT IT DOES: Sets object X's velocity to the vector for the speed in
 * $94, the pitch in $02 and the heading -$40 (HeadingPitchedFrom1F7C),
 * with the z velocity (+$32) cleared instead of set.
 * HOW: $02 to $1F7C, $94 to $82, $40 to $1F7D (8-bit, the first and last
 * through the caller's data bank), then the words at $02 and $08 copied to
 * +$2E and +$30.
 * Entry: M 8-bit, X 16-bit, D = $0000; X = object. Exit: A = the word at
 * $08; M 8-bit; N, Z from it; C, V as HeadingPitchedFrom1F7C leaves them.
 */
void ObjectVelocityLevelFromDp(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint16_t v;

    cyc(16);
    wr8(b | 0x1F7C, cyc_then(24, rd8(H_X)));
    cyc(16);
    wr8(H_SPEED, cyc_then(16, rd8(H_Z)));
    cyc(40);
    wr8(b | 0x1F7D, 0x40);
    (cyc(62), abi_HeadingPitchedFrom1F7C());
    b = (uint32_t)cpu.db << 16;
    cyc(38);
    v = rd16(H_X);
    cyc(30);
    wr16(b | (uint16_t)(0x202E + cpu.x), v);
    cyc(16);
    v = rd16(H_Y);
    cyc(30);
    wr16(b | (uint16_t)(0x2030 + cpu.x), v);
    cyc(30);
    wr16(b | (uint16_t)(0x2032 + cpu.x), 0x0000);
    cyc(66);
    cpu.a = v;
    cpu.p = (uint8_t)(nz16(cpu.p, v) | P_M);
}

void abi_ObjectVelocityLevelFromDp(void)
{
    ObjectVelocityLevelFromDp();
}
