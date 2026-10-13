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
