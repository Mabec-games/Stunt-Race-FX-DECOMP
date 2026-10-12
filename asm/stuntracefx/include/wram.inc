; wram.inc: work RAM and direct page, as the S-CPU code names it.
; Direct page is $0000 wherever the code below relies on it.

; Sound upload stream (audio/apu_upload.asm): long pointer and its bank byte.
!apu_stream_ptr = $FB
!apu_stream_bank = $FD
