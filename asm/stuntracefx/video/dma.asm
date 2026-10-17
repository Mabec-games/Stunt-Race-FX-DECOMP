; dma.asm: the DMA transfers that feed the PPU: VRAM from work RAM or Super FX
; RAM, the sprite table, the palette. All use DMA channel 0 and run from work
; RAM (main_wram in config/stuntracefx/ramcode.csv). To read Super FX RAM the
; S-CPU must own it: the transfers from bank $70 clear the RAN bit of the
; Super FX screen mode register for the transfer, then give ROM and RAM back.
; C: src/stuntracefx/video/dma.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 7E:ABB6 VramDmaFromBank7F
; WHAT IT DOES: Copies a block of work RAM bank $7F to VRAM.
; HOW: Channel 0 in mode 1 to VMDATA, VRAM incrementing after the high byte,
; then starts the transfer.
; Entry: M 16-bit, X 16-bit; A = byte count, X = source address in bank $7F,
; Y = VRAM word address.
; Exit: A = $1801; M 8-bit; N, Z clear; X, Y unchanged.
%ramcode($7EABB6, $0108DD)
VramDmaFromBank7F:
    sta.w !DAS0L
    stx.w !A1T0L
    sty.w !VMADDL
    lda.w #$1801
    sta.w !DMAP0
    sep.b #$20
    lda.b #$7F
    sta.w !A1B0
    lda.b #$80
    sta.w !VMAIN
    lda.b #$01
    sta.w !MDMAEN
    rts

; @routine 7E:ABD7 VramDmaFromGsuRam
; WHAT IT DOES: Copies a block of Super FX RAM (bank $70) to VRAM.
; HOW: As VramDmaFromBank7F from bank $70, with the RAN bit cleared in the
; Super FX screen mode register for the transfer, then bits 4 and 3 set.
; Entry: M 16-bit, X 16-bit; A = byte count, X = source address, Y = VRAM
; word address.
; Exit: A = $18 high, the screen mode value written last low; M 8-bit; N, Z
; from that value; X, Y unchanged.
VramDmaFromGsuRam:
    sta.w !DAS0L
    stx.w !A1T0L
    sty.w !VMADDL
    lda.w #$1801
    sta.w !DMAP0
    sep.b #$20
    lda.b #$70
    sta.w !A1B0
    lda.b #$80
    sta.w !VMAIN
    lda.w !gsu_scmr_shadow
    and.b #$F7
    sta.w !GSU_SCMR
    lda.b #$01
    sta.w !MDMAEN
    lda.w !gsu_scmr_shadow
    ora.b #$18
    sta.w !GSU_SCMR
    rts

; @routine 7E:AC08 VramDmaRowsFromGsuRam
; WHAT IT DOES: Copies rows of $140 bytes from Super FX RAM to consecutive
; VRAM, taking each row $200 bytes further on in Super FX RAM.
; HOW: Sets the VRAM address and channel 0 once, takes the RAM from the
; Super FX (a 16-bit store to the screen mode register and the byte after
; it), then per row sets the source, a count of $140 and starts the transfer.
; Gives ROM and RAM back at the end.
; Entry: M 16-bit, X 16-bit; A = first row's address, X = VRAM word address,
; Y = row count (1-255; the routine switches to 8-bit index registers).
; Exit: A = the word at $0374 with bits 3 and 4 set; X = 1; Y = 0; X flag
; set; N, Z from A; C and V from the last row step.
VramDmaRowsFromGsuRam:
    stx.w !VMADDL
    ldx.w #$1801
    stx.w !DMAP0
    sep.b #$10
    ldx.b #$70
    stx.w !A1B0
    ldx.b #$80
    stx.w !VMAIN
    pha
    lda.w !gsu_scmr_shadow
    and.w #$FFF7
    sta.w !GSU_SCMR
    pla
.row:
    sta.w !A1T0L
    rep.b #$10
    ldx.w #$0140
    stx.w !DAS0L
    sep.b #$10
    ldx.b #$01
    stx.w !MDMAEN
    dey
    beq .done
    clc
    adc.w #$0200
    bra .row
.done:
    lda.w !gsu_scmr_shadow
    ora.w #$0018
    sta.w !GSU_SCMR
    rts

; @routine 7E:ACF7 OamDma
; WHAT IT DOES: Copies a block of work RAM to OAM from its start.
; HOW: OAM address 0, then channel 0 in mode 0 to OAMDATA from bank $00.
; Entry: M 8-bit, X 16-bit; X = source address, Y = byte count.
; Exit: A low byte = 1; X = $0400; N, Z clear; Y unchanged.
%ramcode($7EACF7, $010A1E)
OamDma:
    stx.w !A1T0L
    lda.b #$00
    sta.w !OAMADDL
    sta.w !OAMADDH
    sty.w !DAS0L
    ldx.w #$0400
    stx.w !DMAP0
    lda.b #$00
    sta.w !A1B0
    lda.b #$01
    sta.w !MDMAEN
    rts

; @routine 7E:AD16 OamDmaAll
; WHAT IT DOES: Copies the whole work-RAM sprite table to OAM.
; HOW: OamDma of the $220 bytes from $0375 (the table and its high table).
; Entry: any widths. Exit: A low byte = 1; X = $0400 and Y = $0220 (or their
; low bytes when entered with 8-bit index registers); P unchanged.
OamDmaAll:
    php
    sep.b #$20
    rep.b #$10
    ldx.w #!oam_buffer
    ldy.w #$0220
    jsr.w OamDma
    plp
    rtl

; @routine 7E:B0E5 CgramDmaAll
; WHAT IT DOES: Copies the whole palette buffer ($7E:FD65, 512 bytes) to the
; palette.
; HOW: CgramDma from colour 0.
; Entry: any widths. Exit: A = $2201; X = 0, Y = 0; P unchanged.
%ramcode($7EB0E5, $010E0C)
CgramDmaAll:
    php
    rep.b #$30
    lda.w #$FD65
    ldx.w #$0000
    ldy.w #$0200
    jsr.w CgramDma
    plp
    rtl

; @routine 7E:B0F6 CgramDma
; WHAT IT DOES: Copies a block of work RAM bank $7E to the palette.
; HOW: Channel 0 in mode 0 to CGDATA; the low byte of X is the first colour.
; Entry: M 16-bit, X 16-bit; A = source address, X = first colour, Y = byte
; count. Exit: A = $2201; M and X 8-bit; N, Z clear.
CgramDma:
    sta.w !A1T0L
    sty.w !DAS0L
    lda.w #$2200
    sta.w !DMAP0
    sep.b #$30
    stx.w !CGADD
    lda.b #$7E
    sta.w !A1B0
    lda.b #$01
    sta.w !MDMAEN
    rts

; @routine 7F:08C7 VramDmaWideRowsFromGsuRam
; WHAT IT DOES: Copies rows of $2C0 bytes from Super FX RAM to consecutive
; VRAM, each row $400 bytes further on in Super FX RAM.
; HOW: Takes the RAM from the Super FX, keeps the source in direct page
; $FB-$FC, sets the VRAM address and channel 0 (mode 1, bank $70) once, then
; transfers rows while the 8-bit row counter stays non-negative; gives ROM
; and RAM back. Runs from work RAM bank $7F.
; Entry: M 16-bit, X 16-bit, D = $0000; A low byte = rows - 1, X = source,
; Y = VRAM word address. Exit: A = the source high byte and the screen mode
; value; X = $00FF; Y = 1; P unchanged.
%ramcode($7F08C7, $0580C7)
VramDmaWideRowsFromGsuRam:
    php
    rep.b #$30
    pha
    sep.b #$20
    lda.w !gsu_scmr_shadow
    and.b #$F7
    sta.w !GSU_SCMR
    rep.b #$20
    stx.b !dp_fb
    sty.w !VMADDL
    sep.b #$10
    rep.b #$20
    ldx.b #$80
    stx.w !VMAIN
    ldx.b #$18
    stx.w !DMAP0+1
    ldy.b #$70
    sty.w !A1B0
    ldy.b #$01
    sty.w !DMAP0
    pla
    tax
.row:
    lda.w #$02C0
    sta.w !DAS0L
    lda.b !dp_fb
    sta.w !A1T0L
    clc
    adc.w #$0400
    sta.b !dp_fb
    sty.w !MDMAEN
    dex
    bpl .row
    sep.b #$20
    lda.w !gsu_scmr_shadow
    ora.b #$18
    sta.w !GSU_SCMR
    plp
    rts

; @routine 7F:0BCE VramWriteRowFromGsuRam
; WHAT IT DOES: Copies one row of 64 words from Super FX RAM ($70:3000 plus
; the offset in direct page $02) to VRAM, then moves the offset down a row
; ($200 bytes).
; HOW: Sets the VRAM address and writes the words to VMDATA one by one.
; Entry: M 16-bit, X 16-bit, D = $0000; A = VRAM word address.
; Exit: A = the new offset; X = the old offset + $80; Y = $FFFF; P unchanged.
%ramcode($7F0BCE, $0583CE)
VramWriteRowFromGsuRam:
    php
    rep.b #$30
    sta.w !VMADDL
    ldy.w #$003F
    ldx.b !dp_02
.word:
    lda.l !gsu_picture,x
    sta.w !VMDATAL
    inx
    inx
    dey
    bpl .word
    lda.b !dp_02
    clc
    adc.w #$0200
    sta.b !dp_02
    plp
    rts

; @routine 7E:ACBD VramUploadGsuBuffer
; WHAT IT DOES: When a Super FX picture is waiting, copies it ($C00 bytes)
; from Super FX RAM to VRAM $6600, from whichever half of the double buffer
; is ready, and clears the request.
; HOW: With $0D32 set: channel 0 in mode 1 to VMDATA from $70:4000 when
; $0D7F is set, else from $70:D000; then $0D32 = 0.
; Entry: M 8-bit, X 16-bit. Exit: A low byte = 1 after a copy, else 0; X =
; the source after a copy; N clear; Z set when nothing was copied.
%ramcode($7EACBD, $0109E4)
VramUploadGsuBuffer:
    lda.w !gsu_upload_pending
    beq .lACF6
    lda.b #$80
    sta.w !VMAIN
    ldx.w #$6600
    stx.w !VMADDL
    ldx.w #$1801
    stx.w !DMAP0
    lda.b #$70
    sta.w !A1B0
    ldx.w #$0C00
    stx.w !DAS0L
    lda.w !gsu_buffer_select
    beq .lACE8
    ldx.w #$4000
    bra .lACEB
.lACE8:
    ldx.w #$D000
.lACEB:
    stx.w !A1T0L
    lda.b #$01
    sta.w !MDMAEN
    stz.w !gsu_upload_pending
.lACF6:
    rts

; @routine 03:ECA8 DmaFromDescriptor
; WHAT IT DOES: Runs DMA channel 0 from a seven-byte descriptor: the bytes
; for $4300-$4306 (mode, B-bus register, source address and bank, size).
; HOW: The descriptor address goes to direct page $FB; the bytes are copied
; last to first through ($FB),Y in the data bank, then MDMAEN is set to 1.
; Entry: M 16-bit, D = $0000; A = descriptor address. Exit: $FB = A; A low
; byte = 1; Y = $FF (high byte zero); P unchanged.
%at($03ECA8)
DmaFromDescriptor:
    php
    sta.b !apu_stream_ptr
    sep.b #$30
    ldy.b #$06
.lECAF:
    lda.b (!apu_stream_ptr),y
    sta.w !DMAP0,y
    dey
    bpl .lECAF
    lda.b #$01
    sta.w !MDMAEN
    plp
    rts

; @routine 7F:0B56 CoursePictureToVram
; WHAT IT DOES: Copies four 64-word rows of the current course's picture,
; as the Super FX drew it, to VRAM $6480, $6580, $64C0 and $65C0, starting
; from the Super FX RAM offset the table at $7F:0B96 gives for the course
; in $0E3D.
; HOW: The table word w (index * 2) packs the offset: ((w AND $FFFC) with
; its bytes swapped) * 2 + (w AND 3) * $80, stored in direct page $02; then
; VramWriteRowFromGsuRam four times, each moving the offset down a row.
; $0E3D is read through the data bank.
; Entry: X 16-bit, D = $0000. Exit: A = the final offset; X = the last
; row's offset + $80; Y = $FFFF (low bytes only when P restores 8-bit index
; registers); P unchanged.
%ramcode($7F0B56, $058356)
CoursePictureToVram:
    php
    sep.b #$20
    rep.b #$10
    lda.w !course
    rep.b #$20
    and.w #$00FF
    asl a
    tax
    lda.l !course_picture_offsets,x
    pha
    and.w #$FFFC
    xba
    asl a
    sta.b !dp_02
    pla
    and.w #$0003
    xba
    lsr a
    clc
    adc.b !dp_02
    sta.b !dp_02
    lda.w #$6480
    jsr.w VramWriteRowFromGsuRam
    lda.w #$6580
    jsr.w VramWriteRowFromGsuRam
    lda.w #$64C0
    jsr.w VramWriteRowFromGsuRam
    lda.w #$65C0
    jsr.w VramWriteRowFromGsuRam
    plp
    rtl

; @routine 7F:0CDF AnimTilesQueueNext
; WHAT IT DOES: Queues the next $400-byte frame of the animated tiles at
; $7F:A7D7 for VRAM $2000: the frame offset in $18F7 steps by $400 through
; $2000 bytes (eight frames), and the transfer goes on the VRAM queue.
; HOW: Stages source $A7D7 + offset, bank $7F, VRAM $2000 and count $400 at
; $16B1-$16B8 (through the data bank) and calls VramDmaQueueAdd; then sets
; bit 1 of the update flags at $11D7.
; Entry: any widths. Exit: A low byte = the update flags, high byte from
; the new queue size (or $20 when the queue was full); P, X, Y unchanged.
%ramcode($7F0CDF, $0584DF)
AnimTilesQueueNext:
    php
    rep.b #$30
    lda.l !wram_18F7
    clc
    adc.w #$0400
    and.w #$1FFF
    sta.l !wram_18F7
    clc
    adc.w #$A7D7
    rep.b #$30
    sta.w !vram_queue_staged+2
    lda.w #$0400
    sta.w !vram_queue_staged
    lda.w #$007F
    sta.w !vram_queue_staged+4
    lda.w #$2000
    sta.w !vram_queue_staged+6
    jsl.l VramDmaQueueAdd
    rep.b #$20
    rep.b #$10
    sep.b #$20
    lda.w !frame_update_flags
    ora.b #$02
    sta.w !frame_update_flags
    plp
    rtl

; @routine 7E:A7BE NmiFrameUpload
; WHAT IT DOES: The vertical blank's uploads for the screen being built,
; one stage per frame. With $0D22 nonzero, the split screen's stage $0D23
; (the table at $AA4B): pictures and maps from Super FX RAM and bank $7F,
; sprites, BG2's scroll and the palette. Otherwise the race screen's stage
; $0D7D (the table at $A7D1): halves of the Super FX picture by the buffer
; $0D7F, the bank $7F rows that go with them, the tile and map bases and
; BG2's scroll for the finished buffer ($0D81 = -1 or 1, $0D28
; decremented); each sets the next stage. Stages with nothing to do
; upload the sprites (with $0D28 set and $0D54 clear) and the palette.
; HOW: P saved and restored. JMP (table,X); the stages share their ends.
; The stage table's entries $1C ($A8E6, which pushes P again) and the
; helper at $A7F3 are never used.
; Entry: D = $0000, data bank $00. Exit: P restored; A, X, Y as the stage
; leaves them.
%ramcode($7EA7BE, $0104E5)
NmiFrameUpload:
    php
    sep.b #$20
    lda.w !wram_0D22
    beq .lA7C9
    jmp.w NmiFrameUpload_split
.lA7C9:
    rep.b #$30
    ldx.w !gsu_buffer_offset
    jmp.w (.table,x)
.table:
    dw NmiFrameUpload_s00, NmiFrameUpload_s02, NmiFrameUpload_s04, NmiFrameUpload_s06, NmiFrameUpload_s08, NmiFrameUpload_s0A, NmiFrameUpload_s0C, NmiFrameUpload_s0E
    dw NmiFrameUpload_s10, NmiFrameUpload_s12, NmiFrameUpload_s14, NmiFrameUpload_s16, NmiFrameUpload_s18, NmiFrameUpload_s1A, CgramUploadAll, NmiFrameUpload_s06
    dw NmiFrameUpload_split
    ; nothing jumps here or calls this: a row copy to VMDATA, unused
.lA7F3:
    lda.w !bank0_base,x             ; not reached in the traces so far
    sta.l !VMDATAL                  ; not reached in the traces so far
    inx                             ; not reached in the traces so far
    inx                             ; not reached in the traces so far
    dey                             ; not reached in the traces so far
    bne .lA7F3                      ; not reached in the traces so far
    rts                             ; not reached in the traces so far
NmiFrameUpload_sprites:
    sep.b #$20
    rep.b #$10
    ldx.w #$0001
.lA807:
    dex
    bne .lA807
    lda.w !wram_0D25+3
    beq .lA820
    stz.w !wram_0D25+3
    lda.w !wram_0D54
    bne .lA820
    ldx.w #$0375
    ldy.w #$0220
    jsr.w OamDma
.lA820:
    rep.b #$30
    jsr.w CgramUploadAll
    ldx.w #$000A
.lA828:
    dex
    bne .lA828
    plp
    rts
NmiFrameUpload_s00:
    rep.b #$30
    lda.w #$0001
.lA832:
    dec a
    bne .lA832
    lda.w !gsu_buffer_select
    beq NmiFrameUpload_sprites
    rep.b #$20
    ldx.w #$0002
    stx.w !gsu_buffer_offset
    ldx.w #$2C00
    ldy.w #$0000
    lda.w #$2200
    jsr.w VramDmaFromGsuRam
    rep.b #$30
    lda.w #$0200
    ldx.w #$E3D7
    ldy.w #$5BC0
    jsr.w VramDmaFromBank7F
    plp
    rts
NmiFrameUpload_s02:
    rep.b #$30
    lda.w #$0001
.lA863:
    dec a
    bne .lA863
    ldx.w #$0004
    stx.w !gsu_buffer_offset
    ldx.w #$4E00
    ldy.w #$1100
    lda.w #$2200
    jsr.w VramDmaFromGsuRam
    rep.b #$30
    lda.w #$0240
    ldx.w #$E5D7
    ldy.w #$5CC0
    jsr.w VramDmaFromBank7F
    sep.b #$20
    lda.b #$01
    sta.w !wram_0357
    plp
    rts
NmiFrameUpload_s04:
    rep.b #$30
    lda.w #$0004
.lA894:
    dec a
    bne .lA894
    ldx.w #$0006
    stx.w !gsu_buffer_offset
    lda.w !gradient_src_b+2
    sta.l !gradient_ptr_a
    ldx.w #$7000
    ldy.w #$2200
    lda.w #$2400
    jsr.w VramDmaFromGsuRam
    sep.b #$20
    lda.b #$60
    sta.w !BG12NBA
    lda.b #$78
    sta.w !BG1SC
    lda.w !bg2_scroll_x
    sta.w !BG2HOFS
    lda.w !bg2_scroll_x+1
    sta.w !BG2HOFS
    lda.w !bg2_scroll_y
    sta.w !BG2VOFS
    lda.w !bg2_scroll_y+1
    sta.w !BG2VOFS
    rep.b #$20
    lda.w #$FFFF
    sta.l !wram_0D81
    sep.b #$20
    stz.b !bank0_base
    dec.w !wram_0D25+3
    plp
    rts

; @routine 7E:A8E6 CgramUploadAll
; WHAT IT DOES: Copies the whole palette buffer ($7E:FD65, 512 bytes) to
; CGRAM, noting $FD65 at $0D83.
; HOW: CgramDma from colour 0. NmiFrameUpload's stages follow it in the
; same stretch of code.
; Entry: any widths. Exit: P restored; A = $2201, X = 0, Y = 0.
%ramcode($7EA8E6, $01060D)
CgramUploadAll:
    php
    rep.b #$30
    lda.w #$FD65
    sta.w !player_pal_offset
    ldx.w #$0000
    ldy.w #$0200
    jsr.w CgramDma
    plp
    rts
NmiFrameUpload_s06:
    rep.b #$30
    lda.w !gsu_buffer_select
    beq .lA904
    jmp.w NmiFrameUpload_sprites
.lA904:
    lda.w #$0001
.lA907:
    dec a
    bne .lA907
    ldx.w #$0008
    stx.w !gsu_buffer_offset
    ldx.w #$B800
    ldy.w #$3400
    lda.w #$2200
    jsr.w VramDmaFromGsuRam
    rep.b #$30
    lda.w #$0200
    ldx.w #$E817
    ldy.w #$5DE0
    jsr.w VramDmaFromBank7F
    plp
    rts
NmiFrameUpload_s08:
    lda.w #$0001
.lA92F:
    dec a
    bne .lA92F
    ldx.w #$000A
    stx.w !gsu_buffer_offset
    ldx.w #$DA00
    ldy.w #$4500
    lda.w #$2200
    jsr.w VramDmaFromGsuRam
    rep.b #$30
    lda.w #$0240
    ldx.w #$EA17
    ldy.w #$5EE0
    jsr.w VramDmaFromBank7F
    sep.b #$20
    lda.b #$01
    sta.w !wram_0357
    plp
    rts
NmiFrameUpload_s0A:
    rep.b #$30
    lda.w #$0004
.lA960:
    dec a
    bne .lA960
    lda.w #$0000
    sta.w !gsu_buffer_offset
    lda.w !gradient_src_b+4
    sta.l !gradient_ptr_a
    ldx.w #$9400
    ldy.w #$2200
    lda.w #$2400
    jsr.w VramDmaFromGsuRam
    sep.b #$20
    lda.w !bg2_scroll_x+2
    sta.w !BG2HOFS
    lda.w !bg2_scroll_x+3
    sta.w !BG2HOFS
    lda.w !bg2_scroll_y+2
    sta.w !BG2VOFS
    lda.w !bg2_scroll_y+3
    sta.w !BG2VOFS
    lda.b #$62
    sta.w !BG12NBA
    lda.b #$7C
    sta.w !BG1SC
    rep.b #$20
    lda.w #$0001
    sta.w !wram_0D81
    sep.b #$20
    stz.b !bank0_base
    dec.w !wram_0D25+3
    plp
    rts
NmiFrameUpload_s0C:
    lda.w !gsu_buffer_select
    beq NmiFrameUpload_s0E
    ldx.w #$0010
    stx.w !gsu_buffer_offset
    lda.w #$2C00
    ldx.w #$0000
    ldy.w #$0012
    jsr.w VramDmaRowsFromGsuRam
    sep.b #$20
    lda.b #$01
    sta.w !wram_0357
    stz.b !bank0_base
NmiFrameUpload_s0E:
    plp
    rts
NmiFrameUpload_s10:
    lda.w !gsu_buffer_select
    bne NmiFrameUpload_s12
    ldx.w #$0014
    stx.w !gsu_buffer_offset
    lda.w #$9400
    ldx.w #$0B40
    ldy.w #$0012
    jsr.w VramDmaRowsFromGsuRam
    sep.b #$20
    lda.b #$01
    sta.w !wram_0357
    stz.b !bank0_base
    lda.b #$40
    sta.w !BG12NBA
NmiFrameUpload_s12:
    plp
    rts
NmiFrameUpload_s14:
    lda.w !gsu_buffer_select
    beq NmiFrameUpload_s16
    ldx.w #$0018
    stx.w !gsu_buffer_offset
    lda.w #$2C00
    ldx.w #$2000
    ldy.w #$0012
    jsr.w VramDmaRowsFromGsuRam
    sep.b #$20
    lda.b #$01
    sta.w !wram_0357
    stz.b !bank0_base
NmiFrameUpload_s16:
    plp
    rts
NmiFrameUpload_s18:
    lda.w !gsu_buffer_select
    bne NmiFrameUpload_s1A
    ldx.w #$000C
    stx.w !gsu_buffer_offset
    lda.w #$9400
    ldx.w #$2B40
    ldy.w #$0012
    jsr.w VramDmaRowsFromGsuRam
    sep.b #$20
    lda.b #$01
    sta.w !wram_0357
    stz.b !bank0_base
    lda.b #$42
    sta.w !BG12NBA
NmiFrameUpload_s1A:
    plp
    rts
NmiFrameUpload_split:
    sep.b #$30
    ldx.w !wram_0D23
    jmp.w (.table,x)
.table:
    dw .lABB4, .lAB63, .lAB85, .lAB5C, .lAB6A, .lAB4C, .lAA91, .lAA9A
    dw .lAB0F, .lAA5F
.lAA5F:
    sep.b #$20
    rep.b #$10
    ldx.w !fx_pool_first+8
    stx.w !A1T0L
    lda.b #$7F
    sta.w !A1B0
    ldx.w #$0C00
    stx.w !DAS0L
    ldx.w #$6600
    stx.w !VMADDL
    ldx.w #$1801
    stx.w !DMAP0
    lda.b #$80
    sta.w !VMAIN
    lda.b #$01
    sta.w !MDMAEN
    lda.b #$02
    sta.w !wram_0D23
    plp
    rts
.lAA91:
    sep.b #$20
    rep.b #$10
    ldx.w #$0000
    bra .lAAA1
.lAA9A:
    sep.b #$20
    rep.b #$10
    ldx.w #$0B40
.lAAA1:
    stx.w !VMADDL
    ldx.w #$1680
    stx.w !DAS0L
    ldx.w #$9BD7
    stx.w !A1T0L
    ldx.w #$1801
    stx.w !DMAP0
    lda.b #$7F
    sta.w !A1B0
    lda.b #$80
    sta.w !VMAIN
    lda.w !gsu_scmr_shadow
    and.b #$F7
    sta.w !GSU_SCMR
    lda.b #$01
    sta.w !MDMAEN
    lda.w !gsu_scmr_shadow
    ora.b #$18
    sta.w !GSU_SCMR
    sep.b #$20
    lda.b #$70
    sta.w !BG12NBA
    lda.b #$10
    sta.w !wram_0D23
    lda.b #$00
    sta.w !CGADD
    sta.w !CGADD
    sta.w !CGADD
    sta.w !CGADD
    sta.w !CGADD
    sta.w !CGADD
    sta.w !CGADD
    lda.b #$C4
    sta.w !CGDATA
    sta.l !palette_buffer
    lda.b #$1C
    sta.w !CGDATA
    sta.l !palette_buffer+1
    stz.w !palette_animate
    plp
    rts
.lAB0F:
    rep.b #$20
    lda.w #$0800
    sta.w !DAS0L
    lda.w #$93D7
    sta.w !A1T0L
    lda.w #$7800
    sta.w !VMADDL
    lda.w #$1801
    sta.w !DMAP0
    sep.b #$20
    lda.b #$7F
    sta.w !A1B0
    lda.b #$80
    sta.w !VMAIN
    lda.w !gsu_scmr_shadow
    and.b #$F7
    sta.w !GSU_SCMR
    lda.b #$01
    sta.w !MDMAEN
    lda.w !gsu_scmr_shadow
    ora.b #$18
    sta.w !GSU_SCMR
    plp
    rts
.lAB4C:
    sep.b #$20
    rep.b #$10
    jsr.w VramUploadGsuBuffer
    ldx.w #$0375
    ldy.w #$0220
    jsr.w OamDma
.lAB5C:
    rep.b #$30
    jsr.w CgramUploadAll
    plp
    rts
.lAB63:
    sep.b #$20
    rep.b #$10
    jsr.w TilemapRowsTo5C00
.lAB6A:
    sep.b #$20
    rep.b #$10
    ldx.w #$0375
    ldy.w #$0220
    jsr.w OamDma
    sep.b #$30
    ldx.b #$00
    jsr.w Bg2ScrollFromTable
    rep.b #$30
    jsr.w CgramUploadAll
    plp
    rts
.lAB85:
    rep.b #$30
    ldx.w #$4000
    ldy.w #$6C00
    lda.w #$0A00
    jsr.w VramDmaFromGsuRam
    rep.b #$30
    ldx.w #$4600
    ldy.w #$6240
    lda.w #$0140
    jsr.w VramDmaFromGsuRam
    rep.b #$30
    ldx.w #$4800
    ldy.w #$6340
    lda.w #$0140
    jsr.w VramDmaFromGsuRam
    sep.b #$20
    stz.w !wram_0D23
.lABB4:
    plp
    rts


; @routine 03:ECBE VramClearWords
; WHAT IT DOES: Clears X words of VRAM from word address A. Nothing in the
; game calls it.
; HOW: VMAIN = $80 (a 16-bit store of Y = $0080, which also writes 0 to
; VMADDL), VMADDL = A, then X 16-bit zero stores to VMDATAL; the
; registers through the data bank.
; Entry: M 16-bit, X 16-bit; A = VRAM word address, X = count (not 0).
; Exit: A = 0; X = 0; Y = $0080; Z set, N clear.
%at($03ECBE)
VramClearWords:
    ldy.w #$0080
    sty.w !VMAIN
    sta.w !VMADDL
    lda.w #$0000
.loop:
    sta.w !VMDATAL
    dex
    bne .loop
    rts
