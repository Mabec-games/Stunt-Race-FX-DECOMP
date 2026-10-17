/*
 * raster.c: waiting on the PPU's beam inside the raster interrupt.
 */
#include "machine.h"
#include "hw.h"
#include "cpu_ops.h"

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

    do
        v = rd8(HVBJOY);
    while (!(v & 0x40));
    return v;
}

void abi_WaitHBlank(void)
{
    uint8_t v = WaitHBlank();

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = (uint8_t)(nz8(cpu.p, v) & ~P_Z);
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

    wr8(b | 0x06E4, 0x00);
    do {
        rd8(SLHV);
        lo = rd8(OPVCT);
        rd8(OPVCT);
    } while (lo != rd8(b | 0x06E3));
}

void abi_WaitScanline(void)
{
    WaitScanline(cpu.db);
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
    wr8(addr, v);
}

void abi_WindowTable7F(void)
{
    uint8_t a, b2;

    op_sep(P_M);
    a = rd8(0x7F3438);
    b2 = rd8(0x7F3434);
    cpu.p |= P_C;
    a = op_sbc8(a, b2, &cpu.p);
    put7f(0x7F3402, a);
    cpu.p = (uint8_t)((cpu.p & ~P_C) | (a & 1));
    a = (uint8_t)(a >> 1);
    cpu.p &= (uint8_t)~P_C;
    a = op_adc8(a, rd8(0x7F3434), &cpu.p);
    put7f(0x7F3406, a);
    a = rd8(0x7F3435);
    a = (uint8_t)(a >> 1);
    put7f(0x7F3410, a);
    a++;
    put7f(0x7F3413, a);
    a = rd8(0x7F3433);
    put7f(0x7F3420, a);
    put7f(0x7F3423, a);
    a++;
    put7f(0x7F341D, a);
    put7f(0x7F3426, a);
    a++;
    put7f(0x7F341A, a);
    put7f(0x7F3429, a);
    a++;
    put7f(0x7F3417, a);
    put7f(0x7F342C, a);
    a = rd8(0x7F3437);
    put7f(0x7F3421, a);
    put7f(0x7F3424, a);
    a--;
    put7f(0x7F341E, a);
    put7f(0x7F3427, a);
    a--;
    put7f(0x7F341B, a);
    put7f(0x7F342A, a);
    a--;
    put7f(0x7F3418, a);
    put7f(0x7F342D, a);
    a = rd8(0x7F3439);
    cpu.p |= P_C;
    a = op_sbc8(a, rd8(0x7F3435), &cpu.p);
    a = op_sbc8(a, 0x06, &cpu.p);
    cpu.p = (uint8_t)((cpu.p & ~P_C) | (a & 1));
    a = (uint8_t)(a >> 1);
    cpu.p = nz8(cpu.p, a);
    put7f(0x7F341F, a);
    put7f(0x7F3422, a);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | a);
}

/*
 * @routine 7E:A467 WindowTable7FLong
 * WHAT IT DOES: WindowTable7F for a JSL.
 * HOW: JSR, RTL.
 * Entry: as WindowTable7F. Exit: as WindowTable7F.
 */
void abi_WindowTable7FLong(void)
{
    abi_WindowTable7F();
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
static void put8(uint32_t addr, uint8_t v)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, v);
    wr8(addr, v);
}

/* LDA from / STA to with M 8-bit. */
static void copy8(uint32_t to, uint32_t from)
{
    uint8_t v = rd8(from);

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, v);
    wr8(to, v);
}

/* LDA TIMEUP: acknowledges the raster interrupt. */
static void irq_ack(void)
{
    op_lda8(TIMEUP);
}

/* INC of a work RAM byte with M 8-bit. */
static void inc8(uint32_t addr)
{
    uint8_t v = (uint8_t)(rd8(addr) + 1);

    wr8(addr, v);
    cpu.p = nz8(cpu.p, v);
}

/* LDX (8-bit index) of an immediate or a byte. */
static void ldx8(uint8_t v)
{
    cpu.x = v;
    cpu.p = nz8(cpu.p, v);
}

/* The vertical blank's wait done inline: until HVBJOY bit 6. */
static void hblank_inline(void)
{
    uint8_t v;

    do
        v = rd8(HVBJOY);
    while (!(v & 0x40));
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = (uint8_t)(nz8(cpu.p, v) & ~P_Z);
}

/* The frame's counters: $0E3B and $0306 up by one. */
static void frame_counts(uint32_t b)
{
    inc8(b | 0x0E3B);
    inc8(b | 0x0306);
}

/* BG2's scroll for the split screen's lower half: y by the sign of v
   ($E8,1 / $68,0 or $80,1 / 0,0), then x, its map and tile bases. */
static void split_bg2(uint8_t v, uint8_t ylo, uint8_t yhi_pos, uint8_t yneg, uint8_t xlo)
{
    if (v & 0x80) {
        put8(BG2VOFS, yneg);
        put8(BG2VOFS, 0x00);
    } else {
        put8(BG2VOFS, ylo);
        put8(BG2VOFS, yhi_pos);
    }
    put8(BG2HOFS, xlo);
    put8(BG2HOFS, 0x01);
    put8(BG2SC, 0x78);
    put8(BG12NBA, 0x70);
}

/* The window colours $19AC-$19AE (or $19AF-$19B1) to COLDATA. */
static void window_colours(uint32_t b, uint16_t first)
{
    copy8(COLDATA, b | first);
    copy8(COLDATA, b | (uint16_t)(first + 1));
    copy8(COLDATA, b | (uint16_t)(first + 2));
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
void abi_RasterStage(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint8_t v;

    op_sep(P_M | P_X);
    ldx8(rd8(b | 0x0D3F));
    switch (rd16(0x7E0000 | (uint16_t)(0xA30D + cpu.x))) {
    case 0xA330:
        abi_WaitHBlank();
        copy8(INIDISP, b | 0x0D61);
        put8(VTIMEL, 0xC8);
        irq_ack();
        put8(b | 0x0D3F, 0x02);
        frame_counts(b);
        break;
    case 0xA34D:
        abi_WaitHBlank();
        put8(INIDISP, 0x80);
        put8(VTIMEL, 0x17);
        irq_ack();
        wr8(b | 0x0D3F, 0x00);
        put8(b | 0x0D52, 0xFF);
        put8(HDMAEN, 0x02);
        abi_JoypadRead();
        abi_OamDmaAll();
        abi_SoundPortsSend();
        break;
    case 0xA375:
        abi_WaitHBlank();
        put8(TM, 0x13);
        copy8(BG2VOFS, b | 0x0E37);
        copy8(BG2VOFS, b | 0x0E34);
        put8(VTIMEL, 0xD8);
        irq_ack();
        put8(b | 0x0D3F, 0x12);
        frame_counts(b);
        abi_JoypadRead();
        abi_FadeStepFixed();
        abi_SoundPortsSend();
        break;
    case 0xA3A8: {
        uint8_t c;

        abi_WaitHBlank();
        put8(INIDISP, 0x80);
        put8(VTIMEL, 0x37);
        irq_ack();
        put8(VTIMEH, 0x00);
        copy8(HDMAEN, b | 0x0D45);
        copy8(CGADSUB, b | 0x0D47);
        put8(b | 0x0D3F, 0x10);
        put8(b | 0x0D52, 0xFF);
        abi_OamDmaAll();
        copy8(BG2VOFS, b | 0x0E33);
        copy8(BG2VOFS, b | 0x0E34);
        c = (uint8_t)(rd8(0x7F340D) | 0x80);
        put8(COLDATA, c);
        c = (uint8_t)((c & 0x7F) | 0x40);
        put8(COLDATA, c);
        c = (uint8_t)((c & 0x1F) | 0x20);
        put8(COLDATA, c);
        put8(TM, 0x03);
        op_rep(P_M);
        op_lda16_imm(0x1000);
        wr16(DAS0L, 0x1000);
        op_lda16_imm(0x93D7);
        wr16(A1T0L, 0x93D7);
        op_lda16_imm(0x5000);
        wr16(VMADDL, 0x5000);
        op_lda16_imm(0x1801);
        wr16(DMAP0, 0x1801);
        op_sep(P_M);
        put8(A1B0, 0x7F);
        put8(VMAIN, 0x80);
        c = (uint8_t)(rd8(b | 0x0374) & 0xF7);
        put8(GSU_SCMR, c);
        put8(MDMAEN, 0x01);
        c = (uint8_t)(rd8(b | 0x0374) | 0x18);
        put8(GSU_SCMR, c);
        op_rep(P_M | P_X);
        op_lda16(b | 0x0D7F);
        if (cpu.a) {
            wr16(b | 0x0D7F, 0x0000);
            cpu.x = 0x2C00;
            cpu.y = 0x0400;
            op_lda16_imm(0x0800);
            abi_VramDmaFromGsuRam();
            op_rep(P_M);
            op_lda16_imm(0xFFFF);
            wr16(0x000D81, 0xFFFF);
        }
        abi_WindowTable7F();
        op_sep(P_M);
        copy8(INIDISP, b | 0x0D61);
        break;
    }
    case 0xA4F5:
        ldx8(rd8(b | 0x0D61));
        hblank_inline();
        wr8(INIDISP, (uint8_t)cpu.x);
        put8(HTIMEL, 0x30);
        put8(VTIMEL, 0xC0);
        irq_ack();
        put8(b | 0x0D3F, 0x14);
        frame_counts(b);
        put8(b | 0x0D52, 0xFF);
        abi_FadeStep();
        abi_JoypadAccumulate();
        abi_SoundPortsSend();
        abi_RaceClockStep();
        op_rep(P_M);
        op_lda16_imm(0x01D8);
        wr16(b | 0x0E60, 0x01D8);
        abi_HudDrawTime();
        abi_HudPanelsStep();
        abi_PalettesStep();
        break;
    case 0xA53D:
        put8(VTIMEL, 0xD0);
        irq_ack();
        put8(b | 0x0D3F, 0x06);
        abi_WindowHdmaNormal();
        abi_WindowLayoutStepForScreen();
        abi_WindowSlideOrOscillate();
        abi_WindowSweepIfOn();
        abi_WindowSlideCheck();
        abi_BobWindowsRun();
        abi_WindowBandsAnimate();
        abi_WindowSplitAnimate();
        break;
    case 0xA56B:
        ldx8(0x80);
        hblank_inline();
        wr8(INIDISP, (uint8_t)cpu.x);
        abi_NmiFrameUpload();
        abi_OamDirectStage();
        abi_RasterColoursUnless0D22();
        op_sep(P_M);
        b = (uint32_t)cpu.db << 16;
        put8(HTIMEL, 0xE0);
        put8(VTIMEL, 0x10);
        irq_ack();
        put8(TM, 0x13);
        copy8(HDMAEN, b | 0x0D45);
        put8(b | 0x0D3F, 0x04);
        window_colours(b, 0x19AC);
        abi_WindowsSetupSelect();
        break;
    case 0xA5B6:
        ldx8(rd8(b | 0x0D61));
        abi_WaitHBlank();
        wr8(INIDISP, (uint8_t)cpu.x);
        put8(VTIMEL, 0x6F);
        put8(HTIMEH, 0x00);
        wr8(VTIMEH, 0x00);
        put8(HTIMEL, 0x10);
        irq_ack();
        put8(b | 0x0D3F, 0x0A);
        frame_counts(b);
        put8(b | 0x0D52, 0xFF);
        abi_FadeStep();
        abi_RaceClockStep();
        op_sep(P_M);
        put8(TM, 0x13);
        op_rep(P_M);
        op_lda16_imm(0x0100);
        wr16(b | 0x0E60, 0x0100);
        abi_HudDrawTime();
        abi_HudPanelASlide();
        abi_HudPanelBSlide();
        abi_SoundPortsSend();
        op_sep(P_M | P_X);
        cpu.y = 0x01;
        ldx8(0x02);
        abi_PaletteKeptColoursStep();
        break;
    case 0xA60F:
        ldx8(0x80);
        abi_WaitHBlank();
        wr8(INIDISP, (uint8_t)cpu.x);
        put8(VTIMEL, 0x77);
        irq_ack();
        put8(b | 0x0D3F, 0x0C);
        window_colours(b, 0x19AF);
        op_lda8(b | 0x0D5D);
        v = (uint8_t)cpu.a;
        if (v) {
            split_bg2(v, 0x80, 0x01, 0x00, 0xA0);
            break;
        }
        op_lda8(b | 0x0D5C);
        if ((uint8_t)cpu.a)
            put8(BG12NBA, 0x40);
        put8(BG2SC, 0x3F);
        ldx8(0x02);
        abi_Bg2ScrollFromTable();
        abi_RasterColours();
        break;
    case 0xA681:
        ldx8(rd8(b | 0x0D61));
        abi_WaitHBlank();
        wr8(INIDISP, (uint8_t)cpu.x);
        put8(VTIMEL, 0xD7);
        irq_ack();
        put8(b | 0x0D3F, 0x0E);
        op_rep(P_M);
        op_lda16_imm(0x0000);
        wr16(b | 0x0E60, 0x0000);
        abi_HudDrawTime();
        op_sep(P_M | P_X);
        cpu.y = 0x00;
        ldx8(0x00);
        abi_PaletteKeptColoursStep();
        abi_WindowSplitAnimate();
        break;
    case 0xA6B0:
        ldx8(0x80);
        abi_WaitHBlank();
        wr8(INIDISP, (uint8_t)cpu.x);
        abi_NmiFrameUpload();
        abi_OamDmaAll();
        abi_WindowLayoutStepForScreen();
        abi_RasterPaletteUpload();
        abi_JoypadAccumulate();
        abi_WindowHdmaBands();
        abi_WindowLayoutStepForScreen();
        abi_BobWindowsRun();
        abi_WindowBandsAnimate();
        op_sep(P_M | P_X);
        b = (uint32_t)cpu.db << 16;
        op_lda8(b | 0x0D5C);
        v = (uint8_t)cpu.a;
        if (v) {
            split_bg2(v, 0xE8, 0x01, 0x68, 0xF0);
        } else {
            op_lda8(b | 0x0D5D);
            if ((uint8_t)cpu.a)
                put8(BG12NBA, 0x40);
            ldx8(0x00);
            abi_Bg2ScrollFromTable();
            put8(BG1SC, 0x1C);
            put8(BG2SC, 0x3F);
        }
        put8(BGMODE, 0x01);
        put8(TM, 0x03);
        put8(HDMAEN, 0x1C);
        window_colours(b, 0x19AC);
        abi_WindowsSetupSelect();
        put8(VTIMEL, 0x0F);
        irq_ack();
        put8(b | 0x0D3F, 0x08);
        abi_GradientPointersToBank7F();
        break;
    case 0xA75D:
        put8(VTIMEL, 0x10);
        break;
    case 0xA763:
        abi_WaitHBlank();
        put8(INIDISP, 0x80);
        abi_VblankUpdates();
        abi_FrameSoundAndJoypads();
        abi_FrameCountAndPortsEnd();
        op_sep(P_M);
        put8(VTIMEL, 0xE0);
        irq_ack();
        break;
    case 0xA781:
        abi_WaitHBlank();
        put8(INIDISP, 0x80);
        abi_AnimTilesQueueNext();
        abi_VblankUpdates();
        abi_FrameSoundAndJoypads();
        abi_FrameInputAndCountEnd();
        op_sep(P_M);
        put8(VTIMEL, 0xE0);
        irq_ack();
        break;
    default:
        /* $A32F and $A75C: an RTL alone. */
        break;
    }
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
void abi_RasterIrq(void)
{
    uint16_t a0, x0, y0;
    uint8_t db0 = cpu.db;

    op_rep(P_M | P_X);
    a0 = cpu.a;
    op_lda16(0x004210);
    if (!(cpu.a & 0x8000)) {
        op_lda16(0x003030);
        cpu.a = a0;
        cpu.p = nz16(cpu.p, a0);
        return;
    }
    cpu.p |= P_I;
    x0 = cpu.x;
    y0 = cpu.y;
    op_sep(P_M | P_X);
    cpu.a = (uint16_t)(cpu.a & 0xFF00);
    cpu.p = nz8(cpu.p, 0);
    cpu.db = 0x00;
    cpu.p = nz8(cpu.p, 0);
    abi_RasterStage();
    op_rep(P_M | P_X);
    cpu.y = y0;
    cpu.p = nz16(cpu.p, y0);
    cpu.x = x0;
    cpu.p = nz16(cpu.p, x0);
    cpu.db = db0;
    cpu.p = nz8(cpu.p, db0);
    cpu.a = a0;
    cpu.p = nz16(cpu.p, a0);
}

/* The time stamp both marks take: the frame counter $0306 as the high
   byte and the PPU's line (latched by SLHV, read from OPVCT low then high,
   9 bits) as the low, where lines $100 and over count as $FF and lines
   below $11 (the top of the next frame) as $100 on. $1A37 = the frame part
   (its low byte 0). Interrupts are off around the counter read. Returns
   the line part plus $1A37, with the flags of that add. */
static uint16_t time_stamp(void)
{
    uint16_t f, v, m;
    uint8_t lo, hi;

    cpu.p |= P_I;
    f = rd16(0x000306);
    f = (uint16_t)(((f >> 8) | (f << 8)) & 0xFF00);
    wr16(0x001A37, f);
    rd8(0x002137);
    cpu.p &= (uint8_t)~P_I;
    lo = rd8(0x00213D);
    hi = rd8(0x00213D);
    v = (uint16_t)(((hi << 8) | lo) & 0x01FF);
    if (!((uint16_t)(v - 0x0100) & 0x8000))
        v = 0x00FF;
    if ((uint16_t)(v - 0x0011) & 0x8000)
        v = (uint16_t)(v + 0x0100);
    m = rd16(0x001A37);
    cpu.p &= (uint8_t)~P_C;
    return op_adc16(v, m, &cpu.p);
}

/*
 * @routine 7E:F14F TimeMarkStart
 * WHAT IT DOES: Records the time now (frame and line, as time_stamp says)
 * in the word at $1A39 + X: the start of a timed stretch of a frame.
 * Nothing in the game calls it.
 * The path for lines $100 and over (the last 6 of vertical blank) is
 * not reached in the traces so far.
 * HOW: Long accesses to bank $00; see time_stamp.
 * Entry: X 16-bit; X = the slot's offset. Exit: M 16-bit; A = the time;
 * X, Y unchanged; I clear; N, Z, C, V from the last add.
 */
void abi_TimeMarkStart(void)
{
    uint16_t t;

    cpu.p &= (uint8_t)~P_M;
    t = time_stamp();
    wr16(0x001A39 + cpu.x, t);
    cpu.a = t;
}

/*
 * @routine 7E:F18F TimeMarkEnd
 * WHAT IT DOES: Adds the time since the start recorded at $1A39 + X to the
 * running total at $1A37 + X. A start late in its frame (low byte $D1 or
 * more) is taken as is; an earlier one first has the frames between, each
 * as 64 lines, taken off the end time. Nothing in the game calls it.
 * The path for lines $100 and over (the last 6 of vertical blank) is
 * not reached in the traces so far.
 * HOW: The end time in $1A37 (time_stamp); the start's frame byte kept in
 * $1A39 for the correction (read back through the data bank); 16-bit
 * long adds and subtracts.
 * Entry: X 16-bit; X = the slot's offset; data bank $00. Exit: M 16-bit;
 * A = the new total; X, Y unchanged; I clear; N, Z, C, V from the last
 * add.
 */
void abi_TimeMarkEnd(void)
{
    uint16_t t, a, m;

    cpu.p &= (uint8_t)~P_M;
    t = time_stamp();
    wr16(0x001A37, t);
    a = rd16(0x001A39 + cpu.x);
    a = (uint16_t)(a & 0x00FF);
    cpu.p = nz16(cpu.p, a);
    if ((uint16_t)(a - 0x00D1) & 0x8000) {
        a = rd16(0x001A39 + cpu.x);
        a = (uint16_t)(a & 0xFF00);
        wr16(0x001A39, a);
        a = rd16(0x001A37);
        a = (uint16_t)(a & 0xFF00);
        m = rd16((uint32_t)cpu.db << 16 | 0x1A39);
        cpu.p |= P_C;
        a = op_sbc16(a, m, &cpu.p);
        a = (uint16_t)((a >> 8) | (a << 8));
        a = (uint16_t)(a << 6);
        a = (uint16_t)(~a + 1);
        m = rd16(0x001A37);
        cpu.p &= (uint8_t)~P_C;
        a = op_adc16(a, m, &cpu.p);
        wr16(0x001A37, a);
    }
    a = rd16(0x001A37);
    m = rd16(0x001A39 + cpu.x);
    cpu.p |= P_C;
    a = op_sbc16(a, m, &cpu.p);
    m = rd16(0x001A37 + cpu.x);
    cpu.p &= (uint8_t)~P_C;
    a = op_adc16(a, m, &cpu.p);
    wr16(0x001A37 + cpu.x, a);
    cpu.a = a;
}
