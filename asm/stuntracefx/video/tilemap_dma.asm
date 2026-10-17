; tilemap_dma.asm: two whole BG2 tilemaps uploaded to VRAM $5C00 by DMA from
; work RAM bank $7F, and the entries that set the BG2 scroll first. RAM code.
; C: src/stuntracefx/video/tilemap_dma.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 7E:B236 Bg2Band2TilemapE817
; WHAT IT DOES: Sets BG2's scroll from screen band 2 (Bg2ScrollFromTable),
; then TilemapDmaE817.
; HOW: 8-bit registers; X = 2.
; Entry: DB as TilemapDmaE817. Exit: M, X 8-bit; A = 1 with the gradient
; source's high byte; X = $40; N, Z from the scroll's last byte.
%ramcode($7EB236, $010F5D)
Bg2Band2TilemapE817:
    sep.b #$30
    ldx.b #$02
    jsr.w Bg2ScrollFromTable
    jsr.w TilemapDmaE817
    rts

; @routine 7E:B241 Bg2Band0TilemapE3D7
; WHAT IT DOES: Sets BG2's scroll from screen band 0, then
; TilemapDmaE3D7.
; HOW: 8-bit registers; X = 0.
; Entry: as Bg2Band2TilemapE817. Exit: as Bg2Band2TilemapE817.
%ramcode($7EB241, $010F68)
Bg2Band0TilemapE3D7:
    sep.b #$30
    ldx.b #$00
    jsr.w Bg2ScrollFromTable
    jsr.w TilemapDmaE3D7
    rts

; @routine 7E:B2B6 TilemapDmaE817
; WHAT IT DOES: Chooses the gradient source at $0D9C ($7F:348D = it) and
; uploads the tilemap at $7F:E817 to VRAM $5C00 ($440 bytes, DMA channel
; 0).
; HOW: 16-bit, the DMA registers through the data bank; joins
; TilemapDmaE3D7 for the rest of the set-up (VMAIN $80, address, mode
; $1801, size, MDMAEN). P saved and restored.
; Entry: any widths; DB a bank where the PPU and DMA registers are.
; Exit: P unchanged; A = 1 with the gradient source's high byte; X = $440
; (its low byte with X 8-bit).
%ramcode($7EB2B6, $010FDD)
TilemapDmaE817:
    php
    rep.b #$30
    lda.w !gradient_src_b+4
    sta.l !gradient_ptr_a
    ldx.w #$E817
    stx.w !A1T0L
    sep.b #$20
    lda.b #$7F
    sta.w !A1B0
    bra TilemapDmaUpload

; @routine 7E:B2CF TilemapDmaE3D7
; WHAT IT DOES: As TilemapDmaE817 for the gradient source at $0D9A and the
; tilemap at $7F:E3D7.
; HOW: As TilemapDmaE817 (a LDA #$EC57 is loaded over at once).
; Entry: as TilemapDmaE817. Exit: as TilemapDmaE817.
%ramcode($7EB2CF, $010FF6)
TilemapDmaE3D7:
    php
    rep.b #$30
    lda.w #$EC57
    lda.w !gradient_src_b+2
    sta.l !gradient_ptr_a
    ldx.w #$E3D7
    stx.w !A1T0L
    sep.b #$20
    lda.b #$7F
    sta.w !A1B0
TilemapDmaUpload:
    lda.b #$80
    sta.w !VMAIN
    ldx.w #$5C00
    stx.w !VMADDL
    ldx.w #$1801
    stx.w !DMAP0
    ldx.w #$0440
    stx.w !DAS0L
    lda.b #$01
    sta.w !MDMAEN
    plp
    rts
