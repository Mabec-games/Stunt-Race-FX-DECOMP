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
!records_7f_a = $7F43D7
!records_7f_b = $7F47FF
!records_7f_45b7 = $7F45B7
!records_7f_45d7 = $7F45D7
!records_7f_47b7 = $7F47B7
!records_7f_47d7 = $7F47D7
!records_7f_49b7 = $7F49B7
!records_7f_49d7 = $7F49D7
!records_7f_4bb7 = $7F4BB7
!wram_19B4 = $19B4
!window_edges = $7FF883
!wram_0D75 = $0D75
!sound_hold = $0DD8

!random_word = $0D18
!palette_phase = $0DD5
!player_mode = $0DF4
!wram_0E0C = $0E0C
!wram_0E14 = $0E14
!palette_bits = $F1
!cycle_tilemap = $7F93D6
!obj_04 = $2004
!obj_59 = $2059
!view_position = $0664
!view_matrix = $063A
!view_8325_out = $064C
!selection_handler_a = $18DB
!selection_handler_b = $18DD
!wram_0E4B = $0E4B
!engine_phase_p1 = $0DD6
!engine_phase_p2 = $0DD7
!selection_handler_c = $18DF
!selection_handler_d = $18E1
; Menu scratch, object fields and input used by the selection objects
; (menu/actors.asm): $11E5 a scratch word; $11FB the live list offset of the
; object being run; +$20 and +$22 two fields of the menu objects; $118B
; the player 1 buttons pressed since the menu last cleared it; $11E9 a
; scratch word of MenuActorUploadFrames.
!scratch_11e5 = $11E5
!actor_current_slot = $11FB
!actor_20 = $20
!actor_22 = $22
!menu_joy1_pressed = $118B
!scratch_11e9 = $11E9

; More menu object fields: +$02 the position the sliding objects follow,
; +$1C and +$1E a frame period and its countdown; +$06 a sprite position.
!actor_02 = $02
!actor_1c = $1C
!actor_1e = $1E
!actor_06 = $06

; $11ED: the selection screen's slide position, copied by MenuSelectArrive.
!menu_slide_pos = $11ED

; Object spawning (menu/actors.asm): the record being filled and the
; position in the spawn list.
!menu_spawn_record = $1209
!menu_spawn_pos = $11FD
!sound_block = $706000
!lap_minutes = $F1
!lap_mark = $0EDB
!lap_splits = $0EDE
!lap_times = $0EF0
!lap_digits = $0F69
!save_sums_b_w = $FC43
!save_sums_a_w = $FC46
!save_record_a_w = $FC4A
!save_record_b_w = $FC5E
!save_block_sums_w = $FC33
!save_blocks_w = $FCD0
!save_sum = $F1
!save_block = $F3
!save_block_offset = $F5
!column_pos = $F1
!column_tile = $F3
!wram_0E74 = $0E74
!wram_0D6B = $0D6B
!wram_0D6D = $0D6D
!wram_0D6F = $0D6F
!text_line = $0E3D
!text_tile = $F1
!text_count = $F2
!text_pen = $F3
!text_y = $F4
!text_attr = $F5

; Super FX record programs (object/gsu_record.asm): the record argument
; (the record-making programs leave the new record there, 0 when the pool
; is full), the amount argument (high byte), and record +$0A.
!gsu_record_arg = $70238E
!gsu_in_amount = $702390
!gsu_rec_0a = $70000A

; Object fields and work values used by object/object.asm.
!obj_18 = $2018
!obj_24 = $2024
!obj_3a = $203A
!obj_5f = $205F
!wram_1F58 = $1F58
!wram_1F59 = $1F59
!selection_out_b = $18DB
!obj_19 = $2019
!obj_29 = $2029
!gsu_rec_25 = $700025
!gsu_rec_29 = $700029
; The second view object, read through the direct page like $1A64
; (object/gsu_record.asm).
!view_b = $1A9F
!obj_pos_x_long = $7E200C
!obj_pos_z_long = $7E2010
!wram_1F52 = $1F52
!obj_bucket_long = $7E2040
!gsu_rec_18 = $700018
!wram_1F3C = $1F3C

; The stop screen (system/crash.asm): its backdrop colour, the code it
; scrolls to, and the scroll so far.
!crash_colour = $1A2E
!crash_code = $0717
!crash_scroll = $0719
!wram_0D53 = $0D53
!obj_48 = $2048
!gsu_rec_70 = $700070
!wram_1F36 = $1F36
!wram_1F38 = $1F38
!wram_1F3A = $1F3A
; Non-zero when records may land (object/gsu_record.asm).
!gsu_record_ready = $702482
!gsu_rec_2c = $70002C
!gsu_rec_8a = $70008A
!gsu_rec_82 = $700082
!gsu_rec_34 = $700034
!gsu_rec_36 = $700036
!gsu_rec_38 = $700038
; Contact list entries, reached from a record's +$18 (object/gsu_record.asm):
; the touched record, the next entry, and flags (bit 1: in contact).
!contact_record = $70000C
!contact_next = $70000E
!contact_flags = $700010
!gsu_rec_64 = $700064
!gsu_control_26f2 = $7026F2
; The button sequence (object/gsu_record.asm): frame count, stage, and the
; buttons it reads.
!seq_count = $1A33
!seq_stage = $1A35
!seq_input = $1AF0
!gsu_rec_400 = $700400
!obj_1a_w = $201A
!obj_1c = $201C
; Per-player state bytes, and two race flags (object/object.asm).
!wram_0DD6 = $0DD6
!wram_19ED = $19ED
!wram_0E59 = $0E59
; The attach table entry base (object/object.asm).
!attach_entry = $702396
!gsu_rec_27 = $700027
!gsu_rec_90 = $700090
!gsu_rec_2a = $70002A
!frames_since_tick = $0306
!windows_setup_index = $19A9
!gsu_ab13_polls = $1F21
!dp_06 = $06
!dp_f9 = $F9
!view_matrix_saved = $064C
!view_pos_saved = $065E
!course_picture_offsets = $7F0B96
!level_columns = $0F05
!obj_pos_x_long = $7E200C
!obj_pos_z_long = $7E2010
!map_marker_tiles = $7F0B46
!group_trigger_list = $1AF5
!group_trigger_pos = $1F2B
!group_trigger_code = $1F2D
!obj_24 = $2024
!metasprite_origin_x = $0D3B
!metasprite_origin_y = $0D3C
!metasprite_count = $F1
!oam_size_mask = $F3
!colour_from_r = $344E
!colour_to_r = $3450
!colour_from_g = $3452
!colour_to_g = $3454
!colour_from_b = $3456
!colour_to_b = $3458
!area_row = $94
!area_col0 = $A5
!area_rows_left = $05F2
!bucket_origin = $1F01
!area_window_x = $1F03
!area_window_y = $1F05
!gsu_record_count = $0CEE
!gsu_records_n = $7001C8
!wram_bank_7e = $7E0000
!blend_from = $F1
!blend_to = $F3
!metasprite_set_sel = $0D35
!flash_sprite_x = $0D37
!flash_sprite_y = $0D38
!flash_phase = $0D39
!kept_timer = $0E01
!kept_cycle_c = $0E03
!kept_enable = $0E05
!kept_sub = $0DFB
!kept_acc = $0DFD
!kept_cycle_ab = $0DFF
!kept_colour_c = $7EFF2B
!kept_freeze = $0D53
!kept_colour_b = $7EFF29
!kept_colours_c = $7EB4E9
!kept_colours_a = $7EB505
!kept_colours_b = $7EB521
!map_b_x = $0E09
!map_a_x = $0E0A
!map_b_y_w = $0E0B
!map_a_y_w = $0E0C
!map_marker_step = $F5
!panel_b_shown = $0D55
!gauge_b4_value = $0DF9
!gauge_a4_value = $0DFA
!gauge_b6_value = $0E05
!gauge_a6_value = $0E06
!gauge_b4_x = $0E0E
!gauge_a4_x = $0E0F
!gauge_b4_y = $0E10
!gauge_a4_y = $0E11
!gauge_b6_x = $0E12
!gauge_a6_x = $0E13
!gauge_b6_y = $0E14
!gauge_a6_y = $0E15
!view_area_origin = $1EF5
!fixed_area_origin = $1EFB
!wram_1AEC = $1AEC
!view_pos_y_hi = $0667
!blend_amount = $F5
!blend_target = $3470
!blend_target_amount = $3468
!blended_colours = $000E21
!copy_src_pos = $0CF2
!copy_count = $0CF6
!copy_dest = $07A2
!copy_src = $02D65A
!tilemap_alt_bbd7 = $BBD7
!tilemap_row_e3d7 = $E3D7
!tilemap_main_93d7 = $93D7
!row_count = $F3
!window_layout = $19B6
