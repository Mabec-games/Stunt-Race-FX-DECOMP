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

/* P as an approach leaves it from p: v its result (wide: 16-bit), moved
   whether it moved. */
uint8_t ApproachExitP(uint8_t p, uint16_t v, int wide, int moved);

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

/* An object's velocity from its speed and heading (heading.c): `bank` the
   caller's data bank, x the object, y the Y register; A's low byte back. */
uint8_t HeadingFlat(uint8_t bank, uint16_t x, uint16_t y);
uint8_t HeadingFlatMirrored(uint8_t bank, uint16_t x, uint16_t y);
uint8_t HeadingPitched(uint8_t bank, uint16_t x, uint16_t y);
uint8_t HeadingPitchedVia1F7C(uint8_t bank, uint16_t x, uint16_t y);
uint8_t HeadingPitchedFrom1F7C(uint16_t x, uint16_t y);
uint8_t HeadingFlatQuarterTurn(uint8_t bank, uint8_t speed, uint16_t x, uint16_t y);
uint8_t HeadingFlatSpeedA(uint8_t bank, uint8_t speed, uint16_t x, uint16_t y);
/* Moving an object by the vector at $02, $08, $94: the new +$10. */
uint16_t ObjectMoveByVectorTimes4(uint8_t bank, uint16_t x);
uint16_t ObjectMoveByVectorTimes2(uint8_t bank, uint16_t x);
uint16_t ObjectMoveByVector(uint8_t bank, uint16_t x);
/* The object's velocity from the vector: the last word (A). */
uint16_t ObjectVelocityFromHeading(uint8_t bank, uint16_t x, uint16_t y);
uint16_t ObjectVelocityFromHeadingTimes2(uint8_t bank, uint16_t x, uint16_t y);
uint16_t ObjectVelocityFromHeadingTimes4(uint8_t bank, uint16_t x, uint16_t y);
uint16_t ObjectVelocityFromHeadingTimes8(uint8_t bank, uint16_t x, uint16_t y);
uint16_t ObjectVelocityLevelFromDp(uint8_t bank, uint16_t x, uint16_t y);

/* Fixed scalings of signed 16-bit values (scale.c). Those with a flags
   pointer update N, Z, C, V in *p as the routine leaves them. */
uint16_t ScaleThreeQuarters(uint16_t v);
uint16_t ScaleQuarter(uint16_t v, uint8_t *p);
uint16_t ScaleNineSixteenths(uint16_t v, uint8_t *p);
uint16_t ScaleFiveEighths(uint16_t v, uint8_t *p);
uint16_t ScaleSevenEighths(uint16_t v, uint8_t *p);
uint16_t ScaleFifteenSixteenths(uint8_t bank, uint16_t v, uint8_t *p);
uint16_t ScaleFiveThirtySeconds(uint8_t bank, uint16_t v, uint8_t *p);
uint16_t Mode7ScaleProduct(uint16_t value, int positive);
uint16_t ScaleDp02Dp94(uint8_t *p);
uint16_t ScaleObjectXZ(uint16_t obj, uint8_t *p);
uint16_t ScaleDp02Dp94Long(uint8_t *p);
/* The blend factor stepped, the four words at $7F:3432 set by it: A. */
uint16_t BlendStepUp(void);
uint16_t BlendStepDown(void);

#endif
