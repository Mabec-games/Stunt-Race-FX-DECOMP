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

/* The sliding value (slide.c): A in, A out; SlideOutWobble pulls the data
   bank its caller pushed into *pulled and returns A's low byte. */
uint16_t SlideStep(uint16_t a);
uint8_t SlideOutWobble(uint8_t bank, uint8_t *pulled);

/* The stop screens (crash.c); `bank` the data bank the shared body stores
   through. They never return. */
void CrashWhite(uint8_t bank);
void CrashGreen(uint8_t bank);

/* Debug text and numbers (debug_text.c, debug_numbers.c). DebugDrawChar
   draws the character ch; A, X and Y come back as it leaves them. */
void DebugDrawChar(uint8_t bank, uint8_t ch, uint16_t *a, uint16_t *x, uint16_t *y);

/* A, X and Y as the last DebugDrawChar of a number's digits leaves them. */
typedef struct {
    uint16_t a, x, y;
} DebugRegs;

void DebugHexByte(uint8_t bank, uint8_t byte, DebugRegs *r);
void DebugDecimalByte(uint8_t bank, uint8_t byte, DebugRegs *r);
void DebugSignedByte(uint8_t bank, uint8_t byte, DebugRegs *r);
void DebugPrintBinaryByte(uint8_t bank, uint8_t byte, DebugRegs *r);
void DebugPrintDecimalWord(uint16_t value);
/* The long forms keep every register: they return the A the printer left,
   `p` the P they come with. */
uint16_t DebugPrintHexByte(uint8_t bank, uint8_t byte, uint8_t p);
uint16_t DebugPrintDecimalByte(uint8_t bank, uint8_t byte, uint8_t p);
uint16_t DebugPrintSignedByte(uint8_t bank, uint8_t byte, uint8_t p);
uint16_t DebugPrintHexWord(uint8_t bank, uint16_t a, uint8_t p);

#endif
