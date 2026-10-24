/*
 * backdrop.c: the race screen's backdrop (BG2), scrolled with the view.
 * Each player's half of the screen ($0D7F selects it, 0 or 1) has its own
 * BG2 scroll pair: horizontal at $0D92 + half * 2, vertical at $0D8E +
 * half * 2, and $0D96 + half * 2 for the raster code. The view's angle is
 * at $06CD and its heading at $06CF.
 */
#include "machine.h"
#include "video_calls.h"
#include "carried.h"
#include "wram_names.h"

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

    cyc(player ? 32 : 54);              /* player 1's load has a BRA after it */
    obj = rd16(W(obj_link) + obj);
    cyc(46);
    obj = rd16(W(obj_link) + obj);
    cyc(46);
    return (rd16(0x000000 + obj) & 0x4000) ? (cyc(172), 0x0008) : (cyc(178), 0x0000);
}

/* PlayerVehicleRaised on carried registers. */
void PlayerVehicleRaised_cr(Carried *cr)
{
    cr->a = PlayerVehicleRaised(cr->db, (uint8_t)cr->x);
    cr->x &= 0x00FF;
}

void abi_PlayerVehicleRaised(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PlayerVehicleRaised_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* The vertical scroll base for each half, a word per half, just before
   BackdropScrollFromView. */

/*
 * @routine 7E:E924 BackdropScrollFromView
 * WHAT IT DOES: Sets the backdrop scroll of the current half ($0D7F):
 * horizontal = the view heading $06CF / 4; with h = the view angle $06CD
 * + $18 (+ 8 when PlayerVehicleRaised), vertical = h + the half's base at
 * $7E:E920, and $0D96 = h * 2 + $EF99, at most $F257 (the cap is not
 * reached in the traces so far).
 * HOW: 16-bit arithmetic; the doubling's carry goes into the add of
 * $EF99. X is the half * 2; P is saved and restored.
 * Entry: X 8-bit, DB a bank where $0D8E is work RAM (bank); m16 is
 * whether M is 16-bit, which costs the first load a cycle. Exit: A = the
 * vertical scroll; X = the half * 2 (*x); P unchanged.
 */
uint16_t BackdropScrollFromView(uint8_t bank, uint16_t *x, int m16)
{
    uint16_t a, t;
    unsigned s;

    cyc(54);
    *x = (uint8_t)(rd8(W(gsu_buffer_select)) << 1);
    cyc(m16 ? 90 : 82);                 /* a 16-bit LDA reads a second byte */
    a = (uint16_t)(rd16(W(view_06cf)) >> 2);
    wr16(dbw(bank, (uint16_t)(W(bg2_scroll_x) + *x)), cyc_then(58, a));
    cyc(62);
    a = PlayerVehicleRaised(bank, (uint8_t)*x);
    cyc(46);
    a = (uint16_t)(a + rd16(W(wram_06CD)));
    a = (uint16_t)(a + 0x0018);
    s = (unsigned)(uint16_t)(a << 1) + 0xEF99 + (a >> 15);
    t = (uint16_t)s;
    if (t >= 0xF257)
        { cyc(18); t = 0xF257; }
    wr16(dbw(bank, (uint16_t)(W(gradient_src_a) + *x)), cyc_then(182, t));
    cyc(82);
    a = (uint16_t)(a + rd16(W(backdrop_y_base) + *x));
    wr16(dbw(bank, (uint16_t)(W(bg2_scroll_y) + *x)), cyc_then(30, a));
    cyc(70);
    return a;
}

/* BackdropScrollFromView on carried registers. */
void BackdropScrollFromView_cr(Carried *cr)
{
    cr->a = BackdropScrollFromView(cr->db, &cr->x, !(cr->p & P_M));
}

void abi_BackdropScrollFromView(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    BackdropScrollFromView_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}
