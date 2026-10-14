; gauge.asm: the speed gauge. In the menus a value at $117E rises while
; button B is held and falls back when released, and slides toward a
; target; in a race it comes from the vehicle's speed. The result is kept
; in direct page $02 and copied to $0E49. Runs from work RAM bank $7F.
; C: src/stuntracefx/menu/gauge.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

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

; @routine 7F:112F SpeedGaugeUpdate
; WHAT IT DOES: Updates the speed gauge from A and copies it to $0E49:
; with $0D53 clear the gauge comes from the speed (GaugeFromSpeed, $02 =
; A), and while $0DD6 is below 6 and $117E is not negative it then slides
; toward it (MenuGaugeSlideToTarget); with $0D53 set and $0DD6 below 6
; the gauge follows button B (MenuGaugeFromButtonB), and with $0DD6 at 6
; or more it comes from the speed alone.
; HOW: $02 holds the value throughout; its low byte is stored at $0E49
; with 8-bit A. P is saved and restored.
; Entry: any widths, D = $0000; A = the speed's offset (16 bits are
; stored). Exit: A low byte = $02, high byte from the last step; X as
; GaugeFromSpeed, or unchanged; P unchanged.
%ramcode($7F112F, $05892F)
SpeedGaugeUpdate:
    php
    rep.b #$30
    sta.b !dp_02
    sep.b #$20
    lda.l !wram_0D53
    beq .l1149
    lda.l !engine_phase_p1
    cmp.b #$06
    bcs .l116A
    jsr.w MenuGaugeFromButtonB
    bra .l1160
.l1149:
    jsr.w GaugeFromSpeed
    sep.b #$20
    lda.l !engine_phase_p1
    cmp.b #$06
    bcs .l1160
    rep.b #$30
    lda.w !menu_gauge
    bmi .l1160
    jsr.w MenuGaugeSlideToTarget
.l1160:
    sep.b #$20
    lda.b !dp_02
    sta.l !speed_gauge
    plp
    rtl
.l116A:
    jsr.w GaugeFromSpeed
    sep.b #$20
    lda.b !dp_02
    sta.l !speed_gauge
    plp
    rtl

; @routine 7F:11E9 GaugeFromSpeed
; WHAT IT DOES: Turns a speed into the gauge: for the value at Super FX
; RAM $70:004A + $02 (made positive and halved when negative), capped at
; $257 and divided by 4, the byte at $0B:B158 + GaugeTableIndex; stored
; in direct page $02. When $02 is 0, $02 stays 0. The zero case and the
; cap are not reached in the traces so far; an LDA #0 after the BRA that
; ends the scaling is never run.
; HOW: P is saved and restored; the table byte is read with 8-bit A.
; Entry: any widths, D = $0000; $02 = the speed's offset. Exit: A = the
; gauge (0-$FF); X as GaugeTableIndex, unchanged when $02 is 0; P
; unchanged.
%ramcode($7F11E9, $0589E9)
GaugeFromSpeed:
    php
    rep.b #$30
    lda.b !dp_02
    cmp.w #$0000
    bne GaugeFromSpeed_11F6
    brl GaugeFromSpeed_121B
GaugeFromSpeed_11F6:
    tax
    lda.l !gsu_speeds,x
    bpl GaugeFromSpeed_1202
    eor.w #$FFFF
    inc a
    lsr a
GaugeFromSpeed_1202:
    cmp.w #$0257
    bcc GaugeFromSpeed_120A
    lda.w #$0257
GaugeFromSpeed_120A:
    lsr a
    lsr a
    bra GaugeFromSpeed_1211

; @routine 7F:120E GaugeFromSpeedZero
; WHAT IT DOES: GaugeFromSpeed's LDA #0 after the BRA that ends the
; scaling, which nothing reaches: the gauge for 0 (the byte at $0B:B158 +
; GaugeTableIndex of 0) into $02.
; HOW: As GaugeFromSpeed from there; ends pulling the P its start pushed.
; Entry: M 16-bit, X 16-bit, D = $0000; P on the stack above the return
; address. Exit: as GaugeFromSpeed.
GaugeFromSpeedZero:
    lda.w #$0000
GaugeFromSpeed_1211:
    tax
    jsr.w GaugeTableIndex
    sep.b #$20
    lda.l !gauge_table,x
GaugeFromSpeed_121B:
    rep.b #$20
    and.w #$00FF
    sta.b !dp_02
    plp
    rts

; @routine 7F:1224 GaugeTableIndex
; WHAT IT DOES: Returns X plus the gauge table offset for the kind of the
; vehicle object $0346 names: 0, $98, $130, $1C8 or $260 for the handlers
; $81B9, $853D, $8411, $82E5 and $8669 at its +$27, and 0 for any other.
; The last two kinds and the default are not reached in the traces so far.
; HOW: X is kept at $11DF; a compare chain on the 16-bit handler; P is
; saved and restored.
; Entry: any widths, DB a bank where $0346 is work RAM; X = the value.
; Exit: A = X = the index; P unchanged.
%ramcode($7F1224, $058A24)
GaugeTableIndex:
    php
    rep.b #$30
    txa
    sta.l !scratch_11df
    ldx.w !wram_0346
    lda.l !obj_handler_27,x
    cmp.w #$81B9
    beq .l1251
    cmp.w #$853D
    beq .l1256
    cmp.w #$8411
    beq .l125B
    cmp.w #$82E5
    beq .l1260
    cmp.w #$8669
    beq .l1265
    lda.w #$0000
    bra .l1268
.l1251:
    lda.w #$0000
    bra .l1268
.l1256:
    lda.w #$0098
    bra .l1268
.l125B:
    lda.w #$0130
    bra .l1268
.l1260:
    lda.w #$01C8
    bra .l1268
.l1265:
    lda.w #$0260
.l1268:
    clc
    adc.l !scratch_11df
    tax
    plp
    rts

; @routine 7F:0BEF SpeedGauge2Update
; WHAT IT DOES: Updates the second speed gauge (!speed_gauge_2_value) from A and copies
; it to $0E47. With $0D53 set it follows joypad 2's B button: up by $10
; (to $7F) while held, down by 6 (to 0) otherwise. With $0D53 clear the
; target in direct page $02 comes from the Super FX RAM values at A: the
; speed $70:004A / 4 (0 when negative) plus $70:0050 / 8 while $70:0044 is
; below 4, else half the speed (made positive), at most $FF. A negative
; gauge just shows the target; otherwise the gauge steps 4 toward it and
; becomes $FFFF (shows the target) once within 4. Several cases (A = 0,
; the negative values, the cap, the gauge below the target) are not
; reached in the traces so far.
; HOW: 16-bit arithmetic; the halving is CMP #$8000 then ROR; X is pushed
; and pulled. P is saved and restored. With A = 0 the value is stored with
; 16-bit A (also clearing $0E48).
; Entry: any widths, D = $0000; A = the offset of the values. Exit: A =
; the value shown (16 bits); X, Y, P unchanged.
%ramcode($7F0BEF, $0583EF)
SpeedGauge2Update:
    php
    rep.b #$20
    sta.b !dp_02
    sep.b #$20
    lda.w !wram_0D53
    beq .l0BFE
    brl .l0CA5
.l0BFE:
    rep.b #$30
    lda.b !dp_02
    phx
    cmp.w #$0000
    bne .l0C0B
    brl .l0C9E
.l0C0B:
    tax
    lda.l !gsu_speeds,x
    bmi .l0C18
    lsr a
    lsr a
    sta.b !dp_02
    bra .l0C1D
.l0C18:
    lda.w #$0000
    sta.b !dp_02
.l0C1D:
    lda.l !gsu_speed_44,x
    cmp.w #$0004
    bmi .l0C36
    lda.l !gsu_speeds,x
    cmp.w #$8000
    ror a
    bpl .l0C40
    eor.w #$FFFF
    inc a
    bra .l0C40
.l0C36:
    lda.l !gsu_speed_50,x
    lsr a
    lsr a
    lsr a
    clc
    adc.b !dp_02
.l0C40:
    cmp.w #$00FF
    bcc .l0C48
    lda.w #$00FF
.l0C48:
    sta.b !dp_02
    lda.w !speed_gauge_2_value
    bmi .l0C9A
    plx
    lda.w !speed_gauge_2_value
    cmp.b !dp_02
    beq .l0C85
    bcc .l0C70
    sec
    sbc.w #$0004
    sta.w !speed_gauge_2_value
    sec
    sbc.b !dp_02
    clc
    adc.w #$0004
    bmi .l0C8F
    cmp.w #$0008
    bcs .l0C8F
    bra .l0C85
.l0C70:
    clc
    adc.w #$0004
    sta.w !speed_gauge_2_value
    sec
    sbc.b !dp_02
    clc
    adc.w #$0004
    bmi .l0C8F
    cmp.w #$0008
    bcs .l0C8F
.l0C85:
    lda.w #$FFFF
    sta.w !speed_gauge_2_value
    lda.b !dp_02
    bra .l0C92
.l0C8F:
    lda.w !speed_gauge_2_value
.l0C92:
    sep.b #$20
    sta.l !speed_gauge_2
    plp
    rtl
.l0C9A:
    lda.b !dp_02
    sep.b #$20
.l0C9E:
    plx
    sta.l !speed_gauge_2
    plp
    rtl
.l0CA5:
    rep.b #$30
    lda.w !joy2_held
    bit.w #$8000
    bne .l0CC2
    lda.w !speed_gauge_2_value
    beq .l0CD4
    sec
    sbc.w #$0006
    bpl .l0CBD
    lda.w #$0000
.l0CBD:
    sta.w !speed_gauge_2_value
    bra .l0CD4
.l0CC2:
    lda.w !speed_gauge_2_value
    clc
    adc.w #$0010
    cmp.w #$007F
    bcc .l0CD1
    lda.w #$007F
.l0CD1:
    sta.w !speed_gauge_2_value
.l0CD4:
    lda.w !speed_gauge_2_value
    sep.b #$20
    sta.l !speed_gauge_2
    plp
    rtl
