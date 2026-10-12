; sound_effects.asm: sound commands the game sends for each player's car.
;
; Most effects have one command for player 1 and another for player 2; the
; routines here pick by the player whose car is being processed. A car can
; also hold one looping sound, remembered per player so it is only re-sent
; when it changes. C: src/stuntracefx/audio/sound_effects.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

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

; @routine 03:EDDF SoundFromTableD89
; WHAT IT DOES: Queues, exactly as given, the APU port 0 command that the
; table at $03:EDEF holds for the value in $0D89.
; HOW: Byte lookup indexed by $0D89, then SoundPort0QueuePushRaw.
; Entry: any widths. Exit: A low byte = the command; X = $0D89 (high byte
; zero); P unchanged.
%at($03EDDF)
SoundFromTableD89:
    php
    sep.b #$30
    ldx.w !wram_0D89
    lda.l !sound_table_d89,x
    jsl.l SoundPort0QueuePushRaw
    plp
    rtl

; @routine 0B:FAF9 SoundPairFromTable
; WHAT IT DOES: Queues two APU port 0 commands from the table at $0B:FB18:
; the one for $0D8A with bit 7 set (player 2's), then the one for $0D89.
; HOW: Two byte lookups, each pushed with SoundPort0QueuePushRaw.
; Entry: any widths. Exit: A low byte = the second command; X = $0D89 (high
; byte zero); P unchanged.
%at($0BFAF9)
SoundPairFromTable:
    php
    sep.b #$30
    ldx.w !wram_0D8A
    lda.l !sound_pair_table,x
    ora.b #$80
    jsl.l SoundPort0QueuePushRaw
    sep.b #$30
    ldx.w !wram_0D89
    lda.l !sound_pair_table,x
    jsl.l SoundPort0QueuePushRaw
    plp
    rtl

; @routine 08:D580 SoundAtRateForLevel
; WHAT IT DOES: Queues APU port 0 command $23 at a rate set by the level A:
; none below $28, every fourth frame (bits 0-1 of the counter at $05E9
; clear) up to $9F, every other frame from $A0.
; HOW: Signed compares with $28 and $A0; the counter is read through the
; data bank; SoundPort0QueuePushRaw.
; Entry: M 16-bit; A = level. Exit: below $28: A unchanged, N, Z, C from
; the compare. Skipped: A = the counter bits; N, Z from them. Queued: A =
; $23; N, Z from the data bank. C from the compare with $A0 in both.
%at($08D580)
SoundAtRateForLevel:
    cmp.w #$0028
    bmi .lD5AA
    cmp.w #$00A0
    bpl .lD59B
    lda.w !wram_05E9
    and.w #$0003
    bne .lD5AA
    lda.w #$0023
    jsl.l SoundPort0QueuePushRaw
    bra .lD5AA
.lD59B:
    lda.w !wram_05E9
    and.w #$0001
    bne .lD5AA
    lda.w #$0023
    jsl.l SoundPort0QueuePushRaw
.lD5AA:
    rts

; @routine 04:D0BE SoundIfFlag40
; WHAT IT DOES: When bit 6 of A is set, queues for the current player the
; APU port 0 command the table at $04:D0D6 holds for the byte at $0D89 + Y.
; HOW: An immediate BIT test; byte lookups; SoundPort0QueuePush. A is saved
; and restored.
; Entry: M 16-bit, X 8-bit; A = flags, Y = offset. Exit: A unchanged; X =
; the $0D89 byte when it queued; M 16-bit, X 8-bit; N, Z from A.
%at($04D0BE)
SoundIfFlag40:
    pha
    bit.w #$0040
    beq .lD0D2
    sep.b #$30
    lda.w !wram_0D89,y
    tax
    lda.l !sound_table_d0d6,x
    jsl.l SoundPort0QueuePush
.lD0D2:
    rep.b #$20
    pla
    rts

; @routine 04:D082 SoundHoldByFlags
; WHAT IT DOES: Starts or stops a held sound for player Y (0 or 1): when bit
; 15 or 14 of A is set and the player's byte at $0DD8 + Y is 0, sets it to
; $FF and queues port 1 command $3A (player 1) or $3C (player 2); when both
; bits are clear and the byte is set, clears it and queues $3B or $3D.
; HOW: Immediate BIT tests; SoundPort1QueuePush.
; Entry: M 16-bit, X 8-bit; A = flags, Y = player. Exit: M 8-bit. Nothing
; to do: A low byte = the byte; N, Z from it. Queued: A low byte = the
; command; C set; N, Z from the data bank.
%at($04D082)
SoundHoldByFlags:
    bit.w #$8000
    bne .lD08C
    bit.w #$4000
    beq .lD0A3
.lD08C:
    sep.b #$20
    lda.w !sound_hold,y
    bne .lD0BD
    dec a
    sta.w !sound_hold,y
    cpy.b #$00
    bne .lD09F
    lda.b #$3A
    bra .lD0B9
.lD09F:
    lda.b #$3C
    bra .lD0B9
.lD0A3:
    sep.b #$20
    lda.w !sound_hold,y
    beq .lD0BD
    lda.b #$00
    sta.w !sound_hold,y
    cpy.b #$00
    bne .lD0B7
    lda.b #$3B
    bra .lD0B9
.lD0B7:
    lda.b #$3D
.lD0B9:
    jsl.l SoundPort1QueuePush
.lD0BD:
    rts
