; dma.asm: the DMA transfers that feed the PPU: VRAM from work RAM or Super FX
; RAM, the sprite table, the palette. All use DMA channel 0 and run from work
; RAM (main_wram in config/stuntracefx/ramcode.csv). To read Super FX RAM the
; S-CPU must own it: the transfers from bank $70 clear the RAN bit of the
; Super FX screen mode register for the transfer, then give ROM and RAM back.
; C: src/stuntracefx/video/dma.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"

arch 65816

; @routine 7E:ABB6 VramDmaFromBank7F
; WHAT IT DOES: Copies a block of work RAM bank $7F to VRAM.
; HOW: Channel 0 in mode 1 to VMDATA, VRAM incrementing after the high byte,
; then starts the transfer.
; Entry: M 16-bit, X 16-bit; A = byte count, X = source address in bank $7F,
; Y = VRAM word address.
; Exit: A = $1801; M 8-bit; N, Z clear; X, Y unchanged.
%ramcode($7EABB6, $0108DD)
VramDmaFromBank7F:
    sta.w !DAS0L
    stx.w !A1T0L
    sty.w !VMADDL
    lda.w #$1801
    sta.w !DMAP0
    sep.b #$20
    lda.b #$7F
    sta.w !A1B0
    lda.b #$80
    sta.w !VMAIN
    lda.b #$01
    sta.w !MDMAEN
    rts

; @routine 7E:ABD7 VramDmaFromGsuRam
; WHAT IT DOES: Copies a block of Super FX RAM (bank $70) to VRAM.
; HOW: As VramDmaFromBank7F from bank $70, with the RAN bit cleared in the
; Super FX screen mode register for the transfer, then bits 4 and 3 set.
; Entry: M 16-bit, X 16-bit; A = byte count, X = source address, Y = VRAM
; word address.
; Exit: A = $18 high, the screen mode value written last low; M 8-bit; N, Z
; from that value; X, Y unchanged.
VramDmaFromGsuRam:
    sta.w !DAS0L
    stx.w !A1T0L
    sty.w !VMADDL
    lda.w #$1801
    sta.w !DMAP0
    sep.b #$20
    lda.b #$70
    sta.w !A1B0
    lda.b #$80
    sta.w !VMAIN
    lda.w !gsu_scmr_shadow
    and.b #$F7
    sta.w !GSU_SCMR
    lda.b #$01
    sta.w !MDMAEN
    lda.w !gsu_scmr_shadow
    ora.b #$18
    sta.w !GSU_SCMR
    rts

; @routine 7E:AC08 VramDmaRowsFromGsuRam
; WHAT IT DOES: Copies rows of $140 bytes from Super FX RAM to consecutive
; VRAM, taking each row $200 bytes further on in Super FX RAM.
; HOW: Sets the VRAM address and channel 0 once, takes the RAM from the
; Super FX (a 16-bit store to the screen mode register and the byte after
; it), then per row sets the source, a count of $140 and starts the transfer.
; Gives ROM and RAM back at the end.
; Entry: M 16-bit, X 16-bit; A = first row's address, X = VRAM word address,
; Y = row count (1-255; the routine switches to 8-bit index registers).
; Exit: A = the word at $0374 with bits 3 and 4 set; X = 1; Y = 0; X flag
; set; N, Z from A; C and V from the last row step.
VramDmaRowsFromGsuRam:
    stx.w !VMADDL
    ldx.w #$1801
    stx.w !DMAP0
    sep.b #$10
    ldx.b #$70
    stx.w !A1B0
    ldx.b #$80
    stx.w !VMAIN
    pha
    lda.w !gsu_scmr_shadow
    and.w #$FFF7
    sta.w !GSU_SCMR
    pla
.row:
    sta.w !A1T0L
    rep.b #$10
    ldx.w #$0140
    stx.w !DAS0L
    sep.b #$10
    ldx.b #$01
    stx.w !MDMAEN
    dey
    beq .done
    clc
    adc.w #$0200
    bra .row
.done:
    lda.w !gsu_scmr_shadow
    ora.w #$0018
    sta.w !GSU_SCMR
    rts

; @routine 7E:ACF7 OamDma
; WHAT IT DOES: Copies a block of work RAM to OAM from its start.
; HOW: OAM address 0, then channel 0 in mode 0 to OAMDATA from bank $00.
; Entry: M 8-bit, X 16-bit; X = source address, Y = byte count.
; Exit: A low byte = 1; X = $0400; N, Z clear; Y unchanged.
%ramcode($7EACF7, $010A1E)
OamDma:
    stx.w !A1T0L
    lda.b #$00
    sta.w !OAMADDL
    sta.w !OAMADDH
    sty.w !DAS0L
    ldx.w #$0400
    stx.w !DMAP0
    lda.b #$00
    sta.w !A1B0
    lda.b #$01
    sta.w !MDMAEN
    rts

; @routine 7E:AD16 OamDmaAll
; WHAT IT DOES: Copies the whole work-RAM sprite table to OAM.
; HOW: OamDma of the $220 bytes from $0375 (the table and its high table).
; Entry: any widths. Exit: A low byte = 1; X = $0400 and Y = $0220 (or their
; low bytes when entered with 8-bit index registers); P unchanged.
OamDmaAll:
    php
    sep.b #$20
    rep.b #$10
    ldx.w #!oam_buffer
    ldy.w #$0220
    jsr.w OamDma
    plp
    rtl

; @routine 7E:B0E5 CgramDmaAll
; WHAT IT DOES: Copies the whole palette buffer ($7E:FD65, 512 bytes) to the
; palette.
; HOW: CgramDma from colour 0.
; Entry: any widths. Exit: A = $2201; X = 0, Y = 0; P unchanged.
%ramcode($7EB0E5, $010E0C)
CgramDmaAll:
    php
    rep.b #$30
    lda.w #$FD65
    ldx.w #$0000
    ldy.w #$0200
    jsr.w CgramDma
    plp
    rtl

; @routine 7E:B0F6 CgramDma
; WHAT IT DOES: Copies a block of work RAM bank $7E to the palette.
; HOW: Channel 0 in mode 0 to CGDATA; the low byte of X is the first colour.
; Entry: M 16-bit, X 16-bit; A = source address, X = first colour, Y = byte
; count. Exit: A = $2201; M and X 8-bit; N, Z clear.
CgramDma:
    sta.w !A1T0L
    sty.w !DAS0L
    lda.w #$2200
    sta.w !DMAP0
    sep.b #$30
    stx.w !CGADD
    lda.b #$7E
    sta.w !A1B0
    lda.b #$01
    sta.w !MDMAEN
    rts

; @routine 7F:08C7 VramDmaWideRowsFromGsuRam
; WHAT IT DOES: Copies rows of $2C0 bytes from Super FX RAM to consecutive
; VRAM, each row $400 bytes further on in Super FX RAM.
; HOW: Takes the RAM from the Super FX, keeps the source in direct page
; $FB-$FC, sets the VRAM address and channel 0 (mode 1, bank $70) once, then
; transfers rows while the 8-bit row counter stays non-negative; gives ROM
; and RAM back. Runs from work RAM bank $7F.
; Entry: M 16-bit, X 16-bit, D = $0000; A low byte = rows - 1, X = source,
; Y = VRAM word address. Exit: A = the source high byte and the screen mode
; value; X = $00FF; Y = 1; P unchanged.
%ramcode($7F08C7, $0580C7)
VramDmaWideRowsFromGsuRam:
    php
    rep.b #$30
    pha
    sep.b #$20
    lda.w !gsu_scmr_shadow
    and.b #$F7
    sta.w !GSU_SCMR
    rep.b #$20
    stx.b !dp_fb
    sty.w !VMADDL
    sep.b #$10
    rep.b #$20
    ldx.b #$80
    stx.w !VMAIN
    ldx.b #$18
    stx.w !DMAP0+1
    ldy.b #$70
    sty.w !A1B0
    ldy.b #$01
    sty.w !DMAP0
    pla
    tax
.row:
    lda.w #$02C0
    sta.w !DAS0L
    lda.b !dp_fb
    sta.w !A1T0L
    clc
    adc.w #$0400
    sta.b !dp_fb
    sty.w !MDMAEN
    dex
    bpl .row
    sep.b #$20
    lda.w !gsu_scmr_shadow
    ora.b #$18
    sta.w !GSU_SCMR
    plp
    rts

; @routine 7F:0BCE VramWriteRowFromGsuRam
; WHAT IT DOES: Copies one row of 64 words from Super FX RAM ($70:3000 plus
; the offset in direct page $02) to VRAM, then moves the offset down a row
; ($200 bytes).
; HOW: Sets the VRAM address and writes the words to VMDATA one by one.
; Entry: M 16-bit, X 16-bit, D = $0000; A = VRAM word address.
; Exit: A = the new offset; X = the old offset + $80; Y = $FFFF; P unchanged.
%ramcode($7F0BCE, $0583CE)
VramWriteRowFromGsuRam:
    php
    rep.b #$30
    sta.w !VMADDL
    ldy.w #$003F
    ldx.b !dp_02
.word:
    lda.l !gsu_picture,x
    sta.w !VMDATAL
    inx
    inx
    dey
    bpl .word
    lda.b !dp_02
    clc
    adc.w #$0200
    sta.b !dp_02
    plp
    rts
