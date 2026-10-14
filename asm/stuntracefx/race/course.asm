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
