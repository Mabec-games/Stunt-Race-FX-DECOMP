/*
 * backdrop.c: the race screen's backdrop (BG2), scrolled with the view.
 * Each player's half of the screen ($0D7F selects it, 0 or 1) has its own
 * BG2 scroll pair: horizontal at $0D92 + half * 2, vertical at $0D8E +
 * half * 2, and $0D96 + half * 2 for the raster code. The view's angle is
 * at $06CD and its heading at $06CF.
 */
#include "machine.h"

static uint32_t dbw(uint8_t bank, uint16_t a)
{
    return (uint32_t)bank << 16 | a;
}

/*
 * @routine 7F:1278 PlayerVehicleRaised
 * WHAT IT DOES: Returns 8 when bit 14 is set in the word that player X's
 * vehicle record leads to, else 0: the vehicle object is $0346 for X = 0
 * and $034A otherwise; its link (+$4F in the object table at $7E:2000) is
 * followed twice, and the word read at that offset in bank $00. The value
 * 8 is not reached in the traces so far.
 * HOW: The compare of X is 8-bit; the rest 16-bit, X pushed and pulled
 * back. P is saved and restored.
 * Entry: any widths; X = the player (0 or not). Exit: A = 0 or 8; X's low
 * byte unchanged, high byte clear; P unchanged.
 */
uint16_t PlayerVehicleRaised(uint8_t bank, uint8_t player)
{
    uint16_t obj = rd16(cyc_then(152, dbw(bank, player ? (cyc(6), 0x034A) : 0x0346)));

    obj = rd16(0x7E204F + obj);
    cyc(46);
    obj = rd16(0x7E204F + obj);
    cyc(46);
    return (rd16(0x000000 + obj) & 0x4000) ? (cyc(172), 0x0008) : 0x0000;
}

void abi_PlayerVehicleRaised(void)
{
    cpu.a = PlayerVehicleRaised(cpu.db, (uint8_t)cpu.x);
    cpu.x &= 0x00FF;
}

/* The vertical scroll base for each half, a word per half, just before
   BackdropScrollFromView. */
#define BACKDROP_Y_BASE 0x7EE920u

/*
 * @routine 7E:E924 BackdropScrollFromView
 * WHAT IT DOES: Sets the backdrop scroll of the current half ($0D7F):
 * horizontal = the view heading $06CF / 4; with h = the view angle $06CD
 * + $18 (+ 8 when PlayerVehicleRaised), vertical = h + the half's base at
 * $7E:E920, and $0D96 = h * 2 + $EF99, at most $F257 (the cap is not
 * reached in the traces so far).
 * HOW: 16-bit arithmetic; the doubling's carry goes into the add of
 * $EF99. X is the half * 2; P is saved and restored.
 * Entry: X 8-bit, DB a bank where $0D8E is work RAM. Exit: A = the
 * vertical scroll; X = the half * 2; P unchanged.
 */
uint16_t BackdropScrollFromView(uint8_t bank, uint16_t *x)
{
    uint16_t a, t;
    unsigned s;

    cyc(54);
    *x = (uint8_t)(rd8(0x000D7F) << 1);
    a = (uint16_t)(rd16(0x0006CF) >> 2);
    wr16(dbw(bank, (uint16_t)(0x0D92 + *x)), cyc_then(58, a));
    cyc(62);
    a = PlayerVehicleRaised(bank, (uint8_t)*x);
    cyc(224);
    a = (uint16_t)(a + rd16(0x0006CD));
    a = (uint16_t)(a + 0x0018);
    s = (unsigned)(uint16_t)(a << 1) + 0xEF99 + (a >> 15);
    t = (uint16_t)s;
    if (t >= 0xF257)
        { cyc(18); t = 0xF257; }
    wr16(dbw(bank, (uint16_t)(0x0D96 + *x)), cyc_then(182, t));
    cyc(82);
    a = (uint16_t)(a + rd16(BACKDROP_Y_BASE + *x));
    wr16(dbw(bank, (uint16_t)(0x0D8E + *x)), cyc_then(30, a));
    return a;
}

void abi_BackdropScrollFromView(void)
{
    cpu.a = BackdropScrollFromView(cpu.db, &cpu.x);
}
