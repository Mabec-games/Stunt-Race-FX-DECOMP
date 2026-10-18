; spc_battle_driver.asm: the battle sound driver, the N-SPC engine as
; uploaded at 03:D96F (block list $1A:F091) over the race driver's
; $10B0-$260E: its own sound tick, port commands and sound effect
; channels on the race driver's music core below it.

incsrc "../include/macros.inc"
incsrc "../include/spc.inc"
%spc($10B0, $1AF095)

; @routine 1A:F095 SpcBSfxFrame
; WHAT IT DOES: The battle driver's sound tick, the race one's (SpcSfxFrame) with the battle's game rules: a new port 0 byte becomes the command $00 (else $00 = 0); ports 1-3 copied to $01-$03 and $05-$07; a stop request (bit 1 of $29) clears bits 0 and 1; then the command remap, music fade, port command, channel guards, command block, command channel check, tempo bend, staggered checks, game state rules, pitch ramps, command filter, command $4C, timers, noise sample, port 1 clamp and port 1 command, and the sound effect channels (through the queue start at $1A7F). The race driver's also has the echo transition and the held sounds.
; HOW: A chain of CALLs, then a JMP to the queue start.
; Entry: none (called from SpcReset's loop, in place of the race driver's routine at the same address). Exit: as the sound effect channels leave it.
; @spc 10B0
%spc($10B0, $1AF095)
SpcBSfxFrame:
    mov.w a,!spc_port_latch           ; 10B0
    cmp.b a,!spc_port_out             ; 10B3
    beq .l10C0                        ; 10B5
    mov.w a,!spc_port_latch           ; 10B7
    mov.b !spc_port_out,a             ; 10BA
    mov.b !spc_port_in,a              ; 10BC
    bra .l10C3                        ; 10BE
.l10C0:
    mov !spc_port_in,#$00             ; 10C0
.l10C3:
    mov.w a,!spc_port_forced          ; 10C3
    mov.b !spc_port_out+1,a           ; 10C6
    mov.b !spc_port_in+1,a            ; 10C8
    mov.w a,!spc_port_forced+1        ; 10CA
    mov.b !spc_port_out+2,a           ; 10CD
    mov.b !spc_port_in+2,a            ; 10CF
    mov.w a,!spc_port_forced+2        ; 10D1
    mov.b !spc_port_out+3,a           ; 10D4
    mov.b !spc_port_in+3,a            ; 10D6
    bbc !spc_flags.1,.l10DF           ; 10D8
    clr1 !spc_flags.0                 ; 10DB
    clr1 !spc_flags.1                 ; 10DD
.l10DF:
    call SpcBPortRemap                ; 10DF
    call SpcBMusicFade                ; 10E2
    call SpcBPortCommand              ; 10E5
    call SpcBChannelGuards            ; 10E8
    call SpcBCommandBlock             ; 10EB
    call SpcBCommandChannelCheck      ; 10EE
    call SpcBTempoBend                ; 10F1
    call SpcBStaggeredChecks          ; 10F4
    call SpcBGameStateTick            ; 10F7
    call SpcBPitchRampB               ; 10FA
    call SpcBPitchRampA               ; 10FD
    call SpcBCommandFilter            ; 1100
    call SpcBCommand4C                ; 1103
    call SpcBCommandTimers            ; 1106
    call SpcBNoiseSample              ; 1109
    call SpcBPort1Clamp               ; 110C
    call SpcBPort1Command             ; 110F
    jmp SpcBCommandChannelCheck_l1A7F ; 1112

; @routine 1A:F0FA SpcBPortRemap
; WHAT IT DOES: Port 0 command $88 becomes $15.
; HOW: CMP and MOV on $00.
; Entry: none. Exit: flags from the compare.
; @spc 1115
%spc($1115, $1AF0FA)
SpcBPortRemap:
    cmp !spc_port_in,#$88             ; 1115
    bne .l111D                        ; 1118
    mov !spc_port_in,#$15             ; 111A  ; not reached in the traces so far
.l111D:
    ret                               ; 111D

; @routine 1A:F103 SpcBChannelGuards
; WHAT IT DOES: With bit 6 of $2D, stops channel 5 unless it plays sound $43 (SpcBSfxStop). The code after it (from $112D) is SpcBStaggeredChecks' step 6: with bit 5 of $2D, channel 7 stops when its sound $0397 is $77, $78 or $79, and channel 6 when its sound $0396 is $77-$7A. The race driver checks other sounds (SpcChannelGuards).
; HOW: Compare chains into SpcBSfxStop; the first check returns through SpcBPortRemap's RET.
; Entry: none. Exit: as SpcBSfxStop or a RET.
; @spc 111E
%spc($111E, $1AF103)
SpcBChannelGuards:
    bbc !spc_game_flags.6,SpcBPortRemap_l111D; 111E
    mov.w a,!spc_sfx_command+5        ; 1121
    cmp a,#$43                        ; 1124
    beq SpcBPortRemap_l111D           ; 1126
    mov x,#$05                        ; 1128
    jmp SpcBSfxStop                   ; 112A
.l112D:
    bbc !spc_game_flags.5,.l1156      ; 112D
    mov.w a,!spc_sfx_command+7        ; 1130
    cmp a,#$77                        ; 1133
    beq .l1157                        ; 1135
    cmp a,#$78                        ; 1137
    beq .l1157                        ; 1139
    cmp a,#$79                        ; 113B
    beq .l1157                        ; 113D
    cmp a,#$79                        ; 113F
    beq .l1157                        ; 1141
.l1143:
    mov.w a,!spc_sfx_command+6        ; 1143
    cmp a,#$77                        ; 1146
    beq .l115E                        ; 1148
    cmp a,#$78                        ; 114A
    beq .l115E                        ; 114C
    cmp a,#$79                        ; 114E
    beq .l115E                        ; 1150
    cmp a,#$7A                        ; 1152
    beq .l115E                        ; 1154
.l1156:
    ret                               ; 1156
.l1157:
    mov x,#$07                        ; 1157  ; not reached in the traces so far
    call SpcBSfxStop                  ; 1159  ; not reached in the traces so far
    bra .l1143                        ; 115C  ; not reached in the traces so far
.l115E:
    mov x,#$06                        ; 115E  ; not reached in the traces so far
    jmp SpcBSfxStop                   ; 1160  ; not reached in the traces so far

; @routine 1A:F148 SpcBStaggeredChecks
; WHAT IT DOES: Steps the counter $0581 through 0-$18 each tick and at 6, 9, $0C, $0F, $12, $15 and $18 does one check: step 6 the channel 6 and 7 guards (SpcBChannelGuards' second part); the others restart a looping sound by the bits of $2A (a stop through SpcBPort1Command's stop paths with $20 = $0C, $1C, $0A, $1A, $0B or $1B) or queue sound $17, $97 or $25; outside song 1, $2A is cleared. The race driver's (SpcStaggeredChecks) has other steps and sounds.
; HOW: A compare chain on the counter; the wrap test is SBC #$19 with the carry the last compare left.
; Entry: none. Exit: as the chosen path leaves it.
; @spc 1163
%spc($1163, $1AF148)
SpcBStaggeredChecks:
    inc.w !spc_stagger_step           ; 1163
    mov.w a,!spc_stagger_step         ; 1166
    cmp a,#$06                        ; 1169
    beq SpcBChannelGuards_l112D       ; 116B
    cmp a,#$09                        ; 116D
    beq .l1197                        ; 116F
    cmp a,#$0C                        ; 1171
    beq .l11A5                        ; 1173
    cmp a,#$0F                        ; 1175
    beq .l11B3                        ; 1177
    cmp a,#$12                        ; 1179
    beq .l11C7                        ; 117B
    cmp a,#$15                        ; 117D
    beq .l11DB                        ; 117F
    cmp a,#$18                        ; 1181
    beq .l11E4                        ; 1183
    sbc a,#$19                        ; 1185
    bmi .l118E                        ; 1187
    mov a,#$00                        ; 1189
    mov.w !spc_stagger_step,a         ; 118B
.l118E:
    cmp !spc_song,#$01                ; 118E
    beq .l1196                        ; 1191
    mov !spc_loop_flags,#$00          ; 1193
.l1196:
    ret                               ; 1196
.l1197:
    bbs !spc_loop_flags.0,.l11A0      ; 1197
    mov !spc_sfx_tmp,#$0C             ; 119A
    jmp SpcBPort1Command_l1833        ; 119D
.l11A0:
    mov a,#$17                        ; 11A0
    jmp SpcBSoundQueuePush            ; 11A2
.l11A5:
    bbs !spc_loop_flags.1,.l11AE      ; 11A5
    mov !spc_sfx_tmp,#$1C             ; 11A8
    jmp SpcBPort1Command_l1856        ; 11AB
.l11AE:
    mov a,#$97                        ; 11AE
    jmp SpcBSoundQueuePush            ; 11B0
.l11B3:
    bbs !spc_loop_flags.2,.l11BC      ; 11B3
    mov !spc_sfx_tmp,#$0A             ; 11B6
    jmp SpcBPort1Command_l181D        ; 11B9
.l11BC:
    bbs !spc_loop_flags.0,.l118E      ; 11BC
    bbs !spc_loop_flags.4,.l118E      ; 11BF
    mov a,#$25                        ; 11C2
    jmp SpcBSoundQueuePush            ; 11C4
.l11C7:
    bbs !spc_loop_flags.2,.l11D0      ; 11C7
    mov !spc_sfx_tmp,#$1A             ; 11CA
    jmp SpcBPort1Command_l1840        ; 11CD
.l11D0:
    bbs !spc_loop_flags.1,.l118E      ; 11D0
    bbs !spc_loop_flags.5,.l118E      ; 11D3
    mov a,#$25                        ; 11D6
    jmp SpcBSoundQueuePush            ; 11D8
.l11DB:
    bbs !spc_loop_flags.4,.l118E      ; 11DB
    mov !spc_sfx_tmp,#$0B             ; 11DE
    jmp SpcBPort1Command_l1828        ; 11E1
.l11E4:
    bbs !spc_loop_flags.5,.l118E      ; 11E4
    mov !spc_sfx_tmp,#$1B             ; 11E7
    jmp SpcBPort1Command_l184B        ; 11EA

; @routine 1A:F1D2 SpcBGameStateTick
; WHAT IT DOES: The battle's sound rules each tick: the counters $056A/$056B after song request 5; song request 3 $E6 ticks after request 2 ($0567); while $056A is under $80, port commands $11-$14 (the list at $1251) are dropped; command $32 clears $2A, sets bits 5 and 6 of $2D and stops channels 2, 1 and 0, commands $03 and $10 stop channels 5 and 4 and do the same to $2A and $2D; the timer $0579 sets bit 5 of $2D after $78 ticks; port 1 commands $09 and $0D are cleared while channel 5 plays sound $17 or $21; channel 4 stops when channel 7 plays $32 and channel 4 $38. The race driver's (SpcGameStateTick) also has the countdown, tempo and finish music rules.
; HOW: Compare chains on $1D, $01, $0394-$0397; SpcBSfxStop.
; Entry: none. Exit: A, X changed.
; @spc 11ED
%spc($11ED, $1AF1D2)
SpcBGameStateTick:
    mov.w a,!spc_song5_count          ; 11ED
    cmp a,#$FF                        ; 11F0
    beq .l11F7                        ; 11F2
    inc.w !spc_song5_count            ; 11F4
.l11F7:
    mov.b a,!spc_song_request         ; 11F7
    cmp a,#$05                        ; 11F9
    beq .l11FF                        ; 11FB
    bra .l1207                        ; 11FD
.l11FF:
    mov a,#$00                        ; 11FF
    mov.w !spc_song5_count,a          ; 1201
    mov.w !spc_song5_scale,a          ; 1204
.l1207:
    mov.w a,!spc_song5_count          ; 1207
    bmi .l120E                        ; 120A
    bra .l1218                        ; 120C
.l120E:
    mov.w a,!spc_song5_scale          ; 120E
    cmp a,#$FF                        ; 1211
    beq .l1218                        ; 1213
    inc.w !spc_song5_scale            ; 1215
.l1218:
    cmp !spc_song_request,#$02        ; 1218
    bne .l1222                        ; 121B
    mov a,#$01                        ; 121D
    mov.w !spc_song3_delay,a          ; 121F
.l1222:
    mov.w a,!spc_song3_delay          ; 1222
    beq .l1239                        ; 1225
    inc.w !spc_song3_delay            ; 1227
    mov.w a,!spc_song3_delay          ; 122A
    cmp a,#$E6                        ; 122D
    bne .l1239                        ; 122F
    mov a,#$00                        ; 1231
    mov.w !spc_song3_delay,a          ; 1233
    mov !spc_song_request,#$03        ; 1236
.l1239:
    mov.w a,!spc_song5_count          ; 1239
    bmi .l1256                        ; 123C
    mov x,#$00                        ; 123E
.l1240:
    mov.w a,.t1251+x                  ; 1240
    beq .l1256                        ; 1243
    cmp.b a,!spc_command              ; 1245
    beq .l124C                        ; 1247
    inc x                             ; 1249
    bra .l1240                        ; 124A
.l124C:
    mov !spc_command,#$00             ; 124C  ; not reached in the traces so far
    bra .l1256                        ; 124F  ; not reached in the traces so far
.t1251:
    db $11, $12, $13, $14, $00
.l1256:
    cmp !spc_command,#$32             ; 1256
    bne .l126F                        ; 1259
    mov !spc_loop_flags,#$00          ; 125B
    set1 !spc_game_flags.5            ; 125E
    set1 !spc_game_flags.6            ; 1260
    mov x,#$02                        ; 1262
    call SpcBSfxStop                  ; 1264
    dec x                             ; 1267
    call SpcBSfxStop                  ; 1268
    dec x                             ; 126B
    call SpcBSfxStop                  ; 126C
.l126F:
    cmp !spc_command,#$03             ; 126F
    beq .l127B                        ; 1272
    cmp !spc_command,#$10             ; 1274
    beq .l127B                        ; 1277
    bra .l128B                        ; 1279
.l127B:
    mov x,#$05                        ; 127B  ; not reached in the traces so far
    call SpcBSfxStop                  ; 127D  ; not reached in the traces so far
    dec x                             ; 1280  ; not reached in the traces so far
    call SpcBSfxStop                  ; 1281  ; not reached in the traces so far
    mov !spc_loop_flags,#$00          ; 1284  ; not reached in the traces so far
    set1 !spc_game_flags.6            ; 1287  ; not reached in the traces so far
    set1 !spc_game_flags.5            ; 1289  ; not reached in the traces so far
.l128B:
    mov.w a,!spc_timer_4c             ; 128B
    beq .l12A2                        ; 128E
    inc.w !spc_timer_4c               ; 1290  ; not reached in the traces so far
    mov.w a,!spc_timer_4c             ; 1293  ; not reached in the traces so far
    setc                              ; 1296  ; not reached in the traces so far
    sbc a,#$78                        ; 1297  ; not reached in the traces so far
    bmi .l12A2                        ; 1299  ; not reached in the traces so far
    set1 !spc_game_flags.5            ; 129B  ; not reached in the traces so far
    mov a,#$00                        ; 129D  ; not reached in the traces so far
    mov.w !spc_timer_4c,a             ; 129F  ; not reached in the traces so far
.l12A2:
    mov.w a,!spc_sfx_command+5        ; 12A2
    cmp a,#$17                        ; 12A5
    beq .l12AF                        ; 12A7
    cmp a,#$21                        ; 12A9
    beq .l12AF                        ; 12AB
    bra .l12BE                        ; 12AD
.l12AF:
    cmp !spc_port_in+1,#$09           ; 12AF
    beq .l12BB                        ; 12B2
    cmp !spc_port_in+1,#$0D           ; 12B4
    beq .l12BB                        ; 12B7
    bra .l12BE                        ; 12B9
.l12BB:
    mov !spc_port_in+1,#$00           ; 12BB
.l12BE:
    mov.w a,!spc_sfx_command+7        ; 12BE
    cmp a,#$32                        ; 12C1
    bne .l12D1                        ; 12C3
    mov.w a,!spc_sfx_command+4        ; 12C5
    cmp a,#$38                        ; 12C8
    bne .l12D1                        ; 12CA
    mov x,#$04                        ; 12CC
    call SpcBSfxStop                  ; 12CE
.l12D1:
    ret                               ; 12D1

; @routine 1A:F2B7 SpcBCommand4C
; WHAT IT DOES: Port command $4C: channels 7 and 6 stopped with their fades (SpcBSfxStopRequest), 5, 4 and 3 stopped, the timer $0579 started, $2A cleared, the music paused, bit 6 of $2D set. As the race driver's (SpcCommand4C) without its bit 3 of $2D.
; HOW: CALLs with X counting down.
; Entry: none. Exit: as SpcMusicPause.
; @spc 12D2
%spc($12D2, $1AF2B7)
SpcBCommand4C:
    cmp !spc_command,#$4C             ; 12D2
    bne .l12F9                        ; 12D5
    mov x,#$07                        ; 12D7  ; not reached in the traces so far
    call SpcBSfxStopRequest           ; 12D9  ; not reached in the traces so far
    dec x                             ; 12DC  ; not reached in the traces so far
    call SpcBSfxStopRequest           ; 12DD  ; not reached in the traces so far
    dec x                             ; 12E0  ; not reached in the traces so far
    call SpcBSfxStop                  ; 12E1  ; not reached in the traces so far
    dec x                             ; 12E4  ; not reached in the traces so far
    call SpcBSfxStop                  ; 12E5  ; not reached in the traces so far
    dec x                             ; 12E8  ; not reached in the traces so far
    call SpcBSfxStop                  ; 12E9  ; not reached in the traces so far
    mov a,#$01                        ; 12EC  ; not reached in the traces so far
    mov.w !spc_timer_4c,a             ; 12EE  ; not reached in the traces so far
    mov !spc_loop_flags,#$00          ; 12F1  ; not reached in the traces so far
    call SpcMusicPause                ; 12F4  ; not reached in the traces so far
    set1 !spc_game_flags.6            ; 12F7  ; not reached in the traces so far
.l12F9:
    ret                               ; 12F9

; @routine 1A:F2DF SpcBSoundQueuePush
; WHAT IT DOES: Appends sound A to the zero-ended queue at $0588 (the race driver's SpcSoundQueuePush).
; HOW: Scans for the first zero byte; X kept on the stack.
; Entry: A = sound. Exit: A, X kept.
; @spc 12FA
%spc($12FA, $1AF2DF)
SpcBSoundQueuePush:
    push x                            ; 12FA
    push a                            ; 12FB
    mov x,#$00                        ; 12FC
.l12FE:
    mov.w a,!spc_sound_queue+x        ; 12FE
    beq .l1306                        ; 1301
    inc x                             ; 1303
    bra .l12FE                        ; 1304
.l1306:
    pop a                             ; 1306
    mov.w !spc_sound_queue+x,a        ; 1307
    pop x                             ; 130A
.l130B:
    ret                               ; 130B

; @routine 1A:F2F1 SpcBCommandTimers
; WHAT IT DOES: Unless stopped (bit 0 of $29): command $3E starts the count $0568 (bit 7 set with bit 7 of port 0), which at $2A queues sound $2F or $0F by channel 7's sound $0397 ($77/$79 or $78/$7A), or channel 6's $0396 when bit 7 is set, with bit 7 carried into the sound; commands $20 and $16 mark bits 4 and 5 of $2C, become command $1B or $27 by the track and start the count $055D, which at $11 queues $20 or $16. The race driver's (SpcCommandTimers) also has commands $37, $30 and $29.
; HOW: Byte counters in RAM; ASL of $0568 for its bit 7; SpcBSoundQueuePush.
; Entry: none. Exit: A, X changed.
; @spc 130C
%spc($130C, $1AF2F1)
SpcBCommandTimers:
    bbs !spc_flags.0,SpcBSoundQueuePush_l130B; 130C
    cmp !spc_command,#$3E             ; 130F
    bne .l1325                        ; 1312
    mov a,#$01                        ; 1314
    mov.w !spc_count_3e,a             ; 1316
    bbc !spc_port_in.7,.l1325         ; 1319
    mov.b !spc_sfx_tmp,a              ; 131C
    set1 !spc_sfx_tmp.7               ; 131E
    mov.b a,!spc_sfx_tmp              ; 1320
    mov.w !spc_count_3e,a             ; 1322
.l1325:
    mov.w a,!spc_count_3e             ; 1325
    beq .l132D                        ; 1328
    inc.w !spc_count_3e               ; 132A
.l132D:
    mov.w a,!spc_count_3e             ; 132D
    and a,#$7F                        ; 1330
    cmp a,#$2A                        ; 1332
    bne .l136B                        ; 1334
    mov.w a,!spc_count_3e             ; 1336
    asl a                             ; 1339
    bcs .l1351                        ; 133A
    mov.w a,!spc_sfx_command+7        ; 133C
.l133F:
    cmp a,#$77                        ; 133F
    beq .l135A                        ; 1341
    cmp a,#$78                        ; 1343
    beq .l1356                        ; 1345
    cmp a,#$79                        ; 1347  ; not reached in the traces so far
    beq .l135A                        ; 1349  ; not reached in the traces so far
    cmp a,#$7A                        ; 134B  ; not reached in the traces so far
    beq .l1356                        ; 134D  ; not reached in the traces so far
    bra .l1366                        ; 134F  ; not reached in the traces so far
.l1351:
    mov.w a,!spc_sfx_command+6        ; 1351
    bra .l133F                        ; 1354
.l1356:
    mov a,#$0F                        ; 1356
    bra .l135C                        ; 1358
.l135A:
    mov a,#$2F                        ; 135A
.l135C:
    asl.w !spc_count_3e               ; 135C
    bcc .l1363                        ; 135F
    or a,#$80                         ; 1361
.l1363:
    call SpcBSoundQueuePush           ; 1363
.l1366:
    mov a,#$00                        ; 1366
    mov.w !spc_count_3e,a             ; 1368
.l136B:
    cmp !spc_command,#$20             ; 136B
    beq .l1377                        ; 136E
    cmp !spc_command,#$16             ; 1370
    beq .l137B                        ; 1373
    bra .l1399                        ; 1375
.l1377:
    set1 !spc_track_flags.4           ; 1377  ; not reached in the traces so far
    bra .l137D                        ; 1379  ; not reached in the traces so far
.l137B:
    set1 !spc_track_flags.5           ; 137B  ; not reached in the traces so far
.l137D:
    mov.w a,!spc_sfx_command+7        ; 137D  ; not reached in the traces so far
    cmp a,#$01                        ; 1380  ; not reached in the traces so far
    beq .l1391                        ; 1382  ; not reached in the traces so far
    cmp a,#$04                        ; 1384  ; not reached in the traces so far
    beq .l138C                        ; 1386  ; not reached in the traces so far
    cmp a,#$07                        ; 1388  ; not reached in the traces so far
    beq .l1391                        ; 138A  ; not reached in the traces so far
.l138C:
    mov !spc_command,#$1B             ; 138C  ; not reached in the traces so far
    bra .l1394                        ; 138F  ; not reached in the traces so far
.l1391:
    mov !spc_command,#$27             ; 1391  ; not reached in the traces so far
.l1394:
    mov a,#$01                        ; 1394  ; not reached in the traces so far
    mov.w !spc_count_20_16,a          ; 1396  ; not reached in the traces so far
.l1399:
    mov.w a,!spc_count_20_16          ; 1399
    beq .l13A1                        ; 139C
    inc.w !spc_count_20_16            ; 139E  ; not reached in the traces so far
.l13A1:
    mov.w a,!spc_count_20_16          ; 13A1
    cmp a,#$11                        ; 13A4
    bne .l13C5                        ; 13A6
    mov a,#$00                        ; 13A8  ; not reached in the traces so far
    mov.w !spc_count_20_16,a          ; 13AA  ; not reached in the traces so far
    bbs !spc_track_flags.4,.l13B5     ; 13AD  ; not reached in the traces so far
    bbs !spc_track_flags.5,.l13BE     ; 13B0  ; not reached in the traces so far
    bra .l13C5                        ; 13B3  ; not reached in the traces so far
.l13B5:
    clr1 !spc_track_flags.4           ; 13B5  ; not reached in the traces so far
    mov a,#$20                        ; 13B7  ; not reached in the traces so far
    call SpcBSoundQueuePush           ; 13B9  ; not reached in the traces so far
    bra .l13C5                        ; 13BC  ; not reached in the traces so far
.l13BE:
    clr1 !spc_track_flags.5           ; 13BE  ; not reached in the traces so far
    mov a,#$16                        ; 13C0  ; not reached in the traces so far
    call SpcBSoundQueuePush           ; 13C2  ; not reached in the traces so far
.l13C5:
    ret                               ; 13C5

; @routine 1A:F3AB SpcBTempoBend
; WHAT IT DOES: The tempo bend (SpcBTempoBendStart decides it), as the race driver's SpcTempoBend: while bit 2 of $2C is set, each tick lowers the transpose by the step count / $15 and the tempo by an accumulated amount from its saved values ($0566, $0578); bit 3 restores them and stops.
; HOW: Word accumulator $0553/$0554, DIV by $15.
; Entry: none. Exit: A = the tempo high byte.
; @spc 13C6
%spc($13C6, $1AF3AB)
SpcBTempoBend:
    call SpcBTempoBendStart           ; 13C6
    bbs !spc_track_flags.2,.l13D0     ; 13C9
    bbs !spc_track_flags.3,.l13DB     ; 13CC
.l13CF:
    ret                               ; 13CF
.l13D0:
    mov.w a,!spc_bend_count           ; 13D0  ; not reached in the traces so far
    beq .l13F7                        ; 13D3
    cmp a,#$FE                        ; 13D5  ; not reached in the traces so far
    beq .l13CF                        ; 13D7  ; not reached in the traces so far
    bra .l140F                        ; 13D9  ; not reached in the traces so far
.l13DB:
    clr1 !spc_track_flags.2           ; 13DB  ; not reached in the traces so far
    clr1 !spc_track_flags.3           ; 13DD  ; not reached in the traces so far
    mov a,#$00                        ; 13DF  ; not reached in the traces so far
    mov.w !spc_bend_count,a           ; 13E1  ; not reached in the traces so far
    mov.w !spc_bend_acc,a             ; 13E4  ; not reached in the traces so far
    mov.w !spc_bend_acc+1,a           ; 13E7  ; not reached in the traces so far
    mov.w !spc_bend_tempo_step,a      ; 13EA  ; not reached in the traces so far
    mov.w a,!spc_bend_tempo           ; 13ED  ; not reached in the traces so far
    mov.b !spc_transpose,a            ; 13F0  ; not reached in the traces so far
    mov.w a,!spc_bend_transpose       ; 13F2  ; not reached in the traces so far
    bra .l1441                        ; 13F5  ; not reached in the traces so far
.l13F7:
    mov a,#$04                        ; 13F7  ; not reached in the traces so far
    mov.w !spc_bend_acc+1,a           ; 13F9  ; not reached in the traces so far
    mov.b a,!spc_tempo+1              ; 13FC  ; not reached in the traces so far
    mov.w !spc_bend_transpose,a       ; 13FE  ; not reached in the traces so far
    mov.b a,!spc_transpose            ; 1401  ; not reached in the traces so far
    mov.w !spc_bend_tempo,a           ; 1403  ; not reached in the traces so far
    mov y,#$7F                        ; 1406  ; not reached in the traces so far
    mul ya                            ; 1408  ; not reached in the traces so far
    mov x,#$FE                        ; 1409  ; not reached in the traces so far
    div ya,x                          ; 140B  ; not reached in the traces so far
    mov.w !spc_bend_tempo_step,a      ; 140C  ; not reached in the traces so far
.l140F:
    inc.w !spc_bend_count             ; 140F  ; not reached in the traces so far
    mov.w a,!spc_bend_count           ; 1412  ; not reached in the traces so far
    mov y,#$00                        ; 1415  ; not reached in the traces so far
    mov x,#$15                        ; 1417  ; not reached in the traces so far
    div ya,x                          ; 1419  ; not reached in the traces so far
    mov.b !spc_sfx_tmp,a              ; 141A  ; not reached in the traces so far
    setc                              ; 141C  ; not reached in the traces so far
    mov.w a,!spc_bend_tempo           ; 141D  ; not reached in the traces so far
    sbc.b a,!spc_sfx_tmp              ; 1420  ; not reached in the traces so far
    mov.b !spc_transpose,a            ; 1422  ; not reached in the traces so far
    mov.w a,!spc_bend_tempo_step      ; 1424  ; not reached in the traces so far
    mov.b !spc_sfx_tmp,a              ; 1427  ; not reached in the traces so far
    mov !spc_sfx_tmp+1,#$00           ; 1429  ; not reached in the traces so far
    mov.w a,!spc_bend_acc             ; 142C  ; not reached in the traces so far
    mov.w y,!spc_bend_acc+1           ; 142F  ; not reached in the traces so far
    addw ya,!spc_sfx_tmp              ; 1432  ; not reached in the traces so far
    mov.w !spc_bend_acc,a             ; 1434  ; not reached in the traces so far
    mov.w !spc_bend_acc+1,y           ; 1437  ; not reached in the traces so far
    mov.w a,!spc_bend_transpose       ; 143A  ; not reached in the traces so far
    setc                              ; 143D  ; not reached in the traces so far
    sbc.w a,!spc_bend_acc+1           ; 143E  ; not reached in the traces so far
.l1441:
    mov.b !spc_tempo+1,a              ; 1441  ; not reached in the traces so far
    ret                               ; 1443  ; not reached in the traces so far

; @routine 1A:F429 SpcBTempoBendStart
; WHAT IT DOES: Command $2C during song 1 starts the tempo bend (bit 2 of $2C); commands $30 and $3C during a bend end it (bit 3). The race driver's SpcTempoBendStart.
; HOW: SET1/CLR1 on $2C.
; Entry: none. Exit: flags from the compares.
; @spc 1444
%spc($1444, $1AF429)
SpcBTempoBendStart:
    mov.b a,!spc_command              ; 1444
    cmp a,#$2C                        ; 1446
    beq .l1458                        ; 1448
    mov.w x,!spc_bend_count           ; 144A
    beq .l1461                        ; 144D
    cmp a,#$30                        ; 144F  ; not reached in the traces so far
    beq .l1462                        ; 1451  ; not reached in the traces so far
    cmp a,#$3C                        ; 1453  ; not reached in the traces so far
    beq .l1462                        ; 1455  ; not reached in the traces so far
    ret                               ; 1457  ; not reached in the traces so far
.l1458:
    cmp !spc_song,#$01                ; 1458  ; not reached in the traces so far
    bne .l1461                        ; 145B  ; not reached in the traces so far
    set1 !spc_track_flags.2           ; 145D  ; not reached in the traces so far
    clr1 !spc_track_flags.3           ; 145F  ; not reached in the traces so far
.l1461:
    ret                               ; 1461
.l1462:
    clr1 !spc_track_flags.2           ; 1462  ; not reached in the traces so far
    set1 !spc_track_flags.3           ; 1464  ; not reached in the traces so far
    ret                               ; 1466  ; not reached in the traces so far

; @routine 1A:F44C SpcBPort1Clamp
; WHAT IT DOES: Outside song 1, a port 1 command of 7 or more is ignored ($01 cleared). The race driver's SpcPort1Clamp.
; HOW: SBC and BMI.
; Entry: none. Exit: flags.
; @spc 1467
%spc($1467, $1AF44C)
SpcBPort1Clamp:
    cmp !spc_song,#$01                ; 1467
    beq .l1476                        ; 146A
    mov.b a,!spc_port_in+1            ; 146C
    setc                              ; 146E
    sbc a,#$07                        ; 146F
    bmi .l1476                        ; 1471
    mov !spc_port_in+1,#$00           ; 1473
.l1476:
    ret                               ; 1476

; @routine 1A:F45C SpcBCommandFilter
; WHAT IT DOES: Outside song 1, a port command $1D not in the list at $148F is dropped. As the race driver's SpcCommandFilter, with the battle's list.
; HOW: A zero-ended list scan.
; Entry: none. Exit: X = list index.
; @spc 1477
%spc($1477, $1AF45C)
SpcBCommandFilter:
    mov.b a,!spc_song                 ; 1477
    cmp a,#$01                        ; 1479
    beq .l148E                        ; 147B
    mov x,#$00                        ; 147D
.l147F:
    mov.w a,.t148F+x                  ; 147F
    beq .l148B                        ; 1482
    cmp.b a,!spc_command              ; 1484
    beq .l148E                        ; 1486
    inc x                             ; 1488
    bra .l147F                        ; 1489
.l148B:
    mov !spc_command,#$00             ; 148B
.l148E:
    ret                               ; 148E
.t148F:
    db $03, $10, $32, $2B, $34, $43, $44, $45
    db $77, $78, $79, $7A, $23, $3A, $3B, $3F
    db $06, $22, $41, $40, $1F, $2A, $00

; @routine 1A:F48B SpcBCommandBlock
; WHAT IT DOES: A sound command in the list at $14B8 is dropped. As the race driver's SpcCommandBlock, with a longer list.
; HOW: A zero-ended list scan.
; Entry: none. Exit: X = index.
; @spc 14A6
%spc($14A6, $1AF48B)
SpcBCommandBlock:
    mov x,#$00                        ; 14A6
.l14A8:
    mov.w a,.t14B8+x                  ; 14A8
    beq .l14B7                        ; 14AB
    cmp.b a,!spc_command              ; 14AD
    beq .l14B4                        ; 14AF
    inc x                             ; 14B1
    bra .l14A8                        ; 14B2
.l14B4:
    mov !spc_command,#$00             ; 14B4
.l14B7:
    ret                               ; 14B7
.t14B8:
    db $02, $01, $04, $07, $0A, $0D, $25, $15
    db $17, $1E, $21, $18, $19, $1A, $09, $0C
    db $0B, $2E, $4E, $2D, $29, $4D, $33, $5A
    db $00

; @routine 1A:F4B6 SpcBPitchRampB
; WHAT IT DOES: The second pair of pitch ramps: bit 2 of $28 ramps the offset $054D/$054F up (SpcBPitchRampB1Up), bit 3 down (SpcBPitchRampB1Down); bits 4 and 5 do the same to $054C/$054E (SpcBPitchRampB2Up, SpcBPitchRampB2Down). The race driver (SpcPitchRampB) has only the first.
; HOW: BBC tests.
; Entry: none. Exit: as the last ramp leaves it.
; @spc 14D1
%spc($14D1, $1AF4B6)
SpcBPitchRampB:
    bbc !spc_effect_flags.2,.l14D7    ; 14D1
    call SpcBPitchRampB1Up            ; 14D4
.l14D7:
    bbc !spc_effect_flags.3,.l14DD    ; 14D7
    call SpcBPitchRampB1Down          ; 14DA
.l14DD:
    bbc !spc_effect_flags.4,.l14E3    ; 14DD
    call SpcBPitchRampB2Up            ; 14E0
.l14E3:
    bbc !spc_effect_flags.5,.l14E9    ; 14E3
    call SpcBPitchRampB2Down          ; 14E6
.l14E9:
    ret                               ; 14E9

; @routine 1A:F4CF SpcBPitchRampB1Up
; WHAT IT DOES: Up to $14 steps (count $0547), adds $20 to the pitch offset $054D/$054F. The race driver's SpcPitchRampBUp.
; HOW: ADDW through $20/$21.
; Entry: none. Exit: YA = the offset.
; @spc 14EA
%spc($14EA, $1AF4CF)
SpcBPitchRampB1Up:
    mov.w a,!spc_ramp_b_steps         ; 14EA
    cmp a,#$14                        ; 14ED
    beq .l150A                        ; 14EF
    inc.w !spc_ramp_b_steps           ; 14F1
    mov.w a,!spc_ramp_b_pitch         ; 14F4
    mov.b !spc_sfx_tmp,a              ; 14F7
    mov.w a,!spc_ramp_b_pitch_hi      ; 14F9
    mov.b !spc_sfx_tmp+1,a            ; 14FC
    mov a,#$20                        ; 14FE
    mov y,#$00                        ; 1500
    addw ya,!spc_sfx_tmp              ; 1502
    mov.w !spc_ramp_b_pitch,a         ; 1504
    mov.w !spc_ramp_b_pitch_hi,y      ; 1507
.l150A:
    ret                               ; 150A

; @routine 1A:F4F0 SpcBPitchRampB2Up
; WHAT IT DOES: Up to $14 steps (count $0546), adds $20 to the pitch offset $054C/$054E.
; HOW: ADDW through $20/$21.
; Entry: none. Exit: YA = the offset.
; @spc 150B
%spc($150B, $1AF4F0)
SpcBPitchRampB2Up:
    mov.w a,!spcb_ramp_b2_steps       ; 150B
    cmp a,#$14                        ; 150E
    beq .l152B                        ; 1510
    inc.w !spcb_ramp_b2_steps         ; 1512
    mov.w a,!spcb_ramp_b2_pitch       ; 1515
    mov.b !spc_sfx_tmp,a              ; 1518
    mov.w a,!spcb_ramp_b2_pitch_hi    ; 151A
    mov.b !spc_sfx_tmp+1,a            ; 151D
    mov a,#$20                        ; 151F
    mov y,#$00                        ; 1521
    addw ya,!spc_sfx_tmp              ; 1523
    mov.w !spcb_ramp_b2_pitch,a       ; 1525
    mov.w !spcb_ramp_b2_pitch_hi,y    ; 1528
.l152B:
    ret                               ; 152B

; @routine 1A:F511 SpcBPitchRampB1Down
; WHAT IT DOES: Counts $0547 down, taking $20 from the offset $054D/$054F; at 0 clears bits 2 and 3 of $28 and the offset. The race driver's SpcPitchRampBDown.
; HOW: SUBW through $20/$21.
; Entry: none. Exit: A = 0 at the end.
; @spc 152C
%spc($152C, $1AF511)
SpcBPitchRampB1Down:
    mov.w a,!spc_ramp_b_steps         ; 152C
    cmp a,#$00                        ; 152F
    beq .l154B                        ; 1531
    dec.w !spc_ramp_b_steps           ; 1533
    mov.w a,!spc_ramp_b_pitch         ; 1536
    mov.w y,!spc_ramp_b_pitch_hi      ; 1539
    mov !spc_sfx_tmp,#$20             ; 153C
    mov !spc_sfx_tmp+1,#$00           ; 153F
    subw ya,!spc_sfx_tmp              ; 1542
    mov.w !spc_ramp_b_pitch,a         ; 1544
    mov.w !spc_ramp_b_pitch_hi,y      ; 1547
    ret                               ; 154A
.l154B:
    clr1 !spc_effect_flags.2          ; 154B
    clr1 !spc_effect_flags.3          ; 154D
    mov a,#$00                        ; 154F
    mov.w !spc_ramp_b_pitch,a         ; 1551
    mov.w !spc_ramp_b_pitch_hi,a      ; 1554
    ret                               ; 1557

; @routine 1A:F53D SpcBPitchRampB2Down
; WHAT IT DOES: Counts $0546 down, taking $20 from the offset $054C/$054E; at 0 clears bits 4 and 5 of $28 and the offset.
; HOW: SUBW through $20/$21.
; Entry: none. Exit: A = 0 at the end.
; @spc 1558
%spc($1558, $1AF53D)
SpcBPitchRampB2Down:
    mov.w a,!spcb_ramp_b2_steps       ; 1558
    cmp a,#$00                        ; 155B
    beq .l1577                        ; 155D
    dec.w !spcb_ramp_b2_steps         ; 155F
    mov.w a,!spcb_ramp_b2_pitch       ; 1562
    mov.w y,!spcb_ramp_b2_pitch_hi    ; 1565
    mov !spc_sfx_tmp,#$20             ; 1568
    mov !spc_sfx_tmp+1,#$00           ; 156B
    subw ya,!spc_sfx_tmp              ; 156E
    mov.w !spcb_ramp_b2_pitch,a       ; 1570
    mov.w !spcb_ramp_b2_pitch_hi,y    ; 1573
    ret                               ; 1576
.l1577:
    clr1 !spc_effect_flags.4          ; 1577
    clr1 !spc_effect_flags.5          ; 1579
    mov a,#$00                        ; 157B
    mov.w !spcb_ramp_b2_pitch,a       ; 157D
    mov.w !spcb_ramp_b2_pitch_hi,a    ; 1580
    ret                               ; 1583

; @routine 1A:F569 SpcBPitchRampA
; WHAT IT DOES: The first pair of pitch ramps: bit 2 of $29 ramps the offset $0549/$054B up (SpcBPitchRampA1Up), bit 3 down (SpcBPitchRampA1Down); bits 4 and 5 do the same to $0548/$054A (SpcBPitchRampA2Up, SpcBPitchRampA2Down). The race driver (SpcPitchRampA) has only the first.
; HOW: BBC tests.
; Entry: none. Exit: as the last ramp leaves it.
; @spc 1584
%spc($1584, $1AF569)
SpcBPitchRampA:
    bbc !spc_flags.2,.l158A           ; 1584
    call SpcBPitchRampA1Up            ; 1587
.l158A:
    bbc !spc_flags.3,.l1590           ; 158A
    call SpcBPitchRampA1Down          ; 158D
.l1590:
    bbc !spc_flags.4,.l1596           ; 1590
    call SpcBPitchRampA2Up            ; 1593
.l1596:
    bbc !spc_flags.5,.l159C           ; 1596
    call SpcBPitchRampA2Down          ; 1599
.l159C:
    ret                               ; 159C

; @routine 1A:F582 SpcBPitchRampA1Up
; WHAT IT DOES: Up to $19 steps (count $0543), adds $3C to the offset $0549/$054B. The race driver's SpcPitchRampAUp.
; HOW: ADDW through $20/$21.
; Entry: none. Exit: YA = the offset.
; @spc 159D
%spc($159D, $1AF582)
SpcBPitchRampA1Up:
    mov.w a,!spc_ramp_a_steps         ; 159D
    cmp a,#$19                        ; 15A0
    beq .l15BD                        ; 15A2
    inc.w !spc_ramp_a_steps           ; 15A4
    mov.w a,!spc_ramp_a_pitch         ; 15A7
    mov.b !spc_sfx_tmp,a              ; 15AA
    mov.w a,!spc_ramp_a_pitch_hi      ; 15AC
    mov.b !spc_sfx_tmp+1,a            ; 15AF
    mov a,#$3C                        ; 15B1
    mov y,#$00                        ; 15B3
    addw ya,!spc_sfx_tmp              ; 15B5
    mov.w !spc_ramp_a_pitch,a         ; 15B7
    mov.w !spc_ramp_a_pitch_hi,y      ; 15BA
.l15BD:
    ret                               ; 15BD

; @routine 1A:F5A3 SpcBPitchRampA1Down
; WHAT IT DOES: Counts $0543 down, taking $3C from the offset $0549/$054B; at 0 ends the ramp (SpcBPitchRampA1End). The race driver's SpcPitchRampADown.
; HOW: SUBW through $20/$21; a branch into SpcBPitchRampA1End.
; Entry: none. Exit: YA = the offset, or as SpcBPitchRampA1End.
; @spc 15BE
%spc($15BE, $1AF5A3)
SpcBPitchRampA1Down:
    mov.w a,!spc_ramp_a_steps         ; 15BE
    cmp a,#$00                        ; 15C1
    beq SpcBPitchRampA1End            ; 15C3
    dec.w !spc_ramp_a_steps           ; 15C5
    mov.w a,!spc_ramp_a_pitch         ; 15C8
    mov.w y,!spc_ramp_a_pitch_hi      ; 15CB
    mov !spc_sfx_tmp,#$3C             ; 15CE
    mov !spc_sfx_tmp+1,#$00           ; 15D1
    subw ya,!spc_sfx_tmp              ; 15D4
    mov.w !spc_ramp_a_pitch,a         ; 15D6
    mov.w !spc_ramp_a_pitch_hi,y      ; 15D9
    ret                               ; 15DC

; @routine 1A:F5C2 SpcBPitchRampA1End
; WHAT IT DOES: Ends the first ramp: clears bits 2 and 3 of $29 and the offset $0549/$054B.
; HOW: CLR1, then zero stores.
; Entry: none. Exit: A = 0.
; @spc 15DD
%spc($15DD, $1AF5C2)
SpcBPitchRampA1End:
    clr1 !spc_flags.2                 ; 15DD
    clr1 !spc_flags.3                 ; 15DF
    mov a,#$00                        ; 15E1
    mov.w !spc_ramp_a_pitch,a         ; 15E3
    mov.w !spc_ramp_a_pitch_hi,a      ; 15E6
    ret                               ; 15E9

; @routine 1A:F5CF SpcBPitchRampA2Up
; WHAT IT DOES: Up to $19 steps (count $0542), adds $3C to the offset $0548/$054A.
; HOW: ADDW through $20/$21.
; Entry: none. Exit: YA = the offset.
; @spc 15EA
%spc($15EA, $1AF5CF)
SpcBPitchRampA2Up:
    mov.w a,!spcb_ramp_a2_steps       ; 15EA
    cmp a,#$19                        ; 15ED
    beq .l160A                        ; 15EF
    inc.w !spcb_ramp_a2_steps         ; 15F1
    mov.w a,!spcb_ramp_a2_pitch       ; 15F4
    mov.b !spc_sfx_tmp,a              ; 15F7
    mov.w a,!spcb_ramp_a2_pitch_hi    ; 15F9
    mov.b !spc_sfx_tmp+1,a            ; 15FC
    mov a,#$3C                        ; 15FE
    mov y,#$00                        ; 1600
    addw ya,!spc_sfx_tmp              ; 1602
    mov.w !spcb_ramp_a2_pitch,a       ; 1604
    mov.w !spcb_ramp_a2_pitch_hi,y    ; 1607
.l160A:
    ret                               ; 160A

; @routine 1A:F5F0 SpcBPitchRampA2Down
; WHAT IT DOES: Counts $0542 down, taking $3C from the offset $0548/$054A; at 0 clears bits 4 and 5 of $29 and the offset.
; HOW: SUBW through $20/$21.
; Entry: none. Exit: A = 0 at the end.
; @spc 160B
%spc($160B, $1AF5F0)
SpcBPitchRampA2Down:
    mov.w a,!spcb_ramp_a2_steps       ; 160B
    cmp a,#$00                        ; 160E
    beq .l162A                        ; 1610
    dec.w !spcb_ramp_a2_steps         ; 1612
    mov.w a,!spcb_ramp_a2_pitch       ; 1615
    mov.w y,!spcb_ramp_a2_pitch_hi    ; 1618
    mov !spc_sfx_tmp,#$3C             ; 161B
    mov !spc_sfx_tmp+1,#$00           ; 161E
    subw ya,!spc_sfx_tmp              ; 1621
    mov.w !spcb_ramp_a2_pitch,a       ; 1623
    mov.w !spcb_ramp_a2_pitch_hi,y    ; 1626
    ret                               ; 1629
.l162A:
    clr1 !spc_flags.4                 ; 162A
    clr1 !spc_flags.5                 ; 162C
    mov a,#$00                        ; 162E
    mov.w !spcb_ramp_a2_pitch,a       ; 1630
    mov.w !spcb_ramp_a2_pitch_hi,a    ; 1633
    ret                               ; 1636

; @routine 1A:F61C SpcBPortCommand
; WHAT IT DOES: The port 0 command $00 (bit 7 dropped), as the race driver's SpcPortCommand: 6 and up is a sound command, $1D = value - 5; 1-5 (unless stopped, bit 0 of $29) is a song request, which also clears the tempo bend and fade state and flags 0, 1, 6 of $29 and 2, 3 of $2C; 0 leaves $1D = 0.
; HOW: SBC #6 and BMI.
; Entry: none. Exit: A as stored.
; @spc 1637
%spc($1637, $1AF61C)
SpcBPortCommand:
    mov !spc_command,#$00             ; 1637
    mov.b a,!spc_port_in              ; 163A
    and a,#$7F                        ; 163C
    beq .l1649                        ; 163E
    setc                              ; 1640
    sbc a,#$06                        ; 1641
    bmi .l164C                        ; 1643
    inc a                             ; 1645
    mov.b !spc_command,a              ; 1646
    ret                               ; 1648
.l1649:
    mov.b !spc_command,a              ; 1649
    ret                               ; 164B
.l164C:
    bbs !spc_flags.0,.l1670           ; 164C
    mov.b a,!spc_port_in              ; 164F
    mov.b !spc_song_request,a         ; 1651
    cmp a,#$02                        ; 1653
    bne .l1657                        ; 1655
.l1657:
    mov a,#$00                        ; 1657
    mov.b !spc_fade_amount,a          ; 1659
    mov.b !spc_fade_amount+1,a        ; 165B
    mov.w !spc_bend_acc,a             ; 165D
    mov.w !spc_bend_acc+1,a           ; 1660
    mov.w !spc_bend_count,a           ; 1663
    clr1 !spc_flags.0                 ; 1666
    clr1 !spc_flags.1                 ; 1668
    clr1 !spc_flags.6                 ; 166A
    clr1 !spc_track_flags.2           ; 166C
    clr1 !spc_track_flags.3           ; 166E
.l1670:
    ret                               ; 1670

; @routine 1A:F656 SpcBPort1Command
; WHAT IT DOES: A new port 1 command ($01 differing from the last, $09; below $3E, as $20) runs its handler from the 61-entry table at $1696. The battle's handlers: 9, $0D, $12, $14, $18, $19 start a looping sound ($25, $15, $17, $A5, $95, $97; a bit of $2A) unless bit 6 of $2D is set; $0A-$0C and $1A-$1C end one (the bit of $2A cleared, and channel 5's stop requested with reason $20 while it plays $25, $15 or $17; SpcBStaggeredChecks enters these with its own $20); $23-$26 start the first ramps up or down ($0200 on $0549/$054B or $0548/$054A, bits 2-5 of $29), $3A-$3D the second ($0040 on $054D/$054F or $054C/$054E, bits 2-5 of $28); $32-$35 set $0530 and $36-$39 set $0531 to the command's offset (0-3); 5 and 6 (in SpcBKeyOffAll), $15 (in SpcBSfxPointer), $16 and $22 (in SpcBSfxStop) stop, pause and resume; the rest do nothing. The race driver's (SpcPort1Command) has other commands.
; HOW: JMP ($1696+X) with X = (command - 1) * 2; the handlers end in RETs or tail jumps; the stop paths share the request at $1861 with SpcBSfxStop.
; Entry: none. Exit: as the handler leaves it.
; @spc 1671
%spc($1671, $1AF656)
SpcBPort1Command:
    mov.b a,!spc_port_in+1            ; 1671
    cmp.b a,!spc_port1_last           ; 1673
    bne .l167C                        ; 1675
    mov !spc_sfx_tmp,#$00             ; 1677
    bra .l167F                        ; 167A
.l167C:
    mov !spc_sfx_tmp,!spc_port_in+1   ; 167C
.l167F:
    mov !spc_port1_last,!spc_port_in+1; 167F
    mov.b a,!spc_sfx_tmp              ; 1682
    setc                              ; 1684
    sbc a,#$3E                        ; 1685
    bmi .l168C                        ; 1687
    mov !spc_sfx_tmp,#$00             ; 1689  ; not reached in the traces so far
.l168C:
    mov.b a,!spc_sfx_tmp              ; 168C
    beq SpcBPortCommand_l1670         ; 168E
    dec a                             ; 1690
    asl a                             ; 1691
    mov x,a                           ; 1692
    jmp (.t1696+x)                    ; 1693
.t1696:
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
    bbs !spc_game_flags.6,.l171A      ; 1710
    set1 !spc_loop_flags.2            ; 1713
    mov a,#$25                        ; 1715
    call SpcBSoundQueuePush           ; 1717
.l171A:
    ret                               ; 171A
    bbs !spc_game_flags.6,.l1725      ; 171B
    set1 !spc_loop_flags.4            ; 171E
    mov a,#$15                        ; 1720
    call SpcBSoundQueuePush           ; 1722
.l1725:
    ret                               ; 1725
    bbs !spc_game_flags.6,.l1730      ; 1726
    set1 !spc_loop_flags.0            ; 1729
    mov a,#$17                        ; 172B
    call SpcBSoundQueuePush           ; 172D
.l1730:
    ret                               ; 1730
    bbs !spc_game_flags.6,.l173B      ; 1731
    set1 !spc_loop_flags.3            ; 1734
    mov a,#$A5                        ; 1736
    call SpcBSoundQueuePush           ; 1738
.l173B:
    ret                               ; 173B
    bbs !spc_game_flags.6,.l1746      ; 173C
    set1 !spc_loop_flags.5            ; 173F
    mov a,#$95                        ; 1741
    call SpcBSoundQueuePush           ; 1743
.l1746:
    ret                               ; 1746
    bbs !spc_game_flags.6,.l1751      ; 1747
    set1 !spc_loop_flags.1            ; 174A
    mov a,#$97                        ; 174C
    call SpcBSoundQueuePush           ; 174E
.l1751:
    ret                               ; 1751
    bbs !spc_flags.2,.l17AE           ; 1752
    set1 !spc_flags.2                 ; 1755
    clr1 !spc_flags.3                 ; 1757
    mov a,#$00                        ; 1759
    mov y,#$02                        ; 175B
    mov.w !spc_ramp_a_pitch,a         ; 175D
    mov.w !spc_ramp_a_pitch_hi,y      ; 1760
    ret                               ; 1763
    bbc !spc_flags.2,.l17AE           ; 1764
    clr1 !spc_flags.2                 ; 1767
    set1 !spc_flags.3                 ; 1769
    mov.w a,!spc_ramp_a_pitch         ; 176B
    mov.w y,!spc_ramp_a_pitch_hi      ; 176E
    mov !spc_sfx_tmp,#$00             ; 1771
    mov !spc_sfx_tmp+1,#$02           ; 1774
    subw ya,!spc_sfx_tmp              ; 1777
    mov.w !spc_ramp_a_pitch,a         ; 1779
    mov.w !spc_ramp_a_pitch_hi,y      ; 177C
    ret                               ; 177F
    bbs !spc_flags.4,.l17AE           ; 1780
    set1 !spc_flags.4                 ; 1783
    clr1 !spc_flags.5                 ; 1785
    mov a,#$00                        ; 1787
    mov y,#$02                        ; 1789
    mov.w !spcb_ramp_a2_pitch,a       ; 178B
    mov.w !spcb_ramp_a2_pitch_hi,y    ; 178E
    ret                               ; 1791
    bbc !spc_flags.4,.l17AE           ; 1792
    clr1 !spc_flags.4                 ; 1795
    set1 !spc_flags.5                 ; 1797
    mov.w a,!spcb_ramp_a2_pitch       ; 1799
    mov.w y,!spcb_ramp_a2_pitch_hi    ; 179C
    mov !spc_sfx_tmp,#$00             ; 179F
    mov !spc_sfx_tmp+1,#$02           ; 17A2
    subw ya,!spc_sfx_tmp              ; 17A5
    mov.w !spcb_ramp_a2_pitch,a       ; 17A7
    mov.w !spcb_ramp_a2_pitch_hi,y    ; 17AA
    ret                               ; 17AD
.l17AE:
    ret                               ; 17AE
    bbs !spc_effect_flags.2,.l17AE    ; 17AF
    set1 !spc_effect_flags.2          ; 17B2
    clr1 !spc_effect_flags.3          ; 17B4
    mov a,#$40                        ; 17B6
    mov y,#$00                        ; 17B8
    mov.w !spc_ramp_b_pitch,a         ; 17BA
    mov.w !spc_ramp_b_pitch_hi,y      ; 17BD
    ret                               ; 17C0
    bbs !spc_effect_flags.4,.l17AE    ; 17C1
    set1 !spc_effect_flags.4          ; 17C4
    clr1 !spc_effect_flags.5          ; 17C6
    mov a,#$40                        ; 17C8
    mov y,#$00                        ; 17CA
    mov.w !spcb_ramp_b2_pitch,a       ; 17CC
    mov.w !spcb_ramp_b2_pitch_hi,y    ; 17CF
    ret                               ; 17D2
    bbc !spc_effect_flags.2,.l17AE    ; 17D3
    clr1 !spc_effect_flags.2          ; 17D6
    set1 !spc_effect_flags.3          ; 17D8
    mov.w a,!spc_ramp_b_pitch         ; 17DA
    mov.w y,!spc_ramp_b_pitch_hi      ; 17DD
    mov !spc_sfx_tmp,#$40             ; 17E0
    mov !spc_sfx_tmp+1,#$00           ; 17E3
    subw ya,!spc_sfx_tmp              ; 17E6
    mov.w !spc_ramp_b_pitch,a         ; 17E8
    mov.w !spc_ramp_b_pitch_hi,y      ; 17EB
    ret                               ; 17EE
    bbc !spc_effect_flags.4,.l17AE    ; 17EF
    clr1 !spc_effect_flags.4          ; 17F2
    set1 !spc_effect_flags.5          ; 17F4
    mov.w a,!spcb_ramp_b2_pitch       ; 17F6
    mov.w y,!spcb_ramp_b2_pitch_hi    ; 17F9
    mov !spc_sfx_tmp,#$40             ; 17FC
    mov !spc_sfx_tmp+1,#$00           ; 17FF
    subw ya,!spc_sfx_tmp              ; 1802
    mov.w !spcb_ramp_b2_pitch,a       ; 1804
    mov.w !spcb_ramp_b2_pitch_hi,y    ; 1807
    ret                               ; 180A
    mov.b a,!spc_sfx_tmp              ; 180B
    setc                              ; 180D
    sbc a,#$32                        ; 180E
    mov.w !spc_sfx_volume_add,a       ; 1810
    ret                               ; 1813
    mov.b a,!spc_sfx_tmp              ; 1814
    setc                              ; 1816
    sbc a,#$36                        ; 1817
    mov.w !spcb_sfx_volume_add_4,a    ; 1819
    ret                               ; 181C
.l181D:
    clr1 !spc_loop_flags.2            ; 181D
    mov.w a,!spc_sfx_command+5        ; 181F
    cmp a,#$25                        ; 1822
    bne .l186B                        ; 1824
    bra .l183C                        ; 1826
.l1828:
    clr1 !spc_loop_flags.4            ; 1828
    mov.w a,!spc_sfx_command+5        ; 182A
    cmp a,#$15                        ; 182D
    bne .l186B                        ; 182F
    bra .l183C                        ; 1831
.l1833:
    clr1 !spc_loop_flags.0            ; 1833
    mov.w a,!spc_sfx_command+5        ; 1835
    cmp a,#$17                        ; 1838
    bne .l186B                        ; 183A
.l183C:
    mov x,#$05                        ; 183C
    bra .l1861                        ; 183E
.l1840:
    clr1 !spc_loop_flags.3            ; 1840
    mov.w a,!spc_sfx_command+5        ; 1842
    cmp a,#$25                        ; 1845
    bne .l186B                        ; 1847
    bra .l185F                        ; 1849
.l184B:
    clr1 !spc_loop_flags.5            ; 184B
    mov.w a,!spc_sfx_command+5        ; 184D
    cmp a,#$15                        ; 1850
    bne .l186B                        ; 1852
    bra .l185F                        ; 1854
.l1856:
    clr1 !spc_loop_flags.1            ; 1856
    mov.w a,!spc_sfx_command+5        ; 1858
    cmp a,#$17                        ; 185B
    bne .l186B                        ; 185D
.l185F:
    mov x,#$05                        ; 185F
.l1861:
    mov.w a,!spc_sfx_active+x         ; 1861
    beq .l186B                        ; 1864
    mov.b a,!spc_sfx_tmp              ; 1866
    mov.w !spc_sfx_stop_request+x,a   ; 1868
.l186B:
    ret                               ; 186B

; @routine 1A:F851 SpcBSfxStop
; WHAT IT DOES: Requests the stop of channel X (reason 1, $03E0+X) when it is active ($0408+X), as the race driver's SpcSfxStop. After it, two port 1 handlers: $22 (while stopped, bit 0 of $29: queues sound $4C, clears the stop, the saved sounds $045C-$045F and $04FC-$04FF and the ramp flags, pauses the music and ends both first ramps) and $16 (while stopped: $2A back from $2B, queues $4B, clears the stop and bit 6 of $2D, resumes channels 7-4 and stops channels 1 and 0).
; HOW: $20 = 1, then the request in SpcBPort1Command at $1861; the handlers are CALL chains.
; Entry: X = channel (0-7). Exit: A changed.
; @spc 186C
%spc($186C, $1AF851)
SpcBSfxStop:
    mov !spc_sfx_tmp,#$01             ; 186C
    bra SpcBPort1Command_l1861        ; 186F
    bbc !spc_flags.0,SpcBPort1Command_l186B; 1871  ; not reached in the traces so far
    mov a,#$4C                        ; 1874  ; not reached in the traces so far
    call SpcBSoundQueuePush           ; 1876  ; not reached in the traces so far
    clr1 !spc_flags.0                 ; 1879  ; not reached in the traces so far
    mov a,#$00                        ; 187B  ; not reached in the traces so far
    mov.w !spc_sfx_paused+7,a         ; 187D  ; not reached in the traces so far
    mov.w !spc_sfx_paused+6,a         ; 1880  ; not reached in the traces so far
    mov.w !spc_sfx_paused+5,a         ; 1883  ; not reached in the traces so far
    mov.w !spc_sfx_paused+4,a         ; 1886  ; not reached in the traces so far
    mov.w !spc_sfx_loop_sound+7,a     ; 1889  ; not reached in the traces so far
    mov.w !spc_sfx_loop_sound+6,a     ; 188C  ; not reached in the traces so far
    mov.w !spc_sfx_loop_sound+5,a     ; 188F  ; not reached in the traces so far
    mov.w !spc_sfx_loop_sound+4,a     ; 1892  ; not reached in the traces so far
    clr1 !spc_flags.2                 ; 1895  ; not reached in the traces so far
    clr1 !spc_flags.3                 ; 1897  ; not reached in the traces so far
    clr1 !spc_flags.4                 ; 1899  ; not reached in the traces so far
    clr1 !spc_flags.5                 ; 189B  ; not reached in the traces so far
    call SpcMusicPause                ; 189D  ; not reached in the traces so far
    call SpcBPitchRampA1End           ; 18A0  ; not reached in the traces so far
    jmp SpcBPitchRampA2Down_l162A     ; 18A3  ; not reached in the traces so far
    bbc !spc_flags.0,SpcBPort1Command_l186B; 18A6
    mov !spc_loop_flags,!spc_loop_flags_saved; 18A9
    mov a,#$4B                        ; 18AC
    call SpcBSoundQueuePush           ; 18AE
    clr1 !spc_flags.0                 ; 18B1
    clr1 !spc_game_flags.6            ; 18B3
    mov x,#$07                        ; 18B5
    call SpcBSfxChannelResume         ; 18B7
    mov x,#$06                        ; 18BA
    call SpcBSfxChannelResume         ; 18BC
    mov x,#$05                        ; 18BF
    call SpcBSfxChannelResume         ; 18C1
    mov x,#$04                        ; 18C4
    call SpcBSfxChannelResume         ; 18C6
    mov x,#$01                        ; 18C9
    call SpcBSfxStop                  ; 18CB
    mov x,#$00                        ; 18CE
    jmp SpcBSfxStop                   ; 18D0

; @routine 1A:F8B8 SpcBSfxChannelResume
; WHAT IT DOES: Channel X after a pause: by its saved mode $0450 (1 or 2, else nothing) bit 7 of $00 is cleared or set, the mode cleared, its sound number $03C8 back from $0460, and a sound saved at $0458 restarted (SpcBSfxPointer, SpcBSfxStart); without one the channel stops. The race driver's SpcSfxChannelResume has no mode.
; HOW: Table copies and CALLs; the stop is SpcBSfxStop through the JMP at $1914.
; Entry: X = channel. Exit: X kept.
; @spc 18D3
%spc($18D3, $1AF8B8)
SpcBSfxChannelResume:
    mov.w a,!spcb_sfx_paused_side+x   ; 18D3
    cmp a,#$01                        ; 18D6
    beq .l18DF                        ; 18D8
    cmp a,#$02                        ; 18DA
    beq .l18E3                        ; 18DC
    ret                               ; 18DE
.l18DF:
    clr1 !spc_port_in.7               ; 18DF
    bra .l18E5                        ; 18E1
.l18E3:
    set1 !spc_port_in.7               ; 18E3
.l18E5:
    mov a,#$00                        ; 18E5
    mov.w !spcb_sfx_paused_side+x,a   ; 18E7
    mov.w a,!spc_sfx_paused_number+x  ; 18EA
    mov.w !spc_sfx_number+x,a         ; 18ED
    mov.w a,!spc_sfx_paused+x         ; 18F0
    beq SpcBSfxPointer_l1914          ; 18F3
    push a                            ; 18F5
    call SpcBSfxPointer               ; 18F6
    mov a,#$00                        ; 18F9
    mov.w !spc_sfx_paused+x,a         ; 18FB
    pop a                             ; 18FE
    push x                            ; 18FF
    call SpcBSfxStart                 ; 1900
    pop x                             ; 1903
    ret                               ; 1904

; @routine 1A:F8EA SpcBSfxPointer
; WHAT IT DOES: The data pointer of sound A from the table at $0702 into $24/$25 (the race driver's SpcSfxPointer). After it, the JMP to SpcBSfxStop that SpcBSfxChannelResume takes, and the port 1 handler $15, the pause: unless already stopped, $2A is saved at $2B and cleared, the stop (bit 0 of $29) and bit 6 of $2D set, sound $4A queued, channels 7-4 saved (SpcBSfxChannelPause) and stopped. When already stopped it branches into SpcBSfxChannelPause's save at $1956 with X = $28, the dispatch index (the race driver's branches to a RET).
; HOW: (A - 1) * 2 as the index; the handler is a CALL chain.
; Entry: A = sound (1 and up). Exit: Y = index + 1.
; @spc 1905
%spc($1905, $1AF8EA)
SpcBSfxPointer:
    dec a                             ; 1905
    asl a                             ; 1906
    mov y,a                           ; 1907
    mov.w a,SpcReload_t0702+y         ; 1908
    mov.b !spc_sfx_data,a             ; 190B
    inc y                             ; 190D
    mov.w a,SpcReload_t0702+y         ; 190E
    mov.b !spc_sfx_data+1,a           ; 1911
    ret                               ; 1913
.l1914:
    jmp SpcBSfxStop                   ; 1914  ; not reached in the traces so far
    bbs !spc_flags.0,SpcBSfxChannelPause_l1956; 1917
    mov !spc_loop_flags_saved,!spc_loop_flags; 191A
    mov !spc_loop_flags,#$00          ; 191D
    set1 !spc_flags.0                 ; 1920
    set1 !spc_game_flags.6            ; 1922
    mov a,#$4A                        ; 1924
    call SpcBSoundQueuePush           ; 1926
    mov x,#$07                        ; 1929
    call SpcBSfxChannelPause          ; 192B
    mov x,#$06                        ; 192E
    call SpcBSfxChannelPause          ; 1930
    mov x,#$05                        ; 1933
    call SpcBSfxChannelPause          ; 1935
    mov x,#$04                        ; 1938
    call SpcBSfxChannelPause          ; 193A
    mov x,#$07                        ; 193D
    call SpcBSfxStop                  ; 193F
    mov x,#$06                        ; 1942
    call SpcBSfxStop                  ; 1944
    mov x,#$05                        ; 1947
    call SpcBSfxStop                  ; 1949
    mov x,#$04                        ; 194C
    jmp SpcBSfxStop                   ; 194E

; @routine 1A:F936 SpcBSfxChannelPause
; WHAT IT DOES: Channel X at a pause: a pending queued sound ($04F8) or a running looped one ($0480, mode 1) is saved at $0458 to restart after, with its mode ($0528 or $04B8) at $0450 and, for a looped one, its sound number at $0460. The race driver's SpcSfxChannelPause has no mode.
; HOW: Compares on $04F8, $0480.
; Entry: X = channel. Exit: A changed.
; @spc 1951
%spc($1951, $1AF936)
SpcBSfxChannelPause:
    mov.w a,!spc_sfx_loop_sound+x     ; 1951
    beq .l196E                        ; 1954
.l1956:
    mov.w a,!spc_sfx_loop_sound+x     ; 1956  ; not reached in the traces so far
    mov.w !spc_sfx_paused+x,a         ; 1959  ; not reached in the traces so far
    mov.w a,!spcb_sfx_loop_side+x     ; 195C  ; not reached in the traces so far
    mov.w !spcb_sfx_paused_side+x,a   ; 195F  ; not reached in the traces so far
    mov a,#$00                        ; 1962  ; not reached in the traces so far
    mov.w !spc_sfx_loop_sound+x,a     ; 1964  ; not reached in the traces so far
    mov.w !spc_sfx_loop+x,a           ; 1967  ; not reached in the traces so far
    mov.w !spcb_sfx_loop_side+x,a     ; 196A  ; not reached in the traces so far
    ret                               ; 196D  ; not reached in the traces so far
.l196E:
    mov.w a,!spc_sfx_loop+x           ; 196E
    beq .l198E                        ; 1971
    cmp a,#$02                        ; 1973
    beq .l1956                        ; 1975
    mov.w a,!spcb_sfx_side+x          ; 1977
    mov.w !spcb_sfx_paused_side+x,a   ; 197A
    mov.w a,!spc_sfx_number+x         ; 197D
    mov.w !spc_sfx_paused_number+x,a  ; 1980
    mov.w a,!spc_sfx_command+x        ; 1983
    mov.w !spc_sfx_paused+x,a         ; 1986
    mov a,#$00                        ; 1989
    mov.w !spc_sfx_loop+x,a           ; 198B
.l198E:
    ret                               ; 198E

; @routine 1A:F974 SpcBSfxStopAll
; WHAT IT DOES: Stops every sound effect channel with a fade, $2A cleared, and the music fade-out started (SpcBFadeOutStart). The race driver's SpcSfxStopAll.
; HOW: SpcBSfxStopRequest for X = 7 down to 0.
; Entry: none. Exit: X = $FF.
; @spc 198F
%spc($198F, $1AF974)
SpcBSfxStopAll:
    mov !spc_loop_flags,#$00          ; 198F
    call SpcBFadeOutStart             ; 1992
    mov x,#$07                        ; 1995
.l1997:
    call SpcBSfxStopRequest           ; 1997
    dec x                             ; 199A
    bpl .l1997                        ; 199B
    ret                               ; 199D

; @routine 1A:F983 SpcBSfxStopRequest
; WHAT IT DOES: Channel X's queued and looping state cleared ($04F8, $0480, $2A) and, if active, its fade-out started ($04C0, $04C8 = 1). The race driver's SpcSfxStopRequest.
; HOW: Stores.
; Entry: X = channel. Exit: A = 0 or 1.
; @spc 199E
%spc($199E, $1AF983)
SpcBSfxStopRequest:
    mov a,#$00                        ; 199E
    mov.w !spc_sfx_loop_sound+x,a     ; 19A0
    mov.w !spc_sfx_loop+x,a           ; 19A3
    mov.b !spc_loop_flags,a           ; 19A6
    mov.w a,!spc_sfx_active+x         ; 19A8
    beq .l19B5                        ; 19AB
    mov a,#$01                        ; 19AD
    mov.w !spc_sfx_fade_count+x,a     ; 19AF
    mov.w !spc_sfx_fading+x,a         ; 19B2
.l19B5:
    ret                               ; 19B5

; @routine 1A:F99B SpcBFadeOutStart
; WHAT IT DOES: Unless stopped (bit 0 of $29), starts the music fade-out (bit 6 of $29). The race driver's SpcFadeOutStart.
; HOW: SET1.
; Entry: none. Exit: none.
; @spc 19B6
%spc($19B6, $1AF99B)
SpcBFadeOutStart:
    bbs !spc_flags.0,.l19BB           ; 19B6
    set1 !spc_flags.6                 ; 19B9
.l19BB:
    ret                               ; 19BB

; @routine 1A:F9A1 SpcBMusicFade
; WHAT IT DOES: The music fade, as the race driver's SpcMusicFade: while bit 6 of $29 is set, the fade amount $2E/$2F grows by $2F0 a tick and the volume scale $0565 is the start level $0564 less it; at the bottom the fade ends and, by flags, the driver reloads (bit 7 of $2C: all keys off, SpcReload), returns to the IPL ROM (bit 0 of $2D) or pauses the music. Without a fade, the scale follows the master volume.
; HOW: SUBW from $0564 in the high byte; the no-fade path and the IPL ROM exit are in SpcBKeyOffAll.
; Entry: none. Exit: as the chosen path leaves it.
; @spc 19BC
%spc($19BC, $1AF9A1)
SpcBMusicFade:
    bbc !spc_flags.6,SpcBKeyOffAll_l1A02; 19BC
    mov a,#$F0                        ; 19BF
    mov y,#$02                        ; 19C1
    addw ya,!spc_fade_amount          ; 19C3
    mov.b !spc_fade_amount,a          ; 19C5
    mov.b !spc_fade_amount+1,y        ; 19C7
    mov a,#$00                        ; 19C9
    mov.w y,!spc_fade_start           ; 19CB
    subw ya,!spc_fade_amount          ; 19CE
    mov.w !spc_volume_scale,y         ; 19D0
    cmp y,#$00                        ; 19D3
    beq .l19E4                        ; 19D5
    cmp y,#$01                        ; 19D7
    beq .l19E4                        ; 19D9
    cmp y,#$02                        ; 19DB
    beq .l19E4                        ; 19DD
    cmp y,#$03                        ; 19DF
    beq .l19E4                        ; 19E1
    ret                               ; 19E3
.l19E4:
    mov a,#$00                        ; 19E4
    mov.b !spc_fade_amount,a          ; 19E6
    mov.b !spc_fade_amount+1,a        ; 19E8
    clr1 !spc_flags.6                 ; 19EA
    bbs !spc_track_flags.7,.l19F5     ; 19EC
    bbs !spc_game_flags.0,SpcBKeyOffAll_l1A21; 19EF  ; not reached in the traces so far
    jmp SpcMusicPause                 ; 19F2  ; not reached in the traces so far
.l19F5:
    call SpcBKeyOffAll                ; 19F5
    jmp SpcReload                     ; 19F8

; @routine 1A:F9E0 SpcBKeyOffAll
; WHAT IT DOES: Keys off all eight voices. After it, SpcBMusicFade's no-fade path (the scale and start level from the master volume) and two port 1 handlers: 5 stops every channel (SpcBSfxStopAll) and reloads the driver now when the master volume is 0, else at the fade's end (bit 7 of $2C); 6 does the same with a return to the IPL ROM (bit 0 of $2D; the IPL ROM switched in through $F1 and a JMP to $FFC0).
; HOW: DSP KOF ($5C) = $FF through SpcDspWrite.
; Entry: none. Exit: A = $FF, Y = $5C.
; @spc 19FB
%spc($19FB, $1AF9E0)
SpcBKeyOffAll:
    mov a,#$FF                        ; 19FB
    mov y,#$5C                        ; 19FD
    jmp SpcDspWrite                   ; 19FF
.l1A02:
    mov.b a,!spc_master_volume+1      ; 1A02
    mov.w !spc_fade_start,a           ; 1A04
    mov.w !spc_volume_scale,a         ; 1A07
    ret                               ; 1A0A
    call SpcBSfxStopAll               ; 1A0B
    mov.b a,!spc_master_volume+1      ; 1A0E
    beq SpcBMusicFade_l19F5           ; 1A10
    set1 !spc_track_flags.7           ; 1A12
    ret                               ; 1A14
    set1 !spc_game_flags.0            ; 1A15  ; not reached in the traces so far
    call SpcBSfxStopAll               ; 1A17  ; not reached in the traces so far
    mov.b a,!spc_master_volume+1      ; 1A1A  ; not reached in the traces so far
    beq .l1A21                        ; 1A1C  ; not reached in the traces so far
    set1 !spc_game_flags.0            ; 1A1E  ; not reached in the traces so far
    ret                               ; 1A20  ; not reached in the traces so far
.l1A21:
    call SpcBKeyOffAll                ; 1A21  ; not reached in the traces so far
    mov.w a,!spc_control              ; 1A24  ; not reached in the traces so far
    or a,#$80                         ; 1A27  ; not reached in the traces so far
    mov.w !spc_control,a              ; 1A29  ; not reached in the traces so far
    jmp !spc_iplrom                   ; 1A2C  ; not reached in the traces so far

; @routine 1A:FA14 SpcBCommandChannelCheck
; WHAT IT DOES: A sound command $1D outside the list at $25FF is checked against the list at $2608: when an earlier-listed sound already plays on its channel, the command is dropped; the command kept is also stored at $1F (as the race driver's SpcCommandChannelCheck, with the battle's lists). After it, the sound effect queue start that SpcBSfxFrame jumps to ($1A7F): $057B counts the ticks without a command; after three, the queue's head (SpcBSoundQueuePush) is taken, its bit 7 moved to bit 7 of $00, else a command differing from the last ($1E) is taken; the sound ($23) gets its channel, mode and side (SpcBSfxLookup, SpcBSfxChannel, SpcBSfxMode, SpcBSfxSide), is activated and its pointer saved, then the channels run (SpcBSfxChannelsFrame) with $0410 holding the new sound. The race driver queues every command first.
; HOW: List scans; the queue is shifted down one byte.
; Entry: none. Exit: X = channel, or as SpcBSfxChannelsFrame leaves it.
; @spc 1A2F
%spc($1A2F, $1AFA14)
SpcBCommandChannelCheck:
    mov.b a,!spc_command              ; 1A2F
    beq .l1A79                        ; 1A31
    mov y,#$00                        ; 1A33
.l1A35:
    mov.w a,SpcBNoiseSample_t25FF+y   ; 1A35
    beq .l1A41                        ; 1A38
    cmp.b a,!spc_command              ; 1A3A
    beq .l1A7C                        ; 1A3C
    inc y                             ; 1A3E
    bra .l1A35                        ; 1A3F
.l1A41:
    mov y,#$00                        ; 1A41
.l1A43:
    mov.w a,SpcBNoiseSample_t2608+y   ; 1A43
    beq .l1A7C                        ; 1A46
    push a                            ; 1A48
    mov.b a,!spc_command              ; 1A49
    push y                            ; 1A4B
    call SpcBSfxLookup                ; 1A4C
    call SpcBSfxChannel               ; 1A4F
    pop y                             ; 1A52
    pop a                             ; 1A53
    cmp.w a,!spc_sfx_command+x        ; 1A54
    beq .l1A5C                        ; 1A57
.l1A59:
    inc y                             ; 1A59
    bra .l1A43                        ; 1A5A
.l1A5C:
    mov x,#$00                        ; 1A5C
.l1A5E:
    mov.w a,SpcBNoiseSample_t2608+x   ; 1A5E
    beq .l1A72                        ; 1A61
    cmp.b a,!spc_command              ; 1A63
    beq .l1A6A                        ; 1A65
    inc x                             ; 1A67
    bra .l1A5E                        ; 1A68
.l1A6A:
    mov.b !spc_sfx_tmp,x              ; 1A6A
    mov a,y                           ; 1A6C
    setc                              ; 1A6D
    sbc.b a,!spc_sfx_tmp              ; 1A6E
    bpl .l1A59                        ; 1A70
.l1A72:
    mov !spc_command_kept,!spc_command; 1A72
    mov !spc_command,#$00             ; 1A75
    ret                               ; 1A78
.l1A79:
    mov !spc_command_kept,#$00        ; 1A79
.l1A7C:
    ret                               ; 1A7C
    db $15, $1E
.l1A7F:
    cmp !spc_command,#$00             ; 1A7F
    beq .l1A8B                        ; 1A82
    mov a,#$00                        ; 1A84
    mov.w !spc_hold_index,a           ; 1A86
    bra .l1A95                        ; 1A89
.l1A8B:
    mov.w a,!spc_hold_index           ; 1A8B
    cmp a,#$FF                        ; 1A8E
    beq .l1A95                        ; 1A90
    inc.w !spc_hold_index             ; 1A92
.l1A95:
    cmp !spc_command,#$00             ; 1A95
    bne .l1ACA                        ; 1A98
    mov.w a,!spc_hold_index           ; 1A9A
    beq .l1ACA                        ; 1A9D
    cmp a,#$01                        ; 1A9F
    beq .l1ACA                        ; 1AA1
    cmp a,#$02                        ; 1AA3
    beq .l1ACA                        ; 1AA5
    mov.w a,!spc_sound_queue          ; 1AA7
    beq .l1ACA                        ; 1AAA
    push a                            ; 1AAC
    mov x,#$01                        ; 1AAD
.l1AAF:
    mov.w a,!spc_sound_queue+x        ; 1AAF
    dec x                             ; 1AB2
    mov.w !spc_sound_queue+x,a        ; 1AB3
    inc x                             ; 1AB6
    inc x                             ; 1AB7
    cmp x,#$08                        ; 1AB8
    bne .l1AAF                        ; 1ABA
    pop a                             ; 1ABC
    asl a                             ; 1ABD
    bcc .l1AC2                        ; 1ABE
    set1 !spc_port_in.7               ; 1AC0
.l1AC2:
    lsr a                             ; 1AC2
    mov x,#$00                        ; 1AC3
    mov.w !spc_hold_index,x           ; 1AC5
    bra .l1AD9                        ; 1AC8
.l1ACA:
    mov.b a,!spc_command              ; 1ACA
    cmp.b a,!spc_hold_last            ; 1ACC
    bne .l1AD3                        ; 1ACE
.l1AD0:
    jmp SpcBSfxChannelsFrame          ; 1AD0
.l1AD3:
    mov.b !spc_hold_last,a            ; 1AD3
    mov.b a,!spc_command              ; 1AD5
    beq .l1AD0                        ; 1AD7
.l1AD9:
    mov.b !spc_sfx_work+1,a           ; 1AD9
    call SpcBSfxLookup                ; 1ADB
    call SpcBSfxChannel               ; 1ADE
    call SpcBSfxMode                  ; 1AE1
    call SpcBSfxSide                  ; 1AE4
    call SpcBSfxActivate              ; 1AE7
    call SpcBSfxChannelSave           ; 1AEA
    mov.b a,!spc_sfx_work+1           ; 1AED
    mov.w !spc_sfx_new+x,a            ; 1AEF
    push x                            ; 1AF2
    call SpcBSfxChannelsFrame         ; 1AF3
    pop x                             ; 1AF6
    mov a,#$00                        ; 1AF7
    mov.w !spc_sfx_new+x,a            ; 1AF9
    ret                               ; 1AFC

; @routine 1A:FAE2 SpcBSfxActivate
; WHAT IT DOES: Marks channel X active ($0408 = 1). The race driver's SpcSfxActivate.
; HOW: Store.
; Entry: X = channel. Exit: A = 1.
; @spc 1AFD
%spc($1AFD, $1AFAE2)
SpcBSfxActivate:
    mov a,#$01                        ; 1AFD
    mov.w !spc_sfx_active+x,a         ; 1AFF
    ret                               ; 1B02

; @routine 1A:FAE8 SpcBSfxLookup
; WHAT IT DOES: The data of sound A: its pointer from $0702 into $20/$21 and its first byte (the channel and mode) in X. The race driver's SpcSfxLookup.
; HOW: (A - 1) * 2 as the index; MOV A,($20)+Y.
; Entry: A = sound. Exit: X = header byte, Y = 0.
; @spc 1B03
%spc($1B03, $1AFAE8)
SpcBSfxLookup:
    dec a                             ; 1B03
    asl a                             ; 1B04
    mov y,a                           ; 1B05
    mov.w a,SpcReload_t0702+y         ; 1B06
    mov.b !spc_sfx_tmp,a              ; 1B09
    inc y                             ; 1B0B
    mov.w a,SpcReload_t0702+y         ; 1B0C
    mov.b !spc_sfx_tmp+1,a            ; 1B0F
    mov y,#$00                        ; 1B11
    mov a,(!spc_sfx_tmp)+y            ; 1B13
    mov x,a                           ; 1B15
    ret                               ; 1B16

; @routine 1A:FAFC SpcBSfxChannel
; WHAT IT DOES: Keeps the header byte X at $26 and leaves its channel (low four bits) in X, one lower when the header's bit 6 and bit 7 of $00 are set; sounds $77-$7A ($23) set the channel's sound number $03C8 to $1C, or $20 with bit 7 of $00. The race driver's SpcSfxChannel only takes the channel.
; HOW: AND #$0F; BBC tests; a compare chain on $23.
; Entry: X = header byte, $23 = sound. Exit: X = channel.
; @spc 1B17
%spc($1B17, $1AFAFC)
SpcBSfxChannel:
    mov.b !spc_math_tmp,x             ; 1B17
    mov a,x                           ; 1B19
    and a,#$0F                        ; 1B1A
    mov x,a                           ; 1B1C
    bbc !spc_math_tmp.6,.l1B24        ; 1B1D
    bbc !spc_port_in.7,.l1B24         ; 1B20
    dec x                             ; 1B23
.l1B24:
    cmp !spc_sfx_work+1,#$79          ; 1B24
    beq .l1B39                        ; 1B27
    cmp !spc_sfx_work+1,#$78          ; 1B29
    beq .l1B39                        ; 1B2C
    cmp !spc_sfx_work+1,#$77          ; 1B2E
    beq .l1B39                        ; 1B31
    cmp !spc_sfx_work+1,#$7A          ; 1B33
    beq .l1B39                        ; 1B36
    ret                               ; 1B38
.l1B39:
    bbs !spc_port_in.7,.l1B42         ; 1B39
    mov a,#$1C                        ; 1B3C
    mov.w !spc_sfx_number+x,a         ; 1B3E
    ret                               ; 1B41
.l1B42:
    mov a,#$20                        ; 1B42
    mov.w !spc_sfx_number+x,a         ; 1B44
    ret                               ; 1B47

; @routine 1A:FB2D SpcBSfxMode
; WHAT IT DOES: The header's mode bits for channel X: bit 7 marks a looping sound ($0480 = 1, or 2 with bit 5); otherwise a looping channel saves its sound to restart ($04F8) with its side ($04B8 to $0528). The race driver's SpcSfxMode without the side.
; HOW: BBC tests on $26.
; Entry: X = channel, $26 = header. Exit: A changed.
; @spc 1B48
%spc($1B48, $1AFB2D)
SpcBSfxMode:
    bbc !spc_math_tmp.7,.l1B58        ; 1B48
    bbc !spc_math_tmp.5,.l1B52        ; 1B4B
    mov a,#$02                        ; 1B4E
    bra .l1B54                        ; 1B50
.l1B52:
    mov a,#$01                        ; 1B52
.l1B54:
    mov.w !spc_sfx_loop+x,a           ; 1B54
    ret                               ; 1B57
.l1B58:
    mov.w a,!spc_sfx_loop+x           ; 1B58
    cmp a,#$01                        ; 1B5B
    bne .l1B70                        ; 1B5D
    mov.w a,!spc_sfx_loop_sound+x     ; 1B5F
    bne .l1B70                        ; 1B62
    mov.w a,!spc_sfx_command+x        ; 1B64
    mov.w !spc_sfx_loop_sound+x,a     ; 1B67
    mov.w a,!spcb_sfx_side+x          ; 1B6A
    mov.w !spcb_sfx_loop_side+x,a     ; 1B6D
.l1B70:
    ret                               ; 1B70

; @routine 1A:FB56 SpcBSfxSide
; WHAT IT DOES: Channel X's side $04B8: 1, or 2 with bit 7 of $00.
; HOW: BBS and a store.
; Entry: X = channel. Exit: A = the side.
; @spc 1B71
%spc($1B71, $1AFB56)
SpcBSfxSide:
    bbs !spc_port_in.7,.l1B78         ; 1B71
    mov a,#$01                        ; 1B74
    bra .l1B7A                        ; 1B76
.l1B78:
    mov a,#$02                        ; 1B78
.l1B7A:
    mov.w !spcb_sfx_side+x,a          ; 1B7A
    ret                               ; 1B7D

; @routine 1A:FB63 SpcBSfxChannelsFrame
; WHAT IT DOES: Runs every active sound effect channel's tick (SpcBSfxChannelLoad, SpcBSfxChannelStep). The race driver's SpcSfxChannelsFrame.
; HOW: X from 0 to 7.
; Entry: none. Exit: X = 8.
; @spc 1B7E
%spc($1B7E, $1AFB63)
SpcBSfxChannelsFrame:
    mov x,#$00                        ; 1B7E
.l1B80:
    mov.w a,!spc_sfx_active+x         ; 1B80
    beq .l1B8B                        ; 1B83
    call SpcBSfxChannelLoad           ; 1B85
    call SpcBSfxChannelStep           ; 1B88
.l1B8B:
    inc x                             ; 1B8B
    cmp x,#$08                        ; 1B8C
    beq .l1B92                        ; 1B8E
    bra .l1B80                        ; 1B90
.l1B92:
    ret                               ; 1B92

; @routine 1A:FB78 SpcBSfxChannelLoad
; WHAT IT DOES: Channel X's data pointer ($0488/$0490) into $20/$21. The race driver's SpcSfxChannelLoad.
; HOW: Two copies.
; Entry: X = channel. Exit: A = the pointer's high byte.
; @spc 1B93
%spc($1B93, $1AFB78)
SpcBSfxChannelLoad:
    mov.w a,!spc_sfx_ptr+x            ; 1B93
    mov.b !spc_sfx_tmp,a              ; 1B96
    mov.w a,!spc_sfx_ptr_hi+x         ; 1B98
    mov.b !spc_sfx_tmp+1,a            ; 1B9B
    ret                               ; 1B9D

; @routine 1A:FB83 SpcBSfxChannelSave
; WHAT IT DOES: $20/$21 back as channel X's data pointer. The race driver's SpcSfxChannelSave.
; HOW: Two copies.
; Entry: X = channel. Exit: A = the high byte.
; @spc 1B9E
%spc($1B9E, $1AFB83)
SpcBSfxChannelSave:
    mov.b a,!spc_sfx_tmp              ; 1B9E
    mov.w !spc_sfx_ptr+x,a            ; 1BA0
    mov.b a,!spc_sfx_tmp+1            ; 1BA3
    mov.w !spc_sfx_ptr_hi+x,a         ; 1BA5
    ret                               ; 1BA8

; @routine 1A:FB8E SpcBSfxChannelStep
; WHAT IT DOES: Channel X's state ($03F8: 0 start, 1 started, 2 and 3 playing, 4 stopping), as the race driver's SpcSfxChannelStep: a new sound ($0410) starts with its voice taken from the music and keyed off; playing, it is keyed on, read (SpcBSfxRead), its special handler run (SpcBSfxSpecial) and its DSP registers written; stop requests ($03E0) release the voice (SpcBSfxRelease) and clear the channel: reason 1 queues a saved sound again with bit 7 set for side 1 and clear for side 2 ($0528), reasons $0A/$1A, $0B/$1B and $0C/$1C end the loops $25, $15 and $17. The battle's clear also resets the side ($04B8, $0528).
; HOW: Compare chains on the state and the request; tail jumps into the shared code. The 13 bytes at $1C60 are code no path reaches.
; Entry: X = channel, $20/$21 its pointer. Exit: A, Y changed.
; @spc 1BA9
%spc($1BA9, $1AFB8E)
SpcBSfxChannelStep:
    mov.w a,!spc_sfx_state+x          ; 1BA9
    beq .l1C01                        ; 1BAC
    cmp a,#$01                        ; 1BAE
    beq .l1BCD                        ; 1BB0
    cmp a,#$02                        ; 1BB2
    beq .l1BE2                        ; 1BB4
    cmp a,#$03                        ; 1BB6
    beq .l1BE7                        ; 1BB8
    mov.w a,!spc_sfx_new+x            ; 1BBA
    bne .l1C04                        ; 1BBD
    mov a,#$00                        ; 1BBF
    mov.w !spc_sfx_command+x,a        ; 1BC1
    mov.w !spc_sfx_state+x,a          ; 1BC4
    mov.w !spc_sfx_active+x,a         ; 1BC7
    jmp SpcBVoiceMaskNot_l1CEE        ; 1BCA
.l1BCD:
    mov.w a,!spc_sfx_new+x            ; 1BCD
    bne .l1C04                        ; 1BD0
    mov a,#$02                        ; 1BD2
    mov.w !spc_sfx_state+x,a          ; 1BD4
    call SpcBKeyOnVoice               ; 1BD7
    mov a,#$00                        ; 1BDA
    mov.w !spc_sfx_key_request+x,a    ; 1BDC
.l1BDF:
    jmp SpcBKeyOffVoice_l1D5A         ; 1BDF
.l1BE2:
    mov a,#$00                        ; 1BE2
    mov.w !spc_sfx_key_request+x,a    ; 1BE4
.l1BE7:
    mov.w a,!spc_sfx_new+x            ; 1BE7
    bne .l1C04                        ; 1BEA
    mov a,#$03                        ; 1BEC
    mov.w !spc_sfx_state+x,a          ; 1BEE
    mov.w a,!spc_sfx_key_request+x    ; 1BF1
    beq .l1C1D                        ; 1BF4
    cmp a,#$01                        ; 1BF6
    beq .l1BCD                        ; 1BF8
    mov a,#$00                        ; 1BFA
    mov.w !spc_sfx_age+x,a            ; 1BFC
    bra .l1BCD                        ; 1BFF
.l1C01:
    mov.w a,!spc_sfx_new+x            ; 1C01
.l1C04:
    mov.w !spc_sfx_command+x,a        ; 1C04
    mov a,#$01                        ; 1C07
    mov.w !spc_sfx_state+x,a          ; 1C09
    mov a,#$00                        ; 1C0C
    mov.w !spc_sfx_age+x,a            ; 1C0E
    mov.w !spc_sfx_stop_request+x,a   ; 1C11
    call SpcBSfxMarkVoice             ; 1C14
    jmp SpcBKeyOffVoice               ; 1C17
.l1C1A:
    jmp .l1C87                        ; 1C1A
.l1C1D:
    mov.w a,!spc_sfx_stop_request+x   ; 1C1D
    beq .l1BDF                        ; 1C20
    cmp a,#$01                        ; 1C22
    beq .l1C1A                        ; 1C24
    cmp a,#$0A                        ; 1C26
    beq .l1C45                        ; 1C28
    cmp a,#$1A                        ; 1C2A
    beq .l1C45                        ; 1C2C
    cmp a,#$0B                        ; 1C2E
    beq .l1C4E                        ; 1C30
    cmp a,#$1B                        ; 1C32
    beq .l1C4E                        ; 1C34
    cmp a,#$0C                        ; 1C36
    beq .l1C57                        ; 1C38
    cmp a,#$1C                        ; 1C3A
    beq .l1C57                        ; 1C3C
.l1C3E:
    mov a,#$00                        ; 1C3E  ; not reached in the traces so far
    mov.w !spc_sfx_stop_request+x,a   ; 1C40  ; not reached in the traces so far
    bra .l1BDF                        ; 1C43  ; not reached in the traces so far
.l1C45:
    mov.w a,!spc_sfx_command+x        ; 1C45
    cmp a,#$25                        ; 1C48
    beq .l1C6D                        ; 1C4A
    bra .l1C3E                        ; 1C4C  ; not reached in the traces so far
.l1C4E:
    mov.w a,!spc_sfx_command+x        ; 1C4E
    cmp a,#$15                        ; 1C51
    beq .l1C6D                        ; 1C53
    bra .l1C3E                        ; 1C55  ; not reached in the traces so far
.l1C57:
    mov.w a,!spc_sfx_command+x        ; 1C57
    cmp a,#$17                        ; 1C5A
    beq .l1C6D                        ; 1C5C
    bra .l1C3E                        ; 1C5E  ; not reached in the traces so far
    db $F5, $90, $03, $68, $05, $F0, $20, $68
    db $08, $F0, $1C, $2F, $D1
.l1C6D:
    mov a,#$00                        ; 1C6D
    mov.w !spc_sfx_stop_request+x,a   ; 1C6F
    mov.w a,!spc_sfx_loop_sound+x     ; 1C72
    beq .l1C7F                        ; 1C75
    mov a,#$00                        ; 1C77
    mov.w !spc_sfx_loop_sound+x,a     ; 1C79
    jmp .l1BDF                        ; 1C7C
.l1C7F:
    mov.w a,!spc_sfx_loop+x           ; 1C7F
    bne .l1CAC                        ; 1C82
    jmp .l1BDF                        ; 1C84  ; not reached in the traces so far
.l1C87:
    mov a,#$00                        ; 1C87
    mov.w !spc_sfx_stop_request+x,a   ; 1C89
    mov.w a,!spc_sfx_loop_sound+x     ; 1C8C
    beq .l1CAC                        ; 1C8F
    push a                            ; 1C91  ; not reached in the traces so far
    mov.w a,!spcb_sfx_loop_side+x     ; 1C92  ; not reached in the traces so far
    cmp a,#$01                        ; 1C95  ; not reached in the traces so far
    beq .l1CA0                        ; 1C97  ; not reached in the traces so far
    cmp a,#$02                        ; 1C99  ; not reached in the traces so far
    beq .l1CA5                        ; 1C9B  ; not reached in the traces so far
    pop a                             ; 1C9D  ; not reached in the traces so far
    bra .l1CAC                        ; 1C9E  ; not reached in the traces so far
.l1CA0:
    pop a                             ; 1CA0  ; not reached in the traces so far
    asl a                             ; 1CA1  ; not reached in the traces so far
    setc                              ; 1CA2  ; not reached in the traces so far
    bra .l1CA8                        ; 1CA3  ; not reached in the traces so far
.l1CA5:
    pop a                             ; 1CA5  ; not reached in the traces so far
    asl a                             ; 1CA6  ; not reached in the traces so far
    clrc                              ; 1CA7  ; not reached in the traces so far
.l1CA8:
    ror a                             ; 1CA8  ; not reached in the traces so far
    call SpcBSoundQueuePush           ; 1CA9  ; not reached in the traces so far
.l1CAC:
    mov a,#$04                        ; 1CAC
    mov.w !spc_sfx_state+x,a          ; 1CAE
    call SpcBKeyOffVoice              ; 1CB1
    mov a,#$00                        ; 1CB4
    mov.w !spc_sfx_loop+x,a           ; 1CB6
    mov.w !spc_sfx_loop_sound+x,a     ; 1CB9
    mov.w !spc_sfx_age+x,a            ; 1CBC
    mov.w !spc_sfx_number+x,a         ; 1CBF
    mov.w !spcb_sfx_side+x,a          ; 1CC2
    mov.w !spcb_sfx_loop_side+x,a     ; 1CC5
    mov.w !spc_sfx_mode+x,a           ; 1CC8
    ret                               ; 1CCB

; @routine 1A:FCB1 SpcBSfxMarkVoice
; WHAT IT DOES: Marks voice X as a sound effect's ($1A). The race driver's SpcSfxMarkVoice.
; HOW: SpcBVoiceMask, OR.
; Entry: X = voice. Exit: A = $1A.
; @spc 1CCC
%spc($1CCC, $1AFCB1)
SpcBSfxMarkVoice:
    call SpcBVoiceMask                ; 1CCC
    mov.b a,!spc_sfx_voices           ; 1CCF
    or.b a,!spc_sfx_work              ; 1CD1
    mov.b !spc_sfx_voices,a           ; 1CD3
    ret                               ; 1CD5

; @routine 1A:FCBB SpcBVoiceMask
; WHAT IT DOES: $22 = the bit of voice X (1 << X). The race driver's SpcVoiceMask.
; HOW: ROL of a carry X + 1 times.
; Entry: X = voice. Exit: A = $22.
; @spc 1CD6
%spc($1CD6, $1AFCBB)
SpcBVoiceMask:
    mov.b !spc_sfx_work,x             ; 1CD6
    inc.b !spc_sfx_work               ; 1CD8
    mov a,#$00                        ; 1CDA
    setc                              ; 1CDC
.l1CDD:
    rol a                             ; 1CDD
    dec.b !spc_sfx_work               ; 1CDE
    bne .l1CDD                        ; 1CE0
    mov.b !spc_sfx_work,a             ; 1CE2
    ret                               ; 1CE4

; @routine 1A:FCCA SpcBVoiceMaskNot
; WHAT IT DOES: $22 = every bit but voice X's. After it (from $1CEE, where SpcBSfxChannelStep jumps), the release of voice X to the music, as the race driver's SpcSfxRelease: its sound effect bit cleared, the music instrument reloaded (SpcLoadInstrument), its echo bit from the music's echo voices $4F, and its volume rewritten (SpcVoiceVolumeWrite).
; HOW: ROL of a clear carry through $FF, sharing SpcBVoiceMask's loop; masks through $22, X doubled for the music's arrays.
; Entry: X = voice. Exit: A = $22; the release keeps X.
; @spc 1CE5
%spc($1CE5, $1AFCCA)
SpcBVoiceMaskNot:
    mov.b !spc_sfx_work,x             ; 1CE5
    inc.b !spc_sfx_work               ; 1CE7
    mov a,#$FF                        ; 1CE9
    clrc                              ; 1CEB
    bra SpcBVoiceMask_l1CDD           ; 1CEC
.l1CEE:
    call SpcBVoiceMaskNot             ; 1CEE
    mov.b a,!spc_sfx_voices           ; 1CF1
    and.b a,!spc_sfx_work             ; 1CF3
    mov.b !spc_sfx_voices,a           ; 1CF5
    push x                            ; 1CF7
    clrc                              ; 1CF8
    mov a,x                           ; 1CF9
    asl a                             ; 1CFA
    mov x,a                           ; 1CFB
    mov.w a,!spc_instrument+x         ; 1CFC
    call SpcLoadInstrument            ; 1CFF
    pop x                             ; 1D02
    mov.b !spc_sfx_tmp,x              ; 1D03
    inc.b !spc_sfx_tmp                ; 1D05
    mov.b a,!spc_echo_enable          ; 1D07
.l1D09:
    lsr a                             ; 1D09
    dec.b !spc_sfx_tmp                ; 1D0A
    bne .l1D09                        ; 1D0C
    bcc .l1D1B                        ; 1D0E
    call SpcBVoiceMask                ; 1D10
    mov.b a,!spc_echo_voices          ; 1D13
    or.b a,!spc_sfx_work              ; 1D15
    mov.b !spc_echo_voices,a          ; 1D17
    bra .l1D24                        ; 1D19
.l1D1B:
    call SpcBVoiceMaskNot             ; 1D1B
    mov.b a,!spc_echo_voices          ; 1D1E
    and.b a,!spc_sfx_work             ; 1D20
    mov.b !spc_echo_voices,a          ; 1D22
.l1D24:
    call SpcBVoiceMaskNot             ; 1D24
    mov.b a,!spc_sfx_voices           ; 1D27
    and.b a,!spc_sfx_work             ; 1D29
    mov.b !spc_sfx_voices,a           ; 1D2B
    call SpcBVoiceMask                ; 1D2D
    mov.b a,!spc_voice_bit            ; 1D30
    or.b a,!spc_sfx_work              ; 1D32
    mov.b !spc_voice_bit,a            ; 1D34
    mov.b a,!spc_volume_dirty         ; 1D36
    or.b a,!spc_sfx_tmp               ; 1D38
    mov.b !spc_volume_dirty,a         ; 1D3A
    push x                            ; 1D3C
    clrc                              ; 1D3D
    mov a,x                           ; 1D3E
    asl a                             ; 1D3F
    mov x,a                           ; 1D40
    call SpcVoiceVolumeWrite          ; 1D41
    pop x                             ; 1D44
    ret                               ; 1D45

; @routine 1A:FD2B SpcBKeyOnVoice
; WHAT IT DOES: Marks voice X for key-on ($45). The race driver's SpcKeyOnVoice.
; HOW: SpcBVoiceMask, OR.
; Entry: X = voice. Exit: A = $45.
; @spc 1D46
%spc($1D46, $1AFD2B)
SpcBKeyOnVoice:
    call SpcBVoiceMask                ; 1D46
    mov.b a,!spc_key_on               ; 1D49
    or.b a,!spc_sfx_work              ; 1D4B
    mov.b !spc_key_on,a               ; 1D4D
    ret                               ; 1D4F

; @routine 1A:FD35 SpcBKeyOffVoice
; WHAT IT DOES: Marks voice X for key-off ($46). The race driver's SpcKeyOffVoice. After it, SpcBSfxChannelStep's playing path: SpcBSfxRead, SpcBSfxSpecial, then the DSP writes at $21C8.
; HOW: SpcBVoiceMask, OR.
; Entry: X = voice. Exit: A = $46.
; @spc 1D50
%spc($1D50, $1AFD35)
SpcBKeyOffVoice:
    call SpcBVoiceMask                ; 1D50
    mov.b a,!spc_key_off              ; 1D53
    or.b a,!spc_sfx_work              ; 1D55
    mov.b !spc_key_off,a              ; 1D57
    ret                               ; 1D59
.l1D5A:
    call SpcBSfxRead                  ; 1D5A
    call SpcBSfxSpecial               ; 1D5D
    jmp SpcBSfxStart_l21C8            ; 1D60

; @routine 1A:FD48 SpcBSfxRead
; WHAT IT DOES: Channel X's sound data for this tick, as the race driver's SpcSfxRead: its age $03F0 counts (a set lifetime $0448 stops it); on the first tick its pitch, volume, slide and envelope state is reset (ADSR $8F/$E0, echo off); while a note's length $0418 runs, the slides step (from $213F); then bytes are read from ($20)+Y: note numbers set the pitch (the table at $222B, by octave, with the channel's detune $0440), $00-$7F lengths, and commands $E0 and up run their handler from the 32-entry table at $1E16 (volume, pan, pitch and volume slides, echo on/off, ADSR, sample, lifetime, sound jumps, end).
; HOW: JMP ($1E16+X) with X pushed; handlers return into the read loop at $1E04. The JMP at $1DFD follows the JMP to the slides and nothing reaches it.
; Entry: X = channel. Exit: as the last handler leaves it.
; @spc 1D63
%spc($1D63, $1AFD48)
SpcBSfxRead:
    mov.w a,!spc_sfx_command+x        ; 1D63
    beq .l1D86                        ; 1D66
    mov.w a,!spc_sfx_lifetime+x       ; 1D68
    beq .l1D79                        ; 1D6B
    mov.b !spc_sfx_tmp,a              ; 1D6D
    mov.w a,!spc_sfx_age+x            ; 1D6F
    cmp.b a,!spc_sfx_tmp              ; 1D72
    bne .l1D79                        ; 1D74
    call SpcBSfxStop                  ; 1D76
.l1D79:
    mov.w a,!spc_sfx_age+x            ; 1D79
    cmp a,#$FF                        ; 1D7C
    beq .l1D84                        ; 1D7E
    inc a                             ; 1D80
    mov.w !spc_sfx_age+x,a            ; 1D81
.l1D84:
    bra .l1D8C                        ; 1D84
.l1D86:
    mov a,#$00                        ; 1D86  ; not reached in the traces so far
    mov.w !spc_sfx_age+x,a            ; 1D88  ; not reached in the traces so far
    ret                               ; 1D8B  ; not reached in the traces so far
.l1D8C:
    call SpcBSfxChannelLoad           ; 1D8C
    mov.w a,!spc_sfx_age+x            ; 1D8F
    beq .l1D9A                        ; 1D92
    cmp a,#$01                        ; 1D94
    beq .l1D9B                        ; 1D96
    bra .l1DED                        ; 1D98
.l1D9A:
    ret                               ; 1D9A  ; not reached in the traces so far
.l1D9B:
    mov a,#$00                        ; 1D9B
    mov.w !spc_sfx_read_pos+x,a       ; 1D9D
    mov.w !spc_sfx_slide_on+x,a       ; 1DA0
    mov.w !spc_sfx_sweep_on+x,a       ; 1DA3
    mov.w !spc_sfx_key_state+x,a      ; 1DA6
    mov.w !spc_sfx_lifetime+x,a       ; 1DA9
    mov.w !spc_sfx_pitch_scale+x,a    ; 1DAC
    mov.w !spc_sfx_vol_l_frac+x,a     ; 1DAF
    mov.w !spc_sfx_vol_r_frac+x,a     ; 1DB2
    mov.w !spc_sfx_dsp_pitch+x,a      ; 1DB5
    mov.w !spc_sfx_pitch+x,a          ; 1DB8
    mov.w !spc_sfx_dsp_pitch_hi+x,a   ; 1DBB
    mov.w !spc_sfx_pitch_hi+x,a       ; 1DBE
    mov.w !spc_sfx_fade_count+x,a     ; 1DC1
    mov.w !spc_sfx_fading+x,a         ; 1DC4
    mov.w !spc_sfx_mode+x,a           ; 1DC7
    mov.w !spc_sfx_vol_l+x,a          ; 1DCA
    mov.w !spc_sfx_vol_l_int+x,a      ; 1DCD
    mov.w !spc_sfx_vol_r+x,a          ; 1DD0
    mov.w !spc_sfx_vol_r_int+x,a      ; 1DD3
    mov a,#$8F                        ; 1DD6
    mov.w !spc_sfx_adsr1+x,a          ; 1DD8
    mov a,#$E0                        ; 1DDB
    mov.w !spc_sfx_adsr2+x,a          ; 1DDD
    call SpcBVoiceMaskNot             ; 1DE0
    mov.b a,!spc_echo_voices          ; 1DE3
    and.b a,!spc_sfx_work             ; 1DE5
    mov.b !spc_echo_voices,a          ; 1DE7
    mov y,#$01                        ; 1DE9
    bra .l1E05                        ; 1DEB
.l1DED:
    mov.w a,!spc_sfx_length+x         ; 1DED
    dec a                             ; 1DF0
    mov.w !spc_sfx_length+x,a         ; 1DF1
    beq .l1E00                        ; 1DF4
    cmp a,#$FF                        ; 1DF6
    beq .l1E00                        ; 1DF8
    jmp SpcBSfxStart_l213F            ; 1DFA
    db $5F, $7B, $21
.l1E00:
    mov.w a,!spc_sfx_read_pos+x       ; 1E00
    mov y,a                           ; 1E03
.l1E04:
    inc y                             ; 1E04
.l1E05:
    mov a,(!spc_sfx_tmp)+y            ; 1E05
    beq .l1E04                        ; 1E07
    bpl .l1E56                        ; 1E09
    setc                              ; 1E0B
    sbc a,#$E0                        ; 1E0C
    bmi .l1E87                        ; 1E0E
    asl a                             ; 1E10
    push x                            ; 1E11
    mov x,a                           ; 1E12
    jmp (.t1E16+x)                    ; 1E13
.t1E16:
    db $B6, $20, $0E, $1F, $12, $1F, $1B, $1F
    db $6A, $1F, $D4, $1F, $E3, $1F, $FE, $1F
    db $12, $20, $27, $20, $3C, $20, $48, $20
    db $55, $20, $C0, $20, $E1, $20, $B6, $20
    db $B6, $20, $EB, $20, $F4, $20, $FD, $20
    db $B6, $20, $B6, $20, $B6, $20, $B6, $20
    db $0F, $21, $B6, $20, $B6, $20, $19, $21
    db $25, $21, $2F, $21, $68, $1F, $B6, $20
.l1E56:
    push a                            ; 1E56
    mov.w a,!spc_sfx_key_state+x      ; 1E57
    beq .l1E62                        ; 1E5A
    cmp a,#$01                        ; 1E5C
    beq .l1E65                        ; 1E5E
    bra .l1E6A                        ; 1E60
.l1E62:
    call SpcBKeyOffVoice              ; 1E62
.l1E65:
    mov a,#$01                        ; 1E65
    mov.w !spc_sfx_key_request+x,a    ; 1E67
.l1E6A:
    mov a,#$00                        ; 1E6A
    mov.w !spc_sfx_key_state+x,a      ; 1E6C
    pop a                             ; 1E6F
    mov.w !spc_sfx_length+x,a         ; 1E70
    cmp a,#$7F                        ; 1E73
    bne .l1E7B                        ; 1E75
    mov a,#$01                        ; 1E77
    bra .l1E7D                        ; 1E79
.l1E7B:
    mov a,#$00                        ; 1E7B
.l1E7D:
    mov.w !spc_sfx_held+x,a           ; 1E7D
    mov a,y                           ; 1E80
    mov.w !spc_sfx_read_pos+x,a       ; 1E81
    jmp SpcBSfxChannelSave            ; 1E84
.l1E87:
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
    mov.w a,SpcBSfxStart_t222B+x      ; 1E95
    mov.b !spc_sfx_work,a             ; 1E98
    mov.w a,SpcBSfxStart_t222C+x      ; 1E9A
    mov.b !spc_sfx_work+1,a           ; 1E9D
    pop a                             ; 1E9F
    setc                              ; 1EA0
    sbc a,#$07                        ; 1EA1
    mov.b !spc_sfx_data,a             ; 1EA3
    beq .l1EB9                        ; 1EA5
.l1EA7:
    mov y,#$00                        ; 1EA7
    mov.b a,!spc_sfx_work+1           ; 1EA9
    mov x,#$02                        ; 1EAB
    div ya,x                          ; 1EAD
    mov.b !spc_sfx_work+1,a           ; 1EAE
    mov.b a,!spc_sfx_work             ; 1EB0
    div ya,x                          ; 1EB2
    mov.b !spc_sfx_work,a             ; 1EB3
    inc.b !spc_sfx_data               ; 1EB5
    bne .l1EA7                        ; 1EB7
.l1EB9:
    pop x                             ; 1EB9
    mov.b a,!spc_sfx_work             ; 1EBA
    mov.w !spc_sfx_dsp_pitch+x,a      ; 1EBC
    mov.w !spc_sfx_pitch+x,a          ; 1EBF
    mov.b a,!spc_sfx_work+1           ; 1EC2
    mov.w !spc_sfx_dsp_pitch_hi+x,a   ; 1EC4
    mov.w !spc_sfx_pitch_hi+x,a       ; 1EC7
    mov.w a,!spc_sfx_pitch_scale+x    ; 1ECA
    beq .l1F0A                        ; 1ECD
    push x                            ; 1ECF
    mov.b !spc_sfx_data,a             ; 1ED0
    mov.b a,!spc_sfx_work+1           ; 1ED2
    setc                              ; 1ED4
    sbc a,#$10                        ; 1ED5
    bmi .l1EEB                        ; 1ED7
    mov.b a,!spc_sfx_work             ; 1ED9
    mov.b y,!spc_sfx_work+1           ; 1EDB
    mov x,#$43                        ; 1EDD
    div ya,x                          ; 1EDF
    mov y,#$04                        ; 1EE0
    mul ya                            ; 1EE2
    mov x,#$FC                        ; 1EE3
    div ya,x                          ; 1EE5
    mov.b y,!spc_sfx_data             ; 1EE6
    mul ya                            ; 1EE8
    bra .l1F00                        ; 1EE9
.l1EEB:
    mov.b a,!spc_sfx_work             ; 1EEB
    mov.b y,!spc_sfx_work+1           ; 1EED
    mov x,#$11                        ; 1EEF
    div ya,x                          ; 1EF1
    mov x,a                           ; 1EF2
    mov a,#$FF                        ; 1EF3
    mov y,#$00                        ; 1EF5
    div ya,x                          ; 1EF7
    mov x,a                           ; 1EF8
    mov y,#$00                        ; 1EF9
    mov.b a,!spc_sfx_data             ; 1EFB
    div ya,x                          ; 1EFD
    mov y,#$00                        ; 1EFE
.l1F00:
    addw ya,!spc_sfx_work             ; 1F00
    pop x                             ; 1F02
    mov.w !spc_sfx_dsp_pitch+x,a      ; 1F03
    mov a,y                           ; 1F06
    mov.w !spc_sfx_dsp_pitch_hi+x,a   ; 1F07
.l1F0A:
    pop y                             ; 1F0A
    jmp .l1E04                        ; 1F0B
    mov a,#$01                        ; 1F0E
    bra .l1F14                        ; 1F10
    mov a,#$02                        ; 1F12
.l1F14:
    pop x                             ; 1F14
    mov.w !spc_sfx_key_state+x,a      ; 1F15
    jmp .l1E04                        ; 1F18
    pop x                             ; 1F1B
    inc y                             ; 1F1C
    mov a,(!spc_sfx_tmp)+y            ; 1F1D
    mov.w !spc_sfx_number+x,a         ; 1F1F
    jmp .l1E04                        ; 1F22

; @routine 1A:FF0A SpcBSfxEngineScale
; WHAT IT DOES: For a channel in engine mode ($04F0 = 4), scales the volume A by the engine speed from port 3, or port 2 for side 2 ($02 or $03 * 3, / 2, + $80); otherwise A is kept. After it, SpcBSfxRead's command handlers: the volume command (in engine mode both sides; otherwise side 1's sounds play on the left only and side 2's on the right), the ADSR fields, the pitch slide, echo on and off, and a jump to another sound (the one whose data pointer is the operand, from sound $5F on, started through SpcBSfxStart). The race driver's SpcSfxEngineScale reads port 3 only.
; HOW: SpcBDiv16, SpcBMul16; clamped to $7F; the handlers return to $1E04.
; Entry: A = volume, X = channel. Exit: A = volume; X, Y kept.
; @spc 1F25
%spc($1F25, $1AFF0A)
SpcBSfxEngineScale:
    push x                            ; 1F25
    push y                            ; 1F26
    mov.b !spc_sfx_work,a             ; 1F27
    mov.w a,!spc_sfx_mode+x           ; 1F29
    cmp a,#$04                        ; 1F2C
    bne .l1F64                        ; 1F2E
    mov.w a,!spcb_sfx_side+x          ; 1F30
    cmp a,#$02                        ; 1F33
    beq .l1F3B                        ; 1F35
    mov.b a,!spc_port_in+3            ; 1F37
    bra .l1F3D                        ; 1F39
.l1F3B:
    mov.b a,!spc_port_in+2            ; 1F3B  ; not reached in the traces so far
.l1F3D:
    cmp a,#$00                        ; 1F3D
    bne .l1F42                        ; 1F3F
    inc a                             ; 1F41  ; not reached in the traces so far
.l1F42:
    mov y,#$03                        ; 1F42
    mul ya                            ; 1F44
    mov x,#$02                        ; 1F45
    call SpcBDiv16                    ; 1F47
    mov.b !spc_sfx_data,a             ; 1F4A
    mov.b !spc_sfx_data+1,y           ; 1F4C
    mov a,#$80                        ; 1F4E
    mov y,#$00                        ; 1F50
    addw ya,!spc_sfx_data             ; 1F52
    push y                            ; 1F54
    pop x                             ; 1F55
    mov y,a                           ; 1F56
    mov.b a,!spc_sfx_work             ; 1F57
    call SpcBMul16                    ; 1F59
    mov a,y                           ; 1F5C
    bpl .l1F61                        ; 1F5D
    mov a,#$7F                        ; 1F5F  ; not reached in the traces so far
.l1F61:
    pop y                             ; 1F61
    pop x                             ; 1F62
    ret                               ; 1F63
.l1F64:
    mov.b a,!spc_sfx_work             ; 1F64
    bra .l1F61                        ; 1F66
    set1 !spc_track_flags.6           ; 1F68
    pop x                             ; 1F6A
    inc y                             ; 1F6B
    mov.w a,!spc_sfx_mode+x           ; 1F6C
    cmp a,#$01                        ; 1F6F
    bne .l1F90                        ; 1F71
    mov a,(!spc_sfx_tmp)+y            ; 1F73
    call SpcBSfxEngineScale           ; 1F75
    mov.w !spc_sfx_vol_l+x,a          ; 1F78
    mov.w !spc_sfx_vol_l_int+x,a      ; 1F7B
    bbs !spc_track_flags.6,.l1F82     ; 1F7E
    inc y                             ; 1F81
.l1F82:
    mov a,(!spc_sfx_tmp)+y            ; 1F82
    call SpcBSfxEngineScale           ; 1F84
    mov.w !spc_sfx_vol_r+x,a          ; 1F87
    mov.w !spc_sfx_vol_r_int+x,a      ; 1F8A
    jmp .l1FC7                        ; 1F8D
.l1F90:
    mov.w a,!spcb_sfx_side+x          ; 1F90
    cmp a,#$02                        ; 1F93
    beq .l1FB0                        ; 1F95
    mov a,(!spc_sfx_tmp)+y            ; 1F97
    call SpcBSfxEngineScale           ; 1F99
    mov.w !spc_sfx_vol_l+x,a          ; 1F9C
    mov.w !spc_sfx_vol_l_int+x,a      ; 1F9F
    bbs !spc_track_flags.6,.l1FA6     ; 1FA2
    inc y                             ; 1FA5  ; not reached in the traces so far
.l1FA6:
    mov a,#$00                        ; 1FA6
    mov.w !spc_sfx_vol_r+x,a          ; 1FA8
    mov.w !spc_sfx_vol_r_int+x,a      ; 1FAB
    bra .l1FC7                        ; 1FAE
.l1FB0:
    mov a,#$00                        ; 1FB0
    mov.w !spc_sfx_vol_l+x,a          ; 1FB2
    mov.w !spc_sfx_vol_l_int+x,a      ; 1FB5
    bbs !spc_track_flags.6,.l1FBC     ; 1FB8
    inc y                             ; 1FBB  ; not reached in the traces so far
.l1FBC:
    mov a,(!spc_sfx_tmp)+y            ; 1FBC
    call SpcBSfxEngineScale           ; 1FBE
    mov.w !spc_sfx_vol_r+x,a          ; 1FC1
    mov.w !spc_sfx_vol_r_int+x,a      ; 1FC4
.l1FC7:
    mov a,#$00                        ; 1FC7
    mov.w !spc_sfx_vol_l_frac+x,a     ; 1FC9
    mov.w !spc_sfx_vol_r_frac+x,a     ; 1FCC
    clr1 !spc_track_flags.6           ; 1FCF
    jmp SpcBSfxRead_l1E04             ; 1FD1
    pop x                             ; 1FD4
    inc y                             ; 1FD5
    mov a,(!spc_sfx_tmp)+y            ; 1FD6
    and a,#$0F                        ; 1FD8
    mov.b !spc_sfx_work,a             ; 1FDA
    mov.w a,!spc_sfx_adsr1+x          ; 1FDC
    and a,#$F0                        ; 1FDF
    bra .l1FF4                        ; 1FE1
    pop x                             ; 1FE3
    inc y                             ; 1FE4
    mov a,(!spc_sfx_tmp)+y            ; 1FE5
    and a,#$07                        ; 1FE7
    asl a                             ; 1FE9
    asl a                             ; 1FEA
    asl a                             ; 1FEB
    asl a                             ; 1FEC
    mov.b !spc_sfx_work,a             ; 1FED
    mov.w a,!spc_sfx_adsr1+x          ; 1FEF
    and a,#$8F                        ; 1FF2
.l1FF4:
    or.b a,!spc_sfx_work              ; 1FF4
    or a,#$80                         ; 1FF6
    mov.w !spc_sfx_adsr1+x,a          ; 1FF8
    jmp SpcBSfxRead_l1E04             ; 1FFB
    pop x                             ; 1FFE
    inc y                             ; 1FFF
    mov a,(!spc_sfx_tmp)+y            ; 2000
    and a,#$07                        ; 2002
    asl a                             ; 2004
    asl a                             ; 2005
    asl a                             ; 2006
    asl a                             ; 2007
    asl a                             ; 2008
    mov.b !spc_sfx_work,a             ; 2009
    mov.w a,!spc_sfx_adsr2+x          ; 200B
    and a,#$1F                        ; 200E
    bra .l201F                        ; 2010
    pop x                             ; 2012
    inc y                             ; 2013
    mov a,(!spc_sfx_tmp)+y            ; 2014
    and a,#$1F                        ; 2016
    mov.b !spc_sfx_work,a             ; 2018
    mov.w a,!spc_sfx_adsr2+x          ; 201A
    and a,#$E0                        ; 201D
.l201F:
    or.b a,!spc_sfx_work              ; 201F
    mov.w !spc_sfx_adsr2+x,a          ; 2021
    jmp SpcBSfxRead_l1E04             ; 2024
    pop x                             ; 2027
    inc y                             ; 2028
    mov a,(!spc_sfx_tmp)+y            ; 2029
    mov.w !spc_sfx_slide_delta+x,a    ; 202B
    inc y                             ; 202E
    mov a,(!spc_sfx_tmp)+y            ; 202F
    mov.w !spc_sfx_slide_delta_hi+x,a ; 2031
    mov a,#$01                        ; 2034
    mov.w !spc_sfx_slide_on+x,a       ; 2036
    jmp SpcBSfxRead_l1E04             ; 2039
    pop x                             ; 203C
    call SpcBVoiceMask                ; 203D
    mov.b a,!spc_echo_voices          ; 2040
    or.b a,!spc_sfx_work              ; 2042
    mov.b !spc_echo_voices,a          ; 2044
    bra .l2052                        ; 2046
    pop x                             ; 2048
    call SpcBVoiceMaskNot             ; 2049
    mov.b a,!spc_echo_voices          ; 204C
    and.b a,!spc_sfx_work             ; 204E
    mov.b !spc_echo_voices,a          ; 2050
.l2052:
    jmp SpcBSfxRead_l1E04             ; 2052
    inc y                             ; 2055
    mov a,(!spc_sfx_tmp)+y            ; 2056
    mov.b !spc_sfx_data,a             ; 2058
    inc y                             ; 205A
    mov a,(!spc_sfx_tmp)+y            ; 205B
    mov.b !spc_sfx_data+1,a           ; 205D
    push y                            ; 205F
    mov x,#$BC                        ; 2060
.l2062:
    mov.w a,SpcReload_t0702+x         ; 2062
    cmp.b a,!spc_sfx_data             ; 2065
    beq .l206D                        ; 2067
    inc x                             ; 2069
    inc x                             ; 206A
    bra .l2062                        ; 206B
.l206D:
    inc x                             ; 206D
    mov.w a,SpcReload_t0702+x         ; 206E
    cmp.b a,!spc_sfx_data+1           ; 2071
    beq .l207C                        ; 2073
    inc x                             ; 2075
    cmp x,#$00                        ; 2076
    beq .l2082                        ; 2078
    bra .l2062                        ; 207A
.l207C:
    mov a,x                           ; 207C
    lsr a                             ; 207D
    inc a                             ; 207E
    call SpcBSfxStart                 ; 207F
.l2082:
    pop y                             ; 2082
    pop x                             ; 2083
    jmp SpcBSfxRead_l1E04             ; 2084

; @routine 1B:806C SpcBSfxStart
; WHAT IT DOES: Starts sound A (its data at $24/$25) on its channel unless that channel is in mode 2 or 3: channel, mode bits and side, data pointer, active, then the start state (SpcBSfxChannelStep's code at $1C04). As the race driver's SpcSfxStart, with the side. After it, more of SpcBSfxRead's command handlers (the end, volume slide, detune, slide off, a jump in the data, lifetime, key-off, engine mode, noise), the slides (from $213F: the pitch slide, clamped to $3FFF), and from $21C8 the DSP writes of channel X's voice (volume left and right, by the fade and SpcBSfxVolumeAdjust, pitch, sample, ADSR), then the 12-entry pitch table at $222B. The bytes $217B-$21C7 are code no path reaches.
; HOW: SpcBSfxChannel, SpcBSfxMode, SpcBSfxSide, SpcBSfxActivate; the DSP writes through SpcDspWrite with Y = voice * $10.
; Entry: A = sound, $24/$25 = data. Exit: as SpcBSfxChannelStep.
; @spc 2087
%spc($2087, $1B806C)
SpcBSfxStart:
    mov.b !spc_sfx_work+1,a           ; 2087
    push a                            ; 2089
    mov y,#$00                        ; 208A
    mov a,(!spc_sfx_data)+y           ; 208C
    mov x,a                           ; 208E
    mov.w a,!spc_sfx_mode+x           ; 208F
    cmp a,#$02                        ; 2092
    beq .l20B4                        ; 2094
    cmp a,#$03                        ; 2096
    beq .l20B4                        ; 2098
    call SpcBSfxChannel               ; 209A
    call SpcBSfxMode                  ; 209D
    call SpcBSfxSide                  ; 20A0
    mov.b a,!spc_sfx_data             ; 20A3
    mov.w !spc_sfx_ptr+x,a            ; 20A5
    mov.b a,!spc_sfx_data+1           ; 20A8
    mov.w !spc_sfx_ptr_hi+x,a         ; 20AA
    call SpcBSfxActivate              ; 20AD
    pop a                             ; 20B0
    jmp SpcBSfxChannelStep_l1C04      ; 20B1
.l20B4:
    pop a                             ; 20B4  ; not reached in the traces so far
    ret                               ; 20B5  ; not reached in the traces so far
    pop x                             ; 20B6
    mov.w a,!spc_sfx_held+x           ; 20B7
    bne .l20BF                        ; 20BA
    call SpcBSfxStop                  ; 20BC
.l20BF:
    ret                               ; 20BF
    pop x                             ; 20C0
    inc y                             ; 20C1
    mov a,(!spc_sfx_tmp)+y            ; 20C2
    mov.w !spc_sfx_sweep+x,a          ; 20C4
    inc y                             ; 20C7
    mov a,(!spc_sfx_tmp)+y            ; 20C8
    mov.w !spc_sfx_sweep_b+x,a        ; 20CA
    inc y                             ; 20CD
    mov a,(!spc_sfx_tmp)+y            ; 20CE
    mov.w !spc_sfx_sweep_c+x,a        ; 20D0
    inc y                             ; 20D3
    mov a,(!spc_sfx_tmp)+y            ; 20D4
    mov.w !spc_sfx_sweep_d+x,a        ; 20D6
    mov a,#$01                        ; 20D9
    mov.w !spc_sfx_sweep_on+x,a       ; 20DB
    jmp SpcBSfxRead_l1E04             ; 20DE
    pop x                             ; 20E1
    inc y                             ; 20E2
    mov a,(!spc_sfx_tmp)+y            ; 20E3
    mov.w !spc_sfx_pitch_scale+x,a    ; 20E5
    jmp SpcBSfxRead_l1E04             ; 20E8
    pop x                             ; 20EB
    mov a,#$00                        ; 20EC
    mov.w !spc_sfx_slide_on+x,a       ; 20EE
    jmp SpcBSfxRead_l1E04             ; 20F1
    pop x                             ; 20F4
    mov a,#$00                        ; 20F5
    mov.w !spc_sfx_sweep_on+x,a       ; 20F7
    jmp SpcBSfxRead_l1E04             ; 20FA
    pop x                             ; 20FD
    inc y                             ; 20FE
    mov a,(!spc_sfx_tmp)+y            ; 20FF
    push a                            ; 2101
    inc y                             ; 2102
    mov a,(!spc_sfx_tmp)+y            ; 2103
    mov.b !spc_sfx_tmp+1,a            ; 2105
    pop a                             ; 2107
    mov.b !spc_sfx_tmp,a              ; 2108
    mov y,#$00                        ; 210A
    jmp SpcBSfxRead_l1E05             ; 210C
    pop x                             ; 210F
    inc y                             ; 2110
    mov a,(!spc_sfx_tmp)+y            ; 2111
    mov.w !spc_sfx_lifetime+x,a       ; 2113
    jmp SpcBSfxRead_l1E04             ; 2116
    pop x                             ; 2119
    call SpcBKeyOffVoice              ; 211A
    mov a,#$02                        ; 211D
    mov.w !spc_sfx_key_state+x,a      ; 211F
    jmp SpcBSfxRead_l1E04             ; 2122
    pop x                             ; 2125
    inc y                             ; 2126
    mov a,(!spc_sfx_tmp)+y            ; 2127
    mov.w !spc_sfx_mode+x,a           ; 2129
    jmp SpcBSfxRead_l1E04             ; 212C
    pop x                             ; 212F  ; not reached in the traces so far
    inc y                             ; 2130  ; not reached in the traces so far
    mov a,(!spc_sfx_tmp)+y            ; 2131  ; not reached in the traces so far
    and a,#$FE                        ; 2133
    push y                            ; 2135  ; not reached in the traces so far
    push x                            ; 2136  ; not reached in the traces so far
    call SpcBNoiseSampleInit          ; 2137  ; not reached in the traces so far
    pop x                             ; 213A  ; not reached in the traces so far
    pop y                             ; 213B  ; not reached in the traces so far
    jmp SpcBSfxRead_l1E04             ; 213C  ; not reached in the traces so far
.l213F:
    mov.w a,!spc_sfx_slide_on+x       ; 213F
    beq .l216F                        ; 2142
    mov.w a,!spc_sfx_slide_delta+x    ; 2144
    mov.b !spc_sfx_tmp,a              ; 2147
    mov.w a,!spc_sfx_slide_delta_hi+x ; 2149
    mov.b !spc_sfx_tmp+1,a            ; 214C
    mov.w a,!spc_sfx_pitch_hi+x       ; 214E
    mov y,a                           ; 2151
    mov.w a,!spc_sfx_pitch+x          ; 2152
    addw ya,!spc_sfx_tmp              ; 2155
    mov.w !spc_sfx_pitch+x,a          ; 2157
    mov a,y                           ; 215A
    mov.w !spc_sfx_pitch_hi+x,a       ; 215B
    bmi .l2170                        ; 215E
    setc                              ; 2160
    sbc a,#$40                        ; 2161
    bpl .l2175                        ; 2163
    mov.w a,!spc_sfx_pitch+x          ; 2165
.l2168:
    mov.w !spc_sfx_dsp_pitch+x,a      ; 2168
    mov a,y                           ; 216B
    mov.w !spc_sfx_dsp_pitch_hi+x,a   ; 216C
.l216F:
    ret                               ; 216F
.l2170:
    mov a,#$00                        ; 2170
    mov y,a                           ; 2172
    bra .l2168                        ; 2173
.l2175:
    mov a,#$FF                        ; 2175  ; not reached in the traces so far
    mov y,#$3F                        ; 2177  ; not reached in the traces so far
    bra .l2168                        ; 2179  ; not reached in the traces so far
    db $F5, $20, $05, $F0, $24, $F5, $00, $05
    db $C4, $20, $F5, $08, $05, $C4, $21, $3F
    db $A5, $21, $F5, $10, $05, $C4, $20, $F5
    db $18, $05, $C4, $21, $7D, $60, $88, $08
    db $5D, $3F, $A5, $21, $7D, $80, $A8, $08
    mov x,a                           ; 21A3
    ret                               ; 21A4  ; not reached in the traces so far
    db $F5, $A8, $04, $FD, $F5, $A8, $03, $7A
    db $20, $D5, $A8, $03, $DD, $D5, $A8, $04
    db $30, $02, $2F, $0B, $80, $A8, $C0, $30
    db $04, $E8, $00, $2F, $02, $E8, $7F, $D5
    db $98, $03, $6F
.l21C8:
    mov a,x                           ; 21C8
    mov y,#$10                        ; 21C9
    mul ya                            ; 21CB
    mov y,a                           ; 21CC
    push y                            ; 21CD
    mov.w a,!spc_sfx_active+x         ; 21CE
    beq .l21DD                        ; 21D1
    mov.w a,!spc_sfx_fade_count+x     ; 21D3
    beq .l21DD                        ; 21D6
    call SpcBSfxFadeVolume            ; 21D8
    bra .l21E0                        ; 21DB
.l21DD:
    mov.w a,!spc_sfx_vol_l+x          ; 21DD
.l21E0:
    call SpcBSfxVolumeAdjust          ; 21E0
    pop y                             ; 21E3
    call SpcDspWrite                  ; 21E4
    inc y                             ; 21E7
    push y                            ; 21E8
    mov.w a,!spc_sfx_active+x         ; 21E9
    beq .l21FE                        ; 21EC
    mov.w a,!spc_sfx_fading+x         ; 21EE
    beq .l21FE                        ; 21F1
    call SpcBIndexRight               ; 21F3
    call SpcBSfxFadeVolume            ; 21F6
    call SpcBIndexLeft                ; 21F9
    bra .l2201                        ; 21FC
.l21FE:
    mov.w a,!spc_sfx_vol_r+x          ; 21FE
.l2201:
    call SpcBSfxVolumeAdjust          ; 2201
    pop y                             ; 2204
    call SpcDspWrite                  ; 2205
    inc y                             ; 2208
    mov.w a,!spc_sfx_dsp_pitch+x      ; 2209
    call SpcDspWrite                  ; 220C
    inc y                             ; 220F
    mov.w a,!spc_sfx_dsp_pitch_hi+x   ; 2210
    call SpcDspWrite                  ; 2213
    inc y                             ; 2216
    mov.w a,!spc_sfx_number+x         ; 2217
    call SpcDspWrite                  ; 221A
    inc y                             ; 221D
    mov.w a,!spc_sfx_adsr1+x          ; 221E
    call SpcDspWrite                  ; 2221
    inc y                             ; 2224
    mov.w a,!spc_sfx_adsr2+x          ; 2225
    jmp SpcDspWrite                   ; 2228
.t222B:
    db $7C
.t222C:
    db $21, $7A, $23, $96, $25, $D2, $27, $30
    db $2A, $B2, $2C, $5A, $2F, $2C, $32, $28
    db $35, $50, $38, $AA, $3B, $36, $3F

; @routine 1B:8228 SpcBSfxFadeVolume
; WHAT IT DOES: The volume of channel X while it fades out (count A, $04C0), as the race driver's SpcSfxFadeVolume: the start volume and per-step drop taken at count 1, the volume interpolated down over $3C steps; at the end the channel stops (both sides of a right-side index) or the driver reloads or leaves, by flags.
; HOW: SpcBMul16, SpcBDiv16 by $3C; SUBW.
; Entry: A = count, X = channel. Exit: A = volume.
; @spc 2243
%spc($2243, $1B8228)
SpcBSfxFadeVolume:
    cmp a,#$01                        ; 2243
    beq .l2255                        ; 2245
    cmp a,#$3C                        ; 2247
    beq .l2292                        ; 2249
    cmp a,#$FF                        ; 224B
    beq .l22BA                        ; 224D
    inc a                             ; 224F
    mov.w !spc_sfx_fade_count+x,a     ; 2250
    bra .l226E                        ; 2253
.l2255:
    inc a                             ; 2255
    mov.w !spc_sfx_fade_count+x,a     ; 2256
    mov.w a,!spc_sfx_vol_l+x          ; 2259
    mov y,a                           ; 225C
    mov.w a,!spc_sfx_vol_l_frac+x     ; 225D
    push x                            ; 2260
    mov x,#$3C                        ; 2261
    call SpcBDiv16                    ; 2263
    pop x                             ; 2266
    mov.w !spc_sfx_fade_volume+x,a    ; 2267
    mov a,y                           ; 226A
    mov.w !spc_sfx_fade_step+x,a      ; 226B
.l226E:
    mov.w a,!spc_sfx_fade_count+x     ; 226E
    dec a                             ; 2271
    mov.b !spc_sfx_work,a             ; 2272
    mov.w a,!spc_sfx_fade_volume+x    ; 2274
    mov y,a                           ; 2277
    mov.w a,!spc_sfx_fade_step+x      ; 2278
    push x                            ; 227B
    mov x,a                           ; 227C
    mov.b a,!spc_sfx_work             ; 227D
    call SpcBMul16                    ; 227F
    pop x                             ; 2282
    mov.b !spc_sfx_work,a             ; 2283
    mov.b !spc_sfx_work+1,y           ; 2285
    mov.w a,!spc_sfx_vol_l+x          ; 2287
    mov y,a                           ; 228A
    mov.w a,!spc_sfx_vol_l_frac+x     ; 228B
    subw ya,!spc_sfx_work             ; 228E
    mov a,y                           ; 2290
    ret                               ; 2291
.l2292:
    bbs !spc_track_flags.7,.l229A     ; 2292
    bbs !spc_game_flags.0,.l229D      ; 2295  ; not reached in the traces so far
    bra .l22A0                        ; 2298  ; not reached in the traces so far
.l229A:
    jmp SpcBMusicFade_l19F5           ; 229A
.l229D:
    jmp SpcBKeyOffAll_l1A21           ; 229D  ; not reached in the traces so far
.l22A0:
    mov.b !spc_sfx_work,x             ; 22A0  ; not reached in the traces so far
    bbs !spc_sfx_work.3,.l22AF        ; 22A2  ; not reached in the traces so far
    call SpcBSfxStop                  ; 22A5  ; not reached in the traces so far
.l22A8:
    mov a,#$FF                        ; 22A8  ; not reached in the traces so far
    mov.w !spc_sfx_fade_count+x,a     ; 22AA  ; not reached in the traces so far
    inc a                             ; 22AD  ; not reached in the traces so far
    ret                               ; 22AE  ; not reached in the traces so far
.l22AF:
    call SpcBIndexLeft                ; 22AF  ; not reached in the traces so far
    call SpcBSfxStop                  ; 22B2  ; not reached in the traces so far
    call SpcBIndexRight               ; 22B5  ; not reached in the traces so far
    bra .l22A8                        ; 22B8  ; not reached in the traces so far
.l22BA:
    inc a                             ; 22BA  ; not reached in the traces so far
    mov.w !spc_sfx_fade_count+x,a     ; 22BB  ; not reached in the traces so far
    ret                               ; 22BE  ; not reached in the traces so far

; @routine 1B:82A4 SpcBIndexRight
; WHAT IT DOES: X + 8 (a channel's right-side index). The race driver's SpcIndexRight.
; HOW: Eight INC X.
; Entry: X. Exit: X + 8.
; @spc 22BF
%spc($22BF, $1B82A4)
SpcBIndexRight:
    inc x                             ; 22BF
    inc x                             ; 22C0
    inc x                             ; 22C1
    inc x                             ; 22C2
    inc x                             ; 22C3
    inc x                             ; 22C4
    inc x                             ; 22C5
    inc x                             ; 22C6
    ret                               ; 22C7

; @routine 1B:82AD SpcBIndexLeft
; WHAT IT DOES: X - 8. The race driver's SpcIndexLeft.
; HOW: Eight DEC X.
; Entry: X. Exit: X - 8.
; @spc 22C8
%spc($22C8, $1B82AD)
SpcBIndexLeft:
    dec x                             ; 22C8
    dec x                             ; 22C9
    dec x                             ; 22CA
    dec x                             ; 22CB
    dec x                             ; 22CC
    dec x                             ; 22CD
    dec x                             ; 22CE
    dec x                             ; 22CF
    ret                               ; 22D0

; @routine 1B:82B6 SpcBMul16
; WHAT IT DOES: A times the 16-bit value X:Y (X the high byte): YA = the result's middle 16 bits plus A * Y, as the sound code's fixed-point multiply. The race driver's SpcMul16.
; HOW: Two MULs and ADDW through $26/$27.
; Entry: A, X, Y. Exit: YA, X.
; @spc 22D1
%spc($22D1, $1B82B6)
SpcBMul16:
    push a                            ; 22D1
    mul ya                            ; 22D2
    mov.b !spc_math_tmp,a             ; 22D3
    mov.b !spc_math_tmp+1,y           ; 22D5
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
    addw ya,!spc_math_tmp             ; 22E2
    ret                               ; 22E4

; @routine 1B:82CA SpcBDiv16
; WHAT IT DOES: The 16-bit value Y:A divided by X: YA = the quotient. The race driver's SpcDiv16.
; HOW: Two DIVs, the high quotient through $27.
; Entry: Y:A, X. Exit: YA = quotient.
; @spc 22E5
%spc($22E5, $1B82CA)
SpcBDiv16:
    push a                            ; 22E5
    push y                            ; 22E6
    mov y,#$00                        ; 22E7
    pop a                             ; 22E9
    div ya,x                          ; 22EA
    mov.b !spc_math_tmp+1,a           ; 22EB
    pop a                             ; 22ED
    div ya,x                          ; 22EE
    mov.b y,!spc_math_tmp+1           ; 22EF
    ret                               ; 22F1

; @routine 1B:82D7 SpcBSfxVolumeAdjust
; WHAT IT DOES: For sound $25, adds $0530 * 4 (channel 5) or $0531 * 4 (channel 4) to a nonzero volume A; the levels come from port 1 commands $32-$39. The race driver's SpcSfxVolumeAdjust has $0530 only.
; HOW: MUL, ADC.
; Entry: A = volume, X = channel. Exit: A = volume; Y kept.
; @spc 22F2
%spc($22F2, $1B82D7)
SpcBSfxVolumeAdjust:
    push y                            ; 22F2
    mov.b !spc_sfx_work,a             ; 22F3
    cmp a,#$00                        ; 22F5
    beq .l2318                        ; 22F7
    mov.w a,!spc_sfx_command+x        ; 22F9
    cmp a,#$25                        ; 22FC
    bne .l231A                        ; 22FE
    cmp x,#$05                        ; 2300
    beq .l230A                        ; 2302
    cmp x,#$04                        ; 2304  ; not reached in the traces so far
    beq .l230F                        ; 2306  ; not reached in the traces so far
    bra .l2318                        ; 2308  ; not reached in the traces so far
.l230A:
    mov.w a,!spc_sfx_volume_add       ; 230A
    bra .l2312                        ; 230D
.l230F:
    mov.w a,!spcb_sfx_volume_add_4    ; 230F  ; not reached in the traces so far
.l2312:
    mov y,#$04                        ; 2312
    mul ya                            ; 2314
    clrc                              ; 2315
    adc.b a,!spc_sfx_work             ; 2316
.l2318:
    pop y                             ; 2318
    ret                               ; 2319
.l231A:
    mov.b a,!spc_sfx_work             ; 231A
    bra .l2318                        ; 231C

; @routine 1B:8303 SpcBSfxSpecial
; WHAT IT DOES: When channel X plays a sound in the list at $233A ($77-$7A, $17, $25), runs its handler from the 6-entry table at $2341: for $77-$7A the engine pitch (channel 7 from port 3, channel 6 from port 2, with the sound's curve from the tables at $234D-$235C): a change of speed starts a slew ($0532-$053C) toward the new pitch, full speed ($F0 and up) drifts it up ($056C/$056E), and the pitch ramps' offsets for that side are added, clamped to $3FFF, as the DSP pitch $03B8/$03C0; the volume follows (SpcBEngineVolume). $17 and $25 are the rev timing (in SpcBEngineDamp). As the race driver's SpcSfxSpecial with a channel per side.
; HOW: JMP ($2341+X) with X pushed; Y = 1 for port 3, 0 for port 2.
; Entry: X = channel. Exit: as the handler leaves it.
; @spc 231E
%spc($231E, $1B8303)
SpcBSfxSpecial:
    mov.w a,!spc_sfx_command+x        ; 231E
    mov.b !spc_sfx_tmp,a              ; 2321
    push x                            ; 2323
    mov x,#$00                        ; 2324
.l2326:
    mov.w a,.t233A+x                  ; 2326
    beq .l2332                        ; 2329
    cmp.b a,!spc_sfx_tmp              ; 232B
    beq .l2334                        ; 232D
    inc x                             ; 232F
    bra .l2326                        ; 2330
.l2332:
    pop x                             ; 2332
    ret                               ; 2333
.l2334:
    mov a,x                           ; 2334
    asl a                             ; 2335
    mov x,a                           ; 2336
    jmp (.t2341+x)                    ; 2337
.t233A:
    db $79, $78, $77, $7A, $17, $25, $00
.t2341:
    db $5D, $23, $5D, $23, $5D, $23, $5D, $23
    db $45, $25, $3A, $25
.t234D:
    db $0C, $13, $55, $7F
.t2351:
    db $23, $1A, $17, $17
.t2355:
    db $0C, $0B, $0E, $07
.t2359:
    db $1B, $1E, $1F, $12
    lsr a                             ; 235D
    mov x,a                           ; 235E
    mov.w a,.t234D+x                  ; 235F
    mov.b !spc_sfx_work,a             ; 2362
    mov.w a,.t2351+x                  ; 2364
    mov.b !spc_sfx_work+1,a           ; 2367
    mov.w a,.t2355+x                  ; 2369
    mov.b !spc_sfx_data,a             ; 236C
    mov.w a,.t2359+x                  ; 236E
    mov.b !spc_sfx_data+1,a           ; 2371
    pop x                             ; 2373
    cmp x,#$07                        ; 2374
    beq .l237D                        ; 2376
    cmp x,#$06                        ; 2378
    beq .l2381                        ; 237A
    ret                               ; 237C  ; not reached in the traces so far
.l237D:
    mov y,#$01                        ; 237D
    bra .l2383                        ; 237F
.l2381:
    mov y,#$00                        ; 2381
.l2383:
    call SpcBEngineVolume             ; 2383
    push x                            ; 2386
    mov a,y                           ; 2387
    mov x,a                           ; 2388
    mov.w a,!spc_port_in+2+x          ; 2389
    cmp.w a,!spcb_port2_last+x        ; 238C
    beq .l23DF                        ; 238F
    mov a,#$00                        ; 2391
    mov.w !spcb_engine_step+x,a       ; 2393
    mov.w a,!spc_port_in+2+x          ; 2396
    mov.w !spcb_engine_speed+x,a      ; 2399
    setc                              ; 239C
    sbc.w a,!spcb_port2_last+x        ; 239D
    bmi .l23D4                        ; 23A0
    push a                            ; 23A2
    mov a,#$01                        ; 23A3
    mov.w !spcb_engine_direction+x,a  ; 23A5
.l23A8:
    pop a                             ; 23A8
    mov y,a                           ; 23A9
    mov.w a,!spcb_engine_rising+x     ; 23AA
    push x                            ; 23AD
    mov x,a                           ; 23AE
    bne .l23B3                        ; 23AF
    mov x,#$01                        ; 23B1  ; not reached in the traces so far
.l23B3:
    mov a,#$00                        ; 23B3
    div ya,x                          ; 23B5
    bvs .l23C3                        ; 23B6
    pop x                             ; 23B8
    mov.w !spcb_engine_quotient+x,a   ; 23B9
    mov a,#$00                        ; 23BC
    mov.w !spcb_engine_overflow+x,a   ; 23BE
    bra .l23CC                        ; 23C1
.l23C3:
    pop x                             ; 23C3
    mov.w !spcb_engine_quotient+x,a   ; 23C4
    mov a,#$01                        ; 23C7
    mov.w !spcb_engine_overflow+x,a   ; 23C9
.l23CC:
    mov a,#$00                        ; 23CC
    mov.w !spcb_engine_rising+x,a     ; 23CE
    jmp .l248B                        ; 23D1
.l23D4:
    eor a,#$FF                        ; 23D4
    inc a                             ; 23D6
    push a                            ; 23D7
    mov a,#$02                        ; 23D8
    mov.w !spcb_engine_direction+x,a  ; 23DA
    bra .l23A8                        ; 23DD
.l23DF:
    mov.w a,!spc_port_in+2+x          ; 23DF
    bne .l23EE                        ; 23E2
    mov a,#$00                        ; 23E4
    mov.w !spcb_engine_quotient+x,a   ; 23E6
    mov.w !spcb_engine_overflow+x,a   ; 23E9
    bra .l2414                        ; 23EC
.l23EE:
    mov.w a,!spc_port_in+2+x          ; 23EE
    bpl .l2414                        ; 23F1
    setc                              ; 23F3
    sbc a,#$F0                        ; 23F4
    bmi .l2414                        ; 23F6
    mov.w a,!spcb_engine_table+x      ; 23F8
    mov.b !spc_sfx_work,a             ; 23FB
    mov.w a,!spcb_engine_table_hi+x   ; 23FD
    mov.b !spc_sfx_work+1,a           ; 2400
    incw !spc_sfx_work                ; 2402
    incw !spc_sfx_work                ; 2404
    incw !spc_sfx_work                ; 2406
    mov.b a,!spc_sfx_work             ; 2408
    mov.w !spcb_engine_table+x,a      ; 240A
    mov.b a,!spc_sfx_work+1           ; 240D
    mov.w !spcb_engine_table_hi+x,a   ; 240F
    bra .l241C                        ; 2412
.l2414:
    mov a,#$00                        ; 2414
    mov.w !spcb_engine_table+x,a      ; 2416
    mov.w !spcb_engine_table_hi+x,a   ; 2419
.l241C:
    mov.w a,!spcb_engine_rising+x     ; 241C
    setc                              ; 241F
    sbc a,#$0A                        ; 2420
    bmi .l2440                        ; 2422
    mov !spc_sfx_work,#$01            ; 2424
    mov !spc_sfx_work+1,#$00          ; 2427
    mov.w a,!spcb_engine_overflow+x   ; 242A
    mov y,a                           ; 242D
    mov.w a,!spcb_engine_quotient+x   ; 242E
    subw ya,!spc_sfx_work             ; 2431
    bpl .l2439                        ; 2433
    mov a,#$00                        ; 2435
    mov y,#$00                        ; 2437
.l2439:
    mov.w !spcb_engine_quotient+x,a   ; 2439
    mov a,y                           ; 243C
    mov.w !spcb_engine_overflow+x,a   ; 243D
.l2440:
    mov.w a,!spcb_engine_rising+x     ; 2440
    cmp a,#$FF                        ; 2443
    beq .l244E                        ; 2445
    mov.w a,!spcb_engine_rising+x     ; 2447
    inc a                             ; 244A
    mov.w !spcb_engine_rising+x,a     ; 244B
.l244E:
    mov.w a,!spcb_engine_rising+x     ; 244E
    cmp a,#$01                        ; 2451
    beq .l2483                        ; 2453
    mov.w a,!spcb_engine_speed+x      ; 2455
    mov y,a                           ; 2458
    mov.w a,!spcb_engine_step+x       ; 2459
.l245C:
    push a                            ; 245C
    mov.w a,!spcb_engine_quotient+x   ; 245D
    mov.b !spc_sfx_tmp,a              ; 2460
    mov.w a,!spcb_engine_overflow+x   ; 2462
    mov.b !spc_sfx_tmp+1,a            ; 2465
    mov.w a,!spcb_engine_direction+x  ; 2467
    cmp a,#$02                        ; 246A
    beq .l247C                        ; 246C
    pop a                             ; 246E
    addw ya,!spc_sfx_tmp              ; 246F
    bcs .l248B                        ; 2471
.l2473:
    mov.w !spcb_engine_step+x,a       ; 2473
    mov a,y                           ; 2476
    mov.w !spcb_engine_speed+x,a      ; 2477
    bra .l248B                        ; 247A
.l247C:
    pop a                             ; 247C
    subw ya,!spc_sfx_tmp              ; 247D
    bcc .l248B                        ; 247F
    bra .l2473                        ; 2481
.l2483:
    mov.w a,!spc_port_in+2+x          ; 2483
    mov y,a                           ; 2486
    mov a,#$00                        ; 2487
    bra .l245C                        ; 2489
.l248B:
    mov !spc_sfx_tmp,#$00             ; 248B
    mov !spc_sfx_tmp+1,!spc_sfx_data  ; 248E
    mov.b y,!spc_sfx_data+1           ; 2491
    mov.w a,!spcb_engine_speed+x      ; 2493
    mul ya                            ; 2496
    mov !spc_sfx_tmp,#$00             ; 2497
    addw ya,!spc_sfx_tmp              ; 249A
    mov.b !spc_sfx_tmp,a              ; 249C
    mov.b !spc_sfx_tmp+1,y            ; 249E
    mov.w a,!spcb_ramp_a2_pitch_hi+x  ; 24A0
    mov y,a                           ; 24A3
    mov.w a,!spcb_ramp_a2_pitch+x     ; 24A4
    addw ya,!spc_sfx_tmp              ; 24A7
    mov.b !spc_sfx_tmp,a              ; 24A9
    mov.b !spc_sfx_tmp+1,y            ; 24AB
    mov.w a,!spcb_ramp_b2_pitch_hi+x  ; 24AD
    mov y,a                           ; 24B0
    mov.w a,!spcb_ramp_b2_pitch+x     ; 24B1
    addw ya,!spc_sfx_tmp              ; 24B4
    mov.b !spc_sfx_tmp,a              ; 24B6
    mov.b !spc_sfx_tmp+1,y            ; 24B8
    mov.w a,!spcb_engine_table_hi+x   ; 24BA
    mov y,a                           ; 24BD
    mov.w a,!spcb_engine_table+x      ; 24BE
    addw ya,!spc_sfx_tmp              ; 24C1
    mov.b !spc_sfx_tmp,a              ; 24C3
    mov.b !spc_sfx_tmp+1,y            ; 24C5
    setc                              ; 24C7
    sbc !spc_sfx_tmp+1,#$3F           ; 24C8
    bmi .l24D0                        ; 24CB
    mov !spc_sfx_tmp+1,#$3F           ; 24CD  ; not reached in the traces so far
.l24D0:
    mov.w a,!spc_port_in+2+x          ; 24D0
    mov.w !spcb_port2_last+x,a        ; 24D3
    pop x                             ; 24D6
    mov.b a,!spc_sfx_tmp              ; 24D7
    mov.w !spc_sfx_dsp_pitch+x,a      ; 24D9
    mov.b a,!spc_sfx_tmp+1            ; 24DC
    mov.w !spc_sfx_dsp_pitch_hi+x,a   ; 24DE
    ret                               ; 24E1

; @routine 1B:84C7 SpcBEngineVolume
; WHAT IT DOES: The engine sound's volume from port 3 (Y = 1) or port 2 (Y = 0): (inverted speed) / $22 + $23, plus 3 with bit 2 (or 4) of $29, by $D0 unless bit 2 (or 4) of $28 (SpcBEngineDamp, SpcBEngineDamp2), by $056B; port 3's on the left only, port 2's on the right. The race driver's SpcEngineVolume sets both sides.
; HOW: DIV and MULs keeping high bytes.
; Entry: X = channel, Y = port - 2, $22/$23 = curve. Exit: A = volume.
; @spc 24E2
%spc($24E2, $1B84C7)
SpcBEngineVolume:
    mov.w a,!spc_port_in+2+y          ; 24E2
    eor a,#$FF                        ; 24E5
    push y                            ; 24E7
    mov y,#$00                        ; 24E8
    push x                            ; 24EA
    mov.b x,!spc_sfx_work             ; 24EB
    div ya,x                          ; 24ED
    mov.b !spc_sfx_tmp,a              ; 24EE
    pop x                             ; 24F0
    pop y                             ; 24F1
    mov.b a,!spc_sfx_work+1           ; 24F2
    clrc                              ; 24F4
    adc.b a,!spc_sfx_tmp              ; 24F5
    cmp y,#$00                        ; 24F7
    beq .l2513                        ; 24F9
    bbc !spc_flags.2,.l2500           ; 24FB
    adc a,#$03                        ; 24FE
.l2500:
    call SpcBEngineDamp               ; 2500
    push y                            ; 2503
    mov.w y,!spc_song5_scale          ; 2504
    mul ya                            ; 2507
    mov a,y                           ; 2508
    pop y                             ; 2509
    mov.w !spc_sfx_vol_l+x,a          ; 250A
    mov a,#$00                        ; 250D
    mov.w !spc_sfx_vol_r+x,a          ; 250F
    ret                               ; 2512
.l2513:
    bbc !spc_flags.4,.l2518           ; 2513
    adc a,#$03                        ; 2516
.l2518:
    call SpcBEngineDamp2              ; 2518
    push y                            ; 251B
    mov.w y,!spc_song5_scale          ; 251C
    mul ya                            ; 251F
    mov a,y                           ; 2520
    pop y                             ; 2521
    mov.w !spc_sfx_vol_r+x,a          ; 2522
    mov a,#$00                        ; 2525
    mov.w !spc_sfx_vol_l+x,a          ; 2527
    ret                               ; 252A

; @routine 1B:8510 SpcBEngineDamp2
; WHAT IT DOES: Unless bit 4 of $28 is set, A = A * $D0 / 256 (the port 2 side of SpcBEngineDamp).
; HOW: Branches into SpcBEngineDamp's code.
; Entry: A. Exit: A; Y kept.
; @spc 252B
%spc($252B, $1B8510)
SpcBEngineDamp2:
    bbs !spc_effect_flags.4,SpcBEngineDamp_l2539; 252B
    bra SpcBEngineDamp_l2533          ; 252E

; @routine 1B:8515 SpcBEngineDamp
; WHAT IT DOES: Unless bit 2 of $28 is set, A = A * $D0 / 256 (the race driver's SpcEngineDamp). After it, SpcBSfxSpecial's handlers for sounds $25 and $17, the rev timing: channel 5 by port 3, channel 4 by port 2, the counter $0544 counts up to (inverted speed) / $22 + $23 and restarts the sound's note ($03E8 = $24).
; HOW: MUL; the handlers take their curve ($22-$24) and share the count.
; Entry: A. Exit: A; Y kept.
; @spc 2530
%spc($2530, $1B8515)
SpcBEngineDamp:
    bbs !spc_effect_flags.2,.l2539    ; 2530
.l2533:
    push y                            ; 2533
    mov y,#$D0                        ; 2534
    mul ya                            ; 2536
    mov a,y                           ; 2537
    pop y                             ; 2538
.l2539:
    ret                               ; 2539
    mov !spc_sfx_work,#$1E            ; 253A
    mov !spc_sfx_work+1,#$03          ; 253D
    mov !spc_sfx_data,#$01            ; 2540
    bra .l254E                        ; 2543
    mov !spc_sfx_work,#$14            ; 2545
    mov !spc_sfx_work+1,#$0A          ; 2548
    mov !spc_sfx_data,#$02            ; 254B
.l254E:
    pop x                             ; 254E
    cmp x,#$05                        ; 254F
    beq .l2558                        ; 2551
    cmp x,#$04                        ; 2553  ; not reached in the traces so far
    beq .l255C                        ; 2555  ; not reached in the traces so far
    ret                               ; 2557  ; not reached in the traces so far
.l2558:
    mov y,#$01                        ; 2558
    bra .l255E                        ; 255A
.l255C:
    mov y,#$00                        ; 255C  ; not reached in the traces so far
.l255E:
    mov.w a,!spc_port_in+2+y          ; 255E
    eor a,#$FF                        ; 2561
    push y                            ; 2563
    mov y,#$00                        ; 2564
    push x                            ; 2566
    mov.b x,!spc_sfx_work             ; 2567
    div ya,x                          ; 2569
    pop x                             ; 256A
    pop y                             ; 256B
    clrc                              ; 256C
    adc.b a,!spc_sfx_work+1           ; 256D
    push a                            ; 256F
    mov.w a,!spcb_rev_counter+y       ; 2570
    inc a                             ; 2573
    mov.w !spcb_rev_counter+y,a       ; 2574
    pop a                             ; 2577
    setc                              ; 2578
    sbc.w a,!spcb_rev_counter+y       ; 2579
    bpl .l2587                        ; 257C
    mov.b a,!spc_sfx_data             ; 257E
    mov.w !spc_sfx_key_request+x,a    ; 2580
    dec a                             ; 2583
    mov.w !spcb_rev_counter+y,a       ; 2584
.l2587:
    ret                               ; 2587

; @routine 1B:856D SpcBNoiseSampleInit
; WHAT IT DOES: Builds the sample at $FF00, as the race driver's SpcNoiseSampleInit: every ninth byte (the BRR block headers) set to A for $1B blocks, the last A + 1 (the end flag); the sample directory's entry at $3F8C-$3F8F points start and loop at $FF00; the LFSR made nonzero.
; HOW: Y stepping by 9.
; Entry: A = header byte. Exit: A, Y changed.
; @spc 2588
%spc($2588, $1B856D)
SpcBNoiseSampleInit:
    mov y,#$00                        ; 2588  ; not reached in the traces so far
    mov x,#$1B                        ; 258A  ; not reached in the traces so far
.l258C:
    mov.w !spc_noise_sample+y,a       ; 258C  ; not reached in the traces so far
    inc y                             ; 258F  ; not reached in the traces so far
    inc y                             ; 2590  ; not reached in the traces so far
    inc y                             ; 2591  ; not reached in the traces so far
    inc y                             ; 2592  ; not reached in the traces so far
    inc y                             ; 2593  ; not reached in the traces so far
    inc y                             ; 2594  ; not reached in the traces so far
    inc y                             ; 2595  ; not reached in the traces so far
    inc y                             ; 2596  ; not reached in the traces so far
    inc y                             ; 2597  ; not reached in the traces so far
    dec x                             ; 2598  ; not reached in the traces so far
    bne .l258C                        ; 2599  ; not reached in the traces so far
    inc a                             ; 259B  ; not reached in the traces so far
    mov.w !spc_noise_sample+y,a       ; 259C  ; not reached in the traces so far
    mov y,#$FF                        ; 259F  ; not reached in the traces so far
    mov a,#$00                        ; 25A1  ; not reached in the traces so far
    mov.w !spc_noise_dir,a            ; 25A3  ; not reached in the traces so far
    mov.w !spc_noise_dir+1,y          ; 25A6  ; not reached in the traces so far
    mov.w !spc_noise_dir+2,a          ; 25A9  ; not reached in the traces so far
    mov.w !spc_noise_dir+3,y          ; 25AC  ; not reached in the traces so far
    mov.b a,!spc_random               ; 25AF  ; not reached in the traces so far
    or.b a,!spc_random+1              ; 25B1  ; not reached in the traces so far
    bne .l25B7                        ; 25B3  ; not reached in the traces so far
    inc.b !spc_random                 ; 25B5  ; not reached in the traces so far
.l25B7:
    ret                               ; 25B7

; @routine 1B:859D SpcBNoiseSample
; WHAT IT DOES: While a channel 0-6 plays sound $23, rewrites part of the sample at $FF00 each tick with noise from the LFSR $18/$19: seven blocks of eight bytes, the block chosen by the counter $0563 (the race driver's SpcNoiseSample checks channels 7-1). After it, SpcBCommandChannelCheck's lists at $25FF and $2608.
; HOW: MOV1/EOR1 of two LFSR bits, ROL; bytes ORed with $11.
; Entry: none. Exit: X, Y changed.
; @spc 25B8
%spc($25B8, $1B859D)
SpcBNoiseSample:
    mov x,#$00                        ; 25B8
.l25BA:
    mov.w a,!spc_sfx_number+x         ; 25BA
    cmp a,#$23                        ; 25BD
    beq .l25C8                        ; 25BF
    inc x                             ; 25C1
    cmp x,#$07                        ; 25C2
    beq SpcBNoiseSampleInit_l25B7     ; 25C4
    bra .l25BA                        ; 25C6
.l25C8:
    dec.w !spc_noise_block            ; 25C8  ; not reached in the traces so far
    mov.w a,!spc_noise_block          ; 25CB  ; not reached in the traces so far
    and a,#$03                        ; 25CE  ; not reached in the traces so far
    mov y,#$3F                        ; 25D0  ; not reached in the traces so far
    mul ya                            ; 25D2  ; not reached in the traces so far
    mov y,a                           ; 25D3  ; not reached in the traces so far
    mov !spc_sfx_tmp,#$07             ; 25D4  ; not reached in the traces so far
.l25D7:
    inc y                             ; 25D7  ; not reached in the traces so far
    mov x,#$04                        ; 25D8  ; not reached in the traces so far
.l25DA:
    mov1 c,!spc_random+1.6            ; 25DA  ; not reached in the traces so far
    eor1 c,!spc_random+1.5            ; 25DD  ; not reached in the traces so far
    rol.b !spc_random                 ; 25E0  ; not reached in the traces so far
    rol.b !spc_random+1               ; 25E2  ; not reached in the traces so far
    mov.b a,!spc_random+1             ; 25E4  ; not reached in the traces so far
    and a,#$D0                        ; 25E6  ; not reached in the traces so far
    or a,#$11                         ; 25E8  ; not reached in the traces so far
    mov.w !spc_noise_sample+y,a       ; 25EA  ; not reached in the traces so far
    inc y                             ; 25ED  ; not reached in the traces so far
    mov.b a,!spc_random               ; 25EE  ; not reached in the traces so far
    and a,#$FF                        ; 25F0  ; not reached in the traces so far
    or a,#$11                         ; 25F2  ; not reached in the traces so far
    mov.w !spc_noise_sample+y,a       ; 25F4  ; not reached in the traces so far
    inc y                             ; 25F7  ; not reached in the traces so far
    dec x                             ; 25F8  ; not reached in the traces so far
    bne .l25DA                        ; 25F9  ; not reached in the traces so far
    dbnz !spc_sfx_tmp,.l25D7          ; 25FB  ; not reached in the traces so far
    ret                               ; 25FE  ; not reached in the traces so far
.t25FF:
    db $03, $10, $32, $30, $3C, $16, $20, $56
    db $00
.t2608:
    db $30, $3C, $38, $16, $20, $56, $00
%spc_end()
