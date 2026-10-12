/*
 * gauges.c: the HUD's two bar gauges per panel, each a strip of sprites
 * with a marker (SpriteBar4WithMarker, SpriteBar6WithMarker). A panel's
 * gauge values and their screen positions sit in work RAM at $0DF9-$0E15.
 */
#include "machine.h"

uint16_t SpriteBar4WithMarker(uint8_t bank, uint8_t value, uint16_t pos, uint16_t y);
uint16_t SpriteBar6WithMarker(uint8_t bank, uint8_t value, uint16_t pos, uint16_t y);
void abi_SpriteBar4WithMarker(void);
void abi_SpriteBar6WithMarker(void);

/* A position word from two bytes: X at lo_at, Y at hi_at. */
static uint16_t pos_of(uint32_t b, uint16_t lo_at, uint16_t hi_at)
{
    return (uint16_t)(rd8(b | hi_at) << 8 | rd8(b | lo_at));
}

/* For the abi_ wrappers: the registers as the code sets them for a gauge
   call (M 8-bit, X 16-bit, A high byte = the Y byte, N, Z from the slot),
   then the gauge's own abi_. */
static void abi_gauge(void (*gauge)(void), uint16_t lo_at, uint16_t hi_at, uint16_t value_at, uint16_t slot)
{
    uint32_t b = (uint32_t)cpu.db << 16;

    cpu.x = pos_of(b, lo_at, hi_at);
    cpu.a = (uint16_t)(rd8(b | hi_at) << 8 | rd8(b | value_at));
    cpu.y = slot;
    cpu.p = nz16((uint8_t)((cpu.p | P_M) & ~P_X), slot);
    gauge();
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

    SpriteBar4WithMarker(bank, rd8(b | 0x0DFA), pos_of(b, 0x0E0F, 0x0E11), 0x0140);
    SpriteBar6WithMarker(bank, rd8(b | 0x0E06), pos_of(b, 0x0E13, 0x0E15), 0x016C);
}

void abi_HudGaugesPanelA(void)
{
    abi_gauge(abi_SpriteBar4WithMarker, 0x0E0F, 0x0E11, 0x0DFA, 0x0140);
    abi_gauge(abi_SpriteBar6WithMarker, 0x0E13, 0x0E15, 0x0E06, 0x016C);
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

    if (!rd8(0x000D55) && !rd8(0x000D62))
        return;
    split = rd8(b | 0x0D62) != 0;
    SpriteBar4WithMarker(bank, rd8(b | 0x0DF9), pos_of(b, 0x0E0E, 0x0E10), split ? 0x0040 : 0x0100);
    SpriteBar6WithMarker(bank, rd8(b | 0x0E05), pos_of(b, 0x0E12, 0x0E14), split ? 0x006C : 0x012C);
}

void abi_HudGaugesPanelB(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint8_t v = rd8(0x000D55);
    int split;

    if (!v)
        v = rd8(0x000D62);
    if (!v) {
        cpu.a &= 0xFF00;
        cpu.p = (uint8_t)((nz8(cpu.p, 0) | P_M) & ~P_X);
        return;
    }
    split = rd8(b | 0x0D62) != 0;
    abi_gauge(abi_SpriteBar4WithMarker, 0x0E0E, 0x0E10, 0x0DF9, split ? 0x0040 : 0x0100);
    split = rd8(b | 0x0D62) != 0;
    abi_gauge(abi_SpriteBar6WithMarker, 0x0E12, 0x0E14, 0x0E05, split ? 0x006C : 0x012C);
}
