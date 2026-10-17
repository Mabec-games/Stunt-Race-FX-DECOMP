; pause.asm: the stages of the race's pause screens, one step a frame: the
; pause by pad 1 or pad 2 (OamStagePause, OamStagePause2, through the tables
; !oam_stage_handlers_bb3c and !oam_stage_handlers_bba2) and the replay's
; pause (PauseReplayStages). The stage is $0D2F, its timer $0D5F; the
; continue / quit choice is $0D3D and the quit menu's cursor $0D50 (0, 2, 4;
; 6 for the cars). All run with 8-bit registers and data bank $07.
; C: src/stuntracefx/race/pause.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 07:BC1C PauseReplayStages
; WHAT IT DOES: Runs the replay pause's stage $0D2F from .table:
; PauseStageStart, PauseStageWait, PauseStageChoice, PauseStageReplayEnd,
; PauseStageQuit, PauseStageQuitWait, PauseStageLeave.
; HOW: SEP #$30, JMP (table,X).
; Entry: any widths. Exit: as the stage leaves it.
%at($07BC1C)
PauseReplayStages:
    sep.b #$30
    ldx.w !wram_0D2F
    jmp.w (.table,x)
.table:
    dw PauseStageStart, PauseStageWait, PauseStageChoice, PauseStageReplayEnd, PauseStageQuit, PauseStageQuitWait, PauseStageLeave

; @routine 07:BC32 PauseStageStart
; WHAT IT DOES: The pause's first stage: the timer $0D5F = $A, stage 2.
; HOW: 8-bit stores.
; Entry: any widths. Exit: M 8-bit; A low byte = 2.
%at($07BC32)
PauseStageStart:
    sep.b #$20
    lda.b #$0A
    sta.w !wram_0D5D+2
    lda.b #$02
    sta.w !wram_0D2F
    rts

; @routine 07:BC3F PauseStageWait
; WHAT IT DOES: Counts the timer $0D5F down; at its end stage 4.
; HOW: DEC of the byte.
; Entry: any widths. Exit: M 8-bit.
%at($07BC3F)
PauseStageWait:
    sep.b #$20
    dec.w !wram_0D5D+2
    bne .lBC4B
    lda.b #$04
    sta.w !wram_0D2F
.lBC4B:
    rts

; @routine 07:BC4C PauseStageResumeB
; WHAT IT DOES: Counts the timer $0D5F down; at its end gauge 6B placed
; (Gauge6BPlace) and the race resumed: sound $16 on port 1, the presses' low
; bytes and $0D2D cleared, $0E5B and $0E5F one less.
; HOW: 8-bit.
; Entry: any widths. Exit: M 8-bit.
%at($07BC4C)
PauseStageResumeB:
    sep.b #$20
    dec.w !wram_0D5D+2
    bne PauseStageReplayEnd_lBC9F
    jsr.w Gauge6BPlace
    bra PauseStageReplayEnd_lBC88

; @routine 07:BC58 PauseStageResumeA
; WHAT IT DOES: As PauseStageResumeB with gauge 6A (Gauge6APlace).
; HOW: As PauseStageResumeB. Reached only by unit_pause.
; Entry: any widths. Exit: M 8-bit.
%at($07BC58)
PauseStageResumeA:
    sep.b #$20
    dec.w !wram_0D5D+2
    bne PauseStageReplayEnd_lBC9F
    jsr.w Gauge6APlace
    bra PauseStageReplayEnd_lBC88

; @routine 07:BC64 PauseStageReplayEnd
; WHAT IT DOES: Counts the timer $0D5F down: at 1 the sprite rows
; (OamPlaceRows75And112); at its end $0E14 = $93, $0E0C = $7C, $0E18 = $8C,
; $0D22 cleared and the race resumed (as PauseStageResumeB).
; HOW: 8-bit.
; Entry: any widths. Exit: M 8-bit.
%at($07BC64)
PauseStageReplayEnd:
    sep.b #$20
    dec.w !wram_0D5D+2
    beq .lBC76
    lda.w !wram_0D5D+2
    cmp.b #$01
    bne .lBC9F
    jsr.w OamPlaceRows75And112
    rts
.lBC76:
    lda.b #$93
    sta.w !wram_0E14
    lda.b #$7C
    sta.w !wram_0E0C
    lda.b #$8C
    sta.w !wram_0E16+2
    stz.w !wram_0D22
.lBC88:
    lda.b #$16
    jsl.l SoundPort1QueuePush
    stz.w !joy1_pressed_copy
    stz.w !joy2_pressed_copy
    sep.b #$20
    stz.w !wram_0D2D
    dec.w !wram_0E5B
    dec.w !palette_animate
.lBC9F:
    rts

; @routine 07:BCA0 PauseStageQuit
; WHAT IT DOES: Counts the timer $0D5F down; at its end slides the window up:
; in mode 0 stage $A with $19DF = 1 and the timer $70; in other modes stage
; $C with $19DF cleared.
; HOW: 8-bit.
; Entry: any widths. Exit: M 8-bit.
%at($07BCA0)
PauseStageQuit:
    sep.b #$20
    dec.w !wram_0D5D+2
    beq .lBCA8
    rts
.lBCA8:
    lda.w !hud_mode
    bne .lBCC1
    lda.b #$0A
    sta.w !wram_0D2F
    jsl.l WindowSlideUpSound
    lda.b #$01
    sta.w !wram_19DF
    lda.b #$70
    sta.w !wram_0D5D+2
    rts
.lBCC1:
    lda.b #$0C
    sta.w !wram_0D2F
    jsl.l WindowSlideUpSound
    lda.b #$00
    sta.w !wram_19DF
    rts

; @routine 07:BCD0 PauseStageQuitWait
; WHAT IT DOES: Counts the timer $0D5F down; at its end the race again
; (RaceRestart) with $0D24 set, else PauseStageLeave.
; HOW: 8-bit; jumps on.
; Entry: any widths. Exit: M 8-bit, or does not return.
%at($07BCD0)
PauseStageQuitWait:
    sep.b #$20
    dec.w !wram_0D5D+2
    bne .lBCDF
    lda.w !wram_0D24
    beq PauseStageLeave
    jmp.w RaceRestart
.lBCDF:
    rts

; @routine 07:BCE0 PauseStageLeave
; WHAT IT DOES: Leaves the race from the pause: with $10CD set, the fade out
; ($0E3A = 1) and, once it is done ($0D61 negative), the restart
; (SoftResetToBootWarm); else $0D59 cleared, the championship screen's loop,
; then the race again (RaceRestart, with $0DD4) or the video reset
; (RaceEndVideoReset).
; HOW: 8-bit; jumps on. The race again is not reached in the traces so far.
; Entry: any widths. Exit: M 8-bit, or does not return.
%at($07BCE0)
PauseStageLeave:
    sep.b #$20
    lda.w !wram_10CD
    beq .lBCF5
    lda.b #$01
    sta.w !fade_out
    lda.w !fade_level_hi
    bpl .lBCF4
    jmp.w SoftResetToBootWarm
.lBCF4:
    rts
.lBCF5:
    stz.w !wram_0D59
    jsr.w ChampionshipScreenLoop
    sep.b #$20
    lda.w !sound_port1_last+2
    beq .lBD05
    jmp.w RaceRestart
.lBD05:
    jmp.w RaceEndVideoReset

; @routine 07:BD08 PauseStageLeaveRts
; WHAT IT DOES: The RTS after PauseStageLeave's last JMP, which no code
; reaches: returns at once.
; HOW: RTS.
; Entry: any. Exit: everything unchanged.
%at($07BD08)
    rts

; @routine 07:BD09 PauseStageChoice
; WHAT IT DOES: The replay pause's continue / quit choice for pad 1: left and
; right move it ($0D3D, sprites 52-56: OamBlock52Left / OamBlock52Right,
; sound $45); B or START takes it: $19BD = $FF, the timer 8, stage 8 for
; quit (in mode 0 without the override $0D4C also Countdown0DF8 and sound
; $22 on port 1) or 6 for continue.
; HOW: The presses 16-bit.
; Entry: any widths. Exit: M 8-bit, or 16-bit when nothing moved.
%at($07BD09)
PauseStageChoice:
    rep.b #$20
    lda.w !joy1_pressed_copy
    bit.w #$9000
    beq .lBD16
    jmp.w .lBD51
.lBD16:
    sep.b #$20
    lda.w !wram_0D3D
    bne .lBD37
    rep.b #$20
    lda.w !joy1_pressed_copy
    bit.w #$0200
    beq .lBD50
    lda.w #$0045
    jsl.l SoundPort0QueuePush
    sep.b #$20
    dec.w !wram_0D3D
    jsr.w OamBlock52Left
    rts
.lBD37:
    rep.b #$20
    lda.w !joy1_pressed_copy
    bit.w #$0100
    beq .lBD50
    lda.w #$0045
    jsl.l SoundPort0QueuePush
    sep.b #$20
    stz.w !wram_0D3D
    jsr.w OamBlock52Right
.lBD50:
    rts
.lBD51:
    sep.b #$20
    lda.w !wram_0D3D
    beq .lBD7B
    lda.b #$FF
    sta.w !layout_request
    lda.b #$08
    sta.w !wram_0D2F
    lda.b #$08
    sta.w !wram_0D5D+2
    lda.w !hud_mode
    bne .lBD7A
    lda.w !digit62_override
    bne .lBD7A
    jsr.w Countdown0DF8
    lda.b #$22
    jsl.l SoundPort1QueuePush
.lBD7A:
    rts
.lBD7B:
    sep.b #$20
    lda.b #$FF
    sta.w !layout_request
    lda.b #$06
    sta.w !wram_0D2F
    lda.b #$08
    sta.w !wram_0D5D+2
    rts

; @routine 07:BD8D PauseStageChoiceP1
; WHAT IT DOES: The pause's continue / quit choice for pad 1: left and right
; move it ($0D3D, sprites 59-63: OamBlock59Left / OamBlock59Right, sound
; $45); B or START takes it: $19BD = $FF, the timer 8, stage 8 for quit or 6
; for continue.
; HOW: The presses 16-bit; $0D3D read as a word, masked. The move right is
; not reached in the traces so far.
; Entry: any widths. Exit: M 8-bit, or 16-bit when nothing moved.
%at($07BD8D)
PauseStageChoiceP1:
    rep.b #$20
    lda.w !joy1_pressed_copy
    bit.w #$9000
    bne .lBDD2
    lda.w !wram_0D3D
    and.w #$00FF
    bne .lBDB7
    lda.w !joy1_pressed_copy
    bit.w #$0200
    beq .lBDD1
    lda.w #$0045
    jsl.l SoundPort0QueuePush
    sep.b #$20
    dec.w !wram_0D3D
    jsr.w OamBlock59Left
    rts
.lBDB7:
    rep.b #$20
    lda.w !joy1_pressed_copy
    bit.w #$0100
    sep.b #$20
    beq .lBDD1
    lda.b #$45
    jsl.l SoundPort0QueuePush
    sep.b #$20
    stz.w !wram_0D3D
    jsr.w OamBlock59Right
.lBDD1:
    rts
.lBDD2:
    sep.b #$20
    lda.w !wram_0D3D
    beq .lBDE9
    lda.b #$FF
    sta.w !layout_request
    lda.b #$08
    sta.w !wram_0D2F
    lda.b #$08
    sta.w !wram_0D5D+2
    rts
.lBDE9:
    sep.b #$20
    lda.b #$FF
    sta.w !layout_request
    lda.b #$06
    sta.w !wram_0D2F
    lda.b #$08
    sta.w !wram_0D5D+2
    rts

; @routine 07:BDFB PauseStageChoiceP2
; WHAT IT DOES: As PauseStageChoiceP1 for pad 2 (sprites 123-127:
; OamBlock123Left / OamBlock123Right).
; HOW: As PauseStageChoiceP1. Reached only by unit_pause.
; Entry: any widths. Exit: as PauseStageChoiceP1.
%at($07BDFB)
PauseStageChoiceP2:
    rep.b #$20
    lda.w !joy2_pressed_copy
    bit.w #$9000
    bne PauseStageChoiceP1_lBDD2
    lda.w !wram_0D3D
    and.w #$00FF
    bne .lBE25
    lda.w !joy2_pressed_copy
    bit.w #$0200
    beq .lBE3F
    lda.w #$0045
    jsl.l SoundPort0QueuePush
    sep.b #$20
    dec.w !wram_0D3D
    jsr.w OamBlock123Left
    rts
.lBE25:
    rep.b #$20
    lda.w !joy2_pressed_copy
    bit.w #$0100
    sep.b #$20
    beq .lBE3F
    lda.b #$45
    jsl.l SoundPort0QueuePush
    sep.b #$20
    stz.w !wram_0D3D
    jsr.w OamBlock123Right
.lBE3F:
    rts

; @routine 07:BE40 PauseStageContinueP2
; WHAT IT DOES: Counts the timer $0D5F down; at its end $0D2E = $FF (pad 2's),
; $19DF cleared, the window slid up and stage $A.
; HOW: 8-bit. Reached only by unit_pause.
; Entry: any widths. Exit: M 8-bit.
%at($07BE40)
PauseStageContinueP2:
    sep.b #$20
    lda.w !wram_0D5D+2
    beq .lBE4B
    dec.w !wram_0D5D+2
    rts
.lBE4B:
    lda.b #$FF
    sta.w !wram_0D2E
    bra PauseStageContinue_lBE62

; @routine 07:BE52 PauseStageContinue
; WHAT IT DOES: As PauseStageContinueP2 with $0D2E cleared (pad 1's).
; HOW: 8-bit.
; Entry: any widths. Exit: M 8-bit.
%at($07BE52)
PauseStageContinue:
    sep.b #$20
    lda.w !wram_0D5D+2
    beq .lBE5D
    dec.w !wram_0D5D+2
    rts
.lBE5D:
    lda.b #$00
    sta.w !wram_0D2E
.lBE62:
    lda.b #$00
    sta.w !wram_19DF
    jsl.l WindowSlideUpSound
    lda.b #$0A
    sta.w !wram_0D2F
    rts

; @routine 07:BE71 PauseStageQuitMenuP2
; WHAT IT DOES: Once the split window is closed: $0D2E = $FF, $0D5D = 1, $0D23
; = $E, sprites 58 up parked (OamHighBits58Park112), and the quit menu
; opened ($19DF cleared, window state 2, cursor $0D50 cleared, stage $C,
; $0D22 = $C).
; HOW: 8-bit.
; Entry: M 8-bit. Exit: M 8-bit.
%at($07BE71)
PauseStageQuitMenuP2:
    lda.w !window_hdma_off
    cmp.b #$05
    bne .lBE8E
    sep.b #$20
    lda.b #$FF
    sta.w !wram_0D2E
    lda.b #$01
    sta.w !wram_0D5D
    lda.b #$0E
    sta.w !wram_0D23
    jsr.w OamHighBits58Park112
    bra PauseStageQuitMenu_lBEAB
.lBE8E:
    rts

; @routine 07:BE8F PauseStageQuitMenu
; WHAT IT DOES: As PauseStageQuitMenuP2 for pad 1: $0D2E cleared, $0D5C = 1,
; $0D23 = $C, sprites 18 up parked (OamHighBits18Park48).
; HOW: 8-bit.
; Entry: M 8-bit. Exit: M 8-bit.
%at($07BE8F)
PauseStageQuitMenu:
    lda.w !window_hdma_off
    cmp.b #$05
    beq .lBE97
    rts
.lBE97:
    sep.b #$20
    lda.b #$00
    sta.w !wram_0D2E
    lda.b #$01
    sta.w !wram_0D5C
    lda.b #$0C
    sta.w !wram_0D23
    jsr.w OamHighBits18Park48
.lBEAB:
    lda.b #$00
    sta.w !wram_19DF
    lda.b #$02
    sta.w !window_hdma_off
    lda.b #$0A
    sta.w !wram_0D2F
    stz.w !hud_blank+2
    lda.b #$0C
    sta.w !wram_0D2F
    sta.w !wram_0D22
    rts

; @routine 07:BEC6 PauseStageQuitTaken
; WHAT IT DOES: Once the split window is closed: the race again (RaceRestart)
; with the cursor $0D50 not 0, else RaceEndToAttract.
; HOW: 8-bit; jumps on.
; Entry: any widths. Exit: M 8-bit, or does not return.
%at($07BEC6)
PauseStageQuitTaken:
    sep.b #$20
    lda.w !window_hdma_off
    cmp.b #$05
    bne .lBEDA
    lda.w !hud_blank+2
    beq .lBED7
    jmp.w RaceRestart
.lBED7:
    jmp.w RaceEndToAttract
.lBEDA:
    rts

; @routine 07:BEDB PauseQuitMenuP2
; WHAT IT DOES: The quit menu for pad 2: see PauseQuitMenu; the cars first
; opened set $0D5C = $FF, $0D23 = $C and park sprites 18 up
; (OamHighBits18Park48); the cars drawn with SpritePiecesTwoCarsTop.
; HOW: JMP (table,X) on the cursor. Reached only by unit_pause.
; Entry: any widths. Exit: as the path leaves it.
%at($07BEDB)
PauseQuitMenuP2:
    sep.b #$30
    rep.b #$20
    lda.w !joy2_pressed_copy
    bit.w #$9000
    sep.b #$20
    beq .lBF33
    lda.w !hud_blank+2
    cmp.b #$04
    beq .lBF10
    bcs .lBF06
    lda.b #$46
    jsl.l SoundPort0QueuePush
    lda.b #$0E
    sta.w !wram_0D2F
    lda.b #$01
    sta.w !wram_19DF
    sta.w !window_hdma_off
    rts
.lBF06:
    lda.b #$46
    jsl.l SoundPort0QueuePush
    stz.w !hud_blank+2
    rts
.lBF10:
    lda.w !wram_10BA
    bne .lBF27
    lda.b #$FF
    sta.w !wram_10BA
    lda.b #$FF
    sta.w !wram_0D5C
    lda.b #$0C
    sta.w !wram_0D23
    jsr.w OamHighBits18Park48
.lBF27:
    lda.b #$46
    jsl.l SoundPort0QueuePush
    lda.b #$06
    sta.w !hud_blank+2
    rts
.lBF33:
    ldx.w !hud_blank+2
    jmp.w (.cursor,x)
.cursor:
    dw .lBF49, .lBF6C, .lBFA6, .lBF41
.lBF41:
    jsr.w CarSelectTwoPlayers
    jsr.w SpritePiecesTwoCarsTop
    bra .lBFBD
.lBF49:
    rep.b #$20
    lda.w !joy2_pressed_copy
    bit.w #$0400
    beq .lBF60
    sep.b #$20
    lda.b #$45
    jsl.l SoundPort0QueuePush
    lda.b #$02
    sta.w !hud_blank+2
.lBF60:
    sep.b #$20
    jsr.w TilemapPaletteCycle87
    jsr.w TilemapRow107Palette5
    jsr.w TilemapRow187Palette5
    rts
.lBF6C:
    rep.b #$20
    lda.w !joy2_pressed_copy
    bit.w #$0400
    beq .lBF85
    sep.b #$20
    lda.b #$45
    jsl.l SoundPort0QueuePush
    lda.b #$04
    sta.w !hud_blank+2
    bra .lBF9A
.lBF85:
    lda.w !joy2_pressed_copy
    bit.w #$0800
    beq .lBF9A
    sep.b #$20
    lda.b #$45
    jsl.l SoundPort0QueuePush
    lda.b #$00
    sta.w !hud_blank+2
.lBF9A:
    sep.b #$20
    jsr.w TilemapRow87Palette5
    jsr.w TilemapPaletteCycle107
    jsr.w TilemapRow187Palette5
    rts
.lBFA6:
    rep.b #$20
    lda.w !joy2_pressed_copy
    bit.w #$0800
    beq .lBFBD
    sep.b #$20
    lda.b #$45
    jsl.l SoundPort0QueuePush
    lda.b #$02
    sta.w !hud_blank+2
.lBFBD:
    sep.b #$20
    jsr.w TilemapPaletteCycle187
    jsr.w TilemapRow107Palette5
    jsr.w TilemapRow87Palette5
    rts

; @routine 07:BFC9 PauseQuitMenu
; WHAT IT DOES: The quit menu for pad 1: B or START takes the cursor $0D50 (0
; closes the menu: stage $E, $19DF = $19D8 = 1; 4 shows the cars, cursor 6,
; the first time also $10BA = $FF, $0D5D = $FF, $0D23 = $E and sprites 58
; up parked; 2 and 6 go back to 0; sound $46); else down and up move the
; cursor (sound $45) and the rows show it (the cursor's row cycling its
; palette); at 6 the cars (CarSelectTwoPlayers, SpritePiecesTwoCars). The
; bytes at 07:C021 (cursor 0, RTS: PauseQuitMenuCursor0) and 07:C03E
; (sound $46, cursor 0: PauseQuitMenuBack) are reached by no code.
; HOW: JMP (table,X) on the cursor. The move up from 2 is not reached in the
; traces so far.
; Entry: any widths. Exit: as the path leaves it.
%at($07BFC9)
PauseQuitMenu:
    sep.b #$30
    rep.b #$20
    lda.w !joy1_pressed_copy
    bit.w #$9000
    sep.b #$20
    beq .lC027
    lda.w !hud_blank+2
    cmp.b #$04
    beq .lBFFE
    bcs .lBFF4
    lda.b #$46
    jsl.l SoundPort0QueuePush
    lda.b #$0E
    sta.w !wram_0D2F
    lda.b #$01
    sta.w !wram_19DF
    sta.w !window_hdma_off
    rts
.lBFF4:
    lda.b #$46
    jsl.l SoundPort0QueuePush
    stz.w !hud_blank+2
    rts
.lBFFE:
    lda.w !wram_10BA
    bne .lC015
    lda.b #$FF
    sta.w !wram_10BA
    lda.b #$FF
    sta.w !wram_0D5D
    lda.b #$0E
    sta.w !wram_0D23
    jsr.w OamHighBits58Park112
.lC015:
    lda.b #$46
    jsl.l SoundPort0QueuePush
    lda.b #$06
    sta.w !hud_blank+2
    rts

; @routine 07:C021 PauseQuitMenuCursor0
; WHAT IT DOES: Bytes in PauseQuitMenu after an RTS that no code reaches:
; the quit menu's cursor $0D50 = 0.
; HOW: LDA #0, STA, RTS.
; Entry: M 8-bit. Exit: A low byte 0; Z set, N clear.
%at($07C021)
    lda.b #$00
    sta.w !hud_blank+2
    rts
.lC027:
    ldx.w !hud_blank+2
    jmp.w (.cursor,x)
.cursor:
    dw .lC04A, .lC06D, .lC0A7, .lC035
.lC035:
    jsr.w CarSelectTwoPlayers
    jsr.w SpritePiecesTwoCars
    jmp.w .lC0BE
.lC03E:

; @routine 07:C03E PauseQuitMenuBack
; WHAT IT DOES: Bytes in PauseQuitMenu that no code reaches: sound $46 and
; the quit menu's cursor $0D50 = 0.
; HOW: SEP #$20, LDA #$46, JSL SoundPort0QueuePush, STZ, RTS.
; Entry: any widths. Exit: M 8-bit; as SoundPort0QueuePush leaves the
; rest.
%at($07C03E)
    sep.b #$20
    lda.b #$46
    jsl.l SoundPort0QueuePush
    stz.w !hud_blank+2
    rts
.lC04A:
    rep.b #$20
    lda.w !joy1_pressed_copy
    bit.w #$0400
    beq .lC061
    sep.b #$20
    lda.b #$45
    jsl.l SoundPort0QueuePush
    lda.b #$02
    sta.w !hud_blank+2
.lC061:
    sep.b #$20
    jsr.w TilemapPaletteCycle87
    jsr.w TilemapRow107Palette5
    jsr.w TilemapRow187Palette5
    rts
.lC06D:
    rep.b #$20
    lda.w !joy1_pressed_copy
    bit.w #$0400
    beq .lC086
    sep.b #$20
    lda.b #$45
    jsl.l SoundPort0QueuePush
    lda.b #$04
    sta.w !hud_blank+2
    bra .lC09B
.lC086:
    lda.w !joy1_pressed_copy
    bit.w #$0800
    beq .lC09B
    sep.b #$20
    lda.b #$45
    jsl.l SoundPort0QueuePush
    lda.b #$00
    sta.w !hud_blank+2
.lC09B:
    sep.b #$20
    jsr.w TilemapRow87Palette5
    jsr.w TilemapPaletteCycle107
    jsr.w TilemapRow187Palette5
    rts
.lC0A7:
    rep.b #$20
    lda.w !joy1_pressed_copy
    bit.w #$0800
    beq .lC0BE
    sep.b #$20
    lda.b #$45
    jsl.l SoundPort0QueuePush
    lda.b #$02
    sta.w !hud_blank+2
.lC0BE:
    sep.b #$20
    jsr.w TilemapPaletteCycle187
    jsr.w TilemapRow107Palette5
    jsr.w TilemapRow87Palette5
    rts
