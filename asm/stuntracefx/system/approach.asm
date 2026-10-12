; approach.asm: a value at $11DA that moves one step toward its target at
; $11DB every other call.
; C: src/stuntracefx/system/approach.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"

arch 65816

; @routine 7F:0FDC HalfRateApproach
; WHAT IT DOES: Every other call, moves $11DA one step toward $11DB.
; HOW: Toggles bit 0 of $11DC; on the calls where it becomes 0, compares and
; steps $11DA up or down by one, or leaves it when equal.
; Entry: any widths. Exit: A low byte = the toggle (1), or $11DA's value;
; P unchanged.
%ramcode($7F0FDC, $0587DC)
HalfRateApproach:
    php
    sep.b #$20
    lda.w !wram_11DC
    inc a
    and.b #$01
    sta.w !wram_11DC
    bne .l1005
    lda.w !wram_11DA
    cmp.w !wram_11DB
    beq .l1005
    bcs .l0FFE
    lda.w !wram_11DA
    inc a
    sta.w !wram_11DA
    jmp.w .l1005
.l0FFE:
    lda.w !wram_11DA
    dec a
    sta.w !wram_11DA
.l1005:
    plp
    rts

; @routine 0B:E2C5 ApproachTargetFull
; WHAT IT DOES: Sets the target at $11DB to $0F.
; HOW: One byte store through the data bank.
; Entry: any widths. Exit: A low byte = $0F; P unchanged.
%at($0BE2C5)
ApproachTargetFull:
    php
    sep.b #$20
    lda.b #$0F
    sta.w !wram_11DB
    plp
    rts

; @routine 0B:E2CF ApproachTargetZero
; WHAT IT DOES: Sets the target at $11DB to 0.
; HOW: One byte store through the data bank.
; Entry: any widths. Exit: every register unchanged.
%at($0BE2CF)
ApproachTargetZero:
    php
    sep.b #$20
    stz.w !wram_11DB
    plp
    rts
