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
    jsr.w .lA0FA
.lA0F0:
    rtl
.lA0F1:
    lda.w !obj_25,x
    and.b #$FE
    sta.w !obj_25,x
    rtl
.lA0FA:
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
