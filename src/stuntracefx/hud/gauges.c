/*
 * gauges.c: the HUD's two bar gauges per panel, each a strip of sprites
 * with a marker (SpriteBar4WithMarker, SpriteBar6WithMarker). A panel's
 * gauge values and their screen positions sit in work RAM at $0DF9-$0E15.
 */
#include "machine.h"
#include "cpu_ops.h"

uint16_t SpriteBar4WithMarker(uint8_t bank, uint8_t value, uint16_t pos, uint16_t y);
uint16_t SpriteBar6WithMarker(uint8_t bank, uint8_t value, uint16_t pos, uint16_t y);
void abi_SpriteBar4WithMarker(void);
void abi_SpriteBar6WithMarker(void);

/* One gauge as the code loads it: the Y byte, the X byte, then the value
   (the Y byte stays in A's high byte through XBA). */
typedef struct {
    uint8_t y, x, value;
} Gauge;

static Gauge gauge_of(uint32_t b, uint16_t lo_at, uint16_t hi_at, uint16_t value_at)
{
    Gauge g;

    cyc(40);
    g.y = rd8(b | hi_at);
    cyc(44);
    g.x = rd8(b | lo_at);
    cyc(82);
    g.value = rd8(b | value_at);
    return g;
}

/* For the abi_ wrappers: the registers as the code sets them for a gauge
   call (M 8-bit, X 16-bit, A high byte = the Y byte, N, Z from the slot),
   then the gauge's own abi_. */
static void abi_gauge(void (*gauge)(void), uint16_t lo_at, uint16_t hi_at, uint16_t value_at, uint16_t slot,
                      uint32_t call)
{
    Gauge g = gauge_of((uint32_t)cpu.db << 16, lo_at, hi_at, value_at);

    cyc(call);                          /* LDY # (24), any BRA (22), JSL (62) */
    cpu.x = (uint16_t)(g.y << 8 | g.x);
    cpu.a = (uint16_t)(g.y << 8 | g.value);
    cpu.y = slot;
    cpu.p = nz16((uint8_t)((cpu.p | P_M) & ~P_X), slot);
    gauge();
    cyc(28);
}

/*
 * @routine 7E:EE46 HudGaugesPanelA
 * WHAT IT DOES: Draws panel A's gauges: $0DFA as a four-sprite bar at
 * ($0E0F, $0E11) from sprite slot $140, and $0E06 as a six-sprite bar at
 * ($0E13, $0E15) from slot $16C.
 * HOW: Each position is built in A with XBA and moved to X; the value is
 * loaded last. Work RAM through the data bank.
 * Entry: any widths. Exit: as SpriteBar6WithMarker; P as at its call: M
 * 8-bit, X 16-bit, N, Z clear (from Y = $16C), C, V unchanged.
 */
void HudGaugesPanelA(uint8_t bank)
{
    uint32_t b = (uint32_t)bank << 16;
    Gauge g;

    g = gauge_of(b, 0x0E0F, 0x0E11, 0x0DFA);
    cyc(86);                            /* LDY #, JSL */
    SpriteBar4WithMarker(bank, g.value, (uint16_t)(g.y << 8 | g.x), 0x0140);
    g = gauge_of(b, 0x0E13, 0x0E15, 0x0E06);
    cyc(86);
    SpriteBar6WithMarker(bank, g.value, (uint16_t)(g.y << 8 | g.x), 0x016C);
}

void abi_HudGaugesPanelA(void)
{
    cyc(28);
    abi_gauge(abi_SpriteBar4WithMarker, 0x0E0F, 0x0E11, 0x0DFA, 0x0140, 86);
    abi_gauge(abi_SpriteBar6WithMarker, 0x0E13, 0x0E15, 0x0E06, 0x016C, 86);
    cyc(14);
}

/*
 * @routine 7E:EE7B HudGaugesPanelB
 * WHAT IT DOES: When $0D55 or $0D62 is set, draws panel B's gauges: $0DF9
 * as a four-sprite bar at ($0E0E, $0E10) and $0E05 as a six-sprite bar at
 * ($0E12, $0E14), from sprite slots $40 and $6C when $0D62 is set, else
 * $100 and $12C.
 * HOW: As HudGaugesPanelA; $0D55 and $0D62 are first tested with long
 * loads, then $0D62 through the data bank.
 * Entry: any widths. Exit: when neither is set, A low byte = 0, M 8-bit,
 * X 16-bit, Z set, N clear; otherwise as SpriteBar6WithMarker, P as at its
 * call (M 8-bit, X 16-bit, N, Z from the slot).
 */
void HudGaugesPanelB(uint8_t bank)
{
    uint32_t b = (uint32_t)bank << 16;
    int split;
    Gauge g;

    if (!rd8(0x000D55) && !rd8(0x000D62))
        return;
    split = rd8(b | 0x0D62) != 0;
    g = gauge_of(b, 0x0E0E, 0x0E10, 0x0DF9);
    cyc(split ? 86 : 108);              /* LDY #, the BRA when not split, JSL */
    SpriteBar4WithMarker(bank, g.value, (uint16_t)(g.y << 8 | g.x), split ? 0x0040 : 0x0100);
    split = rd8(b | 0x0D62) != 0;
    g = gauge_of(b, 0x0E12, 0x0E14, 0x0E05);
    cyc(split ? 86 : 108);
    SpriteBar6WithMarker(bank, g.value, (uint16_t)(g.y << 8 | g.x), split ? 0x006C : 0x012C);
}

void abi_HudGaugesPanelB(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    cyc(76);
    uint8_t v = rd8(0x000D55);
    int split;

    if (!v)
        { cyc(48); v = rd8(0x000D62); }
    cyc(46);
    if (!v) {
        cyc(12);
        cpu.a &= 0xFF00;
        cpu.p = (uint8_t)((nz8(cpu.p, 0) | P_M) & ~P_X);
        return;
    }
    split = rd8(b | 0x0D62) != 0;
    abi_gauge(abi_SpriteBar4WithMarker, 0x0E0E, 0x0E10, 0x0DF9, split ? (cyc(50), 0x0040) : 0x0100, split ? 86 : 108);
    cyc(40);
    split = rd8(b | 0x0D62) != 0;
    abi_gauge(abi_SpriteBar6WithMarker, 0x0E12, 0x0E14, 0x0E05, split ? (cyc(50), 0x006C) : 0x012C, split ? 86 : 108);
    cyc(14);
}

/* Gauge y at `at` = y, then the X-position high bits of the twelve sprites
   from 27 + x * 4 cleared in the high table ($057B + x on). */
static void gauge6_place(uint16_t at, uint8_t y, uint8_t x)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    static const uint8_t keep[4] = {0xBF, 0xAA, 0xAA, 0xEA};
    uint8_t p0 = cpu.p, v = 0;
    unsigned i;

    op_sep(P_M | P_X);
    cyc(62);
    wr8(b | at, y);
    cyc(38);
    for (i = 0; i < 4; i++) {
        cyc(24);
        v = (uint8_t)(rd8(b | (uint16_t)(0x057B + i + x)) & keep[i]);
        cyc(46);
        wr8(b | (uint16_t)(0x057B + i + x), v);
    }
    cyc(70);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.x = x;
    cpu.p = p0;
}

/*
 * @routine 07:C53A Gauge6BPlace
 * WHAT IT DOES: Puts panel B's six-sprite gauge at Y = $5A ($0E14) and
 * clears the X-position high bits of sprites 27-38 in the high table
 * ($057B-$057E).
 * HOW: Byte ANDs through the data bank (masks $BF, $AA, $AA, $EA).
 * Entry: any widths. Exit: A low byte = the last byte; X = 0; X and Y high
 * bytes cleared (SEP #$30); P unchanged.
 */
void abi_Gauge6BPlace(void)
{
    gauge6_place(0x0E14, 0x5A, 0x00);
}

/*
 * @routine 07:C546 Gauge6APlace
 * WHAT IT DOES: As Gauge6BPlace for panel A: Y = $C2 ($0E15), sprites
 * 91-102 ($058B-$058E).
 * HOW: As Gauge6BPlace with X = $10. Not reached in the traces so far.
 * Entry: any widths. Exit: as Gauge6BPlace, X = $10.
 */
void abi_Gauge6APlace(void)
{
    gauge6_place(0x0E15, 0xC2, 0x10);
}
