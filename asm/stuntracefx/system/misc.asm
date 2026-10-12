; misc.asm: small routines of the frame code that have no other home yet.
; Run from work RAM. C: src/stuntracefx/system/misc.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 7E:E18B DataBankTest
; WHAT IT DOES: Leaves every register as it was except N and Z, which it sets
; from the data bank.
; HOW: Pushes DB and PB, pulls PB into DB and then the old DB back.
; Entry: any widths. Exit: DB unchanged; N, Z from DB.
%ramcode($7EE18B, $013EB2)
DataBankTest:
    phb
    phk
    plb
    plb
    rtl

; @routine 7E:F217 KeepRegisters
; WHAT IT DOES: Nothing: it saves P, changes the widths and restores P.
; HOW: PHP, SEP #$20, REP #$30, PLP.
; Entry: any widths. Exit: every register unchanged.
%ramcode($7EF217, $014F3E)
KeepRegisters:
    php
    sep.b #$20
    rep.b #$30
    plp
    rtl

; @routine 03:F329 ReturnOnly
; WHAT IT DOES: Nothing: a lone RTS that callers use as an empty handler.
; HOW: RTS.
; Entry: any widths. Exit: every register unchanged.
%at($03F329)
ReturnOnly:
    rts

; @routine 03:AEEE ReadTable9022
; WHAT IT DOES: Reads 128 words' low bytes from the table at $9022 in the
; data bank and keeps none of them; it changes only registers and $64.
; HOW: A loop of 128 loads stepping Y by 2, X by 1, counting down $64.
; Entry: any widths, D = $0000. Exit: A low byte = the last byte read; X =
; $80, Y = 0; $64 = 0; P unchanged.
%at($03AEEE)
ReadTable9022:
    php
    sep.b #$30
    ldy.b #$00
    ldx.b #$00
    lda.b #$80
    sta.b !wram_0064
.lAEF9:
    lda.w !table_9022,y
    inx
    iny
    iny
    dec.b !wram_0064
    bne .lAEF9
    plp
    rtl
