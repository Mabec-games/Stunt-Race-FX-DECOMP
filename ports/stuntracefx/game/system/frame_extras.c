/*
 * frame_extras.c: two pieces of RAM code no play runs: a frame-step pause
 * for the race (two race frames, then hold until START is pressed and let
 * go) and a flush of the VRAM DMA queue and the pending byte writes.
 * unit_frame_extras calls them.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "carried.h"
#include "wram_names.h"


void abi_RaceFrameRun(void);
void abi_WaitScanline(void);
void abi_VramDmaQueueFlush(void);
void abi_ByteWritesIfPending(void);

void FrameQueuesFlush(Carried *cr);
void RaceFrameStepPause(Carried *cr);
static void wait_start(Carried *cr, uint32_t b, int held, int first);
void ByteWritesIfPending_cr(Carried *cr);
void RaceFrameRun(Carried *cr);
void VramDmaQueueFlush_cr(Carried *cr);
void WaitScanline_cr(Carried *cr);

/* Wait for scanline $64 (WaitScanline) and read JOY1H, until START (bit
   4) is as `held` says. */
static void wait_start(Carried *cr, uint32_t b, int held, int first)
{
    uint8_t j;

    do {
        cyc(first ? 62 : 72); /* the first: SEP, LDA #, STA */
        wr8(b | W(scanline_wait), 0x64);
        first = 0;
        cyc(62); /* JSL */
        WaitScanline_cr(cr);
        set_p(cr, (uint8_t)((cr->p | P_M) & ~P_X));
        cyc(68);
        j = hw_rd(b | 0x4219);
        cr->a = (uint16_t)((cr->a & 0xFF00) | j);
    } while (cyc_loop(6, ((j & 0x10) != 0) != held));
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
void RaceFrameStepPause(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;
    uint8_t p = cr->p, v;

    set_p(cr, (uint8_t)((cr->p | P_M) & ~P_X));
    cyc(90);
    v = rd8(b | W(frame_step));
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    if (!v) {
        cyc(40);
        v = rd8(b | W(slot1_count));
        cr->a = (uint16_t)((cr->a & 0xFF00) | v);
        if (v == 0xFF) {
            cyc(72);
            wr8(b | W(frame_step), 0x02);
            cr->a = (uint16_t)((cr->a & 0xFF00) | 0x02);
            (cyc(62), RaceFrameRun(cr));
            (cyc(62), RaceFrameRun(cr));
            set_p(cr, (uint8_t)(cr->p | (P_M)));
            wait_start(cr, b, 0, 1);
            wait_start(cr, b, 1, 0);
            wait_start(cr, b, 0, 0);
            cyc(56);
            wr8(b | W(wram_030A), 0x00);
            cyc(24);
            wr8(b | W(wram_030C), 0x00);
            cyc(24);
            wr8(b | W(joy1_held), 0x00);
            cyc(24);
            wr8(b | W(wram_030B), 0x00);
            cyc(40);
            wr8(b | W(frame_step), 0x01);
            cr->a = (uint16_t)((cr->a & 0xFF00) | 0x01);
        } else { cyc(38); }
    } else { cyc(22); }
    cyc(28);
    r_plp(cr, p);
}

void abi_RaceFrameStepPause(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RaceFrameStepPause(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 7F:0D20 FrameQueuesFlush
 * WHAT IT DOES: Sends the VRAM DMA queue (VramDmaQueueFlush) and the
 * pending byte writes (ByteWritesIfPending).
 * HOW: 16-bit registers; P saved and restored.
 * Entry: any widths. Exit: P unchanged; A, X, Y as ByteWritesIfPending
 * leaves them.
 */
void FrameQueuesFlush(Carried *cr)
{
    uint8_t p = cr->p;

    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~(P_M | P_X))));
    (cyc(90), VramDmaQueueFlush_cr(cr));
    (cyc(46), ByteWritesIfPending_cr(cr));
    cyc(30);
    r_plp(cr, p);
}

void abi_FrameQueuesFlush(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    FrameQueuesFlush(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

