; lap_time.asm: race times as the HUD and the records show them. Times are
; kept packed in a word: hundredths in bits 0-6, seconds in bits 7-12,
; minutes in bits 13-15.
; C: src/stuntracefx/hud/lap_time.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 07:AC46 TimeUnpackBcd
; WHAT IT DOES: Unpacks a packed time word into BCD: A = the minutes (bits
; 13-15), X = the seconds (bits 7-12), Y = the hundredths (bits 0-6); $FFFF
; (no time) gives $FFFF in all three.
; HOW: Masks and shifts, each part through BinaryToBcd. Only the $FFFF
; case is reached in the traces so far.
; Entry: M 16-bit, X 16-bit, D = $0000; A = the packed time. Exit: A, X, Y
; as above; $F1 as BinaryToBcd leaves it; N, Z, C, V from its last add, or
; N set, Z and C set by the compare for $FFFF.
%at($07AC46)
TimeUnpackBcd:
    cmp.w #$FFFF
    beq .none
    tax                    ; not reached in the traces so far
    and.w #$007F
    jsl.l BinaryToBcd
    tay
    txa
    pha
    asl a
    xba
    and.w #$003F
    jsl.l BinaryToBcd
    tax
    pla
    rol a
    rol a
    rol a
    rol a
    and.w #$0007
    jsl.l BinaryToBcd
    rts
.none:
    lda.w #$FFFF
    ldx.w #$FFFF
    ldy.w #$FFFF
    rts

; @routine 07:ACD9 BinaryToBcd
; WHAT IT DOES: Converts A (below 10000) to binary-coded decimal.
; HOW: Counts thousands, hundreds and tens by repeated subtraction into
; direct page $F1 ($1000, $100, $10 per step), then adds the units. Not
; reached in the traces so far.
; Entry: M 16-bit, D = $0000; A = the value. Exit: A = the BCD value; $F1
; = A less its units digit; X, Y unchanged; N, Z, C, V from the last add.
%at($07ACD9)
BinaryToBcd:
    stz.b !bcd_digits
.thousands:
    cmp.w #1000
    bcc .hundreds
    sbc.w #1000
    pha
    lda.b !bcd_digits
    clc
    adc.w #$1000
    sta.b !bcd_digits
    pla
    bra .thousands
.hundreds:
    cmp.w #100
    bcc .tens
    sbc.w #100
    pha
    lda.b !bcd_digits
    clc
    adc.w #$0100
    sta.b !bcd_digits
    pla
    bra .hundreds
.tens:
    cmp.w #10
    bcc .units
    sbc.w #10
    pha
    lda.b !bcd_digits
    clc
    adc.w #$0010
    sta.b !bcd_digits
    pla
    bra .tens
.units:
    clc
    adc.b !bcd_digits
    rtl

; @routine 07:B67E TimeTotalsSum
; WHAT IT DOES: Sums each group of four record times ($0F76, $0F82, $0F8E)
; into its total ($0F9A, $0F9D, $0FA0); a group whose first time has no
; minutes ($FF) totals $FF:$FF:$FF.
; HOW: Decimal mode, $F1-$F3 as the running sum, TimeAddToTotal per time.
; Not reached in the traces so far.
; Entry: D = $0000. Exit: A = the last total's minutes; X = $21; P restored.
%at($07B67E)
TimeTotalsSum:
    php
    sep #$30
    sed
    stz.b !time_total
    stz.b !time_total+1
    stz.b !time_total+2
    ldx.b #$00
    jsr TimeAddToTotal
    ldx.b #$03
    jsr TimeAddToTotal
    ldx.b #$06
    jsr TimeAddToTotal
    ldx.b #$09
    jsr TimeAddToTotal
    lda.w !record_times+2
    cmp.b #$FF
    bne .group0
    sta.b !time_total
    sta.b !time_total+1
    sta.b !time_total+2
.group0:
    lda.b !time_total
    sta.w !record_totals+0
    lda.b !time_total+1
    sta.w !record_totals+1
    lda.b !time_total+2
    sta.w !record_totals+2
    stz.b !time_total
    stz.b !time_total+1
    stz.b !time_total+2
    ldx.b #$0C
    jsr TimeAddToTotal
    ldx.b #$0F
    jsr TimeAddToTotal
    ldx.b #$12
    jsr TimeAddToTotal
    ldx.b #$15
    jsr TimeAddToTotal
    lda.w !record_times+14
    cmp.b #$FF
    bne .group1
    sta.b !time_total
    sta.b !time_total+1
    sta.b !time_total+2
.group1:
    lda.b !time_total
    sta.w !record_totals+3
    lda.b !time_total+1
    sta.w !record_totals+4
    lda.b !time_total+2
    sta.w !record_totals+5
    stz.b !time_total
    stz.b !time_total+1
    stz.b !time_total+2
    ldx.b #$18
    jsr TimeAddToTotal
    ldx.b #$1B
    jsr TimeAddToTotal
    ldx.b #$1E
    jsr TimeAddToTotal
    ldx.b #$21
    jsr TimeAddToTotal
    lda.w !record_times+26
    cmp.b #$FF
    bne .group2
    sta.b !time_total
    sta.b !time_total+1
    sta.b !time_total+2
.group2:
    lda.b !time_total
    sta.w !record_totals+6
    lda.b !time_total+1
    sta.w !record_totals+7
    lda.b !time_total+2
    sta.w !record_totals+8
    cld
    plp
    rts

; @routine 07:B727 TimeRunningTotals
; WHAT IT DOES: Turns each group of four record times into running totals:
; the first three are added into the time after each.
; HOW: Decimal mode, TimeAddToNext for times 0-2, 4-6 and 8-10. Not reached
; in the traces so far.
; Entry: D = $0000. Exit: X = $1E; P restored.
%at($07B727)
TimeRunningTotals:
    php
    sep #$30
    sed
    ldx.b #$00
    jsr TimeAddToNext
    ldx.b #$03
    jsr TimeAddToNext
    ldx.b #$06
    jsr TimeAddToNext
    ldx.b #$0C
    jsr TimeAddToNext
    ldx.b #$0F
    jsr TimeAddToNext
    ldx.b #$12
    jsr TimeAddToNext
    ldx.b #$18
    jsr TimeAddToNext
    ldx.b #$1B
    jsr TimeAddToNext
    ldx.b #$1E
    jsr TimeAddToNext
    cld
    plp
    rts

; @routine 07:B75B TimeAddToTotal
; WHAT IT DOES: Adds record time X (BCD hundredths, seconds, minutes at
; $0F76 + X) to the total in direct page $F1-$F3, unless it is no time ($FF).
; HOW: Decimal adds (the caller sets D); seconds past 59 carry into the
; minutes. Not reached in the traces so far.
; Entry: M 8-bit, D = $0000, decimal mode; X = the time's offset.
; Exit: A = the new minutes; or $FF with Z and C set for no time.
%at($07B75B)
TimeAddToTotal:
    lda.w !record_times,x
    cmp.b #$FF
    beq .done
    clc
    adc.b !time_total
    sta.b !time_total
    lda.w !record_times+1,x
    adc.b !time_total+1
    bcc .seconds
    sec
    sbc.b #$60
    sta.b !time_total+1
    lda.b !time_total+2
    clc
    adc.b #$01
    sta.b !time_total+2
    lda.b !time_total+1
.seconds:
    sta.b !time_total+1
    cmp.b #$60
    bcc .minutes
    sbc.b #$60
    sta.b !time_total+1
    lda.b !time_total+2
    clc
    adc.b #$01
    sta.b !time_total+2
.minutes:
    clc
    lda.w !record_times+2,x
    adc.b !time_total+2
    sta.b !time_total+2
.done:
    rts

; @routine 07:B796 TimeAddToNext
; WHAT IT DOES: Adds record time X to the time after it ($0F79 + X), unless
; it is no time ($FF).
; HOW: As TimeAddToTotal. Not reached in the traces so far.
; Entry: as TimeAddToTotal. Exit: as TimeAddToTotal.
%at($07B796)
TimeAddToNext:
    lda.w !record_times,x
    cmp.b #$FF
    beq .done
    clc
    adc.w !record_times+3,x
    sta.w !record_times+3,x
    lda.w !record_times+1,x
    adc.w !record_times+4,x
    bcc .seconds
    sec
    sbc.b #$60
    sta.w !record_times+4,x
    lda.w !record_times+5,x
    clc
    adc.b #$01
    sta.w !record_times+5,x
    lda.w !record_times+4,x
.seconds:
    sta.w !record_times+4,x
    cmp.b #$60
    bcc .minutes
    sbc.b #$60
    sta.w !record_times+4,x
    lda.w !record_times+5,x
    clc
    adc.b #$01
    sta.w !record_times+5,x
.minutes:
    clc
    lda.w !record_times+2,x
    adc.w !record_times+5,x
    sta.w !record_times+5,x
.done:
    rts

; @routine 07:B93C RecordSlotCopy0
; WHAT IT DOES: Copies the course $0D4A's first three-byte record entry
; ($70:FC6D + course * 9) to the second.
; HOW: The offset as course * 8 + course, then the copy loop of
; RecordSlotCopy3 (its code from the TAX).
; Entry: M 8-bit, X 8-bit. Exit: A = the last byte copied; X = the offset
; + 3; Y = 0; Z set, N clear; C, V from the add.
%at($07B93C)
RecordSlotCopy0:
    lda.w !wram_0D4A
    asl a
    asl a
    asl a
    clc
    adc.w !wram_0D4A
    bra RecordSlotCopy3_copy

; @routine 07:B948 RecordSlotCopy3
; WHAT IT DOES: Copies the course $0D4A's second three-byte record entry
; ($70:FC70 + course * 9) to the third.
; HOW: As RecordSlotCopy0 with 3 added (the carry of the first add in).
; Entry: M 8-bit, X 8-bit. Exit: as RecordSlotCopy0.
%at($07B948)
RecordSlotCopy3:
    lda.w !wram_0D4A
    asl a
    asl a
    asl a
    clc
    adc.w !wram_0D4A
    adc.b #$03
RecordSlotCopy3_copy:
    tax
    ldy.b #$03
.copy_byte:
    lda.l !save_course_names,x
    sta.l !save_course_names+3,x
    inx
    dey
    bne .copy_byte
    rts


; @routine 07:B90C RecordShiftFrom0
; WHAT IT DOES: Moves the course $0D4A's first record down a place: its
; three-byte entry (RecordSlotCopy0) and its eight-byte times at $70:FC88
; + course * 24 to the next eight.
; HOW: The offset as (course * 2 + course) * 8, then the copy loop of
; RecordShiftFrom1 (its code from the TAX).
; Entry: M 8-bit, X 8-bit. Exit: A = the last byte copied; X = the offset
; + 8; Y = 0; Z set, N clear; C from the last shift.
%at($07B90C)
RecordShiftFrom0:
    jsr.w RecordSlotCopy0
    lda.w !wram_0D4A
    asl a
    clc
    adc.w !wram_0D4A
    asl a
    asl a
    asl a
    bra RecordShiftFrom1_copy

; @routine 07:B91C RecordShiftFrom1
; WHAT IT DOES: Moves the course $0D4A's second record down a place: its
; entry (RecordSlotCopy3) and its times at $70:FC90 + course * 24 to the
; next eight.
; HOW: As RecordShiftFrom0 with 8 added (and the last shift's carry).
; Entry: M 8-bit, X 8-bit. Exit: as RecordShiftFrom0, C from the add.
%at($07B91C)
RecordShiftFrom1:
    jsr.w RecordSlotCopy3
    lda.w !wram_0D4A
    asl a
    clc
    adc.w !wram_0D4A
    asl a
    asl a
    asl a
    adc.b #$08
RecordShiftFrom1_copy:
    tax
    ldy.b #$08
.copy_byte:
    lda.l !save_course_times,x
    sta.l !save_course_times+8,x
    inx
    dey
    bne .copy_byte

; @routine 07:B8DD RecordNameStore0
; WHAT IT DOES: Writes the entered name ($0F11, $0F12) and car ($0D89)
; into the course $0D4A's first three-byte record entry ($70:FC6D +
; course * 9).
; HOW: The entry's offset (0, 3 or 6 from the three entries) through
; direct page $F5; 8-bit long stores.
; Entry: M 8-bit, X 8-bit, D = $0000. Exit: A = the car; X = the entry's
; offset; N, Z from the car.
%at($07B8DD)
RecordNameStore0:
    lda.b #$00
    bra RecordNameStore_slot

; @routine 07:B8E1 RecordNameStore3
; WHAT IT DOES: As RecordNameStore0 for the second entry.
; HOW: The same code with 3; no script reaches it (unit_cpu does).
; Entry: as RecordNameStore0. Exit: as RecordNameStore0.
%at($07B8E1)
RecordNameStore3:
    lda.b #$03                      ; not reached in the traces so far
    bra RecordNameStore_slot        ; not reached in the traces so far

; @routine 07:B8E5 RecordNameStore6
; WHAT IT DOES: As RecordNameStore0 for the third entry.
; HOW: The same code with 6; no script reaches it (unit_cpu does).
; Entry: as RecordNameStore0. Exit: as RecordNameStore0.
%at($07B8E5)
RecordNameStore6:
    lda.b #$06                      ; not reached in the traces so far
RecordNameStore_slot:
    sta.b !save_block_offset
    lda.w !wram_0D4A
    asl a
    asl a
    asl a
    clc
    adc.w !wram_0D4A
    adc.b !save_block_offset
    tax
    lda.w !level_columns+12
    sta.l !save_course_names,x
    lda.w !level_columns+13
    sta.l !save_course_names+1,x
    lda.w !wram_0D89

