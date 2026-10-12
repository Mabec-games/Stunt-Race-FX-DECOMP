; map.asm: the course map's markers, one sprite per object, placed from the
; objects' world positions scaled down to map pixels.
; C: src/stuntracefx/hud/map.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 7F:0AD6 MapMarkerSprites
; WHAT IT DOES: Places a map marker sprite for each object in the list at
; $7E:FFBD (object offsets, ended by 0): at X = the object's X ($200C) / 2048
; plus the map origin X in direct page $F1, Y = -(its Z, $2010) / 2048 plus
; the origin in $F3, with the tile word that the low 3 bits of its +$26 pick
; from $7F:0B46. Successive markers go to sprite table offsets Y, Y + $F5,
; ...
; HOW: Each coordinate is shifted right 3 with its sign kept, then the high
; byte taken by XBA (X) or by AND $FF00 (Y) and added to the origin word.
; Direct page $02 and $08 are left with the last tile word and +$26 word;
; the sprite table is written through the data bank.
; Entry: X 16-bit, D = $0000; Y = first sprite table offset. Exit: A = 0;
; X = the list position of the terminating 0; Y past the last marker (low
; bytes only when P restores 8-bit index registers); P unchanged.
%ramcode($7F0AD6, $0582D6)
MapMarkerSprites:
    php
    rep.b #$30
    ldx.w #$0000
.l0ADC:
    lda.l !object_list,x
    bne .l0AE5
    brl .l0B44
.l0AE5:
    phx
    tax
    lda.l !obj_pos_x_long,x
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    xba
    clc
    adc.b !digit_count
    and.w #$00FF
    sta.b !dp_02
    lda.l !obj_pos_z_long,x
    eor.w #$FFFF
    inc a
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    clc
    adc.b !dp_f3
    and.w #$FF00
    ora.b !dp_02
    sta.b !dp_08
    sta.w !oam_buffer,y
    rep.b #$20
    lda.l !obj_22_long+4,x
    sta.b !dp_08
    and.w #$0007
    asl a
    tax
    lda.l !map_marker_tiles,x
    sta.b !dp_02
    lda.b !dp_02
    sta.w !oam_buffer+2,y
    plx
    inx
    inx
    tya
    clc
    adc.b !dp_f3+2
    tay
    jmp.w .l0ADC
.l0B44:
    plp
    rtl

; @routine 7E:EEF7 MapMarkersPanelA
; WHAT IT DOES: Draws the map markers with the map origin at ($0E0A,
; $0E0D), from sprite table offset $158 downward.
; HOW: Direct page $F1 = $0E0A, $F3 high byte = $0E0D, $F5 = -4, Y = $158,
; then MapMarkerSprites. Work RAM through the data bank.
; Entry: X 16-bit, D = $0000. Exit: as MapMarkerSprites, with M 16-bit.
%ramcode($7EEEF7, $014C1E)
MapMarkersPanelA:
    rep.b #$20
    lda.w !map_a_x
    and.w #$00FF
    sta.b !digit_count
    lda.w !map_a_y_w
    and.w #$FF00
    sta.b !dp_f3
    lda.w #$FFFC
    sta.b !map_marker_step
    ldy.w #$0158
    jsl.l MapMarkerSprites
    rts

; @routine 7E:EF16 MapMarkersPanelB
; WHAT IT DOES: When $0D55 or $0D62 is set, draws the map markers with the
; map origin at ($0E09, $0E0C), from sprite table offset $54 upward when
; $0D62 is set, else from $180.
; HOW: Direct page $F1 = $0E09, $F3 high byte = $0E0C, $F5 = 4, then
; MapMarkerSprites. $0D55 and $0D62 are tested with long loads, the rest
; goes through the data bank.
; Entry: X 16-bit, D = $0000. Exit: when neither is set, A low byte = 0, M
; 8-bit, Z set, N clear; otherwise as MapMarkerSprites, with M 8-bit.
%ramcode($7EEF16, $014C3D)
MapMarkersPanelB:
    sep.b #$20
    lda.l !panel_b_shown
    bne .lEF25
    lda.l !wram_0D62
    bne .lEF25
    rts
.lEF25:
    rep.b #$20
    lda.w !map_b_x
    and.w #$00FF
    sta.b !digit_count
    lda.w !map_b_y_w
    and.w #$FF00
    sta.b !dp_f3
    lda.w #$0004
    sta.b !map_marker_step
    sep.b #$20
    lda.w !wram_0D62
    bne .lEF48
    ldy.w #$0180
    bra .lEF4B
.lEF48:
    ldy.w #$0054
.lEF4B:
    jsl.l MapMarkerSprites
    rts
