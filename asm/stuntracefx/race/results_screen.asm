; results_screen.asm: the screens after a race that OverlayFramesUntilFaded
; runs a frame at a time (ResultsScreenStep): by HUD mode the step $0D59
; picks a handler that draws a panel with the Super FX (a picture drawn to
; $70:3000 and copied into the tilemap buffer), shows the records of a
; special course (scores or times, the cars as three-piece sprites), waits
; for B or START, or runs the menu after the race (retry, course, car: $0DD4)
; and the car choice.
; C: src/stuntracefx/race/results_screen.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 07:C6AA ResultsScreenStep
; WHAT IT DOES: Runs the results screens' step $0D59 for HUD mode !hud_mode:
; mode 0 from .mode0 (16 steps), mode 1 from .mode1 (14), mode 3 from .mode3
; (17); mode 2 has none.
; HOW: Two JMP (table,X): by the mode, then by the step. Mode 3 is reached
; only by unit_results_screen.
; Entry: M 8-bit, X 8-bit. Exit: as the step's handler leaves it.
%at($07C6AA)
ResultsScreenStep:
    lda.w !hud_mode
    asl a
    tax
    jmp.w (.modes,x)
.modes:
    dw .lC6BB, .lC70D, .lC6BA, .lC6E3
.lC6BA:
    rts
.lC6BB:
    lda.w !wram_0D59
    asl a
    tax
    jmp.w (.mode0,x)
.mode0:
    dw ResultsUnpackNext, ResultsPicture4000, ResultsCarPicture, ResultsPanelFA39, $CF32, $CFE8, $D029, $CF5B
    dw $D0F5, $D107, $D114, $D121, $D232, $D268, $D12E, $D471
.lC6E3:
    lda.w !wram_0D59
    asl a
    tax
    jmp.w (.mode3,x)
.mode3:
    dw ResultsNext, ResultsPicture4000, ResultsCarPicture, ResultsPanelF38E, $CF32, $CF6F, $D029, $CF5B
    dw $D0F5, $D107, $D114, $D121, TimedRecordsChoice, ResultsPanelFB99, ResultsMenu, TimedRecordsDraw
    dw TimedRecordsWait
.lC70D:
    lda.w !wram_0D59
    asl a
    tax
    jmp.w (.mode1,x)
.mode1:
    dw ResultsNext, ResultsPicture4000, ResultsCarPicture, ResultsPanelFAB5, $CF32, $CF6F, $D029, $CF5B
    dw $D055, StuntRecordsChoice, ResultsPanelFB99, ResultsMenu, StuntRecordsDraw, StuntRecordsWait

; @routine 07:C731 StuntRecordsWait
; WHAT IT DOES: The Stunt Trax records after the race: B or START goes on to
; step $A (sound $46, the three car sprites hidden, $10A2 = 5); else the
; player's place row cycles its palette (TilemapPaletteCycle, 7 entries) and
; the cars of the three records with a score are drawn at X $40.
; HOW: The presses 16-bit, then 8-bit. Reached only by unit_results_screen.
; Entry: M 8-bit, X 8-bit; D = $0000. Exit: as the path leaves it.
%at($07C731)
StuntRecordsWait:
    rep.b #$20
    lda.w !joy1_pressed_copy
    bit.w #$9000
    beq .lC759
    sep.b #$20
    lda.b #$46
    jsl.l SoundPort0QueuePush
    lda.b #$0A
    sta.w !wram_0D59
    lda.b #$55
    sta.w !oam_buffer_high+19
    sta.w !oam_buffer_high+20
    sta.w !oam_buffer_high+21
    lda.b #$05
    sta.w !wram_10A2
    rts
.lC759:
    rep.b #$30
    lda.w !player_place
    beq .lC77A
    cmp.w #$0002
    beq .lC76C
    bcs .lC771
    ldx.w #$018F
    bra .lC774
.lC76C:
    ldx.w #$024F
    bra .lC774
.lC771:
    ldx.w #$09AF
.lC774:
    ldy.w #$0007
    jsr.w TilemapPaletteCycle
.lC77A:
    sep.b #$20
    lda.w !frame_count
    and.b #$07
    cmp.b #$07
    bne .lC792
    lda.w !sprite_piece_select
    inc a
    cmp.b #$04
    bcc .lC78F
    lda.b #$00
.lC78F:
    sta.w !sprite_piece_select
.lC792:
    lda.b #$3E
    sta.w !sprite_piece_palette
    lda.b #$40
    sta.w !sprite_piece_x
    lda.b #$F4
    sta.w !sprite_piece_tile
    rep.b #$20
    lda.w !wram_0F2B
    beq .lC7C8
    lda.w !special_tags
    jsr.w PlaceFrameFromValue
    sep.b #$20
    sta.w !sprite_piece_frame
    lda.b #$54
    sta.w !sprite_piece_y
    rep.b #$20
    ldy.w #$0130
    jsl.l SpritePieces3
    sep.b #$20
    lda.b #$60
    sta.w !oam_buffer_high+19
.lC7C8:
    rep.b #$20
    lda.w !wram_0F2B+2
    beq .lC7EC
    lda.w !special_tags+2
    jsr.w PlaceFrameFromValue
    sep.b #$20
    lda.b #$6C
    sta.w !sprite_piece_y
    rep.b #$20
    ldy.w #$0140
    jsl.l SpritePieces3
    sep.b #$20
    lda.b #$60
    sta.w !oam_buffer_high+20
.lC7EC:
    rep.b #$20
    lda.w !wram_0F2B+4
    beq .lC810
    lda.w !special_tags+4
    jsr.w PlaceFrameFromValue
    sep.b #$20
    lda.b #$84
    sta.w !sprite_piece_y
    rep.b #$20
    ldy.w #$0150
    jsl.l SpritePieces3
    sep.b #$20
    lda.b #$60
    sta.w !oam_buffer_high+21
.lC810:
    rts

; @routine 07:C811 TimedRecordsWait
; WHAT IT DOES: As StuntRecordsWait for the timed records ($FFFF for none):
; step $D, 11 palette entries, the cars at X $58.
; HOW: As StuntRecordsWait. Reached only by unit_results_screen.
; Entry: as StuntRecordsWait. Exit: as StuntRecordsWait.
%at($07C811)
TimedRecordsWait:
    rep.b #$20
    lda.w !joy1_pressed_copy
    bit.w #$9000
    beq .lC839
    sep.b #$20
    lda.b #$46
    jsl.l SoundPort0QueuePush
    lda.b #$0D
    sta.w !wram_0D59
    lda.b #$55
    sta.w !oam_buffer_high+19
    sta.w !oam_buffer_high+20
    sta.w !oam_buffer_high+21
    lda.b #$05
    sta.w !wram_10A2
    rts
.lC839:
    rep.b #$30
    lda.w !player_place
    beq .lC85A
    cmp.w #$0002
    beq .lC84C
    bcs .lC851
    ldx.w #$0195
    bra .lC854
.lC84C:
    ldx.w #$0255
    bra .lC854
.lC851:
    ldx.w #$0315
.lC854:
    ldy.w #$000B
    jsr.w TilemapPaletteCycle
.lC85A:
    sep.b #$20
    lda.w !frame_count
    and.b #$07
    cmp.b #$07
    bne .lC872
    lda.w !sprite_piece_select
    inc a
    cmp.b #$04
    bcc .lC86F
    lda.b #$00
.lC86F:
    sta.w !sprite_piece_select
.lC872:
    lda.b #$3E
    sta.w !sprite_piece_palette
    lda.b #$58
    sta.w !sprite_piece_x
    lda.b #$F4
    sta.w !sprite_piece_tile
    rep.b #$20
    lda.w !wram_0F2B
    cmp.w #$FFFF
    beq .lC8AB
    lda.w !special_tags
    jsr.w PlaceFrameFromValue
    sep.b #$20
    sta.w !sprite_piece_frame
    lda.b #$54
    sta.w !sprite_piece_y
    rep.b #$20
    ldy.w #$0130
    jsl.l SpritePieces3
    sep.b #$20
    lda.b #$60
    sta.w !oam_buffer_high+19
.lC8AB:
    rep.b #$20
    lda.w !wram_0F2B+2
    cmp.w #$FFFF
    beq .lC8D2
    lda.w !special_tags+2
    jsr.w PlaceFrameFromValue
    sep.b #$20
    lda.b #$6C
    sta.w !sprite_piece_y
    rep.b #$20
    ldy.w #$0140
    jsl.l SpritePieces3
    sep.b #$20
    lda.b #$60
    sta.w !oam_buffer_high+20
.lC8D2:
    rep.b #$20
    lda.w !wram_0F2B+4
    cmp.w #$FFFF
    beq .lC8F9
    lda.w !special_tags+4
    jsr.w PlaceFrameFromValue
    sep.b #$20
    lda.b #$84
    sta.w !sprite_piece_y
    rep.b #$20
    ldy.w #$0150
    jsl.l SpritePieces3
    sep.b #$20
    lda.b #$60
    sta.w !oam_buffer_high+21
.lC8F9:
    rts

; @routine 07:C8FA ResultsUnpackNext
; WHAT IT DOES: Unpacks the course's records (RecordsUnpackCourseRunning),
; then ResultsNext.
; HOW: JSL, then ResultsNext's code, which follows.
; Entry: as RecordsUnpackCourseRunning. Exit: as ResultsNext.
%at($07C8FA)
ResultsUnpackNext:
    jsl.l RecordsUnpackCourseRunning

; @routine 07:C8FE ResultsNext
; WHAT IT DOES: $0E5F cleared, $0D22 = 6, $0D23 cleared, the next step.
; HOW: 8-bit stores.
; Entry: any widths. Exit: M 8-bit; A low byte = 0.
%at($07C8FE)
ResultsNext:
    sep.b #$20
    stz.w !palette_animate
    lda.b #$06
    sta.w !wram_0D22
    lda.b #$00
    sta.w !wram_0D23
    inc.w !wram_0D59
    rts

; @routine 07:C911 PlaceFrameFromValue
; WHAT IT DOES: The car sprite's frame for a record's tag A: 0 below $0C, 3
; below $18, 6 below $24, else 9; kept at !sprite_piece_frame.
; HOW: SEP #$20, compares; REP #$20 at the end.
; Entry: A low byte = the tag. Exit: M 16-bit; A low byte = the frame; C
; from the last compare.
%at($07C911)
PlaceFrameFromValue:
    sep.b #$20
    cmp.b #$24
    bcs .lC92B
    cmp.b #$18
    bcs .lC927
    cmp.b #$0C
    bcs .lC923
    lda.b #$00
    bra .lC92D
.lC923:
    lda.b #$03
    bra .lC92D
.lC927:
    lda.b #$06
    bra .lC92D
.lC92B:
    lda.b #$09
.lC92D:
    sta.w !sprite_piece_frame
    rep.b #$20
    rts

; @routine 07:C933 KeepTimeDigits
; WHAT IT DOES: Keeps a time unpacked to BCD (TimeUnpackBcd: Y hundredths, X
; seconds, A minutes) at direct page $F1, $F3 and $F5.
; HOW: Three stores, 16-bit.
; Entry: M 16-bit, X 16-bit; D = $0000. Exit: every register unchanged.
%at($07C933)
KeepTimeDigits:
    sty.b !digit_count
    stx.b !dp_f3
    sta.b !save_block_offset
    rts

; @routine 07:C93A ScoreTiles
; WHAT IT DOES: Writes score A (BCD) into the tilemap buffer at X: the low
; byte's digits at +14 and +16; when the high byte is below $40 its digits
; at +0 and +2 and the tiles $C4, $C0 at +6 and +8; else the word spelled by
; $D9, $CE, $DB, $CF, $CE, $CC, $DD from -2 with attribute $24.
; HOW: A kept at $F1; 8-bit stores; digits are tile $C0 + the digit.
; Entry: M 16-bit; X = the buffer offset; D = $0000. Exit: M 16-bit.
%at($07C93A)
ScoreTiles:
    sta.b !digit_count
    sep.b #$20
    and.b #$0F
    clc
    adc.b #$C0
    sta.l !tilemap_7f93d7+16,x
    lda.b !digit_count
    lsr a
    lsr a
    lsr a
    lsr a
    clc
    adc.b #$C0
    sta.l !tilemap_7f93d7+14,x
    lda.b !text_count
    and.b #$FF
    cmp.b #$40
    bcs .lC981
    and.b #$0F
    clc
    adc.b #$C0
    sta.l !tilemap_7f93d7+2,x
    lda.b !text_count
    lsr a
    lsr a
    lsr a
    lsr a
    clc
    adc.b #$C0
    sta.l !tilemap_7f93d7,x
    lda.b #$C4
    sta.l !tilemap_7f93d7+6,x
    lda.b #$C0
    sta.l !tilemap_7f93d7+8,x
    rep.b #$20
    rts
.lC981:
    sep.b #$20
    lda.b #$D9
    sta.l !tilemap_7f93d7-2,x
    lda.b #$CE
    sta.l !tilemap_7f93d7,x
    lda.b #$DB
    sta.l !tilemap_7f93d7+2,x
    lda.b #$CF
    sta.l !tilemap_7f93d7+4,x
    lda.b #$CE
    sta.l !tilemap_7f93d7+6,x
    lda.b #$CC
    sta.l !tilemap_7f93d7+8,x
    lda.b #$DD
    sta.l !tilemap_7f93d7+10,x
    lda.b #$24
    sta.l !cycle_tilemap,x
    sta.l !tilemap_7f93d7+1,x
    sta.l !tilemap_7f93d7+3,x
    sta.l !tilemap_7f93d7+5,x
    sta.l !tilemap_7f93d7+7,x
    sta.l !tilemap_7f93d7+9,x
    sta.l !tilemap_7f93d7+11,x
    rep.b #$20
    rts

; @routine 07:C9CE TimeTiles
; WHAT IT DOES: Writes the time kept by KeepTimeDigits into the tilemap buffer
; at X: the minutes' digit at +0, the seconds' at +4 and +6, the hundredths'
; at +10 and +12.
; HOW: 8-bit; digits are tile $C0 + the digit.
; Entry: X = the buffer offset; D = $0000. Exit: M 16-bit.
%at($07C9CE)
TimeTiles:
    sep.b #$20
    lda.b !save_block_offset
    and.b #$FF
    clc
    adc.b #$C0
    sta.l !tilemap_7f93d7,x
    lda.b !dp_f3
    lsr a
    lsr a
    lsr a
    lsr a
    clc
    adc.b #$C0
    sta.l !tilemap_7f93d7+4,x
    lda.b !dp_f3
    and.b #$0F
    clc
    adc.b #$C0
    sta.l !tilemap_7f93d7+6,x
    lda.b !digit_count
    lsr a
    lsr a
    lsr a
    lsr a
    clc
    adc.b #$C0
    sta.l !tilemap_7f93d7+10,x
    lda.b !digit_count
    and.b #$0F
    clc
    adc.b #$C0
    sta.l !tilemap_7f93d7+12,x
    rep.b #$20
    rts

; @routine 07:CA0E NameTiles16
; WHAT IT DOES: Writes the three letters of name word A as tiles $CA + the
; letter into the tilemap buffer at X + 16, + 18, + 20.
; HOW: A kept on the stack; each letter AND #$1F after shifts and XBA.
; Entry: M 16-bit; X = the buffer offset. Exit: M 16-bit.
%at($07CA0E)
NameTiles16:
    pha
    sep.b #$20
    and.b #$1F
    clc
    adc.b #$CA
    sta.l !tilemap_7f93d7+16,x
    rep.b #$20
    pla
    pha
    asl a
    asl a
.lCA20:
    asl a
    xba
    sep.b #$20
    and.b #$1F
    clc
    adc.b #$CA
    sta.l !tilemap_7f93d7+18,x
    rep.b #$20
    pla
    lsr a
    lsr a
    xba
    sep.b #$20
    and.b #$1F
    clc
    adc.b #$CA
    sta.l !tilemap_7f93d7+20,x
    rep.b #$20
    rts

; @routine 07:CA41 NameTiles26
; WHAT IT DOES: As NameTiles16 at X + 26, + 28, + 30.
; HOW: As NameTiles16.
; Entry: as NameTiles16. Exit: as NameTiles16.
%at($07CA41)
NameTiles26:
    pha
    sep.b #$20
    and.b #$1F
    clc
    adc.b #$CA
    sta.l !tilemap_7f93d7+26,x
    rep.b #$20
    pla
    pha
    asl a
    asl a
    asl a
    xba
    sep.b #$20
    and.b #$1F
    clc
    adc.b #$CA
    sta.l !tilemap_7f93d7+28,x
    rep.b #$20
    pla
    lsr a
    lsr a
    xba
    sep.b #$20
    and.b #$1F
    clc
    adc.b #$CA
    sta.l !tilemap_7f93d7+30,x
    rep.b #$20
    rts

; @routine 07:CA74 StuntRecordsDraw
; WHAT IT DOES: Draws the Stunt Trax records panel (picture $FB21) and the
; three records with a score: their scores (ScoreTiles) at $190, $250, $310
; and their names (NameTiles26); the next step, $10A2 = 2.
; HOW: GsuCF42To3000, MVN of $440 bytes, DigitTiles1184. Reached only by
; unit_results_screen.
; Entry: D = $0000. Exit: M 8-bit, X 16-bit.
%at($07CA74)
StuntRecordsDraw:
    rep.b #$30
    ldx.w #$001F
    lda.w #$FB21
    jsl.l GsuCF42To3000
    phb
    php
    rep.b #$30
    lda.w #$043F
    ldx.w #$3000
    ldy.w #$93D7
    mvn $7F,$70
    plp
    plb
    jsr.w DigitTiles1184
    rep.b #$30
    lda.w !wram_0F2B
    beq .lCACA
    ldx.w #$0190
    jsr.w ScoreTiles
    lda.w !special_words
    jsr.w NameTiles26
    lda.w !wram_0F2B+2
    beq .lCACA
    ldx.w #$0250
    jsr.w ScoreTiles
    lda.w !special_words+2
    jsr.w NameTiles26
    lda.w !wram_0F2B+4
    beq .lCACA
    ldx.w #$0310
    jsr.w ScoreTiles
    lda.w !special_words+4
    jsr.w NameTiles26
.lCACA:
    sep.b #$20
    inc.w !wram_0D59
    lda.b #$02
    sta.w !wram_10A2
    rts

; @routine 07:CAD5 ResultsPanelFAB5
; WHAT IT DOES: Draws the panel of picture $FAB5; the next step.
; HOW: GsuCF42To3000, MVN of $440 bytes, DigitTiles1184. Reached only by
; unit_results_screen.
; Entry: D = $0000. Exit: M 8-bit, X 16-bit.
%at($07CAD5)
ResultsPanelFAB5:
    rep.b #$30
    ldx.w #$001F
    lda.w #$FAB5
    jsl.l GsuCF42To3000
    phb
    php
    rep.b #$30
    lda.w #$043F
    ldx.w #$3000
    ldy.w #$93D7
    mvn $7F,$70
    plp
    plb
    jsr.w DigitTiles1184
    sep.b #$20
    inc.w !wram_0D59
    rts

; @routine 07:CAFC TimedRecordsDraw
; WHAT IT DOES: Draws the timed records panel (picture $F3F2, X $1C) and the
; three records with a time: their times (TimeUnpackBcd, KeepTimeDigits,
; TimeTiles) at $194, $254, $314 and their names (NameTiles16); the next
; step, $10A2 = 2.
; HOW: As StuntRecordsDraw. Reached only by unit_results_screen.
; Entry: D = $0000. Exit: M 8-bit, X 16-bit.
%at($07CAFC)
TimedRecordsDraw:
    rep.b #$30
    ldx.w #$001C
    lda.w #$F3F2
    jsl.l GsuCF42To3000
    phb
    php
    rep.b #$30
    lda.w #$043F
    ldx.w #$3000
    ldy.w #$93D7
    mvn $7F,$70
    plp
    plb
    jsr.w DigitTiles1184
    rep.b #$20
    lda.w !wram_0F2B
    cmp.w #$FFFF
    beq .lCB71
    jsr.w TimeUnpackBcd
    jsr.w KeepTimeDigits
    ldx.w #$0194
    jsr.w TimeTiles
    lda.w !special_words
    jsr.w NameTiles16
    lda.w !wram_0F2B+2
    cmp.w #$FFFF
    beq .lCB71
    jsr.w TimeUnpackBcd
    jsr.w KeepTimeDigits
    rep.b #$10
    ldx.w #$0254
    jsr.w TimeTiles
    lda.w !special_words+2
    jsr.w NameTiles16
    lda.w !wram_0F2B+4
    cmp.w #$FFFF
    beq .lCB71
    jsr.w TimeUnpackBcd
    jsr.w KeepTimeDigits
    rep.b #$10
    ldx.w #$0314
    jsr.w TimeTiles
    lda.w !special_words+4
    jsr.w NameTiles16
.lCB71:
    sep.b #$20
    inc.w !wram_0D59
    lda.b #$02
    sta.w !wram_10A2
    rts

; @routine 07:CB7C ResultsPanelF38E
; WHAT IT DOES: Draws the panel of picture $F38E (X $1C); the next step.
; HOW: As ResultsPanelFAB5. Reached only by unit_results_screen.
; Entry: D = $0000. Exit: M 8-bit, X 16-bit.
%at($07CB7C)
ResultsPanelF38E:
    rep.b #$30
    ldx.w #$001C
    lda.w #$F38E
    jsl.l GsuCF42To3000
    phb
    php
    rep.b #$30
    lda.w #$043F
    ldx.w #$3000
    ldy.w #$93D7
    mvn $7F,$70
    plp
    plb
    jsr.w DigitTiles1184
    sep.b #$20
    inc.w !wram_0D59
    rts

; @routine 07:CBA3 StuntRecordsChoice
; WHAT IT DOES: With $19EC at $40 the place row at $199 cycles its palette
; (TilemapPaletteCycle, 7 entries); B or START goes on: to step $C for a
; placed player without $10CD, else the next step; sound $46.
; HOW: 8-bit, the presses 16-bit. Reached only by unit_results_screen.
; Entry: M 8-bit. Exit: as the path leaves it.
%at($07CBA3)
StuntRecordsChoice:
    sep.b #$20
    lda.w !hud_digits_19ec
    cmp.b #$40
    bne .lCBB7
    rep.b #$10
    ldx.w #$0199
    ldy.w #$0007
    jsr.w TilemapPaletteCycle
.lCBB7:
    rep.b #$30
    lda.w !joy1_pressed_copy
    bit.w #$9000
    beq .lCBE7
    lda.w !wram_10CD
    and.w #$00FF
    bne .lCBDC
    lda.w !player_place
    beq .lCBDC
    sep.b #$20
    lda.b #$0C
    sta.w !wram_0D59
    lda.b #$46
    jsl.l SoundPort0QueuePush
    rts
.lCBDC:
    sep.b #$20
    inc.w !wram_0D59
    lda.b #$46
    jsl.l SoundPort0QueuePush
.lCBE7:
    rts

; @routine 07:CBE8 TimedRecordsChoice
; WHAT IT DOES: B or START goes on: to step $F for a placed player, else the
; next step; sound $46.
; HOW: The presses 16-bit. Reached only by unit_results_screen.
; Entry: any widths. Exit: as the path leaves it.
%at($07CBE8)
TimedRecordsChoice:
    rep.b #$30
    lda.w !joy1_pressed_copy
    bit.w #$9000
    beq .lCC10
    lda.w !player_place
    beq .lCC05
    sep.b #$20
    lda.b #$0F
    sta.w !wram_0D59
    lda.b #$46
    jsl.l SoundPort0QueuePush
    rts
.lCC05:
    sep.b #$20
    inc.w !wram_0D59
    lda.b #$46
    jsl.l SoundPort0QueuePush
.lCC10:
    rts

; @routine 07:CC11 ResultsPanelFB99
; WHAT IT DOES: $0D23, the menu choice $0DD4 and the palette phase $0DD5
; cleared; the panel of picture $FB99; the next step, $0D23 = 2.
; HOW: As ResultsPanelFAB5. Reached only by unit_results_screen.
; Entry: D = $0000. Exit: M 8-bit, X 16-bit.
%at($07CC11)
ResultsPanelFB99:
    sep.b #$20
    lda.b #$00
    sta.w !wram_0D23
    sta.w !sound_port1_last+2
    sta.w !palette_phase
    rep.b #$30
    ldx.w #$001F
    lda.w #$FB99
    jsl.l GsuCF42To3000
    phb
    php
    rep.b #$30
    lda.w #$043F
    ldx.w #$3000
    ldy.w #$93D7
    mvn $7F,$70
    plp
    plb
    jsr.w DigitTiles1184
    sep.b #$20
    inc.w !wram_0D59
    lda.b #$02
    sta.w !wram_0D23
    rts

; @routine 07:CC4A ResultsPanelFA39
; WHAT IT DOES: Draws the panel of picture $FA39; with $0EEF set 07:D3F6, and
; with a record time at $0F81 other than $FF 07:D4F8; the next step.
; HOW: As ResultsPanelFAB5. The two calls are not reached in the traces so
; far.
; Entry: D = $0000. Exit: M 8-bit, X 16-bit.
%at($07CC4A)
ResultsPanelFA39:
    rep.b #$30
    ldx.w #$001F
    lda.w #$FA39
    jsl.l GsuCF42To3000
    phb
    php
    rep.b #$30
    lda.w #$043F
    ldx.w #$3000
    ldy.w #$93D7
    mvn $7F,$70
    plp
    plb
    jsr.w DigitTiles1184
    sep.b #$20
    lda.w !wram_0EEF
    beq .lCC75
    jsr.w ResultsTotalTiles
.lCC75:
    lda.w !record_times+11
    cmp.b #$FF
    beq .lCC7F
    jsr.w RecordTimeTiles
.lCC7F:
    inc.w !wram_0D59
    rts

; @routine 07:CC83 ResultsPicture4000
; WHAT IT DOES: Draws Super FX picture $C770 (X $18) to $4000
; (GsuCF42To4000); the next step; $10A2 and $10A3 cleared.
; HOW: 16-bit set-up, 8-bit after.
; Entry: D = $0000. Exit: M 8-bit, X 16-bit.
%at($07CC83)
ResultsPicture4000:
    rep.b #$30
    ldx.w #$0018
    lda.w #$C770
    jsl.l GsuCF42To4000
    sep.b #$20
    inc.w !wram_0D59
    stz.w !wram_10A2
    stz.w !wram_10A2+1
    rts

; @routine 07:CC9B ResultsCarPicture
; WHAT IT DOES: $0D23 = $12, the next step, and !fx_pool_first+8 = the car's
; picture word from .pictures ($63D7, $6FD7, $7BD7, $87D7).
; HOW: 8-bit, then 16-bit.
; Entry: any widths. Exit: M 16-bit, X 16-bit.
%at($07CC9B)
ResultsCarPicture:
    sep.b #$20
    lda.b #$12
    sta.w !wram_0D23
    inc.w !wram_0D59
    rep.b #$30
    lda.w !wram_0D89
    and.w #$00FF
    asl a
    tax
    lda.l ResultsCarPicture_pictures,x
    sta.w !fx_pool_first+8
    rts
.pictures:
    dw $63D7, $6FD7, $7BD7, $87D7

; @routine 07:CCBF ResultsMenu
; WHAT IT DOES: The menu after the race, unless the fade out has started: the
; choice $0DD4 (0 retry, 1 course, 2 car, 3 the car's choice) moves with up
; and down (sound $45; down from 0 not with $10CD) and its row cycles its
; palette; B or START: on 0 or 1 starts the fade out (sound $46); on 2 the
; car's choice (3); on 3 back to 0 (the car sprites' size byte = $55). In
; the car's choice, the car sprites 52 and 53 blink by bit 3 of the frame
; counter, and right and left change the car (skipping car 3 unless HUD mode
; 3 or the save's progress bit 2 allows it), then the selection is saved
; (SelectionSave) and the car's picture word set.
; HOW: 8-bit; the presses 16-bit. The move up from 1 and 2 and the car back
; are not reached in the traces so far.
; Entry: M 8-bit. Exit: as the path leaves it.
%at($07CCBF)
ResultsMenu:
    sep.b #$20
    lda.w !fade_out
    beq .lCCC7
    rts
.lCCC7:
    rep.b #$20
    lda.w !joy1_pressed_copy
    bit.w #$9000
    beq .lCD07
    sep.b #$20
    lda.w !sound_port1_last+2
    cmp.b #$02
    bcs .lCCE6
    lda.b #$01
    sta.w !fade_out
    lda.b #$46
    jsl.l SoundPort0QueuePush
    rts
.lCCE6:
    beq .lCCFB
    lda.b #$00
    sta.w !sound_port1_last+2
    lda.b #$46
    jsl.l SoundPort0QueuePush
    lda.b #$55
    sta.w !oam_buffer_high+13
    jmp.w .lCD79
.lCCFB:
    inc.w !sound_port1_last+2
    lda.b #$46
    jsl.l SoundPort0QueuePush
    jmp.w .lCE7F
.lCD07:
    sep.b #$20
    lda.b #$02
    sta.w !wram_0D23
    lda.w !sound_port1_last+2
    beq .lCD5D
    cmp.b #$02
    beq .lCD8D
    bcc .lCD1C
    brl .lCDA9
.lCD1C:
    rep.b #$20
    lda.w !joy1_pressed_copy
    bit.w #$0800
    bne .lCD3C
    lda.w !joy1_pressed_copy
    bit.w #$0400
    beq .lCD49
    sep.b #$20
    inc.w !sound_port1_last+2
    lda.b #$45
    jsl.l SoundPort0QueuePush
    jmp.w .lCE7F
.lCD3C:
    sep.b #$20
    stz.w !sound_port1_last+2
    lda.b #$45
    jsl.l SoundPort0QueuePush
    bra .lCD79
.lCD49:
    sep.b #$20
    rep.b #$10
    ldx.w #$0207
    ldy.w #$000D
    jsr.w TilemapPaletteCycle
    jsr.w TilemapRow187Palette4
    jsr.w TilemapRow287Palette4
    rts
.lCD5D:
    rep.b #$20
    lda.w !joy1_pressed_copy
    bit.w #$0400
    beq .lCD79
    sep.b #$20
    lda.w !wram_10CD
    bne .lCD79
    inc.w !sound_port1_last+2
    lda.b #$45
    jsl.l SoundPort0QueuePush
    bra .lCD49
.lCD79:
    sep.b #$20
    rep.b #$10
    ldx.w #$0187
    ldy.w #$0009
    jsr.w TilemapPaletteCycle
    jsr.w TilemapRow207Palette4
    jsr.w TilemapRow287Palette4
    rts
.lCD8D:
    rep.b #$20
    lda.w !joy1_pressed_copy
    bit.w #$0800
    bne .lCD9A
    brl .lCE7F
.lCD9A:
    sep.b #$20
    lda.b #$45
    jsl.l SoundPort0QueuePush
    lda.b #$01
    sta.w !sound_port1_last+2
    bra .lCD49
.lCDA9:
    sep.b #$20
    lda.w !frame_count
    bit.b #$08
    bne .lCDB6
    lda.b #$55
    bra .lCDE8
.lCDB6:
    lda.w !wram_10A4
    sec
    sbc.b #$08
    sta.w !oam_sprite52_x
    lda.w !wram_10A4
    clc
    adc.b #$40
    sta.w !oam_sprite53_x
    lda.w !wram_10A6
    clc
    adc.b #$18
    sta.w !oam_sprite52_x+1
    sta.w !oam_sprite53_x+1
    lda.b #$19
    sta.w !oam_sprite52_x+2
    sta.w !oam_sprite53_x+2
    lda.b #$76
    sta.w !oam_sprite52_x+3
    lda.b #$36
    sta.w !oam_sprite53_x+3
    lda.b #$50
.lCDE8:
    sta.w !oam_buffer_high+13
    rep.b #$20
    lda.w !joy1_pressed_copy
    bit.w #$0100
    beq .lCE23
    sep.b #$30
    lda.b #$45
    jsl.l SoundPort0QueuePush
    lda.w !wram_0D89
    inc a
    cmp.b #$03
    beq .lCE09
    bcs .lCE1C
    bcc .lCE1E
.lCE09:
    ldx.w !hud_mode
    cpx.b #$03
    beq .lCE1E
    lda.l !save_progress
    bit.b #$04
    beq .lCE1C
    lda.b #$03
    bra .lCE1E
.lCE1C:
    lda.b #$00
.lCE1E:
    sta.w !wram_0D89
    bra .lCE55
.lCE23:
    rep.b #$20
    lda.w !joy1_pressed_copy
    bit.w #$0200
    beq .lCE7F
    sep.b #$20
    lda.b #$45
    jsl.l SoundPort0QueuePush
    lda.w !wram_0D89
    dec a
    bpl .lCE52
    lda.b #$03
    ldx.w !hud_mode
    cpx.b #$03
    beq .lCE52
    lda.l !save_progress
    bit.b #$04
    beq .lCE50
    lda.b #$03
    bra .lCE52
.lCE50:
    lda.b #$02
.lCE52:
    sta.w !wram_0D89
.lCE55:
    sep.b #$30
    lda.w !wram_0D89
    tax
    lda.l ResultsMenu_carCodes,x
    sta.l !selection
    jsl.l SelectionSave
    sep.b #$20
    lda.b #$12
    sta.w !wram_0D23
    rep.b #$30
    lda.w !wram_0D89
    and.w #$00FF
    asl a
    tax
    lda.l ResultsCarPicture_pictures,x
    sta.w !fx_pool_first+8
.lCE7F:
    sep.b #$20
    rep.b #$10
    ldx.w #$0287
    ldy.w #$000A
    jsr.w TilemapPaletteCycle
    jsr.w TilemapRow187Palette4
    jsr.w TilemapRow207Palette4
    rts

; The selection codes of the four cars (ResultsMenu).
ResultsMenu_carCodes:
    db $02, $00, $01, $03

; @routine 07:CF32 ResultsChampionScreen
; WHAT IT DOES: Loads the championship screen's 256 colours into the palette
; buffer, sets BG2's scroll to (-24, -32) and its tilemap to $5C (BG2SC);
; the next step.
; HOW: A word copy from the end; BG2SC written 16-bit.
; Entry: any widths. Exit: M 8-bit, X 16-bit; X = $FFFE.
%at($07CF32)
ResultsChampionScreen:
    rep.b #$30
    ldx.w #$01FE
.lCF37:
    lda.l !championship_palette,x
    sta.l !palette_buffer,x
    dex
    dex
    bpl .lCF37
    lda.w #$FFE0
    sta.w !bg2_scroll_y
    lda.w #$FFE8
    sta.w !bg2_scroll_x
    lda.w #$005C
    sta.w !BG2SC
    sep.b #$20
    inc.w !wram_0D59
    rts

; @routine 07:CF5B ResultsWaitWindow
; WHAT IT DOES: Once the split window state is 0, the next step and sound $28
; on port 0; each frame $0D23 = 6.
; HOW: 8-bit.
; Entry: M 8-bit. Exit: A low byte = 6.
%at($07CF5B)
ResultsWaitWindow:
    lda.w !window_hdma_off
    bne .lCF69
    inc.w !wram_0D59
    lda.b #$28
    jsl.l SoundPort0QueuePushRaw
.lCF69:
    lda.b #$06
    sta.w !wram_0D23
    rts

; @routine 07:CF6F ResultsTotalLine
; WHAT IT DOES: The results' text line ($0D45's low nibble 2; OamTextLine with
; $F1-$F5 = $C0, $60, $67, $2F, $38); $0D23 = 4, the next step and the digit
; position $0E43 = $190.
; HOW: 8-bit, the position 16-bit.
; Entry: any widths; D = $0000. Exit: M 16-bit; A = $190.
%at($07CF6F)
ResultsTotalLine:
    sep.b #$20
    lda.w !wram_0D45
    and.b #$F0
    ora.b #$02
    sta.w !wram_0D45
    lda.b #$60
    sta.b !text_count
    lda.b #$67
    sta.b !dp_f3
    lda.b #$2F
    sta.b !text_y
    lda.b #$C0
    sta.b !digit_count
    lda.b #$38
    sta.b !save_block_offset
    jsr.w OamTextLine
    lda.b #$04
    sta.w !wram_0D23
    inc.w !wram_0D59
    rep.b #$20
    lda.w #$0190
    sta.w !wram_0E42+1
    rts

; @routine 07:CFE8 ResultsTotalLineWords
; WHAT IT DOES: As ResultsTotalLine, with the word row (OamWordRow5) and the
; position $18E; then with minutes at $0EEC the total's tiles
; (ResultsTotalTiles).
; HOW: As ResultsTotalLine; the last is a jump. The minutes are not reached
; in the traces so far.
; Entry: any widths; D = $0000. Exit: M 8-bit.
%at($07CFE8)
ResultsTotalLineWords:
    sep.b #$20
    lda.w !wram_0D45
    and.b #$F0
    ora.b #$02
    sta.w !wram_0D45
    lda.b #$60
    sta.b !text_count
    lda.b #$67
    sta.b !dp_f3
    lda.b #$2F
    sta.b !text_y
    lda.b #$C0
    sta.b !digit_count
    lda.b #$38
    sta.b !save_block_offset
    jsr.w OamTextLine
    jsr.w OamWordRow5
    lda.b #$04
    sta.w !wram_0D23
    inc.w !wram_0D59
    rep.b #$20
    lda.w #$018E
    sta.w !wram_0E42+1
    sep.b #$20
    lda.w !wram_0EEA+2
    beq .lD028
    jmp.w ResultsTotalTiles
.lD028:
    rts

; @routine 07:D055 StuntResultsCount
; WHAT IT DOES: Counts the Stunt Trax results up a step a frame: while the
; score's high byte $19EC is not $40, the count $0E80 one more (BCD) up to
; it, shown as two digit tiles; at $40 the word spelled by $D9, $CE, $DB,
; $CF, $CE, $CC, $DD there and the place row cycling; then the count $0E81
; up to the race time; with both reached, sound $3F and the next step. $0D23
; = 2.
; HOW: 8-bit; the counts in decimal mode. Reached only by
; unit_results_screen.
; Entry: any widths. Exit: M 8-bit.
%at($07D055)
StuntResultsCount:
    sep.b #$20
    lda.b #$02
    sta.w !wram_0D23
    lda.w !hud_digits_19ec
    cmp.b #$40
    beq .lD08B
    cmp.w !time_bonus_step+4
    beq .lD0C0
    sed
    lda.w !time_bonus_step+4
    clc
    adc.b #$01
    sta.w !time_bonus_step+4
    cld
    and.b #$0F
    clc
    adc.b #$C0
    sta.l !tilemap_7f93d7+410
    lda.w !time_bonus_step+4
    lsr a
    lsr a
    lsr a
    lsr a
    clc
    adc.b #$C0
    sta.l !tilemap_7f93d7+408
    rts
.lD08B:
    lda.b #$D9
    sta.l !tilemap_7f93d7+408
    lda.b #$CE
    sta.l !tilemap_7f93d7+410
    lda.b #$DB
    sta.l !tilemap_7f93d7+412
    lda.b #$CF
    sta.l !tilemap_7f93d7+414
    lda.b #$CE
    sta.l !tilemap_7f93d7+416
    lda.b #$CC
    sta.l !tilemap_7f93d7+418
    lda.b #$DD
    sta.l !tilemap_7f93d7+420
    rep.b #$10
    ldx.w #$0199
    ldy.w #$0007
    jsr.w TilemapPaletteCycle
.lD0C0:
    lda.w !race_time_bcd
    cmp.w !time_bonus_step+5
    beq .lD0EB
    sed
    lda.w !time_bonus_step+5
    clc
    adc.b #$01
    sta.w !time_bonus_step+5
    cld
    and.b #$0F
    clc
    adc.b #$C0
    sta.l !tilemap_7f93d7+538
    lda.w !time_bonus_step+5
    lsr a
    lsr a
    lsr a
    lsr a
    clc
    adc.b #$C0
    sta.l !tilemap_7f93d7+536
    rts
.lD0EB:
    lda.b #$3F
    jsl.l SoundPort0QueuePushRaw
    inc.w !wram_0D59
    rts

; @routine 07:D0F5 LapCharsLap1
; WHAT IT DOES: $0D23 = 2, then one character of the first lap's split at the
; digit position $0E43, by the character $0E3F: the minutes, ':', the
; seconds' two digits, '.', the hundredths' two; after the last the position
; moves to the next line and the step goes on.
; HOW: JMP (table,X) on $0E3F (16-bit X); digits are tile $C0 + the digit.
; Entry: any widths. Exit: as the character leaves it.
%at($07D0F5)
LapCharsLap1:
    sep.b #$20
    rep.b #$10
    lda.b #$02
    sta.w !wram_0D23
    ldy.w #$0000
    ldx.w !wram_0E3F
    jmp.w (RaceTimeTiles_chars,x)

; @routine 07:D107 LapCharsLap2
; WHAT IT DOES: As LapCharsLap1 for the second lap's split.
; HOW: As LapCharsLap1, Y = 3.
; Entry: any widths. Exit: as LapCharsLap1.
%at($07D107)
LapCharsLap2:
    sep.b #$20
    rep.b #$10
    ldy.w #$0003
    ldx.w !wram_0E3F
    jmp.w (RaceTimeTiles_chars,x)

; @routine 07:D114 LapCharsLap3
; WHAT IT DOES: As LapCharsLap1 for the third lap's split.
; HOW: As LapCharsLap1, Y = 6.
; Entry: any widths. Exit: as LapCharsLap1.
%at($07D114)
LapCharsLap3:
    sep.b #$20
    rep.b #$10
    ldy.w #$0006
    ldx.w !wram_0E3F
    jmp.w (RaceTimeTiles_chars,x)

; @routine 07:D121 LapCharsTotal
; WHAT IT DOES: As LapCharsLap1 for the split at $0EE7; its last character
; ends the laps (sound $3F, the position $F4 on).
; HOW: As LapCharsLap1, Y = 9.
; Entry: any widths. Exit: as LapCharsLap1.
%at($07D121)
LapCharsTotal:
    sep.b #$20
    rep.b #$10
    ldy.w #$0009
    ldx.w !wram_0E3F
    jmp.w (RaceTimeTiles_chars,x)

; @routine 07:D12E RaceTimeTiles
; WHAT IT DOES: Shows the race time at $0EE7-$0EE9 as digit tiles and keeps it
; for car $0D48 (three bytes at !lap_times+9 + $0D48 * 3); the next step.
; HOW: 8-bit; digits are tile $C0 + the digit. The lap characters' table and
; their code follow.
; Entry: any widths. Exit: M 8-bit, X 8-bit.
%at($07D12E)
RaceTimeTiles:
    sep.b #$20
    lda.w !wram_0EE7+2
    and.b #$0F
    clc
    adc.b #$C0
    sta.l !tilemap_7f93d7+910
    lda.w !wram_0EE7+1
    lsr a
    lsr a
    lsr a
    lsr a
    clc
    adc.b #$C0
    sta.l !tilemap_7f93d7+914
    lda.w !wram_0EE7+1
    and.b #$0F
    clc
    adc.b #$C0
    sta.l !tilemap_7f93d7+916
    lda.w !wram_0EE7
    lsr a
    lsr a
    lsr a
    lsr a
    clc
    adc.b #$C0
    sta.l !tilemap_7f93d7+920
    lda.w !wram_0EE7
    and.b #$0F
    clc
    adc.b #$C0
    sta.l !tilemap_7f93d7+922
    sep.b #$30
    lda.w !wram_0D48
    asl a
    clc
    adc.w !wram_0D48
    tax
    lda.w !wram_0EE7
    sta.w !lap_times+9,x
    lda.w !wram_0EE7+1
    sta.w !lap_times+10,x
    lda.w !wram_0EE7+2
    sta.w !lap_times+11,x
    inc.w !wram_0D59
    rts
RaceTimeTiles_chars:
    dw .lD19F, .lD1AB, .lD1B2, .lD1C3, .lD1D0, .lD1D7, .lD1E6
.lD19F:
    ldx.w !wram_0E42+1
    lda.w !lap_splits+2,y
    clc
    adc.b #$C0
    jmp.w .lD221
.lD1AB:
    ldx.w !wram_0E42+1
    lda.b #$E6
    bra .lD221
.lD1B2:
    ldx.w !wram_0E42+1
    lda.w !lap_splits+1,y
    lsr a
    lsr a
    lsr a
    lsr a
    and.b #$0F
    clc
    adc.b #$C0
    bra .lD221
.lD1C3:
    ldx.w !wram_0E42+1
    lda.w !lap_splits+1,y
    and.b #$0F
    clc
    adc.b #$C0
    bra .lD221
.lD1D0:
    ldx.w !wram_0E42+1
    lda.b #$E7
    bra .lD221
.lD1D7:
    ldx.w !wram_0E42+1
    lda.w !lap_splits,y
    lsr a
    lsr a
    lsr a
    lsr a
    clc
    adc.b #$C0
    bra .lD221
.lD1E6:
    ldx.w !wram_0E42+1
    lda.w !lap_splits,y
    and.b #$0F
    clc
    adc.b #$C0
    sta.l !tilemap_7f93d7,x
    lda.w !wram_0D59
    inc a
    sta.w !wram_0D59
    cmp.b #$0B
    bne .lD211
    lda.b #$3F
    jsl.l SoundPort0QueuePushRaw
    rep.b #$20
    lda.w !wram_0E42+1
    clc
    adc.w #$00F4
    bra .lD21A
.lD211:
    rep.b #$20
    lda.w !wram_0E42+1
    clc
    adc.w #$0074
.lD21A:
    sta.w !wram_0E42+1
    stz.w !wram_0E3F
    rts
.lD221:
    sta.l !tilemap_7f93d7,x
    inc.w !wram_0E3F
    inc.w !wram_0E3F
    inc.w !wram_0E42+1
    inc.w !wram_0E42+1
    rts

; @routine 07:D232 ResultsBlinkWait
; WHAT IT DOES: Counts $0E3F up; below $3C the row at $38F (7 entries) cycles
; its palette; at $3C it clears $0E3F, goes to the next step with sound $28
; on port 0 and gives the row palette 0.
; HOW: 8-bit; TilemapPaletteCycle or TilemapPaletteFill.
; Entry: M 8-bit. Exit: as the palette routine leaves it.
%at($07D232)
ResultsBlinkWait:
    inc.w !wram_0E3F
    lda.w !wram_0E3F
    cmp.b #$3C
    bcc .lD25A
    stz.w !wram_0E3F
    inc.w !wram_0D59
    lda.b #$28
    jsl.l SoundPort0QueuePushRaw
    sep.b #$20
    rep.b #$10
    ldx.w #$038F
    ldy.w #$0007
    lda.b #$00
    sta.b !digit_count
    jsr.w TilemapPaletteFill
    rts
.lD25A:
    sep.b #$20
    rep.b #$10
    ldx.w #$038F
    ldy.w #$0007
    jsr.w TilemapPaletteCycle
    rts

; @routine 07:D268 ResultsBonusCount
; WHAT IT DOES: Counts the time bonus $0EEA-$0EEC into the total $0EED-$0EEF
; (BCD) a step a frame: $0E3F counts the frames (at most $FA); from frame $12
; B or START adds all of it at once; otherwise a step of 1, $57 or $73
; hundredths or 2.17 seconds by the bonus's size, the step on with sound $3F
; when the hundredths run out; seconds past 60 carry into the minutes; then
; the bonus's digits and the total's (ResultsTotalTiles, which follows).
; HOW: Decimal mode (SED ... CLD). B or START, the minutes' carry and the
; bonus of a minute or more are not reached in the traces so far.
; Entry: M 8-bit. Exit: M 8-bit; as ResultsTotalTiles leaves it.
%at($07D268)
ResultsBonusCount:
    lda.w !wram_0E3F
    inc a
    cmp.b #$FA
    bcc .lD272
    lda.b #$FA
.lD272:
    sta.w !wram_0E3F
    cmp.b #$12
    bcc .lD2E2
    rep.b #$20
    lda.w !joy1_pressed_copy
    bit.w #$9000
    beq .lD2E2
    sep.b #$20
    sed
    lda.w !wram_0EED
    clc
    adc.w !wram_0EEA
    sta.w !wram_0EED
    lda.w !wram_0EEE
    adc.w !wram_0EEA+1
    bcc .lD2AA
    sec
    sbc.b #$60
    sta.w !wram_0EEE
    lda.w !wram_0EEF
    clc
    adc.b #$01
    sta.w !wram_0EEF
    lda.w !wram_0EEE
.lD2AA:
    sta.w !wram_0EEE
    cmp.b #$60
    bcc .lD2BF
    sbc.b #$60
    sta.w !wram_0EEE
    lda.w !wram_0EEF
    clc
    adc.b #$01
    sta.w !wram_0EEF
.lD2BF:
    lda.w !wram_0EEF
    adc.w !wram_0EEA+2
    sta.w !wram_0EEF
    stz.w !wram_0EEA
    stz.w !wram_0EEA+1
    stz.w !wram_0EEA+2
    inc.w !wram_0D59
    lda.b #$02
    sta.w !wram_10A2
    lda.b #$3F
    jsl.l SoundPort0QueuePushRaw
    jmp.w .lD3B5
.lD2E2:
    sep.b #$20
    sed
    lda.w !wram_0EEA+2
    beq .lD2ED
    brl .lD36D
.lD2ED:
    lda.w !wram_0EEA+1
    beq .lD2F9
    cmp.b #$01
    beq .lD325
    jmp.w .lD349
.lD2F9:
    lda.w !wram_0EEA
    sec
    sbc.b #$01
    sta.w !wram_0EEA
    bne .lD312
    inc.w !wram_0D59
    lda.b #$02
    sta.w !wram_10A2
    lda.b #$3F
    jsl.l SoundPort0QueuePushRaw
.lD312:
    lda.w !wram_0EED
    clc
    adc.b #$01
    sta.w !wram_0EED
    lda.w !wram_0EEE
    adc.b #$00
    sta.w !wram_0EEE
    bra .lD3A0
.lD325:
    lda.w !wram_0EEA
    sec
    sbc.b #$73
    sta.w !wram_0EEA
    lda.w !wram_0EEA+1
    sbc.b #$00
    sta.w !wram_0EEA+1
    lda.w !wram_0EED
    clc
    adc.b #$73
    sta.w !wram_0EED
    lda.w !wram_0EEE
    adc.b #$00
    sta.w !wram_0EEE
    bra .lD3A0
.lD349:
    lda.w !wram_0EEA
    sec
    sbc.b #$57
    sta.w !wram_0EEA
    lda.w !wram_0EEA+1
    sbc.b #$00
    sta.w !wram_0EEA+1
    lda.w !wram_0EED
    clc
    adc.b #$57
    sta.w !wram_0EED
    lda.w !wram_0EEE
    adc.b #$00
    sta.w !wram_0EEE
    bra .lD3A0
.lD36D:
    lda.w !wram_0EEA
    sec
    sbc.b #$17
    sta.w !wram_0EEA
    lda.w !wram_0EEA+1
    sbc.b #$02
    sta.w !wram_0EEA+1
    bcs .lD38F
    clc
    adc.b #$60
    sta.w !wram_0EEA+1
    lda.w !wram_0EEA+2
    sec
    sbc.b #$01
    sta.w !wram_0EEA+2
.lD38F:
    lda.w !wram_0EED
    clc
    adc.b #$17
    sta.w !wram_0EED
    lda.w !wram_0EEE
    adc.b #$02
    sta.w !wram_0EEE
.lD3A0:
    lda.w !wram_0EEE
    cmp.b #$60
    bcc .lD3B5
    sbc.b #$60
    sta.w !wram_0EEE
    clc
    lda.w !wram_0EEF
    adc.b #$01
    sta.w !wram_0EEF
.lD3B5:
    cld
    lda.w !wram_0EEA+2
    and.b #$0F
    clc
    adc.b #$C0
    sta.l !tilemap_7f93d7+910
    lda.w !wram_0EEA+1
    lsr a
    lsr a
    lsr a
    lsr a
    clc
    adc.b #$C0
    sta.l !tilemap_7f93d7+914
    lda.w !wram_0EEA+1
    and.b #$0F
    clc
    adc.b #$C0
    sta.l !tilemap_7f93d7+916
    lda.w !wram_0EEA
    lsr a
    lsr a
    lsr a
    lsr a
    clc
    adc.b #$C0
    sta.l !tilemap_7f93d7+920
    lda.w !wram_0EEA
    and.b #$0F
    clc
    adc.b #$C0
    sta.l !tilemap_7f93d7+922

; @routine 07:D3F6 ResultsTotalTiles
; WHAT IT DOES: Shows the total time at $0EED-$0EEF as tiles: the minutes'
; tens (blank for 0), units, ':', the seconds, '.', the hundredths.
; HOW: 8-bit; digits are tile $C0 + the digit.
; Entry: M 8-bit. Exit: A low byte = the last tile.
%at($07D3F6)
ResultsTotalTiles:
    lda.w !wram_0EEF
    lsr a
    lsr a
    lsr a
    lsr a
    clc
    adc.b #$C0
    cmp.b #$C0
    bne .lD416
    lda.b #$0F
    sta.l !tilemap_7f93d7+544
    lda.l !tilemap_7f93d7+545
    ora.b #$01
    sta.l !tilemap_7f93d7+545
    bra .lD424
.lD416:
    sta.l !tilemap_7f93d7+544
    lda.l !tilemap_7f93d7+545
    and.b #$E2
    sta.l !tilemap_7f93d7+545
.lD424:
    lda.w !wram_0EEF
    and.b #$0F
    clc
    adc.b #$C0
    sta.l !tilemap_7f93d7+546
    lda.b #$E6
    sta.l !tilemap_7f93d7+548
    lda.w !wram_0EEE
    lsr a
    lsr a
    lsr a
    lsr a
    clc
    adc.b #$C0
    sta.l !tilemap_7f93d7+550
    lda.w !wram_0EEE
    and.b #$0F
    clc
    adc.b #$C0
    sta.l !tilemap_7f93d7+552
    lda.b #$E7
    sta.l !tilemap_7f93d7+554
    lda.w !wram_0EED
    lsr a
    lsr a
    lsr a
    lsr a
    clc
    adc.b #$C0
    sta.l !tilemap_7f93d7+556
    lda.w !wram_0EED
    and.b #$0F
    clc
    adc.b #$C0
    sta.l !tilemap_7f93d7+558
    rts

; @routine 07:D471 ResultsRecordCompare
; WHAT IT DOES: Compares the total with car $0D48's record time: when slower
; the row at $221 gets palette 4; when faster the record's row at $361
; palette 4 (if there is a record) and the total's row at $221 cycles; B or
; START starts the fade out; $10A2 = 3 when it was 1.
; HOW: SEP #$30; byte compares from the minutes. Slower and equal are not
; reached in the traces so far.
; Entry: any widths. Exit: M 8-bit, X 16-bit or 8-bit.
%at($07D471)
ResultsRecordCompare:
    sep.b #$30
    clc
    lda.w !wram_0D48
    adc.w !wram_0D48
    adc.w !wram_0D48
    tax
    lda.w !wram_0EEF
    cmp.w !record_times+2,x
    bcc .lD4AF
    bne .lD49C
    lda.w !wram_0EEE
    cmp.w !record_times+1,x
    bcc .lD4AF
    bne .lD49C
    lda.w !wram_0EED
    cmp.w !record_times,x
    bcc .lD4AF
    beq .lD4D4
.lD49C:
    sep.b #$20
    rep.b #$10
    ldx.w #$0221
    ldy.w #$0008
    lda.b #$10
    sta.b !digit_count
    jsr.w TilemapPaletteFill
    bra .lD4D4
.lD4AF:
    lda.w !record_times+2,x
    cmp.b #$FF
    beq .lD4C7
    sep.b #$20
    rep.b #$10
    ldx.w #$0361
    ldy.w #$0008
    lda.b #$10
    sta.b !digit_count
    jsr.w TilemapPaletteFill
.lD4C7:
    sep.b #$20
    rep.b #$10
    ldx.w #$0221
    ldy.w #$0008
    jsr.w TilemapPaletteCycle
.lD4D4:
    rep.b #$20
    lda.w !joy1_pressed_copy
    bit.w #$9000
    beq .lD4E5
    sep.b #$20
    lda.b #$01
    sta.w !fade_out
.lD4E5:
    sep.b #$20
    lda.w !wram_10A2
    cmp.b #$07
    beq .lD4F7
    cmp.b #$01
    bne .lD4F7
    lda.b #$03
    sta.w !wram_10A2
.lD4F7:
    rts

; @routine 07:D4F8 RecordTimeTiles
; WHAT IT DOES: Shows car $0D48's record time as tiles: the minutes (blank
; tens for 0), ':', the seconds, '.', the hundredths.
; HOW: SEP #$10; 8-bit. Reached only by unit_results_screen.
; Entry: M 8-bit. Exit: X 8-bit.
%at($07D4F8)
RecordTimeTiles:
    sep.b #$10
    clc
    lda.w !wram_0D48
    adc.w !wram_0D48
    adc.w !wram_0D48
    tax
    lda.w !record_times+2,x
    lsr a
    lsr a
    lsr a
    lsr a
    clc
    adc.b #$C0
    cmp.b #$C0
    bne .lD525
    lda.b #$0F
    sta.l !tilemap_7f93d7+864
    lda.l !tilemap_7f93d7+865
    ora.b #$01
    sta.l !tilemap_7f93d7+865
    bra .lD529
.lD525:
    sta.l !tilemap_7f93d7+864
.lD529:
    lda.w !record_times+2,x
    and.b #$0F
    clc
    adc.b #$C0
    sta.l !tilemap_7f93d7+866
    lda.b #$E6
    sta.l !tilemap_7f93d7+868
    lda.w !record_times+1,x
    lsr a
    lsr a
    lsr a
    lsr a
    clc
    adc.b #$C0
    sta.l !tilemap_7f93d7+870
    lda.w !record_times+1,x
    and.b #$0F
    clc
    adc.b #$C0
    sta.l !tilemap_7f93d7+872
    lda.b #$E7
    sta.l !tilemap_7f93d7+874
    lda.w !record_times,x
    lsr a
    lsr a
    lsr a
    lsr a
    clc
    adc.b #$C0
    sta.l !tilemap_7f93d7+876
    lda.w !record_times,x
    and.b #$0F
    clc
    adc.b #$C0
    sta.l !tilemap_7f93d7+878
    rts
