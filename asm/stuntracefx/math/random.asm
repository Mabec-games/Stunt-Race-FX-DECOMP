; random.asm: the pseudo-random generator in direct page $E6-$E9.
; C: src/stuntracefx/math/random.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 02:DF7D RandomStep
; WHAT IT DOES: Steps the generator: each of $E7, $E8, $E9 becomes the
; previous result minus it, with the borrow carried along, starting from
; $E6 - $E7 - 1; finally $E6 becomes the last result minus $E6.
; HOW: A chain of 8-bit SBCs from carry clear.
; Entry: any widths, D = $0000. Exit: A low byte = the new $E6; P
; unchanged.
%at($02DF7D)
RandomStep:
    php
    sep.b #$20
    lda.b !random_e6
    clc
    sbc.b !random_e7
    sta.b !random_e7
    sbc.b !random_e8
    sta.b !random_e8
    sbc.b !random_e9
    sta.b !random_e9
    sbc.b !random_e6
    sta.b !random_e6
    plp
    rts

; @routine 07:D9D1 RandomWordStep
; WHAT IT DOES: Steps the 16-bit generator at $0D18 and returns the new
; value. Called once per object update while racing.
; HOW: The low byte is XORed into the high byte and the bytes swapped (lo'
; = hi ^ lo, hi' = lo). That word is XORed with the old low byte shifted
; left once, shifted right one bit, and XORed with $FF80; when the bit
; shifted out is set it is also XORed with $8180. A result of $AA55 with
; that bit clear stores 0 instead.
; Entry: any widths; DB a bank where $0D18 is work RAM. Exit: A = the
; result ($AA55 when 0 was stored); P unchanged.
%at($07D9D1)
RandomWordStep:
    php
    rep.b #$20
    lda.w !random_word
    and.w #$00FF
    xba
    eor.w !random_word
    xba
    sta.w !random_word
    xba
    and.w #$00FF
    asl a
    eor.w !random_word
    lsr a
    eor.w #$FF80
    bcc .even
    eor.w #$8180
.store:
    sta.w !random_word
    plp
    rtl
.even:
    cmp.w #$AA55
    bne .store
    stz.w !random_word
    plp
    rtl
