; run.asm: starting the Super FX and waiting for it. Runs from work RAM.
; C: src/stuntracefx/gsu/run.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"

arch 65816

; @routine 7E:E1CB GsuWait
; WHAT IT DOES: Waits for a running Super FX program to stop, then takes the
; ROM and RAM back for the S-CPU.
; HOW: Steps the 16-bit poll count and reads the status register until its
; running bit (5) clears, then writes the S-CPU's screen mode value.
; Entry: X 16-bit, D = $0000, DB a bank where $3030 is the Super FX.
; Exit: A low byte = the screen mode value; N, Z from it; M 8-bit.
%ramcode($7EE1CB, $013EF2)
GsuWait:
    sep.b #$20
.poll:
    inc.b !gsu_wait_count
    bne .counted
    inc.b !gsu_wait_count+1
.counted:
    lda.w !GSU_SFR
    and.b #$20
    bne .poll
    lda.w !gsu_scmr_shadow
    sta.w !GSU_SCMR
    rts

; @routine 7E:E1F5 GsuRun
; WHAT IT DOES: Runs a Super FX program to completion.
; HOW: Sets the program bank, gives ROM and RAM to the Super FX, clears its
; status and writes the start address to R15, which starts it; then waits as
; GsuWait does. The data bank is 0 inside and restored on return.
; Entry: M 8-bit, X 16-bit, D = $0000; A = program bank, X = start address.
; Exit: A low byte = the screen mode value; N, Z from the data bank pulled
; back on return; X unchanged.
%ramcode($7EE1F5, $013F1C)
GsuRun:
    sta.l !GSU_PBR
    phb
    lda.b #$00
    pha
    plb
    lda.w !gsu_scmr_shadow
    ora.b #$18
    sta.w !GSU_SCMR
    stz.w !GSU_SFR
    stx.w !GSU_R15
.poll:
    inc.b !gsu_wait_count
    bne .counted
    inc.b !gsu_wait_count+1
.counted:
    lda.w !GSU_SFR
    and.b #$20
    bne .poll
    lda.w !gsu_scmr_shadow
    sta.w !GSU_SCMR
    plb
    rtl

; @routine 7F:0919 GsuAB13WithParams
; WHAT IT DOES: Updates the renderer's parameter block, runs Super FX
; program $01:AB13 to completion and takes the bus back, counting the polls
; in $1F21-$1F22.
; HOW: GsuParamsUpdate, then as GsuRun but written out: program bank 1,
; screen mode $0374 | $18, R15 = $AB13; each poll steps the 16-bit count
; (low byte, then the high byte when the low one wraps) before reading the
; status. Everything but the parameter copy goes through the data bank.
; Entry: any widths, D = $0000. Exit: A low byte = the screen mode value,
; high byte from the last parameter copied; X = $AB13 ($13 when P
; restores 8-bit index registers); P unchanged.
%ramcode($7F0919, $058119)
GsuAB13WithParams:
    php
    jsl.l GsuParamsUpdate
    sep.b #$20
    rep.b #$10
    lda.b #$01
    ldx.w #$AB13
    sta.w !GSU_PBR
    lda.w !gsu_scmr_shadow
    ora.b #$18
    sta.w !GSU_SCMR
    stx.w !GSU_R15
    sep.b #$20
.l0937:
    inc.w !gsu_ab13_polls
    bne .l093F
    inc.w !gsu_ab13_polls+1
.l093F:
    lda.w !GSU_SFR
    and.b #$20
    bne .l0937
    lda.w !gsu_scmr_shadow
    sta.w !GSU_SCMR
    plp
    rtl

; @routine 7E:E1E1 FrameObjectsWhileGsuRuns
; WHAT IT DOES: The S-CPU's share of a race frame while the Super FX
; draws: the trigger list for the view (BucketTriggersFromView), the draw
; list (ObjectDrawListBuild), the fixed area's triggers
; (BucketTriggersFixedArea), the object groups (ObjectGroupsActivate);
; then waits for the Super FX (GsuWait) and uploads the records it needs
; next (GsuRecordsUpload).
; HOW: Six calls in turn.
; Entry: X 16-bit, D = $0000. Exit: as GsuRecordsUpload leaves it.
%ramcode($7EE1E1, $013F08)
FrameObjectsWhileGsuRuns:
    jsr.w BucketTriggersFromView
    jsr.w ObjectDrawListBuild
    jsr.w BucketTriggersFixedArea
    jsr.w ObjectGroupsActivate
    jsr.w GsuWait
    jsl.l GsuRecordsUpload
    rts

; @routine 7E:E221 FrameRunGsu8FCA
; WHAT IT DOES: Starts Super FX program $04:8FCA, does the frame's object
; work while it runs (FrameObjectsWhileGsuRuns), and keeps the number of
; polls the wait took ($FE) in $1F1F.
; HOW: As GsuRun, without waiting before the object work; the poll count
; in direct page $FE is cleared first. Data bank 0 inside, restored at the
; end.
; Entry: D = $0000. Exit: A = the poll count; X, Y as GsuRecordsUpload
; leaves them; M 8-bit, X 16-bit; N, Z from the data bank pulled back.
%ramcode($7EE221, $013F48)
FrameRunGsu8FCA:
    sep.b #$20
    rep.b #$10
    stz.b !gsu_wait_count
    stz.b !gsu_wait_count_hi
    lda.b #$04
    ldx.w #$8FCA
    sta.l !GSU_PBR
    phb
    lda.b #$00
    pha
    plb
    lda.w !gsu_scmr_shadow
    ora.b #$18
    sta.w !GSU_SCMR
    stz.w !GSU_SFR
    stx.w !GSU_R15
    jsr.w FrameObjectsWhileGsuRuns
    rep.b #$20
    lda.b !gsu_wait_count
    sta.w !gsu_wait_polls
    sep.b #$20
    plb
    rtl

; @routine 7E:E303 HudFrameWhileGsuRuns
; WHAT IT DOES: The HUD's share of a frame while the Super FX draws: the
; slide and the fade value step (SlideStep, FadeValueStep), DataBankTest;
; then, with $0D62 clear, the HUD digits (HudDigitSprites62 unless the HUD
; mode $0D2B is 1 or 3; HudDrawDigits19EC unless it is 3), the backdrop
; rows (BackdropRowsUpdate), value A (HudDrawValueA) and the colour work
; at $7E:E34F; with $0D62 set, the backdrop scroll (BackdropScrollFromView)
; and value C for screen buffer 1 ($0D7F), else value B.
; HOW: 8-bit tests; long loads of $0D2B and $0D7F; $7E:E34F is
; HudColourStep.
; Entry: any widths; D = $0000. Exit: as the last call leaves it.
%ramcode($7EE303, $01402A)
HudFrameWhileGsuRuns:
    jsl.l SlideStep
    jsl.l FadeValueStep
    jsl.l DataBankTest
    sep.b #$30
    lda.w !wram_0D62
    beq .lE319
    jmp.w .lE338
.lE319:
    ldx.b #$00
    lda.l !hud_mode
    cmp.b #$01
    beq .lE32A
    cmp.b #$03
    beq .lE32D
    jsr.w HudDigitSprites62
.lE32A:
    jsr.w HudDrawDigits19EC
.lE32D:
    jsr.w BackdropRowsUpdate
    jsr.w HudDrawValueA
    jsl.l HudColourStep
    rts
.lE338:
    lda.l !gsu_buffer_select
    tay
    cpy.b #$01
    beq .lE348
    jsr.w BackdropScrollFromView
    jsr.w HudDrawValueB
    rts
.lE348:
    jsr.w BackdropScrollFromView
    jsr.w HudDrawValueC
    rts

; @routine 7E:E258 FrameRunGsuAB13B
; WHAT IT DOES: A frame for HUD panel B: clears the poll count $FE,
; updates the renderer's parameters (GsuParamsUpdate), starts Super FX
; program $01:AB13, and while it draws does the HUD's frame
; (HudFrameWhileGsuRuns), the gauges (HudGaugesPanelB) and, with $0D62
; set, the objects (FrameObjectsWhileGsuRuns); then waits (GsuWait), keeps
; the poll count in $1F21 and draws the map markers (MapMarkersPanelB).
; HOW: M 8-bit, X 16-bit; the Super FX started as GsuRun does, without
; waiting.
; Entry: D = $0000. Exit: as MapMarkersPanelB leaves it.
%ramcode($7EE258, $013F7F)
FrameRunGsuAB13B:
    sep.b #$20
    rep.b #$10
    stz.b !gsu_wait_count
    stz.b !gsu_wait_count_hi
    jsl.l GsuParamsUpdate
    lda.b #$01
    ldx.w #$AB13
    sta.w !GSU_PBR
    lda.w !gsu_scmr_shadow
    ora.b #$18
    sta.w !GSU_SCMR
    stz.w !GSU_SFR
    stx.w !GSU_R15
    jsr.w HudFrameWhileGsuRuns
    jsr.w HudGaugesPanelB
    sep.b #$20
    lda.w !wram_0D62
    beq .lE28A
    jsr.w FrameObjectsWhileGsuRuns
.lE28A:
    jsr.w GsuWait
    rep.b #$20
    lda.b !gsu_wait_count
    sta.w !gsu_ab13_polls
    jsr.w MapMarkersPanelB
    rtl

; @routine 7E:E298 FrameRunGsuAB13A
; WHAT IT DOES: A frame for HUD panel A: updates the renderer's parameters,
; clears the poll count $1F21, starts Super FX program $01:AB13, and while
; it draws does the HUD's frame (HudFrameWhileGsuRuns) and the gauges
; (HudGaugesPanelA); then waits (GsuWait) and draws the map markers
; (MapMarkersPanelA).
; HOW: As FrameRunGsuAB13B, without the objects or the poll count kept.
; Entry: D = $0000. Exit: as MapMarkersPanelA leaves it.
%ramcode($7EE298, $013FBF)
FrameRunGsuAB13A:
    sep.b #$20
    rep.b #$10
    jsl.l GsuParamsUpdate
    stz.w !gsu_ab13_polls
    stz.w !gsu_ab13_polls+1
    lda.b #$01
    ldx.w #$AB13
    sta.w !GSU_PBR
    lda.w !gsu_scmr_shadow
    ora.b #$18
    sta.w !GSU_SCMR
    stz.w !GSU_SFR
    stx.w !GSU_R15
    jsr.w HudFrameWhileGsuRuns
    jsr.w HudGaugesPanelA
    jsr.w GsuWait
    jsr.w MapMarkersPanelA
    rtl
