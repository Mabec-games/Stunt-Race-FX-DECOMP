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
; Entry, exit: as HandlerTableA.
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
    lda.l !course_pointers+$2,x
    tyx
    rtl
