; selection.asm: small steps of the menu screens' selection logic: the
; choices at $18C9-$18D3, values derived from them at $18E3-$1907, and the
; menu's exit flag at $11EB.
; C: src/stuntracefx/menu/selection.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 0B:C58D MenuRowPosition
; WHAT IT DOES: Returns a position from object X's +$12 and the value at
; $18ED: (2 - $18ED) * 16 + the field + 2.
; HOW: 16-bit arithmetic; the field is read as direct page $12 + X.
; Entry: M 16-bit, D = $0000; X = record. Exit: A = the position; N, Z from
; it; C, V from the add.
%at($0BC58D)
MenuRowPosition:
    lda.w #$0002
    sec
    sbc.l !selection_derived+$A
    asl a
    asl a
    asl a
    asl a
    clc
    adc.b !actor_12,x
    inc a
    inc a
    rts

; @routine 0B:C736 MenuChoiceRepeat
; WHAT IT DOES: Sets $18D3 to the table value for $18C9 when the choice at
; $18CB equals it, and to 0 otherwise.
; HOW: Clears $18D3; on a match reads word table $0B:C758[$18C9].
; Entry: any widths. Exit: M 16-bit, X 16-bit. No match: A = $18CB; N, Z, C
; from the compare. Match: A = the value, X = $18C9 * 2; N, Z from A; C =
; bit 15 of $18C9 (shifted out by the doubling).
%at($0BC736)
MenuChoiceRepeat:
    rep.b #$30
    lda.w #$0000
    sta.l !selection_out
    lda.l !selection
    cmp.l !selection_prev
    bne .lC757
    lda.l !selection_prev
    asl a
    tax
    lda.l !menu_repeat_table,x
    sta.l !selection_out
.lC757:
    rts

; @routine 0B:BCD2 MenuWaitFor0357
; WHAT IT DOES: An object handler that waits for the byte at $0357 to become
; non-zero, then sets $1905 = 2, $1907 = 0 and moves on to handler $BCF1.
; HOW: Byte test through the data bank; the handler is object X's +$08.
; Entry: D = $0000; X = record. Exit: M 16-bit. Waiting: A low byte = 0; Z
; set, N clear. Moved on: A = $BCF1; N set, Z clear.
%at($0BBCD2)
MenuWaitFor0357:
    sep.b #$20
    lda.w !wram_0357
    beq .lBCEE
    rep.b #$20
    lda.w #$0002
    sta.l !menu_1905
    lda.w #$0000
    sta.l !menu_1907
    lda.w #$BCF1
    sta.b !actor_handler,x
.lBCEE:
    rep.b #$20
    rts

; @routine 0B:D4A5 MenuSetExit
; WHAT IT DOES: Sets the menu's exit flag at $11EB.
; HOW: Stores 1 (16-bit) through the data bank.
; Entry: any widths. Exit: A = 1; M 16-bit; N, Z clear.
%at($0BD4A5)
MenuSetExit:
    rep.b #$20
    lda.w #$0001
    sta.w !menu_exit
    rts

; @routine 0B:C723 MenuSet1A66
; WHAT IT DOES: Copies entry A of the word table at $00:116C to $1A66.
; HOW: Long read at $116C + A * 2; the store goes through the data bank.
; Entry: any widths; A = entry (16 bits). Exit: A unchanged; X = A * 2; Y =
; $1A64 (low bytes only when X is 8-bit); P unchanged.
%at($0BC723)
MenuSet1A66:
    php
    rep.b #$30
    pha
    asl a
    tax
    lda.l !table_116c,x
    ldy.w #$1A64
    sta.w 2,y                       ; $1A66
    pla
    plp
    rts
