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

void abi_DebugPrintHexByte(void);

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
    wr8(HDMAEN, 0x00);
    cyc(32);
    wr8(NMITIMEN, 0x00);
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

void abi_SystemHalt(void)
{
    SystemHalt();
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

void abi_SystemHaltSpawn(void)
{
    SystemHaltSpawn();
}

/*
 * @routine 0B:E21F Hang
 * WHAT IT DOES: Stops the game: a branch to itself, for ever.
 * HOW: BRA to its own address (cpu_loop_forever). Never returns.
 * Entry: any. Exit: does not return.
 */
void Hang(void)
{
    cpu_loop_forever(0x0BE21F);
}

void abi_Hang(void)
{
    Hang();
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
void DebugByteHalt(void)
{
    uint32_t b;
    uint16_t v;

    op_sep(P_M);
    op_rep(P_X);
    cpu.db = 0x03;
    cpu.p = nz8(cpu.p, cpu.db);
    b = (uint32_t)cpu.db << 16;
    cyc(94);
    for (;;) {
        cyc(70);
        ppu_wr(INIDISP, 0x0F);
        cyc(40);
        wr8(b | 0x0D61, 0x0F);
        cyc(32);
        wr8((uint16_t)(cpu.d + 0x00), 0x02);
        cyc(70);
        wr8((uint16_t)(cpu.d + 0x46), 0x80);
        cyc(32);
        wr8((uint16_t)(cpu.d + 0x47), 0x06);
        cyc(40);
        wr8(b | 0x0307, 0x00);
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x69);
        cpu.p = nz8(cpu.p, 0x69);
        (cyc(90), abi_DebugPrintHexByte()) /* PLA, JSL */;
        op_sep(P_M);
        cyc(46);
        wr8(b | 0x0357, 0x00);
        op_rep(P_M);
        cyc(24);
        wr16(b | 0x0D7F, 0x0000);
        cyc(24);
        v = (uint16_t)(rd16(b | 0x0374) & 0xFFF7);
        cyc(48);
        wr16(b | 0x303A, v);
        cyc(56);
        wr16(0x70021A, 0x2C00);
        cyc(16);
        wr16((uint16_t)(cpu.d + 0x48), 0x2C00);
        cyc(24);
        v = (uint16_t)(rd16(b | 0x0374) | 0x0018);
        cpu.a = v;
        cyc(48);
        wr16(b | 0x303A, v);
        op_sep(P_M);
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x0B);
        cpu.p = nz8(cpu.p, 0x0B);
        cyc(70);
        wr8(GSU_SCBR, 0x0B);
    }
}

void abi_DebugByteHalt(void)
{
    DebugByteHalt();
}
