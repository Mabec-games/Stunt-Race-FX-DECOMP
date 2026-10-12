; vblank_updates.asm: the work the frame code does in the vertical blank
; (run from work RAM bank $7F): the Super FX picture copied to VRAM in
; stages, the window that opens and closes over the screen, the pending
; DMA and register updates (frame_updates.c), a blinking colour and the
; brightness step.
; C: src/stuntracefx/system/vblank_updates.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 7F:0800 GsuPictureUploadStep
; WHAT IT DOES: Copies the Super FX's finished picture to VRAM in stages,
; one a frame, the stage being $0D7D: stage 0 copies 4 rows from $70:2C00
; to VRAM 0, stage 2 4 rows from $3C00 to $0580, stage 4 5 rows from
; $4C00 to $0B00 and completes the picture with BG1's tilemap at $6000;
; stage 6 does nothing; stage 8 copies 4 rows from $5000 to $1760, stage
; $0A 4 rows from $4000 to $11E0, and stage $0C 5 rows from $2C00 to $0B00
; and completes it with the tilemap at $6400. Completing sets $0357 = 1
; and bit 5 of $11D7; each stage sets the next (2, 4, 6, -, $0A, $0C, 6).
; HOW: A jump table at $7F:0809 on $0D7D; VramDmaWideRowsFromGsuRam for
; the rows. P is saved and restored.
; Entry: any widths, D = $0000. Exit: stage 6: A, Y unchanged; X = 6.
; Otherwise as VramDmaWideRowsFromGsuRam (X = $FF, Y = 1), with A's low
; byte = $11D7 after a completing stage. P unchanged.
%ramcode($7F0800, $058000)
GsuPictureUploadStep:
    php
    rep.b #$30
    ldx.w !gsu_buffer_offset
    jmp.w (.stages,x)
.stages:
    dw .l0817&$FFFF, .l082D&$FFFF, .l0843&$FFFF, .l086D&$FFFF
    dw .l0871&$FFFF, .l0887&$FFFF, .l089D&$FFFF
.l0817:
    rep.b #$30
    ldx.w #$0002
    stx.w !gsu_buffer_offset
    ldx.w #$2C00
    ldy.w #$0000
    lda.w #$0003
    jsr.w VramDmaWideRowsFromGsuRam
    plp
    rts
.l082D:
    rep.b #$30
    ldx.w #$0004
    stx.w !gsu_buffer_offset
    ldx.w #$3C00
    ldy.w #$0580
    lda.w #$0003
    jsr.w VramDmaWideRowsFromGsuRam
    plp
    rts
.l0843:
    rep.b #$30
    ldx.w #$0006
    stx.w !gsu_buffer_offset
    ldx.w #$4C00
    ldy.w #$0B00
    lda.w #$0004
    jsr.w VramDmaWideRowsFromGsuRam
    sep.b #$20
    lda.b #$01
    sta.w !wram_0357
    lda.b #$60
    sta.w !BG1SC
    lda.w !frame_update_flags
    ora.b #$20
    sta.w !frame_update_flags
    plp
    rts
.l086D:
    rep.b #$30
    plp
    rts
.l0871:
    rep.b #$30
    ldx.w #$000A
    stx.w !gsu_buffer_offset
    ldx.w #$5000
    ldy.w #$1760
    lda.w #$0003
    jsr.w VramDmaWideRowsFromGsuRam
    plp
    rts
.l0887:
    rep.b #$30
    ldx.w #$000C
    stx.w !gsu_buffer_offset
    ldx.w #$4000
    ldy.w #$11E0
    lda.w #$0003
    jsr.w VramDmaWideRowsFromGsuRam
    plp
    rts
.l089D:
    rep.b #$30
    ldx.w #$0006
    stx.w !gsu_buffer_offset
    ldx.w #$2C00
    ldy.w #$0B00
    lda.w #$0004
    jsr.w VramDmaWideRowsFromGsuRam
    sep.b #$20
    lda.b #$01
    sta.w !wram_0357
    lda.b #$64
    sta.w !BG1SC
    lda.w !frame_update_flags
    ora.b #$20
    sta.w !frame_update_flags
    plp
    rts

; @routine 7F:0D2B VblankUpdates
; WHAT IT DOES: The vertical blank work of the main loop: clears $11D8,
; copies the next stage of the Super FX picture (GsuPictureUploadStep),
; steps the screen window (WindowOpenCloseStep); unless a picture was just
; completed (bit 5 of $11D7) applies the pending palette, VRAM and sprite
; transfers (CgramDmaIfPending, VramDmaQueueFlush, OamDmaIfPending); then
; the pending byte writes (ByteWritesIfPending); when $18B9 is set blinks
; colour $D9 between the two colours at $7F:0DA8 (toggling every fifth
; frame); steps the brightness (HalfRateApproach); clears bit 5 of $11D7
; and waits for the joypad auto-read to end.
; HOW: Work RAM and registers through the data bank, $1901/$18FF/$18B9
; with long addressing. P is saved and restored.
; Entry: any widths, D = $0000. Exit: A low byte = 0, high byte from the
; last step; X, Y as the last routine called left them (X = $18FF * 2
; after the blink); P unchanged.
%ramcode($7F0D2B, $05852B)
VblankUpdates:
    php
    rep.b #$30
    stz.w !menu_frame_wait
    jsr.w GsuPictureUploadStep
    jsr.w WindowOpenCloseStep
    sep.b #$20
    lda.w !frame_update_flags
    and.b #$20
    bne .l0D49
    jsr.w CgramDmaIfPending
    jsr.w VramDmaQueueFlush
    jsr.w OamDmaIfPending
.l0D49:
    jsr.w ByteWritesIfPending
    rep.b #$20
    lda.l !menu_18b9
    beq .l0D90
    lda.l !wram_1901
    inc a
    sta.l !wram_1901
    cmp.w #$0005
    bne .l0D75
    lda.w #$0000
    sta.l !wram_1901
    lda.l !wram_18FF
    inc a
    and.w #$0001
    sta.l !wram_18FF
.l0D75:
    lda.l !wram_18FF
    asl a
    tax
    sep.b #$20
    lda.b #$D9
    sta.w !CGADD
    lda.l !blink_colours,x
    sta.w !CGDATA
    lda.l !blink_colours+1,x
    sta.w !CGDATA
.l0D90:
    rep.b #$20
    jsr.w HalfRateApproach
    sep.b #$20
    lda.w !frame_update_flags
    and.b #$DF
    sta.w !frame_update_flags
.l0D9F:
    lda.w !HVBJOY
    and.b #$01
    bne .l0D9F
    plp
    rtl

; @routine 7F:0DAC WindowOpenCloseStep
; WHAT IT DOES: Steps the window over the screen ($1905 its mode, $1907 its
; size): opening (1) shrinks the size by 4 a frame until it is negative,
; closing (2) grows it by 4 until it is $2C, then clears colour 0 (in
; the palette copy $7E:FD65 and CGRAM); either then sets the mode to 0.
; Each step rewrites the window HDMA table at $7F:134A from the size
; before the step. Mode 3 (not reached in the traces so far) sets the line
; counts at $7F:12DB, $12E8, $12F6 and $1301 to $1921 + 1, and $191B = $16
; - $1921 + $1925, $191D = $45 - $1921 + $1927.
; HOW: 16-bit arithmetic with 8-bit stores into the table; CGRAM through
; the data bank.
; Entry: any widths. Exit: X, Y unchanged; mode 0: A = 0, Z set; mode 1:
; A = the new size, or 0 once negative (then Z set), N, Z, C, V from the
; subtraction; mode 2: A = the new size and P from its compare with $2C,
; or A = 0 with Z set; M 16-bit, X 16-bit. Mode 3: A = $191D; N, Z, C, V
; from its add; M 8-bit, X 16-bit.
%ramcode($7F0DAC, $0585AC)
WindowOpenCloseStep:
    rep.b #$30
    lda.l !menu_1905
    bne .l0DB7
    brl .l0E6E
.l0DB7:
    cmp.w #$0003
    bne .l0DBF
    brl .l0E8B
.l0DBF:
    cmp.w #$0002
    beq .l0E1C
    lda.w #$002C
    sec
    sbc.l !menu_1907
    sep.b #$20
    sta.l !window_table_7f_134a
    sta.l !window_table_7f_134e
    rep.b #$20
    lda.l !menu_1907
    sep.b #$20
    asl a
    bne .l0DF9
    lda.b #$02
    sta.l !window_table_7f_134c
    lda.b #$03
    sta.l !window_table_7f_134d
    lda.l !window_table_7f_134e
    dec a
    dec a
    sta.l !window_table_7f_134e
    bra .l0E03
.l0DF9:
    sta.l !window_table_7f_134c
    lda.b #$00
    sta.l !window_table_7f_134d
.l0E03:
    rep.b #$20
    lda.l !menu_1907
    sec
    sbc.w #$0004
    sta.l !menu_1907
    bmi .l0E14
    rts
.l0E14:
    lda.w #$0000
    sta.l !menu_1905
    rts
.l0E1C:
    lda.w #$002C
    sec
    sbc.l !menu_1907
    sep.b #$20
    sta.l !window_table_7f_134a
    sta.l !window_table_7f_134e
    rep.b #$20
    lda.l !menu_1907
    sep.b #$20
    asl a
    bne .l0E51
    lda.b #$02
    sta.l !window_table_7f_134c
    lda.b #$03
    sta.l !window_table_7f_134d
    lda.l !window_table_7f_134e
    dec a
    dec a
    sta.l !window_table_7f_134e
    bra .l0E5B
.l0E51:
    sta.l !window_table_7f_134c
    lda.b #$00
    sta.l !window_table_7f_134d
.l0E5B:
    rep.b #$20
    lda.l !menu_1907
    clc
    adc.w #$0004
    sta.l !menu_1907
    cmp.w #$002C
    beq .l0E6F
.l0E6E:
    rts
.l0E6F:
    lda.w #$0000
    sta.l !menu_1905
    lda.w #$0000
    sta.l !palette_buffer
    sep.b #$20
    stz.w !CGADD
    stz.w !CGDATA
    stz.w !CGDATA
    rep.b #$20
    rts
.l0E8B:
    sep.b #$20
    lda.l !wram_1921
    inc a
    sta.l !window_lines_12db
    sta.l !window_lines_12e8
    sta.l !window_lines_12f6
    sta.l !window_lines_1301
    rep.b #$20
    lda.w #$0016
    sec
    sbc.l !wram_1921
    clc
    adc.l !wram_1925
    sta.l !wram_191B
    lda.w #$0045
    sec
    sbc.l !wram_1921
    clc
    adc.l !wram_1927
    sta.l !wram_191D
    sep.b #$20
    rts
