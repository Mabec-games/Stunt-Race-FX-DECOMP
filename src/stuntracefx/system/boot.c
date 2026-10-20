/*
 * boot.c: the game's start: the registers reset, the reset vector's start,
 * the start after a soft reset and the session's loop.
 */
#include "machine.h"
#include "cpu_ops.h"
#include "hw.h"

void abi_ApuUploadBank19(void);
void abi_SaveUnlocksCheck(void);
void abi_MenuScreen0Run(void);
void abi_CourseMenuRun(void);
void abi_RaceModeSetup(void);
void abi_Set7EFFA6Sequence(void);
void abi_RaceLoop(void);
void abi_RaceVideoInit(void);
void abi_WindowsInit(void);
void abi_CourseDataPointer(void);
void abi_CourseScriptRun(void);
void abi_ObjectPlaceAll(void);
void abi_GsuRun(void);
void abi_SpriteSlot1Countdown(void);
void abi_RaceFrameRun(void);
void abi_RaceFrameRunTwoPlayers(void);
void abi_WramFillZero(void);
void abi_WramFillZeroLong(void);
void abi_SoftResetApuLong(void);
void abi_DemoRunThird(void);

/*
 * @routine 03:89B4 HardwareRegsInit
 * WHAT IT DOES: Puts the video and CPU registers in a known state: the
 * screen blanked at full brightness ($2100 = $8F), every PPU register from
 * $2101 to $2133 to 0 except VMAIN = $80, Mode 7's A and D = $0100, CGWSEL
 * = $30 and COLDATA = $E0; the NMI, IRQs, the multiplier, divider, timers,
 * DMA and HDMA off, WRIO = $FF and slow ROM.
 * HOW: 8-bit registers: A = 0 for most stores, X and Y for the others; the
 * scroll and Mode 7 registers written twice (their two bytes). Through the
 * data bank.
 * Entry: any widths. Exit: A = 0, X = $FF, Y = $30 (8-bit); N set, Z clear.
 */
void abi_HardwareRegsInit(void)
{
    static const struct { uint16_t reg; uint8_t v; } writes[] = {
        {0x2100, 0x8F},
        {0x2101, 0x00},
        {0x2102, 0x00},
        {0x2103, 0x00},
        {0x2105, 0x00},
        {0x2106, 0x00},
        {0x2107, 0x00},
        {0x2108, 0x00},
        {0x2109, 0x00},
        {0x210A, 0x00},
        {0x210B, 0x00},
        {0x210C, 0x00},
        {0x210D, 0x00},
        {0x210D, 0x00},
        {0x210E, 0x00},
        {0x210E, 0x00},
        {0x210F, 0x00},
        {0x210F, 0x00},
        {0x2110, 0x00},
        {0x2110, 0x00},
        {0x2111, 0x00},
        {0x2111, 0x00},
        {0x2112, 0x00},
        {0x2112, 0x00},
        {0x2113, 0x00},
        {0x2113, 0x00},
        {0x2114, 0x00},
        {0x2114, 0x00},
        {0x2115, 0x80},
        {0x2116, 0x00},
        {0x2117, 0x00},
        {0x211A, 0x00},
        {0x211B, 0x00},
        {0x211B, 0x01},
        {0x211C, 0x00},
        {0x211C, 0x00},
        {0x211D, 0x00},
        {0x211D, 0x00},
        {0x211E, 0x00},
        {0x211E, 0x01},
        {0x211F, 0x00},
        {0x211F, 0x00},
        {0x2120, 0x00},
        {0x2120, 0x00},
        {0x2121, 0x00},
        {0x2122, 0x00},
        {0x2123, 0x00},
        {0x2124, 0x00},
        {0x2125, 0x00},
        {0x2126, 0x00},
        {0x2127, 0x00},
        {0x2128, 0x00},
        {0x2129, 0x00},
        {0x212A, 0x00},
        {0x212B, 0x00},
        {0x212C, 0x00},
        {0x212D, 0x00},
        {0x212E, 0x00},
        {0x2130, 0x30},
        {0x2131, 0x00},
        {0x2132, 0xE0},
        {0x2133, 0x00},
        {0x4200, 0x00},
        {0x4201, 0xFF},
        {0x4202, 0x00},
        {0x4203, 0x00},
        {0x4204, 0x00},
        {0x4205, 0x00},
        {0x4206, 0x00},
        {0x4207, 0x00},
        {0x4208, 0x00},
        {0x4209, 0x00},
        {0x420A, 0x00},
        {0x420B, 0x00},
        {0x420C, 0x00},
        {0x420D, 0x00}};
    uint32_t b;
    unsigned i;

    op_sep(P_M | P_X);
    b = (uint32_t)cpu.db << 16;
    for (i = 0; i < sizeof writes / sizeof writes[0]; i++) {
        uint16_t reg = writes[i].reg;

        /* STA abs 24; 40 after the LDA/LDX/LDY # that loads its value (the
           first write has SEP and LDA # ahead). */
        cyc(i == 0 || reg == 0x2101 || reg == 0x2115 || (reg == 0x211B && !writes[i].v)
            || reg == 0x2130 || reg == 0x2132 || reg == 0x4201 ? 40 : 24);
        wr8(b | reg, writes[i].v);
    }
    cyc(44);
    cpu.a = (uint16_t)(cpu.a & 0xFF00);
    cpu.x = 0x00FF;
    cpu.y = 0x0030;
    cpu.p = (uint8_t)((cpu.p & ~P_Z) | P_N);
}

/* LDA # (8-bit). */
static void boot_lda8(uint8_t v)
{
    cyc(40);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, v);
}

/* LDX # / LDY # (16-bit). */
static void boot_ldx(uint16_t v)
{
    cyc(22);
    cpu.x = v;
    cpu.p = nz16(cpu.p, v);
}

static void boot_ldy(uint16_t v)
{
    cpu.y = v;
    cpu.p = nz16(cpu.p, v);
}

/* PHK, PLB: the data bank $03. */
static void boot_bank03(void)
{
    cpu.db = 0x03;
    cpu.p = nz8(cpu.p, 0x03);
}

/* REP #$10 (or as is), LDX #$02FF, TXS. */
static void boot_stack(void)
{
    op_rep(P_X);
    cyc(76);
    boot_ldx(0x02FF);
    cpu.s = 0x02FF;
}

/* Work RAM and Super FX RAM cleared: three fills (A = bank, X = start,
   Y = count) by `fill`, then $70:0000 = 0 and MVN from each byte of
   $70:0000-$27FF to the next, P and the data bank kept around it. */
static void boot_clear(void (*fill)(void))
{
    static const struct { uint8_t bank; uint16_t x, y; } f[3] = {
        {0x00, 0x0300, 0x2100}, {0x7E, 0x2000, 0xE000}, {0x7F, 0x0000, 0xFFFF}};
    uint8_t db, p;
    unsigned i;

    for (i = 0; i < 3; i++) {
        cyc(90);
        boot_lda8(f[i].bank);
        boot_ldx(f[i].x);
        boot_ldy(f[i].y);
        fill();
    }
    db = cpu.db;
    p = cpu.p;
    op_rep(P_M | P_X);
    op_lda16_imm(0x0000);
    cyc(104);
    wr16(0x700000, 0x0000);
    op_lda16_imm(0x27FF);
    boot_ldx(0x0000);
    boot_ldy(0x0001);
    for (i = 0; i <= 0x27FF; i++)
        { cyc(36); wr8(0x700001u + i, rd8(0x700000u + i)); }
    cyc(88);
    cpu.a = 0xFFFF;
    cpu.x = 0x2800;
    cpu.y = 0x2801;
    cpu.db = 0x70;
    op_plp(p);
    cpu.db = db;
    cpu.p = nz8(cpu.p, db);
}

/* The screen blanked and HDMA off; interrupts off. */
static void boot_blank(uint32_t b)
{
    op_sep(P_M);
    cyc(96);
    wr8(b | HDMAEN, 0x00);
    boot_lda8(0x80);
    wr8(b | INIDISP, 0x80);
    cpu.p |= P_I;
}

/* One DMA of `count` bytes from bank:$8000 to work RAM bank:to. */
static void boot_dma(uint32_t b, uint16_t to, uint8_t wbank, uint8_t from_bank, uint16_t count)
{
    boot_ldx(0x8000);
    cyc(26);
    wr16(b | 0x4300, 0x8000);
    boot_ldy(to);
    cyc(48);
    wr16(b | WMADDL, to);
    boot_lda8(wbank);
    wr8(b | WMADDH, wbank);
    cyc(24);
    wr16(b | 0x4302, 0x8000);
    boot_lda8(from_bank);
    wr8(b | 0x4304, from_bank);
    boot_ldy(count);
    cyc(48);
    wr16(b | 0x4305, count);
    boot_lda8(0x01);
    wr8(b | MDMAEN, 0x01);
}

enum { BOOT_COLD, BOOT_START, BOOT_WARM, BOOT_RACE, BOOT_ATTRACT, BOOT_SOFT, BOOT_SOFT_REST };

/* The boot's code from one of its entries; none returns. */
static void boot_run(int from)
{
    uint32_t b = 0x030000;
    uint8_t old_e;

    switch (from) {
    case BOOT_START:
        goto start;
    case BOOT_WARM:
        goto warm;
    case BOOT_RACE:
        goto race;
    case BOOT_ATTRACT:
        goto attract;
    case BOOT_SOFT:
        goto soft;
    case BOOT_SOFT_REST:
        goto soft_rest;
    }
    op_sep(P_M | P_X);
    cpu.p |= P_I;
    old_e = cpu.e;
    cpu.e = 0;
    cpu.p = (uint8_t)((cpu.p & ~P_C) | (old_e ? P_C : 0));
    boot_bank03();
    op_rep(P_M | P_X);
    boot_ldx(0x02FF);
    cpu.s = 0x02FF;
    op_lda16_imm(0x0000);
    cpu.d = 0x0000;
    op_sep(P_M);
    boot_lda8(0x1C);
    cyc(130);
    wr8(b | 0x0D3F, 0x1C);
    (cyc(62), abi_HardwareRegsInit());
    cpu.p &= (uint8_t)~P_I;
    op_sep(P_M);
    op_rep(P_X);
    boot_ldx(0x02FE);
    cyc(2);
    do {
        cyc(58);
        wr8((uint16_t)(cpu.d + cpu.x), 0x00);
        cpu.x--;
        cpu.p = nz16(cpu.p, cpu.x);
    } while (!(cpu.x & 0x8000));
    cyc(60);
    op_rep(P_M);
    op_lda16_imm(0x0000);
    cpu.d = 0x0000;
    op_lda16_imm(0x02FF);
    cpu.s = 0x02FF;
    boot_bank03();
    op_sep(P_M);
    op_rep(P_X);
    boot_clear(abi_WramFillZero);
    abi_ApuUploadBank19();
    abi_SaveUnlocksCheck();
    op_rep(P_M | P_X);
    boot_ldx(0x000F);
    do {
        op_lda16(0x038D16 + cpu.x);
        wr16(0x000101 + cpu.x, cpu.a);
        cpu.x--;
        cpu.p = nz16(cpu.p, cpu.x);
    } while (!(cpu.x & 0x8000));
start:
    cyc(22);
    op_sep(P_M);
    op_rep(P_X);
    boot_dma(b, 0xA2D9, 0x7E, 0x02, 0x54A6);
    boot_dma(b, 0x0800, 0x7F, 0x0B, 0x2C00);
    op_sep(P_M);
    boot_lda8(0x01);
    cyc(22);
    wr8(b | 0x0374, 0x01);
    boot_lda8(0x01);
    wr8(b | 0x3039, 0x01);
    boot_lda8(0x20);
    wr8(b | 0x3037, 0x20);
    boot_lda8(0x0B);
    wr8(b | 0x3038, 0x0B);
    (cyc(62), abi_MenuScreen0Run());
    op_rep(P_M | P_X);
    op_lda16_imm(0x0003);
    cyc(62);
    wr16(0x001915, 0x0003);
    op_lda16_imm(0x0000);
    cyc(18);
    wr16(0x001917, 0x0000);
warm:
    boot_bank03();
    boot_stack();
    op_sep(P_M);
    cyc(36);
    wr8(b | 0x0E4E, 0x00);
    boot_lda8(0x02);
    wr8(b | 0x117A, 0x02);
    boot_lda8(0x01);
    wr8(b | 0x117B, 0x01);
    (cyc(62), abi_CourseMenuRun());
    abi_RaceModeSetup();
    abi_Set7EFFA6Sequence();
race:
    boot_bank03();
    boot_blank(b);
    boot_stack();
    op_sep(P_M);
    wr8(b | 0x0E4E, 0x00);
    /* JSL RaceLoop: its return address (03:8BE8) on the stack, which
       RaceLoop leaves by (LoopLeaveWhenEnded). */
    cyc(24);
    wr8((uint16_t)cpu.s, 0x03);
    cpu.s--;
    cyc(14);
    wr8((uint16_t)cpu.s, 0x8B);
    cpu.s--;
    wr8((uint16_t)cpu.s, 0xE8);
    cpu.s--;
    abi_RaceLoop();
    if (cpu.unwound)
        cpu.unwound = 0;
    else
        cpu.s = (uint16_t)(cpu.s + 3);
    b = (uint32_t)cpu.db << 16;
    boot_blank(b);
    abi_RaceVideoInit();
attract:
    boot_bank03();
    b = 0x030000;
    boot_blank(b);
    (cyc(78), boot_stack());
    op_sep(P_M | P_X);
    call_routine(0x02CF3E, 1);
    abi_WindowsInit();
    op_sep(P_M);
    op_rep(P_X);
    b = (uint32_t)cpu.db << 16;
    boot_lda8(0x01);
    cyc(122);
    wr8(b | 0x3039, 0x01);
    boot_lda8(0x01);
    cyc(8);
    wr8(0x7EFF65, 0x01);
    cpu.p |= P_I;
    op_sep(P_M);
    op_rep(P_X);
    cyc(98);
    call_routine(0x02CF45, 1);
    abi_CourseDataPointer();
    (cyc(62), abi_CourseScriptRun());
    (cyc(62), abi_ObjectPlaceAll());
    b = (uint32_t)cpu.db << 16;
    boot_lda8(0x00);
    cyc(8);
    wr8(0x70023A, 0x00);
    cyc(24);
    wr8(b | 0x1A32, 0x00);
    cyc(24);
    wr8(b | 0x05E9, 0x00);
    boot_lda8(0x01);
    wr8(b | 0x0713, 0x01);
    op_sep(P_M);
    op_rep(P_X);
    cyc(62);
    boot_lda8(0x04);
    boot_ldx(0x8800);
    abi_GsuRun();
    op_sep(P_M);
    b = (uint32_t)cpu.db << 16;
    boot_lda8(0x01);
    cyc(94);
    wr8(b | 0x0357, 0x01);
    cyc(32);
    op_lda8(TIMEUP);
    cpu.p &= (uint8_t)~P_I;
    for (;;) {
        cyc(76);
        abi_SpriteSlot1Countdown();
        op_sep(P_M);
        b = (uint32_t)cpu.db << 16;
        cyc(114);
        op_lda8(b | 0x0D62);
        if ((uint8_t)cpu.a)
            abi_RaceFrameRunTwoPlayers();
        else
            { cyc(84); abi_RaceFrameRun(); }
        op_sep(P_M);
        b = (uint32_t)cpu.db << 16;
        cyc(104);
        op_lda8(b | 0x05E9);
        cpu.p = (uint8_t)((nz8(cpu.p, (uint8_t)(cpu.a - 0x1E)) & ~P_C) | ((uint8_t)cpu.a >= 0x1E ? P_C : (cyc(24), 0)));
        if (cpu.p & P_C)
            wr8(b | 0x0713, 0x00);
    }
soft:
    op_rep(P_M);
    op_lda16_imm(0xFFFF);
    cyc(10);
    wr16(b | 0x10DC, 0xFFFF);
    cyc(24);
    wr16(b | 0x10DE, 0x0000);
    (cyc(62), abi_DemoRunThird());
soft_rest:
    op_sep(P_M);
    b = (uint32_t)cpu.db << 16;
    boot_lda8(0x06);
    cyc(22);
    wr8(b | APUIO1, 0x06);
    boot_lda8(0x80);
    wr8(b | INIDISP, 0x80);
    cyc(24);
    wr8(b | NMITIMEN, 0x00);
    cyc(24);
    wr8(b | MDMAEN, 0x00);
    cyc(24);
    wr8(b | HDMAEN, 0x00);
    op_sep(P_M);
    op_rep(P_X);
    boot_clear(abi_WramFillZeroLong);
    abi_SoftResetApuLong();
    b = (uint32_t)cpu.db << 16;
    goto start;
}

/*
 * @routine 03:8AA9 BootColdStart
 * WHAT IT DOES: The game's start after the reset vector: native mode, the
 * stack at $02FF and the direct page at 0, $0D3F = $1C, the registers reset
 * (HardwareRegsInit), $0000-$02FE cleared, work RAM $0300-$20FF,
 * $7E:2000-$FFFF and $7F cleared and Super FX RAM $0000-$27FF, the sound
 * program uploaded (ApuUploadBank19), the save checked (SaveUnlocksCheck)
 * and the interrupt stubs copied to $0101; then BootStart.
 * HOW: As the original: STZ through the direct page, WramFillZero, MVN from
 * each byte to the next; the stubs from $03:8D16. Never returns.
 * Entry: from the reset vector. Exit: never returns.
 */
void abi_BootColdStart(void)
{
    boot_run(BOOT_COLD);
}

/*
 * @routine 03:8B34 BootStart
 * WHAT IT DOES: The start from a cleared machine (the soft reset's jump):
 * copies the code the game runs from work RAM ($02:8000 to $7E:A2D9, $54A6
 * bytes; $0B:8000 to $7F:0800, $2C00 bytes, by DMA), sets the Super FX
 * (clock, $0374 = 1, config $20, screen base $0B), runs the title
 * (MenuScreen0Run), sets $1915 = 3 and $1917 = 0; then BootWarm.
 * HOW: DMA channel 0 to WMDATA; 8-bit A, 16-bit X and Y. Never returns.
 * Entry: data bank $03. Exit: never returns.
 */
void abi_BootStart(void)
{
    boot_run(BOOT_START);
}

/*
 * @routine 03:8BAA BootWarm
 * WHAT IT DOES: The game's loop of a session: the stack reset, sound on, the
 * course menu (CourseMenuRun) and the race's setup (RaceModeSetup,
 * Set7EFFA6Sequence), the race (RaceLoop) with the screen blanked before
 * and after, the video reset (RaceVideoInit), then the attract race: 02:CF3E,
 * the windows, the Super FX clock, the palette flag, 02:CF45, the course
 * (CourseDataPointer, CourseScriptRun, ObjectPlaceAll), program $04:8800,
 * and race frames for ever (RaceFrameRun, or RaceFrameRunTwoPlayers with
 * $0D62 set, after SpriteSlot1Countdown), $0713 cleared once $05E9 reaches
 * $1E.
 * HOW: Data bank $03; the stack reset to $02FF three times; the JSL to
 * RaceLoop leaves its return address on the stack for RaceLoop's way out.
 * Entry: any. Exit: never returns.
 */
void abi_BootWarm(void)
{
    boot_run(BOOT_WARM);
}

/*
 * @routine 03:8BCD BootWarmRace
 * WHAT IT DOES: BootWarm from its race (RaceRestart's way back in, the race
 * run again with the menus' choices kept): the screen blanked and HDMA
 * off, interrupts off, the stack reset to $02FF, sound on, RaceLoop; then
 * on as BootWarm (the video reset and the attract race, for ever).
 * HOW: As BootWarm from its .race label. Never returns.
 * Entry: any. Exit: never returns.
 */
void abi_BootWarmRace(void)
{
    boot_run(BOOT_RACE);
}

/*
 * @routine 03:8BF8 BootWarmAttract
 * WHAT IT DOES: BootWarm from its attract race (RaceEndToAttract's way
 * back in): the screen blanked, HDMA and interrupts off, the stack reset,
 * the attract race set up and run for ever, as BootWarm's end.
 * HOW: As BootWarm from its .attract label. Never returns.
 * Entry: any. Exit: never returns.
 */
void abi_BootWarmAttract(void)
{
    boot_run(BOOT_ATTRACT);
}

/*
 * @routine 03:8C86 BootSoftRestart
 * WHAT IT DOES: Restarts the game from a running one: $10DC = $FFFF and
 * $10DE = 0, 03:865E, command $06 to APU port 1, the screen blanked, the
 * NMI, DMA and HDMA off, work RAM and Super FX RAM cleared as at the start,
 * the sound restarted (SoftResetApuLong); then BootStart. Nothing reaches it:
 * the attract race's loop before it never ends.
 * HOW: As BootColdStart's clears; a jump to BootStart. Never returns.
 * Entry: data bank $03. Exit: never returns.
 */
void abi_BootSoftRestart(void)
{
    boot_run(BOOT_SOFT);
}

/*
 * @routine 03:8C95 BootSoftRestartRest
 * WHAT IT DOES: BootSoftRestart after its demo run (DemoRunThird, which never
 * returns, so nothing reaches this): command $06 to APU port 1, the screen
 * blanked, the NMI, DMA and HDMA off, work RAM and Super FX RAM cleared,
 * the sound restarted (SoftResetApuLong), then BootStart.
 * HOW: As BootSoftRestart from the call's return. Never returns.
 * Entry: data bank $03. Exit: never returns.
 */
void abi_BootSoftRestartRest(void)
{
    boot_run(BOOT_SOFT_REST);
}
