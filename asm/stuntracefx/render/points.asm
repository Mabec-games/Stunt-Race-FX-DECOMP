; points.asm: the Super FX's point records at $0826, ten bytes each: two
; words at +0 and +2, then a point's X, Y, Z at +4, +6, +8. The points are
; taken from the table at $0340 (three words each), picked by index bytes
; in ROM and moved by the vector at $0026.
; C: src/stuntracefx/render/points.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"

arch superfx

; @routine 01:C1BE FxPointsGather
; WHAT IT DOES: Fills R9 point records: the halved word pairs of the
; 8-byte entries at the word at $005A (less 4), and the points named by R9
; index bytes at R14, each moved by the vector at $0026.
; HOW: CACHE. A LOOP of R9 passes from the last entry back: two LDWs from
; entry i, DIV2 of each, STW to record i at +0 and +2 (records 1 to R9).
; Then a LOOP of R9 passes forward: GETB, INC R14, UMULT #6 picks a point
; at $0340; its three words plus the vector go to record k at +4, +6, +8
; (records 0 to R9 - 1). R9 = 0 runs each loop $10000 times.
; Entry: R9 = the count; R14 = the first index byte, in the ROM bank;
; R11 = return address. Exit: R14 past the last index byte; R2 = $082A
; plus ten per record; R1 = the last point's Z address; R0 = the last Z
; stored; R3 = record 1's first word as stored; R5, R6, R7 = the
; vector; R12 = 0; R13 = $C1FD (the second loop); the last RAM address
; the last Z's; OV and CY from the last ADD #6 to R2; Z set, S clear.
%at($01C1BE)
FxPointsGather:
    cache
    lms r0,(!fx_pairs)
    to r2
    sub #4
    iwt r1,#$0826
    from r9
    umult #8
    to r2
    add r2                      ; R2 = entry R9
    from r9
    umult #10
    to r1
    add r1                      ; R1 = record R9
    move r12,r9
    move r13,r15
    to r3
    ldw (r2)
    inc r2
    inc r2
    ldw (r2)
    with r3
    div2
    div2
    from r3
    stw (r1)
    inc r1
    inc r1
    stw (r1)
    with r2
    sub #10
    with r1
    sub #12
    loop
    nop
    lms r5,(!fx_offset)
    lms r6,(!fx_offset+2)
    lms r7,(!fx_offset+4)
    iwt r2,#$082A
    move r12,r9
    move r13,r15
    getb
    inc r14
    umult #6
    iwt r1,#$0340
    with r1
    add r0                      ; R1 = the point
    ldw (r1)
    inc r1
    inc r1
    add r5
    stw (r2)
    inc r2
    inc r2
    ldw (r1)
    inc r1
    inc r1
    add r6
    stw (r2)
    inc r2
    inc r2
    ldw (r1)
    add r7
    stw (r2)
    with r2
    add #6
    loop
    nop
    jmp r11
    nop

arch 65816
