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


; @routine 07:B964 TimeBelowRecord
; WHAT IT DOES: Compares the time $0EEF/$0EEE/$0EED (most significant
; first) with the record time at $0F81 + X: carry clear when it is
; lower; when a byte is higher it returns at once with carry set.
; HOW: 8-bit CMP per byte, BCC lower, BEQ on to the next.
; Entry: M 8-bit; X = the record's offset. Exit: A = the last byte
; compared; N, Z, C from that compare.
%at($07B964)
TimeBelowRecord:
    lda.w !wram_0EEF
    cmp.w !record_times+11,x
    bcc .lB980
    beq .lB96F                      ; not reached in the traces so far
    rts                             ; not reached in the traces so far
.lB96F:
    lda.w !wram_0EEE                ; not reached in the traces so far
    cmp.w !record_times+10,x        ; not reached in the traces so far
    bcc .lB980                      ; not reached in the traces so far
    beq .lB97A                      ; not reached in the traces so far
    rts                             ; not reached in the traces so far
.lB97A:
    lda.w !wram_0EED                ; not reached in the traces so far
    cmp.w !record_times+9,x         ; not reached in the traces so far
.lB980:
    rts


; @routine 07:AC77 BcdToBinary
; WHAT IT DOES: Converts the four BCD digits in A to binary.
; HOW: For each of the thousands, hundreds and tens digits (when A is at
; least $1000, $100, $10), adds 1000, 100 or 10 to $F1 that many times
; and masks the digit off; then adds the units. X kept on the stack. The
; thousands and hundreds run only from unit_cpu.
; Entry: M 16-bit, X 16-bit, D = $0000; A = the BCD value. Exit: A = the
; value; $F1 = A less the units; X unchanged, N and Z from it; C and V from
; the last addition.
%at($07AC77)
BcdToBinary:
    stz.b !digit_count
    phx
    cmp.w #$1000
    bcc .lAC9C
    pha                             ; not reached in the traces so far
    lsr a                           ; not reached in the traces so far
    lsr a                           ; not reached in the traces so far
    lsr a                           ; not reached in the traces so far
    lsr a                           ; not reached in the traces so far
    lsr a                           ; not reached in the traces so far
    lsr a                           ; not reached in the traces so far
    lsr a                           ; not reached in the traces so far
    lsr a                           ; not reached in the traces so far
    lsr a                           ; not reached in the traces so far
    lsr a                           ; not reached in the traces so far
    lsr a                           ; not reached in the traces so far
    lsr a                           ; not reached in the traces so far
    tax                             ; not reached in the traces so far
    lda.b !digit_count              ; not reached in the traces so far
.lAC8F:
    clc                             ; not reached in the traces so far
    adc.w #$03E8                    ; not reached in the traces so far
    dex                             ; not reached in the traces so far
    bne .lAC8F                      ; not reached in the traces so far
    sta.b !digit_count              ; not reached in the traces so far
    pla                             ; not reached in the traces so far
    and.w #$0FFF                    ; not reached in the traces so far
.lAC9C:
    cmp.w #$0100
    bcc .lACBA
    pha                             ; not reached in the traces so far
    lsr a                           ; not reached in the traces so far
    lsr a                           ; not reached in the traces so far
    lsr a                           ; not reached in the traces so far
    lsr a                           ; not reached in the traces so far
    lsr a                           ; not reached in the traces so far
    lsr a                           ; not reached in the traces so far
    lsr a                           ; not reached in the traces so far
    lsr a                           ; not reached in the traces so far
    tax                             ; not reached in the traces so far
    lda.b !digit_count              ; not reached in the traces so far
.lACAD:
    clc                             ; not reached in the traces so far
    adc.w #$0064                    ; not reached in the traces so far
    dex                             ; not reached in the traces so far
    bne .lACAD                      ; not reached in the traces so far
    sta.b !digit_count              ; not reached in the traces so far
    pla                             ; not reached in the traces so far
    and.w #$00FF                    ; not reached in the traces so far
.lACBA:
    cmp.w #$0010
    bcc .lACD4
    pha
    lsr a
    lsr a
    lsr a
    lsr a
    tax
    lda.b !digit_count
.lACC7:
    clc
    adc.w #$000A
    dex
    bne .lACC7
    sta.b !digit_count
    pla
    and.w #$000F
.lACD4:
    clc
    adc.b !digit_count
    plx
    rtl


; @routine 07:AC13 TimePackBcd
; WHAT IT DOES: Packs a time given in BCD into one binary word: the
; hundredths (A's low byte) in bits 0-6, the seconds (X's low byte) in
; bits 7-12, the minutes (Y's low byte) in bits 13-15.
; HOW: BcdToBinary on each; the parts gathered at $F3 (XBA and LSR for the
; seconds, four RORs for the minutes).
; Entry: M 16-bit, X 16-bit, D = $0000. Exit: A = the packed time, $F3 =
; its bits 0-12; N, Z from A; C = bit 3 of the minutes; X and Y unchanged.
%at($07AC13)
TimePackBcd:
    and.w #$00FF
    jsl.l BcdToBinary
    and.w #$007F
    sta.b !dp_f3
    txa
    and.w #$00FF
    jsl.l BcdToBinary
    xba
    lsr a
    and.w #$1F80
    ora.b !dp_f3
    sta.b !dp_f3
    tya
    and.w #$00FF
    jsl.l BcdToBinary
    ror a
    ror a
    ror a
    ror a
    and.w #$E000
    ora.b !dp_f3
    rts

; @routine 07:B7DE RecordTimeInsert
; WHAT IT DOES: Unless $0D24 is 3, places the race time ($0EED-$0EEF) in
; the course $0D4A's records: below the first record (TimeBelowRecord) it
; becomes the first, the first two moving down (RecordShiftFrom1, then
; From0, RecordNameStore0; place 1 in $0FAC, page 0); below the second it
; becomes the second (RecordShiftFrom1, RecordNameStore3; place 2, page
; 8); below the third, the third (RecordNameStore6; place 3, page $10);
; otherwise $10C7 = $FF and $0FAC = 0, and nothing else. Then its four lap
; times ($0EF9 + 3n: hundredths, seconds, minutes) go packed (TimePackBcd)
; to $70:FC88 + course * 24 + page + 2n, and the course's checksum at
; $70:FC2D + course is remade: the sum of its 24 time bytes and its nine
; name bytes at $70:FC6D + course * 9.
; HOW: P pushed; 8-bit compares, the offset at $F7 (16-bit, with the
; carry of its last shift), 16-bit loads for TimePackBcd, then the 8-bit
; sums at $F1. Only the first place and the end are reached in the traces
; so far.
; Entry: any widths; D = $0000. Exit: P restored; A low byte the checksum
; (high byte the last packed time's), X = course, Y = 0; or, at $0D24 = 3,
; A low byte 3; with no record, A low byte 0 and X = $18.
%at($07B7DE)
RecordTimeInsert:
    php
    sep.b #$30
    lda.w !wram_0D24
    cmp.b #$03
    bne .lB7EA
    plp
    rts
.lB7EA:
    sep.b #$30
    ldx.b #$00
    jsr.w TimeBelowRecord
    bcs .lB805
    jsr.w RecordShiftFrom1
    jsr.w RecordShiftFrom0
    jsr.w RecordNameStore0
    lda.b #$01
    sta.w !p1_lead+9
    lda.b #$00
    bra .lB838
.lB805:
    ldx.b #$0C
    jsr.w TimeBelowRecord
    bcs .lB81B
    jsr.w RecordShiftFrom1
    jsr.w RecordNameStore3
    lda.b #$02
    sta.w !p1_lead+9
    lda.b #$08
    bra .lB838
.lB81B:
    ldx.b #$18
    jsr.w TimeBelowRecord
    bcc .lB82E
    lda.b #$FF
    sta.w !player_tag
    lda.b #$00
    sta.w !p1_lead+9
    plp
    rts
.lB82E:
    jsr.w RecordNameStore6
    lda.b #$03
    sta.w !p1_lead+9
    lda.b #$10
.lB838:
    rep.b #$30
    and.w #$00FF
    sta.b !record_page
    lda.w !wram_0D4A
    asl a
    clc
    adc.w !wram_0D4A
    and.w #$00FF
    asl a
    asl a
    asl a
    adc.b !record_page
    sta.b !record_page
    lda.w !lap_times+9
    ldx.w !lap_times+10
    ldy.w !lap_times+11
    jsr.w TimePackBcd
    ldx.b !record_page
    sta.l !save_course_times,x
    lda.w !lap_times+12
    ldx.w !lap_times+13
    ldy.w !lap_times+14
    jsr.w TimePackBcd
    ldx.b !record_page
    sta.l !save_course_times+2,x
    lda.w !lap_times+15
    ldx.w !lap_times+16
    ldy.w !lap_times+17
    jsr.w TimePackBcd
    ldx.b !record_page
    sta.l !save_course_times+4,x
    lda.w !lap_times+18
    ldx.w !lap_times+19
    ldy.w !lap_times+20
    jsr.w TimePackBcd
    ldx.b !record_page
    sta.l !save_course_times+6,x
    sep.b #$30
    stz.b !digit_count
    lda.w !wram_0D4A
    asl a
    clc
    adc.w !wram_0D4A
    asl a
    asl a
    asl a
    tax
    ldy.b #$18
.lB8AB:
    clc
    lda.l !save_course_times,x
    adc.b !digit_count
    sta.b !digit_count
    inx
    dey
    bne .lB8AB
    lda.w !wram_0D4A
    asl a
    asl a
    asl a
    clc
    adc.w !wram_0D4A
    tax
    ldy.b #$09
.lB8C5:
    clc
    lda.l !save_course_names,x
    adc.b !digit_count
    sta.b !digit_count
    inx
    dey
    bne .lB8C5
    ldx.w !wram_0D4A
    lda.b !digit_count
    sta.l !save_table_sums,x
    plp
    rts
