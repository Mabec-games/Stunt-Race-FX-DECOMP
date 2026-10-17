; lamps_08.asm: course object set-ups of bank $08 no course in the scripts
; places: break-off pieces, lamps, a lamp stack and a falling object.
; C: src/stuntracefx/object/lamps_08.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 08:D61B ObjectBreakOffBurst15
; WHAT IT DOES: Makes fifteen break-off pieces (kind $A1AC) at object X's
; position, each with X as its parent (+$06) and the handler $08:D337
; (ObjectBreakOffContact); a spawn that fails only skips that piece.
; HOW: The count kept on the stack (PHA, PLA, 8-bit) across ObjectSpawn
; and ObjectCopyPositionTo; A's low byte is pulled back between the
; parent's store (TXA, 16-bit) and the handler's.
; Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; A low byte = 0, Z
; set.
%at($08D61B)
ObjectBreakOffBurst15:
    lda.b #$0F
.lD61D:
    pha
    rep.b #$20
    lda.w #$A1AC
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .lD632
    jml.l .lD64F
.lD632:
    jsl.l ObjectCopyPositionTo
    pha
    rep.b #$20
    txa
    sta.w !obj_06,y
    sep.b #$20
    pla
    rep.b #$20
    lda.w #$D337
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,y
.lD64F:
    pla
    dec a
    bne .lD61D
    rtl

; @routine 08:D654 ObjectLampPair08
; WHAT IT DOES: Sets up a lamp: object X $78 higher ($FF88 on +$0E), lit
; by $9971 (+$24 bit 3, +$3A = 0, +$5F = $28); and, when one can be made,
; a second light (kind $A078, $5D = $9973, +$5F = $50) at its position
; $140 higher; then no handler.
; HOW: ObjectSpawn, ObjectCopyPositionTo; the second light is not
; rebucketed.
; Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; A low byte = 0.
%at($08D654)
ObjectLampPair08:
    rep.b #$20
    lda.w !obj_pos_y,x
    clc
    adc.w #$FF88
    sta.w !obj_pos_y,x
    sep.b #$20
    lda.w !obj_24,x
    ora.b #$08
    sta.w !obj_24,x
    lda.b #$00
    sta.w !obj_3a,x
    lda.b #$28
    sta.w !obj_5f,x
    rep.b #$20
    lda.w #$9971
    sta.w !obj_5d,x
    sep.b #$20
    rep.b #$20
    lda.w #$A078
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .lD692
    jml.l .lD6C0
.lD692:
    lda.w !obj_24,y
    ora.b #$08
    sta.w !obj_24,y
    lda.b #$00
    sta.w !obj_3a,y
    lda.b #$50
    sta.w !obj_5f,y
    rep.b #$20
    lda.w #$9973
    sta.w !obj_5d,y
    sep.b #$20
    jsl.l ObjectCopyPositionTo
    rep.b #$20
    lda.w !obj_pos_y,y
    clc
    adc.w #$FEC0
    sta.w !obj_pos_y,y
    sep.b #$20
.lD6C0:
    rep.b #$20
    lda.w #$0000
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$00
    sta.w !obj_1c,x
    rtl

; @routine 08:D6D0 ObjectLampStack
; WHAT IT DOES: Sets up a stack of three lights lit by $9975: object X
; $46 higher ($FFBA on +$0E, +$5F = $1E) and two more (kind $A104, +$5F =
; $32) at its position $2BC and $578 higher, each made only when an
; object is free; then no handler.
; HOW: ObjectSpawn, ObjectCopyPositionTo; each light rebucketed with X
; and Y swapped round ObjectRebucket.
; Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; A low byte = 0.
%at($08D6D0)
ObjectLampStack:
    rep.b #$20
    lda.w !obj_pos_y,x
    clc
    adc.w #$FFBA
    sta.w !obj_pos_y,x
    sep.b #$20
    rep.b #$20
    lda.w #$9975
    sta.w !obj_5d,x
    sep.b #$20
    lda.w !obj_24,x
    ora.b #$08
    sta.w !obj_24,x
    lda.b #$00
    sta.w !obj_3a,x
    lda.b #$1E
    sta.w !obj_5f,x
    rep.b #$20
    lda.w #$A104
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .lD70E
    jml.l .lD746
.lD70E:
    lda.w !obj_24,y
    ora.b #$08
    sta.w !obj_24,y
    lda.b #$00
    sta.w !obj_3a,y
    lda.b #$32
    sta.w !obj_5f,y
    rep.b #$20
    lda.w #$9975
    sta.w !obj_5d,y
    sep.b #$20
    jsl.l ObjectCopyPositionTo
    rep.b #$20
    lda.w !obj_pos_y,y
    clc
    adc.w #$FD44
    sta.w !obj_pos_y,y
    sep.b #$20
    phx
    tyx
    ply
    jsl.l ObjectRebucket
    phx
    tyx
    ply
.lD746:
    rep.b #$20
    lda.w #$A104
    sta.w !wram_1F2B
    sep.b #$20
    jsl.l ObjectSpawn
    bcs .lD75A
    jml.l .lD792
.lD75A:
    lda.w !obj_24,y
    ora.b #$08
    sta.w !obj_24,y
    lda.b #$00
    sta.w !obj_3a,y
    lda.b #$32
    sta.w !obj_5f,y
    rep.b #$20
    lda.w #$9975
    sta.w !obj_5d,y
    sep.b #$20
    jsl.l ObjectCopyPositionTo
    rep.b #$20
    lda.w !obj_pos_y,y
    clc
    adc.w #$FA88
    sta.w !obj_pos_y,y
    sep.b #$20
    phx
    tyx
    ply
    jsl.l ObjectRebucket
    phx
    tyx
    ply
.lD792:
    rep.b #$20
    lda.w #$0000
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$00
    sta.w !obj_1c,x
    rtl

; @routine 08:D7A2 ObjectLampLow08
; WHAT IT DOES: Sets up a lamp lit by $9B8F: object X $1E higher ($FFE2
; on +$0E), +$5F = $14; then no handler.
; HOW: Word and byte stores; BRA to the shared tail at $08:D81B.
; Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; A low byte = 0.
%at($08D7A2)
ObjectLampLow08:
    rep.b #$20
    lda.w #$9B8F
    sta.w !obj_5d,x
    sep.b #$20
    rep.b #$20
    lda.w !obj_pos_y,x
    clc
    adc.w #$FFE2
    sta.w !obj_pos_y,x
    sep.b #$20
    bra ObjectFallerStep_lD81B

; @routine 08:D7BC ObjectFaller
; WHAT IT DOES: Sets up a falling object: $12C higher, +$1D = $08:D808,
; lit by its own light (+$24 bit 3, +$3A = 0, +$5F = $14), Y speed -$64,
; handler ObjectFallerStep; then ObjectFallerRecord.
; HOW: BRA to the set-up at $08:D7C0.
; Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectSyncFromRecord.
%at($08D7BC)
ObjectFaller:
    bra ObjectFallerB_lD7C0

; @routine 08:D7BE ObjectFallerB
; WHAT IT DOES: As ObjectFaller (a second entry for another kind).
; HOW: BRA to the set-up at $08:D7C0.
; Entry: as ObjectFaller. Exit: as ObjectFaller.
%at($08D7BE)
ObjectFallerB:
    bra .lD7C0
.lD7C0:
    rep.b #$20
    lda.w !obj_pos_y,x
    clc
    adc.w #$FED4
    sta.w !obj_pos_y,x
    sep.b #$20
    rep.b #$20
    lda.w #$D808
    sta.w !obj_1d,x
    sep.b #$20
    lda.b #$08
    sta.w !obj_1f,x
    lda.w !obj_24,x
    ora.b #$08
    sta.w !obj_24,x
    lda.b #$00
    sta.w !obj_3a,x
    lda.b #$14
    sta.w !obj_5f,x
    rep.b #$20
    lda.w #$FF9C
    sta.w !obj_vel_y,x
    sep.b #$20
    rep.b #$20
    lda.w #$D816
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,x

; @routine 08:D808 ObjectFallerRecord
; WHAT IT DOES: Gives the falling object its Super FX record
; (GsuRecordNew424) with flag $10 and $20 in the high byte of +$2A, then
; its first step (ObjectFallerStep). Also its +$1D pointer.
; HOW: GsuRecordNew424, GsuRecordFlag10, LDA #$20, GsuRecordSet2AHigh;
; falls into ObjectFallerStep. A failed record is not checked.
; Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectSyncFromRecord.
%at($08D808)
ObjectFallerRecord:
    jsl.l GsuRecordNew424
    jsl.l GsuRecordFlag10
    lda.b #$20
    jsl.l GsuRecordSet2AHigh

; @routine 08:D816 ObjectFallerStep
; WHAT IT DOES: The falling object's handler: the object follows its Super
; FX record (ObjectSyncFromRecord).
; HOW: JSL ObjectSyncFromRecord.
; Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectSyncFromRecord.
%at($08D816)
ObjectFallerStep:
    jsl.l ObjectSyncFromRecord
    rtl
.lD81B:
    lda.w !obj_24,x
    ora.b #$08
    sta.w !obj_24,x
    lda.b #$00
    sta.w !obj_3a,x
    lda.b #$14
    sta.w !obj_5f,x
    rep.b #$20
    lda.w #$0000
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$00
    sta.w !obj_1c,x
    rtl
