; rom.inc: tables in ROM that the code reads, named where they are used. The
; bytes themselves stay in the raw intake until their area is filed.

includeonce

; Handler address tables (data/tables.asm), four words each.
!handler_table_a = $03B6F1
!handler_table_b = $03B704

; Per-course long pointers, three bytes per course (data/tables.asm).
!course_pointers = $03F4A5

; APU port 0 commands indexed by $0D89 (audio/sound_effects.asm).
!sound_table_d89 = $03EDEF

; Sprite lists in bank $03, read through the data bank (video/oam.asm): items
; of four bytes (X, Y, tile, attributes) or a single $FE, ended by $FF.
!oam_lists = $F883

; Fixed colour intensities indexed by the step (video/colour_math.asm).
!fixed_colour_table = $03FE7B

; A 128-word table read through the data bank by ReadTable9022.
!table_9022 = $9022

; Selection tables (data/tables.asm).
!sel_base_table = $0BB00C
!sel_table = $0BB014
!sel_derived_table = $03B7B1
!sel_out_table = $03B7B5

; Cosine and sine by angle, bytes, read in bank 0 (math/rotate.asm).
!rot_cos_table = $973E
!rot_sin_table = $96FE

; Menu tables in bank $0B (menu/screen.asm, menu/selection.asm).
!menu_oam_lists = $FCA2
!menu_repeat_table = $0BC758

; Menu sprite animations (menu/actors.asm): duration and sprite group words.
!menu_anim_table = $0BAB60
!menu_anim_group = $0BAB62

; APU port 0 command pairs for $0D89/$0D8A (audio/sound_effects.asm).
!sound_pair_table = $0BFB18

; Sprite tile numbers for MenuActorSetTiles (menu/actors.asm).
!menu_tile_table = $0BC528

; Course pictures for the course screen, three bytes per course: input $68
; and input $6A for program $01:CF42 (menu/screen.asm).
!course_picture_a = $03F4F9
!course_picture_b = $03F5A1

; Per-course 32-byte blocks, three-byte pointers (video/gradient.asm), and
; 32 bytes read after them.
!course_block_ptrs = $03F6F1
!rom_1fa800 = $1FA800

; Words selected by $0D48 (system/misc.asm).
!table_02fc58 = $02FC58

; Renderer inputs by entry, read through the data bank (gsu/params.asm).
!param_216_table_a = $8E3E
!param_216_table_b = $8E52

; Byte tables indexed by object values (object/object.asm).
!table_08e8af = $08E8AF
!table_08c056 = $08C056

; Tilemap rows read through the data bank (video/tilemap.asm).
!tilemap_rows_fb4e = $FB4E

; BCD values per course and entry, three bytes per entry (race/time.asm).
!course_values_d814 = $03D814
!course_values_d815 = $03D815

; Byte tables: sound commands by $0D89, sprite palettes (audio, video/oam).
!sound_table_d0d6 = $04D0D6
!palette_bits_e26a = $04E26A
!palette_bits_e30f = $04E30F

; Position lists and record blocks for the bank $7F records (video/records_7f.asm).
!records_list_f634 = $F634
!records_list_f170 = $F170
!records_blocks = $09F88B
!palette_bits_e30b = $04E30B
!palette_bits_e313 = $04E313

; Palette bits per phase of TilemapPaletteCycle, six bytes each (video/tilemap.asm).
!palette_cycle_a = $07CF26
!palette_cycle_b = $07CF2C

; Selection tables (menu/selection.asm).
!selection_bits_b540 = $03B540
!selection_repeat_b464 = $03B464
!selection_derived_b46c = $03B46C

; Window edge lists by window (video/window_hdma.asm).
!window_edge_offsets = $09F155
!window_edge_lists = $09F98B

; Menu object tables in bank $0B (menu/actors.asm, menu/screen.asm): animation
; pairs (two words each), the choice animations (eight bytes per choice, a
; word per position), the kinds of the three choice objects, and the
; cross-fade factors (one word per two frames).
!menu_anim_pairs = $0BC862
!menu_choice_anims = $0BC1B0
!menu_choice_kinds = $0BC15E
!menu_fade_factors = $0BCFE5

; Menu sprite frame tables in bank $0B (menu/actors.asm): per sprite of a
; group the VRAM slot and the graphics base; per frame step a word whose
; bit 15 is the vertical flip and whose rest is the graphics offset.
!menu_frame_slots = $0BCCBC
!menu_frame_bases = $0BCD14
!menu_frames = $0BCC7C

; Object spawning in bank $0B (menu/actors.asm): the record template, the
; spawn lists (words naming entry lists, ended by a negative word) and the
; entry lists (field offset, value; ended by a negative offset).
!menu_actor_template = $0BEA0F
!menu_spawn_lists = $0BE4E7
!menu_spawn_entries = $0BE547

; Sound blocks in ROM bank $1E (audio/sound_data.asm).
!sound_block_8000 = $1E8000
!sound_block_91bc = $1E91BC
!sound_block_afc6 = $1EAFC6
; Inputs $70:020E and $70:0212 of GsuFA1BFromTables, a word per entry (gsu/programs.asm).
!fa1b_input_e = $0797D6
!fa1b_input_12 = $0797E2

; Sprite text lines, a word pointer per line in bank $07 (video/sprite_text.asm).
!text_lines = $078000
; Object path commands, a word and its operands each (object/path.asm).
!path_table = $0885B9

; BG1 horizontal scroll HDMA table of the stop screen (system/crash.asm).
!crash_hofs_table = $00812E

; Words for an attached block's +$32, by entry (object/object.asm).
!attach_values = $098090
; Window setup jump table, called through the data bank (video/windows.asm).
!windows_setup_table = $DFDA

; Metasprites (video/oam.asm): 4 bytes per metasprite (first piece, piece
; count), 5 bytes per piece (X, Y, tile, attributes, size flag).
!metasprite_groups = $04EDCB
!metasprite_pieces = $04EE4B
; High-table size bit masks, read through the data bank (video/oam.asm).
!oam_size_masks = $FA8D
; Tilemap buffer offsets of the twelve level columns (video/tilemap.asm).
!level_column_offsets = $FB6E
; Second metasprite set in bank $1C (video/oam.asm), laid out as the first.
!metasprite_groups_1c = $1CE5AC
!metasprite_pieces_1c = $1CE62C
; Area records for the trigger list, read through the data bank
; (object/object.asm).
!area_records = $C61D
; Window table builders by layout direction (video/window_hdma.asm).
!layout_builders = $DBC9

; The debug font, 6 bytes a character, read through data bank 0
; (system/debug_text.asm).
!debug_font = $9022
; Inputs of the $07:FA5A piece drawers, a word per entry (gsu/programs.asm).
!fa5a_input_e = $0797BE
!fa5a_input_12 = $0797CA
!fa5a_six_dx = $079806
!piece_scale_w = $85CC                 ; a byte per counter step, read through the data bank

; Tiles for the menu screen of spawn list 0, $720 bytes (menu/screen.asm).
!menu_tiles_dc8c = $1CDC8C

; Menu pictures in bank $1C drawn by the Super FX (MenuScreenOpen6,
; menu/screen.asm): each is preceded by its byte count, big-endian. $1C:BE58
; is also copied raw, its first $40 bytes, to VRAM $7000.
!menu_pic_dc38_size = $1CDC36
!menu_pic_c9dc_size = $1CC9DA
!menu_pic_babc_size = $1CBABA
!menu_pic_be58_size = $1CBE56
!menu_pic_c664_size = $1CC662
!menu_pic_c818_size = $1CC816
!menu_pic_becc_size = $1CBECA
!menu_pic_be58 = $1CBE58

; SaveTablesCheck offsets per table: into $70:FC6D and into $70:FC88 (save/records.asm).
!save_table_second = $07B18A
!save_table_first = $07B18D

; The Super FX's arctangent table: 256 words, angle (a full turn is
; $10000) by the ratio of the smaller to the larger side (render/angle.asm),
; in ROM bank $00 as the Super FX sees it.
!fx_atan_table = $009E40
; Gradient run lengths (video/gradient.asm): a byte per run, 0 ends; the
; table at +0 serves most courses, +$28 the first list of course $0A and
; +$50 the second list of course 1. A course block's colours start at +$20;
; $03:F745 holds a long pointer per course: the second colour block (its
; word 0 for none).
!gradient_run_table = $1FB44E
!course_block_colours = $0020
!course_block2_ptrs = $03F745

; A zero byte, the fixed DMA source WramFillZero clears work RAM from
; (system/misc.asm).
!zero_byte_038d26 = $038D26

; Per-course palettes (video/palette.asm): long pointers, three bytes per
; course, to the palette ($03:F649) and the third copy's palette
; ($03:F69D); colours are read from offset $20. $EF1F (bank $03, through
; the data bank) holds 16 words of bits, one per colour: clear where the
; third palette copy is kept one step off the second.
!course_palette_ptrs = $03F649
!course_palette2_ptrs = $03F69D
!palette_differ_bits = $EF1F

; Per-course picture records in bank $03, three bytes per course, read
; through the data bank by RaceVideoSetup (video/screen_setup.asm): an
; input word and byte for program $01:CF42. $F807 is read and not used.
!course_picture_b_w = $F5A1
!course_picture_c_w = $F54D
!course_picture_d_w = $F5F5
!course_table_f807_w = $F807
; The menu's button sequence, one mask word a step (menu/handlers.asm).
!button_sequence = $0BD66D

; The menu choice a left or a right press moves to from each choice, a
; word per choice (menu/choices.asm).
!choice_left = $0BC81F
!choice_right = $0BC827

; The menu choice for each saved 2-bit setting, a word per value
; (menu/choices.asm).
!choice_from_save = $0BB9BA

; The speed gauge's value for each scaled speed, by vehicle kind
; (menu/gauge.asm).
!gauge_table = $0BB158

; "MASTER" and "EXPERT" in the sprite font, six tiles each (hud/race_text.asm).
!class_names = $02EFD6
; Digit sizes by animation step, a word per step (race/time_bonus.asm).
!digit_size_grow = $07918F
!digit_size_bounce = $0792AD
; Inputs $70:020E and $70:0212 of GsuFA35FromTables, a word per entry (gsu/programs.asm).
!fa35_input_e = $0797EE
!fa35_input_12 = $0797FA
; TimeBonusTally's states, a word handler address per state (race/time_bonus.asm).
!time_bonus_states = $8E9F
; TimeBonusTransfer's digit animation states, a word handler per state, and
; TimeLeftDigits' large digit placements, four words each, both read in bank
; $07 (race/time_bonus.asm).
!bonus_digit_states = $8C43
!time_digit_large = $8DDA
; Sine words by angle / 256 (257 entries), read by the Super FX in bank 0
; (math/fx_rotate.asm).
!fx_sine16_table = $983E

; The Super FX's sine table: 257 words in 1.15, a full turn in 256 steps
; (render/matrix.asm), in ROM bank $00 as the Super FX sees it.
!fx_sine_table = $00983E

; SpritePieces3's tables (video/oam.asm): a set bias per selector byte, the
; piece set offsets (a word per set, the index times 4), and the pieces
; (five bytes: X, Y, tile, attribute and one unused).
!piece_set_bias = $07B981
!piece_sets = $1CF09E
!piece_bytes = $1CF11E

; MusicLoadForCourse's tables (audio/sound_data.asm): per music, three
; bytes (block list address, $FFFF for none; bank) for the first list at
; +0 and the second at +15; and the course blocks in bank $19.
!music_lists = $04D41E
!music_course_blocks = $190000

; WindowsInit's tables in bank $09 (video/window_hdma.asm): the colour HDMA
; list ($FFFF ends it), the slide value and $19E6 tables by $0D4A, and a
; layout record word per $0D89.
!window_colour_list = $09FB74
!slide_value_table = $09F127
!window_19e6_table = $09F115
!layout_records = $09F14D

; Race phase handler tables in bank $07, a word per phase (race/
; player_stats.asm), and the race code PlayerStatsThenPhaseDD35 jumps to
; while a fade runs.
!phase_handlers_e232 = $E232
!phase_handlers_e24f = $E24F
!phase_handlers_dd35 = $DD35
!race_fade_tail = $EBE9

; A course script entry, read through the data bank at $8000 + Y
; (object/object.asm).
!script_entry = $8000

; The course script's opcode handlers, a word per even opcode (bank $08).
!course_script_ops = $8000

; MenuScreenOpenPreview's pictures in bank $17 (menu/course_preview.asm),
; each after its big-endian byte count.
!preview_picture_a = $17CC78
!preview_picture_b = $17CE78
!preview_picture_c = $17DD8C
!race_e944 = $E944
BigSpritesCountDraw = $079EE9                   ; 07:9EE9, the $0E5C alternative to BigSpritesRow5/6 (not filed yet; a label so branches reach it)
!course_values_d813 = $03D813
!course_values_d810 = $03D810          ; by $0D4A
!mode1_values_d8a3 = $03D8A3           ; by ($0E3D - $0C) * 4 + $0D89
!object_speeds = $08B32B               ; speed bytes by Super FX record word $5E
!special_tag_table = $B4EB             ; the tags RecordsLoadSpecialCourse reads through the data bank
!cursor_x_table = $EB49               ; MenuCursorFourWay's cursor X by position (data bank)
!menu_cycle_frame_offsets = $0BDFCB   ; MenuSelectionFramesCycle: 4 words, each frame's offset in the picture
!menu_cycle_selection_offsets = $0BDFD3 ; MenuSelectionFramesCycle: a word per selection, its picture's offset
!results_car_anim = $0BDA0F          ; ResultsPlaceCar: the car animation per kind (4 words)
!results_car_anim_leader = $0BDA17   ; ResultsPlaceCar: the same for the leader's place
!results_blink_palettes = $0BDA86    ; ResultsLeaderBlink: palette bits per step (2 words)
!results_letter_tiles = $0BDDD3     ; ResultsLetterTiles: the tile of each letter (32 words)
!oam_stage_handlers_bb3c = $BB3C     ; OamStageDispatchBB3C: 8 handlers by $0D2F (bank 07)
!oam_stage_handlers_bba2 = $BBA2     ; OamStageDispatchBBA2: 8 handlers by $0D2F (bank 07)
GsuF329At93x6C = $078A90                        ; 07:8A90, the draw tail of the routine at 07:89F5 (not filed yet; a label so jumps reach it)
WindowDigitStepEC6B = $07EC6B                   ; 07:EC6B, inside the routine at 07:E944 (not filed yet)
WindowDigitStepEBDD = $07EBDD                   ; 07:EBDD, inside the routine at 07:E944 (not filed yet)
!race_fade_stage_handlers = $EC18   ; RaceFadeStageDispatch: handlers by $0D48 (bank 02)
!glyph_spr_init_x = $04DA7A         ; GlyphSpritesInit: x per sprite
!glyph_spr_init_glyph = $04DA8E     ; GlyphSpritesInit: glyph per sprite
!oam_word_row_letters = $D5AD       ; OamWordRow5: the five letters (bank 07, through the data bank)
!glyph_messages = $DF6D              ; GlyphListMessageAndTime: the messages' addresses (bank 04, through the data bank)
!glyph_message_length = $0000        ; a message: its length byte (indexed by its address)
!glyph_message_letters = $0001       ; then a glyph word per letter
!glyph_spr_sizes = $DAA2             ; GlyphSpritesGrowAndTimes: per size step, the size (bank 04, data bank)
!glyph_spr_scales = $DAB2            ; the scale
!glyph_spr_dx = $DAC2                ; the x offset
!glyph_spr_dy = $DAD2                ; the y offset
ChampionshipStageCC83 = $07CC83                ; ChampionshipScreenStep: a stage elsewhere in bank 07 (not filed yet)
ChampionshipStageCC9B = $07CC9B                ; ChampionshipScreenStep: a stage elsewhere in bank 07 (not filed yet)
ChampionshipStageCF6F = $07CF6F                ; ChampionshipScreenStep: a stage elsewhere in bank 07 (not filed yet)
ChampionshipStageCCBF = $07CCBF                ; ChampionshipScreenStep: a stage elsewhere in bank 07 (not filed yet)
!championship_palette = $1F8600     ; ChampionshipScreenStep: the screen's 256 colours
ObjectExitPlx = $09C3EB                         ; 09:C3EB, PLX then RTS (a shared exit, not filed yet)
DemoRunBody = $038683                           ; 03:8683, inside the routine at 03:8648 (not filed yet)
WindowSlideCheckD440 = $7ED440                  ; 7E:D440, the slide step after WindowSlideCheck (not filed yet)
