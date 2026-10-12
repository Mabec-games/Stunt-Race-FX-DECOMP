; halt.asm: where the game stops when a fixed-size list overflows. No script
; reaches these routines; they are the targets of the overflow checks in
; FrameByteWritePush (SystemHalt), MenuActorSpawnList (SystemHaltSpawn) and
; MenuActorCopyGroup (Hang).
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
