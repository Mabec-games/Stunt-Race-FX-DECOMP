/*
 * course_preview.c: the menu objects that run the course preview, a race
 * frame (RaceFrameStep) drawn each menu frame behind the course select.
 */
#include "machine.h"
#include "cpu_ops.h"

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

/* `channels` pushed for HDMAEN ($420C) on the frame write list; X kept,
   N and Z from it (pulled back 16-bit). */
static void hdma_on(uint8_t channels)
{
    uint16_t x = cpu.x;

    cpu.p |= P_M;
    cpu.p &= (uint8_t)~P_X;
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | channels);
    cpu.x = 0x420C;
    abi_FrameByteWritePush();
    cpu.x = x;
    cpu.p = nz16(cpu.p, x);  /* PLX, X 16-bit */
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
        cpu.a = 0x464A;
        cpu.p = nz16(cpu.p, cpu.a);
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

void abi_MenuActorPoolInit(void);
void abi_MenuHdmaSetupB(void);
void abi_MenuActorSpawnList(void);
void abi_RaceStart(void);
void abi_MenuActorsRun(void);
void abi_MenuOamBuild(void);
void abi_MenuScrollReset(void);
void abi_MenuPaletteLoad(void);
void abi_MenuPaletteSave(void);
void abi_MenuPaletteScale(void);
void abi_MenuPictureToVram(void);
void abi_MenuPictureDC8CGrid0(void);
void abi_MenuLoadPicturesTo7F(void);
void abi_MenuLoadClearGsu(void);
void abi_MenuLoadBlank0800(void);
void abi_MenuLoadBlank1000(void);
void abi_MenuLoadBlank1800(void);
void abi_SoundReset(void);
void abi_MenuWaitFrame(void);
void abi_MenuScreenOpenPreview(void);
void abi_MenuScreenOpenPreviewFade(void);

/* A store as STZ/STA makes it at the accumulator width in P. */
static void wr_m(uint32_t a, uint16_t v)
{
    if (cpu.p & P_M)
        wr8(a, (uint8_t)v);
    else
        wr16(a, v);
}

/* A bank $17 picture to VRAM `vram`: its byte count is the big-endian
   word before it ($11D5), then MenuPictureToVram with A = the picture and
   X = the bank. */
static void preview_picture(uint16_t vram, uint16_t pic)
{
    uint16_t n;

    cpu.p &= (uint8_t)~(P_M | P_X);
    cpu.x = vram;
    wr16(0x002116, vram);
    n = rd16(0x170000 | (uint16_t)(pic - 2));
    cpu.a = (uint16_t)((n << 8) | (n >> 8));
    wr16(0x0011D5, cpu.a);
    cpu.a = pic;
    cpu.x = 0x0017;
    abi_MenuPictureToVram();
}

/*
 * @routine 0B:F784 MenuScreenOpenPreview
 * WHAT IT DOES: Opens the menu screen with the course preview behind it:
 * with the display, HDMA, DMA and interrupts off, clears the menu's lists
 * and state, makes the menu objects (MenuActorPoolInit, spawn list $10) and
 * starts a race in the background (RaceStart), runs the objects once;
 * sets mode 3 with BG1 and BG2 maps at $6000 and $6800 and tiles at 0 and
 * $6000, no colour math, BG2 scrolled to $FF00 and BG1 to $0100, BG1, BG2
 * and sprites on the main screen with windows on BG1 and sprites; loads
 * the palette from $1C:E3AC, saves it and scales colours 144-159 and
 * 208-223 to full; draws three pictures from bank $17 ($CC78 at $3000,
 * $CE78 at $6800, $DD8C at $4000) and the $DC8C grid, copies $1000 bytes
 * of the Super FX picture ($70:3000) to VRAM $6000; loads the menu's
 * pictures and blank tiles (MenuLoad*); turns on the IRQ at line $E0 and
 * the joypad, waits two frames, runs the objects again, and turns on HDMA
 * channels 4 and 7 with every frame update asked for.
 * HOW: DB = 0 throughout; P and DB saved and restored.
 * Entry: D = $0000. Exit: A = 0; X, Y as MenuOamBuild left them; P and DB
 * unchanged.
 */
void abi_MenuScreenOpenPreview(void)
{
    uint8_t p = cpu.p, db = cpu.db;
    uint16_t x;
    static const struct { uint16_t reg; uint8_t v; } ppu[] = {
        {0x2105, 0x03}, {0x2131, 0x00}, {0x2130, 0x00}, {0x2107, 0x61}, {0x2108, 0x69}, {0x210B, 0x30},
        {0x210F, 0x00}, {0x210F, 0xFF}, {0x2127, 0xFF}, {0x210D, 0x00}, {0x210D, 0x01}, {0x212C, 0x13},
        {0x2123, 0x30}, {0x2125, 0x03}, {0x212E, 0x12}};
    unsigned i;

    cpu.p = (uint8_t)((cpu.p | P_M) & ~P_X);
    cpu.a = (uint16_t)(cpu.a & 0xFF00);
    cpu.db = 0x00;
    wr8(0x002100, 0x80);
    wr8(0x00420C, 0x00);
    wr8(0x00420B, 0x00);
    wr8(0x0011DA, 0x00);
    wr8(0x004200, 0x00);
    cpu.p &= (uint8_t)~(P_M | P_X);
    wr16(0x00162D, 0x0000);
    wr16(0x0016AF, 0x0000);
    wr16(0x001191, 0x0000);
    wr16(0x001193, 0x0000);
    wr16(0x000D7D, 0x0006);
    wr16(0x001189, 0x1234);
    wr16(0x00118F, 0x0000);
    wr16(0x0011EB, 0x0000);
    cpu.a = 0x1234;
    abi_MenuActorPoolInit();
    abi_MenuHdmaSetupB();
    cpu.x = 0x0010;
    abi_MenuActorSpawnList();
    abi_RaceStart();
    abi_MenuActorsRun();
    abi_MenuOamBuild();
    wr_m(0x0011ED, 0x0000);
    wr_m(0x0011EF, 0x0000);
    cpu.p |= P_M;
    for (i = 0; i < sizeof ppu / sizeof ppu[0]; i++)
        wr8(ppu[i].reg, ppu[i].v);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x12);
    abi_MenuScrollReset();
    cpu.p &= (uint8_t)~P_M;
    wr16(0x0000FB, 0xE3AC);
    wr16(0x0000FD, 0x001C);
    cpu.a = 0x001C;
    cpu.p &= (uint8_t)~(P_M | P_X);
    abi_MenuPaletteLoad();
    cpu.a = 0x0000;
    wr16(0x7EFD65, 0x0000);
    abi_MenuPaletteSave();
    wr16(0x0011DF, 0x0020);
    cpu.a = 0x0020;
    cpu.y = 0x0007;
    cpu.x = 0x0120;
    abi_MenuPaletteScale();
    cpu.a = 0x0020;
    wr16(0x0011DF, 0x0020);
    cpu.y = 0x0007;
    cpu.x = 0x01A0;
    abi_MenuPaletteScale();
    preview_picture(0x3000, 0xCC78);
    preview_picture(0x6800, 0xCE78);
    cpu.p &= (uint8_t)~(P_M | P_X);
    abi_MenuPictureDC8CGrid0();
    wr16(0x002116, 0x6000);
    for (x = 0; x != 0x1000; x = (uint16_t)(x + 2))
        wr16(0x002118, rd16(0x703000 + x));
    preview_picture(0x4000, 0xDD8C);
    cpu.p &= (uint8_t)~(P_M | P_X);
    cpu.x = 0x1601;
    wr16(0x0011F9, 0x1601);
    abi_MenuLoadPicturesTo7F();
    abi_MenuLoadClearGsu();
    abi_MenuLoadBlank0800();
    abi_MenuLoadBlank1000();
    abi_MenuLoadBlank1800();
    cpu.p |= P_M;
    wr8(0x000D3F, 0x20);
    wr8(0x004209, 0xE0);
    wr8(0x00420A, 0x00);
    wr8(0x0011D7, 0x0F);
    wr8(0x0010DA, 0x00);
    cpu.a = (uint16_t)(cpu.a & 0xFF00);
    abi_SoundReset();
    cpu.p |= P_M;
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x21);
    wr8(0x004200, 0x21);
    abi_MenuWaitFrame();
    abi_MenuWaitFrame();
    abi_MenuActorsRun();
    abi_MenuOamBuild();
    cpu.p |= P_M;
    wr8(0x00420C, 0x90);
    wr8(0x0011D7, (uint8_t)(rd8(0x0011D7) | 0x0F));
    abi_MenuWaitFrame();
    cpu.db = db;
    cpu.p = p;
}

/* A byte for a PPU register on the frame write list (FrameByteWritePush),
   X kept. */
static void frame_write(uint8_t v, uint16_t reg)
{
    uint16_t x = cpu.x;

    cpu.p = (uint8_t)((cpu.p | P_M) & ~P_X);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.x = reg;
    abi_FrameByteWritePush();
    cpu.x = x;
}

/* MenuPaletteScale with $11DF = `level` (a byte or a word store), X, Y. */
static void palette_scale(uint16_t level, int word, uint16_t x, uint16_t y)
{
    if (word)
        wr16(0x0011DF, level);
    else
        wr8(0x0011DF, (uint8_t)level);
    cpu.a = word ? level : (uint16_t)((cpu.a & 0xFF00) | (uint8_t)level);
    cpu.y = y;
    cpu.x = x;
    abi_MenuPaletteScale();
}

/*
 * @routine 0B:F09F MenuScreenOpenPreviewFade
 * WHAT IT DOES: Opens the menu screen with the course preview, fading in:
 * clears the menu's lists and state, makes the menu objects (spawn list
 * $10), waits a frame, turns on the IRQ at line $E0, copies 24 bytes of the
 * palette at $1C:E3AC to $7E:FF65 (clearing the same at $7E:FD65), starts
 * a race in the background (RaceStart), sets up the menu HDMA and runs the
 * objects; asks (frame write list) for BG1 at $6000, BG2 tiles at 0, colour
 * math adding the sub screen with BG1 on it, BG2 scrolled to 1 then 0, and
 * sprites, BG1 and BG2 on the main screen; then over 17 frames brings the
 * colours from 1/32 to full ($1909 counting $20 down by 2: colours 0-7 to
 * $20 - $1909, 12-14 to $1909); then the full mode: no colour math, BG2 at
 * $6800 with tiles at 0 and $6000, BG2 scrolled to $FF00, HDMA channels 4
 * and 7, BG1, BG2 and sprites on the main screen, windows on BG1 and
 * sprites; loads the palette from $1C:E3AC, saves it and scales colours
 * 144-159 and 208-223 to full; runs the objects again and asks for every
 * frame update over two frames.
 * HOW: DB = 0 throughout; the PPU writes go through FrameByteWritePush, so
 * they land in the vertical blank. The palette source is read through the
 * long pointer at direct page $FB. P and DB saved and restored.
 * Entry: D = $0000. Exit: A = 0; X, Y as MenuOamBuild left them; P and DB
 * unchanged.
 */
void abi_MenuScreenOpenPreviewFade(void)
{
    static const struct { uint8_t v; uint16_t reg; } first[] = {
        {0x61, 0x2107}, {0x70, 0x210B}, {0x01, 0x2131}, {0x02, 0x2130}, {0x02, 0x212D}, {0x00, 0x2132},
        {0x00, 0x212F}, {0x00, 0x212E}, {0x03, 0x212C}, {0x01, 0x210D}, {0x00, 0x210D}};
    static const struct { uint8_t v; uint16_t reg; } full[] = {
        {0x00, 0x2131}, {0x00, 0x2130}, {0x69, 0x2108}, {0x30, 0x210B}, {0xFF, 0x210F}, {0x00, 0x210F},
        {0x90, 0x420C}, {0x13, 0x212C}, {0xFF, 0x2127}, {0x30, 0x2123}, {0x03, 0x2125}, {0x12, 0x212E}};
    uint8_t p = cpu.p, db = cpu.db, v;
    uint16_t x, y;
    uint32_t src;
    unsigned i;

    cpu.p = (uint8_t)((cpu.p | P_M) & ~P_X);
    cpu.a = (uint16_t)(cpu.a & 0xFF00);
    cpu.db = 0x00;
    cpu.p &= (uint8_t)~(P_M | P_X);
    wr16(0x00162D, 0x0000);
    wr16(0x0016AF, 0x0000);
    wr16(0x001191, 0x0000);
    wr16(0x001193, 0x0000);
    wr16(0x0011EB, 0x0000);
    wr16(0x000D7D, 0x0006);
    wr16(0x001189, 0x1234);
    wr16(0x00118F, 0x0000);
    wr16(0x0011ED, 0x0000);
    wr16(0x0011EF, 0x0000);
    cpu.a = 0x0000;
    abi_MenuActorPoolInit();
    cpu.x = 0x0010;
    abi_MenuActorSpawnList();
    cpu.p |= P_M;
    wr8(0x0011D7, 0x00);
    abi_MenuWaitFrame();
    cpu.p |= P_M;
    wr8(0x000D3F, 0x20);
    wr8(0x004209, 0xE0);
    wr8(0x00420A, 0x00);
    cpu.p &= (uint8_t)~(P_M | P_X);
    wr16(0x0000FB, 0xE3AC);
    wr16(0x0000FD, 0x001C);
    for (x = 0, y = 0; x != 0x0018; x = (uint16_t)(x + 2), y = (uint16_t)(y + 2)) {
        src = rd8(0x0000FB);
        src |= (uint32_t)rd8(0x0000FC) << 8;
        src |= (uint32_t)rd8(0x0000FD) << 16;
        wr16(0x7EFF65 + x, rd16((src + y) & 0xFFFFFF));
        wr16(0x7EFD65 + x, 0x0000);
    }
    cpu.a = 0x0000;
    cpu.x = x;
    cpu.y = y;
    abi_RaceStart();
    abi_MenuHdmaSetupB();
    abi_MenuActorsRun();
    abi_MenuOamBuild();
    for (i = 0; i < sizeof first / sizeof first[0]; i++)
        frame_write(first[i].v, first[i].reg);
    cpu.p |= P_M;
    wr8(0x0011D7, (uint8_t)(rd8(0x0011D7) | 0x0F));
    wr8(0x001909, 0x20);
    do {
        abi_MenuWaitFrame();
        cpu.p |= P_M;
        palette_scale((uint8_t)(0x20 - rd8(0x001909)), 0, 0x0000, 0x0007);
        cpu.p |= P_M;
        palette_scale(rd8(0x001909), 0, 0x0018, 0x0003);
        cpu.p |= P_M;
        wr8(0x0011D7, (uint8_t)(rd8(0x0011D7) | 0x0B));
        v = (uint8_t)(rd8(0x001909) - 2);
        wr8(0x001909, v);
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    } while (!(v & 0x80));
    abi_MenuWaitFrame();
    for (i = 0; i < sizeof full / sizeof full[0]; i++)
        frame_write(full[i].v, full[i].reg);
    cpu.p &= (uint8_t)~P_M;
    wr16(0x0000FB, 0xE3AC);
    wr16(0x0000FD, 0x001C);
    cpu.a = 0x001C;
    cpu.p &= (uint8_t)~(P_M | P_X);
    abi_MenuPaletteLoad();
    cpu.a = 0x0000;
    wr16(0x7EFD65, 0x0000);
    abi_MenuPaletteSave();
    cpu.p &= (uint8_t)~P_M;
    palette_scale(0x0020, 1, 0x0120, 0x0007);
    cpu.p &= (uint8_t)~P_M;
    palette_scale(0x0020, 1, 0x01A0, 0x0007);
    cpu.p = (uint8_t)((cpu.p | P_M) & ~P_X);
    abi_MenuActorsRun();
    abi_MenuOamBuild();
    cpu.p |= P_M;
    wr8(0x0011D7, (uint8_t)(rd8(0x0011D7) | 0x0B));
    abi_MenuWaitFrame();
    cpu.p |= P_M;
    wr8(0x0011D7, (uint8_t)(rd8(0x0011D7) | 0x0F));
    abi_MenuWaitFrame();
    cpu.db = db;
    cpu.p = p;
}

void abi_ApproachTargetFull(void);
void abi_ApproachTargetZero(void);
void abi_SoundPort1QueuePush(void);
void abi_MenuRunFrames(void);

/* MenuPreviewRun ($0B:AFAC) and MenuPreviewRunFade ($0B:AFBC) run the
   menu's loop until the player leaves, for longer than lockstep can hold a
   call open; their C is held back until the harness can (their assembly
   is filed in menu/course_preview.asm). */

void abi_MenuSinePosition(void);
void abi_MenuActorFind(void);
void abi_MenuSet1A66(void);
void abi_MenuSetPairAnimations(void);
void abi_RaceFrameStep(void);

/* Direct page word off + X. */
static uint32_t dpx(uint8_t off)
{
    return (uint16_t)(cpu.d + off + cpu.x);
}

/*
 * @routine 0B:C5EF MenuSlideToPreview
 * WHAT IT DOES: Steps menu record X's slide +$0A by 3 up to $80 and places
 * it (MenuSinePosition); when it reaches $80 it sets the record's handler
 * +$08 to $BA4E and restarts the slide, gives kind 4's actor (MenuActorFind)
 * animation $8A (+$14; its position +$16 and timer +$18 restarted when it
 * had another), clears $18D3, sets up selection $18CB's pair (MenuSet1A66,
 * MenuSetPairAnimations), shows a race frame (RaceFrameStep with A = 0),
 * sets $18FD to 1 and clears $1191.
 * HOW: Words through direct page + X, so each step uses the X the call
 * before left. The animation change is not reached in the traces so far.
 * Entry: M 16-bit, X 16-bit, D = $0000; X = record. Exit: A = 0, Z set
 * (or, before $80, A = the slide, N, Z, C from comparing it with $80); X
 * and Y as the last call left them.
 */
void abi_MenuSlideToPreview(void)
{
    cpu.a = (uint16_t)(rd16(dpx(0x0A)) + 3);
    op_cmp16(0x0080);
    if (cpu.a >= 0x0080) {
        cpu.a = 0x0080;
        cpu.p = nz16(cpu.p, cpu.a);
    }
    wr16(dpx(0x0A), cpu.a);
    abi_MenuSinePosition();
    cpu.a = rd16(dpx(0x0A));
    op_cmp16(0x0080);
    if (cpu.a != 0x0080)
        return;
    wr16(dpx(0x08), 0xBA4E);
    wr16(dpx(0x0A), 0x0000);
    cpu.a = 0x0004;
    cpu.p = nz16(cpu.p, cpu.a);
    abi_MenuActorFind();
    cpu.a = rd16(dpx(0x14));
    op_cmp16(0x008A);
    if (cpu.a != 0x008A) {
        cpu.a = 0x008A;
        wr16(dpx(0x14), 0x008A);
        wr16(dpx(0x16), 0x0000);
        wr16(dpx(0x18), 0x0000);
    }
    cpu.a = 0x0000;
    wr16(0x0018D3, 0x0000);
    cpu.a = rd16(0x0018CB);
    cpu.p = nz16(cpu.p, cpu.a);
    abi_MenuSet1A66();
    abi_MenuSetPairAnimations();
    cpu.a = 0x0000;
    cpu.p = nz16(cpu.p, 0);
    abi_RaceFrameStep();
    cpu.a = 0x0001;
    wr16(0x0018FD, 0x0001);
    cpu.a = 0x0000;
    wr16(0x001191, 0x0000);
    cpu.p = nz16(cpu.p, 0);
}
