; macros.inc: placement and shared macros for the companion assembly.

includeonce

; %at(addr): place the following code at S-CPU address `addr` (LoROM, banks
; $00-$3F). Sets the file position and the address labels resolve to.
macro at(addr)
    org ((<addr>>>16)&$3F)*$8000+(<addr>&$7FFF)
    base <addr>
endmacro

; %ramcode(run, rom): place code that the game copies to work RAM and runs
; there. It sits at ROM offset `rom`; labels resolve to its run address `run`.
macro ramcode(run, rom)
    org <rom>
    base <run>
endmacro

; %spc(apu, addr): place SPC700 code that the S-CPU uploads to the sound
; CPU's RAM. It sits at the ROM byte of S-CPU address `addr` (LoROM, banks
; $00-$3F); labels resolve to its APU RAM address `apu`. %spc_end() goes
; back to 65816 code.
macro spc(apu, addr)
    arch spc700
    org ((<addr>>>16)&$3F)*$8000+(<addr>&$7FFF)
    base <apu>
endmacro

macro spc_end()
    arch 65816
endmacro
