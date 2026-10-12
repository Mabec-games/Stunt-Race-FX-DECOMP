; wram.inc: work RAM and direct page, as the S-CPU code names it.

includeonce
; A variable whose purpose is not yet established keeps its address as its
; name until it is.

; Sound upload stream (audio/apu_upload.asm): long pointer and its bank byte.
!apu_stream_ptr = $FB
!apu_stream_bank = $FD

; Sound command queues (audio/sound_queue.asm): two 16-byte rings, drained one
; byte per frame to APU ports 0 and 1 once the sound CPU has taken the last one.
!sound_port0_queue = $0DA8
!sound_port1_queue = $0DB8
!sound_port0_head = $0DC8
!sound_port1_head = $0DCA
!sound_port0_count = $0DCC
!sound_port1_count = $0DCE
!wram_0DD0 = $0DD0
!sound_port1_last = $0DD2       ; the last byte sent on port 1
!wram_0E1A = $0E1A
!wram_0E1B = $0E1B
!wram_0E1C = $0E1C
!sound_muted = $0E4E            ; nonzero: nothing is queued

; Held sounds (audio/sound_effects.asm): the sound each player's car holds.
!player1_held_sound = $0E4F
!player2_held_sound = $0E50

; The player whose car is being processed: 0 or 1.
!current_player = $1A5D
