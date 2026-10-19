; params.asm: the parameter block in Super FX RAM that the renderer reads.
; C: src/stuntracefx/gsu/params.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"

arch 65816

; @routine 7E:E2C9 GsuParamsUpdate
; WHAT IT DOES: Copies the view parameters the renderer needs from work RAM
; into its parameter block in Super FX RAM.
; HOW: Eight 16-bit copies: $0D12 to $70:108A; direct page $C5, $C7, $C9 to
; $70:00D8, 00DA, 00DC; $0664, $0666, $0668 to $70:00DE, 00E0, 00E2; $05FA to
; $70:2736.
; Entry: any widths, D = $0000. Exit: A = the last value copied; P unchanged.
%ramcode($7EE2C9, $013FF0)
GsuParamsUpdate:
    php
    rep.b #$30
    lda.w !wram_0D12
    sta.l !gsu_param_108a
    lda.b !dp_c5
    sta.l !gsu_param_00d8
    lda.b !dp_c7
    sta.l !gsu_param_00da
    lda.b !dp_c9
    sta.l !gsu_param_00dc
    lda.w !wram_0664
    sta.l !gsu_param_00de
    lda.w !wram_0666
    sta.l !gsu_param_00e0
    lda.w !wram_0668
    sta.l !gsu_param_00e2
    lda.w !wram_05FA
    sta.l !gsu_param_2736
    plp
    rtl

; @routine 03:AEB6 GsuParamsFromDp
; WHAT IT DOES: Marks $05EF with $FF and copies eight direct-page words into
; the renderer's inputs at $70:0034-$70:003E, $70:0092 and $70:0094.
; HOW: Word copies from $CE, $D0, $2A, $2E, $2C, $30, $32, $34, in that
; order; $05EF is reached through the data bank.
; Entry: M 8-bit, D = $0000. Exit: A = the last word; M 16-bit; N, Z from A.
%at($03AEB6)
GsuParamsFromDp:
    lda.b #$FF
    sta.w !wram_05EF
    rep.b #$20
    lda.b !dp_ce
    sta.l !gsu_in_34
    lda.b !dp_d0
    sta.l !gsu_in_36
    lda.b !dp_2a
    sta.l !gsu_in_38
    lda.b !dp_2e
    sta.l !gsu_in_3a
    lda.b !dp_2c
    sta.l !gsu_in_3c
    lda.b !dp_30
    sta.l !gsu_in_3e
    lda.b !dp_32
    sta.l !gsu_in_92
    lda.b !dp_34
    sta.l !gsu_in_94
    rtl

; @routine 07:8E2A GsuParam216A
; WHAT IT DOES: Sets the renderer input at $70:0216 to entry A of the word
; table at $8E3E in the data bank.
; HOW: A * 2 as the index (8 bits when X is 8-bit).
; Entry: M 16-bit; A = entry. Exit: A = the value; X = the index; N, Z from
; A; C from the doubling.
%at($078E2A)
GsuParam216A:
    asl a
    tax
    lda.w !param_216_table_a,x
    sta.l !gsu_in_216
    rts

; @routine 07:8E34 GsuParam216B
; WHAT IT DOES: As GsuParam216A, from the table at $8E52.
; HOW: As GsuParam216A.
; Entry: as GsuParam216A. Exit: as GsuParam216A.
%at($078E34)
GsuParam216B:
    asl a
    tax
    lda.w !param_216_table_b,x
    sta.l !gsu_in_216
    rts

; @routine 08:DC13 GsuTableEntryTo1F4C
; WHAT IT DOES: Copies three words of entry A (counted from 1) of a table of
; 8-byte entries into $1F4C-$1F51; the table's bank is the byte at $70:1094
; and its address the word at $70:1092, and the words start 2 bytes into
; the entry.
; HOW: The table's bank becomes the data bank for the copy, so $1F4C and
; the table are both reached through it.
; Entry: M 16-bit, X 16-bit; A = entry. Exit: A = the third word; X
; unchanged; N, Z from X; C, V from the address add.
%at($08DC13)
GsuTableEntryTo1F4C:
    sta.w !wram_1F4C
    phx
    lda.l !gsu_table_bank
    sep.b #$20
    and.b #$FF
    phb
    pha
    plb
    rep.b #$20
    lda.w !wram_1F4C
    dec a
    asl a
    asl a
    asl a
    clc
    adc.l !gsu_table_addr
    inc a
    inc a
    tax
    lda.w 0,x
    sta.w !wram_1F4C
    lda.w 2,x
    sta.w !wram_1F4E
    lda.w 4,x
    sta.w !wram_1F50
    plb
    plx
    rtl

; @routine 04:D7A2 GsuBytes200_BFBABD
; WHAT IT DOES: Sets the four renderer input bytes at $70:0200-$70:0203 to
; 0, $BF, $BA, $BD.
; HOW: Four byte stores (gsu_200).
; Entry: any widths. Exit: A low byte = $BD; P unchanged.
%at($04D7A2)
GsuBytes200_BFBABD:
    php
    sep.b #$20
    lda.b #$00
    sta.l !gsu_bytes_200
    lda.b #$BF
    sta.l !gsu_bytes_200+1
    lda.b #$BA
    sta.l !gsu_bytes_200+2
    lda.b #$BD
    sta.l !gsu_bytes_200+3
    plp
    rtl

; @routine 04:D7BF GsuBytes200_BFB7B6
; WHAT IT DOES: Sets the four renderer input bytes at $70:0200-$70:0203 to
; 0, $BF, $B7, $B6.
; HOW: Four byte stores (gsu_200).
; Entry: any widths. Exit: A low byte = $B6; P unchanged.
%at($04D7BF)
GsuBytes200_BFB7B6:
    php
    sep.b #$20
    lda.b #$00
    sta.l !gsu_bytes_200
    lda.b #$BF
    sta.l !gsu_bytes_200+1
    lda.b #$B7
    sta.l !gsu_bytes_200+2
    lda.b #$B6
    sta.l !gsu_bytes_200+3
    plp
    rtl

; @routine 04:D7DC GsuBytes200_BFB5BB
; WHAT IT DOES: As GsuBytes200_BFB7B6, with 0, $BF, $B5, $BB.
; HOW: As GsuBytes200_BFB7B6.
; Entry: any widths. Exit: A low byte = $BB; P unchanged.
%at($04D7DC)
GsuBytes200_BFB5BB:
    php
    sep.b #$20
    lda.b #$00
    sta.l !gsu_bytes_200
    lda.b #$BF
    sta.l !gsu_bytes_200+1
    lda.b #$B5
    sta.l !gsu_bytes_200+2
    lda.b #$BB
    sta.l !gsu_bytes_200+3
    plp
    rtl

; @routine 04:D938 GsuBytes200_0F060E
; WHAT IT DOES: As GsuBytes200_BFB7B6, with 0, $0F, $06, $0E.
; HOW: As GsuBytes200_BFB7B6.
; Entry: any widths. Exit: A low byte = $0E; P unchanged.
%at($04D938)
GsuBytes200_0F060E:
    php
    sep.b #$20
    lda.b #$00
    sta.l !gsu_bytes_200
    lda.b #$0F
    sta.l !gsu_bytes_200+1
    lda.b #$06
    sta.l !gsu_bytes_200+2
    lda.b #$0E
    sta.l !gsu_bytes_200+3
    plp
    rtl

; @routine 04:D955 GsuBytes200_0F010B
; WHAT IT DOES: As GsuBytes200_BFB7B6, with 0, $0F, $01, $0B. Nothing
; calls it (no JSL, pointer or table reaches it), so a direct call is its
; only way in.
; HOW: As GsuBytes200_BFB7B6.
; Entry: any widths. Exit: A low byte = $0B; P unchanged.
%at($04D955)
GsuBytes200_0F010B:
    php
    sep.b #$20
    lda.b #$00
    sta.l !gsu_bytes_200
    lda.b #$0F
    sta.l !gsu_bytes_200+1
    lda.b #$01
    sta.l !gsu_bytes_200+2
    lda.b #$0B
    sta.l !gsu_bytes_200+3
    plp
    rtl

; @routine 04:D7F9 GsuBytes200_BFBCBD
; WHAT IT DOES: As GsuBytes200_BFB7B6, with 0, $BF, $BC, $BD.
; HOW: As GsuBytes200_BFB7B6.
; Entry: any widths. Exit: A low byte = $BD; P unchanged.
%at($04D7F9)
GsuBytes200_BFBCBD:
    php
    sep.b #$20
    lda.b #$00
    sta.l !gsu_bytes_200
    lda.b #$BF
    sta.l !gsu_bytes_200+1
    lda.b #$BC
    sta.l !gsu_bytes_200+2
    lda.b #$BD
    sta.l !gsu_bytes_200+3
    plp
    rtl

; @routine 04:D816 GsuBytes200_BFBAB9
; WHAT IT DOES: As GsuBytes200_BFB7B6, with 0, $BF, $BA, $B9.
; HOW: As GsuBytes200_BFB7B6.
; Entry: any widths. Exit: A low byte = $B9; P unchanged.
%at($04D816)
GsuBytes200_BFBAB9:
    php
    sep.b #$20
    lda.b #$00
    sta.l !gsu_bytes_200
    lda.b #$BF
    sta.l !gsu_bytes_200+1
    lda.b #$BA
    sta.l !gsu_bytes_200+2
    lda.b #$B9
    sta.l !gsu_bytes_200+3
    plp
    rtl

; @routine 04:D833 GsuBytes200_BFB3BE
; WHAT IT DOES: As GsuBytes200_BFB7B6, with 0, $BF, $B3, $BE.
; HOW: As GsuBytes200_BFB7B6.
; Entry: any widths. Exit: A low byte = $BE; P unchanged.
%at($04D833)
GsuBytes200_BFB3BE:
    php
    sep.b #$20
    lda.b #$00
    sta.l !gsu_bytes_200
    lda.b #$BF
    sta.l !gsu_bytes_200+1
    lda.b #$B3
    sta.l !gsu_bytes_200+2
    lda.b #$BE
    sta.l !gsu_bytes_200+3
    plp
    rtl

; @routine 04:D850 GsuBytes200_BFB2B3
; WHAT IT DOES: As GsuBytes200_BFB7B6, with 0, $BF, $B2, $B3.
; HOW: As GsuBytes200_BFB7B6. Not reached in the traces so far.
; Entry: any widths. Exit: A low byte = $B3; P unchanged.
%at($04D850)
GsuBytes200_BFB2B3:
    php
    sep.b #$20
    lda.b #$00
    sta.l !gsu_bytes_200
    lda.b #$BF
    sta.l !gsu_bytes_200+1
    lda.b #$B2
    sta.l !gsu_bytes_200+2
    lda.b #$B3
    sta.l !gsu_bytes_200+3
    plp
    rtl

; @routine 04:D86D GsuBytes200_BF12B6
; WHAT IT DOES: As GsuBytes200_BFB7B6, with 0, $BF, $12, $B6.
; HOW: As GsuBytes200_BFB7B6. Not reached in the traces so far.
; Entry: any widths. Exit: A low byte = $B6; P unchanged.
%at($04D86D)
GsuBytes200_BF12B6:
    php
    sep.b #$20
    lda.b #$00
    sta.l !gsu_bytes_200
    lda.b #$BF
    sta.l !gsu_bytes_200+1
    lda.b #$12
    sta.l !gsu_bytes_200+2
    lda.b #$B6
    sta.l !gsu_bytes_200+3
    plp
    rtl

; @routine 04:D88A GsuBytes200_BF11BD
; WHAT IT DOES: As GsuBytes200_BFB7B6, with 0, $BF, $11, $BD.
; HOW: As GsuBytes200_BFB7B6. Not reached in the traces so far.
; Entry: any widths. Exit: A low byte = $BD; P unchanged.
%at($04D88A)
GsuBytes200_BF11BD:
    php
    sep.b #$20
    lda.b #$00
    sta.l !gsu_bytes_200
    lda.b #$BF
    sta.l !gsu_bytes_200+1
    lda.b #$11
    sta.l !gsu_bytes_200+2
    lda.b #$BD
    sta.l !gsu_bytes_200+3
    plp
    rtl

; @routine 04:D8A7 GsuBytes200_BF10B9
; WHAT IT DOES: As GsuBytes200_BFB7B6, with 0, $BF, $10, $B9.
; HOW: As GsuBytes200_BFB7B6. Not reached in the traces so far.
; Entry: any widths. Exit: A low byte = $B9; P unchanged.
%at($04D8A7)
GsuBytes200_BF10B9:
    php
    sep.b #$20
    lda.b #$00
    sta.l !gsu_bytes_200
    lda.b #$BF
    sta.l !gsu_bytes_200+1
    lda.b #$10
    sta.l !gsu_bytes_200+2
    lda.b #$B9
    sta.l !gsu_bytes_200+3
    plp
    rtl

; @routine 04:D8C4 GsuBytes200_BFB7B6_D8C4
; WHAT IT DOES: As GsuBytes200_BFB7B6, with 0, $BF, $B7, $B6
; (the same bytes as 04:D7BF).
; HOW: As GsuBytes200_BFB7B6. Not reached in the traces so far.
; Entry: any widths. Exit: A low byte = $B6; P unchanged.
%at($04D8C4)
GsuBytes200_BFB7B6_D8C4:
    php
    sep.b #$20
    lda.b #$00
    sta.l !gsu_bytes_200
    lda.b #$BF
    sta.l !gsu_bytes_200+1
    lda.b #$B7
    sta.l !gsu_bytes_200+2
    lda.b #$B6
    sta.l !gsu_bytes_200+3
    plp
    rtl

; @routine 04:D8E1 GsuBytes200_BFBAB9_D8E1
; WHAT IT DOES: As GsuBytes200_BFB7B6, with 0, $BF, $BA, $B9
; (the same bytes as 04:D816).
; HOW: As GsuBytes200_BFB7B6. Not reached in the traces so far.
; Entry: any widths. Exit: A low byte = $B9; P unchanged.
%at($04D8E1)
GsuBytes200_BFBAB9_D8E1:
    php
    sep.b #$20
    lda.b #$00
    sta.l !gsu_bytes_200
    lda.b #$BF
    sta.l !gsu_bytes_200+1
    lda.b #$BA
    sta.l !gsu_bytes_200+2
    lda.b #$B9
    sta.l !gsu_bytes_200+3
    plp
    rtl

; @routine 04:D8FE GsuBytes200_BFB3BE_D8FE
; WHAT IT DOES: As GsuBytes200_BFB7B6, with 0, $BF, $B3, $BE
; (the same bytes as 04:D833).
; HOW: As GsuBytes200_BFB7B6. Not reached in the traces so far.
; Entry: any widths. Exit: A low byte = $BE; P unchanged.
%at($04D8FE)
GsuBytes200_BFB3BE_D8FE:
    php
    sep.b #$20
    lda.b #$00
    sta.l !gsu_bytes_200
    lda.b #$BF
    sta.l !gsu_bytes_200+1
    lda.b #$B3
    sta.l !gsu_bytes_200+2
    lda.b #$BE
    sta.l !gsu_bytes_200+3
    plp
    rtl

; @routine 04:D91B GsuBytes200_BFBBBE
; WHAT IT DOES: As GsuBytes200_BFB7B6, with 0, $BF, $BB, $BE.
; HOW: As GsuBytes200_BFB7B6. Not reached in the traces so far.
; Entry: any widths. Exit: A low byte = $BE; P unchanged.
%at($04D91B)
GsuBytes200_BFBBBE:
    php
    sep.b #$20
    lda.b #$00
    sta.l !gsu_bytes_200
    lda.b #$BF
    sta.l !gsu_bytes_200+1
    lda.b #$BB
    sta.l !gsu_bytes_200+2
    lda.b #$BE
    sta.l !gsu_bytes_200+3
    plp
    rtl

; @routine 04:D972 GsuBytes200_0F0405
; WHAT IT DOES: As GsuBytes200_BFB7B6, with 0, $0F, $04, $05.
; HOW: As GsuBytes200_BFB7B6.
; Entry: any widths. Exit: A low byte = $05; P unchanged.
%at($04D972)
GsuBytes200_0F0405:
    php
    sep.b #$20
    lda.b #$00
    sta.l !gsu_bytes_200
    lda.b #$0F
    sta.l !gsu_bytes_200+1
    lda.b #$04
    sta.l !gsu_bytes_200+2
    lda.b #$05
    sta.l !gsu_bytes_200+3
    plp
    rtl

; @routine 04:D9AC GsuBytes200_0F090A
; WHAT IT DOES: As GsuBytes200_BFB7B6, with 0, $0F, $09, $0A.
; HOW: As GsuBytes200_BFB7B6.
; Entry: any widths. Exit: A low byte = $0A; P unchanged.
%at($04D9AC)
GsuBytes200_0F090A:
    php
    sep.b #$20
    lda.b #$00
    sta.l !gsu_bytes_200
    lda.b #$0F
    sta.l !gsu_bytes_200+1
    lda.b #$09
    sta.l !gsu_bytes_200+2
    lda.b #$0A
    sta.l !gsu_bytes_200+3
    plp
    rtl

; @routine 04:D9E6 GsuBytes200_0F050E
; WHAT IT DOES: As GsuBytes200_BFB7B6, with 0, $0F, $05, $0E.
; HOW: As GsuBytes200_BFB7B6.
; Entry: any widths. Exit: A low byte = $0E; P unchanged.
%at($04D9E6)
GsuBytes200_0F050E:
    php
    sep.b #$20
    lda.b #$00
    sta.l !gsu_bytes_200
    lda.b #$0F
    sta.l !gsu_bytes_200+1
    lda.b #$05
    sta.l !gsu_bytes_200+2
    lda.b #$0E
    sta.l !gsu_bytes_200+3
    plp
    rtl

; @routine 04:DA03 GsuBytes200_0F080E
; WHAT IT DOES: As GsuBytes200_BFB7B6, with 0, $0F, $08, $0E.
; HOW: As GsuBytes200_BFB7B6.
; Entry: any widths. Exit: A low byte = $0E; P unchanged.
%at($04DA03)
GsuBytes200_0F080E:
    php
    sep.b #$20
    lda.b #$00
    sta.l !gsu_bytes_200
    lda.b #$0F
    sta.l !gsu_bytes_200+1
    lda.b #$08
    sta.l !gsu_bytes_200+2
    lda.b #$0E
    sta.l !gsu_bytes_200+3
    plp
    rtl

; @routine 08:DC48 GsuTableEntryNext
; WHAT IT DOES: Returns entry A + 1 of the table whose bank is the byte at
; $70:1094 and address the word at $70:1092, or 1 when A equals the
; table's first word (its last entry).
; HOW: A is stored at $1F4C through the caller's data bank and read back
; through the table's (as GsuTableEntryTo1F4C does).
; Entry: M 16-bit, X 16-bit; A = entry. Exit: A = the next entry; X
; unchanged; N, Z from X; C from the compare.
%at($08DC48)
GsuTableEntryNext:
    sta.w !wram_1F4C
    phx
    lda.l !gsu_table_bank
    sep.b #$20
    and.b #$FF
    phb
    pha
    plb
    rep.b #$20
    lda.l !gsu_table_addr
    tax
    lda.w !wram_1F4C
    cmp.w 0,x
    bne .lDC69
    lda.w #$0000
.lDC69:
    inc a
    plb
    plx
    rtl

; @routine 02:DA34 GsuRecordsUpload
; WHAT IT DOES: Copies the $0CEE 30-byte records at $7E:F77F to Super FX
; RAM at $70:0AAA and stores the count at $70:01C8.
; HOW: Fifteen word copies per record with the data bank at $70; the count
; runs down in $1F2B.
; Entry: any widths. Exit: X, Y past the copies (unchanged when the count
; is 0); A = 0; M 16-bit, X 16-bit; N, Z from the data bank pulled back;
; C, V from the last source step (unchanged when the count is 0).
%at($02DA34)
GsuRecordsUpload:
    sep.b #$20
    phb
    lda.b #$70
    pha
    plb
    rep.b #$30
    lda.l !gsu_record_count
    sta.l !gsu_records_n
    bne .lDA4A
    brl .lDAD4
.lDA4A:
    ldx.w #$F77F
    ldy.w #$0AAA
    sta.l !wram_1F2B
.lDA54:
    lda.l !wram_bank_7e,x
    sta.w 0,y
    lda.l !wram_bank_7e+2,x
    sta.w 2,y
    lda.l !wram_bank_7e+4,x
    sta.w 4,y
    lda.l !wram_bank_7e+6,x
    sta.w 6,y
    lda.l !wram_bank_7e+8,x
    sta.w 8,y
    lda.l !wram_bank_7e+10,x
    sta.w 10,y
    lda.l !wram_bank_7e+12,x
    sta.w 12,y
    lda.l !wram_bank_7e+14,x
    sta.w 14,y
    lda.l !wram_bank_7e+16,x
    sta.w 16,y
    lda.l !wram_bank_7e+18,x
    sta.w 18,y
    lda.l !wram_bank_7e+20,x
    sta.w 20,y
    lda.l !wram_bank_7e+22,x
    sta.w 22,y
    lda.l !wram_bank_7e+24,x
    sta.w 24,y
    lda.l !wram_bank_7e+26,x
    sta.w 26,y
    lda.l !wram_bank_7e+28,x
    sta.w 28,y
    tya
    clc
    adc.w #$001E
    tay
    txa
    clc
    adc.w #$001E
    tax
    lda.l !wram_1F2B
    dec a
    sta.l !wram_1F2B
    bne .lDA54
.lDAD4:
    plb
    rtl

; @routine 04:D98F GsuBytes200_0F0708
; WHAT IT DOES: As GsuBytes200_BFB7B6, with 0, $0F, $07, $08.
; HOW: As GsuBytes200_BFB7B6.
; Entry: any widths. Exit: A low byte = $08; P unchanged.
%at($04D98F)
GsuBytes200_0F0708:
    php
    sep.b #$20
    lda.b #$00
    sta.l !gsu_bytes_200
    lda.b #$0F
    sta.l !gsu_bytes_200+1
    lda.b #$07
    sta.l !gsu_bytes_200+2
    lda.b #$08
    sta.l !gsu_bytes_200+3
    plp
    rtl

; @routine 04:D9C9 GsuBytes200_0F0D03
; WHAT IT DOES: As GsuBytes200_BFB7B6, with 0, $0F, $0D, $03.
; HOW: As GsuBytes200_BFB7B6.
; Entry: any widths. Exit: A low byte = $03; P unchanged.
%at($04D9C9)
GsuBytes200_0F0D03:
    php
    sep.b #$20
    lda.b #$00
    sta.l !gsu_bytes_200
    lda.b #$0F
    sta.l !gsu_bytes_200+1
    lda.b #$0D
    sta.l !gsu_bytes_200+2
    lda.b #$03
    sta.l !gsu_bytes_200+3
    plp
    rtl
