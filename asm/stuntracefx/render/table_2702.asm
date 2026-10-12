; table_2702.asm: a Super FX program that writes a fixed table of twelve
; four-byte entries at $70:2702: a zero, then three bytes.
; C: src/stuntracefx/render/table_2702.c.

incsrc "../include/macros.inc"

arch superfx

; @routine 01:D1EB FxTable2702Program
; WHAT IT DOES: A program the S-CPU starts: writes the fixed 48-byte table
; at $70:2702.
; HOW: Sets the RAM bank to 0, then stores each byte with STB through R1,
; loading it with IBT (zero) or IWT (the rest), INC R1 after each. Then
; STOP.
; Entry: started by the S-CPU. Exit (at the STOP): R1 = $2732; R0 = the
; last byte; the RAM bank 0; the last RAM address $2731; CY set and OV
; clear (SUB R0); S and Z from R1.
%at($01D1EB)
FxTable2702Program:
    sub r0
    ramb
    iwt r1,#$2702
    ibt r0,#$00
    stb (r1)
    inc r1
    iwt r0,#$001F
    stb (r1)
    inc r1
    iwt r0,#$001B
    stb (r1)
    inc r1
    iwt r0,#$001E
    stb (r1)
    inc r1
    ibt r0,#$00
    stb (r1)
    inc r1
    iwt r0,#$00BF
    stb (r1)
    inc r1
    iwt r0,#$00B7
    stb (r1)
    inc r1
    iwt r0,#$00B6
    stb (r1)
    inc r1
    ibt r0,#$00
    stb (r1)
    inc r1
    iwt r0,#$00BF
    stb (r1)
    inc r1
    iwt r0,#$00BC
    stb (r1)
    inc r1
    iwt r0,#$00BD
    stb (r1)
    inc r1
    ibt r0,#$00
    stb (r1)
    inc r1
    iwt r0,#$00BF
    stb (r1)
    inc r1
    iwt r0,#$00B5
    stb (r1)
    inc r1
    iwt r0,#$00BB
    stb (r1)
    inc r1
    ibt r0,#$00
    stb (r1)
    inc r1
    iwt r0,#$001F
    stb (r1)
    inc r1
    iwt r0,#$005B
    stb (r1)
    inc r1
    iwt r0,#$0058
    stb (r1)
    inc r1
    ibt r0,#$00
    stb (r1)
    inc r1
    iwt r0,#$00BF
    stb (r1)
    inc r1
    iwt r0,#$00B2
    stb (r1)
    inc r1
    iwt r0,#$00B3
    stb (r1)
    inc r1
    ibt r0,#$00
    stb (r1)
    inc r1
    iwt r0,#$001E
    stb (r1)
    inc r1
    iwt r0,#$0015
    stb (r1)
    inc r1
    iwt r0,#$0014
    stb (r1)
    inc r1
    ibt r0,#$00
    stb (r1)
    inc r1
    iwt r0,#$0017
    stb (r1)
    inc r1
    iwt r0,#$0019
    stb (r1)
    inc r1
    iwt r0,#$001A
    stb (r1)
    inc r1
    ibt r0,#$00
    stb (r1)
    inc r1
    iwt r0,#$0017
    stb (r1)
    inc r1
    iwt r0,#$0018
    stb (r1)
    inc r1
    iwt r0,#$001E
    stb (r1)
    inc r1
    ibt r0,#$00
    stb (r1)
    inc r1
    iwt r0,#$0017
    stb (r1)
    inc r1
    iwt r0,#$0019
    stb (r1)
    inc r1
    iwt r0,#$001A
    stb (r1)
    inc r1
    ibt r0,#$00
    stb (r1)
    inc r1
    iwt r0,#$0017
    stb (r1)
    inc r1
    iwt r0,#$0018
    stb (r1)
    inc r1
    iwt r0,#$0015
    stb (r1)
    inc r1
    ibt r0,#$00
    stb (r1)
    inc r1
    iwt r0,#$0014
    stb (r1)
    inc r1
    iwt r0,#$0015
    stb (r1)
    inc r1
    iwt r0,#$001E
    stb (r1)
    inc r1
    stop
    nop

arch 65816
