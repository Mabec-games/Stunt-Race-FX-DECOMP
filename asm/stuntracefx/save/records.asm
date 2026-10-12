; records.asm: five-byte records in the save data (Super FX RAM $70:FC4A and
; $70:FC5E onwards).
; C: src/stuntracefx/save/records.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 07:ADCC SaveRecordSumA
; WHAT IT DOES: Returns the 8-bit sum of the five bytes at $70:FC4A + X.
; HOW: Five additions, each with carry cleared first.
; Entry: M 8-bit, X 8-bit; X = offset. Exit: A low byte = the sum; X + 5;
; Y = 0; Z set, N clear; C, V from the last add.
%at($07ADCC)
SaveRecordSumA:
    ldy.b #$05
    lda.b #$00
.lADD0:
    clc
    adc.l !save_record_a,x
    inx
    dey
    bne .lADD0
    rts

; @routine 07:ADF9 SaveRecordSumB
; WHAT IT DOES: Returns the 8-bit sum of the five bytes at $70:FC5E + X.
; HOW: As SaveRecordSumA.
; Entry: as SaveRecordSumA. Exit: as SaveRecordSumA.
%at($07ADF9)
SaveRecordSumB:
    ldy.b #$05
    lda.b #$00
.lADFD:
    clc
    adc.l !save_record_b,x
    inx
    dey
    bne .lADFD
    rts

; @routine 07:B0D1 SaveRecordsCheckA
; WHAT IT DOES: Validates the four records at $70:FC4A against their sum
; bytes at $70:FC46; a record that fails, or whose first byte is 0, is reset
; to FF FF 00 00 00 with sum byte $FE (which is its sum).
; HOW: SaveRecordSumA for each record, through the data bank.
; Entry: any widths; DB = $70. Exit: A low byte = $FE when the last record
; was reset, else its first byte; X = 20; Y = 4; M 8-bit, X 8-bit; C, Z
; set, N clear; V from the last sum.
%at($07B0D1)
SaveRecordsCheckA:
    sep.b #$30
    ldx.b #$00
    ldy.b #$00
.record:
    phx
    phy
    jsr.w SaveRecordSumA
    ply
    plx
    cmp.w !save_sums_a_w,y
    bne .reset
    lda.w !save_record_a_w,x
    bne .next
.reset:
    lda.b #$FF
    sta.w !save_record_a_w,x
    sta.w !save_record_a_w+1,x
    lda.b #$00
    sta.w !save_record_a_w+2,x
    sta.w !save_record_a_w+3,x
    sta.w !save_record_a_w+4,x
    lda.b #$FE
    sta.w !save_sums_a_w,y
.next:
    inx
    inx
    inx
    inx
    inx
    iny
    cpy.b #$04
    bcc .record
    rts

; @routine 07:B097 SaveRecordsCheckB
; WHAT IT DOES: As SaveRecordsCheckA, for the three records at $70:FC5E
; and their sum bytes at $70:FC43.
; HOW: SaveRecordSumB for each record, through the data bank.
; Entry: any widths; DB = $70. Exit: A low byte = $FE when the last record
; was reset, else its first byte; X = 15; Y = 3; M 8-bit, X 8-bit; C, Z
; set, N clear; V from the last sum.
%at($07B097)
SaveRecordsCheckB:
    sep.b #$30
    ldx.b #$00
    ldy.b #$00
.record:
    phx
    phy
    jsr.w SaveRecordSumB
    ply
    plx
    cmp.w !save_sums_b_w,y
    bne .reset
    lda.w !save_record_b_w,x
    bne .next
.reset:
    lda.b #$FF
    sta.w !save_record_b_w,x
    sta.w !save_record_b_w+1,x
    lda.b #$00
    sta.w !save_record_b_w+2,x
    sta.w !save_record_b_w+3,x
    sta.w !save_record_b_w+4,x
    lda.b #$FE
    sta.w !save_sums_b_w,y
.next:
    inx
    inx
    inx
    inx
    inx
    iny
    cpy.b #$03
    bcc .record
    rts

; @routine 07:B190 SaveBlocksCheck
; WHAT IT DOES: Validates blocks 12-15 of the 48-byte blocks at $70:FCD0:
; each is twelve 4-byte entries whose second byte must be below $41, and
; the 8-bit sum of all 48 bytes must equal the block's sum byte at
; $70:FC33 + block. A block that fails is reset to twelve entries of 00 00
; FF FF, with sum byte $E8 (which is their sum).
; HOW: Direct page $F1 holds the running sum, $F3 the block, $F5 the
; block's offset; tables are read through the data bank. A second byte of
; $41 or more fails the block at once.
; Entry: M 8-bit, X 16-bit, D = $0000; DB = $70. Exit: A = $0210; X = $0F;
; Y = 0; $F3 = $10, $F5 = $02D0; M 8-bit, X 16-bit; C, Z set, N clear; V
; from the last add.
%at($07B190)
SaveBlocksCheck:
    rep.b #$10
    ldx.w #$000C
    stx.b !save_block
.block:
    rep.b #$20
    lda.b !save_block
    asl a
    clc
    adc.b !save_block
    asl a
    asl a
    asl a
    asl a
    tax
    sta.b !save_block_offset
    stz.b !save_sum
    ldy.w #$000C
    sep.b #$20
.entry:
    clc
    lda.w !save_blocks_w,x
    adc.b !save_sum
    sta.b !save_sum
    lda.w !save_blocks_w+1,x
    cmp.b #$41
    bcs .reset
    clc
    adc.b !save_sum
    sta.b !save_sum
    clc
    lda.w !save_blocks_w+2,x
    adc.b !save_sum
    sta.b !save_sum
    clc
    lda.w !save_blocks_w+3,x
    adc.b !save_sum
    sta.b !save_sum
    inx
    inx
    inx
    inx
    dey
    bne .entry
    ldx.b !save_block
    lda.w !save_block_sums_w,x
    cmp.b !save_sum
    bne .reset
.next:
    inc.b !save_block
    lda.b !save_block
    cmp.b #$10
    bcc .block
    rts
.reset:
    ldx.b !save_block_offset
    sep.b #$20
    ldy.w #$000C
.clear:
    lda.b #$00
    sta.w !save_blocks_w,x
    sta.w !save_blocks_w+1,x
    lda.b #$FF
    sta.w !save_blocks_w+2,x
    sta.w !save_blocks_w+3,x
    inx
    inx
    inx
    inx
    dey
    bne .clear
    ldx.b !save_block
    lda.b #$E8
    sta.w !save_block_sums_w,x
    bra .next
