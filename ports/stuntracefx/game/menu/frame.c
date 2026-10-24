/*
 * frame.c: the menu's frame handshake. The menu code sets bit 0 of $11D8
 * when it has finished a frame; the menu's vertical blank code clears $11D8
 * once it has applied the frame's updates.
 */
#include "machine.h"
#include "native.h"
#include "hw.h"
#include "cpu_ops.h"
#include "menu_calls.h"
#include "carried.h"
#include "wram_names.h"

void abi_MenuActorsRun(void);

/*
 * @routine 0B:E2D7 MenuFramesTarget7
 * WHAT IT DOES: Sets the frame update target $11DB (MenuRunFrames runs
 * frames until $11DA reaches it) to 7. Nothing calls it, so a direct call
 * is its only way in.
 * HOW: An 8-bit store through the data bank between PHP and PLP.
 * Entry: any widths. Exit: A low byte = 7; P unchanged.
 */
uint8_t MenuFramesTarget7(uint8_t bank)
{
    (void)bank;
    cyc(84);
    wr8(0x7E0000 | W(wram_11DB), 0x07);
    cyc(70);                            /* PLP, RTS */
    return 0x07;
}

void MenuDimOnL_cr(Carried *cr);
void MenuFramesTarget7_cr(Carried *cr);
void MenuHeldRestart_cr(Carried *cr);
void MenuRandomStep_cr(Carried *cr);
void MenuRegionCheck_cr(Carried *cr);
void MenuRegionPal_cr(Carried *cr);
void MenuRunFrames_cr(Carried *cr);
void MenuScreen0Run_cr(Carried *cr);
void MenuWaitFrame_cr(Carried *cr);
uint16_t MenuHeldRestart(Carried *cr, uint8_t bank, uint16_t *x, uint16_t *y, uint8_t *p);
uint8_t MenuRegionCheck(Carried *cr, uint16_t a, uint16_t x);
void MenuRegionPal(Carried *cr, uint16_t a, uint16_t x);
uint16_t MenuRunFrames(Carried *cr, uint8_t bank, uint8_t p, uint16_t *x, uint16_t *y);
uint16_t MenuScreen0Run(Carried *cr, uint8_t bank, uint16_t a, uint16_t *x, uint16_t *y, uint8_t *p);
static void menu_frame(Carried *cr, uint8_t bank, uint8_t p, uint16_t *x, uint16_t *y);
static void run_actors(Carried *cr, uint8_t bank, uint16_t a, uint8_t p, uint16_t *x, uint16_t *y);
static void w_MenuActorsRun(Carried *cr);

void MenuFramesTarget7_cr(Carried *cr)
{
    cr->a = (uint16_t)((cr->a & 0xFF00) | MenuFramesTarget7(cr->db));
}

void abi_MenuFramesTarget7(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuFramesTarget7_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:E3A3 MenuRandomStep
 * WHAT IT DOES: Steps the menu's random word $1189 (set to $1234 when a
 * screen opens): the word with its bytes swapped and halved, plus the word
 * twice, XORed with the blink counter $11DD. Nothing calls it, so a direct
 * call is its only way in.
 * HOW: The two bytes read in 8-bit with an XBA between; then 16-bit LSR,
 * CLC and two ADCs (the second keeps the first's carry), EOR; through the
 * data bank, between PHP and PLP.
 * Entry: any widths. Exit: A = the new word; P unchanged.
 */
uint16_t MenuRandomStep(uint8_t bank, uint8_t p)
{
    (void)bank;
    uint16_t a;

    cyc(68);
    a = (uint16_t)(rd8(0x7E0000 | W(menu_random)) << 8);
    cyc(44);
    a = (uint16_t)(a | rd8(0x7E0000 | W(wram_118A)));
    a = (uint16_t)(a >> 1);
    p &= (uint8_t)~P_C;
    cyc(74);
    a = alu_adc16(a, rd16(0x7E0000 | W(menu_random)), &p);
    cyc(24);
    a = alu_adc16(a, rd16(0x7E0000 | W(menu_random)), &p);
    cyc(24);
    a ^= rd16(0x7E0000 | W(menu_blink_counter));
    cyc(24);
    wr16(0x7E0000 | W(menu_random), a);
    cyc(70);                            /* PLP, RTS */
    return a;
}

void MenuRandomStep_cr(Carried *cr)
{
    cr->a = MenuRandomStep(cr->db, cr->p);
}

void abi_MenuRandomStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuRandomStep_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:E3BF MenuDimOnL
 * WHAT IT DOES: When the press being worked on ($118F) has the L button
 * (bit 5), sets the screen brightness to 4 (INIDISP). Nothing calls it, so
 * a direct call is its only way in.
 * HOW: AND #$0020; the brightness written in 8-bit with a long store.
 * Entry: M 16-bit. Exit: no L: A = 0; Z set, N clear. L: A = 4; N, Z
 * clear; M 16-bit.
 */
uint16_t MenuDimOnL(uint8_t bank)
{
    (void)bank;
    uint16_t a;

    cyc(24);
    a = (uint16_t)(rd16(0x7E0000 | W(menu_press)) & 0x0020);
    if (!a)
        { cyc(88); return 0x0000; }    /* AND, BEQ taken, RTS */
    cyc(110);
    ppu_wr(INIDISP, 0x04);
    cyc(64);                            /* REP, RTS */
    return 0x0004;
}

void MenuDimOnL_cr(Carried *cr)
{
    cr->a = MenuDimOnL(cr->db);
    set_p(cr, nz16(cr->p, cr->a));
}

void abi_MenuDimOnL(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuDimOnL_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:E390 MenuWaitFrame
 * WHAT IT DOES: Marks the frame finished and waits for the vertical blank
 * code to take it.
 * HOW: Sets bit 0 of $11D8 (16-bit, through the data bank) and polls it
 * until it is zero.
 * Entry: any widths. Exit: A = 0; P unchanged.
 */
void MenuWaitFrame(uint8_t bank)
{
    (void)bank;
    uint32_t a = 0x7E0000 | W(menu_frame_wait);

    cyc(68);
    wr16(a, cyc_then(48, (uint16_t)(rd16(a) | 0x0001)));
    while (rd16(cyc_then(24, a)))
        { cyc(22); ; }
    cyc(86);
}

void MenuWaitFrame_cr(Carried *cr)
{
    MenuWaitFrame(cr->db);
    cr->a = 0x0000;
}

void abi_MenuWaitFrame(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuWaitFrame_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void MenuActorsRun(Carried *cr);
uint16_t MenuOamBuild(uint16_t *x, uint16_t *y);
void MenuScreenOpen0(Carried *cr);

/* The objects run (MenuActorsRun) with A, P = p and the
   data bank going in, X passing through when there are none; the X and Y
   they leave come back in *x and *y. */
static void run_actors(Carried *cr, uint8_t bank, uint16_t a, uint8_t p, uint16_t *x, uint16_t *y)
{
    cr->db = bank;
    set_p(cr, p);
    cr->a = a;
    cr->x = *x;
    cr->y = *y;
    (cyc(62), w_MenuActorsRun(cr));
    *x = cr->x;
    *y = cr->y;
}

/*
 * @routine 0B:E2E1 MenuRunFrames
 * WHAT IT DOES: Runs menu frames until $11DA equals $11DB: each frame waits
 * for the vertical blank (MenuWaitFrame), runs the objects
 * (MenuActorsRun), builds the sprite table (MenuOamBuild) and asks for
 * every frame update (bits 0-4 of $11D7).
 * HOW: 8-bit A for the flags and the compare; P saved and restored.
 * Entry: any widths; p = P, *x and *y = X and Y. Exit: A low byte = $11DA,
 * high byte as MenuOamBuild left it (returned); *x, *y as MenuOamBuild left
 * them; P unchanged.
 */
uint16_t MenuRunFrames(Carried *cr, uint8_t bank, uint8_t p, uint16_t *x, uint16_t *y)
{
    uint16_t a;
    uint8_t m;

    cyc(22);
    do {
        cyc(68);
        MenuWaitFrame(bank);
        run_actors(cr, bank, 0x0000, (uint8_t)(p | P_M), x, y);
        cyc(62);
        a = MenuOamBuild(x, y);
        cyc(46);
        wr8(0x7E0000 | W(frame_update_flags), cyc_then(40, (uint8_t)(rd8(0x7E0000 | W(frame_update_flags)) | 0x1F)));
        cyc(24);
        m = rd8(0x7E0000 | W(wram_11DA));
    } while (m != rd8(cyc_then(24, 0x7E0000 | W(wram_11DB))));
    cyc(86);
    return (uint16_t)((a & 0xFF00) | m);
}

void MenuRunFrames_cr(Carried *cr)
{
    uint16_t x = cr->x, y = cr->y;
    uint8_t p = cr->p;

    cr->a = MenuRunFrames(cr, cr->db, p, &x, &y);
    cr->x = x;
    cr->y = y;
    set_p(cr, p);
}

void abi_MenuRunFrames(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuRunFrames_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

int MenuActorFind(uint8_t bank, uint16_t kind, uint16_t *x, uint16_t *y, uint8_t *p);

/* MenuActorFind as it runs with an 8-bit accumulator: the kind is stored
   and compared as a byte with the low byte of +$1A. Returns the record (or
   whatever X the search left). */
static uint16_t actor_find8(uint8_t kind, uint16_t x)
{
    uint16_t i;

    cyc(86);
    wr8(W(scratch_11e1), kind);
    cyc(24);
    for (i = (uint16_t)(rd16(W(actor_list_len)) - 2); !(i & 0x8000); i = (uint16_t)(i - 2)) {
        cyc(74);
        x = rd16(W(actor_list) + i);
        cyc(22);
        if (rd8((uint16_t)(x + W(actor_kind))) == rd8(cyc_then(24, W(scratch_11e1))))
            break; else { cyc(28); }
    }
    return x;
}

/* One menu frame as the screen loops run it: wait for the vertical blank,
   run the objects (with P = p), build the sprite table and ask for frame
   updates 0-3. */
static void menu_frame(Carried *cr, uint8_t bank, uint8_t p, uint16_t *x, uint16_t *y)
{
    uint8_t f;

    cyc(46);
    MenuWaitFrame(bank);
    run_actors(cr, bank, 0x0000, p, x, y);
    cyc(62);
    MenuOamBuild(x, y);
    cyc(46);
    f = (uint8_t)(rd8(0x7E0000 | W(frame_update_flags)) | 0x0F);
    cyc(40);
    wr8(0x7E0000 | W(frame_update_flags), f);
}

/*
 * @routine 0B:AE58 MenuRegionPal
 * WHAT IT DOES: MenuRegionCheck's stop on a PAL console, which no NTSC
 * run reaches: the region screen's objects set (animation $1E on the
 * kind $0F object at Y $60, +$10 of the kind $10 object cleared), one
 * frame's objects and sprites, then a loop for ever.
 * HOW: With DB = 0: the kind $0F object's animation $1E at Y $60, +$10 of
 * the kind $10 object cleared, the objects run and the sprite table built
 * once, frame updates 0-3 asked for, then the branch to itself at 0B:AE8A
 * for ever.
 * Entry: M 8-bit, X 16-bit (x), D = $0000; DB = 0; P and DB on the stack;
 * a = A (its high byte reaches the objects).
 * Exit: does not return.
 */
void MenuRegionPal(Carried *cr, uint16_t a, uint16_t x)
{
    uint16_t y = 0;

    x = actor_find8(0x0F, x);
    cyc(80);
    if (rd8((uint16_t)(x + W(actor_anim))) != 0x1E) {
        cyc(70);
        wr8((uint16_t)(x + W(actor_anim)), 0x1E);
        cyc(22);
        wr8((uint16_t)(x + W(actor_anim_pos)), 0x00);
        cyc(22);
        wr8((uint16_t)(x + W(actor_anim_timer)), 0x00);
    }
    cyc(38);
    wr8((uint16_t)(x + W(actor_06)), 0x60);
    x = actor_find8(0x10, x);
    cyc(80);
    wr8((uint16_t)(x + W(actor_10)), 0x00);
    y = cr->y;
    run_actors(cr, 0x00, a, cr->p, &x, &y);
    cyc(62);
    cr->a = MenuOamBuild(&x, &y);
    cr->x = x;
    cr->y = y;
    cyc(46);
    wr8(W(frame_update_flags), cyc_then(40, (uint8_t)(rd8(W(frame_update_flags)) | 0x0F)));
    carried_loop_forever(cr, 0x0BAE8A);
}

void MenuRegionPal_cr(Carried *cr)
{
    MenuRegionPal(cr, cr->a, cr->x);
}

void abi_MenuRegionPal(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuRegionPal_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:AE47 MenuRegionCheck
 * WHAT IT DOES: Stops the game on a PAL console (bit 4 of STAT78): starts
 * animation $1E on the kind $0F object and puts it at Y $60, clears +$10
 * of the kind $10 object, runs the objects once, builds the sprite table,
 * asks for frame updates 0-3 and then loops for ever (MenuRegionPal). On
 * NTSC it returns at once; no script runs on PAL.
 * HOW: DB = 0 and 8-bit A throughout, so the searches compare and the
 * field writes store single bytes. The loop is a branch to itself without
 * waiting for a frame, so the frame updates are never taken.
 * Entry: X 16-bit (x); a = A (used on PAL). Exit: (NTSC) A low byte = 0 (returned); X, Y
 * unchanged; P and DB unchanged.
 */
uint8_t MenuRegionCheck(Carried *cr, uint16_t a, uint16_t x)
{
    uint8_t s;

    cyc(178);
    s = hw_rd(STAT78) & 0x10;
    if (!s) {
        cyc(136);
        return 0x00;
    }
    cr->db = 0x00;
    MenuRegionPal(cr, a, x);
    return s;
}

void MenuRegionCheck_cr(Carried *cr)
{
    cr->a = (uint16_t)((cr->a & 0xFF00) | MenuRegionCheck(cr, cr->a, cr->x));
}

void abi_MenuRegionCheck(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuRegionCheck_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* Any of bits 0-3 of either player's held buttons ($0309, $030D), or
   $18FB set. */
static int held_or_18fb(void)
{
    cyc(46);
    return (rd16(0x7E0000 | W(joy1_held)) & 0x000F) || (rd16(cyc_then(64, 0x7E0000 | W(joy2_held))) & 0x000F) || rd16(cyc_then(72, W(wram_18FB)));
}

/*
 * @routine 0B:AE8F MenuHeldRestart
 * WHAT IT DOES: Returns 0 when no player holds a button in bits 0-3 and
 * $18FB is clear. Otherwise starts animation $18 on the kind $0F object and
 * runs menu frames until that is so, then returns $FFFF (the caller opens
 * its screen again). No script holds those buttons here.
 * HOW: The held words are read through the data bank; the frames are as
 * MenuRunFrames's, with frame updates 0-3.
 * Entry: X 16-bit, D = $0000; *x, *y = X, Y; *p = P. Exit: A = 0 or $FFFF
 * (returned); N, Z from it; M 16-bit, X 16-bit; X, Y unchanged when 0, as
 * the last frame left them when $FFFF, C then from the compare of the
 * animation with $18 (*p).
 */
uint16_t MenuHeldRestart(Carried *cr, uint8_t bank, uint16_t *x, uint16_t *y, uint8_t *p)
{
    uint16_t a, rec = *x;
    uint8_t found = *p, now;

    now = (uint8_t)(*p & ~(P_M | P_X));
    if (!held_or_18fb()) {
        *p = nz16(now, 0x0000);
        cyc(82);
        return 0x0000;
    } else { cyc(116); }
    MenuActorFind(bank, 0x000F, &rec, y, &found);
    cyc(22);
    a = rd16((uint16_t)(rec + W(actor_anim)));
    found = (uint8_t)((found & ~P_C) | (a >= 0x0018 ? P_C : 0));
    if (a != 0x0018) {
        cyc(86);
        wr16((uint16_t)(rec + W(actor_anim)), 0x0018);
        cyc(22);
        wr16((uint16_t)(rec + W(actor_anim_pos)), 0x0000);
        cyc(22);
        wr16((uint16_t)(rec + W(actor_anim_timer)), 0x0000);
    }
    *x = rec;
    now = (uint8_t)(found & ~(P_M | P_X));
    do {
        menu_frame(cr, bank, now, x, y);
    } while (held_or_18fb());
    cyc(82);
    *p = nz16(now, 0xFFFF);
    return 0xFFFF;
}

void MenuHeldRestart_cr(Carried *cr)
{
    uint16_t x = cr->x, y = cr->y;
    uint8_t p = cr->p;

    cr->a = MenuHeldRestart(cr, cr->db, &x, &y, &p);
    cr->x = x;
    cr->y = y;
    set_p(cr, p);
}

void abi_MenuHeldRestart(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuHeldRestart_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:AE0A MenuScreen0Run
 * WHAT IT DOES: Runs the menu screen of spawn list 0: opens it
 * (MenuScreenOpen0), runs frames until the menu asks to exit ($11EB), checks
 * the region (MenuRegionCheck), opens it again while buttons are held
 * (MenuHeldRestart), then turns the display, interrupts, HDMA and DMA off.
 * HOW: Frames as MenuRunFrames's, with frame updates 0-3.
 * Entry: D = $0000; a, *x, *y, *p = A, X, Y, P. Exit: A = $0080
 * (returned); X, Y as the last frame left them; M 8-bit, X 16-bit; N set,
 * Z clear; C clear.
 */
uint16_t MenuScreen0Run(Carried *cr, uint8_t bank, uint16_t a, uint16_t *x, uint16_t *y, uint8_t *p)
{
    uint32_t b;

    do {
        *p &= (uint8_t)~(P_M | P_X);
        cr->a = a;
        cr->x = *x;
        cr->y = *y;
        set_p(cr, *p);
        cr->db = bank;
        MenuScreenOpen0(cr);
        a = cr->a;
        *x = cr->x;
        *y = cr->y;
        *p = cr->p;
        bank = cr->db;
        cyc(76);
        do {
            cyc(22);
            menu_frame(cr, bank, *p, x, y);
            *p &= (uint8_t)~(P_M | P_X);
            b = (uint32_t)bank << 16;
            cyc(46);
            a = rd16(0x7E0000 | W(menu_exit));
        } while (!a);
        cyc(62);
        MenuRegionCheck(cr, a, *x);
        a = (cyc(46), MenuHeldRestart(cr, bank, x, y, p));
    } while (a == 0xFFFF);
    b = (uint32_t)bank << 16;
    cyc(102);
    ppu_wr(b | (uint16_t)INIDISP, 0x80);
    cyc(24);
    hw_wr(b | (uint16_t)NMITIMEN, 0x00);
    cyc(24);
    hdmaen_wr(b | (uint16_t)HDMAEN, 0x00);
    cyc(24);
    mdmaen_wr(b | (uint16_t)MDMAEN, 0x00);
    *p = (uint8_t)((*p & ~(P_Z | P_C)) | P_N | P_M);
    cyc(44);
    return 0x0080;
}

void MenuScreen0Run_cr(Carried *cr)
{
    uint16_t x = cr->x, y = cr->y;
    uint8_t p = cr->p;

    cr->a = MenuScreen0Run(cr, cr->db, cr->a, &x, &y, &p);
    cr->x = x;
    cr->y = y;
    set_p(cr, p);
}

void abi_MenuScreen0Run(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuScreen0Run_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* MenuActorsRun on the carried registers. */
static void w_MenuActorsRun(Carried *cr)
{
    MenuActorsRun(cr);
}

