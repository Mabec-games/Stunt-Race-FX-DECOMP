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
