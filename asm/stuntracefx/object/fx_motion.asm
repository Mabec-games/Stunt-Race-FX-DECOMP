; fx_motion.asm: the Super FX moves an object's record by its velocity.
;
; A record (R8) holds a velocity at +$2E/+$30/+$32 and the position it
; moves at +$1E/+$20/+$22; an object of kind 6 (R1) also has a second pair,
; a rate at +$34/+$36/+$38 for the three words at +$24/+$26/+$28. Bit 3 of
; +$1C halves every step.
; C: src/stuntracefx/object/fx_motion.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"

arch superfx

; One word: the rate at R5, halved by the DIV2s given, added to the word at
; R2; both step on.
macro fx_move_word(div2s)
    ldw (r5)
    inc r5
    inc r5
    rep <div2s> : div2
    to r3
    ldw (r2)
    add r3
    stw (r2)
    inc r2
    inc r2
endmacro

; @routine 04:95BE FxRecordMove
; WHAT IT DOES: Adds record R8's velocity / 8 to its position (/ 16 when
; bit 3 of +$1C is set); for kind 6 (R1) also adds the rate at +$34 to the
; words at +$24 (halved when the bit is set).
; HOW: R5 walks the rates and R2 the words; each word is LDW, DIV2s, LDW,
; ADD, STW in that order. The halvings are DIV2 (rounding toward minus
; infinity, -1 to 0) applied once per halving.
; Entry: R1 = the object's kind; R8 = the record; R11 = return address.
; Exit: R2, R5 just past the last word and rate; R3 = the last word before
; the add; kind 6: R0 = the last new word, S, Z from R2, OV, CY from the
; add; otherwise R0 = kind - 6 and the flags from that SUB. The last RAM
; address the last word.
%at($0495BE)
FxRecordMove:
    iwt r5,#!fxrec_velocity
    with r5
    add r8
    iwt r2,#!fxrec_half_x
    with r2
    add r8
    ibt r3,#!fxrec_flags1c
    with r3
    add r8
    to r3
    ldw (r3)
    iwt r0,#$0008
    and r3
    beq .eighth
    nop
    %fx_move_word(4)
    %fx_move_word(4)
    %fx_move_word(4)
    from r1
    sub #6
    bne .done_halved
    nop
    %fx_move_word(1)
    %fx_move_word(1)
    %fx_move_word(1)
.done_halved:
    bra .done
    nop
.eighth:
    %fx_move_word(3)
    %fx_move_word(3)
    %fx_move_word(3)
    from r1
    sub #6
    bne .done
    nop
    %fx_move_word(0)
    %fx_move_word(0)
    %fx_move_word(0)
.done:
    move r15,r11
    nop

arch 65816
