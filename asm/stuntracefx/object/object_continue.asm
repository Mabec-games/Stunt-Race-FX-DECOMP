; object_continue.asm: object commands of bank $03 that go on at an address kept in the object's record (+$1A or +$1D).
; C: src/stuntracefx/object/object_continue.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 03:D25F ObjectStep39Continue
; WHAT IT DOES: Counts object X's +$39 up by one, then ObjectContinue1A.
; HOW: An 8-bit LDA / INC / STA, then a long jump to ObjectContinue1A.
; Entry: as ObjectContinue1A. Exit: as ObjectContinue1A.
%at($03D25F)
ObjectStep39Continue:
    lda.w !obj_39,x
    inc a
    sta.w !obj_39,x
    brl ObjectSet39Continue

; @routine 03:D269 ObjectSet39To0Continue
; WHAT IT DOES: Sets object X's +$39 to 0, then ObjectContinue1A.
; HOW: An 8-bit store, then a long jump to ObjectContinue1A.
; Entry: as ObjectContinue1A. Exit: as ObjectContinue1A.
%at($03D269)
ObjectSet39To0Continue:
    lda.b #$00
    sta.w !obj_39,x
    brl ObjectSet39Continue

; @routine 03:D271 ObjectSet39To1Continue
; WHAT IT DOES: Sets object X's +$39 to 1, then ObjectContinue1A.
; HOW: As ObjectSet39To0Continue.
; Entry: as ObjectContinue1A. Exit: as ObjectContinue1A.
%at($03D271)
ObjectSet39To1Continue:
    lda.b #$01
    sta.w !obj_39,x
    brl ObjectSet39Continue

; @routine 03:D279 ObjectSet39To2Continue
; WHAT IT DOES: Sets object X's +$39 to 2, then ObjectContinue1A.
; HOW: As ObjectSet39To0Continue.
; Entry: as ObjectContinue1A. Exit: as ObjectContinue1A.
%at($03D279)
ObjectSet39To2Continue:
    lda.b #$02
    sta.w !obj_39,x
    brl ObjectSet39Continue

; @routine 03:D281 ObjectSet39To3Continue
; WHAT IT DOES: Sets object X's +$39 to 3, then ObjectContinue1A.
; HOW: As ObjectSet39To0Continue.
; Entry: as ObjectContinue1A. Exit: as ObjectContinue1A.
%at($03D281)
ObjectSet39To3Continue:
    lda.b #$03
    sta.w !obj_39,x
    brl ObjectSet39Continue

; @routine 03:D289 ObjectSet39To4Continue
; WHAT IT DOES: Sets object X's +$39 to 4, then ObjectContinue1A.
; HOW: As ObjectSet39To0Continue.
; Entry: as ObjectContinue1A. Exit: as ObjectContinue1A.
%at($03D289)
ObjectSet39To4Continue:
    lda.b #$04
    sta.w !obj_39,x
    brl ObjectSet39Continue

; @routine 03:D291 ObjectSet39To5Continue
; WHAT IT DOES: Sets object X's +$39 to 5, then ObjectContinue1A.
; HOW: As ObjectSet39To0Continue.
; Entry: as ObjectContinue1A. Exit: as ObjectContinue1A.
%at($03D291)
ObjectSet39To5Continue:
    lda.b #$05
    sta.w !obj_39,x
    brl ObjectSet39Continue
ObjectSet39Continue:
    jml.l ObjectContinue1A

; @routine 03:D29D ObjectStubReturn
; WHAT IT DOES: Nothing.
; HOW: A lone RTL.
; Entry: any. Exit: everything unchanged.
%at($03D29D)
ObjectStubReturn:
    rtl

; @routine 03:D29E ObjectContinue1A
; WHAT IT DOES: Goes on at object X's long address at +$1A (bank +$1C);
; returns when both are 0.
; HOW: The bank pushed, then the address less one (16-bit), then RTL; the
; code there returns to this routine's caller. With bank and address 0
; the bank byte is pulled back and it returns.
; Entry: M 8-bit, X 16-bit; X = object. Exit: going on: the code there is
; entered with A = the address less one, M 8-bit, N, Z from A (16-bit);
; returning: A = 0, M 8-bit, Z set, N clear.
%at($03D29E)
ObjectContinue1A:
    lda.w !obj_1c,x
    pha
    rep.b #$20
    bne .lD2B0
    lda.w !obj_1a_w,x
    beq .lD2B8
    dec a
    pha
    sep.b #$20
    rtl
.lD2B0:
    lda.w !obj_1a_w,x
    dec a
    pha
    sep.b #$20
    rtl
.lD2B8:
    sep.b #$20
    pla
    rtl

; @routine 03:D2BC ObjectContinue1D
; WHAT IT DOES: As ObjectContinue1A for the long address at +$1D (bank
; +$1F).
; HOW: As ObjectContinue1A.
; Entry: as ObjectContinue1A. Exit: as ObjectContinue1A.
%at($03D2BC)
ObjectContinue1D:
    lda.w !obj_1f,x
    pha
    rep.b #$20
    bne .lD2CE
    lda.w !obj_1d,x
    beq .lD2D6
    dec a
    pha
    sep.b #$20
    rtl
.lD2CE:
    lda.w !obj_1d,x
    dec a
    pha
    sep.b #$20
    rtl
.lD2D6:
    sep.b #$20
    pla
    rtl
