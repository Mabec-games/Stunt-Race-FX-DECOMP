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
!palette_saved = $7EFF65              ; the palette buffer's saved copy
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

; Three values stepped toward their limits (video/window_anim.asm).
!wram_3435 = $3435
!wram_3437 = $3437
!wram_3439 = $3439

; Selects the APU port 0 command played by SoundFromTableD89.
!wram_0D89 = $0D89
!wram_0D8A = $0D8A

; Three work copies of the palette in bank $7F (video/palette.asm).
!palette_work_a = $7F35D7
!palette_work_b = $7F37D7
!palette_work_c = $7F39D7

; Scroll HDMA table (video/scroll.asm): entries of a line count and a word.
!scroll_hdma = $0010AA

; Save data in battery-backed Super FX RAM (save/checksum.asm): nine bytes
; summed into a checksum kept twice.
!save_bytes = $70FC06
!save_checksum = $70FC14
!save_checksum_copy = $70FC15

; Colour maths (video/colour_math.asm): the CGADSUB value the frame code
; writes, and the fixed colour's intensity step and value in bank $7F.
!colour_math_mode = $0D47
!fixed_colour_step = $7F3408
!fixed_colour = $7F340D

; Course number.
!course = $0E3D

; The value written into the scroll HDMA table (video/scroll.asm).
!scroll_value = $0E37

; Direct-page scratch used by loops beside $F1.
!dp_f3 = $F3
!wram_0064 = $64

; Tilemap buffers (video/tilemap.asm).
!tilemap_buffer_93d7 = $93D7

; The object table (object/object.asm): $136 records of $62 bytes from $2001,
; read through the data bank. Fields used so far: the free-list link (+0),
; position (+$0C, +$0E, +$10), three bytes at +$14, velocity (+$2E, +$30,
; +$32); +$1A and the link at +$4F are reached by long addressing in bank $7E.
!obj_base = $2000
!obj_pos_x = $200C
!obj_pos_y = $200E
!obj_pos_z = $2010
!obj_14 = $2014
!obj_15 = $2015
!obj_16 = $2016
!obj_vel_x = $202E
!obj_vel_y = $2030
!obj_vel_z = $2032
!obj_1a = $7E201A
!obj_link = $7E204F
!wram_0329 = $0329
!wram_032B = $032B
!wram_032D = $032D

; Super FX program inputs and outputs in its RAM (gsu/programs.asm).
!gsu_in_20 = $700020
!gsu_out_26 = $700026
!gsu_dest = $70002C
!gsu_in_2e = $70002E
!gsu_in_34 = $700034
!gsu_in_68 = $700068
!gsu_in_6a = $70006A
!gsu_in_92 = $700092
!gsu_in_a2 = $7000A2
!gsu_out_e4 = $7000E4
!gsu_screen_start = $70021A
!gsu_ramp = $703C00

; The Super FX draw buffer (gsu/screen.asm): $0D7F selects it, $0D7D is its
; offset; the buffer start is kept in direct page too.
!gsu_buffer_offset = $0D7D
!gsu_screen_start_dp = $48

; Direct page words used as Super FX inputs and outputs.
!dp_04 = $04
!dp_2a = $2A
!dp_2c = $2C
!dp_2e = $2E
!dp_30 = $30
!dp_32 = $32
!dp_34 = $34
!dp_94 = $94
!dp_b7 = $B7
!dp_b9 = $B9
!dp_bb = $BB
!dp_ce = $CE
!dp_d0 = $D0

; Rotation (math/rotate.asm): the angle's sine and cosine, and z'.
!rot_sin = $0624
!rot_cos = $0625
!rot_z = $05F2

!wram_05EF = $05EF
!wram_065E = $065E
!gsu_8325_out = $0608
!wram_0D45 = $0D45
!wram_0D81 = $0D81
!wram_0E33 = $0E33
!wram_0E91 = $0E91
!wram_0E92 = $0E92
!wram_10CD = $10CD

; The window HDMA table for channel 1 in bank $7F (video/screen_setup.asm).
!window_table_7f = $7F3410

; Four selections and the values derived from them (data/tables.asm,
; menu/selection.asm).
!selection = $18CB
!selection_out = $18D3
!selection_derived = $18E3
!scale_divisor = $7F340A

; Scratch words used by the menu code.
!scratch_11df = $11DF
!scratch_11e1 = $11E1
!scratch_11e3 = $11E3

; Menu objects (menu/actors.asm): records of $2C bytes from $1265, a stack
; of free record addresses and the list of live ones.
!actor_free_stack = $120D
!actor_free_depth = $11F5
!actor_list = $1239
!actor_list_len = $11F7
!actor_current = $11F9
!menu_sprite_groups = $7F1356
!menu_sprite_attr = $7F135A
!menu_blink_counter = $11DD
!menu_blink_entry = $11E7
!menu_1201 = $1201
!menu_1203 = $1203
!menu_120b = $120B

; Menu frame handshake and exit flag (menu/frame.asm, menu/selection.asm).
!menu_frame_wait = $11D8
!menu_exit = $11EB
!menu_picture_size = $11D5
!menu_1a64 = $1A64
!wram_0357 = $0357
!menu_1905 = $1905
!menu_1907 = $1907
!selection_prev = $18C9
!table_116c = $116C

; Menu object fields, as direct page offsets indexed by the record address.
!actor_handler = $08
!actor_10 = $10
!actor_12 = $12
!actor_kind = $1A
!actor_palette = $24

; Pictures the menu loader keeps in bank $7F (menu/loader.asm).
!menu_picture_a = $7FA7D7
!menu_picture_b = $7F3FD7

; More menu object fields (menu/actors.asm): a count or angle (+$0A) and the
; sprite animation's offset in its table, entry offset and frame timer.
!actor_0a = $0A
!actor_anim = $14
!actor_anim_pos = $16
!actor_anim_timer = $18
!menu_sine_y = $18BF
!menu_sine_y_min = $18C1

; Menu HDMA tables and positions in bank $7F and work RAM (menu/screen.asm).
!menu_hdma_1354 = $7F1354
!menu_pos_18bb = $18BB
!menu_pos_18bd = $18BD
!gsu_picture_copy = $703800
!menu_18b9 = $18B9

; The colour gradient's HDMA tables in bank $7F (video/gradient.asm): CGADD
; per line, and CGDATA entries of a line count and a colour address; the
; two scrolling colour addresses and their sources in work RAM.
!gradient_cgadd = $7F3497
!gradient_cgdata = $7F3489
!gradient_ptr_a = $7F348D
!gradient_ptr_b = $7F3493
!gradient_src_a = $0D96
!gradient_src_b = $0D98

; Variables next to named ones whose purpose is not yet established; each
; keeps its address (or its neighbour's name and position) as its name.
!dp_05 = $05
!rot_z_high = $05F3
!wram_18C3 = $18C3
!wram_18C5 = $18C5
!wram_18C7 = $18C7
!selection_1 = $18CD
!selection_2 = $18CF
!selection_3 = $18D1
!selection_out_1 = $18D5
!selection_out_2 = $18D7
!selection_out_3 = $18D9
!wram_18E5 = $18E5
!wram_18E7 = $18E7
!wram_18E9 = $18E9
!wram_18EB = $18EB
!wram_18ED = $18ED
!wram_18EF = $18EF
!wram_18F1 = $18F1
!wram_18F3 = $18F3
!wram_18F5 = $18F5
!wram_18F7 = $18F7
!wram_18F9 = $18F9
!wram_18FD = $18FD
!wram_18FF = $18FF
!wram_1901 = $1901
!wram_1903 = $1903
!gsu_in_22 = $700022
!gsu_in_24 = $700024
!gsu_out_28 = $700028
!gsu_out_2a = $70002A
!gsu_in_36 = $700036
!gsu_in_38 = $700038
!gsu_in_3a = $70003A
!gsu_in_3c = $70003C
!gsu_in_3e = $70003E
!gsu_in_94 = $700094
!wram_0D48 = $0D48
!wram_0D37 = $0D37
!wram_7EBC2B = $7EBC2B
!wram_7EBC2D = $7EBC2D
!wram_7EBC2F = $7EBC2F
!wram_0DF8 = $0DF8
!wram_0E5C = $0E5C
!wram_1160 = $1160
!gsu_in_216 = $700216
!save_record_a = $70FC4A
!save_record_b = $70FC5E
!wram_0DDA = $0DDA
!wram_0DDB = $0DDB
!wram_0D22 = $0D22
!wram_0346 = $0346
!wram_06CD = $0006CD
!bank0_base = $000000              ; a bank 0 address indexed by a record pointer

; Object fields used by the record code (object/gsu_record.asm): another
; object (+$06), the Super FX record address (+$22), a flags byte (+$25).
!obj_06 = $2006
!obj_22 = $2022
!obj_25 = $2025

; Super FX object records (object/gsu_record.asm), indexed by the record
; address: position, flags, mode, and the word at +$88.
!gsu_rec_pos_x = $70000C
!gsu_rec_pos_y = $70000E
!gsu_rec_pos_z = $700010
!gsu_rec_flags = $700016
!gsu_rec_mode = $70001A
!gsu_rec_88 = $700088
!gsu_match_2476 = $702476
!wram_1F4C = $1F4C
!wram_1F5A = $1F5A
!wram_7EFFA6 = $7EFFA6
!gsu_rec_42 = $700042
!gsu_rec_58 = $700058
!object_list = $7EFFBD
!object_list_len = $7EFFFD
!obj_27 = $2027
!obj_36 = $2036
!obj_bucket = $2040
!obj_bucket_next = $203C
!obj_bucket_prev = $203E
!object_buckets = $7F0000
!gsu_rec_1c = $70001C
!gsu_rec_vel_x = $70002E
!object_list_b = $7EFFAE
!object_list_b_len = $7EFFB8
!gsu_rec_vel_y = $700030
!gsu_rec_vel_z = $700032
!dp_40 = $40
!obj_22_long = $7E2022
!obj_5d = $205D
!wram_0D62 = $0D62
!wram_1F2B = $1F2B
!gsu_rec_6a = $70006A
!gsu_table_addr = $701092
!gsu_table_bank = $701094
!wram_1F4E = $1F4E
!wram_1F50 = $1F50
!obj_20 = $2020
!obj_53 = $2053
!obj_55 = $2055
!obj_57 = $2057
!obj_5a = $205A
!gsu_rec_12 = $700012
!gsu_rec_14 = $700014
!gsu_rec_1e = $70001E
!gsu_rec_20 = $700020
!gsu_rec_22 = $700022
!gsu_rec_24 = $700024
!wram_05E9 = $05E9
!wram_1AEE = $1AEE
!wram_1F2D = $1F2D
!obj_49 = $2049
!obj_4a = $204A
!place_table = $FF66
!gsu_bytes_200 = $700200
!scanline_wait = $06E3
!scanline_wait_high = $06E4
!random_e6 = $E6
!random_e7 = $E7
!random_e8 = $E8
!random_e9 = $E9
!oam_sprite52_x = $0445
!oam_sprite53_x = $0449
!oam_sprite54_attr = $0450
!oam_sprite55_attr = $0454
!oam_sprite56_attr = $0458
!tilemap_7f93d7 = $7F93D7
!tilemap_7fbbd7 = $7FBBD7
!wram_7F345C = $7F345C
!wram_0E15 = $0E15
!wram_1A24 = $1A24
!wram_1A26 = $1A26
!gsu_rec_40 = $700040
!gsu_match_2478 = $702478
!oam_high_16 = $058B
!wram_0DA4 = $0DA4
!wram_1A1C = $1A1C
!wram_1A1E = $1A1E
!wram_1A20 = $1A20
!wram_1A22 = $1A22
!window_preset_count_w = $19E1
!wram_0D2E = $0D2E
!window_presets_a = $7ED9C5
!window_presets_b = $7EDA1D
!obj_link_w = $204F
!approach_value = $3E
!race_time_bcd = $0E70
!wram_0D79 = $0D79
!wram_0D7B = $0D7B
!wram_10D8 = $10D8
!wram_1073 = $1073
!dp_00 = $00
