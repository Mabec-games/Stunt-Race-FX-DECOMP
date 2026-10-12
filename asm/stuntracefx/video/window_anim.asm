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
