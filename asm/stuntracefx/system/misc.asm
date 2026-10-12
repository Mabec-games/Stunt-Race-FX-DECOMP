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

; @routine 02:FC44 Set0D37FromTable
; WHAT IT DOES: Sets $0D37 to the word table $02:FC58 entry for the byte at
; $0D48.
; HOW: 16-bit lookup; work RAM through the data bank.
; Entry: any widths. Exit: A = the entry; X = the byte * 2 (low byte only
; when X is 8-bit); P unchanged.
%at($02FC44)
Set0D37FromTable:
    php
    rep.b #$30
    lda.w !wram_0D48
    and.w #$00FF
    asl a
    tax
    lda.l !table_02fc58,x
    sta.w !wram_0D37
    plp
    rts

; @routine 02:D65A Set7EBC2BTrio
; WHAT IT DOES: Stores A's low byte at $7E:BC2B, $7E:BC2D and $7E:BC2F.
; HOW: Three long byte stores.
; Entry: any widths. Exit: every register unchanged.
%at($02D65A)
Set7EBC2BTrio:
    php
    sep.b #$20
    sta.l !wram_7EBC2B
    sta.l !wram_7EBC2D
    sta.l !wram_7EBC2F
    plp
    rtl

; @routine 07:EE11 Countdown0DF8
; WHAT IT DOES: Counts $0DF8 down by one; once it is zero, instead counts
; $0E5C down and clears $1160.
; HOW: 8-bit memory decrements through the data bank.
; Entry: any widths. Exit: A low byte = $0DF8's value before; P unchanged.
%at($07EE11)
Countdown0DF8:
    php
    sep.b #$20
    lda.w !wram_0DF8
    bne .lEE21
    dec.w !wram_0E5C
    stz.w !wram_1160
    plp
    rts
.lEE21:
    dec.w !wram_0DF8
    plp
    rts
