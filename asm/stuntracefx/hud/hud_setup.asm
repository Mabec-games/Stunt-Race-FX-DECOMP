; hud_setup.asm: the race HUD's sprites and registers set up after its
; graphics are loaded (RaceHudLoad goes on here): the sprite lists, the HUD
; mode's own sprites, the backgrounds' scroll, the HUD's positions and the
; HDMA channels of the race's gradient; for two players the split screen's
; sprites instead.
; C: src/stuntracefx/hud/hud_setup.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 03:DE39 RaceHudSpritesHide
; WHAT IT DOES: Hides sprites 116 to 127 (Y = $F0).
; HOW: Twelve sprite macros, one byte each.
; Entry: any widths. Exit: M 8-bit, X 8-bit; A low byte $F0; N set, Z
; clear.
%at($03DE39)
RaceHudSpritesHide:
    sep.b #$20
    lda.b #$F0
    sta.l $000546
    sep.b #$20
    sep.b #$10
    sep.b #$20
    lda.b #$F0
    sta.l $00054A
    sep.b #$20
    sep.b #$10
    sep.b #$20
    lda.b #$F0
    sta.l $00054E
    sep.b #$20
    sep.b #$10
    sep.b #$20
    lda.b #$F0
    sta.l $000552
    sep.b #$20
    sep.b #$10
    sep.b #$20
    lda.b #$F0
    sta.l $000556
    sep.b #$20
    sep.b #$10
    sep.b #$20
    lda.b #$F0
    sta.l $00055A
    sep.b #$20
    sep.b #$10
    sep.b #$20
    lda.b #$F0
    sta.l $00055E
    sep.b #$20
    sep.b #$10
    sep.b #$20
    lda.b #$F0
    sta.l $000562
    sep.b #$20
    sep.b #$10
    sep.b #$20
    lda.b #$F0
    sta.l $000566
    sep.b #$20
    sep.b #$10
    sep.b #$20
    lda.b #$F0
    sta.l $00056A
    sep.b #$20
    sep.b #$10
    sep.b #$20
    lda.b #$F0
    sta.l $00056E
    sep.b #$20
    sep.b #$10
    sep.b #$20
    lda.b #$F0
    sta.l $000572
    sep.b #$20
    sep.b #$10
    rts

; @routine 03:DECA RaceHudSetup
; WHAT IT DOES: The one-player race HUD after RaceHudLoad: sounds
; (SoundFromTableD89), the Super FX's $70:004E = $A325, the sprite table
; cleared (OamBufferClear) and filled from the lists at $F883 and
; $F883 + $01AA (OamFromList); the HUD mode's sprites by HUD mode $0D2B
; (the table at $03:E052): 0 Speed Trax and 2 Battle Trax the players'
; numbers (or, with $10CD set, two blank sprites), 1 Stunt Trax the score
; (or, with $0D4E, the course's record time), 3 all courses the best time
; (or as mode 0 with $10CD); then the sprite tables out (OamDmaAll), the
; backgrounds' scroll (BG1 V $FFEF, BG2 V $0080), the HUD's positions
; ($0D3F = 4, $0E09-$0E14, $0D57), HDMA channels 1-3 (the table at
; $7F:3480, $7F:3497 filled by GradientFillCgadd, $7F:3489), $0374 = 3,
; $70:01C4 = 1, RaceVarsReset; then the data bank and P pushed by
; RaceHudLoad are pulled and it returns to RaceHudLoad's caller.
; HOW: Sprite bytes by long stores in runs of a macro; JMP (abs,X) by HUD
; mode. The non-Speed modes' special paths are not reached in the traces
; so far.
; Entry: data bank $03; RaceHudLoad's P and data bank on the stack. Exit:
; P and the data bank as RaceHudLoad found them; A, X, Y as
; RaceVarsReset leaves them.
%at($03DECA)
RaceHudSetup:
    sep.b #$20
    jsl.l SoundFromTableD89
    rep.b #$30
    lda.w #$A325
    sta.l !gsu_rec_4e
    jsl.l OamBufferClear
    sep.b #$20
    rep.b #$10
    ldx.w #$0000
    ldy.w #$0000
    jsr.w OamFromList
    ldx.w #$0200
    ldy.w #$01AA
    jsr.w OamFromList
    sep.b #$30
    lda.w !hud_mode
    asl a
    tax
    jmp.w (RaceHudModes,x)

; @routine 03:DEFD RecordOffsetAllCourses
; WHAT IT DOES: The all-courses records' offset of course $0E3D: (course -
; $14) * 5, in X.
; HOW: 16-bit, the course kept in direct page $F1. Not reached in the
; traces so far.
; Entry: M 16-bit, X 16-bit, D = $0000. Exit: A = X = the offset; N
; clear, Z from it; C, V from the add.
%at($03DEFD)
RecordOffsetAllCourses:
    lda.w !course                   ; not reached in the traces so far
    sec
    sbc.w #$0014
    sta.b !digit_count
    asl a
    asl a
    clc
    adc.b !digit_count
    and.w #$00FF
    tax
    rtl

; @routine 03:DF10 HudBestTimeDigits
; WHAT IT DOES: Shows the packed time A as the best time: its five digits
; as two tiles each, then the sprites placed (HudBestTimeSprites).
; HOW: TimeUnpackBcdLong, then 8-bit (X, Y high bytes cleared); P kept
; (PHP, PLP). Not reached in the traces so far.
; Entry: M 16-bit, X 16-bit, D = $0000; A = the packed time. Exit: P as on
; entry; A low byte $36; X = the seconds, Y = the hundredths (BCD, 8
; bits).
%at($03DF10)
HudBestTimeDigits:
    php                             ; not reached in the traces so far
    jsl.l TimeUnpackBcdLong
    sep.b #$30
    and.b #$0F
    asl a
    sta.l !oam_sprite56_attr+15
    inc a
    sta.l !oam_sprite56_attr+19
    txa
    lsr a
    lsr a
    lsr a
    lsr a
    asl a
    sta.l !oam_sprite56_attr+23
    inc a
    sta.l !oam_sprite56_attr+27
    txa
    and.b #$0F
    asl a
    sta.l $0004D7
    inc a
    sta.l $0004DB
    tya
    lsr a
    lsr a
    lsr a
    lsr a
    asl a
    sta.l $0004DF
    inc a
    sta.l $0004E3
    tya
    and.b #$0F
    asl a
    sta.l $0004E7
    inc a
    sta.l $0004EB
    jsl.l HudBestTimeSprites
    plp
    rtl

; @routine 03:DF61 HudBestTimeSprites
; WHAT IT DOES: Places the best time's ten sprites (X, Y, attributes $36):
; two rows at Y $B8 and $C0 from X $AC to $DC; the tiles are the digits'.
; HOW: Ten sprite macros. Not reached in the traces so far.
; Entry: any widths. Exit: M 8-bit, X 8-bit; A low byte $36; N, Z clear.
%at($03DF61)
HudBestTimeSprites:
    sep.b #$20                      ; not reached in the traces so far
    lda.b #$AC
    sta.l !oam_sprite56_attr+13
    lda.b #$B8
    sta.l !oam_sprite56_attr+14
    lda.b #$36
    sta.l !oam_sprite56_attr+16
    sep.b #$20
    sep.b #$10
    sep.b #$20
    lda.b #$AC
    sta.l !oam_sprite56_attr+17
    lda.b #$C0
    sta.l !oam_sprite56_attr+18
    lda.b #$36
    sta.l !oam_sprite56_attr+20
    sep.b #$20
    sep.b #$10
    sep.b #$20
    lda.b #$BC
    sta.l !oam_sprite56_attr+21
    lda.b #$B8
    sta.l !oam_sprite56_attr+22
    lda.b #$36
    sta.l !oam_sprite56_attr+24
    sep.b #$20
    sep.b #$10
    sep.b #$20
    lda.b #$BC
    sta.l !oam_sprite56_attr+25
    lda.b #$C0
    sta.l !oam_sprite56_attr+26
    lda.b #$36
    sta.l !oam_sprite56_attr+28
    sep.b #$20
    sep.b #$10
    sep.b #$20
    lda.b #$C4
    sta.l $0004D5
    lda.b #$B8
    sta.l $0004D6
    lda.b #$36
    sta.l $0004D8
    sep.b #$20
    sep.b #$10
    sep.b #$20
    lda.b #$C4
    sta.l $0004D9
    lda.b #$C0
    sta.l $0004DA
    lda.b #$36
    sta.l $0004DC
    sep.b #$20
    sep.b #$10
    sep.b #$20
    lda.b #$D4
    sta.l $0004DD
    lda.b #$B8
    sta.l $0004DE
    lda.b #$36
    sta.l $0004E0
    sep.b #$20
    sep.b #$10
    sep.b #$20
    lda.b #$D4
    sta.l $0004E1
    lda.b #$C0
    sta.l $0004E2
    lda.b #$36
    sta.l $0004E4
    sep.b #$20
    sep.b #$10
    sep.b #$20
    lda.b #$DC
    sta.l $0004E5
    lda.b #$B8
    sta.l $0004E6
    lda.b #$36
    sta.l $0004E8
    sep.b #$20
    sep.b #$10
    sep.b #$20
    lda.b #$DC
    sta.l $0004E9
    lda.b #$C0
    sta.l $0004EA
    lda.b #$36
    sta.l $0004EC
    sep.b #$20
    sep.b #$10
    rtl

; RaceHudSetup's table of HUD modes and its body from there on.
%at($03E052)
RaceHudModes:
    dw RaceHudModeSpeed, RaceHudModeStunt, RaceHudModeBattle, RaceHudModeAll
RaceHudModeAll:
    lda.w !wram_10CD
    beq .lE062
    brl RaceHudModeSpeed
.lE062:
    lda.b #$EC                      ; not reached in the traces so far
    sta.w !wram_0E16
    lda.b #$E8
    sta.w !wram_0E16+2
    rep.b #$30
    lda.w #$7FFF
    sta.l !slide_done
    lda.w !digit62_override
    and.w #$00FF
    beq .lE091
    jsl.l RecordOffsetAllCourses
    lda.l !save_record_b,x
    cmp.w #$FFFF
    beq RaceHudNoTime
    jsl.l HudBestTimeDigits
    jmp.w RaceHudFlags
.lE091:
    rep.b #$20
    lda.w !wram_0F2B
    cmp.w #$FFFF
    bne .lE09E
    jmp.w RaceHudNoTime
.lE09E:
    sep.b #$20
    lda.w !record_time_bcd+8
    and.b #$0F
    asl a
    sta.l !oam_sprite56_attr+15
    inc a
    sta.l !oam_sprite56_attr+19
    lda.w !record_time_bcd+7
    lsr a
    lsr a
    lsr a
    lsr a
    asl a
    sta.l !oam_sprite56_attr+23
    inc a
    sta.l !oam_sprite56_attr+27
    lda.w !record_time_bcd+7
    and.b #$0F
    asl a
    sta.l $0004D7
    inc a
    sta.l $0004DB
    lda.w !record_time_bcd+6
    lsr a
    lsr a
    lsr a
    lsr a
    asl a
    sta.l $0004DF
    inc a
    sta.l $0004E3
    lda.w !record_time_bcd+6
    and.b #$0F
    asl a
    sta.l $0004E7
    inc a
    sta.l $0004EB
    jsl.l HudBestTimeSprites
    jmp.w RaceHudFlags
RaceHudNoTime:
    sep.b #$20                      ; not reached in the traces so far
    sep.b #$20
    lda.b #$AB
    sta.l !oam_sprite56_attr+13
    lda.b #$BC
    sta.l !oam_sprite56_attr+14
    lda.b #$3F
    sta.l !oam_sprite56_attr+15
    lda.b #$36
    sta.l !oam_sprite56_attr+16
    sep.b #$20
    rep.b #$10
    sep.b #$20
    lda.b #$AB
    sta.l !oam_sprite56_attr+17
    lda.b #$BC
    sta.l !oam_sprite56_attr+18
    lda.b #$3F
    sta.l !oam_sprite56_attr+19
    lda.b #$36
    sta.l !oam_sprite56_attr+20
    sep.b #$20
    rep.b #$10
    sep.b #$20
    lda.b #$BB
    sta.l !oam_sprite56_attr+21
    lda.b #$BC
    sta.l !oam_sprite56_attr+22
    lda.b #$3F
    sta.l !oam_sprite56_attr+23
    lda.b #$36
    sta.l !oam_sprite56_attr+24
    sep.b #$20
    rep.b #$10
    sep.b #$20
    lda.b #$BB
    sta.l !oam_sprite56_attr+25
    lda.b #$BC
    sta.l !oam_sprite56_attr+26
    lda.b #$3F
    sta.l !oam_sprite56_attr+27
    lda.b #$36
    sta.l !oam_sprite56_attr+28
    sep.b #$20
    rep.b #$10
    sep.b #$20
    lda.b #$C3
    sta.l $0004D5
    lda.b #$BC
    sta.l $0004D6
    lda.b #$3F
    sta.l $0004D7
    lda.b #$36
    sta.l $0004D8
    sep.b #$20
    rep.b #$10
    sep.b #$20
    lda.b #$C3
    sta.l $0004D9
    lda.b #$BC
    sta.l $0004DA
    lda.b #$3F
    sta.l $0004DB
    lda.b #$36
    sta.l $0004DC
    sep.b #$20
    rep.b #$10
    sep.b #$20
    lda.b #$D5
    sta.l $0004DD
    lda.b #$BC
    sta.l $0004DE
    lda.b #$3F
    sta.l $0004DF
    lda.b #$36
    sta.l $0004E0
    sep.b #$20
    rep.b #$10
    sep.b #$20
    lda.b #$D5
    sta.l $0004E9
    lda.b #$BC
    sta.l $0004EA
    lda.b #$3F
    sta.l $0004EB
    lda.b #$36
    sta.l $0004EC
    sep.b #$20
    rep.b #$10
    sep.b #$20
    lda.b #$DD
    sta.l $0004E5
    lda.b #$BC
    sta.l $0004E6
    lda.b #$3F
    sta.l $0004E7
    lda.b #$36
    sta.l $0004E8
    sep.b #$20
    rep.b #$10
    sep.b #$20
    lda.b #$DD
    sta.l $0004E9
    lda.b #$BC
    sta.l $0004EA
    lda.b #$3F
    sta.l $0004EB
    lda.b #$36
    sta.l $0004EC
    sep.b #$20
    rep.b #$10
RaceHudFlags:
    sep.b #$20                      ; not reached in the traces so far
    lda.b #$B4
    sta.l $0004ED
    lda.b #$B8
    sta.l $0004EE
    lda.b #$14
    sta.l $0004EF
    lda.b #$36
    sta.l $0004F0
    sep.b #$20
    rep.b #$10
    sep.b #$20
    lda.b #$CC
    sta.l $0004F1
    lda.b #$B8
    sta.l $0004F2
    lda.b #$15
    sta.l $0004F3
    lda.b #$36
    sta.l $0004F4
    sep.b #$20
    rep.b #$10
    jmp.w RaceHudCommon
RaceHudModeStunt:
    lda.w !hud_blank
    bne .lE26B
    brl .lE383
.lE26B:
    rep.b #$20                      ; not reached in the traces so far
    lda.w #$7FFF
    sta.l !slide_done
    lda.w #$00AC
    sta.w !wram_0E16
    lda.w #$008C
    sta.w !wram_0E16+2
    sep.b #$20
    sep.b #$20
    lda.b #$B4
    sta.l $000545
    sep.b #$20
    rep.b #$10
    sep.b #$20
    lda.b #$CA
    sta.l $000549
    sep.b #$20
    rep.b #$10
    sep.b #$20
    lda.b #$D2
    sta.l $00054D
    sep.b #$20
    rep.b #$10
    sep.b #$20
    lda.b #$D2
    sta.l $000551
    sep.b #$20
    rep.b #$10
    sep.b #$20
    lda.b #$DA
    sta.l $000555
    sep.b #$20
    rep.b #$10
    sep.b #$20
    lda.b #$DA
    sta.l $000559
    sep.b #$20
    rep.b #$10
    sep.b #$20
    lda.b #$BA
    sta.l $00055D
    sep.b #$20
    rep.b #$10
    sep.b #$20
    lda.b #$BA
    sta.l $000561
    sep.b #$20
    rep.b #$10
    sep.b #$20
    lda.b #$C2
    sta.l $000565
    sep.b #$20
    rep.b #$10
    sep.b #$20
    lda.b #$C2
    sta.l $000569
    sep.b #$20
    rep.b #$10
    sep.b #$20
    lda.b #$AC
    sta.l $00056D
    sep.b #$20
    rep.b #$10
    sep.b #$20
    lda.b #$AC
    sta.l $000571
    sep.b #$20
    rep.b #$10
    rep.b #$30
    lda.w !course
    and.w #$00FF
    sec
    sbc.w #$0017
    sta.b !digit_count
    asl a
    asl a
    clc
    adc.b !digit_count
    tax
    lda.l !save_record_a,x
    cmp.w #$FFFF
    bne .lE332
    brl RaceHudNoTime
.lE332:
    jsl.l TimeUnpackBcdLong
    sep.b #$30
    and.b #$0F
    asl a
    sta.l !oam_sprite56_attr+15
    inc a
    sta.l !oam_sprite56_attr+19
    txa
    lsr a
    lsr a
    lsr a
    lsr a
    asl a
    sta.l !oam_sprite56_attr+23
    inc a
    sta.l !oam_sprite56_attr+27
    txa
    and.b #$0F
    asl a
    sta.l $0004D7
    inc a
    sta.l $0004DB
    tya
    lsr a
    lsr a
    lsr a
    lsr a
    asl a
    sta.l $0004DF
    inc a
    sta.l $0004E3
    tya
    and.b #$0F
    asl a
    sta.l $0004E7
    inc a
    sta.l $0004EB
    jsl.l HudBestTimeSprites
    jmp.w RaceHudFlags
.lE383:
    lda.b #$F0
    sta.w !wram_0E16+2
    jsr.w RaceHudSpritesHide
    sep.b #$20
    sep.b #$20
    lda.b #$D0
    sta.l !oam_sprite56_attr+13
    lda.b #$F0
    sta.l !oam_sprite56_attr+14
    lda.b #$3F
    sta.l !oam_sprite56_attr+15
    lda.b #$36
    sta.l !oam_sprite56_attr+16
    sep.b #$20
    sep.b #$10
    sep.b #$20
    lda.b #$F0
    sta.l !oam_sprite56_attr+18
    sep.b #$20
    sep.b #$10
    sep.b #$20
    lda.b #$D8
    sta.l !oam_sprite56_attr+21
    lda.b #$F0
    sta.l !oam_sprite56_attr+22
    lda.b #$3F
    sta.l !oam_sprite56_attr+23
    lda.b #$36
    sta.l !oam_sprite56_attr+24
    sep.b #$20
    sep.b #$10
    sep.b #$20
    lda.b #$F0
    sta.l !oam_sprite56_attr+26
    sep.b #$20
    sep.b #$10
    sep.b #$20
    lda.b #$BD
    sta.l $0004D5
    lda.b #$B8
    sta.l $0004D6
    lda.b #$00
    sta.l $0004D7
    lda.b #$36
    sta.l $0004D8
    sep.b #$20
    sep.b #$10
    sep.b #$20
    lda.b #$BD
    sta.l $0004D9
    lda.b #$C0
    sta.l $0004DA
    lda.b #$01
    sta.l $0004DB
    lda.b #$36
    sta.l $0004DC
    sep.b #$20
    sep.b #$10
    sep.b #$20
    lda.b #$C5
    sta.l $0004DD
    lda.b #$B8
    sta.l $0004DE
    lda.b #$00
    sta.l $0004DF
    lda.b #$36
    sta.l $0004E0
    sep.b #$20
    sep.b #$10
    sep.b #$20
    lda.b #$C5
    sta.l $0004E1
    lda.b #$C0
    sta.l $0004E2
    lda.b #$01
    sta.l $0004E3
    lda.b #$36
    sta.l $0004E4
    sep.b #$20
    sep.b #$10
    sep.b #$20
    lda.b #$DD
    sta.l $0004E5
    lda.b #$B8
    sta.l $0004E6
    lda.b #$08
    sta.l $0004E7
    lda.b #$36
    sta.l $0004E8
    sep.b #$20
    sep.b #$10
    sep.b #$20
    lda.b #$DD
    sta.l $0004E9
    lda.b #$C0
    sta.l $0004EA
    lda.b #$09
    sta.l $0004EB
    lda.b #$36
    sta.l $0004EC
    sep.b #$20
    sep.b #$10
    sep.b #$20
    lda.b #$E5
    sta.l $0004ED
    lda.b #$B8
    sta.l $0004EE
    lda.b #$00
    sta.l $0004EF
    lda.b #$36
    sta.l $0004F0
    sep.b #$20
    sep.b #$10
    sep.b #$20
    lda.b #$E5
    sta.l $0004F1
    lda.b #$C0
    sta.l $0004F2
    lda.b #$01
    sta.l $0004F3
    lda.b #$36
    sta.l $0004F4
    sep.b #$20
    sep.b #$10
    rep.b #$20
    lda.w #$2127
    sta.l !slide_done
    jmp.w RaceHudCommon
RaceHudModeSpeed:
    lda.b #$8C
    sta.w !wram_0E16+2
    lda.b #$EC
    sta.w !wram_0E16
RaceHudModeBattle:
    lda.w !wram_10CD
    beq .lE52C
    lda.b #$3F
    sta.w !oam_sprite56_attr+23
    sta.w !oam_sprite56_attr+15
    sep.b #$20
    lda.b #$AA
    sta.l !oam_sprite56_attr+21
    lda.b #$C0
    sta.l !oam_sprite56_attr+22
    lda.b #$36
    sta.l !oam_sprite56_attr+24
    sep.b #$20
    sep.b #$10
    sep.b #$20
    lda.b #$E6
    sta.l !oam_sprite56_attr+13
    lda.b #$C0
    sta.l !oam_sprite56_attr+14
    lda.b #$36
    sta.l !oam_sprite56_attr+16
    sep.b #$20
    sep.b #$10
    sep.b #$20
    jsr.w RaceHudSpritesHide
    jmp.w RaceHudSlideOff
.lE52C:
    lda.w !player_mode
    asl a
    sta.w !oam_sprite56_attr+23
    inc a
    sta.w !oam_sprite56_attr+27
    sep.b #$20
    lda.b #$AA
    sta.l !oam_sprite56_attr+21
    lda.b #$B8
    sta.l !oam_sprite56_attr+22
    lda.b #$36
    sta.l !oam_sprite56_attr+24
    sep.b #$20
    sep.b #$10
    sep.b #$20
    lda.b #$AA
    sta.l !oam_sprite56_attr+25
    lda.b #$C0
    sta.l !oam_sprite56_attr+26
    lda.b #$36
    sta.l !oam_sprite56_attr+28
    sep.b #$20
    sep.b #$10
    lda.w !wram_0DF8
    asl a
    sta.w !oam_sprite56_attr+15
    inc a
    sta.w !oam_sprite56_attr+19
    sep.b #$20
    lda.b #$E6
    sta.l !oam_sprite56_attr+13
    lda.b #$B8
    sta.l !oam_sprite56_attr+14
    lda.b #$36
    sta.l !oam_sprite56_attr+16
    sep.b #$20
    sep.b #$10
    sep.b #$20
    lda.b #$E6
    sta.l !oam_sprite56_attr+17
    lda.b #$C0
    sta.l !oam_sprite56_attr+18
    lda.b #$36
    sta.l !oam_sprite56_attr+20
    sep.b #$20
    sep.b #$10
    sep.b #$20
    lda.w !digit62_override
    beq RaceHudSlideOff
    rep.b #$20
    lda.w #$3947
    sta.l !slide_done
    bra RaceHudCommon
RaceHudSlideOff:
    rep.b #$20
    lda.w #$7FFF
    sta.l !slide_done
    sep.b #$20
    lda.w !wram_10CD
    beq RaceHudCommon
RaceHudCommon:
    sep.b #$20
    rep.b #$10
    jsl.l OamDmaAll
    sep.b #$30
    stz.w !BG1HOFS
    stz.w !BG1HOFS
    lda.b #$EF
    sta.w !BG1VOFS
    ldx.b #$FF
    stx.w !BG1VOFS
    stz.w !BG2HOFS
    stz.w !BG2HOFS
    lda.b #$80
    sta.w !BG2VOFS
    stz.w !BG2VOFS
    lda.b #$04
    sta.w !wram_0D3F
    lda.b #$FF
    sta.w !wram_0E42+4
    sta.w !wram_0E42+3
    stz.w !gsu_buffer_offset
    lda.b #$C4
    sta.w !gauge_b6_x
    lda.b #$93
    sta.w !wram_0E14
    lda.b #$2A
    sta.w !map_b_x
    lda.b #$FF
    lda.b #$7A
    sta.w !wram_0E0C
    lda.b #$00
    sta.w !map_b_y_w
    lda.b #$E0
    sta.w !hud_slide
    lda.b #$20
    sta.w !hud_slide+1
    lda.b #$50
    sta.w !gauge_b4_x
    lda.b #$B8
    sta.w !gauge_b4_y
    sep.b #$20
    lda.b #$00
    sta.w !DMAP1
    lda.b #$2C
    sta.w !BBAD1
    lda.b #$7F
    sta.w !A1B1
    rep.b #$20
    lda.w #$3480
    sta.w !A1T1L
    sep.b #$20
    lda.b #$20
    sta.l !wram_7F3478+8
    lda.b #$13
    sta.l !wram_7F3478+9
    lda.b #$40
    sta.l !wram_7F3478+10
    lda.b #$12
    sta.l !wram_7F3478+11
    lda.b #$40
    sta.l !wram_7F3478+12
    lda.b #$12
    sta.l !wram_7F3478+13
    lda.b #$01
    sta.l !wram_7F3478+14
    lda.b #$13
    sta.l !wram_7F3478+15
    lda.b #$00
    sta.l !wram_7F3478+16
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
    lda.w #$0120
    sta.l !gradient_cgadd
    ldy.w #$0080
    ldx.w #$0002
    lda.w #$0181
    jsr.w GradientFillCgadd
    lda.w #$0000
    sta.l !gradient_cgadd,x
    lda.w #$EC57
    sta.l !gradient_cgdata+1
    sep.b #$20
    lda.b #$20
    sta.l !gradient_cgdata
    lda.b #$FF
    sta.l !gradient_cgdata+3
    lda.b #$00
    sta.l !gradient_ptr_a+2
    lda.b #$7F
    sta.w !DASB3
    lda.b #$03
    sta.w !gsu_scmr_shadow
    rep.b #$20
    lda.w #$0001
    sta.l !gsu_word_1c4
    jmp.w RaceHudEnd

; @routine 03:E6FA RaceHudSetupTwoPlayers
; WHAT IT DOES: The two-player race HUD after RaceHudLoad: the Super FX's
; $70:004E = $A545 and $70:01C4 = 2, $0374 = 1, the course screen's set-up
; (MenuCourseScreenSetup, PaletteCourseLoad), the split screen's sprites
; (from the tables at $03:E7EA, $03:E80F and $03:E80A, into both halves),
; the Super FX blocks $F845/$F804 to VRAM (GsuCF42ToVram), RaceVarsReset;
; then as RaceHudSetup, the pushed data bank and P pulled.
; HOW: 16-bit copy loops, the size bits 8-bit.
; Entry: as RaceHudSetup. Exit: as RaceHudSetup.
%at($03E6FA)
RaceHudSetupTwoPlayers:
    rep.b #$30
    lda.w #$A545
    sta.l !gsu_rec_4e
    sep.b #$20
    lda.b #$01
    sta.w !gsu_scmr_shadow
    rep.b #$20
    lda.w #$0002
    sta.l !gsu_word_1c4
    jsl.l MenuCourseScreenSetup
    jsr.w PaletteCourseLoad
    rep.b #$30
    ldx.w #$00BC
    ldy.w #$0000
.lE722:
    lda.w $E7EA,y
    sec
    sbc.w #$0800
    sta.l !oam_buffer,x
    clc
    adc.w #$6850
    sta.l !oam_sprite56_attr+29,x
    lda.w $E7EC,y
    sta.l !oam_buffer+2,x
    sta.l !oam_sprite56_attr+31,x
    inx
    inx
    inx
    inx
    iny
    iny
    iny
    iny
    cpy.w #$0020
    bcc .lE722
    lda.w #$0000
    sta.w $0425
    sta.w $0429
    sta.w $042D
    sta.w $0525
    sta.w $0529
    sta.w $052D
    lda.w #$26A0
    sta.w $0427
    sta.w $042B
    sta.w $042F
    sta.w $0527
    sta.w $052B
    sta.w $052F
    ldx.w #$00DC
    ldy.w #$0000
.lE77D:
    lda.w $E80F,y
    clc
    adc.w #$2C28
    sta.w !oam_buffer,x
    lda.w $E811,y
    sta.w !oam_buffer+2,x
    inx
    inx
    inx
    inx
    iny
    iny
    iny
    iny
    cpy.w #$0024
    bcc .lE77D
    ldx.w #$01DC
    ldy.w #$0000
.lE7A0:
    lda.w $E80F,y
    clc
    adc.w #$9476
    sta.w !oam_buffer,x
    lda.w $E811,y
    sta.w !oam_buffer+2,x
    inx
    inx
    inx
    inx
    iny
    iny
    iny
    iny
    cpy.w #$0024
    bcc .lE7A0
    sep.b #$20
    ldx.w #$000B
    ldy.w #$0000
.lE7C5:
    lda.w $E80A,y
    sta.w !oam_buffer_high,x
    sta.w !oam_buffer_high+16,x
    inx
    iny
    cpy.w #$0005
    bcc .lE7C5
    rep.b #$30
    ldy.w #$F845
    ldx.w #$F804
    jsl.l GsuCF42ToVram
RaceHudEnd:
    jsl.l RaceVarsReset
    sep.b #$20
    plb
    plp
    rtl
