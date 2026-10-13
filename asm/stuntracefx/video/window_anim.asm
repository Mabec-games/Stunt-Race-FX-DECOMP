; window_anim.asm: three values at $3435, $3437, $3439 in the data bank (bank
; $7F when called) stepped toward their limits; each step function counts a
; finished value by incrementing X.
; C: src/stuntracefx/video/window_anim.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 03:FE06 WindowStep3437
; WHAT IT DOES: Adds 2 to $3437 in the data bank, stopping at $FF; X counts
; up by one when it stops.
; HOW: 8-bit add; on carry X is incremented and $FF stored instead.
; Entry: M 8-bit, X 8-bit. Exit: A low byte = the stored value; flags from
; the add or, when it stopped, N set, Z clear, C set.
%at($03FE06)
WindowStep3437:
    lda.w !wram_3437
    clc
    adc.b #$02
    bcc .store
    inx
    lda.b #$FF
.store:
    sta.w !wram_3437
    rts

; @routine 03:FE44 WindowStep3435
; WHAT IT DOES: Subtracts 3 from $3435 in the data bank, stopping at 1; X
; counts up by one when it stops.
; HOW: 8-bit subtract; on borrow X is incremented and 1 stored instead.
; Entry: M 8-bit, X 8-bit. Exit: A low byte = the stored value; flags from
; the subtract or, when it stopped, from the load of 1 (C clear).
%at($03FE44)
WindowStep3435:
    lda.w !wram_3435
    sec
    sbc.b #$03
    bcs .store
    inx
    lda.b #$01
.store:
    sta.w !wram_3435
    rts

; @routine 03:FE15 WindowStep3439
; WHAT IT DOES: Adds 1 to $3439 in the data bank, stopping at $E0; X counts
; up by one when it reaches $E0.
; HOW: 8-bit add, compare with $E0.
; Entry: M 8-bit, X 8-bit. Exit: A low byte = the stored value; V from the
; add; N, Z, C from the compare or, when it stopped, from the load of $E0.
%at($03FE15)
WindowStep3439:
    lda.w !wram_3439
    clc
    adc.b #$01
    cmp.b #$E0
    bcc .store
    inx
    lda.b #$E0
.store:
    sta.w !wram_3439
    rts

; @routine 03:FE26 WindowStep3433By2
; WHAT IT DOES: Subtracts 2 from $3433 in the data bank, stopping at 1; X
; counts up by one when it stops.
; HOW: As WindowStep3435. The stop is not reached in the traces so far.
; Entry: M 8-bit, X 8-bit. Exit: as WindowStep3435.
%at($03FE26)
WindowStep3433By2:
    lda.w !wram_3433
    sec
    sbc.b #$02
    bcs .store
    inx
    lda.b #$01
.store:
    sta.w !wram_3433
    rts

; @routine 03:FE35 WindowStep3433By3
; WHAT IT DOES: Subtracts 3 from $3433 in the data bank, stopping at 1; X
; counts up by one when it stops.
; HOW: As WindowStep3435. The stop is not reached in the traces so far.
; Entry: M 8-bit, X 8-bit. Exit: as WindowStep3435.
%at($03FE35)
WindowStep3433By3:
    lda.w !wram_3433
    sec
    sbc.b #$03
    bcs .store
    inx
    lda.b #$01
.store:
    sta.w !wram_3433
    rts

; @routine 03:FDA9 WindowEdgesStep
; WHAT IT DOES: One step of the window edges in bank $7F: $340D counts up
; to $1F; then by how $3433 compares with -$3437: equal, $3437 + 2 and
; $3433 - 2; below, $3437 + 2 and $3433 - 3; above, $3437 + 3 and $3433 -
; 2; then $3439 + 1 and $3435 - 3. When two or more of the steps have
; stopped (X), the fade starts ($0D61 = $80, $0E3A = 1).
; HOW: The data bank is $7F for the steps; X counts the stops. The above
; case is not reached in the traces so far.
; Entry: any widths. Exit: M 8-bit, X 8-bit; X = the stops; the data bank
; restored; A as the last step left it.
%at($03FDA9)
WindowEdgesStep:
    sep.b #$30
    phb
    lda.b #$7F
    pha
    plb
    lda.w !wram_340D
    cmp.b #$1F
    bcs .edges
    inc.w !wram_340D
.edges:
    ldx.b #$00
    lda.w !wram_3437
    eor.b #$FF
    inc a
    cmp.w !wram_3433
    beq .equal
    bcc .below
    jsr.w WindowStep3437By3
    jsr.w WindowStep3433By2
    bra .others
.below:
    jsr.w WindowStep3437
    jsr.w WindowStep3433By3
    bra .others
.equal:
    jsr.w WindowStep3437
    jsr.w WindowStep3433By2
.others:
    jsr.w WindowStep3439
    jsr.w WindowStep3435
    cpx.b #$02
    bcc .done
    lda.b #$80
    sta.l !fade_level_hi
    lda.b #$01
    sta.l !fade_out
.done:
    plb
    rtl

; @routine 03:FDF7 WindowStep3437By3
; WHAT IT DOES: Adds 3 to $3437 in the data bank, stopping at $FF; X
; counts up by one when it stops.
; HOW: As WindowStep3437. Not reached in the traces so far.
; Entry: M 8-bit, X 8-bit. Exit: as WindowStep3437.
%at($03FDF7)
WindowStep3437By3:
    lda.w !wram_3437
    clc
    adc.b #$03
    bcc .store
    inx
    lda.b #$FF
.store:
    sta.w !wram_3437
    rts
