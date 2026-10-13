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

uint16_t GsuParamsUpdate(void);

/* The body; returns the value the abi_ wrapper needs. */
static uint8_t gsu_ab13_with_params(uint8_t bank, uint16_t *last)
{
    uint32_t b = (uint32_t)bank << 16;
    uint8_t mode;

    *last = GsuParamsUpdate();
    wr8(b | 0x3034, 0x01);
    wr8(b | 0x303A, (uint8_t)(rd8(b | 0x0374) | 0x18));
    wr16(b | 0x301E, 0xAB13);
    do {
        uint8_t n = (uint8_t)(rd8(b | 0x1F21) + 1);

        wr8(b | 0x1F21, n);
        if (!n)
            wr8(b | 0x1F22, (uint8_t)(rd8(b | 0x1F22) + 1));
    } while (rd8(b | 0x3030) & 0x20);
    mode = rd8(b | 0x0374);
    wr8(b | 0x303A, mode);
    return mode;
}

/*
 * @routine 7F:0919 GsuAB13WithParams
 * WHAT IT DOES: Updates the renderer's parameter block, runs Super FX
 * program $01:AB13 to completion and takes the bus back, counting the polls
 * in $1F21-$1F22.
 * HOW: GsuParamsUpdate, then as GsuRun but written out: program bank 1,
 * screen mode $0374 | $18, R15 = $AB13; each poll steps the 16-bit count
 * (low byte, then the high byte when the low one wraps) before reading the
 * status. Everything but the parameter copy goes through the data bank.
 * Entry: any widths, D = $0000. Exit: A low byte = the screen mode value,
 * high byte from the last parameter copied; X = $AB13 ($13 when P
 * restores 8-bit index registers); P unchanged.
 */
uint8_t GsuAB13WithParams(uint8_t bank)
{
    uint16_t last;

    return gsu_ab13_with_params(bank, &last);
}

void abi_GsuAB13WithParams(void)
{
    uint16_t last;
    uint8_t mode = gsu_ab13_with_params(cpu.db, &last);

    cpu.a = (uint16_t)((last & 0xFF00) | mode);
    cpu.x = (cpu.p & P_X) ? 0x0013 : 0xAB13;
}

void abi_BucketTriggersFromView(void);
void abi_ObjectDrawListBuild(void);
void abi_BucketTriggersFixedArea(void);
void abi_ObjectGroupsActivate(void);
void abi_GsuRecordsUpload(void);

/*
 * @routine 7E:E1E1 FrameObjectsWhileGsuRuns
 * WHAT IT DOES: The S-CPU's share of a race frame while the Super FX
 * draws: the trigger list for the view (BucketTriggersFromView), the draw
 * list (ObjectDrawListBuild), the fixed area's triggers
 * (BucketTriggersFixedArea), the object groups (ObjectGroupsActivate);
 * then waits for the Super FX (GsuWait) and uploads the records it needs
 * next (GsuRecordsUpload).
 * HOW: Six calls in turn.
 * Entry: X 16-bit, D = $0000. Exit: as GsuRecordsUpload leaves it.
 */
void FrameObjectsWhileGsuRuns(void)
{
    abi_BucketTriggersFromView();
    abi_ObjectDrawListBuild();
    abi_BucketTriggersFixedArea();
    abi_ObjectGroupsActivate();
    abi_GsuWait();
    abi_GsuRecordsUpload();
}

void abi_FrameObjectsWhileGsuRuns(void)
{
    FrameObjectsWhileGsuRuns();
}

/*
 * @routine 7E:E221 FrameRunGsu8FCA
 * WHAT IT DOES: Starts Super FX program $04:8FCA, does the frame's object
 * work while it runs (FrameObjectsWhileGsuRuns), and keeps the number of
 * polls the wait took ($FE) in $1F1F.
 * HOW: As GsuRun, without waiting before the object work; the poll count
 * in direct page $FE is cleared first. Data bank 0 inside, restored at the
 * end.
 * Entry: D = $0000. Exit: A = the poll count; X, Y as GsuRecordsUpload
 * leaves them; M 8-bit, X 16-bit; N, Z from the data bank pulled back.
 */
void FrameRunGsu8FCA(void)
{
    uint8_t db = cpu.db;
    uint16_t n;

    cpu.p = (uint8_t)((cpu.p | P_M) & ~P_X);
    wr8(0x0000FE, 0x00);
    wr8(0x0000FF, 0x00);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x04);
    cpu.x = 0x8FCA;
    wr8(GSU_PBR, 0x04);
    cpu.db = 0x00;
    wr8(GSU_SCMR, (uint8_t)(rd8(GSU_SCMR_SHADOW) | 0x18));
    wr8(GSU_SFR, 0x00);
    wr16(GSU_R15, 0x8FCA);
    FrameObjectsWhileGsuRuns();
    n = rd16(0x0000FE);
    wr16(0x001F1F, n);
    cpu.a = n;
    cpu.db = db;
    cpu.p = (uint8_t)(nz8(cpu.p | P_M, db) & ~P_X);
}

void abi_FrameRunGsu8FCA(void)
{
    FrameRunGsu8FCA();
}
