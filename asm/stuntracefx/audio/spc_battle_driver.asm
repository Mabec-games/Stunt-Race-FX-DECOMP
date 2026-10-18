; spc_battle_driver.asm: the battle sound driver, the N-SPC engine as
; uploaded at 03:D96F (block list $1A:F091: $10B0-$260E, start $0800),
; being filed routine by routine; routines not yet described carry
; plain labels.

incsrc "../include/macros.inc"
incsrc "../include/spc.inc"

%spc($10B0, $1AF095)
    mov.w a,$0558                     ; 10B0
    cmp.b a,$04                       ; 10B3
    beq spcb_10C0                     ; 10B5
    mov.w a,$0558                     ; 10B7
    mov.b $04,a                       ; 10BA
    mov.b $00,a                       ; 10BC
    bra spcb_10C3                     ; 10BE
spcb_10C0:
    mov $00,#$00                      ; 10C0
spcb_10C3:
    mov.w a,$0559                     ; 10C3
    mov.b $05,a                       ; 10C6
    mov.b $01,a                       ; 10C8
    mov.w a,$055A                     ; 10CA
    mov.b $06,a                       ; 10CD
    mov.b $02,a                       ; 10CF
    mov.w a,$055B                     ; 10D1
    mov.b $07,a                       ; 10D4
    mov.b $03,a                       ; 10D6
    bbc $29.1,spcb_10DF               ; 10D8
    clr1 $29.0                        ; 10DB
    clr1 $29.1                        ; 10DD
spcb_10DF:
    call SpcB_1115                    ; 10DF
    call SpcB_19BC                    ; 10E2
    call SpcB_1637                    ; 10E5
    call SpcB_111E                    ; 10E8
    call SpcB_14A6                    ; 10EB
    call SpcB_1A2F                    ; 10EE
    call SpcB_13C6                    ; 10F1
    call SpcB_1163                    ; 10F4
    call SpcB_11ED                    ; 10F7
    call SpcB_14D1                    ; 10FA
    call SpcB_1584                    ; 10FD
    call SpcB_1477                    ; 1100
    call SpcB_12D2                    ; 1103
    call SpcB_130C                    ; 1106
    call SpcB_25B8                    ; 1109
    call SpcB_1467                    ; 110C
    call SpcB_1671                    ; 110F
    jmp spcb_1A7F                     ; 1112

%spc($1115, $1AF0FA)
SpcB_1115:
    cmp $00,#$88                      ; 1115
    bne spcb_111D                     ; 1118
    mov $00,#$15                      ; 111A
spcb_111D:
    ret                               ; 111D

%spc($111E, $1AF103)
SpcB_111E:
    bbc $2D.6,spcb_111D               ; 111E
    mov.w a,$0395                     ; 1121
    cmp a,#$43                        ; 1124
    beq spcb_111D                     ; 1126
    mov x,#$05                        ; 1128
    jmp SpcB_186C                     ; 112A
spcb_112D:
    bbc $2D.5,spcb_1156               ; 112D
    mov.w a,$0397                     ; 1130
    cmp a,#$77                        ; 1133
    beq spcb_1157                     ; 1135
    cmp a,#$78                        ; 1137
    beq spcb_1157                     ; 1139
    cmp a,#$79                        ; 113B
    beq spcb_1157                     ; 113D
    cmp a,#$79                        ; 113F
    beq spcb_1157                     ; 1141
spcb_1143:
    mov.w a,$0396                     ; 1143
    cmp a,#$77                        ; 1146
    beq spcb_115E                     ; 1148
    cmp a,#$78                        ; 114A
    beq spcb_115E                     ; 114C
    cmp a,#$79                        ; 114E
    beq spcb_115E                     ; 1150
    cmp a,#$7A                        ; 1152
    beq spcb_115E                     ; 1154
spcb_1156:
    ret                               ; 1156
spcb_1157:
    mov x,#$07                        ; 1157
    call SpcB_186C                    ; 1159
    bra spcb_1143                     ; 115C
spcb_115E:
    mov x,#$06                        ; 115E
    jmp SpcB_186C                     ; 1160

%spc($1163, $1AF148)
SpcB_1163:
    inc.w $0581                       ; 1163
    mov.w a,$0581                     ; 1166
    cmp a,#$06                        ; 1169
    beq spcb_112D                     ; 116B
    cmp a,#$09                        ; 116D
    beq spcb_1197                     ; 116F
    cmp a,#$0C                        ; 1171
    beq spcb_11A5                     ; 1173
    cmp a,#$0F                        ; 1175
    beq spcb_11B3                     ; 1177
    cmp a,#$12                        ; 1179
    beq spcb_11C7                     ; 117B
    cmp a,#$15                        ; 117D
    beq spcb_11DB                     ; 117F
    cmp a,#$18                        ; 1181
    beq spcb_11E4                     ; 1183
    sbc a,#$19                        ; 1185
    bmi spcb_118E                     ; 1187
    mov a,#$00                        ; 1189
    mov.w $0581,a                     ; 118B
spcb_118E:
    cmp $1C,#$01                      ; 118E
    beq spcb_1196                     ; 1191
    mov $2A,#$00                      ; 1193
spcb_1196:
    ret                               ; 1196
spcb_1197:
    bbs $2A.0,spcb_11A0               ; 1197
    mov $20,#$0C                      ; 119A
    jmp spcb_1833                     ; 119D
spcb_11A0:
    mov a,#$17                        ; 11A0
    jmp SpcB_12FA                     ; 11A2
spcb_11A5:
    bbs $2A.1,spcb_11AE               ; 11A5
    mov $20,#$1C                      ; 11A8
    jmp spcb_1856                     ; 11AB
spcb_11AE:
    mov a,#$97                        ; 11AE
    jmp SpcB_12FA                     ; 11B0
spcb_11B3:
    bbs $2A.2,spcb_11BC               ; 11B3
    mov $20,#$0A                      ; 11B6
    jmp spcb_181D                     ; 11B9
spcb_11BC:
    bbs $2A.0,spcb_118E               ; 11BC
    bbs $2A.4,spcb_118E               ; 11BF
    mov a,#$25                        ; 11C2
    jmp SpcB_12FA                     ; 11C4
spcb_11C7:
    bbs $2A.2,spcb_11D0               ; 11C7
    mov $20,#$1A                      ; 11CA
    jmp spcb_1840                     ; 11CD
spcb_11D0:
    bbs $2A.1,spcb_118E               ; 11D0
    bbs $2A.5,spcb_118E               ; 11D3
    mov a,#$25                        ; 11D6
    jmp SpcB_12FA                     ; 11D8
spcb_11DB:
    bbs $2A.4,spcb_118E               ; 11DB
    mov $20,#$0B                      ; 11DE
    jmp spcb_1828                     ; 11E1
spcb_11E4:
    bbs $2A.5,spcb_118E               ; 11E4
    mov $20,#$1B                      ; 11E7
    jmp spcb_184B                     ; 11EA

%spc($11ED, $1AF1D2)
SpcB_11ED:
    mov.w a,$056A                     ; 11ED
    cmp a,#$FF                        ; 11F0
    beq spcb_11F7                     ; 11F2
    inc.w $056A                       ; 11F4
spcb_11F7:
    mov.b a,$1B                       ; 11F7
    cmp a,#$05                        ; 11F9
    beq spcb_11FF                     ; 11FB
    bra spcb_1207                     ; 11FD
spcb_11FF:
    mov a,#$00                        ; 11FF
    mov.w $056A,a                     ; 1201
    mov.w $056B,a                     ; 1204
spcb_1207:
    mov.w a,$056A                     ; 1207
    bmi spcb_120E                     ; 120A
    bra spcb_1218                     ; 120C
spcb_120E:
    mov.w a,$056B                     ; 120E
    cmp a,#$FF                        ; 1211
    beq spcb_1218                     ; 1213
    inc.w $056B                       ; 1215
spcb_1218:
    cmp $1B,#$02                      ; 1218
    bne spcb_1222                     ; 121B
    mov a,#$01                        ; 121D
    mov.w $0567,a                     ; 121F
spcb_1222:
    mov.w a,$0567                     ; 1222
    beq spcb_1239                     ; 1225
    inc.w $0567                       ; 1227
    mov.w a,$0567                     ; 122A
    cmp a,#$E6                        ; 122D
    bne spcb_1239                     ; 122F
    mov a,#$00                        ; 1231
    mov.w $0567,a                     ; 1233
    mov $1B,#$03                      ; 1236
spcb_1239:
    mov.w a,$056A                     ; 1239
    bmi spcb_1256                     ; 123C
    mov x,#$00                        ; 123E
spcb_1240:
    mov.w a,$1251+x                   ; 1240
    beq spcb_1256                     ; 1243
    cmp.b a,$1D                       ; 1245
    beq spcb_124C                     ; 1247
    inc x                             ; 1249
    bra spcb_1240                     ; 124A
spcb_124C:
    mov $1D,#$00                      ; 124C
    bra spcb_1256                     ; 124F
    tcall 1                           ; 1251
    clr1 $13.0                        ; 1252
    or.b a,$00+x                      ; 1254
spcb_1256:
    cmp $1D,#$32                      ; 1256
    bne spcb_126F                     ; 1259
    mov $2A,#$00                      ; 125B
    set1 $2D.5                        ; 125E
    set1 $2D.6                        ; 1260
    mov x,#$02                        ; 1262
    call SpcB_186C                    ; 1264
    dec x                             ; 1267
    call SpcB_186C                    ; 1268
    dec x                             ; 126B
    call SpcB_186C                    ; 126C
spcb_126F:
    cmp $1D,#$03                      ; 126F
    beq spcb_127B                     ; 1272
    cmp $1D,#$10                      ; 1274
    beq spcb_127B                     ; 1277
    bra spcb_128B                     ; 1279
spcb_127B:
    mov x,#$05                        ; 127B
    call SpcB_186C                    ; 127D
    dec x                             ; 1280
    call SpcB_186C                    ; 1281
    mov $2A,#$00                      ; 1284
    set1 $2D.6                        ; 1287
    set1 $2D.5                        ; 1289
spcb_128B:
    mov.w a,$0579                     ; 128B
    beq spcb_12A2                     ; 128E
    inc.w $0579                       ; 1290
    mov.w a,$0579                     ; 1293
    setc                              ; 1296
    sbc a,#$78                        ; 1297
    bmi spcb_12A2                     ; 1299
    set1 $2D.5                        ; 129B
    mov a,#$00                        ; 129D
    mov.w $0579,a                     ; 129F
spcb_12A2:
    mov.w a,$0395                     ; 12A2
    cmp a,#$17                        ; 12A5
    beq spcb_12AF                     ; 12A7
    cmp a,#$21                        ; 12A9
    beq spcb_12AF                     ; 12AB
    bra spcb_12BE                     ; 12AD
spcb_12AF:
    cmp $01,#$09                      ; 12AF
    beq spcb_12BB                     ; 12B2
    cmp $01,#$0D                      ; 12B4
    beq spcb_12BB                     ; 12B7
    bra spcb_12BE                     ; 12B9
spcb_12BB:
    mov $01,#$00                      ; 12BB
spcb_12BE:
    mov.w a,$0397                     ; 12BE
    cmp a,#$32                        ; 12C1
    bne spcb_12D1                     ; 12C3
    mov.w a,$0394                     ; 12C5
    cmp a,#$38                        ; 12C8
    bne spcb_12D1                     ; 12CA
    mov x,#$04                        ; 12CC
    call SpcB_186C                    ; 12CE
spcb_12D1:
    ret                               ; 12D1

%spc($12D2, $1AF2B7)
SpcB_12D2:
    cmp $1D,#$4C                      ; 12D2
    bne spcb_12F9                     ; 12D5
    mov x,#$07                        ; 12D7
    call SpcB_199E                    ; 12D9
    dec x                             ; 12DC
    call SpcB_199E                    ; 12DD
    dec x                             ; 12E0
    call SpcB_186C                    ; 12E1
    dec x                             ; 12E4
    call SpcB_186C                    ; 12E5
    dec x                             ; 12E8
    call SpcB_186C                    ; 12E9
    mov a,#$01                        ; 12EC
    mov.w $0579,a                     ; 12EE
    mov $2A,#$00                      ; 12F1
    call $0A60                        ; 12F4
    set1 $2D.6                        ; 12F7
spcb_12F9:
    ret                               ; 12F9

%spc($12FA, $1AF2DF)
SpcB_12FA:
    push x                            ; 12FA
    push a                            ; 12FB
    mov x,#$00                        ; 12FC
spcb_12FE:
    mov.w a,$0588+x                   ; 12FE
    beq spcb_1306                     ; 1301
    inc x                             ; 1303
    bra spcb_12FE                     ; 1304
spcb_1306:
    pop a                             ; 1306
    mov.w $0588+x,a                   ; 1307
    pop x                             ; 130A
spcb_130B:
    ret                               ; 130B

%spc($130C, $1AF2F1)
SpcB_130C:
    bbs $29.0,spcb_130B               ; 130C
    cmp $1D,#$3E                      ; 130F
    bne spcb_1325                     ; 1312
    mov a,#$01                        ; 1314
    mov.w $0568,a                     ; 1316
    bbc $00.7,spcb_1325               ; 1319
    mov.b $20,a                       ; 131C
    set1 $20.7                        ; 131E
    mov.b a,$20                       ; 1320
    mov.w $0568,a                     ; 1322
spcb_1325:
    mov.w a,$0568                     ; 1325
    beq spcb_132D                     ; 1328
    inc.w $0568                       ; 132A
spcb_132D:
    mov.w a,$0568                     ; 132D
    and a,#$7F                        ; 1330
    cmp a,#$2A                        ; 1332
    bne spcb_136B                     ; 1334
    mov.w a,$0568                     ; 1336
    asl a                             ; 1339
    bcs spcb_1351                     ; 133A
    mov.w a,$0397                     ; 133C
spcb_133F:
    cmp a,#$77                        ; 133F
    beq spcb_135A                     ; 1341
    cmp a,#$78                        ; 1343
    beq spcb_1356                     ; 1345
    cmp a,#$79                        ; 1347
    beq spcb_135A                     ; 1349
    cmp a,#$7A                        ; 134B
    beq spcb_1356                     ; 134D
    bra spcb_1366                     ; 134F
spcb_1351:
    mov.w a,$0396                     ; 1351
    bra spcb_133F                     ; 1354
spcb_1356:
    mov a,#$0F                        ; 1356
    bra spcb_135C                     ; 1358
spcb_135A:
    mov a,#$2F                        ; 135A
spcb_135C:
    asl.w $0568                       ; 135C
    bcc spcb_1363                     ; 135F
    or a,#$80                         ; 1361
spcb_1363:
    call SpcB_12FA                    ; 1363
spcb_1366:
    mov a,#$00                        ; 1366
    mov.w $0568,a                     ; 1368
spcb_136B:
    cmp $1D,#$20                      ; 136B
    beq spcb_1377                     ; 136E
    cmp $1D,#$16                      ; 1370
    beq spcb_137B                     ; 1373
    bra spcb_1399                     ; 1375
spcb_1377:
    set1 $2C.4                        ; 1377
    bra spcb_137D                     ; 1379
spcb_137B:
    set1 $2C.5                        ; 137B
spcb_137D:
    mov.w a,$0397                     ; 137D
    cmp a,#$01                        ; 1380
    beq spcb_1391                     ; 1382
    cmp a,#$04                        ; 1384
    beq spcb_138C                     ; 1386
    cmp a,#$07                        ; 1388
    beq spcb_1391                     ; 138A
spcb_138C:
    mov $1D,#$1B                      ; 138C
    bra spcb_1394                     ; 138F
spcb_1391:
    mov $1D,#$27                      ; 1391
spcb_1394:
    mov a,#$01                        ; 1394
    mov.w $055D,a                     ; 1396
spcb_1399:
    mov.w a,$055D                     ; 1399
    beq spcb_13A1                     ; 139C
    inc.w $055D                       ; 139E
spcb_13A1:
    mov.w a,$055D                     ; 13A1
    cmp a,#$11                        ; 13A4
    bne spcb_13C5                     ; 13A6
    mov a,#$00                        ; 13A8
    mov.w $055D,a                     ; 13AA
    bbs $2C.4,spcb_13B5               ; 13AD
    bbs $2C.5,spcb_13BE               ; 13B0
    bra spcb_13C5                     ; 13B3
spcb_13B5:
    clr1 $2C.4                        ; 13B5
    mov a,#$20                        ; 13B7
    call SpcB_12FA                    ; 13B9
    bra spcb_13C5                     ; 13BC
spcb_13BE:
    clr1 $2C.5                        ; 13BE
    mov a,#$16                        ; 13C0
    call SpcB_12FA                    ; 13C2
spcb_13C5:
    ret                               ; 13C5

%spc($13C6, $1AF3AB)
SpcB_13C6:
    call SpcB_1444                    ; 13C6
    bbs $2C.2,spcb_13D0               ; 13C9
    bbs $2C.3,spcb_13DB               ; 13CC
spcb_13CF:
    ret                               ; 13CF
spcb_13D0:
    mov.w a,$0555                     ; 13D0
    beq spcb_13F7                     ; 13D3
    cmp a,#$FE                        ; 13D5
    beq spcb_13CF                     ; 13D7
    bra spcb_140F                     ; 13D9
spcb_13DB:
    clr1 $2C.2                        ; 13DB
    clr1 $2C.3                        ; 13DD
    mov a,#$00                        ; 13DF
    mov.w $0555,a                     ; 13E1
    mov.w $0553,a                     ; 13E4
    mov.w $0554,a                     ; 13E7
    mov.w $0569,a                     ; 13EA
    mov.w a,$0578                     ; 13ED
    mov.b $50,a                       ; 13F0
    mov.w a,$0566                     ; 13F2
    bra spcb_1441                     ; 13F5
spcb_13F7:
    mov a,#$04                        ; 13F7
    mov.w $0554,a                     ; 13F9
    mov.b a,$53                       ; 13FC
    mov.w $0566,a                     ; 13FE
    mov.b a,$50                       ; 1401
    mov.w $0578,a                     ; 1403
    mov y,#$7F                        ; 1406
    mul ya                            ; 1408
    mov x,#$FE                        ; 1409
    div ya,x                          ; 140B
    mov.w $0569,a                     ; 140C
spcb_140F:
    inc.w $0555                       ; 140F
    mov.w a,$0555                     ; 1412
    mov y,#$00                        ; 1415
    mov x,#$15                        ; 1417
    div ya,x                          ; 1419
    mov.b $20,a                       ; 141A
    setc                              ; 141C
    mov.w a,$0578                     ; 141D
    sbc.b a,$20                       ; 1420
    mov.b $50,a                       ; 1422
    mov.w a,$0569                     ; 1424
    mov.b $20,a                       ; 1427
    mov $21,#$00                      ; 1429
    mov.w a,$0553                     ; 142C
    mov.w y,$0554                     ; 142F
    addw ya,$20                       ; 1432
    mov.w $0553,a                     ; 1434
    mov.w $0554,y                     ; 1437
    mov.w a,$0566                     ; 143A
    setc                              ; 143D
    sbc.w a,$0554                     ; 143E
spcb_1441:
    mov.b $53,a                       ; 1441
    ret                               ; 1443

%spc($1444, $1AF429)
SpcB_1444:
    mov.b a,$1D                       ; 1444
    cmp a,#$2C                        ; 1446
    beq spcb_1458                     ; 1448
    mov.w x,$0555                     ; 144A
    beq spcb_1461                     ; 144D
    cmp a,#$30                        ; 144F
    beq spcb_1462                     ; 1451
    cmp a,#$3C                        ; 1453
    beq spcb_1462                     ; 1455
    ret                               ; 1457
spcb_1458:
    cmp $1C,#$01                      ; 1458
    bne spcb_1461                     ; 145B
    set1 $2C.2                        ; 145D
    clr1 $2C.3                        ; 145F
spcb_1461:
    ret                               ; 1461
spcb_1462:
    clr1 $2C.2                        ; 1462
    set1 $2C.3                        ; 1464
    ret                               ; 1466

%spc($1467, $1AF44C)
SpcB_1467:
    cmp $1C,#$01                      ; 1467
    beq spcb_1476                     ; 146A
    mov.b a,$01                       ; 146C
    setc                              ; 146E
    sbc a,#$07                        ; 146F
    bmi spcb_1476                     ; 1471
    mov $01,#$00                      ; 1473
spcb_1476:
    ret                               ; 1476

%spc($1477, $1AF45C)
SpcB_1477:
    mov.b a,$1C                       ; 1477
    cmp a,#$01                        ; 1479
    beq spcb_148E                     ; 147B
    mov x,#$00                        ; 147D
spcb_147F:
    mov.w a,$148F+x                   ; 147F
    beq spcb_148B                     ; 1482
    cmp.b a,$1D                       ; 1484
    beq spcb_148E                     ; 1486
    inc x                             ; 1488
    bra spcb_147F                     ; 1489
spcb_148B:
    mov $1D,#$00                      ; 148B
spcb_148E:
    ret                               ; 148E
    db $03, $10, $32, $2B, $34, $43, $44, $45
    db $77, $78, $79, $7A, $23, $3A, $3B, $3F
    db $06, $22, $41, $40, $1F, $2A, $00

%spc($14A6, $1AF48B)
SpcB_14A6:
    mov x,#$00                        ; 14A6
spcb_14A8:
    mov.w a,$14B8+x                   ; 14A8
    beq spcb_14B7                     ; 14AB
    cmp.b a,$1D                       ; 14AD
    beq spcb_14B4                     ; 14AF
    inc x                             ; 14B1
    bra spcb_14A8                     ; 14B2
spcb_14B4:
    mov $1D,#$00                      ; 14B4
spcb_14B7:
    ret                               ; 14B7
    db $02, $01, $04, $07, $0A, $0D, $25, $15
    db $17, $1E, $21, $18, $19, $1A, $09, $0C
    db $0B, $2E, $4E, $2D, $29, $4D, $33, $5A
    db $00

%spc($14D1, $1AF4B6)
SpcB_14D1:
    bbc $28.2,spcb_14D7               ; 14D1
    call SpcB_14EA                    ; 14D4
spcb_14D7:
    bbc $28.3,spcb_14DD               ; 14D7
    call SpcB_152C                    ; 14DA
spcb_14DD:
    bbc $28.4,spcb_14E3               ; 14DD
    call SpcB_150B                    ; 14E0
spcb_14E3:
    bbc $28.5,spcb_14E9               ; 14E3
    call SpcB_1558                    ; 14E6
spcb_14E9:
    ret                               ; 14E9

%spc($14EA, $1AF4CF)
SpcB_14EA:
    mov.w a,$0547                     ; 14EA
    cmp a,#$14                        ; 14ED
    beq spcb_150A                     ; 14EF
    inc.w $0547                       ; 14F1
    mov.w a,$054D                     ; 14F4
    mov.b $20,a                       ; 14F7
    mov.w a,$054F                     ; 14F9
    mov.b $21,a                       ; 14FC
    mov a,#$20                        ; 14FE
    mov y,#$00                        ; 1500
    addw ya,$20                       ; 1502
    mov.w $054D,a                     ; 1504
    mov.w $054F,y                     ; 1507
spcb_150A:
    ret                               ; 150A

%spc($150B, $1AF4F0)
SpcB_150B:
    mov.w a,$0546                     ; 150B
    cmp a,#$14                        ; 150E
    beq spcb_152B                     ; 1510
    inc.w $0546                       ; 1512
    mov.w a,$054C                     ; 1515
    mov.b $20,a                       ; 1518
    mov.w a,$054E                     ; 151A
    mov.b $21,a                       ; 151D
    mov a,#$20                        ; 151F
    mov y,#$00                        ; 1521
    addw ya,$20                       ; 1523
    mov.w $054C,a                     ; 1525
    mov.w $054E,y                     ; 1528
spcb_152B:
    ret                               ; 152B

%spc($152C, $1AF511)
SpcB_152C:
    mov.w a,$0547                     ; 152C
    cmp a,#$00                        ; 152F
    beq spcb_154B                     ; 1531
    dec.w $0547                       ; 1533
    mov.w a,$054D                     ; 1536
    mov.w y,$054F                     ; 1539
    mov $20,#$20                      ; 153C
    mov $21,#$00                      ; 153F
    subw ya,$20                       ; 1542
    mov.w $054D,a                     ; 1544
    mov.w $054F,y                     ; 1547
    ret                               ; 154A
spcb_154B:
    clr1 $28.2                        ; 154B
    clr1 $28.3                        ; 154D
    mov a,#$00                        ; 154F
    mov.w $054D,a                     ; 1551
    mov.w $054F,a                     ; 1554
    ret                               ; 1557

%spc($1558, $1AF53D)
SpcB_1558:
    mov.w a,$0546                     ; 1558
    cmp a,#$00                        ; 155B
    beq spcb_1577                     ; 155D
    dec.w $0546                       ; 155F
    mov.w a,$054C                     ; 1562
    mov.w y,$054E                     ; 1565
    mov $20,#$20                      ; 1568
    mov $21,#$00                      ; 156B
    subw ya,$20                       ; 156E
    mov.w $054C,a                     ; 1570
    mov.w $054E,y                     ; 1573
    ret                               ; 1576
spcb_1577:
    clr1 $28.4                        ; 1577
    clr1 $28.5                        ; 1579
    mov a,#$00                        ; 157B
    mov.w $054C,a                     ; 157D
    mov.w $054E,a                     ; 1580
    ret                               ; 1583

%spc($1584, $1AF569)
SpcB_1584:
    bbc $29.2,spcb_158A               ; 1584
    call SpcB_159D                    ; 1587
spcb_158A:
    bbc $29.3,spcb_1590               ; 158A
    call SpcB_15BE                    ; 158D
spcb_1590:
    bbc $29.4,spcb_1596               ; 1590
    call SpcB_15EA                    ; 1593
spcb_1596:
    bbc $29.5,spcb_159C               ; 1596
    call SpcB_160B                    ; 1599
spcb_159C:
    ret                               ; 159C

%spc($159D, $1AF582)
SpcB_159D:
    mov.w a,$0543                     ; 159D
    cmp a,#$19                        ; 15A0
    beq spcb_15BD                     ; 15A2
    inc.w $0543                       ; 15A4
    mov.w a,$0549                     ; 15A7
    mov.b $20,a                       ; 15AA
    mov.w a,$054B                     ; 15AC
    mov.b $21,a                       ; 15AF
    mov a,#$3C                        ; 15B1
    mov y,#$00                        ; 15B3
    addw ya,$20                       ; 15B5
    mov.w $0549,a                     ; 15B7
    mov.w $054B,y                     ; 15BA
spcb_15BD:
    ret                               ; 15BD

%spc($15BE, $1AF5A3)
SpcB_15BE:
    mov.w a,$0543                     ; 15BE
    cmp a,#$00                        ; 15C1
    beq SpcB_15DD                     ; 15C3
    dec.w $0543                       ; 15C5
    mov.w a,$0549                     ; 15C8
    mov.w y,$054B                     ; 15CB
    mov $20,#$3C                      ; 15CE
    mov $21,#$00                      ; 15D1
    subw ya,$20                       ; 15D4
    mov.w $0549,a                     ; 15D6
    mov.w $054B,y                     ; 15D9
    ret                               ; 15DC

%spc($15DD, $1AF5C2)
SpcB_15DD:
    clr1 $29.2                        ; 15DD
    clr1 $29.3                        ; 15DF
    mov a,#$00                        ; 15E1
    mov.w $0549,a                     ; 15E3
    mov.w $054B,a                     ; 15E6
    ret                               ; 15E9

%spc($15EA, $1AF5CF)
SpcB_15EA:
    mov.w a,$0542                     ; 15EA
    cmp a,#$19                        ; 15ED
    beq spcb_160A                     ; 15EF
    inc.w $0542                       ; 15F1
    mov.w a,$0548                     ; 15F4
    mov.b $20,a                       ; 15F7
    mov.w a,$054A                     ; 15F9
    mov.b $21,a                       ; 15FC
    mov a,#$3C                        ; 15FE
    mov y,#$00                        ; 1600
    addw ya,$20                       ; 1602
    mov.w $0548,a                     ; 1604
    mov.w $054A,y                     ; 1607
spcb_160A:
    ret                               ; 160A

%spc($160B, $1AF5F0)
SpcB_160B:
    mov.w a,$0542                     ; 160B
    cmp a,#$00                        ; 160E
    beq spcb_162A                     ; 1610
    dec.w $0542                       ; 1612
    mov.w a,$0548                     ; 1615
    mov.w y,$054A                     ; 1618
    mov $20,#$3C                      ; 161B
    mov $21,#$00                      ; 161E
    subw ya,$20                       ; 1621
    mov.w $0548,a                     ; 1623
    mov.w $054A,y                     ; 1626
    ret                               ; 1629
spcb_162A:
    clr1 $29.4                        ; 162A
    clr1 $29.5                        ; 162C
    mov a,#$00                        ; 162E
    mov.w $0548,a                     ; 1630
    mov.w $054A,a                     ; 1633
    ret                               ; 1636

%spc($1637, $1AF61C)
SpcB_1637:
    mov $1D,#$00                      ; 1637
    mov.b a,$00                       ; 163A
    and a,#$7F                        ; 163C
    beq spcb_1649                     ; 163E
    setc                              ; 1640
    sbc a,#$06                        ; 1641
    bmi spcb_164C                     ; 1643
    inc a                             ; 1645
    mov.b $1D,a                       ; 1646
    ret                               ; 1648
spcb_1649:
    mov.b $1D,a                       ; 1649
    ret                               ; 164B
spcb_164C:
    bbs $29.0,spcb_1670               ; 164C
    mov.b a,$00                       ; 164F
    mov.b $1B,a                       ; 1651
    cmp a,#$02                        ; 1653
    bne spcb_1657                     ; 1655
spcb_1657:
    mov a,#$00                        ; 1657
    mov.b $2E,a                       ; 1659
    mov.b $2F,a                       ; 165B
    mov.w $0553,a                     ; 165D
    mov.w $0554,a                     ; 1660
    mov.w $0555,a                     ; 1663
    clr1 $29.0                        ; 1666
    clr1 $29.1                        ; 1668
    clr1 $29.6                        ; 166A
    clr1 $2C.2                        ; 166C
    clr1 $2C.3                        ; 166E
spcb_1670:
    ret                               ; 1670

%spc($1671, $1AF656)
SpcB_1671:
    mov.b a,$01                       ; 1671
    cmp.b a,$09                       ; 1673
    bne spcb_167C                     ; 1675
    mov $20,#$00                      ; 1677
    bra spcb_167F                     ; 167A
spcb_167C:
    mov $20,$01                       ; 167C
spcb_167F:
    mov $09,$01                       ; 167F
    mov.b a,$20                       ; 1682
    setc                              ; 1684
    sbc a,#$3E                        ; 1685
    bmi spcb_168C                     ; 1687
    mov $20,#$00                      ; 1689
spcb_168C:
    mov.b a,$20                       ; 168C
    beq spcb_1670                     ; 168E
    dec a                             ; 1690
    asl a                             ; 1691
    mov x,a                           ; 1692
    jmp ($1696+x)                     ; 1693
    db $AE, $17, $AE, $17, $AE, $17, $AE, $17
    db $0B, $1A, $15, $1A, $AE, $17, $AE, $17
    db $10, $17, $1D, $18, $28, $18, $33, $18
    db $1B, $17, $AE, $17, $AE, $17, $AE, $17
    db $AE, $17, $26, $17, $AE, $17, $31, $17
    db $17, $19, $A6, $18, $AE, $17, $3C, $17
    db $47, $17, $40, $18, $4B, $18, $56, $18
    db $AE, $17, $AE, $17, $AE, $17, $AE, $17
    db $AE, $17, $71, $18, $52, $17, $64, $17
    db $80, $17, $92, $17, $AE, $17, $AE, $17
    db $AE, $17, $AE, $17, $AE, $17, $AE, $17
    db $AE, $17, $AE, $17, $AE, $17, $AE, $17
    db $AE, $17, $0B, $18, $0B, $18, $0B, $18
    db $0B, $18, $14, $18, $14, $18, $14, $18
    db $14, $18, $AF, $17, $D3, $17, $C1, $17
    db $EF, $17
    bbs $2D.6,spcb_171A               ; 1710
    set1 $2A.2                        ; 1713
    mov a,#$25                        ; 1715
    call SpcB_12FA                    ; 1717
spcb_171A:
    ret                               ; 171A
    bbs $2D.6,spcb_1725               ; 171B
    set1 $2A.4                        ; 171E
    mov a,#$15                        ; 1720
    call SpcB_12FA                    ; 1722
spcb_1725:
    ret                               ; 1725
    bbs $2D.6,spcb_1730               ; 1726
    set1 $2A.0                        ; 1729
    mov a,#$17                        ; 172B
    call SpcB_12FA                    ; 172D
spcb_1730:
    ret                               ; 1730
    bbs $2D.6,spcb_173B               ; 1731
    set1 $2A.3                        ; 1734
    mov a,#$A5                        ; 1736
    call SpcB_12FA                    ; 1738
spcb_173B:
    ret                               ; 173B
    bbs $2D.6,spcb_1746               ; 173C
    set1 $2A.5                        ; 173F
    mov a,#$95                        ; 1741
    call SpcB_12FA                    ; 1743
spcb_1746:
    ret                               ; 1746
    bbs $2D.6,spcb_1751               ; 1747
    set1 $2A.1                        ; 174A
    mov a,#$97                        ; 174C
    call SpcB_12FA                    ; 174E
spcb_1751:
    ret                               ; 1751
    bbs $29.2,spcb_17AE               ; 1752
    set1 $29.2                        ; 1755
    clr1 $29.3                        ; 1757
    mov a,#$00                        ; 1759
    mov y,#$02                        ; 175B
    mov.w $0549,a                     ; 175D
    mov.w $054B,y                     ; 1760
    ret                               ; 1763
    bbc $29.2,spcb_17AE               ; 1764
    clr1 $29.2                        ; 1767
    set1 $29.3                        ; 1769
    mov.w a,$0549                     ; 176B
    mov.w y,$054B                     ; 176E
    mov $20,#$00                      ; 1771
    mov $21,#$02                      ; 1774
    subw ya,$20                       ; 1777
    mov.w $0549,a                     ; 1779
    mov.w $054B,y                     ; 177C
    ret                               ; 177F
    bbs $29.4,spcb_17AE               ; 1780
    set1 $29.4                        ; 1783
    clr1 $29.5                        ; 1785
    mov a,#$00                        ; 1787
    mov y,#$02                        ; 1789
    mov.w $0548,a                     ; 178B
    mov.w $054A,y                     ; 178E
    ret                               ; 1791
    bbc $29.4,spcb_17AE               ; 1792
    clr1 $29.4                        ; 1795
    set1 $29.5                        ; 1797
    mov.w a,$0548                     ; 1799
    mov.w y,$054A                     ; 179C
    mov $20,#$00                      ; 179F
    mov $21,#$02                      ; 17A2
    subw ya,$20                       ; 17A5
    mov.w $0548,a                     ; 17A7
    mov.w $054A,y                     ; 17AA
    ret                               ; 17AD
spcb_17AE:
    ret                               ; 17AE
    bbs $28.2,spcb_17AE               ; 17AF
    set1 $28.2                        ; 17B2
    clr1 $28.3                        ; 17B4
    mov a,#$40                        ; 17B6
    mov y,#$00                        ; 17B8
    mov.w $054D,a                     ; 17BA
    mov.w $054F,y                     ; 17BD
    ret                               ; 17C0
    bbs $28.4,spcb_17AE               ; 17C1
    set1 $28.4                        ; 17C4
    clr1 $28.5                        ; 17C6
    mov a,#$40                        ; 17C8
    mov y,#$00                        ; 17CA
    mov.w $054C,a                     ; 17CC
    mov.w $054E,y                     ; 17CF
    ret                               ; 17D2
    bbc $28.2,spcb_17AE               ; 17D3
    clr1 $28.2                        ; 17D6
    set1 $28.3                        ; 17D8
    mov.w a,$054D                     ; 17DA
    mov.w y,$054F                     ; 17DD
    mov $20,#$40                      ; 17E0
    mov $21,#$00                      ; 17E3
    subw ya,$20                       ; 17E6
    mov.w $054D,a                     ; 17E8
    mov.w $054F,y                     ; 17EB
    ret                               ; 17EE
    bbc $28.4,spcb_17AE               ; 17EF
    clr1 $28.4                        ; 17F2
    set1 $28.5                        ; 17F4
    mov.w a,$054C                     ; 17F6
    mov.w y,$054E                     ; 17F9
    mov $20,#$40                      ; 17FC
    mov $21,#$00                      ; 17FF
    subw ya,$20                       ; 1802
    mov.w $054C,a                     ; 1804
    mov.w $054E,y                     ; 1807
    ret                               ; 180A
    mov.b a,$20                       ; 180B
    setc                              ; 180D
    sbc a,#$32                        ; 180E
    mov.w $0530,a                     ; 1810
    ret                               ; 1813
    mov.b a,$20                       ; 1814
    setc                              ; 1816
    sbc a,#$36                        ; 1817
    mov.w $0531,a                     ; 1819
    ret                               ; 181C
spcb_181D:
    clr1 $2A.2                        ; 181D
    mov.w a,$0395                     ; 181F
    cmp a,#$25                        ; 1822
    bne spcb_186B                     ; 1824
    bra spcb_183C                     ; 1826
spcb_1828:
    clr1 $2A.4                        ; 1828
    mov.w a,$0395                     ; 182A
    cmp a,#$15                        ; 182D
    bne spcb_186B                     ; 182F
    bra spcb_183C                     ; 1831
spcb_1833:
    clr1 $2A.0                        ; 1833
    mov.w a,$0395                     ; 1835
    cmp a,#$17                        ; 1838
    bne spcb_186B                     ; 183A
spcb_183C:
    mov x,#$05                        ; 183C
    bra spcb_1861                     ; 183E
spcb_1840:
    clr1 $2A.3                        ; 1840
    mov.w a,$0395                     ; 1842
    cmp a,#$25                        ; 1845
    bne spcb_186B                     ; 1847
    bra spcb_185F                     ; 1849
spcb_184B:
    clr1 $2A.5                        ; 184B
    mov.w a,$0395                     ; 184D
    cmp a,#$15                        ; 1850
    bne spcb_186B                     ; 1852
    bra spcb_185F                     ; 1854
spcb_1856:
    clr1 $2A.1                        ; 1856
    mov.w a,$0395                     ; 1858
    cmp a,#$17                        ; 185B
    bne spcb_186B                     ; 185D
spcb_185F:
    mov x,#$05                        ; 185F
spcb_1861:
    mov.w a,$0408+x                   ; 1861
    beq spcb_186B                     ; 1864
    mov.b a,$20                       ; 1866
    mov.w $03E0+x,a                   ; 1868
spcb_186B:
    ret                               ; 186B

%spc($186C, $1AF851)
SpcB_186C:
    mov $20,#$01                      ; 186C
    bra spcb_1861                     ; 186F
    bbc $29.0,spcb_186B               ; 1871
    mov a,#$4C                        ; 1874
    call SpcB_12FA                    ; 1876
    clr1 $29.0                        ; 1879
    mov a,#$00                        ; 187B
    mov.w $045F,a                     ; 187D
    mov.w $045E,a                     ; 1880
    mov.w $045D,a                     ; 1883
    mov.w $045C,a                     ; 1886
    mov.w $04FF,a                     ; 1889
    mov.w $04FE,a                     ; 188C
    mov.w $04FD,a                     ; 188F
    mov.w $04FC,a                     ; 1892
    clr1 $29.2                        ; 1895
    clr1 $29.3                        ; 1897
    clr1 $29.4                        ; 1899
    clr1 $29.5                        ; 189B
    call $0A60                        ; 189D
    call SpcB_15DD                    ; 18A0
    jmp spcb_162A                     ; 18A3
    bbc $29.0,spcb_186B               ; 18A6
    mov $2A,$2B                       ; 18A9
    mov a,#$4B                        ; 18AC
    call SpcB_12FA                    ; 18AE
    clr1 $29.0                        ; 18B1
    clr1 $2D.6                        ; 18B3
    mov x,#$07                        ; 18B5
    call SpcB_18D3                    ; 18B7
    mov x,#$06                        ; 18BA
    call SpcB_18D3                    ; 18BC
    mov x,#$05                        ; 18BF
    call SpcB_18D3                    ; 18C1
    mov x,#$04                        ; 18C4
    call SpcB_18D3                    ; 18C6
    mov x,#$01                        ; 18C9
    call SpcB_186C                    ; 18CB
    mov x,#$00                        ; 18CE
    jmp SpcB_186C                     ; 18D0

%spc($18D3, $1AF8B8)
SpcB_18D3:
    mov.w a,$0450+x                   ; 18D3
    cmp a,#$01                        ; 18D6
    beq spcb_18DF                     ; 18D8
    cmp a,#$02                        ; 18DA
    beq spcb_18E3                     ; 18DC
    ret                               ; 18DE
spcb_18DF:
    clr1 $00.7                        ; 18DF
    bra spcb_18E5                     ; 18E1
spcb_18E3:
    set1 $00.7                        ; 18E3
spcb_18E5:
    mov a,#$00                        ; 18E5
    mov.w $0450+x,a                   ; 18E7
    mov.w a,$0460+x                   ; 18EA
    mov.w $03C8+x,a                   ; 18ED
    mov.w a,$0458+x                   ; 18F0
    beq spcb_1914                     ; 18F3
    push a                            ; 18F5
    call SpcB_1905                    ; 18F6
    mov a,#$00                        ; 18F9
    mov.w $0458+x,a                   ; 18FB
    pop a                             ; 18FE
    push x                            ; 18FF
    call SpcB_2087                    ; 1900
    pop x                             ; 1903
    ret                               ; 1904

%spc($1905, $1AF8EA)
SpcB_1905:
    dec a                             ; 1905
    asl a                             ; 1906
    mov y,a                           ; 1907
    mov.w a,$0702+y                   ; 1908
    mov.b $24,a                       ; 190B
    inc y                             ; 190D
    mov.w a,$0702+y                   ; 190E
    mov.b $25,a                       ; 1911
    ret                               ; 1913
spcb_1914:
    jmp SpcB_186C                     ; 1914
    bbs $29.0,spcb_1956               ; 1917
    mov $2B,$2A                       ; 191A
    mov $2A,#$00                      ; 191D
    set1 $29.0                        ; 1920
    set1 $2D.6                        ; 1922
    mov a,#$4A                        ; 1924
    call SpcB_12FA                    ; 1926
    mov x,#$07                        ; 1929
    call SpcB_1951                    ; 192B
    mov x,#$06                        ; 192E
    call SpcB_1951                    ; 1930
    mov x,#$05                        ; 1933
    call SpcB_1951                    ; 1935
    mov x,#$04                        ; 1938
    call SpcB_1951                    ; 193A
    mov x,#$07                        ; 193D
    call SpcB_186C                    ; 193F
    mov x,#$06                        ; 1942
    call SpcB_186C                    ; 1944
    mov x,#$05                        ; 1947
    call SpcB_186C                    ; 1949
    mov x,#$04                        ; 194C
    jmp SpcB_186C                     ; 194E

%spc($1951, $1AF936)
SpcB_1951:
    mov.w a,$04F8+x                   ; 1951
    beq spcb_196E                     ; 1954
spcb_1956:
    mov.w a,$04F8+x                   ; 1956
    mov.w $0458+x,a                   ; 1959
    mov.w a,$0528+x                   ; 195C
    mov.w $0450+x,a                   ; 195F
    mov a,#$00                        ; 1962
    mov.w $04F8+x,a                   ; 1964
    mov.w $0480+x,a                   ; 1967
    mov.w $0528+x,a                   ; 196A
    ret                               ; 196D
spcb_196E:
    mov.w a,$0480+x                   ; 196E
    beq spcb_198E                     ; 1971
    cmp a,#$02                        ; 1973
    beq spcb_1956                     ; 1975
    mov.w a,$04B8+x                   ; 1977
    mov.w $0450+x,a                   ; 197A
    mov.w a,$03C8+x                   ; 197D
    mov.w $0460+x,a                   ; 1980
    mov.w a,$0390+x                   ; 1983
    mov.w $0458+x,a                   ; 1986
    mov a,#$00                        ; 1989
    mov.w $0480+x,a                   ; 198B
spcb_198E:
    ret                               ; 198E

%spc($198F, $1AF974)
SpcB_198F:
    mov $2A,#$00                      ; 198F
    call SpcB_19B6                    ; 1992
    mov x,#$07                        ; 1995
spcb_1997:
    call SpcB_199E                    ; 1997
    dec x                             ; 199A
    bpl spcb_1997                     ; 199B
    ret                               ; 199D

%spc($199E, $1AF983)
SpcB_199E:
    mov a,#$00                        ; 199E
    mov.w $04F8+x,a                   ; 19A0
    mov.w $0480+x,a                   ; 19A3
    mov.b $2A,a                       ; 19A6
    mov.w a,$0408+x                   ; 19A8
    beq spcb_19B5                     ; 19AB
    mov a,#$01                        ; 19AD
    mov.w $04C0+x,a                   ; 19AF
    mov.w $04C8+x,a                   ; 19B2
spcb_19B5:
    ret                               ; 19B5

%spc($19B6, $1AF99B)
SpcB_19B6:
    bbs $29.0,spcb_19BB               ; 19B6
    set1 $29.6                        ; 19B9
spcb_19BB:
    ret                               ; 19BB

%spc($19BC, $1AF9A1)
SpcB_19BC:
    bbc $29.6,spcb_1A02               ; 19BC
    mov a,#$F0                        ; 19BF
    mov y,#$02                        ; 19C1
    addw ya,$2E                       ; 19C3
    mov.b $2E,a                       ; 19C5
    mov.b $2F,y                       ; 19C7
    mov a,#$00                        ; 19C9
    mov.w y,$0564                     ; 19CB
    subw ya,$2E                       ; 19CE
    mov.w $0565,y                     ; 19D0
    cmp y,#$00                        ; 19D3
    beq spcb_19E4                     ; 19D5
    cmp y,#$01                        ; 19D7
    beq spcb_19E4                     ; 19D9
    cmp y,#$02                        ; 19DB
    beq spcb_19E4                     ; 19DD
    cmp y,#$03                        ; 19DF
    beq spcb_19E4                     ; 19E1
    ret                               ; 19E3
spcb_19E4:
    mov a,#$00                        ; 19E4
    mov.b $2E,a                       ; 19E6
    mov.b $2F,a                       ; 19E8
    clr1 $29.6                        ; 19EA
    bbs $2C.7,spcb_19F5               ; 19EC
    bbs $2D.0,spcb_1A21               ; 19EF
    jmp $0A60                         ; 19F2
spcb_19F5:
    call SpcB_19FB                    ; 19F5
    jmp $0675                         ; 19F8

%spc($19FB, $1AF9E0)
SpcB_19FB:
    mov a,#$FF                        ; 19FB
    mov y,#$5C                        ; 19FD
    jmp $09E8                         ; 19FF
spcb_1A02:
    mov.b a,$59                       ; 1A02
    mov.w $0564,a                     ; 1A04
    mov.w $0565,a                     ; 1A07
    ret                               ; 1A0A
    call SpcB_198F                    ; 1A0B
    mov.b a,$59                       ; 1A0E
    beq spcb_19F5                     ; 1A10
    set1 $2C.7                        ; 1A12
    ret                               ; 1A14
    db $02, $2D, $3F, $8F, $19, $E4, $59, $F0
    db $03, $02, $2D, $6F
spcb_1A21:
    call SpcB_19FB                    ; 1A21
    mov.w a,$00F1                     ; 1A24
    or a,#$80                         ; 1A27
    mov.w $00F1,a                     ; 1A29
    jmp $FFC0                         ; 1A2C

%spc($1A2F, $1AFA14)
SpcB_1A2F:
    mov.b a,$1D                       ; 1A2F
    beq spcb_1A79                     ; 1A31
    mov y,#$00                        ; 1A33
spcb_1A35:
    mov.w a,$25FF+y                   ; 1A35
    beq spcb_1A41                     ; 1A38
    cmp.b a,$1D                       ; 1A3A
    beq spcb_1A7C                     ; 1A3C
    inc y                             ; 1A3E
    bra spcb_1A35                     ; 1A3F
spcb_1A41:
    mov y,#$00                        ; 1A41
spcb_1A43:
    mov.w a,$2608+y                   ; 1A43
    beq spcb_1A7C                     ; 1A46
    push a                            ; 1A48
    mov.b a,$1D                       ; 1A49
    push y                            ; 1A4B
    call SpcB_1B03                    ; 1A4C
    call SpcB_1B17                    ; 1A4F
    pop y                             ; 1A52
    pop a                             ; 1A53
    cmp.w a,$0390+x                   ; 1A54
    beq spcb_1A5C                     ; 1A57
spcb_1A59:
    inc y                             ; 1A59
    bra spcb_1A43                     ; 1A5A
spcb_1A5C:
    mov x,#$00                        ; 1A5C
spcb_1A5E:
    mov.w a,$2608+x                   ; 1A5E
    beq spcb_1A72                     ; 1A61
    cmp.b a,$1D                       ; 1A63
    beq spcb_1A6A                     ; 1A65
    inc x                             ; 1A67
    bra spcb_1A5E                     ; 1A68
spcb_1A6A:
    mov.b $20,x                       ; 1A6A
    mov a,y                           ; 1A6C
    setc                              ; 1A6D
    sbc.b a,$20                       ; 1A6E
    bpl spcb_1A59                     ; 1A70
spcb_1A72:
    mov $1F,$1D                       ; 1A72
    mov $1D,#$00                      ; 1A75
    ret                               ; 1A78
spcb_1A79:
    mov $1F,#$00                      ; 1A79
spcb_1A7C:
    ret                               ; 1A7C
    db $15, $1E
spcb_1A7F:
    cmp $1D,#$00                      ; 1A7F
    beq spcb_1A8B                     ; 1A82
    mov a,#$00                        ; 1A84
    mov.w $057B,a                     ; 1A86
    bra spcb_1A95                     ; 1A89
spcb_1A8B:
    mov.w a,$057B                     ; 1A8B
    cmp a,#$FF                        ; 1A8E
    beq spcb_1A95                     ; 1A90
    inc.w $057B                       ; 1A92
spcb_1A95:
    cmp $1D,#$00                      ; 1A95
    bne spcb_1ACA                     ; 1A98
    mov.w a,$057B                     ; 1A9A
    beq spcb_1ACA                     ; 1A9D
    cmp a,#$01                        ; 1A9F
    beq spcb_1ACA                     ; 1AA1
    cmp a,#$02                        ; 1AA3
    beq spcb_1ACA                     ; 1AA5
    mov.w a,$0588                     ; 1AA7
    beq spcb_1ACA                     ; 1AAA
    push a                            ; 1AAC
    mov x,#$01                        ; 1AAD
spcb_1AAF:
    mov.w a,$0588+x                   ; 1AAF
    dec x                             ; 1AB2
    mov.w $0588+x,a                   ; 1AB3
    inc x                             ; 1AB6
    inc x                             ; 1AB7
    cmp x,#$08                        ; 1AB8
    bne spcb_1AAF                     ; 1ABA
    pop a                             ; 1ABC
    asl a                             ; 1ABD
    bcc spcb_1AC2                     ; 1ABE
    set1 $00.7                        ; 1AC0
spcb_1AC2:
    lsr a                             ; 1AC2
    mov x,#$00                        ; 1AC3
    mov.w $057B,x                     ; 1AC5
    bra spcb_1AD9                     ; 1AC8
spcb_1ACA:
    mov.b a,$1D                       ; 1ACA
    cmp.b a,$1E                       ; 1ACC
    bne spcb_1AD3                     ; 1ACE
spcb_1AD0:
    jmp SpcB_1B7E                     ; 1AD0
spcb_1AD3:
    mov.b $1E,a                       ; 1AD3
    mov.b a,$1D                       ; 1AD5
    beq spcb_1AD0                     ; 1AD7
spcb_1AD9:
    mov.b $23,a                       ; 1AD9
    call SpcB_1B03                    ; 1ADB
    call SpcB_1B17                    ; 1ADE
    call SpcB_1B48                    ; 1AE1
    call SpcB_1B71                    ; 1AE4
    call SpcB_1AFD                    ; 1AE7
    call SpcB_1B9E                    ; 1AEA
    mov.b a,$23                       ; 1AED
    mov.w $0410+x,a                   ; 1AEF
    push x                            ; 1AF2
    call SpcB_1B7E                    ; 1AF3
    pop x                             ; 1AF6
    mov a,#$00                        ; 1AF7
    mov.w $0410+x,a                   ; 1AF9
    ret                               ; 1AFC

%spc($1AFD, $1AFAE2)
SpcB_1AFD:
    mov a,#$01                        ; 1AFD
    mov.w $0408+x,a                   ; 1AFF
    ret                               ; 1B02

%spc($1B03, $1AFAE8)
SpcB_1B03:
    dec a                             ; 1B03
    asl a                             ; 1B04
    mov y,a                           ; 1B05
    mov.w a,$0702+y                   ; 1B06
    mov.b $20,a                       ; 1B09
    inc y                             ; 1B0B
    mov.w a,$0702+y                   ; 1B0C
    mov.b $21,a                       ; 1B0F
    mov y,#$00                        ; 1B11
    mov a,($20)+y                     ; 1B13
    mov x,a                           ; 1B15
    ret                               ; 1B16

%spc($1B17, $1AFAFC)
SpcB_1B17:
    mov.b $26,x                       ; 1B17
    mov a,x                           ; 1B19
    and a,#$0F                        ; 1B1A
    mov x,a                           ; 1B1C
    bbc $26.6,spcb_1B24               ; 1B1D
    bbc $00.7,spcb_1B24               ; 1B20
    dec x                             ; 1B23
spcb_1B24:
    cmp $23,#$79                      ; 1B24
    beq spcb_1B39                     ; 1B27
    cmp $23,#$78                      ; 1B29
    beq spcb_1B39                     ; 1B2C
    cmp $23,#$77                      ; 1B2E
    beq spcb_1B39                     ; 1B31
    cmp $23,#$7A                      ; 1B33
    beq spcb_1B39                     ; 1B36
    ret                               ; 1B38
spcb_1B39:
    bbs $00.7,spcb_1B42               ; 1B39
    mov a,#$1C                        ; 1B3C
    mov.w $03C8+x,a                   ; 1B3E
    ret                               ; 1B41
spcb_1B42:
    mov a,#$20                        ; 1B42
    mov.w $03C8+x,a                   ; 1B44
    ret                               ; 1B47

%spc($1B48, $1AFB2D)
SpcB_1B48:
    bbc $26.7,spcb_1B58               ; 1B48
    bbc $26.5,spcb_1B52               ; 1B4B
    mov a,#$02                        ; 1B4E
    bra spcb_1B54                     ; 1B50
spcb_1B52:
    mov a,#$01                        ; 1B52
spcb_1B54:
    mov.w $0480+x,a                   ; 1B54
    ret                               ; 1B57
spcb_1B58:
    mov.w a,$0480+x                   ; 1B58
    cmp a,#$01                        ; 1B5B
    bne spcb_1B70                     ; 1B5D
    mov.w a,$04F8+x                   ; 1B5F
    bne spcb_1B70                     ; 1B62
    mov.w a,$0390+x                   ; 1B64
    mov.w $04F8+x,a                   ; 1B67
    mov.w a,$04B8+x                   ; 1B6A
    mov.w $0528+x,a                   ; 1B6D
spcb_1B70:
    ret                               ; 1B70

%spc($1B71, $1AFB56)
SpcB_1B71:
    bbs $00.7,spcb_1B78               ; 1B71
    mov a,#$01                        ; 1B74
    bra spcb_1B7A                     ; 1B76
spcb_1B78:
    mov a,#$02                        ; 1B78
spcb_1B7A:
    mov.w $04B8+x,a                   ; 1B7A
    ret                               ; 1B7D

%spc($1B7E, $1AFB63)
SpcB_1B7E:
    mov x,#$00                        ; 1B7E
spcb_1B80:
    mov.w a,$0408+x                   ; 1B80
    beq spcb_1B8B                     ; 1B83
    call SpcB_1B93                    ; 1B85
    call SpcB_1BA9                    ; 1B88
spcb_1B8B:
    inc x                             ; 1B8B
    cmp x,#$08                        ; 1B8C
    beq spcb_1B92                     ; 1B8E
    bra spcb_1B80                     ; 1B90
spcb_1B92:
    ret                               ; 1B92

%spc($1B93, $1AFB78)
SpcB_1B93:
    mov.w a,$0488+x                   ; 1B93
    mov.b $20,a                       ; 1B96
    mov.w a,$0490+x                   ; 1B98
    mov.b $21,a                       ; 1B9B
    ret                               ; 1B9D

%spc($1B9E, $1AFB83)
SpcB_1B9E:
    mov.b a,$20                       ; 1B9E
    mov.w $0488+x,a                   ; 1BA0
    mov.b a,$21                       ; 1BA3
    mov.w $0490+x,a                   ; 1BA5
    ret                               ; 1BA8

%spc($1BA9, $1AFB8E)
SpcB_1BA9:
    mov.w a,$03F8+x                   ; 1BA9
    beq spcb_1C01                     ; 1BAC
    cmp a,#$01                        ; 1BAE
    beq spcb_1BCD                     ; 1BB0
    cmp a,#$02                        ; 1BB2
    beq spcb_1BE2                     ; 1BB4
    cmp a,#$03                        ; 1BB6
    beq spcb_1BE7                     ; 1BB8
    mov.w a,$0410+x                   ; 1BBA
    bne spcb_1C04                     ; 1BBD
    mov a,#$00                        ; 1BBF
    mov.w $0390+x,a                   ; 1BC1
    mov.w $03F8+x,a                   ; 1BC4
    mov.w $0408+x,a                   ; 1BC7
    jmp spcb_1CEE                     ; 1BCA
spcb_1BCD:
    mov.w a,$0410+x                   ; 1BCD
    bne spcb_1C04                     ; 1BD0
    mov a,#$02                        ; 1BD2
    mov.w $03F8+x,a                   ; 1BD4
    call SpcB_1D46                    ; 1BD7
    mov a,#$00                        ; 1BDA
    mov.w $03E8+x,a                   ; 1BDC
spcb_1BDF:
    jmp spcb_1D5A                     ; 1BDF
spcb_1BE2:
    mov a,#$00                        ; 1BE2
    mov.w $03E8+x,a                   ; 1BE4
spcb_1BE7:
    mov.w a,$0410+x                   ; 1BE7
    bne spcb_1C04                     ; 1BEA
    mov a,#$03                        ; 1BEC
    mov.w $03F8+x,a                   ; 1BEE
    mov.w a,$03E8+x                   ; 1BF1
    beq spcb_1C1D                     ; 1BF4
    cmp a,#$01                        ; 1BF6
    beq spcb_1BCD                     ; 1BF8
    mov a,#$00                        ; 1BFA
    mov.w $03F0+x,a                   ; 1BFC
    bra spcb_1BCD                     ; 1BFF
spcb_1C01:
    mov.w a,$0410+x                   ; 1C01
spcb_1C04:
    mov.w $0390+x,a                   ; 1C04
    mov a,#$01                        ; 1C07
    mov.w $03F8+x,a                   ; 1C09
    mov a,#$00                        ; 1C0C
    mov.w $03F0+x,a                   ; 1C0E
    mov.w $03E0+x,a                   ; 1C11
    call SpcB_1CCC                    ; 1C14
    jmp SpcB_1D50                     ; 1C17
spcb_1C1A:
    jmp spcb_1C87                     ; 1C1A
spcb_1C1D:
    mov.w a,$03E0+x                   ; 1C1D
    beq spcb_1BDF                     ; 1C20
    cmp a,#$01                        ; 1C22
    beq spcb_1C1A                     ; 1C24
    cmp a,#$0A                        ; 1C26
    beq spcb_1C45                     ; 1C28
    cmp a,#$1A                        ; 1C2A
    beq spcb_1C45                     ; 1C2C
    cmp a,#$0B                        ; 1C2E
    beq spcb_1C4E                     ; 1C30
    cmp a,#$1B                        ; 1C32
    beq spcb_1C4E                     ; 1C34
    cmp a,#$0C                        ; 1C36
    beq spcb_1C57                     ; 1C38
    cmp a,#$1C                        ; 1C3A
    beq spcb_1C57                     ; 1C3C
spcb_1C3E:
    mov a,#$00                        ; 1C3E
    mov.w $03E0+x,a                   ; 1C40
    bra spcb_1BDF                     ; 1C43
spcb_1C45:
    mov.w a,$0390+x                   ; 1C45
    cmp a,#$25                        ; 1C48
    beq spcb_1C6D                     ; 1C4A
    bra spcb_1C3E                     ; 1C4C
spcb_1C4E:
    mov.w a,$0390+x                   ; 1C4E
    cmp a,#$15                        ; 1C51
    beq spcb_1C6D                     ; 1C53
    bra spcb_1C3E                     ; 1C55
spcb_1C57:
    mov.w a,$0390+x                   ; 1C57
    cmp a,#$17                        ; 1C5A
    beq spcb_1C6D                     ; 1C5C
    bra spcb_1C3E                     ; 1C5E
    mov.w a,$0390+x                   ; 1C60
    cmp a,#$05                        ; 1C63
    beq spcb_1C87                     ; 1C65
    cmp a,#$08                        ; 1C67
    beq spcb_1C87                     ; 1C69
    bra spcb_1C3E                     ; 1C6B
spcb_1C6D:
    mov a,#$00                        ; 1C6D
    mov.w $03E0+x,a                   ; 1C6F
    mov.w a,$04F8+x                   ; 1C72
    beq spcb_1C7F                     ; 1C75
    mov a,#$00                        ; 1C77
    mov.w $04F8+x,a                   ; 1C79
    jmp spcb_1BDF                     ; 1C7C
spcb_1C7F:
    mov.w a,$0480+x                   ; 1C7F
    bne spcb_1CAC                     ; 1C82
    jmp spcb_1BDF                     ; 1C84
spcb_1C87:
    mov a,#$00                        ; 1C87
    mov.w $03E0+x,a                   ; 1C89
    mov.w a,$04F8+x                   ; 1C8C
    beq spcb_1CAC                     ; 1C8F
    push a                            ; 1C91
    mov.w a,$0528+x                   ; 1C92
    cmp a,#$01                        ; 1C95
    beq spcb_1CA0                     ; 1C97
    cmp a,#$02                        ; 1C99
    beq spcb_1CA5                     ; 1C9B
    pop a                             ; 1C9D
    bra spcb_1CAC                     ; 1C9E
spcb_1CA0:
    pop a                             ; 1CA0
    asl a                             ; 1CA1
    setc                              ; 1CA2
    bra spcb_1CA8                     ; 1CA3
spcb_1CA5:
    pop a                             ; 1CA5
    asl a                             ; 1CA6
    clrc                              ; 1CA7
spcb_1CA8:
    ror a                             ; 1CA8
    call SpcB_12FA                    ; 1CA9
spcb_1CAC:
    mov a,#$04                        ; 1CAC
    mov.w $03F8+x,a                   ; 1CAE
    call SpcB_1D50                    ; 1CB1
    mov a,#$00                        ; 1CB4
    mov.w $0480+x,a                   ; 1CB6
    mov.w $04F8+x,a                   ; 1CB9
    mov.w $03F0+x,a                   ; 1CBC
    mov.w $03C8+x,a                   ; 1CBF
    mov.w $04B8+x,a                   ; 1CC2
    mov.w $0528+x,a                   ; 1CC5
    mov.w $04F0+x,a                   ; 1CC8
    ret                               ; 1CCB

%spc($1CCC, $1AFCB1)
SpcB_1CCC:
    call SpcB_1CD6                    ; 1CCC
    mov.b a,$1A                       ; 1CCF
    or.b a,$22                        ; 1CD1
    mov.b $1A,a                       ; 1CD3
    ret                               ; 1CD5

%spc($1CD6, $1AFCBB)
SpcB_1CD6:
    mov.b $22,x                       ; 1CD6
    inc.b $22                         ; 1CD8
    mov a,#$00                        ; 1CDA
    setc                              ; 1CDC
spcb_1CDD:
    rol a                             ; 1CDD
    dec.b $22                         ; 1CDE
    bne spcb_1CDD                     ; 1CE0
    mov.b $22,a                       ; 1CE2
    ret                               ; 1CE4

%spc($1CE5, $1AFCCA)
SpcB_1CE5:
    mov.b $22,x                       ; 1CE5
    inc.b $22                         ; 1CE7
    mov a,#$FF                        ; 1CE9
    clrc                              ; 1CEB
    bra spcb_1CDD                     ; 1CEC
spcb_1CEE:
    call SpcB_1CE5                    ; 1CEE
    mov.b a,$1A                       ; 1CF1
    and.b a,$22                       ; 1CF3
    mov.b $1A,a                       ; 1CF5
    push x                            ; 1CF7
    clrc                              ; 1CF8
    mov a,x                           ; 1CF9
    asl a                             ; 1CFA
    mov x,a                           ; 1CFB
    mov.w a,$0211+x                   ; 1CFC
    call $0BC1                        ; 1CFF
    pop x                             ; 1D02
    mov.b $20,x                       ; 1D03
    inc.b $20                         ; 1D05
    mov.b a,$4F                       ; 1D07
spcb_1D09:
    lsr a                             ; 1D09
    dec.b $20                         ; 1D0A
    bne spcb_1D09                     ; 1D0C
    bcc spcb_1D1B                     ; 1D0E
    call SpcB_1CD6                    ; 1D10
    mov.b a,$4A                       ; 1D13
    or.b a,$22                        ; 1D15
    mov.b $4A,a                       ; 1D17
    bra spcb_1D24                     ; 1D19
spcb_1D1B:
    call SpcB_1CE5                    ; 1D1B
    mov.b a,$4A                       ; 1D1E
    and.b a,$22                       ; 1D20
    mov.b $4A,a                       ; 1D22
spcb_1D24:
    call SpcB_1CE5                    ; 1D24
    mov.b a,$1A                       ; 1D27
    and.b a,$22                       ; 1D29
    mov.b $1A,a                       ; 1D2B
    call SpcB_1CD6                    ; 1D2D
    mov.b a,$47                       ; 1D30
    or.b a,$22                        ; 1D32
    mov.b $47,a                       ; 1D34
    mov.b a,$5E                       ; 1D36
    or.b a,$20                        ; 1D38
    mov.b $5E,a                       ; 1D3A
    push x                            ; 1D3C
    clrc                              ; 1D3D
    mov a,x                           ; 1D3E
    asl a                             ; 1D3F
    mov x,a                           ; 1D40
    call $0E97                        ; 1D41
    pop x                             ; 1D44
    ret                               ; 1D45

%spc($1D46, $1AFD2B)
SpcB_1D46:
    call SpcB_1CD6                    ; 1D46
    mov.b a,$45                       ; 1D49
    or.b a,$22                        ; 1D4B
    mov.b $45,a                       ; 1D4D
    ret                               ; 1D4F

%spc($1D50, $1AFD35)
SpcB_1D50:
    call SpcB_1CD6                    ; 1D50
    mov.b a,$46                       ; 1D53
    or.b a,$22                        ; 1D55
    mov.b $46,a                       ; 1D57
    ret                               ; 1D59
spcb_1D5A:
    call SpcB_1D63                    ; 1D5A
    call SpcB_231E                    ; 1D5D
    jmp spcb_21C8                     ; 1D60

%spc($1D63, $1AFD48)
SpcB_1D63:
    mov.w a,$0390+x                   ; 1D63
    beq spcb_1D86                     ; 1D66
    mov.w a,$0448+x                   ; 1D68
    beq spcb_1D79                     ; 1D6B
    mov.b $20,a                       ; 1D6D
    mov.w a,$03F0+x                   ; 1D6F
    cmp.b a,$20                       ; 1D72
    bne spcb_1D79                     ; 1D74
    call SpcB_186C                    ; 1D76
spcb_1D79:
    mov.w a,$03F0+x                   ; 1D79
    cmp a,#$FF                        ; 1D7C
    beq spcb_1D84                     ; 1D7E
    inc a                             ; 1D80
    mov.w $03F0+x,a                   ; 1D81
spcb_1D84:
    bra spcb_1D8C                     ; 1D84
spcb_1D86:
    mov a,#$00                        ; 1D86
    mov.w $03F0+x,a                   ; 1D88
    ret                               ; 1D8B
spcb_1D8C:
    call SpcB_1B93                    ; 1D8C
    mov.w a,$03F0+x                   ; 1D8F
    beq spcb_1D9A                     ; 1D92
    cmp a,#$01                        ; 1D94
    beq spcb_1D9B                     ; 1D96
    bra spcb_1DED                     ; 1D98
spcb_1D9A:
    ret                               ; 1D9A
spcb_1D9B:
    mov a,#$00                        ; 1D9B
    mov.w $0400+x,a                   ; 1D9D
    mov.w $0438+x,a                   ; 1DA0
    mov.w $0520+x,a                   ; 1DA3
    mov.w $0478+x,a                   ; 1DA6
    mov.w $0448+x,a                   ; 1DA9
    mov.w $0440+x,a                   ; 1DAC
    mov.w $03A8+x,a                   ; 1DAF
    mov.w $03B0+x,a                   ; 1DB2
    mov.w $03B8+x,a                   ; 1DB5
    mov.w $0498+x,a                   ; 1DB8
    mov.w $03C0+x,a                   ; 1DBB
    mov.w $04A0+x,a                   ; 1DBE
    mov.w $04C0+x,a                   ; 1DC1
    mov.w $04C8+x,a                   ; 1DC4
    mov.w $04F0+x,a                   ; 1DC7
    mov.w $0398+x,a                   ; 1DCA
    mov.w $04A8+x,a                   ; 1DCD
    mov.w $03A0+x,a                   ; 1DD0
    mov.w $04B0+x,a                   ; 1DD3
    mov a,#$8F                        ; 1DD6
    mov.w $03D0+x,a                   ; 1DD8
    mov a,#$E0                        ; 1DDB
    mov.w $03D8+x,a                   ; 1DDD
    call SpcB_1CE5                    ; 1DE0
    mov.b a,$4A                       ; 1DE3
    and.b a,$22                       ; 1DE5
    mov.b $4A,a                       ; 1DE7
    mov y,#$01                        ; 1DE9
    bra spcb_1E05                     ; 1DEB
spcb_1DED:
    mov.w a,$0418+x                   ; 1DED
    dec a                             ; 1DF0
    mov.w $0418+x,a                   ; 1DF1
    beq spcb_1E00                     ; 1DF4
    cmp a,#$FF                        ; 1DF6
    beq spcb_1E00                     ; 1DF8
    jmp spcb_213F                     ; 1DFA
    db $5F, $7B, $21
spcb_1E00:
    mov.w a,$0400+x                   ; 1E00
    mov y,a                           ; 1E03
spcb_1E04:
    inc y                             ; 1E04
spcb_1E05:
    mov a,($20)+y                     ; 1E05
    beq spcb_1E04                     ; 1E07
    bpl spcb_1E56                     ; 1E09
    setc                              ; 1E0B
    sbc a,#$E0                        ; 1E0C
    bmi spcb_1E87                     ; 1E0E
    asl a                             ; 1E10
    push x                            ; 1E11
    mov x,a                           ; 1E12
    jmp ($1E16+x)                     ; 1E13
    db $B6, $20, $0E, $1F, $12, $1F, $1B, $1F
    db $6A, $1F, $D4, $1F, $E3, $1F, $FE, $1F
    db $12, $20, $27, $20, $3C, $20, $48, $20
    db $55, $20, $C0, $20, $E1, $20, $B6, $20
    db $B6, $20, $EB, $20, $F4, $20, $FD, $20
    db $B6, $20, $B6, $20, $B6, $20, $B6, $20
    db $0F, $21, $B6, $20, $B6, $20, $19, $21
    db $25, $21, $2F, $21, $68, $1F, $B6, $20
spcb_1E56:
    push a                            ; 1E56
    mov.w a,$0478+x                   ; 1E57
    beq spcb_1E62                     ; 1E5A
    cmp a,#$01                        ; 1E5C
    beq spcb_1E65                     ; 1E5E
    bra spcb_1E6A                     ; 1E60
spcb_1E62:
    call SpcB_1D50                    ; 1E62
spcb_1E65:
    mov a,#$01                        ; 1E65
    mov.w $03E8+x,a                   ; 1E67
spcb_1E6A:
    mov a,#$00                        ; 1E6A
    mov.w $0478+x,a                   ; 1E6C
    pop a                             ; 1E6F
    mov.w $0418+x,a                   ; 1E70
    cmp a,#$7F                        ; 1E73
    bne spcb_1E7B                     ; 1E75
    mov a,#$01                        ; 1E77
    bra spcb_1E7D                     ; 1E79
spcb_1E7B:
    mov a,#$00                        ; 1E7B
spcb_1E7D:
    mov.w $0420+x,a                   ; 1E7D
    mov a,y                           ; 1E80
    mov.w $0400+x,a                   ; 1E81
    jmp SpcB_1B9E                     ; 1E84
spcb_1E87:
    push y                            ; 1E87
    push x                            ; 1E88
    clrc                              ; 1E89
    adc a,#$60                        ; 1E8A
    mov y,#$00                        ; 1E8C
    mov x,#$0C                        ; 1E8E
    div ya,x                          ; 1E90
    push a                            ; 1E91
    mov a,y                           ; 1E92
    asl a                             ; 1E93
    mov x,a                           ; 1E94
    mov.w a,$222B+x                   ; 1E95
    mov.b $22,a                       ; 1E98
    mov.w a,$222C+x                   ; 1E9A
    mov.b $23,a                       ; 1E9D
    pop a                             ; 1E9F
    setc                              ; 1EA0
    sbc a,#$07                        ; 1EA1
    mov.b $24,a                       ; 1EA3
    beq spcb_1EB9                     ; 1EA5
spcb_1EA7:
    mov y,#$00                        ; 1EA7
    mov.b a,$23                       ; 1EA9
    mov x,#$02                        ; 1EAB
    div ya,x                          ; 1EAD
    mov.b $23,a                       ; 1EAE
    mov.b a,$22                       ; 1EB0
    div ya,x                          ; 1EB2
    mov.b $22,a                       ; 1EB3
    inc.b $24                         ; 1EB5
    bne spcb_1EA7                     ; 1EB7
spcb_1EB9:
    pop x                             ; 1EB9
    mov.b a,$22                       ; 1EBA
    mov.w $03B8+x,a                   ; 1EBC
    mov.w $0498+x,a                   ; 1EBF
    mov.b a,$23                       ; 1EC2
    mov.w $03C0+x,a                   ; 1EC4
    mov.w $04A0+x,a                   ; 1EC7
    mov.w a,$0440+x                   ; 1ECA
    beq spcb_1F0A                     ; 1ECD
    push x                            ; 1ECF
    mov.b $24,a                       ; 1ED0
    mov.b a,$23                       ; 1ED2
    setc                              ; 1ED4
    sbc a,#$10                        ; 1ED5
    bmi spcb_1EEB                     ; 1ED7
    mov.b a,$22                       ; 1ED9
    mov.b y,$23                       ; 1EDB
    mov x,#$43                        ; 1EDD
    div ya,x                          ; 1EDF
    mov y,#$04                        ; 1EE0
    mul ya                            ; 1EE2
    mov x,#$FC                        ; 1EE3
    div ya,x                          ; 1EE5
    mov.b y,$24                       ; 1EE6
    mul ya                            ; 1EE8
    bra spcb_1F00                     ; 1EE9
spcb_1EEB:
    mov.b a,$22                       ; 1EEB
    mov.b y,$23                       ; 1EED
    mov x,#$11                        ; 1EEF
    div ya,x                          ; 1EF1
    mov x,a                           ; 1EF2
    mov a,#$FF                        ; 1EF3
    mov y,#$00                        ; 1EF5
    div ya,x                          ; 1EF7
    mov x,a                           ; 1EF8
    mov y,#$00                        ; 1EF9
    mov.b a,$24                       ; 1EFB
    div ya,x                          ; 1EFD
    mov y,#$00                        ; 1EFE
spcb_1F00:
    addw ya,$22                       ; 1F00
    pop x                             ; 1F02
    mov.w $03B8+x,a                   ; 1F03
    mov a,y                           ; 1F06
    mov.w $03C0+x,a                   ; 1F07
spcb_1F0A:
    pop y                             ; 1F0A
    jmp spcb_1E04                     ; 1F0B
    mov a,#$01                        ; 1F0E
    bra spcb_1F14                     ; 1F10
    mov a,#$02                        ; 1F12
spcb_1F14:
    pop x                             ; 1F14
    mov.w $0478+x,a                   ; 1F15
    jmp spcb_1E04                     ; 1F18
    pop x                             ; 1F1B
    inc y                             ; 1F1C
    mov a,($20)+y                     ; 1F1D
    mov.w $03C8+x,a                   ; 1F1F
    jmp spcb_1E04                     ; 1F22

%spc($1F25, $1AFF0A)
SpcB_1F25:
    push x                            ; 1F25
    push y                            ; 1F26
    mov.b $22,a                       ; 1F27
    mov.w a,$04F0+x                   ; 1F29
    cmp a,#$04                        ; 1F2C
    bne spcb_1F64                     ; 1F2E
    mov.w a,$04B8+x                   ; 1F30
    cmp a,#$02                        ; 1F33
    beq spcb_1F3B                     ; 1F35
    mov.b a,$03                       ; 1F37
    bra spcb_1F3D                     ; 1F39
spcb_1F3B:
    mov.b a,$02                       ; 1F3B
spcb_1F3D:
    cmp a,#$00                        ; 1F3D
    bne spcb_1F42                     ; 1F3F
    inc a                             ; 1F41
spcb_1F42:
    mov y,#$03                        ; 1F42
    mul ya                            ; 1F44
    mov x,#$02                        ; 1F45
    call SpcB_22E5                    ; 1F47
    mov.b $24,a                       ; 1F4A
    mov.b $25,y                       ; 1F4C
    mov a,#$80                        ; 1F4E
    mov y,#$00                        ; 1F50
    addw ya,$24                       ; 1F52
    push y                            ; 1F54
    pop x                             ; 1F55
    mov y,a                           ; 1F56
    mov.b a,$22                       ; 1F57
    call SpcB_22D1                    ; 1F59
    mov a,y                           ; 1F5C
    bpl spcb_1F61                     ; 1F5D
    mov a,#$7F                        ; 1F5F
spcb_1F61:
    pop y                             ; 1F61
    pop x                             ; 1F62
    ret                               ; 1F63
spcb_1F64:
    mov.b a,$22                       ; 1F64
    bra spcb_1F61                     ; 1F66
    set1 $2C.6                        ; 1F68
    pop x                             ; 1F6A
    inc y                             ; 1F6B
    mov.w a,$04F0+x                   ; 1F6C
    cmp a,#$01                        ; 1F6F
    bne spcb_1F90                     ; 1F71
    mov a,($20)+y                     ; 1F73
    call SpcB_1F25                    ; 1F75
    mov.w $0398+x,a                   ; 1F78
    mov.w $04A8+x,a                   ; 1F7B
    bbs $2C.6,spcb_1F82               ; 1F7E
    inc y                             ; 1F81
spcb_1F82:
    mov a,($20)+y                     ; 1F82
    call SpcB_1F25                    ; 1F84
    mov.w $03A0+x,a                   ; 1F87
    mov.w $04B0+x,a                   ; 1F8A
    jmp spcb_1FC7                     ; 1F8D
spcb_1F90:
    mov.w a,$04B8+x                   ; 1F90
    cmp a,#$02                        ; 1F93
    beq spcb_1FB0                     ; 1F95
    mov a,($20)+y                     ; 1F97
    call SpcB_1F25                    ; 1F99
    mov.w $0398+x,a                   ; 1F9C
    mov.w $04A8+x,a                   ; 1F9F
    bbs $2C.6,spcb_1FA6               ; 1FA2
    inc y                             ; 1FA5
spcb_1FA6:
    mov a,#$00                        ; 1FA6
    mov.w $03A0+x,a                   ; 1FA8
    mov.w $04B0+x,a                   ; 1FAB
    bra spcb_1FC7                     ; 1FAE
spcb_1FB0:
    mov a,#$00                        ; 1FB0
    mov.w $0398+x,a                   ; 1FB2
    mov.w $04A8+x,a                   ; 1FB5
    bbs $2C.6,spcb_1FBC               ; 1FB8
    inc y                             ; 1FBB
spcb_1FBC:
    mov a,($20)+y                     ; 1FBC
    call SpcB_1F25                    ; 1FBE
    mov.w $03A0+x,a                   ; 1FC1
    mov.w $04B0+x,a                   ; 1FC4
spcb_1FC7:
    mov a,#$00                        ; 1FC7
    mov.w $03A8+x,a                   ; 1FC9
    mov.w $03B0+x,a                   ; 1FCC
    clr1 $2C.6                        ; 1FCF
    jmp spcb_1E04                     ; 1FD1
    pop x                             ; 1FD4
    inc y                             ; 1FD5
    mov a,($20)+y                     ; 1FD6
    and a,#$0F                        ; 1FD8
    mov.b $22,a                       ; 1FDA
    mov.w a,$03D0+x                   ; 1FDC
    and a,#$F0                        ; 1FDF
    bra spcb_1FF4                     ; 1FE1
    pop x                             ; 1FE3
    inc y                             ; 1FE4
    mov a,($20)+y                     ; 1FE5
    and a,#$07                        ; 1FE7
    asl a                             ; 1FE9
    asl a                             ; 1FEA
    asl a                             ; 1FEB
    asl a                             ; 1FEC
    mov.b $22,a                       ; 1FED
    mov.w a,$03D0+x                   ; 1FEF
    and a,#$8F                        ; 1FF2
spcb_1FF4:
    or.b a,$22                        ; 1FF4
    or a,#$80                         ; 1FF6
    mov.w $03D0+x,a                   ; 1FF8
    jmp spcb_1E04                     ; 1FFB
    pop x                             ; 1FFE
    inc y                             ; 1FFF
    mov a,($20)+y                     ; 2000
    and a,#$07                        ; 2002
    asl a                             ; 2004
    asl a                             ; 2005
    asl a                             ; 2006
    asl a                             ; 2007
    asl a                             ; 2008
    mov.b $22,a                       ; 2009
    mov.w a,$03D8+x                   ; 200B
    and a,#$1F                        ; 200E
    bra spcb_201F                     ; 2010
    pop x                             ; 2012
    inc y                             ; 2013
    mov a,($20)+y                     ; 2014
    and a,#$1F                        ; 2016
    mov.b $22,a                       ; 2018
    mov.w a,$03D8+x                   ; 201A
    and a,#$E0                        ; 201D
spcb_201F:
    or.b a,$22                        ; 201F
    mov.w $03D8+x,a                   ; 2021
    jmp spcb_1E04                     ; 2024
    pop x                             ; 2027
    inc y                             ; 2028
    mov a,($20)+y                     ; 2029
    mov.w $0428+x,a                   ; 202B
    inc y                             ; 202E
    mov a,($20)+y                     ; 202F
    mov.w $0430+x,a                   ; 2031
    mov a,#$01                        ; 2034
    mov.w $0438+x,a                   ; 2036
    jmp spcb_1E04                     ; 2039
    pop x                             ; 203C
    call SpcB_1CD6                    ; 203D
    mov.b a,$4A                       ; 2040
    or.b a,$22                        ; 2042
    mov.b $4A,a                       ; 2044
    bra spcb_2052                     ; 2046
    pop x                             ; 2048
    call SpcB_1CE5                    ; 2049
    mov.b a,$4A                       ; 204C
    and.b a,$22                       ; 204E
    mov.b $4A,a                       ; 2050
spcb_2052:
    jmp spcb_1E04                     ; 2052
    inc y                             ; 2055
    mov a,($20)+y                     ; 2056
    mov.b $24,a                       ; 2058
    inc y                             ; 205A
    mov a,($20)+y                     ; 205B
    mov.b $25,a                       ; 205D
    push y                            ; 205F
    mov x,#$BC                        ; 2060
spcb_2062:
    mov.w a,$0702+x                   ; 2062
    cmp.b a,$24                       ; 2065
    beq spcb_206D                     ; 2067
    inc x                             ; 2069
    inc x                             ; 206A
    bra spcb_2062                     ; 206B
spcb_206D:
    inc x                             ; 206D
    mov.w a,$0702+x                   ; 206E
    cmp.b a,$25                       ; 2071
    beq spcb_207C                     ; 2073
    inc x                             ; 2075
    cmp x,#$00                        ; 2076
    beq spcb_2082                     ; 2078
    bra spcb_2062                     ; 207A
spcb_207C:
    mov a,x                           ; 207C
    lsr a                             ; 207D
    inc a                             ; 207E
    call SpcB_2087                    ; 207F
spcb_2082:
    pop y                             ; 2082
    pop x                             ; 2083
    jmp spcb_1E04                     ; 2084

%spc($2087, $1B806C)
SpcB_2087:
    mov.b $23,a                       ; 2087
    push a                            ; 2089
    mov y,#$00                        ; 208A
    mov a,($24)+y                     ; 208C
    mov x,a                           ; 208E
    mov.w a,$04F0+x                   ; 208F
    cmp a,#$02                        ; 2092
    beq spcb_20B4                     ; 2094
    cmp a,#$03                        ; 2096
    beq spcb_20B4                     ; 2098
    call SpcB_1B17                    ; 209A
    call SpcB_1B48                    ; 209D
    call SpcB_1B71                    ; 20A0
    mov.b a,$24                       ; 20A3
    mov.w $0488+x,a                   ; 20A5
    mov.b a,$25                       ; 20A8
    mov.w $0490+x,a                   ; 20AA
    call SpcB_1AFD                    ; 20AD
    pop a                             ; 20B0
    jmp spcb_1C04                     ; 20B1
spcb_20B4:
    pop a                             ; 20B4
    ret                               ; 20B5
    pop x                             ; 20B6
    mov.w a,$0420+x                   ; 20B7
    bne spcb_20BF                     ; 20BA
    call SpcB_186C                    ; 20BC
spcb_20BF:
    ret                               ; 20BF
    pop x                             ; 20C0
    inc y                             ; 20C1
    mov a,($20)+y                     ; 20C2
    mov.w $0500+x,a                   ; 20C4
    inc y                             ; 20C7
    mov a,($20)+y                     ; 20C8
    mov.w $0508+x,a                   ; 20CA
    inc y                             ; 20CD
    mov a,($20)+y                     ; 20CE
    mov.w $0510+x,a                   ; 20D0
    inc y                             ; 20D3
    mov a,($20)+y                     ; 20D4
    mov.w $0518+x,a                   ; 20D6
    mov a,#$01                        ; 20D9
    mov.w $0520+x,a                   ; 20DB
    jmp spcb_1E04                     ; 20DE
    pop x                             ; 20E1
    inc y                             ; 20E2
    mov a,($20)+y                     ; 20E3
    mov.w $0440+x,a                   ; 20E5
    jmp spcb_1E04                     ; 20E8
    pop x                             ; 20EB
    mov a,#$00                        ; 20EC
    mov.w $0438+x,a                   ; 20EE
    jmp spcb_1E04                     ; 20F1
    pop x                             ; 20F4
    mov a,#$00                        ; 20F5
    mov.w $0520+x,a                   ; 20F7
    jmp spcb_1E04                     ; 20FA
    pop x                             ; 20FD
    inc y                             ; 20FE
    mov a,($20)+y                     ; 20FF
    push a                            ; 2101
    inc y                             ; 2102
    mov a,($20)+y                     ; 2103
    mov.b $21,a                       ; 2105
    pop a                             ; 2107
    mov.b $20,a                       ; 2108
    mov y,#$00                        ; 210A
    jmp spcb_1E05                     ; 210C
    pop x                             ; 210F
    inc y                             ; 2110
    mov a,($20)+y                     ; 2111
    mov.w $0448+x,a                   ; 2113
    jmp spcb_1E04                     ; 2116
    pop x                             ; 2119
    call SpcB_1D50                    ; 211A
    mov a,#$02                        ; 211D
    mov.w $0478+x,a                   ; 211F
    jmp spcb_1E04                     ; 2122
    pop x                             ; 2125
    inc y                             ; 2126
    mov a,($20)+y                     ; 2127
    mov.w $04F0+x,a                   ; 2129
    jmp spcb_1E04                     ; 212C
    db $CE, $FC, $F7, $20
    and a,#$FE                        ; 2133
    push y                            ; 2135
    push x                            ; 2136
    call SpcB_2588                    ; 2137
    pop x                             ; 213A
    pop y                             ; 213B
    jmp spcb_1E04                     ; 213C
spcb_213F:
    mov.w a,$0438+x                   ; 213F
    beq spcb_216F                     ; 2142
    mov.w a,$0428+x                   ; 2144
    mov.b $20,a                       ; 2147
    mov.w a,$0430+x                   ; 2149
    mov.b $21,a                       ; 214C
    mov.w a,$04A0+x                   ; 214E
    mov y,a                           ; 2151
    mov.w a,$0498+x                   ; 2152
    addw ya,$20                       ; 2155
    mov.w $0498+x,a                   ; 2157
    mov a,y                           ; 215A
    mov.w $04A0+x,a                   ; 215B
    bmi spcb_2170                     ; 215E
    setc                              ; 2160
    sbc a,#$40                        ; 2161
    bpl spcb_2175                     ; 2163
    mov.w a,$0498+x                   ; 2165
spcb_2168:
    mov.w $03B8+x,a                   ; 2168
    mov a,y                           ; 216B
    mov.w $03C0+x,a                   ; 216C
spcb_216F:
    ret                               ; 216F
spcb_2170:
    mov a,#$00                        ; 2170
    mov y,a                           ; 2172
    bra spcb_2168                     ; 2173
spcb_2175:
    mov a,#$FF                        ; 2175
    mov y,#$3F                        ; 2177
    bra spcb_2168                     ; 2179
    mov.w a,$0520+x                   ; 217B
    beq spcb_21A4                     ; 217E
    mov.w a,$0500+x                   ; 2180
    mov.b $20,a                       ; 2183
    mov.w a,$0508+x                   ; 2185
    mov.b $21,a                       ; 2188
    call SpcB_21A5                    ; 218A
    mov.w a,$0510+x                   ; 218D
    mov.b $20,a                       ; 2190
    mov.w a,$0518+x                   ; 2192
    mov.b $21,a                       ; 2195
    mov a,x                           ; 2197
    clrc                              ; 2198
    adc a,#$08                        ; 2199
    mov x,a                           ; 219B
    call SpcB_21A5                    ; 219C
    mov a,x                           ; 219F
    setc                              ; 21A0
    sbc a,#$08                        ; 21A1
    mov x,a                           ; 21A3
spcb_21A4:
    ret                               ; 21A4

%spc($21A5, $1B818A)
SpcB_21A5:
    mov.w a,$04A8+x                   ; 21A5
    mov y,a                           ; 21A8
    mov.w a,$03A8+x                   ; 21A9
    addw ya,$20                       ; 21AC
    mov.w $03A8+x,a                   ; 21AE
    mov a,y                           ; 21B1
    mov.w $04A8+x,a                   ; 21B2
    bmi spcb_21B9                     ; 21B5
    bra spcb_21C4                     ; 21B7
spcb_21B9:
    setc                              ; 21B9
    sbc a,#$C0                        ; 21BA
    bmi spcb_21C2                     ; 21BC
    mov a,#$00                        ; 21BE
    bra spcb_21C4                     ; 21C0
spcb_21C2:
    mov a,#$7F                        ; 21C2
spcb_21C4:
    mov.w $0398+x,a                   ; 21C4
    ret                               ; 21C7
spcb_21C8:
    mov a,x                           ; 21C8
    mov y,#$10                        ; 21C9
    mul ya                            ; 21CB
    mov y,a                           ; 21CC
    push y                            ; 21CD
    mov.w a,$0408+x                   ; 21CE
    beq spcb_21DD                     ; 21D1
    mov.w a,$04C0+x                   ; 21D3
    beq spcb_21DD                     ; 21D6
    call SpcB_2243                    ; 21D8
    bra spcb_21E0                     ; 21DB
spcb_21DD:
    mov.w a,$0398+x                   ; 21DD
spcb_21E0:
    call SpcB_22F2                    ; 21E0
    pop y                             ; 21E3
    call $09E8                        ; 21E4
    inc y                             ; 21E7
    push y                            ; 21E8
    mov.w a,$0408+x                   ; 21E9
    beq spcb_21FE                     ; 21EC
    mov.w a,$04C8+x                   ; 21EE
    beq spcb_21FE                     ; 21F1
    call SpcB_22BF                    ; 21F3
    call SpcB_2243                    ; 21F6
    call SpcB_22C8                    ; 21F9
    bra spcb_2201                     ; 21FC
spcb_21FE:
    mov.w a,$03A0+x                   ; 21FE
spcb_2201:
    call SpcB_22F2                    ; 2201
    pop y                             ; 2204
    call $09E8                        ; 2205
    inc y                             ; 2208
    mov.w a,$03B8+x                   ; 2209
    call $09E8                        ; 220C
    inc y                             ; 220F
    mov.w a,$03C0+x                   ; 2210
    call $09E8                        ; 2213
    inc y                             ; 2216
    mov.w a,$03C8+x                   ; 2217
    call $09E8                        ; 221A
    inc y                             ; 221D
    mov.w a,$03D0+x                   ; 221E
    call $09E8                        ; 2221
    inc y                             ; 2224
    mov.w a,$03D8+x                   ; 2225
    jmp $09E8                         ; 2228
    db $7C, $21, $7A, $23, $96, $25, $D2, $27
    db $30, $2A, $B2, $2C, $5A, $2F, $2C, $32
    db $28, $35, $50, $38, $AA, $3B, $36, $3F

%spc($2243, $1B8228)
SpcB_2243:
    cmp a,#$01                        ; 2243
    beq spcb_2255                     ; 2245
    cmp a,#$3C                        ; 2247
    beq spcb_2292                     ; 2249
    cmp a,#$FF                        ; 224B
    beq spcb_22BA                     ; 224D
    inc a                             ; 224F
    mov.w $04C0+x,a                   ; 2250
    bra spcb_226E                     ; 2253
spcb_2255:
    inc a                             ; 2255
    mov.w $04C0+x,a                   ; 2256
    mov.w a,$0398+x                   ; 2259
    mov y,a                           ; 225C
    mov.w a,$03A8+x                   ; 225D
    push x                            ; 2260
    mov x,#$3C                        ; 2261
    call SpcB_22E5                    ; 2263
    pop x                             ; 2266
    mov.w $04D0+x,a                   ; 2267
    mov a,y                           ; 226A
    mov.w $04E0+x,a                   ; 226B
spcb_226E:
    mov.w a,$04C0+x                   ; 226E
    dec a                             ; 2271
    mov.b $22,a                       ; 2272
    mov.w a,$04D0+x                   ; 2274
    mov y,a                           ; 2277
    mov.w a,$04E0+x                   ; 2278
    push x                            ; 227B
    mov x,a                           ; 227C
    mov.b a,$22                       ; 227D
    call SpcB_22D1                    ; 227F
    pop x                             ; 2282
    mov.b $22,a                       ; 2283
    mov.b $23,y                       ; 2285
    mov.w a,$0398+x                   ; 2287
    mov y,a                           ; 228A
    mov.w a,$03A8+x                   ; 228B
    subw ya,$22                       ; 228E
    mov a,y                           ; 2290
    ret                               ; 2291
spcb_2292:
    bbs $2C.7,spcb_229A               ; 2292
    bbs $2D.0,spcb_229D               ; 2295
    bra spcb_22A0                     ; 2298
spcb_229A:
    jmp spcb_19F5                     ; 229A
spcb_229D:
    jmp spcb_1A21                     ; 229D
spcb_22A0:
    mov.b $22,x                       ; 22A0
    bbs $22.3,spcb_22AF               ; 22A2
    call SpcB_186C                    ; 22A5
spcb_22A8:
    mov a,#$FF                        ; 22A8
    mov.w $04C0+x,a                   ; 22AA
    inc a                             ; 22AD
    ret                               ; 22AE
spcb_22AF:
    call SpcB_22C8                    ; 22AF
    call SpcB_186C                    ; 22B2
    call SpcB_22BF                    ; 22B5
    bra spcb_22A8                     ; 22B8
spcb_22BA:
    inc a                             ; 22BA
    mov.w $04C0+x,a                   ; 22BB
    ret                               ; 22BE

%spc($22BF, $1B82A4)
SpcB_22BF:
    inc x                             ; 22BF
    inc x                             ; 22C0
    inc x                             ; 22C1
    inc x                             ; 22C2
    inc x                             ; 22C3
    inc x                             ; 22C4
    inc x                             ; 22C5
    inc x                             ; 22C6
    ret                               ; 22C7

%spc($22C8, $1B82AD)
SpcB_22C8:
    dec x                             ; 22C8
    dec x                             ; 22C9
    dec x                             ; 22CA
    dec x                             ; 22CB
    dec x                             ; 22CC
    dec x                             ; 22CD
    dec x                             ; 22CE
    dec x                             ; 22CF
    ret                               ; 22D0

%spc($22D1, $1B82B6)
SpcB_22D1:
    push a                            ; 22D1
    mul ya                            ; 22D2
    mov.b $26,a                       ; 22D3
    mov.b $27,y                       ; 22D5
    mov a,x                           ; 22D7
    mov y,a                           ; 22D8
    pop a                             ; 22D9
    mul ya                            ; 22DA
    push a                            ; 22DB
    mov a,y                           ; 22DC
    mov x,a                           ; 22DD
    pop a                             ; 22DE
    mov y,a                           ; 22DF
    mov a,#$00                        ; 22E0
    addw ya,$26                       ; 22E2
    ret                               ; 22E4

%spc($22E5, $1B82CA)
SpcB_22E5:
    push a                            ; 22E5
    push y                            ; 22E6
    mov y,#$00                        ; 22E7
    pop a                             ; 22E9
    div ya,x                          ; 22EA
    mov.b $27,a                       ; 22EB
    pop a                             ; 22ED
    div ya,x                          ; 22EE
    mov.b y,$27                       ; 22EF
    ret                               ; 22F1

%spc($22F2, $1B82D7)
SpcB_22F2:
    push y                            ; 22F2
    mov.b $22,a                       ; 22F3
    cmp a,#$00                        ; 22F5
    beq spcb_2318                     ; 22F7
    mov.w a,$0390+x                   ; 22F9
    cmp a,#$25                        ; 22FC
    bne spcb_231A                     ; 22FE
    cmp x,#$05                        ; 2300
    beq spcb_230A                     ; 2302
    cmp x,#$04                        ; 2304
    beq spcb_230F                     ; 2306
    bra spcb_2318                     ; 2308
spcb_230A:
    mov.w a,$0530                     ; 230A
    bra spcb_2312                     ; 230D
spcb_230F:
    mov.w a,$0531                     ; 230F
spcb_2312:
    mov y,#$04                        ; 2312
    mul ya                            ; 2314
    clrc                              ; 2315
    adc.b a,$22                       ; 2316
spcb_2318:
    pop y                             ; 2318
    ret                               ; 2319
spcb_231A:
    mov.b a,$22                       ; 231A
    bra spcb_2318                     ; 231C

%spc($231E, $1B8303)
SpcB_231E:
    mov.w a,$0390+x                   ; 231E
    mov.b $20,a                       ; 2321
    push x                            ; 2323
    mov x,#$00                        ; 2324
spcb_2326:
    mov.w a,$233A+x                   ; 2326
    beq spcb_2332                     ; 2329
    cmp.b a,$20                       ; 232B
    beq spcb_2334                     ; 232D
    inc x                             ; 232F
    bra spcb_2326                     ; 2330
spcb_2332:
    pop x                             ; 2332
    ret                               ; 2333
spcb_2334:
    mov a,x                           ; 2334
    asl a                             ; 2335
    mov x,a                           ; 2336
    jmp ($2341+x)                     ; 2337
    db $79, $78, $77, $7A, $17, $25, $00, $5D
    db $23, $5D, $23, $5D, $23, $5D, $23, $45
    db $25, $3A, $25, $0C, $13, $55, $7F, $23
    db $1A, $17, $17, $0C, $0B, $0E, $07, $1B
    db $1E, $1F, $12
    lsr a                             ; 235D
    mov x,a                           ; 235E
    mov.w a,$234D+x                   ; 235F
    mov.b $22,a                       ; 2362
    mov.w a,$2351+x                   ; 2364
    mov.b $23,a                       ; 2367
    mov.w a,$2355+x                   ; 2369
    mov.b $24,a                       ; 236C
    mov.w a,$2359+x                   ; 236E
    mov.b $25,a                       ; 2371
    pop x                             ; 2373
    cmp x,#$07                        ; 2374
    beq spcb_237D                     ; 2376
    cmp x,#$06                        ; 2378
    beq spcb_2381                     ; 237A
    ret                               ; 237C
spcb_237D:
    mov y,#$01                        ; 237D
    bra spcb_2383                     ; 237F
spcb_2381:
    mov y,#$00                        ; 2381
spcb_2383:
    call SpcB_24E2                    ; 2383
    push x                            ; 2386
    mov a,y                           ; 2387
    mov x,a                           ; 2388
    mov.w a,$0002+x                   ; 2389
    cmp.w a,$000A+x                   ; 238C
    beq spcb_23DF                     ; 238F
    mov a,#$00                        ; 2391
    mov.w $0538+x,a                   ; 2393
    mov.w a,$0002+x                   ; 2396
    mov.w $053A+x,a                   ; 2399
    setc                              ; 239C
    sbc.w a,$000A+x                   ; 239D
    bmi spcb_23D4                     ; 23A0
    push a                            ; 23A2
    mov a,#$01                        ; 23A3
    mov.w $053C+x,a                   ; 23A5
spcb_23A8:
    pop a                             ; 23A8
    mov y,a                           ; 23A9
    mov.w a,$0532+x                   ; 23AA
    push x                            ; 23AD
    mov x,a                           ; 23AE
    bne spcb_23B3                     ; 23AF
    mov x,#$01                        ; 23B1
spcb_23B3:
    mov a,#$00                        ; 23B3
    div ya,x                          ; 23B5
    bvs spcb_23C3                     ; 23B6
    pop x                             ; 23B8
    mov.w $0534+x,a                   ; 23B9
    mov a,#$00                        ; 23BC
    mov.w $0536+x,a                   ; 23BE
    bra spcb_23CC                     ; 23C1
spcb_23C3:
    pop x                             ; 23C3
    mov.w $0534+x,a                   ; 23C4
    mov a,#$01                        ; 23C7
    mov.w $0536+x,a                   ; 23C9
spcb_23CC:
    mov a,#$00                        ; 23CC
    mov.w $0532+x,a                   ; 23CE
    jmp spcb_248B                     ; 23D1
spcb_23D4:
    eor a,#$FF                        ; 23D4
    inc a                             ; 23D6
    push a                            ; 23D7
    mov a,#$02                        ; 23D8
    mov.w $053C+x,a                   ; 23DA
    bra spcb_23A8                     ; 23DD
spcb_23DF:
    mov.w a,$0002+x                   ; 23DF
    bne spcb_23EE                     ; 23E2
    mov a,#$00                        ; 23E4
    mov.w $0534+x,a                   ; 23E6
    mov.w $0536+x,a                   ; 23E9
    bra spcb_2414                     ; 23EC
spcb_23EE:
    mov.w a,$0002+x                   ; 23EE
    bpl spcb_2414                     ; 23F1
    setc                              ; 23F3
    sbc a,#$F0                        ; 23F4
    bmi spcb_2414                     ; 23F6
    mov.w a,$056C+x                   ; 23F8
    mov.b $22,a                       ; 23FB
    mov.w a,$056E+x                   ; 23FD
    mov.b $23,a                       ; 2400
    incw $22                          ; 2402
    incw $22                          ; 2404
    incw $22                          ; 2406
    mov.b a,$22                       ; 2408
    mov.w $056C+x,a                   ; 240A
    mov.b a,$23                       ; 240D
    mov.w $056E+x,a                   ; 240F
    bra spcb_241C                     ; 2412
spcb_2414:
    mov a,#$00                        ; 2414
    mov.w $056C+x,a                   ; 2416
    mov.w $056E+x,a                   ; 2419
spcb_241C:
    mov.w a,$0532+x                   ; 241C
    setc                              ; 241F
    sbc a,#$0A                        ; 2420
    bmi spcb_2440                     ; 2422
    mov $22,#$01                      ; 2424
    mov $23,#$00                      ; 2427
    mov.w a,$0536+x                   ; 242A
    mov y,a                           ; 242D
    mov.w a,$0534+x                   ; 242E
    subw ya,$22                       ; 2431
    bpl spcb_2439                     ; 2433
    mov a,#$00                        ; 2435
    mov y,#$00                        ; 2437
spcb_2439:
    mov.w $0534+x,a                   ; 2439
    mov a,y                           ; 243C
    mov.w $0536+x,a                   ; 243D
spcb_2440:
    mov.w a,$0532+x                   ; 2440
    cmp a,#$FF                        ; 2443
    beq spcb_244E                     ; 2445
    mov.w a,$0532+x                   ; 2447
    inc a                             ; 244A
    mov.w $0532+x,a                   ; 244B
spcb_244E:
    mov.w a,$0532+x                   ; 244E
    cmp a,#$01                        ; 2451
    beq spcb_2483                     ; 2453
    mov.w a,$053A+x                   ; 2455
    mov y,a                           ; 2458
    mov.w a,$0538+x                   ; 2459
spcb_245C:
    push a                            ; 245C
    mov.w a,$0534+x                   ; 245D
    mov.b $20,a                       ; 2460
    mov.w a,$0536+x                   ; 2462
    mov.b $21,a                       ; 2465
    mov.w a,$053C+x                   ; 2467
    cmp a,#$02                        ; 246A
    beq spcb_247C                     ; 246C
    pop a                             ; 246E
    addw ya,$20                       ; 246F
    bcs spcb_248B                     ; 2471
spcb_2473:
    mov.w $0538+x,a                   ; 2473
    mov a,y                           ; 2476
    mov.w $053A+x,a                   ; 2477
    bra spcb_248B                     ; 247A
spcb_247C:
    pop a                             ; 247C
    subw ya,$20                       ; 247D
    bcc spcb_248B                     ; 247F
    bra spcb_2473                     ; 2481
spcb_2483:
    mov.w a,$0002+x                   ; 2483
    mov y,a                           ; 2486
    mov a,#$00                        ; 2487
    bra spcb_245C                     ; 2489
spcb_248B:
    mov $20,#$00                      ; 248B
    mov $21,$24                       ; 248E
    mov.b y,$25                       ; 2491
    mov.w a,$053A+x                   ; 2493
    mul ya                            ; 2496
    mov $20,#$00                      ; 2497
    addw ya,$20                       ; 249A
    mov.b $20,a                       ; 249C
    mov.b $21,y                       ; 249E
    mov.w a,$054A+x                   ; 24A0
    mov y,a                           ; 24A3
    mov.w a,$0548+x                   ; 24A4
    addw ya,$20                       ; 24A7
    mov.b $20,a                       ; 24A9
    mov.b $21,y                       ; 24AB
    mov.w a,$054E+x                   ; 24AD
    mov y,a                           ; 24B0
    mov.w a,$054C+x                   ; 24B1
    addw ya,$20                       ; 24B4
    mov.b $20,a                       ; 24B6
    mov.b $21,y                       ; 24B8
    mov.w a,$056E+x                   ; 24BA
    mov y,a                           ; 24BD
    mov.w a,$056C+x                   ; 24BE
    addw ya,$20                       ; 24C1
    mov.b $20,a                       ; 24C3
    mov.b $21,y                       ; 24C5
    setc                              ; 24C7
    sbc $21,#$3F                      ; 24C8
    bmi spcb_24D0                     ; 24CB
    mov $21,#$3F                      ; 24CD
spcb_24D0:
    mov.w a,$0002+x                   ; 24D0
    mov.w $000A+x,a                   ; 24D3
    pop x                             ; 24D6
    mov.b a,$20                       ; 24D7
    mov.w $03B8+x,a                   ; 24D9
    mov.b a,$21                       ; 24DC
    mov.w $03C0+x,a                   ; 24DE
    ret                               ; 24E1

%spc($24E2, $1B84C7)
SpcB_24E2:
    mov.w a,$0002+y                   ; 24E2
    eor a,#$FF                        ; 24E5
    push y                            ; 24E7
    mov y,#$00                        ; 24E8
    push x                            ; 24EA
    mov.b x,$22                       ; 24EB
    div ya,x                          ; 24ED
    mov.b $20,a                       ; 24EE
    pop x                             ; 24F0
    pop y                             ; 24F1
    mov.b a,$23                       ; 24F2
    clrc                              ; 24F4
    adc.b a,$20                       ; 24F5
    cmp y,#$00                        ; 24F7
    beq spcb_2513                     ; 24F9
    bbc $29.2,spcb_2500               ; 24FB
    adc a,#$03                        ; 24FE
spcb_2500:
    call SpcB_2530                    ; 2500
    push y                            ; 2503
    mov.w y,$056B                     ; 2504
    mul ya                            ; 2507
    mov a,y                           ; 2508
    pop y                             ; 2509
    mov.w $0398+x,a                   ; 250A
    mov a,#$00                        ; 250D
    mov.w $03A0+x,a                   ; 250F
    ret                               ; 2512
spcb_2513:
    bbc $29.4,spcb_2518               ; 2513
    adc a,#$03                        ; 2516
spcb_2518:
    call SpcB_252B                    ; 2518
    push y                            ; 251B
    mov.w y,$056B                     ; 251C
    mul ya                            ; 251F
    mov a,y                           ; 2520
    pop y                             ; 2521
    mov.w $03A0+x,a                   ; 2522
    mov a,#$00                        ; 2525
    mov.w $0398+x,a                   ; 2527
    ret                               ; 252A

%spc($252B, $1B8510)
SpcB_252B:
    bbs $28.4,spcb_2539               ; 252B
    bra spcb_2533                     ; 252E

%spc($2530, $1B8515)
SpcB_2530:
    bbs $28.2,spcb_2539               ; 2530
spcb_2533:
    push y                            ; 2533
    mov y,#$D0                        ; 2534
    mul ya                            ; 2536
    mov a,y                           ; 2537
    pop y                             ; 2538
spcb_2539:
    ret                               ; 2539
    mov $22,#$1E                      ; 253A
    mov $23,#$03                      ; 253D
    mov $24,#$01                      ; 2540
    bra spcb_254E                     ; 2543
    mov $22,#$14                      ; 2545
    mov $23,#$0A                      ; 2548
    mov $24,#$02                      ; 254B
spcb_254E:
    pop x                             ; 254E
    cmp x,#$05                        ; 254F
    beq spcb_2558                     ; 2551
    cmp x,#$04                        ; 2553
    beq spcb_255C                     ; 2555
    ret                               ; 2557
spcb_2558:
    mov y,#$01                        ; 2558
    bra spcb_255E                     ; 255A
spcb_255C:
    mov y,#$00                        ; 255C
spcb_255E:
    mov.w a,$0002+y                   ; 255E
    eor a,#$FF                        ; 2561
    push y                            ; 2563
    mov y,#$00                        ; 2564
    push x                            ; 2566
    mov.b x,$22                       ; 2567
    div ya,x                          ; 2569
    pop x                             ; 256A
    pop y                             ; 256B
    clrc                              ; 256C
    adc.b a,$23                       ; 256D
    push a                            ; 256F
    mov.w a,$0544+y                   ; 2570
    inc a                             ; 2573
    mov.w $0544+y,a                   ; 2574
    pop a                             ; 2577
    setc                              ; 2578
    sbc.w a,$0544+y                   ; 2579
    bpl spcb_2587                     ; 257C
    mov.b a,$24                       ; 257E
    mov.w $03E8+x,a                   ; 2580
    dec a                             ; 2583
    mov.w $0544+y,a                   ; 2584
spcb_2587:
    ret                               ; 2587

%spc($2588, $1B856D)
SpcB_2588:
    mov y,#$00                        ; 2588
    mov x,#$1B                        ; 258A
spcb_258C:
    mov.w $FF00+y,a                   ; 258C
    inc y                             ; 258F
    inc y                             ; 2590
    inc y                             ; 2591
    inc y                             ; 2592
    inc y                             ; 2593
    inc y                             ; 2594
    inc y                             ; 2595
    inc y                             ; 2596
    inc y                             ; 2597
    dec x                             ; 2598
    bne spcb_258C                     ; 2599
    inc a                             ; 259B
    mov.w $FF00+y,a                   ; 259C
    mov y,#$FF                        ; 259F
    mov a,#$00                        ; 25A1
    mov.w $3F8C,a                     ; 25A3
    mov.w $3F8D,y                     ; 25A6
    mov.w $3F8E,a                     ; 25A9
    mov.w $3F8F,y                     ; 25AC
    mov.b a,$18                       ; 25AF
    or.b a,$19                        ; 25B1
    bne spcb_25B7                     ; 25B3
    inc.b $18                         ; 25B5
spcb_25B7:
    ret                               ; 25B7

%spc($25B8, $1B859D)
SpcB_25B8:
    mov x,#$00                        ; 25B8
spcb_25BA:
    mov.w a,$03C8+x                   ; 25BA
    cmp a,#$23                        ; 25BD
    beq spcb_25C8                     ; 25BF
    inc x                             ; 25C1
    cmp x,#$07                        ; 25C2
    beq spcb_25B7                     ; 25C4
    bra spcb_25BA                     ; 25C6
spcb_25C8:
    dec.w $0563                       ; 25C8
    mov.w a,$0563                     ; 25CB
    and a,#$03                        ; 25CE
    mov y,#$3F                        ; 25D0
    mul ya                            ; 25D2
    mov y,a                           ; 25D3
    mov $20,#$07                      ; 25D4
spcb_25D7:
    inc y                             ; 25D7
    mov x,#$04                        ; 25D8
spcb_25DA:
    mov1 c,$0019.6                    ; 25DA
    eor1 c,$0019.5                    ; 25DD
    rol.b $18                         ; 25E0
    rol.b $19                         ; 25E2
    mov.b a,$19                       ; 25E4
    and a,#$D0                        ; 25E6
    or a,#$11                         ; 25E8
    mov.w $FF00+y,a                   ; 25EA
    inc y                             ; 25ED
    mov.b a,$18                       ; 25EE
    and a,#$FF                        ; 25F0
    or a,#$11                         ; 25F2
    mov.w $FF00+y,a                   ; 25F4
    inc y                             ; 25F7
    dec x                             ; 25F8
    bne spcb_25DA                     ; 25F9
    dbnz $20,spcb_25D7                ; 25FB
    ret                               ; 25FE
    db $03, $10, $32, $30, $3C, $16, $20, $56
    db $00, $30, $3C, $38, $16, $20, $56, $00
%spc_end()
