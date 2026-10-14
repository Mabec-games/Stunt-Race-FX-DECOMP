; boot.asm: the game's start: the registers reset, the reset vector's
; start, the start after a soft reset and the session's loop.
; C: src/stuntracefx/system/boot.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 03:89B4 HardwareRegsInit
; WHAT IT DOES: Puts the video and CPU registers in a known state: the
; screen blanked at full brightness ($2100 = $8F), every PPU register from
; $2101 to $2133 to 0 except VMAIN = $80, Mode 7's A and D = $0100, CGWSEL
; = $30 and COLDATA = $E0; the NMI, IRQs, the multiplier, divider, timers,
; DMA and HDMA off, WRIO = $FF and slow ROM.
; HOW: 8-bit registers: A = 0 for most stores, X and Y for the others; the
; scroll and Mode 7 registers written twice (their two bytes). Through the
; data bank.
; Entry: any widths. Exit: A = 0, X = $FF, Y = $30 (8-bit); N set, Z clear.
%at($0389B4)
HardwareRegsInit:
    sep.b #$30
    lda.b #$8F
    sta.w !INIDISP
    lda.b #$00
    sta.w !OBSEL
    sta.w !OAMADDL
    sta.w !OAMADDH
    sta.w !BGMODE
    sta.w !MOSAIC
    sta.w !BG1SC
    sta.w !BG2SC
    sta.w !BG3SC
    sta.w !BG4SC
    sta.w !BG12NBA
    sta.w !BG34NBA
    sta.w !BG1HOFS
    sta.w !BG1HOFS
    sta.w !BG1VOFS
    sta.w !BG1VOFS
    sta.w !BG2HOFS
    sta.w !BG2HOFS
    sta.w !BG2VOFS
    sta.w !BG2VOFS
    sta.w !BG3HOFS
    sta.w !BG3HOFS
    sta.w !BG3VOFS
    sta.w !BG3VOFS
    sta.w !BG4HOFS
    sta.w !BG4HOFS
    sta.w !BG4VOFS
    sta.w !BG4VOFS
    ldx.b #$80
    stx.w !VMAIN
    sta.w !VMADDL
    sta.w !VMADDH
    sta.w !M7SEL
    ldx.b #$01
    sta.w !M7A
    stx.w !M7A
    sta.w !M7B
    sta.w !M7B
    sta.w !M7C
    sta.w !M7C
    sta.w !M7D
    stx.w !M7D
    sta.w !M7X
    sta.w !M7X
    sta.w !M7Y
    sta.w !M7Y
    sta.w !CGADD
    sta.w !CGDATA
    sta.w !W12SEL
    sta.w !W34SEL
    sta.w !WOBJSEL
    sta.w !WH0
    sta.w !WH1
    sta.w !WH2
    sta.w !WH3
    sta.w !WBGLOG
    sta.w !WOBJLOG
    sta.w !TM
    sta.w !TS
    sta.w !TMW
    ldy.b #$30
    sty.w !CGWSEL
    sta.w !CGADSUB
    ldx.b #$E0
    stx.w !COLDATA
    sta.w !SETINI
    sta.w !NMITIMEN
    ldx.b #$FF
    stx.w !WRIO
    sta.w !WRMPYA
    sta.w !WRMPYB
    sta.w !WRDIVL
    sta.w !WRDIVH
    sta.w !WRDIVB
    sta.w !HTIMEL
    sta.w !HTIMEH
    sta.w !VTIMEL
    sta.w !VTIMEH
    sta.w !MDMAEN
    sta.w !HDMAEN
    sta.w !MEMSEL
    rtl

; @routine 03:8AA9 BootColdStart
; WHAT IT DOES: The game's start after the reset vector: native mode, the
; stack at $02FF and the direct page at 0, $0D3F = $1C, the registers reset
; (HardwareRegsInit), $0000-$02FE cleared, work RAM $0300-$20FF,
; $7E:2000-$FFFF and $7F cleared and Super FX RAM $0000-$27FF, the sound
; program uploaded (ApuUploadBank19), the save checked (SaveUnlocksCheck)
; and the interrupt stubs copied to $0101; then BootStart.
; HOW: As the original: STZ through the direct page, WramFillZero, MVN from
; each byte to the next; the stubs from $03:8D16. Never returns.
; Entry: from the reset vector. Exit: never returns.
%at($038AA9)
BootColdStart:
    sep.b #$30
    sei
    clc
    xce
    phk
    plb
    rep.b #$30
    ldx.w #$02FF
    txs
    lda.w #$0000
    tcd
    sep.b #$20
    lda.b #$1C
    sta.w !wram_0D3F
    jsl.l HardwareRegsInit
    cli
    sep.b #$20
    rep.b #$10
    ldx.w #$02FE
Boot_l8ACD:
    stz.b !bank0_base,x
    dex
    bpl Boot_l8ACD
    rep.b #$20
    lda.w #$0000
    tcd
    lda.w #$02FF
    tcs
    phk
    plb
    sep.b #$20
    rep.b #$10
    lda.b #$00
    ldx.w #$0300
    ldy.w #$2100
    jsr.w WramFillZero
    lda.b #$7E
    ldx.w #$2000
    ldy.w #$E000
    jsr.w WramFillZero
    lda.b #$7F
    ldx.w #$0000
    ldy.w #$FFFF
    jsr.w WramFillZero
    phb
    php
    rep.b #$30
    lda.w #$0000
    sta.l !gsu_ram
    lda.w #$27FF
    ldx.w #$0000
    ldy.w #$0001
    mvn !gsu_ram>>16,!gsu_ram>>16
    plp
    plb
    jsl.l ApuUploadBank19
    jsl.l SaveUnlocksCheck
    rep.b #$30
    ldx.w #$000F
Boot_l8B29:
    lda.l !boot_interrupt_stubs,x
    sta.l !gsu_wait_count_hi+2,x
    dex
    bpl Boot_l8B29

; @routine 03:8B34 BootStart
; WHAT IT DOES: The start from a cleared machine (the soft reset's jump):
; copies the code the game runs from work RAM ($02:8000 to $7E:A2D9, $54A6
; bytes; $0B:8000 to $7F:0800, $2C00 bytes, by DMA), sets the Super FX
; (clock, $0374 = 1, config $20, screen base $0B), runs the title
; (MenuScreen0Run), sets $1915 = 3 and $1917 = 0; then BootWarm.
; HOW: DMA channel 0 to WMDATA; 8-bit A, 16-bit X and Y. Never returns.
; Entry: data bank $03. Exit: never returns.
%at($038B34)
BootStart:
    sep.b #$20
    rep.b #$10
    ldx.w #$8000
    stx.w !DMAP0
    ldy.w #$A2D9
    sty.w !WMADDL
    lda.b #$7E
    sta.w !WMADDH
    stx.w !A1T0L
    lda.b #$02
    sta.w !A1B0
    ldy.w #$54A6
    sty.w !DAS0L
    lda.b #$01
    sta.w !MDMAEN
    ldx.w #$8000
    stx.w !DMAP0
    ldy.w #$0800
    sty.w !WMADDL
    lda.b #$7F
    sta.w !WMADDH
    stx.w !A1T0L
    lda.b #$0B
    sta.w !A1B0
    ldy.w #$2C00
    sty.w !DAS0L
    lda.b #$01
    sta.w !MDMAEN
    sep.b #$20
    lda.b #$01
    sta.w !gsu_scmr_shadow
    lda.b #$01
    sta.w !GSU_CLSR
    lda.b #$20
    sta.w !GSU_CFGR
    lda.b #$0B
    sta.w !GSU_SCBR
    jsl.l MenuScreen0Run
    rep.b #$30
    lda.w #$0003
    sta.l !wram_1913+2
    lda.w #$0000
    sta.l !wram_1913+4

; @routine 03:8BAA BootWarm
; WHAT IT DOES: The game's loop of a session: the stack reset, sound on, the
; course menu (CourseMenuRun) and the race's setup (RaceModeSetup,
; Set7EFFA6Sequence), the race (RaceLoop) with the screen blanked before
; and after, the video reset (RaceVideoInit), then the attract race: 02:CF3E,
; the windows, the Super FX clock, the palette flag, 02:CF45, the course
; (CourseDataPointer, CourseScriptRun, ObjectPlaceAll), program $04:8800,
; and race frames for ever (RaceFrameRun, or RaceFrameRunTwoPlayers with
; $0D62 set, after SpriteSlot1Countdown), $0713 cleared once $05E9 reaches
; $1E.
; HOW: Data bank $03; the stack reset to $02FF three times; the JSL to
; RaceLoop leaves its return address on the stack for RaceLoop's way out.
; Entry: any. Exit: never returns.
%at($038BAA)
BootWarm:
    phk
    plb
    rep.b #$10
    ldx.w #$02FF
    txs
    sep.b #$20
    stz.w !sound_muted
    lda.b #$02
    sta.w !table_116c+14
    lda.b #$01
    sta.w !table_116c+15
    jsl.l CourseMenuRun
    jsl.l RaceModeSetup
    jsl.l Set7EFFA6Sequence
    phk
    plb
    sep.b #$20
    stz.w !HDMAEN
    lda.b #$80
    sta.w !INIDISP
    sei
    rep.b #$10
    ldx.w #$02FF
    txs
    sep.b #$20
    stz.w !sound_muted
    jsl.l RaceLoop
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
    rep.b #$10
    ldx.w #$02FF
    txs
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
    sep.b #$20
    rep.b #$10
    jsl.l AttractCourseSetup
    jsl.l CourseDataPointer
    jsl.l CourseScriptRun
    jsl.l ObjectPlaceAll
    lda.b #$00
    sta.l !fx_noise_seed+30
    sta.w !crash_colour+4
    stz.w !wram_05E9
    lda.b #$01
    sta.w !scanline_wait_high+47
    sep.b #$20
    rep.b #$10
    lda.b #$04
    ldx.w #$8800
    jsl.l GsuRun
    sep.b #$20
    lda.b #$01
    sta.w !wram_0357
    lda.l !TIMEUP
    cli
Boot_l8C63:
    jsl.l SpriteSlot1Countdown
    sep.b #$20
    lda.w !wram_0D62
    beq Boot_l8C74
    jsl.l RaceFrameRunTwoPlayers
    bra Boot_l8C78
Boot_l8C74:
    jsl.l RaceFrameRun
Boot_l8C78:
    sep.b #$20
    lda.w !wram_05E9
    cmp.b #$1E
    bcc Boot_l8C63
    stz.w !scanline_wait_high+47
    bra Boot_l8C63

; @routine 03:8C86 BootSoftRestart
; WHAT IT DOES: Restarts the game from a running one: $10DC = $FFFF and
; $10DE = 0, 03:865E, command $06 to APU port 1, the screen blanked, the
; NMI, DMA and HDMA off, work RAM and Super FX RAM cleared as at the start,
; the sound restarted (SoftResetApuLong); then BootStart. Nothing reaches it:
; the attract race's loop before it never ends.
; HOW: As BootColdStart's clears; a jump to BootStart. Never returns.
; Entry: data bank $03. Exit: never returns.
%at($038C86)
BootSoftRestart:
    rep.b #$20
    lda.w #$FFFF
    sta.w !wram_10DC
    stz.w !wram_10DC+2
    jsl.l DemoRunThird
    sep.b #$20
    lda.b #$06
    sta.w !APUIO1
    lda.b #$80
    sta.w !INIDISP
    stz.w !NMITIMEN
    stz.w !MDMAEN
    stz.w !HDMAEN
    sep.b #$20
    rep.b #$10
    lda.b #$00
    ldx.w #$0300
    ldy.w #$2100
    jsl.l WramFillZeroLong
    lda.b #$7E
    ldx.w #$2000
    ldy.w #$E000
    jsl.l WramFillZeroLong
    lda.b #$7F
    ldx.w #$0000
    ldy.w #$FFFF
    jsl.l WramFillZeroLong
    phb
    php
    rep.b #$30
    lda.w #$0000
    sta.l !gsu_ram
    lda.w #$27FF
    ldx.w #$0000
    ldy.w #$0001
    mvn !gsu_ram>>16,!gsu_ram>>16
    plp
    plb
    jsl.l SoftResetApuLong
    jmp.w BootStart
