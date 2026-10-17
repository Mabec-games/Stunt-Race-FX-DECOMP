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
    jmp.w ObjectBreakOffContact         ; not reached in the traces so far
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

; @routine 08:BA57 ObjectChildrenSpawn
; WHAT IT DOES: Gives object X a child object for each record in its
; record's chain (its record's +$3E, then each one's +$34, to 0), except
; records with +$46 bit 9: the child's kind, +$29 and +$5F from the three
; words at $09:8000 + the record's +$12; with a nonzero +$29 the child
; gets +$24 bit 3, +$3A = 0, +$5F and +$29 (+ 2 for two players);
; handler $08:BB16 (ObjectChildStart), +$22 = the record, +$06 = object
; X. With no record of its own it just returns ($08:A033).
; HOW: $1F50 keeps the parent; $1F4C, $1F52, $1F4E the table's words; a
; failed spawn skips the record.
; Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; X = the object; Y
; the last child (or as ObjectSpawn left it).
%at($08BA57)
ObjectChildrenSpawn:
    stx.w !wram_1F50
    rep.b #$20
    lda.w !obj_22,x
    bne .lBA64
    brl RtlA033                     ; not reached in the traces so far
.lBA64:
    phx
    tax
    lda.l !gsu_in_3e,x
.lBA6A:
    tax
    bne .lBA70
    brl .lBB12
.lBA70:
    lda.l !gsu_speed_44+2,x
    and.w #$0200
    beq .lBA7C
    brl .lBB09                      ; not reached in the traces so far
.lBA7C:
    phx
    lda.l !gsu_rec_12,x
    tax
    lda.l !attach_templates,x
    sta.w !wram_1F4C
    lda.l !attach_templates+2,x
    sta.w !wram_1F52
    lda.l !attach_templates+4,x
    sta.w !wram_1F4E
    plx
    sep.b #$20
    rep.b #$20
    lda.w !wram_1F4C
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .lBAAE
    jml.l .lBB09                    ; not reached in the traces so far
.lBAAE:
    lda.w !wram_1F52
    ora.w !wram_1F52+1
    beq .lBAE6
    lda.w !obj_24,y
    ora.b #$08
    sta.w !obj_24,y
    lda.b #$00
    sta.w !obj_3a,y
    lda.w !wram_1F4E
    sta.w !obj_5f,y
    rep.b #$20
    lda.w !wram_1F52
    sta.w !obj_29,y
    sep.b #$20
    lda.w !wram_0D62
    beq .lBAE6
    rep.b #$20
    lda.w !obj_29,y
    clc
    adc.w #$0002
    sta.w !obj_29,y
    sep.b #$20
.lBAE6:
    rep.b #$20
    lda.w #$BB16
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,y
    pha
    rep.b #$20
    txa
    sta.w !obj_22,y
    sep.b #$20
    pla
    rep.b #$20
    lda.w !wram_1F50
    sta.w !obj_06,y
    sep.b #$20
.lBB09:
    rep.b #$20
    lda.l !gsu_in_34,x
    jmp.w .lBA6A
.lBB12:
    plx
    sep.b #$20
    rtl

; @routine 08:BB16 ObjectChildStart
; WHAT IT DOES: A child object's first frame: handler $08:BB2E
; (ObjectChildStep), its +$26 from its parent's; then its first step.
; HOW: Falls into ObjectChildStep.
; Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectChildStep.
%at($08BB16)
ObjectChildStart:
    rep.b #$20
    lda.w #$BB2E
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,x
    ldy.w !obj_06,x
    lda.w !obj_26,y
    sta.w !obj_26,x

; @routine 08:BB2E ObjectChildStep
; WHAT IT DOES: A child object's frame on its parent: follows it
; (ObjectChildFollow); when the parent's +$25 has bit 2 its handler
; becomes $08:BB7C (ObjectChildLoose, next frame); then its +$24 flags
; from the parent's and +$36 = 3.
; HOW: Byte tests through the data bank.
; Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; A low byte 3; Y =
; the parent.
%at($08BB2E)
ObjectChildStep:
    jsr.w ObjectChildFollow
    ldy.w !obj_06,x
    lda.w !obj_25,y
    and.b #$04
    bne .lBB3F
    jml.l ObjectChildFlags
.lBB3F:
    rep.b #$20
    lda.w #$BB7C
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,x
ObjectChildFlags:
    ldy.w !obj_06,x
    lda.w !obj_24,x
    and.b #$BF
    sta.w !obj_24,x
    lda.w !obj_24,y
    bit.b #$08
    bne .lBB6E
    bit.b #$40
    beq .lBB76
    lda.w !obj_24,x
    ora.b #$40
    sta.w !obj_24,x
    bra .lBB76
.lBB6E:
    lda.w !obj_24,x
    ora.b #$20
    sta.w !obj_24,x
.lBB76:
    lda.b #$03
    sta.w !obj_36,x
    rtl

; @routine 08:BB7C ObjectChildLoose
; WHAT IT DOES: A child comes loose from its parent: handler $08:BBE2
; (ObjectChildFlying), +$27 = its record (+$22), a record of its own
; (GsuRecordNew426) with the parent's record's velocity
; (ObjectVelocityFromRecordOf), then X and Z halved (+$36, +$3A of the
; old record, arithmetic shift) and Y $FFCE (GsuRecordVelocity); count
; +$4B = random 0-$0F + $0A; then its first flight.
; HOW: Falls into ObjectChildFlying; the halving is CMP #$8000, ROR A.
; Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectChildFlying.
%at($08BB7C)
ObjectChildLoose:
    rep.b #$20
    lda.w #$BBE2
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,x
    rep.b #$20
    lda.w !obj_22,x
    sta.w !obj_27,x
    sep.b #$20
    jsl.l GsuRecordNew426
    ldy.w !obj_06,x
    jsl.l ObjectVelocityFromRecordOf
    bpl .lBBA5
    brl ObjectChildBack             ; not reached in the traces so far
.lBBA5:
    rep.b #$20
    phx
    txy
    lda.w !obj_27,x
    tax
    lda.l !gsu_in_36,x
    cmp.w #$8000
    ror a
    sta.w !obj_vel_x,y
    lda.w #$FFCE
    sta.w !obj_vel_y,y
    lda.l !gsu_in_3a,x
    cmp.w #$8000
    ror a
    sta.w !obj_vel_z,y
    plx
    sep.b #$20
    jsl.l GsuRecordVelocity
    jsl.l RandomStepLong
    and.b #$0F
    sta.w !obj_4b,x
    lda.w !obj_4b,x
    clc
    adc.b #$0A
    sta.w !obj_4b,x

; @routine 08:BBE2 ObjectChildFlying
; WHAT IT DOES: A loose child's flight: synced from its record
; (ObjectSyncFromRecord); with the parent's +$25 bit 4 and a contact
; (RecordContactSummary bit 7, the sync's bit 0 clear) the code at
; $08:D337. While its count +$4B runs it drops ($14 + its +$5F lower)
; and may spawn (ObjectSpawnChanceD460); at 0 with the parent's +$25 bit
; 4 it is released if the parent's record +$64 has bit 4, else may spawn;
; at 0 otherwise it goes back (handler $08:BC73, ObjectChildReturn, at
; once). Then its +$24 flags and +$36 = 3.
; HOW: Byte tests through the data bank; the release is a jump.
; Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; as the path
; leaves it.
%at($08BBE2)
ObjectChildFlying:
    jsl.l ObjectSyncFromRecord
    and.b #$01
    bne .lBC03
    ldy.w !obj_06,x
    lda.w !obj_25,y
    and.b #$10
    bne .lBBF8
    jml.l .lBC03
.lBBF8:
    jsl.l RecordContactSummary
    and.b #$80
    beq .lBC03
    jmp.w ObjectBreakOffContact
.lBC03:
    ldy.w !obj_06,x
    lda.w !obj_25,y
    and.b #$10
    bne .lBC11
    jml.l .lBC1A
.lBC11:
    lda.w !obj_4b,x
    bne .lBC1A
    jml.l .lBC45
.lBC1A:
    lda.w !obj_4b,x
    bne .lBC23
    jml.l .lBC64
.lBC23:
    dec.w !obj_4b,x
    rep.b #$20
    lda.w !obj_5f,x
    and.w #$00FF
    eor.w #$FFFF
    inc a
    clc
    adc.w #$FFEC
    clc
    adc.w !obj_pos_y,x
    sta.w !obj_pos_y,x
    sep.b #$20
    jsr.w ObjectSpawnChanceD460
    jmp.w ObjectChildFlags
.lBC45:
    phx
    rep.b #$20
    ldy.w !obj_06,x
    lda.w !obj_22,y
    tax
    lda.l !gsu_rec_64,x
    plx
    and.w #$0010
    sep.b #$20
    beq .lBC5E
    brl ObjectReleaseLong           ; not reached in the traces so far
.lBC5E:
    jsr.w ObjectSpawnChanceD460
    jmp.w ObjectChildFlags
.lBC64:
    rep.b #$20
    lda.w #$BC73
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,x

; @routine 08:BC73 ObjectChildReturn
; WHAT IT DOES: A loose child going back: its velocity from its parent's
; record (ObjectVelocityFromRecordOf); its position a step (a quarter, at
; least 4: ApproachQuarterMin4) toward its place on the parent's record
; (+$1E + $4A, +$20 + $4C + $FFEC - its +$5F, +$22 + $4E, its turn by
; ObjectTurnFromRecord), moved (ObjectApplyVelocity) and rebucketed; once
; the parent's +$25 bit 2 is clear it is back (+$22 = +$27, handler
; $08:BB2E). Then its +$24 flags and +$36 = 3.
; HOW: The target kept in direct page $04, $0A and $05F2.
; Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; A low byte 3.
%at($08BC73)
ObjectChildReturn:
    ldy.w !obj_06,x
    jsl.l ObjectVelocityFromRecordOf
    rep.b #$20
    phx
    lda.w !obj_27,x
    txy
    tax
    lda.l !gsu_rec_1e,x
    clc
    adc.l !gsu_speeds,x
    sta.b !dp_04
    lda.w !obj_5f,y
    and.w #$00FF
    eor.w #$FFFF
    inc a
    clc
    adc.w #$FFEC
    clc
    adc.l !gsu_in_20,x
    clc
    adc.l !gsu_speeds+2,x
    sta.b !actor_0a
    lda.l !gsu_in_22,x
    clc
    adc.l !gsu_rec_4e,x
    sta.w !rot_z
    jsr.w ObjectTurnFromRecord
    plx
    sep.b #$20
    ldy.w !obj_06,x
    rep.b #$20
    lda.w !obj_pos_x,x
    sta.b !scale_scratch
    lda.b !dp_04
    jsl.l ApproachQuarterMin4
    sta.w !obj_pos_x,x
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_y,x
    sta.b !scale_scratch
    lda.b !actor_0a
    jsl.l ApproachQuarterMin4
    sta.w !obj_pos_y,x
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_z,x
    sta.b !scale_scratch
    lda.w !rot_z
    jsl.l ApproachQuarterMin4
    sta.w !obj_pos_z,x
    sep.b #$20
    jsl.l ObjectApplyVelocity
    jsl.l ObjectRebucket
    lda.w !obj_25,y
    and.b #$04
    beq ObjectChildBack
    jml.l ObjectChildFlags
ObjectChildBack:
    rep.b #$20
    lda.w !obj_27,x
    sta.w !obj_22,x
    sep.b #$20
    rep.b #$20
    lda.w #$BB2E
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,x
    jmp.w ObjectChildFlags

; @routine 08:BD22 ObjectChildFollow
; WHAT IT DOES: Keeps object X (a child of +$06) on its parent: rebucketed;
; when the parent's record has flag $0400, its own record gets $0480 and
; its +$24 bit 5, and nothing more. Otherwise its +$24 bit 5 and its
; record's $0480 are cleared; unless the parent's +$24 has bit 6 or its
; record's +$64 has neither bit 0 nor 1, and when its own record's +$46
; has bits 0 and 4: by its record's +$2C (bit 7, bit 13, bit 11, bit 12,
; bits 5-6) or, failing those, the parent's +$40 bit 11 or its +$46 bit 5,
; a new handler for it ($D1F7, $D249, $D39F, $D2CF; $D33F or $D303 by
; speed; $D3D3) when it moves fast enough (|+$2E| + |+$32| >= $64, where
; asked) and the code at $08:D1BF allows it (C). Then its position is its
; record's (+$1E + $4A, +$20 + $4C + $FFEC - its +$5F, +$22 + $4E) and
; its turn (ObjectTurnFromRecord).
; HOW: 16-bit record reads at $70:0000 + +$22; the object kept on the
; stack (PHX) and in Y.
; Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; X = the object; Y
; as the path leaves it.
%at($08BD22)
ObjectChildFollow:
    phx
    jsl.l ObjectRebucket
    txy
    rep.b #$20
    lda.w !obj_06,x
    tax
    lda.w !obj_22,x
    tax
    lda.l !gsu_rec_flags,x
    and.w #$0400
    beq .lBD57
    lda.w !obj_22,y
    tax
    lda.l !gsu_rec_flags,x
    ora.w #$0480
    sta.l !gsu_rec_flags,x
    sep.b #$20
    lda.w !obj_24,y
    ora.b #$20
    sta.w !obj_24,y
    jmp.w .lBF90
.lBD57:
    tyx
    sep.b #$20
    lda.w !obj_24,x
    and.b #$DF
    sta.w !obj_24,x
    phx
    rep.b #$20
    lda.w !obj_22,x
    tax
    lda.l !gsu_rec_flags,x
    and.w #$FB7F
    sta.l !gsu_rec_flags,x
    tyx
    lda.w !obj_06,x
    tax
    lda.w !obj_24,x
    and.w #$0040
    beq .lBD84
    brl .lBF53
.lBD84:
    lda.w !obj_22,x
    tax
    lda.l !gsu_rec_40,x
    sta.w !wram_1F4C
    lda.l !gsu_rec_64,x
    and.w #$0003
    bne .lBD9B
    brl .lBF53
.lBD9B:
    plx
    phx
    lda.w !obj_22,x
    tax
    lda.l !gsu_speed_44+2,x
    bit.w #$0001
    bne .lBDAD
    brl .lBF53
.lBDAD:
    bit.w #$0010
    bne .lBDB5
    brl .lBF53
.lBDB5:
    lda.l !gsu_dest,x
    bit.w #$0080
    beq .lBDC1
    brl .lBEEF
.lBDC1:
    bit.w #$2000
    beq .lBDC9
    brl .lBEB2
.lBDC9:
    bit.w #$0800
    beq .lBDD1
    brl .lBE38
.lBDD1:
    bit.w #$1000
    beq .lBDD9
    brl .lBDFB
.lBDD9:
    bit.w #$0060
    beq .lBDE1
    brl .lBE75
.lBDE1:
    lda.w !wram_1F4C
    bit.w #$0800
    beq .lBDEC
    brl .lBF3F
.lBDEC:
    lda.l !gsu_speed_44+2,x
    bit.w #$0020
    beq .lBDF8
    brl .lBF3F
.lBDF8:
    jmp.w .lBF53
.lBDFB:
    lda.l !gsu_in_2e,x
    bpl .lBE05
    eor.w #$FFFF
    inc a
.lBE05:
    sta.w !wram_1F2B
    lda.l !gsu_rec_vel_z,x
    bpl .lBE12
    eor.w #$FFFF
    inc a
.lBE12:
    clc
    adc.w !wram_1F2B
    cmp.w #$0064
    bpl .lBE1E
    brl .lBF53
.lBE1E:
    jsr.w ObjectBreakOffChance
    bcs .lBE26
    brl .lBF53
.lBE26:
    rep.b #$20
    lda.w #$D1F7
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,y
    jmp.w .lBF53
.lBE38:
    lda.l !gsu_in_2e,x
    bpl .lBE42
    eor.w #$FFFF
    inc a
.lBE42:
    sta.w !wram_1F2B
    lda.l !gsu_rec_vel_z,x
    bpl .lBE4F
    eor.w #$FFFF
    inc a
.lBE4F:
    clc
    adc.w !wram_1F2B
    cmp.w #$0064
    bpl .lBE5B
    brl .lBF53
.lBE5B:
    jsr.w ObjectBreakOffChance
    bcs .lBE63
    brl .lBF53
.lBE63:
    rep.b #$20
    lda.w #$D249
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,y
    jmp.w .lBF53
.lBE75:
    lda.l !gsu_in_2e,x
    bpl .lBE7F
    eor.w #$FFFF
    inc a
.lBE7F:
    sta.w !wram_1F2B
    lda.l !gsu_rec_vel_z,x
    bpl .lBE8C
    eor.w #$FFFF
    inc a
.lBE8C:
    clc
    adc.w !wram_1F2B
    cmp.w #$0064
    bpl .lBE98
    brl .lBF53
.lBE98:
    jsr.w ObjectBreakOffChance
    bcs .lBEA0
    brl .lBF53
.lBEA0:
    rep.b #$20
    lda.w #$D39F
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,y
    jmp.w .lBF53
.lBEB2:
    lda.l !gsu_in_2e,x
    bpl .lBEBC
    eor.w #$FFFF
    inc a
.lBEBC:
    sta.w !wram_1F2B
    lda.l !gsu_rec_vel_z,x
    bpl .lBEC9
    eor.w #$FFFF
    inc a
.lBEC9:
    clc
    adc.w !wram_1F2B
    cmp.w #$0064
    bpl .lBED5
    brl .lBF53
.lBED5:
    jsr.w ObjectBreakOffChance
    bcs .lBEDD
    brl .lBF53
.lBEDD:
    rep.b #$20
    lda.w #$D2CF
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,y
    jmp.w .lBF53
.lBEEF:
    jsr.w ObjectBreakOffChance
    bcc .lBF53
    rep.b #$20
    lda.l !gsu_in_2e,x
    bpl .lBF00
    eor.w #$FFFF
    inc a
.lBF00:
    sta.w !wram_1F2B
    lda.l !gsu_rec_vel_z,x
    bpl .lBF0D
    eor.w #$FFFF
    inc a
.lBF0D:
    clc
    adc.w !wram_1F2B
    cmp.w #$0064
    bpl .lBF19
    brl .lBF2C
.lBF19:
    sep.b #$20
    rep.b #$20
    lda.w #$D33F
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,y
    bra .lBF53
.lBF2C:
    sep.b #$20
    rep.b #$20
    lda.w #$D303
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,y
    bra .lBF53
.lBF3F:
    jsr.w ObjectBreakOffChance
    bcc .lBF53
    rep.b #$20
    lda.w #$D3D3
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,y
.lBF53:
    ply
    rep.b #$20
    lda.w !obj_22,y
    tax
    lda.l !gsu_rec_1e,x
    clc
    adc.l !gsu_speeds,x
    sta.w !obj_pos_x,y
    lda.w !obj_5f,y
    and.w #$00FF
    eor.w #$FFFF
    inc a
    clc
    adc.w #$FFEC
    clc
    adc.l !gsu_in_20,x
    clc
    adc.l !gsu_speeds+2,x
    sta.w !obj_pos_y,y
    lda.l !gsu_in_22,x
    clc
    adc.l !gsu_rec_4e,x
    sta.w !obj_pos_z,y
    jsr.w ObjectTurnFromRecord
.lBF90:
    sep.b #$20
    plx
    rts

; @routine 08:D1BF ObjectBreakOffChance
; WHAT IT DOES: For child Y breaking off: on a random byte of $7F or more,
; a new object (kind $A1AC) at Y's position with Y's parent (+$06) as its
; +$27; C set when one was made, clear otherwise.
; HOW: X kept (PHX, PLX); ObjectSpawn and ObjectCopyPositionTo from Y
; (TYX).
; Entry: X 16-bit; Y = the child. Exit: M 8-bit; X kept; C as above; Y =
; the new object when there is one.
%at($08D1BF)
ObjectBreakOffChance:
    sep.b #$20
    phx
    tyx
    jsl.l RandomStepLong
    cmp.b #$7F
    bcs .lD1CF
    jml.l .lD1F4
.lD1CF:
    rep.b #$20
    lda.w #$A1AC
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .lD1E3
    jml.l .lD1F4
.lD1E3:
    jsl.l ObjectCopyPositionTo
    rep.b #$20
    lda.w !obj_06,x
    sta.w !obj_27,y
    sep.b #$20
    plx
    sec
    rts
.lD1F4:
    plx
    clc
    rts

; @routine 08:D1F7 ObjectBreakOffA
; WHAT IT DOES: A child broken off (look $998D): +$24 bit 3, +$3A = 0,
; +$5F = $14, its parent's record velocity scaled
; (ObjectVelocityFromRecordScaled), $28 lower; then falls
; (ObjectPieceFall, 4 frames).
; HOW: Ends in ObjectPieceFall with +$25 bit 0 set.
; Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectPieceFall.
%at($08D1F7)
ObjectBreakOffA:
    rep.b #$20
    lda.w #$998D
    sta.w !obj_5d,x
    sep.b #$20
    lda.w !obj_24,x
    ora.b #$08
    sta.w !obj_24,x
    lda.b #$00
    sta.w !obj_3a,x
    lda.b #$14
    sta.w !obj_5f,x
    jsr.w ObjectVelocityFromRecordScaled
    rep.b #$20
    lda.w !obj_pos_y,x
    clc
    adc.w #$0028
    sta.w !obj_pos_y,x
    sep.b #$20
    rep.b #$20
    lda.w #$D549
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,x
    lda.b #$00
    sta.w !obj_48,x
    lda.w !obj_25,x
    ora.b #$01
    sta.w !obj_25,x
    lda.b #$04
    sta.w !obj_49,x
    jml.l ObjectPieceFall

; @routine 08:D249 ObjectBreakOffB
; WHAT IT DOES: As ObjectBreakOffA with look $9B96, the record's velocity
; unscaled (ObjectVelocityFromRecord) and 3 frames.
; HOW: As ObjectBreakOffA. Not reached in the traces so far.
; Entry: as ObjectBreakOffA. Exit: as ObjectBreakOffA.
%at($08D249)
ObjectBreakOffB:
    rep.b #$20                      ; not reached in the traces so far
    lda.w #$9B96
    sta.w !obj_5d,x
    sep.b #$20
    lda.w !obj_24,x
    ora.b #$08
    sta.w !obj_24,x
    lda.b #$00
    sta.w !obj_3a,x
    lda.b #$14
    sta.w !obj_5f,x
    jsr.w ObjectVelocityFromRecord
    rep.b #$20
    lda.w !obj_pos_y,x
    clc
    adc.w #$0028
    sta.w !obj_pos_y,x
    sep.b #$20
    rep.b #$20
    lda.w #$D549
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,x
    lda.b #$00
    sta.w !obj_48,x
    lda.w !obj_25,x
    ora.b #$01
    sta.w !obj_25,x
    lda.b #$03
    sta.w !obj_49,x
    jml.l ObjectPieceFall

; @routine 08:D29B ObjectBreakOffC
; WHAT IT DOES: As ObjectBreakOffB with look $9B9F (ObjectAdvance5DIfFlag)
; and 4 frames.
; HOW: As ObjectBreakOffA. Not reached in the traces so far.
; Entry: as ObjectBreakOffA. Exit: as ObjectBreakOffA.
%at($08D29B)
ObjectBreakOffC:
    rep.b #$20                      ; not reached in the traces so far
    lda.w #$9B9F
    sta.w !obj_5d,x
    sep.b #$20
    jsr.w ObjectAdvance5DIfFlag
    lda.w !obj_24,x
    ora.b #$08
    sta.w !obj_24,x
    lda.b #$00
    sta.w !obj_3a,x
    lda.b #$14
    sta.w !obj_5f,x
    jsr.w ObjectVelocityFromRecord
    rep.b #$20
    lda.w !obj_pos_y,x
    clc
    adc.w #$0028
    sta.w !obj_pos_y,x
    sep.b #$20
    jml.l ObjectPieceFall4

; @routine 08:D2CF ObjectBreakOffD
; WHAT IT DOES: As ObjectBreakOffC with look $9B8F.
; HOW: As ObjectBreakOffA.
; Entry: as ObjectBreakOffA. Exit: as ObjectBreakOffA.
%at($08D2CF)
ObjectBreakOffD:
    rep.b #$20
    lda.w #$9B8F
    sta.w !obj_5d,x
    sep.b #$20
    jsr.w ObjectAdvance5DIfFlag
    lda.w !obj_24,x
    ora.b #$08
    sta.w !obj_24,x
    lda.b #$00
    sta.w !obj_3a,x
    lda.b #$14
    sta.w !obj_5f,x
    jsr.w ObjectVelocityFromRecord
    rep.b #$20
    lda.w !obj_pos_y,x
    clc
    adc.w #$0028
    sta.w !obj_pos_y,x
    sep.b #$20
    jml.l ObjectPieceFall4

; @routine 08:D303 ObjectBreakOffE
; WHAT IT DOES: As ObjectBreakOffC with look $9B79.
; HOW: As ObjectBreakOffA.
; Entry: as ObjectBreakOffA. Exit: as ObjectBreakOffA.
%at($08D303)
ObjectBreakOffE:
    rep.b #$20
    lda.w #$9B79
    sta.w !obj_5d,x
    sep.b #$20
    jsr.w ObjectAdvance5DIfFlag
    lda.w !obj_24,x
    ora.b #$08
    sta.w !obj_24,x
    lda.b #$00
    sta.w !obj_3a,x
    lda.b #$14
    sta.w !obj_5f,x
    jsr.w ObjectVelocityFromRecord
    rep.b #$20
    lda.w !obj_pos_y,x
    clc
    adc.w #$0028
    sta.w !obj_pos_y,x
    sep.b #$20
    jml.l ObjectPieceFall4

; @routine 08:D337 ObjectBreakOffContact
; WHAT IT DOES: A piece hit on contact: look $9B63 with +$5F = $6E, $28
; lower, falls (4 frames).
; HOW: $1F58 = $6E, then the tail of ObjectBreakOffF.
; Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectPieceFall.
%at($08D337)
ObjectBreakOffContact:
    lda.b #$6E
    sta.w !wram_1F58
    jmp.w ObjectBreakOffFTail

; @routine 08:D33F ObjectBreakOffF
; WHAT IT DOES: As ObjectBreakOffC with look $9B63 and +$5F = half the
; speed of its parent's (+$27) record ($4A, its size) + $14, at most $78.
; HOW: The halving is CMP #$8000, ROR A; $1F58 holds +$5F.
; Entry: as ObjectBreakOffA. Exit: as ObjectBreakOffA.
%at($08D33F)
ObjectBreakOffF:
    rep.b #$20
    phx
    lda.w !obj_27,x
    tax
    lda.w !obj_22,x
    tax
    lda.l !gsu_speeds,x
    bpl .lD354
    eor.w #$FFFF
    inc a
.lD354:
    cmp.w #$8000
    ror a
    clc
    adc.w #$0014
    cmp.w #$0078
    bmi .lD364
    lda.w #$0078
.lD364:
    sep.b #$20
    sta.w !wram_1F58
    plx
    jsr.w ObjectVelocityFromRecord
ObjectBreakOffFTail:
    rep.b #$20
    lda.w #$9B63
    sta.w !obj_5d,x
    sep.b #$20
    jsr.w ObjectAdvance5DIfFlag
    lda.w !obj_24,x
    ora.b #$08
    sta.w !obj_24,x
    lda.b #$00
    sta.w !obj_3a,x
    lda.w !wram_1F58
    sta.w !obj_5f,x
    rep.b #$20
    lda.w !obj_pos_y,x
    clc
    adc.w #$0028
    sta.w !obj_pos_y,x
    sep.b #$20
    jml.l ObjectPieceFall4

; @routine 08:D39F ObjectBreakOffG
; WHAT IT DOES: As ObjectBreakOffD with look $9B56, $28 lower before the
; scaled velocity (ObjectVelocityFromRecordScaled).
; HOW: As ObjectBreakOffA.
; Entry: as ObjectBreakOffA. Exit: as ObjectBreakOffA.
%at($08D39F)
ObjectBreakOffG:
    rep.b #$20
    lda.w #$9B56
    sta.w !obj_5d,x
    sep.b #$20
    jsr.w ObjectAdvance5DIfFlag
    lda.w !obj_24,x
    ora.b #$08
    sta.w !obj_24,x
    lda.b #$00
    sta.w !obj_3a,x
    lda.b #$14
    sta.w !obj_5f,x
    rep.b #$20
    lda.w !obj_pos_y,x
    clc
    adc.w #$0028
    sta.w !obj_pos_y,x
    sep.b #$20
    jsr.w ObjectVelocityFromRecordScaled
    jml.l ObjectPieceFall4

; @routine 08:D3D3 ObjectBreakOffH
; WHAT IT DOES: As ObjectBreakOffG with look $9B40 and +$5F = $0A.
; HOW: As ObjectBreakOffA.
; Entry: as ObjectBreakOffA. Exit: as ObjectBreakOffA.
%at($08D3D3)
ObjectBreakOffH:
    rep.b #$20
    lda.w #$9B40
    sta.w !obj_5d,x
    sep.b #$20
    jsr.w ObjectAdvance5DIfFlag
    lda.w !obj_24,x
    ora.b #$08
    sta.w !obj_24,x
    lda.b #$00
    sta.w !obj_3a,x
    lda.b #$0A
    sta.w !obj_5f,x
    rep.b #$20
    lda.w !obj_pos_y,x
    clc
    adc.w #$0028
    sta.w !obj_pos_y,x
    sep.b #$20
    jsr.w ObjectVelocityFromRecordScaled
ObjectPieceFall4:
    rep.b #$20
    lda.w #$D549
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,x
    lda.b #$00
    sta.w !obj_48,x
    lda.w !obj_25,x
    ora.b #$01
    sta.w !obj_25,x
    lda.b #$04
    sta.w !obj_49,x
    jml.l ObjectPieceFall

; @routine 08:D428 ObjectPieceFallStart
; WHAT IT DOES: A piece's first frame: falls for 6 frames
; (ObjectPieceFall), look $997C, +$24 bit 3, +$3A = 0, +$5F = $0A.
; HOW: Falls into ObjectPieceFall. No caller found: not reached in the
; traces so far.
; Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectPieceFall.
%at($08D428)
ObjectPieceFallStart:
    rep.b #$20                      ; not reached in the traces so far
    lda.w #$D549
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,x
    lda.b #$00
    sta.w !obj_48,x
    lda.b #$06
    sta.w !obj_49,x
    rep.b #$20
    lda.w #$997C
    sta.w !obj_5d,x
    sep.b #$20
    lda.w !obj_24,x
    ora.b #$08
    sta.w !obj_24,x
    lda.b #$00
    sta.w !obj_3a,x
    lda.b #$0A
    sta.w !obj_5f,x
    jmp.w ObjectPieceFall

; @routine 08:D460 ObjectPieceSpray
; WHAT IT DOES: A spray piece's first frame (from ObjectSpawnChanceD460):
; falls for 6 frames (ObjectPieceFall), look $997C, +$24 bit 3, +$3A = 0,
; +$5F = $0A; its offset ($00, $00, random -$28 to -$19) turned by its
; parent's (+$27) angles +$14 (RotateXZFrom08) and +$15 (RotateXZ),
; doubled and added to its position; its parent's record velocity scaled
; (ObjectVelocityFromRecordScaled), Y - $14, ObjectAdvance5DIfFlag; then
; its first fall.
; HOW: The offset in direct page $02/$04, $08/$0A, $94/$05F2 with their
; sign bytes; the copies between the turns are 8-bit.
; Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectPieceFall.
%at($08D460)
ObjectPieceSpray:
    rep.b #$20
    lda.w #$D549
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,x
    lda.b #$00
    sta.w !obj_48,x
    lda.b #$06
    sta.w !obj_49,x
    jsl.l RandomStepLong
    and.b #$0F
    sta.w !wram_1F4C
    rep.b #$20
    lda.w !wram_1F4C
    clc
    adc.w #$FFD8
    sta.w !wram_1F4C
    sep.b #$20
    rep.b #$20
    lda.w #$997C
    sta.w !obj_5d,x
    sep.b #$20
    lda.w !obj_24,x
    ora.b #$08
    sta.w !obj_24,x
    lda.b #$00
    sta.w !obj_3a,x
    lda.b #$0A
    sta.w !obj_5f,x
    ldy.w !obj_27,x
    lda.b #$00
    sta.b !dp_02
    sta.b !dp_04
    bit.b #$80
    beq .lD4BF
    lda.b #$FF                      ; not reached in the traces so far
    sta.b !dp_05
    bra .lD4C1
.lD4BF:
    stz.b !dp_05
.lD4C1:
    lda.b #$00
    sta.b !dp_08
    sta.b !actor_0a
    bit.b #$80
    beq .lD4D1
    lda.b #$FF                      ; not reached in the traces so far
    sta.b !dp_0b
    bra .lD4D3
.lD4D1:
    stz.b !dp_0b
.lD4D3:
    lda.w !wram_1F4C
    sta.b !dp_94
    sta.w !rot_z
    bit.b #$80
    beq .lD4E6
    lda.b #$FF
    sta.w !rot_z_high
    bra .lD4E9
.lD4E6:
    stz.w !rot_z_high               ; not reached in the traces so far
.lD4E9:
    lda.w !obj_14,y
    jsl.l RotateXZFrom08
    lda.b !dp_04
    sta.b !dp_02
    lda.b !actor_0a
    sta.b !dp_08
    lda.w !rot_z
    sta.b !dp_94
    lda.w !obj_15,y
    jsl.l RotateXZ
    rep.b #$20
    lda.b !dp_04
    asl a
    sta.b !dp_04
    lda.b !actor_0a
    asl a
    sta.b !actor_0a
    lda.w !rot_z
    asl a
    sta.w !rot_z
    lda.w !obj_pos_x,x
    clc
    adc.b !dp_04
    sta.w !obj_pos_x,x
    lda.w !obj_pos_z,x
    clc
    adc.w !rot_z
    sta.w !obj_pos_z,x
    lda.w !obj_pos_y,x
    clc
    adc.b !actor_0a
    sta.w !obj_pos_y,x
    sep.b #$20
    jsr.w ObjectVelocityFromRecordScaled
    rep.b #$20
    lda.w !obj_vel_y,x
    clc
    adc.w #$FFEC
    sta.w !obj_vel_y,x
    sep.b #$20
    jsr.w ObjectAdvance5DIfFlag

; @routine 08:D549 ObjectPieceFall
; WHAT IT DOES: A broken-off piece's frame: rebucketed; its +$18 and +$19
; = its frame count +$48 with bit 7; released once the count reaches its
; end +$49, otherwise counted up and moved (ObjectApplyVelocity); +$36 =
; 3.
; HOW: $1F58 and $1F59 hold the count and the end.
; Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; A low byte 3, or as
; ObjectReleaseLong leaves it.
%at($08D549)
ObjectPieceFall:
    jsl.l ObjectRebucket
    lda.w !obj_48,x
    sta.w !wram_1F58
    lda.w !wram_1F58
    ora.b #$80
    sta.w !obj_18,x
    lda.w !obj_18,x
    sta.w !obj_19,x
    lda.w !obj_49,x
    sta.w !wram_1F59
    lda.w !obj_48,x
    cmp.w !wram_1F59
    bne .lD573
    jml.l ObjectReleaseLong
.lD573:
    inc.w !obj_48,x
    jsl.l ObjectApplyVelocity
    lda.b #$03
    sta.w !obj_36,x
    rtl

; @routine 08:DCB4 ObjectAttachSpawn
; WHAT IT DOES: Gives object X the attachments listed for the course
; script's entry ($70:2392 indexes the list at $09:8090: a count, then five
; bytes each): for each, a new object (kind $A040) with object X as its
; +$06, its +$04, look, handler and bank from the five words at $09:8000 +
; the entry's first word (+$04, $1F50, $1F52, +$1A, +$1C); with a look
; ($1F50 nonzero) +$24 bit 3, +$3A = 0, +$5F = $1F52, +$5D = $1F50; and
; the entry's three signed bytes as its velocity (+$2E, +$30, +$32).
; HOW: The list index in X, the count on the stack; $1F4C keeps object X;
; a failed spawn skips its entry.
; Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; X = the object; Y
; the last attachment (or as ObjectSpawn left it).
%at($08DCB4)
ObjectAttachSpawn:
    stx.w !wram_1F4C
    rep.b #$20
    lda.l !fx_script_at
    tax
    sep.b #$20
    lda.l !attach_values,x
    bne .lDCC9
    brl .lDD89
.lDCC9:
    inx
.lDCCA:
    pha
    rep.b #$20
    lda.w #$A040
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .lDCDF
    jml.l .lDD78
.lDCDF:
    phx
    rep.b #$20
    lda.w !wram_1F4C
    sta.w !obj_06,y
    lda.l !attach_values,x
    tax
    lda.l !attach_templates,x
    sta.w !obj_04,y
    lda.l !attach_templates+2,x
    sta.w !wram_1F50
    lda.l !attach_templates+4,x
    sta.w !wram_1F52
    lda.l !attach_templates+6,x
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.l !attach_templates+8,x
    sta.w !obj_1c,y
    plx
    lda.w !wram_1F50
    ora.w !wram_1F50+1
    beq .lDD38
    lda.w !obj_24,y
    ora.b #$08
    sta.w !obj_24,y
    lda.b #$00
    sta.w !obj_3a,y
    lda.w !wram_1F52
    sta.w !obj_5f,y
    rep.b #$20
    lda.w !wram_1F50
    sta.w !obj_5d,y
    sep.b #$20
.lDD38:
    rep.b #$20
    lda.l !attach_values+2,x
    bit.w #$0080
    beq .lDD48
    ora.w #$FF00
    bra .lDD4B
.lDD48:
    and.w #$00FF
.lDD4B:
    sta.w !obj_vel_x,y
    lda.l !attach_values+3,x
    bit.w #$0080
    beq .lDD5C
    ora.w #$FF00
    bra .lDD5F
.lDD5C:
    and.w #$00FF
.lDD5F:
    sta.w !obj_vel_y,y
    lda.l !attach_values+4,x
    bit.w #$0080
    beq .lDD70
    ora.w #$FF00
    bra .lDD73
.lDD70:
    and.w #$00FF
.lDD73:
    sta.w !obj_vel_z,y
    sep.b #$20
.lDD78:
    rep.b #$20
    txa
    clc
    adc.w #$0005
    tax
    sep.b #$20
    pla
    dec a
    beq .lDD89
    brl .lDCCA
.lDD89:
    ldx.w !wram_1F4C
    rtl

; @routine 08:DD8D ObjectAttachStart
; WHAT IT DOES: An attachment's first frame: handler $08:DDB4
; (ObjectAttachFollow), its Y velocity's low byte sign-extended and
; doubled; then its first frame.
; HOW: Falls into ObjectAttachFollow.
; Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectAttachFollow.
%at($08DD8D)
ObjectAttachStart:
    rep.b #$20
    lda.w #$DDB4
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,x
    rep.b #$20
    lda.w !obj_vel_y,x
    bit.w #$0080
    beq .lDDAB
    ora.w #$FF00                    ; not reached in the traces so far
    bra .lDDAE
.lDDAB:
    and.w #$00FF
.lDDAE:
    asl a
    sta.w !obj_vel_y,x
    sep.b #$20

; @routine 08:DDB4 ObjectAttachFollow
; WHAT IT DOES: An attachment's frame: hidden ($08:E13C) when its parent
; is gone or loose, or when the parent's record has +$1C bit 4; otherwise
; placed at the parent's record position (+$1E, +$60, +$22) with its
; heading (+$27 to +$14, +$25 to +$15), moved by its velocity
; (ObjectApplyVelocity), rebucketed, +$24 bits 5 and 6 cleared, and bit 6
; set again ($08:E14A) when the parent's +$24 has it.
; HOW: Y = the attachment and X = the parent's record while copying.
; Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; as the path leaves
; it.
%at($08DDB4)
ObjectAttachFollow:
    ldy.w !obj_06,x
    lda.w !obj_24,y
    and.b #$08
    beq .lDDC2
    jml.l ObjectAttachHide
.lDDC2:
    lda.w !obj_25,y
    and.b #$04
    beq .lDDCD
    jml.l ObjectAttachHide
.lDDCD:
    phx
    rep.b #$20
    lda.w !obj_22,y
    txy
    tax
    lda.l !gsu_rec_1c,x
    and.w #$0010
    beq .lDDE4
    sep.b #$20
    plx
    jmp.w ObjectAttachHide
.lDDE4:
    lda.l !gsu_rec_1e,x
    sta.w !obj_pos_x,y
    lda.l !gsu_in_22,x
    sta.w !obj_pos_z,y
    lda.l !gsu_unpack_end,x
    sta.w !obj_pos_y,y
    sep.b #$20
    lda.l !gsu_rec_27,x
    sta.w !obj_14,y
    lda.l !gsu_rec_25,x
    sta.w !obj_15,y
    plx
    jsl.l ObjectApplyVelocity
    jsl.l ObjectRebucket
    lda.w !obj_24,x
    and.b #$DF
    sta.w !obj_24,x
    lda.w !obj_24,x
    and.b #$BF
    sta.w !obj_24,x
    ldy.w !obj_06,x
    lda.w !obj_24,y
    and.b #$40
    beq .lDE30
    jml.l ObjectAttachBit6
.lDE30:
    rtl

; @routine 08:DE31 ObjectSparksStart
; WHAT IT DOES: Gives object X four sparks (kind $A1E4, handler $08:E048
; ObjectSparkStart) at offsets (-$1E or $1E, $0A, $3C or -$14), its own
; handler $08:DF4C (ObjectAttachTurned); then its first frame.
; HOW: Falls into ObjectAttachTurned; a failed spawn skips its spark.
; Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectAttachTurned.
%at($08DE31)
ObjectSparksStart:
    rep.b #$20
    lda.w #$DF4C
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,x
    rep.b #$20
    lda.w #$A1E4
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .lDE54
    jml.l .lDE83                    ; not reached in the traces so far
.lDE54:
    pha
    rep.b #$20
    txa
    sta.w !obj_06,y
    sep.b #$20
    pla
    rep.b #$20
    lda.w #$E048
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,y
    rep.b #$20
    lda.w #$FFE2
    sta.w !obj_vel_x,y
    lda.w #$000A
    sta.w !obj_vel_y,y
    lda.w #$003C
    sta.w !obj_vel_z,y
    sep.b #$20
.lDE83:
    rep.b #$20
    lda.w #$A1E4
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .lDE97
    jml.l .lDEC6                    ; not reached in the traces so far
.lDE97:
    pha
    rep.b #$20
    txa
    sta.w !obj_06,y
    sep.b #$20
    pla
    rep.b #$20
    lda.w #$E048
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,y
    rep.b #$20
    lda.w #$001E
    sta.w !obj_vel_x,y
    lda.w #$000A
    sta.w !obj_vel_y,y
    lda.w #$003C
    sta.w !obj_vel_z,y
    sep.b #$20
.lDEC6:
    rep.b #$20
    lda.w #$A1E4
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .lDEDA
    jml.l .lDF09                    ; not reached in the traces so far
.lDEDA:
    pha
    rep.b #$20
    txa
    sta.w !obj_06,y
    sep.b #$20
    pla
    rep.b #$20
    lda.w #$E048
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,y
    rep.b #$20
    lda.w #$FFE2
    sta.w !obj_vel_x,y
    lda.w #$000A
    sta.w !obj_vel_y,y
    lda.w #$FFEC
    sta.w !obj_vel_z,y
    sep.b #$20
.lDF09:
    rep.b #$20
    lda.w #$A1E4
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .lDF1D
    jml.l ObjectAttachTurned        ; not reached in the traces so far
.lDF1D:
    pha
    rep.b #$20
    txa
    sta.w !obj_06,y
    sep.b #$20
    pla
    rep.b #$20
    lda.w #$E048
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,y
    rep.b #$20
    lda.w #$001E
    sta.w !obj_vel_x,y
    lda.w #$000A
    sta.w !obj_vel_y,y
    lda.w #$FFEC
    sta.w !obj_vel_z,y
    sep.b #$20

; @routine 08:DF4C ObjectAttachTurned
; WHAT IT DOES: An attachment turned with its parent: hidden ($08:E13C)
; when the parent is gone or loose; its heading +$15 = the parent's +$15 +
; the high byte of the parent's record +$3A, +$14 and +$16 the parent's;
; its position the parent's plus its velocity bytes turned by the
; parent's angles, times 8; rebucketed; +$36 = 3.
; HOW: RotateXY, RotateXZFrom08 and RotateXZ on the offset in direct page.
; Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; A low byte 3.
%at($08DF4C)
ObjectAttachTurned:
    ldy.w !obj_06,x
    lda.w !obj_24,y
    and.b #$08
    beq .lDF5A
    jml.l ObjectAttachHide          ; not reached in the traces so far
.lDF5A:
    lda.w !obj_25,y
    and.b #$04
    beq .lDF65
    jml.l ObjectAttachHide          ; not reached in the traces so far
.lDF65:
    phx
    phy
    rep.b #$20
    tyx
    lda.w !obj_22,x
    tax
    lda.l !gsu_in_3a,x
    xba
    sep.b #$20
    ply
    plx
    clc
    adc.w !obj_15,y
    sta.w !obj_15,x
    lda.w !obj_14,y
    sta.w !obj_14,x
    lda.w !obj_16,y
    sta.w !obj_16,x
    lda.w !obj_vel_x,x
    sta.w !wram_1F58
    lda.w !obj_vel_y,x
    sta.w !wram_1F59
    lda.w !obj_vel_z,x
    sta.w !wram_1F5A
    lda.w !wram_1F58
    sta.b !dp_02
    sta.b !dp_04
    bit.b #$80
    beq .lDFAD
    lda.b #$FF                      ; not reached in the traces so far
    sta.b !dp_05
    bra .lDFAF
.lDFAD:
    stz.b !dp_05
.lDFAF:
    lda.w !wram_1F59
    sta.b !dp_08
    sta.b !actor_0a
    bit.b #$80
    beq .lDFC0
    lda.b #$FF                      ; not reached in the traces so far
    sta.b !dp_0b
    bra .lDFC2
.lDFC0:
    stz.b !dp_0b
.lDFC2:
    lda.w !wram_1F5A
    sta.b !dp_94
    sta.w !rot_z
    bit.b #$80
    beq .lDFD5
    lda.b #$FF                      ; not reached in the traces so far
    sta.w !rot_z_high
    bra .lDFD8
.lDFD5:
    stz.w !rot_z_high
.lDFD8:
    lda.w !obj_16,y
    jsl.l RotateXY
    lda.b !dp_04
    sta.b !dp_02
    lda.b !actor_0a
    sta.b !dp_08
    lda.w !rot_z
    sta.b !dp_94
    lda.w !obj_14,y
    jsl.l RotateXZFrom08
    lda.b !dp_04
    sta.b !dp_02
    lda.b !actor_0a
    sta.b !dp_08
    lda.w !rot_z
    sta.b !dp_94
    lda.w !obj_15,y
    jsl.l RotateXZ
    rep.b #$20
    lda.b !dp_04
    asl a
    asl a
    asl a
    sta.b !dp_04
    lda.b !actor_0a
    asl a
    asl a
    asl a
    sta.b !actor_0a
    lda.w !rot_z
    asl a
    asl a
    asl a
    sta.w !rot_z
    lda.w !obj_pos_x,y
    clc
    adc.b !dp_04
    sta.w !obj_pos_x,x
    lda.w !obj_pos_z,y
    clc
    adc.w !rot_z
    sta.w !obj_pos_z,x
    lda.w !obj_pos_y,y
    clc
    adc.b !actor_0a
    sta.w !obj_pos_y,x
    sep.b #$20
    jsl.l ObjectRebucket
    lda.b #$03
    sta.w !obj_36,x
    rtl

; @routine 08:E048 ObjectSparkStart
; WHAT IT DOES: A spark's first frame: +$24 bit 3, +$3A = 0, +$5F = $6E,
; handler $08:E073 (ObjectSparkStep), +$29 = $9BBF; then its first step.
; HOW: Falls into ObjectSparkStep.
; Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectSparkStep.
%at($08E048)
ObjectSparkStart:
    lda.w !obj_24,x
    ora.b #$08
    sta.w !obj_24,x
    lda.b #$00
    sta.w !obj_3a,x
    lda.b #$6E
    sta.w !obj_5f,x
    rep.b #$20
    lda.w #$E073
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,x
    rep.b #$20
    lda.w #$9BBF
    sta.w !obj_29,x
    sep.b #$20

; @routine 08:E073 ObjectSparkStep
; WHAT IT DOES: A spark's frame: its position its parent's plus its
; velocity bytes turned by the parent's angles, times 8; the parent's
; +$14-$16 copied (ObjectCopy14From); tilted toward the camera
; (ObjectBillboardTilt); rebucketed; +$36 = 3.
; HOW: As ObjectAttachTurned without the heading's add.
; Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; A low byte 3.
%at($08E073)
ObjectSparkStep:
    ldy.w !obj_06,x
    lda.w !obj_vel_x,x
    sta.w !wram_1F58
    lda.w !obj_vel_y,x
    sta.w !wram_1F59
    lda.w !obj_vel_z,x
    sta.w !wram_1F5A
    lda.w !wram_1F58
    sta.b !dp_02
    sta.b !dp_04
    bit.b #$80
    beq .lE099
    lda.b #$FF
    sta.b !dp_05
    bra .lE09B
.lE099:
    stz.b !dp_05
.lE09B:
    lda.w !wram_1F59
    sta.b !dp_08
    sta.b !actor_0a
    bit.b #$80
    beq .lE0AC
    lda.b #$FF                      ; not reached in the traces so far
    sta.b !dp_0b
    bra .lE0AE
.lE0AC:
    stz.b !dp_0b
.lE0AE:
    lda.w !wram_1F5A
    sta.b !dp_94
    sta.w !rot_z
    bit.b #$80
    beq .lE0C1
    lda.b #$FF
    sta.w !rot_z_high
    bra .lE0C4
.lE0C1:
    stz.w !rot_z_high
.lE0C4:
    lda.w !obj_16,y
    jsl.l RotateXY
    lda.b !dp_04
    sta.b !dp_02
    lda.b !actor_0a
    sta.b !dp_08
    lda.w !rot_z
    sta.b !dp_94
    lda.w !obj_14,y
    jsl.l RotateXZFrom08
    lda.b !dp_04
    sta.b !dp_02
    lda.b !actor_0a
    sta.b !dp_08
    lda.w !rot_z
    sta.b !dp_94
    lda.w !obj_15,y
    jsl.l RotateXZ
    rep.b #$20
    lda.b !dp_04
    asl a
    asl a
    asl a
    sta.b !dp_04
    lda.b !actor_0a
    asl a
    asl a
    asl a
    sta.b !actor_0a
    lda.w !rot_z
    asl a
    asl a
    asl a
    sta.w !rot_z
    lda.w !obj_pos_x,y
    clc
    adc.b !dp_04
    sta.w !obj_pos_x,x
    lda.w !obj_pos_z,y
    clc
    adc.w !rot_z
    sta.w !obj_pos_z,x
    lda.w !obj_pos_y,y
    clc
    adc.b !actor_0a
    sta.w !obj_pos_y,x
    sep.b #$20
    jsl.l ObjectCopy14From
    txy
    jsr.w ObjectBillboardTilt
    jsl.l ObjectRebucket
    lda.b #$03
    sta.w !obj_36,x
    rtl
ObjectAttachHide:
    lda.w !obj_24,x
    ora.b #$20
    sta.w !obj_24,x
    lda.b #$03
    sta.w !obj_36,x
    rtl
ObjectAttachBit6:
    lda.w !obj_24,x
    ora.b #$40
    sta.w !obj_24,x
    lda.b #$03
    sta.w !obj_36,x
    rtl

; @routine 08:E158 ObjectCarAttachedStart
; WHAT IT DOES: A car carried on its parent (+$06): at the parent's
; position, $640 back and $1E0 up, a car record of its own
; (ObjectCarRecordNew with A = 0) marked carried (+$1C bit 3), each
; record's +$8E naming the other's record; handler $08:E1C7
; (ObjectCarAttachedStep), run at once.
; HOW: The records through X with the objects swapped in X and Y. Not
; reached in the traces so far.
; Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectCarAttachedStep.
%at($08E158)
ObjectCarAttachedStart:
    ldy.w !obj_06,x                 ; not reached in the traces so far
    jsl.l ObjectCopyPositionFrom
    rep.b #$20
    lda.w !obj_pos_z,x
    sec
    sbc.w #$0640
    sta.w !obj_pos_z,x
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_y,x
    clc
    adc.w #$FE20
    sta.w !obj_pos_y,x
    sep.b #$20
    rep.b #$20
    lda.w #$0000
    jsl.l ObjectCarRecordNew
    sep.b #$20
    ldy.w !obj_06,x
    rep.b #$20
    phx
    lda.w !obj_22,x
    tax
    lda.w !obj_22,y
    sta.l !gsu_rec_8a+4,x
    lda.l !gsu_rec_1c,x
    ora.w #$0008
    sta.l !gsu_rec_1c,x
    plx
    phx
    tyx
    ply
    phx
    lda.w !obj_22,x
    tax
    lda.w !obj_22,y
    sta.l !gsu_rec_8a+4,x
    plx
    phx
    tyx
    ply
    sep.b #$20
    rep.b #$20
    lda.w #$E1C7
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,x

; @routine 08:E1C7 ObjectCarAttachedStep
; WHAT IT DOES: A car carried on another: rebucketed, posed from its
; record (ObjectPoseFromRecord) and its record marked (GsuRecordFlag4, the
; code at $08:CD7E).
; HOW: Three calls. Reached only by unit_entries_08.
; Entry: M 8-bit, X 16-bit; X = object. Exit: as GsuRecordFlag4.
%at($08E1C7)
ObjectCarAttachedStep:
    jsl.l ObjectRebucket
    jsl.l ObjectPoseFromRecord
    jmp.w ObjectCarCD7E

; @routine 08:E1D2 ObjectExhaustStart
; WHAT IT DOES: An exhaust flame's first frame: released when its
; parent's record has +$70 bit 3, or $70:26F2 lacks bit 8, or in HUD
; modes 1-3; otherwise handler $08:E244 (ObjectExhaustStep), +$4A = the
; parent's record +$58, +$49 = 0, its Y velocity's low byte
; sign-extended and doubled, start count +$4B = $0A, state +$39 = 1; then
; its first step.
; HOW: Falls into ObjectExhaustStep; the release is ObjectRelease.
; Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectExhaustStep or
; ObjectRelease.
%at($08E1D2)
ObjectExhaustStart:
    phx
    rep.b #$20
    lda.w !obj_06,x
    tax
    lda.w !obj_22,x
    tax
    lda.l !gsu_rec_70,x
    plx
    and.w #$0008
    bne .lE201
    lda.l !gsu_control_26f2
    and.w #$0100
    beq .lE201
    sep.b #$20
    lda.w !hud_mode
    cmp.b #$01
    beq .lE201
    cmp.b #$02
    beq .lE201
    cmp.b #$03
    bne .lE208
.lE201:
    sep.b #$20
    jsl.l ObjectRelease
    rtl
.lE208:
    rep.b #$20
    lda.w #$E244
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,x
    jsr.w GsuRecordWord58
    sta.w !obj_4a,x
    lda.b #$00
    sta.w !obj_49,x
    rep.b #$20
    lda.w !obj_vel_y,x
    bit.w #$0080
    beq .lE231
    ora.w #$FF00
    bra .lE234
.lE231:
    and.w #$00FF                    ; not reached in the traces so far
.lE234:
    asl a
    sta.w !obj_vel_y,x
    sep.b #$20
    lda.b #$0A
    sta.w !obj_4b,x
    lda.b #$01
    sta.w !obj_36+3,x

; @routine 08:E244 ObjectExhaustStep
; WHAT IT DOES: An exhaust flame's frame by its state +$39: after its
; start count +$4B, +$48 = the parent's record +$88 - 1; hidden (+$24 bit
; 5) while the parent is gone or loose (+$25 bits 3, 4) or its record has
; +$40 bit 8 (ObjectParentRecordBit8). State 1: a changed +$88 starts
; state 2 for 8 frames; a drop in the parent's record +$58 of 2 to 9 since
; the last ($1F2B, +$4A) starts state 3 for 8 frames with sound $3B.
; State 2 shows frame +$88 - 1, state 3 frame 4 (+$18 with bit 7). While
; +$49 counts down, every fourth frame it is at the parent's position,
; moved by its velocity, +$24 bit 7 set when the parent's link chain's
; end has bit 14 (there it is rebucketed at the link's +$08, +$0A, +$0C
; offset from (0, -$64, $1F4)), else rebucketed; back to state 1 at 0.
; +$36 = 3.
; HOW: GsuRecordWord88 and GsuRecordWord58 read the parent's record; 8-bit
; with 16-bit position adds.
; Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; A low byte 3.
%at($08E244)
ObjectExhaustStep:
    lda.w !obj_4b,x
    bne .lE24D
    jml.l .lE256
.lE24D:
    dec.w !obj_4b,x
    jsr.w GsuRecordWord88
    sta.w !obj_48,x
.lE256:
    ldy.w !obj_06,x
    lda.w !obj_25,y
    and.b #$08
    beq .lE264
    jml.l .lE3E0
.lE264:
    lda.w !obj_25,y
    and.b #$10
    beq .lE26F
    jml.l .lE3E0                    ; not reached in the traces so far
.lE26F:
    jsr.w ObjectParentRecordBit8
    bcc .lE277
    brl .lE3E0
.lE277:
    lda.w !obj_36+3,x
    cmp.b #$01
    beq .lE282
    jml.l .lE2CD
.lE282:
    jsr.w GsuRecordWord88
    cmp.w !obj_48,x
    beq .lE29E
    bpl .lE291
    sta.w !obj_48,x
    bra .lE294
.lE291:
    sta.w !obj_48,x
.lE294:
    lda.b #$02
    sta.w !obj_36+3,x
    lda.b #$08
    sta.w !obj_49,x
.lE29E:
    jsr.w GsuRecordWord58
    sta.w !wram_1F2B
    bcs .lE2C7
    sec
    sbc.w !obj_4a,x
    bpl .lE2C7
    eor.b #$FF
    inc a
    cmp.b #$02
    bmi .lE2CD
    cmp.b #$0A
    bpl .lE2C7
    lda.b #$03
    sta.w !obj_36+3,x
    lda.b #$08
    sta.w !obj_49,x
    lda.b #$3B
    jsl.l SoundPort0QueuePush
.lE2C7:
    lda.w !wram_1F2B
    sta.w !obj_4a,x
.lE2CD:
    lda.w !obj_36+3,x
    cmp.b #$02
    beq .lE2D8
    jml.l .lE2E6
.lE2D8:
    jsr.w GsuRecordWord88
    sta.w !wram_1F58
    lda.w !wram_1F58
    ora.b #$80
    sta.w !obj_18,x
.lE2E6:
    lda.w !obj_36+3,x
    cmp.b #$03
    beq .lE2F1
    jml.l .lE2FE
.lE2F1:
    lda.b #$04
    ora.b #$80
    sta.w !obj_18,x
    jsr.w GsuRecordWord58
    sta.w !obj_4a,x
.lE2FE:
    lda.w !obj_49,x
    bne .lE306
    brl .lE3E0
.lE306:
    dec a
    beq .lE315
    sta.w !obj_49,x
    and.b #$03
    bne .lE313
    brl .lE3E0
.lE313:
    bra .lE320
.lE315:
    sta.w !obj_49,x
    lda.b #$01
    sta.w !obj_36+3,x
    jmp.w .lE3E0
.lE320:
    lda.w !obj_24,x
    and.b #$DF
    sta.w !obj_24,x
    ldy.w !obj_06,x
    jsl.l ObjectCopyPositionFrom
    jsl.l ObjectApplyVelocity
    lda.w !obj_24,x
    and.b #$7F
    sta.w !obj_24,x
    rep.b #$20
    lda.w !obj_link_w,y
    tay
    lda.w !obj_link_w,y
    tay
    lda.w !dp_08,y
    sta.w !wram_1F4C
    lda.w !actor_0a,y
    sta.w !wram_1F4E
    lda.w !actor_0c,y
    sta.w !wram_1F50
    lda.w !bank0_base,y
    and.w #$4000
    bne .lE362
    brl .lE3D8
.lE362:
    lda.w #$0000
    sta.w !obj_pos_x,x
    lda.w #$FF9C
    sta.w !obj_pos_y,x
    lda.w #$01F4
    sta.w !obj_pos_z,x
    sep.b #$20
    lda.w !obj_24,x
    ora.b #$80
    sta.w !obj_24,x
    rep.b #$20
    lda.w !obj_pos_x,x
    clc
    adc.w !wram_1F4C
    sta.w !obj_pos_x,x
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_y,x
    clc
    adc.w !wram_1F4E
    sta.w !obj_pos_y,x
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_z,x
    clc
    adc.w !wram_1F50
    sta.w !obj_pos_z,x
    sep.b #$20
    jsl.l ObjectRebucket
    rep.b #$20
    lda.w !obj_pos_x,x
    sec
    sbc.w !wram_1F4C
    sta.w !obj_pos_x,x
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_y,x
    sec
    sbc.w !wram_1F4E
    sta.w !obj_pos_y,x
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_z,x
    sec
    sbc.w !wram_1F50
    sta.w !obj_pos_z,x
    sep.b #$20
    bra .lE3E8
.lE3D8:
    sep.b #$20
    jsl.l ObjectRebucket
    bra .lE3E8
.lE3E0:
    lda.w !obj_24,x
    ora.b #$20
    sta.w !obj_24,x
.lE3E8:
    lda.b #$03
    sta.w !obj_36,x
    rtl

; @routine 08:E426 ObjectParentRecordBit8
; WHAT IT DOES: Whether object X's parent's (+$06) record has +$40 bit 8,
; in C.
; HOW: 16-bit through the parent's +$22; X kept.
; Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; Y = the parent; C
; as above; X kept, N and Z from it.
%at($08E426)
ObjectParentRecordBit8:
    ldy.w !obj_06,x
    phx
    rep.b #$20
    lda.w !obj_22,y
    tax
    lda.l !gsu_rec_40,x
    and.w #$0100
    bne .lE43E
    sep.b #$20
    plx
    clc
    rts
.lE43E:
    sep.b #$20
    plx
    sec
    rts

; @routine 08:C124 PlayerCarEvents
; WHAT IT DOES: The player car X's per-frame events from its Super FX
; record: unless its +$25 has bit 4 or the record +$40 bit 8: with $1AEC
; bit 5, the engine sound at its speed (SoundAtRateForLevel) when one of
; its parts' records (the +$3E/+$34 chain) is a loose wheel (+$2C bit 7,
; +$46 bits 0 and 4), else the latch for record +$92 bit 5 (sounds $12 /
; $0C). Then the latches for +$42 bit 10 (sounds $23 / $24) and for a
; landing (+$42 bit 13 with +$40 bit 11, not bit 8: sounds $0D / $0B); the
; held sound by +$57 (stop, $32-$35); a hit by +$94 (sounds $54-$57 by
; size, sparks from $50 on); sounds $0A / $0D for +$2C bits 8 / 9; a
; scrape by -$62 (sounds $16-$19, from $46 on +$25 bit 5 and an attached
; object); a crash by +$40 bit 0 (the code at $08:A034 with A = 1, then
; unless it says so (C) +$25 bit 4 and a burst for one player), bit 6
; (+$25 bit 5, sparks, $08:A034 with A = 0) or bit 5 ($08:A034 with A =
; 1, a burst); unless the car's +$24 has bit 6, trails for +$40 bit 1
; (and a spray chance) or, 14 in 256, for +$42 bit 10; a record +$82 of
; $199 or more latches bit 5 once ($00:1FFE = $11, sound $3D) and may
; spray. Last its +$24 bit 5 follows +$25 bits 2 and 7 and bit 6 the
; link chain's end (bit 14).
; HOW: The car in $1F18; the record in X; the latches through $1A5D; the
; code at $08:A034 runs as original code.
; Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; X = the object.
%at($08C124)
PlayerCarEvents:
    stx.w !draw_side+17
    rep.b #$20
    phx
    txy
    lda.w !obj_22,x
    tax
    lda.w !obj_25,y
    and.w #$0010
    beq .lC13B
    jml.l .lC554
.lC13B:
    lda.l !gsu_rec_40,x
    and.w #$0100
    beq .lC147
    brl .lC213
.lC147:
    lda.w !wram_1AEC
    and.w #$0020
    bne .lC152
    brl .lC1A6
.lC152:
    stz.w !wram_1F4C
    phx
    lda.l !gsu_in_3e,x
.lC15A:
    tax
    lda.l !gsu_dest,x
    bit.w #$0080
    beq .lC178
    lda.l !gsu_speed_44+2,x
    bit.w #$0001
    beq .lC178
    bit.w #$0010
    beq .lC178
    lda.w #$0001
    sta.w !wram_1F4C
.lC178:
    lda.l !gsu_in_34,x
    bne .lC15A
    plx
    lda.w !wram_1F4C
    bne .lC187
    brl .lC213
.lC187:
    lda.w !wram_0D62
    and.w #$00FF
    bne .lC196
    lda.w #$0010
    sta.l !fx_effect_max
.lC196:
    lda.l !gsu_speeds,x
    bpl .lC1A0
    eor.w #$FFFF
    inc a
.lC1A0:
    jsr.w SoundAtRateForLevel
    jmp.w .lC213
.lC1A6:
    lda.l !gsu_in_92,x
    bit.w #$0020
    beq .lC1E2
    lda.w !current_player
    and.w #$00FF
    beq .lC1BC
    lda.w !current_player+3
    bra .lC1BF
.lC1BC:
    lda.w !current_player+1
.lC1BF:
    bit.w #$0004
    bne .lC1E0
    pha
    jsl.l PlayerSound12
    pla
    ora.w #$0004
    pha
    lda.w !current_player
    and.w #$00FF
    beq .lC1DC
    pla
    sta.w !current_player+3
    bra .lC1E0
.lC1DC:
    pla
    sta.w !current_player+1
.lC1E0:
    bra .lC213
.lC1E2:
    lda.w !current_player
    and.w #$00FF
    beq .lC1EF
    lda.w !current_player+3
    bra .lC1F2
.lC1EF:
    lda.w !current_player+1
.lC1F2:
    bit.w #$0004
    beq .lC213
    pha
    jsl.l PlayerSound0C
    pla
    and.w #$FFFB
    pha
    lda.w !current_player
    and.w #$00FF
    beq .lC20F
    pla
    sta.w !current_player+3
    bra .lC213
.lC20F:
    pla
    sta.w !current_player+1
.lC213:
    lda.l !gsu_rec_42,x
    and.w #$0400
    beq .lC24F
    lda.w !current_player
    and.w #$00FF
    beq .lC229
    lda.w !current_player+3
    bra .lC22C
.lC229:
    lda.w !current_player+1
.lC22C:
    bit.w #$0001
    bne .lC24D
    pha
    jsl.l PlayerSound23
    pla
    ora.w #$0001
    pha
    lda.w !current_player
    and.w #$00FF
    beq .lC249
    pla
    sta.w !current_player+3
    bra .lC24D
.lC249:
    pla
    sta.w !current_player+1
.lC24D:
    bra .lC280
.lC24F:
    lda.w !current_player
    and.w #$00FF
    beq .lC25C
    lda.w !current_player+3
    bra .lC25F
.lC25C:
    lda.w !current_player+1
.lC25F:
    bit.w #$0001
    beq .lC280
    pha
    jsl.l PlayerSound24
    pla
    and.w #$FFFE
    pha
    lda.w !current_player
    and.w #$00FF
    beq .lC27C
    pla
    sta.w !current_player+3
    bra .lC280
.lC27C:
    pla
    sta.w !current_player+1
.lC280:
    lda.l !gsu_rec_42,x
    and.w #$0080
    beq .lC289
.lC289:
    lda.l !gsu_rec_42,x
    bit.w #$2000
    beq .lC2D3
    lda.l !gsu_rec_40,x
    bit.w #$0800
    beq .lC2D3
    bit.w #$0100
    bne .lC2D3
    lda.w !current_player
    and.w #$00FF
    beq .lC2AD
    lda.w !current_player+3
    bra .lC2B0
.lC2AD:
    lda.w !current_player+1
.lC2B0:
    bit.w #$0002
    bne .lC2D1
    pha
    jsl.l PlayerSound0D
    pla
    ora.w #$0002
    pha
    lda.w !current_player
    and.w #$00FF
    beq .lC2CD
    pla
    sta.w !current_player+3
    bra .lC2D1
.lC2CD:
    pla
    sta.w !current_player+1
.lC2D1:
    bra .lC304
.lC2D3:
    lda.w !current_player
    and.w #$00FF
    beq .lC2E0
    lda.w !current_player+3
    bra .lC2E3
.lC2E0:
    lda.w !current_player+1
.lC2E3:
    bit.w #$0002
    beq .lC304
    pha
    jsl.l PlayerSound0B
    pla
    and.w #$FFFD
    pha
    lda.w !current_player
    and.w #$00FF
    beq .lC300
    pla
    sta.w !current_player+3
    bra .lC304
.lC300:
    pla
    sta.w !current_player+1
.lC304:
    sep.b #$20
    lda.l !gsu_speed_50+7,x
    sta.w !wram_1F5A+4
    pha
    cmp.b #$00
    bmi .lC344
    cmp.b #$01
    bpl .lC31C
    jsl.l PlayerStopHeldSound
    bra .lC344
.lC31C:
    cmp.b #$02
    bpl .lC326
    jsl.l PlayerHoldSound32
    bra .lC344
.lC326:
    cmp.b #$04
    bpl .lC330
    jsl.l PlayerHoldSound33
    bra .lC344
.lC330:
    cmp.b #$05
    bpl .lC33A
    jsl.l PlayerHoldSound34
    bra .lC344
.lC33A:
    cmp.b #$64
    bpl .lC344
    jsl.l PlayerHoldSound35
    bra .lC344
.lC344:
    pla
    rep.b #$20
    lda.l !gsu_in_94,x
    beq .lC3A3
    bmi .lC3A3
    pha
    pha
    cmp.w #$0032
    bmi .lC38E
    cmp.w #$01F4
    bpl .lC364
    lda.w #$0054                    ; not reached in the traces so far
    jsl.l SoundPort0QueuePush
    bra .lC38E
.lC364:
    cmp.w #$0320
    bpl .lC372
    lda.w #$0055                    ; not reached in the traces so far
    jsl.l SoundPort0QueuePush
    bra .lC38E
.lC372:
    cmp.w #$044C
    bpl .lC380
    lda.w #$0056                    ; not reached in the traces so far
    jsl.l SoundPort0QueuePush
    bra .lC38E
.lC380:
    cmp.w #$07D0
    bpl .lC38E
    lda.w #$0057
    jsl.l SoundPort0QueuePush
    bra .lC38E
.lC38E:
    pla
    pla
    cmp.w #$0032
    bmi .lC3A3
    nop
    phx
    sep.b #$20
    ldx.w !draw_side+17
    jsl.l ObjectSpawnSparklePair
    rep.b #$20
    plx
.lC3A3:
    lda.l !gsu_dest,x
    bit.w #$0100
    beq .lC3B5
    lda.w #$000A
    jsl.l SoundPort0QueuePush
    bra .lC3C1
.lC3B5:
    bit.w #$0200
    beq .lC3C1
    lda.w #$000D
    jsl.l SoundPort0QueuePush
.lC3C1:
    lda.l !gsu_unpack_end+2,x
    bpl .lC42E
    eor.w #$FFFF
    inc a
    pha
    cmp.w #$0041
    bmi .lC409
    cmp.w #$0050
    bpl .lC3DF
    lda.w #$0016
    jsl.l SoundPort0QueuePush
    bra .lC409
.lC3DF:
    cmp.w #$005A
    bpl .lC3ED
    lda.w #$0017
    jsl.l SoundPort0QueuePush
    bra .lC409
.lC3ED:
    cmp.w #$006E
    bpl .lC3FB
    lda.w #$0018
    jsl.l SoundPort0QueuePush
    bra .lC409
.lC3FB:
    cmp.w #$03E8
    bpl .lC409
    lda.w #$0019
    jsl.l SoundPort0QueuePush
    bra .lC409
.lC409:
    pla
    cmp.w #$0046
    bmi .lC42E
    phx
    phy
    tyx
    sep.b #$20
    lda.w !obj_25,x
    ora.b #$20
    sta.w !obj_25,x
    lda.b #$0A
    sta.w !wram_1F59
    lda.b #$00
    sta.w !wram_1F5A
    jsl.l ObjectSpawnAttached
    rep.b #$20
    ply
    plx
.lC42E:
    lda.l !gsu_rec_40,x
    and.w #$0001
    beq .lC45B
    sep.b #$20
    phx
    ldx.w !draw_side+17
    lda.b #$01
    jsl.l CarCrashParts
    bcs .lC456
    lda.w !obj_25,x
    ora.b #$10
    sta.w !obj_25,x
    lda.w !wram_0D62
    bne .lC456
    jsl.l ObjectBurstSpawn
.lC456:
    rep.b #$20
    plx
    bra .lC4A2
.lC45B:
    lda.l !gsu_rec_40,x
    and.w #$0040
    beq .lC481
    phx
    sep.b #$20
    ldx.w !draw_side+17
    lda.w !obj_25,x
    ora.b #$20
    sta.w !obj_25,x
    jsl.l ObjectSpawnSparklePair
    lda.b #$00
    jsl.l CarCrashParts
    rep.b #$20
    plx
    bra .lC4A2
.lC481:
    lda.l !gsu_rec_40,x
    and.w #$0020
    beq .lC4A2
    phx
    sep.b #$20
    ldx.w !draw_side+17
    lda.b #$01
    jsl.l CarCrashParts
    lda.w !wram_0D62
    bne .lC49F
    jsl.l ObjectBurstSpawn
.lC49F:
    rep.b #$20
    plx
.lC4A2:
    ldy.w !draw_side+17
    lda.w !obj_24,y
    and.w #$0040
    beq .lC4B0
    brl .lC554
.lC4B0:
    lda.l !gsu_rec_40,x
    and.w #$0002
    beq .lC4CB
    phx
    sep.b #$20
    ldx.w !draw_side+17
    jsl.l ObjectSpawnTrail
    jsr.w ObjectSpawnD460UnlessFlag
    rep.b #$20
    plx
    bra .lC4EF
.lC4CB:
    lda.l !gsu_rec_42,x
    and.w #$0400
    beq .lC4EF
    sep.b #$20
    jsl.l RandomStepLong
    cmp.b #$F2
    bcs .lC4E2
    jml.l .lC4EF
.lC4E2:
    phx
    sep.b #$20
    ldx.w !draw_side+17
    jsl.l ObjectSpawnTrail
    rep.b #$20
    plx
.lC4EF:
    rep.b #$20
    lda.l !gsu_rec_82,x
    cmp.w #$0199
    bmi .lC554
    sep.b #$20
    lda.w !current_player
    and.b #$FF
    beq .lC508
    lda.w !current_player+3
    bra .lC50B
.lC508:
    lda.w !current_player+1
.lC50B:
    bit.b #$20
    bne .lC533
    pha
    pha
    lda.b #$11
    sta.l !save_bad_flag
    pla
    lda.b #$3D
    jsl.l SoundPort0QueuePush
    pla
    ora.b #$20
    pha
    lda.w !current_player
    and.b #$FF
    beq .lC52F
    pla
    sta.w !current_player+3
    bra .lC533
.lC52F:
    pla
    sta.w !current_player+1
.lC533:
    phx
    ldx.w !draw_side+17
    lda.w !obj_25,x
    and.b #$04
    beq .lC542
    jml.l .lC551
.lC542:
    jsl.l RandomStepLong
    cmp.b #$4C
    bcs .lC54E
    jml.l .lC551
.lC54E:
    jsr.w ObjectSpawnD460UnlessFlag
.lC551:
    rep.b #$20
    plx
.lC554:
    plx
    sep.b #$20
    lda.w !obj_25,x
    and.b #$04
    beq .lC562
    jml.l .lC577
.lC562:
    lda.w !obj_25,x
    and.b #$80
    beq .lC56D
    jml.l .lC577
.lC56D:
    lda.w !obj_24,x
    and.b #$DF
    sta.w !obj_24,x
    bra .lC57F
.lC577:
    lda.w !obj_24,x
    ora.b #$20
    sta.w !obj_24,x
.lC57F:
    ldy.w !obj_link_w,x
    rep.b #$20
    lda.w !obj_link_w,y
    tay
    lda.w !bank0_base,y
    and.w #$4000
    sep.b #$20
    beq .lC59C
    lda.w !obj_24,x
    ora.b #$40
    sta.w !obj_24,x
    bra .lC5A4
.lC59C:
    lda.w !obj_24,x
    and.b #$BF
    sta.w !obj_24,x
.lC5A4:
    rtl

; @routine 08:E443 ObjectOffsetOnParent2
; WHAT IT DOES: An attachment placed at its velocity bytes turned by its
; parent's angles, times 2: hidden ($08:E13C) when the parent's +$24 has
; bit 3 or 5; +$18 = $80; then +$24 bit 5 cleared and rebucketed.
; HOW: As ObjectAttachTurned's placement with one shift. No caller found;
; reached only by unit calls.
; Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; as ObjectRebucket.
%at($08E443)
ObjectOffsetOnParent2:
    ldy.w !obj_06,x
    lda.w !obj_24,y
    and.b #$08
    beq .lE451
    jml.l ObjectAttachHide
.lE451:
    lda.w !obj_24,y
    and.b #$20
    beq .lE45C
    jml.l ObjectAttachHide
.lE45C:
    lda.b #$00
    ora.b #$80
    sta.w !obj_18,x
    lda.w !obj_vel_x,x
    sta.w !wram_1F58
    lda.w !obj_vel_y,x
    sta.w !wram_1F59
    lda.w !obj_vel_z,x
    sta.w !wram_1F5A
    lda.w !wram_1F58
    sta.b !dp_02
    sta.b !dp_04
    bit.b #$80
    beq .lE486
    lda.b #$FF
    sta.b !dp_05
    bra .lE488
.lE486:
    stz.b !dp_05
.lE488:
    lda.w !wram_1F59
    sta.b !dp_08
    sta.b !actor_0a
    bit.b #$80
    beq .lE499
    lda.b #$FF
    sta.b !dp_0b
    bra .lE49B
.lE499:
    stz.b !dp_0b
.lE49B:
    lda.w !wram_1F5A
    sta.b !dp_94
    sta.w !rot_z
    bit.b #$80
    beq .lE4AE
    lda.b #$FF
    sta.w !rot_z_high
    bra .lE4B1
.lE4AE:
    stz.w !rot_z_high
.lE4B1:
    lda.w !obj_16,y
    jsl.l RotateXY
    lda.b !dp_04
    sta.b !dp_02
    lda.b !actor_0a
    sta.b !dp_08
    lda.w !rot_z
    sta.b !dp_94
    lda.w !obj_14,y
    jsl.l RotateXZFrom08
    lda.b !dp_04
    sta.b !dp_02
    lda.b !actor_0a
    sta.b !dp_08
    lda.w !rot_z
    sta.b !dp_94
    lda.w !obj_15,y
    jsl.l RotateXZ
    rep.b #$20
    lda.b !dp_04
    asl a
    sta.b !dp_04
    lda.b !actor_0a
    asl a
    sta.b !actor_0a
    lda.w !rot_z
    asl a
    sta.w !rot_z
    lda.w !obj_pos_x,y
    clc
    adc.b !dp_04
    sta.w !obj_pos_x,x
    lda.w !obj_pos_z,y
    clc
    adc.w !rot_z
    sta.w !obj_pos_z,x
    lda.w !obj_pos_y,y
    clc
    adc.b !actor_0a
    sta.w !obj_pos_y,x
    sep.b #$20
    lda.w !obj_24,x
    and.b #$DF
    sta.w !obj_24,x
    jsl.l ObjectRebucket
    rtl

; @routine 08:E51E ObjectLampAnimate
; WHAT IT DOES: A lamp's look by $1A5B bits 2-4: 4, 8, $0C, $1C step its
; count +$4B up to 4 through the looks at $08:E57F, $08:E5AD, $08:E5DB,
; $08:E609; $10 sets look $9AAF; anything else clears the count; then its
; frame on the parent (ObjectLampOnParent).
; HOW: Compare chain; the paths for $0C and $1C are not reached in the
; traces so far.
; Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectLampOnParent.
%at($08E51E)
ObjectLampAnimate:
    lda.w !seq_stage+38
    and.b #$1C
    cmp.b #$08
    bne .lE52A
    brl .lE589
.lE52A:
    cmp.b #$04
    bne .lE531
    brl .lE55B
.lE531:
    cmp.b #$0C
    bne .lE538
    brl .lE5B7                      ; not reached in the traces so far
.lE538:
    cmp.b #$1C
    bne .lE53F
    brl .lE5E5                      ; not reached in the traces so far
.lE53F:
    cmp.b #$10
    bne .lE546
    brl .lE54E
.lE546:
    lda.b #$00
    sta.w !obj_4b,x
    jmp.w ObjectLampOnParent
.lE54E:
    rep.b #$20
    lda.w #$9AAF
    sta.w !obj_5d,x
    sep.b #$20
    jmp.w ObjectLampOnParent
.lE55B:
    lda.w !obj_4b,x
    cmp.b #$04
    bne .lE565
    brl ObjectLampOnParent
.lE565:
    inc a
    sta.w !obj_4b,x
    phx
    asl a
    sep.b #$10
    tax
    rep.b #$20
    lda.l .looks_04,x
    rep.b #$10
    plx
    sta.w !obj_5d,x
    sep.b #$20
    jmp.w ObjectLampOnParent
.looks_04:                          ; word 0 is never read (the count is 1-4)
    dw $9AAF, $9AB3, $9AB5, $9AB3, $9AAF
.lE589:
    lda.w !obj_4b,x
    cmp.b #$04
    bne .lE593
    brl ObjectLampOnParent
.lE593:
    inc a
    sta.w !obj_4b,x
    phx
    asl a
    sep.b #$10
    tax
    rep.b #$20
    lda.l .looks_08,x
    rep.b #$10
    plx
    sta.w !obj_5d,x
    sep.b #$20
    jmp.w ObjectLampOnParent
.looks_08:                          ; word 0 is never read (the count is 1-4)
    dw $9AAF, $9AB3, $9AB5, $9AB7, $9AB1
.lE5B7:
    lda.w !obj_4b,x                 ; not reached in the traces so far
    cmp.b #$04
    bne .lE5C1
    brl ObjectLampOnParent
.lE5C1:
    inc a
    sta.w !obj_4b,x
    phx
    asl a
    sep.b #$10
    tax
    rep.b #$20
    lda.l .looks_0C,x
    rep.b #$10
    plx
    sta.w !obj_5d,x
    sep.b #$20
    jmp.w ObjectLampOnParent
.looks_0C:                          ; word 0 is never read (the count is 1-4)
    dw $9AB1, $9AB7, $9AB5, $9AB3, $9AAF
.lE5E5:
    lda.w !obj_4b,x                 ; not reached in the traces so far
    cmp.b #$04
    bne .lE5EF
    brl ObjectLampOnParent
.lE5EF:
    inc a
    sta.w !obj_4b,x
    phx
    asl a
    sep.b #$10
    tax
    rep.b #$20
    lda.l .looks_1C,x
    rep.b #$10
    plx
    sta.w !obj_5d,x
    sep.b #$20
    jmp.w ObjectLampOnParent
.looks_1C:                          ; word 0 is never read (the count is 1-4)
    dw $9AB1, $9AB7, $9AB5, $9AB7, $9AB1

; @routine 08:E613 ObjectLampStart
; WHAT IT DOES: A lamp's first frame: its +$26 from its parent's; then as
; ObjectLampWait.
; HOW: Falls into ObjectLampWait.
; Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectLampWait.
%at($08E613)
ObjectLampStart:
    ldy.w !obj_06,x
    lda.w !obj_26,y
    sta.w !obj_26,x

; @routine 08:E61C ObjectLampWait
; WHAT IT DOES: A lamp waiting: with $1A5B bit 5 its count +$4B = 0 and
; handler $08:E51E (ObjectLampAnimate), run at once; otherwise look $9AAF,
; handler $08:E653 (ObjectLampOnParent), run at once.
; HOW: Falls into the chosen handler.
; Entry: M 8-bit, X 16-bit; X = object. Exit: as the handler leaves it.
%at($08E61C)
ObjectLampWait:
    lda.w !seq_stage+38
    and.b #$20
    beq .lE63A
    lda.b #$00
    sta.w !obj_4b,x
    rep.b #$20
    lda.w #$E51E
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,x
    jmp.w ObjectLampAnimate
.lE63A:
    rep.b #$20
    lda.w #$9AAF
    sta.w !obj_5d,x
    sep.b #$20
    rep.b #$20
    lda.w #$E653
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,x

; @routine 08:E653 ObjectLampOnParent
; WHAT IT DOES: A lamp's frame on its parent (+$06): hidden ($08:E13C)
; when the parent is gone (+$24 bit 3); when the parent is loose (+$25 bit
; 2) it drops ($08:E6BE); with +$25 bit 5 it bounces ($08:E712,
; ObjectLampDropBStart); with bit 6 it takes look $9AB1 and drops;
; otherwise placed on the parent ($08:E682).
; HOW: Byte tests through the data bank.
; Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; as the path leaves
; it.
%at($08E653)
ObjectLampOnParent:
    ldy.w !obj_06,x
    lda.w !obj_24,y
    and.b #$08
    beq .lE661
    jml.l ObjectAttachHide
.lE661:
    lda.w !obj_25,y
    and.b #$04
    beq .lE66C
    jml.l ObjectLampDropStart
.lE66C:
    lda.w !obj_25,y
    and.b #$20
    beq .lE677
    jml.l ObjectLampDropBStart
.lE677:
    lda.w !obj_25,y
    and.b #$40
    beq ObjectLampPlace
    jml.l ObjectLampDropLook
ObjectLampPlace:
    ldy.w !obj_06,x
    phx
    tyx
    ply
    jsr.w ObjectPlaceOnParent
    tyx
    jsl.l ObjectRebucket
    lda.w !obj_24,x
    and.b #$DF
    sta.w !obj_24,x
    lda.w !obj_24,x
    and.b #$BF
    sta.w !obj_24,x
    ldy.w !obj_06,x
    lda.w !obj_24,y
    and.b #$40
    beq .lE6AE
    jml.l ObjectAttachBit6
.lE6AE:
    lda.b #$03
    sta.w !obj_36,x
    rtl
ObjectLampDropLook:
    rep.b #$20
    lda.w #$9AB1
    sta.w !obj_5d,x
    sep.b #$20
ObjectLampDropStart:
    ldy.w !obj_06,x
    lda.w !obj_25,y
    and.b #$10
    beq .lE6CC
    jml.l ObjectReleaseLong
.lE6CC:
    rep.b #$20
    lda.w #$E6DE
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,x
    stz.w !obj_49,x

; @routine 08:E6DE ObjectLampDrop
; WHAT IT DOES: A dropping lamp: +$48 up $10; the count +$5F stepped
; (ObjectCountStep5F with $1F58 = 8, $1F59 = $10, $1F5A = 5); when done,
; +$48 = 0 and handler $08:E61C (ObjectLampWait); placed on its parent
; either way.
; HOW: Ends in the placement at $08:E682.
; Entry: M 8-bit, X 16-bit; X = object. Exit: as the placement leaves it.
%at($08E6DE)
ObjectLampDrop:
    lda.w !obj_48,x
    clc
    adc.b #$10
    sta.w !obj_48,x
    lda.b #$08
    sta.w !wram_1F58
    lda.b #$10
    sta.w !wram_1F59
    lda.b #$05
    sta.w !wram_1F5A
    jsr.w ObjectCountStep5F
    bcc ObjectLampPlace
    lda.b #$00
    sta.w !obj_48,x
    rep.b #$20
    lda.w #$E61C
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,x
    jmp.w ObjectLampPlace

; @routine 08:E712 ObjectLampDropBStart
; WHAT IT DOES: A lamp starts bouncing: handler $08:E72C
; (ObjectLampDropB), +$49 = 0, +$25 bit 0; then its first frame.
; HOW: Falls into ObjectLampDropB. Reached only by unit_entries_08.
; Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectLampDropB.
%at($08E712)
ObjectLampDropBStart:
    rep.b #$20
    lda.w #$E72C
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,x
    stz.w !obj_49,x
    lda.w !obj_25,x
    ora.b #$01
    sta.w !obj_25,x

; @routine 08:E72C ObjectLampDropB
; WHAT IT DOES: A bouncing lamp: the count +$5F stepped
; (ObjectCountStep5F with $1F58 = 3, $1F59 = 6, $1F5A = $0A); when done,
; +$49 = 0, handler $08:E61C (ObjectLampWait) and +$25 bit 0 cleared;
; placed on its parent either way.
; HOW: Ends in the placement at $08:E682. Reached only by
; unit_entries_08.
; Entry: M 8-bit, X 16-bit; X = object. Exit: as the placement leaves it.
%at($08E72C)
ObjectLampDropB:
    lda.b #$03
    sta.w !wram_1F58
    lda.b #$06
    sta.w !wram_1F59
    lda.b #$0A
    sta.w !wram_1F5A
    jsr.w ObjectCountStep5F
    bcs .lE743
    brl ObjectLampPlace
.lE743:
    stz.w !obj_49,x
    rep.b #$20
    lda.w #$E61C
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,x
    lda.w !obj_25,x
    and.b #$FE
    sta.w !obj_25,x
    jmp.w ObjectLampPlace

; @routine 08:9A4F CameraPointFromTable
; WHAT IT DOES: Steps the camera point of object X (its link +$4F, a
; pointer Y): unless the point's word has bit 5, when the object's +$25
; bit 0 is set (sound $24) the point's index ($34,Y) moves one through the
; course's list (from $09:8090 + $32,Y: count, start), back when +$25
; bit 1 asks; then the point's flags ($00,Y) and target offset ($02, $08,
; $94) from the list's entry (index * 8 + 2; entry 0 while the parent is
; loose, with +$27 = 7). A point with bit 7 counts the object's +$27
; down, with flags $8201 and offset (0, 0, -$32), or at 0 gets bit 14.
; The target is the offset plus the point's +$26/+$28/+$2A and
; +$2C/+$2E/+$30; the point's position (+$1A, +$1C, +$1E) moves a quarter
; of the way (ApproachQuarterMin4) and the target is kept at +$20, +$22,
; +$24.
; HOW: The point through direct page + X with X and Y swapped (PHX, TYX,
; PLY) and swapped back at the end.
; Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; X = the object, Y
; = the point.
%at($089A4F)
CameraPointFromTable:
    ldy.w !obj_link_w,x
    rep.b #$20
    lda.w !bank0_base,y
    and.w #$0020
    sep.b #$20
    bne .l9ACD
    lda.w !obj_25,x
    and.b #$01
    bne .l9A69
    jml.l .l9ACD
.l9A69:
    lda.w !obj_25,x
    and.b #$FE
    sta.w !obj_25,x
    lda.b #$24
    jsl.l SoundPort0QueuePush
    phx
    rep.b #$20
    lda.w !dp_32,y
    tax
    sep.b #$20
    lda.l !attach_values,x
    sta.w !wram_1F2B
    lda.l !attach_values+1,x
    sta.w !wram_1F2D
    plx
    lda.w !obj_25,x
    and.b #$02
    bne .l9A9A
    jml.l .l9AB5
.l9A9A:
    lda.w !obj_25,x
    and.b #$FD
    sta.w !obj_25,x
    lda.w !wram_1F2D
    beq .l9AB5
    clc
    adc.w !wram_1F2B
    dec a
    cmp.w !dp_34,y
    bne .l9AC0
    lda.b #$00
    bra .l9AC0
.l9AB5:
    lda.w !dp_34,y
    inc a
    cmp.w !wram_1F2B
    bmi .l9AC0
    lda.b #$00
.l9AC0:
    sta.w !dp_34,y
    rep.b #$20
    lda.w #$0007
    sta.w !obj_27,x
    sep.b #$20
.l9ACD:
    phy
    ldy.w !obj_06,x
    lda.w !obj_25,y
    and.b #$04
    bne .l9ADC
    jml.l .l9AEF
.l9ADC:
    ply
    rep.b #$20
    lda.w #$0007
    sta.w !obj_27,x
    sep.b #$20
    rep.b #$20
    phx
    ldx.w #$0000
    bra .l9B01
.l9AEF:
    ply
    phx
    rep.b #$20
    lda.w !dp_34,y
    and.w #$00FF
    asl a
    asl a
    asl a
    clc
    adc.w !dp_32,y
    tax
.l9B01:
    inx
    inx
    lda.l !attach_values,x
    sta.w !bank0_base,y
    lda.l !attach_values+2,x
    sta.b !dp_02
    lda.l !attach_values+4,x
    sta.b !dp_08
    lda.l !attach_values+6,x
    sta.b !dp_94
    plx
    phx
    tyx
    ply
    lda.b !bank0_base,x
    bit.w #$0080
    beq .l9B49
    lda.w !obj_27,y
    beq .l9B42
    dec a
    sta.w !obj_27,y
    lda.b !bank0_base,x
    ora.w #$8201
    sta.b !bank0_base,x
    stz.b !dp_02
    stz.b !dp_08
    lda.w #$FFCE
    sta.b !dp_94
    bra .l9B49
.l9B42:
    lda.b !bank0_base,x
    ora.w #$4000
    sta.b !bank0_base,x
.l9B49:
    sep.b #$20
    rep.b #$20
    lda.b !dp_02
    clc
    adc.b !fxrec_link_422,x
    sta.b !dp_02
    sep.b #$20
    rep.b #$20
    lda.b !dp_08
    clc
    adc.b !fxcar_roll,x
    sta.b !dp_08
    sep.b #$20
    rep.b #$20
    lda.b !dp_94
    clc
    adc.b !dp_2a,x
    sta.b !dp_94
    sep.b #$20
    rep.b #$20
    lda.b !dp_02
    clc
    adc.b !dp_2c,x
    sta.b !dp_02
    sep.b #$20
    rep.b #$20
    lda.b !dp_08
    clc
    adc.b !dp_2e,x
    sta.b !dp_08
    sep.b #$20
    rep.b #$20
    lda.b !dp_94
    clc
    adc.b !dp_30,x
    sta.b !dp_94
    sep.b #$20
    rep.b #$20
    lda.b !actor_kind,x
    sta.b !scale_scratch
    lda.b !dp_02
    jsl.l ApproachQuarterMin4
    sta.b !actor_kind,x
    sep.b #$20
    rep.b #$20
    lda.b !actor_1c,x
    sta.b !scale_scratch
    lda.b !dp_08
    jsl.l ApproachQuarterMin4
    sta.b !actor_1c,x
    sep.b #$20
    rep.b #$20
    lda.b !actor_1e,x
    sta.b !scale_scratch
    lda.b !dp_94
    jsl.l ApproachQuarterMin4
    sta.b !actor_1e,x
    sep.b #$20
    rep.b #$20
    lda.b !dp_02
    sta.b !actor_20,x
    sep.b #$20
    rep.b #$20
    lda.b !dp_08
    sta.b !actor_22,x
    sep.b #$20
    rep.b #$20
    lda.b !dp_94
    sta.b !actor_palette,x
    sep.b #$20
    phx
    tyx
    ply
    rtl

; @routine 08:9BD9 ObjectRingStartZ
; WHAT IT DOES: A spinning ring's first frame heading $00 with velocity
; (0, 0, -$258): in engine phase 8 its Y from the record in hand and its
; four parts made ($08:9C4F); +$36 = 3.
; HOW: NOP, then the shared start at $08:9C4F. With no record in hand
; ($70:2476 = 0) the start branches to 08:9DA8 with M still 16-bit, so
; its LDA #$03, STA $2036,X run as LDA #$9D03, ROL $20,X: that path runs
; as the original.
; Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; A low byte 3.
%at($089BD9)
ObjectRingStartZ:
    nop
    lda.b #$00
    sta.w !obj_15,x
    rep.b #$20
    lda.w #$0000
    sta.w !obj_vel_x,x
    lda.w #$0000
    sta.w !obj_vel_y,x
    lda.w #$FDA8
    sta.w !obj_vel_z,x
    sep.b #$20
    bra ObjectRingSpawn

; @routine 08:9BF7 ObjectRingStartX
; WHAT IT DOES: As ObjectRingStartZ heading $C0 with velocity (-$258, 0,
; 0).
; HOW: As ObjectRingStartZ.
; Entry: as ObjectRingStartZ. Exit: as ObjectRingStartZ.
%at($089BF7)
ObjectRingStartX:
    nop
    lda.b #$C0
    sta.w !obj_15,x
    rep.b #$20
    lda.w #$FDA8
    sta.w !obj_vel_x,x
    lda.w #$0000
    sta.w !obj_vel_y,x
    lda.w #$0000
    sta.w !obj_vel_z,x
    sep.b #$20
    bra ObjectRingSpawn

; @routine 08:9C15 ObjectRingStartBackZ
; WHAT IT DOES: As ObjectRingStartZ heading $80 with velocity (0, 0,
; $258).
; HOW: As ObjectRingStartZ.
; Entry: as ObjectRingStartZ. Exit: as ObjectRingStartZ.
%at($089C15)
ObjectRingStartBackZ:
    nop
    lda.b #$80
    sta.w !obj_15,x
    rep.b #$20
    lda.w #$0000
    sta.w !obj_vel_x,x
    lda.w #$0000
    sta.w !obj_vel_y,x
    lda.w #$0258
    sta.w !obj_vel_z,x
    sep.b #$20
    bra ObjectRingSpawn

; @routine 08:9C33 ObjectRingStartBackX
; WHAT IT DOES: As ObjectRingStartZ heading $40 with velocity ($258, 0,
; 0).
; HOW: As ObjectRingStartZ.
; Entry: as ObjectRingStartZ. Exit: as ObjectRingStartZ.
%at($089C33)
ObjectRingStartBackX:
    nop
    lda.b #$40
    sta.w !obj_15,x
    rep.b #$20
    lda.w #$0258
    sta.w !obj_vel_x,x
    lda.w #$0000
    sta.w !obj_vel_y,x
    lda.w #$0000
    sta.w !obj_vel_z,x
    sep.b #$20
ObjectRingSpawn:
    lda.w !engine_phase_p1
    cmp.b #$08
    beq .l9C59
    brl .l9DA8
.l9C59:
    rep.b #$20
    lda.l !gsu_match_2476
    bne .l9C64
    brl .l9DA8                      ; not reached in the traces so far
.l9C64:
    phx
    tax
    lda.l !gsu_in_20,x
    sta.w !wram_1F4E
    clc
    adc.w #$E4A8
    sta.w !wram_1F4C
    sep.b #$20
    plx
    rep.b #$20
    lda.w !wram_1F4C
    sta.w !obj_pos_y,x
    sep.b #$20
    rep.b #$20
    lda.w !wram_1F4E
    clc
    adc.w #$FE0C
    sta.w !wram_1F4E
    sep.b #$20
    rep.b #$20
    lda.w !wram_1F4E
    sta.w !obj_27,x
    sep.b #$20
    stz.w !wram_1F58
    rep.b #$20
    lda.w #$A660
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .l9CB0
    jml.l .l9CD6                    ; not reached in the traces so far
.l9CB0:
    rep.b #$20
    lda.w #$9DF1
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,y
    rep.b #$20
    lda.w #$FF90
    sta.w !obj_vel_x,y
    sep.b #$20
    pha
    rep.b #$20
    txa
    sta.w !obj_06,y
    sep.b #$20
    pla
    inc.w !wram_1F58
.l9CD6:
    rep.b #$20
    lda.w #$A6D0
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .l9CEA
    jml.l .l9D10                    ; not reached in the traces so far
.l9CEA:
    rep.b #$20
    lda.w #$9DF1
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,y
    rep.b #$20
    lda.w #$FFDB
    sta.w !obj_vel_x,y
    sep.b #$20
    pha
    rep.b #$20
    txa
    sta.w !obj_06,y
    sep.b #$20
    pla
    inc.w !wram_1F58
.l9D10:
    rep.b #$20
    lda.w #$A67C
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .l9D24
    jml.l .l9D4A                    ; not reached in the traces so far
.l9D24:
    rep.b #$20
    lda.w #$9DF1
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,y
    rep.b #$20
    lda.w #$0025
    sta.w !obj_vel_x,y
    sep.b #$20
    pha
    rep.b #$20
    txa
    sta.w !obj_06,y
    sep.b #$20
    pla
    inc.w !wram_1F58
.l9D4A:
    rep.b #$20
    lda.w #$A5D4
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .l9D5E
    jml.l .l9D84                    ; not reached in the traces so far
.l9D5E:
    rep.b #$20
    lda.w #$9DF1
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,y
    rep.b #$20
    lda.w #$0070
    sta.w !obj_vel_x,y
    sep.b #$20
    pha
    rep.b #$20
    txa
    sta.w !obj_06,y
    sep.b #$20
    pla
    inc.w !wram_1F58
.l9D84:
    lda.w !wram_1F58
    cmp.b #$04
    bne ObjectRingIdle
    lda.b #$40
    sta.w !obj_16,x
    lda.w !obj_15,x
    clc
    adc.b #$20
    sta.w !obj_15,x
    rep.b #$20
    lda.w #$9DBE
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,x
.l9DA8:
    lda.b #$03
    sta.w !obj_36,x
    rtl

; @routine 08:9DAE ObjectRingIdle
; WHAT IT DOES: A ring whose parts were not all made: keeps this handler
; and does nothing more.
; HOW: Stores its own address. Not reached in the traces so far.
; Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; A low byte 8.
%at($089DAE)
ObjectRingIdle:
    rep.b #$20                      ; not reached in the traces so far
    lda.w #$9DAE
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,x
    rtl

; @routine 08:9DBE ObjectRingSpin
; WHAT IT DOES: A ring's frame: moved by its velocity, its Y a quarter of
; the way to +$27 (ApproachQuarterMin4), its +$16 down 6 and +$15 down 1
; (it spins), rebucketed; +$36 = 3.
; HOW: 8-bit adds of $FA and $FF.
; Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; A low byte 3.
%at($089DBE)
ObjectRingSpin:
    jsl.l ObjectApplyVelocity
    rep.b #$20
    lda.w !obj_pos_y,x
    sta.b !scale_scratch
    lda.w !obj_27,x
    jsl.l ApproachQuarterMin4
    sta.w !obj_pos_y,x
    sep.b #$20
    lda.w !obj_16,x
    clc
    adc.b #$FA
    sta.w !obj_16,x
    lda.w !obj_15,x
    clc
    adc.b #$FF
    sta.w !obj_15,x
    jsl.l ObjectRebucket
    lda.b #$03
    sta.w !obj_36,x
    rtl

; @routine 08:9DF1 ObjectRingPartStart
; WHAT IT DOES: A ring part's first frame: count +$48 = $28, handler
; $08:9E05 (ObjectRingPartStep); then its first step.
; HOW: Falls into ObjectRingPartStep.
; Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectRingPartStep.
%at($089DF1)
ObjectRingPartStart:
    lda.b #$28
    sta.w !obj_48,x
    rep.b #$20
    lda.w #$9E05
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,x

; @routine 08:9E05 ObjectRingPartStep
; WHAT IT DOES: A ring part's frame: on its parent at its velocity bytes
; turned by the parent's +$16 (RotateXY) and +$15 (RotateXZ), times 16;
; the parent's +$16 and +$15 copied; rebucketed; +$36 = 3; released once
; its count +$48 runs out, else counted down.
; HOW: As ObjectAttachTurned's placement without the +$14 turn.
; Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; as the path leaves
; it.
%at($089E05)
ObjectRingPartStep:
    ldy.w !obj_06,x
    lda.w !obj_vel_x,x
    sta.w !wram_1F58
    lda.w !obj_vel_y,x
    sta.w !wram_1F59
    lda.w !obj_vel_z,x
    sta.w !wram_1F5A
    lda.w !wram_1F58
    sta.b !dp_02
    sta.b !dp_04
    bit.b #$80
    beq .l9E2B
    lda.b #$FF
    sta.b !dp_05
    bra .l9E2D
.l9E2B:
    stz.b !dp_05
.l9E2D:
    lda.w !wram_1F59
    sta.b !dp_08
    sta.b !actor_0a
    bit.b #$80
    beq .l9E3E
    lda.b #$FF                      ; not reached in the traces so far
    sta.b !dp_0b
    bra .l9E40
.l9E3E:
    stz.b !dp_0b
.l9E40:
    lda.w !wram_1F5A
    sta.b !dp_94
    sta.w !rot_z
    bit.b #$80
    beq .l9E53
    lda.b #$FF                      ; not reached in the traces so far
    sta.w !rot_z_high
    bra .l9E56
.l9E53:
    stz.w !rot_z_high
.l9E56:
    lda.w !obj_16,y
    jsl.l RotateXY
    lda.b !dp_04
    sta.b !dp_02
    lda.b !actor_0a
    sta.b !dp_08
    lda.w !rot_z
    sta.b !dp_94
    lda.w !obj_15,y
    jsl.l RotateXZ
    rep.b #$20
    lda.b !dp_04
    asl a
    asl a
    asl a
    asl a
    sta.b !dp_04
    lda.b !actor_0a
    asl a
    asl a
    asl a
    asl a
    sta.b !actor_0a
    lda.w !rot_z
    asl a
    asl a
    asl a
    asl a
    sta.w !rot_z
    lda.w !obj_pos_x,y
    clc
    adc.b !dp_04
    sta.w !obj_pos_x,x
    lda.w !obj_pos_z,y
    clc
    adc.w !rot_z
    sta.w !obj_pos_z,x
    lda.w !obj_pos_y,y
    clc
    adc.b !actor_0a
    sta.w !obj_pos_y,x
    sep.b #$20
    lda.w !obj_16,y
    sta.w !obj_16,x
    lda.w !obj_15,y
    sta.w !obj_15,x
    jsl.l ObjectRebucket
    lda.b #$03
    sta.w !obj_36,x
    lda.w !obj_48,x
    bne .l9EC9
    jml.l ObjectReleaseLong
.l9EC9:
    dec.w !obj_48,x
    rtl

; @routine 08:9ECD ObjectHandlerEmptyC
; WHAT IT DOES: Nothing: an empty object handler.
; HOW: RTL. Not reached in the traces so far.
; Entry: any. Exit: every register unchanged.
%at($089ECD)
ObjectHandlerEmptyC:
    rtl                             ; not reached in the traces so far

; @routine 08:9ECE ObjectMarkerStart
; WHAT IT DOES: A marker's first frame: data bank $7E, handler $08:9EFD
; (ObjectMarkerFollow), +$24 bit 3, +$3A = 0, +$5F = $64, +$04 = $A158;
; then its first frame.
; HOW: PHA, PLB for the bank; falls into ObjectMarkerFollow. Not reached
; in the traces so far.
; Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectMarkerFollow.
%at($089ECE)
ObjectMarkerStart:
    lda.b #$7E                      ; not reached in the traces so far
    pha
    plb
    rep.b #$20
    lda.w #$9EFD
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,x
    lda.w !obj_24,x
    ora.b #$08
    sta.w !obj_24,x
    lda.b #$00
    sta.w !obj_3a,x
    lda.b #$64
    sta.w !obj_5f,x
    rep.b #$20
    lda.w #$A158
    sta.w !obj_04,x
    sep.b #$20

; @routine 08:9EFD ObjectMarkerFollow
; WHAT IT DOES: A marker on the car named by the ($1168 & 3)th word of
; the table at $116C: that car's +$27 kept at $1174, the car at $1176;
; at the car's position (ObjectCopyPositionFrom) $12C higher, rebucketed,
; +$36 = 3; released when $116A is nonzero.
; HOW: 16-bit table reads through the data bank. Not reached in the traces
; so far.
; Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; as the path leaves
; it.
%at($089EFD)
ObjectMarkerFollow:
    rep.b #$20                      ; not reached in the traces so far
    lda.w !packed_time_a+5
    and.w #$0003
    asl a
    tay
    lda.w !table_116c,y
    tay
    lda.w !obj_27,y
    sty.w !table_116c+10
    sta.w !table_116c+8
    sep.b #$20
    jsl.l ObjectCopyPositionFrom
    rep.b #$20
    lda.w !obj_pos_y,x
    clc
    adc.w #$FED4
    sta.w !obj_pos_y,x
    sep.b #$20
    jsl.l ObjectRebucket
    lda.b #$03
    sta.w !obj_36,x
    lda.w !race_word_116a
    beq .l9F3A
    jsl.l ObjectRelease
.l9F3A:
    rtl

; @routine 08:9F3B ObjectDemoCarStart
; WHAT IT DOES: A demo car's first frame: a car record (ObjectCarRecordNew
; with A = 4) with +$70 bit 3 and +$16 bit 7; the camera ($1A64) on it at
; (0, -$C8, $258), mode 1; handler $08:9FB2 (ObjectDemoCarStep); its
; number in the table at $116C ($116A counted up); count +$48 = 4; then
; its first step.
; HOW: Falls into ObjectDemoCarStep.
; Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectDemoCarStep.
%at($089F3B)
ObjectDemoCarStart:
    rep.b #$20
    lda.w #$0004
    jsl.l ObjectCarRecordNew
    rep.b #$20
    lda.w !obj_22,x
    beq .l9F64
    phx
    tax
    lda.l !gsu_rec_70,x
    ora.w #$0008
    sta.l !gsu_rec_70,x
    lda.l !gsu_rec_flags,x
    ora.w #$0080
    sta.l !gsu_rec_flags,x
    plx
.l9F64:
    rep.b #$30
    ldy.w #$1A64
    txa
    sta.w !dp_02,y
    lda.w #$0000
    sta.w !actor_kind,y
    lda.w #$FF38
    sta.w !actor_1c,y
    lda.w #$0258
    sta.w !actor_1e,y
    sep.b #$20
    lda.b #$01
    sta.w !bank0_base,y
    sep.b #$20
    rep.b #$20
    lda.w #$9FB2
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,x
    lda.w !race_word_116a
    asl a
    rep.b #$20
    and.w #$00FF
    phx
    tax
    pla
    sta.l !table_116c,x
    tax
    sep.b #$20
    inc.w !race_word_116a
    lda.b #$04
    sta.w !obj_48,x

; @routine 08:9FB2 ObjectDemoCarStep
; WHAT IT DOES: A demo car's frame: its look by kind (ObjectSet5DByKind
; with $00:18D3); while its count +$48 runs it counts down, at 0 its
; record gets +$1C bit 4; rebucketed, posed from its record
; (ObjectPoseFromRecord), +$36 = 3 and the record marked (GsuRecordFlag4,
; the code at $08:CD7E).
; HOW: The record through +$22.
; Entry: M 8-bit, X 16-bit; X = object. Exit: as GsuRecordFlag4.
%at($089FB2)
ObjectDemoCarStep:
    lda.l !selection_out
    jsr.w ObjectSet5DByKind
    lda.w !obj_48,x
    bne .l9FC2
    jml.l .l9FC7
.l9FC2:
    dec.w !obj_48,x
    bra .l9FDC
.l9FC7:
    phx
    rep.b #$20
    lda.w !obj_22,x
    tax
    lda.l !gsu_rec_1c,x
    ora.w #$0010
    sta.l !gsu_rec_1c,x
    sep.b #$20
    plx
.l9FDC:
    jsl.l ObjectRebucket
    jsl.l ObjectPoseFromRecord
    lda.b #$03
    sta.w !obj_36,x
    jmp.w ObjectCarCD7E

; @routine 08:9FEC CameraOnCarNear
; WHAT IT DOES: The camera ($1A64) on object X at (0, -$1F4, $258), mode
; 1.
; HOW: Stores through Y = $1A64. Not reached in the traces so far.
; Entry: X 16-bit; X = object. Exit: M 8-bit, X 16-bit; Y = $1A64; A low
; byte 1.
%at($089FEC)
CameraOnCarNear:
    rep.b #$30                      ; not reached in the traces so far
    ldy.w #$1A64
    txa
    sta.w !dp_02,y
    lda.w #$0000
    sta.w !actor_kind,y
    lda.w #$FE0C
    sta.w !actor_1c,y
    lda.w #$0258
    sta.w !actor_1e,y
    sep.b #$20
    lda.b #$01
    sta.w !bank0_base,y
    rtl

; @routine 08:A00F CameraOnMarkedCar
; WHAT IT DOES: The camera ($1A64) on the car at $1176 at (0, -$C8, $2BC),
; mode 1.
; HOW: Stores through direct page + X = $1A64. Not reached in the traces
; so far.
; Entry: any. Exit: M 8-bit, X 16-bit; X = $1A64, Y = the car; A low byte
; 1.
%at($08A00F)
CameraOnMarkedCar:
    rep.b #$30                      ; not reached in the traces so far
    ldx.w #$1A64
    ldy.w !table_116c+10
    sty.b !dp_02,x
    lda.w #$0000
    sta.b !actor_kind,x
    lda.w #$FF38
    sta.b !actor_1c,x
    lda.w #$02BC
    sta.b !actor_1e,x
    sep.b #$20
    lda.b #$01
    sta.b !bank0_base,x
    rtl

; @routine 08:A02F DataBank7E
; WHAT IT DOES: Sets the data bank to $7E.
; HOW: LDA #$7E, PHA, PLB; ends at the RTL at $08:A033 that other code
; jumps to. Not reached in the traces so far.
; Entry: M 8-bit. Exit: data bank $7E; A low byte $7E; N clear, Z clear.
%at($08A02F)
DataBank7E:
    lda.b #$7E                      ; not reached in the traces so far
    pha
    plb
RtlA033:
    rtl

; @routine 08:89E6 CameraApproachTarget
; WHAT IT DOES: Moves object X's camera point (its link +$4F) an eighth of
; the way (at least 8: ApproachEighthMin8) from its position (+$1A, +$1C,
; +$1E) to its target (+$20, +$22, +$24); while the object's count +$27
; runs, the target moves by the object's velocity and C is clear;
; otherwise the object's path's next command (ObjectPathCommand) decides
; C.
; HOW: The point through the data bank with Y; 16-bit.
; Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; C as above; Y = the
; point.
%at($0889E6)
CameraApproachTarget:
    ldy.w !obj_link_w,x
    rep.b #$20
    lda.w !actor_kind,y
    sta.b !scale_scratch
    lda.w !actor_20,y
    jsl.l ApproachEighthMin8
    sta.w !actor_kind,y
    sep.b #$20
    rep.b #$20
    lda.w !actor_1c,y
    sta.b !scale_scratch
    lda.w !actor_22,y
    jsl.l ApproachEighthMin8
    sta.w !actor_1c,y
    sep.b #$20
    rep.b #$20
    lda.w !actor_1e,y
    sta.b !scale_scratch
    lda.w !actor_palette,y
    jsl.l ApproachEighthMin8
    sta.w !actor_1e,y
    sep.b #$20
    rep.b #$20
    lda.w !obj_27,x
    beq .l8A4F
    dec a
    sta.w !obj_27,x
    lda.w !obj_vel_x,x
    clc
    adc.w !actor_20,y
    sta.w !actor_20,y
    lda.w !obj_vel_y,x
    clc
    adc.w !actor_22,y
    sta.w !actor_22,y
    lda.w !obj_vel_z,x
    clc
    adc.w !actor_palette,y
    sta.w !actor_palette,y
    sep.b #$20
    bra .l8A57
.l8A4F:
    sep.b #$20
    jsl.l ObjectPathCommand
    bcs .l8A58
.l8A57:
    clc
.l8A58:
    rtl

; @routine 08:958F CameraFollowLink
; WHAT IT DOES: Steps object X's camera point (its link +$4F): it follows
; its target (CameraFollowStep) and faces it (ObjectFaceTied).
; HOW: The point in X between them; X kept (PHX, PLX).
; Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; X = the object.
%at($08958F)
CameraFollowLink:
    phx
    ldy.w !obj_link_w,x
    tyx
    jsl.l CameraFollowStep
    jsl.l ObjectFaceTied
    plx
    sep.b #$20
    rtl

; @routine 08:8A59 CameraDirectorFixedStart
; WHAT IT DOES: The replay camera fixed: handler $08:8A75
; (CameraDirectorFixed), mode 2; then its first frame.
; HOW: Falls into CameraDirectorFixed. Not reached in the traces so far.
; Entry: M 8-bit, X 16-bit; X = object. Exit: as CameraDirectorFixed.
%at($088A59)
CameraDirectorFixedStart:
    rep.b #$20                      ; not reached in the traces so far
    lda.w #$8A75
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,x
    ldy.w !obj_link_w,x
    rep.b #$20
    lda.w #$0002
    sta.w !bank0_base,y
    sep.b #$20

; @routine 08:8A75 CameraDirectorFixed
; WHAT IT DOES: The fixed replay camera: the point's $04 = 0 and $06 =
; $7D0; the point's step ($08:8EF9).
; HOW: Two stores. Not reached in the traces so far.
; Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; A low byte 3.
%at($088A75)
CameraDirectorFixed:
    ldy.w !obj_link_w,x             ; not reached in the traces so far
    lda.b #$00
    sta.w !dp_04,y
    rep.b #$20
    lda.w #$07D0
    sta.w !actor_06,y
    sep.b #$20
    jmp.w CameraDirectorTail

; @routine 08:8A8A CameraNearestCar
; WHAT IT DOES: Points the camera ($1A64) at the car nearest object X: on
; its first frame ($04 = 0) $04 = 1 and mode bit 0 cleared; the distance
; (half-sums of |dx|, |dy|, |dz| / 4) to the first car of the list at
; $7E:FFAE, when below the camera's $06, becomes $06 and that car the
; camera's target ($02) with object X's position at $08/$0A/$0C and mode
; bit 0 set. +$36 = 3. With no car taken, A = $9D03 and the word at
; direct page $20 + X is rotated left through C instead.
; HOW: Only the list's first entry is looked at. With no car taken the
; code reaches $08:8B55 with A 16-bit, where its bytes read LDA #$9D03,
; ROL $20,X, RTL.
; Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit, A low byte 3; with
; no car taken M 16-bit, A = $9D03, N, Z, C from the rotate.
%at($088A8A)
CameraNearestCar:
    ldy.w #$1A64
    lda.w !dp_04,y
    bne .l8AA4
    lda.b #$01
    sta.w !dp_04,y
    rep.b #$20
    lda.w !bank0_base,y
    and.w #$FFFE
    sta.w !bank0_base,y
    sep.b #$20
.l8AA4:
    rep.b #$20
    phx
    lda.w !obj_pos_x,x
    sta.w !wram_1F4C
    lda.w !obj_pos_y,x
    sta.w !wram_1F4E
    lda.w !obj_pos_z,x
    sta.w !wram_1F50
    stz.w !wram_1F52
    lda.w !actor_06,y
    sta.w !wram_1F2D
    ldx.w #$0000
    lda.l !object_list_b,x
    beq .l8B26
    tay
    lda.w !obj_pos_x,y
    sec
    sbc.w !wram_1F4C
    bpl .l8AD9
    eor.w #$FFFF
    inc a
.l8AD9:
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    sta.w !wram_1F2B
    lda.w !obj_pos_y,y
    sec
    sbc.w !wram_1F4E
    bpl .l8AF1
    eor.w #$FFFF
    inc a
.l8AF1:
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    clc
    adc.w !wram_1F2B
    sta.w !wram_1F2B
    lda.w !obj_pos_z,y
    sec
    sbc.w !wram_1F50
    bpl .l8B0D
    eor.w #$FFFF
    inc a
.l8B0D:
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    clc
    adc.w !wram_1F2B
    cmp.w !wram_1F2D
    bpl .l8B24
    sta.w !wram_1F2D
    sty.w !wram_1F52
.l8B24:
    inx
    inx
.l8B26:
    plx
    ldy.w #$1A64
    lda.w !wram_1F2D
    sta.w !actor_06,y
    lda.w !wram_1F52
    beq .l8B55
    sta.w !dp_02,y
    lda.w !obj_pos_x,x
    sta.w !dp_08,y
    lda.w !obj_pos_y,x
    sta.w !actor_0a,y
    lda.w !obj_pos_z,x
    sta.w !actor_0c,y
    lda.w !bank0_base,y
    ora.w #$0001
    sta.w !bank0_base,y
    sep.b #$20
.l8B55:
    lda.b #$03
    sta.w !obj_36,x
    rtl

; @routine 08:8B5B CameraDirectorNext
; WHAT IT DOES: The replay camera on the next entry of the course's camera
; table at its place, $C8 up; handler $08:8D2D (CameraDirectorTrack), mode
; 3, still; then tracking.
; HOW: GsuTableEntryNext once. Reached only by unit_entries_08.
; Entry: M 8-bit, X 16-bit; X = object. Exit: as CameraDirectorTrack.
%at($088B5B)
CameraDirectorNext:
    rep.b #$20
    lda.w #$8D2D
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,x
    ldy.w !obj_link_w,x
    rep.b #$20
    lda.w #$0003
    sta.w !bank0_base,y
    sep.b #$20
    phx
    ldy.w !obj_06,x
    rep.b #$20
    lda.w !obj_22,y
    tax
    lda.l !gsu_rec_58,x
    plx
    jsl.l GsuTableEntryNext
    sta.w !wram_1F2B
    jsl.l GsuTableEntryTo1F4C
    ldy.w !obj_link_w,x
    lda.w !wram_1F4C
    sta.w !dp_08,y
    lda.w !wram_1F4E
    clc
    adc.w #$FF38
    sta.w !actor_0a,y
    lda.w !wram_1F50
    sta.w !actor_0c,y
    sep.b #$20
    rep.b #$20
    lda.w #$0000
    sta.w !obj_27,x
    sep.b #$20
    rep.b #$20
    lda.w #$0000
    sta.w !obj_vel_x,x
    sep.b #$20
    rep.b #$20
    lda.w #$0000
    sta.w !obj_vel_y,x
    sep.b #$20
    rep.b #$20
    lda.w #$0000
    sta.w !obj_vel_z,x
    sep.b #$20
    jmp.w CameraDirectorTrack

; @routine 08:8BD7 CameraDirectorHold
; WHAT IT DOES: The replay camera held: this handler again, mode 3, the
; point stepped twice (CameraFollowLink, then the tail at $08:8EF9).
; HOW: Two CameraFollowLink calls.
; Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; A low byte 3.
%at($088BD7)
CameraDirectorHold:
    rep.b #$20
    lda.w #$8BD7
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,x
    ldy.w !obj_link_w,x
    rep.b #$20
    lda.w #$0003
    sta.w !bank0_base,y
    sep.b #$20
    jsl.l CameraFollowLink
    jml.l CameraDirectorTail

; @routine 08:8BFB CameraDirectorSkip3
; WHAT IT DOES: The replay camera three entries on in the course's camera
; table at that place + ($3E8, -$1F4, $3E8), then tracking
; ($08:8CDA, CameraDirectorTrack).
; HOW: GsuTableEntryNext three times.
; Entry: M 8-bit, X 16-bit; X = object. Exit: as CameraDirectorTrack.
%at($088BFB)
CameraDirectorSkip3:
    phx
    ldy.w !obj_06,x
    rep.b #$20
    lda.w !obj_22,y
    tax
    lda.l !gsu_rec_58,x
    plx
    jsl.l GsuTableEntryNext
    jsl.l GsuTableEntryNext
    jsl.l GsuTableEntryNext
    sta.w !wram_1F2B
    jsl.l GsuTableEntryTo1F4C
    ldy.w !obj_link_w,x
    lda.w !wram_1F4C
    clc
    adc.w #$03E8
    sta.w !dp_08,y
    lda.w !wram_1F4E
    clc
    adc.w #$FE0C
    sta.w !actor_0a,y
    lda.w !wram_1F50
    clc
    adc.w #$03E8
    sta.w !actor_0c,y
    sep.b #$20
    jmp.w CameraDirectorTrackStart

; @routine 08:8C43 CameraDirectorSkip5
; WHAT IT DOES: The replay camera five entries on, the point's $04 = that
; entry, at its place + ($3E8, -$1F4, $3E8); then held
; (CameraDirectorHold).
; HOW: GsuTableEntryNext five times.
; Entry: M 8-bit, X 16-bit; X = object. Exit: as CameraDirectorHold.
%at($088C43)
CameraDirectorSkip5:
    phx
    ldy.w !obj_06,x
    rep.b #$20
    lda.w !obj_22,y
    tax
    lda.l !gsu_rec_58,x
    plx
    jsl.l GsuTableEntryNext
    jsl.l GsuTableEntryNext
    jsl.l GsuTableEntryNext
    jsl.l GsuTableEntryNext
    jsl.l GsuTableEntryNext
    sta.w !wram_1F2B
    jsl.l GsuTableEntryTo1F4C
    ldy.w !obj_link_w,x
    lda.w !wram_1F2B
    sta.w !dp_04,y
    lda.w !wram_1F4C
    clc
    adc.w #$03E8
    sta.w !dp_08,y
    lda.w !wram_1F4E
    clc
    adc.w #$FE0C
    sta.w !actor_0a,y
    lda.w !wram_1F50
    clc
    adc.w #$03E8
    sta.w !actor_0c,y
    sep.b #$20
    jmp.w CameraDirectorHold

; @routine 08:8C99 CameraDirectorSkip2
; WHAT IT DOES: As CameraDirectorSkip3 two entries on.
; HOW: GsuTableEntryNext twice.
; Entry: as CameraDirectorSkip3. Exit: as CameraDirectorSkip3.
%at($088C99)
CameraDirectorSkip2:
    phx
    ldy.w !obj_06,x
    rep.b #$20
    lda.w !obj_22,y
    tax
    lda.l !gsu_rec_58,x
    plx
    jsl.l GsuTableEntryNext
    jsl.l GsuTableEntryNext
    sta.w !wram_1F2B
    jsl.l GsuTableEntryTo1F4C
    ldy.w !obj_link_w,x
    lda.w !wram_1F4C
    clc
    adc.w #$03E8
    sta.w !dp_08,y
    lda.w !wram_1F4E
    clc
    adc.w #$FE0C
    sta.w !actor_0a,y
    lda.w !wram_1F50
    clc
    adc.w #$03E8
    sta.w !actor_0c,y
    sep.b #$20
CameraDirectorTrackStart:
    rep.b #$20
    lda.w #$8D2D
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,x
    ldy.w !obj_link_w,x
    rep.b #$20
    lda.w #$0003
    sta.w !bank0_base,y
    sep.b #$20
    rep.b #$20
    lda.l !fx_init_273c
    and.w #$FFFE
    sta.l !fx_init_273c
    sep.b #$20
    rep.b #$20
    lda.w #$0000
    sta.w !obj_27,x
    sep.b #$20
    rep.b #$20
    lda.w #$0000
    sta.w !obj_vel_x,x
    sep.b #$20
    rep.b #$20
    lda.w #$0000
    sta.w !obj_vel_y,x
    sep.b #$20
    rep.b #$20
    lda.w #$0000
    sta.w !obj_vel_z,x
    sep.b #$20

; @routine 08:8D2D CameraDirectorTrack
; WHAT IT DOES: The replay camera on the course's camera table: when the
; parent's record +$58 moves to a new entry (not 0, not the point's $04,
; low two bits clear) the point's $04 = it, the camera ($08, $0A, $0C) at
; the next entry's place + ($3E8, -$5DC, $3E8), +$27 = $18 and a velocity
; a thirty-second of the way to the entry after; then while +$27 runs the
; camera rises $19 a frame, and it moves by the velocity (X and Z; the Y
; add is not stored); then the point's step ($08:8EF9).
; HOW: GsuTableEntryNext and GsuTableEntryTo1F4C through the record's
; table; 16-bit.
; Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; A low byte 3.
%at($088D2D)
CameraDirectorTrack:
    phx
    ldy.w !obj_06,x
    rep.b #$20
    lda.w !obj_22,y
    tax
    lda.l !gsu_rec_58,x
    plx
    cmp.w #$0000
    bne .l8D44
    brl .l8DF4                      ; not reached in the traces so far
.l8D44:
    ldy.w !obj_link_w,x
    cmp.w !dp_04,y
    bne .l8D4F
    brl .l8DF4
.l8D4F:
    bit.w #$0003
    beq .l8D57
    brl .l8DF4
.l8D57:
    sta.w !dp_04,y
    jsl.l GsuTableEntryNext
    sta.w !wram_1F2B
    jsl.l GsuTableEntryTo1F4C
    ldy.w !obj_link_w,x
    lda.w !wram_1F4C
    clc
    adc.w #$03E8
    sta.w !dp_08,y
    lda.w !wram_1F4E
    clc
    adc.w #$FA24
    sta.w !actor_0a,y
    lda.w !wram_1F50
    clc
    adc.w #$03E8
    sta.w !actor_0c,y
    lda.w #$0018
    sta.w !obj_27,x
    lda.w !wram_1F2B
    jsl.l GsuTableEntryNext
    jsl.l GsuTableEntryTo1F4C
    ldy.w !obj_link_w,x
    lda.w !wram_1F4C
    sec
    sbc.w !dp_08,y
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    sta.w !obj_vel_x,x
    lda.w !wram_1F4E
    sec
    sbc.w !actor_0a,y
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    sta.w !obj_vel_y,x
    lda.w !wram_1F50
    sec
    sbc.w !actor_0c,y
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    sta.w !obj_vel_z,x
.l8DF4:
    lda.w !obj_27,x
    beq .l8E0A
    dec a
    sta.w !obj_27,x
    ldy.w !obj_link_w,x
    lda.w !actor_0a,y
    clc
    adc.w #$0019
    sta.w !actor_0a,y
.l8E0A:
    lda.w !dp_08,y
    clc
    adc.w !obj_vel_x,x
    sta.w !dp_08,y
    lda.w !actor_0a,y
    clc
    adc.w !obj_vel_y,x
    lda.w !actor_0c,y
    clc
    adc.w !obj_vel_z,x
    sta.w !actor_0c,y
    sep.b #$20
    jmp.w CameraDirectorTail

; @routine 08:8E2A CameraDirectorPathStart
; WHAT IT DOES: The replay camera starts on path $87F1 (ObjectPathStart):
; handler $08:8E51 (CameraDirectorPath), mode 1, the point stepped once;
; then its first frame.
; HOW: Falls into CameraDirectorPath.
; Entry: M 8-bit, X 16-bit; X = object. Exit: as CameraDirectorPath.
%at($088E2A)
CameraDirectorPathStart:
    rep.b #$20
    lda.w #$8E51
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,x
    ldy.w !obj_link_w,x
    rep.b #$20
    lda.w #$0001
    sta.w !bank0_base,y
    sep.b #$20
    ldy.w #$87F1
    jsl.l ObjectPathStart
    jsl.l CameraFollowLink

; @routine 08:8E51 CameraDirectorPath
; WHAT IT DOES: The replay camera along its path (CameraApproachTarget);
; at the path's end it goes to the table ($08:8EC1), else the point's
; step.
; HOW: By C from CameraApproachTarget.
; Entry: M 8-bit, X 16-bit; X = object. Exit: as the path leaves it.
%at($088E51)
CameraDirectorPath:
    jsl.l CameraApproachTarget
    bcs .l8E5A
    brl CameraDirectorTail
.l8E5A:
    jmp.w CameraDirectorTableStart

; @routine 08:8E5D CameraDirectorBehind
; WHAT IT DOES: The replay camera behind the car: mode $21, target offset
; (0, -$8C, -$1F4), the code at $08:8F03, handler $08:8EA2
; (CameraDirectorWait) with count $50; then its first frame.
; HOW: Falls into CameraDirectorWait; $08:8F03 runs as original code.
; Entry: M 8-bit, X 16-bit; X = object. Exit: as CameraDirectorWait.
%at($088E5D)
CameraDirectorBehind:
    ldy.w !obj_link_w,x
    rep.b #$20
    lda.w #$0021
    sta.w !bank0_base,y
    sep.b #$20
    ldy.w !obj_link_w,x
    rep.b #$20
    lda.w #$0000
    sta.w !actor_20,y
    sep.b #$20
    rep.b #$20
    lda.w #$FF74
    sta.w !actor_22,y
    sep.b #$20
    rep.b #$20
    lda.w #$FE0C
    sta.w !actor_palette,y
    sep.b #$20
    jsr.w CameraSnapToCar
    rep.b #$20
    lda.w #$8EA2
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,x
    lda.b #$50
    sta.w !obj_48,x

; @routine 08:8EA2 CameraDirectorWait
; WHAT IT DOES: The replay camera waits out its count +$48 (stepping the
; point), then goes to the table ($08:8EC1).
; HOW: DEC of the count.
; Entry: M 8-bit, X 16-bit; X = object. Exit: as the path leaves it.
%at($088EA2)
CameraDirectorWait:
    lda.w !obj_48,x
    bne .l8EAB
    jml.l CameraDirectorTableStart
.l8EAB:
    dec.w !obj_48,x
    jmp.w CameraDirectorTail

; @routine 08:8EB1 CameraDirectorReset
; WHAT IT DOES: The replay camera in mode $655, the code at $08:8F03,
; then to the table ($08:8EC1).
; HOW: As CameraDirectorBehind. Not reached in the traces so far.
; Entry: M 8-bit, X 16-bit; X = object. Exit: as CameraDirectorTable.
%at($088EB1)
CameraDirectorReset:
    ldy.w !obj_link_w,x             ; not reached in the traces so far
    rep.b #$20
    lda.w #$0655
    sta.w !bank0_base,y
    sep.b #$20
    jsr.w CameraSnapToCar
CameraDirectorTableStart:
    ldy.w !obj_link_w,x
    rep.b #$20
    lda.w #$0000
    sta.w !dp_04,y
    sep.b #$20
    ldy.w !obj_link_w,x
    lda.b #$00
    sta.w !dp_34,y
    lda.w !obj_25,x
    and.b #$FE
    sta.w !obj_25,x
    lda.w !obj_25,x
    and.b #$FD
    sta.w !obj_25,x
    rep.b #$20
    lda.w #$8EF5
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,x

; @routine 08:8EF5 CameraDirectorTable
; WHAT IT DOES: The replay camera from the camera point table
; (CameraPointFromTable), then the point's step ($08:8EF9).
; HOW: Two calls.
; Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; A low byte 3.
%at($088EF5)
CameraDirectorTable:
    jsl.l CameraPointFromTable
CameraDirectorTail:
    jsl.l CameraFollowLink
    lda.b #$03
    sta.w !obj_36,x
    rtl
