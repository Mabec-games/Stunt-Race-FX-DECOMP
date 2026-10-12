; misc.asm: small routines of the frame code that have no other home yet.
; Run from work RAM. C: src/stuntracefx/system/misc.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"

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
