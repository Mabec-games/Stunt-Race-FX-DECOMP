; course_screen.asm: setting up the menu screen that shows the chosen
; course: its sprites, its picture drawn by the Super FX, and the colour
; gradient behind it.
; C: src/stuntracefx/menu/course_screen.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 0B:FA24 MenuCourseScreenSetup
; WHAT IT DOES: Sets up the course screen: queues the two players' sounds
; (SoundPairFromTable); hides every sprite (each word of the sprite table
; copy $E0E0, of its high table $5555), then builds four sprite groups from
; the lists at $0B:FCA2 (MenuOamFromList: sprite offsets 0, $200, $100,
; $210 from list offsets 0, $154, $A8, $162) and copies the table to OAM;
; has the Super FX draw the course picture (GsuCF42To3000, A = $D970, X =
; $1D) and copies it to VRAM (CoursePictureToVram); sets OBSEL to 3,
; $0D3F = 8, $0D7D = $0C and the HUD's gauge and map positions
; $0E09-$0E15; then sets up the gradient (GradientSetup).
; HOW: The lists are read with the data bank set to $0B; the stores to
; OBSEL and work RAM go through the caller's data bank. P is saved and
; restored.
; Entry: D = $0000. Exit: A as GradientSetup; X = Y = $20; P unchanged.
%at($0BFA24)
MenuCourseScreenSetup:
    php
    rep.b #$30
    jsl.l SoundPairFromTable
    rep.b #$30
    ldx.w #$0000
.lFA30:
    lda.w #$E0E0
    sta.w !oam_buffer,x
    inx
    inx
    cpx.w #$0200
    bne .lFA30
    ldx.w #$0000
.lFA40:
    lda.w #$5555
    sta.w !oam_buffer_high,x
    inx
    inx
    cpx.w #$0020
    bne .lFA40
    sep.b #$20
    phb
    lda.b #$0B
    pha
    plb
    rep.b #$30
    ldx.w #$0000
    ldy.w #$0000
    jsr.w MenuOamFromList
    ldx.w #$0200
    ldy.w #$0154
    jsr.w MenuOamFromList
    ldx.w #$0100
    ldy.w #$00A8
    jsr.w MenuOamFromList
    ldx.w #$0210
    ldy.w #$0162
    jsr.w MenuOamFromList
    plb
    jsl.l OamDmaAll
    rep.b #$30
    lda.w #$D970
    ldx.w #$001D
    jsl.l GsuCF42To3000
    jsl.l CoursePictureToVram
    sep.b #$20
    lda.b #$03
    sta.w !OBSEL
    sep.b #$20
    lda.b #$08
    sta.w !wram_0D3F
    lda.b #$0C
    sta.w !gsu_buffer_offset
    lda.b #$B0
    sta.w !gauge_b4_x
    lda.b #$1C
    sta.w !gauge_b4_y
    lda.b #$18
    sta.w !gauge_b6_x
    lda.b #$5A
    sta.w !wram_0E14
    lda.b #$B7
    sta.w !map_b_x
    lda.b #$3C
    sta.w !wram_0E0C
    lda.b #$10
    sta.w !gauge_a4_x
    lda.b #$84
    sta.w !gauge_a4_y
    lda.b #$68
    sta.w !gauge_a6_x
    lda.b #$C2
    sta.w !wram_0E15
    lda.b #$45
    sta.w !map_a_x
    lda.b #$A4
    sta.w !wram_0E0D
    jsr.w GradientSetup
    rep.b #$20
    plp
    rtl
