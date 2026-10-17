; replay_camera.asm: the replay camera director's handlers of bank $08
; (08:8F03-958E): a director object moves the camera actor its +$4F links
; to around its car (+$06); each handler sets the next frame and falls into
; it, frames ending in the director's tail (CameraDirectorTail) or going
; back to its table (CameraDirectorTableStart).
; C: src/stuntracefx/object/replay_camera.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

; @routine 08:8F03 CameraSnapToCar
; WHAT IT DOES: Puts the camera back on its car at once: mode 1 for the
; moment, its turn words +$26-$30 cleared, +$1A-$1E from +$20-$24, one
; CameraFollowLink; then its mode word +$00 as it was.
; HOW: The mode word kept on the stack (PHA/PLA, 16-bit).
; Entry: M 8-bit, X 16-bit; X = the director. Exit: M 8-bit; A = the mode
; word; N, Z from it.
%at($088F03)
CameraSnapToCar:
    ldy.w !obj_link_w,x
    rep.b #$20
    lda.w !bank0_base,y
    pha
    sep.b #$20
    ldy.w !obj_link_w,x
    rep.b #$20
    lda.w #$0001
    sta.w !bank0_base,y
    sep.b #$20
    ldy.w !obj_link_w,x
    rep.b #$20
    lda.w #$0000
    sta.w !fxrec_link_422,y
    sep.b #$20
    rep.b #$20
    lda.w #$0000
    sta.w !fxcar_roll,y
    sep.b #$20
    rep.b #$20
    lda.w #$0000
    sta.w !dp_2a,y
    sep.b #$20
    ldy.w !obj_link_w,x
    rep.b #$20
    lda.w #$0000
    sta.w !dp_2c,y
    sep.b #$20
    rep.b #$20
    lda.w #$0000
    sta.w !dp_2e,y
    sep.b #$20
    rep.b #$20
    lda.w #$0000
    sta.w !dp_30,y
    sep.b #$20
    ldy.w !obj_link_w,x
    rep.b #$20
    lda.w !actor_20,y
    sta.w !actor_kind,y
    sep.b #$20
    rep.b #$20
    lda.w !actor_22,y
    sta.w !actor_1c,y
    sep.b #$20
    rep.b #$20
    lda.w !actor_palette,y
    sta.w !actor_1e,y
    sep.b #$20
    jsl.l CameraFollowLink
    ldy.w !obj_link_w,x
    rep.b #$20
    pla
    sta.w !bank0_base,y
    sep.b #$20
    rts

; @routine 08:8F8E CameraOverhead
; WHAT IT DOES: The overhead shot: the director at the car's X and Z ($12C
; nearer) at height -$3E8, the camera's point there too, mode 3, one
; CameraFollowLink; frame $08:8FFE (CameraOverheadStep), +$48 = $0A; then
; that frame.
; HOW: The positions through $1F4C-$1F50 (16-bit). Reached only by
; unit_replay_camera_08.
; Entry: M 8-bit, X 16-bit; X = the director; D = $0000. Exit: as the frame
; leaves it.
%at($088F8E)
CameraOverhead:
    ldy.w !obj_link_w,x
    rep.b #$20
    lda.w !dp_02,y
    tay
    lda.w !obj_pos_x,y
    sta.w !wram_1F4C
    sta.w !obj_pos_x,x
    lda.w #$FC18
    sta.w !wram_1F4E
    sta.w !obj_pos_y,x
    lda.w !obj_pos_z,y
    clc
    adc.w #$FED4
    sta.w !wram_1F50
    sta.w !obj_pos_z,x
    sep.b #$20
    ldy.w !obj_link_w,x
    rep.b #$20
    lda.w !wram_1F4C
    sta.w !dp_08,y
    sep.b #$20
    rep.b #$20
    lda.w !wram_1F4E
    sta.w !actor_0a,y
    sep.b #$20
    rep.b #$20
    lda.w !wram_1F50
    sta.w !actor_0c,y
    sep.b #$20
    ldy.w !obj_link_w,x
    rep.b #$20
    lda.w #$0003
    sta.w !bank0_base,y
    sep.b #$20
    jsl.l CameraFollowLink
    rep.b #$20
    lda.w #$8FFE
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,x
    lda.b #$0A
    sta.w !obj_48,x

; @routine 08:8FFE CameraOverheadStep
; WHAT IT DOES: The overhead shot's frame: while +$48 counts down the car's
; record +$42 gets bits 4 and 12, after it bit 12 goes; on a cue (+$25 bit
; 0, cleared) the camera's +$34 flips between 1 (sound $24, height -$7D0)
; and 0 (sound $2F, height -$3E8); the camera's height +$0A moves half way
; to the director's (rounded toward it) and its X/Z three quarters of the
; way from the director's toward the car's (ScaleThreeQuarters); then the
; director's tail.
; HOW: 16-bit SBC, CMP #$8000 and ROR for the signed half; the X and Z
; stored through the direct page with X = the camera.
; Entry: M 8-bit, X 16-bit; X = the director; D = $0000. Exit: as the tail
; leaves it.
%at($088FFE)
CameraOverheadStep:
    lda.w !obj_48,x
    bne .l9007
    jml.l .l9024
.l9007:
    dec.w !obj_48,x
    rep.b #$20
    phx
    ldy.w !obj_06,x
    lda.w !obj_22,y
    tax
    lda.l !gsu_rec_42,x
    ora.w #$1010
    sta.l !gsu_rec_42,x
    sep.b #$20
    plx
    bra .l903C
.l9024:
    rep.b #$20
    phx
    ldy.w !obj_06,x
    lda.w !obj_22,y
    tax
    lda.l !gsu_rec_42,x
    and.w #$EFFF
    sta.l !gsu_rec_42,x
    sep.b #$20
    plx
.l903C:
    ldy.w !obj_link_w,x
    lda.w !obj_25,x
    and.b #$01
    bne .l904A
    jml.l .l9083
.l904A:
    lda.w !obj_25,x
    and.b #$FE
    sta.w !obj_25,x
    lda.w !dp_34,y
    inc a
    cmp.b #$02
    bmi .l905C
    lda.b #$00
.l905C:
    sta.w !dp_34,y
    beq .l9073
    lda.b #$24
    jsl.l SoundPort0QueuePush
    rep.b #$20
    lda.w #$F830
    sta.w !obj_pos_y,x
    sep.b #$20
    bra .l9083
.l9073:
    lda.b #$2F
    jsl.l SoundPort0QueuePush
    rep.b #$20
    lda.w #$FC18
    sta.w !obj_pos_y,x
    sep.b #$20
.l9083:
    rep.b #$20
    lda.w !obj_pos_y,x
    sec
    sbc.w !actor_0a,y
    cmp.w #$8000
    ror a
    bpl .l9095
    adc.w #$0000
.l9095:
    clc
    adc.w !actor_0a,y
    sta.w !actor_0a,y
    lda.w !obj_pos_x,x
    sta.w !wram_1F4C
    lda.w !obj_pos_z,x
    sta.w !wram_1F50
    phx
    ldy.w !obj_link_w,x
    lda.w !dp_02,y
    tyx
    tay
    lda.w !obj_pos_x,y
    sec
    sbc.w !wram_1F4C
    jsl.l ScaleThreeQuarters
    clc
    adc.w !wram_1F4C
    sta.b !dp_08,x
    lda.w !obj_pos_z,y
    sec
    sbc.w !wram_1F50
    jsl.l ScaleThreeQuarters
    clc
    adc.w !wram_1F50
    sta.b !actor_0c,x
    sep.b #$20
    plx
    jmp.w CameraDirectorTail

; @routine 08:90D9 CameraSnapToTable
; WHAT IT DOES: The camera put back on its car (mode 1, turn words cleared,
; CameraFollowLink), its +$04 cleared, mode $0755, frame $08:9176
; (CameraTableStep); then that frame.
; HOW: Word stores. Reached only by unit_replay_camera_08.
; Entry: M 8-bit, X 16-bit; X = the director. Exit: as the frame leaves it.
%at($0890D9)
CameraSnapToTable:
    ldy.w !obj_link_w,x
    rep.b #$20
    lda.w #$0001
    sta.w !bank0_base,y
    sep.b #$20
    ldy.w !obj_link_w,x
    rep.b #$20
    lda.w #$0000
    sta.w !fxrec_link_422,y
    sep.b #$20
    rep.b #$20
    lda.w #$0000
    sta.w !fxcar_roll,y
    sep.b #$20
    rep.b #$20
    lda.w #$0000
    sta.w !dp_2a,y
    sep.b #$20
    ldy.w !obj_link_w,x
    rep.b #$20
    lda.w #$0000
    sta.w !dp_2c,y
    sep.b #$20
    rep.b #$20
    lda.w #$0000
    sta.w !dp_2e,y
    sep.b #$20
    rep.b #$20
    lda.w #$0000
    sta.w !dp_30,y
    sep.b #$20
    ldy.w !obj_link_w,x
    rep.b #$20
    lda.w !actor_20,y
    sta.w !actor_kind,y
    sep.b #$20
    rep.b #$20
    lda.w !actor_22,y
    sta.w !actor_1c,y
    sep.b #$20
    rep.b #$20
    lda.w !actor_palette,y
    sta.w !actor_1e,y
    sep.b #$20
    jsl.l CameraFollowLink
    ldy.w !obj_link_w,x
    rep.b #$20
    lda.w #$0000
    sta.w !dp_04,y
    sep.b #$20
    ldy.w !obj_link_w,x
    rep.b #$20
    lda.w #$0755
    sta.w !bank0_base,y
    sep.b #$20
    rep.b #$20
    lda.w #$9176
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,x

; @routine 08:9176 CameraTableStep
; WHAT IT DOES: A frame: the camera's point from the director's table
; (CameraPointFromTable), then the director's tail.
; HOW: JSL, JMP to 08:8EF9.
; Entry: M 8-bit, X 16-bit; X = the director. Exit: as the tail leaves it.
%at($089176)
CameraTableStep:
    jsl.l CameraPointFromTable
    jmp.w CameraDirectorTail

; @routine 08:917D CameraPathA
; WHAT IT DOES: A path shot: frame $08:91D0 (CameraPathStep), mode 1, the
; director on path $885F (ObjectPathStart), CameraFollowLink; then that
; frame.
; HOW: Word stores; a JMP into the shared start at 08:91C8.
; Entry: M 8-bit, X 16-bit; X = the director. Exit: as the frame leaves it.
%at($08917D)
CameraPathA:
    rep.b #$20
    lda.w #$91D0
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,x
    ldy.w !obj_link_w,x
    rep.b #$20
    lda.w #$0001
    sta.w !bank0_base,y
    sep.b #$20
    ldy.w #$885F
    jmp.w CameraPathB_l91C8

; @routine 08:919F CameraPathB
; WHAT IT DOES: As CameraPathA on path $882D, or $85B9 in HUD mode 3.
; HOW: CMP #3 on $0D2B.
; Entry: as CameraPathA. Exit: as CameraPathA.
%at($08919F)
CameraPathB:
    rep.b #$20
    lda.w #$91D0
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,x
    ldy.w !obj_link_w,x
    rep.b #$20
    lda.w #$0001
    sta.w !bank0_base,y
    sep.b #$20
    ldy.w #$882D
    lda.w !hud_mode
    cmp.b #$03
    bne .l91C8
    ldy.w #$85B9
.l91C8:
    jsl.l ObjectPathStart
    jsl.l CameraFollowLink

; @routine 08:91D0 CameraPathStep
; WHAT IT DOES: The path shot's frame: the director moves on along its path
; (CameraApproachTarget); once there, $70:273C bit 0 is set and the
; director goes back to its table (08:8EC1); else the director's tail.
; HOW: Carry from CameraApproachTarget; a 16-bit long OR.
; Entry: M 8-bit, X 16-bit; X = the director. Exit: as the path leaves it.
%at($0891D0)
CameraPathStep:
    jsl.l CameraApproachTarget
    bcs .l91D9
    brl CameraDirectorTail
.l91D9:
    rep.b #$20
    lda.l !fx_init_273c
    ora.w #$0001
    sta.l !fx_init_273c
    sep.b #$20
    jmp.w CameraDirectorTableStart

; @routine 08:91EB CameraDrift
; WHAT IT DOES: The drift shot: frame $08:921D (CameraDriftStep), mode 3,
; +$48 = $1E, the director's velocity from its car's record
; (ObjectVelocityFromRecordOf) with no Y; then that frame.
; HOW: Word and byte stores.
; Entry: M 8-bit, X 16-bit; X = the director. Exit: as the frame leaves it.
%at($0891EB)
CameraDrift:
    rep.b #$20
    lda.w #$921D
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,x
    ldy.w !obj_link_w,x
    rep.b #$20
    lda.w #$0003
    sta.w !bank0_base,y
    sep.b #$20
    lda.b #$1E
    sta.w !obj_48,x
    ldy.w !obj_06,x
    jsl.l ObjectVelocityFromRecordOf
    rep.b #$20
    lda.w #$0000
    sta.w !obj_vel_y,x
    sep.b #$20

; @routine 08:921D CameraDriftStep
; WHAT IT DOES: The drift shot's frame: the camera's point moves by the
; director's velocity, which eases toward 0 (ApproachEighthMin8); while
; +$48 counts down the director's tail, then CameraDirectorSkip2.
; HOW: 16-bit ADCs; ApproachEighthMin8 through $3E; JML.
; Entry: M 8-bit, X 16-bit; X = the director. Exit: as the path leaves it.
%at($08921D)
CameraDriftStep:
    rep.b #$20
    ldy.w !obj_link_w,x
    lda.w !obj_vel_x,x
    clc
    adc.w !dp_08,y
    sta.w !dp_08,y
    lda.w !obj_vel_y,x
    clc
    adc.w !actor_0a,y
    sta.w !actor_0a,y
    lda.w !obj_vel_z,x
    clc
    adc.w !actor_0c,y
    sta.w !actor_0c,y
    sep.b #$20
    rep.b #$20
    lda.w !obj_vel_x,x
    sta.b !scale_scratch
    lda.w #$0000
    jsl.l ApproachEighthMin8
    sta.w !obj_vel_x,x
    sep.b #$20
    rep.b #$20
    lda.w !obj_vel_y,x
    sta.b !scale_scratch
    lda.w #$0000
    jsl.l ApproachEighthMin8
    sta.w !obj_vel_y,x
    sep.b #$20
    rep.b #$20
    lda.w !obj_vel_z,x
    sta.b !scale_scratch
    lda.w #$0000
    jsl.l ApproachEighthMin8
    sta.w !obj_vel_z,x
    sep.b #$20
    lda.w !obj_48,x
    bne .l9284
    jml.l CameraDirectorSkip2
.l9284:
    dec.w !obj_48,x
    jmp.w CameraDirectorTail

; @routine 08:928A CameraFixedAhead
; WHAT IT DOES: A fixed shot: frame $08:92DC (CameraHoldStep), mode 3, the
; camera's point at the car's track table entry two on (GsuTableEntryNext
; twice, GsuTableEntryTo1F4C), $64 up; then the tail.
; HOW: The table pointer from the car's record +$58. Reached only by
; unit_replay_camera_08.
; Entry: M 8-bit, X 16-bit; X = the director. Exit: as the tail leaves it.
%at($08928A)
CameraFixedAhead:
    rep.b #$20
    lda.w #$92DC
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
    jsl.l GsuTableEntryNext
    jsl.l GsuTableEntryTo1F4C
    ldy.w !obj_link_w,x
    lda.w !wram_1F4C
    sta.w !dp_08,y
    lda.w !wram_1F4E
    clc
    adc.w #$FF9C
    sta.w !actor_0a,y
    lda.w !wram_1F50
    sta.w !actor_0c,y
    sep.b #$20

; @routine 08:92DC CameraHoldStep
; WHAT IT DOES: A held shot's frame: the director's tail only.
; HOW: JMP to 08:8EF9.
; Entry: M 8-bit, X 16-bit; X = the director. Exit: as the tail leaves it.
%at($0892DC)
CameraHoldStep:
    jmp.w CameraDirectorTail

; @routine 08:92DF CameraDriftLong
; WHAT IT DOES: As CameraDrift with +$48 = $28 and frame $08:9311
; (CameraDriftLongStep).
; HOW: As CameraDrift. Reached only by unit_replay_camera_08.
; Entry: as CameraDrift. Exit: as CameraDrift.
%at($0892DF)
CameraDriftLong:
    rep.b #$20
    lda.w #$9311
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,x
    ldy.w !obj_link_w,x
    rep.b #$20
    lda.w #$0003
    sta.w !bank0_base,y
    sep.b #$20
    lda.b #$28
    sta.w !obj_48,x
    ldy.w !obj_06,x
    jsl.l ObjectVelocityFromRecordOf
    rep.b #$20
    lda.w #$0000
    sta.w !obj_vel_y,x
    sep.b #$20

; @routine 08:9311 CameraDriftLongStep
; WHAT IT DOES: As CameraDriftStep (its own copy of the code).
; HOW: As CameraDriftStep. Reached only by unit_replay_camera_08.
; Entry: as CameraDriftStep. Exit: as CameraDriftStep.
%at($089311)
CameraDriftLongStep:
    rep.b #$20
    ldy.w !obj_link_w,x
    lda.w !obj_vel_x,x
    clc
    adc.w !dp_08,y
    sta.w !dp_08,y
    lda.w !obj_vel_y,x
    clc
    adc.w !actor_0a,y
    sta.w !actor_0a,y
    lda.w !obj_vel_z,x
    clc
    adc.w !actor_0c,y
    sta.w !actor_0c,y
    sep.b #$20
    rep.b #$20
    lda.w !obj_vel_x,x
    sta.b !scale_scratch
    lda.w #$0000
    jsl.l ApproachEighthMin8
    sta.w !obj_vel_x,x
    sep.b #$20
    rep.b #$20
    lda.w !obj_vel_y,x
    sta.b !scale_scratch
    lda.w #$0000
    jsl.l ApproachEighthMin8
    sta.w !obj_vel_y,x
    sep.b #$20
    rep.b #$20
    lda.w !obj_vel_z,x
    sta.b !scale_scratch
    lda.w #$0000
    jsl.l ApproachEighthMin8
    sta.w !obj_vel_z,x
    sep.b #$20
    lda.w !obj_48,x
    bne .l9378
    jml.l CameraDirectorSkip2
.l9378:
    dec.w !obj_48,x
    jmp.w CameraDirectorTail

; @routine 08:937E CameraFixedSide
; WHAT IT DOES: A fixed shot: frame $08:93D4 (CameraHoldStep2), mode 3, the
; camera's point at the car's next track table entry moved ($3E8, -$64,
; $3E8); then the tail.
; HOW: As CameraFixedAhead with one GsuTableEntryNext. Reached only by
; unit_replay_camera_08.
; Entry: as CameraFixedAhead. Exit: as CameraFixedAhead.
%at($08937E)
CameraFixedSide:
    rep.b #$20
    lda.w #$93D4
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
    jsl.l GsuTableEntryTo1F4C
    ldy.w !obj_link_w,x
    lda.w !wram_1F4C
    clc
    adc.w #$03E8
    sta.w !dp_08,y
    lda.w !wram_1F4E
    clc
    adc.w #$FF9C
    sta.w !actor_0a,y
    lda.w !wram_1F50
    clc
    adc.w #$03E8
    sta.w !actor_0c,y
    sep.b #$20

; @routine 08:93D4 CameraHoldStep2
; WHAT IT DOES: As CameraHoldStep (its own JMP).
; HOW: JMP to 08:8EF9. Reached only by unit_replay_camera_08.
; Entry: as CameraHoldStep. Exit: as CameraHoldStep.
%at($0893D4)
CameraHoldStep2:
    jmp.w CameraDirectorTail

; @routine 08:93D7 CameraRise
; WHAT IT DOES: The rising drift: frame $08:93FF (CameraRiseStep), mode 3,
; +$48 = $28, the director's velocity from its car's record; then that
; frame.
; HOW: Word and byte stores.
; Entry: M 8-bit, X 16-bit; X = the director. Exit: as the frame leaves it.
%at($0893D7)
CameraRise:
    rep.b #$20
    lda.w #$93FF
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,x
    ldy.w !obj_link_w,x
    rep.b #$20
    lda.w #$0003
    sta.w !bank0_base,y
    sep.b #$20
    lda.b #$28
    sta.w !obj_48,x
    ldy.w !obj_06,x
    jsl.l ObjectVelocityFromRecordOf

; @routine 08:93FF CameraRiseStep
; WHAT IT DOES: The rising drift's frame: as CameraDriftStep with the Y
; velocity eased toward -$0A; when +$48 runs out, CameraZoomIn.
; HOW: As CameraDriftStep; a JML into CameraZoomIn.
; Entry: as CameraDriftStep. Exit: as the path leaves it.
%at($0893FF)
CameraRiseStep:
    rep.b #$20
    ldy.w !obj_link_w,x
    lda.w !obj_vel_x,x
    clc
    adc.w !dp_08,y
    sta.w !dp_08,y
    lda.w !obj_vel_y,x
    clc
    adc.w !actor_0a,y
    sta.w !actor_0a,y
    lda.w !obj_vel_z,x
    clc
    adc.w !actor_0c,y
    sta.w !actor_0c,y
    sep.b #$20
    rep.b #$20
    lda.w !obj_vel_x,x
    sta.b !scale_scratch
    lda.w #$0000
    jsl.l ApproachEighthMin8
    sta.w !obj_vel_x,x
    sep.b #$20
    rep.b #$20
    lda.w !obj_vel_y,x
    sta.b !scale_scratch
    lda.w #$FFF6
    jsl.l ApproachEighthMin8
    sta.w !obj_vel_y,x
    sep.b #$20
    rep.b #$20
    lda.w !obj_vel_z,x
    sta.b !scale_scratch
    lda.w #$0000
    jsl.l ApproachEighthMin8
    sta.w !obj_vel_z,x
    sep.b #$20
    lda.w !obj_48,x
    bne .l9466
    jml.l CameraZoomIn
.l9466:
    dec.w !obj_48,x
    jmp.w CameraDirectorTail

; @routine 08:946C CameraZoomIn
; WHAT IT DOES: The zoom-in: frame $08:9488 (CameraZoomInStep), mode 2;
; then that frame.
; HOW: Word stores.
; Entry: M 8-bit, X 16-bit; X = the director. Exit: as the frame leaves it.
%at($08946C)
CameraZoomIn:
    rep.b #$20
    lda.w #$9488
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,x
    ldy.w !obj_link_w,x
    rep.b #$20
    lda.w #$0002
    sta.w !bank0_base,y
    sep.b #$20

; @routine 08:9488 CameraZoomInStep
; WHAT IT DOES: The zoom-in's frame: the camera's zoom +$0E eased toward
; $1000 (ApproachThirtySecondMin32); the tail.
; HOW: Through $3E (16-bit).
; Entry: M 8-bit, X 16-bit; X = the director. Exit: as the tail leaves it.
%at($089488)
CameraZoomInStep:
    ldy.w !obj_link_w,x
    rep.b #$20
    lda.w !actor_0e,y
    sta.b !scale_scratch
    lda.w #$1000
    jsl.l ApproachThirtySecondMin32
    sta.w !actor_0e,y
    sep.b #$20
    jmp.w CameraDirectorTail

; @routine 08:94A1 CameraOrbit
; WHAT IT DOES: The orbit shot: frame $08:94DD (CameraOrbitStep), mode
; $0655, the director on path $88BB (ObjectPathStart), the car's record
; +$40 bit 2 set; then that frame.
; HOW: Word stores; the record word through X (PHX/PLX, 16-bit).
; Entry: M 8-bit, X 16-bit; X = the director. Exit: as the frame leaves it.
%at($0894A1)
CameraOrbit:
    rep.b #$20
    lda.w #$94DD
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,x
    ldy.w !obj_link_w,x
    rep.b #$20
    lda.w #$0655
    sta.w !bank0_base,y
    sep.b #$20
    ldy.w #$88BB
    jsl.l ObjectPathStart
    phx
    rep.b #$20
    lda.w !obj_06,x
    tax
    lda.w !obj_22,x
    tax
    lda.l !gsu_rec_40,x
    ora.w #$0004
    sta.l !gsu_rec_40,x
    sep.b #$20
    plx

; @routine 08:94DD CameraOrbitStep
; WHAT IT DOES: The orbit shot's frame: the director moves along its path
; (CameraApproachTarget); once there it runs into CameraZoomOut, else the
; tail.
; HOW: Carry from CameraApproachTarget; the fall into 08:94E6.
; Entry: M 8-bit, X 16-bit; X = the director. Exit: as the path leaves it.
%at($0894DD)
CameraOrbitStep:
    jsl.l CameraApproachTarget
    bcs CameraZoomOut
    jmp.w CameraDirectorTail

; @routine 08:94E6 CameraZoomOut
; WHAT IT DOES: The zoom-out: frame $08:9502 (CameraZoomOutStep), mode
; $0654; then that frame.
; HOW: Word stores.
; Entry: M 8-bit, X 16-bit; X = the director. Exit: as the frame leaves it.
%at($0894E6)
CameraZoomOut:
    rep.b #$20
    lda.w #$9502
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,x
    ldy.w !obj_link_w,x
    rep.b #$20
    lda.w #$0654
    sta.w !bank0_base,y
    sep.b #$20

; @routine 08:9502 CameraZoomOutStep
; WHAT IT DOES: The zoom-out's frame: the zoom +$0E eased toward $2000
; (ApproachSixtyFourthMin64); the tail.
; HOW: Through $3E (16-bit).
; Entry: as CameraZoomInStep. Exit: as CameraZoomInStep.
%at($089502)
CameraZoomOutStep:
    ldy.w !obj_link_w,x
    rep.b #$20
    lda.w !actor_0e,y
    sta.b !scale_scratch
    lda.w #$2000
    jsl.l ApproachSixtyFourthMin64
    sta.w !actor_0e,y
    sep.b #$20
    jmp.w CameraDirectorTail

; @routine 08:951B CameraStill
; WHAT IT DOES: The still shot: frame $08:953C (CameraStillStep), +$48 =
; $0A, the camera's +$37 word = $8000; then that frame.
; HOW: Word and byte stores. Reached only by unit_replay_camera_08.
; Entry: M 8-bit, X 16-bit; X = the director. Exit: as the frame leaves it.
%at($08951B)
CameraStill:
    rep.b #$20
    lda.w #$953C
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,x
    lda.b #$0A
    sta.w !obj_48,x
    ldy.w !obj_link_w,x
    rep.b #$20
    lda.w #$8000
    sta.w !fxpart_world+1,y
    sep.b #$20

; @routine 08:953C CameraStillStep
; WHAT IT DOES: The still shot's frame: the car's record +$42 = $0410;
; while +$48 counts down, the camera's point from the table
; (CameraPointFromTable) and the tail; then CameraEnd.
; HOW: The record word through X (PHX, TYX, PLX); a JML into CameraEnd.
; Reached only by unit_replay_camera_08.
; Entry: M 8-bit, X 16-bit; X = the director. Exit: as the path leaves it.
%at($08953C)
CameraStillStep:
    ldy.w !obj_06,x
    phx
    tyx
    rep.b #$20
    lda.w !obj_22,x
    tax
    lda.w #$0410
    sta.l !gsu_rec_42,x
    sep.b #$20
    plx
    lda.w !obj_48,x
    bne .l955A
    jml.l CameraEnd
.l955A:
    dec.w !obj_48,x
    jsl.l CameraPointFromTable
    jmp.w CameraDirectorTail

; @routine 08:9564 CameraEnd
; WHAT IT DOES: The end of the replay shot: frame $08:958C (CameraEndStep),
; the car's record +$1C = $10 (read first); then the tail.
; HOW: The record word through X (PHX, TYX, PLX). Reached only by
; unit_replay_camera_08.
; Entry: M 8-bit, X 16-bit; X = the director. Exit: as the tail leaves it.
%at($089564)
CameraEnd:
    rep.b #$20
    lda.w #$958C
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,x
    ldy.w !obj_06,x
    phx
    tyx
    rep.b #$20
    lda.w !obj_22,x
    tax
    lda.l !gsu_rec_1c,x
    lda.w #$0010
    sta.l !gsu_rec_1c,x
    sep.b #$20
    plx

; @routine 08:958C CameraEndStep
; WHAT IT DOES: The end shot's frame: the director's tail only.
; HOW: JMP to 08:8EF9. Reached only by unit_replay_camera_08.
; Entry: M 8-bit, X 16-bit; X = the director. Exit: as the tail leaves it.
%at($08958C)
CameraEndStep:
    jmp.w CameraDirectorTail
