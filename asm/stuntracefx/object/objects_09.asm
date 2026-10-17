; objects_09.asm: the course objects' handlers of bank $09: the set-up a
; course's object list points at (a long pointer per object kind in the
; course data) and the per-frame handlers they install at +$1A/+$1C (run by
; ObjectRunHandlerA). Most start with a NOP, as the course data's pointers
; expect.
; C: src/stuntracefx/object/objects_09.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816


; @routine 09:8B3B ObjectLampTower
; WHAT IT DOES: Sets up a lamp on a tower: object X 116 units lower
; ($FF74 on +$0E), its light ($5D = $9971, +$24 bit 3, +$3A = 0, +$5F =
; $3C), shape $A104, +$09 bit 0 cleared; and, when one can be made, a
; second light (kind $A120, $5D = $9973, +$5F = $64) at its position $578
; higher, rebucketed; then no handler.
; HOW: ObjectSpawn, ObjectCopyPositionTo, ObjectRebucket with X and Y
; swapped round it. Without a free object the second light is not made.
; Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; A low byte = 0.
%at($098B3B)
ObjectLampTower:
    nop
    rep.b #$20
    lda.w !obj_pos_y,x
    clc
    adc.w #$FF74
    sta.w !obj_pos_y,x
    sep.b #$20
    rep.b #$20
    lda.w #$9971
    sta.w !obj_5d,x
    sep.b #$20
    lda.w !obj_24,x
    ora.b #$08
    sta.w !obj_24,x
    lda.b #$00
    sta.w !obj_3a,x
    lda.b #$3C
    sta.w !obj_5f,x
    rep.b #$20
    lda.w #$A104
    sta.w !obj_04,x
    sep.b #$20
    lda.w !obj_09,x
    and.b #$FE
    sta.w !obj_09,x
    rep.b #$20
    lda.w #$A120
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .l8B8C
    jml.l .l8BC4
.l8B8C:
    lda.w !obj_24,y
    ora.b #$08
    sta.w !obj_24,y
    lda.b #$00
    sta.w !obj_3a,y
    lda.b #$64
    sta.w !obj_5f,y
    rep.b #$20
    lda.w #$9973
    sta.w !obj_5d,y
    sep.b #$20
    jsl.l ObjectCopyPositionTo
    rep.b #$20
    lda.w !obj_pos_y,y
    clc
    adc.w #$FA88
    sta.w !obj_pos_y,y
    sep.b #$20
    phx
    tyx
    ply
    jsl.l ObjectRebucket
    phx
    tyx
    ply
.l8BC4:
    rep.b #$20
    lda.w #$0000
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$00
    sta.w !obj_1c,x
    rtl

; @routine 09:8BD4 ObjectLampLow
; WHAT IT DOES: Sets up a lamp: its light ($5D = $9973, +$24 bit 3, +$3A =
; 0, +$5F = $50), object X 380 units higher ($FE84 on +$0E); then no
; handler.
; HOW: Byte and word stores.
; Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; A low byte = 0.
%at($098BD4)
ObjectLampLow:
    nop
    lda.w !obj_24,x
    ora.b #$08
    sta.w !obj_24,x
    lda.b #$00
    sta.w !obj_3a,x
    lda.b #$50
    sta.w !obj_5f,x
    rep.b #$20
    lda.w #$9973
    sta.w !obj_5d,x
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_y,x
    clc
    adc.w #$FE84
    sta.w !obj_pos_y,x
    sep.b #$20
    rep.b #$20
    lda.w #$0000
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$00
    sta.w !obj_1c,x
    rtl

; @routine 09:8C0F ObjectScale40
; WHAT IT DOES: Sets object X's scale +$15 to $40.
; HOW: One store.
; Entry: M 8-bit; X = object. Exit: A low byte = $40.
%at($098C0F)
ObjectScale40:
    nop
    lda.b #$40
    sta.w !obj_15,x
    rtl

; @routine 09:8C16 ObjectScoreGate
; WHAT IT DOES: Sets up a scoring gate: its restart handler (+$1D/+$1F =
; $09:8CAF, ObjectScoreGateRecord) and frame handler ($09:8CD6,
; ObjectScoreGateStep); a counter object (kind $A040) at its position
; $6E0 left, $A0 lower and $F00 further on, linked back (+$27 = X), with
; handler $09:8CF8 (ObjectScoreCounterStep), +$49 = 2, scale $20, +$5A =
; $80, count +$48 = 0, +$0A = $50; then the gate's record and frame.
; HOW: ObjectSpawn and ObjectCopyPositionTo; without a free object only
; the gate is set up.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as
; ObjectScoreGateStep.
%at($098C16)
ObjectScoreGate:
    rep.b #$20
    lda.w #$8CAF
    sta.w !obj_1d,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1f,x
    rep.b #$20
    lda.w #$8CD6
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,x
    rep.b #$20
    lda.w #$A040
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .l8C48
    jml.l ObjectScoreGateRecord
.l8C48:
    jsl.l ObjectCopyPositionTo
    pha
    rep.b #$20
    txa
    sta.w !obj_27,y
    sep.b #$20
    pla
    rep.b #$20
    lda.w #$8CF8
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,y
    lda.b #$02
    sta.w !obj_49,y
    rep.b #$20
    lda.w !obj_pos_x,y
    clc
    adc.w #$F920
    sta.w !obj_pos_x,y
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_y,y
    clc
    adc.w #$00A0
    sta.w !obj_pos_y,y
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_z,y
    clc
    adc.w #$0F00
    sta.w !obj_pos_z,y
    sep.b #$20
    rep.b #$20
    lda.w #$0020
    sta.w !obj_15,y
    sep.b #$20
    lda.b #$00
    ora.b #$80
    sta.w !obj_5a,y
    lda.b #$00
    sta.w !obj_48,y
    lda.b #$50
    sta.w !obj_09+1,y

; @routine 09:8CAF ObjectScoreGateRecord
; WHAT IT DOES: The gate's restart: a new Super FX record ($42D) with
; flag bit 4 at (+$F00, +$F00, +$F00), then ObjectScoreGateStep.
; HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as
; ObjectScoreGateStep.
%at($098CAF)
ObjectScoreGateRecord:
    jsl.l GsuRecordNew42D
    jsl.l GsuRecordFlag10
    rep.b #$20
    lda.w #$0F00
    sta.b !dp_02
    sep.b #$20
    rep.b #$20
    lda.w #$0F00
    sta.b !dp_08
    sep.b #$20
    rep.b #$20
    lda.w #$0F00
    sta.b !dp_94
    sep.b #$20
    jsl.l GsuRecordPosition

; @routine 09:8CD6 ObjectScoreGateStep
; WHAT IT DOES: The gate's frame: synced from its record; its +$25 bit 0
; set while the record's sync and contact summary both have bit 0 set,
; else cleared.
; HOW: ObjectSyncFromRecord, RecordContactSummary.
; Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; A low byte the new
; +$25.
%at($098CD6)
ObjectScoreGateStep:
    jsl.l ObjectSyncFromRecord
    and.b #$01
    beq .l8CEF
    jsl.l RecordContactSummary
    and.b #$01
    beq .l8CEF
    lda.w !obj_25,x
    ora.b #$01
    sta.w !obj_25,x
    rtl
.l8CEF:
    lda.w !obj_25,x
    and.b #$FE
    sta.w !obj_25,x
    rtl

; @routine 09:8CF8 ObjectScoreCounterStep
; WHAT IT DOES: The score counter's frame, by the lap count $0DEA: on the
; first lap the first time its +$25 bit 1 and shape $B754, then its tick;
; on the second it slides in (counter_slide); then rebucketed.
; HOW: The tick (counter_tick) every third frame follows its gate's +$25
; bit 0. Reached only by unit_objects_09.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as
; ObjectRebucket.
%at($098CF8)
ObjectScoreCounterStep:
    nop
    sep.b #$20
    lda.w !wram_0DEA
    beq .l8D2F
    cmp.b #$01
    beq .l8D0A
    cmp.b #$02
    beq .l8D2C
    bra .l8D2F
.l8D0A:
    lda.w !obj_25,x
    and.b #$02
    beq .l8D15
    jml.l .l8D27
.l8D15:
    lda.w !obj_25,x
    ora.b #$02
    sta.w !obj_25,x
    rep.b #$20
    lda.w #$B754
    sta.w !obj_04,x
    sep.b #$20
.l8D27:
    jsr.w ObjectScoreCounterTick
    bra .l8D2F
.l8D2C:
    jsr.w ObjectScoreCounterSlide
.l8D2F:
    jsl.l ObjectRebucket
    rtl

; @routine 09:8D34 ObjectScoreCounterTick
; WHAT IT DOES: Every third frame (+$49 counting down from 2) follows its
; gate (+$27): while the gate's +$25 bit 0 is set, sets its own (sound $32
; the first time) and counts +$48 up to 9; else clears it and counts +$48
; down to 0; shows the count at +$5A with bit 7 ($1F58 = the count).
; HOW: Byte compares and JMLs inside the bank; ends RTS.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: M 8-bit; A low byte
; = the shown count with bit 7, or the countdown's value when it only
; counted down; N, Z from that.
%at($098D34)
ObjectScoreCounterTick:
    lda.w !obj_49,x
    cmp.b #$00
    bne .l8D3F
    jml.l .l8D43
.l8D3F:
    dec.w !obj_49,x
    rts
.l8D43:
    lda.b #$02
    sta.w !obj_49,x
    ldy.w !obj_27,x
    lda.w !obj_25,y
    and.b #$01
    bne .l8D56
    jml.l .l8D8C
.l8D56:
    lda.w !obj_25,x
    and.b #$01
    beq .l8D61
    jml.l .l8D6F
.l8D61:
    lda.w !obj_25,x
    ora.b #$01
    sta.w !obj_25,x
    lda.b #$32
    jsl.l SoundPort0QueuePush
.l8D6F:
    lda.w !obj_48,x
    cmp.b #$09
    bne .l8D7A
    jml.l .l8D7D
.l8D7A:
    inc.w !obj_48,x
.l8D7D:
    lda.w !obj_48,x
    sta.w !wram_1F58
    lda.w !wram_1F58
    ora.b #$80
    sta.w !obj_5a,x
    rts
.l8D8C:
    lda.w !obj_25,x
    and.b #$FE
    sta.w !obj_25,x
    lda.w !obj_48,x
    cmp.b #$00
    bne .l8D9F
    jml.l .l8DA2
.l8D9F:
    dec.w !obj_48,x
.l8DA2:
    lda.w !obj_48,x
    sta.w !wram_1F58
    lda.w !wram_1F58
    ora.b #$80
    sta.w !obj_5a,x
    rts

; @routine 09:8DB1 ObjectScoreCounterSlide
; WHAT IT DOES: While +$0A runs down, eases the scale +$15 toward $40
; (ApproachQuarterMin4Byte via $3E) and moves the counter $5A right; then
; ObjectScoreCounterTick.
; HOW: 16-bit add for X; ends RTS.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as
; ObjectScoreCounterTick.
%at($098DB1)
ObjectScoreCounterSlide:
    lda.w !obj_09+1,x
    cmp.b #$00
    bne .l8DBC
    jml.l .l8DDB
.l8DBC:
    dec.w !obj_09+1,x
    lda.w !obj_15,x
    sta.b !scale_scratch
    lda.b #$40
    jsl.l ApproachQuarterMin4Byte
    sta.w !obj_15,x
    rep.b #$20
    lda.w !obj_pos_x,x
    clc
    adc.w #$005A
    sta.w !obj_pos_x,x
    sep.b #$20
.l8DDB:
    jsr.w ObjectScoreCounterTick
    rts

; @routine 09:8DDF ObjectFlyingPiece
; WHAT IT DOES: Sets up a flying piece: handler $09:8DF8
; (ObjectFlyingPieceStep), a Super FX record from 08:CBCC (released at
; once when there is none), its velocity (GsuRecordVelocity), then its
; frame.
; HOW: GsuRecordNew424Piece (08:CBCC); its N flag tells the failure.
; Entry: M 8-bit, X 16-bit; X = object. Exit: as the path leaves it.
%at($098DDF)
ObjectFlyingPiece:
    rep.b #$20
    lda.w #$8DF8
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,x
    jsl.l GsuRecordNew424Piece
    bmi ObjectFlyingPieceRelease
    jsl.l GsuRecordVelocity

; @routine 09:8DF8 ObjectFlyingPieceStep
; WHAT IT DOES: A flying piece's frame: synced from its record
; (ObjectSyncFromRecord); released once +$0A runs down.
; HOW: DEC of the byte.
; Entry: M 8-bit, X 16-bit; X = object. Exit: as the path leaves it.
%at($098DF8)
ObjectFlyingPieceStep:
    jsl.l ObjectSyncFromRecord
    dec.w !obj_09+1,x
    beq .l8E05
    jml.l .l8E09
.l8E05:
    jsl.l ObjectRelease
.l8E09:
    rtl

; @routine 09:8E0A ObjectFlyingPieceRelease
; WHAT IT DOES: Releases object X (ObjectRelease): where a flying piece
; goes when it gets no Super FX record.
; HOW: JSL ObjectRelease, RTL.
; Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectRelease.
%at($098E0A)
ObjectFlyingPieceRelease:
    jsl.l ObjectRelease
    rtl

; @routine 09:8E0F ObjectFireworkShell
; WHAT IT DOES: Sets up a firework shell: frame handler $09:8E5A
; (ObjectFireworkShellStep), restart $09:8E2D (ObjectFireworkShellRecord),
; then its record and frame.
; HOW: Word and byte stores; the record from GsuRecordNew424Piece (08:CBCC).
; Reached only by unit_objects_09.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
; leaves it.
%at($098E0F)
ObjectFireworkShell:
    rep.b #$20
    lda.w #$8E5A
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,x
    rep.b #$20
    lda.w #$8E2D
    sta.w !obj_1d,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1f,x

; @routine 09:8E2D ObjectFireworkShellRecord
; WHAT IT DOES: The shell's restart: a Super FX record (08:CBCC) with its
; velocity, +$2A's high byte 8, at ($190, $190, $190); then its frame.
; HOW: GsuRecordVelocity, GsuRecordSet2AHigh, GsuRecordPosition. Reached
; only by unit_objects_09.
; Entry: as ObjectFireworkShell. Exit: as ObjectFireworkShell.
%at($098E2D)
ObjectFireworkShellRecord:
    jsl.l GsuRecordNew424Piece
    jsl.l GsuRecordVelocity
    lda.b #$08
    jsl.l GsuRecordSet2AHigh
    rep.b #$20
    lda.w #$0190
    sta.b !dp_02
    sep.b #$20
    rep.b #$20
    lda.w #$0190
    sta.b !dp_08
    sep.b #$20
    rep.b #$20
    lda.w #$0190
    sta.b !dp_94
    sep.b #$20
    jsl.l GsuRecordPosition

; @routine 09:8E5A ObjectFireworkShellStep
; WHAT IT DOES: The shell's frame: released when +$0A runs down; its size
; bytes +$14-$16 grow; on a hit (its record's sync and contact bit 0) it is
; released with sound $56 and bursts into four sparks (kind $A0CC, lit by
; $996B, handler ObjectFlyingPiece, +$0A = $0F, flying out at four
; velocities).
; HOW: ObjectSyncFromRecord, RecordContactSummary; the sparks by
; ObjectSpawn and ObjectCopyPositionTo. Reached only by unit_objects_09.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path
; leaves it.
%at($098E5A)
ObjectFireworkShellStep:
    dec.w !obj_09+1,x
    beq .l8E63
    jml.l .l8E67
.l8E63:
    jsl.l ObjectRelease
.l8E67:
    rep.b #$20
    lda.w !obj_14,x
    clc
    adc.w #$000C
    sta.w !obj_14,x
    sep.b #$20
    rep.b #$20
    lda.w !obj_15,x
    clc
    adc.w #$000C
    sta.w !obj_15,x
    sep.b #$20
    rep.b #$20
    lda.w !obj_16,x
    clc
    adc.w #$000C
    sta.w !obj_16,x
    sep.b #$20
    jsl.l ObjectSyncFromRecord
    and.b #$01
    beq .l8EAE
    jsl.l RecordContactSummary
    and.b #$01
    beq .l8EAE
    jsl.l ObjectRelease
    lda.b #$56
    jsl.l SoundPort0QueuePush
    jsr.w ObjectFireworkBurst
.l8EAE:
    rtl

; @routine 09:8EAF ObjectFireworkBurst
; WHAT IT DOES: Bursts a firework into four sparks (kind $A0CC, lit by
; $996B, handler ObjectFlyingPiece, +$0A = $0F) at object X, flying out at
; ($28, -$55, 0), (-$28, -$55, 0), (0, -$5A, $28), (0, -$5A, -$28); a spark
; without a free object is skipped.
; HOW: ObjectSpawn, ObjectCopyPositionTo for each; ends RTS.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: M 8-bit; A, Y as
; the last spark leaves them.
%at($098EAF)
ObjectFireworkBurst:
    rep.b #$20
    lda.w #$A0CC
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .l8EC3
    jml.l .l8F15
.l8EC3:
    rep.b #$20
    lda.w #$996B
    sta.w !obj_5d,y
    sep.b #$20
    lda.w !obj_24,y
    ora.b #$08
    sta.w !obj_24,y
    lda.b #$00
    sta.w !obj_3a,y
    lda.b #$14
    sta.w !obj_5f,y
    jsl.l ObjectCopyPositionTo
    rep.b #$20
    lda.w #$8DDF
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,y
    rep.b #$20
    lda.w #$0028
    sta.w !obj_vel_x,y
    sep.b #$20
    rep.b #$20
    lda.w #$FFAB
    sta.w !obj_vel_y,y
    sep.b #$20
    rep.b #$20
    lda.w #$0000
    sta.w !obj_vel_z,y
    sep.b #$20
    lda.b #$0F
    sta.w !obj_09+1,y
.l8F15:
    rep.b #$20
    lda.w #$A0CC
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .l8F29
    jml.l .l8F7B
.l8F29:
    rep.b #$20
    lda.w #$996B
    sta.w !obj_5d,y
    sep.b #$20
    lda.w !obj_24,y
    ora.b #$08
    sta.w !obj_24,y
    lda.b #$00
    sta.w !obj_3a,y
    lda.b #$03
    sta.w !obj_5f,y
    jsl.l ObjectCopyPositionTo
    rep.b #$20
    lda.w #$8DDF
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,y
    rep.b #$20
    lda.w #$FFD8
    sta.w !obj_vel_x,y
    sep.b #$20
    rep.b #$20
    lda.w #$FFAB
    sta.w !obj_vel_y,y
    sep.b #$20
    rep.b #$20
    lda.w #$0000
    sta.w !obj_vel_z,y
    sep.b #$20
    lda.b #$0F
    sta.w !obj_09+1,y
.l8F7B:
    rep.b #$20
    lda.w #$A0CC
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .l8F8F
    jml.l .l8FE1
.l8F8F:
    rep.b #$20
    lda.w #$996B
    sta.w !obj_5d,y
    sep.b #$20
    lda.w !obj_24,y
    ora.b #$08
    sta.w !obj_24,y
    lda.b #$00
    sta.w !obj_3a,y
    lda.b #$0A
    sta.w !obj_5f,y
    jsl.l ObjectCopyPositionTo
    rep.b #$20
    lda.w #$8DDF
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,y
    rep.b #$20
    lda.w #$0000
    sta.w !obj_vel_x,y
    sep.b #$20
    rep.b #$20
    lda.w #$FFA6
    sta.w !obj_vel_y,y
    sep.b #$20
    rep.b #$20
    lda.w #$0028
    sta.w !obj_vel_z,y
    sep.b #$20
    lda.b #$0F
    sta.w !obj_09+1,y
.l8FE1:
    rep.b #$20
    lda.w #$A0CC
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .l8FF5
    jml.l .l9047
.l8FF5:
    rep.b #$20
    lda.w #$996B
    sta.w !obj_5d,y
    sep.b #$20
    lda.w !obj_24,y
    ora.b #$08
    sta.w !obj_24,y
    lda.b #$00
    sta.w !obj_3a,y
    lda.b #$01
    sta.w !obj_5f,y
    jsl.l ObjectCopyPositionTo
    rep.b #$20
    lda.w #$8DDF
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,y
    rep.b #$20
    lda.w #$0000
    sta.w !obj_vel_x,y
    sep.b #$20
    rep.b #$20
    lda.w #$FFA6
    sta.w !obj_vel_y,y
    sep.b #$20
    rep.b #$20
    lda.w #$FFD8
    sta.w !obj_vel_z,y
    sep.b #$20
    lda.b #$0F
    sta.w !obj_09+1,y
.l9047:
    rts

; @routine 09:9048 ObjectFinishGate
; WHAT IT DOES: Sets up the finish gate's fireworks: restart $09:9066
; (ObjectFinishGateRecord) and frame handler $09:908D
; (ObjectFinishGateStep), then its record and frame.
; HOW: Word and byte stores. Reached only by unit_objects_09.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
; leaves it.
%at($099048)
ObjectFinishGate:
    rep.b #$20
    lda.w #$9066
    sta.w !obj_1d,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1f,x
    rep.b #$20
    lda.w #$908D
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,x

; @routine 09:9066 ObjectFinishGateRecord
; WHAT IT DOES: The finish gate's restart: a Super FX record ($42D) with
; flag bit 4 at ($1680, $1680, $1680), then its frame.
; HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition. Reached only
; by unit_objects_09.
; Entry: as ObjectFinishGate. Exit: as ObjectFinishGate.
%at($099066)
ObjectFinishGateRecord:
    jsl.l GsuRecordNew42D
    jsl.l GsuRecordFlag10
    rep.b #$20
    lda.w #$1680
    sta.b !dp_02
    sep.b #$20
    rep.b #$20
    lda.w #$1680
    sta.b !dp_08
    sep.b #$20
    rep.b #$20
    lda.w #$1680
    sta.b !dp_94
    sep.b #$20
    jsl.l GsuRecordPosition

; @routine 09:908D ObjectFinishGateStep
; WHAT IT DOES: The finish gate's frame: synced from its record; when it is
; hit (sync and contact bit 0) on the second lap for the first time (+$25
; bit 0), sound $11 and two firework shells (kind $B6E4, handler
; ObjectFireworkShell) launched beside it.
; HOW: ObjectSyncFromRecord, RecordContactSummary, ObjectSpawn. Reached
; only by unit_objects_09.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
; it.
%at($09908D)
ObjectFinishGateStep:
    jsl.l ObjectSyncFromRecord
    and.b #$01
    beq .l90C2
    jsl.l RecordContactSummary
    and.b #$01
    beq .l90C2
    sep.b #$20
    lda.w !wram_0DEA
    cmp.b #$02
    bne .l90C2
    lda.w !obj_25,x
    and.b #$01
    beq .l90B1
    jml.l .l90C2
.l90B1:
    lda.w !obj_25,x
    ora.b #$01
    sta.w !obj_25,x
    lda.b #$11
    jsl.l SoundPort0QueuePush
    jsr.w ObjectFireworkShellPair
.l90C2:
    rtl

; @routine 09:90C3 ObjectFireworkShellPair
; WHAT IT DOES: Fires the finish gate's two firework shells (kind $B6E4,
; handler ObjectFireworkShell), $3C0 to each side, $500 up, $1900 back,
; rising at $15E, with their sizes and +$0A = $1E.
; HOW: ObjectSpawn, ObjectCopyPositionTo for each; ends RTS.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: M 8-bit; A, Y as
; the second shell leaves them.
%at($0990C3)
ObjectFireworkShellPair:
    rep.b #$20
    lda.w #$B6E4
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .l90D7
    jml.l .l9146
.l90D7:
    jsl.l ObjectCopyPositionTo
    rep.b #$20
    lda.w #$8E0F
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,y
    rep.b #$20
    lda.w !obj_pos_y,y
    clc
    adc.w #$FB00
    sta.w !obj_pos_y,y
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_z,y
    clc
    adc.w #$E700
    sta.w !obj_pos_z,y
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_x,y
    clc
    adc.w #$03C0
    sta.w !obj_pos_x,y
    sep.b #$20
    rep.b #$20
    lda.w #$0000
    sta.w !obj_vel_x,y
    sep.b #$20
    rep.b #$20
    lda.w #$0000
    sta.w !obj_vel_y,y
    sep.b #$20
    rep.b #$20
    lda.w #$015E
    sta.w !obj_vel_z,y
    sep.b #$20
    lda.b #$3C
    sta.w !obj_14,y
    lda.b #$32
    sta.w !obj_15,y
    lda.b #$23
    sta.w !obj_16,y
    lda.b #$1E
    sta.w !obj_09+1,y
.l9146:
    rep.b #$20
    lda.w #$B6E4
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .l915A
    jml.l .l91C9
.l915A:
    jsl.l ObjectCopyPositionTo
    rep.b #$20
    lda.w #$8E0F
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,y
    rep.b #$20
    lda.w !obj_pos_y,y
    clc
    adc.w #$FB00
    sta.w !obj_pos_y,y
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_z,y
    clc
    adc.w #$E700
    sta.w !obj_pos_z,y
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_x,y
    clc
    adc.w #$FC40
    sta.w !obj_pos_x,y
    sep.b #$20
    rep.b #$20
    lda.w #$0000
    sta.w !obj_vel_x,y
    sep.b #$20
    rep.b #$20
    lda.w #$0000
    sta.w !obj_vel_y,y
    sep.b #$20
    rep.b #$20
    lda.w #$015E
    sta.w !obj_vel_z,y
    sep.b #$20
    lda.b #$78
    sta.w !obj_14,y
    lda.b #$3C
    sta.w !obj_15,y
    lda.b #$0C
    sta.w !obj_16,y
    lda.b #$1E
    sta.w !obj_09+1,y
.l91C9:
    rts

; @routine 09:91CA ObjectBumper
; WHAT IT DOES: Sets up a bumper: restart $09:91E8 (ObjectBumperRecord),
; frame $09:91F6 (ObjectBumperStep), then its record and frame.
; HOW: Word and byte stores.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
; leaves it.
%at($0991CA)
ObjectBumper:
    rep.b #$20
    lda.w #$91E8
    sta.w !obj_1d,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1f,x
    rep.b #$20
    lda.w #$91F6
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,x

; @routine 09:91E8 ObjectBumperRecord
; WHAT IT DOES: The bumper's restart: a Super FX record ($424) with flag
; bit 4 and +$2A's high byte 1, then its frame.
; HOW: GsuRecordNew424, GsuRecordFlag10, GsuRecordSet2AHigh.
; Entry: as ObjectBumper. Exit: as ObjectBumper.
%at($0991E8)
ObjectBumperRecord:
    jsl.l GsuRecordNew424
    jsl.l GsuRecordFlag10
    lda.b #$01
    jsl.l GsuRecordSet2AHigh

; @routine 09:91F6 ObjectBumperStep
; WHAT IT DOES: The bumper's frame: while +$48 counts down it bulges
; (+$15 by 6, +$14 and +$16 by $0A a frame); then it settles back (+$14
; and +$16 eased to 0 by ApproachHalfMin2Byte, +$15 growing by $0C); a car
; hitting it sets +$25 bit 0 the first time with sound $54 and a new
; bulge (+$48 = 6); no hit clears the bit.
; HOW: ObjectSyncFromRecord, RecordContactSummary.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: M 8-bit.
%at($0991F6)
ObjectBumperStep:
    lda.w !obj_48,x
    bne .l91FF
    jml.l .l9226
.l91FF:
    lda.w !obj_15,x
    clc
    adc.b #$06
    sta.w !obj_15,x
    lda.w !obj_14,x
    clc
    adc.b #$0A
    sta.w !obj_14,x
    lda.w !obj_16,x
    clc
    adc.b #$0A
    sta.w !obj_16,x
    lda.w !obj_48,x
    clc
    adc.b #$FF
    sta.w !obj_48,x
    jmp.w .l9251
.l9226:
    lda.w !obj_14,x
    sta.b !scale_scratch
    lda.b #$00
    jsl.l ApproachHalfMin2Byte
    sta.w !obj_14,x
    bcc .l923A
    jml.l .l9251
.l923A:
    lda.w !obj_16,x
    sta.b !scale_scratch
    lda.b #$00
    jsl.l ApproachHalfMin2Byte
    sta.w !obj_16,x
    lda.w !obj_15,x
    clc
    adc.b #$0C
    sta.w !obj_15,x
.l9251:
    jsl.l ObjectSyncFromRecord
    and.b #$01
    beq .l9280
    jsl.l RecordContactSummary
    and.b #$01
    beq .l9280
    lda.w !obj_25,x
    and.b #$01
    beq .l926C
    jml.l .l927F
.l926C:
    lda.w !obj_25,x
    ora.b #$01
    sta.w !obj_25,x
    lda.b #$54
    jsl.l SoundPort0QueuePush
    lda.b #$06
    sta.w !obj_48,x
.l927F:
    rtl
.l9280:
    lda.w !obj_25,x
    and.b #$FE
    sta.w !obj_25,x
    rtl

; @routine 09:9289 ObjectSwitch
; WHAT IT DOES: Sets up a switch: restart $09:92AE (ObjectSwitchRecord),
; frame $09:92BC (ObjectSwitchStep), +$5A = $80; then its record and frame.
; HOW: Word and byte stores.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
; leaves it.
%at($099289)
ObjectSwitch:
    rep.b #$20
    lda.w #$92AE
    sta.w !obj_1d,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1f,x
    rep.b #$20
    lda.w #$92BC
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,x
    lda.b #$00
    ora.b #$80
    sta.w !obj_5a,x

; @routine 09:92AE ObjectSwitchRecord
; WHAT IT DOES: The switch's restart: a Super FX record ($424) with flag
; bit 4 and +$2A's high byte 1, then its frame.
; HOW: GsuRecordNew424, GsuRecordFlag10, GsuRecordSet2AHigh.
; Entry: as ObjectSwitch. Exit: as ObjectSwitch.
%at($0992AE)
ObjectSwitchRecord:
    jsl.l GsuRecordNew424
    jsl.l GsuRecordFlag10
    lda.b #$01
    jsl.l GsuRecordSet2AHigh

; @routine 09:92BC ObjectSwitchStep
; WHAT IT DOES: The switch's frame: once on (+$25 bit 0), its count +$48
; eases toward 3 (03:D07C); the count is shown at +$5A with bit 7; while
; off, a car hitting it turns it on (+$15 by $0C, sound $55). The bytes at
; 09:931C (clearing +$25 bit 0) are reached by no code.
; HOW: ObjectSyncFromRecord, RecordContactSummary; 03:D07C as original
; code.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: M 8-bit.
%at($0992BC)
ObjectSwitchStep:
    lda.w !obj_25,x
    and.b #$01
    bne .l92C7
    jml.l .l92DB
.l92C7:
    lda.w !obj_48,x
    sta.b !scale_scratch
    lda.b #$03
    jsl.l ApproachSixteenthMin16Byte
    sta.w !obj_48,x
    bcc .l92DB
    jml.l .l92DB
.l92DB:
    lda.w !obj_48,x
    sta.w !wram_1F58
    lda.w !wram_1F58
    ora.b #$80
    sta.w !obj_5a,x
    lda.w !obj_25,x
    and.b #$01
    beq .l92F4
    jml.l .l931B
.l92F4:
    jsl.l ObjectSyncFromRecord
    and.b #$01
    beq .l931B
    jsl.l RecordContactSummary
    and.b #$01
    beq .l931B
    lda.w !obj_25,x
    ora.b #$01
    sta.w !obj_25,x
    lda.w !obj_15,x
    clc
    adc.b #$0C
    sta.w !obj_15,x
    lda.b #$55
    jsl.l SoundPort0QueuePush
.l931B:
    rtl

; @routine 09:931C ObjectClear25Bit0
; WHAT IT DOES: Clears bit 0 of object X's +$25 (the switch's "on").
; HOW: LDA / AND #$FE / STA, 8-bit; RTL.
; Entry: M 8-bit, X 16-bit; X = object. Exit: A low byte = the new +$25;
; N, Z from it.
%at($09931C)
ObjectClear25Bit0:
    lda.w !obj_25,x
    and.b #$FE
    sta.w !obj_25,x
    rtl

; @routine 09:9325 ObjectPlatform
; WHAT IT DOES: Sets up a platform: +$20 = $EC60, restart $09:934D
; (ObjectPlatformRecord), frame $09:937A (ObjectPlatformStep); then its
; record (mode 2, +$2A high $50, at ($500, $1180, $500)) and sync.
; HOW: Word and byte stores. Reached only by unit_objects_09.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as
; ObjectSyncFromRecord.
%at($099325)
ObjectPlatform:
    rep.b #$20
    lda.w #$EC60
    sta.w !obj_20,x
    sep.b #$20
    rep.b #$20
    lda.w #$934D
    sta.w !obj_1d,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1f,x
    rep.b #$20
    lda.w #$937A
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,x

; @routine 09:934D ObjectPlatformRecord
; WHAT IT DOES: The platform's restart: its Super FX record (GsuRecordNew424Alt,
; GsuRecordMode2, +$2A high $50, at ($500, $1180, $500)), then synced.
; HOW: As ObjectPlatform. Reached only by unit_objects_09.
; Entry: as ObjectPlatform. Exit: as ObjectPlatform.
%at($09934D)
ObjectPlatformRecord:
    jsl.l GsuRecordNew424Alt
    jsl.l GsuRecordMode2
    lda.b #$50
    jsl.l GsuRecordSet2AHigh
    rep.b #$20
    lda.w #$0500
    sta.b !dp_02
    sep.b #$20
    rep.b #$20
    lda.w #$1180
    sta.b !dp_08
    sep.b #$20
    rep.b #$20
    lda.w #$0500
    sta.b !dp_94
    sep.b #$20
    jsl.l GsuRecordPosition

; @routine 09:937A ObjectPlatformStep
; WHAT IT DOES: The platform's frame: synced from its record.
; HOW: ObjectSyncFromRecord. Reached only by unit_objects_09.
; Entry: as ObjectSyncFromRecord. Exit: as ObjectSyncFromRecord.
%at($09937A)
ObjectPlatformStep:
    jsl.l ObjectSyncFromRecord
    rtl

; @routine 09:937F ObjectPostA
; WHAT IT DOES: Sets up the first of a gate's two posts: restart $09:939D
; (ObjectPostARecord), frame $09:93C4 (ObjectPostAStep); its record ($42D,
; flag bit 4, at ($1E0, $1E0, $1E0)) and frame.
; HOW: Word and byte stores.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
; leaves it.
%at($09937F)
ObjectPostA:
    rep.b #$20
    lda.w #$939D
    sta.w !obj_1d,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1f,x
    rep.b #$20
    lda.w #$93C4
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,x

; @routine 09:939D ObjectPostARecord
; WHAT IT DOES: The first post's restart: its record ($42D, flag bit 4, at
; ($1E0, $1E0, $1E0)), then its frame.
; HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition.
; Entry: as ObjectPostA. Exit: as ObjectPostA.
%at($09939D)
ObjectPostARecord:
    jsl.l GsuRecordNew42D
    jsl.l GsuRecordFlag10
    rep.b #$20
    lda.w #$01E0
    sta.b !dp_02
    sep.b #$20
    rep.b #$20
    lda.w #$01E0
    sta.b !dp_08
    sep.b #$20
    rep.b #$20
    lda.w #$01E0
    sta.b !dp_94
    sep.b #$20
    jsl.l GsuRecordPosition

; @routine 09:93C4 ObjectPostAStep
; WHAT IT DOES: The first post's frame: when a car hits it, +$14 eases
; toward $33 (ApproachHalfMin2Byte) and bit 0 of $1A07 is set.
; HOW: ObjectSyncFromRecord, RecordContactSummary.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: M 8-bit.
%at($0993C4)
ObjectPostAStep:
    jsl.l ObjectSyncFromRecord
    and.b #$01
    beq .l93EC
    jsl.l RecordContactSummary
    and.b #$01
    beq .l93EC
    lda.w !obj_14,x
    sta.b !scale_scratch
    lda.b #$33
    jsl.l ApproachHalfMin2Byte
    sta.w !obj_14,x
    sep.b #$20
    lda.w !wram_1A07
    ora.b #$01
    sta.w !wram_1A07
.l93EC:
    rtl

; @routine 09:93ED ObjectPostB
; WHAT IT DOES: Sets up the second post: restart $09:940B
; (ObjectPostBRecord), frame $09:9432 (ObjectPostBStep); its record ($42D,
; flag bit 4, at ($F00, $F00, $F00)) and frame.
; HOW: Word and byte stores.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
; leaves it.
%at($0993ED)
ObjectPostB:
    rep.b #$20
    lda.w #$940B
    sta.w !obj_1d,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1f,x
    rep.b #$20
    lda.w #$9432
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,x

; @routine 09:940B ObjectPostBRecord
; WHAT IT DOES: The second post's restart: its record ($42D, flag bit 4,
; at ($F00, $F00, $F00)), then its frame.
; HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition.
; Entry: as ObjectPostB. Exit: as ObjectPostB.
%at($09940B)
ObjectPostBRecord:
    jsl.l GsuRecordNew42D
    jsl.l GsuRecordFlag10
    rep.b #$20
    lda.w #$0F00
    sta.b !dp_02
    sep.b #$20
    rep.b #$20
    lda.w #$0F00
    sta.b !dp_08
    sep.b #$20
    rep.b #$20
    lda.w #$0F00
    sta.b !dp_94
    sep.b #$20
    jsl.l GsuRecordPosition

; @routine 09:9432 ObjectPostBStep
; WHAT IT DOES: The second post's frame: when a car hits it, its +$25 bit
; 0 and bit 1 of $1A07 the first time; once both posts are hit ($1A07 =
; 3), the first time (its +$25 bit 1), a sky ball (kind $AA34, handler
; ObjectSkyBallStep) whose target +$27 is this post's +$10 less $2300.
; HOW: ObjectSyncFromRecord, RecordContactSummary, ObjectSpawn. The sky
; ball's spawn is reached only by unit_objects_09.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path
; leaves it.
%at($099432)
ObjectPostBStep:
    jsl.l ObjectSyncFromRecord
    and.b #$01
    beq .l9442
    jsl.l RecordContactSummary
    and.b #$01
    bne .l9443
.l9442:
    rtl
.l9443:
    lda.w !obj_25,x
    and.b #$01
    beq .l944E
    jml.l .l9460
.l944E:
    lda.w !obj_25,x
    ora.b #$01
    sta.w !obj_25,x
    sep.b #$20
    lda.w !wram_1A07
    ora.b #$02
    sta.w !wram_1A07
.l9460:
    lda.w !wram_1A07
    cmp.b #$03
    bne .l9495
    lda.w !obj_25,x
    and.b #$02
    beq .l9472
    jml.l .l9495
.l9472:
    lda.w !obj_25,x
    ora.b #$02
    sta.w !obj_25,x
    jsr.w ObjectSkyBallLaunch
    rep.b #$20
    lda.w !obj_pos_z,x
    sta.w !obj_27,y
    sep.b #$20
    rep.b #$20
    lda.w !obj_27,y
    clc
    adc.w #$DD00
    sta.w !obj_27,y
    sep.b #$20
.l9495:
    rtl

; @routine 09:9496 ObjectSkyBallLaunch
; WHAT IT DOES: Launches the sky ball: a new object of kind $AA34 linked to
; object X (+$27), handler ObjectSkyBallStep, $C0 up and $2BC0 on, scale
; $80, +$0A = +$49 = $0A, sound $0E; nothing without a free object.
; HOW: ObjectSpawn, ObjectCopyPositionTo; ends RTS.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: M 8-bit; Y = the
; new object; A low byte as the sound push leaves it.
%at($099496)
ObjectSkyBallLaunch:
    rep.b #$20
    lda.w #$AA34
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .l94AA
    jml.l .l951F
.l94AA:
    jsl.l ObjectCopyPositionTo
    pha
    rep.b #$20
    txa
    sta.w !obj_27,y
    sep.b #$20
    pla
    rep.b #$20
    lda.w #$9520
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,y
    rep.b #$20
    lda.w !obj_pos_x,y
    clc
    adc.w #$0000
    sta.w !obj_pos_x,y
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_y,y
    clc
    adc.w #$FF40
    sta.w !obj_pos_y,y
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_z,y
    clc
    adc.w #$2BC0
    sta.w !obj_pos_z,y
    sep.b #$20
    lda.b #$80
    sta.w !obj_15,y
    lda.b #$00
    sta.w !obj_48,y
    rep.b #$20
    lda.w #$0000
    sta.w !obj_29,y
    sep.b #$20
    rep.b #$20
    lda.w #$0000
    sta.w !obj_29+2,y
    sep.b #$20
    lda.b #$0A
    sta.w !obj_09+1,y
    lda.b #$0A
    sta.w !obj_49,y
    lda.b #$0E
    jsl.l SoundPort0QueuePush
.l951F:
    rts

; @routine 09:9520 ObjectSkyBallStep
; WHAT IT DOES: The sky ball's frame: while its +$10 has not passed its
; target +$27 it rises ($50 back, $F0 up, +$14 toward $C0) and is released
; when +$0A runs out; past it, for +$49 frames it follows the player, then
; the first time (+$25 bit 0) it drops a ball (kind $B7A8, handler
; $09:9634, ObjectBallDrop) and keeps falling ($180 back, $1E up, +$14
; toward $F8); rebucketed each frame.
; HOW: A 16-bit compare (BEQ or BMI); ApproachQuarterMin4, 03:CDC1 and
; 03:D07C (original code) ease the moves. Reached only by
; unit_objects_09.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as ObjectRebucket.
%at($099520)
ObjectSkyBallStep:
    nop
    rep.b #$20
    lda.w !obj_pos_z,x
    sta.w !wram_1F4C
    sep.b #$20
    rep.b #$20
    lda.w !obj_27,x
    cmp.w !wram_1F4C
    sep.b #$20
    beq .l953D
    bmi .l953D
    jml.l .l9591
.l953D:
    lda.w !obj_49,x
    bne .l9546
    jml.l .l954E
.l9546:
    dec.w !obj_49,x
    jsr.w ObjectBallFollowPlayer
    bra .l9594
.l954E:
    lda.w !obj_25,x
    and.b #$01
    beq .l9559
    jml.l .l958C
.l9559:
    lda.w !obj_25,x
    ora.b #$01
    sta.w !obj_25,x
    phx
    phy
    rep.b #$20
    lda.w #$B7A8
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .l9577
    jml.l .l958A
.l9577:
    jsl.l ObjectCopyPositionTo
    rep.b #$20
    lda.w #$9634
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,y
.l958A:
    ply
    plx
.l958C:
    jsr.w ObjectSkyBallFall
    bra .l9594
.l9591:
    jsr.w ObjectSkyBallRise
.l9594:
    jsl.l ObjectRebucket
    rtl

; @routine 09:9599 ObjectBallFollowPlayer
; WHAT IT DOES: Moves object X toward the player ($0346): its X eased toward
; the player's (ApproachQuarterMin4), its Z toward the player's
; (ApproachHalfMin2), then $380 back.
; HOW: 16-bit; ends RTS.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: M 8-bit; Y = the
; player's object; A as the last add leaves it.
%at($099599)
ObjectBallFollowPlayer:
    ldy.w !wram_0346
    rep.b #$20
    lda.w !obj_pos_x,x
    sta.b !scale_scratch
    lda.w !obj_pos_x,y
    jsl.l ApproachQuarterMin4
    sta.w !obj_pos_x,x
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_z,x
    sta.b !scale_scratch
    lda.w !obj_pos_z,y
    jsl.l ApproachHalfMin2
    sta.w !obj_pos_z,x
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_z,x
    clc
    adc.w #$FC80
    sta.w !obj_pos_z,x
    sep.b #$20
    rts

; @routine 09:95D1 ObjectSkyBallFall
; WHAT IT DOES: The falling sky ball's step: $180 back, $1E up, +$14 eased
; toward $F8 (ApproachSixteenthMin16Byte).
; HOW: 16-bit adds, then the byte eased; ends RTS.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: M 8-bit; A low byte
; = the new +$14.
%at($0995D1)
ObjectSkyBallFall:
    rep.b #$20
    lda.w !obj_pos_z,x
    clc
    adc.w #$FE80
    sta.w !obj_pos_z,x
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_y,x
    clc
    adc.w #$FFE2
    sta.w !obj_pos_y,x
    sep.b #$20
    lda.w !obj_14,x
    sta.b !scale_scratch
    lda.b #$F8
    jsl.l ApproachSixteenthMin16Byte
    sta.w !obj_14,x
    rts

; @routine 09:95FC ObjectSkyBallRise
; WHAT IT DOES: The rising sky ball's step: $50 back, $F0 up, +$14 eased
; toward $C0.
; HOW: As ObjectSkyBallFall.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as
; ObjectSkyBallFall.
%at($0995FC)
ObjectSkyBallRise:
    rep.b #$20
    lda.w !obj_pos_z,x
    clc
    adc.w #$FFB0
    sta.w !obj_pos_z,x
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_y,x
    clc
    adc.w #$FF10
    sta.w !obj_pos_y,x
    sep.b #$20
    lda.w !obj_14,x
    sta.b !scale_scratch
    lda.b #$C0
    jsl.l ApproachSixteenthMin16Byte
    sta.w !obj_14,x
    dec.w !obj_09+1,x
    beq .l962F
    jml.l .l9633
.l962F:
    jsl.l ObjectRelease
.l9633:
    rts

; @routine 09:9634 ObjectBallDrop
; WHAT IT DOES: Sets up a dropped ball: restart $09:9652
; (ObjectBallDropRecord), frame $09:967F (ObjectBallDropStep); its record
; and frame.
; HOW: Word and byte stores. Reached only by unit_objects_09.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
; leaves it.
%at($099634)
ObjectBallDrop:
    rep.b #$20
    lda.w #$9652
    sta.w !obj_1d,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1f,x
    rep.b #$20
    lda.w #$967F
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,x

; @routine 09:9652 ObjectBallDropRecord
; WHAT IT DOES: The dropped ball's restart: its record ($42D, flag bit 4,
; +$2A high $14, at ($A0, $A0, $A0)), then its frame.
; HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordSet2AHigh,
; GsuRecordPosition. Reached only by unit_objects_09.
; Entry: as ObjectBallDrop. Exit: as ObjectBallDrop.
%at($099652)
ObjectBallDropRecord:
    jsl.l GsuRecordNew42D
    jsl.l GsuRecordFlag10
    lda.b #$14
    jsl.l GsuRecordSet2AHigh
    rep.b #$20
    lda.w #$00A0
    sta.b !dp_02
    sep.b #$20
    rep.b #$20
    lda.w #$00A0
    sta.b !dp_08
    sep.b #$20
    rep.b #$20
    lda.w #$00A0
    sta.b !dp_94
    sep.b #$20
    jsl.l GsuRecordPosition

; @routine 09:967F ObjectBallDropStep
; WHAT IT DOES: The dropped ball's frame: it grows (+$15 and +$14 by $10);
; on a car's contact it raises that car's record +$6A by $40, plays sound
; $25, leaves a spark (kind $B7A8 at the player's position, handler
; $09:AE60) and is released.
; HOW: ObjectSyncFromRecord, then RecordNextContact over the contacts.
; Reached only by unit_objects_09.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
; it.
%at($09967F)
ObjectBallDropStep:
    lda.w !obj_15,x
    clc
    adc.b #$10
    sta.w !obj_15,x
    lda.w !obj_14,x
    clc
    adc.b #$10
    sta.w !obj_14,x
    jsl.l ObjectSyncFromRecord
    and.b #$01
    beq .l96FB
.l9699:
    jsl.l RecordNextContact
    bcc .l96FB
    and.b #$01
    beq .l9699
    lda.b #$40
    jsl.l GsuRecordRaise6A
    sep.b #$20
    lda.b #$25
    jsl.l SoundPort0QueuePush
    phx
    tyx
    rep.b #$20
    lda.w #$B7A8
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .l96C7
    jml.l .l96F6
.l96C7:
    ldx.w !wram_0346
    lda.b #$0A
    sta.w !obj_09+1,y
    jsl.l ObjectCopyPositionTo
    rep.b #$20
    lda.w #$0000
    sta.w !obj_27,y
    sep.b #$20
    pha
    rep.b #$20
    txa
    sta.w !obj_29,y
    sep.b #$20
    pla
    rep.b #$20
    lda.w #$AE60
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,y
.l96F6:
    plx
    jsl.l ObjectRelease
.l96FB:
    rtl

; @routine 09:96FC ObjectCannonGate
; WHAT IT DOES: Sets up a cannon gate: restart $09:971F
; (ObjectCannonGateRecord), frame $09:9746 (ObjectCannonGateStep), its lap
; +$0A = 4; then its record and frame.
; HOW: Word and byte stores.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
; leaves it.
%at($0996FC)
ObjectCannonGate:
    rep.b #$20
    lda.w #$971F
    sta.w !obj_1d,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1f,x
    rep.b #$20
    lda.w #$9746
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,x
    lda.b #$04
    sta.w !obj_09+1,x

; @routine 09:971F ObjectCannonGateRecord
; WHAT IT DOES: The cannon gate's restart: its record ($42D, flag bit 4, at
; ($F00, $F00, $F00)), then its frame.
; HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition.
; Entry: as ObjectCannonGate. Exit: as ObjectCannonGate.
%at($09971F)
ObjectCannonGateRecord:
    jsl.l GsuRecordNew42D
    jsl.l GsuRecordFlag10
    rep.b #$20
    lda.w #$0F00
    sta.b !dp_02
    sep.b #$20
    rep.b #$20
    lda.w #$0F00
    sta.b !dp_08
    sep.b #$20
    rep.b #$20
    lda.w #$0F00
    sta.b !dp_94
    sep.b #$20
    jsl.l GsuRecordPosition

; @routine 09:9746 ObjectCannonGateStep
; WHAT IT DOES: The cannon gate's frame: when a car hits it on a lap ($0DEA)
; other than the one it last fired on (+$0A), it takes that lap; on lap 0
; it fires three cannon balls to one side (kind $AA34, handler
; ObjectCannonBallLeft), on lap 2 three to the other (ObjectCannonBallRight),
; each with its own drop, rise and drift, sound $0E.
; HOW: ObjectSyncFromRecord, RecordContactSummary; the lap compared through
; $1F58. The firing is reached only by unit_objects_09.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
; it.
%at($099746)
ObjectCannonGateStep:
    jsl.l ObjectSyncFromRecord
    and.b #$01
    beq .l9792
    jsl.l RecordContactSummary
    and.b #$01
    beq .l9792
    lda.w !obj_09+1,x
    sta.w !wram_1F58
    sep.b #$20
    lda.w !wram_0DEA
    cmp.w !wram_1F58
    beq .l9792
    lda.w !wram_0DEA
    sta.w !wram_1F58
    lda.w !wram_1F58
    sta.w !obj_09+1,x
    sep.b #$20
    lda.w !wram_0DEA
    cmp.b #$00
    beq .l9786
    cmp.b #$01
    beq .l978C
    cmp.b #$02
    beq .l978F
    jmp.w .l9792
.l9786:
    jsr.w ObjectCannonBallsLeft
    jmp.w .l9792
.l978C:
    jmp.w .l9792
.l978F:
    jsr.w ObjectCannonBallsRight
.l9792:
    rtl

; @routine 09:9793 ObjectCannonBallsLeft
; WHAT IT DOES: Fires the first lap's three cannon balls (kind via
; ObjectSpawn, handler ObjectCannonBallLeft) from object X, $3C00 to the
; left, at three heights and spreads.
; HOW: ObjectSpawn, ObjectCopyPositionTo for each; ends RTS.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: M 8-bit; A, Y as
; the third ball leaves them.
%at($099793)
ObjectCannonBallsLeft:
    rep.b #$20
    lda.w #$AA34
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .l97A7
    jml.l .l981C
.l97A7:
    jsl.l ObjectCopyPositionTo
    pha
    rep.b #$20
    txa
    sta.w !obj_27,y
    sep.b #$20
    pla
    rep.b #$20
    lda.w #$9ACB
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,y
    rep.b #$20
    lda.w !obj_pos_x,y
    clc
    adc.w #$C400
    sta.w !obj_pos_x,y
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_y,y
    clc
    adc.w #$F880
    sta.w !obj_pos_y,y
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_z,y
    clc
    adc.w #$0500
    sta.w !obj_pos_z,y
    sep.b #$20
    lda.b #$C0
    sta.w !obj_15,y
    lda.b #$00
    sta.w !obj_48,y
    rep.b #$20
    lda.w #$FD60
    sta.w !obj_29,y
    sep.b #$20
    rep.b #$20
    lda.w #$0000
    sta.w !obj_29+2,y
    sep.b #$20
    lda.b #$05
    sta.w !obj_09+1,y
    lda.b #$09
    sta.w !obj_49,y
    lda.b #$0E
    jsl.l SoundPort0QueuePush
.l981C:
    rep.b #$20
    lda.w #$AA34
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .l9830
    jml.l .l98A5
.l9830:
    jsl.l ObjectCopyPositionTo
    pha
    rep.b #$20
    txa
    sta.w !obj_27,y
    sep.b #$20
    pla
    rep.b #$20
    lda.w #$9ACB
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,y
    rep.b #$20
    lda.w !obj_pos_x,y
    clc
    adc.w #$C400
    sta.w !obj_pos_x,y
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_y,y
    clc
    adc.w #$FC40
    sta.w !obj_pos_y,y
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_z,y
    clc
    adc.w #$F880
    sta.w !obj_pos_z,y
    sep.b #$20
    lda.b #$C0
    sta.w !obj_15,y
    lda.b #$00
    sta.w !obj_48,y
    rep.b #$20
    lda.w #$FD40
    sta.w !obj_29,y
    sep.b #$20
    rep.b #$20
    lda.w #$FE00
    sta.w !obj_29+2,y
    sep.b #$20
    lda.b #$05
    sta.w !obj_09+1,y
    lda.b #$09
    sta.w !obj_49,y
    lda.b #$0E
    jsl.l SoundPort0QueuePush
.l98A5:
    rep.b #$20
    lda.w #$AA34
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .l98B9
    jml.l .l992E
.l98B9:
    jsl.l ObjectCopyPositionTo
    pha
    rep.b #$20
    txa
    sta.w !obj_27,y
    sep.b #$20
    pla
    rep.b #$20
    lda.w #$9ACB
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,y
    rep.b #$20
    lda.w !obj_pos_x,y
    clc
    adc.w #$C400
    sta.w !obj_pos_x,y
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_y,y
    clc
    adc.w #$F9C0
    sta.w !obj_pos_y,y
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_z,y
    clc
    adc.w #$0000
    sta.w !obj_pos_z,y
    sep.b #$20
    lda.b #$C0
    sta.w !obj_15,y
    lda.b #$00
    sta.w !obj_48,y
    rep.b #$20
    lda.w #$FD20
    sta.w !obj_29,y
    sep.b #$20
    rep.b #$20
    lda.w #$0200
    sta.w !obj_29+2,y
    sep.b #$20
    lda.b #$05
    sta.w !obj_09+1,y
    lda.b #$09
    sta.w !obj_49,y
    lda.b #$0E
    jsl.l SoundPort0QueuePush
.l992E:
    rts

; @routine 09:992F ObjectCannonBallsRight
; WHAT IT DOES: As ObjectCannonBallsLeft for the third lap: handler
; ObjectCannonBallRight, $2580 to the right.
; HOW: As ObjectCannonBallsLeft.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as
; ObjectCannonBallsLeft.
%at($09992F)
ObjectCannonBallsRight:
    rep.b #$20
    lda.w #$AA34
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .l9943
    jml.l .l99B8
.l9943:
    jsl.l ObjectCopyPositionTo
    pha
    rep.b #$20
    txa
    sta.w !obj_27,y
    sep.b #$20
    pla
    rep.b #$20
    lda.w #$9AE8
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,y
    rep.b #$20
    lda.w !obj_pos_x,y
    clc
    adc.w #$2580
    sta.w !obj_pos_x,y
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_y,y
    clc
    adc.w #$F880
    sta.w !obj_pos_y,y
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_z,y
    clc
    adc.w #$0500
    sta.w !obj_pos_z,y
    sep.b #$20
    lda.b #$40
    sta.w !obj_15,y
    lda.b #$00
    sta.w !obj_48,y
    rep.b #$20
    lda.w #$FDC0
    sta.w !obj_29,y
    sep.b #$20
    rep.b #$20
    lda.w #$0000
    sta.w !obj_29+2,y
    sep.b #$20
    lda.b #$12
    sta.w !obj_09+1,y
    lda.b #$06
    sta.w !obj_49,y
    lda.b #$0E
    jsl.l SoundPort0QueuePush
.l99B8:
    rep.b #$20
    lda.w #$AA34
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .l99CC
    jml.l .l9A41
.l99CC:
    jsl.l ObjectCopyPositionTo
    pha
    rep.b #$20
    txa
    sta.w !obj_27,y
    sep.b #$20
    pla
    rep.b #$20
    lda.w #$9AE8
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,y
    rep.b #$20
    lda.w !obj_pos_x,y
    clc
    adc.w #$2580
    sta.w !obj_pos_x,y
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_y,y
    clc
    adc.w #$FC40
    sta.w !obj_pos_y,y
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_z,y
    clc
    adc.w #$F880
    sta.w !obj_pos_z,y
    sep.b #$20
    lda.b #$40
    sta.w !obj_15,y
    lda.b #$00
    sta.w !obj_48,y
    rep.b #$20
    lda.w #$FDA0
    sta.w !obj_29,y
    sep.b #$20
    rep.b #$20
    lda.w #$FF00
    sta.w !obj_29+2,y
    sep.b #$20
    lda.b #$12
    sta.w !obj_09+1,y
    lda.b #$06
    sta.w !obj_49,y
    lda.b #$0E
    jsl.l SoundPort0QueuePush
.l9A41:
    rep.b #$20
    lda.w #$AA34
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .l9A55
    jml.l .l9ACA
.l9A55:
    jsl.l ObjectCopyPositionTo
    pha
    rep.b #$20
    txa
    sta.w !obj_27,y
    sep.b #$20
    pla
    rep.b #$20
    lda.w #$9AE8
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,y
    rep.b #$20
    lda.w !obj_pos_x,y
    clc
    adc.w #$2580
    sta.w !obj_pos_x,y
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_y,y
    clc
    adc.w #$F9C0
    sta.w !obj_pos_y,y
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_z,y
    clc
    adc.w #$0000
    sta.w !obj_pos_z,y
    sep.b #$20
    lda.b #$40
    sta.w !obj_15,y
    lda.b #$00
    sta.w !obj_48,y
    rep.b #$20
    lda.w #$FDC0
    sta.w !obj_29,y
    sep.b #$20
    rep.b #$20
    lda.w #$0100
    sta.w !obj_29+2,y
    sep.b #$20
    lda.b #$12
    sta.w !obj_09+1,y
    lda.b #$06
    sta.w !obj_49,y
    lda.b #$0E
    jsl.l SoundPort0QueuePush
.l9ACA:
    rts

; @routine 09:9ACB ObjectCannonBallLeft
; WHAT IT DOES: A cannon ball's frame: for +$49 frames it homes on its gate
; (+$27; X and Z eased to the gate's, then its drift +$2B and rise +$29
; added), then it flies off ($100 a frame along X, drifting) until its life
; +$0A runs out; rebucketed.
; HOW: ApproachQuarterMin4, 03:CDC1 (original code), ObjectRelease,
; ObjectRebucket. Reached only by unit_objects_09.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as ObjectRebucket.
%at($099ACB)
ObjectCannonBallLeft:
    nop
    ldy.w !obj_27,x
    lda.w !obj_49,x
    bne .l9AD8
    jml.l .l9AE0
.l9AD8:
    dec.w !obj_49,x
    jsr.w ObjectCannonBallHome
    bra .l9AE3
.l9AE0:
    jsr.w ObjectCannonBallFlyLeft
.l9AE3:
    jsl.l ObjectRebucket
    rtl

; @routine 09:9AE8 ObjectCannonBallRight
; WHAT IT DOES: As ObjectCannonBallLeft, flying off the other way ($380 a
; frame back along X).
; HOW: As ObjectCannonBallLeft. Reached only by unit_objects_09.
; Entry: as ObjectCannonBallLeft. Exit: as ObjectCannonBallLeft.
%at($099AE8)
ObjectCannonBallRight:
    nop
    ldy.w !obj_27,x
    lda.w !obj_49,x
    bne .l9AF5
    jml.l .l9AFD
.l9AF5:
    dec.w !obj_49,x
    jsr.w ObjectCannonBallHome
    bra .l9B00
.l9AFD:
    jsr.w ObjectCannonBallFlyRight
.l9B00:
    jsl.l ObjectRebucket
    rtl

; @routine 09:9B05 ObjectCannonBallFlyLeft
; WHAT IT DOES: The left cannon ball flies: Z += +$2B, X += $100; released
; (ObjectRelease) when +$0A runs out.
; HOW: 16-bit adds, a byte DEC; ends RTS.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
; it.
%at($099B05)
ObjectCannonBallFlyLeft:
    rep.b #$20
    lda.w !obj_29+2,x
    sta.w !wram_1F4C
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_z,x
    clc
    adc.w !wram_1F4C
    sta.w !obj_pos_z,x
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_x,x
    clc
    adc.w #$0100
    sta.w !obj_pos_x,x
    sep.b #$20
    dec.w !obj_09+1,x
    beq .l9B34
    jml.l .l9B38
.l9B34:
    jsl.l ObjectRelease
.l9B38:
    rts

; @routine 09:9B39 ObjectCannonBallFlyRight
; WHAT IT DOES: As ObjectCannonBallFlyLeft with X -= $380.
; HOW: As ObjectCannonBallFlyLeft.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
; it.
%at($099B39)
ObjectCannonBallFlyRight:
    rep.b #$20
    lda.w !obj_29+2,x
    sta.w !wram_1F4C
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_z,x
    clc
    adc.w !wram_1F4C
    sta.w !obj_pos_z,x
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_x,x
    clc
    adc.w #$FC80
    sta.w !obj_pos_x,x
    sep.b #$20
    dec.w !obj_09+1,x
    beq .l9B68
    jml.l .l9B6C
.l9B68:
    jsl.l ObjectRelease
.l9B6C:
    rts

; @routine 09:9B6D ObjectCannonBallHome
; WHAT IT DOES: The cannon ball homes on its gate (Y): X and Z eased toward
; the gate's (ApproachQuarterMin4), Z += +$2B; Y eased toward the gate's
; (ApproachHalfMin2), Y += +$29.
; HOW: 16-bit eases and adds; ends RTS.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: M 8-bit; A as the
; last add leaves it.
%at($099B6D)
ObjectCannonBallHome:
    rep.b #$20
    lda.w !obj_pos_x,x
    sta.b !scale_scratch
    lda.w !obj_pos_x,y
    jsl.l ApproachQuarterMin4
    sta.w !obj_pos_x,x
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_z,x
    sta.b !scale_scratch
    lda.w !obj_pos_z,y
    jsl.l ApproachQuarterMin4
    sta.w !obj_pos_z,x
    sep.b #$20
    rep.b #$20
    lda.w !obj_29+2,x
    sta.w !wram_1F4C
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_z,x
    clc
    adc.w !wram_1F4C
    sta.w !obj_pos_z,x
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_y,x
    sta.b !scale_scratch
    lda.w !obj_pos_y,y
    jsl.l ApproachHalfMin2
    sta.w !obj_pos_y,x
    sep.b #$20
    rep.b #$20
    lda.w !obj_29,x
    sta.w !wram_1F4C
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_y,x
    clc
    adc.w !wram_1F4C
    sta.w !obj_pos_y,x
    sep.b #$20
    rts

; @routine 09:9BD7 ObjectWaveGateA
; WHAT IT DOES: Sets up the first wave gate: restart $09:9BF5
; (ObjectWaveGateRecord), frame $09:9C1C (ObjectWaveGateAStep); its record
; and frame.
; HOW: Word and byte stores.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
; leaves it.
%at($099BD7)
ObjectWaveGateA:
    rep.b #$20
    lda.w #$9BF5
    sta.w !obj_1d,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1f,x
    rep.b #$20
    lda.w #$9C1C
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,x

; @routine 09:9BF5 ObjectWaveGateRecord
; WHAT IT DOES: A wave gate's restart (all three use it): its record
; ($42D, flag bit 4, at ($F00, $F00, $F00)), then the first gate's frame
; (ObjectWaveGateAStep).
; HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition.
; Entry: as ObjectWaveGateA. Exit: as ObjectWaveGateA.
%at($099BF5)
ObjectWaveGateRecord:
    jsl.l GsuRecordNew42D
    jsl.l GsuRecordFlag10
    rep.b #$20
    lda.w #$0F00
    sta.b !dp_02
    sep.b #$20
    rep.b #$20
    lda.w #$0F00
    sta.b !dp_08
    sep.b #$20
    rep.b #$20
    lda.w #$0F00
    sta.b !dp_94
    sep.b #$20
    jsl.l GsuRecordPosition

; @routine 09:9C1C ObjectWaveGateAStep
; WHAT IT DOES: The first wave gate's frame: when a car hits it on lap 1
; ($0DEA), the first time (+$25 bit 0), a wave ball (kind $AA34, handler
; ObjectWaveBallBig) $2580 left, $180 up and $2300 back, and the gate moves
; $F00 on in X and Z.
; HOW: ObjectSyncToRecord, RecordContactSummary, ObjectSpawn.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
; it.
%at($099C1C)
ObjectWaveGateAStep:
    jsl.l ObjectSyncToRecord
    and.b #$01
    beq .l9C49
    jsl.l RecordContactSummary
    and.b #$01
    beq .l9C49
    lda.w !wram_0DEA
    cmp.b #$01
    bne .l9C49
    lda.w !obj_25,x
    and.b #$01
    beq .l9C3E
    jml.l .l9C49
.l9C3E:
    lda.w !obj_25,x
    ora.b #$01
    sta.w !obj_25,x
    jsr.w ObjectWaveBallLaunchA
.l9C49:
    rtl

; @routine 09:9C4A ObjectWaveGateB
; WHAT IT DOES: Sets up the second wave gate: restart $09:9BF5
; (ObjectWaveGateRecord), frame $09:9C69 (ObjectWaveGateBStep).
; HOW: Word and byte stores.
; Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; A low byte = 9.
%at($099C4A)
ObjectWaveGateB:
    rep.b #$20
    lda.w #$9BF5
    sta.w !obj_1d,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1f,x
    rep.b #$20
    lda.w #$9C69
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,x
    rtl

; @routine 09:9C69 ObjectWaveGateBStep
; WHAT IT DOES: As ObjectWaveGateAStep with a smaller ball (handler
; ObjectWaveBallStep, $2300 right, $3C0 up, $1E00 back, scale $20, life
; $A); the gate moves $F00 back in X and on in Z.
; HOW: As ObjectWaveGateAStep.
; Entry: as ObjectWaveGateAStep. Exit: as ObjectWaveGateAStep.
%at($099C69)
ObjectWaveGateBStep:
    jsl.l ObjectSyncToRecord
    and.b #$01
    beq .l9C96
    jsl.l RecordContactSummary
    and.b #$01
    beq .l9C96
    lda.w !wram_0DEA
    cmp.b #$01
    bne .l9C96
    lda.w !obj_25,x
    and.b #$01
    beq .l9C8B
    jml.l .l9C96
.l9C8B:
    lda.w !obj_25,x
    ora.b #$01
    sta.w !obj_25,x
    jsr.w ObjectWaveBallLaunchB
.l9C96:
    rtl

; @routine 09:9C97 ObjectWaveGateC
; WHAT IT DOES: Sets up the third wave gate: restart $09:9BF5
; (ObjectWaveGateRecord), frame $09:9CB6 (ObjectWaveGateCStep).
; HOW: Word and byte stores.
; Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; A low byte = 9.
%at($099C97)
ObjectWaveGateC:
    rep.b #$20
    lda.w #$9BF5
    sta.w !obj_1d,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1f,x
    rep.b #$20
    lda.w #$9CB6
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,x
    rtl

; @routine 09:9CB6 ObjectWaveGateCStep
; WHAT IT DOES: As ObjectWaveGateBStep with the ball $1E00 on (scale $60,
; life $C); the gate moves $F00 back in X and Z.
; HOW: As ObjectWaveGateAStep.
; Entry: as ObjectWaveGateAStep. Exit: as ObjectWaveGateAStep.
%at($099CB6)
ObjectWaveGateCStep:
    jsl.l ObjectSyncToRecord
    and.b #$01
    beq .l9CE3
    jsl.l RecordContactSummary
    and.b #$01
    beq .l9CE3
    lda.w !wram_0DEA
    cmp.b #$01
    bne .l9CE3
    lda.w !obj_25,x
    and.b #$01
    beq .l9CD8
    jml.l .l9CE3
.l9CD8:
    lda.w !obj_25,x
    ora.b #$01
    sta.w !obj_25,x
    jsr.w ObjectWaveBallLaunchC
.l9CE3:
    rtl

; @routine 09:9CE4 ObjectWaveBallLaunchA
; WHAT IT DOES: Launches the first wave gate's big ball (kind $AA34, handler
; ObjectWaveBallBig) linked to gate X: $2580 left, $180 up, $2300 back,
; scale $E0, rise -$100, life $0F; the gate moves $F00 on in X and Z; sound
; $0E. Nothing without a free object.
; HOW: ObjectSpawn, ObjectCopyPositionTo, 16-bit adds; ends RTS.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: M 8-bit; Y = the
; new object; A low byte as the sound push leaves it.
%at($099CE4)
ObjectWaveBallLaunchA:
    rep.b #$20
    lda.w #$AA34
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .l9CF8
    jml.l .l9D75
.l9CF8:
    jsl.l ObjectCopyPositionTo
    pha
    rep.b #$20
    txa
    sta.w !obj_27,y
    sep.b #$20
    pla
    rep.b #$20
    lda.w #$9F05
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,y
    rep.b #$20
    lda.w !obj_pos_x,y
    clc
    adc.w #$DA80
    sta.w !obj_pos_x,y
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_y,y
    clc
    adc.w #$FE80
    sta.w !obj_pos_y,y
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_z,y
    clc
    adc.w #$DD00
    sta.w !obj_pos_z,y
    sep.b #$20
    lda.b #$E0
    sta.w !obj_15,y
    rep.b #$20
    lda.w #$FF00
    sta.w !obj_29,y
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_x,x
    clc
    adc.w #$0F00
    sta.w !obj_pos_x,x
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_z,x
    clc
    adc.w #$0F00
    sta.w !obj_pos_z,x
    sep.b #$20
    lda.b #$0F
    sta.w !obj_09+1,y
    lda.b #$0E
    jsl.l SoundPort0QueuePush
.l9D75:
    rts

; @routine 09:9D76 ObjectWaveBallLaunchB
; WHAT IT DOES: As ObjectWaveBallLaunchA for the second gate: handler
; ObjectWaveBallStep, $2300 right, $3C0 up, $1E00 back, scale $20, rise
; -$A0, life $0A; the gate moves $F00 back in X and on in Z.
; HOW: As ObjectWaveBallLaunchA.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as
; ObjectWaveBallLaunchA.
%at($099D76)
ObjectWaveBallLaunchB:
    rep.b #$20
    lda.w #$AA34
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .l9D8A
    jml.l .l9E07
.l9D8A:
    jsl.l ObjectCopyPositionTo
    pha
    rep.b #$20
    txa
    sta.w !obj_27,y
    sep.b #$20
    pla
    rep.b #$20
    lda.w #$9E9A
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,y
    rep.b #$20
    lda.w !obj_pos_x,y
    clc
    adc.w #$2300
    sta.w !obj_pos_x,y
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_y,y
    clc
    adc.w #$FC40
    sta.w !obj_pos_y,y
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_z,y
    clc
    adc.w #$E200
    sta.w !obj_pos_z,y
    sep.b #$20
    lda.b #$20
    sta.w !obj_15,y
    rep.b #$20
    lda.w #$FF60
    sta.w !obj_29,y
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_x,x
    clc
    adc.w #$F100
    sta.w !obj_pos_x,x
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_z,x
    clc
    adc.w #$0F00
    sta.w !obj_pos_z,x
    sep.b #$20
    lda.b #$0A
    sta.w !obj_09+1,y
    lda.b #$0E
    jsl.l SoundPort0QueuePush
.l9E07:
    rts

; @routine 09:9E08 ObjectWaveBallLaunchC
; WHAT IT DOES: As ObjectWaveBallLaunchB for the third gate: the ball $1E00
; on, scale $60, life $0C; the gate moves $F00 back in X and Z.
; HOW: As ObjectWaveBallLaunchA.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as
; ObjectWaveBallLaunchA.
%at($099E08)
ObjectWaveBallLaunchC:
    rep.b #$20
    lda.w #$AA34
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .l9E1C
    jml.l .l9E99
.l9E1C:
    jsl.l ObjectCopyPositionTo
    pha
    rep.b #$20
    txa
    sta.w !obj_27,y
    sep.b #$20
    pla
    rep.b #$20
    lda.w #$9E9A
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,y
    rep.b #$20
    lda.w !obj_pos_x,y
    clc
    adc.w #$2300
    sta.w !obj_pos_x,y
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_y,y
    clc
    adc.w #$FC40
    sta.w !obj_pos_y,y
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_z,y
    clc
    adc.w #$1E00
    sta.w !obj_pos_z,y
    sep.b #$20
    lda.b #$60
    sta.w !obj_15,y
    rep.b #$20
    lda.w #$FF60
    sta.w !obj_29,y
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_x,x
    clc
    adc.w #$F100
    sta.w !obj_pos_x,x
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_z,x
    clc
    adc.w #$F100
    sta.w !obj_pos_z,x
    sep.b #$20
    lda.b #$0C
    sta.w !obj_09+1,y
    lda.b #$0E
    jsl.l SoundPort0QueuePush
.l9E99:
    rts

; @routine 09:9E9A ObjectWaveBallStep
; WHAT IT DOES: A wave ball's frame: it homes on its gate (+$27; X, Z and
; Y eased by ApproachQuarterMin4), rises by +$29, is released when its
; life +$0A runs out; rebucketed.
; HOW: The NOP first, as the course pointers expect. Reached only by
; unit_objects_09.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as ObjectRebucket.
%at($099E9A)
ObjectWaveBallStep:
    nop
    ldy.w !obj_27,x
    jsr.w ObjectWaveBallHome
    jsl.l ObjectRebucket
    rtl

; @routine 09:9EA6 ObjectWaveBallHome
; WHAT IT DOES: The wave ball homes on Y: X, Z and Y eased toward Y's
; (ApproachQuarterMin4), rises by +$29, and is released when +$0A runs out.
; HOW: 16-bit eases and adds, a byte DEC; ends RTS.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
; it.
%at($099EA6)
ObjectWaveBallHome:
    rep.b #$20
    lda.w !obj_pos_x,x
    sta.b !scale_scratch
    lda.w !obj_pos_x,y
    jsl.l ApproachQuarterMin4
    sta.w !obj_pos_x,x
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_z,x
    sta.b !scale_scratch
    lda.w !obj_pos_z,y
    jsl.l ApproachQuarterMin4
    sta.w !obj_pos_z,x
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_y,x
    sta.b !scale_scratch
    lda.w !obj_pos_y,y
    jsl.l ApproachQuarterMin4
    sta.w !obj_pos_y,x
    sep.b #$20
    rep.b #$20
    lda.w !obj_29,x
    sta.w !wram_1F4C
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_y,x
    clc
    adc.w !wram_1F4C
    sta.w !obj_pos_y,x
    sep.b #$20
    dec.w !obj_09+1,x
    beq .l9F00
    jml.l .l9F04
.l9F00:
    jsl.l ObjectRelease
.l9F04:
    rts

; @routine 09:9F05 ObjectWaveBallBig
; WHAT IT DOES: The first wave gate's ball: its handler $09:9F14
; (ObjectWaveBallBigSetup), then that set-up at once.
; HOW: Word and byte stores. Reached only by unit_objects_09.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
; leaves it.
%at($099F05)
ObjectWaveBallBig:
    rep.b #$20
    lda.w #$9F14
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,x

; @routine 09:9F14 ObjectWaveBallBigSetup
; WHAT IT DOES: The big wave ball's set-up: restart $09:9F3C
; (ObjectWaveBallBigRecord), frame $09:9F63 (ObjectWaveBallBigStep), rise
; +$29 = -$50; its record ($42D, flag bit 4, at ($320, $320, $320)) and
; frame.
; HOW: Word and byte stores. Reached only by unit_objects_09.
; Entry: as ObjectWaveBallBig. Exit: as ObjectWaveBallBig.
%at($099F14)
ObjectWaveBallBigSetup:
    rep.b #$20
    lda.w #$9F3C
    sta.w !obj_1d,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1f,x
    rep.b #$20
    lda.w #$9F63
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,x
    rep.b #$20
    lda.w #$FFB0
    sta.w !obj_29,x
    sep.b #$20

; @routine 09:9F3C ObjectWaveBallBigRecord
; WHAT IT DOES: The big wave ball's restart: its record ($42D, flag bit 4,
; at ($320, $320, $320)), then its frame.
; HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition. Reached only
; by unit_objects_09.
; Entry: as ObjectWaveBallBig. Exit: as ObjectWaveBallBig.
%at($099F3C)
ObjectWaveBallBigRecord:
    jsl.l GsuRecordNew42D
    jsl.l GsuRecordFlag10
    rep.b #$20
    lda.w #$0320
    sta.b !dp_02
    sep.b #$20
    rep.b #$20
    lda.w #$0320
    sta.b !dp_08
    sep.b #$20
    rep.b #$20
    lda.w #$0320
    sta.b !dp_94
    sep.b #$20
    jsl.l GsuRecordPosition

; @routine 09:9F63 ObjectWaveBallBigStep
; WHAT IT DOES: The big wave ball's frame: once a car has hit it (+$25 bit
; 0) it grows (+$16, +$15 by $18), flies off ($32 back, $28A right, $FA
; up) and leaves a sparkle (kind $A158, handler ObjectStarSparkleStep) each
; frame; until then it homes on its gate. A hit sets +$25 bit 0 the first
; time with sound $59.
; HOW: ObjectSyncToRecord, RecordContactSummary. Reached only by
; unit_objects_09.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
; it.
%at($099F63)
ObjectWaveBallBigStep:
    lda.w !obj_25,x
    and.b #$01
    bne .l9F6E
    jml.l .l9FAF
.l9F6E:
    lda.w !obj_16,x
    clc
    adc.b #$18
    sta.w !obj_16,x
    lda.w !obj_15,x
    clc
    adc.b #$18
    sta.w !obj_15,x
    rep.b #$20
    lda.w !obj_pos_z,x
    clc
    adc.w #$FFCE
    sta.w !obj_pos_z,x
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_x,x
    clc
    adc.w #$028A
    sta.w !obj_pos_x,x
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_y,x
    clc
    adc.w #$FF06
    sta.w !obj_pos_y,x
    sep.b #$20
    jsr.w ObjectStarSparkleSpawn
    bra .l9FB7
.l9FAF:
    phy
    ldy.w !obj_27,x
    jsr.w ObjectWaveBallHome
    ply
.l9FB7:
    jsl.l ObjectSyncToRecord
    and.b #$01
    beq .l9FE0
    jsl.l RecordContactSummary
    and.b #$01
    beq .l9FE0
    lda.w !obj_25,x
    and.b #$01
    beq .l9FD2
    jml.l .l9FE0
.l9FD2:
    lda.w !obj_25,x
    ora.b #$01
    sta.w !obj_25,x
    lda.b #$59
    jsl.l SoundPort0QueuePush
.l9FE0:
    rtl

; @routine 09:9FE1 ObjectStarSparkleSpawn
; WHAT IT DOES: Leaves a sparkle: a new object of kind $A158 lit by $997C
; (+$5F = $64), frame +$18 = $82, at object X's position $C8 back, life 8,
; handler ObjectStarSparkleStep; nothing without a free object.
; HOW: ObjectSpawn, ObjectCopyPositionTo; ends RTS.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: M 8-bit; Y = the
; new object; A low byte = 9.
%at($099FE1)
ObjectStarSparkleSpawn:
    rep.b #$20
    lda.w #$A158
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .l9FF5
    jml.l .lA03E
.l9FF5:
    rep.b #$20
    lda.w #$997C
    sta.w !obj_5d,y
    sep.b #$20
    lda.b #$02
    ora.b #$80
    sta.w !obj_18,y
    lda.w !obj_24,y
    ora.b #$08
    sta.w !obj_24,y
    lda.b #$00
    sta.w !obj_3a,y
    lda.b #$64
    sta.w !obj_5f,y
    jsl.l ObjectCopyPositionTo
    rep.b #$20
    lda.w !obj_pos_z,y
    clc
    adc.w #$FF38
    sta.w !obj_pos_z,y
    sep.b #$20
    lda.b #$08
    sta.w !obj_09+1,y
    rep.b #$20
    lda.w #$A03F
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,y
.lA03E:
    rts

; @routine 09:A03F ObjectStarSparkleStep
; WHAT IT DOES: A sparkle's frame: released when its life +$0A runs out;
; its picture frame +$18 steps through 0-5 (bit 7 kept); rebucketed.
; HOW: Byte adds and compares. Reached only by unit_objects_09.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as ObjectRebucket.
%at($09A03F)
ObjectStarSparkleStep:
    dec.w !obj_09+1,x
    beq .lA048
    jml.l .lA04C
.lA048:
    jsl.l ObjectRelease
.lA04C:
    lda.w !obj_18,x
    clc
    adc.b #$01
    bmi .lA057
    clc
    adc.b #$06
.lA057:
    and.b #$7F
    cmp.b #$06
    bcc .lA060
    sec
    sbc.b #$06
.lA060:
    ora.b #$80
    sta.w !obj_18,x
    jsl.l ObjectRebucket
    rtl

; @routine 09:A06A ObjectLantern
; WHAT IT DOES: Sets up a lantern: $78 higher, lit by $996D (+$24 bit 3,
; +$3A = 0, +$5F = $64), restart $09:A0B2 (ObjectLanternRecord), frame
; $09:A0C0 (ObjectLanternStep); its record and frame.
; HOW: Word and byte stores.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
; leaves it.
%at($09A06A)
ObjectLantern:
    rep.b #$20
    lda.w !obj_pos_y,x
    clc
    adc.w #$FF88
    sta.w !obj_pos_y,x
    sep.b #$20
    rep.b #$20
    lda.w #$996D
    sta.w !obj_5d,x
    sep.b #$20
    lda.w !obj_24,x
    ora.b #$08
    sta.w !obj_24,x
    lda.b #$00
    sta.w !obj_3a,x
    lda.b #$64
    sta.w !obj_5f,x
    rep.b #$20
    lda.w #$A0B2
    sta.w !obj_1d,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1f,x
    rep.b #$20
    lda.w #$A0C0
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,x

; @routine 09:A0B2 ObjectLanternRecord
; WHAT IT DOES: The lantern's restart: its record (GsuRecordNew424Alt, flag
; bit 4, +$2A high 2), then its frame.
; HOW: GsuRecordNew424Alt, GsuRecordFlag10, GsuRecordSet2AHigh.
; Entry: as ObjectLantern. Exit: as ObjectLantern.
%at($09A0B2)
ObjectLanternRecord:
    jsl.l GsuRecordNew424Alt
    jsl.l GsuRecordFlag10
    lda.b #$02
    jsl.l GsuRecordSet2AHigh

; @routine 09:A0C0 ObjectLanternStep
; WHAT IT DOES: The lantern's frame: when a car hits it the first time
; (+$25 bit 0), sound $10, it is released and bursts into four sparks
; (kind $A0CC, handler ObjectFlyingPiece); no hit clears the bit.
; HOW: ObjectSyncFromRecord, RecordContactSummary, ObjectSpawn.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
; it.
%at($09A0C0)
ObjectLanternStep:
    jsl.l ObjectSyncFromRecord
    and.b #$01
    beq .lA0F1
    jsl.l RecordContactSummary
    and.b #$01
    beq .lA0F1
    lda.w !obj_25,x
    and.b #$01
    beq .lA0DB
    jml.l .lA0F0
.lA0DB:
    lda.w !obj_25,x
    ora.b #$01
    sta.w !obj_25,x
    lda.b #$10
    jsl.l SoundPort0QueuePush
    jsl.l ObjectRelease
    jsr.w ObjectLanternBurst
.lA0F0:
    rtl
.lA0F1:
    lda.w !obj_25,x
    and.b #$FE
    sta.w !obj_25,x
    rtl

; @routine 09:A0FA ObjectLanternBurst
; WHAT IT DOES: Bursts the lantern into four sparks (kind $A0CC, lit by
; $996B, handler ObjectFlyingPiece, +$0A = $0F) flying out at ($28, -$2D,
; 0), (-$28, -$2D, 0), (0, -$32, $28) and (0, -$32, -$28); a spark is
; skipped without a free object.
; HOW: Four ObjectSpawn blocks, each from object X's position.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the last spark
; leaves it.
%at($09A0FA)
ObjectLanternBurst:
    rep.b #$20
    lda.w #$A0CC
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .lA10E
    jml.l .lA160
.lA10E:
    rep.b #$20
    lda.w #$996B
    sta.w !obj_5d,y
    sep.b #$20
    lda.w !obj_24,y
    ora.b #$08
    sta.w !obj_24,y
    lda.b #$00
    sta.w !obj_3a,y
    lda.b #$14
    sta.w !obj_5f,y
    jsl.l ObjectCopyPositionTo
    rep.b #$20
    lda.w #$8DDF
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,y
    rep.b #$20
    lda.w #$0028
    sta.w !obj_vel_x,y
    sep.b #$20
    rep.b #$20
    lda.w #$FFD3
    sta.w !obj_vel_y,y
    sep.b #$20
    rep.b #$20
    lda.w #$0000
    sta.w !obj_vel_z,y
    sep.b #$20
    lda.b #$0F
    sta.w !obj_09+1,y
.lA160:
    rep.b #$20
    lda.w #$A0CC
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .lA174
    jml.l .lA1C6
.lA174:
    rep.b #$20
    lda.w #$996B
    sta.w !obj_5d,y
    sep.b #$20
    lda.w !obj_24,y
    ora.b #$08
    sta.w !obj_24,y
    lda.b #$00
    sta.w !obj_3a,y
    lda.b #$03
    sta.w !obj_5f,y
    jsl.l ObjectCopyPositionTo
    rep.b #$20
    lda.w #$8DDF
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,y
    rep.b #$20
    lda.w #$FFD8
    sta.w !obj_vel_x,y
    sep.b #$20
    rep.b #$20
    lda.w #$FFD3
    sta.w !obj_vel_y,y
    sep.b #$20
    rep.b #$20
    lda.w #$0000
    sta.w !obj_vel_z,y
    sep.b #$20
    lda.b #$0F
    sta.w !obj_09+1,y
.lA1C6:
    rep.b #$20
    lda.w #$A0CC
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .lA1DA
    jml.l .lA22C
.lA1DA:
    rep.b #$20
    lda.w #$996B
    sta.w !obj_5d,y
    sep.b #$20
    lda.w !obj_24,y
    ora.b #$08
    sta.w !obj_24,y
    lda.b #$00
    sta.w !obj_3a,y
    lda.b #$0A
    sta.w !obj_5f,y
    jsl.l ObjectCopyPositionTo
    rep.b #$20
    lda.w #$8DDF
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,y
    rep.b #$20
    lda.w #$0000
    sta.w !obj_vel_x,y
    sep.b #$20
    rep.b #$20
    lda.w #$FFCE
    sta.w !obj_vel_y,y
    sep.b #$20
    rep.b #$20
    lda.w #$0028
    sta.w !obj_vel_z,y
    sep.b #$20
    lda.b #$0F
    sta.w !obj_09+1,y
.lA22C:
    rep.b #$20
    lda.w #$A0CC
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .lA240
    jml.l .lA292
.lA240:
    rep.b #$20
    lda.w #$996B
    sta.w !obj_5d,y
    sep.b #$20
    lda.w !obj_24,y
    ora.b #$08
    sta.w !obj_24,y
    lda.b #$00
    sta.w !obj_3a,y
    lda.b #$01
    sta.w !obj_5f,y
    jsl.l ObjectCopyPositionTo
    rep.b #$20
    lda.w #$8DDF
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,y
    rep.b #$20
    lda.w #$0000
    sta.w !obj_vel_x,y
    sep.b #$20
    rep.b #$20
    lda.w #$FFCE
    sta.w !obj_vel_y,y
    sep.b #$20
    rep.b #$20
    lda.w #$FFD8
    sta.w !obj_vel_z,y
    sep.b #$20
    lda.b #$0F
    sta.w !obj_09+1,y
.lA292:
    rts

; @routine 09:A293 ObjectChimeGate
; WHAT IT DOES: Sets up a chime gate: restart $09:A2B1
; (ObjectChimeGateRecord), frame $09:A2D8 (ObjectChimeGateStep); its record
; and frame.
; HOW: Word and byte stores. Reached only by unit_gates_09.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
; leaves it.
%at($09A293)
ObjectChimeGate:
    rep.b #$20
    lda.w #$A2B1
    sta.w !obj_1d,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1f,x
    rep.b #$20
    lda.w #$A2D8
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,x

; @routine 09:A2B1 ObjectChimeGateRecord
; WHAT IT DOES: The chime gate's restart: its record ($42D, flag bit 4, at
; ($F00, $F00, $F00)), then its frame.
; HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition. Reached only
; by unit_gates_09.
; Entry: as ObjectChimeGate. Exit: as ObjectChimeGate.
%at($09A2B1)
ObjectChimeGateRecord:
    jsl.l GsuRecordNew42D
    jsl.l GsuRecordFlag10
    rep.b #$20
    lda.w #$0F00
    sta.b !dp_02
    sep.b #$20
    rep.b #$20
    lda.w #$0F00
    sta.b !dp_08
    sep.b #$20
    rep.b #$20
    lda.w #$0F00
    sta.b !dp_94
    sep.b #$20
    jsl.l GsuRecordPosition

; @routine 09:A2D8 ObjectChimeGateStep
; WHAT IT DOES: The chime gate's frame: when a car enters it (contact bit
; 0) the first time, sound $13 on port 1; when it leaves, sound $0E; +$25
; bit 0 marks inside, bit 1 outside.
; HOW: ObjectSyncFromRecord, RecordContactSummary, SoundPort1QueuePush.
; Reached only by unit_gates_09.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
; it.
%at($09A2D8)
ObjectChimeGateStep:
    jsl.l ObjectSyncFromRecord
    and.b #$01
    beq .lA30A
    jsl.l RecordContactSummary
    and.b #$01
    beq .lA30A
    lda.w !obj_25,x
    and.b #$01
    beq .lA2F3
    jml.l .lA309
.lA2F3:
    lda.w !obj_25,x
    ora.b #$01
    sta.w !obj_25,x
    lda.w !obj_25,x
    and.b #$FD
    sta.w !obj_25,x
    lda.b #$13
    jsl.l SoundPort1QueuePush
.lA309:
    rtl
.lA30A:
    lda.w !obj_25,x
    and.b #$02
    beq .lA315
    jml.l .lA32B
.lA315:
    lda.w !obj_25,x
    ora.b #$02
    sta.w !obj_25,x
    lda.w !obj_25,x
    and.b #$FE
    sta.w !obj_25,x
    lda.b #$0E
    jsl.l SoundPort1QueuePush
.lA32B:
    rtl

; @routine 09:A32C ObjectBrightGate
; WHAT IT DOES: Sets up a bright gate: restart $09:A34A
; (ObjectBrightGateRecord), frame $09:A371 (ObjectBrightGateStep); its
; record and frame.
; HOW: Word and byte stores. Reached only by unit_gates_09.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
; leaves it.
%at($09A32C)
ObjectBrightGate:
    rep.b #$20
    lda.w #$A34A
    sta.w !obj_1d,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1f,x
    rep.b #$20
    lda.w #$A371
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,x

; @routine 09:A34A ObjectBrightGateRecord
; WHAT IT DOES: The bright gate's restart: its record ($42D, flag bit 4,
; at ($780, $780, $780)), then its frame.
; HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition. Reached only
; by unit_gates_09.
; Entry: as ObjectBrightGate. Exit: as ObjectBrightGate.
%at($09A34A)
ObjectBrightGateRecord:
    jsl.l GsuRecordNew42D
    jsl.l GsuRecordFlag10
    rep.b #$20
    lda.w #$0780
    sta.b !dp_02
    sep.b #$20
    rep.b #$20
    lda.w #$0780
    sta.b !dp_08
    sep.b #$20
    rep.b #$20
    lda.w #$0780
    sta.b !dp_94
    sep.b #$20
    jsl.l GsuRecordPosition

; @routine 09:A371 ObjectBrightGateStep
; WHAT IT DOES: The bright gate's frame: a car's hit, the first time (+$25
; bit 0), brightens the screen (flash step $19C5 = $20, fade by $200 up to
; $2000, $0DA5 bits 0 and 5); no hit clears the bit.
; HOW: ObjectSyncFromRecord, RecordContactSummary; the flash word by a long
; store. Reached only by unit_gates_09.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
; it.
%at($09A371)
ObjectBrightGateStep:
    jsl.l ObjectSyncFromRecord
    and.b #$01
    beq .lA3B8
    jsl.l RecordContactSummary
    and.b #$01
    beq .lA3B8
    lda.w !obj_25,x
    and.b #$01
    beq .lA38C
    jml.l .lA3B7
.lA38C:
    lda.w !obj_25,x
    ora.b #$01
    sta.w !obj_25,x
    rep.b #$20
    lda.w #$0020
    sta.l !wram_19C5
    rep.b #$20
    lda.w #$0200
    sta.w !wram_1A00
    lda.w #$2000
    sta.w !wram_1A02
    sep.b #$20
    lda.w !slide_flags
    ora.b #$01
    ora.b #$20
    sta.w !slide_flags
.lA3B7:
    rtl
.lA3B8:
    sep.b #$20
    lda.w !obj_25,x
    and.b #$FE
    sta.w !obj_25,x
    rtl

; @routine 09:A3C3 ObjectDarkGate
; WHAT IT DOES: Sets up a dark gate: restart $09:A3E1
; (ObjectDarkGateRecord), frame $09:A408 (ObjectDarkGateStep); its record
; and frame.
; HOW: Word and byte stores.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
; leaves it.
%at($09A3C3)
ObjectDarkGate:
    rep.b #$20
    lda.w #$A3E1
    sta.w !obj_1d,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1f,x
    rep.b #$20
    lda.w #$A408
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,x

; @routine 09:A3E1 ObjectDarkGateRecord
; WHAT IT DOES: The dark gate's restart: its record ($42D, flag bit 4, at
; ($780, $780, $780)), then its frame.
; HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition.
; Entry: as ObjectDarkGate. Exit: as ObjectDarkGate.
%at($09A3E1)
ObjectDarkGateRecord:
    jsl.l GsuRecordNew42D
    jsl.l GsuRecordFlag10
    rep.b #$20
    lda.w #$0780
    sta.b !dp_02
    sep.b #$20
    rep.b #$20
    lda.w #$0780
    sta.b !dp_08
    sep.b #$20
    rep.b #$20
    lda.w #$0780
    sta.b !dp_94
    sep.b #$20
    jsl.l GsuRecordPosition

; @routine 09:A408 ObjectDarkGateStep
; WHAT IT DOES: The dark gate's frame: a car's hit, the first time (+$25
; bit 0), darkens the screen (flash step $19C5 = -$20, fade by -$200 down
; to $FE00, $0DA5 bits 0 and 5); no hit clears the bit.
; HOW: ObjectSyncFromRecord, RecordContactSummary; the flash word by a long
; store.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
; it.
%at($09A408)
ObjectDarkGateStep:
    jsl.l ObjectSyncFromRecord
    and.b #$01
    beq .lA44D
    jsl.l RecordContactSummary
    and.b #$01
    beq .lA44D
    lda.w !obj_25,x
    and.b #$01
    beq .lA423
    jml.l .lA44C
.lA423:
    lda.w !obj_25,x
    ora.b #$01
    sta.w !obj_25,x
    rep.b #$20
    lda.w #$FFE0
    sta.l !wram_19C5
    lda.w #$FE00
    sta.w !wram_1A00
    lda.w #$FE00
    sta.w !wram_1A02
    sep.b #$20
    lda.w !slide_flags
    ora.b #$01
    ora.b #$20
    sta.w !slide_flags
.lA44C:
    rtl
.lA44D:
    sep.b #$20
    lda.w !obj_25,x
    and.b #$FE
    sta.w !obj_25,x
    rtl

; @routine 09:A458 ObjectCarFlagOffGate
; WHAT IT DOES: Sets up a gate that takes bit 1 of a car's record +$8A
; away: restart $09:A476 (ObjectCarFlagOffGateRecord), frame $09:A49D
; (ObjectCarFlagOffGateStep); its record and frame.
; HOW: Word and byte stores.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
; leaves it.
%at($09A458)
ObjectCarFlagOffGate:
    rep.b #$20
    lda.w #$A476
    sta.w !obj_1d,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1f,x
    rep.b #$20
    lda.w #$A49D
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,x

; @routine 09:A476 ObjectCarFlagOffGateRecord
; WHAT IT DOES: The gate's restart: its record ($42D, flag bit 4, at
; ($780, $780, $780)), then its frame.
; HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition.
; Entry: as ObjectCarFlagOffGate. Exit: as ObjectCarFlagOffGate.
%at($09A476)
ObjectCarFlagOffGateRecord:
    jsl.l GsuRecordNew42D
    jsl.l GsuRecordFlag10
    rep.b #$20
    lda.w #$0780
    sta.b !dp_02
    sep.b #$20
    rep.b #$20
    lda.w #$0780
    sta.b !dp_08
    sep.b #$20
    rep.b #$20
    lda.w #$0780
    sta.b !dp_94
    sep.b #$20
    jsl.l GsuRecordPosition

; @routine 09:A49D ObjectCarFlagOffGateStep
; WHAT IT DOES: The gate's frame: the first car contact (contact flags bit
; 0; +$25 bit 0 marks it) clears bit 1 of that car's record +$8A unless
; $7F:3460 is set, and plays sound $11 on port 1; no contact clears the
; mark.
; HOW: ObjectSyncFromRecord, then RecordNextContact over the contacts;
; 08:B743 with A = 2 (16-bit) and Y = the touched record.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
; it.
%at($09A49D)
ObjectCarFlagOffGateStep:
    jsl.l ObjectSyncFromRecord
    and.b #$01
    beq .lA4DE
.lA4A5:
    jsl.l RecordNextContact
    bcc .lA4DE
    and.b #$01
    beq .lA4A5
    lda.w !obj_25,x
    and.b #$01
    beq .lA4BA
    jml.l .lA4DD
.lA4BA:
    lda.w !obj_25,x
    ora.b #$01
    sta.w !obj_25,x
    sep.b #$20
    lda.l !wram_7F3460
    bne .lA4D3
    rep.b #$20
    lda.w #$0002
    jsl.l GsuRecordFlags8AClear
.lA4D3:
    rep.b #$20
    sep.b #$20
    lda.b #$11
    jsl.l SoundPort1QueuePush
.lA4DD:
    rtl
.lA4DE:
    lda.w !obj_25,x
    and.b #$FE
    sta.w !obj_25,x
    rtl

; @routine 09:A4E7 ObjectCarFlagOnGate
; WHAT IT DOES: Sets up a gate that gives a car's record +$8A bit 1:
; restart $09:A505 (ObjectCarFlagOnGateRecord), frame $09:A52C
; (ObjectCarFlagOnGateStep); its record and frame.
; HOW: Word and byte stores.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
; leaves it.
%at($09A4E7)
ObjectCarFlagOnGate:
    rep.b #$20
    lda.w #$A505
    sta.w !obj_1d,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1f,x
    rep.b #$20
    lda.w #$A52C
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,x

; @routine 09:A505 ObjectCarFlagOnGateRecord
; WHAT IT DOES: The gate's restart: its record ($42D, flag bit 4, at
; ($780, $780, $780)), then its frame.
; HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition.
; Entry: as ObjectCarFlagOnGate. Exit: as ObjectCarFlagOnGate.
%at($09A505)
ObjectCarFlagOnGateRecord:
    jsl.l GsuRecordNew42D
    jsl.l GsuRecordFlag10
    rep.b #$20
    lda.w #$0780
    sta.b !dp_02
    sep.b #$20
    rep.b #$20
    lda.w #$0780
    sta.b !dp_08
    sep.b #$20
    rep.b #$20
    lda.w #$0780
    sta.b !dp_94
    sep.b #$20
    jsl.l GsuRecordPosition

; @routine 09:A52C ObjectCarFlagOnGateStep
; WHAT IT DOES: The gate's frame: the first car contact (contact flags bit
; 0; +$25 bit 0 marks it) sets bit 1 of that car's record +$8A unless
; $7F:3460 is set, and plays sound $10 on port 1; no contact clears the
; mark.
; HOW: ObjectSyncFromRecord, then RecordNextContact over the contacts;
; 08:B72E with A = 2 (16-bit) and Y = the touched record.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
; it.
%at($09A52C)
ObjectCarFlagOnGateStep:
    jsl.l ObjectSyncFromRecord
    and.b #$01
    beq .lA56B
.lA534:
    jsl.l RecordNextContact
    bcc .lA56B
    and.b #$01
    beq .lA534
    lda.w !obj_25,x
    and.b #$01
    beq .lA549
    jml.l .lA56A
.lA549:
    lda.w !obj_25,x
    ora.b #$01
    sta.w !obj_25,x
    sep.b #$20
    lda.l !wram_7F3460
    bne .lA562
    rep.b #$20
    lda.w #$0002
    jsl.l GsuRecordFlags8ASet
.lA562:
    sep.b #$20
    lda.b #$10
    jsl.l SoundPort1QueuePush
.lA56A:
    rtl
.lA56B:
    lda.w !obj_25,x
    and.b #$FE
    sta.w !obj_25,x
    rtl

; @routine 09:A574 ObjectSlideGate
; WHAT IT DOES: Sets up the lap 1 slide gate: restart $09:A592
; (ObjectSlideGateRecord), frame $09:A5B9 (ObjectSlideGateStep); its record
; and frame.
; HOW: Word and byte stores.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
; leaves it.
%at($09A574)
ObjectSlideGate:
    rep.b #$20
    lda.w #$A592
    sta.w !obj_1d,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1f,x
    rep.b #$20
    lda.w #$A5B9
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,x

; @routine 09:A592 ObjectSlideGateRecord
; WHAT IT DOES: The slide gate's restart: its record ($42D, flag bit 4, at
; ($780, $780, $780)), then its frame.
; HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition.
; Entry: as ObjectSlideGate. Exit: as ObjectSlideGate.
%at($09A592)
ObjectSlideGateRecord:
    jsl.l GsuRecordNew42D
    jsl.l GsuRecordFlag10
    rep.b #$20
    lda.w #$0780
    sta.b !dp_02
    sep.b #$20
    rep.b #$20
    lda.w #$0780
    sta.b !dp_08
    sep.b #$20
    rep.b #$20
    lda.w #$0780
    sta.b !dp_94
    sep.b #$20
    jsl.l GsuRecordPosition

; @routine 09:A5B9 ObjectSlideGateStep
; WHAT IT DOES: The slide gate's frame: on lap 1 ($0DEA) the first car
; contact (+$25 bit 0 marks it) clears bit 1 of the car's record +$8A,
; starts the slide down ($0DA5 bits 5 and 7; step $40 to target -$40,
; direction $8000) and a fade (step $20 up to $2000).
; HOW: ObjectSyncFromRecord, RecordNextContact; 08:B743 with A = 2;
; $0DA5 by long accesses.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
; it.
%at($09A5B9)
ObjectSlideGateStep:
    jsl.l ObjectSyncFromRecord
    and.b #$01
    beq .lA620
.lA5C1:
    jsl.l RecordNextContact
    bcc .lA620
    and.b #$01
    beq .lA5C1
    sep.b #$20
    lda.w !wram_0DEA
    cmp.b #$01
    bne .lA620
    lda.w !obj_25,x
    and.b #$01
    beq .lA5DF
    jml.l .lA620
.lA5DF:
    lda.w !obj_25,x
    ora.b #$01
    sta.w !obj_25,x
    sep.b #$20
    lda.l !slide_flags
    ora.b #$20
    ora.b #$80
    sta.l !slide_flags
    rep.b #$20
    lda.w #$0002
    jsl.l GsuRecordFlags8AClear
    rep.b #$20
    lda.w #$0020
    sta.w !wram_1A00
    lda.w #$2000
    sta.w !wram_1A02
    lda.w #$0040
    sta.w !slide_step
    lda.w #$FFC0
    sta.w !slide_target
    lda.w #$8000
    sta.w !slide_direction
    sep.b #$20
.lA620:
    rtl

; @routine 09:A621 ObjectLapFlashGate
; WHAT IT DOES: Sets up a gate that flashes on laps 0 and 1 and fades on
; lap 2: restart $09:A63F (ObjectLapFlashGateRecord), frame $09:A666
; (ObjectLapFlashGateStep); its record and frame.
; HOW: Word and byte stores.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
; leaves it.
%at($09A621)
ObjectLapFlashGate:
    rep.b #$20
    lda.w #$A63F
    sta.w !obj_1d,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1f,x
    rep.b #$20
    lda.w #$A666
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,x

; @routine 09:A63F ObjectLapFlashGateRecord
; WHAT IT DOES: The gate's restart: its record ($42D, flag bit 4, at
; ($780, $780, $780)), then its frame.
; HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition.
; Entry: as ObjectLapFlashGate. Exit: as ObjectLapFlashGate.
%at($09A63F)
ObjectLapFlashGateRecord:
    jsl.l GsuRecordNew42D
    jsl.l GsuRecordFlag10
    rep.b #$20
    lda.w #$0780
    sta.b !dp_02
    sep.b #$20
    rep.b #$20
    lda.w #$0780
    sta.b !dp_08
    sep.b #$20
    rep.b #$20
    lda.w #$0780
    sta.b !dp_94
    sep.b #$20
    jsl.l GsuRecordPosition

; @routine 09:A666 ObjectLapFlashGateStep
; WHAT IT DOES: The gate's frame: a car's hit on lap 0 or 1 ($0DEA), the
; first time (+$25 bit 0), sweeps the screen on even clock ticks
; (GateFlashOnClock); on lap 2, the first time (+$25 bit 1), a fade
; (GateFadeUp) and the gate is released.
; HOW: ObjectSyncFromRecord, RecordContactSummary; the lap compared with
; 0, 1 and 2.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
; it.
%at($09A666)
ObjectLapFlashGateStep:
    jsl.l ObjectSyncFromRecord
    and.b #$01
    beq .lA6BD
    jsl.l RecordContactSummary
    and.b #$01
    beq .lA6BD
    sep.b #$20
    lda.w !wram_0DEA
    cmp.b #$00
    beq .lA68A
    cmp.b #$01
    beq .lA68A
    cmp.b #$02
    beq .lA6A3
    jmp.w .lA6BD
.lA68A:
    lda.w !obj_25,x
    and.b #$01
    beq .lA695
    jml.l .lA6BD
.lA695:
    lda.w !obj_25,x
    ora.b #$01
    sta.w !obj_25,x
    jsr.w GateFlashOnClock
    jmp.w .lA6BD
.lA6A3:
    lda.w !obj_25,x
    and.b #$02
    beq .lA6AE
    jml.l .lA6BD
.lA6AE:
    lda.w !obj_25,x
    ora.b #$02
    sta.w !obj_25,x
    jsr.w GateFadeUp
    jsl.l ObjectRelease
.lA6BD:
    rtl

; @routine 09:A6BE GateFlashOnClock
; WHAT IT DOES: When bit 4 of the race clock's fraction $0E63 is clear,
; sweeps the screen ($0DA5 bit 4) with sound $36.
; HOW: Four LSRs and AND #1 (the carry is the fraction's bit 3); $0DA5 by
; long accesses. Reached only by unit_gates_09.
; Entry: X 16-bit. Exit: M 8-bit; carry = bit 3 of $0E63; bit 4 set: A
; low byte = 1; else as SoundPort0QueuePush leaves it.
%at($09A6BE)
GateFlashOnClock:
    sep.b #$20
    lda.w !race_time_fraction
    lsr a
    lsr a
    lsr a
    lsr a
    and.b #$01
    bne .lA6DB
    lda.l !slide_flags
    ora.b #$10
    sta.l !slide_flags
    lda.b #$36
    jsl.l SoundPort0QueuePush
.lA6DB:
    rts

; @routine 09:A6DC GateFadeUp
; WHAT IT DOES: Starts a fade ($0DA5 bit 5) by $80 up to $2000.
; HOW: $0DA5 by long accesses; the words through the data bank.
; Reached only by unit_gates_09.
; Entry: X 16-bit. Exit: M 8-bit; A = $2000; N, Z from it.
%at($09A6DC)
GateFadeUp:
    sep.b #$20
    lda.l !slide_flags
    ora.b #$20
    sta.l !slide_flags
    rep.b #$20
    lda.w #$0080
    sta.w !wram_1A00
    lda.w #$2000
    sta.w !wram_1A02
    sep.b #$20
    rts

; @routine 09:A6F9 ObjectSweepGate
; WHAT IT DOES: Sets up the lap 0 sweep gate: restart $09:A717
; (ObjectSweepGateRecord), frame $09:A73E (ObjectSweepGateStep); its record
; and frame.
; HOW: Word and byte stores. Reached only by unit_gates_09.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
; leaves it.
%at($09A6F9)
ObjectSweepGate:
    rep.b #$20
    lda.w #$A717
    sta.w !obj_1d,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1f,x
    rep.b #$20
    lda.w #$A73E
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,x

; @routine 09:A717 ObjectSweepGateRecord
; WHAT IT DOES: The sweep gate's restart: its record ($42D, flag bit 4, at
; ($780, $780, $780)), then its frame.
; HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition. Reached only
; by unit_gates_09.
; Entry: as ObjectSweepGate. Exit: as ObjectSweepGate.
%at($09A717)
ObjectSweepGateRecord:
    jsl.l GsuRecordNew42D
    jsl.l GsuRecordFlag10
    rep.b #$20
    lda.w #$0780
    sta.b !dp_02
    sep.b #$20
    rep.b #$20
    lda.w #$0780
    sta.b !dp_08
    sep.b #$20
    rep.b #$20
    lda.w #$0780
    sta.b !dp_94
    sep.b #$20
    jsl.l GsuRecordPosition

; @routine 09:A73E ObjectSweepGateStep
; WHAT IT DOES: The sweep gate's frame: a car's hit on lap 0, the first
; time (+$25 bit 0), sweeps the screen ($0DA5 bit 4) with sound $36 and
; releases the gate; no hit clears the bit.
; HOW: ObjectSyncFromRecord, RecordContactSummary; $0DA5 by long accesses.
; Reached only by unit_gates_09.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
; it.
%at($09A73E)
ObjectSweepGateStep:
    jsl.l ObjectSyncFromRecord
    and.b #$01
    beq .lA77F
    jsl.l RecordContactSummary
    and.b #$01
    beq .lA77F
    sep.b #$20
    lda.w !wram_0DEA
    cmp.b #$00
    bne .lA77E
    lda.w !obj_25,x
    and.b #$01
    beq .lA762
    jml.l .lA77E
.lA762:
    lda.w !obj_25,x
    ora.b #$01
    sta.w !obj_25,x
    lda.l !slide_flags
    ora.b #$10
    sta.l !slide_flags
    lda.b #$36
    jsl.l SoundPort0QueuePush
    jsl.l ObjectRelease
.lA77E:
    rtl
.lA77F:
    lda.w !obj_25,x
    and.b #$FE
    sta.w !obj_25,x
    rtl

; @routine 09:A788 ObjectSlideChimeGate
; WHAT IT DOES: Sets up the slide chime gate: restart $09:A7A6
; (ObjectSlideChimeGateRecord), frame $09:A7CD (ObjectSlideChimeGateStep);
; its record and frame.
; HOW: Word and byte stores.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
; leaves it.
%at($09A788)
ObjectSlideChimeGate:
    rep.b #$20
    lda.w #$A7A6
    sta.w !obj_1d,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1f,x
    rep.b #$20
    lda.w #$A7CD
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,x

; @routine 09:A7A6 ObjectSlideChimeGateRecord
; WHAT IT DOES: The gate's restart: its record ($42D, flag bit 4, at
; ($F00, $F00, $F00)), then its frame.
; HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition.
; Entry: as ObjectSlideChimeGate. Exit: as ObjectSlideChimeGate.
%at($09A7A6)
ObjectSlideChimeGateRecord:
    jsl.l GsuRecordNew42D
    jsl.l GsuRecordFlag10
    rep.b #$20
    lda.w #$0F00
    sta.b !dp_02
    sep.b #$20
    rep.b #$20
    lda.w #$0F00
    sta.b !dp_08
    sep.b #$20
    rep.b #$20
    lda.w #$0F00
    sta.b !dp_94
    sep.b #$20
    jsl.l GsuRecordPosition

; @routine 09:A7CD ObjectSlideChimeGateStep
; WHAT IT DOES: The gate's frame: a car entering (contact summary bit 0)
; the first time plays sound $13 on port 1 (+$25 bit 1 marks inside); on
; lap 1 the first entry (+$25 bit 0) also starts the slide down (step $80
; to -$80, $1A04 = $FF) and a fade ($20 up to $2000). Leaving plays sound
; $0E the first time (+$25 bit 2 marks outside).
; HOW: ObjectSyncFromRecord, RecordContactSummary (not tested on the
; sync); $0DA5 by long accesses.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
; it.
%at($09A7CD)
ObjectSlideChimeGateStep:
    jsl.l ObjectSyncFromRecord
    jsl.l RecordContactSummary
    and.b #$01
    beq .lA853
    lda.w !obj_25,x
    and.b #$02
    beq .lA7E4
    jml.l .lA7FA
.lA7E4:
    lda.w !obj_25,x
    ora.b #$02
    sta.w !obj_25,x
    lda.w !obj_25,x
    and.b #$FB
    sta.w !obj_25,x
    lda.b #$13
    jsl.l SoundPort1QueuePush
.lA7FA:
    sep.b #$20
    lda.w !wram_0DEA
    cmp.b #$01
    bne .lA852
    lda.w !obj_25,x
    and.b #$01
    beq .lA80E
    jml.l .lA852
.lA80E:
    lda.w !obj_25,x
    ora.b #$01
    sta.w !obj_25,x
    sep.b #$20
    lda.l !slide_flags
    ora.b #$A0
    sta.l !slide_flags
    rep.b #$20
    lda.w #$0080
    sta.w !slide_step
    lda.w #$FF80
    sta.w !slide_target
    lda.w #$8000
    sta.w !slide_direction
    sep.b #$20
    lda.b #$FF
    sta.w !wram_1A04
    lda.b #$00
    sta.w !wram_1A04+1
    rep.b #$20
    lda.w #$0020
    sta.w !wram_1A00
    lda.w #$2000
    sta.w !wram_1A02
    sep.b #$20
.lA852:
    rtl
.lA853:
    sep.b #$20
    lda.w !obj_25,x
    and.b #$04
    beq .lA860
    jml.l .lA86E
.lA860:
    lda.w !obj_25,x
    ora.b #$04
    sta.w !obj_25,x
    lda.b #$0E
    jsl.l SoundPort1QueuePush
.lA86E:
    lda.w !obj_25,x
    and.b #$FD
    sta.w !obj_25,x
    rtl

; @routine 09:A877 ObjectFadeGate0
; WHAT IT DOES: Sets up the lap 0 fade gate: restart $09:A895
; (ObjectFadeGate0Record), frame $09:A8BC (ObjectFadeGate0Step); its record
; and frame.
; HOW: Word and byte stores. Reached only by unit_gates_09.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
; leaves it.
%at($09A877)
ObjectFadeGate0:
    rep.b #$20
    lda.w #$A895
    sta.w !obj_1d,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1f,x
    rep.b #$20
    lda.w #$A8BC
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,x

; @routine 09:A895 ObjectFadeGate0Record
; WHAT IT DOES: The gate's restart: its record ($42D, flag bit 4, at
; ($780, $780, $780)), then its frame.
; HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition. Reached only
; by unit_gates_09.
; Entry: as ObjectFadeGate0. Exit: as ObjectFadeGate0.
%at($09A895)
ObjectFadeGate0Record:
    jsl.l GsuRecordNew42D
    jsl.l GsuRecordFlag10
    rep.b #$20
    lda.w #$0780
    sta.b !dp_02
    sep.b #$20
    rep.b #$20
    lda.w #$0780
    sta.b !dp_08
    sep.b #$20
    rep.b #$20
    lda.w #$0780
    sta.b !dp_94
    sep.b #$20
    jsl.l GsuRecordPosition

; @routine 09:A8BC ObjectFadeGate0Step
; WHAT IT DOES: The gate's frame: on lap 0 the first car contact (+$25
; bit 0 marks it) starts a slow fade ($0DA5 bit 5, step $0A up to $1FFE).
; HOW: ObjectSyncFromRecord, RecordNextContact; $0DA5 by long accesses.
; Reached only by unit_gates_09.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
; it.
%at($09A8BC)
ObjectFadeGate0Step:
    jsl.l ObjectSyncFromRecord
    and.b #$01
    beq .lA906
.lA8C4:
    jsl.l RecordNextContact
    bcc .lA906
    and.b #$01
    beq .lA8C4
    sep.b #$20
    lda.w !wram_0DEA
    cmp.b #$00
    bne .lA906
    lda.w !obj_25,x
    and.b #$01
    beq .lA8E2
    jml.l .lA906
.lA8E2:
    lda.w !obj_25,x
    ora.b #$01
    sta.w !obj_25,x
    sep.b #$20
    lda.l !slide_flags
    ora.b #$20
    sta.l !slide_flags
    rep.b #$20
    lda.w #$000A
    sta.w !wram_1A00
    lda.w #$1FFE
    sta.w !wram_1A02
    sep.b #$20
.lA906:
    rtl

; @routine 09:A907 ObjectFadeGate2
; WHAT IT DOES: Sets up the lap 2 fade gate: restart $09:A925
; (ObjectFadeGate2Record), frame $09:A94C (ObjectFadeGate2Step); its record
; and frame.
; HOW: Word and byte stores. Reached only by unit_gates_09.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
; leaves it.
%at($09A907)
ObjectFadeGate2:
    rep.b #$20
    lda.w #$A925
    sta.w !obj_1d,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1f,x
    rep.b #$20
    lda.w #$A94C
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,x

; @routine 09:A925 ObjectFadeGate2Record
; WHAT IT DOES: The gate's restart: its record ($42D, flag bit 4, at
; ($780, $780, $780)), then its frame.
; HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition. Reached only
; by unit_gates_09.
; Entry: as ObjectFadeGate2. Exit: as ObjectFadeGate2.
%at($09A925)
ObjectFadeGate2Record:
    jsl.l GsuRecordNew42D
    jsl.l GsuRecordFlag10
    rep.b #$20
    lda.w #$0780
    sta.b !dp_02
    sep.b #$20
    rep.b #$20
    lda.w #$0780
    sta.b !dp_08
    sep.b #$20
    rep.b #$20
    lda.w #$0780
    sta.b !dp_94
    sep.b #$20
    jsl.l GsuRecordPosition

; @routine 09:A94C ObjectFadeGate2Step
; WHAT IT DOES: The gate's frame: on lap 2 the first car contact (+$25
; bit 0 marks it) starts a fade ($0DA5 bit 5, step $20 up to $1F20).
; HOW: ObjectSyncFromRecord, RecordNextContact; $0DA5 by long accesses;
; its no-contact exit is the RTL of 09:A8BC's code (09:A906). Reached only
; by unit_gates_09.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
; it.
%at($09A94C)
ObjectFadeGate2Step:
    jsl.l ObjectSyncFromRecord
    and.b #$01
    beq ObjectFadeGate0Step_lA906
.lA954:
    jsl.l RecordNextContact
    bcc ObjectFadeGate0Step_lA906
    and.b #$01
    beq .lA954
    sep.b #$20
    lda.w !wram_0DEA
    cmp.b #$02
    bne .lA996
    lda.w !obj_25,x
    and.b #$01
    beq .lA972
    jml.l .lA996
.lA972:
    lda.w !obj_25,x
    ora.b #$01
    sta.w !obj_25,x
    sep.b #$20
    lda.l !slide_flags
    ora.b #$20
    sta.l !slide_flags
    rep.b #$20
    lda.w #$0020
    sta.w !wram_1A00
    lda.w #$1F20
    sta.w !wram_1A02
    sep.b #$20
.lA996:
    rtl

; @routine 09:A997 ObjectSlideZone
; WHAT IT DOES: Sets up a slide zone: restart $09:A9B5
; (ObjectSlideZoneRecord), frame $09:A9DC (ObjectSlideZoneStep); its record
; and frame.
; HOW: Word and byte stores. Reached only by unit_gates_09.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
; leaves it.
%at($09A997)
ObjectSlideZone:
    rep.b #$20
    lda.w #$A9B5
    sta.w !obj_1d,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1f,x
    rep.b #$20
    lda.w #$A9DC
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,x

; @routine 09:A9B5 ObjectSlideZoneRecord
; WHAT IT DOES: The zone's restart: its record ($42D, flag bit 4, at
; ($F00, $F00, $F00)), then its frame.
; HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition. Reached only
; by unit_gates_09.
; Entry: as ObjectSlideZone. Exit: as ObjectSlideZone.
%at($09A9B5)
ObjectSlideZoneRecord:
    jsl.l GsuRecordNew42D
    jsl.l GsuRecordFlag10
    rep.b #$20
    lda.w #$0F00
    sta.b !dp_02
    sep.b #$20
    rep.b #$20
    lda.w #$0F00
    sta.b !dp_08
    sep.b #$20
    rep.b #$20
    lda.w #$0F00
    sta.b !dp_94
    sep.b #$20
    jsl.l GsuRecordPosition

; @routine 09:A9DC ObjectSlideZoneStep
; WHAT IT DOES: The zone's frame: with a car's hit the slide runs up (step
; $100 to $1000, direction 0, done mark $7F:3470 = $7FFF, $0DA5 bit 7);
; without, back down (step $100 to -$100, direction $8000, $1A04 = $FF).
; HOW: ObjectSyncFromRecord, RecordContactSummary; word stores.
; Reached only by unit_gates_09.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
; it.
%at($09A9DC)
ObjectSlideZoneStep:
    jsl.l ObjectSyncFromRecord
    and.b #$01
    beq .lAA12
    jsl.l RecordContactSummary
    and.b #$01
    beq .lAA12
    rep.b #$20
    lda.w #$7FFF
    sta.l !slide_done
    lda.w #$0100
    sta.w !slide_step
    lda.w #$1000
    sta.w !slide_target
    lda.w #$0000
    sta.w !slide_direction
    sep.b #$20
    lda.w !slide_flags
    ora.b #$80
    sta.w !slide_flags
    rtl
.lAA12:
    rep.b #$20
    lda.w #$0100
    sta.w !slide_step
    lda.w #$FF00
    sta.w !slide_target
    lda.w #$8000
    sta.w !slide_direction
    sep.b #$20
    lda.b #$FF
    sta.w !wram_1A04
    lda.b #$00
    sta.w !wram_1A04+1
    rtl

; @routine 09:AA33 ObjectBonusMarker
; WHAT IT DOES: Sets up a bonus marker: restart $09:AA51
; (ObjectBonusMarkerRecord), frame $09:AA78 (ObjectBonusMarkerStep); its
; record and frame.
; HOW: Word and byte stores. Reached only by unit_gates_09.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
; leaves it.
%at($09AA33)
ObjectBonusMarker:
    rep.b #$20
    lda.w #$AA51
    sta.w !obj_1d,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1f,x
    rep.b #$20
    lda.w #$AA78
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,x

; @routine 09:AA51 ObjectBonusMarkerRecord
; WHAT IT DOES: The marker's restart: its record ($42D, flag bit 4, at
; ($780, $780, $780)), then its frame.
; HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition. Reached only
; by unit_gates_09.
; Entry: as ObjectBonusMarker. Exit: as ObjectBonusMarker.
%at($09AA51)
ObjectBonusMarkerRecord:
    jsl.l GsuRecordNew42D
    jsl.l GsuRecordFlag10
    rep.b #$20
    lda.w #$0780
    sta.b !dp_02
    sep.b #$20
    rep.b #$20
    lda.w #$0780
    sta.b !dp_08
    sep.b #$20
    rep.b #$20
    lda.w #$0780
    sta.b !dp_94
    sep.b #$20
    jsl.l GsuRecordPosition

; @routine 09:AA78 ObjectBonusMarkerStep
; WHAT IT DOES: The marker's frame: a car's contact on lap 0 counts the
; markers at $1A06 up, sets bit 0 of the car's record +$8A, sweeps the
; screen ($0DA5 bit 4) with sound $36 unless bits 4-5 of the clock
; fraction $0E63 are clear, and releases the marker.
; HOW: ObjectSyncFromRecord, RecordNextContact; 08:B72E with A low byte 1.
; Reached only by unit_gates_09.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
; it.
%at($09AA78)
ObjectBonusMarkerStep:
    jsl.l ObjectSyncFromRecord
    and.b #$01
    beq .lAABB
.lAA80:
    jsl.l RecordNextContact
    bcc .lAABB
    and.b #$01
    beq .lAA80
    sep.b #$20
    lda.w !wram_0DEA
    cmp.b #$00
    bne .lAABB
    inc.w !wram_1A06
    lda.b #$01
    jsl.l GsuRecordFlags8ASet
    sep.b #$20
    lda.w !race_time_fraction
    lsr a
    lsr a
    lsr a
    lsr a
    and.b #$03
    beq .lAAB7
    lda.w !slide_flags
    ora.b #$10
    sta.w !slide_flags
    lda.b #$36
    jsl.l SoundPort0QueuePush
.lAAB7:
    jsl.l ObjectRelease
.lAABB:
    rtl

; @routine 09:AABC ObjectBonusGate
; WHAT IT DOES: Sets up the bonus gate: restart $09:AADA
; (ObjectBonusGateRecord), frame $09:AB01 (ObjectBonusGateStep); its record
; and frame.
; HOW: Word and byte stores. Reached only by unit_gates_09.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
; leaves it.
%at($09AABC)
ObjectBonusGate:
    rep.b #$20
    lda.w #$AADA
    sta.w !obj_1d,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1f,x
    rep.b #$20
    lda.w #$AB01
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,x

; @routine 09:AADA ObjectBonusGateRecord
; WHAT IT DOES: The gate's restart: its record ($42D, flag bit 4, at
; ($780, $780, $780)), then its frame.
; HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition. Reached only
; by unit_gates_09.
; Entry: as ObjectBonusGate. Exit: as ObjectBonusGate.
%at($09AADA)
ObjectBonusGateRecord:
    jsl.l GsuRecordNew42D
    jsl.l GsuRecordFlag10
    rep.b #$20
    lda.w #$0780
    sta.b !dp_02
    sep.b #$20
    rep.b #$20
    lda.w #$0780
    sta.b !dp_08
    sep.b #$20
    rep.b #$20
    lda.w #$0780
    sta.b !dp_94
    sep.b #$20
    jsl.l GsuRecordPosition

; @routine 09:AB01 ObjectBonusGateStep
; WHAT IT DOES: The gate's frame: a car's contact sets bit 2 of $0DA5
; (clearing bit 3); the first contact (+$25 bit 0 marks it) plays sound
; $10 on port 1 and, on lap 0 with markers taken ($1A06), sets bit 0 of
; the car's record +$8A. No contact clears the mark.
; HOW: ObjectSyncFromRecord, RecordNextContact; 08:B72E with A low byte 1.
; Reached only by unit_gates_09.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
; it.
%at($09AB01)
ObjectBonusGateStep:
    jsl.l ObjectSyncFromRecord
    and.b #$01
    beq .lAB4D
.lAB09:
    jsl.l RecordNextContact
    bcc .lAB4D
    and.b #$01
    beq .lAB09
    sep.b #$20
    lda.w !slide_flags
    and.b #$F7
    ora.b #$04
    sta.w !slide_flags
    lda.w !obj_25,x
    and.b #$01
    beq .lAB2A
    jml.l .lAB4C
.lAB2A:
    lda.w !obj_25,x
    ora.b #$01
    sta.w !obj_25,x
    lda.b #$10
    jsl.l SoundPort1QueuePush
    sep.b #$20
    lda.w !wram_0DEA
    cmp.b #$00
    bne .lAB4C
    lda.w !wram_1A06
    beq .lAB4C
    lda.b #$01
    jsl.l GsuRecordFlags8ASet
.lAB4C:
    rtl
.lAB4D:
    lda.w !obj_25,x
    and.b #$FE
    sta.w !obj_25,x
    rtl

; @routine 09:AB56 ObjectBonusEndGate
; WHAT IT DOES: Sets up the bonus end gate: restart $09:AB74
; (ObjectBonusEndGateRecord), frame $09:AB9B (ObjectBonusEndGateStep); its
; record and frame.
; HOW: Word and byte stores. Reached only by unit_gates_09.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
; leaves it.
%at($09AB56)
ObjectBonusEndGate:
    rep.b #$20
    lda.w #$AB74
    sta.w !obj_1d,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1f,x
    rep.b #$20
    lda.w #$AB9B
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,x

; @routine 09:AB74 ObjectBonusEndGateRecord
; WHAT IT DOES: The gate's restart: its record ($42D, flag bit 4, at
; ($780, $780, $780)), then its frame.
; HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition. Reached only
; by unit_gates_09.
; Entry: as ObjectBonusEndGate. Exit: as ObjectBonusEndGate.
%at($09AB74)
ObjectBonusEndGateRecord:
    jsl.l GsuRecordNew42D
    jsl.l GsuRecordFlag10
    rep.b #$20
    lda.w #$0780
    sta.b !dp_02
    sep.b #$20
    rep.b #$20
    lda.w #$0780
    sta.b !dp_08
    sep.b #$20
    rep.b #$20
    lda.w #$0780
    sta.b !dp_94
    sep.b #$20
    jsl.l GsuRecordPosition

; @routine 09:AB9B ObjectBonusEndGateStep
; WHAT IT DOES: The gate's frame: a car's contact sets bits 1 and 3 of
; $0DA5; the first contact (+$25 bit 0 marks it) plays sound $0F on port 1
; and, unless on lap 2, with markers taken ($1A06), clears bit 0 of the
; car's record +$8A. No contact clears the mark.
; HOW: ObjectSyncFromRecord, RecordNextContact; 08:B743 with A = 1.
; Reached only by unit_gates_09.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
; it.
%at($09AB9B)
ObjectBonusEndGateStep:
    jsl.l ObjectSyncFromRecord
    and.b #$01
    beq .lABEA
.lABA3:
    jsl.l RecordNextContact
    bcc .lABEA
    and.b #$01
    beq .lABA3
    sep.b #$20
    lda.w !slide_flags
    ora.b #$0A
    sta.w !slide_flags
    lda.w !obj_25,x
    and.b #$01
    beq .lABC2
    jml.l .lABE9
.lABC2:
    lda.w !obj_25,x
    ora.b #$01
    sta.w !obj_25,x
    lda.b #$0F
    jsl.l SoundPort1QueuePush
    sep.b #$20
    lda.w !wram_0DEA
    cmp.b #$02
    beq .lABE9
    lda.w !wram_1A06
    beq .lABE9
    rep.b #$20
    lda.w #$0001
    jsl.l GsuRecordFlags8AClear
    sep.b #$20
.lABE9:
    rtl
.lABEA:
    lda.w !obj_25,x
    and.b #$FE
    sta.w !obj_25,x
    rtl

; @routine 09:ABF3 ObjectPopMarker
; WHAT IT DOES: In a one-player race (HUD mode $0D2B = 0) puts a pop marker
; (kind $B7FC) at object X's position $1E0 higher with its scale +$15,
; handler $09:AD50 (ObjectPopMarkerStep), and clears X's handler; with a
; HUD mode X is released.
; HOW: ObjectSpawn, ObjectCopyPositionTo; starts with a NOP, as the course
; data's pointers expect.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: M 8-bit; released:
; as ObjectRelease leaves it; else A low byte = 0, Z set.
%at($09ABF3)
ObjectPopMarker:
    nop
    rep.b #$20
    lda.w !hud_mode
    beq .lAC02
    sep.b #$20
    jsl.l ObjectRelease
    rtl
.lAC02:
    sep.b #$20
    rep.b #$20
    lda.w #$B7FC
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .lAC18
    jml.l .lAC3F
.lAC18:
    jsl.l ObjectCopyPositionTo
    lda.w !obj_15,x
    sta.w !obj_15,y
    rep.b #$20
    lda.w !obj_pos_y,y
    clc
    adc.w #$FE20
    sta.w !obj_pos_y,y
    sep.b #$20
    rep.b #$20
    lda.w #$AD50
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,y
.lAC3F:
    rep.b #$20
    lda.w #$0000
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$00
    sta.w !obj_1c,x
    rtl

; @routine 09:AC4F ObjectTimeGate
; WHAT IT DOES: Sets up a time gate in a one-player race: restart $09:AC83
; (ObjectTimeGateRecord), frame $09:AD07 (ObjectTimeGateStep), +$48 = 4;
; with a HUD mode ($0D2B) it is released.
; HOW: Word and byte stores.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: M 8-bit; A low
; byte = 4, or as ObjectRelease leaves it.
%at($09AC4F)
ObjectTimeGate:
    rep.b #$20
    lda.w !hud_mode
    beq .lAC5D
    sep.b #$20
    jsl.l ObjectRelease
    rtl
.lAC5D:
    sep.b #$20
    rep.b #$20
    lda.w #$AC83
    sta.w !obj_1d,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1f,x
    rep.b #$20
    lda.w #$AD07
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,x
    lda.b #$04
    sta.w !obj_48,x
    rtl

; @routine 09:AC83 ObjectTimeGateRecord
; WHAT IT DOES: The time gate's restart: its record ($42D, flag bit 4, at
; ($780, $780, $780)).
; HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as
; GsuRecordPosition leaves it, M 8-bit.
%at($09AC83)
ObjectTimeGateRecord:
    jsl.l GsuRecordNew42D
    jsl.l GsuRecordFlag10
    rep.b #$20
    lda.w #$0780
    sta.b !dp_02
    sep.b #$20
    rep.b #$20
    lda.w #$0780
    sta.b !dp_08
    sep.b #$20
    rep.b #$20
    lda.w #$0780
    sta.b !dp_94
    sep.b #$20
    jsl.l GsuRecordPosition
    rtl

; @routine 09:ACAB ObjectTimeGateSmall
; WHAT IT DOES: Sets up a small time gate in a one-player race: restart
; $09:ACDF (ObjectTimeGateSmallRecord), frame $09:AD07
; (ObjectTimeGateStep), +$48 = 4; with a HUD mode ($0D2B) it is released.
; HOW: Word and byte stores. Reached only by unit_gates_09.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: M 8-bit; A low
; byte = 4, or as ObjectRelease leaves it.
%at($09ACAB)
ObjectTimeGateSmall:
    rep.b #$20
    lda.w !hud_mode
    beq .lACB9
    sep.b #$20
    jsl.l ObjectRelease
    rtl
.lACB9:
    sep.b #$20
    rep.b #$20
    lda.w #$ACDF
    sta.w !obj_1d,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1f,x
    rep.b #$20
    lda.w #$AD07
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,x
    lda.b #$04
    sta.w !obj_48,x
    rtl

; @routine 09:ACDF ObjectTimeGateSmallRecord
; WHAT IT DOES: The small time gate's restart: its record ($42D, flag bit
; 4, at ($3C0, $3C0, $3C0)).
; HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition. Reached only
; by unit_gates_09.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as
; GsuRecordPosition leaves it, M 8-bit.
%at($09ACDF)
ObjectTimeGateSmallRecord:
    jsl.l GsuRecordNew42D
    jsl.l GsuRecordFlag10
    rep.b #$20
    lda.w #$03C0
    sta.b !dp_02
    sep.b #$20
    rep.b #$20
    lda.w #$03C0
    sta.b !dp_08
    sep.b #$20
    rep.b #$20
    lda.w #$03C0
    sta.b !dp_94
    sep.b #$20
    jsl.l GsuRecordPosition
    rtl

; @routine 09:AD07 ObjectTimeGateStep
; WHAT IT DOES: The time gates' frame: a car's hit on a lap ($0DEA) other
; than the one it last counted (+$48) counts this one, plays sound $41 and
; adds the bonus $19E6 to the race time $0E70-$0E71 (BCD).
; HOW: ObjectSyncFromRecord, RecordContactSummary; the laps compared
; through $1F58; SED, ADC, CLD.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
; it; D flag clear.
%at($09AD07)
ObjectTimeGateStep:
    jsl.l ObjectSyncFromRecord
    and.b #$01
    beq .lAD4F
    jsl.l RecordContactSummary
    and.b #$01
    beq .lAD4F
    lda.w !obj_48,x
    sta.w !wram_1F58
    sep.b #$20
    lda.w !wram_0DEA
    cmp.w !wram_1F58
    beq .lAD4F
    sep.b #$20
    lda.w !wram_0DEA
    sta.w !wram_1F58
    lda.w !wram_1F58
    sta.w !obj_48,x
    lda.b #$41
    jsl.l SoundPort0QueuePush
    sed
    lda.w !race_time_bcd
    clc
    adc.w !wram_19E6
    sta.w !race_time_bcd
    lda.w !race_time_bcd+1
    adc.b #$00
    sta.w !race_time_bcd+1
    cld
.lAD4F:
    rtl

; @routine 09:AD50 ObjectPopMarkerStep
; WHAT IT DOES: The pop marker's frame: its size +$14 shrinks by 8.
; HOW: ADC #$F8; ObjectRebucket.
; Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectRebucket leaves it.
%at($09AD50)
ObjectPopMarkerStep:
    lda.w !obj_14,x
    clc
    adc.b #$F8
    sta.w !obj_14,x
    jsl.l ObjectRebucket
    rtl

; @routine 09:AD5E ObjectLapBallUp
; WHAT IT DOES: Sets up an up ball: restart $09:AD81 (ObjectLapBallUpRecord),
; frame $09:ADB6 (ObjectLapBallUpStep), +$0A = 4 (no lap lit); its record
; and frame.
; HOW: Word and byte stores.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
; leaves it.
%at($09AD5E)
ObjectLapBallUp:
    rep.b #$20
    lda.w #$AD81
    sta.w !obj_1d,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1f,x
    rep.b #$20
    lda.w #$ADB6
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,x
    lda.b #$04
    sta.w !obj_09+1,x

; @routine 09:AD81 ObjectLapBallUpRecord
; WHAT IT DOES: The up ball's restart: its record ($42D, flag bit 4, +$2A
; high $14), its model lit ($A040) when +$0A is the current lap, else
; $B7A8; then its frame.
; HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordSet2AHigh; the lap
; compared through $1F58.
; Entry: as ObjectLapBallUp. Exit: as ObjectLapBallUp.
%at($09AD81)
ObjectLapBallUpRecord:
    jsl.l GsuRecordNew42D
    jsl.l GsuRecordFlag10
    lda.b #$14
    jsl.l GsuRecordSet2AHigh
    lda.w !obj_09+1,x
    sta.w !wram_1F58
    sep.b #$20
    lda.w !wram_0DEA
    cmp.w !wram_1F58
    bne .lADAC
    rep.b #$20
    lda.w #$A040
    sta.w !obj_04,x
    sep.b #$20
    jmp.w ObjectLapBallUpStep
.lADAC:
    rep.b #$20
    lda.w #$B7A8
    sta.w !obj_04,x
    sep.b #$20

; @routine 09:ADB6 ObjectLapBallUpStep
; WHAT IT DOES: The up ball's frame: it grows (+$15 and +$14 by $10); on a
; car's contact with flags 1 or 2 (AND #$0B) on a lap it has not lit
; (+$0A), it takes the lap, lights up ($A040), raises that car's record
; +$6A by $40, plays sound $25 and leaves a spark (kind $B7A8) following
; the player.
; HOW: ObjectSyncFromRecord, RecordNextContact, GsuRecordRaise6A,
; ObjectSpawn.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
; it.
%at($09ADB6)
ObjectLapBallUpStep:
    lda.w !obj_15,x
    clc
    adc.b #$10
    sta.w !obj_15,x
    lda.w !obj_14,x
    clc
    adc.b #$10
    sta.w !obj_14,x
    jsl.l ObjectSyncFromRecord
    and.b #$01
    bne .lADD1
    rtl
.lADD1:
    jsl.l RecordNextContact
    bcc .lADE4
    and.b #$0B
    cmp.b #$01
    beq .lADE5
    cmp.b #$02
    beq .lADE5
    jmp.w .lADD1
.lADE4:
    rtl
.lADE5:
    lda.w !obj_09+1,x
    sta.w !wram_1F58
    sep.b #$20
    lda.w !wram_0DEA
    cmp.w !wram_1F58
    beq .lAE5F
    lda.w !wram_0DEA
    sta.w !wram_1F58
    lda.w !wram_1F58
    sta.w !obj_09+1,x
    rep.b #$20
    lda.w #$A040
    sta.w !obj_04,x
    sep.b #$20
    lda.b #$40
    jsl.l GsuRecordRaise6A
    sep.b #$20
    lda.b #$25
    jsl.l SoundPort0QueuePush
    phx
    tyx
    rep.b #$20
    lda.w #$B7A8
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .lAE2F
    jml.l .lAE5E
.lAE2F:
    ldx.w !wram_0346
    lda.b #$0A
    sta.w !obj_09+1,y
    jsl.l ObjectCopyPositionTo
    rep.b #$20
    lda.w #$0000
    sta.w !obj_27,y
    sep.b #$20
    pha
    rep.b #$20
    txa
    sta.w !obj_29,y
    sep.b #$20
    pla
    rep.b #$20
    lda.w #$AE60
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,y
.lAE5E:
    plx
.lAE5F:
    rtl

; @routine 09:AE60 ObjectSparkFollow
; WHAT IT DOES: A following spark's frame: at its object's (+$29) position,
; released when +$0A runs out; it rises by $C0 and a speed +$27 that grows
; by $28 each frame, and grows (+$15, +$14 by $10).
; HOW: ObjectCopyPositionFrom; the speed added through $1F4C;
; ObjectRebucket.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as ObjectRebucket
; leaves it.
%at($09AE60)
ObjectSparkFollow:
    ldy.w !obj_29,x
    jsl.l ObjectCopyPositionFrom
    dec.w !obj_09+1,x
    beq .lAE70
    jml.l .lAE74
.lAE70:
    jsl.l ObjectRelease
.lAE74:
    rep.b #$20
    lda.w !obj_pos_y,x
    clc
    adc.w #$FF40
    sta.w !obj_pos_y,x
    sep.b #$20
    rep.b #$20
    lda.w !obj_27,x
    clc
    adc.w #$FFD8
    sta.w !obj_27,x
    sep.b #$20
    rep.b #$20
    lda.w !obj_27,x
    sta.w !wram_1F4C
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_y,x
    clc
    adc.w !wram_1F4C
    sta.w !obj_pos_y,x
    sep.b #$20
    lda.w !obj_15,x
    clc
    adc.b #$10
    sta.w !obj_15,x
    lda.w !obj_14,x
    clc
    adc.b #$10
    sta.w !obj_14,x
    jsl.l ObjectRebucket
    rtl

; @routine 09:AEBF ObjectLapBallDown
; WHAT IT DOES: Sets up a down ball: restart $09:AEE2
; (ObjectLapBallDownRecord), frame $09:AF17 (ObjectLapBallDownStep), +$0A
; = 4 (no lap lit); its record and frame.
; HOW: Word and byte stores.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
; leaves it.
%at($09AEBF)
ObjectLapBallDown:
    rep.b #$20
    lda.w #$AEE2
    sta.w !obj_1d,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1f,x
    rep.b #$20
    lda.w #$AF17
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,x
    lda.b #$04
    sta.w !obj_09+1,x

; @routine 09:AEE2 ObjectLapBallDownRecord
; WHAT IT DOES: The down ball's restart: its record ($42D, flag bit 4,
; +$2A high $14), its model lit ($A040) when +$0A is the current lap, else
; $B7C4; then its frame.
; HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordSet2AHigh; the lap
; compared through $1F58.
; Entry: as ObjectLapBallDown. Exit: as ObjectLapBallDown.
%at($09AEE2)
ObjectLapBallDownRecord:
    jsl.l GsuRecordNew42D
    jsl.l GsuRecordFlag10
    lda.b #$14
    jsl.l GsuRecordSet2AHigh
    lda.w !obj_09+1,x
    sta.w !wram_1F58
    sep.b #$20
    lda.w !wram_0DEA
    cmp.w !wram_1F58
    bne .lAF0D
    rep.b #$20
    lda.w #$A040
    sta.w !obj_04,x
    sep.b #$20
    jmp.w ObjectLapBallDownStep
.lAF0D:
    rep.b #$20
    lda.w #$B7C4
    sta.w !obj_04,x
    sep.b #$20

; @routine 09:AF17 ObjectLapBallDownStep
; WHAT IT DOES: The down ball's frame: it grows (+$15 and +$14 by $10); on
; a car's contact (flags bit 0) on a lap it has not lit (+$0A), it takes
; the lap, lights up ($A040), lowers that car's record +$82 by $40, plays
; sound $1B and leaves a spark (kind $B7C4) following the player.
; HOW: ObjectSyncFromRecord, RecordNextContact, GsuRecordLower82,
; ObjectSpawn.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
; it.
%at($09AF17)
ObjectLapBallDownStep:
    lda.w !obj_15,x
    clc
    adc.b #$10
    sta.w !obj_15,x
    lda.w !obj_14,x
    clc
    adc.b #$10
    sta.w !obj_14,x
    jsl.l ObjectSyncFromRecord
    and.b #$01
    bne .lAF32
    rtl
.lAF32:
    jsl.l RecordNextContact
    bcc .lAFB6
    and.b #$01
    beq .lAF32
    lda.w !obj_09+1,x
    sta.w !wram_1F58
    sep.b #$20
    lda.w !wram_0DEA
    cmp.w !wram_1F58
    beq .lAFB6
    lda.w !wram_0DEA
    sta.w !wram_1F58
    lda.w !wram_1F58
    sta.w !obj_09+1,x
    rep.b #$20
    lda.w #$A040
    sta.w !obj_04,x
    sep.b #$20
    lda.b #$40
    jsl.l GsuRecordLower82
    lda.b #$1B
    jsl.l SoundPort0QueuePush
    sep.b #$20
    phx
    tyx
    rep.b #$20
    lda.w #$B7C4
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .lAF86
    jml.l .lAFB5
.lAF86:
    ldx.w !wram_0346
    lda.b #$0A
    sta.w !obj_09+1,y
    jsl.l ObjectCopyPositionTo
    rep.b #$20
    lda.w #$0000
    sta.w !obj_27,y
    sep.b #$20
    pha
    rep.b #$20
    txa
    sta.w !obj_29,y
    sep.b #$20
    pla
    rep.b #$20
    lda.w #$AE60
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,y
.lAFB5:
    plx
.lAFB6:
    rtl

; @routine 09:AFB7 ObjectCountGate
; WHAT IT DOES: Sets up the count gate: restart $09:AFD5
; (ObjectCountGateRecord), frame $09:AFFC (ObjectCountGateStep); its record
; and frame.
; HOW: Word and byte stores.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
; leaves it.
%at($09AFB7)
ObjectCountGate:
    rep.b #$20
    lda.w #$AFD5
    sta.w !obj_1d,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1f,x
    rep.b #$20
    lda.w #$AFFC
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,x

; @routine 09:AFD5 ObjectCountGateRecord
; WHAT IT DOES: The count gate's restart: its record ($42D, flag bit 4, at
; ($780, $780, $780)), then its frame.
; HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition.
; Entry: as ObjectCountGate. Exit: as ObjectCountGate.
%at($09AFD5)
ObjectCountGateRecord:
    jsl.l GsuRecordNew42D
    jsl.l GsuRecordFlag10
    rep.b #$20
    lda.w #$0780
    sta.b !dp_02
    sep.b #$20
    rep.b #$20
    lda.w #$0780
    sta.b !dp_08
    sep.b #$20
    rep.b #$20
    lda.w #$0780
    sta.b !dp_94
    sep.b #$20
    jsl.l GsuRecordPosition

; @routine 09:AFFC ObjectCountGateStep
; WHAT IT DOES: The count gate's frame: a car's hit, the first time (+$25
; bit 0), moves the count at $19EE to $19F0 (clearing $19EE, $19F2 = 1)
; and sets $19ED to 8.
; HOW: ObjectSyncFromRecord, RecordContactSummary; JSR into
; HudCountMove19EE at its copy (09:C0AB).
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
; it.
%at($09AFFC)
ObjectCountGateStep:
    jsl.l ObjectSyncFromRecord
    and.b #$01
    beq .lB029
    jsl.l RecordContactSummary
    and.b #$01
    beq .lB029
    lda.w !obj_25,x
    and.b #$01
    beq .lB017
    jml.l .lB029
.lB017:
    lda.w !obj_25,x
    ora.b #$01
    sta.w !obj_25,x
    jsr.w HudCountMove19EE_copy
    sep.b #$20
    lda.b #$08
    sta.w !wram_19ED
.lB029:
    rtl

; @routine 09:B02A ObjectCountMarker
; WHAT IT DOES: Sets up the counting marker: restart $09:B065
; (ObjectCountMarkerRecord), frame $09:B092 (ObjectCountMarkerStep), +$20
; = $ECD0, shown as 0 (+$5A = $80, twice), +$48 = 4; its record and frame.
; HOW: Word and byte stores; LDA #0, ORA #$80 for +$5A.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
; leaves it.
%at($09B02A)
ObjectCountMarker:
    rep.b #$20
    lda.w #$B065
    sta.w !obj_1d,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1f,x
    rep.b #$20
    lda.w #$B092
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,x
    rep.b #$20
    lda.w #$ECD0
    sta.w !obj_20,x
    sep.b #$20
    lda.b #$00
    ora.b #$80
    sta.w !obj_5a,x
    lda.b #$04
    sta.w !obj_48,x
    lda.b #$00
    ora.b #$80
    sta.w !obj_5a,x

; @routine 09:B065 ObjectCountMarkerRecord
; WHAT IT DOES: The marker's restart: its record (GsuRecordNew424Alt) in
; mode 2 with +$2A high $14 at ($500, $500, $500), then its frame.
; HOW: GsuRecordNew424Alt, GsuRecordMode2, GsuRecordSet2AHigh,
; GsuRecordPosition.
; Entry: as ObjectCountMarker. Exit: as ObjectCountMarker.
%at($09B065)
ObjectCountMarkerRecord:
    jsl.l GsuRecordNew424Alt
    jsl.l GsuRecordMode2
    lda.b #$14
    jsl.l GsuRecordSet2AHigh
    rep.b #$20
    lda.w #$0500
    sta.b !dp_02
    sep.b #$20
    rep.b #$20
    lda.w #$0500
    sta.b !dp_08
    sep.b #$20
    rep.b #$20
    lda.w #$0500
    sta.b !dp_94
    sep.b #$20
    jsl.l GsuRecordPosition

; @routine 09:B092 ObjectCountMarkerStep
; WHAT IT DOES: The marker's frame: while $19E8 is set its count +$48
; steps (ObjectStep48To8); then it is synced from its record.
; HOW: JSR ObjectStep48To8; ObjectSyncFromRecord.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as
; ObjectSyncFromRecord leaves it.
%at($09B092)
ObjectCountMarkerStep:
    sep.b #$20
    lda.w !wram_19E8
    beq .lB09C
    jsr.w ObjectStep48To8
.lB09C:
    jsl.l ObjectSyncFromRecord
    rtl

; @routine 09:C438 ObjectCountUpTo2
; WHAT IT DOES: Counts object X's +$48 up, to at most 2, and shows it at
; +$5A with bit 7 ($1F58 = the count).
; HOW: CMP #2 and INC; the count through $1F58.
; Entry: M 8-bit, X 16-bit; X = object. Exit: A low byte = the count | $80;
; N, Z from it.
%at($09C438)
ObjectCountUpTo2:
    lda.w !obj_48,x
    cmp.b #$02
    bne .lC443
    jml.l .lC446
.lC443:
    inc.w !obj_48,x
.lC446:
    lda.w !obj_48,x
    sta.w !wram_1F58
    lda.w !wram_1F58
    ora.b #$80
    sta.w !obj_5a,x
    rts

; @routine 09:C455 ObjectBob
; WHAT IT DOES: Sets up a bobbing object: offset +$27 = 1, speed +$29 = 1,
; frame $09:C478 (ObjectBobStep); then its frame.
; HOW: Word and byte stores. Reached only by unit_objects_09c.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
; leaves it.
%at($09C455)
ObjectBob:
    rep.b #$20
    lda.w #$0001
    sta.w !obj_27,x
    sep.b #$20
    rep.b #$20
    lda.w #$0001
    sta.w !obj_29,x
    sep.b #$20
    rep.b #$20
    lda.w #$C478
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,x

; @routine 09:C478 ObjectBobStep
; WHAT IT DOES: The bob's frame: the speed +$29 is added to the offset
; +$27, the speed turning round when the offset reaches $10 or -$10; the
; height +$0E moves by the offset and the scale +$15 grows by 4.
; HOW: 16-bit ADC, CMP, EOR #$FFFF + INC; the offset added through $1F4C;
; ObjectRebucket. Reached only by unit_objects_09c.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as ObjectRebucket
; leaves it.
%at($09C478)
ObjectBobStep:
    rep.b #$20
    lda.w !obj_27,x
    clc
    adc.w !obj_29,x
    sta.w !obj_27,x
    cmp.w #$0010
    beq .lC48E
    cmp.w #$FFF0
    bne .lC498
.lC48E:
    lda.w !obj_29,x
    eor.w #$FFFF
    inc a
    sta.w !obj_29,x
.lC498:
    sep.b #$20
    rep.b #$20
    lda.w !obj_27,x
    sta.w !wram_1F4C
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_y,x
    clc
    adc.w !wram_1F4C
    sta.w !obj_pos_y,x
    sep.b #$20
    lda.w !obj_15,x
    clc
    adc.b #$04
    sta.w !obj_15,x
    jsl.l ObjectRebucket
    rtl

; @routine 09:C4C0 ObjectBreakablePiece
; WHAT IT DOES: Sets up a piece of a burst (09:D79C's): frame $09:C4E1
; (ObjectBreakablePieceStep), a flying piece's record (GsuRecordNew424Piece)
; with its velocity and +$2A high 0, then its frame; with no record free it
; is released.
; HOW: GsuRecordNew424Piece (N set: none), GsuRecordVelocity,
; GsuRecordSet2AHigh.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame or
; ObjectRelease leaves it.
%at($09C4C0)
ObjectBreakablePiece:
    sep.b #$20
    rep.b #$20
    lda.w #$C4E1
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,x
    jsl.l GsuRecordNew424Piece
    bmi ObjectBreakablePieceStep_lC50E
    jsl.l GsuRecordVelocity
    lda.b #$00
    jsl.l GsuRecordSet2AHigh

; @routine 09:C4E1 ObjectBreakablePieceStep
; WHAT IT DOES: A burst piece's frame: synced from its record, released
; when its life +$0A runs out, and it grows (+$14, +$15, +$16 by $18).
; HOW: ObjectSyncFromRecord, DEC, ADCs.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: A low byte = the
; new +$16; N, Z, C, V from its add.
%at($09C4E1)
ObjectBreakablePieceStep:
    jsl.l ObjectSyncFromRecord
    dec.w !obj_09+1,x
    beq .lC4EE
    jml.l .lC4F2
.lC4EE:
    jsl.l ObjectRelease
.lC4F2:
    lda.w !obj_14,x
    clc
    adc.b #$18
    sta.w !obj_14,x
    lda.w !obj_15,x
    clc
    adc.b #$18
    sta.w !obj_15,x
    lda.w !obj_16,x
    clc
    adc.b #$18
    sta.w !obj_16,x
    rtl
.lC50E:
    jsl.l ObjectRelease
    rtl

; @routine 09:C513 ObjectBreakable
; WHAT IT DOES: Sets up a breakable object: frame $09:C560
; (ObjectBreakableStep), restart $09:C531 (ObjectBreakableRecord); its
; record and frame.
; HOW: Word and byte stores (the frame's first). Reached only by
; unit_objects_09c.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
; leaves it.
%at($09C513)
ObjectBreakable:
    rep.b #$20
    lda.w #$C560
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,x
    rep.b #$20
    lda.w #$C531
    sta.w !obj_1d,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1f,x

; @routine 09:C531 ObjectBreakableRecord
; WHAT IT DOES: The breakable's restart: a flying piece's record
; (GsuRecordNew424Piece) with flag bit 4, +$2A high $46, at ($C0, $C0,
; $C0), then its frame; with no record free it is released.
; HOW: GsuRecordNew424Piece (N set: none; the release is 09:C50E's),
; GsuRecordFlag10, GsuRecordSet2AHigh, GsuRecordPosition.
; Reached only by unit_objects_09c.
; Entry: as ObjectBreakable. Exit: as ObjectBreakable.
%at($09C531)
ObjectBreakableRecord:
    jsl.l GsuRecordNew424Piece
    bmi ObjectBreakablePieceStep_lC50E
    jsl.l GsuRecordFlag10
    lda.b #$46
    jsl.l GsuRecordSet2AHigh
    rep.b #$20
    lda.w #$00C0
    sta.b !dp_02
    sep.b #$20
    rep.b #$20
    lda.w #$00C0
    sta.b !dp_08
    sep.b #$20
    rep.b #$20
    lda.w #$00C0
    sta.b !dp_94
    sep.b #$20
    jsl.l GsuRecordPosition

; @routine 09:C560 ObjectBreakableStep
; WHAT IT DOES: The breakable's frame: released when its life +$0A runs
; out; it grows by $0C (the words at +$14, +$15 and +$16 in turn); a car's
; hit releases it with sound $5A and bursts it into eight pieces
; (ObjectBreakableBurst).
; HOW: DEC, 16-bit ADCs, ObjectSyncFromRecord, RecordContactSummary.
; Reached only by unit_objects_09c.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
; it.
%at($09C560)
ObjectBreakableStep:
    dec.w !obj_09+1,x
    beq .lC569
    jml.l .lC56D
.lC569:
    jsl.l ObjectRelease
.lC56D:
    rep.b #$20
    lda.w !obj_14,x
    clc
    adc.w #$000C
    sta.w !obj_14,x
    sep.b #$20
    rep.b #$20
    lda.w !obj_15,x
    clc
    adc.w #$000C
    sta.w !obj_15,x
    sep.b #$20
    rep.b #$20
    lda.w !obj_16,x
    clc
    adc.w #$000C
    sta.w !obj_16,x
    sep.b #$20
    jsl.l ObjectSyncFromRecord
    and.b #$01
    beq .lC5B5
    jsl.l RecordContactSummary
    and.b #$01
    beq .lC5B5
    jsl.l ObjectRelease
    lda.b #$5A
    jsl.l SoundPort0QueuePush
    jsl.l ObjectBreakableBurst
.lC5B5:
    rtl

; @routine 09:C5B6 ObjectColour0
; WHAT IT DOES: Sets object X's colour +$18 to 0 with bit 7.
; HOW: LDA #0, ORA #$80; starts with a NOP, as the course data's pointers
; expect. Reached only by unit_objects_09c.
; Entry: M 8-bit; X = object. Exit: A low byte = $80; N set.
%at($09C5B6)
ObjectColour0:
    nop
    lda.b #$00
    ora.b #$80
    sta.w !obj_18,x
    rtl

; @routine 09:C5BF ObjectColour1
; WHAT IT DOES: Sets object X's colour +$18 to 1 with bit 7.
; HOW: LDA #1, ORA #$80; starts with a NOP, as the course data's pointers
; expect. Reached only by unit_objects_09c.
; Entry: M 8-bit; X = object. Exit: A low byte = $81; N set.
%at($09C5BF)
ObjectColour1:
    nop
    lda.b #$01
    ora.b #$80
    sta.w !obj_18,x
    rtl

; @routine 09:C5C8 ObjectColour2
; WHAT IT DOES: Sets object X's colour +$18 to 2 with bit 7.
; HOW: LDA #2, ORA #$80; starts with a NOP, as the course data's pointers
; expect.
; Entry: M 8-bit; X = object. Exit: A low byte = $82; N set.
%at($09C5C8)
ObjectColour2:
    nop
    lda.b #$02
    ora.b #$80
    sta.w !obj_18,x
    rtl

; @routine 09:C5D1 ObjectColour3
; WHAT IT DOES: Sets object X's colour +$18 to 3 with bit 7.
; HOW: LDA #3, ORA #$80; starts with a NOP, as the course data's pointers
; expect.
; Entry: M 8-bit; X = object. Exit: A low byte = $83; N set.
%at($09C5D1)
ObjectColour3:
    nop
    lda.b #$03
    ora.b #$80
    sta.w !obj_18,x
    rtl

; @routine 09:C5DA ObjectColour4
; WHAT IT DOES: Sets object X's colour +$18 to 4 with bit 7.
; HOW: LDA #4, ORA #$80; starts with a NOP, as the course data's pointers
; expect.
; Entry: M 8-bit; X = object. Exit: A low byte = $84; N set.
%at($09C5DA)
ObjectColour4:
    nop
    lda.b #$04
    ora.b #$80
    sta.w !obj_18,x
    rtl

; @routine 09:C5E3 ObjectColour5
; WHAT IT DOES: Sets object X's colour +$18 to 5 with bit 7.
; HOW: LDA #5, ORA #$80; starts with a NOP, as the course data's pointers
; expect.
; Entry: M 8-bit; X = object. Exit: A low byte = $85; N set.
%at($09C5E3)
ObjectColour5:
    nop
    lda.b #$05
    ora.b #$80
    sta.w !obj_18,x
    rtl

; @routine 09:C5EC ObjectColour6
; WHAT IT DOES: Sets object X's colour +$18 to 6 with bit 7.
; HOW: LDA #6, ORA #$80; starts with a NOP, as the course data's pointers
; expect.
; Entry: M 8-bit; X = object. Exit: A low byte = $86; N set.
%at($09C5EC)
ObjectColour6:
    nop
    lda.b #$06
    ora.b #$80
    sta.w !obj_18,x
    rtl

; @routine 09:C5F5 ObjectColour7
; WHAT IT DOES: Sets object X's colour +$18 to 7 with bit 7.
; HOW: LDA #7, ORA #$80; starts with a NOP, as the course data's pointers
; expect.
; Entry: M 8-bit; X = object. Exit: A low byte = $87; N set.
%at($09C5F5)
ObjectColour7:
    nop
    lda.b #$07
    ora.b #$80
    sta.w !obj_18,x
    rtl

; @routine 09:C5FE ObjectScaleE0
; WHAT IT DOES: Sets object X's scale +$15 to $E0.
; HOW: One store; starts with a NOP, as the course data's pointers expect.
; Entry: M 8-bit; X = object. Exit: A low byte = $E0.
%at($09C5FE)
ObjectScaleE0:
    nop
    lda.b #$E0
    sta.w !obj_15,x
    rtl

; @routine 09:C605 ObjectScaleA0
; WHAT IT DOES: Sets object X's scale +$15 to $A0.
; HOW: One store; starts with a NOP, as the course data's pointers expect.
; Entry: M 8-bit; X = object. Exit: A low byte = $A0.
%at($09C605)
ObjectScaleA0:
    nop
    lda.b #$A0
    sta.w !obj_15,x
    rtl

; @routine 09:C60C ObjectScale20
; WHAT IT DOES: Sets object X's scale +$15 to $20.
; HOW: One store; starts with a NOP, as the course data's pointers expect.
; Entry: M 8-bit; X = object. Exit: A low byte = $20.
%at($09C60C)
ObjectScale20:
    nop
    lda.b #$20
    sta.w !obj_15,x
    rtl

; @routine 09:C613 ObjectSwing
; WHAT IT DOES: Sets up a swinging object: scale +$15 = 0, +$0A = $28,
; frame $09:C62D (ObjectSwingStep); then its frame.
; HOW: Byte and word stores; starts with a NOP, as the course data's
; pointers expect.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
; leaves it.
%at($09C613)
ObjectSwing:
    nop
    lda.b #$00
    sta.w !obj_15,x
    lda.b #$28
    sta.w !obj_09+1,x
    rep.b #$20
    lda.w #$C62D
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,x

; @routine 09:C62D ObjectSwingStep
; WHAT IT DOES: The swing's frame: it moves $28 to one side (+$25 bit 0:
; right) with its scale +$15 eased toward $40 (left) or $C0 (right); every
; $50 frames (+$0A) it turns to the other side.
; HOW: 16-bit ADC on +$0C; ApproachEighthMin8Byte through $3E;
; ObjectRebucket.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as ObjectRebucket
; leaves it.
%at($09C62D)
ObjectSwingStep:
    lda.w !obj_25,x
    and.b #$01
    beq .lC638
    jml.l .lC657
.lC638:
    rep.b #$20
    lda.w !obj_pos_x,x
    clc
    adc.w #$FFD8
    sta.w !obj_pos_x,x
    sep.b #$20
    lda.w !obj_15,x
    sta.b !scale_scratch
    lda.b #$40
    jsl.l ApproachEighthMin8Byte
    sta.w !obj_15,x
    jmp.w .lC673
.lC657:
    rep.b #$20
    lda.w !obj_pos_x,x
    clc
    adc.w #$0028
    sta.w !obj_pos_x,x
    sep.b #$20
    lda.w !obj_15,x
    sta.b !scale_scratch
    lda.b #$C0
    jsl.l ApproachEighthMin8Byte
    sta.w !obj_15,x
.lC673:
    lda.w !obj_09+1,x
    bne .lC67C
    jml.l .lC682
.lC67C:
    dec.w !obj_09+1,x
    jmp.w .lC68F
.lC682:
    lda.w !obj_25,x
    eor.b #$01
    sta.w !obj_25,x
    lda.b #$50
    sta.w !obj_09+1,x
.lC68F:
    jsl.l ObjectRebucket
    rtl

; @routine 09:C694 LapCrowdSpawn0
; WHAT IT DOES: The lap crowd gate's lap 0 crowd: kind $B770 at the gate
; moved ($F00, -$1C0, -$2800), scale $40, life $1C, handler $09:C7C9
; (LapCrowdStep0), sound $52; nothing without a free object.
; HOW: ObjectSpawn, ObjectCopyPositionTo, 16-bit ADCs. Reached only by
; unit_objects_09c.
; Entry: M 8-bit, X 16-bit; X = the gate. Exit: as SoundPort0QueuePush or
; ObjectSpawn leaves it.
%at($09C694)
LapCrowdSpawn0:
    rep.b #$20
    lda.w #$B770
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .lC6A8
    jml.l .lC6FA
.lC6A8:
    jsl.l ObjectCopyPositionTo
    rep.b #$20
    lda.w !obj_pos_x,y
    clc
    adc.w #$0F00
    sta.w !obj_pos_x,y
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_y,y
    clc
    adc.w #$FE40
    sta.w !obj_pos_y,y
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_z,y
    clc
    adc.w #$D800
    sta.w !obj_pos_z,y
    sep.b #$20
    lda.b #$40
    sta.w !obj_15,y
    lda.b #$F8
    sta.w !obj_14,y
    lda.b #$1C
    sta.w !obj_09+1,y
    rep.b #$20
    lda.w #$C7C9
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,y
    lda.b #$52
    jsl.l SoundPort0QueuePush
.lC6FA:
    rts

; @routine 09:C6FB LapCrowdSpawn1
; WHAT IT DOES: The lap 1 crowd: kind $B770 moved (-$A00, -$1E0, -$2BC0),
; scale $E0, life $16, handler $09:C7FC (LapCrowdStep1), sound $52.
; HOW: As LapCrowdSpawn0. Reached only by unit_objects_09c.
; Entry: as LapCrowdSpawn0. Exit: as LapCrowdSpawn0.
%at($09C6FB)
LapCrowdSpawn1:
    rep.b #$20
    lda.w #$B770
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .lC70F
    jml.l .lC761
.lC70F:
    jsl.l ObjectCopyPositionTo
    rep.b #$20
    lda.w !obj_pos_x,y
    clc
    adc.w #$F600
    sta.w !obj_pos_x,y
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_y,y
    clc
    adc.w #$FE20
    sta.w !obj_pos_y,y
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_z,y
    clc
    adc.w #$D440
    sta.w !obj_pos_z,y
    sep.b #$20
    lda.b #$E0
    sta.w !obj_15,y
    lda.b #$F8
    sta.w !obj_14,y
    lda.b #$16
    sta.w !obj_09+1,y
    rep.b #$20
    lda.w #$C7FC
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,y
    lda.b #$52
    jsl.l SoundPort0QueuePush
.lC761:
    rts

; @routine 09:C762 LapCrowdSpawn2
; WHAT IT DOES: The lap 2 crowd: kind $B770 moved ($3C0, -$1E0, -$2F80),
; scale 0, life $16, handler $09:C83D (LapCrowdStep2), sound $52.
; HOW: As LapCrowdSpawn0. Reached only by unit_objects_09c.
; Entry: as LapCrowdSpawn0. Exit: as LapCrowdSpawn0.
%at($09C762)
LapCrowdSpawn2:
    rep.b #$20
    lda.w #$B770
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .lC776
    jml.l .lC7C8
.lC776:
    jsl.l ObjectCopyPositionTo
    rep.b #$20
    lda.w !obj_pos_x,y
    clc
    adc.w #$03C0
    sta.w !obj_pos_x,y
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_y,y
    clc
    adc.w #$FE20
    sta.w !obj_pos_y,y
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_z,y
    clc
    adc.w #$D080
    sta.w !obj_pos_z,y
    sep.b #$20
    lda.b #$00
    sta.w !obj_15,y
    lda.b #$F8
    sta.w !obj_14,y
    lda.b #$16
    sta.w !obj_09+1,y
    rep.b #$20
    lda.w #$C83D
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,y
    lda.b #$52
    jsl.l SoundPort0QueuePush
.lC7C8:
    rts

; @routine 09:C7C9 LapCrowdStep0
; WHAT IT DOES: The lap 0 crowd's frame: LapCrowdMove0, then ObjectRebucket.
; HOW: Starts with a NOP, as the handler pointers expect. Reached only by
; unit_objects_09c.
; Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectRebucket leaves it.
%at($09C7C9)
LapCrowdStep0:
    nop
    jsr.w LapCrowdMove0
    jsl.l ObjectRebucket
    rtl

; @routine 09:C7D2 LapCrowdMove0
; WHAT IT DOES: Moves the lap 0 crowd by (-$B4, -$0A, 0); released when its
; life +$0A runs out.
; HOW: 16-bit ADCs, DEC. Reached only by unit_objects_09c.
; Entry: M 8-bit, X 16-bit; X = object. Exit: as DEC or ObjectRelease
; leaves it.
%at($09C7D2)
LapCrowdMove0:
    rep.b #$20
    lda.w !obj_pos_x,x
    clc
    adc.w #$FF4C
    sta.w !obj_pos_x,x
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_y,x
    clc
    adc.w #$FFF6
    sta.w !obj_pos_y,x
    sep.b #$20
    dec.w !obj_09+1,x
    beq .lC7F7
    jml.l .lC7FB
.lC7F7:
    jsl.l ObjectRelease
.lC7FB:
    rts

; @routine 09:C7FC LapCrowdStep1
; WHAT IT DOES: The lap 1 crowd's frame: LapCrowdMove1, then ObjectRebucket.
; HOW: Starts with a NOP. Reached only by unit_objects_09c.
; Entry: as LapCrowdStep0. Exit: as LapCrowdStep0.
%at($09C7FC)
LapCrowdStep1:
    nop
    jsr.w LapCrowdMove1
    jsl.l ObjectRebucket
    rtl

; @routine 09:C805 LapCrowdMove1
; WHAT IT DOES: Moves the lap 1 crowd by ($104, -$0A, $C8); released when
; its life +$0A runs out.
; HOW: 16-bit ADCs, DEC. Reached only by unit_objects_09c.
; Entry: as LapCrowdMove0. Exit: as LapCrowdMove0.
%at($09C805)
LapCrowdMove1:
    rep.b #$20
    lda.w !obj_pos_x,x
    clc
    adc.w #$0104
    sta.w !obj_pos_x,x
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_y,x
    clc
    adc.w #$FFF6
    sta.w !obj_pos_y,x
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_z,x
    clc
    adc.w #$00C8
    sta.w !obj_pos_z,x
    sep.b #$20
    dec.w !obj_09+1,x
    beq .lC838
    jml.l .lC83C
.lC838:
    jsl.l ObjectRelease
.lC83C:
    rts

; @routine 09:C83D LapCrowdStep2
; WHAT IT DOES: The lap 2 crowd's frame: LapCrowdMove2, then ObjectRebucket.
; HOW: Starts with a NOP. Reached only by unit_objects_09c.
; Entry: as LapCrowdStep0. Exit: as LapCrowdStep0.
%at($09C83D)
LapCrowdStep2:
    nop
    jsr.w LapCrowdMove2
    jsl.l ObjectRebucket
    rtl

; @routine 09:C846 LapCrowdMove2
; WHAT IT DOES: Moves the lap 2 crowd by (0, -$0D, $C8); released when its
; life +$0A runs out.
; HOW: 16-bit ADCs, DEC. Reached only by unit_objects_09c.
; Entry: as LapCrowdMove0. Exit: as LapCrowdMove0.
%at($09C846)
LapCrowdMove2:
    rep.b #$20
    lda.w !obj_pos_y,x
    clc
    adc.w #$FFF3
    sta.w !obj_pos_y,x
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_z,x
    clc
    adc.w #$00C8
    sta.w !obj_pos_z,x
    sep.b #$20
    dec.w !obj_09+1,x
    beq .lC86B
    jml.l .lC86F
.lC86B:
    jsl.l ObjectRelease
.lC86F:
    rts

; @routine 09:C870 ObjectLapCrowdGate
; WHAT IT DOES: Sets up the lap crowd gate: restart $09:C893
; (ObjectLapCrowdGateRecord), frame $09:C8BA (ObjectLapCrowdGateStep), +$48
; = 4 (no lap counted); its record and frame.
; HOW: Word and byte stores.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
; leaves it.
%at($09C870)
ObjectLapCrowdGate:
    rep.b #$20
    lda.w #$C893
    sta.w !obj_1d,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1f,x
    rep.b #$20
    lda.w #$C8BA
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,x
    lda.b #$04
    sta.w !obj_48,x

; @routine 09:C893 ObjectLapCrowdGateRecord
; WHAT IT DOES: The gate's restart: its record ($42D, flag bit 4, at
; ($780, $780, $780)), then its frame.
; HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition.
; Entry: as ObjectLapCrowdGate. Exit: as ObjectLapCrowdGate.
%at($09C893)
ObjectLapCrowdGateRecord:
    jsl.l GsuRecordNew42D
    jsl.l GsuRecordFlag10
    rep.b #$20
    lda.w #$0780
    sta.b !dp_02
    sep.b #$20
    rep.b #$20
    lda.w #$0780
    sta.b !dp_08
    sep.b #$20
    rep.b #$20
    lda.w #$0780
    sta.b !dp_94
    sep.b #$20
    jsl.l GsuRecordPosition

; @routine 09:C8BA ObjectLapCrowdGateStep
; WHAT IT DOES: The gate's frame: synced to its record; a car's hit on a lap
; ($0DEA) it has not counted (+$48) counts that lap and, on laps 0, 1 and
; 2, brings out that lap's crowd (LapCrowdSpawn0-2).
; HOW: ObjectSyncToRecord, RecordContactSummary; the laps compared through
; $1F58; JSRs.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
; it.
%at($09C8BA)
ObjectLapCrowdGateStep:
    jsl.l ObjectSyncToRecord
    and.b #$01
    beq .lC90B
    jsl.l RecordContactSummary
    and.b #$01
    beq .lC90B
    lda.w !obj_48,x
    sta.w !wram_1F58
    sep.b #$20
    lda.w !wram_0DEA
    cmp.w !wram_1F58
    beq .lC90B
    sep.b #$20
    lda.w !wram_0DEA
    sta.w !wram_1F58
    lda.w !wram_1F58
    sta.w !obj_48,x
    sep.b #$20
    lda.w !wram_0DEA
    cmp.b #$00
    beq .lC8FC
    cmp.b #$01
    beq .lC902
    cmp.b #$02
    beq .lC908
    jmp.w .lC90B
.lC8FC:
    jsr.w LapCrowdSpawn0
    jmp.w .lC90B
.lC902:
    jsr.w LapCrowdSpawn1
    jmp.w .lC90B
.lC908:
    jsr.w LapCrowdSpawn2
.lC90B:
    rtl

; @routine 09:C90C ObjectFloaterSpawner
; WHAT IT DOES: Sets object X's frame to $09:C956 (ObjectFloaterSpawnerStep)
; and puts a floater (kind $B3B8) $6E0 lower with life +$0A = $28 and
; handler $09:C95C (ObjectFloaterStep); no floater without a free object.
; HOW: ObjectSpawn, ObjectCopyPositionTo.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the last store
; or ObjectSpawn leaves it.
%at($09C90C)
ObjectFloaterSpawner:
    rep.b #$20
    lda.w #$C956
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,x
    rep.b #$20
    lda.w #$B3B8
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .lC92F
    jml.l .lC955
.lC92F:
    jsl.l ObjectCopyPositionTo
    rep.b #$20
    lda.w !obj_pos_y,y
    clc
    adc.w #$06E0
    sta.w !obj_pos_y,y
    sep.b #$20
    lda.b #$28
    sta.w !obj_09+1,y
    rep.b #$20
    lda.w #$C95C
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,y
.lC955:
    rtl

; @routine 09:C956 ObjectFloaterSpawnerStep
; WHAT IT DOES: The spawner's frame: its scale +$15 = $40.
; HOW: One store.
; Entry: M 8-bit; X = object. Exit: A low byte = $40.
%at($09C956)
ObjectFloaterSpawnerStep:
    lda.b #$40
    sta.w !obj_15,x
    rtl

; @routine 09:C95C ObjectFloaterStep
; WHAT IT DOES: The floater's frame: rebucketed, its scale +$15 grows by 8;
; it rises $14 (+$25 bit 0 clear) or sinks $14; every $28 frames (+$0A) the
; direction flips.
; HOW: ObjectRebucket first; 16-bit ADC on +$0E; EOR #1.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the last store
; leaves it.
%at($09C95C)
ObjectFloaterStep:
    jsl.l ObjectRebucket
    lda.w !obj_15,x
    clc
    adc.b #$08
    sta.w !obj_15,x
    lda.w !obj_25,x
    and.b #$01
    beq .lC974
    jml.l .lC985
.lC974:
    rep.b #$20
    lda.w !obj_pos_y,x
    clc
    adc.w #$0014
    sta.w !obj_pos_y,x
    sep.b #$20
    jmp.w .lC993
.lC985:
    rep.b #$20
    lda.w !obj_pos_y,x
    clc
    adc.w #$FFEC
    sta.w !obj_pos_y,x
    sep.b #$20
.lC993:
    lda.w !obj_09+1,x
    bne .lC99C
    jml.l .lC9A2
.lC99C:
    dec.w !obj_09+1,x
    jmp.w .lC9AF
.lC9A2:
    lda.w !obj_25,x
    eor.b #$01
    sta.w !obj_25,x
    lda.b #$28
    sta.w !obj_09+1,x
.lC9AF:
    rtl

; @routine 09:C9B0 ObjectRing
; WHAT IT DOES: Sets up a ring piece circling a centre: the centre (kind
; $A040, handler $09:CAA3 ObjectRingCentreStep) at its position, its frame
; $09:C9F0 (ObjectRingStep); then that frame.
; HOW: ObjectSpawn, ObjectCopyPositionTo; +$27 = the centre. Reached only
; by unit_objects_09c.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
; leaves it.
%at($09C9B0)
ObjectRing:
    rep.b #$20
    lda.w #$A040
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .lC9C4
    jml.l .lC9E1
.lC9C4:
    jsl.l ObjectCopyPositionTo
    pha
    rep.b #$20
    tya
    sta.w !obj_27,x
    sep.b #$20
    pla
    rep.b #$20
    lda.w #$CAA3
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,y
.lC9E1:
    rep.b #$20
    lda.w #$C9F0
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,x

; @routine 09:C9F0 ObjectRingStep
; WHAT IT DOES: The ring piece's frame: at its centre (+$27) plus ($64, 0,
; 0) turned by the centre's angles +$16, +$14 and +$15, times 32; it takes
; the centre's scale +$15.
; HOW: RotateXY, RotateXZFrom08, RotateXZ through direct page; five ASLs;
; ObjectRebucket. The sign bytes come from constants: their $FF stores
; (09:C9FD, 09:CA0F, 09:CA22) never run. Reached only by unit_objects_09c.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as ObjectRebucket
; leaves it.
%at($09C9F0)
ObjectRingStep:
    ldy.w !obj_27,x
    lda.b #$64
    sta.b !dp_02
    sta.b !dp_04
    bit.b #$80
    beq .lCA03
    lda.b #$FF
    sta.b !dp_05
    bra .lCA05
.lCA03:
    stz.b !dp_05
.lCA05:
    lda.b #$00
    sta.b !dp_08
    sta.b !actor_0a
    bit.b #$80
    beq .lCA15
    lda.b #$FF
    sta.b !dp_0b
    bra .lCA17
.lCA15:
    stz.b !dp_0b
.lCA17:
    lda.b #$00
    sta.b !dp_94
    sta.w !rot_z
    bit.b #$80
    beq .lCA29
    lda.b #$FF
    sta.w !rot_z_high
    bra .lCA2C
.lCA29:
    stz.w !rot_z_high
.lCA2C:
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
    asl a
    asl a
    sta.b !dp_04
    lda.b !actor_0a
    asl a
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
    lda.w !obj_15,y
    sta.w !obj_15,x
    jsl.l ObjectRebucket
    rtl

; @routine 09:CAA3 ObjectRingCentreStep
; WHAT IT DOES: The ring centre's frame: its scale +$15 grows by 1.
; HOW: ADC; ObjectRebucket. Reached only by unit_objects_09c.
; Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectRebucket leaves it.
%at($09CAA3)
ObjectRingCentreStep:
    lda.w !obj_15,x
    clc
    adc.b #$01
    sta.w !obj_15,x
    jsl.l ObjectRebucket
    rtl

; @routine 09:CAB1 ObjectSpinner
; WHAT IT DOES: Sets up a spinner: colour +$5A = 0 with bit 7, a centre
; (kind $A040, handler $09:CBC4 ObjectSpinnerCentreStep) at its position,
; its frame $09:CAF8 (ObjectSpinnerStep); then that frame.
; HOW: ObjectSpawn, ObjectCopyPositionTo; +$27 = the centre.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
; leaves it.
%at($09CAB1)
ObjectSpinner:
    lda.b #$00
    ora.b #$80
    sta.w !obj_5a,x
    rep.b #$20
    lda.w #$A040
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .lCACC
    jml.l .lCAE9
.lCACC:
    jsl.l ObjectCopyPositionTo
    pha
    rep.b #$20
    tya
    sta.w !obj_27,x
    sep.b #$20
    pla
    rep.b #$20
    lda.w #$CBC4
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,y
.lCAE9:
    rep.b #$20
    lda.w #$CAF8
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,x

; @routine 09:CAF8 ObjectSpinnerStep
; WHAT IT DOES: The spinner's frame: at its centre (+$27) plus ($28, $0A,
; 0) turned by the centre's angles, times 32, with the centre's scale; its
; colour +$5A steps on through 0-5 (bit 7 kept).
; HOW: As ObjectRingStep; the colour by ADC, AND #$7F, SBC #6. The sign
; bytes come from constants: their $FF stores (09:CB05, 09:CB17, 09:CB2A)
; never run.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as ObjectRebucket
; leaves it.
%at($09CAF8)
ObjectSpinnerStep:
    ldy.w !obj_27,x
    lda.b #$28
    sta.b !dp_02
    sta.b !dp_04
    bit.b #$80
    beq .lCB0B
    lda.b #$FF
    sta.b !dp_05
    bra .lCB0D
.lCB0B:
    stz.b !dp_05
.lCB0D:
    lda.b #$0A
    sta.b !dp_08
    sta.b !actor_0a
    bit.b #$80
    beq .lCB1D
    lda.b #$FF
    sta.b !dp_0b
    bra .lCB1F
.lCB1D:
    stz.b !dp_0b
.lCB1F:
    lda.b #$00
    sta.b !dp_94
    sta.w !rot_z
    bit.b #$80
    beq .lCB31
    lda.b #$FF
    sta.w !rot_z_high
    bra .lCB34
.lCB31:
    stz.w !rot_z_high
.lCB34:
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
    asl a
    asl a
    sta.b !dp_04
    lda.b !actor_0a
    asl a
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
    lda.w !obj_15,y
    sta.w !obj_15,x
    lda.w !obj_5a,x
    clc
    adc.b #$01
    bmi .lCBB1
    clc
    adc.b #$06
.lCBB1:
    and.b #$7F
    cmp.b #$06
    bcc .lCBBA
    sec
    sbc.b #$06
.lCBBA:
    ora.b #$80
    sta.w !obj_5a,x
    jsl.l ObjectRebucket
    rtl

; @routine 09:CBC4 ObjectSpinnerCentreStep
; WHAT IT DOES: The spinner centre's frame: scale +$15 down by 8, +$14 up by
; $0C.
; HOW: ADCs; ObjectRebucket.
; Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectRebucket leaves it.
%at($09CBC4)
ObjectSpinnerCentreStep:
    lda.w !obj_15,x
    clc
    adc.b #$F8
    sta.w !obj_15,x
    lda.w !obj_14,x
    clc
    adc.b #$0C
    sta.w !obj_14,x
    jsl.l ObjectRebucket
    rtl

; @routine 09:CBDB ObjectBellGate
; WHAT IT DOES: Sets up a bell gate: restart $09:CBF9 (ObjectBellGateRecord),
; frame $09:CC20 (ObjectBellGateStep); its record and frame.
; HOW: Word and byte stores.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
; leaves it.
%at($09CBDB)
ObjectBellGate:
    rep.b #$20
    lda.w #$CBF9
    sta.w !obj_1d,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1f,x
    rep.b #$20
    lda.w #$CC20
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,x

; @routine 09:CBF9 ObjectBellGateRecord
; WHAT IT DOES: The bell gate's restart: its record ($42D, flag bit 4, at
; ($F00, $F00, $F00)), then its frame.
; HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition.
; Entry: as ObjectBellGate. Exit: as ObjectBellGate.
%at($09CBF9)
ObjectBellGateRecord:
    jsl.l GsuRecordNew42D
    jsl.l GsuRecordFlag10
    rep.b #$20
    lda.w #$0F00
    sta.b !dp_02
    sep.b #$20
    rep.b #$20
    lda.w #$0F00
    sta.b !dp_08
    sep.b #$20
    rep.b #$20
    lda.w #$0F00
    sta.b !dp_94
    sep.b #$20
    jsl.l GsuRecordPosition

; @routine 09:CC20 ObjectBellGateStep
; WHAT IT DOES: The bell gate's frame: a car's hit, the first time (+$25 bit
; 0), plays sound $2E; no hit clears the bit.
; HOW: ObjectSyncFromRecord, RecordContactSummary.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
; it.
%at($09CC20)
ObjectBellGateStep:
    jsl.l ObjectSyncFromRecord
    and.b #$01
    beq .lCC4A
    jsl.l RecordContactSummary
    and.b #$01
    beq .lCC4A
    lda.w !obj_25,x
    and.b #$01
    beq .lCC3B
    jml.l .lCC49
.lCC3B:
    lda.w !obj_25,x
    ora.b #$01
    sta.w !obj_25,x
    lda.b #$2E
    jsl.l SoundPort0QueuePush
.lCC49:
    rtl
.lCC4A:
    lda.w !obj_25,x
    and.b #$FE
    sta.w !obj_25,x
    rtl

; @routine 09:CC53 ObjectLapSignGate
; WHAT IT DOES: Sets up the lap sign gate: restart $09:CC76
; (ObjectLapSignGateRecord), frame $09:CC9D (ObjectLapSignGateStep), +$48 =
; 0; its record and frame.
; HOW: Word and byte stores. Reached only by unit_objects_09c.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
; leaves it.
%at($09CC53)
ObjectLapSignGate:
    rep.b #$20
    lda.w #$CC76
    sta.w !obj_1d,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1f,x
    rep.b #$20
    lda.w #$CC9D
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,x
    lda.b #$00
    sta.w !obj_48,x

; @routine 09:CC76 ObjectLapSignGateRecord
; WHAT IT DOES: The gate's restart: its record ($42D, flag bit 4, at ($F00,
; $F00, $F00)), then its frame.
; HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition. Reached only
; by unit_objects_09c.
; Entry: as ObjectLapSignGate. Exit: as ObjectLapSignGate.
%at($09CC76)
ObjectLapSignGateRecord:
    jsl.l GsuRecordNew42D
    jsl.l GsuRecordFlag10
    rep.b #$20
    lda.w #$0F00
    sta.b !dp_02
    sep.b #$20
    rep.b #$20
    lda.w #$0F00
    sta.b !dp_08
    sep.b #$20
    rep.b #$20
    lda.w #$0F00
    sta.b !dp_94
    sep.b #$20
    jsl.l GsuRecordPosition

; @routine 09:CC9D ObjectLapSignGateStep
; WHAT IT DOES: The gate's frame: a car's hit on a lap ($0DEA) it has not
; counted (+$48) counts it and, on laps 0, 1 and 2, puts up that lap's
; sign (LapSignSpawn0-2).
; HOW: ObjectSyncFromRecord, RecordContactSummary; the laps compared
; through $1F58; JSRs. Reached only by unit_objects_09c.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
; it.
%at($09CC9D)
ObjectLapSignGateStep:
    jsl.l ObjectSyncFromRecord
    and.b #$01
    beq .lCCEE
    jsl.l RecordContactSummary
    and.b #$01
    beq .lCCEE
    lda.w !obj_48,x
    sta.w !wram_1F58
    sep.b #$20
    lda.w !wram_0DEA
    cmp.w !wram_1F58
    beq .lCCEE
    sep.b #$20
    lda.w !wram_0DEA
    sta.w !wram_1F58
    lda.w !wram_1F58
    sta.w !obj_48,x
    sep.b #$20
    lda.w !wram_0DEA
    cmp.b #$00
    beq .lCCDF
    cmp.b #$01
    beq .lCCE5
    cmp.b #$02
    beq .lCCEB
    jmp.w .lCCEE
.lCCDF:
    jsr.w LapSignSpawn0
    jmp.w .lCCEE
.lCCE5:
    jsr.w LapSignSpawn1
    jmp.w .lCCEE
.lCCEB:
    jsr.w LapSignSpawn2
.lCCEE:
    rtl

; @routine 09:CCEF LapSignSpawn0
; WHAT IT DOES: The lap sign gate's lap 0 sign: kind $ABD8 moved ($A00,
; -$100, $140), life $14, handler $09:CDF4 (LapSignStep0); nothing without
; a free object.
; HOW: ObjectSpawn, ObjectCopyPositionTo, 16-bit ADCs. Reached only by
; unit_objects_09c.
; Entry: M 8-bit, X 16-bit; X = the gate. Exit: as the last store or
; ObjectSpawn leaves it.
%at($09CCEF)
LapSignSpawn0:
    rep.b #$20
    lda.w #$ABD8
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .lCD03
    jml.l .lCD45
.lCD03:
    jsl.l ObjectCopyPositionTo
    rep.b #$20
    lda.w !obj_pos_x,y
    clc
    adc.w #$0A00
    sta.w !obj_pos_x,y
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_y,y
    clc
    adc.w #$FF00
    sta.w !obj_pos_y,y
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_z,y
    clc
    adc.w #$0140
    sta.w !obj_pos_z,y
    sep.b #$20
    lda.b #$14
    sta.w !obj_09+1,y
    rep.b #$20
    lda.w #$CDF4
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,y
.lCD45:
    rts

; @routine 09:CD46 LapSignSpawn1
; WHAT IT DOES: The lap 1 sign: kind $ABD8 moved (-$A00, -$100, $140), life
; $14, handler $09:CE30 (LapSignStep1).
; HOW: As LapSignSpawn0. Reached only by unit_objects_09c.
; Entry: as LapSignSpawn0. Exit: as LapSignSpawn0.
%at($09CD46)
LapSignSpawn1:
    rep.b #$20
    lda.w #$ABD8
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .lCD5A
    jml.l .lCD9C
.lCD5A:
    jsl.l ObjectCopyPositionTo
    rep.b #$20
    lda.w !obj_pos_x,y
    clc
    adc.w #$F600
    sta.w !obj_pos_x,y
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_y,y
    clc
    adc.w #$FF00
    sta.w !obj_pos_y,y
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_z,y
    clc
    adc.w #$0140
    sta.w !obj_pos_z,y
    sep.b #$20
    lda.b #$14
    sta.w !obj_09+1,y
    rep.b #$20
    lda.w #$CE30
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,y
.lCD9C:
    rts

; @routine 09:CD9D LapSignSpawn2
; WHAT IT DOES: The lap 2 sign: kind $ABD8 at the gate (moved by 0), life
; $14, handler $09:CE6C (LapSignStep2).
; HOW: As LapSignSpawn0. Reached only by unit_objects_09c.
; Entry: as LapSignSpawn0. Exit: as LapSignSpawn0.
%at($09CD9D)
LapSignSpawn2:
    rep.b #$20
    lda.w #$ABD8
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .lCDB1
    jml.l .lCDF3
.lCDB1:
    jsl.l ObjectCopyPositionTo
    rep.b #$20
    lda.w !obj_pos_x,y
    clc
    adc.w #$0000
    sta.w !obj_pos_x,y
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_y,y
    clc
    adc.w #$0000
    sta.w !obj_pos_y,y
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_z,y
    clc
    adc.w #$0000
    sta.w !obj_pos_z,y
    sep.b #$20
    lda.b #$14
    sta.w !obj_09+1,y
    rep.b #$20
    lda.w #$CE6C
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,y
.lCDF3:
    rts

; @routine 09:CDF4 LapSignStep0
; WHAT IT DOES: The lap 0 sign's frame: it slides -$E0 sideways and is
; released when its life +$0A runs out.
; HOW: 16-bit ADCs (two of 0), DEC, ObjectRebucket. Reached only by
; unit_objects_09c.
; Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectRebucket leaves it.
%at($09CDF4)
LapSignStep0:
    rep.b #$20
    lda.w !obj_pos_x,x
    clc
    adc.w #$FF20
    sta.w !obj_pos_x,x
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_y,x
    clc
    adc.w #$0000
    sta.w !obj_pos_y,x
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_z,x
    clc
    adc.w #$0000
    sta.w !obj_pos_z,x
    sep.b #$20
    dec.w !obj_09+1,x
    beq .lCE27
    jml.l .lCE2B
.lCE27:
    jsl.l ObjectRelease
.lCE2B:
    jsl.l ObjectRebucket
    rtl

; @routine 09:CE30 LapSignStep1
; WHAT IT DOES: The lap 1 sign's frame: it slides $E0 sideways and is
; released when its life +$0A runs out.
; HOW: As LapSignStep0. Reached only by unit_objects_09c.
; Entry: as LapSignStep0. Exit: as LapSignStep0.
%at($09CE30)
LapSignStep1:
    rep.b #$20
    lda.w !obj_pos_x,x
    clc
    adc.w #$00E0
    sta.w !obj_pos_x,x
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_y,x
    clc
    adc.w #$0000
    sta.w !obj_pos_y,x
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_z,x
    clc
    adc.w #$0000
    sta.w !obj_pos_z,x
    sep.b #$20
    dec.w !obj_09+1,x
    beq .lCE63
    jml.l .lCE67
.lCE63:
    jsl.l ObjectRelease
.lCE67:
    jsl.l ObjectRebucket
    rtl

; @routine 09:CE6C LapSignStep2
; WHAT IT DOES: The lap 2 sign's frame: it rises $20 and is released when
; its life +$0A runs out.
; HOW: As LapSignStep0. Reached only by unit_objects_09c.
; Entry: as LapSignStep0. Exit: as LapSignStep0.
%at($09CE6C)
LapSignStep2:
    rep.b #$20
    lda.w !obj_pos_x,x
    clc
    adc.w #$0000
    sta.w !obj_pos_x,x
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_y,x
    clc
    adc.w #$FFE0
    sta.w !obj_pos_y,x
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_z,x
    clc
    adc.w #$0000
    sta.w !obj_pos_z,x
    sep.b #$20
    dec.w !obj_09+1,x
    beq .lCE9F
    jml.l .lCEA3
.lCE9F:
    jsl.l ObjectRelease
.lCEA3:
    jsl.l ObjectRebucket
    rtl

; @routine 09:CEA8 ObjectFan
; WHAT IT DOES: Sets up a fan blade circling a centre: the centre (kind
; $A040, handler $09:CFD5 ObjectFanCentreStep), its frame $09:CEE8
; (ObjectFanStep); then that frame.
; HOW: ObjectSpawn, ObjectCopyPositionTo; +$27 = the centre.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
; leaves it.
%at($09CEA8)
ObjectFan:
    rep.b #$20
    lda.w #$A040
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .lCEBC
    jml.l .lCED9
.lCEBC:
    jsl.l ObjectCopyPositionTo
    pha
    rep.b #$20
    tya
    sta.w !obj_27,x
    sep.b #$20
    pla
    rep.b #$20
    lda.w #$CFD5
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,y
.lCED9:
    rep.b #$20
    lda.w #$CEE8
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,x

; @routine 09:CEE8 ObjectFanStep
; WHAT IT DOES: The blade's frame: at its centre (+$27) plus ($64, $46, 0)
; turned by the centre's angle +$16, times 32, with that angle; at angles
; from $3B to $BF (signed compares with $3A and $C0) it shows model $AF04
; (+$25 bit 0 set), otherwise $A040 (bit 0 cleared); its angle then steps
; by $AA.
; HOW: RotateXY only; five ASLs; ObjectRebucket. The sign bytes come from
; constants: their $FF stores (09:CEF5, 09:CF07, 09:CF1A) never run.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as ObjectRebucket
; leaves it.
%at($09CEE8)
ObjectFanStep:
    ldy.w !obj_27,x
    lda.b #$64
    sta.b !dp_02
    sta.b !dp_04
    bit.b #$80
    beq .lCEFB
    lda.b #$FF
    sta.b !dp_05
    bra .lCEFD
.lCEFB:
    stz.b !dp_05
.lCEFD:
    lda.b #$46
    sta.b !dp_08
    sta.b !actor_0a
    bit.b #$80
    beq .lCF0D
    lda.b #$FF
    sta.b !dp_0b
    bra .lCF0F
.lCF0D:
    stz.b !dp_0b
.lCF0F:
    lda.b #$00
    sta.b !dp_94
    sta.w !rot_z
    bit.b #$80
    beq .lCF21
    lda.b #$FF
    sta.w !rot_z_high
    bra .lCF24
.lCF21:
    stz.w !rot_z_high
.lCF24:
    lda.w !obj_16,y
    jsl.l RotateXY
    lda.b !dp_04
    sta.b !dp_02
    lda.b !actor_0a
    sta.b !dp_08
    lda.w !rot_z
    sta.b !dp_94
    rep.b #$20
    lda.b !dp_04
    asl a
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
    asl a
    sta.b !actor_0a
    lda.w !rot_z
    asl a
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
    lda.w !obj_16,x
    cmp.b #$3A
    beq .lCF88
    bmi .lCF88
    jml.l .lCF9D
.lCF88:
    rep.b #$20
    lda.w #$A040
    sta.w !obj_04,x
    sep.b #$20
    lda.w !obj_25,x
    and.b #$FE
    sta.w !obj_25,x
    jmp.w .lCFC7
.lCF9D:
    lda.w !obj_16,x
    cmp.b #$C0
    beq .lCFAA
    bmi .lCFAA
    jml.l .lCF88
.lCFAA:
    rep.b #$20
    lda.w #$AF04
    sta.w !obj_04,x
    sep.b #$20
    lda.w !obj_25,x
    and.b #$01
    beq .lCFBF
    jml.l .lCFC7
.lCFBF:
    lda.w !obj_25,x
    ora.b #$01
    sta.w !obj_25,x
.lCFC7:
    lda.w !obj_16,x
    clc
    adc.b #$AA
    sta.w !obj_16,x
    jsl.l ObjectRebucket
    rtl

; @routine 09:CFD5 ObjectFanCentreStep
; WHAT IT DOES: The fan centre's frame: its angle +$16 turns by -8.
; HOW: ADC; ObjectRebucket.
; Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectRebucket leaves it.
%at($09CFD5)
ObjectFanCentreStep:
    lda.w !obj_16,x
    clc
    adc.b #$F8
    sta.w !obj_16,x
    jsl.l ObjectRebucket
    rtl

; @routine 09:CFE3 ObjectWheel
; WHAT IT DOES: Sets up a wheel piece circling a centre: the centre (kind
; $A040, handler $09:D131 ObjectWheelCentreStep), its frame $09:D023
; (ObjectWheelStep); then that frame.
; HOW: ObjectSpawn, ObjectCopyPositionTo; +$27 = the centre.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
; leaves it.
%at($09CFE3)
ObjectWheel:
    rep.b #$20
    lda.w #$A040
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .lCFF7
    jml.l .lD014
.lCFF7:
    jsl.l ObjectCopyPositionTo
    pha
    rep.b #$20
    tya
    sta.w !obj_27,x
    sep.b #$20
    pla
    rep.b #$20
    lda.w #$D131
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,y
.lD014:
    rep.b #$20
    lda.w #$D023
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,x

; @routine 09:D023 ObjectWheelStep
; WHAT IT DOES: The wheel piece's frame: as ObjectFanStep, turned three ways
; (+$16, +$14, +$15), lit as model $AF20 with sound $38 the first time.
; HOW: RotateXY, RotateXZFrom08, RotateXZ; five ASLs; ObjectRebucket. The
; sign bytes come from constants: their $FF stores (09:D030, 09:D042,
; 09:D055) never run.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as ObjectRebucket
; leaves it.
%at($09D023)
ObjectWheelStep:
    ldy.w !obj_27,x
    lda.b #$64
    sta.b !dp_02
    sta.b !dp_04
    bit.b #$80
    beq .lD036
    lda.b #$FF
    sta.b !dp_05
    bra .lD038
.lD036:
    stz.b !dp_05
.lD038:
    lda.b #$46
    sta.b !dp_08
    sta.b !actor_0a
    bit.b #$80
    beq .lD048
    lda.b #$FF
    sta.b !dp_0b
    bra .lD04A
.lD048:
    stz.b !dp_0b
.lD04A:
    lda.b #$00
    sta.b !dp_94
    sta.w !rot_z
    bit.b #$80
    beq .lD05C
    lda.b #$FF
    sta.w !rot_z_high
    bra .lD05F
.lD05C:
    stz.w !rot_z_high
.lD05F:
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
    asl a
    asl a
    sta.b !dp_04
    lda.b !actor_0a
    asl a
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
    lda.w !obj_16,x
    cmp.b #$3A
    beq .lD0DE
    bmi .lD0DE
    jml.l .lD0F3
.lD0DE:
    rep.b #$20
    lda.w #$A040
    sta.w !obj_04,x
    sep.b #$20
    lda.w !obj_25,x
    and.b #$FE
    sta.w !obj_25,x
    jmp.w .lD123
.lD0F3:
    lda.w !obj_16,x
    cmp.b #$C0
    beq .lD100
    bmi .lD100
    jml.l .lD0DE
.lD100:
    rep.b #$20
    lda.w #$AF20
    sta.w !obj_04,x
    sep.b #$20
    lda.w !obj_25,x
    and.b #$01
    beq .lD115
    jml.l .lD123
.lD115:
    lda.w !obj_25,x
    ora.b #$01
    sta.w !obj_25,x
    lda.b #$38
    jsl.l SoundPort0QueuePush
.lD123:
    lda.w !obj_16,x
    clc
    adc.b #$AA
    sta.w !obj_16,x
    jsl.l ObjectRebucket
    rtl

; @routine 09:D131 ObjectWheelCentreStep
; WHAT IT DOES: The wheel centre's frame: its angle +$16 turns by 8.
; HOW: ADC; ObjectRebucket.
; Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectRebucket leaves it.
%at($09D131)
ObjectWheelCentreStep:
    lda.w !obj_16,x
    clc
    adc.b #$08
    sta.w !obj_16,x
    jsl.l ObjectRebucket
    rtl

; @routine 09:D79C ObjectBreakableBurst
; WHAT IT DOES: Bursts the breakable into eight pieces (kind $ABF4, handler
; ObjectBreakablePiece, life $0F) flying out at ($1B3, -5, $302), ($186, -7,
; $2B2), (-$15E, -$0A, 0), (-$1A4, -$0A, $316), ($258, -$0A, $212), (-$127,
; -$14, $19F), (-$1A4, -5, 0) and (-$104, -$0A, -$244); a piece is skipped
; without a free object.
; HOW: Eight ObjectSpawn blocks from object X's position. Reached only by
; unit_objects_09c.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the last piece
; leaves it.
%at($09D79C)
ObjectBreakableBurst:
    rep.b #$20
    lda.w #$ABF4
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .lD7B0
    jml.l .lD7F4
.lD7B0:
    jsl.l ObjectCopyPositionTo
    rep.b #$20
    lda.w #$C4C0
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,y
    rep.b #$20
    lda.w !obj_pos_y,y
    clc
    adc.w #$0000
    sta.w !obj_pos_y,y
    sep.b #$20
    rep.b #$20
    lda.w #$01B3
    sta.w !obj_vel_x,y
    sep.b #$20
    rep.b #$20
    lda.w #$FFFB
    sta.w !obj_vel_y,y
    sep.b #$20
    rep.b #$20
    lda.w #$0302
    sta.w !obj_vel_z,y
    sep.b #$20
    lda.b #$0F
    sta.w !obj_09+1,y
.lD7F4:
    rep.b #$20
    lda.w #$ABF4
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .lD808
    jml.l .lD84C
.lD808:
    jsl.l ObjectCopyPositionTo
    rep.b #$20
    lda.w #$C4C0
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,y
    rep.b #$20
    lda.w !obj_pos_y,y
    clc
    adc.w #$0000
    sta.w !obj_pos_y,y
    sep.b #$20
    rep.b #$20
    lda.w #$0186
    sta.w !obj_vel_x,y
    sep.b #$20
    rep.b #$20
    lda.w #$FFF9
    sta.w !obj_vel_y,y
    sep.b #$20
    rep.b #$20
    lda.w #$02B2
    sta.w !obj_vel_z,y
    sep.b #$20
    lda.b #$0F
    sta.w !obj_09+1,y
.lD84C:
    rep.b #$20
    lda.w #$ABF4
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .lD860
    jml.l .lD8A4
.lD860:
    jsl.l ObjectCopyPositionTo
    rep.b #$20
    lda.w #$C4C0
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,y
    rep.b #$20
    lda.w !obj_pos_y,y
    clc
    adc.w #$0000
    sta.w !obj_pos_y,y
    sep.b #$20
    rep.b #$20
    lda.w #$FEA2
    sta.w !obj_vel_x,y
    sep.b #$20
    rep.b #$20
    lda.w #$FFF6
    sta.w !obj_vel_y,y
    sep.b #$20
    rep.b #$20
    lda.w #$0000
    sta.w !obj_vel_z,y
    sep.b #$20
    lda.b #$0F
    sta.w !obj_09+1,y
.lD8A4:
    rep.b #$20
    lda.w #$ABF4
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .lD8B8
    jml.l .lD8FC
.lD8B8:
    jsl.l ObjectCopyPositionTo
    rep.b #$20
    lda.w #$C4C0
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,y
    rep.b #$20
    lda.w !obj_pos_y,y
    clc
    adc.w #$0000
    sta.w !obj_pos_y,y
    sep.b #$20
    rep.b #$20
    lda.w #$FE5C
    sta.w !obj_vel_x,y
    sep.b #$20
    rep.b #$20
    lda.w #$FFF6
    sta.w !obj_vel_y,y
    sep.b #$20
    rep.b #$20
    lda.w #$0316
    sta.w !obj_vel_z,y
    sep.b #$20
    lda.b #$0F
    sta.w !obj_09+1,y
.lD8FC:
    rep.b #$20
    lda.w #$ABF4
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .lD910
    jml.l .lD954
.lD910:
    jsl.l ObjectCopyPositionTo
    rep.b #$20
    lda.w #$C4C0
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,y
    rep.b #$20
    lda.w !obj_pos_y,y
    clc
    adc.w #$0000
    sta.w !obj_pos_y,y
    sep.b #$20
    rep.b #$20
    lda.w #$0258
    sta.w !obj_vel_x,y
    sep.b #$20
    rep.b #$20
    lda.w #$FFF6
    sta.w !obj_vel_y,y
    sep.b #$20
    rep.b #$20
    lda.w #$0212
    sta.w !obj_vel_z,y
    sep.b #$20
    lda.b #$0F
    sta.w !obj_09+1,y
.lD954:
    rep.b #$20
    lda.w #$ABF4
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .lD968
    jml.l .lD9AC
.lD968:
    jsl.l ObjectCopyPositionTo
    rep.b #$20
    lda.w #$C4C0
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,y
    rep.b #$20
    lda.w !obj_pos_y,y
    clc
    adc.w #$0000
    sta.w !obj_pos_y,y
    sep.b #$20
    rep.b #$20
    lda.w #$FED9
    sta.w !obj_vel_x,y
    sep.b #$20
    rep.b #$20
    lda.w #$FFEC
    sta.w !obj_vel_y,y
    sep.b #$20
    rep.b #$20
    lda.w #$019F
    sta.w !obj_vel_z,y
    sep.b #$20
    lda.b #$0F
    sta.w !obj_09+1,y
.lD9AC:
    rep.b #$20
    lda.w #$ABF4
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .lD9C0
    jml.l .lDA04
.lD9C0:
    jsl.l ObjectCopyPositionTo
    rep.b #$20
    lda.w #$C4C0
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,y
    rep.b #$20
    lda.w !obj_pos_y,y
    clc
    adc.w #$0000
    sta.w !obj_pos_y,y
    sep.b #$20
    rep.b #$20
    lda.w #$FE5C
    sta.w !obj_vel_x,y
    sep.b #$20
    rep.b #$20
    lda.w #$FFFB
    sta.w !obj_vel_y,y
    sep.b #$20
    rep.b #$20
    lda.w #$0000
    sta.w !obj_vel_z,y
    sep.b #$20
    lda.b #$0F
    sta.w !obj_09+1,y
.lDA04:
    rep.b #$20
    lda.w #$ABF4
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .lDA18
    jml.l .lDA5C
.lDA18:
    jsl.l ObjectCopyPositionTo
    rep.b #$20
    lda.w #$C4C0
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,y
    rep.b #$20
    lda.w !obj_pos_y,y
    clc
    adc.w #$0000
    sta.w !obj_pos_y,y
    sep.b #$20
    rep.b #$20
    lda.w #$FEFC
    sta.w !obj_vel_x,y
    sep.b #$20
    rep.b #$20
    lda.w #$FFF6
    sta.w !obj_vel_y,y
    sep.b #$20
    rep.b #$20
    lda.w #$FDBC
    sta.w !obj_vel_z,y
    sep.b #$20
    lda.b #$0F
    sta.w !obj_09+1,y
.lDA5C:
    rtl

; @routine 09:D13F ObjectFinaleGate
; WHAT IT DOES: Sets up the finale gate: restart $09:D15D
; (ObjectFinaleGateRecord), frame $09:D184 (ObjectFinaleGateStep); its
; record and frame.
; HOW: Word and byte stores.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
; leaves it.
%at($09D13F)
ObjectFinaleGate:
    rep.b #$20
    lda.w #$D15D
    sta.w !obj_1d,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1f,x
    rep.b #$20
    lda.w #$D184
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,x

; @routine 09:D15D ObjectFinaleGateRecord
; WHAT IT DOES: The finale gate's restart: its record ($42D, flag bit 4, at
; ($2A80, $2A80, $2A80)), then its frame.
; HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition.
; Entry: as ObjectFinaleGate. Exit: as ObjectFinaleGate.
%at($09D15D)
ObjectFinaleGateRecord:
    jsl.l GsuRecordNew42D
    jsl.l GsuRecordFlag10
    rep.b #$20
    lda.w #$2A80
    sta.b !dp_02
    sep.b #$20
    rep.b #$20
    lda.w #$2A80
    sta.b !dp_08
    sep.b #$20
    rep.b #$20
    lda.w #$2A80
    sta.b !dp_94
    sep.b #$20
    jsl.l GsuRecordPosition

; @routine 09:D184 ObjectFinaleGateStep
; WHAT IT DOES: The finale gate's frame: a car's hit on lap 2 ($0DEA) puts
; up the rails (RailsSpawn), plays sound $33 and releases the gate.
; HOW: ObjectSyncFromRecord, RecordContactSummary; JSR RailsSpawn.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
; it.
%at($09D184)
ObjectFinaleGateStep:
    jsl.l ObjectSyncFromRecord
    and.b #$01
    beq .lD1AA
    jsl.l RecordContactSummary
    and.b #$01
    beq .lD1AA
    sep.b #$20
    lda.w !wram_0DEA
    cmp.b #$02
    bne .lD1AA
    jsr.w RailsSpawn
    lda.b #$33
    jsl.l SoundPort0QueuePush
    jsl.l ObjectRelease
.lD1AA:
    rtl

; @routine 09:D1AB RailsSpawn
; WHAT IT DOES: Puts up the five rails of the finale gate: kind $B2D8 at
; object X moved (0, 0), (-$280, -$280), (-$500, 0), (-$640, -$500) and
; (-$8C0, 0) on X and Z, each sliding on to its +$10 + $1180, $1400,
; $1180, $1680 and $1180 (RailStep); a rail is skipped without a free
; object.
; HOW: Five ObjectSpawn blocks.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the last rail
; leaves it.
%at($09D1AB)
RailsSpawn:
    rep.b #$20
    lda.w #$B2D8
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .lD1BF
    jml.l .lD212
.lD1BF:
    lda.b !bank0_base
    ora.b #$80
    sta.w !obj_5a,y
    lda.b #$80
    sta.w !obj_15,y
    jsl.l ObjectCopyPositionTo
    rep.b #$20
    lda.w !obj_pos_x,y
    clc
    adc.w #$0000
    sta.w !obj_pos_x,y
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_z,y
    clc
    adc.w #$0000
    sta.w !obj_pos_z,y
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_z,y
    sta.w !obj_27,y
    sep.b #$20
    rep.b #$20
    lda.w !obj_27,y
    clc
    adc.w #$1180
    sta.w !obj_27,y
    sep.b #$20
    rep.b #$20
    lda.w #$D3AF
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,y
.lD212:
    rep.b #$20
    lda.w #$B2D8
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .lD226
    jml.l .lD279
.lD226:
    lda.b !bank0_base
    ora.b #$80
    sta.w !obj_5a,y
    lda.b #$80
    sta.w !obj_15,y
    jsl.l ObjectCopyPositionTo
    rep.b #$20
    lda.w !obj_pos_x,y
    clc
    adc.w #$FD80
    sta.w !obj_pos_x,y
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_z,y
    clc
    adc.w #$FD80
    sta.w !obj_pos_z,y
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_z,y
    sta.w !obj_27,y
    sep.b #$20
    rep.b #$20
    lda.w !obj_27,y
    clc
    adc.w #$1400
    sta.w !obj_27,y
    sep.b #$20
    rep.b #$20
    lda.w #$D3AF
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,y
.lD279:
    rep.b #$20
    lda.w #$B2D8
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .lD28D
    jml.l .lD2E0
.lD28D:
    lda.b !bank0_base
    ora.b #$80
    sta.w !obj_5a,y
    lda.b #$80
    sta.w !obj_15,y
    jsl.l ObjectCopyPositionTo
    rep.b #$20
    lda.w !obj_pos_x,y
    clc
    adc.w #$FB00
    sta.w !obj_pos_x,y
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_z,y
    clc
    adc.w #$0000
    sta.w !obj_pos_z,y
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_z,y
    sta.w !obj_27,y
    sep.b #$20
    rep.b #$20
    lda.w !obj_27,y
    clc
    adc.w #$1180
    sta.w !obj_27,y
    sep.b #$20
    rep.b #$20
    lda.w #$D3AF
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,y
.lD2E0:
    rep.b #$20
    lda.w #$B2D8
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .lD2F4
    jml.l .lD347
.lD2F4:
    lda.b !bank0_base
    ora.b #$80
    sta.w !obj_5a,y
    lda.b #$80
    sta.w !obj_15,y
    jsl.l ObjectCopyPositionTo
    rep.b #$20
    lda.w !obj_pos_x,y
    clc
    adc.w #$F9C0
    sta.w !obj_pos_x,y
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_z,y
    clc
    adc.w #$FB00
    sta.w !obj_pos_z,y
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_z,y
    sta.w !obj_27,y
    sep.b #$20
    rep.b #$20
    lda.w !obj_27,y
    clc
    adc.w #$1680
    sta.w !obj_27,y
    sep.b #$20
    rep.b #$20
    lda.w #$D3AF
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,y
.lD347:
    rep.b #$20
    lda.w #$B2D8
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .lD35B
    jml.l .lD3AE
.lD35B:
    lda.b !bank0_base
    ora.b #$80
    sta.w !obj_5a,y
    lda.b #$80
    sta.w !obj_15,y
    jsl.l ObjectCopyPositionTo
    rep.b #$20
    lda.w !obj_pos_x,y
    clc
    adc.w #$F740
    sta.w !obj_pos_x,y
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_z,y
    clc
    adc.w #$0000
    sta.w !obj_pos_z,y
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_z,y
    sta.w !obj_27,y
    sep.b #$20
    rep.b #$20
    lda.w !obj_27,y
    clc
    adc.w #$1180
    sta.w !obj_27,y
    sep.b #$20
    rep.b #$20
    lda.w #$D3AF
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,y
.lD3AE:
    rts

; @routine 09:D3AF RailStep
; WHAT IT DOES: A rail's frame: its colour +$5A steps through 0-5; it
; slides $280 at a time toward its target +$27 (+$10), and there its frame
; becomes RailDrop.
; HOW: ADC/AND/SBC on the colour; a 16-bit CMP, then ADC or SBC #$280 by
; N; ObjectSyncToRecord; at the target a JML into RailStop.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as
; ObjectSyncToRecord or RailStop leaves it.
%at($09D3AF)
RailStep:
    lda.w !obj_5a,x
    clc
    adc.b #$01
    bmi .lD3BA
    clc
    adc.b #$06
.lD3BA:
    and.b #$7F
    cmp.b #$06
    bcc .lD3C3
    sec
    sbc.b #$06
.lD3C3:
    ora.b #$80
    sta.w !obj_5a,x
    rep.b #$20
    lda.w !obj_27,x
    sta.w !wram_1F4C
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_z,x
    cmp.w !wram_1F4C
    sep.b #$20
    bne .lD3E2
    jml.l RailStop
.lD3E2:
    rep.b #$20
    beq .lD3F2
    bmi .lD3EE
    sec
    sbc.w #$0280
    bra .lD3F2
.lD3EE:
    clc
    adc.w #$0280
.lD3F2:
    sta.w !obj_pos_z,x
    sep.b #$20
    jsl.l ObjectSyncToRecord
    rtl

; @routine 09:D3FC RailStop
; WHAT IT DOES: The rail has reached its target: its frame becomes $09:D40C
; (RailDrop).
; HOW: Word and byte stores.
; Entry: M 8-bit, X 16-bit; X = object. Exit: A low byte = 9.
%at($09D3FC)
RailStop:
    rep.b #$20
    lda.w #$D40C
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,x
    rtl

; @routine 09:D40C RailDrop
; WHAT IT DOES: The rail drops: colour from direct page $00, model +$04 =
; $B2F4, frame $09:D42C (RailDropStep); then that frame.
; HOW: Word and byte stores.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
; leaves it.
%at($09D40C)
RailDrop:
    lda.b !bank0_base
    ora.b #$80
    sta.w !obj_5a,x
    rep.b #$20
    lda.w #$B2F4
    sta.w !obj_04,x
    sep.b #$20
    rep.b #$20
    lda.w #$D42C
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,x

; @routine 09:D42C RailDropStep
; WHAT IT DOES: The dropping rail's frame: its colour counts on, and at 7
; (set to 6) the rail rises back (RailRise); until then it moves $40 on
; (+$10) and syncs to its record.
; HOW: ADC/AND/CMP #7 on the colour; LDA #7, DEC A for the last one; a
; JMP into RailRise.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as
; ObjectSyncToRecord or RailRise leaves it.
%at($09D42C)
RailDropStep:
    lda.w !obj_5a,x
    clc
    adc.b #$01
    bmi .lD437
    clc
    adc.b #$07
.lD437:
    and.b #$7F
    cmp.b #$07
    bcc .lD448
    lda.b #$07
    dec a
    ora.b #$80
    sta.w !obj_5a,x
    jmp.w RailRise
.lD448:
    ora.b #$80
    sta.w !obj_5a,x
    rep.b #$20
    lda.w !obj_pos_z,x
    clc
    adc.w #$0040
    sta.w !obj_pos_z,x
    sep.b #$20
    jsl.l ObjectSyncToRecord
    rtl

; @routine 09:D460 RailRise
; WHAT IT DOES: The rail rises back: its target +$27 = +$10 + $500, the
; colour from direct page $00, model +$04 = $B2D8, frame $09:D498
; (RailRiseStep); then that frame.
; HOW: 16-bit copy and ADC; word and byte stores.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
; leaves it.
%at($09D460)
RailRise:
    rep.b #$20
    lda.w !obj_pos_z,x
    sta.w !obj_27,x
    sep.b #$20
    lda.b !bank0_base
    ora.b #$80
    sta.w !obj_5a,x
    rep.b #$20
    lda.w #$B2D8
    sta.w !obj_04,x
    sep.b #$20
    rep.b #$20
    lda.w !obj_27,x
    clc
    adc.w #$0500
    sta.w !obj_27,x
    sep.b #$20
    rep.b #$20
    lda.w #$D498
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,x

; @routine 09:D498 RailRiseStep
; WHAT IT DOES: The rising rail's frame: its colour steps through 0-5; it
; slides $140 at a time toward its target +$27, and there it is released.
; HOW: As RailStep with $140; at the target ObjectRelease.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as
; ObjectSyncToRecord or ObjectRelease leaves it.
%at($09D498)
RailRiseStep:
    lda.w !obj_5a,x
    clc
    adc.b #$01
    bmi .lD4A3
    clc
    adc.b #$06
.lD4A3:
    and.b #$7F
    cmp.b #$06
    bcc .lD4AC
    sec
    sbc.b #$06
.lD4AC:
    ora.b #$80
    sta.w !obj_5a,x
    rep.b #$20
    lda.w !obj_27,x
    sta.w !wram_1F4C
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_z,x
    cmp.w !wram_1F4C
    sep.b #$20
    bne .lD4CB
    jml.l .lD4E5
.lD4CB:
    rep.b #$20
    beq .lD4DB
    bmi .lD4D7
    sec
    sbc.w #$0140
    bra .lD4DB
.lD4D7:
    clc
    adc.w #$0140
.lD4DB:
    sta.w !obj_pos_z,x
    sep.b #$20
    jsl.l ObjectSyncToRecord
    rtl
.lD4E5:
    jsl.l ObjectRelease
    rtl

; @routine 09:D4EA ObjectScaleUp6
; WHAT IT DOES: Grows object X's scale +$15 by 6.
; HOW: CLC, ADC. Reached only by unit_objects_09d.
; Entry: M 8-bit; X = object. Exit: A low byte = the new scale; N, Z, C, V
; from the add.
%at($09D4EA)
ObjectScaleUp6:
    lda.w !obj_15,x
    clc
    adc.b #$06
    sta.w !obj_15,x
    rtl

; @routine 09:D4F4 ObjectBreakableGate
; WHAT IT DOES: Sets up the breakable gate: restart $09:D517
; (ObjectBreakableGateRecord), frame $09:D53E (ObjectBreakableGateStep),
; +$0A = 4 (no lap counted); its record and frame.
; HOW: Word and byte stores.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
; leaves it.
%at($09D4F4)
ObjectBreakableGate:
    rep.b #$20
    lda.w #$D517
    sta.w !obj_1d,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1f,x
    rep.b #$20
    lda.w #$D53E
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,x
    lda.b #$04
    sta.w !obj_09+1,x

; @routine 09:D517 ObjectBreakableGateRecord
; WHAT IT DOES: The gate's restart: its record ($42D, flag bit 4, at
; ($1E00, $1E00, $1E00)), then its frame.
; HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition.
; Entry: as ObjectBreakableGate. Exit: as ObjectBreakableGate.
%at($09D517)
ObjectBreakableGateRecord:
    jsl.l GsuRecordNew42D
    jsl.l GsuRecordFlag10
    rep.b #$20
    lda.w #$1E00
    sta.b !dp_02
    sep.b #$20
    rep.b #$20
    lda.w #$1E00
    sta.b !dp_08
    sep.b #$20
    rep.b #$20
    lda.w #$1E00
    sta.b !dp_94
    sep.b #$20
    jsl.l GsuRecordPosition

; @routine 09:D53E ObjectBreakableGateStep
; WHAT IT DOES: The gate's frame: a car's hit on a lap ($0DEA) it has not
; counted (+$0A) counts it, and on lap 2 puts up two breakables
; (BreakablePairA) with sound $53.
; HOW: ObjectSyncFromRecord, RecordContactSummary; the laps compared
; through $1F58.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
; it.
%at($09D53E)
ObjectBreakableGateStep:
    jsl.l ObjectSyncFromRecord
    and.b #$01
    beq .lD58D
    jsl.l RecordContactSummary
    and.b #$01
    beq .lD58D
    lda.w !obj_09+1,x
    sta.w !wram_1F58
    sep.b #$20
    lda.w !wram_0DEA
    cmp.w !wram_1F58
    beq .lD58D
    lda.w !wram_0DEA
    sta.w !wram_1F58
    lda.w !wram_1F58
    sta.w !obj_09+1,x
    sep.b #$20
    lda.w !wram_0DEA
    cmp.b #$00
    beq .lD57E
    cmp.b #$01
    beq .lD581
    cmp.b #$02
    beq .lD584
    jmp.w .lD58D
.lD57E:
    jmp.w .lD58D
.lD581:
    jmp.w .lD58D
.lD584:
    jsr.w BreakablePairA
    lda.b #$53
    jsl.l SoundPort0QueuePush
.lD58D:
    rtl

; @routine 09:D58E BreakablePairA
; WHAT IT DOES: Puts two breakables (ObjectBreakable) by object X: moved
; (+$B40, -$320, -$680) and (+$F00, -$3C0, -$6E0) on X, Y and Z; X's size
; bytes become $5A, then $23.
; HOW: Two ObjectSpawn blocks. Reached only by unit_objects_09d.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the last
; block leaves it.
%at($09D58E)
BreakablePairA:
    rep.b #$20
    lda.w #$B4D0
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .lD5A2
    jml.l .lD611
.lD5A2:
    jsl.l ObjectCopyPositionTo
    rep.b #$20
    lda.w #$C513
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,y
    rep.b #$20
    lda.w !obj_pos_y,y
    clc
    adc.w #$FCE0
    sta.w !obj_pos_y,y
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_z,y
    clc
    adc.w #$F980
    sta.w !obj_pos_z,y
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_x,y
    clc
    adc.w #$0B40
    sta.w !obj_pos_x,y
    sep.b #$20
    rep.b #$20
    lda.w #$0000
    sta.w !obj_vel_x,y
    sep.b #$20
    rep.b #$20
    lda.w #$0000
    sta.w !obj_vel_y,y
    sep.b #$20
    rep.b #$20
    lda.w #$0000
    sta.w !obj_vel_z,y
    sep.b #$20
    lda.b #$5A
    sta.w !obj_14,x
    lda.b #$5A
    sta.w !obj_15,x
    lda.b #$5A
    sta.w !obj_16,x
    lda.b #$1C
    sta.w !obj_09+1,y
.lD611:
    rep.b #$20
    lda.w #$B4D0
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .lD625
    jml.l .lD694
.lD625:
    jsl.l ObjectCopyPositionTo
    rep.b #$20
    lda.w #$C513
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,y
    rep.b #$20
    lda.w !obj_pos_y,y
    clc
    adc.w #$FC40
    sta.w !obj_pos_y,y
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_z,y
    clc
    adc.w #$F920
    sta.w !obj_pos_z,y
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_x,y
    clc
    adc.w #$0F00
    sta.w !obj_pos_x,y
    sep.b #$20
    rep.b #$20
    lda.w #$0000
    sta.w !obj_vel_x,y
    sep.b #$20
    rep.b #$20
    lda.w #$0000
    sta.w !obj_vel_y,y
    sep.b #$20
    rep.b #$20
    lda.w #$0000
    sta.w !obj_vel_z,y
    sep.b #$20
    lda.b #$23
    sta.w !obj_14,x
    lda.b #$23
    sta.w !obj_15,x
    lda.b #$23
    sta.w !obj_16,x
    lda.b #$1C
    sta.w !obj_09+1,y
.lD694:
    rts

; @routine 09:D695 BreakablePairB
; WHAT IT DOES: Puts two breakables by object X: moved (+$500, -$640,
; -$3C0) and (+$B40, -$780, -$280); X's size bytes become $5A, then $23.
; HOW: As BreakablePairA. Nothing in the game reaches it; reached only by
; unit_objects_09d.
; Entry: as BreakablePairA. Exit: as BreakablePairA.
%at($09D695)
BreakablePairB:
    rep.b #$20
    lda.w #$B4D0
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .lD6A9
    jml.l .lD718
.lD6A9:
    jsl.l ObjectCopyPositionTo
    rep.b #$20
    lda.w #$C513
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,y
    rep.b #$20
    lda.w !obj_pos_y,y
    clc
    adc.w #$F9C0
    sta.w !obj_pos_y,y
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_z,y
    clc
    adc.w #$FC40
    sta.w !obj_pos_z,y
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_x,y
    clc
    adc.w #$0500
    sta.w !obj_pos_x,y
    sep.b #$20
    rep.b #$20
    lda.w #$0000
    sta.w !obj_vel_x,y
    sep.b #$20
    rep.b #$20
    lda.w #$0000
    sta.w !obj_vel_y,y
    sep.b #$20
    rep.b #$20
    lda.w #$0000
    sta.w !obj_vel_z,y
    sep.b #$20
    lda.b #$5A
    sta.w !obj_14,x
    lda.b #$5A
    sta.w !obj_15,x
    lda.b #$5A
    sta.w !obj_16,x
    lda.b #$1C
    sta.w !obj_09+1,y
.lD718:
    rep.b #$20
    lda.w #$B4D0
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .lD72C
    jml.l .lD79B
.lD72C:
    jsl.l ObjectCopyPositionTo
    rep.b #$20
    lda.w #$C513
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,y
    rep.b #$20
    lda.w !obj_pos_y,y
    clc
    adc.w #$F880
    sta.w !obj_pos_y,y
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_z,y
    clc
    adc.w #$FD80
    sta.w !obj_pos_z,y
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_x,y
    clc
    adc.w #$0B40
    sta.w !obj_pos_x,y
    sep.b #$20
    rep.b #$20
    lda.w #$0000
    sta.w !obj_vel_x,y
    sep.b #$20
    rep.b #$20
    lda.w #$0000
    sta.w !obj_vel_y,y
    sep.b #$20
    rep.b #$20
    lda.w #$0000
    sta.w !obj_vel_z,y
    sep.b #$20
    lda.b #$23
    sta.w !obj_14,x
    lda.b #$23
    sta.w !obj_15,x
    lda.b #$23
    sta.w !obj_16,x
    lda.b #$1C
    sta.w !obj_09+1,y
.lD79B:
    rts

; @routine 09:DA5D ObjectBreakableBurst2
; WHAT IT DOES: As ObjectBreakableBurst, the pieces alternating kinds $AC10
; and $ABF4 (the last two both $AC10).
; HOW: Eight ObjectSpawn blocks from object X's position.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the last piece
; leaves it.
%at($09DA5D)
ObjectBreakableBurst2:
    rep.b #$20
    lda.w #$AC10
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .lDA71
    jml.l .lDAB5
.lDA71:
    jsl.l ObjectCopyPositionTo
    rep.b #$20
    lda.w #$C4C0
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,y
    rep.b #$20
    lda.w !obj_pos_y,y
    clc
    adc.w #$0000
    sta.w !obj_pos_y,y
    sep.b #$20
    rep.b #$20
    lda.w #$01B3
    sta.w !obj_vel_x,y
    sep.b #$20
    rep.b #$20
    lda.w #$FFFB
    sta.w !obj_vel_y,y
    sep.b #$20
    rep.b #$20
    lda.w #$0302
    sta.w !obj_vel_z,y
    sep.b #$20
    lda.b #$0F
    sta.w !obj_09+1,y
.lDAB5:
    rep.b #$20
    lda.w #$ABF4
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .lDAC9
    jml.l .lDB0D
.lDAC9:
    jsl.l ObjectCopyPositionTo
    rep.b #$20
    lda.w #$C4C0
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,y
    rep.b #$20
    lda.w !obj_pos_y,y
    clc
    adc.w #$0000
    sta.w !obj_pos_y,y
    sep.b #$20
    rep.b #$20
    lda.w #$0186
    sta.w !obj_vel_x,y
    sep.b #$20
    rep.b #$20
    lda.w #$FFF9
    sta.w !obj_vel_y,y
    sep.b #$20
    rep.b #$20
    lda.w #$02B2
    sta.w !obj_vel_z,y
    sep.b #$20
    lda.b #$0F
    sta.w !obj_09+1,y
.lDB0D:
    rep.b #$20
    lda.w #$AC10
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .lDB21
    jml.l .lDB65
.lDB21:
    jsl.l ObjectCopyPositionTo
    rep.b #$20
    lda.w #$C4C0
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,y
    rep.b #$20
    lda.w !obj_pos_y,y
    clc
    adc.w #$0000
    sta.w !obj_pos_y,y
    sep.b #$20
    rep.b #$20
    lda.w #$FEA2
    sta.w !obj_vel_x,y
    sep.b #$20
    rep.b #$20
    lda.w #$FFF6
    sta.w !obj_vel_y,y
    sep.b #$20
    rep.b #$20
    lda.w #$0000
    sta.w !obj_vel_z,y
    sep.b #$20
    lda.b #$0F
    sta.w !obj_09+1,y
.lDB65:
    rep.b #$20
    lda.w #$ABF4
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .lDB79
    jml.l .lDBBD
.lDB79:
    jsl.l ObjectCopyPositionTo
    rep.b #$20
    lda.w #$C4C0
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,y
    rep.b #$20
    lda.w !obj_pos_y,y
    clc
    adc.w #$0000
    sta.w !obj_pos_y,y
    sep.b #$20
    rep.b #$20
    lda.w #$FE5C
    sta.w !obj_vel_x,y
    sep.b #$20
    rep.b #$20
    lda.w #$FFF6
    sta.w !obj_vel_y,y
    sep.b #$20
    rep.b #$20
    lda.w #$0316
    sta.w !obj_vel_z,y
    sep.b #$20
    lda.b #$0F
    sta.w !obj_09+1,y
.lDBBD:
    rep.b #$20
    lda.w #$AC10
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .lDBD1
    jml.l .lDC15
.lDBD1:
    jsl.l ObjectCopyPositionTo
    rep.b #$20
    lda.w #$C4C0
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,y
    rep.b #$20
    lda.w !obj_pos_y,y
    clc
    adc.w #$0000
    sta.w !obj_pos_y,y
    sep.b #$20
    rep.b #$20
    lda.w #$0258
    sta.w !obj_vel_x,y
    sep.b #$20
    rep.b #$20
    lda.w #$FFF6
    sta.w !obj_vel_y,y
    sep.b #$20
    rep.b #$20
    lda.w #$0212
    sta.w !obj_vel_z,y
    sep.b #$20
    lda.b #$0F
    sta.w !obj_09+1,y
.lDC15:
    rep.b #$20
    lda.w #$ABF4
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .lDC29
    jml.l .lDC6D
.lDC29:
    jsl.l ObjectCopyPositionTo
    rep.b #$20
    lda.w #$C4C0
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,y
    rep.b #$20
    lda.w !obj_pos_y,y
    clc
    adc.w #$0000
    sta.w !obj_pos_y,y
    sep.b #$20
    rep.b #$20
    lda.w #$FED9
    sta.w !obj_vel_x,y
    sep.b #$20
    rep.b #$20
    lda.w #$FFEC
    sta.w !obj_vel_y,y
    sep.b #$20
    rep.b #$20
    lda.w #$019F
    sta.w !obj_vel_z,y
    sep.b #$20
    lda.b #$0F
    sta.w !obj_09+1,y
.lDC6D:
    rep.b #$20
    lda.w #$AC10
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .lDC81
    jml.l .lDCC5
.lDC81:
    jsl.l ObjectCopyPositionTo
    rep.b #$20
    lda.w #$C4C0
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,y
    rep.b #$20
    lda.w !obj_pos_y,y
    clc
    adc.w #$0000
    sta.w !obj_pos_y,y
    sep.b #$20
    rep.b #$20
    lda.w #$FE5C
    sta.w !obj_vel_x,y
    sep.b #$20
    rep.b #$20
    lda.w #$FFFB
    sta.w !obj_vel_y,y
    sep.b #$20
    rep.b #$20
    lda.w #$0000
    sta.w !obj_vel_z,y
    sep.b #$20
    lda.b #$0F
    sta.w !obj_09+1,y
.lDCC5:
    rep.b #$20
    lda.w #$AC10
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .lDCD9
    jml.l .lDD1D
.lDCD9:
    jsl.l ObjectCopyPositionTo
    rep.b #$20
    lda.w #$C4C0
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,y
    rep.b #$20
    lda.w !obj_pos_y,y
    clc
    adc.w #$0000
    sta.w !obj_pos_y,y
    sep.b #$20
    rep.b #$20
    lda.w #$FEFC
    sta.w !obj_vel_x,y
    sep.b #$20
    rep.b #$20
    lda.w #$FFF6
    sta.w !obj_vel_y,y
    sep.b #$20
    rep.b #$20
    lda.w #$FDBC
    sta.w !obj_vel_z,y
    sep.b #$20
    lda.b #$0F
    sta.w !obj_09+1,y
.lDD1D:
    rtl

; @routine 09:DD1E ObjectBreakableGate2
; WHAT IT DOES: Sets up the second breakable gate: restart $09:DD41
; (ObjectBreakableGate2Record), frame $09:DD68 (ObjectBreakableGate2Step),
; +$0A = 4; its record and frame.
; HOW: Word and byte stores. Reached only by unit_objects_09d.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
; leaves it.
%at($09DD1E)
ObjectBreakableGate2:
    rep.b #$20
    lda.w #$DD41
    sta.w !obj_1d,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1f,x
    rep.b #$20
    lda.w #$DD68
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,x
    lda.b #$04
    sta.w !obj_09+1,x

; @routine 09:DD41 ObjectBreakableGate2Record
; WHAT IT DOES: The gate's restart: its record ($42D, flag bit 4, at
; ($780, $780, $780)), then its frame.
; HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition. Reached only
; by unit_objects_09d.
; Entry: as ObjectBreakableGate2. Exit: as ObjectBreakableGate2.
%at($09DD41)
ObjectBreakableGate2Record:
    jsl.l GsuRecordNew42D
    jsl.l GsuRecordFlag10
    rep.b #$20
    lda.w #$0780
    sta.b !dp_02
    sep.b #$20
    rep.b #$20
    lda.w #$0780
    sta.b !dp_08
    sep.b #$20
    rep.b #$20
    lda.w #$0780
    sta.b !dp_94
    sep.b #$20
    jsl.l GsuRecordPosition

; @routine 09:DD68 ObjectBreakableGate2Step
; WHAT IT DOES: The gate's frame: a car's hit on a lap ($0DEA) it has not
; counted (+$0A) counts it: on lap 0 a sweep ($0DA5 bit 4) with sound $36
; when bit 4 of the clock fraction $0E63 is clear; on laps 1 and 2 two
; breakables (BreakablePairC, BreakablePairD) with sound $53.
; HOW: ObjectSyncFromRecord, RecordContactSummary; the laps through $1F58;
; $0DA5 by long accesses. Reached only by unit_objects_09d.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
; it.
%at($09DD68)
ObjectBreakableGate2Step:
    jsl.l ObjectSyncFromRecord
    and.b #$01
    beq .lDDDD
    jsl.l RecordContactSummary
    and.b #$01
    beq .lDDDD
    lda.w !obj_09+1,x
    sta.w !wram_1F58
    sep.b #$20
    lda.w !wram_0DEA
    cmp.w !wram_1F58
    beq .lDDDD
    lda.w !wram_0DEA
    sta.w !wram_1F58
    lda.w !wram_1F58
    sta.w !obj_09+1,x
    sep.b #$20
    lda.w !wram_0DEA
    cmp.b #$00
    beq .lDDA8
    cmp.b #$01
    beq .lDDC8
    cmp.b #$02
    beq .lDDD4
    jmp.w .lDDDD
.lDDA8:
    sep.b #$20
    lda.w !race_time_fraction
    lsr a
    lsr a
    lsr a
    lsr a
    and.b #$01
    bne .lDDDD
    lda.l !slide_flags
    ora.b #$10
    sta.l !slide_flags
    lda.b #$36
    jsl.l SoundPort0QueuePush
    jmp.w .lDDDD
.lDDC8:
    jsr.w BreakablePairC
    lda.b #$53
    jsl.l SoundPort0QueuePush
    jmp.w .lDDDD
.lDDD4:
    jsr.w BreakablePairD
    lda.b #$53
    jsl.l SoundPort0QueuePush
.lDDDD:
    rtl

; @routine 09:DDDE BreakablePairC
; WHAT IT DOES: Puts two breakables by object X: moved (-$1180, -$500,
; -$780) and (-$F00, -$5A0, -$6E0); X's size bytes become $5A, then $23.
; HOW: As BreakablePairA. Reached only by unit_objects_09d.
; Entry: as BreakablePairA. Exit: as BreakablePairA.
%at($09DDDE)
BreakablePairC:
    rep.b #$20
    lda.w #$B4D0
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .lDDF2
    jml.l .lDE61
.lDDF2:
    jsl.l ObjectCopyPositionTo
    rep.b #$20
    lda.w #$C513
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,y
    rep.b #$20
    lda.w !obj_pos_y,y
    clc
    adc.w #$FB00
    sta.w !obj_pos_y,y
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_z,y
    clc
    adc.w #$F880
    sta.w !obj_pos_z,y
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_x,y
    clc
    adc.w #$EE80
    sta.w !obj_pos_x,y
    sep.b #$20
    rep.b #$20
    lda.w #$0000
    sta.w !obj_vel_x,y
    sep.b #$20
    rep.b #$20
    lda.w #$0000
    sta.w !obj_vel_y,y
    sep.b #$20
    rep.b #$20
    lda.w #$0000
    sta.w !obj_vel_z,y
    sep.b #$20
    lda.b #$5A
    sta.w !obj_14,x
    lda.b #$5A
    sta.w !obj_15,x
    lda.b #$5A
    sta.w !obj_16,x
    lda.b #$1C
    sta.w !obj_09+1,y
.lDE61:
    rep.b #$20
    lda.w #$B4D0
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .lDE75
    jml.l .lDEE4
.lDE75:
    jsl.l ObjectCopyPositionTo
    rep.b #$20
    lda.w #$C513
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,y
    rep.b #$20
    lda.w !obj_pos_y,y
    clc
    adc.w #$FA60
    sta.w !obj_pos_y,y
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_z,y
    clc
    adc.w #$F920
    sta.w !obj_pos_z,y
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_x,y
    clc
    adc.w #$F100
    sta.w !obj_pos_x,y
    sep.b #$20
    rep.b #$20
    lda.w #$0000
    sta.w !obj_vel_x,y
    sep.b #$20
    rep.b #$20
    lda.w #$0000
    sta.w !obj_vel_y,y
    sep.b #$20
    rep.b #$20
    lda.w #$0000
    sta.w !obj_vel_z,y
    sep.b #$20
    lda.b #$23
    sta.w !obj_14,x
    lda.b #$23
    sta.w !obj_15,x
    lda.b #$23
    sta.w !obj_16,x
    lda.b #$1C
    sta.w !obj_09+1,y
.lDEE4:
    rts

; @routine 09:DEE5 BreakablePairD
; WHAT IT DOES: Puts two breakables by object X: moved (-$1180, -$500,
; -$4C0) and (-$F00, -$5A0, -$5A0); X's size bytes become $5A, then $23.
; HOW: As BreakablePairA. Reached only by unit_objects_09d.
; Entry: as BreakablePairA. Exit: as BreakablePairA.
%at($09DEE5)
BreakablePairD:
    rep.b #$20
    lda.w #$B4D0
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .lDEF9
    jml.l .lDF68
.lDEF9:
    jsl.l ObjectCopyPositionTo
    rep.b #$20
    lda.w #$C513
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,y
    rep.b #$20
    lda.w !obj_pos_y,y
    clc
    adc.w #$FB00
    sta.w !obj_pos_y,y
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_z,y
    clc
    adc.w #$FB40
    sta.w !obj_pos_z,y
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_x,y
    clc
    adc.w #$EE80
    sta.w !obj_pos_x,y
    sep.b #$20
    rep.b #$20
    lda.w #$0000
    sta.w !obj_vel_x,y
    sep.b #$20
    rep.b #$20
    lda.w #$0000
    sta.w !obj_vel_y,y
    sep.b #$20
    rep.b #$20
    lda.w #$0000
    sta.w !obj_vel_z,y
    sep.b #$20
    lda.b #$5A
    sta.w !obj_14,x
    lda.b #$5A
    sta.w !obj_15,x
    lda.b #$5A
    sta.w !obj_16,x
    lda.b #$1C
    sta.w !obj_09+1,y
.lDF68:
    rep.b #$20
    lda.w #$B4D0
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .lDF7C
    jml.l .lDFEB
.lDF7C:
    jsl.l ObjectCopyPositionTo
    rep.b #$20
    lda.w #$C513
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,y
    rep.b #$20
    lda.w !obj_pos_y,y
    clc
    adc.w #$FA60
    sta.w !obj_pos_y,y
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_z,y
    clc
    adc.w #$FA60
    sta.w !obj_pos_z,y
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_x,y
    clc
    adc.w #$F100
    sta.w !obj_pos_x,y
    sep.b #$20
    rep.b #$20
    lda.w #$0000
    sta.w !obj_vel_x,y
    sep.b #$20
    rep.b #$20
    lda.w #$0000
    sta.w !obj_vel_y,y
    sep.b #$20
    rep.b #$20
    lda.w #$0000
    sta.w !obj_vel_z,y
    sep.b #$20
    lda.b #$23
    sta.w !obj_14,x
    lda.b #$23
    sta.w !obj_15,x
    lda.b #$23
    sta.w !obj_16,x
    lda.b #$1C
    sta.w !obj_09+1,y
.lDFEB:
    rts

; @routine 09:DFEC ObjectDuoGate
; WHAT IT DOES: Sets up the two-player gate: restart $09:E00A
; (ObjectDuoGateRecord), frame $09:E018 (ObjectDuoGateStep); its record
; and frame.
; HOW: Word and byte stores.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
; leaves it.
%at($09DFEC)
ObjectDuoGate:
    rep.b #$20
    lda.w #$E00A
    sta.w !obj_1d,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1f,x
    rep.b #$20
    lda.w #$E018
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,x

; @routine 09:E00A ObjectDuoGateRecord
; WHAT IT DOES: The gate's restart: its record (GsuRecordNew424Alt, flag
; bit 4, +$2A high $14), then its frame.
; HOW: GsuRecordNew424Alt, GsuRecordFlag10, GsuRecordSet2AHigh.
; Entry: as ObjectDuoGate. Exit: as ObjectDuoGate.
%at($09E00A)
ObjectDuoGateRecord:
    jsl.l GsuRecordNew424Alt
    jsl.l GsuRecordFlag10
    lda.b #$14
    jsl.l GsuRecordSet2AHigh

; @routine 09:E018 ObjectDuoGateStep
; WHAT IT DOES: The two-player gate's frame: with contacts, the contact
; summary's bit 0, the first time (+$25 bit 0), plays sound $5A for player
; 0 ($1A5D = 0) and its bit 1, the first time (+$25 bit 1), for player 1;
; a bit whose contact is gone is cleared, and both with no contacts.
; HOW: ObjectSyncFromRecord, RecordContactSummary twice.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
; it.
%at($09E018)
ObjectDuoGateStep:
    jsl.l ObjectSyncFromRecord
    and.b #$01
    beq .lE086
    jsl.l RecordContactSummary
    and.b #$01
    bne .lE033
    lda.w !obj_25,x
    and.b #$FE
    sta.w !obj_25,x
    jmp.w .lE053
.lE033:
    lda.w !obj_25,x
    and.b #$01
    beq .lE03E
    jml.l .lE053
.lE03E:
    lda.w !obj_25,x
    ora.b #$01
    sta.w !obj_25,x
    sep.b #$20
    lda.b #$00
    sta.w !current_player
    lda.b #$5A
    jsl.l SoundPort0QueuePush
.lE053:
    jsl.l RecordContactSummary
    and.b #$02
    bne .lE065
    lda.w !obj_25,x
    and.b #$FD
    sta.w !obj_25,x
    bra .lE085
.lE065:
    lda.w !obj_25,x
    and.b #$02
    beq .lE070
    jml.l .lE085
.lE070:
    lda.w !obj_25,x
    ora.b #$02
    sta.w !obj_25,x
    sep.b #$20
    lda.b #$01
    sta.w !current_player
    lda.b #$5A
    jsl.l SoundPort0QueuePush
.lE085:
    rtl
.lE086:
    lda.w !obj_25,x
    and.b #$FE
    sta.w !obj_25,x
    lda.w !obj_25,x
    and.b #$FD
    sta.w !obj_25,x
    rtl

; @routine 09:E097 ObjectBurstGate
; WHAT IT DOES: Sets up a burst gate: restart $09:E0BA
; (ObjectBurstGateRecord), frame $09:E0C8 (ObjectBurstGateStep), +$14 =
; direct page $80; its record and frame.
; HOW: Word and byte stores.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
; leaves it.
%at($09E097)
ObjectBurstGate:
    rep.b #$20
    lda.w #$E0BA
    sta.w !obj_1d,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1f,x
    rep.b #$20
    lda.w #$E0C8
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,x
    lda.b !heading_cos_a+1
    sta.w !obj_14,x

; @routine 09:E0BA ObjectBurstGateRecord
; WHAT IT DOES: The gate's restart: its record (GsuRecordNew424Alt, flag
; bit 4, +$2A high $14), then its frame.
; HOW: GsuRecordNew424Alt, GsuRecordFlag10, GsuRecordSet2AHigh.
; Entry: as ObjectBurstGate. Exit: as ObjectBurstGate.
%at($09E0BA)
ObjectBurstGateRecord:
    jsl.l GsuRecordNew424Alt
    jsl.l GsuRecordFlag10
    lda.b #$14
    jsl.l GsuRecordSet2AHigh

; @routine 09:E0C8 ObjectBurstGateStep
; WHAT IT DOES: The burst gate's frame: a car's hit, the first time (+$25
; bit 0), bursts it into eight pieces (ObjectBreakableBurst2) with sound
; $5A; no hit clears the bit.
; HOW: ObjectSyncFromRecord, RecordContactSummary.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
; it.
%at($09E0C8)
ObjectBurstGateStep:
    jsl.l ObjectSyncFromRecord
    and.b #$01
    beq .lE0F6
    jsl.l RecordContactSummary
    and.b #$01
    beq .lE0F6
    lda.w !obj_25,x
    and.b #$01
    beq .lE0E3
    jml.l .lE0F5
.lE0E3:
    lda.w !obj_25,x
    ora.b #$01
    sta.w !obj_25,x
    jsl.l ObjectBreakableBurst2
    lda.b #$5A
    jsl.l SoundPort0QueuePush
.lE0F5:
    rtl
.lE0F6:
    lda.w !obj_25,x
    and.b #$FE
    sta.w !obj_25,x
    rtl

; @routine 09:E0FF ObjectLampPost
; WHAT IT DOES: Sets up a lamp post: $8C higher, lit by $9971 (+$24 bit 3,
; +$3A = 0, +$5F = $64), frame $09:E185 (ObjectLampPostLight), and its lamp
; (kind $A0E8, lit by $9973) $26C above it; no lamp without a free object.
; HOW: ObjectSpawn, ObjectCopyPositionTo; the lamp rebucketed with X and
; Y swapped (PHX, TYX, PLY) around ObjectRebucket. Reached only by
; unit_objects_09d.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the last swap
; or ObjectSpawn leaves it.
%at($09E0FF)
ObjectLampPost:
    rep.b #$20
    lda.w !obj_pos_y,x
    clc
    adc.w #$FF74
    sta.w !obj_pos_y,x
    sep.b #$20
    rep.b #$20
    lda.w #$9971
    sta.w !obj_5d,x
    sep.b #$20
    lda.w !obj_24,x
    ora.b #$08
    sta.w !obj_24,x
    lda.b #$00
    sta.w !obj_3a,x
    lda.b #$64
    sta.w !obj_5f,x
    rep.b #$20
    lda.w #$E185
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,x
    rep.b #$20
    lda.w #$A0E8
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .lE14C
    jml.l .lE184
.lE14C:
    lda.w !obj_24,y
    ora.b #$08
    sta.w !obj_24,y
    lda.b #$00
    sta.w !obj_3a,y
    lda.b #$64
    sta.w !obj_5f,y
    rep.b #$20
    lda.w #$9973
    sta.w !obj_5d,y
    sep.b #$20
    jsl.l ObjectCopyPositionTo
    rep.b #$20
    lda.w !obj_pos_y,y
    clc
    adc.w #$FD94
    sta.w !obj_pos_y,y
    sep.b #$20
    phx
    tyx
    ply
    jsl.l ObjectRebucket
    phx
    tyx
    ply
.lE184:
    rtl

; @routine 09:E185 ObjectLampPostLight
; WHAT IT DOES: The lamp post's first frame: restart $09:E1A3
; (ObjectLampPostRecord), frame $09:E1B1 (ObjectLampPostStep); its record
; and frame.
; HOW: Word and byte stores. Reached only by unit_objects_09d.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
; leaves it.
%at($09E185)
ObjectLampPostLight:
    rep.b #$20
    lda.w #$E1A3
    sta.w !obj_1d,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1f,x
    rep.b #$20
    lda.w #$E1B1
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,x

; @routine 09:E1A3 ObjectLampPostRecord
; WHAT IT DOES: The lamp post's restart: its record (GsuRecordNew424Alt,
; flag bit 4, +$2A high 8), then its frame.
; HOW: GsuRecordNew424Alt, GsuRecordFlag10, GsuRecordSet2AHigh. Reached
; only by unit_objects_09d.
; Entry: as ObjectLampPostLight. Exit: as ObjectLampPostLight.
%at($09E1A3)
ObjectLampPostRecord:
    jsl.l GsuRecordNew424Alt
    jsl.l GsuRecordFlag10
    lda.b #$08
    jsl.l GsuRecordSet2AHigh

; @routine 09:E1B1 ObjectLampPostStep
; WHAT IT DOES: The lamp post's frame: a car's hit runs LampPostHit (burst
; the first time, sound $58 each new contact); no hit clears +$25 bit 0.
; HOW: ObjectSyncFromRecord, RecordContactSummary; JSR LampPostHit.
; Reached only by unit_objects_09d.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
; it.
%at($09E1B1)
ObjectLampPostStep:
    jsl.l ObjectSyncFromRecord
    and.b #$01
    beq .lE1C5
    jsl.l RecordContactSummary
    and.b #$01
    beq .lE1C5
    jsr.w LampPostHit
    rtl
.lE1C5:
    lda.w !obj_25,x
    and.b #$FE
    sta.w !obj_25,x
    rtl

; @routine 09:E1CE LampPostHit
; WHAT IT DOES: A car hits the lamp post: the first time ever (+$25 bit 2)
; it bursts (LampPostBurst); the first time of this contact (+$25 bit 0)
; sound $58.
; HOW: Bit tests and ORs on +$25. Reached only by unit_objects_09d.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the last step
; leaves it.
%at($09E1CE)
LampPostHit:
    lda.w !obj_25,x
    and.b #$04
    beq .lE1D9
    jml.l .lE1E5
.lE1D9:
    lda.w !obj_25,x
    ora.b #$04
    sta.w !obj_25,x
    jsl.l LampPostBurst
.lE1E5:
    lda.w !obj_25,x
    and.b #$01
    beq .lE1F0
    jml.l .lE1FE
.lE1F0:
    lda.w !obj_25,x
    ora.b #$01
    sta.w !obj_25,x
    lda.b #$58
    jsl.l SoundPort0QueuePush
.lE1FE:
    rts

; @routine 09:E1FF LampPostBurst
; WHAT IT DOES: Bursts a lamp post into four pieces (kind $ABF4, handler
; ObjectBreakablePiece, life $0F) at heights -$5A, -$A0, -$5A and -$B4,
; flying out at ($23, -2, $0A), (-$1E, -1, $0F), ($1E, -2, -$0D) and
; (-$23, -1, -$0A).
; HOW: Four ObjectSpawn blocks from object X's position. Reached only by
; unit_objects_09d.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the last piece
; leaves it.
%at($09E1FF)
LampPostBurst:
    rep.b #$20
    lda.w #$ABF4
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .lE213
    jml.l .lE257
.lE213:
    jsl.l ObjectCopyPositionTo
    rep.b #$20
    lda.w #$C4C0
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,y
    rep.b #$20
    lda.w !obj_pos_y,y
    clc
    adc.w #$FFA6
    sta.w !obj_pos_y,y
    sep.b #$20
    rep.b #$20
    lda.w #$0023
    sta.w !obj_vel_x,y
    sep.b #$20
    rep.b #$20
    lda.w #$FFFE
    sta.w !obj_vel_y,y
    sep.b #$20
    rep.b #$20
    lda.w #$000A
    sta.w !obj_vel_z,y
    sep.b #$20
    lda.b #$0F
    sta.w !obj_09+1,y
.lE257:
    rep.b #$20
    lda.w #$ABF4
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .lE26B
    jml.l .lE2AF
.lE26B:
    jsl.l ObjectCopyPositionTo
    rep.b #$20
    lda.w #$C4C0
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,y
    rep.b #$20
    lda.w !obj_pos_y,y
    clc
    adc.w #$FF60
    sta.w !obj_pos_y,y
    sep.b #$20
    rep.b #$20
    lda.w #$FFE2
    sta.w !obj_vel_x,y
    sep.b #$20
    rep.b #$20
    lda.w #$FFFF
    sta.w !obj_vel_y,y
    sep.b #$20
    rep.b #$20
    lda.w #$000F
    sta.w !obj_vel_z,y
    sep.b #$20
    lda.b #$0F
    sta.w !obj_09+1,y
.lE2AF:
    rep.b #$20
    lda.w #$ABF4
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .lE2C3
    jml.l .lE307
.lE2C3:
    jsl.l ObjectCopyPositionTo
    rep.b #$20
    lda.w #$C4C0
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,y
    rep.b #$20
    lda.w !obj_pos_y,y
    clc
    adc.w #$FFA6
    sta.w !obj_pos_y,y
    sep.b #$20
    rep.b #$20
    lda.w #$001E
    sta.w !obj_vel_x,y
    sep.b #$20
    rep.b #$20
    lda.w #$FFFE
    sta.w !obj_vel_y,y
    sep.b #$20
    rep.b #$20
    lda.w #$FFF3
    sta.w !obj_vel_z,y
    sep.b #$20
    lda.b #$0F
    sta.w !obj_09+1,y
.lE307:
    rep.b #$20
    lda.w #$ABF4
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .lE31B
    jml.l .lE35F
.lE31B:
    jsl.l ObjectCopyPositionTo
    rep.b #$20
    lda.w #$C4C0
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,y
    rep.b #$20
    lda.w !obj_pos_y,y
    clc
    adc.w #$FF4C
    sta.w !obj_pos_y,y
    sep.b #$20
    rep.b #$20
    lda.w #$FFDD
    sta.w !obj_vel_x,y
    sep.b #$20
    rep.b #$20
    lda.w #$FFFF
    sta.w !obj_vel_y,y
    sep.b #$20
    rep.b #$20
    lda.w #$FFF6
    sta.w !obj_vel_z,y
    sep.b #$20
    lda.b #$0F
    sta.w !obj_09+1,y
.lE35F:
    rtl

; @routine 09:E360 ObjectBumpPost
; WHAT IT DOES: Sets up a bump post: restart $09:E383 (ObjectBumpPostRecord),
; frame $09:E391 (ObjectBumpPostStep), +$48 = 0; its record and frame.
; HOW: Word and byte stores.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
; leaves it.
%at($09E360)
ObjectBumpPost:
    rep.b #$20
    lda.w #$E383
    sta.w !obj_1d,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1f,x
    rep.b #$20
    lda.w #$E391
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$09
    sta.w !obj_1c,x
    lda.b #$00
    sta.w !obj_48,x

; @routine 09:E383 ObjectBumpPostRecord
; WHAT IT DOES: The bump post's restart: its record ($424, flag bit 4,
; +$2A high 1), then its frame.
; HOW: GsuRecordNew424, GsuRecordFlag10, GsuRecordSet2AHigh.
; Entry: as ObjectBumpPost. Exit: as ObjectBumpPost.
%at($09E383)
ObjectBumpPostRecord:
    jsl.l GsuRecordNew424
    jsl.l GsuRecordFlag10
    lda.b #$01
    jsl.l GsuRecordSet2AHigh

; @routine 09:E391 ObjectBumpPostStep
; WHAT IT DOES: The bump post's frame: while +$48 counts down it bulges
; (+$15 by 6); a car's hit, the first time (+$25 bit 0), plays sound $59
; and sets +$48 = 6; no hit clears the bit.
; HOW: ADCs; ObjectSyncFromRecord, RecordContactSummary.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
; it.
%at($09E391)
ObjectBumpPostStep:
    lda.w !obj_48,x
    bne .lE39A
    jml.l .lE3AC
.lE39A:
    lda.w !obj_15,x
    clc
    adc.b #$06
    sta.w !obj_15,x
    lda.w !obj_48,x
    clc
    adc.b #$FF
    sta.w !obj_48,x
.lE3AC:
    jsl.l ObjectSyncFromRecord
    and.b #$01
    beq .lE3DB
    jsl.l RecordContactSummary
    and.b #$01
    beq .lE3DB
    lda.w !obj_25,x
    and.b #$01
    beq .lE3C7
    jml.l .lE3DA
.lE3C7:
    lda.w !obj_25,x
    ora.b #$01
    sta.w !obj_25,x
    lda.b #$59
    jsl.l SoundPort0QueuePush
    lda.b #$06
    sta.w !obj_48,x
.lE3DA:
    rtl
.lE3DB:
    lda.w !obj_25,x
    and.b #$FE
    sta.w !obj_25,x
    rtl
