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
    sbc.l !wram_18ED
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

; @routine 0B:D4C3 MenuStateInit
; WHAT IT DOES: Sets the menu's selection state to its defaults: the four
; choices at $18CB-$18D1 to 0, 0, 1, 2, the derived values and positions to
; their starting values, the draw-buffer offset $0D7D to 6, and three
; values at $18EF-$18F3 from the save data ($70:FC08-$70:FC0A).
; HOW: 16-bit long stores in the original's order.
; Entry: any widths. Exit: A = $001F; P unchanged.
%at($0BD4C3)
MenuStateInit:
    php
    rep.b #$30
    lda.w #$0006
    sta.l !gsu_buffer_offset
    lda.w #$0000
    sta.l !wram_18FD
    lda.w #$0000
    sta.l !wram_1903
    lda.w #$0001
    sta.l !menu_18b9
    lda.w #$0000
    sta.l !wram_1901
    sta.l !wram_18FF
    sta.l !wram_18F7
    sta.l !wram_18C3
    sta.l !selection
    sta.l !selection_prev
    sta.l !wram_18ED
    sta.l !selection_1
    sta.l !wram_18F5
    lda.w #$0000
    sta.l !selection_out
    lda.w #$0000
    sta.l !selection_out_1
    lda.w #$0000
    sta.l !selection_out_2
    lda.w #$0003
    sta.l !selection_out_3
    lda.w #$0001
    sta.l !wram_18C7
    sta.l !wram_18EB
    sta.l !selection_2
    lda.w #$0002
    sta.l !selection_3
    lda.l !save_bytes+2
    and.w #$00FF
    sta.l !wram_18EF
    lda.l !save_bytes+3
    and.w #$00FF
    sta.l !wram_18F1
    lda.l !save_bytes+4
    and.w #$00FF
    sta.l !wram_18F3
    lda.w #$FFFF
    sta.l !wram_18C5
    lda.w #$001F
    sta.l !wram_18F9
    plp
    rts

; @routine 03:B548 SelectionCommit
; WHAT IT DOES: Records the two choices at $18CB and $18C9: their handler
; addresses (HandlerTableA) at $18DB and $18DD; both packed as a byte (the
; table at $03:B540 for each low two bits, $18C9's in the high nibble) into
; the save data at $70:FC0E, with the checksum renewed; and the derived
; values $18E3 and $18E5 from the table at $03:B46C, where $18D5 is the
; table $03:B464 entry for $18C9 when the two choices are the same, else 0.
; HOW: 16-bit lookups; $11DF holds the high nibble.
; Entry: any widths. Exit: A = $18E5's value; X = its table offset; P
; unchanged.
%at($03B548)
SelectionCommit:
    php
    rep.b #$30
    lda.l !selection
    jsr.w HandlerTableA
    sta.l !selection_handler_a
    lda.l !selection_prev
    jsr.w HandlerTableA
    sta.l !selection_handler_b
    lda.l !selection_prev
    and.w #$0003
    asl a
    tax
    lda.l !selection_bits_b540,x
    asl a
    asl a
    asl a
    asl a
    sta.w !scratch_11df
    lda.l !selection
    and.w #$0003
    asl a
    tax
    lda.l !selection_bits_b540,x
    ora.w !scratch_11df
    sep.b #$20
    sta.l !save_bytes+8
    rep.b #$20
    jsl.l SaveChecksum
    lda.w #$0000
    sta.l !selection_out
    sta.l !selection_out_1
    lda.l !selection
    cmp.l !selection_prev
    bne .lB5B3
    lda.l !selection_prev
    asl a
    tax
    lda.l !selection_repeat_b464,x
    sta.w !selection_out_1
.lB5B3:
    lda.l !selection
    asl a
    asl a
    clc
    adc.l !selection_out
    asl a
    tax
    lda.l !selection_derived_b46c,x
    sta.l !selection_derived
    lda.l !selection_prev
    asl a
    asl a
    clc
    adc.l !selection_out_1
    asl a
    tax
    lda.l !selection_derived_b46c,x
    sta.l !wram_18E5
    plp
    rtl

; @routine 03:B48C SelectionSave
; WHAT IT DOES: Records the choices: the handler addresses for $18CB and
; $18C9 (HandlerTableA) at $18DB and $18DD, then for $18CD, $18CF, $18D1
; (HandlerTableB) at $18DD, $18DF, $18E1; derives the values from them
; (SelectionTables18CB); and packs the choice into the save data by mode
; (see save_selection), renewing the checksum.
; HOW: 16-bit work, a byte store into the save data, SaveChecksum.
; Entry: any widths. Exit: A low byte = the checksum, high byte from the
; packed value; X = the table offset (low byte when X is 8-bit); P
; unchanged.
%at($03B48C)
SelectionSave:
    php
    rep.b #$30
    lda.l !selection
    jsr.w HandlerTableA
    sta.l !selection_handler_a
    lda.l !selection_prev
    jsr.w HandlerTableA
    sta.l !selection_handler_b
    lda.l !selection_1
    jsr.w HandlerTableB
    sta.l !selection_handler_b
    lda.l !selection_2
    jsr.w HandlerTableB
    sta.l !selection_handler_c
    lda.l !selection_3
    jsr.w HandlerTableB
    sta.l !selection_handler_d
    jsr.w SelectionTables18CB
    lda.l !wram_18C3
    cmp.w #$0003
    beq .lB525
    cmp.w #$0001
    beq .lB4FC
    lda.l !save_bytes+6
    and.w #$0030
    sta.w !scratch_11df
    lda.l !selection
    asl a
    tax
    lda.l !selection_bits_b540,x
    ora.w !scratch_11df
    sep.b #$20
    sta.l !save_bytes+6
    rep.b #$20
    jsl.l SaveChecksum
    bra .lB53E
.lB4FC:
    lda.l !save_bytes+6
    and.w #$0003
    sta.w !scratch_11df
    lda.l !selection
    asl a
    tax
    lda.l !selection_bits_b540,x
    asl a
    asl a
    asl a
    asl a
    ora.w !scratch_11df
    sep.b #$20
    sta.l !save_bytes+6
    rep.b #$20
    jsl.l SaveChecksum
    bra .lB53E
.lB525:
    lda.l !selection
    and.w #$0003
    asl a
    tax
    lda.l !selection_bits_b540,x
    sep.b #$20
    sta.l !save_bytes+7
    rep.b #$20
    jsl.l SaveChecksum
.lB53E:
    plp
    rtl
