/*
 * reset.c: the soft reset (L, R, START and SELECT held) and the jumps that
 * restart the game.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "hw.h"
#include "audio_calls.h"

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

/* The jump to 03:8B34 with the registers the reset leaves: A = $FFFF,
   X = $2800, Y = $2801 (the MVN's), M 8-bit, X 16-bit, C set when this
   call started the reset (the CMP of the buttons), N, Z from the data
   bank. */
static void reset_jump(uint8_t bank, int started)
{
    cpu.a = 0xFFFF;
    cpu.x = 0x2800;
    cpu.y = 0x2801;
    cpu.db = bank;
    cpu.p = (uint8_t)((cpu.p | P_M) & ~P_X);
    if (started)
        cpu.p |= P_C;
    cpu.p = nz8(cpu.p, bank);
    jump_routine(0x038B34);
}

/*
 * @routine 07:AB82 SoftResetWarmBoot
 * WHAT IT DOES: Restarts the game from 03:8BAA. Nothing reaches it: it
 * follows SoftResetCheck's jump to 03:8B34.
 * HOW: A long jump.
 * Entry: any. Exit: never returns.
 */
void abi_SoftResetWarmBoot(void)
{
    cyc(32);
    jump_routine(0x038BAA);
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
int SoftResetCheck(uint8_t bank, int x16, uint16_t *a)
{
    uint32_t b = (uint32_t)bank << 16;
    uint8_t v;
    uint16_t i;
    int fresh = 0;

    cyc(68);
    if (!rd8(b | RESET_STATE)) {
        cyc(22 + 40);
        *a = rd16(b | 0x0309) & 0x3030;
        cyc(2);
        if (*a != 0x3030)
            { cyc(120 + 42); return 0; }
        cyc(92);
        *a = rd16(b | 0x0309) & 0xCFCF;
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
        wr8(b | 0x0E3A, 0x01);
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
    wr8(b | 0x0D2B, 0x00);
    cyc(24);
    wr8(b | 0x0D62, 0x00);
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
    reset_jump(bank, started);
    return 1;
}

void abi_SoftResetCheck(void)
{
    uint16_t a;

    if (!SoftResetCheck(cpu.db, !(cpu.p & P_X), &a))
        cpu.a = a;
}

/*
 * @routine 07:AACE SoftResetLong
 * WHAT IT DOES: SoftResetCheck for callers in other banks.
 * HOW: P and the data bank saved; the data bank set to its own ($07).
 * Entry: any widths. Exit: as SoftResetCheck, then P and the data bank put
 * back.
 */
int SoftResetLong(int x16, uint16_t *a)
{
    int r;

    cyc(162);
    r = SoftResetCheck(0x07, x16, a);
    cyc(42 + 80);
    return r;
}

void abi_SoftResetLong(void)
{
    uint8_t p0 = cpu.p, db = cpu.db;
    uint16_t a;

    if (!SoftResetLong(!(cpu.p & P_X), &a))
        cpu.a = a;
    cpu.db = db;
    cpu.p = p0;
    if (p0 & P_X) {
        cpu.x &= 0x00FF;
        cpu.y &= 0x00FF;
    }
}

/*
 * @routine 00:FE88 ResetVector
 * WHAT IT DOES: Where the console starts the game (the reset vector): stops
 * interrupts, switches the CPU to native mode and decimal off, and jumps
 * to the start (03:8AA9).
 * HOW: SEI, CLC, XCE (the old emulation flag into C), CLD, a long jump.
 * Entry: the reset (emulation mode). Exit: never returns.
 */
void abi_ResetVector(void)
{
    uint8_t old_e = cpu.e;

    cpu.p |= P_I;
    cpu.p &= (uint8_t)~P_C;
    cpu.e = 0;
    cpu.p = (uint8_t)((cpu.p & ~P_C) | (old_e ? P_C : (cyc(88), 0)));
    cpu.p &= (uint8_t)~P_D;
    jump_routine(0x038AA9);
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
void abi_InterruptReturn(void)
{
    /* P from the frame; the PC and PB above it are the return, taken as
       for any routine's. */
    uint8_t p;

    cyc(20);
    p = rd8((uint16_t)(cpu.s + 1));
    cyc(24);   /* the pulls of PC and PB */
    cpu.p = p;
    if (p & P_X) {
        cpu.x &= 0x00FF;
        cpu.y &= 0x00FF;
    }
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

void abi_SoftResetApu(void)
{
    cpu.y = SoftResetApu();
    cpu.a = 0x0000;
    cpu.x = 0x0000;
    if (cpu.p & P_X)
        cpu.y &= 0x00FF;
}

/*
 * @routine 07:AB88 SoftResetApuLong
 * WHAT IT DOES: SoftResetApu for callers in other banks. Nothing calls it.
 * HOW: A near call, then a long return.
 * Entry: any widths. Exit: as SoftResetApu.
 */
void abi_SoftResetApuLong(void)
{
    abi_SoftResetApu();
    cyc(44); /* RTL */
}
