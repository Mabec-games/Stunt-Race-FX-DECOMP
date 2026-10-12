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
