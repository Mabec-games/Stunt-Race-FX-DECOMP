/*
 * gsu_programs_calls.h: the S-CPU routines that set up and run the Super
 * FX's drawing programs (gsu/programs.c) as C functions, for the C that
 * calls them. A uint8_t result named nothing else is the screen mode value
 * GsuRun leaves; a routine's other exit registers are its adapter's.
 */
#ifndef STUNTRACEFX_GSU_PROGRAMS_CALLS_H
#define STUNTRACEFX_GSU_PROGRAMS_CALLS_H

#include <stdint.h>

/* The view and the object vectors. */
uint16_t ViewSetup(uint8_t bank);
uint16_t ViewSetupTail(uint8_t bank);
uint16_t Gsu81E3(void);
uint16_t Gsu81E3Long(void);
uint16_t ViewMatrixUpdateAndSave(uint8_t bank);
uint8_t ViewUpdateAndGsuAEF8(uint8_t bank);
uint16_t ObjectGsu80BCThen81E3(uint8_t bank, uint16_t x);
uint16_t ObjectGsu81E3(uint8_t bank, uint16_t x);

/* The vector programs (gsu/programs_vector.c); x8: index registers 8-bit
   (Gsu8277's pushes and pull are a byte each then). */
uint16_t Gsu8277(int x8);
uint16_t VectorGsu81E3(uint8_t bank);
void ScaledOffset(uint8_t bank, uint8_t *ra, uint8_t *rx, uint8_t *p);
void ScaledOffsetPair(uint8_t bank, uint8_t *ra, uint8_t *rx, uint8_t *p);
uint16_t ViewFromAnglesGsu8277(uint8_t bank);
uint16_t ViewToGsuParams(uint8_t bank);

/* Piece lists and records: the result is the end marker's offset. */
uint16_t GsuPiecesFromRecord(uint8_t bank, uint16_t x);
uint16_t GsuPiecesListFAF2(uint8_t bank, uint16_t list);
uint16_t GsuPiecesListF6DF(uint8_t bank, uint16_t list);
uint16_t GsuPiecesFromBytes(uint8_t bank, uint16_t x);
uint16_t GsuPieceOffsetFromView(uint8_t bank);
uint16_t GsuPieceScaleFromView(uint8_t bank);

/* Glyph rows and words. */
uint8_t GsuDigitPair0E74(uint8_t bank);
uint8_t GsuF329Sequence(uint8_t bank, uint16_t y);
uint8_t GlyphRowsAt32(uint16_t input);
uint8_t GlyphRowsAt56(uint16_t input);
uint8_t HudSixGlyphs(void);
uint8_t GsuF15BFourA(void);
uint8_t GsuF15BFourB(void);
uint8_t GlyphAtY60(uint16_t glyph);
uint8_t GlyphRowY60From0(void);
uint8_t GlyphRowY60From1(void);
uint8_t GlyphRowY60From2(void);
uint8_t GlyphRowY60From3(void);
uint8_t GlyphRowY60From4(void);
uint8_t GlyphRowY60From5(void);
uint8_t GlyphWordAt1C28(void);
uint8_t GlyphGrid1A(void);
uint8_t GlyphGrid1D(void);

/* The big 24 x 40 sprites; with $0E5C set the rows' result is the new
   count BigSpritesCountDraw returns. */
uint8_t BigSpritesCountDraw(uint8_t bank);
uint8_t BigSpritesRow6(uint8_t bank);
uint8_t BigSpritesRow5(uint8_t bank);
uint8_t BigSpritesNine(uint8_t bank);
uint8_t BigSpritesRow6B(uint8_t bank);

/* The glyph sprites, the glyph list and the best times' display. */
void GlyphSpritesInit(uint8_t bank);
uint8_t GlyphSpriteDraw(uint8_t bank, uint16_t x);
uint8_t GlyphSpriteDrawLeft8(uint8_t bank, uint16_t x);
uint8_t GlyphSpritesDrawAll(uint8_t bank);
uint8_t GlyphSlideThree(uint8_t bank);
int GlyphSpritesSlideOut(uint8_t bank);         /* 1: both out, nothing drawn */
uint8_t GlyphSpritesSlideIn(uint8_t bank);
uint16_t GlyphListDraw(uint8_t bank);           /* the end offset */
uint16_t GlyphListMessageAndTime(uint8_t bank, uint16_t message);  /* the last entry */

/* The HUD's times, lap digits and marks. */
int HudHoldDraw(uint8_t bank);                  /* 1 when it drew */
void HudTimesDraw(uint8_t bank);
uint8_t HudTimesDrawEven(uint8_t bank);         /* the P it pulls */
void HudTimesPackedDraw(uint8_t bank);
void HudTimesPackedEven(uint8_t bank);
void TimeDigitsStore(uint8_t bank, uint8_t min, uint8_t sec, uint8_t hund);
uint8_t HudLeadWordDraw(void);
uint8_t HudLapDigitsDraw(uint8_t bank);
void HudBlankDraw(uint8_t bank);

#endif
