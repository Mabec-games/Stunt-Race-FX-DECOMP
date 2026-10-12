; hw.inc: S-CPU hardware registers used by the code, by their standard names.

includeonce

!APUIO0 = $2140                 ; APU ports 0-3: S-CPU <-> SPC700 mailbox
!APUIO1 = $2141
!APUIO2 = $2142
!APUIO3 = $2143
!HVBJOY = $4212                 ; status: V-blank (bit 7), H-blank (bit 6), joypad busy (bit 0)
!VMAIN = $2115                  ; VRAM address increment mode
!VMADDL = $2116                 ; VRAM word address
!OAMADDL = $2102                ; OAM address
!OAMADDH = $2103
!CGADD = $2121                  ; CGRAM (palette) address
!MDMAEN = $420B                 ; DMA enable, one bit per channel
!DMAP0 = $4300                  ; DMA channel 0: mode, then B-bus register (BBAD0)
!A1T0L = $4302                  ; DMA channel 0: A-bus address
!A1B0 = $4304                   ; DMA channel 0: A-bus bank
!DAS0L = $4305                  ; DMA channel 0: byte count
!GSU_SCMR = $303A               ; Super FX screen mode: who owns ROM (bit 4) and RAM (bit 3)
