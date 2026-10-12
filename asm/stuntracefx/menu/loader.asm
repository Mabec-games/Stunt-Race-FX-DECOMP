; loader.asm: the object that loads a menu screen's graphics, one step per
; frame. Each step either has the Super FX draw a picture into $70:3000
; (program $01:CF42) or uses what is already in Super FX RAM, queues a
; VRAM transfer from Super FX RAM for the next vertical blank, and sets the
; object's handler (+$08) to the next step:
;
;   D1F7 -> D234 -> D26F -> D2C9 -> D304 -> D333 -> D362 -> D391 -> D3D8
;   -> D418 -> D447 -> D476 -> D4A5 (MenuSetExit)
;
; The object being run is the one at $11F9 (D = $0000).
; C: src/stuntracefx/menu/loader.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 0B:D1F7 MenuLoadPictureCC78
; WHAT IT DOES: Draws picture $CC78 (input $17) and queues $1000 bytes of it
; to VRAM $3000; next step D234.
; HOW: GsuCF42To3000, then queue_from_gsu_ram.
; Entry: D = $0000. Exit: A = $D234; X = the object; M 16-bit, X 16-bit;
; N set, Z clear.
%at($0BD1F7)
MenuLoadPictureCC78:
    rep.b #$30
    lda.w #$CC78
    ldx.w #$0017
    jsl.l GsuCF42To3000
    rep.b #$30
    rep.b #$30
    lda.w #$1000
    sta.l !vram_queue_staged
    lda.w #$0070
    sta.l !vram_queue_staged+4
    lda.w #$3000
    sta.l !vram_queue_staged+2
    lda.w #$3000
    sta.l !vram_queue_staged+6
    jsl.l VramDmaQueueAdd
    rep.b #$20
    rep.b #$10
    ldx.w !actor_current
    lda.w #$D234
    sta.b !actor_handler,x
    rts

; @routine 0B:D234 MenuLoadPictureCE78
; WHAT IT DOES: Draws picture $CE78 (input $17) and queues $1000 bytes of it
; to VRAM $6800; next step D26F.
; HOW: As MenuLoadPictureCC78.
; Entry: M 16-bit, X 16-bit, D = $0000. Exit: as MenuLoadPictureCC78, A =
; $D26F.
%at($0BD234)
MenuLoadPictureCE78:
    lda.w #$CE78
    ldx.w #$0017
    jsl.l GsuCF42To3000
    rep.b #$30
    rep.b #$30
    lda.w #$1000
    sta.l !vram_queue_staged
    lda.w #$0070
    sta.l !vram_queue_staged+4
    lda.w #$3000
    sta.l !vram_queue_staged+2
    lda.w #$6800
    sta.l !vram_queue_staged+6
    jsl.l VramDmaQueueAdd
    rep.b #$20
    rep.b #$10
    ldx.w !actor_current
    lda.w #$D26F
    sta.b !actor_handler,x
    rts

; @routine 0B:D2C9 MenuLoadPictureDD8C
; WHAT IT DOES: Draws picture $DD8C (input $17) and queues $1000 bytes of it
; to VRAM $4000; next step D304.
; HOW: As MenuLoadPictureCC78.
; Entry: M 16-bit, X 16-bit, D = $0000. Exit: as MenuLoadPictureCC78, A =
; $D304.
%at($0BD2C9)
MenuLoadPictureDD8C:
    lda.w #$DD8C
    ldx.w #$0017
    jsl.l GsuCF42To3000
    rep.b #$30
    rep.b #$30
    lda.w #$1000
    sta.l !vram_queue_staged
    lda.w #$0070
    sta.l !vram_queue_staged+4
    lda.w #$3000
    sta.l !vram_queue_staged+2
    lda.w #$4000
    sta.l !vram_queue_staged+6
    jsl.l VramDmaQueueAdd
    rep.b #$20
    rep.b #$10
    ldx.w !actor_current
    lda.w #$D304
    sta.b !actor_handler,x
    rts

; @routine 0B:D304 MenuLoadGsu4000
; WHAT IT DOES: Queues $1000 bytes from $70:4000 to VRAM $4800; next step
; D333.
; HOW: queue_from_gsu_ram.
; Entry: D = $0000. Exit: A = $D333; X = the object; M 16-bit, X 16-bit; N
; set, Z clear.
%at($0BD304)
MenuLoadGsu4000:
    rep.b #$30
    lda.w #$1000
    sta.l !vram_queue_staged
    lda.w #$0070
    sta.l !vram_queue_staged+4
    lda.w #$4000
    sta.l !vram_queue_staged+2
    lda.w #$4800
    sta.l !vram_queue_staged+6
    jsl.l VramDmaQueueAdd
    rep.b #$20
    rep.b #$10
    ldx.w !actor_current
    lda.w #$D333
    sta.b !actor_handler,x
    rts

; @routine 0B:D333 MenuLoadGsu5000
; WHAT IT DOES: Queues $1000 bytes from $70:5000 to VRAM $5000; next step
; D362.
; HOW: As MenuLoadGsu4000.
; Entry: as MenuLoadGsu4000. Exit: as MenuLoadGsu4000, A = $D362.
%at($0BD333)
MenuLoadGsu5000:
    rep.b #$30
    lda.w #$1000
    sta.l !vram_queue_staged
    lda.w #$0070
    sta.l !vram_queue_staged+4
    lda.w #$5000
    sta.l !vram_queue_staged+2
    lda.w #$5000
    sta.l !vram_queue_staged+6
    jsl.l VramDmaQueueAdd
    rep.b #$20
    rep.b #$10
    ldx.w !actor_current
    lda.w #$D362
    sta.b !actor_handler,x
    rts

; @routine 0B:D362 MenuLoadGsu6000
; WHAT IT DOES: Queues $1000 bytes from $70:6000 to VRAM $5800; next step
; D391.
; HOW: As MenuLoadGsu4000.
; Entry: as MenuLoadGsu4000. Exit: as MenuLoadGsu4000, A = $D391.
%at($0BD362)
MenuLoadGsu6000:
    rep.b #$30
    lda.w #$1000
    sta.l !vram_queue_staged
    lda.w #$0070
    sta.l !vram_queue_staged+4
    lda.w #$6000
    sta.l !vram_queue_staged+2
    lda.w #$5800
    sta.l !vram_queue_staged+6
    jsl.l VramDmaQueueAdd
    rep.b #$20
    rep.b #$10
    ldx.w !actor_current
    lda.w #$D391
    sta.b !actor_handler,x
    rts

; @routine 0B:D391 MenuLoadPicturesTo7F
; WHAT IT DOES: Draws picture $9068 (input $1D) and keeps its first $2000
; bytes at $7F:A7D7, then picture $F8E8 (input $17) and keeps $6800 bytes at
; $7F:3FD7; next step D3D8.
; HOW: GsuCF42To3000 and a word copy, twice.
; Entry: D = $0000. Exit: A = $D3D8; X = the object; M 16-bit, X 16-bit; N
; set, Z clear.
%at($0BD391)
MenuLoadPicturesTo7F:
    rep.b #$30
    lda.w #$9068
    ldx.w #$001D
    jsl.l GsuCF42To3000
    rep.b #$30
    ldx.w #$0000
.lD3A2:
    lda.l !gsu_picture,x
    sta.l !menu_picture_a,x
    inx
    inx
    cpx.w #$2000
    bne .lD3A2
    lda.w #$F8E8
    ldx.w #$0017
    jsl.l GsuCF42To3000
    rep.b #$30
    ldx.w #$0000
.lD3C0:
    lda.l !gsu_picture,x
    sta.l !menu_picture_b,x
    inx
    inx
    cpx.w #$6800
    bne .lD3C0
    ldx.w !actor_current
    lda.w #$D3D8
    sta.b !actor_handler,x
    rts

; @routine 0B:D3D8 MenuLoadClearGsu
; WHAT IT DOES: Clears $3400 bytes of Super FX RAM from $70:3000 and queues
; $1000 of them to VRAM $0000; next step D418.
; HOW: A word loop of zero stores, then queue_from_gsu_ram.
; Entry: M 16-bit, X 16-bit, D = $0000. Exit: A = $D418; X = the object; N
; set, Z clear.
%at($0BD3D8)
MenuLoadClearGsu:
    ldx.w #$0000
    lda.w #$0000
.lD3DE:
    sta.l !gsu_picture,x
    inx
    inx
    cpx.w #$3400
    bne .lD3DE
    rep.b #$30
    lda.w #$1000
    sta.l !vram_queue_staged
    lda.w #$0070
    sta.l !vram_queue_staged+4
    lda.w #$3000
    sta.l !vram_queue_staged+2
    lda.w #$0000
    sta.l !vram_queue_staged+6
    jsl.l VramDmaQueueAdd
    rep.b #$20
    rep.b #$10
    ldx.w !actor_current
    lda.w #$D418
    sta.b !actor_handler,x
    rts

; @routine 0B:D418 MenuLoadBlank0800
; WHAT IT DOES: Queues $1000 bytes from $70:3000 (cleared) to VRAM $0800;
; next step D447.
; HOW: As MenuLoadGsu4000.
; Entry: as MenuLoadGsu4000. Exit: as MenuLoadGsu4000, A = $D447.
%at($0BD418)
MenuLoadBlank0800:
    rep.b #$30
    lda.w #$1000
    sta.l !vram_queue_staged
    lda.w #$0070
    sta.l !vram_queue_staged+4
    lda.w #$3000
    sta.l !vram_queue_staged+2
    lda.w #$0800
    sta.l !vram_queue_staged+6
    jsl.l VramDmaQueueAdd
    rep.b #$20
    rep.b #$10
    ldx.w !actor_current
    lda.w #$D447
    sta.b !actor_handler,x
    rts

; @routine 0B:D447 MenuLoadBlank1000
; WHAT IT DOES: Queues $1000 bytes from $70:3000 to VRAM $1000; next step
; D476.
; HOW: As MenuLoadGsu4000.
; Entry: as MenuLoadGsu4000. Exit: as MenuLoadGsu4000, A = $D476.
%at($0BD447)
MenuLoadBlank1000:
    rep.b #$30
    lda.w #$1000
    sta.l !vram_queue_staged
    lda.w #$0070
    sta.l !vram_queue_staged+4
    lda.w #$3000
    sta.l !vram_queue_staged+2
    lda.w #$1000
    sta.l !vram_queue_staged+6
    jsl.l VramDmaQueueAdd
    rep.b #$20
    rep.b #$10
    ldx.w !actor_current
    lda.w #$D476
    sta.b !actor_handler,x
    rts

; @routine 0B:D476 MenuLoadBlank1800
; WHAT IT DOES: Queues $400 bytes from $70:3000 to VRAM $1800; next step
; D4A5 (MenuSetExit).
; HOW: As MenuLoadGsu4000.
; Entry: as MenuLoadGsu4000. Exit: as MenuLoadGsu4000, A = $D4A5.
%at($0BD476)
MenuLoadBlank1800:
    rep.b #$30
    lda.w #$0400
    sta.l !vram_queue_staged
    lda.w #$0070
    sta.l !vram_queue_staged+4
    lda.w #$3000
    sta.l !vram_queue_staged+2
    lda.w #$1800
    sta.l !vram_queue_staged+6
    jsl.l VramDmaQueueAdd
    rep.b #$20
    rep.b #$10
    ldx.w !actor_current
    lda.w #$D4A5
    sta.b !actor_handler,x
    rts
