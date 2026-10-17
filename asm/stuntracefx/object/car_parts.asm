; car_parts.asm: the cars' loose parts and the player cars of bank $08: a
; part's place on its car (+$06, the parent) from the part's offset bytes
; +$48-$4A turned by the car's angles, the part knocked loose and flying,
; falling back into place, and the two player cars' set-ups and frames;
; with the car set-ups that put a car of a kind on the object list.
; C: src/stuntracefx/object/car_parts.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

; @routine 08:AB71 PartOffset
; WHAT IT DOES: A part's offset on its car: the vector of its bytes +$48,
; +$49, +$4A (through $1F58-$1F5A) turned by the car's (Y's) angles +$16
; (RotateXY), +$14 (RotateXZFrom08) and +$15 (RotateXZ), doubled; left in
; direct page $04/$0A and $05F2 as words.
; HOW: Direct page $02/$08/$94 in (sign bytes $05, $0B, $05F3), one ASL
; per word.
; Entry: M 8-bit, X 16-bit, D = $0000; X = the part, Y = its car. Exit: M
; 8-bit; A = the doubled Z word; N, Z, C from its ASL.
%at($08AB71)
PartOffset:
    lda.w !obj_48,x
    sta.w !wram_1F58
    lda.w !obj_49,x
    sta.w !wram_1F59
    lda.w !obj_4a,x
    sta.w !wram_1F5A
    lda.w !wram_1F58
    sta.b !dp_02
    sta.b !dp_04
    bit.b #$80
    beq .lAB94
    lda.b #$FF
    sta.b !dp_05
    bra .lAB96
.lAB94:
    stz.b !dp_05
.lAB96:
    lda.w !wram_1F59
    sta.b !dp_08
    sta.b !actor_0a
    bit.b #$80
    beq .lABA7
    lda.b #$FF
    sta.b !dp_0b
    bra .lABA9
.lABA7:
    stz.b !dp_0b
.lABA9:
    lda.w !wram_1F5A
    sta.b !dp_94
    sta.w !rot_z
    bit.b #$80
    beq .lABBC
    lda.b #$FF
    sta.w !rot_z_high
    bra .lABBF
.lABBC:
    stz.w !rot_z_high
.lABBF:
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
    sep.b #$20
    rts

; @routine 08:AC04 PartDetach
; WHAT IT DOES: A part comes off its car: the car's +$25 bit 7 set, the
; part's model by the car's kind (+$49, ObjectSet5DByKind), +$4B = 0, frame
; $08:AC27 (PartDetachStep); then that frame.
; HOW: Byte and word stores; JSR ObjectSet5DByKind.
; Entry: M 8-bit, X 16-bit; X = the part. Exit: as the frame leaves it.
%at($08AC04)
PartDetach:
    ldy.w !obj_06,x
    lda.w !obj_25,y
    ora.b #$80
    sta.w !obj_25,y
    lda.w !obj_49,y
    jsr.w ObjectSet5DByKind
    stz.w !obj_4b,x
    rep.b #$20
    lda.w #$AC27
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,x

; @routine 08:AC27 PartDetachStep
; WHAT IT DOES: The detaching part's frame: its count +$4B goes up each
; frame; it sits at its car (+$06) plus its offset (PartOffset) taken 1 to
; 4 times (rising to 4, then falling); at 7 the car's +$25 bit 7 is
; cleared and at 8 the part is released.
; HOW: CMP, INC; SBC #9 and EOR/INC for the falling half; a PHA/PLA count
; loop of 16-bit ADCs; ObjectReleaseLong by JML.
; Entry: M 8-bit, X 16-bit; X = the part; D = $0000. Exit: as
; ObjectRebucket or ObjectReleaseLong leaves it.
%at($08AC27)
PartDetachStep:
    ldy.w !obj_06,x
    lda.w !obj_4b,x
    cmp.b #$08
    bne .lAC35
    jml.l ObjectReleaseLong
.lAC35:
    lda.w !obj_4b,x
    cmp.b #$07
    beq .lAC40
    jml.l .lAC48
.lAC40:
    lda.w !obj_25,y
    and.b #$7F
    sta.w !obj_25,y
.lAC48:
    inc.w !obj_4b,x
    jsr.w PartOffset
    jsl.l ObjectCopyPositionFrom
    jsl.l ObjectCopy14From
    lda.w !obj_4b,x
    cmp.b #$04
    bmi .lAC65
    sec
    sbc.b #$09
    beq .lAC92
    eor.b #$FF
    inc a
.lAC65:
    pha
    rep.b #$20
    lda.w !obj_pos_x,x
    clc
    adc.b !dp_04
    sta.w !obj_pos_x,x
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_y,x
    clc
    adc.b !actor_0a
    sta.w !obj_pos_y,x
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_z,x
    clc
    adc.w !rot_z
    sta.w !obj_pos_z,x
    sep.b #$20
    pla
    dec a
    bne .lAC65
.lAC92:
    jsl.l ObjectRebucket
    rtl

; @routine 08:AC97 PartHit
; WHAT IT DOES: A part hit off its car: runs Super FX program $04:888E,
; sets its +$25 bit 0 and its car's +$25 bit 2, and knocks it off
; (PartKnockOff).
; HOW: GsuRun with X and Y kept (PHX, PHY, PLY, PLX); a BRA into
; PartKnockOff.
; Entry: M 8-bit, X 16-bit; X = the part; D = $0000. Exit: as PartKnockOff
; leaves it.
%at($08AC97)
PartHit:
    phx
    phy
    lda.b #$04
    ldx.w #$888E
    jsl.l GsuRun
    ply
    plx
    lda.w !obj_25,x
    ora.b #$01
    sta.w !obj_25,x
    ldy.w !obj_06,x
    lda.w !obj_25,y
    ora.b #$04
    sta.w !obj_25,y
    bra PartKnockOff

; @routine 08:ACB9 PartKnockOff
; WHAT IT DOES: A part knocked off its car: its model by the car's kind,
; at the car's position plus its offset, the car's record velocity, a
; record of its own (GsuRecordNew426). With none free it is released
; (and with +$25 bit 0 the car's +$25 bits 2 and 4 are cleared first).
; With one: a life +$27 with bit 7 clear sets bit 7 of the record's flags
; (+$16); its velocity grows by the offset (Y doubled), given to the
; record; frame $08:AD70 (PartFlyStep); then that frame.
; HOW: JSR ObjectSet5DByKind, PartOffset; ObjectCopyPositionFrom,
; ObjectCopy14From; 16-bit ADCs; GsuRecordVelocity.
; Entry: M 8-bit, X 16-bit; X = the part; D = $0000. Exit: as the path
; leaves it.
%at($08ACB9)
PartKnockOff:
    ldy.w !obj_06,x
    lda.w !obj_49,y
    jsr.w ObjectSet5DByKind
    jsr.w PartOffset
    jsl.l ObjectCopyPositionFrom
    jsl.l ObjectCopy14From
    rep.b #$20
    lda.w !obj_pos_x,x
    clc
    adc.b !dp_04
    sta.w !obj_pos_x,x
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_y,x
    clc
    adc.b !actor_0a
    sta.w !obj_pos_y,x
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_z,x
    clc
    adc.w !rot_z
    sta.w !obj_pos_z,x
    sep.b #$20
    jsl.l ObjectVelocityFromRecordOf
    jsl.l GsuRecordNew426
    bpl .lAD22
    lda.w !obj_25,x
    and.b #$01
    bne .lAD0A
    jml.l .lAD1D
.lAD0A:
    ldy.w !obj_06,x
    lda.w !obj_25,y
    and.b #$FB
    sta.w !obj_25,y
    lda.w !obj_25,y
    and.b #$EF
    sta.w !obj_25,y
.lAD1D:
    jsl.l ObjectRelease
    rtl
.lAD22:
    lda.w !obj_27,x
    bmi .lAD3C
    phx
    rep.b #$20
    lda.w !obj_22,x
    tax
    lda.l !gsu_rec_flags,x
    ora.w #$0080
    sta.l !gsu_rec_flags,x
    plx
    sep.b #$20
.lAD3C:
    rep.b #$20
    lda.b !dp_04
    clc
    adc.w !obj_vel_x,x
    sta.w !obj_vel_x,x
    lda.b !actor_0a
    asl a
    clc
    adc.w !obj_vel_y,x
    sta.w !obj_vel_y,x
    lda.w !rot_z
    clc
    adc.w !obj_vel_z,x
    sta.w !obj_vel_z,x
    sep.b #$20
    jsl.l GsuRecordVelocity
    rep.b #$20
    lda.w #$AD70
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,x

; @routine 08:AD70 PartFlyStep
; WHAT IT DOES: The flying part's frame: +$36 = 3; synced from its record;
; with no contacts, while its car's +$25 bit 4 is set, a record summary
; with bit 7 breaks it off (ObjectBreakOffContact). Then, unless the car's
; bit 4 is set and +$4B is 0, it tumbles (+$4B down, +$14 by 8, +$15 by
; 4) or, at +$4B = 0, turns back (PartReturn); a life +$27 (bit 7 clear)
; counts down and releases it at 0; ObjectSpawnChanceD460.
; HOW: ObjectSyncFromRecord, RecordContactSummary; JMP, JML and BRL to
; the routines named.
; Entry: M 8-bit, X 16-bit; X = the part; D = $0000. Exit: as the path
; leaves it.
%at($08AD70)
PartFlyStep:
    lda.b #$03
    sta.w !obj_36,x
    jsl.l ObjectSyncFromRecord
    and.b #$01
    bne .lAD96
    ldy.w !obj_06,x
    lda.w !obj_25,y
    and.b #$10
    bne .lAD8B
    jml.l .lAD96
.lAD8B:
    jsl.l RecordContactSummary
    and.b #$80
    beq .lAD96
    jmp.w ObjectBreakOffContact
.lAD96:
    lda.b #$03
    sta.w !obj_36,x
    ldy.w !obj_06,x
    lda.w !obj_25,y
    and.b #$10
    bne .lADA9
    jml.l .lADB2
.lADA9:
    lda.w !obj_4b,x
    bne .lADB2
    jml.l .lADD0
.lADB2:
    lda.w !obj_4b,x
    bne .lADBB
    jml.l PartReturn
.lADBB:
    dec.w !obj_4b,x
    lda.w !obj_14,x
    clc
    adc.b #$08
    sta.w !obj_14,x
    lda.w !obj_15,x
    clc
    adc.b #$04
    sta.w !obj_15,x
.lADD0:
    lda.w !obj_27,x
    bmi .lADDE
    dec a
    bne .lADDB
    brl ObjectReleaseLong
.lADDB:
    sta.w !obj_27,x
.lADDE:
    jsr.w ObjectSpawnChanceD460
    rtl

; @routine 08:ADE2 PartReturn
; WHAT IT DOES: The flying part turns back to its car: frame $08:AE0F
; (PartReturnStep), +$4B = $0A, sound $4B ($4C with +$25 bit 1); then that
; frame.
; HOW: Word and byte stores; entered by a JML from PartFlyStep.
; Entry: M 8-bit, X 16-bit; X = the part; D = $0000. Exit: as the frame
; leaves it.
%at($08ADE2)
PartReturn:
    rep.b #$20
    lda.w #$AE0F
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,x
    lda.b #$0A
    sta.w !obj_4b,x
    lda.w !obj_25,x
    and.b #$02
    beq .lAE01
    jml.l .lAE09
.lAE01:
    lda.b #$4B
    jsl.l SoundPort0QueuePush
    bra PartReturnStep
.lAE09:
    lda.b #$4C
    jsl.l SoundPort0QueuePush

; @routine 08:AE0F PartReturnStep
; WHAT IT DOES: The part falling back onto its car: it moves a quarter of
; the way to its place on the car (PartOffset from the car's position) and
; turns a quarter of the way to the car's angles, with the car's record
; velocity (ObjectVelocityFromRecordOf, ObjectApplyVelocity); on the last
; count (+$4B = 1) sound $4E ($4D with +$25 bit 1); while +$4B lasts it
; counts down; then, unless the car's +$25 bit 2 is set, its frame becomes
; ObjectReleaseLong ($08:F927), and with its own +$25 bit 0 the car's bit
; 2 is cleared.
; HOW: ApproachQuarterMin4 on the position words, ApproachQuarterMin4Byte
; on +$14-$16, all through $3E; +$36 = 3.
; Entry: M 8-bit, X 16-bit; X = the part; D = $0000. Exit: as the path
; leaves it.
%at($08AE0F)
PartReturnStep:
    lda.b #$03
    sta.w !obj_36,x
    ldy.w !obj_06,x
    jsl.l ObjectVelocityFromRecordOf
    jsr.w PartOffset
    rep.b #$20
    lda.w !obj_pos_x,y
    clc
    adc.b !dp_04
    sta.b !dp_04
    lda.w !obj_pos_y,y
    clc
    adc.b !actor_0a
    sta.b !actor_0a
    lda.w !obj_pos_z,y
    clc
    adc.w !rot_z
    sta.w !rot_z
    sep.b #$20
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
    lda.w !obj_14,x
    sta.b !scale_scratch
    lda.w !obj_14,y
    jsl.l ApproachQuarterMin4Byte
    sta.w !obj_14,x
    lda.w !obj_15,x
    sta.b !scale_scratch
    lda.w !obj_15,y
    jsl.l ApproachQuarterMin4Byte
    sta.w !obj_15,x
    lda.w !obj_16,x
    sta.b !scale_scratch
    lda.w !obj_16,y
    jsl.l ApproachQuarterMin4Byte
    sta.w !obj_16,x
    jsl.l ObjectApplyVelocity
    lda.w !obj_4b,x
    cmp.b #$01
    beq .lAEAF
    jml.l .lAEC8
.lAEAF:
    lda.w !obj_25,x
    and.b #$02
    beq .lAEBA
    jml.l .lAEC2
.lAEBA:
    lda.b #$4E
    jsl.l SoundPort0QueuePush
    bra .lAEC8
.lAEC2:
    lda.b #$4D
    jsl.l SoundPort0QueuePush
.lAEC8:
    jsl.l ObjectRebucket
    lda.b #$03
    sta.w !obj_36,x
    lda.w !obj_4b,x
    bne .lAEDA
    jml.l .lAEDE
.lAEDA:
    dec.w !obj_4b,x
    rtl
.lAEDE:
    ldy.w !obj_06,x
    lda.w !obj_25,y
    and.b #$04
    beq .lAEEC
    jml.l .lAEFB
.lAEEC:
    rep.b #$20
    lda.w #$F927
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,x
.lAEFB:
    lda.w !obj_25,x
    and.b #$01
    bne .lAF06
    jml.l .lAF0E
.lAF06:
    lda.w !obj_25,y
    and.b #$FB
    sta.w !obj_25,y
.lAF0E:
    rtl

; @routine 08:AF0F PlayerCar1
; WHAT IT DOES: Sets up player 1's car: list kind +$26 = 1 on the object
; list (ObjectListAppend), $A0 higher; in a one-player race with the bare
; object's frame at $8E2A, +$27 = $8669; its record (ObjectCarRecordNew 1)
; matched at $70:2476 with bit 3 of +$1C; frame $08:AF93 (PlayerCar1Step),
; attached to the bare object $1ADA, its model by kind, $1A5E = 0, +$29 =
; 0; then that frame.
; HOW: 16-bit compares and long record accesses; ObjectCarRecordNew is
; called with M 16-bit.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
; leaves it.
%at($08AF0F)
PlayerCar1:
    lda.b #$01
    sta.w !obj_26,x
    jsl.l ObjectListAppend
    rep.b #$20
    lda.w !obj_pos_y,x
    clc
    adc.w #$FF60
    sta.w !obj_pos_y,x
    sep.b #$20
    rep.b #$20
    lda.w !hud_mode
    and.w #$00FF
    bne .lAF41
    ldy.w !bare_object_a
    lda.w !obj_1a_w,y
    cmp.w #$8E2A
    bne .lAF41
    lda.w #$8669
    sta.w !obj_27,x
.lAF41:
    lda.w #$0001
    jsl.l ObjectCarRecordNew
    rep.b #$20
    lda.w !obj_22,x
    beq .lAF61
    phx
    tax
    sta.l !gsu_match_2476
    lda.l !gsu_rec_1c,x
    ora.w #$0008
    sta.l !gsu_rec_1c,x
    plx
.lAF61:
    sep.b #$20
    rep.b #$20
    lda.w #$AF93
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,x
    ldy.w !bare_object_a
    jsl.l ObjectAttachLinked
    lda.w !obj_49,x
    jsr.w ObjectSet5DByKind
    rep.b #$20
    lda.w #$0000
    sta.w !current_player+1
    sep.b #$20
    rep.b #$20
    lda.w #$0000
    sta.w !obj_29,x
    sep.b #$20

; @routine 08:AF93 PlayerCar1Step
; WHAT IT DOES: Player 1's car frame: current player $1A5D = 0, then the
; car's drive (GsuRecordLand, PlayerCarEvents, ObjectPoseFromRecord,
; GsuRecordStep6A, ObjectMoveForward, ObjectRebucket), +$36 = 3; +$24 bit
; 1 follows bit 1 of its record's +$1C.
; HOW: JSRs and JSLs; the record word through X (PHX/PLX).
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: M 8-bit; X
; unchanged, Y = X; N, Z from X.
%at($08AF93)
PlayerCar1Step:
    lda.b #$00
    sta.w !current_player
    jsr.w GsuRecordLand
    jsl.l PlayerCarEvents
    jsl.l ObjectPoseFromRecord
    jsr.w GsuRecordStep6A
    jsr.w ObjectMoveForward
    jsl.l ObjectRebucket
    lda.b #$03
    sta.w !obj_36,x
    lda.w !obj_24,x
    and.b #$FD
    sta.w !obj_24,x
    txy
    rep.b #$20
    phx
    lda.w !obj_22,y
    tax
    lda.l !gsu_rec_1c,x
    and.w #$0002
    sep.b #$20
    beq .lAFD5
    lda.w !obj_24,y
    ora.b #$02
    sta.w !obj_24,y
.lAFD5:
    plx
    rtl

; @routine 08:AFD7 PlayerCar2
; WHAT IT DOES: Sets up player 2's car: list kind +$26 = 2 on the object
; list, $A0 higher; its record (ObjectCarRecordNew 2) matched at
; $70:2478 with bit 3 of +$1C; frame $08:B049 (PlayerCar2Step), attached
; to the bare object $1ADC, its model by kind, $1A5E = 0, +$29 = 0; then
; that frame.
; HOW: As PlayerCar1 (the record's +$64 is read and ORed, the result
; unused).
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
; leaves it.
%at($08AFD7)
PlayerCar2:
    lda.b #$02
    sta.w !obj_26,x
    jsl.l ObjectListAppend
    rep.b #$20
    lda.w !obj_pos_y,x
    clc
    adc.w #$FF60
    sta.w !obj_pos_y,x
    sep.b #$20
    rep.b #$20
    lda.w #$0002
    jsl.l ObjectCarRecordNew
    rep.b #$20
    lda.w !obj_22,x
    beq .lB017
    phx
    tax
    sta.l !gsu_match_2478
    lda.l !gsu_rec_64,x
    ora.w #$0008
    lda.l !gsu_rec_1c,x
    ora.w #$0008
    sta.l !gsu_rec_1c,x
    plx
.lB017:
    sep.b #$20
    rep.b #$20
    lda.w #$B049
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,x
    ldy.w !bare_object_b
    jsl.l ObjectAttachLinked
    lda.w !obj_49,x
    jsr.w ObjectSet5DByKind
    rep.b #$20
    lda.w #$0000
    sta.w !current_player+1
    sep.b #$20
    rep.b #$20
    lda.w #$0000
    sta.w !obj_29,x
    sep.b #$20

; @routine 08:B049 PlayerCar2Step
; WHAT IT DOES: Player 2's car frame: current player $1A5D = 1, player 2's
; held buttons ($030D) and $1F25 copied to $1AF0/$1AF2, then the car's
; drive (GsuRecordLand, PlayerCarEvents, ObjectPoseFromRecord,
; GsuRecordStep6A, ObjectMoveForward, ObjectRebucket), +$36 = 3.
; HOW: 16-bit copies; JSRs and JSLs.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: A low byte = 3.
%at($08B049)
PlayerCar2Step:
    lda.b #$01
    sta.w !current_player
    rep.b #$20
    lda.w !joy2_held
    sta.w !seq_input
    lda.w !gsu_ab13_polls+4
    sta.w !seq_input+2
    sep.b #$20
    jsr.w GsuRecordLand
    jsl.l PlayerCarEvents
    jsl.l ObjectPoseFromRecord
    jsr.w GsuRecordStep6A
    jsr.w ObjectMoveForward
    jsl.l ObjectRebucket
    lda.b #$03
    sta.w !obj_36,x
    rtl

; @routine 08:B1BA ObjectCarKind3
; WHAT IT DOES: Sets up a car of list kind 3 (+$48 = 0, +$26 = 3) and
; spawns it (ObjectCarSpawnListed).
; HOW: Two stores and a JMP; course data points here.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as ObjectCarSpawn
; leaves it.
%at($08B1BA)
ObjectCarKind3:
    lda.b #$00
    sta.w !obj_48,x
    lda.b #$03
    sta.w !obj_26,x
    jmp.w ObjectCarSpawnListed

; @routine 08:B1C7 ObjectCarKind4
; WHAT IT DOES: Sets up a car of list kind 4 (+$48 = 5, +$26 = 4) and
; spawns it (ObjectCarSpawnListed).
; HOW: As ObjectCarKind3.
; Entry: as ObjectCarKind3. Exit: as ObjectCarKind3.
%at($08B1C7)
ObjectCarKind4:
    lda.b #$05
    sta.w !obj_48,x
    lda.b #$04
    sta.w !obj_26,x
    jmp.w ObjectCarSpawnListed

; @routine 08:B1D4 ObjectCarKind5
; WHAT IT DOES: Sets up a car of list kind 5 (+$48 = 0, +$26 = 5) and
; spawns it (ObjectCarSpawnListed).
; HOW: As ObjectCarKind3.
; Entry: as ObjectCarKind3. Exit: as ObjectCarKind3.
%at($08B1D4)
ObjectCarKind5:
    lda.b #$00
    sta.w !obj_48,x
    lda.b #$05
    sta.w !obj_26,x
    jmp.w ObjectCarSpawnListed

; @routine 08:B1E1 ObjectCarSpawnListed
; WHAT IT DOES: Puts object X on the object list (ObjectListAppend), then
; runs into ObjectCarSpawn.
; HOW: JSL ObjectListAppend, falling into 08:B1E5.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as ObjectCarSpawn
; leaves it.
%at($08B1E1)
ObjectCarSpawnListed:
    jsl.l ObjectListAppend
