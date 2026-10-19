/*
 * sprite_slots.c: eight 8-byte records at $0595-$05D4, after the work-RAM
 * sprite table, and the mask of the ones in use at $05D5 (bit i for the
 * record at $0595 + i * 8). $06D8 points at the first one in use.
 * Fields used here: +$00, +$04, +$05, +$06 and +$07 (bytes).
 */
#include "machine.h"
#include "cpu_ops.h"

#define SLOT_MASK 0x05D5u
#define SLOTS 0x0595u

static uint32_t dbw(uint8_t bank, uint16_t a)
{
    return (uint32_t)bank << 16 | a;
}

/*
 * @routine 02:D932 SpriteSlotFirst
 * WHAT IT DOES: Sets $06D8 to the address of the first record in use (the
 * lowest set bit of $05D5), or 0 when none is. Not reached in the traces
 * so far with any record in use.
 * HOW: The mask is copied to the word at $1F2B ($1F2C cleared) and shifted
 * right through the carry (ROR, so the entry carry goes into bit 15)
 * eight times, the address stepping by 8 from $0595; work RAM through the
 * data bank.
 * Entry: M 8-bit, X 16-bit. Exit: A = $06D8's new value; M 16-bit. Found:
 * X = 8 - the bit number; N, Z from the shifted word; C set. None: X
 * unchanged, or 0 after eight shifts; Z set, N clear; C from the last
 * shift or add.
 */
uint16_t SpriteSlotFirst(uint8_t bank, uint16_t *x, uint8_t *p)
{
    uint16_t a = 0x0000, w;
    uint8_t m;
    int found = 0;

    wr8(dbw(bank, 0x1F2C), cyc_then(24, 0x00));
    m = rd8(cyc_then(24, dbw(bank, SLOT_MASK)));
    wr8(dbw(bank, 0x1F2B), cyc_then(24, m));
    *p &= (uint8_t)~P_M;
    if (m) {
        cyc(10);
        a = SLOTS;
        for (*x = 8; *x; ) {
            uint16_t c = (*p & P_C) ? 0x8000 : 0;

            w = rd16(cyc_then(100, dbw(bank, 0x1F2B)));
            *p = (uint8_t)((*p & ~P_C) | (w & 1 ? P_C : 0));
            w = (uint16_t)(c | w >> 1);
            wr16(dbw(bank, 0x1F2B), cyc_then(14, w));
            *p = nz16(*p, w);
            if (*p & P_C) {
                found = 1;
                break;
            }
            {
                unsigned s = (unsigned)a + 8;
                uint16_t r = (uint16_t)s;

                *p = (uint8_t)((*p & ~(P_C | P_V)) | (s > 0xFFFF ? P_C : 0) | ((~(a ^ 8) & (a ^ r) & 0x8000) ? P_V : 0));
                a = r;
            }
            *x = (uint16_t)(*x - 1);
            *p = nz16(*p, *x);
        }
    } else { cyc(46); }
    if (!found) {
        a = 0x0000;
        *p = nz16(*p, a);
    }
    wr16(dbw(bank, 0x06D8), cyc_then(46, a));
    return a;
}

void abi_SpriteSlotFirst(void)
{
    uint8_t p = cpu.p;

    cpu.a = SpriteSlotFirst(cpu.db, &cpu.x, &p);
    cpu.p = p;
}

/*
 * @routine 02:D928 SpriteSlotFirstLong
 * WHAT IT DOES: SpriteSlotFirst for callers in other banks.
 * HOW: 8-bit A, 16-bit index registers around the call; P is saved and
 * restored.
 * Entry: any widths. Exit: A, X as SpriteSlotFirst; P unchanged.
 */
uint16_t SpriteSlotFirstLong(uint8_t bank, uint16_t *x)
{
    uint8_t p = 0;

    return SpriteSlotFirst(bank, x, &p);
}

void abi_SpriteSlotFirstLong(void)
{
    cyc(174);
    uint8_t p = (uint8_t)((cpu.p | P_M) & ~P_X);
    uint16_t x = cpu.x;

    cpu.a = SpriteSlotFirst(cpu.db, &x, &p);
    cpu.x = (cpu.p & P_X) ? (uint8_t)x : (cyc(176), x);
}

/*
 * @routine 02:E0A9 SpriteSlot1Countdown
 * WHAT IT DOES: Runs record 1 ($059D) while the count at $08FB is not $FF:
 * marks it in use and sets +$04 = 0, +$05 = 2, +$06 = $A7, +$00 = +$07 OR
 * $E0. While the count is not 0 it is stepped down, with +$06 = $E7 and
 * +$00 = $FF. At 0: the first time ($7E:FF65 = 0) $7E:FF65 is set to 1 and
 * the same is done; later +$07 steps down by 2 each call until 0, then the
 * count steps on to $FF. A count of $FF marks the record unused. The paths
 * at 0 are not reached in the traces so far.
 * HOW: The record's fields as direct page offsets indexed by X = $059D (D
 * = $0000); work RAM through the data bank. P is saved and restored.
 * Entry: D = $0000. Exit: A low byte = the last value loaded; X = $059D,
 * unchanged when the count is $FF; P unchanged.
 */
uint8_t SpriteSlot1Countdown(uint8_t bank, uint16_t *x)
{
    uint16_t r = 0x059D;
    uint8_t a;

    a = (uint8_t)(rd8(cyc_then(90, dbw(bank, 0x08FB))) + 1);
    if (!a) {
        a = (uint8_t)(rd8(cyc_then(82, dbw(bank, SLOT_MASK))) & 0xFD);
        wr8(dbw(bank, SLOT_MASK), cyc_then(40, a));
        return a;
    }
    wr8(dbw(bank, SLOT_MASK), cyc_then(40, (uint8_t)(rd8(cyc_then(98, dbw(bank, SLOT_MASK))) | 0x02)));
    *x = r;
    cyc(90);
    wr8((uint16_t)(r + 4), 0x00);
    cyc(38);
    wr8((uint16_t)(r + 5), 0x02);
    cyc(38);
    wr8((uint16_t)(r + 6), 0xA7);
    cyc(22);
    wr8((uint16_t)(r + 0), cyc_then(38, (uint8_t)(rd8((uint16_t)(r + 7)) | 0xE0)));
    if (!rd8(cyc_then(24, dbw(bank, 0x08FB)))) {
        cyc(48);
        a = rd8(0x7EFF65);
        if (a) {
            cyc(38);
            a = rd8((uint16_t)(r + 7));
            if (!a)
                { cyc(22); goto count; }
            a = (uint8_t)(a - 2);
            cyc(66);
            wr8((uint16_t)(r + 7), a);
            return a;
        }
        cyc(68);
        wr8(0x7EFF65, (uint8_t)(a + 1));
    }
    cyc(44);
    wr8((uint16_t)(r + 4), 0x00);
    cyc(38);
    wr8((uint16_t)(r + 6), 0xE7);
    a = 0xFF;
    cyc(38);
    wr8((uint16_t)(r + 0), a);
count:
    wr8(dbw(bank, 0x08FB), cyc_then(6, (uint8_t)(rd8(cyc_then(24, dbw(bank, 0x08FB))) - 1)));
    return a;
}

void abi_SpriteSlot1Countdown(void)
{
    uint16_t x = cpu.x;
    uint8_t a = SpriteSlot1Countdown(cpu.db, &x);

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | a);
    cpu.x = (cpu.p & P_X) ? (uint8_t)x : (cyc(72), x);
}

/*
 * @routine 7E:F33C SpriteSlot4Brighten
 * WHAT IT DOES: Shows record 4 ($05B5) and brightens it a step: marks it in
 * use and sets +$04 = 0, +$05 = 2, +$06 = $E6, +$00 = +$07 OR $E0, then
 * steps +$07 up by 1 until it reaches $0F. Nothing in the game calls it.
 * HOW: The record's fields as direct page offsets indexed by X = $05B5 (D
 * = $0000); the mask through the data bank. P and X are saved and restored.
 * Entry: D = $0000. Exit: A low byte = +$07's new value; X, P unchanged.
 */
uint8_t SpriteSlot4Brighten(uint8_t bank)
{
    uint16_t r = 0x05B5;
    uint8_t a;

    a = rd8(cyc_then(164, dbw(bank, SLOT_MASK)));
    wr8(dbw(bank, SLOT_MASK), cyc_then(40, (uint8_t)(a | 0x10)));
    cyc(90);
    wr8((uint16_t)(r + 4), 0x00);
    cyc(38);
    wr8((uint16_t)(r + 5), 0x02);
    cyc(38);
    wr8((uint16_t)(r + 6), 0xE6);
    cyc(38);
    a = rd8((uint16_t)(r + 7));
    cyc(22);
    wr8((uint16_t)(r + 0), (uint8_t)(a | 0xE0));
    cyc(22);
    a = rd8((uint16_t)(r + 7));
    if (a != 0x0F) {
        a = (uint8_t)(a + 1);
        cyc(68);
        wr8((uint16_t)(r + 7), a);
    } else { cyc(38); }
    cyc(106);
    return a;
}

void abi_SpriteSlot4Brighten(void)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | SpriteSlot4Brighten(cpu.db));
}

/*
 * @routine 7E:F371 SpriteSlot4Darken
 * WHAT IT DOES: Shows record 4 ($05B5) and darkens it a step: marks it in
 * use and sets +$04 = 0, +$05 = 2, +$06 = $E6, +$00 = +$07 OR $E0, then
 * steps +$07 down by 1; once +$07 is 0 the record is marked unused
 * instead. Nothing in the game calls it.
 * HOW: The record's fields as direct page offsets indexed by X = $05B5 (D
 * = $0000); the mask through the data bank. P and X are saved and restored.
 * Entry: D = $0000. Exit: A low byte = +$07's new value, or the new mask
 * at 0; X, P unchanged.
 */
uint8_t SpriteSlot4Darken(uint8_t bank)
{
    uint16_t r = 0x05B5;
    uint8_t a;

    a = rd8(cyc_then(164, dbw(bank, SLOT_MASK)));
    wr8(dbw(bank, SLOT_MASK), cyc_then(40, (uint8_t)(a | 0x10)));
    cyc(90);
    wr8((uint16_t)(r + 4), 0x00);
    cyc(38);
    wr8((uint16_t)(r + 5), 0x02);
    cyc(38);
    wr8((uint16_t)(r + 6), 0xE6);
    cyc(38);
    a = rd8((uint16_t)(r + 7));
    cyc(22);
    wr8((uint16_t)(r + 0), (uint8_t)(a | 0xE0));
    cyc(22);
    a = rd8((uint16_t)(r + 7));
    if (a) {
        a = (uint8_t)(a - 1);
        cyc(52);
        wr8((uint16_t)(r + 7), a);
        return a;
    }
    a = rd8(cyc_then(68, dbw(bank, SLOT_MASK)));
    a = (uint8_t)(a & 0xEF);
    wr8(dbw(bank, SLOT_MASK), cyc_then(40, a));
    return a;
}

void abi_SpriteSlot4Darken(void)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | SpriteSlot4Darken(cpu.db));
}

/*
 * @routine 7E:F3B5 SpriteSlot4Full
 * WHAT IT DOES: Shows record 4 ($05B5) at full brightness: marks it in use
 * and sets +$04 = 0, +$05 = 2, +$06 = $E6, +$00 = $FF and +$07 = $1F.
 * Nothing in the game calls it.
 * HOW: The record's fields as direct page offsets indexed by X = $05B5 (D
 * = $0000); the mask through the data bank. P and X are saved and restored.
 * Entry: D = $0000. Exit: A low byte = $1F; X, P unchanged.
 */
uint8_t SpriteSlot4Full(uint8_t bank)
{
    uint16_t r = 0x05B5;
    uint8_t a;

    a = rd8(cyc_then(164, dbw(bank, SLOT_MASK)));
    wr8(dbw(bank, SLOT_MASK), cyc_then(40, (uint8_t)(a | 0x10)));
    cyc(90);
    wr8((uint16_t)(r + 4), 0x00);
    cyc(38);
    wr8((uint16_t)(r + 5), 0x02);
    cyc(38);
    wr8((uint16_t)(r + 6), 0xE6);
    cyc(38);
    wr8((uint16_t)(r + 0), 0xFF);
    cyc(38);
    wr8((uint16_t)(r + 7), 0x1F);
    return 0x1F;
}

void abi_SpriteSlot4Full(void)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | SpriteSlot4Full(cpu.db));
}

void abi_SpriteSlot1Countdown(void);

/* Marks record r in use (mask bit) and sets +$04 = 0, +$05 = 2, +$06 = k. */
static void slot_show(uint8_t bank, uint16_t r, uint8_t bit, uint8_t k)
{
    uint8_t a;

    a = rd8(cyc_then(164, dbw(bank, SLOT_MASK)));
    wr8(dbw(bank, SLOT_MASK), cyc_then(40, (uint8_t)(a | bit)));
    cyc(90);
    wr8((uint16_t)(r + 4), 0x00);
    cyc(38);
    wr8((uint16_t)(r + 5), 0x02);
    cyc(38);
    wr8((uint16_t)(r + 6), k);
}

/*
 * @routine 02:E046 SpriteSlot2Brighten
 * WHAT IT DOES: Shows record 2 ($05A5) and brightens it a step: marks it in
 * use and sets +$04 = 0, +$05 = 2, +$06 = $07, +$00 = +$07 OR $E0, then
 * steps +$07 up by 1 until it reaches $1F. Nothing in the game calls it.
 * HOW: As SpriteSlot4Brighten, for record 2.
 * Entry: D = $0000. Exit: A low byte = +$07's new value; X, P unchanged.
 */
uint8_t SpriteSlot2Brighten(uint8_t bank)
{
    uint16_t r = 0x05A5;
    uint8_t a;

    slot_show(bank, r, 0x04, 0x07);
    cyc(38);
    a = rd8((uint16_t)(r + 7));
    cyc(22);
    wr8((uint16_t)(r + 0), (uint8_t)(a | 0xE0));
    cyc(22);
    a = rd8((uint16_t)(r + 7));
    if (a != 0x1F) {
        a = (uint8_t)(a + 1);
        cyc(68);
        wr8((uint16_t)(r + 7), a);
    } else { cyc(38); }
    cyc(106);
    return a;
}

void abi_SpriteSlot2Brighten(void)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | SpriteSlot2Brighten(cpu.db));
}

/*
 * @routine 02:E042 SpriteSlot2BrightenLong
 * WHAT IT DOES: SpriteSlot2Brighten for callers in other banks.
 * HOW: JSR, RTL.
 * Entry: D = $0000. Exit: as SpriteSlot2Brighten.
 */
void abi_SpriteSlot2BrightenLong(void)
{
    (cyc(46), abi_SpriteSlot2Brighten());
}

/*
 * @routine 02:E07B SpriteSlot1Start
 * WHAT IT DOES: Starts record 1's countdown: $08FB = $28, record 1 marked
 * in use with +$07 = $1E and +$04 = 0, then a first SpriteSlot1Countdown;
 * $084C is cleared. Nothing in the game calls it.
 * HOW: 8-bit stores through the data bank and direct page (D = $0000).
 * P is saved and restored.
 * Entry: D = $0000. Exit: A low byte and X as SpriteSlot1Countdown leaves
 * them (X's low byte when X is 8-bit); P unchanged.
 */
void abi_SpriteSlot1Start(void)
{
    uint8_t b = cpu.db, p = cpu.p, a;

    wr8(dbw(b, 0x08FB), cyc_then(106, 0x28));
    a = rd8(cyc_then(68, dbw(b, SLOT_MASK)));
    wr8(dbw(b, SLOT_MASK), cyc_then(40, (uint8_t)(a | 0x02)));
    cpu.x = 0x059D;
    cyc(106);
    wr8(0x05A4, 0x1E);
    cyc(22);
    wr8(0x05A1, 0x00);
    cpu.p = (uint8_t)((cpu.p | P_M) & ~P_X);
    (cyc(62), abi_SpriteSlot1Countdown());
    wr8(dbw(b, 0x084C), cyc_then(46, 0x00));
    op_plp(p);
}

/* Records 5 and 6 with +$01, +$02, +$03 and +$00: the shared stores. */
static void slot_colour(uint8_t bank, uint16_t r, uint8_t bit, uint8_t c1, uint8_t c23, int c1_last)
{
    slot_show(bank, r, bit, 0x27);
    if (!c1_last) {
        cyc(22);
        wr8((uint16_t)(r + 1), c1);
        cyc(38);
        wr8((uint16_t)(r + 2), c23);
        cyc(22);
        wr8((uint16_t)(r + 3), c23);
        cyc(22);
        wr8((uint16_t)(r + 0), 0x00);
    } else {
        cyc(22);
        wr8((uint16_t)(r + 0), 0x00);
        cyc(38);
        wr8((uint16_t)(r + 1), c1);
        cyc(22);
        wr8((uint16_t)(r + 2), c23);
        cyc(22);
        wr8((uint16_t)(r + 3), c23);
    }
cyc(108);
    }

/*
 * @routine 02:E117 SpriteSlot6Yellow
 * WHAT IT DOES: Shows record 6 ($05C5) with +$04 = 0, +$05 = 2, +$06 =
 * $27, +$01 = 0, +$02 = +$03 = $1F and +$00 = 0. Nothing in the game
 * calls it.
 * HOW: 8-bit stores through the data bank and direct page (D = $0000);
 * P and X are saved and restored.
 * Entry: D = $0000. Exit: A low byte = $1F; X, P unchanged.
 */
void abi_SpriteSlot6Yellow(void)
{
    slot_colour(cpu.db, 0x05C5, 0x40, 0x00, 0x1F, 0);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x1F);
}

/*
 * @routine 02:E147 SpriteSlot6DimYellow
 * WHAT IT DOES: As SpriteSlot6Yellow with +$02 = +$03 = $07. Nothing in
 * the game calls it.
 * HOW: As SpriteSlot6Yellow.
 * Entry: D = $0000. Exit: A low byte = $07; X, P unchanged.
 */
void abi_SpriteSlot6DimYellow(void)
{
    slot_colour(cpu.db, 0x05C5, 0x40, 0x00, 0x07, 0);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x07);
}

/*
 * @routine 02:E177 SpriteSlot6Red
 * WHAT IT DOES: Shows record 6 ($05C5) with +$04 = 0, +$05 = 2, +$06 =
 * $27, +$00 = 0, +$01 = $1F and +$02 = +$03 = 0. Nothing in the game
 * calls it.
 * HOW: As SpriteSlot6Yellow, +$00 stored first.
 * Entry: D = $0000. Exit: A low byte = $1F; X, P unchanged.
 */
void abi_SpriteSlot6Red(void)
{
    slot_colour(cpu.db, 0x05C5, 0x40, 0x1F, 0x00, 1);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x1F);
}

/*
 * @routine 02:E1A7 SpriteSlot5DimRed
 * WHAT IT DOES: Shows record 5 ($05BD) with +$04 = 0, +$05 = 2, +$06 =
 * $27, +$00 = 0, +$01 = $0A and +$02 = +$03 = 0. Nothing in the game
 * calls it.
 * HOW: As SpriteSlot6Red, for record 5.
 * Entry: D = $0000. Exit: A low byte = $0A; X, P unchanged.
 */
void abi_SpriteSlot5DimRed(void)
{
    slot_colour(cpu.db, 0x05BD, 0x20, 0x0A, 0x00, 1);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x0A);
}

/*
 * @routine 02:E1D7 SpriteSlot5Off
 * WHAT IT DOES: Marks record 5 unused. Nothing in the game calls it.
 * HOW: Clears bit 5 of the mask through the data bank; P and X are saved
 * and restored.
 * Entry: any widths. Exit: A low byte = the new mask; X, P unchanged.
 */
void abi_SpriteSlot5Off(void)
{
    uint8_t a;

    a = rd8(cyc_then(142, dbw(cpu.db, SLOT_MASK)));
    a = (uint8_t)(a & 0xDF);
    wr8(dbw(cpu.db, SLOT_MASK), cyc_then(40, a));
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | a);
}

/*
 * @routine 02:E215 SpriteSlot2Darken
 * WHAT IT DOES: Shows record 2 ($05A5) and darkens it a step: marks it in
 * use and sets +$04 = 0, +$05 = 2, +$06 = $07, +$00 = +$07 OR $E0, then
 * steps +$07 down by 1; once +$07 is 0 the record is marked unused
 * instead. Nothing in the game calls it.
 * HOW: As SpriteSlot4Darken, for record 2.
 * Entry: D = $0000. Exit: A low byte = +$07's new value, or the new mask
 * at 0; X, P unchanged.
 */
uint8_t SpriteSlot2Darken(uint8_t bank)
{
    uint16_t r = 0x05A5;
    uint8_t a;

    slot_show(bank, r, 0x04, 0x07);
    cyc(38);
    a = rd8((uint16_t)(r + 7));
    cyc(22);
    wr8((uint16_t)(r + 0), (uint8_t)(a | 0xE0));
    cyc(22);
    a = rd8((uint16_t)(r + 7));
    if (a) {
        a = (uint8_t)(a - 1);
        cyc(52);
        wr8((uint16_t)(r + 7), a);
        return a;
    }
    a = rd8(cyc_then(68, dbw(bank, SLOT_MASK)));
    a = (uint8_t)(a & 0xFB);
    wr8(dbw(bank, SLOT_MASK), cyc_then(40, a));
    return a;
}

void abi_SpriteSlot2Darken(void)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | SpriteSlot2Darken(cpu.db));
}

/*
 * @routine 02:E211 SpriteSlot2DarkenLong
 * WHAT IT DOES: SpriteSlot2Darken for callers in other banks.
 * HOW: JSR, RTL.
 * Entry: D = $0000. Exit: as SpriteSlot2Darken.
 */
void abi_SpriteSlot2DarkenLong(void)
{
    (cyc(46), abi_SpriteSlot2Darken());
}

/*
 * @routine 02:E1EE SpriteSlot2FadeOut
 * WHAT IT DOES: Starts record 2's fade out from full: marks it in use with
 * +$07 = $1F, then SpriteSlot2Darken. Nothing in the game calls it.
 * HOW: P and X saved and restored around the first stores; JSR to
 * SpriteSlot2Darken, RTL.
 * Entry: D = $0000. Exit: as SpriteSlot2Darken.
 */
void abi_SpriteSlot2FadeOut(void)
{
    uint8_t a;

    a = rd8(cyc_then(164, dbw(cpu.db, SLOT_MASK)));
    wr8(dbw(cpu.db, SLOT_MASK), cyc_then(40, (uint8_t)(a | 0x04)));
    cyc(106);
    wr8(0x05AC, 0x1F);
    (cyc(110), abi_SpriteSlot2Darken());
}

/*
 * @routine 02:E259 SpriteSlot5Yellow
 * WHAT IT DOES: As SpriteSlot6Yellow for record 5 ($05BD). Nothing in the
 * game calls it.
 * HOW: As SpriteSlot6Yellow.
 * Entry: D = $0000. Exit: A low byte = $1F; X, P unchanged.
 */
void abi_SpriteSlot5Yellow(void)
{
    slot_colour(cpu.db, 0x05BD, 0x20, 0x00, 0x1F, 0);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x1F);
}
