/*
 * screen.c: the Super FX's side of its screen buffer. The S-CPU sets the
 * buffer's start ($2C00) at $70:021A when it flips buffers (gsu/screen.c);
 * the buffer is $6800 bytes, 832 characters of 4 bits per pixel.
 */
#include "gsu.h"

#define FX_SCREEN_START 0x021Au  /* the word holding the buffer's start */
#define FX_SCREEN_WORDS 0x3400u

/*
 * @routine 01:AEB9 FxScreenClear
 * WHAT IT DOES: Clears the screen buffer whose start is the word at $021A
 * to colour 0.
 * HOW: CACHE, then a LOOP of $3400 passes, each storing a zero word at R1
 * and stepping R1 by 2 (one INC before LOOP, one in its delay slot).
 * Entry: R11 = return address. Exit: R0 = 0; R1 = the buffer's end; R12 = 0;
 * R13 = $AEC4 (the loop); the last RAM address the last word stored; CY
 * set and OV clear (SUB R0); S and Z from R1 (the INC in LOOP's delay slot).
 * Returns the address just past the buffer.
 */
uint16_t FxScreenClear(void)
{
    uint16_t at = gsu_ldw(FX_SCREEN_START);
    uint16_t n;

    gsu_code(0x01AEBF, 4); gsu_cache(0xAEC2); gsu_code(0x01AEC3, 1);
    for (n = 0; n < FX_SCREEN_WORDS; n++) {
        gsu_code(0x01AEC4, 2);
        gsu_stw(at, 0x0000);
        at = (uint16_t)(at + 2);
        gsu_code(0x01AEC6, 2);
    }
    return at;
}

void abi_FxScreenClear(void)
{
    gsu.r[1] = FxScreenClear();
    gsu.r[0] = 0;
    gsu.r[12] = 0;
    gsu.r[13] = 0xAEC4;
    gsu.cy = 1;
    gsu.ov = 0;
    gsu_sz(gsu.r[1]);
}
