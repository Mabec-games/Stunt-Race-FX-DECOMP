/*
 * tilemap_dma.c: two whole BG2 tilemaps uploaded to VRAM $5C00 by DMA
 * from work RAM bank $7F ($7F:E817 and $7F:E3D7, $440 bytes each), with
 * the matching gradient source chosen, and the two entries that also set
 * BG2's scroll for a screen band first. RAM code (copied to bank $7E at
 * boot); no play runs it; unit_tilemap_dma calls it.
 */
#include "machine.h"
#include "cpu_ops.h"

void abi_Bg2ScrollFromTable(void);

/* The gradient source word at gradient_src (through the data bank) to
   $7F:348D, then channel 0 from $7F:src to VMDATA at VRAM $5C00, $440
   bytes; P saved and restored. */
static void upload(uint16_t gradient_src, uint16_t src)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint8_t p = cpu.p;
    uint16_t v;

    v = rd16(b | gradient_src);
    wr16(0x7F348D, v);
    wr16(b | 0x4302, src);
    wr8(b | 0x4304, 0x7F);
    wr8(b | 0x2115, 0x80);
    wr16(b | 0x2116, 0x5C00);
    wr16(b | 0x4300, 0x1801);
    wr16(b | 0x4305, 0x0440);
    wr8(b | 0x420B, 0x01);
    cpu.a = (uint16_t)((v & 0xFF00) | 0x01);
    cpu.x = 0x0440;
    op_plp(p);
}

/*
 * @routine 7E:B2B6 TilemapDmaE817
 * WHAT IT DOES: Chooses the gradient source at $0D9C ($7F:348D = it) and
 * uploads the tilemap at $7F:E817 to VRAM $5C00 ($440 bytes, DMA channel
 * 0).
 * HOW: 16-bit, the DMA registers through the data bank; joins
 * TilemapDmaE3D7 for the rest of the set-up (VMAIN $80, address, mode
 * $1801, size, MDMAEN). P saved and restored.
 * Entry: any widths; DB a bank where the PPU and DMA registers are.
 * Exit: P unchanged; A = 1 with the gradient source's high byte; X = $440
 * (its low byte with X 8-bit).
 */
void TilemapDmaE817(void)
{
    upload(0x0D9C, 0xE817);
}

void abi_TilemapDmaE817(void)
{
    cyc(68);
    upload(0x0D9C, 0xE817);
}

/*
 * @routine 7E:B2CF TilemapDmaE3D7
 * WHAT IT DOES: As TilemapDmaE817 for the gradient source at $0D9A and the
 * tilemap at $7F:E3D7.
 * HOW: As TilemapDmaE817 (a LDA #$EC57 is loaded over at once).
 * Entry: as TilemapDmaE817. Exit: as TilemapDmaE817.
 */
void TilemapDmaE3D7(void)
{
    upload(0x0D9A, 0xE3D7);
}

void abi_TilemapDmaE3D7(void)
{
    cyc(92);
    upload(0x0D9A, 0xE3D7);
}

/*
 * @routine 7E:B236 Bg2Band2TilemapE817
 * WHAT IT DOES: Sets BG2's scroll from screen band 2 (Bg2ScrollFromTable),
 * then TilemapDmaE817.
 * HOW: 8-bit registers; X = 2.
 * Entry: DB as TilemapDmaE817. Exit: M, X 8-bit; A = 1 with the gradient
 * source's high byte; X = $40; N, Z from the scroll's last byte.
 */
void Bg2Band2TilemapE817(void)
{
    cpu.p |= P_M | P_X;
    cpu.x = 0x02;
    cpu.y &= 0x00FF;
    (cyc(84), abi_Bg2ScrollFromTable());
    cyc(156);
    upload(0x0D9C, 0xE817);
}

void abi_Bg2Band2TilemapE817(void)
{
    Bg2Band2TilemapE817();
}

/*
 * @routine 7E:B241 Bg2Band0TilemapE3D7
 * WHAT IT DOES: Sets BG2's scroll from screen band 0, then
 * TilemapDmaE3D7.
 * HOW: 8-bit registers; X = 0.
 * Entry: as Bg2Band2TilemapE817. Exit: as Bg2Band2TilemapE817.
 */
void Bg2Band0TilemapE3D7(void)
{
    cpu.p |= P_M | P_X;
    cpu.x = 0x00;
    cpu.y &= 0x00FF;
    (cyc(84), abi_Bg2ScrollFromTable());
    cyc(180);
    upload(0x0D9A, 0xE3D7);
}

void abi_Bg2Band0TilemapE3D7(void)
{
    Bg2Band0TilemapE3D7();
}
