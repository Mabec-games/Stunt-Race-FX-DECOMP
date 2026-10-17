; raster.asm: waiting on the PPU's beam inside the raster interrupt.
; Runs from work RAM (copied there with the rest of main_wram, see
; config/stuntracefx/ramcode.csv). C: src/stuntracefx/system/raster.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"

arch 65816

; @routine 7E:A2FD WaitHBlank
; WHAT IT DOES: Waits until the PPU is in horizontal blank, so the raster
; interrupt can change PPU registers between two lines.
; HOW: Reads HVBJOY until its H-blank bit (bit 6) is set.
; Entry: M 8-bit, X 8-bit. Exit: A = the last HVBJOY read; N from it; Z clear.
%ramcode($7EA2FD, $010024)
WaitHBlank:
    lda.w !HVBJOY
    bit.b #$40
    beq WaitHBlank
    rts

; @routine 02:D7AB WaitScanline
; WHAT IT DOES: Waits until the low byte of the PPU's vertical counter
; equals the byte at $06E3; clears $06E4 first.
; HOW: Latches the counters (SLHV) and reads OPVCT twice (low, high) each
; time round; the compare is 8-bit.
; Entry: X 16-bit. Exit: A, X, P unchanged.
%at($02D7AB)
WaitScanline:
    php
    rep.b #$30
    phx
    pha
    sep.b #$20
    stz.w !scanline_wait_high
.lD7B5:
    sep.b #$20
    lda.w !SLHV
    lda.w !OPVCT
    xba
    lda.w !OPVCT
    xba
    cmp.w !scanline_wait
    bne .lD7B5
    rep.b #$30
    pla
    plx
    plp
    rtl

; @routine 7E:A2D9 RasterIrq
; WHAT IT DOES: The IRQ handler: for the raster interrupt (TIMEUP bit 7,
; read with RDNMI as one word, which acknowledges it) runs the stage
; (RasterStage) with interrupts off and data bank 0, keeping A, X, Y and
; the data bank; for any other source reads the Super FX status word,
; which acknowledges the Super FX's interrupt.
; HOW: 16-bit pushes around the stage; ends with RTL to the vector code.
; Entry: any widths. Exit: A, X, Y and the data bank as at entry; M and X
; 16-bit; N, Z from A; after a stage I set, C and V as the stage left
; them.
%ramcode($7EA2D9, $010000)
RasterIrq:
    rep.b #$30
    pha
    lda.l !RDNMI
    bmi .lA2E8
    lda.l !GSU_SFR
    pla
    rtl
.lA2E8:
    phb
    sei
    phx
    phy
    sep.b #$30
    lda.b #$00
    pha
    plb
    jsl.l RasterStage
    rep.b #$30
    ply
    plx
    plb
    pla
    rtl

; @routine 7E:A305 RasterStage
; WHAT IT DOES: Runs the raster interrupt's stage $0D3F (the table at
; $7E:A30D), each of which sets the next line and stage: the screen off
; or on (INIDISP, after waiting for horizontal blank), BG2's scroll and
; the split screen's bases, colours and window setups, the frame's
; counters and work for the vertical blank (fades, pads, sound ports, the
; race clock, the HUD time and panels, palettes, the window animations,
; the upload of the frame (NmiFrameUpload), sprites and the Super FX
; picture copies). Stages $16 and $18-$1A do nothing; $1C sets the next
; line to $10 (VTIMEL).
; HOW: SEP #$30, JMP (table,X) on $0D3F; the stages end with RTL. Stages
; 0, 2 and $1C and the negative split scroll at $A6EE are not reached in
; the traces so far.
; Entry: data bank 0. Exit: as the stage leaves it.
%ramcode($7EA305, $01002C)
RasterStage:
    sep.b #$30
    ldx.w !wram_0D3F
    jmp.w (.table,x)
.table:
    dw RasterStage_s00, RasterStage_s02, RasterStage_s04, RasterStage_s06, RasterStage_s08, RasterStage_s0A, RasterStage_s0C, RasterStage_s0E
    dw RasterStage_s10, RasterStage_s12, RasterStage_s14, RasterStage_s16, RasterStage_s18, RasterStage_s18, RasterStage_s1C, RasterStage_s1E
    dw RasterStage_s20
RasterStage_s16:
    rtl                             ; not reached in the traces so far
RasterStage_s00:
    jsr.w WaitHBlank                ; not reached in the traces so far
    lda.w !fade_level_hi            ; not reached in the traces so far
    sta.w !INIDISP                  ; not reached in the traces so far
    lda.b #$C8                      ; not reached in the traces so far
    sta.w !VTIMEL                   ; not reached in the traces so far
    lda.w !TIMEUP                   ; not reached in the traces so far
    lda.b #$02                      ; not reached in the traces so far
    sta.w !wram_0D3F                ; not reached in the traces so far
    inc.w !frame_count              ; not reached in the traces so far
    inc.w !frames_since_tick        ; not reached in the traces so far
    rtl                             ; not reached in the traces so far
RasterStage_s02:
    jsr.w WaitHBlank                ; not reached in the traces so far
    lda.b #$80                      ; not reached in the traces so far
    sta.w !INIDISP                  ; not reached in the traces so far
    lda.b #$17                      ; not reached in the traces so far
    sta.w !VTIMEL                   ; not reached in the traces so far
    lda.w !TIMEUP                   ; not reached in the traces so far
    stz.w !wram_0D3F                ; not reached in the traces so far
    lda.b #$FF                      ; not reached in the traces so far
    sta.w !hud_blank+4              ; not reached in the traces so far
    lda.b #$02                      ; not reached in the traces so far
    sta.w !HDMAEN                   ; not reached in the traces so far
    jsr.w JoypadRead                ; not reached in the traces so far
    jsl.l OamDmaAll                 ; not reached in the traces so far
    jsr.w SoundPortsSend            ; not reached in the traces so far
    rtl                             ; not reached in the traces so far
RasterStage_s10:
    sep.b #$30
    jsr.w WaitHBlank
    lda.b #$13
    sta.w !TM
    lda.w !scroll_value
    sta.w !BG2VOFS
    lda.w !wram_0E33+1
    sta.w !BG2VOFS
    lda.b #$D8
    sta.w !VTIMEL
    lda.w !TIMEUP
    lda.b #$12
    sta.w !wram_0D3F
    inc.w !frame_count
    inc.w !frames_since_tick
    jsr.w JoypadRead
    jsr.w FadeStepFixed
    jsr.w SoundPortsSend
    rtl
RasterStage_s12:
    sep.b #$30
    jsr.w WaitHBlank
    lda.b #$80
    sta.w !INIDISP
    lda.b #$37
    sta.w !VTIMEL
    lda.w !TIMEUP
    lda.b #$00
    sta.w !VTIMEH
    lda.w !wram_0D45
    sta.w !HDMAEN
    lda.w !colour_math_mode
    sta.w !CGADSUB
    lda.b #$10
    sta.w !wram_0D3F
    lda.b #$FF
    sta.w !hud_blank+4
    jsl.l OamDmaAll
    lda.w !wram_0E33
    sta.w !BG2VOFS
    lda.w !wram_0E33+1
    sta.w !BG2VOFS
    lda.l !fixed_colour
    ora.b #$80
    sta.w !COLDATA
    and.b #$7F
    ora.b #$40
    sta.w !COLDATA
    and.b #$1F
    ora.b #$20
    sta.w !COLDATA
    lda.b #$03
    sta.w !TM
    rep.b #$20
    lda.w #$1000
    sta.w !DAS0L
    lda.w #$93D7
    sta.w !A1T0L
    lda.w #$5000
    sta.w !VMADDL
    lda.w #$1801
    sta.w !DMAP0
    sep.b #$20
    lda.b #$7F
    sta.w !A1B0
    lda.b #$80
    sta.w !VMAIN
    lda.w !gsu_scmr_shadow
    and.b #$F7
    sta.w !GSU_SCMR
    lda.b #$01
    sta.w !MDMAEN
    lda.w !gsu_scmr_shadow
    ora.b #$18
    sta.w !GSU_SCMR
    rep.b #$30
    lda.w !gsu_buffer_select
    beq .lA45B
    stz.w !gsu_buffer_select
    ldx.w #$2C00
    ldy.w #$0400
    lda.w #$0800
    jsr.w VramDmaFromGsuRam
    rep.b #$20
    lda.w #$FFFF
    sta.l !wram_0D81
.lA45B:
    jsr.w WindowTable7F
    sep.b #$20
    lda.w !fade_level_hi
    sta.w !INIDISP
    rtl

; @routine 7E:A467 WindowTable7FLong
; WHAT IT DOES: WindowTable7F for a JSL.
; HOW: JSR, RTL.
; Entry: as WindowTable7F. Exit: as WindowTable7F.
%ramcode($7EA467, $01018E)
WindowTable7FLong:
    jsr.w WindowTable7F
    rtl

; @routine 7E:A46B WindowTable7F
; WHAT IT DOES: Builds the window band table at $7F:3410 from the four
; scaled values at $7F:3432 (low bytes $3433, $3435, $3437, $3439 and
; $3434, $3438): $7F:3402 = the width $3438 - $3434, $7F:3406 its middle;
; the top line $3435 / 2 (and + 1), the left edges $3433 up by one a band
; and the right edges $3437 down by one, mirrored, and the middle bands'
; height ($3439 - $3435 - 6) / 2 (the subtraction keeping its borrow).
; HOW: 8-bit long stores in the original's order.
; Entry: any widths. Exit: M 8-bit; A low byte = the middle height; N, Z,
; C from its LSR; V from the subtraction.
%ramcode($7EA46B, $010192)
WindowTable7F:
    sep.b #$20
    lda.l !scaled_words+6
    sec
    sbc.l !scaled_words+2
    sta.l !window_width_7f
    lsr a
    clc
    adc.l !scaled_words+2
    sta.l !window_middle_7f
    lda.l !scaled_words+3
    lsr a
    sta.l !window_table_7f
    inc a
    sta.l !window_table_7f+3
    lda.l !scaled_words+1
    sta.l !window_table_7f+16
    sta.l !window_table_7f+19
    inc a
    sta.l !window_table_7f+13
    sta.l !window_table_7f+22
    inc a
    sta.l !window_table_7f+10
    sta.l !window_table_7f+25
    inc a
    sta.l !window_table_7f+7
    sta.l !window_table_7f+28
    lda.l !scaled_words+5
    sta.l !window_table_7f+17
    sta.l !window_table_7f+20
    dec a
    sta.l !window_table_7f+14
    sta.l !window_table_7f+23
    dec a
    sta.l !window_table_7f+11
    sta.l !window_table_7f+26
    dec a
    sta.l !window_table_7f+8
    sta.l !window_table_7f+29
    lda.l !scaled_words+7
    sec
    sbc.l !scaled_words+3
    sbc.b #$06
    lsr a
    sta.l !window_table_7f+15
    sta.l !window_table_7f+18
    rts
RasterStage_s04:
    ldx.w !fade_level_hi
.lA4F8:
    lda.w !HVBJOY
    bit.b #$40
    beq .lA4F8
    stx.w !INIDISP
    lda.b #$30
    sta.w !HTIMEL
    lda.b #$C0
    sta.w !VTIMEL
    lda.w !TIMEUP
    lda.b #$14
    sta.w !wram_0D3F
    inc.w !frame_count
    inc.w !frames_since_tick
    lda.b #$FF
    sta.w !hud_blank+4
    jsr.w FadeStep
    jsr.w JoypadAccumulate
    jsr.w SoundPortsSend
    jsr.w RaceClockStep
    rep.b #$20
    lda.w #$01D8
    sta.w !hud_time_sprite
    jsr.w HudDrawTime
    jsr.w HudPanelsStep
    jsr.w PalettesStep
    rtl
RasterStage_s14:
    lda.b #$D0
    sta.w !VTIMEL
    lda.w !TIMEUP
    lda.b #$06
    sta.w !wram_0D3F
    jsl.l WindowHdmaNormal
    jsl.l WindowLayoutStepForScreen
    jsl.l WindowSlideOrOscillate
    jsl.l WindowSweepIfOn
    jsl.l WindowSlideCheck
    jsl.l BobWindowsRun
    jsl.l WindowBandsAnimate
    jsl.l WindowSplitAnimate
    rtl
RasterStage_s06:
    ldx.b #$80
.lA56D:
    lda.w !HVBJOY
    bit.b #$40
    beq .lA56D
    stx.w !INIDISP
    jsr.w NmiFrameUpload
    jsr.w OamDirectStage
    jsr.w RasterColoursUnless0D22
    sep.b #$20
    lda.b #$E0
    sta.w !HTIMEL
    lda.b #$10
    sta.w !VTIMEL
    lda.w !TIMEUP
    lda.b #$13
    sta.w !TM
    lda.w !wram_0D45
    sta.w !HDMAEN
    lda.b #$04
    sta.w !wram_0D3F
    lda.w !window_setup_bytes
    sta.w !COLDATA
    lda.w !wram_19AD
    sta.w !COLDATA
    lda.w !wram_19AE
    sta.w !COLDATA
    jsl.l WindowsSetupSelect
    rtl
RasterStage_s08:
    ldx.w !fade_level_hi
    jsr.w WaitHBlank
    stx.w !INIDISP
    lda.b #$6F
    sta.w !VTIMEL
    lda.b #$00
    sta.w !HTIMEH
    sta.w !VTIMEH
    lda.b #$10
    sta.w !HTIMEL
    lda.w !TIMEUP
    lda.b #$0A
    sta.w !wram_0D3F
    inc.w !frame_count
    inc.w !frames_since_tick
    lda.b #$FF
    sta.w !hud_blank+4
    jsr.w FadeStep
    jsr.w RaceClockStep
    sep.b #$20
    lda.b #$13
    sta.w !TM
    rep.b #$20
    lda.w #$0100
    sta.w !hud_time_sprite
    jsr.w HudDrawTime
    jsr.w HudPanelASlide
    jsr.w HudPanelBSlide
    jsr.w SoundPortsSend
    sep.b #$30
    ldy.b #$01
    ldx.b #$02
    jsr.w PaletteKeptColoursStep
    rtl
RasterStage_s0A:
    ldx.b #$80
    jsr.w WaitHBlank
    nop
    stx.w !INIDISP
    lda.b #$77
    sta.w !VTIMEL
    lda.w !TIMEUP
    lda.b #$0C
    sta.w !wram_0D3F
    lda.w !wram_19AF
    sta.w !COLDATA
    lda.w !wram_19B0
    sta.w !COLDATA
    lda.w !wram_19B1
    sta.w !COLDATA
    lda.w !wram_0D5D
    beq .lA669
    bmi .lA64A
    lda.b #$80
    sta.w !BG2VOFS
    lda.b #$01
    sta.w !BG2VOFS
    bra .lA654
.lA64A:
    lda.b #$00
    sta.w !BG2VOFS
    lda.b #$00
    sta.w !BG2VOFS
.lA654:
    lda.b #$A0
    sta.w !BG2HOFS
    lda.b #$01
    sta.w !BG2HOFS
    lda.b #$78
    sta.w !BG2SC
    lda.b #$70
    sta.w !BG12NBA
    rtl
.lA669:
    lda.w !wram_0D5C
    beq .lA673
    lda.b #$40
    sta.w !BG12NBA
.lA673:
    lda.b #$3F
    sta.w !BG2SC
    ldx.b #$02
    jsr.w Bg2ScrollFromTable
    jsr.w RasterColours
    rtl
RasterStage_s0C:
    ldx.w !fade_level_hi
    jsr.w WaitHBlank
    stx.w !INIDISP
    lda.b #$D7
    sta.w !VTIMEL
    lda.w !TIMEUP
    lda.b #$0E
    sta.w !wram_0D3F
    rep.b #$20
    lda.w #$0000
    sta.w !hud_time_sprite
    jsr.w HudDrawTime
    sep.b #$30
    ldy.b #$00
    ldx.b #$00
    jsr.w PaletteKeptColoursStep
    jsl.l WindowSplitAnimate
    rtl
RasterStage_s0E:
    ldx.b #$80
    jsr.w WaitHBlank
    stx.w !INIDISP
    jsr.w NmiFrameUpload
    jsl.l OamDmaAll
    jsl.l WindowLayoutStepForScreen
    jsr.w RasterPaletteUpload
    jsr.w JoypadAccumulate
    jsl.l WindowHdmaBands
    jsl.l WindowLayoutStepForScreen
    jsl.l BobWindowsRun
    jsl.l WindowBandsAnimate
    sep.b #$30
    lda.w !wram_0D5C
    beq .lA70E
    bmi .lA6EE
    lda.b #$E8
    sta.w !BG2VOFS
    lda.b #$01
    sta.w !BG2VOFS
    bra .lA6F8
.lA6EE:
    lda.b #$68                      ; not reached in the traces so far
    sta.w !BG2VOFS                  ; not reached in the traces so far
    lda.b #$00                      ; not reached in the traces so far
    sta.w !BG2VOFS                  ; not reached in the traces so far
.lA6F8:
    lda.b #$F0
    sta.w !BG2HOFS
    lda.b #$01
    sta.w !BG2HOFS
    lda.b #$78
    sta.w !BG2SC
    lda.b #$70
    sta.w !BG12NBA
    bra .lA727
.lA70E:
    lda.w !wram_0D5D
    beq .lA718
    lda.b #$40
    sta.w !BG12NBA
.lA718:
    ldx.b #$00
    jsr.w Bg2ScrollFromTable
    lda.b #$1C
    sta.w !BG1SC
    lda.b #$3F
    sta.w !BG2SC
.lA727:
    lda.b #$01
    sta.w !BGMODE
    lda.b #$03
    sta.w !TM
    lda.b #$1C
    sta.w !HDMAEN
    lda.w !window_setup_bytes
    sta.w !COLDATA
    lda.w !wram_19AD
    sta.w !COLDATA
    lda.w !wram_19AE
    sta.w !COLDATA
    jsl.l WindowsSetupSelect
    lda.b #$0F
    sta.w !VTIMEL
    lda.w !TIMEUP
    lda.b #$08
    sta.w !wram_0D3F
    jsr.w GradientPointersToBank7F
RasterStage_s18:
    rtl
RasterStage_s1C:
    lda.b #$10                      ; not reached in the traces so far
    sta.w !VTIMEL                   ; not reached in the traces so far
    rtl                             ; not reached in the traces so far
RasterStage_s1E:
    jsr.w WaitHBlank
    lda.b #$80
    sta.w !INIDISP
    jsl.l VblankUpdates
    jsr.w FrameSoundAndJoypads
    jsl.l FrameCountAndPortsEnd
    sep.b #$20
    lda.b #$E0
    sta.w !VTIMEL
    lda.w !TIMEUP
    rtl
RasterStage_s20:
    jsr.w WaitHBlank
    lda.b #$80
    sta.w !INIDISP
    jsl.l AnimTilesQueueNext
    jsl.l VblankUpdates
    jsr.w FrameSoundAndJoypads
    jsl.l FrameInputAndCountEnd
    sep.b #$20
    lda.b #$E0
    sta.w !VTIMEL
    lda.w !TIMEUP
    rtl


; @routine 7E:F14F TimeMarkStart
; WHAT IT DOES: Records the time now (frame and line, as time_stamp says)
; in the word at $1A39 + X: the start of a timed stretch of a frame.
; Nothing in the game calls it.
; HOW: Long accesses to bank $00; see time_stamp.
; Entry: X 16-bit; X = the slot's offset. Exit: M 16-bit; A = the time;
; X, Y unchanged; I clear; N, Z, C, V from the last add.
%ramcode($7EF14F, $014E76)
TimeMarkStart:
    rep.b #$20
    sei
    lda.l !frames_since_tick
    xba
    and.w #$FF00
    sta.l !time_now
    sep.b #$20
    lda.l !SLHV
    cli
    lda.l !OPVCT
    xba
    lda.l !OPVCT
    xba
    rep.b #$20
    and.w #$01FF
    cmp.w #$0100
    bmi .lF17C
    lda.w #$00FF
.lF17C:
    cmp.w #$0011
    bpl .lF185
    clc
    adc.w #$0100
.lF185:
    clc
    adc.l !time_now
    sta.l !time_marks,x
    rtl

; @routine 7E:F18F TimeMarkEnd
; WHAT IT DOES: Adds the time since the start recorded at $1A39 + X to the
; running total at $1A37 + X. A start late in its frame (low byte $D1 or
; more) is taken as is; an earlier one first has the frames between, each
; as 64 lines, taken off the end time. Nothing in the game calls it.
; HOW: The end time in $1A37 (time_stamp); the start's frame byte kept in
; $1A39 for the correction (read back through the data bank); 16-bit
; long adds and subtracts.
; Entry: X 16-bit; X = the slot's offset; data bank $00. Exit: M 16-bit;
; A = the new total; X, Y unchanged; I clear; N, Z, C, V from the last
; add.
TimeMarkEnd:
    rep.b #$20
    sei
    lda.l !frames_since_tick
    xba
    and.w #$FF00
    sta.l !time_now
    sep.b #$20
    lda.l !SLHV
    cli
    lda.l !OPVCT
    xba
    lda.l !OPVCT
    xba
    rep.b #$20
    and.w #$01FF
    cmp.w #$0100
    bmi .lF1BC
    lda.w #$00FF
.lF1BC:
    cmp.w #$0011
    bpl .lF1C5
    clc
    adc.w #$0100
.lF1C5:
    clc
    adc.l !time_now
    sta.l !time_now
    lda.l !time_marks,x
    and.w #$00FF
    cmp.w #$00D1
    bpl .lF204
    lda.l !time_marks,x
    and.w #$FF00
    sta.l !time_marks
    lda.l !time_now
    and.w #$FF00
    sec
    sbc.w !time_marks
    xba
    asl a
    asl a
    asl a
    asl a
    asl a
    asl a
    eor.w #$FFFF
    inc a
    clc
    adc.l !time_now
    sta.l !time_now
.lF204:
    lda.l !time_now
    sec
    sbc.l !time_marks,x
    clc
    adc.l !time_now,x
    sta.l !time_now,x
    rtl
