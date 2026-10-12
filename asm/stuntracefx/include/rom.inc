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
