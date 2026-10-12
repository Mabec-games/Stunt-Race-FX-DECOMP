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

arch 65816
