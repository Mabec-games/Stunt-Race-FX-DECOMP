/*
 * reset.c: the soft reset (L, R, START and SELECT held) and the jumps that
 * restart the game.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "hw.h"
#include "audio_calls.h"
#include "carried.h"
#include "wram_names.h"

void abi_BootColdStart(void);
void abi_BootStart(void);
void abi_BootWarm(void);

void SoundPort1QueuePushW(uint8_t command, unsigned wide);
void WramFillZero(uint8_t bank, uint16_t addr, uint16_t count);
uint16_t SoftResetApu(void);

#define RESET_STATE 0x0D51u     /* $0D51: a reset under way when not 0 */
#define RESET_TICK 0x0D52u      /* set by each vertical blank */
#define RESET_FRAMES 0x0D5Fu    /* frames left to wait */

/* LDA #bank, LDX, LDY (the pre clocks), then WramFillZeroLong. */
static void reset_fill(uint8_t bank, uint16_t x, uint16_t y, uint32_t pre)
{
    cyc(pre + 40 + 46);
    WramFillZero(bank, x, y);
    cyc(44);                            /* RTL */
}

void InterruptReturn(Carried *cr);
void ResetVector(Carried *cr);
void SoftResetApu_cr(Carried *cr);
void SoftResetApuLong(Carried *cr);
void SoftResetCheck_cr(Carried *cr);
void SoftResetLong_cr(Carried *cr);
void SoftResetWarmBoot(Carried *cr);
int SoftResetCheck(Carried *cr, uint8_t bank, int x16, uint16_t *a);
int SoftResetLong(Carried *cr, int x16, uint16_t *a);
static void reset_jump(Carried *cr, uint8_t bank, int started);
void BootColdStart(Carried *cr);
void BootStart(Carried *cr);
void BootWarm(Carried *cr);

/* The jump to 03:8B34 with the registers the reset leaves: A = $FFFF,
   X = $2800, Y = $2801 (the MVN's), M 8-bit, X 16-bit, C set when this
   call started the reset (the CMP of the buttons), N, Z from the data
   bank. */
static void reset_jump(Carried *cr, uint8_t bank, int started)
{
    cr->a = 0xFFFF;
    cr->x = 0x2800;
    cr->y = 0x2801;
    cr->db = bank;
    set_p(cr, (uint8_t)((cr->p | P_M) & ~P_X));
    if (started)
        set_p(cr, (uint8_t)(cr->p | (P_C)));
    set_p(cr, nz8(cr->p, bank));
    BootStart(cr);
}

/*
 * @routine 07:AB82 SoftResetWarmBoot
 * WHAT IT DOES: Restarts the game from 03:8BAA. Nothing reaches it: it
 * follows SoftResetCheck's jump to 03:8B34.
 * HOW: A long jump.
 * Entry: any. Exit: never returns.
 */
void SoftResetWarmBoot(Carried *cr)
{
    cyc(32);
    BootWarm(cr);
}

void abi_SoftResetWarmBoot(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    SoftResetWarmBoot(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:AADC SoftResetCheck
 * WHAT IT DOES: The soft reset: when player 1 holds L, R, START and SELECT
 * and nothing else ($0309), with no reset under way ($0D51), starts one:
 * $0D51 down by one, sound $04 on port 1, the fade out ($0E3A = 1) and $70
 * frames to wait ($0D5F). Under way, it waits those frames ($0D52 set by
 * each vertical blank), then blanks the screen, stops the NMI, DMA and HDMA,
 * clears $0D51, resets the sound (07:AB8C), clears $0D2B and $0D62, clears
 * work RAM ($00:0300-$2100, $7E:2000-$E000, $7F:0000-$FFFF by 03:8CF2) and
 * Super FX RAM $70:0000-$27FF, and restarts the game (03:8B34).
 * HOW: 8-bit A for the flags, 16-bit for the buttons; through the data bank;
 * the Super FX RAM by MVN from each byte to the next.
 * Entry: any widths; DB = $07. Exit: P unchanged; A = the buttons masked
 * (no reset); the reset never returns.
 */
int SoftResetCheck(Carried *cr, uint8_t bank, int x16, uint16_t *a)
{
    uint32_t b = (uint32_t)bank << 16;
    uint8_t v;
    uint16_t i;
    int fresh = 0;

    cyc(68);
    if (!rd8(b | RESET_STATE)) {
        cyc(22 + 40);
        *a = rd16(b | W(joy1_held)) & 0x3030;
        cyc(2);
        if (*a != 0x3030)
            { cyc(120 + 42); return 0; }
        cyc(92);
        *a = rd16(b | W(joy1_held)) & 0xCFCF;
        cyc(2);
        if (*a)
            { cyc(96 + 42); return 0; }
        cyc(90);
        v = (uint8_t)(rd8(b | RESET_STATE) - 1);
        cyc(6);
        wr8(b | RESET_STATE, v);
        cyc(38 + 40);
        SoundPort1QueuePushW(0x04, x16 ? 16 : 0);
        cyc(40);
        wr8(b | W(fade_out), 0x01);
        cyc(40);
        wr8(b | RESET_FRAMES, 0x70);
        fresh = 1;
    }
    int started = fresh;
    do {
        do
            { cyc(fresh ? 24 : 46); fresh = 0; }
        while (!rd8(b | RESET_TICK));
        cyc(40);
        wr8(b | RESET_TICK, 0x00);
        cyc(24);
        v = (uint8_t)(rd8(b | RESET_FRAMES) - 1);
        cyc(6);
        wr8(b | RESET_FRAMES, v);
    } while (v);
    cyc(40 + 16);
    ppu_wr(b | INIDISP, 0x80);
    cyc(24);
    hw_wr(b | NMITIMEN, 0x00);
    cyc(24);
    mdmaen_wr(b | MDMAEN, 0x00);
    cyc(24);
    hdmaen_wr(b | HDMAEN, 0x00);
    cyc(24);
    wr8(b | RESET_STATE, 0x00);
    SoftResetApu();
    cyc(22 + 24);                       /* SEP */
    wr8(b | W(hud_mode), 0x00);
    cyc(24);
    wr8(b | W(wram_0D62), 0x00);
    cyc(22);                            /* REP #$10 */
    reset_fill(0x00, 0x0300, 0x2100, 108);
    reset_fill(0x7E, 0x2000, 0xE000, 86);
    reset_fill(0x7F, 0x0000, 0xFFFF, 86);
    cyc(22 + 38 + 62);
    wr16(0x700000, 0x0000);
    cyc(38 + 22);
    for (i = 0; i <= 0x27FF; i++)
        { cyc(36); wr8(0x700001u + i, rd8(0x700000u + i)); }
    cyc(58 + 42);
    reset_jump(cr, bank, started);
    return 1;
}

void SoftResetCheck_cr(Carried *cr)
{
    uint16_t a;

    if (!SoftResetCheck(cr, cr->db, !(cr->p & P_X), &a))
        cr->a = a;
}

void abi_SoftResetCheck(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    SoftResetCheck_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:AACE SoftResetLong
 * WHAT IT DOES: SoftResetCheck for callers in other banks.
 * HOW: P and the data bank saved; the data bank set to its own ($07).
 * Entry: any widths. Exit: as SoftResetCheck, then P and the data bank put
 * back.
 */
int SoftResetLong(Carried *cr, int x16, uint16_t *a)
{
    int r;

    cyc(162);
    r = SoftResetCheck(cr, 0x07, x16, a);
    cyc(42 + 80);
    return r;
}

void SoftResetLong_cr(Carried *cr)
{
    uint8_t p0 = cr->p, db = cr->db;
    uint16_t a;

    if (!SoftResetLong(cr, !(cr->p & P_X), &a))
        cr->a = a;
    cr->db = db;
    set_p(cr, p0);
    if (p0 & P_X) {
        cr->x &= 0x00FF;
        cr->y &= 0x00FF;
    }
}

void abi_SoftResetLong(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    SoftResetLong_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 00:FE88 ResetVector
 * WHAT IT DOES: Where the console starts the game (the reset vector): stops
 * interrupts, switches the CPU to native mode and decimal off, and jumps
 * to the start (03:8AA9).
 * HOW: SEI, CLC, XCE (the old emulation flag into C), CLD, a long jump.
 * Entry: the reset (emulation mode). Exit: never returns.
 */
void ResetVector(Carried *cr)
{
    uint8_t old_e = cpu_mode();

    set_p(cr, (uint8_t)(cr->p | (P_I)));
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_C)));
    cpu_mode_set(0);
    set_p(cr, (uint8_t)((cr->p & ~P_C) | (old_e ? P_C : (cyc(88), 0))));
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_D)));
    BootColdStart(cr);
}

void abi_ResetVector(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ResetVector(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 00:FE90 InterruptReturn
 * WHAT IT DOES: The handler of every interrupt the game does not use: the
 * COP, BRK and ABORT vectors (native and emulation) and the emulation
 * NMI and IRQ vectors all point here, and it returns at once. BRK runs
 * here in the game (ObjectSlideGateUpStep's width-hazard paths).
 * HOW: RTI.
 * Entry: native mode, the interrupt's frame on the stack (P, then PC and
 * PB). Exit: P, PC and PB as the interrupt pushed them; S 4 higher.
 */
void InterruptReturn(Carried *cr)
{
    /* P from the frame; the PC and PB above it are the return, taken as
       for any routine's. */
    uint8_t p;

    cyc(20);
    p = rd8((uint16_t)(cpu.s + 1));
    cyc(24);   /* the pulls of PC and PB */
    set_p(cr, p);
    if (p & P_X) {
        cr->x &= 0x00FF;
        cr->y &= 0x00FF;
    }
}

void abi_InterruptReturn(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    InterruptReturn(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:AB8C SoftResetApu
 * WHAT IT DOES: Restarts the sound for the soft reset: a pause, command $06
 * to APU port 1, another pause, the sound program uploaded again (ApuUpload
 * of block $19 at $C27C) and a last pause.
 * HOW: Delay loops of $8000 counts of X and $4000 of A; P saved around the
 * upload and around the whole.
 * Entry: any widths. Exit: P unchanged; A = 0; X = 0; Y as ApuUpload leaves
 * it.
 * From C: returns Y.
 */
uint16_t SoftResetApu(void)
{
    ApuUploadEnd end;

    cyc(22 + 162);
    delay_turns(0x8000);
    hw_wr(APUIO1, 0x06);
    cyc(156);
    delay_turns(0x8000);
    cyc(22);
    end = ApuUpload(0x19, 0xC27C);
    cyc(22 + 38);
    delay_turns(0x4000);
    cyc(42 + 42);                       /* PLP, PLP */
    return end.offset;
}

void SoftResetApu_cr(Carried *cr)
{
    cr->y = SoftResetApu();
    cr->a = 0x0000;
    cr->x = 0x0000;
    if (cr->p & P_X)
        cr->y &= 0x00FF;
}

void abi_SoftResetApu(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    SoftResetApu_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:AB88 SoftResetApuLong
 * WHAT IT DOES: SoftResetApu for callers in other banks. Nothing calls it.
 * HOW: A near call, then a long return.
 * Entry: any widths. Exit: as SoftResetApu.
 */
void SoftResetApuLong(Carried *cr)
{
    SoftResetApu_cr(cr);
    cyc(44); /* RTL */
}

void abi_SoftResetApuLong(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    SoftResetApuLong(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

