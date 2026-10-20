/*
 * digits.c: numbers drawn as sprite digits. Each digit is two 8x8 sprite
 * tiles stacked: tile 2d on top, 2d+1 below. The routines write only the
 * tile bytes of sprite entries already placed in the work-RAM sprite table.
 */
#include "machine.h"
#include "wram.h"

#define RACE_TIME_FRACTION 0x000E63u  /* BCD */
#define RACE_TIME_SECONDS 0x000E64u   /* BCD, 00-59 */
#define RACE_TIME_MINUTES 0x000E66u
#define HUD_TIME_SPRITE 0x000E60u     /* table offset of the time's first sprite */

/* The tile bytes of sprite entry `offset` (in bytes) and the one after it. */
static void digit(uint32_t tile_byte, uint8_t d)
{
    cyc(44);
    wr8(tile_byte, (uint8_t)(d << 1));
    cyc(44);
    wr8(tile_byte + 4, (uint8_t)((d << 1) + 1));
}

/*
 * @routine 7E:B00C HudDrawTime
 * WHAT IT DOES: Draws the race time with sprite digits, unless the HUD is
 * blanked; also counts down the HUD's hold timer.
 * HOW: Unless $0D2B is 1, the 16-bit hold timer at $0E5D counts down by one
 * while it is not zero, and the time is drawn whenever it is zero; otherwise
 * the time is drawn only when $0D4E is set. The five digits (two of the
 * fraction, two of the seconds, the minutes) go to the five sprite pairs
 * from the table offset at $0E60.
 * Entry: X 8-bit. Exit: P unchanged; A low byte = the last tile written, or
 * $0D4E when nothing was drawn; X = the sprite offset after drawing, or the
 * new hold timer (low byte with 8-bit index registers).
 */
typedef struct {
    int drew;
    uint8_t a;
    uint16_t x;
    int x_set;
} TimeEnd;

static TimeEnd hud_draw_time(void)
{
    TimeEnd end = {0, 0, 0, 0};
    uint16_t base;
    uint8_t v;

    cyc(136);
    if (rd8(0x000D2B) != 0x01) {
        cyc(56);
        uint16_t hold = rd16(0x000E5D);
        if (hold == 0)
            goto draw;
        hold--;
        cyc(54);
        wr16(0x000E5D, hold);
        end.x = hold;
        end.x_set = 1;
    } else { cyc(38); }
    cyc(24);
    end.a = rd8(0x000D4E);
    if (end.a == 0)
        { cyc(16); return end; }
draw:
    cyc(46);
    base = rd16(HUD_TIME_SPRITE);
    cyc(24);
    v = rd8(RACE_TIME_SECONDS);
    cyc(16);
    digit(OAM_BUFFER + 0x1A + base, v & 0x0F);
    cyc(88);
    digit(OAM_BUFFER + 0x12 + base, v >> 4);
    cyc(24);
    v = rd8(RACE_TIME_FRACTION);
    cyc(16);
    digit(OAM_BUFFER + 0x0A + base, v & 0x0F);
    cyc(88);
    digit(OAM_BUFFER + 0x02 + base, v >> 4);
    cyc(24);
    v = rd8(RACE_TIME_MINUTES);
    cyc(16);
    digit(OAM_BUFFER + 0x22 + base, v & 0x0F);
    end.drew = 1;
    end.a = (uint8_t)(((v & 0x0F) << 1) + 1);
    end.x = base;
    end.x_set = 1;
    return end;
}

void HudDrawTime(void)
{
    hud_draw_time();
}

void abi_HudDrawTime(void)
{
    TimeEnd end = hud_draw_time();

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | end.a);
    if (end.x_set)
        cpu.x = (cpu.p & P_X) ? (uint8_t)end.x : end.x;
    cyc(70);
}

/*
 * @routine 7E:E7D6 HudDrawDigits19EC
 * WHAT IT DOES: Draws the two-digit BCD value at $19EC with sprite digits
 * (sprite pairs at table offsets $160 and $168), unless the HUD is blanked.
 * HOW: Does nothing when $0D4E is set; else ones digit, then tens digit.
 * Entry: X 8-bit. Exit: A low byte = the last tile written, or $0D4E; C clear
 * after drawing; N, Z from A; M 8-bit.
 */
static uint8_t hud_draw_digits_19ec(int *drew)
{
    cyc(46);
    uint8_t v, blank = rd8(0x000D4E);

    *drew = 0;
    if (blank)
        return blank;
    cyc(48);
    v = rd8(0x0019EC);
    cyc(10);
    digit(OAM_BUFFER + 0x16A, v & 0x0F);
    cyc(32);
    v = rd8(0x0019EC);
    cyc(50);
    digit(OAM_BUFFER + 0x162, v >> 4);
    *drew = 1;
    return (uint8_t)(((v >> 4) << 1) + 1);
}

void HudDrawDigits19EC(void)
{
    int drew;

    hud_draw_digits_19ec(&drew);
}

void abi_HudDrawDigits19EC(void)
{
    int drew;
    uint8_t a = hud_draw_digits_19ec(&drew);

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | a);
    cpu.p = (uint8_t)(nz8(cpu.p, a) | P_M);
    if (drew)
        cpu.p &= (uint8_t)~P_C;
    cyc(42);
}

/*
 * @routine 7E:E95E HudDrawNumber3
 * WHAT IT DOES: Draws an 8-bit number as three sprite digits (hundreds, tens,
 * ones) into three sprite pairs starting at table offset X.
 * HOW: Counts hundreds by subtracting 100, then tens by subtracting 10, in
 * direct page $F1; each digit's tiles go to its pair.
 * Entry: M 8-bit, X 16-bit, D = $0000; A = number, X = first sprite offset.
 * Exit: A low byte = the ones digit's lower tile; $F1 = the tens; C clear; V
 * from the last subtraction; N, Z from A.
 */
uint8_t HudDrawNumber3(uint8_t n, uint16_t offset, uint8_t *p)
{
    uint8_t count;
    uint32_t c;

    wr8(0x0000F1, 0x00);
    while (n >= 100) {
        *p = (uint8_t)((*p & ~P_V) | (((n ^ 100) & (n ^ (uint8_t)(n - 100)) & 0x80) ? P_V : 0));
        n = (uint8_t)(n - 100);
        cyc(64);
        count = rd8(0x0000F1);
        cyc(6);
        wr8(0x0000F1, (uint8_t)(count + 1));
        cyc(22);
    }
    cyc(76);
    count = rd8(0x0000F1);
    digit(OAM_BUFFER + 2 + offset, count);
    cyc(44);
    wr8(0x0000F1, 0x00);
    c = 64;                             /* the first ten; 86 for each after */
    while (n >= 10) {
        *p = (uint8_t)((*p & ~P_V) | (((n ^ 10) & (n ^ (uint8_t)(n - 10)) & 0x80) ? P_V : 0));
        n = (uint8_t)(n - 10);
        cyc(c);
        c = 86;
        count = rd8(0x0000F1);
        cyc(6);
        wr8(0x0000F1, (uint8_t)(count + 1));
    }
    cyc(c == 86 ? 98 : 76);             /* out after a ten, or with none */
    count = rd8(0x0000F1);
    digit(OAM_BUFFER + 0x0A + offset, count);
    cyc(28);
    digit(OAM_BUFFER + 0x12 + offset, n);
    cyc(42);
    return (uint8_t)((n << 1) + 1);
}

void abi_HudDrawNumber3(void)
{
    uint8_t p = cpu.p;
    uint8_t a = HudDrawNumber3((uint8_t)cpu.a, cpu.x, &p);

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | a);
    cpu.p = (uint8_t)(nz8(p, a) & ~P_C);
}

/* Draw the byte at `value` (data bank) with HudDrawNumber3 at `offset`;
   the routines below then switch to 8-bit index registers. */
static uint8_t draw_value(uint8_t bank, uint16_t value, uint16_t offset, uint8_t *p)
{
    uint8_t v;

    cyc(46);
    v = rd8((uint32_t)bank << 16 | value);
    cyc(130);
    return cyc_then(64, HudDrawNumber3(v, offset, p));
}

static void abi_draw_value(uint16_t value, uint16_t offset)
{
    uint8_t p = cpu.p, a = draw_value(cpu.db, value, offset, &p);

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | a);
    cpu.x = offset & 0x00FF;
    cpu.y &= 0x00FF;
    cpu.p = (uint8_t)((nz8(p, a) & ~P_C) | P_M | P_X);
}

/*
 * @routine 7E:E7AE HudDrawValueA
 * WHAT IT DOES: Draws the number at $0DDA as three digits at sprite offset
 * $114.
 * HOW: HudDrawNumber3, through the shared tail at $7E:E7D0.
 * Entry: any widths, D = $0000. Exit: as HudDrawNumber3; X = $14, Y high
 * byte clear; M 8-bit, X 8-bit.
 */
uint8_t HudDrawValueA(uint8_t bank, uint8_t *p)
{
    return draw_value(bank, 0x0DDA, 0x0114, p);
}

void abi_HudDrawValueA(void)
{
    abi_draw_value(0x0DDA, 0x0114);
}

/*
 * @routine 7E:E7BA HudDrawValueB
 * WHAT IT DOES: Draws the number at $0DDA as three digits at sprite offset
 * $28.
 * HOW: As HudDrawValueA.
 * Entry: as HudDrawValueA. Exit: as HudDrawValueA, X = $28.
 */
uint8_t HudDrawValueB(uint8_t bank, uint8_t *p)
{
    return draw_value(bank, 0x0DDA, 0x0028, p);
}

void abi_HudDrawValueB(void)
{
    abi_draw_value(0x0DDA, 0x0028);
}

/*
 * @routine 7E:E7C6 HudDrawValueC
 * WHAT IT DOES: Draws the number at $0DDB as three digits at sprite offset
 * $128.
 * HOW: As HudDrawValueA; its last three instructions are the shared tail.
 * Entry: as HudDrawValueA. Exit: as HudDrawValueA, X = $28.
 */
uint8_t HudDrawValueC(uint8_t bank, uint8_t *p)
{
    return draw_value(bank, 0x0DDB, 0x0128, p);
}

void abi_HudDrawValueC(void)
{
    abi_draw_value(0x0DDB, 0x0128);
}

void SoundPort0QueuePush(uint8_t command);

/* An 8-bit ADC in decimal mode, as the 65816 does it (carry in `c`). */
static uint8_t adc_bcd8(uint8_t a, uint8_t m, int c, uint8_t *p)
{
    unsigned r = (a & 0x0F) + (m & 0x0F) + (unsigned)c;

    if (r > 0x09) r += 0x06;
    c = r > 0x0F;
    r = (a & 0xF0) + (m & 0xF0) + ((unsigned)c << 4) + (r & 0x0F);
    *p = (uint8_t)((*p & ~(P_V | P_C)) | ((~(a ^ m) & (a ^ r) & 0x80) ? P_V : 0));
    if (r > 0x9F) r += 0x60;
    if (r > 0xFF) *p |= P_C;
    *p = nz8(*p, (uint8_t)r);
    return (uint8_t)r;
}

/*
 * @routine 09:C072 HudCountUp19EC
 * WHAT IT DOES: Marks object X (bit 0 of +$25), adds one to the BCD count at
 * $19EC and queues sound $3C for the current player.
 * HOW: An 8-bit decimal-mode add; SoundPort0QueuePush.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: A low byte = $3C; D clear;
 * C, V from the decimal add; N, Z from the data bank.
 */
void HudCountUp19EC(uint8_t bank, uint16_t obj, uint8_t *p)
{
    uint32_t b = (uint32_t)bank << 16;

    cyc(30);
    wr8(b | (uint16_t)(0x2025 + obj), cyc_then(46, (uint8_t)(rd8(b | (uint16_t)(0x2025 + obj)) | 0x01)));
    cyc(60);
    wr8(b | 0x19EC, cyc_then(54, adc_bcd8(rd8(b | 0x19EC), 0x01, 0, p)));
    cyc(92);
    SoundPort0QueuePush(0x3C);
    cyc(42);
}

void abi_HudCountUp19EC(void)
{
    uint8_t p = cpu.p;

    HudCountUp19EC(cpu.db, cpu.x, &p);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x3C);
    cpu.p = (uint8_t)(nz8(p & ~P_D, cpu.db) | P_M);
}

/*
 * @routine 7E:E7FC HudDigitSprites62
 * WHAT IT DOES: Shows the digit in $0DF4 on sprites 62 and 63 (tile 2d on
 * top, 2d + 1 below), or the digit 1 when $0D4C is set (no script has
 * reached that case yet).
 * HOW: Byte stores to the two sprites' tile bytes ($046F, $0473) through
 * the data bank.
 * Entry: M 8-bit. Exit: A low byte = 2d + 1; N, Z from it; C = bit 7 of d.
 */
static uint8_t hud_digit_sprites_62(uint8_t bank, uint8_t *digit)
{
    uint32_t b = (uint32_t)bank << 16;
    cyc(24);
    uint8_t d = rd8(b | 0x0D4C) ? (cyc(54), 0x01) : rd8(cyc_then(46, b | 0x0DF4));

    *digit = d;
    cyc(38);
    wr8(b | 0x046F, (uint8_t)(d << 1));
    cyc(38);
    wr8(b | 0x0473, (uint8_t)((d << 1) + 1));
    return (uint8_t)((d << 1) + 1);
}

uint8_t HudDigitSprites62(uint8_t bank)
{
    uint8_t d;

    return hud_digit_sprites_62(bank, &d);
}

void abi_HudDigitSprites62(void)
{
    uint8_t d, v = hud_digit_sprites_62(cpu.db, &d);

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = (uint8_t)((nz8(cpu.p, v) & ~P_C) | ((d & 0x80) ? P_C : 0));
    cyc(42);
}

uint16_t GsuRecordRaise6A(uint8_t bank, uint16_t rec, uint8_t amount, uint8_t *p);
uint16_t GsuRecordLower82(uint8_t bank, uint16_t rec, uint8_t amount, uint8_t *p);
void abi_SoundPort0QueuePush(void);
void abi_GsuRecordRaise6A(void);
void abi_GsuRecordLower82(void);

/*
 * @routine 09:C08E HudCountMove19EE
 * WHAT IT DOES: Unless $0E59 is set: queues sound $35 for the current
 * player, raises +$6A of Super FX record Y and lowers its +$82 by $7F's
 * worth (GsuRecordRaise6A, GsuRecordLower82), moves the count at $19EE to
 * $19F0 (clearing $19EE) and sets $19F2 to 1.
 * HOW: 16-bit copy and clear through the data bank; the rest 8-bit.
 * Entry: X 16-bit; Y = record. Exit: skipped: A low byte = $0E59; N, Z
 * from it. Otherwise: A = the old $19EE's high byte and 1; N, Z from 1;
 * M 8-bit; X, Y unchanged.
 */
void HudCountMove19EE(uint8_t bank, uint16_t rec)
{
    uint32_t b = (uint32_t)bank << 16;
    uint8_t p = 0;
    uint16_t n;

    if (rd8(b | 0x0E59))
        return;
    SoundPort0QueuePush(0x35);
    GsuRecordRaise6A(bank, rec, 0x7F, &p);
    GsuRecordLower82(bank, rec, 0x7F, &p);
    n = rd16(b | 0x19EE);
    wr16(b | 0x19F0, n);
    wr16(b | 0x19EE, 0x0000);
    wr8(b | 0x19F2, 0x01);
}

void abi_HudCountMove19EECopy(void);

void abi_HudCountMove19EE(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    cyc(46);
    uint8_t v = rd8(b | 0x0E59);

    cpu.p |= P_M;
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, v);
    if (v)
        return; else { cyc(94); }
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x35);
    abi_SoundPort0QueuePush();
    cpu.p |= P_M;
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x7F);
    (cyc(100), abi_GsuRecordRaise6A());
    cpu.p |= P_M;
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x7F);
    (cyc(172), abi_GsuRecordLower82());
    (cyc(72), abi_HudCountMove19EECopy());
}

/*
 * @routine 09:C0AB HudCountMove19EECopy
 * WHAT IT DOES: HudCountMove19EE's last part, also entered by the count
 * gate (09:AFFC): moves the count at $19EE to $19F0 (clearing $19EE) and
 * sets $19F2 to 1.
 * HOW: 16-bit copy and STZ through the data bank, then an 8-bit store.
 * Entry: any widths. Exit: A = the old $19EE's high byte and 1; N, Z from
 * 1; M 8-bit.
 */
void abi_HudCountMove19EECopy(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint16_t n;

    cyc(46);
    n = rd16(b | 0x19EE);
    cyc(24);
    wr16(b | 0x19F0, n);
    cyc(24);
    wr16(b | 0x19EE, 0x0000);
    cyc(62);
    wr8(b | 0x19F2, 0x01);
    cpu.a = (uint16_t)((n & 0xFF00) | 0x01);
    cpu.p = (uint8_t)(nz8(cpu.p, 0x01) | P_M);
    cyc(42);
}
