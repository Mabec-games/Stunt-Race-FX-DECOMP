; sound_effects.asm: sound commands the game sends for each player's car.
;
; Most effects have one command for player 1 and another for player 2; the
; routines here pick by the player whose car is being processed. A car can
; also hold one looping sound, remembered per player so it is only re-sent
; when it changes. C: src/stuntracefx/audio/sound_effects.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"

arch 65816

; @routine 04:D463 PlayerHoldSound32
; WHAT IT DOES: Makes the current player's car hold sound $32 (player 1) or
; $36 (player 2).
; HOW: Compares the sound with the player's held sound; when they differ,
; HeldSoundSetPlayer1/2 starts it.
; Entry: M any, DB a bank where $1A5D is work RAM. Exit: A low byte = the
; sound chosen; P unchanged.
%at($04D463)
PlayerHoldSound32:
    php
    sep.b #$20
    lda.w !current_player
    bne .player2
    lda.b #$32
    cmp.l !player1_held_sound
    bne HeldSoundSetPlayer1
    plp
    rtl
.player2:
    lda.b #$36
    cmp.l !player2_held_sound
    bne HeldSoundSetPlayer2
    plp
    rtl

; @routine 04:D47F PlayerHoldSound33
; WHAT IT DOES: Makes the current player's car hold sound $33 (player 1) or
; $37 (player 2).
; HOW: Compares the sound with the player's held sound; when they differ,
; HeldSoundSetPlayer1/2 starts it.
; Entry: M any, DB a bank where $1A5D is work RAM. Exit: A low byte = the
; sound chosen; P unchanged.
%at($04D47F)
PlayerHoldSound33:
    php
    sep.b #$20
    lda.w !current_player
    bne .player2
    lda.b #$33
    cmp.l !player1_held_sound
    bne HeldSoundSetPlayer1
    plp
    rtl
.player2:
    lda.b #$37
    cmp.l !player2_held_sound
    bne HeldSoundSetPlayer2
    plp
    rtl

; @routine 04:D49B PlayerStopHeldSound
; WHAT IT DOES: Stops the current player's held sound, if any.
; HOW: If the player holds a sound, clears it and sends $0A (player 1) or $1A
; (player 2) on port 1. The two held-sound starts shared by PlayerHoldSound*
; follow it.
; Entry: M any, DB a bank where $1A5D is work RAM. Exit: A low byte = the stop
; command, or 0 when nothing was held; P unchanged.
%at($04D49B)
PlayerStopHeldSound:
    php
    sep.b #$20
    lda.w !current_player
    bne .player2
    lda.l !player1_held_sound
    beq HeldSoundSetPlayer1_done
    lda.b #$00
    sta.l !player1_held_sound
    lda.b #$0A
    jsl.l SoundPort1QueuePush
    plp
    rtl
.player2:
    lda.l !player2_held_sound
    beq HeldSoundSetPlayer1_done
    lda.b #$00
    sta.l !player2_held_sound
    lda.b #$1A
    jsl.l SoundPort1QueuePush
    plp
    rtl

; Start held sound A for player 1: announce with $09 if nothing was held,
; record it, send it. Entered by branch from PlayerHoldSound*, with P saved.
HeldSoundSetPlayer1:
    pha
    lda.l !player1_held_sound
    bne .announced
    lda.b #$09
    jsl.l SoundPort1QueuePush
.announced:
    pla
    sta.l !player1_held_sound
    jsl.l SoundPort1QueuePush
HeldSoundSetPlayer1_done:
    plp
    rtl

; The same for player 2, announcing with $14.
HeldSoundSetPlayer2:
    pha
    lda.l !player2_held_sound
    bne .announced
    lda.b #$14
    jsl.l SoundPort1QueuePush
.announced:
    pla
    sta.l !player2_held_sound
    jsl.l SoundPort1QueuePush
    plp
    rtl

; @routine 04:D4FB PlayerHoldSound34
; WHAT IT DOES: Makes the current player's car hold sound $34 (player 1) or
; $38 (player 2).
; HOW: Compares the sound with the player's held sound; when they differ,
; HeldSoundSetPlayer1/2 starts it.
; Entry: M any, DB a bank where $1A5D is work RAM. Exit: A low byte = the
; sound chosen; P unchanged.
%at($04D4FB)
PlayerHoldSound34:
    php
    sep.b #$20
    lda.w !current_player
    bne .player2
    lda.b #$34
    cmp.l !player1_held_sound
    bne HeldSoundSetPlayer1
    plp
    rtl
.player2:
    lda.b #$38
    cmp.l !player2_held_sound
    bne HeldSoundSetPlayer2
    plp
    rtl

; @routine 04:D517 PlayerHoldSound35
; WHAT IT DOES: Makes the current player's car hold sound $35 (player 1) or
; $39 (player 2).
; HOW: Compares the sound with the player's held sound; when they differ,
; HeldSoundSetPlayer1/2 starts it.
; Entry: M any, DB a bank where $1A5D is work RAM. Exit: A low byte = the
; sound chosen; P unchanged.
%at($04D517)
PlayerHoldSound35:
    php
    sep.b #$20
    lda.w !current_player
    bne .player2
    lda.b #$35
    cmp.l !player1_held_sound
    bne HeldSoundSetPlayer1
    plp
    rtl
.player2:
    lda.b #$39
    cmp.l !player2_held_sound
    bne HeldSoundSetPlayer2
    plp
    rtl

; @routine 04:D5A2 Sound0E1AStop
; WHAT IT DOES: Ends the sound marked by $0E1A: clears the mark and sends $08
; on port 1, if the mark was set.
; HOW: Tests $0E1A; when nonzero, stores 0 there and queues $08 for port 1.
; Entry: M 8-bit, DB a bank where $0E1A is work RAM. Exit: A low byte = $08,
; or 0 when the mark was clear; P unchanged.
%at($04D5A2)
Sound0E1AStop:
    php
    lda.w !wram_0E1A
    beq .done
    stz.w !wram_0E1A
    lda.b #$08
    jsl.l SoundPort1QueuePush
.done:
    plp
    rtl

; @routine 04:D5B3 PlayerSound23
; WHAT IT DOES: Sends sound $23 (player 1) or $25 (player 2) on port 1 for
; the player whose car is being processed.
; HOW: Picks the command by $1A5D and queues it for port 1.
; Entry: any widths. Exit: A low byte = the command sent; P unchanged.
%at($04D5B3)
PlayerSound23:
    php
    sep.b #$20
    lda.l !current_player
    bne .player2
    lda.b #$23
    jsl.l SoundPort1QueuePush
    plp
    rtl
.player2:
    lda.b #$25
    jsl.l SoundPort1QueuePush
    plp
    rtl

; @routine 04:D5CC PlayerSound24
; WHAT IT DOES: Sends sound $24 (player 1) or $26 (player 2) on port 1 for
; the player whose car is being processed.
; HOW: Picks the command by $1A5D and queues it for port 1.
; Entry: any widths. Exit: A low byte = the command sent; P unchanged.
%at($04D5CC)
PlayerSound24:
    php
    sep.b #$20
    lda.l !current_player
    bne .player2
    lda.b #$24
    jsl.l SoundPort1QueuePush
    plp
    rtl
.player2:
    lda.b #$26
    jsl.l SoundPort1QueuePush
    plp
    rtl

; @routine 04:D5E5 PlayerSound0D
; WHAT IT DOES: Sends sound $0D (player 1) or $18 (player 2) on port 1 for
; the player whose car is being processed.
; HOW: Picks the command by $1A5D and queues it for port 1.
; Entry: any widths. Exit: A low byte = the command sent; P unchanged.
%at($04D5E5)
PlayerSound0D:
    php
    sep.b #$20
    lda.l !current_player
    bne .player2
    lda.b #$0D
    jsl.l SoundPort1QueuePush
    plp
    rtl
.player2:
    lda.b #$18
    jsl.l SoundPort1QueuePush
    plp
    rtl

; @routine 04:D5FE PlayerSound0B
; WHAT IT DOES: Sends sound $0B (player 1) or $1B (player 2) on port 1 for
; the player whose car is being processed.
; HOW: Picks the command by $1A5D and queues it for port 1.
; Entry: any widths. Exit: A low byte = the command sent; P unchanged.
%at($04D5FE)
PlayerSound0B:
    php
    sep.b #$20
    lda.l !current_player
    bne .player2
    lda.b #$0B
    jsl.l SoundPort1QueuePush
    plp
    rtl
.player2:
    lda.b #$1B
    jsl.l SoundPort1QueuePush
    plp
    rtl

; @routine 04:D617 PlayerSound12
; WHAT IT DOES: Sends sound $12 (player 1) or $19 (player 2) on port 1 for
; the player whose car is being processed.
; HOW: Picks the command by $1A5D and queues it for port 1.
; Entry: any widths. Exit: A low byte = the command sent; P unchanged.
%at($04D617)
PlayerSound12:
    php
    sep.b #$20
    lda.l !current_player
    bne .player2
    lda.b #$12
    jsl.l SoundPort1QueuePush
    plp
    rtl
.player2:
    lda.b #$19
    jsl.l SoundPort1QueuePush
    plp
    rtl

; @routine 04:D630 PlayerSound0C
; WHAT IT DOES: Sends sound $0C (player 1) or $1C (player 2) on port 1 for
; the player whose car is being processed.
; HOW: Picks the command by $1A5D and queues it for port 1.
; Entry: any widths. Exit: A low byte = the command sent; P unchanged.
%at($04D630)
PlayerSound0C:
    php
    sep.b #$20
    lda.l !current_player
    bne .player2
    lda.b #$0C
    jsl.l SoundPort1QueuePush
    plp
    rtl
.player2:
    lda.b #$1C
    jsl.l SoundPort1QueuePush
    plp
    rtl
