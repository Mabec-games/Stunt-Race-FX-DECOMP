; palette.asm: colours the raster interrupt writes straight to CGRAM each
; frame, and the colour cycling it steps. Runs from work RAM.
; C: src/stuntracefx/video/palette.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; The three cycling colours' tables (eight colours each), in the frame code's
; work-RAM copy just before RasterPaletteUpload.
!cycle_colours_b = $7EB6FB
!cycle_colours_c = $7EB70B
!cycle_colours_a = $7EB71B

; @routine 7E:B31E RasterColoursCycling
; WHAT IT DOES: Writes the animated colours $E1-$E3, then two cycling colours
; for this frame: $10-$11 on even frames, $12-$13 on odd ones, from the buffer
; at $7E:FD85 + X.
; HOW: Six bytes at colour $E1 from $7E:FF27, then four bytes at colour $10
; (or $12) from $7E:FD85+X (or $7E:FD89+X).
; Entry: M 8-bit, X 16-bit. Exit: A low byte = the last byte written; N, Z
; from it.
%ramcode($7EB31E, $011045)
RasterColoursCycling:
    sep.b #$20
    lda.b #$E1
    sta.w !CGADD
    lda.l !palette_kept
    sta.w !CGDATA
    lda.l !palette_kept+1
    sta.w !CGDATA
    lda.l !palette_kept+2
    sta.w !CGDATA
    lda.l !palette_kept+3
    sta.w !CGDATA
    lda.l !palette_kept+4
    sta.w !CGDATA
    lda.l !palette_kept+5
    sta.w !CGDATA
    lda.w !frame_count
    bit.b #$01
    bne .odd
    lda.b #$10
    sta.w !CGADD
    lda.l !palette_buffer+32,x
    sta.w !CGDATA
    lda.l !palette_buffer+32+1,x
    sta.w !CGDATA
    lda.l !palette_buffer+32+2,x
    sta.w !CGDATA
    lda.l !palette_buffer+32+3,x
    sta.w !CGDATA
    rts
.odd:
    lda.b #$12
    sta.w !CGADD
    lda.l !palette_buffer+36,x
    sta.w !CGDATA
    lda.l !palette_buffer+36+1,x
    sta.w !CGDATA
    lda.l !palette_buffer+36+2,x
    sta.w !CGDATA
    lda.l !palette_buffer+36+3,x
    sta.w !CGDATA
    rts

; @routine 7E:B39A RasterColoursE4
; WHAT IT DOES: Writes six colours ($E4-$E9) from the buffer at $7E:FF2D + X.
; HOW: Twelve bytes to CGRAM from colour $E4.
; Entry: M 8-bit, X 16-bit. Exit: A low byte = the last byte written; N, Z
; from it.
RasterColoursE4:
    lda.b #$E4
    sta.w !CGADD
    lda.l !palette_kept+6,x
    sta.w !CGDATA
    lda.l !palette_kept+6+1,x
    sta.w !CGDATA
    lda.l !palette_kept+6+2,x
    sta.w !CGDATA
    lda.l !palette_kept+6+3,x
    sta.w !CGDATA
    lda.l !palette_kept+6+4,x
    sta.w !CGDATA
    lda.l !palette_kept+6+5,x
    sta.w !CGDATA
    lda.l !palette_kept+6+6,x
    sta.w !CGDATA
    lda.l !palette_kept+6+7,x
    sta.w !CGDATA
    lda.l !palette_kept+6+8,x
    sta.w !CGDATA
    lda.l !palette_kept+6+9,x
    sta.w !CGDATA
    lda.l !palette_kept+6+10,x
    sta.w !CGDATA
    lda.l !palette_kept+6+11,x
    sta.w !CGDATA
    rts

; @routine 7E:B63A PaletteCycle
; WHAT IT DOES: Steps the palette's cycling colours: two on even frames (into
; $7E:FD85 and $7E:FD87), one on odd frames (into $7E:FD89).
; HOW: Each cycling colour has a phase 0-15 and a table of eight colours; the
; phase picks the colour, running back down the table for phases 8-15, then
; steps and wraps at 16. The data bank is $7E inside.
; Entry: M 16-bit. Exit: A = the last phase stored; X = the last table offset;
; Y = 0; M 16-bit; N, Z from the data bank pulled back; C from the wrap test.
%ramcode($7EB63A, $011361)
PaletteCycle:
    rep.b #$10
    ldy.w #$0000
    sep.b #$20
    phb
    lda.b #$7E
    pha
    plb
    lda.l !frame_count
    bit.b #$01
    bne .odd
    rep.b #$20
    lda.w !cycle_phase_a
    cmp.w #$0008
    bcc .a_index
    lda.w #$000F
    sec
    sbc.w !cycle_phase_a
.a_index:
    asl a
    tax
    lda.l !cycle_colours_a,x
    sta.w !cycle_colour_a,y
    lda.w !cycle_phase_a
    inc a
    cmp.w #$0010
    bcc .a_store
    lda.w #$0000
.a_store:
    sta.w !cycle_phase_a
    lda.w !cycle_phase_b
    cmp.w #$0008
    bcc .b_index
    lda.w #$000F
    sec
    sbc.w !cycle_phase_b
.b_index:
    asl a
    tax
    lda.l !cycle_colours_b,x
    sta.w !cycle_colour_b,y
    lda.w !cycle_phase_b
    inc a
    cmp.w #$0010
    bcc .b_store
    lda.w #$0000
.b_store:
    sta.w !cycle_phase_b
    plb
    rts
.odd:
    rep.b #$20
    lda.w !cycle_phase_c
    cmp.w #$0008
    bcc .c_index
    lda.w #$000F
    sec
    sbc.w !cycle_phase_c
.c_index:
    asl a
    tax
    lda.l !cycle_colours_c,x
    sta.w !cycle_colour_c,y
    lda.w !cycle_phase_c
    inc a
    cmp.w #$0010
    bcc .c_store
    lda.w #$0000
.c_store:
    sta.w !cycle_phase_c
    plb
    rts


; @routine 7E:B72B RasterPaletteUpload
; WHAT IT DOES: When the animated colours are in use, copies colours 0-$2F
; and $B0-$BF from the palette buffer to CGRAM and then writes the animated
; colours (falls into RasterColours); otherwise writes colour 0 only.
; HOW: Two CgramDma transfers: $60 bytes from $7E:FD65 at colour 0, $20 bytes
; from $7E:FEC5 at colour $B0.
; Entry: M 8-bit, X 8-bit. Exit: A low byte = the last byte written; N, Z from
; it; when animating X = $B0, Y = $20, M and X 8-bit.
%ramcode($7EB72B, $011452)
RasterPaletteUpload:
    sep.b #$20
    lda.w !palette_animate
    beq RasterColours_colour0
    rep.b #$30
    lda.w #!palette_buffer
    ldy.w #$0060
    ldx.w #$0000
    jsr.w CgramDma
    sep.b #$20
    lda.w !palette_animate
    beq RasterColours_colour0
    rep.b #$30
    lda.w #!palette_buffer+352
    ldy.w #$0020
    ldx.w #$00B0
    jsr.w CgramDma

; @routine 7E:B755 RasterColours
; WHAT IT DOES: Writes the animated colours $E1-$E3 when they are in use,
; otherwise colour 0, from the palette buffer to CGRAM.
; HOW: With $0E5F set, six bytes from $7E:FF27 at colour $E1; else two bytes
; from $7E:FD65 at colour 0.
; Entry: any widths. Exit: A low byte = the last byte written; N, Z from it;
; M and X 8-bit.
RasterColours:
    sep.b #$30
    lda.w !palette_animate
    beq RasterColours_colour0
    lda.b #$E1
    sta.w !CGADD
    lda.l !palette_kept
    sta.w !CGDATA
    lda.l !palette_kept+1
    sta.w !CGDATA
    lda.l !palette_kept+2
    sta.w !CGDATA
    lda.l !palette_kept+3
    sta.w !CGDATA
    lda.l !palette_kept+4
    sta.w !CGDATA
    lda.l !palette_kept+5
    sta.w !CGDATA
    rts
RasterColours_colour0:
    lda.b #$00
    sta.w !CGADD
    lda.l !palette_buffer
    sta.w !CGDATA
    lda.l !palette_buffer+1
    sta.w !CGDATA
    rts

; @routine 03:EDF4 PaletteCourseLoad
; WHAT IT DOES: Loads the course's palettes: colours 16-255 of the first two
; work copies and the palette buffer from the course palette, colours 16-255
; of the third copy from its own palette, colours 0-15 from the course block
; (PaletteLoad16) and the third copy's 0-15 from the second colour block
; (PaletteCopy16To7F39D7). Then, colour by colour from 255 down, the third
; copy is matched to the second where the bit table at $03:EF1F has the
; colour's bit set and kept one step off it where clear (an equal colour
; goes one down, or one up when its red is 0). Sets the six blended colours
; at $0E21-$0E2B; with $10DC set, clears colours $B0-$BF of all four copies.
; HOW: The pointer tables ($03:F649, $03:F69D, $03:F6F1, $03:F745) give each
; source's bank, which becomes the data bank while it is read. The table's
; word for 16 colours is shifted right by the colour's place in it, counted
; in direct page $F1. The $10DC case is not reached in the traces so far.
; Entry: M 16-bit, X 16-bit, D = $0000; DB = the bit table's bank ($03).
; Exit: A = $10DC's word (0); X = $FFFE; Y = 0; $F1 = 0; M 16-bit; Z set, N
; clear; C from the last compare of colour 0; V from the course index add;
; DB unchanged. (With $10DC set: A low byte 0, X = $20, M 8-bit, Z, C set.)
%at($03EDF4)
PaletteCourseLoad:
    rep.b #$30
    lda.w !course
    asl a
    clc
    adc.w !course
    and.w #$00FF
    tax
    phx
    sep.b #$20
    lda.l !course_palette_ptrs+2,x
    phb
    pha
    rep.b #$20
    lda.l !course_palette_ptrs,x
    plb
    tay
    ldx.w #$0020
.copies:
    lda.w !course_block_colours,y
    sta.l !palette_work_a,x
    sta.l !palette_work_b,x
    sta.l !palette_buffer,x
    inx
    inx
    iny
    iny
    cpx.w #$0200
    bcc .copies
    sep.b #$20
    plb
    plx
    phx
    lda.l !course_palette2_ptrs+2,x
    phb
    pha
    plb
    rep.b #$20
    lda.l !course_palette2_ptrs,x
    tay
    ldx.w #$0020
.third:
    lda.w !course_block_colours,y
    sta.l !palette_work_c,x
    inx
    inx
    iny
    iny
    cpx.w #$0200
    bcc .third
    sep.b #$20
    plb
    sep.b #$20
    plx
    phx
    lda.l !course_block_ptrs+2,x
    phb
    pha
    plb
    rep.b #$20
    lda.l !course_block_ptrs,x
    jsr.w PaletteLoad16
    sep.b #$20
    plb
    plx
    lda.l !course_block2_ptrs+2,x
    phb
    pha
    plb
    rep.b #$20
    lda.l !course_block2_ptrs,x
    jsr.w PaletteCopy16To7F39D7
    plb
    rep.b #$20
    ldx.w #$01FE
.colour:
    txa
    lsr a
    lsr a
    lsr a
    lsr a
    and.w #$001E
    tay
    txa
    lsr a
    and.w #$000F
    sta.b !palette_bits
    cmp.w #$0000
    bne .shift
    lda.w !palette_differ_bits,y
    bra .test
.shift:
    lda.w !palette_differ_bits,y
.shift_one:
    lsr a
    dec.b !palette_bits
    bne .shift_one
.test:
    bit.w #$0001
    bne .match
    lda.l !palette_work_b,x
    cmp.l !palette_work_c,x
    bne .next
    pha
    and.w #$001F
    beq .up
    pla
    dec a
    bra .store
.up:
    pla
    inc a
    bra .store
.match:
    lda.l !palette_work_b,x
    cmp.l !palette_work_c,x
    beq .next
.store:
    sta.l !palette_work_c,x
.next:
    dex
    dex
    bpl .colour
    lda.w #$45CC
    sta.w !blend_set_a
    lda.w #$2528
    sta.w !blend_set_a+2
    lda.w #$1CE5
    sta.w !blend_set_a+4
    lda.w #$739A
    sta.w !blend_set_a+6
    lda.w #$1CE5
    sta.w !blend_set_a+8
    lda.w #$2528
    sta.w !blend_set_a+10
    lda.w !wram_10DC
    beq .done
    sep.b #$20                      ; not reached in the traces so far
    rep.b #$10
    lda.b #$00
    ldx.w #$0000
.clear:
    sta.l !palette_buffer+$160,x
    sta.l !palette_work_b+$160,x
    sta.l !palette_work_c+$160,x
    sta.l !palette_work_a+$160,x
    inx
    cpx.w #$0020
    bne .clear
.done:
    rts

; @routine 03:EDA5 PaletteLoad16
; WHAT IT DOES: Loads 16 colours from address A in the data bank into both
; palette work copies in bank $7F ($7F:35D7, $7F:37D7) and the palette
; buffer ($7E:FD65), from colour 0.
; HOW: A word loop counted in direct page $F1; each word is stored to the
; three copies in that order.
; Entry: M 16-bit, X 16-bit, D = $0000; A = source. Exit: A = the last
; colour; X = $20; Y = source + $20; $F1 = 0; Z set, N clear.
%at($03EDA5)
PaletteLoad16:
    tay
    ldx.w #$0000
    lda.w #$0010
    sta.b !digit_count
.lEDAE:
    lda.w 0,y
    sta.l !palette_work_a,x
    sta.l !palette_work_b,x
    sta.l !palette_buffer,x
    inx
    inx
    iny
    iny
    dec.b !digit_count
    bne .lEDAE
    rts

; @routine 7E:B307 RasterColoursUnless0D22
; WHAT IT DOES: Unless the byte at $0D22 is set, writes the raster colours
; from the start of their buffers: RasterColoursE4, then RasterColoursCycling.
; HOW: X = 0 for both; $0D22 is read through the data bank.
; Entry: any widths. Exit: skipped: A low byte = $0D22. Written: A low byte
; as RasterColoursCycling leaves it, X = 0. P unchanged.
%ramcode($7EB307, $01102E)
RasterColoursUnless0D22:
    php
    sep.b #$20
    rep.b #$10
    lda.w !wram_0D22
    bne .lB31C
    ldx.w #$0000
    sep.b #$20
    jsr.w RasterColoursE4
    jsr.w RasterColoursCycling
.lB31C:
    plp
    rts

; @routine 04:E1FE PaletteDarkenFour
; WHAT IT DOES: Stores A at $7F:345C and sets colours 16-19 of the third
; palette copy ($7F:39D7) to those of the second ($7F:37D7) minus one.
; HOW: Four 16-bit decrements, long addressing.
; Entry: M 16-bit; A = value. Exit: A = the last colour written; N, Z from
; it.
%at($04E1FE)
PaletteDarkenFour:
    sta.l !wram_7F345C
    lda.l !palette_work_b+32
    dec a
    sta.l !palette_work_c+32
    lda.l !palette_work_b+34
    dec a
    sta.l !palette_work_c+34
    lda.l !palette_work_b+36
    dec a
    sta.l !palette_work_c+36
    lda.l !palette_work_b+38
    dec a
    sta.l !palette_work_c+38
    rtl

; @routine 03:EDC6 PaletteCopy16To7F39D7
; WHAT IT DOES: Copies 16 colours from address A in the data bank to the
; third palette copy at $7F:39D7.
; HOW: A word loop counted in direct page $F1.
; Entry: M 16-bit, X 16-bit, D = $0000; A = source. Exit: A = the last word;
; X = $20; Y = source + $20; $F1 = 0; Z set, N clear.
%at($03EDC6)
PaletteCopy16To7F39D7:
    tay
    ldx.w #$0000
    lda.w #$0010
    sta.b !digit_count
.lEDCF:
    lda.w 0,y
    sta.l !palette_work_c,x
    inx
    inx
    iny
    iny
    dec.b !digit_count
    bne .lEDCF
    rts

; @routine 7E:E6E9 ColourBlend
; WHAT IT DOES: Blends two BGR555 colours, from direct page $F1 toward $F3,
; by the fraction loaded in the PPU multiplier (factor t * 8 gives t / 32),
; and returns the blend.
; HOW: Splits both colours into 5-bit components at $344E-$3458 (from: red,
; green, blue at $344E, $3452, $3456; to at $3450, $3454, $3458), shifting
; each colour word right 5 at a time in place; each component becomes from
; + PpuMultiply(to - from), red masked to 5 bits, and the three are packed
; back; $F1 is left with the blue field. The components go through the
; data bank.
; Entry: M 16-bit, D = $0000; the multiplier's factor set. Exit: A = the
; blended colour; N, Z from it; C = bit 11 of the green sum; V from the
; blue add; X, Y unchanged.
%ramcode($7EE6E9, $014410)
ColourBlend:
    lda.b !blend_from
    and.w #$001F
    lsr.b !blend_from
    lsr.b !blend_from
    lsr.b !blend_from
    lsr.b !blend_from
    lsr.b !blend_from
    sta.w !colour_from_r
    lda.b !blend_from
    and.w #$001F
    lsr.b !blend_from
    lsr.b !blend_from
    lsr.b !blend_from
    lsr.b !blend_from
    lsr.b !blend_from
    sta.w !colour_from_g
    lda.b !blend_from
    and.w #$001F
    lsr.b !blend_from
    lsr.b !blend_from
    lsr.b !blend_from
    lsr.b !blend_from
    lsr.b !blend_from
    sta.w !colour_from_b
    lda.b !blend_to
    and.w #$001F
    lsr.b !blend_to
    lsr.b !blend_to
    lsr.b !blend_to
    lsr.b !blend_to
    lsr.b !blend_to
    sta.w !colour_to_r
    lda.b !blend_to
    and.w #$001F
    lsr.b !blend_to
    lsr.b !blend_to
    lsr.b !blend_to
    lsr.b !blend_to
    lsr.b !blend_to
    sta.w !colour_to_g
    lda.b !blend_to
    and.w #$001F
    lsr.b !blend_to
    lsr.b !blend_to
    lsr.b !blend_to
    lsr.b !blend_to
    lsr.b !blend_to
    sta.w !colour_to_b
    lda.w !colour_to_r
    sec
    sbc.w !colour_from_r
    jsr.w PpuMultiply
    clc
    adc.w !colour_from_r
    and.w #$001F
    sta.w !colour_from_r
    lda.w !colour_to_g
    sec
    sbc.w !colour_from_g
    jsr.w PpuMultiply
    clc
    adc.w !colour_from_g
    sta.w !colour_from_g
    lda.w !colour_to_b
    sec
    sbc.w !colour_from_b
    jsr.w PpuMultiply
    clc
    adc.w !colour_from_b
    xba
    asl a
    asl a
    and.w #$7C00
    sta.b !blend_from
    lda.w !colour_from_g
    asl a
    asl a
    asl a
    asl a
    asl a
    and.w #$03E0
    ora.b !blend_from
    ora.w !colour_from_r
    rts

; @routine 7E:B41F PaletteKeptColoursStep
; WHAT IT DOES: Animates the kept colours at $7E:FF27-$7E:FF2C for player Y
; (the timer at $0E01 + Y and the two 13-step cycles at $0E03 + Y and $0DFF
; + Y). While the player's flag at $0E05 + Y is set, $0D53 is clear and
; bit 14 of $0D1E + X is set, the timer counts up and the third colour's
; cycle speeds up with it (every fourth frame below $3C, every other frame
; below $78, every frame below $B4, two steps a frame from there); otherwise
; the timer counts back down through the same speeds and at 0 the third
; colour rests at $7E63. The first two colours step together whenever
; $0DFD + Y, less $0DFB + Y each frame, borrows.
; HOW: The third colour from the table at $7E:B4E9, the first two from
; $7E:B505 and $7E:B521, each indexed by its cycle step * 2. Counting down
; subtracts 5, 4, 3 or 1 by band ($78, $3C, $1E). Work RAM through the data
; bank.
; Entry: M 8-bit, X 8-bit; X = offset into $0D1E, Y = player offset.
; Exit: when the first two colours stepped, A = the second colour, M 16-bit,
; N, Z from it, C clear; otherwise A low byte = $0DFD + Y, M 8-bit, N, Z
; from it, C set; V from that subtract; X = the last cycle step * 2 when a
; step was made, else unchanged; Y unchanged.
%ramcode($7EB41F, $011146)
PaletteKeptColoursStep:
    lda.w !kept_freeze
    bne .lB455
    lda.w !kept_enable,y
    beq .lB455
    rep.b #$20
    lda.w !joy1_last,x
    bit.w #$4000
    beq .lB455
    sep.b #$20
    lda.w !kept_timer,y
    cmp.b #$FF
    beq .lB484
    inc a
    sta.w !kept_timer,y
    cmp.b #$B4
    bcs .lB484
    cmp.b #$78
    bcs .lB499
    cmp.b #$3C
    bcs .lB492
.lB44C:
    lda.w !frame_count
    and.b #$03
    bne .lB4B9
    bra .lB499
.lB455:
    sep.b #$20
    lda.w !kept_timer,y
    beq .lB4B0
    cmp.b #$78
    bcs .lB47F
    cmp.b #$3C
    bcs .lB478
    cmp.b #$1E
    bcs .lB471
    lda.w !kept_timer,y
    dec a
    sta.w !kept_timer,y
    bra .lB44C
.lB471:
    sbc.b #$03
    sta.w !kept_timer,y
    bra .lB492
.lB478:
    sbc.b #$04
    sta.w !kept_timer,y
    bra .lB499
.lB47F:
    sbc.b #$05
    sta.w !kept_timer,y
.lB484:
    lda.w !kept_cycle_c,y
    clc
    adc.b #$02
    cmp.b #$0D
    bcc .lB4A3
    sbc.b #$0D
    bra .lB4A3
.lB492:
    lda.w !frame_count
    bit.b #$01
    bne .lB4B9
.lB499:
    lda.w !kept_cycle_c,y
    inc a
    cmp.b #$0D
    bcc .lB4A3
    lda.b #$00
.lB4A3:
    sta.w !kept_cycle_c,y
    asl a
    tax
    rep.b #$20
    lda.l !kept_colours_c,x
    bra .lB4B5
.lB4B0:
    rep.b #$20
    lda.w #$7E63
.lB4B5:
    sta.l !kept_colour_c
.lB4B9:
    sep.b #$20
    lda.w !kept_acc,y
    sec
    sbc.w !kept_sub,y
    sta.w !kept_acc,y
    bcs .lB4E8
    lda.w !kept_cycle_ab,y
    inc a
    cmp.b #$0D
    bcc .lB4D1
    lda.b #$00
.lB4D1:
    sta.w !kept_cycle_ab,y
    asl a
    tax
    rep.b #$20
    lda.l !kept_colours_a,x
    sta.l !palette_kept
    lda.l !kept_colours_b,x
    sta.l !kept_colour_b
.lB4E8:
    rts

; @routine 7E:E6D1 ColourBlendBy
; WHAT IT DOES: ColourBlend by the fraction in direct page $F5 (t / 32):
; blends the colour in $F1 toward the one in $F3.
; HOW: Loads the multiplier's factor with the low byte of $F5 times 8 (two
; long writes to M7A, low then high), then falls into ColourBlend.
; Entry: M 16-bit, D = $0000. Exit: as ColourBlend.
%ramcode($7EE6D1, $0143F8)
ColourBlendBy:
    lda.b !blend_amount
    xba
    and.w #$FF00
    lsr a
    lsr a
    lsr a
    lsr a
    lsr a
    sep.b #$20
    sta.l !M7A
    xba
    sta.l !M7A
    rep.b #$20

; @routine 7E:E6A2 ColourBlendToTarget
; WHAT IT DOES: Blends colour A toward the target colour at $3470 by the
; amount at $3468 and stores the result at $0E21 + X; a negative target
; leaves the colour as it is (blended toward itself).
; HOW: Direct page $F1 = A, $F3 = the target (or A), $F5 = $3468, then
; ColourBlendBy; a long store of the result. $3470 and $3468 go through
; the data bank.
; Entry: M 16-bit, X 16-bit, D = $0000; A = colour, X = offset. Exit: as
; ColourBlend.
%ramcode($7EE6A2, $0143C9)
ColourBlendToTarget:
    sta.b !blend_from
    lda.w !blend_target
    bpl .lE6AB
    lda.b !blend_from
.lE6AB:
    sta.b !blend_to
    lda.w !blend_target_amount
    sta.b !blend_amount
    jsr.w ColourBlendBy
    sta.l !blended_colours,x
    rts

; @routine 7E:B5C1 PlayerColoursFlash
; WHAT IT DOES: Writes player Y's three colours into the palette at
; $7E:01C8 + the player's offset ($0D83 + 2Y): the set at $0E21 normally;
; for a level A of 8 or more, flashing between the sets at $0E27 and $0E21
; faster the higher the level (on for n calls, off for n, where n = 8 - the
; top three bits of A + $60, the sum held at $FF; no script has reached the
; held case yet).
; HOW: The flash counter is at $0E1F + Y and runs 1 to 2n; n and 2n are
; kept in direct page $F9 and $FA; counts below n show $0E27. Work RAM
; through the data bank; the palette with long stores.
; Entry: M 8-bit, X 16-bit, D = $0000; A = level, Y = player. Exit: A = the
; third colour written; X = the player's palette offset; Y = 2Y; M 16-bit;
; N, Z from A; C = bit 15 of Y on entry; V clear after a flash step, from
; the add of $60 when only that ran.
%ramcode($7EB5C1, $0112E8)
PlayerColoursFlash:
    cmp.b #$08
    bcc .lB5CC
    clc
    adc.b #$60
    bcc .lB5CC
    lda.b #$FF                      ; not reached in the traces so far
.lB5CC:
    rol a
    rol a
    rol a
    rol a
    and.b #$07
    beq .lB5DC
    sec
    sbc.b #$08
    eor.b #$FF
    inc a
    bra .lB5E4
.lB5DC:
    lda.b #$00
    sta.w !flash_count,y
    bra .lB5FC
    rts                             ; unreachable: nothing branches here
.lB5E4:
    rep.b #$10
    sta.b !flash_n
    asl a
    sta.b !flash_2n
    lda.w !flash_count,y
    cmp.b !flash_2n
    bcc .lB5F4
    lda.b #$FF
.lB5F4:
    inc a
    sta.w !flash_count,y
    cmp.b !flash_n
    bcc .lB61B
.lB5FC:
    rep.b #$20
    tya
    asl a
    tay
    lda.w !player_pal_offset,y
    tax
    lda.w !blended_colours
    sta.l !player_palettes,x
    lda.w !blended_colours+2
    sta.l !player_palettes+2,x
    lda.w !blended_colours+4
    sta.l !player_palettes+4,x
    rts
.lB61B:
    rep.b #$20
    tya
    asl a
    tay
    lda.w !player_pal_offset,y
    tax
    lda.w !blend_set_b
    sta.l !player_palettes,x
    lda.w !blend_set_b+2
    sta.l !player_palettes+2,x
    lda.w !blend_set_b+4
    sta.l !player_palettes+4,x
    rts

; @routine 7E:B53F StatusColourPick
; WHAT IT DOES: Picks the colour for item Y and stores it at $7E:FD8B: 0
; while its timer $0DDA + Y is 0; with flag $0400 (of $0DF0/$0DF2 + Y) and
; $0E05 + Y set, entry ($0E3B & 3) of the colours at $7E:B5B9; with flag
; $0010 and the timer at $3C or more, entry ($0E3B & 3) of $7E:B5B1;
; otherwise, on even frames only, entry (($0E3B >> 1) & 3) of $7E:B5B1
; (odd frames store nothing).
; HOW: The tables are read from the code's work RAM copy. The $0400 path
; with $0E05 clear is not reached in the traces so far.
; Entry: M 8-bit, X 16-bit; Y = item. Exit: M 16-bit; A = the colour (or
; $0E3B on an odd frame, X unchanged); X = 0 after a store.
%ramcode($7EB53F, $011266)
StatusColourPick:
    lda.w !wram_0DDA,y
    beq .none
    lda.w !player_rec_43,y
    xba
    lda.w !player_rec_42,y
    rep.b #$20
    bit.w #$0400
    bne .flag400
    bit.w #$0010
    bne .flag10
    bra .even
.flag400:
    pha
    sep.b #$20
    lda.w !kept_enable,y
    bne .table_b
    rep.b #$20
    pla
    bit.w #$0010
    bne .flag10
    bra .even
.table_b:
    rep.b #$20
    pla
    lda.w !frame_count
    and.w #$0003
    asl a
    tax
    lda.l !status_colours_b,x
    bra .store
.flag10:
    sep.b #$20
    lda.w !wram_0DDA,y
    cmp.b #$3C
    bcc .even
    rep.b #$20
    lda.w !frame_count
    bra .pick
.even:
    rep.b #$20
    lda.w !frame_count
    bit.w #$0001
    bne .done
    lsr a
.pick:
    and.w #$0003
    asl a
    tax
    lda.l !status_colours_a,x
    bra .store
.none:
    rep.b #$20
    lda.w #$0000
    bra .store
.store:
    ldx.w #$0000
    sta.l !status_colour,x
.done:
    rts

; @routine 7E:B3F4 PalettesStep
; WHAT IT DOES: While $0E5F is set, steps the frame's palette effects for
; player 0: its colours (PlayerColoursFlash with A = $0DDA), its status
; colour (StatusColourPick), the cycling colours (PaletteCycle) and the
; kept colours (PaletteKeptColoursStep).
; HOW: Y = 0 for each; M 8-bit, X 16-bit, then both 8-bit for the last.
; Entry: any widths. Exit: P restored; X and Y as the last call left them.
%ramcode($7EB3F4, $01111B)
PalettesStep:
    php
    sep.b #$20
    rep.b #$10
    lda.w !palette_animate
    beq .done
    ldy.w #$0000
    lda.w !wram_0DDA
    jsr.w PlayerColoursFlash
    sep.b #$20
    rep.b #$10
    ldy.w #$0000
    jsr.w StatusColourPick
    jsr.w PaletteCycle
    sep.b #$30
    ldy.b #$00
    ldx.b #$00
    jsr.w PaletteKeptColoursStep
.done:
    plp
    rts

; @routine 7E:E149 FadeValueStep
; WHAT IT DOES: Unless bit 7 of $0DA5 is set, while bit 5 is set: steps
; the word $19FE by $1A00 until it reaches $1A02 (then clears bit 5,
; leaving $19FE as it was) and puts its high byte in $7F:3460.
; HOW: The data bank is the code's ($7E) and restored; $0DA5 is read
; long. The step is not reached in the traces so far.
; Entry: any widths. Exit: P and the data bank restored; A as the last
; step left it.
%ramcode($7EE149, $013E70)
FadeValueStep:
    php
    phb
    phk
    plb
    sep.b #$20
    lda.l !slide_flags
    and.b #$80
    bne .done
    lda.l !slide_flags
    and.b #$20
    beq .done
    rep.b #$20
    lda.w !wram_19FE
    clc
    adc.w !wram_1A00
    cmp.w !wram_1A02
    beq .end
    sta.w !wram_19FE
    jmp.w .show
.end:
    sep.b #$20
    lda.l !slide_flags
    and.b #$DF
    sta.l !slide_flags
.show:
    sep.b #$20
    lda.w !wram_19FF
    sta.l !wram_7F3460
.done:
    plb
    plp
    rtl

; @routine 02:D55F PaletteFadeStep
; WHAT IT DOES: While the fade count $0CFA is nonzero, decrements it and
; moves $1167 colours of the buffer at $0722 + $1168 one step toward the
; colours at $02:D65A + $1168: each of blue, green and red up or down by
; one where it differs.
; HOW: Each part masked ($7C00, $03E0, $001F) and compared through
; $1F2B; blue and green kept at $3E and $40, then ORed with red. The count
; kept on the stack. Runs only from unit_cpu.
; Entry: D = $0000. Exit: with $0CFA 0, M 8-bit and A's low byte 0;
; otherwise M 16-bit, X 16-bit, A = 0, X and Y past the last colour.
%at($02D55F)
PaletteFadeStep:
    sep.b #$20
    lda.w !copy_count+4
    bne .lD567
    rtl
.lD567:
    dec a                           ; not reached in the traces so far
    sta.w !copy_count+4             ; not reached in the traces so far
    rep.b #$30                      ; not reached in the traces so far
    ldx.w !packed_time_a+5          ; not reached in the traces so far
    ldy.w !packed_time_a+5          ; not reached in the traces so far
    lda.w !packed_time_a+4          ; not reached in the traces so far
.lD576:
    pha                             ; not reached in the traces so far
    lda.w !crash_scroll+9,y         ; not reached in the traces so far
    and.w #$7C00                    ; not reached in the traces so far
    sta.w !wram_1F2B                ; not reached in the traces so far
    lda.l !copy_src,x               ; not reached in the traces so far
    and.w #$7C00                    ; not reached in the traces so far
    cmp.w !wram_1F2B                ; not reached in the traces so far
    beq .lD59E                      ; not reached in the traces so far
    bcs .lD597                      ; not reached in the traces so far
    lda.w !wram_1F2B                ; not reached in the traces so far
    sec                             ; not reached in the traces so far
    sbc.w #$0400                    ; not reached in the traces so far
    bra .lD59E                      ; not reached in the traces so far
.lD597:
    lda.w !wram_1F2B                ; not reached in the traces so far
    clc                             ; not reached in the traces so far
    adc.w #$0400                    ; not reached in the traces so far
.lD59E:
    sta.b !scale_scratch            ; not reached in the traces so far
    lda.w !crash_scroll+9,y         ; not reached in the traces so far
    and.w #$03E0                    ; not reached in the traces so far
    sta.w !wram_1F2B                ; not reached in the traces so far
    lda.l !copy_src,x               ; not reached in the traces so far
    and.w #$03E0                    ; not reached in the traces so far
    cmp.w !wram_1F2B                ; not reached in the traces so far
    beq .lD5C7                      ; not reached in the traces so far
    bcs .lD5C0                      ; not reached in the traces so far
    lda.w !wram_1F2B                ; not reached in the traces so far
    sec                             ; not reached in the traces so far
    sbc.w #$0020                    ; not reached in the traces so far
    bra .lD5C7                      ; not reached in the traces so far
.lD5C0:
    lda.w !wram_1F2B                ; not reached in the traces so far
    clc                             ; not reached in the traces so far
    adc.w #$0020                    ; not reached in the traces so far
.lD5C7:
    sta.b !dp_40                    ; not reached in the traces so far
    lda.w !crash_scroll+9,y         ; not reached in the traces so far
    and.w #$001F                    ; not reached in the traces so far
    sta.w !wram_1F2B                ; not reached in the traces so far
    lda.l !copy_src,x               ; not reached in the traces so far
    and.w #$001F                    ; not reached in the traces so far
    cmp.w !wram_1F2B                ; not reached in the traces so far
    beq .lD5EA                      ; not reached in the traces so far
    bcs .lD5E6                      ; not reached in the traces so far
    lda.w !wram_1F2B                ; not reached in the traces so far
    dec a                           ; not reached in the traces so far
    bra .lD5EA                      ; not reached in the traces so far
.lD5E6:
    lda.w !wram_1F2B                ; not reached in the traces so far
    inc a                           ; not reached in the traces so far
.lD5EA:
    ora.b !scale_scratch            ; not reached in the traces so far
    ora.b !dp_40                    ; not reached in the traces so far
    sta.w !crash_scroll+9,y         ; not reached in the traces so far
    iny                             ; not reached in the traces so far
    iny                             ; not reached in the traces so far
    inx                             ; not reached in the traces so far
    inx                             ; not reached in the traces so far
    pla                             ; not reached in the traces so far
    dec a                           ; not reached in the traces so far
    beq .lD5FC                      ; not reached in the traces so far
    brl .lD576                      ; not reached in the traces so far
.lD5FC:
    rtl                             ; not reached in the traces so far

; @routine 04:E32D CoursePaletteCycle
; WHAT IT DOES: Steps the colour cycle of courses 3, 7 and $0B: the step
; $0D30 goes round 0 to $15, and its entry in word tables in bank 04 sets
; colours 12-15 of the palette: for course 3 in $7E:FD7D from the tables
; at $04:E45F, $E48B, $E4B7 and $E4E3; for course 7 in $7F:39EF from those
; at $E50F, $E53B, $E4B7 and $E4E3, with fixed colours $14A7, $1086,
; $15FF and $269F in $7F:37EF; for course $0B only the low byte of colour
; 15, in all three copies, from the byte tables at $04:E433 (when
; $7F:3460 is $18 or more) or $04:E407. Other courses: nothing.
; HOW: P and the data bank ($04) pushed; 8-bit step, 16-bit colours (8-bit
; for course $0B). Course $0B is not reached in the traces so far.
; Entry: any widths. Exit: P and the data bank restored; A the last value
; loaded; X the step's offset.
%at($04E32D)
CoursePaletteCycle:
    php
    sep.b #$20
    phb
    phk
    plb
    lda.w !course
    cmp.b #$03
    bne .lE36E
    lda.w !wram_0D2F+1
    inc a
    cmp.b #$16
    bcc .lE344
    lda.b #$00
.lE344:
    sta.w !wram_0D2F+1
    asl a
    tax
    rep.b #$20
    lda.l !palette_cycle_3_12,x
    sta.l !palette_buffer+24
    lda.l !palette_cycle_3_13,x
    sta.l !palette_buffer+26
    lda.l !palette_cycle_14,x
    sta.l !palette_buffer+28
    lda.l !palette_cycle_15,x
    sta.l !palette_buffer+30
    jmp.w .lE402
.lE36E:
    cmp.b #$07
    beq .lE375
    brl .lE3C5
.lE375:
    lda.w !wram_0D2F+1
    inc a
    cmp.b #$16
    bcc .lE37F
    lda.b #$00
.lE37F:
    sta.w !wram_0D2F+1
    asl a
    tax
    rep.b #$20
    lda.w #$14A7
    sta.l !palette_work_b+24
    lda.w #$1086
    sta.l !palette_work_b+26
    lda.w #$15FF
    sta.l !palette_work_b+28
    lda.w #$269F
    sta.l !palette_work_b+30
    lda.l !palette_cycle_7_12,x
    sta.l !palette_work_c+24
    lda.l !palette_cycle_7_13,x
    sta.l !palette_work_c+26
    lda.l !palette_cycle_14,x
    sta.l !palette_work_c+28
    lda.l !palette_cycle_15,x
    sta.l !palette_work_c+30
    jmp.w .lE402
.lE3C5:
    cmp.b #$0B
    bne .lE402
    lda.w !wram_0D2F+1
    inc a
    cmp.b #$16
    bcc .lE3D3
    lda.b #$00
.lE3D3:
    sta.w !wram_0D2F+1
    asl a
    tax
    lda.l !wram_7F3460
    cmp.b #$18
    bcc .lE3F2
    lda.l !palette_cycle_b_bright,x
    sta.l !palette_buffer+30
    sta.l !palette_work_b+30
    sta.l !palette_work_c+30
    bra .lE402
.lE3F2:
    lda.l !palette_cycle_b_dim,x
    sta.l !palette_buffer+30
    sta.l !palette_work_b+30
    sta.l !palette_work_c+30
.lE402:
    sep.b #$20
    plb
    plp
    rtl

; @routine 7E:E34F HudColourStep
; WHAT IT DOES: The race's colour work each frame, with the data bank $7F:
; steps the blend toward the target colour $3470 by $3468 ($345C asks: a
; negative value sets the target to its low 15 bits and adds 2 to the
; amount, at most $1F; a positive one takes 3 off it, down to $0A on course
; 6 and $0F on course 4, elsewhere to nothing once the fade $0D60 is $0F00,
; the target then $FFFF); with $0DA4 bit 11 the HUD's gradient goes to $F757
; and the first 16 colours of the buffer at $7E:FD65 to 0, the target (when
; not negative) blended from black at $F1 first. Otherwise it rebuilds one
; of three gradient bands (the table at $7E:E69C, counter $347C) at
; $7F:EC57 from the colours at $EF57/$F357 + $0D96, blended by $3460 and
; toward the target; then the buffer from $7F:35D7, $37D7 and $39D7 as the
; blend, target and amount have changed, five HUD colours at $0E21
; (ColourBlendToTarget), on course 7 colours $0C-$0F again, and keeps the
; blend, amount and target at $3464, $346C and $3474.
; HOW: P and the data bank pushed at the start and pulled at the end; 16-bit
; registers; colours through ColourBlend and ColourBlendBy with direct page
; $F1, $F3 and $F5.
; Entry: any widths; D = $0000. Exit: M 8-bit, X 8-bit inside, then P and
; the data bank as they were; A, X, Y as the last step left them.
%ramcode($7EE34F, $014076)
HudColourStep:
    php
    phb
    sep.b #$20
    lda.b #$7F
    pha
    plb
    rep.b #$30
    lda.w !colour_to_b+4
    bne HudColourStep_lE361
    brl HudColourStep_lE3DE
HudColourStep_lE361:
    bmi HudColourStep_lE3C6
    lda.l !course
    and.w #$00FF
    cmp.w #$0006
    beq HudColourStep_lE385
    cmp.w #$0004
    bne HudColourStep_lE39F
    lda.w !blend_target_amount
    sec
    sbc.w #$0003
    cmp.w #$000F
    bcs HudColourStep_lE3DB
    lda.w #$000F
    bra HudColourStep_lE394
HudColourStep_lE385:
    lda.w !blend_target_amount
    sec
    sbc.w #$0003
    cmp.w #$000A
    bcs HudColourStep_lE3DB
    lda.w #$000A
HudColourStep_lE394:
    sta.w !blend_target_amount
    lda.w #$0000
    sta.w !colour_to_b+4
    bra HudColourStep_lE3DE
HudColourStep_lE39F:
    lda.l !fade_level
    cmp.w #$0F00
    beq HudColourStep_lE3AD
    lda.w !blend_target_amount
    bra HudColourStep_lE3DB
HudColourStep_lE3AD:
    lda.w !blend_target_amount
    beq HudColourStep_lE3DE
    sec
    sbc.w #$0003
    bcs HudColourStep_lE3DB
    lda.w #$FFFF
    sta.w !blend_target
    lda.w #$0000
    sta.w !colour_to_b+4
    bra HudColourStep_lE3DB
HudColourStep_lE3C6:
    and.w #$7FFF
    sta.w !blend_target
    lda.w !blend_target_amount
    clc
    adc.w #$0002
    cmp.w #$001F
    bcc HudColourStep_lE3DB
    lda.w #$001F
HudColourStep_lE3DB:
    sta.w !blend_target_amount
HudColourStep_lE3DE:
    lda.l !wram_0DA4
    bit.w #$0800
    beq HudColourStep_lE42E
    sta.w !blend_target+8
    ldy.w #$0080
    ldx.w #$00FE
    lda.l !gsu_buffer_select
    beq HudColourStep_lE3F9
    ldx.w #$01FE
HudColourStep_lE3F9:
    lda.w !blend_target
    bmi HudColourStep_lE40D
    sta.b !dp_f3
    lda.w #$0000
    sta.b !digit_count
    lda.w !blend_target_amount
    sta.b !save_block_offset
    jsr.w ColourBlendBy
HudColourStep_lE40D:
    lda.l !gsu_buffer_select
    asl a
    tax
    lda.w #$F757
    sta.l !gradient_src_b+2,x
    lda.w #$0000
    ldx.w #$0004
HudColourStep_lE420:
    sta.l !palette_buffer,x
    inx
    inx
    cpx.w #$0020
    bne HudColourStep_lE420
    jmp.w HudColourStep_lE697
HudColourStep_lE42E:
    lda.w !blend_target+8
    beq HudColourStep_lE445
    lda.w !blend_target
    sta.w !blend_target+4
    lda.w #$0000
    sta.w !blend_target+8
    ldx.w #$0004
    jmp.w HudColourStep_lE579
HudColourStep_lE445:
    lda.w !blend_target+12
    inc a
    cmp.w #$0003
    bcc HudColourStep_lE451
    lda.w #$0000
HudColourStep_lE451:
    sta.w !blend_target+12
    asl a
    tax
    lda.l HudColourStep_bands,x
    tay
    clc
    adc.w #$0100
    sta.w !colour_to_b+2
    lda.l !gsu_buffer_select
    asl a
    tax
    tya
    clc
    adc.w #$EC57
    sta.l !gradient_src_b+2,x
    lda.l !gradient_src_a
    tax
    lda.w !blend_target
    bpl HudColourStep_lE4B7
    lda.w !colour_to_b+8
    jsr.w PpuMultiplierSetLowByteTimes8
    lda.w !palette_differ_bits+56,x
    sta.b !digit_count
    bra HudColourStep_lE4A1
HudColourStep_lE488:
    lda.w !palette_differ_bits+56,x
    cmp.w !palette_differ_bits+54,x
    bne HudColourStep_lE49F
    sta.b !digit_count
    lda.w !palette_differ_bits+1080,x
    cmp.w !palette_differ_bits+1078,x
    bne HudColourStep_lE4A4
    lda.w !race_fade_stage_handlers+61,y
    bra HudColourStep_lE4A9
HudColourStep_lE49F:
    sta.b !digit_count
HudColourStep_lE4A1:
    lda.w !palette_differ_bits+1080,x
HudColourStep_lE4A4:
    sta.b !dp_f3
    jsr.w ColourBlend
HudColourStep_lE4A9:
    sta.w !race_fade_stage_handlers+63,y
    inx
    inx
    iny
    iny
    cpy.w !colour_to_b+2
    bcc HudColourStep_lE488
    bra HudColourStep_lE506
HudColourStep_lE4B7:
    lda.w !palette_differ_bits+56,x
    sta.b !digit_count
    bra HudColourStep_lE4D7
HudColourStep_lE4BE:
    lda.w !palette_differ_bits+56,x
    cmp.w !palette_differ_bits+54,x
    bne HudColourStep_lE4D5
    sta.b !digit_count
    lda.w !palette_differ_bits+1080,x
    cmp.w !palette_differ_bits+1078,x
    bne HudColourStep_lE4DA
    lda.w !race_fade_stage_handlers+61,y
    bra HudColourStep_lE4FA
HudColourStep_lE4D5:
    sta.b !digit_count
HudColourStep_lE4D7:
    lda.w !palette_differ_bits+1080,x
HudColourStep_lE4DA:
    sta.b !dp_f3
    lda.w !colour_to_b+8
    sta.b !save_block_offset
    jsr.w ColourBlendBy

; @routine 7E:E4E4 HudColourRowsRest
; WHAT IT DOES: HudColourStep from inside its second band loop, with the
; blended colour in A: a negative target ($3470) keeps it as the row's
; colour; otherwise it is blended toward the target by $3468. Then the
; loop's other rows and the rest of HudColourStep. Nothing reaches it with a
; negative target: the loop runs only when the target is positive.
; HOW: Pushes A and pulls it back, as the loop does; ends as HudColourStep,
; pulling the data bank and P the start pushed.
; Entry: as inside HudColourStep: M 16-bit, X 16-bit, D = $0000, DB = $7F;
; X = colour offset, Y = row offset; the data bank and P on the stack above
; the return address. Exit: as HudColourStep.
%ramcode($7EE4E4, $01420B)
HudColourRowsRest:
    pha
    lda.w !blend_target
    bmi HudColourStep_lE4F9
    sta.b !dp_f3
    pla
    sta.b !digit_count
    lda.w !blend_target_amount
    sta.b !save_block_offset
    jsr.w ColourBlendBy
    bra HudColourStep_lE4FA
HudColourStep_lE4F9:
    pla
HudColourStep_lE4FA:
    sta.w !race_fade_stage_handlers+63,y
    inx
    inx
    iny
    iny
    cpy.w !colour_to_b+2
    bcc HudColourStep_lE4BE
HudColourStep_lE506:
    ldx.w #$0004
    bra HudColourStep_lE50E

; @routine 7E:E50B HudColourStepFrom14
; WHAT IT DOES: HudColourStep's buffer step from colour $14 (offset $14 into
; the buffers) instead of 4. Nothing reaches it: the code before branches
; over it.
; HOW: X = $14, then as HudColourStep from its buffer step.
; Entry: as inside HudColourStep: M 16-bit, X 16-bit, D = $0000, DB = $7F;
; the data bank and P on the stack above the return address. Exit: as
; HudColourStep.
%ramcode($7EE50B, $014232)
HudColourStepFrom14:
    ldx.w #$0014
HudColourStep_lE50E:
    lda.w !colour_to_b+8
    cmp.w !colour_to_b+12
    bne HudColourStep_lE574
    lda.w !blend_target
    bpl HudColourStep_lE535
    lda.w !blend_target+4
    bpl HudColourStep_lE523
    jmp.w HudColourStep_lE613
HudColourStep_lE523:
    lda.l !palette_work_a,x
    sta.l !palette_buffer,x
    inx
    inx
    cpx.w #$0200
    bcc HudColourStep_lE523
    jmp.w HudColourStep_lE613
HudColourStep_lE535:
    cmp.w !blend_target+4
    beq HudColourStep_lE53D
    jmp.w HudColourStep_lE548
HudColourStep_lE53D:
    lda.w !blend_target_amount
    cmp.w !blend_target_amount+4
    bne HudColourStep_lE548
    jmp.w HudColourStep_lE685
HudColourStep_lE548:
    lda.w !blend_target_amount
    jsr.w PpuMultiplierSetLowByteTimes8
HudColourStep_lE54E:
    lda.l !palette_work_b,x
    cmp.l !palette_work_c,x
    beq HudColourStep_lE56A
    lda.w !blend_target
    sta.b !dp_f3
    lda.l !palette_work_a,x
    sta.b !digit_count
    jsr.w ColourBlend
    sta.l !palette_buffer,x
HudColourStep_lE56A:
    inx
    inx
    cpx.w #$0200
    bcc HudColourStep_lE54E
    jmp.w HudColourStep_lE613
HudColourStep_lE574:
    lda.w !blend_target
    bmi HudColourStep_lE5D8
HudColourStep_lE579:
    lda.l !palette_work_b,x
    sec
    sbc.l !palette_work_c,x
    beq HudColourStep_lE5CF
    cmp.w #$0001
    beq HudColourStep_lE59E
    cmp.w #$FFFF
    beq HudColourStep_lE59E
    lda.w !colour_to_b+8
    beq HudColourStep_lE59E
    cmp.w #$001F
    bne HudColourStep_lE5A4
    lda.l !palette_work_c,x
    bra HudColourStep_lE5B8
HudColourStep_lE59E:
    lda.l !palette_work_b,x
    bra HudColourStep_lE5B8
HudColourStep_lE5A4:
    lda.l !palette_work_b,x
    sta.b !digit_count
    lda.l !palette_work_c,x
    sta.b !dp_f3
    lda.w !colour_to_b+8
    sta.b !save_block_offset
    jsr.w ColourBlendBy
HudColourStep_lE5B8:
    sta.l !palette_work_a,x
    sta.b !digit_count
    lda.w !blend_target
    sta.b !dp_f3
    lda.w !blend_target_amount
    sta.b !save_block_offset
    jsr.w ColourBlendBy
    sta.l !palette_buffer,x
HudColourStep_lE5CF:
    inx
    inx
    cpx.w #$0200
    bcc HudColourStep_lE579
    bra HudColourStep_lE613
HudColourStep_lE5D8:
    lda.w !colour_to_b+8
    sta.b !save_block_offset
    jsr.w PpuMultiplierSetLowByteTimes8; not reached in the traces so far
HudColourStep_lE5E0:
    lda.l !palette_work_b,x
    sec
    sbc.l !palette_work_c,x
    beq HudColourStep_lE60C
    cmp.w #$0001
    beq HudColourStep_lE60C
    cmp.w #$FFFF
    beq HudColourStep_lE60C
    lda.l !palette_work_b,x
    sta.b !digit_count
    lda.l !palette_work_c,x
    sta.b !dp_f3
    jsr.w ColourBlend
    sta.l !palette_work_a,x
    sta.l !palette_buffer,x
HudColourStep_lE60C:
    inx
    inx
    cpx.w #$0200
    bcc HudColourStep_lE5E0
HudColourStep_lE613:
    lda.w #$45CC
    ldx.w #$0000
    jsr.w ColourBlendToTarget
    lda.w #$2528
    ldx.w #$0002
    jsr.w ColourBlendToTarget
    lda.w #$1CE5
    ldx.w #$0004
    jsr.w ColourBlendToTarget
    lda.w #$739A
    ldx.w #$0006
    jsr.w ColourBlendToTarget
    lda.w #$1CE5
    ldx.w #$0008
    jsr.w ColourBlendToTarget
    lda.w #$2528
    ldx.w #$000A
    jsr.w ColourBlendToTarget
    rep.b #$30
    lda.l !course
    and.w #$00FF
    cmp.w #$0007
    bne HudColourStep_lE685
    lda.w !colour_to_b+4
    bne HudColourStep_lE685
    lda.w !colour_to_b+8
    sta.b !save_block_offset
    jsr.w PpuMultiplierSetLowByteTimes8
    ldx.w #$0018
HudColourStep_lE667:
    lda.l !palette_work_b,x
    sta.b !digit_count
    lda.l !palette_work_c,x
    sta.b !dp_f3
    jsr.w ColourBlend
    sta.l !palette_work_a,x
    sta.l !palette_buffer,x
    inx
    inx
    cpx.w #$0020
    bcc HudColourStep_lE667
HudColourStep_lE685:
    lda.w !colour_to_b+8
    sta.w !colour_to_b+12
    lda.w !blend_target_amount
    sta.w !blend_target_amount+4
    lda.w !blend_target
    sta.w !blend_target+4
HudColourStep_lE697:
    sep.b #$30
    plb
    plp
    rtl
HudColourStep_bands:
    dw $0000,$0100,$0200
