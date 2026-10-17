; debug_panels.asm: the developers' debug panels of bank 08, numbers and
; names from work RAM and the Super FX records printed into the picture.
; C: src/stuntracefx/system/debug_panels.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 08:F10D DebugTimersPanel
; WHAT IT DOES: Prints the list of timers at $08:FE5E down the left of
; the picture, one a row: each entry's name (GAME_TM, DISP_TM, DYN__TM,
; STRA_TM, TMP1_TM to TMP3_TM) and the word at its long address in
; decimal (the frame counters at $1A3B-$1A53).
; HOW: An entry is nine bytes of zero-terminated name and a long pointer;
; a zero first byte ends the list. The name is printed with DebugPrint
; from $08:FE5E + the entry's offset; the word is read with the data bank
; set to the pointer's bank (PHA, PLB), then DebugPrintDecimalWord. The
; text position starts at 0 and steps $40 a row.
; Entry: any widths. Exit: M 8-bit, X 16-bit; X = the offset of the
; list's end; A low byte 0, its high byte the last text position's; Z
; set, N clear; C, V from the last step of the text position.
%at($08F10D)
DebugTimersPanel:
    sep.b #$20
    rep.b #$10
    lda.b #$00
    sta.b !text_pos
    lda.b #$00
    sta.b !text_pos+1
    lda.b #$00
    sta.w !text_half
    sep.b #$20
    lda.b #$08
    sta.b !dp_54+2
    ldx.w #$0000
.lF127:
    sep.b #$20
    lda.l !debug_timer_list,x
    beq .lF170
    rep.b #$20
    lda.b !text_pos
    pha
    stx.b !dp_54
    lda.w #$FE5E
    clc
    adc.b !dp_54
    sta.b !dp_54
    jsl.l DebugPrint
    txa
    clc
    adc.w #$0009
    tax
    phx
    phb
    sep.b #$20
    lda.l !debug_timer_list,x
    pha
    plb
    rep.b #$20
    lda.l !debug_timer_list+1,x
    tax
    lda.w !bank0_base,x
    plb
    jsl.l DebugPrintDecimalWord
    pla
    clc
    adc.w #$0003
    tax
    pla
    clc
    adc.w #$0040
    sta.b !text_pos
    bra .lF127
.lF170:
    rtl

; @routine 08:F171 DebugMatchSpeedPanel
; WHAT IT DOES: Prints, in decimal at text position $2840, the speed word
; (+$4A) of the Super FX record whose address is at $70:2476.
; HOW: The record address read into X, the field read and printed with
; DebugPrintDecimalWord.
; Entry: any widths. Exit: M and X 16-bit; X = the record; A = the speed,
; N and Z from it.
%at($08F171)
DebugMatchSpeedPanel:
    rep.b #$30
    lda.l !gsu_match_2476
    tax
    lda.l !gsu_ram+74,x
    pha
    lda.w #$2840
    sta.b !text_pos
    lda.w #$0000
    sta.w !text_half
    pla
    jsl.l DebugPrintDecimalWord
    rtl

; @routine 08:F18E DebugLastCarPanel
; WHAT IT DOES: Prints, in decimal at text position $0440, the word +$6A
; of the last car's Super FX record (address at $70:2474).
; HOW: As DebugMatchSpeedPanel.
; Entry: M and X 16-bit. Exit: X = the record; A = the word, N and Z from
; it.
%at($08F18E)
DebugLastCarPanel:
    lda.l !fx_car_last
    tax
    lda.l !gsu_ram+106,x
    pha
    lda.w #$0440
    sta.b !text_pos
    lda.w #$0000
    sta.w !text_half
    pla
    jsl.l DebugPrintDecimalWord
    rtl

; @routine 08:F1A9 DebugPanelNone
; WHAT IT DOES: Nothing: an empty panel.
; HOW: RTL.
; Entry: any. Exit: unchanged.
%at($08F1A9)
DebugPanelNone:
    rtl

; @routine 08:F1AA DebugCarsPanel
; WHAT IT DOES: Prints the two Super FX wait counters ($1F1F, $1F21) at
; $5040 and $5080, then for the first four cars of the car list (from
; $70:262E, each next one at +$74 of the one before) four fields of the
; record (+$88, +$7A, +$86, +$84) in decimal, a car a row from $0440.
; HOW: Each value read, pushed while the text position is set, pulled and
; printed with DebugPrintDecimalWord.
; Entry: M and X 16-bit. Exit: X = the fourth car; A = its +$84, N and Z
; from it.
%at($08F1AA)
DebugCarsPanel:
    lda.w !gsu_wait_polls
    pha
    lda.w #$5040
    sta.b !text_pos
    lda.w #$0000
    sta.w !text_half
    pla
    jsl.l DebugPrintDecimalWord
    lda.w !gsu_ab13_polls
    pha
    lda.w #$5080
    sta.b !text_pos
    lda.w #$0000
    sta.w !text_half
    pla
    jsl.l DebugPrintDecimalWord
    lda.l !fx_cars
    tax
    lda.l !gsu_ram+136,x
    pha
    lda.w #$0440
    sta.b !text_pos
    lda.w #$0000
    sta.w !text_half
    pla
    jsl.l DebugPrintDecimalWord
    lda.l !gsu_ram+122,x
    pha
    lda.w #$1440
    sta.b !text_pos
    lda.w #$0000
    sta.w !text_half
    pla
    jsl.l DebugPrintDecimalWord
    lda.l !gsu_ram+134,x
    pha
    lda.w #$2840
    sta.b !text_pos
    lda.w #$0000
    sta.w !text_half
    pla
    jsl.l DebugPrintDecimalWord
    lda.l !gsu_ram+132,x
    pha
    lda.w #$3C40
    sta.b !text_pos
    lda.w #$0000
    sta.w !text_half
    pla
    jsl.l DebugPrintDecimalWord
    lda.l !gsu_ram+116,x
    tax
    lda.l !gsu_ram+136,x
    pha
    lda.w #$0480
    sta.b !text_pos
    lda.w #$0000
    sta.w !text_half
    pla
    jsl.l DebugPrintDecimalWord
    lda.l !gsu_ram+122,x
    pha
    lda.w #$1480
    sta.b !text_pos
    lda.w #$0000
    sta.w !text_half
    pla
    jsl.l DebugPrintDecimalWord
    lda.l !gsu_ram+134,x
    pha
    lda.w #$2880
    sta.b !text_pos
    lda.w #$0000
    sta.w !text_half
    pla
    jsl.l DebugPrintDecimalWord
    lda.l !gsu_ram+132,x
    pha
    lda.w #$3C80
    sta.b !text_pos
    lda.w #$0000
    sta.w !text_half
    pla
    jsl.l DebugPrintDecimalWord
    lda.l !gsu_ram+116,x
    tax
    lda.l !gsu_ram+136,x
    pha
    lda.w #$04C0
    sta.b !text_pos
    lda.w #$0000
    sta.w !text_half
    pla
    jsl.l DebugPrintDecimalWord
    lda.l !gsu_ram+122,x
    pha
    lda.w #$14C0
    sta.b !text_pos
    lda.w #$0000
    sta.w !text_half
    pla
    jsl.l DebugPrintDecimalWord
    lda.l !gsu_ram+134,x
    pha
    lda.w #$28C0
    sta.b !text_pos
    lda.w #$0000
    sta.w !text_half
    pla
    jsl.l DebugPrintDecimalWord
    lda.l !gsu_ram+132,x
    pha
    lda.w #$3CC0
    sta.b !text_pos
    lda.w #$0000
    sta.w !text_half
    pla
    jsl.l DebugPrintDecimalWord
    lda.l !gsu_ram+116,x
    tax
    lda.l !gsu_ram+136,x
    pha
    lda.w #$0500
    sta.b !text_pos
    lda.w #$0000
    sta.w !text_half
    pla
    jsl.l DebugPrintDecimalWord
    lda.l !gsu_ram+122,x
    pha
    lda.w #$1500
    sta.b !text_pos
    lda.w #$0000
    sta.w !text_half
    pla
    jsl.l DebugPrintDecimalWord
    lda.l !gsu_ram+134,x
    pha
    lda.w #$2900
    sta.b !text_pos
    lda.w #$0000
    sta.w !text_half
    pla
    jsl.l DebugPrintDecimalWord
    lda.l !gsu_ram+132,x
    pha
    lda.w #$3D00
    sta.b !text_pos
    lda.w #$0000
    sta.w !text_half
    pla
    jsl.l DebugPrintDecimalWord
    rtl

; @routine 08:F337 DebugFxTestPanel
; WHAT IT DOES: Prints the Super FX words $70:235C and $70:235E in
; decimal at $3980 and $39C0.
; HOW: As DebugCarsPanel.
; Entry: M and X 16-bit. Exit: A = the second word, N and Z from it.
%at($08F337)
DebugFxTestPanel:
    lda.l !fx_ram_test
    pha
    lda.w #$3980
    sta.b !text_pos
    lda.w #$0000
    sta.w !text_half
    pla
    jsl.l DebugPrintDecimalWord
    lda.l !fx_part_z_speed
    pha
    lda.w #$39C0
    sta.b !text_pos
    lda.w #$0000
    sta.w !text_half
    pla
    jsl.l DebugPrintDecimalWord
    rtl

; @routine 08:F362 DebugModelNamesPanel
; WHAT IT DOES: Prints a name a row from $0840 for each part of each model
; in the chain of the last car's record: from the record at +$3E of the
; last car, each record's parts list (+$18, each part's next at +$0E) and
; the next record at +$34. A part's name is the zero-terminated string in
; bank 0 at $1C past the word at +$14 of the record at the part's +$0C.
; HOW: The pointer at direct page $54 gets the bank 0 (STZ $55, 16-bit)
; and each name's address; DebugPrint prints it; the text position steps
; $40 a name. The record and the part are kept on the stack.
; Entry: M and X 16-bit. Exit: A = X = 0; Z set, N clear; C, V from the
; last step of the text position (from entry when nothing was printed).
%at($08F362)
DebugModelNamesPanel:
    lda.l !fx_car_last
    tax
    lda.l !gsu_ram+62,x
    tax
    pha
    lda.w #$0840
    sta.b !text_pos
    lda.w #$0000
    sta.w !text_half
    stz.b !dp_54+1
    pla
.lF37B:
    phx
    lda.l !gsu_ram+24,x
    beq .lF3A8
.lF382:
    tax
    phx
    lda.l !gsu_ram+12,x
    tax
    lda.l !gsu_ram+20,x
    clc
    adc.w #$001C
    sta.b !dp_54
    lda.b !text_pos
    pha
    jsl.l DebugPrint
    pla
    clc
    adc.w #$0040
    sta.b !text_pos
    plx
    lda.l !gsu_ram+14,x
    bne .lF382
.lF3A8:
    plx
    lda.l !gsu_ram+52,x
    tax
    bne .lF37B
    rtl

; @routine 08:F3B1 DebugModelChainPanel
; WHAT IT DOES: Prints in decimal the word +$42 of the first four records
; of the last car's model chain (from +$3E of the last car's record, each
; next at +$34), from $08C0 down a row each; then reads the viewed
; object's statistics record (+$22 of the object at $0346) and its +$80.
; HOW: As DebugCarsPanel. The last reads are left in X and A.
; Entry: M 16-bit or 8-bit, X 16-bit. Exit: M 16-bit; X = the viewed
; object's record; A = its +$80, N and Z from it.
%at($08F3B1)
DebugModelChainPanel:
    rep.b #$20
    lda.l !fx_car_last
    tax
    lda.l !gsu_ram+62,x
    tax
    lda.l !gsu_ram+66,x
    pha
    lda.w #$08C0
    sta.b !text_pos
    lda.w #$0000
    sta.w !text_half
    pla
    jsl.l DebugPrintDecimalWord
    lda.l !gsu_ram+52,x
    tax
    lda.l !gsu_ram+66,x
    pha
    lda.w #$0900
    sta.b !text_pos
    lda.w #$0000
    sta.w !text_half
    pla
    jsl.l DebugPrintDecimalWord
    lda.l !gsu_ram+52,x
    tax
    lda.l !gsu_ram+66,x
    pha
    lda.w #$0940
    sta.b !text_pos
    lda.w #$0000
    sta.w !text_half
    pla
    jsl.l DebugPrintDecimalWord
    lda.l !gsu_ram+52,x
    tax
    lda.l !gsu_ram+66,x
    pha
    lda.w #$0980
    sta.b !text_pos
    lda.w #$0000
    sta.w !text_half
    pla
    jsl.l DebugPrintDecimalWord
    lda.l !gsu_ram+52,x
    ldx.w !wram_0346
    lda.l !obj_22_long,x
    tax
    lda.l !gsu_ram+128,x
    rtl

; @routine 08:F431 DebugLastCarMotionPanel
; WHAT IT DOES: Prints in decimal five words of the last car's record
; (+$4E, +$4A, +$2E, +$30, +$32) down the column at $4900, then the
; traffic words $70:2638 and $70:263A at $1C40 and $1C80; returns the byte
; at $1F41.
; HOW: As DebugCarsPanel; SEP #$20 and LDA $1F41 at the end.
; Entry: M and X 16-bit. Exit: M 8-bit; X = the last car's record; A low
; byte = $1F41, its high byte the traffic distance's; N, Z from the byte.
%at($08F431)
DebugLastCarMotionPanel:
    lda.l !fx_car_last
    tax
    lda.l !gsu_ram+78,x
    pha
    lda.w #$4900
    sta.b !text_pos
    lda.w #$0000
    sta.w !text_half
    pla
    jsl.l DebugPrintDecimalWord
    lda.l !gsu_ram+74,x
    pha
    lda.w #$4940
    sta.b !text_pos
    lda.w #$0000
    sta.w !text_half
    pla
    jsl.l DebugPrintDecimalWord
    lda.l !gsu_ram+46,x
    pha
    lda.w #$4980
    sta.b !text_pos
    lda.w #$0000
    sta.w !text_half
    pla
    jsl.l DebugPrintDecimalWord
    lda.l !gsu_ram+48,x
    pha
    lda.w #$49C0
    sta.b !text_pos
    lda.w #$0000
    sta.w !text_half
    pla
    jsl.l DebugPrintDecimalWord
    lda.l !gsu_ram+50,x
    pha
    lda.w #$4A00
    sta.b !text_pos
    lda.w #$0000
    sta.w !text_half
    pla
    jsl.l DebugPrintDecimalWord
    lda.l !fx_traffic_close
    pha
    lda.w #$1C40
    sta.b !text_pos
    lda.w #$0000
    sta.w !text_half
    pla
    jsl.l DebugPrintDecimalWord
    lda.l !fx_traffic_dist
    pha
    lda.w #$1C80
    sta.b !text_pos
    lda.w #$0000
    sta.w !text_half
    pla
    jsl.l DebugPrintDecimalWord
    sep.b #$20
    lda.w !wram_1F3C+5
    rtl

; @routine 08:F4CF DebugTrafficHexPanel
; WHAT IT DOES: Prints in hexadecimal (one byte each: M is 8-bit) the
; Super FX bytes $70:235C, $70:235E, $70:2638 and $70:263A at $0940,
; $0980, $1D40 and $1D80, then the byte +$46 of the viewed object's
; statistics record at $3140.
; HOW: Each byte pushed while $46, $47 and $0307 are stored a byte at a
; time, pulled and printed with DebugPrintHexWord. The record address is
; the byte at +$22 of the object at $0346 with A's high byte above it
; (TAX with M 8-bit, X 16-bit).
; Entry: M 8-bit, X 16-bit. Exit: X = the record; A and P as the last
; DebugPrintHexWord leaves them.
%at($08F4CF)
DebugTrafficHexPanel:
    lda.l !fx_ram_test
    pha
    lda.b #$40
    sta.b !text_pos
    lda.b #$09
    sta.b !text_pos+1
    lda.b #$00
    sta.w !text_half
    pla
    jsl.l DebugPrintHexWord
    lda.l !fx_part_z_speed
    pha
    lda.b #$80
    sta.b !text_pos
    lda.b #$09
    sta.b !text_pos+1
    lda.b #$00
    sta.w !text_half
    pla
    jsl.l DebugPrintHexWord
    lda.l !fx_traffic_close
    pha
    lda.b #$40
    sta.b !text_pos
    lda.b #$1D
    sta.b !text_pos+1
    lda.b #$00
    sta.w !text_half
    pla
    jsl.l DebugPrintHexWord
    lda.l !fx_traffic_dist
    pha
    lda.b #$80
    sta.b !text_pos
    lda.b #$1D
    sta.b !text_pos+1
    lda.b #$00
    sta.w !text_half
    pla
    jsl.l DebugPrintHexWord
    ldx.w !wram_0346
    lda.l !obj_22_long,x
    tax
    lda.l !gsu_ram+70,x
    pha
    lda.b #$40
    sta.b !text_pos
    lda.b #$31
    sta.b !text_pos+1
    lda.b #$00
    sta.w !text_half
    pla
    jsl.l DebugPrintHexWord
    rtl

; @routine 08:F54B DebugWorkBytesPanel
; WHAT IT DOES: Prints in hexadecimal (a byte each) $1F41 at $4940, the
; negated byte $70:235E shifted right five places with its sign at $4980,
; $1F43 at $49C0, then $1F41 and $1F43 again at $0940 and $0980.
; HOW: EOR #$FF, INC A, then five CMP #$80, ROR A (an arithmetic shift);
; printing as DebugTrafficHexPanel.
; Entry: M 8-bit, X 16-bit. Exit: A and P as the last DebugPrintHexWord
; leaves them, C from the last ROR.
%at($08F54B)
DebugWorkBytesPanel:
    lda.w !wram_1F3C+5
    pha
    lda.b #$40
    sta.b !text_pos
    lda.b #$49
    sta.b !text_pos+1
    lda.b #$00
    sta.w !text_half
    pla
    jsl.l DebugPrintHexWord
    lda.l !fx_part_z_speed
    eor.b #$FF
    inc a
    cmp.b #$80
    ror a
    cmp.b #$80
    ror a
    cmp.b #$80
    ror a
    cmp.b #$80
    ror a
    cmp.b #$80
    ror a
    pha
    lda.b #$80
    sta.b !text_pos
    lda.b #$49
    sta.b !text_pos+1
    lda.b #$00
    sta.w !text_half
    pla
    jsl.l DebugPrintHexWord
    lda.w !wram_1F3C+7
    pha
    lda.b #$C0
    sta.b !text_pos
    lda.b #$49
    sta.b !text_pos+1
    lda.b #$00
    sta.w !text_half
    pla
    jsl.l DebugPrintHexWord
    lda.w !wram_1F3C+5
    pha
    lda.b #$40
    sta.b !text_pos
    lda.b #$09
    sta.b !text_pos+1
    lda.b #$00
    sta.w !text_half
    pla
    jsl.l DebugPrintHexWord
    lda.w !wram_1F3C+7
    pha
    lda.b #$80
    sta.b !text_pos
    lda.b #$09
    sta.b !text_pos+1
    lda.b #$00
    sta.w !text_half
    pla
    jsl.l DebugPrintHexWord
    rtl

; @routine 08:F5CD DebugViewRecordPanel
; WHAT IT DOES: Prints in hexadecimal (a byte each) the bytes +$10 and
; +$58 of the viewed object's statistics record at $49C0 and $0880, and
; the Super FX byte $70:2638 at $0900.
; HOW: As DebugTrafficHexPanel.
; Entry: M 8-bit, X 16-bit. Exit: X = the record; A and P as the last
; DebugPrintHexWord leaves them.
%at($08F5CD)
DebugViewRecordPanel:
    ldx.w !wram_0346
    lda.l !obj_22_long,x
    tax
    lda.l !gsu_ram+16,x
    pha
    lda.b #$C0
    sta.b !text_pos
    lda.b #$49
    sta.b !text_pos+1
    lda.b #$00
    sta.w !text_half
    pla
    jsl.l DebugPrintHexWord
    lda.l !gsu_ram+88,x
    pha
    lda.b #$80
    sta.b !text_pos
    lda.b #$08
    sta.b !text_pos+1
    lda.b #$00
    sta.w !text_half
    pla
    jsl.l DebugPrintHexWord
    lda.l !fx_traffic_close
    pha
    lda.b #$00
    sta.b !text_pos
    lda.b #$09
    sta.b !text_pos+1
    lda.b #$00
    sta.w !text_half
    pla
    jsl.l DebugPrintHexWord
    rtl

; @routine 08:F61B DebugViewSpeedPanel
; WHAT IT DOES: Prints in hexadecimal (a byte each) the speed byte +$4A
; of the viewed object's statistics record at $4B00, the Super FX byte
; $70:2482 at $4440 and $1F64 at $44C0.
; HOW: As DebugTrafficHexPanel.
; Entry: M 8-bit, X 16-bit. Exit: X = the record; A and P as the last
; DebugPrintHexWord leaves them.
%at($08F61B)
DebugViewSpeedPanel:
    ldx.w !wram_0346
    lda.l !obj_22_long,x
    tax
    lda.l !gsu_ram+74,x
    pha
    lda.b #$00
    sta.b !text_pos
    lda.b #$4B
    sta.b !text_pos+1
    lda.b #$00
    sta.w !text_half
    pla
    jsl.l DebugPrintHexWord
    lda.l !gsu_record_ready
    pha
    lda.b #$40
    sta.b !text_pos
    lda.b #$44
    sta.b !text_pos+1
    lda.b #$00
    sta.w !text_half
    pla
    jsl.l DebugPrintHexWord
    lda.w !wram_1F5A+10
    pha
    lda.b #$C0
    sta.b !text_pos
    lda.b #$44
    sta.b !text_pos+1
    lda.b #$00
    sta.w !text_half
    pla
    jsl.l DebugPrintHexWord
    rtl

; @routine 08:F668 DebugBucketsPanel
; WHAT IT DOES: Prints in hexadecimal words: the view's height row
; ($0667 / 16 + 1, made even) at $1C40, the Super FX turn push $70:247E at
; $3040, the viewed object's bucket (+$40 of the object at $0346) at
; $1E80, the view's area bucket $1EF5 at $1EC0; then the objects in each
; of those two buckets, from $7F:0000 + the bucket and along their +$3C
; links, up to five a row at $0900 and $0940.
; HOW: Each value pushed while the text position is set, pulled and
; printed with DebugPrintHexWord (16-bit: the whole word back in A); a
; printed zero ends that row.
; Entry: any widths. Exit: M and X 16-bit; X = the last object printed of
; the second row, or 0; A and P as the last print or TAX leaves them, C
; from the last LSR of the height.
%at($08F668)
DebugBucketsPanel:
    rep.b #$20
    lda.w !view_pos_y_hi
    and.w #$00FF
    lsr a
    lsr a
    lsr a
    lsr a
    inc a
    and.w #$00FE
    pha
    lda.w #$1C40
    sta.b !text_pos
    lda.w #$0000
    sta.w !text_half
    pla
    jsl.l DebugPrintHexWord
    lda.l !fx_push_turn
    pha
    lda.w #$3040
    sta.b !text_pos
    lda.w #$0000
    sta.w !text_half
    pla
    jsl.l DebugPrintHexWord
    rep.b #$10
    ldx.w !wram_0346
    lda.l !obj_bucket_long,x
    pha
    lda.w #$1E80
    sta.b !text_pos
    lda.w #$0000
    sta.w !text_half
    pla
    jsl.l DebugPrintHexWord
    lda.w !view_area_origin
    pha
    lda.w #$1EC0
    sta.b !text_pos
    lda.w #$0000
    sta.w !text_half
    pla
    jsl.l DebugPrintHexWord
    rep.b #$30
    ldx.w !wram_0346
    lda.l !obj_bucket_long,x
    tax
    lda.l !object_buckets,x
    pha
    lda.w #$0900
    sta.b !text_pos
    lda.w #$0000
    sta.w !text_half
    pla
    jsl.l DebugPrintHexWord
    tax
    beq .lF74B
    lda.l !obj_36_long+6,x
    pha
    lda.w #$1D00
    sta.b !text_pos
    lda.w #$0000
    sta.w !text_half
    pla
    jsl.l DebugPrintHexWord
    tax
    beq .lF74B
    lda.l !obj_36_long+6,x
    pha
    lda.w #$3100
    sta.b !text_pos
    lda.w #$0000
    sta.w !text_half
    pla
    jsl.l DebugPrintHexWord
    tax
    beq .lF74B
    lda.l !obj_36_long+6,x
    pha
    lda.w #$4500
    sta.b !text_pos
    lda.w #$0000
    sta.w !text_half
    pla
    jsl.l DebugPrintHexWord
    tax
    beq .lF74B
    lda.l !obj_36_long+6,x
    pha
    lda.w #$5900
    sta.b !text_pos
    lda.w #$0000
    sta.w !text_half
    pla
    jsl.l DebugPrintHexWord
.lF74B:
    lda.w !view_area_origin
    tax
    lda.l !object_buckets,x
    pha
    lda.w #$0940
    sta.b !text_pos
    lda.w #$0000
    sta.w !text_half
    pla
    jsl.l DebugPrintHexWord
    tax
    beq .lF7C4
    lda.l !obj_36_long+6,x
    pha
    lda.w #$1D40
    sta.b !text_pos
    lda.w #$0000
    sta.w !text_half
    pla
    jsl.l DebugPrintHexWord
    tax
    beq .lF7C4
    lda.l !obj_36_long+6,x
    pha
    lda.w #$3140
    sta.b !text_pos
    lda.w #$0000
    sta.w !text_half
    pla
    jsl.l DebugPrintHexWord
    tax
    beq .lF7C4
    lda.l !obj_36_long+6,x
    pha
    lda.w #$4540
    sta.b !text_pos
    lda.w #$0000
    sta.w !text_half
    pla
    jsl.l DebugPrintHexWord
    tax
    beq .lF7C4
    lda.l !obj_36_long+6,x
    pha
    lda.w #$5940
    sta.b !text_pos
    lda.w #$0000
    sta.w !text_half
    pla
    jsl.l DebugPrintHexWord
.lF7C4:
    rtl

; @routine 08:F7C5 DebugFxWordsPanel
; WHAT IT DOES: Prints in hexadecimal words the Super FX word $70:235C at
; $4940 and the last car's +$80 at $4A00.
; HOW: As DebugBucketsPanel.
; Entry: M and X 16-bit. Exit: X = the last car's record; A and P as the
; last DebugPrintHexWord leaves them.
%at($08F7C5)
DebugFxWordsPanel:
    lda.l !fx_ram_test
    pha
    lda.w #$4940
    sta.b !text_pos
    lda.w #$0000
    sta.w !text_half
    pla
    jsl.l DebugPrintHexWord
    lda.l !fx_car_last
    tax
    lda.l !gsu_ram+128,x
    pha
    lda.w #$4A00
    sta.b !text_pos
    lda.w #$0000
    sta.w !text_half
    pla
    jsl.l DebugPrintHexWord
    rtl

; @routine 08:F7F5 DebugCounterPanel
; WHAT IT DOES: Prints a column from $0440: the Super FX byte $70:108E in
; decimal, the two Super FX wait counters ($1F1F, $1F21) in hexadecimal,
; the bytes +$4C and +$4E of the object at $0346 in decimal, then six
; words in hexadecimal: the last car's +$50, +$4E, +$4A, +$3C and +$56,
; and $70:235C.
; HOW: Bytes with M 8-bit (text position stored a byte at a time), words
; with M 16-bit; each value pushed, pulled and printed.
; Entry: X 16-bit or 8-bit. Exit: M and X 16-bit; X = the last car's
; record; A and P as the last DebugPrintHexWord leaves them.
%at($08F7F5)
DebugCounterPanel:
    sep.b #$20
    lda.l !gsu_word_108c+2
    pha
    lda.b #$40
    sta.b !text_pos
    lda.b #$04
    sta.b !text_pos+1
    lda.b #$00
    sta.w !text_half
    pla
    jsl.l DebugPrintDecimalByte
    rep.b #$20
    lda.w !gsu_wait_polls
    pha
    lda.w #$0480
    sta.b !text_pos
    lda.w #$0000
    sta.w !text_half
    pla
    jsl.l DebugPrintHexWord
    lda.w !gsu_ab13_polls
    pha
    lda.w #$04C0
    sta.b !text_pos
    lda.w #$0000
    sta.w !text_half
    pla
    jsl.l DebugPrintHexWord
    rep.b #$10
    ldx.w !wram_0346
    sep.b #$20
    lda.l !obj_4a_long+2,x
    pha
    lda.b #$00
    sta.b !text_pos
    lda.b #$05
    sta.b !text_pos+1
    lda.b #$00
    sta.w !text_half
    pla
    jsl.l DebugPrintDecimalByte
    lda.l !obj_4a_long+4,x
    pha
    lda.b #$40
    sta.b !text_pos
    lda.b #$05
    sta.b !text_pos+1
    lda.b #$00
    sta.w !text_half
    pla
    jsl.l DebugPrintDecimalByte
    rep.b #$20
    lda.l !fx_car_last
    tax
    lda.l !gsu_ram+80,x
    pha
    lda.w #$0600
    sta.b !text_pos
    lda.w #$0000
    sta.w !text_half
    pla
    jsl.l DebugPrintHexWord
    lda.l !gsu_ram+78,x
    pha
    lda.w #$0640
    sta.b !text_pos
    lda.w #$0000
    sta.w !text_half
    pla
    jsl.l DebugPrintHexWord
    lda.l !gsu_ram+74,x
    pha
    lda.w #$0680
    sta.b !text_pos
    lda.w #$0000
    sta.w !text_half
    pla
    jsl.l DebugPrintHexWord
    lda.l !gsu_ram+60,x
    pha
    lda.w #$06C0
    sta.b !text_pos
    lda.w #$0000
    sta.w !text_half
    pla
    jsl.l DebugPrintHexWord
    lda.l !gsu_ram+86,x
    pha
    lda.w #$0700
    sta.b !text_pos
    lda.w #$0000
    sta.w !text_half
    pla
    jsl.l DebugPrintHexWord
    lda.l !fx_ram_test
    pha
    lda.w #$0740
    sta.b !text_pos
    lda.w #$0000
    sta.w !text_half
    pla
    jsl.l DebugPrintHexWord
    rtl

; @routine 08:F8F3 DebugPanelSep
; WHAT IT DOES: Sets M 8-bit and returns: an empty panel.
; HOW: SEP #$20, RTL.
; Entry: any. Exit: M 8-bit; the rest unchanged.
%at($08F8F3)
DebugPanelSep:
    sep.b #$20
    rtl

; @routine 08:F8F6 DebugFxTestWordPanel
; WHAT IT DOES: Prints the Super FX word $70:235C in hexadecimal at
; $5040.
; HOW: Reads $1F1B first (left unused: A is loaded again at once); then
; as DebugFxWordsPanel.
; Entry: any widths. Exit: M and X 16-bit; A and P as DebugPrintHexWord
; leaves them.
%at($08F8F6)
DebugFxTestWordPanel:
    rep.b #$30
    lda.w !draw_side+20
    lda.l !fx_ram_test
    pha
    lda.w #$5040
    sta.b !text_pos
    lda.w #$0000
    sta.w !text_half
    pla
    jsl.l DebugPrintHexWord
    rtl
