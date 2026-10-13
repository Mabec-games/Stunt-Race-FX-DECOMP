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

; @routine 04:D533 SoundNearCarOnce
; WHAT IT DOES: The first time it runs (mark $0E1A clear), marks it, sends $1D
; on port 1 and, unless $0D53 is set, a sound for the car nearest the player
; when that car's record word $88 is not below the player's: $1D, $1F or $1E
; on port 0 for car scripts $876E, $8AA8 and $8990.
; HOW: DEC marks $0E1A ($FF). The car records are in Super FX RAM at $70:0000
; + the offsets in $70:2476 (player) and $70:2648 (nearest car); the player's
; word is kept in $11DF for the compare. The $876E sound and the no-match path
; are not reached in the traces so far.
; Entry: DB a bank where $0E1A is work RAM. Exit: X and Y high bytes cleared
; (SEP #$30 before the PHX); A as the path left it; P restored.
%at($04D533)
SoundNearCarOnce:
    php
    sep.b #$30
    lda.w !wram_0E1A
    bne .done
    dec.w !wram_0E1A
    lda.b #$1D
    jsl.l SoundPort1QueuePush
    sep.b #$20
    lda.l !wram_0D53
    bne .done
    rep.b #$30
    phx
    lda.l !fx_player
    tax
    lda.l !gsu_rec_88,x
    sta.l !scratch_11df
    lda.l !fx_near_car
    tax
    lda.l !gsu_rec_88,x
    cmp.l !scratch_11df
    bcc .behind
    lda.l !gsu_rec_66,x
    plx
    cmp.w #$876E
    beq .sound1D
    cmp.w #$8AA8
    beq .sound1F
    cmp.w #$8990
    beq .sound1E
    bra .done
.sound1D:
    sep.b #$20
    lda.b #$1D
    jsl.l SoundPort0QueuePush
    bra .done
.sound1F:
    sep.b #$20
    lda.b #$1F
    jsl.l SoundPort0QueuePush
    bra .done
.sound1E:
    sep.b #$20
    lda.b #$1E
    jsl.l SoundPort0QueuePush
.done:
    plp
    rtl
.behind:
    plx
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

; @routine 07:D6D1 PlayerSound35
; WHAT IT DOES: Queues sound command $35 on port 0 for the current player.
; HOW: Tests whether the byte at $0DF4 + Y is 1, but both branches make
; the same SoundPort0QueuePush call.
; Entry: M 16-bit, X 16-bit; Y = player index (bytes). Exit: A low byte =
; $35; M 8-bit; C set when the byte at $0DF4 + Y is 1 or more; N, Z from
; the data bank.
%at($07D6D1)
PlayerSound35:
    sep.b #$20
    lda.w !player_mode,y
    cmp.b #$01
    beq .one
    lda.b #$35
    jsl.l SoundPort0QueuePush
    rts
.one:
    lda.b #$35
    jsl.l SoundPort0QueuePush

; @routine 04:D040 EngineSoundsPlayer2
; WHAT IT DOES: Runs player 2's engine sounds: when $0E4B is 0 and $0DD7 is
; 6, SoundIfFlag40 with the buttons newly pressed ($0D1C) and
; SoundHoldByFlags with the buttons held ($030D).
; HOW: 8-bit tests through the data bank; Y is the caller's.
; Entry: any widths; Y = the player. Exit: P unchanged; X, Y high bytes
; clear; A and X as the sound routines leave them, or A low byte = the byte
; that failed the test.
%at($04D040)
EngineSoundsPlayer2:
    php
    sep.b #$30
    lda.w !wram_0E4B
    bne .lD05F
    lda.w !engine_phase_p2
    cmp.b #$06
    bne .lD05F
    rep.b #$20
    lda.w !joy2_pressed_copy
    jsr.w SoundIfFlag40
    rep.b #$20
    lda.w !joy2_held
    jsr.w SoundHoldByFlags
.lD05F:
    plp
    rtl

; @routine 04:D061 EngineSoundsPlayer1
; WHAT IT DOES: As EngineSoundsPlayer2, for player 1: $0DD6, the buttons
; newly pressed at $0D1A and held at $0309.
; HOW: As EngineSoundsPlayer2.
; Entry: as EngineSoundsPlayer2. Exit: as EngineSoundsPlayer2.
%at($04D061)
EngineSoundsPlayer1:
    php
    sep.b #$30
    lda.w !wram_0E4B
    bne .lD080
    lda.w !engine_phase_p1
    cmp.b #$06
    bne .lD080
    rep.b #$20
    lda.w !joy1_pressed_copy
    jsr.w SoundIfFlag40
    rep.b #$20
    lda.w !joy1_held
    jsr.w SoundHoldByFlags
.lD080:
    plp
    rtl

; @routine 03:EC43 HandlerSoundIndex
; WHAT IT DOES: Maps a selection handler address to a sound index: $81B9 to
; 0, $853D to 1, $8411 to 2, $82E5 to 3, any other to 4.
; HOW: A chain of 16-bit compares. The $82E5 case is not reached in the
; traces so far.
; Entry: M 16-bit; A = the handler address. Exit: A = the index; N clear, Z
; from it; C set on a match, else from the last compare; X, Y unchanged.
%at($03EC43)
HandlerSoundIndex:
    cmp.w #$81B9
    bne .l853D
    lda.w #$0000
    rts
.l853D:
    cmp.w #$853D
    bne .l8411
    lda.w #$0001
    rts
.l8411:
    cmp.w #$8411
    bne .l82E5
    lda.w #$0002
    rts
.l82E5:
    cmp.w #$82E5
    bne .other
    lda.w #$0003                    ; not reached in the traces so far
    rts                             ; not reached in the traces so far
.other:
    lda.w #$0004
    rts

; @routine 03:DCEF SoundIndicesFromHandlers
; WHAT IT DOES: Sets the sound indices from the two selection handlers:
; $0D89 from handler A ($18DB), also $0D8B when $10DA or $10DC is nonzero,
; and $0D8A from handler B ($18DD).
; HOW: HandlerSoundIndex on each (the handlers read in bank 0, the rest
; through the data bank), with 8-bit registers for the stores and tests.
; Entry: any widths. Exit: A = handler B's index; X = the byte that decided
; $0D8B ($10DA, or $10DC when it is 0); Y high byte cleared; P unchanged.
%at($03DCEF)
SoundIndicesFromHandlers:
    php
    rep.b #$20
    lda.l !selection_handler_a
    jsr.w HandlerSoundIndex
    sep.b #$30
    sta.w !wram_0D89
    ldx.w !wram_10DA
    bne .both
    ldx.w !wram_10DC
    beq .second
.both:
    sta.w !wram_0D8B
.second:
    rep.b #$20
    lda.l !selection_handler_b
    jsr.w HandlerSoundIndex
    sep.b #$20
    sta.w !wram_0D8A
    plp
    rtl
