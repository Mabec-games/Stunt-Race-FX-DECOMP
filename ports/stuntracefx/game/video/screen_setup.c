/*
 * screen_setup.c: PPU, window, HDMA and Super FX screen settings written
 * when a screen is set up. Register stores go through the data bank, which
 * the callers keep in the I/O banks.
 */
#include "machine.h"
#include "native.h"
#include "hw.h"
#include "cpu_ops.h"
#include "carried.h"
#include "wram_names.h"
#include "wram_struct.h"


static void io8(uint8_t bank, uint32_t reg, uint8_t v)
{
    reg_wr((uint32_t)bank << 16 | (reg & 0xFFFF), v);
}

/*
 * @routine 03:EB0E VideoRegsReset
 * WHAT IT DOES: Puts the PPU in a known state: mode 3 with the BG1/BG2 maps
 * at $7A/$5A and characters at $60, colour 0 black, windows, colour maths,
 * IRQ timers, interrupts and DMA all off; then resets the Super FX screen
 * base and buffer start.
 * HOW: Register stores in a fixed order. COLDATA is stored with X (16-bit,
 * so SETINI gets X's high byte) before SETINI is cleared. The buffer start
 * $2C00 goes to $70:021A and direct page $48.
 * Entry: X 16-bit, D = $0000. Exit: A = $2C00; P unchanged.
 */
void VideoRegsReset(uint8_t bank, uint16_t x)
{
    static const uint32_t zero[] = {TS, TMW, TSW, W12SEL, W34SEL, WOBJSEL, WBGLOG, WOBJLOG, CGWSEL, CGADSUB};
    static const uint32_t off[] = {HTIMEL, HTIMEH, VTIMEL, VTIMEH, NMITIMEN, MDMAEN, HDMAEN};
    unsigned i;

    cyc(106);
    io8(bank, BGMODE, 0x03);
    cyc(24);
    io8(bank, MOSAIC, 0x00);
    cyc(40);
    io8(bank, OBSEL, 0x03);
    cyc(40);
    io8(bank, BG1SC, 0x7A);
    cyc(40);
    io8(bank, BG2SC, 0x5A);
    cyc(40);
    io8(bank, BG12NBA, 0x60);
    cyc(24);
    io8(bank, CGADD, 0x00);
    cyc(24);
    io8(bank, CGDATA, 0x00);
    cyc(24);
    io8(bank, CGDATA, 0x00);
    for (i = 0; i < sizeof zero / sizeof zero[0]; i++)
        { cyc(24); io8(bank, zero[i], 0x00); }
    cyc(24);
    io8(bank, COLDATA, (uint8_t)x);
    io8(bank, SETINI, (uint8_t)(x >> 8));
    cyc(24);
    io8(bank, SETINI, 0x00);
    for (i = 0; i < sizeof off / sizeof off[0]; i++)
        { cyc(24); io8(bank, off[i], 0x00); }
    cyc(40);
    io8(bank, GSU_SCBR, 0x0B);
    cyc(78);
    wr16(0x70021A, 0x2C00);
    cyc(16);
    WR16(dp_48, 0x2C00);
    cyc(72);
}

void RaceHudLoad(Carried *cr);
void RaceScreenSetup_cr(Carried *cr);
void RaceVideoInit_cr(Carried *cr);
void RaceVideoSetup_cr(Carried *cr);
void VideoRegsReset_cr(Carried *cr);
void VideoStartUploads_cr(Carried *cr);
static void hud_tile(Carried *cr, uint32_t b, uint16_t from, uint16_t w, uint16_t vram);
static void pull_p(Carried *cr, uint8_t p);
static void race_video_exit(Carried *cr, int swapped, uint8_t p0, uint8_t db0, uint16_t a, uint16_t x, uint16_t y);
static void video_gsu_3000(Carried *cr, uint16_t a, uint16_t x, int x_first);
static void video_gsu_ram_dma(Carried *cr, uint16_t src, uint16_t vram, uint16_t size);
static void video_gsu_vram(Carried *cr, uint16_t y, uint16_t x);
static void video_mvn(Carried *cr, uint32_t src, uint32_t dst, uint16_t n);
void CourseSoundLoad_cr(Carried *cr);
void GsuCF42To3000_cr(Carried *cr);
void GsuCF42ToVram_cr(Carried *cr);
void RaceTimeInit_cr(Carried *cr);
void SoundIndicesFromHandlers_cr(Carried *cr);
void SoundReset_cr(Carried *cr);
void VramDmaTileFromGsuRam_cr(Carried *cr);

void VideoRegsReset_cr(Carried *cr)
{
    VideoRegsReset(cr->db, cr->x);
    cr->a = 0x2C00;
}

void abi_VideoRegsReset(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    VideoRegsReset_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:F32A RaceScreenSetup
 * WHAT IT DOES: Sets up the race screen: the Super FX buffer selection, the
 * flags at $7F:3411-$7F:3431, HDMA channel 1 (from $7F:3410 to WH0-WH1 via
 * mode 1) and channel 2 (the scroll table at $10AA to BG1VOFS via mode 2,
 * which it fills from $0E33 and $0E37), the windows and colour maths, BG
 * mode $0B with its maps, colour 1 black and every BG scroll at 0.
 * HOW: Straight-line stores in a fixed order; work RAM below $2000 and the
 * registers are reached through the data bank.
 * Entry: D = $0000. Exit: A = $1001; X, Y high bytes clear; M 8-bit, X
 * 8-bit; N, Z clear.
 */
void RaceScreenSetup(uint8_t bank)
{
    static const uint16_t ones[10] = {0x3411, 0x3414, 0x3416, 0x3419, 0x341C, 0x3425, 0x3428, 0x342B, 0x342E, 0x342F};
    static const uint16_t zeros[4] = {0x3412, 0x3415, 0x3430, 0x3431};
    static const uint32_t scroll[6] = {BG1HOFS, BG1VOFS, BG2HOFS, BG2VOFS, BG3HOFS, BG3VOFS};
    uint8_t lo, hi;
    unsigned i;

    cyc(70);
    WR16(gsu_buffer_select, 0x0000);
    cyc(48);
    WR16(wram_0D81, 0xFFFF);
    cyc(38);
    for (i = 0; i < 10; i++)
        { cyc(32); wr8(W(object_buckets) | ones[i], 0x01); }
    cyc(16);
    for (i = 0; i < 4; i++)
        { cyc(32); wr8(W(object_buckets) | zeros[i], 0x00); }
    cyc(24);
    WR8(colour_math_mode, 0x00);
    cyc(40);
    io8(bank, DMAP1, 0x01);
    cyc(40);
    io8(bank, BBAD1, 0x26);
    cyc(40);
    io8(bank, A1B1, 0x7F);
    cyc(70);
    io8(bank, A1T1L, 0x10);
    io8(bank, A1T1H, 0x34);
    cyc(62);
    io8(bank, WOBJSEL, 0x30);
    cyc(40);
    io8(bank, TMW, 0x07);
    cyc(40);
    io8(bank, TM, 0x02);
    cyc(40);
    io8(bank, W34SEL, 0x03);
    cyc(40);
    io8(bank, CGWSEL, 0x20);

    /* The scroll HDMA table: $38 lines of $0E33, $4F lines of $0E37, then
       three single lines of $0E37 and the end. */
    cyc(62);
    WR8(scroll_hdma, 0x38);
    cyc(24);
    WR8(wram_10AB, cyc_then(24, RD8(wram_0E33)));
    cyc(24);
    WR8(wram_10AC, cyc_then(24, RD8(wram_0E34)));
    cyc(40);
    WR8(wram_10AD, 0x4F);
    for (i = 0; i < 4; i++) {
        cyc(24);
        lo = RD8(scroll_value);
        cyc(24);
        WR8I(scroll_hdma, 4 + 3 * i, lo);
        cyc(24);
        hi = RD8(wram_0E38);
        cyc(24);
        WR8I(scroll_hdma, 5 + 3 * i, hi);
        cyc(40);
        WR8I(scroll_hdma, 6 + 3 * i, i < 3 ? 0x01 : 0x00);
    }
    cyc(40);
    io8(bank, DMAP2, 0x02);
    cyc(40);
    io8(bank, BBAD2, 0x10);
    cyc(40);
    io8(bank, A1B2, 0x00);
    cyc(70);
    io8(bank, A1T2L, 0xAA);
    io8(bank, A1T2H, 0x10);

    cyc(62);
    WR8(wram_0D45, 0x02);
    cyc(40);
    WR8(wram_0E91, 0xF0);
    cyc(24);
    WR8(wram_0E92, 0xF0);
    cyc(40);
    io8(bank, BGMODE, 0x0B);
    cyc(40);
    io8(bank, OBSEL, 0x03);
    cyc(40);
    io8(bank, BG1SC, 0x00);
    cyc(40);
    io8(bank, BG2SC, 0x52);
    cyc(40);
    io8(bank, BG12NBA, 0x20);
    cyc(40);
    io8(bank, TM, 0x12);
    cyc(40);
    io8(bank, CGADD, 0x01);
    cyc(24);
    io8(bank, CGDATA, 0x00);
    cyc(24);
    io8(bank, CGDATA, 0x00);
    for (i = 0; i < 6; i++) {
        cyc(24);
        io8(bank, scroll[i], 0x00);
        cyc(24);
        io8(bank, scroll[i], 0x00);
    }
    cyc(42);
}

void RaceScreenSetup_cr(Carried *cr)
{
    RaceScreenSetup(cr->db);
    cr->a = 0x1001;
    cr->x &= 0x00FF;
    cr->y &= 0x00FF;
    set_p(cr, (uint8_t)((cr->p & ~(P_N | P_Z)) | P_M | P_X));
}

void abi_RaceScreenSetup(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RaceScreenSetup_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void DmaFromDescriptor(uint8_t bank, uint16_t desc);

/*
 * @routine 03:EC6D VideoStartUploads
 * WHAT IT DOES: With interrupts off, clears direct page $00, acknowledges the
 * Super FX, sets the S-CPU's screen mode value ($0374) to 3 and runs the two
 * DMA transfers described at $F873 (to VRAM $0000) and $F87A (to $7C00).
 * HOW: NMITIMEN = 0 and SEI; reads $1073 and the Super FX status (16-bit,
 * so $3031 too); VMAIN = $80; DmaFromDescriptor twice. P (with the
 * interrupt flag) is restored on return.
 * Entry: any widths, D = $0000. Exit: A = $F801; X = $80; Y = $FF; P
 * unchanged.
 */
void VideoStartUploads(uint8_t bank)
{
    uint32_t b = (uint32_t)bank << 16;

    cyc(82);
    io8(bank, NMITIMEN, 0x00);
    cyc(16);
    WR8(dp_00, 0x00);
    cyc(24);
    RD8(wram_1073);
    cyc(54);
    hw_rd16(GSU_SFR);
    cyc(48);
    WR16(gsu_scmr_shadow, 0x0003);
    cyc(40);
    io8(bank, VMAIN, 0x80);
    cyc(48);
    ppu_wr16(b | (VMADDL & 0xFFFF), 0x0000);
    cyc(70);
    DmaFromDescriptor(bank, 0xF873);
    cyc(48);
    ppu_wr16(b | (VMADDL & 0xFFFF), 0x7C00);
    cyc(70);
    DmaFromDescriptor(bank, 0xF87A);
    cyc(72);
}

void VideoStartUploads_cr(Carried *cr)
{
    VideoStartUploads(cr->db);
    cr->a = 0xF801;
    cr->x = 0x0080;
    cr->y = 0x00FF;
}

void abi_VideoStartUploads(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    VideoStartUploads_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void abi_SoundIndicesFromHandlers(void);
void abi_GsuCF42ToVram(void);
void abi_GsuCF42To3000(void);

/* REP #$30, LDY, LDX, JSL GsuCF42ToVram. */
static void video_gsu_vram(Carried *cr, uint16_t y, uint16_t x)
{
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~(P_M | P_X))));
    cr->y = y;
    cr->x = x;
    set_p(cr, nz16(cr->p, x));
    (cyc(116), GsuCF42ToVram_cr(cr));
}

/* REP #$30, LDA and LDX (x_last: LDX first), JSL GsuCF42To3000. */
static void video_gsu_3000(Carried *cr, uint16_t a, uint16_t x, int x_first)
{
    cyc(28);
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~(P_M | P_X))));
    cr->a = a;
    cr->x = x;
    set_p(cr, nz16(cr->p, x_first ? a : (cyc(40), x)));
    (cyc(46), GsuCF42To3000_cr(cr));
}

/* PHP, REP #$20: DMA channel 0 in mode 1 of `size` bytes from $70:src to
   VRAM `vram` (VMAIN $80), started at once; PLP. */
static void video_gsu_ram_dma(Carried *cr, uint16_t src, uint16_t vram, uint16_t size)
{
    uint8_t p = cr->p;

    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cyc(80);
    dma_wr16(DMAP0, 0x1801);
    cyc(56);
    dma_wr16(A1T0L, src);
    cyc(56);
    ppu_wr16(VMADDL, vram);
    cr->a = size;
    cyc(56);
    dma_wr16(DAS0L, size);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cyc(70);
    ppu_wr(VMAIN, 0x80);
    cyc(48);
    dma_wr(A1B0, 0x70);
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0x01);
    set_p(cr, nz8(cr->p, 0x01));
    cyc(48);
    mdmaen_wr(MDMAEN, 0x01);
    r_plp(cr, p);
    cyc(28);
}

/* MVN within bank $7F or from $70 to $7F: n bytes from src to dst, a
   byte at a time (read, then write), with M and X 16-bit and P restored
   after (the caller's PHB, PHP, REP #$30 ... PLP, PLB). */
static void video_mvn(Carried *cr, uint32_t src, uint32_t dst, uint16_t n)
{
    uint8_t p = cr->p;
    unsigned i;

    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~(P_M | P_X))));
    cyc(60);
    for (i = 0; i < n; i++)
        { cyc(36); wr8(dst + i, rd8(src + i)); }
    cr->a = 0xFFFF;
    cr->x = (uint16_t)(src + n);
    cr->y = (uint16_t)(dst + n);
    r_plp(cr, p);
}

uint8_t SoundIndicesFromHandlers(uint8_t bank, uint8_t *x);
void GsuCF42ToVram(uint8_t bank, uint16_t in, uint16_t out);
uint8_t GsuCF42To3000(uint16_t in68, uint16_t in6a);
uint16_t PaletteCourseLoad(uint8_t bank, uint16_t *xo, uint16_t *yo, uint8_t *p);
void CgramDmaAll(void);
uint16_t GradientBuildColours(uint16_t x, int *carry);
uint16_t CoursePictureToVram(uint8_t bank);
void ScreenFrameAndPictures(uint8_t bank);
uint16_t TilemapRowFromGsuRam(uint8_t bank, uint16_t a, uint16_t *x, uint16_t *y, uint8_t *p);
uint16_t MenuCourseGraphics(uint8_t bank);

/* REP #$30, LDY, LDX, JSL GsuCF42ToVram. */
static void rv_gsu_vram(uint16_t out, uint16_t in)
{
    cyc(116);
    GsuCF42ToVram(0x03, in, out);
}

/* REP #$30, LDA and LDX (x_first: LDA first), JSL GsuCF42To3000; the
   typed call states 98 of the clocks after it. */
static void rv_gsu_3000(uint16_t a, uint16_t x, int x_first)
{
    cyc(28);
    if (!x_first)
        cyc(40);
    cyc(46);
    GsuCF42To3000(a, x);
}

/* PHP, REP #$20: DMA channel 0 in mode 1 of `size` bytes from $70:src to
   VRAM `vram` (VMAIN $80), started at once; PLP. */
static void rv_gsu_ram_dma(uint16_t src, uint16_t vram, uint16_t size)
{
    cyc(80);
    dma_wr16(DMAP0, 0x1801);
    cyc(56);
    dma_wr16(A1T0L, src);
    cyc(56);
    ppu_wr16(VMADDL, vram);
    cyc(56);
    dma_wr16(DAS0L, size);
    cyc(70);
    ppu_wr(VMAIN, 0x80);
    cyc(48);
    dma_wr(A1B0, 0x70);
    cyc(48);
    mdmaen_wr(MDMAEN, 0x01);
    cyc(42 + 28);
}

/* The course tilemap from the Super FX picture into $7F:93D7 + y0 (lead:
   the clocks before the first store): rows from TilemapRowFromGsuRam
   (sources 0, $800, $1000, $1800 and 0 again), direct page $F1 up by $40
   to $800. Returns A; X and Y as the last row leaves them. */
static uint16_t rv_tilemap_rows(uint16_t y0, uint32_t lead, uint16_t *x, uint16_t *y)
{
    static const uint16_t src[5] = {0x0000, 0x0800, 0x1000, 0x1800, 0x0000};
    uint16_t a;
    uint8_t p = 0;
    unsigned i;

    *y = y0;
    cyc(lead);
    WR16(tilemap_row_offset, 0x0000);
    do {
        for (i = 0; i < 5; i++)
            (cyc(70), TilemapRowFromGsuRam(0x7F, src[i], x, y, &p));
        cyc(16);
        a = (uint16_t)(RD16(tilemap_row_offset) + 0x0040);
        cyc(54);
        WR16(tilemap_row_offset, a);
    } while (cyc_loop(46, a < 0x0800));
    cyc(112);
    return a;
}

/* MVN of n bytes from src to dst, a byte at a time (read, then write),
   with the caller's PHB, PHP, REP #$30 ... PLP. */
static void rv_mvn(uint32_t src, uint32_t dst, uint16_t n)
{
    unsigned i;

    cyc(60);
    for (i = 0; i < n; i++)
        { cyc(36); wr8(dst + i, rd8(src + i)); }
    cyc(42);
}

/*
 * @routine 03:D9B9 RaceVideoSetup
 * WHAT IT DOES: Sets up the screen for the course in $0E3D: resets the PPU
 * (VideoRegsReset), starts the uploads (VideoStartUploads), sets the sound
 * indices, has the Super FX draw the course's pictures into VRAM, loads the
 * course palettes (PaletteCourseLoad, then CgramDmaAll) and gradient colours.
 * Then, unless $0D62 is set: the course's pictures, the HUD graphics for the
 * HUD mode $0D2B, the framed panels (ScreenFrameAndPictures) and the course
 * tilemap built from the Super FX picture into $7F:93D7 (and a second one
 * into $7F:BBD7 when the course has one). With $0D62 set: the menu's course
 * graphics (MenuCourseGraphics), $7F:9BD7 cleared ($2001 bytes), the Super
 * FX picture copied to $7F:93D7 ($800 bytes) and two DMA transfers of it to
 * VRAM $7C00 and $6600.
 * HOW: Data bank $03 for the course tables (three bytes per course: $F4F9,
 * $F54D, $F5A1, $F5F5; $F807 is read and not used). HUD modes 0-3 go through
 * the table at $03:DA53: mode 0 (and 3 with $10CD set) draws $9228 and $FAE8
 * and copies $80 words of the picture row for $0D8B's low nibble to VRAM
 * $5B40; mode 1 (unless $0D4E is set, which goes on as mode 3) draws $FA38,
 * sends $180 bytes to VRAM $5AC0 and draws $FB90; mode 3 does the same with
 * $100 bytes from $70:3180 and $FC20; mode 2 draws nothing more. Tilemap
 * rows come from TilemapRowFromGsuRam, five per step of $40 in direct page
 * $F1 up to $800. The two block copies are MVN (byte by byte, read then
 * write). The $0D62 path ends with its pulls swapped (PLP takes the data
 * bank byte, PLB the status byte), so it returns with P = the caller's data
 * bank and the data bank = the caller's P. Mode 3's own drawing, the third
 * course picture and the second tilemap are not reached in the traces so
 * far.
 * Entry: D = $0000; X as VideoRegsReset takes it. Exit: A = 0 (the second
 * tilemap: $800); X = $800; Y = course * 3 (the second tilemap: $5000); P
 * and the data bank unchanged. With $0D62 set: A = $0401; X = $CF42; Y =
 * $9BD7; P and the data bank swapped as above.
 * From C: x = X at entry; returns 1 on the $0D62 path (P and the data bank
 * swapped), with A, X, Y in *a, *x, *y.
 */
int RaceVideoSetup(uint16_t x_in, uint16_t *a, uint16_t *x, uint16_t *y)
{
    const uint32_t b = 0x030000;
    uint16_t k3, w, ra, rx, ry;
    uint8_t v, p = 0, x8;
    int carry;

    (cyc(200), VideoRegsReset(0x03, x_in));
    (cyc(62), VideoStartUploads(0x03));
    (cyc(62), SoundIndicesFromHandlers(0x03, &x8));
    cyc(16);
    rv_gsu_vram(0xF80D, 0xF7DA);
    cyc(46);
    w = (uint16_t)(RD16(course) << 1);
    cyc(52);
    k3 = (uint16_t)((w + RD16(course)) & 0x00FF);
    (cyc(100), PaletteCourseLoad(0x03, &rx, &ry, &p));
    cyc(64);
    (cyc(62), CgramDmaAll());
    cyc(268);
    rd16(b + 0xF807 + k3);                          /* read and not used */
    GradientBuildColours(k3, &carry);
    cyc(104 + 46);
    if (RD8(wram_0D62)) {
        cyc(138);
        MenuCourseGraphics(0x03);
        cyc(112 + 122);
        WR16(tilemap_7f9bd7, 0x0000);
        rv_mvn(W(tilemap_7f9bd7), W(wram_7F9BD8), 0x2000);
        cyc(42);                                    /* PLP */
        rv_gsu_vram(0xF84C, 0xF7E3);
        cyc(88);
        rv_mvn(0x703000, W(tilemap_7f93d7), 0x0800);
        cyc(44);
        rv_gsu_3000(0xC770, 0x0018, 0);
        cyc(158 - 98);
        rv_gsu_ram_dma(0x3000, 0x7C00, 0x0800);
        rv_gsu_ram_dma(0x3600, 0x6600, 0x0400);
        cyc(16 + 42);                               /* ..., PLP */
        *a = 0x0401;
        *x = 0xCF42;
        *y = 0x9BD7;
        return 1;
    }
    cyc(248);
    GsuCF42ToVram(0x03, (uint16_t)(k3 + 0xF4F9), 0xF853);
    cyc(156);
    if (rd16(b + 0xF54D + k3))
        { cyc(146); GsuCF42ToVram(0x03, (uint16_t)(k3 + 0xF54D), 0xF85A); }
    cyc(16);
    rv_gsu_vram(0xF814, 0xF7DD);
    cyc(18);
    rv_gsu_3000(0xD970, 0x001D, 0);
    cyc(62 + 98 - 98);                              /* JSL; CoursePictureToVram's PHP ... */
    ra = CoursePictureToVram(0x03);
    cyc(22 + 24);                                   /* SEP #$30, LDA */
    v = (uint8_t)(RD8(hud_mode) << 1);
    cyc(66);
    w = rd16(b | (uint16_t)(0xDA53 + v));
    if (w == 0xDB23)
        goto mode0;
    if (w == 0xDA5B)
        goto mode3;
    if (w == 0xDB66)
        { cyc(8); goto mode2; }
    if (w != 0xDAC0) {
        *a = (uint16_t)((ra & 0xFF00) | v);
        *x = v;
        *y = 0x00FF;
        goto done;
    }
    cyc(54);
    if (!RD8(hud_blank)) {
        cyc(32);
        rv_gsu_vram(0xF830, 0xF7D4);
        cyc(58);
        rv_gsu_3000(0xFA38, 0x0018, 1);
        cyc(158 - 98);
        rv_gsu_ram_dma(0x3000, 0x5AC0, 0x0180);
        cyc(16);
        rv_gsu_3000(0xFB90, 0x0018, 1);
        cyc(138 - 98);                              /* the Super FX call's return, BRA */
        goto mode2;
    } else { cyc(14); }
mode3:
    cyc(32);
    v = RD8(wram_10CD);
    cyc(38);
    if (!v) {
        rv_gsu_vram(0xF830, 0xF7D4);
        cyc(58);
        rv_gsu_3000(0xFA38, 0x0018, 1);
        cyc(158 - 98);
        rv_gsu_ram_dma(0x3180, 0x5AC0, 0x0100);
        cyc(16);
        rv_gsu_3000(0xFC20, 0x0018, 1);
        cyc(140 - 98);                              /* the Super FX call's return, JMP */
        goto mode2;
    }
mode0:
    cyc(24);
    rv_gsu_vram(0xF829, 0xF7D1);
    cyc(18);
    rv_gsu_3000(0x9228, 0x001D, 0);
    cyc(162 - 98);
    rx = (uint16_t)((RD16(wram_0D8B) & 0x000F) << 8);  /* XBA */
    cyc(130);
    ppu_wr16(b | 0x2116, 0x5B40);
    for (w = 0x0080; w; w--) {
        cyc(32);
        ra = rd16(0x703000 + rx);
        cyc(24);
        ppu_wr16(b | 0x2118, ra);
        rx = (uint16_t)(rx + 2);
        cyc(64);
    }
    cyc(52);
    rv_gsu_3000(0xFAE8, 0x0018, 1);
    cyc(116 - 98);                                  /* the Super FX call's return */
mode2:
    (cyc(68), ScreenFrameAndPictures(0x03));
    cyc(256);
    w = rd16(b + 0xF5A3 + k3);
    GsuCF42To3000(rd16(b + 0xF5A1 + k3), w);
    *a = rv_tilemap_rows(0x0000, 288 - 98, x, y);
    *y = k3;
    cyc(172);
    w = (uint16_t)(rd16(b + 0xF5F7 + k3) & 0x00FF);
    *a = w;
    if (w) {
        cyc(72);
        GsuCF42To3000(rd16(b + 0xF5F5 + k3), w);
        *a = rv_tilemap_rows(0x2800, 288 - 98, x, y);
    }
    cyc(36);
done:
    cyc(42);                                        /* PLP */
    return 0;
}

/* PLP's register effects (its time stated by the typed function). */
static void pull_p(Carried *cr, uint8_t p)
{
    set_p(cr, p);
    if (p & P_X) {
        cr->x &= 0x00FF;
        cr->y &= 0x00FF;
    }
}

/* RaceVideoSetup's exit from P p0 and data bank db0 at its entry. */
static void race_video_exit(Carried *cr, int swapped, uint8_t p0, uint8_t db0, uint16_t a, uint16_t x, uint16_t y)
{
    cr->a = a;
    cr->x = x;
    cr->y = y;
    if (swapped) {
        pull_p(cr, db0);
        cr->db = p0;
        set_p(cr, nz8(cr->p, p0));
    } else {
        cr->db = db0;
        pull_p(cr, p0);
    }
}

void RaceVideoSetup_cr(Carried *cr)
{
    uint8_t p0 = cr->p, db0 = cr->db;
    uint16_t a, x, y;
    int swapped = RaceVideoSetup(cr->x, &a, &x, &y);

    race_video_exit(cr, swapped, p0, db0, a, x, y);
}

void abi_RaceVideoSetup(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RaceVideoSetup_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

static uint8_t rvi_v;                   /* RaceVideoInit's $0D24 */

/*
 * @routine 03:D996 RaceVideoInit
 * WHAT IT DOES: RaceVideoSetup, first clearing $0EED-$0EEF and the words at
 * $0E6C and $0E6E when $0D24 is 3 or more.
 * HOW: Data bank $03 (the program bank) around the call, so the clears go to
 * work RAM through it.
 * Entry: M 8-bit, D = $0000. Exit: as RaceVideoSetup; the data bank
 * unchanged, N and Z from it (as RaceVideoSetup's swapped return leaves P
 * otherwise).
 * From C: as RaceVideoSetup.
 */
int RaceVideoInit(uint16_t x_in, uint16_t *a, uint16_t *x, uint16_t *y)
{
    int r;

    cyc(118);
    rvi_v = RD8(wram_0D24);
    if (rvi_v >= 0x03) {
        cyc(56);
        WR8(wram_0EED, 0x00);
        cyc(24);
        WR8(wram_0EEE, 0x00);
        cyc(24);
        WR8(wram_0EEF, 0x00);
        cyc(22 + 24);
        WR16(wram_0E6C, 0x0000);
        cyc(24);
        WR16(wram_0E6E, 0x0000);
    } else { cyc(38); }
    cyc(62);
    r = RaceVideoSetup(x_in, a, x, y);
    cyc(72);
    return r;
}

void RaceVideoInit_cr(Carried *cr)
{
    uint8_t db0 = cr->db, p1;
    uint16_t a, x, y;
    int swapped = RaceVideoInit(cr->x, &a, &x, &y);

    /* P as RaceVideoSetup found it: M set, N and Z from the compare of
       $0D24 with 3, C from it, M clear again on the clearing path. */
    p1 = (uint8_t)(nz8(cr->p, (uint8_t)(rvi_v - 0x03)) | P_M);
    p1 = (uint8_t)((p1 & ~P_C) | (rvi_v >= 0x03 ? P_C : 0));
    if (rvi_v >= 0x03)
        p1 &= (uint8_t)~P_M;
    race_video_exit(cr, swapped, p1, 0x03, a, x, y);
    cr->db = db0;
    set_p(cr, nz8(cr->p, db0));
}

void abi_RaceVideoInit(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RaceVideoInit_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void abi_SoundReset(void);
void abi_CourseSoundLoad(void);
void abi_VramDmaTileFromGsuRam(void);
void abi_RaceTimeInit(void);

/* LDA #from, STA $F1, LDA (a word of work RAM at w), LDX #vram, JSR
   VramDmaTileFromGsuRam: one tile of the Super FX picture to VRAM. */
static void hud_tile(Carried *cr, uint32_t b, uint16_t from, uint16_t w, uint16_t vram)
{
    cr->a = from;
    cyc(34);
    WR16(tilemap_row_offset, from);
    cyc(24);
    r_lda16(cr, b | w);
    cr->x = vram;
    set_p(cr, nz16(cr->p, vram));
    (cyc(70), VramDmaTileFromGsuRam_cr(cr));
    cyc(6);
}

void RaceHudSetup(Carried *cr, uint8_t p, uint8_t db);
void RaceHudSetupTwoPlayers(Carried *cr, uint8_t p, uint8_t db);

/*
 * @routine 03:DD1B RaceHudLoad
 * WHAT IT DOES: Loads a race's sound and HUD graphics: the sound reset, the
 * car $0D89 = 4 when $0D4C is set in HUD mode 0 (else the sounds chosen,
 * SoundIndicesFromHandlers), the course's sound (CourseSoundLoad); the
 * Super FX picture $1D:8078, and by HUD mode $0D2B (the table at $03:DD56)
 * tiles of it to VRAM: mode 0 the tiles $18E5, $18E7 and $18E9 (from
 * $30A0) at $62F0, $63E0 and $63F0, then as mode 1; mode 1 nothing more
 * unless $0D4E is set, which goes on as mode 3; mode 2 tile $18E5 (from
 * $3000) at $62F0; mode 3 $20 bytes from $70:3080 to $63F0. Then tile
 * $18E3 (from $3000) at $62E0, the Super FX blocks $F845/$F804 to VRAM
 * and $18:D920 drawn, $3000 bytes of it copied to $7F:63D7, the sound reset
 * again, the race time set up (RaceTimeInit), the Super FX's speed $70:0050
 * from the course's word at $03:F799; then on to RaceHudSetup, or
 * RaceHudSetupTwoPlayers with $0D62 set, which pull P and the data bank
 * pushed here.
 * HOW: P and the data bank ($03) pushed; MVN for the copy; the last part
 * is a jump (the C passes the two bytes). HUD modes above 3
 * would take code bytes as addresses and are not modelled.
 * Entry: any widths; D = $0000. Exit: as RaceHudSetup or
 * RaceHudSetupTwoPlayers leaves it.
 */
void RaceHudLoad(Carried *cr)
{
    cyc(178);
    const uint32_t b = 0x030000;
    uint8_t pushed[2] = {cr->p, cr->db};
    uint16_t w, x;

    r_sep(cr, P_M);
    cr->db = 0x03;
    set_p(cr, nz8(cr->p, 0x03));
    SoundReset_cr(cr);
    r_sep(cr, P_M);
    cyc(46);
    r_lda8(cr, WA(digit62_override));
    if ((uint8_t)cr->a && (r_lda8(cr, cyc_then(40, WA(hud_mode))), !(uint8_t)cr->a)) {
        cr->a = (uint16_t)((cr->a & 0xFF00) | 0x04);
        set_p(cr, nz8(cr->p, 0x04));
        cyc(56);
        WR8(wram_0D89, 0x04);
        cyc(22);                                /* BRA */
    } else
        { cyc(84); SoundIndicesFromHandlers_cr(cr); }
    (cyc(62), CourseSoundLoad_cr(cr));
    cyc(18);
    video_gsu_3000(cr, 0x8078, 0x001D, 0);
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~(P_M | P_X))));
    cyc(162);
    w = RD16(hud_mode);
    set_p(cr, (uint8_t)((cr->p & ~P_C) | (w >> 15)));
    x = (uint16_t)(w << 1);
    cr->a = x;
    cr->x = x;
    set_p(cr, nz16(cr->p, x));
    cyc(46);
    w = rd16(b | (uint16_t)(0xDD56 + x));
    if (w == 0xDDA4) {
        cyc(34);
        hud_tile(cr, b, 0x30A0, W(wram_18E5), 0x62F0);
        cyc(0);
        hud_tile(cr, b, 0x30A0, W(wram_18E7), 0x63E0);
        cyc(0);
        hud_tile(cr, b, 0x30A0, W(wram_18E9), 0x63F0);
        w = 0xDDCE;
    } else { cyc(34); }
    cyc(0);
    if (w == 0xDDCE) {
        r_sep(cr, P_M);
        cyc(40);
        r_lda8(cr, WA(hud_blank));
        if ((uint8_t)cr->a)
            w = 0xDD5E;
    }
    if (w == 0xDD94)
        hud_tile(cr, b, 0x3000, W(wram_18E5), 0x62F0);
    cyc(0);
    if (w == 0xDD5E)
        { cyc(14); video_gsu_ram_dma(cr, 0x3080, 0x63F0, 0x0020); } else { cyc(36); }
    cyc(8);
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~(P_M | P_X))));
    hud_tile(cr, b, 0x3000, W(selection_derived), 0x62E0);
    cyc(10);
    video_gsu_vram(cr, 0xF845, 0xF804);
    cyc(18);
    video_gsu_3000(cr, 0xD920, 0x0018, 0);
    cyc(182);
    video_mvn(cr, 0x703000, W(wram_7F63D7), 0x3000);
    r_sep(cr, P_M);
    (cyc(110), SoundReset_cr(cr));
    r_sep(cr, P_M);
    (cyc(68), RaceTimeInit_cr(cr));
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~(P_M | P_X))));
    cyc(46);
    x = (uint16_t)((RD16(course) & 0x00FF) << 1);
    cr->x = x;
    cyc(132);
    cr->a = rd16(0x03F799 + x);
    wr16(0x700050, cr->a);
    r_sep(cr, P_M);
    cyc(46);
    r_lda8(cr, WA(wram_0D62));
    if ((uint8_t)cr->a)
        { cyc(46); RaceHudSetupTwoPlayers(cr, pushed[0], pushed[1]); }
    else
        { cyc(46); RaceHudSetup(cr, pushed[0], pushed[1]); }
    cyc(72);
}

void abi_RaceHudLoad(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RaceHudLoad(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

