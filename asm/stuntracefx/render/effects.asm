; effects.asm: the Super FX's short-lived screen effects. Sixteen records of
; six bytes at $2688: +$00 the time left (0: the record is free), +$01 a
; byte, +$02 X and +$04 Y on the screen (words).
; C: src/stuntracefx/render/effects.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"

arch superfx

; @routine 01:C060 FxEffectsAge
; WHAT IT DOES: Counts down the time of each effect in use and frees the
; ones outside the screen less a margin of 8: X below $38 + 8 or not below
; $3A - 8, Y below $3C + 8 or not below $3E - 8.
; HOW: The margins into R8, R7, R4 and R5 (ADD #8, SUB #8). R6 = 8 times
; the word at +$4A of the record at $247A, or 0 when that is negative
; (nothing here uses it). A LOOP over the sixteen records, R13 = its
; start ($C095), R3 stepping by 6: a record whose time (LDB, LOB) is not
; 0 has it counted down (DEC, STB); then R2 = its byte, R9 = $0034
; (unused), R1 = X, R2 = Y, and CMP against each margin; one outside has
; its first word set to 0. WITH then TO is a MOVE, WITH then FROM a
; MOVES. Returns by a MOVE of R11 to R15.
; Entry: R11 = return address. Exit: R3 = $26E8; R12 = 0; R13 = $C095;
; R8, R7, R4, R5 the margins; R6 as above; R0, R1, R2, R9 and the OV and
; CY flags as the last record in use, or the last free record, left them;
; S clear, Z set (LOOP).
%at($01C060)
FxEffectsAge:
    lms r0,(!gsu_in_38-!gsu_ram)
    to r8
    add #8
    lms r0,(!gsu_in_3a-!gsu_ram)
    to r7
    sub #8
    lms r0,(!gsu_in_3c-!gsu_ram)
    to r4
    add #8
    lms r0,(!gsu_in_3e-!gsu_ram)
    to r5
    sub #8
    iwt r6,#$0000
    lm r1,(!gsu_record_247a)
    ibt r0,#$4A
    add r1
    ldw (r0)
    moves r0,r0
    bmi .loop_setup
    nop
    add r0
    add r0
    add r0
    move r6,r0
.loop_setup:
    iwt r3,#$2688
    iwt r12,#$0010
    move r13,r15
    ldb (r3)
    lob
    moves r0,r0
    beq .free
    nop
    dec r0
    stb (r3)
    inc r3
    to r2
    ldb (r3)
    inc r3
    lms r9,(!gsu_in_34-!gsu_ram)
    to r1
    ldw (r3)
    inc r3
    inc r3
    move r0,r2                  ; then LOB of it, not used
    lob
    to r2
    ldw (r3)
    inc r3
    inc r3
    move r0,r1
    cmp r8
    bmi .outside
    nop
    cmp r7
    bpl .outside
    nop
    move r0,r2
    cmp r4
    bmi .outside
    nop
    cmp r5
    bmi .next
    nop
.outside:
    with r3
    sub #6
    sub r0
    stw (r3)
    with r3
    add #6
.next:
    loop
    nop
    bra .done
    nop
.free:
    with r3
    add #6
    loop
    nop
.done:
    move r15,r11
    nop

; @routine 01:BF6D FxEffectsRun
; WHAT IT DOES: With bit 14 of $2736 set, starts new effects (as many as
; the tied object's speed, +$4A of the record at $247A, over 8, at most
; $26EE; none when it is negative), ages them and draws them.
; HOW: Pushes R11 on the R10 stack; three DIV2s; CMP against $26EE keeps
; the smaller. FxEffectsSpawn (when there are any to start),
; FxEffectsAge and FxEffectsDraw. Returns by pulling R15.
; Entry: R10 = stack; R11 = return address. Exit: as FxEffectsDraw, or
; with the bit clear R0 = 0, R1 = $2736, Z set by the AND then S and Z
; from R10; the last RAM address R10.
%at($01BF6D)
FxEffectsRun:
    from r11
    stw (r10)
    inc r10
    inc r10
    lm r1,(!fx_effect_flags)
    iwt r0,#$4000
    and r1
    beq .lBFB2
    nop
    ibt r2,#$00
    lm r1,(!gsu_record_247a)
    ibt r0,#$4A
    add r1
    ldw (r0)
    moves r0,r0
    bmi .lBF9E
    nop
    div2
    div2
    div2
    move r2,r0
    lm r0,(!fx_effect_max)
    cmp r2
    bpl .lBF9E
    nop
    move r2,r0
.lBF9E:
    moves r0,r2
    beq .lBFA8
    nop
    link #4
    iwt r15,#FxEffectsSpawn
    nop
.lBFA8:
    link #4
    iwt r15,#FxEffectsAge
    nop
    link #4
    iwt r15,#FxEffectsDraw
    nop
.lBFB2:
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop

; @routine 01:BFB7 FxHazeSample
; WHAT IT DOES: Reads the pixels of a 16 x 16 shape at (R1, R2), each row
; moved down or up by its own offset, and lists the ones the shape covers
; at $0342 with where each goes back: X - 8 + its column, Y - 8 + its row
; (the pixel word, then those two bytes). $0340 counts them.
; HOW: The shape for the time R0 is found at $08:F92E + 4 * R0: a word
; pointing to sixteen signed row offsets and one pointing to the shape's
; 256 bytes (ROM bank 8). CACHE; sixteen rows (R7, BNE), each with a LOOP
; of sixteen columns (R12, R13 = $C008): a shape byte (GETB, SEX) of 0
; skips the pixel; otherwise its value moves X and RPIX reads the pixel
; at that X and the row's Y. R5 and R6 step the return position (INC in
; the delay slots). The count, (R11 - $0342) / 4 by two DIV2s, is added
; to $0340 by SBK. Returns by pulling R15.
; Entry: R0 = the time; R1 = X, R2 = Y; R10 = stack; R11 = return address.
; Exit: R11 past the list; R7 = R12 = 0; R1 = $0340; R2 its old value; R0
; = the new count; R3 = X, R4 = the last row's Y, R5, R6 past the last
; return position, R8 past the row offsets, R9 = Y; R14 past the shape; the
; ROM bank 8; S and Z from R10; the last RAM address R10. R14 and the ROM
; buffer move as on the Super FX.
%at($01BFB7)
FxHazeSample:
    from r11
    stw (r10)
    inc r10
    inc r10
    sm (!fx_haze_x),r1
    with r1
    sub #8
    sm (!fx_haze_x8),r1
    move r9,r2
    move r6,r2
    with r6
    sub #8
    add r0
    to r1
    add r0
    ibt r0,#$08
    romb
    iwt r14,#$F92E
    with r14
    add r1
    to r1
    getb
    inc r14
    with r1
    getbh
    inc r14
    to r8
    getb
    inc r14
    with r8
    getbh
    inc r14
    move r14,r1
    iwt r7,#$0010
    iwt r11,#$0342
    cache
.lBFF0:
    move r1,r14
    move r14,r8
    getbs
    inc r8
    to r4
    add r9
    move r14,r1
    lm r3,(!fx_haze_x)
    lm r5,(!fx_haze_x8)
    iwt r12,#$0010
    move r13,r15
    getb
    sex
    beq .lC01E
    inc r14
    to r1
    add r3
    move r2,r4
    rpix
    stw (r11)
    inc r11
    inc r11
    from r5
    stb (r11)
    inc r11
    from r6
    stb (r11)
    inc r11
.lC01E:
    loop
    inc r5
    dec r7
    bne .lBFF0
    inc r6
    iwt r1,#$0342
    from r11
    sub r1
    div2
    div2
    iwt r1,#$0340
    to r2
    ldw (r1)
    add r2
    sbk
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop

; @routine 01:C039 FxHazePlot
; WHAT IT DOES: Plots back the pixels FxHazeSample listed, each at its
; return position, so the shape's patch of screen is drawn again moved:
; a shimmer.
; HOW: The count from $0340; none returns at once. CMODE 1 (colour 0 is
; plotted); CACHE; a LOOP (R13 = $C04B) of COLOR from the pixel word, R1
; and R2 from the two bytes (LDB) and PLOT in the delay slot. Returns
; with MOVE R15,R11.
; Entry: R11 = return address. Exit: R3 past the list; R1 one past the
; last X (PLOT steps it), R2 the last Y; R12 = 0; R13 = $C04B; R0 the
; last pixel word; the plot options 1; flags from the LOOP; the last RAM
; address the last Y's.
%at($01C039)
FxHazePlot:
    iwt r3,#$0340
    ldw (r3)
    inc r3
    inc r3
    moves r12,r0
    beq .lC05D
    nop
    ibt r0,#$01
    cmode
    cache
    move r13,r15
    ldw (r3)
    inc r3
    inc r3
    color
    to r1
    sub r0
    move r2,r1
    to r1
    ldb (r3)
    inc r3
    to r2
    ldb (r3)
    inc r3
    loop
    plot
.lC05D:
    move r15,r11
    nop

; @routine 01:C130 FxEffectsSpawn
; WHAT IT DOES: Starts an effect in each free one of the first R0 effect
; records: time 8, its byte 1 or 2 at random, and a random X and Y (the
; low and high bytes of a random word).
; HOW: The seed from $01D0 in R4; a LOOP of R0 records from $2688 (R13 =
; $C13A), each taken by LDB, LOB; a record in use is skipped with ADD #6.
; Each random number is one random step (SWAP, ROR, ADD, ADC). The seed
; goes back to $01D0; returns with MOVE R15,R11.
; Entry: R0 = the number of records to look at; R11 = return address.
; Exit: R4 = the seed; R1 past the last record looked at; R12 = 0; R13 =
; $C13A; R0, R2, R3 as the last record left them; S, Z from the LOOP
; (Z set), OV and CY from the last ADD or ADC; the last RAM address $01D0.
%at($01C130)
FxEffectsSpawn:
    lms r4,(!fx_effect_seed-!gsu_ram)
    iwt r1,#$2688
    move r12,r0
    move r13,r15
    ldb (r1)
    lob
    bne .lC16A
    nop
    ibt r0,#$08
    stb (r1)
    inc r1
    from r4
    swap
    ror
    add r4
    to r4
    adc r4
    and #1
    inc r0
    stb (r1)
    inc r1
    from r4
    swap
    ror
    add r4
    to r4
    adc r4
    to r2
    lob
    to r3
    hib
    from r2
    stw (r1)
    inc r1
    inc r1
    from r3
    stw (r1)
    inc r1
    inc r1
    loop
    nop
    bra .lC16F
    nop
.lC16A:
    with r1
    add #6
    loop
    nop
.lC16F:
    sms (!fx_effect_seed-!gsu_ram),r4
    move r15,r11
    nop

; @routine 01:C175 FxEffectsDraw
; WHAT IT DOES: Draws each effect in use: FxHazeSample at its X and Y
; with its time, then FxHazePlot.
; HOW: Pushes R11 on the R10 stack; sixteen records from $2688 (R4
; counting, BNE), R4 and R3 kept on the R10 stack around each. For a
; record in use (its time by LDB, LOB, MOVES) $0340 is cleared first.
; Returns by pulling R15.
; Entry: R10 = stack; R11 = return address. Exit: R3 = $26E8, R4 = 0;
; the rest as the last effect drawn leaves them; S and Z from R10; the
; last RAM address R10.
%at($01C175)
FxEffectsDraw:
    from r11
    stw (r10)
    inc r10
    inc r10
    iwt r3,#$2688
    iwt r4,#$0010
.lC17F:
    from r4
    stw (r10)
    inc r10
    inc r10
    from r3
    stw (r10)
    inc r10
    inc r10
    ldb (r3)
    lob
    moves r0,r0
    beq .lC1AA
    nop
    iwt r1,#$0340
    ibt r2,#$00
    from r2
    stw (r1)
    inc r3
    inc r3
    to r1
    ldw (r3)
    inc r3
    inc r3
    to r2
    ldw (r3)
    inc r3
    inc r3
    link #4
    iwt r15,#FxHazeSample
    nop
    link #4
    iwt r15,#FxHazePlot
    nop
.lC1AA:
    dec r10
    dec r10
    to r3
    ldw (r10)
    dec r10
    dec r10
    to r4
    ldw (r10)
    with r3
    add #6
    dec r4
    bne .lC17F
    nop
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop

arch 65816
