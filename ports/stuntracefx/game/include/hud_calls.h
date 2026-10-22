/*
 * hud_calls.h: the HUD routines (game/hud) as C functions, for the C that
 * calls them.
 */
#ifndef STUNTRACEFX_HUD_CALLS_H
#define STUNTRACEFX_HUD_CALLS_H

#include <stdint.h>

/* The map markers (map.c): each returns X, the list position of the
   terminating 0, and sets *y (Y) past the last marker. MapMarkersPanelB
   returns 0 (drawing nothing) when neither $0D55 nor $0D62 is set. */
uint16_t MapMarkerSprites(uint8_t bank, uint16_t *y);
uint16_t MapMarkersPanelA(uint8_t bank, uint16_t *y);
int MapMarkersPanelB(uint8_t bank, uint16_t *x, uint16_t *y);

/* The panels' gauges (gauges.c): *a, *x, *y are A, X and Y as the last
   SpriteBar6WithMarker leaves them. HudGaugesPanelB returns 0, drawing
   nothing, when neither $0D55 nor $0D62 is set. Gauge6APlace/B return A's
   low byte on exit. */
void HudGaugesPanelA(uint8_t bank, uint16_t *a, uint16_t *x, uint16_t *y);
int HudGaugesPanelB(uint8_t bank, uint16_t *a, uint16_t *x, uint16_t *y);
uint8_t Gauge6BPlace(uint8_t bank);
uint8_t Gauge6APlace(uint8_t bank);

/* Times (lap_time.c). *p is the routine's P in and out (the decimal flag is
   the caller's; N, Z, C, V as the routine leaves them). */
uint16_t BinaryToBcd(uint16_t v, uint8_t *p);
void TimeUnpackBcd(uint16_t packed, uint16_t *min, uint16_t *sec, uint16_t *hund, uint8_t *p);
void TimeUnpackBcdLong(uint16_t packed, uint16_t *min, uint16_t *sec, uint16_t *hund, uint8_t *p);
uint8_t TimeAddToTotal(uint8_t bank, uint16_t x, uint8_t *p);
uint8_t TimeAddToNext(uint8_t bank, uint16_t x, uint8_t *p);
uint8_t TimeTotalsSum(uint8_t bank, uint8_t p0);
uint8_t TimeRunningTotals(uint8_t bank, uint8_t p0);
uint8_t RecordSlotCopy0(uint8_t bank, uint8_t *p);
uint8_t RecordSlotCopy3(uint8_t bank, uint8_t *p);
uint8_t RecordShiftFrom0(uint8_t bank, uint8_t *p);
uint8_t RecordShiftFrom1(uint8_t bank, uint8_t *p);
uint8_t RecordNameStore0(uint8_t bank, uint8_t *p);
uint8_t RecordNameStore3(uint8_t bank, uint8_t *p);
uint8_t RecordNameStore6(uint8_t bank, uint8_t *p);
int TimeBelowRecord(uint8_t bank, uint16_t x, uint8_t *p);
uint16_t BcdToBinary(uint16_t v, uint8_t *p);
uint16_t TimePackBcd(uint16_t hund, uint16_t sec, uint16_t min, uint8_t *p);
uint16_t RecordTimeInsert(uint8_t bank, uint16_t a, uint16_t *x, uint16_t *y, uint8_t p0);

/* The HUD panels' slides (hud/slide.c): the result is A's low byte as
   each ends; d is the direct page register. */
uint8_t HudSlideOut(uint8_t bank);
uint8_t HudPanelASlide(uint8_t bank, uint16_t d);
uint8_t HudPanelBSlide(uint8_t bank, uint16_t d);
void HudPanelsStep(uint8_t bank, uint16_t d);

#endif
