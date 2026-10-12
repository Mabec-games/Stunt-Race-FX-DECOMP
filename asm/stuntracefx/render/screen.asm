; screen.asm: the Super FX's side of its screen buffer. The S-CPU sets the
; buffer's start ($2C00) at $70:021A when it flips buffers (gsu/screen.asm);
; the buffer is $6800 bytes, 832 characters of 4 bits per pixel.
; C: src/stuntracefx/render/screen.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"

arch superfx

; @routine 01:AEB9 FxScreenClear
; WHAT IT DOES: Clears the screen buffer whose start is the word at $021A
; to colour 0.
; HOW: CACHE, then a LOOP of $3400 passes, each storing a zero word at R1
; and stepping R1 by 2 (one INC before LOOP, one in its delay slot).
; Entry: R11 = return address. Exit: R0 = 0; R1 = the buffer's end; R12 = 0;
; R13 = $AEC4 (the loop); the last RAM address the last word stored; CY
; set and OV clear (SUB R0); S and Z from R1 (the INC in LOOP's delay slot).
%at($01AEB9)
FxScreenClear:
    sub r0
    lm r1,(!gsu_screen_start)
    iwt r12,#$3400
    cache
    move r13,r15
    stw (r1)
    inc r1
    loop
    inc r1
    jmp r11
    nop

arch 65816
