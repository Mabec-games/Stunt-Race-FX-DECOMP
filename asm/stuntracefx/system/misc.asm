; misc.asm: small routines of the frame code that have no other home yet.
; Run from work RAM. C: src/stuntracefx/system/misc.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 7E:E18B DataBankTest
; WHAT IT DOES: Leaves every register as it was except N and Z, which it sets
; from the data bank.
; HOW: Pushes DB and PB, pulls PB into DB and then the old DB back.
; Entry: any widths. Exit: DB unchanged; N, Z from DB.
%ramcode($7EE18B, $013EB2)
DataBankTest:
    phb
    phk
    plb
    plb
    rtl

; @routine 7E:F217 KeepRegisters
; WHAT IT DOES: Nothing: it saves P, changes the widths and restores P.
; HOW: PHP, SEP #$20, REP #$30, PLP.
; Entry: any widths. Exit: every register unchanged.
%ramcode($7EF217, $014F3E)
KeepRegisters:
    php
    sep.b #$20
    rep.b #$30
    plp
    rtl

; @routine 03:F329 ReturnOnly
; WHAT IT DOES: Nothing: a lone RTS that callers use as an empty handler.
; HOW: RTS.
; Entry: any widths. Exit: every register unchanged.
%at($03F329)
ReturnOnly:
    rts

; @routine 03:AEEE ReadTable9022
; WHAT IT DOES: Reads 128 words' low bytes from the table at $9022 in the
; data bank and keeps none of them; it changes only registers and $64.
; HOW: A loop of 128 loads stepping Y by 2, X by 1, counting down $64.
; Entry: any widths, D = $0000. Exit: A low byte = the last byte read; X =
; $80, Y = 0; $64 = 0; P unchanged.
%at($03AEEE)
ReadTable9022:
    php
    sep.b #$30
    ldy.b #$00
    ldx.b #$00
    lda.b #$80
    sta.b !wram_0064
.lAEF9:
    lda.w !table_9022,y
    inx
    iny
    iny
    dec.b !wram_0064
    bne .lAEF9
    plp
    rtl

; @routine 02:FC44 Set0D37FromTable
; WHAT IT DOES: Sets $0D37 to the word table $02:FC58 entry for the byte at
; $0D48.
; HOW: 16-bit lookup; work RAM through the data bank.
; Entry: any widths. Exit: A = the entry; X = the byte * 2 (low byte only
; when X is 8-bit); P unchanged.
%at($02FC44)
Set0D37FromTable:
    php
    rep.b #$30
    lda.w !wram_0D48
    and.w #$00FF
    asl a
    tax
    lda.l !table_02fc58,x
    sta.w !wram_0D37
    plp
    rts

; @routine 02:D65A Set7EBC2BTrio
; WHAT IT DOES: Stores A's low byte at $7E:BC2B, $7E:BC2D and $7E:BC2F.
; HOW: Three long byte stores.
; Entry: any widths. Exit: every register unchanged.
%at($02D65A)
Set7EBC2BTrio:
    php
    sep.b #$20
    sta.l !wram_7EBC2B
    sta.l !wram_7EBC2D
    sta.l !wram_7EBC2F
    plp
    rtl

; @routine 07:EE11 Countdown0DF8
; WHAT IT DOES: Counts $0DF8 down by one; once it is zero, instead counts
; $0E5C down and clears $1160.
; HOW: 8-bit memory decrements through the data bank.
; Entry: any widths. Exit: A low byte = $0DF8's value before; P unchanged.
%at($07EE11)
Countdown0DF8:
    php
    sep.b #$20
    lda.w !wram_0DF8
    bne .lEE21
    dec.w !wram_0E5C
    stz.w !wram_1160
    plp
    rts
.lEE21:
    dec.w !wram_0DF8
    plp
    rts

; @routine 08:B893 Set7EFFA6Sequence
; WHAT IT DOES: Stores the words 1, 2, 3, 4 at $7E:FFA6-$7E:FFAD.
; HOW: Four 16-bit long stores.
; Entry: any widths. Exit: A = 4 (16-bit); P unchanged.
%at($08B893)
Set7EFFA6Sequence:
    php
    rep.b #$20
    lda.w #$0001
    sta.l !wram_7EFFA6
    lda.w #$0002
    sta.l !wram_7EFFA6+2
    lda.w #$0003
    sta.l !wram_7EFFA6+4
    lda.w #$0004
    sta.l !wram_7EFFA6+6
    plp
    rtl

; @routine 02:D53D WordCopyStep
; WHAT IT DOES: While the count at $0CF6 is non-zero, copies one word from
; $02:D65A + $0CF2 to $07A2 + $0CF6 and steps both offsets down by 2. No
; script has reached the copy yet: $0CF6 has always been 0.
; HOW: 16-bit offsets and stores through the data bank; the source is a
; long read, and each offset is stepped with two DECs.
; Entry: any widths. Exit: when $0CF6 is 0, Y = 0 and A unchanged; else A
; = the word copied, X and Y the offsets before the step; M 16-bit, X
; 16-bit; N, Z from the last step (Z set when there was nothing to copy).
%at($02D53D)
WordCopyStep:
    rep.b #$30
    ldy.w !copy_count
    bne .copy
    rtl
.copy:                              ; not reached in the traces so far
    ldx.w !copy_src_pos
    ldy.w !copy_count
    lda.l !copy_src,x
    sta.w !copy_dest,y
    dec.w !copy_src_pos
    dec.w !copy_src_pos
    dec.w !copy_count
    dec.w !copy_count
    rtl

; @routine 03:8648 DemoRunSecond
; WHAT IT DOES: Runs the second of the attract demos: $1A57 = $6B (its
; offset in the demo tables), then the shared demo run at $03:8683.
; HOW: A tail JMP into the demo run, reached as original code
; (jump_routine).
; Entry: D = $0000. Exit: as the demo run leaves it.
%at($038648)
DemoRunSecond:
    rep.b #$30
    lda.w #$006B
    sta.w !seq_stage+34
    jmp.w DemoRun

; @routine 03:8653 DemoRunFourth
; WHAT IT DOES: Runs the third demo of the title screen's cycle: $1A57 = $FC
; (its offset in the demo tables), then the shared demo run at $03:8683.
; HOW: A tail JMP into the demo run, reached as original code
; (jump_routine).
; Entry: D = $0000. Exit: as the demo run leaves it.
%at($038653)
DemoRunFourth:
    rep.b #$30
    lda.w #$00FC
    sta.w !seq_stage+34
    jmp.w DemoRun

; @routine 03:8683 DemoRun
; WHAT IT DOES: Runs one attract demo from its script at $03:8004 +
; $1A57: the course ($0E3D, script byte 2) and the selection handler
; ($00:18DB, byte 0) from the script's head, $1A5B = $20, HUD mode 0, the
; Super FX set (clock, config $20, screen base $0B), the race set up
; (Set7EFFA6Sequence, RaceVideoInit, 02:CF3E, WindowsInit, the palette
; flag, 02:CF45 as AttractCourseSetup, HUD mode 3, CourseDataPointer,
; CourseScriptRun, ObjectPlaceAll, Super FX program $04:8800,
; ObjectsRunHandlers), the player object's flag 8 set and $70:2732
; cleared; then the script's commands, one race frame (RaceFrameRun,
; $1A59 counting them) wherever a command waits:
;   1 word: ObjectLinkedSet1A with it on object $0346; 2 word: to
;   $70:2732 (and its high byte again to $70:2733); 3 word: wait until
;   frame $1A59 is the word; 5: fade out; 6: wait for the fade's end; 7:
;   $19DF = 0, $19D8 = 1; 8: wait until $19D8 is 5; 9 word word: the
;   camera object's +$08 and +$0C set, then moved (CameraFollowStep,
;   ObjectFaceTied); $0A word: added to its +$0A, then moved; $0B: wait
;   for a button held, then fade out to the end; $0C word, $0D word: ORA,
;   AND into $1A5B; any other byte ends the script.
; Each frame, with $1A5B bit 0 set the demo runs on to the fade's end;
; with bit 1 clear a button held ($030A) ends it (bits $41, fade out).
; At the end the screen is blanked, NMI, DMA and HDMA off.
; HOW: Compare chain on the command byte, every script read long from
; bank $03; PlayerStatsAndPaletteAnimate before each command; data bank
; $03 from RaceVideoInit on (PHK, PLB), $7E around the camera's calls.
; Entry: M 16-bit, X 16-bit; $1A57 = the demo's script offset. Exit: C set
; when a button ended the demo; A = $1A5B & $40, $1A5B cleared; M 16-bit,
; X 16-bit.
%at($038683)
DemoRun:
    rep.b #$30
    ldx.w !seq_stage+34
    lda.l DemoScripts+2,x
    sta.w !course
    lda.w #$0020
    sta.w !seq_stage+38
    ldx.w !seq_stage+34
    lda.l DemoScripts,x
    sta.l !selection_handler_a
    lda.w #$0000
    sta.w !hud_mode
    sep.b #$20
    lda.b #$01
    sta.w !gsu_scmr_shadow
    lda.b #$01
    sta.w !GSU_CLSR
    lda.b #$20
    sta.w !GSU_CFGR
    lda.b #$0B
    sta.w !GSU_SCBR
    jsl.l Set7EFFA6Sequence
    sep.b #$20
    stz.w !HDMAEN
    lda.b #$80
    sta.w !INIDISP
    sei
    jsl.l RaceVideoInit
    phk
    plb
    sep.b #$20
    stz.w !HDMAEN
    lda.b #$80
    sta.w !INIDISP
    sei
    sep.b #$30
    jsl.l AttractRaceSetup
    jsl.l WindowsInit
    sep.b #$20
    rep.b #$10
    lda.b #$01
    sta.w !GSU_CLSR
    lda.b #$01
    sta.l !palette_saved
    sei
    jsl.l AttractCourseSetupRom
    sep.b #$20
    lda.b #$03
    sta.w !hud_mode
    jsl.l CourseDataPointer
    jsl.l CourseScriptRun
    jsl.l ObjectPlaceAll
    sep.b #$20
    rep.b #$10
    lda.b #$04
    ldx.w #$8800
    jsl.l GsuRun
    sep.b #$20
    lda.b #$01
    sta.w !wram_0357
    stz.w !wram_05E9
    rep.b #$20
    stz.w !seq_stage+36
    jsl.l ObjectsRunHandlers
    sep.b #$20
    lda.l !TIMEUP
    rep.b #$20
    cli
    rep.b #$30
    lda.l !gsu_match_2476
    tax
    lda.l !gsu_rec_64,x
    ora.w #$0008
    sta.l !gsu_rec_64,x
    lda.w #$0000
    sta.l !fx_init_2732
    lda.w !seq_stage+34
    clc
    adc.w #$0003
    sta.w !seq_stage+34
.l8759:
    jsl.l PlayerStatsAndPaletteAnimate
    rep.b #$30
    ldx.w !seq_stage+34
.l8762:
    stx.w !seq_stage+34
    lda.l DemoScripts,x
    and.w #$00FF
    cmp.w #$0001
    bne .l8774
    brl .l87FD
.l8774:
    cmp.w #$0003
    bne .l877C
    brl .l87E8
.l877C:
    cmp.w #$0002
    bne .l8784
    brl .l87CF
.l8784:
    cmp.w #$0005
    bne .l878C
    brl .l8815
.l878C:
    cmp.w #$0006
    bne .l8794
    brl .l8827
.l8794:
    cmp.w #$0007
    bne .l879C
    brl .l8845
.l879C:
    cmp.w #$0008
    bne .l87A4
    brl .l885C
.l87A4:
    cmp.w #$0009
    bne .l87AC
    brl .l8873
.l87AC:
    cmp.w #$000A
    bne .l87B4
    brl .l88AE
.l87B4:
    cmp.w #$000B
    bne .l87BC
    brl .l8908
.l87BC:
    cmp.w #$000C
    bne .l87C4
    brl .l88E2
.l87C4:
    cmp.w #$000D
    bne .l87CC
    brl .l88F5
.l87CC:
    jmp.w .l8973
.l87CF:
    lda.l DemoScripts+1,x
    sta.l !fx_init_2732
    lda.l DemoScripts+2,x
    sta.l !fx_init_2732+1
    txa
    clc
    adc.w #$0004
    tax
    jmp.w .l8762
.l87E8:
    lda.l DemoScripts+1,x
    cmp.w !seq_stage+36
    beq .l87F4
    brl .l892E
.l87F4:
    txa
    clc
    adc.w #$0003
    tax
    jmp.w .l8762
.l87FD:
    phx
    lda.l DemoScripts+1,x
    ldx.w !wram_0346
    jsl.l ObjectLinkedSet1A
    plx
    rep.b #$30
    txa
    clc
    adc.w #$0003
    tax
    jmp.w .l8762
.l8815:
    txa
    clc
    adc.w #$0001
    tax
    sep.b #$20
    lda.b #$01
    sta.w !fade_out
    rep.b #$20
    jmp.w .l8762
.l8827:
    lda.w !fade_level_hi
    bit.w #$0080
    beq .l8834
    ora.w #$FF00
    bra .l8837
.l8834:
    and.w #$00FF
.l8837:
    bmi .l883C
    brl .l892E
.l883C:
    txa
    clc
    adc.w #$0001
    tax
    jmp.w .l8762
.l8845:
    txa
    clc
    adc.w #$0001
    tax
    sep.b #$20
    lda.b #$00
    sta.w !wram_19DF
    lda.b #$01
    sta.w !window_hdma_off
    rep.b #$20
    jmp.w .l8762
.l885C:
    lda.w !window_hdma_off
    and.w #$00FF
    cmp.w #$0005
    beq .l886A
    brl .l892E
.l886A:
    txa
    clc
    adc.w #$0001
    tax
    jmp.w .l8762
.l8873:
    lda.l DemoScripts+1,x
    sta.b !dp_02
    lda.l DemoScripts+3,x
    sta.b !dp_94
    phx
    ldx.w #$1A64
    lda.b !dp_02
    sta.l !bank0_base+8,x
    lda.b !dp_94
    sta.l !bank0_base+12,x
    sep.b #$20
    phb
    lda.b #$7E
    pha
    plb
    ldx.w #$1A64
    jsl.l CameraFollowStep
    jsl.l ObjectFaceTied
    plb
    rep.b #$30
    plx
    txa
    clc
    adc.w #$0005
    tax
    jmp.w .l8762
.l88AE:
    lda.l DemoScripts+1,x
    sta.b !dp_08
    phx
    ldx.w #$1A64
    lda.l !bank0_base+10,x
    clc
    adc.b !dp_08
    sta.l !bank0_base+10,x
    sep.b #$20
    phb
    lda.b #$7E
    pha
    plb
    ldx.w #$1A64
    jsl.l CameraFollowStep
    jsl.l ObjectFaceTied
    plb
    rep.b #$30
    plx
    txa
    clc
    adc.w #$0003
    tax
    jmp.w .l8762
.l88E2:
    lda.w !seq_stage+38
    ora.l DemoScripts+1,x
    sta.w !seq_stage+38
    txa
    clc
    adc.w #$0003
    tax
    jmp.w .l8762
.l88F5:
    lda.w !seq_stage+38
    and.l DemoScripts+1,x
    sta.w !seq_stage+38
    txa
    clc
    adc.w #$0003
    tax
    jmp.w .l8762
.l8908:
    lda.w !joy1_held+1
    bne .l8910
    brl .l8759
.l8910:
    sep.b #$20
    lda.b #$01
    sta.w !fade_out
    rep.b #$20
.l8919:
    lda.w !fade_level_hi
    bit.w #$0080
    beq .l8926
    ora.w #$FF00
    bra .l8929
.l8926:
    and.w #$00FF
.l8929:
    bpl .l8919
    jmp.w .l8973
.l892E:
    jsl.l RaceFrameRun
    rep.b #$30
    inc.w !seq_stage+36
    lda.w !seq_stage+38
    bit.w #$0001
    bne .l8961
    bit.w #$0002
    beq .l8947
    brl .l8759
.l8947:
    lda.w !joy1_held+1
    bne .l894F
    brl .l8759
.l894F:
    lda.w !seq_stage+38
    ora.w #$0041
    sta.w !seq_stage+38
    sep.b #$20
    lda.b #$01
    sta.w !fade_out
    rep.b #$20
.l8961:
    lda.w !fade_level_hi
    bit.w #$0080
    beq .l896E
    ora.w #$FF00
    bra .l8971
.l896E:
    and.w #$00FF
.l8971:
    bpl .l892E
.l8973:
    sep.b #$20
    lda.b #$80
    sta.w !INIDISP
    stz.w !NMITIMEN
    stz.w !MDMAEN
    stz.w !HDMAEN
    rep.b #$20
    lda.w !seq_stage+38
    and.w #$0040
    bne .l8992
    stz.w !seq_stage+38
    clc
    rtl
.l8992:
    stz.w !seq_stage+38
    sec
    rtl

; @routine 03:865E DemoRunThird
; WHAT IT DOES: Runs the third of the attract demos: $1A57 = $159, then the
; shared demo run at $03:8683, then two more (DemoRunThirdRest, $03:866A).
; HOW: A JSL to the demo run, then on into DemoRunThirdRest.
; Entry: D = $0000. Exit: as the demo run leaves it.
%at($03865E)
DemoRunThird:
    rep.b #$30
    lda.w #$0159
    sta.w !seq_stage+34
    jsl.l DemoRun

; @routine 03:866A DemoRunThirdRest
; WHAT IT DOES: DemoRunThird after its first demo returns: two more attract
; demos, $1A57 = $1FA and $287 (the last one waits for a button held,
; command $0B), each by the shared demo run at $03:8683, then the return
; to DemoRunThird's caller.
; HOW: REP #$30, LDA #offset, STA $1A57, JSL to the demo run, twice; RTL.
; Not reached in the traces so far: a unit call of it runs about 6,150
; frames, longer than the gate gives one lockstep run.
; Entry: D = $0000. Exit: as the second demo run leaves it.
%at($03866A)
DemoRunThirdRest:
    rep.b #$30
    lda.w #$01FA
    sta.w !seq_stage+34
    jsl.l DemoRun
    rep.b #$30
    lda.w #$0287
    sta.w !seq_stage+34
    jsl.l DemoRun
    rtl

; @routine 03:8CF2 WramFillZeroLong
; WHAT IT DOES: WramFillZero for callers in other banks: clears Y bytes of
; work RAM from bank A, address X.
; HOW: A near call, then a long return.
; Entry: as WramFillZero. Exit: as WramFillZero.
%at($038CF2)
WramFillZeroLong:
    jsr.w WramFillZero
    rtl

; @routine 03:8CF6 WramFillZero
; WHAT IT DOES: Clears Y bytes of work RAM from bank A, address X.
; HOW: DMA channel 0, fixed source (the zero byte at $03:8D26), to WMDATA
; ($2180) with the work RAM address set at $2181-$2183; starts it.
; Entry: M 8-bit, X 16-bit; A = bank, X = address, Y = byte count (0 means
; 65536). Exit: A low byte = 1; X = $8008; N, Z clear.
%at($038CF6)
WramFillZero:
    stx.w !WMADDL
    sta.w !WMADDH
    sty.w !DAS0L
    ldx.w #!zero_byte_038d26
    stx.w !A1T0L
    lda.b #!zero_byte_038d26>>16
    sta.w !A1B0
    ldx.w #$8008
    stx.w !DMAP0
    lda.b #$01
    sta.w !MDMAEN
    rts

; @routine 03:863D DemoRunFirst
; WHAT IT DOES: Runs the first of the attract demos: $1A57 = 0 (its offset
; in the demo tables at $03:8004), then the shared demo run at $03:8683
; (course, settings and the race itself).
; HOW: A tail JMP into the demo runs of the routine at $03:8648, reached
; as original code (jump_routine).
; Entry: D = $0000. Exit: as the demo run leaves it.
%at($03863D)
DemoRunFirst:
    rep.b #$30
    lda.w #$0000
    sta.w !seq_stage+34
    jmp.w DemoRun

