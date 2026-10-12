; tables.asm: lookups into small tables in ROM: handler addresses and the
; per-course table index ($0E3D holds the course number).
; C: src/stuntracefx/data/tables.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 03:B6E6 HandlerTableA
; WHAT IT DOES: Returns entry A of the four-entry handler address table at
; $03:B6F1.
; HOW: Word lookup at A * 2.
; Entry: any widths. Exit: A = the entry; X = A * 2; P unchanged.
%at($03B6E6)
HandlerTableA:
    php
    rep.b #$30
    asl a
    tax
    lda.l !handler_table_a,x
    plp
    rts

; @routine 03:B6F9 HandlerTableB
; WHAT IT DOES: Returns entry A of the handler address table at $03:B704.
; HOW: Word lookup at A * 2.
; Entry: as HandlerTableA. Exit: as HandlerTableA.
%at($03B6F9)
HandlerTableB:
    php
    rep.b #$30
    asl a
    tax
    lda.l !handler_table_b,x
    plp
    rts

; @routine 03:EFBB CourseIndex12
; WHAT IT DOES: Returns the index course * 12 + A * 3 for tables with twelve
; bytes per course and three per entry.
; HOW: 8-bit arithmetic; A * 3 is kept in direct page $F1.
; Entry: M 8-bit, X 8-bit, D = $0000; A = entry. Exit: X = A = the index;
; $F1 = A * 3; flags from the last add.
%at($03EFBB)
CourseIndex12:
    sta.b !digit_count
    asl a
    clc
    adc.b !digit_count
    sta.b !digit_count
    lda.w !course
    asl a
    clc
    adc.w !course
    asl a
    asl a
    clc
    adc.b !digit_count
    tax
    rts

; @routine 03:EC24 CourseDataPointer
; WHAT IT DOES: Returns the current course's long pointer from the table at
; $03:F4A5: X = address - $8000, A low byte = bank.
; HOW: Three bytes per course; the index is course * 3 (8-bit).
; Entry: M 8-bit, X 16-bit. Exit: X = address - $8000; A = the same high
; byte, the bank low; M 8-bit, X 16-bit; N, Z from X; C, V from the subtract.
%at($03EC24)
CourseDataPointer:
    rep.b #$30
    lda.w !course
    asl a
    clc
    adc.w !course
    and.w #$00FF
    tax
    lda.l !course_pointers,x
    sec
    sbc.w #$8000
    tay
    sep.b #$20
    lda.l !course_pointers+2,x
    tyx
    rtl

; @routine 03:B70E SelectionTables18CB
; WHAT IT DOES: Derives the 2-bit values at $18D3-$18E9 from the four
; selections at $18CB, $18CD, $18CF, $18D1 through tables in banks $0B and
; $03.
; HOW: base = word table $0B:B00C[$18CB], kept at $11DF; index = base +
; $18CD * 3 + $18CF * 9 + $18D1 * 27 (partial sums at $11E1 and $11E3);
; $18E5, $18E7, $18E9 = bytes 0-2 of $0B:B014[index] & 3. $18E3 =
; $03:B7B1[$18CB] & 3; then for i = 0-3, $18D3 + 2i = $03:B7B5[selection i
; * 4 + the value at $18E3 + 2i] & 3. All 16-bit; $11DF-$11E3 through the
; data bank.
; Entry: any widths. Exit: A = the last value stored; X = 8; P unchanged.
%at($03B70E)
SelectionTables18CB:
    php
    rep.b #$30
    lda.l !selection
    asl a
    tax
    lda.l !sel_base_table,x
    sta.w !scratch_11df
    lda.l !selection_1
    asl a
    clc
    adc.l !selection_1
    sta.w !scratch_11e1
    lda.l !selection_2
    asl a
    asl a
    asl a
    clc
    adc.l !selection_2
    clc
    adc.w !scratch_11e1
    sta.w !scratch_11e1
    lda.l !selection_3
    asl a
    clc
    adc.l !selection_3
    sta.w !scratch_11e3
    asl a
    asl a
    asl a
    clc
    adc.w !scratch_11e3
    clc
    adc.w !scratch_11e1
    clc
    adc.w !scratch_11df
    tax
    lda.l !sel_table,x
    and.w #$0003
    sta.l !wram_18E5
    lda.l !sel_table+1,x
    and.w #$0003
    sta.l !wram_18E7
    lda.l !sel_table+2,x
    and.w #$0003
    sta.l !wram_18E9
    lda.l !selection
    tax
    lda.l !sel_derived_table,x
    and.w #$0003
    sta.l !selection_derived
    ldx.w #$0000
.lB78F:
    phx
    lda.l !selection,x
    asl a
    asl a
    clc
    adc.l !selection_derived,x
    tax
    lda.l !sel_out_table,x
    and.w #$0003
    plx
    sta.l !selection_out,x
    inx
    inx
    cpx.w #$0008
    bne .lB78F
    plp
    rts

; @routine 03:B70A SelectionTables18CBLong
; WHAT IT DOES: SelectionTables18CB for callers in other banks.
; HOW: JSR to it, then RTL.
; Entry: as SelectionTables18CB. Exit: as SelectionTables18CB.
%at($03B70A)
SelectionTables18CBLong:
    jsr.w SelectionTables18CB
    rtl
