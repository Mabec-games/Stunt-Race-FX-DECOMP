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

; @routine 08:95A0 CameraFollowCar
; WHAT IT DOES: The follow shot: view $05FA = 5, frame $08:95C7
; (CameraFollowCarStep), a record of its own (GsuRecordNewCamera), one
; CameraFollowStep, then synced to its record.
; HOW: PHX/PHB around CameraFollowStep. Reached only by
; unit_replay_camera_08.
; Entry: M 8-bit, X 16-bit; X = the director; D = $0000. Exit: as
; ObjectSyncToRecord leaves it.
%at($0895A0)
CameraFollowCar:
    lda.b #$05
    sta.w !wram_05FA
    rep.b #$20
    lda.w #$95C7
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,x
    jsl.l GsuRecordNewCamera
    phx
    phb
    jsl.l CameraFollowStep
    sep.b #$20
    plb
    plx
    jsl.l ObjectSyncToRecord
    rtl

; @routine 08:95C7 CameraFollowCarStep
; WHAT IT DOES: The follower's frame: the camera point (direct page
; $C5-$C9) kept at $1F64-$1F68, CameraFollowStep; its record placed a
; quarter of $B7/$BB from the car (+$06) and $C8 up (the old place kept
; at $1F4C-$1F50), synced (ObjectSyncAngles, $1F41 = $FF); touching
; something (contact flags 3-4) puts the camera point at the old place
; $C8 higher, no contacts puts it back; ObjectFaceTied, +$36 = 3.
; HOW: 16-bit long record accesses; CMP #$8000/ROR twice for the
; quarters. Reached only by unit_replay_camera_08.
; Entry: M 8-bit, X 16-bit; X = the director; D = $0000. Exit: A low byte
; = 3.
%at($0895C7)
CameraFollowCarStep:
    ldy.w !obj_06,x
    stz.w !wram_1F3C+5
    rep.b #$20
    lda.b !dp_c5
    sta.w !wram_1F5A+10
    sep.b #$20
    rep.b #$20
    lda.b !dp_c7
    sta.w !wram_1F5A+12
    sep.b #$20
    rep.b #$20
    lda.b !dp_c9
    sta.w !wram_1F5A+14
    sep.b #$20
    phx
    phb
    jsl.l CameraFollowStep
    sep.b #$20
    plb
    plx
    rep.b #$20
    txy
    lda.w !obj_22,x
    tax
    lda.l !gsu_rec_1e,x
    sta.w !wram_1F4C
    lda.l !gsu_in_20,x
    sta.w !wram_1F4E
    lda.l !gsu_in_22,x
    sta.w !wram_1F50
    lda.b !dp_b7
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    clc
    adc.w !obj_pos_x,y
    sta.l !gsu_rec_1e,x
    lda.w #$00C8
    clc
    adc.w !obj_pos_y,y
    sta.l !gsu_in_20,x
    lda.b !dp_bb
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    clc
    adc.w !obj_pos_z,y
    sta.l !gsu_in_22,x
    tyx
    sep.b #$20
    lda.b #$FF
    sta.w !wram_1F3C+5
    jsl.l ObjectSyncAngles
    and.b #$01
    beq .l967E
    jsl.l RecordContactSummary
    and.b #$18
    beq .l9699
    rep.b #$20
    lda.w !wram_1F4C
    sta.b !dp_c5
    sep.b #$20
    rep.b #$20
    lda.w !wram_1F4E
    sta.b !dp_c7
    sep.b #$20
    rep.b #$20
    lda.b !dp_c7
    clc
    adc.w #$FF38
    sta.b !dp_c7
    sep.b #$20
    rep.b #$20
    lda.w !wram_1F50
    sta.b !dp_c9
    sep.b #$20
    bra .l9699
.l967E:
    rep.b #$20
    lda.w !wram_1F5A+10
    sta.b !dp_c5
    sep.b #$20
    rep.b #$20
    lda.w !wram_1F5A+12
    sta.b !dp_c7
    sep.b #$20
    rep.b #$20
    lda.w !wram_1F5A+14
    sta.b !dp_c9
    sep.b #$20
.l9699:
    jsl.l ObjectFaceTied
    lda.b #$03
    sta.w !obj_36,x
    rtl

; @routine 08:96A3 CameraLiftOff
; WHAT IT DOES: The lift-off: view 3, the car's track entry +$29 taken,
; frame $08:96D1 (CameraLiftOffStep), +$48 = $96, +$25 bit 0 cleared; then
; that frame.
; HOW: 16-bit copy; word and byte stores.
; Entry: M 8-bit, X 16-bit; X = the director. Exit: as the frame leaves it.
%at($0896A3)
CameraLiftOff:
    lda.b #$03
    sta.w !wram_05FA
    ldy.w !obj_06,x
    rep.b #$20
    lda.w !obj_29,y
    sta.w !obj_29,x
    sep.b #$20
    rep.b #$20
    lda.w #$96D1
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,x
    lda.b #$96
    sta.w !obj_48,x
    lda.w !obj_25,x
    and.b #$FE
    sta.w !obj_25,x

; @routine 08:96D1 CameraLiftOffStep
; WHAT IT DOES: The lift-off's frame: while +$48 counts down the director
; rises $32; once the car (+$06) is no higher than -$3E8 the side shot
; (CameraBeside), else the tail; at 0 back (CameraReturn).
; HOW: 16-bit ADC and CMP; JMLs.
; Entry: M 8-bit, X 16-bit; X = the director. Exit: as the path leaves it.
%at($0896D1)
CameraLiftOffStep:
    lda.w !obj_48,x
    bne .l96DA
    jml.l CameraReturn
.l96DA:
    dec.w !obj_48,x
    rep.b #$20
    lda.w !obj_pos_y,x
    clc
    adc.w #$FFCE
    sta.w !obj_pos_y,x
    sep.b #$20
    ldy.w !obj_06,x
    rep.b #$20
    lda.w !obj_pos_y,y
    cmp.w #$FC18
    sep.b #$20
    bmi .l96FE
    jml.l CameraBeside
.l96FE:
    jml.l CameraDirectorTail

; @routine 08:9702 CameraBeside
; WHAT IT DOES: The side shot: frame $08:9711 (CameraBesideStep); then
; that frame.
; HOW: Word and byte stores; entered by a JML from CameraLiftOffStep.
; Entry: M 8-bit, X 16-bit; X = the director. Exit: as the frame leaves it.
%at($089702)
CameraBeside:
    rep.b #$20
    lda.w #$9711
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,x

; @routine 08:9711 CameraBesideStep
; WHAT IT DOES: The side shot's first frame: the director at the car's
; (+$06) X and Z plus $258 each, height -$C8; frame $08:9787
; (CameraBesideWait); the car's record velocity and +$34-$38 cleared;
; +$48 = $3C; then that frame.
; HOW: 16-bit copies and ADCs; the record words through X (PHX/PLX).
; Entry: M 8-bit, X 16-bit; X = the director. Exit: as the frame leaves it.
%at($089711)
CameraBesideStep:
    ldy.w !obj_06,x
    rep.b #$20
    lda.w !obj_pos_x,y
    sta.w !obj_pos_x,x
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_z,y
    sta.w !obj_pos_z,x
    sep.b #$20
    rep.b #$20
    lda.w #$FF38
    sta.w !obj_pos_y,x
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_x,x
    clc
    adc.w #$0258
    sta.w !obj_pos_x,x
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_z,x
    clc
    adc.w #$0258
    sta.w !obj_pos_z,x
    sep.b #$20
    rep.b #$20
    lda.w #$9787
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,x
    phx
    rep.b #$20
    lda.w !obj_22,y
    tax
    lda.w #$0000
    sta.l !gsu_in_2e,x
    sta.l !gsu_rec_vel_y,x
    sta.l !gsu_rec_vel_z,x
    sta.l !gsu_in_36,x
    sta.l !gsu_in_34,x
    sta.l !gsu_in_38,x
    sep.b #$20
    plx
    lda.b #$3C
    sta.w !obj_48,x

; @routine 08:9787 CameraBesideWait
; WHAT IT DOES: The side shot's frame: while +$48 counts down the
; director's tail, then the track shot (CameraTrackHalf).
; HOW: DEC; JMLs.
; Entry: M 8-bit, X 16-bit; X = the director. Exit: as the path leaves it.
%at($089787)
CameraBesideWait:
    lda.w !obj_48,x
    bne .l9790
    jml.l CameraTrackHalf
.l9790:
    dec.w !obj_48,x
    jml.l CameraDirectorTail

; @routine 08:9797 CameraTrackHalf
; WHAT IT DOES: The track shot: view 2, frame $08:9825 (CameraTrackHalfStep);
; the director half way between the car's track table entry (+$29) and
; the next one ($1F4 higher), turned by the Super FX (ObjectGsu81E3,
; ObjectGsu80BCThen81E3, the result negated); then that frame.
; HOW: GsuTableEntryNext, GsuTableEntryTo1F4C through direct page
; $04/$0A and $05F2; CMP #$8000/ROR for the halves.
; Entry: M 8-bit, X 16-bit; X = the director; D = $0000. Exit: as the frame
; leaves it.
%at($089797)
CameraTrackHalf:
    lda.b #$02
    sta.w !wram_05FA
    rep.b #$20
    lda.w #$9825
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,x
    rep.b #$20
    lda.w !obj_29,x
    jsl.l GsuTableEntryNext
    jsl.l GsuTableEntryTo1F4C
    lda.w !wram_1F4C
    sta.b !dp_04
    lda.w !wram_1F4E
    sta.b !actor_0a
    lda.w !wram_1F50
    sta.w !rot_z
    lda.w !obj_29,x
    jsl.l GsuTableEntryTo1F4C
    lda.w !wram_1F4E
    clc
    adc.w #$FE0C
    sta.w !wram_1F4E
    lda.b !dp_04
    sec
    sbc.w !wram_1F4C
    cmp.w #$8000
    ror a
    clc
    adc.w !wram_1F4C
    sta.w !obj_pos_x,x
    lda.b !actor_0a
    clc
    adc.w #$FE0C
    sec
    sbc.w !wram_1F4E
    cmp.w #$8000
    ror a
    clc
    adc.w !wram_1F4E
    sta.w !obj_pos_y,x
    lda.w !rot_z
    sec
    sbc.w !wram_1F50
    cmp.w #$8000
    ror a
    clc
    adc.w !wram_1F50
    sta.w !obj_pos_z,x
    jsl.l ObjectGsu81E3
    jsl.l ObjectGsu80BCThen81E3
    eor.w #$FFFF
    inc a
    sep.b #$20
    ldy.w !obj_06,x
    phx
    tyx
    plx

; @routine 08:9825 CameraTrackHalfStep
; WHAT IT DOES: The track shot's frame: a cue (+$25 bit 0) sends the
; director back (CameraReturn), else the director's tail.
; HOW: AND #1; JML CameraReturn.
; Entry: M 8-bit, X 16-bit; X = the director. Exit: as the path leaves it.
%at($089825)
CameraTrackHalfStep:
    lda.w !obj_25,x
    and.b #$01
    beq .l9830
    jml.l CameraReturn
.l9830:
    jmp.w CameraDirectorTail

; @routine 08:9833 CameraReturn
; WHAT IT DOES: The director heads back to its table: frame $08:9847
; (CameraReturnStep), +$48 = $0F; then that frame.
; HOW: Word and byte stores; entered by JMLs from the shots below.
; Entry: M 8-bit, X 16-bit; X = the director. Exit: as the frame leaves it.
%at($089833)
CameraReturn:
    rep.b #$20
    lda.w #$9847
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,x
    lda.b #$0F
    sta.w !obj_48,x

; @routine 08:9847 CameraReturnStep
; WHAT IT DOES: The return's frame: view $05FA = 9; +$48 counts down with
; the director's table shot (CameraDirectorTable) each frame; at 0 the
; car's +$48 = 0 and the director goes back to its table (08:8EC1).
; HOW: DEC, JML CameraDirectorTable; JML to 08:8EC1.
; Entry: M 8-bit, X 16-bit; X = the director. Exit: as the path leaves it.
%at($089847)
CameraReturnStep:
    lda.b #$09
    sta.w !wram_05FA
    dec.w !obj_48,x
    beq .l9855
    jml.l CameraDirectorTable
.l9855:
    ldy.w !obj_06,x
    lda.b #$00
    sta.w !obj_48,y
    jml.l CameraDirectorTableStart

; @routine 08:9861 CameraPathFollow
; WHAT IT DOES: The director on path $85C3 (ObjectPathStart), frame
; $08:9888 (CameraPathFollowStep), mode 1, CameraFollowLink; then that
; frame.
; HOW: JSLs and word stores.
; Entry: M 8-bit, X 16-bit; X = the director. Exit: as the frame leaves it.
%at($089861)
CameraPathFollow:
    ldy.w #$85C3
    jsl.l ObjectPathStart
    rep.b #$20
    lda.w #$9888
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,x
    ldy.w !obj_link_w,x
    rep.b #$20
    lda.w #$0001
    sta.w !bank0_base,y
    sep.b #$20
    jsl.l CameraFollowLink

; @routine 08:9888 CameraPathFollowStep
; WHAT IT DOES: The followed path's frame: +$36 = 3, CameraApproachTarget,
; CameraFollowLink.
; HOW: Two JSLs.
; Entry: M 8-bit, X 16-bit; X = the director. Exit: as CameraFollowLink
; leaves it.
%at($089888)
CameraPathFollowStep:
    lda.b #$03
    sta.w !obj_36,x
    jsl.l CameraApproachTarget
    jsl.l CameraFollowLink
    rtl

; @routine 08:9896 CameraPath8603
; WHAT IT DOES: A path shot: the director on path $8603 (ObjectPathStart), mode 1,
; CameraFollowLink, then CameraPathGo.
; HOW: JSLs; a JMP into CameraPathGo.
; Entry: M 8-bit, X 16-bit; X = the director. Exit: as CameraPathGo leaves it.
%at($089896)
CameraPath8603:
    ldy.w #$8603
    jsl.l ObjectPathStart
    ldy.w !obj_link_w,x
    rep.b #$20
    lda.w #$0001
    sta.w !bank0_base,y
    sep.b #$20
    jsl.l CameraFollowLink
    jmp.w CameraPathGo

; @routine 08:98B1 CameraPath8643
; WHAT IT DOES: A path shot: the director on path $8643 (ObjectPathStart), mode 1,
; CameraFollowLink, then CameraPathGo.
; HOW: JSLs; a JMP into CameraPathGo.
; Entry: M 8-bit, X 16-bit; X = the director. Exit: as CameraPathGo leaves it.
%at($0898B1)
CameraPath8643:
    ldy.w #$8643
    jsl.l ObjectPathStart
    ldy.w !obj_link_w,x
    rep.b #$20
    lda.w #$0001
    sta.w !bank0_base,y
    sep.b #$20
    jsl.l CameraFollowLink
    jmp.w CameraPathGo

; @routine 08:98CC CameraPath8679
; WHAT IT DOES: A path shot: the director on path $8679 (ObjectPathStart), mode 1,
; CameraFollowLink, then CameraPathGo.
; HOW: JSLs; a JMP into CameraPathGo.
; Entry: M 8-bit, X 16-bit; X = the director. Exit: as CameraPathGo leaves it.
%at($0898CC)
CameraPath8679:
    ldy.w #$8679
    jsl.l ObjectPathStart
    ldy.w !obj_link_w,x
    rep.b #$20
    lda.w #$0001
    sta.w !bank0_base,y
    sep.b #$20
    jsl.l CameraFollowLink
    jmp.w CameraPathGo

; @routine 08:98E7 CameraPath869B
; WHAT IT DOES: A path shot: the director on path $869B (ObjectPathStart), mode 1,
; CameraFollowLink, then CameraPathGo.
; HOW: JSLs; a JMP into CameraPathGo.
; Entry: M 8-bit, X 16-bit; X = the director. Exit: as CameraPathGo leaves it.
%at($0898E7)
CameraPath869B:
    ldy.w #$869B
    jsl.l ObjectPathStart
    ldy.w !obj_link_w,x
    rep.b #$20
    lda.w #$0001
    sta.w !bank0_base,y
    sep.b #$20
    jsl.l CameraFollowLink
    jmp.w CameraPathGo

; @routine 08:9902 CameraPath86B3
; WHAT IT DOES: A path shot: the director on path $86B3 (ObjectPathStart), mode 1,
; CameraFollowLink, then CameraPathGo.
; HOW: JSLs; a JMP into CameraPathGo.
; Entry: M 8-bit, X 16-bit; X = the director. Exit: as CameraPathGo leaves it.
%at($089902)
CameraPath86B3:
    ldy.w #$86B3
    jsl.l ObjectPathStart
    ldy.w !obj_link_w,x
    rep.b #$20
    lda.w #$0001
    sta.w !bank0_base,y
    sep.b #$20
    jsl.l CameraFollowLink
    jmp.w CameraPathGo

; @routine 08:991D CameraPath86CB
; WHAT IT DOES: A path shot: the director on path $86CB (ObjectPathStart), mode 1,
; CameraFollowLink, then CameraPathGo.
; HOW: JSLs; a JMP into CameraPathGo.
; Entry: M 8-bit, X 16-bit; X = the director. Exit: as CameraPathGo leaves it.
%at($08991D)
CameraPath86CB:
    ldy.w #$86CB
    jsl.l ObjectPathStart
    ldy.w !obj_link_w,x
    rep.b #$20
    lda.w #$0001
    sta.w !bank0_base,y
    sep.b #$20
    jsl.l CameraFollowLink
    jmp.w CameraPathGo

; @routine 08:9938 CameraPath8709
; WHAT IT DOES: A path shot: the director on path $8709 (ObjectPathStart),
; CameraFollowLink, then CameraPathGo.
; HOW: JSLs; a JMP into CameraPathGo.
; Entry: M 8-bit, X 16-bit; X = the director. Exit: as CameraPathGo leaves it.
%at($089938)
CameraPath8709:
    ldy.w #$8709
    jsl.l ObjectPathStart
    jsl.l CameraFollowLink
    jmp.w CameraPathGo

; @routine 08:9946 CameraPath86E3
; WHAT IT DOES: A path shot: the director on path $86E3 (ObjectPathStart), mode 1,
; CameraFollowLink, then CameraPathGo.
; HOW: JSLs; a JMP into CameraPathGo.
; Entry: M 8-bit, X 16-bit; X = the director. Exit: as CameraPathGo leaves it.
%at($089946)
CameraPath86E3:
    ldy.w #$86E3
    jsl.l ObjectPathStart
    ldy.w !obj_link_w,x
    rep.b #$20
    lda.w #$0001
    sta.w !bank0_base,y
    sep.b #$20
    jsl.l CameraFollowLink
    jmp.w CameraPathGo

; @routine 08:9961 CameraPath86FB
; WHAT IT DOES: A path shot: the director on path $86FB (ObjectPathStart), mode 1,
; CameraFollowLink, then CameraPathGo.
; HOW: JSLs; a JMP into CameraPathGo.
; Entry: M 8-bit, X 16-bit; X = the director. Exit: as CameraPathGo leaves it.
%at($089961)
CameraPath86FB:
    ldy.w #$86FB
    jsl.l ObjectPathStart
    ldy.w !obj_link_w,x
    rep.b #$20
    lda.w #$0001
    sta.w !bank0_base,y
    sep.b #$20
    jsl.l CameraFollowLink
    jmp.w CameraPathGo

; @routine 08:997C CameraPathGo
; WHAT IT DOES: The path shots' shared end: frame $08:998B
; (CameraPathGoStep); then that frame.
; HOW: Word and byte stores; entered by JMPs.
; Entry: M 8-bit, X 16-bit; X = the director. Exit: as the frame leaves it.
%at($08997C)
CameraPathGo:
    rep.b #$20
    lda.w #$998B
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,x

; @routine 08:998B CameraPathGoStep
; WHAT IT DOES: A path shot's frame: CameraApproachTarget, then the
; director's tail.
; HOW: JSL, JMP to 08:8EF9.
; Entry: M 8-bit, X 16-bit; X = the director. Exit: as the tail leaves it.
%at($08998B)
CameraPathGoStep:
    jsl.l CameraApproachTarget
    jmp.w CameraDirectorTail

; @routine 08:9992 CameraLowBehind
; WHAT IT DOES: The low shot from behind: mode $C080, frame $08:9A25
; (CameraLowHoldStep), the camera's offset (0, -$64, 0), CameraFollowLink;
; then that frame.
; HOW: Word stores; a JMP into the frame.
; Entry: M 8-bit, X 16-bit; X = the director. Exit: as the frame leaves it.
%at($089992)
CameraLowBehind:
    ldy.w !obj_link_w,x
    rep.b #$20
    lda.w #$C080
    sta.w !bank0_base,y
    sep.b #$20
    rep.b #$20
    lda.w #$9A25
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,x
    ldy.w !obj_link_w,x
    rep.b #$20
    lda.w #$0000
    sta.w !actor_kind,y
    sep.b #$20
    ldy.w !obj_link_w,x
    rep.b #$20
    lda.w #$FF9C
    sta.w !actor_1c,y
    sep.b #$20
    ldy.w !obj_link_w,x
    rep.b #$20
    lda.w #$0000
    sta.w !actor_1e,y
    sep.b #$20
    jsl.l CameraFollowLink
    jmp.w CameraLowHoldStep

; @routine 08:99DC CameraPathLow87B7
; WHAT IT DOES: A low path shot on path $87B7: ObjectPathStart, mode 1,
; CameraFollowLink, frame $08:9A0D (CameraPathLowStep); then that frame.
; HOW: JSLs; a JMP into the shared part at 08:99ED.
; Entry: M 8-bit, X 16-bit; X = the director. Exit: as the frame leaves it.
%at($0899DC)
CameraPathLow87B7:
    ldy.w #$87B7
    jsl.l ObjectPathStart
    jmp.w CameraPathLow875B_l99ED

; @routine 08:99E6 CameraPathLow875B
; WHAT IT DOES: As CameraPathLow87B7 on path $875B.
; HOW: As CameraPathLow87B7.
; Entry: as CameraPathLow87B7. Exit: as CameraPathLow87B7.
%at($0899E6)
CameraPathLow875B:
    ldy.w #$875B
    jsl.l ObjectPathStart
.l99ED:
    ldy.w !obj_link_w,x
    rep.b #$20
    lda.w #$0001
    sta.w !bank0_base,y
    sep.b #$20
    jsl.l CameraFollowLink
    rep.b #$20
    lda.w #$9A0D
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,x

; @routine 08:9A0D CameraPathLowStep
; WHAT IT DOES: The low path shot's frame: CameraApproachTarget; once there
; the low hold (CameraLowHold), else the tail.
; HOW: Carry from CameraApproachTarget; BRL to 08:8EF9.
; Entry: M 8-bit, X 16-bit; X = the director. Exit: as the path leaves it.
%at($089A0D)
CameraPathLowStep:
    jsl.l CameraApproachTarget
    bcs CameraLowHold
    brl CameraDirectorTail

; @routine 08:9A16 CameraLowHold
; WHAT IT DOES: The low shot held: frame $08:9A25 (CameraLowHoldStep);
; then that frame.
; HOW: Word and byte stores.
; Entry: M 8-bit, X 16-bit; X = the director. Exit: as the frame leaves it.
%at($089A16)
CameraLowHold:
    rep.b #$20
    lda.w #$9A25
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,x

; @routine 08:9A25 CameraLowHoldStep
; WHAT IT DOES: The low shot's frame: the camera's offset +$1A-$1E = (0,
; -$64, 0), then the director's tail.
; HOW: Word stores; JMP to 08:8EF9.
; Entry: M 8-bit, X 16-bit; X = the director. Exit: as the tail leaves it.
%at($089A25)
CameraLowHoldStep:
    ldy.w !obj_link_w,x
    rep.b #$20
    lda.w #$0000
    sta.w !actor_kind,y
    sep.b #$20
    ldy.w !obj_link_w,x
    rep.b #$20
    lda.w #$FF9C
    sta.w !actor_1c,y
    sep.b #$20
    ldy.w !obj_link_w,x
    rep.b #$20
    lda.w #$0000
    sta.w !actor_1e,y
    sep.b #$20
    jmp.w CameraDirectorTail
