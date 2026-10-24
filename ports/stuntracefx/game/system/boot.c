/*
 * boot.c: the game's start: the registers reset, the reset vector's start,
 * the start after a soft reset and the session's loop.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "hw.h"
#include "carried.h"

#include "audio_calls.h"
uint16_t CourseDataPointer(uint8_t *bank, uint8_t *p);
#include "video_calls.h"
#include "wram_names.h"
#include "wram_struct.h"
void abi_AttractCourseSetup(void);
void abi_KeepRegisters(void);

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

void BootColdStart(Carried *cr);
void BootSoftRestart(Carried *cr);
void BootSoftRestartRest(Carried *cr);
void BootStart(Carried *cr);
void BootWarm(Carried *cr);
void BootWarmAttract(Carried *cr);
void BootWarmRace(Carried *cr);
void HardwareRegsInit(Carried *cr);
static void boot_bank03(Carried *cr);
static void boot_blank(Carried *cr, uint32_t b);
static void boot_clear(Carried *cr, void (*fill)(Carried *), uint32_t first, uint32_t rest, uint32_t after);
static void boot_dma(Carried *cr, uint32_t b, uint16_t to, uint8_t wbank, uint8_t from_bank, uint16_t count);
static void boot_lda8(Carried *cr, uint8_t v);
static void boot_ldx(Carried *cr, uint16_t v);
static void boot_ldy(Carried *cr, uint16_t v);
static void boot_run(Carried *cr, int from);
static void boot_stack(Carried *cr);
static void w_ApuUploadBank19(Carried *cr);
static void w_CourseDataPointer(Carried *cr);
void KeepRegisters_cr(Carried *cr);
static void w_KeepRegisters(Carried *cr);
static void w_SpriteSlot1Countdown(Carried *cr);
void AttractCourseSetup(Carried *cr);
void CourseMenuRun(Carried *cr);
void CourseScriptRun(Carried *cr);
void DemoRunThird(Carried *cr);
void GsuRun_cr(Carried *cr);
void MenuScreen0Run_cr(Carried *cr);
void ObjectPlaceAll_cr(Carried *cr);
void RaceFrameRun(Carried *cr);
void RaceFrameRunTwoPlayers(Carried *cr);
void RaceLoop(Carried *cr);
void RaceModeSetup(Carried *cr);
void RaceVideoInit_cr(Carried *cr);
void SaveUnlocksCheck_cr(Carried *cr);
void Set7EFFA6Sequence_cr(Carried *cr);
void SoftResetApuLong(Carried *cr);
void WindowsInit_cr(Carried *cr);
void WramFillZeroLong(Carried *cr);
void WramFillZero_cr(Carried *cr);

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
void HardwareRegsInit(Carried *cr)
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

    r_sep(cr, P_M | P_X);
    b = (uint32_t)cr->db << 16;
    for (i = 0; i < sizeof writes / sizeof writes[0]; i++) {
        uint16_t reg = writes[i].reg;

        /* STA abs 24; 40 after the LDA/LDX/LDY # that loads its value (the
           first write has SEP and LDA # ahead). */
        cyc(i == 0 || reg == 0x2101 || reg == 0x2115 || (reg == 0x211B && !writes[i].v)
            || reg == 0x2130 || reg == 0x2132 || reg == 0x4201 ? 40 : 24);
        if ((reg >= 0x4202 && reg <= 0x4206) || reg == 0x211B || reg == 0x211C)
            io_wr8(b | reg, writes[i].v);   /* the multiply units: as C */
        else
            reg_wr(b | reg, writes[i].v);
    }
    ppu_mul_factor = 0x0100;                 /* M7A: $00, then $01 */
    cyc(44);
    cr->a = (uint16_t)(cr->a & 0xFF00);
    cr->x = 0x00FF;
    cr->y = 0x0030;
    set_p(cr, (uint8_t)((cr->p & ~P_Z) | P_N));
}

void abi_HardwareRegsInit(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    HardwareRegsInit(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* LDA # (8-bit). */
static void boot_lda8(Carried *cr, uint8_t v)
{
    cyc(40);
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    set_p(cr, nz8(cr->p, v));
}

/* LDX # / LDY # (16-bit). */
static void boot_ldx(Carried *cr, uint16_t v)
{
    cyc(22);
    cr->x = v;
    set_p(cr, nz16(cr->p, v));
}

static void boot_ldy(Carried *cr, uint16_t v)
{
    cr->y = v;
    set_p(cr, nz16(cr->p, v));
}

/* PHK, PLB: the data bank $03. */
static void boot_bank03(Carried *cr)
{
    cr->db = 0x03;
    set_p(cr, nz8(cr->p, 0x03));
}

/* REP #$10 (or as is), LDX #$02FF, TXS. */
static void boot_stack(Carried *cr)
{
    r_rep(cr, P_X);
    cyc(76);
    boot_ldx(cr, 0x02FF);
    cpu.s = 0x02FF;
}

/* Work RAM and Super FX RAM cleared: three fills (A = bank, X = start,
   Y = count) by `fill`, then $70:0000 = 0 and MVN from each byte of
   $70:0000-$27FF to the next, P and the data bank kept around it. */
static void boot_clear(Carried *cr, void (*fill)(Carried *), uint32_t first, uint32_t rest, uint32_t after)
{
    static const struct { uint8_t bank; uint16_t x, y; } f[3] = {
        {0x00, 0x0300, 0x2100}, {0x7E, 0x2000, 0xE000}, {0x7F, 0x0000, 0xFFFF}};
    uint8_t db, p;
    unsigned i;

    for (i = 0; i < 3; i++) {
        cyc(i ? rest : first);
        boot_lda8(cr, f[i].bank);
        boot_ldx(cr, f[i].x);
        boot_ldy(cr, f[i].y);
        fill(cr);
    }
    db = cr->db;
    p = cr->p;
    r_rep(cr, P_M | P_X);
    r_lda16_imm(cr, 0x0000);
    cyc(after);
    wr16(0x700000, 0x0000);
    r_lda16_imm(cr, 0x27FF);
    boot_ldx(cr, 0x0000);
    boot_ldy(cr, 0x0001);
    for (i = 0; i <= 0x27FF; i++)
        { cyc(36); wr8(0x700001u + i, rd8(0x700000u + i)); }
    cyc(88);
    cr->a = 0xFFFF;
    cr->x = 0x2800;
    cr->y = 0x2801;
    cr->db = 0x70;
    r_plp(cr, p);
    cr->db = db;
    set_p(cr, nz8(cr->p, db));
}

/* The screen blanked and HDMA off; interrupts off. */
static void boot_blank(Carried *cr, uint32_t b)
{
    r_sep(cr, P_M);
    cyc(96);
    hdmaen_wr(b | HDMAEN, 0x00);
    boot_lda8(cr, 0x80);
    ppu_wr(b | INIDISP, 0x80);
    set_p(cr, (uint8_t)(cr->p | (P_I)));
}

/* One DMA of `count` bytes from bank:$8000 to work RAM bank:to. */
static void boot_dma(Carried *cr, uint32_t b, uint16_t to, uint8_t wbank, uint8_t from_bank, uint16_t count)
{
    boot_ldx(cr, 0x8000);
    cyc(26);
    dma_wr16(b | 0x4300, 0x8000);
    boot_ldy(cr, to);
    cyc(48);
    hw_wr16(b | WMADDL, to);
    boot_lda8(cr, wbank);
    hw_wr(b | WMADDH, wbank);
    cyc(24);
    dma_wr16(b | 0x4302, 0x8000);
    boot_lda8(cr, from_bank);
    dma_wr(b | 0x4304, from_bank);
    boot_ldy(cr, count);
    cyc(48);
    dma_wr16(b | 0x4305, count);
    boot_lda8(cr, 0x01);
    mdmaen_wr(b | MDMAEN, 0x01);
}

enum { BOOT_COLD, BOOT_START, BOOT_WARM, BOOT_RACE, BOOT_ATTRACT, BOOT_SOFT, BOOT_SOFT_REST };

/* The boot's code from one of its entries; none returns. */
static void boot_run(Carried *cr, int from)
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
    r_sep(cr, P_M | P_X);
    set_p(cr, (uint8_t)(cr->p | (P_I)));
    old_e = cpu_mode();
    cpu_mode_set(0);
    set_p(cr, (uint8_t)((cr->p & ~P_C) | (old_e ? P_C : 0)));
    boot_bank03(cr);
    r_rep(cr, P_M | P_X);
    boot_ldx(cr, 0x02FF);
    cpu.s = 0x02FF;
    r_lda16_imm(cr, 0x0000);
    cr->d = 0x0000;
    r_sep(cr, P_M);
    boot_lda8(cr, 0x1C);
    cyc(130);
    WR8(wram_0D3F, 0x1C);
    (cyc(62), HardwareRegsInit(cr));
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_I)));
    r_sep(cr, P_M);
    r_rep(cr, P_X);
    boot_ldx(cr, 0x02FE);
    cyc(2);
    do {
        cyc(58);
        wr8((uint16_t)(cr->d + cr->x), 0x00);
        cr->x--;
        set_p(cr, nz16(cr->p, cr->x));
    } while (!(cr->x & 0x8000));
    cyc(60);
    r_rep(cr, P_M);
    r_lda16_imm(cr, 0x0000);
    cr->d = 0x0000;
    r_lda16_imm(cr, 0x02FF);
    cpu.s = 0x02FF;
    boot_bank03(cr);
    r_sep(cr, P_M);
    r_rep(cr, P_X);
    boot_clear(cr, WramFillZero_cr, 90, 48, 62);
    w_ApuUploadBank19(cr);
    SaveUnlocksCheck_cr(cr);
    r_rep(cr, P_M | P_X);
    boot_ldx(cr, 0x000F);
    do {
        r_lda16(cr, 0x038D16 + cr->x);
        WR16X(wram_0101, cr->x, cr->a);
        cr->x--;
        set_p(cr, nz16(cr->p, cr->x));
    } while (!(cr->x & 0x8000));
start:
    cyc(22);
    r_sep(cr, P_M);
    r_rep(cr, P_X);
    boot_dma(cr, b, 0xA2D9, 0x7E, 0x02, 0x54A6);
    boot_dma(cr, b, 0x0800, 0x7F, 0x0B, 0x2C00);
    r_sep(cr, P_M);
    boot_lda8(cr, 0x01);
    cyc(22);
    WR8(gsu_scmr_shadow, 0x01);
    boot_lda8(cr, 0x01);
    hw_wr(b | 0x3039, 0x01);
    boot_lda8(cr, 0x20);
    hw_wr(b | 0x3037, 0x20);
    boot_lda8(cr, 0x0B);
    hw_wr(b | 0x3038, 0x0B);
    (cyc(62), MenuScreen0Run_cr(cr));
    r_rep(cr, P_M | P_X);
    r_lda16_imm(cr, 0x0003);
    cyc(18);
    WR16(wram_1915, 0x0003);
    r_lda16_imm(cr, 0x0000);
    cyc(18);
    WR16(wram_1917, 0x0000);
warm:
    boot_bank03(cr);
    boot_stack(cr);
    r_sep(cr, P_M);
    cyc(36);
    WR8(sound_muted, 0x00);
    boot_lda8(cr, 0x02);
    WR8(wram_117A, 0x02);
    boot_lda8(cr, 0x01);
    WR8(wram_117B, 0x01);
    (cyc(62), CourseMenuRun(cr));
    RaceModeSetup(cr);
    Set7EFFA6Sequence_cr(cr);
race:
    boot_bank03(cr);
    boot_blank(cr, b);
    boot_stack(cr);
    r_sep(cr, P_M);
    WR8(sound_muted, 0x00);
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
    RaceLoop(cr);
    if (cpu.unwound)
        cpu.unwound = 0;
    else
        cpu.s = (uint16_t)(cpu.s + 3);
    b = (uint32_t)cr->db << 16;
    boot_blank(cr, b);
    RaceVideoInit_cr(cr);
attract:
    boot_bank03(cr);
    b = 0x030000;
    boot_blank(cr, b);
    (cyc(78), boot_stack(cr));
    r_sep(cr, P_M | P_X);
    w_KeepRegisters(cr);
    WindowsInit_cr(cr);
    r_sep(cr, P_M);
    r_rep(cr, P_X);
    b = (uint32_t)cr->db << 16;
    boot_lda8(cr, 0x01);
    cyc(22);
    hw_wr(b | 0x3039, 0x01);
    boot_lda8(cr, 0x01);
    cyc(8);
    WR8(palette_saved, 0x01);
    set_p(cr, (uint8_t)(cr->p | (P_I)));
    r_sep(cr, P_M);
    r_rep(cr, P_X);
    cyc(98);
    AttractCourseSetup(cr);
    (cyc(62), w_CourseDataPointer(cr));
    (cyc(62), CourseScriptRun(cr));
    (cyc(62), ObjectPlaceAll_cr(cr));
    b = (uint32_t)cr->db << 16;
    boot_lda8(cr, 0x00);
    cyc(8);
    wr8(0x70023A, 0x00);
    cyc(24);
    WR8(wram_1A32, 0x00);
    cyc(24);
    WR8(wram_05E9, 0x00);
    boot_lda8(cr, 0x01);
    WR8(wram_0713, 0x01);
    r_sep(cr, P_M);
    r_rep(cr, P_X);
    cyc(62);
    boot_lda8(cr, 0x04);
    boot_ldx(cr, 0x8800);
    GsuRun_cr(cr);
    r_sep(cr, P_M);
    b = (uint32_t)cr->db << 16;
    boot_lda8(cr, 0x01);
    cyc(22);
    WR8(wram_0357, 0x01);
    cyc(32);
    r_lda8(cr, TIMEUP);
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_I)));
    for (;;) {
        cyc(76);
        w_SpriteSlot1Countdown(cr);
        r_sep(cr, P_M);
        b = (uint32_t)cr->db << 16;
        cyc(46);
        r_lda8(cr, WA(wram_0D62));
        if ((uint8_t)cr->a)
            RaceFrameRunTwoPlayers(cr);
        else
            { cyc(84); RaceFrameRun(cr); }
        r_sep(cr, P_M);
        b = (uint32_t)cr->db << 16;
        cyc(46);
        r_lda8(cr, WA(wram_05E9));
        set_p(cr, (uint8_t)((nz8(cr->p, (uint8_t)(cr->a - 0x1E)) & ~P_C) | ((uint8_t)cr->a >= 0x1E ? P_C : (cyc(24), 0))));
        if (cr->p & P_C)
            WR8(wram_0713, 0x00);
    }
soft:
    r_rep(cr, P_M);
    r_lda16_imm(cr, 0xFFFF);
    cyc(10);
    WR16(wram_10DC, 0xFFFF);
    cyc(24);
    WR16(wram_10DE, 0x0000);
    (cyc(62), DemoRunThird(cr));
soft_rest:
    r_sep(cr, P_M);
    b = (uint32_t)cr->db << 16;
    boot_lda8(cr, 0x06);
    cyc(22);
    hw_wr(b | APUIO1, 0x06);
    boot_lda8(cr, 0x80);
    ppu_wr(b | INIDISP, 0x80);
    cyc(24);
    hw_wr(b | NMITIMEN, 0x00);
    cyc(24);
    mdmaen_wr(b | MDMAEN, 0x00);
    cyc(24);
    hdmaen_wr(b | HDMAEN, 0x00);
    r_sep(cr, P_M);
    r_rep(cr, P_X);
    boot_clear(cr, WramFillZeroLong, 86, 64, 62);
    SoftResetApuLong(cr);
    b = (uint32_t)cr->db << 16;
    cyc(24); /* JMP */
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
void BootColdStart(Carried *cr)
{
    boot_run(cr, BOOT_COLD);
}

void abi_BootColdStart(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    BootColdStart(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
void BootStart(Carried *cr)
{
    boot_run(cr, BOOT_START);
}

void abi_BootStart(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    BootStart(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
void BootWarm(Carried *cr)
{
    boot_run(cr, BOOT_WARM);
}

void abi_BootWarm(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    BootWarm(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
void BootWarmRace(Carried *cr)
{
    boot_run(cr, BOOT_RACE);
}

void abi_BootWarmRace(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    BootWarmRace(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:8BF8 BootWarmAttract
 * WHAT IT DOES: BootWarm from its attract race (RaceEndToAttract's way
 * back in): the screen blanked, HDMA and interrupts off, the stack reset,
 * the attract race set up and run for ever, as BootWarm's end.
 * HOW: As BootWarm from its .attract label. Never returns.
 * Entry: any. Exit: never returns.
 */
void BootWarmAttract(Carried *cr)
{
    boot_run(cr, BOOT_ATTRACT);
}

void abi_BootWarmAttract(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    BootWarmAttract(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
void BootSoftRestart(Carried *cr)
{
    boot_run(cr, BOOT_SOFT);
}

void abi_BootSoftRestart(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    BootSoftRestart(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
void BootSoftRestartRest(Carried *cr)
{
    boot_run(cr, BOOT_SOFT_REST);
}

void abi_BootSoftRestartRest(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    BootSoftRestartRest(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* ApuUploadBank19 as its adapter leaves the registers. */
static void w_ApuUploadBank19(Carried *cr)
{
    uint16_t offset = ApuUploadBank19();

    cr->a = 0;
    cr->x = 0;
    cr->y = offset;
}

/* CourseDataPointer as its adapter leaves the registers. */
static void w_CourseDataPointer(Carried *cr)
{
    uint8_t bank, p = cr->p;
    uint16_t x = CourseDataPointer(&bank, &p);

    cr->x = x;
    cr->y = x;
    cr->a = (uint16_t)((x & 0xFF00) | bank);
    set_p(cr, (uint8_t)((nz16(p, x) & ~P_X) | P_M));
}

/* KeepRegisters on the carried registers. */
static void w_KeepRegisters(Carried *cr)
{
    KeepRegisters_cr(cr);
}

/* SpriteSlot1Countdown as its adapter leaves the registers. */
static void w_SpriteSlot1Countdown(Carried *cr)
{
    uint16_t x = cr->x;
    uint8_t a = SpriteSlot1Countdown(cr->db, &x);

    cr->a = (uint16_t)((cr->a & 0xFF00) | a);
    cr->x = (cr->p & P_X) ? (uint8_t)x : (cyc(72), x);
}
