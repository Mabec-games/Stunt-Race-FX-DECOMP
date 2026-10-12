; window_hdma.asm: see the C, src/stuntracefx/video/window_hdma.c, for what this file covers.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"

arch 65816

; @routine 7E:D6E8 WindowHdmaFromList
; WHAT IT DOES: Sets the window table to one normal band of 32 lines followed
; by the entries of the list X selects, up to its end marker.
; HOW: The list's address comes from the table at $7E:D8DE; its bytes are
; copied (in the data bank) from the fourth table byte on, end marker
; included.
; Entry: M 8-bit, X 8-bit; DB the bank the lists are in. Exit: A low byte =
; 0; X = 3 + bytes copied; Y past the end marker's address... see wrapper;
; M 8-bit, X 16-bit; Z set.
%ramcode($7ED6E8, $01340F)
WindowHdmaFromList:
    rep.b #$20
    sep.b #$10
    lda.l !window_hdma_lists,x
    rep.b #$10
    tay
    sep.b #$20
    lda.b #$20
    sta.l !window_hdma
    rep.b #$20
    lda.w #$F857
    sta.l !window_hdma+$1
    sep.b #$20
    ldx.w #$0003
.lD709:
    lda.w 0,y
    sta.l !window_hdma,x
    beq .lD717
    iny
    inx
    jmp.w .lD709
.lD717:
    rts

; @routine 7E:D85C WindowHdmaAt
; WHAT IT DOES: Sets the window table to four bands around line A: normal
; windows in the middle two bands, $FFDD above and below.
; HOW: Band heights come from A and the offsets at $19E2-$19E5 and the height
; at $19DC.
; Entry: M 8-bit; A low byte = line. Exit: A = $FFDD; M 16-bit; N set, Z
; clear; C and V from the last height sum.
%ramcode($7ED85C, $013583)
WindowHdmaAt:
    sep.b #$20
    pha
    clc
    adc.w !window_19e5
    sta.l !window_hdma+$3
    pla
    clc
    adc.w !window_19e3
    sta.l !window_hdma+$6
    lda.w !window_lines
    sec
    sbc.b #$01
    clc
    adc.w !window_19e4
    sta.l !window_hdma
    lda.w !window_lines
    clc
    adc.w !window_19e2
    sta.l !window_hdma+$9
    lda.b #$00
    sta.l !window_hdma+$C
    rep.b #$20
    lda.w #$F857
    sta.l !window_hdma+$4
    sta.l !window_hdma+$7
    lda.w #$FFDD
    sta.l !window_hdma+$1
    sta.l !window_hdma+$A
    rts

; @routine 7E:D8A8 WindowHdmaFFDD
; WHAT IT DOES: Sets the window table to four bands (64, 64, 64, 62 lines),
; all using the window values at $FFDD.
; HOW: Writes the four counts and the end marker, then the four addresses.
; Entry: any widths. Exit: A = $FFDD; M 16-bit; N set, Z clear.
%ramcode($7ED8A8, $0135CF)
WindowHdmaFFDD:
    sep.b #$20
    lda.b #$40
    sta.l !window_hdma
    lda.b #$40
    sta.l !window_hdma+$3
    lda.b #$40
    sta.l !window_hdma+$6
    lda.b #$3E
    sta.l !window_hdma+$9
    lda.b #$00
    sta.l !window_hdma+$C
    rep.b #$20
    lda.w #$FFDD
    sta.l !window_hdma+$1
    sta.l !window_hdma+$4
    sta.l !window_hdma+$7
    sta.l !window_hdma+$A
    rts

; @routine 7E:DB10 WindowHdmaFromTable
; WHAT IT DOES: Sets the window table to three bands from the layout $19B6
; selects: normal, the selected window values, normal.
; HOW: The first two counts and the middle address come from tables at
; $7E:E0B4 and $7E:E0D2; the third band is $30 lines.
; Entry: any widths. Exit: A low byte = 0 (high byte that of the middle
; address); X = $19B6; M 8-bit, X 16-bit; Z set, N clear.
%ramcode($7EDB10, $013837)
WindowHdmaFromTable:
    rep.b #$30
    ldx.w !window_hdma_select
    lda.w #$F857
    sta.l !window_hdma+$1
    sta.l !window_hdma+$7
    lda.l !window_hdma_addrs,x
    sta.l !window_hdma+$4
    sep.b #$20
    lda.l !window_hdma_counts,x
    sta.l !window_hdma
    lda.l !window_hdma_counts+$1,x
    sta.l !window_hdma+$3
    lda.b #$30
    sta.l !window_hdma+$6
    lda.b #$00
    sta.l !window_hdma+$9
    rts

; @routine 7E:DBCD WindowHdmaNormal
; WHAT IT DOES: Sets the window table to the normal window over the whole
; screen (four bands of 64 lines), unless window HDMA is off or no layout is
; set ($19BC = $FF).
; HOW: Four entries at $F857 and the end marker.
; Entry: any widths. Exit: A = $F857 when written; X and Y high bytes clear;
; P unchanged.
%ramcode($7EDBCD, $0138F4)
WindowHdmaNormal:
    php
    phb
    phk
    plb
    sep.b #$30
    lda.w !window_hdma_off
    bne .lDC15
    lda.l !window_hdma_19bc
    cmp.b #$FF
    beq .lDC15
    sep.b #$20
    lda.b #$40
    sta.l !window_hdma
    lda.b #$40
    sta.l !window_hdma+$3
    lda.b #$40
    sta.l !window_hdma+$6
    lda.b #$40
    sta.l !window_hdma+$9
    lda.b #$00
    sta.l !window_hdma+$C
    rep.b #$20
    lda.w #$F857
    sta.l !window_hdma+$1
    sta.l !window_hdma+$4
    sta.l !window_hdma+$7
    sta.l !window_hdma+$A
.lDC15:
    plb
    plp
    rtl

; @routine 7E:DC18 WindowHdmaBands
; WHAT IT DOES: Sets the window table to nine normal bands (23, 16, 48, 16,
; 24, 16, 48, 16, 1 lines), unless window HDMA is off.
; HOW: Nine entries at $F857 and the end marker.
; Entry: any widths. Exit: A = $F857 when written, else $19D8; X and Y high
; bytes clear; P unchanged.
%ramcode($7EDC18, $01393F)
WindowHdmaBands:
    php
    phb
    phk
    plb
    sep.b #$30
    lda.w !window_hdma_off
    bne .lDC8A
    sep.b #$20
    lda.b #$17
    sta.l !window_hdma
    lda.b #$10
    sta.l !window_hdma+$3
    lda.b #$30
    sta.l !window_hdma+$6
    lda.b #$10
    sta.l !window_hdma+$9
    lda.b #$18
    sta.l !window_hdma+$C
    lda.b #$10
    sta.l !window_hdma+$F
    lda.b #$30
    sta.l !window_hdma+$12
    lda.b #$10
    sta.l !window_hdma+$15
    lda.b #$01
    sta.l !window_hdma+$18
    lda.b #$00
    sta.l !window_hdma+$1B
    rep.b #$20
    lda.w #$F857
    sta.l !window_hdma+$1
    sta.l !window_hdma+$4
    sta.l !window_hdma+$7
    sta.l !window_hdma+$A
    sta.l !window_hdma+$D
    sta.l !window_hdma+$10
    sta.l !window_hdma+$13
    sta.l !window_hdma+$16
    sta.l !window_hdma+$19
.lDC8A:
    plb
    plp
    rtl

; @routine 7E:DC8D WindowHdmaFFD1
; WHAT IT DOES: Sets the window table to four bands (32, 112, 16, 8 lines):
; normal, $FFD1, $FFD1, normal.
; HOW: Writes the counts and end marker, then the addresses.
; Entry: any widths. Exit: A = $FFD1; M 16-bit; N set, Z clear.
%ramcode($7EDC8D, $0139B4)
WindowHdmaFFD1:
    sep.b #$20
    lda.b #$20
    sta.l !window_hdma
    lda.b #$70
    sta.l !window_hdma+$3
    lda.b #$10
    sta.l !window_hdma+$6
    lda.b #$08
    sta.l !window_hdma+$9
    lda.b #$00
    sta.l !window_hdma+$C
    rep.b #$20
    lda.w #$F857
    sta.l !window_hdma+$1
    sta.l !window_hdma+$A
    lda.w #$FFD1
    sta.l !window_hdma+$4
    sta.l !window_hdma+$7
    rts
