; object_links.asm: object helpers of bank $03 that walk the links between object records: the list at $0325 and the +$27 chain of children.
; C: src/stuntracefx/object/object_links.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 03:B8E7 ObjectListTableFirst
; WHAT IT DOES: Clears the count at $0D04 and walks the list at $0325 to
; its end. Only for its first object, and only when A's low byte is not 0
; and the object's +$08 has bit 0 clear, it writes a 5-word entry at
; $7E:96AD: the object, then the words at +$08, +$0A, +$0C and +$0E of the
; record its +$04 points to, and counts it in $0D04.
; HOW: 16-bit, P saved and restored. The test that skips an object is a
; BNE on the flags of the instruction before the loop's PHX: LDY #0 for
; the first object, the TAX of its next link for the others (never 0
; there), so the table gets one entry at most. Through the caller's data
; bank.
; Entry: any widths; A = the flag tested. Exit: P unchanged; X = 0; list
; not empty: A = 0, Y = 10 when an entry was written, else 0; empty: A, Y
; unchanged.
%at($03B8E7)
ObjectListTableFirst:
    php
    rep.b #$30
    stz.w !object_table_count
    ldx.w !object_list_0325
    beq .lB93C
    ldy.w #$0000
.lB8F5:
    phx
    bne .lB935
    and.w #$00FF
    beq .lB935
    lda.w !obj_08,x
    bit.w #$0001
    bne .lB935
    phx
    lda.w !obj_04,x
    tyx
    tay
    pla
    sta.l !object_table,x
    lda.w !dp_08,y
    sta.l !object_table+2,x
    lda.w !actor_0a,y
    sta.l !object_table+4,x
    lda.w !actor_0c,y
    sta.l !object_table+6,x
    lda.w !actor_0e,y
    sta.l !object_table+8,x
    inc.w !object_table_count
    txa
    clc
    adc.w #$000A
    tay
.lB935:
    plx
    lda.w !obj_base,x
    tax
    bne .lB8F5
.lB93C:
    plp
    rtl

; @routine 03:CBB0 ObjectListFreeAll
; WHAT IT DOES: Frees every object of the list whose head is at $0325,
; following each one's +$00 link (ObjectFreeLong on each).
; HOW: X 16-bit from the start; X saved and restored; the next link is
; kept in Y across each free. Through the caller's data bank.
; Entry: any widths. Exit: X unchanged, X 16-bit; N, Z from X (16-bit); if
; the list was not empty, M 8-bit, Y = 0 and A from the last free (the free
; list's old head); otherwise A, Y and M unchanged.
%at($03CBB0)
ObjectListFreeAll:
    rep.b #$10
    phx
    ldx.w !object_list_0325
    beq .lCBC6
.lCBB8:
    ldy.w !obj_base,x
    sep.b #$20
    phy
    jsl.l ObjectFreeLong
    ply
    tyx
    bne .lCBB8
.lCBC6:
    plx
    rtl

; @routine 03:CBC8 ObjectListFreeAllCopy
; WHAT IT DOES: The same bytes as ObjectListFreeAll, at another address.
; HOW: As ObjectListFreeAll.
; Entry: as ObjectListFreeAll. Exit: as ObjectListFreeAll.
%at($03CBC8)
ObjectListFreeAllCopy:
    rep.b #$10
    phx
    ldx.w !object_list_0325
    beq .lCBDE
.lCBD0:
    ldy.w !obj_base,x
    sep.b #$20
    phy
    jsl.l ObjectFreeLong
    ply
    tyx
    bne .lCBD0
.lCBDE:
    plx
    rtl

; @routine 03:CCEB ObjectLinkChild
; WHAT IT DOES: Makes object Y a child of object X tagged A: stores the tag
; at $1F2B and at Y's +$48, X at Y's +$06, clears Y's +$27, then puts Y at
; the end of the +$27 chain that starts at X.
; HOW: Y kept in direct page $3E while the chain is walked (16-bit) to the
; record whose +$27 is 0. Through the caller's data bank.
; Entry: M 8-bit, X 16-bit, D = $0000; A = tag, X = parent, Y = child.
; Exit: A = Y = the child (A 16 bits wide in M 8-bit); X unchanged; M 8-bit;
; N, Z from Y.
%at($03CCEB)
ObjectLinkChild:
    sta.w !wram_1F2B
    lda.w !wram_1F2B
    sta.w !obj_48,y
    pha
    rep.b #$20
    txa
    sta.w !obj_06,y
    sep.b #$20
    pla
    rep.b #$20
    lda.w #$0000
    sta.w !obj_27,y
    sep.b #$20
    rep.b #$20
    sty.b !dp_3e
    txa
.lCD0D:
    tay
    lda.w !obj_27,y
    bne .lCD0D
    lda.b !dp_3e
    sta.w !obj_27,y
    ldy.b !dp_3e
    sep.b #$20
    rtl

; @routine 03:D2DA ObjectChildFindTagX
; WHAT IT DOES: Finds the child of object X tagged A: Y = the first record
; along X's +$27 chain whose +$48 is A, or 0 when there is none.
; HOW: The tag kept at $1A30; X saved, walked along the chain, restored.
; Through the caller's data bank.
; Entry: M 8-bit, X 16-bit; A = tag, X = object. Exit: Y = the child or 0;
; X unchanged; A = the last tag read (unchanged if none); C from the last
; compare; N, Z from X.
%at($03D2DA)
ObjectChildFindTagX:
    sta.w !child_search_tag
    phx
.lD2DE:
    ldy.w !obj_27,x
    beq .lD2EE
    lda.w !obj_48,y
    cmp.w !child_search_tag
    beq .lD2EE
    tyx
    bra .lD2DE
.lD2EE:
    plx
    rtl

; @routine 03:D2F0 ObjectChildFindTagY
; WHAT IT DOES: ObjectChildFindTagX with X and Y the other way: X = the
; child of object Y tagged A, or 0.
; HOW: As ObjectChildFindTagX, Y saved and restored.
; Entry: M 8-bit, X 16-bit; A = tag, Y = object. Exit: X = the child or 0;
; Y unchanged; A, C as ObjectChildFindTagX; N, Z from Y.
%at($03D2F0)
ObjectChildFindTagY:
    sta.w !child_search_tag
    phy
.lD2F4:
    ldx.w !obj_27,y
    beq .lD304
    lda.w !obj_48,x
    cmp.w !child_search_tag
    beq .lD304
    txy
    bra .lD2F4
.lD304:
    ply
    rtl

; @routine 03:D365 ObjectSpawnA040
; WHAT IT DOES: Takes a new object (ObjectAlloc) and clears it (ObjectInit),
; then sets its +$36 to 3 and its +$04 (its handler) to $A040. Carry set
; and X = Y = the object when it got one; carry clear and X = 0 otherwise.
; HOW: As ObjectSpawn without the count limit, the fields written with
; long stores into bank $7E.
; Entry: X 16-bit. Exit: got one: A = $A040, M 16-bit, N set, Z clear;
; none: X = 0, N clear, Z set, A, Y and M as ObjectAlloc leaves them.
%at($03D365)
ObjectSpawnA040:
    jsl.l ObjectAlloc
    bcc .lD383
    txy
    sep.b #$20
    jsl.l ObjectInit
    lda.b #$03
    sta.l !obj_36_long,x
    rep.b #$20
    lda.w #$A040
    sta.l !obj_04_long,x
    sec
    rtl
.lD383:
    ldx.w #$0000
    clc
    rtl
