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
    return (uint8_t)((nz16(p, (uint16_t)(a - b)) & ~P_C) | (a >= b ? P_C : 0));
}

/* SoundPort0QueuePush: N, Z come back from the data bank. */
static void sound0(uint8_t bank, uint8_t command, uint8_t *p)
{
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
        wr16(count, *a);
        return 0;
    }
    v = (uint16_t)(rd16(count) + 1);
    *p = cmp16(*p, v, 0x000D);
    *a = v;
    if (*p & P_C)
        return 1;
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

    a = rd16(dbw(bank, PRESS)) & 0x0200;
    *p = nz16(*p, a);
    if (a) {
        a = rd16(POSITION);
        *p = nz16(*p, a);
        if (a) {
            sound0(bank, 0x46, p);
            MenuRowPalette5(bank, x, y, p);
            a = (uint16_t)(rd16(POSITION) - 1);
            *p = nz16(*p, a);
            wr16(POSITION, a);
        }
    }
    a = rd16(dbw(bank, PRESS)) & 0x0100;
    *p = nz16(*p, a);
    if (a) {
        a = rd16(POSITION);
        *p = cmp16(*p, a, 0x0002);
        if (a != 0x0002) {
            sound0(bank, 0x46, p);
            MenuRowPalette5(bank, x, y, p);
            a = (uint16_t)(rd16(POSITION) + 1);
            *p = nz16(*p, a);
            wr16(POSITION, a);
        }
    }
    a = rd16(POSITION);
    *p = (uint8_t)((*p & ~P_C) | (a & 0x8000 ? P_C : 0));
    *x = (uint16_t)(a << 1);
    *p = nz16(*p, *x);

    rep = hold_repeats(rd16(JOY1_HELD) & 0x0800, UP_HELD, p, &a);
    if (!rep) {
        a = rd16(dbw(bank, PRESS)) & 0x0800;
        *p = nz16(*p, a);
    }
    if (rep || a) {
        wr16(UP_HELD, 0x0000);
        sound0(bank, 0x45, p);
        v = (uint16_t)(rd16(LETTERS + *x) + 1);
        *p = cmp16(*p, v, 0x001A);
        if (v == 0x001A) {
            v = 0x0000;
            *p = nz16(*p, v);
        }
        wr16(LETTERS + *x, v);
        return v;
    }

    rep = hold_repeats(rd16(JOY1_HELD) & 0x0400, DOWN_HELD, p, &a);
    if (!rep) {
        a = rd16(dbw(bank, PRESS)) & 0x0400;
        *p = nz16(*p, a);
        if (!a)
            return a;
    }
    wr16(DOWN_HELD, 0x0000);
    sound0(bank, 0x45, p);
    v = (uint16_t)(rd16(LETTERS + *x) - 1);
    *p = cmp16(*p, v, 0xFFFF);
    if (v == 0xFFFF) {
        v = 0x0019;
        *p = nz16(*p, v);
    }
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

    wr16(dbw(bank, 0x118B), 0x0000);
    i = (uint16_t)(rd16(0x001191) - 2);
    if (!(i & 0x8000)) {
        do {
            *x = i;
            v = rd16(0x001195 + i);
            wr16(dbw(bank, PRESS), v);
            v |= rd16(dbw(bank, 0x118B));
            wr16(dbw(bank, 0x118B), v);
            MenuInitialsStep(bank, x, y, p);
            i = (uint16_t)(i - 2);
        } while (!(i & 0x8000));
        wr16(0x001191, 0x0000);
    }
    MenuActorFind(bank, 0x0009, x, y, p);
    a = MenuRowBlinkSettings(bank, x, y, p);
    *x = rd16(dbw(bank, 0x11F9));
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
    abi_SoundPort0QueuePush();
    cpu.p &= (uint8_t)~P_M;
}

/* MenuActorsSet10 for kind k with value v. */
static void set10(uint16_t k, uint16_t v)
{
    cpu.a = k;
    cpu.y = v;
    cpu.p = nz16(cpu.p, v);
    abi_MenuActorsSet10();
}

/* Kind k's actor (MenuActorFind) to animation n, restarting its position
   and timer when it had another. */
static void kind_anim(uint16_t k, uint16_t n)
{
    cpu.a = k;
    cpu.p = nz16(cpu.p, k);
    abi_MenuActorFind();
    cpu.a = rd16(dpx(0x14));
    op_cmp16(n);
    if (cpu.a == n)
        return;
    cpu.a = n;
    wr16(dpx(0x14), n);
    wr16(dpx(0x16), 0x0000);
    wr16(dpx(0x18), 0x0000);
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
    abi_RaceFrameStep();
    cpu.a = 0x0001;
    wr16(0x0018FD, 0x0001);
    abi_MenuInitialsPresses();
    pad = rd16(b | 0x118B);
    cpu.y = pad;
    wr16(b | 0x118B, 0x0000);
    cpu.a = pad;
    op_and16(0x9000);
    if (!(cpu.p & P_Z)) {
        cpu.a = 0x0001;
        wr16(b | 0x11EB, 0x0001);
        sound_port0(0x46);
        cpu.y = rd16(b | 0x11FB);
        cpu.p = nz16(cpu.p, cpu.y);
        abi_MenuActorFree();
        return;
    }
    cpu.a = pad;
    op_and16(0x4000);
    if (!(cpu.p & P_Z)) {
        cpu.x = rd16(b | 0x11F9);
        set10(0x0005, 0x0000);
        {
            uint16_t x = cpu.x;                  /* PHX */

            kind_anim(0x0006, 0x00AE);
            kind_anim(0x0007, 0x00B8);
            cpu.x = x;                           /* PLX */
            cpu.p = nz16(cpu.p, x);
        }
        set10(0x0008, 0xFFFF);
        set10(0x0009, 0xFFFF);
        cpu.a = 0xBA4E;
        wr16(dpx(0x08), 0xBA4E);
        wr16(dpx(0x0A), 0x0000);
        kind_anim(0x0004, 0x008A);
        cpu.a = 0x0000;
        wr16(0x001191, 0x0000);
        sound_port0(0x47);
        return;
    }
    cpu.a = pad;
    op_and16(0x2000);
    if (cpu.p & P_Z)
        return;
    cpu.a = rd16(0x0018C3);
    op_cmp16(0x0000);
    if (cpu.a != 0)
        return;
    sound_port0(0x46);
    set10(0x0004, 0xFFFF);
    set10(0x0008, 0xFFFF);
    set10(0x0009, 0xFFFF);
    for (k = 0x0A; k <= 0x0E; k++)
        set10((uint16_t)k, 0x0000);
    cpu.a = 0xBFB8;
    wr16(dpx(0x08), 0xBFB8);
    cpu.a = 0x0000;
    wr16(0x001191, 0x0000);
    cpu.p = nz16(cpu.p, 0);
}
