; reset.asm: the soft reset (L, R, START and SELECT held) and the jumps
; that restart the game. C: src/stuntracefx/system/reset.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 07:AACE SoftResetLong
; WHAT IT DOES: SoftResetCheck for callers in other banks.
; HOW: P and the data bank saved; the data bank set to its own ($07).
; Entry: any widths. Exit: as SoftResetCheck, then P and the data bank put
; back.
%at($07AACE)
SoftResetLong:
    php
    sep.b #$20
    phb
    phk
    plb
    jsr.w SoftResetCheck
    sep.b #$20
    plb
    plp
    rtl

; @routine 07:AADC SoftResetCheck
; WHAT IT DOES: The soft reset: when player 1 holds L, R, START and SELECT
; and nothing else ($0309), with no reset under way ($0D51), starts one:
; $0D51 down by one, sound $04 on port 1, the fade out ($0E3A = 1) and $70
; frames to wait ($0D5F). Under way, it waits those frames ($0D52 set by
; each vertical blank), then blanks the screen, stops the NMI, DMA and HDMA,
; clears $0D51, resets the sound (07:AB8C), clears $0D2B and $0D62, clears
; work RAM ($00:0300-$2100, $7E:2000-$E000, $7F:0000-$FFFF by 03:8CF2) and
; Super FX RAM $70:0000-$27FF, and restarts the game (03:8B34).
; HOW: 8-bit A for the flags, 16-bit for the buttons; through the data bank;
; the Super FX RAM by MVN from each byte to the next.
; Entry: any widths; DB = $07. Exit: P unchanged; A = the buttons masked
; (no reset); the reset never returns.
%at($07AADC)
SoftResetCheck:
    php
    sep.b #$20
    lda.w !hud_blank+3
    bne .lAB14
    rep.b #$20
    lda.w !joy1_held
    and.w #$3030
    cmp.w #$3030
    beq .lAAF4
    brl SoftResetCheck_done
.lAAF4:
    lda.w !joy1_held
    and.w #$CFCF
    beq .lAAFF
    brl SoftResetCheck_done
.lAAFF:
    sep.b #$20
    dec.w !hud_blank+3
    lda.b #$04
    jsl.l SoundPort1QueuePush
    lda.b #$01
    sta.w !fade_out
    lda.b #$70
    sta.w !wram_0D5D+2
.lAB14:
    lda.w !hud_blank+4
    beq .lAB14
    stz.w !hud_blank+4
    dec.w !wram_0D5D+2
    bne .lAB14
    lda.b #$80
    sta.w !INIDISP
    stz.w !NMITIMEN
    stz.w !MDMAEN
    stz.w !HDMAEN
    stz.w !hud_blank+3
    jsr.w SoftResetApu
    sep.b #$20
    stz.w !hud_mode
    stz.w !wram_0D62
    sep.b #$20
    rep.b #$10
    lda.b #$00
    ldx.w #$0300
    ldy.w #$2100
    jsl.l WramFillZeroLong
    lda.b #$7E
    ldx.w #$2000
    ldy.w #$E000
    jsl.l WramFillZeroLong
    lda.b #$7F
    ldx.w #$0000
    ldy.w #$FFFF
    jsl.l WramFillZeroLong
    phb
    php
    rep.b #$30
    lda.w #$0000
    sta.l !gsu_ram
    lda.w #$27FF
    ldx.w #$0000
    ldy.w #$0001
    mvn !gsu_ram>>16,!gsu_ram>>16
    plp
    plb
    jml.l BootStart

; @routine 07:AB82 SoftResetWarmBoot
; WHAT IT DOES: Restarts the game from 03:8BAA. Nothing reaches it: it
; follows SoftResetCheck's jump to 03:8B34.
; HOW: A long jump.
; Entry: any. Exit: never returns.
%at($07AB82)
SoftResetWarmBoot:
    jml.l BootWarm
SoftResetCheck_done:
    plp
    rts

; @routine 07:AB88 SoftResetApuLong
; WHAT IT DOES: SoftResetApu for callers in other banks. Nothing calls it.
; HOW: A near call, then a long return.
; Entry: any widths. Exit: as SoftResetApu.
%at($07AB88)
SoftResetApuLong:
    jsr.w SoftResetApu
    rtl

; @routine 07:AB8C SoftResetApu
; WHAT IT DOES: Restarts the sound for the soft reset: a pause, command $06
; to APU port 1, another pause, the sound program uploaded again (ApuUpload
; of block $19 at $C27C) and a last pause.
; HOW: Delay loops of $8000 counts of X and $4000 of A; P saved around the
; upload and around the whole.
; Entry: any widths. Exit: P unchanged; A = 0; X = 0; Y as ApuUpload leaves
; it.
%at($07AB8C)
SoftResetApu:
    php
    sep.b #$20
    rep.b #$10
    ldx.w #$8000
.lAB94:
    dex
    bne .lAB94
    lda.b #$06
    sta.l !APUIO1
    ldx.w #$8000
.lABA0:
    dex
    bne .lABA0
    php
    rep.b #$30
    lda.w #$0019
    ldy.w #$C27C
    jsl.l ApuUpload
    rep.b #$20
    lda.w #$4000
.lABB5:
    dec a
    bne .lABB5
    plp
    plp
    rts

; @routine 00:FE88 ResetVector
; WHAT IT DOES: Where the console starts the game (the reset vector): stops
; interrupts, switches the CPU to native mode and decimal off, and jumps
; to the start (03:8AA9).
; HOW: SEI, CLC, XCE (the old emulation flag into C), CLD, a long jump.
; Entry: the reset (emulation mode). Exit: never returns.
%at($00FE88)
ResetVector:
    sei
    clc
    xce
    cld
    jml.l BootColdStart

; @routine 00:FE90 InterruptReturn
; WHAT IT DOES: The handler of every interrupt the game does not use: the
; COP, BRK and ABORT vectors (native and emulation) and the emulation
; NMI and IRQ vectors all point here, and it returns at once. BRK runs
; here in the game (ObjectSlideGateUpStep's width-hazard paths).
; HOW: RTI.
; Entry: native mode, the interrupt's frame on the stack (P, then PC and
; PB). Exit: P, PC and PB as the interrupt pushed them; S 4 higher.
%at($00FE90)
InterruptReturn:
    rti
