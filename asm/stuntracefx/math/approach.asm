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

; @routine 03:CDC1 ApproachHalfMin2
; WHAT IT DOES: Moves $3E half of the way toward A, by at least 1.
; HOW: The difference, at least 2 either way, halved once.
; Entry: as ApproachQuarterMin4. Exit: as ApproachQuarterMin4.
%at($03CDC1)
ApproachHalfMin2:
    cmp.b !approach_value
    bne .lCDC9
    jml.l ApproachArrived
.lCDC9:
    sec
    sbc.b !approach_value
    cmp.w #$0000
    bmi .lCDDB
    cmp.w #$0002
    bpl .lCDE3
    lda.w #$0002
    bra .lCDE3
.lCDDB:
    cmp.w #$FFFE
    bmi .lCDE3
    lda.w #$FFFE
.lCDE3:
    cmp.w #$8000
    ror a
    bpl .lCDEC
    adc.w #$0000
.lCDEC:
    clc
    adc.b !approach_value
    sta.b !approach_value
    brl ApproachMoved

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

; @routine 03:CE75 ApproachSixteenthMin16
; WHAT IT DOES: Moves $3E a sixteenth of the way toward A, by at least 1.
; HOW: The difference, at least $10 either way, halved four times.
; Entry: as ApproachQuarterMin4. Exit: as ApproachQuarterMin4.
%at($03CE75)
ApproachSixteenthMin16:
    cmp.b !approach_value
    bne .lCE7D
    jml.l ApproachArrived
.lCE7D:
    sec
    sbc.b !approach_value
    cmp.w #$0000
    bmi .lCE8F
    cmp.w #$0010
    bpl .lCE97
    lda.w #$0010
    bra .lCE97
.lCE8F:
    cmp.w #$FFF0
    bmi .lCE97
    lda.w #$FFF0
.lCE97:
    cmp.w #$8000
    ror a
    bpl .lCEA0
    adc.w #$0000
.lCEA0:
    cmp.w #$8000
    ror a
    bpl .lCEA9
    adc.w #$0000
.lCEA9:
    cmp.w #$8000
    ror a
    bpl .lCEB2
    adc.w #$0000
.lCEB2:
    cmp.w #$8000
    ror a
    bpl .lCEBB
    adc.w #$0000
.lCEBB:
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

; @routine 03:CF7A ApproachOneTwentyEighthMin128
; WHAT IT DOES: Moves $3E a 128th of the way toward A, by at least 1.
; HOW: The difference, at least $80 either way, halved seven times.
; Entry: as ApproachQuarterMin4. Exit: as ApproachQuarterMin4.
%at($03CF7A)
ApproachOneTwentyEighthMin128:
    cmp.b !approach_value
    bne .lCF82
    jml.l ApproachArrived
.lCF82:
    sec
    sbc.b !approach_value
    cmp.w #$0000
    bmi .lCF94
    cmp.w #$0080
    bpl .lCF9C
    lda.w #$0080
    bra .lCF9C
.lCF94:
    cmp.w #$FF80
    bmi .lCF9C
    lda.w #$FF80
.lCF9C:
    cmp.w #$8000
    ror a
    bpl .lCFA5
    adc.w #$0000
.lCFA5:
    cmp.w #$8000
    ror a
    bpl .lCFAE
    adc.w #$0000
.lCFAE:
    cmp.w #$8000
    ror a
    bpl .lCFB7
    adc.w #$0000
.lCFB7:
    cmp.w #$8000
    ror a
    bpl .lCFC0
    adc.w #$0000
.lCFC0:
    cmp.w #$8000
    ror a
    bpl .lCFC9
    adc.w #$0000
.lCFC9:
    cmp.w #$8000
    ror a
    bpl .lCFD2
    adc.w #$0000
.lCFD2:
    cmp.w #$8000
    ror a
    bpl .lCFDB
    adc.w #$0000
.lCFDB:
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

; @routine 03:D07C ApproachSixteenthMin16Byte
; WHAT IT DOES: Moves the byte at $3E a sixteenth of the way toward A, by at
; least 1.
; HOW: The difference, at least $10 either way, halved four times, with
; 8-bit arithmetic.
; Entry: M 8-bit, D = $0000; A = target. Exit: as ApproachQuarterMin4 (A
; low byte).
%at($03D07C)
ApproachSixteenthMin16Byte:
    cmp.b !approach_value
    bne .lD084
    jml.l ApproachArrived
.lD084:
    sec
    sbc.b !approach_value
    cmp.b #$00
    bmi .lD093
    cmp.b #$10
    bpl .lD099
    lda.b #$10
    bra .lD099
.lD093:
    cmp.b #$F0
    bmi .lD099
    lda.b #$F0
.lD099:
    cmp.b #$80
    ror a
    bpl .lD0A0
    adc.b #$00
.lD0A0:
    cmp.b #$80
    ror a
    bpl .lD0A7
    adc.b #$00
.lD0A7:
    cmp.b #$80
    ror a
    bpl .lD0AE
    adc.b #$00
.lD0AE:
    cmp.b #$80
    ror a
    bpl .lD0B5
    adc.b #$00
.lD0B5:
    clc
    adc.b !approach_value
    sta.b !approach_value
    brl ApproachMoved

; @routine 03:D0BD ApproachThirtySecondMin32Byte
; WHAT IT DOES: Moves the byte at $3E a thirty-second of the way toward A, by
; at least 1.
; HOW: The difference, at least $20 either way, halved five times, with
; 8-bit arithmetic.
; Entry: M 8-bit, D = $0000; A = target. Exit: as ApproachQuarterMin4 (A
; low byte).
%at($03D0BD)
ApproachThirtySecondMin32Byte:
    cmp.b !approach_value
    bne .lD0C5
    jml.l ApproachArrived
.lD0C5:
    sec
    sbc.b !approach_value
    cmp.b #$00
    bmi .lD0D4
    cmp.b #$20
    bpl .lD0DA
    lda.b #$20
    bra .lD0DA
.lD0D4:
    cmp.b #$E0
    bmi .lD0DA
    lda.b #$E0
.lD0DA:
    cmp.b #$80
    ror a
    bpl .lD0E1
    adc.b #$00
.lD0E1:
    cmp.b #$80
    ror a
    bpl .lD0E8
    adc.b #$00
.lD0E8:
    cmp.b #$80
    ror a
    bpl .lD0EF
    adc.b #$00
.lD0EF:
    cmp.b #$80
    ror a
    bpl .lD0F6
    adc.b #$00
.lD0F6:
    cmp.b #$80
    ror a
    bpl .lD0FD
    adc.b #$00
.lD0FD:
    clc
    adc.b !approach_value
    sta.b !approach_value
    brl ApproachMoved

; @routine 03:D105 ApproachSixtyFourthMin64Byte
; WHAT IT DOES: Moves the byte at $3E a sixty-fourth of the way toward A, by
; at least 1.
; HOW: The difference, at least $40 either way, halved six times, with
; 8-bit arithmetic.
; Entry: M 8-bit, D = $0000; A = target. Exit: as ApproachQuarterMin4 (A
; low byte).
%at($03D105)
ApproachSixtyFourthMin64Byte:
    cmp.b !approach_value
    bne .lD10D
    jml.l ApproachArrived
.lD10D:
    sec
    sbc.b !approach_value
    cmp.b #$00
    bmi .lD11C
    cmp.b #$40
    bpl .lD122
    lda.b #$40
    bra .lD122
.lD11C:
    cmp.b #$C0
    bmi .lD122
    lda.b #$C0
.lD122:
    cmp.b #$80
    ror a
    bpl .lD129
    adc.b #$00
.lD129:
    cmp.b #$80
    ror a
    bpl .lD130
    adc.b #$00
.lD130:
    cmp.b #$80
    ror a
    bpl .lD137
    adc.b #$00
.lD137:
    cmp.b #$80
    ror a
    bpl .lD13E
    adc.b #$00
.lD13E:
    cmp.b #$80
    ror a
    bpl .lD145
    adc.b #$00
.lD145:
    cmp.b #$80
    ror a
    bpl .lD14C
    adc.b #$00
.lD14C:
    clc
    adc.b !approach_value
    sta.b !approach_value
    brl ApproachMoved

; @routine 03:D154 ApproachOneTwentyEighthMin128Byte
; WHAT IT DOES: Meant to move the byte at $3E a 128th of the way toward A;
; unless it is already there, it always steps it down by 1.
; HOW: The difference is raised to $80 either way, and $80 is -128 as a
; byte: halved seven times it is -1 whichever side the target is on.
; Entry: M 8-bit, D = $0000; A = target. Exit: as ApproachQuarterMin4 (A
; low byte).
%at($03D154)
ApproachOneTwentyEighthMin128Byte:
    cmp.b !approach_value
    bne .lD15C
    jml.l ApproachArrived
.lD15C:
    sec
    sbc.b !approach_value
    cmp.b #$00
    bmi .lD16B
    cmp.b #$80
    bpl .lD171
    lda.b #$80
    bra .lD171
.lD16B:
    cmp.b #$80
    bmi .lD171
    lda.b #$80
.lD171:
    cmp.b #$80
    ror a
    bpl .lD178
    adc.b #$00
.lD178:
    cmp.b #$80
    ror a
    bpl .lD17F
    adc.b #$00
.lD17F:
    cmp.b #$80
    ror a
    bpl .lD186
    adc.b #$00
.lD186:
    cmp.b #$80
    ror a
    bpl .lD18D
    adc.b #$00
.lD18D:
    cmp.b #$80
    ror a
    bpl .lD194
    adc.b #$00
.lD194:
    cmp.b #$80
    ror a
    bpl .lD19B
    adc.b #$00
.lD19B:
    cmp.b #$80
    ror a
    bpl .lD1A2
    adc.b #$00
.lD1A2:
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
