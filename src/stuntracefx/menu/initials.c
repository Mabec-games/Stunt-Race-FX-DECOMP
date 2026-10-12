/*
 * initials.c: the menu screen where player 1 enters three letters, 0-$19
 * each, at $18EF, $18F1 and $18F3; the position being changed is $18ED
 * (0-2). Holding up or down repeats after 13 frames, counted in $190B (up)
 * and $190D (down) from the held buttons at $0309.
 */
#include "machine.h"

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
