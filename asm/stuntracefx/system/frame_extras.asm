; frame_extras.asm: RAM code no play runs: the race frame-step pause and a flush of the VRAM DMA queue and the byte writes.
; C: src/stuntracefx/system/frame_extras.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 7E:F2CD RaceFrameStepPause
; WHAT IT DOES: Unless $1168 is set or $08FB is not $FF: sets $1168 to 2,
; runs two race frames (RaceFrameRun), waits for START to be let go, then
; pressed, then let go again, clears the held buttons at $0309-$030C and
; sets $1168 to 1.
; HOW: Each wait polls JOY1H once a frame at scanline $64 (WaitScanline);
; everything through the data bank, so it needs one where the joypad
; registers are. P saved and restored.
; Entry: any widths. Exit: P unchanged; A low byte = 1 when it paused,
; else the byte it tested; X, Y as RaceFrameRun leaves them.
%ramcode($7EF2CD, $014FF4)
RaceFrameStepPause:
    php
    sep.b #$20
    rep.b #$10
    lda.w !frame_step
    bne .lF33A
    lda.w !slot1_count
    cmp.b #$FF
    bne .lF33A
    lda.b #$02
    sta.w !frame_step
    jsl.l RaceFrameRun
    jsl.l RaceFrameRun
    sep.b #$20
.lF2ED:
    lda.b #$64
    sta.w !scanline_wait
    jsl.l WaitScanline
    sep.b #$20
    rep.b #$10
    lda.w !JOY1H
    bit.b #$10
    bne .lF2ED
.lF301:
    lda.b #$64
    sta.w !scanline_wait
    jsl.l WaitScanline
    sep.b #$20
    rep.b #$10
    lda.w !JOY1H
    bit.b #$10
    beq .lF301
.lF315:
    lda.b #$64
    sta.w !scanline_wait
    jsl.l WaitScanline
    sep.b #$20
    rep.b #$10
    lda.w !JOY1H
    bit.b #$10
    bne .lF315
    stz.w !joy1_held+1
    stz.w !joy1_held+3
    stz.w !joy1_held
    stz.w !joy1_held+2
    lda.b #$01
    sta.w !frame_step
.lF33A:
    plp
    rts

; @routine 7F:0D20 FrameQueuesFlush
; WHAT IT DOES: Sends the VRAM DMA queue (VramDmaQueueFlush) and the
; pending byte writes (ByteWritesIfPending).
; HOW: 16-bit registers; P saved and restored.
; Entry: any widths. Exit: P unchanged; A, X, Y as ByteWritesIfPending
; leaves them.
%ramcode($7F0D20, $058520)
FrameQueuesFlush:
    php
    rep.b #$30
    jsr.w VramDmaQueueFlush
    jsr.w ByteWritesIfPending
    plp
    rtl
