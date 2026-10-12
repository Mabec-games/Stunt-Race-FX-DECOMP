; wram.inc: work RAM and direct page, as the S-CPU code names it.

includeonce
; A variable whose purpose is not yet established keeps its address as its
; name until it is.

; Sound upload stream (audio/apu_upload.asm): long pointer and its bank byte.
!apu_stream_ptr = $FB
!apu_stream_bank = $FD

; Direct page $3E: scratch left by math/scale.asm.
!scale_scratch = $3E

; The value last written to the Super FX screen mode register (video/dma.asm).
!gsu_scmr_shadow = $0374

; Direct page $FE-$FF: counts the polls while waiting for the Super FX (gsu/run.asm).
!gsu_wait_count = $FE

; Joypads (input/joypad.asm): held buttons, buttons newly pressed this frame,
; and copies the frame code keeps.
!joy1_held = $0309
!joy2_held = $030D
!joy1_pressed = $0311
!joy2_pressed = $0313
!joy1_pressed_copy = $0D1A
!joy2_pressed_copy = $0D1C
!joy1_last = $0D1E
!joy2_last = $0D20

; Screen fade (video/fade.asm): brightness accumulator, high byte = INIDISP
; brightness; bit 15 set = forced blank. $0E3A nonzero: fading out.
!fade_level = $0D60
!fade_out = $0E3A

; BG2 scroll pairs per screen band (video/scroll.asm), and two scroll values
; copied to bank $7F.
!bg2_scroll_y = $0D8E
!bg2_scroll_x = $0D92
!scroll_pair_a = $0D96
!scroll_pair_b = $0D98
!scroll_pair_a_7f = $7F348D
!scroll_pair_b_7f = $7F3493

; The sliding HUD panel's offset (hud/slide.asm); $10 is fully shown.
!hud_slide = $0D57

; Menu frame updates (system/frame_updates.asm).
!frame_update_flags = $11D7
!byte_writes_size = $162D
!byte_writes = $162F
!vram_queue_size = $16AF
!vram_queue_staged = $16B1        ; size, source address, bank, VRAM address
!vram_queue = $16B9
!word_list_a_size = $1191
!word_list_b_size = $1193
!word_list_a = $1195
!word_list_b = $11B5

; HUD (hud/digits.asm): race time in BCD, the sprite offset it is drawn at,
; the HUD hold timer, and the blank flag.
!hud_mode = $0D2B
!hud_blank = $0D4E
!hud_hold = $0E5D
!hud_time_sprite = $0E60
!race_time_fraction = $0E63
!race_time_seconds = $0E64
!race_time_minutes = $0E66
!hud_digits_19ec = $19EC
!digit_count = $F1

; Frame counter, stepped by the raster interrupt.
!frame_count = $0E3B

; Palette (video/palette.asm): animated colours in use; cycling phases.
!palette_animate = $0E5F
!palette_buffer = $7EFD65
!palette_kept = $7EFF27
!cycle_phase_a = $FD5B
!cycle_phase_b = $FD5D
!cycle_phase_c = $FD5F
!cycle_colour_a = $FD85
!cycle_colour_b = $FD87
!cycle_colour_c = $FD89

; Menu gauge (menu/gauge.asm).
!menu_gauge = $117E

; Direct page words the menu frame code passes values in.
!dp_02 = $02
!dp_08 = $08
!dp_fb = $FB

; Super FX RAM: a 64-word-wide picture the menu frame code copies row by row.
!gsu_picture = $703000

; The work-RAM copy of the sprite table (video/oam.asm): 128 four-byte entries
; (X, Y, tile, attributes) from $0375, then the 32-byte high table from $0575.
!oam_buffer = $0375
!oam_buffer_high = $0575

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

; Super FX buffer upload (video/dma.asm): pending flag, which half is ready.
!gsu_upload_pending = $0D32
!gsu_buffer_select = $0D7F

; The Super FX's view parameters (gsu/params.asm): sources in work RAM and
; their places in the Super FX RAM parameter block.
!wram_0D12 = $0D12
!wram_05FA = $05FA
!wram_0664 = $0664
!wram_0666 = $0666
!wram_0668 = $0668
!dp_c5 = $C5
!dp_c7 = $C7
!dp_c9 = $C9
!gsu_param_108a = $70108A
!gsu_param_00d8 = $7000D8
!gsu_param_00da = $7000DA
!gsu_param_00dc = $7000DC
!gsu_param_00de = $7000DE
!gsu_param_00e0 = $7000E0
!gsu_param_00e2 = $7000E2
!gsu_param_2736 = $702736

; A value stepped toward its target every other frame (system/approach.asm).
!wram_11DA = $11DA
!wram_11DB = $11DB
!wram_11DC = $11DC

; Window HDMA table in bank $7F (video/window_hdma.asm): three-byte entries
; (line count, 16-bit indirect address), ended by a zero count.
!window_hdma = $7FF85B
!window_hdma_select = $19B6
!window_hdma_off = $19D8
!window_hdma_19bc = $19BC
!window_lines = $19DC
!window_19e2 = $19E2
!window_19e3 = $19E3
!window_19e4 = $19E4
!window_19e5 = $19E5
!window_hdma_lists = $7ED8DE
!window_hdma_counts = $7EE0B4
!window_hdma_addrs = $7EE0D2

; A 16-bit value that slides toward a target (system/slide.asm), its high byte
; copied to bank $7F.
!slide_flags = $0DA5
!slide_value = $19F6
!slide_step = $19F8
!slide_target = $19FA
!slide_direction = $19FC
!slide_out = $7F3468
!slide_done = $7F3470
