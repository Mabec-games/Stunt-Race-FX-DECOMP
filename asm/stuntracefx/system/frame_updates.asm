; frame_updates.asm: updates the game logic asks for and the menu frame code
; applies in the next vertical blank. Bits of $11D7: 0 byte writes, 2 sprite
; table, 3 palette. Runs from work RAM bank $7F.
; C: src/stuntracefx/system/frame_updates.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"

arch 65816

; @routine 7F:0EC9 OamDmaIfPending
; WHAT IT DOES: Copies the whole sprite table to OAM if an update is pending.
; HOW: With bit 2 of the update flags set: channel 0 in mode 0 from $00:0375
; to OAMDATA for $220 bytes from OAM address 0; then clears the bit.
; Entry: X 16-bit. Exit: A low byte = the flags after clearing, or 0 when
; nothing was pending; X = 0 after a copy; P unchanged.
%ramcode($7F0EC9, $0586C9)
OamDmaIfPending:
    php
    sep.b #$20
    rep.b #$10
    lda.w !frame_update_flags
    and.b #$04
    beq .done
    stz.w !DMAP0
    lda.b #$04
    sta.w !DMAP0+1
    ldx.w #!oam_buffer
    stx.w !A1T0L
    lda.b #$00
    sta.w !A1B0
    ldx.w #$0220
    stx.w !DAS0L
    ldx.w #$0000
    stx.w !OAMADDL
    lda.b #$01
    sta.w !MDMAEN
    lda.w !frame_update_flags
    and.b #$FB
    sta.w !frame_update_flags
.done:
    plp
    rts

; @routine 7F:0F03 CgramDmaIfPending
; WHAT IT DOES: Copies the whole palette buffer to the palette if an update
; is pending.
; HOW: With bit 3 of the update flags set: channel 0 in mode 0 from $7E:FD65
; to CGDATA for $200 bytes from colour 0; then clears the bit.
; Entry: X 16-bit. Exit: A low byte = the flags after clearing, or 0 when
; nothing was pending; Y = $0200 after a copy; P unchanged.
CgramDmaIfPending:
    php
    sep.b #$20
    rep.b #$10
    lda.w !frame_update_flags
    and.b #$08
    beq .done
    stz.w !DMAP0
    lda.b #$22
    sta.w !DMAP0+1
    ldy.w #$FD65
    sty.w !A1T0L
    lda.b #$7E
    sta.w !A1B0
    ldy.w #$0200
    sty.w !DAS0L
    stz.w !CGADD
    lda.b #$01
    sta.w !MDMAEN
    lda.w !frame_update_flags
    and.b #$F7
    sta.w !frame_update_flags
.done:
    plp
    rts

; @routine 7F:0F3A ByteWritesIfPending
; WHAT IT DOES: Applies the queued byte writes if any are pending.
; HOW: With bit 0 of the update flags set: walks the list from its last entry
; to its first (16-bit address, 16-bit value), storing each value's low byte
; at its address in the data bank; then empties the list and clears the bit.
; Entry: X 16-bit; DB the bank the addresses are in. Exit: A low byte = the
; flags after clearing, or 0 when nothing was pending; P unchanged.
ByteWritesIfPending:
    php
    sep.b #$20
    rep.b #$10
    lda.w !frame_update_flags
    and.b #$01
    beq .done
    rep.b #$20
    lda.w !byte_writes_size
    sec
    sbc.w #$0004
    bmi .empty
    tax
.next:
    lda.l !byte_writes,x
    tay
    lda.l !byte_writes+2,x
    sep.b #$20
    sta.w 0,y
    rep.b #$20
    dex
    dex
    dex
    dex
    bpl .next
.empty:
    stz.w !byte_writes_size
    sep.b #$20
    lda.w !frame_update_flags
    and.b #$FE
    sta.w !frame_update_flags
.done:
    plp
    rts

; @routine 7F:0F77 VramDmaQueueFlush
; WHAT IT DOES: Runs the queued VRAM transfers if any are pending.
; HOW: With bit 1 of the update flags set: VRAM increments after the high
; byte and channel 0 targets VMDATA; then for each queued transfer, last
; first, sets the source address, VRAM address, count and bank and starts it
; (mode 1); then empties the queue and clears the bit.
; Entry: X 16-bit. Exit: A low byte = the flags after clearing, or 0 when
; nothing was pending; P unchanged.
VramDmaQueueFlush:
    php
    sep.b #$20
    rep.b #$10
    lda.w !frame_update_flags
    and.b #$02
    beq .done
    lda.b #$80
    sta.w !VMAIN
    lda.b #$18
    sta.w !DMAP0+1
    rep.b #$20
    lda.w !vram_queue_size
    sec
    sbc.w #$0008
    bmi .empty
    tax
.next:
    rep.b #$20
    lda.l !vram_queue,x
    sta.w !A1T0L
    lda.l !vram_queue+3,x
    sta.w !VMADDL
    lda.l !vram_queue+5,x
    sta.w !DAS0L
    sep.b #$20
    lda.l !vram_queue+2,x
    sta.w !A1B0
    lda.b #$01
    sta.w !DMAP0
    sta.w !MDMAEN
    rep.b #$20
    txa
    sec
    sbc.w #$0008
    tax
    bpl .next
.empty:
    rep.b #$20
    stz.w !vram_queue_size
    sep.b #$20
    lda.w !frame_update_flags
    and.b #$FD
    sta.w !frame_update_flags
.done:
    plp
    rts

; @routine 7F:1007 VramDmaQueueAdd
; WHAT IT DOES: Queues the VRAM transfer staged at $16B1-$16B8 for the next
; vertical blank, unless the queue is full ($200 bytes, 64 transfers).
; HOW: Copies the source address ($16B3), source bank ($16B5), VRAM address
; ($16B7) and byte count ($16B1) into the next 8-byte slot, in that order, and
; advances the queue size by 8. VramDmaQueueFlush runs the queue.
; Entry: any widths. Exit: A = the new size when appended; P, X unchanged.
%ramcode($7F1007, $058807)
VramDmaQueueAdd:
    php
    phx
    rep.b #$30
    ldx.w !vram_queue_size
    cpx.w #$0200
    beq .done
    lda.w !vram_queue_staged+2
    sta.l !vram_queue,x
    sep.b #$20
    lda.w !vram_queue_staged+4
    sta.l !vram_queue+2,x
    rep.b #$20
    lda.w !vram_queue_staged+6
    sta.l !vram_queue+3,x
    lda.w !vram_queue_staged
    sta.l !vram_queue+5,x
    txa
    clc
    adc.w #$0008
    sta.w !vram_queue_size
.done:
    plx
    plp
    rtl

; @routine 7F:10A1 WordListAAppend
; WHAT IT DOES: Appends A to the eight-word list at $1195, unless it is full.
; HOW: Size in bytes at $1191; stores at $1195 + size and adds 2.
; Entry: any widths. Exit: X = the size (its low byte with 8-bit index
; registers); P unchanged.
%ramcode($7F10A1, $0588A1)
WordListAAppend:
    php
    rep.b #$30
    ldx.w !word_list_a_size
    cpx.w #$0010
    beq .done
    sta.l !word_list_a,x
    inx
    inx
    stx.w !word_list_a_size
.done:
    plp
    rts

; @routine 7F:10B7 WordListBAppend
; WHAT IT DOES: Appends A to the eight-word list at $11B5, unless it is full.
; HOW: Size in bytes at $1193; stores at $11B5 + size and adds 2.
; Entry: as WordListAAppend. Exit: as WordListAAppend.
WordListBAppend:
    php
    rep.b #$30
    ldx.w !word_list_b_size
    cpx.w #$0010
    beq .done
    sta.l !word_list_b,x
    inx
    inx
    stx.w !word_list_b_size
.done:
    plp
    rts
