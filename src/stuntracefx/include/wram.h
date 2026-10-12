/*
 * wram.h: work RAM and direct page, as the C names it. The same names as
 * asm/stuntracefx/include/wram.inc, as 24-bit S-CPU addresses.
 */
#ifndef STUNTRACEFX_WRAM_H
#define STUNTRACEFX_WRAM_H

/* Sound upload stream (audio/apu_upload.c): long pointer and its bank byte. */
#define APU_STREAM_PTR 0x0000FBu
#define APU_STREAM_BANK 0x0000FDu

#endif
