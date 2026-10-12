/*
 * gauge.c: a menu value at $117E that rises while button B is held and falls
 * back when it is released, and slides toward a target. The menu frame code
 * reads the result from direct page $02.
 */
#include "machine.h"
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

    if (rd16(JOY1_HELD) & 0x8000) {
        g = (uint16_t)(rd16(MENU_GAUGE) + 0x0028);
        if (g >= 0x00FF)
            g = 0x00FF;
        wr16(MENU_GAUGE, g);
    } else {
        g = rd16(MENU_GAUGE);
        if (g != 0) {
            g = (uint16_t)(g - 0x000C);
            if (g & 0x8000)
                g = 0;
            wr16(MENU_GAUGE, g);
        }
    }
    g = rd16(MENU_GAUGE);
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
