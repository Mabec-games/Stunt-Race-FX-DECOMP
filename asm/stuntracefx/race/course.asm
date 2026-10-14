; course.asm: the course number $0E3D, from the menu's selections.
; C: src/stuntracefx/race/course.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 02:EB4D CourseFromSelectionLong
; WHAT IT DOES: CourseFromSelection for callers in other banks.
; HOW: JSR to it, then RTL.
; Entry: as CourseFromSelection. Exit: as CourseFromSelection.
%at($02EB4D)
CourseFromSelectionLong:
    jsr.w CourseFromSelection
    rtl

; @routine 02:EB51 CourseFromSelection
; WHAT IT DOES: Sets the course number $0E3D: !wram_0D4A * 4 + $0D48, or $0D4A
; + $14 when $0D4C is set (not reached in the traces so far).
; HOW: 8-bit arithmetic through the data bank.
; Entry: M 8-bit. Exit: A low byte = the course; N, Z, C, V from the add.
%at($02EB51)
CourseFromSelection:
    lda.w !digit62_override
    beq .lEB60
    lda.w !wram_0D4A
    clc
    adc.b #$14
    sta.w !course
    rts
.lEB60:
    lda.w !wram_0D4A
    asl a
    asl a
    clc
    adc.w !wram_0D48
    sta.w !course
    rts

; @routine 02:EC10 RaceFadeStageDispatch
; WHAT IT DOES: Runs the handler for stage $0D48 from the table at
; $02:EC18 ($ECB8, $ECD9, $EC30, $EC73).
; HOW: ASL, TAX, JMP (table,X) with 8-bit registers; the handler returns
; to the caller.
; Entry: M 8-bit, X 8-bit. Exit: as the handler leaves it.
%at($02EC10)
RaceFadeStageDispatch:
    lda.w !wram_0D48
    asl a
    tax
    jmp.w (!race_fade_stage_handlers,x)

; @routine 03:FBB6 CourseWindowZoomSetup
; WHAT IT DOES: Sets up the course window's zoom: the four words at
; $7F:3432 and their bases at $7F:3442 from the four words at $03:FC11 +
; X, their targets at $7F:343A from the four at $03:FB8E + Y, the blend
; factor $7F:3408 = 0 and its limit $7F:340A = 15; then builds the window
; table from them (WindowTable7FLong).
; HOW: 16-bit loads through the data bank ($03), long stores.
; Entry: M and X 16-bit; data bank $03. Exit: as WindowTable7FLong.
%at($03FBB6)
CourseWindowZoomSetup:
    lda.w !course_zoom_targets,y
    sta.l !scaled_words_target
    lda.w !course_zoom_targets+2,y
    sta.l !scaled_words_target+2
    lda.w !course_zoom_targets+4,y
    sta.l !scaled_words_target+4
    lda.w !course_zoom_targets+6,y
    sta.l !scaled_words_target+6
    lda.w !course_zoom_boxes,x
    sta.l !scaled_words
    sta.l !scaled_words_base
    lda.w !course_zoom_boxes+2,x
    sta.l !scaled_words+2
    sta.l !scaled_words_base+2
    lda.w !course_zoom_boxes+4,x
    sta.l !scaled_words+4
    sta.l !scaled_words_base+4
    lda.w !course_zoom_boxes+6,x
    sta.l !scaled_words+6
    sta.l !scaled_words_base+6
    lda.w #$000F
    sta.l !scale_divisor
    lda.w #$0000
    sta.l !fixed_colour_step
    jsl.l WindowTable7FLong
    rts

; @routine 03:FB25 CourseZoomStunt
; WHAT IT DOES: Sets the course $0E3D from the cursor $0D48 in the list at
; $03:FB86 (courses $0C-$0F), then starts the course window's zoom from
; box $0D48 * 8 + $80 at $03:FC11 to target 8 at $03:FB8E: colour maths
; $A3, the large sprites hidden, $0D40 = 4 and sound $24.
; HOW: 16-bit; the zoom is the tail of CourseZoomBattle (BRA $FB64), with
; the data bank $03 around it.
; Entry: any widths. Exit: M 8-bit, X 16-bit; the data bank restored; N,
; Z from it; A, X, Y as SoundPort0QueuePush leaves them.
%at($03FB25)
CourseZoomStunt:
    rep.b #$30
    ldx.w !wram_0D48
    lda.l !course_list_stunt,x
    and.w #$00FF
    sta.w !course
    and.w #$00FF
    lda.w !wram_0D48
    asl a
    asl a
    asl a
    clc
    adc.w #$0080
    tax
    ldy.w #$0008
    bra CourseZoomBattle_zoom

; @routine 03:FB47 CourseZoomBattle
; WHAT IT DOES: As CourseZoomStunt with the list at $03:FB8A (courses
; $10-$13) and target $10.
; HOW: As CourseZoomStunt; the zoom's tail at $FB64 is also where the
; other course choices go.
; Entry: any widths. Exit: as CourseZoomStunt.
%at($03FB47)
CourseZoomBattle:
    rep.b #$30
    ldx.w !wram_0D48
    lda.l !course_list_battle,x
    and.w #$00FF
    sta.w !course
    lda.w !wram_0D48
    asl a
    asl a
    asl a
    clc
    adc.w #$0080
    tax
    ldy.w #$0010
CourseZoomBattle_zoom:
    sep.b #$20
    phb
    phk
    plb
    rep.b #$30
    jsr.w CourseWindowZoomSetup
    jsr.w ColourMathA3
    jsl.l OamBufferHideLargeLong
    sep.b #$20
    lda.b #$04
    sta.w !wram_0D3F+1
    lda.b #$24
    jsl.l SoundPort0QueuePush
    sep.b #$20
    plb
    rtl

; @routine 03:FA51 CourseZoomFromSelection4
; WHAT IT DOES: Sets the course from the menu's selections
; (CourseFromSelectionLong) and starts the course window's zoom: for a
; course below $0C from box (course - 4) * 8 (+ $40, target $18, with a
; HUD mode $0D2B; else target 0), for course $15 from box $A8 and for the
; others from box $B0, both to target $20.
; HOW: As CourseZoomStunt, through the zoom's tail at $03:FB64. A HUD mode
; is not reached in the traces so far.
; Entry: any widths. Exit: as CourseZoomStunt.
%at($03FA51)
CourseZoomFromSelection4:
    sep.b #$20
    jsl.l CourseFromSelectionLong
    cmp.b #$0C
    bcc .lFA61
    cmp.b #$15
    beq CourseZoomFromSelection4_a8
    bra CourseZoomFromSelection4_b0
.lFA61:
    rep.b #$30
    and.w #$00FF
    sec
    sbc.w #$0004
    asl a
    asl a
    asl a
    ldx.w !hud_mode
    beq .lFA7D
    clc
    adc.w #$0040
    tax
    ldy.w #$0018
    jmp.w CourseZoomBattle_zoom
.lFA7D:
    clc
    adc.w #$0000
    tax
    ldy.w #$0000
    jmp.w CourseZoomBattle_zoom
CourseZoomFromSelection4_b0:
    rep.b #$30
    lda.w #$0008
    clc
    adc.w #$00A8
    tax
    ldy.w #$0020
    jmp.w CourseZoomBattle_zoom
CourseZoomFromSelection4_a8:
    rep.b #$30
    lda.w #$0000
    clc
    adc.w #$00A8
    tax
    ldy.w #$0020
    jmp.w CourseZoomBattle_zoom

; @routine 03:FAA8 CourseZoomFromSelection
; WHAT IT DOES: As CourseZoomFromSelection4 with box course * 8 for the
; courses below $0C, and boxes $B0 for course $15 and $A8 for the others.
; HOW: 8-bit index registers for CourseFromSelectionLong; the other
; choices go to CourseZoomFromSelection4's. A HUD mode is not reached in
; the traces so far.
; Entry: any widths. Exit: as CourseZoomStunt.
%at($03FAA8)
CourseZoomFromSelection:
    sep.b #$30
    jsl.l CourseFromSelectionLong
    cmp.b #$0C
    bcc .lFAB8
    cmp.b #$15
    beq CourseZoomFromSelection4_b0
    bra CourseZoomFromSelection4_a8
.lFAB8:
    rep.b #$30
    and.w #$00FF
    asl a
    asl a
    asl a
    ldx.w !hud_mode
    beq .lFAD0
    clc
    adc.w #$0040
    tax
    ldy.w #$0018
    jmp.w CourseZoomBattle_zoom
.lFAD0:
    clc
    adc.w #$0000
    tax
    ldy.w #$0000
    jmp.w CourseZoomBattle_zoom

; @routine 02:EC20 CourseCursorStep
; WHAT IT DOES: Moves the course cursor $0D48 by the pressed buttons: the
; handler for its place from the table at $02:EC28 (CourseCursorAt0 to
; CourseCursorAt3).
; HOW: LDA, ASL, TAX in the current widths (with M 8-bit and X 16-bit, TAX
; takes A's high byte too), then JMP (table,X).
; Entry: any widths. Exit: as the handler leaves it.
%at($02EC20)
CourseCursorStep:
    lda.w !wram_0D48
    asl a
    tax
    jmp.w (CourseCursorStep_table,x)
CourseCursorStep_table:
    dw CourseCursorAt0, CourseCursorAt1, CourseCursorAt2, CourseCursorAt3

; @routine 02:EC30 CourseCursorAt2Open
; WHAT IT DOES: As CourseCursorAt2 where a fourth course can open: first,
; Up ($0800) with the low four bits of $70:FC06 all set gives sound $45
; and $0D63 = $FF; otherwise Right ($0100) moves the cursor to 3, Down
; ($0400) to 0.
; HOW: As CourseCursorAt0; the Up test is 8-bit. Up is not reached in the
; traces so far.
; Entry: any widths. Exit: as CourseCursorAt0; after Up, A low byte $FF.
CourseCursorAt2Open:
    rep.b #$20
    lda.w !joy1_pressed_copy
    bit.w #$0800
    beq .lEC52
    sep.b #$20
    lda.l !save_bytes
    and.b #$0F
    cmp.b #$0F
    bne .lEC52
    lda.b #$45
    jsl.l SoundPort0QueuePush
    lda.b #$FF
    sta.w !wram_0D63
    rts
.lEC52:
    rep.b #$20
    lda.w !joy1_pressed_copy
    bit.w #$0100
    bne CourseCursorStep_to3_open
    lda.w !joy1_pressed_copy
    bit.w #$0400
    bne CourseCursorStep_to0
    rts
CourseCursorStep_to2_open:
    sep.b #$20
    lda.b #$45
    jsl.l SoundPort0QueuePush
    lda.b #$02
    sta.w !wram_0D48
    rts

; @routine 02:EC73 CourseCursorAt3Open
; WHAT IT DOES: As CourseCursorAt3 where a fourth course can open: Up as
; in CourseCursorAt2Open; otherwise Left ($0200) moves the cursor to 2,
; Down ($0400) to 1.
; HOW: As CourseCursorAt2Open. Up and Down are not reached in the traces
; so far.
; Entry: any widths. Exit: as CourseCursorAt2Open.
CourseCursorAt3Open:
    rep.b #$20
    lda.w !joy1_pressed_copy
    bit.w #$0800
    beq .lEC95
    sep.b #$20
    lda.l !save_bytes
    and.b #$0F
    cmp.b #$0F
    bne .lEC95
    lda.b #$45
    jsl.l SoundPort0QueuePush
    lda.b #$FF
    sta.w !wram_0D63
    rts
.lEC95:
    rep.b #$20
    lda.w !joy1_pressed_copy
    bit.w #$0200
    bne CourseCursorStep_to2_open
    lda.w !joy1_pressed_copy
    bit.w #$0400
    beq CourseCursorStep_done
    jmp.w $ECEC
CourseCursorStep_to3_open:
    sep.b #$20
    lda.b #$45
    jsl.l SoundPort0QueuePush
    lda.b #$03
    sta.w !wram_0D48
    rts

; @routine 02:ECB8 CourseCursorAt0
; WHAT IT DOES: The course cursor at 0 (bottom left): Right ($0100 in the
; pressed buttons $0D1A) moves it to 1, Up ($0800) to 2, each with sound
; $45.
; HOW: 16-bit BIT tests; the move is 8-bit.
; Entry: any widths. Exit: M 8-bit after a move (A low byte the new
; place), else M 16-bit with A = the buttons and Z set.
CourseCursorAt0:
    rep.b #$20
    lda.w !joy1_pressed_copy
    bit.w #$0100
    bne CourseCursorStep_to1
    lda.w !joy1_pressed_copy
    bit.w #$0800
    bne CourseCursorStep_to2
    rts
CourseCursorStep_to0:
    sep.b #$20
    lda.b #$45
    jsl.l SoundPort0QueuePush
    lda.b #$00
    sta.w !wram_0D48
    rts

; @routine 02:ECD9 CourseCursorAt1
; WHAT IT DOES: The course cursor at 1 (bottom right): Left ($0200) moves
; it to 0, Up ($0800) to 3.
; HOW: As CourseCursorAt0.
; Entry: any widths. Exit: as CourseCursorAt0.
CourseCursorAt1:
    rep.b #$20
    lda.w !joy1_pressed_copy
    bit.w #$0200
    bne CourseCursorStep_to0
    lda.w !joy1_pressed_copy
    bit.w #$0800
    bne CourseCursorStep_to3
    rts
CourseCursorStep_to1:
    sep.b #$20
    lda.b #$45
    jsl.l SoundPort0QueuePush
    lda.b #$01
    sta.w !wram_0D48
CourseCursorStep_done:
    rts

; @routine 02:ECFA CourseCursorAt2
; WHAT IT DOES: The course cursor at 2 (top left): Right ($0100) moves it
; to 3, Down ($0400) to 0.
; HOW: As CourseCursorAt0.
; Entry: any widths. Exit: as CourseCursorAt0.
CourseCursorAt2:
    rep.b #$20
    lda.w !joy1_pressed_copy
    bit.w #$0100
    bne CourseCursorStep_to3
    lda.w !joy1_pressed_copy
    bit.w #$0400
    bne CourseCursorStep_to0
    rts
CourseCursorStep_to2:
    sep.b #$20
    lda.b #$45
    jsl.l SoundPort0QueuePush
    lda.b #$02
    sta.w !wram_0D48
    rts

; @routine 02:ED1B CourseCursorAt3
; WHAT IT DOES: The course cursor at 3 (top right): Left ($0200) moves it
; to 2, Down ($0400) to 1.
; HOW: As CourseCursorAt0. The move to 1 is not reached in the traces so
; far.
; Entry: any widths. Exit: as CourseCursorAt0.
CourseCursorAt3:
    rep.b #$20
    lda.w !joy1_pressed_copy
    bit.w #$0200
    bne CourseCursorStep_to2
    lda.w !joy1_pressed_copy
    bit.w #$0400
    beq CourseCursorStep_done
    jmp.w $ECEC
CourseCursorStep_to3:
    sep.b #$20
    lda.b #$45
    jsl.l SoundPort0QueuePush
    lda.b #$03
    sta.w !wram_0D48
    rts

; @routine 02:ED3E CourseZoomInStep
; WHAT IT DOES: A step of the course window's zoom: colour maths $A3
; (ColourMathA3Long), the blend one step on (BlendStepUp) and the fixed
; colour from it (FixedColourFromStep); at the last step, $0D50 = 0, the
; course's records loaded (RecordsLoadForCourse), $0D41 = $20, $0D40 = 6
; and $10D8 = 0.
; HOW: FixedColourFromStep leaves M and X 8-bit, so the compare of
; $7F:3408 with $7F:340A is of their low bytes; 8-bit stores.
; Entry: any widths. Exit: M, X 8-bit; before the last step A low byte the
; blend factor's (high byte as FixedColourFromStep left it), Z clear;
; then A low byte 6.
CourseZoomInStep:
    jsl.l ColourMathA3Long
    rep.b #$30
    jsl.l BlendStepUp
    jsl.l FixedColourFromStep
    lda.l !fixed_colour_step
    cmp.l !scale_divisor
    bne .lED6C
    sep.b #$20
    stz.w !hud_blank+2
    jsl.l RecordsLoadForCourse
    lda.b #$20
    sta.w !wram_0D3F+2
    lda.b #$06
    sta.w !wram_0D3F+1
    stz.w !wram_10D8
.lED6C:
    rts

; @routine 02:E7E8 LevelColumnsSet4
; WHAT IT DOES: Stores A in the four bytes of $0F05 + X (a level column
; group).
; HOW: Four STA.W abs,X, as wide as M.
; Entry: M 8-bit; X = the offset; data bank $7E or a mirror. Exit: all
; unchanged.
%at($02E7E8)
LevelColumnsSet4:
    sta.w !level_columns,x
    sta.w !level_columns+1,x
    sta.w !level_columns+2,x
    sta.w !level_columns+3,x
    rts

; @routine 02:E7F5 SpeedCourseSelectStep
; WHAT IT DOES: The Speed Trax course select's frame: the level columns
; for groups 0-2 by the save's opened bits ($70:FC0B bits 0-2: 0 open, 4
; closed; group 0 open with bit 0 set, groups 1 and 2 with theirs) drawn
; (TilemapLevelColumns); once the fade is full ($0D61 = $0F), on page 0
; ($0D63 = 0) the scroll $0E37 runs up to $50 by 4, on page 1 down to 0;
; there, the cursor's rows: Up ($0800) and Down ($0400) move it between
; the open groups (sound $45, $0D4A, and $0D63 down on page 1's top), and
; otherwise Start or A zooms into the course and the cursor moves.
; HOW: 8-bit; the pads' word 16-bit.
; Entry: M 8-bit, X 8-bit; data bank $7E or a mirror. Exit: M 8-bit; as
; the last step leaves it.
%at($02E7F5)
SpeedCourseSelectStep:
    lda.l !save_progress
    bit.b #$01
    bne .lE801
    lda.b #$04
    bra .lE803
.lE801:
    lda.b #$00
.lE803:
    ldx.b #$00
    jsr.w LevelColumnsSet4
    lda.l !save_progress
    bit.b #$02
    beq .lE814
    lda.b #$00
    bra .lE816
.lE814:
    lda.b #$04
.lE816:
    ldx.b #$04
    jsr.w LevelColumnsSet4
    lda.l !save_progress
    bit.b #$04
    beq .lE827
    lda.b #$00
    bra .lE829
.lE827:
    lda.b #$04
.lE829:
    ldx.b #$08
    jsr.w LevelColumnsSet4
    jsr.w TilemapLevelColumns
    lda.w !fade_level_hi
    cmp.b #$0F
    beq .lE839
    rts
.lE839:
    sep.b #$20
    lda.w !wram_0D63
    bne .lE85E
    lda.w !scroll_value
    cmp.b #$50
    beq .lE87B
    inc a
    inc a
    inc a
    inc a
    cmp.b #$50
    bcc .lE851
    lda.b #$50
.lE851:
    sta.w !scroll_value
    lda.b #$F0
    sta.w !wram_0E92
    jsl.l OamBufferHideLargeLong
    rts
.lE85E:
    lda.w !scroll_value
    bne .lE866
    brl .lE8ED
.lE866:
    dec a
    dec a
    dec a
    dec a
    bpl .lE86E
    lda.b #$00
.lE86E:
    sta.w !scroll_value
    lda.b #$F0
    sta.w !wram_0E92
    jsl.l OamBufferHideLargeLong
    rts
.lE87B:
    jsr.w FlashingMetaspriteDrawByMode
    lda.w !wram_0D4A
    bne .lE8A8
    sep.b #$20
    lda.l !save_progress
    bit.b #$02
    beq .lE899
    rep.b #$20
    lda.w !joy1_pressed_copy
    bit.w #$0800
    sep.b #$20
    bne .lE89C
.lE899:
    jmp.w .lE954
.lE89C:
    lda.b #$45
    jsl.l SoundPort0QueuePush
    lda.b #$01
    sta.w !wram_0D4A
    rts
.lE8A8:
    sep.b #$20
    lda.l !save_progress
    bit.b #$04
    beq .lE8CD
    rep.b #$20
    lda.w !joy1_pressed_copy
    bit.w #$0800
    sep.b #$20
    beq .lE8CD
    lda.b #$45
    jsl.l SoundPort0QueuePush
    dec.w !wram_0D63
    lda.b #$02
    sta.w !wram_0D4A
    rts
.lE8CD:
    sep.b #$20
    lda.l !save_progress
    bit.b #$01
    beq .lE954
    rep.b #$20
    lda.w !joy1_pressed_copy
    bit.w #$0400
    sep.b #$20
    beq .lE954
    lda.b #$45
    jsl.l SoundPort0QueuePush
    stz.w !wram_0D4A
    rts
.lE8ED:
    jsr.w FlashingMetaspriteDrawByMode
    lda.w !wram_0D4A
    cmp.b #$01
    bne .lE93C
    rep.b #$20
    lda.w !joy1_pressed_copy
    bit.w #$0400
    sep.b #$20
    beq .lE91A
    sep.b #$20
    lda.l !save_progress
    bit.b #$01
    beq .lE91A
    lda.b #$45
    jsl.l SoundPort0QueuePush
    stz.w !wram_0D63
    stz.w !wram_0D4A
    rts
.lE91A:
    rep.b #$20
    lda.w !joy1_pressed_copy
    bit.w #$0800
    sep.b #$20
    beq .lE954
    sep.b #$20
    lda.l !save_progress
    bit.b #$04
    beq .lE954
    lda.b #$45
    jsl.l SoundPort0QueuePush
    lda.b #$02
    sta.w !wram_0D4A
    rts
.lE93C:
    rep.b #$20
    lda.w !joy1_pressed_copy
    bit.w #$0400
    sep.b #$20
    beq .lE954
    lda.b #$45
    jsl.l SoundPort0QueuePush
    lda.b #$01
    sta.w !wram_0D4A
    rts
.lE954:
    lda.w !wram_0D63
    bne .lE981
    rep.b #$20
    lda.w !joy1_pressed_copy
    bit.w #$9000
    beq .lE968
    jsl.l CourseZoomFromSelection
    rts
.lE968:
    sep.b #$20
    lda.w !wram_0D4A
    bne .lE978
    lda.b #$B0
    sta.w !flash_sprite_y
    jsr.w MenuCursorFourWay
    rts
.lE978:
    lda.b #$60
    sta.w !flash_sprite_y
    jsr.w MenuCursorFourWay
    rts
.lE981:
    rep.b #$20
    lda.w !joy1_pressed_copy
    bit.w #$9000
    beq .lE990
    jsl.l CourseZoomFromSelection4
    rts
.lE990:
    sep.b #$20
    lda.w !wram_0D4A
    cmp.b #$01
    bne .lE9A2
    lda.b #$B0
    sta.w !flash_sprite_y
    jsr.w MenuCursorFourWay
    rts
.lE9A2:
    lda.b #$60
    sta.w !flash_sprite_y
    jsr.w MenuCursorFourWay
    rts

; @routine 02:E9AB StuntCourseSelectStep
; WHAT IT DOES: The Stunt Trax course select's frame, once the fade is
; full ($0D61 = $0F): on page 0 ($0D63 = 0) the scroll $0E37 runs up to
; $50 by 4, then the course's picture setting (Set0D37FromTable), the
; flashing sprite, and Start or A zooms in (CourseZoomStunt), otherwise
; the cursor (RaceFadeStageDispatch); on page 1 (which no run shows) the
; scroll runs down to 0, then TimeUpSelectDraw, the flashing sprite, and
; Start or A zooms by $0D4E (CourseZoomTimeUp or CourseZoomTimeUpNext),
; otherwise TimeUpCursorStep.
; HOW: 8-bit; the pads' word 16-bit.
; Entry: M 8-bit, X 8-bit; data bank $7E or a mirror. Exit: M 8-bit, or
; 16-bit after a zoom; as the last step leaves it.
%at($02E9AB)
StuntCourseSelectStep:
    lda.w !fade_level_hi
    cmp.b #$0F
    beq .lE9B3
    rts
.lE9B3:
    lda.w !wram_0D63
    bne .lE9D6
    lda.w !scroll_value
    cmp.b #$50
    beq .lEA17
    inc a
    inc a
    inc a
    inc a
    cmp.b #$50
    bcc .lE9C9
    lda.b #$50
.lE9C9:
    sta.w !scroll_value
    lda.b #$F0
    sta.w !wram_0E92
    jsl.l OamBufferHideLargeLong
    rts
.lE9D6:
    lda.w !scroll_value
    beq .lE9F0
    dec a
    dec a
    dec a
    dec a
    bpl .lE9E3
    lda.b #$00
.lE9E3:
    sta.w !scroll_value
    lda.b #$F0
    sta.w !wram_0E92
    jsl.l OamBufferHideLargeLong
    rts
.lE9F0:
    jsr.w TimeUpSelectDraw
    jsr.w FlashingMetaspriteDraw
    rep.b #$20
    lda.w !joy1_pressed_copy
    bit.w #$9000
    beq .lEA11
    sep.b #$20
    lda.w !hud_blank
    bne .lEA0C
    jsl.l CourseZoomTimeUp
    rts
.lEA0C:
    jsl.l CourseZoomTimeUpNext
    rts
.lEA11:
    sep.b #$20
    jsr.w TimeUpCursorStep
    rts
.lEA17:
    jsr.w Set0D37FromTable
    jsr.w FlashingMetaspriteDraw
    rep.b #$20
    lda.w !joy1_pressed_copy
    bit.w #$9000
    beq .lEA2C
    jsl.l CourseZoomStunt
    rts
.lEA2C:
    sep.b #$20
    jsr.w RaceFadeStageDispatch
    rts

; @routine 02:EA32 BattleCourseSelectStep
; WHAT IT DOES: The Battle Trax course select's frame, once the fade is
; full ($0D61 = $0F): the course's picture setting (Set0D37FromTable),
; the flashing sprite, and Start or A zooms in (CourseZoomBattle),
; otherwise the cursor (CourseCursorStep).
; HOW: The pads' word 16-bit.
; Entry: M 8-bit; data bank $7E or a mirror. Exit: M 16-bit; as the last
; step leaves it.
%at($02EA32)
BattleCourseSelectStep:
    lda.w !fade_level_hi
    cmp.b #$0F
    beq .lEA3A
    rts
.lEA3A:
    jsr.w Set0D37FromTable
    jsr.w FlashingMetaspriteDraw
    rep.b #$20
    lda.w !joy1_pressed_copy
    bit.w #$9000
    beq .lEA4F
    jsl.l CourseZoomBattle
    rts
.lEA4F:
    jsr.w CourseCursorStep
    rts

; @routine 02:EB6D TimeUpCursorStep
; WHAT IT DOES: The time-up course select's cursor: with $0D4E set, Down
; ($0400) clears it and puts the cursor $0D48 at 2; otherwise Up ($0800)
; sets $0D4E to $FF; with the cursor at 2, Down puts it at 0 and Right
; ($0100) at 3 ($0D63 cleared with Down); elsewhere Down puts it at 1
; ($0D63 cleared) and Left ($0200) at 2. Each move with sound $45.
; HOW: The pads' word 16-bit, the stores 8-bit.
; Entry: M 8-bit; data bank $7E or a mirror. Exit: M 8-bit after a move,
; else 16-bit; A as the last step leaves it.
%at($02EB6D)
TimeUpCursorStep:
    lda.w !hud_blank
    beq .free
    rep.b #$20
    lda.w !joy1_pressed_copy
    bit.w #$0400
    beq .done_set
    sep.b #$20
    lda.b #$45
    jsl.l SoundPort0QueuePush
    stz.w !hud_blank
    lda.b #$02
    sta.w !wram_0D48
.done_set:
    rts
.free:
    rep.b #$20
    lda.w !joy1_pressed_copy
    bit.w #$0800
    beq .rows
    sep.b #$20
    lda.b #$45
    jsl.l SoundPort0QueuePush
    lda.b #$FF
    sta.w !hud_blank
    rts
.rows:
    sep.b #$20
    lda.w !wram_0D48
    cmp.b #$02
    beq .at2
    rep.b #$20
    lda.w !joy1_pressed_copy
    bit.w #$0400
    beq .left
    sep.b #$20
    lda.b #$45
    jsl.l SoundPort0QueuePush
    stz.w !wram_0D63
    lda.b #$01
    sta.w !wram_0D48
    rts
.left:
    lda.w !joy1_pressed_copy
    bit.w #$0200
    beq .done
    sep.b #$20
    lda.b #$45
    jsl.l SoundPort0QueuePush
    lda.b #$02
    sta.w !wram_0D48
    rts
.at2:
    rep.b #$20
    lda.w !joy1_pressed_copy
    bit.w #$0400
    beq .right
    sep.b #$20
    lda.b #$45
    jsl.l SoundPort0QueuePush
    stz.w !wram_0D63
    lda.b #$00
    sta.w !wram_0D48
    rts
.right:
    lda.w !joy1_pressed_copy
    bit.w #$0100
    beq .done
    sep.b #$20
    lda.b #$45
    jsl.l SoundPort0QueuePush
    lda.b #$03
    sta.w !wram_0D48
.done:
    rts

; @routine 02:FC60 TimeUpSelectDraw
; WHAT IT DOES: The time-up course select's flashing sprite position
; $0D37: ($80, $60) with $0D4E set; else ($48, $B0) with the cursor $0D48
; at 2 and ($B8, $B0) otherwise.
; HOW: The bytes tested with an 8-bit X; a 16-bit store; P saved and
; restored.
; Entry: any widths; data bank $7E or a mirror. Exit: P restored; A = the
; position word; X = the last byte tested.
%at($02FC60)
TimeUpSelectDraw:
    php
    sep.b #$10
    rep.b #$20
    ldx.w !hud_blank
    bne .blank
    ldx.w !wram_0D48
    cpx.b #$02
    beq .left
    lda.w #$B0B8
    bra .store
.left:
    lda.w #$B048
    bra .store
.blank:
    lda.w #$6080
.store:
    sta.w !wram_0D37
    plp
    rts

; @routine 03:FADB CourseZoomTimeUpNext
; WHAT IT DOES: The time-up select's zoom with $0D4E set: the course
; $0E3D stepped on within $17-$1A ($17 below or past them), then the zoom
; from box $A0 to target 8.
; HOW: 16-bit; a jump to CourseZoomBattle's zoom.
; Entry: any widths. Exit: as CourseZoomStunt.
%at($03FADB)
CourseZoomTimeUpNext:
    rep.b #$30
    lda.w !course
    cmp.w #$0017
    bcc .first
    inc a
    cmp.w #$001B
    bcc .store
.first:
    lda.w #$0017
.store:
    sta.w !course
    lda.w #$0000
    clc
    adc.w #$00A0
    tax
    ldy.w #$0008
    jmp.w CourseZoomBattle_zoom

; @routine 03:FAFF CourseZoomTimeUp
; WHAT IT DOES: As CourseZoomStunt for the time-up select: the course
; $0E3D from the cursor $0D48 in the list at $03:FB86, then the zoom from
; box ($0D48 - 2) * 8 + $80 to target 8.
; HOW: As CourseZoomStunt (BRA to CourseZoomBattle's zoom).
; Entry: any widths. Exit: as CourseZoomStunt.
%at($03FAFF)
CourseZoomTimeUp:
    rep.b #$30
    ldx.w !wram_0D48
    lda.l !course_list_stunt,x
    and.w #$00FF
    sta.w !course
    and.w #$00FF
    lda.w !wram_0D48
    sec
    sbc.w #$0002
    asl a
    asl a
    asl a
    clc
    adc.w #$0080
    tax
    ldy.w #$0008
    bra CourseZoomBattle_zoom

; @routine 02:E60D CourseRecordsKey
; WHAT IT DOES: The course select's records key: $0E91 = $30; with the
; records panel already up ($0D44) or Start or A pressed (sound $46), the
; course's totals unpacked (RecordsUnpackCourseTotals), $0D44 = $0D43 = 2,
; $0D42 = $1E, and a return through its caller's frame (its own return
; address pulled); otherwise the flashing sprite of set 0
; (FlashingMetaspriteDraw0).
; HOW: 8-bit; PLA, PLA, RTS to leave.
; Entry: M 8-bit; data bank $7E or a mirror. Exit: as
; FlashingMetaspriteDraw0; or at the caller's return with A low byte its
; return address's high byte.
%at($02E60D)
CourseRecordsKey:
    sep.b #$20
    lda.b #$30
    sta.w !wram_0E91
    lda.w !wram_0D44
    bne .lE62B
    rep.b #$20
    lda.w !joy1_pressed_copy
    bit.w #$9000
    sep.b #$20
    beq .lE63F
    lda.b #$46
    jsl.l SoundPort0QueuePush
.lE62B:
    jsl.l RecordsUnpackCourseTotals
    lda.b #$02
    sta.w !wram_0D44
    sta.w !wram_0D3F+4
    lda.b #$1E
    sta.w !wram_0D3F+3
    pla
    pla
    rts
.lE63F:
    lda.b #$00
    sta.w !metasprite_set_sel
    jsr.w FlashingMetaspriteDraw0
    rts

; @routine 02:E648 CourseRecordsSelectStep
; WHAT IT DOES: The course select's frame where the records can be shown
; (state 0 of $0D43): on page 0 ($0D63 = 0) the scroll $0E37 runs up to
; $50 by 4, on page 1 down to 0; there, the records key
; (CourseRecordsKey, which leaves when it opens them), the flashing
; sprite at ($80, $B0) on the cursor's first row and ($80, $60) on the
; other, and Up ($0800) and Down ($0400) move the cursor $0D4A (on page
; 0's second row Up, with group 1 open, also pages down) with sound $45.
; HOW: 8-bit; the pads' word 16-bit.
; Entry: any widths; data bank $7E or a mirror. Exit: M and X 8-bit; as
; the last step leaves it.
%at($02E648)
CourseRecordsSelectStep:
    rep.b #$30
    sep.b #$30
    lda.w !wram_0D63
    bne .lE666
    lda.w !scroll_value
    cmp.b #$50
    beq .lE67A
    inc a
    inc a
    inc a
    inc a
    cmp.b #$50
    bcc .lE662
    lda.b #$50
.lE662:
    sta.w !scroll_value
    rts
.lE666:
    lda.w !scroll_value
    bne .lE66E
    jmp.w .lE6E9
.lE66E:
    dec a
    dec a
    dec a
    dec a
    bpl .lE676
    lda.b #$00
.lE676:
    sta.w !scroll_value
    rts
.lE67A:
    jsr.w CourseRecordsKey
    lda.w !wram_0D4A
    bne .lE6A4
    lda.b #$80
    sta.w !wram_0D37
    lda.b #$B0
    sta.w !flash_sprite_y
    rep.b #$20
    lda.w !joy1_pressed_copy
    bit.w #$0800
    sep.b #$20
    beq .lE6A3
    lda.b #$45
    jsl.l SoundPort0QueuePush
    lda.b #$01
    sta.w !wram_0D4A
.lE6A3:
    rts
.lE6A4:
    lda.b #$80
    sta.w !wram_0D37
    lda.b #$60
    sta.w !flash_sprite_y
    sep.b #$20
    lda.l !save_progress
    bit.b #$02
    beq .lE6D3
    rep.b #$20
    lda.w !joy1_pressed_copy
    bit.w #$0800
    sep.b #$20
    beq .lE6D3
    lda.b #$45
    jsl.l SoundPort0QueuePush
    dec.w !wram_0D63
    lda.b #$02
    sta.w !wram_0D4A
    rts
.lE6D3:
    rep.b #$20
    lda.w !joy1_pressed_copy
    bit.w #$0400
    sep.b #$20
    beq .lE6A3
    lda.b #$45
    jsl.l SoundPort0QueuePush
    stz.w !wram_0D4A
.lE6E8:
    rts
.lE6E9:
    jsr.w CourseRecordsKey
    lda.w !wram_0D4A
    cmp.b #$01
    bne .lE72E
    lda.b #$80
    sta.w !wram_0D37
    lda.b #$B0
    sta.w !flash_sprite_y
    rep.b #$20
    lda.w !joy1_pressed_copy
    bit.w #$0400
    sep.b #$20
    beq .lE716
    lda.b #$45
    jsl.l SoundPort0QueuePush
    stz.w !wram_0D63
    stz.w !wram_0D4A
    rts
.lE716:
    rep.b #$20
    lda.w !joy1_pressed_copy
    bit.w #$0800
    sep.b #$20
    beq .lE6E8
    lda.b #$45
    jsl.l SoundPort0QueuePush
    lda.b #$02
    sta.w !wram_0D4A
    rts
.lE72E:
    lda.b #$80
    sta.w !wram_0D37
    lda.b #$60
    sta.w !flash_sprite_y
    rep.b #$20
    lda.w !joy1_pressed_copy
    bit.w #$0400
    sep.b #$20
    beq .lE74F
    lda.b #$45
    jsl.l SoundPort0QueuePush
    lda.b #$01
    sta.w !wram_0D4A
.lE74F:
    rts

; @routine 02:E750 CourseRecordsOpening
; WHAT IT DOES: The records panel opening (state 2 of $0D43): counts
; $0D42 down; at 0, $0D42 = $2A, state 4 and, unless $0D4C is set, sound
; $2D.
; HOW: 8-bit through the data bank.
; Entry: M 8-bit; data bank $7E or a mirror. Exit: A low byte as the last
; load leaves it; N, Z from it.
%at($02E750)
CourseRecordsOpening:
    dec.w !wram_0D3F+3
    bne .lE76A
    lda.b #$2A
    sta.w !wram_0D3F+3
    lda.b #$04
    sta.w !wram_0D3F+4
    lda.w !digit62_override
    bne .lE76A
    lda.b #$2D
    jsl.l SoundPort0QueuePush
.lE76A:
    rts

; @routine 02:E76B CourseRecordsReopen
; WHAT IT DOES: State 8 of $0D43 (no run reaches it): counts $0D42 up; at
; $2A, $0D44 = 2 and state 0.
; HOW: 8-bit through the data bank.
; Entry: M 8-bit; data bank $7E or a mirror. Exit: N, Z, C from the
; compare with $2A, or A low byte 0.
%at($02E76B)
CourseRecordsReopen:
    lda.w !wram_0D3F+3
    inc a
    sta.w !wram_0D3F+3
    cmp.b #$2A
    bcc .lE780
    lda.b #$02
    sta.w !wram_0D44
    lda.b #$00
    sta.w !wram_0D3F+4
.lE780:
    rts

; @routine 02:E781 CourseRecordsClosing
; WHAT IT DOES: The records panel closing (state 6 of $0D43): counts $0D42
; up; at $2A, state 0 and $0D44 = 0; otherwise the cursor's level column
; pulses with the count (as CourseRecordsShown's).
; HOW: 8-bit through the data bank; the pulse is a branch into
; CourseRecordsShown.
; Entry: M 8-bit, X 8-bit; data bank $7E or a mirror. Exit: A low byte 0
; at the end, else the column's level.
%at($02E781)
CourseRecordsClosing:
    lda.w !wram_0D3F+3
    inc a
    sta.w !wram_0D3F+3
    cmp.b #$2A
    bcc CourseRecordsPulse
    lda.b #$00
    sta.w !wram_0D3F+4
    sta.w !wram_0D44
    rts

; @routine 02:E795 CourseRecordsShown
; WHAT IT DOES: The records panel up (state 4 of $0D43): counts $0D42
; down; at 0, $0D40 = 2 and the zoom into the course
; (CourseZoomFromSelection, or CourseZoomFromSelection4 on page 1);
; otherwise, unless $0D4C is set, the cursor's level column ($0F05 +
; $0D4A * 4 + $0D48) pulses with the count: 0 below $10, 1 to $17, 2 to
; $1F, 3 at $20-$21, 4 from $22.
; HOW: 8-bit through the data bank; the column by a long store.
; Entry: M 8-bit, X 8-bit; data bank $7E or a mirror. Exit: as the zoom
; leaves it, or A low byte the column's level.
%at($02E795)
CourseRecordsShown:
    dec.w !wram_0D3F+3
    bne .lE7AE
    lda.b #$02
    sta.w !wram_0D3F+1
    lda.w !wram_0D63
    bne .lE7A9
    jsl.l CourseZoomFromSelection
    rts
.lE7A9:
    jsl.l CourseZoomFromSelection4
    rts
.lE7AE:
    lda.w !digit62_override
    beq CourseRecordsPulse
    rts
CourseRecordsPulse:                     ; the level column pulse (CourseRecordsClosing too)
    lda.w !wram_0D4A
    asl a
    asl a
    clc
    adc.w !wram_0D48
    tax
    lda.w !wram_0D3F+3
    cmp.b #$22
    bcs .lE7E1
    cmp.b #$20
    bcs .lE7DD
    cmp.b #$18
    bcs .lE7D9
    cmp.b #$10
    bcs .lE7D5
    lda.b #$00
    bra .lE7E3
.lE7D5:
    lda.b #$01
    bra .lE7E3
.lE7D9:
    lda.b #$02
    bra .lE7E3
.lE7DD:
    lda.b #$03
    bra .lE7E3
.lE7E1:
    lda.b #$04
.lE7E3:
    sta.l !level_columns,x
    rts

; @routine 02:E356 CourseScreenStep
; WHAT IT DOES: The course screens' frame by HUD mode $0D2B, each running
; its state $0D40 from its table: mode 0 Speed Trax ($02:E3E1; with all
; courses opened, the all-courses table), 1 Stunt Trax ($02:E3FE), 2
; Battle Trax ($02:E493), 3 the all-courses select ($02:E4A7). In mode 0
; player 2 holding L ($0020) with nothing else and pressing Select seven
; times opens every course: $0D24 = 3, the level columns cleared, sound
; $36, the save's course bytes $0F and $0F and its progress 7
; (SaveChecksum); another button clears the count ($0D25).
; HOW: JMP (abs,X) by mode, then by state; 8-bit, the pads' words
; 16-bit.
; Entry: any widths; data bank $7E or a mirror. Exit: as the state's step
; leaves it.
%at($02E356)
CourseScreenStep:
    sep.b #$30
    lda.w !hud_mode
    asl a
    tax
    jmp.w (.modes,x)
.modes:
    dw .mode0,CourseScreenStep_stunt,CourseScreenStep_battle,CourseScreenStep_all
.mode0:
    sep.b #$20
    lda.w !wram_0D24
    cmp.b #$03
    beq .lE3CF
    rep.b #$20
    lda.w !joy2_held
    and.w #$DFD0
    bne .lE39E
    lda.w !joy2_pressed_copy
    bit.w #$2000
    beq .lE3CF
    lda.w !joy2_held
    bit.w #$0020
    beq .lE39E
    sep.b #$20
    inc.w !wram_0D25
    lda.w !wram_0D25
    cmp.b #$07
    bcc .lE3CF
    lda.b #$03
    sta.w !wram_0D24
    bra .lE3A8
.lE39E:
    sep.b #$20
    stz.w !wram_0D24
    stz.w !wram_0D25
    bra .lE3CF
.lE3A8:
    ldx.b #$0B
    lda.b #$00
.lE3AC:
    sta.l !level_columns,x
    dex
    bpl .lE3AC
    lda.b #$36
    jsl.l SoundPort0QueuePush
    sep.b #$30
    lda.b #$0F
    sta.l !save_bytes
    sta.l !save_bytes+1
    lda.b #$07
    sta.l !save_progress
    jsl.l SaveChecksum
.lE3CF:
    sep.b #$30
    lda.w !wram_0D24
    cmp.b #$03
    bne .speed
    jmp.w CourseScreenStep_all
.speed:
    ldx.w !wram_0D3F+1
    jmp.w (.speed_states,x)
.speed_states:
    dw CourseScreenStartSpeed,CourseRecordsDispatch,CourseZoomInStep,$EDD2,$ED98,CourseScreenWindowStep,CourseScreenNothing,$ED75

; @routine 02:E3F1 CourseScreenWindowStep
; WHAT IT DOES: The course screens' state 10: the window's edges
; (WindowEdgesStep).
; HOW: JSL, RTS.
; Entry: as WindowEdgesStep. Exit: as WindowEdgesStep.
%at($02E3F1)
CourseScreenWindowStep:
    jsl.l WindowEdgesStep
    rts
CourseScreenStep_stunt:
    sep.b #$30
    ldx.w !wram_0D3F+1
    jmp.w (.states,x)
.states:
    dw CourseScreenStartStunt,StuntCourseSelectStep,CourseZoomInStep,$F148,$ED98,CourseScreenWindowStep
    dw CourseScreenZoomOut,CourseScreenWipeOut,CoursePreviewRowsStunt,CourseScreenWipeIn,CourseScreenZoomIn

; @routine 02:E414 CourseScreenWipeOut
; WHAT IT DOES: State 14 (Stunt Trax and the all-courses select): $0D45 =
; 6, the scroll HDMA's first count ($10AD) down by 4 and the next two
; ($10B0, $10B3) up by 4; once the first would pass 0 it is 1, $10B3 goes
; up by 2 and state $10 follows.
; HOW: 8-bit through the data bank; the adds chain the carry.
; Entry: any widths; data bank $7E or a mirror. Exit: M 8-bit; A as the
; last store leaves it.
%at($02E414)
CourseScreenWipeOut:
    sep.b #$20
    lda.b #$06
    sta.w !wram_0D45
    lda.w !scroll_hdma+3
    sec
    sbc.b #$04
    bcc .lE438
    sta.w !scroll_hdma+3
    lda.w !scroll_hdma+6
    clc
    adc.b #$04
    sta.w !scroll_hdma+6
    lda.w !scroll_hdma+9
    adc.b #$04
    sta.w !scroll_hdma+9
    rts
.lE438:
    lda.b #$01
    sta.w !scroll_hdma+3
    inc.w !scroll_hdma+9
    inc.w !scroll_hdma+9
    sep.b #$20
    lda.b #$10
    sta.w !wram_0D3F+1
    rts

; @routine 02:E44B CourseScreenWipeIn
; WHAT IT DOES: State 18: $0D45 = 6, the scroll HDMA's first count ($10AD)
; up by 4 and the next two ($10B0, $10B3) down by 4; at $4F the counts are
; $4F, 1 and 1, state $14 and $0D45 = 2.
; HOW: 8-bit through the data bank; the subtracts chain the borrow.
; Entry: any widths; data bank $7E or a mirror. Exit: M 8-bit; A as the
; last store leaves it.
%at($02E44B)
CourseScreenWipeIn:
    sep.b #$20
    lda.b #$06
    sta.w !wram_0D45
    lda.w !scroll_hdma+3
    clc
    adc.b #$04
    cmp.b #$4F
    bcs .lE471
    sta.w !scroll_hdma+3
    lda.w !scroll_hdma+6
    sec
    sbc.b #$04
    sta.w !scroll_hdma+6
    lda.w !scroll_hdma+9
    sbc.b #$04
    sta.w !scroll_hdma+9
    rts
.lE471:
    sep.b #$20
    lda.b #$4F
    sta.w !scroll_hdma+3
    lda.b #$01
    sta.w !scroll_hdma+6
    sta.w !scroll_hdma+9
    lda.b #$14
    sta.w !wram_0D3F+1
    lda.b #$02
    sta.w !wram_0D45
    rts
CourseScreenStep_battle:
    sep.b #$30
    ldx.w !wram_0D3F+1
    jmp.w (.states,x)
.states:
    dw CourseScreenStartBattle,BattleCourseSelectStep,CourseZoomInStep,$F30A,$ED98,CourseScreenWindowStep
CourseScreenStep_all:
    sep.b #$30
    ldx.w !wram_0D3F+1
    jmp.w (.states,x)
.states:
    dw CourseScreenStartAll,SpeedCourseSelectStep,CourseZoomInStep,$F37A,$ED98,CourseScreenWindowStep
    dw CourseScreenZoomOut,CourseScreenWipeOut,CoursePreviewRowsAll,CourseScreenWipeIn,CourseScreenZoomIn

; @routine 02:E4BD CoursePreviewRowsStunt
; WHAT IT DOES: Stunt Trax's state 16: any button pressed ($0D1A) leaves
; for state $12 with sound $2F; otherwise, until $0FAA reaches 3, the
; four preview rows (PreviewRowDrawStunt with $0FA8 = 0-3), then $0FA6
; counts to 6 and $0FAA steps.
; HOW: 16-bit through the data bank.
; Entry: any widths; data bank $7E or a mirror. Exit: M 16-bit, X 16-bit
; or M 8-bit after the sound; A as the last step leaves it.
%at($02E4BD)
CoursePreviewRowsStunt:
    rep.b #$30
    lda.w !joy1_pressed_copy
    bne .lE4F5
    lda.w !p1_lead+7
    cmp.w #$0003
    bcc .lE4CD
    rts
.lE4CD:
    stz.w !p1_lead+5
.lE4D0:
    jsl.l PreviewRowDrawStunt
    lda.w !p1_lead+5
    inc a
    cmp.w #$0004
    bcs .lE4E2
    sta.w !p1_lead+5
    bra .lE4D0
.lE4E2:
    lda.w !p1_lead+3
    cmp.w #$0006
    bcs .lE4EE
    inc.w !p1_lead+3
    rts
.lE4EE:
    stz.w !p1_lead+3
    inc.w !p1_lead+7
    rts
.lE4F5:
    sep.b #$20
    lda.b #$12
    sta.w !wram_0D3F+1
    lda.b #$2F
    jsl.l SoundPort0QueuePush
    rts

; @routine 02:E503 CoursePreviewReset
; WHAT IT DOES: The preview's counts $0FAA and $0FA6 cleared and $800
; bytes copied from $7F:C3D7 to $7F:9BD7.
; HOW: MVN, with P and the data bank kept (MVN leaves the data bank $7F).
; Entry: any widths. Exit: P and the data bank unchanged; A = $FFFF, X =
; $CBD7, Y = $A3D7.
%at($02E503)
CoursePreviewReset:
    rep.b #$20
    stz.w !p1_lead+7
    stz.w !p1_lead+3
    phb
    php
    rep.b #$30
    lda.w #$07FF
    ldx.w #$C3D7
    ldy.w #$9BD7
    mvn !tilemap_7f9bd7>>16,!tilemap_7f9bd7>>16
    plp
    plb
    rts

; @routine 02:E51E CoursePreviewRowsAll
; WHAT IT DOES: The all-courses select's state 16: as
; CoursePreviewRowsStunt with PreviewRowDrawAll and $0FA6 counting to 7.
; HOW: As CoursePreviewRowsStunt.
; Entry: as CoursePreviewRowsStunt. Exit: as CoursePreviewRowsStunt.
%at($02E51E)
CoursePreviewRowsAll:
    rep.b #$30
    lda.w !joy1_pressed_copy
    bne .lE556
    lda.w !p1_lead+7
    cmp.w #$0003
    bcc .lE52E
    rts
.lE52E:
    stz.w !p1_lead+5
.lE531:
    jsl.l PreviewRowDrawAll
    lda.w !p1_lead+5
    inc a
    cmp.w #$0004
    bcs .lE543
    sta.w !p1_lead+5
    bra .lE531
.lE543:
    lda.w !p1_lead+3
    cmp.w #$0007
    bcs .lE54F
    inc.w !p1_lead+3
    rts
.lE54F:
    stz.w !p1_lead+3
    inc.w !p1_lead+7
    rts
.lE556:
    sep.b #$20
    lda.b #$12
    sta.w !wram_0D3F+1
    lda.b #$2F
    jsl.l SoundPort0QueuePush
    rts

; @routine 02:E564 CourseScreenStartSpeed
; WHAT IT DOES: The Speed Trax course select's state 0: state 2 and $10D8
; cleared.
; HOW: 8-bit stores through the data bank.
; Entry: M 8-bit; data bank $7E or a mirror. Exit: A low byte 2.
%at($02E564)
CourseScreenStartSpeed:
    lda.b #$02
    sta.w !wram_0D3F+1
    stz.w !wram_10D8
    rts

; @routine 02:E56D CourseScreenStartAll
; WHAT IT DOES: The all-courses select's state 0: state 2.
; HOW: An 8-bit store through the data bank.
; Entry: M 8-bit; data bank $7E or a mirror. Exit: A low byte 2.
%at($02E56D)
CourseScreenStartAll:
    lda.b #$02
    sta.w !wram_0D3F+1
    rts

; @routine 02:E573 CourseScreenStartBattle
; WHAT IT DOES: The Battle Trax course select's state 0: state 2.
; HOW: As CourseScreenStartAll.
; Entry: as CourseScreenStartAll. Exit: as CourseScreenStartAll.
%at($02E573)
CourseScreenStartBattle:
    lda.b #$02
    sta.w !wram_0D3F+1
    rts

; @routine 02:E579 CourseScreenStartStunt
; WHAT IT DOES: The Stunt Trax course select's state 0: state 2.
; HOW: As CourseScreenStartAll.
; Entry: as CourseScreenStartAll. Exit: as CourseScreenStartAll.
%at($02E579)
CourseScreenStartStunt:
    lda.b #$02
    sta.w !wram_0D3F+1
    rts

; @routine 02:E57F CourseScreenNothing
; WHAT IT DOES: The Speed Trax course select's state 12, which nothing
; reaches: returns.
; HOW: RTS.
; Entry: any. Exit: all unchanged.
%at($02E57F)
CourseScreenNothing:
    rts

; @routine 02:E580 CourseScreenZoomOut
; WHAT IT DOES: State 12 (Stunt Trax and the all-courses select): the
; window's scaled words $7F:3434 up by $300 and $7F:3438 down by $300;
; once they are less than $700 apart, state $0E, colour maths off
; (ColourMathOffLong) and the preview reset (CoursePreviewReset).
; HOW: 16-bit long accesses.
; Entry: any widths. Exit: M 16-bit, X 16-bit, or M 8-bit at the end; as
; the last step leaves it.
%at($02E580)
CourseScreenZoomOut:
    rep.b #$30
    lda.l !scaled_words+2
    clc
    adc.w #$0300
    sta.l !scaled_words+2
    lda.l !scaled_words+6
    sec
    sbc.w #$0300
    sta.l !scaled_words+6
    sec
    sbc.l !scaled_words+2
    cmp.w #$0700
    bcc .lE5A5
    rts
.lE5A5:
    sep.b #$20
    lda.b #$0E
    sta.w !wram_0D3F+1
    jsl.l ColourMathOffLong
    jsr.w CoursePreviewReset
    rts

; @routine 02:E5B4 CourseScreenZoomIn
; WHAT IT DOES: State 20: colour maths $A3 (ColourMathA3Long), the scaled
; words $7F:3438 up by $300 and $7F:3434 down by $300; once $7F:3434 is
; below its target $7F:343C, both are set to their targets ($7F:343C,
; $7F:3440) and state 6 follows.
; HOW: 16-bit long accesses.
; Entry: any widths. Exit: M 16-bit, X 16-bit, or M 8-bit at the end; as
; the last step leaves it.
%at($02E5B4)
CourseScreenZoomIn:
    jsl.l ColourMathA3Long
    rep.b #$30
    lda.l !scaled_words+6
    clc
    adc.w #$0300
    sta.l !scaled_words+6
    lda.l !scaled_words+2
    sec
    sbc.w #$0300
    sta.l !scaled_words+2
    cmp.l !scaled_words_target+2
    bcs .lE5EF
    lda.l !scaled_words_target+2
    sta.l !scaled_words+2
    lda.l !scaled_words_target+6
    sta.l !scaled_words+6
    sep.b #$20
    lda.b #$06
    sta.w !wram_0D3F+1
.lE5EF:
    rts

; @routine 02:E5F0 CourseRecordsDispatch
; WHAT IT DOES: The Speed Trax course select's state 2: the level columns
; drawn (TilemapLevelColumns); once the fade is full ($0D61 = $0F), the
; records panel's state $0D43 from the table at $02:E603
; (CourseRecordsSelectStep, CourseRecordsOpening, CourseRecordsShown,
; CourseRecordsClosing, CourseRecordsReopen).
; HOW: JMP (abs,X) with X 8-bit as the caller left it.
; Entry: X 8-bit; data bank $7E or a mirror. Exit: as the state's step
; leaves it.
%at($02E5F0)
CourseRecordsDispatch:
    jsr.w TilemapLevelColumns
    sep.b #$20
    lda.w !fade_level_hi
    cmp.b #$0F
    beq .lE5FD
    rts
.lE5FD:
    ldx.w !wram_0D3F+4
    jmp.w (.states,x)
.states:
    dw CourseRecordsSelectStep,CourseRecordsOpening,CourseRecordsShown,CourseRecordsClosing,CourseRecordsReopen
