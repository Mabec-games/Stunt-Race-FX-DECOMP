; programs.asm: S-CPU wrappers that set a Super FX program's inputs in Super
; FX RAM, run it with GsuRun, and collect what it leaves there.
;
; Program $01:CF42 draws into a buffer whose address it takes at $70:002C
; (with inputs at $70:0068, $70:006A and a zero at $70:00A2); $01:80BC and
; $01:82CE take a vector and return results; $01:8325 fills nine words at
; $70:00E4; $07:EE88 reads an eight-word ramp at $70:3C00.
; C: src/stuntracefx/gsu/programs.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 03:EBEF GsuCF42ToC000
; WHAT IT DOES: Runs Super FX program $01:CF42 drawing to $C000, with inputs
; A and X.
; HOW: $70:0068 = A, $70:002C = $C000, $70:00A2 = 0, $70:006A = X, then
; GsuRun.
; Entry: M 16-bit, X 16-bit, D = $0000. Exit: A = X's high byte and the
; screen mode value; X = $CF42; M 8-bit; N, Z from the data bank.
%at($03EBEF)
GsuCF42ToC000:
    sta.l !gsu_in_68
    lda.w #$C000
    bra GsuCF42_dest

; @routine 03:EBF8 GsuCF42To4000
; WHAT IT DOES: As GsuCF42ToC000, drawing to $4000.
; HOW: As GsuCF42ToC000.
; Entry: as GsuCF42ToC000. Exit: as GsuCF42ToC000.
%at($03EBF8)
GsuCF42To4000:
    sta.l !gsu_in_68
    lda.w #$4000
    bra GsuCF42_dest

; @routine 03:EC01 GsuCF42To3000
; WHAT IT DOES: As GsuCF42ToC000, drawing to $3000.
; HOW: As GsuCF42ToC000.
; Entry: as GsuCF42ToC000. Exit: as GsuCF42ToC000.
%at($03EC01)
GsuCF42To3000:
    sta.l !gsu_in_68
    lda.w #$3000
GsuCF42_dest:                       ; GsuCF42ToC000 and GsuCF42To4000 join here
    sta.l !gsu_dest
    lda.w #$0000
    sta.l !gsu_in_a2
    txa
    sta.l !gsu_in_6a
    sep.b #$20
    lda.b #$01
    ldx.w #$CF42
    jsl.l GsuRun
    rtl

; @routine 03:EB83 GsuCF42ToVram
; WHAT IT DOES: Runs program $01:CF42 drawing to $70:3000 and copies the
; result to VRAM by DMA.
; HOW: The inputs come from the record at X in the data bank (word at +0 to
; $70:0068, byte at +2 to $70:006A); after GsuRun, the transfer comes from
; the record at Y: VMAIN, the VRAM address, DMA mode and B-bus register, and
; the size, with the source fixed at $70:3000.
; Entry: M 16-bit, X 16-bit; X = input record, Y = transfer record. Exit: A =
; 1; X = Y; M 8-bit, X 16-bit; N, Z clear.
%at($03EB83)
GsuCF42ToVram:
    phy
    lda.w 0,x
    sta.l !gsu_in_68
    lda.w #$3000
    sta.l !gsu_dest
    lda.w #$0000
    sta.l !gsu_in_a2
    sep.b #$20
    lda.w 2,x
    sta.l !gsu_in_6a
    lda.b #$01
    ldx.w #$CF42
    jsl.l GsuRun
    sep.b #$20
    rep.b #$10
    plx
    lda.w 0,x
    sta.w !VMAIN
    lda.w 1,x
    sta.w !VMADDL
    lda.w 2,x
    sta.w !VMADDH
    lda.w 3,x
    sta.w !DMAP0
    lda.w 4,x
    sta.w !BBAD0
    lda.b #$00
    sta.w !A1T0L
    lda.b #$30
    sta.w !A1T0H
    lda.b #$70
    sta.w !A1B0
    lda.w 5,x
    sta.w !DAS0L
    lda.w 6,x
    sta.w !DAS0H
    lda.b #$01
    sta.w !MDMAEN
    rtl

; @routine 03:C9EE ObjectGsu80BC
; WHAT IT DOES: Runs Super FX program $01:80BC on object X's offset from a
; point and returns its result.
; HOW: $70:0068 = direct page $04 - the object's +$0C; $70:002E = $05F2 -
; its +$10 (both through the data bank); GsuRun; the result is $70:006A.
; Entry: M 16-bit, X 16-bit, D = $0000; X = object. Exit: A = the result;
; X unchanged; M 16-bit, X 16-bit; N, Z from A; C, V from the second
; subtract.
%at($03C9EE)
ObjectGsu80BC:
    lda.b !dp_04
    sec
    sbc.w !obj_pos_x,x
    sta.l !gsu_in_68
    lda.w !rot_z
    sec
    sbc.w !obj_pos_z,x
    sta.l !gsu_in_2e
    phx
    sep.b #$20
    lda.b #$01
    ldx.w #$80BC
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    plx
    lda.l !gsu_in_6a
    rtl

; @routine 03:AFD7 Gsu82CE
; WHAT IT DOES: Runs Super FX program $01:82CE on the vector in direct page
; $02, $08, $94 and puts its three results in direct page $B7, $B9, $BB.
; HOW: Inputs to $70:0068, $70:002C, $70:002E; GsuRun; results from
; $70:0026, $70:0028, $70:002A.
; Entry: any widths, D = $0000. Exit: A = the third result; X, Y, P
; unchanged.
%at($03AFD7)
Gsu82CE:
    phx
    phy
    php
    rep.b #$30
    lda.b !dp_02
    sta.l !gsu_in_68
    lda.b !dp_08
    sta.l !gsu_dest
    lda.b !dp_94
    sta.l !gsu_in_2e
    sep.b #$20
    lda.b #$01
    ldx.w #$82CE
    jsl.l GsuRun
    rep.b #$20
    lda.l !gsu_out_26
    sta.b !dp_b7
    lda.l !gsu_out_28
    sta.b !dp_b9
    lda.l !gsu_out_2a
    sta.b !dp_bb
    plp
    ply
    plx
    rtl

; @routine 03:B011 Gsu8325
; WHAT IT DOES: Runs Super FX program $01:8325 on the three words at $065E
; and copies the nine words it leaves at $70:00E4 to $0608.
; HOW: Inputs to $70:0020-$70:0024; GsuRun; nine word copies. Work RAM is
; reached through the data bank.
; Entry: X 16-bit, D = $0000. Exit: A = the last word; X = $25, Y high byte
; clear; M 8-bit, X 8-bit; N, Z from A (16-bit).
%at($03B011)
Gsu8325:
    rep.b #$20
    lda.w !wram_065E
    sta.l !gsu_in_20
    lda.w !wram_065E+2
    sta.l !gsu_in_22
    lda.w !wram_065E+4
    sta.l !gsu_in_24
    sep.b #$20
    rep.b #$10
    lda.b #$01
    ldx.w #$8325
    jsl.l GsuRun
    rep.b #$20
    lda.l !gsu_out_e4
    sta.w !gsu_8325_out
    lda.l !gsu_out_e4+2
    sta.w !gsu_8325_out+2
    lda.l !gsu_out_e4+4
    sta.w !gsu_8325_out+4
    lda.l !gsu_out_e4+6
    sta.w !gsu_8325_out+6
    lda.l !gsu_out_e4+8
    sta.w !gsu_8325_out+8
    lda.l !gsu_out_e4+10
    sta.w !gsu_8325_out+10
    lda.l !gsu_out_e4+12
    sta.w !gsu_8325_out+12
    lda.l !gsu_out_e4+14
    sta.w !gsu_8325_out+14
    lda.l !gsu_out_e4+16
    sta.w !gsu_8325_out+16
    sep.b #$30
    rtl

; @routine 03:FE8D GsuEE88Ramp
; WHAT IT DOES: Writes the ramp $80, $90, ... $F0 to $70:3C00 and runs Super
; FX program $07:EE88.
; HOW: Eight word stores, then GsuRun.
; Entry: any widths. Exit: A = the screen mode value (high byte clear); X =
; $88, Y high byte clear; M 16-bit, X 8-bit; N, Z from the data bank.
%at($03FE8D)
GsuEE88Ramp:
    rep.b #$20
    lda.w #$0080
    sta.l !gsu_ramp
    lda.w #$0090
    sta.l !gsu_ramp+2
    lda.w #$00A0
    sta.l !gsu_ramp+4
    lda.w #$00B0
    sta.l !gsu_ramp+6
    lda.w #$00C0
    sta.l !gsu_ramp+8
    lda.w #$00D0
    sta.l !gsu_ramp+10
    lda.w #$00E0
    sta.l !gsu_ramp+12
    lda.w #$00F0
    sta.l !gsu_ramp+14
    sep.b #$20
    rep.b #$10
    ldx.w #$EE88
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    sep.b #$10
    rtl

; @routine 07:8B9B GsuF329Entry
; WHAT IT DOES: Runs Super FX program $07:F329 with the renderer input at
; $70:0216 set to entry A (low four bits) of the table at $8E52.
; HOW: GsuParam216B, then GsuRun.
; Entry: M 16-bit, X 16-bit, D = $0000; A = entry. Exit: A = the table
; word's high byte and the screen mode value; X = $F329; C clear; N, Z
; from the data bank.
%at($078B9B)
GsuF329Entry:
    and.w #$000F
    jsr.w GsuParam216B
    sep.b #$20
    rep.b #$10
    ldx.w #$F329
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rts

; @routine 07:A684 GsuF441Player1
; WHAT IT DOES: Runs Super FX program $07:F441 for player 1, with the
; inputs chosen by the low four bits of $0DF4 (1, or anything else).
; HOW: Kind 1: GsuBytes200_0F050E and $70:0216 = $080E; otherwise
; GsuBytes200_0F080E and $0A0E. Then $70:020A = $74, $70:020C = $3C and
; GsuRun.
; Entry: any widths, D = $0000. Exit: A = the screen mode value (high byte
; clear); X = $F441 ($41 when X is 8-bit); Y high byte clear when X is
; 8-bit; P unchanged.
%at($07A684)
GsuF441Player1:
    php
    rep.b #$30
    lda.w !player_mode
    and.w #$000F
    cmp.w #$0001
    beq GsuF441_kind1
    bra GsuF441_other

; @routine 07:A694 GsuF441Player2
; WHAT IT DOES: As GsuF441Player1, for player 2 ($0DF5).
; HOW: The same tail as GsuF441Player1.
; Entry: any widths, D = $0000. Exit: A = the screen mode value (high byte
; clear); X = $F441 ($41 when X is 8-bit); Y high byte clear when X is
; 8-bit; P unchanged.
%at($07A694)
GsuF441Player2:
    php
    rep.b #$30
    lda.w !player_mode+1
    and.w #$000F
    cmp.w #$0001
    beq GsuF441_kind1
    bra GsuF441_other
GsuF441_kind1:                          ; GsuF441Player1 joins here
    jsl.l GsuBytes200_0F050E
    lda.w #$080E
    sta.l !gsu_in_216
    bra GsuF441_inputs
GsuF441_other:                          ; GsuF441Player1 joins here
    jsl.l GsuBytes200_0F080E
    lda.w #$0A0E
    sta.l !gsu_in_216
GsuF441_inputs:
    lda.w #$0074
    sta.l !gsu_bytes_200+10
    lda.w #$003C
    sta.l !gsu_bytes_200+12
    sep.b #$20
    rep.b #$10
    ldx.w #$F441
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    plp
    rts

; @routine 03:AF05 ViewReset
; WHAT IT DOES: Puts the view at the origin facing straight ahead: clears
; the position at $0664-$0669 and copies it to the Super FX inputs at
; $065E; runs program $01:8325 (Gsu8325) and keeps its nine words at
; $064C-$065D; sets the nine words at $063A to the identity matrix ($7F00
; on the diagonal).
; HOW: Work RAM through the data bank; the matrix is written a byte at a
; time (high bytes of the diagonal, then the low bytes).
; Entry: any widths. Exit: A = $7F with the high byte of the last result
; word; X = $25; Y high byte clear; P unchanged.
%at($03AF05)
ViewReset:
    php
    rep.b #$20
    stz.w !wram_0664
    stz.w !wram_0666
    stz.w !wram_0668
    lda.w !wram_0664
    sta.w !wram_065E
    lda.w !wram_0666
    sta.w !wram_065E+2
    lda.w !wram_0668
    sta.w !wram_065E+4
    sep.b #$20
    jsl.l Gsu8325
    rep.b #$20
    lda.w !gsu_8325_out
    sta.w !view_8325_out
    lda.w !gsu_8325_out+2
    sta.w !view_8325_out+2
    lda.w !gsu_8325_out+4
    sta.w !view_8325_out+4
    lda.w !gsu_8325_out+6
    sta.w !view_8325_out+6
    lda.w !gsu_8325_out+8
    sta.w !view_8325_out+8
    lda.w !gsu_8325_out+10
    sta.w !view_8325_out+10
    lda.w !gsu_8325_out+12
    sta.w !view_8325_out+12
    lda.w !gsu_8325_out+14
    sta.w !view_8325_out+14
    lda.w !gsu_8325_out+16
    sta.w !view_8325_out+16
    sep.b #$20
    lda.b #$7F
    sta.w !view_matrix+1
    stz.w !view_matrix+3
    stz.w !view_matrix+5
    stz.w !view_matrix+7
    sta.w !view_matrix+9
    stz.w !view_matrix+11
    stz.w !view_matrix+13
    stz.w !view_matrix+15
    sta.w !view_matrix+17
    stz.w !view_matrix
    stz.w !view_matrix+2
    stz.w !view_matrix+4
    stz.w !view_matrix+6
    stz.w !view_matrix+8
    stz.w !view_matrix+10
    stz.w !view_matrix+12
    stz.w !view_matrix+14
    stz.w !view_matrix+16
    plp
    rtl

; The tail ViewDefaults ends in ($03:AE73), with an entry of its own at
; $03:AE70 (PHP, then 8-bit registers) that no traced code calls: seeds the
; random number state ($E6-$E9: $1A6C's byte, then $A7, $55, $7F), clears
; $9C, $BD, $99, $9F-$A1, $9D, $D3, the view position's high bytes, $035D,
; $035E and $05FA, then runs ViewReset, ReadTable9022 and GsuParamsFromDp
; and returns through the PLP.
%at($03AE70)
ViewSetup:
    php                             ; not reached in the traces so far
    sep.b #$30                      ; not reached in the traces so far
ViewSetupTail:
    lda.w !wram_1A6C
    sta.b !random_e6
    lda.b #$A7
    sta.b !random_e7
    lda.b #$55
    sta.b !random_e8
    lda.b #$7F
    sta.b !random_e9
    stz.b !dp_9c
    stz.b !dp_bd
    stz.b !dp_99
    stz.b !dp_9f
    stz.b !dp_a0
    stz.b !dp_a1
    stz.b !dp_9d
    stz.b !dp_d3
    stz.w !view_position+1
    stz.w !view_position+3
    stz.w !view_position+5
    stz.w !wram_035D
    stz.w !wram_035E
    lda.b #$00
    sta.w !wram_05FA
    jsl.l ViewReset
    jsl.l ReadTable9022
    jsl.l GsuParamsFromDp
    plp
    rtl

; @routine 03:B67B ViewDefaults
; WHAT IT DOES: Sets the default view: Super FX words $70:01D0 = $1234,
; $70:01C4 = 1, $70:108C = 0; $A0 = $80, $05EF = $FF, $00 = 0; the
; direct-page view parameters $2A = 0, $2E = $67, $2C = 0, $30 = $58,
; $CE = $34, $D0 = $2C, $34 = $5D17, $32 = $9D89; then the setup tail at
; $03:AE73 (random seed, cleared state, ViewReset, ReadTable9022,
; GsuParamsFromDp).
; HOW: Byte stores in the original's order ($A0 is written here and cleared
; again in the tail), then a long jump into the tail, which returns through
; this routine's PLP.
; Entry: D = $0000; any widths. Exit: A = $5D17; X = $80; Y = 0; P
; unchanged.
%at($03B67B)
ViewDefaults:
    php
    rep.b #$20
    lda.w #$1234
    sta.l !gsu_word_1d0
    lda.w #$0001
    sta.l !gsu_word_1c4
    lda.w #$0000
    sta.l !gsu_word_108c
    sep.b #$20
    lda.b #$80
    sta.b !dp_a0
    lda.b #$FF
    sta.w !wram_05EF
    stz.b !dp_00
    sep.b #$30
    lda.b #$00
    sta.b !dp_2a
    lda.b #$00
    sta.b !dp_2a+1
    lda.b #$67
    sta.b !dp_2e
    lda.b #$00
    sta.b !dp_2e+1
    lda.b #$00
    sta.b !dp_2c
    lda.b #$00
    sta.b !dp_2c+1
    lda.b #$58
    sta.b !dp_30
    lda.b #$00
    sta.b !dp_30+1
    lda.b #$34
    sta.b !dp_ce
    lda.b #$00
    sta.b !dp_ce+1
    lda.b #$2C
    sta.b !dp_d0
    lda.b #$00
    sta.b !dp_d0+1
    lda.b #$17
    sta.b !dp_34
    lda.b #$5D
    sta.b !dp_34+1
    lda.b #$89
    sta.b !dp_32
    lda.b #$9D
    sta.b !dp_32+1
    jml.l ViewSetupTail

; @routine 07:85E1 GsuF0A5OrF0B4
; WHAT IT DOES: Sets the renderer input at $70:020C to A, then runs Super FX
; program $07:F0A5 with the bytes $0F, $09, $0A at $70:0200 when $0D62 is
; nonzero, else program $07:F0B4 with $BF, $BC, $BD.
; HOW: GsuBytes200_0F090A or GsuBytes200_BFBCBD, then GsuRun.
; Entry: M 16-bit, X 8-bit, D = $0000; A = the input. Exit: A = the screen
; mode value (high byte clear); X = the program address's low byte; Y high
; byte clear; M 16-bit, X 8-bit; N, Z from the data bank.
%at($0785E1)
GsuF0A5OrF0B4:
    sta.l !gsu_bytes_200+12
    lda.w !wram_0D62
    and.w #$00FF
    beq .f0b4
    jsl.l GsuBytes200_0F090A
    sep.b #$20
    rep.b #$10
    ldx.w #$F0A5
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    sep.b #$10
    rts
.f0b4:
    jsl.l GsuBytes200_BFBCBD
    sep.b #$20
    rep.b #$10
    ldx.w #$F0B4
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    sep.b #$10
    rts

; @routine 07:8619 GsuEEF5OrEFCD
; WHAT IT DOES: Sets the renderer input at $70:020C to A, then runs Super FX
; program $07:EEF5 with the bytes $0F, $04, $05 at $70:0200 when $0D62 is
; nonzero, else program $07:EFCD with $BF, $B7, $B6.
; HOW: GsuBytes200_0F0405 or GsuBytes200_BFB7B6, then GsuRun.
; Entry: M 16-bit, X 8-bit, D = $0000; A = the input. Exit: A = the screen
; mode value (high byte clear); X = the program address; M 16-bit, X
; 16-bit; N, Z from the data bank.
%at($078619)
GsuEEF5OrEFCD:
    sta.l !gsu_bytes_200+12
    rep.b #$30
    lda.w !wram_0D62
    and.w #$00FF
    beq .efcd
    jsl.l GsuBytes200_0F0405
    sep.b #$20
    rep.b #$10
    ldx.w #$EEF5
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rts
.efcd:
    jsl.l GsuBytes200_BFB7B6
    sep.b #$20
    rep.b #$10
    ldx.w #$EFCD
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rts

; @routine 07:9252 GsuDigitPair0E74
; WHAT IT DOES: Draws the two digits of the byte at $0E74 (high digit, then
; low) with Super FX program $07:F15B: $70:020E = $10, $70:020A = 4,
; $70:020C = 8, and $70:0216 = the digit's entry in the table at $8E3E.
; HOW: GsuParam216A and GsuRun for each digit.
; Entry: M 16-bit, X 16-bit, D = $0000. Exit: A = the low digit's table
; word's high byte and the screen mode value; X = $F15B; C clear; N, Z
; from the data bank.
%at($079252)
GsuDigitPair0E74:
    lda.w #$0010
    sta.l !gsu_bytes_200+14
    lda.w #$0004
    sta.l !gsu_bytes_200+10
    lda.w #$0008
    lda.w #$0008
    sta.l !gsu_bytes_200+12
    lda.w !wram_0E74
    lsr a
    lsr a
    lsr a
    lsr a
    and.w #$000F
    jsr.w GsuParam216A
    sep.b #$20
    rep.b #$10
    ldx.w #$F15B
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rep.b #$30
    lda.w !wram_0E74
    and.w #$000F
    jsr.w GsuParam216A
    sep.b #$20
    rep.b #$10
    ldx.w #$F15B
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rts

; @routine 07:976B GsuFA1BFromTables
; WHAT IT DOES: Sets the renderer inputs $70:020A = $0D6D, $70:020C = $0D6F
; + 8, $70:0210 = $70:0214 = $100 and the bytes $BF, $BC, $BD at $70:0200;
; then, when entry $0D6B of the word table at $07:97D6 is nonzero, runs
; Super FX program $07:FA1B with $70:020E = that word, $70:0212 = the same
; entry of the table at $07:97E2 and $70:0216 = $1E00.
; HOW: GsuBytes200_BFBCBD, word copies, GsuRun.
; Entry: M 16-bit, X 16-bit, D = $0000. Exit: when the entry is 0, A = 0, X
; = $0D6B, Z set and N clear; otherwise A = $1E and the screen mode value,
; X = $FA1B, N and Z from the data bank. C and V from the add of 8.
%at($07976B)
GsuFA1BFromTables:
    jsl.l GsuBytes200_BFBCBD
    lda.w !wram_0D6D
    sta.l !gsu_bytes_200+10
    lda.w !wram_0D6F
    clc
    adc.w #$0008
    sta.l !gsu_bytes_200+12
    lda.w #$0100
    sta.l !gsu_bytes_200+16
    lda.w #$0100
    sta.l !gsu_bytes_200+20
    ldx.w !wram_0D6B
    lda.l !fa1b_input_e,x
    bne .run
    rts
.run:
    sta.l !gsu_bytes_200+14
    lda.l !fa1b_input_12,x
    sta.l !gsu_bytes_200+18
    lda.w #$1E00
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$FA1B
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rts

; @routine 07:9883 GsuF15BThree
; WHAT IT DOES: Runs Super FX program $07:F15B three times, with $70:0216
; = $9000, $8808 and $881C, after setting $70:0210 = $20, $70:020E = $10,
; $70:020A = $30, $70:020C = $18 and the bytes $0F, $04, $05 at $70:0200.
; HOW: GsuBytes200_0F0405, then GsuRun for each value.
; Entry: any widths, D = $0000. Exit: A = $88 and the screen mode value; X
; = $F15B; M 16-bit, X 16-bit; N, Z from the data bank.
%at($079883)
GsuF15BThree:
    rep.b #$30
    lda.w #$0020
    sta.l !gsu_bytes_200+16
    lda.w #$0010
    sta.l !gsu_bytes_200+14
    lda.w #$0030
    sta.l !gsu_bytes_200+10
    lda.w #$0018
    sta.l !gsu_bytes_200+12
    jsl.l GsuBytes200_0F0405
    rep.b #$20
    lda.w #$9000
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F15B
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rep.b #$20
    lda.w #$8808
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F15B
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rep.b #$20
    lda.w #$881C
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F15B
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rts

; @routine 02:E01C Gsu81E3
; WHAT IT DOES: Runs Super FX program $01:81E3 with direct page $02 at
; $70:0068 and direct page $08 at $70:002C, and returns the word it leaves
; at $70:0040.
; HOW: Two long word stores, GsuRun, one long word load; X and P are saved
; around it.
; Entry: any widths, D = $0000. Exit: A = $70:0040; X and P unchanged.
%at($02E01C)
Gsu81E3:
    phx
    php
    rep.b #$20
    lda.b !dp_02
    sta.l !gsu_in_68
    lda.b !dp_08
    sta.l !gsu_dest
    sep.b #$20
    rep.b #$10
    lda.b #$01
    ldx.w #$81E3
    jsl.l GsuRun
    rep.b #$20
    lda.l !gsu_rec_40
    plp
    plx
    rts

; @routine 02:DF24 ViewMatrixUpdateAndSave
; WHAT IT DOES: Saves the view position ($0664-$0669 to $065E-$0663), has
; the Super FX recompute the nine-word view matrix at $0608 (Gsu8325) and
; saves a copy of it at $064C-$065D.
; HOW: Word copies through the data bank around Gsu8325.
; Entry: D = $0000. Exit: A = the last matrix word (16-bit); X = $25, Y
; high byte clear; M 8-bit, X 8-bit; N, Z from A as a 16-bit value.
%at($02DF24)
ViewMatrixUpdateAndSave:
    rep.b #$20
    lda.w !wram_0664
    sta.w !wram_065E
    lda.w !wram_0666
    sta.w !wram_065E+2
    lda.w !wram_0668
    sta.w !wram_065E+4
    sep.b #$30
    jsl.l Gsu8325
    rep.b #$20
    lda.w !gsu_8325_out
    sta.w !view_matrix_saved
    lda.w !gsu_8325_out+2
    sta.w !view_matrix_saved+2
    lda.w !gsu_8325_out+4
    sta.w !view_matrix_saved+4
    lda.w !gsu_8325_out+6
    sta.w !view_matrix_saved+6
    lda.w !gsu_8325_out+8
    sta.w !view_matrix_saved+8
    lda.w !gsu_8325_out+10
    sta.w !view_matrix_saved+10
    lda.w !gsu_8325_out+12
    sta.w !view_matrix_saved+12
    lda.w !gsu_8325_out+14
    sta.w !view_matrix_saved+14
    lda.w !gsu_8325_out+16
    sta.w !view_matrix_saved+16
    sep.b #$20
    rtl

; @routine 02:E016 Gsu81E3Long
; WHAT IT DOES: Gsu81E3 for callers in other banks.
; HOW: A near call to Gsu81E3, then 16-bit A and a long return.
; Entry: as Gsu81E3. Exit: as Gsu81E3, with M 16-bit.
%at($02E016)
Gsu81E3Long:
    jsr.w Gsu81E3
    rep.b #$20
    rtl

; @routine 02:DB59 ViewUpdateAndGsuAEF8
; WHAT IT DOES: Updates the view matrix (ViewMatrixUpdateAndSave), then
; runs Super FX program $01:AEF8.
; HOW: Two long calls; GsuRun with A = 1, X = $AEF8.
; Entry: D = $0000. Exit: A low byte = the screen mode value; X = $AEF8;
; Y high byte clear; M 8-bit, X 16-bit; N, Z from the data bank.
%at($02DB59)
ViewUpdateAndGsuAEF8:
    jsl.l ViewMatrixUpdateAndSave
    sep.b #$20
    rep.b #$10
    sep.b #$20
    lda.b #$01
    ldx.w #$AEF8
    jsl.l GsuRun
    sep.b #$20
    rep.b #$10
    rtl

; @routine 07:8AA6 GsuF329Sequence
; WHAT IT DOES: Runs Super FX program $07:F329 seven times: for each of the
; five bytes at Y (through the data bank) via GsuF329Entry, with two runs
; between them that set $70:0216 = $980A (after the first byte) and $980C
; (after the third).
; HOW: GsuF329Entry and GsuRun.
; Entry: M 16-bit, X 16-bit, D = $0000; Y = the bytes. Exit: as
; GsuF329Entry for the fifth byte; Y unchanged.
%at($078AA6)
GsuF329Sequence:
    lda.w 0,y
    jsr.w GsuF329Entry
    lda.w #$980A
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F329
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    lda.w 1,y
    jsr.w GsuF329Entry
    lda.w 2,y
    jsr.w GsuF329Entry
    lda.w #$980C
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F329
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    lda.w 3,y
    jsr.w GsuF329Entry
    lda.w 4,y
    jsr.w GsuF329Entry
    rts

; @routine 07:997F GsuF15BFive
; WHAT IT DOES: Runs Super FX program $07:F15B five times, with $70:0216
; = $8410, $8C0C, $8408, $8C10 and $8804, after setting $70:0210 = $20,
; $70:020E = $10, $70:020A = $18, $70:020C = $18 and the bytes $0F, $09,
; $0A at $70:0200.
; HOW: GsuBytes200_0F090A, then GsuRun for each value.
; Entry: any widths, D = $0000. Exit: A = $88 and the screen mode value; X
; = $F15B; M 16-bit, X 16-bit; N, Z from the data bank.
%at($07997F)
GsuF15BFive:
    rep.b #$30
    lda.w #$0020
    sta.l !gsu_bytes_200+16
    lda.w #$0010
    sta.l !gsu_bytes_200+14
    lda.w #$0018
    sta.l !gsu_bytes_200+10
    lda.w #$0018
    sta.l !gsu_bytes_200+12
    jsl.l GsuBytes200_0F090A
    rep.b #$20
    lda.w #$8410
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F15B
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rep.b #$20
    lda.w #$8C0C
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F15B
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rep.b #$20
    lda.w #$8408
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F15B
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rep.b #$20
    lda.w #$8C10
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F15B
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rep.b #$20
    lda.w #$8804
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F15B
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rts

; @routine 07:92B5 GsuFA5AThree
; WHAT IT DOES: Runs Super FX program $07:FA5A three times around the point
; ($0D6D, $0D6F): at (X - 8, Y) with $70:0216 = $1A10 and (X - 8, Y + $18)
; with $1B10, both with $70:020E/0210/0212/0214 = $100; then at (X - 8,
; Y + 8) with $1A18, $70:0210 = $200 and $70:0214 = $80. The point goes to
; $70:020A and $70:020C.
; HOW: Word stores and GsuRun.
; Entry: M 16-bit, X 16-bit, D = $0000. Exit: A = $1A and the screen mode
; value; X = $FA5A; N, Z from the data bank; C, V from the add of 8.
%at($0792B5)
GsuFA5AThree:
    lda.w #$0100
    sta.l !gsu_bytes_200+14
    lda.w #$0100
    sta.l !gsu_bytes_200+18
    lda.w #$0100
    sta.l !gsu_bytes_200+16
    lda.w #$0100
    sta.l !gsu_bytes_200+20
    lda.w !wram_0D6D
    clc
    adc.w #$FFF8
    sta.l !gsu_bytes_200+10
    lda.w !wram_0D6F
    sta.l !gsu_bytes_200+12
    lda.w #$1A10
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$FA5A
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    lda.w !wram_0D6D
    clc
    adc.w #$FFF8
    sta.l !gsu_bytes_200+10
    lda.w !wram_0D6F
    clc
    adc.w #$0018
    sta.l !gsu_bytes_200+12
    lda.w #$1B10
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$FA5A
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    lda.w #$0200
    sta.l !gsu_bytes_200+16
    lda.w #$0080
    sta.l !gsu_bytes_200+20
    lda.w !wram_0D6D
    clc
    adc.w #$FFF8
    sta.l !gsu_bytes_200+10
    lda.w !wram_0D6F
    clc
    adc.w #$0008
    sta.l !gsu_bytes_200+12
    lda.w #$1A18
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$FA5A
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rts

; @routine 07:9660 GsuFA5AFromTables
; WHAT IT DOES: Runs Super FX program $07:FA5A three times at the point
; ($0D6D, $0D6F): piece $1B18 at (X, Y) with $70:020E and $70:0212 from
; entry $0D6B of the word tables at $07:97BE and $07:97CA; piece $1D18 at
; (X, Y + $18); piece $1A55 at (X, Y + 8) with $70:0210 = $200 and
; $70:0214 = $80 (the first two with both $100).
; HOW: Word stores and GsuRun.
; Entry: M 16-bit, X 16-bit, D = $0000. Exit: A = the screen mode value
; (high byte clear); X = $FA5A; N, Z from the data bank; C, V from the add
; of 8.
%at($079660)
GsuFA5AFromTables:
    ldx.w !wram_0D6B
    lda.l !fa5a_input_e,x
    sta.l !gsu_bytes_200+14
    lda.l !fa5a_input_12,x
    sta.l !gsu_bytes_200+18
    lda.w #$1B18
    sta.l !gsu_in_216
    lda.w !wram_0D6D
    sta.l !gsu_bytes_200+10
    lda.w !wram_0D6F
    sta.l !gsu_bytes_200+12
    lda.w #$0100
    sta.l !gsu_bytes_200+16
    lda.w #$0100
    sta.l !gsu_bytes_200+20
    sep.b #$20
    rep.b #$10
    ldx.w #$FA5A
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    lda.w !wram_0D6D
    sta.l !gsu_bytes_200+10
    lda.w !wram_0D6F
    clc
    adc.w #$0018
    sta.l !gsu_bytes_200+12
    lda.w #$0100
    sta.l !gsu_bytes_200+16
    lda.w #$0100
    sta.l !gsu_bytes_200+20
    lda.w #$1D18
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$FA5A
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    lda.w !wram_0D6F
    clc
    adc.w #$0008
    sta.l !gsu_bytes_200+12
    lda.w !wram_0D6D
    sta.l !gsu_bytes_200+10
    lda.w #$1A55
    sta.l !gsu_in_216
    lda.w #$0200
    sta.l !gsu_bytes_200+16
    lda.w #$0080
    sta.l !gsu_bytes_200+20
    sep.b #$20
    rep.b #$10
    ldx.w #$FA5A
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rts

; @routine 07:9366 GsuFA5ASix
; WHAT IT DOES: Runs Super FX program $07:FA5A six times, all with
; $70:020E/0210/0212/0214 = $100, from X = $0D6D + entry $0D6B of the word
; table at $07:9806 (kept in direct page $F1) and Y = $0D6F: pieces $1A0E,
; $1A16, $1B16, $1B0E at Y, Y + 8, Y + $10, Y + $18; then at X + 8, pieces
; $1A0E and $1B0E at Y + 8 and Y + $10.
; HOW: Word stores and GsuRun; $F1 ends as X + 8.
; Entry: M 16-bit, X 16-bit, D = $0000. Exit: A = $1B and the screen mode
; value; X = $FA5A; N, Z from the data bank; C, V from the add of $10.
%at($079366)
GsuFA5ASix:
    lda.w #$0100
    sta.l !gsu_bytes_200+14
    lda.w #$0100
    sta.l !gsu_bytes_200+18
    lda.w #$0100
    sta.l !gsu_bytes_200+16
    lda.w #$0100
    sta.l !gsu_bytes_200+20
    ldx.w !wram_0D6B
    lda.l !fa5a_six_dx,x
    clc
    adc.w !wram_0D6D
    sta.b !piece_x
    sta.l !gsu_bytes_200+10
    lda.w #$1A0E
    sta.l !gsu_in_216
    lda.w !wram_0D6F
    sta.l !gsu_bytes_200+12
    sep.b #$20
    rep.b #$10
    ldx.w #$FA5A
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    lda.b !piece_x
    sta.l !gsu_bytes_200+10
    lda.w !wram_0D6F
    clc
    adc.w #$0008
    sta.l !gsu_bytes_200+12
    lda.w #$1A16
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$FA5A
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    lda.b !piece_x
    sta.l !gsu_bytes_200+10
    lda.w !wram_0D6F
    clc
    adc.w #$0010
    sta.l !gsu_bytes_200+12
    lda.w #$1B16
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$FA5A
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    lda.b !piece_x
    sta.l !gsu_bytes_200+10
    lda.w !wram_0D6F
    clc
    adc.w #$0018
    sta.l !gsu_bytes_200+12
    lda.w #$1B0E
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$FA5A
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    lda.b !piece_x
    clc
    adc.w #$0008
    sta.b !piece_x
    sta.l !gsu_bytes_200+10
    lda.w !wram_0D6F
    clc
    adc.w #$0008
    sta.l !gsu_bytes_200+12
    lda.w #$1A0E
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$FA5A
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    lda.b !piece_x
    sta.l !gsu_bytes_200+10
    lda.w !wram_0D6F
    clc
    adc.w #$0010
    sta.l !gsu_bytes_200+12
    lda.w #$1B0E
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$FA5A
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rts

; @routine 07:8653 GsuF15BTwoLines
; WHAT IT DOES: Draws sixteen pieces with Super FX program $07:F15B (a run
; per piece, $70:0216 = the piece): three from ($50, 8) and 13 from
; (0, $2C), with $70:0210 = A, $70:020E = $10 and the bytes $BF, $B7,
; $B6 at $70:0200. A piece of 0 is a space.
; HOW: GsuBytes200_BFB7B6, then GsuRun per piece; the program steps the
; position itself.
; Entry: M 16-bit, X 8-bit, D = $0000; A = the input for $70:0210. Exit:
; A = $8C and the screen mode value; X = $F15B; M 16-bit, X 16-bit; N, Z
; from the data bank.
%at($078653)
GsuF15BTwoLines:
    sta.l !gsu_bytes_200+16
    lda.w #$0010
    sta.l !gsu_bytes_200+14
    lda.w #$0050
    sta.l !gsu_bytes_200+10
    lda.w #$0008
    sta.l !gsu_bytes_200+12
    jsl.l GsuBytes200_BFB7B6
    rep.b #$20
    lda.w #$8800
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F15B
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rep.b #$20
    lda.w #$8418
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F15B
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rep.b #$20
    lda.w #$8C14
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F15B
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rep.b #$20
    lda.w #$0000
    sta.l !gsu_bytes_200+10
    lda.w #$002C
    sta.l !gsu_bytes_200+12
    rep.b #$20
    lda.w #$8408
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F15B
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rep.b #$20
    lda.w #$8814
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F15B
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rep.b #$20
    lda.w #$8814
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F15B
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rep.b #$20
    lda.w #$0000
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F15B
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rep.b #$20
    lda.w #$8C14
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F15B
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rep.b #$20
    lda.w #$8804
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F15B
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rep.b #$20
    lda.w #$8418
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F15B
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rep.b #$20
    lda.w #$0000
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F15B
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rep.b #$20
    lda.w #$8C10
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F15B
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rep.b #$20
    lda.w #$8C14
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F15B
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rep.b #$20
    lda.w #$8408
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F15B
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rep.b #$20
    lda.w #$8C0C
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F15B
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rep.b #$20
    lda.w #$8C10
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F15B
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rts

; @routine 07:833B GsuPiecesStep10CB
; WHAT IT DOES: Steps the counter at $10CB (up to $14) and draws eleven
; pieces from ($10, $28): $800C, $8814, $8408, $8C04, $8C10, space, $8C14,
; $8C00, space, $8800, $8C00, with $70:0210 = the counter's byte in the
; table at $85CC (through the data bank), $70:020E = $10 and the bytes
; $BF, $BA, $B9 at $70:0200. While the counter is below 8 the pieces go
; through Super FX program $07:F15B, after that through $07:F121.
; HOW: GsuBytes200_BFBAB9, then GsuRun per piece; after the first piece
; the X position the program left at $70:020A is moved on by 4.
; Entry: M 16-bit, X 16-bit, D = $0000. Exit: A = $8C and the screen mode
; value; X = the program address; N, Z from the data bank; C, V from the
; add of 4.
%at($07833B)
GsuPiecesStep10CB:
    lda.w !piece_counter
    inc a
    cmp.w #$0014
    bcc .capped
    lda.w #$0014
.capped:
    sta.w !piece_counter
    tax
    lda.w #$0010
    sta.l !gsu_bytes_200+14
    lda.w #$0010
    sta.l !gsu_bytes_200+10
    lda.w #$0028
    sta.l !gsu_bytes_200+12
    jsl.l GsuBytes200_BFBAB9
    lda.w !piece_scale_w,x
    and.w #$00FF
    sta.l !gsu_bytes_200+16
    cpx.w #$0008
    bcc .f15b
    jmp.w .f121
.f15b:
    rep.b #$20
    lda.w #$800C
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F15B
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    lda.l !gsu_bytes_200+10
    clc
    adc.w #$0004
    sta.l !gsu_bytes_200+10
    rep.b #$20
    lda.w #$8814
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F15B
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rep.b #$20
    lda.w #$8408
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F15B
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rep.b #$20
    lda.w #$8C04
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F15B
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rep.b #$20
    lda.w #$8C10
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F15B
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rep.b #$20
    lda.w #$0000
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F15B
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rep.b #$20
    lda.w #$8C14
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F15B
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rep.b #$20
    lda.w #$8C00
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F15B
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rep.b #$20
    lda.w #$0000
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F15B
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rep.b #$20
    lda.w #$8800
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F15B
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rep.b #$20
    lda.w #$8C00
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F15B
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rts
.f121:
    rep.b #$20
    lda.w #$800C
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F121
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    lda.l !gsu_bytes_200+10
    clc
    adc.w #$0004
    sta.l !gsu_bytes_200+10
    rep.b #$20
    lda.w #$8814
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F121
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rep.b #$20
    lda.w #$8408
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F121
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rep.b #$20
    lda.w #$8C04
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F121
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rep.b #$20
    lda.w #$8C10
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F121
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rep.b #$20
    lda.w #$0000
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F121
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rep.b #$20
    lda.w #$8C14
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F121
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rep.b #$20
    lda.w #$8C00
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F121
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rep.b #$20
    lda.w #$0000
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F121
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rep.b #$20
    lda.w #$8800
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F121
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rep.b #$20
    lda.w #$8C00
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F121
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rts

; @routine 03:C998 ObjectGsu80BCThen81E3
; WHAT IT DOES: For object X, runs program $01:80BC (ObjectGsu80BC) and
; keeps its result in direct page $08, puts $000A (data bank) - the
; object's +$0E in $02, then runs Gsu81E3Long.
; HOW: 16-bit subtract; X and Y are saved and restored.
; Entry: X 16-bit, D = $0000; X = object. Exit: A as Gsu81E3Long leaves it;
; X, Y unchanged; M 16-bit; N, Z from X.
%at($03C998)
ObjectGsu80BCThen81E3:
    phx
    phy
    rep.b #$20
    jsl.l ObjectGsu80BC
    sta.b !dp_08
    lda.w !wram_000A
    sec
    sbc.w !obj_pos_y,x
    sta.b !dp_02
    jsl.l Gsu81E3Long
    rep.b #$20
    ply
    plx
    rtl

; @routine 03:C9B4 ObjectGsu81E3
; WHAT IT DOES: For object X, puts direct page $04 - its +$0C in $02 and
; $05F2 - its +$10 in $08, then runs Gsu81E3Long.
; HOW: 16-bit subtracts through the data bank; X and Y saved and restored.
; Entry: X 16-bit, D = $0000; X = object. Exit: as ObjectGsu80BCThen81E3.
%at($03C9B4)
ObjectGsu81E3:
    phx
    phy
    rep.b #$20
    lda.b !dp_04
    sec
    sbc.w !obj_pos_x,x
    sta.b !dp_02
    lda.w !rot_z
    sec
    sbc.w !obj_pos_z,x
    sta.b !dp_08
    jsl.l Gsu81E3Long
    rep.b #$20
    ply
    plx
    rtl

; @routine 07:9486 GsuFA5AThreeShifted
; WHAT IT DOES: As GsuFA5AThree, from X = $0D6D + entry $0D6B of the word
; table at $07:9806 (kept in direct page $F1) instead of $0D6D - 8, with
; pieces $1A12, $1B12 and $1C18.
; HOW: Word stores and GsuRun. Not reached in the traces so far.
; Entry: M 16-bit, X 16-bit, D = $0000. Exit: A = $1C and the screen mode
; value; X = $FA5A; N, Z from the data bank; C, V from the add of 8.
%at($079486)
GsuFA5AThreeShifted:
    lda.w #$0100
    sta.l !gsu_bytes_200+14
    lda.w #$0100
    sta.l !gsu_bytes_200+18
    lda.w #$0100
    sta.l !gsu_bytes_200+16
    lda.w #$0100
    sta.l !gsu_bytes_200+20
    ldx.w !wram_0D6B
    lda.l !fa5a_six_dx,x
    clc
    adc.w !wram_0D6D
    sta.b !piece_x
    sta.l !gsu_bytes_200+10
    lda.w !wram_0D6F
    sta.l !gsu_bytes_200+12
    lda.w #$1A12
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$FA5A
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    lda.b !piece_x
    sta.l !gsu_bytes_200+10
    lda.w !wram_0D6F
    clc
    adc.w #$0018
    sta.l !gsu_bytes_200+12
    lda.w #$1B12
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$FA5A
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    lda.w #$0200
    sta.l !gsu_bytes_200+16
    lda.w #$0080
    sta.l !gsu_bytes_200+20
    lda.b !piece_x
    sta.l !gsu_bytes_200+10
    lda.w !wram_0D6F
    clc
    adc.w #$0008
    sta.l !gsu_bytes_200+12
    lda.w #$1C18
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$FA5A
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rts

; @routine 07:9533 GsuFA5ASixLeft
; WHAT IT DOES: Runs Super FX program $07:FA5A six times, all with
; $70:020E/0210/0212/0214 = $100, around the point ($0D6D, $0D6F): at X -
; 8, pieces $1A0C, $1A14, $1B14, $1B0C at Y, Y + 8, Y + $10, Y + $18; then
; at X - $10, pieces $1A0C and $1B0C at Y + 8 and Y + $10. The mirror of
; GsuFA5ASix.
; HOW: Word stores and GsuRun; each run reads the point again. Not reached
; in the traces so far.
; Entry: M 16-bit, X 16-bit, D = $0000. Exit: A = $1B and the screen mode
; value; X = $FA5A; N, Z from the data bank; C, V from the add of -$10.
%at($079533)
GsuFA5ASixLeft:
    lda.w #$0100
    sta.l !gsu_bytes_200+14
    lda.w #$0100
    sta.l !gsu_bytes_200+18
    lda.w #$0100
    sta.l !gsu_bytes_200+16
    lda.w #$0100
    sta.l !gsu_bytes_200+20
    lda.w !wram_0D6F
    sta.l !gsu_bytes_200+12
    lda.w !wram_0D6D
    clc
    adc.w #$FFF8
    sta.l !gsu_bytes_200+10
    lda.w #$1A0C
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$FA5A
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    lda.w !wram_0D6F
    clc
    adc.w #$0008
    sta.l !gsu_bytes_200+12
    lda.w !wram_0D6D
    clc
    adc.w #$FFF8
    sta.l !gsu_bytes_200+10
    lda.w #$1A14
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$FA5A
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    lda.w !wram_0D6F
    clc
    adc.w #$0010
    sta.l !gsu_bytes_200+12
    lda.w !wram_0D6D
    clc
    adc.w #$FFF8
    sta.l !gsu_bytes_200+10
    lda.w #$1B14
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$FA5A
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    lda.w !wram_0D6F
    clc
    adc.w #$0018
    sta.l !gsu_bytes_200+12
    lda.w !wram_0D6D
    clc
    adc.w #$FFF8
    sta.l !gsu_bytes_200+10
    lda.w #$1B0C
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$FA5A
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    lda.w !wram_0D6F
    clc
    adc.w #$0008
    sta.l !gsu_bytes_200+12
    lda.w !wram_0D6D
    clc
    adc.w #$FFF0
    sta.l !gsu_bytes_200+10
    lda.w #$1A0C
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$FA5A
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    lda.w !wram_0D6F
    clc
    adc.w #$0010
    sta.l !gsu_bytes_200+12
    lda.w !wram_0D6D
    clc
    adc.w #$FFF0
    sta.l !gsu_bytes_200+10
    lda.w #$1B0C
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$FA5A
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rts

; @routine 07:9718 GsuFA35FromTables
; WHAT IT DOES: Sets the renderer inputs $70:020A = $0D6D, $70:020C = $0D6F
; + 8, $70:0210 = $70:0214 = $100 and the bytes $BF, $BC, $BD at $70:0200;
; then, when entry $0D6B of the word table at $07:97EE is nonzero, runs
; Super FX program $07:FA35 with $70:020E = that word, $70:0212 = the same
; entry of the table at $07:97FA and $70:0216 = $1C00. The counterpart of
; GsuFA1BFromTables.
; HOW: GsuBytes200_BFBCBD, word copies, GsuRun. Not reached in the traces
; so far.
; Entry: M 16-bit, X 16-bit, D = $0000. Exit: when the entry is 0, A = 0, X
; = $0D6B, Z set and N clear; otherwise A = $1C and the screen mode value,
; X = $FA35, N and Z from the data bank. C and V from the add of 8.
%at($079718)
GsuFA35FromTables:
    jsl.l GsuBytes200_BFBCBD
    lda.w !wram_0D6D
    sta.l !gsu_bytes_200+10
    lda.w !wram_0D6F
    clc
    adc.w #$0008
    sta.l !gsu_bytes_200+12
    lda.w #$0100
    sta.l !gsu_bytes_200+16
    lda.w #$0100
    sta.l !gsu_bytes_200+20
    ldx.w !wram_0D6B
    lda.l !fa35_input_e,x
    bne .run
    rts
.run:
    sta.l !gsu_bytes_200+14
    lda.l !fa35_input_12,x
    sta.l !gsu_bytes_200+18
    lda.w #$1C00
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$FA35
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rts

; @routine 07:9AB0 HudSixGlyphs
; WHAT IT DOES: Draws six HUD glyphs with Super FX program $07:F15B: the
; input bytes 0, $0F, $06, $0E (GsuBytes200_0F060E), $20 rows, an advance
; of $10, starting at X $10, Y $18; the glyph words $8C10, $8C04, $8814,
; $8408, $8C10 and $8804 in turn at $70:0216.
; HOW: Long word stores to Super FX RAM, then GsuRun per glyph.
; Entry: D = $0000. Exit: as the last GsuRun leaves it, with M and X 16-bit.
%at($079AB0)
HudSixGlyphs:
    rep.b #$30
    jsl.l GsuBytes200_0F060E
    lda.w #$0020
    sta.l !fx_draw_rows
    lda.w #$0010
    sta.l !fx_draw_advance
    lda.w #$0010
    sta.l !fx_draw_x
    lda.w #$0018
    sta.l !fx_draw_y
    rep.b #$20
    lda.w #$8C10
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F15B
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rep.b #$20
    lda.w #$8C04
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F15B
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rep.b #$20
    lda.w #$8814
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F15B
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rep.b #$20
    lda.w #$8408
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F15B
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rep.b #$20
    lda.w #$8C10
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F15B
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rep.b #$20
    lda.w #$8804
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F15B
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rts

; @routine 07:9D67 BigSpritesRow6
; WHAT IT DOES: Unless $0E5C is set, draws six 24 x 40 sprites (Super FX
; program FxSprite24x40) from (24, 40): sources $B000, $A506, $A518 and
; $A00C, then 16 further on $B006 and $AA0C. With $0E5C set it goes to
; 07:9EE9 instead.
; HOW: GsuBytes200_BFB5BB first; GsuRun for each sprite; the x the program
; leaves is read back for the gap. The jump is not reached in the traces so
; far.
; Entry: any widths, D = $0000. Exit: M, X 16-bit; X = $FA88; A = the last
; source's high byte and the screen mode value; N, Z from the data bank; C,
; V from adding the gap.
%at($079D67)
BigSpritesRow6:
    rep.b #$20
    jsl.l GsuBytes200_BFB5BB
    sep.b #$20
    lda.w !wram_0E5C
    beq .draw
    brl BigSpritesCountDraw
.draw:
    rep.b #$30
    lda.w #$0018
    sta.l !fx_draw_x
    lda.w #$0028
    sta.l !fx_draw_y
    lda.w #$0100
    sta.l !fx_draw_step_x
    sta.l !fx_draw_step_y
    rep.b #$20
    lda.w #$B000
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #FxSprite24x40
    lda.b #FxSprite24x40>>16
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rep.b #$20
    lda.w #$A506
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #FxSprite24x40
    lda.b #FxSprite24x40>>16
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rep.b #$20
    lda.w #$A518
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #FxSprite24x40
    lda.b #FxSprite24x40>>16
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rep.b #$20
    lda.w #$A00C
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #FxSprite24x40
    lda.b #FxSprite24x40>>16
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    lda.l !fx_draw_x
    clc
    adc.w #$0010
    sta.l !fx_draw_x
    rep.b #$20
    lda.w #$B006
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #FxSprite24x40
    lda.b #FxSprite24x40>>16
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rep.b #$20
    lda.w #$AA0C
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #FxSprite24x40
    lda.b #FxSprite24x40>>16
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rts

; @routine 07:9E3B BigSpritesRow5
; WHAT IT DOES: Unless $0E5C is set, draws five 24 x 40 sprites
; (FxSprite24x40) from (32, 40): sources $A006, $AA12, $A000, $AA18 and
; $A500. With $0E5C set it goes to 07:9EE9 instead.
; HOW: GsuBytes200_BFBCBD first; GsuRun for each sprite. The jump is not
; reached in the traces so far.
; Entry: any widths, D = $0000. Exit: as BigSpritesRow6, C and V as the
; entry left them.
%at($079E3B)
BigSpritesRow5:
    rep.b #$20
    jsl.l GsuBytes200_BFBCBD
    sep.b #$20
    lda.w !wram_0E5C
    beq .draw
    brl BigSpritesCountDraw
.draw:
    rep.b #$30
    lda.w #$0020
    sta.l !fx_draw_x
    lda.w #$0028
    sta.l !fx_draw_y
    lda.w #$0100
    sta.l !fx_draw_step_x
    sta.l !fx_draw_step_y
    rep.b #$20
    lda.w #$A006
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #FxSprite24x40
    lda.b #FxSprite24x40>>16
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rep.b #$20
    lda.w #$AA12
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #FxSprite24x40
    lda.b #FxSprite24x40>>16
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rep.b #$20
    lda.w #$A000
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #FxSprite24x40
    lda.b #FxSprite24x40>>16
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rep.b #$20
    lda.w #$AA18
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #FxSprite24x40
    lda.b #FxSprite24x40>>16
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rep.b #$20
    lda.w #$A500
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #FxSprite24x40
    lda.b #FxSprite24x40>>16
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rts

; @routine 07:89E4 HudHoldDraw
; WHAT IT DOES: When the HUD hold flag $0E5D is set, sets the renderer
; bytes 0, $BF, $B3, $BE (GsuBytes200_BFB3BE) and draws the sequence at
; $0F69 at (147, 108) (GsuF329Sequence, through HudTimesSecondRow, the
; tail of HudTimesDraw).
; HOW: P saved and restored; 16-bit.
; Entry: any widths. Exit: P restored; A, X, Y as the draw leaves them
; (A = $0E5D when not set).
%at($0789E4)
HudHoldDraw:
    php
    rep.b #$30
    lda.w !hud_hold
    bne .l89EE
    plp
    rts
.l89EE:
    jsl.l GsuBytes200_BFB3BE
    jmp.w HudTimesSecondRow

; @routine 07:89F5 HudTimesDraw
; WHAT IT DOES: Unless the low byte of $10CD is set, and only while the
; HUD hold flag $0E5D is set: in HUD mode 3 with $0D4C set, the two packed
; times (HudTimesPackedDraw); with no time at $0F2B ($FFFF), the sequence
; at $0F69 alone at (147, 108); with $0E57 set, the lap digits
; (HudLapDigitsDraw) and, when player 1 leads, the word at (60, 72)
; (HudLeadWordDraw); otherwise the sequences at $0F64 and $0F69 at (147,
; 88) and (147, 108), coloured by player 1's lead $0FA3.
; HOW: P saved and restored; 16-bit tests, 8-bit for the lead. The first
; row's colour test is BMI then BPL, so its zero case (07:8A47) is never
; reached; 07:8A90 is the second row's tail HudHoldDraw jumps to.
; Entry: any widths. Exit: P restored; A, X, Y as the last step leaves
; them.
%at($0789F5)
HudTimesDraw:
    php
    rep.b #$30
    lda.w !wram_10CD
    and.w #$00FF
    bne .done
    lda.w !hud_hold
    bne .held
.done:
    plp
    rts
.held:
    lda.w !hud_mode
    cmp.w #$0003
    bne .times
    lda.w !digit62_override
    beq .times
    jsr.w HudTimesPackedDraw
    plp
    rts
.times:
    lda.w !wram_0F2B
    cmp.w #$FFFF
    bne .any
    jsl.l GsuBytes200_BFB3BE
    jmp.w HudTimesSecondRow
.any:
    sep.b #$20
    lda.w !wram_0E57
    beq .rows
    jsr.w HudLapDigitsDraw
    sep.b #$20
    lda.w !p1_lead
    bmi .end
    beq .end
    jsr.w HudLeadWordDraw
.end:
    plp
    rts
.rows:
    lda.w !p1_lead
    bmi HudTimesDraw_behind
    bpl HudTimesDraw_ahead

; @routine 07:8A47 HudTimesDrawEven
; WHAT IT DOES: HudTimesDraw's first row for no lead, which nothing
; reaches (the test before it branches on both signs): the renderer
; bytes 0, $BF, $10, $B9 (GsuBytes200_BF10B9), then both rows as
; HudTimesDraw.
; HOW: As inside HudTimesDraw; ends pulling the P its start pushed.
; Entry: M 8-bit, X 16-bit; P on the stack above the return address.
; Exit: as HudTimesDraw.
%at($078A47)
HudTimesDrawEven:
    rep.b #$30
    jsl.l GsuBytes200_BF10B9
    bra HudTimesDraw_first
HudTimesDraw_ahead:
    rep.b #$30
    jsl.l GsuBytes200_BFB2B3
    bra HudTimesDraw_first
HudTimesDraw_behind:
    rep.b #$30
    jsl.l GsuBytes200_BFB3BE
HudTimesDraw_first:
    lda.w #$0093
    sta.l !fx_draw_x
    lda.w #$0058
    sta.l !fx_draw_y
    ldy.w #!lap_record_a
    jsr.w GsuF329Sequence
    sep.b #$20
    lda.w !p1_lead
    bmi .behind
    bne .ahead
    rep.b #$30
    jsl.l GsuBytes200_BF10B9
    bra HudTimesSecondRow
.behind:
    rep.b #$30
    jsl.l GsuBytes200_BFB2B3
    bra HudTimesSecondRow
.ahead:
    rep.b #$30
    jsl.l GsuBytes200_BF11BD
HudTimesSecondRow:
    lda.w #$0093
    sta.l !fx_draw_x
    lda.w #$006C
    sta.l !fx_draw_y
    ldy.w #!lap_digits
    jsr.w GsuF329Sequence
    plp
    rts

; @routine 07:8AF5 TimeDigitsStore
; WHAT IT DOES: Stores a time unpacked by TimeUnpackBcd as five digits at
; $0F64: the minutes A, then the seconds X and the hundredths Y, each as
; its high and low nibble.
; HOW: 8-bit; four LSRs for each high nibble, AND #$0F for each low.
; Entry: A, X, Y as TimeUnpackBcd leaves them. Exit: M and X 8-bit (X and
; Y high bytes 0); A = Y AND $0F; N, Z from it, C bit 3 of Y.
%at($078AF5)
TimeDigitsStore:
    sep.b #$30
    sta.w !lap_record_a
    txa
    lsr a
    lsr a
    lsr a
    lsr a
    sta.w !lap_record_a+1
    txa
    and.b #$0F
    sta.w !lap_record_a+2
    tya
    lsr a
    lsr a
    lsr a
    lsr a
    sta.w !lap_record_a+3
    tya
    and.b #$0F
    sta.w !lap_record_a+4
    rts

; @routine 07:8B17 HudTimesPackedDraw
; WHAT IT DOES: HudTimesDraw's rows from the packed times: $1163 (unless
; $FFFF, no time) at (147, 88) and $1161 at (147, 108), each unpacked to
; the digits at $0F64 (TimeUnpackBcd, TimeDigitsStore) and drawn from
; there, coloured by player 1's lead $0FA3.
; HOW: 8-bit for the lead, else 16-bit. The first colour test is BMI then
; BPL, so its zero case (07:8B22) is never reached.
; Entry: any widths. Exit: M 16-bit, X 16-bit; A, X, Y as
; GsuF329Sequence leaves them.
%at($078B17)
HudTimesPackedDraw:
    rep.b #$30
    sep.b #$20
    lda.w !p1_lead
    bmi HudTimesPacked_behind
    bpl HudTimesPacked_ahead

; @routine 07:8B22 HudTimesPackedEven
; WHAT IT DOES: HudTimesPackedDraw's first row for no lead, which nothing
; reaches: the renderer bytes 0, $BF, $10, $B9 (GsuBytes200_BF10B9), then
; both rows as HudTimesPackedDraw.
; HOW: As inside HudTimesPackedDraw.
; Entry: M 8-bit, X 16-bit. Exit: as HudTimesPackedDraw.
%at($078B22)
HudTimesPackedEven:
    rep.b #$30
    jsl.l GsuBytes200_BF10B9
    bra HudTimesPacked_first
HudTimesPacked_ahead:
    rep.b #$30
    jsl.l GsuBytes200_BFB2B3
    bra HudTimesPacked_first
HudTimesPacked_behind:
    rep.b #$30
    jsl.l GsuBytes200_BFB3BE
HudTimesPacked_first:
    lda.w #$0093
    sta.l !fx_draw_x
    lda.w #$0058
    sta.l !fx_draw_y
    lda.w !packed_time_a
    cmp.w #$FFFF
    beq .second
    jsr.w TimeUnpackBcd
    jsr.w TimeDigitsStore
    rep.b #$30
    ldy.w #!lap_record_a
    jsr.w GsuF329Sequence
.second:
    sep.b #$20
    lda.w !p1_lead
    bmi .behind
    bne .ahead
    rep.b #$30
    jsl.l GsuBytes200_BF10B9
    bra .row
.behind:
    rep.b #$30
    jsl.l GsuBytes200_BFB2B3
    bra .row
.ahead:
    rep.b #$30
    jsl.l GsuBytes200_BF11BD
.row:
    lda.w #$0093
    sta.l !fx_draw_x
    lda.w #$006C
    sta.l !fx_draw_y
    lda.w !packed_time_b
    jsr.w TimeUnpackBcd
    jsr.w TimeDigitsStore
    rep.b #$30
    ldy.w #!lap_record_a
    jsr.w GsuF329Sequence
    rts


; @routine 07:A7AE HudLeadWordDraw
; WHAT IT DOES: Draws a word of glyphs at (60, 72) with the renderer bytes
; 0, $BF, $11, $BD (GsuBytes200_BF11BD): four glyphs, the position moved 8
; right, then six more.
; HOW: Each glyph by a long store to $70:0216 and Super FX program
; $07:F329.
; Entry: any widths. Exit: M 16-bit, X 16-bit; X = $F329; A as GsuRun
; leaves it.
%at($07A7AE)
HudLeadWordDraw:
    rep.b #$30
    jsl.l GsuBytes200_BF11BD
    lda.w #$003C
    sta.l !fx_draw_x
    lda.w #$0048
    sta.l !fx_draw_y
    rep.b #$20
    lda.w #$9416
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F329
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rep.b #$20
    lda.w #$941C
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F329
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rep.b #$20
    lda.w #$9618
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F329
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rep.b #$20
    lda.w #$961A
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F329
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    lda.l !fx_draw_x
    clc
    adc.w #$0008
    sta.l !fx_draw_x
    rep.b #$20
    lda.w #$9616
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F329
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rep.b #$20
    lda.w #$941C
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F329
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rep.b #$20
    lda.w #$9418
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F329
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rep.b #$20
    lda.w #$9610
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F329
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rep.b #$20
    lda.w #$9616
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F329
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rep.b #$20
    lda.w #$941A
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F329
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rts

; @routine 07:A8D3 HudLapDigitsDraw
; WHAT IT DOES: Draws the five digits at $0F69 at (48, 40) with the
; renderer bytes 0, $BF, $B3, $BE (GsuBytes200_BFB3BE), as M:SS:HH: the
; separators glyphs $9014 and $9018 by program $07:FAB0.
; HOW: Each digit through the table GsuParam216A reads and Super FX
; program $07:F6DF.
; Entry: any widths; data bank $07 (GsuParam216A's table). Exit: M 16-bit,
; X 16-bit; X = $F6DF; A as GsuRun leaves it.
%at($07A8D3)
HudLapDigitsDraw:
    rep.b #$30
    jsl.l GsuBytes200_BFB3BE
    lda.w #$0030
    sta.l !fx_draw_x
    lda.w #$0028
    sta.l !fx_draw_y
    lda.w !lap_digits
    and.w #$00FF
    jsr.w GsuParam216A
    sep.b #$20
    rep.b #$10
    ldx.w #$F6DF
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    lda.w #$9014
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$FAB0
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    lda.w !lap_digits+1
    and.w #$00FF
    jsr.w GsuParam216A
    sep.b #$20
    rep.b #$10
    ldx.w #$F6DF
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    lda.w !lap_digits+2
    and.w #$00FF
    jsr.w GsuParam216A
    sep.b #$20
    rep.b #$10
    ldx.w #$F6DF
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    lda.w #$9018
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$FAB0
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    lda.w !lap_digits+3
    and.w #$00FF
    jsr.w GsuParam216A
    sep.b #$20
    rep.b #$10
    ldx.w #$F6DF
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    lda.w !lap_digits+4
    and.w #$00FF
    jsr.w GsuParam216A
    sep.b #$20
    rep.b #$10
    ldx.w #$F6DF
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rts


; @routine 04:DDB0 GlyphAtY60
; WHAT IT DOES: Draws glyph A at ($70:020A, 96) with the renderer bytes 0,
; $BF, $B5, $BB (GsuBytes200_BFB5BB) and program $07:F329.
; HOW: Long stores to the program's parameters, then GsuRun.
; Entry: M 16-bit, X 16-bit; A = the glyph. Exit: M 16-bit, X 16-bit; X =
; $F329; A as GsuRun leaves it.
%at($04DDB0)
GlyphAtY60:
    sta.l !gsu_in_216
    lda.w #$0060
    sta.l !fx_draw_y
    jsl.l GsuBytes200_BFB5BB
    sep.b #$20
    rep.b #$10
    ldx.w #$F329
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rts

; @routine 04:DD61 GlyphRowY60From0
; WHAT IT DOES: Draws a row of six glyphs at y 96 from x $78 down by 8:
; $9808 three times, then $941A, $960E and $9414 (GlyphAtY60).
; HOW: Six steps of LDA #x, STA.L $70:020A, LDA #glyph, JSR; the entries
; at $04:DD6E, $DD7B, $DD88, $DD95 and $DDA2 start further along.
; Entry: M 16-bit, X 16-bit. Exit: as GlyphAtY60.
%at($04DD61)
GlyphRowY60From0:
    lda.w #$0078
    sta.l !fx_draw_x
    lda.w #$9808
    jsr.w GlyphAtY60

; @routine 04:DD6E GlyphRowY60From1
; WHAT IT DOES: As GlyphRowY60From0 from its second glyph (x $70).
; HOW: The same code, entered further along.
; Entry: M 16-bit, X 16-bit. Exit: as GlyphAtY60.
%at($04DD6E)
GlyphRowY60From1:
    lda.w #$0070
    sta.l !fx_draw_x
    lda.w #$9808
    jsr.w GlyphAtY60

; @routine 04:DD7B GlyphRowY60From2
; WHAT IT DOES: As GlyphRowY60From0 from its third glyph (x $68).
; HOW: The same code, entered further along.
; Entry: M 16-bit, X 16-bit. Exit: as GlyphAtY60.
%at($04DD7B)
GlyphRowY60From2:
    lda.w #$0068
    sta.l !fx_draw_x
    lda.w #$9808
    jsr.w GlyphAtY60

; @routine 04:DD88 GlyphRowY60From3
; WHAT IT DOES: As GlyphRowY60From0 from its fourth glyph (x $60).
; HOW: The same code, entered further along.
; Entry: M 16-bit, X 16-bit. Exit: as GlyphAtY60.
%at($04DD88)
GlyphRowY60From3:
    lda.w #$0060
    sta.l !fx_draw_x
    lda.w #$941A
    jsr.w GlyphAtY60

; @routine 04:DD95 GlyphRowY60From4
; WHAT IT DOES: As GlyphRowY60From0 from its fifth glyph (x $58).
; HOW: The same code, entered further along.
; Entry: M 16-bit, X 16-bit. Exit: as GlyphAtY60.
%at($04DD95)
GlyphRowY60From4:
    lda.w #$0058
    sta.l !fx_draw_x
    lda.w #$960E
    jsr.w GlyphAtY60

; @routine 04:DDA2 GlyphRowY60From5
; WHAT IT DOES: Draws the row's last glyph, $9414 at x $50 (GlyphAtY60).
; HOW: The same code, entered at its last step.
; Entry: M 16-bit, X 16-bit. Exit: as GlyphAtY60.
%at($04DDA2)
GlyphRowY60From5:
    lda.w #$0050
    sta.l !fx_draw_x
    lda.w #$9414
    jsr.w GlyphAtY60
    rts

; @routine 04:DDD1 GlyphListDraw
; WHAT IT DOES: Draws the glyph list: for each of the $10E6 entries (2
; bytes apart) whose word at $110E is set, glyph $10E8 at x and y the bytes
; at $1134 and $1135, the first $10E4 entries with the renderer bytes 0,
; $BF, $BC, $BD and the rest with 0, $BF, $B3, $BE; program $07:F329.
; HOW: The limits doubled into direct page $F1 and $F3; X and P kept on the
; stack around GsuRun.
; Entry: M 16-bit, X 16-bit, D = $0000. Exit: M 16-bit, X 16-bit; X = the
; end; N, Z, C from its compare.
%at($04DDD1)
GlyphListDraw:
    ldx.w #$0000
    lda.w !glyph_list_first
    asl a
    sta.b !digit_count
    lda.w !glyph_list_count
    asl a
    sta.b !dp_f3
.lDDE0:
    rep.b #$20
    lda.w !glyph_list_on,x
    beq .lDE25
    lda.w !glyph_list_glyph,x
    sta.l !gsu_in_216
    lda.w !glyph_list_x,x
    and.w #$00FF
    sta.l !fx_draw_x
    lda.w !glyph_list_y,x
    and.w #$00FF
    sta.l !fx_draw_y
    cpx.b !digit_count
    bcs .lDE0C
    jsl.l GsuBytes200_BFBCBD
    bra .lDE10
.lDE0C:
    jsl.l GsuBytes200_BFB3BE
.lDE10:
    phx
    php
    sep.b #$20
    rep.b #$10
    ldx.w #$F329
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    plp
    plx
.lDE25:
    inx
    inx
    cpx.b !dp_f3
    bcc .lDDE0
    rts


; @routine 07:A426 GlyphSpriteDraw
; WHAT IT DOES: Draws glyph sprite X/2: glyph $1010 + X at ($0FB0 + X,
; $0FE0 + X) with program $07:F329, then steps X by 2.
; HOW: Long stores to the program's parameters; X kept on the stack around
; GsuRun.
; Entry: M 16-bit, X 16-bit; X = the sprite * 2. Exit: X + 2; M 16-bit, X
; 16-bit; N, Z from X; A as GsuRun leaves it.
%at($07A426)
GlyphSpriteDraw:
    lda.w !glyph_spr_x,x
GlyphSpriteDraw_at:
    sta.l !fx_draw_x
    lda.w !glyph_spr_y,x
    sta.l !fx_draw_y
    lda.w !glyph_spr_glyph,x
    sta.l !gsu_in_216
    phx
    sep.b #$20
    rep.b #$10
    ldx.w #$F329
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20

; @routine 04:DA3C GlyphSpritesInit
; WHAT IT DOES: Sets the 16 glyph sprites up: x from the table at
; $04:DA7A, y 8, glyph from the table at $04:DA8E, $1040 + X = 0; then
; $10D8 = 0, $115A = $1E, and $0D5B and the HUD hold $0E5D cleared.
; HOW: A 16-bit loop over X = 0 to $1E by 2; P saved and restored.
; Entry: any widths. Exit: P restored; A = $001E; X = $20; Y unchanged.
%at($04DA3C)
GlyphSpritesInit:
    php
    rep.b #$30
    ldx.w #$0000
.lDA42:
    lda.l !glyph_spr_init_x,x
    sta.w !glyph_spr_x,x
    lda.w #$0008
    sta.w !glyph_spr_y,x
    lda.l !glyph_spr_init_glyph,x
    sta.w !glyph_spr_glyph,x
    stz.w !glyph_spr_1040,x
    inx
    inx
    cpx.w #$0020
    bcc .lDA42
    stz.w !wram_10D8
    lda.w #$001E
    sta.w !wram_115A
    sep.b #$20
    stz.w !wram_0D5B
    stz.w !hud_hold
    plp
    rtl


; @routine 04:DE2C GlyphListMessageAndTime
; WHAT IT DOES: Fills the glyph list with message X/2 (the string its word
; in the table at $04:DF6D points to: a length byte, then a glyph word per
; letter, 0 for a space) centred on x $68 at y $58, then the time row at y
; $68 from x $4C by 8: the digits of $F7 (low), $F6 and $F5 with the
; separator glyphs $980A and $980C; $10E4 = the letters - 1 and $10E6 =
; that + 7.
; HOW: The string through the data bank (bank $04); the letter count in
; direct page $F1 and the x in $F3; spaces take a place but no entry. The
; letters' on-flags are cleared as bytes, the time's as words.
; Entry: M 16-bit, X 16-bit, D = $0000, DB = $04; X = the message * 2.
; Exit: M 8-bit; Y = the last entry; A low byte = $68; N, Z from it.
%at($04DE2C)
GlyphListMessageAndTime:
    lda.w !glyph_messages,x
    tax
    lda.w !glyph_message_length,x
    and.w #$00FF
    sta.b !digit_count
    asl a
    asl a
    sec
    sbc.w #$0068
    eor.w #$FFFF
    inc a
    sta.b !dp_f3
    lda.b !digit_count
    dec a
    sta.w !glyph_list_first
    clc
    adc.w #$0007
    sta.w !glyph_list_count
    ldy.w #$0000
.lDE54:
    lda.w !glyph_message_letters,x
    beq .lDE6F
    sta.w !glyph_list_glyph,y
    sep.b #$20
    lda.b !dp_f3
    sta.w !glyph_list_x,y
    lda.b #$58
    sta.w !glyph_list_y,y
    lda.b #$00
    sta.w !glyph_list_on,y
    iny
    iny
.lDE6F:
    rep.b #$20
    lda.b !dp_f3
    clc
    adc.w #$0008
    sta.b !dp_f3
    inx
    inx
    dec.b !digit_count
    bne .lDE54
    rep.b #$20
    lda.b !glyph_time_low
    and.w #$000F
    asl a
    clc
    adc.w #$9400
    sta.w !glyph_list_glyph,y
    lda.w #$0000
    sta.w !glyph_list_on,y
    sep.b #$20
    lda.b #$4C
    sta.w !glyph_list_x,y
    lda.b #$68
    sta.w !glyph_list_y,y
    iny
    iny
    rep.b #$20
    lda.w #$980A
    sta.w !glyph_list_glyph,y
    lda.w #$0000
    sta.w !glyph_list_on,y
    sep.b #$20
    lda.b #$54
    sta.w !glyph_list_x,y
    lda.b #$68
    sta.w !glyph_list_y,y
    iny
    iny
    rep.b #$20
    lda.b !glyph_time_mid
    and.w #$00FF
    lsr a
    lsr a
    lsr a
    lsr a
    asl a
    clc
    adc.w #$9400
    sta.w !glyph_list_glyph,y
    lda.w #$0000
    sta.w !glyph_list_on,y
    sep.b #$20
    lda.b #$5C
    sta.w !glyph_list_x,y
    lda.b #$68
    sta.w !glyph_list_y,y
    iny
    iny
    rep.b #$20
    lda.b !glyph_time_mid
    and.w #$000F
    asl a
    clc
    adc.w #$9400
    sta.w !glyph_list_glyph,y
    lda.w #$0000
    sta.w !glyph_list_on,y
    sep.b #$20
    lda.b #$64
    sta.w !glyph_list_x,y
    lda.b #$68
    sta.w !glyph_list_y,y
    iny
    iny
    rep.b #$20
    lda.w #$980C
    sta.w !glyph_list_glyph,y
    lda.w #$0000
    sta.w !glyph_list_on,y
    sep.b #$20
    lda.b #$6C
    sta.w !glyph_list_x,y
    lda.b #$68
    sta.w !glyph_list_y,y
    iny
    iny
    rep.b #$20
    lda.b !glyph_time_high
    and.w #$00FF
    lsr a
    lsr a
    lsr a
    lsr a
    asl a
    clc
    adc.w #$9400
    sta.w !glyph_list_glyph,y
    lda.w #$0000
    sta.w !glyph_list_on,y
    sep.b #$20
    lda.b #$74
    sta.w !glyph_list_x,y
    lda.b #$68
    sta.w !glyph_list_y,y
    iny
    iny
    rep.b #$20
    lda.b !glyph_time_high
    and.w #$000F
    asl a
    clc
    adc.w #$9400
    sta.w !glyph_list_glyph,y
    lda.w #$0000
    sta.w !glyph_list_on,y
    sep.b #$20
    lda.b #$7C
    sta.w !glyph_list_x,y
    lda.b #$68
    sta.w !glyph_list_y,y
    rts


; @routine 07:A03F GlyphWordAt1C28
; WHAT IT DOES: Draws a nine-glyph word with program $07:F15B at (28, 40),
; 32 rows, advancing 16, with the renderer bytes 0, $BF, $BC, $BD: glyphs
; $840C, $8C00, $881C, $8C18, $8C10, then 4 further on $8C14, $8C0C, $8408
; and $9004.
; HOW: Long stores to the program's parameters; GsuRun per glyph.
; Entry: any widths. Exit: M 16-bit, X 16-bit; X = $F15B; A as GsuRun
; leaves it.
%at($07A03F)
GlyphWordAt1C28:
    rep.b #$30
    lda.w #$001C
    sta.l !fx_draw_x
    lda.w #$0028
    sta.l !fx_draw_y
    lda.w #$0020
    sta.l !fx_draw_rows
    lda.w #$0010
    sta.l !fx_draw_advance
    jsl.l GsuBytes200_BFBCBD
    rep.b #$20
    lda.w #$840C
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F15B
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rep.b #$20
    lda.w #$8C00
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F15B
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rep.b #$20
    lda.w #$881C
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F15B
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rep.b #$20
    lda.w #$8C18
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F15B
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rep.b #$20
    lda.w #$8C10
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F15B
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    lda.l !fx_draw_x
    clc
    adc.w #$0004
    sta.l !fx_draw_x
    rep.b #$20
    lda.w #$8C14
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F15B
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rep.b #$20
    lda.w #$8C0C
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F15B
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rep.b #$20
    lda.w #$8408
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F15B
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rep.b #$20
    lda.w #$9004
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F15B
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rts

; @routine 07:A158 GlyphGrid1A
; WHAT IT DOES: Draws the 3 x 3 glyph picture $1A1A-$1C1E at (56, 48) with
; program $07:F280 (8 x 8 glyphs): its first two glyphs with the renderer
; bytes 0, $BF, $B7, $B6, the third with 0, $BF, $BA, $B9 and the rest
; with 0, $BF, $B3, $BE.
; HOW: Each row of three, then 8 down and 24 back; the renderer setters'
; long entries ($04:D8C4, $D8E1, $D8FE).
; Entry: any widths. Exit: M 16-bit, X 16-bit; X = $F280; A as GsuRun
; leaves it.
%at($07A158)
GlyphGrid1A:
    rep.b #$20
    lda.w #$0038
    sta.l !fx_draw_x
    lda.w #$0030
    sta.l !fx_draw_y
    jsl.l GsuBytes200_BFB7B6_D8C4
    lda.w #$1A1A
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F280
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    lda.w #$1A1C
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F280
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    jsl.l GsuBytes200_BFBAB9_D8E1
    lda.w #$1A1E
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F280
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    lda.l !fx_draw_y
    clc
    adc.w #$0008
    sta.l !fx_draw_y
    lda.l !fx_draw_x
    sec
    sbc.w #$0018
    sta.l !fx_draw_x
    jsl.l GsuBytes200_BFB3BE_D8FE
    lda.w #$1B1A
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F280
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    lda.w #$1B1C
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F280
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    lda.w #$1B1E
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F280
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    lda.l !fx_draw_y
    clc
    adc.w #$0008
    sta.l !fx_draw_y
    lda.l !fx_draw_x
    sec
    sbc.w #$0018
    sta.l !fx_draw_x
    lda.w #$1C1A
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F280
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    lda.w #$1C1C
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F280
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    lda.w #$1C1E
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F280
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rts

; @routine 07:A27D GlyphGrid1D
; WHAT IT DOES: Draws the 3 x 3 glyph picture $1D1A-$1F1E at (56, 72) with
; program $07:F280, the renderer bytes 0, $BF, $BB, $BE.
; HOW: As GlyphGrid1A, one setter first.
; Entry: any widths. Exit: as GlyphGrid1A.
%at($07A27D)
GlyphGrid1D:
    rep.b #$20
    lda.w #$0038
    sta.l !fx_draw_x
    lda.w #$0048
    sta.l !fx_draw_y
    jsl.l GsuBytes200_BFBBBE
    lda.w #$1D1A
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F280
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    lda.w #$1D1C
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F280
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    lda.w #$1D1E
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F280
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    lda.l !fx_draw_y
    clc
    adc.w #$0008
    sta.l !fx_draw_y
    lda.l !fx_draw_x
    sec
    sbc.w #$0018
    sta.l !fx_draw_x
    lda.w #$1E1A
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F280
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    lda.w #$1E1C
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F280
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    lda.w #$1E1E
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F280
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    lda.l !fx_draw_y
    clc
    adc.w #$0008
    sta.l !fx_draw_y
    lda.l !fx_draw_x
    sec
    sbc.w #$0018
    sta.l !fx_draw_x
    lda.w #$1F1A
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F280
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    lda.w #$1F1C
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F280
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    lda.w #$1F1E
    sta.l !gsu_in_216
    sep.b #$20
    rep.b #$10
    ldx.w #$F280
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rts

; @routine 04:DB87 GlyphTimesSequence
; WHAT IT DOES: One frame of the three best times' display, by its step
; $10D8 (the table at $04:DB94): two waits; then for each of the course
; $0D4A's three times ($0EF9, $0EFC, $0EFF): its message and time as a
; glyph list (GlyphListMessageAndTime), shown a glyph a frame
; (GlyphListDraw with the entries' flags set to $FF one by one), a wait of
; 10, then cleared a glyph a frame, a wait of 4; then the row of six glyphs
; at y 96 drawn from its last glyph back to its first a step at a time
; with waits of 4 and 8; at the end the split window on ($19D8 = 1, $19DF
; cleared) with sound $04 on port 1, and after $14 frames the screen at
; $07:EE26 (with data bank $07).
; HOW: JMP (table,X) on the step; $115A counts frames, $115C the entry.
; Entry: any widths, D = $0000. Exit: M 16-bit, X 16-bit; A, X, Y as the
; step leaves them.
%at($04DB87)
GlyphTimesSequence:
    rep.b #$30
    lda.w !wram_10D8
    and.w #$00FF
    asl a
    tax
    jmp.w (.steps,x)
.steps:
    dw .step_DBC2
    dw .step_DBD1
    dw .step_DBDA
    dw .step_DC01
    dw .step_DC27
    dw .step_DC36
    dw .step_DC5C
    dw .step_DC01
    dw .step_DC27
    dw .step_DC36
    dw .step_DC87
    dw .step_DC01
    dw .step_DC27
    dw .step_DC36
    dw .step_DCB2
    dw .step_DCBC
    dw .step_DCCE
    dw .step_DCE0
    dw .step_DCF2
    dw .step_DD04
    dw .step_DD16
    dw .step_DD28
    dw .step_DD4C
.step_DBC2:
    dec.w !wram_115A
    bne .lDBD0
    lda.w #$001E
    sta.w !wram_115A
    inc.w !wram_10D8
.lDBD0:
    rts
.step_DBD1:
    dec.w !wram_115A
    bne .lDBD9
    inc.w !wram_10D8
.lDBD9:
    rts
.step_DBDA:
    sep.b #$20
    lda.w !lap_times+9
    sta.b !save_block_offset
    lda.w !lap_times+10
    sta.b !glyph_time_mid
    lda.w !lap_times+11
    sta.b !record_page
    rep.b #$20
    stz.w !wram_115A+2
    inc.w !wram_10D8
    lda.w !wram_0D4A
    and.w #$00FF
    asl a
    asl a
    asl a
    tax
    jsr.w GlyphListMessageAndTime
    rts
.step_DC01:
    lda.w !wram_115A+2
    tax
    lda.w #$00FF
    sta.w !glyph_list_on,x
    inc.w !wram_115A+2
    inc.w !wram_115A+2
    lda.w !wram_115A+2
    lsr a
    cmp.w !glyph_list_count
    bcc .lDC23
    inc.w !wram_10D8
    lda.w #$000A
    sta.w !wram_115A
.lDC23:
    jsr.w GlyphListDraw
    rts
.step_DC27:
    dec.w !wram_115A
    bne .lDC32
    stz.w !wram_115A+2
    inc.w !wram_10D8
.lDC32:
    jsr.w GlyphListDraw
    rts
.step_DC36:
    lda.w !wram_115A+2
    tax
    lda.w #$0000
    sta.w !glyph_list_on,x
    inc.w !wram_115A+2
    inc.w !wram_115A+2
    lda.w !wram_115A+2
    lsr a
    cmp.w !glyph_list_count
    bcc .lDC58
    inc.w !wram_10D8
    lda.w #$0004
    sta.w !wram_115A
.lDC58:
    jsr.w GlyphListDraw
    rts
.step_DC5C:
    sep.b #$20
    lda.w !lap_times+12
    sta.b !save_block_offset
    lda.w !lap_times+13
    sta.b !glyph_time_mid
    lda.w !lap_times+14
    sta.b !record_page
    rep.b #$20
    stz.w !wram_115A+2
    inc.w !wram_10D8
    lda.w !wram_0D4A
    and.w #$00FF
    asl a
    asl a
    asl a
    clc
    adc.w #$0002
    tax
    jsr.w GlyphListMessageAndTime
    rts
.step_DC87:
    sep.b #$20
    lda.w !lap_times+15
    sta.b !save_block_offset
    lda.w !lap_times+16
    sta.b !glyph_time_mid
    lda.w !lap_times+17
    sta.b !record_page
    rep.b #$20
    stz.w !wram_115A+2
    inc.w !wram_10D8
    lda.w !wram_0D4A
    and.w #$00FF
    asl a
    asl a
    asl a
    clc
    adc.w #$0004
    tax
    jsr.w GlyphListMessageAndTime
    rts
.step_DCB2:
    lda.w #$0004
    sta.w !wram_115A
    inc.w !wram_10D8
    rts
.step_DCBC:
    dec.w !wram_115A
    bne .lDCCA
    lda.w #$0004
    sta.w !wram_115A
    inc.w !wram_10D8
.lDCCA:
    jsr.w GlyphRowY60From5
    rts
.step_DCCE:
    dec.w !wram_115A
    bne .lDCDC
    lda.w #$0004
    sta.w !wram_115A
    inc.w !wram_10D8
.lDCDC:
    jsr.w GlyphRowY60From4
    rts
.step_DCE0:
    dec.w !wram_115A
    bne .lDCEE
    lda.w #$0008
    sta.w !wram_115A
    inc.w !wram_10D8
.lDCEE:
    jsr.w GlyphRowY60From3
    rts
.step_DCF2:
    dec.w !wram_115A
    bne .lDD00
    lda.w #$0008
    sta.w !wram_115A
    inc.w !wram_10D8
.lDD00:
    jsr.w GlyphRowY60From2
    rts
.step_DD04:
    dec.w !wram_115A
    bne .lDD12
    lda.w #$0008
    sta.w !wram_115A
    inc.w !wram_10D8
.lDD12:
    jsr.w GlyphRowY60From1
    rts
.step_DD16:
    dec.w !wram_115A
    bne .lDD24
    lda.w #$0004
    sta.w !wram_115A
    inc.w !wram_10D8
.lDD24:
    jsr.w GlyphRowY60From0
    rts
.step_DD28:
    dec.w !wram_115A
    bne .lDD48
    lda.w #$0014
    sta.w !wram_115A
    inc.w !wram_10D8
    sep.b #$20
    lda.b #$01
    sta.w !window_hdma_off
    stz.w !wram_19DF
    lda.b #$04
    jsl.l SoundPort1QueuePush
    rep.b #$20
.lDD48:
    jsr.w GlyphRowY60From0
    rts
.step_DD4C:
    dec.w !wram_115A
    bne .lDD5B
    sep.b #$20
    lda.b #$07
    pha
    plb
    jml.l $07EE26
.lDD5B:
    rep.b #$30
    jsr.w GlyphRowY60From0
    rts

; @routine 04:DAE2 GlyphSpritesGrowAndTimes
; WHAT IT DOES: Once the best times' display has started ($10D8 not 0):
; each glyph sprite's size step $1040 (0-7) grows by one once the sprite
; before it has reached 3 (the first always), and every sprite with a
; step is drawn scaled (program $07:FAF2, the renderer bytes 0, $BF, $B7,
; $B6) at its x and y plus the step's offsets, with the step's size and
; scale (tables at $04:DAA2, $DAB2, $DAC2, $DAD2). Then GlyphTimesSequence.
; HOW: Data bank $04 (the program's) for the tables; P and the data bank
; saved and restored.
; Entry: any widths, D = $0000. Exit: P and the data bank restored; A, X,
; Y as GlyphTimesSequence leaves them.
%at($04DAE2)
GlyphSpritesGrowAndTimes:
    php
    sep.b #$20
    phb
    phk
    plb
    rep.b #$30
    lda.w !wram_10D8
    bne .lDAF2
    brl .lDB7F
.lDAF2:
    ldx.w #$0002
.lDAF5:
    lda.w !glyph_spr_1040-2,x
    cmp.w #$0003
    bcc .lDB09
    lda.w !glyph_spr_1040,x
    cmp.w #$0007
    bcs .lDB09
    inc a
    sta.w !glyph_spr_1040,x
.lDB09:
    inx
    inx
    cpx.w #$0014
    bcc .lDAF5
    lda.w !glyph_spr_1040
    cmp.w #$0007
    bcs .lDB1B
    inc.w !glyph_spr_1040
.lDB1B:
    jsl.l GsuBytes200_BFB7B6
    ldx.w #$0000
.lDB22:
    lda.w !glyph_spr_1040,x
    beq .lDB78
    asl a
    tay
    lda.w !glyph_spr_sizes,y
    sta.l !fx_draw_advance
    sta.l !fx_draw_rows
    lda.w !glyph_spr_scales,y
    sta.l !fx_draw_step_x
    sta.l !fx_draw_step_y
    lda.w !glyph_spr_x,x
    clc
    adc.w !glyph_spr_dx,y
    sta.l !fx_draw_x
    lda.w #$0006
    sta.l !fx_draw_y
    lda.w !glyph_spr_y,x
    clc
    adc.w !glyph_spr_dy,y
    sta.l !fx_draw_y
    lda.w !glyph_spr_glyph,x
    sta.l !gsu_in_216
    phx
    php
    sep.b #$20
    rep.b #$10
    ldx.w #$FAF2
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    plp
    plx
.lDB78:
    inx
    inx
    cpx.w #$0020
    bne .lDB22
.lDB7F:
    jsr.w GlyphTimesSequence
    sep.b #$20
    plb
    plp
    rtl

; @routine 07:9814 GlyphSlideThree
; WHAT IT DOES: Slides the first three glyph sprites left: each one's x
; goes down by its step $1040 but not below its target $1070 (nor past
; 0), and it is drawn at y $50 with program $07:F3D2, the renderer bytes
; by its glyph: 0, $BF, $B7, $B6 for $8000, $BC, $BD for $8006, $BA, $B9
; for $800C, $B3, $BE for the others.
; HOW: 16-bit; X and P kept on the stack around GsuRun; P saved and
; restored.
; Entry: any widths. Exit: P restored; X = 6; A as the last draw left it.
%at($079814)
GlyphSlideThree:
    php
    rep.b #$30
    ldx.w #$0000
.l981A:
    lda.w !glyph_spr_x,x
    sec
    sbc.w !glyph_spr_1040,x
    bcc .l9828
    cmp.w !glyph_spr_1040+48,x
    bcs .l982B
.l9828:
    lda.w !glyph_spr_1040+48,x
.l982B:
    sta.w !glyph_spr_x,x
    sta.l !fx_draw_x
    lda.w #$0050
    sta.l !fx_draw_y
    lda.w !glyph_spr_glyph,x
    sta.l !gsu_in_216
    cmp.w #$8000
    beq .l9861
    cmp.w #$8006
    beq .l985B
    cmp.w #$800C
    beq .l9855
    jsl.l GsuBytes200_BFB3BE
    bra .l9865
.l9855:
    jsl.l GsuBytes200_BFBAB9        ; not reached in the traces so far
    bra .l9865                      ; not reached in the traces so far
.l985B:
    jsl.l GsuBytes200_BFBCBD        ; not reached in the traces so far
    bra .l9865                      ; not reached in the traces so far
.l9861:
    jsl.l GsuBytes200_BFB7B6
.l9865:
    phx
    php
    sep.b #$20
    rep.b #$10
    ldx.w #$F3D2
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    plp
    plx
    inx
    inx
    cpx.w #$0006
    bcc .l981A
    plp
    rts


; @routine 07:A39A GlyphSpritesDrawAll
; WHAT IT DOES: Draws glyph sprites 1 to 11 (GlyphSpriteDraw) with the
; renderer bytes switched between them (GsuBytes200_BFB3BE before 1 and
; 4, GsuBytes200_BFBCBD before 2 and 9); then, with $0FAE set, sprites 12
; to 24 (BFB3BE before 12, BF12B6 before 16), or with $0FAE 0 only 12 to
; 15, each 8 pixels to the left; then the glyph pictures GlyphGrid1A and
; GlyphGrid1D.
; HOW: X steps by 2 through the sprites; the left-shifted ones by
; GlyphSpriteDrawLeft8 (only from unit_cpu).
; Entry: D = $0000. Exit: as GlyphGrid1D leaves it.
%at($07A39A)
GlyphSpritesDrawAll:
    rep.b #$30
    ldx.w #$0002
    jsl.l GsuBytes200_BFB3BE
    jsr.w GlyphSpriteDraw
    jsl.l GsuBytes200_BFBCBD
    jsr.w GlyphSpriteDraw
    jsr.w GlyphSpriteDraw
    jsl.l GsuBytes200_BFB3BE
    jsr.w GlyphSpriteDraw
    jsr.w GlyphSpriteDraw
    jsr.w GlyphSpriteDraw
    jsr.w GlyphSpriteDraw
    jsr.w GlyphSpriteDraw
    jsl.l GsuBytes200_BFBCBD
    jsr.w GlyphSpriteDraw
    jsr.w GlyphSpriteDraw
    jsr.w GlyphSpriteDraw
    lda.w !p1_lead+11
    bne .lA3E7
    jsl.l GsuBytes200_BFB3BE        ; not reached in the traces so far
    jsr.w GlyphSpriteDrawLeft8      ; not reached in the traces so far
    jsr.w GlyphSpriteDrawLeft8      ; not reached in the traces so far
    jsr.w GlyphSpriteDrawLeft8      ; not reached in the traces so far
    jsr.w GlyphSpriteDrawLeft8      ; not reached in the traces so far
    bra .lA416                      ; not reached in the traces so far
.lA3E7:
    jsl.l GsuBytes200_BFB3BE
    jsr.w GlyphSpriteDraw
    jsr.w GlyphSpriteDraw
    jsr.w GlyphSpriteDraw
    jsr.w GlyphSpriteDraw
    jsl.l GsuBytes200_BF12B6
    jsr.w GlyphSpriteDraw
    jsr.w GlyphSpriteDraw
    jsr.w GlyphSpriteDraw
    jsr.w GlyphSpriteDraw
    jsr.w GlyphSpriteDraw
    jsr.w GlyphSpriteDraw
    jsr.w GlyphSpriteDraw
    jsr.w GlyphSpriteDraw
    jsr.w GlyphSpriteDraw
.lA416:
    jsr.w GlyphGrid1A
    jsr.w GlyphGrid1D
    rts
; @routine 07:A41D GlyphSpriteDrawLeft8
; WHAT IT DOES: As GlyphSpriteDraw, 8 pixels to the left.
; HOW: Subtracts 8 from the sprite's x and joins GlyphSpriteDraw after its
; first load. Only GlyphSpritesDrawAll calls it, with $0FAE 0 (unit_cpu).
; Entry: M 16-bit, X 16-bit; X = the sprite * 2. Exit: as GlyphSpriteDraw.
GlyphSpriteDrawLeft8:
    lda.w !glyph_spr_x,x            ; not reached in the traces so far
    sec                             ; not reached in the traces so far
    sbc.w #$0008                    ; not reached in the traces so far
    bra GlyphSpriteDraw_at          ; not reached in the traces so far

