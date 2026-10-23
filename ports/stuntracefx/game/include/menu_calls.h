/*
 * menu_calls.h: the S-CPU's menu routines (game/menu) as C functions, for
 * the C that calls them.
 */
#ifndef STUNTRACEFX_MENU_CALLS_H
#define STUNTRACEFX_MENU_CALLS_H

#include <stddef.h>
#include <stdint.h>

/* The speed gauges (gauge.c). GaugeTableIndex and the gauge updates keep
   X through *x where the routine does. */
uint16_t MenuGaugeFromButtonB(void);
uint16_t MenuGaugeSlideToTarget(void);
uint16_t GaugeTableIndex(uint8_t bank, uint16_t value);
uint16_t GaugeFromSpeed(uint8_t bank, uint16_t *x);
uint16_t GaugeFromSpeedZero(uint8_t bank, uint16_t *x);
uint16_t SpeedGaugeUpdate(uint8_t bank, uint16_t a, uint16_t *x);
uint16_t SpeedGauge2Update(uint8_t bank, uint16_t a);

/* The menu loader's steps (loader.c): each returns the object it ran (X). */
uint16_t MenuLoadPictureCC78(uint8_t bank);
uint16_t MenuLoadPictureCE78(uint8_t bank);
uint16_t MenuLoadPictureGrid(uint8_t bank);
uint16_t MenuLoadPictureDD8C(uint8_t bank);
uint16_t MenuLoadGsu4000(uint8_t bank);
uint16_t MenuLoadGsu5000(uint8_t bank);
uint16_t MenuLoadGsu6000(uint8_t bank);
uint16_t MenuLoadPicturesTo7F(uint8_t bank);
uint16_t MenuLoadClearGsu(uint8_t bank);
uint16_t MenuLoadBlank0800(uint8_t bank);
uint16_t MenuLoadBlank1000(uint8_t bank);
uint16_t MenuLoadBlank1800(uint8_t bank);

/* The menu frame loop (frame.c). p is the P going in, where the routine
   reads or restores it; *x and *y are X and Y in and out; *p is P in and
   out. */
uint8_t MenuFramesTarget7(uint8_t bank);
uint16_t MenuRandomStep(uint8_t bank, uint8_t p);
uint16_t MenuDimOnL(uint8_t bank);
void MenuWaitFrame(uint8_t bank);

/* The menu objects (actors.c). */
int MenuActorFind(uint8_t bank, uint16_t kind, uint16_t *x, uint16_t *y, uint8_t *p);
void MenuActorFind8(uint8_t bank, uint8_t kind, uint16_t *x, uint16_t *y, uint8_t *p, uint8_t *a);
uint16_t MenuActorAddX(uint16_t rec, uint8_t *p);
uint16_t MenuActorAddY(uint16_t rec, uint8_t *p);
void MenuActorsKind2Lower(uint8_t bank, uint16_t *x, uint16_t *y, uint16_t *a, uint8_t *p);
uint16_t MenuDropAwayFinish(uint8_t bank, uint8_t *p);

#endif
