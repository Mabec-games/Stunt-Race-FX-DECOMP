; spc.inc: the SPC700's I/O registers and the race driver's RAM, for the
; sound driver's assembly (asm/stuntracefx/audio/spc/).

includeonce

!spc_test = $F0          ; test register
!spc_control = $F1       ; timer enables, port clears, IPL ROM switch
!spc_dspaddr = $F2       ; DSP register number
!spc_dspdata = $F3       ; DSP register data
!spc_cpuio0 = $F4        ; ports to and from the S-CPU ($2140-$2143)
!spc_cpuio1 = $F5
!spc_cpuio2 = $F6
!spc_cpuio3 = $F7
!spc_aux0 = $F8
!spc_aux1 = $F9
!spc_t0target = $FA      ; timer dividers
!spc_t1target = $FB
!spc_t2target = $FC
!spc_t0out = $FD         ; timer counters (read clears)
!spc_t1out = $FE
!spc_t2out = $FF
