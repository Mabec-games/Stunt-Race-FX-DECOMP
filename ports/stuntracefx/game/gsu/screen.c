/*
 * screen.c: the Super FX's screen buffers. The renderer draws into one of
 * two buffers while the other is shown; $0D7F selects which, and $0D7D holds
 * the matching offset (0 or 8).
 */
#include "machine.h"
#include "native.h"
#include "hw.h"
#include "carried.h"
#include "wram_names.h"
#include "wram_struct.h"

/*
 * @routine 03:B83C GsuScreenFlip
 * WHAT IT DOES: Swaps the Super FX's draw buffer and resets its screen base.
 * HOW: Toggles $0D7F between 0 and 1 and sets $0D7D to 8 or 0 to match;
 * gives RAM back to the S-CPU (screen mode value without bit 3), stores the
 * buffer start $2C00 at $70:021A and direct page $48, hands ROM and RAM to
 * the Super FX again (bits 4 and 3) and sets the screen base register to
 * $0B. Work RAM is reached through the data bank; the screen mode stores
 * are 16-bit.
 * Entry: D = $0000. Exit: A = the high byte of the last screen mode value
 * and $0B; M 8-bit; N, Z clear.
 */
uint16_t GsuScreenFlip(uint8_t bank)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t mode;

    cyc(46);
    if (RD16(gsu_buffer_select)) {
        cyc(62);
        WR16(gsu_buffer_select, 0x0000);
        cyc(48);
        WR16(gsu_buffer_offset, 0x0008);
    } else {
        cyc(68);
        WR16(gsu_buffer_select, cyc_then(14, (uint16_t)(RD16(gsu_buffer_select) + 1)));
        cyc(48);
        WR16(gsu_buffer_offset, 0x0000);
    }
    cyc(24);
    hw_wr16(b | (GSU_SCMR & 0xFFFF), cyc_then(48, (uint16_t)(RD16(gsu_scmr_shadow) & 0xFFF7)));
    cyc(56);
    wr16(0x70021A, 0x2C00);
    cyc(16);
    wr16(W(gsu_screen_start_dp), 0x2C00);
    cyc(24);
    mode = (uint16_t)(RD16(gsu_scmr_shadow) | 0x0018);
    cyc(48);
    hw_wr16(b | (GSU_SCMR & 0xFFFF), mode);
    cyc(70);
    hw_wr(GSU_SCBR, 0x0B);
    cyc(42);
    return (uint16_t)((mode & 0xFF00) | 0x0B);
}

/* GsuScreenFlip on carried registers. */
void GsuScreenFlip_cr(Carried *cr)
{
    cr->a = GsuScreenFlip(cr->db);
    set_p(cr, (uint8_t)((cr->p & ~(P_N | P_Z)) | P_M));
}

void abi_GsuScreenFlip(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    GsuScreenFlip_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:FC49 GsuScreenFlipMenu
 * WHAT IT DOES: The menu screens' form of GsuScreenFlip: swaps the Super
 * FX's draw buffer between the one at $2C00 (screen base $0B) and the one
 * at $9400 (screen base $25), leaving $0D7D alone.
 * HOW: $0D7F set: clears it and selects $2C00; clear: sets it to 1 and
 * selects $9400. The start goes to $70:021A and direct page $48 between
 * giving RAM back to the S-CPU and handing ROM and RAM to the Super FX
 * again; then the screen base register.
 * Entry: D = $0000. Exit: A = the high byte of the last screen mode value
 * and the screen base; M 8-bit; N, Z clear.
 */
uint16_t GsuScreenFlipMenu(uint8_t bank)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t mode, start;
    uint8_t base;

    cyc(46);
    if (RD16(gsu_buffer_select)) {
        cyc(62);
        WR16(gsu_buffer_select, 0x0000);
        start = 0x2C00;
        base = 0x0B;
    } else {
        cyc(68);
        WR16(gsu_buffer_select, cyc_then(14, (uint16_t)(RD16(gsu_buffer_select) + 1)));
        start = 0x9400;
        base = 0x25;
    }
    cyc(24);
    hw_wr16(b | (GSU_SCMR & 0xFFFF), cyc_then(48, (uint16_t)(RD16(gsu_scmr_shadow) & 0xFFF7)));
    cyc(56);
    wr16(0x70021A, start);
    cyc(16);
    wr16(W(gsu_screen_start_dp), start);
    cyc(24);
    mode = (uint16_t)(RD16(gsu_scmr_shadow) | 0x0018);
    cyc(48);
    hw_wr16(b | (GSU_SCMR & 0xFFFF), mode);
    cyc(70);
    hw_wr(GSU_SCBR, base);
    cyc(42);
    return (uint16_t)((mode & 0xFF00) | base);
}

/* GsuScreenFlipMenu on carried registers. */
void GsuScreenFlipMenu_cr(Carried *cr)
{
    cr->a = GsuScreenFlipMenu(cr->db);
    set_p(cr, (uint8_t)((cr->p & ~(P_N | P_Z)) | P_M));
}

void abi_GsuScreenFlipMenu(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    GsuScreenFlipMenu_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 02:D8CD GsuScreenToggle
 * WHAT IT DOES: Toggles $0D7F between 0 and 1 and points the Super FX at
 * the matching screen: start $2C00 with screen base $0B when $0D7F becomes
 * 0, start $9400 with screen base $25 when it becomes 1.
 * HOW: As GsuScreenFlip without $0D7D: gives RAM back to the S-CPU, stores
 * the start at $70:021A and direct page $48, hands ROM and RAM to the Super
 * FX again, then a long store to the screen base register. Work RAM and the
 * screen mode register are reached through the data bank, 16-bit.
 * Entry: D = $0000. Exit: A = the high byte of the last screen mode value
 * and the screen base; M 8-bit; N, Z clear.
 */
uint16_t GsuScreenToggle(uint8_t bank)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t mode, start;
    uint8_t base;

    cyc(46);
    if (RD16(gsu_buffer_select)) {
        cyc(62);
        WR16(gsu_buffer_select, 0x0000);
        start = 0x2C00;
        base = 0x0B;
    } else {
        cyc(68);
        WR16(gsu_buffer_select, cyc_then(14, (uint16_t)(RD16(gsu_buffer_select) + 1)));
        start = 0x9400;
        base = 0x25;
    }
    cyc(24);
    hw_wr16(b | (GSU_SCMR & 0xFFFF), cyc_then(48, (uint16_t)(RD16(gsu_scmr_shadow) & 0xFFF7)));
    cyc(56);
    wr16(0x70021A, start);
    cyc(16);
    wr16(W(gsu_screen_start_dp), start);
    cyc(24);
    mode = (uint16_t)(RD16(gsu_scmr_shadow) | 0x0018);
    cyc(48);
    hw_wr16(b | (GSU_SCMR & 0xFFFF), mode);
    cyc(70);
    hw_wr(GSU_SCBR, base);
    cyc(42);
    return (uint16_t)((mode & 0xFF00) | base);
}

/* GsuScreenToggle on carried registers. */
void GsuScreenToggle_cr(Carried *cr)
{
    cr->a = GsuScreenToggle(cr->db);
    set_p(cr, (uint8_t)((cr->p & ~(P_N | P_Z)) | P_M));
}

void abi_GsuScreenToggle(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    GsuScreenToggle_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}
