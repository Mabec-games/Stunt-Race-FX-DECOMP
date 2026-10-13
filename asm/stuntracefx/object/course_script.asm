; course_script.asm: the course script, the byte-coded list of objects and
; settings a course starts with, run once when the course is set up.
; C: src/stuntracefx/object/course_script.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 08:801C CourseScriptRun
; WHAT IT DOES: Runs the course script at X in bank A (read at $8000 + X):
; each entry starts with an opcode byte, the offset of its handler in the
; table at $08:8000. $02 spawns an object from a $16-byte entry
; (ObjectSpawnFromScript); $0C does the same and sets bit 0 of +$09; $10
; spawns from a $18-byte entry with +$27 from the entry and +$29 from a
; count at $1ADE; $0A spawns from a $19-byte entry with +$48-$4A from it
; (and +$5A = +$48 OR $80); $0E spawns from a $1A-byte entry (unless HUD
; mode 3 and bits 2-4 of its flags byte), keeping it as player 0's ($0346)
; or player 1's ($034A) object by bits 0 and 1, with +$36 = 4, +$15 and
; +$27 from the entry, and adds it to the list at $7E:FFAE (at most 4); $06
; empties the buckets and resets the object state (ObjectPoolInit); $08
; sets $1AF4; $12 writes a 7-byte record into $7E:FF66 + (n - 1) * 8; $14
; ORs a word into $1AEC (and $70:26F2); $1A sets $1AEE; $16 and $18 make
; bare objects kept at $1ADA and $1ADC; $04 ends it, leaving the address
; after it ($8000 + X) and the bank at $70:1092-$70:1094. ($00 starts
; over at offset 0.)
; HOW: The data bank is the script bank throughout; JMP (table,X) by the
; opcode, each handler going back to the loop with X past its entry.
; Opcode $10 and the no-free-object stops are not reached in the traces
; so far.
; Entry: M 8-bit, X 16-bit, D = $0000; A = script bank (PHA, PLB), X =
; script offset. Exit: A = the script bank; X = 4; Y = the offset after
; the end opcode; M 8-bit; N, Z from the data bank pulled back; C, V from
; adding $8000.
%at($08801C)
CourseScriptRun:
    phb
    pha
    plb
.next:
    rep.b #$20
    lda.w !script_entry,x
    and.w #$00FF
    txy
    tax
    jmp.w (!course_script_ops,x)
.op_end:
    iny
    rep.b #$20
    tya
    clc
    adc.w #!script_entry
    sta.l !gsu_script_next
    sep.b #$20
    phb
    pla
    rep.b #$20
    and.w #$00FF
    sta.l !gsu_script_bank
    sep.b #$20
    plb
    rtl
.op_spawn:
    jsr.w ObjectSpawnFromScript
    tya
    clc
    adc.w #$0016
    tax
    jmp.w .next
.op_spawn_flag:
    jsr.w ObjectSpawnFromScript
    sep.b #$20
    lda.l !obj_09_long,x
    ora.b #$01
    sta.l !obj_09_long,x
    rep.b #$20
    tya
    clc
    adc.w #$0016
    tax
    jmp.w .next
.op_spawn_counted:                  ; not reached in the traces so far
    jsr.w ObjectSpawnFromScript
    rep.b #$20
    lda.w !script_entry+22,y
    sta.l !obj_27_long,x
    lda.w !spawn_counter
    inc a
    sta.w !spawn_counter
    sta.l !obj_29_long,x
    tya
    clc
    adc.w #$0018
    tax
    jmp.w .next
.op_spawn_48:
    jsr.w ObjectSpawnFromScript
    sep.b #$20
    lda.w !script_entry+22,y
    sta.l !obj_48_long,x
    ora.b #$80
    sta.l !obj_5a_long,x
    lda.w !script_entry+23,y
    sta.l !obj_49_long,x
    lda.w !script_entry+24,y
    sta.l !obj_4a_long,x
    rep.b #$20
    tya
    clc
    adc.w #$0019
    tax
    jmp.w .next
.op_reset:
    phy
    rep.b #$20
    ldx.w #$0000
    ldy.w #$0400
    lda.w #$0000
.clear_buckets:
    sta.l !object_buckets,x
    inx
    inx
    dey
    bne .clear_buckets
    sep.b #$20
    lda.b #$00
    sta.l !spawn_mirror
    rep.b #$20
    stz.w !spawn_counter
    lda.w #$0000
    sta.l !object_list_b_len
    sta.l !object_list_len
    sta.l !gsu_control_26f2
    sta.l !script_flags_long
    lda.w !course
    and.w #$00FF
    sta.l !gsu_control_26f2+2
    stz.w !draw_count
    stz.w !wram_1AEE
    sep.b #$20
    stz.w !script_byte_1f71
    stz.w !script_byte_1f72
    rep.b #$20
    jsl.l ObjectPoolInit
    plx
    inx
    jmp.w .next
.op_mirror:
    sep.b #$20
    lda.w !script_entry+1,y
    sta.l !spawn_mirror
    rep.b #$20
    tya
    clc
    adc.w #$0002
    tax
    jmp.w .next
.op_player:
    lda.w !hud_mode
    cmp.w #$0003
    bne .player_spawn
    lda.w !script_entry+24,y
    and.w #$001C
    beq .player_spawn
    brl .player_next
.player_spawn:
    jsr.w ObjectSpawnFromScript
    sep.b #$20
    lda.w !script_entry+24,y
    and.b #$01
    beq .not_p0
    stx.w !wram_0346
.not_p0:
    lda.w !script_entry+24,y
    and.b #$02
    beq .not_p1
    stx.w !wram_034A
.not_p1:
    lda.b #$04
    sta.l !obj_36_long,x
    lda.w !script_entry+25,y
    sta.l !obj_15_long,x
    rep.b #$20
    lda.w !script_entry+22,y
    sta.l !obj_27_long,x
    phx
    phy
    txy
    lda.l !object_list_b_len
    tax
    cmp.w #$0008
    beq .list_full
    inc a
    inc a
    sta.l !object_list_b_len
    tya
    sta.l !object_list_b,x
    inx
    inx
    lda.w #$0000
    sta.l !object_list_b,x
.list_full:
    ply
    plx
.player_next:
    rep.b #$20
    tya
    clc
    adc.w #$001A
    tax
    jmp.w .next
.op_record:
    rep.b #$20
    lda.w !script_entry+1,y
    and.w #$00FF
    dec a
    asl a
    asl a
    asl a
    tax
    lda.w !script_entry+2,y
    sta.l !script_records,x
    lda.w !script_entry+4,y
    sta.l !script_records+2,x
    lda.w !script_entry+6,y
    sta.l !script_records+4,x
    sep.b #$20
    lda.w !script_entry+8,y
    sta.l !script_records+6,x
    rep.b #$20
    tya
    clc
    adc.w #$0009
    tax
    jmp.w .next
.op_flags:
    rep.b #$20
    lda.l !script_flags_long
    ora.w !script_entry+1,y
    sta.l !gsu_control_26f2
    sta.l !script_flags_long
    tya
    clc
    adc.w #$0003
    tax
    jmp.w .next
.op_1aee:
    rep.b #$20
    lda.w !script_entry+1,y
    sta.l !wram_1AEE_long
    tya
    clc
    adc.w #$0003
    tax
    jmp.w .next
.op_bare_1ada:
    phy
    sep.b #$20
    jsl.l ObjectAlloc
    bcs .bare_1ada
    brl ObjectSpawnFailed           ; not reached in the traces so far
.bare_1ada:
    txy
    jsl.l ObjectInit
    ply
    stx.w !bare_object_a
    lda.b #$03
    sta.l !obj_36_long,x
    rep.b #$20
    lda.w #$1A64
    sta.l !obj_link,x
    lda.w !script_entry+1,y
    sta.l !obj_1a,x
    lda.w !script_entry+2,y
    sta.l !obj_1b_long,x
    tya
    clc
    adc.w #$0004
    tax
    jmp.w .next
.op_bare_1adc:
    phy
    sep.b #$20
    jsl.l ObjectAlloc
    bcs .bare_1adc
    brl ObjectSpawnFailed           ; not reached in the traces so far
.bare_1adc:
    txy
    jsl.l ObjectInit
    ply
    stx.w !bare_object_b
    lda.b #$03
    sta.l !obj_36_long,x
    rep.b #$20
    lda.w #$1A9F
    sta.l !obj_link,x
    lda.w !script_entry+1,y
    sta.l !obj_1a,x
    lda.w !script_entry+2,y
    sta.l !obj_1b_long,x
    tya
    clc
    adc.w #$0004
    tax
    jmp.w .next
