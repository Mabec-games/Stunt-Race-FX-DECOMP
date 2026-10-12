; hw.inc: S-CPU hardware registers used by the code, by their standard names.

includeonce

!APUIO0 = $2140                 ; APU ports 0-3: S-CPU <-> SPC700 mailbox
!APUIO1 = $2141
!APUIO2 = $2142
!APUIO3 = $2143
!HVBJOY = $4212                 ; status: V-blank (bit 7), H-blank (bit 6), joypad busy (bit 0)
!VMAIN = $2115                  ; VRAM address increment mode
!VMADDL = $2116                 ; VRAM word address
!VMDATAL = $2118                ; VRAM data write (low, then high at $2119)
!OAMADDL = $2102                ; OAM address
!OAMADDH = $2103
!CGADD = $2121                  ; CGRAM (palette) address
!MDMAEN = $420B                 ; DMA enable, one bit per channel
!DMAP0 = $4300                  ; DMA channel 0: mode, then B-bus register (BBAD0)
!A1T0L = $4302                  ; DMA channel 0: A-bus address
!A1B0 = $4304                   ; DMA channel 0: A-bus bank
!DAS0L = $4305                  ; DMA channel 0: byte count
!GSU_SCMR = $303A               ; Super FX screen mode: who owns ROM (bit 4) and RAM (bit 3)
!INIDISP = $2100      ; screen brightness and forced blank
!OAMDATA = $2104      ; OAM data write
!BG2HOFS = $210F      ; BG2 horizontal scroll (written twice)
!BG2VOFS = $2110      ; BG2 vertical scroll (written twice)
!M7A = $211B          ; Mode 7 matrix A, also the multiplier's 16-bit factor (written twice)
!M7B = $211C          ; Mode 7 matrix B, also the multiplier's 8-bit factor
!CGDATA = $2122       ; CGRAM data write
!W12SEL = $2123       ; window masks for BG1/BG2
!W34SEL = $2124       ; window masks for BG3/BG4
!WOBJSEL = $2125      ; window masks for sprites and colour
!WBGLOG = $212A       ; window logic for BGs
!WOBJLOG = $212B      ; window logic for sprites and colour
!TMW = $212E          ; main screen window mask enables
!CGWSEL = $2130       ; colour maths control
!CGADSUB = $2131      ; colour maths add/subtract and layers
!MPYM = $2135         ; multiplier result, middle byte (then high at $2136)
!JOY1L = $4218        ; joypad 1 auto-read result
!JOY2L = $421A        ; joypad 2 auto-read result
!GSU_R15 = $301E      ; Super FX program counter; writing it starts the Super FX
!GSU_SFR = $3030      ; Super FX status (bit 5: running)
!GSU_PBR = $3034      ; Super FX program bank
