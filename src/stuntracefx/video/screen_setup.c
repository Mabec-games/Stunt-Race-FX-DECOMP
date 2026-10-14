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
