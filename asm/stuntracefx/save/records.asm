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

; @routine 07:B10B SaveTablesCheck
; WHAT IT DOES: Validates three save tables, last first: table k is twelve
; two-byte entries at $70:FC88 + 24k and nine bytes at $70:FC6D + 9k, and
; every entry's second byte must be $1F or more; the 8-bit sum of all 33
; bytes must equal the sum byte at $70:FC2D + k. A table that fails is reset
; to 24 bytes of $FF and nine of 0, with sum byte $E8 (which is their sum).
; HOW: The table's two offsets come from the byte tables at $07:B18D and
; $07:B18A into direct page $F3 and $F5 (high bytes $F4, $F6 cleared);
; $F1 holds the running sum. Tables are read through the data bank. A
; second byte below $1F fails the table at once.
; Entry: any widths, D = $0000; DB = $70. Exit: A = the last sum byte read
; or written (high byte clear); X = $FFFF; Y = 0; M 8-bit, X 16-bit; N
; set, Z clear; C, V as the last table's check left them.
%at($07B10B)
SaveTablesCheck:
    rep.b #$30
    lda.w #$0000
    ldx.w #$0002
    sep.b #$20
    sta.b !table_first+1
    sta.b !table_second+1
.table:
    phx
    lda.l !save_table_first,x
    sta.b !table_first
    lda.l !save_table_second,x
    sta.b !table_second
    stz.b !save_sum
    ldy.w #$000C
    ldx.b !table_first
.entry:
    clc
    lda.w !save_tables_a_w,x
    adc.b !save_sum
    sta.b !save_sum
    lda.w !save_tables_a_w+1,x
    cmp.b #$1F
    bcc .short
    clc
    adc.b !save_sum
    sta.b !save_sum
    inx
    inx
    dey
    bne .entry
    ldx.b !table_second
    ldy.w #$0009
.tail:
    clc
    lda.w !save_tables_b_w,x
    adc.b !save_sum
    sta.b !save_sum
    inx
    dey
    bne .tail
    plx
    lda.w !save_table_sums,x
    cmp.b !save_sum
    bne .reset
    bra .next
.short:
    plx
.reset:
    phx
    lda.b #$FF
    ldx.b !table_first
    ldy.w #$0018
.fill:
    sta.w !save_tables_a_w,x
    inx
    dey
    bne .fill
    ldx.b !table_second
    lda.b #$00
    ldy.w #$0009
.clear:
    sta.w !save_tables_b_w,x
    inx
    dey
    bne .clear
    plx
    lda.b #$E8
    sta.l !save_table_sums,x
.next:
    dex
    bpl .table
    rts

; @routine 07:B211 SaveBlocksValidate
; WHAT IT DOES: Validates blocks 0-11 of the 48-byte blocks at $70:FCD0:
; each is twelve 4-byte entries whose bytes must be $FF or in range (first
; byte's low seven bits below 100, second byte's low five bits below 30 and
; top three bits not 0, third byte's low five bits below 28); the 8-bit sum
; of the block's four bytes at $70:FFD0 + 4 * block and its 48 bytes must
; equal its sum byte at $70:FC33 + block. A block that fails sets $1FFE to
; $11 and is reset to all $FF, its four bytes at $70:FFD0 too, with sum
; byte $CC (which is their sum); then it is checked again.
; HOW: Direct page $F1 holds the running sum, $F3 the block, $F5 its
; offset. The block bytes are read through the data bank, the four-byte
; groups long. The fourth entry byte's test, low two bits $38 or more, can
; never fail.
; Entry: M 8-bit, D = $0000; DB = $70. Exit: A = $020C; X = $0B; Y = 0;
; $F3 = $0C; M 8-bit, X 16-bit; C, Z set, N clear; V from the last add.
%at($07B211)
SaveBlocksValidate:
    rep.b #$10
    ldx.w #$0000
    stx.b !save_block
.block:
    rep.b #$20
    lda.b !save_block
    asl a
    asl a
    tax
    sep.b #$20
    lda.l !save_block_groups,x
    sta.b !save_sum
    lda.l !save_block_groups+1,x
    clc
    adc.b !save_sum
    sta.b !save_sum
    lda.l !save_block_groups+2,x
    clc
    adc.b !save_sum
    sta.b !save_sum
    lda.l !save_block_groups+3,x
    clc
    adc.b !save_sum
    rep.b #$20
    and.w #$00FF
    sta.b !save_sum
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
    ldy.w #$000C
    sep.b #$20
    clc
.entry:
    lda.w !save_blocks_w,x
    pha
    cmp.b #$FF
    beq .first
    and.b #$7F
    cmp.b #$64
    bcc .first
    pla
    bra .reset
.first:
    pla
    clc
    adc.b !save_sum
    sta.b !save_sum
    lda.w !save_blocks_w+1,x
    pha
    cmp.b #$FF
    beq .second
    and.b #$1F
    cmp.b #$1E
    bcc .high
    pla
    bra .reset
.high:
    pla
    pha
    and.b #$E0
    cmp.b #$20
    bcs .second
    pla
    bra .reset
.second:
    pla
    clc
    adc.b !save_sum
    sta.b !save_sum
    lda.w !save_blocks_w+2,x
    pha
    cmp.b #$FF
    beq .third
    and.b #$1F
    cmp.b #$1C
    bcc .third
    pla
    bra .reset
.third:
    pla
    clc
    adc.b !save_sum
    sta.b !save_sum
    lda.w !save_blocks_w+3,x
    pha
    cmp.b #$FF
    beq .fourth
    and.b #$03
    cmp.b #$38
    bcc .fourth
    pla
    bra .reset
.fourth:
    pla
    clc
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
    inc.b !save_block
    lda.b !save_block
    cmp.b #$0C
    bcs .done
    jmp.w .block
.done:
    rts
.reset:
    pha
    lda.b #$11
    sta.l !save_bad_flag
    pla
    rep.b #$20
    ldx.b !save_block_offset
    sep.b #$20
    ldy.w #$0030
    lda.b #$FF
.fill:
    sta.w !save_blocks_w,x
    inx
    dey
    bne .fill
    ldx.b !save_block
    rep.b #$20
    lda.b !save_block
    and.w #$00FF
    asl a
    asl a
    tay
    sep.b #$20
    lda.b #$FF
    sta.w !save_block_groups_w,y
    sta.w !save_block_groups_w+1,y
    sta.w !save_block_groups_w+2,y
    sta.w !save_block_groups_w+3,y
    lda.b #$CC
    sta.w !save_block_sums_w,x
    jmp.w .block

; @routine 07:B085 SaveValidate
; WHAT IT DOES: Validates the whole save area: the three tables, blocks
; 0-11 and 12-15, then the two record sets, resetting whatever fails.
; HOW: SaveTablesCheck, SaveBlocksValidate, SaveBlocksCheck,
; SaveRecordsCheckB and SaveRecordsCheckA in turn; P is saved around them.
; Entry: any widths, D = $0000; DB = $70. Exit: A = $02 and the low byte
; SaveRecordsCheckA leaves; X = 20, Y = 4; P unchanged.
%at($07B085)
SaveValidate:
    php
    jsr.w SaveTablesCheck
    jsr.w SaveBlocksValidate
    jsr.w SaveBlocksCheck
    jsr.w SaveRecordsCheckB
    jsr.w SaveRecordsCheckA
    plp
    rtl
