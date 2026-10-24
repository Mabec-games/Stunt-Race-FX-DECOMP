/*
 * digits.c: numbers drawn as sprite digits. Each digit is two 8x8 sprite
 * tiles stacked: tile 2d on top, 2d+1 below. The routines write only the
 * tile bytes of sprite entries already placed in the work-RAM sprite table.
 */
#include "machine.h"
#include "carried.h"
#include "wram_names.h"



/* The tile bytes of sprite entry `offset` (in bytes) and the one after it;
   second is the time to the second store, each copy's own (44, or 38). */
static void digit(uint32_t tile_byte, uint8_t d, uint32_t second)
{
    cyc(44);
    wr8(tile_byte, (uint8_t)(d << 1));
    cyc(second);
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
    if (rd8(W(hud_mode)) != 0x01) {
        cyc(56);
        uint16_t hold = rd16(W(hud_hold));
        if (hold == 0)
            goto draw;
        hold--;
        cyc(54);
        wr16(W(hud_hold), hold);
        end.x = hold;
        end.x_set = 1;
    } else { cyc(38); }
    cyc(24);
    end.a = rd8(W(hud_blank));
    if (end.a == 0)
        { cyc(16); return end; }
draw:
    cyc(46);
    base = rd16(W(hud_time_sprite));
    cyc(24);
    v = rd8(W(race_time_seconds));
    cyc(16);
    digit(W(oam_buffer) + 0x1A + base, v & 0x0F, 44);
    cyc(88);
    digit(W(oam_buffer) + 0x12 + base, v >> 4, 44);
    cyc(24);
    v = rd8(W(race_time_fraction));
    cyc(16);
    digit(W(oam_buffer) + 0x0A + base, v & 0x0F, 44);
    cyc(88);
    digit(W(oam_buffer) + 0x02 + base, v >> 4, 44);
    cyc(24);
    v = rd8(W(race_time_minutes));
    cyc(16);
    digit(W(oam_buffer) + 0x22 + base, v & 0x0F, 44);
    end.drew = 1;
    end.a = (uint8_t)(((v & 0x0F) << 1) + 1);
    end.x = base;
    end.x_set = 1;
    return end;
}

/* From C: returns A's low byte; *x_set when X was set, to *x. */
uint8_t HudDrawTime(uint16_t *x, int *x_set)
{
    TimeEnd end = hud_draw_time();

    cyc(70);
    *x = end.x;
    *x_set = end.x_set;
    return end.a;
}

void HudCountMove19EE_cr(Carried *cr);
void HudCountMove19EECopy_cr(Carried *cr);
void HudCountUp19EC_cr(Carried *cr);
void HudDigitSprites62_cr(Carried *cr);
void HudDrawDigits19EC_cr(Carried *cr);
void HudDrawNumber3_cr(Carried *cr);
void HudDrawTime_cr(Carried *cr);
void HudDrawValueA_cr(Carried *cr);
void HudDrawValueB_cr(Carried *cr);
void HudDrawValueC_cr(Carried *cr);
static void abi_draw_value(Carried *cr, uint16_t value, uint16_t offset);

void HudDrawTime_cr(Carried *cr)
{
    uint16_t x;
    int x_set;
    uint8_t a = HudDrawTime(&x, &x_set);

    cr->a = (uint16_t)((cr->a & 0xFF00) | a);
    if (x_set)
        cr->x = (cr->p & P_X) ? (uint8_t)x : x;
}

void abi_HudDrawTime(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    HudDrawTime_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
    uint8_t v, blank = rd8(W(hud_blank));

    *drew = 0;
    if (blank)
        return blank;
    cyc(48);
    v = rd8(W(hud_digits_19ec));
    cyc(10);
    digit(W(oam_buffer) + 0x16A, v & 0x0F, 38);
    cyc(32);
    v = rd8(W(hud_digits_19ec));
    cyc(50);
    digit(W(oam_buffer) + 0x162, v >> 4, 38);
    *drew = 1;
    return (uint8_t)(((v >> 4) << 1) + 1);
}

/* From C: returns A's low byte; *drew when it drew. */
uint8_t HudDrawDigits19EC(int *drew)
{
    uint8_t a = hud_draw_digits_19ec(drew);

    cyc(42);
    return a;
}

void HudDrawDigits19EC_cr(Carried *cr)
{
    int drew;
    uint8_t a = HudDrawDigits19EC(&drew);

    cr->a = (uint16_t)((cr->a & 0xFF00) | a);
    set_p(cr, (uint8_t)(nz8(cr->p, a) | P_M));
    if (drew)
        set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_C)));
}

void abi_HudDrawDigits19EC(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    HudDrawDigits19EC_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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

    wr8(W(digit_count), 0x00);
    while (n >= 100) {
        *p = (uint8_t)((*p & ~P_V) | (((n ^ 100) & (n ^ (uint8_t)(n - 100)) & 0x80) ? P_V : 0));
        n = (uint8_t)(n - 100);
        cyc(64);
        count = rd8(W(digit_count));
        cyc(6);
        wr8(W(digit_count), (uint8_t)(count + 1));
        cyc(22);
    }
    cyc(76);
    count = rd8(W(digit_count));
    digit(W(oam_buffer) + 2 + offset, count, 44);
    cyc(44);
    wr8(W(digit_count), 0x00);
    c = 64;                             /* the first ten; 86 for each after */
    while (n >= 10) {
        *p = (uint8_t)((*p & ~P_V) | (((n ^ 10) & (n ^ (uint8_t)(n - 10)) & 0x80) ? P_V : 0));
        n = (uint8_t)(n - 10);
        cyc(c);
        c = 86;
        count = rd8(W(digit_count));
        cyc(6);
        wr8(W(digit_count), (uint8_t)(count + 1));
    }
    cyc(c == 86 ? 98 : 76);             /* out after a ten, or with none */
    count = rd8(W(digit_count));
    digit(W(oam_buffer) + 0x0A + offset, count, 44);
    cyc(28);
    digit(W(oam_buffer) + 0x12 + offset, n, 44);
    cyc(42);
    return (uint8_t)((n << 1) + 1);
}

void HudDrawNumber3_cr(Carried *cr)
{
    uint8_t p = cr->p;
    uint8_t a = HudDrawNumber3((uint8_t)cr->a, cr->x, &p);

    cr->a = (uint16_t)((cr->a & 0xFF00) | a);
    set_p(cr, (uint8_t)(nz8(p, a) & ~P_C));
}

void abi_HudDrawNumber3(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    HudDrawNumber3_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* Draw the byte at `value` (data bank) with HudDrawNumber3 at `offset`;
   the routines below then switch to 8-bit index registers. */
static uint8_t draw_value(uint8_t bank, uint16_t value, uint16_t offset, uint8_t *p)
{
    uint8_t v;

    cyc(46);
    v = rd8((uint32_t)bank << 16 | value);
    cyc(offset == 0x0128 ? 108 : 130);  /* REP, LDX, (A and B: BRA to the tail), JSR */
    return cyc_then(64, HudDrawNumber3(v, offset, p));
}

static void abi_draw_value(Carried *cr, uint16_t value, uint16_t offset)
{
    uint8_t p = cr->p, a = draw_value(cr->db, value, offset, &p);

    cr->a = (uint16_t)((cr->a & 0xFF00) | a);
    cr->x = offset & 0x00FF;
    cr->y &= 0x00FF;
    set_p(cr, (uint8_t)((nz8(p, a) & ~P_C) | P_M | P_X));
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

void HudDrawValueA_cr(Carried *cr)
{
    abi_draw_value(cr, 0x0DDA, 0x0114);
}

void abi_HudDrawValueA(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    HudDrawValueA_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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

void HudDrawValueB_cr(Carried *cr)
{
    abi_draw_value(cr, 0x0DDA, 0x0028);
}

void abi_HudDrawValueB(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    HudDrawValueB_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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

void HudDrawValueC_cr(Carried *cr)
{
    abi_draw_value(cr, 0x0DDB, 0x0128);
}

void abi_HudDrawValueC(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    HudDrawValueC_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void SoundPort0QueuePush(uint8_t command);
void SoundPort0QueuePushW(uint8_t command, unsigned wide);

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
    wr8(b | (uint16_t)(W(obj_25) + obj), cyc_then(46, (uint8_t)(rd8(b | (uint16_t)(W(obj_25) + obj)) | 0x01)));
    cyc(60);
    wr8(b | W(hud_digits_19ec), cyc_then(54, adc_bcd8(rd8(b | W(hud_digits_19ec)), 0x01, 0, p)));
    cyc(92);
    SoundPort0QueuePush(0x3C);
    cyc(42);
}

void HudCountUp19EC_cr(Carried *cr)
{
    uint8_t p = cr->p;

    HudCountUp19EC(cr->db, cr->x, &p);
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0x3C);
    set_p(cr, (uint8_t)(nz8(p & ~P_D, cr->db) | P_M));
}

void abi_HudCountUp19EC(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    HudCountUp19EC_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
    uint8_t d = rd8(b | W(digit62_override)) ? (cyc(54), 0x01) : rd8(cyc_then(46, b | W(wram_0DF4)));

    *digit = d;
    cyc(38);
    wr8(b | W(wram_046F), (uint8_t)(d << 1));
    cyc(38);
    wr8(b | W(wram_0473), (uint8_t)((d << 1) + 1));
    return (uint8_t)((d << 1) + 1);
}

/* From C: returns A's low byte; *d the digit. */
uint8_t HudDigitSprites62(uint8_t bank, uint8_t *d)
{
    uint8_t v = hud_digit_sprites_62(bank, d);

    cyc(42);
    return v;
}

void HudDigitSprites62_cr(Carried *cr)
{
    uint8_t d, v = HudDigitSprites62(cr->db, &d);

    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    set_p(cr, (uint8_t)((nz8(cr->p, v) & ~P_C) | ((d & 0x80) ? P_C : 0)));
}

void abi_HudDigitSprites62(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    HudDigitSprites62_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

uint16_t GsuRecordRaise6A(uint8_t bank, uint16_t rec, uint8_t amount, uint8_t *p);
uint16_t GsuRecordLower82(uint8_t bank, uint16_t rec, uint8_t amount, uint8_t *p);

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
static int hc_skipped;                  /* $0E59 was set: A's low byte only */

uint16_t HudCountMove19EECopy(uint8_t bank, uint8_t *p);

/* From C: x = X (for the flags the record calls leave), *p = P in and
   out; returns A (its low byte only when hc_skipped). */
uint16_t HudCountMove19EE(uint8_t bank, uint16_t rec, uint16_t x, uint8_t *p)
{
    uint32_t b = (uint32_t)bank << 16;
    uint8_t v;

    cyc(46);
    v = rd8(b | W(wram_0E59));
    *p = nz8((uint8_t)(*p | P_M), v);
    hc_skipped = v != 0;
    if (v)
        return v;
    cyc(94);
    SoundPort0QueuePushW(0x35, (*p & P_X) ? 0 : 16);
    *p = nz8(*p, bank);
    (cyc(100), GsuRecordRaise6A(bank, rec, 0x7F, p));
    *p = (uint8_t)((nz16(*p, x) | P_M) & ~P_X);
    (cyc(100), GsuRecordLower82(bank, rec, 0x7F, p));
    *p = (uint8_t)((nz16(*p, x) | P_M) & ~P_X);
    return (cyc(72), HudCountMove19EECopy(bank, p));
}

void HudCountMove19EE_cr(Carried *cr)
{
    uint8_t p = cr->p;
    uint16_t a = HudCountMove19EE(cr->db, cr->y, cr->x, &p);

    cr->a = hc_skipped ? (uint16_t)((cr->a & 0xFF00) | (uint8_t)a) : a;
    set_p(cr, p);
}

void abi_HudCountMove19EE(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    HudCountMove19EE_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:C0AB HudCountMove19EECopy
 * WHAT IT DOES: HudCountMove19EE's last part, also entered by the count
 * gate (09:AFFC): moves the count at $19EE to $19F0 (clearing $19EE) and
 * sets $19F2 to 1.
 * HOW: 16-bit copy and STZ through the data bank, then an 8-bit store.
 * Entry: any widths. Exit: A = the old $19EE's high byte and 1; N, Z from
 * 1; M 8-bit.
 * From C: returns A; *p as P leaves.
 */
uint16_t HudCountMove19EECopy(uint8_t bank, uint8_t *p)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t n;

    cyc(46);
    n = rd16(b | W(wram_19EE));
    cyc(24);
    wr16(b | W(wram_19F0), n);
    cyc(24);
    wr16(b | W(wram_19EE), 0x0000);
    cyc(62);
    wr8(b | W(wram_19F2), 0x01);
    *p = (uint8_t)(nz8(*p, 0x01) | P_M);
    cyc(42);
    return (uint16_t)((n & 0xFF00) | 0x01);
}

void HudCountMove19EECopy_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = HudCountMove19EECopy(cr->db, &p);
    set_p(cr, p);
}

void abi_HudCountMove19EECopy(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    HudCountMove19EECopy_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

