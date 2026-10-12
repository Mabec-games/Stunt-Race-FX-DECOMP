; approach.asm: moving the value in direct page $3E toward a target. The
; step is the difference, raised to a minimum size, then divided by a power
; of two (each halving rounds toward zero). Every routine ends in one of two
; shared tails: $03:D1AA returns $3E with carry clear (it moved), $03:D1AE
; returns it with carry set (it was already there).
; C: src/stuntracefx/math/approach.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 03:CDF4 ApproachQuarterMin4
; WHAT IT DOES: Moves $3E a quarter of the way toward A, by at least 1.
; HOW: The difference, at least 4 either way, halved twice; the tails at
; $03:D1AA / $03:D1AE.
; Entry: M 16-bit, D = $0000; A = target. Exit: A = $3E; N, Z from it; C
; clear when it moved (V from the add), set when it was already there.
%at($03CDF4)
ApproachQuarterMin4:
    cmp.b !approach_value
    bne .lCDFC
    jml.l ApproachArrived
.lCDFC:
    sec
    sbc.b !approach_value
    cmp.w #$0000
    bmi .lCE0E
    cmp.w #$0004
    bpl .lCE16
    lda.w #$0004
    bra .lCE16
.lCE0E:
    cmp.w #$FFFC
    bmi .lCE16
    lda.w #$FFFC
.lCE16:
    cmp.w #$8000
    ror a
    bpl .lCE1F
    adc.w #$0000
.lCE1F:
    cmp.w #$8000
    ror a
    bpl .lCE28
    adc.w #$0000
.lCE28:
    clc
    adc.b !approach_value
    sta.b !approach_value
    brl ApproachMoved

; @routine 03:CE30 ApproachEighthMin8
; WHAT IT DOES: Moves $3E an eighth of the way toward A, by at least 1.
; HOW: The difference, at least 8 either way, halved three times.
; Entry: as ApproachQuarterMin4. Exit: as ApproachQuarterMin4.
%at($03CE30)
ApproachEighthMin8:
    cmp.b !approach_value
    bne .lCE38
    jml.l ApproachArrived
.lCE38:
    sec
    sbc.b !approach_value
    cmp.w #$0000
    bmi .lCE4A
    cmp.w #$0008
    bpl .lCE52
    lda.w #$0008
    bra .lCE52
.lCE4A:
    cmp.w #$FFF8
    bmi .lCE52
    lda.w #$FFF8
.lCE52:
    cmp.w #$8000
    ror a
    bpl .lCE5B
    adc.w #$0000
.lCE5B:
    cmp.w #$8000
    ror a
    bpl .lCE64
    adc.w #$0000
.lCE64:
    cmp.w #$8000
    ror a
    bpl .lCE6D
    adc.w #$0000
.lCE6D:
    clc
    adc.b !approach_value
    sta.b !approach_value
    brl ApproachMoved

; @routine 03:CEC3 ApproachThirtySecondMin32
; WHAT IT DOES: Moves $3E a thirty-second of the way toward A, by at least
; 1.
; HOW: The difference, at least $20 either way, halved five times.
; Entry: as ApproachQuarterMin4. Exit: as ApproachQuarterMin4.
%at($03CEC3)
ApproachThirtySecondMin32:
    cmp.b !approach_value
    bne .lCECB
    jml.l ApproachArrived           ; not reached in the traces so far
.lCECB:
    sec
    sbc.b !approach_value
    cmp.w #$0000
    bmi .lCEDD
    cmp.w #$0020
    bpl .lCEE5
    lda.w #$0020                    ; not reached in the traces so far
    bra .lCEE5                      ; not reached in the traces so far
.lCEDD:
    cmp.w #$FFE0                    ; not reached in the traces so far
    bmi .lCEE5                      ; not reached in the traces so far
    lda.w #$FFE0                    ; not reached in the traces so far
.lCEE5:
    cmp.w #$8000
    ror a
    bpl .lCEEE
    adc.w #$0000                    ; not reached in the traces so far
.lCEEE:
    cmp.w #$8000
    ror a
    bpl .lCEF7
    adc.w #$0000                    ; not reached in the traces so far
.lCEF7:
    cmp.w #$8000
    ror a
    bpl .lCF00
    adc.w #$0000                    ; not reached in the traces so far
.lCF00:
    cmp.w #$8000
    ror a
    bpl .lCF09
    adc.w #$0000                    ; not reached in the traces so far
.lCF09:
    cmp.w #$8000
    ror a
    bpl .lCF12
    adc.w #$0000                    ; not reached in the traces so far
.lCF12:
    clc
    adc.b !approach_value
    sta.b !approach_value
    brl ApproachMoved

; @routine 03:CF1A ApproachSixtyFourthMin64
; WHAT IT DOES: Moves $3E a sixty-fourth of the way toward A, by at least 1.
; HOW: The difference, at least $40 either way, halved six times.
; Entry: as ApproachQuarterMin4. Exit: as ApproachQuarterMin4.
%at($03CF1A)
ApproachSixtyFourthMin64:
    cmp.b !approach_value
    bne .lCF22
    jml.l ApproachArrived           ; not reached in the traces so far
.lCF22:
    sec
    sbc.b !approach_value
    cmp.w #$0000
    bmi .lCF34
    cmp.w #$0040
    bpl .lCF3C
    lda.w #$0040                    ; not reached in the traces so far
    bra .lCF3C                      ; not reached in the traces so far
.lCF34:
    cmp.w #$FFC0                    ; not reached in the traces so far
    bmi .lCF3C                      ; not reached in the traces so far
    lda.w #$FFC0                    ; not reached in the traces so far
.lCF3C:
    cmp.w #$8000
    ror a
    bpl .lCF45
    adc.w #$0000                    ; not reached in the traces so far
.lCF45:
    cmp.w #$8000
    ror a
    bpl .lCF4E
    adc.w #$0000                    ; not reached in the traces so far
.lCF4E:
    cmp.w #$8000
    ror a
    bpl .lCF57
    adc.w #$0000                    ; not reached in the traces so far
.lCF57:
    cmp.w #$8000
    ror a
    bpl .lCF60
    adc.w #$0000                    ; not reached in the traces so far
.lCF60:
    cmp.w #$8000
    ror a
    bpl .lCF69
    adc.w #$0000                    ; not reached in the traces so far
.lCF69:
    cmp.w #$8000
    ror a
    bpl .lCF72
    adc.w #$0000                    ; not reached in the traces so far
.lCF72:
    clc
    adc.b !approach_value
    sta.b !approach_value
    brl ApproachMoved

; @routine 03:CFE3 ApproachHalfMin2Byte
; WHAT IT DOES: Moves the byte at $3E half of the way toward A, by at least
; 1.
; HOW: The difference, at least 2 either way, halved once, with 8-bit
; arithmetic. Only the arrived case is reached in the traces so far.
; Entry: M 8-bit, D = $0000; A = target. Exit: as ApproachQuarterMin4 (A low
; byte).
%at($03CFE3)
ApproachHalfMin2Byte:
    cmp.b !approach_value
    bne .lCFEB
    jml.l ApproachArrived
.lCFEB:                             ; not reached in the traces so far, to the end
    sec
    sbc.b !approach_value
    cmp.b #$00
    bmi .lCFFA
    cmp.b #$02
    bpl .lD000
    lda.b #$02
    bra .lD000
.lCFFA:
    cmp.b #$FE
    bmi .lD000
    lda.b #$FE
.lD000:
    cmp.b #$80
    ror a
    bpl .lD007
    adc.b #$00
.lD007:
    clc
    adc.b !approach_value
    sta.b !approach_value
    brl ApproachMoved

; @routine 03:D00F ApproachQuarterMin4Byte
; WHAT IT DOES: The 8-bit form of ApproachQuarterMin4, on the byte at $3E.
; HOW: As ApproachQuarterMin4, with 8-bit arithmetic.
; Entry: M 8-bit, D = $0000; A = target. Exit: as ApproachQuarterMin4 (A
; low byte).
%at($03D00F)
ApproachQuarterMin4Byte:
    cmp.b !approach_value
    bne .lD017
    jml.l ApproachArrived
.lD017:
    sec
    sbc.b !approach_value
    cmp.b #$00
    bmi .lD026
    cmp.b #$04
    bpl .lD02C
    lda.b #$04
    bra .lD02C
.lD026:
    cmp.b #$FC
    bmi .lD02C
    lda.b #$FC
.lD02C:
    cmp.b #$80
    ror a
    bpl .lD033
    adc.b #$00
.lD033:
    cmp.b #$80
    ror a
    bpl .lD03A
    adc.b #$00
.lD03A:
    clc
    adc.b !approach_value
    sta.b !approach_value
    brl ApproachMoved

; @routine 03:D042 ApproachEighthMin8Byte
; WHAT IT DOES: Moves the byte at $3E an eighth of the way toward A, by at
; least 1.
; HOW: The difference, at least 8 either way, halved three times, with 8-bit
; arithmetic.
; Entry: M 8-bit, D = $0000; A = target. Exit: as ApproachQuarterMin4 (A low
; byte).
%at($03D042)
ApproachEighthMin8Byte:
    cmp.b !approach_value
    bne .lD04A
    jml.l ApproachArrived
.lD04A:
    sec
    sbc.b !approach_value
    cmp.b #$00
    bmi .lD059
    cmp.b #$08
    bpl .lD05F
    lda.b #$08
    bra .lD05F
.lD059:
    cmp.b #$F8
    bmi .lD05F
    lda.b #$F8
.lD05F:
    cmp.b #$80
    ror a
    bpl .lD066
    adc.b #$00
.lD066:
    cmp.b #$80
    ror a
    bpl .lD06D
    adc.b #$00
.lD06D:
    cmp.b #$80
    ror a
    bpl .lD074
    adc.b #$00
.lD074:
    clc
    adc.b !approach_value
    sta.b !approach_value
    brl ApproachMoved

; The shared tails ($03:D1AA, $03:D1AE): $3E back in A, carry clear when it
; moved, set when it was already at the target.
%at($03D1AA)
ApproachMoved:
    lda.b !approach_value
    clc
    rtl
ApproachArrived:
    lda.b !approach_value
    sec
    rtl
