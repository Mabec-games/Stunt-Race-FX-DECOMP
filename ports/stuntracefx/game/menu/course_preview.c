/*
 * course_preview.c: the menu objects that run the course preview, a race
 * frame (RaceFrameStep) drawn each menu frame behind the course select.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "carried.h"
#include "wram_names.h"

void ApproachTargetFull(uint8_t bank);
void ApproachTargetZero(uint8_t bank);
uint16_t MenuChoicePressesP2(uint8_t bank, uint16_t *x, uint16_t *y, uint8_t *p);
uint8_t SaveChecksum(void);
void abi_TitleScreenRun(void);

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

void CourseMenuRun(Carried *cr);
void MenuPreviewBackEnd(Carried *cr);
void MenuPreviewBackOut(Carried *cr);
void MenuPreviewBackWait(Carried *cr);
void MenuPreviewCountdown(Carried *cr);
void MenuPreviewIdle(Carried *cr);
void MenuPreviewRun(Carried *cr);
void MenuPreviewRunFade(Carried *cr);
void MenuPreviewStart(Carried *cr);
void MenuScreenOpenPreview(Carried *cr);
void MenuScreenOpenPreviewFade(Carried *cr);
void MenuSlideToPreview(Carried *cr);
static uint32_t dpx(Carried *cr, uint8_t off);
static void frame_write(Carried *cr, uint8_t v, uint16_t reg);
static void groups_45(Carried *cr, uint16_t y);
static void hdma_on(Carried *cr, uint8_t channels);
static void kind4_animation(Carried *cr, uint16_t anim);
static void palette_scale(Carried *cr, uint16_t level, int word, uint16_t x, uint16_t y);
static uint16_t preview_frame(Carried *cr, unsigned n);
static void preview_menu_loop(Carried *cr);
static void preview_picture(Carried *cr, uint16_t vram, uint16_t pic);
static void wr_m(Carried *cr, uint32_t a, uint16_t v);
static void w_ApproachTargetFull(Carried *cr);
static void w_ApproachTargetZero(Carried *cr);
void MenuActorsRun(Carried *cr);
static void w_MenuActorsRun(Carried *cr);
static void w_MenuChoicePressesP2(Carried *cr);
static void w_SaveChecksum(Carried *cr);
void FrameByteWritePush_cr(Carried *cr);
void MenuActorFind_cr(Carried *cr);
void MenuActorFree_cr(Carried *cr);
void MenuActorPoolInit_cr(Carried *cr);
void MenuActorSpawnList_cr(Carried *cr);
void MenuActorsSet10_cr(Carried *cr);
void MenuChoiceRepeat_cr(Carried *cr);
void MenuFixedColourDown_cr(Carried *cr);
void MenuHdmaSetupB_cr(Carried *cr);
void MenuLoadBlank0800_cr(Carried *cr);
void MenuLoadBlank1000_cr(Carried *cr);
void MenuLoadBlank1800_cr(Carried *cr);
void MenuLoadClearGsu_cr(Carried *cr);
void MenuLoadPicturesTo7F_cr(Carried *cr);
void MenuOamBuild_cr(Carried *cr);
void MenuPaletteLoad_cr(Carried *cr);
void MenuPaletteSave_cr(Carried *cr);
void MenuPaletteScale_cr(Carried *cr);
void MenuPictureDC8CGrid0_cr(Carried *cr);
void MenuPictureToVram_cr(Carried *cr);
void MenuRunFrames_cr(Carried *cr);
void MenuScrollReset_cr(Carried *cr);
void MenuSet1A66_cr(Carried *cr);
void MenuSetPairAnimations_cr(Carried *cr);
void MenuSinePosition_cr(Carried *cr);
void MenuWaitFrame_cr(Carried *cr);
void RaceFrameStep(Carried *cr);
void RaceStart(Carried *cr);
void SelectionCommit_cr(Carried *cr);
void SelectionSave_cr(Carried *cr);
void SoundPort0QueuePushRaw_cr(Carried *cr);
void SoundPort0QueuePush_cr(Carried *cr);
void SoundPort1QueuePush_cr(Carried *cr);
void SoundReset_cr(Carried *cr);
void TitleScreenRun(Carried *cr);

/* The preview's frame: the fixed colour stepped down `n` times
   (MenuFixedColourDown), a race frame shown (RaceFrameStep with A = 0)
   and $18FD = 1. Returns $1905. */
static uint16_t preview_frame(Carried *cr, unsigned n)
{
    while (cyc_loop(46, n--))
        MenuFixedColourDown_cr(cr);
    cyc(8);
    cr->a = 0x0000;
    RaceFrameStep(cr);
    cr->a = 0x0001;
    cyc(56);
    wr16(W(wram_18FD), 0x0001);
    cyc(32);
    cr->a = rd16(W(menu_1905));
    set_p(cr, nz16(cr->p, cr->a));
    return cr->a;
}

/* MenuActorsSet10 for groups 4 and 5 with Y. */
static void groups_45(Carried *cr, uint16_t y)
{
    cr->a = 0x0004;
    cr->y = y;
    (cyc(94), MenuActorsSet10_cr(cr));
    cr->a = 0x0005;
    cr->y = y;
    (cyc(94), MenuActorsSet10_cr(cr));
}

/* `channels` pushed for HDMAEN ($420C) on the frame write list; X kept,
   N and Z from it (pulled back 16-bit). */
static void hdma_on(Carried *cr, uint8_t channels)
{
    uint16_t x = cr->x;

    set_p(cr, (uint8_t)(cr->p | (P_M)));
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_X)));
    cr->a = (uint16_t)((cr->a & 0xFF00) | channels);
    cr->x = 0x420C;
    (cyc(198), FrameByteWritePush_cr(cr));
    cr->x = x;
    set_p(cr, nz16(cr->p, x));  /* PLX, X 16-bit */
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cyc(208);
}

/* Animation `anim` on the kind 4 object unless it has it already. */
static void kind4_animation(Carried *cr, uint16_t anim)
{
    uint16_t x;

    cr->a = 0x0004;
    (cyc(70), MenuActorFind_cr(cr));
    x = cr->x;
    cyc(22);
    cr->a = rd16(ACTOR(x, W(actor_anim)));
    if (cr->a != anim) {
        cr->a = anim;
        cyc(86);
        wr16(ACTOR(x, W(actor_anim)), anim);
        cyc(22);
        wr16(ACTOR(x, W(actor_anim_pos)), 0x0000);
        cyc(22);
        wr16(ACTOR(x, W(actor_anim_timer)), 0x0000);
    } else { cyc(46); }
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
void MenuPreviewIdle(Carried *cr)
{
    uint16_t x;

    if (preview_frame(cr, 2))
        { cyc(58); return; }
    cyc(46);
    x = rd16(W(actor_current));
    cr->x = x;
    cr->a = 0xBCD2;
    cyc(46);
    wr16(ACTOR(x, W(actor_handler)), 0xBCD2);
    (cyc(46), MenuChoiceRepeat_cr(cr));
    cyc(74);                            /* RTS, the LDA long's fetch */
    cr->a = rd16(W(selection_prev));
    (cyc(46), MenuSet1A66_cr(cr));
    MenuSetPairAnimations_cr(cr);
    cr->a = 0x0000;
    (cyc(8), RaceFrameStep(cr));
    cyc(42);
}

void abi_MenuPreviewIdle(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuPreviewIdle(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
void MenuPreviewStart(Carried *cr)
{
    uint16_t x;

    if (preview_frame(cr, 2))
        { cyc(58); return; }
    cyc(46);
    x = rd16(W(actor_current));
    cr->x = x;
    cyc(46);
    wr16(ACTOR(x, W(actor_handler)), 0xBD55);
    cyc(46);
    wr16(ACTOR(x, W(actor_0a)), 0x0014);
    kind4_animation(cr, 0x00D8);
    groups_45(cr, 0x0000);
    hdma_on(cr, 0xC0);
}

void abi_MenuPreviewStart(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuPreviewStart(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
void MenuPreviewCountdown(Carried *cr)
{
    uint16_t x = cr->x, n, keys;

    cyc(22);
    n = rd16(ACTOR(x, W(actor_0a)));
    cr->a = n;
    if (n) {
        n = (uint16_t)(n - 1);
        cr->a = n;
        cyc(52);
        wr16(ACTOR(x, W(actor_0a)), n);
        if (n == 0x0013 || (cyc(40), n == 0x000E) || (cyc(40), n == 0x0009) || (cyc(34), n == 0x0004)) {
            cyc(108);
            set_p(cr, (uint8_t)(cr->p | (P_M)));
            cr->a = (uint16_t)((cr->a & 0xFF00) | 0x3F);
            SoundPort0QueuePushRaw_cr(cr);
            set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
        } else { cyc(22); }
        cyc(52);
        n = rd16(ACTOR(x, W(actor_0a)));
        cr->a = n;
        if (!n)
            { cyc(16); kind4_animation(cr, 0x0090); } else { cyc(22); }
    } else { cyc(22); }
    cyc(24);
    cr->x = rd16(W(actor_current));
    (cyc(46), MenuFixedColourDown_cr(cr));
    cr->a = 0x0000;
    (cyc(8), RaceFrameStep(cr));
    cr->a = 0x0001;
    cyc(56);
    wr16(W(wram_18FD), 0x0001);
    cyc(24);
    cr->x = rd16(W(actor_current));
    (cyc(46), w_MenuChoicePressesP2(cr));
    cyc(24);
    keys = rd16(W(menu_joy2_pressed));
    cr->y = keys;
    cr->a = (uint16_t)(keys & 0x4000);
    if (cr->a) {
        cyc(60);
        groups_45(cr, 0xFFFF);
        set_p(cr, (uint8_t)(cr->p | (P_M)));
        cr->a = (uint16_t)((cr->a & 0xFF00) | 0x47);
        (cyc(100), SoundPort0QueuePush_cr(cr));
        set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
        cyc(46);
        x = rd16(W(actor_current));
        cr->x = x;
        cyc(46);
        wr16(ACTOR(x, W(actor_handler)), 0xBE29);
        cyc(56);
        wr16(W(menu_1905), 0x0001);
        cyc(56);
        wr16(W(menu_1907), 0x0028);
        cr->a = 0x464A;
        set_p(cr, nz16(cr->p, cr->a));
        cyc(56);
        wr16(W(palette_word_fd65), 0x464A);
        hdma_on(cr, 0xE0);
        return;
    }
    cr->a = (uint16_t)(keys & 0x9000);
    set_p(cr, nz16(cr->p, cr->a));
    if (!cr->a)
        { cyc(150); return; }
    cr->a = 0x0001;
    cyc(162);
    wr16(W(menu_exit), 0x0001);
    cr->a = 0x0000;
    cyc(56);
    wr16(W(wram_18FD), 0x0000);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0x46);
    (cyc(100), SoundPort0QueuePush_cr(cr));
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cyc(46);
    cr->y = rd16(W(actor_current_slot));
    (cyc(46), MenuActorFree_cr(cr));
    cyc(42);
}

void abi_MenuPreviewCountdown(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuPreviewCountdown(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
static void wr_m(Carried *cr, uint32_t a, uint16_t v)
{
    if (cr->p & P_M)
        wr8(a, (uint8_t)v);
    else
        { cyc(24); wr16(a, v); }
}

/* A bank $17 picture to VRAM `vram`: its byte count is the big-endian
   word before it ($11D5), then MenuPictureToVram with A = the picture and
   X = the bank. */
static void preview_picture(Carried *cr, uint16_t vram, uint16_t pic)
{
    uint16_t n;

    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~(P_M | P_X))));
    cr->x = vram;
    cyc(64);
    ppu_wr16(0x002116, vram);
    cyc(114);
    n = rd16(0x170000 | (uint16_t)(pic - 2));
    cr->a = (uint16_t)((n << 8) | (n >> 8));
    wr16(W(menu_picture_size), cr->a);
    cr->a = pic;
    cr->x = 0x0017;
    (cyc(138), MenuPictureToVram_cr(cr));
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
void MenuScreenOpenPreview(Carried *cr)
{
    uint8_t p = cr->p, db = cr->db;
    uint16_t x;
    /* cost: clocks since the previous store (LDA #, STA, or a STZ). */
    static const struct { uint16_t reg; uint8_t v; uint8_t cost; } ppu[] = {
        {0x2105, 0x03, 62}, {0x2131, 0x00, 24}, {0x2130, 0x00, 24}, {0x2107, 0x61, 40}, {0x2108, 0x69, 40},
        {0x210B, 0x30, 40}, {0x210F, 0x00, 24}, {0x210F, 0xFF, 40}, {0x2127, 0xFF, 24}, {0x210D, 0x00, 24},
        {0x210D, 0x01, 40}, {0x212C, 0x13, 40}, {0x2123, 0x30, 40}, {0x2125, 0x03, 40}, {0x212E, 0x12, 40}};
    unsigned i;

    set_p(cr, (uint8_t)((cr->p | P_M) & ~P_X));
    cr->a = (uint16_t)(cr->a & 0xFF00);
    cr->db = 0x00;
    cyc(262);
    ppu_wr(0x002100, 0x80);
    cyc(24);
    hdmaen_wr(0x00420C, 0x00);
    cyc(24);
    mdmaen_wr(0x00420B, 0x00);
    cyc(24);
    wr8(W(wram_11DA), 0x00);
    cyc(24);
    hw_wr(0x004200, 0x00);
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~(P_M | P_X))));
    cyc(46);
    wr16(W(byte_writes_size), 0x0000);
    cyc(24);
    wr16(W(vram_queue_size), 0x0000);
    cyc(24);
    wr16(W(word_list_a_size), 0x0000);
    cyc(24);
    wr16(W(word_list_b_size), 0x0000);
    cyc(48);
    wr16(W(gsu_buffer_offset), 0x0006);
    cyc(48);
    wr16(W(wram_1189), 0x1234);
    cyc(24);
    wr16(W(wram_118F), 0x0000);
    cyc(24);
    wr16(W(menu_exit), 0x0000);
    cr->a = 0x1234;
    (cyc(62), MenuActorPoolInit_cr(cr));
    (cyc(46), MenuHdmaSetupB_cr(cr));
    cr->x = 0x0010;
    (cyc(86), MenuActorSpawnList_cr(cr));
    (cyc(62), RaceStart(cr));
    (cyc(62), w_MenuActorsRun(cr));
    (cyc(62), MenuOamBuild_cr(cr));
    cyc(0);
    wr_m(cr, W(menu_slide_pos), 0x0000);
    wr_m(cr, W(menu_scroll_y), 0x0000);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    for (i = 0; i < sizeof ppu / sizeof ppu[0]; i++)
        { cyc(ppu[i].cost); reg_wr(ppu[i].reg, ppu[i].v); }
    cyc(46);
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0x12);
    MenuScrollReset_cr(cr);
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cyc(62);
    wr16(W(dp_fb), 0xE3AC);
    cyc(40);
    wr16(W(apu_stream_bank), 0x001C);
    cr->a = 0x001C;
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~(P_M | P_X))));
    (cyc(112), MenuPaletteLoad_cr(cr));
    cr->a = 0x0000;
    cyc(56);
    wr16(W(palette_word_fd65), 0x0000);
    (cyc(46), MenuPaletteSave_cr(cr));
    cyc(48);
    wr16(W(scratch_11df), 0x0020);
    cr->a = 0x0020;
    cr->y = 0x0007;
    cr->x = 0x0120;
    (cyc(110), MenuPaletteScale_cr(cr));
    cr->a = 0x0020;
    cyc(48);
    wr16(W(scratch_11df), 0x0020);
    cr->y = 0x0007;
    cr->x = 0x01A0;
    (cyc(110), MenuPaletteScale_cr(cr));
    cyc(28);
    preview_picture(cr, 0x3000, 0xCC78);
    cyc(50);
    preview_picture(cr, 0x6800, 0xCE78);
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~(P_M | P_X))));
    (cyc(90), MenuPictureDC8CGrid0_cr(cr));
    cyc(72);
    ppu_wr16(0x002116, 0x6000);
    for (x = 0; x != 0x1000; x = (uint16_t)(x + 2))
        { cyc(32); ppu_wr16(0x002118, cyc_then(24, rd16(0x703000 + x))); cyc(74); }
    cyc(0);
    preview_picture(cr, 0x4000, 0xDD8C);
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~(P_M | P_X))));
    cr->x = 0x1601;
    cyc(114);
    wr16(W(actor_current), 0x1601);
    (cyc(46), MenuLoadPicturesTo7F_cr(cr));
    (cyc(46), MenuLoadClearGsu_cr(cr));
    (cyc(46), MenuLoadBlank0800_cr(cr));
    (cyc(46), MenuLoadBlank1000_cr(cr));
    (cyc(46), MenuLoadBlank1800_cr(cr));
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cyc(62);
    wr8(W(wram_0D3F), 0x20);
    cyc(40);
    hw_wr(0x004209, 0xE0);
    cyc(24);
    hw_wr(0x00420A, 0x00);
    cyc(40);
    wr8(W(frame_update_flags), 0x0F);
    cyc(70);
    wr8(W(wram_10DA), 0x00);
    cr->a = (uint16_t)(cr->a & 0xFF00);
    (cyc(62), SoundReset_cr(cr));
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0x21);
    cyc(62);
    hw_wr(0x004200, 0x21);
    (cyc(46), MenuWaitFrame_cr(cr));
    (cyc(46), MenuWaitFrame_cr(cr));
    (cyc(62), w_MenuActorsRun(cr));
    (cyc(62), MenuOamBuild_cr(cr));
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cyc(62);
    hdmaen_wr(0x00420C, 0x90);
    cyc(24);
    wr8(W(frame_update_flags), cyc_then(40, (uint8_t)(rd8(W(frame_update_flags)) | 0x0F)));
    (cyc(46), MenuWaitFrame_cr(cr));
    cr->db = db;
    set_p(cr, p);
    cyc(98);
}

void abi_MenuScreenOpenPreview(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuScreenOpenPreview(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* A byte for a PPU register on the frame write list (FrameByteWritePush),
   X kept. */
static void frame_write(Carried *cr, uint8_t v, uint16_t reg)
{
    cyc(76);
    uint16_t x = cr->x;

    set_p(cr, (uint8_t)((cr->p | P_M) & ~P_X));
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    cr->x = reg;
    FrameByteWritePush_cr(cr);
    cr->x = x;
}

/* MenuPaletteScale with $11DF = `level` (a byte or a word store), X, Y. */
static void palette_scale(Carried *cr, uint16_t level, int word, uint16_t x, uint16_t y)
{
    if (word)
        { cyc(48); wr16(W(scratch_11df), level); }
    else
        { cyc(24); wr8(W(scratch_11df), (uint8_t)level); }
    cyc(110);
    cr->a = word ? level : (uint16_t)((cr->a & 0xFF00) | (uint8_t)level);
    cr->y = y;
    cr->x = x;
    MenuPaletteScale_cr(cr);
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
void MenuScreenOpenPreviewFade(Carried *cr)
{
    /* lda: 0 when the block reuses A (no LDA #). */
    static const struct { uint8_t v; uint16_t reg; uint8_t lda; } first[] = {
        {0x61, 0x2107, 1}, {0x70, 0x210B, 1}, {0x01, 0x2131, 1}, {0x02, 0x2130, 1}, {0x02, 0x212D, 1},
        {0x00, 0x2132, 1}, {0x00, 0x212F, 0}, {0x00, 0x212E, 0}, {0x03, 0x212C, 1}, {0x01, 0x210D, 1},
        {0x00, 0x210D, 1}};
    static const struct { uint8_t v; uint16_t reg; uint8_t lda; } full[] = {
        {0x00, 0x2131, 1}, {0x00, 0x2130, 0}, {0x69, 0x2108, 1}, {0x30, 0x210B, 1}, {0xFF, 0x210F, 1},
        {0x00, 0x210F, 1}, {0x90, 0x420C, 1}, {0x13, 0x212C, 1}, {0xFF, 0x2127, 1}, {0x30, 0x2123, 1},
        {0x03, 0x2125, 1}, {0x12, 0x212E, 1}};
    uint8_t p = cr->p, db = cr->db, v;
    uint16_t x, y;
    uint32_t src;
    unsigned i;

    set_p(cr, (uint8_t)((cr->p | P_M) & ~P_X));
    cr->a = (uint16_t)(cr->a & 0xFF00);
    cr->db = 0x00;
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~(P_M | P_X))));
    cyc(268);
    wr16(W(byte_writes_size), 0x0000);
    cyc(24);
    wr16(W(vram_queue_size), 0x0000);
    cyc(24);
    wr16(W(word_list_a_size), 0x0000);
    cyc(24);
    wr16(W(word_list_b_size), 0x0000);
    cyc(24);
    wr16(W(menu_exit), 0x0000);
    cyc(48);
    wr16(W(gsu_buffer_offset), 0x0006);
    cyc(48);
    wr16(W(wram_1189), 0x1234);
    cyc(24);
    wr16(W(wram_118F), 0x0000);
    cyc(48);
    wr16(W(menu_slide_pos), 0x0000);
    cyc(48);
    wr16(W(menu_scroll_y), 0x0000);
    cr->a = 0x0000;
    (cyc(62), MenuActorPoolInit_cr(cr));
    cr->x = 0x0010;
    (cyc(86), MenuActorSpawnList_cr(cr));
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cyc(46);
    wr8(W(frame_update_flags), 0x00);
    (cyc(46), MenuWaitFrame_cr(cr));
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cyc(40);
    wr8(W(wram_0D3F), 0x20);
    cyc(40);
    hw_wr(0x004209, 0xE0);
    cyc(24);
    hw_wr(0x00420A, 0x00);
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~(P_M | P_X))));
    cyc(84);
    wr16(W(dp_fb), 0xE3AC);
    cyc(40);
    wr16(W(apu_stream_bank), 0x001C);
    for (x = 0, y = 0; x != 0x0018; x = (uint16_t)(x + 2), y = (uint16_t)(y + 2)) {
        cyc(108);
        src = rd8(W(dp_fb));
        src |= (uint32_t)rd8(W(dp_fc)) << 8;
        src |= (uint32_t)rd8(W(apu_stream_bank)) << 16;
        cyc(48);
        wr16(W(palette_saved) + x, rd16((src + y) & 0xFFFFFF));
        cyc(56);
        wr16(W(palette_word_fd65) + x, 0x0000);
        cyc(10);
    }
    cyc(148);
    cr->a = 0x0000;
    cr->x = x;
    cr->y = y;
    RaceStart(cr);
    (cyc(46), MenuHdmaSetupB_cr(cr));
    (cyc(62), w_MenuActorsRun(cr));
    (cyc(62), MenuOamBuild_cr(cr));
    cyc(122);
    for (i = 0; i < sizeof first / sizeof first[0]; i++)
        { frame_write(cr, first[i].v, first[i].reg);
          cyc(i + 1 == sizeof first / sizeof first[0] ? 100 : first[i + 1].lda ? 244 : 228); }
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cyc(68);
    wr8(W(frame_update_flags), cyc_then(40, (uint8_t)(rd8(W(frame_update_flags)) | 0x0F)));
    cyc(48);
    wr8(W(wram_1909), 0x20);
    do {
        cyc(46);
        MenuWaitFrame_cr(cr);
        set_p(cr, (uint8_t)(cr->p | (P_M)));
        cyc(62);
        palette_scale(cr, (uint8_t)(0x20 - rd8(W(wram_1909))), 0, 0x0000, 0x0007);
        set_p(cr, (uint8_t)(cr->p | (P_M)));
        cyc(32);
        palette_scale(cr, rd8(W(wram_1909)), 0, 0x0018, 0x0003);
        set_p(cr, (uint8_t)(cr->p | (P_M)));
        cyc(24);
        wr8(W(frame_update_flags), cyc_then(40, (uint8_t)(rd8(W(frame_update_flags)) | 0x0B)));
        cyc(32);
        v = (uint8_t)(rd8(W(wram_1909)) - 2);
        cyc(60);
        wr8(W(wram_1909), v);
        cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    } while (cyc_loop(22, !(v & 0x80)));
    cyc(62);
    MenuWaitFrame_cr(cr);
    cyc(44);
    for (i = 0; i < sizeof full / sizeof full[0]; i++)
        { cyc(!i ? 100 : full[i].lda ? 244 : 228); frame_write(cr, full[i].v, full[i].reg); }
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cyc(206);
    wr16(W(dp_fb), 0xE3AC);
    cyc(40);
    wr16(W(apu_stream_bank), 0x001C);
    cr->a = 0x001C;
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~(P_M | P_X))));
    (cyc(112), MenuPaletteLoad_cr(cr));
    cr->a = 0x0000;
    cyc(56);
    wr16(W(palette_word_fd65), 0x0000);
    (cyc(46), MenuPaletteSave_cr(cr));
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    palette_scale(cr, 0x0020, 1, 0x0120, 0x0007);
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    palette_scale(cr, 0x0020, 1, 0x01A0, 0x0007);
    set_p(cr, (uint8_t)((cr->p | P_M) & ~P_X));
    (cyc(106), w_MenuActorsRun(cr));
    (cyc(62), MenuOamBuild_cr(cr));
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cyc(24);
    wr8(W(frame_update_flags), cyc_then(40, (uint8_t)(rd8(W(frame_update_flags)) | 0x0B)));
    (cyc(46), MenuWaitFrame_cr(cr));
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cyc(24);
    wr8(W(frame_update_flags), cyc_then(40, (uint8_t)(rd8(W(frame_update_flags)) | 0x0F)));
    (cyc(46), MenuWaitFrame_cr(cr));
    cr->db = db;
    set_p(cr, p);
}

void abi_MenuScreenOpenPreviewFade(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuScreenOpenPreviewFade(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void abi_ApproachTargetFull(void);
void abi_ApproachTargetZero(void);
void abi_SoundPort1QueuePush(void);
void abi_MenuRunFrames(void);

void abi_MenuSinePosition(void);
void abi_MenuActorFind(void);
void abi_MenuSet1A66(void);
void abi_MenuSetPairAnimations(void);
void abi_RaceFrameStep(void);

/* Direct page word off + X. */
static uint32_t dpx(Carried *cr, uint8_t off)
{
    return (uint16_t)(cr->d + off + cr->x);
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
void MenuSlideToPreview(Carried *cr)
{
    cr->a = (uint16_t)(rd16(cyc_then(22, dpx(cr, 0x0A))) + 3);
    r_cmp16(cr, 0x0080);
    if (cr->a >= 0x0080) {
        cyc(18);
        cr->a = 0x0080;
        set_p(cr, nz16(cr->p, cr->a));
    }
    cyc(4);
    wr16(dpx(cr, 0x0A), cyc_then(106, cr->a));
    (cyc(46), MenuSinePosition_cr(cr));
    cr->a = rd16(cyc_then(22, dpx(cr, 0x0A)));
    r_cmp16(cr, 0x0080);
    if (cr->a != 0x0080)
        { cyc(88); return; }
    wr16(dpx(cr, 0x08), cyc_then(86, 0xBA4E));
    wr16(dpx(cr, 0x0A), cyc_then(22, 0x0000));
    cr->a = 0x0004;
    set_p(cr, nz16(cr->p, cr->a));
    (cyc(70), MenuActorFind_cr(cr));
    cr->a = rd16(cyc_then(22, dpx(cr, 0x14)));
    r_cmp16(cr, 0x008A);
    if (cr->a != 0x008A) {
        cr->a = 0x008A;
        wr16(dpx(cr, 0x14), cyc_then(86, 0x008A));
        wr16(dpx(cr, 0x16), cyc_then(22, 0x0000));
        wr16(dpx(cr, 0x18), cyc_then(22, 0x0000));
    } else { cyc(46); }
    cr->a = 0x0000;
    cyc(56);
    wr16(W(selection_out), 0x0000);
    cyc(32);
    cr->a = rd16(W(selection));
    set_p(cr, nz16(cr->p, cr->a));
    (cyc(46), MenuSet1A66_cr(cr));
    MenuSetPairAnimations_cr(cr);
    cr->a = 0x0000;
    set_p(cr, nz16(cr->p, 0));
    (cyc(8), RaceFrameStep(cr));
    cr->a = 0x0001;
    cyc(56);
    wr16(W(wram_18FD), 0x0001);
    cr->a = 0x0000;
    cyc(56);
    wr16(W(word_list_a_size), 0x0000);
    set_p(cr, nz16(cr->p, 0));
    cyc(42);
}

void abi_MenuSlideToPreview(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuSlideToPreview(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* The course preview menu's loop: each frame (MenuWaitFrame) $18FD = 0,
   the blink counter $11DD up, the objects run and the sprite table built,
   both players' presses cleared and frame updates 0-3 asked for, until
   the menu asks to leave ($11EB); then sound $04 on port 1, the fade
   target to 0 (ApproachTargetZero) and the menu frames run out
   (MenuRunFrames). */
static void preview_menu_loop(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;

    do {
        cyc(68);
        MenuWaitFrame_cr(cr);
        set_p(cr, (uint8_t)(cr->p & ((uint8_t)~(P_M | P_X))));
        cr->a = 0x0000;
        cyc(56);
        wr16(W(wram_18FD), 0x0000);
        cyc(24);
        wr16(b | W(menu_blink_counter), cyc_then(14, (uint16_t)(rd16(b | W(menu_blink_counter)) + 1)));
        (cyc(62), w_MenuActorsRun(cr));
        (cyc(62), MenuOamBuild_cr(cr));
        set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
        cyc(46);
        wr16(b | W(menu_joy1_pressed), 0x0000);
        cyc(24);
        wr16(b | W(menu_joy2_pressed), 0x0000);
        set_p(cr, (uint8_t)(cr->p | (P_M)));
        cyc(46);
        cr->a = (uint16_t)((cr->a & 0xFF00) | (uint8_t)(rd8(b | W(frame_update_flags)) | 0x0F));
        cyc(40);
        wr8(b | W(frame_update_flags), (uint8_t)cr->a);
        set_p(cr, (uint8_t)(cr->p & ((uint8_t)~(P_M | P_X))));
        cyc(46);
        cr->a = rd16(b | W(menu_exit));
    } while (!cr->a);
    cyc(116);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0x04);
    SoundPort1QueuePush_cr(cr);
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    (cyc(68), w_ApproachTargetZero(cr));
    (cyc(46), MenuRunFrames_cr(cr));
}

/*
 * @routine 0B:AFAC MenuPreviewRun
 * WHAT IT DOES: Runs the course preview menu: opens it
 * (MenuScreenOpenPreview), waits a frame, sets the fade target to full
 * (ApproachTargetFull), then runs the menu's loop until it asks to leave.
 * HOW: Joins MenuPreviewRunFade's loop.
 * Entry: D = $0000. Exit: as MenuRunFrames leaves it.
 */
void MenuPreviewRun(Carried *cr)
{
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~(P_M | P_X))));
    MenuScreenOpenPreview(cr);
    (cyc(46), MenuWaitFrame_cr(cr));
    (cyc(46), w_ApproachTargetFull(cr));
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~(P_M | P_X))));
    (cyc(24), preview_menu_loop(cr));
    cyc(44);                            /* RTL */
}

void abi_MenuPreviewRun(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuPreviewRun(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:AFBC MenuPreviewRunFade
 * WHAT IT DOES: Runs the course preview menu, fading it in
 * (MenuScreenOpenPreviewFade) with every frame update on ($11DA = $11DB =
 * $0F), then the menu's loop until it asks to leave ($11EB): each frame
 * the objects run, presses are cleared and the blink counter steps; at
 * the end sound $04 on port 1, the fade target to 0 and the menu frames
 * run out.
 * HOW: The loop is shared with MenuPreviewRun.
 * Entry: D = $0000. Exit: as MenuRunFrames leaves it.
 */
void MenuPreviewRunFade(Carried *cr)
{
    uint32_t b;

    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~(P_M | P_X))));
    MenuScreenOpenPreviewFade(cr);
    b = (uint32_t)cr->db << 16;
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0x0F);
    cyc(160);
    wr8(b | W(wram_11DA), 0x0F);
    cyc(24);
    wr8(b | W(wram_11DB), 0x0F);
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~(P_M | P_X))));
    preview_menu_loop(cr);
    cyc(44);                            /* RTL */
}

void abi_MenuPreviewRunFade(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuPreviewRunFade(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void abi_SelectionSave(void);
void abi_SelectionCommit(void);
void abi_SaveChecksum(void);

/*
 * @routine 03:B3DA CourseMenuRun
 * WHAT IT DOES: The course menu before a race: clears side $1F07, the
 * selection $18C3 and $1168; the first time ($1917 clear) 0B:AEF2's screen
 * first, then the course preview, faded in (MenuPreviewRunFade) when that
 * screen returned 0, else MenuPreviewRun; then the HUD mode $0D2B = the
 * selection, $1917 = 1, the selection kept (SelectionCommit for choice 2,
 * else SelectionSave), the three settings $18EF, $18F1, $18F3 at $1184-$1186
 * and in the save at $70:FC08-$70:FC0A (SaveChecksum); $0D3F = $1C, the NMI
 * and HDMA off and the screen blanked.
 * HOW: Through the data bank, long accesses to $18C3-$18F3 and $1917; P
 * saved and put back.
 * Entry: any widths. Exit: P unchanged; A low byte $80.
 */
void CourseMenuRun(Carried *cr)
{
    uint8_t p0 = cr->p;
    uint32_t b = (uint32_t)cr->db << 16;
    static const uint16_t setting[3] = {0x18EF, 0x18F1, 0x18F3};
    int i;

    r_sep(cr, P_M);
    cr->a = (uint16_t)(cr->a & 0xFF00);
    set_p(cr, nz8(cr->p, 0x00));
    cyc(84);
    wr8(b | W(draw_side), 0x00);
    r_rep(cr, P_M);
    cyc(24);
    wr16(b | W(wram_18C3), 0x0000);
    cyc(24);
    wr16(b | W(frame_step), 0x0000);
    cyc(32);
    r_lda16(cr, W(wram_1917));
    if (!cr->a) {
        TitleScreenRun(cr);
        r_rep(cr, P_M);
        r_cmp16(cr, 0x0000);
        if (cr->p & P_Z) {
            cyc(108);
            MenuPreviewRunFade(cr);
            r_rep(cr, P_M);
            b = (uint32_t)cr->db << 16;
            cyc(32);
            goto chosen;
        }
    } else { cyc(84); }
    MenuPreviewRun(cr);
    r_rep(cr, P_M);
    b = (uint32_t)cr->db << 16;
    cyc(32);
chosen:
    r_lda16(cr, W(wram_18C3));
    cyc(24);
    wr16(b | W(hud_mode), cr->a);
    r_lda16_imm(cr, 0x0001);
    cyc(18);
    wr16(W(wram_1917), 0x0001);
    cyc(32);
    r_lda16(cr, W(wram_18C3));
    r_cmp16(cr, 0x0002);
    if (cr->p & P_Z)
        { cyc(108); SelectionCommit_cr(cr); }
    else
        SelectionSave_cr(cr);
    cyc(22);
    r_sep(cr, P_M);
    b = (uint32_t)cr->db << 16;
    for (i = 0; i < 3; i++) {
        cyc(32);
        r_lda8(cr, setting[i]);
        cyc(24);
        wr8(b | (uint16_t)(0x1184 + i), (uint8_t)cr->a);
        cyc(32);
        wr8(0x70FC08u + i, (uint8_t)cr->a);
    }
    cyc(62);
    w_SaveChecksum(cr);
    r_sep(cr, P_M);
    b = (uint32_t)cr->db << 16;
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0x1C);
    set_p(cr, nz8(cr->p, 0x1C));
    cyc(62);
    wr8(b | W(wram_0D3F), 0x1C);
    cyc(24);
    hw_wr(b | 0x4200, 0x00);
    cyc(24);
    hdmaen_wr(b | 0x420C, 0x00);
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0x80);
    set_p(cr, nz8(cr->p, 0x80));
    cyc(40);
    ppu_wr(b | 0x2100, 0x80);
    r_plp(cr, p0);
    cyc(30);
}

void abi_CourseMenuRun(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CourseMenuRun(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:BE29 MenuPreviewBackOut
 * WHAT IT DOES: The course preview after player 2 backs out with Y
 * (MenuPreviewCountdown's $4000 case): two fixed colour steps down, a race
 * frame shown, $18FD = 1; once $1905 is clear, makes $BE65
 * (MenuPreviewBackWait) the object's handler, clears the selection out
 * ($18D3), sets $1A66 and the pair's animations from the selection ($18CB)
 * and shows another race frame.
 * HOW: Calls in turn; the object is the current one ($11F9). No play
 * reaches it; unit_preview_back_0b calls it.
 * Entry: M 16-bit, X 16-bit, D = $0000. Exit: as the last call leaves it.
 */
void MenuPreviewBackOut(Carried *cr)
{
    uint16_t x;

    if (preview_frame(cr, 2))
        { cyc(58); return; }
    cyc(46);
    x = rd16((uint32_t)cr->db << 16 | 0x11F9);
    cr->x = x;
    cr->a = 0xBE65;
    cyc(46);
    wr16(ACTOR(x, W(actor_handler)), 0xBE65);
    cr->a = 0x0000;
    cyc(56);
    wr16(W(selection_out), 0x0000);
    cyc(32);
    cr->a = rd16(W(selection));
    (cyc(46), MenuSet1A66_cr(cr));
    MenuSetPairAnimations_cr(cr);
    cr->a = 0x0000;
    (cyc(8), RaceFrameStep(cr));
    cyc(42);
}

void abi_MenuPreviewBackOut(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuPreviewBackOut(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:BE65 MenuPreviewBackWait
 * WHAT IT DOES: Waits for the byte $0357; once it is set, $1905 = 2, $1907
 * = 0 and the object's handler becomes $BE84 (MenuPreviewBackEnd).
 * HOW: An 8-bit test of $0357, then 16-bit long stores.
 * Entry: M 16-bit, X 16-bit, D = $0000; X = the object. Exit: M 16-bit;
 * waiting: A low byte = 0, Z set; else A = $BE84, N set.
 */
void MenuPreviewBackWait(Carried *cr)
{
    uint8_t v;

    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cyc(46);
    v = rd8((uint32_t)cr->db << 16 | 0x0357);
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    set_p(cr, nz8(cr->p, v));
    if (v) {
        set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
        cr->a = 0x0002;
        cyc(94);
        wr16(W(menu_1905), 0x0002);
        cr->a = 0x0000;
        cyc(56);
        wr16(W(menu_1907), 0x0000);
        cr->a = 0xBE84;
        set_p(cr, nz16(cr->p, 0xBE84));
        cyc(46);
        wr16(ACTOR(cr->x, 0x08), 0xBE84);
    } else { cyc(22); }
    cyc(64);
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
}

void abi_MenuPreviewBackWait(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuPreviewBackWait(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:BE84 MenuPreviewBackEnd
 * WHAT IT DOES: The last frame of backing out of the preview: two fixed
 * colour steps down, a race frame shown, $18FD = 1; once $1905 is clear,
 * the object's handler becomes $BA4E with a count of 20 (+$0A), the kind 4
 * object gets animation $CE (unless it has it), +$10 of groups 4 and 5 is
 * cleared (MenuActorsSet10 with Y = 0), the word list's size $1191 = 0 and
 * HDMA channels 6 and 7 are asked for ($C0 to HDMAEN on the frame write
 * list).
 * HOW: As MenuPreviewStart.
 * Entry: M 16-bit, X 16-bit, D = $0000. Exit: as FrameByteWritePush
 * leaves it, with X the kind 4 object and M 16-bit.
 */
void MenuPreviewBackEnd(Carried *cr)
{
    uint16_t x;

    if (preview_frame(cr, 2))
        { cyc(58); return; }
    cyc(46);
    x = rd16((uint32_t)cr->db << 16 | 0x11F9);
    cr->x = x;
    cyc(46);
    wr16(ACTOR(x, W(actor_handler)), 0xBA4E);
    cyc(46);
    wr16(ACTOR(x, W(actor_0a)), 0x0014);
    kind4_animation(cr, 0x00CE);
    groups_45(cr, 0x0000);
    cr->a = 0x0000;
    cyc(56);
    wr16(W(word_list_a_size), 0x0000);
    hdma_on(cr, 0xC0);
}

void abi_MenuPreviewBackEnd(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuPreviewBackEnd(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* MenuChoicePressesP2 as its adapter leaves the registers. */
static void w_MenuChoicePressesP2(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = MenuChoicePressesP2(cr->db, &cr->x, &cr->y, &p);
    set_p(cr, p);
}

/* MenuActorsRun on the carried registers. */
static void w_MenuActorsRun(Carried *cr)
{
    MenuActorsRun(cr);
}

/* ApproachTargetZero as its adapter leaves the registers. */
static void w_ApproachTargetZero(Carried *cr)
{
    ApproachTargetZero(cr->db);
}

/* ApproachTargetFull as its adapter leaves the registers. */
static void w_ApproachTargetFull(Carried *cr)
{
    ApproachTargetFull(cr->db);
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0x0F);
}

/* SaveChecksum as its adapter leaves the registers. */
static void w_SaveChecksum(Carried *cr)
{
    cr->a = (uint16_t)((cr->a & 0xFF00) | SaveChecksum());
}
