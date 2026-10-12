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
