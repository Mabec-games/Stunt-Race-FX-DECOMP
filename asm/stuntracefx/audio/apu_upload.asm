; apu_upload.asm: sending the sound program and its data to the SPC700.
;
; At boot the SPC700 runs its built-in loader. The S-CPU sends it blocks of
; bytes through the four APU ports with the loader's handshake, then tells it
; where to start. The data is a stream in ROM: a list of blocks, each a
; 16-bit length and a 16-bit SPC700 address followed by that many bytes; a
; block of length 0 ends the list and its address is where the SPC700 starts.
; C: src/stuntracefx/audio/apu_upload.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"

arch 65816

; @routine 04:D43C ApuPort1Send05
; WHAT IT DOES: Puts $05 on APU port 1, then waits while the sound CPU
; responds (falls into ApuSettleDelay).
; HOW: An 8-bit store to port 1.
; Entry: M 16-bit. Exit: as ApuSettleDelay.
%at($04D43C)
ApuPort1Send05:
    sep.b #$20
    lda.b #$05
    sta.w !APUIO1

; @routine 04:D443 ApuSettleDelay
; WHAT IT DOES: Waits a fixed time for the sound CPU to act on a command.
; HOW: Counts A down from $4000 to zero; it has no other effect.
; Entry: any widths. Exit: A = 0; M 16-bit; Z set, N clear.
ApuSettleDelay:
    rep.b #$20
    lda.w #$4000
.wait:
    dec a
    bne .wait
    rtl

; @routine 04:D44C ApuUploadBank19
; WHAT IT DOES: Uploads the sound block list at ROM $19:C27C to the SPC700,
; then waits for it to settle.
; HOW: ApuUpload with bank $19 and address $C27C, then the same count-down
; as ApuSettleDelay.
; Entry: any widths, D = $0000. Exit: A = 0, X = 0, Y = the stream offset
; after the list; P unchanged.
%at($04D44C)
ApuUploadBank19:
    php
    rep.b #$30
    lda.w #$0019
    ldy.w #$C27C
    jsl.l ApuUpload
    rep.b #$20
    lda.w #$4000
.wait:
    dec a
    bne .wait
    plp
    rtl

; @routine 04:D720 ApuUpload
; WHAT IT DOES: Uploads a block list from ROM to the SPC700's boot loader and
; starts the SPC700 at the address the list ends with.
; HOW: Points the upload stream at A:Y, waits for the loader's ready signature
; $BBAA on ports 0-1, then for each block sends the SPC700 address on ports
; 2-3, a flag on port 1 (1: data follows, 0: start here) and a kick value on
; port 0, and waits for the loader to echo the kick. The block's bytes go out
; on port 1 with a running index on port 0 (one 16-bit store), each after the
; loader has echoed the previous index. The first kick is $CC; each later one
; is the last index plus 4 (plus 3 more while that wraps to zero). The
; overflow flag from adding $7F to the port 1 flag says whether to go on.
; Entry: M 16-bit, X 16-bit, D = $0000; A low byte = ROM bank, Y = address.
; Exit: A = the final kick (high byte 0); X = 0; Y = the stream offset after
; the list; N, V clear; Z, C set; M 8-bit.
%at($04D720)
ApuUpload:
    sep.b #$20
    sty.b !apu_stream_ptr
    sta.b !apu_stream_bank
    ldy.w #$0000
    ldx.w #$BBAA
.wait_ready:
    cpx.w !APUIO0
    bne .wait_ready
    lda.b #$CC
    bra .header
.block:
    jsr.w ApuUploadNextByte
    xba
    lda.b #$00
    bra .send
.next_byte:
    xba
    jsr.w ApuUploadNextByte
    xba
.wait_index:
    cmp.w !APUIO0
    bne .wait_index
    inc a
.send:
    rep.b #$20
    sta.w !APUIO0
    sep.b #$20
    dex
    bne .next_byte
.wait_last:
    cmp.w !APUIO0
    bne .wait_last
.next_kick:
    adc.b #$03
    beq .next_kick
.header:
    pha
    jsr.w ApuUploadNextByte
    xba
    jsr.w ApuUploadNextByte
    xba
    tax
    jsr.w ApuUploadNextByte
    sta.w !APUIO2
    jsr.w ApuUploadNextByte
    sta.w !APUIO3
    cpx.w #$0001
    lda.b #$00
    rol a
    sta.w !APUIO1
    adc.b #$7F
    pla
    sta.w !APUIO0
.wait_kick:
    cmp.w !APUIO0
    bne .wait_kick
    bvs .block
    rtl

; @routine 04:D788 ApuUploadNextByte
; WHAT IT DOES: Returns the next byte of the sound data being uploaded to the
; SPC700 and advances the stream past it.
; HOW: Reads the byte at stream pointer + Y, then steps Y. When pointer + Y
; runs past the end of the 64 KB bank, the stream goes on at $8000 of the next
; bank, the next 32 KB of LoROM: the pointer's low word becomes 0, Y becomes
; $8000, and the 16-bit word holding the bank byte is incremented. A is saved
; around the add so its high byte comes back unchanged.
; Entry: M 8-bit, X 16-bit, D = $0000; Y = offset.
; Exit: A low byte = the byte read; Y and the pointer advanced; C set when the
; stream moved to the next bank; N, Z from the 16-bit A; M 8-bit.
ApuUploadNextByte:
    lda.b [!apu_stream_ptr],y
    iny
    rep.b #$20
    pha
    tya
    clc
    adc.b !apu_stream_ptr
    bcc .done
    lda.w #$0000
    sta.b !apu_stream_ptr
    ldy.w #$8000
    inc.b !apu_stream_bank
.done:
    pla
    sep.b #$20
    rts
