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
    jsl.l MemoryFill
    lda.b #$7E
    ldx.w #$2000
    ldy.w #$E000
    jsl.l MemoryFill
    lda.b #$7F
    ldx.w #$0000
    ldy.w #$FFFF
    jsl.l MemoryFill
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
