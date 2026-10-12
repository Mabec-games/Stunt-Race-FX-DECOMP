/*
 * fx_calls.h: the Super FX routines that other Super FX C calls.
 */
#ifndef STUNTRACEFX_FX_CALLS_H
#define STUNTRACEFX_FX_CALLS_H

#include <stdint.h>

typedef struct {
    uint16_t quot, rem;
} FxQuotient;

typedef struct {
    uint16_t root, rem, trial, diff;
} FxRoot;

/* The register file, for routines whose C leaves most of it. */
typedef struct {
    uint16_t r[16];
} FxRegs;

FxQuotient FxDivide(uint16_t hi, uint16_t lo, uint16_t divisor);
FxQuotient FxDivideHalvedIfLarge(uint16_t hi, uint16_t lo, uint16_t *divisor);
FxRoot FxSquareRoot(uint16_t hi, uint16_t lo);
FxRegs FxNormalize(FxRegs g);
FxRegs FxAngleOf(FxRegs g);
FxRegs FxPoolTake(FxRegs g);
FxRegs FxListSweepRegs(FxRegs g);
FxRegs FxContactNote(FxRegs g);
FxRegs FxRecordsApart(FxRegs g);
FxRegs FxBearingTo(FxRegs g);
FxRegs FxCourseNearest(FxRegs g);
FxRegs FxCourseNextEntry(FxRegs g);

#endif
