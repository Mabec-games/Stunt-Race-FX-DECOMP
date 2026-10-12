; table.asm: the Super FX's side of the 8-byte entry table whose address is
; the word at $70:1092 (the S-CPU's side is in gsu/params.asm).
;
; Entries are counted from 1; the words of an entry start 2 bytes into it.
; C: src/stuntracefx/render/table.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"

arch superfx

; @routine 04:CD83 FxTableEntryWords
; WHAT IT DOES: Returns the address of the words of entry R0 of the table
; whose address is the word at $1092: the Super FX side of
; GsuTableEntryTo1F4C.
; HOW: (entry - 1) * 8 with LMULT (R6 = 8; R4 takes the low word of the
; product), then the table address + 2 + that.
; Entry: R0 = entry; R11 = return address. Exit: R5 = the address; R0 and
; R1 = the entry; R4 = (entry - 1) * 8; R6 = 8; the last RAM address $1092;
; OV, S, CY, Z from the final add (table + 2, plus R4).
%at($04CD83)
FxTableEntryWords:
    move r1,r0
    dec r0
    iwt r6,#$0008
    lmult
    lm r0,(!gsu_table_addr)
    add #2
    to r5
    add r4
    move r0,r1
    move r15,r11
    nop

arch 65816
