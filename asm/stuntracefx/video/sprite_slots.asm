; sprite_slots.asm: eight 8-byte records at $0595-$05D4, after the work-RAM
; sprite table, and the mask of the ones in use at !slot_mask (bit i for the
; record at $0595 + i * 8). !slot_first points at the first one in use.
; Fields used here: +$00, +$04, +$05, +$06 and +$07 (bytes).
; C: src/stuntracefx/video/sprite_slots.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 02:D928 SpriteSlotFirstLong
; WHAT IT DOES: SpriteSlotFirst for callers in other banks.
; HOW: 8-bit A, 16-bit index registers around the call; P is saved and
; restored.
; Entry: any widths. Exit: A, X as SpriteSlotFirst; P unchanged.
%at($02D928)
SpriteSlotFirstLong:
    php
    sep.b #$20
    rep.b #$10
    jsr.w SpriteSlotFirst
    plp
    rtl

; @routine 02:D932 SpriteSlotFirst
; WHAT IT DOES: Sets !slot_first to the address of the first record in use (the
; lowest set bit of !slot_mask), or 0 when none is. Not reached in the traces
; so far with any record in use.
; HOW: The mask is copied to the word at $1F2B ($1F2C cleared) and shifted
; right through the carry (ROR, so the entry carry goes into bit 15)
; eight times, the address stepping by 8 from $0595; work RAM through the
; data bank.
; Entry: M 8-bit, X 16-bit. Exit: A = !slot_first's new value; M 16-bit. Found:
; X = 8 - the bit number; N, Z from the shifted word; C set. None: X
; unchanged, or 0 after eight shifts; Z set, N clear; C from the last
; shift or add.
%at($02D932)
SpriteSlotFirst:
    stz.w !slot_scan+1
    lda.w !slot_mask
    sta.w !slot_scan
    rep.b #$20
    beq .lD950
    ldx.w #$0008
    lda.w #$0595
.lD945:
    ror.w !slot_scan
    bcs .lD953
    adc.w #$0008
    dex
    bne .lD945
.lD950:
    lda.w #$0000
.lD953:
    sta.w !slot_first
    rts

; @routine 02:E0A9 SpriteSlot1Countdown
; WHAT IT DOES: Runs record 1 ($059D) while the count at !slot1_count is not $FF:
; marks it in use and sets +$04 = 0, +$05 = 2, +$06 = $A7, +$00 = +$07 OR
; $E0. While the count is not 0 it is stepped down, with +$06 = $E7 and
; +$00 = $FF. At 0: the first time ($7E:FF65 = 0) $7E:FF65 is set to 1 and
; the same is done; later +$07 steps down by 2 each call until 0, then the
; count steps on to $FF. A count of $FF marks the record unused. The paths
; at 0 are not reached in the traces so far.
; HOW: The record's fields as direct page offsets indexed by X = $059D (D
; = $0000); work RAM through the data bank. P is saved and restored.
; Entry: D = $0000. Exit: A low byte = the last value loaded; X = $059D,
; unchanged when the count is $FF; P unchanged.
%at($02E0A9)
SpriteSlot1Countdown:
    php
    sep.b #$20
    rep.b #$10
    lda.w !slot1_count
    inc a
    beq .lE106
    sep.b #$20
    rep.b #$10
    lda.w !slot_mask
    ora.b #$02
    sta.w !slot_mask
    ldx.w #$059D
    sep.b #$20
    rep.b #$10
    stz.b !slot_04,x
    lda.b #$02
    sta.b !slot_05,x
    lda.b #$A7
    sta.b !slot_06,x
    lda.b !slot_07,x
    ora.b #$E0
    sta.b !slot_00,x
    lda.w !slot1_count
    bne .lE0F7
    lda.l !palette_saved
    beq .lE0EC
    lda.b !slot_07,x
    beq .lE101
    dec a
    dec a
    sta.b !slot_07,x
    bra .lE104
.lE0EC:
    inc a
    sta.l !palette_saved
    rep.b #$20
    sep.b #$20
    lda.b #$00
.lE0F7:
    stz.b !slot_04,x
    lda.b #$E7
    sta.b !slot_06,x
    lda.b #$FF
    sta.b !slot_00,x
.lE101:
    dec.w !slot1_count
.lE104:
    plp
    rtl
.lE106:
    sep.b #$20
    lda.w !slot_mask
    and.b #$FD
    sta.w !slot_mask
    sep.b #$20
    rep.b #$10
    jmp.w .lE104

; @routine 7E:F33C SpriteSlot4Brighten
; WHAT IT DOES: Shows record 4 ($05B5) and brightens it a step: marks it in
; use and sets +$04 = 0, +$05 = 2, +$06 = $E6, +$00 = +$07 OR $E0, then
; steps +$07 up by 1 until it reaches $0F. Nothing in the game calls it.
; HOW: The record's fields as direct page offsets indexed by X = $05B5 (D
; = $0000); the mask through the data bank. P and X are saved and restored.
; Entry: D = $0000. Exit: A low byte = +$07's new value; X, P unchanged.
%ramcode($7EF33C, $015063)
SpriteSlot4Brighten:
    php
    sep.b #$20
    rep.b #$10
    phx
    sep.b #$20
    rep.b #$10
    lda.w !slot_mask
    ora.b #$10
    sta.w !slot_mask
    ldx.w #$05B5
    sep.b #$20
    rep.b #$10
    stz.b !slot_04,x
    lda.b #$02
    sta.b !slot_05,x
    lda.b #$E6
    sta.b !slot_06,x
    lda.b #$E0
    ora.b !slot_07,x
    sta.b !slot_00,x
    lda.b !slot_07,x
    cmp.b #$0F
    beq .done
    inc a
    sta.b !slot_07,x
.done:
    plx
    plp
    rts

; @routine 7E:F371 SpriteSlot4Darken
; WHAT IT DOES: Shows record 4 ($05B5) and darkens it a step: marks it in
; use and sets +$04 = 0, +$05 = 2, +$06 = $E6, +$00 = +$07 OR $E0, then
; steps +$07 down by 1; once +$07 is 0 the record is marked unused
; instead. Nothing in the game calls it.
; HOW: The record's fields as direct page offsets indexed by X = $05B5 (D
; = $0000); the mask through the data bank. P and X are saved and restored.
; Entry: D = $0000. Exit: A low byte = +$07's new value, or the new mask
; at 0; X, P unchanged.
%ramcode($7EF371, $015098)
SpriteSlot4Darken:
    php
    sep.b #$20
    rep.b #$10
    phx
    sep.b #$20
    rep.b #$10
    lda.w !slot_mask
    ora.b #$10
    sta.w !slot_mask
    ldx.w #$05B5
    sep.b #$20
    rep.b #$10
    stz.b !slot_04,x
    lda.b #$02
    sta.b !slot_05,x
    lda.b #$E6
    sta.b !slot_06,x
    lda.b #$E0
    ora.b !slot_07,x
    sta.b !slot_00,x
    lda.b !slot_07,x
    beq .off
    dec a
    sta.b !slot_07,x
    plx
    plp
    rts
.off:
    sep.b #$20
    lda.w !slot_mask
    and.b #$EF
    sta.w !slot_mask
    sep.b #$20
    rep.b #$10
    plx
    plp
    rts

; @routine 7E:F3B5 SpriteSlot4Full
; WHAT IT DOES: Shows record 4 ($05B5) at full brightness: marks it in use
; and sets +$04 = 0, +$05 = 2, +$06 = $E6, +$00 = $FF and +$07 = $1F.
; Nothing in the game calls it.
; HOW: The record's fields as direct page offsets indexed by X = $05B5 (D
; = $0000); the mask through the data bank. P and X are saved and restored.
; Entry: D = $0000. Exit: A low byte = $1F; X, P unchanged.
%ramcode($7EF3B5, $0150DC)
SpriteSlot4Full:
    php
    sep.b #$20
    rep.b #$10
    phx
    sep.b #$20
    rep.b #$10
    lda.w !slot_mask
    ora.b #$10
    sta.w !slot_mask
    ldx.w #$05B5
    sep.b #$20
    rep.b #$10
    stz.b !slot_04,x
    lda.b #$02
    sta.b !slot_05,x
    lda.b #$E6
    sta.b !slot_06,x
    lda.b #$FF
    sta.b !slot_00,x
    lda.b #$1F
    sta.b !slot_07,x
    plx
    plp
    rts

; @routine 02:E042 SpriteSlot2BrightenLong
; WHAT IT DOES: SpriteSlot2Brighten for callers in other banks.
; HOW: JSR, RTL.
; Entry: D = $0000. Exit: as SpriteSlot2Brighten.
%at($02E042)
SpriteSlot2BrightenLong:
    jsr.w SpriteSlot2Brighten
    rtl

; @routine 02:E046 SpriteSlot2Brighten
; WHAT IT DOES: Shows record 2 ($05A5) and brightens it a step: marks it in
; use and sets +$04 = 0, +$05 = 2, +$06 = $07, +$00 = +$07 OR $E0, then
; steps +$07 up by 1 until it reaches $1F. Nothing in the game calls it.
; HOW: As SpriteSlot4Brighten, for record 2.
; Entry: D = $0000. Exit: A low byte = +$07's new value; X, P unchanged.
SpriteSlot2Brighten:
    php
    sep.b #$20
    rep.b #$10
    phx
    sep.b #$20
    rep.b #$10
    lda.w !slot_mask
    ora.b #$04
    sta.w !slot_mask
    ldx.w #$05A5
    sep.b #$20
    rep.b #$10
    stz.b !slot_04,x
    lda.b #$02
    sta.b !slot_05,x
    lda.b #$07
    sta.b !slot_06,x
    lda.b #$E0
    ora.b !slot_07,x
    sta.b !slot_00,x
    lda.b !slot_07,x
    cmp.b #$1F
    beq .lE078
    inc a
    sta.b !slot_07,x
.lE078:
    plx
    plp
    rts

; @routine 02:E07B SpriteSlot1Start
; WHAT IT DOES: Starts record 1's countdown: $08FB = $28, record 1 marked
; in use with +$07 = $1E and +$04 = 0, then a first SpriteSlot1Countdown;
; $084C is cleared. Nothing in the game calls it.
; HOW: 8-bit stores through the data bank and direct page (D = $0000).
; P is saved and restored.
; Entry: D = $0000. Exit: A low byte and X as SpriteSlot1Countdown leaves
; them (X's low byte when X is 8-bit); P unchanged.
SpriteSlot1Start:
    php
    sep.b #$20
    rep.b #$10
    lda.b #$28
    sta.w !slot1_count
    sep.b #$20
    rep.b #$10
    lda.w !slot_mask
    ora.b #$02
    sta.w !slot_mask
    ldx.w #$059D
    sep.b #$20
    rep.b #$10
    lda.b #$1E
    sta.b !slot_07,x
    stz.b !slot_04,x
    jsl.l SpriteSlot1Countdown
    sep.b #$20
    stz.w !wram_084C
    plp
    rtl

; @routine 02:E117 SpriteSlot6Yellow
; WHAT IT DOES: Shows record 6 ($05C5) with +$04 = 0, +$05 = 2, +$06 =
; $27, +$01 = 0, +$02 = +$03 = $1F and +$00 = 0. Nothing in the game
; calls it.
; HOW: 8-bit stores through the data bank and direct page (D = $0000);
; P and X are saved and restored.
; Entry: D = $0000. Exit: A low byte = $1F; X, P unchanged.
%at($02E117)
SpriteSlot6Yellow:
    php
    sep.b #$20
    rep.b #$10
    phx
    sep.b #$20
    rep.b #$10
    lda.w !slot_mask
    ora.b #$40
    sta.w !slot_mask
    ldx.w #$05C5
    sep.b #$20
    rep.b #$10
    stz.b !slot_04,x
    lda.b #$02
    sta.b !slot_05,x
    lda.b #$27
    sta.b !slot_06,x
    stz.b !slot_01,x
    lda.b #$1F
    sta.b !slot_02,x
    sta.b !slot_03,x
    stz.b !slot_00,x
    plx
    plp
    rtl

; @routine 02:E147 SpriteSlot6DimYellow
; WHAT IT DOES: As SpriteSlot6Yellow with +$02 = +$03 = $07. Nothing in
; the game calls it.
; HOW: As SpriteSlot6Yellow.
; Entry: D = $0000. Exit: A low byte = $07; X, P unchanged.
SpriteSlot6DimYellow:
    php
    sep.b #$20
    rep.b #$10
    phx
    sep.b #$20
    rep.b #$10
    lda.w !slot_mask
    ora.b #$40
    sta.w !slot_mask
    ldx.w #$05C5
    sep.b #$20
    rep.b #$10
    stz.b !slot_04,x
    lda.b #$02
    sta.b !slot_05,x
    lda.b #$27
    sta.b !slot_06,x
    stz.b !slot_01,x
    lda.b #$07
    sta.b !slot_02,x
    sta.b !slot_03,x
    stz.b !slot_00,x
    plx
    plp
    rtl

; @routine 02:E177 SpriteSlot6Red
; WHAT IT DOES: Shows record 6 ($05C5) with +$04 = 0, +$05 = 2, +$06 =
; $27, +$00 = 0, +$01 = $1F and +$02 = +$03 = 0. Nothing in the game
; calls it.
; HOW: As SpriteSlot6Yellow, +$00 stored first.
; Entry: D = $0000. Exit: A low byte = $1F; X, P unchanged.
SpriteSlot6Red:
    php
    sep.b #$20
    rep.b #$10
    phx
    sep.b #$20
    rep.b #$10
    lda.w !slot_mask
    ora.b #$40
    sta.w !slot_mask
    ldx.w #$05C5
    sep.b #$20
    rep.b #$10
    stz.b !slot_04,x
    lda.b #$02
    sta.b !slot_05,x
    lda.b #$27
    sta.b !slot_06,x
    stz.b !slot_00,x
    lda.b #$1F
    sta.b !slot_01,x
    stz.b !slot_02,x
    stz.b !slot_03,x
    plx
    plp
    rtl

; @routine 02:E1A7 SpriteSlot5DimRed
; WHAT IT DOES: Shows record 5 ($05BD) with +$04 = 0, +$05 = 2, +$06 =
; $27, +$00 = 0, +$01 = $0A and +$02 = +$03 = 0. Nothing in the game
; calls it.
; HOW: As SpriteSlot6Red, for record 5.
; Entry: D = $0000. Exit: A low byte = $0A; X, P unchanged.
SpriteSlot5DimRed:
    php
    sep.b #$20
    rep.b #$10
    phx
    sep.b #$20
    rep.b #$10
    lda.w !slot_mask
    ora.b #$20
    sta.w !slot_mask
    ldx.w #$05BD
    sep.b #$20
    rep.b #$10
    stz.b !slot_04,x
    lda.b #$02
    sta.b !slot_05,x
    lda.b #$27
    sta.b !slot_06,x
    stz.b !slot_00,x
    lda.b #$0A
    sta.b !slot_01,x
    stz.b !slot_02,x
    stz.b !slot_03,x
    plx
    plp
    rtl

; @routine 02:E1D7 SpriteSlot5Off
; WHAT IT DOES: Marks record 5 unused. Nothing in the game calls it.
; HOW: Clears bit 5 of the mask through the data bank; P and X are saved
; and restored.
; Entry: any widths. Exit: A low byte = the new mask; X, P unchanged.
SpriteSlot5Off:
    php
    sep.b #$20
    rep.b #$10
    phx
    sep.b #$20
    lda.w !slot_mask
    and.b #$DF
    sta.w !slot_mask
    sep.b #$20
    rep.b #$10
    plx
    plp
    rtl

; @routine 02:E1EE SpriteSlot2FadeOut
; WHAT IT DOES: Starts record 2's fade out from full: marks it in use with
; +$07 = $1F, then SpriteSlot2Darken. Nothing in the game calls it.
; HOW: P and X saved and restored around the first stores; JSR to
; SpriteSlot2Darken, RTL.
; Entry: D = $0000. Exit: as SpriteSlot2Darken.
SpriteSlot2FadeOut:
    php
    sep.b #$20
    rep.b #$10
    phx
    sep.b #$20
    rep.b #$10
    lda.w !slot_mask
    ora.b #$04
    sta.w !slot_mask
    ldx.w #$05A5
    sep.b #$20
    rep.b #$10
    lda.b #$1F
    sta.b !slot_07,x
    plx
    plp
    jsr.w SpriteSlot2Darken
    rtl

; @routine 02:E211 SpriteSlot2DarkenLong
; WHAT IT DOES: SpriteSlot2Darken for callers in other banks.
; HOW: JSR, RTL.
; Entry: D = $0000. Exit: as SpriteSlot2Darken.
SpriteSlot2DarkenLong:
    jsr.w SpriteSlot2Darken
    rtl

; @routine 02:E215 SpriteSlot2Darken
; WHAT IT DOES: Shows record 2 ($05A5) and darkens it a step: marks it in
; use and sets +$04 = 0, +$05 = 2, +$06 = $07, +$00 = +$07 OR $E0, then
; steps +$07 down by 1; once +$07 is 0 the record is marked unused
; instead. Nothing in the game calls it.
; HOW: As SpriteSlot4Darken, for record 2.
; Entry: D = $0000. Exit: A low byte = +$07's new value, or the new mask
; at 0; X, P unchanged.
SpriteSlot2Darken:
    php
    sep.b #$20
    rep.b #$10
    phx
    sep.b #$20
    rep.b #$10
    lda.w !slot_mask
    ora.b #$04
    sta.w !slot_mask
    ldx.w #$05A5
    sep.b #$20
    rep.b #$10
    stz.b !slot_04,x
    lda.b #$02
    sta.b !slot_05,x
    lda.b #$07
    sta.b !slot_06,x
    lda.b #$E0
    ora.b !slot_07,x
    sta.b !slot_00,x
    lda.b !slot_07,x
    beq .lE248
    dec a
    sta.b !slot_07,x
    plx
    plp
    rts
.lE248:
    sep.b #$20
    lda.w !slot_mask
    and.b #$FB
    sta.w !slot_mask
    sep.b #$20
    rep.b #$10
    plx
    plp
    rts

; @routine 02:E259 SpriteSlot5Yellow
; WHAT IT DOES: As SpriteSlot6Yellow for record 5 ($05BD). Nothing in the
; game calls it.
; HOW: As SpriteSlot6Yellow.
; Entry: D = $0000. Exit: A low byte = $1F; X, P unchanged.
SpriteSlot5Yellow:
    php
    sep.b #$20
    rep.b #$10
    phx
    sep.b #$20
    rep.b #$10
    lda.w !slot_mask
    ora.b #$20
    sta.w !slot_mask
    ldx.w #$05BD
    sep.b #$20
    rep.b #$10
    stz.b !slot_04,x
    lda.b #$02
    sta.b !slot_05,x
    lda.b #$27
    sta.b !slot_06,x
    stz.b !slot_01,x
    lda.b #$1F
    sta.b !slot_02,x
    sta.b !slot_03,x
    stz.b !slot_00,x
    plx
    plp
    rtl
