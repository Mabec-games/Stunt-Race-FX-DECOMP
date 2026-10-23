/*
 * windows.c: the PPU window and colour maths setups the frame code switches
 * between.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "hw.h"
#include "carried.h"

#include "video_calls.h"

/*
 * @routine 7E:DFE4 WindowsColourAddBg12
 * WHAT IT DOES: Sets the windows and colour maths for adding the sub screen
 * to BG1 and BG2 inside window 1.
 * HOW: W12SEL $99, WOBJSEL $30, CGWSEL $22, CGADSUB $03, WBGLOG $00,
 * WOBJLOG $01, TMW $17, in that order.
 * Entry: any widths. Exit: A low byte = $17; N, Z clear; M 8-bit.
 */
void WindowsColourAddBg12(void)
{
    cyc(70);
    ppu_wr(W12SEL, 0x99);
    cyc(48);
    ppu_wr(WOBJSEL, 0x30);
    cyc(48);
    ppu_wr(CGWSEL, 0x22);
    cyc(48);
    ppu_wr(CGADSUB, 0x03);
    cyc(48);
    ppu_wr(WBGLOG, 0x00);
    cyc(48);
    ppu_wr(WOBJLOG, 0x01);
    cyc(48);
    ppu_wr(TMW, 0x17);
    cyc(42);
}

void abi_WindowsColourAddBg12(void)
{
    WindowsColourAddBg12();
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x17);
    cpu.p = (uint8_t)((cpu.p & ~(P_N | P_Z)) | P_M);
}

/*
 * @routine 7E:E01D WindowsColourWindowOnly
 * WHAT IT DOES: Turns the BG windows off and keeps window 1 for colour only.
 * HOW: W12SEL $00, W34SEL $00, WOBJSEL $20, CGWSEL $10, WBGLOG $00,
 * WOBJLOG $00, TMW $03, in that order.
 * Entry: any widths. Exit: A low byte = $03; N, Z clear; M 8-bit.
 */
void WindowsColourWindowOnly(void)
{
    cyc(70);
    ppu_wr(W12SEL, 0x00);
    cyc(32);
    ppu_wr(W34SEL, 0x00);
    cyc(48);
    ppu_wr(WOBJSEL, 0x20);
    cyc(48);
    ppu_wr(CGWSEL, 0x10);
    cyc(48);
    ppu_wr(WBGLOG, 0x00);
    cyc(48);
    ppu_wr(WOBJLOG, 0x00);
    cyc(48);
    ppu_wr(TMW, 0x03);
    cyc(42);
}

void abi_WindowsColourWindowOnly(void)
{
    WindowsColourWindowOnly();
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x03);
    cpu.p = (uint8_t)((cpu.p & ~(P_N | P_Z)) | P_M);
}

/*
 * @routine 7E:E085 WindowsAllInverted
 * WHAT IT DOES: Sets every layer's window 1 inverted and colour maths
 * subtracting on all layers.
 * HOW: CGADSUB $BF, W12SEL $AA, W34SEL $AA, WOBJSEL $AA, CGWSEL $10,
 * WBGLOG $00, WOBJLOG $00, TMW $1F, in that order.
 * Entry: any widths. Exit: A low byte = $1F; N, Z clear; M 8-bit.
 */
void WindowsAllInverted(void)
{
    cyc(70);
    ppu_wr(CGADSUB, 0xBF);
    cyc(48);
    ppu_wr(W12SEL, 0xAA);
    cyc(32);
    ppu_wr(W34SEL, 0xAA);
    cyc(48);
    ppu_wr(WOBJSEL, 0xAA);
    cyc(48);
    ppu_wr(CGWSEL, 0x10);
    cyc(48);
    ppu_wr(WBGLOG, 0x00);
    cyc(32);
    ppu_wr(WOBJLOG, 0x00);
    cyc(48);
    ppu_wr(TMW, 0x1F);
    cyc(42);
}

void abi_WindowsAllInverted(void)
{
    WindowsAllInverted();
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x1F);
    cpu.p = (uint8_t)((cpu.p & ~(P_N | P_Z)) | P_M);
}

void ColourMathSub83Windowed(void);

/* Setup 2 ($7E:E048), which no script selects: colour maths $03 (add, on
   BG1 and BG2), then the windows for colour only. Leaves A low byte $03. */
static void windows_colour_add03(void)
{
    cyc(146);
    ppu_wr(CGADSUB, 0x03);
    (cyc(46), WindowsColourWindowOnly());
    cyc(42);                            /* RTS */
}

/*
 * @routine 7E:E054 WindowsColourSub81
 * WHAT IT DOES: Window setup 6, which no script selects: colour maths $81,
 * no BG windows, sprites window 1, colour window only, BG1 on the main
 * screen inside the window.
 * HOW: CGADSUB $81, W12SEL $00, W34SEL $00, WOBJSEL $20, CGWSEL $10,
 * WBGLOG $00, WOBJLOG $00, TMW $01, in that order; 8-bit long stores.
 * Entry: any widths. Exit: A low byte = $01; N, Z clear; M 8-bit.
 */
void WindowsColourSub81(void)
{
    cyc(146);
    ppu_wr(CGADSUB, 0x81);
    cyc(48);
    ppu_wr(W12SEL, 0x00);
    cyc(32);
    ppu_wr(W34SEL, 0x00);
    cyc(48);
    ppu_wr(WOBJSEL, 0x20);
    cyc(48);
    ppu_wr(CGWSEL, 0x10);
    cyc(48);
    ppu_wr(WBGLOG, 0x00);
    cyc(48);
    ppu_wr(WOBJLOG, 0x00);
    cyc(48);
    ppu_wr(TMW, 0x01);
    cyc(42);                            /* RTS */
}

void abi_WindowsColourSub81(void)
{
    WindowsColourSub81();
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x01);
    cpu.p = (uint8_t)((cpu.p & ~(P_N | P_Z)) | P_M);
}

/*
 * @routine 7E:DFCA WindowsSetupSelect
 * WHAT IT DOES: Applies the window and colour maths setup chosen by $19A9:
 * 0 WindowsColourAddBg12, 2 ColourMathSub83Windowed, 4 colour maths $03
 * with the windows for colour only, 6 colour maths $81 with BG1 alone
 * inside the window, 8 WindowsAllInverted.
 * HOW: An indexed indirect call through the five-entry table at $7E:DFDA,
 * with the data bank set to $7E. Only 0, 2 and 8 are seen in play.
 * Entry: any widths; $19A9 = 0, 2, 4, 6 or 8. Exit: A low byte as the
 * setup leaves it; X = $19A9 (high byte clear); P and the data bank
 * unchanged.
 */
static uint8_t windows_setup_select(uint8_t *sel)
{
    cyc(140);
    *sel = rd8(0x7E19A9);
    switch (*sel) {
    case 0: cyc(76); WindowsColourAddBg12(); cyc(100); return 0x17;
    case 2: cyc(76); ColourMathSub83Windowed(); cyc(100); return 0x03;
    case 4: windows_colour_add03(); cyc(100); return 0x03;
    case 6: WindowsColourSub81(); cyc(100); return 0x01;
    default: cyc(76); WindowsAllInverted(); cyc(100); return 0x1F;
    }
}

uint8_t WindowsSetupSelect(void)
{
    uint8_t sel;

    return windows_setup_select(&sel);
}

void abi_WindowsSetupSelect(void)
{
    uint8_t sel, a = windows_setup_select(&sel);

    cpu.x = sel;
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | a);
}

void BobStep(Carried *cr);
void BobWindow0(Carried *cr);
void BobWindow1(Carried *cr);
void BobWindowsRun(Carried *cr);
void OscillatorStep(Carried *cr);
void OscillatorWindow(Carried *cr);
void WindowSlideOrOscillate(Carried *cr);
void WindowSweep(Carried *cr);
void WindowSweepIfOn(Carried *cr);
void WindowSweepStep(Carried *cr);
void WindowsColourSubtract3(Carried *cr);
static void bob_setup(Carried *cr, uint32_t b);
static int bob_window(Carried *cr, uint8_t x, uint16_t at);
static void w_WindowHdmaFFD1(Carried *cr);
static void w_WindowHdmaFFD5(Carried *cr);
static void w_WindowHdmaFFD9(Carried *cr);
static void w_WindowSlideStep(Carried *cr);
static void w_WindowsColourWindowOnly(Carried *cr);
void ObjectLinkEventTrigger2(Carried *cr);
void ObjectLinkEventTrigger_cr(Carried *cr);

/*
 * @routine 7E:DF61 BobStep
 * WHAT IT DOES: Steps bob X (0 or 1) at half rate: on every second call
 * ($1A1E + X toggling) its count $1A22 + X goes up; at 10 the count
 * restarts and bit 7 of $0DA4 (bob 0) or $0DA6 (bob 1) is cleared;
 * otherwise its position $1A1C + X moves by its velocity $1A20 + X, which
 * turns round at $1C and $16.
 * HOW: 8-bit; long accesses to bank 0 except the velocity add and the
 * flag bytes (data bank). Bob 1's flag is not reached in the traces so
 * far.
 * Entry: X = bob. Exit: M 8-bit, X 8-bit (high bytes cleared); A low byte
 * and flags from the last step taken.
 */
void BobStep(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;
    uint8_t v, c;

    r_sep(cr, P_M | P_X);
    cr->p &= (uint8_t)~P_C;
    cyc(32);
    v = alu_adc8(rd8(0x001A1E + cr->x), 0x01, &cr->p);
    cr->p = (uint8_t)((nz8(cr->p, (uint8_t)(v - 0x02)) & ~P_C) | (v >= 0x02 ? P_C : 0));
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    if (v != 0x02) {
        cyc(94);
        wr8(0x001A1E + cr->x, v);
        cyc(66);                        /* JMP, RTS */
        return;
    }
    cr->p &= (uint8_t)~P_C;
    cyc(100);
    c = alu_adc8(rd8(0x001A22 + cr->x), 0x01, &cr->p);
    cyc(62);
    wr8(0x001A22 + cr->x, c);
    cr->p = (uint8_t)((nz8(cr->p, (uint8_t)(c - 0x0A)) & ~P_C) | (c >= 0x0A ? P_C : 0));
    if (c == 0x0A) {
        uint16_t at = cr->x == 0x01 ? 0x0DA6 : 0x0DA4;

        cyc(80);
        wr8(0x001A22 + cr->x, 0x00);
        cr->p = (uint8_t)((nz8(cr->p, (uint8_t)(cr->x - 0x01)) & ~P_C) | (cr->x >= 0x01 ? P_C : 0));
        cyc(62);
        v = (uint8_t)(rd8(b | at) & 0x7F);
        cyc(40);
        wr8(b | at, v);
        cr->a = (uint16_t)((cr->a & 0xFF00) | v);
        cr->p = nz8(cr->p, v);
        cyc(cr->x == 0x01 ? 42 : 64);   /* RTS, after BRA for bob 0 */
        return;
    }
    cyc(86);
    wr8(0x001A1E + cr->x, 0x00);
    cyc(32);
    v = rd8(0x001A1C + cr->x);
    cr->p &= (uint8_t)~P_C;
    cyc(38);
    v = alu_adc8(v, rd8(b | (uint16_t)(0x1A20 + cr->x)), &cr->p);
    cyc(32);
    wr8(0x001A1C + cr->x, v);
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    cr->p = (uint8_t)((nz8(cr->p, (uint8_t)(v - 0x1C)) & ~P_C) | (v >= 0x1C ? P_C : (cyc(64), 0)));
    if (v != 0x1C) {
        cr->p = (uint8_t)((nz8(cr->p, (uint8_t)(v - 0x16)) & ~P_C) | (v >= 0x16 ? P_C : 0));
        if (v != 0x16)
            { cyc(66); return; }    /* JMP, RTS */
    }
    v = (uint8_t)((rd8(0x001A20 + cr->x) ^ 0xFF) + 1);
    wr8(0x001A20 + cr->x, v);
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    cr->p = nz8(cr->p, v);
}

void abi_BobStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    BobStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void abi_WindowHdmaFFD1(void);
void abi_WindowHdmaFFD5(void);
void abi_WindowHdmaFFD9(void);

/* Bob x's window: BobStep, then, unless its position is $18, the setup
   bytes at `at` ($20, (position | $40) - 2, position | $80). */
static int bob_window(Carried *cr, uint8_t x, uint16_t at)
{
    uint8_t v, w;

    r_sep(cr, P_M | P_X);
    cr->x = x;
    (cyc(62), BobStep(cr));
    r_sep(cr, P_M | P_X);
    cyc(32);
    v = rd8((uint32_t)(0x001A1C + x));
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    cr->p = (uint8_t)((nz8(cr->p, (uint8_t)(v - 0x18)) & ~P_C) | (v >= 0x18 ? P_C : 0));
    if (v == 0x18)
        return 0;
    cyc(102);
    wr8((uint32_t)(0x000002 + at), (uint8_t)(v | 0x80));
    cr->p |= P_C;
    w = alu_sbc8((uint8_t)(v | 0x40), 0x02, &cr->p);
    cyc(90);
    wr8((uint32_t)(0x000001 + at), w);
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0x20);
    cyc(48);
    wr8((uint32_t)at, 0x20);
    return 1;
}

/*
 * @routine 7E:DEFE BobWindow0
 * WHAT IT DOES: Steps bob 0 (BobStep) and, unless it is at $18, sets window
 * setup bytes $19AC-$19AE from its position ($20, (position | $40) - 2,
 * position | $80) and the window bands (WindowHdmaFFD1, or WindowHdmaFFD5
 * with $0D62 set).
 * HOW: Long accesses to bank 0; $0D62 through the data bank. The $0D62
 * case is not reached in the traces so far.
 * Entry: any widths. Exit: M 8-bit, X 8-bit; as the last call left them.
 */
void BobWindow0(Carried *cr)
{
    uint8_t v;

    cyc(46);
    if (!bob_window(cr, 0, 0x19AC))
        { cyc(42); return; }
    cyc(24);
    v = rd8((uint32_t)cr->db << 16 | 0x0D62);
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    if (v)
        { cyc(68); w_WindowHdmaFFD5(cr); cyc(42); }
    else
        { cyc(62); w_WindowHdmaFFD1(cr); cyc(66); } /* JMP, RTS */
}

void abi_BobWindow0(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    BobWindow0(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 7E:DF35 BobWindow1
 * WHAT IT DOES: As BobWindow0 for bob 1: setup bytes $19AF-$19B1 and the
 * bands WindowHdmaFFD9.
 * HOW: As BobWindow0. Reached by a unit test
 * (unit_uncalled).
 * Entry: any widths. Exit: as BobWindow0.
 */
void BobWindow1(Carried *cr)
{
    if (bob_window(cr, 1, 0x19AF))
        { cyc(46); w_WindowHdmaFFD9(cr); cyc(42); }
    else
        cyc(80);
}

void abi_BobWindow1(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    BobWindow1(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 7E:DE88 WindowSweepStep
 * WHAT IT DOES: Steps sweep X at half rate ($1A17 + X toggling): its
 * position $1A15 + X moves by its velocity $1A19 + X, which turns round at
 * $F2 and $E8; the sixteenth step ($1A1B) ends the sweep: bit 4 of $0DA5
 * cleared, $1A17/$1A18/$1A1B = 0, velocities $FE, positions $F0.
 * HOW: 8-bit; long accesses to bank 0 except the velocity add and the
 * end's stores (data bank). Not reached in the traces so far.
 * Entry: X = sweep. Exit: M 8-bit, X 8-bit; A and flags from the last
 * step.
 */
void WindowSweepStep(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;
    uint8_t v, c;
    unsigned i;

    r_sep(cr, P_M | P_X);
    cr->p &= (uint8_t)~P_C;
    cyc(32);
    v = alu_adc8(rd8(0x001A17 + cr->x), 0x01, &cr->p);
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    cr->p = (uint8_t)((nz8(cr->p, (uint8_t)(v - 0x02)) & ~P_C) | (v >= 0x02 ? P_C : 0));
    if (v != 0x02) {
        cyc(94);
        wr8(0x001A17 + cr->x, v);
        cyc(66);                        /* JMP, RTS */
        return;
    }
    cr->p &= (uint8_t)~P_C;
    cyc(100);
    c = alu_adc8(rd8(0x001A1B), 0x01, &cr->p);
    cyc(62);
    wr8(0x001A1B, c);
    cr->p = (uint8_t)((nz8(cr->p, (uint8_t)(c - 0x10)) & ~P_C) | (c >= 0x10 ? P_C : 0));
    if (c == 0x10) {
        static const uint16_t z[3] = {0x1A17, 0x1A18, 0x1A1B};

        cyc(80);
        wr8(0x001A1B, 0x00);
        cyc(24);
        v = (uint8_t)(rd8(b | 0x0DA5) & 0xEF);
        cyc(40);
        wr8(b | 0x0DA5, v);
        cyc(16);
        for (i = 0; i < 3; i++)
            { cyc(24); wr8(b | z[i], 0x00); }
        cyc(40);
        wr8(b | 0x1A19, 0xFE);
        cyc(24);
        wr8(b | 0x1A1A, 0xFE);
        cyc(40);
        wr8(b | 0x1A15, 0xF0);
        cyc(24);
        wr8(b | 0x1A16, 0xF0);
        cr->a = (uint16_t)((cr->a & 0xFF00) | 0xF0);
        cr->p = nz8(cr->p, 0xF0);
        cyc(42);                        /* RTS */
        return;
    }
    cyc(86);
    wr8(0x001A17 + cr->x, 0x00);
    cyc(32);
    v = rd8(0x001A15 + cr->x);
    cr->p &= (uint8_t)~P_C;
    cyc(38);
    v = alu_adc8(v, rd8(b | (uint16_t)(0x1A19 + cr->x)), &cr->p);
    cyc(32);
    wr8(0x001A15 + cr->x, v);
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    cr->p = (uint8_t)((nz8(cr->p, (uint8_t)(v - 0xF2)) & ~P_C) | (v >= 0xF2 ? P_C : (cyc(64), 0)));
    if (v != 0xF2) {
        cr->p = (uint8_t)((nz8(cr->p, (uint8_t)(v - 0xE8)) & ~P_C) | (v >= 0xE8 ? P_C : 0));
        if (v != 0xE8)
            { cyc(66); return; }    /* JMP, RTS */
    }
    cyc(70);
    v = (uint8_t)((rd8(0x001A19 + cr->x) ^ 0xFF) + 1);
    cyc(62);
    wr8(0x001A19 + cr->x, v);
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    cr->p = nz8(cr->p, v);
    cyc(42);                            /* RTS */
}

void abi_WindowSweepStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    WindowSweepStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 7E:DE5E WindowSweep
 * WHAT IT DOES: Steps sweep 0 (WindowSweepStep) and, unless its position
 * $1A15 is $EE, puts the position in window setup bytes $19AC, $19AE and
 * $19AD and sets the window bands (WindowHdmaFFD1, or WindowHdmaFFD5 with
 * $0D62 set).
 * HOW: Long stores to bank 0. Not reached in the traces so far.
 * Entry: any widths. Exit: M 8-bit, X 8-bit; as the last call left them.
 */
void WindowSweep(Carried *cr)
{
    uint8_t v;

    r_sep(cr, P_M | P_X);
    cr->x = 0x00;
    (cyc(62), WindowSweepStep(cr));
    cyc(32);
    v = rd8(0x001A15);
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    cr->p = (uint8_t)((nz8(cr->p, (uint8_t)(v - 0xEE)) & ~P_C) | (v >= 0xEE ? P_C : 0));
    if (v == 0xEE)
        { cyc(80); return; }
    cyc(64);
    wr8(0x0019AC, v);
    cyc(32);
    wr8(0x0019AE, v);
    cyc(32);
    wr8(0x0019AD, v);
    cyc(24);
    v = rd8((uint32_t)cr->db << 16 | 0x0D62);
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    if (v)
        { cyc(68); w_WindowHdmaFFD5(cr); }
    else
        (cyc(62), w_WindowHdmaFFD1(cr));
    cyc(v ? 42 : 66);                   /* RTS; JMP, RTS after FFD1 */
}

void abi_WindowSweep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    WindowSweep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 7E:D311 WindowSweepIfOn
 * WHAT IT DOES: Unless the window HDMA state $19D8 is set or no layout is
 * chosen ($19BC = $FF), while bit 4 of $0DA5 is set selects window setup
 * 4 ($19A9) and steps the sweep (WindowSweep).
 * HOW: The data bank is set to the code's ($7E) and restored. The sweep
 * is not reached in the traces so far.
 * Entry: any widths. Exit: P and the data bank restored; A as the last
 * test or call left it.
 */
void WindowSweepIfOn(Carried *cr)
{
    uint8_t p0 = cr->p, db0 = cr->db, v;
    uint32_t b = 0x7E0000;

    cr->db = 0x7E;
    cr->p |= P_M;
    cyc(140);
    v = rd8(b | 0x19D8);
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    if (!v) {
        cyc(48);
        v = rd8(0x0019BC);
        cr->a = (uint16_t)((cr->a & 0xFF00) | v);
        if (v != 0xFF) {
            cyc(78);
            v = (uint8_t)(rd8(b | 0x0DA5) & 0x10);
            cr->a = (uint16_t)((cr->a & 0xFF00) | v);
            if (v) {
                cr->a = (uint16_t)((cr->a & 0xFF00) | 0x04);
                cyc(94);
                wr8(b | 0x19A9, 0x04);
                (cyc(46), WindowSweep(cr));
            } else { cyc(38); }
        } else { cyc(38); }
    } else { cyc(22); }
    cyc(58);
    cr->db = db0;
    r_plp(cr, p0);
}

void abi_WindowSweepIfOn(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    WindowSweepIfOn(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 7E:DD43 OscillatorStep
 * WHAT IT DOES: Steps oscillator X by the level in bits 5-7 of $1A11 (4
 * while bit 7 is set, with $1A0E + X = $80): a change of level restarts
 * its count $1A0A + X; level 0 resets it ($1A08 + X = $40, rising by 1);
 * otherwise its count runs to the rate for the level (the table at
 * $7E:D30D, kept in $1A10) and then $1A08 + X moves by $1A0C + X, turning
 * round at $40 and $42.
 * HOW: 8-bit; long accesses to bank 0 except $1A10's store and the step
 * add (data bank). Bit 7 and level 0 are not reached in the traces so far.
 * Entry: X = oscillator. Exit: M 8-bit, X 8-bit; A and flags from the
 * last step.
 */
void OscillatorStep(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;
    uint8_t v, a, c;

    r_sep(cr, P_M | P_X);
    cyc(32);
    v = (uint8_t)(rd8(0x001A11) & 0x80);
    if (v) {
        cyc(80);
        wr8(0x001A0E + cr->x, 0x80);
        a = 0x04;
        cyc(18);                        /* LDA #, JMP instead of BNE */
    } else {
        cyc(70);
        a = (uint8_t)(rd8(0x001A11) >> 5);
        cyc(102);
        c = rd8(0x001A0E + cr->x);
        cr->p = (uint8_t)((nz8(cr->p, (uint8_t)(a - c)) & ~P_C) | (a >= c ? P_C : 0));
        if (a != c)
            wr8(0x001A0A + cr->x, 0x00);
        cyc(54);
        v = rd8(0x001A11);
        a = (uint8_t)(v >> 5);
        cr->p = (uint8_t)((nz8(cr->p, a) & ~P_C) | ((v & 0x10) ? P_C : 0));
        cyc(102);
        wr8(0x001A0E + cr->x, a);
        if (!a) {
            cyc(64);
            wr8(0x001A0A + cr->x, 0x00);
            cyc(48);
            wr8(0x001A08 + cr->x, 0x40);
            cyc(48);
            wr8(0x001A0C + cr->x, 0x01);
            cr->a = (uint16_t)((cr->a & 0xFF00) | 0x01);
            cr->p = nz8(cr->p, 0x01);
            cyc(66);
            return;
        }
    }
    a = (uint8_t)(a - 1);
    cyc(104);
    v = rd8((uint32_t)(0x7ED30D + a));
    cyc(24);
    wr8(b | 0x1A10, v);
    cr->p &= (uint8_t)~P_C;
    cyc(60);
    c = alu_adc8(rd8(0x001A0A + cr->x), 0x01, &cr->p);
    cyc(62);
    v = rd8(0x001A10);
    cr->a = (uint16_t)((cr->a & 0xFF00) | c);
    cr->p = (uint8_t)((nz8(cr->p, (uint8_t)(c - v)) & ~P_C) | (c >= v ? P_C : 0));
    if (c != v) {
        cyc(48);
        wr8(0x001A0A + cr->x, c);
        cyc(66);
        return;
    }
    wr8(0x001A0A + cr->x, 0x00);
    v = rd8(0x001A08 + cr->x);
    cr->p &= (uint8_t)~P_C;
    v = alu_adc8(v, rd8(b | (uint16_t)(0x1A0C + cr->x)), &cr->p);
    wr8(0x001A08 + cr->x, v);
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    cr->p = (uint8_t)((nz8(cr->p, (uint8_t)(v - 0x40)) & ~P_C) | (v >= 0x40 ? P_C : 0));
    if (v != 0x40) {
        cr->p = (uint8_t)((nz8(cr->p, (uint8_t)(v - 0x42)) & ~P_C) | (v >= 0x42 ? P_C : 0));
        if (v != 0x42)
            return;
    }
    v = (uint8_t)((rd8(0x001A0C + cr->x) ^ 0xFF) + 1);
    wr8(0x001A0C + cr->x, v);
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    cr->p = nz8(cr->p, v);
}

void abi_OscillatorStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    OscillatorStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 7E:DD12 OscillatorWindow
 * WHAT IT DOES: Sets the oscillator level $1A11 from $0DDA, steps
 * oscillator 0 (OscillatorStep) and puts its value in window setup byte
 * $19AD with $19AC = $20 and $19AE = $95, then sets the window bands
 * (WindowHdmaFFD1, or WindowHdmaFFD5 with $0D62 set).
 * HOW: Long stores to bank 0. The $0D62 case is not reached in the traces
 * so far.
 * Entry: any widths. Exit: M 8-bit, X 8-bit; as the last call left them.
 */
void OscillatorWindow(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;
    uint8_t v;

    r_sep(cr, P_M | P_X);
    cyc(32);
    v = rd8(0x000DDA);
    cyc(32);
    wr8(0x001A11, v);
    cr->x = 0x00;
    (cyc(62), OscillatorStep(cr));
    cyc(24);
    v = rd8(b | 0x1A08);
    cyc(32);
    wr8(0x0019AD, v);
    cyc(48);
    wr8(0x0019AC, 0x20);
    cyc(48);
    wr8(0x0019AE, 0x95);
    cyc(24);
    v = rd8(b | 0x0D62);
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    if (v)
        { cyc(68); w_WindowHdmaFFD5(cr); }
    else
        { cyc(62); w_WindowHdmaFFD1(cr); }
    cyc(v ? 42 : 66);                   /* RTS; JMP, RTS after FFD1 */
}

void abi_OscillatorWindow(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    OscillatorWindow(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void abi_ObjectLinkEventTrigger(void);
void abi_ObjectLinkEventTrigger2(void);

/* Bit 5 of $0D45 cleared and window setup 2 ($19A9). */
static void bob_setup(Carried *cr, uint32_t b)
{
    uint8_t v;

    cr->p |= P_M;
    cyc(86);
    v = (uint8_t)(rd8(0x000D45) & 0xDF);
    cyc(48);
    wr8(0x000D45, v);
    cyc(40);
    wr8(b | 0x19A9, 0x02);
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0x02);
}

/*
 * @routine 7E:D33A BobWindowsRun
 * WHAT IT DOES: Unless the window HDMA state $19D8 is set or no layout is
 * chosen ($19BC = $FF): checks player 1's link event
 * (ObjectLinkEventTrigger) and, while bob 0 runs (bit 7 of $0DA4), sets
 * window setup 2 and steps it (BobWindow0); with $0D62 set the same for
 * player 2 (ObjectLinkEventTrigger2, $0DA6, BobWindow1).
 * HOW: The data bank is the code's ($7E) and restored. $0D62 is read at
 * the width BobWindow0 left (a word after WindowHdmaFFD1). Bob 1's window
 * is not reached in the traces so far.
 * Entry: any widths. Exit: P and the data bank restored.
 */
void BobWindowsRun(Carried *cr)
{
    uint8_t p0 = cr->p, db0 = cr->db, v;
    uint32_t b = 0x7E0000;

    cr->db = 0x7E;
    cr->p |= P_M;
    cyc(140);
    v = rd8(b | 0x19D8);
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    if (v)
        { cyc(22); goto out; }
    cyc(48);
    v = rd8(0x0019BC);
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    if (v == 0xFF)
        { cyc(38); goto out; }
    (cyc(78), ObjectLinkEventTrigger_cr(cr));
    r_sep(cr, P_M | P_X);
    cyc(cr->p & P_Z ? 110 : 126);       /* beq taken at lda.w $1A24, not bne at $0DA4 */
    v = (uint8_t)(rd8(b | 0x0DA4) & 0x80);
    if (v) {
        bob_setup(cr, b);
        BobWindow0(cr);
    }
    if (cr->p & P_M) {
        cyc(62);
        v = rd8(b | 0x0D62);
        cr->a = (uint16_t)((cr->a & 0xFF00) | v);
        if (!v)
            { cyc(22); goto out; }
    } else {                    /* WindowHdmaFFD1 left M 16-bit: a word */
        cyc(24);
        cr->a = rd16(b | 0x0D62);
        if (!cr->a)
            goto out;
    }
    cyc(62);
    r_sep(cr, P_M | P_X);
    ObjectLinkEventTrigger2(cr);
    cr->p |= P_M;
    cyc(46);
    v = (uint8_t)(rd8(b | 0x0DA6) & 0x80);
    if (v) {
        bob_setup(cr, b);
        (cyc(46), BobWindow1(cr));
    } else { cyc(38); }
out:
    cyc(100);
    cr->db = db0;
    cr->p = p0;
}

void abi_BobWindowsRun(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    BobWindowsRun(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 7E:E048 WindowsColourSubtract3
 * WHAT IT DOES: Colour maths adds on BG1 and BG2 ($03 to CGADSUB), then the
 * colour window only (WindowsColourWindowOnly).
 * HOW: An 8-bit long store, then the call.
 * Entry: any widths. Exit: M 8-bit; as WindowsColourWindowOnly.
 */
void WindowsColourSubtract3(Carried *cr)
{
    r_sep(cr, P_M);
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0x03);
    cr->p = nz8(cr->p, 0x03);
    ppu_wr(0x002131, 0x03);
    (cyc(46), w_WindowsColourWindowOnly(cr));
    cyc(42);                            /* RTS */
}

void abi_WindowsColourSubtract3(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    WindowsColourSubtract3(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void abi_WindowSlideStep(void);

/*
 * @routine 7E:D2C9 WindowSlideOrOscillate
 * WHAT IT DOES: Unless the split window is on ($19D8) or its line $19BC is
 * $FF: with bits 1-2 of $0DA5, steps the slide (window setup 2,
 * WindowSlideStep) and, if still sliding, stops; otherwise, with bit 3,
 * window setup 2 and the oscillating window (OscillatorWindow).
 * HOW: Data bank $7E (the program's) around it; P saved and restored.
 * Entry: any widths. Exit: P and the data bank restored; A, X, Y as the
 * last step left them.
 */
void WindowSlideOrOscillate(Carried *cr)
{
    uint8_t p0 = cr->p, db0 = cr->db, v;

    cr->db = 0x7E;
    r_sep(cr, P_M);
    cyc(140);
    r_lda8(cr, 0x7E19D8);
    if (!(uint8_t)cr->a) {
        cyc(48);
        r_lda8(cr, 0x0019BC);
        v = (uint8_t)cr->a;
        cr->p = (uint8_t)((nz8(cr->p, (uint8_t)(v - 0xFF)) & ~P_C) | (v >= 0xFF ? (cyc(38), P_C) : 0));
        if (v != 0xFF) {
            cyc(78);
            r_lda8(cr, 0x7E0DA5);
            v = (uint8_t)(cr->a & 0x06);
            cr->a = (uint16_t)((cr->a & 0xFF00) | v);
            cr->p = nz8(cr->p, v);
            if (v) {
                cr->a = (uint16_t)((cr->a & 0xFF00) | 0x02);
                cr->p = nz8(cr->p, 0x02);
                cyc(72);
                wr8(0x7E19A9, 0x02);
                w_WindowSlideStep(cr);
                cyc(108);
                r_lda8(cr, 0x7E0DA5);
                v = (uint8_t)(cr->a & 0x06);
                cr->a = (uint16_t)((cr->a & 0xFF00) | v);
                cr->p = nz8(cr->p, v);
                if (v)
                    { cyc(32); goto out; }
            }
            r_sep(cr, P_M);
            cyc(84);
            r_lda8(cr, 0x7E0DA5);
            v = (uint8_t)(cr->a & 0x08);
            cr->a = (uint16_t)((cr->a & 0xFF00) | v);
            cr->p = nz8(cr->p, v);
            if (v) {
                r_sep(cr, P_M);
                cr->a = (uint16_t)((cr->a & 0xFF00) | 0x02);
                cr->p = nz8(cr->p, 0x02);
                cyc(94);
                wr8(0x7E19A9, 0x02);
                (cyc(46), OscillatorWindow(cr));
            } else { cyc(38); }
        }
        cyc(0);
    } else { cyc(22); }
out:
    cyc(58);
    cr->db = db0;
    r_plp(cr, p0);
}

void abi_WindowSlideOrOscillate(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    WindowSlideOrOscillate(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* WindowHdmaFFD1 as its adapter leaves the registers. */
static void w_WindowHdmaFFD1(Carried *cr)
{
    WindowHdmaFFD1();
    cr->a = 0xFFD1;
    cr->p = (uint8_t)((cr->p & ~(P_Z | P_M)) | P_N);
}

/* WindowHdmaFFD5 as its adapter leaves the registers. */
static void w_WindowHdmaFFD5(Carried *cr)
{
    cr->a = WindowHdmaFFD5();
    cr->p = (uint8_t)((cr->p & ~(P_Z | P_M)) | P_N);
}

/* WindowHdmaFFD9 as its adapter leaves the registers. */
static void w_WindowHdmaFFD9(Carried *cr)
{
    cr->a = WindowHdmaFFD9();
    cr->p = (uint8_t)((cr->p & ~(P_Z | P_M)) | P_N);
}

/* WindowsColourWindowOnly as its adapter leaves the registers. */
static void w_WindowsColourWindowOnly(Carried *cr)
{
    WindowsColourWindowOnly();
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0x03);
    cr->p = (uint8_t)((cr->p & ~(P_N | P_Z)) | P_M);
}

/* WindowSlideStep as its adapter leaves the registers. */
static void w_WindowSlideStep(Carried *cr)
{
    uint8_t p = cr->p;
    int moved;
    uint8_t a = WindowSlideStep(cr->db, &p, &moved);

    cr->a = moved ? (uint16_t)(0xFF00 | a) : (uint16_t)((cr->a & 0xFF00) | a);
    cr->p = p;
}
