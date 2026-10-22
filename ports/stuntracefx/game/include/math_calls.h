/*
 * math_calls.h: the S-CPU's maths routines (game/math) as C functions, for
 * the C that calls them.
 */
#ifndef STUNTRACEFX_MATH_CALLS_H
#define STUNTRACEFX_MATH_CALLS_H

#include <stddef.h>
#include <stdint.h>

/* $3E moved toward target by the fraction each names, at least its
   minimum (approach.c): the new $3E; *moved (when not NULL) whether it
   moved. The Byte forms work on $3E's low byte. */
uint16_t ApproachHalfMin2(uint16_t target, int *moved);
uint16_t ApproachQuarterMin4(uint16_t target, int *moved);
uint16_t ApproachEighthMin8(uint16_t target, int *moved);
uint16_t ApproachSixteenthMin16(uint16_t target, int *moved);
uint16_t ApproachThirtySecondMin32(uint16_t target, int *moved);
uint16_t ApproachSixtyFourthMin64(uint16_t target, int *moved);
uint16_t ApproachOneTwentyEighthMin128(uint16_t target, int *moved);
uint8_t ApproachHalfMin2Byte(uint8_t target, int *moved);
uint8_t ApproachEighthMin8Byte(uint8_t target, int *moved);
uint8_t ApproachQuarterMin4Byte(uint8_t target, int *moved);
uint8_t ApproachSixteenthMin16Byte(uint8_t target, int *moved);
uint8_t ApproachThirtySecondMin32Byte(uint8_t target, int *moved);
uint8_t ApproachSixtyFourthMin64Byte(uint8_t target, int *moved);
uint8_t ApproachOneTwentyEighthMin128Byte(uint8_t target, int *moved);

/* The same from C that still works in registers: A in, the new $3E in A,
   the flags as the routine leaves them. */
void approach_regs(uint16_t (*f)(uint16_t, int *));
void approach_regs_byte(uint8_t (*f)(uint8_t, int *));

#endif
