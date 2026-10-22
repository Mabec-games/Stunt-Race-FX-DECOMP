/*
 * object_calls.h: the S-CPU's object routines (game/object) as C
 * functions, for the C that calls them.
 */
#ifndef STUNTRACEFX_OBJECT_CALLS_H
#define STUNTRACEFX_OBJECT_CALLS_H

#include <stdint.h>

/* Object commands that go on at an address kept in the object's record
   (object_continue.c): each returns the long address to go on at (bank
   << 16 | address), or 0 when the command just returns; *a is the A it
   is entered or returns with. `obj` is the object's offset (X), `bank`
   the data bank. */
uint32_t ObjectContinue1A(uint8_t bank, uint16_t obj, uint16_t *a);
uint32_t ObjectContinue1D(uint8_t bank, uint16_t obj, uint16_t *a);
uint32_t ObjectStep39Continue(uint8_t bank, uint16_t obj, uint16_t *a);
uint32_t ObjectSet39To0Continue(uint8_t bank, uint16_t obj, uint16_t *a);
uint32_t ObjectSet39To1Continue(uint8_t bank, uint16_t obj, uint16_t *a);
uint32_t ObjectSet39To2Continue(uint8_t bank, uint16_t obj, uint16_t *a);
uint32_t ObjectSet39To3Continue(uint8_t bank, uint16_t obj, uint16_t *a);
uint32_t ObjectSet39To4Continue(uint8_t bank, uint16_t obj, uint16_t *a);
uint32_t ObjectSet39To5Continue(uint8_t bank, uint16_t obj, uint16_t *a);

/* Object fields (object_fields.c). The `p` ones keep the flags of the
   last arithmetic there, decimal mode included. */
uint16_t ObjectStubZero(void);
uint16_t ObjectStubZeroCopy(void);
uint16_t ObjectPlaceAtOffset(uint8_t bank, uint16_t from, uint16_t to, uint8_t *p);
uint8_t ObjectStepByte17(uint8_t bank, uint8_t step, uint16_t obj, uint8_t *p);
void ObjectStubNothing(void);
void ObjectStubSwapTwice(int x8);
void ObjectStopX(uint8_t bank, uint16_t obj);
void ObjectStopY(uint8_t bank, uint16_t obj);

/* The draw list's build (draw_list.c): A = 0 at the end; X is the
   trigger position there, Y the last object looked at (0 once a bucket has
   been walked, else what it came in as). */
typedef struct {
    uint16_t x, y;
} ObjectDrawListEnd;

ObjectDrawListEnd ObjectDrawListBuild(uint16_t y);
/* The arm after its RTS: object y's +$08 cleared, then the walk on from
   the next record x. */
ObjectDrawListEnd ObjectDrawListClear08(uint16_t y, uint16_t x);

#endif
