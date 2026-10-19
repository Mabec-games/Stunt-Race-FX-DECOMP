; lamps_08.asm: course object set-ups of bank $08 no course in the scripts
; places: break-off pieces, lamps, a lamp stack and a falling object.
; Also bank $08 code nothing in the game calls: two camera pads
; (08:843C-8524) and a spray of eight pieces (08:D8E4-DC12).
; C: src/stuntracefx/object/lamps_08.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 08:D61B ObjectBreakOffBurst15
; WHAT IT DOES: Makes fifteen break-off pieces (kind $A1AC) at object X's
; position, each with X as its parent (+$06) and the handler $08:D337
; (ObjectBreakOffContact); a spawn that fails only skips that piece.
; HOW: The count kept on the stack (PHA, PLA, 8-bit) across ObjectSpawn
; and ObjectCopyPositionTo; A's low byte is pulled back between the
; parent's store (TXA, 16-bit) and the handler's.
; Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; A low byte = 0, Z
; set.
%at($08D61B)
ObjectBreakOffBurst15:
    lda.b #$0F
.lD61D:
    pha
    rep.b #$20
    lda.w #$A1AC
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .lD632
    jml.l .lD64F
.lD632:
    jsl.l ObjectCopyPositionTo
    pha
    rep.b #$20
    txa
    sta.w !obj_06,y
    sep.b #$20
    pla
    rep.b #$20
    lda.w #$D337
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,y
.lD64F:
    pla
    dec a
    bne .lD61D
    rtl

; @routine 08:D654 ObjectLampPair08
; WHAT IT DOES: Sets up a lamp: object X $78 higher ($FF88 on +$0E), lit
; by $9971 (+$24 bit 3, +$3A = 0, +$5F = $28); and, when one can be made,
; a second light (kind $A078, $5D = $9973, +$5F = $50) at its position
; $140 higher; then no handler.
; HOW: ObjectSpawn, ObjectCopyPositionTo; the second light is not
; rebucketed.
; Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; A low byte = 0.
%at($08D654)
ObjectLampPair08:
    rep.b #$20
    lda.w !obj_pos_y,x
    clc
    adc.w #$FF88
    sta.w !obj_pos_y,x
    sep.b #$20
    lda.w !obj_24,x
    ora.b #$08
    sta.w !obj_24,x
    lda.b #$00
    sta.w !obj_3a,x
    lda.b #$28
    sta.w !obj_5f,x
    rep.b #$20
    lda.w #$9971
    sta.w !obj_5d,x
    sep.b #$20
    rep.b #$20
    lda.w #$A078
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .lD692
    jml.l .lD6C0
.lD692:
    lda.w !obj_24,y
    ora.b #$08
    sta.w !obj_24,y
    lda.b #$00
    sta.w !obj_3a,y
    lda.b #$50
    sta.w !obj_5f,y
    rep.b #$20
    lda.w #$9973
    sta.w !obj_5d,y
    sep.b #$20
    jsl.l ObjectCopyPositionTo
    rep.b #$20
    lda.w !obj_pos_y,y
    clc
    adc.w #$FEC0
    sta.w !obj_pos_y,y
    sep.b #$20
.lD6C0:
    rep.b #$20
    lda.w #$0000
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$00
    sta.w !obj_1c,x
    rtl

; @routine 08:D6D0 ObjectLampStack
; WHAT IT DOES: Sets up a stack of three lights lit by $9975: object X
; $46 higher ($FFBA on +$0E, +$5F = $1E) and two more (kind $A104, +$5F =
; $32) at its position $2BC and $578 higher, each made only when an
; object is free; then no handler.
; HOW: ObjectSpawn, ObjectCopyPositionTo; each light rebucketed with X
; and Y swapped round ObjectRebucket.
; Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; A low byte = 0.
%at($08D6D0)
ObjectLampStack:
    rep.b #$20
    lda.w !obj_pos_y,x
    clc
    adc.w #$FFBA
    sta.w !obj_pos_y,x
    sep.b #$20
    rep.b #$20
    lda.w #$9975
    sta.w !obj_5d,x
    sep.b #$20
    lda.w !obj_24,x
    ora.b #$08
    sta.w !obj_24,x
    lda.b #$00
    sta.w !obj_3a,x
    lda.b #$1E
    sta.w !obj_5f,x
    rep.b #$20
    lda.w #$A104
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .lD70E
    jml.l .lD746
.lD70E:
    lda.w !obj_24,y
    ora.b #$08
    sta.w !obj_24,y
    lda.b #$00
    sta.w !obj_3a,y
    lda.b #$32
    sta.w !obj_5f,y
    rep.b #$20
    lda.w #$9975
    sta.w !obj_5d,y
    sep.b #$20
    jsl.l ObjectCopyPositionTo
    rep.b #$20
    lda.w !obj_pos_y,y
    clc
    adc.w #$FD44
    sta.w !obj_pos_y,y
    sep.b #$20
    phx
    tyx
    ply
    jsl.l ObjectRebucket
    phx
    tyx
    ply
.lD746:
    rep.b #$20
    lda.w #$A104
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .lD75A
    jml.l .lD792
.lD75A:
    lda.w !obj_24,y
    ora.b #$08
    sta.w !obj_24,y
    lda.b #$00
    sta.w !obj_3a,y
    lda.b #$32
    sta.w !obj_5f,y
    rep.b #$20
    lda.w #$9975
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
.lD792:
    rep.b #$20
    lda.w #$0000
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$00
    sta.w !obj_1c,x
    rtl

; @routine 08:D7A2 ObjectLampLow08
; WHAT IT DOES: Sets up a lamp lit by $9B8F: object X $1E higher ($FFE2
; on +$0E), +$5F = $14; then no handler.
; HOW: Word and byte stores; BRA to the shared tail at $08:D81B.
; Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; A low byte = 0.
%at($08D7A2)
ObjectLampLow08:
    rep.b #$20
    lda.w #$9B8F
    sta.w !obj_5d,x
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_y,x
    clc
    adc.w #$FFE2
    sta.w !obj_pos_y,x
    sep.b #$20
    bra ObjectFallerStep_lD81B

; @routine 08:D7BC ObjectFaller
; WHAT IT DOES: Sets up a falling object: $12C higher, +$1D = $08:D808,
; lit by its own light (+$24 bit 3, +$3A = 0, +$5F = $14), Y speed -$64,
; handler ObjectFallerStep; then ObjectFallerRecord.
; HOW: BRA to the set-up at $08:D7C0.
; Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectSyncFromRecord.
%at($08D7BC)
ObjectFaller:
    bra ObjectFallerB_lD7C0

; @routine 08:D7BE ObjectFallerB
; WHAT IT DOES: As ObjectFaller (a second entry for another kind).
; HOW: BRA to the set-up at $08:D7C0.
; Entry: as ObjectFaller. Exit: as ObjectFaller.
%at($08D7BE)
ObjectFallerB:
    bra .lD7C0
.lD7C0:
    rep.b #$20
    lda.w !obj_pos_y,x
    clc
    adc.w #$FED4
    sta.w !obj_pos_y,x
    sep.b #$20
    rep.b #$20
    lda.w #$D808
    sta.w !obj_1d,x
    sep.b #$20
    lda.b #$08
    sta.w !obj_1f,x
    lda.w !obj_24,x
    ora.b #$08
    sta.w !obj_24,x
    lda.b #$00
    sta.w !obj_3a,x
    lda.b #$14
    sta.w !obj_5f,x
    rep.b #$20
    lda.w #$FF9C
    sta.w !obj_vel_y,x
    sep.b #$20
    rep.b #$20
    lda.w #$D816
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,x

; @routine 08:D808 ObjectFallerRecord
; WHAT IT DOES: Gives the falling object its Super FX record
; (GsuRecordNew424) with flag $10 and $20 in the high byte of +$2A, then
; its first step (ObjectFallerStep). Also its +$1D pointer.
; HOW: GsuRecordNew424, GsuRecordFlag10, LDA #$20, GsuRecordSet2AHigh;
; falls into ObjectFallerStep. A failed record is not checked.
; Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectSyncFromRecord.
%at($08D808)
ObjectFallerRecord:
    jsl.l GsuRecordNew424
    jsl.l GsuRecordFlag10
    lda.b #$20
    jsl.l GsuRecordSet2AHigh

; @routine 08:D816 ObjectFallerStep
; WHAT IT DOES: The falling object's handler: the object follows its Super
; FX record (ObjectSyncFromRecord).
; HOW: JSL ObjectSyncFromRecord.
; Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectSyncFromRecord.
%at($08D816)
ObjectFallerStep:
    jsl.l ObjectSyncFromRecord
    rtl
.lD81B:
    lda.w !obj_24,x
    ora.b #$08
    sta.w !obj_24,x
    lda.b #$00
    sta.w !obj_3a,x
    lda.b #$14
    sta.w !obj_5f,x
    rep.b #$20
    lda.w #$0000
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$00
    sta.w !obj_1c,x
    rtl

; @routine 08:D8E4 ObjectSpray8
; WHAT IT DOES: Sprays eight pieces from object Y: each a new object of
; its kind (the table above) with the handler $08:DB7A
; (ObjectSprayPieceStart), Y as its parent (+$27), the speeds +$2E (four
; sideways values, twice), +$30 = $03E8 and +$32 = $1770, the rise +$29
; ($FC7C for the first four, $FE70 for the rest) and the turn +$15 from 0
; down by 4. A spawn that fails skips only that piece. Nothing in the
; game calls it; a direct call is its only way in.
; HOW: Eight copies of one block, each jumping to the next when
; ObjectSpawn fails; X and Y saved on the stack (PHX, PHY) and X = Y for
; the parent; A's low byte kept on the stack (PHA, PLA) across the
; parent's store (TXA, 16-bit).
; Entry: M 8-bit, X 16-bit; Y = the parent; D = $0000. Exit: X and Y
; unchanged, N and Z from X; M 8-bit; A as the last piece left it.
%at($08D8E4)
ObjectSpray8:
    phx
    phy
    tyx
    rep.b #$20
    lda.w #$A660
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .lD8FB
    jml.l .lD939
.lD8FB:
    rep.b #$20
    lda.w #$DB7A
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,y
    rep.b #$20
    lda.w #$FE3E
    sta.w !obj_vel_x,y
    lda.w #$03E8
    sta.w !obj_vel_y,y
    lda.w #$1770
    sta.w !obj_vel_z,y
    sep.b #$20
    rep.b #$20
    lda.w #$FC7C
    sta.w !obj_29,y
    sep.b #$20
    pha
    rep.b #$20
    txa
    sta.w !obj_27,y
    sep.b #$20
    pla
    lda.b #$00
    sta.w !obj_15,y
.lD939:
    rep.b #$20
    lda.w #$A67C
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .lD94D
    jml.l .lD98B
.lD94D:
    rep.b #$20
    lda.w #$DB7A
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,y
    rep.b #$20
    lda.w #$FF6A
    sta.w !obj_vel_x,y
    lda.w #$03E8
    sta.w !obj_vel_y,y
    lda.w #$1770
    sta.w !obj_vel_z,y
    sep.b #$20
    rep.b #$20
    lda.w #$FC7C
    sta.w !obj_29,y
    sep.b #$20
    pha
    rep.b #$20
    txa
    sta.w !obj_27,y
    sep.b #$20
    pla
    lda.b #$FC
    sta.w !obj_15,y
.lD98B:
    rep.b #$20
    lda.w #$A698
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .lD99F
    jml.l .lD9DD
.lD99F:
    rep.b #$20
    lda.w #$DB7A
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,y
    rep.b #$20
    lda.w #$0096
    sta.w !obj_vel_x,y
    lda.w #$03E8
    sta.w !obj_vel_y,y
    lda.w #$1770
    sta.w !obj_vel_z,y
    sep.b #$20
    rep.b #$20
    lda.w #$FC7C
    sta.w !obj_29,y
    sep.b #$20
    pha
    rep.b #$20
    txa
    sta.w !obj_27,y
    sep.b #$20
    pla
    lda.b #$F8
    sta.w !obj_15,y
.lD9DD:
    rep.b #$20
    lda.w #$A6B4
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .lD9F1
    jml.l .lDA2F
.lD9F1:
    rep.b #$20
    lda.w #$DB7A
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,y
    rep.b #$20
    lda.w #$01C2
    sta.w !obj_vel_x,y
    lda.w #$03E8
    sta.w !obj_vel_y,y
    lda.w #$1770
    sta.w !obj_vel_z,y
    sep.b #$20
    rep.b #$20
    lda.w #$FC7C
    sta.w !obj_29,y
    sep.b #$20
    pha
    rep.b #$20
    txa
    sta.w !obj_27,y
    sep.b #$20
    pla
    lda.b #$F4
    sta.w !obj_15,y
.lDA2F:
    rep.b #$20
    lda.w #$A6D0
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .lDA43
    jml.l .lDA81
.lDA43:
    rep.b #$20
    lda.w #$DB7A
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,y
    rep.b #$20
    lda.w #$FE3E
    sta.w !obj_vel_x,y
    lda.w #$03E8
    sta.w !obj_vel_y,y
    lda.w #$1770
    sta.w !obj_vel_z,y
    sep.b #$20
    rep.b #$20
    lda.w #$FE70
    sta.w !obj_29,y
    sep.b #$20
    pha
    rep.b #$20
    txa
    sta.w !obj_27,y
    sep.b #$20
    pla
    lda.b #$F0
    sta.w !obj_15,y
.lDA81:
    rep.b #$20
    lda.w #$A6EC
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .lDA95
    jml.l .lDAD3
.lDA95:
    rep.b #$20
    lda.w #$DB7A
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,y
    rep.b #$20
    lda.w #$FF6A
    sta.w !obj_vel_x,y
    lda.w #$03E8
    sta.w !obj_vel_y,y
    lda.w #$1770
    sta.w !obj_vel_z,y
    sep.b #$20
    rep.b #$20
    lda.w #$FE70
    sta.w !obj_29,y
    sep.b #$20
    pha
    rep.b #$20
    txa
    sta.w !obj_27,y
    sep.b #$20
    pla
    lda.b #$EC
    sta.w !obj_15,y
.lDAD3:
    rep.b #$20
    lda.w #$A6B4
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .lDAE7
    jml.l .lDB25
.lDAE7:
    rep.b #$20
    lda.w #$DB7A
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,y
    rep.b #$20
    lda.w #$0096
    sta.w !obj_vel_x,y
    lda.w #$03E8
    sta.w !obj_vel_y,y
    lda.w #$1770
    sta.w !obj_vel_z,y
    sep.b #$20
    rep.b #$20
    lda.w #$FE70
    sta.w !obj_29,y
    sep.b #$20
    pha
    rep.b #$20
    txa
    sta.w !obj_27,y
    sep.b #$20
    pla
    lda.b #$E8
    sta.w !obj_15,y
.lDB25:
    rep.b #$20
    lda.w #$A708
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .lDB39
    jml.l .lDB77
.lDB39:
    rep.b #$20
    lda.w #$DB7A
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,y
    rep.b #$20
    lda.w #$01C2
    sta.w !obj_vel_x,y
    lda.w #$03E8
    sta.w !obj_vel_y,y
    lda.w #$1770
    sta.w !obj_vel_z,y
    sep.b #$20
    rep.b #$20
    lda.w #$FE70
    sta.w !obj_29,y
    sep.b #$20
    pha
    rep.b #$20
    txa
    sta.w !obj_27,y
    sep.b #$20
    pla
    lda.b #$E4
    sta.w !obj_15,y
.lDB77:
    ply
    plx
    rts

; @routine 08:DB7A ObjectSprayPieceStart
; WHAT IT DOES: The first tick of a piece of ObjectSpray8: its handler
; becomes ObjectSprayPieceStep ($08:DB90) and +$14 a random byte
; (RandomStepLong); then the step itself.
; HOW: Runs on into ObjectSprayPieceStep.
; Entry: M 8-bit, X 16-bit; X = the piece; D = $0000. Exit: as
; ObjectSprayPieceStep.
%at($08DB7A)
ObjectSprayPieceStart:
    rep.b #$20
    lda.w #$DB90
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,x
    jsl.l RandomStepLong
    sta.w !obj_14,x

; @routine 08:DB90 ObjectSprayPieceStep
; WHAT IT DOES: A tick of a piece of ObjectSpray8: +$14 eases a sixteenth
; of the way to 0 (at least 1), the turn +$15 grows by 8, the piece takes
; its parent's (+$27) position (ObjectCopyPositionFrom) and moves by its
; speeds (ObjectApplyVelocity); its speed +$30 eases an eighth of the way
; (at least 8) to its rise +$29, and +$32 to $05DC.
; HOW: ApproachSixteenthMin16Byte and ApproachEighthMin8 with the start
; in direct page $3E.
; Entry: M 8-bit, X 16-bit; X = the piece; D = $0000. Exit: M 8-bit; A =
; +$32; Y = the parent.
%at($08DB90)
ObjectSprayPieceStep:
    lda.w !obj_14,x
    sta.b !scale_scratch
    lda.b #$00
    jsl.l ApproachSixteenthMin16Byte
    sta.w !obj_14,x
    lda.w !obj_15,x
    clc
    adc.b #$08
    sta.w !obj_15,x
    ldy.w !obj_27,x
    jsl.l ObjectCopyPositionFrom
    jsl.l ObjectApplyVelocity
    rep.b #$20
    lda.w !obj_vel_y,x
    sta.b !scale_scratch
    lda.w !obj_29,x
    jsl.l ApproachEighthMin8
    sta.w !obj_vel_y,x
    sep.b #$20
    rep.b #$20
    lda.w !obj_vel_z,x
    sta.b !scale_scratch
    lda.w #$05DC
    jsl.l ApproachEighthMin8
    sta.w !obj_vel_z,x
    sep.b #$20
    rtl

; @routine 08:DBD9 ObjectPlayerPassCount
; WHAT IT DOES: Counts the player ($0346) past object X once: unless the
; object's +$25 bit 0 is set already, when the player is less than $1F4
; from it along +$10 the player's +$4C goes up by one and the object's
; +$25 bit 0 is set. Nothing in the game calls it; a direct call is its
; only way in.
; HOW: The distance as |player - object| (EOR #$FFFF, INC), compared
; signed (BPL); a CMP #1 and both its branches to the same place.
; Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; Y = the player
; unless the bit was set; A and the flags as the last step left them.
%at($08DBD9)
ObjectPlayerPassCount:
    lda.w !obj_25,x
    and.b #$01
    beq .lDBE4
    jml.l .lDC12
.lDBE4:
    ldy.w !wram_0346
    rep.b #$20
    lda.w !obj_pos_z,y
    sec
    sbc.w !obj_pos_z,x
    bpl .lDBF6
    eor.w #$FFFF
    inc a
.lDBF6:
    cmp.w #$01F4
    sep.b #$20
    bpl .lDC12
    lda.w !obj_4b+1,y
    inc a
    sta.w !obj_4b+1,y
    cmp.b #$01
    beq .lDC0A
    bra .lDC0A
.lDC0A:
    lda.w !obj_25,x
    ora.b #$01
    sta.w !obj_25,x
.lDC12:
    rtl

; @routine 08:843C ObjectCameraPad
; WHAT IT DOES: Sets up a pad that resets the camera of a player touching
; it: its handler becomes $08:844F (ObjectCameraPadStep), then the shared
; set-up (ObjectCameraPadSetup). Nothing in the game calls it; a direct
; call is its only way in.
; HOW: A JML to the set-up.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as
; ObjectCameraPadSetup.
%at($08843C)
ObjectCameraPad:
    rep.b #$20
    lda.w #$844F
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,x
    jml.l ObjectCameraPadSetup

; @routine 08:844F ObjectCameraPadStep
; WHAT IT DOES: A tick of ObjectCameraPad: $1F4C, $1F4E and $1F50 set to
; 0 and given to the camera point of every player touching it
; (ObjectContactsCameraPoint).
; HOW: Three 16-bit stores of 0, then the call.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as
; ObjectContactsCameraPoint.
%at($08844F)
ObjectCameraPadStep:
    rep.b #$20
    lda.w #$0000
    sta.w !wram_1F4C
    sep.b #$20
    rep.b #$20
    lda.w #$0000
    sta.w !wram_1F4E
    sep.b #$20
    rep.b #$20
    lda.w #$0000
    sta.w !wram_1F50
    sep.b #$20
    jsl.l ObjectContactsCameraPoint
    rtl

; @routine 08:8472 ObjectCameraPadB
; WHAT IT DOES: ObjectCameraPad with its own copy of the handler,
; $08:8485 (ObjectCameraPadStepB). Nothing in the game calls it.
; HOW: As ObjectCameraPad.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as
; ObjectCameraPadSetup.
%at($088472)
ObjectCameraPadB:
    rep.b #$20
    lda.w #$8485
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,x
    jml.l ObjectCameraPadSetup

; @routine 08:8485 ObjectCameraPadStepB
; WHAT IT DOES: ObjectCameraPadB's copy of ObjectCameraPadStep.
; HOW: As ObjectCameraPadStep.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as
; ObjectContactsCameraPoint.
%at($088485)
ObjectCameraPadStepB:
    rep.b #$20
    lda.w #$0000
    sta.w !wram_1F4C
    sep.b #$20
    rep.b #$20
    lda.w #$0000
    sta.w !wram_1F4E
    sep.b #$20
    rep.b #$20
    lda.w #$0000
    sta.w !wram_1F50
    sep.b #$20
    jsl.l ObjectContactsCameraPoint
    rtl

; @routine 08:84A8 ObjectCameraPadSetup
; WHAT IT DOES: The set-up both camera pads jump to: the record handler
; (+$1D, +$1F) becomes $08:84B7 (ObjectCameraPadRecord), which then runs.
; HOW: Runs on into ObjectCameraPadRecord.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as
; ObjectCameraPadRecord.
%at($0884A8)
ObjectCameraPadSetup:
    rep.b #$20
    lda.w #$84B7
    sta.w !obj_1d,x
    sep.b #$20
    lda.b #$08
    sta.w !obj_1f,x

; @routine 08:84B7 ObjectCameraPadRecord
; WHAT IT DOES: The Super FX record of a camera pad (ObjectCameraPad,
; ObjectCameraPadB): a new record (GsuRecordNew42D) with bit 4 of its
; flags (GsuRecordFlag10), the three sizes in direct page $02, $08 and
; $94 all $0780, and its position (GsuRecordPosition). It is the record
; handler ObjectCameraPadSetup stores at +$1D.
; HOW: Three 16-bit stores of $0780 between the calls.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as
; GsuRecordPosition.
%at($0884B7)
ObjectCameraPadRecord:
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

; @routine 08:84DF ObjectContactsCameraPoint
; WHAT IT DOES: Takes object X's position back from its record
; (ObjectSyncFromRecord) and, when the record has contacts, walks them
; (RecordNextContact): for each with bit 2 of its flags, the player its
; bit 0 or bit 1 names ($0346 or $034A) gets $1F4C, $1F4E and $1F50 as
; +$2C, +$2E and +$30 of its camera point (the link, +$4F, of its link),
; when it has a link.
; HOW: X kept on the stack (PHX, PLX) while it is the player and then
; its link; BIT #imm tests of the flags in A; the stores 16-bit through
; the data bank with Y.
; Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: M 8-bit; X = the
; object; carry clear; A and the rest as the last call left them.
%at($0884DF)
ObjectContactsCameraPoint:
    jsl.l ObjectSyncFromRecord
    and.b #$01
    beq .l8524
.l84E7:
    jsl.l RecordNextContact
    bcc .l8524
    bit.b #$04
    beq .l84E7
    phx
    bit.b #$01
    beq .l84FB
    ldx.w !wram_0346
    bra .l8502
.l84FB:
    bit.b #$02
    beq .l8521
    ldx.w !wram_034A
.l8502:
    ldy.w !obj_link_w,x
    beq .l8521
    tyx
    ldy.w !obj_link_w,x
    rep.b #$20
    lda.w !wram_1F4C
    sta.w !dp_2c,y
    lda.w !wram_1F4E
    sta.w !dp_2e,y
    lda.w !wram_1F50
    sta.w !dp_30,y
    sep.b #$20
.l8521:
    plx
    bra .l84E7
.l8524:
    rtl
