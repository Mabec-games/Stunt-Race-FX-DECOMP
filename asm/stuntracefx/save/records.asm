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
SaveBlocksValidate_block:
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
SaveBlocksValidate_entry:
    lda.w !save_blocks_w,x
    pha
    cmp.b #$FF
    beq SaveBlocksValidate_first
    and.b #$7F
    cmp.b #$64
    bcc SaveBlocksValidate_first
    pla
    bra SaveBlocksValidate_reset
SaveBlocksValidate_first:
    pla
    clc
    adc.b !save_sum
    sta.b !save_sum
    lda.w !save_blocks_w+1,x
    pha
    cmp.b #$FF
    beq SaveBlocksValidate_second
    and.b #$1F
    cmp.b #$1E
    bcc SaveBlocksValidate_high
    pla
    bra SaveBlocksValidate_reset
SaveBlocksValidate_high:
    pla
    pha
    and.b #$E0
    cmp.b #$20
    bcs SaveBlocksValidate_second
    pla
    bra SaveBlocksValidate_reset
SaveBlocksValidate_second:
    pla
    clc
    adc.b !save_sum
    sta.b !save_sum
    lda.w !save_blocks_w+2,x
    pha
    cmp.b #$FF
    beq SaveBlocksValidate_third
    and.b #$1F
    cmp.b #$1C
    bcc SaveBlocksValidate_third
    pla
    bra SaveBlocksValidate_reset
SaveBlocksValidate_third:
    pla
    clc
    adc.b !save_sum
    sta.b !save_sum
    lda.w !save_blocks_w+3,x
    pha
    cmp.b #$FF
    beq SaveBlocksValidate_fourth
    and.b #$03
    cmp.b #$38
    bcc SaveBlocksValidate_fourth

; @routine 07:B2B8 SaveBlocksFourthBad
; WHAT IT DOES: SaveBlocksValidate's failure of an entry's fourth byte,
; which cannot happen (its low two bits are never $38 or more): the block
; $F3 is reset as for any failure and checked again, then the rest.
; HOW: Pulls the byte the test pushed, then as SaveBlocksValidate's reset.
; Entry: M 8-bit, X 16-bit, D = $0000; DB = $70; $F3 = the block, $F5 its
; offset; the entry byte on the stack above the return address. Exit: as
; SaveBlocksValidate.
SaveBlocksFourthBad:
    pla
    bra SaveBlocksValidate_reset
SaveBlocksValidate_fourth:
    pla
    clc
    adc.b !save_sum
    sta.b !save_sum
    inx
    inx
    inx
    inx
    dey
    bne SaveBlocksValidate_entry
    ldx.b !save_block
    lda.w !save_block_sums_w,x
    cmp.b !save_sum
    bne SaveBlocksValidate_reset
    inc.b !save_block
    lda.b !save_block
    cmp.b #$0C
    bcs SaveBlocksValidate_done
    jmp.w SaveBlocksValidate_block
SaveBlocksValidate_done:
    rts
SaveBlocksValidate_reset:
    pha
    lda.b #$11
    sta.l !save_bad_flag
    pla
    rep.b #$20
    ldx.b !save_block_offset
    sep.b #$20
    ldy.w #$0030
    lda.b #$FF
SaveBlocksValidate_fill:
    sta.w !save_blocks_w,x
    inx
    dey
    bne SaveBlocksValidate_fill
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
    jmp.w SaveBlocksValidate_block

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

; @routine 03:EFD2 SaveUnlocksCheck
; WHAT IT DOES: Checks the unlock bytes of the save ($70:FC06-!save_unlocks+9, the
; courses and classes opened) against their checksum at $70:FC14; when it
; differs, clears them and the checksum word (!save_unlocks_sum-!save_unlocks_sum+1). Then validates
; the whole save (SaveValidate).
; HOW: Data bank $70; the checksum is an 8-bit add with carry over the ten
; bytes in the order !save_unlocks+5, !save_unlocks+9, !save_unlocks, !save_unlocks+1, !save_unlocks+6-!save_unlocks+8, !save_unlocks+2-!save_unlocks+4,
; the carry cleared only before the first. P and the data bank are saved
; and restored.
; Entry: any widths, D = $0000. Exit: as SaveValidate (A = $02 and the
; low byte SaveRecordsCheckA leaves; X = 20, Y = 4); P and the data bank
; unchanged.
%at($03EFD2)
SaveUnlocksCheck:
    php
    sep.b #$30
    phb
    lda.b #$70
    pha
    plb
    sep.b #$20
    clc
    lda.w !save_unlocks+5
    adc.w !save_unlocks+9
    adc.w !save_unlocks
    adc.w !save_unlocks+1
    adc.w !save_unlocks+6
    adc.w !save_unlocks+7
    adc.w !save_unlocks+8
    adc.w !save_unlocks+2
    adc.w !save_unlocks+3
    adc.w !save_unlocks+4
    cmp.w !save_unlocks_sum
    beq .valid
    stz.w !save_unlocks+5
    stz.w !save_unlocks+9
    stz.w !save_unlocks
    stz.w !save_unlocks+1
    stz.w !save_unlocks+6
    stz.w !save_unlocks+7
    stz.w !save_unlocks+8
    stz.w !save_unlocks+2
    stz.w !save_unlocks+3
    stz.w !save_unlocks+4
    stz.w !save_unlocks_sum
    stz.w !save_unlocks_sum+1
.valid:
    jsl.l SaveValidate
    plb
    plp
    rtl

; @routine 07:B503 RecordsUnpackCourseTotals
; WHAT IT DOES: Unpacks course $0D4A's records (RecordsUnpackCourse) and sums
; each group of four times (TimeTotalsSum).
; HOW: Two calls. Not reached in the traces so far.
; Entry: as RecordsUnpackCourse. Exit: as TimeTotalsSum.
%at($07B503)
RecordsUnpackCourseTotals:
    jsr RecordsUnpackCourse
    jsr TimeTotalsSum
    rtl

; @routine 07:B50A RecordsUnpackCourseRunning
; WHAT IT DOES: Unpacks course $0D4A's records (RecordsUnpackCourse) and
; makes each group of four times running totals (TimeRunningTotals).
; HOW: Two calls. Not reached in the traces so far.
; Entry: as RecordsUnpackCourse. Exit: as TimeRunningTotals.
%at($07B50A)
RecordsUnpackCourseRunning:
    jsr RecordsUnpackCourse
    jsr TimeRunningTotals
    rtl

; @routine 07:B518 RecordsUnpackCourse
; WHAT IT DOES: Unpacks the records of course $0D4A for display: its twelve
; packed times (the words at $70:FC88 + course * 24) each into three BCD
; bytes (hundredths, seconds, minutes) from $0F76 on (TimeUnpackBcd), then
; its nine name bytes ($70:FC6D + course * 9) to $10CF-$10D7.
; HOW: The table offset is kept in direct page $F5; the time stores are
; 8-bit (X and Y truncated by SEP #$30). P saved and restored.
; Entry: DB a bank where $0D4A is work RAM, D = $0000. Exit: A low byte =
; the last name byte; X = course * 9 (8-bit); Y as the last TimeUnpackBcd
; left it, truncated; P unchanged.
%at($07B518)
RecordsUnpackCourse:
    php
    rep.b #$30
    lda.w !wram_0D4A
    asl a
    clc
    adc.w !wram_0D4A
    and.w #$00FF
    asl a
    asl a
    asl a
    tax
    stx.b !save_block_offset
    lda.l !save_course_times,x
    jsr.w TimeUnpackBcd
    sep.b #$30
    sty.w !record_times
    stx.w !record_times+1
    sta.w !record_times+2
    rep.b #$30
    ldx.b !save_block_offset
    lda.l !save_course_times+2,x
    jsr.w TimeUnpackBcd
    sep.b #$30
    sty.w !record_times+3
    stx.w !record_times+4
    sta.w !record_times+5
    rep.b #$30
    ldx.b !save_block_offset
    lda.l !save_course_times+4,x
    jsr.w TimeUnpackBcd
    sep.b #$30
    sty.w !record_times+6
    stx.w !record_times+7
    sta.w !record_times+8
    rep.b #$30
    ldx.b !save_block_offset
    lda.l !save_course_times+6,x
    jsr.w TimeUnpackBcd
    sep.b #$30
    sty.w !record_times+9
    stx.w !record_times+10
    sta.w !record_times+11
    rep.b #$30
    ldx.b !save_block_offset
    lda.l !save_course_times+8,x
    jsr.w TimeUnpackBcd
    sep.b #$30
    sty.w !record_times+12
    stx.w !record_times+13
    sta.w !record_times+14
    rep.b #$30
    ldx.b !save_block_offset
    lda.l !save_course_times+10,x
    jsr.w TimeUnpackBcd
    sep.b #$30
    sty.w !record_times+15
    stx.w !record_times+16
    sta.w !record_times+17
    rep.b #$30
    ldx.b !save_block_offset
    lda.l !save_course_times+12,x
    jsr.w TimeUnpackBcd
    sep.b #$30
    sty.w !record_times+18
    stx.w !record_times+19
    sta.w !record_times+20
    rep.b #$30
    ldx.b !save_block_offset
    lda.l !save_course_times+14,x
    jsr.w TimeUnpackBcd
    sep.b #$30
    sty.w !record_times+21
    stx.w !record_times+22
    sta.w !record_times+23
    rep.b #$30
    ldx.b !save_block_offset
    lda.l !save_course_times+16,x
    jsr.w TimeUnpackBcd
    sep.b #$30
    sty.w !record_times+24
    stx.w !record_times+25
    sta.w !record_times+26
    rep.b #$30
    ldx.b !save_block_offset
    lda.l !save_course_times+18,x
    jsr.w TimeUnpackBcd
    sep.b #$30
    sty.w !record_times+27
    stx.w !record_times+28
    sta.w !record_times+29
    rep.b #$30
    ldx.b !save_block_offset
    lda.l !save_course_times+20,x
    jsr.w TimeUnpackBcd
    sep.b #$30
    sty.w !record_times+30
    stx.w !record_times+31
    sta.w !record_times+32
    rep.b #$30
    ldx.b !save_block_offset
    lda.l !save_course_times+22,x
    jsr.w TimeUnpackBcd
    sep.b #$30
    sty.w !record_times+33
    stx.w !record_times+34
    sta.w !record_times+35
    sep.b #$30
    lda.w !wram_0D4A
    asl a
    asl a
    asl a
    clc
    adc.w !wram_0D4A
    tax
    lda.l !save_course_names,x
    sta.w !wram_10CD+2
    lda.l !save_course_names+1,x
    sta.w !wram_10CD+3
    lda.l !save_course_names+2,x
    sta.w !wram_10CD+4
    lda.l !save_course_names+3,x
    sta.w !wram_10CD+5
    lda.l !save_course_names+4,x
    sta.w !wram_10CD+6
    lda.l !save_course_names+5,x
    sta.w !wram_10CD+7
    lda.l !save_course_names+6,x
    sta.w !wram_10CD+8
    lda.l !save_course_names+7,x
    sta.w !wram_10CD+9
    lda.l !save_course_names+8,x
    sta.w !wram_10CD+10
    plp
    rts

; @routine 02:EFE2 RecordRowsGroupFirsts
; WHAT IT DOES: Draws the three records rows, each with the first time of its
; group (offsets 0, $0C, $18).
; HOW: RecordRow1Time0, RecordRow2Time4, RecordRow3Time8.
; Entry: any widths. Exit: as RecordRow3.
%at($02EFE2)
RecordRowsGroupFirsts:
    jsr RecordRow1Time0
    jsr RecordRow2Time4
    jsr RecordRow3Time8
    rts

; @routine 02:EFEC RecordRowsFirstTimes
; WHAT IT DOES: Draws the three records rows with times 0, 1 and 2 (offsets
; 0, 3, 6).
; HOW: Sets $F2 before each of RecordRow1, RecordRow2, RecordRow3.
; Entry: any widths. Exit: as RecordRow3.
%at($02EFEC)
RecordRowsFirstTimes:
    sep.b #$20
    rep.b #$10
    lda.b #$00
    sta.b !record_row_time
    jsr RecordRow1
    sep.b #$20
    rep.b #$10
    lda.b #$03
    sta.b !record_row_time
    jsr RecordRow2
    sep.b #$20
    rep.b #$10
    lda.b #$06
    sta.b !record_row_time
    jsr RecordRow3
    rts

; @routine 02:F00E RecordRow1Time0
; WHAT IT DOES: RecordRow1 with time offset $00.
; HOW: Sets $F2 and falls into RecordRow1.
; Entry: any widths. Exit: as RecordRow1.
%at($02F00E)
RecordRow1Time0:
    sep.b #$20
    rep.b #$10
    lda.b #$00
    sta.b !record_row_time

; @routine 02:F016 RecordRow1
; WHAT IT DOES: Draws records row 1 (RecordRowDraw) with name entry 1, at
; screen Y = $7C, from the sprite in $0D75.
; HOW: The name entry to $F3-$F5, the screen Y to $10BF.
; Entry: M 8-bit, X 16-bit; $F2 = the time's offset. Exit: as RecordRowDraw.
RecordRow1:
    lda.w !record_names+0
    sta.b !record_row_name
    lda.w !record_names+1
    sta.b !record_row_name+1
    lda.w !record_names+2
    sta.b !record_row_name+2
    lda.b #$7C
    sta.w !sprite_piece_y
    ldy.w !record_row_sprites+0
    jsr RecordRowDraw
    rts

; @routine 02:F031 RecordRow2Time4
; WHAT IT DOES: RecordRow2 with time offset $0C.
; HOW: Sets $F2 and falls into RecordRow2.
; Entry: any widths. Exit: as RecordRow2.
%at($02F031)
RecordRow2Time4:
    sep.b #$20
    rep.b #$10
    lda.b #$0C
    sta.b !record_row_time

; @routine 02:F039 RecordRow2
; WHAT IT DOES: Draws records row 2 (RecordRowDraw) with name entry 2, at
; screen Y = $8D, from the sprite in $0D77.
; HOW: The name entry to $F3-$F5, the screen Y to $10BF.
; Entry: M 8-bit, X 16-bit; $F2 = the time's offset. Exit: as RecordRowDraw.
RecordRow2:
    lda.w !record_names+3
    sta.b !record_row_name
    lda.w !record_names+4
    sta.b !record_row_name+1
    lda.w !record_names+5
    sta.b !record_row_name+2
    lda.b #$8D
    sta.w !sprite_piece_y
    ldy.w !record_row_sprites+2
    jsr RecordRowDraw
    rts

; @routine 02:F054 RecordRow3Time8
; WHAT IT DOES: RecordRow3 with time offset $18.
; HOW: Sets $F2 and falls into RecordRow3.
; Entry: any widths. Exit: as RecordRow3.
%at($02F054)
RecordRow3Time8:
    sep.b #$20
    rep.b #$10
    lda.b #$18
    sta.b !record_row_time

; @routine 02:F05C RecordRow3
; WHAT IT DOES: Draws records row 3 (RecordRowDraw) with name entry 3, at
; screen Y = $9E, from the sprite in $0D79.
; HOW: The name entry to $F3-$F5, the screen Y to $10BF.
; Entry: M 8-bit, X 16-bit; $F2 = the time's offset. Exit: as RecordRowDraw.
RecordRow3:
    lda.w !record_names+6
    sta.b !record_row_name
    lda.w !record_names+7
    sta.b !record_row_name+1
    lda.w !record_names+8
    sta.b !record_row_name+2
    lda.b #$9E
    sta.w !sprite_piece_y
    ldy.w !record_row_sprites+4
    jsr RecordRowDraw
    rts

; @routine 02:F077 RecordRowDraw
; WHAT IT DOES: Draws one records row from sprite Y: the time at $0F76 + ($F7
; + $F2) as six digit sprites (minutes, seconds, hundredths, a digit a
; sprite), the car (piece set (name byte 3 * 3) & 15 at X = $57) and the
; three letters of the name; a time with no seconds ($FF) hides the row's
; first three sprites instead (Y = $F0).
; HOW: Digit nibbles to the sprites' tile bytes; SpritePieces3 for the car
; with palette 0; RecordNameLetters four bytes on. All but the no-time path
; are not reached in the traces so far.
; Entry: Y = sprite number. Exit: no time: A low byte = $F0, M 8-bit, X =
; the time's offset, Y = sprite * 4; otherwise as RecordNameLetters, X as
; SpritePieces3 left it, Y = sprite * 4 + 4.
%at($02F077)
RecordRowDraw:
    rep.b #$30
    lda.b !record_page
    clc
    adc.b !record_row_time
    and.w #$00FF
    tax
    tya
    asl a
    asl a
    tay
    sep.b #$20
    lda.w !record_times+1,x
    cmp.b #$FF
    beq .none
    lda.w !record_times,x
    lsr a
    lsr a
    lsr a
    lsr a
    sta.w !oam_buffer+38,y
    lda.w !record_times,x
    and.b #$0F
    sta.w !oam_buffer+42,y
    lda.w !record_times+1,x
    lsr a
    lsr a
    lsr a
    lsr a
    sta.w !oam_buffer+26,y
    lda.w !record_times+1,x
    and.b #$0F
    sta.w !oam_buffer+30,y
    lda.w !record_times+2,x
    lsr a
    lsr a
    lsr a
    lsr a
    sta.w !oam_buffer+14,y
    lda.w !record_times+2,x
    and.b #$0F
    sta.w !oam_buffer+18,y
    rep.b #$20
    lda.b !record_row_name+2
    asl a
    clc
    adc.b !record_row_name+2
    and.w #$000F
    sta.w !sprite_piece_frame
    lda.w #$0057
    sta.w !sprite_piece_x
    stz.w !sprite_piece_palette
    phy
    jsl.l SpritePieces3
    ply
    iny
    iny
    iny
    iny
    rep.b #$20
    lda.b !record_row_name
    jsr RecordNameLetters
    rts
.none:
    lda.b #$F0
    sta.w !oam_buffer+1,y
    sta.w !oam_buffer+5,y
    sta.w !oam_buffer+9,y
    rts

; @routine 02:FF01 RecordNameLetters
; WHAT IT DOES: Puts the three 5-bit letters of A (bits 0-4, 5-9, 10-14) plus
; $0A into the tile bytes of sprites Y / 4, + 1 and + 2.
; HOW: The letters are kept on the stack between them. Not reached in the
; traces so far.
; Entry: M 16-bit, X 16-bit; A = the letters, Y = sprite buffer offset.
; Exit: A = the third tile; M 16-bit; N, Z, C, V clear (the last add).
%at($02FF01)
RecordNameLetters:
    pha
    and.w #$001F
    clc
    adc.w #$000A
    sep.b #$20
    sta.w !oam_buffer+42,y
    rep.b #$20
    pla
    pha
    asl a
    asl a
    asl a
    xba
    and.w #$001F
    clc
    adc.w #$000A
    sep.b #$20
    sta.w !oam_buffer+46,y
    rep.b #$20
    pla
    lsr a
    lsr a
    xba
    and.w #$001F
    clc
    adc.w #$000A
    sep.b #$20
    sta.w !oam_buffer+50,y
    rep.b #$20
    rts

; @routine 07:D6E8 RecordTimeCompare
; WHAT IT DOES: Compares record time X (BCD minutes, seconds, hundredths at
; $0F5B + X, unpacked to the five digits at $0F64-$0F68) with the best time's
; digits at $0F69, most significant first: $0FA3 = $FF when it is better, 1
; when worse or when there is no best time ($0F2B = $FFFF), 0 when equal;
; then sound $35 on port 0 unless $0E57 is set.
; HOW: Nibbles split with AND and LSR (the byte kept on the stack); an 8-bit
; compare per digit. The no-best, worse and equal results are not reached in
; the traces so far.
; Entry: M 16-bit, X 16-bit; X = the time's offset. Exit: M 8-bit; X = the
; digit the compare stopped at (or X unchanged with no best time); A low
; byte = $0E57's value, or as SoundPort0QueuePush leaves it.
%at($07D6E8)
RecordTimeCompare:
    lda.w !wram_0F2B
    cmp.w #$FFFF
    beq .worse
    sep.b #$20
    lda.w !record_time_bcd,x
    pha
    and.b #$0F
    sta.w !lap_record_a+4
    pla
    lsr a
    lsr a
    lsr a
    lsr a
    sta.w !lap_record_a+3
    lda.w !record_time_bcd+1,x
    pha
    and.b #$0F
    sta.w !lap_record_a+2
    pla
    lsr a
    lsr a
    lsr a
    lsr a
    sta.w !lap_record_a+1
    lda.w !record_time_bcd+2,x
    and.b #$0F
    sta.w !lap_record_a
    ldx.w #$0000
.digit:
    lda.w !lap_record_a,x
    cmp.w !lap_digits,x
    bcc .better
    bne .worse
    inx
    cpx.w #$0005
    bcc .digit
    sep.b #$20
    lda.b #$00
    sta.w !p1_lead
    lda.w !wram_0E57
    bne .doneE
    lda.b #$35
    jsl.l SoundPort0QueuePush
.doneE:
    rts
.better:
    sep.b #$20
    lda.b #$FF
    sta.w !p1_lead
    lda.w !wram_0E57
    bne .doneB
    lda.b #$35
    jsl.l SoundPort0QueuePush
.doneB:
    rts
.worse:
    sep.b #$20
    lda.b #$01
    sta.w !p1_lead
    lda.w !wram_0E57
    bne .doneW
    lda.b #$35
    jsl.l SoundPort0QueuePush
.doneW:
    rts

; @routine 07:D62C RecordCompareTime0
; WHAT IT DOES: By mode $0D2B: 0 plays sound $35, 2 PlayerSound35, others
; compare record time 0 with the best (RecordTimeCompare).
; HOW: 16-bit mode test.
; Entry: any widths. Exit: as the call made; P restored.
%at($07D62C)
RecordCompareTime0:
    php
    rep.b #$30
    lda.w !hud_mode
    beq .sound
    cmp.w #$0002
    beq .mode2
    ldx.w #$0000
    jsr.w RecordTimeCompare
    plp
    rtl
.mode2:
    jsr.w PlayerSound35
    plp
    rtl
.sound:
    lda.w #$0035
    jsl.l SoundPort0QueuePush
    plp
    rtl

; @routine 07:D64F RecordCompareTime1
; WHAT IT DOES: As RecordCompareTime0 for record time 1 (offset 3).
; HOW: As RecordCompareTime0. The mode 0 sound is not reached in the
; traces so far.
; Entry: any widths. Exit: as RecordCompareTime0.
%at($07D64F)
RecordCompareTime1:
    php
    rep.b #$30
    lda.w !hud_mode
    beq .sound
    cmp.w #$0002
    beq .mode2
    ldx.w #$0003
    jsr.w RecordTimeCompare
    plp
    rtl
.mode2:
    jsr.w PlayerSound35
    plp
    rtl
.sound:
    lda.w #$0035
    jsl.l SoundPort0QueuePush
    plp
    rtl

; @routine 07:B31B RecordsLoadSpecialCourseLong
; WHAT IT DOES: RecordsLoadSpecialCourse for a JSL caller
; (StuntPanelWait): with data bank $07, P and the data bank kept.
; HOW: PHP, SEP #$20, PHB, PHK, PLB, JSR; then PLB, PLP, RTL.
; Entry: any widths; D = $0000. Exit: P and the data bank unchanged; A,
; X, Y as RecordsLoadSpecialCourse leaves them.
%at($07B31B)
RecordsLoadSpecialCourseLong:
    php
    sep.b #$20
    phb
    phk
    plb
    jsr.w RecordsLoadSpecialCourse
    sep.b #$20
    plb
    plp
    rtl

; @routine 07:B329 RecordsLoadSpecialCourse
; WHAT IT DOES: For courses $0C-$0F ($0E3D): loads the course's twelve
; saved entries ($70:FCD0 + course * 48, four bytes each) into the times
; at $0F2B and the words at $0F43, with the tags from the table at $B4EB
; (data bank) at $0F13; sorts them so a larger time never follows a
; smaller one, the three arrays together; then $10C9 = the place (1-3) of
; tag $10C7 among the first three, or 0.
; HOW: Direct page $F3 holds the course for course * 3; an insertion pass
; from the end. The swaps and places other than 1 are not reached in the
; traces so far.
; Entry: any widths; D = $0000. Exit: M, X 16-bit; A = the place; X =
; $FFFE; Y = $18; or, for other courses, A = the course.
%at($07B329)
RecordsLoadSpecialCourse:
    rep.b #$30
    lda.w !course
    and.w #$00FF
    sta.b !dp_f3
    cmp.w #$000C
    bcc .done
    cmp.w #$0010
    bcc .load
.done:
    rts
.load:
    asl a
    clc
    adc.b !dp_f3
    asl a
    asl a
    asl a
    asl a
    tax
    ldy.w #$0000
.copy:
    lda.l !special_saves,x
    sta.w !special_times,y
    lda.l !special_saves+2,x
    sta.w !special_words,y
    lda.w !special_tag_table,y
    sta.w !special_tags,y
    inx
    inx
    inx
    inx
    iny
    iny
    cpy.w #$0018
    bcc .copy
    ldx.w #$0014
.outer:
    phx
.inner:
    lda.w !special_times+2,x
    cmp.w !special_times,x
    bcc .next
    beq .next
    pha
    lda.w !special_times,x
    sta.w !special_times+2,x
    pla
    sta.w !special_times,x
    lda.w !special_tags,x
    pha
    lda.w !special_tags+2,x
    sta.w !special_tags,x
    pla
    sta.w !special_tags+2,x
    lda.w !special_words,x
    pha
    lda.w !special_words+2,x
    sta.w !special_words,x
    pla
    sta.w !special_words+2,x
    cpx.w #$0014
    bcs .next
    inx
    inx
    bra .inner
.next:
    plx
    dex
    dex
    bpl .outer
    lda.w !player_tag
    cmp.w !special_tags
    beq .first
    cmp.w !special_tags+2
    beq .second
    cmp.w !special_tags+4
    beq .third
    lda.w #$0000
    bra .place
.third:
    lda.w #$0003
    bra .place
.second:
    lda.w #$0002
    bra .place
.first:
    lda.w #$0001
.place:
    sta.w !player_place
    rts

; @routine 07:B3D4 RecordsLoadForCourse
; WHAT IT DOES: Loads course $0E3D's records: RecordsLoadSpecialCourse for
; courses $0C-$0F, RecordsLoadCourse below $0C, nothing above.
; HOW: The data bank is set to $07 (the tag table's) and restored.
; Entry: any widths; D = $0000. Exit: P and the data bank restored.
%at($07B3D4)
RecordsLoadForCourse:
    php
    sep.b #$20
    phb
    phk
    plb
    lda.w !course
    cmp.b #$0C
    bcc .normal
    cmp.b #$10
    bcs .done
    jsr.w RecordsLoadSpecialCourse
    bra .done
.normal:
    jsr.w RecordsLoadCourse
.done:
    sep.b #$20
    plb
    plp
    rtl

; @routine 07:B3F2 RecordsLoadCourseLong
; WHAT IT DOES: RecordsLoadCourse for callers in other banks, with the
; data bank $07.
; HOW: As RecordsLoadForCourse. Not reached in the traces so far.
; Entry: any widths; D = $0000. Exit: P and the data bank restored.
%at($07B3F2)
RecordsLoadCourseLong:
    php
    sep.b #$20
    phb
    phk
    plb
    jsr.w RecordsLoadCourse
    sep.b #$20
    plb
    plp
    rtl

; @routine 07:B400 RecordsLoadCourse
; WHAT IT DOES: For a normal course (below $0C): loads its twelve saved
; entries and sorts them, smallest time first (as RecordsLoadSpecialCourse
; otherwise); for any course finds the player's place ($10C9); then
; unpacks the course's two best times ($70:FFD0 + course * 4) and the
; first time to BCD at $0F5B, $0F5E and $0F61 (TimeUnpackBcd).
; HOW: Direct page $F3 holds the course. The swaps and places other than
; 1 are not reached in the traces so far.
; Entry: any widths; D = $0000. Exit: M 8-bit, X 8-bit; A low byte = the
; last minutes; X, Y the last seconds and hundredths.
%at($07B400)
RecordsLoadCourse:
    rep.b #$30
    lda.w !course
    and.w #$00FF
    sta.b !dp_f3
    cmp.w #$000C
    bcs .place
    asl a
    clc
    adc.b !dp_f3
    asl a
    asl a
    asl a
    asl a
    tax
    ldy.w #$0000
.copy:
    lda.l !special_saves,x
    sta.w !special_times,y
    lda.l !special_saves+2,x
    sta.w !special_words,y
    lda.w !special_tag_table,y
    sta.w !special_tags,y
    inx
    inx
    inx
    inx
    iny
    iny
    cpy.w #$0018
    bcc .copy
    ldx.w #$0014
.outer:
    phx
.inner:
    lda.w !special_times+2,x
    cmp.w !special_times,x
    bcs .next
    pha
    lda.w !special_times,x
    sta.w !special_times+2,x
    pla
    sta.w !special_times,x
    lda.w !special_tags,x
    pha
    lda.w !special_tags+2,x
    sta.w !special_tags,x
    pla
    sta.w !special_tags+2,x
    lda.w !special_words,x
    pha
    lda.w !special_words+2,x
    sta.w !special_words,x
    pla
    sta.w !special_words+2,x
    cpx.w #$0014
    bcs .next
    inx
    inx
    bra .inner
.next:
    plx
    dex
    dex
    bpl .outer
.place:
    lda.w !player_tag
    cmp.w !special_tags
    beq .first
    cmp.w !special_tags+2
    beq .second
    cmp.w !special_tags+4
    beq .third
    lda.w #$0000
    bra .placed
.third:
    lda.w #$0003
    bra .placed
.second:
    lda.w #$0002
    bra .placed
.first:
    lda.w #$0001
.placed:
    sta.w !player_place
    lda.w !course
    and.w #$00FF
    asl a
    asl a
    tax
    phx
    rep.b #$30
    lda.l !best_times,x
    jsr.w TimeUnpackBcd
    sep.b #$30
    sty.w !record_time_bcd+0
    stx.w !record_time_bcd+1
    sta.w !record_time_bcd+2
    rep.b #$30
    plx
    lda.l !best_times+2,x
    jsr.w TimeUnpackBcd
    sep.b #$30
    sty.w !record_time_bcd+3
    stx.w !record_time_bcd+4
    sta.w !record_time_bcd+5
    rep.b #$30
    lda.w !special_times
    jsr.w TimeUnpackBcd
    sep.b #$30
    sty.w !record_time_bcd+6
    stx.w !record_time_bcd+7
    sta.w !record_time_bcd+8
    sep.b #$20
    rts

; @routine 07:AEF3 SpecialRecordInsert
; WHAT IT DOES: Puts time A into a special course's three best times for
; car $0D89, at $70:FCD0 + course * 48 + car * 12 (each entry the time and
; the player's level $0F11): a time below the first moves the others down
; (the third lost) and tags the car's first place (RecordTagCar0), then,
; unless the time is still above another of the course's entries, keeps
; the lap's packed times (TimePackBcd of $0EF0-$0EF2 and $0EF3-$0EF5) at
; $70:FFD0 + course * 4; below the second or third likewise (RecordTagCar4,
; RecordTagCar8); no place: the tag $10C7 = $FFFF. Then the course's check
; sum (SpecialRecordsChecksum). First, with HUD mode $0D2B at 0, the word at
; $0F6E + $0D48 * 2 is set to it. Course $0C or more, or car 4 or more:
; nothing else.
; HOW: $F1, $F3, $F5 and $F7 (direct page) hold the course's offset, the
; car, a loop count and the entry's offset ($0D14 keeps the course's).
; Entry: M 16-bit, X 16-bit; A = the time. Exit: as SpecialRecordsChecksum
; leaves it; or A = $FFFF; or, for no special course or car, A = the time
; and N, Z from it.
%at($07AEF3)
SpecialRecordInsert:
    pha
    lda.w !hud_mode
    bne .lAF04
    lda.w !wram_0D48
    asl a
    and.w #$00FF
    tax
    sta.w !lap_digits+5,x
.lAF04:
    lda.w !course
    and.w #$00FF
    cmp.w #$000C
    bcc .lAF11
.lAF0F:
    pla
    rts
.lAF11:
    sta.b !digit_count
    asl a
    clc
    adc.b !digit_count
    asl a
    asl a
    asl a
    asl a
    sta.b !digit_count
    sta.w !wram_0D12+2
    tay
    lda.w !wram_0D89
    and.w #$00FF
    cmp.w #$0004
    bcs .lAF0F
    sta.b !dp_f3
    asl a
    clc
    adc.b !dp_f3
    asl a
    asl a
    clc
    adc.b !digit_count
    tax
    sta.b !record_page
    pla
    cmp.l !special_saves,x
    bcc .lAF44
    jmp.w .lAFC0
.lAF44:
    pha
    jsr.w RecordTagCar0
    lda.l !special_saves+4,x
    sta.l !special_saves+8,x
    lda.l !special_saves+6,x
    sta.l !special_saves+10,x
    lda.l !special_saves,x
    sta.l !special_saves+4,x
    lda.l !special_saves+2,x
    sta.l !special_saves+6,x
    lda.w !level_columns+12
    sta.l !special_saves+2,x
    pla
    sta.l !special_saves,x
    stx.b !digit_count
    ldx.w #$000C
    stx.b !save_block_offset
    tyx
.lAF7C:
    cpx.b !digit_count
    beq .lAF89
    cmp.l !special_saves,x
    bcc .lAF89
    jmp.w SpecialRecordsChecksum
.lAF89:
    inx
    inx
    inx
    inx
    dec.b !save_block_offset
    bne .lAF7C
    lda.w !lap_times
    ldx.w !lap_times+1
    ldy.w !lap_times+2
    jsr.w TimePackBcd
    pha
    lda.w !lap_times+3
    ldx.w !lap_times+4
    ldy.w !lap_times+5
    jsr.w TimePackBcd
    pha
    lda.w !course
    and.w #$00FF
    asl a
    asl a
    tax
    pla
    sta.l !save_block_groups+2,x
    pla
    sta.l !save_block_groups,x
    bra SpecialRecordsChecksum
.lAFC0:
    cmp.l !special_saves+4,x
    bcs .lAFE8
    pha
    jsr.w RecordTagCar4
    lda.l !special_saves+4,x
    sta.l !special_saves+8,x
    lda.l !special_saves+6,x
    sta.l !special_saves+10,x
    pla
    sta.l !special_saves+4,x
    lda.w !level_columns+12
    sta.l !special_saves+6,x
    bra SpecialRecordsChecksum
.lAFE8:
    cmp.l !special_saves+8,x
    bcs SpecialRecordInsert_slow
    sta.l !special_saves+8,x
    jsr.w RecordTagCar8
    lda.w !level_columns+12
    sta.l !special_saves+10,x

; @routine 07:AFFC SpecialRecordsChecksum
; WHAT IT DOES: The check sum of a special course's records: the bytes of
; its four packed lap times at $70:FFD0 + course * 4 (0 for course $0C or
; more) and its 48 bytes from $70:FCD0 + $0D14, summed with carries, at
; $70:FC33 + course.
; HOW: 8-bit sums through $F1 (direct page), X 8-bit for the store.
; Entry: M 16-bit, X 16-bit; $0D14 = the course's offset. Exit: A = $00 and
; the low byte of the sum; X = the course, Y = 0; M 16-bit, X 16-bit; N, Z
; from the last count; C, V from the last add.
%at($07AFFC)
SpecialRecordsChecksum:
    lda.w !course
    and.w #$00FF
    cmp.w #$000C
    bcs .lB02D
    asl a
    asl a
    tax
    sep.b #$20
    lda.l !save_block_groups,x
    sta.b !digit_count
    lda.l !save_block_groups+1,x
    clc
    adc.b !digit_count
    sta.b !digit_count
    lda.l !save_block_groups+2,x
    clc
    adc.b !digit_count
    sta.b !digit_count
    lda.l !save_block_groups+3,x
    clc
    adc.b !digit_count
    bra .lB031
.lB02D:
    sep.b #$20
    lda.b #$00
.lB031:
    sta.b !digit_count
    ldy.w #$0030
    ldx.w !wram_0D12+2
    clc
.lB03A:
    lda.l !special_saves,x
    clc
    adc.b !digit_count
    sta.b !digit_count
    inx
    dey
    bne .lB03A
    sep.b #$10
    ldx.w !course
    lda.b !digit_count
    sta.l !save_table_sums+6,x
    rep.b #$30
    rts
SpecialRecordInsert_slow:
    rep.b #$30
    lda.w #$FFFF
    sta.w !player_tag
    rts

; @routine 07:B077 RecordCarTimes12
; WHAT IT DOES: A = $0D89 * 12, low byte only.
; HOW: ASL, CLC, ADC $0D89, ASL, ASL, AND #$FF (16-bit); through the data
; bank.
; Entry: M 16-bit. Exit: A = the value; N, Z from it; C from the last
; shift; V from the add.
%at($07B077)
RecordCarTimes12:
    lda.w !wram_0D89
    asl a
    clc
    adc.w !wram_0D89
    asl a
    asl a
    and.w #$00FF
    rts

; @routine 07:B05E RecordTagCar0
; WHAT IT DOES: Sets the tag $10C7 to $0D89 * 12 (low byte), the place of
; the car's first record.
; HOW: RecordCarTimes12; the entries at $07:B063 and $07:B06C add 4 and 8.
; Entry: M 16-bit. Exit: A = the tag; N, Z from RecordCarTimes12's AND.
%at($07B05E)
RecordTagCar0:
    jsr.w RecordCarTimes12
    bra RecordTagCar_store

; @routine 07:B063 RecordTagCar4
; WHAT IT DOES: As RecordTagCar0 plus 4 (the second record).
; HOW: As RecordTagCar0 with CLC, ADC #4; no script reaches it (unit_cpu
; does).
; Entry: M 16-bit. Exit: A = the tag; N, Z, C, V from the add.
%at($07B063)
RecordTagCar4:
    jsr.w RecordCarTimes12          ; not reached in the traces so far
    clc                             ; not reached in the traces so far
    adc.w #$0004                    ; not reached in the traces so far
    bra RecordTagCar_store                     ; not reached in the traces so far

; @routine 07:B06C RecordTagCar8
; WHAT IT DOES: As RecordTagCar0 plus 8 (the third record).
; HOW: As RecordTagCar4 with 8; no script reaches it (unit_cpu does).
; Entry: M 16-bit. Exit: as RecordTagCar4.
%at($07B06C)
RecordTagCar8:
    jsr.w RecordCarTimes12          ; not reached in the traces so far
    clc                             ; not reached in the traces so far
    adc.w #$0008                    ; not reached in the traces so far
RecordTagCar_store:
    sta.w !player_tag
    rts

; @routine 07:AD1B TimeUpRecordUpdate
; WHAT IT DOES: The time-up run's end: its time ($0E66 minutes, $0E64
; seconds, $0E63 hundredths, BCD) as five digits at $0F69, then packed
; (TimePackBcd) and set against the record for the course ($0E3D - $17,
; five bytes each from $70:FC4A): lower, it becomes the record with $0F11
; and the car $0D89, $0FA3 = $FF and the sums made again
; (SaveRecordSumsTimeUp); higher, $0FA3 = 1; equal, $0FA3 = 0.
; HOW: 8-bit digits through the data bank; the compare 16-bit; the record
; by long stores; $F1 holds the course for course * 5.
; Entry: any widths; data bank $07, D = $0000. Exit: M 8-bit; X the
; record's offset (or as the sums leave it); A low byte = $0FA3's value.
%at($07AD1B)
TimeUpRecordUpdate:
    sep.b #$20
    lda.w !race_time_minutes
    and.b #$0F
    sta.w !lap_digits
    lda.w !race_time_seconds
    lsr a
    lsr a
    lsr a
    lsr a
    sta.w !lap_digits+1
    lda.w !race_time_seconds
    and.b #$0F
    sta.w !lap_digits+2
    lda.w !race_time_fraction
    lsr a
    lsr a
    lsr a
    lsr a
    sta.w !lap_digits+3
    lda.w !race_time_fraction
    and.b #$0F
    sta.w !lap_digits+4
    rep.b #$30
    lda.w !race_time_fraction
    ldx.w !race_time_seconds
    ldy.w !race_time_minutes
    jsr.w TimePackBcd
    pha
    lda.w !course
    sec
    sbc.w #$0017
    sta.b !digit_count
    asl a
    asl a
    clc
    adc.b !digit_count
    and.w #$00FF
    tax
    pla
    cmp.l !save_record_a,x
    beq .equal
    bcs .slower
    sta.l !save_record_a,x
    lda.w !level_columns+12
    sta.l !save_record_a+2,x
    sep.b #$20
    lda.w !wram_0D89
    sta.l !save_record_a+4,x
    lda.b #$FF
    sta.l !p1_lead
    jsr.w SaveRecordSumsTimeUp
    rts
.slower:
    sep.b #$20
    lda.b #$01
    sta.l !p1_lead
    rts
.equal:
    sep.b #$20
    lda.b #$00
    sta.l !p1_lead
    rts

; @routine 07:ADA3 SaveRecordSumsTimeUp
; WHAT IT DOES: The sums of the four five-byte time-up records from
; $70:FC4A (SaveRecordSumA at offsets 0, 5, 10, 15) into
; $70:FC46-$70:FC49; $F1 cleared first.
; HOW: 8-bit registers.
; Entry: any widths, D = $0000. Exit: M 8-bit, X 8-bit; A low byte = the
; last sum; X = $14, Y = 0; Z set, N clear; C, V from the last add.
%at($07ADA3)
SaveRecordSumsTimeUp:
    sep.b #$30
    stz.b !digit_count
    ldx.b #$00
    jsr.w SaveRecordSumA
    sta.l !save_table_sums+25
    ldx.b #$05
    jsr.w SaveRecordSumA
    sta.l !save_table_sums+26
    ldx.b #$0A
    jsr.w SaveRecordSumA
    sta.l !save_table_sums+27
    ldx.b #$0F
    jsr.w SaveRecordSumA
    sta.l !save_table_sums+28
    rts

; @routine 07:AE07 StuntRecordTime
; WHAT IT DOES: Stunt Trax's score against the course's three best for
; the car: the score is $19EC as the high byte and the BCD sum of $0E70
; and $19F0 as the low; for courses $0C-$0F, at $70:FCD0 + course * 48 +
; car ($0D89) * 12 (three entries of score and $0F11, best first), a
; higher score goes in at its place (tag RecordTagCar0, 4 or 8), the
; lower ones moving down, and the course's 48 bytes are summed into
; $70:FC33 + course. For any other course its RTS takes the score it
; pushed as the return address (no run reaches that: the courses here
; are $0C-$0F).
; HOW: SED for the BCD add; $F1 holds course * 48 (also $0D14), $F3 the
; car, $F7 the entry's offset; the score is kept on the stack.
; Entry: any widths; data bank $07, D = $0000. Exit: M 16-bit, X 16-bit;
; A as the last step leaves it.
%at($07AE07)
StuntRecordTime:
    sep.b #$20
    lda.w !hud_digits_19ec
    xba
    sed
    lda.w !race_time_bcd
    clc
    adc.w !wram_19F0
    cld
    rep.b #$30
    pha
    lda.w !course
    and.w #$00FF
    cmp.w #$000C
    bcc .other
    cmp.w #$0010
    bcc .stunt
.other:
    rts
.stunt:
    sta.b !digit_count
    asl a
    clc
    adc.b !digit_count
    asl a
    asl a
    asl a
    asl a
    sta.b !digit_count
    sta.w !wram_0D12+2
    tay
    lda.w !wram_0D89
    and.w #$00FF
    sta.b !dp_f3
    asl a
    clc
    adc.b !dp_f3
    asl a
    asl a
    clc
    adc.b !digit_count
    tax
    sta.b !record_page
    pla
    cmp.l !special_saves,x
    bcc .second
    beq .second
    pha
    jsr.w RecordTagCar0
    lda.l !special_saves+4,x
    sta.l !special_saves+8,x
    lda.l !special_saves+6,x
    sta.l !special_saves+10,x
    lda.l !special_saves,x
    sta.l !special_saves+4,x
    lda.l !special_saves+2,x
    sta.l !special_saves+6,x
    lda.w !level_columns+12
    sta.l !special_saves+2,x
    pla
    sta.l !special_saves,x
    jmp.w .sum
.second:
    cmp.l !special_saves+4,x
    bcc .third
    beq .third
    pha
    jsr.w RecordTagCar4
    lda.l !special_saves+4,x
    sta.l !special_saves+8,x
    lda.l !special_saves+6,x
    sta.l !special_saves+10,x
    pla
    sta.l !special_saves+4,x
    lda.w !level_columns+12
    sta.l !special_saves+6,x
    bra .sum
.third:
    cmp.l !special_saves+8,x
    bcc .done
    beq .done
    sta.l !special_saves+8,x
    jsr.w RecordTagCar8
    lda.w !level_columns+12
    sta.l !special_saves+10,x
.sum:
    sep.b #$20
    lda.b #$00
    sta.b !digit_count
    ldy.w #$0030
    ldx.b !record_page
    ldx.w !wram_0D12+2
.add:
    lda.l !special_saves,x
    clc
    adc.b !digit_count
    sta.b !digit_count
    inx
    dey
    bne .add
    sep.b #$10
    ldx.w !course
    lda.b !digit_count
    sta.l !save_table_sums+6,x
.done:
    rep.b #$30
    rts
