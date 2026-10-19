/*
 * initials.c: the menu screen where player 1 enters three letters, 0-$19
 * each, at $18EF, $18F1 and $18F3; the position being changed is $18ED
 * (0-2). Holding up or down repeats after 13 frames, counted in $190B (up)
 * and $190D (down) from the held buttons at $0309.
 */
#include "machine.h"
#include "cpu_ops.h"

void SoundPort0QueuePush(uint8_t command);
int MenuActorFind(uint8_t bank, uint16_t kind, uint16_t *x, uint16_t *y, uint8_t *p);
uint16_t MenuRowPalette5(uint8_t bank, uint16_t *x, uint16_t *y, uint8_t *p);
uint16_t MenuRowBlinkSettings(uint8_t bank, uint16_t *x, uint16_t *y, uint8_t *p);

#define POSITION 0x0018EDu
#define LETTERS 0x0018EFu
#define UP_HELD 0x00190Bu
#define DOWN_HELD 0x00190Du
#define JOY1_HELD 0x000309u
#define PRESS 0x118Fu

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

    a = rd16(cyc_then(24, dbw(bank, PRESS))) & 0x0200;
    *p = nz16(*p, a);
    if (a) {
        cyc(72);
        a = rd16(POSITION);
        *p = nz16(*p, a);
        if (a) {
            cyc(16);
            sound0(bank, 0x46, p);
            MenuRowPalette5(bank, x, y, p);
            cyc(74);
            a = (uint16_t)(rd16(POSITION) - 1);
            *p = nz16(*p, a);
            cyc(46);
            wr16(POSITION, a);
        }
    } else { cyc(46); }
    a = rd16(cyc_then(24, dbw(bank, PRESS))) & 0x0100;
    *p = nz16(*p, a);
    if (a) {
        cyc(72);
        a = rd16(POSITION);
        *p = cmp16(*p, a, 0x0002);
        if (a != 0x0002) {
            cyc(40);
            sound0(bank, 0x46, p);
            MenuRowPalette5(bank, x, y, p);
            cyc(74);
            a = (uint16_t)(rd16(POSITION) + 1);
            *p = nz16(*p, a);
            cyc(46);
            wr16(POSITION, a);
        }
    } else { cyc(46); }
    cyc(32);
    a = rd16(POSITION);
    *p = (uint8_t)((*p & ~P_C) | (a & 0x8000 ? P_C : 0));
    *x = (uint16_t)(a << 1);
    *p = nz16(*p, *x);

    cyc(60);
    rep = hold_repeats(rd16(JOY1_HELD) & 0x0800, UP_HELD, p, &a);
    if (!rep) {
        a = rd16(dbw(bank, PRESS)) & 0x0800;
        *p = nz16(*p, a);
    }
    if (rep || a) {
        cyc(96);
        wr16(UP_HELD, 0x0000);
        sound0(bank, 0x45, p);
        cyc(54);
        v = (uint16_t)(rd16(LETTERS + *x) + 1);
        *p = cmp16(*p, v, 0x001A);
        if (v == 0x001A) {
            v = 0x0000;
            *p = nz16(*p, v);
        }
        cyc(92);
        wr16(LETTERS + *x, v);
        return v;
    }

    cyc(78);
    rep = hold_repeats(rd16(JOY1_HELD) & 0x0400, DOWN_HELD, p, &a);
    if (!rep) {
        a = rd16(dbw(bank, PRESS)) & 0x0400;
        *p = nz16(*p, a);
        if (!a)
            { cyc(88); return a; }
    }
    cyc(96);
    wr16(DOWN_HELD, 0x0000);
    sound0(bank, 0x45, p);
    cyc(54);
    v = (uint16_t)(rd16(LETTERS + *x) - 1);
    *p = cmp16(*p, v, 0xFFFF);
    if (v == 0xFFFF) {
        v = 0x0019;
        *p = nz16(*p, v);
    }
    cyc(92);
    wr16(LETTERS + *x, v);
    return v;
}

void abi_MenuInitialsStep(void)
{
    uint8_t p = cpu.p;

    cpu.a = MenuInitialsStep(cpu.db, &cpu.x, &cpu.y, &p);
    cpu.p = p;
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

    wr16(dbw(bank, 0x118B), cyc_then(24, 0x0000));
    cyc(32);
    i = (uint16_t)(rd16(0x001191) - 2);
    if (!(i & 0x8000)) {
        do {
            *x = i;
            cyc(90);
            v = rd16(0x001195 + i);
            wr16(dbw(bank, PRESS), cyc_then(24, v));
            v |= rd16(cyc_then(24, dbw(bank, 0x118B)));
            wr16(dbw(bank, 0x118B), cyc_then(24, v));
            cyc(76);
            MenuInitialsStep(bank, x, y, p);
            i = (uint16_t)(i - 2);
        } while (cyc_loop(42, !(i & 0x8000)));
        cyc(136);
        wr16(0x001191, 0x0000);
    }
    cyc(70);
    MenuActorFind(bank, 0x0009, x, y, p);
    a = MenuRowBlinkSettings(bank, x, y, p);
    *x = rd16(cyc_then(144, dbw(bank, 0x11F9)));
    *p = nz16(*p, *x);
    return a;
}

void abi_MenuInitialsPresses(void)
{
    uint8_t p = cpu.p;

    cpu.a = MenuInitialsPresses(cpu.db, &cpu.x, &cpu.y, &p);
    cpu.p = p;
}

void abi_MenuFixedColourDown(void);
void abi_RaceFrameStep(void);
void abi_MenuActorsSet10(void);
void abi_MenuActorFind(void);
void abi_MenuActorFree(void);
void abi_SoundPort0QueuePush(void);

/* Direct page word off + X. */
static uint32_t dpx(uint8_t off)
{
    return (uint16_t)(cpu.d + off + cpu.x);
}

/* Sound n on port 0 with 8-bit A (SEP, LDA, JSL, REP). */
static void sound_port0(uint8_t n)
{
    cpu.p |= P_M;
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | n);
    cpu.p = nz8(cpu.p, n);
    (cyc(100), abi_SoundPort0QueuePush());
    cpu.p &= (uint8_t)~P_M;
}

/* MenuActorsSet10 for kind k with value v. */
static void set10(uint16_t k, uint16_t v)
{
    cpu.a = k;
    cpu.y = v;
    cpu.p = nz16(cpu.p, v);
    (cyc(38), abi_MenuActorsSet10());
}

/* Kind k's actor (MenuActorFind) to animation n, restarting its position
   and timer when it had another. */
static void kind_anim(uint16_t k, uint16_t n)
{
    cpu.a = k;
    cpu.p = nz16(cpu.p, k);
    (cyc(70), abi_MenuActorFind());
    cpu.a = rd16(cyc_then(22, dpx(0x14)));
    op_cmp16(n);
    if (cpu.a == n)
        { cyc(42); return; }
    cpu.a = n;
    wr16(dpx(0x14), cyc_then(82, n));
    wr16(dpx(0x16), cyc_then(22, 0x0000));
    wr16(dpx(0x18), cyc_then(22, 0x0000));
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
void abi_MenuInitialsFrame(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint16_t pad;
    unsigned k;

    abi_MenuFixedColourDown();
    cpu.a = 0x0000;
    cpu.p = nz16(cpu.p, 0);
    (cyc(72), abi_RaceFrameStep());
    cpu.a = 0x0001;
    cyc(56);
    wr16(0x0018FD, 0x0001);
    (cyc(46), abi_MenuInitialsPresses());
    cyc(66);
    pad = rd16(b | 0x118B);
    cpu.y = pad;
    cyc(24);
    wr16(b | 0x118B, 0x0000);
    cpu.a = pad;
    op_and16(0x9000);
    if (!(cpu.p & P_Z)) {
        cpu.a = 0x0001;
        cyc(106);
        wr16(b | 0x11EB, 0x0001);
        sound_port0(0x46);
        cyc(46);
        cpu.y = rd16(b | 0x11FB);
        cpu.p = nz16(cpu.p, cpu.y);
        (cyc(46), abi_MenuActorFree());
        return;
    }
    cpu.a = pad;
    op_and16(0x4000);
    if (!(cpu.p & P_Z)) {
        cyc(158);
        cpu.x = rd16(b | 0x11F9);
        set10(0x0005, 0x0000);
        {
            cyc(30);
            uint16_t x = cpu.x;                  /* PHX */

            kind_anim(0x0006, 0x00AE);
            kind_anim(0x0007, 0x00B8);
            cpu.x = x;                           /* PLX */
            cpu.p = nz16(cpu.p, x);
        }
        cyc(36);
        set10(0x0008, 0xFFFF);
        set10(0x0009, 0xFFFF);
        cpu.a = 0xBA4E;
        wr16(dpx(0x08), cyc_then(46, 0xBA4E));
        wr16(dpx(0x0A), cyc_then(22, 0x0000));
        kind_anim(0x0004, 0x008A);
        cpu.a = 0x0000;
        cyc(56);
        wr16(0x001191, 0x0000);
        sound_port0(0x47);
        return;
    }
    cpu.a = pad;
    op_and16(0x2000);
    if (cpu.p & P_Z)
        { cyc(162); return; }
    cyc(200);
    cpu.a = rd16(0x0018C3);
    op_cmp16(0x0000);
    if (cpu.a != 0)
        { cyc(42); return; } else { cyc(36); }
    sound_port0(0x46);
    cyc(22);
    set10(0x0004, 0xFFFF);
    set10(0x0008, 0xFFFF);
    set10(0x0009, 0xFFFF);
    for (k = 0x0A; k <= 0x0E; k++)
        set10((uint16_t)k, 0x0000);
    cpu.a = 0xBFB8;
    wr16(dpx(0x08), cyc_then(46, 0xBFB8));
    cpu.a = 0x0000;
    cyc(56);
    wr16(0x001191, 0x0000);
    cpu.p = nz16(cpu.p, 0);
}

void abi_SoundPort0QueuePushRaw(void);
void abi_MenuChoicePressesP1(void);
void abi_MenuActorSpawn(void);
void abi_FrameByteWritePush(void);

/* Kinds 6 and 7 to animations $C2 and $C8, X kept (PHX, PLX). */
static void kinds67_anim(void)
{
    uint16_t x = cpu.x;

    cyc(30);
    kind_anim(0x0006, 0x00C2);
    kind_anim(0x0007, 0x00C8);
    cpu.x = x;
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
void abi_MenuSettingsFrame(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint16_t pad, v;
    unsigned k;

    cpu.a = rd16(cyc_then(22, dpx(0x0A)));
    cpu.p = nz16(cpu.p, cpu.a);
    if (cpu.a) {
        cpu.a = (uint16_t)(cpu.a - 1);
        cpu.p = nz16(cpu.p, cpu.a);
        wr16(dpx(0x0A), cyc_then(52, cpu.a));
        v = cpu.a;
        op_cmp16(0x0013);
        if (v == 0x0013 || (cyc(62), v == 0x000E) || (cyc(40), v == 0x0009) || (cyc(34), v == 0x0004)) {
            cyc(78);
            if (v != 0x0013)
                op_cmp16(v); else { cyc(26); }
            cpu.p |= P_M;
            cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x3F);
            cpu.p = nz8(cpu.p, 0x3F);
            abi_SoundPort0QueuePushRaw();
            cpu.p &= (uint8_t)~P_M;
        } else
            op_cmp16(0x0004);
        cpu.a = rd16(cyc_then(44, dpx(0x0A)));
        cpu.p = nz16(cpu.p, cpu.a);
        if (!cpu.a)
            { cyc(16); kind_anim(0x0004, 0x008A); } else { cyc(22); }
    } else { cyc(22); }
    cyc(24);
    cpu.x = rd16(b | 0x11F9);
    abi_MenuFixedColourDown();
    (cyc(64), abi_MenuFixedColourDown());
    cpu.a = 0x0000;
    cpu.p = nz16(cpu.p, 0);
    (cyc(72), abi_RaceFrameStep());
    cpu.a = 0x0001;
    cpu.p = nz16(cpu.p, 1);
    cyc(56);
    wr16(0x0018FD, 0x0001);
    cyc(24);
    cpu.x = rd16(b | 0x11F9);
    (cyc(46), abi_MenuChoicePressesP1());
    cyc(66);
    cpu.x = rd16(b | 0x11F9);
    cyc(24);
    pad = rd16(b | 0x118B);
    cpu.y = pad;
    cyc(24);
    wr16(b | 0x118B, 0x0000);
    cpu.a = pad;
    op_and16(0x4000);
    if (cpu.a) {
        cyc(106);
        cpu.x = rd16(b | 0x11F9);
        cpu.a = 0xC872;
        wr16(dpx(0x08), cyc_then(46, 0xC872));
        cpu.a = 0x0080;
        wr16(dpx(0x0A), cyc_then(46, 0x0080));
        sound_port0(0x47);
        cpu.x = 0x027E;
        cpu.p = nz16(cpu.p, 0x027E);
        (cyc(92), abi_MenuActorSpawn());
        cyc(42);
        set10(0x0004, 0xFFFF);
        set10(0x0005, 0xFFFF);
        kinds67_anim();
        cyc(60);
        cpu.x = rd16(b | 0x11F9);
        cpu.p = nz16(cpu.p, cpu.x);
        return;
    }
    cyc(90);
    op_lda16(0x0018F9);
    if (!cpu.a) {
        cyc(52);
        cpu.a = pad;
        op_and16(0x9000);
        if (cpu.a) {
            cyc(78);
            op_lda16(0x0018C3);
            op_cmp16(0x0002);
            if (cpu.a == 0x0002) {
                cyc(42);
                kind_anim(0x0004, 0x0090);
                set10(0x0004, 0xFFFF);
                set10(0x0005, 0xFFFF);
                sound_port0(0x46);
                cyc(46);
                cpu.x = rd16(b | 0x11F9);
                cpu.a = 0xBC9A;
                wr16(dpx(0x08), cyc_then(46, 0xBC9A));
                cpu.a = 0x0001;
                cyc(56);
                wr16(0x001905, 0x0001);
                cpu.a = 0x0028;
                cyc(56);
                wr16(0x001907, 0x0028);
                cpu.a = 0x464A;
                cyc(56);
                wr16(0x7EFD65, 0x464A);
                cpu.a = 0x0000;
                cpu.p = nz16(cpu.p, 0);
                cyc(56);
                wr16(0x001193, 0x0000);
                op_sep(P_M);
                cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0xE0);
                cpu.p = nz8(cpu.p, 0xE0);
                {
                    cyc(22);
                    uint16_t x = cpu.x;

                    cpu.x = 0x420C;
                    cpu.p = nz16(cpu.p, 0x420C);
                    abi_FrameByteWritePush();
                    cpu.x = x;
                }
                op_rep(P_M);
                cyc(144);
                return;
            } else { cyc(36); }
            sound_port0(0x46);
            cpu.a = 0x0000;
            cpu.p = nz16(cpu.p, 0);
            cyc(78);
            wr16(0x00190B, 0x0000);
            cyc(32);
            wr16(0x00190D, 0x0000);
            set10(0x0005, 0xFFFF);
            kinds67_anim();
            cyc(36);
            set10(0x0008, 0x0000);
            set10(0x0009, 0x0000);
            cpu.a = 0xC225;
            wr16(dpx(0x08), cyc_then(46, 0xC225));
            cpu.a = 0x0000;
            cpu.p = nz16(cpu.p, 0);
            cyc(56);
            wr16(0x001191, 0x0000);
            return;
        }
    }
    cyc(74);
    cpu.a = pad;
    op_and16(0x2000);
    if (!cpu.a)
        return;
    cyc(38);
    op_lda16(0x0018C3);
    op_cmp16(0x0000);
    if (cpu.a)
        { cyc(66); return; } else { cyc(42); }
    sound_port0(0x46);
    cyc(22);
    set10(0x0004, 0xFFFF);
    set10(0x0005, 0xFFFF);
    kinds67_anim();
    cyc(36);
    for (k = 0x000A; k <= 0x000E; k++)
        set10((uint16_t)k, 0x0000);
    cpu.a = 0xBEEF;
    wr16(dpx(0x08), cyc_then(46, 0xBEEF));
}
