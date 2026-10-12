; gradient.asm: the colour gradient drawn by HDMA. Channel 2 writes CGADD
; once a line from the table at $7F:3497 (a line count and a colour number
; per entry); channel 3 writes CGDATA, indirect, from the table at
; $7F:3489, whose entries are a line count and the address in bank $7F of
; the colours for those lines. Moving the addresses at $7F:348D and
; $7F:3493 (copied from $0D96 and $0D98) scrolls the gradient.
; C: src/stuntracefx/video/gradient.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 03:ED7F GradientFillCgadd
; WHAT IT DOES: Fills Y entries of the CGADD table at $7F:3497 + X with A.
; HOW: 16-bit stores, X stepping by 2.
; Entry: M 16-bit, X 16-bit; Y = count (0 means 65536). Exit: X past the
; last entry; Y = 0; Z set, N clear.
%at($03ED7F)
GradientFillCgadd:
    sta.l !gradient_cgadd,x
    inx
    inx
    dey
    bne GradientFillCgadd
    rts

; @routine 0B:FB1C GradientFillCgaddB
; WHAT IT DOES: The same fill as GradientFillCgadd, in bank $0B.
; HOW: As GradientFillCgadd.
; Entry: as GradientFillCgadd. Exit: as GradientFillCgadd.
%at($0BFB1C)
GradientFillCgaddB:
    sta.l !gradient_cgadd,x
    inx
    inx
    dey
    bne GradientFillCgaddB
    rts

; @routine 7E:B2A5 GradientPointersToBank7F
; WHAT IT DOES: Copies the two colour addresses at $0D96 and $0D98 into the
; CGDATA table's entries ($7F:348D, $7F:3493), scrolling the gradient.
; HOW: Two 16-bit copies.
; Entry: any widths, DB a bank where $0D96 is work RAM. Exit: A = the second
; address; N, Z from it; M 16-bit.
%ramcode($7EB2A5, $010FCC)
GradientPointersToBank7F:
    rep.b #$20
    lda.w !gradient_src_a
    sta.l !gradient_ptr_a
    lda.w !gradient_src_b
    sta.l !gradient_ptr_b
    rts

; @routine 0B:FE12 GradientSetup
; WHAT IT DOES: Sets up the gradient: HDMA channel 2 to CGADD from
; $7F:3497 and channel 3 to CGDATA (indirect, mode 2) from $7F:3489; fills
; the CGADD table ($17 lines of colour 1, $50 single lines of colour 1,
; then $18 lines and $50 more, then the end) and the CGDATA table (entries
; of $17, $50, $18 and $50 lines with colour addresses $EC57, $EC57, $EC57
; and $ED57 in bank $7F). Then
; it reads the course's 32-byte block (pointer table $03:F6F1) and 32 bytes
; at $1F:A800 without using them.
; HOW: Channel 3's mode, B-bus register and bank are stored as words, so
; $4331, $4332 and $4335 get a zero byte first; the table is written in the
; original's order, including two addresses later overwritten.
; Entry: any widths, D = $0000. Exit: A = the high byte of the course's
; block address and the last byte read; X = Y = $20; P unchanged.
%at($0BFE12)
GradientSetup:
    php
    sep.b #$20
    rep.b #$10
    lda.b #$00
    sta.w !DMAP2
    lda.b #$21
    sta.w !BBAD2
    lda.b #$7F
    sta.w !A1B2
    rep.b #$20
    lda.w #$3497
    sta.w !A1T2L
    lda.w #$0042
    sta.w !DMAP3
    lda.w #$0022
    sta.w !BBAD3
    lda.w #$007F
    sta.w !A1B3
    rep.b #$20
    lda.w #$3489
    sta.w !A1T3L
    rep.b #$30
    ldx.w #$0002
    lda.w #$0117
    sta.l !gradient_cgadd
    ldy.w #$0050
    lda.w #$0181
    jsr.w GradientFillCgaddB
    lda.w #$0118
    sta.l !gradient_cgadd,x
    inx
    inx
    ldy.w #$0050
    lda.w #$0181
    jsr.w GradientFillCgaddB
    lda.w #$0000
    sta.l !gradient_cgadd,x
    lda.w #$EC57
    sta.l !gradient_cgdata+1
    lda.w #$ECD7
    sta.l !gradient_ptr_a
    lda.w #$EC57
    sta.l !gradient_cgdata+7
    lda.w #$ECD7
    sta.l !gradient_ptr_b
    lda.w #$EC57
    sta.l !gradient_ptr_a
    lda.w #$ED57
    sta.l !gradient_ptr_b
    sep.b #$20
    lda.b #$17
    sta.l !gradient_cgdata
    lda.b #$D0
    sta.l !gradient_cgdata+3
    lda.b #$18
    sta.l !gradient_cgdata+6
    lda.b #$D0
    sta.l !gradient_cgdata+9
    lda.b #$00
    sta.l !gradient_cgdata+11
    lda.b #$7F
    sta.w !DASB3
    sep.b #$20
    rep.b #$10
    phb
    lda.w !course
    asl a
    clc
    adc.w !course
    rep.b #$20
    and.w #$00FF
    tax
    sep.b #$20
    lda.l !course_block_ptrs+2,x
    pha
    plb
    rep.b #$20
    lda.l !course_block_ptrs,x
    tax
    sep.b #$20
    ldy.w #$0000
.lFEEC:
    lda.w 0,x
    inx
    iny
    cpy.w #$0020
    bne .lFEEC
    plb
    ldx.w #$0000
.lFEFA:
    lda.l !rom_1fa800,x
    inx
    cpx.w #$0020
    bne .lFEFA
    plp
    rts
