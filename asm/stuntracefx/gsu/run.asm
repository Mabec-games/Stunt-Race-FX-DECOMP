; run.asm: starting the Super FX and waiting for it. Runs from work RAM.
; C: src/stuntracefx/gsu/run.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"

arch 65816

; @routine 7E:E1CB GsuWait
; WHAT IT DOES: Waits for a running Super FX program to stop, then takes the
; ROM and RAM back for the S-CPU.
; HOW: Steps the 16-bit poll count and reads the status register until its
; running bit (5) clears, then writes the S-CPU's screen mode value.
; Entry: X 16-bit, D = $0000, DB a bank where $3030 is the Super FX.
; Exit: A low byte = the screen mode value; N, Z from it; M 8-bit.
%ramcode($7EE1CB, $013EF2)
GsuWait:
    sep.b #$20
.poll:
    inc.b !gsu_wait_count
    bne .counted
    inc.b !gsu_wait_count+1
.counted:
    lda.w !GSU_SFR
    and.b #$20
    bne .poll
    lda.w !gsu_scmr_shadow
    sta.w !GSU_SCMR
    rts

; @routine 7E:E1F5 GsuRun
; WHAT IT DOES: Runs a Super FX program to completion.
; HOW: Sets the program bank, gives ROM and RAM to the Super FX, clears its
; status and writes the start address to R15, which starts it; then waits as
; GsuWait does. The data bank is 0 inside and restored on return.
; Entry: M 8-bit, X 16-bit, D = $0000; A = program bank, X = start address.
; Exit: A low byte = the screen mode value; N, Z from the data bank pulled
; back on return; X unchanged.
%ramcode($7EE1F5, $013F1C)
GsuRun:
    sta.l !GSU_PBR
    phb
    lda.b #$00
    pha
    plb
    lda.w !gsu_scmr_shadow
    ora.b #$18
    sta.w !GSU_SCMR
    stz.w !GSU_SFR
    stx.w !GSU_R15
.poll:
    inc.b !gsu_wait_count
    bne .counted
    inc.b !gsu_wait_count+1
.counted:
    lda.w !GSU_SFR
    and.b #$20
    bne .poll
    lda.w !gsu_scmr_shadow
    sta.w !GSU_SCMR
    plb
    rtl

; @routine 7F:0919 GsuAB13WithParams
; WHAT IT DOES: Updates the renderer's parameter block, runs Super FX
; program $01:AB13 to completion and takes the bus back, counting the polls
; in $1F21-$1F22.
; HOW: GsuParamsUpdate, then as GsuRun but written out: program bank 1,
; screen mode $0374 | $18, R15 = $AB13; each poll steps the 16-bit count
; (low byte, then the high byte when the low one wraps) before reading the
; status. Everything but the parameter copy goes through the data bank.
; Entry: any widths, D = $0000. Exit: A low byte = the screen mode value,
; high byte from the last parameter copied; X = $AB13 ($13 when P
; restores 8-bit index registers); P unchanged.
%ramcode($7F0919, $058119)
GsuAB13WithParams:
    php
    jsl.l GsuParamsUpdate
    sep.b #$20
    rep.b #$10
    lda.b #$01
    ldx.w #$AB13
    sta.w !GSU_PBR
    lda.w !gsu_scmr_shadow
    ora.b #$18
    sta.w !GSU_SCMR
    stx.w !GSU_R15
    sep.b #$20
.l0937:
    inc.w !gsu_ab13_polls
    bne .l093F
    inc.w !gsu_ab13_polls+1
.l093F:
    lda.w !GSU_SFR
    and.b #$20
    bne .l0937
    lda.w !gsu_scmr_shadow
    sta.w !GSU_SCMR
    plp
    rtl
