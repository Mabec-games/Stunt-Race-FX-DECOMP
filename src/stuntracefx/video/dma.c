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

/* DMA channel 0, mode 1 (two registers, write once) to VMDATAL/H. */
static void vram_channel(uint16_t size, uint16_t src, uint16_t vram)
{
    wr16(DAS0L, size);
    wr16(A1T0L, src);
    wr16(VMADDL, vram);
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
    wr8(A1B0, 0x7F);
    wr8(VMAIN, 0x80);
    wr8(MDMAEN, 0x01);
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
    wr8(A1B0, 0x70);
    wr8(VMAIN, 0x80);
    wr8(GSU_SCMR, rd8(GSU_SCMR_SHADOW) & 0xF7);
    wr8(MDMAEN, 0x01);
    mode = rd8(GSU_SCMR_SHADOW) | 0x18;
    wr8(GSU_SCMR, mode);
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

    wr16(VMADDL, vram);
    wr16(DMAP0, 0x1801);
    wr8(A1B0, 0x70);
    wr8(VMAIN, 0x80);
    wr16(GSU_SCMR, rd16(GSU_SCMR_SHADOW) & 0xFFF7);
    for (;;) {
        wr16(A1T0L, src);
        wr16(DAS0L, 0x0140);
        wr8(MDMAEN, 0x01);
        if (--rows == 0)
            break;
        src = (uint16_t)(src + 0x0200);
    }
    mode = rd16(GSU_SCMR_SHADOW) | 0x0018;
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
    wr16(A1T0L, src);
    wr8(OAMADDL, 0x00);
    wr8(OAMADDH, 0x00);
    wr16(DAS0L, size);
    wr16(DMAP0, 0x0400);
    wr8(A1B0, 0x00);
    wr8(MDMAEN, 0x01);
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
    OamDma(OAM_BUFFER, 0x0220);
}

void abi_OamDmaAll(void)
{
    OamDmaAll();
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x01);
    cpu.x = (cpu.p & P_X) ? 0x00 : 0x0400;
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
    wr16(A1T0L, src);
    wr16(DAS0L, size);
    wr16(DMAP0, 0x2200);
    wr8(CGADD, colour);
    wr8(A1B0, 0x7E);
    wr8(MDMAEN, 0x01);
}

void abi_CgramDma(void)
{
    CgramDma(cpu.a, (uint8_t)cpu.x, cpu.y);
    cpu.a = 0x2201;
    cpu.x &= 0x00FF;
    cpu.y &= 0x00FF;
    cpu.p = (uint8_t)((cpu.p & ~(P_N | P_Z)) | P_M | P_X);
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
    CgramDma(PALETTE_BUFFER, 0x00, 0x0200);
}

void abi_CgramDmaAll(void)
{
    CgramDmaAll();
    cpu.a = 0x2201;
    cpu.x = 0;
    cpu.y = 0;
}
