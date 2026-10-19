/*
 * frame_extras.c: two pieces of RAM code no play runs: a frame-step pause
 * for the race (two race frames, then hold until START is pressed and let
 * go) and a flush of the VRAM DMA queue and the pending byte writes.
 * unit_frame_extras calls them.
 */
#include "machine.h"
#include "cpu_ops.h"

void abi_RaceFrameRun(void);
void abi_WaitScanline(void);
void abi_VramDmaQueueFlush(void);
void abi_ByteWritesIfPending(void);

/* Wait for scanline $64 (WaitScanline) and read JOY1H, until START (bit
   4) is as `held` says. */
static void wait_start(uint32_t b, int held)
{
    uint8_t j;

    do {
        cyc(72);
        wr8(b | 0x06E3, 0x64);
        abi_WaitScanline();
        cpu.p = (uint8_t)((cpu.p | P_M) & ~P_X);
        cyc(46);
        j = rd8(b | 0x4219);
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | j);
    } while (((j & 0x10) != 0) != held);
}

/*
 * @routine 7E:F2CD RaceFrameStepPause
 * WHAT IT DOES: Unless $1168 is set or $08FB is not $FF: sets $1168 to 2,
 * runs two race frames (RaceFrameRun), waits for START to be let go, then
 * pressed, then let go again, clears the held buttons at $0309-$030C and
 * sets $1168 to 1.
 * HOW: Each wait polls JOY1H once a frame at scanline $64 (WaitScanline);
 * everything through the data bank, so it needs one where the joypad
 * registers are. P saved and restored.
 * Entry: any widths. Exit: P unchanged; A low byte = 1 when it paused,
 * else the byte it tested; X, Y as RaceFrameRun leaves them.
 */
void RaceFrameStepPause(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint8_t p = cpu.p, v;

    cpu.p = (uint8_t)((cpu.p | P_M) & ~P_X);
    cyc(90);
    v = rd8(b | 0x1168);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    if (!v) {
        cyc(40);
        v = rd8(b | 0x08FB);
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
        if (v == 0xFF) {
            cyc(72);
            wr8(b | 0x1168, 0x02);
            cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x02);
            (cyc(62), abi_RaceFrameRun());
            (cyc(120), abi_RaceFrameRun());
            cpu.p |= P_M;
            cyc(48);
            wait_start(b, 0);
            wait_start(b, 1);
            wait_start(b, 0);
            cyc(56);
            wr8(b | 0x030A, 0x00);
            cyc(24);
            wr8(b | 0x030C, 0x00);
            cyc(24);
            wr8(b | 0x0309, 0x00);
            cyc(24);
            wr8(b | 0x030B, 0x00);
            cyc(40);
            wr8(b | 0x1168, 0x01);
            cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x01);
        } else { cyc(38); }
    } else { cyc(22); }
    cyc(28);
    op_plp(p);
}

void abi_RaceFrameStepPause(void)
{
    RaceFrameStepPause();
}

/*
 * @routine 7F:0D20 FrameQueuesFlush
 * WHAT IT DOES: Sends the VRAM DMA queue (VramDmaQueueFlush) and the
 * pending byte writes (ByteWritesIfPending).
 * HOW: 16-bit registers; P saved and restored.
 * Entry: any widths. Exit: P unchanged; A, X, Y as ByteWritesIfPending
 * leaves them.
 */
void FrameQueuesFlush(void)
{
    uint8_t p = cpu.p;

    cpu.p &= (uint8_t)~(P_M | P_X);
    (cyc(90), abi_VramDmaQueueFlush());
    (cyc(46), abi_ByteWritesIfPending());
    op_plp(p);
}

void abi_FrameQueuesFlush(void)
{
    FrameQueuesFlush();
}
