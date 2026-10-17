/*
 * sprite_slots.c: eight 8-byte records at $0595-$05D4, after the work-RAM
 * sprite table, and the mask of the ones in use at $05D5 (bit i for the
 * record at $0595 + i * 8). $06D8 points at the first one in use.
 * Fields used here: +$00, +$04, +$05, +$06 and +$07 (bytes).
 */
#include "machine.h"

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

    wr8(dbw(bank, 0x1F2C), 0x00);
    m = rd8(dbw(bank, SLOT_MASK));
    wr8(dbw(bank, 0x1F2B), m);
    *p &= (uint8_t)~P_M;
    if (m) {
        a = SLOTS;
        for (*x = 8; *x; ) {
            uint16_t c = (*p & P_C) ? 0x8000 : 0;

            w = rd16(dbw(bank, 0x1F2B));
            *p = (uint8_t)((*p & ~P_C) | (w & 1 ? P_C : 0));
            w = (uint16_t)(c | w >> 1);
            wr16(dbw(bank, 0x1F2B), w);
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
    }
    if (!found) {
        a = 0x0000;
        *p = nz16(*p, a);
    }
    wr16(dbw(bank, 0x06D8), a);
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
    uint8_t p = (uint8_t)((cpu.p | P_M) & ~P_X);
    uint16_t x = cpu.x;

    cpu.a = SpriteSlotFirst(cpu.db, &x, &p);
    cpu.x = (cpu.p & P_X) ? (uint8_t)x : x;
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

    a = (uint8_t)(rd8(dbw(bank, 0x08FB)) + 1);
    if (!a) {
        a = (uint8_t)(rd8(dbw(bank, SLOT_MASK)) & 0xFD);
        wr8(dbw(bank, SLOT_MASK), a);
        return a;
    }
    wr8(dbw(bank, SLOT_MASK), (uint8_t)(rd8(dbw(bank, SLOT_MASK)) | 0x02));
    *x = r;
    wr8((uint16_t)(r + 4), 0x00);
    wr8((uint16_t)(r + 5), 0x02);
    wr8((uint16_t)(r + 6), 0xA7);
    wr8((uint16_t)(r + 0), (uint8_t)(rd8((uint16_t)(r + 7)) | 0xE0));
    if (!rd8(dbw(bank, 0x08FB))) {
        a = rd8(0x7EFF65);
        if (a) {
            a = rd8((uint16_t)(r + 7));
            if (!a)
                goto count;
            a = (uint8_t)(a - 2);
            wr8((uint16_t)(r + 7), a);
            return a;
        }
        wr8(0x7EFF65, (uint8_t)(a + 1));
    }
    wr8((uint16_t)(r + 4), 0x00);
    wr8((uint16_t)(r + 6), 0xE7);
    a = 0xFF;
    wr8((uint16_t)(r + 0), a);
count:
    wr8(dbw(bank, 0x08FB), (uint8_t)(rd8(dbw(bank, 0x08FB)) - 1));
    return a;
}

void abi_SpriteSlot1Countdown(void)
{
    uint16_t x = cpu.x;
    uint8_t a = SpriteSlot1Countdown(cpu.db, &x);

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | a);
    cpu.x = (cpu.p & P_X) ? (uint8_t)x : x;
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

    a = rd8(dbw(bank, SLOT_MASK));
    wr8(dbw(bank, SLOT_MASK), (uint8_t)(a | 0x10));
    wr8((uint16_t)(r + 4), 0x00);
    wr8((uint16_t)(r + 5), 0x02);
    wr8((uint16_t)(r + 6), 0xE6);
    a = rd8((uint16_t)(r + 7));
    wr8((uint16_t)(r + 0), (uint8_t)(a | 0xE0));
    a = rd8((uint16_t)(r + 7));
    if (a != 0x0F) {
        a = (uint8_t)(a + 1);
        wr8((uint16_t)(r + 7), a);
    }
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

    a = rd8(dbw(bank, SLOT_MASK));
    wr8(dbw(bank, SLOT_MASK), (uint8_t)(a | 0x10));
    wr8((uint16_t)(r + 4), 0x00);
    wr8((uint16_t)(r + 5), 0x02);
    wr8((uint16_t)(r + 6), 0xE6);
    a = rd8((uint16_t)(r + 7));
    wr8((uint16_t)(r + 0), (uint8_t)(a | 0xE0));
    a = rd8((uint16_t)(r + 7));
    if (a) {
        a = (uint8_t)(a - 1);
        wr8((uint16_t)(r + 7), a);
        return a;
    }
    a = rd8(dbw(bank, SLOT_MASK));
    a = (uint8_t)(a & 0xEF);
    wr8(dbw(bank, SLOT_MASK), a);
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

    a = rd8(dbw(bank, SLOT_MASK));
    wr8(dbw(bank, SLOT_MASK), (uint8_t)(a | 0x10));
    wr8((uint16_t)(r + 4), 0x00);
    wr8((uint16_t)(r + 5), 0x02);
    wr8((uint16_t)(r + 6), 0xE6);
    wr8((uint16_t)(r + 0), 0xFF);
    wr8((uint16_t)(r + 7), 0x1F);
    return 0x1F;
}

void abi_SpriteSlot4Full(void)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | SpriteSlot4Full(cpu.db));
}
