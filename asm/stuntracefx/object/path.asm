; path.asm: objects that follow a path of commands in the table at $08:85B9.
; An object's +$29 is its offset into the table; each command is a word,
; then its operands, and the object steps +$29 past them. The commands act
; on the object and on the object its +$4F points at (a full address in the
; data bank, not an offset).
; C: src/stuntracefx/object/path.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 08:88D8 ObjectPathCommand
; WHAT IT DOES: Runs object X's next path command. 3: places the linked
; object (+$1A/+$20, +$1C/+$22, +$1E/+$24 from the three operand words),
; stops the object (velocity and +$27 zero). 2: sets the linked object's
; first word. 1: sets +$27 (the step count) and gives the object the
; velocity that covers the distance from the linked object's +$20/+$22/+$24
; to the target in 2^n frames (n the second operand byte). Any other
; command does nothing and returns carry set.
; HOW: The command word at $08:85B9 + +$29 picks the case; the velocity is
; the difference halved n times, rounding toward zero; $1F4C-$1F53 hold n
; and the three differences. +$29 steps by 8, 4 or 10.
; Entry: M 8-bit, X 16-bit; X = object. Exit: X unchanged; M 8-bit; N, Z
; from X; carry set only for an unknown command (A = the command word, Y =
; X); otherwise carry clear, A = the new +$29, V from its add, Y = X (3),
; the linked object (2), or 0 (1).
%at($0888D8)
ObjectPathCommand:
    phx
    rep.b #$20
    txy
    lda.w !obj_29,x
    tax
    lda.l !path_table,x
    cmp.w #$0003
    beq .l88F8
    cmp.w #$0001
    beq .l8952
    cmp.w #$0002
    beq .l893A
    tyx
    sec
    jmp.w .l89E2
.l88F8:
    phy
    phx
    tyx
    ldy.w !obj_link_w,x
    plx
    lda.l !path_table+2,x
    sta.w 32,y
    sta.w 26,y
    lda.l !path_table+4,x
    sta.w 34,y
    sta.w 28,y
    lda.l !path_table+6,x
    sta.w 36,y
    sta.w 30,y
    ply
    lda.w #$0000
    sta.w !obj_vel_x,y
    sta.w !obj_vel_y,y
    sta.w !obj_vel_z,y
    sta.w !obj_27,y
    lda.w !obj_29,y
    clc
    adc.w #$0008
    sta.w !obj_29,y
    jmp.w .l89E1
.l893A:
    lda.l !path_table+2,x
    tyx
    ldy.w !obj_link_w,x
    sta.w 0,y
    lda.w !obj_29,x
    clc
    adc.w #$0004
    sta.w !obj_29,x
    jmp.w .l89E1
.l8952:
    phy
    lda.l !path_table+2,x
    and.w #$00FF
    sta.w !obj_27,y
    phx
    tyx
    ldy.w !obj_link_w,x
    plx
    lda.l !path_table+3,x
    and.w #$00FF
    sta.w !wram_1F4C
    lda.l !path_table+4,x
    sec
    sbc.w 32,y
    sta.w !wram_1F4E
    lda.l !path_table+6,x
    sec
    sbc.w 34,y
    sta.w !wram_1F50
    lda.l !path_table+8,x
    sec
    sbc.w 36,y
    sta.w !wram_1F52
    plx
    ldy.w !wram_1F4C
    iny
.l8993:
    dey
    beq .l89C5
    lda.w !wram_1F4E
    cmp.w #$8000
    ror a
    bpl .l89A2
    adc.w #$0000
.l89A2:
    sta.w !wram_1F4E
    lda.w !wram_1F50
    cmp.w #$8000
    ror a
    bpl .l89B1
    adc.w #$0000
.l89B1:
    sta.w !wram_1F50
    lda.w !wram_1F52
    cmp.w #$8000
    ror a
    bpl .l89C0
    adc.w #$0000
.l89C0:
    sta.w !wram_1F52
    bra .l8993
.l89C5:
    lda.w !wram_1F4E
    sta.w !obj_vel_x,x
    lda.w !wram_1F50
    sta.w !obj_vel_y,x
    lda.w !wram_1F52
    sta.w !obj_vel_z,x
    lda.w !obj_29,x
    clc
    adc.w #$000A
    sta.w !obj_29,x
.l89E1:
    clc
.l89E2:
    sep.b #$20
    plx
    rtl
