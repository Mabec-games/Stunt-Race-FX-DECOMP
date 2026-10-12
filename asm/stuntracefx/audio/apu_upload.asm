; apu_upload.asm: sending the sound program and its data to the SPC700.
;
; The S-CPU streams bytes to the sound CPU's boot loader through the four
; APU ports. The stream is read from ROM through a long pointer in direct
; page plus an offset in Y. C: src/stuntracefx/audio/apu_upload.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"

arch 65816

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
%at($04D788)
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
