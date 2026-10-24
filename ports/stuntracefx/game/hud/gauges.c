/*
 * gauges.c: the HUD's two bar gauges per panel, each a strip of sprites
 * with a marker (SpriteBar4WithMarker, SpriteBar6WithMarker). A panel's
 * gauge values and their screen positions sit in work RAM at $0DF9-$0E15.
 */
#include "machine.h"
#include "hud_calls.h"
#include "carried.h"
#include "wram_names.h"


uint16_t SpriteBar4WithMarker(uint8_t bank, uint8_t value, uint16_t pos, uint16_t y);
uint16_t SpriteBar6WithMarker(uint8_t bank, uint8_t value, uint16_t pos, uint16_t y);

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

/* A gauge call as the code makes it: the bar's JSL (call clocks, with
   the LDY # before it), the bar, then the PLP that restores 16-bit index
   registers (72) and the return (28). X is 16-bit in these calls. */
static uint16_t bar4(uint8_t bank, Gauge g, uint16_t slot, uint32_t call)
{
    uint16_t tile;

    cyc(call);
    tile = SpriteBar4WithMarker(bank, g.value, (uint16_t)(g.y << 8 | g.x), slot);
    cyc(72);
    cyc(28);
    return tile;
}

/* The six-sprite bar's X on exit (SpriteStrip6's, from the value). */
static uint16_t bar6(uint8_t bank, Gauge g, uint16_t slot, uint32_t call, uint16_t *x)
{
    uint16_t tile;

    cyc(call);
    tile = SpriteBar6WithMarker(bank, g.value, (uint16_t)(g.y << 8 | g.x), slot);
    *x = (uint16_t)(((g.value & 0x0038) << 1) + 12);
    cyc(72);
    cyc(28);
    return tile;
}

/*
 * @routine 7E:EE46 HudGaugesPanelA
 * WHAT IT DOES: Draws panel A's gauges: $0DFA as a four-sprite bar at
 * ($0E0F, $0E11) from sprite slot $140, and $0E06 as a six-sprite bar at
 * ($0E13, $0E15) from slot $16C.
 * HOW: Each position is built in A with XBA and moved to X; the value is
 * loaded last. Work RAM through the data bank.
 * Entry: any widths. Exit: as SpriteBar6WithMarker: *a the marker's tile
 * word, *x its X, *y = $16C; the adapter sets P as at its call: M 8-bit,
 * X 16-bit, N, Z clear (from Y = $16C), C, V unchanged.
 */
void HudGaugesPanelA(uint8_t bank, uint16_t *a, uint16_t *x, uint16_t *y)
{
    uint32_t b = (uint32_t)bank << 16;
    Gauge g;

    cyc(28);
    g = gauge_of(b, W(gauge_a4_x), W(gauge_a4_y), W(gauge_a4_value));
    bar4(bank, g, 0x0140, 86);                  /* LDY #, JSL */
    g = gauge_of(b, W(gauge_a6_x), W(wram_0E15), W(gauge_a6_value));
    *a = bar6(bank, g, 0x016C, 86, x);
    *y = 0x016C;
    cyc(14);
}

/* P as a gauge call leaves it: M 8-bit, X 16-bit, N and Z from a positive
   slot. */
static uint8_t gauge_p(uint8_t p)
{
    return (uint8_t)(((p | P_M) & ~P_X) & ~(P_N | P_Z));
}

void Gauge6APlace_cr(Carried *cr);
void Gauge6BPlace_cr(Carried *cr);
void HudGaugesPanelA_cr(Carried *cr);
void HudGaugesPanelB_cr(Carried *cr);

void HudGaugesPanelA_cr(Carried *cr)
{
    uint16_t a, x, y;

    /* an interrupt inside the gauges pushes the P the bars run under */
    set_p(cr, gauge_p(cr->p));
    HudGaugesPanelA(cr->db, &a, &x, &y);
    cr->a = a;
    cr->x = x;
    cr->y = y;
    set_p(cr, gauge_p(cr->p));
}

void abi_HudGaugesPanelA(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    HudGaugesPanelA_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 7E:EE7B HudGaugesPanelB
 * WHAT IT DOES: When $0D55 or $0D62 is set, draws panel B's gauges: $0DF9
 * as a four-sprite bar at ($0E0E, $0E10) and $0E05 as a six-sprite bar at
 * ($0E12, $0E14), from sprite slots $40 and $6C when $0D62 is set, else
 * $100 and $12C.
 * HOW: As HudGaugesPanelA; $0D55 and $0D62 are first tested with long
 * loads, then $0D62 through the data bank.
 * Entry: any widths. Exit: returns 0 when neither is set (the adapter then
 * leaves A low byte = 0, M 8-bit, X 16-bit, Z set, N clear); otherwise 1
 * with *a, *x, *y as SpriteBar6WithMarker leaves them (the adapter sets P
 * as at its call: M 8-bit, X 16-bit, N, Z clear).
 */
int HudGaugesPanelB(uint8_t bank, uint16_t *a, uint16_t *x, uint16_t *y)
{
    uint32_t b = (uint32_t)bank << 16;
    uint8_t v;
    int split;
    Gauge g;

    cyc(76);
    v = rd8(W(panel_b_shown));
    if (!v)
        { cyc(48); v = rd8(W(wram_0D62)); }
    cyc(46);
    if (!v) {
        cyc(12);
        return 0;
    }
    split = rd8(b | W(wram_0D62)) != 0;
    if (split)
        cyc(50);                        /* LDY # and the BRA's other arm */
    g = gauge_of(b, W(gauge_b4_x), W(gauge_b4_y), W(gauge_b4_value));
    bar4(bank, g, split ? 0x0040 : 0x0100, split ? 86 : 108);
    cyc(40);
    split = rd8(b | W(wram_0D62)) != 0;
    if (split)
        cyc(50);
    g = gauge_of(b, W(gauge_b6_x), W(wram_0E14), W(wram_0E05));
    *y = split ? 0x006C : 0x012C;
    *a = bar6(bank, g, *y, split ? 86 : 108, x);
    cyc(14);
    return 1;
}

void HudGaugesPanelB_cr(Carried *cr)
{
    uint16_t a, x, y;
    uint8_t p0 = cr->p;

    set_p(cr, gauge_p(p0));
    if (!HudGaugesPanelB(cr->db, &a, &x, &y)) {
        set_p(cr, p0);
        cr->a &= 0xFF00;
        set_p(cr, (uint8_t)((nz8(cr->p, 0) | P_M) & ~P_X));
        return;
    }
    cr->a = a;
    cr->x = x;
    cr->y = y;
    set_p(cr, gauge_p(cr->p));
}

void abi_HudGaugesPanelB(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    HudGaugesPanelB_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* Gauge y at `at` = y, then the X-position high bits of the twelve sprites
   from 27 + x * 4 cleared in the high table ($057B + x on). Returns the
   last byte stored (A's low byte on exit). */
static uint8_t gauge6_place(uint8_t bank, uint16_t at, uint8_t y, uint8_t x)
{
    uint32_t b = (uint32_t)bank << 16;
    static const uint8_t keep[4] = {0xBF, 0xAA, 0xAA, 0xEA};
    uint8_t v = 0;
    unsigned i;

    cyc(22);                            /* SEP #$30 */
    cyc(62);
    wr8(b | at, y);
    cyc(x ? 16 : 38);                   /* LDX #imm; panel B's also BRAs to the shared tail */
    for (i = 0; i < 4; i++) {
        cyc(24);
        v = (uint8_t)(rd8(b | (uint16_t)(W(oam_buffer_high) + 6 + i + x)) & keep[i]);
        cyc(46);
        wr8(b | (uint16_t)(W(oam_buffer_high) + 6 + i + x), v);
    }
    cyc(70);
    return v;
}

/*
 * @routine 07:C53A Gauge6BPlace
 * WHAT IT DOES: Puts panel B's six-sprite gauge at Y = $5A ($0E14) and
 * clears the X-position high bits of sprites 27-38 in the high table
 * ($057B-$057E).
 * HOW: Byte ANDs through the data bank (masks $BF, $AA, $AA, $EA).
 * Entry: any widths. Exit: returns the last byte (A low byte); the adapter
 * sets X = 0; X and Y high bytes cleared (SEP #$30); P unchanged.
 */
uint8_t Gauge6BPlace(uint8_t bank)
{
    return gauge6_place(bank, W(wram_0E14), 0x5A, 0x00);
}

void Gauge6BPlace_cr(Carried *cr)
{
    uint8_t v = Gauge6BPlace(cr->db);

    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    cr->x = 0x00;
    cr->y &= 0x00FF;
}

void abi_Gauge6BPlace(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    Gauge6BPlace_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:C546 Gauge6APlace
 * WHAT IT DOES: As Gauge6BPlace for panel A: Y = $C2 ($0E15), sprites
 * 91-102 ($058B-$058E).
 * HOW: As Gauge6BPlace with X = $10. Not reached in the traces so far.
 * Entry: any widths. Exit: as Gauge6BPlace, X = $10.
 */
uint8_t Gauge6APlace(uint8_t bank)
{
    return gauge6_place(bank, W(wram_0E15), 0xC2, 0x10);
}

void Gauge6APlace_cr(Carried *cr)
{
    uint8_t v = Gauge6APlace(cr->db);

    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    cr->x = 0x10;
    cr->y &= 0x00FF;
}

void abi_Gauge6APlace(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    Gauge6APlace_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

