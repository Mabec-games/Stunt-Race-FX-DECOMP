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
!map_marker_tiles = $7F0B46
!group_trigger_list = $1AF5
!group_trigger_pos = $1F2B
!group_trigger_code = $1F2D
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
!digit62_override = $0D4C
!digit62_value = $0DF4
!flash_count = $0E1F
!flash_n = $F9
!flash_2n = $FA
!player_pal_offset = $0D83
!blend_set_a = $0E21
!blend_set_b = $0E27
!player_palettes = $7E01C8
!layout_step = $0019B8
!layout_dir = $0019BA
!layout_moving = $0019BC
!layout_request = $0019BD
!layout_limits = $7EDBC5
!window_setup_bytes = $0019AC
!backdrop_half = $000D7F
!view_angle = $0006CD
!backdrop_0d96 = $0D96
!backdrop_scroll_base = $7EE811
!view_06cf = $06CF
!view_0da2 = $0DA2
!backdrop_rows_left = $F1
!dp_0a = $0A
!dp_0b = $0B
; Objects (object/object.asm): the record's previous link, how many are in
; use, and a direct page scratch word.
!obj_02 = $2002
!object_count = $0CEE
!dp_3e = $3E
; Debug text (system/debug_text.asm): nibble half, second nibble, keep mask,
; draw place and its base, the string pointer, Super FX RAM, three error flags.
!text_half = $0307
!text_high = $0308
!text_mask = $1F2C
!text_pos = $46
!text_base = $48
!dp_54 = $54
!gsu_ram = $700000
!gsu_flag_26f0 = $7026F0
!gsu_flag_1090 = $701090
!gsu_flag_2650 = $702650
; An object ObjectRelease never frees, and the count of refused frees
; (object/object.asm).
!wram_034C = $034C
!wram_035C = $035C
!obj_1d = $201D
!obj_1f = $201F
!handler_object = $034C
!handler_addr = $0354
!handler_bank = $0356
!piece_x = $F1
!piece_counter = $10CB

; The menu sprite builder (menu/actors.asm, MenuOamBuild): bytes used in the
; sprite table and its high table this frame, the scroll's Y (X is
; !menu_slide_pos), a sprite's X high bit and size while they are shifted
; in, and a sprite group's position relative to the scroll; the object
; fields +$0C (size in bit 0) and +$0E (tile and attributes) of a
; single-sprite object.
!oam_low_count = $1205
!oam_high_count = $1207
!menu_scroll_y = $11EF
!oam_x_high = $11F1
!oam_size = $11F3
!oam_group_x = $11FD
!oam_group_y = $11FF
!actor_0c = $0C
!actor_0e = $0E

; Set by MenuScreenOpen0 (menu/screen.asm): $1189 (to $1234), $118F and
; $0D3F (to $1E); their readers are not filed yet.
!menu_1189 = $1189
!menu_118f = $118F
!wram_0D3F = $0D3F

; Tested with the held buttons by MenuHeldRestart (menu/frame.asm).
!wram_18FB = $18FB
!wram_000A = $000A
!save_tables_a_w = $FC88
!save_tables_b_w = $FC6D
!save_table_sums = $70FC2D
!table_first = $F3
!table_second = $F5
!save_block_groups = $70FFD0
!save_bad_flag = $001FFE
!save_block_groups_w = $FFD0

; Super FX RAM read by bank 01's Super FX code (render/): three vectors
; of three words multiplied by the matrix at $00E4 (matrix.asm), and the
; address (plus 4) of the word pairs the point records take (points.asm).
!gsu_vectors = $700116
!gsu_point_pairs = $70005A
!fx_draw_x = $70020A
!fx_draw_y = $70020C
!fx_draw_advance = $70020E
!fx_draw_rows = $700210
!fx_draw_step_x = $700212
!fx_draw_step_y = $700214
!fx_noise_seed = $70021C
; Gradient colour lists in bank $7F built by GradientBuildColours
; (video/gradient.asm): the first list from $EF57, the second from $F357,
; and $100 bytes from $F757 cleared; $F1 counts a run down, $F3 is the
; write offset.
!gradient_colours = $7FEF57
!gradient_colours_clear = $7FF757
!gradient_run_left = $F1
!gradient_out = $F3

; Cleared or set by ViewDefaults and its shared tail (gsu/programs.asm);
; their readers are not filed yet.
!dp_99 = $99
!dp_9c = $9C
!dp_9d = $9D
!dp_9f = $9F
!dp_a0 = $A0
!dp_a1 = $A1
!dp_bd = $BD
!dp_d3 = $D3
!wram_035D = $035D
!wram_035E = $035E
!wram_1A6C = $1A6C
!gsu_word_1c4 = $7001C4
!gsu_word_1d0 = $7001D0
!gsu_word_108c = $70108C

; Sound indices from the selection handlers (audio/sound_effects.asm): $0D8B
; is set too when $10DA or $10DC is nonzero.
!wram_0D8B = $0D8B
!wram_10DA = $10DA
!wram_10DC = $10DC

; RaceVideoSetup (video/screen_setup.asm): the second tilemap buffer in bank
; $7F, cleared when $0D62 is set; $F1 steps the rows of the course tilemap;
; $0D24 3 or more makes RaceVideoInit clear $0EED-$0EEF, $0E6C and $0E6E.
!tilemap_7f9bd7 = $7F9BD7
!tilemap_row_offset = $F1
!wram_0D24 = $0D24
!wram_0EED = $0EED
!wram_0E6C = $0E6C
!wram_0E6E = $0E6E
; Cleared by MenuButtonSequence when X is pressed (menu/handlers.asm); its
; reader is not filed yet. Written with a long store.
!wram_1913 = $001913

; Player 2's buttons newly pressed this frame, as the menu collects them
; (menu/choices.asm); $118B is player 1's.
!menu_joy2_pressed = $118D

; The saved progress byte in Super FX RAM: bit 2 opens the menu's fourth
; choice (menu/choices.asm).
!save_progress = $70FC0B

; Saved menu settings in Super FX RAM, two bits a choice
; (menu/choices.asm, MenuChoicesFromSave).
!save_settings_a = $70FC0C
!save_settings_b = $70FC0D
!save_settings_c = $70FC0E

; Cleared when the second settings screen is left (menu/choices.asm);
; written with long stores.
!wram_190B = $00190B
!wram_190D = $00190D

; Set with the HUD positions by MenuCourseScreenSetup (menu/course_screen.asm).
!wram_0E0D = $0E0D

; The speed gauge (menu/gauge.asm): the value shown, a word per vehicle
; of speeds in Super FX RAM, and the object field (+$27) whose handler
; word gives the vehicle's kind.
!speed_gauge = $000E49
!gsu_speeds = $70004A
!obj_handler_27 = $7E2027

; Serial joypad bits collected by JoypadPortCheck (input/joypad.asm).
!joy_serial_bits = $1182

; Player 2's vehicle object, as $0346 is player 1's (video/backdrop.asm).
!wram_034A = $034A

; The backdrop's vertical scroll base, a word per screen half, stored just
; before BackdropScrollFromView in the work RAM code (video/backdrop.asm).
!backdrop_y_base = $7EE920

; The screen window's HDMA table entries in bank $7F that
; WindowOpenCloseStep rewrites (system/vblank_updates.asm): line counts at
; $134A and $134E, and $134C/$134D; and the line counts its mode 3 sets.
!window_table_7f_134a = $7F134A
!window_table_7f_134c = $7F134C
!window_table_7f_134d = $7F134D
!window_table_7f_134e = $7F134E
!window_lines_12db = $7F12DB
!window_lines_12e8 = $7F12E8
!window_lines_12f6 = $7F12F6
!window_lines_1301 = $7F1301
!wram_1921 = $001921
!wram_1925 = $001925
!wram_1927 = $001927
!wram_191B = $00191B
!wram_191D = $00191D

; The two colours VblankUpdates alternates colour $D9 between, stored in
; the work RAM code (system/vblank_updates.asm).
!blink_colours = $7F0DA8

; The second speed gauge (menu/gauge.asm): its value and the value shown,
; and two more per-vehicle words in Super FX RAM it reads.
!speed_gauge_2_value = $1180
!speed_gauge_2 = $000E47
!gsu_speed_44 = $700044
!gsu_speed_50 = $700050

; $0D4A, a menu selection used for the course number (race/course.asm).
!wram_0D4A = $0D4A

; Eight 8-byte records after the sprite table (video/sprite_slots.asm): the
; mask of those in use, the first in use, record 1's countdown, the
; scratch word the mask is shifted in, and the record fields, as direct
; page offsets indexed by the record.
!slot_mask = $05D5
!slot_first = $06D8
!slot1_count = $08FB
!slot_scan = $1F2B
!slot_00 = $00
!slot_04 = $04
!slot_05 = $05
!slot_06 = $06
!slot_07 = $07

; A record tied to an object (object/orient.asm), as offsets from the
; record: flags, the tied object's offset in the object table, a position
; and three angles.
!tie_flags = $00
!tie_object = $02
!tie_x = $08
!tie_y = $0A
!tie_z = $0C
!tie_angle_0e = $0E
!tie_angle_10 = $10
!tie_angle_12 = $12

; The tied object's +$22, given to the Super FX by ViewFromRecord
; (object/orient.asm).
!gsu_tied_22 = $70247A
; Inputs of the Super FX digit program $07:F15B (race/time_bonus.asm): X, Y,
; the step to the next digit, and the digit size.
!gsu_digit_x = $70020A
!gsu_digit_y = $70020C
!gsu_digit_step = $70020E
!gsu_digit_size = $700210
; !wram_0E6C (above) is the time bonus to show; nonzero also shows the
; bonus digits (race/time_bonus.asm).
; The time bonus animation (race/time_bonus.asm): its state (0-24,
; even), the amount counted per frame, a copy of the time taken back, a
; count byte and a flag the counting tests, and the flag that starts the
; second animation.
!time_bonus_state = $0D65
!time_bonus_step = $0E7C
!time_taken_copy = $0E76
!wram_0D5A = $0D5A
!wram_0D5B = $0D5B
!wram_19F0 = $19F0
; Renderer inputs $70:0212 and $70:0214 (gsu/programs.asm, race/time_bonus.asm).
!gsu_in_212 = $700212
!gsu_in_214 = $700214
; TimeBonusTransfer's digit animation: its state and the digit size; the
; target it counts $0E74 towards (race/time_bonus.asm).
!bonus_digit_state = $0D67
!bonus_digit_size = $0D69
!wram_19EE = $19EE
; TimeLeftDigits (race/time_bonus.asm): the time last drawn, and the byte
; whose high four bits pick the large digit's placement.
!time_left_shown = $0E78
!wram_0E69 = $0E69
; Lap time display (hud/lap_time.asm): record A's five digit bytes (record
; B is !lap_digits), player 1's lead (signed), the no-record mark for A
; ($FFFF), the large-record flag, and two packed times.
!lap_record_a = $0F64
!p1_lead = $0FA3
!wram_0F2B = $0F2B
!wram_0E57 = $0E57
!packed_time_b = $1161
!packed_time_a = $1163
; Direct page $F1 as BinaryToBcd builds its result (hud/lap_time.asm).
!bcd_digits = $F1
; Super FX side of the object step (object/fx_sums.asm): two vector sums,
; the rotation matrix's words (short address), and record fields.
!fx_world_sum = $702340
!fx_local_sum = $702346
!fx_matrix_w = $00E4
!fxrec_half_x = $1E
!fxrec_scaled_x = $2E
; Car Super FX record fields for the lap count (race/fx_course.asm).
!fxrec_segment = $58
!fxrec_laps = $7A
!fxrec_counted = $86
; Super FX record fields and the push (object/fx_record.asm).
!fx_push = $7023A4
!fxrec_next = $0E
!fxrec_flags10 = $10
!fxrec_list = $18
!fxrec_y = $20
!fxrec_flags40 = $40
!fxrec_y_held = $60
; The two record pools' chain heads, short addresses (object/fx_pool.asm).
!fx_pool_first = $1098
!fx_pool_second = $1D70
; Record motion fields (object/fx_motion.asm).
!fxrec_flags1c = $1C
!fxrec_velocity = $2E
; The three angles FxMatrixFromAngles reads, short address (math/fx_rotate.asm).
!fx_angles_w = $0020
; Record speed and alignment (object/fx_record.asm).
!fxrec_speed = $4A
!fxrec_align = $6C
; The collision push (object/fx_collide.asm): the unit vector (short
; addresses), the closing speed, record mass and contact words.
!fx_normal_x_w = $0068
!fx_normal_y_w = $002C
!fx_normal_z_w = $002E
!fx_closing = $7026F6
!fxrec_mass = $2A
!fxhit_velocity = $04
!fxhit_x = $08
; Car record direction bits (race/fx_course.asm).
!fxrec_direction = $70
; The cars' list and rank fields (race/fx_rank.asm).
!fx_cars = $70262E
!fxrec_next_car = $74
!fxrec_to_go = $84
!fxrec_rank = $88
; Record fields the amount programs change (object/fx_record.asm).
!fxrec_6a = $6A
!fxrec_82 = $82
; The R10 stack Super FX programs start with (object/fx_pool.asm).
!fx_stack = $0240
; The list of all Super FX records and their list and pool fields
; (object/fx_pool.asm).
!fx_records = $702332
!fxrec_all_next = $04
!fxrec_all_prev = $06
!fxrec_pool = $08
; Record creation (object/fx_pool.asm): mode $0422 records' count and list,
; and the fields set.
!fx_count_422 = $702482
!fx_list_422 = $702398
!fxrec_flags16 = $16
!fxrec_mode = $1A
!fxrec_link_422 = $26
; Car set-up (object/fx_car.asm): the script's values and the part and car
; record fields.
!fx_part_bytes = $0052
!fx_part_mode = $700068
!fx_script_at = $702392
!fx_car_26f2 = $7026F2
!fxpart_0c = $0C
!fxpart_12 = $12
!fxpart_next = $34
!fxpart_3c = $3C
!fxpart_44 = $44
!fxpart_46 = $46
!fxpart_50 = $50
!fxcar_parts = $3E
!fxcar_entry = $6E
!fxcar_8c = $8C
; Super FX RAM the start-up program sets (gsu/fx_init.asm).
!fx_ram_test = $70235C
!fx_ram_test_w = $235C
!fx_ram_bad = $7026F0
!fx_init_2732 = $702732
!fx_init_2636 = $702636
!fx_init_2380 = $702380
!fx_init_2382 = $702382
!fx_init_2642 = $702642
!fx_init_2b92 = $702B92
!fx_init_2b94 = $702B94
!fx_init_2684 = $702684
!fx_init_2686 = $702686
!fx_init_1090 = $701090
!fx_init_2470 = $702470
!fx_init_273c = $70273C
!fx_init_2472 = $702472
; A model's unpacked points and the variant word that picks among its
; parts, short addresses (render/fx_points.asm).
!fx_points_w = $2488
!fx_variant_w = $0044
; The traffic test's copies and results (race/fx_traffic.asm).
!fx_me_x = $7026F6
!fx_me_z = $7026FA
!fx_me_vx = $7026FC
!fx_me_vz = $702700
!fx_traffic_dist = $70263A
!fx_traffic_close = $702638
!fxrec_sides = $42
; The touch test's distance and closing speed (object/fx_collide.asm).
!fx_normal_x = $700068
!fx_normal_z = $70002E
!fx_reach = $70006A
!fxrec_size = $0C
; A contact's size and other record (object/fx_collide.asm).
!fx_contact_bytes = $12
!fxcon_other = $0C
; Record sizes held for the box tests (object/fx_collide.asm).
!fx_sizes = $70236C
; Car push fields (race/fx_traffic.asm).
!fxrec_yaw_rate = $34
!fxrec_spread = $7C
; Car pose (object/fx_car.asm): angles, the position for the parts,
; part fields.
!fx_car_pos = $702354
!fxcar_pitch = $24
!fxcar_yaw = $26
!fxcar_roll = $28
!fxpart_world = $36
; Car creation (object/fx_car.asm).
!fx_car_bad = $702650
!fx_car_2392 = $702392
!fx_car_2394 = $702394
!fx_car_2396 = $702396
!fx_car_002c = $70002C
!fx_car_0022 = $700022
!fx_car_108c = $70108C
!fx_car_last = $702474
!fxcar_0a = $0A
!fxcar_64 = $64
!fxcar_script = $66
; The nearest car to the player (race/fx_traffic.asm).
!fx_player = $702476
!fx_near_level = $702642
!fx_near_side = $702644
!fx_near_band = $702646
!fx_near_car = $702648
!fx_near_z = $70264A
!fx_near_x = $70264C
!fx_view_x_w = $00D8
!fx_view_z_w = $00DC
!fx_view_yaw_w = $00E0
; Car hit fields and the hit (object/fx_car.asm).
!fx_hit = $702334
!fx_hit_scale = $702678
!fx_hit_on = $70263C
!fx_hit_sum = $70263E
!fxcar_impact = $62
!fxcar_44 = $44
; Computer driving fields (race/fx_drive.asm).
!fxcar_steer_bias = $3A
!fxcar_stuck = $7E
; Steering rates (race/fx_drive.asm).
!fx_steer_rate = $702662
!fx_steer_rate2 = $702664
!fx_steer_max = $702666
!fx_steer_max2 = $702668
!fx_steer_back = $70266A
; Push application (object/fx_sums.asm).
!fx_push_flags = $702480
!fx_push_amount = $702378
!fx_push_turn = $70247E

; The head of the Super FX's draw list of object slots (render/sort.asm).
!gsu_draw_list = $70023C

; The window slide position (video/window_hdma.asm): the line the moving
; window band starts at, stepped by WindowSlideDown/WindowSlideUp.
!window_slide_pos = $001A14

; $19F2: set to 1 when HudCountMove19EE moves the count at $19EE to $19F0
; (hud/digits.asm).
!wram_19F2 = $19F2

; The save's unlock bytes in bank $70, through the data bank (save/
; records.asm): $FC06-$FC0F (courses and classes opened) and their 8-bit
; checksum word at $FC14.
!save_unlocks = $FC06
!save_unlocks_sum = $FC14

; The piece sprite inputs of SpritePieces3 (video/oam.asm): set selector
; and frame, then the X, Y, tile and palette added to each piece.
!sprite_piece_select = $10BB
!sprite_piece_x = $10BD
!sprite_piece_y = $10BF
!sprite_piece_tile = $10C1
!sprite_piece_frame = $10C3
!sprite_piece_palette = $10C5
!sprite_piece_count = $F1

; The music table index (music * 3) MusicLoadForCourse computes through
; direct page (audio/sound_data.asm).
!music_index = $F1

; WindowsInit's targets (video/window_hdma.asm): the first window band
; entry, the four band entries at $7F:FFD1, the colour HDMA list it copies
; to $7F:FF8B, and the shifted copy of the window edges.
!window_band_first = $7FF857
!window_bands = $7FFFD1
!window_colour_hdma = $7FFF8B
!window_edges_copy = $7FFB03
; Window, slide and layout state WindowsInit sets (video/window_hdma.asm).
!wram_19BE = $19BE
!wram_1A07 = $1A07
!wram_1A0A = $1A0A
!wram_1A0B = $1A0B
!wram_1A17 = $1A17
!wram_1A18 = $1A18
!wram_19BF = $19BF
!wram_19C0 = $19C0
!wram_1A0C = $1A0C
!wram_1A0D = $1A0D
!wram_1A08 = $1A08
!wram_1A09 = $1A09
!wram_1A19 = $1A19
!wram_1A1A = $1A1A
!wram_1A15 = $1A15
!wram_1A16 = $1A16
!wram_19AD = $19AD
!wram_19AE = $19AE
!wram_19AF = $19AF
!wram_19B0 = $19B0
!wram_19B1 = $19B1
!wram_1A0E = $1A0E
!wram_1A0F = $1A0F
!wram_1A1B = $1A1B
!wram_1A1D = $1A1D
!wram_1A1F = $1A1F
!wram_1A23 = $1A23
!wram_1A21 = $1A21
!wram_19E6 = $19E6
!wram_19FE = $19FE
!wram_19D1 = $19D1
!wram_19CB = $19CB
!wram_19D3 = $19D3
!wram_19CF = $19CF
!wram_19C5 = $19C5
!wram_19C3 = $19C3
!wram_19F3 = $19F3
!wram_19F5 = $19F5
!wram_19E0 = $19E0
!wram_19DF = $19DF
!wram_19DB = $19DB
!wram_19DD = $19DD
!wram_1A04 = $1A04
!wram_19E8 = $19E8
!wram_19E9 = $19E9
!wram_19EA = $19EA
!wram_19EB = $19EB
!wram_1A06 = $1A06

; Per-player race figures (race/player_stats.asm): speedometer samples and
; their 32-bit totals and counts per player, and bytes of the car's Super
; FX record (+$40-$43, +$4E, +$7A) a byte per player.
!speed_sample = $0DDA
!speed_total_p0 = $0DDC
!speed_total_p1 = $0DE0
!speed_count_p0 = $0DE4
!speed_count_p1 = $0DE6
!player_rec_7a = $0DE8
!player_rec_40 = $0DEC
!player_rec_41 = $0DEE
!player_rec_42 = $0DF0
!player_rec_43 = $0DF2
!player_rec_4e = $0DF6
!fade_level_hi = $0D61
!gsu_rec_41 = $700041
!gsu_rec_43 = $700043
!gsu_rec_4e = $70004E
!gsu_rec_7a = $70007A

; ObjectDrawListBuild (object/draw_list.asm): the records written, the
; trigger position and word, the object being looked at, the next record's
; address (direct page), the view side, and the 30-byte draw record's
; fields (offsets from the record, data bank $7E).
!draw_count = $0CEE
!trigger_pos = $1F2B
!trigger_word = $1F2D
!draw_object = $034C
!draw_next = $3E
!draw_side = $1F07
!draw_depth = $0002
!draw_angle_a = $0004
!draw_angle_b = $0005
!draw_angle_c = $0006
!draw_flags = $0007
!draw_model = $0008
!draw_link = $000A
!draw_link_48 = $000C
!draw_link_49 = $000D
!draw_link_4a = $000E
!draw_y = $0010
!draw_x = $0012
!draw_z = $0014
!draw_anim = $0016
!draw_no_link = $0018
!draw_palette = $0019
!draw_frame = $001A
!draw_kind = $001B
!draw_5f = $001C
!draw_60 = $001D
!gsu_wait_count_hi = $FF
!gsu_wait_polls = $1F1F
!obj_26 = $2026
!obj_09 = $2009
!obj_60 = $2060

; ObjectSpawnFromScript (object/object.asm): object fields as long
; addresses, the bank-0 record its +$20 can point to, and the byte at
; $1AF4 it copies to +$15 (and that decides the mirror).
!obj_04_long = $7E2004
!obj_06_long = $7E2006
!obj_15_long = $7E2015
!obj_pos_y_long = $7E200E
!obj_1b_long = $7E201B
!obj_1c_long = $7E201C
!obj_20_long = $7E2020
!obj_36_long = $7E2036
!obj_53_long = $7E2053
!obj_55_long = $7E2055
!obj_57_long = $7E2057
!spawn_base_0a = $000A
!spawn_base_0c = $000C
!spawn_base_0e = $000E
!spawn_mirror = $001AF4

; CourseScriptRun (object/course_script.asm): where it leaves the script
; position, object fields as long addresses, and the state it sets.
!gsu_script_next = $701092
!gsu_script_bank = $701094
!obj_09_long = $7E2009
!obj_27_long = $7E2027
!obj_29_long = $7E2029
!obj_48_long = $7E2048
!obj_49_long = $7E2049
!obj_4a_long = $7E204A
!obj_5a_long = $7E205A
!spawn_counter = $1ADE
!script_flags_long = $001AEC
!script_byte_1f71 = $1F71
!script_byte_1f72 = $1F72
!script_records = $7EFF66
!wram_1AEE_long = $001AEE
!bare_object_a = $1ADA
!bare_object_b = $1ADC

; CameraFollowStep (race/camera.asm): the camera record's fields (direct
; page, D = 0), its target and mode, the view angles and matrix copy.
!cam_flags = $00
!cam_target = $02
!cam_pos = $08
!cam_pitch = $14
!cam_turn = $16
!cam_base = $1A
!cam_lead = $26
!cam_angle_add = $35
!cam_target_obj = $1F33
!cam_mode = $05FA
!cam_lag = $1F2B
!cam_tmp_a = $02
!cam_tmp_b = $08
!cam_tmp_c = $94
!cam_offset_x = $B7
!cam_offset_y = $B9
!cam_offset_z = $BB
!view_pitch = $065E
!view_turn = $0660
!view_roll = $0662
!view_matrix_copy = $064C
!gsu_record_247a = $70247A
!gsu_rec_6c = $70006C
!view_matrix_out = $0608

; RaceFrameStep and RaceStart (race/race_frame.asm): the frames counted
; for the last race frame, a word RaceStart clears, the camera target's
; source, the Super FX word set to $FFFF each frame, and the Super FX screen
; area RaceStart clears ($70:2C00-$5FFF).
!frames_taken = $06D4
!race_word_116a = $116A
!race_target_116c = $00116C
!gsu_view_246c = $70246C
!gsu_screen_area = $702C00

; MenuPreviewCountdown's held-preview state (menu/course_preview.asm).
!menu_1905_long = $001905
!menu_1907_long = $001907
!palette_word_fd65 = $7EFD65

; MenuScreenOpenPreviewFade's fade level, $20 counted down by 2
; (menu/course_preview.asm).
!preview_fade_step = $001909

; The sprite table's high part (size and X bit 8), after the 512 bytes at
; $0375 (video/oam.asm).
!oam_high_table = $0575

; RaceFinishPhases0E/10 (race/race_frame.asm): the finish state bytes, and
; the bank $07 code they call when bit 0 of $0DED is set (never run yet).
!wram_0E58 = $0E58
!wram_0E5B = $0E5B

; RecordsUnpackCourse (save/records.asm): the save's per-course record
; times (12 packed words per course) and names (9 bytes per course), and
; their unpacked copies for display.
!save_course_times = $70FC88
!save_course_names = $70FC6D
!record_times = $0F76
!record_names = $10CF
!time_total = $F1                      ; BCD hundredths, seconds, minutes being summed (07:B67E)
!record_totals = $0F9A                 ; three BCD totals (hundredths, seconds, minutes) of the four-time groups at !record_times
!gsu_rec_66 = $700066                  ; car record word $66: the car's script
!near_car_pos = $0E47                  ; nearest car's level / 16 | side * 16 (07:EDC1)
!near_car_band_sent = $115E            ; $31 - the nearest car's band, as last sent on sound port 1
!near_car_none = $117C                 ; 1 while there is no nearest car
!record_row_time = $F2                 ; records rows: the time's offset in !record_times
!record_row_name = $F3                 ; records rows: the name entry (two bytes of letters, then the car)
!record_page = $F7                     ; records rows: the page's offset in !record_times
!record_row_sprites = $0D75            ; the three records rows' first sprite numbers (words)
!fx_pushes = $7023AE                   ; the 19 pushes of five words FxPushesApply applies
!scaled_words = $7F3432                ; four words set between their bases and targets (BlendStepUp/Down)
!scaled_words_target = $7F343A
!scaled_words_base = $7F3442
!wram_3433 = $3433
!countdown_fraction = $0E68             ; the countdown's BCD fraction (RaceTimeInit sets $0E68-$0E6B to $99; RaceClockStep counts it down)
!wram_0DA6 = $0DA6
!record_time_bcd = $0F5B                ; record times as BCD minutes, seconds, hundredths (three bytes each)
!wram_0D29 = $0D29
!wram_0E62 = $0E62
!wram_10A2 = $10A2
!wram_10A4 = $10A4
!wram_10A6 = $10A6
!wram_10A8 = $10A8
!wram_0D9E = $0D9E
!wram_0DA0 = $0DA0
!wram_0D85 = $0D85
!wram_7EFD5B = $7EFD5B
!wram_7EFD5D = $7EFD5D
!wram_7EFD5F = $7EFD5F
!wram_7EFD61 = $7EFD61
!wram_7EFD63 = $7EFD63
!wram_7F3460 = $7F3460
!wram_7F3462 = $7F3462
!wram_7F346C = $7F346C
!wram_7F346E = $7F346E
!wram_7F3464 = $7F3464
!wram_7F3466 = $7F3466
!wram_7F3474 = $7F3474
!wram_7F3476 = $7F3476
!wram_7F3478 = $7F3478
!wram_7F345E = $7F345E
!wram_7F346A = $7F346A
!wram_0DDE = $0DDE
!wram_0DE2 = $0DE2
!wram_10BA = $10BA
!wram_0D54 = $0D54
!wram_0E49 = $0E49
!wram_0D5C = $0D5C
!wram_0D5D = $0D5D
!wram_0EDC = $0EDC
!wram_0EDD = $0EDD
!wram_0DD9 = $0DD9
!wram_0DE9 = $0DE9
!wram_1A2D = $1A2D
!wram_0DEA = $0DEA
!wram_0DEB = $0DEB
!wram_0DF7 = $0DF7
!wram_0DED = $0DED
!wram_0DEF = $0DEF
!wram_0DF5 = $0DF5
!wram_0E07 = $0E07
!wram_0E08 = $0E08
!wram_0D23 = $0D23
!wram_0D2F = $0D2F
!wram_0D2D = $0D2D
!wram_0D3D = $0D3D
!wram_0E54 = $0E54
!wram_0E3F = $0E3F
!wram_0E40 = $0E40
!wram_0E41 = $0E41
!wram_0E42 = $0E42
!wram_0E51 = $0E51
!wram_0E52 = $0E52
!wram_0E53 = $0E53
!wram_0E90 = $0E90
!wram_0E02 = $0E02
!status_colour = $7EFD8B               ; the colour StatusColourPick picks
!status_colours_a = $7EB5B1            ; four colours, in the work RAM copy of the code
!status_colours_b = $7EB5B9            ; four more
!sound_direct0 = $10E2                  ; a byte sent straight to APU port 0 (then cleared)
!sound_direct1 = $10E3                  ; the same for port 1
!wram_0E16 = $0E16
!window_band_pos = $19D4               ; window band animation: position (high byte = the table entry)
!window_band_step = $19D6              ; its step per frame
!window_band_target = $19D9            ; where it stops
!gsu_rec_5e = $70005E                  ; Super FX record word $5E
!obj_4b = $204B
!attach_y = $1F59                      ; an attached object's y offset from its parent (+$30)
!attach_z = $1F5A                      ; its z offset (+$32)
!wram_0E3E = $0E3E
!wram_0D8C = $0D8C
!wram_0D63 = $0D63
!wram_0D44 = $0D44
!wram_0EEE = $0EEE
!wram_0EEF = $0EEF
!wram_0D25 = $0D25
!wram_340D = $340D
