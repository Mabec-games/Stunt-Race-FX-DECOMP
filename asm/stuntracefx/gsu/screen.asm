; screen.asm: the Super FX's screen buffers. The renderer draws into one of
; two buffers while the other is shown; $0D7F selects which, and $0D7D holds
; the matching offset (0 or 8).
; C: src/stuntracefx/gsu/screen.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 03:B83C GsuScreenFlip
; WHAT IT DOES: Swaps the Super FX's draw buffer and resets its screen base.
; HOW: Toggles $0D7F between 0 and 1 and sets $0D7D to 8 or 0 to match;
; gives RAM back to the S-CPU (screen mode value without bit 3), stores the
; buffer start $2C00 at $70:021A and direct page $48, hands ROM and RAM to
; the Super FX again (bits 4 and 3) and sets the screen base register to
; $0B. Work RAM is reached through the data bank; the screen mode stores
; are 16-bit.
; Entry: D = $0000. Exit: A = the high byte of the last screen mode value
; and $0B; M 8-bit; N, Z clear.
%at($03B83C)
GsuScreenFlip:
    rep.b #$20
    lda.w !gsu_buffer_select
    beq .lB872
    rep.b #$20
    stz.w !gsu_buffer_select
    lda.w #$0008
    sta.w !gsu_buffer_offset
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
    rts
.lB872:
    rep.b #$20
    inc.w !gsu_buffer_select
    lda.w #$0000
    sta.w !gsu_buffer_offset
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
    rts

; @routine 0B:FC49 GsuScreenFlipMenu
; WHAT IT DOES: The menu screens' form of GsuScreenFlip: swaps the Super
; FX's draw buffer between the one at $2C00 (screen base $0B) and the one
; at $9400 (screen base $25), leaving $0D7D alone.
; HOW: $0D7F set: clears it and selects $2C00; clear: sets it to 1 and
; selects $9400. The start goes to $70:021A and direct page $48 between
; giving RAM back to the S-CPU and handing ROM and RAM to the Super FX
; again; then the screen base register.
; Entry: D = $0000. Exit: A = the high byte of the last screen mode value
; and the screen base; M 8-bit; N, Z clear.
%at($0BFC49)
GsuScreenFlipMenu:
    rep.b #$20
    lda.w !gsu_buffer_select
    beq .lFC79
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
    rts
.lFC79:
    rep.b #$20
    inc.w !gsu_buffer_select
    lda.w !gsu_scmr_shadow
    and.w #$FFF7
    sta.w !GSU_SCMR
    lda.w #$9400
    sta.l !gsu_screen_start
    sta.b !gsu_screen_start_dp
    lda.w !gsu_scmr_shadow
    ora.w #$0018
    sta.w !GSU_SCMR
    sep.b #$20
    lda.b #$25
    sta.l !GSU_SCBR
    rts
