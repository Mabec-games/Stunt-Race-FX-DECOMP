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

/* Rotating a vector by an 8-bit angle (rotate.c): the high byte of the
   second result. */
uint8_t RotateXZ(uint8_t angle);
uint8_t RotateXZFrom08(uint8_t angle);
uint8_t RotateXY(uint8_t angle);
/* The signed word in $67 times the signed byte at $69 (times 2), top 16
   bits to $6A; the data bank for the multiplier's registers; *yout the
   first product's high byte. */
uint16_t MulWordByte(uint8_t bank, uint8_t *yout);
uint16_t MulWordByteLong(uint8_t bank, uint8_t *yout);
/* The 16-bit rotations: the angle, X and Y (8 bits wide when x8) in and
   out; returns A. */
uint16_t RotateXZWide(uint8_t angle, int x8, uint16_t *x, uint16_t *y);
uint16_t RotateXZWideFrom08(uint8_t angle, int x8, uint16_t *x, uint16_t *y);

#endif
