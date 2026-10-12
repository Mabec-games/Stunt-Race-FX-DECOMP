; sort.asm: the Super FX's draw list. Each frame the objects' slots at $0AAA
; ($1E bytes each, the count's low byte at $01C8) are turned into view
; space, culled, and linked into one list in depth order, the farthest
; first, for the renderer to paint back to front.
;
; A slot: +$00 the next slot in the list (0 ends it), +$02 the depth key,
; +$07 flags (bit 7: already in view space), +$08 the object's shape in
; ROM bank 0, +$10, +$12, +$14 the position (Y, X, Z once in view space).
; The shape gives the object's radius at +$10 and a depth bias at +$05.
; The list's head is the word at $023C.
; C: src/stuntracefx/render/sort.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"

arch superfx

; @routine 01:AEF8 FxSortObjectsProgram
; WHAT IT DOES: A program the S-CPU starts: builds the draw list of the
; object slots at $0AAA, the farthest first, leaving out those behind the
; view or outside its sides.
; HOW: Sets the ROM bank to 0 and the list head ($023C) to 0. Stops at
; once when the count's low byte is 0, else CACHE and a LOOP over the
; slots. A slot without flag bit 7 has its position multiplied by the
; matrix at $00E4 (as FxMatrixApply, written back to +$12, +$10, +$14).
; The radius (shape +$10, GETB/GETBH) culls it when Z + radius is
; negative or |X| - radius is not below |Z|. Otherwise its key is the
; old key + Z + the shape's bias (+$05), stored at +$02, and it is linked
; in before the first slot whose key is not above it (+$00). Then STOP.
; Entry: started by the S-CPU. Exit (at the STOP): R9 past the last slot;
; R12 = 0; R13 = $AF0F (the slot loop); the other registers, the flags
; and the last RAM address as the last slot left them; the ROM bank 0.
; R14 and the ROM buffer move as on the Super FX.
%at($01AEF8)
FxSortObjectsProgram:
    sub r0
    romb
    sm (!gsu_draw_list),r0
    iwt r9,#$0AAA
    lms r12,(!gsu_records_n-!gsu_ram)
    with r12
    lob
    bne .start
    nop
    stop
    nop
.start:
    cache
    move r13,r15
    ibt r8,#$07
    with r8
    add r9
    ldb (r8)
    iwt r1,#$0080
    and r1
    bne .cull
    nop
    ibt r8,#$12
    with r8
    add r9
    to r1
    ldw (r8)
    ibt r8,#$10
    with r8
    add r9
    to r2
    ldw (r8)
    ibt r8,#$14
    with r8
    add r9
    to r3
    ldw (r8)
    lms r6,(!gsu_out_e4-!gsu_ram)
    from r1
    fmult
    to r5
    rol
    lms r6,(!gsu_out_e4-!gsu_ram+6)
    from r2
    fmult
    rol
    with r5
    add r0
    lms r6,(!gsu_out_e4-!gsu_ram+12)
    from r3
    fmult
    rol
    add r5
    ibt r8,#$12
    with r8
    add r9
    stw (r8)
    lms r6,(!gsu_out_e4-!gsu_ram+2)
    from r1
    fmult
    to r5
    rol
    lms r6,(!gsu_out_e4-!gsu_ram+8)
    from r2
    fmult
    rol
    with r5
    add r0
    lms r6,(!gsu_out_e4-!gsu_ram+14)
    from r3
    fmult
    rol
    add r5
    ibt r8,#$10
    with r8
    add r9
    stw (r8)
    lms r6,(!gsu_out_e4-!gsu_ram+4)
    from r1
    fmult
    to r5
    rol
    lms r6,(!gsu_out_e4-!gsu_ram+10)
    from r2
    fmult
    rol
    with r5
    add r0
    lms r6,(!gsu_out_e4-!gsu_ram+16)
    from r3
    fmult
    rol
    add r5
    ibt r8,#$14
    with r8
    add r9
    stw (r8)
.cull:
    ibt r8,#$08
    with r8
    add r9
    ldw (r8)
    ibt r14,#$10
    with r14
    add r0
    to r1
    getb
    inc r14
    with r1
    getbh
    ibt r8,#$14
    with r8
    add r9
    to r3
    ldw (r8)
    from r3
    add r1
    bpl .z_abs
    nop
    iwt r15,#.next
    nop
.z_abs:
    moves r3,r3
    bpl .x_abs
    nop
    with r3
    not
    inc r3
.x_abs:
    ibt r8,#$12
    with r8
    add r9
    ldw (r8)
    moves r0,r0
    bpl .x_test
    nop
    not
    inc r0
.x_test:
    sub r1
    cmp r3
    bmi .key
    nop
    iwt r15,#.next
    nop
.key:
    ibt r8,#$08
    with r8
    add r9
    ldw (r8)
    iwt r1,#$0005
    to r14
    add r1
    getb
    inc r14
    ibt r8,#$14
    with r8
    add r9
    to r1
    ldw (r8)
    getbh
    to r1
    add r1
    ibt r8,#$02
    with r8
    add r9
    ldw (r8)
    to r1
    add r1
    ibt r2,#$00
    lm r7,(!gsu_draw_list)
    moves r7,r7
    bne .walk
    nop
    sm (!gsu_draw_list),r9
    move r7,r2
    bra .store
    nop
.walk:
    inc r7
    inc r7
    ldw (r7)
    dec r7
    dec r0
    cmp r1
    bpl .step
    dec r7
    moves r2,r2
    bne .after_prev
    nop
    sm (!gsu_draw_list),r9
    bra .store
    nop
.after_prev:
    from r9
    stw (r2)
    bra .store
    nop
.step:
    move r2,r7
    to r7
    ldw (r7)
    moves r7,r7
    bne .walk
    nop
    bra .after_prev
    nop
.store:
    ibt r8,#$02
    with r8
    add r9
    from r1
    stw (r8)
    ibt r8,#$00
    with r8
    add r9
    from r7
    stw (r8)
.next:
    ibt r0,#$1E
    to r9
    add r9
    loop
    nop
    stop
    nop

arch 65816
