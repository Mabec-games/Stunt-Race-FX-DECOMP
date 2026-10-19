; frame.asm: the Super FX's frame program (01:AB13), the object walk
; (01:AB81), the object sprite (01:B0D9), the effects' spawn (01:B6D5)
; and the weather (01:BC7D). C: src/stuntracefx/render/frame.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"

arch superfx

;
; @routine 01:AB13 FxFrameProgram
; WHAT IT DOES: A program the S-CPU starts once a frame: clears the
; screen (FxScreenClear, unless the byte at $01C4 is 0), draws the
; objects of the sorted draw list (FxObjectsDraw), the weather
; (FxWeatherDraw, by bits 0 and 1 of the word at +$8A of the record
; $247A points to; none clears $2B94), the effects (FxEffectsRun) and
; the text (FxTextNotice), then STOP.
; HOW: R10 = $0240 before each part; $01B4 = 0; CMODE 0; $26EE = $10
; for the first weather; RPIX before the STOP writes the pixel caches
; out.
; Entry: started by the S-CPU.
; Exit: (at the STOP) as the last part leaves the registers, flags and
; RAM; R14 and the ROM buffer move as on the Super FX.
%at($01AB13)
FxFrameProgram:
    iwt r10,#$0240
    lms r0,(!gsu_word_1c4-!gsu_ram)
    lob
    beq .lAB22
    nop
    link #4
    iwt r15,#FxScreenClear
    nop
.lAB22:
    iwt r10,#$0240
    sub r0
    sms (!fx_model_1b4-!gsu_ram),r0
    link #4
    iwt r15,#FxObjectsDraw
    nop
    iwt r10,#$0240
    sub r0
    cmode
    lm r2,(!gsu_tied_22-!gsu_ram)
    moves r2,r2
    beq .lAB69
    nop
    iwt r1,#$008A
    with r1
    add r2
    to r1
    ldw (r1)
    iwt r0,#$0001
    and r1
    beq .lAB56
    nop
    ibt r0,#$10
    sm (!fx_effect_max-!gsu_ram),r0
    ibt r0,#$01
    bra .lAB5F
    nop
.lAB56:
    iwt r0,#$0002
    and r1
    beq .lAB69
    nop
    ibt r0,#$02
.lAB5F:
    ibt r1,#$50
    link #4
    iwt r15,#FxWeatherDraw
    nop
    bra .lAB6E
    nop
.lAB69:
    sub r0
    sm (!fx_init_2b94-!gsu_ram),r0
.lAB6E:
    link #4
    iwt r15,#FxEffectsRun
    nop
    sub r0
    sm (!fx_effect_max-!gsu_ram),r0
    link #4
    iwt r15,#FxTextNotice
    nop
    rpix
    stop
    nop
;
; @routine 01:AB81 FxObjectsDraw
; WHAT IT DOES: Draws every object in the draw list from $023C: its
; angles, flags and shape from the slot; a slot whose flags have bit 2
; adds a burst of effect particles (FxEffectsAdd) and draws them
; (FxParticlesDraw); else the shape's header (in ROM bank $14) picks
; the model by distance (+$16, +$18, +$1A at depths $3E8, $7D0, $BB8)
; and the object is drawn as a sprite (FxObjectSprite) or a model
; (FxModelDraw).
; HOW: Pushes R11 on the R10 stack and each slot's address around its
; drawing; the next slot from its link word (+$00) until 0; returns by
; pulling R15.
; Entry: R10 = stack; R11 = return address.
; Exit: as the last object leaves the registers, flags and RAM; R14 and
; the ROM buffer move as on the Super FX.
%at($01AB81)
FxObjectsDraw:
    lm r1,(!gsu_draw_list-!gsu_ram)
    moves r1,r1
    bne .lAB8C
    nop
    jmp r11
    nop
.lAB8C:
    from r11
    stw (r10)
    inc r10
    inc r10
.lAB90:
    sub r0
    romb
    sub r0
    cmode
    sms (!fx_points_total-!gsu_ram),r0
    from r1
    stw (r10)
    inc r10
    inc r10
    inc r1
    inc r1
    inc r1
    inc r1
    ldb (r1)
    sms (!fx_object_angle_x-!gsu_ram),r0
    inc r1
    ldb (r1)
    sms (!fx_car_0022-!gsu_ram),r0
    inc r1
    ldb (r1)
    sms (!fx_object_angle_z-!gsu_ram),r0
    inc r1
    to r4
    ldb (r1)
    inc r1
    to r3
    ldw (r1)
    moves r3,r3
    bne .lABC2
    nop
    iwt r15,#.lAD53
    nop
.lABC2:
    inc r1
    inc r1
    inc r1
    inc r1
    inc r1
    inc r1
    inc r1
    inc r1
    ldw (r1)
    sms (!fx_object_y-!gsu_ram),r0
    inc r1
    inc r1
    ldw (r1)
    sms (!fx_object_x-!gsu_ram),r0
    inc r1
    inc r1
    ldw (r1)
    sms (!fx_object_z-!gsu_ram),r0
    move r6,r0
    inc r1
    inc r1
    iwt r5,#$0004
    from r4
    and r5
    beq .lAC31
    nop
    ibt r0,#$F4
    to r1
    add r1
    ldw (r1)
    sms (!fx_model_148-!gsu_ram),r0
    inc r1
    inc r1
    ldb (r1)
    lob
    sms (!fx_model_14e-!gsu_ram),r0
    inc r1
    ldb (r1)
    lob
    sms (!fx_model_150-!gsu_ram),r0
    inc r1
    ldb (r1)
    lob
    ibt r5,#$07
    and r5
    bne .lAC09
    nop
    ibt r0,#$FF
.lAC09:
    sms (!fx_points_total-!gsu_ram),r0
    lms r0,(!fx_points_total-!gsu_ram)
    moves r0,r0
    beq .lAC2D
    nop
    bmi .lAC25
    nop
    lms r1,(!fx_model_150-!gsu_ram)
    lms r2,(!fx_model_148-!gsu_ram)
    lms r3,(!fx_model_14e-!gsu_ram)
    link #4
    iwt r15,#FxEffectsAdd
    nop
.lAC25:
    lms r2,(!fx_model_148-!gsu_ram)
    link #4
    iwt r15,#FxParticlesDraw
    nop
.lAC2D:
    iwt r15,#.lAD53
    nop
.lAC31:
    ibt r5,#$02
    from r4
    and r5
    beq .lAC48
    nop
    ibt r5,#$08
    from r4
    and r5
    bne .lAC48
    nop
    iwt r5,#$8D30
    sms (!fx_material_table-!gsu_ram),r5
    bra .lAC6D
    nop
.lAC48:
    ldw (r1)
    moves r0,r0
    bne .lAC6A
    nop
    ibt r14,#$12
    from r3
    to r14
    add r14
    getb
    inc r14
    getbh
    iwt r5,#$8F91
    cmp r5
    bne .lAC6A
    nop
    lm r5,(!gsu_param_108a-!gsu_ram)
    moves r5,r5
    beq .lAC6A
    nop
    move r0,r5
.lAC6A:
    sms (!fx_material_table-!gsu_ram),r0
.lAC6D:
    inc r1
    inc r1
    ibt r14,#$07
    from r3
    to r14
    add r14
    getb
    sms (!fx_point_shift-!gsu_ram),r0
    from r3
    stw (r10)
    inc r10
    inc r10
    move r0,r6
    iwt r5,#$0BB8
    cmp r5
    bcs .lAC99
    nop
    iwt r5,#$07D0
    cmp r5
    bcs .lACA7
    nop
    iwt r5,#$03E8
    cmp r5
    bcs .lACB5
    nop
    bra .lACC0
    nop
.lAC99:
    ibt r14,#$1A
    from r3
    to r14
    add r14
    to r3
    getb
    inc r14
    with r3
    getbh
    bra .lACC0
    nop
.lACA7:
    ibt r14,#$18
    from r3
    to r14
    add r14
    to r3
    getb
    inc r14
    with r3
    getbh
    bra .lACC0
    nop
.lACB5:
    ibt r14,#$16
    from r3
    to r14
    add r14
    to r3
    getb
    inc r14
    with r3
    getbh
.lACC0:
    ibt r14,#$00
    from r3
    to r14
    add r14
    getb
    inc r14
    getbh
    sms (!fx_model_at-!gsu_ram),r0
    ibt r14,#$02
    from r3
    to r14
    add r14
    getb
    sms (!fx_rom_bank-!gsu_ram),r0
    ibt r14,#$03
    from r3
    to r14
    add r14
    getb
    inc r14
    getbh
    sms (!fx_model_18-!gsu_ram),r0
    dec r10
    dec r10
    to r3
    ldw (r10)
    ldb (r1)
    inc r1
    sms (!fx_object_kind-!gsu_ram),r0
    ldb (r1)
    sms (!fx_model_frame-!gsu_ram),r0
    inc r1
    ldb (r1)
    sms (!fx_material_mask-!gsu_ram),r0
    inc r1
    ibt r5,#$08
    from r4
    and r5
    beq .lAD31
    nop
    lms r0,(!fx_model_1b4-!gsu_ram)
    lob
    bne .lAD31
    nop
    sub r0
    romb
    ibt r14,#$10
    from r3
    to r14
    add r14
    getb
    inc r14
    getbh
    sms (!fx_sprite_size-!gsu_ram),r0
    ldb (r1)
    sms (!fx_sprite_material-!gsu_ram),r0
    inc r1
    ldb (r1)
    sex
    sms (!fx_model_ce-!gsu_ram),r0
    lms r0,(!fx_model_at-!gsu_ram)
    moves r0,r0
    beq .lAD2D
    nop
    link #4
    iwt r15,#FxObjectSprite
    nop
.lAD2D:
    iwt r15,#.lAD53
    nop
.lAD31:
    ldb (r1)
    sms (!fx_model_1b6-!gsu_ram),r0
    inc r1
    sub r0
    ldb (r1)
    swap
    sms (!fx_tex_u_off-!gsu_ram),r0
    inc r1
    ldb (r1)
    swap
    sms (!fx_tex_v_off-!gsu_ram),r0
    inc r1
    lms r0,(!fx_model_18-!gsu_ram)
    moves r0,r0
    beq .lAD53
    nop
    link #4
    iwt r15,#FxModelDraw
    nop
.lAD53:
    dec r10
    dec r10
    to r1
    ldw (r10)
    to r1
    ldw (r1)
    moves r1,r1
    beq .lAD62
    nop
    iwt r15,#.lAB90
    nop
.lAD62:
    nop
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop
;
; @routine 01:B0D9 FxObjectSprite
; WHAT IT DOES: Draws an object as a sprite scaled by its depth: its
; screen position (FxProjectScaled), its colour from its material
; (FxSpriteMaterial), and its rows of texels plotted, clipped to the
; screen; nothing when its depth $002A is at most $0080.
; HOW: Pushes R11 on the R10 stack; the size from $00CE and $00AA
; shifted by $0032; the drawing goes on in FxObjectSpriteRows
; (01:B2ED); returns by pulling R15.
; Entry: R10 = stack; R11 = return address; the object in $0020-$002A,
; $00AA, $00CE, $00D6.
; Exit: as the code leaves the registers, flags and RAM; R14 and the
; ROM buffer move as on the Super FX.
%at($01B0D9)
FxObjectSprite:
    from r11
    stw (r10)
    inc r10
    inc r10
    lms r3,(!fx_object_z-!gsu_ram)
    iwt r0,#$0080
    sub r3
    bpl .lB123
    nop
    lms r0,(!fx_model_ce-!gsu_ram)
    moves r1,r0
    bmi .lB128
    nop
    cache
    lms r12,(!fx_point_shift-!gsu_ram)
    moves r12,r12
    beq .lB100
    nop
    move r0,r1
    move r13,r15
    loop
    add r0
    move r1,r0
.lB100:
    lms r0,(!fx_sprite_size-!gsu_ram)
    add r0
    to r1
    add r1
    bne .lB10B
    nop
    ibt r1,#$01
.lB10B:
    move r5,r1
    lms r1,(!fx_object_x-!gsu_ram)
    lms r2,(!fx_object_y-!gsu_ram)
    link #4
    iwt r15,#FxProjectScaled
    nop
    sms (!fx_model_ba-!gsu_ram),r1
    sms (!fx_model_bc-!gsu_ram),r2
    moves r1,r5
    bne .lB14C
    nop
.lB123:
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop
.lB128:
    from r1
    stw (r10)
    inc r10
    inc r10
    lms r1,(!fx_object_x-!gsu_ram)
    lms r2,(!fx_object_y-!gsu_ram)
    link #4
    iwt r15,#FxProjectScaled
    nop
    sms (!fx_model_ba-!gsu_ram),r1
    sms (!fx_model_bc-!gsu_ram),r2
    dec r10
    dec r10
    to r1
    ldw (r10)
    with r1
    not
    inc r1
    bne .lB14C
    nop
;
; @routine 01:B147 FxObjectSpritePull
; WHAT IT DOES: FxObjectSprite's return when the size it pulled back at
; 01:B13F negates to 0. The game never runs it: that size was pushed at
; 01:B128 only when it is negative ($00CE), so its negation is never 0
; and the BNE at 01:B144 always branches. A unit start runs it so the C
; is judged against the original.
; HOW: DEC R10 twice, then R15 = the word there (NOP in the delay slot).
; Entry: R10 = stack, the return address the word below it. Exit: R10
; two lower; S, Z from R10.
%at($01B147)
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop
.lB14C:
    iwt r0,#$00F0
    sub r1
    bpl .lB155
    add r1
    move r1,r0
.lB155:
    sms (!fx_model_ce-!gsu_ram),r1
    lms r0,(!fx_sprite_material-!gsu_ram)
    link #4
    iwt r15,#FxSpriteMaterial
    nop
    lms r0,(!fx_face_kind-!gsu_ram)
    moves r0,r0
    beq .lB123
    nop
    lms r1,(!fx_model_ce-!gsu_ram)
    lms r0,(!fx_tex_mask-!gsu_ram)
    lob
    inc r0
    sms (!fx_model_c8-!gsu_ram),r0
    lms r0,(!fx_face_colour-!gsu_ram)
    color
    swap
    cmode
    bra FxObjectSpriteAt_lB182
    nop
;
; @routine 01:B17D FxObjectSpriteAt
; WHAT IT DOES: A second way into FxObjectSprite's drawing (the word
; $B17D is at 01:B0D2, in code no script has run): pushes R11 on the
; R10 stack and goes on as FxObjectSprite does once its colour is set
; (not reached in the traces so far).
; HOW: FROM R11; STW (R10); INC R10 twice; then 01:B182.
; Entry: R10 = stack; R11 = return address; as FxObjectSprite has them
; at 01:B182.
; Exit: as FxObjectSprite.
%at($01B17D)
FxObjectSpriteAt:
    from r11
    stw (r10)
    inc r10
    inc r10
    cache
.lB182:
    from r1
    to r3
    not
    inc r3
    sms (!fx_sprite_clip_x-!gsu_ram),r3
    lms r3,(!fx_screen_right-!gsu_ram)
    with r3
    sub r1
    sms (!fx_sprite_clip_w-!gsu_ram),r3
    ibt r3,#$FF
    from r1
    lsr
    sbc r3
    lms r3,(!fx_model_ba-!gsu_ram)
    from r3
    sub r0
    sbk
    lms r3,(!fx_sprite_clip_x-!gsu_ram)
    inc r3
    cmp r3
    bpl .lB1AB
    nop
.lB1A6:
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop
.lB1AB:
    lms r3,(!fx_screen_right-!gsu_ram)
    cmp r3
    bpl .lB1A6
    nop
    ibt r3,#$FF
    from r1
    lsr
    sbc r3
    lms r3,(!fx_model_bc-!gsu_ram)
    from r3
    sub r0
    sbk
    lms r6,(!fx_model_c8-!gsu_ram)
    from r1
    sub r6
    bcc .lB1CB
    nop
    iwt r15,#FxObjectSpriteRows
    nop
.lB1CB:
    move r4,r6
    ibt r0,#$00
    romb
    iwt r0,#$94A0
    add r1
    to r14
    add r1
    to r6
    getb
    inc r14
    with r6
    getbh
    from r4
    swap
    fmult
    to r9
    rol
    cache
    lms r2,(!fx_model_bc-!gsu_ram)
    ibt r7,#$00
    move r8,r7
    lms r4,(!fx_model_ce-!gsu_ram)
    move r6,r4
    lms r0,(!fx_tex_bank-!gsu_ram)
    romb
    lms r5,(!fx_tex_base-!gsu_ram)
    lms r3,(!fx_model_ba-!gsu_ram)
    lms r0,(!fx_tex_mask-!gsu_ram)
    moves r0,r0
    bpl .lB206
    nop
    iwt r15,#.lB251
    nop
.lB206:
    moves r3,r3
    bpl .lB220
    nop
    move r12,r6
    move r13,r15
    bpl .lB21B
    nop
    with r8
    add r9
    loop
    inc r3
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop
.lB21B:
    move r6,r12
    merge
    to r5
    add r5
.lB220:
    lms r0,(!fx_sprite_clip_w-!gsu_ram)
    sub r3
    bpl .lB22A
    nop
    with r6
    add r0
    inc r6
.lB22A:
    iwt r13,#$B23E
.lB22D:
    lms r0,(!fx_screen_bottom-!gsu_ram)
    dec r0
    sub r2
    bcc .lB246
    nop
    ibt r8,#$00
    merge
    to r14
    add r5
    move r1,r3
    move r12,r6
    with r8
    add r9
    merge
    getc
    to r14
    add r5
    loop
    plot
.lB246:
    with r7
    add r9
    dec r4
    bne .lB22D
    inc r2
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop
.lB251:
    lms r0,(!fx_model_c8-!gsu_ram)
    with r5
    add r0
    dec r5
    moves r3,r3
    bpl .lB271
    nop
    move r12,r6
    move r13,r15
    bpl .lB26C
    nop
    with r8
    sub r9
    loop
    inc r3
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop
.lB26C:
    move r6,r12
    merge
    to r5
    add r5
.lB271:
    move r1,r3
    lms r0,(!fx_sprite_clip_w-!gsu_ram)
    sub r1
    bpl .lB27D
    nop
    with r6
    add r0
    inc r6
.lB27D:
    iwt r13,#$B291
.lB280:
    lms r0,(!fx_screen_bottom-!gsu_ram)
    dec r0
    sub r2
    bcc .lB299
    nop
    ibt r8,#$00
    merge
    to r14
    add r5
    move r1,r3
    move r12,r6
    with r8
    sub r9
    merge
    getc
    to r14
    add r5
    loop
    plot
.lB299:
    with r7
    add r9
    dec r4
    bne .lB280
    inc r2
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop
;
; @routine 01:B2ED FxObjectSpriteRows
; WHAT IT DOES: The rows of an object sprite: the texel step from the
; size (FxDivide32By16Halving, or $FFFF when the size is the scale),
; then each row of texels from the sprite's texture (bank $00A6,
; address $00A8, mask $00D4) plotted between the screen's edges,
; columns stepped by R9; mirrored when the mask's bit 15 is set
; (01:B3AB).
; HOW: Reached from FxObjectSprite (01:B1CA jumps here, after
; FxProjectScaled's code ends at 01:B2EC). CACHE; GETC, PLOT in a LOOP
; per row; returns by pulling R15 for FxObjectSprite.
; Entry: as FxObjectSprite leaves them: R1 = the size, R6 = the scale;
; R10 = stack.
; Exit: as the code leaves the registers, flags and RAM; R14 and the
; ROM buffer move as on the Super FX.
%at($01B2ED)
FxObjectSpriteRows:
    move r0,r6
    move r6,r1
    cmp r6
    bne .lB2FC
    nop
    iwt r4,#$FFFF
    bra .lB303
    nop
.lB2FC:
    ibt r4,#$00
    link #4
    iwt r15,#FxDivide32By16Halving
    nop
.lB303:
    move r9,r4
    cache
    lms r6,(!fx_model_c8-!gsu_ram)
    lms r3,(!fx_model_ba-!gsu_ram)
    lms r2,(!fx_model_bc-!gsu_ram)
    ibt r7,#$00
    ibt r11,#$00
    lms r0,(!fx_tex_bank-!gsu_ram)
    romb
    lms r5,(!fx_tex_base-!gsu_ram)
    with r9
    bic #1
    lms r0,(!fx_tex_mask-!gsu_ram)
    moves r0,r0
    bpl .lB32A
    nop
    iwt r15,#.lB3AB
    nop
.lB32A:
    move r13,r6
    lms r0,(!fx_screen_right-!gsu_ram)
    to r6
    sub r3
    from r9
    lsr
    fmult
    to r4
    rol
    from r13
    sub r4
    dec r0
    bmi .lB33E
    nop
    move r13,r4
.lB33E:
    move r6,r13
    moves r3,r3
    bpl .lB35E
    nop
    from r3
    to r6
    not
    inc r6
    from r9
    lsr
    lmult
    from r4
    to r11
    add r4
    rol
    bmi .lB3A6
    nop
    with r5
    add r0
    from r13
    to r6
    sub r0
    bmi .lB3A6
    nop
    ibt r3,#$00
.lB35E:
    moves r6,r6
    beq .lB3A6
    nop
    ibt r4,#$00
.lB365:
    moves r2,r2
    bmi .lB398
    nop
    lms r0,(!fx_screen_bottom-!gsu_ram)
    dec r0
    sub r2
    bmi .lB3A6
    nop
    from r4
    swap
    to r14
    add r5
    move r1,r3
    move r0,r11
    getc
    move r12,r6
    dec r12
    beq .lB38A
    nop
    move r13,r15
    inc r14
.lB384:
    add r9
    bcc .lB384
    plot
    loop
    getc
.lB38A:
    move r8,r0
    lms r0,(!fx_screen_right-!gsu_ram)
.lB38F:
    cmp r1
    bmi .lB398
    with r8
    add r9
    bcc .lB38F
    plot
.lB398:
    nop
    with r7
    sub r9
    bcs .lB365
    inc r2
    inc r4
    lms r0,(!fx_model_c8-!gsu_ram)
    sub r4
    bne .lB365
    nop
.lB3A6:
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop
.lB3AB:
    with r5
    add r6
    dec r5
    move r13,r6
    lms r0,(!fx_screen_right-!gsu_ram)
    to r6
    sub r3
    from r9
    lsr
    fmult
    to r4
    rol
    from r13
    sub r4
    dec r0
    bmi .lB3C2
    nop
    move r13,r4
.lB3C2:
    move r6,r13
    moves r3,r3
    bpl .lB3E2
    nop
    from r3
    to r6
    not
    inc r6
    from r9
    lsr
    lmult
    from r4
    to r11
    add r4
    rol
    bmi .lB42A
    nop
    with r5
    sub r0
    from r13
    to r6
    sub r0
    bmi .lB42A
    nop
    ibt r3,#$00
.lB3E2:
    moves r6,r6
    beq .lB42A
    nop
    ibt r4,#$00
.lB3E9:
    moves r2,r2
    bmi .lB41C
    nop
    lms r0,(!fx_screen_bottom-!gsu_ram)
    dec r0
    sub r2
    bmi .lB42A
    nop
    from r4
    swap
    to r14
    add r5
    move r1,r3
    move r0,r11
    getc
    move r12,r6
    dec r12
    beq .lB40E
    nop
    move r13,r15
    dec r14
.lB408:
    add r9
    bcc .lB408
    plot
    loop
    getc
.lB40E:
    move r8,r0
    lms r0,(!fx_screen_right-!gsu_ram)
.lB413:
    cmp r1
    bmi .lB41C
    with r8
    add r9
    bcc .lB413
    plot
.lB41C:
    nop
    with r7
    sub r9
    bcs .lB3E9
    inc r2
    inc r4
    lms r0,(!fx_model_c8-!gsu_ram)
    sub r4
    bne .lB3E9
    nop
.lB42A:
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop
;
; @routine 01:B6D5 FxEffectsAdd
; WHAT IT DOES: Adds R3 effect particles of kind R0 (1 to 7) at R1 / R2
; to the twenty 14-byte slots at $273E (a slot with byte 0 free), each
; kind's speeds, life and colour from the random seed at $01D0.
; HOW: Pushes R11 on the R10 stack; ROMB $14, RAMB 0; a LOOP of R12 =
; 20 (R13 = $B6F4) over the slots; the seed stepped as SWAP, ROR, ADD,
; ADC; returns by pulling R15.
; Entry: R0 = kind, R1, R2 = place, R3 = count; R10 = stack; R11 =
; return address.
; Exit: R0-R9 as the last slot left them; $01D0 = the seed; R12, R13
; the loop; the rest as the code leaves them.
%at($01B6D5)
FxEffectsAdd:
    from r11
    stw (r10)
    inc r10
    inc r10
    move r8,r0
    move r6,r1
    move r5,r2
    move r9,r3
    ibt r0,#$14
    romb
    sub r0
    ramb
    iwt r3,#$273E
    lms r2,(!fx_effect_seed-!gsu_ram)
    iwt r12,#$0014
    cache
    move r13,r15
    ldb (r3)
    lob
    bne .lB767
    nop
    sub r0
    ibt r7,#$06
    with r7
    add r3
    from r0
    stw (r7)
    ibt r7,#$08
    with r7
    add r3
    from r0
    stw (r7)
    ibt r7,#$0A
    with r7
    add r3
    from r0
    stw (r7)
    ibt r7,#$0C
    with r7
    add r3
    from r5
    stw (r7)
    from r6
    stb (r3)
    ibt r0,#$01
    cmp r8
    bne .lB721
    nop
    iwt r15,#FxEffectsAddKinds_lB99F
    nop
.lB721:
    ibt r0,#$02
    cmp r8
    bne .lB72C
    nop
    iwt r15,#FxEffectsAddKinds_lB8C3
    nop
.lB72C:
    ibt r0,#$03
    cmp r8
    bne .lB737
    nop
    iwt r15,#FxEffectsAddKinds
    nop
.lB737:
    ibt r0,#$04
    cmp r8
    bne .lB742
    nop
    iwt r15,#FxEffectsAddKinds_lB92F
    nop
.lB742:
    ibt r0,#$05
    cmp r8
    bne .lB74D
    nop
    iwt r15,#FxEffectsAddKinds_lB99B
    nop
.lB74D:
    ibt r0,#$06
    cmp r8
    bne .lB758
    nop
    iwt r15,#.lB774
    nop
.lB758:
    ibt r0,#$07
    cmp r8
    bne .lB763
    nop
    iwt r15,#FxEffectsAddKinds_lB9A3
    nop
;
; @routine 01:B763 FxEffectsAddNext
; WHAT IT DOES: FxEffectsAdd's step to the next particle: one fewer to
; add (R9); when none are left, or after the last of the twenty slots,
; the seed is kept at $01D0 and FxEffectsAdd returns.
; HOW: DEC R9; the next slot R3 + 14 by LOOP (R13 = $B6F4); the return
; pulled from the R10 stack. Every kind's set-up jumps here.
; Entry: as FxEffectsAdd leaves them in its slot loop.
; Exit: as FxEffectsAdd.
%at($01B763)
.lB763:
    dec r9
    beq .lB76C
    nop
.lB767:
    with r3
    add #14
    loop
    nop
.lB76C:
    sms (!fx_effect_seed-!gsu_ram),r2
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop
.lB774:
    from r9
    and #15
    from r6
    add r0
    sub #7
    ibt r7,#$00
    with r7
    add r3
    from r0
    stb (r7)
    ibt r0,#$10
    ibt r7,#$01
    with r7
    add r3
    from r0
    stb (r7)
    move r0,r9
    dec r0
    add r0
    iwt r7,#$D0DC
    to r14
    add r7
    getbs
    inc r14
    div2
    ibt r7,#$03
    with r7
    add r3
    from r0
    stb (r7)
    ibt r7,#$06
    with r7
    add r3
    from r0
    stw (r7)
    getbs
    div2
    ibt r7,#$04
    with r7
    add r3
    from r0
    stb (r7)
    ibt r7,#$08
    with r7
    add r3
    from r0
    stw (r7)
    from r2
    swap
    ror
    add r2
    to r2
    adc r2
    iwt r0,#$001F
    and r2
    sub #15
    ibt r7,#$05
    with r7
    add r3
    from r0
    stb (r7)
    ibt r7,#$0A
    with r7
    add r3
    from r0
    stw (r7)
    ibt r4,#$04
    ibt r0,#$01
    and r12
    beq .lB7DB
    nop
    ibt r4,#$0E
.lB7DB:
    ibt r7,#$02
    with r7
    add r3
    from r4
    stb (r7)
    iwt r15,#.lB763
    nop
;
; @routine 01:B7E6 FxEffectsAddUnused
; WHAT IT DOES: Code no jump or branch reaches: after kind 6's set-up
; (which ends by jumping to 01:B763) a second copy of a kind's set-up,
; its speeds halved differently; nothing in bank 01 jumps or branches
; here, so a direct call is its only way in.
; HOW: As kind 6 at 01:B774, ending at 01:B855 with the jump to the
; next slot (01:B763).
; Entry: as FxEffectsAdd at a kind's set-up: R3 = the slot, R9 = the
; particles left, R2 = the seed, R6, R12, R10 = stack. Exit: as
; FxEffectsAdd.
%at($01B7E6)
FxEffectsAddUnused:
    from r9
    and #15
    from r6
    add r0
    sub #7
    ibt r7,#$00
    with r7
    add r3
    from r0
    stb (r7)
    ibt r0,#$10
    ibt r7,#$01
    with r7
    add r3
    from r0
    stb (r7)
    move r0,r9
    dec r0
    add r0
    iwt r7,#$D0DC
    to r14
    add r7
    getbs
    inc r14
    ibt r7,#$03
    with r7
    add r3
    from r0
    stb (r7)
    ibt r7,#$06
    with r7
    add r3
    from r0
    stw (r7)
    getbs
    ibt r7,#$04
    with r7
    add r3
    from r0
    stb (r7)
    ibt r7,#$08
    with r7
    add r3
    from r0
    stw (r7)
    from r2
    swap
    ror
    add r2
    to r2
    adc r2
    iwt r0,#$003F
    and r2
    iwt r7,#$001F
    sub r7
    ibt r7,#$05
    with r7
    add r3
    from r0
    stb (r7)
    ibt r7,#$0A
    with r7
    add r3
    from r0
    stw (r7)
    ibt r4,#$04
    ibt r0,#$01
    and r12
    beq .lB84B
    nop
    ibt r4,#$0E
.lB84B:
    ibt r7,#$02
    with r7
    add r3
    from r4
    stb (r7)
    iwt r15,#FxEffectsAdd_lB763
    nop
;
; @routine 01:B856 FxEffectsAddKinds
; WHAT IT DOES: FxEffectsAdd's set-ups for kinds 3, 4, 5 and 7
; (01:B856, 01:B92F, 01:B99B, 01:B9A3) and kind 1's (01:B99F): each
; particle's colour, life and speeds from the seed, then the next slot
; (01:B763).
; HOW: Reached by jump from FxEffectsAdd's dispatch on the kind; the
; seed stepped as SWAP, ROR, ADD, ADC.
; Entry: as FxEffectsAdd leaves them at the dispatch.
; Exit: as FxEffectsAdd.
%at($01B856)
FxEffectsAddKinds:
    from r2
    swap
    ror
    add r2
    to r2
    adc r2
    ibt r7,#$07
    and r7
    from r6
    sub r0
    stb (r3)
    ibt r0,#$00
    ibt r7,#$01
    with r7
    add r3
    from r0
    stb (r7)
    ibt r4,#$00
    from r2
    swap
    ror
    add r2
    to r2
    adc r2
    ibt r0,#$0F
    and r2
    ibt r7,#$07
    sub r7
    ibt r7,#$03
    with r7
    add r3
    from r0
    stb (r7)
    from r2
    swap
    ror
    add r2
    to r2
    adc r2
    ibt r0,#$0F
    and r2
    ibt r7,#$07
    sub r7
    ibt r7,#$04
    with r7
    add r3
    from r0
    stb (r7)
    from r2
    swap
    ror
    add r2
    to r2
    adc r2
    ibt r0,#$7F
    and r2
    not
    inc r0
    ibt r7,#$05
    with r7
    add r3
    from r0
    stb (r7)
    from r2
    swap
    ror
    add r2
    to r2
    adc r2
    ibt r0,#$03
    and r2
    iwt r7,#$0083
    add r7
    ibt r7,#$02
    with r7
    add r3
    from r0
    stb (r7)
    iwt r15,#FxEffectsAdd_lB763
    nop
.lB8C3:
    from r2
    swap
    ror
    add r2
    to r2
    adc r2
    ibt r7,#$07
    and r7
    from r6
    sub r0
    stb (r3)
    ibt r0,#$02
    ibt r7,#$01
    with r7
    add r3
    from r0
    stb (r7)
    ibt r4,#$00
    from r2
    swap
    ror
    add r2
    to r2
    adc r2
    ibt r0,#$0F
    and r2
    ibt r7,#$07
    sub r7
    ibt r7,#$03
    with r7
    add r3
    from r0
    stb (r7)
    from r2
    swap
    ror
    add r2
    to r2
    adc r2
    ibt r0,#$07
    and r2
    ibt r7,#$03
    sub r7
    ibt r7,#$04
    with r7
    add r3
    from r0
    stb (r7)
    from r2
    swap
    ror
    add r2
    to r2
    adc r2
    ibt r0,#$1F
    and r2
    not
    inc r0
    ibt r7,#$05
    with r7
    add r3
    from r0
    stb (r7)
    from r2
    swap
    ror
    add r2
    to r2
    adc r2
    ibt r0,#$03
    and r2
    ibt r7,#$68
    add r7
    ibt r7,#$02
    with r7
    add r3
    from r0
    stb (r7)
    iwt r15,#FxEffectsAdd_lB763
    nop
.lB92F:
    from r2
    swap
    ror
    add r2
    to r2
    adc r2
    ibt r7,#$07
    and r7
    from r6
    sub r0
    stb (r3)
    ibt r0,#$04
    ibt r7,#$01
    with r7
    add r3
    from r0
    stb (r7)
    ibt r4,#$00
    from r2
    swap
    ror
    add r2
    to r2
    adc r2
    ibt r0,#$0F
    and r2
    ibt r7,#$07
    sub r7
    ibt r7,#$03
    with r7
    add r3
    from r0
    stb (r7)
    from r2
    swap
    ror
    add r2
    to r2
    adc r2
    ibt r0,#$07
    and r2
    ibt r7,#$03
    sub r7
    ibt r7,#$04
    with r7
    add r3
    from r0
    stb (r7)
    from r2
    swap
    ror
    add r2
    to r2
    adc r2
    ibt r0,#$1F
    and r2
    not
    inc r0
    ibt r7,#$05
    with r7
    add r3
    from r0
    stb (r7)
    from r2
    swap
    ror
    add r2
    to r2
    adc r2
    ibt r0,#$03
    and r2
    ibt r7,#$68
    add r7
    ibt r7,#$02
    with r7
    add r3
    from r0
    stb (r7)
    iwt r15,#FxEffectsAdd_lB763
    nop
.lB99B:
    iwt r15,#FxEffectsAdd_lB763
    nop
.lB99F:
    iwt r15,#FxEffectsAdd_lB763
    nop
.lB9A3:
    from r9
    and #15
    from r6
    add r0
    sub #7
    ibt r7,#$00
    with r7
    add r3
    from r0
    stb (r7)
    ibt r0,#$10
    ibt r7,#$01
    with r7
    add r3
    from r0
    stb (r7)
    move r0,r9
    dec r0
    add r0
    iwt r7,#$D0DC
    to r14
    add r7
    getbs
    inc r14
    ibt r7,#$03
    with r7
    add r3
    from r0
    stb (r7)
    ibt r7,#$06
    with r7
    add r3
    from r0
    stw (r7)
    getbs
    ibt r7,#$04
    with r7
    add r3
    from r0
    stb (r7)
    ibt r7,#$08
    with r7
    add r3
    from r0
    stw (r7)
    from r2
    swap
    ror
    add r2
    to r2
    adc r2
    iwt r0,#$003F
    and r2
    iwt r7,#$001F
    sub r7
    ibt r7,#$05
    with r7
    add r3
    from r0
    stb (r7)
    ibt r7,#$0A
    with r7
    add r3
    from r0
    stw (r7)
    ibt r4,#$04
    ibt r0,#$01
    and r12
    beq .lBA08
    nop
    ibt r4,#$0E
.lBA08:
    ibt r7,#$02
    with r7
    add r3
    from r4
    stb (r7)
    iwt r15,#FxEffectsAdd_lB763
    nop
;
; @routine 01:BC35 FxWeatherReset
; WHAT IT DOES: Sets the weather afresh: $2B94 = R0 (the weather now
; drawn), the 80 drops at $2864 (ten bytes each: a height from 0 down
; by one, then nine zeros), $2B8E and $2B90 cleared and the 48 words at
; $2688 cleared.
; HOW: Pushes R11 on the R10 stack; CACHE; two LOOPs of STB and one of
; STW; returns by pulling R15. FxWeatherDraw branches here (01:BC83)
; when R0 is not $2B94.
; Entry: R0 = the weather; R10 = stack; R11 = return address.
; Exit: R0 = 0; R1 past the words at $2688; R2 = 0; R12 = 0; R13 the
; last loop; S and Z from R10; the last RAM address R10.
%at($01BC35)
FxWeatherReset:
    sm (!fx_init_2b94-!gsu_ram),r0
    from r11
    stw (r10)
    inc r10
    inc r10
    sub r0
    iwt r2,#$0050
    iwt r1,#$2864
    ibt r3,#$00
    iwt r4,#$003F
    cache
.lBC4A:
    move r0,r3
    not
    inc r0
    stb (r1)
    inc r1
    inc r3
    with r3
    and r4
    sub r0
    iwt r12,#$0009
    move r13,r15
    stb (r1)
    loop
    inc r1
    dec r2
    bne .lBC4A
    nop
    sub r0
    sm (!fx_weather_turn-!gsu_ram),r0
    sm (!fx_weather_drift-!gsu_ram),r0
    iwt r1,#$2688
    iwt r12,#$0030
    move r13,r15
    stw (r1)
    inc r1
    inc r1
    loop
    nop
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop
;
; @routine 01:BC7D FxWeatherDraw
; WHAT IT DOES: Draws the weather: R0 = 1 (rain) or 2 (snow) with R1
; drops; when R0 is not $2B94 the drops are set afresh instead
; (FxWeatherReset); each drop moved by the view's turn ($00E0) and
; drawn with PLOT.
; HOW: Pushes R11 on the R10 stack; RAMB 0, ROMB $14, CMODE 1; the drop
; list at $2864; the seed at $01D0; returns by pulling R15.
; Entry: R0 = the weather, R1 = the drops; R10 = stack; R11 = return
; address.
; Exit: $01D0 = the seed; the rest as the code leaves them.
%at($01BC7D)
FxWeatherDraw:
    lm r3,(!fx_init_2b94-!gsu_ram)
    cmp r3
    bne FxWeatherReset
    nop
    ibt r3,#$01
    cmp r3
    beq .lBC97
    nop
    ibt r3,#$02
    cmp r3
    beq .lBC97
    nop
    move r15,r11
    nop
.lBC97:
    from r11
    stw (r10)
    inc r10
    inc r10
    lms r7,(!fx_effect_seed-!gsu_ram)
    moves r12,r1
    bne .lBCA7
    nop
    iwt r15,#FxWeatherDropNext_lBF65
    nop
.lBCA7:
    move r11,r0
    sub r0
    ramb
    ibt r0,#$14
    romb
    ibt r0,#$01
    cmode
    from r11
    sub #1
    beq .lBCD2
    nop
    ibt r3,#$1F
    ibt r4,#$0F
    ibt r5,#$07
    ibt r6,#$05
    iwt r0,#$0020
    sm (!fx_weather_colour-!gsu_ram),r0
    ibt r0,#$64
    sm (!fx_weather_life-!gsu_ram),r0
    bra .lBCE7
    nop
.lBCD2:
    ibt r3,#$1F
    ibt r4,#$3F
    ibt r5,#$1F
    ibt r6,#$28
    iwt r0,#$002A
    sm (!fx_weather_colour-!gsu_ram),r0
    ibt r0,#$1E
    sm (!fx_weather_life-!gsu_ram),r0
.lBCE7:
    lm r2,(!fx_weather_drift-!gsu_ram)
    from r7
    swap
    ror
    add r7
    to r7
    adc r7
    to r1
    and r3
    bne .lBCFB
    nop
    hib
    and r4
    to r2
    sub r5
.lBCFB:
    lm r1,(!fx_weather_drift-!gsu_ram)
    from r2
    sub r1
    div2
    div2
    div2
    add r1
    sbk
    lm r0,(!gsu_param_00e0-!gsu_ram)
    lm r1,(!fx_weather_turn-!gsu_ram)
    sm (!fx_weather_turn-!gsu_ram),r0
    sub r1
    div2
    div2
    div2
    div2
    div2
    not
    inc r0
    lm r1,(!fx_weather_drift-!gsu_ram)
    add r1
    sm (!fx_closing-!gsu_ram),r0
    sm (!fx_haze_x8-!gsu_ram),r6
    lm r1,(!gsu_tied_22-!gsu_ram)
    ibt r0,#$4A
    add r1
    ldw (r0)
    moves r0,r0
    bpl .lBD40
    nop
    iwt r15,#FxWeatherDropNext_lBF65
    nop
.lBD40:
    not
    inc r0
    div2
    div2
    div2
    sm (!fx_me_z-!gsu_ram),r0
    iwt r8,#$2864
    cache
    move r13,r15
    ldb (r8)
    sex
    beq .lBD66
    dec r0
    bpl .lBDAA
    nop
    inc r0
    inc r0
    stb (r8)
    with r8
    add #10
    iwt r15,#.lBF2A
    nop
.lBD66:
    lm r0,(!fx_weather_life-!gsu_ram)
    stb (r8)
    inc r8
    from r7
    swap
    ror
    add r7
    to r7
    adc r7
    lm r0,(!fx_weather_colour-!gsu_ram)
    stb (r8)
    inc r8
    iwt r3,#$01FF
    to r3
    and r3
    from r7
    swap
    ror
    add r7
    to r7
    adc r7
    iwt r1,#$03FF
    and r1
    iwt r1,#$01FF
    sub r1
    stw (r8)
    inc r8
    inc r8
    lms r1,(!fx_model_36-!gsu_ram)
    from r3
    div2
    add r1
    not
    inc r0
    stw (r8)
    inc r8
    inc r8
    from r3
    stw (r8)
    inc r8
    inc r8
    sub r0
    stw (r8)
    inc r8
    inc r8
    iwt r15,#.lBF2A
    nop
.lBDAA:
    stb (r8)
    inc r8
    ldb (r8)
    inc r8
    color
    iwt r6,#$01FF
    lm r1,(!fx_closing-!gsu_ram)
    ldw (r8)
    inc r8
    inc r8
    add r1
    bpl .lBDC9
    nop
    add r6
    bpl .lBDDD
    sub r6
    move r0,r6
    dec r0
    bra .lBDD1
    nop
.lBDC9:
    sub r6
    bmi .lBDDD
    add r6
    move r0,r6
    not
    inc r0
.lBDD1:
    sbk
    with r8
    add #4
    sub r0
    stw (r8)
    inc r8
    inc r8
    iwt r15,#.lBF2A
    nop
.lBDDD:
    sbk
    move r9,r0
    iwt r6,#$00FF
    lm r1,(!fx_haze_x8-!gsu_ram)
    ldw (r8)
    inc r8
    inc r8
    add r1
    bpl .lBDF8
    nop
    add r6
    bpl .lBE0C
    sub r6
    move r0,r6
    dec r0
    bra .lBE00
    nop
.lBDF8:
    sub r6
    bmi .lBE0C
    add r6
    move r0,r6
    not
    inc r0
.lBE00:
    sbk
    with r8
    add #2
    sub r0
    stw (r8)
    inc r8
    inc r8
    iwt r15,#.lBF2A
    nop
.lBE0C:
    sbk
    move r5,r0
    iwt r6,#$01FF
    lm r1,(!fx_me_z-!gsu_ram)
    ldw (r8)
    inc r8
    inc r8
    add r1
    bpl .lBE22
    nop
    move r0,r6
    bra .lBE28
    nop
.lBE22:
    cmp r6
    bmi .lBE31
    nop
    sub r0
.lBE28:
    sbk
    sub r0
    stw (r8)
    inc r8
    inc r8
    iwt r15,#.lBF2A
    nop
.lBE31:
    sbk
    move r4,r0
    iwt r6,#$012C
    with r6
    add r4
    with r6
    add r6
    iwt r0,#$896C
    to r14
    add r6
    to r6
    getb
    inc r14
    with r6
    getbh
    with r9
    fmult
    with r9
    rol
    lms r1,(!fx_model_34-!gsu_ram)
    with r1
    add r9
    lms r0,(!fx_screen_left-!gsu_ram)
    cmp r1
    bmi .lBE5B
    nop
    iwt r15,#FxWeatherDropNext
    nop
.lBE5B:
    lms r0,(!fx_screen_right-!gsu_ram)
    sub #2
    cmp r1
    bpl .lBE69
    nop
    iwt r15,#FxWeatherDropNext
    nop
.lBE69:
    with r5
    fmult
    with r5
    rol
    lms r2,(!fx_model_36-!gsu_ram)
    iwt r0,#$0014
    with r2
    sub r0
    with r2
    add r5
    lms r0,(!fx_screen_top-!gsu_ram)
    cmp r2
    bmi .lBE83
    nop
    iwt r15,#FxWeatherDropNext
    nop
.lBE83:
    nop
    lms r0,(!fx_screen_bottom-!gsu_ram)
    sub #4
    cmp r2
    bpl .lBE92
    nop
    iwt r15,#FxWeatherDropNext_lBF53
    nop
.lBE92:
    nop
    from r11
    sub #1
    beq .lBEB2
    nop
    inc r8
    inc r8
    iwt r0,#$0100
    cmp r4
    bpl .lBEA8
    nop
    plot
    iwt r15,#.lBF2A
    nop
.lBEA8:
    plot
    plot
    inc r2
    dec r1
    dec r1
    plot
    plot
    bra .lBF2A
    nop
.lBEB2:
    ldw (r8)
    to r3
    lob
    to r9
    hib
    from r1
    stb (r8)
    inc r8
    from r2
    stb (r8)
    inc r8
    moves r0,r0
    beq .lBF2A
    nop
    from r12
    stw (r10)
    inc r10
    inc r10
    from r13
    stw (r10)
    inc r10
    inc r10
    from r8
    stw (r10)
    inc r10
    inc r10
    from r7
    stw (r10)
    inc r10
    inc r10
    ibt r5,#$01
    with r3
    sub r1
    bpl .lBEE0
    nop
    ibt r5,#$FF
    with r3
    not
    inc r3
.lBEE0:
    ibt r6,#$01
    with r9
    sub r2
    bpl .lBEEC
    nop
    ibt r6,#$FF
    with r9
    not
    inc r9
.lBEEC:
    from r3
    sub r9
    bmi .lBF06
    nop
    move r12,r3
    inc r12
    lsr
    sub r9
    move r13,r15
    bcs .lBEFE
    plot
    add r3
    with r2
    add r6
.lBEFE:
    dec r1
    with r1
    add r5
    loop
    sub r9
    bra .lBF1A
    nop
.lBF06:
    move r12,r9
    inc r12
    not
    inc r0
    lsr
    sub r3
    move r13,r15
    bcs .lBF15
    plot
    add r9
    with r1
    add r5
.lBF15:
    dec r1
    with r2
    add r6
    loop
    sub r3
.lBF1A:
    dec r10
    dec r10
    to r7
    ldw (r10)
    dec r10
    dec r10
    to r8
    ldw (r10)
    dec r10
    dec r10
    to r13
    ldw (r10)
    dec r10
    dec r10
    to r12
    ldw (r10)
.lBF2A:
    loop
    nop
    bra FxWeatherDropNext_lBF65
    nop
;
; @routine 01:BF2F FxWeatherUnused
; WHAT IT DOES: Code no jump or branch reaches: a branch back into
; FxWeatherDraw's drop loop (01:BF2A); nothing in bank 01 jumps or
; branches here, so a direct call is its only way in. The code after
; it is three more such entries: FxWeatherUnusedLoop,
; FxWeatherUnusedClear and FxWeatherUnusedFull.
; HOW: BRA with a NOP in its delay slot; then as FxWeatherDraw.
; Entry: as FxWeatherDraw's drop loop: R12 = the drops left, R13 = the
; loop, R10 = stack. Exit: as FxWeatherDraw.
%at($01BF2F)
FxWeatherUnused:
    bra FxWeatherDraw_lBF2A
    nop
;
; @routine 01:BF32 FxWeatherUnusedLoop
; WHAT IT DOES: The second of FxWeatherUnused's branches back into
; FxWeatherDraw's drop loop (01:BF2A), which nothing reaches; a direct
; call is its only way in.
; HOW: BRA with a NOP in its delay slot; then as FxWeatherDraw.
; Entry: as FxWeatherDraw's drop loop: R12 = the drops left, R13 = the
; loop, R10 = stack. Exit: as FxWeatherDraw.
%at($01BF32)
    bra FxWeatherDraw_lBF2A
    nop
;
; @routine 01:BF35 FxWeatherUnusedClear
; WHAT IT DOES: Sets the word four bytes before R8 (a drop's) to 0 and
; goes on with the drop loop (01:BF2A); nothing reaches it (it follows
; an unconditional BRA), so a direct call is its only way in.
; HOW: IWT R2,#0; BRA to the store at 01:BF3E (R8 - 10 + 6), R8 kept.
; Entry: R8 = the drop; the drop loop's R12, R13, R10. Exit: R1 = R8;
; R2 = 0; then as FxWeatherDraw.
%at($01BF35)
    iwt r2,#$0000
    bra .lBF3E
    nop
;
; @routine 01:BF3B FxWeatherUnusedFull
; WHAT IT DOES: As FxWeatherUnusedClear with $01FF: the word four bytes
; before R8 set to $01FF, then the drop loop (01:BF2A); nothing reaches
; it, so a direct call is its only way in.
; HOW: IWT R2,#$01FF; the store at 01:BF3E.
; Entry: R8 = the drop; the drop loop's R12, R13, R10. Exit: R1 = R8;
; R2 = $01FF; then as FxWeatherDraw.
%at($01BF3B)
    iwt r2,#$01FF
.lBF3E:
    move r1,r8
    with r8
    sub #10
    with r8
    add #6
    from r2
    stw (r8)
    move r8,r1
    bra FxWeatherDraw_lBF2A
    nop
;
; @routine 01:BF4D FxWeatherDropNext
; WHAT IT DOES: The ends of FxWeatherDraw's drop loop: a drop's last
; word cleared (01:BF4D) or its state byte set to 1 (01:BF53), then the
; next drop (01:BF2A); after the last, the seed kept at $01D0 and the
; return (01:BF65).
; HOW: Reached by jump from the drop's moves; returns by pulling R15
; for FxWeatherDraw.
; Entry: as FxWeatherDraw leaves them in its drop loop.
; Exit: as FxWeatherDraw.
%at($01BF4D)
FxWeatherDropNext:
    sub r0
    stw (r8)
    inc r8
    bra FxWeatherDraw_lBF2A
    inc r8
.lBF53:
    move r1,r8
    with r8
    sub #8
    with r8
    add #0
    ibt r0,#$01
    stb (r8)
    move r8,r1
    inc r8
    bra FxWeatherDraw_lBF2A
    inc r8
.lBF65:
    sms (!fx_effect_seed-!gsu_ram),r7
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop

