; draw_list.asm: the list of objects the Super FX draws this frame, built in
; work RAM from the objects in the buckets the trigger list names.
; C: src/stuntracefx/object/draw_list.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 7E:EF50 ObjectDrawListBuild
; WHAT IT DOES: Builds the list of objects to draw: for each trigger (two
; words at $1AF5, 0 ends the list), walks the objects of the bucket its
; second word names ($7F:0000 + the word AND $7FE, linked by +$3C) and,
; for each one drawn (not hidden by bit 5 of +$24; with bit 6, kind 2 only
; on the near side and the others only on the far side, $1F07; at most
; $31; never model $A040), writes a 30-byte draw record from $7E:F77F:
; position relative to the view and a depth key, model, angles, link and
; frame fields, flags (clearing bit 1 of +$24) and palette bytes. $0CEE
; counts the records; $034C holds the object being looked at.
; HOW: Data bank $7E; the record address is kept in direct page $3E
; between buckets, the trigger position in $1F2B. The test of bit 6 for
; kind-2 objects is not reached in the traces so far.
; Entry: D = $0000. Exit: A = 0; X = the trigger position; Y = the last
; object looked at (0 after a bucket's end); P and the data bank unchanged.
%ramcode($7EEF50, $014C77)
ObjectDrawListBuild:
    php
    phb
    sep.b #$20
    lda.b #$7E
    pha
    plb
    rep.b #$30
    stz.w !draw_count
    stz.w !trigger_pos
    ldx.w #$F77F
    stx.b !draw_next
.next_trigger:
    ldx.w !trigger_pos
    lda.w !group_trigger_list,x
    bne .trigger
    brl .done
.trigger:
    sta.w !trigger_word
    lda.w !group_trigger_list+2,x
    inx
    inx
    inx
    inx
    stx.w !trigger_pos
    and.w #$07FE
    tax
    lda.l !object_buckets,x
    beq .next_trigger
    tay
    ldx.b !draw_next
.object:
    sep.b #$20
    sty.w !draw_object
    lda.w !obj_24,y
    bit.b #$20
    bne .skip
    bit.b #$40
    beq .draw
    lda.w !obj_26,y
    cmp.b #$02
    beq .kind2
    lda.w !draw_side
    bne .draw
    bra .skip
.kind2:
    lda.w !draw_side                ; not reached in the traces so far
    beq .draw                      ; not reached in the traces so far
.skip:
    jmp.w .next_object
.draw:
    rep.b #$20
    lda.w !draw_count
    cmp.w #$0031
    bmi .count_ok
    brl .next_object
.count_ok:
    lda.w !obj_04,y
    cmp.w #$A040
    bne .model_ok
    brl .next_object
.model_ok:
    lda.w !obj_24,y
    bit.w #$0080
    beq .relative
    lda.w !obj_pos_x,y
    sta.w !draw_x,x
    lda.w !obj_pos_y,y
    sta.w !draw_y,x
    lda.w !obj_pos_z,y
    sta.w !draw_z,x
    lda.w #$F060
    sta.w !draw_depth,x
    bra .fields
.relative:
    lda.w !obj_pos_y,y
    sec
    sbc.b !dp_c7
    sta.w !draw_y,x
    bpl .depth
    eor.w #$FFFF
    inc a
.depth:
    asl a
    sta.w !draw_depth,x
    lda.w !obj_09,y
    bit.w #$0001
    beq .near
    lda.w #$1F40
    bra .depth_add
.near:
    lda.w #$0000
.depth_add:
    clc
    adc.w !draw_depth,x
    sta.w !draw_depth,x
    lda.w !obj_pos_x,y
    sec
    sbc.b !dp_c5
    sta.w !draw_x,x
    lda.w !obj_pos_z,y
    sec
    sbc.b !dp_c9
    sta.w !draw_z,x
.fields:
    lda.w !obj_04,y
    sta.w !draw_model,x
    lda.w !obj_14,y
    sta.w !draw_angle_a,x
    lda.w !obj_15,y
    sta.w !draw_angle_b,x
    lda.w !obj_16,y
    sta.w !draw_angle_c,x
    lda.w !obj_24,y
    bit.w #$0004
    beq .no_link
    rep.b #$20
    tya
    sta.w !draw_link,x
    sep.b #$20
    lda.w !obj_48,y
    sta.w !draw_link_48,x
    lda.w !obj_49,y
    sta.w !draw_link_49,x
    lda.w !obj_4a,y
    sta.w !draw_link_4a,x
    bra .flags
.no_link:
    sep.b #$20
    lda.b #$00
    sta.w !draw_no_link,x
.flags:
    lda.w !obj_24,y
    sta.w !draw_flags,x
    and.b #$FD
    sta.w !obj_24,y
    lda.w !obj_5a,y
    bmi .palette
    lda.w !wram_05E9
.palette:
    and.b #$7F
    sta.w !draw_palette,x
    lda.w !draw_side
    beq .near_frame
    lda.w !obj_19,y
    bmi .frame
    lda.w !wram_05E9
    bra .frame
.near_frame:
    lda.w !obj_18,y
    bmi .frame
    lda.w !wram_05E9
.frame:
    and.b #$7F
    sta.w !draw_frame,x
    lda.w !obj_3a,y
    sta.w !draw_kind,x
    lda.w !obj_5f,y
    sta.w !draw_5f,x
    lda.w !obj_60,y
    sta.w !draw_60,x
    rep.b #$20
    lda.w !obj_5d,y
    sta.w !draw_anim,x
    txa
    clc
    adc.w #$001E
    tax
    inc.w !draw_count
.next_object:
    rep.b #$20
    lda.w !obj_bucket_next,y
    tay
    beq .bucket_end
    brl .object
.bucket_end:
    stx.b !draw_next
    jmp.w .next_trigger
.done:
    plb
    plp
    rts

; @routine 7E:F0D3 ObjectDrawListClear08
; WHAT IT DOES: An arm after ObjectDrawListBuild's RTS that nothing
; reaches: clears bit 1 of object Y's +$08, then goes on with
; ObjectDrawListBuild's walk from the object after it in its bucket (+$3C)
; and the rest of the trigger list ($1F2B), X the next draw record. A
; direct call is its only way in.
; HOW: SEP #$20 for the AND #$FD, REP #$20, BRA into the bucket loop
; (7E:F0C0); ends with ObjectDrawListBuild's PLB, PLP, RTS.
; Entry: M 16-bit, X 16-bit, D = $0000, data bank $7E; Y = object; X = the
; next draw record; the caller's P and data bank pushed above the return
; (PHP, PHB). Exit: as ObjectDrawListBuild, P and the data bank those
; pushed.
%ramcode($7EF0D3, $014DFA)
ObjectDrawListClear08:
    sep.b #$20
    lda.w !obj_08,y
    and.b #$FD
    sta.w !obj_08,y
    rep.b #$20
    bra ObjectDrawListBuild_next_object
