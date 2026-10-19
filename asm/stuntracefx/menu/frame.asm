; frame.asm: the menu's frame handshake. The menu code sets bit 0 of $11D8
; when it has finished a frame; the menu's vertical blank code clears $11D8
; once it has applied the frame's updates.
; C: src/stuntracefx/menu/frame.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 0B:E2D7 MenuFramesTarget7
; WHAT IT DOES: Sets the frame update target $11DB (MenuRunFrames runs
; frames until $11DA reaches it) to 7. Nothing calls it, so a direct call
; is its only way in.
; HOW: An 8-bit store through the data bank between PHP and PLP.
; Entry: any widths. Exit: A low byte = 7; P unchanged.
%at($0BE2D7)
MenuFramesTarget7:
    php
    sep.b #$20
    lda.b #$07
    sta.w !wram_11DB
    plp
    rts

; @routine 0B:E3A3 MenuRandomStep
; WHAT IT DOES: Steps the menu's random word $1189 (set to $1234 when a
; screen opens): the word with its bytes swapped and halved, plus the word
; twice, XORed with the blink counter $11DD. Nothing calls it, so a direct
; call is its only way in.
; HOW: The two bytes read in 8-bit with an XBA between; then 16-bit LSR,
; CLC and two ADCs (the second keeps the first's carry), EOR; through the
; data bank, between PHP and PLP.
; Entry: any widths. Exit: A = the new word; P unchanged.
%at($0BE3A3)
MenuRandomStep:
    php
    sep.b #$20
    lda.w !menu_random
    xba
    lda.w !menu_random+1
    rep.b #$20
    lsr a
    clc
    adc.w !menu_random
    adc.w !menu_random
    eor.w !menu_blink_counter
    sta.w !menu_random
    plp
    rts

; @routine 0B:E3BF MenuDimOnL
; WHAT IT DOES: When the press being worked on ($118F) has the L button
; (bit 5), sets the screen brightness to 4 (INIDISP). Nothing calls it, so
; a direct call is its only way in.
; HOW: AND #$0020; the brightness written in 8-bit with a long store.
; Entry: M 16-bit. Exit: no L: A = 0; Z set, N clear. L: A = 4; N, Z
; clear; M 16-bit.
%at($0BE3BF)
MenuDimOnL:
    lda.w !menu_press
    and.w #$0020
    beq .done
    sep.b #$20
    lda.b #$04
    sta.l !INIDISP
    rep.b #$20
.done:
    rts

; @routine 0B:E390 MenuWaitFrame
; WHAT IT DOES: Marks the frame finished and waits for the vertical blank
; code to take it.
; HOW: Sets bit 0 of $11D8 (16-bit, through the data bank) and polls it
; until it is zero.
; Entry: any widths. Exit: A = 0; P unchanged.
%at($0BE390)
MenuWaitFrame:
    php
    rep.b #$20
    lda.w !menu_frame_wait
    ora.w #$0001
    sta.w !menu_frame_wait
.lE39C:
    lda.w !menu_frame_wait
    bne .lE39C
    plp
    rts

; @routine 0B:E2E1 MenuRunFrames
; WHAT IT DOES: Runs menu frames until $11DA equals $11DB: each frame waits
; for the vertical blank (MenuWaitFrame), runs the objects
; (MenuActorsRun), builds the sprite table (MenuOamBuild) and asks for
; every frame update (bits 0-4 of $11D7).
; HOW: 8-bit A for the flags and the compare; P saved and restored.
; Entry: any widths. Exit: A low byte = $11DA, high byte as MenuOamBuild
; left it; X, Y as MenuOamBuild left them; P unchanged.
%at($0BE2E1)
MenuRunFrames:
    php
    sep.b #$20
.lE2E4:
    jsr.w MenuWaitFrame
    jsl.l MenuActorsRun
    jsl.l MenuOamBuild
    sep.b #$20
    lda.w !frame_update_flags
    ora.b #$1F
    sta.w !frame_update_flags
    lda.w !wram_11DA
    cmp.w !wram_11DB
    bne .lE2E4
    plp
    rts

; @routine 0B:AE47 MenuRegionCheck
; WHAT IT DOES: Stops the game on a PAL console (bit 4 of STAT78): starts
; animation $1E on the kind $0F object and puts it at Y $60, clears +$10
; of the kind $10 object, runs the objects once, builds the sprite table,
; asks for frame updates 0-3 and then loops for ever. On NTSC it returns
; at once; no script runs on PAL.
; HOW: DB = 0 and 8-bit A throughout, so the searches compare and the
; field writes store single bytes. The loop is a branch to itself without
; waiting for a frame, so the frame updates are never taken.
; Entry: X 16-bit. Exit: (NTSC) A low byte = 0; X, Y unchanged; P and DB
; unchanged.
%at($0BAE47)
MenuRegionCheck:
    php
    phb
    sep.b #$20
    rep.b #$10
    lda.b #$00
    pha
    plb
    lda.w !STAT78
    and.b #$10
    beq MenuRegionCheck_ntsc

; @routine 0B:AE58 MenuRegionPal
; WHAT IT DOES: MenuRegionCheck's stop on a PAL console, which no NTSC
; run reaches: the region screen's objects set (animation $1E on the
; kind $0F object at Y $60, +$10 of the kind $10 object cleared), one
; frame's objects and sprites, then a loop for ever.
; HOW: As inside MenuRegionCheck.
; Entry: M 8-bit, X 16-bit, D = $0000; DB = 0; P and DB on the stack.
; Exit: does not return.
MenuRegionPal:
    lda.b #$0F
    jsr.w MenuActorFind
    lda.b !actor_anim,x
    cmp.b #$1E
    beq .lAE6B
    lda.b #$1E
    sta.b !actor_anim,x
    stz.b !actor_anim_pos,x
    stz.b !actor_anim_timer,x
.lAE6B:
    lda.b #$60
    sta.b !actor_06,x
    lda.b #$10
    jsr.w MenuActorFind
    stz.b !actor_10,x
    jsl.l MenuActorsRun
    jsl.l MenuOamBuild
    sep.b #$20
    lda.w !frame_update_flags
    ora.b #$0F
    sta.w !frame_update_flags
    rep.b #$30
.lAE8A:
    bra .lAE8A
MenuRegionCheck_ntsc:
    plb
    plp
    rts

; @routine 0B:AE8F MenuHeldRestart
; WHAT IT DOES: Returns 0 when no player holds a button in bits 0-3 and
; $18FB is clear. Otherwise starts animation $18 on the kind $0F object and
; runs menu frames until that is so, then returns $FFFF (the caller opens
; its screen again). No script holds those buttons here.
; HOW: The held words are read through the data bank; the frames are as
; MenuRunFrames's, with frame updates 0-3.
; Entry: X 16-bit, D = $0000. Exit: A = 0 or $FFFF; N, Z from it; M
; 16-bit, X 16-bit; X, Y unchanged when 0, as the last frame left them
; when $FFFF, C then from the compare of the animation with $18.
%at($0BAE8F)
MenuHeldRestart:
    rep.b #$30
    lda.w !joy1_held
    and.w #$000F
    bne .lAEAB
    lda.w !joy2_held
    and.w #$000F
    bne .lAEAB
    lda.l !wram_18FB
    bne .lAEAB
    lda.w #$0000
    rts
.lAEAB:
    lda.w #$000F
    jsr.w MenuActorFind
    lda.b !actor_anim,x
    cmp.w #$0018
    beq .lAEC1
    lda.w #$0018
    sta.b !actor_anim,x
    stz.b !actor_anim_pos,x
    stz.b !actor_anim_timer,x
.lAEC1:
    jsr.w MenuWaitFrame
    jsl.l MenuActorsRun
    jsl.l MenuOamBuild
    sep.b #$20
    lda.w !frame_update_flags
    ora.b #$0F
    sta.w !frame_update_flags
    rep.b #$30
    lda.w !joy1_held
    and.w #$000F
    bne .lAEC1
    lda.w !joy2_held
    and.w #$000F
    bne .lAEC1
    lda.l !wram_18FB
    bne .lAEC1
    lda.w #$FFFF
    rts

; @routine 0B:AE0A MenuScreen0Run
; WHAT IT DOES: Runs the menu screen of spawn list 0: opens it
; (MenuScreenOpen0), runs frames until the menu asks to exit ($11EB), checks
; the region (MenuRegionCheck), opens it again while buttons are held
; (MenuHeldRestart), then turns the display, interrupts, HDMA and DMA off.
; HOW: Frames as MenuRunFrames's, with frame updates 0-3.
; Entry: D = $0000. Exit: A = $0080; X, Y as the last frame left them; M
; 8-bit, X 16-bit; N set, Z clear; C clear.
%at($0BAE0A)
MenuScreen0Run:
    rep.b #$30
    jsr.w MenuScreenOpen0
.lAE0F:
    jsr.w MenuWaitFrame
    jsl.l MenuActorsRun
    jsl.l MenuOamBuild
    sep.b #$20
    lda.w !frame_update_flags
    ora.b #$0F
    sta.w !frame_update_flags
    rep.b #$30
    lda.w !menu_exit
    beq .lAE0F
    jsr.w MenuRegionCheck
    jsr.w MenuHeldRestart
    cmp.w #$FFFF
    beq MenuScreen0Run
    sep.b #$20
    lda.b #$80
    sta.w !INIDISP
    stz.w !NMITIMEN
    stz.w !HDMAEN
    stz.w !MDMAEN
    rtl
