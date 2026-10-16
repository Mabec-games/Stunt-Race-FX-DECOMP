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
    lda.l $098000,x
    sta.w !wram_1F4C
    lda.l $098002,x
    sta.w !wram_1F52
    lda.l $098004,x
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
    brl .lBE38                      ; not reached in the traces so far
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
    lda.l !gsu_in_2e,x              ; not reached in the traces so far
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
