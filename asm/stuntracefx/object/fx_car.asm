; fx_car.asm: the Super FX sets up a car's record from a script in ROM.
;
; The script is read through the ROM buffer from R14 (in ROMBR's bank): a
; byte giving the number of parts, a word for the parts' mode, seven words
; per part, then a byte choosing one of the 5-byte entries that follow.
; Each part is a $52-byte record of its own on the car's list of parts:
; the car's +$3E is the first part, a part's +$34 the next.
; C: src/stuntracefx/object/fx_car.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"

arch superfx

; A word of the script into R3.
macro fx_script_word()
    to r3
    getb
    inc r14
    with r3
    getbh
    inc r14
endmacro

; Field <field> of record <rec> = R3, through R0.
macro fx_field_set(field, rec)
    ibt r0,#<field>
    add <rec>
    from r3
    stw (r0)
endmacro

; @routine 04:8E84 FxCarAddPart
; WHAT IT DOES: Adds a part to car R1: a new $52-byte record from the first
; pool whose +$12, +$3C (negated), +$3E, +$40, +$44, +$50 and +$46 are the
; next seven script words; its +$0C, +$0E, +$10 = $14, bit 2 of +$16 set,
; +$1A = the parts' mode ($0068), +$2A = the car's +$2A; it goes on the
; front of the car's parts list. With no room the script is not read
; (decoded from the ROM bytes and not reached in the traces so far).
; HOW: FxPoolTake; each field is STW through R0 = field + R2 with R3 the
; value. R1 and the return address are kept on the R10 stack.
; Entry: R1 = the car's record; R14 = the script; R10 = stack; R11 =
; return address. Exit: R1 = the car; R2 = the part (0 without); R3 = the
; car's +$2A; R0 = the part + $2A; R14 past the seven words; R11 = $8E97;
; R4, R12, R13 as FxPoolTake leaves them; S, Z from R10, OV, CY from the
; last ADD; the last RAM address R10.
%at($048E84)
FxCarAddPart:
    from r11
    stw (r10)
    inc r10
    inc r10
    from r1
    stw (r10)
    inc r10
    inc r10
    iwt r4,#!fx_part_bytes
    iwt r3,#!fx_pool_first
    link #4
    iwt r15,#FxPoolTake
    nop
    move r2,r1
    dec r10
    dec r10
    to r1
    ldw (r10)
    moves r2,r2
    bne .part
    nop
    iwt r15,#.done
    nop
.part:
    %fx_script_word()
    %fx_field_set(!fxpart_12, r2)
    %fx_script_word()
    with r3
    not
    inc r3
    %fx_field_set(!fxpart_3c, r2)
    %fx_script_word()
    %fx_field_set(!fxpart_3c+2, r2)
    %fx_script_word()
    %fx_field_set(!fxpart_3c+4, r2)
    %fx_script_word()
    %fx_field_set(!fxpart_44, r2)
    %fx_script_word()
    %fx_field_set(!fxpart_50, r2)
    %fx_script_word()
    %fx_field_set(!fxpart_46, r2)
    ibt r3,#!fxcar_parts
    with r3
    add r1
    to r3
    ldw (r3)
    %fx_field_set(!fxpart_next, r2)
    ibt r0,#!fxcar_parts
    add r1
    from r2
    stw (r0)
    iwt r3,#$0014
    %fx_field_set(!fxpart_0c, r2)
    %fx_field_set(!fxpart_0c+2, r2)
    %fx_field_set(!fxpart_0c+4, r2)
    ibt r3,#!fxrec_flags16
    with r3
    add r2
    to r3
    ldw (r3)
    iwt r0,#$0004
    to r3
    or r3
    %fx_field_set(!fxrec_flags16, r2)
    lm r3,(!fx_part_mode)
    %fx_field_set(!fxrec_mode, r2)
    ibt r3,#!fxrec_mass
    with r3
    add r1
    to r3
    ldw (r3)
    %fx_field_set(!fxrec_mass, r2)
.done:
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop

; @routine 04:8F48 FxCarFromScript
; WHAT IT DOES: Sets up car R1 from the script at R14: its parts
; (FxCarAddPart for each), $2392 = where the entries start less $8090, +$6E
; = the entry the next byte chooses (5 bytes each), and +$6A = $0180, +$8C
; = $1000, +$8A = the word at $26F2, +$7A = 0 (laps), +$86 = 2 and +$58 = 1
; (segments).
; HOW: The part count goes to R12 for LOOP, with R13 and R12 kept on the
; R10 stack around each call; the entry is LMULT of the byte by 5 added to
; R14. The return address is kept on the R10 stack.
; Entry: R1 = the car's record; R14 = the script; R10 = stack; R11 =
; return address. Exit: R14 = R2 = the chosen entry; R0 = R1 + $58; R4 =
; the byte times 5; R6 = 5; R11 = $8E97 (with parts); R12 = 0; R13 =
; $8F61; S, Z from R10, OV, CY from the last ADD; the last RAM address R10.
%at($048F48)
FxCarFromScript:
    from r11
    stw (r10)
    inc r10
    inc r10
    sub r0
    getb
    inc r14
    to r2
    getb
    inc r14
    with r2
    getbh
    inc r14
    sm (!fx_part_mode),r2
    moves r12,r0
    beq .parts_done
    nop
    move r13,r15
    from r13
    stw (r10)
    inc r10
    inc r10
    from r12
    stw (r10)
    inc r10
    inc r10
    link #4
    iwt r15,#FxCarAddPart
    nop
    dec r10
    dec r10
    to r12
    ldw (r10)
    dec r10
    dec r10
    to r13
    ldw (r10)
    loop
    nop
.parts_done:
    iwt r0,#$8090
    from r14
    sub r0
    sm (!fx_script_at),r0
    sub r0
    getb
    inc r14
    ibt r6,#5
    lmult
    with r14
    add r4
    move r2,r14
    ibt r0,#!fxcar_entry
    add r1
    from r2
    stw (r0)
    iwt r2,#$0180
    ibt r0,#!fxrec_6a
    add r1
    from r2
    stw (r0)
    iwt r2,#$1000
    iwt r0,#!fxcar_8c
    add r1
    from r2
    stw (r0)
    lm r2,(!fx_car_26f2)
    iwt r0,#!fxcar_8c-2
    add r1
    from r2
    stw (r0)
    iwt r2,#$0000
    ibt r0,#!fxrec_laps
    add r1
    from r2
    stw (r0)
    iwt r2,#$0002
    iwt r0,#!fxrec_counted
    add r1
    from r2
    stw (r0)
    iwt r2,#$0001
    ibt r0,#!fxrec_segment
    add r1
    from r2
    stw (r0)
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop

arch 65816
