; spc_race_driver.asm: the race sound driver, Nintendo's N-SPC engine as
; uploaded by ApuUploadBank19 (block list $19:C27C), being filed routine by
; routine; routines not yet described carry plain labels.

incsrc "../include/macros.inc"
incsrc "../include/spc.inc"

%spc($0670, $19C2FC)
    db $94, $04, $26, $08, $53

Spc_0675:
    mov a,#$AA                        ; 0675
    mov.w !spc_cpuio0,a               ; 0677
    mov a,#$BB                        ; 067A
    mov.w !spc_cpuio1,a               ; 067C
.l067F:
    mov.w a,!spc_cpuio0               ; 067F
    cmp a,#$CC                        ; 0682
    bne .l067F                        ; 0684
    bra .l06A8                        ; 0686
.l0688:
    mov.w y,!spc_cpuio0               ; 0688
    bne .l0688                        ; 068B
.l068D:
    cmp.w y,!spc_cpuio0               ; 068D
    bne .l06A1                        ; 0690
    mov.w a,!spc_cpuio1               ; 0692
    mov.w !spc_cpuio0,y               ; 0695
    mov ($14)+y,a                     ; 0698
    inc y                             ; 069A
    bne .l068D                        ; 069B
    inc.b $15                         ; 069D
    bra .l068D                        ; 069F
.l06A1:
    bpl .l068D                        ; 06A1
    cmp.w y,!spc_cpuio0               ; 06A3
    bpl .l068D                        ; 06A6
.l06A8:
    mov.w a,!spc_cpuio2               ; 06A8
    mov.w y,!spc_cpuio3               ; 06AB
    movw $14,ya                       ; 06AE
    mov.w y,!spc_cpuio0               ; 06B0
    mov.w a,!spc_cpuio1               ; 06B3
    mov.w !spc_cpuio0,y               ; 06B6
    bne .l0688                        ; 06B9
    mov x,#$31                        ; 06BB
    mov.w !spc_control,x              ; 06BD
    jmp Spc_0800                      ; 06C0
    db $61, $63, $4E, $4A, $48, $45, $0E, $49
    db $4B, $46, $5F, $08, $DE, $08, $65, $09
    db $F4, $09, $8C, $0A, $2C, $0B, $D6, $0B
    db $8B, $0C, $4A, $0D, $14, $0E, $EA, $0E
    db $CD, $0F, $BE, $10, $18, $15, $59, $16
    db $AD, $17, $16, $19, $94, $1A, $28, $1C
    db $D5, $1D, $9B, $1F, $2A, $56, $65, $72
    db $20, $53, $31, $2E, $32, $30, $2A, $94
    db $C6, $77, $C6, $9C, $BE, $8A, $C6, $73
    db $BD, $51, $C1, $80, $C6, $A9, $BD, $14
    db $BD, $9E, $C6, $B2, $BD, $3E, $BD, $A8
    db $C6, $A0, $BC, $7E, $BE, $B5, $BE, $63
    db $C4, $6B, $C4, $73, $C4, $7B, $C4, $D2
    db $BF, $21, $BF, $FA, $BF, $F0, $C5, $16
    db $C6, $39, $C6, $DA, $C6, $53, $C6, $E7
    db $C5, $EE, $BF, $89, $C5, $55, $BF, $E1
    db $BF, $72, $C1, $5F, $C1, $30, $C5, $BF
    db $BF, $0D, $C6, $ED, $C6, $AF, $BF, $F5
    db $C0, $98, $C5, $E0, $C1, $54, $C4, $00
    db $BE, $C1, $BD, $8D, $BE, $D3, $BE, $74
    db $BF, $1A, $C0, $A9, $C5, $30, $C6, $FF
    db $C6, $9D, $C0, $B0, $BC, $2C, $BD, $14
    db $C7, $79, $C1, $43, $C1, $0B, $C1, $3D
    db $C4, $E7, $C4, $7F, $C1, $1A, $C4, $23
    db $BE, $5A, $BE, $67, $BE, $85, $BF, $9A
    db $BF, $B5, $C0, $C8, $C0, $D1, $C0, $EC
    db $C0, $FA, $C1, $B1, $C1, $4F, $C3, $91
    db $C1, $DF, $BD, $EA, $C3, $F5, $C3, $FB
    db $C3, $FF, $C3, $83, $C4, $99, $C4, $C0
    db $C4, $14, $C5, $E8, $BC, $6E, $C6, $26
    db $C7, $C9, $C5, $43, $C6, $A0, $BC, $AD
    db $C4, $6E, $C5, $05, $BD, $52, $C5, $38
    db $C1, $11, $BF, $BD, $BE, $F3, $C1, $06
    db $C5, $42, $BF, $C9, $BC, $31, $C4, $41
    db $BE, $3B, $C0, $61, $C0, $7D, $C3, $95
    db $C3, $C9, $C1, $37, $C2, $87, $C2, $DC
    db $C2, $C0, $C6, $B9, $C6, $B2, $C6, $C7
    db $C6, $CE, $C6, $D5, $C6, $D5, $C6, $D5
    db $C6, $D5, $C6, $40, $EE, $E2, $C8, $35
    db $C7, $C0, $E0, $10, $CB

Spc_0800:
    clrp                              ; 0800
    mov x,#$CF                        ; 0801
    mov sp,x                          ; 0803
    mov a,#$00                        ; 0804
    mov x,a                           ; 0806
.l0807:
    mov (x+),a                        ; 0807
    cmp x,#$E0                        ; 0808
    bne .l0807                        ; 080A
    inc a                             ; 080C
    call Spc_0D73                     ; 080D
    mov a,#$00                        ; 0810
    mov x,a                           ; 0812
.l0813:
    mov.w $0200+x,a                   ; 0813
    inc x                             ; 0816
    bne .l0813                        ; 0817
.l0819:
    mov.w $0300+x,a                   ; 0819
    inc x                             ; 081C
    bne .l0819                        ; 081D
.l081F:
    mov.w $0400+x,a                   ; 081F
    inc x                             ; 0822
    bne .l081F                        ; 0823
.l0825:
    mov.w $0500+x,a                   ; 0825
    inc x                             ; 0828
    cmp x,#$90                        ; 0829
    bne .l0825                        ; 082B
    mov x,a                           ; 082D
    inc a                             ; 082E
.l082F:
    mov.w $0361+x,a                   ; 082F
    inc x                             ; 0832
    inc x                             ; 0833
    cmp x,#$10                        ; 0834
    bne .l082F                        ; 0836
    mov a,#$60                        ; 0838
    mov y,#$0C                        ; 083A
    call Spc_09E8                     ; 083C
    mov y,#$1C                        ; 083F
    call Spc_09E8                     ; 0841
    mov a,#$3F                        ; 0844
    mov y,#$5D                        ; 0846
    call Spc_09E8                     ; 0848
    mov a,#$30                        ; 084B
    mov.w !spc_control,a              ; 084D
    mov a,#$10                        ; 0850
    mov.w !spc_t0target,a             ; 0852
    mov.b $53,a                       ; 0855
    mov a,#$01                        ; 0857
    mov.w !spc_control,a              ; 0859
    dec a                             ; 085C
    mov.b $04,a                       ; 085D
    mov.b $05,a                       ; 085F
    mov.b $06,a                       ; 0861
    mov.b $07,a                       ; 0863
    dec a                             ; 0865
    mov.b $46,a                       ; 0866
.l0868:
    mov y,#$0A                        ; 0868
.l086A:
    cmp y,#$05                        ; 086A
    beq .l0875                        ; 086C
    bcs .l0878                        ; 086E
    cmp $4C,$4D                       ; 0870
    bne .l0886                        ; 0873
.l0875:
    bbs $4C.7,.l0886                  ; 0875
.l0878:
    mov.w a,$3F9F+y                   ; 0878
    mov.w !spc_dspaddr,a              ; 087B
    mov.w a,$06C2+y                   ; 087E
    mov x,a                           ; 0881
    mov a,(x)                         ; 0882
    mov.w !spc_dspdata,a              ; 0883
.l0886:
    dbnz y,.l086A                     ; 0886
    mov.b $45,y                       ; 0888
    mov.b $46,y                       ; 088A
    mov.b a,$18                       ; 088C
    eor.b a,$19                       ; 088E
    lsr a                             ; 0890
    lsr a                             ; 0891
    notc                              ; 0892
    ror.b $18                         ; 0893
    ror.b $19                         ; 0895
.l0897:
    mov.w y,!spc_t0out                ; 0897
    beq .l0897                        ; 089A
    push y                            ; 089C
    mov a,#$20                        ; 089D
    mul ya                            ; 089F
    clrc                              ; 08A0
    adc.b a,$43                       ; 08A1
    mov.b $43,a                       ; 08A3
    bcc .l08B1                        ; 08A5
    call Spc_10B0                     ; 08A7
    cmp $4C,$4D                       ; 08AA
    beq .l08B1                        ; 08AD
    inc.b $4C                         ; 08AF
.l08B1:
    mov.b a,$53                       ; 08B1
    pop y                             ; 08B3
    mul ya                            ; 08B4
    clrc                              ; 08B5
    adc.b a,$51                       ; 08B6
    mov.b $51,a                       ; 08B8
    bcc .l08C9                        ; 08BA
    call Spc_0A6F                     ; 08BC
    mov x,#$03                        ; 08BF
.l08C1:
    call Spc_08E2                     ; 08C1
    dec x                             ; 08C4
    bpl .l08C1                        ; 08C5
    bra .l0868                        ; 08C7
.l08C9:
    mov.b a,$1C                       ; 08C9
    beq .l0868                        ; 08CB
    mov x,#$00                        ; 08CD
    mov $47,#$01                      ; 08CF
.l08D2:
    mov.b a,$31+x                     ; 08D2
    beq .l08D9                        ; 08D4
    call Spc_0FD6                     ; 08D6
.l08D9:
    inc x                             ; 08D9
    inc x                             ; 08DA
    asl.b $47                         ; 08DB
    bne .l08D2                        ; 08DD
    jmp $0868                         ; 08DF

Spc_08E2:
    mov.b a,$04+x                     ; 08E2
    mov.w !spc_cpuio0+x,a             ; 08E4
.l08E7:
    mov.w a,!spc_cpuio0+x             ; 08E7
    cmp.w a,!spc_cpuio0+x             ; 08EA
    bne .l08E7                        ; 08ED
    mov.w $0558+x,a                   ; 08EF
.l08F2:
    ret                               ; 08F2

Spc_08F3:
    cmp y,#$CA                        ; 08F3
    bcc .l08FC                        ; 08F5
    call Spc_0BBE                     ; 08F7
    mov y,#$A4                        ; 08FA
.l08FC:
    cmp y,#$C8                        ; 08FC
    bcs Spc_08E2_l08F2                ; 08FE
    mov.b a,$1A                       ; 0900
    and.b a,$47                       ; 0902
    bne Spc_08E2_l08F2                ; 0904
    mov a,y                           ; 0906
    and a,#$7F                        ; 0907
    clrc                              ; 0909
    adc.b a,$50                       ; 090A
    clrc                              ; 090C
    adc.w a,$02F0+x                   ; 090D
    mov.w $0361+x,a                   ; 0910
    mov.w a,$0381+x                   ; 0913
    mov.w $0360+x,a                   ; 0916
    mov.w a,$02B1+x                   ; 0919
    lsr a                             ; 091C
    mov a,#$00                        ; 091D
    ror a                             ; 091F
    mov.w $02A0+x,a                   ; 0920
    mov a,#$00                        ; 0923
    mov.b $B0+x,a                     ; 0925
    mov.w $0100+x,a                   ; 0927
    mov.w $02D0+x,a                   ; 092A
    mov.b $C0+x,a                     ; 092D
    or $5E,$47                        ; 092F
    or $45,$47                        ; 0932
    mov.w a,$0280+x                   ; 0935
    mov.b $A0+x,a                     ; 0938
    beq .l095A                        ; 093A
    mov.w a,$0281+x                   ; 093C  ; not reached in the traces so far
    mov.b $A1+x,a                     ; 093F  ; not reached in the traces so far
    mov.w a,$0290+x                   ; 0941  ; not reached in the traces so far
    bne .l0950                        ; 0944  ; not reached in the traces so far
    mov.w a,$0361+x                   ; 0946  ; not reached in the traces so far
    setc                              ; 0949  ; not reached in the traces so far
    sbc.w a,$0291+x                   ; 094A  ; not reached in the traces so far
    mov.w $0361+x,a                   ; 094D  ; not reached in the traces so far
.l0950:
    mov.w a,$0291+x                   ; 0950  ; not reached in the traces so far
    clrc                              ; 0953  ; not reached in the traces so far
    adc.w a,$0361+x                   ; 0954  ; not reached in the traces so far
    call $0DDC                        ; 0957  ; not reached in the traces so far
.l095A:
    call Spc_0DF4                     ; 095A
.l095D:
    mov y,#$00                        ; 095D
    mov.b a,$11                       ; 095F
    setc                              ; 0961
    sbc a,#$34                        ; 0962
    bcs .l096F                        ; 0964
    mov.b a,$11                       ; 0966
    setc                              ; 0968
    sbc a,#$13                        ; 0969
    bcs .l0973                        ; 096B
    dec y                             ; 096D
    asl a                             ; 096E
.l096F:
    addw ya,$10                       ; 096F
    movw $10,ya                       ; 0971
.l0973:
    push x                            ; 0973
    mov.b a,$11                       ; 0974
    asl a                             ; 0976
    mov y,#$00                        ; 0977
    mov x,#$18                        ; 0979
    div ya,x                          ; 097B
    mov x,a                           ; 097C
    mov.w a,$06CE+y                   ; 097D
    mov.b $15,a                       ; 0980
    mov.w a,$06CD+y                   ; 0982
    mov.b $14,a                       ; 0985
    mov.w a,$06D0+y                   ; 0987
    push a                            ; 098A
    mov.w a,$06CF+y                   ; 098B
    pop y                             ; 098E
    subw ya,$14                       ; 098F
    mov.b y,$10                       ; 0991
    mul ya                            ; 0993
    mov a,y                           ; 0994
    mov y,#$00                        ; 0995
    addw ya,$14                       ; 0997
    mov.b $15,y                       ; 0999
    asl a                             ; 099B
    rol.b $15                         ; 099C
    mov.b $14,a                       ; 099E
    bra .l09A6                        ; 09A0
.l09A2:
    lsr.b $15                         ; 09A2
    ror a                             ; 09A4
    inc x                             ; 09A5
.l09A6:
    cmp x,#$06                        ; 09A6
    bne .l09A2                        ; 09A8
    mov.b $14,a                       ; 09AA
    pop x                             ; 09AC
    mov.w a,$0220+x                   ; 09AD
    mov.b y,$15                       ; 09B0
    mul ya                            ; 09B2
    movw $16,ya                       ; 09B3
    mov.w a,$0220+x                   ; 09B5
    mov.b y,$14                       ; 09B8
    mul ya                            ; 09BA
    push y                            ; 09BB
    mov.w a,$0221+x                   ; 09BC
    mov.b y,$14                       ; 09BF
    mul ya                            ; 09C1
    addw ya,$16                       ; 09C2
    movw $16,ya                       ; 09C4
    mov.w a,$0221+x                   ; 09C6
    mov.b y,$15                       ; 09C9
    mul ya                            ; 09CB
    mov y,a                           ; 09CC
    pop a                             ; 09CD
    addw ya,$16                       ; 09CE
    movw $16,ya                       ; 09D0
    mov a,x                           ; 09D2
    xcn a                             ; 09D3
    lsr a                             ; 09D4
    or a,#$02                         ; 09D5
    mov y,a                           ; 09D7
    mov.b a,$16                       ; 09D8
    call Spc_09E0                     ; 09DA
    inc y                             ; 09DD
    mov.b a,$17                       ; 09DE

Spc_09E0:
    push a                            ; 09E0
    mov.b a,$47                       ; 09E1
    and.b a,$1A                       ; 09E3
    pop a                             ; 09E5
    bne Spc_09E8_l09EE                ; 09E6

Spc_09E8:
    mov.w !spc_dspaddr,y              ; 09E8
    mov.w !spc_dspdata,a              ; 09EB
.l09EE:
    ret                               ; 09EE
    db $CC, $F2, $00, $E5, $F3, $00, $6F

Spc_09F6:
    mov y,#$00                        ; 09F6
    mov a,($40)+y                     ; 09F8
    incw $40                          ; 09FA
    push a                            ; 09FC
    mov a,($40)+y                     ; 09FD
    incw $40                          ; 09FF
    mov y,a                           ; 0A01
    pop a                             ; 0A02
    ret                               ; 0A03
.l0A04:
    mov x,#$0E                        ; 0A04
    mov $47,#$80                      ; 0A06
.l0A09:
    mov a,#$FF                        ; 0A09
    mov.w $0301+x,a                   ; 0A0B
    mov a,#$0A                        ; 0A0E
    call Spc_0C17                     ; 0A10
    mov.w $0211+x,a                   ; 0A13
    mov.w $0381+x,a                   ; 0A16
    mov.w $02F0+x,a                   ; 0A19
    mov.w $0280+x,a                   ; 0A1C
    mov.b $B1+x,a                     ; 0A1F
    mov.b $C1+x,a                     ; 0A21
    dec x                             ; 0A23
    dec x                             ; 0A24
    lsr.b $47                         ; 0A25
    bne .l0A09                        ; 0A27
    mov.b $5A,a                       ; 0A29
    mov.b $68,a                       ; 0A2B
    mov.b $54,a                       ; 0A2D
    mov.b $50,a                       ; 0A2F
    mov.b $42,a                       ; 0A31
    mov.b $5F,a                       ; 0A33
    mov $59,#$C0                      ; 0A35
    mov $53,#$20                      ; 0A38
.l0A3B:
    ret                               ; 0A3B
.l0A3C:
    bra Spc_0A60                      ; 0A3C
.l0A3E:
    mov.w $0584,a                     ; 0A3E
    mov.b a,$1B                       ; 0A41
    beq Spc_0A6F_l0A81                ; 0A43
    mov.b $1C,a                       ; 0A45
    mov $1B,#$00                      ; 0A47
    asl a                             ; 0A4A
    mov x,a                           ; 0A4B
    mov.w a,$07F5+x                   ; 0A4C
    mov y,a                           ; 0A4F
    mov.w a,$07F4+x                   ; 0A50
    movw $40,ya                       ; 0A53
    mov $0C,#$02                      ; 0A55
.l0A58:
    mov.b a,$1A                       ; 0A58
    eor a,#$FF                        ; 0A5A
    tset $0046,a                      ; 0A5C
    ret                               ; 0A5F

Spc_0A60:
    bbs $29.1,Spc_09F6_l0A3B          ; 0A60
    set1 $29.1                        ; 0A63
    mov $1B,#$00                      ; 0A65
    mov $1C,#$00                      ; 0A68
    bra Spc_09F6_l0A58                ; 0A6B
.l0A6D:
    bra Spc_09F6_l0A04                ; 0A6D

Spc_0A6F:
    bbs $29.0,Spc_09F6_l0A58          ; 0A6F
    mov.b a,$1B                       ; 0A72
    beq .l0A7B                        ; 0A74
    cmp.w a,$0584                     ; 0A76
    bne Spc_09F6_l0A3E                ; 0A79
.l0A7B:
    mov.w $0584,a                     ; 0A7B
    mov $1B,#$00                      ; 0A7E
.l0A81:
    mov.b a,$1C                       ; 0A81
    beq Spc_09F6_l0A3B                ; 0A83
    mov.b a,$0C                       ; 0A85
    beq .l0AD5                        ; 0A87
    dbnz $0C,Spc_0A60_l0A6D           ; 0A89
.l0A8C:
    call Spc_09F6                     ; 0A8C
    bne .l0AA5                        ; 0A8F
    mov y,a                           ; 0A91
    beq Spc_09F6_l0A3C                ; 0A92
    dec.b $42                         ; 0A94
    bpl .l0A9A                        ; 0A96
    mov.b $42,a                       ; 0A98
.l0A9A:
    call Spc_09F6                     ; 0A9A
    mov.b x,$42                       ; 0A9D
    beq .l0A8C                        ; 0A9F
    movw $40,ya                       ; 0AA1
    bra .l0A8C                        ; 0AA3
.l0AA5:
    movw $16,ya                       ; 0AA5
    mov y,#$0F                        ; 0AA7
.l0AA9:
    mov a,($16)+y                     ; 0AA9
    mov.w $0030+y,a                   ; 0AAB
    dec y                             ; 0AAE
    bpl .l0AA9                        ; 0AAF
    mov x,#$00                        ; 0AB1
    mov $47,#$01                      ; 0AB3
.l0AB6:
    mov.b a,$31+x                     ; 0AB6
    beq .l0AC4                        ; 0AB8
    mov.w a,$0211+x                   ; 0ABA
    bne .l0AC4                        ; 0ABD
    mov a,#$00                        ; 0ABF
    call Spc_0BBE                     ; 0AC1
.l0AC4:
    mov a,#$00                        ; 0AC4
    mov.b $80+x,a                     ; 0AC6
    mov.b $90+x,a                     ; 0AC8
    mov.b $91+x,a                     ; 0ACA
    inc a                             ; 0ACC
    mov.b $70+x,a                     ; 0ACD
    inc x                             ; 0ACF
    inc x                             ; 0AD0
    asl.b $47                         ; 0AD1
    bne .l0AB6                        ; 0AD3
.l0AD5:
    mov x,#$00                        ; 0AD5
    mov.b $5E,x                       ; 0AD7
    mov $47,#$01                      ; 0AD9
.l0ADC:
    mov.b $44,x                       ; 0ADC
    mov.b a,$31+x                     ; 0ADE
    beq .l0B4F                        ; 0AE0
    dec.b $70+x                       ; 0AE2
    bne .l0B49                        ; 0AE4
.l0AE6:
    call Spc_0BB4                     ; 0AE6
    bne .l0B06                        ; 0AE9
    mov.b a,$80+x                     ; 0AEB
    beq .l0A8C                        ; 0AED
    call Spc_0D05                     ; 0AEF
    dec.b $80+x                       ; 0AF2
    bne .l0AE6                        ; 0AF4
    mov.w a,$0230+x                   ; 0AF6
    mov.b $30+x,a                     ; 0AF9
    mov.w a,$0231+x                   ; 0AFB
    mov.b $31+x,a                     ; 0AFE
    bra .l0AE6                        ; 0B00
.l0B02:
    clr1 $2C.0                        ; 0B02
    bra .l0A8C                        ; 0B04
.l0B06:
    bbs $2C.0,.l0B02                  ; 0B06
    bmi .l0B2B                        ; 0B09
    mov.w $0200+x,a                   ; 0B0B
    call Spc_0BB4                     ; 0B0E
    bmi .l0B2B                        ; 0B11
    push a                            ; 0B13
    xcn a                             ; 0B14
    and a,#$07                        ; 0B15
    mov y,a                           ; 0B17
    mov.w a,$0650+y                   ; 0B18
    mov.w $0201+x,a                   ; 0B1B
    pop a                             ; 0B1E
    and a,#$0F                        ; 0B1F
    mov y,a                           ; 0B21
    mov.w a,$0658+y                   ; 0B22
    mov.w $0210+x,a                   ; 0B25
    call Spc_0BB4                     ; 0B28
.l0B2B:
    cmp a,#$E0                        ; 0B2B
    bcc .l0B34                        ; 0B2D
    call Spc_0BA2                     ; 0B2F
    bra .l0AE6                        ; 0B32
.l0B34:
    call Spc_08F3                     ; 0B34
    mov.w a,$0200+x                   ; 0B37
    mov.b $70+x,a                     ; 0B3A
    mov y,a                           ; 0B3C
    mov.w a,$0201+x                   ; 0B3D
    mul ya                            ; 0B40
    mov a,y                           ; 0B41
    bne .l0B45                        ; 0B42
    inc a                             ; 0B44
.l0B45:
    mov.b $71+x,a                     ; 0B45
    bra .l0B4C                        ; 0B47
.l0B49:
    call Spc_0EE4                     ; 0B49
.l0B4C:
    call Spc_0DBC                     ; 0B4C
.l0B4F:
    inc x                             ; 0B4F
    inc x                             ; 0B50
    asl.b $47                         ; 0B51
    bne .l0ADC                        ; 0B53
    mov.b a,$54                       ; 0B55
    beq .l0B64                        ; 0B57
    movw ya,$56                       ; 0B59  ; not reached in the traces so far
    addw ya,$52                       ; 0B5B  ; not reached in the traces so far
    dbnz $54,.l0B62                   ; 0B5D  ; not reached in the traces so far
    movw ya,$54                       ; 0B60  ; not reached in the traces so far
.l0B62:
    movw $52,ya                       ; 0B62  ; not reached in the traces so far
.l0B64:
    mov.b a,$68                       ; 0B64
    beq .l0B7D                        ; 0B66
    movw ya,$64                       ; 0B68  ; not reached in the traces so far
    addw ya,$60                       ; 0B6A  ; not reached in the traces so far
    movw $60,ya                       ; 0B6C  ; not reached in the traces so far
    movw ya,$66                       ; 0B6E  ; not reached in the traces so far
    addw ya,$62                       ; 0B70  ; not reached in the traces so far
    dbnz $68,.l0B7B                   ; 0B72  ; not reached in the traces so far
    movw ya,$68                       ; 0B75  ; not reached in the traces so far
    movw $60,ya                       ; 0B77  ; not reached in the traces so far
    mov.b y,$6A                       ; 0B79  ; not reached in the traces so far
.l0B7B:
    movw $62,ya                       ; 0B7B  ; not reached in the traces so far
.l0B7D:
    mov.b a,$5A                       ; 0B7D
    beq .l0B8F                        ; 0B7F
    movw ya,$5C                       ; 0B81  ; not reached in the traces so far
    addw ya,$58                       ; 0B83  ; not reached in the traces so far
    dbnz $5A,.l0B8A                   ; 0B85  ; not reached in the traces so far
    movw ya,$5A                       ; 0B88  ; not reached in the traces so far
.l0B8A:
    movw $58,ya                       ; 0B8A  ; not reached in the traces so far
    mov $5E,#$FF                      ; 0B8C  ; not reached in the traces so far
.l0B8F:
    mov x,#$00                        ; 0B8F
    mov $47,#$01                      ; 0B91
.l0B94:
    mov.b a,$31+x                     ; 0B94
    beq .l0B9B                        ; 0B96
    call Spc_0E1B                     ; 0B98
.l0B9B:
    inc x                             ; 0B9B
    inc x                             ; 0B9C
    asl.b $47                         ; 0B9D
    bne .l0B94                        ; 0B9F
    ret                               ; 0BA1

Spc_0BA2:
    asl a                             ; 0BA2
    mov y,a                           ; 0BA3
    mov.w a,$3F0B+y                   ; 0BA4
    push a                            ; 0BA7
    mov.w a,$3F0A+y                   ; 0BA8
    push a                            ; 0BAB
    mov a,y                           ; 0BAC
    lsr a                             ; 0BAD
    mov y,a                           ; 0BAE
    mov.w a,$102F+y                   ; 0BAF
    beq Spc_0BB6_l0BBC                ; 0BB2

Spc_0BB4:
    mov a,($30+x)                     ; 0BB4

Spc_0BB6:
    inc.b $30+x                       ; 0BB6
    bne .l0BBC                        ; 0BB8
    inc.b $31+x                       ; 0BBA
.l0BBC:
    mov y,a                           ; 0BBC
    ret                               ; 0BBD

Spc_0BBE:
    mov.w $0211+x,a                   ; 0BBE

Spc_0BC1:
    mov y,a                           ; 0BC1
    bpl .l0BCA                        ; 0BC2
    setc                              ; 0BC4
    sbc a,#$CA                        ; 0BC5
    clrc                              ; 0BC7
    adc.b a,$5F                       ; 0BC8
.l0BCA:
    mov y,#$06                        ; 0BCA
    mul ya                            ; 0BCC
    movw $14,ya                       ; 0BCD
    clrc                              ; 0BCF
    adc $14,#$90                      ; 0BD0
    adc $15,#$05                      ; 0BD3
    mov.b a,$1A                       ; 0BD6
    and.b a,$47                       ; 0BD8
    bne .l0C16                        ; 0BDA
    push x                            ; 0BDC
    mov a,x                           ; 0BDD
    xcn a                             ; 0BDE
    lsr a                             ; 0BDF
    or a,#$04                         ; 0BE0
    mov x,a                           ; 0BE2
    mov y,#$00                        ; 0BE3
    mov a,($14)+y                     ; 0BE5
    bpl .l0BF7                        ; 0BE7
    and a,#$1F                        ; 0BE9  ; not reached in the traces so far
    and $48,#$20                      ; 0BEB  ; not reached in the traces so far
    tset $0048,a                      ; 0BEE  ; not reached in the traces so far
    or $49,$47                        ; 0BF1  ; not reached in the traces so far
    mov a,y                           ; 0BF4  ; not reached in the traces so far
    bra .l0BFE                        ; 0BF5  ; not reached in the traces so far
.l0BF7:
    mov.b a,$47                       ; 0BF7
    tclr $0049,a                      ; 0BF9
.l0BFC:
    mov a,($14)+y                     ; 0BFC
.l0BFE:
    mov.w !spc_dspaddr,x              ; 0BFE
    mov.w !spc_dspdata,a              ; 0C01
    inc x                             ; 0C04
    inc y                             ; 0C05
    cmp y,#$04                        ; 0C06
    bne .l0BFC                        ; 0C08
    pop x                             ; 0C0A
    mov a,($14)+y                     ; 0C0B
    mov.w $0221+x,a                   ; 0C0D
    inc y                             ; 0C10
    mov a,($14)+y                     ; 0C11
    mov.w $0220+x,a                   ; 0C13
.l0C16:
    ret                               ; 0C16

Spc_0C17:
    mov.w $0351+x,a                   ; 0C17
    and a,#$1F                        ; 0C1A
    mov.w $0331+x,a                   ; 0C1C
    mov a,#$00                        ; 0C1F
    mov.w $0330+x,a                   ; 0C21
    ret                               ; 0C24
    mov.b $91+x,a                     ; 0C25
    push a                            ; 0C27
    call Spc_0BB4                     ; 0C28
    mov.w $0350+x,a                   ; 0C2B
    setc                              ; 0C2E
    sbc.w a,$0331+x                   ; 0C2F
    pop x                             ; 0C32
    call Spc_0DFF                     ; 0C33
    mov.w $0340+x,a                   ; 0C36
    mov a,y                           ; 0C39
    mov.w $0341+x,a                   ; 0C3A
    ret                               ; 0C3D
    mov.w $02B0+x,a                   ; 0C3E
    call Spc_0BB4                     ; 0C41
    mov.w $02A1+x,a                   ; 0C44
    call Spc_0BB4                     ; 0C47
    mov.b $B1+x,a                     ; 0C4A
    mov.w $02C1+x,a                   ; 0C4C
    mov a,#$00                        ; 0C4F
    mov.w $02B1+x,a                   ; 0C51
    ret                               ; 0C54
    db $D5, $B1, $02, $2D, $8D, $00, $F4, $B1
    db $CE, $9E, $F8, $44, $D5, $C0, $02, $6F
    mov a,#$00                        ; 0C65
    movw $58,ya                       ; 0C67
    ret                               ; 0C69
    db $C4, $5A, $3F, $B4, $0B, $C4, $5B, $80
    db $A4, $59, $F8, $5A, $3F, $FF, $0D, $DA
    db $5C, $6F
    mov a,#$00                        ; 0C7C
    movw $52,ya                       ; 0C7E
    ret                               ; 0C80
    db $C4, $54, $3F, $B4, $0B, $C4, $55, $80
    db $A4, $53, $F8, $54, $3F, $FF, $0D, $DA
    db $56, $6F, $C4, $50, $6F, $D5, $F0, $02
    db $6F, $D5, $E0, $02, $3F, $B4, $0B, $D5
    db $D1, $02, $3F, $B4, $0B, $D4, $C1, $6F
    db $E8, $01, $2F, $02, $E8, $00, $D5, $90
    db $02, $DD, $D5, $81, $02, $3F, $B4, $0B
    db $D5, $80, $02, $3F, $B4, $0B, $D5, $91
    db $02, $6F, $D5, $80, $02, $6F
    mov.w $0301+x,a                   ; 0CC7
    mov a,#$00                        ; 0CCA
    mov.w $0300+x,a                   ; 0CCC
    ret                               ; 0CCF
    mov.b $90+x,a                     ; 0CD0
    push a                            ; 0CD2
    call Spc_0BB4                     ; 0CD3
    mov.w $0320+x,a                   ; 0CD6
    setc                              ; 0CD9
    sbc.w a,$0301+x                   ; 0CDA
    pop x                             ; 0CDD
    call Spc_0DFF                     ; 0CDE
    mov.w $0310+x,a                   ; 0CE1
    mov a,y                           ; 0CE4
    mov.w $0311+x,a                   ; 0CE5
    ret                               ; 0CE8
    mov.w $0381+x,a                   ; 0CE9
    ret                               ; 0CEC
    mov.w $0240+x,a                   ; 0CED
    call Spc_0BB4                     ; 0CF0
    mov.w $0241+x,a                   ; 0CF3
    call Spc_0BB4                     ; 0CF6
    mov.b $80+x,a                     ; 0CF9
    mov.b a,$30+x                     ; 0CFB
    mov.w $0230+x,a                   ; 0CFD
    mov.b a,$31+x                     ; 0D00
    mov.w $0231+x,a                   ; 0D02

Spc_0D05:
    mov.w a,$0240+x                   ; 0D05
    mov.b $30+x,a                     ; 0D08
    mov.w a,$0241+x                   ; 0D0A
    mov.b $31+x,a                     ; 0D0D
    ret                               ; 0D0F
    mov.b $4F,a                       ; 0D10
    and.b a,$1A                       ; 0D12
    eor.b a,$4F                       ; 0D14
    mov.b $4A,a                       ; 0D16
    call Spc_0BB4                     ; 0D18
    mov a,#$00                        ; 0D1B
    movw $60,ya                       ; 0D1D
    call Spc_0BB4                     ; 0D1F
    mov a,#$00                        ; 0D22
    movw $62,ya                       ; 0D24
    clr1 $48.5                        ; 0D26
    ret                               ; 0D28
    db $C4, $68, $3F, $B4, $0B, $C4, $69, $80
    db $A4, $61, $F8, $68, $3F, $FF, $0D, $DA
    db $64, $3F, $B4, $0B, $C4, $6A, $80, $A4
    db $63, $F8, $68, $3F, $FF, $0D, $DA, $66
    db $6F, $DA, $60, $DA, $62, $A2, $48, $6F
    call Spc_0D73                     ; 0D51
    call Spc_0BB4                     ; 0D54
    mov.b $4E,a                       ; 0D57
    call Spc_0BB4                     ; 0D59
    mov y,#$08                        ; 0D5C
    mul ya                            ; 0D5E
    mov x,a                           ; 0D5F
    mov y,#$0F                        ; 0D60
.l0D62:
    mov.w a,$3FAA+x                   ; 0D62
    call Spc_09E8                     ; 0D65
    inc x                             ; 0D68
    mov a,y                           ; 0D69
    clrc                              ; 0D6A
    adc a,#$10                        ; 0D6B
    mov y,a                           ; 0D6D
    bpl .l0D62                        ; 0D6E
    mov.b x,$44                       ; 0D70
    ret                               ; 0D72

Spc_0D73:
    mov.b $4D,a                       ; 0D73
    mov y,#$7D                        ; 0D75
    mov.w !spc_dspaddr,y              ; 0D77
    mov.w a,!spc_dspdata              ; 0D7A
    cmp.b a,$4D                       ; 0D7D
    beq .l0DAC                        ; 0D7F
    and a,#$0F                        ; 0D81
    eor a,#$FF                        ; 0D83
    bbc $4C.7,.l0D8B                  ; 0D85
    clrc                              ; 0D88  ; not reached in the traces so far
    adc.b a,$4C                       ; 0D89  ; not reached in the traces so far
.l0D8B:
    mov.b $4C,a                       ; 0D8B
    mov y,#$04                        ; 0D8D
.l0D8F:
    mov.w a,$3F9F+y                   ; 0D8F
    mov.w !spc_dspaddr,a              ; 0D92
    mov a,#$00                        ; 0D95
    mov.w !spc_dspdata,a              ; 0D97
    dbnz y,.l0D8F                     ; 0D9A
    mov.b a,$48                       ; 0D9C
    or a,#$20                         ; 0D9E
    mov y,#$6C                        ; 0DA0
    call Spc_09E8                     ; 0DA2
    mov.b a,$4D                       ; 0DA5
    mov y,#$7D                        ; 0DA7
    call Spc_09E8                     ; 0DA9
.l0DAC:
    asl a                             ; 0DAC
    asl a                             ; 0DAD
    asl a                             ; 0DAE
    eor a,#$FF                        ; 0DAF
    setc                              ; 0DB1
    adc a,#$3F                        ; 0DB2
    mov y,#$6D                        ; 0DB4
    jmp Spc_09E8                      ; 0DB6
    mov.b $5F,a                       ; 0DB9
    ret                               ; 0DBB

Spc_0DBC:
    mov.b a,$A0+x                     ; 0DBC
    bne .l0DF3                        ; 0DBE
    mov a,($30+x)                     ; 0DC0
    cmp a,#$F9                        ; 0DC2
    bne .l0DF3                        ; 0DC4
    call Spc_0BB6                     ; 0DC6
    call Spc_0BB4                     ; 0DC9
    mov.b $A1+x,a                     ; 0DCC
    call Spc_0BB4                     ; 0DCE
    mov.b $A0+x,a                     ; 0DD1
    call Spc_0BB4                     ; 0DD3
    clrc                              ; 0DD6
    adc.b a,$50                       ; 0DD7
    adc.w a,$02F0+x                   ; 0DD9
.l0DDC:
    and a,#$7F                        ; 0DDC
    mov.w $0380+x,a                   ; 0DDE
    setc                              ; 0DE1
    sbc.w a,$0361+x                   ; 0DE2
    mov.b y,$A0+x                     ; 0DE5
    push y                            ; 0DE7
    pop x                             ; 0DE8
    call Spc_0DFF                     ; 0DE9
    mov.w $0370+x,a                   ; 0DEC
    mov a,y                           ; 0DEF
    mov.w $0371+x,a                   ; 0DF0
.l0DF3:
    ret                               ; 0DF3

Spc_0DF4:
    mov.w a,$0361+x                   ; 0DF4
    mov.b $11,a                       ; 0DF7
    mov.w a,$0360+x                   ; 0DF9
    mov.b $10,a                       ; 0DFC
    ret                               ; 0DFE

Spc_0DFF:
    notc                              ; 0DFF
    ror.b $12                         ; 0E00
    bpl .l0E07                        ; 0E02
    eor a,#$FF                        ; 0E04
    inc a                             ; 0E06
.l0E07:
    mov y,#$00                        ; 0E07
    div ya,x                          ; 0E09
    push a                            ; 0E0A
    mov a,#$00                        ; 0E0B
    div ya,x                          ; 0E0D
    pop y                             ; 0E0E
    mov.b x,$44                       ; 0E0F

Spc_0E11:
    bbc $12.7,.l0E1A                  ; 0E11
    movw $14,ya                       ; 0E14
    movw ya,$0E                       ; 0E16
    subw ya,$14                       ; 0E18
.l0E1A:
    ret                               ; 0E1A

Spc_0E1B:
    mov.b a,$90+x                     ; 0E1B
    beq .l0E43                        ; 0E1D
    or $5E,$47                        ; 0E1F
    dec.b $90+x                       ; 0E22
    bne .l0E30                        ; 0E24
    mov a,#$00                        ; 0E26
    mov.w $0300+x,a                   ; 0E28
    mov.w a,$0320+x                   ; 0E2B
    bra .l0E40                        ; 0E2E
.l0E30:
    clrc                              ; 0E30
    mov.w a,$0300+x                   ; 0E31
    adc.w a,$0310+x                   ; 0E34
    mov.w $0300+x,a                   ; 0E37
    mov.w a,$0301+x                   ; 0E3A
    adc.w a,$0311+x                   ; 0E3D
.l0E40:
    mov.w $0301+x,a                   ; 0E40
.l0E43:
    mov.b y,$C1+x                     ; 0E43
    beq .l0E6A                        ; 0E45
    mov.w a,$02E0+x                   ; 0E47  ; not reached in the traces so far
    cbne $C0+x,.l0E68                 ; 0E4A  ; not reached in the traces so far
    or $5E,$47                        ; 0E4D  ; not reached in the traces so far
    mov.w a,$02D0+x                   ; 0E50  ; not reached in the traces so far
    bpl .l0E5C                        ; 0E53  ; not reached in the traces so far
    inc y                             ; 0E55  ; not reached in the traces so far
    bne .l0E5C                        ; 0E56  ; not reached in the traces so far
    mov a,#$80                        ; 0E58  ; not reached in the traces so far
    bra .l0E60                        ; 0E5A  ; not reached in the traces so far
.l0E5C:
    clrc                              ; 0E5C  ; not reached in the traces so far
    adc.w a,$02D1+x                   ; 0E5D  ; not reached in the traces so far
.l0E60:
    mov.w $02D0+x,a                   ; 0E60  ; not reached in the traces so far
    call $105C                        ; 0E63  ; not reached in the traces so far
    bra .l0E6F                        ; 0E66  ; not reached in the traces so far
.l0E68:
    inc.b $C0+x                       ; 0E68  ; not reached in the traces so far
.l0E6A:
    mov a,#$FF                        ; 0E6A
    call Spc_1067                     ; 0E6C
.l0E6F:
    mov.b a,$91+x                     ; 0E6F
    beq Spc_0E97                      ; 0E71
    or $5E,$47                        ; 0E73
    dec.b $91+x                       ; 0E76
    bne .l0E84                        ; 0E78
    mov a,#$00                        ; 0E7A
    mov.w $0330+x,a                   ; 0E7C
    mov.w a,$0350+x                   ; 0E7F
    bra .l0E94                        ; 0E82
.l0E84:
    clrc                              ; 0E84
    mov.w a,$0330+x                   ; 0E85
    adc.w a,$0340+x                   ; 0E88
    mov.w $0330+x,a                   ; 0E8B
    mov.w a,$0331+x                   ; 0E8E
    adc.w a,$0341+x                   ; 0E91
.l0E94:
    mov.w $0331+x,a                   ; 0E94

Spc_0E97:
    mov.b a,$47                       ; 0E97
    and.b a,$5E                       ; 0E99
    beq Spc_0EA6_l0EE3                ; 0E9B
    mov.w a,$0331+x                   ; 0E9D
    mov y,a                           ; 0EA0
    mov.w a,$0330+x                   ; 0EA1
    movw $10,ya                       ; 0EA4

Spc_0EA6:
    mov a,x                           ; 0EA6
    xcn a                             ; 0EA7
    lsr a                             ; 0EA8
    mov.b $12,a                       ; 0EA9
.l0EAB:
    mov.b y,$11                       ; 0EAB
    mov.w a,$107B+y                   ; 0EAD
    setc                              ; 0EB0
    sbc.w a,$107A+y                   ; 0EB1
    mov.b y,$10                       ; 0EB4
    mul ya                            ; 0EB6
    mov a,y                           ; 0EB7
    mov.b y,$11                       ; 0EB8
    clrc                              ; 0EBA
    adc.w a,$107A+y                   ; 0EBB
    mov y,a                           ; 0EBE
    mov.w a,$0321+x                   ; 0EBF
    mul ya                            ; 0EC2
    mov.w a,$0351+x                   ; 0EC3
    asl a                             ; 0EC6
    bbc $12.0,.l0ECB                  ; 0EC7
    asl a                             ; 0ECA
.l0ECB:
    mov a,y                           ; 0ECB
    bcc .l0ED1                        ; 0ECC
    eor a,#$FF                        ; 0ECE  ; not reached in the traces so far
    inc a                             ; 0ED0  ; not reached in the traces so far
.l0ED1:
    mov.b y,$12                       ; 0ED1
    call Spc_09E0                     ; 0ED3
    mov y,#$14                        ; 0ED6
    mov a,#$00                        ; 0ED8
    subw ya,$10                       ; 0EDA
    movw $10,ya                       ; 0EDC
    inc.b $12                         ; 0EDE
    bbc $12.1,.l0EAB                  ; 0EE0
.l0EE3:
    ret                               ; 0EE3

Spc_0EE4:
    mov.b a,$71+x                     ; 0EE4
    beq .l0F4D                        ; 0EE6
    dec.b $71+x                       ; 0EE8
    beq .l0EF1                        ; 0EEA
    mov a,#$02                        ; 0EEC
    cbne $70+x,.l0F4D                 ; 0EEE
.l0EF1:
    mov.b a,$80+x                     ; 0EF1
    mov.b $17,a                       ; 0EF3
    mov.b a,$30+x                     ; 0EF5
    mov.b y,$31+x                     ; 0EF7
.l0EF9:
    movw $14,ya                       ; 0EF9
    mov y,#$00                        ; 0EFB
.l0EFD:
    mov a,($14)+y                     ; 0EFD
    beq .l0F1F                        ; 0EFF
    bmi .l0F0A                        ; 0F01
.l0F03:
    inc y                             ; 0F03
    bmi .l0F46                        ; 0F04
    mov a,($14)+y                     ; 0F06
    bpl .l0F03                        ; 0F08
.l0F0A:
    cmp a,#$C8                        ; 0F0A
    beq .l0F4D                        ; 0F0C
    cmp a,#$EF                        ; 0F0E
    beq .l0F3B                        ; 0F10
    cmp a,#$E0                        ; 0F12
    bcc .l0F46                        ; 0F14
    push y                            ; 0F16
    mov y,a                           ; 0F17
    pop a                             ; 0F18
    adc.w a,$0FAF+y                   ; 0F19
    mov y,a                           ; 0F1C
    bra .l0EFD                        ; 0F1D
.l0F1F:
    mov.b a,$17                       ; 0F1F
    beq .l0F46                        ; 0F21
    dec.b $17                         ; 0F23
    bne .l0F31                        ; 0F25
    mov.w a,$0231+x                   ; 0F27
    push a                            ; 0F2A
    mov.w a,$0230+x                   ; 0F2B
    pop y                             ; 0F2E
    bra .l0EF9                        ; 0F2F
.l0F31:
    mov.w a,$0241+x                   ; 0F31
    push a                            ; 0F34
    mov.w a,$0240+x                   ; 0F35
    pop y                             ; 0F38
    bra .l0EF9                        ; 0F39
.l0F3B:
    inc y                             ; 0F3B
    mov a,($14)+y                     ; 0F3C
    push a                            ; 0F3E
    inc y                             ; 0F3F
    mov a,($14)+y                     ; 0F40
    mov y,a                           ; 0F42
    pop a                             ; 0F43
    bra .l0EF9                        ; 0F44
.l0F46:
    mov.b a,$47                       ; 0F46
    mov y,#$5C                        ; 0F48
    call Spc_09E0                     ; 0F4A
.l0F4D:
    clr1 $13.7                        ; 0F4D
    mov.b a,$A0+x                     ; 0F4F
    beq .l0F7F                        ; 0F51
    mov.b a,$A1+x                     ; 0F53
    beq .l0F5B                        ; 0F55
    dec.b $A1+x                       ; 0F57
    bra .l0F7F                        ; 0F59
.l0F5B:
    set1 $13.7                        ; 0F5B
    dec.b $A0+x                       ; 0F5D
    bne .l0F6C                        ; 0F5F
    mov.w a,$0381+x                   ; 0F61
    mov.w $0360+x,a                   ; 0F64
    mov.w a,$0380+x                   ; 0F67
    bra .l0F7C                        ; 0F6A
.l0F6C:
    clrc                              ; 0F6C
    mov.w a,$0360+x                   ; 0F6D
    adc.w a,$0370+x                   ; 0F70
    mov.w $0360+x,a                   ; 0F73
    mov.w a,$0361+x                   ; 0F76
    adc.w a,$0371+x                   ; 0F79
.l0F7C:
    mov.w $0361+x,a                   ; 0F7C
.l0F7F:
    call Spc_0DF4                     ; 0F7F
    mov.b a,$B1+x                     ; 0F82
    beq .l0FD2                        ; 0F84
    mov.w a,$02B0+x                   ; 0F86
    cbne $B0+x,.l0FD0                 ; 0F89
    mov.w a,$0100+x                   ; 0F8C
    cmp.w a,$02B1+x                   ; 0F8F
    bne .l0F99                        ; 0F92
    mov.w a,$02C1+x                   ; 0F94
    bra .l0FA6                        ; 0F97
.l0F99:
    setp                              ; 0F99  ; not reached in the traces so far
    inc.b $00+x                       ; 0F9A  ; not reached in the traces so far
    clrp                              ; 0F9C  ; not reached in the traces so far
    mov y,a                           ; 0F9D  ; not reached in the traces so far
    beq .l0FA2                        ; 0F9E  ; not reached in the traces so far
    mov.b a,$B1+x                     ; 0FA0  ; not reached in the traces so far
.l0FA2:
    clrc                              ; 0FA2  ; not reached in the traces so far
    adc.w a,$02C0+x                   ; 0FA3  ; not reached in the traces so far
.l0FA6:
    mov.b $B1+x,a                     ; 0FA6
    mov.w a,$02A0+x                   ; 0FA8
    clrc                              ; 0FAB
    adc.w a,$02A1+x                   ; 0FAC
    mov.w $02A0+x,a                   ; 0FAF
.l0FB2:
    mov.b $12,a                       ; 0FB2
    asl a                             ; 0FB4
    asl a                             ; 0FB5
    bcc .l0FBA                        ; 0FB6
    eor a,#$FF                        ; 0FB8
.l0FBA:
    mov y,a                           ; 0FBA
    mov.b a,$B1+x                     ; 0FBB
    cmp a,#$F1                        ; 0FBD
    bcc .l0FC6                        ; 0FBF
    and a,#$0F                        ; 0FC1  ; not reached in the traces so far
    mul ya                            ; 0FC3  ; not reached in the traces so far
    bra .l0FCA                        ; 0FC4  ; not reached in the traces so far
.l0FC6:
    mul ya                            ; 0FC6
    mov a,y                           ; 0FC7
    mov y,#$00                        ; 0FC8
.l0FCA:
    call Spc_1047                     ; 0FCA
.l0FCD:
    jmp $095D                         ; 0FCD
.l0FD0:
    inc.b $B0+x                       ; 0FD0
.l0FD2:
    bbs $13.7,.l0FCD                  ; 0FD2
    ret                               ; 0FD5

Spc_0FD6:
    clr1 $13.7                        ; 0FD6
    mov.b a,$C1+x                     ; 0FD8
    beq .l0FE5                        ; 0FDA
    mov.w a,$02E0+x                   ; 0FDC  ; not reached in the traces so far
    cbne $C0+x,.l0FE5                 ; 0FDF  ; not reached in the traces so far
    call $104F                        ; 0FE2  ; not reached in the traces so far
.l0FE5:
    mov.w a,$0331+x                   ; 0FE5
    mov y,a                           ; 0FE8
    mov.w a,$0330+x                   ; 0FE9
    movw $10,ya                       ; 0FEC
    mov.b a,$91+x                     ; 0FEE
    beq .l0FFC                        ; 0FF0
    mov.w a,$0341+x                   ; 0FF2
    mov y,a                           ; 0FF5
    mov.w a,$0340+x                   ; 0FF6
    call Spc_1031                     ; 0FF9
.l0FFC:
    bbc $13.7,.l1002                  ; 0FFC
    call Spc_0EA6                     ; 0FFF
.l1002:
    clr1 $13.7                        ; 1002
    call Spc_0DF4                     ; 1004
    mov.b a,$A0+x                     ; 1007
    beq .l1019                        ; 1009
    mov.b a,$A1+x                     ; 100B
    bne .l1019                        ; 100D
    mov.w a,$0371+x                   ; 100F
    mov y,a                           ; 1012
    mov.w a,$0370+x                   ; 1013
    call Spc_1031                     ; 1016
.l1019:
    mov.b a,$B1+x                     ; 1019
    beq Spc_0EE4_l0FD2                ; 101B
    mov.w a,$02B0+x                   ; 101D
    cbne $B0+x,Spc_0EE4_l0FD2         ; 1020
    mov.b y,$51                       ; 1023
    mov.w a,$02A1+x                   ; 1025
    mul ya                            ; 1028
    mov a,y                           ; 1029
    clrc                              ; 102A
    adc.w a,$02A0+x                   ; 102B
    jmp $0FB2                         ; 102E

Spc_1031:
    set1 $13.7                        ; 1031
    mov.b $12,y                       ; 1033
    call Spc_0E11                     ; 1035
    push y                            ; 1038
    mov.b y,$51                       ; 1039
    mul ya                            ; 103B
    mov.b $14,y                       ; 103C
    mov $15,#$00                      ; 103E
    mov.b y,$51                       ; 1041
    pop a                             ; 1043
    mul ya                            ; 1044
    addw ya,$14                       ; 1045

Spc_1047:
    call Spc_0E11                     ; 1047
    addw ya,$10                       ; 104A
    movw $10,ya                       ; 104C
    ret                               ; 104E
.l104F:
    set1 $13.7                        ; 104F  ; not reached in the traces so far
    mov.b y,$51                       ; 1051  ; not reached in the traces so far
    mov.w a,$02D1+x                   ; 1053  ; not reached in the traces so far
    mul ya                            ; 1056  ; not reached in the traces so far
    mov a,y                           ; 1057  ; not reached in the traces so far
    clrc                              ; 1058  ; not reached in the traces so far
    adc.w a,$02D0+x                   ; 1059  ; not reached in the traces so far
.l105C:
    asl a                             ; 105C  ; not reached in the traces so far
    bcc .l1061                        ; 105D  ; not reached in the traces so far
    eor a,#$FF                        ; 105F  ; not reached in the traces so far
.l1061:
    mov.b y,$C1+x                     ; 1061  ; not reached in the traces so far
    mul ya                            ; 1063  ; not reached in the traces so far
    mov a,y                           ; 1064  ; not reached in the traces so far
    eor a,#$FF                        ; 1065  ; not reached in the traces so far

Spc_1067:
    mov.w y,$0565                     ; 1067
    mul ya                            ; 106A
    mov.w a,$0210+x                   ; 106B
    mul ya                            ; 106E
    mov.w a,$0301+x                   ; 106F
    mul ya                            ; 1072
    mov a,y                           ; 1073
    mul ya                            ; 1074
    mov a,y                           ; 1075
    mov.w $0321+x,a                   ; 1076
    ret                               ; 1079
    db $00, $01, $03, $07, $0D, $15, $1E, $29
    db $34, $42, $51, $5E, $67, $6E, $73, $77
    db $7A, $7C, $7D, $7E, $7F, $01, $01, $02
    db $03, $00, $01, $02, $01, $02, $01, $01
    db $03, $00, $01, $02, $03, $01, $03, $03
    db $00, $01, $03, $00, $03, $03, $03, $01

%spc($10B0, $19CD3A)

Spc_10B0:
    mov.w a,$0558                     ; 10B0
    cmp.b a,$04                       ; 10B3
    beq .l10C0                        ; 10B5
    mov.w a,$0558                     ; 10B7
    mov.b $04,a                       ; 10BA
    mov.b $00,a                       ; 10BC
    bra .l10C3                        ; 10BE
.l10C0:
    mov $00,#$00                      ; 10C0
.l10C3:
    mov.w a,$0559                     ; 10C3
    mov.b $05,a                       ; 10C6
    mov.b $01,a                       ; 10C8
    mov.w a,$055A                     ; 10CA
    mov.b $06,a                       ; 10CD
    mov.b $02,a                       ; 10CF
    mov.w a,$055B                     ; 10D1
    mov.b $07,a                       ; 10D4
    mov.b $03,a                       ; 10D6
    bbc $29.1,.l10DF                  ; 10D8
    clr1 $29.0                        ; 10DB
    clr1 $29.1                        ; 10DD
.l10DF:
    call Spc_111B                     ; 10DF
    call Spc_15C4                     ; 10E2
    call Spc_1AB0                     ; 10E5
    call Spc_17A5                     ; 10E8
    call Spc_112F                     ; 10EB
    call Spc_1B7A                     ; 10EE
    call Spc_1B2C                     ; 10F1
    call Spc_1523                     ; 10F4
    call Spc_117C                     ; 10F7
    call Spc_11F8                     ; 10FA
    call Spc_1B92                     ; 10FD
    call Spc_16F1                     ; 1100
    call Spc_174B                     ; 1103
    call Spc_16C1                     ; 1106
    call Spc_13FA                     ; 1109
    call Spc_1436                     ; 110C
    call Spc_CAC0                     ; 110F
    call Spc_16B1                     ; 1112

Spc_1115:
    call Spc_17DF                     ; 1115
    jmp $1C07                         ; 1118

Spc_111B:
    bbc $2D.7,Spc_111E_l112E          ; 111B

Spc_111E:
    cmp $01,#$09                      ; 111E
    bne .l1126                        ; 1121
    mov $01,#$12                      ; 1123
.l1126:
    cmp $01,#$0A                      ; 1126
    bne .l112E                        ; 1129
    mov $01,#$0C                      ; 112B
.l112E:
    ret                               ; 112E

Spc_112F:
    bbc $2D.6,Spc_111E_l112E          ; 112F
    mov.w a,$0395                     ; 1132
    cmp a,#$43                        ; 1135
    beq Spc_111E_l112E                ; 1137
    mov x,#$05                        ; 1139
    jmp Spc_194C                      ; 113B
.l113E:
    bbc $2D.5,Spc_1163_l116F          ; 113E
    mov.w a,$0397                     ; 1141
    cmp a,#$01                        ; 1144
    beq Spc_1163_l1170                ; 1146
    cmp a,#$04                        ; 1148
    beq Spc_1163_l1170                ; 114A
    cmp a,#$07                        ; 114C
    beq Spc_1163_l1170                ; 114E
    cmp a,#$0A                        ; 1150
    beq Spc_1163_l1170                ; 1152
    cmp a,#$0D                        ; 1154
    beq Spc_1163_l1170                ; 1156
.l1158:
    mov.w a,$0396                     ; 1158
    cmp a,#$72                        ; 115B
    beq Spc_1163_l1177                ; 115D
    cmp a,#$73                        ; 115F
    beq Spc_1163_l1177                ; 1161

Spc_1163:
    cmp a,#$74                        ; 1163
    beq .l1177                        ; 1165
    cmp a,#$75                        ; 1167
    beq .l1177                        ; 1169
    cmp a,#$76                        ; 116B
    beq .l1177                        ; 116D
.l116F:
    ret                               ; 116F
.l1170:
    mov x,#$07                        ; 1170  ; not reached in the traces so far
    call Spc_194C                     ; 1172  ; not reached in the traces so far
    bra Spc_112F_l1158                ; 1175  ; not reached in the traces so far
.l1177:
    mov x,#$06                        ; 1177  ; not reached in the traces so far
    jmp Spc_194C                      ; 1179  ; not reached in the traces so far

Spc_117C:
    inc.w $0581                       ; 117C
    mov.w a,$0581                     ; 117F
    cmp a,#$03                        ; 1182
    beq .l11AC                        ; 1184
    cmp a,#$09                        ; 1186
    beq Spc_112F_l113E                ; 1188
    cmp a,#$0C                        ; 118A
    beq .l11BC                        ; 118C
    cmp a,#$0F                        ; 118E
    beq .l11CA                        ; 1190
    cmp a,#$12                        ; 1192
    beq .l11E1                        ; 1194
    cmp a,#$15                        ; 1196
    beq .l11EA                        ; 1198
    sbc a,#$16                        ; 119A
    bmi .l11A3                        ; 119C
    mov a,#$00                        ; 119E
    mov.w $0581,a                     ; 11A0
.l11A3:
    cmp $1C,#$01                      ; 11A3
    beq .l11AB                        ; 11A6
    mov $2A,#$00                      ; 11A8
.l11AB:
    ret                               ; 11AB
.l11AC:
    bbc $2D.3,.l11A3                  ; 11AC
    mov.w a,$0394                     ; 11AF
    cmp a,#$02                        ; 11B2
    bne .l11A3                        ; 11B4
    mov $20,#$08                      ; 11B6
    jmp Spc_1908                      ; 11B9
.l11BC:
    bbs $2A.0,.l11C5                  ; 11BC
    mov $20,#$0C                      ; 11BF
    jmp Spc_192B                      ; 11C2
.l11C5:
    mov a,#$17                        ; 11C5
    jmp Spc_1424                      ; 11C7
.l11CA:
    bbs $2A.2,.l11D3                  ; 11CA
    mov $20,#$0A                      ; 11CD
    jmp Spc_1915                      ; 11D0
.l11D3:
    bbs $2A.0,.l11A3                  ; 11D3
    bbs $2A.7,.l11A3                  ; 11D6
    bbs $2A.4,.l11A3                  ; 11D9
    mov a,#$25                        ; 11DC
    jmp Spc_1424                      ; 11DE
.l11E1:
    bbs $2A.4,.l11A3                  ; 11E1
    mov $20,#$0B                      ; 11E4
    jmp Spc_1920                      ; 11E7
.l11EA:
    bbs $2A.7,Spc_11ED_l11F3          ; 11EA

Spc_11ED:
    mov $20,#$0E                      ; 11ED
    jmp Spc_1936                      ; 11F0
.l11F3:
    mov a,#$21                        ; 11F3
    jmp Spc_1424                      ; 11F5

Spc_11F8:
    mov.w a,$EE40                     ; 11F8
    cmp a,#$98                        ; 11FB
    bne .l1204                        ; 11FD
    bbc $2D.4,.l1204                  ; 11FF
    set1 $2D.7                        ; 1202
.l1204:
    mov.w a,$CB10                     ; 1204
    cmp a,#$14                        ; 1207
    beq .l1211                        ; 1209
    mov a,#$04                        ; 120B
    clr1 $2D.4                        ; 120D
    bra .l1215                        ; 120F
.l1211:
    mov a,#$1E                        ; 1211
    set1 $2D.4                        ; 1213
.l1215:
    mov.w $1C05,a                     ; 1215
    mov.w a,$056A                     ; 1218
    cmp a,#$FF                        ; 121B
    beq .l1222                        ; 121D
    inc.w $056A                       ; 121F
.l1222:
    mov.b a,$1B                       ; 1222
    cmp a,#$05                        ; 1224
    beq .l122A                        ; 1226
    bra .l1232                        ; 1228
.l122A:
    mov a,#$00                        ; 122A
    mov.w $056A,a                     ; 122C
    mov.w $056B,a                     ; 122F
.l1232:
    mov.w a,$056A                     ; 1232
    bmi .l1239                        ; 1235
    bra .l1243                        ; 1237
.l1239:
    mov.w a,$056B                     ; 1239
    cmp a,#$FF                        ; 123C
    beq .l1243                        ; 123E
    inc.w $056B                       ; 1240
.l1243:
    cmp $1D,#$35                      ; 1243
    bne .l124D                        ; 1246
    mov a,#$FF                        ; 1248  ; not reached in the traces so far
    mov.w $056B,a                     ; 124A  ; not reached in the traces so far
.l124D:
    cmp $1B,#$02                      ; 124D
    bne .l1257                        ; 1250
    mov a,#$01                        ; 1252
    mov.w $0567,a                     ; 1254
.l1257:
    mov.w a,$0567                     ; 1257
    beq .l126E                        ; 125A
    inc.w $0567                       ; 125C
    mov.w a,$0567                     ; 125F
    cmp a,#$E6                        ; 1262
    bne .l126E                        ; 1264
    mov a,#$00                        ; 1266
    mov.w $0567,a                     ; 1268
    mov $1B,#$03                      ; 126B
.l126E:
    mov.b a,$53                       ; 126E
    cmp a,#$1B                        ; 1270
    beq .l1288                        ; 1272
    cmp a,#$1D                        ; 1274
    beq .l128C                        ; 1276
    cmp a,#$1E                        ; 1278
    beq .l1290                        ; 127A
    cmp a,#$29                        ; 127C
    beq .l1294                        ; 127E
    cmp a,#$2A                        ; 1280
    beq .l1298                        ; 1282
    mov a,#$8E                        ; 1284
    bra .l129C                        ; 1286
.l1288:
    mov a,#$73                        ; 1288
    bra .l129C                        ; 128A
.l128C:
    mov a,#$6A                        ; 128C
    bra .l129C                        ; 128E
.l1290:
    mov a,#$67                        ; 1290
    bra .l129C                        ; 1292
.l1294:
    mov a,#$92                        ; 1294
    bra .l129C                        ; 1296
.l1298:
    mov a,#$93                        ; 1298
    bra .l129C                        ; 129A
.l129C:
    mov.w $0586,a                     ; 129C
    mov.w a,$0397                     ; 129F
    cmp a,#$0D                        ; 12A2
    beq Spc_12FA_l1315                ; 12A4
    mov.w a,$055F                     ; 12A6
    bne .l12B7                        ; 12A9
    cmp $01,#$1D                      ; 12AB
    bne Spc_12FA_l1315                ; 12AE
    mov a,#$01                        ; 12B0
    mov.w $055F,a                     ; 12B2
    bra Spc_12FA_l1315                ; 12B5
.l12B7:
    mov.w a,$0582                     ; 12B7
    beq .l12CA                        ; 12BA
    dec.w $0582                       ; 12BC
    mov.w a,$0582                     ; 12BF
    cmp a,#$04                        ; 12C2
    beq Spc_12FA                      ; 12C4
    cmp a,#$01                        ; 12C6
    beq Spc_12FA_l130B                ; 12C8
.l12CA:
    cmp $1D,#$30                      ; 12CA
    bne Spc_12FA_l1315                ; 12CD
    inc.w $055F                       ; 12CF

Spc_12D2:
    mov.w a,$055F                     ; 12D2
    cmp a,#$03                        ; 12D5
    bne Spc_12FA_l1315                ; 12D7
    mov $40,#$48                      ; 12D9
    mov $41,#$EE                      ; 12DC
    set1 $2C.0                        ; 12DF
    mov.w a,$0586                     ; 12E1
    mov.w $0582,a                     ; 12E4
    mov x,#$00                        ; 12E7
    call Spc_1D63                     ; 12E9
    inc x                             ; 12EC
    call Spc_1D63                     ; 12ED
    mov.b a,$59                       ; 12F0
    mov.w $0587,a                     ; 12F2
    mov $59,#$BE                      ; 12F5
    bra Spc_12FA_l1315                ; 12F8

Spc_12FA:
    mov.w a,$0587                     ; 12FA
    mov.b $59,a                       ; 12FD
    mov x,#$00                        ; 12FF
    call Spc_1E31                     ; 1301
    mov x,#$01                        ; 1304
    call Spc_1E31                     ; 1306
    bra .l1315                        ; 1309
.l130B:
    inc.b $50                         ; 130B
    mov $40,#$48                      ; 130D
    mov $41,#$EE                      ; 1310
    set1 $2C.0                        ; 1313
.l1315:
    cmp $01,#$1D                      ; 1315
    bne .l1324                        ; 1318
    mov.w a,$0397                     ; 131A
    cmp a,#$0D                        ; 131D
    bne .l1324                        ; 131F
    mov $01,#$00                      ; 1321
.l1324:
    cmp $1B,#$01                      ; 1324
    bne .l133A                        ; 1327
    mov.w a,$055F                     ; 1329
    beq .l133A                        ; 132C
    mov.w a,$0397                     ; 132E
    cmp a,#$0D                        ; 1331
    beq .l133A                        ; 1333
    mov a,#$02                        ; 1335
    call Spc_1424                     ; 1337
.l133A:
    mov.w a,$056A                     ; 133A
    cmp a,#$F0                        ; 133D
    bne .l1357                        ; 133F
    mov.w a,$055F                     ; 1341
    beq .l1357                        ; 1344
    mov.w a,$0397                     ; 1346
    cmp a,#$0D                        ; 1349
    beq .l1357                        ; 134B
    cmp $1C,#$05                      ; 134D
    bne .l1357                        ; 1350
    mov a,#$24                        ; 1352
    call Spc_1424                     ; 1354
.l1357:
    cmp $1D,#$39                      ; 1357
    bne .l1376                        ; 135A
    mov $2A,#$00                      ; 135C  ; not reached in the traces so far
    set1 $2D.3                        ; 135F  ; not reached in the traces so far
    set1 $2D.6                        ; 1361  ; not reached in the traces so far
    mov a,#$01                        ; 1363  ; not reached in the traces so far
    mov.w $0579,a                     ; 1365  ; not reached in the traces so far
    mov x,#$07                        ; 1368  ; not reached in the traces so far
    call Spc_1A92                     ; 136A  ; not reached in the traces so far
    dec x                             ; 136D  ; not reached in the traces so far
    call Spc_1A92                     ; 136E  ; not reached in the traces so far
    mov x,#$08                        ; 1371  ; not reached in the traces so far
    call Spc_1908                     ; 1373  ; not reached in the traces so far
.l1376:
    cmp $1D,#$32                      ; 1376
    bne .l139C                        ; 1379
    set1 $2D.5                        ; 137B
    mov x,#$04                        ; 137D
    call Spc_194C                     ; 137F
    mov a,#$00                        ; 1382
    mov.b $2A,a                       ; 1384
    mov.b $1C,a                       ; 1386
    call Spc_0A60                     ; 1388
    set1 $2D.3                        ; 138B
    set1 $2D.6                        ; 138D
    mov x,#$02                        ; 138F
    call Spc_194C                     ; 1391
    dec x                             ; 1394
    call Spc_194C                     ; 1395
    dec x                             ; 1398
    call Spc_194C                     ; 1399
.l139C:
    cmp $1D,#$03                      ; 139C
    bne .l13B3                        ; 139F
    set1 $2D.5                        ; 13A1
    mov x,#$05                        ; 13A3
    call Spc_194C                     ; 13A5
    dec x                             ; 13A8
    call Spc_194C                     ; 13A9
    set1 $2D.3                        ; 13AC
    mov $2A,#$00                      ; 13AE
    set1 $2D.6                        ; 13B1
.l13B3:
    mov.w a,$0579                     ; 13B3
    beq .l13CA                        ; 13B6
    inc.w $0579                       ; 13B8
    mov.w a,$0579                     ; 13BB
    setc                              ; 13BE
    sbc a,#$78                        ; 13BF
    bmi .l13CA                        ; 13C1
    set1 $2D.5                        ; 13C3
    mov a,#$00                        ; 13C5
    mov.w $0579,a                     ; 13C7
.l13CA:
    mov.w a,$0395                     ; 13CA
    cmp a,#$17                        ; 13CD
    beq .l13D7                        ; 13CF
    cmp a,#$21                        ; 13D1
    beq .l13D7                        ; 13D3
    bra .l13E6                        ; 13D5
.l13D7:
    cmp $01,#$09                      ; 13D7
    beq .l13E3                        ; 13DA
    cmp $01,#$0D                      ; 13DC
    beq .l13E3                        ; 13DF
    bra .l13E6                        ; 13E1
.l13E3:
    mov $01,#$00                      ; 13E3
.l13E6:
    mov.w a,$0397                     ; 13E6
    cmp a,#$32                        ; 13E9
    bne .l13F9                        ; 13EB
    mov.w a,$0394                     ; 13ED
    cmp a,#$38                        ; 13F0
    bne .l13F9                        ; 13F2
    mov x,#$04                        ; 13F4  ; not reached in the traces so far
    call Spc_194C                     ; 13F6  ; not reached in the traces so far
.l13F9:
    ret                               ; 13F9

Spc_13FA:
    cmp $1D,#$4C                      ; 13FA
    bne .l1423                        ; 13FD
    mov x,#$07                        ; 13FF
    call Spc_1A92                     ; 1401
    dec x                             ; 1404
    call Spc_1A92                     ; 1405
    dec x                             ; 1408
    call Spc_194C                     ; 1409
    dec x                             ; 140C
    call Spc_194C                     ; 140D
    dec x                             ; 1410
    call Spc_194C                     ; 1411
    mov a,#$01                        ; 1414
    mov.w $0579,a                     ; 1416
    set1 $2D.3                        ; 1419
    mov $2A,#$00                      ; 141B
    call Spc_0A60                     ; 141E
    set1 $2D.6                        ; 1421
.l1423:
    ret                               ; 1423

Spc_1424:
    push x                            ; 1424
    push a                            ; 1425
    mov x,#$00                        ; 1426
.l1428:
    mov.w a,$0588+x                   ; 1428
    beq .l1430                        ; 142B
    inc x                             ; 142D
    bra .l1428                        ; 142E
.l1430:
    pop a                             ; 1430
    mov.w $0588+x,a                   ; 1431
    pop x                             ; 1434
.l1435:
    ret                               ; 1435

Spc_1436:
    bbs $29.0,Spc_1424_l1435          ; 1436
    cmp $1D,#$3E                      ; 1439
    bne .l1443                        ; 143C
    mov a,#$01                        ; 143E
    mov.w $0568,a                     ; 1440
.l1443:
    mov.w a,$0568                     ; 1443
    beq .l144B                        ; 1446
    inc.w $0568                       ; 1448
.l144B:
    mov.w a,$0568                     ; 144B
    cmp a,#$2A                        ; 144E
    bne Spc_1467_l1475                ; 1450
    mov.w a,$0397                     ; 1452
    cmp a,#$01                        ; 1455
    beq Spc_1467_l146B                ; 1457
    cmp a,#$04                        ; 1459
    beq Spc_1467                      ; 145B
    cmp a,#$07                        ; 145D
    beq Spc_1467_l146B                ; 145F
    cmp a,#$0A                        ; 1461
    beq Spc_1467                      ; 1463
    bra Spc_1467_l1470                ; 1465

Spc_1467:
    mov a,#$0F                        ; 1467
    bra .l146D                        ; 1469
.l146B:
    mov a,#$2F                        ; 146B
.l146D:
    call Spc_1424                     ; 146D
.l1470:
    mov a,#$00                        ; 1470
    mov.w $0568,a                     ; 1472
.l1475:
    cmp $1D,#$20                      ; 1475
    beq .l1481                        ; 1478
    cmp $1D,#$16                      ; 147A
    beq .l1485                        ; 147D
    bra .l14A3                        ; 147F
.l1481:
    set1 $2C.4                        ; 1481
    bra .l1487                        ; 1483
.l1485:
    set1 $2C.5                        ; 1485
.l1487:
    mov.w a,$0397                     ; 1487
    cmp a,#$01                        ; 148A
    beq .l149B                        ; 148C
    cmp a,#$04                        ; 148E
    beq .l1496                        ; 1490
    cmp a,#$07                        ; 1492
    beq .l149B                        ; 1494
.l1496:
    mov $1D,#$1B                      ; 1496
    bra .l149E                        ; 1499
.l149B:
    mov $1D,#$27                      ; 149B
.l149E:
    mov a,#$01                        ; 149E
    mov.w $055D,a                     ; 14A0
.l14A3:
    mov.w a,$055D                     ; 14A3

Spc_14A6:
    beq .l14AB                        ; 14A6
    inc.w $055D                       ; 14A8
.l14AB:
    mov.w a,$055D                     ; 14AB
    cmp a,#$11                        ; 14AE
    bne .l14CF                        ; 14B0
    mov a,#$00                        ; 14B2
    mov.w $055D,a                     ; 14B4
    bbs $2C.4,.l14BF                  ; 14B7
    bbs $2C.5,.l14C8                  ; 14BA
    bra .l14CF                        ; 14BD  ; not reached in the traces so far
.l14BF:
    clr1 $2C.4                        ; 14BF
    mov a,#$20                        ; 14C1
    call Spc_1424                     ; 14C3
    bra .l14CF                        ; 14C6
.l14C8:
    clr1 $2C.5                        ; 14C8
    mov a,#$16                        ; 14CA
    call Spc_1424                     ; 14CC
.l14CF:
    cmp $1D,#$37                      ; 14CF
    bne .l14D7                        ; 14D2
    inc.w $0580                       ; 14D4
.l14D7:
    cmp $1D,#$30                      ; 14D7
    bne Spc_14EA_l14ED                ; 14DA
    mov.w a,$0580                     ; 14DC
    cmp a,#$0A                        ; 14DF
    bne .l14E8                        ; 14E1
    mov a,#$43                        ; 14E3  ; not reached in the traces so far
    call Spc_1424                     ; 14E5  ; not reached in the traces so far
.l14E8:
    mov a,#$00                        ; 14E8

Spc_14EA:
    mov.w $0580,a                     ; 14EA
.l14ED:
    mov.w a,$0560                     ; 14ED
    bne .l150A                        ; 14F0
    mov.b a,$1D                       ; 14F2
    cmp a,#$29                        ; 14F4
    beq .l14FA                        ; 14F6
    bra .l1522                        ; 14F8
.l14FA:
    mov a,#$01                        ; 14FA  ; not reached in the traces so far
    mov.w $0560,a                     ; 14FC  ; not reached in the traces so far
    mov.b a,$18                       ; 14FF  ; not reached in the traces so far
    and a,#$3F                        ; 1501  ; not reached in the traces so far
    adc a,#$1E                        ; 1503  ; not reached in the traces so far
    mov.w $0561,a                     ; 1505  ; not reached in the traces so far
    bra .l1522                        ; 1508  ; not reached in the traces so far
.l150A:
    inc.w $0560                       ; 150A  ; not reached in the traces so far
    cmp.w a,$0561                     ; 150D  ; not reached in the traces so far
    bne .l1522                        ; 1510  ; not reached in the traces so far
    mov a,#$29                        ; 1512  ; not reached in the traces so far
    call Spc_1424                     ; 1514  ; not reached in the traces so far
    mov $1D,#$29                      ; 1517  ; not reached in the traces so far
    mov a,#$00                        ; 151A  ; not reached in the traces so far
    mov.w $0560,a                     ; 151C  ; not reached in the traces so far
    mov.w $0561,a                     ; 151F  ; not reached in the traces so far
.l1522:
    ret                               ; 1522

Spc_1523:
    call Spc_15A1                     ; 1523
    bbs $2C.2,Spc_152C_l152D          ; 1526
    bbs $2C.3,Spc_152C_l1538          ; 1529

Spc_152C:
    ret                               ; 152C
.l152D:
    mov.w a,$0555                     ; 152D
    beq .l1554                        ; 1530
    cmp a,#$FE                        ; 1532
    beq Spc_152C                      ; 1534
    bra .l156C                        ; 1536
.l1538:
    clr1 $2C.2                        ; 1538  ; not reached in the traces so far
    clr1 $2C.3                        ; 153A  ; not reached in the traces so far
    mov a,#$00                        ; 153C  ; not reached in the traces so far
    mov.w $0555,a                     ; 153E  ; not reached in the traces so far
    mov.w $0553,a                     ; 1541  ; not reached in the traces so far
    mov.w $0554,a                     ; 1544  ; not reached in the traces so far
    mov.w $0569,a                     ; 1547  ; not reached in the traces so far
    mov.w a,$0578                     ; 154A  ; not reached in the traces so far
    mov.b $50,a                       ; 154D  ; not reached in the traces so far
    mov.w a,$0566                     ; 154F  ; not reached in the traces so far
    bra Spc_1584_l159E                ; 1552  ; not reached in the traces so far
.l1554:
    mov a,#$04                        ; 1554
    mov.w $0554,a                     ; 1556
    mov.b a,$53                       ; 1559
    mov.w $0566,a                     ; 155B
    mov.b a,$50                       ; 155E
    mov.w $0578,a                     ; 1560
    mov y,#$7F                        ; 1563
    mul ya                            ; 1565
    mov x,#$FE                        ; 1566
    div ya,x                          ; 1568
    mov.w $0569,a                     ; 1569
.l156C:
    inc.w $0555                       ; 156C
    mov.w a,$0555                     ; 156F
    mov y,#$00                        ; 1572
    mov x,#$15                        ; 1574
    div ya,x                          ; 1576
    mov.b $20,a                       ; 1577
    setc                              ; 1579
    mov.w a,$0578                     ; 157A
    sbc.b a,$20                       ; 157D
    mov.b $50,a                       ; 157F
    mov.w a,$0569                     ; 1581

Spc_1584:
    mov.b $20,a                       ; 1584
    mov $21,#$00                      ; 1586
    mov.w a,$0553                     ; 1589
    mov.w y,$0554                     ; 158C
    addw ya,$20                       ; 158F
    mov.w $0553,a                     ; 1591
    mov.w $0554,y                     ; 1594
    mov.w a,$0566                     ; 1597
    setc                              ; 159A
    sbc.w a,$0554                     ; 159B
.l159E:
    mov.b $53,a                       ; 159E
    ret                               ; 15A0

Spc_15A1:
    mov.b a,$1D                       ; 15A1
    cmp a,#$2C                        ; 15A3
    beq .l15B5                        ; 15A5
    mov.w x,$0555                     ; 15A7
    beq Spc_15BE                      ; 15AA
    cmp a,#$30                        ; 15AC
    beq Spc_15BE_l15BF                ; 15AE
    cmp a,#$3C                        ; 15B0
    beq Spc_15BE_l15BF                ; 15B2
    ret                               ; 15B4
.l15B5:
    cmp $1C,#$01                      ; 15B5
    bne Spc_15BE                      ; 15B8
    set1 $2C.2                        ; 15BA
    clr1 $2C.3                        ; 15BC

Spc_15BE:
    ret                               ; 15BE
.l15BF:
    clr1 $2C.2                        ; 15BF  ; not reached in the traces so far
    set1 $2C.3                        ; 15C1  ; not reached in the traces so far
    ret                               ; 15C3  ; not reached in the traces so far

Spc_15C4:
    bbs $29.0,.l15E3                  ; 15C4
    mov.w a,$0550                     ; 15C7
    bne Spc_160B_l1618                ; 15CA
    mov.w a,$0551                     ; 15CC
    cmp a,#$01                        ; 15CF
    beq .l15E4                        ; 15D1
    cmp a,#$03                        ; 15D3
    beq .l15FE                        ; 15D5
    bbs $28.0,.l15DE                  ; 15D7
    bbs $28.1,.l15DE                  ; 15DA
    ret                               ; 15DD
.l15DE:
    mov a,#$01                        ; 15DE
    mov.w $0551,a                     ; 15E0
.l15E3:
    ret                               ; 15E3
.l15E4:
    mov.b a,$4A                       ; 15E4
    mov.w $0570,a                     ; 15E6
    mov.b a,$4E                       ; 15E9
    mov.w $0571,a                     ; 15EB
    mov.b a,$61                       ; 15EE
    mov.w $0572,a                     ; 15F0
    mov.b a,$63                       ; 15F3
    mov.w $0573,a                     ; 15F5
    mov a,#$01                        ; 15F8
    mov.w $0550,a                     ; 15FA
    ret                               ; 15FD
.l15FE:
    mov.w a,$0570                     ; 15FE
    mov.b $4A,a                       ; 1601
    mov.w a,$0571                     ; 1603
    mov.b $4E,a                       ; 1606
    mov.w a,$0572                     ; 1608

Spc_160B:
    mov.b $61,a                       ; 160B
    mov.w a,$0573                     ; 160D
    mov.b $63,a                       ; 1610
    mov a,#$00                        ; 1612
    mov.w $0551,a                     ; 1614
    ret                               ; 1617
.l1618:
    mov $4A,#$00                      ; 1618
    set1 $4A.7                        ; 161B
    set1 $4A.6                        ; 161D
    call Spc_1624                     ; 161F
    bra Spc_1637_l167C                ; 1622

Spc_1624:
    mov.w a,$0551                     ; 1624
    cmp a,#$01                        ; 1627
    beq Spc_1637_l163E                ; 1629
    cmp a,#$02                        ; 162B
    beq Spc_1637_l1656                ; 162D
    dec.w $0550                       ; 162F
    mov.w a,$0550                     ; 1632
    cmp a,#$3C                        ; 1635

Spc_1637:
    beq .l163A                        ; 1637
    ret                               ; 1639
.l163A:
    bbs $28.1,.l1650                  ; 163A  ; not reached in the traces so far
    ret                               ; 163D  ; not reached in the traces so far
.l163E:
    inc.w $0550                       ; 163E
    mov.w a,$0550                     ; 1641
    cmp a,#$1E                        ; 1644
    beq .l164D                        ; 1646
    cmp a,#$3C                        ; 1648
    beq .l1650                        ; 164A
    ret                               ; 164C
.l164D:
    bbs $28.0,.l1655                  ; 164D
.l1650:
    mov a,#$02                        ; 1650
    mov.w $0551,a                     ; 1652
.l1655:
    ret                               ; 1655
.l1656:
    bbs $28.0,.l1662                  ; 1656
    bbs $28.1,.l166F                  ; 1659
    mov a,#$03                        ; 165C
    mov.w $0551,a                     ; 165E
    ret                               ; 1661
.l1662:
    mov.w a,$0550                     ; 1662
    cmp a,#$3C                        ; 1665
    beq .l1655                        ; 1667
    mov a,#$01                        ; 1669  ; not reached in the traces so far
    mov.w $0551,a                     ; 166B  ; not reached in the traces so far
    ret                               ; 166E  ; not reached in the traces so far
.l166F:
    mov.w a,$0550                     ; 166F
    cmp a,#$1E                        ; 1672
    beq .l1655                        ; 1674
    mov a,#$03                        ; 1676  ; not reached in the traces so far
    mov.w $0551,a                     ; 1678  ; not reached in the traces so far
    ret                               ; 167B  ; not reached in the traces so far
.l167C:
    mov.w a,$0550                     ; 167C
    mov y,#$B0                        ; 167F
    mul ya                            ; 1681
    mov a,y                           ; 1682
    eor a,#$FF                        ; 1683
    mov.w $0552,a                     ; 1685
    mov a,#$64                        ; 1688
    setc                              ; 168A
    sbc.w a,$0571                     ; 168B
    mov.w y,$0550                     ; 168E
    mul ya                            ; 1691
    mov x,#$3C                        ; 1692
    div ya,x                          ; 1694
    clrc                              ; 1695
    adc.w a,$0571                     ; 1696
    mov.b $4E,a                       ; 1699
    mov a,#$64                        ; 169B
    setc                              ; 169D
    sbc.w a,$0572                     ; 169E
    mov.w y,$0550                     ; 16A1
    mul ya                            ; 16A4
    mov x,#$3C                        ; 16A5
    div ya,x                          ; 16A7
    clrc                              ; 16A8
    adc.w a,$0572                     ; 16A9
    mov.b $61,a                       ; 16AC
    mov.b $63,a                       ; 16AE
    ret                               ; 16B0

Spc_16B1:
    cmp $1C,#$01                      ; 16B1
    beq .l16C0                        ; 16B4
    mov.b a,$01                       ; 16B6
    setc                              ; 16B8
    sbc a,#$07                        ; 16B9
    bmi .l16C0                        ; 16BB
    mov $01,#$00                      ; 16BD
.l16C0:
    ret                               ; 16C0

Spc_16C1:
    cmp $1C,#$01                      ; 16C1
    beq .l16D7                        ; 16C4
    mov x,#$00                        ; 16C6
.l16C8:
    mov.w a,$16D8+x                   ; 16C8
    beq .l16D4                        ; 16CB
    cmp.b a,$1D                       ; 16CD
    beq .l16D7                        ; 16CF
    inc x                             ; 16D1
    bra .l16C8                        ; 16D2
.l16D4:
    mov $1D,#$00                      ; 16D4
.l16D7:
    ret                               ; 16D7
    db $03, $58, $32, $35, $2B, $34, $43, $44
    db $45, $01, $04, $07, $0A, $0D, $23, $3A
    db $3B, $3F, $06, $22, $41, $40, $1F, $2A
    db $00

Spc_16F1:
    bbc $28.2,.l16F7                  ; 16F1
    call Spc_16FE                     ; 16F4
.l16F7:
    bbc $28.3,.l16FD                  ; 16F7
    call Spc_171F                     ; 16FA
.l16FD:
    ret                               ; 16FD

Spc_16FE:
    mov.w a,$0547                     ; 16FE
    cmp a,#$14                        ; 1701
    beq Spc_171B_l171E                ; 1703
    inc.w $0547                       ; 1705
    mov.w a,$054D                     ; 1708
    mov.b $20,a                       ; 170B
    mov.w a,$054F                     ; 170D

Spc_1710:
    mov.b $21,a                       ; 1710
    mov a,#$20                        ; 1712
    mov y,#$00                        ; 1714
    addw ya,$20                       ; 1716
    mov.w $054D,a                     ; 1718

Spc_171B:
    mov.w $054F,y                     ; 171B
.l171E:
    ret                               ; 171E

Spc_171F:
    mov.w a,$0547                     ; 171F
    cmp a,#$00                        ; 1722
    beq Spc_1726_l173E                ; 1724

Spc_1726:
    dec.w $0547                       ; 1726
    mov.w a,$054D                     ; 1729
    mov.w y,$054F                     ; 172C
    mov $20,#$20                      ; 172F
    mov $21,#$00                      ; 1732
    subw ya,$20                       ; 1735
    mov.w $054D,a                     ; 1737
    mov.w $054F,y                     ; 173A
    ret                               ; 173D
.l173E:
    clr1 $28.2                        ; 173E
    clr1 $28.3                        ; 1740
    mov a,#$00                        ; 1742
    mov.w $054D,a                     ; 1744

Spc_1747:
    mov.w $054F,a                     ; 1747
    ret                               ; 174A

Spc_174B:
    bbc $29.2,.l1751                  ; 174B
    call Spc_1758                     ; 174E
.l1751:
    bbc $29.3,.l1757                  ; 1751
    call Spc_1779                     ; 1754
.l1757:
    ret                               ; 1757

Spc_1758:
    mov.w a,$0543                     ; 1758
    cmp a,#$19                        ; 175B
    beq .l1778                        ; 175D
    inc.w $0543                       ; 175F
    mov.w a,$0549                     ; 1762
    mov.b $20,a                       ; 1765
    mov.w a,$054B                     ; 1767
    mov.b $21,a                       ; 176A
    mov a,#$3C                        ; 176C
    mov y,#$00                        ; 176E
    addw ya,$20                       ; 1770
    mov.w $0549,a                     ; 1772
    mov.w $054B,y                     ; 1775
.l1778:
    ret                               ; 1778

Spc_1779:
    mov.w a,$0543                     ; 1779
    cmp a,#$00                        ; 177C
    beq Spc_1780_l1798                ; 177E

Spc_1780:
    dec.w $0543                       ; 1780
    mov.w a,$0549                     ; 1783
    mov.w y,$054B                     ; 1786
    mov $20,#$3C                      ; 1789
    mov $21,#$00                      ; 178C
    subw ya,$20                       ; 178F
    mov.w $0549,a                     ; 1791
    mov.w $054B,y                     ; 1794
    ret                               ; 1797
.l1798:
    clr1 $29.2                        ; 1798
    clr1 $29.3                        ; 179A
    mov a,#$00                        ; 179C
    mov.w $0549,a                     ; 179E
    mov.w $054B,a                     ; 17A1
    ret                               ; 17A4

Spc_17A5:
    mov $1D,#$00                      ; 17A5
    mov.b a,$00                       ; 17A8
    and a,#$7F                        ; 17AA
    beq Spc_17AF_l17B7                ; 17AC

Spc_17AE:
    setc                              ; 17AE

Spc_17AF:
    sbc a,#$06                        ; 17AF
    bmi .l17BA                        ; 17B1
    inc a                             ; 17B3
    mov.b $1D,a                       ; 17B4
    ret                               ; 17B6
.l17B7:
    mov.b $1D,a                       ; 17B7
    ret                               ; 17B9
.l17BA:
    bbs $29.0,Spc_17C1_l17DE          ; 17BA
    mov.b a,$00                       ; 17BD
    mov.b $1B,a                       ; 17BF

Spc_17C1:
    cmp a,#$02                        ; 17C1
    bne .l17C5                        ; 17C3
.l17C5:
    mov a,#$00                        ; 17C5
    mov.b $2E,a                       ; 17C7
    mov.b $2F,a                       ; 17C9
    mov.w $0553,a                     ; 17CB
    mov.w $0554,a                     ; 17CE
    mov.w $0555,a                     ; 17D1
    clr1 $29.0                        ; 17D4
    clr1 $29.1                        ; 17D6
    clr1 $29.6                        ; 17D8
    clr1 $2C.2                        ; 17DA
    clr1 $2C.3                        ; 17DC
.l17DE:
    ret                               ; 17DE

Spc_17DF:
    mov.b a,$01                       ; 17DF
    cmp.b a,$09                       ; 17E1
    bne .l17EA                        ; 17E3
    mov $20,#$00                      ; 17E5
    bra .l17ED                        ; 17E8
.l17EA:
    mov $20,$01                       ; 17EA
.l17ED:
    mov $09,$01                       ; 17ED
    mov.b a,$20                       ; 17F0
    setc                              ; 17F2
    sbc a,#$3E                        ; 17F3
    bmi .l17FA                        ; 17F5
    mov $20,#$00                      ; 17F7  ; not reached in the traces so far
.l17FA:
    mov.b a,$20                       ; 17FA
    beq Spc_17C1_l17DE                ; 17FC
    dec a                             ; 17FE
    asl a                             ; 17FF
    mov x,a                           ; 1800
    jmp ($1804+x)                     ; 1801
    db $D5, $18, $D5, $18, $D5, $18, $D5, $18
    db $FF, $1A, $09, $1B, $D5, $18, $08, $19
    db $87, $18, $15, $19, $20, $19, $2B, $19
    db $90, $18, $36, $19, $51, $19, $56, $19
    db $5B, $19, $99, $18, $A2, $18, $D5, $18
    db $F9, $19, $8A, $19, $D5, $18, $D5, $18
    db $D5, $18, $D5, $18, $D5, $18, $D5, $18
    db $7E, $18, $D5, $18, $D5, $18, $D5, $18
    db $D5, $18, $60, $19, $AC, $18, $B9, $18
    db $D5, $18, $D5, $18, $D5, $18, $D5, $18
    db $D5, $18, $23, $1B, $23, $1B, $23, $1B
    db $23, $1B, $23, $1B, $23, $1B, $23, $1B
    db $23, $1B, $FF, $18, $FF, $18, $FF, $18
    db $FF, $18, $D5, $18, $D5, $18, $D5, $18
    db $D5, $18, $D6, $18, $E3, $18, $D5, $18
    db $D5, $18

Spc_187E:
    bbs $2D.3,Spc_18D5                ; 187E
    set1 $2A.6                        ; 1881
    mov a,#$02                        ; 1883
    bra Spc_18A2_l18A9                ; 1885

Spc_1887:
    bbs $2D.6,Spc_18D5                ; 1887
    set1 $2A.2                        ; 188A
    mov a,#$25                        ; 188C
    bra Spc_18A2_l18A9                ; 188E

Spc_1890:
    bbs $2D.6,Spc_18D5                ; 1890
    set1 $2A.4                        ; 1893
    mov a,#$15                        ; 1895
    bra Spc_18A2_l18A9                ; 1897

Spc_1899:
    bbs $2D.6,Spc_18D5                ; 1899
    set1 $2A.0                        ; 189C
    mov a,#$17                        ; 189E
    bra Spc_18A2_l18A9                ; 18A0

Spc_18A2:
    bbs $2D.6,Spc_18D5                ; 18A2
    set1 $2A.7                        ; 18A5
    mov a,#$21                        ; 18A7
.l18A9:
    jmp Spc_1424                      ; 18A9

Spc_18AC:
    bbs $29.2,Spc_18D5                ; 18AC
    set1 $29.2                        ; 18AF
    clr1 $29.3                        ; 18B1
    mov a,#$00                        ; 18B3
    mov y,#$02                        ; 18B5
    bra Spc_18B9_l18CE                ; 18B7

Spc_18B9:
    bbc $29.2,Spc_18D5                ; 18B9
    clr1 $29.2                        ; 18BC
    set1 $29.3                        ; 18BE
    mov.w a,$0549                     ; 18C0
    mov.w y,$054B                     ; 18C3
    mov $20,#$00                      ; 18C6
    mov $21,#$02                      ; 18C9
    subw ya,$20                       ; 18CC
.l18CE:
    mov.w $0549,a                     ; 18CE
    mov.w $054B,y                     ; 18D1
    ret                               ; 18D4

Spc_18D5:
    ret                               ; 18D5

Spc_18D6:
    bbs $28.2,Spc_18D5                ; 18D6
    set1 $28.2                        ; 18D9
    clr1 $28.3                        ; 18DB
    mov a,#$40                        ; 18DD
    mov y,#$00                        ; 18DF
    bra Spc_18E3_l18F8                ; 18E1

Spc_18E3:
    bbc $28.2,Spc_18D5                ; 18E3
    clr1 $28.2                        ; 18E6
    set1 $28.3                        ; 18E8
    mov.w a,$054D                     ; 18EA
    mov.w y,$054F                     ; 18ED
    mov $20,#$40                      ; 18F0
    mov $21,#$00                      ; 18F3
    subw ya,$20                       ; 18F6
.l18F8:
    mov.w $054D,a                     ; 18F8
    mov.w $054F,y                     ; 18FB
    ret                               ; 18FE

Spc_18FF:
    mov.b a,$20                       ; 18FF
    setc                              ; 1901
    sbc a,#$32                        ; 1902
    mov.w $0530,a                     ; 1904
    ret                               ; 1907

Spc_1908:
    clr1 $2A.6                        ; 1908
    mov.w a,$0394                     ; 190A
    cmp a,#$02                        ; 190D
    bne Spc_1936_l194B                ; 190F
    mov x,#$04                        ; 1911
    bra Spc_1936_l1941                ; 1913

Spc_1915:
    clr1 $2A.2                        ; 1915

Spc_1917:
    mov.w a,$0395                     ; 1917
    cmp a,#$25                        ; 191A
    bne Spc_1936_l194B                ; 191C
    bra Spc_1936_l193F                ; 191E

Spc_1920:
    clr1 $2A.4                        ; 1920
    mov.w a,$0395                     ; 1922
    cmp a,#$15                        ; 1925
    bne Spc_1936_l194B                ; 1927
    bra Spc_1936_l193F                ; 1929

Spc_192B:
    clr1 $2A.0                        ; 192B
    mov.w a,$0395                     ; 192D
    cmp a,#$17                        ; 1930
    bne Spc_1936_l194B                ; 1932
    bra Spc_1936_l193F                ; 1934

Spc_1936:
    clr1 $2A.7                        ; 1936
    mov.w a,$0395                     ; 1938
    cmp a,#$21                        ; 193B
    bne .l194B                        ; 193D
.l193F:
    mov x,#$05                        ; 193F
.l1941:
    mov.w a,$0408+x                   ; 1941
    beq .l194B                        ; 1944
    mov.b a,$20                       ; 1946
    mov.w $03E0+x,a                   ; 1948
.l194B:
    ret                               ; 194B

Spc_194C:
    mov $20,#$01                      ; 194C
    bra Spc_1936_l1941                ; 194F

Spc_1951:
    clr1 $28.1                        ; 1951
    set1 $28.0                        ; 1953
    ret                               ; 1955

Spc_1956:
    clr1 $28.0                        ; 1956
    clr1 $28.1                        ; 1958
    ret                               ; 195A

Spc_195B:
    clr1 $28.0                        ; 195B
    set1 $28.1                        ; 195D
.l195F:
    ret                               ; 195F

Spc_1960:
    bbc $29.0,Spc_195B_l195F          ; 1960
    mov a,#$4C                        ; 1963
    call Spc_1424                     ; 1965
    clr1 $29.0                        ; 1968
    mov a,#$00                        ; 196A
    mov.w $045F,a                     ; 196C
    mov.w $045E,a                     ; 196F
    mov.w $045D,a                     ; 1972
    mov.w $045C,a                     ; 1975
    mov.w $04FF,a                     ; 1978
    mov.w $04FE,a                     ; 197B
    mov.w $04FD,a                     ; 197E
    mov.w $04FC,a                     ; 1981
    call Spc_0A60                     ; 1984
    jmp $1798                         ; 1987

Spc_198A:
    bbc $29.0,Spc_195B_l195F          ; 198A
    mov $2A,$2B                       ; 198D
    bbc $29.7,.l19A9                  ; 1990
    clr1 $29.7                        ; 1993  ; not reached in the traces so far
    mov.w a,$0574                     ; 1995  ; not reached in the traces so far
    mov.b $4A,a                       ; 1998  ; not reached in the traces so far
    mov.w a,$0575                     ; 199A  ; not reached in the traces so far
    mov.b $4E,a                       ; 199D  ; not reached in the traces so far
    mov.w a,$0576                     ; 199F  ; not reached in the traces so far
    mov.b $61,a                       ; 19A2  ; not reached in the traces so far
    mov.w a,$0577                     ; 19A4  ; not reached in the traces so far
    mov.b $63,a                       ; 19A7  ; not reached in the traces so far
.l19A9:
    mov a,#$4B                        ; 19A9
    call Spc_1424                     ; 19AB
    clr1 $29.0                        ; 19AE
    clr1 $2D.6                        ; 19B0
    mov x,#$07                        ; 19B2
    call Spc_19CC                     ; 19B4
    dec x                             ; 19B7
    call Spc_19CC                     ; 19B8
    dec x                             ; 19BB

Spc_19BC:
    call Spc_19CC                     ; 19BC
    dec x                             ; 19BF
    call Spc_19CC                     ; 19C0
    mov x,#$01                        ; 19C3
    call Spc_194C                     ; 19C5
    dec x                             ; 19C8
    jmp Spc_194C                      ; 19C9

Spc_19CC:
    mov.w a,$0460+x                   ; 19CC
    mov.w $03C8+x,a                   ; 19CF
    mov.w a,$0458+x                   ; 19D2
    beq .l19E7                        ; 19D5
    push a                            ; 19D7
    call Spc_19EA                     ; 19D8
    mov a,#$00                        ; 19DB
    mov.w $0458+x,a                   ; 19DD
    pop a                             ; 19E0
    push x                            ; 19E1
    call Spc_219C                     ; 19E2
    pop x                             ; 19E5
    ret                               ; 19E6
.l19E7:
    jmp Spc_194C                      ; 19E7

Spc_19EA:
    dec a                             ; 19EA
    asl a                             ; 19EB
    mov y,a                           ; 19EC
    mov.w a,$0702+y                   ; 19ED
    mov.b $24,a                       ; 19F0
    inc y                             ; 19F2
    mov.w a,$0702+y                   ; 19F3
    mov.b $25,a                       ; 19F6
    ret                               ; 19F8

Spc_19F9:
    bbs $29.0,Spc_1A54_l1A67          ; 19F9
    mov $2B,$2A                       ; 19FC
    mov $2A,#$00                      ; 19FF
    mov.w a,$0551                     ; 1A02
    beq .l1A2B                        ; 1A05
    set1 $29.7                        ; 1A07  ; not reached in the traces so far
    set1 $2D.6                        ; 1A09  ; not reached in the traces so far
    mov.b a,$4A                       ; 1A0B  ; not reached in the traces so far
    mov.w $0574,a                     ; 1A0D  ; not reached in the traces so far
    mov.b a,$4E                       ; 1A10  ; not reached in the traces so far
    mov.w $0575,a                     ; 1A12  ; not reached in the traces so far
    mov.b a,$61                       ; 1A15  ; not reached in the traces so far
    mov.w $0576,a                     ; 1A17  ; not reached in the traces so far
    mov.b a,$63                       ; 1A1A  ; not reached in the traces so far
    mov.w $0577,a                     ; 1A1C  ; not reached in the traces so far
    mov a,#$50                        ; 1A1F  ; not reached in the traces so far
    mov.b $4E,a                       ; 1A21  ; not reached in the traces so far
    mov a,#$11                        ; 1A23  ; not reached in the traces so far
    mov.b $61,a                       ; 1A25  ; not reached in the traces so far
    mov a,#$11                        ; 1A27  ; not reached in the traces so far
    mov.b $63,a                       ; 1A29  ; not reached in the traces so far
.l1A2B:
    set1 $29.0                        ; 1A2B
    mov a,#$4A                        ; 1A2D

Spc_1A2F:
    call Spc_1424                     ; 1A2F
    mov x,#$07                        ; 1A32
    call Spc_1A54                     ; 1A34
    dec x                             ; 1A37
    call Spc_1A54                     ; 1A38
    dec x                             ; 1A3B
    call Spc_1A54                     ; 1A3C
    dec x                             ; 1A3F
    call Spc_1A54                     ; 1A40
    mov x,#$07                        ; 1A43
    call Spc_194C                     ; 1A45
    dec x                             ; 1A48
    call Spc_194C                     ; 1A49
    dec x                             ; 1A4C
    call Spc_194C                     ; 1A4D
    dec x                             ; 1A50
    jmp Spc_194C                      ; 1A51

Spc_1A54:
    mov.w a,$04F8+x                   ; 1A54
    beq .l1A68                        ; 1A57
.l1A59:
    mov.w a,$04F8+x                   ; 1A59  ; not reached in the traces so far
    mov.w $0458+x,a                   ; 1A5C  ; not reached in the traces so far
    mov a,#$00                        ; 1A5F  ; not reached in the traces so far
    mov.w $04F8+x,a                   ; 1A61  ; not reached in the traces so far
    mov.w $0480+x,a                   ; 1A64  ; not reached in the traces so far
.l1A67:
    ret                               ; 1A67
.l1A68:
    mov.w a,$0480+x                   ; 1A68
    beq .l1A67                        ; 1A6B
    cmp a,#$02                        ; 1A6D
    beq .l1A59                        ; 1A6F
    mov.w a,$03C8+x                   ; 1A71
    mov.w $0460+x,a                   ; 1A74
    mov.w a,$0390+x                   ; 1A77
    mov.w $0458+x,a                   ; 1A7A
    mov a,#$00                        ; 1A7D
    mov.w $0480+x,a                   ; 1A7F
    ret                               ; 1A82

Spc_1A83:
    mov $2A,#$00                      ; 1A83
    call Spc_1AAA                     ; 1A86
    mov x,#$07                        ; 1A89
.l1A8B:
    call Spc_1A92                     ; 1A8B
    dec x                             ; 1A8E
    bpl .l1A8B                        ; 1A8F
    ret                               ; 1A91

Spc_1A92:
    mov a,#$00                        ; 1A92
    mov.w $04F8+x,a                   ; 1A94
    mov.w $0480+x,a                   ; 1A97
    mov.b $2A,a                       ; 1A9A
    mov.w a,$0408+x                   ; 1A9C
    beq .l1AA9                        ; 1A9F
    mov a,#$01                        ; 1AA1
    mov.w $04C0+x,a                   ; 1AA3
    mov.w $04C8+x,a                   ; 1AA6
.l1AA9:
    ret                               ; 1AA9

Spc_1AAA:
    bbs $29.0,.l1AAF                  ; 1AAA
    set1 $29.6                        ; 1AAD
.l1AAF:
    ret                               ; 1AAF

Spc_1AB0:
    bbc $29.6,Spc_1AEF_l1AF6          ; 1AB0
    mov a,#$F0                        ; 1AB3
    mov y,#$02                        ; 1AB5
    addw ya,$2E                       ; 1AB7
    mov.b $2E,a                       ; 1AB9
    mov.b $2F,y                       ; 1ABB
    mov a,#$00                        ; 1ABD
    mov.w y,$0564                     ; 1ABF
    subw ya,$2E                       ; 1AC2
    mov.w $0565,y                     ; 1AC4
    cmp y,#$00                        ; 1AC7
    beq .l1AD8                        ; 1AC9
    cmp y,#$01                        ; 1ACB
    beq .l1AD8                        ; 1ACD
    cmp y,#$02                        ; 1ACF
    beq .l1AD8                        ; 1AD1
    cmp y,#$03                        ; 1AD3
    beq .l1AD8                        ; 1AD5
    ret                               ; 1AD7
.l1AD8:
    mov a,#$00                        ; 1AD8
    mov.b $2E,a                       ; 1ADA
    mov.b $2F,a                       ; 1ADC
    clr1 $29.6                        ; 1ADE
    bbs $2C.7,.l1AE9                  ; 1AE0
    bbs $2D.0,Spc_1B09_l1B15          ; 1AE3
    jmp Spc_0A60                      ; 1AE6  ; not reached in the traces so far
.l1AE9:
    call Spc_1AEF                     ; 1AE9
    jmp Spc_0675                      ; 1AEC

Spc_1AEF:
    mov a,#$FF                        ; 1AEF
    mov y,#$5C                        ; 1AF1
    jmp Spc_09E8                      ; 1AF3
.l1AF6:
    mov.b a,$59                       ; 1AF6
    mov.w $0564,a                     ; 1AF8
    mov.w $0565,a                     ; 1AFB
    ret                               ; 1AFE

Spc_1AFF:
    call Spc_1A83                     ; 1AFF
    mov.b a,$59                       ; 1B02
    beq Spc_1AB0_l1AE9                ; 1B04
    set1 $2C.7                        ; 1B06
    ret                               ; 1B08

Spc_1B09:
    set1 $2D.0                        ; 1B09
    call Spc_1A83                     ; 1B0B
    mov.b a,$59                       ; 1B0E
    beq .l1B15                        ; 1B10
    set1 $2D.0                        ; 1B12
    ret                               ; 1B14
.l1B15:
    call Spc_1AEF                     ; 1B15
    mov.w a,!spc_control              ; 1B18
    or a,#$80                         ; 1B1B
    mov.w !spc_control,a              ; 1B1D
    jmp $FFC0                         ; 1B20

Spc_1B23:
    mov.b a,$20                       ; 1B23
    setc                              ; 1B25
    sbc a,#$2A                        ; 1B26
    mov.w $0562,a                     ; 1B28
    ret                               ; 1B2B

Spc_1B2C:
    mov.b a,$1D                       ; 1B2C
    beq Spc_1B48_l1B76                ; 1B2E
    mov y,#$00                        ; 1B30
.l1B32:
    mov.w a,$26D6+y                   ; 1B32
    beq .l1B3E                        ; 1B35
    cmp.b a,$1D                       ; 1B37
    beq Spc_1B48_l1B79                ; 1B39
    inc y                             ; 1B3B
    bra .l1B32                        ; 1B3C
.l1B3E:
    mov y,#$00                        ; 1B3E
.l1B40:
    mov.w a,$26E4+y                   ; 1B40
    beq Spc_1B48_l1B79                ; 1B43
    push a                            ; 1B45
    mov.b a,$1D                       ; 1B46

Spc_1B48:
    push y                            ; 1B48
    call Spc_1C8A                     ; 1B49
    call Spc_1C9E                     ; 1B4C
    pop y                             ; 1B4F
    pop a                             ; 1B50
    cmp.w a,$0390+x                   ; 1B51
    beq .l1B59                        ; 1B54
.l1B56:
    inc y                             ; 1B56
    bra Spc_1B2C_l1B40                ; 1B57
.l1B59:
    mov x,#$00                        ; 1B59
.l1B5B:
    mov.w a,$26E4+x                   ; 1B5B
    beq .l1B6F                        ; 1B5E
    cmp.b a,$1D                       ; 1B60
    beq .l1B67                        ; 1B62
    inc x                             ; 1B64
    bra .l1B5B                        ; 1B65
.l1B67:
    mov.b $20,x                       ; 1B67
    mov a,y                           ; 1B69
    setc                              ; 1B6A
    sbc.b a,$20                       ; 1B6B
    bpl .l1B56                        ; 1B6D
.l1B6F:
    mov $1F,$1D                       ; 1B6F
    mov $1D,#$00                      ; 1B72
    ret                               ; 1B75
.l1B76:
    mov $1F,#$00                      ; 1B76
.l1B79:
    ret                               ; 1B79

Spc_1B7A:
    mov x,#$00                        ; 1B7A
.l1B7C:
    mov.w a,$1B8C+x                   ; 1B7C
    beq .l1B8B                        ; 1B7F
    cmp.b a,$1D                       ; 1B81
    beq .l1B88                        ; 1B83
    inc x                             ; 1B85
    bra .l1B7C                        ; 1B86
.l1B88:
    mov $1D,#$00                      ; 1B88  ; not reached in the traces so far
.l1B8B:
    ret                               ; 1B8B
    db $02, $25, $15, $17, $21, $00

Spc_1B92:
    mov x,#$00                        ; 1B92
.l1B94:
    mov.w a,$0468+x                   ; 1B94
    beq .l1B9D                        ; 1B97
    inc a                             ; 1B99
    mov.w $0468+x,a                   ; 1B9A
.l1B9D:
    inc x                             ; 1B9D

Spc_1B9E:
    cmp x,#$08                        ; 1B9E
    bne Spc_1B92_l1B94                ; 1BA0
    mov.b a,$1D                       ; 1BA2
    beq Spc_1BA9_l1BD3                ; 1BA4
    call Spc_19EA                     ; 1BA6

Spc_1BA9:
    mov y,#$00                        ; 1BA9
    mov a,($24)+y                     ; 1BAB
    and a,#$07                        ; 1BAD
    mov x,a                           ; 1BAF
    mov $26,$1D                       ; 1BB0
    mov.w a,$0470+x                   ; 1BB3
    beq .l1BBB                        ; 1BB6
    mov $1D,#$00                      ; 1BB8
.l1BBB:
    mov y,#$00                        ; 1BBB
.l1BBD:
    mov.w a,$1C04+y                   ; 1BBD
    beq .l1BD2                        ; 1BC0
    cmp.b a,$26                       ; 1BC2
    beq .l1BCA                        ; 1BC4
    inc y                             ; 1BC6
    inc y                             ; 1BC7
    bra .l1BBD                        ; 1BC8
.l1BCA:
    mov.w $0470+x,a                   ; 1BCA
    mov a,#$01                        ; 1BCD
    mov.w $0468+x,a                   ; 1BCF
.l1BD2:
    ret                               ; 1BD2
.l1BD3:
    mov y,#$00                        ; 1BD3
.l1BD5:
    mov.w a,$0470+y                   ; 1BD5
    bne .l1BE0                        ; 1BD8
.l1BDA:
    inc y                             ; 1BDA
    cmp y,#$08                        ; 1BDB
    bne .l1BD5                        ; 1BDD
    ret                               ; 1BDF
.l1BE0:
    mov.b $26,a                       ; 1BE0
    mov x,#$00                        ; 1BE2
.l1BE4:
    mov.w a,$1C04+x                   ; 1BE4
    beq .l1BDA                        ; 1BE7
    cmp.b a,$26                       ; 1BE9
    beq .l1BF1                        ; 1BEB
    inc x                             ; 1BED  ; not reached in the traces so far
    inc x                             ; 1BEE  ; not reached in the traces so far
    bra .l1BE4                        ; 1BEF  ; not reached in the traces so far
.l1BF1:
    inc x                             ; 1BF1
    mov.w a,$1C04+x                   ; 1BF2
    cmp.w a,$0468+y                   ; 1BF5
    bne .l1BDA                        ; 1BF8
    mov a,#$00                        ; 1BFA
    mov.w $0468+y,a                   ; 1BFC
    mov.w $0470+y,a                   ; 1BFF
    bra .l1BDA                        ; 1C02
    db $1E, $09, $00
.l1C07:
    mov.b a,$1D                       ; 1C07
    call Spc_1424                     ; 1C09
    mov $1D,#$00                      ; 1C0C
    cmp $1D,#$00                      ; 1C0F
    beq .l1C1B                        ; 1C12
    mov a,#$00                        ; 1C14  ; not reached in the traces so far
    mov.w $057B,a                     ; 1C16  ; not reached in the traces so far
    bra .l1C25                        ; 1C19  ; not reached in the traces so far
.l1C1B:
    mov.w a,$057B                     ; 1C1B
    cmp a,#$FF                        ; 1C1E
    beq .l1C25                        ; 1C20
    inc.w $057B                       ; 1C22
.l1C25:
    cmp $1D,#$00                      ; 1C25
    bne .l1C54                        ; 1C28
    mov.w a,$057B                     ; 1C2A
    beq .l1C54                        ; 1C2D
    cmp a,#$01                        ; 1C2F
    beq .l1C54                        ; 1C31
    cmp a,#$02                        ; 1C33
    beq .l1C54                        ; 1C35
    mov.w a,$0588                     ; 1C37
    beq .l1C54                        ; 1C3A
    push a                            ; 1C3C
    mov x,#$01                        ; 1C3D
.l1C3F:
    mov.w a,$0588+x                   ; 1C3F
    dec x                             ; 1C42
    mov.w $0588+x,a                   ; 1C43
    inc x                             ; 1C46
    inc x                             ; 1C47
    cmp x,#$08                        ; 1C48
    bne .l1C3F                        ; 1C4A
    pop a                             ; 1C4C
    mov x,#$00                        ; 1C4D
    mov.w $057B,x                     ; 1C4F
    bra .l1C63                        ; 1C52
.l1C54:
    mov.b a,$1D                       ; 1C54
    cmp.b a,$1E                       ; 1C56
    bne .l1C5D                        ; 1C58
.l1C5A:
    jmp Spc_1CCD                      ; 1C5A
.l1C5D:
    mov.b $1E,a                       ; 1C5D  ; not reached in the traces so far
    mov.b a,$1D                       ; 1C5F  ; not reached in the traces so far
    beq .l1C5A                        ; 1C61  ; not reached in the traces so far
.l1C63:
    mov.b $23,a                       ; 1C63
    call Spc_1C8A                     ; 1C65
    call Spc_1C9E                     ; 1C68
    call Spc_1CA5                     ; 1C6B
    call Spc_1C84                     ; 1C6E
    call Spc_1CED                     ; 1C71
    mov.b a,$23                       ; 1C74
    mov.w $0410+x,a                   ; 1C76
    push x                            ; 1C79
    call Spc_1CCD                     ; 1C7A
    pop x                             ; 1C7D
    mov a,#$00                        ; 1C7E
    mov.w $0410+x,a                   ; 1C80
    ret                               ; 1C83

Spc_1C84:
    mov a,#$01                        ; 1C84
    mov.w $0408+x,a                   ; 1C86
    ret                               ; 1C89

Spc_1C8A:
    dec a                             ; 1C8A
    asl a                             ; 1C8B
    mov y,a                           ; 1C8C
    mov.w a,$0702+y                   ; 1C8D
    mov.b $20,a                       ; 1C90
    inc y                             ; 1C92
    mov.w a,$0702+y                   ; 1C93
    mov.b $21,a                       ; 1C96
    mov y,#$00                        ; 1C98
    mov a,($20)+y                     ; 1C9A
    mov x,a                           ; 1C9C
    ret                               ; 1C9D

Spc_1C9E:
    mov.b $26,x                       ; 1C9E
    mov a,x                           ; 1CA0
    and a,#$0F                        ; 1CA1
    mov x,a                           ; 1CA3
    ret                               ; 1CA4

Spc_1CA5:
    bbc $26.7,.l1CBA                  ; 1CA5
    bbc $26.5,.l1CB4                  ; 1CA8
    mov a,#$00                        ; 1CAB
    mov.w $04F8+x,a                   ; 1CAD
    mov a,#$02                        ; 1CB0
    bra .l1CB6                        ; 1CB2
.l1CB4:
    mov a,#$01                        ; 1CB4
.l1CB6:
    mov.w $0480+x,a                   ; 1CB6
    ret                               ; 1CB9
.l1CBA:
    mov.w a,$0480+x                   ; 1CBA
    cmp a,#$01                        ; 1CBD
    bne Spc_1CCC                      ; 1CBF
    mov.w a,$04F8+x                   ; 1CC1
    bne Spc_1CCC                      ; 1CC4
    mov.w a,$0390+x                   ; 1CC6
    mov.w $04F8+x,a                   ; 1CC9

Spc_1CCC:
    ret                               ; 1CCC

Spc_1CCD:
    mov x,#$00                        ; 1CCD
.l1CCF:
    mov.w a,$0408+x                   ; 1CCF
    beq .l1CDA                        ; 1CD2
    call Spc_1CE2                     ; 1CD4
    call Spc_1CF8                     ; 1CD7
.l1CDA:
    inc x                             ; 1CDA
    cmp x,#$08                        ; 1CDB
    beq .l1CE1                        ; 1CDD
    bra .l1CCF                        ; 1CDF
.l1CE1:
    ret                               ; 1CE1

Spc_1CE2:
    mov.w a,$0488+x                   ; 1CE2

Spc_1CE5:
    mov.b $20,a                       ; 1CE5
    mov.w a,$0490+x                   ; 1CE7
    mov.b $21,a                       ; 1CEA
    ret                               ; 1CEC

Spc_1CED:
    mov.b a,$20                       ; 1CED
    mov.w $0488+x,a                   ; 1CEF
    mov.b a,$21                       ; 1CF2
    mov.w $0490+x,a                   ; 1CF4
    ret                               ; 1CF7

Spc_1CF8:
    mov.w a,$03F8+x                   ; 1CF8
    beq Spc_1D50                      ; 1CFB
    cmp a,#$01                        ; 1CFD
    beq .l1D1C                        ; 1CFF
    cmp a,#$02                        ; 1D01
    beq .l1D31                        ; 1D03
    cmp a,#$03                        ; 1D05
    beq .l1D36                        ; 1D07
    mov.w a,$0410+x                   ; 1D09
    bne Spc_1D50_l1D53                ; 1D0C
    mov a,#$00                        ; 1D0E
    mov.w $0390+x,a                   ; 1D10
    mov.w $03F8+x,a                   ; 1D13
    mov.w $0408+x,a                   ; 1D16
    jmp Spc_1E31                      ; 1D19
.l1D1C:
    mov.w a,$0410+x                   ; 1D1C
    bne Spc_1D50_l1D53                ; 1D1F
    mov a,#$02                        ; 1D21
    mov.w $03F8+x,a                   ; 1D23
    call Spc_1E89                     ; 1D26
    mov a,#$00                        ; 1D29
    mov.w $03E8+x,a                   ; 1D2B
.l1D2E:
    jmp $1E9D                         ; 1D2E
.l1D31:
    mov a,#$00                        ; 1D31
    mov.w $03E8+x,a                   ; 1D33
.l1D36:
    mov.w a,$0410+x                   ; 1D36
    bne Spc_1D50_l1D53                ; 1D39
    mov a,#$03                        ; 1D3B
    mov.w $03F8+x,a                   ; 1D3D
    mov.w a,$03E8+x                   ; 1D40
    beq Spc_1D63_l1D6C                ; 1D43
    cmp a,#$01                        ; 1D45
    beq .l1D1C                        ; 1D47
    mov a,#$00                        ; 1D49
    mov.w $03F0+x,a                   ; 1D4B
    bra .l1D1C                        ; 1D4E

Spc_1D50:
    mov.w a,$0410+x                   ; 1D50
.l1D53:
    mov.w $0390+x,a                   ; 1D53
    mov a,#$01                        ; 1D56
    mov.w $03F8+x,a                   ; 1D58
    mov a,#$00                        ; 1D5B
    mov.w $03F0+x,a                   ; 1D5D
    mov.w $03E0+x,a                   ; 1D60

Spc_1D63:
    call Spc_1E0F                     ; 1D63
    jmp Spc_1E93                      ; 1D66
.l1D69:
    jmp $1DE8                         ; 1D69
.l1D6C:
    mov.w a,$03E0+x                   ; 1D6C
    beq Spc_1CF8_l1D2E                ; 1D6F
    cmp a,#$01                        ; 1D71
    beq .l1D69                        ; 1D73
    cmp a,#$07                        ; 1D75
    beq .l1DCE                        ; 1D77
    cmp a,#$08                        ; 1D79
    beq .l1D94                        ; 1D7B
    cmp a,#$0A                        ; 1D7D
    beq .l1D9D                        ; 1D7F
    cmp a,#$0B                        ; 1D81
    beq .l1DA6                        ; 1D83
    cmp a,#$0C                        ; 1D85
    beq .l1DAF                        ; 1D87
    cmp a,#$0E                        ; 1D89
    beq .l1DB8                        ; 1D8B
.l1D8D:
    mov a,#$00                        ; 1D8D  ; not reached in the traces so far
    mov.w $03E0+x,a                   ; 1D8F  ; not reached in the traces so far
    bra Spc_1CF8_l1D2E                ; 1D92  ; not reached in the traces so far
.l1D94:
    mov.w a,$0390+x                   ; 1D94
    cmp a,#$02                        ; 1D97
    beq .l1DCE                        ; 1D99
    bra .l1D8D                        ; 1D9B  ; not reached in the traces so far
.l1D9D:
    mov.w a,$0390+x                   ; 1D9D
    cmp a,#$25                        ; 1DA0
    beq .l1DCE                        ; 1DA2
    bra .l1D8D                        ; 1DA4  ; not reached in the traces so far
.l1DA6:
    mov.w a,$0390+x                   ; 1DA6
    cmp a,#$15                        ; 1DA9
    beq .l1DCE                        ; 1DAB
    bra .l1D8D                        ; 1DAD  ; not reached in the traces so far
.l1DAF:
    mov.w a,$0390+x                   ; 1DAF
    cmp a,#$17                        ; 1DB2
    beq .l1DCE                        ; 1DB4
    bra .l1D8D                        ; 1DB6  ; not reached in the traces so far
.l1DB8:
    mov.w a,$0390+x                   ; 1DB8
    cmp a,#$21                        ; 1DBB
    beq .l1DCE                        ; 1DBD
    bra .l1D8D                        ; 1DBF  ; not reached in the traces so far
    db $F5, $90, $03, $68, $05, $F0, $20, $68
    db $08, $F0, $1C, $2F, $BF
.l1DCE:
    mov a,#$00                        ; 1DCE
    mov.w $03E0+x,a                   ; 1DD0
    mov.w a,$04F8+x                   ; 1DD3
    beq .l1DE0                        ; 1DD6
    mov a,#$00                        ; 1DD8
    mov.w $04F8+x,a                   ; 1DDA
    jmp $1D2E                         ; 1DDD
.l1DE0:
    mov.w a,$0480+x                   ; 1DE0
    bne .l1DF5                        ; 1DE3
    jmp $1D2E                         ; 1DE5  ; not reached in the traces so far
.l1DE8:
    mov a,#$00                        ; 1DE8
    mov.w $03E0+x,a                   ; 1DEA
    mov.w a,$04F8+x                   ; 1DED
    beq .l1DF5                        ; 1DF0
    call Spc_1424                     ; 1DF2
.l1DF5:
    mov a,#$04                        ; 1DF5
    mov.w $03F8+x,a                   ; 1DF7
    call Spc_1E93                     ; 1DFA
    mov a,#$00                        ; 1DFD
    mov.w $0480+x,a                   ; 1DFF
    mov.w $04F8+x,a                   ; 1E02
    mov.w $03F0+x,a                   ; 1E05
    mov.w $03C8+x,a                   ; 1E08
    mov.w $04F0+x,a                   ; 1E0B
    ret                               ; 1E0E

Spc_1E0F:
    call Spc_1E19                     ; 1E0F
    mov.b a,$1A                       ; 1E12
    or.b a,$22                        ; 1E14
    mov.b $1A,a                       ; 1E16
    ret                               ; 1E18

Spc_1E19:
    mov.b $22,x                       ; 1E19
    inc.b $22                         ; 1E1B
    mov a,#$00                        ; 1E1D
    setc                              ; 1E1F
.l1E20:
    rol a                             ; 1E20
    dec.b $22                         ; 1E21
    bne .l1E20                        ; 1E23
    mov.b $22,a                       ; 1E25
    ret                               ; 1E27

Spc_1E28:
    mov.b $22,x                       ; 1E28
    inc.b $22                         ; 1E2A
    mov a,#$FF                        ; 1E2C
    clrc                              ; 1E2E
    bra Spc_1E19_l1E20                ; 1E2F

Spc_1E31:
    call Spc_1E28                     ; 1E31
    mov.b a,$1A                       ; 1E34
    and.b a,$22                       ; 1E36
    mov.b $1A,a                       ; 1E38
    push x                            ; 1E3A
    clrc                              ; 1E3B
    mov a,x                           ; 1E3C
    asl a                             ; 1E3D
    mov x,a                           ; 1E3E
    mov.w a,$0211+x                   ; 1E3F
    call Spc_0BC1                     ; 1E42
    pop x                             ; 1E45
    mov.b $20,x                       ; 1E46
    inc.b $20                         ; 1E48
    mov.b a,$4F                       ; 1E4A
.l1E4C:
    lsr a                             ; 1E4C
    dec.b $20                         ; 1E4D
    bne .l1E4C                        ; 1E4F
    bcc .l1E5E                        ; 1E51
    call Spc_1E19                     ; 1E53
    mov.b a,$4A                       ; 1E56
    or.b a,$22                        ; 1E58
    mov.b $4A,a                       ; 1E5A
    bra .l1E67                        ; 1E5C
.l1E5E:
    call Spc_1E28                     ; 1E5E
    mov.b a,$4A                       ; 1E61
    and.b a,$22                       ; 1E63
    mov.b $4A,a                       ; 1E65
.l1E67:
    call Spc_1E28                     ; 1E67
    mov.b a,$1A                       ; 1E6A
    and.b a,$22                       ; 1E6C
    mov.b $1A,a                       ; 1E6E
    call Spc_1E19                     ; 1E70
    mov.b a,$47                       ; 1E73
    or.b a,$22                        ; 1E75
    mov.b $47,a                       ; 1E77
    mov.b a,$5E                       ; 1E79
    or.b a,$20                        ; 1E7B
    mov.b $5E,a                       ; 1E7D
    push x                            ; 1E7F
    clrc                              ; 1E80
    mov a,x                           ; 1E81
    asl a                             ; 1E82
    mov x,a                           ; 1E83
    call Spc_0E97                     ; 1E84
    pop x                             ; 1E87
    ret                               ; 1E88

Spc_1E89:
    call Spc_1E19                     ; 1E89
    mov.b a,$45                       ; 1E8C
    or.b a,$22                        ; 1E8E
    mov.b $45,a                       ; 1E90
    ret                               ; 1E92

Spc_1E93:
    call Spc_1E19                     ; 1E93
    mov.b a,$46                       ; 1E96
    or.b a,$22                        ; 1E98
    mov.b $46,a                       ; 1E9A
    ret                               ; 1E9C
.l1E9D:
    call Spc_1EA6                     ; 1E9D
    call Spc_23CF                     ; 1EA0
    jmp $22D3                         ; 1EA3

Spc_1EA6:
    mov.w a,$0390+x                   ; 1EA6
    beq .l1EC9                        ; 1EA9
    mov.w a,$0448+x                   ; 1EAB
    beq .l1EBC                        ; 1EAE
    mov.w a,$03F0+x                   ; 1EB0
    setc                              ; 1EB3
    sbc.w a,$0448+x                   ; 1EB4
    bmi .l1EBC                        ; 1EB7
    call Spc_194C                     ; 1EB9
.l1EBC:
    mov.w a,$03F0+x                   ; 1EBC
    cmp a,#$FF                        ; 1EBF
    beq .l1EC7                        ; 1EC1
    inc a                             ; 1EC3
    mov.w $03F0+x,a                   ; 1EC4
.l1EC7:
    bra .l1ECF                        ; 1EC7
.l1EC9:
    mov a,#$00                        ; 1EC9  ; not reached in the traces so far
    mov.w $03F0+x,a                   ; 1ECB  ; not reached in the traces so far
    ret                               ; 1ECE  ; not reached in the traces so far
.l1ECF:
    call Spc_1CE2                     ; 1ECF
    mov.w a,$03F0+x                   ; 1ED2
    beq .l1EDD                        ; 1ED5
    cmp a,#$01                        ; 1ED7
    beq .l1EDE                        ; 1ED9
    bra Spc_1F1B_l1F30                ; 1EDB
.l1EDD:
    ret                               ; 1EDD  ; not reached in the traces so far
.l1EDE:
    mov a,#$00                        ; 1EDE
    mov.w $0400+x,a                   ; 1EE0
    mov.w $0438+x,a                   ; 1EE3
    mov.w $0520+x,a                   ; 1EE6
    mov.w $0478+x,a                   ; 1EE9
    mov.w $0448+x,a                   ; 1EEC
    mov.w $0440+x,a                   ; 1EEF
    mov.w $03A8+x,a                   ; 1EF2
    mov.w $03B0+x,a                   ; 1EF5
    mov.w $04C0+x,a                   ; 1EF8
    mov.w $04C8+x,a                   ; 1EFB
    mov.w $04F0+x,a                   ; 1EFE
    mov.w $0398+x,a                   ; 1F01
    mov.w $04A8+x,a                   ; 1F04
    mov.w $03A0+x,a                   ; 1F07
    mov.w $04B0+x,a                   ; 1F0A
    mov.w $03B8+x,a                   ; 1F0D
    mov.w $0498+x,a                   ; 1F10
    mov.w $03C0+x,a                   ; 1F13
    mov.w $04A0+x,a                   ; 1F16
    mov a,#$8F                        ; 1F19

Spc_1F1B:
    mov.w $03D0+x,a                   ; 1F1B
    mov a,#$E0                        ; 1F1E
    mov.w $03D8+x,a                   ; 1F20
    call Spc_1E28                     ; 1F23
    mov.b a,$4A                       ; 1F26
    and.b a,$22                       ; 1F28
    mov.b $4A,a                       ; 1F2A
    mov y,#$01                        ; 1F2C
    bra .l1F48                        ; 1F2E
.l1F30:
    mov.w a,$0418+x                   ; 1F30
    dec a                             ; 1F33
    mov.w $0418+x,a                   ; 1F34
    beq .l1F43                        ; 1F37
    cmp a,#$FF                        ; 1F39
    beq .l1F43                        ; 1F3B
    call Spc_224A                     ; 1F3D
    jmp $2286                         ; 1F40
.l1F43:
    mov.w a,$0400+x                   ; 1F43
    mov y,a                           ; 1F46
.l1F47:
    inc y                             ; 1F47
.l1F48:
    mov a,($20)+y                     ; 1F48
    beq .l1F47                        ; 1F4A
    bpl .l1F9B                        ; 1F4C
    setc                              ; 1F4E
    sbc a,#$E0                        ; 1F4F
    bmi .l1FCC                        ; 1F51
    asl a                             ; 1F53
    push x                            ; 1F54
    mov x,a                           ; 1F55
    jmp ($1F59+x)                     ; 1F56
    db $C8, $21, $53, $20, $57, $20, $5F, $20
    db $A0, $20, $EB, $20, $FA, $20, $14, $21
    db $28, $21, $3C, $21, $50, $21, $5F, $21
    db $6B, $21, $D2, $21, $F3, $21, $99, $1F
    db $99, $1F, $FC, $21, $04, $22, $0C, $22
    db $99, $1F, $99, $1F, $99, $1F, $99, $1F
    db $1E, $22, $99, $1F, $99, $1F, $27, $22
    db $32, $22, $3B, $22, $9E, $20, $99, $1F
    db $CE, $6F
.l1F9B:
    push a                            ; 1F9B
    mov.w a,$0478+x                   ; 1F9C
    beq .l1FA7                        ; 1F9F
    cmp a,#$01                        ; 1FA1
    beq .l1FAA                        ; 1FA3
    bra .l1FAF                        ; 1FA5
.l1FA7:
    call Spc_1E93                     ; 1FA7
.l1FAA:
    mov a,#$01                        ; 1FAA
    mov.w $03E8+x,a                   ; 1FAC
.l1FAF:
    mov a,#$00                        ; 1FAF
    mov.w $0478+x,a                   ; 1FB1
    pop a                             ; 1FB4
    mov.w $0418+x,a                   ; 1FB5
    cmp a,#$7F                        ; 1FB8
    bne .l1FC0                        ; 1FBA
    mov a,#$01                        ; 1FBC
    bra .l1FC2                        ; 1FBE
.l1FC0:
    mov a,#$00                        ; 1FC0
.l1FC2:
    mov.w $0420+x,a                   ; 1FC2
    mov a,y                           ; 1FC5
    mov.w $0400+x,a                   ; 1FC6
    jmp Spc_1CED                      ; 1FC9
.l1FCC:
    push y                            ; 1FCC
    push x                            ; 1FCD
    clrc                              ; 1FCE
    adc a,#$60                        ; 1FCF
    mov y,#$00                        ; 1FD1
    mov x,#$0C                        ; 1FD3
    div ya,x                          ; 1FD5
    push a                            ; 1FD6
    mov a,y                           ; 1FD7
    asl a                             ; 1FD8
    mov x,a                           ; 1FD9
    mov.w a,$CAA8+x                   ; 1FDA
    mov.b $22,a                       ; 1FDD
    mov.w a,$CAA9+x                   ; 1FDF
    mov.b $23,a                       ; 1FE2
    pop a                             ; 1FE4
    setc                              ; 1FE5
    sbc a,#$07                        ; 1FE6
    mov.b $24,a                       ; 1FE8
    beq Spc_1FFE                      ; 1FEA
.l1FEC:
    mov y,#$00                        ; 1FEC
    mov.b a,$23                       ; 1FEE
    mov x,#$02                        ; 1FF0
    div ya,x                          ; 1FF2
    mov.b $23,a                       ; 1FF3
    mov.b a,$22                       ; 1FF5
    div ya,x                          ; 1FF7
    mov.b $22,a                       ; 1FF8
    inc.b $24                         ; 1FFA
    bne .l1FEC                        ; 1FFC

Spc_1FFE:
    pop x                             ; 1FFE
    mov.b a,$22                       ; 1FFF
    mov.w $03B8+x,a                   ; 2001
    mov.w $0498+x,a                   ; 2004
    mov.b a,$23                       ; 2007
    mov.w $03C0+x,a                   ; 2009
    mov.w $04A0+x,a                   ; 200C
    mov.w a,$0440+x                   ; 200F

Spc_2012:
    beq Spc_2048_l204F                ; 2012
    push x                            ; 2014
    mov.b $24,a                       ; 2015
    mov.b a,$23                       ; 2017
    setc                              ; 2019
    sbc a,#$10                        ; 201A
    bmi Spc_2027_l2030                ; 201C
    mov.b a,$22                       ; 201E
    mov.b y,$23                       ; 2020
    mov x,#$43                        ; 2022
    div ya,x                          ; 2024
    mov y,#$04                        ; 2025

Spc_2027:
    mul ya                            ; 2027
    mov x,#$FC                        ; 2028
    div ya,x                          ; 202A
    mov.b y,$24                       ; 202B
    mul ya                            ; 202D
    bra Spc_203C_l2045                ; 202E
.l2030:
    mov.b a,$22                       ; 2030
    mov.b y,$23                       ; 2032
    mov x,#$11                        ; 2034
    div ya,x                          ; 2036
    mov x,a                           ; 2037
    mov a,#$FF                        ; 2038
    mov y,#$00                        ; 203A

Spc_203C:
    div ya,x                          ; 203C
    mov x,a                           ; 203D
    mov y,#$00                        ; 203E
    mov.b a,$24                       ; 2040
    div ya,x                          ; 2042
    mov y,#$00                        ; 2043
.l2045:
    addw ya,$22                       ; 2045
    pop x                             ; 2047

Spc_2048:
    mov.w $03B8+x,a                   ; 2048
    mov a,y                           ; 204B
    mov.w $03C0+x,a                   ; 204C
.l204F:
    pop y                             ; 204F
.l2050:
    jmp $1F47                         ; 2050

Spc_2053:
    mov a,#$01                        ; 2053

Spc_2055:
    bra Spc_2057_l2059                ; 2055

Spc_2057:
    mov a,#$02                        ; 2057
.l2059:
    pop x                             ; 2059
    mov.w $0478+x,a                   ; 205A
.l205D:
    bra Spc_2048_l2050                ; 205D

Spc_205F:
    pop x                             ; 205F
    inc y                             ; 2060
    mov a,($20)+y                     ; 2061
    mov.w $03C8+x,a                   ; 2063
    bra Spc_2057_l205D                ; 2066

Spc_2068:
    push x                            ; 2068
    push y                            ; 2069
    mov.b $22,a                       ; 206A
    mov.w a,$04F0+x                   ; 206C
    cmp a,#$04                        ; 206F
    bne .l209A                        ; 2071
    mov.b a,$03                       ; 2073
    bne .l2078                        ; 2075
    inc a                             ; 2077  ; not reached in the traces so far
.l2078:
    mov y,#$03                        ; 2078
    mul ya                            ; 207A
    mov x,#$02                        ; 207B
    call Spc_CA89                     ; 207D
    mov.b $24,a                       ; 2080
    mov.b $25,y                       ; 2082
    mov a,#$80                        ; 2084
    mov y,#$00                        ; 2086
    addw ya,$24                       ; 2088
    push y                            ; 208A
    pop x                             ; 208B
    mov y,a                           ; 208C
    mov.b a,$22                       ; 208D
    call Spc_CA75                     ; 208F
    mov a,y                           ; 2092
    bpl .l2097                        ; 2093
    mov a,#$7F                        ; 2095  ; not reached in the traces so far
.l2097:
    pop y                             ; 2097
    pop x                             ; 2098
    ret                               ; 2099
.l209A:
    mov.b a,$22                       ; 209A
    bra .l2097                        ; 209C

Spc_209E:
    set1 $2C.6                        ; 209E

Spc_20A0:
    pop x                             ; 20A0
    inc y                             ; 20A1
    mov.w a,$04F0+x                   ; 20A2
    cmp a,#$01                        ; 20A5
    beq .l20AD                        ; 20A7
    cmp a,#$02                        ; 20A9
    beq .l20CA                        ; 20AB
.l20AD:
    mov a,($20)+y                     ; 20AD
    call Spc_2068                     ; 20AF
    mov.w $0398+x,a                   ; 20B2
    mov.w $04A8+x,a                   ; 20B5
    bbs $2C.6,.l20BC                  ; 20B8
    inc y                             ; 20BB
.l20BC:
    mov a,($20)+y                     ; 20BC
    call Spc_2068                     ; 20BE
    mov.w $03A0+x,a                   ; 20C1
    mov.w $04B0+x,a                   ; 20C4
    jmp $20DE                         ; 20C7
.l20CA:
    mov a,($20)+y                     ; 20CA
    mov.w $0398+x,a                   ; 20CC
    mov.w $04A8+x,a                   ; 20CF
    bbs $2C.6,.l20D6                  ; 20D2
    inc y                             ; 20D5  ; not reached in the traces so far
.l20D6:
    mov a,($20)+y                     ; 20D6
    mov.w $03A0+x,a                   ; 20D8
    mov.w $04B0+x,a                   ; 20DB
.l20DE:
    mov a,#$00                        ; 20DE
    mov.w $03A8+x,a                   ; 20E0
    mov.w $03B0+x,a                   ; 20E3
    clr1 $2C.6                        ; 20E6
.l20E8:
    jmp $1F47                         ; 20E8

Spc_20EB:
    pop x                             ; 20EB
    inc y                             ; 20EC
    mov a,($20)+y                     ; 20ED
    and a,#$0F                        ; 20EF
    mov.b $22,a                       ; 20F1
    mov.w a,$03D0+x                   ; 20F3
    and a,#$F0                        ; 20F6
    bra Spc_20FA_l210B                ; 20F8

Spc_20FA:
    pop x                             ; 20FA
    inc y                             ; 20FB
    mov a,($20)+y                     ; 20FC
    and a,#$07                        ; 20FE
    asl a                             ; 2100
    asl a                             ; 2101
    asl a                             ; 2102
    asl a                             ; 2103
    mov.b $22,a                       ; 2104
    mov.w a,$03D0+x                   ; 2106
    and a,#$8F                        ; 2109
.l210B:
    or.b a,$22                        ; 210B
    or a,#$80                         ; 210D

Spc_210F:
    mov.w $03D0+x,a                   ; 210F
.l2112:
    bra Spc_20A0_l20E8                ; 2112

Spc_2114:
    pop x                             ; 2114
    inc y                             ; 2115
    mov a,($20)+y                     ; 2116
    and a,#$07                        ; 2118
    asl a                             ; 211A
    asl a                             ; 211B
    asl a                             ; 211C
    asl a                             ; 211D
    asl a                             ; 211E
    mov.b $22,a                       ; 211F
    mov.w a,$03D8+x                   ; 2121
    and a,#$1F                        ; 2124
    bra Spc_2128_l2135                ; 2126

Spc_2128:
    pop x                             ; 2128
    inc y                             ; 2129
    mov a,($20)+y                     ; 212A
    and a,#$1F                        ; 212C
    mov.b $22,a                       ; 212E
    mov.w a,$03D8+x                   ; 2130
    and a,#$E0                        ; 2133
.l2135:
    or.b a,$22                        ; 2135
    mov.w $03D8+x,a                   ; 2137
.l213A:
    bra Spc_210F_l2112                ; 213A

Spc_213C:
    pop x                             ; 213C
    inc y                             ; 213D
    mov a,($20)+y                     ; 213E
    mov.w $0428+x,a                   ; 2140
    inc y                             ; 2143
    mov a,($20)+y                     ; 2144
    mov.w $0430+x,a                   ; 2146
    mov a,#$01                        ; 2149
    mov.w $0438+x,a                   ; 214B
.l214E:
    bra Spc_2128_l213A                ; 214E

Spc_2150:
    pop x                             ; 2150
    bbs $28.0,Spc_215F_l2169          ; 2151
    call Spc_1E19                     ; 2154
    mov.b a,$4A                       ; 2157
    or.b a,$22                        ; 2159
    mov.b $4A,a                       ; 215B
    bra Spc_215F_l2169                ; 215D

Spc_215F:
    pop x                             ; 215F
    call Spc_1E28                     ; 2160
    mov.b a,$4A                       ; 2163
    and.b a,$22                       ; 2165
    mov.b $4A,a                       ; 2167
.l2169:
    bra Spc_213C_l214E                ; 2169

Spc_216B:
    inc y                             ; 216B
    mov a,($20)+y                     ; 216C
    mov.b $24,a                       ; 216E
    inc y                             ; 2170
    mov a,($20)+y                     ; 2171
    mov.b $25,a                       ; 2173
    push y                            ; 2175
    mov x,#$BC                        ; 2176
.l2178:
    mov.w a,$0702+x                   ; 2178
    cmp.b a,$24                       ; 217B
    beq .l2183                        ; 217D
    inc x                             ; 217F
    inc x                             ; 2180
    bra .l2178                        ; 2181
.l2183:
    inc x                             ; 2183
    mov.w a,$0702+x                   ; 2184
    cmp.b a,$25                       ; 2187
    beq .l2192                        ; 2189
    inc x                             ; 218B
    cmp x,#$00                        ; 218C
    beq .l2198                        ; 218E
    bra .l2178                        ; 2190
.l2192:
    mov a,x                           ; 2192
    lsr a                             ; 2193
    inc a                             ; 2194
    call Spc_219C                     ; 2195
.l2198:
    pop y                             ; 2198
    pop x                             ; 2199
    bra Spc_215F_l2169                ; 219A

Spc_219C:
    mov.b $23,a                       ; 219C
    push a                            ; 219E
    mov y,#$00                        ; 219F
    mov a,($24)+y                     ; 21A1
    mov x,a                           ; 21A3
    mov.w a,$04F0+x                   ; 21A4
    cmp a,#$02                        ; 21A7
    beq .l21C6                        ; 21A9
    cmp a,#$03                        ; 21AB
    beq .l21C6                        ; 21AD
    call Spc_1C9E                     ; 21AF
    call Spc_1CA5                     ; 21B2
    mov.b a,$24                       ; 21B5
    mov.w $0488+x,a                   ; 21B7
    mov.b a,$25                       ; 21BA
    mov.w $0490+x,a                   ; 21BC
    call Spc_1C84                     ; 21BF
    pop a                             ; 21C2
    jmp $1D53                         ; 21C3
.l21C6:
    pop a                             ; 21C6  ; not reached in the traces so far
    ret                               ; 21C7  ; not reached in the traces so far

Spc_21C8:
    pop x                             ; 21C8
    mov.w a,$0420+x                   ; 21C9
    bne .l21D1                        ; 21CC
    call Spc_194C                     ; 21CE
.l21D1:
    ret                               ; 21D1

Spc_21D2:
    pop x                             ; 21D2
    inc y                             ; 21D3
    mov a,($20)+y                     ; 21D4
    mov.w $0500+x,a                   ; 21D6
    inc y                             ; 21D9
    mov a,($20)+y                     ; 21DA
    mov.w $0508+x,a                   ; 21DC
    inc y                             ; 21DF
    mov a,($20)+y                     ; 21E0
    mov.w $0510+x,a                   ; 21E2
    inc y                             ; 21E5
    mov a,($20)+y                     ; 21E6
    mov.w $0518+x,a                   ; 21E8
    mov a,#$01                        ; 21EB
    mov.w $0520+x,a                   ; 21ED
.l21F0:
    jmp $1F47                         ; 21F0

Spc_21F3:
    pop x                             ; 21F3
    inc y                             ; 21F4
    mov a,($20)+y                     ; 21F5
    mov.w $0440+x,a                   ; 21F7
.l21FA:
    bra Spc_21D2_l21F0                ; 21FA

Spc_21FC:
    pop x                             ; 21FC
    mov a,#$00                        ; 21FD
    mov.w $0438+x,a                   ; 21FF
.l2202:
    bra Spc_21F3_l21FA                ; 2202

Spc_2204:
    pop x                             ; 2204
    mov a,#$00                        ; 2205
    mov.w $0520+x,a                   ; 2207
.l220A:
    bra Spc_21FC_l2202                ; 220A

Spc_220C:
    pop x                             ; 220C
    inc y                             ; 220D
    mov a,($20)+y                     ; 220E
    push a                            ; 2210
    inc y                             ; 2211
    mov a,($20)+y                     ; 2212
    mov.b $21,a                       ; 2214
    pop a                             ; 2216
    mov.b $20,a                       ; 2217
    mov y,#$00                        ; 2219
    jmp $1F48                         ; 221B

Spc_221E:
    pop x                             ; 221E
    inc y                             ; 221F
    mov a,($20)+y                     ; 2220
    mov.w $0448+x,a                   ; 2222
.l2225:
    bra Spc_2204_l220A                ; 2225

Spc_2227:
    pop x                             ; 2227
    call Spc_1E93                     ; 2228
    mov a,#$02                        ; 222B
    mov.w $0478+x,a                   ; 222D
.l2230:
    bra Spc_221E_l2225                ; 2230

Spc_2232:
    pop x                             ; 2232
    inc y                             ; 2233
    mov a,($20)+y                     ; 2234
    mov.w $04F0+x,a                   ; 2236
.l2239:
    bra Spc_2227_l2230                ; 2239

Spc_223B:
    pop x                             ; 223B
    inc y                             ; 223C
    mov a,($20)+y                     ; 223D
    and a,#$FE                        ; 223F
    push y                            ; 2241
    push x                            ; 2242

Spc_2243:
    call Spc_EDF0                     ; 2243
    pop x                             ; 2246
    pop y                             ; 2247
    bra Spc_2232_l2239                ; 2248

Spc_224A:
    mov.w a,$0438+x                   ; 224A
    beq .l227A                        ; 224D
    mov.w a,$0428+x                   ; 224F
    mov.b $20,a                       ; 2252
    mov.w a,$0430+x                   ; 2254
    mov.b $21,a                       ; 2257
    mov.w a,$04A0+x                   ; 2259
    mov y,a                           ; 225C
    mov.w a,$0498+x                   ; 225D
    addw ya,$20                       ; 2260
    mov.w $0498+x,a                   ; 2262
    mov a,y                           ; 2265
    mov.w $04A0+x,a                   ; 2266
    bmi .l227B                        ; 2269
    setc                              ; 226B
    sbc a,#$40                        ; 226C
    bpl .l2280                        ; 226E
    mov.w a,$0498+x                   ; 2270
.l2273:
    mov.w $03B8+x,a                   ; 2273
    mov a,y                           ; 2276
    mov.w $03C0+x,a                   ; 2277
.l227A:
    ret                               ; 227A
.l227B:
    mov a,#$00                        ; 227B
    mov y,a                           ; 227D
    bra .l2273                        ; 227E
.l2280:
    mov a,#$FF                        ; 2280  ; not reached in the traces so far
    mov y,#$3F                        ; 2282  ; not reached in the traces so far
    bra .l2273                        ; 2284  ; not reached in the traces so far
.l2286:
    mov.w a,$0520+x                   ; 2286
    beq .l22AF                        ; 2289
    mov.w a,$0500+x                   ; 228B
    mov.b $20,a                       ; 228E
    mov.w a,$0508+x                   ; 2290
    mov.b $21,a                       ; 2293
    call Spc_22B0                     ; 2295
    mov.w a,$0510+x                   ; 2298
    mov.b $20,a                       ; 229B
    mov.w a,$0518+x                   ; 229D
    mov.b $21,a                       ; 22A0
    mov a,x                           ; 22A2
    clrc                              ; 22A3
    adc a,#$08                        ; 22A4
    mov x,a                           ; 22A6
    call Spc_22B0                     ; 22A7
    mov a,x                           ; 22AA
    setc                              ; 22AB
    sbc a,#$08                        ; 22AC
    mov x,a                           ; 22AE
.l22AF:
    ret                               ; 22AF

Spc_22B0:
    mov.w a,$04A8+x                   ; 22B0
    mov y,a                           ; 22B3
    mov.w a,$03A8+x                   ; 22B4
    addw ya,$20                       ; 22B7
    mov.w $03A8+x,a                   ; 22B9
    mov a,y                           ; 22BC
    mov.w $04A8+x,a                   ; 22BD
    bmi .l22C4                        ; 22C0
    bra .l22CF                        ; 22C2
.l22C4:
    setc                              ; 22C4
    sbc a,#$C0                        ; 22C5
    bmi .l22CD                        ; 22C7
    mov a,#$00                        ; 22C9
    bra .l22CF                        ; 22CB
.l22CD:
    mov a,#$7F                        ; 22CD  ; not reached in the traces so far
.l22CF:
    mov.w $0398+x,a                   ; 22CF
    ret                               ; 22D2
.l22D3:
    mov a,x                           ; 22D3
    mov y,#$10                        ; 22D4
    mul ya                            ; 22D6
    mov y,a                           ; 22D7
    push y                            ; 22D8
    mov.w a,$0408+x                   ; 22D9
    beq .l22E8                        ; 22DC
    mov.w a,$04C0+x                   ; 22DE
    beq .l22E8                        ; 22E1
    call Spc_2336                     ; 22E3
    bra .l22EB                        ; 22E6
.l22E8:
    mov.w a,$0398+x                   ; 22E8
.l22EB:
    call Spc_23B2                     ; 22EB
    pop y                             ; 22EE
    call Spc_09E8                     ; 22EF

Spc_22F2:
    inc y                             ; 22F2
    push y                            ; 22F3
    mov.w a,$0408+x                   ; 22F4
    beq .l2309                        ; 22F7
    mov.w a,$04C8+x                   ; 22F9
    beq .l2309                        ; 22FC
    call Spc_CA96                     ; 22FE
    call Spc_2336                     ; 2301
    call Spc_CA9F                     ; 2304
    bra .l230C                        ; 2307
.l2309:
    mov.w a,$03A0+x                   ; 2309
.l230C:
    call Spc_23B2                     ; 230C
    pop y                             ; 230F
    call Spc_09E8                     ; 2310
    inc y                             ; 2313
    mov.w a,$03B8+x                   ; 2314
    call Spc_09E8                     ; 2317
    inc y                             ; 231A
    mov.w a,$03C0+x                   ; 231B

Spc_231E:
    call Spc_09E8                     ; 231E
    inc y                             ; 2321
    mov.w a,$03C8+x                   ; 2322
    call Spc_09E8                     ; 2325
    inc y                             ; 2328
    mov.w a,$03D0+x                   ; 2329
    call Spc_09E8                     ; 232C
    inc y                             ; 232F
    mov.w a,$03D8+x                   ; 2330
    jmp Spc_09E8                      ; 2333

Spc_2336:
    cmp a,#$01                        ; 2336
    beq .l2348                        ; 2338
    cmp a,#$3C                        ; 233A
    beq Spc_235D_l2385                ; 233C
    cmp a,#$FF                        ; 233E
    beq Spc_235D_l23AD                ; 2340
    inc a                             ; 2342
    mov.w $04C0+x,a                   ; 2343
    bra Spc_235D_l2361                ; 2346
.l2348:
    inc a                             ; 2348
    mov.w $04C0+x,a                   ; 2349
    mov.w a,$0398+x                   ; 234C
    mov y,a                           ; 234F
    mov.w a,$03A8+x                   ; 2350
    push x                            ; 2353
    mov x,#$3C                        ; 2354
    call Spc_CA89                     ; 2356
    pop x                             ; 2359
    mov.w $04D0+x,a                   ; 235A

Spc_235D:
    mov a,y                           ; 235D
    mov.w $04E0+x,a                   ; 235E
.l2361:
    mov.w a,$04C0+x                   ; 2361
    dec a                             ; 2364
    mov.b $22,a                       ; 2365
    mov.w a,$04D0+x                   ; 2367
    mov y,a                           ; 236A
    mov.w a,$04E0+x                   ; 236B
    push x                            ; 236E
    mov x,a                           ; 236F
    mov.b a,$22                       ; 2370
    call Spc_CA75                     ; 2372
    pop x                             ; 2375
    mov.b $22,a                       ; 2376
    mov.b $23,y                       ; 2378
    mov.w a,$0398+x                   ; 237A
    mov y,a                           ; 237D
    mov.w a,$03A8+x                   ; 237E
    subw ya,$22                       ; 2381
    mov a,y                           ; 2383
    ret                               ; 2384
.l2385:
    bbs $2C.7,.l238D                  ; 2385
    bbs $2D.0,.l2390                  ; 2388
    bra .l2393                        ; 238B
.l238D:
    jmp $1AE9                         ; 238D
.l2390:
    jmp $1B15                         ; 2390  ; not reached in the traces so far
.l2393:
    mov.b $22,x                       ; 2393
    bbs $22.3,.l23A2                  ; 2395
    call Spc_194C                     ; 2398
.l239B:
    mov a,#$FF                        ; 239B
    mov.w $04C0+x,a                   ; 239D
    inc a                             ; 23A0
    ret                               ; 23A1
.l23A2:
    call Spc_CA9F                     ; 23A2
    call Spc_194C                     ; 23A5
    call Spc_CA96                     ; 23A8
    bra .l239B                        ; 23AB
.l23AD:
    inc a                             ; 23AD  ; not reached in the traces so far
    mov.w $04C0+x,a                   ; 23AE  ; not reached in the traces so far
    ret                               ; 23B1  ; not reached in the traces so far

Spc_23B2:
    push y                            ; 23B2
    mov.b $22,a                       ; 23B3
    cmp a,#$00                        ; 23B5
    beq .l23C9                        ; 23B7
    mov.w a,$0390+x                   ; 23B9
    cmp a,#$25                        ; 23BC
    bne .l23CB                        ; 23BE
    mov.w a,$0530                     ; 23C0
    mov y,#$04                        ; 23C3
    mul ya                            ; 23C5
    clrc                              ; 23C6
    adc.b a,$22                       ; 23C7
.l23C9:
    pop y                             ; 23C9
    ret                               ; 23CA
.l23CB:
    mov.b a,$22                       ; 23CB
    bra .l23C9                        ; 23CD

Spc_23CF:
    mov.w a,$0390+x                   ; 23CF
    mov.b $20,a                       ; 23D2
    push x                            ; 23D4
    mov x,#$00                        ; 23D5
.l23D7:
    mov.w a,$23EB+x                   ; 23D7
    beq .l23E3                        ; 23DA
    cmp.b a,$20                       ; 23DC
    beq .l23E5                        ; 23DE
    inc x                             ; 23E0
    bra .l23D7                        ; 23E1
.l23E3:
    pop x                             ; 23E3
    ret                               ; 23E4
.l23E5:
    mov a,x                           ; 23E5
    asl a                             ; 23E6
    mov x,a                           ; 23E7
    jmp ($23FB+x)                     ; 23E8
    db $07, $04, $01, $0A, $0D, $74, $73, $72
    db $75, $76, $17, $25, $21, $02, $58, $00
    db $41, $24, $41, $24, $41, $24, $41, $24
    db $41, $24, $41, $24, $41, $24, $41, $24
    db $41, $24, $41, $24, $2A, $26, $1F, $26
    db $E2, $25, $5C, $26, $5C, $26, $0C, $13
    db $55, $7F, $00, $00, $00, $00, $00, $00
    db $23, $1A, $17, $17, $16, $0D, $11, $11
    db $14, $20, $0C, $0B, $0E, $07, $06, $0C
    db $0B, $0E, $07, $06, $1B, $1E, $1F, $12
    db $0F, $1B, $1E, $1F, $12, $0F

Spc_2441:
    lsr a                             ; 2441
    mov x,a                           ; 2442
    mov.w a,$2419+x                   ; 2443
    mov.b $22,a                       ; 2446
    mov.w a,$2423+x                   ; 2448
    mov.b $23,a                       ; 244B
    mov.w a,$242D+x                   ; 244D
    mov.b $24,a                       ; 2450
    mov.w a,$2437+x                   ; 2452
    mov.b $25,a                       ; 2455
    pop x                             ; 2457
    call Spc_25A0                     ; 2458
    push x                            ; 245B
    cmp $0B,$03                       ; 245C
    beq .l24A9                        ; 245F
    mov a,#$00                        ; 2461
    mov.w $0539,a                     ; 2463
    mov.b a,$03                       ; 2466
    mov.w $053B,a                     ; 2468
    setc                              ; 246B
    sbc.b a,$0B                       ; 246C
    bmi .l249E                        ; 246E
    push a                            ; 2470
    mov a,#$01                        ; 2471
    mov.w $053D,a                     ; 2473
.l2476:
    pop a                             ; 2476
    mov y,a                           ; 2477
    mov.w x,$0533                     ; 2478
    bne .l247F                        ; 247B
    mov x,#$01                        ; 247D  ; not reached in the traces so far
.l247F:
    mov a,#$00                        ; 247F
    div ya,x                          ; 2481
    bvs .l248E                        ; 2482
    mov.w $0535,a                     ; 2484
    mov a,#$00                        ; 2487
    mov.w $0537,a                     ; 2489
    bra .l2496                        ; 248C
.l248E:
    mov.w $0535,a                     ; 248E
    mov a,#$01                        ; 2491
    mov.w $0537,a                     ; 2493
.l2496:
    mov a,#$00                        ; 2496
    mov.w $0533,a                     ; 2498
    jmp $254F                         ; 249B
.l249E:
    eor a,#$FF                        ; 249E
    inc a                             ; 24A0
    push a                            ; 24A1
    mov a,#$02                        ; 24A2
    mov.w $053D,a                     ; 24A4
    bra .l2476                        ; 24A7
.l24A9:
    mov.b a,$03                       ; 24A9
    bne .l24B7                        ; 24AB
    mov a,#$00                        ; 24AD
    mov.w $0535,a                     ; 24AF
    mov.w $0536,a                     ; 24B2
    bra .l24DC                        ; 24B5
.l24B7:
    mov.b a,$03                       ; 24B7
    bpl .l24DC                        ; 24B9
    setc                              ; 24BB
    sbc a,#$F0                        ; 24BC
    bmi .l24DC                        ; 24BE
    mov.w a,$056D                     ; 24C0
    mov.b $22,a                       ; 24C3
    mov.w a,$056F                     ; 24C5
    mov.b $23,a                       ; 24C8
    incw $22                          ; 24CA
    incw $22                          ; 24CC
    incw $22                          ; 24CE
    mov.b a,$22                       ; 24D0
    mov.w $056D,a                     ; 24D2
    mov.b a,$23                       ; 24D5
    mov.w $056F,a                     ; 24D7
    bra .l24E4                        ; 24DA
.l24DC:
    mov a,#$00                        ; 24DC
    mov.w $056D,a                     ; 24DE
    mov.w $056F,a                     ; 24E1
.l24E4:
    mov.w a,$0533                     ; 24E4
    setc                              ; 24E7
    sbc a,#$0A                        ; 24E8
    bmi .l2506                        ; 24EA
    mov $22,#$01                      ; 24EC
    mov $23,#$00                      ; 24EF
    mov.w y,$0537                     ; 24F2
    mov.w a,$0535                     ; 24F5
    subw ya,$22                       ; 24F8
    bpl .l2500                        ; 24FA
    mov a,#$00                        ; 24FC
    mov y,#$00                        ; 24FE
.l2500:
    mov.w $0535,a                     ; 2500
    mov.w $0537,y                     ; 2503
.l2506:
    mov.w a,$0533                     ; 2506
    cmp a,#$FF                        ; 2509
    beq .l2514                        ; 250B
    mov.w a,$0533                     ; 250D
    inc a                             ; 2510
    mov.w $0533,a                     ; 2511
.l2514:
    mov.w a,$0533                     ; 2514
    cmp a,#$01                        ; 2517
    beq .l2548                        ; 2519
    mov.w y,$053B                     ; 251B
    mov.w a,$0539                     ; 251E
.l2521:
    push a                            ; 2521
    mov.w a,$0535                     ; 2522
    mov.b $20,a                       ; 2525
    mov.w a,$0537                     ; 2527
    mov.b $21,a                       ; 252A
    mov.w a,$053D                     ; 252C
    cmp a,#$02                        ; 252F
    beq .l2541                        ; 2531
    pop a                             ; 2533
    addw ya,$20                       ; 2534
    bcs .l254F                        ; 2536
.l2538:
    mov.w $0539,a                     ; 2538
    mov a,y                           ; 253B
    mov.w $053B,a                     ; 253C
    bra .l254F                        ; 253F
.l2541:
    pop a                             ; 2541
    subw ya,$20                       ; 2542
    bcc .l254F                        ; 2544
    bra .l2538                        ; 2546
.l2548:
    mov.b a,$03                       ; 2548
    mov y,a                           ; 254A
    mov a,#$00                        ; 254B
    bra .l2521                        ; 254D
.l254F:
    mov $20,#$00                      ; 254F
    mov $21,$24                       ; 2552
    mov.b y,$25                       ; 2555
    mov.w a,$053B                     ; 2557
    mul ya                            ; 255A
    mov $20,#$00                      ; 255B
    addw ya,$20                       ; 255E
    mov.b $20,a                       ; 2560
    mov.b $21,y                       ; 2562
    mov.w a,$0549                     ; 2564
    mov.w y,$054B                     ; 2567
    addw ya,$20                       ; 256A
    mov.b $20,a                       ; 256C
    mov.b $21,y                       ; 256E
    mov.w a,$054D                     ; 2570
    mov.w y,$054F                     ; 2573
    addw ya,$20                       ; 2576
    mov.b $20,a                       ; 2578
    mov.b $21,y                       ; 257A
    mov.w a,$056D                     ; 257C
    mov.w y,$056F                     ; 257F
    addw ya,$20                       ; 2582
    mov.b $20,a                       ; 2584
    mov.b $21,y                       ; 2586
    setc                              ; 2588
    sbc $21,#$3F                      ; 2589
    bmi .l2591                        ; 258C
    mov $21,#$3F                      ; 258E  ; not reached in the traces so far
.l2591:
    mov $0B,$03                       ; 2591
    pop x                             ; 2594
    mov.b a,$20                       ; 2595
    mov.w $03B8+x,a                   ; 2597
    mov.b a,$21                       ; 259A
    mov.w $03C0+x,a                   ; 259C
    ret                               ; 259F

Spc_25A0:
    mov.b a,$03                       ; 25A0
    eor a,#$FF                        ; 25A2
    mov y,#$00                        ; 25A4
    push x                            ; 25A6
    mov.b x,$22                       ; 25A7
    div ya,x                          ; 25A9
    mov.b $20,a                       ; 25AA
    pop x                             ; 25AC
    mov.b a,$23                       ; 25AD
    clrc                              ; 25AF
    adc.b a,$20                       ; 25B0
    bbc $28.0,Spc_25B8_l25BB          ; 25B2
    mov.w y,$0552                     ; 25B5

Spc_25B8:
    dec y                             ; 25B8
    mul ya                            ; 25B9
    mov a,y                           ; 25BA
.l25BB:
    bbc $29.2,.l25C0                  ; 25BB
    adc a,#$03                        ; 25BE
.l25C0:
    call Spc_25D8                     ; 25C0
    mov.w y,$056B                     ; 25C3
    mul ya                            ; 25C6
    mov a,y                           ; 25C7
    bbc $2D.4,.l25D1                  ; 25C8
    push x                            ; 25CB
    mov y,#$C0                        ; 25CC
    mul ya                            ; 25CE
    mov a,y                           ; 25CF
    pop x                             ; 25D0
.l25D1:
    mov.w $0398+x,a                   ; 25D1
    mov.w $03A0+x,a                   ; 25D4
    ret                               ; 25D7

Spc_25D8:
    bbs $28.2,.l25E1                  ; 25D8
    push y                            ; 25DB
    mov y,#$D0                        ; 25DC
    mul ya                            ; 25DE
    mov a,y                           ; 25DF
    pop y                             ; 25E0
.l25E1:
    ret                               ; 25E1

Spc_25E2:
    pop x                             ; 25E2
    mov $22,#$28                      ; 25E3
    mov $23,#$03                      ; 25E6
    mov $24,#$02                      ; 25E9
    call Spc_2634                     ; 25EC
    mov.w a,$03E8+x                   ; 25EF
    cmp a,#$02                        ; 25F2
    beq .l25FB                        ; 25F4
.l25F6:
    bbs $28.6,Spc_2600_l2614          ; 25F6
    bra Spc_2600_l2609                ; 25F9
.l25FB:
    call Spc_2600                     ; 25FB
    bra .l25F6                        ; 25FE

Spc_2600:
    bbs $28.6,.l2606                  ; 2600
    set1 $28.6                        ; 2603
    ret                               ; 2605
.l2606:
    clr1 $28.6                        ; 2606
    ret                               ; 2608
.l2609:
    mov a,#$80                        ; 2609
    mov.w $03B8+x,a                   ; 260B
    mov a,#$01                        ; 260E
    mov.w $03C0+x,a                   ; 2610
    ret                               ; 2613
.l2614:
    mov a,#$10                        ; 2614
    mov.w $03B8+x,a                   ; 2616
    mov a,#$01                        ; 2619
    mov.w $03C0+x,a                   ; 261B
    ret                               ; 261E

Spc_261F:
    mov $22,#$1E                      ; 261F
    mov $23,#$03                      ; 2622
    mov $24,#$01                      ; 2625
    bra Spc_262A_l2633                ; 2628

Spc_262A:
    mov $22,#$14                      ; 262A
    mov $23,#$0A                      ; 262D
    mov $24,#$02                      ; 2630
.l2633:
    pop x                             ; 2633

Spc_2634:
    mov.b a,$03                       ; 2634
    eor a,#$FF                        ; 2636
    mov y,#$00                        ; 2638
    push x                            ; 263A
    mov.b x,$22                       ; 263B
    div ya,x                          ; 263D
    pop x                             ; 263E
    clrc                              ; 263F
    adc.b a,$23                       ; 2640
    push a                            ; 2642
    mov.w a,$0545                     ; 2643
    inc a                             ; 2646
    mov.w $0545,a                     ; 2647
    pop a                             ; 264A
    setc                              ; 264B
    sbc.w a,$0545                     ; 264C
    bpl .l265B                        ; 264F
    mov.b a,$24                       ; 2651
    mov.w $03E8+x,a                   ; 2653
    mov a,#$01                        ; 2656
    mov.w $0545,a                     ; 2658
.l265B:
    ret                               ; 265B

Spc_265C:
    pop x                             ; 265C
    mov.b a,$02                       ; 265D
    and a,#$07                        ; 265F
    bbs $02.3,.l2666                  ; 2661
    eor a,#$07                        ; 2664
.l2666:
    asl a                             ; 2666
    mov y,a                           ; 2667
    mov.w a,$EE20+y                   ; 2668
    mov.b $20,a                       ; 266B
    inc y                             ; 266D
    mov.w a,$EE20+y                   ; 266E
    mov.b $21,a                       ; 2671
    mov.w a,$04A0+x                   ; 2673
    mov y,a                           ; 2676
    mov.w a,$0498+x                   ; 2677
    bbs $02.3,.l2681                  ; 267A
    subw ya,$20                       ; 267D
    bra .l2683                        ; 267F
.l2681:
    addw ya,$20                       ; 2681
.l2683:
    mov.w $03B8+x,a                   ; 2683
    mov a,y                           ; 2686
    mov.w $03C0+x,a                   ; 2687
    mov.b a,$02                       ; 268A
    and a,#$F0                        ; 268C
    xcn a                             ; 268E
    bbs $02.7,.l26A8                  ; 268F
    asl a                             ; 2692
    mov y,a                           ; 2693
    mov.w a,$EE30+y                   ; 2694
    call Spc_26C0                     ; 2697
    mov.w $0398+x,a                   ; 269A
    inc y                             ; 269D
    mov.w a,$EE30+y                   ; 269E
    call Spc_26C0                     ; 26A1
    mov.w $03A0+x,a                   ; 26A4
    ret                               ; 26A7
.l26A8:
    eor a,#$0F                        ; 26A8
    asl a                             ; 26AA
    mov y,a                           ; 26AB
    mov.w a,$EE30+y                   ; 26AC
    call Spc_26C0                     ; 26AF
    mov.w $03A0+x,a                   ; 26B2
    inc y                             ; 26B5
    mov.w a,$EE30+y                   ; 26B6
    call Spc_26C0                     ; 26B9
    mov.w $0398+x,a                   ; 26BC
    ret                               ; 26BF

Spc_26C0:
    push y                            ; 26C0
    push x                            ; 26C1
    mov.w y,$0562                     ; 26C2
    inc y                             ; 26C5
    mul ya                            ; 26C6
    mov x,#$15                        ; 26C7
    div ya,x                          ; 26C9
    pop x                             ; 26CA
    pop y                             ; 26CB
    mov $22,$02                       ; 26CC
    and $22,#$0F                      ; 26CF
    setc                              ; 26D2
    sbc.b a,$22                       ; 26D3
    ret                               ; 26D5
    db $03, $32, $37, $30, $3C, $16, $20, $54
    db $31, $57, $3D, $35, $39, $00, $5A, $4D
    db $4E, $2D, $2E, $0B, $0C, $29, $33, $43
    db $30, $3C, $38, $3E, $56, $31, $2C, $57
    db $3D, $16, $20, $35, $39, $00

%spc($BCA0, $19E3EE)
    db $04, $E3, $04, $FB, $05, $FE, $06, $EA
    db $E9, $9C, $FF, $D1, $E8, $0D, $2D, $E0
    db $04, $EC, $C9, $BC, $E3, $0F, $FE, $32
    db $E5, $0D, $E9, $BC, $02, $80, $0A, $E3
    db $0D, $E8, $10, $E9, $FE, $FF, $B0, $3A
    db $E0, $03, $E4, $00, $50, $E3, $23, $FD
    db $96, $E8, $14, $ED, $4C, $04, $A8, $FD
    db $E9, $38, $FF, $C3, $0A, $E8, $10, $F2
    db $E9, $FE, $FF, $E3, $0A, $B7, $30, $E0
    db $04, $EC, $05, $BD, $E4, $23, $00, $E6
    db $03, $E7, $00, $EA, $E3, $03, $E8, $14
    db $C2, $04, $C4, $04, $C6, $05, $E6, $01
    db $E8, $10, $C7, $1E, $E0, $03, $EE, $0A
    db $E3, $03, $FB, $07, $E4, $00, $25, $E5
    db $0E, $F3, $EF, $BC, $06, $FE, $12, $E3
    db $23, $FD, $C2, $E5, $08, $E9, $F1, $FF
    db $C0, $3C, $E2, $E9, $FA, $FF, $3C, $E2
    db $E8, $10, $1E, $E0, $04, $FE, $1E, $E3
    db $03, $C8, $E9, $64, $00, $C8, $0E, $C8
    db $0E, $C8, $0E, $C8, $0E, $E0, $06, $FE
    db $32, $E3, $0A, $ED, $FA, $00, $FA, $00
    db $9E, $02, $05, $9B, $04, $02, $9D, $06
    db $0A, $9B, $05, $02, $9E, $04, $03, $9B
    db $06, $0A, $9D, $08, $03, $9B, $06, $02
    db $9E, $06, $0A, $9B, $04, $02, $9D, $04
    db $03, $A0, $06, $0A, $F2, $FE, $64, $A8
    db $05, $32, $E0, $04, $FC, $01, $E4, $4B
    db $14, $E3, $0F, $E5, $0C, $A4, $04, $9F
    db $05, $E5, $0F, $E9, $6A, $FF, $F8, $1E
    db $E3, $0D, $AA, $03, $E2, $E3, $0F, $A4
    db $05, $E2, $E3, $0D, $A9, $02, $E2, $E3
    db $0F, $A5, $04, $E2, $E3, $0D, $A7, $02
    db $E2, $E3, $0F, $AA, $05, $E2, $F3, $88
    db $BD, $04, $FC, $01, $E4, $14, $4B, $F3
    db $79, $BD, $06, $E3, $12, $E5, $0A, $E9
    db $3C, $00, $AB, $06, $E2, $FE, $28, $19
    db $E0, $06, $EA, $E3, $0E, $FB, $14, $E4
    db $5A, $00, $E9, $0A, $00, $AA, $03, $AB
    db $07, $AB, $09, $E9, $FD, $FF, $1D, $E9
    db $FD, $FF, $E8, $15, $A9, $12, $E0, $06
    db $FE, $11, $E3, $04, $E5, $01, $CC, $02
    db $E2, $E9, $EC, $FF, $CC, $6A, $FB, $01
    db $FE, $3C, $E5, $0F, $F1, $E3, $0F, $A4
    db $05, $9F, $08, $E3, $0A, $A4, $5A, $E0
    db $06, $E4, $00, $64, $E3, $24, $E9, $0A
    db $00, $B4, $13, $B9, $0A, $FB, $01, $E4
    db $00, $14, $0A, $FB, $01, $E9, $E2, $FF
    db $E4, $00, $5A, $B5, $14, $E2, $F1, $E8
    db $10, $28, $E0, $03, $FC, $01, $EC, $41
    db $BE, $E3, $03, $EA, $EE, $14, $E5, $0E
    db $E8, $1A, $E4, $0A, $28, $C5, $06, $CC
    db $05, $E8, $13, $E4, $06, $1E, $D1, $3C
    db $E0, $04, $FC, $01, $E3, $03, $EA, $E5
    db $0E, $E8, $1A, $FB, $02, $E4, $28, $0A
    db $C9, $06, $E8, $13, $E4, $1E, $06, $D1
    db $3C, $E0, $05, $FC, $01, $FE, $32, $E3
    db $02, $A3, $05, $FB, $02, $1C, $E0, $05
    db $FC, $01, $FE, $37, $E5, $05, $E3, $10
    db $B9, $32, $E2, $BA, $28, $E2, $E8, $12
    db $B9, $1E, $E2, $B8, $1E, $E0, $04, $FE
    db $41, $E3, $0E, $E5, $0A, $E9, $1E, $00
    db $90, $12, $90, $11, $E0, $04, $FE, $28
    db $E3, $12, $E5, $0A, $E9, $3C, $00, $AB
    db $13, $AB, $16, $E0, $A7, $FC, $01, $E3
    db $1C, $FE, $2A, $CB, $EC, $BD, $BE, $E8
    db $0A, $E9, $20, $03, $0C, $E2, $E9, $E2
    db $FF, $7E, $E2, $3C, $E0, $A7, $FC, $01
    db $E3, $20, $F3, $A1, $BE, $A6, $FC, $01
    db $FE, $23, $E3, $0D, $E5, $08, $80, $E8
    db $0C, $E9, $B4, $00, $0A, $E2, $E9, $F0
    db $FF, $7E, $E0, $04, $FC, $01, $EC, $11
    db $BF, $EE, $0A, $E4, $3F, $0F, $E3, $03
    db $EA, $E6, $05, $E7, $02, $E8, $15, $E9
    db $0A, $00, $C3, $04, $CA, $04, $D1, $04
    db $D3, $06, $CF, $07, $CA, $08, $E8, $11
    db $CF, $08, $ED, $78, $EC, $78, $EC, $02
    db $F2, $D1, $04, $D3, $06, $CF, $07, $CA
    db $08, $E8, $14, $E9, $03, $00, $CF, $0C
    db $E0, $03, $FC, $01, $E3, $03, $FB, $0A
    db $EA, $E4, $0F, $3F, $E5, $0E, $F3, $E0
    db $BE, $04, $FC, $01, $EC, $42, $BF, $E4
    db $2B, $14, $AB, $F8, $41, $E3, $02, $E5
    db $09, $ED, $2E, $FF, $2E, $FF, $E9, $E4
    db $FD, $02, $E2, $E9, $F4, $01, $04, $F3
    db $36, $BF, $03, $FC, $01, $F8, $3C, $E4
    db $14, $2B, $E3, $12, $E9, $4B, $00, $84
    db $0A, $AF, $F3, $2D, $BF, $04, $FC, $01
    db $E3, $03, $E5, $07, $AE, $E9, $FA, $00
    db $FE, $5A, $0A, $AB, $FE, $4B, $0A, $AB
    db $FE, $28, $0A, $AB, $FE, $19, $0A, $AB
    db $FE, $0F, $0A, $E0, $04, $FE, $64, $E3
    db $0A, $AB, $01, $01, $A8, $01, $03, $E9
    db $FF, $FF, $9A, $7E, $E0, $01, $FC, $01
    db $FE, $19, $E3, $03, $E5, $0D, $C3, $14
    db $FB, $01, $FE, $07, $E5, $0F, $E8, $0F
    db $14, $E0, $00, $FC, $01, $FE, $19, $E3
    db $03, $E5, $0D, $CF, $3C, $FB, $01, $FE
    db $07, $E5, $0F, $E8, $0F, $28, $E0, $05
    db $EA, $E3, $24, $FE, $2B, $C7, $E9, $90
    db $01, $04, $E2, $F1, $1B, $E2, $E0, $85
    db $FC, $02, $FE, $0A, $E3, $0E, $B4, $02
    db $E2, $B6, $02, $E2, $B3, $02, $E2, $F3
    db $C6, $BF, $A5, $FC, $02, $FE, $1E, $E8
    db $08, $E9, $FE, $FF, $E3, $0E, $B8, $7E
    db $E0, $85, $FC, $03, $EA, $FE, $55, $E8
    db $19, $E3, $0F, $80, $32, $E0, $05, $FC
    db $02, $FE, $7F, $E3, $24, $E8, $0F, $B2
    db $1E, $E0, $85, $FC, $02, $FE, $50, $E5
    db $0D, $E3, $0D, $EA, $A4, $04, $E2, $EB
    db $A7, $03, $E2, $EA, $A9, $02, $E2, $EB
    db $E5, $08, $E9, $18, $FC, $A6, $1E, $F3
    db $04, $C0, $A7, $EC, $3B, $C0, $EC, $61
    db $C0, $FE, $40, $E3, $0D, $E5, $04, $E8
    db $0C, $E9, $FD, $FF, $B0, $78, $E2, $E9
    db $01, $00, $7E, $E2, $7E, $F1, $E5, $0F
    db $E8, $00, $E0, $A6, $EA, $E8, $10, $E3
    db $0F, $FE, $48, $BC, $03, $FE, $30, $B5
    db $07, $B4, $78, $E3, $0D, $E9, $05, $00
    db $FE, $23, $E5, $0D, $E8, $0B, $8B, $1E
    db $E2, $E9, $FE, $FF, $32, $E2, $F1, $4B
    db $E0, $04, $FC, $01, $E8, $0B, $E3, $0A
    db $FE, $40, $B7, $02, $02, $BB, $64, $FE
    db $00, $ED, $1E, $00, $00, $00, $E3, $04
    db $E1, $E6, $07, $E8, $01, $EA, $C7, $1A
    db $C3, $1A, $C7, $1A, $C3, $1A, $ED, $F1
    db $FF, $1E, $00, $C7, $1A, $C3, $1A, $EB
    db $E9, $F9, $FF, $ED, $F6, $FF, $50, $00
    db $C7, $1A, $C3, $1A, $E0, $04, $E3, $04
    db $E4, $08, $0F, $E5, $0A, $F8, $3F, $E9
    db $90, $01, $C3, $08, $E2, $E9, $70, $FE
    db $08, $E2, $F3, $A7, $C0, $04, $FC, $01
    db $E4, $20, $00, $E3, $0D, $EA, $E5, $04
    db $E8, $0D, $E9, $E2, $FF, $C8, $64, $E0
    db $04, $FC, $01, $E4, $00, $20, $F3, $BB
    db $C0, $04, $FC, $01, $E4, $32, $1E, $E8
    db $1C, $EA, $E3, $0F, $A4, $02, $E1, $A8
    db $03, $E8, $00, $E3, $0D, $E9, $D0, $07
    db $E1, $8C, $02, $E0, $04, $FC, $01, $E4
    db $1E, $32, $F3, $D7, $C0, $06, $E3, $25
    db $E4, $04, $20, $E5, $0C, $C3, $06, $E2
    db $E9, $28, $00, $0F, $E2, $F1, $E9, $D8
    db $FF, $0F, $E0, $04, $FC, $01, $EC, $38
    db $C1, $EE, $28, $E5, $0D, $E3, $03, $FB
    db $0C, $E4, $14, $28, $E3, $05, $EA, $E8
    db $12, $CA, $04, $CC, $04, $CF, $04, $D1
    db $05, $D3, $06, $E2, $ED, $60, $F0, $60
    db $F0, $02, $F2, $D1, $04, $D3, $04, $E0
    db $03, $FC, $01, $EE, $3C, $E4, $28, $14
    db $F3, $1C, $C1, $06, $FC, $01, $E4, $01
    db $1E, $ED, $1E, $00, $E2, $FF, $F3, $64
    db $C1, $06, $FC, $01, $E4, $1E, $01, $ED
    db $E2, $FF, $1E, $00, $F3, $64, $C1, $06
    db $FC, $01, $FE, $19, $EA, $E3, $03, $E6
    db $04, $E7, $04, $E8, $10, $C8, $06, $F3
    db $6E, $C1, $06, $E4, $01, $1E, $F3, $83
    db $C1, $06, $FE, $19, $F3, $83, $C1, $06
    db $E4, $1E, $01, $FC, $01, $EA, $E3, $03
    db $E6, $04, $E7, $04, $E8, $10, $C8, $32
    db $E0, $06, $EA, $FE, $1E, $E3, $23, $FD
    db $A6, $E5, $02, $E9, $FE, $FF, $CA, $7E
    db $E2, $E9, $F6, $FF, $7E, $E2, $28, $E2
    db $E9, $FF, $FF, $E8, $0A, $7E, $E2, $7E
    db $E0, $03, $FC, $01, $EC, $C9, $C1, $EA
    db $E3, $05, $EE, $32, $E8, $14, $E4, $20
    db $00, $C2, $07, $E4, $0B, $18, $CA, $18
    db $E0, $02, $FC, $01, $EA, $E3, $05, $EE
    db $32, $E8, $14, $FB, $02, $E4, $18, $0B
    db $C6, $08, $E4, $00, $32, $CE, $14, $E0
    db $02, $FC, $01, $EC, $F3, $C1, $EC, $DC
    db $C2, $EA, $E3, $05, $EE, $32, $E8, $14
    db $F3, $5C, $C2, $01, $FC, $01, $EA, $F3
    db $B3, $C2, $03, $FC, $01, $EC, $37, $C2
    db $EC, $87, $C2, $EA, $E3, $05, $EE, $32
    db $E8, $14, $E4, $00, $20, $C2, $07, $E4
    db $18, $0B, $CA, $18, $E5, $0E, $E4, $00
    db $0A, $C2, $08, $E4, $06, $03, $CA, $18
    db $E5, $0D, $E4, $00, $07, $C2, $04, $E4
    db $02, $04, $C6, $04, $E4, $04, $02, $CA
    db $04, $E4, $07, $00, $CE, $18, $E0, $02
    db $FC, $01, $EA, $E3, $05, $EE, $32, $E8
    db $14, $FB, $02, $E4, $0B, $18, $C6, $08
    db $E4, $32, $00, $CE, $18, $E5, $0E, $E4
    db $03, $06, $C6, $08, $E4, $0A, $00, $CE
    db $18, $EC, $DC, $C2, $E3, $02, $EE, $00
    db $E8, $14, $FE, $0A, $AB, $18, $A6, $18
    db $A4, $18, $A4, $10, $AB, $08, $AB, $10
    db $A4, $08, $A5, $08, $A6, $18, $AD, $18
    db $AB, $18, $AB, $10, $AB, $08, $A6, $10
    db $A8, $08, $AA, $08, $F3, $62, $C2, $01
    db $FC, $01, $EA, $FB, $04, $E3, $0B, $FE
    db $02, $C0, $08, $FE, $02, $BF, $08, $FE
    db $03, $C0, $08, $FE, $07, $BF, $08, $E3
    db $0A, $FE, $04, $C9, $08, $E3, $0B, $FE
    db $02, $BF, $08, $FE, $05, $C0, $08, $FE
    db $08, $BF, $08, $E3, $0B, $FE, $09, $C0
    db $08, $FE, $04, $BF, $08, $FE, $08, $C0
    db $08, $FE, $0B, $BF, $08, $E3, $0A, $FE
    db $08, $C9, $08, $E3, $0B, $FE, $04, $BF
    db $08, $FE, $08, $C0, $08, $FE, $0B, $BF
    db $08, $F3, $B3, $C2, $00, $FC, $01, $EA
    db $E3, $03, $E8, $14, $FB, $01, $FE, $08
    db $C3, $10, $C7, $08, $CA, $0C, $E2, $FE
    db $05, $04, $FE, $08, $BE, $08, $E2, $FE
    db $05, $08, $FE, $08, $C0, $08, $E2, $FE
    db $05, $08, $FE, $08, $C3, $08, $E2, $FE
    db $05, $08, $FE, $08, $C8, $08, $E2, $FE
    db $02, $08, $FE, $08, $C0, $08, $E2, $FE
    db $05, $08, $FE, $08, $C1, $08, $C2, $10
    db $C5, $08, $CA, $0C, $E2, $FE, $05, $04
    db $FE, $08, $C2, $08, $E2, $FE, $05, $08
    db $FE, $08, $C3, $08, $E2, $FE, $05, $08
    db $FE, $08, $C7, $08, $E2, $FE, $05, $08
    db $FE, $08, $CA, $0E, $E2, $FE, $05, $02
    db $FE, $08, $BE, $18, $F3, $E6, $C2, $02
    db $FC, $01, $EC, $7D, $C3, $EC, $95, $C3
    db $E3, $03, $E4, $03, $19, $EA, $E8, $0B
    db $C7, $07, $C1, $07, $C7, $07, $C6, $07
    db $C0, $07, $C6, $07, $C5, $07, $BF, $07
    db $C5, $08, $C4, $08, $BE, $09, $C4, $09
    db $E8, $0E, $BD, $64, $E0, $01, $FC, $01
    db $E3, $03, $E4, $00, $23, $EA, $E8, $0B
    db $B7, $15, $BC, $15, $B5, $15, $B4, $15
    db $E8, $0E, $B3, $64, $E0, $00, $FC, $01
    db $E3, $02, $EA, $E4, $10, $00, $E9, $26
    db $00, $C5, $09, $E2, $C7, $F1, $07, $E2
    db $E9, $B4, $FF, $04, $FB, $02, $E9, $26
    db $00, $C4, $09, $E2, $C6, $F1, $07, $E2
    db $E9, $B4, $FF, $04, $FB, $02, $E9, $26
    db $00, $C3, $09, $E2, $C5, $F1, $07, $E2
    db $E9, $B4, $FF, $04, $FB, $02, $E9, $26
    db $00, $C2, $09, $E2, $C4, $F1, $07, $E2
    db $E9, $B4, $FF, $04, $FB, $02, $E8, $10
    db $E9, $27, $00, $C1, $09, $E2, $C3, $F1
    db $64, $E0, $04, $F8, $23, $C2, $FE, $30
    db $E8, $13, $F3, $F2, $C4, $04, $F8, $13
    db $F3, $09, $C4, $04, $F3, $09, $C4, $04
    db $EC, $AD, $C4, $FE, $50, $E3, $0F, $AB
    db $05, $FE, $46, $E3, $0F, $A8, $E9, $FB
    db $FF, $02, $E3, $0A, $05, $E9, $03, $00
    db $28, $E0, $07, $FC, $01, $EC, $31, $C4
    db $FE, $13, $E3, $10, $C8, $E5, $0E, $E9
    db $44, $FD, $07, $E2, $E9, $26, $02, $07
    db $E0, $06, $FC, $01, $EA, $FE, $12, $E3
    db $0A, $CC, $F3, $25, $C4, $04, $FC, $01
    db $E4, $00, $1D, $E3, $03, $E5, $0D, $CE
    db $0A, $FB, $01, $E4, $02, $0A, $E5, $0F
    db $E8, $0F, $14, $E0, $04, $FC, $01, $E4
    db $00, $18, $E3, $03, $E5, $0D, $CE, $3C
    db $F3, $49, $C4, $04, $E3, $0A, $FE, $50
    db $AB, $08, $E0, $04, $E3, $0A, $FE, $5A
    db $AB, $10, $E0, $04, $E3, $0A, $FE, $5F
    db $AB, $20, $E0, $04, $E3, $0A, $FE, $64
    db $AB, $40, $E0, $04, $EA, $E3, $0F, $FC
    db $04, $FE, $28, $B7, $E8, $10, $02, $FE
    db $19, $E3, $0A, $B7, $EB, $05, $AB, $05
    db $E0, $04, $E3, $0F, $FC, $04, $EC, $AD
    db $C4, $FE, $23, $E8, $0D, $AF, $01, $04
    db $FE, $32, $B2, $2D, $E0, $03, $E3, $0A
    db $FC, $04, $FE, $23, $A6, $05, $E3, $0D
    db $E8, $0F, $E9, $02, $00, $AF, $23, $E0
    db $04, $FC, $04, $FE, $32, $E8, $0E, $E3
    db $0A, $B5, $05, $E9, $24, $FF, $E3, $0F
    db $AA, $02, $B2, $03, $B1, $02, $ED, $7C
    db $FC, $7C, $FC, $A8, $03, $B2, $06, $B0
    db $07, $E9, $F6, $FF, $A4, $0A, $E0, $04
    db $EC, $06, $C5, $F8, $78, $C0, $FE, $2D
    db $E8, $0F, $EA, $E3, $06, $E9, $A2, $FE
    db $06, $E9, $91, $01, $04, $E2, $E9, $70
    db $FE, $04, $E2, $F3, $F9, $C4, $03, $E8
    db $05, $E3, $0F, $FE, $28, $A8, $02, $A3
    db $03, $AD, $27, $E0, $04, $E8, $10, $FE
    db $50, $E3, $0F, $B0, $02, $02, $E3, $0A
    db $B7, $05, $EC, $06, $C5, $0F, $FE, $46
    db $E3, $0D, $A3, $E9, $02, $00, $74, $E0
    db $04, $EC, $52, $C5, $EC, $6E, $C5, $EA
    db $E4, $1C, $21, $E3, $20, $ED, $FB, $FF
    db $00, $00, $9F, $0E, $E2, $E9, $13, $01
    db $0E, $E2, $E9, $CE, $FF, $46, $E2, $F3
    db $45, $C5, $00, $E4, $32, $00, $E3, $20
    db $EA, $B4, $ED, $00, $00, $F6, $FF, $24
    db $E9, $66, $00, $24, $E2, $E9, $9C, $FF
    db $24, $E2, $F1, $F3, $5F, $C5, $01, $E4
    db $00, $1B, $E3, $20, $EA, $9F, $0A, $ED
    db $FB, $FF, $FB, $FF, $E9, $90, $01, $10
    db $E2, $E9, $06, $FF, $18, $E2, $F3, $7C
    db $C5, $04, $FC, $01, $FE, $23, $E3, $03
    db $E5, $06, $E9, $1E, $00, $A8, $1E, $E0
    db $04, $FC, $01, $FE, $2D, $E3, $03, $E5
    db $06, $E8, $0E, $E9, $D8, $FF, $BC, $28
    db $E0, $06, $EA, $E4, $1E, $00, $E3, $25
    db $E5, $0B, $F8, $32, $C3, $05, $E2, $E9
    db $6A, $FF, $08, $E2, $F1, $E9, $3C, $00
    db $0A, $E9, $82, $00, $FB, $05, $F3, $B5
    db $C5, $06, $E3, $24, $EA, $E5, $06, $E4
    db $00, $7E, $ED, $2C, $01, $4C, $FF, $E9
    db $64, $00, $A2, $05, $E2, $F1, $5F, $F2
    db $FB, $01, $E4, $0F, $03, $14, $E0, $04
    db $FE, $1E, $C8, $F8, $1C, $F3, $F5, $C5
    db $06, $FE, $0E, $C9, $EA, $E3, $11, $E5
    db $0E, $E9, $32, $00, $03, $E2, $E9, $01
    db $00, $19, $F1, $FB, $01, $FE, $08, $E5
    db $0F, $E8, $0A, $14, $E0, $04, $FE, $1E
    db $AF, $F8, $28, $F3, $1B, $C6, $06, $FE
    db $14, $B0, $EA, $E3, $02, $E9, $01, $00
    db $E5, $0E, $0A, $E2, $F1, $1E, $FB, $01
    db $FE, $0D, $E5, $0F, $E8, $0A, $14, $E0
    db $04, $FE, $2F, $E3, $12, $B2, $EA, $19
    db $E0, $06, $FE, $19, $E3, $12, $B3, $EA
    db $14, $16, $E0, $04, $FE, $1E, $EA, $E3
    db $11, $BD, $19, $FB, $01, $FE, $09, $E8
    db $0A, $14, $E0, $04, $FE, $64, $E3, $24
    db $E5, $0B, $E6, $00, $E7, $04, $E9, $14
    db $00, $A8, $03, $E2, $F1, $50, $FB, $01
    db $FE, $14, $E5, $0F, $1E, $E0, $04, $D3
    db $E5, $09, $E3, $1C, $EA, $7F, $E0, $84
    db $D3, $E5, $06, $E3, $20, $EA, $7F, $E0
    db $87, $EA, $EC, $B2, $C6, $80, $E3, $1C
    db $7F, $E0, $87, $EA, $EC, $B9, $C6, $E3
    db $1C, $80, $7F, $E0, $87, $EA, $EC, $C0
    db $C6, $E3, $1C, $80, $7F, $E0, $87, $EA
    db $EC, $C7, $C6, $E3, $1C, $80, $7F, $E0
    db $87, $EA, $EC, $CE, $C6, $E3, $1C, $80
    db $7F, $E0, $86, $EA, $E3, $20, $80, $7F
    db $E0, $86, $EA, $E3, $20, $80, $7F, $E0
    db $86, $EA, $E3, $20, $80, $7F, $E0, $86
    db $EA, $E3, $20, $80, $7F, $E0, $86, $EA
    db $E3, $20, $80, $7F, $E0, $C7, $EA, $80
    db $7F, $E0, $04, $FE, $3C, $E3, $12, $E5
    db $0A, $E9, $B0, $FF, $B2, $07, $E2, $E9
    db $50, $00, $9F, $0D, $E0, $04, $FE, $32
    db $E3, $0E, $E9, $9C, $FF, $B2, $05, $E3
    db $12, $E9, $50, $00, $A6, $0F, $E0, $04
    db $E3, $24, $FE, $64, $E6, $05, $E7, $03
    db $E8, $10, $C0, $14, $FB, $01, $FE, $08
    db $E8, $12, $13, $E0, $04, $E3, $24, $FE
    db $46, $E6, $05, $E7, $03, $E8, $10, $C0
    db $14, $14, $14, $F3, $0B, $C7, $04, $E3
    db $0D, $FE, $32, $E5, $08, $E8, $12, $E9
    db $EC, $FF, $B7, $1E, $E0, $3F, $C7, $4F
    db $C7, $FF, $00, $37, $C7, $00, $00, $5F
    db $C7, $00, $00, $00, $00, $00, $00, $00
    db $00, $00, $00, $00, $00, $00, $00, $6E
    db $C7, $B9, $C7, $26, $C8, $49, $C8, $00
    db $00, $00, $00, $68, $C8, $00, $00, $FA
    db $09, $E7, $1B, $E5, $B4, $F5, $DB, $11
    db $11, $F7, $03, $50, $02, $00, $ED, $E6
    db $E0, $05, $F4, $AA, $E1, $0F, $06, $28
    db $A4, $A8, $AB, $A4, $A8, $AB, $A4, $A8
    db $AB, $A4, $A8, $AB, $A4, $A8, $AB, $A4
    db $A6, $AA, $AD, $A6, $AA, $AD, $A6, $AA
    db $AD, $A6, $AA, $AD, $A6, $AA, $AD, $AA
    db $A6, $AB, $AF, $A6, $AB, $AF, $A6, $AB
    db $AF, $A6, $AB, $AF, $A6, $AB, $AF, $AB
    db $A4, $A9, $AD, $A4, $A9, $AD, $A4, $A7
    db $AC, $A4, $A7, $AC, $A4, $A7, $AC, $A7
    db $00, $ED, $FF, $E0, $00, $F4, $1E, $E1
    db $06, $18, $5D, $8C, $06, $6C, $8C, $0C
    db $2D, $8C, $0C, $2C, $8C, $0C, $2D, $8C
    db $06, $6C, $8C, $0C, $4D, $8C, $0C, $7C
    db $87, $18, $5D, $8C, $06, $6C, $8C, $0C
    db $2D, $8C, $0C, $2C, $8C, $0C, $2D, $8C
    db $06, $6C, $8C, $0C, $2C, $8C, $0C, $7D
    db $8C, $17, $5C, $8B, $07, $7F, $87, $0C
    db $2C, $8B, $8E, $0C, $7B, $92, $F9, $00
    db $05, $93, $06, $91, $0C, $4D, $8E, $F9
    db $06, $01, $8B, $0C, $7F, $87, $18, $5F
    db $85, $06, $7D, $8C, $0C, $2C, $91, $0C
    db $2F, $88, $0C, $2E, $8C, $06, $7D, $8F
    db $0C, $3C, $94, $0C, $7E, $88, $ED, $F5
    db $E1, $0B, $EF, $A0, $C8, $03, $18, $7D
    db $CA, $12, $78, $CB, $0C, $7B, $CA, $06
    db $79, $CB, $0C, $7B, $CA, $06, $7A, $CB
    db $06, $75, $CB, $06, $79, $CB, $06, $78
    db $CB, $ED, $C8, $E0, $05, $F4, $AA, $E1
    db $0F, $0C, $C9, $12, $08, $B0, $B0, $B0
    db $B0, $B0, $EF, $B6, $C8, $02, $06, $C9
    db $12, $B0, $B0, $B0, $B0, $0C, $18, $B0
    db $ED, $FF, $E1, $08, $24, $77, $CE, $06
    db $7A, $CC, $06, $78, $CC, $0C, $7A, $CC
    db $06, $CC, $06, $78, $CC, $0C, $7C, $CC
    db $06, $7A, $CC, $06, $78, $CC, $EF, $BF
    db $C8, $02, $0C, $7A, $CC, $06, $CC, $06
    db $78, $CC, $0C, $7C, $CC, $06, $7A, $CC
    db $1E, $76, $CE, $E1, $0D, $18, $CE, $00
    db $18, $7D, $CA, $12, $78, $CB, $0C, $7B
    db $CA, $0C, $7C, $CA, $06, $7B, $CA, $0C
    db $78, $CB, $0C, $7B, $CA, $00, $06, $C9
    db $12, $B2, $B2, $B2, $B2, $B2, $00, $0C
    db $7A, $CC, $06, $CC, $06, $78, $CC, $0C
    db $7C, $CC, $06, $7A, $CC, $06, $78, $CC
    db $0C, $7A, $CC, $06, $CC, $06, $78, $CC
    db $0C, $7C, $CC, $06, $7A, $CC, $06, $78
    db $CC, $00, $E8, $C8, $F8, $C8, $00, $00
    db $08, $C9, $00, $00, $00, $00, $00, $00
    db $00, $00, $00, $00, $00, $00, $00, $00
    db $17, $C9, $49, $C9, $7F, $C9, $E1, $C9
    db $14, $CA, $43, $CA, $00, $00, $00, $00
    db $FA, $09, $E7, $1B, $E5, $AA, $F5, $3B
    db $11, $11, $F7, $03, $50, $02, $00, $ED
    db $FF, $E0, $01, $F4, $19, $E1, $06, $0C
    db $C9, $06, $7C, $A4, $A3, $0C, $2C, $A2
    db $06, $7C, $A0, $0C, $2C, $A1, $A3, $A4
    db $A6, $06, $7C, $A7, $A8, $9F, $A0, $0C
    db $2C, $A1, $06, $7C, $A2, $A3, $18, $2C
    db $A4, $1E, $3D, $A4, $F9, $0E, $07, $98
    db $00, $ED, $FF, $E0, $00, $F4, $1E, $E1
    db $06, $0C, $6F, $8C, $06, $7E, $8C, $8C
    db $0C, $4E, $90, $06, $7E, $90, $0C, $3E
    db $91, $91, $92, $0C, $6F, $88, $06, $7F
    db $88, $87, $18, $7C, $93, $F9, $04, $14
    db $88, $06, $6E, $87, $06, $7F, $87, $18
    db $2E, $8C, $12, $6F, $8C, $0C, $C9, $ED
    db $FF, $E1, $0B, $06, $7A, $CB, $06, $7E
    db $CA, $06, $79, $CB, $06, $76, $CB, $E0
    db $CD, $E1, $10, $9D, $E1, $0B, $06, $7A
    db $CB, $06, $75, $CB, $06, $78, $CB, $06
    db $7E, $CA, $06, $78, $CB, $06, $74, $CB
    db $06, $76, $CB, $06, $7E, $CA, $06, $7A
    db $CB, $06, $75, $CB, $06, $78, $CB, $06
    db $7A, $CB, $06, $76, $CB, $06, $7E, $CA
    db $06, $7A, $CB, $06, $75, $CB, $06, $79
    db $CB, $06, $76, $CB, $12, $7B, $CB, $06
    db $77, $CB, $06, $75, $CB, $06, $7B, $CB
    db $06, $76, $CB, $06, $7A, $CB, $06, $78
    db $CB, $ED, $FF, $E0, $03, $F4, $0A, $E1
    db $0F, $0C, $77, $A4, $06, $A4, $A6, $0C
    db $A8, $06, $A8, $0C, $A9, $A9, $AA, $AA
    db $06, $AA, $12, $37, $A8, $E0, $01, $F4
    db $19, $E1, $07, $0C, $29, $9F, $06, $79
    db $9F, $9F, $18, $29, $9F, $1E, $3A, $A1
    db $F9, $0E, $07, $95, $ED, $FF, $E0, $03
    db $F4, $0A, $E1, $0F, $0C, $77, $B0, $B0
    db $AE, $06, $AE, $0C, $AD, $AD, $AD, $AD
    db $06, $AD, $12, $AB, $E0, $01, $F4, $19
    db $E1, $08, $0C, $29, $9D, $06, $79, $9D
    db $9D, $18, $29, $9C, $1E, $3A, $9F, $F9
    db $0E, $07, $93, $ED, $D7, $E0, $02, $F4
    db $00, $E1, $0E, $0C, $C9, $06, $7B, $B0
    db $AF, $0C, $2B, $AE, $06, $7B, $AC, $0C
    db $2B, $AD, $AF, $B0, $B2, $06, $7B, $B3
    db $B4, $AB, $AC, $0C, $2B, $AD, $06, $7B
    db $AE, $AF, $18, $2B, $B0, $1E, $3C, $BC
    db $F9, $0E, $07, $B0, $00

Spc_CA75:
    push a                            ; CA75
    mul ya                            ; CA76
    mov.b $26,a                       ; CA77
    mov.b $27,y                       ; CA79
    mov a,x                           ; CA7B
    mov y,a                           ; CA7C
    pop a                             ; CA7D
    mul ya                            ; CA7E
    push a                            ; CA7F
    mov a,y                           ; CA80
    mov x,a                           ; CA81
    pop a                             ; CA82
    mov y,a                           ; CA83
    mov a,#$00                        ; CA84
    addw ya,$26                       ; CA86
    ret                               ; CA88

Spc_CA89:
    push a                            ; CA89
    push y                            ; CA8A
    mov y,#$00                        ; CA8B
    pop a                             ; CA8D
    div ya,x                          ; CA8E
    mov.b $27,a                       ; CA8F
    pop a                             ; CA91
    div ya,x                          ; CA92
    mov.b y,$27                       ; CA93
    ret                               ; CA95

Spc_CA96:
    inc x                             ; CA96
    inc x                             ; CA97
    inc x                             ; CA98
    inc x                             ; CA99
    inc x                             ; CA9A
    inc x                             ; CA9B
    inc x                             ; CA9C
    inc x                             ; CA9D
    ret                               ; CA9E

Spc_CA9F:
    dec x                             ; CA9F
    dec x                             ; CAA0
    dec x                             ; CAA1
    dec x                             ; CAA2
    dec x                             ; CAA3
    dec x                             ; CAA4
    dec x                             ; CAA5
    dec x                             ; CAA6
    ret                               ; CAA7
    db $7C, $21, $7A, $23, $96, $25, $D2, $27
    db $30, $2A, $B2, $2C, $5A, $2F, $2C, $32
    db $28, $35, $50, $38, $AA, $3B, $36, $3F

Spc_CAC0:
    mov x,#$07                        ; CAC0
.lCAC2:
    mov.w a,$03C8+x                   ; CAC2
    cmp a,#$23                        ; CAC5
    beq .lCACD                        ; CAC7
    dec x                             ; CAC9
    bne .lCAC2                        ; CACA
    ret                               ; CACC
.lCACD:
    dec.w $0563                       ; CACD
    mov.w a,$0563                     ; CAD0
    and a,#$03                        ; CAD3
    mov y,#$3F                        ; CAD5
    mul ya                            ; CAD7
    mov y,a                           ; CAD8
    mov $20,#$07                      ; CAD9
.lCADC:
    inc y                             ; CADC
    mov x,#$04                        ; CADD
.lCADF:
    mov1 c,$0019.6                    ; CADF
    eor1 c,$0019.5                    ; CAE2
    rol.b $18                         ; CAE5
    rol.b $19                         ; CAE7
    mov.b a,$19                       ; CAE9
    and a,#$D0                        ; CAEB
    or a,#$11                         ; CAED
    mov.w $FF00+y,a                   ; CAEF
    inc y                             ; CAF2
    mov.b a,$18                       ; CAF3
    and a,#$FF                        ; CAF5
    or a,#$11                         ; CAF7
    mov.w $FF00+y,a                   ; CAF9
    inc y                             ; CAFC
    dec x                             ; CAFD
    bne .lCADF                        ; CAFE
    dbnz $20,.lCADC                   ; CB00
    ret                               ; CB03

%spc($EDF0, $19F40D)

Spc_EDF0:
    mov y,#$00                        ; EDF0
    mov x,#$1B                        ; EDF2
.lEDF4:
    mov.w $FF00+y,a                   ; EDF4
    inc y                             ; EDF7
    inc y                             ; EDF8
    inc y                             ; EDF9
    inc y                             ; EDFA
    inc y                             ; EDFB
    inc y                             ; EDFC
    inc y                             ; EDFD
    inc y                             ; EDFE
    inc y                             ; EDFF
    dec x                             ; EE00
    bne .lEDF4                        ; EE01
    inc a                             ; EE03
    mov.w $FF00+y,a                   ; EE04
    mov y,#$FF                        ; EE07
    mov a,#$00                        ; EE09
    mov.w $3F8C,a                     ; EE0B
    mov.w $3F8D,y                     ; EE0E
    mov.w $3F8E,a                     ; EE11
    mov.w $3F8F,y                     ; EE14
    mov.b a,$18                       ; EE17
    or.b a,$19                        ; EE19
    bne .lEE1F                        ; EE1B
    inc.b $18                         ; EE1D  ; not reached in the traces so far
.lEE1F:
    ret                               ; EE1F
    db $EE, $02, $D6, $06, $CA, $08, $C4, $09
    db $28, $0A, $5A, $0A, $8C, $0A, $BE, $0A
    db $78, $14, $8C, $28, $A0, $3C, $B4, $50
    db $C8, $64, $DC, $78, $F0, $8C, $DC, $A0
%spc_end()
