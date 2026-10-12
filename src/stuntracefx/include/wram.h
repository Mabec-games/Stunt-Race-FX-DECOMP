/*
 * wram.h: work RAM and direct page, as the C names it. The same names as
 * asm/stuntracefx/include/wram.inc, as 24-bit S-CPU addresses. A variable
 * whose purpose is not yet established keeps its address as its name.
 */
#ifndef STUNTRACEFX_WRAM_H
#define STUNTRACEFX_WRAM_H

/* Sound upload stream (audio/apu_upload.c): long pointer and its bank byte. */
#define APU_STREAM_PTR 0x0000FBu
#define APU_STREAM_BANK 0x0000FDu

/* The value last written to the Super FX screen mode register (video/dma.c). */
#define GSU_SCMR_SHADOW 0x000374u

/* The work-RAM copy of the sprite table (video/oam.c): 128 four-byte entries
   (X, Y, tile, attributes) from $0375, then the 32-byte high table from $0575. */
#define OAM_BUFFER 0x000375u
#define OAM_BUFFER_HIGH 0x000575u

/* Sound command queues (audio/sound_queue.c): two 16-byte rings, drained one
   byte per frame to APU ports 0 and 1 once the sound CPU has taken the last. */
#define SOUND_PORT0_QUEUE 0x000DA8u
#define SOUND_PORT1_QUEUE 0x000DB8u
#define SOUND_PORT0_HEAD 0x000DC8u
#define SOUND_PORT1_HEAD 0x000DCAu
#define SOUND_PORT0_COUNT 0x000DCCu
#define SOUND_PORT1_COUNT 0x000DCEu
#define WRAM_0DD0 0x000DD0u
#define SOUND_PORT1_LAST 0x000DD2u  /* the last byte sent on port 1 */
#define WRAM_0E1A 0x000E1Au
#define WRAM_0E1B 0x000E1Bu
#define WRAM_0E1C 0x000E1Cu
#define SOUND_MUTED 0x000E4Eu       /* nonzero: nothing is queued */

/* Held sounds (audio/sound_effects.c): the sound each player's car holds. */
#define PLAYER1_HELD_SOUND 0x000E4Fu
#define PLAYER2_HELD_SOUND 0x000E50u

/* The player whose car is being processed: 0 or 1. */
#define CURRENT_PLAYER 0x001A5Du

#endif
