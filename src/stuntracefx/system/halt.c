/*
 * halt.c: where the game stops when a fixed-size list overflows. No script
 * reaches these routines; they are the targets of the overflow checks in
 * FrameByteWritePush (SystemHalt), MenuActorSpawnList (SystemHaltSpawn) and
 * MenuActorCopyGroup (Hang).
 */
#include "machine.h"
#include "hw.h"

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

    wr8(INIDISP, 0x0F);
    wr8(HDMAEN, 0x00);
    wr8(NMITIMEN, 0x00);
    wr8(TM, 0x00);
    for (;;) {
        wr8(CGADD, 0x00);
        wr8(CGADD, 0x00);
        wr8(CGDATA, (uint8_t)y);
        wr8(CGDATA, (uint8_t)y);
        y = (uint16_t)(y + 1);
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
 * HOW: BRA to its own address. Never returns.
 * Entry: any. Exit: does not return.
 */
void Hang(void)
{
    for (;;)
        ;
}

void abi_Hang(void)
{
    Hang();
}
