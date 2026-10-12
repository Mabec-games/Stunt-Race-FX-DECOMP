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
    wr8(tile_byte, (uint8_t)(d << 1));
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

    if (rd8(0x000D2B) != 0x01) {
        uint16_t hold = rd16(0x000E5D);
        if (hold == 0)
            goto draw;
        hold--;
        wr16(0x000E5D, hold);
        end.x = hold;
        end.x_set = 1;
    }
    end.a = rd8(0x000D4E);
    if (end.a == 0)
        return end;
draw:
    base = rd16(HUD_TIME_SPRITE);
    v = rd8(RACE_TIME_SECONDS);
    digit(OAM_BUFFER + 0x1A + base, v & 0x0F);
    digit(OAM_BUFFER + 0x12 + base, v >> 4);
    v = rd8(RACE_TIME_FRACTION);
    digit(OAM_BUFFER + 0x0A + base, v & 0x0F);
    digit(OAM_BUFFER + 0x02 + base, v >> 4);
    v = rd8(RACE_TIME_MINUTES);
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
    uint8_t v, blank = rd8(0x000D4E);

    *drew = 0;
    if (blank)
        return blank;
    v = rd8(0x0019EC);
    digit(OAM_BUFFER + 0x16A, v & 0x0F);
    v = rd8(0x0019EC);
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
    uint8_t count = 0;

    while (n >= 100) {
        *p = (uint8_t)((*p & ~P_V) | (((n ^ 100) & (n ^ (uint8_t)(n - 100)) & 0x80) ? P_V : 0));
        n = (uint8_t)(n - 100);
        count++;
    }
    digit(OAM_BUFFER + 2 + offset, count);
    count = 0;
    while (n >= 10) {
        *p = (uint8_t)((*p & ~P_V) | (((n ^ 10) & (n ^ (uint8_t)(n - 10)) & 0x80) ? P_V : 0));
        n = (uint8_t)(n - 10);
        count++;
    }
    digit(OAM_BUFFER + 0x0A + offset, count);
    digit(OAM_BUFFER + 0x12 + offset, n);
    wr8(0x0000F1, count);
    return (uint8_t)((n << 1) + 1);
}

void abi_HudDrawNumber3(void)
{
    uint8_t p = cpu.p;
    uint8_t a = HudDrawNumber3((uint8_t)cpu.a, cpu.x, &p);

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | a);
    cpu.p = (uint8_t)(nz8(p, a) & ~P_C);
}
