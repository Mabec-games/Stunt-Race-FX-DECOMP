; window_hdma.asm: the window HDMA table in bank $7F ($7F:F85B). Each entry is
; three bytes: a line count and a 16-bit address of the window values for
; those lines; a zero count ends the table. $F857 is the normal window
; setting; $FFD1 and $FFDD are other ones.
; C: src/stuntracefx/video/window_hdma.c.

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
    sta.l !window_hdma+1
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
    sta.l !window_hdma+3
    pla
    clc
    adc.w !window_19e3
    sta.l !window_hdma+6
    lda.w !window_lines
    sec
    sbc.b #$01
    clc
    adc.w !window_19e4
    sta.l !window_hdma
    lda.w !window_lines
    clc
    adc.w !window_19e2
    sta.l !window_hdma+9
    lda.b #$00
    sta.l !window_hdma+12
    rep.b #$20
    lda.w #$F857
    sta.l !window_hdma+4
    sta.l !window_hdma+7
    lda.w #$FFDD
    sta.l !window_hdma+1
    sta.l !window_hdma+10
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
    sta.l !window_hdma+3
    lda.b #$40
    sta.l !window_hdma+6
    lda.b #$3E
    sta.l !window_hdma+9
    lda.b #$00
    sta.l !window_hdma+12
    rep.b #$20
    lda.w #$FFDD
    sta.l !window_hdma+1
    sta.l !window_hdma+4
    sta.l !window_hdma+7
    sta.l !window_hdma+10
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
    sta.l !window_hdma+1
    sta.l !window_hdma+7
    lda.l !window_hdma_addrs,x
    sta.l !window_hdma+4
    sep.b #$20
    lda.l !window_hdma_counts,x
    sta.l !window_hdma
    lda.l !window_hdma_counts+1,x
    sta.l !window_hdma+3
    lda.b #$30
    sta.l !window_hdma+6
    lda.b #$00
    sta.l !window_hdma+9
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
    sta.l !window_hdma+3
    lda.b #$40
    sta.l !window_hdma+6
    lda.b #$40
    sta.l !window_hdma+9
    lda.b #$00
    sta.l !window_hdma+12
    rep.b #$20
    lda.w #$F857
    sta.l !window_hdma+1
    sta.l !window_hdma+4
    sta.l !window_hdma+7
    sta.l !window_hdma+10
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
    sta.l !window_hdma+3
    lda.b #$30
    sta.l !window_hdma+6
    lda.b #$10
    sta.l !window_hdma+9
    lda.b #$18
    sta.l !window_hdma+12
    lda.b #$10
    sta.l !window_hdma+15
    lda.b #$30
    sta.l !window_hdma+18
    lda.b #$10
    sta.l !window_hdma+21
    lda.b #$01
    sta.l !window_hdma+24
    lda.b #$00
    sta.l !window_hdma+27
    rep.b #$20
    lda.w #$F857
    sta.l !window_hdma+1
    sta.l !window_hdma+4
    sta.l !window_hdma+7
    sta.l !window_hdma+10
    sta.l !window_hdma+13
    sta.l !window_hdma+16
    sta.l !window_hdma+19
    sta.l !window_hdma+22
    sta.l !window_hdma+25
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
    sta.l !window_hdma+3
    lda.b #$10
    sta.l !window_hdma+6
    lda.b #$08
    sta.l !window_hdma+9
    lda.b #$00
    sta.l !window_hdma+12
    rep.b #$20
    lda.w #$F857
    sta.l !window_hdma+1
    sta.l !window_hdma+10
    lda.w #$FFD1
    sta.l !window_hdma+4
    sta.l !window_hdma+7
    rts

; @routine 7E:D718 WindowHdmaPreset
; WHAT IT DOES: Copies a 9-byte preset into the window HDMA table: from the
; address at $7E:DA1D + X to entry offset $0F when $0D2E is set, otherwise
; from the address at $7E:D9C5 + X to offset 3.
; HOW: The preset is read through the data bank; $19E1 counts the bytes
; (cleared with a long store, stepped through the data bank).
; Entry: X 8-bit; X = preset index (bytes). Exit: A = 9, with the high byte
; of the preset's address; X, Y past the copy; M 8-bit, X 16-bit; Z, C set,
; N clear.
%ramcode($7ED718, $01343F)
WindowHdmaPreset:
    sep.b #$30
    lda.b #$00
    sta.l !window_preset_count_w
    lda.l !wram_0D2E
    beq .lD736
    rep.b #$20
    sep.b #$10
    lda.l !window_presets_b,x
    rep.b #$10
    tay
    ldx.w #$000F
    bra .lD744
.lD736:
    rep.b #$20
    sep.b #$10
    lda.l !window_presets_a,x
    rep.b #$10
    tay
    ldx.w #$0003
.lD744:
    sep.b #$20
.lD746:
    lda.w 0,y
    sta.l !window_hdma,x
    iny
    inx
    lda.l !window_preset_count_w
    clc
    adc.b #$01
    sta.w !window_preset_count_w
    cmp.b #$09
    bne .lD746
    rts

; @routine 09:F065 WindowEdgesFromPair
; WHAT IT DOES: Writes a pair of window edges and their mirror images about
; $80 into the window HDMA table at offset $28 + the byte at $19B4: the
; first byte at X, its mirror, the second byte at X + 1, its mirror ($80
; when the byte is $81).
; HOW: Byte reads through the data bank; long stores at $7F:F883-$7F:F886
; + the offset.
; Entry: M 8-bit, X 16-bit; X = the pair's address. Exit: A low byte = the
; last value; X unchanged; N, Z from X; C, V from the last mirror (C set
; when the second byte was $81).
%at($09F065)
WindowEdgesFromPair:
    sep.b #$20
    rep.b #$10
    lda.w !bank0_base,x
    phx
    ldx.w !wram_19B4
    sta.l !window_edges,x
    plx
    lda.b #$80
    sec
    sbc.w !bank0_base,x
    clc
    adc.b #$80
    phx
    ldx.w !wram_19B4
    sta.l !window_edges+1,x
    plx
    lda.w !bank0_base+1,x
    phx
    ldx.w !wram_19B4
    sta.l !window_edges+2,x
    plx
    lda.w !bank0_base+1,x
    cmp.b #$81
    bne .lF09F
    lda.b #$80
    jmp.w .lF0A8
.lF09F:
    lda.b #$80
    sec
    sbc.w !bank0_base+1,x
    clc
    adc.b #$80
.lF0A8:
    phx
    ldx.w !wram_19B4
    sta.l !window_edges+3,x
    plx
    rts

; @routine 09:F0B2 WindowEdgesFromList
; WHAT IT DOES: Writes a symmetrical run of window edges for the window
; selected by $19B6: the pairs of the list it points to (table $09:F98B,
; read through the data bank, ended by $FF) in order and then in reverse,
; each with WindowEdgesFromPair at a table offset that starts at the
; table $09:F155 value and steps by 4; then moves $19B6 on by 2.
; HOW: The offset is kept at $19B4; Y counts the pairs.
; Entry: any widths. Exit: A = the new $19B6; Y = 0; X at the first pair
; minus 2; M 16-bit, X 16-bit; N, Z, C, V from the add.
%at($09F0B2)
WindowEdgesFromList:
    rep.b #$30
    lda.w !window_hdma_select
    tax
    lda.l !window_edge_offsets,x
    sta.w !wram_19B4
    ldx.w !window_hdma_select
    lda.l !window_edge_lists,x
    tax
    sep.b #$20
    ldy.w #$0000
.lF0CC:
    jsr.w WindowEdgesFromPair
    inx
    inx
    iny
    lda.w 0,x
    cmp.b #$FF
    beq .lF0EA
    rep.b #$20
    lda.w !wram_19B4
    clc
    adc.w #$0004
    sta.w !wram_19B4
    sep.b #$20
    jmp.w .lF0CC
.lF0EA:
    dex
    dex
.lF0EC:
    jsr.w WindowEdgesFromPair
    dex
    dex
    dey
    cpy.w #$0000
    beq .lF108
    rep.b #$20
    lda.w !wram_19B4
    clc
    adc.w #$0004
    sta.w !wram_19B4
    sep.b #$20
    jmp.w .lF0EC
.lF108:
    rep.b #$20
    lda.w !window_hdma_select
    clc
    adc.w #$0002
    sta.w !window_hdma_select
    rts

; @routine 7E:DB47 WindowHdmaLayout
; WHAT IT DOES: Writes the window HDMA table's line counts and data for
; layout $19B6: one screen (two entries) when $0D2E is clear; the split
; screen (three entries) when it is set, which no script has reached yet.
; HOW: Long stores into the table at $7F:F85B. One screen: entry 0 lines
; from $7E:E0C2 + X, entry 1 lines from $7E:E0C3 + X, data pointers $F857
; and the word at $7E:E0D2 + X, then $10 lines and the end. Split: entries
; 0 and 1 lines from $7E:E0CA + X, entry 2 from $7E:E0CB + X, pointers
; $F857, $F857, the word at $7E:E0E0 + X, $F857, then $10 lines and the
; end.
; Entry: any widths. Exit: A low byte 0, high byte from the layout's data
; word; X = $19B6; M 8-bit, X 16-bit; N clear, Z set.
%ramcode($7EDB47, $01386E)
WindowHdmaLayout:
    sep.b #$20
    lda.l !wram_0D2E
    bne .split
    rep.b #$30
    ldx.w !window_hdma_select
    lda.w #$F857
    sta.l !window_hdma+1
    sta.l !window_hdma+7
    lda.l !window_hdma_addrs,x
    sta.l !window_hdma+4
    sep.b #$20
    lda.l !window_hdma_counts+14,x
    sta.l !window_hdma
    lda.l !window_hdma_counts+15,x
    sta.l !window_hdma+3
    lda.b #$10
    sta.l !window_hdma+6
    lda.b #$00
    sta.l !window_hdma+9
    rts
.split:                             ; not reached in the traces so far
    rep.b #$30
    ldx.w !window_hdma_select
    lda.w #$F857
    sta.l !window_hdma+1
    sta.l !window_hdma+4
    sta.l !window_hdma+10
    lda.l !window_hdma_addrs+14,x
    sta.l !window_hdma+7
    sep.b #$20
    lda.l !window_hdma_counts+22,x
    sta.l !window_hdma
    sta.l !window_hdma+3
    lda.l !window_hdma_counts+23,x
    sta.l !window_hdma+6
    lda.b #$10
    sta.l !window_hdma+9
    lda.b #$00
    sta.l !window_hdma+12
    rts
