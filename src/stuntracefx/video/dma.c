/*
 * dma.c: the DMA transfers that feed the PPU: VRAM from work RAM or Super FX
 * RAM, the sprite table, the palette. All use DMA channel 0 and run from work
 * RAM, inside the raster interrupt or with the screen off.
 *
 * To read Super FX RAM the S-CPU must own it: the transfers from bank $70
 * clear the RAN bit of the Super FX screen mode register for the transfer
 * and then give ROM and RAM back to the Super FX.
 */
#include "machine.h"
#include "hw.h"
#include "wram.h"
#include "cpu_ops.h"

/* DMA channel 0, mode 1 (two registers, write once) to VMDATAL/H. */
static void vram_channel(uint16_t size, uint16_t src, uint16_t vram)
{
    cyc(24);
    wr16(DAS0L, size);
    cyc(24);
    wr16(A1T0L, src);
    cyc(24);
    wr16(VMADDL, vram);
    cyc(48);
    wr16(DMAP0, 0x1801);
}

/*
 * @routine 7E:ABB6 VramDmaFromBank7F
 * WHAT IT DOES: Copies a block of work RAM bank $7F to VRAM.
 * HOW: Channel 0 in mode 1 to VMDATA, VRAM incrementing after the high
 * byte, then starts the transfer.
 * Entry: M 16-bit, X 16-bit; A = byte count, X = source address in bank $7F,
 * Y = VRAM word address.
 * Exit: A = $1801; M 8-bit; N, Z clear; X, Y unchanged.
 */
void VramDmaFromBank7F(uint16_t size, uint16_t src, uint16_t vram)
{
    vram_channel(size, src, vram);
    cyc(62);
    wr8(A1B0, 0x7F);
    cyc(40);
    wr8(VMAIN, 0x80);
    cyc(40);
    wr8(MDMAEN, 0x01);
    cyc(42);
}

void abi_VramDmaFromBank7F(void)
{
    VramDmaFromBank7F(cpu.a, cpu.x, cpu.y);
    cpu.a = 0x1801;
    cpu.p = (uint8_t)((cpu.p & ~(P_N | P_Z)) | P_M);
}

/*
 * @routine 7E:ABD7 VramDmaFromGsuRam
 * WHAT IT DOES: Copies a block of Super FX RAM (bank $70) to VRAM.
 * HOW: As VramDmaFromBank7F from bank $70, with the Super FX screen mode
 * register's RAN bit cleared for the transfer (the S-CPU takes the RAM),
 * then ROM and RAM given back (bits 4 and 3 set).
 * Entry: M 16-bit, X 16-bit; A = byte count, X = source address, Y = VRAM
 * word address.
 * Exit: A = $18 in the high byte, the screen mode value written last in the
 * low byte; M 8-bit; N, Z from that value; X, Y unchanged.
 */
uint8_t VramDmaFromGsuRam(uint16_t size, uint16_t src, uint16_t vram)
{
    uint8_t mode;

    vram_channel(size, src, vram);
    cyc(62);
    wr8(A1B0, 0x70);
    cyc(40);
    wr8(VMAIN, 0x80);
    cyc(24);
    wr8(GSU_SCMR, cyc_then(40, rd8(GSU_SCMR_SHADOW) & 0xF7));
    cyc(40);
    wr8(MDMAEN, 0x01);
    cyc(24);
    mode = rd8(GSU_SCMR_SHADOW) | 0x18;
    cyc(40);
    wr8(GSU_SCMR, mode);
    cyc(42);
    return mode;
}

void abi_VramDmaFromGsuRam(void)
{
    uint8_t mode = VramDmaFromGsuRam(cpu.a, cpu.x, cpu.y);

    cpu.a = (uint16_t)(0x1800 | mode);
    cpu.p = (uint8_t)(nz8(cpu.p, mode) | P_M);
}

/*
 * @routine 7E:AC08 VramDmaRowsFromGsuRam
 * WHAT IT DOES: Copies rows of $140 bytes from Super FX RAM to consecutive
 * VRAM, taking each row $200 bytes further on in Super FX RAM.
 * HOW: Sets the VRAM address and channel 0 once, takes the RAM from the
 * Super FX (16-bit store to the screen mode register and the byte after it),
 * then per row sets the source address, a count of $140 and starts the
 * transfer; VRAM runs on from row to row. Gives ROM and RAM back at the end.
 * Entry: M 16-bit, X 16-bit; A = first row's address, X = VRAM word address,
 * Y = row count (1-255; the routine switches to 8-bit index registers).
 * Exit: A = the 16-bit word at $0374 with bits 3 and 4 set; X = 1; Y = 0;
 * X flag set; N, Z from A; C and V from the last row step, or unchanged for
 * one row.
 */
uint16_t VramDmaRowsFromGsuRam(uint16_t src, uint16_t vram, uint8_t rows)
{
    uint16_t mode;

    cyc(24);
    wr16(VMADDL, vram);
    cyc(48);
    wr16(DMAP0, 0x1801);
    cyc(62);
    wr8(A1B0, 0x70);
    cyc(40);
    wr8(VMAIN, 0x80);
    cyc(54);
    wr16(GSU_SCMR, cyc_then(48, rd16(GSU_SCMR_SHADOW) & 0xFFF7));
    for (;;) {
        cyc(60);
        wr16(A1T0L, src);
        cyc(70);
        wr16(DAS0L, 0x0140);
        cyc(62);
        wr8(MDMAEN, 0x01);
        if (--rows == 0)
            break; else { cyc(54); }
        src = (uint16_t)(src + 0x0200);
    }
    cyc(60);
    mode = rd16(GSU_SCMR_SHADOW) | 0x0018;
    cyc(48);
    wr16(GSU_SCMR, mode);
    return mode;
}

void abi_VramDmaRowsFromGsuRam(void)
{
    uint8_t rows = (uint8_t)cpu.y;
    uint16_t last = (uint16_t)(cpu.a + 0x0200 * (uint8_t)(rows - 2));
    uint16_t mode = VramDmaRowsFromGsuRam(cpu.a, cpu.x, rows);

    if (rows != 1) {
        uint32_t sum = (uint32_t)last + 0x0200;
        cpu.p = (uint8_t)((cpu.p & ~(P_C | P_V)) | (sum > 0xFFFF ? P_C : 0)
                          | ((~(last ^ 0x0200) & (last ^ sum) & 0x8000) ? P_V : 0));
    }
    cpu.a = mode;
    cpu.x = 0x01;
    cpu.y = 0;
    cpu.p = (uint8_t)(nz16(cpu.p, mode) | P_X);
    cyc(42);
}

/*
 * @routine 7E:ACF7 OamDma
 * WHAT IT DOES: Copies a block of work RAM to OAM from its start.
 * HOW: Sets the OAM address to 0, then channel 0 in mode 0 to OAMDATA from
 * bank $00, and starts the transfer.
 * Entry: M 8-bit, X 16-bit; X = source address, Y = byte count.
 * Exit: A low byte = 1; X = $0400; N, Z clear; Y unchanged.
 */
void OamDma(uint16_t src, uint16_t size)
{
    cyc(24);
    wr16(A1T0L, src);
    cyc(40);
    wr8(OAMADDL, 0x00);
    cyc(24);
    wr8(OAMADDH, 0x00);
    cyc(24);
    wr16(DAS0L, size);
    cyc(48);
    wr16(DMAP0, 0x0400);
    cyc(40);
    wr8(A1B0, 0x00);
    cyc(40);
    wr8(MDMAEN, 0x01);
    cyc(42);
}

void abi_OamDma(void)
{
    OamDma(cpu.x, cpu.y);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x01);
    cpu.x = 0x0400;
    cpu.p &= (uint8_t)~(P_N | P_Z);
}

/*
 * @routine 7E:AD16 OamDmaAll
 * WHAT IT DOES: Copies the whole work-RAM sprite table to OAM.
 * HOW: OamDma of the $220 bytes from $0375 (the 512-byte table and its
 * 32-byte high table).
 * Entry: any widths. Exit: A low byte = 1; X = $0400 and Y = $0220, or $00
 * and $20 when entered with 8-bit index registers; P unchanged.
 */
void OamDmaAll(void)
{
    cyc(160);
    OamDma(OAM_BUFFER, 0x0220);
    cyc(72);
}

void abi_OamDmaAll(void)
{
    OamDmaAll();
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x01);
    cpu.x = (cpu.p & P_X) ? (cyc(0), 0x00) : 0x0400;
    cpu.y = (cpu.p & P_X) ? 0x20 : 0x0220;
}

/*
 * @routine 7E:B0F6 CgramDma
 * WHAT IT DOES: Copies a block of work RAM bank $7E to the palette.
 * HOW: Channel 0 in mode 0 to CGDATA; the low byte of X is the first colour.
 * Entry: M 16-bit, X 16-bit; A = source address, X = first colour, Y = byte
 * count.
 * Exit: A = $2201; M and X 8-bit (X and Y keep only their low bytes); N, Z
 * clear.
 */
void CgramDma(uint16_t src, uint8_t colour, uint16_t size)
{
    cyc(24);
    wr16(A1T0L, src);
    cyc(24);
    wr16(DAS0L, size);
    cyc(48);
    wr16(DMAP0, 0x2200);
    cyc(46);
    wr8(CGADD, colour);
    cyc(40);
    wr8(A1B0, 0x7E);
    cyc(40);
    wr8(MDMAEN, 0x01);
}

void abi_CgramDma(void)
{
    CgramDma(cpu.a, (uint8_t)cpu.x, cpu.y);
    cpu.a = 0x2201;
    cpu.x &= 0x00FF;
    cpu.y &= 0x00FF;
    cpu.p = (uint8_t)((cpu.p & ~(P_N | P_Z)) | P_M | P_X);
    cyc(42);
}

/* Work RAM $7E:FD65: the full 512-byte palette as the game last built it. */
#define PALETTE_BUFFER 0xFD65u

/*
 * @routine 7E:B0E5 CgramDmaAll
 * WHAT IT DOES: Copies the whole palette buffer ($7E:FD65, 512 bytes) to the
 * palette.
 * HOW: CgramDma from colour 0.
 * Entry: any widths. Exit: A = $2201; X = 0, Y = 0; P unchanged.
 */
void CgramDmaAll(void)
{
    cyc(162);
    CgramDma(PALETTE_BUFFER, 0x00, 0x0200);
    cyc(72);
}

void abi_CgramDmaAll(void)
{
    CgramDmaAll();
    cpu.a = 0x2201;
    cpu.x = 0;
    cpu.y = 0;
}

/*
 * @routine 7F:0BCE VramWriteRowFromGsuRam
 * WHAT IT DOES: Copies one row of 64 words from Super FX RAM ($70:3000 plus
 * the offset in direct page $02) to VRAM, then moves the offset down a row
 * ($200 bytes).
 * HOW: Sets the VRAM address and writes the words to VMDATA one by one.
 * Entry: M 16-bit, X 16-bit, D = $0000; A = VRAM word address.
 * Exit: A = the new offset; X = the old offset + $80; Y = $FFFF; P unchanged.
 */
static uint16_t vram_write_row(uint16_t vram, uint16_t *x)
{
    uint16_t src, w;
    unsigned i;

    cyc(68);
    wr16(VMADDL, vram);
    cyc(40);
    src = rd16(0x000002);
    for (i = 0; i < 64; i++) {
        cyc(32);
        w = rd16(0x703000 + (uint16_t)(src + 2 * i));
        cyc(24);
        wr16(0x002118, w);
        cyc(64);
    }
    *x = (uint16_t)(src + 0x80);
    cyc(10);
    src = rd16(0x000002);
    src = (uint16_t)(src + 0x0200);
    cyc(54);
    wr16(0x000002, src);
    cyc(70);
    return src;
}

uint16_t VramWriteRowFromGsuRam(uint16_t vram)
{
    uint16_t x;

    return vram_write_row(vram, &x);
}

void abi_VramWriteRowFromGsuRam(void)
{
    uint16_t x;

    cpu.a = vram_write_row(cpu.a, &x);
    cpu.x = x;
    cpu.y = 0xFFFF;
    if (cpu.p & P_X) {
        cpu.x &= 0x00FF;
        cpu.y &= 0x00FF;
    }
}

/*
 * @routine 7F:08C7 VramDmaWideRowsFromGsuRam
 * WHAT IT DOES: Copies rows of $2C0 bytes from Super FX RAM to consecutive
 * VRAM, each row $400 bytes further on in Super FX RAM.
 * HOW: Takes the RAM from the Super FX, keeps the source in direct page
 * $FB-$FC, sets the VRAM address and channel 0 (mode 1 to VMDATA, bank $70)
 * once, then transfers rows while the row counter (the low byte of A, as a
 * signed 8-bit count down to -1) stays non-negative; gives ROM and RAM back.
 * Entry: M 16-bit, X 16-bit, D = $0000; A low byte = rows - 1, X = source,
 * Y = VRAM word address. Exit: A = the source high byte and the screen mode
 * value; X = $00FF; Y = 1; P unchanged.
 */
uint8_t VramDmaWideRowsFromGsuRam(uint8_t rows_less_one, uint16_t src, uint16_t vram, uint16_t *next)
{
    uint8_t mode;
    uint16_t s;

    cyc(238);
    wr8(GSU_SCMR, cyc_then(40, rd8(GSU_SCMR_SHADOW) & 0xF7));
    cyc(38);
    wr16(0x0000FB, src);
    cyc(24);
    wr16(VMADDL, vram);
    cyc(84);
    wr8(VMAIN, 0x80);
    cyc(40);
    wr8(DMAP0 + 1, 0x18);
    cyc(40);
    wr8(A1B0, 0x70);
    cyc(40);
    wr8(DMAP0, 0x01);
    cyc(14);
    do {
        cyc(84);
        wr16(DAS0L, 0x02C0);
        cyc(16);
        s = rd16(0x0000FB);
        cyc(24);
        wr16(A1T0L, s);
        s = (uint16_t)(s + 0x0400);
        cyc(54);
        wr16(0x0000FB, s);
        cyc(24);
        wr8(MDMAEN, 0x01);
    } while (!(--rows_less_one & 0x80));
    cyc(76);
    mode = rd8(GSU_SCMR_SHADOW) | 0x18;
    cyc(40);
    wr8(GSU_SCMR, mode);
    *next = s;
    cyc(70);
    return mode;
}

void abi_VramDmaWideRowsFromGsuRam(void)
{
    uint16_t next;
    uint8_t mode = VramDmaWideRowsFromGsuRam((uint8_t)cpu.a, cpu.x, cpu.y, &next);

    cpu.a = (uint16_t)((next & 0xFF00) | mode);
    cpu.x = 0x00FF;
    cpu.y = 0x0001;
}

/*
 * @routine 7E:ACBD VramUploadGsuBuffer
 * WHAT IT DOES: When a Super FX picture is waiting, copies it ($C00 bytes)
 * from Super FX RAM to VRAM $6600, from whichever half of the double buffer
 * is ready, and clears the request.
 * HOW: With $0D32 set: channel 0 in mode 1 to VMDATA from $70:4000 when
 * $0D7F is set, else from $70:D000; then $0D32 = 0.
 * Entry: M 8-bit, X 16-bit. Exit: A low byte = 1 after a copy, else 0; X =
 * the source after a copy; N clear; Z set when nothing was copied.
 */
static int vram_upload_gsu_buffer(uint16_t *src)
{
    cyc(24);
    if (rd8(0x000D32) == 0)
        { cyc(22); return 0; }
    cyc(56);
    wr8(VMAIN, 0x80);
    cyc(48);
    wr16(VMADDL, 0x6600);
    cyc(48);
    wr16(DMAP0, 0x1801);
    cyc(40);
    wr8(A1B0, 0x70);
    cyc(48);
    wr16(DAS0L, 0x0C00);
    cyc(24);
    *src = rd8(0x000D7F) ? (cyc(16), 0x4000) : 0xD000;
    cyc(70);
    wr16(A1T0L, *src);
    cyc(40);
    wr8(MDMAEN, 0x01);
    cyc(24);
    wr8(0x000D32, 0x00);
    return 1;
}

void VramUploadGsuBuffer(void)
{
    uint16_t src;

    vram_upload_gsu_buffer(&src);
}

void abi_VramUploadGsuBuffer(void)
{
    uint16_t src;

    if (vram_upload_gsu_buffer(&src)) {
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x01);
        cpu.x = src;
        cpu.p &= (uint8_t)~(P_N | P_Z);
    } else {
        cpu.a &= 0xFF00;
        cpu.p = (uint8_t)((cpu.p & ~P_N) | P_Z);
    }
    cyc(42);
}

/*
 * @routine 03:ECA8 DmaFromDescriptor
 * WHAT IT DOES: Runs DMA channel 0 from a seven-byte descriptor: the bytes
 * for $4300-$4306 (mode, B-bus register, source address and bank, size).
 * HOW: The descriptor address goes to direct page $FB; the bytes are copied
 * last to first through ($FB),Y in the data bank, then MDMAEN is set to 1.
 * Entry: M 16-bit, D = $0000; A = descriptor address. Exit: $FB = A; A low
 * byte = 1; Y = $FF (high byte zero); P unchanged.
 */
void DmaFromDescriptor(uint8_t bank, uint16_t desc)
{
    int y;

    cyc(38);
    wr16(0x0000FB, desc);
    cyc(2);
    for (y = 6; y >= 0; y--)
        { cyc(106); wr8(DMAP0 + (unsigned)y, rd8((uint32_t)bank << 16 | (uint16_t)(desc + y))); }
    cyc(70);
    wr8(MDMAEN, 0x01);
    cyc(70);
}

void abi_DmaFromDescriptor(void)
{
    DmaFromDescriptor(cpu.db, cpu.a);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x01);
    cpu.y = 0x00FF;
}

/*
 * @routine 7F:0B56 CoursePictureToVram
 * WHAT IT DOES: Copies four 64-word rows of the current course's picture,
 * as the Super FX drew it, to VRAM $6480, $6580, $64C0 and $65C0, starting
 * from the Super FX RAM offset the table at $7F:0B96 gives for the course
 * in $0E3D.
 * HOW: The table word w (index * 2) packs the offset: ((w AND $FFFC) with
 * its bytes swapped) * 2 + (w AND 3) * $80, stored in direct page $02; then
 * VramWriteRowFromGsuRam four times, each moving the offset down a row.
 * $0E3D is read through the data bank.
 * Entry: X 16-bit, D = $0000. Exit: A = the final offset; X = the last
 * row's offset + $80; Y = $FFFF (low bytes only when P restores 8-bit index
 * registers); P unchanged.
 */
uint16_t CoursePictureToVram(uint8_t bank)
{
    cyc(108);
    uint16_t w = rd16(cyc_then(106, 0x7F0B96 + 2 * (unsigned)rd8((uint32_t)bank << 16 | 0x0E3D)));
    uint16_t t = (uint16_t)(w & 0xFFFC);

    t = (uint16_t)((uint16_t)(t >> 8 | t << 8) << 1);
    cyc(104);
    wr16(0x000002, t);
    cyc(124);
    t = rd16(0x000002);
    cyc(16);
    wr16(0x000002, (uint16_t)(t + ((w & 0x0003) << 7)));
    cyc(70);
    VramWriteRowFromGsuRam(0x6480);
    cyc(70);
    VramWriteRowFromGsuRam(0x6580);
    cyc(70);
    VramWriteRowFromGsuRam(0x64C0);
    cyc(70);
    return cyc_then(72, VramWriteRowFromGsuRam(0x65C0));
}

void abi_CoursePictureToVram(void)
{
    cyc(160);
    uint16_t a = CoursePictureToVram(cpu.db);

    cpu.a = a;
    cpu.x = (uint16_t)(a - 0x0200 + 0x80);
    cpu.y = 0xFFFF;
    if (cpu.p & P_X) {
        cpu.x &= 0x00FF;
        cpu.y &= 0x00FF;
    }
}

uint16_t VramDmaQueueAddA(uint16_t a);

/*
 * @routine 7F:0CDF AnimTilesQueueNext
 * WHAT IT DOES: Queues the next $400-byte frame of the animated tiles at
 * $7F:A7D7 for VRAM $2000: the frame offset in $18F7 steps by $400 through
 * $2000 bytes (eight frames), and the transfer goes on the VRAM queue.
 * HOW: Stages source $A7D7 + offset, bank $7F, VRAM $2000 and count $400 at
 * $16B1-$16B8 (through the data bank) and calls VramDmaQueueAdd; then sets
 * bit 1 of the update flags at $11D7.
 * Entry: any widths. Exit: A low byte = the update flags, high byte from
 * the new queue size (or $20 when the queue was full); P, X, Y unchanged.
 */
uint8_t AnimTilesQueueNext(uint8_t bank, uint16_t *a)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t off;
    uint8_t f;

    cyc(76);
    off = rd16(0x0018F7);
    off = (uint16_t)((off + 0x0400) & 0x1FFF);
    cyc(94);
    wr16(0x0018F7, off);
    cyc(84);
    wr16(b | 0x16B3, (uint16_t)(off + 0xA7D7));
    cyc(48);
    wr16(b | 0x16B1, 0x0400);
    cyc(48);
    wr16(b | 0x16B5, 0x007F);
    cyc(48);
    wr16(b | 0x16B7, 0x2000);
    cyc(62);
    *a = VramDmaQueueAddA(0x2000);
    cyc(198);
    f = (uint8_t)(rd8(b | 0x11D7) | 0x02);
    cyc(40);
    wr8(b | 0x11D7, f);
    cyc(72);
    return f;
}

void abi_AnimTilesQueueNext(void)
{
    uint16_t a;
    uint8_t f = AnimTilesQueueNext(cpu.db, &a);

    cpu.a = (uint16_t)((a & 0xFF00) | f);
}

/*
 * @routine 7E:A8E6 CgramUploadAll
 * WHAT IT DOES: Copies the whole palette buffer ($7E:FD65, 512 bytes) to
 * CGRAM, noting $FD65 at $0D83.
 * HOW: CgramDma from colour 0. NmiFrameUpload's stages follow it in the
 * same stretch of code.
 * Entry: any widths. Exit: P restored; A = $2201, X = 0, Y = 0.
 */
void abi_CgramUploadAll(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint8_t p0 = cpu.p;

    cpu.p &= (uint8_t)~(P_M | P_X);
    cpu.a = 0xFD65;
    cyc(92);
    wr16(b | 0x0D83, 0xFD65);
    cpu.x = 0x0000;
    cpu.y = 0x0200;
    cpu.p = nz16(cpu.p, cpu.y);
    (cyc(94), abi_CgramDma());
    cpu.p = p0;
    if (p0 & P_X) {
        cpu.x &= 0x00FF;
        cpu.y &= 0x00FF;
    } else { cyc(70); }
}

void abi_TilemapRowsTo5C00(void);
void abi_Bg2ScrollFromTable(void);

/* LDA #n, LDX #n, LDY #n in some order: N, Z from the last one loaded. */
static void ld3(uint16_t a, uint16_t x, uint16_t y, uint16_t last)
{
    cyc(118);
    cpu.a = a;
    cpu.x = x;
    cpu.y = y;
    cpu.p = nz16(cpu.p, last);
}

/* LDA #n / DEC A / BNE: a delay of n turns, leaving A = 0. */
static void delay_a(uint32_t n)
{
    cyc(8);
    delay_turns(n);
    cpu.a = 0;
    cpu.p = nz16(cpu.p, 0);
}

/* LDX #n / STX $0D7D: the next stage. */
static void next_stage(uint32_t b, uint16_t n)
{
    cpu.x = n;
    cpu.p = nz16(cpu.p, n);
    cyc(64);
    wr16(b | 0x0D7D, n);
}

/* SEP #$20, STZ $00, DEC $0D28. */
static void frame_done(uint32_t b)
{
    uint8_t v;

    cpu.p |= P_M;
    cyc(38);
    wr8(0x000000, 0x00);
    cyc(24);
    v = (uint8_t)(rd8(b | 0x0D28) - 1);
    cyc(6);
    wr8(b | 0x0D28, v);
    cpu.p = nz8(cpu.p, v);
}

/* SEP #$20, $0357 = 1 (and STZ $00 when z). */
static void flag_0357(uint32_t b, int z)
{
    cpu.p |= P_M;
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x01);
    cpu.p = nz8(cpu.p, 0x01);
    cyc(62);
    wr8(b | 0x0357, 0x01);
    if (z)
        { cyc(16); wr8(0x000000, 0x00); }
}

/* LDA #n / STA reg with M 8-bit. */
static void put8(uint32_t addr, uint8_t v)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, v);
    cyc(40);
    wr8(addr, v);
}

/* LDA abs / STA reg with M 8-bit. */
static void copy8(uint32_t to, uint32_t from)
{
    cyc(24);
    uint8_t v = rd8(from);

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, v);
    cyc(24);
    wr8(to, v);
}

/* The stage at $A800: sprites to OAM when $0D28 is set (and $0D54 clear),
   then the whole palette. */
static void upload_sprites_palette(uint32_t b)
{
    uint8_t v;

    cpu.p = (uint8_t)((cpu.p | P_M) & ~P_X);
    cyc(92);
    delay_turns(1);
    cpu.x = 0;
    cpu.p = nz16(cpu.p, 0);
    v = rd8(b | 0x0D28);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, v);
    if (v) {
        cyc(40);
        wr8(b | 0x0D28, 0x00);
        cyc(24);
        v = rd8(b | 0x0D54);
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
        cpu.p = nz8(cpu.p, v);
        if (!v) {
            cyc(110);
            cpu.x = 0x0375;
            cpu.y = 0x0220;
            cpu.p = nz16(cpu.p, cpu.y);
            abi_OamDma();
        }
        cyc(20);
    }
    cyc(90);
    cpu.p &= (uint8_t)~(P_M | P_X);
    abi_CgramUploadAll();
    cyc(24);
    delay_turns(0x0A);
    cpu.x = 0;
    cpu.p = nz16(cpu.p, 0);
}

/* The rows stages ($A9B1, $A9D3, $A9FA, $AA1C): with $0D7F set (or clear,
   by want), the next stage, rows from Super FX RAM, $0357 = 1, and BG1/2's
   tile bases when nba is nonzero. */
static void upload_rows(uint32_t b, int want, uint16_t n, uint16_t src,
                        uint16_t vram, uint8_t nba)
{
    uint16_t sel;

    cyc(24);
    sel = rd16(b | 0x0D7F);
    cpu.a = sel;
    cpu.p = nz16(cpu.p, sel);
    if (!sel != !want)
        { cyc(22); return; }
    next_stage(b, n);
    ld3(src, vram, 0x0012, 0x0012);
    abi_VramDmaRowsFromGsuRam();
    flag_0357(b, 1);
    if (nba)
        put8(BG12NBA, nba);
}

/* The split screen's DMA of $7F:9BD7 ($1680 bytes) to VRAM word x, then
   BG1/2 tile bases $70, stage $10 and colour 0 = $1CC4. */
static void split_picture(uint32_t b, uint16_t x)
{
    int i;

    cpu.x = x;
    cpu.p = nz16(cpu.p, x);
    cyc(92);
    wr16(VMADDL, x);
    cpu.x = 0x1680;
    cyc(48);
    wr16(DAS0L, 0x1680);
    cpu.x = 0x9BD7;
    cyc(48);
    wr16(A1T0L, 0x9BD7);
    cpu.x = 0x1801;
    cpu.p = nz16(cpu.p, 0x1801);
    cyc(48);
    wr16(DMAP0, 0x1801);
    put8(A1B0, 0x7F);
    put8(VMAIN, 0x80);
    cyc(24);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | rd8(b | 0x0374));
    cpu.a &= 0xFFF7;
    cpu.p = nz8(cpu.p, (uint8_t)cpu.a);
    cyc(40);
    wr8(GSU_SCMR, (uint8_t)cpu.a);
    put8(MDMAEN, 0x01);
    cyc(24);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | rd8(b | 0x0374));
    cpu.a |= 0x0018;
    cpu.p = nz8(cpu.p, (uint8_t)cpu.a);
    cyc(40);
    wr8(GSU_SCMR, (uint8_t)cpu.a);
    cyc(22);
    put8(BG12NBA, 0x70);
    put8(b | 0x0D23, 0x10);
    put8(CGADD, 0x00);
    for (i = 1; i < 7; i++)
        { cyc(24); wr8(CGADD, 0x00); }
    put8(CGDATA, 0xC4);
    cyc(32);
    wr8(0x7EFD65, 0xC4);
    put8(CGDATA, 0x1C);
    cyc(32);
    wr8(0x7EFD66, 0x1C);
    cyc(24);
    wr8(b | 0x0E5F, 0x00);
}

/* The split screen's stages, by $0D23 (the table at $AA4B). */
static void split_stage(uint32_t b)
{
    uint8_t s;

    cpu.p |= P_M | P_X;
    cpu.x &= 0x00FF;
    cpu.y &= 0x00FF;
    cyc(46);
    s = rd8(b | 0x0D23);
    cpu.x = s;
    cpu.p = nz8(cpu.p, s);
    cyc(30);
    switch (rd16(0x7E0000 | (uint16_t)(0xAA4B + s))) {
    case 0xABB4:
        break;
    case 0xAB63:
        cpu.p = (uint8_t)((cpu.p | P_M) & ~P_X);
        (cyc(90), abi_TilemapRowsTo5C00());
        /* fall through */
    case 0xAB6A:
        cyc(138);
        cpu.p = (uint8_t)((cpu.p | P_M) & ~P_X);
        cpu.x = 0x0375;
        cpu.y = 0x0220;
        cpu.p = nz16(cpu.p, cpu.y);
        abi_OamDma();
        cpu.p |= P_M | P_X;
        cpu.x = 0;
        cpu.y &= 0x00FF;
        cpu.p = nz8(cpu.p, 0);
        (cyc(84), abi_Bg2ScrollFromTable());
        cpu.p &= (uint8_t)~(P_M | P_X);
        (cyc(68), abi_CgramUploadAll());
        break;
    case 0xAB85:
        cpu.p &= (uint8_t)~(P_M | P_X);
        ld3(0x0A00, 0x4000, 0x6C00, 0x0A00);
        abi_VramDmaFromGsuRam();
        cpu.p &= (uint8_t)~(P_M | P_X);
        ld3(0x0140, 0x4600, 0x6240, 0x0140);
        abi_VramDmaFromGsuRam();
        cpu.p &= (uint8_t)~(P_M | P_X);
        ld3(0x0140, 0x4800, 0x6340, 0x0140);
        abi_VramDmaFromGsuRam();
        cpu.p |= P_M;
        wr8(b | 0x0D23, 0x00);
        break;
    case 0xAB4C:
        cpu.p = (uint8_t)((cpu.p | P_M) & ~P_X);
        (cyc(90), abi_VramUploadGsuBuffer());
        cpu.x = 0x0375;
        cpu.y = 0x0220;
        cpu.p = nz16(cpu.p, cpu.y);
        (cyc(94), abi_OamDma());
        /* fall through */
    case 0xAB5C:
        cyc(68);
        cpu.p &= (uint8_t)~(P_M | P_X);
        abi_CgramUploadAll();
        break;
    case 0xAA91:
        cyc(22);
        cpu.p = (uint8_t)((cpu.p | P_M) & ~P_X);
        split_picture(b, 0x0000);
        break;
    case 0xAA9A:
        cpu.p = (uint8_t)((cpu.p | P_M) & ~P_X);
        split_picture(b, 0x0B40);
        break;
    case 0xAB0F:
        cpu.p &= (uint8_t)~P_M;
        cpu.a = 0x0800;
        cyc(70);
        wr16(DAS0L, 0x0800);
        cpu.a = 0x93D7;
        cyc(48);
        wr16(A1T0L, 0x93D7);
        cpu.a = 0x7800;
        cyc(48);
        wr16(VMADDL, 0x7800);
        cpu.a = 0x1801;
        cpu.p = nz16(cpu.p, 0x1801);
        cyc(48);
        wr16(DMAP0, 0x1801);
        cpu.p |= P_M;
        cyc(22);
        put8(A1B0, 0x7F);
        put8(VMAIN, 0x80);
        cyc(24);
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | rd8(b | 0x0374));
        cpu.a &= 0xFFF7;
        cpu.p = nz8(cpu.p, (uint8_t)cpu.a);
        cyc(40);
        wr8(GSU_SCMR, (uint8_t)cpu.a);
        put8(MDMAEN, 0x01);
        cyc(24);
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | rd8(b | 0x0374));
        cpu.a |= 0x0018;
        cpu.p = nz8(cpu.p, (uint8_t)cpu.a);
        cyc(40);
        wr8(GSU_SCMR, (uint8_t)cpu.a);
        break;
    case 0xAA5F: {
        uint16_t src;

        cpu.p = (uint8_t)((cpu.p | P_M) & ~P_X);
        cyc(68);
        src = rd16(b | 0x10A0);
        cpu.x = src;
        cyc(24);
        wr16(A1T0L, src);
        put8(A1B0, 0x7F);
        cpu.x = 0x0C00;
        cyc(48);
        wr16(DAS0L, 0x0C00);
        cpu.x = 0x6600;
        cyc(48);
        wr16(VMADDL, 0x6600);
        cpu.x = 0x1801;
        cpu.p = nz16(cpu.p, 0x1801);
        cyc(48);
        wr16(DMAP0, 0x1801);
        put8(VMAIN, 0x80);
        put8(MDMAEN, 0x01);
        put8(b | 0x0D23, 0x02);
        break;
    }
    }
}

/*
 * @routine 7E:A7BE NmiFrameUpload
 * WHAT IT DOES: The vertical blank's uploads for the screen being built,
 * one stage per frame. With $0D22 nonzero, the split screen's stage $0D23
 * (the table at $AA4B): pictures and maps from Super FX RAM and bank $7F,
 * sprites, BG2's scroll and the palette. Otherwise the race screen's stage
 * $0D7D (the table at $A7D1): halves of the Super FX picture by the buffer
 * $0D7F, the bank $7F rows that go with them, the tile and map bases and
 * BG2's scroll for the finished buffer ($0D81 = -1 or 1, $0D28
 * decremented); each sets the next stage. Stages with nothing to do
 * upload the sprites (with $0D28 set and $0D54 clear) and the palette.
 * HOW: P saved and restored. JMP (table,X); the stages share their ends.
 * The race stages from the jump on are NmiFrameUploadRace; the table's
 * entry $1C ($A8E6, CgramUploadAll) and the helper at $A7F3
 * (VramRowsCopyUnused) are never used by the game.
 * Entry: D = $0000, data bank $00. Exit: P restored; A, X, Y as the stage
 * leaves them.
 */
static int race_stage(uint32_t b);

void abi_NmiFrameUpload(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint8_t p0 = cpu.p;
    uint8_t v;

    cpu.p |= P_M;
    cyc(68);
    v = rd8(b | 0x0D22);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, v);
    if (v) {
        cyc(40);
        split_stage(b);
        op_plp(p0);
        cyc(28);
        return;
    } else { cyc(22); }
    race_stage(b);
    op_plp(p0);
}

/* NmiFrameUpload's race screen stage: X = $0D7D, the stage by the table at
   $7E:A7D1. Returns 0 for the entry $1C, CgramUploadAll, which the stage
   code reaches by its own JSR elsewhere and here would take P's place on
   the stack: its RTS ends the caller instead. */
static int race_stage(uint32_t b)
{
    uint16_t sel;

    cpu.p &= (uint8_t)~(P_M | P_X);
    cyc(46);
    cpu.x = rd16(b | 0x0D7D);
    cpu.p = nz16(cpu.p, cpu.x);
    cyc(30);
    switch (rd16(0x7E0000 | (uint16_t)(0xA7D1 + cpu.x))) {
    case 0xA82D:
        delay_a(1);
        cyc(62);
        sel = rd16(b | 0x0D7F);
        cpu.a = sel;
        cpu.p = nz16(cpu.p, sel);
        if (!sel) {
            cyc(22);
            upload_sprites_palette(b);
            break;
        } else { cyc(22); }
        next_stage(b, 0x0002);
        ld3(0x2200, 0x2C00, 0x0000, 0x2200);
        abi_VramDmaFromGsuRam();
        cpu.p &= (uint8_t)~(P_M | P_X);
        cyc(22);
        ld3(0x0200, 0xE3D7, 0x5BC0, 0x5BC0);
        abi_VramDmaFromBank7F();
        break;
    case 0xA85E:
        cyc(22);
        delay_a(1);
        next_stage(b, 0x0004);
        ld3(0x2200, 0x4E00, 0x1100, 0x2200);
        abi_VramDmaFromGsuRam();
        cpu.p &= (uint8_t)~(P_M | P_X);
        cyc(64);
        ld3(0x0240, 0xE5D7, 0x5CC0, 0x5CC0);
        abi_VramDmaFromBank7F();
        flag_0357(b, 0);
        break;
    case 0xA88F:
        cyc(22);
        delay_a(4);
        next_stage(b, 0x0006);
        cyc(24);
        op_lda16(b | 0x0D9A);
        cyc(32);
        wr16(0x7F348D, cpu.a);
        ld3(0x2400, 0x7000, 0x2200, 0x2400);
        abi_VramDmaFromGsuRam();
        cpu.p |= P_M;
        cyc(64);
        put8(BG12NBA, 0x60);
        put8(BG1SC, 0x78);
        copy8(BG2HOFS, b | 0x0D92);
        copy8(BG2HOFS, b | 0x0D93);
        copy8(BG2VOFS, b | 0x0D8E);
        copy8(BG2VOFS, b | 0x0D8F);
        cpu.p &= (uint8_t)~P_M;
        cpu.a = 0xFFFF;
        cpu.p = nz16(cpu.p, 0xFFFF);
        cyc(78);
        wr16(0x000D81, 0xFFFF);
        frame_done(b);
        break;
    case 0xA8FA:
        cyc(46);
        sel = rd16(b | 0x0D7F);
        cyc(22);
        cpu.a = sel;
        cpu.p = nz16(cpu.p, sel);
        if (sel) {
            cyc(18);
            upload_sprites_palette(b);
            break;
        }
        delay_a(1);
        next_stage(b, 0x0008);
        ld3(0x2200, 0xB800, 0x3400, 0x2200);
        abi_VramDmaFromGsuRam();
        cpu.p &= (uint8_t)~(P_M | P_X);
        cyc(22);
        ld3(0x0200, 0xE817, 0x5DE0, 0x5DE0);
        abi_VramDmaFromBank7F();
        break;
    case 0xA92C:
        delay_a(1);
        next_stage(b, 0x000A);
        ld3(0x2200, 0xDA00, 0x4500, 0x2200);
        abi_VramDmaFromGsuRam();
        cpu.p &= (uint8_t)~(P_M | P_X);
        cyc(64);
        ld3(0x0240, 0xEA17, 0x5EE0, 0x5EE0);
        abi_VramDmaFromBank7F();
        flag_0357(b, 0);
        break;
    case 0xA95B:
        delay_a(4);
        cpu.a = 0;
        cpu.p = nz16(cpu.p, 0);
        cyc(86);
        wr16(b | 0x0D7D, 0x0000);
        cyc(24);
        op_lda16(b | 0x0D9C);
        cyc(32);
        wr16(0x7F348D, cpu.a);
        ld3(0x2400, 0x9400, 0x2200, 0x2400);
        abi_VramDmaFromGsuRam();
        cpu.p |= P_M;
        cyc(64);
        copy8(BG2HOFS, b | 0x0D94);
        copy8(BG2HOFS, b | 0x0D95);
        copy8(BG2VOFS, b | 0x0D90);
        copy8(BG2VOFS, b | 0x0D91);
        put8(BG12NBA, 0x62);
        put8(BG1SC, 0x7C);
        cpu.p &= (uint8_t)~P_M;
        cpu.a = 0x0001;
        cpu.p = nz16(cpu.p, 0x0001);
        cyc(70);
        wr16(b | 0x0D81, 0x0001);
        frame_done(b);
        break;
    case 0xA9B1:
        upload_rows(b, 1, 0x0010, 0x2C00, 0x0000, 0);
        break;
    case 0xA9D1:
        break;
    case 0xA9D3:
        upload_rows(b, 0, 0x0014, 0x9400, 0x0B40, 0x40);
        break;
    case 0xA9F8:
        break;
    case 0xA9FA:
        upload_rows(b, 1, 0x0018, 0x2C00, 0x2000, 0);
        break;
    case 0xAA1A:
        break;
    case 0xAA1C:
        upload_rows(b, 0, 0x000C, 0x9400, 0x2B40, 0x42);
        break;
    case 0xAA41:
        break;
    case 0xAA43:
        split_stage(b);
        break;
    case 0xA8E6:
        abi_CgramUploadAll();
        return 0;
    }
    cyc(28);
    return 1;
}

/*
 * @routine 7E:A7C9 NmiFrameUploadRace
 * WHAT IT DOES: NmiFrameUpload's race screen stage from its jump on: the
 * stage $0D7D by the table at $A7D1, then P pulled as NmiFrameUpload
 * pushed it. The table's entry $1C is CgramUploadAll, whose own RTS ends
 * the call with nothing pulled (from NmiFrameUpload, with P on the stack,
 * that would return to the wrong place: the game never sets stage $1C).
 * HOW: REP #$30, JMP (table,X); the stages end PLP, RTS.
 * Entry: D = $0000, data bank $00; NmiFrameUpload's P on the stack (not
 * for stage $1C). Exit: as NmiFrameUpload.
 */
void abi_NmiFrameUploadRace(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;

    if (!race_stage(b))
        return;
    cyc(20);
    op_plp(rd8((uint16_t)(cpu.s + 1)));
    cpu.s = (uint16_t)(cpu.s + 1);
}

/*
 * @routine 7E:A7F3 VramRowsCopyUnused
 * WHAT IT DOES: Copies Y words from the data bank at X to VMDATAL, X
 * stepping 2 each. Nothing jumps to it or calls it.
 * HOW: LDA abs,X; STA long $2118; INX twice, DEY, BNE; RTS.
 * Entry: M 16-bit, X 16-bit; X = source, Y = count (not 0). Exit: Y = 0,
 * X past the last word; A = the last word; Z set.
 */
void abi_VramRowsCopyUnused(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;

    do {
        cyc(30);
        op_lda16(b | cpu.x);
        cyc(32);
        wr16(0x002118, cpu.a);
        cpu.x = (uint16_t)(cpu.x + 2);
        cpu.y = (uint16_t)(cpu.y - 1);
        cpu.p = nz16(cpu.p, cpu.y);
    } while (cpu.y);
    cyc(100);
}

/*
 * @routine 7E:A800 NmiFrameUploadSprites
 * WHAT IT DOES: NmiFrameUpload's shared end for the stages with nothing to
 * upload: with $0D28 set it is cleared and, with $0D54 clear, the sprite
 * table goes out (OamDma, $0375 bytes from $0220); then the palette
 * (CgramUploadAll); then P pulled as NmiFrameUpload pushed it.
 * HOW: Two short delay loops; SEP #$20/REP #$10; PLP, RTS.
 * Entry: D = $0000, data bank $00; NmiFrameUpload's P on the stack.
 * Exit: as NmiFrameUpload; X = 0.
 */
void abi_NmiFrameUploadSprites(void)
{
    upload_sprites_palette((uint32_t)cpu.db << 16);
    cyc(20);
    op_plp(rd8((uint16_t)(cpu.s + 1)));
    cpu.s = (uint16_t)(cpu.s + 1);
}

/*
 * @routine 03:ECBE VramClearWords
 * WHAT IT DOES: Clears X words of VRAM from word address A. Nothing in the
 * game calls it.
 * HOW: VMAIN = $80 (a 16-bit store of Y = $0080, which also writes 0 to
 * VMADDL), VMADDL = A, then X 16-bit zero stores to VMDATAL; the
 * registers through the data bank.
 * Entry: M 16-bit, X 16-bit; A = VRAM word address, X = count (not 0).
 * Exit: A = 0; X = 0; Y = $0080; Z set, N clear.
 */
void VramClearWords(uint8_t bank, uint16_t at, uint16_t n)
{
    uint32_t b = (uint32_t)bank << 16;

    cyc(48);
    wr16(b | 0x2115, 0x0080);
    cyc(24);
    wr16(b | 0x2116, at);
    do {
        cyc(60);
        wr16(b | 0x2118, 0x0000);
        n = (uint16_t)(n - 1);
    } while (n);
    cyc(72);
}

void abi_VramClearWords(void)
{
    VramClearWords(cpu.db, cpu.a, cpu.x);
    cpu.a = 0x0000;
    cpu.x = 0x0000;
    cpu.y = 0x0080;
    cpu.p = nz16(cpu.p, 0x0000);
}
