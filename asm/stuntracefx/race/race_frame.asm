; race_frame.asm: a race frame on the S-CPU, and the race's start, which
; sets up the Super FX, runs the course script and steps two frames.
; C: src/stuntracefx/race/race_frame.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 03:B7C5 RaceFrameStep
; WHAT IT DOES: One race frame: the frames since the last one ($0306) go to
; $06D4 and $0306 starts again; the near side is set ($1F07 = 0) and
; $70:246C = $FFFF; the objects run (ObjectsRunHandlers); the camera
; record at $1A64 moves on (+$37 += $400, CameraFollowStep) and the tied
; objects face it (ObjectFaceTied); then the view (ViewFromRecord), the
; Super FX frame with the object work (FrameRunGsu8FCA) and the view
; update (ViewUpdateAndGsuAEF8). With A = 0 it then waits for the vertical
; blank to take the frame ($0357), draws the screen (GsuAB13WithParams)
; and flips it (GsuScreenFlip).
; HOW: Data bank $03 (the program bank), $7E around the camera calls; A is
; kept on the stack; P and the data bank saved and restored.
; Entry: A = 0 to show the frame, else not. Exit: A = the entry A, or as
; GsuScreenFlip leaves it; X, Y as the last call leaves them; P and the
; data bank unchanged.
%at($03B7C5)
RaceFrameStep:
    php
    rep.b #$30
    pha
    sep.b #$20
    phb
    phk
    plb
    lda.w !frames_since_tick
    sta.w !frames_taken
    stz.w !frames_since_tick
    sep.b #$20
    lda.b #$00
    sta.w !draw_side
    rep.b #$20
    lda.w #$FFFF
    sta.l !gsu_view_246c
    sep.b #$20
    rep.b #$10
    jsl.l ObjectsRunHandlers
    sep.b #$20
    rep.b #$10
    phb
    lda.b #$7E
    pha
    plb
    ldx.w #$1A64
    rep.b #$20
    lda.b !cam_angle_add+2,x
    clc
    adc.w #$0400
    sta.b !cam_angle_add+2,x
    jsl.l CameraFollowStep
    jsl.l ObjectFaceTied
    plb
    rep.b #$10
    ldx.w #$1A64
    jsl.l ViewFromRecord
    jsl.l FrameRunGsu8FCA
    jsl.l ViewUpdateAndGsuAEF8
    rep.b #$20
    pla
    bne .done
    sep.b #$20
.wait_vblank:
    lda.w !wram_0357
    beq .wait_vblank
    rep.b #$30
    jsl.l GsuAB13WithParams
    jsr.w GsuScreenFlip
    sep.b #$20
    stz.w !wram_0357
.done:
    plb
    plp
    rtl

; @routine 03:B5DF RaceStart
; WHAT IT DOES: Starts a race: the Super FX's screen mode ($0374 = 3),
; clock (CLSR = 1), config (CFGR = $20) and screen base (SCBR = $0B); not
; the menu ($0D62 = 0); the object pools (ObjectPoolInit) and view
; (ViewDefaults); the screen start $2C00 ($70:021A and direct page $48),
; $70:01C4 = 2, $70:108C = 0; Super FX program $04:8800 (GsuRun); $116A =
; 0; the course script at $05:8000 (CourseScriptRun); two race frames not
; shown (RaceFrameStep with A = 1); the camera's target ($1A64 + 2) from
; $116C; $0357 = 1; and Super FX RAM $70:2C00-$5FFF cleared.
; HOW: Through the data bank as the caller has it, with long stores to
; Super FX RAM. P saved and restored.
; Entry: D = $0000. Exit: A = 0; X = $3400; Y = $1A64; P unchanged.
%at($03B5DF)
RaceStart:
    php
    sep.b #$20
    lda.b #$03
    sta.w !gsu_scmr_shadow
    lda.b #$01
    sta.w !GSU_CLSR
    lda.b #$20
    sta.w !GSU_CFGR
    lda.b #$0B
    sta.w !GSU_SCBR
    stz.w !wram_0D62
    rep.b #$30
    lda.w #$A545
    sta.l !gsu_rec_4e
    jsl.l ObjectPoolInit
    jsl.l ViewDefaults
    rep.b #$20
    lda.w #$2C00
    sta.l !gsu_screen_start
    sta.b !gsu_screen_start_dp
    lda.w #$0002
    sta.l !gsu_word_1c4
    lda.w #$0000
    sta.l !gsu_word_108c
    sep.b #$20
    rep.b #$10
    lda.b #$04
    ldx.w #$8800
    jsl.l GsuRun
    rep.b #$20
    stz.w !race_word_116a
    sep.b #$20
    rep.b #$10
    lda.b #$05
    ldx.w #$0000
    jsl.l CourseScriptRun
    lda.b #$01
    jsl.l RaceFrameStep
    lda.b #$01
    jsl.l RaceFrameStep
    rep.b #$30
    lda.w #$0000
    asl a
    tax
    lda.l !race_target_116c,x
    ldy.w #$1A64
    sta.w !cam_target,y
    sep.b #$20
    lda.b #$01
    sta.w !wram_0357
    rep.b #$30
    lda.w #$0000
    ldx.w #$0000
.clear:
    sta.l !gsu_screen_area,x
    inx
    inx
    cpx.w #$3400
    bne .clear
    plp
    rtl
