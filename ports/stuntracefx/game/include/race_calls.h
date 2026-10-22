/*
 * race_calls.h: the race routines (game/race) as C functions, for the C that
 * calls them.
 */
#ifndef STUNTRACEFX_RACE_CALLS_H
#define STUNTRACEFX_RACE_CALLS_H

#include <stdint.h>

/* The race time (time.c). RaceTimeInit starts $0E70 by the mode in $0D2B:
   returns the value; *x is X (8-bit) as the path leaves it, *p is P. */
uint8_t RaceTimeInit(uint8_t bank, uint8_t *x, uint8_t *p);
/* One clock step; acc is A at entry, the result is A at exit; p0 the
   caller's P (restored on exit). */
uint16_t RaceClockStep(uint8_t bank, uint16_t acc, uint8_t p0);
uint16_t RaceTimeAddD815(uint16_t *x, uint8_t *p);
uint16_t RaceTimeAddD814(uint16_t *x, uint8_t *p);

/* Lap splits (lap.c): LapSplit returns the minutes, *p is P; RaceTimeKeep
   returns the minutes. */
uint8_t LapSplit(uint8_t bank, uint8_t x, uint8_t *p);
uint8_t RaceTimeKeep(uint8_t bank);

#endif
