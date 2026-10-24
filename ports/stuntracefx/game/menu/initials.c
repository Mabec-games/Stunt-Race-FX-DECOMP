/*
 * initials.c: the menu screen where player 1 enters three letters, 0-$19
 * each, at $18EF, $18F1 and $18F3; the position being changed is $18ED
 * (0-2). Holding up or down repeats after 13 frames, counted in $190B (up)
 * and $190D (down) from the held buttons at $0309.
 */
#include "machine.h"
#include "cpu_ops.h"
#include "carried.h"
#include "wram_names.h"

uint16_t MenuChoicePressesP1(uint8_t bank, uint16_t *x, uint16_t *y, uint8_t *p);

void SoundPort0QueuePush(uint8_t command);
int MenuActorFind(uint8_t bank, uint16_t kind, uint16_t *x, uint16_t *y, uint8_t *p);
uint16_t MenuRowPalette5(uint8_t bank, uint16_t *x, uint16_t *y, uint8_t *p);
uint16_t MenuRowBlinkSettings(uint8_t bank, uint16_t *x, uint16_t *y, uint8_t *p);


static uint32_t dbw(uint8_t bank, uint16_t a)
{
    return (uint32_t)bank << 16 | a;
}

/* Compare as CMP does: C when a >= b; N, Z from a - b. */
static uint8_t cmp16(uint8_t p, uint16_t a, uint16_t b)
{
    return (uint8_t)((nz16(p, (uint16_t)(a - b)) & ~P_C) | (a >= b ? (cyc(18), P_C) : 0));
}

/* SoundPort0QueuePush: N, Z come back from the data bank. */
static void sound0(uint8_t bank, uint8_t command, uint8_t *p)
{
    cyc(100);
    SoundPort0QueuePush(command);
    *p = nz8(*p, bank);
}

/* A hold counter: one more frame held, up to 12; returns 1 when the hold
   repeats (the count would reach 13), else stores the count. Not held: the
   count is cleared. Sets C and N, Z as the code does. */
static int hold_repeats(uint32_t held_bit_set, uint32_t count, uint8_t *p, uint16_t *a)
{
    uint16_t v;

    if (!held_bit_set) {
        *a = 0x0000;
        *p = nz16(*p, *a);
        cyc(102);
        wr16(count, *a);
        cyc(24);
        return 0;
    }
    cyc(72);
    v = (uint16_t)(rd16(count) + 1);
    *p = cmp16(*p, v, 0x000D);
    *a = v;
    if (*p & P_C)
        return 1;
    cyc(86);
    wr16(count, v);
    cyc(46);
    return 0;
}

/*
 * @routine 0B:C385 MenuInitialsStep
 * WHAT IT DOES: Applies one press of player 1 ($118F) to the initials:
 * left and right move the position $18ED (0-2), with sound $46 and the
 * position marker's palette reset (MenuRowPalette5); up and down step the
 * letter at the position through 0-$19, wrapping, with sound $45, on a
 * press or when the button has been held 13 frames. Left is not reached in
 * the traces so far, nor a letter wrapping up.
 * HOW: The letter's offset (position * 2) is kept in X; up is handled
 * before down, and a handled up skips down.
 * Entry: M 16-bit, X 16-bit, D = $0000. Exit: A = the last value loaded or
 * stored; X = the position * 2; Y unchanged, or from the search; N, Z, C
 * from the last test or compare.
 */
uint16_t MenuInitialsStep(uint8_t bank, uint16_t *x, uint16_t *y, uint8_t *p)
{
    uint16_t a, v;
    int rep;

    a = rd16(cyc_then(24, (0x7E0000 | W(wram_118F)))) & 0x0200;
    *p = nz16(*p, a);
    if (a) {
        cyc(72);
        a = rd16(W(wram_18ED));
        *p = nz16(*p, a);
        if (a) {
            cyc(16);
            sound0(bank, 0x46, p);
            MenuRowPalette5(bank, x, y, p);
            cyc(32);
            a = (uint16_t)(rd16(W(wram_18ED)) - 1);
            *p = nz16(*p, a);
            cyc(46);
            wr16(W(wram_18ED), a);
        }
    } else { cyc(46); }
    a = rd16(cyc_then(24, (0x7E0000 | W(wram_118F)))) & 0x0100;
    *p = nz16(*p, a);
    if (a) {
        cyc(72);
        a = rd16(W(wram_18ED));
        *p = cmp16(*p, a, 0x0002);
        if (a != 0x0002) {
            cyc(40);
            sound0(bank, 0x46, p);
            MenuRowPalette5(bank, x, y, p);
            cyc(32);
            a = (uint16_t)(rd16(W(wram_18ED)) + 1);
            *p = nz16(*p, a);
            cyc(46);
            wr16(W(wram_18ED), a);
        }
    } else { cyc(46); }
    cyc(32);
    a = rd16(W(wram_18ED));
    *p = (uint8_t)((*p & ~P_C) | (a & 0x8000 ? P_C : 0));
    *x = (uint16_t)(a << 1);
    *p = nz16(*p, *x);

    cyc(60);
    rep = hold_repeats(rd16(W(joy1_held)) & 0x0800, W(wram_190B), p, &a);
    if (!rep) {
        a = rd16((0x7E0000 | W(wram_118F))) & 0x0800;
        *p = nz16(*p, a);
    }
    if (rep || a) {
        cyc(96);
        wr16(W(wram_190B), 0x0000);
        sound0(bank, 0x45, p);
        cyc(54);
        v = (uint16_t)(rd16(W(wram_18EF) + *x) + 1);
        *p = cmp16(*p, v, 0x001A);
        if (v == 0x001A) {
            v = 0x0000;
            *p = nz16(*p, v);
        }
        cyc(92);
        wr16(W(wram_18EF) + *x, v);
        cyc(64);
        return v;
    }

    cyc(78);
    rep = hold_repeats(rd16(W(joy1_held)) & 0x0400, W(wram_190D), p, &a);
    if (!rep) {
        a = rd16((0x7E0000 | W(wram_118F))) & 0x0400;
        *p = nz16(*p, a);
        if (!a)
            { cyc(88); return a; }
    }
    cyc(96);
    wr16(W(wram_190D), 0x0000);
    sound0(bank, 0x45, p);
    cyc(54);
    v = (uint16_t)(rd16(W(wram_18EF) + *x) - 1);
    *p = cmp16(*p, v, 0xFFFF);
    if (v == 0xFFFF) {
        v = 0x0019;
        *p = nz16(*p, v);
    }
    cyc(92);
    wr16(W(wram_18EF) + *x, v);
    cyc(42);
    return v;
}

void MenuInitialsFrame(Carried *cr);
void MenuInitialsPresses_cr(Carried *cr);
void MenuInitialsStep_cr(Carried *cr);
void MenuSettingsFrame(Carried *cr);
static uint32_t dpx(Carried *cr, uint8_t off);
static void kind_anim(Carried *cr, uint16_t k, uint16_t n);
static void kinds67_anim(Carried *cr);
static void set10(Carried *cr, uint16_t k, uint16_t v);
static void sound_port0(Carried *cr, uint8_t n);
static void w_MenuChoicePressesP1(Carried *cr);
void FrameByteWritePush_cr(Carried *cr);
void MenuActorFind_cr(Carried *cr);
void MenuActorFree_cr(Carried *cr);
void MenuActorSpawn_cr(Carried *cr);
void MenuActorsSet10_cr(Carried *cr);
void MenuFixedColourDown_cr(Carried *cr);
void RaceFrameStep(Carried *cr);
void SoundPort0QueuePushRaw_cr(Carried *cr);
void SoundPort0QueuePush_cr(Carried *cr);

void MenuInitialsStep_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = MenuInitialsStep(cr->db, &cr->x, &cr->y, &p);
    set_p(cr, p);
}

void abi_MenuInitialsStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuInitialsStep_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:C34F MenuInitialsPresses
 * WHAT IT DOES: Works through player 1's presses queued this frame, last
 * first, as MenuChoicePressesP1 does, applying each to the initials
 * (MenuInitialsStep) and then emptying the list; then blinks the marker
 * and shows the letters on the kind 9 object (MenuRowBlinkSettings).
 * HOW: The list offset is pushed around each step and pulled back into A;
 * an empty list skips straight to the display.
 * Entry: M 16-bit, X 16-bit, D = $0000. Exit: A and C as
 * MenuRowBlinkSettings; X = $11F9; Y = $0A; N, Z from X.
 */
uint16_t MenuInitialsPresses(uint8_t bank, uint16_t *x, uint16_t *y, uint8_t *p)
{
    uint16_t i, v, a;

    wr16((0x7E0000 | W(menu_joy1_pressed)), cyc_then(24, 0x0000));
    cyc(32);
    i = (uint16_t)(rd16(W(word_list_a_size)) - 2);
    if (!(i & 0x8000)) {
        do {
            *x = i;
            cyc(90);
            v = rd16(W(word_list_a) + i);
            wr16((0x7E0000 | W(wram_118F)), cyc_then(24, v));
            v |= rd16(cyc_then(24, (0x7E0000 | W(menu_joy1_pressed))));
            wr16((0x7E0000 | W(menu_joy1_pressed)), cyc_then(24, v));
            cyc(76);
            MenuInitialsStep(bank, x, y, p);
            i = (uint16_t)(i - 2);
        } while (cyc_loop(42, !(i & 0x8000)));
        cyc(136);
        wr16(W(word_list_a_size), 0x0000);
    }
    cyc(70);
    MenuActorFind(bank, 0x0009, x, y, p);
    a = MenuRowBlinkSettings(bank, x, y, p);
    *x = rd16(cyc_then(24, (0x7E0000 | W(actor_current))));
    *p = nz16(*p, *x);
    cyc(42);
    return a;
}

void MenuInitialsPresses_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = MenuInitialsPresses(cr->db, &cr->x, &cr->y, &p);
    set_p(cr, p);
}

void abi_MenuInitialsPresses(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuInitialsPresses_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void abi_MenuFixedColourDown(void);
void abi_RaceFrameStep(void);
void abi_MenuActorsSet10(void);
void abi_MenuActorFind(void);
void abi_MenuActorFree(void);
void abi_SoundPort0QueuePush(void);

/* Direct page word off + X. */
static uint32_t dpx(Carried *cr, uint8_t off)
{
    return (uint16_t)(cr->d + off + cr->x);
}

/* Sound n on port 0 with 8-bit A (SEP, LDA, JSL, REP). */
static void sound_port0(Carried *cr, uint8_t n)
{
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cr->a = (uint16_t)((cr->a & 0xFF00) | n);
    set_p(cr, nz8(cr->p, n));
    (cyc(100), SoundPort0QueuePush_cr(cr));
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
}

/* MenuActorsSet10 for kind k with value v. */
static void set10(Carried *cr, uint16_t k, uint16_t v)
{
    cr->a = k;
    cr->y = v;
    set_p(cr, nz16(cr->p, v));
    (cyc(94), MenuActorsSet10_cr(cr));
}

/* Kind k's actor (MenuActorFind) to animation n, restarting its position
   and timer when it had another. */
static void kind_anim(Carried *cr, uint16_t k, uint16_t n)
{
    cr->a = k;
    set_p(cr, nz16(cr->p, k));
    (cyc(70), MenuActorFind_cr(cr));
    cr->a = rd16(cyc_then(22, dpx(cr, 0x14)));
    r_cmp16(cr, n);
    if (cr->a == n)
        { cyc(46); return; } else { cyc(4); }
    cr->a = n;
    wr16(dpx(cr, 0x14), cyc_then(82, n));
    wr16(dpx(cr, 0x16), cyc_then(22, 0x0000));
    wr16(dpx(cr, 0x18), cyc_then(22, 0x0000));
}

/*
 * @routine 0B:C225 MenuInitialsFrame
 * WHAT IT DOES: The initials entry screen's frame: dims the fixed colour
 * (MenuFixedColourDown), shows a race frame (RaceFrameStep, A = 0), sets
 * $18FD, applies player 1's presses (MenuInitialsPresses), then takes the
 * buttons pressed ($118B, cleared): start or B ($9000) ends the menu
 * ($11EB = 1, sound $46, the current record freed); Y ($4000) goes back:
 * kind 5's +$10 = 0, kinds 6 and 7 to animations $AE and $B8, kinds 8 and
 * 9 +$10 = $FFFF, this record's handler $BA4E with its slide restarted,
 * kind 4 to animation $8A, $1191 cleared and sound $47; select ($2000)
 * with selection $18C3 at 0 confirms: sound $46, kinds 4, 8 and 9 +$10 =
 * $FFFF, kinds 10-14 +$10 = 0, handler $BFB8, $1191 cleared.
 * HOW: Y keeps the buttons; record X is $11F9 as MenuInitialsPresses
 * leaves it. Kind 4's animation change is not reached in the traces so
 * far.
 * Entry: M 16-bit, X 16-bit, D = $0000. Exit: M 16-bit; A, X, Y and flags
 * as the path's last step left them.
 */
void MenuInitialsFrame(Carried *cr)
{
    cyc(46);
    uint32_t b = (uint32_t)cr->db << 16;
    uint16_t pad;
    unsigned k;

    MenuFixedColourDown_cr(cr);
    cr->a = 0x0000;
    set_p(cr, nz16(cr->p, 0));
    (cyc(8), RaceFrameStep(cr));
    cr->a = 0x0001;
    cyc(56);
    wr16(W(wram_18FD), 0x0001);
    (cyc(46), MenuInitialsPresses_cr(cr));
    cyc(24);
    pad = rd16(0x7E0000 | W(menu_joy1_pressed));
    cr->y = pad;
    cyc(24);
    wr16(0x7E0000 | W(menu_joy1_pressed), 0x0000);
    cr->a = pad;
    r_and16(cr, 0x9000);
    if (!(cr->p & P_Z)) {
        cr->a = 0x0001;
        cyc(106);
        wr16(0x7E0000 | W(menu_exit), 0x0001);
        sound_port0(cr, 0x46);
        cyc(46);
        cr->y = rd16(0x7E0000 | W(actor_current_slot));
        set_p(cr, nz16(cr->p, cr->y));
        (cyc(46), MenuActorFree_cr(cr));
        cyc(42);
        return;
    }
    cr->a = pad;
    r_and16(cr, 0x4000);
    if (!(cr->p & P_Z)) {
        cyc(158);
        cr->x = rd16(0x7E0000 | W(actor_current));
        set10(cr, 0x0005, 0x0000);
        {
            cyc(30);
            uint16_t x = cr->x;                  /* PHX */

            kind_anim(cr, 0x0006, 0x00AE);
            kind_anim(cr, 0x0007, 0x00B8);
            cr->x = x;                           /* PLX */
            set_p(cr, nz16(cr->p, x));
        }
        cyc(36);
        set10(cr, 0x0008, 0xFFFF);
        set10(cr, 0x0009, 0xFFFF);
        cr->a = 0xBA4E;
        wr16(dpx(cr, 0x08), cyc_then(46, 0xBA4E));
        wr16(dpx(cr, 0x0A), cyc_then(22, 0x0000));
        kind_anim(cr, 0x0004, 0x008A);
        cr->a = 0x0000;
        cyc(56);
        wr16(W(word_list_a_size), 0x0000);
        sound_port0(cr, 0x47);
        cyc(64);
        return;
    }
    cr->a = pad;
    r_and16(cr, 0x2000);
    if (cr->p & P_Z)
        { cyc(204); return; }
    cyc(200);
    cr->a = rd16(W(wram_18C3));
    r_cmp16(cr, 0x0000);
    if (cr->a != 0)
        { cyc(88); return; } else { cyc(40); }
    sound_port0(cr, 0x46);
    cyc(22);
    set10(cr, 0x0004, 0xFFFF);
    set10(cr, 0x0008, 0xFFFF);
    set10(cr, 0x0009, 0xFFFF);
    for (k = 0x0A; k <= 0x0E; k++)
        set10(cr, (uint16_t)k, 0x0000);
    cr->a = 0xBFB8;
    wr16(dpx(cr, 0x08), cyc_then(46, 0xBFB8));
    cr->a = 0x0000;
    cyc(56);
    wr16(W(word_list_a_size), 0x0000);
    set_p(cr, nz16(cr->p, 0));
    cyc(42);
}

void abi_MenuInitialsFrame(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuInitialsFrame(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void abi_SoundPort0QueuePushRaw(void);
void abi_MenuChoicePressesP1(void);
void abi_MenuActorSpawn(void);
void abi_FrameByteWritePush(void);

/* Kinds 6 and 7 to animations $C2 and $C8, X kept (PHX, PLX). */
static void kinds67_anim(Carried *cr)
{
    uint16_t x = cr->x;

    cyc(30);
    kind_anim(cr, 0x0006, 0x00C2);
    kind_anim(cr, 0x0007, 0x00C8);
    cr->x = x;
}

/*
 * @routine 0B:BA4E MenuSettingsFrame
 * WHAT IT DOES: The settings screen's frame for actor record X. While its
 * count +$0A runs it is counted down (sound $3F, raw, at $13, $0E, $09 and
 * $04; at 0 kind 4 to animation $8A). Then the fixed colour is dimmed
 * twice (MenuFixedColourDown), a race frame shown (RaceFrameStep, A = 0),
 * $18FD = 1, player 1's presses applied (MenuChoicePressesP1) and the
 * buttons pressed ($118B, cleared) taken: Y ($4000) goes back to the
 * initials (handler $C872, count $80, sound $47, an actor from $027E,
 * kinds 4 and 5 +$10 = $FFFF, kinds 6 and 7 to animations $C2 and $C8);
 * B or Start ($9000), unless $18F9 is set, takes the choice $18C3: 2 ends
 * (kind 4 to animation $90, kinds 4 and 5 +$10 = $FFFF, sound $46, handler
 * $BC9A, $1905 = 1, $1907 = $28, colour 0 = $464A, $1193 = 0, $E0 queued
 * for HDMAEN), others go on (sound $46, $190B = $190D = 0, kind 5 +$10 =
 * $FFFF, kinds 6 and 7 as above, kinds 8 and 9 +$10 = 0, handler $C225,
 * $1191 = 0); Select ($2000) with choice 0 sound $46, kinds 4 and 5 +$10 =
 * $FFFF, kinds 6 and 7 as above, kinds 10-14 +$10 = 0, handler $BEEF.
 * HOW: Y keeps the buttons; X is $11F9 again after the calls. The count
 * is not reached in the traces so far.
 * Entry: M 16-bit, X 16-bit, D = $0000; X = the record. Exit: M 16-bit; A,
 * X, Y and flags as the path's last step left them.
 */
void MenuSettingsFrame(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;
    uint16_t pad, v;
    unsigned k;

    cr->a = rd16(cyc_then(22, dpx(cr, 0x0A)));
    set_p(cr, nz16(cr->p, cr->a));
    if (cr->a) {
        cr->a = (uint16_t)(cr->a - 1);
        set_p(cr, nz16(cr->p, cr->a));
        wr16(dpx(cr, 0x0A), cyc_then(52, cr->a));
        v = cr->a;
        r_cmp16(cr, 0x0013);
        if (v == 0x0013 || (cyc(62), v == 0x000E) || (cyc(40), v == 0x0009) || (cyc(34), v == 0x0004)) {
            cyc(82);
            if (v != 0x0013)
                { cyc(4); r_cmp16(cr, v); } else { cyc(26); }
            set_p(cr, (uint8_t)(cr->p | (P_M)));
            cr->a = (uint16_t)((cr->a & 0xFF00) | 0x3F);
            set_p(cr, nz8(cr->p, 0x3F));
            SoundPort0QueuePushRaw_cr(cr);
            set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
        } else
            r_cmp16(cr, 0x0004);
        cr->a = rd16(cyc_then(52, dpx(cr, 0x0A)));
        set_p(cr, nz16(cr->p, cr->a));
        if (!cr->a)
            { cyc(16); kind_anim(cr, 0x0004, 0x008A); } else { cyc(22); }
    } else { cyc(22); }
    cyc(24);
    cr->x = rd16(0x7E0000 | W(actor_current));
    (cyc(46), MenuFixedColourDown_cr(cr));
    (cyc(46), MenuFixedColourDown_cr(cr));
    cr->a = 0x0000;
    set_p(cr, nz16(cr->p, 0));
    (cyc(8), RaceFrameStep(cr));
    cr->a = 0x0001;
    set_p(cr, nz16(cr->p, 1));
    cyc(56);
    wr16(W(wram_18FD), 0x0001);
    cyc(24);
    cr->x = rd16(0x7E0000 | W(actor_current));
    (cyc(46), w_MenuChoicePressesP1(cr));
    cyc(24);
    cr->x = rd16(0x7E0000 | W(actor_current));
    cyc(24);
    pad = rd16(0x7E0000 | W(menu_joy1_pressed));
    cr->y = pad;
    cyc(24);
    wr16(0x7E0000 | W(menu_joy1_pressed), 0x0000);
    cr->a = pad;
    r_and16(cr, 0x4000);
    if (cr->a) {
        cyc(106);
        cr->x = rd16(0x7E0000 | W(actor_current));
        cr->a = 0xC872;
        wr16(dpx(cr, 0x08), cyc_then(46, 0xC872));
        cr->a = 0x0080;
        wr16(dpx(cr, 0x0A), cyc_then(46, 0x0080));
        sound_port0(cr, 0x47);
        cr->x = 0x027E;
        set_p(cr, nz16(cr->p, 0x027E));
        (cyc(92), MenuActorSpawn_cr(cr));
        cyc(0);
        set10(cr, 0x0004, 0xFFFF);
        set10(cr, 0x0005, 0xFFFF);
        kinds67_anim(cr);
        cyc(60);
        cr->x = rd16(0x7E0000 | W(actor_current));
        set_p(cr, nz16(cr->p, cr->x));
        cyc(42);
        return;
    }
    cyc(90);
    r_lda16(cr, W(wram_18F9));
    if (!cr->a) {
        cyc(52);
        cr->a = pad;
        r_and16(cr, 0x9000);
        if (cr->a) {
            cyc(78);
            r_lda16(cr, W(wram_18C3));
            r_cmp16(cr, 0x0002);
            if (cr->a == 0x0002) {
                cyc(46);
                kind_anim(cr, 0x0004, 0x0090);
                set10(cr, 0x0004, 0xFFFF);
                set10(cr, 0x0005, 0xFFFF);
                sound_port0(cr, 0x46);
                cyc(46);
                cr->x = rd16(0x7E0000 | W(actor_current));
                cr->a = 0xBC9A;
                wr16(dpx(cr, 0x08), cyc_then(46, 0xBC9A));
                cr->a = 0x0001;
                cyc(56);
                wr16(W(menu_1905), 0x0001);
                cr->a = 0x0028;
                cyc(56);
                wr16(W(menu_1907), 0x0028);
                cr->a = 0x464A;
                cyc(56);
                wr16(W(wram_7EFD65), 0x464A);
                cr->a = 0x0000;
                set_p(cr, nz16(cr->p, 0));
                cyc(56);
                wr16(W(word_list_b_size), 0x0000);
                r_sep(cr, P_M);
                cr->a = (uint16_t)((cr->a & 0xFF00) | 0xE0);
                set_p(cr, nz8(cr->p, 0xE0));
                {
                    cyc(198);
                    uint16_t x = cr->x;

                    cr->x = 0x420C;
                    set_p(cr, nz16(cr->p, 0x420C));
                    FrameByteWritePush_cr(cr);
                    cr->x = x;
                }
                r_rep(cr, P_M);
                cyc(186);
                return;
            } else { cyc(40); }
            sound_port0(cr, 0x46);
            cr->a = 0x0000;
            set_p(cr, nz16(cr->p, 0));
            cyc(78);
            wr16(W(wram_190B), 0x0000);
            cyc(32);
            wr16(W(wram_190D), 0x0000);
            set10(cr, 0x0005, 0xFFFF);
            kinds67_anim(cr);
            cyc(36);
            set10(cr, 0x0008, 0x0000);
            set10(cr, 0x0009, 0x0000);
            cr->a = 0xC225;
            wr16(dpx(cr, 0x08), cyc_then(46, 0xC225));
            cr->a = 0x0000;
            set_p(cr, nz16(cr->p, 0));
            cyc(56);
            wr16(W(word_list_a_size), 0x0000);
            cyc(42);
            return;
        }
    }
    cyc(74);
    cr->a = pad;
    r_and16(cr, 0x2000);
    if (!cr->a)
        { cyc(42); return; }
    cyc(38);
    r_lda16(cr, W(wram_18C3));
    r_cmp16(cr, 0x0000);
    if (cr->a)
        { cyc(112); return; } else { cyc(46); }
    sound_port0(cr, 0x46);
    cyc(22);
    set10(cr, 0x0004, 0xFFFF);
    set10(cr, 0x0005, 0xFFFF);
    kinds67_anim(cr);
    cyc(36);
    for (k = 0x000A; k <= 0x000E; k++)
        set10(cr, (uint16_t)k, 0x0000);
    cr->a = 0xBEEF;
    wr16(dpx(cr, 0x08), cyc_then(46, 0xBEEF));
    cyc(42);
}

void abi_MenuSettingsFrame(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuSettingsFrame(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* MenuChoicePressesP1 as its adapter leaves the registers. */
static void w_MenuChoicePressesP1(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = MenuChoicePressesP1(cr->db, &cr->x, &cr->y, &p);
    set_p(cr, p);
}
