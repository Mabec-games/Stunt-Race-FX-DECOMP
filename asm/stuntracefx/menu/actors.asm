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
; Entry: M 16-bit, X 16-bit, D = $0000; A = kind. Exit: A = the kind of
; the last record looked at (the kind searched for with an empty list);
; found: X = the record, Y = its list offset, Z and C set. Not found: Y =
; $FFFE, N set, Z clear, C from the last compare.
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
; Entry: any. Exit: nothing changes.
%at($0BC47B)
MenuActorIdleC47B:
    rts

; @routine 0B:CE89 MenuActorIdleCE89
; WHAT IT DOES: A handler that does nothing.
; HOW: Returns.
; Entry: any. Exit: nothing changes.
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

; @routine 0B:E250 MenuActorAnimate
; WHAT IT DOES: Steps object X's sprite animation: when its frame timer
; (+$18) runs out, moves to the next entry of its animation (+$14 = the
; animation's offset in the table at $0B:AB60, +$16 = the entry offset),
; restarting at the first entry when the next one's duration is negative,
; and sets the sprite group (+$12) and timer from the entry.
; HOW: Entries are four bytes: a duration word and a sprite group word; the
; timer is set to duration - 1. No animation when +$14 is negative. The
; offset is kept at $11DF while it is used.
; Entry: M 16-bit, X 16-bit, D = $0000; X = the object being run ($11F9).
; Exit: no animation: A = +$14; N set. Timer running: A = +$14; N, Z from
; the timer. Stepped: A = the new timer; X = $11F9; N, Z from A; C, V from
; the entry offset + animation offset.
%at($0BE250)
MenuActorAnimate:
    lda.b !actor_anim,x
    bmi .lE28C
    dec.b !actor_anim_timer,x
    bpl .lE28C
    sta.w !scratch_11df
    lda.b !actor_anim_pos,x
    clc
    adc.w #$0004
    sta.b !actor_anim_pos,x
    clc
    adc.w !scratch_11df
    tax
    lda.l !menu_anim_table,x
    bpl .lE27A
    ldx.w !actor_current
    stz.b !actor_anim_pos,x
    ldx.w !scratch_11df
    lda.l !menu_anim_table,x
.lE27A:
    dec a
    sta.w !scratch_11df
    lda.l !menu_anim_group,x
    ldx.w !actor_current
    sta.b !actor_12,x
    lda.w !scratch_11df
    sta.b !actor_anim_timer,x
.lE28C:
    rts

; @routine 0B:CF9C MenuActorCountdown
; WHAT IT DOES: Counts object X's +$0A down; when it passes zero, moves the
; object to handler $CFAB with the count at 0.
; HOW: A 16-bit memory decrement.
; Entry: M 16-bit, D = $0000; X = record. Exit: counting: A unchanged; N, Z
; from the count. Moved on: A = 0; Z set, N clear.
%at($0BCF9C)
MenuActorCountdown:
    dec.b !actor_0a,x
    bpl .lCFAA
    lda.w #$CFAB
    sta.b !actor_handler,x
    lda.w #$0000
    sta.b !actor_0a,x
.lCFAA:
    rts

; @routine 0B:C9FD MenuSinePosition
; WHAT IT DOES: From the angle in object X's +$0A, sets $18BF = (sine of
; the angle - $40, from the table at $00:96FE) + $81, kept to 8 bits, and
; $18C1 = the same, but at least $A8.
; HOW: The table byte is sign-extended, $80 added and the sum kept to 8
; bits before the + 1.
; Entry: M 16-bit, X 16-bit, D = $0000; X = record. Exit: A = the value
; stored at $18C1; X = $11F9; N, Z from X; C = (value before the minimum
; >= $A8); V from the add of $80.
%at($0BC9FD)
MenuSinePosition:
    lda.b !actor_0a,x
    sec
    sbc.w #$0040
    and.w #$00FF
    tax
    lda.l !rot_sin_table,x
    and.w #$00FF
    cmp.w #$0080
    bcc .lCA16
    ora.w #$FF00
.lCA16:
    clc
    adc.w #$0080
    and.w #$00FF
    inc a
    sta.l !menu_sine_y
    cmp.w #$00A8
    bcs .lCA2A
    lda.w #$00A8
.lCA2A:
    sta.l !menu_sine_y_min
    ldx.w !actor_current
    rts

; @routine 0B:C4E4 MenuActorSetTiles
; WHAT IT DOES: Sets the tile numbers of two sprite entries of object X's
; group: entry Y (offset in bytes from the group's +$12) gets tile
; $0B:C528[A], the entry before it that tile + $10.
; HOW: Each attribute word keeps bits 9-15 and takes the tile in bits 0-8;
; the entry's offset is kept at $11E7 and the tile at $11DF.
; Entry: M 16-bit, X 16-bit, D = $0000; X = record, A = tile index, Y =
; entry offset. Exit: A = the second word written; X unchanged; N, Z from
; X; C, V from the step back of 8.
%at($0BC4E4)
MenuActorSetTiles:
    phx
    pha
    tya
    clc
    adc.b !actor_12,x
    sta.w !menu_blink_entry
    pla
    asl a
    tax
    lda.l !menu_tile_table,x
    sta.w !scratch_11df
    ldx.w !menu_blink_entry
    lda.l !menu_sprite_attr,x
    and.w #$FE00
    ora.w !scratch_11df
    sta.l !menu_sprite_attr,x
    lda.w !scratch_11df
    clc
    adc.w #$0010
    sta.w !scratch_11df
    txa
    sec
    sbc.w #$0008
    tax
    lda.l !menu_sprite_attr,x
    and.w #$FE00
    ora.w !scratch_11df
    sta.l !menu_sprite_attr,x
    plx
    rts

; @routine 0B:CA32 MenuSinePosition7F
; WHAT IT DOES: As MenuSinePosition, but two lower: (sine of the angle - $40)
; + $7F, kept to 8 bits, at $18BF, and the same, at least $A8, at $18C1.
; HOW: The same code as MenuSinePosition with a decrement where it has an
; increment; a value of 0 before it becomes $FFFF.
; Entry: as MenuSinePosition. Exit: as MenuSinePosition.
%at($0BCA32)
MenuSinePosition7F:
    lda.b !actor_0a,x
    sec
    sbc.w #$0040
    and.w #$00FF
    tax
    lda.l !rot_sin_table,x
    and.w #$00FF
    cmp.w #$0080
    bcc .lCA4B
    ora.w #$FF00
.lCA4B:
    clc
    adc.w #$0080
    and.w #$00FF
    dec a
    sta.l !menu_sine_y
    cmp.w #$00A8
    bcs .lCA5F
    lda.w #$00A8
.lCA5F:
    sta.l !menu_sine_y_min
    ldx.w !actor_current
    rts

; @routine 0B:E23D MenuActorAnimateFrame
; WHAT IT DOES: Steps object X's sprite animation once a frame, or four
; times when the fast animation flag $18FD is set.
; HOW: MenuActorAnimate, then three more calls when $18FD is not zero.
; Entry: M 16-bit, X 16-bit, D = $0000; X = the object being run ($11F9).
; Exit: flag clear: A = 0; Z set, N clear; X, C, V from the one step.
; Flag set: A, X, P from the last step.
%at($0BE23D)
MenuActorAnimateFrame:
    jsr.w MenuActorAnimate
    lda.l !wram_18FD
    beq .lE24F
    jsr.w MenuActorAnimate
    jsr.w MenuActorAnimate
    jsr.w MenuActorAnimate
.lE24F:
    rts

; @routine 0B:C82F MenuSetPairAnimations
; WHAT IT DOES: Starts animation pair A of the table at $0B:C862 on the
; objects of kinds 3 and 5: the pair's second word on kind 5, its first on
; kind 3.
; HOW: Both words are pushed, then pulled in reverse after each
; MenuActorFind; an object already playing the animation is left alone.
; Entry: M 16-bit, X 16-bit, D = $0000; A = pair index. Exit: A = the
; pair's first word; X = $11F9; Y from the second search; N, Z from X; C
; from the last compare.
%at($0BC82F)
MenuSetPairAnimations:
    asl a
    asl a
    tax
    lda.l !menu_anim_pairs,x
    pha
    lda.l !menu_anim_pairs+2,x
    pha
    lda.w #$0005
    jsr.w MenuActorFind
    pla
    cmp.b !actor_anim,x
    beq .lC84D
    sta.b !actor_anim,x
    stz.b !actor_anim_pos,x
    stz.b !actor_anim_timer,x
.lC84D:
    lda.w #$0003
    jsr.w MenuActorFind
    pla
    cmp.b !actor_anim,x
    beq .lC85E
    sta.b !actor_anim,x
    stz.b !actor_anim_pos,x
    stz.b !actor_anim_timer,x
.lC85E:
    ldx.w !actor_current
    rts

; @routine 0B:C164 MenuChoiceAnimations
; WHAT IT DOES: Starts on each of the three objects whose kinds are listed
; at $0B:C15E the animation that shows its choice: for choice i, the word
; at $0B:C1B0 + $18CD[i] * 8 + $18E5[i] * 2.
; HOW: i runs over 0, 2, 4, kept on the stack and at $11E3; the table
; offset, then the animation, is built at $11E5. An object already playing
; the animation is left alone.
; Entry: M 16-bit, X 16-bit, D = $0000. Exit: A = the last animation; X =
; $11F9; Y from the last search; N, Z from X; C set; V from the last add of
; $18E5[i] * 2.
%at($0BC164)
MenuChoiceAnimations:
    ldx.w #$0000
.lC167:
    phx
    stx.w !scratch_11e3
    stz.w !scratch_11e5
    lda.l !selection_1,x
    asl a
    asl a
    asl a
    clc
    adc.w !scratch_11e5
    sta.w !scratch_11e5
    lda.l !wram_18E5,x
    asl a
    clc
    adc.w !scratch_11e5
    tax
    lda.l !menu_choice_anims,x
    sta.w !scratch_11e5
    ldx.w !scratch_11e3
    lda.l !menu_choice_kinds,x
    jsr.w MenuActorFind
    lda.w !scratch_11e5
    cmp.b !actor_anim,x
    beq .lC1A4
    sta.b !actor_anim,x
    stz.b !actor_anim_pos,x
    stz.b !actor_anim_timer,x
.lC1A4:
    plx
    inx
    inx
    cpx.w #$0006
    bne .lC167
    ldx.w !actor_current
    rts

; @routine 0B:C8BF MenuSlideInDone
; WHAT IT DOES: Once the sliding sprites' lowest position ($18C1) reaches
; $100: clears +$10 of the objects of kinds 4 and 5, starts animation $AE
; on kind 6 and $B8 on kind 7, and frees object X (the one being run).
; HOW: MenuActorsSet10 twice, MenuActorFind twice with X kept on the stack,
; then MenuActorFree with Y = its list offset at $11FB. An object already
; playing its animation is left alone.
; Entry: M 16-bit, X 16-bit, D = $0000; X = the object being run. Exit:
; not yet: A = $18C1; N, Z, C from the compare with $100. Done: as
; MenuActorFree, Y = $11FB, C from the last animation compare.
%at($0BC8BF)
MenuSlideInDone:
    lda.l !menu_sine_y_min
    cmp.w #$0100
    bne .lC90E
    lda.w #$0004
    ldy.w #$0000
    jsr.w MenuActorsSet10
    lda.w #$0005
    ldy.w #$0000
    jsr.w MenuActorsSet10
    phx
    lda.w #$0006
    jsr.w MenuActorFind
    lda.b !actor_anim,x
    cmp.w #$00AE
    beq .lC8F1
    lda.w #$00AE
    sta.b !actor_anim,x
    stz.b !actor_anim_pos,x
    stz.b !actor_anim_timer,x
.lC8F1:
    lda.w #$0007
    jsr.w MenuActorFind
    lda.b !actor_anim,x
    cmp.w #$00B8
    beq .lC907
    lda.w #$00B8
    sta.b !actor_anim,x
    stz.b !actor_anim_pos,x
    stz.b !actor_anim_timer,x
.lC907:
    plx
    ldy.w !actor_current_slot
    jsr.w MenuActorFree
.lC90E:
    rts

; @routine 0B:CA67 MenuChoiceObjectUpdate
; WHAT IT DOES: Keeps object X, the choice numbered +$20 of a four-way
; selection, in step with the selection ($18C3, previously $18C5): it gets
; palette $0200 when the selection has just left it and $0A00 when it has
; just come to it; arriving there, or resting on it once the selection's
; countdown $18F5 is 0, sets +$22 = 1 and +$0A = 0 (unless +$10 is
; negative), and arriving also restarts the countdown at $FF.
; HOW: Three compare chains on the 16-bit words.
; Entry: M 16-bit, X 16-bit, D = $0000; X = record. Exit: A = the last value
; loaded; N, Z from it or the last compare; C from the last compare; V
; unchanged.
%at($0BCA67)
MenuChoiceObjectUpdate:
    lda.l !wram_18C5
    cmp.b !actor_20,x
    bne .lCA7A
    cmp.l !wram_18C3
    beq .lCA7A
    lda.w #$0200
    sta.b !actor_palette,x
.lCA7A:
    lda.l !wram_18C3
    cmp.b !actor_20,x
    bne .lCA9F
    cmp.l !wram_18C5
    beq .lCA9F
    lda.w #$0A00
    sta.b !actor_palette,x
    lda.b !actor_10,x
    bmi .lCA9F
    lda.w #$0001
    sta.b !actor_22,x
    stz.b !actor_0a,x
    lda.w #$00FF
    sta.l !wram_18F5
.lCA9F:
    lda.l !wram_18F5
    bne .lCAB8
    lda.l !wram_18C3
    cmp.b !actor_20,x
    bne .lCAB8
    lda.b !actor_10,x
    bmi .lCAB8
    lda.w #$0001
    sta.b !actor_22,x
    stz.b !actor_0a,x
.lCAB8:
    rts

; @routine 0B:C47C MenuRowPalette5
; WHAT IT DOES: Gives the two sprite entries at the row position of the
; kind 9 object (MenuRowPosition) palette 5.
; HOW: MenuActorFind for kind 9; each entry's attribute word keeps all but
; bits 9-11 and takes $0A00; the second entry is 8 bytes on.
; Entry: M 16-bit, X 16-bit, D = $0000. Exit: A = the second word written;
; X = the position + 8; Y from the search; N, Z from A; C, V from the add
; of 8.
%at($0BC47C)
MenuRowPalette5:
    lda.w #$0009
    jsr.w MenuActorFind
    jsr.w MenuRowPosition
    tax
    lda.l !menu_sprite_attr,x
    and.w #$F1FF
    ora.w #$0A00
    sta.l !menu_sprite_attr,x
    txa
    clc
    adc.w #$0008
    tax
    lda.l !menu_sprite_attr,x
    and.w #$F1FF
    ora.w #$0A00
    sta.l !menu_sprite_attr,x
    rts

; @routine 0B:C4A9 MenuRowBlinkSettings
; WHAT IT DOES: Blinks the two sprite entries at object X's row position
; (MenuRowPosition, MenuActorBlink on each), then shows the three saved
; settings $18EF, $18F1 and $18F3 as tiles in the kind 9 object's sprite
; entries $2A, $1A and $0A (MenuActorSetTiles).
; HOW: The entry offset is kept at $11E7, the second 8 bytes on.
; Entry: M 16-bit, X 16-bit, D = $0000; X = record. Exit: A and P as the
; last MenuActorSetTiles; X from the search; Y = $000A.
%at($0BC4A9)
MenuRowBlinkSettings:
    jsr.w MenuRowPosition
    sta.w !menu_blink_entry
    jsr.w MenuActorBlink
    lda.w !menu_blink_entry
    clc
    adc.w #$0008
    sta.w !menu_blink_entry
    jsr.w MenuActorBlink
    lda.w #$0009
    jsr.w MenuActorFind
    ldy.w #$002A
    lda.l !wram_18EF
    jsr.w MenuActorSetTiles
    ldy.w #$001A
    lda.l !wram_18F1
    jsr.w MenuActorSetTiles
    ldy.w #$000A
    lda.l !wram_18F3
    jsr.w MenuActorSetTiles
    rts

; @routine 0B:CBC3 MenuActorUploadFrames
; WHAT IT DOES: Shows animation frame A on every sprite of object X's
; group: for each sprite, its graphics (two rows of $40 bytes in bank $7F,
; the second $200 bytes after the first) are queued to its VRAM slot and
; the slot + $100, and bit 15 of the frame word becomes bit 15 (vertical
; flip) of the sprite's attribute word.
; HOW: The group's sprite count is the word at $7F:1356 + +$12, its entries
; follow at $7F:135A. Per sprite i (table offset $11E3, set by the caller):
; VRAM slot $0B:CCBC[i], graphics base $0B:CD14[i]; the frame word comes
; from $0B:CC7C at $11DF = A * 2, which steps by 2 per sprite, wrapping at
; $40. $11E1 holds the slot (its high byte incremented for the second
; row), $11E5 the entry offset, $11E7 the flip bit, $11E9 the base.
; Entry: M 16-bit, X 16-bit, D = $0000; X = record, A = frame, $11E3 =
; first table offset. Exit: A = the entry offset past the last sprite; X =
; $11F9; Y = $FFFF; N, Z from X; C, V from the last add of 8.
%at($0BCBC3)
MenuActorUploadFrames:
    asl a
    sta.w !scratch_11df
    lda.b !actor_12,x
    tax
    lda.l !menu_sprite_groups,x
    inx
    inx
    stx.w !scratch_11e5
    tay
.lCBD4:
    ldx.w !scratch_11e3
    lda.l !menu_frame_slots,x
    sta.w !scratch_11e1
    lda.l !menu_frame_bases,x
    sta.w !scratch_11e9
    ldx.w !scratch_11df
    lda.l !menu_frames,x
    pha
    and.w #$8000
    sta.w !menu_blink_entry
    ldx.w !scratch_11e5
    lda.l !menu_sprite_attr,x
    and.w #$7FFF
    ora.w !menu_blink_entry
    sta.l !menu_sprite_attr,x
    pla
    and.w #$7FFF
    clc
    adc.w !scratch_11e9
    pha
    ldx.w !scratch_11e1
    rep.b #$30
    stx.w !vram_queue_staged+6
    sta.w !vram_queue_staged+2
    lda.w #$0040
    sta.w !vram_queue_staged
    lda.w #$007F
    sta.w !vram_queue_staged+4
    jsl.l VramDmaQueueAdd
    rep.b #$20
    rep.b #$10
    sep.b #$20
    inc.w !scratch_11e1+1
    rep.b #$20
    ldx.w !scratch_11e1
    pla
    clc
    adc.w #$0200
    rep.b #$30
    stx.w !vram_queue_staged+6
    sta.w !vram_queue_staged+2
    lda.w #$0040
    sta.w !vram_queue_staged
    lda.w #$007F
    sta.w !vram_queue_staged+4
    jsl.l VramDmaQueueAdd
    rep.b #$20
    rep.b #$10
    inc.w !scratch_11e3
    inc.w !scratch_11e3
    lda.w !scratch_11df
    inc a
    inc a
    and.w #$003F
    sta.w !scratch_11df
    lda.w !scratch_11e5
    clc
    adc.w #$0008
    sta.w !scratch_11e5
    dey
    bmi .lCC78
    brl .lCBD4
.lCC78:
    ldx.w !actor_current
    rts

; @routine 0B:D5F6 MenuActorAnimateOnly
; WHAT IT DOES: The handler of an object that only animates.
; HOW: MenuActorAnimateFrame.
; Entry: as MenuActorAnimateFrame. Exit: as MenuActorAnimateFrame.
%at($0BD5F6)
MenuActorAnimateOnly:
    jsr.w MenuActorAnimateFrame
    rts

; @routine 0B:C985 MenuActorFollowSlide1CF
; WHAT IT DOES: The handler of an object that moves with the sliding
; sprites: +$02 (its position) = $1CF - $18C1, then its animation step.
; HOW: A 16-bit subtraction, then MenuActorAnimateFrame.
; Entry: M 16-bit, X 16-bit, D = $0000; X = record. Exit: as
; MenuActorAnimateFrame; C and V from the subtraction when no step changed
; them.
%at($0BC985)
MenuActorFollowSlide1CF:
    lda.w #$01CF
    sec
    sbc.l !menu_sine_y_min
    sta.b !actor_02,x
    jsr.w MenuActorAnimateFrame
    rts

; @routine 0B:C993 MenuActorFollowSlide194
; WHAT IT DOES: As MenuActorFollowSlide1CF, with +$02 = $194 - $18C1.
; HOW: As MenuActorFollowSlide1CF.
; Entry: as MenuActorFollowSlide1CF. Exit: as MenuActorFollowSlide1CF.
%at($0BC993)
MenuActorFollowSlide194:
    lda.w #$0194
    sec
    sbc.l !menu_sine_y_min
    sta.b !actor_02,x
    jsr.w MenuActorAnimateFrame
    rts

; @routine 0B:CD6C MenuActorAnimateThenExit
; WHAT IT DOES: The handler of an object that animates for +$0A + 1 frames
; and then asks the menu to exit ($11EB = 1) on every later frame.
; HOW: 16-bit registers forced; MenuActorAnimateFrame; a memory decrement
; of +$0A.
; Entry: D = $0000; X = record. Exit: A = 1 when the count is negative,
; otherwise as MenuActorAnimateFrame; N, Z from the count or A; M 16-bit,
; X 16-bit.
%at($0BCD6C)
MenuActorAnimateThenExit:
    rep.b #$30
    jsr.w MenuActorAnimateFrame
    dec.b !actor_0a,x
    bpl .lCD7B
    lda.w #$0001
    sta.w !menu_exit
.lCD7B:
    rts

; @routine 0B:C872 MenuSlideInHandler
; WHAT IT DOES: The handler that slides the selection sprites in: the angle
; +$0A falls by 3 a frame to 0 and sets the sprites' height
; (MenuSinePosition7F); once it is below $60 the selection can be moved
; (MenuChoiceUpDown); at 0 the handler becomes $B952.
; HOW: A negative angle is clamped to 0 before it is stored; the fields
; are read again through $11F9, which MenuSinePosition7F leaves in X.
; Entry: M 16-bit, X 16-bit, D = $0000; X = record. Exit: at 0: A = $B952;
; N set, Z clear. Otherwise A = +$0A and P from the compare with $60, or
; as MenuChoiceUpDown (C clear); X = $11F9.
%at($0BC872)
MenuSlideInHandler:
    lda.b !actor_0a,x
    dec a
    dec a
    dec a
    bpl .lC87C
    lda.w #$0000
.lC87C:
    sta.b !actor_0a,x
    jsr.w MenuSinePosition7F
    lda.b !actor_0a,x
    bne .lC88B
    lda.w #$B952
    sta.b !actor_handler,x
    rts
.lC88B:
    lda.b !actor_0a,x
    cmp.w #$0060
    bcs .lC895
    jsr.w MenuChoiceUpDown
.lC895:
    rts

; @routine 0B:C896 MenuFadeInHandler
; WHAT IT DOES: The handler that fades sprite palettes 5 and 1 as +$0A
; counts down by 2 (MenuSpritePalettesScale with the count); at 0 it sets
; +$10 of kind 2 objects to $FFFF and of kind 3 objects to 0, and moves on
; to handler $C8BF (MenuSlideInDone) with +$1E = 30.
; HOW: MenuActorsSet10 twice for the last step.
; Entry: M 16-bit, X 16-bit, D = $0000; X = record. Exit: fading: as
; MenuSpritePalettesScale. Done: A = $1E; Y = $FFFE; N, Z clear; C from
; the last compare.
%at($0BC896)
MenuFadeInHandler:
    lda.b !actor_0a,x
    beq .lC8A2
    dec a
    dec a
    sta.b !actor_0a,x
    jsr.w MenuSpritePalettesScale
    rts
.lC8A2:
    lda.w #$0002
    ldy.w #$FFFF
    jsr.w MenuActorsSet10
    lda.w #$0003
    ldy.w #$0000
    jsr.w MenuActorsSet10
    lda.w #$C8BF
    sta.b !actor_handler,x
    lda.w #$001E
    sta.b !actor_1e,x
    rts

; @routine 0B:CB81 MenuChoiceObjectFrames
; WHAT IT DOES: Every +$1C + 1 frames (counted in +$1E), applies object X's
; pending palette and, while it is selected (+$22), steps its frame count
; +$0A down through 31-1 (from 2 when +$10 is negative), showing frame
; +$0A on its sprites (MenuActorUploadFrames); the count reaching 0, or 1
; when +$10 is negative, ends the selection (+$22 = +$0A = 0).
; HOW: Memory decrement of +$1E; MenuActorApplyPalette; the count kept to
; 5 bits. The fields after the frame upload are read through $11F9, which
; it leaves in X.
; Entry: M 16-bit, X 16-bit, D = $0000; X = record; $11E3 = the first
; frame table offset. Exit: waiting: A unchanged; N, Z from +$1E.
; Otherwise A and P from the last load, compare or call; X = $11F9 after
; a frame upload or an end.
%at($0BCB81)
MenuChoiceObjectFrames:
    dec.b !actor_1e,x
    bmi .lCB88
    brl .lCBBA
.lCB88:
    lda.b !actor_1c,x
    sta.b !actor_1e,x
    jsr.w MenuActorApplyPalette
    lda.b !actor_22,x
    bne .lCB96
    brl .lCBBA
.lCB96:
    lda.b !actor_10,x
    bpl .lCB9F
    lda.w #$0002
    sta.b !actor_0a,x
.lCB9F:
    lda.b !actor_0a,x
    dec a
    and.w #$001F
    sta.b !actor_0a,x
    bne .lCBAC
    brl .lCBBB
.lCBAC:
    jsr.w MenuActorUploadFrames
    lda.b !actor_10,x
    bpl .lCBBA
    lda.b !actor_0a,x
    cmp.w #$0001
    beq .lCBBB
.lCBBA:
    rts
.lCBBB:
    ldx.w !actor_current
    stz.b !actor_22,x
    stz.b !actor_0a,x
    rts

; @routine 0B:CB01 MenuChoiceObject12Start
; WHAT IT DOES: First frame of a MenuChoiceObject12 object: makes that its
; handler and runs it.
; HOW: +$08 = $CB06, then falls through into MenuChoiceObject12.
; Entry: as MenuChoiceObject12. Exit: as MenuChoiceObject12.
%at($0BCB01)
MenuChoiceObject12Start:
    lda.w #$CB06
    sta.b !actor_handler,x

; @routine 0B:CAEE MenuChoiceObject00
; WHAT IT DOES: The handler of a choice object whose frames start at frame
; table offset 0: MenuChoiceObjectUpdate, then MenuChoiceObjectFrames.
; HOW: $11E3 = 0 between the two.
; Entry: M 16-bit, X 16-bit, D = $0000; X = record. Exit: as
; MenuChoiceObjectFrames.
%at($0BCAEE)
MenuChoiceObject00:
    jsr.w MenuChoiceObjectUpdate
    stz.w !scratch_11e3
    jsr.w MenuChoiceObjectFrames
    rts

; @routine 0B:CB06 MenuChoiceObject12
; WHAT IT DOES: As MenuChoiceObject00, the frames starting at offset $12.
; HOW: As MenuChoiceObject00.
; Entry: as MenuChoiceObject00. Exit: as MenuChoiceObject00.
%at($0BCB06)
MenuChoiceObject12:
    jsr.w MenuChoiceObjectUpdate
    lda.w #$0012
    sta.w !scratch_11e3
    jsr.w MenuChoiceObjectFrames
    rts

; @routine 0B:CB1B MenuChoiceObject24
; WHAT IT DOES: As MenuChoiceObject00, the frames starting at offset $24.
; HOW: As MenuChoiceObject00.
; Entry: as MenuChoiceObject00. Exit: as MenuChoiceObject00.
%at($0BCB1B)
MenuChoiceObject24:
    jsr.w MenuChoiceObjectUpdate
    lda.w #$0024
    sta.w !scratch_11e3
    jsr.w MenuChoiceObjectFrames
    rts

; @routine 0B:CB74 MenuChoiceObject48
; WHAT IT DOES: As MenuChoiceObject00, the frames starting at offset $48.
; HOW: As MenuChoiceObject00.
; Entry: as MenuChoiceObject00. Exit: as MenuChoiceObject00.
%at($0BCB74)
MenuChoiceObject48:
    jsr.w MenuChoiceObjectUpdate
    lda.w #$0048
    sta.w !scratch_11e3
    jsr.w MenuChoiceObjectFrames
    rts

; @routine 0B:E1C5 MenuActorCopyGroup
; WHAT IT DOES: Gives object X ($11F9) its own copy of the sprite group its
; animation starts with: the group (a count word, then count + 1 entries
; of 8 bytes) is copied to the next free place in the group area, whose
; offset becomes the object's +$12. A copy that would reach $2C81 stops the
; game (Hang); no script gets there.
; HOW: The source group is the word at $0B:AB62 + +$14. $11DF = the size
; in bytes, (count + 1) * 8 + 2; $11E1 the source offset, $11E3 the
; destination; the next free offset $120B advances by the size. Copied a
; word at a time, $11DF counting down by 2.
; Entry: M 16-bit, X 16-bit, D = $0000. Exit: A = the last word copied; X =
; $11F9; N, Z from X; C clear; V from the add of the size.
%at($0BE1C5)
MenuActorCopyGroup:
    ldx.w !actor_current
    lda.b !actor_anim,x
    tax
    lda.l !menu_anim_group,x
    tax
    lda.l !menu_sprite_groups,x
    inc a
    asl a
    asl a
    asl a
    clc
    adc.w #$0002
    sta.w !scratch_11df
    stx.w !scratch_11e1
    ldx.w !actor_current
    lda.w !menu_120b
    sta.b !actor_12,x
    sta.w !scratch_11e3
    clc
    adc.w !scratch_11df
    cmp.w #$2C81
    bcs Hang
    sta.w !menu_120b
.lE1F9:
    ldx.w !scratch_11e1
    lda.l !menu_sprite_groups,x
    ldx.w !scratch_11e3
    sta.l !menu_sprite_groups,x
    inc.w !scratch_11e1
    inc.w !scratch_11e1
    inc.w !scratch_11e3
    inc.w !scratch_11e3
    dec.w !scratch_11df
    dec.w !scratch_11df
    bne .lE1F9
    ldx.w !actor_current
    rts

; @routine 0B:C90F MenuFixedColourDown
; WHAT IT DOES: Steps the fixed colour's intensity $18F9 down by 2 toward 0
; (not below 0) and queues it to COLDATA for all three components.
; HOW: Nothing happens at 0; otherwise fixed_colour_set, with X kept on the
; stack around FrameByteWritePush.
; Entry: M 16-bit, X 16-bit. Exit: at 0: A = 0; Z set, N clear. Otherwise
; A = the COLDATA value (high byte 0); X unchanged; N, Z from X; M 16-bit,
; X 16-bit. C and V unchanged.
%at($0BC90F)
MenuFixedColourDown:
    lda.l !wram_18F9
    beq .lC933
    dec a
    dec a
    bpl .lC91C
    lda.w #$0000
.lC91C:
    sta.l !wram_18F9
    sep.b #$20
    ora.b #$E0
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #!COLDATA
    jsl.l FrameByteWritePush
    plx
    rep.b #$20
.lC933:
    rts

; @routine 0B:C934 MenuFixedColourUp
; WHAT IT DOES: Steps the fixed colour's intensity $18F9 up by 2 toward $1F
; (not above it) and queues it to COLDATA for all three components.
; HOW: Nothing happens at $1F; otherwise fixed_colour_set, with X kept on
; the stack around FrameByteWritePush.
; Entry: M 16-bit, X 16-bit. Exit: at $1F: A = $1F; Z, C set, N clear.
; Otherwise A = the COLDATA value (high byte 0); X unchanged; N, Z from X;
; C = (the raised value >= $1F); M 16-bit, X 16-bit.
%at($0BC934)
MenuFixedColourUp:
    lda.l !wram_18F9
    cmp.w #$001F
    beq .lC95E
    inc a
    inc a
    cmp.w #$001F
    bcc .lC947
    lda.w #$001F
.lC947:
    sta.l !wram_18F9
    sep.b #$20
    ora.b #$E0
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #!COLDATA
    jsl.l FrameByteWritePush
    plx
    rep.b #$20
.lC95E:
    rts

; @routine 0B:C95F MenuFadeUpHandler
; WHAT IT DOES: The handler that raises the fixed colour each frame
; (MenuFixedColourUp) until the sliding sprites' lowest position $18C1 is
; back at $A8; then it sets +$10 of kind 3 objects to $FFFF and of kind 2
; objects to 0 and moves on to handler $C9C4 with +$0A = 0.
; HOW: MenuActorsSet10 around the handler change.
; Entry: M 16-bit, X 16-bit, D = $0000; X = record. Exit: waiting: A =
; $18C1; N, Z, C from the compare with $A8. Done: A as MenuActorsSet10;
; Y = $FFFE; N, Z from X; C from its last compare.
%at($0BC95F)
MenuFadeUpHandler:
    jsr.w MenuFixedColourUp
    lda.l !menu_sine_y_min
    cmp.w #$00A8
    bne .lC984
    lda.w #$0003
    ldy.w #$FFFF
    jsr.w MenuActorsSet10
    lda.w #$C9C4
    sta.b !actor_handler,x
    stz.b !actor_0a,x
    lda.w #$0002
    ldy.w #$0000
    jsr.w MenuActorsSet10
.lC984:
    rts

; @routine 0B:C9C4 MenuFadeOutHandler
; WHAT IT DOES: The handler that keeps raising the fixed colour
; (MenuFixedColourUp) while +$0A counts up by 2 to $20, scaling sprite
; palettes 5 and 1 by it (MenuSpritePalettesScale); at $20 it frees the
; object.
; HOW: MenuActorFree with Y = its list offset at $11FB.
; Entry: M 16-bit, X 16-bit, D = $0000; X = record. Exit: counting: as
; MenuSpritePalettesScale. At $20: as MenuActorFree, Y = $11FB, C set.
%at($0BC9C4)
MenuFadeOutHandler:
    jsr.w MenuFixedColourUp
    lda.b !actor_0a,x
    cmp.w #$0020
    beq .lC9D6
    inc a
    inc a
    sta.b !actor_0a,x
    jsr.w MenuSpritePalettesScale
    rts
.lC9D6:
    ldy.w !actor_current_slot
    jsr.w MenuActorFree
    rts

; @routine 0B:CAE6 MenuChoiceObject00Start
; WHAT IT DOES: First frame of a MenuChoiceObject00 object: copies its
; sprite group (MenuActorCopyGroup), makes MenuChoiceObject00 its handler
; and runs it.
; HOW: +$08 = $CAEE, then falls through into MenuChoiceObject00.
; Entry: M 16-bit, X 16-bit, D = $0000. Exit: as MenuChoiceObject00.
%at($0BCAE6)
MenuChoiceObject00Start:
    jsr.w MenuActorCopyGroup
    lda.w #$CAEE
    sta.b !actor_handler,x

; @routine 0B:CAF8 MenuChoiceObject12Copy
; WHAT IT DOES: First frame of a MenuChoiceObject12 object: copies its
; sprite group (MenuActorCopyGroup) and makes MenuChoiceObject12Start
; ($CB01) its handler for the next frame.
; HOW: +$08 = $CB01.
; Entry: M 16-bit, X 16-bit, D = $0000. Exit: A = $CB01; X = $11F9; N set,
; Z clear; C clear.
%at($0BCAF8)
MenuChoiceObject12Copy:
    jsr.w MenuActorCopyGroup
    lda.w #$CB01
    sta.b !actor_handler,x
    rts

; @routine 0B:CB13 MenuChoiceObject24Start
; WHAT IT DOES: As MenuChoiceObject00Start, for MenuChoiceObject24
; ($CB1B).
; HOW: +$08 = $CB1B, then falls through into MenuChoiceObject24.
; Entry: as MenuChoiceObject00Start. Exit: as MenuChoiceObject24.
%at($0BCB13)
MenuChoiceObject24Start:
    jsr.w MenuActorCopyGroup
    lda.w #$CB1B
    sta.b !actor_handler,x

; @routine 0B:C46D MenuRowStart
; WHAT IT DOES: First frame of the settings row object: copies its sprite
; group (MenuActorCopyGroup), blinks the row and shows the saved settings
; (MenuRowBlinkSettings), and leaves it idle (handler $C47B).
; HOW: +$08 = $C47B, then falls through into MenuActorIdleC47B, which only
; returns.
; Entry: M 16-bit, X 16-bit, D = $0000. Exit: A = $C47B; X = $11F9; Y =
; $000A; N set, Z clear; C, V as MenuRowBlinkSettings left them.
%at($0BC46D)
MenuRowStart:
    jsr.w MenuActorCopyGroup
    jsr.w MenuRowBlinkSettings
    ldx.w !actor_current
    lda.w #$C47B
    sta.b !actor_handler,x

; @routine 0B:CD7C MenuDropStart
; WHAT IT DOES: The handler that waits +$0A + 1 frames and then starts a
; drop of BG1: handler $CE19 (MenuDropBounce) with speed +$22 = 1, BG1's
; vertical scroll set from the height +$20, and for the next vertical
; blank HDMA channel 4 on, the sub screen window mask and colour math off,
; BG1 and BG2 maps at VRAM $6800 and $6000 (both 64 tiles wide), BG1
; tiles at $4000 and BG2 tiles at 0.
; HOW: A memory decrement of +$0A; the register values go through
; FrameByteWritePush, X kept on the stack around each.
; Entry: M 16-bit, X 16-bit, D = $0000; X = record. Exit: waiting: A
; unchanged; N, Z from the count. Started: A = the scroll's high byte and
; $04; X unchanged; N, Z from X; C, V from 0 - the height; M 16-bit.
%at($0BCD7C)
MenuDropStart:
    dec.b !actor_0a,x
    bmi .lCD83
    brl .lCE18
.lCD83:
    lda.w #$CE19
    sta.b !actor_handler,x
    lda.w #$0001
    sta.b !actor_22,x
    lda.w #$0000
    sec
    sbc.b !actor_20,x
    sep.b #$20
    xba
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #!BG1VOFS
    jsl.l FrameByteWritePush
    plx
    xba
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #!BG1VOFS
    jsl.l FrameByteWritePush
    plx
    lda.b #$10
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #!HDMAEN
    jsl.l FrameByteWritePush
    plx
    lda.b #$00
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #!TSW
    jsl.l FrameByteWritePush
    plx
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #!CGWSEL
    jsl.l FrameByteWritePush
    plx
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #!CGADSUB
    jsl.l FrameByteWritePush
    plx
    lda.b #$69
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #!BG1SC
    jsl.l FrameByteWritePush
    plx
    lda.b #$61
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #!BG2SC
    jsl.l FrameByteWritePush
    plx
    lda.b #$04
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #!BG12NBA
    jsl.l FrameByteWritePush
    plx
    rep.b #$20
.lCE18:
    rts

; @routine 0B:CE19 MenuDropBounce
; WHAT IT DOES: One frame of BG1 dropping into place and bouncing: the
; speed +$22 grows by 1 every second frame and moves the height +$20
; (negative is above the rest position); reaching the rest position, the
; object bounces back with the speed reversed and reduced by 2 and sound
; $19 plays; once the speed is below 2 there, the handler becomes $CE89
; (idle). BG1's vertical scroll follows -height and +$06 = height + $4D.
; HOW: MenuActorAnimateFrame first; +$0A toggles 0/1 for the every second
; frame; the scroll goes through queue_bg1_vscroll.
; Entry: M 16-bit, X 16-bit, D = $0000; X = record. Exit: moving: A =
; +$06; N, Z, C, V from its add; X = the record after the animation step;
; M 16-bit, X 16-bit. Stopped: A = $CE89; X = $11F9; N set, Z clear; C, V
; from the height's add.
%at($0BCE19)
MenuDropBounce:
    jsr.w MenuActorAnimateFrame
    lda.b !actor_0a,x
    inc a
    and.w #$0001
    sta.b !actor_0a,x
    bne .lCE2E
    lda.b !actor_22,x
    clc
    adc.w #$0001
    sta.b !actor_22,x
.lCE2E:
    lda.b !actor_20,x
    clc
    adc.b !actor_22,x
    sta.b !actor_20,x
    lda.b !actor_20,x
    bmi .lCE51
    lda.b !actor_22,x
    dec a
    dec a
    bmi .lCE80
    eor.w #$FFFF
    inc a
    sta.b !actor_22,x
    stz.b !actor_20,x
    sep.b #$20
    lda.b #$19
    jsl.l SoundPort0QueuePush
    rep.b #$20
.lCE51:
    lda.w #$0000
    sec
    sbc.b !actor_20,x
    sep.b #$20
    xba
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #!BG1VOFS
    jsl.l FrameByteWritePush
    plx
    xba
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #!BG1VOFS
    jsl.l FrameByteWritePush
    plx
    rep.b #$20
    lda.b !actor_20,x
    clc
    adc.w #$004D
    sta.b !actor_06,x
    rts
.lCE80:
    ldx.w !actor_current
    lda.w #$CE89
    sta.b !actor_handler,x
    rts

; @routine 0B:CF27 MenuDropAway
; WHAT IT DOES: One frame of BG1 falling out of view: the speed +$22 grows
; by 1 every second frame and moves the height +$20 down; from $1E on,
; HDMA channel 5 alone is switched on each frame; at $DC the handler
; becomes $CE89 (idle). BG1's vertical scroll follows -height and +$06 =
; height + $4D.
; HOW: As MenuDropBounce without the bounce; the HDMA enable goes through
; FrameByteWritePush.
; Entry: M 16-bit, X 16-bit, D = $0000; X = record. Exit: moving: A =
; +$06; N, Z, C, V from its add; X = the record after the animation step;
; M 16-bit, X 16-bit. Gone: A = $CE89; X = $11F9; N, C set, Z clear; V
; from the height's add.
%at($0BCF27)
MenuDropAway:
    jsr.w MenuActorAnimateFrame
    lda.b !actor_0a,x
    inc a
    and.w #$0001
    sta.b !actor_0a,x
    bne .lCF3C
    lda.b !actor_22,x
    clc
    adc.w #$0001
    sta.b !actor_22,x
.lCF3C:
    lda.b !actor_20,x
    clc
    adc.b !actor_22,x
    sta.b !actor_20,x
    lda.b !actor_20,x
    bmi .lCF64
    cmp.w #$00DC
    bcs .lCF93
    cmp.w #$001E
    bcc .lCF64
    sep.b #$20
    lda.b #$20
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #!HDMAEN
    jsl.l FrameByteWritePush
    plx
    rep.b #$20
.lCF64:
    lda.w #$0000
    sec
    sbc.b !actor_20,x
    sep.b #$20
    xba
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #!BG1VOFS
    jsl.l FrameByteWritePush
    plx
    xba
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #!BG1VOFS
    jsl.l FrameByteWritePush
    plx
    rep.b #$20
    lda.b !actor_20,x
    clc
    adc.w #$004D
    sta.b !actor_06,x
    rts
.lCF93:
    ldx.w !actor_current
    lda.w #$CE89
    sta.b !actor_handler,x
    rts

; @routine 0B:D07C MenuDropAwayFinish
; WHAT IT DOES: Once BG1 has fallen away: for the next vertical blank, BG1's
; vertical scroll 0, HDMA and the window masks off, colour math adding the
; sub screen to BG1 only, BG2 on the sub screen, maps BG2 at $6400 and BG1
; at $6000 (64 x 32), BG1 tiles at 0 and BG2 tiles at $2000, BG1-BG3 and
; sprites on the main screen. Then the object being run ($11F9) and every
; kind $11 object get +$10 = $FFFF, and the object counts 64 frames in
; +$0A under handler $D14A (MenuCrossFadeOut).
; HOW: FrameByteWritePush for each register; MenuActorsSet10 for the kind
; $11 objects.
; Entry: D = $0000. Exit: A = $40; X = $11F9; Y = $FFFE; N, Z clear; C
; from MenuActorsSet10's last compare; M 16-bit, X 16-bit.
%at($0BD07C)
MenuDropAwayFinish:
    sep.b #$20
    rep.b #$10
    lda.b #$00
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #!BG1VOFS
    jsl.l FrameByteWritePush
    plx
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #!BG1VOFS
    jsl.l FrameByteWritePush
    plx
    lda.b #$00
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #!HDMAEN
    jsl.l FrameByteWritePush
    plx
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #!TSW
    jsl.l FrameByteWritePush
    plx
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #!TMW
    jsl.l FrameByteWritePush
    plx
    lda.b #$01
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #!CGADSUB
    jsl.l FrameByteWritePush
    plx
    lda.b #$02
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #!CGWSEL
    jsl.l FrameByteWritePush
    plx
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #!TS
    jsl.l FrameByteWritePush
    plx
    lda.b #$65
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #!BG2SC
    jsl.l FrameByteWritePush
    plx
    lda.b #$61
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #!BG1SC
    jsl.l FrameByteWritePush
    plx
    lda.b #$20
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #!BG12NBA
    jsl.l FrameByteWritePush
    plx
    lda.b #$17
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #!TM
    jsl.l FrameByteWritePush
    plx
    rep.b #$20
    ldx.w !actor_current
    lda.w #$0011
    ldy.w #$FFFF
    jsr.w MenuActorsSet10
    lda.w #$FFFF
    sta.b !actor_10,x
    lda.w #$D14A
    sta.b !actor_handler,x
    lda.w #$0040
    sta.b !actor_0a,x
    rts

; @routine 0B:D068 MenuDropAwayCheck
; WHAT IT DOES: Finishes the drop (MenuDropAwayFinish) once the kind $13
; object has gone idle (its handler is $CE89).
; HOW: MenuActorFind, then a compare of its +$08.
; Entry: M 16-bit, X 16-bit, D = $0000. Exit: not yet: A = that object's
; handler; X = the object; Y from the search; N, Z, C from the compare.
; Finished: as MenuDropAwayFinish.
%at($0BD068)
MenuDropAwayCheck:
    lda.w #$0013
    jsr.w MenuActorFind
    lda.b !actor_handler,x
    cmp.w #$CE89
    bne .lD07B
    ldx.w !actor_current
    jsr.w MenuDropAwayFinish
.lD07B:
    rts

; @routine 0B:B814 MenuSelectArrive
; WHAT IT DOES: The handler that brings the selection screen in from the
; side: the speed +$20 grows by 1 a frame and moves the position +$22 (it
; starts at -$100) toward 0, which sets BG1's horizontal scroll, the
; sliding sprites' heights ($18BF = position, $18C1 = position + $A8),
; $11ED and the byte at $7F:1354 (= $100 - position, at most $1FF). On
; arrival (position >= 0): sound $19, handler $B952 with +$0A = 0, and for
; the next vertical blank HDMA channels 5-7 on, window 1 at 32-136 on BG1,
; BG2 and (inverted) the colour window, BG1 on the sub screen and in both
; window masks, and colour math subtracting the fixed colour (white) from
; BG1 except inside the colour window; then the picture with the tile grid
; from $58 is drawn (MenuPictureDC8CGrid58) and $800 bytes of it queued to
; VRAM $6400.
; HOW: The scroll is queued as the position's high byte, then its low byte.
; MenuPictureDC8CGrid58 leaves X = $0700, so on arrival the store of 0
; meant for the position goes to $0722, and the scroll, heights and
; $7F:1354 are made from $0722 (0); the object's +$22 keeps its value.
; Entry: D = $0000; X = record. Exit: A = $100 - position, at most $1FF;
; X unchanged, or $0700 on arrival; N, Z from the compare with $1FF (or
; from $1FF); C from the compare; V from the subtraction; M 16-bit, X
; 16-bit.
%at($0BB814)
MenuSelectArrive:
    rep.b #$30
    inc.b !actor_20,x
    lda.b !actor_22,x
    clc
    adc.b !actor_20,x
    sta.b !actor_22,x
    bpl .lB824
    brl .lB908
.lB824:
    sep.b #$20
    lda.b #$19
    jsl.l SoundPort0QueuePush
    rep.b #$20
    lda.w #$B952
    sta.b !actor_handler,x
    stz.b !actor_0a,x
    sep.b #$20
    lda.b #$E0
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #!HDMAEN
    jsl.l FrameByteWritePush
    plx
    lda.b #$22
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #!W12SEL
    jsl.l FrameByteWritePush
    plx
    lda.b #$30
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #!WOBJSEL
    jsl.l FrameByteWritePush
    plx
    lda.b #$20
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #!WH0
    jsl.l FrameByteWritePush
    plx
    lda.b #$88
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #!WH1
    jsl.l FrameByteWritePush
    plx
    lda.b #$01
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #!TMW
    jsl.l FrameByteWritePush
    plx
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #!TSW
    jsl.l FrameByteWritePush
    plx
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #!TS
    jsl.l FrameByteWritePush
    plx
    lda.b #$81
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #!CGADSUB
    jsl.l FrameByteWritePush
    plx
    lda.b #$20
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #!CGWSEL
    jsl.l FrameByteWritePush
    plx
    lda.b #$FF
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #!COLDATA
    jsl.l FrameByteWritePush
    plx
    jsr.w MenuPictureDC8CGrid58
    rep.b #$30
    lda.w #$0800
    sta.l !vram_queue_staged
    lda.w #$0070
    sta.l !vram_queue_staged+4
    lda.w #$3000
    sta.l !vram_queue_staged+2
    lda.w #$6400
    sta.l !vram_queue_staged+6
    jsl.l VramDmaQueueAdd
    sep.b #$20
    rep.b #$10
    rep.b #$20
    lda.w #$0000
    sta.b !actor_22,x
.lB908:
    lda.b !actor_22,x
    pha
    sta.w !menu_slide_pos
    sta.l !menu_sine_y
    clc
    adc.w #$00A8
    sta.l !menu_sine_y_min
    pla
    sep.b #$20
    xba
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #!BG1HOFS
    jsl.l FrameByteWritePush
    plx
    xba
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #!BG1HOFS
    jsl.l FrameByteWritePush
    plx
    rep.b #$20
    lda.w #$0100
    sec
    sbc.b !actor_22,x
    cmp.w #$01FF
    bcc .lB949
    lda.w #$01FF
.lB949:
    sep.b #$20
    sta.l !menu_hdma_1354
    rep.b #$20
    rts

; @routine 0B:B506 MenuActorNew
; WHAT IT DOES: Takes a free object record (MenuActorAlloc) and fills it
; from the template at $0B:EA0F; X = $FFFF when none is free (no script
; runs out).
; HOW: 44 words, $0B:EA0F-$0B:EA66, stored through the data bank from the
; record on: $58 bytes, twice the record size, so the copy also runs over
; the next record. P is saved and restored.
; Entry: X 16-bit. Exit: A = the template's last word, or as
; MenuActorAlloc when none is free; X = the record or $FFFF; P unchanged.
%at($0BB506)
MenuActorNew:
    php
    rep.b #$30
    jsr.w MenuActorAlloc
    cpx.w #$FFFF
    bne .copy
    brl .done
.copy:
    lda.l !menu_actor_template+0
    sta.w 0,x
    lda.l !menu_actor_template+2
    sta.w 2,x
    lda.l !menu_actor_template+4
    sta.w 4,x
    lda.l !menu_actor_template+6
    sta.w 6,x
    lda.l !menu_actor_template+8
    sta.w 8,x
    lda.l !menu_actor_template+10
    sta.w 10,x
    lda.l !menu_actor_template+12
    sta.w 12,x
    lda.l !menu_actor_template+14
    sta.w 14,x
    lda.l !menu_actor_template+16
    sta.w 16,x
    lda.l !menu_actor_template+18
    sta.w 18,x
    lda.l !menu_actor_template+20
    sta.w 20,x
    lda.l !menu_actor_template+22
    sta.w 22,x
    lda.l !menu_actor_template+24
    sta.w 24,x
    lda.l !menu_actor_template+26
    sta.w 26,x
    lda.l !menu_actor_template+28
    sta.w 28,x
    lda.l !menu_actor_template+30
    sta.w 30,x
    lda.l !menu_actor_template+32
    sta.w 32,x
    lda.l !menu_actor_template+34
    sta.w 34,x
    lda.l !menu_actor_template+36
    sta.w 36,x
    lda.l !menu_actor_template+38
    sta.w 38,x
    lda.l !menu_actor_template+40
    sta.w 40,x
    lda.l !menu_actor_template+42
    sta.w 42,x
    lda.l !menu_actor_template+44
    sta.w 44,x
    lda.l !menu_actor_template+46
    sta.w 46,x
    lda.l !menu_actor_template+48
    sta.w 48,x
    lda.l !menu_actor_template+50
    sta.w 50,x
    lda.l !menu_actor_template+52
    sta.w 52,x
    lda.l !menu_actor_template+54
    sta.w 54,x
    lda.l !menu_actor_template+56
    sta.w 56,x
    lda.l !menu_actor_template+58
    sta.w 58,x
    lda.l !menu_actor_template+60
    sta.w 60,x
    lda.l !menu_actor_template+62
    sta.w 62,x
    lda.l !menu_actor_template+64
    sta.w 64,x
    lda.l !menu_actor_template+66
    sta.w 66,x
    lda.l !menu_actor_template+68
    sta.w 68,x
    lda.l !menu_actor_template+70
    sta.w 70,x
    lda.l !menu_actor_template+72
    sta.w 72,x
    lda.l !menu_actor_template+74
    sta.w 74,x
    lda.l !menu_actor_template+76
    sta.w 76,x
    lda.l !menu_actor_template+78
    sta.w 78,x
    lda.l !menu_actor_template+80
    sta.w 80,x
    lda.l !menu_actor_template+82
    sta.w 82,x
    lda.l !menu_actor_template+84
    sta.w 84,x
    lda.l !menu_actor_template+86
    sta.w 86,x
.done:
    plp
    rts

; @routine 0B:E4B5 MenuActorSpawn
; WHAT IT DOES: Makes a new object (MenuActorNew) and applies the spawn
; entry list at $0B:E547 + X to it: each entry is a field offset and a
; value, ended by a negative offset. X = $FFFF when no record is free (no
; script runs out).
; HOW: The record is kept at $1209; the field writes go through the data
; bank. The failure return ($E4E3, LDX #$FFFF) follows the routine.
; Entry: M 16-bit, X 16-bit; X = the entry list's offset. Exit: A = the
; terminating offset; X = the record; Y = the last field written (the list
; offset if none); N, Z from X; C, V from the last step of 4 (C clear if
; none). None free: A as MenuActorAlloc; X = $FFFF; Y = the list offset; N,
; C set, Z clear.
%at($0BE4B5)
MenuActorSpawn:
    txy
    jsr.w MenuActorNew
    cpx.w #$FFFF
    bne .lE4C1
    brl .none
.lE4C1:
    stx.w !menu_spawn_record
    tyx
.lE4C5:
    lda.l !menu_spawn_entries,x
    bmi .lE4DF
    clc
    adc.w !menu_spawn_record
    tay
    lda.l !menu_spawn_entries+2,x
    sta.w 0,y
    txa
    clc
    adc.w #$0004
    tax
    bra .lE4C5
.lE4DF:
    ldx.w !menu_spawn_record
    rts
.none:
    ldx.w #$FFFF
    rts

; @routine 0B:E458 MenuActorSpawnList
; WHAT IT DOES: Spawns every object of spawn list X: the word list at
; $0B:E4E7 + X names each object's entry list (MenuActorSpawn), ended by a
; negative word. A full object pool stops the game (SystemHaltSpawn); no
; script fills it.
; HOW: The position in the list is kept at $11FD; P is saved and restored.
; Entry: X 16-bit; X = the list's offset. Exit: A = the terminating word;
; X = $11FD (the terminator's offset); Y as the last MenuActorSpawn; P
; unchanged.
%at($0BE458)
MenuActorSpawnList:
    php
    rep.b #$30
    stx.w !menu_spawn_pos
.lE45E:
    ldx.w !menu_spawn_pos
    lda.l !menu_spawn_lists,x
    bmi .lE478
    tax
    jsr.w MenuActorSpawn
    cpx.w #$FFFF
    beq SystemHaltSpawn
    inc.w !menu_spawn_pos
    inc.w !menu_spawn_pos
    bra .lE45E
.lE478:
    plp
    rtl

; @routine 0B:D025 MenuSelectConfirm
; WHAT IT DOES: When B or Start has been pressed ($118B bits 15 and 12):
; sets +$10 of every kind $11 object to $FFFF, starts the kind $13 object
; (BG1) moving away (handler $CE8A, speed -10), moves the object being run
; on to MenuDropAwayCheck ($D068) with +$10 = $FFFF, and plays sound $46.
; HOW: MenuActorsSet10, MenuActorFind, then the sound with 8-bit A.
; Entry: M 16-bit, X 16-bit, D = $0000. Exit: not pressed: A = 0; Z set, N
; clear. Confirmed: A = $FF46; X = $11F9; Y from the search; N, Z from the
; data bank; C from the search; M 16-bit.
%at($0BD025)
MenuSelectConfirm:
    lda.w !menu_joy1_pressed
    and.w #$9000
    bne .lD030
    brl MenuSelectConfirmNone
.lD030:
    lda.w #$0011
    ldy.w #$FFFF
    jsr.w MenuActorsSet10
    lda.w #$0013
    jsr.w MenuActorFind
    lda.w #$CE8A
    sta.b !actor_handler,x
    lda.w #$FFF6
    sta.b !actor_22,x
    ldx.w !actor_current
    lda.w #$D068
    sta.b !actor_handler,x
    lda.w #$FFFF
    sta.b !actor_10,x
    sep.b #$20
    lda.b #$46
    jsl.l SoundPort0QueuePush
    rep.b #$20
    rts

; @routine 0B:D061 MenuDropAwayFinishCurrent
; WHAT IT DOES: MenuDropAwayFinish with X = the object being run. Nothing
; the scripts run reaches it: it sits after MenuSelectConfirm's last RTS
; and no traced jump or call targets it.
; HOW: LDX $11F9, then MenuDropAwayFinish.
; Entry: as MenuDropAwayFinish. Exit: as MenuDropAwayFinish.
%at($0BD061)
MenuDropAwayFinishCurrent:
    ldx.w !actor_current
    jsr.w MenuDropAwayFinish
    rts

; MenuSelectConfirm's return when nothing was pressed.
%at($0BD067)
MenuSelectConfirmNone:
    rts

; @routine 0B:B64A MenuOamBuild
; WHAT IT DOES: Builds the sprite table at $0375 (and its high table at
; $0575) from the menu objects, last live object first, relative to the
; scroll position ($11ED, $11EF); entries left over from the previous frame
; are moved off screen ($EFEF) and their high bits set to $5555.
; HOW: An object with +$10 = $FFFF is skipped. +$12 = 0 is a single
; sprite from the record (X +$02, Y +$06, tile +$0E | +$10, size bit 0 of
; +$0C); +$12 = $FFFF is skipped; otherwise +$12 is a sprite group at
; $7F:1356 (a count, then 8-byte entries: X, Y, tile and attributes, size
; in bit 0) placed at the object's position. A sprite is kept when X is
; -16 to 255 and Y -16 to 223; negative X sets its X high bit. Each sprite
; shifts its X high bit and size bit into $11E1 (two RORs each); every
; eighth sprite the word is stored; the last partial word is padded with
; X-high pairs. $1205 and $1207 count the bytes used in the two tables,
; $1201 and $1203 hold the previous frame's counts, $11DF the sprites
; left in the current high word. A full table ($200 bytes) ends the
; object. The single-sprite path, negative positions and a full table are
; not reached by any script. DB is 0 throughout.
; Entry: any widths. Exit: A = the high table's byte count; X = the last
; record, group offset or entry offset used; Y = the end of the high
; table fill; P unchanged.
%at($0BB64A)
MenuOamBuild:
    php
    sep.b #$20
    phb
    lda.b #$00
    pha
    plb
    rep.b #$30
    stz.w !oam_low_count
    stz.w !oam_high_count
    lda.w #$0007
    sta.w !scratch_11df
    stz.w !scratch_11e1
    ldy.w !actor_list_len
    dey
    dey
    bpl .lB66D
    brl .lB7BF
.lB66D:
    sty.w !actor_current_slot
    ldx.w !actor_list,y
    lda.b !actor_10,x
    inc a
    bne .lB67B
    brl .lB7B5
.lB67B:
    lda.b !actor_12,x
    beq .lB68A
    cmp.w #$FFFF
    bne .lB687
    brl .lB7B5
.lB687:
    jmp.w .lB70E
.lB68A:
    stz.w !oam_x_high
    lda.b !actor_0c,x
    sta.w !oam_size
    ldy.w !oam_low_count
    cpy.w #$0200
    bne .lB69D
    brl .lB7B5
.lB69D:
    lda.b !actor_02,x
    sec
    sbc.w !menu_slide_pos
    bpl .lB6B2
    cmp.w #$FFF0
    bcs .lB6AD
    brl .lB7B5
.lB6AD:
    inc.w !oam_x_high
    bra .lB6BA
.lB6B2:
    cmp.w #$0100
    bcc .lB6BA
    brl .lB7B5
.lB6BA:
    sta.w !oam_buffer,y
    lda.b !actor_06,x
    sec
    sbc.w !menu_scroll_y
    bpl .lB6CD
    cmp.w #$FFF0
    bcs .lB6D5
    jmp.w .lB7B5
.lB6CD:
    cmp.w #$00E0
    bcc .lB6D5
    brl .lB7B5
.lB6D5:
    sta.w !oam_buffer+1,y
    lda.b !actor_0e,x
    ora.b !actor_10,x
    sta.w !oam_buffer+2,y
    iny
    iny
    iny
    iny
    sty.w !oam_low_count
    ror.w !oam_x_high
    ror.w !scratch_11e1
    ror.w !oam_size
    ror.w !scratch_11e1
    dec.w !scratch_11df
    bpl .lB70B
    lda.w #$0007
    sta.w !scratch_11df
    ldy.w !oam_high_count
    lda.w !scratch_11e1
    sta.w !oam_buffer_high,y
    iny
    iny
    sty.w !oam_high_count
.lB70B:
    jmp.w .lB7B5
.lB70E:
    sta.w !scratch_11e3
    lda.b !actor_02,x
    sec
    sbc.w !menu_slide_pos
    sta.w !oam_group_x
    lda.b !actor_06,x
    sec
    sbc.w !menu_scroll_y
    sta.w !oam_group_y
    ldx.w !scratch_11e3
    lda.l !menu_sprite_groups,x
    sta.w !scratch_11e3
    inx
    inx
.lB72F:
    stz.w !oam_x_high
    ldy.w !oam_low_count
    cpy.w #$0200
    bne .lB73D
    brl .lB7B5
.lB73D:
    lda.l !menu_sprite_groups,x
    clc
    adc.w !oam_group_x
    bpl .lB751
    cmp.w #$FFF0
    bcc .lB7A7
    inc.w !oam_x_high
    bra .lB756
.lB751:
    cmp.w #$0100
    bcs .lB7A7
.lB756:
    sta.w !oam_buffer,y
    lda.l !menu_sprite_groups+2,x
    clc
    adc.w !oam_group_y
    bpl .lB76A
    cmp.w #$FFF0
    bcs .lB76F
    bra .lB7A7
.lB76A:
    cmp.w #$00E0
    bcs .lB7A7
.lB76F:
    sta.w !oam_buffer+1,y
    iny
    iny
    lda.l !menu_sprite_attr,x
    sta.w !oam_buffer,y
    iny
    iny
    sty.w !oam_low_count
    lsr.w !oam_x_high
    ror.w !scratch_11e1
    lda.l !menu_sprite_attr+2,x
    lsr a
    ror.w !scratch_11e1
    dec.w !scratch_11df
    bpl .lB7A7
    ldy.w !oam_high_count
    lda.w !scratch_11e1
    sta.w !oam_buffer_high,y
    iny
    iny
    sty.w !oam_high_count
    lda.w #$0007
    sta.w !scratch_11df
.lB7A7:
    txa
    clc
    adc.w #$0008
    tax
    dec.w !scratch_11e3
    bmi .lB7B5
    brl .lB72F
.lB7B5:
    ldy.w !actor_current_slot
    dey
    dey
    bmi .lB7BF
    brl .lB66D
.lB7BF:
    ldy.w !scratch_11df
    cpy.w #$0007
    beq .lB7DB
    lda.w !scratch_11e1
.lB7CA:
    sec
    ror a
    lsr a
    dey
    bpl .lB7CA
    ldy.w !oam_high_count
    sta.w !oam_buffer_high,y
    iny
    iny
    sty.w !oam_high_count
.lB7DB:
    ldy.w !oam_low_count
    cpy.w !menu_1201
    bcs .lB7F0
    lda.w #$EFEF
.lB7E6:
    sta.w !oam_buffer,y
    iny
    iny
    cpy.w !menu_1201
    bcc .lB7E6
.lB7F0:
    lda.w !oam_low_count
    sta.w !menu_1201
    ldy.w !oam_high_count
    cpy.w !menu_1203
    bcs .lB80B
    lda.w #$5555
.lB801:
    sta.w !oam_buffer_high,y
    iny
    iny
    cpy.w !menu_1203
    bcc .lB801
.lB80B:
    lda.w !oam_high_count
    sta.w !menu_1203
    plb
    plp
    rtl

; @routine 0B:B450 MenuActorsRun
; WHAT IT DOES: Runs every live menu object's handler once, from the last
; in the live list to the first.
; HOW: With DB = 0: $11FB = the list offset and $11F9 = the record, then
; JSR (+$08) in bank $0B, the handler running with X = the record and Y =
; the offset; the offset is reloaded from $11FB afterwards, so a handler
; may free its object. P and DB are saved and restored.
; Entry: any widths. Exit: A, X as the last handler left them (A's low
; byte 0 when there are none); Y = the last offset - 2; P and DB
; unchanged.
%at($0BB450)
MenuActorsRun:
    php
    phb
    sep.b #$20
    rep.b #$10
    lda.b #$00
    pha
    plb
    rep.b #$20
    ldy.w !actor_list_len
    dey
    dey
    bmi .done
.next:
    sty.w !actor_current_slot
    ldx.w !actor_list,y
    stx.w !actor_current
    jsr.w (!actor_handler,x)
    ldy.w !actor_current_slot
    dey
    dey
    bpl .next
.done:
    plb
    plp
    rtl
