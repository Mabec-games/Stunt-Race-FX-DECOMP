/*
 * screen_setup.c: PPU, window, HDMA and Super FX screen settings written
 * when a screen is set up. Register stores go through the data bank, which
 * the callers keep in the I/O banks.
 */
#include "machine.h"
#include "hw.h"

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

    io8(bank, BGMODE, 0x03);
    io8(bank, MOSAIC, 0x00);
    io8(bank, OBSEL, 0x03);
    io8(bank, BG1SC, 0x7A);
    io8(bank, BG2SC, 0x5A);
    io8(bank, BG12NBA, 0x60);
    io8(bank, CGADD, 0x00);
    io8(bank, CGDATA, 0x00);
    io8(bank, CGDATA, 0x00);
    for (i = 0; i < sizeof zero / sizeof zero[0]; i++)
        io8(bank, zero[i], 0x00);
    io8(bank, COLDATA, (uint8_t)x);
    io8(bank, SETINI, (uint8_t)(x >> 8));
    io8(bank, SETINI, 0x00);
    for (i = 0; i < sizeof off / sizeof off[0]; i++)
        io8(bank, off[i], 0x00);
    io8(bank, GSU_SCBR, 0x0B);
    wr16(0x70021A, 0x2C00);
    wr16(0x000048, 0x2C00);
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

    wr16(b | 0x0D7F, 0x0000);
    wr16(b | 0x0D81, 0xFFFF);
    for (i = 0; i < 10; i++)
        wr8(0x7F0000 | ones[i], 0x01);
    for (i = 0; i < 4; i++)
        wr8(0x7F0000 | zeros[i], 0x00);
    wr8(b | 0x0D47, 0x00);
    io8(bank, DMAP1, 0x01);
    io8(bank, BBAD1, 0x26);
    io8(bank, A1B1, 0x7F);
    io8(bank, A1T1L, 0x10);
    io8(bank, A1T1H, 0x34);
    io8(bank, WOBJSEL, 0x30);
    io8(bank, TMW, 0x07);
    io8(bank, TM, 0x02);
    io8(bank, W34SEL, 0x03);
    io8(bank, CGWSEL, 0x20);

    /* The scroll HDMA table: $38 lines of $0E33, $4F lines of $0E37, then
       three single lines of $0E37 and the end. */
    wr8(b | 0x10AA, 0x38);
    wr8(b | 0x10AB, rd8(b | 0x0E33));
    wr8(b | 0x10AC, rd8(b | 0x0E34));
    wr8(b | 0x10AD, 0x4F);
    for (i = 0; i < 4; i++) {
        lo = rd8(b | 0x0E37);
        wr8(b | (uint16_t)(0x10AE + 3 * i), lo);
        hi = rd8(b | 0x0E38);
        wr8(b | (uint16_t)(0x10AF + 3 * i), hi);
        wr8(b | (uint16_t)(0x10B0 + 3 * i), i < 3 ? 0x01 : 0x00);
    }
    io8(bank, DMAP2, 0x02);
    io8(bank, BBAD2, 0x10);
    io8(bank, A1B2, 0x00);
    io8(bank, A1T2L, 0xAA);
    io8(bank, A1T2H, 0x10);

    wr8(b | 0x0D45, 0x02);
    wr8(b | 0x0E91, 0xF0);
    wr8(b | 0x0E92, 0xF0);
    io8(bank, BGMODE, 0x0B);
    io8(bank, OBSEL, 0x03);
    io8(bank, BG1SC, 0x00);
    io8(bank, BG2SC, 0x52);
    io8(bank, BG12NBA, 0x20);
    io8(bank, TM, 0x12);
    io8(bank, CGADD, 0x01);
    io8(bank, CGDATA, 0x00);
    io8(bank, CGDATA, 0x00);
    for (i = 0; i < 6; i++) {
        io8(bank, scroll[i], 0x00);
        io8(bank, scroll[i], 0x00);
    }
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

    io8(bank, NMITIMEN, 0x00);
    wr8(0x000000, 0x00);
    rd8(b | 0x1073);
    rd16(GSU_SFR);
    wr16(b | 0x0374, 0x0003);
    io8(bank, VMAIN, 0x80);
    wr16(b | (VMADDL & 0xFFFF), 0x0000);
    DmaFromDescriptor(bank, 0xF873);
    wr16(b | (VMADDL & 0xFFFF), 0x7C00);
    DmaFromDescriptor(bank, 0xF87A);
}

void abi_VideoStartUploads(void)
{
    VideoStartUploads(cpu.db);
    cpu.a = 0xF801;
    cpu.x = 0x0080;
    cpu.y = 0x00FF;
}

void abi_VideoRegsReset(void);
void abi_VideoStartUploads(void);
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

/* GsuCF42ToVram with the input record X and the transfer record Y. */
static void cf42_vram(uint16_t in, uint16_t out)
{
    cpu.p &= (uint8_t)~(P_M | P_X);
    cpu.x = in;
    cpu.y = out;
    abi_GsuCF42ToVram();
}

/* GsuCF42To3000 with A and X. */
static void cf42_3000(uint16_t a, uint16_t x)
{
    cpu.p &= (uint8_t)~(P_M | P_X);
    cpu.a = a;
    cpu.x = x;
    abi_GsuCF42To3000();
}

/* DMA channel 0 from Super FX RAM $70:src to VRAM, as the inline
   transfers write it (long addressing). */
static void dma_gsu_to_vram(uint16_t src, uint16_t vram, uint16_t size)
{
    wr16(DMAP0, 0x1801);
    wr16(A1T0L, src);
    wr16(VMADDL, vram);
    wr16(DAS0L, size);
    wr8(VMAIN, 0x80);
    wr8(A1B0, 0x70);
    wr8(MDMAEN, 0x01);
}

/* MVN: count + 1 bytes, each read and then written, upwards. */
static void block_move(uint8_t sb, uint16_t src, uint8_t db, uint16_t dst, uint16_t count)
{
    uint32_t n = (uint32_t)count + 1;

    while (n--) {
        uint8_t v = rd8((uint32_t)sb << 16 | src);

        wr8((uint32_t)db << 16 | dst, v);
        src = (uint16_t)(src + 1);
        dst = (uint16_t)(dst + 1);
    }
}

/* The course tilemap from the Super FX picture: rows of five
   TilemapRowFromGsuRam calls into the data bank $7F from y, $F1 stepping by
   $40 to $800. */
static void course_tilemap(uint16_t y)
{
    static const uint16_t part[5] = {0x0000, 0x0800, 0x1000, 0x1800, 0x0000};
    uint8_t keep = cpu.db;
    uint16_t f1;
    unsigned k;

    cpu.db = 0x7F;
    cpu.p &= (uint8_t)~(P_M | P_X);
    cpu.y = y;
    wr16(0x0000F1, 0x0000);
    do {
        for (k = 0; k < 5; k++) {
            cpu.a = part[k];
            abi_TilemapRowFromGsuRam();
        }
        f1 = (uint16_t)(rd16(0x0000F1) + 0x0040);
        wr16(0x0000F1, f1);
        cpu.a = f1;
    } while (f1 < 0x0800);
    cpu.db = keep;
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
void RaceVideoSetup(void)
{
    uint8_t p = cpu.p, db = cpu.db, mode;
    uint32_t b = 0x030000;
    uint16_t c, i, k;

    cpu.db = 0x03;
    cpu.p = (uint8_t)(cpu.p & ~P_M);
    abi_VideoRegsReset();
    abi_VideoStartUploads();
    abi_SoundIndicesFromHandlers();
    cf42_vram(0xF7DA, 0xF80D);
    c = rd16(b | 0x0E3D);
    i = (uint16_t)(((uint16_t)(c << 1) + rd16(b | 0x0E3D)) & 0x00FF);
    cpu.a = i;
    abi_PaletteCourseLoad();
    abi_CgramDmaAll();
    cpu.a = rd16(b | (uint16_t)(0xF807 + i));
    cpu.p = (uint8_t)((cpu.p | P_M) & ~P_X);
    cpu.x = i;
    abi_GradientBuildColours();
    if (rd8(b | 0x0D62)) {
        abi_MenuCourseGraphics();
        wr16(0x7F9BD7, 0x0000);
        block_move(0x7F, 0x9BD7, 0x7F, 0x9BD8, 0x1FFF);
        cf42_vram(0xF7E3, 0xF84C);
        block_move(0x70, 0x3000, 0x7F, 0x93D7, 0x07FF);
        cf42_3000(0xC770, 0x0018);
        dma_gsu_to_vram(0x3000, 0x7C00, 0x0800);
        dma_gsu_to_vram(0x3600, 0x6600, 0x0400);
        cpu.a = 0x0401;
        cpu.y = 0x9BD7;
        cpu.p = db;
        cpu.db = p;
        return;
    }
    cf42_vram((uint16_t)(i + 0xF4F9), 0xF853);
    if (rd16(b | (uint16_t)(0xF54D + i)))
        cf42_vram((uint16_t)(i + 0xF54D), 0xF85A);
    cf42_vram(0xF7DD, 0xF814);
    cf42_3000(0xD970, 0x001D);
    abi_CoursePictureToVram();
    mode = rd8(b | 0x0D2B);
    if (mode == 1 && !rd8(b | 0x0D4E)) {
        cf42_vram(0xF7D4, 0xF830);
        cf42_3000(0xFA38, 0x0018);
        dma_gsu_to_vram(0x3000, 0x5AC0, 0x0180);
        cf42_3000(0xFB90, 0x0018);
    } else if ((mode == 1 || mode == 3) && !rd8(b | 0x10CD)) {
        cf42_vram(0xF7D4, 0xF830);
        cf42_3000(0xFA38, 0x0018);
        dma_gsu_to_vram(0x3180, 0x5AC0, 0x0100);
        cf42_3000(0xFC20, 0x0018);
    } else if (mode != 2) {
        cf42_vram(0xF7D1, 0xF829);
        cf42_3000(0x9228, 0x001D);
        cpu.p &= (uint8_t)~(P_M | P_X);
        k = (uint16_t)((rd16(b | 0x0D8B) & 0x000F) << 8);
        wr16(b | 0x2116, 0x5B40);
        for (c = 0; c < 0x80; c++) {
            wr16(b | 0x2118, rd16(0x703000 + k));
            k = (uint16_t)(k + 2);
        }
        cf42_3000(0xFAE8, 0x0018);
    }
    cpu.p &= (uint8_t)~(P_M | P_X);
    abi_ScreenFrameAndPictures();
    cf42_3000(rd16(b | (uint16_t)(0xF5A1 + i)), rd16(b | (uint16_t)(0xF5A3 + i)));
    course_tilemap(0x0000);
    cpu.y = i;
    k = (uint16_t)(rd16(b | (uint16_t)(0xF5F7 + i)) & 0x00FF);
    cpu.a = k;
    if (k) {
        cf42_3000(rd16(b | (uint16_t)(0xF5F5 + i)), k);
        course_tilemap(0x2800);
    }
    cpu.db = db;
    cpu.p = p;
}

void abi_RaceVideoSetup(void)
{
    RaceVideoSetup();
}

/*
 * @routine 03:D996 RaceVideoInit
 * WHAT IT DOES: RaceVideoSetup, first clearing $0EED-$0EEF and the words at
 * $0E6C and $0E6E when $0D24 is 3 or more.
 * HOW: Data bank $03 (the program bank) around the call, so the clears go to
 * work RAM through it. The clears are not reached in the traces so far.
 * Entry: M 8-bit, D = $0000. Exit: as RaceVideoSetup; the data bank
 * unchanged, N and Z from it (as RaceVideoSetup's swapped return leaves P
 * otherwise).
 */
void RaceVideoInit(void)
{
    uint8_t db = cpu.db, v = rd8(0x030D24);

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = (uint8_t)((nz8(cpu.p, (uint8_t)(v - 0x03)) & ~P_C) | P_M | (v >= 0x03 ? P_C : 0));
    if (v >= 0x03) {
        wr8(0x030EED, 0x00);
        wr8(0x030EEE, 0x00);
        wr8(0x030EEF, 0x00);
        wr16(0x030E6C, 0x0000);
        wr16(0x030E6E, 0x0000);
        cpu.p &= (uint8_t)~P_M;
    }
    cpu.db = 0x03;
    RaceVideoSetup();
    cpu.db = db;
    cpu.p = nz8(cpu.p, db);
}

void abi_RaceVideoInit(void)
{
    RaceVideoInit();
}
