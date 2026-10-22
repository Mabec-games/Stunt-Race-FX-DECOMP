/*
 * gsu_calls.h: the S-CPU's routines that drive the Super FX (game/gsu) as
 * C functions, for the C that calls them.
 */
#ifndef STUNTRACEFX_GSU_CALLS_H
#define STUNTRACEFX_GSU_CALLS_H

#include <stdint.h>

/* The renderer's parameter block (params.c). The ones with `uint8_t *p`
   keep the flags of their last arithmetic there. */
uint16_t GsuParamsUpdate(void);
uint16_t GsuParamsFromDp(uint8_t bank);
uint16_t GsuParam216A(uint8_t bank, uint16_t index);
uint16_t GsuParam216B(uint8_t bank, uint16_t index);
uint16_t GsuTableEntryTo1F4C(uint8_t bank, uint16_t entry, uint8_t *p);
uint16_t GsuTableEntryNext(uint8_t bank, uint16_t entry, uint8_t *p);
uint16_t GsuRecordsUpload(uint16_t *x, uint16_t *y, uint8_t *p);

/* The four input bytes at $70:0200: the last one stored comes back (A's
   low byte). The names give the bytes after the leading 0. */
uint8_t GsuBytes200_0F060E(void);
uint8_t GsuBytes200_0F010B(void);
uint8_t GsuBytes200_BFB3BE(void);
uint8_t GsuBytes200_BFB2B3(void);
uint8_t GsuBytes200_BF12B6(void);
uint8_t GsuBytes200_BF11BD(void);
uint8_t GsuBytes200_BF10B9(void);
uint8_t GsuBytes200_BFB7B6_D8C4(void);
uint8_t GsuBytes200_BFBAB9_D8E1(void);
uint8_t GsuBytes200_BFB3BE_D8FE(void);
uint8_t GsuBytes200_BFBBBE(void);
uint8_t GsuBytes200_BFBABD(void);
/* These are called from C that states the time around them itself. */
void GsuBytes200_BFB7B6(void);
void GsuBytes200_BFBCBD(void);
void GsuBytes200_0F0405(void);
void GsuBytes200_0F090A(void);
void GsuBytes200_0F050E(void);
void GsuBytes200_0F080E(void);
void GsuBytes200_BFB5BB(void);
void GsuBytes200_BFBAB9(void);
void GsuBytes200_0F0708(void);
void GsuBytes200_0F0D03(void);

#endif
