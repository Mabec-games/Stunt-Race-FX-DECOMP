; fx_points.asm: the Super FX unpacks a model's points from a byte script in
; ROM into the list at $70:2488, three words (x, y, z) a point.
;
; The script is read through the ROM buffer. Commands: 4, a count, then
; that many points as three signed bytes; $38, a count, then that many
; points each stored twice, as (x, y, z) and (-x, y, z); $20, a word
; added to R14 (a jump, counted from the word's second byte); $1C, a skipped
; byte, then a jump through a table of such words indexed by the word at
; $0044. Anything else ends the script.
; C: src/stuntracefx/render/fx_points.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"

arch superfx

; @routine 04:A154 FxModelPoints
; WHAT IT DOES: Unpacks the point script at R9 (in ROMBR's bank) into the
; list at $2488, as described above.
; HOW: R14 takes the script and R9 the list; each command byte is GETBS
; and compared with 4, $38, $1C and $20 in turn. Points go out with STW
; under LOOP (the last INC R9 in its delay slot); the jumps add to R14.
; R3, R1 and R2 are kept on the R10 stack. The routine runs under CACHE
; (the code cache only; nothing the C has to do).
; Entry: R9 = the script; R10 = stack; R11 = return address. Exit: R9 =
; just past the last point; R14 = just past the ending byte; R0 = that
; byte (sign-extended); R1, R2, R3 as on entry; R12, R13 from the last
; count; S, Z from R10, OV, CY from the compare with $20; the last RAM
; address R10.
%at($04A154)
FxModelPoints:
    from r3
    stw (r10)
    inc r10
    inc r10
    from r1
    stw (r10)
    inc r10
    inc r10
    from r2
    stw (r10)
    inc r10
    inc r10
    move r14,r9
    iwt r9,#!fx_points_w
    cache
.command:
    getbs
    inc r14
    ibt r3,#$04
    cmp r3
    beq .points
    nop
    ibt r3,#$38
    cmp r3
    beq .mirrored
    nop
    ibt r3,#$1C
    cmp r3
    beq .variant
    nop
    ibt r3,#$20
    cmp r3
    beq .jump
    nop
    dec r10
    dec r10
    to r2
    ldw (r10)
    dec r10
    dec r10
    to r1
    ldw (r10)
    dec r10
    dec r10
    to r3
    ldw (r10)
    move r15,r11
    nop
.points:
    to r12
    getb
    inc r14
    move r13,r15
    getbs
    inc r14
    stw (r9)
    inc r9
    inc r9
    getbs
    inc r14
    stw (r9)
    inc r9
    inc r9
    getbs
    inc r14
    stw (r9)
    inc r9
    loop
    inc r9
    bra .command
    nop
.mirrored:
    to r12
    getb
    inc r14
    move r13,r15
    getbs
    inc r14
    stw (r9)
    inc r9
    inc r9
    to r1
    getbs
    inc r14
    from r1
    stw (r9)
    inc r9
    inc r9
    to r2
    getbs
    inc r14
    from r2
    stw (r9)
    inc r9
    inc r9
    not
    inc r0
    stw (r9)
    inc r9
    inc r9
    from r1
    stw (r9)
    inc r9
    inc r9
    from r2
    stw (r9)
    inc r9
    loop
    inc r9
    bra .command
    nop
.variant:
    inc r14
    lms r0,(!fx_variant_w)
    add r0
    with r14
    add r0
.jump:
    getb
    inc r14
    getbh
    with r14
    add r0
    iwt r15,#.command
    nop

arch 65816
