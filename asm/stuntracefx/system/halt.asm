; halt.asm: where the game stops when a fixed-size list overflows. No script
; reaches these routines; they are the targets of the overflow checks in
; FrameByteWritePush (SystemHalt), MenuActorSpawnList (SystemHaltSpawn) and
; MenuActorCopyGroup (Hang). DebugByteHalt is a debug stop that nothing
; calls.
; C: src/stuntracefx/system/halt.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 0B:E21F Hang
; WHAT IT DOES: Stops the game: a branch to itself, for ever.
; HOW: BRA to its own address. Never returns.
; Entry: any. Exit: does not return.
%at($0BE21F)
Hang:
    bra Hang

; @routine 0B:E41D SystemHalt
; WHAT IT DOES: Stops the game with a flickering screen: display on at full
; brightness with no layers, HDMA and interrupts off, then the backdrop
; colour cycles for ever.
; HOW: INIDISP = $0F, HDMAEN = NMITIMEN = TM = 0; then a loop that sets
; the palette address to 0 (written twice) and writes Y's low byte to both
; bytes of colour 0, increments Y and waits nine NOPs. Never returns.
; Entry: X 16-bit. Exit: does not return.
%at($0BE41D)
SystemHalt:
    sep.b #$20
    lda.b #$0F
    sta.l !INIDISP
    lda.b #$00
    sta.l !HDMAEN
    sta.l !NMITIMEN
    sta.l !TM
    ldx.w #$0000
    txy
.loop:
    txa
    sta.l !CGADD
    sta.l !CGADD
    tya
    sta.l !CGDATA
    sta.l !CGDATA
    iny
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    jmp.w .loop

; @routine 0B:E47A SystemHaltSpawn
; WHAT IT DOES: A second copy of SystemHalt, the target of
; MenuActorSpawnList's check for a full object pool.
; HOW: The same code as SystemHalt; its loop jumps back within this copy.
; Entry: X 16-bit. Exit: does not return.
%at($0BE47A)
SystemHaltSpawn:
    sep.b #$20
    lda.b #$0F
    sta.l !INIDISP
    lda.b #$00
    sta.l !HDMAEN
    sta.l !NMITIMEN
    sta.l !TM
    ldx.w #$0000
    txy
.loop:
    txa
    sta.l !CGADD
    sta.l !CGADD
    tya
    sta.l !CGDATA
    sta.l !CGDATA
    iny
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    jmp.w .loop

; @routine 03:D40A DebugByteHalt
; WHAT IT DOES: A debug stop that nothing calls: for ever, the screen on at
; full brightness, $0D61 = $0F, $00 = 2, the byte $69 printed as two hex
; digits (DebugPrintHexByte) at text position $0680 ($46, half $0307 = 0);
; $0357 and the Super FX buffer select $0D7F cleared; the Super FX screen
; mode from $0374 with its RAM taken back (bit 3 off), the screen start
; $70:021A and $48 = $2C00, the mode again with RAM and ROM given to the
; Super FX (bits $18), and the screen base $0B.
; HOW: PHP, SEP #$20, REP #$10, PHB, PHK, PLB (never pulled), then a BRA
; loop; the $69 kept across the stores by PHA, PLA. Never returns.
; Entry: D = $0000. Exit: does not return.
%at($03D40A)
DebugByteHalt:
    php
    sep.b #$20
    rep.b #$10
    phb
    phk
    plb
.loop:
    lda.b #$0F
    sta.l !INIDISP
    lda.b #$0F
    sta.w !fade_level_hi
    lda.b #$02
    sta.b !dp_00
    lda.b #$69
    pha
    lda.b #$80
    sta.b !text_pos
    lda.b #$06
    sta.b !text_pos+1
    lda.b #$00
    sta.w !text_half
    pla
    jsl.l DebugPrintHexByte
    sep.b #$20
    stz.w !wram_0357
    rep.b #$20
    stz.w !gsu_buffer_select
    lda.w !gsu_scmr_shadow
    and.w #$FFF7
    sta.w !GSU_SCMR
    lda.w #$2C00
    sta.l !gsu_screen_start
    sta.b !gsu_screen_start_dp
    lda.w !gsu_scmr_shadow
    ora.w #$0018
    sta.w !GSU_SCMR
    sep.b #$20
    lda.b #$0B
    sta.l !GSU_SCBR
    bra .loop
