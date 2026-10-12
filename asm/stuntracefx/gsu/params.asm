; params.asm: the parameter block in Super FX RAM that the renderer reads.
; C: src/stuntracefx/gsu/params.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"

arch 65816

; @routine 7E:E2C9 GsuParamsUpdate
; WHAT IT DOES: Copies the view parameters the renderer needs from work RAM
; into its parameter block in Super FX RAM.
; HOW: Eight 16-bit copies: $0D12 to $70:108A; direct page $C5, $C7, $C9 to
; $70:00D8, 00DA, 00DC; $0664, $0666, $0668 to $70:00DE, 00E0, 00E2; $05FA to
; $70:2736.
; Entry: any widths, D = $0000. Exit: A = the last value copied; P unchanged.
%ramcode($7EE2C9, $013FF0)
GsuParamsUpdate:
    php
    rep.b #$30
    lda.w !wram_0D12
    sta.l !gsu_param_108a
    lda.b !dp_c5
    sta.l !gsu_param_00d8
    lda.b !dp_c7
    sta.l !gsu_param_00da
    lda.b !dp_c9
    sta.l !gsu_param_00dc
    lda.w !wram_0664
    sta.l !gsu_param_00de
    lda.w !wram_0666
    sta.l !gsu_param_00e0
    lda.w !wram_0668
    sta.l !gsu_param_00e2
    lda.w !wram_05FA
    sta.l !gsu_param_2736
    plp
    rtl

; @routine 03:AEB6 GsuParamsFromDp
; WHAT IT DOES: Marks $05EF with $FF and copies eight direct-page words into
; the renderer's inputs at $70:0034-$70:003E, $70:0092 and $70:0094.
; HOW: Word copies from $CE, $D0, $2A, $2E, $2C, $30, $32, $34, in that
; order; $05EF is reached through the data bank.
; Entry: M 8-bit, D = $0000. Exit: A = the last word; M 16-bit; N, Z from A.
%at($03AEB6)
GsuParamsFromDp:
    lda.b #$FF
    sta.w !wram_05EF
    rep.b #$20
    lda.b !dp_ce
    sta.l !gsu_in_34
    lda.b !dp_d0
    sta.l !gsu_in_36
    lda.b !dp_2a
    sta.l !gsu_in_38
    lda.b !dp_2e
    sta.l !gsu_in_3a
    lda.b !dp_2c
    sta.l !gsu_in_3c
    lda.b !dp_30
    sta.l !gsu_in_3e
    lda.b !dp_32
    sta.l !gsu_in_92
    lda.b !dp_34
    sta.l !gsu_in_94
    rtl
