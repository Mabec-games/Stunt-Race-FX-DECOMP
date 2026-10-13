/*
 * course_preview.c: the menu objects that run the course preview, a race
 * frame (RaceFrameStep) drawn each menu frame behind the course select.
 */
#include "machine.h"

void abi_MenuFixedColourDown(void);
void abi_RaceFrameStep(void);
void abi_MenuChoiceRepeat(void);
void abi_MenuSet1A66(void);
void abi_MenuSetPairAnimations(void);
void abi_MenuActorFind(void);
void abi_MenuActorsSet10(void);
void abi_FrameByteWritePush(void);
void abi_MenuChoicePressesP2(void);
void abi_MenuActorFree(void);
void abi_SoundPort0QueuePushRaw(void);
void abi_SoundPort0QueuePush(void);

#define ACTOR(x, f) ((uint16_t)((f) + (x)))     /* an actor field, bank 0 */

/* The preview's frame: the fixed colour stepped down `n` times
   (MenuFixedColourDown), a race frame shown (RaceFrameStep with A = 0)
   and $18FD = 1. Returns $1905. */
static uint16_t preview_frame(unsigned n)
{
    while (n--)
        abi_MenuFixedColourDown();
    cpu.a = 0x0000;
    abi_RaceFrameStep();
    cpu.a = 0x0001;
    wr16(0x0018FD, 0x0001);
    cpu.a = rd16(0x001905);
    cpu.p = nz16(cpu.p, cpu.a);
    return cpu.a;
}

/* MenuActorsSet10 for groups 4 and 5 with Y. */
static void groups_45(uint16_t y)
{
    cpu.a = 0x0004;
    cpu.y = y;
    abi_MenuActorsSet10();
    cpu.a = 0x0005;
    cpu.y = y;
    abi_MenuActorsSet10();
}

/* `channels` pushed for HDMAEN ($420C) on the frame write list. */
static void hdma_on(uint8_t channels)
{
    uint16_t x = cpu.x;

    cpu.p |= P_M;
    cpu.p &= (uint8_t)~P_X;
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | channels);
    cpu.x = 0x420C;
    abi_FrameByteWritePush();
    cpu.x = x;
    cpu.p &= (uint8_t)~P_M;
}

/* Animation `anim` on the kind 4 object unless it has it already. */
static void kind4_animation(uint16_t anim)
{
    uint16_t x;

    cpu.a = 0x0004;
    abi_MenuActorFind();
    x = cpu.x;
    cpu.a = rd16(ACTOR(x, 0x14));
    if (cpu.a != anim) {
        cpu.a = anim;
        wr16(ACTOR(x, 0x14), anim);
        wr16(ACTOR(x, 0x16), 0x0000);
        wr16(ACTOR(x, 0x18), 0x0000);
    }
}

/*
 * @routine 0B:BC9A MenuPreviewIdle
 * WHAT IT DOES: The course preview while the menu waits: two fixed colour
 * steps down, a race frame shown, $18FD = 1; unless $1905 is set, makes
 * $BCD2 the object's handler, repeats the choice (MenuChoiceRepeat), sets
 * $1A66 from the previous selection ($18C9, MenuSet1A66) and the pair's
 * animations (MenuSetPairAnimations), and shows another race frame.
 * HOW: Calls in turn; the object is the current one ($11F9).
 * Entry: M 16-bit, X 16-bit, D = $0000. Exit: as the last call leaves it.
 */
void abi_MenuPreviewIdle(void)
{
    uint16_t x;

    if (preview_frame(2))
        return;
    x = rd16(0x0011F9);
    cpu.x = x;
    cpu.a = 0xBCD2;
    wr16(ACTOR(x, 0x08), 0xBCD2);
    abi_MenuChoiceRepeat();
    cpu.a = rd16(0x0018C9);
    abi_MenuSet1A66();
    abi_MenuSetPairAnimations();
    cpu.a = 0x0000;
    abi_RaceFrameStep();
}

/*
 * @routine 0B:BCF1 MenuPreviewStart
 * WHAT IT DOES: The first frame of the course preview's countdown: two
 * fixed colour steps down, a race frame shown, $18FD = 1; unless $1905 is
 * set, makes MenuPreviewCountdown ($BD55) the object's handler with a count
 * of 20 (+$0A), starts animation $D8 on the kind 4 object (unless it has
 * it), clears +$10 of groups 4 and 5 (MenuActorsSet10 with Y = 0) and asks
 * for HDMA channels 6 and 7 ($C0 to HDMAEN on the frame write list).
 * HOW: Calls in turn; the object is the current one ($11F9).
 * Entry: M 16-bit, X 16-bit, D = $0000. Exit: as FrameByteWritePush
 * leaves it, with X the kind 4 object and M 16-bit.
 */
void abi_MenuPreviewStart(void)
{
    uint16_t x;

    if (preview_frame(2))
        return;
    x = rd16(0x0011F9);
    cpu.x = x;
    wr16(ACTOR(x, 0x08), 0xBD55);
    wr16(ACTOR(x, 0x0A), 0x0014);
    kind4_animation(0x00D8);
    groups_45(0x0000);
    hdma_on(0xC0);
}

/*
 * @routine 0B:BD55 MenuPreviewCountdown
 * WHAT IT DOES: The course preview's countdown: steps the object's count
 * (+$0A) down, with sound $3F at 19, 14, 9 and 4, and at 0 starts
 * animation $90 on the kind 4 object; then a fixed colour step down, a race
 * frame shown and $18FD = 1, and player 2's choice keys
 * (MenuChoicePressesP2): with $9000 pressed it ends the menu ($11EB = 1,
 * $18FD = 0, sound $46, the object freed); with $4000 it sets groups 4 and
 * 5 to $FFFF (MenuActorsSet10), plays sound $47, makes $BE29 the object's
 * handler, sets $1905 = 1, $1907 = $28 and $7E:FD65 = $464A, and asks for
 * HDMA channels 5, 6 and 7 ($E0).
 * HOW: The count compares through 16-bit A; the object is reloaded from
 * $11F9 after the kind 4 search. The $4000 case is not reached in the
 * traces so far.
 * Entry: M 16-bit, X 16-bit, D = $0000; X = the object. Exit: as the last
 * call leaves it, M 16-bit.
 */
void abi_MenuPreviewCountdown(void)
{
    uint16_t x = cpu.x, n, keys;

    n = rd16(ACTOR(x, 0x0A));
    cpu.a = n;
    if (n) {
        n = (uint16_t)(n - 1);
        cpu.a = n;
        wr16(ACTOR(x, 0x0A), n);
        if (n == 0x0013 || n == 0x000E || n == 0x0009 || n == 0x0004) {
            cpu.p |= P_M;
            cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x3F);
            abi_SoundPort0QueuePushRaw();
            cpu.p &= (uint8_t)~P_M;
        }
        n = rd16(ACTOR(x, 0x0A));
        cpu.a = n;
        if (!n)
            kind4_animation(0x0090);
    }
    cpu.x = rd16(0x0011F9);
    abi_MenuFixedColourDown();
    cpu.a = 0x0000;
    abi_RaceFrameStep();
    cpu.a = 0x0001;
    wr16(0x0018FD, 0x0001);
    cpu.x = rd16(0x0011F9);
    abi_MenuChoicePressesP2();
    keys = rd16(0x00118D);
    cpu.y = keys;
    cpu.a = (uint16_t)(keys & 0x4000);
    if (cpu.a) {
        groups_45(0xFFFF);
        cpu.p |= P_M;
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x47);
        abi_SoundPort0QueuePush();
        cpu.p &= (uint8_t)~P_M;
        x = rd16(0x0011F9);
        cpu.x = x;
        wr16(ACTOR(x, 0x08), 0xBE29);
        wr16(0x001905, 0x0001);
        wr16(0x001907, 0x0028);
        wr16(0x7EFD65, 0x464A);
        hdma_on(0xE0);
        return;
    }
    cpu.a = (uint16_t)(keys & 0x9000);
    cpu.p = nz16(cpu.p, cpu.a);
    if (!cpu.a)
        return;
    cpu.a = 0x0001;
    wr16(0x0011EB, 0x0001);
    cpu.a = 0x0000;
    wr16(0x0018FD, 0x0000);
    cpu.p |= P_M;
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x46);
    abi_SoundPort0QueuePush();
    cpu.p &= (uint8_t)~P_M;
    cpu.y = rd16(0x0011FB);
    abi_MenuActorFree();
}
