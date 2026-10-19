/*
 * gauge.c: the speed gauge. In the menus a value at $117E rises while
 * button B is held and falls back when it is released, and slides toward a
 * target; in a race it comes from the vehicle's speed. The result is kept
 * in direct page $02 and copied to $0E49.
 */
#include "machine.h"
#include "cpu_ops.h"
#include "wram.h"

#define MENU_GAUGE 0x00117Eu

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

    cyc(154);
    if (rd16(JOY1_HELD) & 0x8000) {
        cyc(78);
        g = (uint16_t)(rd16(MENU_GAUGE) + 0x0028);
        if (g >= 0x00FF)
            { cyc(18); g = 0x00FF; }
        cyc(116);
        wr16(MENU_GAUGE, g);
    } else {
        cyc(72);
        g = rd16(MENU_GAUGE);
        if (g != 0) {
            g = (uint16_t)(g - 0x000C);
            if (g & 0x8000)
                { cyc(18); g = 0; }
            cyc(108);
            wr16(MENU_GAUGE, g);
        }
    cyc(22);
        }
    cyc(32);
    g = rd16(MENU_GAUGE);
    cyc(16);
    wr16(0x000002, g);
    return g;
}

void abi_MenuGaugeFromButtonB(void)
{
    cpu.a = MenuGaugeFromButtonB();
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
    uint16_t g = rd16(MENU_GAUGE), target = rd16(0x000002);

    if (g != target) {
        if (g >= target)
            g = (uint16_t)(g - 0x0008);
        else
            g = (uint16_t)(g + 0x0004);
        wr16(MENU_GAUGE, g);
        if (!((uint16_t)(g - target) & 0x8000)) {
            g = rd16(MENU_GAUGE);
            wr16(0x000002, g);
            return g;
        }
    }
    wr16(MENU_GAUGE, 0xFF00);
    return 0xFF00;
}

void abi_MenuGaugeSlideToTarget(void)
{
    cpu.a = MenuGaugeSlideToTarget();
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

    wr16(0x0011DF, value);
    obj = rd16((uint32_t)bank << 16 | 0x0346);
    kind = rd16(0x7E2027 + obj);
    for (i = 0; i < 5; i++)
        if (kind == gauge_kinds[i]) {
            off = gauge_offsets[i];
            break;
        }
    return (uint16_t)(off + rd16(0x0011DF));
}

void abi_GaugeTableIndex(void)
{
    cpu.a = GaugeTableIndex(cpu.db, cpu.x);
    cpu.x = (cpu.p & P_X) ? (uint8_t)cpu.a : cpu.a;
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
    uint16_t a = rd16(0x000002);

    if (a) {
        *x = a;
        cyc(92);
        a = rd16(0x70004A + *x);
        if (a & 0x8000)
            { cyc(46); a = (uint16_t)((uint16_t)(~a + 1) >> 1); }
        if (a >= 0x0257)
            { cyc(18); a = 0x0257; }
        a = (uint16_t)(a >> 2);
        cyc(178);
        *x = GaugeTableIndex(bank, a);
        cyc(76);
        a = rd8(0x0BB158 + *x);
    }
    cyc(132);
    wr16(0x000002, a);
    return a;
}

void abi_GaugeFromSpeed(void)
{
    uint16_t x = cpu.x;

    cyc(60);
    cpu.a = GaugeFromSpeed(cpu.db, &x);
    cpu.x = (cpu.p & P_X) ? (uint8_t)x : x;
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
void abi_GaugeFromSpeedZero(void)
{
    uint16_t x, a;

    cyc(84);
    x = GaugeTableIndex(cpu.db, 0x0000);
    cyc(208);
    a = rd8(0x0BB158 + x);
    wr16(0x000002, a);
    cpu.a = a;
    cpu.x = x;
    cpu.s++;
    cyc(20);
    op_plp(rd8((uint16_t)cpu.s));
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
    wr16(0x000002, a);
    if (rd8(0x000D53)) {
        if (rd8(0x000DD6) >= 0x06) {
            a = GaugeFromSpeed(bank, x);
        } else {
            a = MenuGaugeFromButtonB();
        }
    } else {
        a = GaugeFromSpeed(bank, x);
        if (rd8(0x000DD6) < 0x06) {
            a = rd16((uint32_t)bank << 16 | 0x117E);
            if (!(a & 0x8000))
                a = MenuGaugeSlideToTarget();
        }
    }
    a = (uint16_t)((a & 0xFF00) | rd8(0x000002));
    wr8(0x000E49, (uint8_t)a);
    return a;
}

void abi_SpeedGaugeUpdate(void)
{
    uint16_t x = cpu.x;

    cpu.a = SpeedGaugeUpdate(cpu.db, cpu.a, &x);
    cpu.x = (cpu.p & P_X) ? (uint8_t)x : x;
}

#define GAUGE_2 0x1180u   /* the second gauge, through the data bank */

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

    wr16(0x000002, a);
    if (rd8(b | 0x0D53)) {
        if (rd16(b | 0x030D) & 0x8000) {
            g = (uint16_t)(rd16(b | GAUGE_2) + 0x0010);
            if (g >= 0x007F)
                g = 0x007F;
            wr16(b | GAUGE_2, g);
        } else {
            g = rd16(b | GAUGE_2);
            if (g) {
                g = (uint16_t)(g - 0x0006);
                if (g & 0x8000)
                    g = 0x0000;
                wr16(b | GAUGE_2, g);
            }
        }
        g = rd16(b | GAUGE_2);
        wr8(0x000E47, (uint8_t)g);
        return g;
    }
    a = rd16(0x000002);
    if (!a) {
        wr16(0x000E47, a);
        return a;
    }
    x = a;
    v = rd16(0x70004A + x);
    wr16(0x000002, (v & 0x8000) ? 0x0000 : (uint16_t)(v >> 2));
    if ((uint16_t)(rd16(0x700044 + x) - 4) & 0x8000) {
        t = (uint16_t)(rd16(0x700050 + x) >> 3);
        t = (uint16_t)(t + rd16(0x000002));
    } else {
        v = rd16(0x70004A + x);
        t = (uint16_t)((v & 0x8000) | v >> 1);
        if (t & 0x8000)
            t = (uint16_t)(~t + 1);
    }
    if (t >= 0x00FF)
        t = 0x00FF;
    wr16(0x000002, t);
    g = rd16(b | GAUGE_2);
    if (g & 0x8000) {
        a = rd16(0x000002);
        wr8(0x000E47, (uint8_t)a);
        return a;
    }
    g = rd16(b | GAUGE_2);
    t = rd16(0x000002);
    if (g != t) {
        g = (uint16_t)(g > t ? g - 4 : g + 4);
        wr16(b | GAUGE_2, g);
        v = (uint16_t)(g - rd16(0x000002) + 4);
        if ((v & 0x8000) || v >= 0x0008) {
            a = rd16(b | GAUGE_2);
            wr8(0x000E47, (uint8_t)a);
            return a;
        }
    }
    wr16(b | GAUGE_2, 0xFFFF);
    a = rd16(0x000002);
    wr8(0x000E47, (uint8_t)a);
    return a;
}

void abi_SpeedGauge2Update(void)
{
    cpu.a = SpeedGauge2Update(cpu.db, cpu.a);
}
