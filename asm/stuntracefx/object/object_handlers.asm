; object_handlers.asm: object handlers of bank $08 (bursts of pieces, a
; blinking child, the Speed Trax pace car and its signal).
; C: src/stuntracefx/object/object_handlers.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 08:EC9C ObjectBurstSpawn
; WHAT IT DOES: Object X bursts into four pieces: new objects of kinds
; $A5F0, $A60C, $A628 and $A644 at its position, each with handler $08:ED39
; (ObjectBurstPieceStart).
; HOW: ObjectSpawn and ObjectCopyPositionTo for each; a failed spawn skips
; its piece.
; Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; Y = the last piece
; (or as ObjectSpawn left it); A and flags as the last call left them.
%at($08EC9C)
ObjectBurstSpawn:
    rep.b #$20
    lda.w #$A5F0
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .lECB0
    jml.l .lECC3                    ; not reached in the traces so far
.lECB0:
    rep.b #$20
    lda.w #$ED39
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,y
    jsl.l ObjectCopyPositionTo
.lECC3:
    rep.b #$20
    lda.w #$A60C
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .lECD7
    jml.l .lECEA                    ; not reached in the traces so far
.lECD7:
    rep.b #$20
    lda.w #$ED39
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,y
    jsl.l ObjectCopyPositionTo
.lECEA:
    rep.b #$20
    lda.w #$A628
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .lECFE
    jml.l .lED11                    ; not reached in the traces so far
.lECFE:
    rep.b #$20
    lda.w #$ED39
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,y
    jsl.l ObjectCopyPositionTo
.lED11:
    rep.b #$20
    lda.w #$A644
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .lED25
    jml.l .lED38                    ; not reached in the traces so far
.lED25:
    rep.b #$20
    lda.w #$ED39
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,y
    jsl.l ObjectCopyPositionTo
.lED38:
    rtl

; @routine 08:ED39 ObjectBurstPieceStart
; WHAT IT DOES: A burst piece's first frame: handler $08:EDB6
; (ObjectBurstPieceStep), a random velocity (X and Z -$1F to +$20, Y up
; -$1E to -$5D), count +$48 = $28, random +$15 and +$14, a Super FX record
; (GsuRecordNew426) given the velocity (GsuRecordVelocity); then its first
; step.
; HOW: Each random part is RandomStepLong & $3F stored to the low byte and
; then adjusted as a word (the high byte as it was). Falls into
; ObjectBurstPieceStep.
; Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectBurstPieceStep.
%at($08ED39)
ObjectBurstPieceStart:
    rep.b #$20
    lda.w #$EDB6
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,x
    jsl.l RandomStepLong
    and.b #$3F
    sta.w !obj_vel_x,x
    rep.b #$20
    lda.w !obj_vel_x,x
    sec
    sbc.w #$001F
    sta.w !obj_vel_x,x
    sep.b #$20
    jsl.l RandomStepLong
    and.b #$3F
    sta.w !obj_vel_y,x
    rep.b #$20
    lda.w !obj_vel_y,x
    clc
    adc.w #$001E
    sta.w !obj_vel_y,x
    sep.b #$20
    rep.b #$20
    lda.w !obj_vel_y,x
    eor.w #$FFFF
    inc a
    sta.w !obj_vel_y,x
    sep.b #$20
    jsl.l RandomStepLong
    and.b #$3F
    sta.w !obj_vel_z,x
    rep.b #$20
    lda.w !obj_vel_z,x
    sec
    sbc.w #$001F
    sta.w !obj_vel_z,x
    sep.b #$20
    lda.b #$28
    sta.w !obj_48,x
    jsl.l RandomStepLong
    sta.w !obj_15,x
    jsl.l RandomStepLong
    sta.w !obj_14,x
    jsl.l GsuRecordNew426
    jsl.l GsuRecordVelocity

; @routine 08:EDB6 ObjectBurstPieceStep
; WHAT IT DOES: A burst piece's frame: synced from its Super FX record
; (ObjectSyncFromRecord); released when that says so (C set), or after
; the code at $08:D337 when the record's contact summary has bit 7 and
; the sync's bit 0 is clear; otherwise released once its count +$48 runs
; out, else counted down.
; HOW: The releases are jumps (ObjectReleaseLong; $08:D337 as original
; code).
; Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; A low byte the
; count; as the release leaves it otherwise.
%at($08EDB6)
ObjectBurstPieceStep:
    jsl.l ObjectSyncFromRecord
    bcc .lEDBF
    brl ObjectReleaseLong           ; not reached in the traces so far
.lEDBF:
    and.b #$01
    bne .lEDCE
    jsl.l RecordContactSummary
    and.b #$80
    beq .lEDCE
    jmp.w ObjectReleaseD337         ; not reached in the traces so far
.lEDCE:
    lda.w !obj_48,x
    bne .lEDD7
    jml.l ObjectReleaseLong
.lEDD7:
    dec.w !obj_48,x
    rtl

; @routine 08:EDDB ObjectHandlerEmptyA
; WHAT IT DOES: Nothing: an empty object handler.
; HOW: RTL. Not reached in the traces so far.
; Entry: any. Exit: every register unchanged.
%at($08EDDB)
ObjectHandlerEmptyA:
    rtl                             ; not reached in the traces so far

; @routine 08:EDDC ObjectHandlerEmptyB
; WHAT IT DOES: Nothing: an empty object handler.
; HOW: RTL. Not reached in the traces so far.
; Entry: any. Exit: every register unchanged.
%at($08EDDC)
ObjectHandlerEmptyB:
    rtl                             ; not reached in the traces so far

; @routine 08:EDDD ObjectBlinkerStart
; WHAT IT DOES: Makes object X a blinker: a child object of kind $A91C at
; its position (its +$06, put in its bucket), handler $08:EE63
; (ObjectBlinkerRise), count +$48 = $32, colour +$5A = $8F, +$20 =
; $D354, a Super FX record (GsuRecordNew422) at $02D0, $0190, $02D0 with
; flags $4000; then the first phase at once. With no record the handler
; just returns ($08:A033).
; HOW: Falls into ObjectBlinkerRise; a failed spawn skips the child.
; Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectBlinkerRise.
%at($08EDDD)
ObjectBlinkerStart:
    rep.b #$20
    lda.w #$A91C
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .lEDF1
    jml.l .lEE09                    ; not reached in the traces so far
.lEDF1:
    jsl.l ObjectCopyPositionTo
    pha
    rep.b #$20
    tya
    sta.w !obj_06,x
    sep.b #$20
    pla
    phx
    tyx
    ply
    jsl.l ObjectRebucket
    phx
    tyx
    ply
.lEE09:
    rep.b #$20
    lda.w #$EE63
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,x
    lda.b #$32
    sta.w !obj_48,x
    lda.b #$0F
    ora.b #$80
    sta.w !obj_5a,x
    rep.b #$20
    lda.w #$D354
    sta.w !obj_20,x
    sep.b #$20
    jsl.l GsuRecordNew422
    bpl .lEE37
    brl RtlA033                     ; not reached in the traces so far
.lEE37:
    rep.b #$20
    lda.w #$02D0
    sta.b !dp_02
    sep.b #$20
    rep.b #$20
    lda.w #$0190
    sta.b !dp_08
    sep.b #$20
    rep.b #$20
    lda.w #$02D0
    sta.b !dp_94
    sep.b #$20
    jsl.l GsuRecordPosition
    rep.b #$20
    lda.w #$4000
    sta.b !dp_02
    sep.b #$20
    jsl.l GsuRecordSetFlags

; @routine 08:EE63 ObjectBlinkerRise
; WHAT IT DOES: The blinker's first phase: while its count +$48 runs, its
; record shown (+$42 = $1000) and synced (+$36 = 3); then count $28 and
; handler $08:EE9F (ObjectBlinkerFade), run at once.
; HOW: Falls into ObjectBlinkerFade.
; Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; as
; ObjectSyncFromRecord leaves it, A low byte 3.
%at($08EE63)
ObjectBlinkerRise:
    lda.w !obj_48,x
    bne .lEE6C
    jml.l ObjectBlinkerRiseEnd
.lEE6C:
    dec.w !obj_48,x
ObjectBlinkerShow:
    phx
    rep.b #$20
    lda.l !gsu_match_2476
    tax
    lda.w #$1000
    sta.l !gsu_rec_42,x
    sep.b #$20
    plx
ObjectBlinkerSync:
    jsl.l ObjectSyncFromRecord
    lda.b #$03
    sta.w !obj_36,x
    rtl
ObjectBlinkerRiseEnd:
    lda.b #$28
    sta.w !obj_48,x
    rep.b #$20
    lda.w #$EE9F
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,x

; @routine 08:EE9F ObjectBlinkerFade
; WHAT IT DOES: The blinker's second phase: while its count +$48 runs, its
; colour +$5A (low seven bits, 0-$0F) steps down one a frame with the
; record shown, and at 0 the record gets +$42 = $1010; when the count runs
; out, the record's bit 12 of +$42 is cleared and both its child (+$06,
; ObjectReleaseY) and it are released.
; HOW: The colour steps by ADC #$FF with a wrap to $10 kept in its low
; seven bits, bit 7 set; the release is a jump (ObjectReleaseLong).
; Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; as the last call
; leaves it.
%at($08EE9F)
ObjectBlinkerFade:
    lda.w !obj_48,x
    bne .lEEA8
    jml.l .lEEE5
.lEEA8:
    dec.w !obj_48,x
    lda.w !obj_5a,x
    and.b #$7F
    cmp.b #$00
    beq .lEED0
    lda.w !obj_5a,x
    clc
    adc.b #$FF
    bmi .lEEBF
    clc                             ; not reached in the traces so far
    adc.b #$10
.lEEBF:
    and.b #$7F
    cmp.b #$10
    bcc .lEEC8
    sec                             ; not reached in the traces so far
    sbc.b #$10
.lEEC8:
    ora.b #$80
    sta.w !obj_5a,x
    jmp.w ObjectBlinkerShow
.lEED0:
    phx
    rep.b #$20
    lda.l !gsu_match_2476
    tax
    lda.w #$1010
    sta.l !gsu_rec_42,x
    sep.b #$20
    plx
    jmp.w ObjectBlinkerSync
.lEEE5:
    phx
    rep.b #$20
    lda.l !gsu_match_2476
    tax
    lda.l !gsu_rec_42,x
    and.w #$EFFF
    sta.l !gsu_rec_42,x
    sep.b #$20
    plx
    ldy.w !obj_06,x
    jsl.l ObjectReleaseY
    jml.l ObjectReleaseLong

; @routine 08:EF06 ObjectPaceCarStart
; WHAT IT DOES: The Speed Trax pace car's first frame: its car set up
; (ObjectCarSpawn), handler $08:EF19 (ObjectPaceCarStep), run at once.
; HOW: Falls into ObjectPaceCarStep.
; Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectPaceCarStep.
%at($08EF06)
ObjectPaceCarStart:
    jsl.l ObjectCarSpawn
    rep.b #$20
    lda.w #$EF19
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,x

; @routine 08:EF19 ObjectPaceCarStep
; WHAT IT DOES: The Speed Trax pace car's frame: near the focus
; (ObjectNearFocus), rebucketed, posed from its record
; (ObjectPoseFromRecord). Until $1F71 is 2: once its record's Z ($70:0022)
; + $258 is past $1F73, $1F71 = 1; its record's bit 11 of +$42 is set
; while its speed ($70:004A) is below $1E, cleared otherwise; then the
; code at $08:CD7E. At $1F71 = 2: the record's +$1C gets bit 4, its +$24
; bits 5 and 3, its Y + $B1E0, and its handler cleared.
; HOW: 16-bit record words through $70:0000 + its +$22; the code at
; $08:CD7E runs as original code.
; Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; Y = the object; as
; the code at $08:CD7E leaves it, or A low byte 0 after the clear.
%at($08EF19)
ObjectPaceCarStep:
    jsl.l ObjectNearFocus
    jsl.l ObjectRebucket
    jsl.l ObjectPoseFromRecord
    phx
    rep.b #$20
    txy
    lda.w !obj_22,x
    tax
    lda.w !script_byte_1f71
    and.w #$00FF
    cmp.w #$0002
    beq .lEF73
    lda.l !gsu_in_22,x
    clc
    adc.w #$0258
    cmp.w !script_byte_1f72+1
    bmi .lEF4E
    sep.b #$20
    lda.b #$01
    sta.w !script_byte_1f71
    rep.b #$20
.lEF4E:
    lda.l !gsu_rec_42,x
    and.w #$F7FF
    sta.l !gsu_rec_42,x
    lda.l !gsu_speeds,x
    cmp.w #$001E
    bpl .lEF6D
    lda.l !gsu_rec_42,x
    ora.w #$0800
    sta.l !gsu_rec_42,x
.lEF6D:
    sep.b #$20
    plx
    jmp.w ObjectCarCD7E
.lEF73:
    lda.l !gsu_rec_1c,x
    ora.w #$0010
    sta.l !gsu_rec_1c,x
    sep.b #$20
    plx
    lda.w !obj_24,x
    ora.b #$20
    sta.w !obj_24,x
    lda.w !obj_24,x
    ora.b #$08
    sta.w !obj_24,x
    rep.b #$20
    lda.w !obj_pos_y,x
    clc
    adc.w #$B1E0
    sta.w !obj_pos_y,x
    sep.b #$20
    rep.b #$20
    lda.w #$0000
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$00
    sta.w !obj_1c,x
    rtl

; @routine 08:EFAF ObjectPaceCarSetup
; WHAT IT DOES: In Speed Trax (HUD mode 0; otherwise released): $1F73 =
; its Z, $1F71 = 0, its Y - $64; a pace car (kind $A040, handler $08:EF06
; ObjectPaceCarStart) at its position, Z - $DAC, Y 0, +$27 = the selection
; handler $18DB; a signal (kind $A7B0) at its position, Y + $32, light
; +$5A = $80, put in its bucket and made its child; then it becomes the
; signal's driver: handler $08:F0B1 (ObjectSignalStep), +$20 = $C6DC, a
; Super FX record (GsuRecordNew422) at $0640, $0640, $0640 with flags
; $C100, count +$48 = $14, and its first step. A failed spawn or record
; returns ($08:A033).
; HOW: Falls into ObjectSignalStep.
; Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectSignalStep.
%at($08EFAF)
ObjectPaceCarSetup:
    lda.w !hud_mode
    beq .lEFB7
    brl ObjectReleaseLong           ; not reached in the traces so far
.lEFB7:
    rep.b #$20
    lda.w !obj_pos_z,x
    sta.w !script_byte_1f72+1
    sep.b #$20
    lda.b #$00
    sta.w !script_byte_1f71
    rep.b #$20
    lda.w !obj_pos_y,x
    clc
    adc.w #$FF9C
    sta.w !obj_pos_y,x
    sep.b #$20
    rep.b #$20
    lda.w #$A040
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .lEFE8
    jml.l RtlA033                   ; not reached in the traces so far
.lEFE8:
    jsl.l ObjectCopyPositionTo
    rep.b #$20
    lda.w !obj_pos_z,y
    clc
    adc.w #$F254
    sta.w !obj_pos_z,y
    sep.b #$20
    rep.b #$20
    lda.w #$0000
    sta.w !obj_pos_y,y
    sep.b #$20
    rep.b #$20
    lda.w !selection_handler_a
    sta.w !obj_27,y
    sep.b #$20
    rep.b #$20
    lda.w #$EF06
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,y
    rep.b #$20
    lda.w #$A7B0
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .lF031
    jml.l RtlA033                   ; not reached in the traces so far
.lF031:
    jsl.l ObjectCopyPositionTo
    rep.b #$20
    lda.w !obj_pos_y,y
    clc
    adc.w #$0032
    sta.w !obj_pos_y,y
    sep.b #$20
    lda.b #$00
    ora.b #$80
    sta.w !obj_5a,y
    phx
    tyx
    ply
    jsl.l ObjectRebucket
    phx
    tyx
    ply
    pha
    rep.b #$20
    tya
    sta.w !obj_06,x
    sep.b #$20
    pla
    rep.b #$20
    lda.w #$F0B1
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,x
    rep.b #$20
    lda.w #$C6DC
    sta.w !obj_20,x
    sep.b #$20
    jsl.l GsuRecordNew422
    bpl .lF080
    brl RtlA033                     ; not reached in the traces so far
.lF080:
    rep.b #$20
    lda.w #$0640
    sta.b !dp_02
    sep.b #$20
    rep.b #$20
    lda.w #$0640
    sta.b !dp_08
    sep.b #$20
    rep.b #$20
    lda.w #$0640
    sta.b !dp_94
    sep.b #$20
    jsl.l GsuRecordPosition
    rep.b #$20
    lda.w #$C100
    sta.b !dp_02
    sep.b #$20
    jsl.l GsuRecordSetFlags
    lda.b #$14
    sta.w !obj_48,x

; @routine 08:F0B1 ObjectSignalStep
; WHAT IT DOES: The pace car's signal: synced (+$36 = 3) each frame; once
; $1F71 is set, its child (+$06) gets +$04 = $A794 and its light +$5A
; (low seven bits, 0-7) steps up one a frame, and at 7 the object $0346
; gets +$04 = $A75C, the child and the signal are released and $1F71 =
; 2.
; HOW: The light steps by ADC #1 with a wrap to 8 in its low seven bits,
; bit 7 set.
; Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; as the last call
; leaves it.
%at($08F0B1)
ObjectSignalStep:
    lda.w !script_byte_1f71
    beq .lF0E5
    ldy.w !obj_06,x
    rep.b #$20
    lda.w #$A794
    sta.w !obj_04,y
    sep.b #$20
    lda.w !obj_5a,y
    and.b #$7F
    cmp.b #$07
    beq .lF0EF
    lda.w !obj_5a,y
    clc
    adc.b #$01
    bmi .lF0D7
    clc                             ; not reached in the traces so far
    adc.b #$08
.lF0D7:
    and.b #$7F
    cmp.b #$08
    bcc .lF0E0
    sec                             ; not reached in the traces so far
    sbc.b #$08
.lF0E0:
    ora.b #$80
    sta.w !obj_5a,y
.lF0E5:
    jsl.l ObjectSyncFromRecord
    lda.b #$03
    sta.w !obj_36,x
    rtl
.lF0EF:
    ldy.w !wram_0346
    rep.b #$20
    lda.w #$A75C
    sta.w !obj_04,y
    sep.b #$20
    ldy.w !obj_06,x
    jsl.l ObjectReleaseY
    jsl.l ObjectRelease
    lda.b #$02
    sta.w !script_byte_1f71
    rtl
