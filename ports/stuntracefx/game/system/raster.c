/*
 * raster.c: waiting on the PPU's beam inside the raster interrupt.
 */
#include "machine.h"
#include "native.h"
#include "hw.h"
#include "draw_list.h"
#include "cpu_ops.h"
#include "carried.h"

#include "video_calls.h"
#include "wram_names.h"
uint8_t HudDrawTime(uint16_t *x, int *x_set);

/*
 * @routine 7E:A2FD WaitHBlank
 * WHAT IT DOES: Waits until the PPU is in horizontal blank, so the raster
 * interrupt can change PPU registers between two lines.
 * HOW: Reads HVBJOY until its H-blank bit (bit 6) is set.
 * Entry: M 8-bit, X 8-bit; runs from work RAM. Exit: A = the last HVBJOY
 * read; N from it; Z clear.
 */
uint8_t WaitHBlank(void)
{
    uint8_t v;

    cyc(0);
    do
        { cyc(24); v = hw_rd(HVBJOY); }
    while (cyc_loop(38, !(v & 0x40)));
    cyc(74);
    return v;
}

void RasterIrq(Carried *cr);
void RasterStage(Carried *cr);
void TimeMarkEnd(Carried *cr);
void TimeMarkStart(Carried *cr);
void WaitHBlank_cr(Carried *cr);
void WaitScanline_cr(Carried *cr);
void WindowTable7F(Carried *cr);
void WindowTable7FLong(Carried *cr);
static void copy8(Carried *cr, uint32_t to, uint32_t from);
static void frame_counts(Carried *cr, uint32_t b);
static void hblank_inline(Carried *cr);
static void inc8(Carried *cr, uint32_t addr);
static void irq_ack(Carried *cr);
static void ldx8(Carried *cr, uint8_t v);
static void put8(Carried *cr, uint32_t addr, uint8_t v);
static void split_bg2(Carried *cr, uint8_t v, uint8_t ylo, uint8_t yhi_pos, uint8_t yneg, uint8_t xlo);
static uint16_t time_stamp(Carried *cr);
static void window_colours(Carried *cr, uint32_t b, uint16_t first);
static void w_GradientPointersToBank7F(Carried *cr);
static void w_HudDrawTime(Carried *cr);
void AnimTilesQueueNext_cr(Carried *cr);
void Bg2ScrollFromTable_cr(Carried *cr);
void BobWindowsRun(Carried *cr);
void FadeStepFixed_cr(Carried *cr);
void FadeStep_cr(Carried *cr);
void FrameCountAndPortsEnd_cr(Carried *cr);
void FrameInputAndCountEnd_cr(Carried *cr);
void FrameSoundAndJoypads(Carried *cr);
void HudPanelASlide_cr(Carried *cr);
void HudPanelBSlide_cr(Carried *cr);
void HudPanelsStep_cr(Carried *cr);
void JoypadAccumulate_cr(Carried *cr);
void JoypadRead_cr(Carried *cr);
void NmiFrameUpload_cr(Carried *cr);
void OamDirectStage_cr(Carried *cr);
void OamDmaAll_cr(Carried *cr);
void PaletteKeptColoursStep_cr(Carried *cr);
void PalettesStep_cr(Carried *cr);
void RaceClockStep_cr(Carried *cr);
void RasterColoursUnless0D22_cr(Carried *cr);
void RasterColours_cr(Carried *cr);
void RasterPaletteUpload_cr(Carried *cr);
void SoundPortsSend_cr(Carried *cr);
void VblankUpdates_cr(Carried *cr);
void VramDmaFromGsuRam_cr(Carried *cr);
void WindowBandsAnimate_cr(Carried *cr);
void WindowHdmaBands_cr(Carried *cr);
void WindowHdmaNormal_cr(Carried *cr);
void WindowLayoutStepForScreen_cr(Carried *cr);
void WindowSlideCheck_cr(Carried *cr);
void WindowSlideOrOscillate(Carried *cr);
void WindowSplitAnimate_cr(Carried *cr);
void WindowSweepIfOn(Carried *cr);
void WindowsSetupSelect_cr(Carried *cr);

void WaitHBlank_cr(Carried *cr)
{
    uint8_t v = WaitHBlank();

    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    set_p(cr, (uint8_t)(nz8(cr->p, v) & ~P_Z));
}

void abi_WaitHBlank(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    WaitHBlank_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 02:D7AB WaitScanline
 * WHAT IT DOES: Waits until the low byte of the PPU's vertical counter
 * equals the byte at $06E3; clears $06E4 first.
 * HOW: Latches the counters (SLHV) and reads OPVCT twice (low, high) each
 * time round; the compare is 8-bit.
 * Entry: X 16-bit. Exit: A, X, P unchanged.
 */
void WaitScanline(uint8_t bank)
{
    uint32_t b = (uint32_t)bank << 16;
    uint8_t lo;

    cyc(150);
    wr8(0x7E0000 | W(scanline_wait_high), 0x00);
    do {
        cyc(46);
        hw_rd(SLHV);
        cyc(24);
        lo = hw_rd(OPVCT);
        cyc(44);
        hw_rd(OPVCT);
    } while (cyc_loop(22, lo != rd8(cyc_then(44, 0x7E0000 | W(scanline_wait)))));
    cyc(182);
}

void WaitScanline_cr(Carried *cr)
{
    WaitScanline(cr->db);
}

void abi_WaitScanline(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    WaitScanline_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 7E:A46B WindowTable7F
 * WHAT IT DOES: Builds the window band table at $7F:3410 from the four
 * scaled values at $7F:3432 (low bytes $3433, $3435, $3437, $3439 and
 * $3434, $3438): $7F:3402 = the width $3438 - $3434, $7F:3406 its middle;
 * the top line $3435 / 2 (and + 1), the left edges $3433 up by one a band
 * and the right edges $3437 down by one, mirrored, and the middle bands'
 * height ($3439 - $3435 - 6) / 2 (the subtraction keeping its borrow).
 * HOW: 8-bit long stores in the original's order.
 * Entry: any widths. Exit: M 8-bit; A low byte = the middle height; N, Z,
 * C from its LSR; V from the subtraction.
 */
static void put7f(uint32_t addr, uint8_t v)
{
    cyc(12);
    reg_wr(addr, v);
    cyc(20);
}

void WindowTable7F(Carried *cr)
{
    cyc(54);
    uint8_t a, b2;

    r_sep(cr, P_M);
    a = rd8(W(wram_7F3438));
    cyc(46);
    b2 = rd8(W(wram_7F3434));
    set_p(cr, (uint8_t)(cr->p | (P_C)));
    a = alu_sbc8(a, b2, &cr->p);
    cyc(4);
    put7f(W(window_width_7f), a);
    set_p(cr, (uint8_t)((cr->p & ~P_C) | (a & 1)));
    a = (uint8_t)(a >> 1);
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_C)));
    cyc(40);
    a = alu_adc8(a, rd8(W(wram_7F3434)), &cr->p);
    cyc(20);
    put7f(W(window_middle_7f), a);
    cyc(12);
    a = rd8(W(wram_7F3435));
    a = (uint8_t)(a >> 1);
    cyc(34);
    put7f(W(window_table_7f), a);
    a++;
    cyc(14);
    put7f(W(wram_7F3413), a);
    cyc(12);
    a = rd8(W(wram_7F3433));
    cyc(20);
    put7f(W(wram_7F3420), a);
    put7f(W(wram_7F3423), a);
    a++;
    cyc(14);
    put7f(W(wram_7F341D), a);
    put7f(W(wram_7F3426), a);
    a++;
    cyc(14);
    put7f(W(wram_7F341A), a);
    put7f(W(wram_7F3429), a);
    a++;
    cyc(14);
    put7f(W(wram_7F3417), a);
    put7f(W(wram_7F342C), a);
    cyc(12);
    a = rd8(W(wram_7F3437));
    cyc(20);
    put7f(W(wram_7F3421), a);
    put7f(W(wram_7F3424), a);
    a--;
    cyc(14);
    put7f(W(wram_7F341E), a);
    put7f(W(wram_7F3427), a);
    a--;
    cyc(14);
    put7f(W(wram_7F341B), a);
    put7f(W(wram_7F342A), a);
    a--;
    cyc(14);
    put7f(W(wram_7F3418), a);
    put7f(W(wram_7F342D), a);
    cyc(12);
    a = rd8(W(wram_7F3439));
    set_p(cr, (uint8_t)(cr->p | (P_C)));
    cyc(46);
    a = alu_sbc8(a, rd8(W(wram_7F3435)), &cr->p);
    a = alu_sbc8(a, 0x06, &cr->p);
    set_p(cr, (uint8_t)((cr->p & ~P_C) | (a & 1)));
    a = (uint8_t)(a >> 1);
    set_p(cr, nz8(cr->p, a));
    cyc(18);
    put7f(W(wram_7F341F), a);
    put7f(W(wram_7F3422), a);
    cr->a = (uint16_t)((cr->a & 0xFF00) | a);
    cyc(22);
}

void abi_WindowTable7F(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    WindowTable7F(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 7E:A467 WindowTable7FLong
 * WHAT IT DOES: WindowTable7F for a JSL.
 * HOW: JSR, RTL.
 * Entry: as WindowTable7F. Exit: as WindowTable7F.
 */
void WindowTable7FLong(Carried *cr)
{
    cyc(46);
    WindowTable7F(cr);
    cyc(44);
}

void abi_WindowTable7FLong(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    WindowTable7FLong(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void abi_JoypadRead(void);
void abi_OamDmaAll(void);
void abi_SoundPortsSend(void);
void abi_FadeStepFixed(void);
void abi_FadeStep(void);
void abi_JoypadAccumulate(void);
void abi_RaceClockStep(void);
void abi_HudDrawTime(void);
void abi_HudPanelsStep(void);
void abi_PalettesStep(void);
void abi_WindowHdmaNormal(void);
void abi_WindowLayoutStepForScreen(void);
void abi_WindowSlideOrOscillate(void);
void abi_WindowSweepIfOn(void);
void abi_WindowSlideCheck(void);
void abi_BobWindowsRun(void);
void abi_WindowBandsAnimate(void);
void abi_WindowSplitAnimate(void);
void abi_NmiFrameUpload(void);
void abi_OamDirectStage(void);
void abi_RasterColoursUnless0D22(void);
void abi_WindowsSetupSelect(void);
void abi_HudPanelASlide(void);
void abi_HudPanelBSlide(void);
void abi_PaletteKeptColoursStep(void);
void abi_Bg2ScrollFromTable(void);
void abi_RasterColours(void);
void abi_RasterPaletteUpload(void);
void abi_WindowHdmaBands(void);
void abi_GradientPointersToBank7F(void);
void abi_VblankUpdates(void);
void abi_FrameSoundAndJoypads(void);
void abi_FrameCountAndPortsEnd(void);
void abi_AnimTilesQueueNext(void);
void abi_FrameInputAndCountEnd(void);
void abi_VramDmaFromGsuRam(void);

/* LDA #v / STA addr with M 8-bit. */
static void put8(Carried *cr, uint32_t addr, uint8_t v)
{
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    set_p(cr, nz8(cr->p, v));
    cyc(40);
    reg_wr(addr, v);
}

/* LDA from / STA to with M 8-bit. */
static void copy8(Carried *cr, uint32_t to, uint32_t from)
{
    cyc(24);
    uint8_t v = rd8(from);

    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    set_p(cr, nz8(cr->p, v));
    cyc(24);
    reg_wr(to, v);
}

/* LDA TIMEUP: acknowledges the raster interrupt. */
static void irq_ack(Carried *cr)
{
    cyc(24);
    r_lda8(cr, TIMEUP);
}

/* INC of a work RAM byte with M 8-bit. */
static void inc8(Carried *cr, uint32_t addr)
{
    cyc(24);
    uint8_t v = (uint8_t)(rd8(addr) + 1);

    cyc(6);
    wr8(addr, v);
    set_p(cr, nz8(cr->p, v));
}

/* LDX (8-bit index) of an immediate or a byte. */
static void ldx8(Carried *cr, uint8_t v)
{
    cr->x = v;
    set_p(cr, nz8(cr->p, v));
}

/* The vertical blank's wait done inline: until HVBJOY bit 6. */
static void hblank_inline(Carried *cr)
{
    uint8_t v;

    do
        { cyc(24); v = hw_rd(HVBJOY); }
    while (cyc_loop(38, !(v & 0x40)));
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    set_p(cr, (uint8_t)(nz8(cr->p, v) & ~P_Z));
}

/* The frame's counters: $0E3B and $0306 up by one. */
static void frame_counts(Carried *cr, uint32_t b)
{
    inc8(cr, 0x7E0000 | W(frame_count));
    inc8(cr, 0x7E0000 | W(frames_since_tick));
}

/* BG2's scroll for the split screen's lower half: y by the sign of v
   ($E8,1 / $68,0 or $80,1 / 0,0), then x, its map and tile bases. */
static void split_bg2(Carried *cr, uint8_t v, uint8_t ylo, uint8_t yhi_pos, uint8_t yneg, uint8_t xlo)
{
    if (v & 0x80) {
        cyc(6);
        put8(cr, BG2VOFS, yneg);
        put8(cr, BG2VOFS, 0x00);
    } else {
        put8(cr, BG2VOFS, ylo);
        put8(cr, BG2VOFS, yhi_pos);
        cyc(22);                                       /* BRA */
    }
    put8(cr, BG2HOFS, xlo);
    put8(cr, BG2HOFS, 0x01);
    put8(cr, BG2SC, 0x78);
    put8(cr, BG12NBA, 0x70);
}

/* The window colours $19AC-$19AE (or $19AF-$19B1) to COLDATA. */
static void window_colours(Carried *cr, uint32_t b, uint16_t first)
{
    copy8(cr, COLDATA, b | first);
    copy8(cr, COLDATA, b | (uint16_t)(first + 1));
    copy8(cr, COLDATA, b | (uint16_t)(first + 2));
}

/*
 * @routine 7E:A305 RasterStage
 * WHAT IT DOES: Runs the raster interrupt's stage $0D3F (the table at
 * $7E:A30D), each of which sets the next line and stage: the screen off
 * or on (INIDISP, after waiting for horizontal blank), BG2's scroll and
 * the split screen's bases, colours and window setups, the frame's
 * counters and work for the vertical blank (fades, pads, sound ports, the
 * race clock, the HUD time and panels, palettes, the window animations,
 * the upload of the frame (NmiFrameUpload), sprites and the Super FX
 * picture copies). Stages $16 and $18-$1A do nothing; $1C sets the next
 * line to $10 (VTIMEL).
 * HOW: SEP #$30, JMP (table,X) on $0D3F; the stages end with RTL. Stages
 * 0, 2 and $1C and the negative split scroll at $A6EE are not reached in
 * the traces so far.
 * Entry: data bank 0. Exit: as the stage leaves it.
 */
void RasterStage(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;
    uint8_t v;

    r_sep(cr, P_M | P_X);
    cyc(24);
    ldx8(cr, rd8(0x7E0000 | W(wram_0D3F)));
    cyc(30);
    switch (rd16(W(wram_bank_7e) | (uint16_t)(0xA30D + cr->x))) {
    case 0xA330:
        cyc(46);
        WaitHBlank_cr(cr);
        copy8(cr, INIDISP, 0x7E0000 | W(fade_level_hi));
        put8(cr, VTIMEL, 0xC8);
        irq_ack(cr);
        put8(cr, 0x7E0000 | W(wram_0D3F), 0x02);
        frame_counts(cr, b);
        break;
    case 0xA34D:
        cyc(46);
        WaitHBlank_cr(cr);
        put8(cr, INIDISP, 0x80);
        put8(cr, VTIMEL, 0x17);
        irq_ack(cr);
        cyc(24);
        wr8(0x7E0000 | W(wram_0D3F), 0x00);
        put8(cr, 0x7E0000 | W(wram_0D52), 0xFF);
        put8(cr, HDMAEN, 0x02);
        (cyc(46), JoypadRead_cr(cr));
        (cyc(132), OamDmaAll_cr(cr));
        (cyc(46), SoundPortsSend_cr(cr));
        break;
    case 0xA375:
        cyc(68);
        WaitHBlank_cr(cr);
        put8(cr, TM, 0x13);
        copy8(cr, BG2VOFS, 0x7E0000 | W(scroll_value));
        copy8(cr, BG2VOFS, 0x7E0000 | W(wram_0E34));
        put8(cr, VTIMEL, 0xD8);
        irq_ack(cr);
        put8(cr, 0x7E0000 | W(wram_0D3F), 0x12);
        frame_counts(cr, b);
        (cyc(46), JoypadRead_cr(cr));
        (cyc(46), FadeStepFixed_cr(cr));
        (cyc(46), SoundPortsSend_cr(cr));
        break;
    case 0xA3A8: cyc(68); {
        uint8_t c;

        WaitHBlank_cr(cr);
        put8(cr, INIDISP, 0x80);
        put8(cr, VTIMEL, 0x37);
        irq_ack(cr);
        put8(cr, VTIMEH, 0x00);
        copy8(cr, HDMAEN, 0x7E0000 | W(wram_0D45));
        copy8(cr, CGADSUB, 0x7E0000 | W(colour_math_mode));
        put8(cr, 0x7E0000 | W(wram_0D3F), 0x10);
        put8(cr, 0x7E0000 | W(wram_0D52), 0xFF);
        (cyc(62), OamDmaAll_cr(cr));
        copy8(cr, BG2VOFS, 0x7E0000 | W(wram_0E33));
        copy8(cr, BG2VOFS, 0x7E0000 | W(wram_0E34));
        cyc(32);
        c = (uint8_t)(rd8(W(fixed_colour)) | 0x80);
        put8(cr, COLDATA, c);
        c = (uint8_t)((c & 0x7F) | 0x40);
        cyc(16);
        put8(cr, COLDATA, c);
        c = (uint8_t)((c & 0x1F) | 0x20);
        cyc(16);
        put8(cr, COLDATA, c);
        put8(cr, TM, 0x03);
        r_rep(cr, P_M);
        r_lda16_imm(cr, 0x1000);
        cyc(10);
        dma_wr16(DAS0L, 0x1000);
        r_lda16_imm(cr, 0x93D7);
        cyc(10);
        dma_wr16(A1T0L, 0x93D7);
        r_lda16_imm(cr, 0x5000);
        cyc(10);
        ppu_wr16(VMADDL, 0x5000);
        r_lda16_imm(cr, 0x1801);
        cyc(10);
        dma_wr16(DMAP0, 0x1801);
        r_sep(cr, P_M);
        cyc(22);
        put8(cr, A1B0, 0x7F);
        put8(cr, VMAIN, 0x80);
        cyc(24);
        c = (uint8_t)(rd8(0x7E0000 | W(gsu_scmr_shadow)) & 0xF7);
        put8(cr, GSU_SCMR, c);
        put8(cr, MDMAEN, 0x01);
        cyc(24);
        c = (uint8_t)(rd8(0x7E0000 | W(gsu_scmr_shadow)) | 0x18);
        put8(cr, GSU_SCMR, c);
        r_rep(cr, P_M | P_X);
        cyc(24);
        r_lda16(cr, 0x7E0000 | W(gsu_buffer_select));
        if (cr->a) {
            cyc(40);
            wr16(0x7E0000 | W(gsu_buffer_select), 0x0000);
            cr->x = 0x2C00;
            cr->y = 0x0400;
            r_lda16_imm(cr, 0x0800);
            fx_op_upload(1);
            (cyc(80), VramDmaFromGsuRam_cr(cr));
            fx_op_upload(0);
            r_rep(cr, P_M);
            r_lda16_imm(cr, 0xFFFF);
            cyc(18);
            wr16(W(wram_0D81), 0xFFFF);
        } else { cyc(22); }
        cyc(46);
        WindowTable7F(cr);
        r_sep(cr, P_M);
        cyc(22);
        copy8(cr, INIDISP, 0x7E0000 | W(fade_level_hi));
        break;
    }
    case 0xA4F5:
        cyc(24);
        ldx8(cr, rd8(0x7E0000 | W(fade_level_hi)));
        hblank_inline(cr);
        cyc(56);
        ppu_wr(INIDISP, (uint8_t)cr->x);
        put8(cr, HTIMEL, 0x30);
        put8(cr, VTIMEL, 0xC0);
        irq_ack(cr);
        put8(cr, 0x7E0000 | W(wram_0D3F), 0x14);
        frame_counts(cr, b);
        put8(cr, 0x7E0000 | W(wram_0D52), 0xFF);
        (cyc(46), FadeStep_cr(cr));
        (cyc(46), JoypadAccumulate_cr(cr));
        (cyc(46), SoundPortsSend_cr(cr));
        (cyc(46), RaceClockStep_cr(cr));
        r_rep(cr, P_M);
        r_lda16_imm(cr, 0x01D8);
        cyc(10);
        wr16(0x7E0000 | W(hud_time_sprite), 0x01D8);
        w_HudDrawTime(cr);
        (cyc(46), HudPanelsStep_cr(cr));
        (cyc(46), PalettesStep_cr(cr));
        break;
    case 0xA53D:
        put8(cr, VTIMEL, 0xD0);
        irq_ack(cr);
        put8(cr, 0x7E0000 | W(wram_0D3F), 0x06);
        (cyc(62), WindowHdmaNormal_cr(cr));
        (cyc(62), WindowLayoutStepForScreen_cr(cr));
        (cyc(62), WindowSlideOrOscillate(cr));
        (cyc(62), WindowSweepIfOn(cr));
        (cyc(62), WindowSlideCheck_cr(cr));
        (cyc(62), BobWindowsRun(cr));
        (cyc(62), WindowBandsAnimate_cr(cr));
        (cyc(62), WindowSplitAnimate_cr(cr));
        break;
    case 0xA56B:
        cyc(16);
        ldx8(cr, 0x80);
        hblank_inline(cr);
        cyc(56);
        ppu_wr(INIDISP, (uint8_t)cr->x);
        (cyc(46), NmiFrameUpload_cr(cr));
        (cyc(46), OamDirectStage_cr(cr));
        (cyc(46), RasterColoursUnless0D22_cr(cr));
        r_sep(cr, P_M);
        b = (uint32_t)cr->db << 16;
        cyc(22);
        put8(cr, HTIMEL, 0xE0);
        put8(cr, VTIMEL, 0x10);
        irq_ack(cr);
        put8(cr, TM, 0x13);
        copy8(cr, HDMAEN, 0x7E0000 | W(wram_0D45));
        put8(cr, 0x7E0000 | W(wram_0D3F), 0x04);
        window_colours(cr, b, W(window_setup_bytes));
        (cyc(62), WindowsSetupSelect_cr(cr));
        break;
    case 0xA5B6:
        cyc(24);
        ldx8(cr, cyc_then(46, rd8(0x7E0000 | W(fade_level_hi))));
        WaitHBlank_cr(cr);
        cyc(24);
        ppu_wr(INIDISP, (uint8_t)cr->x);
        put8(cr, VTIMEL, 0x6F);
        put8(cr, HTIMEH, 0x00);
        cyc(24);
        hw_wr(VTIMEH, 0x00);
        put8(cr, HTIMEL, 0x10);
        irq_ack(cr);
        put8(cr, 0x7E0000 | W(wram_0D3F), 0x0A);
        frame_counts(cr, b);
        put8(cr, 0x7E0000 | W(wram_0D52), 0xFF);
        (cyc(46), FadeStep_cr(cr));
        (cyc(46), RaceClockStep_cr(cr));
        r_sep(cr, P_M);
        cyc(22);
        put8(cr, TM, 0x13);
        r_rep(cr, P_M);
        r_lda16_imm(cr, 0x0100);
        cyc(10);
        wr16(0x7E0000 | W(hud_time_sprite), 0x0100);
        w_HudDrawTime(cr);
        (cyc(46), HudPanelASlide_cr(cr));
        (cyc(46), HudPanelBSlide_cr(cr));
        (cyc(46), SoundPortsSend_cr(cr));
        r_sep(cr, P_M | P_X);
        cr->y = 0x01;
        ldx8(cr, 0x02);
        PaletteKeptColoursStep_cr(cr);
        break;
    case 0xA60F:
        cyc(62);
        ldx8(cr, 0x80);
        WaitHBlank_cr(cr);
        cyc(38);
        ppu_wr(INIDISP, (uint8_t)cr->x);
        put8(cr, VTIMEL, 0x77);
        irq_ack(cr);
        put8(cr, 0x7E0000 | W(wram_0D3F), 0x0C);
        window_colours(cr, b, W(wram_19AF));
        cyc(24);
        r_lda8(cr, 0x7E0000 | W(wram_0D5D));
        v = (uint8_t)cr->a;
        if (v) {
            cyc(32);
            split_bg2(cr, v, 0x80, 0x01, 0x00, 0xA0);
            break;
        }
        cyc(46);
        r_lda8(cr, 0x7E0000 | W(wram_0D5C));
        if ((uint8_t)cr->a)
            { cyc(16); put8(cr, BG12NBA, 0x40); } else { cyc(22); }
        put8(cr, BG2SC, 0x3F);
        cyc(62);
        ldx8(cr, 0x02);
        Bg2ScrollFromTable_cr(cr);
        (cyc(46), RasterColours_cr(cr));
        break;
    case 0xA681:
        cyc(24);
        ldx8(cr, cyc_then(46, rd8(0x7E0000 | W(fade_level_hi))));
        WaitHBlank_cr(cr);
        cyc(24);
        ppu_wr(INIDISP, (uint8_t)cr->x);
        put8(cr, VTIMEL, 0xD7);
        irq_ack(cr);
        put8(cr, 0x7E0000 | W(wram_0D3F), 0x0E);
        r_rep(cr, P_M);
        r_lda16_imm(cr, 0x0000);
        cyc(10);
        wr16(0x7E0000 | W(hud_time_sprite), 0x0000);
        w_HudDrawTime(cr);
        r_sep(cr, P_M | P_X);
        cr->y = 0x00;
        ldx8(cr, 0x00);
        PaletteKeptColoursStep_cr(cr);
        (cyc(62), WindowSplitAnimate_cr(cr));
        break;
    case 0xA6B0:
        cyc(62);
        ldx8(cr, 0x80);
        WaitHBlank_cr(cr);
        cyc(24);
        ppu_wr(INIDISP, (uint8_t)cr->x);
        (cyc(46), NmiFrameUpload_cr(cr));
        (cyc(62), OamDmaAll_cr(cr));
        (cyc(62), WindowLayoutStepForScreen_cr(cr));
        (cyc(46), RasterPaletteUpload_cr(cr));
        (cyc(46), JoypadAccumulate_cr(cr));
        (cyc(62), WindowHdmaBands_cr(cr));
        (cyc(62), WindowLayoutStepForScreen_cr(cr));
        (cyc(62), BobWindowsRun(cr));
        (cyc(62), WindowBandsAnimate_cr(cr));
        r_sep(cr, P_M | P_X);
        b = (uint32_t)cr->db << 16;
        cyc(24);
        r_lda8(cr, 0x7E0000 | W(wram_0D5C));
        v = (uint8_t)cr->a;
        if (v) {
            cyc(32);
            split_bg2(cr, v, 0xE8, 0x01, 0x68, 0xF0);
            cyc(22);                                   /* BRA */
        } else {
            cyc(46);
            r_lda8(cr, 0x7E0000 | W(wram_0D5D));
            if ((uint8_t)cr->a)
                put8(cr, BG12NBA, 0x40); else { cyc(84); }
            ldx8(cr, 0x00);
            Bg2ScrollFromTable_cr(cr);
            cyc(0);
            put8(cr, BG1SC, 0x1C);
            put8(cr, BG2SC, 0x3F);
        }
        put8(cr, BGMODE, 0x01);
        put8(cr, TM, 0x03);
        put8(cr, HDMAEN, 0x1C);
        window_colours(cr, b, W(window_setup_bytes));
        (cyc(62), WindowsSetupSelect_cr(cr));
        put8(cr, VTIMEL, 0x0F);
        irq_ack(cr);
        put8(cr, 0x7E0000 | W(wram_0D3F), 0x08);
        (cyc(46), w_GradientPointersToBank7F(cr));
        break;
    case 0xA75D:
        put8(cr, VTIMEL, 0x10);
        break;
    case 0xA763:
        cyc(46);
        WaitHBlank_cr(cr);
        put8(cr, INIDISP, 0x80);
        (cyc(62), VblankUpdates_cr(cr));
        (cyc(46), FrameSoundAndJoypads(cr));
        (cyc(62), FrameCountAndPortsEnd_cr(cr));
        r_sep(cr, P_M);
        cyc(22);
        put8(cr, VTIMEL, 0xE0);
        irq_ack(cr);
        break;
    case 0xA781:
        cyc(46);
        WaitHBlank_cr(cr);
        put8(cr, INIDISP, 0x80);
        (cyc(62), AnimTilesQueueNext_cr(cr));
        (cyc(62), VblankUpdates_cr(cr));
        (cyc(46), FrameSoundAndJoypads(cr));
        (cyc(62), FrameInputAndCountEnd_cr(cr));
        r_sep(cr, P_M);
        cyc(22);
        put8(cr, VTIMEL, 0xE0);
        irq_ack(cr);
        break;
    default:
        /* $A32F and $A75C: an RTL alone. */
        break;
    }
    cyc(44);
}

void abi_RasterStage(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RasterStage(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 7E:A2D9 RasterIrq
 * WHAT IT DOES: The IRQ handler: for the raster interrupt (TIMEUP bit 7,
 * read with RDNMI as one word, which acknowledges it) runs the stage
 * (RasterStage) with interrupts off and data bank 0, keeping A, X, Y and
 * the data bank; for any other source reads the Super FX status word,
 * which acknowledges the Super FX's interrupt.
 * HOW: 16-bit pushes around the stage; ends with RTL to the vector code.
 * Entry: any widths. Exit: A, X, Y and the data bank as at entry; M and X
 * 16-bit; N, Z from A; after a stage I set, C and V as the stage left
 * them.
 */
void RasterIrq(Carried *cr)
{
    uint16_t a0, x0, y0;
    uint8_t db0 = cr->db;

    r_rep(cr, P_M | P_X);
    a0 = cr->a;
    cyc(62);
    r_lda16(cr, 0x004210);
    if (!(cr->a & 0x8000)) {
        cyc(48);
        r_lda16(cr, 0x003030);
        cr->a = a0;
        set_p(cr, nz16(cr->p, a0));
        cyc(80);
        return;
    } else { cyc(246); }
    set_p(cr, (uint8_t)(cr->p | (P_I)));
    x0 = cr->x;
    y0 = cr->y;
    r_sep(cr, P_M | P_X);
    cr->a = (uint16_t)(cr->a & 0xFF00);
    set_p(cr, nz8(cr->p, 0));
    cr->db = 0x00;
    set_p(cr, nz8(cr->p, 0));
    RasterStage(cr);
    r_rep(cr, P_M | P_X);
    cr->y = y0;
    set_p(cr, nz16(cr->p, y0));
    cr->x = x0;
    set_p(cr, nz16(cr->p, x0));
    cr->db = db0;
    set_p(cr, nz8(cr->p, db0));
    cr->a = a0;
    set_p(cr, nz16(cr->p, a0));
    cyc(180);
}

void abi_RasterIrq(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RasterIrq(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* The time stamp both marks take: the frame counter $0306 as the high
   byte and the PPU's line (latched by SLHV, read from OPVCT low then high,
   9 bits) as the low, where lines $100 and over count as $FF and lines
   below $11 (the top of the next frame) as $100 on. $1A37 = the frame part
   (its low byte 0). Interrupts are off around the counter read. Returns
   the line part plus $1A37, with the flags of that add. */
static uint16_t time_stamp(Carried *cr)
{
    uint16_t f, v, m;
    uint8_t lo, hi;

    set_p(cr, (uint8_t)(cr->p | (P_I)));
    cyc(68);
    f = rd16(W(frames_since_tick));
    f = (uint16_t)(((f >> 8) | (f << 8)) & 0xFF00);
    cyc(76);
    wr16(W(time_now), f);
    cyc(54);
    hw_rd(0x002137);
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_I)));
    cyc(46);
    lo = hw_rd(0x00213D);
    cyc(52);
    hi = hw_rd(0x00213D);
    v = (uint16_t)(((hi << 8) | lo) & 0x01FF);
    if (!((uint16_t)(v - 0x0100) & 0x8000))
        v = 0x00FF;
    if ((uint16_t)(v - 0x0011) & 0x8000)
        v = (uint16_t)(v + 0x0100);
    cyc(236);
    m = rd16(W(time_now));
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_C)));
    return alu_adc16(v, m, &cr->p);
}

/*
 * @routine 7E:F14F TimeMarkStart
 * WHAT IT DOES: Records the time now (frame and line, as time_stamp says)
 * in the word at $1A39 + X: the start of a timed stretch of a frame.
 * Nothing in the game calls it.
 * HOW: Long accesses to bank $00; see time_stamp.
 * Entry: X 16-bit; X = the slot's offset. Exit: M 16-bit; A = the time;
 * X, Y unchanged; I clear; N, Z, C, V from the last add.
 */
void TimeMarkStart(Carried *cr)
{
    uint16_t t;

    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    t = time_stamp(cr);
    cyc(32);
    wr16(W(time_marks) + cr->x, t);
    cyc(44);
    cr->a = t;
}

void abi_TimeMarkStart(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    TimeMarkStart(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 7E:F18F TimeMarkEnd
 * WHAT IT DOES: Adds the time since the start recorded at $1A39 + X to the
 * running total at $1A37 + X. A start late in its frame (low byte $D1 or
 * more) is taken as is; an earlier one first has the frames between, each
 * as 64 lines, taken off the end time. Nothing in the game calls it.
 * HOW: The end time in $1A37 (time_stamp); the start's frame byte kept in
 * $1A39 for the correction (read back through the data bank); 16-bit
 * long adds and subtracts.
 * Entry: X 16-bit; X = the slot's offset; data bank $00. Exit: M 16-bit;
 * A = the new total; X, Y unchanged; I clear; N, Z, C, V from the last
 * add.
 */
void TimeMarkEnd(Carried *cr)
{
    uint16_t t, a, m;

    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    t = time_stamp(cr);
    cyc(32);
    wr16(W(time_now), t);
    cyc(32);
    a = rd16(W(time_marks) + cr->x);
    a = (uint16_t)(a & 0x00FF);
    set_p(cr, nz16(cr->p, a));
    if ((uint16_t)(a - 0x00D1) & 0x8000) {
        cyc(96);
        a = rd16(W(time_marks) + cr->x);
        a = (uint16_t)(a & 0xFF00);
        cyc(56);
        wr16(W(time_marks), a);
        cyc(32);
        a = rd16(W(time_now));
        a = (uint16_t)(a & 0xFF00);
        cyc(62);
        m = rd16(0x7E0000 | W(time_marks));
        set_p(cr, (uint8_t)(cr->p | (P_C)));
        a = alu_sbc16(a, m, &cr->p);
        a = (uint16_t)((a >> 8) | (a << 8));
        a = (uint16_t)(a << 6);
        a = (uint16_t)(~a + 1);
        cyc(182);
        m = rd16(W(time_now));
        set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_C)));
        a = alu_adc16(a, m, &cr->p);
        cyc(32);
        wr16(W(time_now), a);
    } else { cyc(70); }
    cyc(32);
    a = rd16(W(time_now));
    cyc(46);
    m = rd16(W(time_marks) + cr->x);
    set_p(cr, (uint8_t)(cr->p | (P_C)));
    a = alu_sbc16(a, m, &cr->p);
    cyc(40);
    m = rd16(W(time_now) + cr->x);
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_C)));
    a = alu_adc16(a, m, &cr->p);
    cyc(32);
    wr16(W(time_now) + cr->x, a);
    cyc(44);
    cr->a = a;
}

void abi_TimeMarkEnd(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    TimeMarkEnd(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* GradientPointersToBank7F as its adapter leaves the registers. */
static void w_GradientPointersToBank7F(Carried *cr)
{
    cr->a = GradientPointersToBank7F();
    set_p(cr, (uint8_t)(nz16(cr->p, cr->a) & ~P_M));
}

/* HudDrawTime as its adapter leaves the registers. */
static void w_HudDrawTime(Carried *cr)
{
    uint16_t x;
    int x_set;
    uint8_t a = HudDrawTime(&x, &x_set);

    cr->a = (uint16_t)((cr->a & 0xFF00) | a);
    if (x_set)
        cr->x = (cr->p & P_X) ? (uint8_t)x : x;
}
