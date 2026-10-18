; spc_race_driver.asm: the race sound driver, Nintendo's N-SPC engine as
; uploaded by ApuUploadBank19 (block list $19:C27C), being filed routine by
; routine; routines not yet described carry plain labels.

incsrc "../include/macros.inc"
incsrc "../include/spc.inc"

%spc($0670, $19C2FC)
    db $94, $04, $26, $08, $53

SpcReload:
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
    mov (!spc_ptr)+y,a                ; 0698
    inc y                             ; 069A
    bne .l068D                        ; 069B
    inc.b !spc_ptr+1                  ; 069D
    bra .l068D                        ; 069F
.l06A1:
    bpl .l068D                        ; 06A1
    cmp.w y,!spc_cpuio0               ; 06A3
    bpl .l068D                        ; 06A6
.l06A8:
    mov.w a,!spc_cpuio2               ; 06A8
    mov.w y,!spc_cpuio3               ; 06AB
    movw !spc_ptr,ya                  ; 06AE
    mov.w y,!spc_cpuio0               ; 06B0
    mov.w a,!spc_cpuio1               ; 06B3
    mov.w !spc_cpuio0,y               ; 06B6
    bne .l0688                        ; 06B9
    mov x,#$31                        ; 06BB
    mov.w !spc_control,x              ; 06BD
    jmp SpcReset                      ; 06C0
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

; @routine 19:C48C SpcReset
; WHAT IT DOES: The driver's start: the stack at $01CF, direct page $00-$DF cleared, the echo delay set to 1 (SpcSetEchoDelay), RAM $0200-$058F cleared, every voice's pitch high byte 1, the master volumes MVOLL/MVOLR $60 and the sample directory DIR at $3F00; the ports cleared, timer 0 at $10 (the tempo $53 too), the port bytes $04-$07 zero and every voice keyed off. Then the main loop forever: the DSP registers in the list at $3F9F written from the RAM bytes the list at $06C2 names (the echo ones only once the echo delay has settled), KON/KOF cleared, the noise LFSR $18/$19 stepped, a wait for timer 0, then the sound effects (SpcSfxFrame) when $43 + ticks * $20 carries, and the music (SpcMusicFrame) with the four port exchanges (SpcPortExchange) when $51 + ticks * tempo carries; otherwise, with a song playing, the between-tick work for each voice with a track (SpcVoiceBetweenTicks).
; HOW: Runs at SPC $0800 (the block lists' start address). Clears by MOV (X+),A and indexed loops; DSP writes through SpcDspWrite. The LFSR: EOR of the two bytes, two LSRs, NOTC, ROR of both. The echo settle count $4C steps on each sound effect tick until it reaches $4D.
; Entry: from the uploader (SpcReload or the IPL ROM). Exit: never returns.
; @spc 0800
%spc($0800, $19C48C)
SpcReset:
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
    call SpcSetEchoDelay              ; 080D
    mov a,#$00                        ; 0810
    mov x,a                           ; 0812
.l0813:
    mov.w !spc_note_length+x,a        ; 0813
    inc x                             ; 0816
    bne .l0813                        ; 0817
.l0819:
    mov.w !spc_volume+x,a             ; 0819
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
    mov.w !spc_pitch+1+x,a            ; 082F
    inc x                             ; 0832
    inc x                             ; 0833
    cmp x,#$10                        ; 0834
    bne .l082F                        ; 0836
    mov a,#$60                        ; 0838
    mov y,#$0C                        ; 083A
    call SpcDspWrite                  ; 083C
    mov y,#$1C                        ; 083F
    call SpcDspWrite                  ; 0841
    mov a,#$3F                        ; 0844
    mov y,#$5D                        ; 0846
    call SpcDspWrite                  ; 0848
    mov a,#$30                        ; 084B
    mov.w !spc_control,a              ; 084D
    mov a,#$10                        ; 0850
    mov.w !spc_t0target,a             ; 0852
    mov.b !spc_tempo+1,a              ; 0855
    mov a,#$01                        ; 0857
    mov.w !spc_control,a              ; 0859
    dec a                             ; 085C
    mov.b !spc_port_out,a             ; 085D
    mov.b $05,a                       ; 085F
    mov.b $06,a                       ; 0861
    mov.b $07,a                       ; 0863
    dec a                             ; 0865
    mov.b !spc_key_off,a              ; 0866
.l0868:
    mov y,#$0A                        ; 0868
.l086A:
    cmp y,#$05                        ; 086A
    beq .l0875                        ; 086C
    bcs .l0878                        ; 086E
    cmp !spc_echo_delay_wait,!spc_echo_delay; 0870
    bne .l0886                        ; 0873
.l0875:
    bbs !spc_echo_delay_wait.7,.l0886 ; 0875
.l0878:
    mov.w a,!spc_dsp_shadow_regs+y    ; 0878
    mov.w !spc_dspaddr,a              ; 087B
    mov.w a,!spc_dsp_shadow_src+y     ; 087E
    mov x,a                           ; 0881
    mov a,(x)                         ; 0882
    mov.w !spc_dspdata,a              ; 0883
.l0886:
    dbnz y,.l086A                     ; 0886
    mov.b !spc_key_on,y               ; 0888
    mov.b !spc_key_off,y              ; 088A
    mov.b a,!spc_random               ; 088C
    eor.b a,!spc_random+1             ; 088E
    lsr a                             ; 0890
    lsr a                             ; 0891
    notc                              ; 0892
    ror.b !spc_random                 ; 0893
    ror.b !spc_random+1               ; 0895
.l0897:
    mov.w y,!spc_t0out                ; 0897
    beq .l0897                        ; 089A
    push y                            ; 089C
    mov a,#$20                        ; 089D
    mul ya                            ; 089F
    clrc                              ; 08A0
    adc.b a,!spc_tempo_acc            ; 08A1
    mov.b !spc_tempo_acc,a            ; 08A3
    bcc .l08B1                        ; 08A5
    call SpcSfxFrame                  ; 08A7
    cmp !spc_echo_delay_wait,!spc_echo_delay; 08AA
    beq .l08B1                        ; 08AD
    inc.b !spc_echo_delay_wait        ; 08AF
.l08B1:
    mov.b a,!spc_tempo+1              ; 08B1
    pop y                             ; 08B3
    mul ya                            ; 08B4
    clrc                              ; 08B5
    adc.b a,!spc_sfx_tempo_acc        ; 08B6
    mov.b !spc_sfx_tempo_acc,a        ; 08B8
    bcc .l08C9                        ; 08BA
    call SpcMusicFrame                ; 08BC
    mov x,#$03                        ; 08BF
.l08C1:
    call SpcPortExchange              ; 08C1
    dec x                             ; 08C4
    bpl .l08C1                        ; 08C5
    bra .l0868                        ; 08C7
.l08C9:
    mov.b a,!spc_song                 ; 08C9
    beq .l0868                        ; 08CB
    mov x,#$00                        ; 08CD
    mov !spc_voice_bit,#$01           ; 08CF
.l08D2:
    mov.b a,!spc_track_ptr+1+x        ; 08D2
    beq .l08D9                        ; 08D4
    call SpcVoiceBetweenTicks         ; 08D6
.l08D9:
    inc x                             ; 08D9
    inc x                             ; 08DA
    asl.b !spc_voice_bit              ; 08DB
    bne .l08D2                        ; 08DD
    jmp $0868                         ; 08DF

; @routine 19:C56E SpcPortExchange
; WHAT IT DOES: Port X's exchange with the S-CPU: the byte $04+X goes out on port X, then the port is read until two reads agree and kept at $0558+X.
; HOW: MOV to $F4+X, then a read-compare loop on $F4+X.
; Entry: X = port (0-3). Exit: A = the byte read; flags from the last compare.
; @spc 08E2
%spc($08E2, $19C56E)
SpcPortExchange:
    mov.b a,!spc_port_out+x           ; 08E2
    mov.w !spc_cpuio0+x,a             ; 08E4
.l08E7:
    mov.w a,!spc_cpuio0+x             ; 08E7
    cmp.w a,!spc_cpuio0+x             ; 08EA
    bne .l08E7                        ; 08ED
    mov.w !spc_port_latch+x,a         ; 08EF
.l08F2:
    ret                               ; 08F2

; @routine 19:C57F SpcNoteOn
; WHAT IT DOES: Plays note Y on voice X: percussion notes ($CA and up) first set their instrument (SpcSetInstrument) and play as note $24; ties and rests ($C8, $C9) and voices taken by a sound effect do nothing. Otherwise the pitch is the note plus the global and voice transposes, the tuning goes in, vibrato's phase restarts, vibrato, tremolo and their fades reset, the voice is marked for key-on and a volume update, a pitch envelope (VCMD $EB/$EC) starts if set, then the DSP pitch is written.
; HOW: The pitch is a note number with a fraction ($0360/$0361); the DSP pitch comes from the 12-entry table at $06CD by octave (DIV by $18) and interpolation, times the instrument's multiplier $0220/$0221, written to VxPITCH through SpcDspWriteVoice. The code after $095D is also the pitch write the vibrato and slide paths jump back to.
; Entry: X = voice * 2, $47 = the voice's bit, Y = note. Exit: A, Y as the last DSP write left them; X kept.
; @spc 08F3
%spc($08F3, $19C57F)
SpcNoteOn:
    cmp y,#$CA                        ; 08F3
    bcc .l08FC                        ; 08F5
    call SpcSetInstrument             ; 08F7
    mov y,#$A4                        ; 08FA
.l08FC:
    cmp y,#$C8                        ; 08FC
    bcs SpcPortExchange_l08F2         ; 08FE
    mov.b a,!spc_sfx_voices           ; 0900
    and.b a,!spc_voice_bit            ; 0902
    bne SpcPortExchange_l08F2         ; 0904
    mov a,y                           ; 0906
    and a,#$7F                        ; 0907
    clrc                              ; 0909
    adc.b a,!spc_transpose            ; 090A
    clrc                              ; 090C
    adc.w a,!spc_voice_transpose+x    ; 090D
    mov.w !spc_pitch+1+x,a            ; 0910
    mov.w a,!spc_tuning+x             ; 0913
    mov.w !spc_pitch+x,a              ; 0916
    mov.w a,!spc_vibrato_fade_len+x   ; 0919
    lsr a                             ; 091C
    mov a,#$00                        ; 091D
    ror a                             ; 091F
    mov.w !spc_vibrato_phase+x,a      ; 0920
    mov a,#$00                        ; 0923
    mov.b !spc_vibrato_wait+x,a       ; 0925
    mov.w !spc_vibrato_fade_step+x,a  ; 0927
    mov.w !spc_tremolo_phase+x,a      ; 092A
    mov.b !spc_tremolo_wait+x,a       ; 092D
    or !spc_volume_dirty,!spc_voice_bit; 092F
    or !spc_key_on,!spc_voice_bit     ; 0932
    mov.w a,!spc_env_steps+x          ; 0935
    mov.b !spc_slide_steps+x,a        ; 0938
    beq .l095A                        ; 093A
    mov.w a,!spc_env_delay+x          ; 093C  ; not reached in the traces so far
    mov.b !spc_slide_delay+x,a        ; 093F  ; not reached in the traces so far
    mov.w a,!spc_env_dir+x            ; 0941  ; not reached in the traces so far
    bne .l0950                        ; 0944  ; not reached in the traces so far
    mov.w a,!spc_pitch+1+x            ; 0946  ; not reached in the traces so far
    setc                              ; 0949  ; not reached in the traces so far
    sbc.w a,!spc_env_semitones+x      ; 094A  ; not reached in the traces so far
    mov.w !spc_pitch+1+x,a            ; 094D  ; not reached in the traces so far
.l0950:
    mov.w a,!spc_env_semitones+x      ; 0950  ; not reached in the traces so far
    clrc                              ; 0953  ; not reached in the traces so far
    adc.w a,!spc_pitch+1+x            ; 0954  ; not reached in the traces so far
    call SpcSlideToNote               ; 0957  ; not reached in the traces so far
.l095A:
    call SpcLoadPitch                 ; 095A
.l095D:
    mov y,#$00                        ; 095D
    mov.b a,!spc_tmp_hi               ; 095F
    setc                              ; 0961
    sbc a,#$34                        ; 0962
    bcs .l096F                        ; 0964
    mov.b a,!spc_tmp_hi               ; 0966
    setc                              ; 0968
    sbc a,#$13                        ; 0969
    bcs .l0973                        ; 096B
    dec y                             ; 096D
    asl a                             ; 096E
.l096F:
    addw ya,!spc_tmp_lo               ; 096F
    movw !spc_tmp_lo,ya               ; 0971
.l0973:
    push x                            ; 0973
    mov.b a,!spc_tmp_hi               ; 0974
    asl a                             ; 0976
    mov y,#$00                        ; 0977
    mov x,#$18                        ; 0979
    div ya,x                          ; 097B
    mov x,a                           ; 097C
    mov.w a,!spc_pitch_table+1+y      ; 097D
    mov.b !spc_ptr+1,a                ; 0980
    mov.w a,!spc_pitch_table+y        ; 0982
    mov.b !spc_ptr,a                  ; 0985
    mov.w a,!spc_pitch_table+3+y      ; 0987
    push a                            ; 098A
    mov.w a,!spc_pitch_table+2+y      ; 098B
    pop y                             ; 098E
    subw ya,!spc_ptr                  ; 098F
    mov.b y,!spc_tmp_lo               ; 0991
    mul ya                            ; 0993
    mov a,y                           ; 0994
    mov y,#$00                        ; 0995
    addw ya,!spc_ptr                  ; 0997
    mov.b !spc_ptr+1,y                ; 0999
    asl a                             ; 099B
    rol.b !spc_ptr+1                  ; 099C
    mov.b !spc_ptr,a                  ; 099E
    bra .l09A6                        ; 09A0
.l09A2:
    lsr.b !spc_ptr+1                  ; 09A2
    ror a                             ; 09A4
    inc x                             ; 09A5
.l09A6:
    cmp x,#$06                        ; 09A6
    bne .l09A2                        ; 09A8
    mov.b !spc_ptr,a                  ; 09AA
    pop x                             ; 09AC
    mov.w a,!spc_pitch_mult+x         ; 09AD
    mov.b y,!spc_ptr+1                ; 09B0
    mul ya                            ; 09B2
    movw !spc_tmp_word,ya             ; 09B3
    mov.w a,!spc_pitch_mult+x         ; 09B5
    mov.b y,!spc_ptr                  ; 09B8
    mul ya                            ; 09BA
    push y                            ; 09BB
    mov.w a,!spc_pitch_mult+1+x       ; 09BC
    mov.b y,!spc_ptr                  ; 09BF
    mul ya                            ; 09C1
    addw ya,!spc_tmp_word             ; 09C2
    movw !spc_tmp_word,ya             ; 09C4
    mov.w a,!spc_pitch_mult+1+x       ; 09C6
    mov.b y,!spc_ptr+1                ; 09C9
    mul ya                            ; 09CB
    mov y,a                           ; 09CC
    pop a                             ; 09CD
    addw ya,!spc_tmp_word             ; 09CE
    movw !spc_tmp_word,ya             ; 09D0
    mov a,x                           ; 09D2
    xcn a                             ; 09D3
    lsr a                             ; 09D4
    or a,#$02                         ; 09D5
    mov y,a                           ; 09D7
    mov.b a,!spc_tmp_word             ; 09D8
    call SpcDspWriteVoice             ; 09DA
    inc y                             ; 09DD
    mov.b a,!spc_tmp_word+1           ; 09DE

; @routine 19:C66C SpcDspWriteVoice
; WHAT IT DOES: Writes A to DSP register Y unless the current voice ($47) belongs to a sound effect ($1A).
; HOW: A tested against $1A on the stack, then as SpcDspWrite.
; Entry: Y = DSP register, A = value. Exit: A, Y kept.
; @spc 09E0
%spc($09E0, $19C66C)
SpcDspWriteVoice:
    push a                            ; 09E0
    mov.b a,!spc_voice_bit            ; 09E1
    and.b a,!spc_sfx_voices           ; 09E3
    pop a                             ; 09E5
    bne SpcDspWrite_l09EE             ; 09E6

; @routine 19:C674 SpcDspWrite
; WHAT IT DOES: Writes A to DSP register Y.
; HOW: $F2 = Y, $F3 = A.
; Entry: Y = DSP register, A = value. Exit: A, Y kept.
; @spc 09E8
%spc($09E8, $19C674)
SpcDspWrite:
    mov.w !spc_dspaddr,y              ; 09E8
    mov.w !spc_dspdata,a              ; 09EB
.l09EE:
    ret                               ; 09EE

SpcDspRead:
    mov.w !spc_dspaddr,y              ; 09EF  ; not reached in the traces so far
    mov.w a,!spc_dspdata              ; 09F2  ; not reached in the traces so far
    ret                               ; 09F5  ; not reached in the traces so far

; @routine 19:C682 SpcReadPatternWord
; WHAT IT DOES: Reads the next word of the song's pattern list at $40 and steps past it. The code that follows is reached by branches from SpcMusicFrame: the song start (the request $1B kept as $1C and cleared, the pattern list from the song table $07F4, a two-tick delay $0C, then every voice no sound effect holds keyed off), and the tracks' reset (instrument, volume $FF, pan $0A, vibrato and tremolo off, transposes and fades cleared, master volume $C0, tempo $20).
; HOW: MOV A,($40)+Y twice with INCW; the word in YA.
; Entry: none. Exit: YA = the word; Z, N from the high byte.
; @spc 09F6
%spc($09F6, $19C682)
SpcReadPatternWord:
    mov y,#$00                        ; 09F6
    mov a,(!spc_pattern_ptr)+y        ; 09F8
    incw !spc_pattern_ptr             ; 09FA
    push a                            ; 09FC
    mov a,(!spc_pattern_ptr)+y        ; 09FD
    incw !spc_pattern_ptr             ; 09FF
    mov y,a                           ; 0A01
    pop a                             ; 0A02
    ret                               ; 0A03
.l0A04:
    mov x,#$0E                        ; 0A04
    mov !spc_voice_bit,#$80           ; 0A06
.l0A09:
    mov a,#$FF                        ; 0A09
    mov.w !spc_volume+1+x,a           ; 0A0B
    mov a,#$0A                        ; 0A0E
    call SpcSetPan                    ; 0A10
    mov.w !spc_instrument+x,a         ; 0A13
    mov.w !spc_tuning+x,a             ; 0A16
    mov.w !spc_voice_transpose+x,a    ; 0A19
    mov.w !spc_env_steps+x,a          ; 0A1C
    mov.b !spc_vibrato_depth+x,a      ; 0A1F
    mov.b !spc_tremolo_depth+x,a      ; 0A21
    dec x                             ; 0A23
    dec x                             ; 0A24
    lsr.b !spc_voice_bit              ; 0A25
    bne .l0A09                        ; 0A27
    mov.b !spc_master_fade,a          ; 0A29
    mov.b !spc_echo_fade,a            ; 0A2B
    mov.b !spc_tempo_fade,a           ; 0A2D
    mov.b !spc_transpose,a            ; 0A2F
    mov.b !spc_pattern_loops,a        ; 0A31
    mov.b !spc_percussion_base,a      ; 0A33
    mov !spc_master_volume+1,#$C0     ; 0A35
    mov !spc_tempo+1,#$20             ; 0A38
.l0A3B:
    ret                               ; 0A3B
.l0A3C:
    bra SpcMusicPause                 ; 0A3C
.l0A3E:
    mov.w !spc_song_last,a            ; 0A3E
    mov.b a,!spc_song_request         ; 0A41
    beq SpcMusicFrame_l0A81           ; 0A43
    mov.b !spc_song,a                 ; 0A45
    mov !spc_song_request,#$00        ; 0A47
    asl a                             ; 0A4A
    mov x,a                           ; 0A4B
    mov.w a,!spc_song_table+1+x       ; 0A4C
    mov y,a                           ; 0A4F
    mov.w a,!spc_song_table+x         ; 0A50
    movw !spc_pattern_ptr,ya          ; 0A53
    mov $0C,#$02                      ; 0A55
.l0A58:
    mov.b a,!spc_sfx_voices           ; 0A58
    eor a,#$FF                        ; 0A5A
    tset !spc_key_off,a               ; 0A5C
    ret                               ; 0A5F

; @routine 19:C6EC SpcMusicPause
; WHAT IT DOES: Stops the music once: sets bit 1 of $29 (nothing if already set), clears the song request and the song, and keys off every voice no sound effect holds.
; HOW: BBS, SET1, then the release code in SpcReadPatternWord's tail (TSET of the free voices into the key-off mask $46).
; Entry: none. Exit: A = the free voices' mask.
; @spc 0A60
%spc($0A60, $19C6EC)
SpcMusicPause:
    bbs !spc_flags.1,SpcReadPatternWord_l0A3B; 0A60
    set1 !spc_flags.1                 ; 0A63
    mov !spc_song_request,#$00        ; 0A65
    mov !spc_song,#$00                ; 0A68
    bra SpcReadPatternWord_l0A58      ; 0A6B
.l0A6D:
    bra SpcReadPatternWord_l0A04      ; 0A6D

; @routine 19:C6FB SpcMusicFrame
; WHAT IT DOES: One tick of the music: with bit 0 of $29 set it only keys off the voices no sound effect holds. Otherwise a new song request (one not already playing, $0584) starts its song; the pattern list advances when the tracks end (a zero word ends the song, a count word loops), each track's commands are read (note lengths, gate and velocity bytes, voice commands through SpcVcmdDispatch, notes through SpcNoteOn), notes held count down (SpcVoiceTick), pitch slides are checked (SpcSlideCheck); then the tempo, echo volume and master volume fades step and every playing voice's fades and modulation are applied (SpcVoiceEffects).
; HOW: Track pointers at $30+X, timers $70/$71, subroutine counts $80; the gate and velocity come from the tables $0650 and $0658.
; Entry: none. Exit: A, X, Y changed.
; @spc 0A6F
%spc($0A6F, $19C6FB)
SpcMusicFrame:
    bbs !spc_flags.0,SpcReadPatternWord_l0A58; 0A6F
    mov.b a,!spc_song_request         ; 0A72
    beq .l0A7B                        ; 0A74
    cmp.w a,!spc_song_last            ; 0A76
    bne SpcReadPatternWord_l0A3E      ; 0A79
.l0A7B:
    mov.w !spc_song_last,a            ; 0A7B
    mov !spc_song_request,#$00        ; 0A7E
.l0A81:
    mov.b a,!spc_song                 ; 0A81
    beq SpcReadPatternWord_l0A3B      ; 0A83
    mov.b a,$0C                       ; 0A85
    beq .l0AD5                        ; 0A87
    dbnz $0C,SpcMusicPause_l0A6D      ; 0A89
.l0A8C:
    call SpcReadPatternWord           ; 0A8C
    bne .l0AA5                        ; 0A8F
    mov y,a                           ; 0A91
    beq SpcReadPatternWord_l0A3C      ; 0A92
    dec.b !spc_pattern_loops          ; 0A94
    bpl .l0A9A                        ; 0A96
    mov.b !spc_pattern_loops,a        ; 0A98
.l0A9A:
    call SpcReadPatternWord           ; 0A9A
    mov.b x,!spc_pattern_loops        ; 0A9D
    beq .l0A8C                        ; 0A9F
    movw !spc_pattern_ptr,ya          ; 0AA1
    bra .l0A8C                        ; 0AA3
.l0AA5:
    movw !spc_tmp_word,ya             ; 0AA5
    mov y,#$0F                        ; 0AA7
.l0AA9:
    mov a,(!spc_tmp_word)+y           ; 0AA9
    mov.w !spc_track_ptr+y,a          ; 0AAB
    dec y                             ; 0AAE
    bpl .l0AA9                        ; 0AAF
    mov x,#$00                        ; 0AB1
    mov !spc_voice_bit,#$01           ; 0AB3
.l0AB6:
    mov.b a,!spc_track_ptr+1+x        ; 0AB6
    beq .l0AC4                        ; 0AB8
    mov.w a,!spc_instrument+x         ; 0ABA
    bne .l0AC4                        ; 0ABD
    mov a,#$00                        ; 0ABF
    call SpcSetInstrument             ; 0AC1
.l0AC4:
    mov a,#$00                        ; 0AC4
    mov.b !spc_sub_count+x,a          ; 0AC6
    mov.b !spc_volume_fade+x,a        ; 0AC8
    mov.b !spc_pan_fade+x,a           ; 0ACA
    inc a                             ; 0ACC
    mov.b !spc_note_timer+x,a         ; 0ACD
    inc x                             ; 0ACF
    inc x                             ; 0AD0
    asl.b !spc_voice_bit              ; 0AD1
    bne .l0AB6                        ; 0AD3
.l0AD5:
    mov x,#$00                        ; 0AD5
    mov.b !spc_volume_dirty,x         ; 0AD7
    mov !spc_voice_bit,#$01           ; 0AD9
.l0ADC:
    mov.b !spc_voice_x,x              ; 0ADC
    mov.b a,!spc_track_ptr+1+x        ; 0ADE
    beq .l0B4F                        ; 0AE0
    dec.b !spc_note_timer+x           ; 0AE2
    bne .l0B49                        ; 0AE4
.l0AE6:
    call SpcTrackRead                 ; 0AE6
    bne .l0B06                        ; 0AE9
    mov.b a,!spc_sub_count+x          ; 0AEB
    beq .l0A8C                        ; 0AED
    call SpcSubroutineRestart         ; 0AEF
    dec.b !spc_sub_count+x            ; 0AF2
    bne .l0AE6                        ; 0AF4
    mov.w a,!spc_sub_return+x         ; 0AF6
    mov.b !spc_track_ptr+x,a          ; 0AF9
    mov.w a,!spc_sub_return+1+x       ; 0AFB
    mov.b !spc_track_ptr+1+x,a        ; 0AFE
    bra .l0AE6                        ; 0B00
.l0B02:
    clr1 !spc_track_flags.0           ; 0B02
    bra .l0A8C                        ; 0B04
.l0B06:
    bbs !spc_track_flags.0,.l0B02     ; 0B06
    bmi .l0B2B                        ; 0B09
    mov.w !spc_note_length+x,a        ; 0B0B
    call SpcTrackRead                 ; 0B0E
    bmi .l0B2B                        ; 0B11
    push a                            ; 0B13
    xcn a                             ; 0B14
    and a,#$07                        ; 0B15
    mov y,a                           ; 0B17
    mov.w a,!spc_gate_table+y         ; 0B18
    mov.w !spc_note_gate+x,a          ; 0B1B
    pop a                             ; 0B1E
    and a,#$0F                        ; 0B1F
    mov y,a                           ; 0B21
    mov.w a,!spc_velocity_table+y     ; 0B22
    mov.w !spc_note_velocity+x,a      ; 0B25
    call SpcTrackRead                 ; 0B28
.l0B2B:
    cmp a,#$E0                        ; 0B2B
    bcc .l0B34                        ; 0B2D
    call SpcVcmdDispatch              ; 0B2F
    bra .l0AE6                        ; 0B32
.l0B34:
    call SpcNoteOn                    ; 0B34
    mov.w a,!spc_note_length+x        ; 0B37
    mov.b !spc_note_timer+x,a         ; 0B3A
    mov y,a                           ; 0B3C
    mov.w a,!spc_note_gate+x          ; 0B3D
    mul ya                            ; 0B40
    mov a,y                           ; 0B41
    bne .l0B45                        ; 0B42
    inc a                             ; 0B44
.l0B45:
    mov.b !spc_note_keyoff+x,a        ; 0B45
    bra .l0B4C                        ; 0B47
.l0B49:
    call SpcVoiceTick                 ; 0B49
.l0B4C:
    call SpcSlideCheck                ; 0B4C
.l0B4F:
    inc x                             ; 0B4F
    inc x                             ; 0B50
    asl.b !spc_voice_bit              ; 0B51
    bne .l0ADC                        ; 0B53
    mov.b a,!spc_tempo_fade           ; 0B55
    beq .l0B64                        ; 0B57
    movw ya,!spc_tempo_delta          ; 0B59  ; not reached in the traces so far
    addw ya,!spc_tempo                ; 0B5B  ; not reached in the traces so far
    dbnz !spc_tempo_fade,.l0B62       ; 0B5D  ; not reached in the traces so far
    movw ya,!spc_tempo_fade           ; 0B60  ; not reached in the traces so far
.l0B62:
    movw !spc_tempo,ya                ; 0B62  ; not reached in the traces so far
.l0B64:
    mov.b a,!spc_echo_fade            ; 0B64
    beq .l0B7D                        ; 0B66
    movw ya,!spc_echo_delta_l         ; 0B68  ; not reached in the traces so far
    addw ya,!spc_echo_volume_l        ; 0B6A  ; not reached in the traces so far
    movw !spc_echo_volume_l,ya        ; 0B6C  ; not reached in the traces so far
    movw ya,!spc_echo_delta_r         ; 0B6E  ; not reached in the traces so far
    addw ya,!spc_echo_volume_r        ; 0B70  ; not reached in the traces so far
    dbnz !spc_echo_fade,.l0B7B        ; 0B72  ; not reached in the traces so far
    movw ya,!spc_echo_fade            ; 0B75  ; not reached in the traces so far
    movw !spc_echo_volume_l,ya        ; 0B77  ; not reached in the traces so far
    mov.b y,!spc_echo_target_r        ; 0B79  ; not reached in the traces so far
.l0B7B:
    movw !spc_echo_volume_r,ya        ; 0B7B  ; not reached in the traces so far
.l0B7D:
    mov.b a,!spc_master_fade          ; 0B7D
    beq .l0B8F                        ; 0B7F
    movw ya,!spc_master_delta         ; 0B81  ; not reached in the traces so far
    addw ya,!spc_master_volume        ; 0B83  ; not reached in the traces so far
    dbnz !spc_master_fade,.l0B8A      ; 0B85  ; not reached in the traces so far
    movw ya,!spc_master_fade          ; 0B88  ; not reached in the traces so far
.l0B8A:
    movw !spc_master_volume,ya        ; 0B8A  ; not reached in the traces so far
    mov !spc_volume_dirty,#$FF        ; 0B8C  ; not reached in the traces so far
.l0B8F:
    mov x,#$00                        ; 0B8F
    mov !spc_voice_bit,#$01           ; 0B91
.l0B94:
    mov.b a,!spc_track_ptr+1+x        ; 0B94
    beq .l0B9B                        ; 0B96
    call SpcVoiceEffects              ; 0B98
.l0B9B:
    inc x                             ; 0B9B
    inc x                             ; 0B9C
    asl.b !spc_voice_bit              ; 0B9D
    bne .l0B94                        ; 0B9F
    ret                               ; 0BA1

; @routine 19:C82E SpcVcmdDispatch
; WHAT IT DOES: Runs voice command A ($E0-$FA) for voice X: its handler from the table at $3F0A (by the command), with its first parameter byte read when the parameter count at $102F says there is one.
; HOW: The handler's address minus one is pushed, then the RET at the end of SpcTrackRead (or SpcTrackAdvance's tail) enters it; the handlers follow SpcSetPan's code (SpcVcmdPanFade and on).
; Entry: A = command, X = voice * 2. Exit: as the handler leaves it.
; @spc 0BA2
%spc($0BA2, $19C82E)
SpcVcmdDispatch:
    asl a                             ; 0BA2
    mov y,a                           ; 0BA3
    mov.w a,!spc_vcmd_handlers+1+y    ; 0BA4
    push a                            ; 0BA7
    mov.w a,!spc_vcmd_handlers+y      ; 0BA8
    push a                            ; 0BAB
    mov a,y                           ; 0BAC
    lsr a                             ; 0BAD
    mov y,a                           ; 0BAE
    mov.w a,!spc_vcmd_param_table+y   ; 0BAF
    beq SpcTrackAdvance_l0BBC         ; 0BB2

; @routine 19:C840 SpcTrackRead
; WHAT IT DOES: Reads the next byte of voice X's track and steps the pointer.
; HOW: MOV A,($30+X), then SpcTrackAdvance.
; Entry: X = voice * 2. Exit: A = Y = the byte; N, Z from it.
; @spc 0BB4
%spc($0BB4, $19C840)
SpcTrackRead:
    mov a,(!spc_track_ptr+x)          ; 0BB4

; @routine 19:C842 SpcTrackAdvance
; WHAT IT DOES: Steps voice X's track pointer by one, keeping A.
; HOW: INC of the pointer's low byte, carrying into the high byte.
; Entry: X = voice * 2. Exit: Y = A.
; @spc 0BB6
%spc($0BB6, $19C842)
SpcTrackAdvance:
    inc.b !spc_track_ptr+x            ; 0BB6
    bne .l0BBC                        ; 0BB8
    inc.b !spc_track_ptr+1+x          ; 0BBA
.l0BBC:
    mov y,a                           ; 0BBC
    ret                               ; 0BBD

; @routine 19:C84A SpcSetInstrument
; WHAT IT DOES: Voice command $E0: voice X's instrument A is kept at $0211+X and loaded (SpcLoadInstrument).
; HOW: Falls into SpcLoadInstrument.
; Entry: A = instrument, X = voice * 2. Exit: as SpcLoadInstrument.
; @spc 0BBE
%spc($0BBE, $19C84A)
SpcSetInstrument:
    mov.w !spc_instrument+x,a         ; 0BBE

; @routine 19:C84D SpcLoadInstrument
; WHAT IT DOES: Loads instrument A into voice X: its six bytes at $0590 + 6 * A (percussion instruments from $CA on map from the percussion base $5F): the sample number, ADSR and gain to the voice's DSP registers (a sample number with bit 7 set selects noise with its low five bits as the noise clock instead), and the pitch multiplier to $0220/$0221. Nothing for a voice a sound effect holds.
; HOW: The DSP registers VxSRCN-VxGAIN by an index loop; noise through $48 (FLG) and $49.
; Entry: A = instrument, X = voice * 2, $47 = its bit. Exit: X kept.
; @spc 0BC1
%spc($0BC1, $19C84D)
SpcLoadInstrument:
    mov y,a                           ; 0BC1
    bpl .l0BCA                        ; 0BC2
    setc                              ; 0BC4
    sbc a,#$CA                        ; 0BC5
    clrc                              ; 0BC7
    adc.b a,!spc_percussion_base      ; 0BC8
.l0BCA:
    mov y,#$06                        ; 0BCA
    mul ya                            ; 0BCC
    movw !spc_ptr,ya                  ; 0BCD
    clrc                              ; 0BCF
    adc !spc_ptr,#$90                 ; 0BD0
    adc !spc_ptr+1,#$05               ; 0BD3
    mov.b a,!spc_sfx_voices           ; 0BD6
    and.b a,!spc_voice_bit            ; 0BD8
    bne .l0C16                        ; 0BDA
    push x                            ; 0BDC
    mov a,x                           ; 0BDD
    xcn a                             ; 0BDE
    lsr a                             ; 0BDF
    or a,#$04                         ; 0BE0
    mov x,a                           ; 0BE2
    mov y,#$00                        ; 0BE3
    mov a,(!spc_ptr)+y                ; 0BE5
    bpl .l0BF7                        ; 0BE7
    and a,#$1F                        ; 0BE9  ; not reached in the traces so far
    and !spc_flg,#$20                 ; 0BEB  ; not reached in the traces so far
    tset !spc_flg,a                   ; 0BEE  ; not reached in the traces so far
    or !spc_noise_voices,!spc_voice_bit; 0BF1  ; not reached in the traces so far
    mov a,y                           ; 0BF4  ; not reached in the traces so far
    bra .l0BFE                        ; 0BF5  ; not reached in the traces so far
.l0BF7:
    mov.b a,!spc_voice_bit            ; 0BF7
    tclr !spc_noise_voices,a          ; 0BF9
.l0BFC:
    mov a,(!spc_ptr)+y                ; 0BFC
.l0BFE:
    mov.w !spc_dspaddr,x              ; 0BFE
    mov.w !spc_dspdata,a              ; 0C01
    inc x                             ; 0C04
    inc y                             ; 0C05
    cmp y,#$04                        ; 0C06
    bne .l0BFC                        ; 0C08
    pop x                             ; 0C0A
    mov a,(!spc_ptr)+y                ; 0C0B
    mov.w !spc_pitch_mult+1+x,a       ; 0C0D
    inc y                             ; 0C10
    mov a,(!spc_ptr)+y                ; 0C11
    mov.w !spc_pitch_mult+x,a         ; 0C13
.l0C16:
    ret                               ; 0C16

; @routine 19:C8A3 SpcSetPan
; WHAT IT DOES: Voice command $E1: voice X's pan A (bits 0-4 the position, bits 6-7 inverting either side) is kept and its fade cleared. The voice command handlers follow it: pan fade, vibrato on/off/fade, master volume and fade, tempo and fade, transposes, tremolo on/off, the pitch envelope, volume and fade, tuning, subroutine calls, echo volume, fade, off and setup.
; HOW: Stores to $0351, $0331 and $0330.
; Entry: A = pan, X = voice * 2. Exit: A = 0.
; @spc 0C17
%spc($0C17, $19C8A3)
SpcSetPan:
    mov.w !spc_pan_flags+x,a          ; 0C17
    and a,#$1F                        ; 0C1A
    mov.w !spc_pan+1+x,a              ; 0C1C
    mov a,#$00                        ; 0C1F
    mov.w !spc_pan+x,a                ; 0C21
    ret                               ; 0C24

SpcVcmdPanFade:
    mov.b !spc_pan_fade+x,a           ; 0C25
    push a                            ; 0C27
    call SpcTrackRead                 ; 0C28
    mov.w !spc_pan_target+x,a         ; 0C2B
    setc                              ; 0C2E
    sbc.w a,!spc_pan+1+x              ; 0C2F
    pop x                             ; 0C32
    call SpcFadeDelta                 ; 0C33
    mov.w !spc_pan_delta+x,a          ; 0C36
    mov a,y                           ; 0C39
    mov.w !spc_pan_delta+1+x,a        ; 0C3A
    ret                               ; 0C3D

SpcVcmdVibratoOn:
    mov.w !spc_vibrato_delay+x,a      ; 0C3E
    call SpcTrackRead                 ; 0C41
    mov.w !spc_vibrato_rate+x,a       ; 0C44
    call SpcTrackRead                 ; 0C47

SpcVcmdVibratoOff:
    mov.b !spc_vibrato_depth+x,a      ; 0C4A
    mov.w !spc_vibrato_target+x,a     ; 0C4C
    mov a,#$00                        ; 0C4F
    mov.w !spc_vibrato_fade_len+x,a   ; 0C51
    ret                               ; 0C54

SpcVcmdVibratoFade:
    mov.w !spc_vibrato_fade_len+x,a   ; 0C55  ; not reached in the traces so far
    push a                            ; 0C58  ; not reached in the traces so far
    mov y,#$00                        ; 0C59  ; not reached in the traces so far
    mov.b a,!spc_vibrato_depth+x      ; 0C5B  ; not reached in the traces so far
    pop x                             ; 0C5D  ; not reached in the traces so far
    div ya,x                          ; 0C5E  ; not reached in the traces so far
    mov.b x,!spc_voice_x              ; 0C5F  ; not reached in the traces so far
    mov.w !spc_vibrato_fade_delta+x,a ; 0C61  ; not reached in the traces so far
    ret                               ; 0C64  ; not reached in the traces so far

SpcVcmdMasterVolume:
    mov a,#$00                        ; 0C65
    movw !spc_master_volume,ya        ; 0C67
    ret                               ; 0C69

SpcVcmdMasterFade:
    mov.b !spc_master_fade,a          ; 0C6A  ; not reached in the traces so far
    call SpcTrackRead                 ; 0C6C  ; not reached in the traces so far
    mov.b $5B,a                       ; 0C6F  ; not reached in the traces so far
    setc                              ; 0C71  ; not reached in the traces so far
    sbc.b a,!spc_master_volume+1      ; 0C72  ; not reached in the traces so far
    mov.b x,!spc_master_fade          ; 0C74  ; not reached in the traces so far
    call SpcFadeDelta                 ; 0C76  ; not reached in the traces so far
    movw !spc_master_delta,ya         ; 0C79  ; not reached in the traces so far
    ret                               ; 0C7B  ; not reached in the traces so far

SpcVcmdTempo:
    mov a,#$00                        ; 0C7C
    movw !spc_tempo,ya                ; 0C7E
    ret                               ; 0C80

SpcVcmdTempoFade:
    mov.b !spc_tempo_fade,a           ; 0C81  ; not reached in the traces so far
    call SpcTrackRead                 ; 0C83  ; not reached in the traces so far
    mov.b $55,a                       ; 0C86  ; not reached in the traces so far
    setc                              ; 0C88  ; not reached in the traces so far
    sbc.b a,!spc_tempo+1              ; 0C89  ; not reached in the traces so far
    mov.b x,!spc_tempo_fade           ; 0C8B  ; not reached in the traces so far
    call SpcFadeDelta                 ; 0C8D  ; not reached in the traces so far
    movw !spc_tempo_delta,ya          ; 0C90  ; not reached in the traces so far
    ret                               ; 0C92  ; not reached in the traces so far

SpcVcmdTranspose:
    mov.b !spc_transpose,a            ; 0C93  ; not reached in the traces so far
    ret                               ; 0C95  ; not reached in the traces so far

SpcVcmdVoiceTranspose:
    mov.w !spc_voice_transpose+x,a    ; 0C96  ; not reached in the traces so far
    ret                               ; 0C99  ; not reached in the traces so far

SpcVcmdTremoloOn:
    mov.w !spc_tremolo_delay+x,a      ; 0C9A  ; not reached in the traces so far
    call SpcTrackRead                 ; 0C9D  ; not reached in the traces so far
    mov.w !spc_tremolo_rate+x,a       ; 0CA0  ; not reached in the traces so far
    call SpcTrackRead                 ; 0CA3  ; not reached in the traces so far

SpcVcmdTremoloOff:
    mov.b !spc_tremolo_depth+x,a      ; 0CA6  ; not reached in the traces so far
    ret                               ; 0CA8  ; not reached in the traces so far

SpcVcmdEnvelopeFrom:
    mov a,#$01                        ; 0CA9  ; not reached in the traces so far
    bra SpcVcmdEnvelopeTo_l0CAF       ; 0CAB  ; not reached in the traces so far

SpcVcmdEnvelopeTo:
    mov a,#$00                        ; 0CAD  ; not reached in the traces so far
.l0CAF:
    mov.w !spc_env_dir+x,a            ; 0CAF  ; not reached in the traces so far
    mov a,y                           ; 0CB2  ; not reached in the traces so far
    mov.w !spc_env_delay+x,a          ; 0CB3  ; not reached in the traces so far
    call SpcTrackRead                 ; 0CB6  ; not reached in the traces so far
    mov.w !spc_env_steps+x,a          ; 0CB9  ; not reached in the traces so far
    call SpcTrackRead                 ; 0CBC  ; not reached in the traces so far
    mov.w !spc_env_semitones+x,a      ; 0CBF  ; not reached in the traces so far
    ret                               ; 0CC2  ; not reached in the traces so far

SpcVcmdEnvelopeOff:
    mov.w !spc_env_steps+x,a          ; 0CC3  ; not reached in the traces so far
    ret                               ; 0CC6  ; not reached in the traces so far

SpcVcmdVolume:
    mov.w !spc_volume+1+x,a           ; 0CC7
    mov a,#$00                        ; 0CCA
    mov.w !spc_volume+x,a             ; 0CCC
    ret                               ; 0CCF

SpcVcmdVolumeFade:
    mov.b !spc_volume_fade+x,a        ; 0CD0
    push a                            ; 0CD2
    call SpcTrackRead                 ; 0CD3
    mov.w !spc_volume_target+x,a      ; 0CD6
    setc                              ; 0CD9
    sbc.w a,!spc_volume+1+x           ; 0CDA
    pop x                             ; 0CDD
    call SpcFadeDelta                 ; 0CDE
    mov.w !spc_volume_delta+x,a       ; 0CE1
    mov a,y                           ; 0CE4
    mov.w !spc_volume_delta+1+x,a     ; 0CE5
    ret                               ; 0CE8

SpcVcmdTuning:
    mov.w !spc_tuning+x,a             ; 0CE9
    ret                               ; 0CEC

SpcVcmdSubroutine:
    mov.w !spc_sub_start+x,a          ; 0CED
    call SpcTrackRead                 ; 0CF0
    mov.w !spc_sub_start+1+x,a        ; 0CF3
    call SpcTrackRead                 ; 0CF6
    mov.b !spc_sub_count+x,a          ; 0CF9
    mov.b a,!spc_track_ptr+x          ; 0CFB
    mov.w !spc_sub_return+x,a         ; 0CFD
    mov.b a,!spc_track_ptr+1+x        ; 0D00
    mov.w !spc_sub_return+1+x,a       ; 0D02

; @routine 19:C991 SpcSubroutineRestart
; WHAT IT DOES: Points voice X's track back at its subroutine's start ($0240/$0241), for a repeat.
; HOW: Two byte copies.
; Entry: X = voice * 2. Exit: A = the start's high byte.
; @spc 0D05
%spc($0D05, $19C991)
SpcSubroutineRestart:
    mov.w a,!spc_sub_start+x          ; 0D05
    mov.b !spc_track_ptr+x,a          ; 0D08
    mov.w a,!spc_sub_start+1+x        ; 0D0A
    mov.b !spc_track_ptr+1+x,a        ; 0D0D
    ret                               ; 0D0F

SpcVcmdEchoVolume:
    mov.b !spc_echo_enable,a          ; 0D10
    and.b a,!spc_sfx_voices           ; 0D12
    eor.b a,!spc_echo_enable          ; 0D14
    mov.b !spc_echo_voices,a          ; 0D16
    call SpcTrackRead                 ; 0D18
    mov a,#$00                        ; 0D1B
    movw !spc_echo_volume_l,ya        ; 0D1D
    call SpcTrackRead                 ; 0D1F
    mov a,#$00                        ; 0D22
    movw !spc_echo_volume_r,ya        ; 0D24
    clr1 !spc_flg.5                   ; 0D26
    ret                               ; 0D28

SpcVcmdEchoFade:
    mov.b !spc_echo_fade,a            ; 0D29  ; not reached in the traces so far
    call SpcTrackRead                 ; 0D2B  ; not reached in the traces so far
    mov.b $69,a                       ; 0D2E  ; not reached in the traces so far
    setc                              ; 0D30  ; not reached in the traces so far
    sbc.b a,$61                       ; 0D31  ; not reached in the traces so far
    mov.b x,!spc_echo_fade            ; 0D33  ; not reached in the traces so far
    call SpcFadeDelta                 ; 0D35  ; not reached in the traces so far
    movw !spc_echo_delta_l,ya         ; 0D38  ; not reached in the traces so far
    call SpcTrackRead                 ; 0D3A  ; not reached in the traces so far
    mov.b !spc_echo_target_r,a        ; 0D3D  ; not reached in the traces so far
    setc                              ; 0D3F  ; not reached in the traces so far
    sbc.b a,$63                       ; 0D40  ; not reached in the traces so far
    mov.b x,!spc_echo_fade            ; 0D42  ; not reached in the traces so far
    call SpcFadeDelta                 ; 0D44  ; not reached in the traces so far
    movw !spc_echo_delta_r,ya         ; 0D47  ; not reached in the traces so far
    ret                               ; 0D49  ; not reached in the traces so far

SpcVcmdEchoOff:
    movw !spc_echo_volume_l,ya        ; 0D4A  ; not reached in the traces so far
    movw !spc_echo_volume_r,ya        ; 0D4C  ; not reached in the traces so far
    set1 !spc_flg.5                   ; 0D4E  ; not reached in the traces so far
    ret                               ; 0D50  ; not reached in the traces so far

SpcVcmdEchoSetup:
    call SpcSetEchoDelay              ; 0D51
    call SpcTrackRead                 ; 0D54
    mov.b !spc_echo_feedback,a        ; 0D57
    call SpcTrackRead                 ; 0D59
    mov y,#$08                        ; 0D5C
    mul ya                            ; 0D5E
    mov x,a                           ; 0D5F
    mov y,#$0F                        ; 0D60
.l0D62:
    mov.w a,!spc_echo_fir_sets+x      ; 0D62
    call SpcDspWrite                  ; 0D65
    inc x                             ; 0D68
    mov a,y                           ; 0D69
    clrc                              ; 0D6A
    adc a,#$10                        ; 0D6B
    mov y,a                           ; 0D6D
    bpl .l0D62                        ; 0D6E
    mov.b x,!spc_voice_x              ; 0D70
    ret                               ; 0D72

; @routine 19:C9FF SpcSetEchoDelay
; WHAT IT DOES: Sets the echo delay to A ($4D): when EDL differs, the DSP registers named by the first four entries of the list at $3F9F are zeroed, echo writes are turned off in FLG ($48 with bit 5), EDL is written and $4C starts the count SpcReset waits on before writing the echo registers again; in every case the echo buffer start ESA = $40 - 8 * delay.
; HOW: EDL ($7D) read back to compare; ESA ($6D) by ASLs, EOR and ADC.
; Entry: A = delay (0-15). Exit: A, Y as the last DSP write left them.
; @spc 0D73
%spc($0D73, $19C9FF)
SpcSetEchoDelay:
    mov.b !spc_echo_delay,a           ; 0D73
    mov y,#$7D                        ; 0D75
    mov.w !spc_dspaddr,y              ; 0D77
    mov.w a,!spc_dspdata              ; 0D7A
    cmp.b a,!spc_echo_delay           ; 0D7D
    beq .l0DAC                        ; 0D7F
    and a,#$0F                        ; 0D81
    eor a,#$FF                        ; 0D83
    bbc !spc_echo_delay_wait.7,.l0D8B ; 0D85
    clrc                              ; 0D88  ; not reached in the traces so far
    adc.b a,!spc_echo_delay_wait      ; 0D89  ; not reached in the traces so far
.l0D8B:
    mov.b !spc_echo_delay_wait,a      ; 0D8B
    mov y,#$04                        ; 0D8D
.l0D8F:
    mov.w a,!spc_dsp_shadow_regs+y    ; 0D8F
    mov.w !spc_dspaddr,a              ; 0D92
    mov a,#$00                        ; 0D95
    mov.w !spc_dspdata,a              ; 0D97
    dbnz y,.l0D8F                     ; 0D9A
    mov.b a,!spc_flg                  ; 0D9C
    or a,#$20                         ; 0D9E
    mov y,#$6C                        ; 0DA0
    call SpcDspWrite                  ; 0DA2
    mov.b a,!spc_echo_delay           ; 0DA5
    mov y,#$7D                        ; 0DA7
    call SpcDspWrite                  ; 0DA9
.l0DAC:
    asl a                             ; 0DAC
    asl a                             ; 0DAD
    asl a                             ; 0DAE
    eor a,#$FF                        ; 0DAF
    setc                              ; 0DB1
    adc a,#$3F                        ; 0DB2
    mov y,#$6D                        ; 0DB4
    jmp SpcDspWrite                   ; 0DB6

SpcVcmdPercussionBase:
    mov.b !spc_percussion_base,a      ; 0DB9
    ret                               ; 0DBB

; @routine 19:CA48 SpcSlideCheck
; WHAT IT DOES: When voice X's next track byte is a pitch slide ($F9) and no slide runs, reads its delay, length and target note and starts the slide (SpcSlideStart's code).
; HOW: Peeks at ($30+X); the slide delta from SpcFadeDelta.
; Entry: X = voice * 2. Exit: A, Y changed.
; @spc 0DBC
%spc($0DBC, $19CA48)
SpcSlideCheck:
    mov.b a,!spc_slide_steps+x        ; 0DBC
    bne SpcSlideToNote_l0DF3          ; 0DBE
    mov a,(!spc_track_ptr+x)          ; 0DC0
    cmp a,#$F9                        ; 0DC2
    bne SpcSlideToNote_l0DF3          ; 0DC4
    call SpcTrackAdvance              ; 0DC6
    call SpcTrackRead                 ; 0DC9

SpcVcmdPitchSlide:
    mov.b !spc_slide_delay+x,a        ; 0DCC
    call SpcTrackRead                 ; 0DCE
    mov.b !spc_slide_steps+x,a        ; 0DD1
    call SpcTrackRead                 ; 0DD3
    clrc                              ; 0DD6
    adc.b a,!spc_transpose            ; 0DD7
    adc.w a,!spc_voice_transpose+x    ; 0DD9

; @routine 19:CA68 SpcSlideToNote
; WHAT IT DOES: Starts voice X's pitch slide to note A (bits 0-6): the target kept at $0380 and the per-step delta over the slide's steps $A0 at $0370/$0371.
; HOW: SpcFadeDelta on target minus pitch; X from the steps through the stack.
; Entry: A = target note, X = voice * 2. Exit: X = voice * 2; YA = delta.
; @spc 0DDC
%spc($0DDC, $19CA68)
SpcSlideToNote:
    and a,#$7F                        ; 0DDC
    mov.w !spc_slide_target+x,a       ; 0DDE
    setc                              ; 0DE1
    sbc.w a,!spc_pitch+1+x            ; 0DE2
    mov.b y,!spc_slide_steps+x        ; 0DE5
    push y                            ; 0DE7
    pop x                             ; 0DE8
    call SpcFadeDelta                 ; 0DE9
    mov.w !spc_slide_delta+x,a        ; 0DEC
    mov a,y                           ; 0DEF
    mov.w !spc_slide_delta+1+x,a      ; 0DF0
.l0DF3:
    ret                               ; 0DF3

; @routine 19:CA80 SpcLoadPitch
; WHAT IT DOES: Copies voice X's pitch $0360/$0361 to $10/$11.
; HOW: Two byte copies.
; Entry: X = voice * 2. Exit: A = the pitch's low byte.
; @spc 0DF4
%spc($0DF4, $19CA80)
SpcLoadPitch:
    mov.w a,!spc_pitch+1+x            ; 0DF4
    mov.b !spc_tmp_hi,a               ; 0DF7
    mov.w a,!spc_pitch+x              ; 0DF9
    mov.b !spc_tmp_lo,a               ; 0DFC
    ret                               ; 0DFE

; @routine 19:CA8B SpcFadeDelta
; WHAT IT DOES: The per-step delta of a fade: the signed difference A (with the borrow C from the subtraction that made it) divided by the step count X, as a 16-bit value.
; HOW: The sign kept in $12 (NOTC, ROR), the magnitude divided in two DIVs (whole, then fraction), negated back through SpcNegateIfNeg; X reloaded from $44.
; Entry: A = difference, C = no borrow, X = steps. Exit: YA = delta; X = $44.
; @spc 0DFF
%spc($0DFF, $19CA8B)
SpcFadeDelta:
    notc                              ; 0DFF
    ror.b !spc_tmp_sign               ; 0E00
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
    mov.b x,!spc_voice_x              ; 0E0F

; @routine 19:CA9D SpcNegateIfNeg
; WHAT IT DOES: Negates YA when bit 7 of $12 is set.
; HOW: 0 - YA through $0E and $14.
; Entry: YA = value, $12 = sign. Exit: YA.
; @spc 0E11
%spc($0E11, $19CA9D)
SpcNegateIfNeg:
    bbc !spc_tmp_sign.7,.l0E1A        ; 0E11
    movw !spc_ptr,ya                  ; 0E14
    movw ya,$0E                       ; 0E16
    subw ya,!spc_ptr                  ; 0E18
.l0E1A:
    ret                               ; 0E1A

; @routine 19:CAA7 SpcVoiceEffects
; WHAT IT DOES: Voice X's per-tick modulation: the volume fade steps, tremolo moves the volume, the volume is recomputed (SpcVolumeCalc), the pan fade steps; then the volume and pan go to the DSP (SpcVoiceVolumeWrite).
; HOW: 16-bit fades as low/high byte pairs; tremolo by its phase and depth.
; Entry: X = voice * 2, $47 = its bit. Exit: as SpcVoiceVolumeWrite.
; @spc 0E1B
%spc($0E1B, $19CAA7)
SpcVoiceEffects:
    mov.b a,!spc_volume_fade+x        ; 0E1B
    beq .l0E43                        ; 0E1D
    or !spc_volume_dirty,!spc_voice_bit; 0E1F
    dec.b !spc_volume_fade+x          ; 0E22
    bne .l0E30                        ; 0E24
    mov a,#$00                        ; 0E26
    mov.w !spc_volume+x,a             ; 0E28
    mov.w a,!spc_volume_target+x      ; 0E2B
    bra .l0E40                        ; 0E2E
.l0E30:
    clrc                              ; 0E30
    mov.w a,!spc_volume+x             ; 0E31
    adc.w a,!spc_volume_delta+x       ; 0E34
    mov.w !spc_volume+x,a             ; 0E37
    mov.w a,!spc_volume+1+x           ; 0E3A
    adc.w a,!spc_volume_delta+1+x     ; 0E3D
.l0E40:
    mov.w !spc_volume+1+x,a           ; 0E40
.l0E43:
    mov.b y,!spc_tremolo_depth+x      ; 0E43
    beq .l0E6A                        ; 0E45
    mov.w a,!spc_tremolo_delay+x      ; 0E47  ; not reached in the traces so far
    cbne !spc_tremolo_wait+x,.l0E68   ; 0E4A  ; not reached in the traces so far
    or !spc_volume_dirty,!spc_voice_bit; 0E4D  ; not reached in the traces so far
    mov.w a,!spc_tremolo_phase+x      ; 0E50  ; not reached in the traces so far
    bpl .l0E5C                        ; 0E53  ; not reached in the traces so far
    inc y                             ; 0E55  ; not reached in the traces so far
    bne .l0E5C                        ; 0E56  ; not reached in the traces so far
    mov a,#$80                        ; 0E58  ; not reached in the traces so far
    bra .l0E60                        ; 0E5A  ; not reached in the traces so far
.l0E5C:
    clrc                              ; 0E5C  ; not reached in the traces so far
    adc.w a,!spc_tremolo_rate+x       ; 0E5D  ; not reached in the traces so far
.l0E60:
    mov.w !spc_tremolo_phase+x,a      ; 0E60  ; not reached in the traces so far
    call SpcTremoloFactor             ; 0E63  ; not reached in the traces so far
    bra .l0E6F                        ; 0E66  ; not reached in the traces so far
.l0E68:
    inc.b !spc_tremolo_wait+x         ; 0E68  ; not reached in the traces so far
.l0E6A:
    mov a,#$FF                        ; 0E6A
    call SpcVolumeCalc                ; 0E6C
.l0E6F:
    mov.b a,!spc_pan_fade+x           ; 0E6F
    beq SpcVoiceVolumeWrite           ; 0E71
    or !spc_volume_dirty,!spc_voice_bit; 0E73
    dec.b !spc_pan_fade+x             ; 0E76
    bne .l0E84                        ; 0E78
    mov a,#$00                        ; 0E7A
    mov.w !spc_pan+x,a                ; 0E7C
    mov.w a,!spc_pan_target+x         ; 0E7F
    bra .l0E94                        ; 0E82
.l0E84:
    clrc                              ; 0E84
    mov.w a,!spc_pan+x                ; 0E85
    adc.w a,!spc_pan_delta+x          ; 0E88
    mov.w !spc_pan+x,a                ; 0E8B
    mov.w a,!spc_pan+1+x              ; 0E8E
    adc.w a,!spc_pan_delta+1+x        ; 0E91
.l0E94:
    mov.w !spc_pan+1+x,a              ; 0E94

; @routine 19:CB23 SpcVoiceVolumeWrite
; WHAT IT DOES: When voice X's volume or pan changed ($5E), writes its left and right volumes (SpcVoiceVolumeLR) from its pan.
; HOW: The pan $0330/$0331 to $10/$11, then SpcVoiceVolumeLR.
; Entry: X = voice * 2, $47 = its bit. Exit: as SpcVoiceVolumeLR.
; @spc 0E97
%spc($0E97, $19CB23)
SpcVoiceVolumeWrite:
    mov.b a,!spc_voice_bit            ; 0E97
    and.b a,!spc_volume_dirty         ; 0E99
    beq SpcVoiceVolumeLR_l0EE3        ; 0E9B
    mov.w a,!spc_pan+1+x              ; 0E9D
    mov y,a                           ; 0EA0
    mov.w a,!spc_pan+x                ; 0EA1
    movw !spc_tmp_lo,ya               ; 0EA4

; @routine 19:CB32 SpcVoiceVolumeLR
; WHAT IT DOES: Writes voice X's VxVOLL and VxVOLR: the pan curve at $107A interpolated at the pan $10/$11, times the voice's volume, negated for a side the pan flags invert; the right side from 20 minus the pan.
; HOW: Two passes over $12 (the register), MUL for the interpolation and the volume.
; Entry: $10/$11 = pan, X = voice * 2. Exit: A, Y changed.
; @spc 0EA6
%spc($0EA6, $19CB32)
SpcVoiceVolumeLR:
    mov a,x                           ; 0EA6
    xcn a                             ; 0EA7
    lsr a                             ; 0EA8
    mov.b !spc_tmp_sign,a             ; 0EA9
.l0EAB:
    mov.b y,!spc_tmp_hi               ; 0EAB
    mov.w a,!spc_pan_table+1+y        ; 0EAD
    setc                              ; 0EB0
    sbc.w a,!spc_pan_table+y          ; 0EB1
    mov.b y,!spc_tmp_lo               ; 0EB4
    mul ya                            ; 0EB6
    mov a,y                           ; 0EB7
    mov.b y,!spc_tmp_hi               ; 0EB8
    clrc                              ; 0EBA
    adc.w a,!spc_pan_table+y          ; 0EBB
    mov y,a                           ; 0EBE
    mov.w a,!spc_volume_out+x         ; 0EBF
    mul ya                            ; 0EC2
    mov.w a,!spc_pan_flags+x          ; 0EC3
    asl a                             ; 0EC6
    bbc !spc_tmp_sign.0,.l0ECB        ; 0EC7
    asl a                             ; 0ECA
.l0ECB:
    mov a,y                           ; 0ECB
    bcc .l0ED1                        ; 0ECC
    eor a,#$FF                        ; 0ECE  ; not reached in the traces so far
    inc a                             ; 0ED0  ; not reached in the traces so far
.l0ED1:
    mov.b y,!spc_tmp_sign             ; 0ED1
    call SpcDspWriteVoice             ; 0ED3
    mov y,#$14                        ; 0ED6
    mov a,#$00                        ; 0ED8
    subw ya,!spc_tmp_lo               ; 0EDA
    movw !spc_tmp_lo,ya               ; 0EDC
    inc.b !spc_tmp_sign               ; 0EDE
    bbc !spc_tmp_sign.1,.l0EAB        ; 0EE0
.l0EE3:
    ret                               ; 0EE3

; @routine 19:CB70 SpcVoiceTick
; WHAT IT DOES: Voice X between its notes: at the gate time, key-off unless the next note is a tie (the track is scanned ahead through voice commands, subroutine ends and returns); the pitch slide steps; vibrato moves the pitch; the pitch is written when it changed.
; HOW: The look-ahead skips command parameters by the table at $0FAF; the slide and vibrato through the pitch code at $095D.
; Entry: X = voice * 2, $47 = its bit. Exit: A, Y changed.
; @spc 0EE4
%spc($0EE4, $19CB70)
SpcVoiceTick:
    mov.b a,!spc_note_keyoff+x        ; 0EE4
    beq .l0F4D                        ; 0EE6
    dec.b !spc_note_keyoff+x          ; 0EE8
    beq .l0EF1                        ; 0EEA
    mov a,#$02                        ; 0EEC
    cbne !spc_note_timer+x,.l0F4D     ; 0EEE
.l0EF1:
    mov.b a,!spc_sub_count+x          ; 0EF1
    mov.b !spc_tmp_word+1,a           ; 0EF3
    mov.b a,!spc_track_ptr+x          ; 0EF5
    mov.b y,!spc_track_ptr+1+x        ; 0EF7
.l0EF9:
    movw !spc_ptr,ya                  ; 0EF9
    mov y,#$00                        ; 0EFB
.l0EFD:
    mov a,(!spc_ptr)+y                ; 0EFD
    beq .l0F1F                        ; 0EFF
    bmi .l0F0A                        ; 0F01
.l0F03:
    inc y                             ; 0F03
    bmi .l0F46                        ; 0F04
    mov a,(!spc_ptr)+y                ; 0F06
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
    adc.w a,!spc_vcmd_skip_table+y    ; 0F19
    mov y,a                           ; 0F1C
    bra .l0EFD                        ; 0F1D
.l0F1F:
    mov.b a,!spc_tmp_word+1           ; 0F1F
    beq .l0F46                        ; 0F21
    dec.b !spc_tmp_word+1             ; 0F23
    bne .l0F31                        ; 0F25
    mov.w a,!spc_sub_return+1+x       ; 0F27
    push a                            ; 0F2A
    mov.w a,!spc_sub_return+x         ; 0F2B
    pop y                             ; 0F2E
    bra .l0EF9                        ; 0F2F
.l0F31:
    mov.w a,!spc_sub_start+1+x        ; 0F31
    push a                            ; 0F34
    mov.w a,!spc_sub_start+x          ; 0F35
    pop y                             ; 0F38
    bra .l0EF9                        ; 0F39
.l0F3B:
    inc y                             ; 0F3B
    mov a,(!spc_ptr)+y                ; 0F3C
    push a                            ; 0F3E
    inc y                             ; 0F3F
    mov a,(!spc_ptr)+y                ; 0F40
    mov y,a                           ; 0F42
    pop a                             ; 0F43
    bra .l0EF9                        ; 0F44
.l0F46:
    mov.b a,!spc_voice_bit            ; 0F46
    mov y,#$5C                        ; 0F48
    call SpcDspWriteVoice             ; 0F4A
.l0F4D:
    clr1 !spc_voice_dirty.7           ; 0F4D
    mov.b a,!spc_slide_steps+x        ; 0F4F
    beq .l0F7F                        ; 0F51
    mov.b a,!spc_slide_delay+x        ; 0F53
    beq .l0F5B                        ; 0F55
    dec.b !spc_slide_delay+x          ; 0F57
    bra .l0F7F                        ; 0F59
.l0F5B:
    set1 !spc_voice_dirty.7           ; 0F5B
    dec.b !spc_slide_steps+x          ; 0F5D
    bne .l0F6C                        ; 0F5F
    mov.w a,!spc_tuning+x             ; 0F61
    mov.w !spc_pitch+x,a              ; 0F64
    mov.w a,!spc_slide_target+x       ; 0F67
    bra .l0F7C                        ; 0F6A
.l0F6C:
    clrc                              ; 0F6C
    mov.w a,!spc_pitch+x              ; 0F6D
    adc.w a,!spc_slide_delta+x        ; 0F70
    mov.w !spc_pitch+x,a              ; 0F73
    mov.w a,!spc_pitch+1+x            ; 0F76
    adc.w a,!spc_slide_delta+1+x      ; 0F79
.l0F7C:
    mov.w !spc_pitch+1+x,a            ; 0F7C
.l0F7F:
    call SpcLoadPitch                 ; 0F7F
    mov.b a,!spc_vibrato_depth+x      ; 0F82
    beq .l0FD2                        ; 0F84
    mov.w a,!spc_vibrato_delay+x      ; 0F86
    cbne !spc_vibrato_wait+x,.l0FD0   ; 0F89
    mov.w a,!spc_vibrato_fade_step+x  ; 0F8C
    cmp.w a,!spc_vibrato_fade_len+x   ; 0F8F
    bne .l0F99                        ; 0F92
    mov.w a,!spc_vibrato_target+x     ; 0F94
    bra .l0FA6                        ; 0F97
.l0F99:
    setp                              ; 0F99  ; not reached in the traces so far
    inc.b !spc_port_in+x              ; 0F9A  ; not reached in the traces so far
    clrp                              ; 0F9C  ; not reached in the traces so far
    mov y,a                           ; 0F9D  ; not reached in the traces so far
    beq .l0FA2                        ; 0F9E  ; not reached in the traces so far
    mov.b a,!spc_vibrato_depth+x      ; 0FA0  ; not reached in the traces so far
.l0FA2:
    clrc                              ; 0FA2  ; not reached in the traces so far
    adc.w a,!spc_vibrato_fade_delta+x ; 0FA3  ; not reached in the traces so far
.l0FA6:
    mov.b !spc_vibrato_depth+x,a      ; 0FA6
    mov.w a,!spc_vibrato_phase+x      ; 0FA8
    clrc                              ; 0FAB
    adc.w a,!spc_vibrato_rate+x       ; 0FAC
    mov.w !spc_vibrato_phase+x,a      ; 0FAF
.l0FB2:
    mov.b !spc_tmp_sign,a             ; 0FB2
    asl a                             ; 0FB4
    asl a                             ; 0FB5
    bcc .l0FBA                        ; 0FB6
    eor a,#$FF                        ; 0FB8
.l0FBA:
    mov y,a                           ; 0FBA
    mov.b a,!spc_vibrato_depth+x      ; 0FBB
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
    call SpcAddDelta                  ; 0FCA
.l0FCD:
    jmp $095D                         ; 0FCD
.l0FD0:
    inc.b !spc_vibrato_wait+x         ; 0FD0
.l0FD2:
    bbs !spc_voice_dirty.7,.l0FCD     ; 0FD2
    ret                               ; 0FD5

; @routine 19:CC62 SpcVoiceBetweenTicks
; WHAT IT DOES: Voice X's smoothing between music ticks: the pan fade, pitch slide and vibrato moved by the part of a tick elapsed ($51), and the DSP volume and pitch written when they moved.
; HOW: SpcAddScaledDelta for each running fade; the vibrato through SpcVoiceTick's code at $0FB2.
; Entry: X = voice * 2, $47 = its bit. Exit: A, Y changed.
; @spc 0FD6
%spc($0FD6, $19CC62)
SpcVoiceBetweenTicks:
    clr1 !spc_voice_dirty.7           ; 0FD6
    mov.b a,!spc_tremolo_depth+x      ; 0FD8
    beq .l0FE5                        ; 0FDA
    mov.w a,!spc_tremolo_delay+x      ; 0FDC  ; not reached in the traces so far
    cbne !spc_tremolo_wait+x,.l0FE5   ; 0FDF  ; not reached in the traces so far
    call SpcTremoloBetweenTicks       ; 0FE2  ; not reached in the traces so far
.l0FE5:
    mov.w a,!spc_pan+1+x              ; 0FE5
    mov y,a                           ; 0FE8
    mov.w a,!spc_pan+x                ; 0FE9
    movw !spc_tmp_lo,ya               ; 0FEC
    mov.b a,!spc_pan_fade+x           ; 0FEE
    beq .l0FFC                        ; 0FF0
    mov.w a,!spc_pan_delta+1+x        ; 0FF2
    mov y,a                           ; 0FF5
    mov.w a,!spc_pan_delta+x          ; 0FF6
    call SpcAddScaledDelta            ; 0FF9
.l0FFC:
    bbc !spc_voice_dirty.7,.l1002     ; 0FFC
    call SpcVoiceVolumeLR             ; 0FFF
.l1002:
    clr1 !spc_voice_dirty.7           ; 1002
    call SpcLoadPitch                 ; 1004
    mov.b a,!spc_slide_steps+x        ; 1007
    beq .l1019                        ; 1009
    mov.b a,!spc_slide_delay+x        ; 100B
    bne .l1019                        ; 100D
    mov.w a,!spc_slide_delta+1+x      ; 100F
    mov y,a                           ; 1012
    mov.w a,!spc_slide_delta+x        ; 1013
    call SpcAddScaledDelta            ; 1016
.l1019:
    mov.b a,!spc_vibrato_depth+x      ; 1019
    beq SpcVoiceTick_l0FD2            ; 101B
    mov.w a,!spc_vibrato_delay+x      ; 101D
    cbne !spc_vibrato_wait+x,SpcVoiceTick_l0FD2; 1020
    mov.b y,!spc_sfx_tempo_acc        ; 1023
    mov.w a,!spc_vibrato_rate+x       ; 1025
    mul ya                            ; 1028
    mov a,y                           ; 1029
    clrc                              ; 102A
    adc.w a,!spc_vibrato_phase+x      ; 102B
    jmp $0FB2                         ; 102E

; @routine 19:CCBD SpcAddScaledDelta
; WHAT IT DOES: Adds the 16-bit delta YA scaled by the tick fraction $51 to the value at $10/$11, and marks the voice changed (bit 7 of $13).
; HOW: Two MULs by $51, the sign kept in $12; then SpcAddDelta.
; Entry: YA = delta, $10/$11 = value. Exit: $10/$11 updated.
; @spc 1031
%spc($1031, $19CCBD)
SpcAddScaledDelta:
    set1 !spc_voice_dirty.7           ; 1031
    mov.b !spc_tmp_sign,y             ; 1033
    call SpcNegateIfNeg               ; 1035
    push y                            ; 1038
    mov.b y,!spc_sfx_tempo_acc        ; 1039
    mul ya                            ; 103B
    mov.b !spc_ptr,y                  ; 103C
    mov !spc_ptr+1,#$00               ; 103E
    mov.b y,!spc_sfx_tempo_acc        ; 1041
    pop a                             ; 1043
    mul ya                            ; 1044
    addw ya,!spc_ptr                  ; 1045

; @routine 19:CCD3 SpcAddDelta
; WHAT IT DOES: Adds YA (negated when $12 bit 7 is set) to $10/$11.
; HOW: SpcNegateIfNeg, ADDW.
; Entry: YA, $12. Exit: YA = $10/$11 = the sum.
; @spc 1047
%spc($1047, $19CCD3)
SpcAddDelta:
    call SpcNegateIfNeg               ; 1047
    addw ya,!spc_tmp_lo               ; 104A
    movw !spc_tmp_lo,ya               ; 104C
    ret                               ; 104E

; @routine 19:CCDB SpcTremoloBetweenTicks
; WHAT IT DOES: Voice X's tremolo moved by the part of a tick elapsed ($51), then its volume factor (SpcTremoloFactor's code).
; HOW: The rate $02D1 times $51 added to the phase $02D0; sets bit 7 of $13.
; Entry: X = voice * 2. Exit: as SpcVolumeCalc.
; @spc 104F
%spc($104F, $19CCDB)
SpcTremoloBetweenTicks:
    set1 !spc_voice_dirty.7           ; 104F  ; not reached in the traces so far
    mov.b y,!spc_sfx_tempo_acc        ; 1051  ; not reached in the traces so far
    mov.w a,!spc_tremolo_rate+x       ; 1053  ; not reached in the traces so far
    mul ya                            ; 1056  ; not reached in the traces so far
    mov a,y                           ; 1057  ; not reached in the traces so far
    clrc                              ; 1058  ; not reached in the traces so far
    adc.w a,!spc_tremolo_phase+x      ; 1059  ; not reached in the traces so far

; @routine 19:CCE8 SpcTremoloFactor
; WHAT IT DOES: The tremolo's volume factor from its phase A: a triangle of the phase times the depth $C1, inverted, then the volume (SpcVolumeCalc).
; HOW: ASL with the carry folding the wave, MUL by the depth.
; Entry: A = phase, X = voice * 2. Exit: as SpcVolumeCalc.
; @spc 105C
%spc($105C, $19CCE8)
SpcTremoloFactor:
    asl a                             ; 105C  ; not reached in the traces so far
    bcc .l1061                        ; 105D  ; not reached in the traces so far
    eor a,#$FF                        ; 105F  ; not reached in the traces so far
.l1061:
    mov.b y,!spc_tremolo_depth+x      ; 1061  ; not reached in the traces so far
    mul ya                            ; 1063  ; not reached in the traces so far
    mov a,y                           ; 1064  ; not reached in the traces so far
    eor a,#$FF                        ; 1065  ; not reached in the traces so far

; @routine 19:CCF3 SpcVolumeCalc
; WHAT IT DOES: Voice X's output volume $0321: A (the tremolo factor, $FF for none) times the global scale $0565, the note velocity and the voice volume, squared.
; HOW: A chain of MULs keeping the high bytes.
; Entry: A = tremolo factor, X = voice * 2. Exit: A = the volume.
; @spc 1067
%spc($1067, $19CCF3)
SpcVolumeCalc:
    mov.w y,!spc_volume_scale         ; 1067
    mul ya                            ; 106A
    mov.w a,!spc_note_velocity+x      ; 106B
    mul ya                            ; 106E
    mov.w a,!spc_volume+1+x           ; 106F
    mul ya                            ; 1072
    mov a,y                           ; 1073
    mul ya                            ; 1074
    mov a,y                           ; 1075
    mov.w !spc_volume_out+x,a         ; 1076
    ret                               ; 1079
    db $00, $01, $03, $07, $0D, $15, $1E, $29
    db $34, $42, $51, $5E, $67, $6E, $73, $77
    db $7A, $7C, $7D, $7E, $7F, $01, $01, $02
    db $03, $00, $01, $02, $01, $02, $01, $01
    db $03, $00, $01, $02, $03, $01, $03, $03
    db $00, $01, $03, $00, $03, $03, $03, $01

%spc($10B0, $19CD3A)

; @routine 19:CD3A SpcSfxFrame
; WHAT IT DOES: The game's sound tick: a new port 0 byte (one differing from the last, $04) becomes the command $00 (else $00 = 0); ports 1-3 copied to $01-$03 and $05-$07; a stop request (bit 1 of $29) clears bits 0 and 1; then the tick's game sound work in order (command remaps, echo and tempo effects, port commands, channel stops, the sound queue, timers, pitch and fade ramps, noise sample) and the sound effect channels (through the queue start at $1C07).
; HOW: A chain of CALLs, then SpcPort1Command and a JMP to the queue start.
; Entry: none (called from SpcReset's loop). Exit: as the sound effect channels leave it.
; @spc 10B0
%spc($10B0, $19CD3A)
SpcSfxFrame:
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
    mov.w a,$0559                     ; 10C3
    mov.b $05,a                       ; 10C6
    mov.b $01,a                       ; 10C8
    mov.w a,$055A                     ; 10CA
    mov.b $06,a                       ; 10CD
    mov.b $02,a                       ; 10CF
    mov.w a,$055B                     ; 10D1
    mov.b $07,a                       ; 10D4
    mov.b $03,a                       ; 10D6
    bbc !spc_flags.1,.l10DF           ; 10D8
    clr1 !spc_flags.0                 ; 10DB
    clr1 !spc_flags.1                 ; 10DD
.l10DF:
    call SpcPort1Remap                ; 10DF
    call SpcEchoTransition            ; 10E2
    call SpcMusicFade                 ; 10E5
    call SpcPortCommand               ; 10E8
    call SpcChannelGuards             ; 10EB
    call SpcCommandBlock              ; 10EE
    call SpcCommandChannelCheck       ; 10F1
    call SpcTempoBend                 ; 10F4
    call SpcStaggeredChecks           ; 10F7
    call SpcGameStateTick             ; 10FA
    call SpcSfxHolds                  ; 10FD
    call SpcPitchRampB                ; 1100
    call SpcPitchRampA                ; 1103
    call SpcCommandFilter             ; 1106
    call SpcCommand4C                 ; 1109
    call SpcCommandTimers             ; 110C
    call SpcNoiseSample               ; 110F
    call SpcPort1Clamp                ; 1112
    call SpcPort1Command              ; 1115
    jmp $1C07                         ; 1118

; @routine 19:CDA5 SpcPort1Remap
; WHAT IT DOES: With bit 7 of $2D set, port 1 command $09 becomes $12 and $0A becomes $0C.
; HOW: CMP and MOV on $01.
; Entry: none. Exit: flags from the last compare.
; @spc 111B
%spc($111B, $19CDA5)
SpcPort1Remap:
    bbc $2D.7,.l112E                  ; 111B
    cmp $01,#$09                      ; 111E
    bne .l1126                        ; 1121
    mov $01,#$12                      ; 1123
.l1126:
    cmp $01,#$0A                      ; 1126
    bne .l112E                        ; 1129
    mov $01,#$0C                      ; 112B
.l112E:
    ret                               ; 112E

; @routine 19:CDB9 SpcChannelGuards
; WHAT IT DOES: With bit 6 of $2D, stops channel 5 unless it plays sound $43; with bit 5, channel 7 stops when track $0397 is 1, 4, 7, $0A or $0D and channel 6 when channel 6's sound $0396 is $72-$76.
; HOW: Compare chains into SpcSfxStop.
; Entry: none. Exit: as SpcSfxStop or a RET.
; @spc 112F
%spc($112F, $19CDB9)
SpcChannelGuards:
    bbc $2D.6,SpcPort1Remap_l112E     ; 112F
    mov.w a,$0395                     ; 1132
    cmp a,#$43                        ; 1135
    beq SpcPort1Remap_l112E           ; 1137
    mov x,#$05                        ; 1139
    jmp SpcSfxStop                    ; 113B
.l113E:
    bbc $2D.5,.l116F                  ; 113E
    mov.w a,$0397                     ; 1141
    cmp a,#$01                        ; 1144
    beq .l1170                        ; 1146
    cmp a,#$04                        ; 1148
    beq .l1170                        ; 114A
    cmp a,#$07                        ; 114C
    beq .l1170                        ; 114E
    cmp a,#$0A                        ; 1150
    beq .l1170                        ; 1152
    cmp a,#$0D                        ; 1154
    beq .l1170                        ; 1156
.l1158:
    mov.w a,$0396                     ; 1158
    cmp a,#$72                        ; 115B
    beq .l1177                        ; 115D
    cmp a,#$73                        ; 115F
    beq .l1177                        ; 1161
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
    call SpcSfxStop                   ; 1172  ; not reached in the traces so far
    bra .l1158                        ; 1175  ; not reached in the traces so far
.l1177:
    mov x,#$06                        ; 1177  ; not reached in the traces so far
    jmp SpcSfxStop                    ; 1179  ; not reached in the traces so far

; @routine 19:CE06 SpcStaggeredChecks
; WHAT IT DOES: Steps the counter $0581 through 0-$16 each tick and at 3, 9, $0C, $0F, $12 and $15 does one check: restarting a looping sound (channels by bits of $2A and $2D, stop through SpcSfxStopIf with $20 = the step) or queueing sound $17, $25 or $21; outside song 1, $2A is cleared.
; HOW: A compare chain on the counter; the stop helpers are SpcSfxStopIf's entries.
; Entry: none. Exit: as the chosen path leaves it.
; @spc 117C
%spc($117C, $19CE06)
SpcStaggeredChecks:
    inc.w $0581                       ; 117C
    mov.w a,$0581                     ; 117F
    cmp a,#$03                        ; 1182
    beq .l11AC                        ; 1184
    cmp a,#$09                        ; 1186
    beq SpcChannelGuards_l113E        ; 1188
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
    cmp !spc_song,#$01                ; 11A3
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
    jmp SpcSfxStopIf                  ; 11B9
.l11BC:
    bbs $2A.0,.l11C5                  ; 11BC
    mov $20,#$0C                      ; 11BF
    jmp $192B                         ; 11C2
.l11C5:
    mov a,#$17                        ; 11C5
    jmp SpcSoundQueuePush             ; 11C7
.l11CA:
    bbs $2A.2,.l11D3                  ; 11CA
    mov $20,#$0A                      ; 11CD
    jmp $1915                         ; 11D0
.l11D3:
    bbs $2A.0,.l11A3                  ; 11D3
    bbs $2A.7,.l11A3                  ; 11D6
    bbs $2A.4,.l11A3                  ; 11D9
    mov a,#$25                        ; 11DC
    jmp SpcSoundQueuePush             ; 11DE
.l11E1:
    bbs $2A.4,.l11A3                  ; 11E1
    mov $20,#$0B                      ; 11E4
    jmp $1920                         ; 11E7
.l11EA:
    bbs $2A.7,.l11F3                  ; 11EA
    mov $20,#$0E                      ; 11ED
    jmp $1936                         ; 11F0
.l11F3:
    mov a,#$21                        ; 11F3
    jmp SpcSoundQueuePush             ; 11F5

; @routine 19:CE82 SpcGameStateTick
; WHAT IT DOES: The game state's sound rules each tick: the countdown flags in $2D from the bytes $EE40 and $CB10; the counters $056A/$056B after song request 5; song request 3 $E6 ticks after request 2 ($0567); a tempo-dependent value $0586; the finish music trigger ($055F, $0582, pattern list $EE48 with key-up $50); then the port commands $39, $32, $03 (stopping channels, pausing the music), the timer $0579 and port 1 clears for some channel 5 sounds.
; HOW: Compare chains on $1D, $01, $0394-$0397; SpcSfxStop, SpcSfxStopRequest, SpcMusicPause, SpcSoundQueuePush.
; Entry: none. Exit: A, X, Y changed.
; @spc 11F8
%spc($11F8, $19CE82)
SpcGameStateTick:
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
    mov.b a,!spc_song_request         ; 1222
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
    cmp !spc_song_request,#$02        ; 124D
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
    mov !spc_song_request,#$03        ; 126B
.l126E:
    mov.b a,!spc_tempo+1              ; 126E
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
    beq .l1315                        ; 12A4
    mov.w a,$055F                     ; 12A6
    bne .l12B7                        ; 12A9
    cmp $01,#$1D                      ; 12AB
    bne .l1315                        ; 12AE
    mov a,#$01                        ; 12B0
    mov.w $055F,a                     ; 12B2
    bra .l1315                        ; 12B5
.l12B7:
    mov.w a,$0582                     ; 12B7
    beq .l12CA                        ; 12BA
    dec.w $0582                       ; 12BC
    mov.w a,$0582                     ; 12BF
    cmp a,#$04                        ; 12C2
    beq .l12FA                        ; 12C4
    cmp a,#$01                        ; 12C6
    beq .l130B                        ; 12C8
.l12CA:
    cmp $1D,#$30                      ; 12CA
    bne .l1315                        ; 12CD
    inc.w $055F                       ; 12CF
    mov.w a,$055F                     ; 12D2
    cmp a,#$03                        ; 12D5
    bne .l1315                        ; 12D7
    mov !spc_pattern_ptr,#$48         ; 12D9
    mov !spc_pattern_ptr+1,#$EE       ; 12DC
    set1 !spc_track_flags.0           ; 12DF
    mov.w a,$0586                     ; 12E1
    mov.w $0582,a                     ; 12E4
    mov x,#$00                        ; 12E7
    call SpcSfxTakeVoice              ; 12E9
    inc x                             ; 12EC
    call SpcSfxTakeVoice              ; 12ED
    mov.b a,!spc_master_volume+1      ; 12F0
    mov.w $0587,a                     ; 12F2
    mov !spc_master_volume+1,#$BE     ; 12F5
    bra .l1315                        ; 12F8
.l12FA:
    mov.w a,$0587                     ; 12FA
    mov.b !spc_master_volume+1,a      ; 12FD
    mov x,#$00                        ; 12FF
    call SpcSfxRelease                ; 1301
    mov x,#$01                        ; 1304
    call SpcSfxRelease                ; 1306
    bra .l1315                        ; 1309
.l130B:
    inc.b !spc_transpose              ; 130B
    mov !spc_pattern_ptr,#$48         ; 130D
    mov !spc_pattern_ptr+1,#$EE       ; 1310
    set1 !spc_track_flags.0           ; 1313
.l1315:
    cmp $01,#$1D                      ; 1315
    bne .l1324                        ; 1318
    mov.w a,$0397                     ; 131A
    cmp a,#$0D                        ; 131D
    bne .l1324                        ; 131F
    mov $01,#$00                      ; 1321
.l1324:
    cmp !spc_song_request,#$01        ; 1324
    bne .l133A                        ; 1327
    mov.w a,$055F                     ; 1329
    beq .l133A                        ; 132C
    mov.w a,$0397                     ; 132E
    cmp a,#$0D                        ; 1331
    beq .l133A                        ; 1333
    mov a,#$02                        ; 1335
    call SpcSoundQueuePush            ; 1337
.l133A:
    mov.w a,$056A                     ; 133A
    cmp a,#$F0                        ; 133D
    bne .l1357                        ; 133F
    mov.w a,$055F                     ; 1341
    beq .l1357                        ; 1344
    mov.w a,$0397                     ; 1346
    cmp a,#$0D                        ; 1349
    beq .l1357                        ; 134B
    cmp !spc_song,#$05                ; 134D
    bne .l1357                        ; 1350
    mov a,#$24                        ; 1352
    call SpcSoundQueuePush            ; 1354
.l1357:
    cmp $1D,#$39                      ; 1357
    bne .l1376                        ; 135A
    mov $2A,#$00                      ; 135C  ; not reached in the traces so far
    set1 $2D.3                        ; 135F  ; not reached in the traces so far
    set1 $2D.6                        ; 1361  ; not reached in the traces so far
    mov a,#$01                        ; 1363  ; not reached in the traces so far
    mov.w $0579,a                     ; 1365  ; not reached in the traces so far
    mov x,#$07                        ; 1368  ; not reached in the traces so far
    call SpcSfxStopRequest            ; 136A  ; not reached in the traces so far
    dec x                             ; 136D  ; not reached in the traces so far
    call SpcSfxStopRequest            ; 136E  ; not reached in the traces so far
    mov x,#$08                        ; 1371  ; not reached in the traces so far
    call SpcSfxStopIf                 ; 1373  ; not reached in the traces so far
.l1376:
    cmp $1D,#$32                      ; 1376
    bne .l139C                        ; 1379
    set1 $2D.5                        ; 137B
    mov x,#$04                        ; 137D
    call SpcSfxStop                   ; 137F
    mov a,#$00                        ; 1382
    mov.b $2A,a                       ; 1384
    mov.b !spc_song,a                 ; 1386
    call SpcMusicPause                ; 1388
    set1 $2D.3                        ; 138B
    set1 $2D.6                        ; 138D
    mov x,#$02                        ; 138F
    call SpcSfxStop                   ; 1391
    dec x                             ; 1394
    call SpcSfxStop                   ; 1395
    dec x                             ; 1398
    call SpcSfxStop                   ; 1399
.l139C:
    cmp $1D,#$03                      ; 139C
    bne .l13B3                        ; 139F
    set1 $2D.5                        ; 13A1
    mov x,#$05                        ; 13A3
    call SpcSfxStop                   ; 13A5
    dec x                             ; 13A8
    call SpcSfxStop                   ; 13A9
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
    call SpcSfxStop                   ; 13F6  ; not reached in the traces so far
.l13F9:
    ret                               ; 13F9

; @routine 19:D084 SpcCommand4C
; WHAT IT DOES: Port command $4C: channels 7 and 6 stopped with their fades (SpcSfxStopRequest), 5, 4 and 3 stopped, the timer $0579 started, bits 3 and 6 of $2D set, $2A cleared, the music paused.
; HOW: CALLs with X counting down.
; Entry: none. Exit: as SpcMusicPause.
; @spc 13FA
%spc($13FA, $19D084)
SpcCommand4C:
    cmp $1D,#$4C                      ; 13FA
    bne .l1423                        ; 13FD
    mov x,#$07                        ; 13FF
    call SpcSfxStopRequest            ; 1401
    dec x                             ; 1404
    call SpcSfxStopRequest            ; 1405
    dec x                             ; 1408
    call SpcSfxStop                   ; 1409
    dec x                             ; 140C
    call SpcSfxStop                   ; 140D
    dec x                             ; 1410
    call SpcSfxStop                   ; 1411
    mov a,#$01                        ; 1414
    mov.w $0579,a                     ; 1416
    set1 $2D.3                        ; 1419
    mov $2A,#$00                      ; 141B
    call SpcMusicPause                ; 141E
    set1 $2D.6                        ; 1421
.l1423:
    ret                               ; 1423

; @routine 19:D0AE SpcSoundQueuePush
; WHAT IT DOES: Appends sound A to the zero-ended queue at $0588.
; HOW: Scans for the first zero byte; X kept on the stack.
; Entry: A = sound. Exit: A, X kept.
; @spc 1424
%spc($1424, $19D0AE)
SpcSoundQueuePush:
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

; @routine 19:D0C0 SpcCommandTimers
; WHAT IT DOES: Unless stopped (bit 0 of $29): command $3E starts the count $0568, which at $2A queues sound $0F or $2F by the track $0397; commands $20 and $16 mark bits 4 and 5 of $2C, become command $1B or $27 by the track and start the count $055D, which at $11 queues $20 or $16; command $37 counts $0580, and command $30 queues $43 when that count is $0A; command $29 queues itself again after a random delay ($0560, $0561).
; HOW: Byte counters in RAM; SpcSoundQueuePush.
; Entry: none. Exit: A, X, Y changed.
; @spc 1436
%spc($1436, $19D0C0)
SpcCommandTimers:
    bbs !spc_flags.0,SpcSoundQueuePush_l1435; 1436
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
    bne .l1475                        ; 1450
    mov.w a,$0397                     ; 1452
    cmp a,#$01                        ; 1455
    beq .l146B                        ; 1457
    cmp a,#$04                        ; 1459
    beq .l1467                        ; 145B
    cmp a,#$07                        ; 145D
    beq .l146B                        ; 145F
    cmp a,#$0A                        ; 1461
    beq .l1467                        ; 1463
    bra .l1470                        ; 1465
.l1467:
    mov a,#$0F                        ; 1467
    bra .l146D                        ; 1469
.l146B:
    mov a,#$2F                        ; 146B
.l146D:
    call SpcSoundQueuePush            ; 146D
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
    set1 !spc_track_flags.4           ; 1481
    bra .l1487                        ; 1483
.l1485:
    set1 !spc_track_flags.5           ; 1485
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
    beq .l14AB                        ; 14A6
    inc.w $055D                       ; 14A8
.l14AB:
    mov.w a,$055D                     ; 14AB
    cmp a,#$11                        ; 14AE
    bne .l14CF                        ; 14B0
    mov a,#$00                        ; 14B2
    mov.w $055D,a                     ; 14B4
    bbs !spc_track_flags.4,.l14BF     ; 14B7
    bbs !spc_track_flags.5,.l14C8     ; 14BA
    bra .l14CF                        ; 14BD  ; not reached in the traces so far
.l14BF:
    clr1 !spc_track_flags.4           ; 14BF
    mov a,#$20                        ; 14C1
    call SpcSoundQueuePush            ; 14C3
    bra .l14CF                        ; 14C6
.l14C8:
    clr1 !spc_track_flags.5           ; 14C8
    mov a,#$16                        ; 14CA
    call SpcSoundQueuePush            ; 14CC
.l14CF:
    cmp $1D,#$37                      ; 14CF
    bne .l14D7                        ; 14D2
    inc.w $0580                       ; 14D4
.l14D7:
    cmp $1D,#$30                      ; 14D7
    bne .l14ED                        ; 14DA
    mov.w a,$0580                     ; 14DC
    cmp a,#$0A                        ; 14DF
    bne .l14E8                        ; 14E1
    mov a,#$43                        ; 14E3  ; not reached in the traces so far
    call SpcSoundQueuePush            ; 14E5  ; not reached in the traces so far
.l14E8:
    mov a,#$00                        ; 14E8
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
    mov.b a,!spc_random               ; 14FF  ; not reached in the traces so far
    and a,#$3F                        ; 1501  ; not reached in the traces so far
    adc a,#$1E                        ; 1503  ; not reached in the traces so far
    mov.w $0561,a                     ; 1505  ; not reached in the traces so far
    bra .l1522                        ; 1508  ; not reached in the traces so far
.l150A:
    inc.w $0560                       ; 150A  ; not reached in the traces so far
    cmp.w a,$0561                     ; 150D  ; not reached in the traces so far
    bne .l1522                        ; 1510  ; not reached in the traces so far
    mov a,#$29                        ; 1512  ; not reached in the traces so far
    call SpcSoundQueuePush            ; 1514  ; not reached in the traces so far
    mov $1D,#$29                      ; 1517  ; not reached in the traces so far
    mov a,#$00                        ; 151A  ; not reached in the traces so far
    mov.w $0560,a                     ; 151C  ; not reached in the traces so far
    mov.w $0561,a                     ; 151F  ; not reached in the traces so far
.l1522:
    ret                               ; 1522

; @routine 19:D1AD SpcTempoBend
; WHAT IT DOES: The tempo bend (SpcTempoBendStart decides it): while bit 2 of $2C is set, each tick lowers the transpose by the step count / $15 and the tempo by an accumulated amount from its saved values ($0566, $0578); bit 3 restores them and stops.
; HOW: Word accumulator $0553/$0554, DIV by $15.
; Entry: none. Exit: A = the tempo high byte.
; @spc 1523
%spc($1523, $19D1AD)
SpcTempoBend:
    call SpcTempoBendStart            ; 1523
    bbs !spc_track_flags.2,.l152D     ; 1526
    bbs !spc_track_flags.3,.l1538     ; 1529
.l152C:
    ret                               ; 152C
.l152D:
    mov.w a,$0555                     ; 152D
    beq .l1554                        ; 1530
    cmp a,#$FE                        ; 1532
    beq .l152C                        ; 1534
    bra .l156C                        ; 1536
.l1538:
    clr1 !spc_track_flags.2           ; 1538  ; not reached in the traces so far
    clr1 !spc_track_flags.3           ; 153A  ; not reached in the traces so far
    mov a,#$00                        ; 153C  ; not reached in the traces so far
    mov.w $0555,a                     ; 153E  ; not reached in the traces so far
    mov.w $0553,a                     ; 1541  ; not reached in the traces so far
    mov.w $0554,a                     ; 1544  ; not reached in the traces so far
    mov.w $0569,a                     ; 1547  ; not reached in the traces so far
    mov.w a,$0578                     ; 154A  ; not reached in the traces so far
    mov.b !spc_transpose,a            ; 154D  ; not reached in the traces so far
    mov.w a,$0566                     ; 154F  ; not reached in the traces so far
    bra .l159E                        ; 1552  ; not reached in the traces so far
.l1554:
    mov a,#$04                        ; 1554
    mov.w $0554,a                     ; 1556
    mov.b a,!spc_tempo+1              ; 1559
    mov.w $0566,a                     ; 155B
    mov.b a,!spc_transpose            ; 155E
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
    mov.b !spc_transpose,a            ; 157F
    mov.w a,$0569                     ; 1581
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
    mov.b !spc_tempo+1,a              ; 159E
    ret                               ; 15A0

; @routine 19:D22B SpcTempoBendStart
; WHAT IT DOES: Command $2C during song 1 starts the tempo bend (bit 2 of $2C); commands $30 and $3C during a bend end it (bit 3).
; HOW: SET1/CLR1 on $2C.
; Entry: none. Exit: flags from the compares.
; @spc 15A1
%spc($15A1, $19D22B)
SpcTempoBendStart:
    mov.b a,$1D                       ; 15A1
    cmp a,#$2C                        ; 15A3
    beq .l15B5                        ; 15A5
    mov.w x,$0555                     ; 15A7
    beq .l15BE                        ; 15AA
    cmp a,#$30                        ; 15AC
    beq .l15BF                        ; 15AE
    cmp a,#$3C                        ; 15B0
    beq .l15BF                        ; 15B2
    ret                               ; 15B4
.l15B5:
    cmp !spc_song,#$01                ; 15B5
    bne .l15BE                        ; 15B8
    set1 !spc_track_flags.2           ; 15BA
    clr1 !spc_track_flags.3           ; 15BC
.l15BE:
    ret                               ; 15BE
.l15BF:
    clr1 !spc_track_flags.2           ; 15BF  ; not reached in the traces so far
    set1 !spc_track_flags.3           ; 15C1  ; not reached in the traces so far
    ret                               ; 15C3  ; not reached in the traces so far

; @routine 19:D24E SpcEchoTransition
; WHAT IT DOES: The echo transition by the state $0551 (0 idle; bit 0 or 1 of $28 starts it): the echo voices, feedback and volumes saved ($0570-$0573), then the step count $0550 runs up or down (SpcEchoTransitionStep) with every voice echoing ($4A bits 6, 7) and the feedback and echo volume interpolated from the saved values toward $64 by the count; state 3 restores the saved settings.
; HOW: MUL and DIV by $3C for the interpolation.
; Entry: none (unless bit 0 of $29). Exit: A, X, Y changed.
; @spc 15C4
%spc($15C4, $19D24E)
SpcEchoTransition:
    bbs !spc_flags.0,.l15E3           ; 15C4
    mov.w a,$0550                     ; 15C7
    bne .l1618                        ; 15CA
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
    mov.b a,!spc_echo_voices          ; 15E4
    mov.w $0570,a                     ; 15E6
    mov.b a,!spc_echo_feedback        ; 15E9
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
    mov.b !spc_echo_voices,a          ; 1601
    mov.w a,$0571                     ; 1603
    mov.b !spc_echo_feedback,a        ; 1606
    mov.w a,$0572                     ; 1608
    mov.b $61,a                       ; 160B
    mov.w a,$0573                     ; 160D
    mov.b $63,a                       ; 1610
    mov a,#$00                        ; 1612
    mov.w $0551,a                     ; 1614
    ret                               ; 1617
.l1618:
    mov !spc_echo_voices,#$00         ; 1618
    set1 !spc_echo_voices.7           ; 161B
    set1 !spc_echo_voices.6           ; 161D
    call SpcEchoTransitionStep        ; 161F
    bra SpcEchoTransitionStep_l167C   ; 1622

; @routine 19:D2AE SpcEchoTransitionStep
; WHAT IT DOES: Steps the echo transition count $0550 by the state $0551 and the bits of $28 (up to $1E or $3C, down to $3C), changing the state at the ends.
; HOW: Compare chains.
; Entry: none. Exit: flags from the last compare.
; @spc 1624
%spc($1624, $19D2AE)
SpcEchoTransitionStep:
    mov.w a,$0551                     ; 1624
    cmp a,#$01                        ; 1627
    beq .l163E                        ; 1629
    cmp a,#$02                        ; 162B
    beq .l1656                        ; 162D
    dec.w $0550                       ; 162F
    mov.w a,$0550                     ; 1632
    cmp a,#$3C                        ; 1635
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
    mov.b !spc_echo_feedback,a        ; 1699
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

; @routine 19:D33B SpcPort1Clamp
; WHAT IT DOES: Outside song 1, a port 1 command of 7 or more is ignored ($01 cleared).
; HOW: SBC and BMI.
; Entry: none. Exit: flags.
; @spc 16B1
%spc($16B1, $19D33B)
SpcPort1Clamp:
    cmp !spc_song,#$01                ; 16B1
    beq .l16C0                        ; 16B4
    mov.b a,$01                       ; 16B6
    setc                              ; 16B8
    sbc a,#$07                        ; 16B9
    bmi .l16C0                        ; 16BB
    mov $01,#$00                      ; 16BD
.l16C0:
    ret                               ; 16C0

; @routine 19:D34B SpcCommandFilter
; WHAT IT DOES: Outside song 1, a port command $1D not in the list at $16D8 is dropped.
; HOW: A zero-ended list scan.
; Entry: none. Exit: X = list index.
; @spc 16C1
%spc($16C1, $19D34B)
SpcCommandFilter:
    cmp !spc_song,#$01                ; 16C1
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

; @routine 19:D37B SpcPitchRampB
; WHAT IT DOES: Bit 2 of $28 ramps the offset $054D/$054F up (SpcPitchRampBUp), bit 3 down (SpcPitchRampBDown).
; HOW: BBC tests.
; Entry: none. Exit: as the ramp leaves it.
; @spc 16F1
%spc($16F1, $19D37B)
SpcPitchRampB:
    bbc $28.2,.l16F7                  ; 16F1
    call SpcPitchRampBUp              ; 16F4
.l16F7:
    bbc $28.3,.l16FD                  ; 16F7
    call SpcPitchRampBDown            ; 16FA
.l16FD:
    ret                               ; 16FD

; @routine 19:D388 SpcPitchRampBUp
; WHAT IT DOES: Up to $14 steps (count $0547), adds $20 to the pitch offset $054D/$054F.
; HOW: ADDW through $20/$21.
; Entry: none. Exit: YA = the offset.
; @spc 16FE
%spc($16FE, $19D388)
SpcPitchRampBUp:
    mov.w a,$0547                     ; 16FE
    cmp a,#$14                        ; 1701
    beq .l171E                        ; 1703
    inc.w $0547                       ; 1705
    mov.w a,$054D                     ; 1708
    mov.b $20,a                       ; 170B
    mov.w a,$054F                     ; 170D
    mov.b $21,a                       ; 1710
    mov a,#$20                        ; 1712
    mov y,#$00                        ; 1714
    addw ya,$20                       ; 1716
    mov.w $054D,a                     ; 1718
    mov.w $054F,y                     ; 171B
.l171E:
    ret                               ; 171E

; @routine 19:D3A9 SpcPitchRampBDown
; WHAT IT DOES: Counts $0547 down, taking $20 from the offset; at 0 clears bits 2 and 3 of $28 and the offset.
; HOW: SUBW through $20/$21.
; Entry: none. Exit: A = 0 at the end.
; @spc 171F
%spc($171F, $19D3A9)
SpcPitchRampBDown:
    mov.w a,$0547                     ; 171F
    cmp a,#$00                        ; 1722
    beq .l173E                        ; 1724
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
    mov.w $054F,a                     ; 1747
    ret                               ; 174A

; @routine 19:D3D5 SpcPitchRampA
; WHAT IT DOES: Bit 2 of $29 ramps the offset $0549/$054B up (SpcPitchRampAUp), bit 3 down (SpcPitchRampADown).
; HOW: BBC tests.
; Entry: none. Exit: as the ramp leaves it.
; @spc 174B
%spc($174B, $19D3D5)
SpcPitchRampA:
    bbc !spc_flags.2,.l1751           ; 174B
    call SpcPitchRampAUp              ; 174E
.l1751:
    bbc !spc_flags.3,.l1757           ; 1751
    call SpcPitchRampADown            ; 1754
.l1757:
    ret                               ; 1757

; @routine 19:D3E2 SpcPitchRampAUp
; WHAT IT DOES: Up to $19 steps (count $0543), adds $3C to the offset $0549/$054B.
; HOW: ADDW through $20/$21.
; Entry: none. Exit: YA = the offset.
; @spc 1758
%spc($1758, $19D3E2)
SpcPitchRampAUp:
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

; @routine 19:D403 SpcPitchRampADown
; WHAT IT DOES: Counts $0543 down, taking $3C from the offset; at 0 clears bits 2 and 3 of $29 and the offset.
; HOW: SUBW through $20/$21.
; Entry: none. Exit: A = 0 at the end.
; @spc 1779
%spc($1779, $19D403)
SpcPitchRampADown:
    mov.w a,$0543                     ; 1779
    cmp a,#$00                        ; 177C
    beq .l1798                        ; 177E
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
    clr1 !spc_flags.2                 ; 1798
    clr1 !spc_flags.3                 ; 179A
    mov a,#$00                        ; 179C
    mov.w $0549,a                     ; 179E
    mov.w $054B,a                     ; 17A1
    ret                               ; 17A4

; @routine 19:D42F SpcPortCommand
; WHAT IT DOES: The port 0 command $00 (bit 7 dropped): 6 and up is a sound command, $1D = value - 5; 1-5 (unless stopped, bit 0 of $29) is a song request, which also clears the tempo bend and fade state and flags 0, 1, 6 of $29 and 2, 3 of $2C; 0 leaves $1D = 0.
; HOW: SBC #6 and BMI.
; Entry: none. Exit: A as stored.
; @spc 17A5
%spc($17A5, $19D42F)
SpcPortCommand:
    mov $1D,#$00                      ; 17A5
    mov.b a,!spc_port_in              ; 17A8
    and a,#$7F                        ; 17AA
    beq .l17B7                        ; 17AC
    setc                              ; 17AE
    sbc a,#$06                        ; 17AF
    bmi .l17BA                        ; 17B1
    inc a                             ; 17B3
    mov.b $1D,a                       ; 17B4
    ret                               ; 17B6
.l17B7:
    mov.b $1D,a                       ; 17B7
    ret                               ; 17B9
.l17BA:
    bbs !spc_flags.0,.l17DE           ; 17BA
    mov.b a,!spc_port_in              ; 17BD
    mov.b !spc_song_request,a         ; 17BF
    cmp a,#$02                        ; 17C1
    bne .l17C5                        ; 17C3
.l17C5:
    mov a,#$00                        ; 17C5
    mov.b $2E,a                       ; 17C7
    mov.b $2F,a                       ; 17C9
    mov.w $0553,a                     ; 17CB
    mov.w $0554,a                     ; 17CE
    mov.w $0555,a                     ; 17D1
    clr1 !spc_flags.0                 ; 17D4
    clr1 !spc_flags.1                 ; 17D6
    clr1 !spc_flags.6                 ; 17D8
    clr1 !spc_track_flags.2           ; 17DA
    clr1 !spc_track_flags.3           ; 17DC
.l17DE:
    ret                               ; 17DE

; @routine 19:D469 SpcPort1Command
; WHAT IT DOES: A new port 1 command ($01 differing from the last, $09; below $3E) runs its handler from the table at $1804: the handlers set flags in $28, $29, $2A and $2D, start or stop sounds and loops, ramps, the echo transition, pause, resume and fade the music.
; HOW: JMP ($1804+X); the handlers end in RETs or tail jumps.
; Entry: none. Exit: as the handler leaves it.
; @spc 17DF
%spc($17DF, $19D469)
SpcPort1Command:
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
    beq SpcPortCommand_l17DE          ; 17FC
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
    bbs $2D.3,.l18D5                  ; 187E
    set1 $2A.6                        ; 1881
    mov a,#$02                        ; 1883
    bra .l18A9                        ; 1885
    bbs $2D.6,.l18D5                  ; 1887
    set1 $2A.2                        ; 188A
    mov a,#$25                        ; 188C
    bra .l18A9                        ; 188E
    bbs $2D.6,.l18D5                  ; 1890
    set1 $2A.4                        ; 1893
    mov a,#$15                        ; 1895
    bra .l18A9                        ; 1897
    bbs $2D.6,.l18D5                  ; 1899
    set1 $2A.0                        ; 189C
    mov a,#$17                        ; 189E
    bra .l18A9                        ; 18A0
    bbs $2D.6,.l18D5                  ; 18A2
    set1 $2A.7                        ; 18A5
    mov a,#$21                        ; 18A7
.l18A9:
    jmp SpcSoundQueuePush             ; 18A9
    bbs !spc_flags.2,.l18D5           ; 18AC
    set1 !spc_flags.2                 ; 18AF
    clr1 !spc_flags.3                 ; 18B1
    mov a,#$00                        ; 18B3
    mov y,#$02                        ; 18B5
    bra .l18CE                        ; 18B7
    bbc !spc_flags.2,.l18D5           ; 18B9
    clr1 !spc_flags.2                 ; 18BC
    set1 !spc_flags.3                 ; 18BE
    mov.w a,$0549                     ; 18C0
    mov.w y,$054B                     ; 18C3
    mov $20,#$00                      ; 18C6
    mov $21,#$02                      ; 18C9
    subw ya,$20                       ; 18CC
.l18CE:
    mov.w $0549,a                     ; 18CE
    mov.w $054B,y                     ; 18D1
    ret                               ; 18D4
.l18D5:
    ret                               ; 18D5
    bbs $28.2,.l18D5                  ; 18D6
    set1 $28.2                        ; 18D9
    clr1 $28.3                        ; 18DB
    mov a,#$40                        ; 18DD
    mov y,#$00                        ; 18DF
    bra .l18F8                        ; 18E1
    bbc $28.2,.l18D5                  ; 18E3
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
    mov.b a,$20                       ; 18FF
    setc                              ; 1901
    sbc a,#$32                        ; 1902
    mov.w $0530,a                     ; 1904
    ret                               ; 1907

; @routine 19:D592 SpcSfxStopIf
; WHAT IT DOES: Clears bit 6 of $2A and, when channel 4's sound ($0394) is 2, requests channel 4's stop with the reason $20; the entries after it do the same for channel 5 and its sounds $25, $15, $17, $21.
; HOW: The stop request at $03E0 + X when the channel is active ($0408 + X).
; Entry: $20 = reason. Exit: A, X changed.
; @spc 1908
%spc($1908, $19D592)
SpcSfxStopIf:
    clr1 $2A.6                        ; 1908
    mov.w a,$0394                     ; 190A
    cmp a,#$02                        ; 190D
    bne .l194B                        ; 190F
    mov x,#$04                        ; 1911
    bra .l1941                        ; 1913
.l1915:
    clr1 $2A.2                        ; 1915
    mov.w a,$0395                     ; 1917
    cmp a,#$25                        ; 191A
    bne .l194B                        ; 191C
    bra .l193F                        ; 191E
.l1920:
    clr1 $2A.4                        ; 1920
    mov.w a,$0395                     ; 1922
    cmp a,#$15                        ; 1925
    bne .l194B                        ; 1927
    bra .l193F                        ; 1929
.l192B:
    clr1 $2A.0                        ; 192B
    mov.w a,$0395                     ; 192D
    cmp a,#$17                        ; 1930
    bne .l194B                        ; 1932
    bra .l193F                        ; 1934
.l1936:
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

; @routine 19:D5D6 SpcSfxStop
; WHAT IT DOES: Requests the stop of channel X (reason 1) when it is active.
; HOW: $20 = 1, then SpcSfxStopIf's request.
; Entry: X = channel (0-7). Exit: A changed.
; @spc 194C
%spc($194C, $19D5D6)
SpcSfxStop:
    mov $20,#$01                      ; 194C
    bra SpcSfxStopIf_l1941            ; 194F

; @routine 19:D5DB SpcEchoModeA
; WHAT IT DOES: Port 1 handler: $28 bit 0 set, bit 1 cleared (the echo transition's first mode).
; HOW: SET1/CLR1.
; Entry: none. Exit: none changed but $28.
; @spc 1951
%spc($1951, $19D5DB)
SpcEchoModeA:
    clr1 $28.1                        ; 1951
    set1 $28.0                        ; 1953
    ret                               ; 1955
    clr1 $28.0                        ; 1956
    clr1 $28.1                        ; 1958
    ret                               ; 195A
    clr1 $28.0                        ; 195B
    set1 $28.1                        ; 195D
.l195F:
    ret                               ; 195F
    bbc !spc_flags.0,.l195F           ; 1960
    mov a,#$4C                        ; 1963
    call SpcSoundQueuePush            ; 1965
    clr1 !spc_flags.0                 ; 1968
    mov a,#$00                        ; 196A
    mov.w $045F,a                     ; 196C
    mov.w $045E,a                     ; 196F
    mov.w $045D,a                     ; 1972
    mov.w $045C,a                     ; 1975
    mov.w $04FF,a                     ; 1978
    mov.w $04FE,a                     ; 197B
    mov.w $04FD,a                     ; 197E
    mov.w $04FC,a                     ; 1981
    call SpcMusicPause                ; 1984
    jmp $1798                         ; 1987
    bbc !spc_flags.0,.l195F           ; 198A
    mov $2A,$2B                       ; 198D
    bbc !spc_flags.7,.l19A9           ; 1990
    clr1 !spc_flags.7                 ; 1993  ; not reached in the traces so far
    mov.w a,$0574                     ; 1995  ; not reached in the traces so far
    mov.b !spc_echo_voices,a          ; 1998  ; not reached in the traces so far
    mov.w a,$0575                     ; 199A  ; not reached in the traces so far
    mov.b !spc_echo_feedback,a        ; 199D  ; not reached in the traces so far
    mov.w a,$0576                     ; 199F  ; not reached in the traces so far
    mov.b $61,a                       ; 19A2  ; not reached in the traces so far
    mov.w a,$0577                     ; 19A4  ; not reached in the traces so far
    mov.b $63,a                       ; 19A7  ; not reached in the traces so far
.l19A9:
    mov a,#$4B                        ; 19A9
    call SpcSoundQueuePush            ; 19AB
    clr1 !spc_flags.0                 ; 19AE
    clr1 $2D.6                        ; 19B0
    mov x,#$07                        ; 19B2
    call SpcSfxChannelResume          ; 19B4
    dec x                             ; 19B7
    call SpcSfxChannelResume          ; 19B8
    dec x                             ; 19BB
    call SpcSfxChannelResume          ; 19BC
    dec x                             ; 19BF
    call SpcSfxChannelResume          ; 19C0
    mov x,#$01                        ; 19C3
    call SpcSfxStop                   ; 19C5
    dec x                             ; 19C8
    jmp SpcSfxStop                    ; 19C9

; @routine 19:D656 SpcSfxChannelResume
; WHAT IT DOES: Channel X after a pause: its sound number $03C8 back from $0460, and a sound saved at $0458 restarted (SpcSfxPointer, SpcSfxStart); without one the channel stops.
; HOW: Two table copies and CALLs.
; Entry: X = channel. Exit: X kept.
; @spc 19CC
%spc($19CC, $19D656)
SpcSfxChannelResume:
    mov.w a,$0460+x                   ; 19CC
    mov.w $03C8+x,a                   ; 19CF
    mov.w a,$0458+x                   ; 19D2
    beq .l19E7                        ; 19D5
    push a                            ; 19D7
    call SpcSfxPointer                ; 19D8
    mov a,#$00                        ; 19DB
    mov.w $0458+x,a                   ; 19DD
    pop a                             ; 19E0
    push x                            ; 19E1
    call SpcSfxStart                  ; 19E2
    pop x                             ; 19E5
    ret                               ; 19E6
.l19E7:
    jmp SpcSfxStop                    ; 19E7

; @routine 19:D674 SpcSfxPointer
; WHAT IT DOES: The data pointer of sound A from the table at $0702 into $24/$25.
; HOW: (A - 1) * 2 as the index.
; Entry: A = sound (1 and up). Exit: Y = index + 1.
; @spc 19EA
%spc($19EA, $19D674)
SpcSfxPointer:
    dec a                             ; 19EA
    asl a                             ; 19EB
    mov y,a                           ; 19EC
    mov.w a,$0702+y                   ; 19ED
    mov.b $24,a                       ; 19F0
    inc y                             ; 19F2
    mov.w a,$0702+y                   ; 19F3
    mov.b $25,a                       ; 19F6
    ret                               ; 19F8
    bbs !spc_flags.0,SpcSfxChannelPause_l1A67; 19F9
    mov $2B,$2A                       ; 19FC
    mov $2A,#$00                      ; 19FF
    mov.w a,$0551                     ; 1A02
    beq .l1A2B                        ; 1A05
    set1 !spc_flags.7                 ; 1A07  ; not reached in the traces so far
    set1 $2D.6                        ; 1A09  ; not reached in the traces so far
    mov.b a,!spc_echo_voices          ; 1A0B  ; not reached in the traces so far
    mov.w $0574,a                     ; 1A0D  ; not reached in the traces so far
    mov.b a,!spc_echo_feedback        ; 1A10  ; not reached in the traces so far
    mov.w $0575,a                     ; 1A12  ; not reached in the traces so far
    mov.b a,$61                       ; 1A15  ; not reached in the traces so far
    mov.w $0576,a                     ; 1A17  ; not reached in the traces so far
    mov.b a,$63                       ; 1A1A  ; not reached in the traces so far
    mov.w $0577,a                     ; 1A1C  ; not reached in the traces so far
    mov a,#$50                        ; 1A1F  ; not reached in the traces so far
    mov.b !spc_echo_feedback,a        ; 1A21  ; not reached in the traces so far
    mov a,#$11                        ; 1A23  ; not reached in the traces so far
    mov.b $61,a                       ; 1A25  ; not reached in the traces so far
    mov a,#$11                        ; 1A27  ; not reached in the traces so far
    mov.b $63,a                       ; 1A29  ; not reached in the traces so far
.l1A2B:
    set1 !spc_flags.0                 ; 1A2B
    mov a,#$4A                        ; 1A2D
    call SpcSoundQueuePush            ; 1A2F
    mov x,#$07                        ; 1A32
    call SpcSfxChannelPause           ; 1A34
    dec x                             ; 1A37
    call SpcSfxChannelPause           ; 1A38
    dec x                             ; 1A3B
    call SpcSfxChannelPause           ; 1A3C
    dec x                             ; 1A3F
    call SpcSfxChannelPause           ; 1A40
    mov x,#$07                        ; 1A43
    call SpcSfxStop                   ; 1A45
    dec x                             ; 1A48
    call SpcSfxStop                   ; 1A49
    dec x                             ; 1A4C
    call SpcSfxStop                   ; 1A4D
    dec x                             ; 1A50
    jmp SpcSfxStop                    ; 1A51

; @routine 19:D6DE SpcSfxChannelPause
; WHAT IT DOES: Channel X at a pause: a pending queued sound or a running looped one ($0480) is saved at $0458 with its sound number, to restart after.
; HOW: Compares on $04F8, $0480.
; Entry: X = channel. Exit: A changed.
; @spc 1A54
%spc($1A54, $19D6DE)
SpcSfxChannelPause:
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

; @routine 19:D70D SpcSfxStopAll
; WHAT IT DOES: Stops every sound effect channel with a fade, $2A cleared, and the music fade-out started (SpcFadeOutStart).
; HOW: SpcSfxStopRequest for X = 7 down to 0.
; Entry: none. Exit: X = $FF.
; @spc 1A83
%spc($1A83, $19D70D)
SpcSfxStopAll:
    mov $2A,#$00                      ; 1A83
    call SpcFadeOutStart              ; 1A86
    mov x,#$07                        ; 1A89
.l1A8B:
    call SpcSfxStopRequest            ; 1A8B
    dec x                             ; 1A8E
    bpl .l1A8B                        ; 1A8F
    ret                               ; 1A91

; @routine 19:D71C SpcSfxStopRequest
; WHAT IT DOES: Channel X's queued and looping state cleared ($04F8, $0480, $2A) and, if active, its fade-out started ($04C0, $04C8 = 1).
; HOW: Stores.
; Entry: X = channel. Exit: A = 0 or 1.
; @spc 1A92
%spc($1A92, $19D71C)
SpcSfxStopRequest:
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

; @routine 19:D734 SpcFadeOutStart
; WHAT IT DOES: Unless stopped (bit 0 of $29), starts the music fade-out (bit 6 of $29).
; HOW: SET1.
; Entry: none. Exit: none.
; @spc 1AAA
%spc($1AAA, $19D734)
SpcFadeOutStart:
    bbs !spc_flags.0,.l1AAF           ; 1AAA
    set1 !spc_flags.6                 ; 1AAD
.l1AAF:
    ret                               ; 1AAF

; @routine 19:D73A SpcMusicFade
; WHAT IT DOES: The music fade: while bit 6 of $29 is set, the fade amount $2E/$2F grows by $2F0 a tick and the volume scale $0565 is the start level $0564 less it; at the bottom the fade ends and, by flags, the driver reloads (bit 7 of $2C: all keys off, SpcReload), returns to the IPL ROM (bit 0 of $2D) or pauses the music. Without a fade, the scale follows the master volume.
; HOW: SUBW from $0564 in the high byte.
; Entry: none. Exit: as the chosen path leaves it.
; @spc 1AB0
%spc($1AB0, $19D73A)
SpcMusicFade:
    bbc !spc_flags.6,SpcKeyOffAll_l1AF6; 1AB0
    mov a,#$F0                        ; 1AB3
    mov y,#$02                        ; 1AB5
    addw ya,$2E                       ; 1AB7
    mov.b $2E,a                       ; 1AB9
    mov.b $2F,y                       ; 1ABB
    mov a,#$00                        ; 1ABD
    mov.w y,$0564                     ; 1ABF
    subw ya,$2E                       ; 1AC2
    mov.w !spc_volume_scale,y         ; 1AC4
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
    clr1 !spc_flags.6                 ; 1ADE
    bbs !spc_track_flags.7,.l1AE9     ; 1AE0
    bbs $2D.0,SpcKeyOffAll_l1B15      ; 1AE3
    jmp SpcMusicPause                 ; 1AE6  ; not reached in the traces so far
.l1AE9:
    call SpcKeyOffAll                 ; 1AE9
    jmp SpcReload                     ; 1AEC

; @routine 19:D779 SpcKeyOffAll
; WHAT IT DOES: Keys off all eight voices.
; HOW: DSP KOF ($5C) = $FF through SpcDspWrite.
; Entry: none. Exit: A = $FF, Y = $5C.
; @spc 1AEF
%spc($1AEF, $19D779)
SpcKeyOffAll:
    mov a,#$FF                        ; 1AEF
    mov y,#$5C                        ; 1AF1
    jmp SpcDspWrite                   ; 1AF3
.l1AF6:
    mov.b a,!spc_master_volume+1      ; 1AF6
    mov.w $0564,a                     ; 1AF8
    mov.w !spc_volume_scale,a         ; 1AFB
    ret                               ; 1AFE
    call SpcSfxStopAll                ; 1AFF
    mov.b a,!spc_master_volume+1      ; 1B02
    beq SpcMusicFade_l1AE9            ; 1B04
    set1 !spc_track_flags.7           ; 1B06
    ret                               ; 1B08
    set1 $2D.0                        ; 1B09
    call SpcSfxStopAll                ; 1B0B
    mov.b a,!spc_master_volume+1      ; 1B0E
    beq .l1B15                        ; 1B10
    set1 $2D.0                        ; 1B12
    ret                               ; 1B14
.l1B15:
    call SpcKeyOffAll                 ; 1B15
    mov.w a,!spc_control              ; 1B18
    or a,#$80                         ; 1B1B
    mov.w !spc_control,a              ; 1B1D
    jmp $FFC0                         ; 1B20
    mov.b a,$20                       ; 1B23
    setc                              ; 1B25
    sbc a,#$2A                        ; 1B26
    mov.w $0562,a                     ; 1B28
    ret                               ; 1B2B

; @routine 19:D7B6 SpcCommandChannelCheck
; WHAT IT DOES: A sound command $1D outside the list at $26D6 is checked against the list at $26E4: when an earlier-listed sound already plays on its channel, the command is dropped; the command kept is also stored at $1F.
; HOW: List scans; SpcSfxLookup, SpcSfxChannel.
; Entry: none. Exit: X = channel.
; @spc 1B2C
%spc($1B2C, $19D7B6)
SpcCommandChannelCheck:
    mov.b a,$1D                       ; 1B2C
    beq .l1B76                        ; 1B2E
    mov y,#$00                        ; 1B30
.l1B32:
    mov.w a,$26D6+y                   ; 1B32
    beq .l1B3E                        ; 1B35
    cmp.b a,$1D                       ; 1B37
    beq .l1B79                        ; 1B39
    inc y                             ; 1B3B
    bra .l1B32                        ; 1B3C
.l1B3E:
    mov y,#$00                        ; 1B3E
.l1B40:
    mov.w a,$26E4+y                   ; 1B40
    beq .l1B79                        ; 1B43
    push a                            ; 1B45
    mov.b a,$1D                       ; 1B46
    push y                            ; 1B48
    call SpcSfxLookup                 ; 1B49
    call SpcSfxChannel                ; 1B4C
    pop y                             ; 1B4F
    pop a                             ; 1B50
    cmp.w a,$0390+x                   ; 1B51
    beq .l1B59                        ; 1B54
.l1B56:
    inc y                             ; 1B56
    bra .l1B40                        ; 1B57
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

; @routine 19:D804 SpcCommandBlock
; WHAT IT DOES: A sound command in the list at $1B8C is dropped.
; HOW: A zero-ended list scan.
; Entry: none. Exit: X = index.
; @spc 1B7A
%spc($1B7A, $19D804)
SpcCommandBlock:
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

; @routine 19:D81C SpcSfxHolds
; WHAT IT DOES: The channels' hold counters $0468 count up; a new command whose channel is held ($0470) is dropped, and a command in the list at $1C04 starts its channel's hold; a hold ends when its count reaches the list's time.
; HOW: Pairs (sound, time) in the list at $1C04.
; Entry: none. Exit: A, X, Y changed.
; @spc 1B92
%spc($1B92, $19D81C)
SpcSfxHolds:
    mov x,#$00                        ; 1B92
.l1B94:
    mov.w a,$0468+x                   ; 1B94
    beq .l1B9D                        ; 1B97
    inc a                             ; 1B99
    mov.w $0468+x,a                   ; 1B9A
.l1B9D:
    inc x                             ; 1B9D
    cmp x,#$08                        ; 1B9E
    bne .l1B94                        ; 1BA0
    mov.b a,$1D                       ; 1BA2
    beq .l1BD3                        ; 1BA4
    call SpcSfxPointer                ; 1BA6
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
    call SpcSoundQueuePush            ; 1C09
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
    jmp SpcSfxChannelsFrame           ; 1C5A
.l1C5D:
    mov.b $1E,a                       ; 1C5D  ; not reached in the traces so far
    mov.b a,$1D                       ; 1C5F  ; not reached in the traces so far
    beq .l1C5A                        ; 1C61  ; not reached in the traces so far
.l1C63:
    mov.b $23,a                       ; 1C63
    call SpcSfxLookup                 ; 1C65
    call SpcSfxChannel                ; 1C68
    call SpcSfxMode                   ; 1C6B
    call SpcSfxActivate               ; 1C6E
    call SpcSfxChannelSave            ; 1C71
    mov.b a,$23                       ; 1C74
    mov.w $0410+x,a                   ; 1C76
    push x                            ; 1C79
    call SpcSfxChannelsFrame          ; 1C7A
    pop x                             ; 1C7D
    mov a,#$00                        ; 1C7E
    mov.w $0410+x,a                   ; 1C80
    ret                               ; 1C83

; @routine 19:D90E SpcSfxActivate
; WHAT IT DOES: Marks channel X active ($0408 = 1).
; HOW: Store.
; Entry: X = channel. Exit: A = 1.
; @spc 1C84
%spc($1C84, $19D90E)
SpcSfxActivate:
    mov a,#$01                        ; 1C84
    mov.w $0408+x,a                   ; 1C86
    ret                               ; 1C89

; @routine 19:D914 SpcSfxLookup
; WHAT IT DOES: The data of sound A: its pointer from $0702 into $20/$21 and its first byte (the channel and mode) in X.
; HOW: (A - 1) * 2 as the index; MOV A,($20)+Y.
; Entry: A = sound. Exit: X = header byte, Y = 0.
; @spc 1C8A
%spc($1C8A, $19D914)
SpcSfxLookup:
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

; @routine 19:D928 SpcSfxChannel
; WHAT IT DOES: Keeps the header byte X at $26 and leaves its channel (low four bits) in X.
; HOW: AND #$0F.
; Entry: X = header byte. Exit: X = channel.
; @spc 1C9E
%spc($1C9E, $19D928)
SpcSfxChannel:
    mov.b $26,x                       ; 1C9E
    mov a,x                           ; 1CA0
    and a,#$0F                        ; 1CA1
    mov x,a                           ; 1CA3
    ret                               ; 1CA4

; @routine 19:D92F SpcSfxMode
; WHAT IT DOES: The header's mode bits for channel X: bit 7 marks a looping sound ($0480 = 1, or 2 with bit 5, which also drops a queued one); otherwise a looping channel saves its sound to restart ($04F8).
; HOW: BBC tests on $26.
; Entry: X = channel, $26 = header. Exit: A changed.
; @spc 1CA5
%spc($1CA5, $19D92F)
SpcSfxMode:
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
    bne .l1CCC                        ; 1CBF
    mov.w a,$04F8+x                   ; 1CC1
    bne .l1CCC                        ; 1CC4
    mov.w a,$0390+x                   ; 1CC6
    mov.w $04F8+x,a                   ; 1CC9
.l1CCC:
    ret                               ; 1CCC

; @routine 19:D957 SpcSfxChannelsFrame
; WHAT IT DOES: Runs every active sound effect channel's tick (SpcSfxChannelLoad, SpcSfxChannelStep).
; HOW: X from 0 to 7.
; Entry: none. Exit: X = 8.
; @spc 1CCD
%spc($1CCD, $19D957)
SpcSfxChannelsFrame:
    mov x,#$00                        ; 1CCD
.l1CCF:
    mov.w a,$0408+x                   ; 1CCF
    beq .l1CDA                        ; 1CD2
    call SpcSfxChannelLoad            ; 1CD4
    call SpcSfxChannelStep            ; 1CD7
.l1CDA:
    inc x                             ; 1CDA
    cmp x,#$08                        ; 1CDB
    beq .l1CE1                        ; 1CDD
    bra .l1CCF                        ; 1CDF
.l1CE1:
    ret                               ; 1CE1

; @routine 19:D96C SpcSfxChannelLoad
; WHAT IT DOES: Channel X's data pointer ($0488/$0490) into $20/$21.
; HOW: Two copies.
; Entry: X = channel. Exit: A = the pointer's high byte.
; @spc 1CE2
%spc($1CE2, $19D96C)
SpcSfxChannelLoad:
    mov.w a,$0488+x                   ; 1CE2
    mov.b $20,a                       ; 1CE5
    mov.w a,$0490+x                   ; 1CE7
    mov.b $21,a                       ; 1CEA
    ret                               ; 1CEC

; @routine 19:D977 SpcSfxChannelSave
; WHAT IT DOES: $20/$21 back as channel X's data pointer.
; HOW: Two copies.
; Entry: X = channel. Exit: A = the high byte.
; @spc 1CED
%spc($1CED, $19D977)
SpcSfxChannelSave:
    mov.b a,$20                       ; 1CED
    mov.w $0488+x,a                   ; 1CEF
    mov.b a,$21                       ; 1CF2
    mov.w $0490+x,a                   ; 1CF4
    ret                               ; 1CF7

; @routine 19:D982 SpcSfxChannelStep
; WHAT IT DOES: Channel X's state ($03F8: 0 start, 1 started, 2 and 3 playing, 4 stopping): a new sound ($0410) starts with its voice taken from the music and keyed off; playing, it is keyed on, read (SpcSfxRead), its special handler run (SpcSfxSpecial) and its DSP registers written; stop requests ($03E0) by reason, the end of a non-looping sound, or a fade's end release the voice (SpcSfxRelease) and clear the channel.
; HOW: Compare chains on the state and the request; tail jumps into the shared code.
; Entry: X = channel, $20/$21 its pointer. Exit: A, Y changed.
; @spc 1CF8
%spc($1CF8, $19D982)
SpcSfxChannelStep:
    mov.w a,$03F8+x                   ; 1CF8
    beq .l1D50                        ; 1CFB
    cmp a,#$01                        ; 1CFD
    beq .l1D1C                        ; 1CFF
    cmp a,#$02                        ; 1D01
    beq .l1D31                        ; 1D03
    cmp a,#$03                        ; 1D05
    beq .l1D36                        ; 1D07
    mov.w a,$0410+x                   ; 1D09
    bne .l1D53                        ; 1D0C
    mov a,#$00                        ; 1D0E
    mov.w $0390+x,a                   ; 1D10
    mov.w $03F8+x,a                   ; 1D13
    mov.w $0408+x,a                   ; 1D16
    jmp SpcSfxRelease                 ; 1D19
.l1D1C:
    mov.w a,$0410+x                   ; 1D1C
    bne .l1D53                        ; 1D1F
    mov a,#$02                        ; 1D21
    mov.w $03F8+x,a                   ; 1D23
    call SpcKeyOnVoice                ; 1D26
    mov a,#$00                        ; 1D29
    mov.w $03E8+x,a                   ; 1D2B
.l1D2E:
    jmp $1E9D                         ; 1D2E
.l1D31:
    mov a,#$00                        ; 1D31
    mov.w $03E8+x,a                   ; 1D33
.l1D36:
    mov.w a,$0410+x                   ; 1D36
    bne .l1D53                        ; 1D39
    mov a,#$03                        ; 1D3B
    mov.w $03F8+x,a                   ; 1D3D
    mov.w a,$03E8+x                   ; 1D40
    beq SpcSfxTakeVoice_l1D6C         ; 1D43
    cmp a,#$01                        ; 1D45
    beq .l1D1C                        ; 1D47
    mov a,#$00                        ; 1D49
    mov.w $03F0+x,a                   ; 1D4B
    bra .l1D1C                        ; 1D4E
.l1D50:
    mov.w a,$0410+x                   ; 1D50
.l1D53:
    mov.w $0390+x,a                   ; 1D53
    mov a,#$01                        ; 1D56
    mov.w $03F8+x,a                   ; 1D58
    mov a,#$00                        ; 1D5B
    mov.w $03F0+x,a                   ; 1D5D
    mov.w $03E0+x,a                   ; 1D60

; @routine 19:D9ED SpcSfxTakeVoice
; WHAT IT DOES: Takes voice X from the music (SpcSfxMarkVoice) and keys it off.
; HOW: Two CALLs.
; Entry: X = channel. Exit: as SpcKeyOffVoice.
; @spc 1D63
%spc($1D63, $19D9ED)
SpcSfxTakeVoice:
    call SpcSfxMarkVoice              ; 1D63
    jmp SpcKeyOffVoice                ; 1D66
.l1D69:
    jmp $1DE8                         ; 1D69
.l1D6C:
    mov.w a,$03E0+x                   ; 1D6C
    beq SpcSfxChannelStep_l1D2E       ; 1D6F
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
    bra SpcSfxChannelStep_l1D2E       ; 1D92  ; not reached in the traces so far
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
    call SpcSoundQueuePush            ; 1DF2
.l1DF5:
    mov a,#$04                        ; 1DF5
    mov.w $03F8+x,a                   ; 1DF7
    call SpcKeyOffVoice               ; 1DFA
    mov a,#$00                        ; 1DFD
    mov.w $0480+x,a                   ; 1DFF
    mov.w $04F8+x,a                   ; 1E02
    mov.w $03F0+x,a                   ; 1E05
    mov.w $03C8+x,a                   ; 1E08
    mov.w $04F0+x,a                   ; 1E0B
    ret                               ; 1E0E

; @routine 19:DA99 SpcSfxMarkVoice
; WHAT IT DOES: Marks voice X as a sound effect's ($1A).
; HOW: SpcVoiceMask, OR.
; Entry: X = voice. Exit: A = $1A.
; @spc 1E0F
%spc($1E0F, $19DA99)
SpcSfxMarkVoice:
    call SpcVoiceMask                 ; 1E0F
    mov.b a,!spc_sfx_voices           ; 1E12
    or.b a,$22                        ; 1E14
    mov.b !spc_sfx_voices,a           ; 1E16
    ret                               ; 1E18

; @routine 19:DAA3 SpcVoiceMask
; WHAT IT DOES: $22 = the bit of voice X (1 << X).
; HOW: ROL of a carry X + 1 times.
; Entry: X = voice. Exit: A = $22.
; @spc 1E19
%spc($1E19, $19DAA3)
SpcVoiceMask:
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

; @routine 19:DAB2 SpcVoiceMaskNot
; WHAT IT DOES: $22 = every bit but voice X's.
; HOW: ROL of $FF with a clear carry.
; Entry: X = voice. Exit: A = $22.
; @spc 1E28
%spc($1E28, $19DAB2)
SpcVoiceMaskNot:
    mov.b $22,x                       ; 1E28
    inc.b $22                         ; 1E2A
    mov a,#$FF                        ; 1E2C
    clrc                              ; 1E2E
    bra SpcVoiceMask_l1E20            ; 1E2F

; @routine 19:DABB SpcSfxRelease
; WHAT IT DOES: Gives voice X back to the music: its sound effect bit cleared, the music instrument reloaded (SpcLoadInstrument), its echo bit from the music's echo voices $4F, and its volume rewritten (SpcVoiceVolumeWrite).
; HOW: Masks through $22; X doubled for the music's arrays.
; Entry: X = voice. Exit: X kept.
; @spc 1E31
%spc($1E31, $19DABB)
SpcSfxRelease:
    call SpcVoiceMaskNot              ; 1E31
    mov.b a,!spc_sfx_voices           ; 1E34
    and.b a,$22                       ; 1E36
    mov.b !spc_sfx_voices,a           ; 1E38
    push x                            ; 1E3A
    clrc                              ; 1E3B
    mov a,x                           ; 1E3C
    asl a                             ; 1E3D
    mov x,a                           ; 1E3E
    mov.w a,!spc_instrument+x         ; 1E3F
    call SpcLoadInstrument            ; 1E42
    pop x                             ; 1E45
    mov.b $20,x                       ; 1E46
    inc.b $20                         ; 1E48
    mov.b a,!spc_echo_enable          ; 1E4A
.l1E4C:
    lsr a                             ; 1E4C
    dec.b $20                         ; 1E4D
    bne .l1E4C                        ; 1E4F
    bcc .l1E5E                        ; 1E51
    call SpcVoiceMask                 ; 1E53
    mov.b a,!spc_echo_voices          ; 1E56
    or.b a,$22                        ; 1E58
    mov.b !spc_echo_voices,a          ; 1E5A
    bra .l1E67                        ; 1E5C
.l1E5E:
    call SpcVoiceMaskNot              ; 1E5E
    mov.b a,!spc_echo_voices          ; 1E61
    and.b a,$22                       ; 1E63
    mov.b !spc_echo_voices,a          ; 1E65
.l1E67:
    call SpcVoiceMaskNot              ; 1E67
    mov.b a,!spc_sfx_voices           ; 1E6A
    and.b a,$22                       ; 1E6C
    mov.b !spc_sfx_voices,a           ; 1E6E
    call SpcVoiceMask                 ; 1E70
    mov.b a,!spc_voice_bit            ; 1E73
    or.b a,$22                        ; 1E75
    mov.b !spc_voice_bit,a            ; 1E77
    mov.b a,!spc_volume_dirty         ; 1E79
    or.b a,$20                        ; 1E7B
    mov.b !spc_volume_dirty,a         ; 1E7D
    push x                            ; 1E7F
    clrc                              ; 1E80
    mov a,x                           ; 1E81
    asl a                             ; 1E82
    mov x,a                           ; 1E83
    call SpcVoiceVolumeWrite          ; 1E84
    pop x                             ; 1E87
    ret                               ; 1E88

; @routine 19:DB13 SpcKeyOnVoice
; WHAT IT DOES: Marks voice X for key-on ($45).
; HOW: SpcVoiceMask, OR.
; Entry: X = voice. Exit: A = $45.
; @spc 1E89
%spc($1E89, $19DB13)
SpcKeyOnVoice:
    call SpcVoiceMask                 ; 1E89
    mov.b a,!spc_key_on               ; 1E8C
    or.b a,$22                        ; 1E8E
    mov.b !spc_key_on,a               ; 1E90
    ret                               ; 1E92

; @routine 19:DB1D SpcKeyOffVoice
; WHAT IT DOES: Marks voice X for key-off ($46).
; HOW: SpcVoiceMask, OR.
; Entry: X = voice. Exit: A = $46.
; @spc 1E93
%spc($1E93, $19DB1D)
SpcKeyOffVoice:
    call SpcVoiceMask                 ; 1E93
    mov.b a,!spc_key_off              ; 1E96
    or.b a,$22                        ; 1E98
    mov.b !spc_key_off,a              ; 1E9A
    ret                               ; 1E9C
.l1E9D:
    call SpcSfxRead                   ; 1E9D
    call SpcSfxSpecial                ; 1EA0
    jmp $22D3                         ; 1EA3

; @routine 19:DB30 SpcSfxRead
; WHAT IT DOES: Channel X's sound data for this tick: its age $03F0 counts (a set lifetime $0448 stops it); on the first tick its pitch, volume, slide and envelope state is reset (ADSR $8F/$E0, echo off); while a note's length $0418 runs, the slides step (SpcSfxPitchSlide, volume slides); then bytes are read from ($20)+Y: note numbers set the pitch (table $CAA8, by octave), $00-$7F lengths, and commands $E0 and up run their handler from the table at $1F59 (volume, pan, pitch and volume slides, echo on/off, ADSR, sample, lifetime, jumps, end).
; HOW: JMP ($1F59+X) with X pushed; handlers return into the read loop.
; Entry: X = channel. Exit: as the last handler leaves it.
; @spc 1EA6
%spc($1EA6, $19DB30)
SpcSfxRead:
    mov.w a,$0390+x                   ; 1EA6
    beq .l1EC9                        ; 1EA9
    mov.w a,$0448+x                   ; 1EAB
    beq .l1EBC                        ; 1EAE
    mov.w a,$03F0+x                   ; 1EB0
    setc                              ; 1EB3
    sbc.w a,$0448+x                   ; 1EB4
    bmi .l1EBC                        ; 1EB7
    call SpcSfxStop                   ; 1EB9
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
    call SpcSfxChannelLoad            ; 1ECF
    mov.w a,$03F0+x                   ; 1ED2
    beq .l1EDD                        ; 1ED5
    cmp a,#$01                        ; 1ED7
    beq .l1EDE                        ; 1ED9
    bra .l1F30                        ; 1EDB
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
    mov.w $03D0+x,a                   ; 1F1B
    mov a,#$E0                        ; 1F1E
    mov.w $03D8+x,a                   ; 1F20
    call SpcVoiceMaskNot              ; 1F23
    mov.b a,!spc_echo_voices          ; 1F26
    and.b a,$22                       ; 1F28
    mov.b !spc_echo_voices,a          ; 1F2A
    mov y,#$01                        ; 1F2C
    bra .l1F48                        ; 1F2E
.l1F30:
    mov.w a,$0418+x                   ; 1F30
    dec a                             ; 1F33
    mov.w $0418+x,a                   ; 1F34
    beq .l1F43                        ; 1F37
    cmp a,#$FF                        ; 1F39
    beq .l1F43                        ; 1F3B
    call SpcSfxPitchSlide             ; 1F3D
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
    call SpcKeyOffVoice               ; 1FA7
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
    jmp SpcSfxChannelSave             ; 1FC9
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
    beq .l1FFE                        ; 1FEA
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
.l1FFE:
    pop x                             ; 1FFE
    mov.b a,$22                       ; 1FFF
    mov.w $03B8+x,a                   ; 2001
    mov.w $0498+x,a                   ; 2004
    mov.b a,$23                       ; 2007
    mov.w $03C0+x,a                   ; 2009
    mov.w $04A0+x,a                   ; 200C
    mov.w a,$0440+x                   ; 200F
    beq .l204F                        ; 2012
    push x                            ; 2014
    mov.b $24,a                       ; 2015
    mov.b a,$23                       ; 2017
    setc                              ; 2019
    sbc a,#$10                        ; 201A
    bmi .l2030                        ; 201C
    mov.b a,$22                       ; 201E
    mov.b y,$23                       ; 2020
    mov x,#$43                        ; 2022
    div ya,x                          ; 2024
    mov y,#$04                        ; 2025
    mul ya                            ; 2027
    mov x,#$FC                        ; 2028
    div ya,x                          ; 202A
    mov.b y,$24                       ; 202B
    mul ya                            ; 202D
    bra .l2045                        ; 202E
.l2030:
    mov.b a,$22                       ; 2030
    mov.b y,$23                       ; 2032
    mov x,#$11                        ; 2034
    div ya,x                          ; 2036
    mov x,a                           ; 2037
    mov a,#$FF                        ; 2038
    mov y,#$00                        ; 203A
    div ya,x                          ; 203C
    mov x,a                           ; 203D
    mov y,#$00                        ; 203E
    mov.b a,$24                       ; 2040
    div ya,x                          ; 2042
    mov y,#$00                        ; 2043
.l2045:
    addw ya,$22                       ; 2045
    pop x                             ; 2047
    mov.w $03B8+x,a                   ; 2048
    mov a,y                           ; 204B
    mov.w $03C0+x,a                   ; 204C
.l204F:
    pop y                             ; 204F
.l2050:
    jmp $1F47                         ; 2050
    mov a,#$01                        ; 2053
    bra .l2059                        ; 2055
    mov a,#$02                        ; 2057
.l2059:
    pop x                             ; 2059
    mov.w $0478+x,a                   ; 205A
.l205D:
    bra .l2050                        ; 205D
    pop x                             ; 205F
    inc y                             ; 2060
    mov a,($20)+y                     ; 2061
    mov.w $03C8+x,a                   ; 2063
    bra .l205D                        ; 2066

; @routine 19:DCF2 SpcSfxEngineScale
; WHAT IT DOES: For a channel in engine mode ($04F0 = 4), scales the volume A by the engine speed from port 3 ($03 * 3, / 2, + $80); otherwise A is kept.
; HOW: SpcDiv16, SpcMul16; clamped to $7F.
; Entry: A = volume, X = channel. Exit: A = volume; X, Y kept.
; @spc 2068
%spc($2068, $19DCF2)
SpcSfxEngineScale:
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
    call SpcDiv16                     ; 207D
    mov.b $24,a                       ; 2080
    mov.b $25,y                       ; 2082
    mov a,#$80                        ; 2084
    mov y,#$00                        ; 2086
    addw ya,$24                       ; 2088
    push y                            ; 208A
    pop x                             ; 208B
    mov y,a                           ; 208C
    mov.b a,$22                       ; 208D
    call SpcMul16                     ; 208F
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
    set1 !spc_track_flags.6           ; 209E
    pop x                             ; 20A0
    inc y                             ; 20A1
    mov.w a,$04F0+x                   ; 20A2
    cmp a,#$01                        ; 20A5
    beq .l20AD                        ; 20A7
    cmp a,#$02                        ; 20A9
    beq .l20CA                        ; 20AB
.l20AD:
    mov a,($20)+y                     ; 20AD
    call SpcSfxEngineScale            ; 20AF
    mov.w $0398+x,a                   ; 20B2
    mov.w $04A8+x,a                   ; 20B5
    bbs !spc_track_flags.6,.l20BC     ; 20B8
    inc y                             ; 20BB
.l20BC:
    mov a,($20)+y                     ; 20BC
    call SpcSfxEngineScale            ; 20BE
    mov.w $03A0+x,a                   ; 20C1
    mov.w $04B0+x,a                   ; 20C4
    jmp $20DE                         ; 20C7
.l20CA:
    mov a,($20)+y                     ; 20CA
    mov.w $0398+x,a                   ; 20CC
    mov.w $04A8+x,a                   ; 20CF
    bbs !spc_track_flags.6,.l20D6     ; 20D2
    inc y                             ; 20D5  ; not reached in the traces so far
.l20D6:
    mov a,($20)+y                     ; 20D6
    mov.w $03A0+x,a                   ; 20D8
    mov.w $04B0+x,a                   ; 20DB
.l20DE:
    mov a,#$00                        ; 20DE
    mov.w $03A8+x,a                   ; 20E0
    mov.w $03B0+x,a                   ; 20E3
    clr1 !spc_track_flags.6           ; 20E6
.l20E8:
    jmp $1F47                         ; 20E8
    pop x                             ; 20EB
    inc y                             ; 20EC
    mov a,($20)+y                     ; 20ED
    and a,#$0F                        ; 20EF
    mov.b $22,a                       ; 20F1
    mov.w a,$03D0+x                   ; 20F3
    and a,#$F0                        ; 20F6
    bra .l210B                        ; 20F8
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
    mov.w $03D0+x,a                   ; 210F
.l2112:
    bra .l20E8                        ; 2112
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
    bra .l2135                        ; 2126
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
    bra .l2112                        ; 213A
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
    bra .l213A                        ; 214E
    pop x                             ; 2150
    bbs $28.0,.l2169                  ; 2151
    call SpcVoiceMask                 ; 2154
    mov.b a,!spc_echo_voices          ; 2157
    or.b a,$22                        ; 2159
    mov.b !spc_echo_voices,a          ; 215B
    bra .l2169                        ; 215D
    pop x                             ; 215F
    call SpcVoiceMaskNot              ; 2160
    mov.b a,!spc_echo_voices          ; 2163
    and.b a,$22                       ; 2165
    mov.b !spc_echo_voices,a          ; 2167
.l2169:
    bra .l214E                        ; 2169
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
    call SpcSfxStart                  ; 2195
.l2198:
    pop y                             ; 2198
    pop x                             ; 2199
    bra .l2169                        ; 219A

; @routine 19:DE26 SpcSfxStart
; WHAT IT DOES: Starts sound A (its data at $24/$25) on its channel unless that channel is in mode 2 or 3: mode bits, data pointer, active, then the start state (SpcSfxChannelStep's code).
; HOW: SpcSfxChannel, SpcSfxMode, SpcSfxActivate.
; Entry: A = sound, $24/$25 = data. Exit: as SpcSfxChannelStep.
; @spc 219C
%spc($219C, $19DE26)
SpcSfxStart:
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
    call SpcSfxChannel                ; 21AF
    call SpcSfxMode                   ; 21B2
    mov.b a,$24                       ; 21B5
    mov.w $0488+x,a                   ; 21B7
    mov.b a,$25                       ; 21BA
    mov.w $0490+x,a                   ; 21BC
    call SpcSfxActivate               ; 21BF
    pop a                             ; 21C2
    jmp $1D53                         ; 21C3
.l21C6:
    pop a                             ; 21C6  ; not reached in the traces so far
    ret                               ; 21C7  ; not reached in the traces so far
    pop x                             ; 21C8
    mov.w a,$0420+x                   ; 21C9
    bne .l21D1                        ; 21CC
    call SpcSfxStop                   ; 21CE
.l21D1:
    ret                               ; 21D1
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
    pop x                             ; 21F3
    inc y                             ; 21F4
    mov a,($20)+y                     ; 21F5
    mov.w $0440+x,a                   ; 21F7
.l21FA:
    bra .l21F0                        ; 21FA
    pop x                             ; 21FC
    mov a,#$00                        ; 21FD
    mov.w $0438+x,a                   ; 21FF
.l2202:
    bra .l21FA                        ; 2202
    pop x                             ; 2204
    mov a,#$00                        ; 2205
    mov.w $0520+x,a                   ; 2207
.l220A:
    bra .l2202                        ; 220A
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
    pop x                             ; 221E
    inc y                             ; 221F
    mov a,($20)+y                     ; 2220
    mov.w $0448+x,a                   ; 2222
.l2225:
    bra .l220A                        ; 2225
    pop x                             ; 2227
    call SpcKeyOffVoice               ; 2228
    mov a,#$02                        ; 222B
    mov.w $0478+x,a                   ; 222D
.l2230:
    bra .l2225                        ; 2230
    pop x                             ; 2232
    inc y                             ; 2233
    mov a,($20)+y                     ; 2234
    mov.w $04F0+x,a                   ; 2236
.l2239:
    bra .l2230                        ; 2239
    pop x                             ; 223B
    inc y                             ; 223C
    mov a,($20)+y                     ; 223D
    and a,#$FE                        ; 223F
    push y                            ; 2241
    push x                            ; 2242
    call SpcNoiseSampleInit           ; 2243
    pop x                             ; 2246
    pop y                             ; 2247
    bra .l2239                        ; 2248

; @routine 19:DED4 SpcSfxPitchSlide
; WHAT IT DOES: Channel X's pitch slide ($0438 on): the delta $0428/$0430 added to the pitch $0498/$04A0, clamped to 0-$3FFF, and copied to the DSP pitch $03B8/$03C0.
; HOW: ADDW; sign and range tests.
; Entry: X = channel. Exit: A, Y changed.
; @spc 224A
%spc($224A, $19DED4)
SpcSfxPitchSlide:
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
    call SpcSfxVolumeSlide            ; 2295
    mov.w a,$0510+x                   ; 2298
    mov.b $20,a                       ; 229B
    mov.w a,$0518+x                   ; 229D
    mov.b $21,a                       ; 22A0
    mov a,x                           ; 22A2
    clrc                              ; 22A3
    adc a,#$08                        ; 22A4
    mov x,a                           ; 22A6
    call SpcSfxVolumeSlide            ; 22A7
    mov a,x                           ; 22AA
    setc                              ; 22AB
    sbc a,#$08                        ; 22AC
    mov x,a                           ; 22AE
.l22AF:
    ret                               ; 22AF

; @routine 19:DF3A SpcSfxVolumeSlide
; WHAT IT DOES: One side of channel X's volume slide: the delta $20/$21 added to the volume $03A8/$04A8, clamped, and the volume byte to $0398.
; HOW: ADDW; sign tests.
; Entry: X = channel (plus 8 for the right side). Exit: A = the volume.
; @spc 22B0
%spc($22B0, $19DF3A)
SpcSfxVolumeSlide:
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
    call SpcSfxFadeVolume             ; 22E3
    bra .l22EB                        ; 22E6
.l22E8:
    mov.w a,$0398+x                   ; 22E8
.l22EB:
    call SpcSfxVolumeAdjust           ; 22EB
    pop y                             ; 22EE
    call SpcDspWrite                  ; 22EF
    inc y                             ; 22F2
    push y                            ; 22F3
    mov.w a,$0408+x                   ; 22F4
    beq .l2309                        ; 22F7
    mov.w a,$04C8+x                   ; 22F9
    beq .l2309                        ; 22FC
    call SpcIndexRight                ; 22FE
    call SpcSfxFadeVolume             ; 2301
    call SpcIndexLeft                 ; 2304
    bra .l230C                        ; 2307
.l2309:
    mov.w a,$03A0+x                   ; 2309
.l230C:
    call SpcSfxVolumeAdjust           ; 230C
    pop y                             ; 230F
    call SpcDspWrite                  ; 2310
    inc y                             ; 2313
    mov.w a,$03B8+x                   ; 2314
    call SpcDspWrite                  ; 2317
    inc y                             ; 231A
    mov.w a,$03C0+x                   ; 231B
    call SpcDspWrite                  ; 231E
    inc y                             ; 2321
    mov.w a,$03C8+x                   ; 2322
    call SpcDspWrite                  ; 2325
    inc y                             ; 2328
    mov.w a,$03D0+x                   ; 2329
    call SpcDspWrite                  ; 232C
    inc y                             ; 232F
    mov.w a,$03D8+x                   ; 2330
    jmp SpcDspWrite                   ; 2333

; @routine 19:DFC0 SpcSfxFadeVolume
; WHAT IT DOES: The volume of channel X while it fades out (count A, $04C0): the start volume and per-step drop taken at count 1, the volume interpolated down over $3C steps; at the end the channel stops (or the driver reloads or leaves, by flags).
; HOW: SpcMul16, SpcDiv16 by $3C; SUBW.
; Entry: A = count, X = channel. Exit: A = volume.
; @spc 2336
%spc($2336, $19DFC0)
SpcSfxFadeVolume:
    cmp a,#$01                        ; 2336
    beq .l2348                        ; 2338
    cmp a,#$3C                        ; 233A
    beq .l2385                        ; 233C
    cmp a,#$FF                        ; 233E
    beq .l23AD                        ; 2340
    inc a                             ; 2342
    mov.w $04C0+x,a                   ; 2343
    bra .l2361                        ; 2346
.l2348:
    inc a                             ; 2348
    mov.w $04C0+x,a                   ; 2349
    mov.w a,$0398+x                   ; 234C
    mov y,a                           ; 234F
    mov.w a,$03A8+x                   ; 2350
    push x                            ; 2353
    mov x,#$3C                        ; 2354
    call SpcDiv16                     ; 2356
    pop x                             ; 2359
    mov.w $04D0+x,a                   ; 235A
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
    call SpcMul16                     ; 2372
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
    bbs !spc_track_flags.7,.l238D     ; 2385
    bbs $2D.0,.l2390                  ; 2388
    bra .l2393                        ; 238B
.l238D:
    jmp $1AE9                         ; 238D
.l2390:
    jmp $1B15                         ; 2390  ; not reached in the traces so far
.l2393:
    mov.b $22,x                       ; 2393
    bbs $22.3,.l23A2                  ; 2395
    call SpcSfxStop                   ; 2398
.l239B:
    mov a,#$FF                        ; 239B
    mov.w $04C0+x,a                   ; 239D
    inc a                             ; 23A0
    ret                               ; 23A1
.l23A2:
    call SpcIndexLeft                 ; 23A2
    call SpcSfxStop                   ; 23A5
    call SpcIndexRight                ; 23A8
    bra .l239B                        ; 23AB
.l23AD:
    inc a                             ; 23AD  ; not reached in the traces so far
    mov.w $04C0+x,a                   ; 23AE  ; not reached in the traces so far
    ret                               ; 23B1  ; not reached in the traces so far

; @routine 19:E03C SpcSfxVolumeAdjust
; WHAT IT DOES: For sound $25, adds $0530 * 4 to the volume A.
; HOW: MUL, ADC.
; Entry: A = volume, X = channel. Exit: A = volume; Y kept.
; @spc 23B2
%spc($23B2, $19E03C)
SpcSfxVolumeAdjust:
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

; @routine 19:E059 SpcSfxSpecial
; WHAT IT DOES: When channel X plays a sound in the list at $23EB, runs its handler from the table at $23FB: the engine sounds' pitch and volume from the car's speed on port 3 (SpcEnginePitch and its variants), and rev timing.
; HOW: JMP ($23FB+X) with X pushed.
; Entry: X = channel. Exit: as the handler leaves it.
; @spc 23CF
%spc($23CF, $19E059)
SpcSfxSpecial:
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
    call SpcEngineVolume              ; 2458
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

; @routine 19:E22A SpcEngineVolume
; WHAT IT DOES: The engine sound's volume from port 3: (inverted speed) / $22 + $23, scaled by $0552 in echo mode A, plus 3 with bit 2 of $29, by $D0 unless bit 2 of $28 (SpcEngineDamp), by $056B, by $C0 with bit 4 of $2D; both sides' volume set.
; HOW: DIV and MULs keeping high bytes.
; Entry: X = channel, $22/$23 = curve. Exit: A = volume.
; @spc 25A0
%spc($25A0, $19E22A)
SpcEngineVolume:
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
    bbc $28.0,.l25BB                  ; 25B2
    mov.w y,$0552                     ; 25B5
    dec y                             ; 25B8
    mul ya                            ; 25B9
    mov a,y                           ; 25BA
.l25BB:
    bbc !spc_flags.2,.l25C0           ; 25BB
    adc a,#$03                        ; 25BE
.l25C0:
    call SpcEngineDamp                ; 25C0
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

; @routine 19:E262 SpcEngineDamp
; WHAT IT DOES: Unless bit 2 of $28 is set, A = A * $D0 / 256.
; HOW: MUL.
; Entry: A. Exit: A; Y kept.
; @spc 25D8
%spc($25D8, $19E262)
SpcEngineDamp:
    bbs $28.2,.l25E1                  ; 25D8
    push y                            ; 25DB
    mov y,#$D0                        ; 25DC
    mul ya                            ; 25DE
    mov a,y                           ; 25DF
    pop y                             ; 25E0
.l25E1:
    ret                               ; 25E1
    pop x                             ; 25E2
    mov $22,#$28                      ; 25E3
    mov $23,#$03                      ; 25E6
    mov $24,#$02                      ; 25E9
    call SpcEngineRevTimer            ; 25EC
    mov.w a,$03E8+x                   ; 25EF
    cmp a,#$02                        ; 25F2
    beq .l25FB                        ; 25F4
.l25F6:
    bbs $28.6,SpcEngineToggle_l2614   ; 25F6
    bra SpcEngineToggle_l2609         ; 25F9
.l25FB:
    call SpcEngineToggle              ; 25FB
    bra .l25F6                        ; 25FE

; @routine 19:E28A SpcEngineToggle
; WHAT IT DOES: Toggles bit 6 of $28.
; HOW: BBS, SET1/CLR1.
; Entry: none. Exit: none.
; @spc 2600
%spc($2600, $19E28A)
SpcEngineToggle:
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
    mov $22,#$1E                      ; 261F
    mov $23,#$03                      ; 2622
    mov $24,#$01                      ; 2625
    bra .l2633                        ; 2628
    mov $22,#$14                      ; 262A
    mov $23,#$0A                      ; 262D
    mov $24,#$02                      ; 2630
.l2633:
    pop x                             ; 2633

; @routine 19:E2BE SpcEngineRevTimer
; WHAT IT DOES: The rev pulse: the counter $0545 counts up to (inverted speed) / $22 + $23 and then restarts with the channel's mode $03E8 = $24.
; HOW: DIV, compare.
; Entry: X = channel, $22-$24 = curve. Exit: A changed.
; @spc 2634
%spc($2634, $19E2BE)
SpcEngineRevTimer:
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
    call SpcSfxPanScale               ; 2697
    mov.w $0398+x,a                   ; 269A
    inc y                             ; 269D
    mov.w a,$EE30+y                   ; 269E
    call SpcSfxPanScale               ; 26A1
    mov.w $03A0+x,a                   ; 26A4
    ret                               ; 26A7
.l26A8:
    eor a,#$0F                        ; 26A8
    asl a                             ; 26AA
    mov y,a                           ; 26AB
    mov.w a,$EE30+y                   ; 26AC
    call SpcSfxPanScale               ; 26AF
    mov.w $03A0+x,a                   ; 26B2
    inc y                             ; 26B5
    mov.w a,$EE30+y                   ; 26B6
    call SpcSfxPanScale               ; 26B9
    mov.w $0398+x,a                   ; 26BC
    ret                               ; 26BF

; @routine 19:E34A SpcSfxPanScale
; WHAT IT DOES: A pan table value A scaled by $0562 + 1 and divided by $15, less the low four bits of port 2.
; HOW: MUL, DIV.
; Entry: A = table value. Exit: A; X, Y kept.
; @spc 26C0
%spc($26C0, $19E34A)
SpcSfxPanScale:
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

; @routine 19:F1C3 SpcMul16
; WHAT IT DOES: A times the 16-bit value X:Y (X the high byte): YA = the result's middle 16 bits plus A * Y, as the sound code's fixed-point multiply.
; HOW: Two MULs and ADDW through $26/$27.
; Entry: A, X, Y. Exit: YA, X.
; @spc CA75
%spc($CA75, $19F1C3)
SpcMul16:
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

; @routine 19:F1D7 SpcDiv16
; WHAT IT DOES: The 16-bit value Y:A divided by X: YA = the quotient.
; HOW: Two DIVs, the high quotient through $27.
; Entry: Y:A, X. Exit: YA = quotient.
; @spc CA89
%spc($CA89, $19F1D7)
SpcDiv16:
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

; @routine 19:F1E4 SpcIndexRight
; WHAT IT DOES: X + 8 (a channel's right-side index).
; HOW: Eight INC X.
; Entry: X. Exit: X + 8.
; @spc CA96
%spc($CA96, $19F1E4)
SpcIndexRight:
    inc x                             ; CA96
    inc x                             ; CA97
    inc x                             ; CA98
    inc x                             ; CA99
    inc x                             ; CA9A
    inc x                             ; CA9B
    inc x                             ; CA9C
    inc x                             ; CA9D
    ret                               ; CA9E

; @routine 19:F1ED SpcIndexLeft
; WHAT IT DOES: X - 8.
; HOW: Eight DEC X.
; Entry: X. Exit: X - 8.
; @spc CA9F
%spc($CA9F, $19F1ED)
SpcIndexLeft:
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

; @routine 19:F20E SpcNoiseSample
; WHAT IT DOES: While a channel plays sound $23, rewrites part of the sample at $FF00 each tick with noise from the LFSR $18/$19: seven blocks of eight bytes, the block chosen by the counter $0563.
; HOW: MOV1/EOR1 of two LFSR bits, ROL; bytes ORed with $11.
; Entry: none. Exit: X, Y changed.
; @spc CAC0
%spc($CAC0, $19F20E)
SpcNoiseSample:
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
    rol.b !spc_random                 ; CAE5
    rol.b !spc_random+1               ; CAE7
    mov.b a,!spc_random+1             ; CAE9
    and a,#$D0                        ; CAEB
    or a,#$11                         ; CAED
    mov.w $FF00+y,a                   ; CAEF
    inc y                             ; CAF2
    mov.b a,!spc_random               ; CAF3
    and a,#$FF                        ; CAF5
    or a,#$11                         ; CAF7
    mov.w $FF00+y,a                   ; CAF9
    inc y                             ; CAFC
    dec x                             ; CAFD
    bne .lCADF                        ; CAFE
    dbnz $20,.lCADC                   ; CB00
    ret                               ; CB03

%spc($EDF0, $19F40D)

; @routine 19:F40D SpcNoiseSampleInit
; WHAT IT DOES: Builds the sample at $FF00: every ninth byte (the BRR block headers) set to A for $1B blocks, the last A + 1 (the end flag); the sample directory's entry at $3F8C-$3F8F points start and loop at $FF00; the LFSR made nonzero.
; HOW: Y stepping by 9.
; Entry: A = header byte. Exit: A, Y changed.
; @spc EDF0
%spc($EDF0, $19F40D)
SpcNoiseSampleInit:
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
    mov.b a,!spc_random               ; EE17
    or.b a,!spc_random+1              ; EE19
    bne .lEE1F                        ; EE1B
    inc.b !spc_random                 ; EE1D  ; not reached in the traces so far
.lEE1F:
    ret                               ; EE1F
    db $EE, $02, $D6, $06, $CA, $08, $C4, $09
    db $28, $0A, $5A, $0A, $8C, $0A, $BE, $0A
    db $78, $14, $8C, $28, $A0, $3C, $B4, $50
    db $C8, $64, $DC, $78, $F0, $8C, $DC, $A0
%spc_end()
