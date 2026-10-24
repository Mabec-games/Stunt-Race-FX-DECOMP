/*
 * gauge.c: the speed gauge. In the menus a value at $117E rises while
 * button B is held and falls back when it is released, and slides toward a
 * target; in a race it comes from the vehicle's speed. The result is kept
 * in direct page $02 and copied to $0E49.
 */
#include "machine.h"
#include "cpu_ops.h"
#include "menu_calls.h"
#include "carried.h"
#include "wram_names.h"


/*
 * @routine 7F:11AC MenuGaugeFromButtonB
 * WHAT IT DOES: Raises the menu gauge by $28 (up to $FF) while joypad 1's B
 * button is held, otherwise lowers it by $0C (down to 0), and returns it in
 * direct page $02.
 * HOW: Tests bit 15 of joypad 1's held buttons.
 * Entry: any widths, D = $0000. Exit: A = the gauge; P unchanged.
 */
uint16_t MenuGaugeFromButtonB(void)
{
    uint16_t g;

    cyc(76);
    if (rd16(W(joy1_held)) & 0x8000) {
        cyc(78);
        g = (uint16_t)(rd16(W(menu_gauge)) + 0x0028);
        if (g >= 0x00FF)
            { cyc(18); g = 0x00FF; }
        cyc(116);
        wr16(W(menu_gauge), g);
    } else {
        cyc(72);
        g = rd16(W(menu_gauge));
        if (g != 0) {
            g = (uint16_t)(g - 0x000C);
            if (g & 0x8000)
                { cyc(18); g = 0; }
            cyc(108);
            wr16(W(menu_gauge), g);
        }
        cyc(22);
    }
    cyc(32);
    g = rd16(W(menu_gauge));
    cyc(16);
    wr16(W(dp_02), g);
    cyc(22);
    return g;
}

/* MenuGaugeFromButtonB on carried registers. */
void MenuGaugeFromButtonB_cr(Carried *cr)
{
    cr->a = MenuGaugeFromButtonB();
}

void abi_MenuGaugeFromButtonB(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuGaugeFromButtonB_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 7F:1177 MenuGaugeSlideToTarget
 * WHAT IT DOES: Steps the menu gauge toward the target in direct page $02:
 * down by 8 when above it, up by 4 when below. If the stepped gauge is at or
 * above the target, $02 takes it; if it is below (or was equal to start
 * with), the gauge becomes $FF00 (idle).
 * HOW: Compares with the target, steps, and tests the sign of gauge minus
 * target.
 * Entry: any widths, D = $0000. Exit: A = $FF00 or the new value; P
 * unchanged.
 */
uint16_t MenuGaugeSlideToTarget(void)
{
    cyc(68);
    uint16_t g = rd16(W(menu_gauge)), target = rd16(cyc_then(16, W(dp_02)));

    if (g != target) {
        int down = g >= target;

        if (down)
            g = (uint16_t)(g - 0x0008);
        else
            { cyc(6); g = (uint16_t)(g + 0x0004); }
        cyc(94);
        wr16(W(menu_gauge), g);
        if (!((uint16_t)(g - target) & 0x8000)) {
            cyc(92);
            g = rd16(W(menu_gauge));
            cyc(16);
            wr16(W(dp_02), g);
            cyc(70);
            return g;
        }
        cyc(down ? 62 : 40);        /* SEC, SBC, BPL; the down arm's BRA */
    }
    cyc(70);
    wr16(W(menu_gauge), 0xFF00);
    cyc(92);
    return 0xFF00;
}

/* MenuGaugeSlideToTarget on carried registers. */
void MenuGaugeSlideToTarget_cr(Carried *cr)
{
    cr->a = MenuGaugeSlideToTarget();
}

void abi_MenuGaugeSlideToTarget(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuGaugeSlideToTarget_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* The vehicle kinds by the handler word at +$27 of the object $0346 names
   (bank $7E object table), and each kind's offset into the gauge table. */
static const uint16_t gauge_kinds[5] = {0x81B9, 0x853D, 0x8411, 0x82E5, 0x8669};
static const uint16_t gauge_offsets[5] = {0x0000, 0x0098, 0x0130, 0x01C8, 0x0260};

/*
 * @routine 7F:1224 GaugeTableIndex
 * WHAT IT DOES: Returns X plus the gauge table offset for the kind of the
 * vehicle object $0346 names: 0, $98, $130, $1C8 or $260 for the handlers
 * $81B9, $853D, $8411, $82E5 and $8669 at its +$27, and 0 for any other.
 * The last two kinds and the default are not reached in the traces so far.
 * HOW: X is kept at $11DF; a compare chain on the 16-bit handler; P is
 * saved and restored.
 * Entry: any widths, DB a bank where $0346 is work RAM; X = the value.
 * Exit: A = X = the index; P unchanged.
 */
uint16_t GaugeTableIndex(uint8_t bank, uint16_t value)
{
    uint16_t obj, kind, off = 0;
    unsigned i;

    cyc(90);
    wr16(W(scratch_11df), value);
    cyc(24);
    obj = rd16(0x7E0000 | W(wram_0346));
    cyc(32);
    kind = rd16(0x7E0000 | W(obj_27) + obj);
    for (i = 0; i < 5; i++)
        { cyc(40); if (kind == gauge_kinds[i]) {
            cyc(6);
            off = gauge_offsets[i];
            break;
        } }
    cyc(i == 4 ? 70 : 92);                  /* the last case has no BRA */
    return cyc_then(84, (uint16_t)(off + rd16(W(scratch_11df))));
}

/* GaugeTableIndex on carried registers. */
void GaugeTableIndex_cr(Carried *cr)
{
    cr->a = GaugeTableIndex(cr->db, cr->x);
    cr->x = (cr->p & P_X) ? (uint8_t)cr->a : cr->a;
}

void abi_GaugeTableIndex(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    GaugeTableIndex_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 7F:11E9 GaugeFromSpeed
 * WHAT IT DOES: Turns a speed into the gauge: for the value at Super FX
 * RAM $70:004A + $02 (made positive and halved when negative), capped at
 * $257 and divided by 4, the byte at $0B:B158 + GaugeTableIndex; stored
 * in direct page $02. When $02 is 0, $02 stays 0. The zero case and the
 * cap are not reached in the traces so far; an LDA #0 after the BRA that
 * ends the scaling is never run.
 * HOW: P is saved and restored; the table byte is read with 8-bit A.
 * Entry: any widths, D = $0000; $02 = the speed's offset. Exit: A = the
 * gauge (0-$FF); X as GaugeTableIndex, unchanged when $02 is 0; P
 * unchanged.
 */
uint16_t GaugeFromSpeed(uint8_t bank, uint16_t *x)
{
    cyc(60);
    uint16_t a = rd16(W(dp_02));

    if (a) {
        *x = a;
        cyc(92);
        a = rd16(0x70004A + *x);
        if (a & 0x8000)
            { cyc(46); a = (uint16_t)((uint16_t)(~a + 1) >> 1); }
        cyc(178);
        if (a >= 0x0257)
            { cyc(18); a = 0x0257; }
        a = (uint16_t)(a >> 2);
        *x = GaugeTableIndex(bank, a);
        cyc(76);
        a = rd8(0x0BB158 + *x);
    } else { cyc(84); }                 /* CMP, BNE, BRL */
    cyc(48);
    wr16(W(dp_02), a);
    return a;
}

/* GaugeFromSpeed on carried registers. */
void GaugeFromSpeed_cr(Carried *cr)
{
    uint16_t x = cr->x;

    cr->a = GaugeFromSpeed(cr->db, &x);
    cr->x = (cr->p & P_X) ? (cyc(70), (uint8_t)x) : (cyc(70), x);
}

void abi_GaugeFromSpeed(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    GaugeFromSpeed_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 7F:120E GaugeFromSpeedZero
 * WHAT IT DOES: GaugeFromSpeed's LDA #0 after the BRA that ends the
 * scaling, which nothing reaches: the gauge for 0 (the byte at $0B:B158 +
 * GaugeTableIndex of 0) into $02.
 * HOW: As GaugeFromSpeed from there; ends pulling the P its start pushed.
 * Entry: M 16-bit, X 16-bit, D = $0000; P on the stack above the return
 * address. Exit: as GaugeFromSpeed.
 */
uint16_t GaugeFromSpeedZero(uint8_t bank, uint16_t *x)
{
    uint16_t a;

    cyc(84);
    *x = GaugeTableIndex(bank, 0x0000);
    cyc(124);
    a = rd8(0x0BB158 + *x);
    wr16(W(dp_02), a);
    return a;
}

/* The tail pulls the P the routine's start pushed. */
/* GaugeFromSpeedZero on carried registers. */
void GaugeFromSpeedZero_cr(Carried *cr)
{
    uint16_t x;

    cr->a = GaugeFromSpeedZero(cr->db, &x);
    cr->x = x;
    cpu.s++;
    cyc(20);
    r_plp(cr, rd8((uint16_t)cpu.s));
}

void abi_GaugeFromSpeedZero(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    GaugeFromSpeedZero_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 7F:112F SpeedGaugeUpdate
 * WHAT IT DOES: Updates the speed gauge from A and copies it to $0E49:
 * with $0D53 clear the gauge comes from the speed (GaugeFromSpeed, $02 =
 * A), and while $0DD6 is below 6 and $117E is not negative it then slides
 * toward it (MenuGaugeSlideToTarget); with $0D53 set and $0DD6 below 6
 * the gauge follows button B (MenuGaugeFromButtonB), and with $0DD6 at 6
 * or more it comes from the speed alone.
 * HOW: $02 holds the value throughout; its low byte is stored at $0E49
 * with 8-bit A. P is saved and restored.
 * Entry: any widths, D = $0000; A = the speed's offset (16 bits are
 * stored). Exit: A low byte = $02, high byte from the last step; X as
 * GaugeFromSpeed, or unchanged; P unchanged.
 */
uint16_t SpeedGaugeUpdate(uint8_t bank, uint16_t a, uint16_t *x)
{
    cyc(60);
    wr16(W(dp_02), a);
    cyc(54);
    if (rd8(W(wram_0D53))) {
        cyc(48);
        if (rd8(W(wram_0DD6)) >= 0x06) {
            cyc(84);
            a = GaugeFromSpeed(bank, x);
        } else {
            cyc(78);
            a = MenuGaugeFromButtonB();
        }
    cyc(70);
        } else {
        cyc(68);
        a = GaugeFromSpeed(bank, x);
        cyc(124);
        if (rd8(W(wram_0DD6)) < 0x06) {
            cyc(78);
            a = rd16(0x7E0000 | W(menu_gauge));
            if (!(a & 0x8000))
                { cyc(62); a = MenuGaugeSlideToTarget(); } else { cyc(22); }
        } else { cyc(38); }
    }
    cyc(38);
    a = (uint16_t)((a & 0xFF00) | rd8(W(dp_02)));
    cyc(32);
    wr8(W(wram_0E49), (uint8_t)a);
    return a;
}

/* SpeedGaugeUpdate on carried registers. */
void SpeedGaugeUpdate_cr(Carried *cr)
{
    uint16_t x = cr->x;

    cr->a = SpeedGaugeUpdate(cr->db, cr->a, &x);
    cr->x = (cr->p & P_X) ? (uint8_t)x : (cyc(72), x);
}

void abi_SpeedGaugeUpdate(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    SpeedGaugeUpdate_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}


/*
 * @routine 7F:0BEF SpeedGauge2Update
 * WHAT IT DOES: Updates the second speed gauge ($1180) from A and copies
 * it to $0E47. With $0D53 set it follows joypad 2's B button: up by $10
 * (to $7F) while held, down by 6 (to 0) otherwise. With $0D53 clear the
 * target in direct page $02 comes from the Super FX RAM values at A: the
 * speed $70:004A / 4 (0 when negative) plus $70:0050 / 8 while $70:0044 is
 * below 4, else half the speed (made positive), at most $FF. A negative
 * gauge just shows the target; otherwise the gauge steps 4 toward it and
 * becomes $FFFF (shows the target) once within 4. Several cases (A = 0,
 * the negative values, the cap, the gauge below the target) are not
 * reached in the traces so far.
 * HOW: 16-bit arithmetic; the halving is CMP #$8000 then ROR; X is pushed
 * and pulled. P is saved and restored. With A = 0 the value is stored with
 * 16-bit A (also clearing $0E48).
 * Entry: any widths, D = $0000; A = the offset of the values. Exit: A =
 * the value shown (16 bits); X, Y, P unchanged.
 */
uint16_t SpeedGauge2Update(uint8_t bank, uint16_t a)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t x, g, t, v;

    cyc(60);
    wr16(W(dp_02), a);
    cyc(46);
    if (rd8(0x7E0000 | W(wram_0D53))) {
        cyc(92);
        if (rd16(0x7E0000 | W(joy2_held)) & 0x8000) {
            cyc(70);
            g = (uint16_t)(rd16(0x7E0000 | W(speed_gauge_2_value)) + 0x0010);
            if (g >= 0x007F)
                { cyc(18); g = 0x007F; }
            cyc(108);
            wr16(0x7E0000 | W(speed_gauge_2_value), g);
        } else {
            cyc(64);
            g = rd16(0x7E0000 | W(speed_gauge_2_value));
            if (g) {
                g = (uint16_t)(g - 0x0006);
                if (g & 0x8000)
                    { cyc(18); g = 0x0000; }
                cyc(100);
                wr16(0x7E0000 | W(speed_gauge_2_value), g);
            }
            cyc(22);
        }
        cyc(24);
        g = rd16(0x7E0000 | W(speed_gauge_2_value));
        cyc(54);
        wr8(W(near_car_pos), (uint8_t)g);
        return g;
    }
    cyc(60);
    a = rd16(W(dp_02));
    if (!a) {
        cyc(168);
        wr16(W(near_car_pos), a);
        return a;
    }
    x = a;
    cyc(122);
    v = rd16(0x70004A + x);
    cyc(60);
    wr16(W(dp_02), (v & 0x8000) ? (cyc(2), 0x0000) : (uint16_t)(v >> 2));
    cyc(v & 0x8000 ? 32 : 54);          /* the LSR arm's BRA */
    if ((uint16_t)(rd16(0x700044 + x) - 4) & 0x8000) {
        cyc(78);
        t = (uint16_t)(rd16(0x700050 + x) >> 3);
        cyc(72);
        t = (uint16_t)(t + rd16(W(dp_02)));
    } else {
        cyc(72);
        v = rd16(0x70004A + x);
        t = (uint16_t)((v & 0x8000) | v >> 1);
        if (t & 0x8000)
            { cyc(54); t = (uint16_t)(~t + 1); }
        cyc(60);
    }
    if (t >= 0x00FF)
        { cyc(18); t = 0x00FF; }
    cyc(62);
    wr16(W(dp_02), t);
    cyc(24);
    g = rd16(0x7E0000 | W(speed_gauge_2_value));
    if (g & 0x8000) {
        cyc(38);
        a = rd16(W(dp_02));
        cyc(90);
        wr8(W(near_car_pos), (uint8_t)a);
        return a;
    }
    cyc(76);
    g = rd16(0x7E0000 | W(speed_gauge_2_value));
    cyc(16);
    t = rd16(W(dp_02));
    if (g != t) {
        int down = g > t;
        g = (uint16_t)(down ? g - 4 : (cyc(6), g + 4));
        cyc(94);
        wr16(0x7E0000 | W(speed_gauge_2_value), g);
        cyc(30);
        v = (uint16_t)(g - rd16(W(dp_02)) + 4);
        if ((v & 0x8000) || (cyc(40), v >= 0x0008)) {
            cyc(84);
            a = rd16(0x7E0000 | W(speed_gauge_2_value));
            cyc(54);
            wr8(W(near_car_pos), (uint8_t)a);
            return a;
        } else { cyc(down ? 54 : 32); }   /* down: BRA to the shared store */
    }
    cyc(70);
    wr16(0x7E0000 | W(speed_gauge_2_value), 0xFFFF);
    cyc(16);
    a = rd16(W(dp_02));
    cyc(76);
    wr8(W(near_car_pos), (uint8_t)a);
    return a;
}

/* SpeedGauge2Update on carried registers. */
void SpeedGauge2Update_cr(Carried *cr)
{
    cr->a = SpeedGauge2Update(cr->db, cr->a);
    cyc(72);
}

void abi_SpeedGauge2Update(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    SpeedGauge2Update_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}
