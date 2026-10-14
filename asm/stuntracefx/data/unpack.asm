; unpack.asm: the Super FX's decompressor. Packed data is read backward from
; ROM (R14, in the ROM bank) through a 32-bit bit buffer, R2:R3 (R2 the
; high word), shifted out from bit 0. A refill loads the next four bytes
; and sets a marker bit above them, so the buffer is empty when R2:R3
; shifts down to 0.
; C: src/stuntracefx/data/unpack.c.

incsrc "../include/macros.inc"

arch superfx

; @routine 01:D0DB FxUnpackRefill
; WHAT IT DOES: Refills the bit buffer from the four ROM bytes at R14 and
; below and takes its first bit.
; HOW: GETB/GETBH with R14 stepping down: R3 = byte at R14 with the byte
; below as its high byte, R2 = the next two the same way. Then ROR of R0
; (just set to 1) puts 1 in CY, and ROR R2, ROR R3 shift the 32 bits down
; one with that 1 as the marker on top; the bit shifted out is the first
; bit. The caller's delay slot steps R14 down to the first byte.
; Entry: R14 = the address of the first byte; R11 = return address.
; Exit: R2:R3 = the bit buffer; CY = the first bit; R14 = the address of
; the fourth byte; R0 = R2 | R3 (not 0), S and Z from it; OV kept.
; Returns the first bit. R14 and the ROM buffer move as on the Super FX.
%at($01D0DB)
FxUnpackRefill:
    to r3
    getb
    dec r14
    with r3
    getbh
    dec r14
    to r2
    getb
    dec r14
    with r2
    getbh
    ibt r0,#$01
    ror                         ; CY = 1, the marker
    with r2
    ror
    with r3
    ror                         ; CY = the first bit
    from r2
    or r3
    jmp r11
    nop

; @routine 01:D0F3 FxUnpackBits
; WHAT IT DOES: Reads R12 bits from the bit buffer into R4, the first
; read ending as the highest.
; HOW: Its first SUB R4 runs with the WITH R4 the caller leaves in its
; call's delay slot, clearing R4. R13 = R7, the loop address the caller
; sets ($D0F7, the LSR after a WITH R2, so LOOP's delay slot WITH R2
; belongs to the next pass). Each pass: LSR R2, ROR R3 take the next bit
; into CY; when R2:R3 is then 0 the bit was the marker, so the buffer is
; refilled (FxUnpackRefill's code, inline after a DEC R14) and its first
; bit taken; ROL R4 shifts the bit in. R12 = 0 reads $10000 bits.
; Entry: R12 = the count; R7 = $D0F7; R2:R3 = the bit buffer, R14 its
; source; R11 = return address; WITH R4 pending. Exit: R4 = the bits;
; R2:R3 and R14 moved on; R0 = R2 | R3 after the last bit (or $8000 when
; that bit needed a refill); R12 = 0; R13 = R7; OV from the first SUB; CY
; from the last ROL R4; Z set, S clear; WITH R2 pending at the return.
; R14 and the ROM buffer move as on the Super FX.
%at($01D0F3)
FxUnpackBits:
    sub r4                      ; with the caller's WITH R4: R4 = 0
    move r13,r7
    with r2
FxUnpackBitsLoop:               ; the callers' R7 (LOOP's delay slot WITH R2 leads in)
    lsr
    with r3
    ror
    from r2
    or r3
    beq .refill
    nop
    with r4
    rol
    loop
    with r2
    jmp r11
    nop
.refill:
    dec r14
    to r3
    getb
    dec r14
    with r3
    getbh
    dec r14
    to r2
    getb
    dec r14
    with r2
    getbh
    ibt r0,#$01
    ror
    with r2
    ror
    with r3
    ror
    with r4
    rol
    loop
    with r2
    jmp r11
    nop

; @routine 01:CF42 FxUnpackProgram
; WHAT IT DOES: A program the S-CPU starts: unpacks the data that ends at
; $70:0068 in the ROM bank at $70:006A into game pak RAM bank 0 from
; $70:002C up, writing from the output's end down; then, when $70:00A2 is
; not 0, adds it to the words from $70:002C up.
; HOW: Reads backward from the packed data's end: the unpacked size (a
; word), then two words that fill the bit buffer R2:R3. The output's end
; (the start + the size) goes to $0060 and R1 steps down from it, one byte
; per store. Each command (from R8 = $CF78): a run of literal bytes, its
; length in 3 bits (0: none; 7: 4 more bits + 7, or after a 1, 10 bits,
; or 18 when those are 0), each byte 8 bits; then, until R1 is back at the
; start, a copy from R1 + offset - 1 down, its length from a 2-bit code
; (0: 2; 1: 3; 2: 4; 3: 2 more bits, giving 5 or 6, 7 + 2 bits, or 8
; bits) and its offset of 8, 12, 14 or 16 bits. Bits come through
; FxUnpackBits and FxUnpackRefill, and inline in the literal loop. At the
; end R14 goes back to $0068. The pass for $00A2 runs one LOOP pass per
; byte of the size, a word each, so it covers twice the size.
; Entry: started by the S-CPU. Exit: (at the STOP) $70:0068 = the address
; of the last packed byte read; $70:0060 = the output's end; R9 = the
; start; R7 = $D0F7, R8 = $CF78; R12 = 2, and R1 = the start, or after the
; pass R12 = 0 and R1 past it; R11, R13 and the flags as the last call or
; the pass leaves them; RAM bank 0. R14 and the ROM buffer move as on the
; Super FX.
%at($01CF42)
FxUnpackProgram:
    lms r0,(!gsu_in_6a-!gsu_ram)
    romb
    sub r0
    ramb
    lms r14,(!gsu_in_68-!gsu_ram)
    dec r14
    to r1
    getb
    dec r14
    with r1
    getbh
    dec r14
    dec r14
    dec r14
    to r3
    getb
    dec r14
    with r3
    getbh
    dec r14
    to r2
    getb
    dec r14
    with r2
    getbh
    lms r9,(!gsu_dest-!gsu_ram)
    with r1
    add r9
    sms (!gsu_unpack_end-!gsu_ram),r1
    ibt r4,#$00
    ibt r6,#$00
    ibt r5,#$00
    iwt r7,#FxUnpackBitsLoop
    cache
    move r8,r15
    ibt r12,#$03
    link #4
    iwt r15,#FxUnpackBits
    with r4
    ibt r0,#$FF
    and r4
    bne .literals
    nop
    iwt r15,#.copy_or_end
    nop
.literals:
    ibt r0,#$07
    cmp r4
    bne .literal
    nop
    with r2
    lsr
    with r3
    ror
    from r2
    or r3
    bne .long_run
    nop
    link #4
    iwt r15,#FxUnpackRefill
    dec r14
.long_run:
    bcc .mid_run
    nop
    ibt r12,#$0A
    link #4
    iwt r15,#FxUnpackBits
    with r4
    moves r4,r4
    bne .literal
    nop
    ibt r12,#$12                ; 10 bits of 0: 18 bits of length
    link #4
    iwt r15,#FxUnpackBits
    with r4
    bra .literal
    nop
.mid_run:
    ibt r12,#$04
    link #4
    iwt r15,#FxUnpackBits
    with r4
    with r4
    add #7
.literal:
    ibt r12,#$08
    move r13,r15
    with r2
    lsr
    with r3
    ror
    from r2
    or r3
    beq .literal_refill
    nop
    with r6
    rol
    with r5
    rol
    loop
    nop
    dec r1
    from r6
    stb (r1)
    dec r4
    bne .literal
    nop
    bra .copy_or_end
    nop
.literal_refill:
    dec r14
    to r3
    getb
    dec r14
    with r3
    getbh
    dec r14
    to r2
    getb
    dec r14
    with r2
    getbh
    ibt r0,#$01
    ror
    with r2
    ror
    with r3
    ror
    with r6
    rol
    with r5
    rol
    loop
    nop
    dec r1
    from r6
    stb (r1)
    dec r4
    bne .literal
    nop
.copy_or_end:
    with r9
    cmp r1
    bne .copy_code
    ibt r12,#$02
    sms (!gsu_in_68-!gsu_ram),r14
    lms r3,(!gsu_in_a2-!gsu_ram)
    moves r3,r3
    beq .stop
    nop
    lms r1,(!gsu_dest-!gsu_ram)
    lms r2,(!gsu_unpack_end-!gsu_ram)
    to r12
    from r2
    sub r1
    move r13,r15
    ldw (r1)
    add r3
    stw (r1)
    inc r1
    loop
    inc r1
.stop:
    stop
    nop
.copy_code:
    cache
    link #4
    iwt r15,#FxUnpackBits
    with r4
    ibt r6,#$02
    ibt r5,#$00
    ibt r12,#$08
    moves r4,r4
    bne .code_not_0
    nop
    iwt r15,#.copy
    nop
.code_not_0:
    ibt r6,#$04
    ibt r0,#$02
    cmp r4
    beq .offset_width
    nop
    ibt r6,#$03
    ibt r0,#$01
    cmp r4
    beq .length_3
    nop
    ibt r12,#$02
    link #4
    iwt r15,#FxUnpackBits
    with r4
    ibt r0,#$03
    cmp r4
    beq .length_8_bits
    nop
    ibt r0,#$02
    cmp r4
    beq .length_7
    nop
    with r4
    add #5
    move r6,r4
    bra .offset_width
    nop
.length_7:
    ibt r12,#$02
    link #4
    iwt r15,#FxUnpackBits
    with r4
    with r4
    add #7
    move r6,r4
    bra .offset_width
    nop
.length_8_bits:
    ibt r12,#$08
    link #4
    iwt r15,#FxUnpackBits
    with r4
    move r6,r4
    bra .offset_width
    nop
.length_3:
    ibt r12,#$08
    with r2
    lsr
    with r3
    ror
    from r2
    or r3
    bne .length_3_bit
    nop
    link #4
    iwt r15,#FxUnpackRefill
    dec r14
    nop
.length_3_bit:
    bcs .copy
    nop
    ibt r12,#$0E
    bra .copy
    nop
.offset_width:
    ibt r12,#$10
    with r2
    lsr
    with r3
    ror
    from r2
    or r3
    bne .offset_bit
    nop
    link #4
    iwt r15,#FxUnpackRefill
    dec r14
.offset_bit:
    bcc .copy
    nop
    ibt r12,#$08
    with r2
    lsr
    with r3
    ror
    from r2
    or r3
    bne .offset_bit2
    nop
    link #4
    iwt r15,#FxUnpackRefill
    dec r14
.offset_bit2:
    bcs .copy
    nop
    ibt r12,#$0C
.copy:
    link #4
    iwt r15,#FxUnpackBits
    with r4
    dec r6
.copy_byte:
    from r1
    add r4
    dec r0
    ldb (r0)
    dec r1
    stb (r1)
    dec r6
    bpl .copy_byte
    nop
    jmp r8
    nop

arch 65816
