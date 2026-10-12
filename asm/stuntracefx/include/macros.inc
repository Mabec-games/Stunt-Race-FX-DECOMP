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
