; fx_unused.asm: Super FX code of bank 04 that nothing in the game calls
; or starts: a checksum program over the object pools' RAM, an object-free
; program and an object-new program.
; C: src/stuntracefx/object/fx_unused.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch superfx

; @routine 04:8915 FxPoolChecksumProgram
; WHAT IT DOES: A program: the object pools' checksum (FxPoolChecksum)
; into $2686, then STOP. Nothing in the game starts it.
; HOW: R10 = $0240, LINK to FxPoolChecksum, SM.
; Entry: run as a program. Exit: (at the STOP) as FxPoolChecksum leaves
; the registers, R11 = $891D.
%at($048915)
FxPoolChecksumProgram:
    iwt r10,#$0240
    link #4
    iwt r15,#FxPoolChecksum
    nop
    sm (!fx_init_2686-!gsu_ram),r0
    stop
    nop
; @routine 04:8923 FxPoolChecksum
; WHAT IT DOES: The XOR of the $AF9 words of Super FX RAM from $1090
; (the object pools) into R0. Only FxPoolChecksumProgram calls it.
; HOW: RAM bank 0; a LOOP of LDW, INC R1 twice, XOR.
; Entry: R11 = return address. Exit: R0 = the checksum, S and Z from it;
; R1 = $2682; R2 = the last word; R12 = 0; the cache base at 04:8920.
%at($048923)
FxPoolChecksum:
    sub r0
    ramb
    iwt r1,#$1090
    iwt r12,#$0AF9
    cache
    move r13,r15
    to r2
    ldw (r1)
    inc r1
    inc r1
    xor r2
    loop
    nop
    move r15,r11
    nop


; @routine 04:89FA FxObjectFreeProgram
; WHAT IT DOES: A program: frees the Super FX object at $238E
; (FxObjectFree), then STOP. Nothing in the game starts it.
; HOW: R10 = $0240, R1 = $238E, LINK to FxObjectFree.
; Entry: run as a program; $238E = the object. Exit: (at the STOP) as
; FxObjectFree leaves the registers.
%at($0489FA)
FxObjectFreeProgram:
    iwt r10,#$0240
    lm r1,(!gsu_record_arg-!gsu_ram)
    link #4
    iwt r15,#FxObjectFree
    nop
    stop
    nop


; @routine 04:8AD0 FxObjectNewProgram
; WHAT IT DOES: A program: takes an object of $28 bytes from the pool at
; $1098 (FxPoolTake); none: $238E = 0. Otherwise it is placed at
; ($238E, $2390, $2392) (+$1E/+$20/+$22), its +$16 gets bits $A0,
; +$1A = $FFFF, $238E = it, then its course segment (FxRecordSegment)
; and FxCourseNextEntry, whose R0 goes to $2390; then STOP. Nothing in
; the game starts it.
; HOW: R10 = $0240; the object in R1, then R8 for the calls.
; Entry: run as a program; $238E-$2392 = the place. Exit: (at the STOP)
; as the last call leaves the registers.
%at($048AD0)
FxObjectNewProgram:
    iwt r10,#$0240
    iwt r4,#$0028
    iwt r3,#$1098
    link #4
    iwt r15,#FxPoolTake
    nop
    moves r1,r1
    beq .l8B2C
    nop
    lm r2,(!gsu_record_arg-!gsu_ram)
    ibt r0,#$1E
    add r1
    from r2
    stw (r0)
    lm r2,(!gsu_in_amount-!gsu_ram)
    ibt r0,#$20
    add r1
    from r2
    stw (r0)
    lm r2,(!fx_script_at-!gsu_ram)
    ibt r0,#$22
    add r1
    from r2
    stw (r0)
    ibt r2,#$16
    with r2
    add r1
    to r2
    ldw (r2)
    iwt r0,#$00A0
    to r2
    or r2
    ibt r0,#$16
    add r1
    from r2
    stw (r0)
    iwt r2,#$FFFF
    ibt r0,#$1A
    add r1
    from r2
    stw (r0)
    sm (!gsu_record_arg-!gsu_ram),r1
    move r8,r1
    link #4
    iwt r15,#FxRecordSegment
    nop
    link #4
    iwt r15,#FxCourseNextEntry
    nop
    sm (!gsu_in_amount-!gsu_ram),r0
    stop
    nop
.l8B2C:
    sub r0
    sm (!gsu_record_arg-!gsu_ram),r0
    stop

