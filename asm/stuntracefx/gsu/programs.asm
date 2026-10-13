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
; $0F69 at (147, 108) (GsuF329Sequence, through the tail of the routine at
; $07:89F5).
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
    jmp.w GsuF329At93x6C


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

