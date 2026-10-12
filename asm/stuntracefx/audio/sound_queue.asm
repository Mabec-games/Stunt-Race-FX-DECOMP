; sound_queue.asm: the two sound command queues.
;
; Game code asks for sounds by queueing command bytes. Each queue is a 16-byte
; ring with a head index and a count; once a frame the RAM-resident frame code
; sends the oldest byte of each queue to its APU port (0 or 1) when the sound
; CPU has taken the previous one. C: src/stuntracefx/audio/sound_queue.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"

arch 65816

; @routine 04:D649 SoundPort0QueuePushRaw
; WHAT IT DOES: Queues a sound command for APU port 0 exactly as given.
; HOW: Saves every register, sets the data bank to 0 and joins the port 0
; append below without marking the player.
; Entry: any widths; A low byte = command. Exit: every register unchanged
; except N and Z, which the final pull of DB sets from DB.
%at($04D649)
SoundPort0QueuePushRaw:
    phb
    pha
    phy
    phx
    php
    sep.b #$30
    ldy.b #$00
    phy
    plb
    bra SoundPort0QueueAppend

; @routine 04:D656 SoundPort0QueuePush
; WHAT IT DOES: Queues a sound command for APU port 0 on behalf of the player
; whose car is being processed.
; HOW: Saves every register and sets the data bank to 0; sets bit 7 of the
; command when that is player 2. Then, unless sound is muted: an empty ring
; takes the byte at its head; otherwise, unless 16 are queued, the byte goes
; count places past the head (wrapping at 16) and the count steps.
; Entry: any widths; A low byte = command. Exit: every register unchanged
; except N and Z, which the final pull of DB sets from DB.
SoundPort0QueuePush:
    phb
    pha
    phy
    phx
    php
    sep.b #$30
    ldy.b #$00
    phy
    plb
    ldx.w !current_player
    beq SoundPort0QueueAppend
    ora.b #$80
SoundPort0QueueAppend:
    ldx.w !sound_muted
    bne .done
    ldy.w !sound_port0_count
    beq .empty
    cpy.b #$10
    bcs .done
    pha
    tya
    clc
    adc.w !sound_port0_head
    cmp.b #$10
    bcc .slot
    sbc.b #$10
.slot:
    tay
    pla
    sta.w !sound_port0_queue,y
    inc.w !sound_port0_count
    plp
    plx
    ply
    pla
    plb
    rtl
.empty:
    inc.w !sound_port0_count
    ldy.w !sound_port0_head
    sta.w !sound_port0_queue,y
.done:
    plp
    plx
    ply
    pla
    plb
    rtl

; @routine 04:D69F SoundPort1QueuePush
; WHAT IT DOES: Queues a sound command for APU port 1.
; HOW: As SoundPort0QueuePush on the port 1 ring, without the player mark,
; using X as the index.
; Entry: any widths; A low byte = command. Exit: every register unchanged
; except N and Z, which the final pull of DB sets from DB.
SoundPort1QueuePush:
    phb
    pha
    phy
    phx
    php
    sep.b #$30
    ldx.b #$00
    phx
    plb
    ldx.w !sound_muted
    bne .done
    ldx.w !sound_port1_count
    beq .empty
    cpx.b #$10
    bcs .done
    pha
    txa
    clc
    adc.w !sound_port1_head
    cmp.b #$10
    bcc .slot
    sbc.b #$10
.slot:
    tax
    pla
    sta.w !sound_port1_queue,x
    inc.w !sound_port1_count
    plp
    plx
    ply
    pla
    plb
    rtl
.empty:
    inc.w !sound_port1_count
    ldx.w !sound_port1_head
    sta.w !sound_port1_queue,x
.done:
    plp
    plx
    ply
    pla
    plb
    rtl

; @routine 04:D6E1 SoundReset
; WHAT IT DOES: Empties both sound queues, unmutes, clears the sound state
; bytes and puts 0 on APU ports 3 and 2.
; HOW: 8-bit stores of 0 to the mute flag, $0DD0, the last-port-1 byte, both
; heads and counts, $0E1B and $0E1C, then to ports 3 and 2.
; Entry: any widths. Exit: A low byte = 0; X and Y high bytes cleared (the
; switch to 8-bit index registers drops them); P unchanged.
SoundReset:
    php
    sep.b #$30
    lda.b #$00
    sta.l !sound_muted
    sta.l !wram_0DD0
    sta.l !sound_port1_last
    sta.l !sound_port0_head
    sta.l !sound_port1_head
    sta.l !sound_port1_count
    sta.l !sound_port0_count
    sta.l !wram_0E1B
    sta.l !wram_0E1C
    sta.l !APUIO3
    sta.l !APUIO2
    plp
    rtl
