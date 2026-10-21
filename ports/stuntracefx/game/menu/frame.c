/*
 * frame.c: the menu's frame handshake. The menu code sets bit 0 of $11D8
 * when it has finished a frame; the menu's vertical blank code clears $11D8
 * once it has applied the frame's updates.
 */
#include "machine.h"
#include "native.h"
#include "hw.h"
#include "cpu_ops.h"

/*
 * @routine 0B:E2D7 MenuFramesTarget7
 * WHAT IT DOES: Sets the frame update target $11DB (MenuRunFrames runs
 * frames until $11DA reaches it) to 7. Nothing calls it, so a direct call
 * is its only way in.
 * HOW: An 8-bit store through the data bank between PHP and PLP.
 * Entry: any widths. Exit: A low byte = 7; P unchanged.
 */
void abi_MenuFramesTarget7(void)
{
    cyc(84);
    wr8((uint32_t)cpu.db << 16 | 0x11DB, 0x07);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x07);
    cyc(70);                            /* PLP, RTS */
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
void abi_MenuRandomStep(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint8_t p = cpu.p;
    uint16_t a;

    cyc(68);
    a = (uint16_t)(rd8(b | 0x1189) << 8);
    cyc(44);
    a = (uint16_t)(a | rd8(b | 0x118A));
    a = (uint16_t)(a >> 1);
    p &= (uint8_t)~P_C;
    cyc(74);
    a = op_adc16(a, rd16(b | 0x1189), &p);
    cyc(24);
    a = op_adc16(a, rd16(b | 0x1189), &p);
    cyc(24);
    a ^= rd16(b | 0x11DD);
    cyc(24);
    wr16(b | 0x1189, a);
    cpu.a = a;
    cyc(70);                            /* PLP, RTS */
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
void abi_MenuDimOnL(void)
{
    cyc(24);
    cpu.a = (uint16_t)(rd16((uint32_t)cpu.db << 16 | 0x118F) & 0x0020);
    cpu.p = nz16(cpu.p, cpu.a);
    if (!cpu.a)
        { cyc(88); return; }           /* AND, BEQ taken, RTS */
    cpu.a = 0x0004;
    cpu.p = nz8(cpu.p, 0x04);
    cyc(110);
    ppu_wr(INIDISP, 0x04);
    cyc(64);                            /* REP, RTS */
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
    uint32_t a = (uint32_t)bank << 16 | 0x11D8;

    cyc(68);
    wr16(a, cyc_then(48, (uint16_t)(rd16(a) | 0x0001)));
    while (rd16(cyc_then(24, a)))
        { cyc(22); ; }
    cyc(86);
}

void abi_MenuWaitFrame(void)
{
    MenuWaitFrame(cpu.db);
    cpu.a = 0x0000;
}

void MenuActorsRun(void);
uint16_t MenuOamBuild(uint16_t *x, uint16_t *y);

/*
 * @routine 0B:E2E1 MenuRunFrames
 * WHAT IT DOES: Runs menu frames until $11DA equals $11DB: each frame waits
 * for the vertical blank (MenuWaitFrame), runs the objects
 * (MenuActorsRun), builds the sprite table (MenuOamBuild) and asks for
 * every frame update (bits 0-4 of $11D7).
 * HOW: 8-bit A for the flags and the compare; P saved and restored.
 * Entry: any widths. Exit: A low byte = $11DA, high byte as MenuOamBuild
 * left it; X, Y as MenuOamBuild left them; P unchanged.
 */
void MenuRunFrames(void)
{
    uint32_t b;
    uint8_t p = cpu.p, a;

    cpu.p |= P_M;
    cyc(22);
    do {
        cyc(68);
        MenuWaitFrame(cpu.db);
        cpu.a = 0x0000;
        (cyc(62), MenuActorsRun());
        cyc(62);
        cpu.a = MenuOamBuild(&cpu.x, &cpu.y);
        b = (uint32_t)cpu.db << 16;
        cyc(46);
        wr8(b | 0x11D7, cyc_then(40, (uint8_t)(rd8(b | 0x11D7) | 0x1F)));
        cyc(24);
        a = rd8(b | 0x11DA);
    } while (a != rd8(cyc_then(24, b | 0x11DB)));
    cyc(86);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | a);
    cpu.p = p;
}

void abi_MenuRunFrames(void)
{
    MenuRunFrames();
}

int MenuActorFind(uint8_t bank, uint16_t kind, uint16_t *x, uint16_t *y, uint8_t *p);

/* MenuActorFind as it runs with an 8-bit accumulator: the kind is stored
   and compared as a byte with the low byte of +$1A. Returns the record (or
   whatever X the search left). */
static uint16_t actor_find8(uint8_t kind, uint16_t x)
{
    uint16_t i;

    cyc(86);
    wr8(0x0011E1, kind);
    cyc(24);
    for (i = (uint16_t)(rd16(0x0011F7) - 2); !(i & 0x8000); i = (uint16_t)(i - 2)) {
        cyc(74);
        x = rd16(0x001239 + i);
        cyc(22);
        if (rd8((uint16_t)(x + 0x1A)) == rd8(cyc_then(24, 0x0011E1)))
            break; else { cyc(28); }
    }
    return x;
}

/* One menu frame as the screen loops run it: wait for the vertical blank,
   run the objects, build the sprite table and ask for frame updates 0-3. */
static void menu_frame(void)
{
    uint32_t b;
    uint8_t f;

    cyc(46);
    MenuWaitFrame(cpu.db);
    cpu.a = 0x0000;
    (cyc(62), MenuActorsRun());
    cyc(62);
    cpu.a = MenuOamBuild(&cpu.x, &cpu.y);
    b = (uint32_t)cpu.db << 16;
    cyc(46);
    f = (uint8_t)(rd8(b | 0x11D7) | 0x0F);
    cyc(40);
    wr8(b | 0x11D7, f);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | f);
}

/* MenuRegionCheck's PAL stop (from 0B:AE58), with DB = 0: the kind $0F
   object's animation $1E at Y $60, +$10 of the kind $10 object cleared,
   the objects run and the sprite table built once, frame updates 0-3
   asked for, then the branch to itself at 0B:AE8A for ever. */
static void region_stop(void)
{
    uint16_t x;

    x = actor_find8(0x0F, cpu.x);
    cyc(80);
    if (rd8((uint16_t)(x + 0x14)) != 0x1E) {
        cyc(70);
        wr8((uint16_t)(x + 0x14), 0x1E);
        cyc(22);
        wr8((uint16_t)(x + 0x16), 0x00);
        cyc(22);
        wr8((uint16_t)(x + 0x18), 0x00);
    }
    cyc(38);
    wr8((uint16_t)(x + 0x06), 0x60);
    x = actor_find8(0x10, x);
    cyc(80);
    wr8((uint16_t)(x + 0x10), 0x00);
    cpu.x = x;
    (cyc(62), MenuActorsRun());
    cyc(62);
    cpu.a = MenuOamBuild(&cpu.x, &cpu.y);
    cyc(46);
    wr8(0x0011D7, cyc_then(40, (uint8_t)(rd8(0x0011D7) | 0x0F)));
    cpu_loop_forever(0x0BAE8A);
}

/*
 * @routine 0B:AE47 MenuRegionCheck
 * WHAT IT DOES: Stops the game on a PAL console (bit 4 of STAT78): starts
 * animation $1E on the kind $0F object and puts it at Y $60, clears +$10
 * of the kind $10 object, runs the objects once, builds the sprite table,
 * asks for frame updates 0-3 and then loops for ever. On NTSC it returns
 * at once; no script runs on PAL.
 * HOW: DB = 0 and 8-bit A throughout, so the searches compare and the
 * field writes store single bytes. The loop is a branch to itself without
 * waiting for a frame, so the frame updates are never taken.
 * Entry: X 16-bit. Exit: (NTSC) A low byte = 0; X, Y unchanged; P and DB
 * unchanged.
 */
void MenuRegionCheck(void)
{
    cyc(178);
    uint8_t p = cpu.p, db = cpu.db, s = hw_rd(STAT78) & 0x10;

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | s);
    if (!s) {
        cyc(136);
        cpu.p = p;
        cpu.db = db;
        return;
    }
    cpu.db = 0x00;
    region_stop();
}

void abi_MenuRegionCheck(void)
{
    MenuRegionCheck();
}

/*
 * @routine 0B:AE58 MenuRegionPal
 * WHAT IT DOES: MenuRegionCheck's stop on a PAL console, which no NTSC
 * run reaches: the region screen's objects set (animation $1E on the
 * kind $0F object at Y $60, +$10 of the kind $10 object cleared), one
 * frame's objects and sprites, then a loop for ever.
 * HOW: As inside MenuRegionCheck.
 * Entry: M 8-bit, X 16-bit, D = $0000; DB = 0; P and DB on the stack.
 * Exit: does not return.
 */
void abi_MenuRegionPal(void)
{
    region_stop();
}

/* Any of bits 0-3 of either player's held buttons ($0309, $030D), or
   $18FB set. */
static int held_or_18fb(uint32_t b)
{
    cyc(46);
    return (rd16(b | 0x0309) & 0x000F) || (rd16(cyc_then(64, b | 0x030D)) & 0x000F) || rd16(cyc_then(72, 0x0018FB));
}

/*
 * @routine 0B:AE8F MenuHeldRestart
 * WHAT IT DOES: Returns 0 when no player holds a button in bits 0-3 and
 * $18FB is clear. Otherwise starts animation $18 on the kind $0F object and
 * runs menu frames until that is so, then returns $FFFF (the caller opens
 * its screen again). No script holds those buttons here.
 * HOW: The held words are read through the data bank; the frames are as
 * MenuRunFrames's, with frame updates 0-3.
 * Entry: X 16-bit, D = $0000. Exit: A = 0 or $FFFF; N, Z from it; M
 * 16-bit, X 16-bit; X, Y unchanged when 0, as the last frame left them
 * when $FFFF, C then from the compare of the animation with $18.
 */
uint16_t MenuHeldRestart(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint16_t x = cpu.x, y = cpu.y, a;
    uint8_t p = cpu.p;

    cpu.p &= (uint8_t)~(P_M | P_X);
    if (!held_or_18fb(b)) {
        cpu.p = nz16(cpu.p, 0x0000);
        cyc(82);
        return 0x0000;
    } else { cyc(116); }
    MenuActorFind(cpu.db, 0x000F, &x, &y, &p);
    cyc(22);
    a = rd16((uint16_t)(x + 0x14));
    p = (uint8_t)((p & ~P_C) | (a >= 0x0018 ? P_C : 0));
    if (a != 0x0018) {
        cyc(86);
        wr16((uint16_t)(x + 0x14), 0x0018);
        cyc(22);
        wr16((uint16_t)(x + 0x16), 0x0000);
        cyc(22);
        wr16((uint16_t)(x + 0x18), 0x0000);
    }
    cpu.x = x;
    cpu.y = y;
    cpu.p = (uint8_t)((p & ~(P_M | P_X)) | (cpu.p & (P_M | P_X)));
    do {
        menu_frame();
        cpu.p &= (uint8_t)~(P_M | P_X);
    } while (held_or_18fb(b));
    cyc(82);
    cpu.p = nz16(cpu.p, 0xFFFF);
    return 0xFFFF;
}

void abi_MenuHeldRestart(void)
{
    cpu.a = MenuHeldRestart();
}

void MenuScreenOpen0(void);

/*
 * @routine 0B:AE0A MenuScreen0Run
 * WHAT IT DOES: Runs the menu screen of spawn list 0: opens it
 * (MenuScreenOpen0), runs frames until the menu asks to exit ($11EB), checks
 * the region (MenuRegionCheck), opens it again while buttons are held
 * (MenuHeldRestart), then turns the display, interrupts, HDMA and DMA off.
 * HOW: Frames as MenuRunFrames's, with frame updates 0-3.
 * Entry: D = $0000. Exit: A = $0080; X, Y as the last frame left them; M
 * 8-bit, X 16-bit; N set, Z clear; C clear.
 */
void MenuScreen0Run(void)
{
    uint32_t b;

    do {
        cpu.p &= (uint8_t)~(P_M | P_X);
        MenuScreenOpen0();
        cyc(76);
        do {
            cyc(22);
            menu_frame();
            cpu.p &= (uint8_t)~(P_M | P_X);
            b = (uint32_t)cpu.db << 16;
            cyc(46);
            cpu.a = rd16(b | 0x11EB);
        } while (!cpu.a);
        cyc(62);
        MenuRegionCheck();
        cpu.a = (cyc(46), MenuHeldRestart());
    } while (cpu.a == 0xFFFF);
    b = (uint32_t)cpu.db << 16;
    cyc(102);
    ppu_wr(b | (uint16_t)INIDISP, 0x80);
    cyc(24);
    hw_wr(b | (uint16_t)NMITIMEN, 0x00);
    cyc(24);
    hdmaen_wr(b | (uint16_t)HDMAEN, 0x00);
    cyc(24);
    mdmaen_wr(b | (uint16_t)MDMAEN, 0x00);
    cpu.a = 0x0080;
    cpu.p = (uint8_t)((cpu.p & ~(P_Z | P_C)) | P_N | P_M);
    cyc(44);
}

void abi_MenuScreen0Run(void)
{
    MenuScreen0Run();
}
