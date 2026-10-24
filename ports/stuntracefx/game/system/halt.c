/*
 * halt.c: where the game stops when a fixed-size list overflows. No script
 * reaches these routines; they are the targets of the overflow checks in
 * FrameByteWritePush (SystemHalt), MenuActorSpawnList (SystemHaltSpawn) and
 * MenuActorCopyGroup (Hang). DebugByteHalt is a debug stop that nothing
 * calls.
 */
#include "machine.h"
#include "native.h"
#include "hw.h"
#include "cpu_ops.h"

#include "system_calls.h"
#include "carried.h"
#include "wram_names.h"
#include "wram_struct.h"

/*
 * @routine 0B:E41D SystemHalt
 * WHAT IT DOES: Stops the game with a flickering screen: display on at full
 * brightness with no layers, HDMA and interrupts off, then the backdrop
 * colour cycles for ever.
 * HOW: INIDISP = $0F, HDMAEN = NMITIMEN = TM = 0; then a loop that sets
 * the palette address to 0 (written twice) and writes Y's low byte to both
 * bytes of colour 0, increments Y and waits nine NOPs. Never returns.
 * Entry: X 16-bit. Exit: does not return.
 */
/* Display on at full brightness with no layers, HDMA and interrupts off,
   then colour 0 cycles for ever. */
static void halt(void)
{
    uint16_t y = 0;
    int first = 1;

    cyc(70);
    ppu_wr(INIDISP, 0x0F);
    cyc(48);
    hdmaen_wr(HDMAEN, 0x00);
    cyc(32);
    hw_wr(NMITIMEN, 0x00);
    cyc(32);
    ppu_wr(TM, 0x00);
    for (;;) {
        cyc(first ? 84 : 210);              /* the first round: LDX, TXY, TXA; then INY, nine NOPs, JMP, TXA */
        ppu_wr(CGADD, 0x00);
        cyc(32);
        ppu_wr(CGADD, 0x00);
        cyc(46);
        ppu_wr(CGDATA, (uint8_t)y);
        cyc(32);
        ppu_wr(CGDATA, (uint8_t)y);
        y = (uint16_t)(y + 1);
        first = 0;
    }
}

void SystemHalt(void)
{
    halt();
}

/* SystemHalt on carried registers: no register work. */
void SystemHalt_cr(Carried *cr)
{
    (void)cr;
    SystemHalt();
}

void abi_SystemHalt(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    SystemHalt_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:E47A SystemHaltSpawn
 * WHAT IT DOES: A second copy of SystemHalt, the target of
 * MenuActorSpawnList's check for a full object pool.
 * HOW: The same code as SystemHalt; its loop jumps back within this copy.
 * Entry: X 16-bit. Exit: does not return.
 */
void SystemHaltSpawn(void)
{
    halt();
}

/* SystemHaltSpawn on carried registers: no register work. */
void SystemHaltSpawn_cr(Carried *cr)
{
    (void)cr;
    SystemHaltSpawn();
}

void abi_SystemHaltSpawn(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    SystemHaltSpawn_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:E21F Hang
 * WHAT IT DOES: Stops the game: a branch to itself, for ever.
 * HOW: BRA to its own address (carried_loop_forever). Never returns.
 * Entry: any. Exit: does not return.
 */
void Hang(Carried *cr)
{
    carried_loop_forever(cr, 0x0BE21F);
}

void abi_Hang(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    Hang(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:D40A DebugByteHalt
 * WHAT IT DOES: A debug stop that nothing calls: for ever, the screen on at
 * full brightness, $0D61 = $0F, $00 = 2, the byte $69 printed as two hex
 * digits (DebugPrintHexByte) at text position $0680 ($46, half $0307 = 0);
 * $0357 and the Super FX buffer select $0D7F cleared; the Super FX screen
 * mode from $0374 with its RAM taken back (bit 3 off), the screen start
 * $70:021A and $48 = $2C00, the mode again with RAM and ROM given to the
 * Super FX (bits $18), and the screen base $0B.
 * HOW: PHP, SEP #$20, REP #$10, PHB, PHK, PLB (never pulled), then a BRA
 * loop; the $69 kept across the stores by PHA, PLA. Never returns.
 * Entry: D = $0000. Exit: does not return.
 */
void DebugByteHalt(uint16_t d)
{
    uint32_t b = 0x030000;
    uint16_t v;

    cyc(22);                            /* REP #$10 */
    cyc(94);
    for (;;) {
        cyc(70);
        ppu_wr(INIDISP, 0x0F);
        cyc(40);
        WR8(fade_level_hi, 0x0F);
        cyc(32);
        wr8((uint16_t)(d + W(dp_00)), 0x02);
        cyc(70);
        wr8((uint16_t)(d + W(text_pos)), 0x80);
        cyc(32);
        wr8((uint16_t)(d + W(text_pos) + 1), 0x06);
        cyc(40);
        WR8(text_half, 0x00);
        (cyc(90), DebugPrintHexByte(0x03, 0x69, P_M)) /* PLA, JSL */;
        cyc(46);
        WR8(wram_0357, 0x00);
        cyc(22);                        /* REP #$20 */
        cyc(24);
        WR16(gsu_buffer_select, 0x0000);
        cyc(24);
        v = (uint16_t)(RD16(gsu_scmr_shadow) & 0xFFF7);
        cyc(48);
        hw_wr16(b | 0x303A, v);
        cyc(56);
        wr16(0x70021A, 0x2C00);
        cyc(16);
        wr16((uint16_t)(d + W(gsu_screen_start_dp)), 0x2C00);
        cyc(24);
        v = (uint16_t)(RD16(gsu_scmr_shadow) | 0x0018);
        cyc(48);
        hw_wr16(b | 0x303A, v);
        cyc(70);
        hw_wr(GSU_SCBR, 0x0B);
    }
}

/* DebugByteHalt on carried registers. */
void DebugByteHalt_cr(Carried *cr)
{
    /* SEP #$20, REP #$10, PHB, PHK, PLB: what an interrupt in the loop
       finds (the C below never returns) */
    set_p(cr, nz8((uint8_t)((cr->p | P_M) & ~P_X), 0x03));
    cr->db = 0x03;
    DebugByteHalt(cr->d);
}

void abi_DebugByteHalt(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    DebugByteHalt_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}
