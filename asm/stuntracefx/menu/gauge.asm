; gauge.asm: a menu value at $117E that rises while button B is held and falls
; back when released, and slides toward a target; the menu frame code reads
; the result from direct page $02. Runs from work RAM bank $7F.
; C: src/stuntracefx/menu/gauge.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"

arch 65816

; @routine 7F:1177 MenuGaugeSlideToTarget
; WHAT IT DOES: Steps the menu gauge toward the target in direct page $02:
; down by 8 when above it, up by 4 when below. If the stepped gauge is at or
; above the target, $02 takes it; if it is below (or was equal to start
; with), the gauge becomes $FF00 (idle).
; HOW: Compares with the target, steps, and tests the sign of gauge minus
; target.
; Entry: any widths, D = $0000. Exit: A = $FF00 or the new value; P unchanged.
%ramcode($7F1177, $058977)
MenuGaugeSlideToTarget:
    php
    rep.b #$30
    lda.w !menu_gauge
    cmp.b !dp_02
    beq .idle
    bcc .up
    sec
    sbc.w #$0008
    sta.w !menu_gauge
    sec
    sbc.b !dp_02
    bpl .keep
    bra .idle
.up:
    clc
    adc.w #$0004
    sta.w !menu_gauge
    sec
    sbc.b !dp_02
    bpl .keep
.idle:
    lda.w #$FF00
    sta.w !menu_gauge
    bra .done
.keep:
    lda.w !menu_gauge
    sta.b !dp_02
.done:
    plp
    rts

; @routine 7F:11AC MenuGaugeFromButtonB
; WHAT IT DOES: Raises the menu gauge by $28 (up to $FF) while joypad 1's B
; button is held, otherwise lowers it by $0C (down to 0), and returns it in
; direct page $02.
; HOW: Tests bit 15 of joypad 1's held buttons.
; Entry: any widths, D = $0000. Exit: A = the gauge; P unchanged.
MenuGaugeFromButtonB:
    php
    rep.b #$30
    lda.l !joy1_held
    and.w #$8000
    bne .held
    lda.l !menu_gauge
    beq .done
    sec
    sbc.w #$000C
    bpl .down
    lda.w #$0000
.down:
    sta.l !menu_gauge
    bra .done
.held:
    lda.l !menu_gauge
    clc
    adc.w #$0028
    cmp.w #$00FF
    bcc .up
    lda.w #$00FF
.up:
    sta.l !menu_gauge
.done:
    lda.l !menu_gauge
    sta.b !dp_02
    plp
    rts
