; actors.asm: the menu screens' objects. 22 records of $2C bytes from $1265
; ($1265-$162C), each run once a frame through the handler address at +$08.
; Free records are kept on a stack of addresses at $120D (depth $11F5, in
; bytes); live ones in the list at $1239 (length $11F7, in bytes), which the
; dispatcher walks from the end. Other fields used here: +$10 (set to $FFFF
; when a record is made free), +$12 (an index into the sprite records at
; $7F:135A), +$1A (the object's kind) and +$24 (a pending palette).
;
; The records are reached as direct page offsets indexed by the record
; address (D = $0000); the lists through the data bank.
; C: src/stuntracefx/menu/actors.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 0B:B479 MenuActorPoolInit
; WHAT IT DOES: Empties the menu object pool and sets the menu's sprite
; defaults.
; HOW: Pushes all 22 record addresses on the free stack, giving each the
; idle handler $D8A8 and +$10 = $FFFF; clears both lengths; stores $0200,
; $0020 and $2881 at $1201, $1203 and $120B; sets OBSEL to 2.
; Entry: D = $0000. Exit: A = $2802; X = $162D; Y = $2C; P unchanged.
%at($0BB479)
MenuActorPoolInit:
    php
    rep.b #$30
    ldy.w #$0000
    ldx.w #$1265
.lB482:
    txa
    sta.w !actor_free_stack,y
    lda.w #$D8A8
    sta.b !actor_handler,x
    lda.w #$FFFF
    sta.b !actor_10,x
    iny
    iny
    txa
    clc
    adc.w #$002C
    tax
    cmp.w #$162D
    bne .lB482
    lda.w #$0000
    sta.w !actor_free_depth
    sta.w !actor_list_len
    lda.w #$0200
    sta.w !menu_1201
    lda.w #$0020
    sta.w !menu_1203
    lda.w #$2881
    sta.w !menu_120b
    sep.b #$20
    lda.b #$02
    sta.w !OBSEL
    rep.b #$20
    plp
    rtl

; @routine 0B:B4C3 MenuActorAlloc
; WHAT IT DOES: Takes a record from the free stack and adds it to the live
; list; X = $FFFF when none is free.
; HOW: Pops the next address at $120D + depth and appends it to $1239.
; Entry: M 16-bit, X 16-bit. Exit: A = X = the record; N, Z from it; C
; clear. When none is free: X = $FFFF, A unchanged; N, C set, Z clear.
%at($0BB4C3)
MenuActorAlloc:
    ldx.w !actor_free_depth
    cpx.w #$002C
    beq .full
    lda.w !actor_free_stack,x
    inx
    inx
    stx.w !actor_free_depth
    ldx.w !actor_list_len
    sta.w !actor_list,x
    inc.w !actor_list_len
    inc.w !actor_list_len
    tax
    rts
.full:                              ; not reached in the traces so far
    ldx.w #$FFFF
    rts

; @routine 0B:B4E5 MenuActorFree
; WHAT IT DOES: Returns record X to the free stack and removes it from the
; live list, whose entry Y it occupied, by moving the last entry there.
; HOW: Push on $120D; shorten $1239 by one and copy its last entry to Y;
; $11F9 (the record being run) = $FFFF.
; Entry: M 16-bit, X 16-bit; X = record, Y = its live list offset. Exit: A =
; the entry moved; X = $FFFF; N set, Z clear.
%at($0BB4E5)
MenuActorFree:
    txa
    ldx.w !actor_free_depth
    dex
    dex
    stx.w !actor_free_depth
    sta.w !actor_free_stack,x
    ldx.w !actor_list_len
    dex
    dex
    stx.w !actor_list_len
    lda.w !actor_list,x
    sta.w !actor_list,y
    ldx.w #$FFFF
    stx.w !actor_current
    rts

; @routine 0B:E28D MenuActorFind
; WHAT IT DOES: Finds the live object of kind A (+$1A), searching from the
; end of the list.
; HOW: Compares each record's +$1A with A (kept at $11E1).
; Entry: M 16-bit, X 16-bit, D = $0000; A = kind. Exit: found: X = the
; record, Y = its list offset, Z and C set. Not found: Y = $FFFE, N set, Z
; clear, C from the last compare.
%at($0BE28D)
MenuActorFind:
    sta.w !scratch_11e1
    ldy.w !actor_list_len
    dey
    dey
    bmi .done
.lE297:
    ldx.w !actor_list,y
    lda.b !actor_kind,x
    cmp.w !scratch_11e1
    bne .lE2A2
    rts
.lE2A2:
    dey
    dey
    bpl .lE297
.done:                              ; not reached in the traces so far
    rts

; @routine 0B:C9A1 MenuActorsSet10
; WHAT IT DOES: Sets field +$10 to Y in every live object of kind A.
; HOW: Y and A are kept at $11DF and $11E1; the list is walked from the end.
; Entry: M 16-bit, X 16-bit, D = $0000; A = kind, Y = value. Exit: A = the
; last value loaded; X unchanged; Y = $FFFE; N, Z from X; C from the last
; compare.
%at($0BC9A1)
MenuActorsSet10:
    sty.w !scratch_11df
    sta.w !scratch_11e1
    phx
    ldy.w !actor_list_len
    dey
    dey
    bmi .lC9C2
.lC9AF:
    ldx.w !actor_list,y
    lda.b !actor_kind,x
    cmp.w !scratch_11e1
    bne .lC9BE
    lda.w !scratch_11df
    sta.b !actor_10,x
.lC9BE:
    dey
    dey
    bpl .lC9AF
.lC9C2:
    plx
    rts

; @routine 0B:CAB9 MenuActorApplyPalette
; WHAT IT DOES: Applies object X's pending palette (+$24) to its sprites and
; clears it.
; HOW: The object's sprite group starts at $7F:1356 + its +$12: a count
; (entries - 1), then 8-byte entries from $7F:135A whose attribute word gets
; bits 9-11 replaced by the palette (kept at $11DF).
; Entry: M 16-bit, X 16-bit, D = $0000; X = record. Exit: nothing pending: A
; = 0, Z set. Otherwise A = the last entry offset + 8; Y = $FFFF; X
; unchanged; N, Z from X; C, V from the last add.
%at($0BCAB9)
MenuActorApplyPalette:
    lda.b !actor_palette,x
    beq .lCAE5
    sta.w !scratch_11df
    phx
    lda.b !actor_12,x
    tax
    lda.l !menu_sprite_groups,x
    tay
    inx
    inx
.lCACB:
    lda.l !menu_sprite_attr,x
    and.w #$F1FF
    ora.w !scratch_11df
    sta.l !menu_sprite_attr,x
    txa
    clc
    adc.w #$0008
    tax
    dey
    bpl .lCACB
    plx
    stz.b !actor_palette,x
.lCAE5:
    rts

; @routine 0B:C47B MenuActorIdleC47B
; WHAT IT DOES: A handler that does nothing.
; HOW: Returns.
; Entry, exit: any; nothing changes.
%at($0BC47B)
MenuActorIdleC47B:
    rts

; @routine 0B:CE89 MenuActorIdleCE89
; WHAT IT DOES: A handler that does nothing.
; HOW: Returns.
; Entry, exit: any; nothing changes.
%at($0BCE89)
MenuActorIdleCE89:
    rts

; @routine 0B:C576 MenuActorBlink
; WHAT IT DOES: Every fourth step of the counter at $11DD, flips bit 11 of
; the attribute word of the sprite entry at $7F:135A + $11E7.
; HOW: Tests the counter's low two bits; on zero, an EOR with $0800.
; Entry: M 16-bit, X 16-bit. Exit: A = the counter's low bits or the new
; attribute word, X = $11E7 when flipped; N, Z from A.
%at($0BC576)
MenuActorBlink:
    lda.w !menu_blink_counter
    and.w #$0003
    bne .lC58C
    ldx.w !menu_blink_entry
    lda.l !menu_sprite_attr,x
    eor.w #$0800
    sta.l !menu_sprite_attr,x
.lC58C:
    rts
