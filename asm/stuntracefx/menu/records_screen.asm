; records_screen.asm: the course screens' records pages (state 3 of each
; course select): the best times, names and cars kept in the save, the
; Speed Trax pages of each course's times, and the menu under them.
; C: src/stuntracefx/menu/records_screen.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 02:EDD2 SpeedRecordsScreen
; WHAT IT DOES: The Speed Trax course select's state 3, the records page.
; With $0D4C set: three metasprite groups ($1A, $19, 0), the class name
; (OamClassName), the title (SpriteTextAt895E), palettes
; (OamPaletteForD75) and RecordsMenuSpeedAlt. Otherwise: six groups (the
; first $05, or $1B once $0D48 is set; then 1-4 and 0), the class name,
; the pages of times (Right / Left), palettes (OamPaletteFromTableE26A);
; while $0D48 is clear the menu (RecordsMenuSpeed), else $0E92 = $F0 and
; B or Start leaves: sound $46, state $0A, and with $0D4C set the car
; $0D89 = 4 and the selection handler $00:18DB = $8669, colour maths $A7
; (ColourMathA7Start).
; HOW: The groups' slots chain from the sizes at $04:EDCB (MetaspriteDraw
; from slot 0, origin $89, $89); 16-bit, 8-bit stores. The $0D4C test
; after B or Start always finds it clear (that path runs only with $0D4C
; clear), so its car set-up (02:EEDD) runs only as SpeedRecordsLeaveCar.
; Entry: any widths; data bank $7E or a mirror. Exit: as the last call
; leaves it.
%at($02EDD2)
SpeedRecordsScreen:
    sep.b #$30
    lda.w !digit62_override
    beq .normal
    lda.b #$89
    sta.w !metasprite_origin_x
    lda.b #$89
    sta.w !metasprite_origin_y
    rep.b #$30
    ldy.w #$0000
    ldx.w #$001A
    stz.w !wram_0D6F+2
    jsr.w MetaspriteDraw
    lda.l !metasprite_groups+106
    tay
    sta.w !wram_0D6F+4
    ldx.w #$0019
    jsr.w MetaspriteDraw
    lda.l !metasprite_groups+102
    clc
    adc.w !wram_0D6F+4
    tay
    sta.w !wram_0D75
    ldx.w #$0000
    jsr.w MetaspriteDraw
    lda.w !wram_0D75
    sta.w !wram_0D7B
    jsr.w OamClassName
    jsr.w SpriteTextAt895E
    jsl.l OamPaletteForD75
    jmp.w RecordsMenuSpeedAlt
.normal:
    lda.b #$89
    sta.w !metasprite_origin_x
    lda.b #$89
    sta.w !metasprite_origin_y
    rep.b #$30
    lda.w !wram_0D48
    and.w #$00FF
    bne .after
    ldy.w #$0000
    ldx.w #$0005
    stz.w !wram_0D6F+2
    jsr.w MetaspriteDraw
    bra .groups
.after:
    ldy.w #$0000
    ldx.w #$001B
    stz.w !wram_0D6F+2
    jsr.w MetaspriteDraw
.groups:
    lda.l !metasprite_groups+22
    tay
    sta.w !wram_0D6F+4
    ldx.w #$0001
    jsr.w MetaspriteDraw
    lda.l !metasprite_groups+6
    clc
    adc.w !wram_0D6F+4
    tay
    sta.w !wram_0D75
    ldx.w #$0002
    jsr.w MetaspriteDraw
    lda.l !metasprite_groups+10
    clc
    adc.w !wram_0D75
    tay
    sta.w !wram_0D75+2
    ldx.w #$0003
    jsr.w MetaspriteDraw
    lda.l !metasprite_groups+14
    clc
    adc.w !wram_0D75+2
    tay
    sta.w !wram_0D79
    ldx.w #$0004
    jsr.w MetaspriteDraw
    lda.l !metasprite_groups+18
    clc
    adc.w !wram_0D79
    tay
    sta.w !wram_0D7B
    ldx.w #$0000
    jsr.w MetaspriteDraw
    jsr.w OamClassName
    jsr.w SpeedRecordsPage
    jsl.l OamPaletteFromTableE26A
    sep.b #$30
    lda.w !wram_0D48
    bne .shown
    jmp.w RecordsMenuSpeed
.shown:
    lda.b #$F0
    sta.w !wram_0E92
    rep.b #$20
    lda.w !joy1_pressed_copy
    bit.w #$9000
    beq .done
    sep.b #$30
    lda.b #$46
    jsl.l SoundPort0QueuePush
    lda.b #$0A
    sta.w !wram_0D3F+1
    lda.w !digit62_override
    beq .colours

; @routine 02:EEDD SpeedRecordsLeaveCar
; WHAT IT DOES: SpeedRecordsScreen's leaving with $0D4C set: the car $0D89
; = 4, the selection handler $00:18DB = $8669, colour maths $A7
; (ColourMathA7Start).
; HOW: 8-bit and 16-bit stores. SpeedRecordsScreen reaches it only with
; $0D4C clear (nothing on that path writes $0D4C), so no code runs these
; bytes; a direct call is their only way in.
; Entry: M 8-bit; data bank $7E or a mirror. Exit: as ColourMathA7Start
; leaves it.
%at($02EEDD)
    lda.b #$04
    sta.w !wram_0D89
    rep.b #$20
    lda.w #$8669
    sta.l !selection_handler_a
    sep.b #$20
.colours:
    jsl.l ColourMathA7Start
.done:
    rts

; @routine 02:EEF2 SpeedRecordsPage
; WHAT IT DOES: The Speed Trax records pages: Right and Left step the page
; $10D8 (0-4, sound $45 each step); page 0 draws the best three times ($F7
; = $24, RecordRowsFirstTimes), pages 1-4 the first times of each group of
; the course's laps ($F7 = 0, 3, 6, 9, RecordRowsGroupFirsts) with the
; page mark turned (RecordsPageMarkFlip, on page 4
; RecordsPageMarkFlipHide); then the page's title text (SpriteTextAt895E)
; with the course byte $0E3D set for it ($1C on page 0, else $0D4A * 4 +
; page - 1) and put back.
; HOW: JMP (abs,X) by page; 8-bit, the pad's word 16-bit.
; Entry: any widths, X 16-bit; data bank $7E or a mirror. Exit: M 8-bit, X
; 8-bit; A low byte = the course byte, N and Z from it; the rest as
; SpriteTextAt895E leaves it.
%at($02EEF2)
SpeedRecordsPage:
    rep.b #$20
    lda.w !joy1_pressed_copy
    bit.w #$0100
    bne .right
    lda.w !joy1_pressed_copy
    bit.w #$0200
    beq .page
    sep.b #$20
    lda.w !wram_10D8
    beq .page
    lda.b #$45
    jsl.l SoundPort0QueuePush
    dec.w !wram_10D8
    bra .page
.right:
    sep.b #$20
    lda.w !wram_10D8
    cmp.b #$04
    bcs .page
    lda.b #$45
    jsl.l SoundPort0QueuePush
    inc.w !wram_10D8
.page:
    sep.b #$30
    lda.w !wram_10D8
    cmp.b #$05
    bcc .valid
    lda.b #$00
    sta.w !wram_10D8
.valid:
    asl a
    tax
    jmp.w (.page_table,x)
.page_table:
    dw .best, .group1, .group2, .group3, .group4
.best:
    lda.b #$24
    sta.b !record_page
    jsr.w RecordRowsFirstTimes
    bra .title
.group1:
    jsr.w RecordsPageMarkFlip
    lda.b #$00
    sta.b !record_page
    jsr.w RecordRowsGroupFirsts
    bra .title
.group2:
    jsr.w RecordsPageMarkFlip
    lda.b #$03
    sta.b !record_page
    jsr.w RecordRowsGroupFirsts
    bra .title
.group3:
    jsr.w RecordsPageMarkFlip
    lda.b #$06
    sta.b !record_page
    jsr.w RecordRowsGroupFirsts
    bra .title
.group4:
    jsr.w RecordsPageMarkFlipHide
    lda.b #$09
    sta.b !record_page
    jsr.w RecordRowsGroupFirsts
.title:
    sep.b #$20
    lda.w !course
    pha
    lda.w !wram_10D8
    beq .course_best
    dec a
    sta.b !digit_count
    lda.w !wram_0D4A
    asl a
    asl a
    clc
    adc.b !digit_count
    bra .course_set
.course_best:
    lda.b #$1C
.course_set:
    sta.w !course
    jsr.w SpriteTextAt895E
    sep.b #$20
    pla
    sta.w !course
    rts

; @routine 02:F0FA RecordsPageMarkFlipHide
; WHAT IT DOES: RecordsPageMarkFlip with the group's sprites 1, 3 and 5
; hidden (Y = $F0) first: the Speed Trax records' last page.
; HOW: Shares RecordsPageMarkFlip's tail.
; Entry: any widths; data bank $7E or a mirror. Exit: as
; RecordsPageMarkFlip.
%at($02F0FA)
RecordsPageMarkFlipHide:
    php
    rep.b #$30
    lda.w !wram_0D6F+4
    and.w #$00FF
    asl a
    asl a
    tay
    sep.b #$20
    lda.b #$F0
    sta.w !oam_buffer+5,y
    sta.w !oam_buffer+13,y
    sta.w !oam_buffer+21,y
    bra RecordsPageMarkFlipTiles

; @routine 02:F115 RecordsPageMarkFlip
; WHAT IT DOES: Turns the page mark (the metasprite group from slot $0D73)
; for the Speed Trax records' middle pages: its sprites 0, 2 and 4 get
; tile $F0 and are flipped across.
; HOW: Byte stores through the data bank, the attributes read back and
; ORed with $40; P kept (PHP, PLP).
; Entry: any widths; data bank $7E or a mirror. Exit: P as on entry; A
; low byte the last attribute; Y = the group's sprite buffer offset.
%at($02F115)
RecordsPageMarkFlip:
    php
    rep.b #$30
    lda.w !wram_0D6F+4
    and.w #$00FF
    asl a
    asl a
    tay
    sep.b #$20
RecordsPageMarkFlipTiles:           ; shared with RecordsPageMarkFlipHide
    lda.b #$F0
    sta.w !oam_buffer+2,y
    sta.w !oam_buffer+10,y
    sta.w !oam_buffer+18,y
    lda.w !oam_buffer+3,y
    ora.b #$40
    sta.w !oam_buffer+3,y
    lda.w !oam_buffer+11,y
    ora.b #$40
    sta.w !oam_buffer+11,y
    lda.w !oam_buffer+19,y
    ora.b #$40
    sta.w !oam_buffer+19,y
    plp
    rts

; @routine 02:F148 StuntRecordsScreen
; WHAT IT DOES: The Stunt Trax course select's state 3, the records page.
; With $0D4E set: Right and Left step the course $0E3D through $17-$1A
; (sound $45), three metasprite groups ($15, $13, $14), the course's best
; time, name and car from the save ($70:FC4A), the title
; (SpriteTextAt8152) and the two-choice menu (RecordsMenuTwo). Otherwise:
; five groups ($12, 7, 8, 9, 6), the title, the three special scores
; ($0F2B-$0F2F) with their names and cars, palettes
; (OamPaletteFromTableE30F) and the three-choice menu (RecordsMenuThree).
; HOW: As SpeedRecordsScreen (origin $81, $89).
; Entry: any widths; data bank $7E or a mirror. Exit: as the menu leaves
; it.
%at($02F148)
StuntRecordsScreen:
    sep.b #$30
    lda.w !hud_blank
    bne .course
    brl .scores
.course:
    sep.b #$20
    lda.b #$81
    sta.w !metasprite_origin_x
    lda.b #$89
    sta.w !metasprite_origin_y
    rep.b #$30
    lda.w !joy1_pressed_copy
    bit.w #$0100
    beq .left
    lda.w #$0045
    jsl.l SoundPort0QueuePush
    lda.w !course
    inc a
    cmp.w #$001B
    bcc .set
    lda.w #$0017
    bra .set
.left:
    lda.w !joy1_pressed_copy
    bit.w #$0200
    beq .draw
    lda.w #$0045
    jsl.l SoundPort0QueuePush
    lda.w !course
    dec a
    cmp.w #$0017
    bcs .set
    lda.w #$001A
.set:
    sta.w !course
.draw:
    ldy.w #$0000
    ldx.w #$0015
    stz.w !wram_0D6F+2
    jsr.w MetaspriteDraw
    lda.l !metasprite_groups+86
    tay
    sta.w !wram_0D6F+4
    ldx.w #$0013
    jsr.w MetaspriteDraw
    rep.b #$20
    lda.l !metasprite_groups+78
    clc
    adc.w !wram_0D6F+4
    tay
    sta.w !wram_0D75
    ldx.w #$0014
    jsr.w MetaspriteDraw
    lda.w !wram_0D75
    asl a
    asl a
    tay
    lda.w !course
    sec
    sbc.w #$0017
    sta.b !digit_count
    asl a
    asl a
    clc
    adc.b !digit_count
    and.w #$00FF
    tax
    lda.l !save_record_a,x
    cmp.w #$FFFF
    beq .no_time
    phx
    lda.l !save_record_a,x
    jsr.w RecordTimeDigitsLarge
    plx
    lda.l !save_record_a+2,x
    jsr.w RecordNameLettersLarge
    lda.w !wram_0D75
    inc a
    asl a
    asl a
    tay
    lda.l !save_record_a+4,x
    jsr.w SpritePiecesCarAt51
    bra .title
.no_time:
    lda.w !wram_0D75
    inc a
    asl a
    asl a
    tay
    sep.b #$20
    lda.b #$0F
    sta.w !oam_buffer+1,y
    sta.w !oam_buffer+5,y
    sta.w !oam_buffer+9,y
.title:
    jsr.w SpriteTextAt8152
    jmp.w RecordsMenuTwo
.scores:
    lda.b #$81
    sta.w !metasprite_origin_x
    lda.b #$89
    sta.w !metasprite_origin_y
    rep.b #$30
    ldy.w #$0000
    ldx.w #$0012
    stz.w !wram_0D6F+2
    jsr.w MetaspriteDraw
    lda.l !metasprite_groups+42
    tay
    sta.w !wram_0D6F+4
    ldx.w #$0007
    jsr.w MetaspriteDraw
    lda.l !metasprite_groups+30
    clc
    adc.w !wram_0D6F+4
    tay
    sta.w !wram_0D75
    ldx.w #$0008
    jsr.w MetaspriteDraw
    lda.l !metasprite_groups+34
    clc
    adc.w !wram_0D75
    tay
    sta.w !wram_0D75+2
    ldx.w #$0009
    jsr.w MetaspriteDraw
    lda.l !metasprite_groups+38
    clc
    adc.w !wram_0D75+2
    tay
    sta.w !wram_0D79
    ldx.w #$0006
    jsr.w MetaspriteDraw
    jsr.w SpriteTextAt8152
    rep.b #$30
    lda.w !wram_0D75+2
    asl a
    asl a
    tay
    lda.l !wram_0F2B+4
    beq .no_score3
    phy
    jsr.w RecordScoreDigits
    lda.l !special_words+4
    jsr.w RecordNameLettersSmall
    lda.l !special_tags+4
    ply
    jsr.w RecordTagCarSet
    jsr.w SpritePiecesAt46Y93
    bra .row2
.no_score3:
    jsr.w OamHideEntries0To2
    jsr.w OamHideEntries3And9
.row2:
    lda.w !wram_0D75
    asl a
    asl a
    tay
    lda.l !wram_0F2B+2
    beq .no_score2
    phy
    jsr.w RecordScoreDigits
    lda.l !special_words+2
    jsr.w RecordNameLettersSmall
    lda.l !special_tags+2
    ply
    jsr.w RecordTagCarSet
    jsr.w SpritePiecesAt46Y82
    bra .row1
.no_score2:
    jsr.w OamHideEntries0To2
    jsr.w OamHideEntries3And9
.row1:
    lda.w !wram_0D6F+4
    asl a
    asl a
    tay
    lda.l !wram_0F2B
    beq .no_score1
    phy
    jsr.w RecordScoreDigits
    lda.l !special_words
    jsr.w RecordNameLettersSmall
    lda.l !special_tags
    ply
    jsr.w RecordTagCarSet
    jsr.w SpritePiecesAt46Y71
    bra .palettes
.no_score1:
    jsr.w OamHideEntries0To2
    jsr.w OamHideEntries3And9
.palettes:
    jsl.l OamPaletteFromTableE30F
    jmp.w RecordsMenuThree

; @routine 02:F30A BattleRecordsScreen
; WHAT IT DOES: The Battle Trax course select's state 3: three metasprite
; groups ($0D, $0C, $0B), the two players' cars (car * 3 as piece sets,
; SpritePiecesAtLeft from the second group, SpritePiecesAtRight three
; sprites on), the title (SpriteTextAt8152B), palettes (OamPalettesE28D)
; and the two-choice menu (RecordsMenuTwo).
; HOW: As SpeedRecordsScreen (origin $81, $89).
; Entry: any widths; data bank $7E or a mirror. Exit: as the menu leaves
; it.
%at($02F30A)
BattleRecordsScreen:
    sep.b #$30
    lda.b #$81
    sta.w !metasprite_origin_x
    lda.b #$89
    sta.w !metasprite_origin_y
    rep.b #$30
    ldy.w #$0000
    ldx.w #$000D
    stz.w !wram_0D6F+2
    jsr.w MetaspriteDraw
    lda.l !metasprite_groups+54
    tay
    sta.w !wram_0D6F+4
    ldx.w #$000C
    jsr.w MetaspriteDraw
    lda.l !metasprite_groups+50
    clc
    adc.w !wram_0D6F+4
    tay
    sta.w !wram_0D75
    ldx.w #$000B
    jsr.w MetaspriteDraw
    lda.w !wram_0D6F+4
    asl a
    asl a
    tay
    lda.w !wram_0D89
    asl a
    clc
    adc.w !wram_0D89
    and.w #$00FF
    jsr.w SpritePiecesAtLeft
    lda.w !wram_0D6F+4
    clc
    adc.w #$0003
    asl a
    asl a
    tay
    lda.w !wram_0D8A
    asl a
    clc
    adc.w !wram_0D8A
    and.w #$00FF
    jsr.w SpritePiecesAtRight
    jsr.w SpriteTextAt8152B
    jsl.l OamPalettesE28D
    jmp.w RecordsMenuTwo

; @routine 02:F37A AllRecordsScreen
; WHAT IT DOES: The all-courses select's state 3. With $0D4C set: three
; metasprite groups ($18, $16, $17), the course's best time, name and car
; from the save ($70:FC5E, course - $14), the title (SpriteTextAt8952) and
; the two-choice menu (RecordsMenuTwo). Otherwise: five groups ($12, $0F,
; $10, $11, $0E), the title, palettes (OamPaletteFromTableE26AByMode), the
; three special times ($0F2B-$0F2F) with names and cars, then its own
; three-choice menu (table $02:F519: RecordsMenuAllTop,
; RecordsMenuAllBottom, RecordsMenuAllMiddle) after the countdown $0D41, Y
; going back (RecordsMenuBack).
; HOW: As SpeedRecordsScreen (origin $89, $89).
; Entry: any widths; data bank $7E or a mirror. Exit: as the last call
; leaves it.
%at($02F37A)
AllRecordsScreen:
    sep.b #$30
    lda.w !digit62_override
    bne .course
    brl .times
.course:
    lda.b #$89
    sta.w !metasprite_origin_x
    lda.b #$89
    sta.w !metasprite_origin_y
    rep.b #$30
    ldy.w #$0000
    ldx.w #$0018
    stz.w !wram_0D6F+2
    jsr.w MetaspriteDraw
    lda.l !metasprite_groups+98
    tay
    sta.w !wram_0D6F+4
    ldx.w #$0016
    jsr.w MetaspriteDraw
    lda.l !metasprite_groups+90
    clc
    adc.w !wram_0D6F+4
    tay
    sta.w !wram_0D75
    ldx.w #$0017
    jsr.w MetaspriteDraw
    lda.w !wram_0D75
    dec a
    asl a
    asl a
    tay
    lda.w !course
    sec
    sbc.w #$0014
    sta.b !digit_count
    asl a
    asl a
    clc
    adc.b !digit_count
    and.w #$00FF
    tax
    lda.l !save_record_b,x
    cmp.w #$FFFF
    beq .no_time
    phx
    lda.l !save_record_b,x
    jsr.w RecordTimeDigitsLarge
    plx
    lda.l !save_record_b+2,x
    jsr.w RecordNameLettersLarge
    lda.w !wram_0D75
    asl a
    asl a
    tay
    lda.l !save_record_b+4,x
    jsr.w SpritePiecesCarAt51
    bra .title
.no_time:
    lda.w !wram_0D75
    asl a
    asl a
    tay
    sep.b #$20
    lda.b #$0F
    sta.w !oam_buffer+1,y
    sta.w !oam_buffer+5,y
    sta.w !oam_buffer+9,y
.title:
    jsr.w SpriteTextAt8952
    jmp.w RecordsMenuTwo
.times:
    lda.b #$89
    sta.w !metasprite_origin_x
    lda.b #$89
    sta.w !metasprite_origin_y
    rep.b #$30
    ldy.w #$0000
    ldx.w #$0012
    stz.w !wram_0D6F+2
    jsr.w MetaspriteDraw
    lda.l !metasprite_groups+58
    tay
    sta.w !wram_0D6F+4
    ldx.w #$000F
    jsr.w MetaspriteDraw
    lda.l !metasprite_groups+62
    clc
    adc.w !wram_0D6F+4
    tay
    sta.w !wram_0D75
    ldx.w #$0010
    jsr.w MetaspriteDraw
    lda.l !metasprite_groups+66
    clc
    adc.w !wram_0D75
    tay
    sta.w !wram_0D75+2
    ldx.w #$0011
    jsr.w MetaspriteDraw
    lda.l !metasprite_groups+70
    clc
    adc.w !wram_0D75+2
    tay
    sta.w !wram_0D79
    ldx.w #$000E
    jsr.w MetaspriteDraw
    jsr.w SpriteTextAt8952
    jsl.l OamPaletteFromTableE26AByMode
    rep.b #$30
    lda.w !wram_0D75+2
    asl a
    asl a
    tay
    lda.l !wram_0F2B+4
    cmp.w #$FFFF
    beq .no_time3
    phy
    jsr.w RecordTimeDigitsSmall
    lda.l !special_words+4
    jsr.w RecordNameLetters
    lda.l !special_tags+4
    ply
    jsr.w RecordTagCarSet
    jsr.w SpritePiecesAt62Y95
    bra .row2
.no_time3:
    jsr.w OamHideEntries0To2
.row2:
    lda.w !wram_0D75
    asl a
    asl a
    tay
    lda.l !wram_0F2B+2
    cmp.w #$FFFF
    beq .no_time2
    phy
    jsr.w RecordTimeDigitsSmall
    lda.l !special_words+2
    jsr.w RecordNameLetters
    lda.l !special_tags+2
    ply
    jsr.w RecordTagCarSet
    jsr.w SpritePiecesAt62Y83
    bra .row1
.no_time2:
    jsr.w OamHideEntries0To2
.row1:
    lda.w !wram_0D6F+4
    asl a
    asl a
    tay
    lda.l !wram_0F2B
    cmp.w #$FFFF
    beq .no_time1
    phy
    jsr.w RecordTimeDigitsSmall
    lda.l !special_words
    jsr.w RecordNameLetters
    lda.l !special_tags
    ply
    jsr.w RecordTagCarSet
    jsr.w SpritePiecesAt62Y71
    bra .menu
.no_time1:
    jsr.w OamHideEntries0To2
.menu:
    sep.b #$20
    lda.w !wram_0D3F+2
    beq .read
    dec.w !wram_0D3F+2
    rts
.read:
    sep.b #$10
    rep.b #$20
    lda.w !joy1_pressed_copy
    bit.w #$4000
    beq .choice
    brl RecordsMenuBack
.choice:
    ldx.w !hud_blank+2
    jmp.w (.menu_table,x)
.menu_table:
    dw RecordsMenuAllTop, RecordsMenuAllBottom, RecordsMenuAllMiddle

; @routine 02:F51F RecordsMenuAllTop
; WHAT IT DOES: The all-courses records menu with the cursor on its first
; choice: B or Start takes it (RecordsMenuGo); Down moves to the middle
; choice (sound $45); otherwise the first choice stays highlighted.
; HOW: 16-bit tests of the pad's new presses ($9000, $0400).
; Entry: M 16-bit; data bank $7E or a mirror. Exit: as RecordsMenuGo, or
; as OamSprite0AddY leaves it (M 8-bit, X 16-bit).
%at($02F51F)
RecordsMenuAllTop:
    lda.w !joy1_pressed_copy
    bit.w #$9000
    beq .down
    jmp.w RecordsMenuGo
.down:
    lda.w !joy1_pressed_copy
    bit.w #$0400
    beq RecordsMenuAllTopShow
    lda.w #$0045
    jsl.l SoundPort0QueuePush
    bra RecordsMenuAllMiddleShow
RecordsMenuAllTopShow:              ; also RecordsMenuAllMiddle's on Up
    sep.b #$20
    lda.b #$00
    sta.w !hud_blank+2
    rep.b #$30
    lda.w #$0001
    jsr.w OamAttr36x4
    lda.w #$0005
    jsr.w OamAttr38x7
    lda.w #$000C
    jsr.w OamAttr38x6
    sep.b #$20
    lda.b #$00
    jsr.w OamSprite0AddY
    rts

; @routine 02:F55E RecordsMenuAllMiddle
; WHAT IT DOES: The all-courses records menu with the cursor on its middle
; choice: B or Start, unless every course is opened ($0D24 = 3), takes it
; (state $0C, sound $24); Up and Down move the cursor (sound $45);
; otherwise the middle choice stays highlighted.
; HOW: An 8-bit compare, then 16-bit pad tests.
; Entry: M 16-bit; data bank $7E or a mirror. Exit: as the push of sound
; $24 leaves it, or as OamSprite0AddY.
%at($02F55E)
RecordsMenuAllMiddle:
    sep.b #$20
    lda.w !wram_0D24
    cmp.b #$03
    beq .move
    rep.b #$20
    lda.w !joy1_pressed_copy
    bit.w #$9000
    beq .move
    lda.w #$000C
    sta.w !wram_0D3F+1
    lda.w #$0024
    jsl.l SoundPort0QueuePush
    rts
.move:
    rep.b #$20
    lda.w !joy1_pressed_copy
    bit.w #$0800
    beq .down
    lda.w #$0045
    jsl.l SoundPort0QueuePush
    bra RecordsMenuAllTopShow
.down:
    lda.w !joy1_pressed_copy
    bit.w #$0400
    beq RecordsMenuAllMiddleShow
    lda.w #$0045
    jsl.l SoundPort0QueuePush
    bra RecordsMenuAllBottomShow
RecordsMenuAllMiddleShow:           ; also RecordsMenuAllTop's and Bottom's
    sep.b #$20
    lda.b #$04
    sta.w !hud_blank+2
    rep.b #$30
    lda.w #$0001
    jsr.w OamAttr38x4
    lda.w #$0005
    jsr.w OamAttr36x7
    lda.w #$000C
    jsr.w OamAttr38x6
    sep.b #$20
    lda.b #$0F
    jsr.w OamSprite0AddY
    rts

; @routine 02:F5C6 RecordsMenuAllBottom
; WHAT IT DOES: The all-courses records menu with the cursor on its last
; choice: B or Start goes back to the course select (RecordsMenuBack); Up
; moves to the middle choice (sound $45); otherwise the last choice stays
; highlighted.
; HOW: 16-bit pad tests.
; Entry: M 16-bit; data bank $7E or a mirror. Exit: as RecordsMenuBack, or
; as OamSprite0AddY.
%at($02F5C6)
RecordsMenuAllBottom:
    lda.w !joy1_pressed_copy
    bit.w #$9000
    beq .up
    jmp.w RecordsMenuBack
.up:
    lda.w !joy1_pressed_copy
    bit.w #$0800
    beq RecordsMenuAllBottomShow
    lda.w #$0045
    jsl.l SoundPort0QueuePush
    bra RecordsMenuAllMiddleShow
RecordsMenuAllBottomShow:           ; also RecordsMenuAllMiddle's on Down
    sep.b #$20
    lda.b #$02
    sta.w !hud_blank+2
    rep.b #$30
    lda.w #$0001
    jsr.w OamAttr38x4
    lda.w #$0005
    jsr.w OamAttr38x7
    lda.w #$000C
    jsr.w OamAttr36x6
    sep.b #$20
    lda.b #$1E
    jsr.w OamSprite0AddY
    rts

; @routine 02:F605 RecordsMenuSpeed
; WHAT IT DOES: The Speed Trax records menu: after the countdown $0D41, Y
; goes back (RecordsMenuBack), otherwise the cursor's handler from the
; table at $02:F625 (RecordsMenuSpeedTop, RecordsMenuSpeedBottom).
; HOW: JMP (abs,X) with X 8-bit.
; Entry: any widths; data bank $7E or a mirror. Exit: as the handler
; leaves it.
%at($02F605)
RecordsMenuSpeed:
    sep.b #$20
    lda.w !wram_0D3F+2
    beq .read
    dec.w !wram_0D3F+2
    rts
.read:
    sep.b #$10
    rep.b #$20
    lda.w !joy1_pressed_copy
    bit.w #$4000
    beq .choice
    brl RecordsMenuBack
.choice:
    ldx.w !hud_blank+2
    jmp.w (.table,x)
.table:
    dw RecordsMenuSpeedTop, RecordsMenuSpeedBottom

; @routine 02:F629 RecordsMenuSpeedTop
; WHAT IT DOES: The Speed Trax records menu with the cursor on its first
; choice: B or Start takes it (RecordsMenuGo); Down moves to the second
; (sound $45); otherwise the first stays highlighted.
; HOW: 16-bit pad tests.
; Entry: M 16-bit; data bank $7E or a mirror. Exit: as RecordsMenuGo, or
; as OamSprite0AddY.
%at($02F629)
RecordsMenuSpeedTop:
    lda.w !joy1_pressed_copy
    bit.w #$9000
    beq .down
    jmp.w RecordsMenuGo
.down:
    lda.w !joy1_pressed_copy
    bit.w #$0400
    beq RecordsMenuSpeedTopShow
    lda.w #$0045
    jsl.l SoundPort0QueuePush
    bra RecordsMenuSpeedBottomShow
RecordsMenuSpeedTopShow:            ; also RecordsMenuSpeedBottom's on Up
    sep.b #$20
    lda.b #$00
    sta.w !hud_blank+2
    rep.b #$30
    lda.w #$0001
    jsr.w OamAttr36x4
    lda.w #$0005
    jsr.w OamAttr38x6
    sep.b #$20
    lda.b #$00
    jsr.w OamSprite0AddY
    rts

; @routine 02:F662 RecordsMenuSpeedBottom
; WHAT IT DOES: The Speed Trax records menu with the cursor on its second
; choice: B or Start goes back (RecordsMenuBack); Up moves to the first
; (sound $45); otherwise the second stays highlighted.
; HOW: 16-bit pad tests.
; Entry: M 16-bit; data bank $7E or a mirror. Exit: as RecordsMenuBack, or
; as OamSprite0AddY.
%at($02F662)
RecordsMenuSpeedBottom:
    lda.w !joy1_pressed_copy
    bit.w #$9000
    beq .up
    jmp.w RecordsMenuBack
.up:
    lda.w !joy1_pressed_copy
    bit.w #$0800
    beq RecordsMenuSpeedBottomShow
    lda.w #$0045
    jsl.l SoundPort0QueuePush
    bra RecordsMenuSpeedTopShow
RecordsMenuSpeedBottomShow:         ; also RecordsMenuSpeedTop's on Down
    sep.b #$20
    lda.b #$02
    sta.w !hud_blank+2
    rep.b #$30
    lda.w #$0001
    jsr.w OamAttr38x4
    lda.w #$0005
    jsr.w OamAttr36x6
    sep.b #$20
    lda.b #$0F
    jsr.w OamSprite0AddY
    rts

; @routine 02:F69B RecordsMenuSpeedAlt
; WHAT IT DOES: The Speed Trax records menu when $0D4C is set: after the
; countdown $0D41, the cursor's handler from the table at $02:F6AE
; (RecordsMenuSpeedAltTop, RecordsMenuSpeedAltBottom); Y is the
; handlers' to read.
; HOW: JMP (abs,X) with X 8-bit, M left 8-bit.
; Entry: any widths; data bank $7E or a mirror. Exit: as the handler
; leaves it.
%at($02F69B)
RecordsMenuSpeedAlt:
    sep.b #$20
    lda.w !wram_0D3F+2
    beq .read
    dec.w !wram_0D3F+2
    rts
.read:
    sep.b #$10
    ldx.w !hud_blank+2
    jmp.w (.table,x)
.table:
    dw RecordsMenuSpeedAltTop, RecordsMenuSpeedAltBottom

; @routine 02:F6B2 RecordsMenuSpeedAltTop
; WHAT IT DOES: The Speed Trax records menu with $0D4C set, cursor on its
; first choice: Y leaves ($0D4C cleared, sound $2F, state $0E, $0D48
; counted up); B or Start takes the choice (RecordsMenuGo); Down moves to
; the second (sound $45); otherwise the first stays highlighted.
; HOW: 16-bit pad tests.
; Entry: M 8-bit; data bank $7E or a mirror. Exit: as the path's last
; call leaves it.
%at($02F6B2)
RecordsMenuSpeedAltTop:
    rep.b #$20
    lda.w !joy1_pressed_copy
    bit.w #$4000
    beq .go
    brl RecordsMenuSpeedAltLeave
.go:
    lda.w !joy1_pressed_copy
    bit.w #$9000
    beq .down
    jmp.w RecordsMenuGo
.down:
    lda.w !joy1_pressed_copy
    bit.w #$0400
    beq RecordsMenuSpeedAltTopShow
    lda.w #$0045
    jsl.l SoundPort0QueuePush
    bra RecordsMenuSpeedAltBottomShow
RecordsMenuSpeedAltTopShow:         ; also RecordsMenuSpeedAltBottom's on Up
    sep.b #$20
    lda.b #$00
    sta.w !hud_blank+2
    rep.b #$30
    lda.w #$0001
    jsr.w OamAttr36x4
    lda.w #$0005
    jsr.w OamAttr38x4
    sep.b #$20
    lda.b #$00
    jsr.w OamSprite0AddY
    rts

; @routine 02:F6F8 RecordsMenuSpeedAltBottom
; WHAT IT DOES: As RecordsMenuSpeedAltTop with the cursor on the second
; choice: Y, B or Start leave ($0D4C cleared, sound $2F, state $0E); Up
; moves to the first (sound $45); otherwise the second stays highlighted.
; HOW: 16-bit pad tests.
; Entry: M 8-bit; data bank $7E or a mirror. Exit: as the path's last
; call leaves it.
%at($02F6F8)
RecordsMenuSpeedAltBottom:
    rep.b #$20
    lda.w !joy1_pressed_copy
    bit.w #$4000
    beq .back
    brl RecordsMenuSpeedAltLeave
.back:
    lda.w !joy1_pressed_copy
    bit.w #$9000
    beq RecordsMenuSpeedAltBottomUp
RecordsMenuSpeedAltLeave:           ; also RecordsMenuSpeedAltTop's on Y
    sep.b #$20
    stz.w !digit62_override
    rep.b #$30
    lda.w #$002F
    jsl.l SoundPort0QueuePush
    sep.b #$20
    lda.b #$0E
    sta.w !wram_0D3F+1
    inc.w !wram_0D48
    jsl.l OamBufferHideLargeLong
    lda.b #$F0
    sta.l !wram_0E92
    rts
RecordsMenuSpeedAltBottomUp:
    lda.w !joy1_pressed_copy
    bit.w #$0800
    beq RecordsMenuSpeedAltBottomShow
    lda.w #$0045
    jsl.l SoundPort0QueuePush
    bra RecordsMenuSpeedAltTopShow
RecordsMenuSpeedAltBottomShow:      ; also RecordsMenuSpeedAltTop's on Down
    sep.b #$20
    lda.b #$02
    sta.w !hud_blank+2
    rep.b #$30
    lda.w #$0001
    jsr.w OamAttr38x4
    lda.w #$0005
    jsr.w OamAttr36x4
    sep.b #$20
    lda.b #$0F
    jsr.w OamSprite0AddY
    rts

; @routine 02:F75E RecordsMenuTwo
; WHAT IT DOES: The two-choice records menu: after the countdown $0D41, Y
; goes back (RecordsMenuBack), otherwise the cursor's handler from the
; table at $02:F77E (RecordsMenuTwoTop, RecordsMenuTwoBottom).
; HOW: As RecordsMenuSpeed.
; Entry: any widths; data bank $7E or a mirror. Exit: as the handler
; leaves it.
%at($02F75E)
RecordsMenuTwo:
    sep.b #$20
    lda.w !wram_0D3F+2
    beq .read
    dec.w !wram_0D3F+2
    rts
.read:
    sep.b #$10
    rep.b #$20
    lda.w !joy1_pressed_copy
    bit.w #$4000
    beq .choice
    brl RecordsMenuBack
.choice:
    ldx.w !hud_blank+2
    jmp.w (.table,x)
.table:
    dw RecordsMenuTwoTop, RecordsMenuTwoBottom

; @routine 02:F782 RecordsMenuTwoTop
; WHAT IT DOES: The two-choice records menu (Stunt Trax special, Battle
; Trax, all-courses special) with the cursor on its first choice: B or
; Start takes it (RecordsMenuGo); Down moves to the second (sound $45);
; otherwise the first stays highlighted.
; HOW: 16-bit pad tests.
; Entry: M 16-bit; data bank $7E or a mirror. Exit: as RecordsMenuGo, or
; as OamSprite0AddY.
%at($02F782)
RecordsMenuTwoTop:
    lda.w !joy1_pressed_copy
    bit.w #$9000
    beq .down
    brl RecordsMenuGo
.down:
    lda.w !joy1_pressed_copy
    bit.w #$0400
    beq RecordsMenuTwoTopShow
    lda.w #$0045
    jsl.l SoundPort0QueuePush
    bra RecordsMenuTwoBottomShow
RecordsMenuTwoTopShow:              ; also RecordsMenuTwoBottom's on Up
    sep.b #$20
    lda.b #$00
    sta.w !hud_blank+2
    rep.b #$30
    lda.w #$0001
    jsr.w OamAttr36x4
    lda.w #$0005
    jsr.w OamAttr38x6
    sep.b #$20
    lda.b #$00
    jsr.w OamSprite0AddY
    rts

; @routine 02:F7BB RecordsMenuTwoBottom
; WHAT IT DOES: The two-choice records menu with the cursor on its second
; choice: B or Start goes back (RecordsMenuBack); Up moves to the first
; (sound $45); otherwise the second stays highlighted.
; HOW: 16-bit pad tests.
; Entry: M 16-bit; data bank $7E or a mirror. Exit: as RecordsMenuBack, or
; as OamSprite0AddY.
%at($02F7BB)
RecordsMenuTwoBottom:
    lda.w !joy1_pressed_copy
    bit.w #$9000
    beq .up
    brl RecordsMenuBack
.up:
    lda.w !joy1_pressed_copy
    bit.w #$0800
    beq RecordsMenuTwoBottomShow
    lda.w #$0045
    jsl.l SoundPort0QueuePush
    bra RecordsMenuTwoTopShow
RecordsMenuTwoBottomShow:           ; also RecordsMenuTwoTop's on Down
    sep.b #$20
    lda.b #$02
    sta.w !hud_blank+2
    rep.b #$30
    lda.w #$0001
    jsr.w OamAttr38x4
    lda.w #$0005
    jsr.w OamAttr36x6
    sep.b #$20
    lda.b #$0F
    jsr.w OamSprite0AddY
    rts

; @routine 02:F7F4 RecordsMenuThree
; WHAT IT DOES: The Stunt Trax records menu: after the countdown $0D41, Y
; goes back (RecordsMenuBack), otherwise the cursor's handler from the
; table at $02:F814 (RecordsMenuThreeTop, RecordsMenuThreeBottom,
; RecordsMenuThreeMiddle).
; HOW: As RecordsMenuSpeed.
; Entry: any widths; data bank $7E or a mirror. Exit: as the handler
; leaves it.
%at($02F7F4)
RecordsMenuThree:
    sep.b #$20
    lda.w !wram_0D3F+2
    beq .read
    dec.w !wram_0D3F+2
    rts
.read:
    sep.b #$10
    rep.b #$20
    lda.w !joy1_pressed_copy
    bit.w #$4000
    beq .choice
    brl RecordsMenuBack
.choice:
    ldx.w !hud_blank+2
    jmp.w (.table,x)
.table:
    dw RecordsMenuThreeTop, RecordsMenuThreeBottom, RecordsMenuThreeMiddle

; @routine 02:F81A RecordsMenuThreeTop
; WHAT IT DOES: The Stunt Trax records menu with the cursor on its first
; choice: B or Start takes it (RecordsMenuGo); Down moves to the middle
; choice (sound $45); otherwise the first stays highlighted.
; HOW: 16-bit pad tests.
; Entry: M 16-bit; data bank $7E or a mirror. Exit: as RecordsMenuGo, or
; as OamSprite0AddY.
%at($02F81A)
RecordsMenuThreeTop:
    lda.w !joy1_pressed_copy
    bit.w #$9000
    beq .down
    jmp.w RecordsMenuGo
.down:
    lda.w !joy1_pressed_copy
    bit.w #$0400
    beq RecordsMenuThreeTopShow
    lda.w #$0045
    jsl.l SoundPort0QueuePush
    bra RecordsMenuThreeTopDown
RecordsMenuThreeTopShow:            ; also RecordsMenuThreeMiddle's on Up
    sep.b #$20
    lda.b #$00
    sta.w !hud_blank+2
    rep.b #$30
    lda.w #$0001
    jsr.w OamAttr36x4
    lda.w #$0005
    jsr.w OamAttr38x7
    lda.w #$000C
    jsr.w OamAttr38x6
    sep.b #$20
    lda.b #$00
    jsr.w OamSprite0AddY
    rts
RecordsMenuThreeTopDown:
    bra RecordsMenuThreeMiddleShow

; @routine 02:F85B RecordsMenuThreeMiddle
; WHAT IT DOES: The Stunt Trax records menu with the cursor on its middle
; choice: B or Start takes it (state $0C, sound $24); Up and Down move the
; cursor (sound $45); otherwise the middle stays highlighted.
; HOW: 16-bit pad tests.
; Entry: M 16-bit; data bank $7E or a mirror. Exit: as the push of sound
; $24 leaves it, or as OamSprite0AddY.
%at($02F85B)
RecordsMenuThreeMiddle:
    lda.w !joy1_pressed_copy
    bit.w #$9000
    beq .up
    lda.w #$000C
    sta.w !wram_0D3F+1
    lda.w #$0024
    jsl.l SoundPort0QueuePush
    rts
.up:
    lda.w !joy1_pressed_copy
    bit.w #$0800
    beq .down
    lda.w #$0045
    jsl.l SoundPort0QueuePush
    bra RecordsMenuThreeTopShow
.down:
    lda.w !joy1_pressed_copy
    bit.w #$0400
    beq RecordsMenuThreeMiddleShow
    lda.w #$0045
    jsl.l SoundPort0QueuePush
    bra RecordsMenuThreeBottomShow
RecordsMenuThreeMiddleShow:         ; also RecordsMenuThreeTop's and Bottom's
    sep.b #$20
    lda.b #$04
    sta.w !hud_blank+2
    rep.b #$30
    lda.w #$0001
    jsr.w OamAttr38x4
    lda.w #$0005
    jsr.w OamAttr36x7
    lda.w #$000C
    jsr.w OamAttr38x6
    sep.b #$20
    lda.b #$0F
    jsr.w OamSprite0AddY
    rts

; @routine 02:F8B6 RecordsMenuThreeBottom
; WHAT IT DOES: The Stunt Trax records menu with the cursor on its last
; choice: B or Start goes back (RecordsMenuBack); Up moves to the middle
; (sound $45); otherwise the last stays highlighted.
; HOW: 16-bit pad tests.
; Entry: M 16-bit; data bank $7E or a mirror. Exit: as RecordsMenuBack, or
; as OamSprite0AddY.
%at($02F8B6)
RecordsMenuThreeBottom:
    lda.w !joy1_pressed_copy
    bit.w #$9000
    beq .up
    jmp.w RecordsMenuBack
.up:
    lda.w !joy1_pressed_copy
    bit.w #$0800
    beq RecordsMenuThreeBottomShow
    lda.w #$0045
    jsl.l SoundPort0QueuePush
    bra RecordsMenuThreeMiddleShow
RecordsMenuThreeBottomShow:         ; also RecordsMenuThreeMiddle's on Down
    sep.b #$20
    lda.b #$02
    sta.w !hud_blank+2
    rep.b #$30
    lda.w #$0001
    jsr.w OamAttr38x4
    lda.w #$0005
    jsr.w OamAttr38x7
    lda.w #$000C
    jsr.w OamAttr36x6
    sep.b #$20
    lda.b #$1E
    jsr.w OamSprite0AddY
    rts

; @routine 02:F8F5 RecordsMenuGo
; WHAT IT DOES: Takes a records page's first choice: sound $46, state $0A
; (the window step), colour maths $A7 (ColourMathA7Start) and the large
; sprites hidden (OamBufferHideLargeLong).
; HOW: 8-bit.
; Entry: any widths; data bank $7E or a mirror. Exit: M 8-bit, X 8-bit; as
; OamBufferHideLargeLong leaves the rest.
%at($02F8F5)
RecordsMenuGo:
    sep.b #$30
    lda.b #$46
    jsl.l SoundPort0QueuePush
    lda.b #$0A
    sta.w !wram_0D3F+1
    jsl.l ColourMathA7Start
    jsl.l OamBufferHideLargeLong
    rts

; @routine 02:F90B RecordsMenuBack
; WHAT IT DOES: Leaves a records page for the course select: sound $2F,
; state 8 (CourseScreenState8), the large sprites hidden
; (OamBufferHideLargeLong) and $0E92 = $F0.
; HOW: 16-bit, then an 8-bit long store to $00:0E92.
; Entry: any widths; data bank $7E or a mirror. Exit: M 16-bit, X 16-bit;
; A high byte as OamBufferHideLargeLong left it, low byte $F0; N set, Z
; clear.
%at($02F90B)
RecordsMenuBack:
    rep.b #$30
    lda.w #$002F
    jsl.l SoundPort0QueuePush
    jsr.w CourseScreenState8
    jsl.l OamBufferHideLargeLong
    sep.b #$20
    lda.b #$F0
    sta.l !wram_0E92
    rep.b #$20
    rts
