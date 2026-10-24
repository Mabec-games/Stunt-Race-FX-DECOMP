/*
 * tilemap_dma.c: two whole BG2 tilemaps uploaded to VRAM $5C00 by DMA
 * from work RAM bank $7F ($7F:E817 and $7F:E3D7, $440 bytes each), with
 * the matching gradient source chosen, and the two entries that also set
 * BG2's scroll for a screen band first. RAM code (copied to bank $7E at
 * boot); no play runs it; unit_tilemap_dma calls it.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "carried.h"
#include "wram_names.h"
#include "wram_struct.h"


void abi_Bg2ScrollFromTable(void);

void TilemapDmaE3D7_cr(Carried *cr);
void TilemapDmaE817_cr(Carried *cr);
void Bg2Band0TilemapE3D7(Carried *cr);
void Bg2Band2TilemapE817(Carried *cr);
void TilemapDmaE3D7(Carried *cr);
void TilemapDmaE817(Carried *cr);
static void upload(Carried *cr, uint16_t gradient_src, uint16_t src, int bra);
void Bg2ScrollFromTable_cr(Carried *cr);

/* The gradient source word at gradient_src (through the data bank) to
   $7F:348D, then channel 0 from $7F:src to VMDATA at VRAM $5C00, $440
   bytes; P saved and restored. */
static void upload(Carried *cr, uint16_t gradient_src, uint16_t src, int bra)
{
    uint32_t b = (uint32_t)cr->db << 16;
    uint8_t p = cr->p;
    uint16_t v;

    cyc(68);
    v = rd16(0x7E0000 | gradient_src);
    cyc(32);
    WR16(gradient_ptr_a, v);
    cyc(48);
    dma_wr16(b | 0x4302, src);
    cyc(62);
    dma_wr(b | 0x4304, 0x7F);
    cyc(bra ? 62 : 40);                 /* BRA into the shared tail, or not */
    ppu_wr(b | 0x2115, 0x80);
    cyc(48);
    ppu_wr16(b | 0x2116, 0x5C00);
    cyc(48);
    dma_wr16(b | 0x4300, 0x1801);
    cyc(48);
    dma_wr16(b | 0x4305, 0x0440);
    cyc(40);
    mdmaen_wr(b | 0x420B, 0x01);
    cr->a = (uint16_t)((v & 0xFF00) | 0x01);
    cr->x = 0x0440;
    cyc(28);
    r_plp(cr, p);
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
void TilemapDmaE817(Carried *cr)
{
    upload(cr, W(wram_0D9C), 0xE817, 1);
}

void TilemapDmaE817_cr(Carried *cr)
{
    upload(cr, W(wram_0D9C), 0xE817, 1);
}

void abi_TilemapDmaE817(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    TilemapDmaE817_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 7E:B2CF TilemapDmaE3D7
 * WHAT IT DOES: As TilemapDmaE817 for the gradient source at $0D9A and the
 * tilemap at $7F:E3D7.
 * HOW: As TilemapDmaE817 (a LDA #$EC57 is loaded over at once).
 * Entry: as TilemapDmaE817. Exit: as TilemapDmaE817.
 */
void TilemapDmaE3D7(Carried *cr)
{
    upload(cr, W(wram_0D9A), 0xE3D7, 0);
}

void TilemapDmaE3D7_cr(Carried *cr)
{
    cyc(24);
    upload(cr, W(wram_0D9A), 0xE3D7, 0);
}

void abi_TilemapDmaE3D7(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    TilemapDmaE3D7_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 7E:B236 Bg2Band2TilemapE817
 * WHAT IT DOES: Sets BG2's scroll from screen band 2 (Bg2ScrollFromTable),
 * then TilemapDmaE817.
 * HOW: 8-bit registers; X = 2.
 * Entry: DB as TilemapDmaE817. Exit: M, X 8-bit; A = 1 with the gradient
 * source's high byte; X = $40; N, Z from the scroll's last byte.
 */
void Bg2Band2TilemapE817(Carried *cr)
{
    set_p(cr, (uint8_t)(cr->p | (P_M | P_X)));
    cr->x = 0x02;
    cr->y &= 0x00FF;
    (cyc(84), Bg2ScrollFromTable_cr(cr));
    cyc(46);
    upload(cr, W(wram_0D9C), 0xE817, 1);
    cyc(42);                            /* RTS */
}

void abi_Bg2Band2TilemapE817(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    Bg2Band2TilemapE817(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 7E:B241 Bg2Band0TilemapE3D7
 * WHAT IT DOES: Sets BG2's scroll from screen band 0, then
 * TilemapDmaE3D7.
 * HOW: 8-bit registers; X = 0.
 * Entry: as Bg2Band2TilemapE817. Exit: as Bg2Band2TilemapE817.
 */
void Bg2Band0TilemapE3D7(Carried *cr)
{
    set_p(cr, (uint8_t)(cr->p | (P_M | P_X)));
    cr->x = 0x00;
    cr->y &= 0x00FF;
    (cyc(84), Bg2ScrollFromTable_cr(cr));
    cyc(70);
    upload(cr, W(wram_0D9A), 0xE3D7, 0);
    cyc(42);                            /* RTS */
}

void abi_Bg2Band0TilemapE3D7(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    Bg2Band0TilemapE3D7(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

