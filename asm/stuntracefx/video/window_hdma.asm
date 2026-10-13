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

; @routine 7E:D28F WindowLayoutStepForScreen
; WHAT IT DOES: Clears bit 5 of $0D45 and steps the window layout
; (WindowLayoutStep) with Y = 0, or Y = 2 on the menu's course screen
; ($0D62 set).
; HOW: Data bank $7E inside, P and the data bank saved and restored.
; Entry: any widths. Exit: A, X, Y as WindowLayoutStep leaves them; P and
; the data bank unchanged.
%ramcode($7ED28F, $012FB6)
WindowLayoutStepForScreen:
    php
    phb
    phk
    plb
    sep.b #$20
    lda.l !wram_0D45
    and.b #$DF
    sta.l !wram_0D45
    lda.w !wram_0D62
    bne .course_screen
    rep.b #$30
    ldy.w #$0000
    jsr.w WindowLayoutStep
    jmp.w .done
.course_screen:
    rep.b #$30
    ldy.w #$0002
    jsr.w WindowLayoutStep
.done:
    plb
    plp
    rtl

; @routine 7E:DDD9 WindowSlideStep
; WHAT IT DOES: Steps the window slide the way bits 1-2 of $0DA5 ask: 2 slides
; it down (WindowSlideDown), 4 or 6 up (WindowSlideUp), 0 not at all.
; HOW: The bits are tested through the data bank.
; Entry: X 16-bit or 8-bit (unchanged). Exit: M 8-bit; as the slide it runs,
; or A low byte = 0 with Z set.
%ramcode($7EDDD9, $013B00)
WindowSlideStep:
    sep.b #$20
    lda.w !slide_flags
    and.b #$06
    beq .done
    cmp.b #$02
    bne .up
    jsr WindowSlideDown
    jmp .done
.up:
    jsr WindowSlideUp
.done:
    rts

; @routine 7E:DDF0 WindowSlideDown
; WHAT IT DOES: Moves the window slide position $1A14 down a line; at $96
; the slide ends.
; HOW: window_slide with +1, stopping at $96. Long accesses except $0DA5
; (data bank).
; Entry: X 16-bit or 8-bit (unchanged). Exit: M 8-bit; A low byte = $0DA5
; when the slide ends, else the new position (A high byte $FF); N, Z from
; it; C from comparing the new position with $96; V from the add.
%ramcode($7EDDF0, $013B17)
WindowSlideDown:
    sep.b #$20
    lda.l !window_slide_pos
    clc
    adc.b #$01
    cmp.b #$96
    beq .stop
    sta.l !window_slide_pos
    jmp.w .move
.stop:
    lda.w !slide_flags
    and.b #$F9
    sta.w !slide_flags
    rts
.move:
    jsr.w WindowHdmaFFD1
    sep.b #$20
    lda.b #$40
    sta.l !window_setup_bytes+1
    lda.b #$20
    sta.l !window_setup_bytes
    lda.l !window_slide_pos
    sta.l !window_setup_bytes+2
    rts

; @routine 7E:DE27 WindowSlideUp
; WHAT IT DOES: Moves the window slide position $1A14 up a line; at $7F
; the slide ends.
; HOW: As WindowSlideDown, with -1, stopping at $7F.
; Entry: as WindowSlideDown. Exit: as WindowSlideDown, comparing with $7F.
%ramcode($7EDE27, $013B4E)
WindowSlideUp:
    sep.b #$20
    lda.l !window_slide_pos
    sec
    sbc.b #$01
    cmp.b #$7F
    beq .stop
    sta.l !window_slide_pos
    jmp.w .move
.stop:
    lda.w !slide_flags
    and.b #$F9
    sta.w !slide_flags
    rts
.move:
    jsr.w WindowHdmaFFD1
    sep.b #$20
    lda.b #$40
    sta.l !window_setup_bytes+1
    lda.b #$20
    sta.l !window_setup_bytes
    lda.l !window_slide_pos
    sta.l !window_setup_bytes+2
    rts

; @routine 7E:DA75 WindowLayoutStep
; WHAT IT DOES: Acts on a request in $19BD to move the window layout: 1
; starts it moving up by 2 a step ($19B8 = 2, $19BA = 1), $FF down ($19B8 =
; -2, $19BA = 0); $19BC = $FF marks it moving. While it moves, each call
; clears the request and steps the layout $19B6 by $19B8, stopping (and
; clearing $19BC) when that would give -2, and not storing it when it
; reaches the limit for Y in the table at $7E:DBC5; then resets the window
; setup ($19A9 = 0, $19AC-$19B1 = $E7) and rebuilds the window table with
; WindowHdmaFromTable (Y = 0) or WindowHdmaLayout (Y = 2).
; HOW: Long accesses to $19B6-$19BD except one data-bank read of $19BC and
; the store to $19A9; the builder is called through the table at
; $7E:DBC9.
; Entry: Y = 0 or 2. Exit: when nothing moves, A low byte = $19BC, M 8-bit,
; X 8-bit, N, Z, C from comparing it with $FF; at the -2 stop A low byte =
; 0, Z, C set, N clear, M and X 8-bit; otherwise as the builder, with C
; from the limit compare and V from the step add.
%ramcode($7EDA75, $01379C)
WindowLayoutStep:
    sep.b #$30
    lda.l !layout_request
    cmp.b #$01
    beq .lDA8B
    cmp.b #$FF
    beq .lDAA4
    lda.w !window_hdma_19bc
    cmp.b #$FF
    beq .lDABC
    rts
.lDA8B:
    lda.b #$FF
    sta.l !window_hdma_19bc
    rep.b #$20
    lda.w #$0002
    sta.l !layout_step
    lda.w #$0001
    sta.l !layout_dir
    jmp.w .lDABC
.lDAA4:
    sep.b #$20
    lda.b #$FF
    sta.l !window_hdma_19bc
    rep.b #$20
    lda.w #$FFFE
    sta.l !layout_step
    lda.w #$0000
    sta.l !layout_dir
.lDABC:
    sep.b #$20
    lda.b #$00
    sta.l !layout_request
    rep.b #$20
    lda.l !window_hdma_select
    clc
    adc.l !layout_step
    cmp.w #$FFFE
    bne .lDADD
    sep.b #$20
    lda.b #$00
    sta.l !window_hdma_19bc
    rts
.lDADD:
    rep.b #$30
    tyx
    cmp.l !layout_limits,x
    beq .lDAEA
    sta.l !window_hdma_select
.lDAEA:
    sep.b #$20
    lda.b #$00
    sta.w !windows_setup_index
    lda.b #$E7
    sta.l !window_setup_bytes
    sta.l !window_setup_bytes+1
    sta.l !window_setup_bytes+2
    sta.l !window_setup_bytes+3
    sta.l !window_setup_bytes+4
    sta.l !window_setup_bytes+5
    tyx
    jsr.w (!layout_builders,x)
    rts

; @routine 09:ECE0 WindowsInit
; WHAT IT DOES: Sets up the race's windows: the four-byte HDMA band entries
; at $7F:F857 and $7F:FFD1-$FFE0 and the 16-byte window table header at
; $7F:F85B from the lists in bank $09; the course records into bank $7F
; (Records7FFromF170, or Records7FFromF634 on the menu's course screen);
; the window edges for every list (WindowEdgesFromList until $19B6 reaches
; $0E); the word list at $09:FB74 (to its $FFFF) to $7F:FF8B; on the
; course screen, every edge byte at $7F:F883 moved by -$28 and a copy +$50
; at $7F:FB03; then the window, slide and layout state ($19A9-$1A29) to
; its start values, the slide value $19F6 and $19E6 from the tables at
; $09:F127 and $09:F115 for HUD modes 0 and 3 (and 0), the layout record
; for $0D89 from $09:F14D, and HDMA channels 4 (to $2126, indirect, from
; $7F:F85B) and 5 (to $2132 from $7F:FF8B).
; HOW: Data bank $09 (the program bank) and P saved and restored; the
; state stores are in the original's order, byte and word as it makes
; them. The ROM bytes $09:xxxx indexed by the tables are read through the
; data bank.
; Entry: any widths, D = $0000. Exit: A = $007F; X = 0; Y as the last
; callee left it; P and the data bank unchanged.
%at($09ECE0)
WindowsInit:
    phb
    phk
    plb
    php
    sep.b #$30
    ldx.b #$00
.copy_first:
    lda.l !window_edge_lists+14,x
    sta.l !window_band_first,x
    inx
    cpx.b #$04
    bne .copy_first
    ldx.b #$00
.copy_band0:
    lda.l !window_edge_lists+18,x
    sta.l !window_bands,x
    inx
    cpx.b #$04
    bne .copy_band0
    ldx.b #$00
.copy_band1:
    lda.l !window_edge_lists+22,x
    sta.l !window_bands+4,x
    inx
    cpx.b #$04
    bne .copy_band1
    ldx.b #$00
.copy_band2:
    lda.l !window_edge_lists+26,x
    sta.l !window_bands+8,x
    inx
    cpx.b #$04
    bne .copy_band2
    ldx.b #$00
.copy_band3:
    lda.l !window_edge_lists+30,x
    sta.l !window_bands+12,x
    inx
    cpx.b #$04
    bne .copy_band3
    sep.b #$30
    lda.w !wram_0D62
    bne .course_screen
    jsr.w Records7FFromF170
    jmp.w .records_done
.course_screen:
    jsr.w Records7FFromF634
.records_done:
    sep.b #$30
    ldx.b #$00
.copy_header:
    lda.l !window_edge_offsets+14,x
    sta.l !window_hdma,x
    inx
    cpx.b #$10
    bne .copy_header
    rep.b #$20
    stz.w !window_hdma_select
.next_list:
    jsr.w WindowEdgesFromList
    rep.b #$20
    lda.w !window_hdma_select
    cmp.w #$000E
    bne .next_list
    rep.b #$30
    ldx.w #$0000
.copy_colours:
    lda.l !window_colour_list,x
    cmp.w #$FFFF
    beq .colours_done
    sta.l !window_colour_hdma,x
    inx
    inx
    jmp.w .copy_colours
.colours_done:
    sep.b #$20
    rep.b #$10
    lda.w !wram_0D62
    beq .state
    ldx.w #$0000
.shift_edge:
    lda.l !window_edges,x
    clc
    adc.b #$D8
    sta.l !window_edges,x
    clc
    adc.b #$50
    sta.l !window_edges_copy,x
    inx
    cpx.w #$0280
    bne .shift_edge
.state:
    rep.b #$20
    lda.w #$FFFE
    sta.w !window_hdma_select
    lda.w #$FFFE
    sta.w !layout_step
    stz.w !layout_dir
    sep.b #$20
    stz.w !window_hdma_19bc
    lda.b #$00
    sta.w !wram_19BE
    sta.w !windows_setup_index
    sta.w !wram_1A07
    lda.b #$00
    sta.w !wram_1A0A
    sta.w !wram_1A0B
    sta.w !wram_1A17
    sta.w !wram_1A18
    sta.w !wram_19BF
    lda.b #$F9
    sta.w !wram_19C0
    lda.b #$01
    sta.w !wram_1A0C
    sta.w !wram_1A0D
    lda.b #$40
    sta.w !wram_1A08
    sta.w !wram_1A09
    lda.b #$FE
    sta.w !wram_1A19
    sta.w !wram_1A1A
    lda.b #$F0
    sta.w !wram_1A15
    sta.w !wram_1A16
    sep.b #$20
    lda.b #$00
    sta.w !window_setup_bytes
    sta.w !wram_19AD
    sta.w !wram_19AE
    sta.w !wram_19AF
    sta.w !wram_19B0
    sta.w !wram_19B1
    sta.w !wram_1A0E
    sta.w !wram_1A0F
    sta.w !wram_1A1B
    sep.b #$20
    lda.b #$1A
    sta.w !wram_1A1C
    sta.w !wram_1A1D
    stz.w !wram_1A1E
    stz.w !wram_1A1F
    stz.w !wram_1A22
    stz.w !wram_1A23
    lda.b #$FE
    sta.w !wram_1A20
    sta.w !wram_1A21
    rep.b #$30
    lda.w !hud_mode
    beq .slide_value
    cmp.w #$0003
    bne .mode0_only
.slide_value:
    lda.w !wram_0D4A
    and.w #$00FF
    asl a
    tax
    lda.w !wram_0D48
    and.w #$00FF
    asl a
    clc
    adc.l !slide_value_table,x
    tax
    lda.w !bank0_base,x
    sta.w !slide_value
.mode0_only:
    rep.b #$30
    lda.w !hud_mode
    bne .layout
    lda.w !wram_0D4A
    and.w #$00FF
    asl a
    tax
    lda.w !wram_0D48
    and.w #$00FF
    clc
    adc.l !window_19e6_table,x
    tax
    sep.b #$20
    lda.w !bank0_base,x
    sta.w !wram_19E6
.layout:
    rep.b #$20
    lda.w #$0000
    sta.w !wram_19FE
    sep.b #$10
    lda.w !wram_0D89
    asl a
    tax
    stx.w !wram_19D1
    lda.l !layout_records,x
    sta.w !wram_19CB
    stz.w !wram_19D3
    rep.b #$20
    lda.w #$0017
    sta.w !wram_19CF
    lda.w #$0020
    sta.w !wram_19C5
    lda.w #$E020
    sta.w !wram_19C3
    lda.w #$E600
    sta.w !wram_19F3
    sep.b #$20
    stz.w !wram_19F5
    sep.b #$20
    lda.b #$80
    sta.w !window_slide_pos
    lda.b #$00
    sta.w !window_hdma_off
    lda.b #$FE
    sta.w !wram_19E0
    lda.b #$00
    sta.w !wram_19DF
    rep.b #$20
    lda.w #$0000
    sta.w !wram_19DB
    sta.w !wram_19DD
    sep.b #$20
    lda.b #$FF
    sta.w !wram_1A04
    sep.b #$20
    lda.b #$00
    sta.w !hud_digits_19ec
    sta.w !wram_19ED
    sta.w !wram_19E8
    sta.w !wram_19E9
    sta.w !wram_19EA
    sta.w !wram_19EB
    sta.w !wram_19F2
    sta.w !wram_19EE
    sta.w !wram_19EE+1
    sta.w !wram_19F0
    sta.w !wram_19F0+1
    sta.w !wram_1A06
    sta.w !wram_1A26+3
    sep.b #$20
    rep.b #$10
    lda.b #$44
    sta.w !DMAP4
    lda.b #$26
    sta.w !BBAD4
    ldx.w #$F85B
    stx.w !A1T4L
    lda.b #$7F
    sta.w !A1B4
    ldx.w #$0000
    stx.w !DAS4L
    lda.b #$7F
    sta.w !DASB4
    lda.b #$00
    sta.w !DMAP5
    lda.b #$32
    sta.w !BBAD5
    ldx.w #$FF8B
    stx.w !A1T5L
    lda.b #$7F
    sta.w !A1B5
    ldx.w #$0000
    stx.w !DAS5L
    lda.b #$7F
    sta.w !DASB5
    plp
    plb
    rtl

; @routine 7E:D5EC WindowBandsAnimate
; WHAT IT DOES: Runs the window band animation of state $19D8 (unless
; $19DF is set): 1 starts an opening (step +$80 from 0 to $0780, or $0C80
; when $0D62 is clear), 2 a closing (step -$80 from $0700 or $0C00 to 0),
; both going on to state 4 with $0DA4 cleared; in states 4 and 5 it clears
; bit 5 of $0D45, selects window setup 8 ($19A9) with $19E0 in
; $19AC-$19B1, moves $19D4 by the step until it reaches the target (then
; state 0 when its high byte is 0, else 5) and rebuilds the window table
; for entry $19D5 (WindowHdmaPreset with $0D62 set, else
; WindowHdmaFromList). State 3 and others do nothing.
; HOW: The data bank is the code's own ($7E) throughout; some accesses
; are long to bank 0. States 3 and above 5 are not reached in the traces
; so far.
; Entry: any widths. Exit: P and the data bank restored; X and Y high
; bytes cleared; A, X as the last step left them.
%ramcode($7ED5EC, $013313)
WindowBandsAnimate:
    php
    phb
    phk
    plb
    sep.b #$30
    lda.l !wram_19DF
    bne .out
    lda.w !window_hdma_off
    bne .state
.out:
    plb
    plp
    rtl
.state:
    sep.b #$30
    lda.w !window_hdma_off
    cmp.b #$04
    beq .run
    cmp.b #$05
    beq .run
    cmp.b #$01
    beq .open
    cmp.b #$02
    beq .close
    cmp.b #$03
    beq .state3
    plb
    plp
    rtl
.state3:
    plb
    plp
    rtl
.open:
    rep.b #$20
    lda.w #$0080
    sta.w !window_band_step
    lda.w #$0000
    sta.l !window_band_pos
    sep.b #$20
    lda.w !wram_0D62
    beq .open_slow
    rep.b #$20
    lda.w #$0780
    bra .open_target
.open_slow:
    rep.b #$20
    lda.w #$0C80
.open_target:
    sta.l !window_band_target
    jmp.w .started
.close:
    rep.b #$20
    lda.w #$FF80
    sta.w !window_band_step
    sep.b #$20
    lda.w !wram_0D62
    beq .close_slow
    rep.b #$20
    lda.w #$0700
    bra .close_from
.close_slow:
    rep.b #$20
    lda.w #$0C00
.close_from:
    sta.l !window_band_pos
    lda.w #$0000
    sta.l !window_band_target
.started:
    rep.b #$20
    lda.w #$0000
    sta.l !wram_0DA4
    sep.b #$30
    lda.b #$04
    sta.w !window_hdma_off
.run:
    sep.b #$20
    lda.l !wram_0D45
    and.b #$DF
    sta.l !wram_0D45
    lda.b #$08
    sta.w !windows_setup_index
    lda.w !wram_19E0
    sta.w !window_setup_bytes
    sta.w !window_setup_bytes+1
    sta.w !window_setup_bytes+2
    sta.w !window_setup_bytes+3
    sta.w !window_setup_bytes+4
    sta.w !window_setup_bytes+5
    rep.b #$20
    lda.l !window_band_pos
    clc
    adc.w !window_band_step
    cmp.w !window_band_target
    beq .arrived
    sta.l !window_band_pos
    jmp.w .table
.arrived:
    sep.b #$20
    lda.l !window_band_pos+1
    bne .last
    stz.w !window_hdma_off
    plb
    plp
    rtl
.last:
    lda.b #$05
    sta.w !window_hdma_off
.table:
    sep.b #$30
    lda.l !window_band_pos+1
    asl a
    tax
    lda.w !wram_0D62
    beq .list
    jsr.w WindowHdmaPreset
    jmp.w .done
.list:
    jsr.w WindowHdmaFromList
.done:
    sep.b #$30
    plb
    plp
    rtl

; @routine 7E:DCC6 WindowHdmaFFD5
; WHAT IT DOES: Sets the window table's second to fourth bands to 16, 48
; and 16 lines at $FFD5.
; HOW: Counts, then addresses. Reached by a unit test
; (unit_uncalled).
; Entry: any widths. Exit: as WindowHdmaFFD1, A = $FFD5.
%ramcode($7EDCC6, $0139ED)
WindowHdmaFFD5:
    sep.b #$20
    lda.b #$10
    sta.l !window_hdma+3
    lda.b #$30
    sta.l !window_hdma+6
    lda.b #$10
    sta.l !window_hdma+9
    rep.b #$20
    lda.w #$FFD5
    sta.l !window_hdma+4
    sta.l !window_hdma+7
    sta.l !window_hdma+10
    rts

; @routine 7E:DCEC WindowHdmaFFD9
; WHAT IT DOES: As WindowHdmaFFD5 for the sixth to eighth bands, at $FFD9.
; HOW: As WindowHdmaFFD5. Reached by a unit test
; (unit_uncalled).
; Entry: any widths. Exit: as WindowHdmaFFD1, A = $FFD9.
%ramcode($7EDCEC, $013A13)
WindowHdmaFFD9:
    sep.b #$20
    lda.b #$10
    sta.l !window_hdma+15
    lda.b #$30
    sta.l !window_hdma+18
    lda.b #$10
    sta.l !window_hdma+21
    rep.b #$20
    lda.w #$FFD9
    sta.l !window_hdma+16
    sta.l !window_hdma+19
    sta.l !window_hdma+22
    rts

; @routine 7E:D75E WindowSplitAnimate
; WHAT IT DOES: While $19DF is 1, runs the split window animation of state
; $19D8: 1 starts an opening ($19DB from 0 by $0400 to $7400, $19E2 =
; $F50B, $19E4 = 0), 2 a closing (from $8000 by -$0400 to $0800, $19E4 =
; $0BF5, $19E2 = 0), both going on to state 4 with $0DA4 cleared; in
; states 4 and 5 it clears bit 5 of $0D45, selects window setup 8 with
; $19E0 in $19AC-$19B1, and moves $19DB on, setting the bands around line
; ($FF - 2 * $19DC) / 2 (WindowHdmaAt); at the end, from $7000 up it sets
; the full bands (WindowHdmaFFDD) and state 5, below it state 0.
; HOW: The data bank is the code's ($7E); some accesses are long to bank
; 0. The closing, state 3 and the state 0 end are not reached in the
; traces so far.
; Entry: any widths. Exit: P and the data bank restored; X and Y high
; bytes cleared; A as the last step left it.
%ramcode($7ED75E, $013485)
WindowSplitAnimate:
    php
    phb
    phk
    plb
    sep.b #$30
    lda.l !wram_19DF
    cmp.b #$01
    bne .out
    lda.w !window_hdma_off
    bne .state
.out:
    plb
    plp
    rtl
.state:
    sep.b #$30
    lda.w !window_hdma_off
    cmp.b #$04
    beq .run
    cmp.b #$05
    beq .run
    cmp.b #$01
    beq .open
    cmp.b #$02
    beq .close
    cmp.b #$03
    beq .state3
    plb
    plp
    rtl
.state3:
    sep.b #$20
    plb
    plp
    rtl
.open:
    rep.b #$20
    lda.w #$0400
    sta.l !wram_19DD
    lda.w #$0000
    sta.l !wram_19DB
    lda.w #$7400
    sta.l !window_band_target
    lda.w #$F50B
    sta.w !wram_19E2
    lda.w #$0000
    sta.w !wram_19E4
    jmp.w .started
.close:
    rep.b #$20
    lda.w #$FC00
    sta.l !wram_19DD
    lda.w #$8000
    sta.l !wram_19DB
    lda.w #$0800
    sta.l !window_band_target
    lda.w #$0BF5
    sta.w !wram_19E4
    lda.w #$0000
    sta.w !wram_19E2
.started:
    rep.b #$20
    lda.w #$0000
    sta.l !wram_0DA4
    sep.b #$30
    lda.b #$04
    sta.l !window_hdma_off
.run:
    sep.b #$20
    lda.l !wram_0D45
    and.b #$DF
    sta.l !wram_0D45
    lda.b #$08
    sta.w !windows_setup_index
    lda.w !wram_19E0
    sta.w !window_setup_bytes
    sta.w !window_setup_bytes+1
    sta.w !window_setup_bytes+2
    sta.w !window_setup_bytes+3
    sta.w !window_setup_bytes+4
    sta.w !window_setup_bytes+5
    rep.b #$20
    lda.l !wram_19DB
    clc
    adc.l !wram_19DD
    cmp.w !window_band_target
    beq .arrived
    sta.l !wram_19DB
    sep.b #$20
    lda.b #$FF
    sec
    sbc.l !wram_19DB+1
    sec
    sbc.l !wram_19DB+1
    lsr a
    jsr.w WindowHdmaAt
    bra .done
.arrived:
    rep.b #$20
    lda.l !wram_19DB
    cmp.w #$7000
    bcc .closed
    jsr.w WindowHdmaFFDD
    sep.b #$20
    lda.b #$05
    sta.w !window_hdma_off
    bra .done
.closed:
    sep.b #$20
    stz.w !window_hdma_off
.done:
    plb
    plp
    rtl
