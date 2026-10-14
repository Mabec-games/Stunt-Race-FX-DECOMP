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

; @routine 08:B893 Set7EFFA6Sequence
; WHAT IT DOES: Stores the words 1, 2, 3, 4 at $7E:FFA6-$7E:FFAD.
; HOW: Four 16-bit long stores.
; Entry: any widths. Exit: A = 4 (16-bit); P unchanged.
%at($08B893)
Set7EFFA6Sequence:
    php
    rep.b #$20
    lda.w #$0001
    sta.l !wram_7EFFA6
    lda.w #$0002
    sta.l !wram_7EFFA6+2
    lda.w #$0003
    sta.l !wram_7EFFA6+4
    lda.w #$0004
    sta.l !wram_7EFFA6+6
    plp
    rtl

; @routine 02:D53D WordCopyStep
; WHAT IT DOES: While the count at $0CF6 is non-zero, copies one word from
; $02:D65A + $0CF2 to $07A2 + $0CF6 and steps both offsets down by 2. No
; script has reached the copy yet: $0CF6 has always been 0.
; HOW: 16-bit offsets and stores through the data bank; the source is a
; long read, and each offset is stepped with two DECs.
; Entry: any widths. Exit: when $0CF6 is 0, Y = 0 and A unchanged; else A
; = the word copied, X and Y the offsets before the step; M 16-bit, X
; 16-bit; N, Z from the last step (Z set when there was nothing to copy).
%at($02D53D)
WordCopyStep:
    rep.b #$30
    ldy.w !copy_count
    bne .copy
    rtl
.copy:                              ; not reached in the traces so far
    ldx.w !copy_src_pos
    ldy.w !copy_count
    lda.l !copy_src,x
    sta.w !copy_dest,y
    dec.w !copy_src_pos
    dec.w !copy_src_pos
    dec.w !copy_count
    dec.w !copy_count
    rtl

; @routine 03:8648 DemoRunSecond
; WHAT IT DOES: Runs the second of the attract demos: $1A57 = $6B (its
; offset in the demo tables), then the shared demo run at $03:8683.
; HOW: A tail JMP into the demo run, reached as original code
; (jump_routine).
; Entry: D = $0000. Exit: as the demo run leaves it.
%at($038648)
DemoRunSecond:
    rep.b #$30
    lda.w #$006B
    sta.w !seq_stage+34
    jmp.w DemoRunBody

; @routine 03:8653 DemoRunFourth
; WHAT IT DOES: Runs the third demo of the title screen's cycle: $1A57 = $FC
; (its offset in the demo tables), then the shared demo run at $03:8683.
; HOW: A tail JMP into the demo run, reached as original code
; (jump_routine).
; Entry: D = $0000. Exit: as the demo run leaves it.
%at($038653)
DemoRunFourth:
    rep.b #$30
    lda.w #$00FC
    sta.w !seq_stage+34
    jmp.w DemoRunBody

; @routine 03:865E DemoRunThird
; WHAT IT DOES: Runs the third of the attract demos: $1A57 = $159, then the
; shared demo run at $03:8683; the code after the call (more demos, at
; $03:866A) has not been reached in the traces so far.
; HOW: A JSL to the demo run, then on into $03:866A as original code.
; Entry: D = $0000. Exit: as the demo run leaves it.
%at($03865E)
DemoRunThird:
    rep.b #$30
    lda.w #$0159
    sta.w !seq_stage+34
    jsl.l DemoRunBody

; @routine 03:8CF2 WramFillZeroLong
; WHAT IT DOES: WramFillZero for callers in other banks: clears Y bytes of
; work RAM from bank A, address X.
; HOW: A near call, then a long return.
; Entry: as WramFillZero. Exit: as WramFillZero.
%at($038CF2)
WramFillZeroLong:
    jsr.w WramFillZero
    rtl

; @routine 03:8CF6 WramFillZero
; WHAT IT DOES: Clears Y bytes of work RAM from bank A, address X.
; HOW: DMA channel 0, fixed source (the zero byte at $03:8D26), to WMDATA
; ($2180) with the work RAM address set at $2181-$2183; starts it.
; Entry: M 8-bit, X 16-bit; A = bank, X = address, Y = byte count (0 means
; 65536). Exit: A low byte = 1; X = $8008; N, Z clear.
%at($038CF6)
WramFillZero:
    stx.w !WMADDL
    sta.w !WMADDH
    sty.w !DAS0L
    ldx.w #!zero_byte_038d26
    stx.w !A1T0L
    lda.b #!zero_byte_038d26>>16
    sta.w !A1B0
    ldx.w #$8008
    stx.w !DMAP0
    lda.b #$01
    sta.w !MDMAEN
    rts

; @routine 03:863D DemoRunFirst
; WHAT IT DOES: Runs the first of the attract demos: $1A57 = 0 (its offset
; in the demo tables at $03:8004), then the shared demo run at $03:8683
; (course, settings and the race itself).
; HOW: A tail JMP into the demo runs of the routine at $03:8648, reached
; as original code (jump_routine).
; Entry: D = $0000. Exit: as the demo run leaves it.
%at($03863D)
DemoRunFirst:
    rep.b #$30
    lda.w #$0000
    sta.w !seq_stage+34
    jmp.w DemoRunBody

