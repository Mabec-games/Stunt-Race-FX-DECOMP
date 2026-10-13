; fx_init.asm: the Super FX program that sets up its RAM.
; C: src/stuntracefx/gsu/fx_init.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"

arch superfx

; @routine 04:8800 FxInit
; WHAT IT DOES: A program: tests Super FX RAM (writes $1234 to $235C and
; reads it back; a mismatch would set $26F0 to $0F, decoded from the ROM
; bytes and not reached in the traces so far), clears $0B2C words from
; $1098, makes each record pool one free block ($0CD8 bytes at $1098,
; $05BE at $1D70), and sets $2636 = $0100, $2380 = $2382 = $2642 = -1,
; $2472 = 3 and $2732, $26F0, $2B92, $2B94, $2684, $2686, $1090, $2470,
; $273C, $235C = 0.
; HOW: RAMB 0; the clearing is STW under LOOP with INC R1 in its delay
; slot; the test reads the word twice (into R3, then R0) and compares.
; Entry: the S-CPU starts it. Exit: (at its STOP) R0 = 0; R1 = $1D72; R2
; = $1234; R3 = the word read back; R10 = $0240; R12 = 0; R13 = $8831;
; RAMBR = 0; Z, CY set, S, OV clear; the last RAM address $235C.
%at($048800)
FxInit:
    iwt r10,#!fx_stack
    sub r0
    ramb
    sub r0
    sm (!fx_init_2732),r0
    sub r0
    sm (!fx_ram_bad),r0
    iwt r1,#!fx_ram_test_w
    iwt r0,#$1234
    move r2,r0
    stw (r1)
    sub r0
    to r3
    ldw (r1)
    ldw (r1)
    cmp r2
    beq .ram_ok
    nop
    ibt r0,#$0F
    sm (!fx_ram_bad),r0
.ram_ok:
    iwt r1,#!fx_pool_first
    sub r0
    iwt r12,#$0B2C
    move r13,r15
    stw (r1)
    inc r1
    loop
    inc r1
    iwt r0,#$0100
    sm (!fx_init_2636),r0
    ibt r0,#-1
    sm (!fx_init_2382),r0
    sm (!fx_init_2380),r0
    ibt r0,#-1
    sm (!fx_init_2642),r0
    iwt r1,#!fx_pool_first
    sub r0
    stw (r1)
    inc r1
    inc r1
    iwt r0,#$0CD8
    stw (r1)
    iwt r1,#!fx_pool_second
    sub r0
    stw (r1)
    inc r1
    inc r1
    iwt r0,#$05BE
    stw (r1)
    sub r0
    sm (!fx_init_2b94),r0
    sm (!fx_init_2b92),r0
    sm (!fx_init_2684),r0
    sm (!fx_init_2686),r0
    sm (!fx_init_1090),r0
    sm (!fx_init_2470),r0
    sm (!fx_init_273c),r0
    ibt r0,#3
    sm (!fx_init_2472),r0
    iwt r0,#$0000
    sm (!fx_ram_test),r0
    stop
    nop

arch 65816
