/*
 * fx_calls.h: the Super FX routines that other Super FX C calls.
 */
#ifndef STUNTRACEFX_FX_CALLS_H
#define STUNTRACEFX_FX_CALLS_H

#include <stdint.h>
#include "gsu.h"

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

FxQuotient FxDivide(uint16_t hi, uint16_t lo, uint16_t divisor, int copy);
FxQuotient FxDivideHalvedIfLarge(uint16_t hi, uint16_t lo, uint16_t *divisor, int copy);
FxRoot FxSquareRoot(uint16_t hi, uint16_t lo, int copy);
FxRegs FxNormalize(FxRegs g, int copy);
FxRegs FxAngleOf(FxRegs g, int copy);
FxRegs FxTurnRegs(FxRegs g, int copy);
FxRegs FxSpreadCars(FxRegs g);
FxRegs FxPoolTake(FxRegs g, int copy);
FxRegs FxListSweepRegs(FxRegs g);
FxRegs FxContactNote(FxRegs g, int copy);
FxRegs FxRecordsApart(FxRegs g, int copy);
FxRegs FxBearingTo(FxRegs g, int copy);
FxRegs FxCourseNearest(FxRegs g, int copy);
FxRegs FxCourseNextEntry(FxRegs g, int copy);

void abi_FxMatrixFromAngles(void);
void abi_FxMatrixTranspose04(void);

#endif
