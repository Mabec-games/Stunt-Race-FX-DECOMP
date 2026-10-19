/*
 * reset.c: the soft reset (L, R, START and SELECT held) and the jumps that
 * restart the game.
 */
#include "machine.h"
#include "cpu_ops.h"
#include "hw.h"

void abi_SoundPort1QueuePush(void);
void abi_SoftResetApu(void);
void abi_WramFillZeroLong(void);

#define RESET_STATE 0x0D51u     /* $0D51: a reset under way when not 0 */
#define RESET_TICK 0x0D52u      /* set by each vertical blank */
#define RESET_FRAMES 0x0D5Fu    /* frames left to wait */

/* LDA # (8-bit). */
static void reset_lda8(uint8_t v)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, v);
}

/* WramFillZeroLong on a work RAM range: A = bank, X = start, Y = count. */
static void reset_fill(uint8_t bank, uint16_t x, uint16_t y)
{
    reset_lda8(bank);
    cpu.x = x;
    cpu.p = nz16(cpu.p, x);
    cpu.y = y;
    cpu.p = nz16(cpu.p, y);
    abi_WramFillZeroLong();
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
void abi_SoftResetCheck(void)
{
    uint8_t p0 = cpu.p, db;
    uint32_t b = (uint32_t)cpu.db << 16;
    uint8_t v;
    uint16_t i;

    op_sep(P_M);
    op_lda8(b | RESET_STATE);
    if (!(uint8_t)cpu.a) {
        op_rep(P_M);
        op_lda16(b | 0x0309);
        op_and16(0x3030);
        op_cmp16(0x3030);
        if (!(cpu.p & P_Z)) {
            op_plp(p0);
            return;
        }
        op_lda16(b | 0x0309);
        op_and16(0xCFCF);
        if (cpu.a) {
            op_plp(p0);
            return;
        }
        op_sep(P_M);
        v = (uint8_t)(rd8(b | RESET_STATE) - 1);
        wr8(b | RESET_STATE, v);
        cpu.p = nz8(cpu.p, v);
        reset_lda8(0x04);
        abi_SoundPort1QueuePush();
        reset_lda8(0x01);
        wr8(b | 0x0E3A, 0x01);
        reset_lda8(0x70);
        wr8(b | RESET_FRAMES, 0x70);
    }
    do {
        do
            op_lda8(b | RESET_TICK);
        while (!(uint8_t)cpu.a);
        wr8(b | RESET_TICK, 0x00);
        v = (uint8_t)(rd8(b | RESET_FRAMES) - 1);
        wr8(b | RESET_FRAMES, v);
        cpu.p = nz8(cpu.p, v);
    } while (v);
    reset_lda8(0x80);
    wr8(b | INIDISP, 0x80);
    wr8(b | NMITIMEN, 0x00);
    wr8(b | MDMAEN, 0x00);
    wr8(b | HDMAEN, 0x00);
    wr8(b | RESET_STATE, 0x00);
    abi_SoftResetApu();
    op_sep(P_M);
    wr8(b | 0x0D2B, 0x00);
    wr8(b | 0x0D62, 0x00);
    op_sep(P_M);
    op_rep(P_X);
    reset_fill(0x00, 0x0300, 0x2100);
    reset_fill(0x7E, 0x2000, 0xE000);
    reset_fill(0x7F, 0x0000, 0xFFFF);
    db = cpu.db;
    v = cpu.p;
    op_rep(P_M | P_X);
    op_lda16_imm(0x0000);
    wr16(0x700000, 0x0000);
    op_lda16_imm(0x27FF);
    cpu.x = 0x0000;
    cpu.p = nz16(cpu.p, 0);
    cpu.y = 0x0001;
    cpu.p = nz16(cpu.p, 1);
    for (i = 0; i <= 0x27FF; i++)
        wr8(0x700001u + i, rd8(0x700000u + i));
    cpu.a = 0xFFFF;
    cpu.x = 0x2800;
    cpu.y = 0x2801;
    cpu.db = 0x70;
    op_plp(v);
    cpu.db = db;
    cpu.p = nz8(cpu.p, db);
    jump_routine(0x038B34);
}

/*
 * @routine 07:AACE SoftResetLong
 * WHAT IT DOES: SoftResetCheck for callers in other banks.
 * HOW: P and the data bank saved; the data bank set to its own ($07).
 * Entry: any widths. Exit: as SoftResetCheck, then P and the data bank put
 * back.
 */
void abi_SoftResetLong(void)
{
    uint8_t p0 = cpu.p, db = cpu.db;

    op_sep(P_M);
    cpu.db = 0x07;
    cpu.p = nz8(cpu.p, 0x07);
    abi_SoftResetCheck();
    op_sep(P_M);
    cpu.db = db;
    cpu.p = nz8(cpu.p, db);
    op_plp(p0);
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
    cpu.p = (uint8_t)((cpu.p & ~P_C) | (old_e ? P_C : 0));
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
    uint16_t lo;

    op_plp(rd8((uint16_t)(cpu.s + 1)));
    cpu.s = (uint16_t)(cpu.s + 1);
    lo = rd16((uint16_t)(cpu.s + 1));
    cpu.pc = (uint32_t)rd8((uint16_t)(cpu.s + 3)) << 16 | lo;
    cpu.s = (uint16_t)(cpu.s + 3);
    cpu.unwound = 1;
}

void abi_ApuUpload(void);

/*
 * @routine 07:AB8C SoftResetApu
 * WHAT IT DOES: Restarts the sound for the soft reset: a pause, command $06
 * to APU port 1, another pause, the sound program uploaded again (ApuUpload
 * of block $19 at $C27C) and a last pause.
 * HOW: Delay loops of $8000 counts of X and $4000 of A; P saved around the
 * upload and around the whole.
 * Entry: any widths. Exit: P unchanged; A = 0; X = 0; Y as ApuUpload leaves
 * it.
 */
void abi_SoftResetApu(void)
{
    uint8_t p0 = cpu.p, p1;

    op_sep(P_M);
    op_rep(P_X);
    cpu.x = 0x0000;
    cpu.p = (uint8_t)((cpu.p & ~P_N) | P_Z);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x06);
    cpu.p = nz8(cpu.p, 0x06);
    wr8(APUIO1, 0x06);
    cpu.x = 0x0000;
    cpu.p = (uint8_t)((cpu.p & ~P_N) | P_Z);
    p1 = cpu.p;
    op_rep(P_M | P_X);
    cpu.a = 0x0019;
    cpu.p = nz16(cpu.p, cpu.a);
    cpu.y = 0xC27C;
    cpu.p = nz16(cpu.p, cpu.y);
    abi_ApuUpload();
    op_rep(P_M);
    cpu.a = 0x0000;
    cpu.p = (uint8_t)((cpu.p & ~P_N) | P_Z);
    op_plp(p1);
    op_plp(p0);
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
}
