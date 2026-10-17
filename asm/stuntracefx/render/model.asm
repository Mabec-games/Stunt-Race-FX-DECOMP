; model.asm: the Super FX's 3D model interpreter (01:8561), its command
; code (the IWT R15 table at 01:8000 picks it, through the bank's mirror
; at 01:00xx), the material lookups and the textured screen clip it uses.
; C: src/stuntracefx/render/model.c, which describes the commands.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"

arch superfx

;
; @routine 01:8561 FxModelDraw
; WHAT IT DOES: Draws a 3D model: sets up the object's rotation, light
; and point scale, then runs the model's command list from ROM ($0016,
; bank $001C) until its end command, each command transforming points,
; projecting them, or queueing and drawing faces.
; HOW: Pushes R11 on the R10 stack; a non-zero $01B4 first (01:8568)
; and $01B6 pick other set-ups. $005C = $08AA (the draw list empty).
; The object's angles ($0020-$0024) become negated 8.8 words; with all
; three zero the matrix at $00E4 is copied to $0132 (or negated in
; parts when the y angle is $40 or $80), else FxMatrixFromAngles01,
; FxMatrixTranspose and FxMatrixApplyVectors build it. The light $0000,
; $393E, $727C is turned into $0106-$010A. Points start at $0340
; ($001E); the end command (9243) is pushed on the stack, and the first
; command byte is dispatched through the IWT R15 table at 01:8000, as
; every command's last GETB is.
; Entry: R10 = stack; R11 = return address; the object's set-up in RAM.
; Exit: as the code leaves the registers, flags and RAM; R14 and the
; ROM buffer move as on the Super FX.
%at($018561)
FxModelDraw:
    lms r0,(!fx_model_1b4-!gsu_ram)
    lob
    beq FxModelDrawFlagged_l8584
    nop
    from r11
    stw (r10)
    inc r10
    inc r10
    link #4
    iwt r15,#FxModelDrawFlagged
    nop
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop
;
; @routine 01:8576 FxModelDrawFlagged
; WHAT IT DOES: FxModelDraw's set-up when $01B4 is set (01:8561 calls
; it with R11 = $8571): the model flags $0054 become bit 12 of them
; with bit 13 set, then the model is drawn as FxModelDraw draws it.
; HOW: R2 = $1000, R1 = $2000: ($0054 AND R2) OR R1 into $0054
; (01:858F), the rest as in FxModelDraw; its end command's pull returns
; to R11.
; Entry: R10 = stack; R11 = return address; the object's set-up in RAM.
; Exit: as the code leaves the registers, flags and RAM; R14 and the
; ROM buffer move as on the Super FX.
%at($018576)
FxModelDrawFlagged:
    iwt r2,#$1000
    iwt r1,#$2000
    lms r0,(!fx_model_flags-!gsu_ram)
    and r2
    or r1
    bra .l858F
    nop
.l8584:
    from r11
    stw (r10)
    inc r10
    inc r10
    dec r10
    dec r10
    to r11
    ldw (r10)
    iwt r0,#$8000
.l858F:
    sms (!fx_model_flags-!gsu_ram),r0
    from r11
    stw (r10)
    inc r10
    inc r10
    ibt r0,#$03
    romb
    iwt r0,#$08AA
    sms (!fx_list_end-!gsu_ram),r0
    ibt r0,#$03
    romb
    lms r14,(!fx_model_list-!gsu_ram)
    lms r0,(!fx_model_1b6-!gsu_ram)
    lob
    beq .l85B6
    nop
    dec r0
    add r0
    add r0
    iwt r2,#$AE45
    to r14
    add r2
.l85B6:
    lm r0,(!fx_car_108c-!gsu_ram)
    lob
    beq .l8609
    sub r0
    lms r1,(!fx_object_z-!gsu_ram)
    getbh
    inc r14
    add r1
    bpl .l85D2
    nop
    iwt r0,#$A275
    iwt r2,#$0000
    iwt r15,#.l85FC
    nop
.l85D2:
    sub r0
    getbh
    inc r14
    add r1
    bpl .l85E4
    nop
    iwt r0,#$A2A1
    iwt r2,#$0088
    iwt r15,#.l85FC
    nop
.l85E4:
    sub r0
    getbh
    inc r14
    add r1
    bpl .l85F6
    nop
    iwt r0,#$A2CD
    iwt r2,#$0110
    iwt r15,#.l85FC
    nop
.l85F6:
    iwt r0,#$A2F9
    iwt r2,#$0198
.l85FC:
    sms (!fx_shade_table-!gsu_ram),r0
    lms r0,(!fx_shade_base-!gsu_ram)
    add r2
    sms (!fx_shade_at-!gsu_ram),r0
    bra .l8651
    nop
.l8609:
    lms r1,(!fx_object_z-!gsu_ram)
    getbh
    inc r14
    add r1
    bpl .l861D
    nop
    iwt r0,#$A1C5
    iwt r2,#$0000
    iwt r15,#.l8647
    nop
.l861D:
    sub r0
    getbh
    inc r14
    add r1
    bpl .l862F
    nop
    iwt r0,#$A1F1
    iwt r2,#$0088
    iwt r15,#.l8647
    nop
.l862F:
    sub r0
    getbh
    inc r14
    add r1
    bpl .l8641
    nop
    iwt r0,#$A21D
    iwt r2,#$0110
    iwt r15,#.l8647
    nop
.l8641:
    iwt r0,#$A249
    iwt r2,#$0198
.l8647:
    sms (!fx_shade_table-!gsu_ram),r0
    lms r0,(!fx_shade_base-!gsu_ram)
    add r2
    sms (!fx_shade_at-!gsu_ram),r0
.l8651:
    cache
    ibt r0,#$01
    lms r12,(!fx_point_shift-!gsu_ram)
    moves r12,r12
    beq .l8660
    nop
    move r13,r15
    loop
    add r0
.l8660:
    sms (!fx_point_scale-!gsu_ram),r0
    lms r1,(!fx_object_angle_x-!gsu_ram)
    from r1
    lob
    swap
    not
    inc r0
    sbk
    lms r2,(!fx_object_angle_y-!gsu_ram)
    from r2
    lob
    swap
    not
    inc r0
    sbk
    lms r3,(!fx_object_angle_z-!gsu_ram)
    from r3
    lob
    swap
    not
    inc r0
    sbk
    from r1
    or r3
    beq .l8687
    nop
    iwt r15,#.l8741
    nop
.l8687:
    or r2
    bne .l86D8
    nop
    lms r0,(!fx_object_matrix_0-!gsu_ram)
    sms (!fx_view_matrix_0-!gsu_ram),r0
    lms r0,(!fx_object_matrix_1-!gsu_ram)
    sms (!fx_view_matrix_1-!gsu_ram),r0
    lms r0,(!fx_object_matrix_2-!gsu_ram)
    sms (!fx_view_matrix_2-!gsu_ram),r0
    lms r0,(!fx_object_matrix_3-!gsu_ram)
    sms (!fx_view_matrix_3-!gsu_ram),r0
    lms r0,(!fx_object_matrix_4-!gsu_ram)
    sms (!fx_view_matrix_4-!gsu_ram),r0
    lms r0,(!fx_object_matrix_5-!gsu_ram)
    sms (!fx_view_matrix_5-!gsu_ram),r0
    lms r0,(!fx_object_matrix_6-!gsu_ram)
    sms (!fx_view_matrix_6-!gsu_ram),r0
    lms r0,(!fx_object_matrix_7-!gsu_ram)
    sms (!fx_view_matrix_7-!gsu_ram),r0
    lms r0,(!fx_object_matrix_8-!gsu_ram)
    sms (!fx_view_matrix_8-!gsu_ram),r0
    lms r0,(!fx_model_flags-!gsu_ram)
    iwt r1,#$2000
    and r1
    beq .l86D4
    sub r0
    sms (!fx_view_matrix_3-!gsu_ram),r0
    sms (!fx_view_matrix_4-!gsu_ram),r0
    sms (!fx_view_matrix_5-!gsu_ram),r0
.l86D4:
    iwt r15,#.l8778
    nop
.l86D8:
    iwt r4,#$0040
    from r2
    sub r4
    bne .l86E4
    nop
    iwt r15,#.l8741
    nop
.l86E4:
    sub r4
    bne .l8741
    nop
    lms r0,(!fx_object_matrix_0-!gsu_ram)
    not
    inc r0
    sms (!fx_view_matrix_0-!gsu_ram),r0
    lms r0,(!fx_object_matrix_1-!gsu_ram)
    not
    inc r0
    sms (!fx_view_matrix_1-!gsu_ram),r0
    lms r0,(!fx_object_matrix_2-!gsu_ram)
    not
    inc r0
    sms (!fx_view_matrix_2-!gsu_ram),r0
    lms r0,(!fx_object_matrix_3-!gsu_ram)
    sms (!fx_view_matrix_3-!gsu_ram),r0
    lms r0,(!fx_object_matrix_4-!gsu_ram)
    sms (!fx_view_matrix_4-!gsu_ram),r0
    lms r0,(!fx_object_matrix_5-!gsu_ram)
    sms (!fx_view_matrix_5-!gsu_ram),r0
    lms r0,(!fx_object_matrix_6-!gsu_ram)
    not
    inc r0
    sms (!fx_view_matrix_6-!gsu_ram),r0
    lms r0,(!fx_object_matrix_7-!gsu_ram)
    not
    inc r0
    sms (!fx_view_matrix_7-!gsu_ram),r0
    lms r0,(!fx_object_matrix_8-!gsu_ram)
    not
    inc r0
    sms (!fx_view_matrix_8-!gsu_ram),r0
    lms r0,(!fx_model_flags-!gsu_ram)
    iwt r1,#$2000
    and r1
    beq .l873D
    sub r0
    sms (!fx_view_matrix_3-!gsu_ram),r0
    sms (!fx_view_matrix_4-!gsu_ram),r0
    sms (!fx_view_matrix_5-!gsu_ram),r0
.l873D:
    iwt r15,#.l8778
    nop
.l8741:
    iwt r9,#$0116
    link #4
    iwt r15,#FxMatrixFromAngles01
    nop
    iwt r9,#$0116
    link #4
    iwt r15,#FxMatrixTranspose
    nop
    lms r0,(!fx_model_flags-!gsu_ram)
    iwt r1,#$2000
    and r1
    beq .l8770
    sub r0
    sms (!fx_model_118-!gsu_ram),r0
    sms (!fx_model_11e-!gsu_ram),r0
    sms (!fx_model_124-!gsu_ram),r0
    iwt r9,#$0132
    link #4
    iwt r15,#FxMatrixApplyVectors
    nop
    iwt r15,#.l8778
    nop
.l8770:
    iwt r9,#$0132
    link #4
    iwt r15,#FxMatrixApplyVectors
    nop
.l8778:
    iwt r0,#$0000
    sms (!fx_light_x-!gsu_ram),r0
    iwt r0,#$393E
    sms (!fx_light_y-!gsu_ram),r0
    iwt r0,#$727C
    sms (!fx_light_z-!gsu_ram),r0
    lms r1,(!fx_light_x-!gsu_ram)
    lms r2,(!fx_light_y-!gsu_ram)
    lms r3,(!fx_light_z-!gsu_ram)
    lms r6,(!fx_view_matrix_0-!gsu_ram)
    from r1
    fmult
    to r5
    rol
    lms r6,(!fx_view_matrix_1-!gsu_ram)
    from r2
    fmult
    rol
    with r5
    add r0
    lms r6,(!fx_view_matrix_2-!gsu_ram)
    from r3
    fmult
    rol
    add r5
    hib
    sex
    sms (!fx_light_local_x-!gsu_ram),r0
    lms r6,(!fx_view_matrix_3-!gsu_ram)
    from r1
    fmult
    to r5
    rol
    lms r6,(!fx_view_matrix_4-!gsu_ram)
    from r2
    fmult
    rol
    with r5
    add r0
    lms r6,(!fx_view_matrix_5-!gsu_ram)
    from r3
    fmult
    rol
    add r5
    hib
    sex
    sms (!fx_light_local_y-!gsu_ram),r0
    lms r6,(!fx_view_matrix_6-!gsu_ram)
    from r1
    fmult
    to r5
    rol
    lms r6,(!fx_view_matrix_7-!gsu_ram)
    from r2
    fmult
    rol
    with r5
    add r0
    lms r6,(!fx_view_matrix_8-!gsu_ram)
    from r3
    fmult
    rol
    add r5
    hib
    sex
    sms (!fx_light_local_z-!gsu_ram),r0
    lms r0,(!fx_point_shift-!gsu_ram)
    sub #3
    bpl .l882C
    nop
    iwt r2,#$0133
    to r1
    ldb (r2)
    inc r2
    inc r2
    ldb (r2)
    swap
    or r1
    sms (!fx_matrix_packed_0-!gsu_ram),r0
    inc r2
    inc r2
    to r1
    ldb (r2)
    inc r2
    inc r2
    ldb (r2)
    swap
    or r1
    sms (!fx_matrix_packed_1-!gsu_ram),r0
    inc r2
    inc r2
    to r1
    ldb (r2)
    inc r2
    inc r2
    ldb (r2)
    swap
    or r1
    sms (!fx_matrix_packed_2-!gsu_ram),r0
    inc r2
    inc r2
    to r1
    ldb (r2)
    inc r2
    inc r2
    ldb (r2)
    swap
    or r1
    sms (!fx_matrix_packed_3-!gsu_ram),r0
    inc r2
    inc r2
    ldb (r2)
    sms (!fx_matrix_packed_4-!gsu_ram),r0
.l882C:
    sub r0
    sms (!fx_points_total-!gsu_ram),r0
    iwt r0,#$0340
    sms (!fx_point_out-!gsu_ram),r0
    lms r0,(!fx_rom_bank-!gsu_ram)
    romb
    lms r14,(!fx_model_at-!gsu_ram)
    sms (!fx_rom_r14-!gsu_ram),r14
    iwt r0,#$9243
    stw (r10)
    inc r10
    inc r10
    to r15
    getb
    inc r14
;
; @routine 01:884A FxModelReturn
; WHAT IT DOES: Commands $18, $24, $2C and $48: return, pulling the
; next command address from the R10 stack.
; HOW: DEC R10 twice, R15 from (R10).
; Entry: R10 = stack; R11 = return address; R14 at the command's bytes
; (bank $001C).
; Exit: as the code leaves the registers, flags and RAM; R14 and the
; ROM buffer move as on the Super FX.
%at($01884A)
FxModelReturn:
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop
;
; @routine 01:884F FxModelFramePick
; WHAT IT DOES: Command $1C: of the n lists that follow (n the next
; byte), runs the one the model frame $0044 picks ($0044 AND $3F, less
; n until below it).
; HOW: R14 skips to the picked list's word offset and the list's first
; command is dispatched.
; Entry: R10 = stack; R11 = return address; R14 at the command's bytes
; (bank $001C).
; Exit: as the code leaves the registers, flags and RAM; R14 and the
; ROM buffer move as on the Super FX.
%at($01884F)
FxModelFramePick:
    to r1
    getb
    inc r14
    lms r0,(!fx_model_frame-!gsu_ram)
    ibt r2,#$3F
    and r2
.l8858:
    cmp r1
    bmi .l8860
    nop
    bra .l8858
    sub r1
.l8860:
    add r0
    with r14
    add r0
;
; @routine 01:8863 FxModelJump
; WHAT IT DOES: Command $20: jumps within the commands by the signed
; word that follows.
; HOW: GETB, GETBH, R14 += it, dispatch.
; Entry: R10 = stack; R11 = return address; R14 at the command's bytes
; (bank $001C).
; Exit: as the code leaves the registers, flags and RAM; R14 and the
; ROM buffer move as on the Super FX.
%at($018863)
FxModelJump:
    getb
    inc r14
    getbh
    with r14
    add r0
    to r15
    getb
    inc r14
;
; @routine 01:886C FxModelPointsFull
; WHAT IT DOES: Commands $08 and $4C: n points (the next byte) turned
; by the view matrix ($0132-$0142, held in R10, R11, R13, R4, R7, R8
; and two RAM words) into x, y, z words at $001E (not reached in the
; traces so far).
; HOW: R10 kept at $0012 while it holds a matrix element; a count of
; DEC R12; $0144 counts the points.
; Entry: R10 = stack; R11 = return address; R14 at the command's bytes
; (bank $001C).
; Exit: as the code leaves the registers, flags and RAM; R14 and the
; ROM buffer move as on the Super FX.
%at($01886C)
FxModelPointsFull:
    to r12
    getb
    inc r14
    lms r0,(!fx_points_total-!gsu_ram)
    add r12
    sbk
    lms r9,(!fx_point_out-!gsu_ram)
    sms (!fx_model_stack-!gsu_ram),r10
    lms r10,(!fx_view_matrix_0-!gsu_ram)
    lms r11,(!fx_view_matrix_3-!gsu_ram)
    lms r13,(!fx_view_matrix_6-!gsu_ram)
    lms r4,(!fx_view_matrix_1-!gsu_ram)
    lms r7,(!fx_view_matrix_4-!gsu_ram)
    lms r8,(!fx_view_matrix_7-!gsu_ram)
    cache
.l888D:
    to r1
    getb
    inc r14
    with r1
    getbh
    inc r14
    to r2
    getb
    inc r14
    with r2
    getbh
    inc r14
    to r3
    getb
    inc r14
    with r3
    getbh
    inc r14
    move r6,r10
    from r1
    fmult
    to r5
    rol
    move r6,r11
    from r2
    fmult
    rol
    with r5
    add r0
    move r6,r13
    from r3
    fmult
    rol
    add r5
    stw (r9)
    inc r9
    inc r9
    move r6,r4
    from r1
    fmult
    to r5
    rol
    move r6,r7
    from r2
    fmult
    rol
    with r5
    add r0
    move r6,r8
    from r3
    fmult
    rol
    add r5
    stw (r9)
    inc r9
    inc r9
    lms r6,(!fx_view_matrix_2-!gsu_ram)
    from r1
    fmult
    to r5
    rol
    lms r6,(!fx_view_matrix_5-!gsu_ram)
    from r2
    fmult
    rol
    with r5
    add r0
    lms r6,(!fx_view_matrix_8-!gsu_ram)
    from r3
    fmult
    rol
    add r5
    stw (r9)
    inc r9
    dec r12
    bne .l888D
    inc r9
    sms (!fx_point_out-!gsu_ram),r9
    lms r10,(!fx_model_stack-!gsu_ram)
    to r15
    getb
    inc r14
;
; @routine 01:88F3 FxModelPointsScaled
; WHAT IT DOES: Command $34: n points of three words (GETB, GETBH),
; each turned by the view matrix (FMULT, ROL) and written twice at
; $001E: as given and with x negated (the x term subtracted twice);
; $0144 counts 2n (not reached in the traces so far).
; HOW: As FxModelPointsFull, with FMULT and ROL by the matrix
; registers.
; Entry: R10 = stack; R11 = return address; R14 at the command's bytes
; (bank $001C).
; Exit: as the code leaves the registers, flags and RAM; R14 and the
; ROM buffer move as on the Super FX.
%at($0188F3)
FxModelPointsScaled:
    to r12
    getb
    inc r14
    lms r0,(!fx_points_total-!gsu_ram)
    add r12
    add r12
    sbk
    lms r9,(!fx_point_out-!gsu_ram)
    cache
    move r13,r15
    to r1
    getb
    inc r14
    with r1
    getbh
    inc r14
    to r2
    getb
    inc r14
    with r2
    getbh
    inc r14
    to r3
    getb
    inc r14
    with r3
    getbh
    inc r14
    lms r6,(!fx_view_matrix_0-!gsu_ram)
    from r1
    fmult
    to r8
    rol
    lms r6,(!fx_view_matrix_3-!gsu_ram)
    from r2
    fmult
    to r5
    rol
    lms r6,(!fx_view_matrix_6-!gsu_ram)
    from r3
    fmult
    rol
    add r5
    add r8
    stw (r9)
    sub r8
    sub r8
    with r9
    add #6
    stw (r9)
    with r9
    sub #4
    lms r6,(!fx_view_matrix_1-!gsu_ram)
    from r1
    fmult
    to r8
    rol
    lms r6,(!fx_view_matrix_4-!gsu_ram)
    from r2
    fmult
    to r5
    rol
    lms r6,(!fx_view_matrix_7-!gsu_ram)
    from r3
    fmult
    rol
    add r5
    add r8
    stw (r9)
    sub r8
    sub r8
    with r9
    add #6
    stw (r9)
    with r9
    sub #4
    lms r6,(!fx_view_matrix_2-!gsu_ram)
    from r1
    fmult
    to r8
    rol
    lms r6,(!fx_view_matrix_5-!gsu_ram)
    from r2
    fmult
    to r5
    rol
    lms r6,(!fx_view_matrix_8-!gsu_ram)
    from r3
    fmult
    rol
    add r5
    add r8
    stw (r9)
    sub r8
    sub r8
    with r9
    add #6
    stw (r9)
    with r9
    sub #4
    with r9
    add #6
    loop
    nop
    sms (!fx_point_out-!gsu_ram),r9
    to r15
    getb
    inc r14
.l8982:
    lms r9,(!fx_point_out-!gsu_ram)
    sms (!fx_model_stack-!gsu_ram),r10
    lms r11,(!fx_view_matrix_3-!gsu_ram)
    lms r13,(!fx_view_matrix_6-!gsu_ram)
    lms r4,(!fx_view_matrix_1-!gsu_ram)
    lms r7,(!fx_view_matrix_4-!gsu_ram)
    lms r8,(!fx_view_matrix_7-!gsu_ram)
    lms r10,(!fx_view_matrix_2-!gsu_ram)
    to r12
    getb
    inc r14
    lms r0,(!fx_points_total-!gsu_ram)
    add r12
    sbk
    cache
.l89A3:
    getb
    inc r14
    lms r5,(!fx_point_scale-!gsu_ram)
    to r1
    mult r5
    getb
    inc r14
    lms r6,(!fx_view_matrix_0-!gsu_ram)
    to r2
    mult r5
    getb
    inc r14
    to r3
    mult r5
    from r1
    fmult
    to r5
    rol
    move r6,r11
    from r2
    fmult
    rol
    with r5
    add r0
    move r6,r13
    from r3
    fmult
    rol
    add r5
    stw (r9)
    inc r9
    inc r9
    move r6,r4
    from r1
    fmult
    to r5
    rol
    move r6,r7
    from r2
    fmult
    rol
    with r5
    add r0
    move r6,r8
    from r3
    fmult
    rol
    add r5
    stw (r9)
    inc r9
    inc r9
    move r6,r10
    from r1
    fmult
    to r5
    rol
    lms r6,(!fx_view_matrix_5-!gsu_ram)
    from r2
    fmult
    rol
    with r5
    add r0
    lms r6,(!fx_view_matrix_8-!gsu_ram)
    from r3
    fmult
    rol
    add r5
    stw (r9)
    inc r9
    dec r12
    bne .l89A3
    inc r9
    lms r10,(!fx_model_stack-!gsu_ram)
    sms (!fx_point_out-!gsu_ram),r9
    to r15
    getb
    inc r14
.l8A03:
    lms r9,(!fx_point_out-!gsu_ram)
    sms (!fx_model_stack-!gsu_ram),r10
    lms r11,(!fx_view_matrix_3-!gsu_ram)
    lms r13,(!fx_view_matrix_6-!gsu_ram)
    lms r4,(!fx_view_matrix_1-!gsu_ram)
    lms r7,(!fx_view_matrix_4-!gsu_ram)
    lms r10,(!fx_view_matrix_2-!gsu_ram)
    to r12
    getb
    inc r14
    lms r0,(!fx_points_total-!gsu_ram)
    add r12
    add r12
    sbk
    cache
.l8A22:
    getb
    inc r14
    lms r5,(!fx_point_scale-!gsu_ram)
    to r1
    mult r5
    getb
    inc r14
    lms r6,(!fx_view_matrix_0-!gsu_ram)
    to r2
    mult r5
    getb
    inc r14
    to r3
    mult r5
    from r1
    fmult
    to r8
    rol
    move r6,r11
    from r2
    fmult
    to r5
    rol
    move r6,r13
    from r3
    fmult
    rol
    add r5
    add r8
    stw (r9)
    sub r8
    sub r8
    with r9
    add #6
    stw (r9)
    with r9
    sub #4
    move r6,r4
    from r1
    fmult
    to r8
    rol
    move r6,r7
    from r2
    fmult
    to r5
    rol
    lms r6,(!fx_view_matrix_7-!gsu_ram)
    from r3
    fmult
    rol
    add r5
    add r8
    stw (r9)
    sub r8
    sub r8
    with r9
    add #6
    stw (r9)
    with r9
    sub #4
    move r6,r10
    from r1
    fmult
    to r8
    rol
    lms r6,(!fx_view_matrix_5-!gsu_ram)
    from r2
    fmult
    to r5
    rol
    lms r6,(!fx_view_matrix_8-!gsu_ram)
    from r3
    fmult
    rol
    add r5
    add r8
    stw (r9)
    sub r8
    sub r8
    with r9
    add #6
    stw (r9)
    inc r9
    dec r12
    bne .l8A22
    inc r9
    lms r10,(!fx_model_stack-!gsu_ram)
    sms (!fx_point_out-!gsu_ram),r9
    to r15
    getb
    inc r14
;
; @routine 01:8A97 FxModelPoints01
; WHAT IT DOES: Command $04: n points of three signed bytes each,
; scaled by $0030 and turned by the packed matrix ($0128-$0130),
; written as x, y, z words at $001E.
; HOW: A LOOP of R12 = n; MULT of each byte by the packed matrix bytes;
; $0144 counts the points; models of fewer than 3 points per $0032 take
; 01:8982.
; Entry: R10 = stack; R11 = return address; R14 at the command's bytes
; (bank $001C).
; Exit: as the code leaves the registers, flags and RAM; R14 and the
; ROM buffer move as on the Super FX.
%at($018A97)
FxModelPoints01:
    lms r0,(!fx_point_shift-!gsu_ram)
    sub #3
    bmi .l8AA3
    nop
    iwt r15,#FxModelPointsScaled_l8982
    nop
.l8AA3:
    nop
    lms r11,(!fx_point_out-!gsu_ram)
    lms r5,(!fx_matrix_packed_0-!gsu_ram)
    lms r6,(!fx_matrix_packed_1-!gsu_ram)
    lms r7,(!fx_matrix_packed_2-!gsu_ram)
    lms r8,(!fx_matrix_packed_3-!gsu_ram)
    lms r9,(!fx_matrix_packed_4-!gsu_ram)
    lms r1,(!fx_point_scale-!gsu_ram)
    from r9
    lob
    swap
    to r9
    or r1
    to r12
    getb
    inc r14
    lms r0,(!fx_points_total-!gsu_ram)
    add r12
    sbk
    cache
    move r13,r15
    to r1
    getb
    inc r14
    from r5
    to r4
    mult r1
    to r2
    getb
    inc r14
    from r6
    hib
    mult r2
    to r4
    add r4
    to r3
    getb
    inc r14
    from r8
    mult r3
    add r4
    add r0
    hib
    mult r9
    stw (r11)
    inc r11
    inc r11
    from r5
    hib
    to r4
    mult r1
    from r7
    mult r2
    to r4
    add r4
    from r8
    hib
    mult r3
    add r4
    add r0
    hib
    mult r9
    stw (r11)
    inc r11
    inc r11
    from r6
    to r4
    mult r1
    from r7
    hib
    mult r2
    to r4
    add r4
    from r9
    hib
    mult r3
    add r4
    add r0
    hib
    mult r9
    stw (r11)
    inc r11
    loop
    inc r11
    sms (!fx_point_out-!gsu_ram),r11
    to r15
    getb
    inc r14
;
; @routine 01:8B0E FxModelPointsMirrored
; WHAT IT DOES: Command $38: as command $04, each point also written
; mirrored in x (two points per entry).
; HOW: The x term added and subtracted (ADD R8, SUB R8 twice) for the
; pair.
; Entry: R10 = stack; R11 = return address; R14 at the command's bytes
; (bank $001C).
; Exit: as the code leaves the registers, flags and RAM; R14 and the
; ROM buffer move as on the Super FX.
%at($018B0E)
FxModelPointsMirrored:
    lms r0,(!fx_point_shift-!gsu_ram)
    sub #3
    bmi .l8B1A
    nop
    iwt r15,#FxModelPointsScaled_l8A03
    nop
.l8B1A:
    nop
    lms r11,(!fx_point_out-!gsu_ram)
    lms r5,(!fx_matrix_packed_0-!gsu_ram)
    lms r6,(!fx_matrix_packed_1-!gsu_ram)
    lms r7,(!fx_matrix_packed_2-!gsu_ram)
    lms r8,(!fx_matrix_packed_3-!gsu_ram)
    lms r9,(!fx_matrix_packed_4-!gsu_ram)
    lms r1,(!fx_point_scale-!gsu_ram)
    from r9
    lob
    swap
    to r9
    or r1
    to r12
    getb
    inc r14
    lms r0,(!fx_points_total-!gsu_ram)
    add r12
    add r12
    sbk
    cache
.l8B3F:
    to r1
    getb
    inc r14
    from r5
    to r13
    mult r1
    to r2
    getb
    inc r14
    from r6
    hib
    to r4
    mult r2
    to r3
    getb
    inc r14
    from r8
    mult r3
    to r4
    add r4
    from r4
    add r13
    add r0
    hib
    mult r9
    stw (r11)
    with r11
    add #6
    from r4
    sub r13
    add r0
    hib
    mult r9
    stw (r11)
    with r11
    sub #4
    from r5
    hib
    to r13
    mult r1
    from r7
    to r4
    mult r2
    from r8
    hib
    mult r3
    to r4
    add r4
    from r4
    add r13
    add r0
    hib
    mult r9
    stw (r11)
    with r11
    add #6
    from r4
    sub r13
    add r0
    hib
    mult r9
    stw (r11)
    with r11
    sub #4
    from r6
    to r13
    mult r1
    from r7
    hib
    to r4
    mult r2
    from r9
    hib
    mult r3
    to r4
    add r4
    from r4
    add r13
    add r0
    hib
    mult r9
    stw (r11)
    with r11
    add #6
    from r4
    sub r13
    add r0
    hib
    mult r9
    stw (r11)
    inc r11
    inc r11
    dec r12
    bne .l8B3F
    nop
    sms (!fx_point_out-!gsu_ram),r11
    to r15
    getb
    inc r14
;
; @routine 01:8BAA FxModelProject
; WHAT IT DOES: Command $0C: projects the points written so far
; (FxProjectVertex, or FxProjectPoint for near ones) for the faces that
; follow.
; HOW: Its rest runs at 01:8C79 (FxModelProjectRest).
; Entry: R10 = stack; R11 = return address; R14 at the command's bytes
; (bank $001C).
; Exit: as the code leaves the registers, flags and RAM; R14 and the
; ROM buffer move as on the Super FX.
%at($018BAA)
FxModelProject:
    iwt r15,#FxModelProjectRest_l8C89
    nop
    lms r12,(!fx_points_total-!gsu_ram)
    ibt r0,#$14
    romb
    iwt r7,#$0340
    sms (!fx_rom_r14-!gsu_ram),r14
    iwt r8,#$04E4
    sub r0
.l8BBF:
    sms (!fx_outcodes-!gsu_ram),r0
    lms r9,(!fx_object_z-!gsu_ram)
    lms r3,(!fx_object_x-!gsu_ram)
    lms r13,(!fx_object_y-!gsu_ram)
    cache
.l8BCC:
    ldw (r7)
    inc r7
    inc r7
    to r1
    add r3
    ldw (r7)
    inc r7
    inc r7
    to r2
    add r13
    ldw (r7)
    inc r7
    inc r7
    to r6
    add r9
    link #4
    iwt r15,#FxProjectVertex
    nop
    from r1
    stw (r8)
    inc r8
    inc r8
    from r2
    stw (r8)
    inc r8
    inc r8
    stw (r8)
    inc r8
    inc r8
    lms r6,(!fx_outcodes-!gsu_ram)
    or r6
    dec r12
    bne .l8BCC
    sbk
    lms r0,(!fx_rom_bank-!gsu_ram)
    romb
    lms r14,(!fx_rom_r14-!gsu_ram)
    iwt r15,#FxModelProjectRest
    nop
;
; @routine 01:8C79 FxModelProjectRest
; WHAT IT DOES: The rest of command $0C's projection: each point's
; screen position and outcode, then the next command.
; HOW: Reached by jump from 01:8BFE.
; Entry: R10 = stack; R11 = return address; R14 at the command's bytes
; (bank $001C).
; Exit: as the code leaves the registers, flags and RAM; R14 and the
; ROM buffer move as on the Super FX.
%at($018C79)
FxModelProjectRest:
    iwt r15,#.l8D23
    nop
.l8C7D:
    with r7
    sub #4
    move r0,r10
    lms r10,(!fx_model_stack-!gsu_ram)
    iwt r15,#FxModelProject_l8BBF
    nop
.l8C89:
    lms r11,(!fx_screen_right-!gsu_ram)
    lms r13,(!fx_screen_bottom-!gsu_ram)
    lms r1,(!fx_model_34-!gsu_ram)
    lms r9,(!fx_model_36-!gsu_ram)
    lms r3,(!fx_object_x-!gsu_ram)
    lms r4,(!fx_object_y-!gsu_ram)
    lms r5,(!fx_object_z-!gsu_ram)
    lms r12,(!fx_points_total-!gsu_ram)
    sms (!fx_model_stack-!gsu_ram),r10
    ibt r10,#$00
    ibt r0,#$14
    romb
    sms (!fx_rom_r14-!gsu_ram),r14
    iwt r7,#$0344
    iwt r8,#$04E4
    cache
.l8CB4:
    ldw (r7)
    add r5
    iwt r6,#$0100
    sub r6
    bpl .l8CC1
    add r6
    iwt r15,#.l8C7D
    nop
.l8CC1:
    iwt r6,#$43FE
    sub r6
    bmi .l8CCA
    add r6
    move r0,r6
.l8CCA:
    bic #1
    iwt r6,#$896C
    to r14
    add r6
    dec r7
    dec r7
    ldw (r7)
    to r2
    add r4
    to r6
    getb
    inc r14
    dec r7
    dec r7
    ldw (r7)
    add r3
    with r6
    getbh
    fmult
    add r1
    stw (r8)
    ibt r14,#$00
    bmi .l8CEC
    sub r11
    bmi .l8CF0
    nop
    inc r14
.l8CEC:
    inc r14
    with r14
    mult #4
.l8CF0:
    inc r8
    inc r8
    from r2
    fmult
    add r9
    stw (r8)
    bmi .l8CFD
    sub r13
    bmi .l8CFE
    nop
    inc r14
.l8CFD:
    inc r14
.l8CFE:
    inc r8
    inc r8
    ibt r0,#$1F
    xor r14
    swap
    or r14
    stw (r8)
    to r10
    or r10
    inc r8
    with r7
    add #10
    dec r12
    bne .l8CB4
    inc r8
    iwt r15,#.l8D15
    nop
.l8D15:
    sms (!fx_outcodes-!gsu_ram),r10
    lms r10,(!fx_model_stack-!gsu_ram)
    lms r0,(!fx_rom_bank-!gsu_ram)
    romb
    lms r14,(!fx_rom_r14-!gsu_ram)
.l8D23:
    lms r0,(!fx_outcodes-!gsu_ram)
    iwt r1,#$1F00
    and r1
    cmp r1
    beq .l8D33
    nop
    iwt r15,#FxModelEnd_l924D
    nop
.l8D33:
    lms r1,(!fx_outcodes-!gsu_ram)
    ibt r0,#$1F
    and r1
    bne .l8D44
    nop
    lms r0,(!fx_model_flags-!gsu_ram)
    iwt r1,#$4000
    or r1
    sbk
.l8D44:
    lms r14,(!fx_model_18-!gsu_ram)
    sms (!fx_rom_r14-!gsu_ram),r14
    to r15
    getb
    inc r14
;
; @routine 01:8D4D FxModelListStart
; WHAT IT DOES: Command $58: starts the sub-list returns: $0056 = $0788
; and 01:8E7D pushed on the R10 stack, then the next command.
; HOW: CACHE; dispatch.
; Entry: R10 = stack; R11 = return address; R14 at the command's bytes
; (bank $001C).
; Exit: as the code leaves the registers, flags and RAM; R14 and the
; ROM buffer move as on the Super FX.
%at($018D4D)
FxModelListStart:
    iwt r1,#$0788
    sms (!fx_model_return_at-!gsu_ram),r1
    iwt r0,#$8E7D
    stw (r10)
    inc r10
    inc r10
    cache
    to r15
    getb
    inc r14
;
; @routine 01:8D5D FxModelCall
; WHAT IT DOES: Command $60: calls the commands at the signed word
; offset that follows, the return (past the word) kept at the $0056
; stack.
; HOW: R14 refilled (DEC, INC); the return stored and $0056 stepped;
; R14 += the offset; dispatch.
; Entry: R10 = stack; R11 = return address; R14 at the command's bytes
; (bank $001C).
; Exit: as the code leaves the registers, flags and RAM; R14 and the
; ROM buffer move as on the Super FX.
%at($018D5D)
FxModelCall:
    dec r14
    inc r14
    move r0,r14
    inc r0
    inc r0
    lms r1,(!fx_model_return_at-!gsu_ram)
    stw (r1)
    inc r1
    inc r1
    sms (!fx_model_return_at-!gsu_ram),r1
    getb
    inc r14
    getbh
    to r14
    add r14
    to r15
    getb
    inc r14
;
; @routine 01:8D75 FxModelReturnPull
; WHAT IT DOES: Command $64: return, pulling the next command address
; from the R10 stack.
; HOW: DEC R10 twice, R15 from (R10).
; Entry: R10 = stack; R11 = return address; R14 at the command's bytes
; (bank $001C).
; Exit: as the code leaves the registers, flags and RAM; R14 and the
; ROM buffer move as on the Super FX.
%at($018D75)
FxModelReturnPull:
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop
;
; @routine 01:8D7A FxModelIfCall
; WHAT IT DOES: Command $5C: the flag byte at $07C6 plus the next byte
; picks, by its bit 7, one of two lists at the word offsets that
; follow; the one picked runs with R14 pushed on the R10 stack and
; comes back past both.
; HOW: R14 pushed on the R10 stack around the call; two forms (8D7A,
; 8DBD).
; Entry: R10 = stack; R11 = return address; R14 at the command's bytes
; (bank $001C).
; Exit: as the code leaves the registers, flags and RAM; R14 and the
; ROM buffer move as on the Super FX.
%at($018D7A)
FxModelIfCall:
    getb
    inc r14
    iwt r1,#$07C6
    add r1
    ldb (r0)
    swap
    moves r0,r0
    bmi .l8DBD
    nop
    from r14
    stw (r10)
    inc r10
    inc r10
    getb
    inc r14
    getbh
    to r14
    add r14
    from r15
    add #5
    stw (r10)
    inc r10
    inc r10
    to r15
    getb
    inc r14
    dec r10
    dec r10
    to r14
    ldw (r10)
    inc r14
    inc r14
    from r14
    stw (r10)
    inc r10
    inc r10
    getb
    inc r14
    getbh
    to r14
    add r14
    from r15
    add #5
    stw (r10)
    inc r10
    inc r10
    to r15
    getb
    inc r14
    dec r10
    dec r10
    to r14
    ldw (r10)
    inc r14
    inc r14
    to r15
    getb
    inc r14
.l8DBD:
    from r14
    stw (r10)
    inc r10
    inc r10
    inc r14
    inc r14
    getb
    inc r14
    getbh
    to r14
    add r14
    from r15
    add #5
    stw (r10)
    inc r10
    inc r10
    to r15
    getb
    inc r14
    dec r10
    dec r10
    to r14
    ldw (r10)
    dec r14
    inc r14
    from r14
    stw (r10)
    inc r10
    inc r10
    getb
    inc r14
    getbh
    to r14
    add r14
    from r15
    add #5
    stw (r10)
    inc r10
    inc r10
    to r15
    getb
    inc r14
    dec r10
    dec r10
    to r14
    ldw (r10)
    inc r14
    inc r14
    inc r14
    inc r14
    to r15
    getb
    inc r14
;
; @routine 01:8DF6 FxModelListStart3C
; WHAT IT DOES: Command $3C: as command $58.
; HOW: $0056 = $0788, 01:8E7D pushed, dispatch.
; Entry: R10 = stack; R11 = return address; R14 at the command's bytes
; (bank $001C).
; Exit: as the code leaves the registers, flags and RAM; R14 and the
; ROM buffer move as on the Super FX.
%at($018DF6)
FxModelListStart3C:
    iwt r1,#$0788
    sms (!fx_model_return_at-!gsu_ram),r1
    iwt r0,#$8E7D
    stw (r10)
    inc r10
    inc r10
    cache
    to r15
    getb
    inc r14
;
; @routine 01:8E06 FxModelPushReturn
; WHAT IT DOES: Command $44: keeps the address at the word offset that
; follows on the $0056 stack, then returns as command $40.
; HOW: Falls into 01:8E14.
; Entry: R10 = stack; R11 = return address; R14 at the command's bytes
; (bank $001C).
; Exit: as the code leaves the registers, flags and RAM; R14 and the
; ROM buffer move as on the Super FX.
%at($018E06)
FxModelPushReturn:
    getb
    inc r14
    getbh
    add r14
    lms r1,(!fx_model_return_at-!gsu_ram)
    stw (r1)
    inc r1
    inc r1
    sms (!fx_model_return_at-!gsu_ram),r1
;
; @routine 01:8E14 FxModelReturn40
; WHAT IT DOES: Command $40: return, pulling the next command address
; from the R10 stack.
; HOW: DEC R10 twice, R15 from (R10).
; Entry: R10 = stack; R11 = return address; R14 at the command's bytes
; (bank $001C).
; Exit: as the code leaves the registers, flags and RAM; R14 and the
; ROM buffer move as on the Super FX.
%at($018E14)
FxModelReturn40:
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop
;
; @routine 01:8E19 FxModelIfSkip
; WHAT IT DOES: Command $28: the flag byte at $07C6 plus the next byte
; picks, by its bit 7, whether the list that follows runs (R14 pushed
; on the R10 stack) or is passed by its length byte.
; HOW: R14 pushed on the R10 stack around each list.
; Entry: R10 = stack; R11 = return address; R14 at the command's bytes
; (bank $001C).
; Exit: as the code leaves the registers, flags and RAM; R14 and the
; ROM buffer move as on the Super FX.
%at($018E19)
FxModelIfSkip:
    getb
    inc r14
    iwt r1,#$07C6
    add r1
    ldb (r0)
    swap
    moves r0,r0
    bmi .l8E45
    nop
    inc r14
    inc r14
    from r14
    stw (r10)
    inc r10
    inc r10
    getb
    lob
    beq .l8E3D
    nop
    to r14
    add r14
    from r15
    add #5
    stw (r10)
    inc r10
    inc r10
    to r15
    getb
    inc r14
.l8E3D:
    dec r10
    dec r10
    to r14
    ldw (r10)
    inc r14
    to r15
    getb
    inc r14
.l8E45:
    getb
    inc r14
    getbh
    to r12
    add r14
    inc r14
    from r14
    stw (r10)
    inc r10
    inc r10
    inc r14
    from r12
    stw (r10)
    inc r10
    inc r10
    from r15
    add #5
    stw (r10)
    inc r10
    inc r10
    to r15
    getb
    inc r14
    dec r10
    dec r10
    ldw (r10)
    lms r1,(!fx_model_return_at-!gsu_ram)
    stw (r1)
    inc r1
    inc r1
    sms (!fx_model_return_at-!gsu_ram),r1
    dec r10
    dec r10
    to r14
    ldw (r10)
    getb
    lob
    beq .l8E78
    nop
    to r14
    add r14
    to r15
    getb
    inc r14
.l8E78:
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop
    sub r0
    lms r1,(!fx_model_return_at-!gsu_ram)
    stw (r1)
    iwt r0,#$0788
    sms (!fx_model_return_at-!gsu_ram),r0
.l8E88:
    lms r1,(!fx_model_return_at-!gsu_ram)
    to r14
    ldw (r1)
    inc r1
    inc r1
    sms (!fx_model_return_at-!gsu_ram),r1
    moves r14,r14
    beq .l8EA3
    nop
    from r15
    add #5
    stw (r10)
    inc r10
    inc r10
    to r15
    getb
    inc r14
    bra .l8E88
    nop
.l8EA3:
    iwt r15,#FxModelEnd
    nop
;
; @routine 01:8EA7 FxModelDepthSort
; WHAT IT DOES: Command $10: n sub-lists (n the next byte) run back to
; front: the depth words of the n points the following bytes pick
; ($0344 + 6index) copied to $07B0, then n times the deepest left
; marked $8000 and its list (a word offset in the table after the
; bytes) run, coming back to 01:8EF9 (not reached in the traces so
; far).
; HOW: $0148 = n, $014E counts down, $0150 = the offset table; the end
; command when done.
; Entry: R10 = stack; R11 = return address; R14 at the command's bytes
; (bank $001C).
; Exit: as the code leaves the registers, flags and RAM; R14 and the
; ROM buffer move as on the Super FX.
%at($018EA7)
FxModelDepthSort:
    to r12
    getb
    inc r14
    iwt r0,#$F850
    add r12
    add r14
    sms (!fx_model_150-!gsu_ram),r0
    sms (!fx_model_14e-!gsu_ram),r12
    sms (!fx_model_148-!gsu_ram),r12
    iwt r9,#$0344
    iwt r5,#$07B0
    move r13,r15
    getb
    inc r14
    umult #6
    add r9
    ldw (r0)
    stw (r5)
    inc r5
    loop
    inc r5
.l8ECA:
    iwt r5,#$07B0
    iwt r7,#$8000
    lms r12,(!fx_model_148-!gsu_ram)
    move r13,r15
    ldw (r5)
    cmp r7
    blt .l8EDF
    nop
    move r7,r0
    move r6,r5
.l8EDF:
    inc r5
    loop
    inc r5
    iwt r0,#$8000
    stw (r6)
    lms r0,(!fx_model_150-!gsu_ram)
    to r14
    add r6
    getb
    inc r14
    getbh
    move r14,r0
    from r15
    add #5
    stw (r10)
    inc r10
    inc r10
    to r15
    getb
    inc r14
    lms r0,(!fx_model_14e-!gsu_ram)
    dec r0
    bne .l8ECA
    sbk
    iwt r15,#FxModelEnd
    nop
;
; @routine 01:8F05 FxModelIfSprite
; WHAT IT DOES: Command $54: the flag byte at $07C6 plus the next byte;
; with bit 7 set, command $50 follows (01:8F18), else its bytes are
; skipped (not reached in the traces so far).
; HOW: SWAP, HIB, BMI.
; Entry: R10 = stack; R11 = return address; R14 at the command's bytes
; (bank $001C).
; Exit: as the code leaves the registers, flags and RAM; R14 and the
; ROM buffer move as on the Super FX.
%at($018F05)
FxModelIfSprite:
    getb
    inc r14
    iwt r1,#$07C6
    add r1
    ldb (r0)
    swap
    hib
    bmi FxModelSprite
    nop
    inc r14
    inc r14
    inc r14
    to r15
    getb
    inc r14
;
; @routine 01:8F18 FxModelSprite
; WHAT IT DOES: Command $50: a shape at the projected point ($04E4 + 6
; * the next byte) whose depth, against $002A, sets its size; its
; colour from FxModelSpriteColour (01:9250) and a LINK to 01:AB11 (not
; reached in the traces so far).
; HOW: $00BA-$00D2 set for it; ROMB and R14 back from $001C and $001A.
; Entry: R10 = stack; R11 = return address; R14 at the command's bytes
; (bank $001C).
; Exit: as the code leaves the registers, flags and RAM; R14 and the
; ROM buffer move as on the Super FX.
%at($018F18)
FxModelSprite:
    getb
    inc r14
    to r3
    umult #6
    iwt r1,#$04E4
    from r3
    to r2
    add r1
    ldw (r2)
    inc r2
    inc r2
    sms (!fx_model_ba-!gsu_ram),r0
    ldw (r2)
    sms (!fx_model_bc-!gsu_ram),r0
    iwt r1,#$04E4
    iwt r1,#$0344
    from r3
    add r1
    ldw (r0)
    lms r1,(!fx_object_z-!gsu_ram)
    add r1
    sms (!fx_model_be-!gsu_ram),r0
    iwt r1,#$0020
    sub r1
    bmi .l8F78
    nop
    link #4
    iwt r15,#FxModelSpriteColour
    nop
    inc r14
    getb
    inc r14
    lms r0,(!fx_tex_mask-!gsu_ram)
    lob
    inc r0
    sms (!fx_model_c8-!gsu_ram),r0
    iwt r1,#$0100
    add r0
    swap
    bpl .l8F5E
    nop
    with r1
    lsr
.l8F5E:
    sms (!fx_model_ce-!gsu_ram),r1
    sms (!fx_rom_r14-!gsu_ram),r14
    sub r0
    sms (!fx_model_d2-!gsu_ram),r0
    link #4
    iwt r15,#FxReturnR11
    nop
    lms r0,(!fx_rom_bank-!gsu_ram)
    romb
    lms r14,(!fx_rom_r14-!gsu_ram)
    to r15
    getb
    inc r14
.l8F78:
    inc r14
    inc r14
    to r15
    getb
    inc r14
;
; @routine 01:8F7D FxModelMaterial
; WHAT IT DOES: Sets the face colour (and for textured faces the
; texture's bank, address and mask) from the face's material word in
; ROM: a shade from the light ($0106-$010A) for shaded materials.
; HOW: R2 keeps R14; ROMB $03; the material table at $004A, looked up
; through $0046 for animated ones; the shade table at $004C; ROMB and
; R14 back. Returns with JMP R11.
; Entry: R14 at the face's material word; R11 = return address.
; Exit: as the code leaves the registers, flags and RAM; R14 and the
; ROM buffer move as on the Super FX.
%at($018F7D)
FxModelMaterial:
    move r2,r14
    lms r0,(!fx_model_flags-!gsu_ram)
    iwt r1,#$2000
    and r1
    beq .l8F98
    nop
    sub r0
    sms (!fx_face_kind-!gsu_ram),r0
    ibt r0,#$02
    cmode
    iwt r0,#$0009
    iwt r15,#.l90B6
    nop
.l8F98:
    getb
    add r0
    ibt r1,#$03
    from r1
    romb
    lms r1,(!fx_material_table-!gsu_ram)
    to r14
    add r1
.l8FA4:
    to r3
    getb
    inc r14
    with r3
    getbh
    moves r3,r3
    bpl .l8FE1
    nop
    from r3
    add r3
    bpl .l8FCF
    nop
    iwt r0,#$FFFE
    sms (!fx_face_kind-!gsu_ram),r0
    from r3
    hib
    and #15
    add r0
    add r0
    add r0
    add r0
    swap
    or #8
    swap
    sms (!fx_face_colour-!gsu_ram),r0
    ibt r0,#$FE
    iwt r15,#.l90B6
    nop
.l8FCF:
    iwt r0,#$1FFF
    to r14
    and r3
    lms r0,(!fx_material_mask-!gsu_ram)
    to r3
    getb
    dec r3
    and r3
    add r0
    inc r0
    to r14
    bra .l8FA4
    add r14
.l8FE1:
    from r3
    lob
    iwt r1,#$9FB1
    to r14
    add r1
    to r1
    getb
    iwt r0,#$0800
    add r1
    sms (!fx_face_colour-!gsu_ram),r0
    from r3
    add r3
    bpl .l8FF7
    sub r0
    dec r0
.l8FF7:
    sms (!fx_face_kind-!gsu_ram),r0
    beq .l9045
    nop
    iwt r0,#$2000
    and r3
    beq .l900B
    nop
    iwt r0,#$0C00
    add r1
    sms (!fx_face_colour-!gsu_ram),r0
.l900B:
    from r3
    lob
    umult #3
    iwt r1,#$9D59
    to r14
    add r1
    getb
    inc r14
    getbh
    inc r14
    sms (!fx_tex_base-!gsu_ram),r0
    getb
    sms (!fx_tex_bank-!gsu_ram),r0
    from r3
    hib
    ibt r1,#$1F
    and r1
    add r0
    iwt r1,#$A079
    to r14
    add r1
    getb
    inc r14
    getbh
    move r14,r0
    to r1
    getb
    inc r14
    with r1
    getbh
    sms (!fx_tex_mask-!gsu_ram),r1
    inc r0
    inc r0
    sms (!fx_uv_at-!gsu_ram),r0
    ibt r0,#$FF
    iwt r15,#.l90B6
    nop
.l9045:
    ibt r1,#$3F
    from r3
    hib
    inc r0
    and r1
    beq .l90B3
    nop
    inc r0
    and r1
    bne .l905E
    nop
    from r3
    lob
    lms r3,(!fx_shade_at-!gsu_ram)
    to r14
    add r3
    getb
    bra .l90B6
    color
.l905E:
    dec r0
    dec r0
    and r1
    move r3,r0
    lms r0,(!fx_shade_table-!gsu_ram)
    add r3
    to r14
    add r3
    to r6
    getb
    inc r14
    with r6
    getbh
    lms r0,(!fx_rom_bank-!gsu_ram)
    romb
    move r14,r2
    inc r14
    getb
    inc r14
    lms r4,(!fx_light_local_x-!gsu_ram)
    to r5
    mult r4
    getb
    inc r14
    lms r4,(!fx_light_local_y-!gsu_ram)
    mult r4
    to r5
    add r5
    getb
    lms r4,(!fx_light_local_z-!gsu_ram)
    mult r4
    add r5
    asr
    asr
    hib
    sex
    ibt r4,#$06
    cmp r4
    bge .l909C
    nop
    ibt r0,#$06
    bra .l90A5
    nop
.l909C:
    ibt r4,#$10
    cmp r4
    blt .l90A5
    nop
    ibt r0,#$0F
.l90A5:
    to r1
    sub #6
    ibt r0,#$03
    romb
    from r6
    to r14
    add r1
    getb
    bra .l90B6
    color
.l90B3:
    from r3
    lob
    color
.l90B6:
    lms r1,(!fx_rom_bank-!gsu_ram)
    from r1
    romb
    move r14,r2
    jmp r11
    nop
.l90C0:
    inc r0
    lob
    beq .l90C8
    nop
    to r15
    getb
    inc r14
.l90C8:
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop
;
; @routine 01:90CD FxModelFaces
; WHAT IT DOES: Command $14: the faces that follow, each with its
; material (FxModelMaterial), its points gathered and clipped
; (FxPolygonGather or FxPolygonDraw, FxPolygonClipScreen) and queued in
; the draw list, which is drawn (FxDrawList) when nearly full.
; HOW: CACHE; a face count byte less than 0 ends it.
; Entry: R10 = stack; R11 = return address; R14 at the command's bytes
; (bank $001C).
; Exit: as the code leaves the registers, flags and RAM; R14 and the
; ROM buffer move as on the Super FX.
%at($0190CD)
FxModelFaces:
    cache
    getb
    lob
    bmi FxModelMaterial_l90C0
    inc r14
    inc r14
    move r2,r0
    sub #2
    beq .l90EF
    getb
    iwt r1,#$07C6
    add r1
    ldb (r0)
    lob
    bmi .l90F0
    inc r14
    beq .l90F0
    nop
    with r2
    add #4
    with r14
    bra FxModelFaces
    add r2
.l90EF:
    inc r14
.l90F0:
    sms (!fx_point_count-!gsu_ram),r2
    iwt r0,#$09AA
    lms r6,(!fx_list_end-!gsu_ram)
    sub r6
    bpl .l910D
    nop
    from r14
    stw (r10)
    inc r10
    inc r10
    link #4
    iwt r15,#FxDrawList
    nop
    dec r10
    dec r10
    to r14
    ldw (r10)
    lms r6,(!fx_list_end-!gsu_ram)
.l910D:
    link #4
    iwt r15,#FxModelMaterial
    nop
    lms r6,(!fx_list_end-!gsu_ram)
    move r3,r0
    with r14
    add #4
    lms r12,(!fx_point_count-!gsu_ram)
    from r3
    lob
    swap
    or r12
    moves r3,r3
    bpl .l9142
    nop
    from r12
    not
    to r1
    lob
    lms r0,(!fx_tex_bank-!gsu_ram)
    lob
    swap
    or r1
    stw (r6)
    inc r6
    inc r6
    lms r0,(!fx_face_colour-!gsu_ram)
    stw (r6)
    inc r6
    inc r6
    lms r0,(!fx_tex_base-!gsu_ram)
    stw (r6)
    inc r6
    inc r6
    lms r0,(!fx_tex_mask-!gsu_ram)
.l9142:
    stw (r6)
    inc r6
    inc r6
    move r7,r6
    iwt r9,#$04E4
    ibt r5,#$00
    move r13,r15
    to r4
    getb
    inc r14
    from r4
    umult #6
    to r8
    add r9
    ldw (r8)
    stw (r6)
    inc r6
    inc r6
    inc r8
    inc r8
    ldw (r8)
    stw (r6)
    inc r6
    inc r6
    inc r8
    inc r8
    ldw (r8)
    to r5
    or r5
    moves r3,r3
    bpl .l918B
    nop
    sms (!fx_rom_r14-!gsu_ram),r14
    ibt r0,#$03
    romb
    lms r14,(!fx_uv_at-!gsu_ram)
    getb
    inc r14
    swap
    stw (r6)
    inc r6
    inc r6
    getb
    inc r14
    swap
    stw (r6)
    inc r6
    inc r6
    lms r0,(!fx_rom_bank-!gsu_ram)
    sms (!fx_uv_at-!gsu_ram),r14
    romb
    lms r14,(!fx_rom_r14-!gsu_ram)
.l918B:
    loop
    nop
    from r5
    lob
    bne .l9199
    nop
    sms (!fx_list_end-!gsu_ram),r6
    iwt r15,#FxModelFaces
    nop
.l9199:
    iwt r1,#$1F00
    from r1
    and r5
    sub r1
    beq .l91A6
    nop
    iwt r15,#.l923F
    nop
.l91A6:
    nop
    sms (!fx_outcodes-!gsu_ram),r5
    sms (!fx_rom_r14-!gsu_ram),r14
    sms (!fx_poly_out-!gsu_ram),r7
    from r5
    lob
    beq .l91CA
    nop
    lms r0,(!fx_face_kind-!gsu_ram)
    hib
    beq .l91CA
    nop
    link #4
    iwt r15,#FxPolygonDraw
    nop
    moves r0,r0
    bne .l920F
    nop
    iwt r15,#.l923C
    nop
.l91CA:
    ibt r0,#$10
    and r5
    beq .l91EB
    nop
    lms r0,(!fx_point_count-!gsu_ram)
    with r14
    sub r0
    link #4
    iwt r15,#FxPolygonGather
    nop
    moves r0,r0
    beq .l923C
    nop
    lms r1,(!fx_face_kind-!gsu_ram)
    with r1
    hib
    beq .l91F5
    nop
    iwt r15,#.l9205
    nop
.l91EB:
    lms r0,(!fx_face_kind-!gsu_ram)
    hib
    bne .l9202
    nop
    lms r0,(!fx_point_count-!gsu_ram)
.l91F5:
    link #4
    iwt r15,#FxPolygonClipScreen
    nop
    moves r0,r0
    beq .l923C
    nop
    bra .l920F
    nop
.l9202:
    lms r0,(!fx_point_count-!gsu_ram)
.l9205:
    link #4
    iwt r15,#FxPolygonClipScreenUV
    nop
    moves r0,r0
    beq .l923C
    nop
.l920F:
    lms r3,(!fx_face_kind-!gsu_ram)
    lms r6,(!fx_poly_out-!gsu_ram)
    move r2,r0
    moves r3,r3
    bpl .l922C
    add r0
    add r0
    add r0
    add r6
    sms (!fx_list_end-!gsu_ram),r0
    with r6
    sub #8
    from r2
    not
    stb (r6)
    bra .l9237
    nop
.l922C:
    add r0
    add r6
    sms (!fx_list_end-!gsu_ram),r0
    with r6
    sub #2
    from r2
    stb (r6)
.l9237:
    lms r0,(!fx_rom_bank-!gsu_ram)
    romb
.l923C:
    lms r14,(!fx_rom_r14-!gsu_ram)
.l923F:
    iwt r15,#FxModelFaces
    nop
;
; @routine 01:9243 FxModelEnd
; WHAT IT DOES: Command $00: draws the queued faces (FxDrawList) and
; returns from the model, pulling R15.
; HOW: LINK to FxDrawList, then DEC R10 twice and R15 from (R10).
; Entry: R10 = stack; R11 = return address; R14 at the command's bytes
; (bank $001C).
; Exit: as the code leaves the registers, flags and RAM; R14 and the
; ROM buffer move as on the Super FX.
%at($019243)
FxModelEnd:
    link #4
    iwt r15,#FxDrawList
    nop
.l9248:
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop
.l924D:
    bra .l9248
    nop
;
; @routine 01:9250 FxModelSpriteColour
; WHAT IT DOES: The sprite's colour: as FxModelMaterial for the flat
; case (not reached in the traces so far).
; HOW: Shares FxSpriteMaterial's ending (01:939E).
; Entry: R14 at the sprite's bytes; R11 = return address.
; Exit: as the code leaves the registers, flags and RAM; R14 and the
; ROM buffer move as on the Super FX.
%at($019250)
FxModelSpriteColour:
    move r2,r14
    lms r0,(!fx_model_flags-!gsu_ram)
    iwt r1,#$2000
    to r1
    and r1
    beq .l9275
    nop
    iwt r1,#$1000
    and r1
    bne .l9275
    nop
    sub r0
    cmode
    ibt r0,#$09
    color
    sub r0
    sms (!fx_face_colour-!gsu_ram),r0
    sms (!fx_face_kind-!gsu_ram),r0
    iwt r15,#FxSpriteMaterial_l939E
    nop
.l9275:
    getb
;
; @routine 01:9276 FxSpriteMaterial
; WHAT IT DOES: Sets the colour of a sprite from its material word: as
; FxModelMaterial without the textured faces.
; HOW: R2 keeps R14; ends at 939E: ROMB from $001C, R14 = R2, JMP R11.
; Entry: R0 = the material index; R11 = return address.
; Exit: as the code leaves the registers, flags and RAM; R14 and the
; ROM buffer move as on the Super FX.
%at($019276)
FxSpriteMaterial:
    add r0
    ibt r1,#$03
    from r1
    romb
    lms r1,(!fx_material_table-!gsu_ram)
    to r14
    add r1
.l9281:
    to r3
    getb
    inc r14
    with r3
    getbh
    moves r3,r3
    bpl .l92BC
    nop
    from r3
    add r3
    bpl .l92AA
    nop
    iwt r0,#$FFFE
    sms (!fx_face_kind-!gsu_ram),r0
    from r3
    hib
    and #15
    add r0
    add r0
    add r0
    add r0
    swap
    or #8
    swap
    sms (!fx_face_colour-!gsu_ram),r0
    iwt r15,#.l939E
    nop
.l92AA:
    iwt r0,#$1FFF
    to r14
    and r3
    lms r0,(!fx_material_mask-!gsu_ram)
    to r3
    getb
    dec r3
    and r3
    add r0
    inc r0
    to r14
    bra .l9281
    add r14
.l92BC:
    from r3
    lob
    iwt r1,#$9FB1
    to r14
    add r1
    to r1
    getb
    iwt r0,#$0800
    add r1
    sms (!fx_face_colour-!gsu_ram),r0
    from r3
    add r3
    bpl .l92D2
    sub r0
    dec r0
.l92D2:
    sms (!fx_face_kind-!gsu_ram),r0
    beq .l931E
    nop
    iwt r0,#$2000
    and r3
    beq .l92E6
    nop
    iwt r0,#$0C00
    add r1
    sms (!fx_face_colour-!gsu_ram),r0
.l92E6:
    from r3
    lob
    umult #3
    iwt r1,#$9D59
    to r14
    add r1
    getb
    inc r14
    getbh
    inc r14
    sms (!fx_tex_base-!gsu_ram),r0
    getb
    sms (!fx_tex_bank-!gsu_ram),r0
    from r3
    hib
    ibt r1,#$1F
    and r1
    add r0
    iwt r1,#$A079
    to r14
    add r1
    getb
    inc r14
    getbh
    move r14,r0
    to r1
    getb
    inc r14
    with r1
    getbh
    sms (!fx_tex_mask-!gsu_ram),r1
    inc r0
    inc r0
    sms (!fx_uv_at-!gsu_ram),r0
    iwt r15,#.l939E
    nop
.l931E:
    ibt r1,#$3F
    from r3
    hib
    inc r0
    and r1
    bne .l932B
    nop
    iwt r15,#.l939B
    nop
.l932B:
    inc r0
    and r1
    beq .l9334
    nop
    iwt r15,#.l9340
    nop
.l9334:
    from r3
    lob
    lms r3,(!fx_shade_at-!gsu_ram)
    to r14
    add r3
    getc
    iwt r15,#.l939E
    nop
.l9340:
    dec r0
    dec r0
    and r1
    move r3,r0
    lms r0,(!fx_model_flags-!gsu_ram)
    hib
    bpl .l939B
    nop
    lms r0,(!fx_shade_table-!gsu_ram)
    add r3
    to r14
    add r3
    to r6
    getb
    inc r14
    with r6
    getbh
    lms r0,(!fx_rom_bank-!gsu_ram)
    romb
    move r14,r2
    inc r14
    getb
    inc r14
    lms r4,(!fx_light_local_x-!gsu_ram)
    to r5
    mult r4
    getb
    inc r14
    lms r4,(!fx_light_local_y-!gsu_ram)
    mult r4
    to r5
    add r5
    getb
    lms r4,(!fx_light_local_z-!gsu_ram)
    mult r4
    add r5
    asr
    asr
    hib
    sex
    ibt r4,#$06
    cmp r4
    bge .l9385
    nop
    ibt r0,#$06
    bra .l938E
    nop
.l9385:
    ibt r4,#$10
    cmp r4
    blt .l938E
    nop
    ibt r0,#$0F
.l938E:
    to r1
    sub #6
    ibt r0,#$03
    romb
    from r6
    to r14
    add r1
    bra .l939E
    getc
.l939B:
    from r3
    lob
    color
.l939E:
    lms r0,(!fx_rom_bank-!gsu_ram)
    romb
    move r14,r2
    jmp r11
    nop
;
; @routine 01:93A7 FxModelFacing
; WHAT IT DOES: Command $30: for n triangles (n the next byte) of three
; projected points ($04E4 + 6index), the sign of their cross product
; XORed with the points' attribute bytes, stored as a flag byte at
; $07C6 on (skipped to 01:941B when bit 14 of $0054 is set).
; HOW: A LOOP of R12 = n (R13 = $93C1); LMULT twice, SUB and SBC.
; Entry: R10 = stack; R11 = return address; R14 at the command's bytes
; (bank $001C).
; Exit: as the code leaves the registers, flags and RAM; R14 and the
; ROM buffer move as on the Super FX.
%at($0193A7)
FxModelFacing:
    lms r0,(!fx_model_flags-!gsu_ram)
    iwt r1,#$4000
    and r1
    beq .l93B5
    nop
    iwt r15,#.l941B
    nop
.l93B5:
    to r12
    getb
    inc r14
    iwt r9,#$04E4
    iwt r11,#$07C6
    cache
    move r13,r15
    getb
    inc r14
    mult #6
    add r9
    to r1
    ldw (r0)
    inc r0
    inc r0
    to r2
    ldw (r0)
    inc r0
    inc r0
    to r5
    ldb (r0)
    getb
    inc r14
    mult #6
    to r8
    add r9
    ldw (r8)
    inc r8
    inc r8
    to r3
    sub r1
    ldw (r8)
    inc r8
    inc r8
    to r7
    sub r2
    ldb (r8)
    to r5
    xor r5
    getb
    inc r14
    mult #6
    to r8
    add r9
    ldw (r8)
    inc r8
    inc r8
    to r1
    sub r1
    ldw (r8)
    inc r8
    inc r8
    to r2
    sub r2
    ldb (r8)
    to r5
    xor r5
    move r6,r1
    with r7
    lmult
    move r1,r4
    move r6,r3
    with r2
    lmult
    with r4
    sub r1
    with r2
    sbc r7
    from r5
    add r5
    add r0
    add r0
    swap
    xor r2
    hib
    stb (r11)
    loop
    inc r11
    to r15
    getb
    inc r14
.l941B:
    to r12
    getb
    inc r14
    iwt r9,#$04E4
    iwt r11,#$07C6
    cache
    move r13,r15
    getb
    inc r14
    umult #6
    add r9
    to r1
    ldb (r0)
    inc r0
    inc r0
    to r2
    ldb (r0)
    getb
    inc r14
    umult #6
    to r8
    add r9
    ldb (r8)
    inc r8
    inc r8
    to r3
    sub r1
    ldb (r8)
    to r7
    sub r2
    getb
    inc r14
    umult #6
    to r8
    add r9
    ldb (r8)
    inc r8
    inc r8
    to r1
    sub r1
    ldb (r8)
    to r2
    sub r2
    move r6,r1
    with r7
    lmult
    move r1,r4
    move r6,r3
    with r2
    lmult
    with r4
    sub r1
    from r2
    sbc r7
    hib
    stb (r11)
    loop
    inc r11
    to r15
    getb
    inc r14
;
; @routine 01:9A52 FxPolygonClipScreenUV
; WHAT IT DOES: Clips a textured polygon (x, y, u, v words a vertex) at
; $005A against the four screen edges, as FxPolygonClipScreen does for
; flat ones (not reached in the traces so far).
; HOW: Pushes R11; the crossings by FxDivideSigned; pulls R15.
; Entry: R0 = the vertex count; R10 = stack; R11 = return address.
; Exit: as the code leaves the registers, flags and RAM; R14 and the
; ROM buffer move as on the Super FX.
%at($019A52)
FxPolygonClipScreenUV:
    from r11
    stw (r10)
    inc r10
    inc r10
    ibt r1,#$03
    cmp r1
    bpl .l9A61
    nop
    iwt r15,#.l9EFB
    nop
.l9A61:
    sms (!fx_vertex_count_save-!gsu_ram),r0
    sms (!fx_vertex_count-!gsu_ram),r0
    lms r8,(!fx_poly_out-!gsu_ram)
    iwt r9,#$0688
    to r1
    ldw (r8)
    inc r8
    inc r8
    to r2
    ldw (r8)
    inc r8
    inc r8
    to r3
    ldw (r8)
    inc r8
    inc r8
    to r6
    ldw (r8)
    with r8
    sub #6
    lms r0,(!fx_vertex_count-!gsu_ram)
    mult #8
    add r8
    from r1
    stw (r0)
    inc r0
    inc r0
    from r2
    stw (r0)
    inc r0
    inc r0
    from r3
    stw (r0)
    inc r0
    inc r0
    from r6
    stw (r0)
.l9A92:
    to r1
    ldw (r8)
    inc r8
    inc r8
    to r2
    ldw (r8)
    inc r8
    inc r8
    with r8
    add #4
    to r3
    ldw (r8)
    inc r8
    inc r8
    to r6
    ldw (r8)
    with r8
    sub #2
    lms r0,(!fx_screen_left-!gsu_ram)
    from r1
    sub r0
    blt .l9AB2
    nop
    iwt r15,#.l9AC7
    nop
.l9AB2:
    lms r0,(!fx_screen_left-!gsu_ram)
    from r3
    sub r0
    blt .l9ABE
    nop
    iwt r15,#.l9AEF
    nop
.l9ABE:
    lms r0,(!fx_vertex_count_save-!gsu_ram)
    dec r0
    sbk
    iwt r15,#.l9B68
    nop
.l9AC7:
    lms r0,(!fx_screen_left-!gsu_ram)
    from r3
    sub r0
    blt .l9AD3
    nop
    iwt r15,#.l9B51
    nop
.l9AD3:
    lms r0,(!fx_vertex_count_save-!gsu_ram)
    inc r0
    sbk
    from r1
    stw (r9)
    inc r9
    inc r9
    from r2
    stw (r9)
    inc r9
    inc r9
    with r8
    sub #4
    ldw (r8)
    inc r8
    inc r8
    stw (r9)
    inc r9
    inc r9
    ldw (r8)
    inc r8
    inc r8
    stw (r9)
    inc r9
    inc r9
.l9AEF:
    with r3
    sub r1
    with r6
    sub r2
    lms r0,(!fx_screen_left-!gsu_ram)
    sub r1
    to r5
    lmult
    move r6,r3
    link #4
    iwt r15,#FxDivideSigned
    nop
    from r4
    add r2
    lms r6,(!fx_screen_left-!gsu_ram)
    from r6
    stw (r9)
    inc r9
    inc r9
    stw (r9)
    inc r9
    inc r9
    with r8
    add #4
    to r6
    ldw (r8)
    with r8
    sub #8
    to r2
    ldw (r8)
    inc r8
    inc r8
    with r6
    sub r2
    lms r0,(!fx_screen_left-!gsu_ram)
    sub r1
    to r5
    lmult
    move r6,r3
    link #4
    iwt r15,#FxDivideSigned
    nop
    from r4
    add r2
    stw (r9)
    inc r9
    inc r9
    to r2
    ldw (r8)
    with r8
    add #8
    to r6
    ldw (r8)
    with r8
    sub #6
    with r6
    sub r2
    lms r0,(!fx_screen_left-!gsu_ram)
    sub r1
    to r5
    lmult
    move r6,r3
    link #4
    iwt r15,#FxDivideSigned
    nop
    from r4
    add r2
    stw (r9)
    inc r9
    inc r9
    iwt r15,#.l9B68
    nop
.l9B51:
    from r1
    stw (r9)
    inc r9
    inc r9
    from r2
    stw (r9)
    inc r9
    inc r9
    with r8
    sub #4
    ldw (r8)
    inc r8
    inc r8
    stw (r9)
    inc r9
    inc r9
    ldw (r8)
    inc r8
    inc r8
    stw (r9)
    inc r9
    inc r9
.l9B68:
    lms r0,(!fx_vertex_count-!gsu_ram)
    dec r0
    sbk
    beq .l9B74
    nop
    iwt r15,#.l9A92
    nop
.l9B74:
    lms r0,(!fx_vertex_count_save-!gsu_ram)
    sub #3
    bpl .l9B80
    nop
    iwt r15,#.l9EFB
    nop
.l9B80:
    add #3
    sms (!fx_vertex_count-!gsu_ram),r0
    iwt r8,#$0688
    lms r9,(!fx_poly_out-!gsu_ram)
    to r1
    ldw (r8)
    inc r8
    inc r8
    to r2
    ldw (r8)
    inc r8
    inc r8
    to r3
    ldw (r8)
    inc r8
    inc r8
    to r6
    ldw (r8)
    with r8
    sub #6
    lms r0,(!fx_vertex_count-!gsu_ram)
    mult #8
    add r8
    from r1
    stw (r0)
    inc r0
    inc r0
    from r2
    stw (r0)
    inc r0
    inc r0
    from r3
    stw (r0)
    inc r0
    inc r0
    from r6
    stw (r0)
.l9BB0:
    to r1
    ldw (r8)
    inc r8
    inc r8
    to r2
    ldw (r8)
    inc r8
    inc r8
    with r8
    add #4
    to r3
    ldw (r8)
    inc r8
    inc r8
    to r6
    ldw (r8)
    with r8
    sub #2
    lms r0,(!fx_screen_right-!gsu_ram)
    from r1
    sub r0
    sub #1
    bge .l9BD2
    nop
    iwt r15,#.l9BE9
    nop
.l9BD2:
    lms r0,(!fx_screen_right-!gsu_ram)
    from r3
    sub r0
    sub #1
    bge .l9BE0
    nop
    iwt r15,#.l9C13
    nop
.l9BE0:
    lms r0,(!fx_vertex_count_save-!gsu_ram)
    dec r0
    sbk
    iwt r15,#.l9C8C
    nop
.l9BE9:
    lms r0,(!fx_screen_right-!gsu_ram)
    from r3
    sub r0
    sub #1
    bge .l9BF7
    nop
    iwt r15,#.l9C75
    nop
.l9BF7:
    lms r0,(!fx_vertex_count_save-!gsu_ram)
    inc r0
    sbk
    from r1
    stw (r9)
    inc r9
    inc r9
    from r2
    stw (r9)
    inc r9
    inc r9
    with r8
    sub #4
    ldw (r8)
    inc r8
    inc r8
    stw (r9)
    inc r9
    inc r9
    ldw (r8)
    inc r8
    inc r8
    stw (r9)
    inc r9
    inc r9
.l9C13:
    with r3
    sub r1
    with r6
    sub r2
    lms r0,(!fx_screen_right-!gsu_ram)
    sub r1
    to r5
    lmult
    move r6,r3
    link #4
    iwt r15,#FxDivideSigned
    nop
    from r4
    add r2
    lms r6,(!fx_screen_right-!gsu_ram)
    from r6
    stw (r9)
    inc r9
    inc r9
    stw (r9)
    inc r9
    inc r9
    with r8
    add #4
    to r6
    ldw (r8)
    with r8
    sub #8
    to r2
    ldw (r8)
    inc r8
    inc r8
    with r6
    sub r2
    lms r0,(!fx_screen_right-!gsu_ram)
    sub r1
    to r5
    lmult
    move r6,r3
    link #4
    iwt r15,#FxDivideSigned
    nop
    from r4
    add r2
    stw (r9)
    inc r9
    inc r9
    to r2
    ldw (r8)
    with r8
    add #8
    to r6
    ldw (r8)
    with r8
    sub #6
    with r6
    sub r2
    lms r0,(!fx_screen_right-!gsu_ram)
    sub r1
    to r5
    lmult
    move r6,r3
    link #4
    iwt r15,#FxDivideSigned
    nop
    from r4
    add r2
    stw (r9)
    inc r9
    inc r9
    iwt r15,#.l9C8C
    nop
.l9C75:
    from r1
    stw (r9)
    inc r9
    inc r9
    from r2
    stw (r9)
    inc r9
    inc r9
    with r8
    sub #4
    ldw (r8)
    inc r8
    inc r8
    stw (r9)
    inc r9
    inc r9
    ldw (r8)
    inc r8
    inc r8
    stw (r9)
    inc r9
    inc r9
.l9C8C:
    lms r0,(!fx_vertex_count-!gsu_ram)
    dec r0
    sbk
    beq .l9C98
    nop
    iwt r15,#.l9BB0
    nop
.l9C98:
    lms r0,(!fx_vertex_count_save-!gsu_ram)
    sub #3
    bpl .l9CA4
    nop
    iwt r15,#.l9EFB
    nop
.l9CA4:
    add #3
    sms (!fx_vertex_count-!gsu_ram),r0
    lms r8,(!fx_poly_out-!gsu_ram)
    iwt r9,#$0688
    to r1
    ldw (r8)
    inc r8
    inc r8
    to r2
    ldw (r8)
    inc r8
    inc r8
    to r3
    ldw (r8)
    inc r8
    inc r8
    to r6
    ldw (r8)
    with r8
    sub #6
    lms r0,(!fx_vertex_count-!gsu_ram)
    mult #8
    add r8
    from r1
    stw (r0)
    inc r0
    inc r0
    from r2
    stw (r0)
    inc r0
    inc r0
    from r3
    stw (r0)
    inc r0
    inc r0
    from r6
    stw (r0)
.l9CD4:
    to r1
    ldw (r8)
    inc r8
    inc r8
    to r2
    ldw (r8)
    inc r8
    inc r8
    with r8
    add #4
    to r3
    ldw (r8)
    inc r8
    inc r8
    to r6
    ldw (r8)
    with r8
    sub #2
    lms r0,(!fx_screen_top-!gsu_ram)
    from r2
    sub r0
    blt .l9CF4
    nop
    iwt r15,#.l9D09
    nop
.l9CF4:
    lms r0,(!fx_screen_top-!gsu_ram)
    from r6
    sub r0
    blt .l9D00
    nop
    iwt r15,#.l9D31
    nop
.l9D00:
    lms r0,(!fx_vertex_count_save-!gsu_ram)
    dec r0
    sbk
    iwt r15,#.l9DB1
    nop
.l9D09:
    lms r0,(!fx_screen_top-!gsu_ram)
    from r6
    sub r0
    blt .l9D15
    nop
    iwt r15,#.l9D9A
    nop
.l9D15:
    lms r0,(!fx_vertex_count_save-!gsu_ram)
    inc r0
    sbk
    from r1
    stw (r9)
    inc r9
    inc r9
    from r2
    stw (r9)
    inc r9
    inc r9
    with r8
    sub #4
    ldw (r8)
    inc r8
    inc r8
    stw (r9)
    inc r9
    inc r9
    ldw (r8)
    inc r8
    inc r8
    stw (r9)
    inc r9
    inc r9
.l9D31:
    with r3
    sub r1
    with r6
    sub r2
    move r0,r3
    move r3,r6
    move r6,r0
    lms r0,(!fx_screen_top-!gsu_ram)
    sub r2
    sms (!fx_model_96-!gsu_ram),r0
    to r5
    lmult
    move r6,r3
    link #4
    iwt r15,#FxDivideSigned
    nop
    from r4
    add r1
    lms r6,(!fx_screen_top-!gsu_ram)
    stw (r9)
    inc r9
    inc r9
    from r6
    stw (r9)
    inc r9
    inc r9
    with r8
    add #4
    to r6
    ldw (r8)
    with r8
    sub #8
    to r2
    ldw (r8)
    inc r8
    inc r8
    with r6
    sub r2
    lms r0,(!fx_model_96-!gsu_ram)
    to r5
    lmult
    move r6,r3
    link #4
    iwt r15,#FxDivideSigned
    nop
    from r4
    add r2
    stw (r9)
    inc r9
    inc r9
    to r2
    ldw (r8)
    with r8
    add #8
    to r6
    ldw (r8)
    with r8
    sub #6
    with r6
    sub r2
    lms r0,(!fx_model_96-!gsu_ram)
    to r5
    lmult
    move r6,r3
    link #4
    iwt r15,#FxDivideSigned
    nop
    from r4
    add r2
    stw (r9)
    inc r9
    inc r9
    iwt r15,#.l9DB1
    nop
.l9D9A:
    from r1
    stw (r9)
    inc r9
    inc r9
    from r2
    stw (r9)
    inc r9
    inc r9
    with r8
    sub #4
    ldw (r8)
    inc r8
    inc r8
    stw (r9)
    inc r9
    inc r9
    ldw (r8)
    inc r8
    inc r8
    stw (r9)
    inc r9
    inc r9
.l9DB1:
    lms r0,(!fx_vertex_count-!gsu_ram)
    dec r0
    sbk
    beq .l9DBD
    nop
    iwt r15,#.l9CD4
    nop
.l9DBD:
    lms r0,(!fx_vertex_count_save-!gsu_ram)
    sub #3
    bpl .l9DC9
    nop
    iwt r15,#.l9EFB
    nop
.l9DC9:
    add #3
    sms (!fx_vertex_count-!gsu_ram),r0
    iwt r8,#$0688
    lms r9,(!fx_poly_out-!gsu_ram)
    to r1
    ldw (r8)
    inc r8
    inc r8
    to r2
    ldw (r8)
    inc r8
    inc r8
    to r3
    ldw (r8)
    inc r8
    inc r8
    to r6
    ldw (r8)
    with r8
    sub #6
    lms r0,(!fx_vertex_count-!gsu_ram)
    mult #8
    add r8
    from r1
    stw (r0)
    inc r0
    inc r0
    from r2
    stw (r0)
    inc r0
    inc r0
    from r3
    stw (r0)
    inc r0
    inc r0
    from r6
    stw (r0)
.l9DF9:
    to r1
    ldw (r8)
    inc r8
    inc r8
    to r2
    ldw (r8)
    inc r8
    inc r8
    with r8
    add #4
    to r3
    ldw (r8)
    inc r8
    inc r8
    to r6
    ldw (r8)
    with r8
    sub #2
    lms r0,(!fx_screen_bottom-!gsu_ram)
    from r2
    sub r0
    sub #1
    bge .l9E1B
    nop
    iwt r15,#.l9E32
    nop
.l9E1B:
    lms r0,(!fx_screen_bottom-!gsu_ram)
    from r6
    sub r0
    sub #1
    bge .l9E29
    nop
    iwt r15,#.l9E5C
    nop
.l9E29:
    lms r0,(!fx_vertex_count_save-!gsu_ram)
    dec r0
    sbk
    iwt r15,#.l9EDC
    nop
.l9E32:
    lms r0,(!fx_screen_bottom-!gsu_ram)
    from r6
    sub r0
    sub #1
    bge .l9E40
    nop
    iwt r15,#.l9EC5
    nop
.l9E40:
    lms r0,(!fx_vertex_count_save-!gsu_ram)
    inc r0
    sbk
    from r1
    stw (r9)
    inc r9
    inc r9
    from r2
    stw (r9)
    inc r9
    inc r9
    with r8
    sub #4
    ldw (r8)
    inc r8
    inc r8
    stw (r9)
    inc r9
    inc r9
    ldw (r8)
    inc r8
    inc r8
    stw (r9)
    inc r9
    inc r9
.l9E5C:
    with r3
    sub r1
    with r6
    sub r2
    move r0,r3
    move r3,r6
    move r6,r0
    lms r0,(!fx_screen_bottom-!gsu_ram)
    sub r2
    sms (!fx_model_96-!gsu_ram),r0
    to r5
    lmult
    move r6,r3
    link #4
    iwt r15,#FxDivideSigned
    nop
    from r4
    add r1
    lms r6,(!fx_screen_bottom-!gsu_ram)
    stw (r9)
    inc r9
    inc r9
    from r6
    stw (r9)
    inc r9
    inc r9
    with r8
    add #4
    to r6
    ldw (r8)
    with r8
    sub #8
    to r2
    ldw (r8)
    inc r8
    inc r8
    with r6
    sub r2
    lms r0,(!fx_model_96-!gsu_ram)
    to r5
    lmult
    move r6,r3
    link #4
    iwt r15,#FxDivideSigned
    nop
    from r4
    add r2
    stw (r9)
    inc r9
    inc r9
    to r2
    ldw (r8)
    with r8
    add #8
    to r6
    ldw (r8)
    with r8
    sub #6
    with r6
    sub r2
    lms r0,(!fx_model_96-!gsu_ram)
    to r5
    lmult
    move r6,r3
    link #4
    iwt r15,#FxDivideSigned
    nop
    from r4
    add r2
    stw (r9)
    inc r9
    inc r9
    iwt r15,#.l9EDC
    nop
.l9EC5:
    from r1
    stw (r9)
    inc r9
    inc r9
    from r2
    stw (r9)
    inc r9
    inc r9
    with r8
    sub #4
    ldw (r8)
    inc r8
    inc r8
    stw (r9)
    inc r9
    inc r9
    ldw (r8)
    inc r8
    inc r8
    stw (r9)
    inc r9
    inc r9
.l9EDC:
    lms r0,(!fx_vertex_count-!gsu_ram)
    dec r0
    sbk
    beq .l9EE8
    nop
    iwt r15,#.l9DF9
    nop
.l9EE8:
    lms r0,(!fx_vertex_count_save-!gsu_ram)
    sub #3
    bpl .l9EF4
    nop
    iwt r15,#.l9EFB
    nop
.l9EF4:
    add #3
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop
.l9EFB:
    sub r0
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop
;
; @routine 01:AB11 FxReturnR11
; WHAT IT DOES: A routine of one JMP R11: returns at once (not reached
; in the traces so far).
; HOW: JMP R11; NOP.
; Entry: R11 = return address.
; Exit: as the code leaves the registers, flags and RAM; R14 and the
; ROM buffer move as on the Super FX.
%at($01AB11)
FxReturnR11:
    jmp r11
    nop

