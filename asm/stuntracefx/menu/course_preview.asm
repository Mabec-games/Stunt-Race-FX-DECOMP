; course_preview.asm: the menu objects that run the course preview, a race
; frame (RaceFrameStep) drawn each menu frame behind the course select.
; C: src/stuntracefx/menu/course_preview.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 0B:BC9A MenuPreviewIdle
; WHAT IT DOES: The course preview while the menu waits: two fixed colour
; steps down, a race frame shown, $18FD = 1; unless $1905 is set, makes
; $BCD2 the object's handler, repeats the choice (MenuChoiceRepeat), sets
; $1A66 from the previous selection ($18C9, MenuSet1A66) and the pair's
; animations (MenuSetPairAnimations), and shows another race frame.
; HOW: Calls in turn; the object is the current one ($11F9).
; Entry: M 16-bit, X 16-bit, D = $0000. Exit: as the last call leaves it.
%at($0BBC9A)
MenuPreviewIdle:
    jsr.w MenuFixedColourDown
    jsr.w MenuFixedColourDown
    lda.w #$0000
    jsl.l RaceFrameStep
    lda.w #$0001
    sta.l !wram_18FD
    lda.l !menu_1905
    beq .not_held
    rts
.not_held:
    ldx.w !actor_current
    lda.w #$BCD2
    sta.b !actor_handler,x
    jsr.w MenuChoiceRepeat
    lda.l !selection_prev
    jsr.w MenuSet1A66
    jsr.w MenuSetPairAnimations
    lda.w #$0000
    jsl.l RaceFrameStep
    rts

; @routine 0B:BCF1 MenuPreviewStart
; WHAT IT DOES: The first frame of the course preview's countdown: two
; fixed colour steps down, a race frame shown, $18FD = 1; unless $1905 is
; set, makes MenuPreviewCountdown ($BD55) the object's handler with a count
; of 20 (+$0A), starts animation $D8 on the kind 4 object (unless it has
; it), clears +$10 of groups 4 and 5 (MenuActorsSet10 with Y = 0) and asks
; for HDMA channels 6 and 7 ($C0 to HDMAEN on the frame write list).
; HOW: Calls in turn; the object is the current one ($11F9).
; Entry: M 16-bit, X 16-bit, D = $0000. Exit: as FrameByteWritePush
; leaves it, with X the kind 4 object and M 16-bit.
%at($0BBCF1)
MenuPreviewStart:
    jsr.w MenuFixedColourDown
    jsr.w MenuFixedColourDown
    lda.w #$0000
    jsl.l RaceFrameStep
    lda.w #$0001
    sta.l !wram_18FD
    lda.l !menu_1905
    beq .not_held
    rts
.not_held:
    ldx.w !actor_current
    lda.w #$BD55
    sta.b !actor_handler,x
    lda.w #$0014
    sta.b !actor_0a,x
    lda.w #$0004
    jsr.w MenuActorFind
    lda.b !actor_anim,x
    cmp.w #$00D8
    beq .animated
    lda.w #$00D8
    sta.b !actor_anim,x
    stz.b !actor_anim_pos,x
    stz.b !actor_anim_timer,x
.animated:
    lda.w #$0004
    ldy.w #$0000
    jsr.w MenuActorsSet10
    lda.w #$0005
    ldy.w #$0000
    jsr.w MenuActorsSet10
    sep.b #$20
    lda.b #$C0
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #!HDMAEN
    jsl.l FrameByteWritePush
    plx
    rep.b #$20
    rts

; @routine 0B:BD55 MenuPreviewCountdown
; WHAT IT DOES: The course preview's countdown: steps the object's count
; (+$0A) down, with sound $3F at 19, 14, 9 and 4, and at 0 starts
; animation $90 on the kind 4 object; then a fixed colour step down, a race
; frame shown and $18FD = 1, and player 2's choice keys
; (MenuChoicePressesP2): with $9000 pressed it ends the menu ($11EB = 1,
; $18FD = 0, sound $46, the object freed); with $4000 it sets groups 4 and
; 5 to $FFFF (MenuActorsSet10), plays sound $47, makes $BE29 the object's
; handler, sets $1905 = 1, $1907 = $28 and $7E:FD65 = $464A, and asks for
; HDMA channels 5, 6 and 7 ($E0).
; HOW: The count compares through 16-bit A; the object is reloaded from
; $11F9 after the kind 4 search. The $4000 case is not reached in the
; traces so far.
; Entry: M 16-bit, X 16-bit, D = $0000; X = the object. Exit: as the last
; call leaves it, M 16-bit.
%at($0BBD55)
MenuPreviewCountdown:
    lda.b !actor_0a,x
    beq .frame
    dec a
    sta.b !actor_0a,x
    cmp.w #$0013
    beq .tick_sound
    cmp.w #$000E
    beq .tick_sound
    cmp.w #$0009
    beq .tick_sound
    cmp.w #$0004
    bne .counted
.tick_sound:
    sep.b #$20
    lda.b #$3F
    jsl.l SoundPort0QueuePushRaw
    rep.b #$20
.counted:
    lda.b !actor_0a,x
    bne .frame
    lda.w #$0004
    jsr.w MenuActorFind
    lda.b !actor_anim,x
    cmp.w #$0090
    beq .frame
    lda.w #$0090
    sta.b !actor_anim,x
    stz.b !actor_anim_pos,x
    stz.b !actor_anim_timer,x
.frame:
    ldx.w !actor_current
    jsr.w MenuFixedColourDown
    lda.w #$0000
    jsl.l RaceFrameStep
    lda.w #$0001
    sta.l !wram_18FD
    ldx.w !actor_current
    jsr.w MenuChoicePressesP2
    ldy.w !menu_joy2_pressed
    tya
    and.w #$4000
    bne .keep_preview
    tya
    and.w #$9000
    bne .leave
    rts
.leave:
    lda.w #$0001
    sta.w !menu_exit
    lda.w #$0000
    sta.l !wram_18FD
    sep.b #$20
    lda.b #$46
    jsl.l SoundPort0QueuePush
    rep.b #$20
    ldy.w !actor_current_slot
    jsr.w MenuActorFree
    rts
.keep_preview:
    lda.w #$0004                    ; not reached in the traces so far
    ldy.w #$FFFF                    ; not reached in the traces so far
    jsr.w MenuActorsSet10           ; not reached in the traces so far
    lda.w #$0005                    ; not reached in the traces so far
    ldy.w #$FFFF                    ; not reached in the traces so far
    jsr.w MenuActorsSet10           ; not reached in the traces so far
    sep.b #$20                      ; not reached in the traces so far
    lda.b #$47                      ; not reached in the traces so far
    jsl.l SoundPort0QueuePush       ; not reached in the traces so far
    rep.b #$20                      ; not reached in the traces so far
    ldx.w !actor_current            ; not reached in the traces so far
    lda.w #$BE29                    ; not reached in the traces so far
    sta.b !actor_handler,x          ; not reached in the traces so far
    lda.w #$0001                    ; not reached in the traces so far
    sta.l !menu_1905_long           ; not reached in the traces so far
    lda.w #$0028                    ; not reached in the traces so far
    sta.l !menu_1907_long           ; not reached in the traces so far
    lda.w #$464A                    ; not reached in the traces so far
    sta.l !palette_word_fd65        ; not reached in the traces so far
    sep.b #$20                      ; not reached in the traces so far
    lda.b #$E0                      ; not reached in the traces so far
    sep.b #$20                      ; not reached in the traces so far
    rep.b #$10                      ; not reached in the traces so far
    phx                             ; not reached in the traces so far
    ldx.w #!HDMAEN                  ; not reached in the traces so far
    jsl.l FrameByteWritePush        ; not reached in the traces so far
    plx                             ; not reached in the traces so far
    rep.b #$20                      ; not reached in the traces so far
    rts                             ; not reached in the traces so far

; @routine 0B:F784 MenuScreenOpenPreview
; WHAT IT DOES: Opens the menu screen with the course preview behind it:
; with the display, HDMA, DMA and interrupts off, clears the menu's lists
; and state, makes the menu objects (MenuActorPoolInit, spawn list $10) and
; starts a race in the background (RaceStart), runs the objects once;
; sets mode 3 with BG1 and BG2 maps at $6000 and $6800 and tiles at 0 and
; $6000, no colour math, BG2 scrolled to $FF00 and BG1 to $0100, BG1, BG2
; and sprites on the main screen with windows on BG1 and sprites; loads
; the palette from $1C:E3AC, saves it and scales colours 144-159 and
; 208-223 to full; draws three pictures from bank $17 ($CC78 at $3000,
; $CE78 at $6800, $DD8C at $4000) and the $DC8C grid, copies $1000 bytes
; of the Super FX picture ($70:3000) to VRAM $6000; loads the menu's
; pictures and blank tiles (MenuLoad*); turns on the IRQ at line $E0 and
; the joypad, waits two frames, runs the objects again, and turns on HDMA
; channels 4 and 7 with every frame update asked for.
; HOW: DB = 0 throughout; P and DB saved and restored.
; Entry: D = $0000. Exit: A = 0; X, Y as MenuOamBuild left them; P and DB
; unchanged.
%at($0BF784)
MenuScreenOpenPreview:
    php
    phb
    sep.b #$20
    rep.b #$10
    lda.b #$00
    pha
    plb
    lda.b #$80
    sta.w !INIDISP
    stz.w !HDMAEN
    stz.w !MDMAEN
    stz.w !wram_11DA
    stz.w !NMITIMEN
    rep.b #$30
    stz.w !byte_writes_size
    stz.w !vram_queue_size
    stz.w !word_list_a_size
    stz.w !word_list_b_size
    lda.w #$0006
    sta.w !gsu_buffer_offset
    lda.w #$1234
    sta.w !menu_1189
    stz.w !menu_118f
    stz.w !menu_exit
    jsl.l MenuActorPoolInit
    jsr.w MenuHdmaSetupB
    ldx.w #$0010
    jsl.l MenuActorSpawnList
    jsl.l RaceStart
    jsl.l MenuActorsRun
    jsl.l MenuOamBuild
    stz.w !menu_slide_pos
    stz.w !menu_scroll_y
    sep.b #$20
    lda.b #$03
    sta.w !BGMODE
    stz.w !CGADSUB
    stz.w !CGWSEL
    lda.b #$61
    sta.w !BG1SC
    lda.b #$69
    sta.w !BG2SC
    lda.b #$30
    sta.w !BG12NBA
    stz.w !BG2HOFS
    lda.b #$FF
    sta.w !BG2HOFS
    sta.w !WH1
    stz.w !BG1HOFS
    lda.b #$01
    sta.w !BG1HOFS
    lda.b #$13
    sta.w !TM
    lda.b #$30
    sta.w !W12SEL
    lda.b #$03
    sta.w !WOBJSEL
    lda.b #$12
    sta.w !TMW
    jsr.w MenuScrollReset
    rep.b #$20
    lda.w #$E3AC
    sta.b !apu_stream_ptr
    lda.w #$001C
    sta.b !apu_stream_bank
    sep.b #$20
    rep.b #$10
    rep.b #$30
    jsr.w MenuPaletteLoad
    lda.w #$0000
    sta.l !palette_buffer
    jsr.w MenuPaletteSave
    lda.w #$0020
    sta.w !scratch_11df
    ldy.w #$0007
    ldx.w #$0120
    jsl.l MenuPaletteScale
    lda.w #$0020
    sta.w !scratch_11df
    ldy.w #$0007
    ldx.w #$01A0
    jsl.l MenuPaletteScale
    rep.b #$30
    rep.b #$30
    ldx.w #$3000
    stx.w !VMADDL
    rep.b #$30
    lda.l !preview_picture_a-2
    xba
    sta.w !menu_picture_size
    lda.w #$CC78
    ldx.w #$0017
    rep.b #$20
    rep.b #$10
    jsr.w MenuPictureToVram
    rep.b #$20
    rep.b #$10
    rep.b #$30
    ldx.w #$6800
    stx.w !VMADDL
    rep.b #$30
    lda.l !preview_picture_b-2
    xba
    sta.w !menu_picture_size
    lda.w #$CE78
    ldx.w #$0017
    rep.b #$20
    rep.b #$10
    jsr.w MenuPictureToVram
    rep.b #$20
    rep.b #$10
    jsr.w MenuPictureDC8CGrid0
    ldx.w #$0000
    lda.w #$6000
    sta.w !VMADDL
.copy_picture:
    lda.l !gsu_picture,x
    sta.w !VMDATAL
    inx
    inx
    cpx.w #$1000
    bne .copy_picture
    rep.b #$30
    ldx.w #$4000
    stx.w !VMADDL
    rep.b #$30
    lda.l !preview_picture_c-2
    xba
    sta.w !menu_picture_size
    lda.w #$DD8C
    ldx.w #$0017
    rep.b #$20
    rep.b #$10
    jsr.w MenuPictureToVram
    rep.b #$20
    rep.b #$10
    rep.b #$30
    ldx.w #$1601
    stx.w !actor_current
    jsr.w MenuLoadPicturesTo7F
    jsr.w MenuLoadClearGsu
    jsr.w MenuLoadBlank0800
    jsr.w MenuLoadBlank1000
    jsr.w MenuLoadBlank1800
    sep.b #$20
    lda.b #$20
    sta.w !wram_0D3F
    lda.b #$E0
    sta.w !VTIMEL
    stz.w !VTIMEH
    lda.b #$0F
    sta.w !frame_update_flags
    sep.b #$20
    lda.b #$00
    sta.l !wram_10DA
    jsl.l SoundReset
    sep.b #$20
    lda.b #$21
    sta.w !NMITIMEN
    jsr.w MenuWaitFrame
    jsr.w MenuWaitFrame
    jsl.l MenuActorsRun
    jsl.l MenuOamBuild
    sep.b #$20
    lda.b #$90
    sta.w !HDMAEN
    lda.w !frame_update_flags
    ora.b #$0F
    sta.w !frame_update_flags
    jsr.w MenuWaitFrame
    plb
    plp
    rts

; @routine 0B:F09F MenuScreenOpenPreviewFade
; WHAT IT DOES: Opens the menu screen with the course preview, fading in:
; clears the menu's lists and state, makes the menu objects (spawn list
; $10), waits a frame, turns on the IRQ at line $E0, copies 24 bytes of the
; palette at $1C:E3AC to $7E:FF65 (clearing the same at $7E:FD65), starts
; a race in the background (RaceStart), sets up the menu HDMA and runs the
; objects; asks (frame write list) for BG1 at $6000, BG2 tiles at 0, colour
; math adding the sub screen with BG1 on it, BG2 scrolled to 1 then 0, and
; sprites, BG1 and BG2 on the main screen; then over 17 frames brings the
; colours from 1/32 to full ($1909 counting $20 down by 2: colours 0-7 to
; $20 - $1909, 12-14 to $1909); then the full mode: no colour math, BG2 at
; $6800 with tiles at 0 and $6000, BG2 scrolled to $FF00, HDMA channels 4
; and 7, BG1, BG2 and sprites on the main screen, windows on BG1 and
; sprites; loads the palette from $1C:E3AC, saves it and scales colours
; 144-159 and 208-223 to full; runs the objects again and asks for every
; frame update over two frames.
; HOW: DB = 0 throughout; the PPU writes go through FrameByteWritePush, so
; they land in the vertical blank. The palette source is read through the
; long pointer at direct page $FB. P and DB saved and restored.
; Entry: D = $0000. Exit: A = 0; X, Y as MenuOamBuild left them; P and DB
; unchanged.
%at($0BF09F)
MenuScreenOpenPreviewFade:
    php
    phb
    sep.b #$20
    rep.b #$10
    lda.b #$00
    pha
    plb
    rep.b #$30
    stz.w !byte_writes_size
    stz.w !vram_queue_size
    stz.w !word_list_a_size
    stz.w !word_list_b_size
    stz.w !menu_exit
    lda.w #$0006
    sta.w !gsu_buffer_offset
    lda.w #$1234
    sta.w !menu_1189
    stz.w !menu_118f
    lda.w #$0000
    sta.w !menu_slide_pos
    lda.w #$0000
    sta.w !menu_scroll_y
    jsl.l MenuActorPoolInit
    ldx.w #$0010
    jsl.l MenuActorSpawnList
    sep.b #$20
    stz.w !frame_update_flags
    jsr.w MenuWaitFrame
    lda.b #$20
    sta.w !wram_0D3F
    lda.b #$E0
    sta.w !VTIMEL
    stz.w !VTIMEH
    rep.b #$30
    rep.b #$20
    lda.w #$E3AC
    sta.b !apu_stream_ptr
    lda.w #$001C
    sta.b !apu_stream_bank
    rep.b #$20
    rep.b #$10
    ldx.w #$0000
    ldy.w #$0000
.copy_palette:
    lda.b [!apu_stream_ptr],y
    sta.l !palette_saved,x
    lda.w #$0000
    sta.l !palette_buffer,x
    iny
    iny
    inx
    inx
    cpx.w #$0018
    bne .copy_palette
    jsl.l RaceStart
    jsr.w MenuHdmaSetupB
    jsl.l MenuActorsRun
    jsl.l MenuOamBuild
    sep.b #$20
    lda.b #$61
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #$2107
    jsl.l FrameByteWritePush
    plx
    lda.b #$70
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #$210B
    jsl.l FrameByteWritePush
    plx
    lda.b #$01
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #$2131
    jsl.l FrameByteWritePush
    plx
    lda.b #$02
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #$2130
    jsl.l FrameByteWritePush
    plx
    lda.b #$02
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #$212D
    jsl.l FrameByteWritePush
    plx
    lda.b #$00
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #$2132
    jsl.l FrameByteWritePush
    plx
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #$212F
    jsl.l FrameByteWritePush
    plx
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #$212E
    jsl.l FrameByteWritePush
    plx
    lda.b #$03
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #$212C
    jsl.l FrameByteWritePush
    plx
    lda.b #$01
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #$210D
    jsl.l FrameByteWritePush
    plx
    lda.b #$00
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #$210D
    jsl.l FrameByteWritePush
    plx
    lda.w !frame_update_flags
    ora.b #$0F
    sta.w !frame_update_flags
    lda.b #$20
    sta.l !preview_fade_step
.fade:
    jsr.w MenuWaitFrame
    lda.b #$20
    sec
    sbc.l !preview_fade_step
    sta.w !scratch_11df
    ldy.w #$0007
    ldx.w #$0000
    jsl.l MenuPaletteScale
    lda.l !preview_fade_step
    sta.w !scratch_11df
    ldy.w #$0003
    ldx.w #$0018
    jsl.l MenuPaletteScale
    lda.w !frame_update_flags
    ora.b #$0B
    sta.w !frame_update_flags
    lda.l !preview_fade_step
    dec a
    dec a
    sta.l !preview_fade_step
    bpl .fade
    jsr.w MenuWaitFrame
    sep.b #$20
    rep.b #$10
    lda.b #$00
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #$2131
    jsl.l FrameByteWritePush
    plx
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #$2130
    jsl.l FrameByteWritePush
    plx
    lda.b #$69
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #$2108
    jsl.l FrameByteWritePush
    plx
    lda.b #$30
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #$210B
    jsl.l FrameByteWritePush
    plx
    lda.b #$FF
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #$210F
    jsl.l FrameByteWritePush
    plx
    lda.b #$00
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #$210F
    jsl.l FrameByteWritePush
    plx
    lda.b #$90
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #$420C
    jsl.l FrameByteWritePush
    plx
    lda.b #$13
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #$212C
    jsl.l FrameByteWritePush
    plx
    lda.b #$FF
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #$2127
    jsl.l FrameByteWritePush
    plx
    lda.b #$30
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #$2123
    jsl.l FrameByteWritePush
    plx
    lda.b #$03
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #$2125
    jsl.l FrameByteWritePush
    plx
    lda.b #$12
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #$212E
    jsl.l FrameByteWritePush
    plx
    rep.b #$20
    lda.w #$E3AC
    sta.b !apu_stream_ptr
    lda.w #$001C
    sta.b !apu_stream_bank
    sep.b #$20
    rep.b #$10
    rep.b #$30
    jsr.w MenuPaletteLoad
    lda.w #$0000
    sta.l !palette_buffer
    jsr.w MenuPaletteSave
    lda.w #$0020
    sta.w !scratch_11df
    ldy.w #$0007
    ldx.w #$0120
    jsl.l MenuPaletteScale
    lda.w #$0020
    sta.w !scratch_11df
    ldy.w #$0007
    ldx.w #$01A0
    jsl.l MenuPaletteScale
    sep.b #$20
    rep.b #$10
    jsl.l MenuActorsRun
    jsl.l MenuOamBuild
    lda.w !frame_update_flags
    ora.b #$0B
    sta.w !frame_update_flags
    jsr.w MenuWaitFrame
    lda.w !frame_update_flags
    ora.b #$0F
    sta.w !frame_update_flags
    jsr.w MenuWaitFrame
    plb
    plp
    rts

; @routine 0B:AFAC MenuPreviewRun
; WHAT IT DOES: Runs the course preview menu: opens it
; (MenuScreenOpenPreview), waits a frame, sets the fade target to full
; (ApproachTargetFull), then runs the menu's loop until it asks to leave.
; HOW: Joins MenuPreviewRunFade's loop.
; Entry: D = $0000. Exit: as MenuRunFrames leaves it.
%at($0BAFAC)
MenuPreviewRun:
    rep.b #$30
    jsr.w MenuScreenOpenPreview
    jsr.w MenuWaitFrame
    jsr.w ApproachTargetFull
    rep.b #$30
    jmp.w MenuPreviewLoop

; @routine 0B:AFBC MenuPreviewRunFade
; WHAT IT DOES: Runs the course preview menu, fading it in
; (MenuScreenOpenPreviewFade) with every frame update on ($11DA = $11DB =
; $0F), then the menu's loop until it asks to leave ($11EB): each frame
; the objects run, presses are cleared and the blink counter steps; at
; the end sound $04 on port 1, the fade target to 0 and the menu frames
; run out.
; HOW: The loop is shared with MenuPreviewRun.
; Entry: D = $0000. Exit: as MenuRunFrames leaves it.
%at($0BAFBC)
MenuPreviewRunFade:
    rep.b #$30
    jsr.w MenuScreenOpenPreviewFade
    sep.b #$20
    lda.b #$0F
    sta.w !wram_11DA
    sta.w !wram_11DB
    rep.b #$30
MenuPreviewLoop:                    ; MenuPreviewRun joins here
    jsr.w MenuWaitFrame
    lda.w #$0000
    sta.l !wram_18FD
    inc.w !menu_blink_counter
    jsl.l MenuActorsRun
    jsl.l MenuOamBuild
    rep.b #$20
    stz.w !menu_joy1_pressed
    stz.w !menu_joy2_pressed
    sep.b #$20
    lda.w !frame_update_flags
    ora.b #$0F
    sta.w !frame_update_flags
    rep.b #$30
    lda.w !menu_exit
    beq MenuPreviewLoop
    sep.b #$20
    lda.b #$04
    jsl.l SoundPort1QueuePush
    rep.b #$20
    jsr.w ApproachTargetZero
    jsr.w MenuRunFrames
    rtl

; @routine 0B:C5EF MenuSlideToPreview
; WHAT IT DOES: Steps menu record X's slide +$0A by 3 up to $80 and places
; it (MenuSinePosition); when it reaches $80 it sets the record's handler
; +$08 to $BA4E and restarts the slide, gives kind 4's actor (MenuActorFind)
; animation $8A (+$14; its position +$16 and timer +$18 restarted when it
; had another), clears $18D3, sets up selection $18CB's pair (MenuSet1A66,
; MenuSetPairAnimations), shows a race frame (RaceFrameStep with A = 0),
; sets $18FD to 1 and clears $1191.
; HOW: Words through direct page + X, so each step uses the X the call
; before left. The animation change is not reached in the traces so far.
; Entry: M 16-bit, X 16-bit, D = $0000; X = record. Exit: A = 0, Z set
; (or, before $80, A = the slide, N, Z, C from comparing it with $80); X
; and Y as the last call left them.
%at($0BC5EF)
MenuSlideToPreview:
    lda.b !actor_0a,x
    inc a
    inc a
    inc a
    cmp.w #$0080
    bcc .store
    lda.w #$0080
.store:
    sta.b !actor_0a,x
    jsr.w MenuSinePosition
    lda.b !actor_0a,x
    cmp.w #$0080
    bne .done
    lda.w #$BA4E
    sta.b !actor_handler,x
    stz.b !actor_0a,x
    lda.w #$0004
    jsr.w MenuActorFind
    lda.b !actor_anim,x
    cmp.w #$008A
    beq .animated
    lda.w #$008A
    sta.b !actor_anim,x
    stz.b !actor_anim_pos,x
    stz.b !actor_anim_timer,x
.animated:
    lda.w #$0000
    sta.l !selection_out
    lda.l !selection
    jsr.w MenuSet1A66
    jsr.w MenuSetPairAnimations
    lda.w #$0000
    jsl.l RaceFrameStep
    lda.w #$0001
    sta.l !wram_18FD
    lda.w #$0000
    sta.l !word_list_a_size
.done:
    rts

; @routine 03:B3DA CourseMenuRun
; WHAT IT DOES: The course menu before a race: clears side $1F07, the
; selection $18C3 and $1168; the first time ($1917 clear) 0B:AEF2's screen
; first, then the course preview, faded in (MenuPreviewRunFade) when that
; screen returned 0, else MenuPreviewRun; then the HUD mode $0D2B = the
; selection, $1917 = 1, the selection kept (SelectionCommit for choice 2,
; else SelectionSave), the three settings $18EF, $18F1, $18F3 at $1184-$1186
; and in the save at $70:FC08-$70:FC0A (SaveChecksum); $0D3F = $1C, the NMI
; and HDMA off and the screen blanked.
; HOW: Through the data bank, long accesses to $18C3-$18F3 and $1917; P
; saved and put back.
; Entry: any widths. Exit: P unchanged; A low byte $80.
%at($03B3DA)
CourseMenuRun:
    php
    sep.b #$20
    lda.b #$00
    sta.w !draw_side
    rep.b #$20
    stz.w !wram_18C3
    stz.w !packed_time_a+5
    lda.l !wram_1913+4
    bne .lB3FB
    jsl.l TitleScreenRun
    rep.b #$20
    cmp.w #$0000
    beq .lB402
.lB3FB:
    jsl.l MenuPreviewRun
    jmp.w .lB406
.lB402:
    jsl.l MenuPreviewRunFade
.lB406:
    rep.b #$20
    lda.l !wram_18C3
    sta.w !hud_mode
    lda.w #$0001
    sta.l !wram_1913+4
    lda.l !wram_18C3
    cmp.w #$0002
    beq .lB425
    jsl.l SelectionSave
    bra .lB429
.lB425:
    jsl.l SelectionCommit
.lB429:
    sep.b #$20
    lda.l !wram_18EF
    sta.w !wram_1184
    sta.l !save_bytes+2
    lda.l !wram_18F1
    sta.w !wram_1184+1
    sta.l !save_bytes+3
    lda.l !wram_18F3
    sta.w !wram_1184+2
    sta.l !save_bytes+4
    jsl.l SaveChecksum
    sep.b #$20
    lda.b #$1C
    sta.w !wram_0D3F
    stz.w !NMITIMEN
    stz.w !HDMAEN
    lda.b #$80
    sta.w !INIDISP
    plp
    rtl


; @routine 0B:BE29 MenuPreviewBackOut
; WHAT IT DOES: The course preview after player 2 backs out with Y
; (MenuPreviewCountdown's $4000 case): two fixed colour steps down, a race
; frame shown, $18FD = 1; once $1905 is clear, makes $BE65
; (MenuPreviewBackWait) the object's handler, clears the selection out
; ($18D3), sets $1A66 and the pair's animations from the selection ($18CB)
; and shows another race frame.
; HOW: Calls in turn; the object is the current one ($11F9). No play
; reaches it; unit_preview_back_0b calls it.
; Entry: M 16-bit, X 16-bit, D = $0000. Exit: as the last call leaves it.
%at($0BBE29)
MenuPreviewBackOut:
    jsr.w MenuFixedColourDown
    jsr.w MenuFixedColourDown
    lda.w #$0000
    jsl.l RaceFrameStep
    lda.w #$0001
    sta.l !wram_18FD
    lda.l !menu_1905
    beq .lBE44
    rts
.lBE44:
    ldx.w !actor_current
    lda.w #$BE65
    sta.b !dp_08,x
    lda.w #$0000
    sta.l !selection_out
    lda.l !selection
    jsr.w MenuSet1A66
    jsr.w MenuSetPairAnimations
    lda.w #$0000
    jsl.l RaceFrameStep
    rts

; @routine 0B:BE65 MenuPreviewBackWait
; WHAT IT DOES: Waits for the byte $0357; once it is set, $1905 = 2, $1907
; = 0 and the object's handler becomes $BE84 (MenuPreviewBackEnd).
; HOW: An 8-bit test of $0357, then 16-bit long stores.
; Entry: M 16-bit, X 16-bit, D = $0000; X = the object. Exit: M 16-bit;
; waiting: A low byte = 0, Z set; else A = $BE84, N set.
%at($0BBE65)
MenuPreviewBackWait:
    sep.b #$20
    lda.w !wram_0357
    beq .lBE81
    rep.b #$20
    lda.w #$0002
    sta.l !menu_1905
    lda.w #$0000
    sta.l !menu_1907
    lda.w #$BE84
    sta.b !dp_08,x
.lBE81:
    rep.b #$20
    rts

; @routine 0B:BE84 MenuPreviewBackEnd
; WHAT IT DOES: The last frame of backing out of the preview: two fixed
; colour steps down, a race frame shown, $18FD = 1; once $1905 is clear,
; the object's handler becomes $BA4E with a count of 20 (+$0A), the kind 4
; object gets animation $CE (unless it has it), +$10 of groups 4 and 5 is
; cleared (MenuActorsSet10 with Y = 0), the word list's size $1191 = 0 and
; HDMA channels 6 and 7 are asked for ($C0 to HDMAEN on the frame write
; list).
; HOW: As MenuPreviewStart.
; Entry: M 16-bit, X 16-bit, D = $0000. Exit: as FrameByteWritePush
; leaves it, with X the kind 4 object and M 16-bit.
%at($0BBE84)
MenuPreviewBackEnd:
    jsr.w MenuFixedColourDown
    jsr.w MenuFixedColourDown
    lda.w #$0000
    jsl.l RaceFrameStep
    lda.w #$0001
    sta.l !wram_18FD
    lda.l !menu_1905
    beq .lBE9F
    rts
.lBE9F:
    ldx.w !actor_current
    lda.w #$BA4E
    sta.b !dp_08,x
    lda.w #$0014
    sta.b !actor_0a,x
    lda.w #$0004
    jsr.w MenuActorFind
    lda.b !actor_anim,x
    cmp.w #$00CE
    beq .lBEC2
    lda.w #$00CE
    sta.b !actor_anim,x
    stz.b !actor_anim_pos,x
    stz.b !actor_anim_timer,x
.lBEC2:
    lda.w #$0004
    ldy.w #$0000
    jsr.w MenuActorsSet10
    lda.w #$0005
    ldy.w #$0000
    jsr.w MenuActorsSet10
    lda.w #$0000
    sta.l !word_list_a_size
    sep.b #$20
    lda.b #$C0
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #!HDMAEN
    jsl.l FrameByteWritePush
    plx
    rep.b #$20
    rts
