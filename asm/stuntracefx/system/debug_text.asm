; debug_text.c: text printed into Super FX RAM, two characters to a byte
; column. Direct page $54 points at a zero-terminated string; $46 is the
; place to draw the next character, $48 the base it is relative to; bit 2
; of $0307 says whether the character goes in the high or the low nibbles.
; The font is 6 bytes a character at $00:9022 (two 4-pixel rows packed in
; each byte's nibbles). No script reaches this code.
; C: src/stuntracefx/system/debug_text.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 03:D651 DebugDrawChar
; WHAT IT DOES: Draws character A of the debug font at $46 + $48 in Super
; FX RAM, then moves $46 on: every second character to the next cell
; (+$400, wrapping from $6800 back by $67C0).
; HOW: Six font bytes, each split into two nibbles placed in the high or
; the low half by bit 2 of $0307 ($1F2B the bits, $1F2C the keep mask,
; $0308 the second nibble); writing the low half first fills the cell's
; rows with $FF/$00. Direct page $40 counts the rows.
; Entry: A low byte = character; D = $0000. Exit: P unchanged; A = the new
; $46; X, Y past the cell.
%at($03D651)
DebugDrawChar:
    php
    rep.b #$30
    pha
    lda.b !text_pos
    clc
    adc.b !text_base
    tax
    pla
    and.w #$00FF
    asl a
    sta.b !dp_40
    asl a
    clc
    adc.b !dp_40
    tay
    lda.w #$0006
    sta.b !dp_40
    phx
.lD66D:
    sep.b #$20
    lda.w !text_half
    and.b #$04
    beq .lD695
    lda.w !debug_font,y
    sta.w !wram_1F2B
    lsr a
    lsr a
    lsr a
    lsr a
    sta.w !text_high
    lda.w !wram_1F2B
    and.b #$0F
    sta.w !wram_1F2B
    ora.w !text_high
    eor.b #$FF
    sta.w !text_mask
    bra .lD6B2
.lD695:
    lda.w !debug_font,y
    sta.w !wram_1F2B
    and.b #$F0
    sta.w !text_high
    lda.w !wram_1F2B
    asl a
    asl a
    asl a
    asl a
    sta.w !wram_1F2B
    ora.w !text_high
    eor.b #$FF
    sta.w !text_mask
.lD6B2:
    lda.w !text_half
    and.b #$04
    bne .lD6DD
    lda.b #$FF
    sta.l !gsu_ram,x
    sta.l !gsu_ram+1,x
    sta.l !gsu_ram+16,x
    sta.l !gsu_ram+17,x
    sta.l !gsu_ram+32,x
    lda.b #$00
    sta.l !gsu_ram+33,x
    sta.l !gsu_ram+48,x
    sta.l !gsu_ram+49,x
.lD6DD:
    lda.l !gsu_ram,x
    and.w !text_mask
    ora.w !wram_1F2B
    sta.l !gsu_ram,x
    lda.l !gsu_ram+1,x
    and.w !text_mask
    ora.w !wram_1F2B
    ora.w !text_high
    sta.l !gsu_ram+1,x
    lda.l !gsu_ram+16,x
    and.w !text_mask
    ora.w !wram_1F2B
    ora.w !text_high
    sta.l !gsu_ram+16,x
    lda.l !gsu_ram+17,x
    and.w !text_mask
    ora.w !wram_1F2B
    ora.w !text_high
    sta.l !gsu_ram+17,x
    lda.l !gsu_ram+32,x
    and.w !text_mask
    ora.w !wram_1F2B
    ora.w !text_high
    sta.l !gsu_ram+32,x
    lda.l !gsu_ram+33,x
    and.w !text_mask
    sta.l !gsu_ram+33,x
    lda.l !gsu_ram+48,x
    and.w !text_mask
    sta.l !gsu_ram+48,x
    lda.l !gsu_ram+49,x
    and.w !text_mask
    sta.l !gsu_ram+49,x
    rep.b #$20
    inx
    inx
    iny
    dec.b !dp_40
    beq .lD75C
    brl .lD66D
.lD75C:
    pla
    sec
    sbc.b !text_base
    pha
    lda.w !text_half
    eor.w #$0004
    sta.w !text_half
    bne .lD77D
    pla
    clc
    adc.w #$0400
    tax
    cmp.w #$6800
    bcc .lD77E
    sec
    sbc.w #$67C0
    bra .lD77E
.lD77D:
    pla
.lD77E:
    sta.b !text_pos
    plp
    rts

; @routine 03:D465 DebugPrint
; WHAT IT DOES: Prints the zero-terminated string at the long pointer in
; direct page $54 with DebugDrawChar, starting in the low nibbles; lower
; case letters print as upper case.
; HOW: Each character from $61 up loses $20 more, then all lose $20 (the
; font starts at the space). The pointer is read again for each character
; (LDA [$54],Y). Data bank 0 inside; every register and P are saved and
; restored.
; Entry: any widths; D = $0000. Exit: every register and P unchanged but
; N, Z, which the final PLB sets from the data bank.
%at($03D465)
DebugPrint:
    phb
    php
    rep.b #$30
    pha
    phx
    phy
    stz.w !text_half
    sep.b #$20
    rep.b #$10
    lda.b #$00
    pha
    plb
    ldy.w #$0000
.lD47A:
    lda.b [!dp_54],y
    beq .lD491
    phy
    cmp.b #$61
    bcc .lD486
    sec
    sbc.b #$20
.lD486:
    sec
    sbc.b #$20
    jsr.w DebugDrawChar
    ply
    iny
    jmp.w .lD47A
.lD491:
    rep.b #$30
    ply
    plx
    pla
    plp
    plb
    rtl

; @routine 08:FEB4 DebugShowErrors
; WHAT IT DOES: Prints an error message into Super FX RAM for each of three
; flags that is set: $70:26F0 (an old Super FX chip), $70:1090 (a failed
; checksum of the car dynamics), $70:2650 (a car made from bad data).
; HOW: For each: $46 = the place, $0307 = 0, the long pointer at direct
; page $54 = the message in bank 08, then DebugPrint. No script sets a
; flag.
; Entry: M 8-bit, D = $0000. Exit: M 16-bit; X, Y unchanged; A = the
; last message's address when it printed, else the last flag; N, Z from
; that.
%at($08FEB4)
DebugShowErrors:
    rep.b #$20
    lda.l !gsu_flag_26f0
    beq .lFED8
    lda.w #$2240
    sta.b !text_pos
    lda.w #$0000
    sta.w !text_half
    sep.b #$20
    lda.b #$08
    sta.b !dp_54+2
    rep.b #$20
    lda.w #$FF34
    sta.b !dp_54
    jsl.l DebugPrint
.lFED8:
    lda.l !gsu_flag_1090
    beq .lFEFA
    lda.w #$2280
    sta.b !text_pos
    lda.w #$0000
    sta.w !text_half
    sep.b #$20
    lda.b #$08
    sta.b !dp_54+2
    rep.b #$20
    lda.w #$FF4B
    sta.b !dp_54
    jsl.l DebugPrint
.lFEFA:
    lda.l !gsu_flag_2650
    beq .lFF1C
    lda.w #$1AC0
    sta.b !text_pos
    lda.w #$0000
    sta.w !text_half
    sep.b #$20
    lda.b #$08
    sta.b !dp_54+2
    rep.b #$20
    lda.w #$FF66
    sta.b !dp_54
    jsl.l DebugPrint
.lFF1C:
    rtl
