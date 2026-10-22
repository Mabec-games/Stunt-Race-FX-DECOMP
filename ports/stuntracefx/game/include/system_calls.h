/*
 * system_calls.h: the S-CPU's system routines (game/system) as C functions,
 * for the C that calls them.
 */
#ifndef STUNTRACEFX_SYSTEM_CALLS_H
#define STUNTRACEFX_SYSTEM_CALLS_H

#include <stddef.h>
#include <stdint.h>

/* The updates the frame code applies in the vertical blank
   (frame_updates.c). */
void OamDmaIfPending(void);
void CgramDmaIfPending(void);
void ByteWritesIfPending(uint8_t bank);
void VramDmaQueueAdd(void);
uint16_t VramDmaQueueAddA(uint16_t a);
uint16_t WordListAAppend(uint16_t value);
uint16_t WordListBAppend(uint16_t value);
void VramDmaQueueFlush(void);
/* A byte write queued for the next blank: the entry's offset. */
uint16_t FrameByteWritePush(uint8_t bank, uint16_t addr, uint8_t value);
/* The frame's ends: A's low byte (the brightness). */
uint8_t FrameInputAndCountEnd(uint8_t bank);
uint8_t FrameCountAndPortsEnd(uint8_t bank, uint16_t *high, int *checked);

#endif
