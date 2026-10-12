/*
 * run.c: starting the Super FX and waiting for it.
 *
 * The S-CPU hands ROM and RAM to the Super FX (screen mode register bits 4
 * and 3), sets its program bank and writes its program counter, which starts
 * it. While it runs, the S-CPU polls its status and counts the polls in
 * direct page $FE-$FF; when it stops, the S-CPU takes the bus back by
 * restoring its own screen mode value.
 */
#include "machine.h"
#include "hw.h"
#include "wram.h"

/* Poll until the Super FX stops (status bit 5 clear), counting each poll. */
static void gsu_wait(void)
{
    do
        wr16(GSU_WAIT_COUNT, (uint16_t)(rd16(GSU_WAIT_COUNT) + 1));
    while (rd8(GSU_SFR) & 0x20);
}

/*
 * @routine 7E:E1CB GsuWait
 * WHAT IT DOES: Waits for a running Super FX program to stop, then takes the
 * ROM and RAM back for the S-CPU.
 * HOW: Steps the 16-bit poll count and reads the status register until its
 * running bit clears, then writes the S-CPU's screen mode value ($0374).
 * Entry: X 16-bit, D = $0000. Exit: A low byte = the screen mode value; N, Z
 * from it; M 8-bit.
 */
uint8_t GsuWait(void)
{
    uint8_t mode;

    gsu_wait();
    mode = rd8(GSU_SCMR_SHADOW);
    wr8(GSU_SCMR, mode);
    return mode;
}

void abi_GsuWait(void)
{
    uint8_t mode = GsuWait();

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | mode);
    cpu.p = (uint8_t)(nz8(cpu.p, mode) | P_M);
}

/*
 * @routine 7E:E1F5 GsuRun
 * WHAT IT DOES: Runs a Super FX program to completion.
 * HOW: Sets the program bank, gives ROM and RAM to the Super FX, clears its
 * status and writes the start address to R15, which starts it; then waits as
 * GsuWait does. The data bank is 0 inside and restored on return.
 * Entry: M 8-bit, X 16-bit, D = $0000; A = program bank, X = start address.
 * Exit: A low byte = the screen mode value; N, Z from the data bank pulled
 * back on return; X unchanged.
 */
uint8_t GsuRun(uint8_t bank, uint16_t start)
{
    wr8(GSU_PBR, bank);
    wr8(GSU_SCMR, rd8(GSU_SCMR_SHADOW) | 0x18);
    wr8(GSU_SFR, 0x00);
    wr16(GSU_R15, start);
    return GsuWait();
}

void abi_GsuRun(void)
{
    uint8_t mode = GsuRun((uint8_t)cpu.a, cpu.x);

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | mode);
    cpu.p = nz8(cpu.p, cpu.db);
}
