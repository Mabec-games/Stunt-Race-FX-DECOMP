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
