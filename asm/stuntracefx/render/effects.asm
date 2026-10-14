; effects.asm: the Super FX's short-lived screen effects. Sixteen records of
; six bytes at $2688: +$00 the time left (0: the record is free), +$01 a
; byte, +$02 X and +$04 Y on the screen (words).
; C: src/stuntracefx/render/effects.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"

arch superfx

; @routine 01:C060 FxEffectsAge
; WHAT IT DOES: Counts down the time of each effect in use and frees the
; ones outside the screen less a margin of 8: X below $38 + 8 or not below
; $3A - 8, Y below $3C + 8 or not below $3E - 8.
; HOW: The margins into R8, R7, R4 and R5 (ADD #8, SUB #8). R6 = 8 times
; the word at +$4A of the record at $247A, or 0 when that is negative
; (nothing here uses it). A LOOP over the sixteen records, R13 = its
; start ($C095), R3 stepping by 6: a record whose time (LDB, LOB) is not
; 0 has it counted down (DEC, STB); then R2 = its byte, R9 = $0034
; (unused), R1 = X, R2 = Y, and CMP against each margin; one outside has
; its first word set to 0. WITH then TO is a MOVE, WITH then FROM a
; MOVES. Returns by a MOVE of R11 to R15.
; Entry: R11 = return address. Exit: R3 = $26E8; R12 = 0; R13 = $C095;
; R8, R7, R4, R5 the margins; R6 as above; R0, R1, R2, R9 and the OV and
; CY flags as the last record in use, or the last free record, left them;
; S clear, Z set (LOOP).
%at($01C060)
FxEffectsAge:
    lms r0,(!gsu_in_38-!gsu_ram)
    to r8
    add #8
    lms r0,(!gsu_in_3a-!gsu_ram)
    to r7
    sub #8
    lms r0,(!gsu_in_3c-!gsu_ram)
    to r4
    add #8
    lms r0,(!gsu_in_3e-!gsu_ram)
    to r5
    sub #8
    iwt r6,#$0000
    lm r1,(!gsu_record_247a)
    ibt r0,#$4A
    add r1
    ldw (r0)
    moves r0,r0
    bmi .loop_setup
    nop
    add r0
    add r0
    add r0
    move r6,r0
.loop_setup:
    iwt r3,#$2688
    iwt r12,#$0010
    move r13,r15
    ldb (r3)
    lob
    moves r0,r0
    beq .free
    nop
    dec r0
    stb (r3)
    inc r3
    to r2
    ldb (r3)
    inc r3
    lms r9,(!gsu_in_34-!gsu_ram)
    to r1
    ldw (r3)
    inc r3
    inc r3
    move r0,r2                  ; then LOB of it, not used
    lob
    to r2
    ldw (r3)
    inc r3
    inc r3
    move r0,r1
    cmp r8
    bmi .outside
    nop
    cmp r7
    bpl .outside
    nop
    move r0,r2
    cmp r4
    bmi .outside
    nop
    cmp r5
    bmi .next
    nop
.outside:
    with r3
    sub #6
    sub r0
    stw (r3)
    with r3
    add #6
.next:
    loop
    nop
    bra .done
    nop
.free:
    with r3
    add #6
    loop
    nop
.done:
    move r15,r11
    nop

arch 65816
