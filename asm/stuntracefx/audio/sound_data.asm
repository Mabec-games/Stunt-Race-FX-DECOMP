; sound_data.asm: getting a block of sound data into the SPC700. The block is
; first put in Super FX RAM at $70:6000, either copied from ROM bank $1E or
; unpacked there by Super FX program $01:CF42, then uploaded from there.
; C: src/stuntracefx/audio/sound_data.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 03:F05E SoundDataLoad
; WHAT IT DOES: Loads sound block A into the SPC700: puts it at $70:6000
; (put_block: copied from bank $1E for $8000, $91BC or $AFC6, otherwise
; unpacked by the Super FX with Y as its second input), uploads it with
; ApuUpload, then waits a moment ($4000 turns of a countdown).
; HOW: Word copies or GsuRun; ApuUpload from $70:6000; a DEC loop.
; Entry: M 16-bit, X 16-bit, D = $0000; A = block, Y = Super FX input.
; Exit: A = 0; X = 0; Y as ApuUpload leaves it; M 16-bit; Z, C set, N, V
; clear. (The copies for $91BC and $AFC6 are not reached by any script
; yet.)
%at($03F05E)
SoundDataLoad:
    cmp.w #$8000
    bne .not8000
    ldx.w #$0000
.copy8000:
    lda.l !sound_block_8000,x
    sta.l !sound_block,x
    inx
    inx
    cpx.w #$11F8
    bcc .copy8000
    bra .upload
.not8000:
    cmp.w #$91BC
    bne .not91bc
    ldx.w #$0000                    ; not reached by any script yet
.copy91bc:
    lda.l !sound_block_91bc,x
    sta.l !sound_block,x
    inx
    inx
    cpx.w #$11F8
    bcc .copy91bc
    bra .upload
.not91bc:
    cmp.w #$AFC6
    bne .unpack
    ldx.w #$0000                    ; not reached by any script yet
.copyafc6:
    lda.l !sound_block_afc6,x
    sta.l !sound_block,x
    inx
    inx
    cpx.w #$0DAC
    bcc .copyafc6
    bra .upload
.unpack:
    sta.l !gsu_in_68
    lda.w #$6000
    sta.l !gsu_dest
    lda.w #$0000
    sta.l !gsu_in_a2
    tya
    sta.l !gsu_in_6a
    sep.b #$20
    lda.b #$01
    ldx.w #$CF42
    jsl.l GsuRun
    rep.b #$30
.upload:
    ldy.w #$6000
    lda.w #$0070
    jsl.l ApuUpload
    rep.b #$20
    lda.w #$4000
.wait:
    dec a
    bne .wait
    rtl

; @routine 04:D251 MusicLoadForCourse
; WHAT IT DOES: Loads music A's sound data into the SPC700: the first
; block list from the table at $04:D41E (three bytes per music: an address,
; $FFFF for none, and a bank); then, in HUD mode 2, $1000 bytes of the
; block for the course in $0D8A, copied from bank $19 to Super FX RAM at
; $70:3000 and patched (+$10 to its word at 2, +$D30 to its words at 4, 6
; and 22) before uploading it from there; otherwise the second list from
; the table at $04:D42D. Each upload is preceded by ApuPort1Send05 and
; followed by ApuSettleDelay.
; HOW: The index is the music times 3 through direct page $F1; the copy is
; MVN (byte by byte, each read then written). X is saved on the stack
; around the first upload.
; Entry: M 16-bit, X 16-bit, D = $0000; A low nibble = music. Exit: A = 0;
; M 16-bit; Z set, N clear; X, Y, C and V as the last call or compare
; leaves them.
%at($04D251)
MusicLoadForCourse:
    and.w #$000F
    sta.b !music_index
    asl a
    clc
    adc.b !music_index
    tax
    phx
    jsl.l ApuPort1Send05
    rep.b #$30
    lda.l !music_lists,x
    cmp.w #$FFFF
    beq .first_done
    tay
    jsl.l ApuPort1Send05
    lda.l !music_lists+2,x
    and.w #$00FF
    jsl.l ApuUpload
.first_done:
    rep.b #$20
    rep.b #$10
    jsl.l ApuSettleDelay
    rep.b #$30
    plx
    lda.w !hud_mode
    and.w #$00FF
    cmp.w #$0002
    beq .course_block
    rep.b #$30
    lda.l !music_lists+15,x
    cmp.w #$FFFF
    beq .second_done
    tay
    jsl.l ApuPort1Send05
    lda.l !music_lists+17,x
    and.w #$00FF
    jsl.l ApuUpload
.second_done:
    rep.b #$20
    rep.b #$10
    jsl.l ApuSettleDelay
    rts
.course_block:
    jsl.l ApuPort1Send05
    lda.w !wram_0D8A
    and.w #$000F
    sta.b !music_index
    asl a
    clc
    adc.b !music_index
    tax
    lda.l !music_lists,x
    tax
    ldy.w #$3000
    sep.b #$20
    phb
    rep.b #$20
    lda.w #$0FFF
    mvn !gsu_picture>>16,!music_course_blocks>>16
    sep.b #$20
    plb
    rep.b #$20
    lda.l !gsu_picture+2
    clc
    adc.w #$0010
    sta.l !gsu_picture+2
    lda.l !gsu_picture+4
    clc
    adc.w #$0D30
    sta.l !gsu_picture+4
    lda.l !gsu_picture+6
    clc
    adc.w #$0D30
    sta.l !gsu_picture+6
    lda.l !gsu_picture+22
    clc
    adc.w #$0D30
    sta.l !gsu_picture+22
    ldy.w #$3000
    lda.w #$0070
    jsl.l ApuUpload
    jsl.l ApuSettleDelay
    rts

; @routine 04:D0DB CourseSoundLoad
; WHAT IT DOES: Loads the course's sound unless $10DA is set. With $10DC
; set it is the other set, loaded once ($10DE counted down from 0 first):
; block $19:8D4C, the $1518 bytes from $1E:9B2C through $70:6000, block
; $1B:B8A0, then SoundReset and 4 on APU port 0. Otherwise, by the course
; $0E3D (entry course * 3 of the tables in bank 04): the samples
; (SoundDataLoad, block and input from $04:D322), the effects block from
; $04:D376 and, after the music for the car $0D89 (MusicLoadForCourse),
; the block from $04:D3CA (each skipped at $FFFF); in HUD mode 1 with
; $0D4E clear also block $F9B8 (input $1D). Last it waits until all four
; APU ports read 0.
; HOW: P pushed; each upload after 5 on APU port 1 or ApuPort1Send05,
; then a settling delay; X (the table offset) kept on the stack. The
; $10DC set is not reached in the traces so far.
; Entry: any widths; the data bank where work RAM and the APU ports are
; mapped, D = $0000. Exit: P restored; A low byte 0 after the wait.
%at($04D0DB)
CourseSoundLoad:
    php
    sep.b #$20
    lda.w !wram_10DA
    beq .lD0E5
    plp
    rtl
.lD0E5:
    lda.w !wram_10DC
    bne .lD0ED
    brl .lD18A
.lD0ED:
    lda.w !wram_10DC+2
    beq .lD0F5
    brl .lD188
.lD0F5:
    dec.w !wram_10DC+2
    rep.b #$30
    jsl.l ApuPort1Send05
    php
    rep.b #$30
    lda.w #$0005
    sta.l !APUIO1
    lda.w #$4000
.lD10B:
    dec a
    bne .lD10B
    lda.w #$0019
    ldy.w #$8D4C
    jsl.l ApuUpload
    rep.b #$20
    lda.w #$4000
.lD11D:
    dec a
    bne .lD11D
    plp
    lda.w #$4000
.lD124:
    dec a
    bne .lD124
    lda.w #$0005
    sta.l !APUIO1
    rep.b #$20
    lda.w #$4000
.lD133:
    dec a
    bne .lD133
    ldx.w #$0000
.lD139:
    lda.l !course_sound_alt,x
    sta.l !sound_block,x
    inx
    inx
    cpx.w #$1518
    bcc .lD139
    ldy.w #$6000
    lda.w #$0070
    jsl.l ApuUpload
    rep.b #$20
    lda.w #$4000
.lD157:
    dec a
    bne .lD157
    php
    rep.b #$30
    lda.w #$0005
    sta.l !APUIO1
    lda.w #$4000
.lD167:
    dec a
    bne .lD167
    lda.w #$001B
    ldy.w #$B8A0
    jsl.l ApuUpload
    rep.b #$20
    lda.w #$4000
.lD179:
    dec a
    bne .lD179
    plp
    jsl.l SoundReset
    sep.b #$20
    lda.b #$04
    sta.w !APUIO0
.lD188:
    plp
    rtl
.lD18A:
    rep.b #$30
    lda.w !course
    asl a
    clc
    adc.w !course
    and.w #$00FF
    tax
    phx
    rep.b #$30
    jsl.l ApuPort1Send05
    lda.l !course_sound_samples+2,x
    and.w #$00FF
    tay
    lda.l !course_sound_samples,x
    jsl.l SoundDataLoad
    rep.b #$20
    rep.b #$10
    jsl.l ApuSettleDelay
    rep.b #$30
    plx
    phx
    rep.b #$30
    lda.l !course_sound_effects,x
    cmp.w #$FFFF
    beq .lD1D6
    tay
    jsl.l ApuPort1Send05
    lda.l !course_sound_effects+2,x
    and.w #$00FF
    jsl.l ApuUpload
.lD1D6:
    rep.b #$20
    rep.b #$10
    jsl.l ApuSettleDelay
    rep.b #$20
    lda.w !wram_0D89
    jsr.w MusicLoadForCourse
    jsl.l ApuSettleDelay
    rep.b #$30
    plx
    rep.b #$30
    lda.l !course_sound_extra,x
    cmp.w #$FFFF
    beq .lD208
    tay
    jsl.l ApuPort1Send05
    lda.l !course_sound_extra+2,x
    and.w #$00FF
    jsl.l ApuUpload
.lD208:
    rep.b #$20
    rep.b #$10
    jsl.l ApuSettleDelay
    sep.b #$20
    lda.w !hud_mode
    cmp.b #$01
    bne .lD239
    lda.w !hud_blank
    bne .lD239
    php
    rep.b #$30
    lda.w #$0005
    sta.l !APUIO1
    lda.w #$4000
.lD22B:
    dec a
    bne .lD22B
    lda.w #$F9B8
    ldy.w #$001D
    jsl.l SoundDataLoad
    plp
.lD239:
    sep.b #$20
.lD23B:
    lda.w !APUIO0
    bne .lD23B
    lda.w !APUIO1
    bne .lD23B
    lda.w !APUIO2
    bne .lD23B
    lda.w !APUIO3
    bne .lD23B
    plp
    rtl
