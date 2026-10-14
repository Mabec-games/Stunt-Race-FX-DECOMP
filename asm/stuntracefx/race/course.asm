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
