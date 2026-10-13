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
