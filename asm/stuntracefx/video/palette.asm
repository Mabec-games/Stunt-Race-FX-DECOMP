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
