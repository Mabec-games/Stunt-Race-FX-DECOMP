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
 *
 * The functions take what the routines read from registers (the caller's
 * data bank, X the object, Y, A the speed) and return A. X and Y come back
 * unchanged (they are saved and read back from direct page); the flags are
 * the adapters' business (heading_regs, vel_regs, move_regs).
 */
#include "machine.h"
#include "native.h"
#include "hw.h"
#include "math_calls.h"
#include "carried.h"
#include "wram_names.h"


#define SIN_TABLE 0x0096FEu
#define COS_TABLE 0x00973Eu

/* The V flag the last SEC / SBC left, for the adapters: -1 until the
   routine has made one (V is then the caller's). */
static int head_v;

/* SEC / SBC (8-bit, binary): the result, V in head_v. */
static uint8_t sbc8(uint8_t a, uint8_t m)
{
    uint8_t nm = (uint8_t)~m;
    unsigned r = (unsigned)a + nm + 1;

    cyc(16);
    head_v = ((~(a ^ nm) & (a ^ r) & 0x80) != 0);
    return (uint8_t)r;
}

/*
 * Signed 8 x 8 multiply of the bytes at a_at and b_at, as each routine
 * here does it inline: the magnitude of the first, doubled, to WRMPYA; the
 * magnitude of the second to WRMPYB; the high byte of the product from
 * RDMPYH, subtracted from 0 when the signs differ (the subtract sets V).
 */
static uint8_t smul_at(uint32_t a_at, uint32_t b_at)
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
    io_wr8(WRMPYA, (uint8_t)(a << 1));
    cyc(16);
    b = rd8(b_at);
    if (b & 0x80) {
        cyc(36);
        b = (uint8_t)(~b + 1);
        neg ^= 1;
    }
    cyc(40);
    io_wr8(WRMPYB, b);
    /* four NOPs and LDA RDMPYH; or NOP, JMP, then LDA #0 / SEC / SBC (82
       when only b was negated, 92 when only a was) */
    cyc(!neg ? 80 : a_neg ? 92 : 82);
    h = io_rd8(RDMPYH, (uint8_t)(cpu_mul((uint8_t)(a << 1), b) >> 8));
    if (neg) {
        h = sbc8(0x00, h);
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
static uint8_t flat(uint8_t i)
{
    uint8_t v;

    cyc(150 + crossed(SIN_TABLE, i));
    wr8(W(heading_sin_a), rd8(SIN_TABLE + i));
    cyc(48 + crossed(COS_TABLE, i));
    wr8(W(heading_cos_a), rd8(COS_TABLE + i));
    v = smul_at(W(heading_speed), W(heading_sin_a));
    store_extended(W(dp_02), v);
    v = smul_at(W(heading_speed), W(heading_cos_a));
    store_extended(W(dp_94), v);
    return v;
}

/* The vector with a pitch, from the tables at index ia (into $7E/$7F) and
   ib (into $86/$87): x = (speed sin a) cos b, z = (speed cos a) cos b, y =
   speed sin b; each first product read back from its place before the
   second multiply. */
static uint8_t pitched(uint8_t ia, uint8_t ib)
{
    uint8_t v;

    cyc(84 + crossed(SIN_TABLE, ia));
    wr8(W(heading_sin_a), rd8(SIN_TABLE + ia));
    cyc(48 + crossed(COS_TABLE, ia));
    wr8(W(heading_cos_a), rd8(COS_TABLE + ia));
    cyc(48 + crossed(SIN_TABLE, ib));
    wr8(W(heading_sin_b), rd8(SIN_TABLE + ib));
    cyc(48 + crossed(COS_TABLE, ib));
    wr8(W(heading_cos_b), rd8(COS_TABLE + ib));
    wr8(W(dp_02), cyc_then(14, smul_at(W(heading_speed), W(heading_sin_a))));
    v = smul_at(W(dp_02), W(heading_cos_b));
    store_dec(W(dp_02), v);
    wr8(W(dp_94), cyc_then(14, smul_at(W(heading_speed), W(heading_cos_a))));
    v = smul_at(W(dp_94), W(heading_cos_b));
    store_dec(W(dp_94), v);
    v = smul_at(W(heading_speed), W(heading_sin_b));
    store_dec(W(dp_08), v);
    return v;
}

/* Every routine here saves X and Y at its start and ends with X 16-bit,
   X and Y read back (they come back as they went in: nothing between
   writes $7A-$7D), and the PLB of the caller's data bank (N, Z from it);
   C is set by the last sign test (CMP #0); A keeps its high byte. */
static void save_xy(uint16_t x, uint16_t y)
{
    cyc(16);
    wr16(W(heading_save_x), x);
    cyc(16);
    wr16(W(heading_save_y), y);
}

static void finish(void)
{
    cyc(38);
    (void)rd16(W(heading_save_x));
    cyc(16);
    (void)rd16(W(heading_save_y));
    cyc(72);
}

/* The registers a heading routine leaves: A's low byte v; X, Y as they
   were; X 16-bit, C set, V from the last subtract (the caller's when
   there was none), N, Z from the data bank. */
static uint8_t heading_p(uint8_t p)
{
    p = (uint8_t)((p & ~P_X) | P_C);
    if (head_v >= 0)
        p = (uint8_t)((p & ~P_V) | (head_v ? P_V : 0));
    return p;
}

void HeadingFlat_cr(Carried *cr);
void HeadingFlatMirrored_cr(Carried *cr);
void HeadingFlatQuarterTurn_cr(Carried *cr);
void HeadingFlatSpeedA_cr(Carried *cr);
void HeadingPitched_cr(Carried *cr);
void HeadingPitchedFrom1F7C_cr(Carried *cr);
void HeadingPitchedVia1F7C_cr(Carried *cr);
void ObjectMoveByVector_cr(Carried *cr);
void ObjectMoveByVectorTimes2_cr(Carried *cr);
void ObjectMoveByVectorTimes4_cr(Carried *cr);
void ObjectVelocityFromHeading_cr(Carried *cr);
void ObjectVelocityFromHeadingTimes2_cr(Carried *cr);
void ObjectVelocityFromHeadingTimes4_cr(Carried *cr);
void ObjectVelocityFromHeadingTimes8_cr(Carried *cr);
void ObjectVelocityLevelFromDp_cr(Carried *cr);
static void heading_regs(Carried *cr, uint8_t v);
static void move_regs(Carried *cr, uint16_t v);
static void vel_regs(Carried *cr, uint16_t v);

static void heading_regs(Carried *cr, uint8_t v)
{
    set_p(cr, nz8(heading_p(cr->p), cr->db));
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
}

/*
 * @routine 03:B93E HeadingFlat
 * WHAT IT DOES: Object X's flat velocity from its speed (+$17) and its
 * heading (+$15): $02 = speed sin, $94 = speed cos, $08 = 0.
 * HOW: The tables read with the data bank 0; two signed multiplies.
 * Entry: M 8-bit, X 16-bit, D = $0000; X = object; `bank` the caller's
 * data bank. Exit: the return value is A's low byte = the z component's
 * low byte; X, Y unchanged, X 16-bit; C set; V from the last negating
 * subtract; N, Z from the caller's data bank (the adapter has these).
 */
uint8_t HeadingFlat(uint8_t bank, uint16_t x, uint16_t y)
{
    (void)bank;
    uint8_t i, v;

    head_v = -1;
    cyc(30);
    wr8(W(heading_speed), cyc_then(16, rd8(0x7E0000 | (uint16_t)(W(obj_17) + x))));
    (cyc(22), save_xy(x, y));
    cyc(16);
    wr8(W(dp_08), 0x00);
    cyc(16);
    wr8(W(dp_08) + 1, 0x00);
    cyc(30);
    i = rd8(0x7E0000 | (uint16_t)(W(obj_15) + x));
    v = flat(i);
    finish();
    return v;
}

void HeadingFlat_cr(Carried *cr)
{
    heading_regs(cr, HeadingFlat(cr->db, cr->x, cr->y));
}

void abi_HeadingFlat(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    HeadingFlat_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:BA12 HeadingFlatMirrored
 * WHAT IT DOES: As HeadingFlat, for the heading 1 - (+$15): the mirror of
 * the object's heading, one step on.
 * HOW: The heading negated, then INX; the INY it makes is undone when Y is
 * read back.
 * Entry: as HeadingFlat. Exit: as HeadingFlat.
 */
uint8_t HeadingFlatMirrored(uint8_t bank, uint16_t x, uint16_t y)
{
    (void)bank;
    uint8_t i, v;

    head_v = -1;
    cyc(30);
    wr8(W(heading_speed), cyc_then(16, rd8(0x7E0000 | (uint16_t)(W(obj_17) + x))));
    cyc(30);
    i = rd8(0x7E0000 | (uint16_t)(W(obj_15) + x));
    save_xy(x, y);
    cyc(16);
    wr8(W(dp_08), 0x00);
    cyc(16);
    wr8(W(dp_08) + 1, 0x00);
    cyc(80);
    v = flat((uint8_t)(~i + 2));
    finish();
    return v;
}

void HeadingFlatMirrored_cr(Carried *cr)
{
    heading_regs(cr, HeadingFlatMirrored(cr->db, cr->x, cr->y));
}

void abi_HeadingFlatMirrored(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    HeadingFlatMirrored_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
uint8_t HeadingPitched(uint8_t bank, uint16_t x, uint16_t y)
{
    (void)bank;
    uint8_t h, pitch, v;

    head_v = -1;
    cyc(30);
    wr8(W(heading_speed), cyc_then(16, rd8(0x7E0000 | (uint16_t)(W(obj_17) + x))));
    cyc(16);
    wr8(W(dp_02) + 1, 0x00);
    cyc(16);
    wr8(W(dp_08) + 1, 0x00);
    cyc(16);
    wr8(W(dp_94) + 1, 0x00);
    save_xy(x, y);
    cyc(30);
    h = rd8(0x7E0000 | (uint16_t)(W(obj_15) + x));
    cyc(44);
    pitch = rd8(0x7E0000 | (uint16_t)(W(obj_14) + x));
    cyc(118);
    v = pitched(h, (uint8_t)(~pitch + 1));
    finish();
    return v;
}

void HeadingPitched_cr(Carried *cr)
{
    heading_regs(cr, HeadingPitched(cr->db, cr->x, cr->y));
}

void abi_HeadingPitched(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    HeadingPitched_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* HeadingPitchedFrom1F7C from its second entry on: the angles from $1F7C
   (pitch) and $1F7D (heading) through data bank 0. */
static uint8_t pitched_1f7c(uint16_t x, uint16_t y)
{
    uint8_t h, pitch, v;

    cyc(16);
    wr8(W(dp_02) + 1, 0x00);
    cyc(16);
    wr8(W(dp_08) + 1, 0x00);
    cyc(16);
    wr8(W(dp_94) + 1, 0x00);
    save_xy(x, y);
    cyc(112);
    h = rd8(W(heading_turn));
    cyc(68);
    pitch = rd8(W(heading_pitch));
    v = pitched((uint8_t)(~h + 1), pitch);
    finish();
    return v;
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
uint8_t HeadingPitchedVia1F7C(uint8_t bank, uint16_t x, uint16_t y)
{
    (void)bank;

    head_v = -1;
    cyc(30);
    wr8(W(heading_speed), cyc_then(16, rd8(0x7E0000 | (uint16_t)(W(obj_17) + x))));
    cyc(30);
    wr8(0x7E0000 | W(heading_turn), cyc_then(24, rd8(0x7E0000 | (uint16_t)(W(obj_15) + x))));
    cyc(30);
    wr8(0x7E0000 | W(heading_pitch), cyc_then(24, rd8(0x7E0000 | (uint16_t)(W(obj_14) + x))));
    return pitched_1f7c(x, y);
}

void HeadingPitchedVia1F7C_cr(Carried *cr)
{
    heading_regs(cr, HeadingPitchedVia1F7C(cr->db, cr->x, cr->y));
}

void abi_HeadingPitchedVia1F7C(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    HeadingPitchedVia1F7C_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:BCC7 HeadingPitchedFrom1F7C
 * WHAT IT DOES: The velocity for the speed in $82, heading -($1F7D) and
 * pitch $1F7C: $02 = (speed sin h) cos p, $94 = (speed cos h) cos p, $08 =
 * speed sin p.
 * HOW: As HeadingPitched, the angles read with the data bank already 0.
 * Entry: M 8-bit, X 16-bit, D = $0000; $82 = speed. Exit: as HeadingPitched.
 */
uint8_t HeadingPitchedFrom1F7C(uint16_t x, uint16_t y)
{
    head_v = -1;
    return pitched_1f7c(x, y);
}

void HeadingPitchedFrom1F7C_cr(Carried *cr)
{
    heading_regs(cr, HeadingPitchedFrom1F7C(cr->x, cr->y));
}

void abi_HeadingPitchedFrom1F7C(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    HeadingPitchedFrom1F7C_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:BE8D HeadingFlatQuarterTurn
 * WHAT IT DOES: As HeadingFlat for the speed A and the heading $41 - (+$15)
 * of object X: a quarter turn from its mirror, one step on.
 * HOW: SEC / LDA #$40 / SBC (the subtract sets V), then INX.
 * Entry: M 8-bit, X 16-bit, D = $0000; A = speed (`speed`), X = object.
 * Exit: as HeadingFlat.
 */
uint8_t HeadingFlatQuarterTurn(uint8_t bank, uint8_t speed, uint16_t x, uint16_t y)
{
    (void)bank;
    uint8_t i, v;

    head_v = -1;
    cyc(16);
    wr8(W(heading_speed), speed);
    save_xy(x, y);
    cyc(16);
    wr8(W(dp_08), 0x00);
    cyc(16);
    wr8(W(dp_08) + 1, 0x00);
    cyc(60);
    i = sbc8(0x40, rd8(0x7E0000 | (uint16_t)(W(obj_15) + x)));
    cyc(20);
    v = flat((uint8_t)(i + 1));
    finish();
    return v;
}

void HeadingFlatQuarterTurn_cr(Carried *cr)
{
    heading_regs(cr, HeadingFlatQuarterTurn(cr->db, (uint8_t)cr->a, cr->x, cr->y));
}

void abi_HeadingFlatQuarterTurn(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    HeadingFlatQuarterTurn_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:BF62 HeadingFlatSpeedA
 * WHAT IT DOES: As HeadingFlat for the speed A and the heading (+$15) + 1
 * of object X.
 * HOW: As HeadingFlat, with INX.
 * Entry: as HeadingFlatQuarterTurn. Exit: as HeadingFlat.
 */
uint8_t HeadingFlatSpeedA(uint8_t bank, uint8_t speed, uint16_t x, uint16_t y)
{
    (void)bank;
    uint8_t i, v;

    head_v = -1;
    cyc(16);
    wr8(W(heading_speed), speed);
    save_xy(x, y);
    cyc(16);
    wr8(W(dp_08), 0x00);
    cyc(16);
    wr8(W(dp_08) + 1, 0x00);
    cyc(30);
    i = rd8(0x7E0000 | (uint16_t)(W(obj_15) + x));
    cyc(36);
    v = flat((uint8_t)(i + 1));
    finish();
    return v;
}

void HeadingFlatSpeedA_cr(Carried *cr)
{
    heading_regs(cr, HeadingFlatSpeedA(cr->db, (uint8_t)cr->a, cr->x, cr->y));
}

void abi_HeadingFlatSpeedA(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    HeadingFlatSpeedA_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* The flags N, Z, C, V of the last 16-bit add, for move_regs. */
static uint8_t move_flags;

static uint16_t add16(uint16_t a, uint16_t m)
{
    uint32_t r = (uint32_t)a + m;

    move_flags = (uint8_t)(((r & 0x8000) ? P_N : 0) | ((r & 0xFFFF) == 0 ? P_Z : 0) |
                           (r > 0xFFFF ? P_C : 0) | ((~(a ^ m) & (a ^ r) & 0x8000) ? P_V : 0));
    return (uint16_t)r;
}

/* Object X's position (+$0C, +$0E, +$10) plus the words at $02, $08 and
   $94, in that order; returns the new +$10. */
static uint16_t move_by(uint16_t x)
{
    uint16_t v;

    cyc(52);
    v = rd16(0x7E0000 | (uint16_t)(W(obj_pos_x) + x));
    cyc(30);
    v = add16(v, rd16(W(dp_02)));
    cyc(30);
    wr16(0x7E0000 | (uint16_t)(W(obj_pos_x) + x), v);
    cyc(30);
    v = rd16(0x7E0000 | (uint16_t)(W(obj_pos_y) + x));
    cyc(30);
    v = add16(v, rd16(W(dp_08)));
    cyc(30);
    wr16(0x7E0000 | (uint16_t)(W(obj_pos_y) + x), v);
    cyc(30);
    v = rd16(0x7E0000 | (uint16_t)(W(obj_pos_z) + x));
    cyc(30);
    v = add16(v, rd16(W(dp_94)));
    cyc(30);
    wr16(0x7E0000 | (uint16_t)(W(obj_pos_z) + x), v);
    return v;
}

/* The moves' registers: A the new +$10, M 8-bit, N, Z, C, V from its add. */
static void move_regs(Carried *cr, uint16_t v)
{
    cr->a = v;
    set_p(cr, (uint8_t)((cr->p & ~(P_N | P_Z | P_C | P_V)) | move_flags | P_M));
}

/* ASL of the three words at $02, $08, $94 in that order. */
static void double_vector(void)
{
    static const uint32_t at[3] = {W(dp_02), W(dp_08), W(dp_94)};
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
 * Entry: X 16-bit, D = $0000; X = object; `bank` the caller's data bank.
 * Exit: the return value is A = the new +$10; M 8-bit; N, Z, C, V from its
 * add (the adapter has them).
 */
uint16_t ObjectMoveByVectorTimes4(uint8_t bank, uint16_t x)
{
    (void)bank;
    uint16_t v;

    (cyc(22), double_vector());
    double_vector();
    v = move_by(x);
    cyc(66);
    return v;
}

void ObjectMoveByVectorTimes4_cr(Carried *cr)
{
    move_regs(cr, ObjectMoveByVectorTimes4(cr->db, cr->x));
}

void abi_ObjectMoveByVectorTimes4(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectMoveByVectorTimes4_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:C044 ObjectMoveByVectorTimes2
 * WHAT IT DOES: Moves object X by twice the vector at $02, $08, $94, which
 * is left doubled.
 * HOW: Each word shifted left once, then ObjectMoveByVector.
 * Entry: as ObjectMoveByVectorTimes4. Exit: as ObjectMoveByVectorTimes4.
 */
uint16_t ObjectMoveByVectorTimes2(uint8_t bank, uint16_t x)
{
    (void)bank;
    uint16_t v;

    (cyc(22), double_vector());
    v = move_by(x);
    cyc(66);
    return v;
}

void ObjectMoveByVectorTimes2_cr(Carried *cr)
{
    move_regs(cr, ObjectMoveByVectorTimes2(cr->db, cr->x));
}

void abi_ObjectMoveByVectorTimes2(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectMoveByVectorTimes2_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:C04C ObjectMoveByVector
 * WHAT IT DOES: Moves object X by the vector at $02, $08, $94: adds them
 * to its +$0C, +$0E and +$10.
 * HOW: Three 16-bit adds, through the caller's data bank.
 * Entry: as ObjectMoveByVectorTimes4. Exit: as ObjectMoveByVectorTimes4.
 */
uint16_t ObjectMoveByVector(uint8_t bank, uint16_t x)
{
    (void)bank;
    uint16_t v = move_by(x);

    cyc(66);
    return v;
}

void ObjectMoveByVector_cr(Carried *cr)
{
    move_regs(cr, ObjectMoveByVector(cr->db, cr->x));
}

void abi_ObjectMoveByVector(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectMoveByVector_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* The carry the velocity routines' last shift leaves (set, as
   HeadingPitchedFrom1F7C leaves it, when there is none). */
static int vel_carry;

/* Object X's velocity (+$2E, +$30, +$32) from the vector at $02, $08, $94
   after HeadingPitchedFrom1F7C, each word shifted left `shift` times (each
   shift a 16-bit ASL of A). Returns A, the last word. */
static uint16_t velocity_from_vector(uint16_t x, uint16_t y, int shift)
{
    static const uint32_t from[3] = {W(dp_02), W(dp_08), W(dp_94)};
    uint16_t v = 0;
    unsigned i;
    int k;

    cyc(62);
    HeadingPitchedFrom1F7C(x, y);
    vel_carry = 1;
    cyc(22);
    for (i = 0; i < 3; i++) {
        cyc(16);
        v = rd16(from[i]);
        for (k = 0; k < shift; k++) {
            cyc(14);
            vel_carry = v >> 15;
            v = (uint16_t)(v << 1);
        }
        cyc(30);
        wr16(0x7E0000 | (uint16_t)(W(obj_vel_x) + 2 * i + x), v);
    }
    cyc(66);
    return v;
}

/* The velocity routines' registers: A the last word, M 8-bit, X 16-bit;
   N, Z from A; C and V as left. */
static void vel_regs(Carried *cr, uint16_t v)
{
    uint8_t p = heading_p(cr->p);

    p = (uint8_t)((p & ~P_C) | (vel_carry ? P_C : 0));
    set_p(cr, (uint8_t)(nz16(p, v) | P_M));
    cr->a = v;
}

/*
 * @routine 03:CD1D ObjectVelocityFromHeading
 * WHAT IT DOES: Sets object X's velocity (+$2E, +$30, +$32) to the vector
 * HeadingPitchedFrom1F7C makes for the speed in $82 and the angles at
 * $1F7C/$1F7D.
 * HOW: The vector's words copied, 16-bit, through the caller's data bank.
 * Entry: M 8-bit, X 16-bit, D = $0000; X = object. Exit: the return value
 * is A = the z word; M 8-bit; N, Z from it; C, V as HeadingPitchedFrom1F7C
 * leaves them (the adapter has them).
 */
uint16_t ObjectVelocityFromHeading(uint8_t bank, uint16_t x, uint16_t y)
{
    (void)bank;
    return velocity_from_vector(x, y, 0);
}

void ObjectVelocityFromHeading_cr(Carried *cr)
{
    vel_regs(cr, ObjectVelocityFromHeading(cr->db, cr->x, cr->y));
}

void abi_ObjectVelocityFromHeading(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectVelocityFromHeading_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:CD35 ObjectVelocityFromHeadingTimes2
 * WHAT IT DOES: As ObjectVelocityFromHeading with each word doubled.
 * HOW: One ASL A on each word before it is stored.
 * Entry: as ObjectVelocityFromHeading. Exit: as ObjectVelocityFromHeading,
 * C from the last shift.
 */
uint16_t ObjectVelocityFromHeadingTimes2(uint8_t bank, uint16_t x, uint16_t y)
{
    (void)bank;
    return velocity_from_vector(x, y, 1);
}

void ObjectVelocityFromHeadingTimes2_cr(Carried *cr)
{
    vel_regs(cr, ObjectVelocityFromHeadingTimes2(cr->db, cr->x, cr->y));
}

void abi_ObjectVelocityFromHeadingTimes2(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectVelocityFromHeadingTimes2_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:CD50 ObjectVelocityFromHeadingTimes4
 * WHAT IT DOES: As ObjectVelocityFromHeading with each word times 4.
 * HOW: Two ASL A on each word.
 * Entry: as ObjectVelocityFromHeading. Exit: as
 * ObjectVelocityFromHeadingTimes2.
 */
uint16_t ObjectVelocityFromHeadingTimes4(uint8_t bank, uint16_t x, uint16_t y)
{
    (void)bank;
    return velocity_from_vector(x, y, 2);
}

void ObjectVelocityFromHeadingTimes4_cr(Carried *cr)
{
    vel_regs(cr, ObjectVelocityFromHeadingTimes4(cr->db, cr->x, cr->y));
}

void abi_ObjectVelocityFromHeadingTimes4(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectVelocityFromHeadingTimes4_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:CD6E ObjectVelocityFromHeadingTimes8
 * WHAT IT DOES: As ObjectVelocityFromHeading with each word times 8.
 * HOW: Three ASL A on each word.
 * Entry: as ObjectVelocityFromHeading. Exit: as
 * ObjectVelocityFromHeadingTimes2.
 */
uint16_t ObjectVelocityFromHeadingTimes8(uint8_t bank, uint16_t x, uint16_t y)
{
    (void)bank;
    return velocity_from_vector(x, y, 3);
}

void ObjectVelocityFromHeadingTimes8_cr(Carried *cr)
{
    vel_regs(cr, ObjectVelocityFromHeadingTimes8(cr->db, cr->x, cr->y));
}

void abi_ObjectVelocityFromHeadingTimes8(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectVelocityFromHeadingTimes8_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:D1C2 ObjectVelocityLevelFromDp
 * WHAT IT DOES: Sets object X's velocity to the vector for the speed in
 * $94, the pitch in $02 and the heading -$40 (HeadingPitchedFrom1F7C),
 * with the z velocity (+$32) cleared instead of set.
 * HOW: $02 to $1F7C, $94 to $82, $40 to $1F7D (8-bit, the first and last
 * through the caller's data bank), then the words at $02 and $08 copied to
 * +$2E and +$30.
 * Entry: M 8-bit, X 16-bit, D = $0000; X = object. Exit: the return value
 * is A = the word at $08; M 8-bit; N, Z from it; C, V as
 * HeadingPitchedFrom1F7C leaves them (the adapter has them).
 */
uint16_t ObjectVelocityLevelFromDp(uint8_t bank, uint16_t x, uint16_t y)
{
    (void)bank;
    uint16_t v;

    cyc(16);
    wr8(0x7E0000 | W(heading_pitch), cyc_then(24, rd8(W(dp_02))));
    cyc(16);
    wr8(W(heading_speed), cyc_then(16, rd8(W(dp_94))));
    cyc(40);
    wr8(0x7E0000 | W(heading_turn), 0x40);
    cyc(62);
    HeadingPitchedFrom1F7C(x, y);
    cyc(38);
    v = rd16(W(dp_02));
    cyc(30);
    wr16(0x7E0000 | (uint16_t)(W(obj_vel_x) + x), v);
    cyc(16);
    v = rd16(W(dp_08));
    cyc(30);
    wr16(0x7E0000 | (uint16_t)(W(obj_vel_y) + x), v);
    cyc(30);
    wr16(0x7E0000 | (uint16_t)(W(obj_vel_z) + x), 0x0000);
    cyc(66);
    return v;
}

void ObjectVelocityLevelFromDp_cr(Carried *cr)
{
    vel_carry = 1;
    vel_regs(cr, ObjectVelocityLevelFromDp(cr->db, cr->x, cr->y));
}

void abi_ObjectVelocityLevelFromDp(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectVelocityLevelFromDp_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

