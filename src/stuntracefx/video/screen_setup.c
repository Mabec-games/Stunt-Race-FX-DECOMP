/*
 * screen_setup.c: PPU, window, HDMA and Super FX screen settings written
 * when a screen is set up. Register stores go through the data bank, which
 * the callers keep in the I/O banks.
 */
#include "machine.h"
#include "hw.h"
#include "cpu_ops.h"

static void io8(uint8_t bank, uint32_t reg, uint8_t v)
{
    wr8((uint32_t)bank << 16 | (reg & 0xFFFF), v);
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
    wr16(0x000048, 0x2C00);
    cyc(72);
}

void abi_VideoRegsReset(void)
{
    VideoRegsReset(cpu.db, cpu.x);
    cpu.a = 0x2C00;
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
    uint32_t b = (uint32_t)bank << 16;
    uint8_t lo, hi;
    unsigned i;

    cyc(70);
    wr16(b | 0x0D7F, 0x0000);
    cyc(48);
    wr16(b | 0x0D81, 0xFFFF);
    cyc(38);
    for (i = 0; i < 10; i++)
        { cyc(32); wr8(0x7F0000 | ones[i], 0x01); }
    cyc(16);
    for (i = 0; i < 4; i++)
        { cyc(32); wr8(0x7F0000 | zeros[i], 0x00); }
    cyc(24);
    wr8(b | 0x0D47, 0x00);
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
    wr8(b | 0x10AA, 0x38);
    cyc(24);
    wr8(b | 0x10AB, cyc_then(24, rd8(b | 0x0E33)));
    cyc(24);
    wr8(b | 0x10AC, cyc_then(24, rd8(b | 0x0E34)));
    cyc(40);
    wr8(b | 0x10AD, 0x4F);
    for (i = 0; i < 4; i++) {
        cyc(24);
        lo = rd8(b | 0x0E37);
        cyc(24);
        wr8(b | (uint16_t)(0x10AE + 3 * i), lo);
        cyc(24);
        hi = rd8(b | 0x0E38);
        cyc(24);
        wr8(b | (uint16_t)(0x10AF + 3 * i), hi);
        cyc(40);
        wr8(b | (uint16_t)(0x10B0 + 3 * i), i < 3 ? 0x01 : 0x00);
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
    wr8(b | 0x0D45, 0x02);
    cyc(40);
    wr8(b | 0x0E91, 0xF0);
    cyc(24);
    wr8(b | 0x0E92, 0xF0);
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

void abi_RaceScreenSetup(void)
{
    RaceScreenSetup(cpu.db);
    cpu.a = 0x1001;
    cpu.x &= 0x00FF;
    cpu.y &= 0x00FF;
    cpu.p = (uint8_t)((cpu.p & ~(P_N | P_Z)) | P_M | P_X);
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
    wr8(0x000000, 0x00);
    cyc(24);
    rd8(b | 0x1073);
    cyc(54);
    rd16(GSU_SFR);
    cyc(48);
    wr16(b | 0x0374, 0x0003);
    cyc(40);
    io8(bank, VMAIN, 0x80);
    cyc(48);
    wr16(b | (VMADDL & 0xFFFF), 0x0000);
    cyc(70);
    DmaFromDescriptor(bank, 0xF873);
    cyc(48);
    wr16(b | (VMADDL & 0xFFFF), 0x7C00);
    cyc(70);
    DmaFromDescriptor(bank, 0xF87A);
    cyc(72);
}

void abi_VideoStartUploads(void)
{
    VideoStartUploads(cpu.db);
    cpu.a = 0xF801;
    cpu.x = 0x0080;
    cpu.y = 0x00FF;
}

void abi_VideoRegsReset(void);
void abi_SoundIndicesFromHandlers(void);
void abi_GsuCF42ToVram(void);
void abi_GsuCF42To3000(void);
void abi_PaletteCourseLoad(void);
void abi_CgramDmaAll(void);
void abi_GradientBuildColours(void);
void abi_CoursePictureToVram(void);
void abi_ScreenFrameAndPictures(void);
void abi_TilemapRowFromGsuRam(void);
void abi_MenuCourseGraphics(void);

/* REP #$30, LDY, LDX, JSL GsuCF42ToVram. */
static void video_gsu_vram(uint16_t y, uint16_t x)
{
    cpu.p &= (uint8_t)~(P_M | P_X);
    cpu.y = y;
    cpu.x = x;
    cpu.p = nz16(cpu.p, x);
    (cyc(116), abi_GsuCF42ToVram());
}

/* REP #$30, LDA and LDX (x_last: LDX first), JSL GsuCF42To3000. */
static void video_gsu_3000(uint16_t a, uint16_t x, int x_first)
{
    cyc(28);
    cpu.p &= (uint8_t)~(P_M | P_X);
    cpu.a = a;
    cpu.x = x;
    cpu.p = nz16(cpu.p, x_first ? a : (cyc(40), x));
    (cyc(46), abi_GsuCF42To3000());
}

/* PHP, REP #$20: DMA channel 0 in mode 1 of `size` bytes from $70:src to
   VRAM `vram` (VMAIN $80), started at once; PLP. */
static void video_gsu_ram_dma(uint16_t src, uint16_t vram, uint16_t size)
{
    uint8_t p = cpu.p;

    cpu.p &= (uint8_t)~P_M;
    cyc(80);
    wr16(DMAP0, 0x1801);
    cyc(56);
    wr16(A1T0L, src);
    cyc(56);
    wr16(VMADDL, vram);
    cpu.a = size;
    cyc(56);
    wr16(DAS0L, size);
    cpu.p |= P_M;
    cyc(70);
    wr8(VMAIN, 0x80);
    cyc(48);
    wr8(A1B0, 0x70);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x01);
    cpu.p = nz8(cpu.p, 0x01);
    cyc(48);
    wr8(MDMAEN, 0x01);
    op_plp(p);
    cyc(28);
}

/* The data bank $7F (PHB, PHA, PLB), then the course tilemap from the
   Super FX picture: rows from TilemapRowFromGsuRam (sources 0, $800,
   $1000, $1800 and 0 again), direct page $F1 up by $40 to $800, Y from
   y0. */
static void video_tilemap_rows(uint16_t y0)
{
    static const uint16_t src[5] = {0x0000, 0x0800, 0x1000, 0x1800, 0x0000};
    uint16_t a;
    unsigned i;

    op_sep(P_M);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x7F);
    cpu.db = 0x7F;
    cpu.p = nz8(cpu.p, 0x7F);
    cpu.p &= (uint8_t)~(P_M | P_X);
    cpu.y = y0;
    cpu.p = nz16(cpu.p, y0);
    cyc(288);
    wr16(0x0000F1, 0x0000);
    do {
        for (i = 0; i < 5; i++) {
            cpu.a = src[i];
            cpu.p = nz16(cpu.p, src[i]);
            (cyc(70), abi_TilemapRowFromGsuRam());
        }
        cyc(16);
        a = rd16(0x0000F1);
        cpu.p &= (uint8_t)~P_C;
        a = op_adc16(a, 0x0040, &cpu.p);
        cyc(54);
        wr16(0x0000F1, a);
        cpu.a = a;
        op_cmp16(0x0800);
    } while (cyc_loop(46, !(cpu.p & P_C)));
    cyc(112);
}

/* MVN within bank $7F or from $70 to $7F: n bytes from src to dst, a
   byte at a time (read, then write), with M and X 16-bit and P restored
   after (the caller's PHB, PHP, REP #$30 ... PLP, PLB). */
static void video_mvn(uint32_t src, uint32_t dst, uint16_t n)
{
    uint8_t p = cpu.p;
    unsigned i;

    cpu.p &= (uint8_t)~(P_M | P_X);
    cyc(60);
    for (i = 0; i < n; i++)
        { cyc(36); wr8(dst + i, rd8(src + i)); }
    cpu.a = 0xFFFF;
    cpu.x = (uint16_t)(src + n);
    cpu.y = (uint16_t)(dst + n);
    op_plp(p);
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
 */
void abi_RaceVideoSetup(void)
{
    const uint32_t b = 0x030000;
    uint8_t p0 = cpu.p, db0 = cpu.db, v;
    uint16_t k3, a, w, y;

    op_sep(P_M);
    cpu.db = 0x03;
    cpu.p = nz8(cpu.p, 0x03);
    cpu.p &= (uint8_t)~P_M;
    (cyc(200), abi_VideoRegsReset());
    (cyc(62), abi_VideoStartUploads());
    (cyc(62), abi_SoundIndicesFromHandlers());
    cyc(16);
    video_gsu_vram(0xF80D, 0xF7DA);
    cpu.p &= (uint8_t)~(P_M | P_X);
    cyc(46);
    a = rd16(b | 0x0E3D);
    cpu.p = (uint8_t)((cpu.p & ~P_C) | (a >> 15));
    a = (uint16_t)(a << 1);
    cpu.p &= (uint8_t)~P_C;
    cyc(52);
    a = op_adc16(a, rd16(b | 0x0E3D), &cpu.p);
    k3 = (uint16_t)(a & 0x00FF);
    cpu.a = k3;
    cpu.p = nz16(cpu.p, k3);
    (cyc(100), abi_PaletteCourseLoad());
    (cyc(62), abi_CgramDmaAll());
    cpu.p &= (uint8_t)~(P_M | P_X);
    cpu.x = k3;
    cyc(268);
    op_lda16(b + 0xF807 + k3);
    op_sep(P_M);
    cpu.x = k3;
    cpu.p = nz16(cpu.p, k3);
    abi_GradientBuildColours();
    op_sep(P_M);
    cyc(46);
    op_lda8(b | 0x0D62);
    if ((uint8_t)cpu.a) {
        cyc(138);
        cpu.y = k3;
        cpu.p = nz16(cpu.p, k3);
        abi_MenuCourseGraphics();
        {
            uint8_t p1 = cpu.p;

            cpu.p &= (uint8_t)~(P_M | P_X);
            cpu.a = 0x0000;
            cyc(122);
            wr16(0x7F9BD7, 0x0000);
            video_mvn(0x7F9BD7, 0x7F9BD8, 0x2000);
            op_plp(p1);
        }
        video_gsu_vram(0xF84C, 0xF7E3);
        cyc(88);
        video_mvn(0x703000, 0x7F93D7, 0x0800);
        cyc(44);
        video_gsu_3000(0xC770, 0x0018, 0);
        cpu.p &= (uint8_t)~(P_M | P_X);
        cyc(158);
        video_gsu_ram_dma(0x3000, 0x7C00, 0x0800);
        cpu.p &= (uint8_t)~(P_M | P_X);
        video_gsu_ram_dma(0x3600, 0x6600, 0x0400);
        op_plp(db0);
        cpu.db = p0;
        cpu.p = nz8(cpu.p, p0);
        return;
    } else { cyc(248); }
    cpu.p &= (uint8_t)~(P_M | P_X);
    cpu.p &= (uint8_t)~P_C;
    cpu.a = op_adc16(k3, 0xF4F9, &cpu.p);
    cpu.x = cpu.a;
    cpu.y = 0xF853;
    cpu.p = nz16(cpu.p, 0xF853);
    abi_GsuCF42ToVram();
    cpu.p &= (uint8_t)~(P_M | P_X);
    cpu.x = k3;
    cyc(156);
    op_lda16(b + 0xF54D + k3);
    if (cpu.a) {
        cyc(146);
        cpu.p &= (uint8_t)~P_C;
        cpu.a = op_adc16(k3, 0xF54D, &cpu.p);
        cpu.x = cpu.a;
        cpu.y = 0xF85A;
        cpu.p = nz16(cpu.p, 0xF85A);
        abi_GsuCF42ToVram();
    }
    cyc(16);
    video_gsu_vram(0xF814, 0xF7DD);
    cyc(18);
    video_gsu_3000(0xD970, 0x001D, 0);
    (cyc(62), abi_CoursePictureToVram());
    op_sep(P_M | P_X);
    cyc(24);
    v = rd8(b | 0x0D2B);
    cpu.p = (uint8_t)((cpu.p & ~P_C) | (v >> 7));
    v = (uint8_t)(v << 1);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.x = v;
    cpu.p = nz8(cpu.p, v);
    cyc(66);
    w = rd16(b | (uint16_t)(0xDA53 + v));
    if (w == 0xDB23)
        goto mode0;
    if (w == 0xDA5B)
        goto mode3;
    if (w == 0xDB66)
        goto mode2;
    if (w != 0xDAC0)
        goto done;
    op_sep(P_M);
    cyc(54);
    op_lda8(b | 0x0D4E);
    if (!(uint8_t)cpu.a) {
        cyc(32);
        video_gsu_vram(0xF830, 0xF7D4);
        cyc(58);
        video_gsu_3000(0xFA38, 0x0018, 1);
        cpu.p &= (uint8_t)~(P_M | P_X);
        cyc(158);
        video_gsu_ram_dma(0x3000, 0x5AC0, 0x0180);
        cyc(16);
        video_gsu_3000(0xFB90, 0x0018, 1);
        cyc(22);                                /* BRA */
        goto mode2;
    } else { cyc(14); }
mode3:
    cyc(32);
    op_lda8(b | 0x10CD);
    cyc(38);
    if (!(uint8_t)cpu.a) {
        video_gsu_vram(0xF830, 0xF7D4);
        cyc(58);
        video_gsu_3000(0xFA38, 0x0018, 1);
        cpu.p &= (uint8_t)~(P_M | P_X);
        cyc(130);
        video_gsu_ram_dma(0x3180, 0x5AC0, 0x0100);
        video_gsu_3000(0xFC20, 0x0018, 1);
        goto mode2;
    }
mode0:
    cyc(24);
    video_gsu_vram(0xF829, 0xF7D1);
    cyc(18);
    video_gsu_3000(0x9228, 0x001D, 0);
    cpu.p &= (uint8_t)~(P_M | P_X);
    cyc(162);
    cpu.a = (uint16_t)(rd16(b | 0x0D8B) & 0x000F);
    op_xba();
    cpu.x = cpu.a;
    cpu.p = nz16(cpu.p, cpu.x);
    cpu.a = 0x5B40;
    cyc(130);
    wr16(b | 0x2116, 0x5B40);
    for (y = 0x0080; y; y--) {
        cyc(32);
        cpu.a = rd16(0x703000 + cpu.x);
        cyc(24);
        wr16(b | 0x2118, cpu.a);
        cpu.x = (uint16_t)(cpu.x + 2);
        cyc(64);
    }
    cyc(52);
    cpu.y = 0x0000;
    cpu.p = (uint8_t)((cpu.p & ~P_N) | P_Z);
    video_gsu_3000(0xFAE8, 0x0018, 1);
mode2:
    cpu.p &= (uint8_t)~(P_M | P_X);
    (cyc(90), abi_ScreenFrameAndPictures());
    cpu.p &= (uint8_t)~(P_M | P_X);
    cpu.y = k3;
    cyc(256);
    cpu.a = rd16(b + 0xF5A3 + k3);
    cpu.x = cpu.a;
    op_lda16(b + 0xF5A1 + k3);
    abi_GsuCF42To3000();
    video_tilemap_rows(0x0000);
    op_sep(P_M);
    cpu.db = 0x03;
    cpu.p = nz8(cpu.p, 0x03);
    cpu.p &= (uint8_t)~(P_M | P_X);
    cpu.y = k3;
    cyc(172);
    cpu.a = (uint16_t)(rd16(b + 0xF5F7 + k3) & 0x00FF);
    cpu.p = nz16(cpu.p, cpu.a);
    if (cpu.a) {
        cyc(72);
        cpu.x = cpu.a;
        op_lda16(b + 0xF5F5 + k3);
        abi_GsuCF42To3000();
        video_tilemap_rows(0x2800);
        cpu.db = 0x03;
        cpu.p = nz8(cpu.p, 0x03);
    }
cyc(36);
done:
    op_sep(P_M);
    cpu.db = db0;
    cpu.p = nz8(cpu.p, db0);
    op_plp(p0);
}

/*
 * @routine 03:D996 RaceVideoInit
 * WHAT IT DOES: RaceVideoSetup, first clearing $0EED-$0EEF and the words at
 * $0E6C and $0E6E when $0D24 is 3 or more.
 * HOW: Data bank $03 (the program bank) around the call, so the clears go to
 * work RAM through it.
 * Entry: M 8-bit, D = $0000. Exit: as RaceVideoSetup; the data bank
 * unchanged, N and Z from it (as RaceVideoSetup's swapped return leaves P
 * otherwise).
 */
void abi_RaceVideoInit(void)
{
    const uint32_t b = 0x030000;
    uint8_t db0 = cpu.db, v;

    cpu.db = 0x03;
    cpu.p = nz8(cpu.p, 0x03);
    op_sep(P_M);
    cyc(118);
    op_lda8(b | 0x0D24);
    v = (uint8_t)cpu.a;
    cpu.p = (uint8_t)((nz8(cpu.p, (uint8_t)(v - 0x03)) & ~P_C) | (v >= 0x03 ? P_C : (cyc(38), 0)));
    if (v >= 0x03) {
        cyc(56);
        wr8(b | 0x0EED, 0x00);
        cyc(24);
        wr8(b | 0x0EEE, 0x00);
        cyc(24);
        wr8(b | 0x0EEF, 0x00);
        op_rep(P_M);
        cyc(24);
        wr16(b | 0x0E6C, 0x0000);
        cyc(24);
        wr16(b | 0x0E6E, 0x0000);
    }
    cyc(62);
    abi_RaceVideoSetup();
    cpu.db = db0;
    cpu.p = nz8(cpu.p, db0);
    cyc(72);
}

void abi_SoundReset(void);
void abi_CourseSoundLoad(void);
void abi_VramDmaTileFromGsuRam(void);
void abi_RaceTimeInit(void);

/* LDA #from, STA $F1, LDA (a word of work RAM at w), LDX #vram, JSR
   VramDmaTileFromGsuRam: one tile of the Super FX picture to VRAM. */
static void hud_tile(uint32_t b, uint16_t from, uint16_t w, uint16_t vram)
{
    cpu.a = from;
    cyc(34);
    wr16(0x0000F1, from);
    cyc(24);
    op_lda16(b | w);
    cpu.x = vram;
    cpu.p = nz16(cpu.p, vram);
    (cyc(70), abi_VramDmaTileFromGsuRam());
    cyc(6);
}

void RaceHudSetup(uint8_t p, uint8_t db);
void RaceHudSetupTwoPlayers(uint8_t p, uint8_t db);

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
void abi_RaceHudLoad(void)
{
    cyc(178);
    const uint32_t b = 0x030000;
    uint8_t pushed[2] = {cpu.p, cpu.db};
    uint16_t w, x;

    op_sep(P_M);
    cpu.db = 0x03;
    cpu.p = nz8(cpu.p, 0x03);
    abi_SoundReset();
    op_sep(P_M);
    cyc(46);
    op_lda8(b | 0x0D4C);
    if ((uint8_t)cpu.a && (op_lda8(cyc_then(40, b | 0x0D2B)), !(uint8_t)cpu.a)) {
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x04);
        cpu.p = nz8(cpu.p, 0x04);
        cyc(56);
        wr8(b | 0x0D89, 0x04);
        cyc(22);                                /* BRA */
    } else
        { cyc(84); abi_SoundIndicesFromHandlers(); }
    abi_CourseSoundLoad();
    cyc(18);
    video_gsu_3000(0x8078, 0x001D, 0);
    cpu.p &= (uint8_t)~(P_M | P_X);
    cyc(162);
    w = rd16(b | 0x0D2B);
    cpu.p = (uint8_t)((cpu.p & ~P_C) | (w >> 15));
    x = (uint16_t)(w << 1);
    cpu.a = x;
    cpu.x = x;
    cpu.p = nz16(cpu.p, x);
    cyc(46);
    w = rd16(b | (uint16_t)(0xDD56 + x));
    if (w == 0xDDA4) {
        cyc(34);
        hud_tile(b, 0x30A0, 0x18E5, 0x62F0);
        cyc(0);
        hud_tile(b, 0x30A0, 0x18E7, 0x63E0);
        cyc(0);
        hud_tile(b, 0x30A0, 0x18E9, 0x63F0);
        w = 0xDDCE;
    } else { cyc(34); }
    cyc(0);
    if (w == 0xDDCE) {
        op_sep(P_M);
        cyc(40);
        op_lda8(b | 0x0D4E);
        if ((uint8_t)cpu.a)
            w = 0xDD5E;
    }
    if (w == 0xDD94)
        hud_tile(b, 0x3000, 0x18E5, 0x62F0);
    cyc(0);
    if (w == 0xDD5E)
        { cyc(14); video_gsu_ram_dma(0x3080, 0x63F0, 0x0020); } else { cyc(36); }
    cyc(8);
    cpu.p &= (uint8_t)~(P_M | P_X);
    hud_tile(b, 0x3000, 0x18E3, 0x62E0);
    cyc(10);
    video_gsu_vram(0xF845, 0xF804);
    cyc(18);
    video_gsu_3000(0xD920, 0x0018, 0);
    cyc(182);
    video_mvn(0x703000, 0x7F63D7, 0x3000);
    op_sep(P_M);
    (cyc(110), abi_SoundReset());
    op_sep(P_M);
    (cyc(68), abi_RaceTimeInit());
    cpu.p &= (uint8_t)~(P_M | P_X);
    cyc(46);
    x = (uint16_t)((rd16(b | 0x0E3D) & 0x00FF) << 1);
    cpu.x = x;
    cyc(132);
    cpu.a = rd16(0x03F799 + x);
    wr16(0x700050, cpu.a);
    op_sep(P_M);
    cyc(46);
    op_lda8(b | 0x0D62);
    if ((uint8_t)cpu.a)
        { cyc(46); RaceHudSetupTwoPlayers(pushed[0], pushed[1]); }
    else
        { cyc(46); RaceHudSetup(pushed[0], pushed[1]); }
    cyc(72);
}
