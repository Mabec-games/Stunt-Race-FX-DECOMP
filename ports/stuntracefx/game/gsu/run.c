/*
 * run.c: starting the Super FX and waiting for it.
 *
 * The S-CPU hands ROM and RAM to the Super FX (screen mode register bits 4
 * and 3), sets its program bank and writes its program counter, which starts
 * it. While it runs, the S-CPU polls its status and counts the polls in
 * direct page $FE-$FF; when it stops, the S-CPU takes the bus back by
 * restoring its own screen mode value.
 */
#include "machine.h"
#include "native.h"
#include "hw.h"
#include "wram.h"
#include "cpu_ops.h"
#include "carried.h"

#include "video_calls.h"
#include "hud_calls.h"
#include "object_calls.h"
#include "system_calls.h"
#include "wram_names.h"
void abi_HudColourStep(void);

/* Poll until the Super FX stops (status bit 5 clear), counting each poll:
   an 8-bit INC of $FE, and of $FF only when $FE wraps to 0. `first` is
   the time before the first INC's read: GsuWait's SEP and the INC (38),
   or GsuRun's INC alone (16); after a poll, AND #, BNE and the INC (54). */
static void gsu_wait(uint32_t first)
{
    uint8_t n;

    cyc(first);
    do {
        n = (uint8_t)(rd8(GSU_WAIT_COUNT) + 1);
        cyc(6);
        wr8(GSU_WAIT_COUNT, n);
        if (!n) {
            cyc(32);
            n = (uint8_t)(rd8(GSU_WAIT_COUNT + 1) + 1);
            cyc(6);
            wr8(GSU_WAIT_COUNT + 1, n);
        } else { cyc(22); }
    } while (cyc_loop(54, hw_rd(cyc_then(24, GSU_SFR)) & 0x20));
}

/* After the poll: the screen mode value back to SCMR. */
static uint8_t gsu_scmr_back(void)
{
    uint8_t mode;

    cyc(56);
    mode = rd8(GSU_SCMR_SHADOW);
    cyc(24);
    hw_wr(GSU_SCMR, mode);
    return mode;
}

/*
 * @routine 7E:E1CB GsuWait
 * WHAT IT DOES: Waits for a running Super FX program to stop, then takes the
 * ROM and RAM back for the S-CPU.
 * HOW: Steps the 16-bit poll count and reads the status register until its
 * running bit clears, then writes the S-CPU's screen mode value ($0374).
 * Entry: X 16-bit, D = $0000. Exit: A low byte = the screen mode value; N, Z
 * from it; M 8-bit.
 */
uint8_t GsuWait(void)
{
    gsu_wait(38);
    return cyc_then(42, gsu_scmr_back());
}

void FrameRunGsuAB13A(Carried *cr);
void FrameRunGsuAB13B(Carried *cr);
void GsuAB13WithParams_cr(Carried *cr);
void GsuRun_cr(Carried *cr);
void GsuWait_cr(Carried *cr);
void HudFrameWhileGsuRuns(Carried *cr);
void FrameObjectsWhileGsuRuns(Carried *cr);
void FrameRunGsu8FCA(Carried *cr);
static void run_cmp8(Carried *cr, uint8_t v, uint8_t m);
static void start_ab13(Carried *cr, uint32_t b);
static void w_BackdropScrollFromView(Carried *cr);
static void w_FrameObjectsWhileGsuRuns(Carried *cr);
static void w_MapMarkersPanelA(Carried *cr);
static void w_MapMarkersPanelB(Carried *cr);
static void w_ObjectDrawListBuild(Carried *cr);
static void w_SlideStep(Carried *cr);
void BackdropRowsUpdate_cr(Carried *cr);
void BucketTriggersFixedArea_cr(Carried *cr);
void BucketTriggersFromView_cr(Carried *cr);
void DataBankTest_cr(Carried *cr);
void FadeValueStep_cr(Carried *cr);
void GsuParamsUpdate_cr(Carried *cr);
void GsuRecordsUpload_cr(Carried *cr);
void HudColourStep_cr(Carried *cr);
void HudDigitSprites62_cr(Carried *cr);
void HudDrawDigits19EC_cr(Carried *cr);
void HudDrawValueA_cr(Carried *cr);
void HudDrawValueB_cr(Carried *cr);
void HudDrawValueC_cr(Carried *cr);
void HudGaugesPanelA_cr(Carried *cr);
void HudGaugesPanelB_cr(Carried *cr);
void ObjectGroupsActivate_cr(Carried *cr);

void GsuWait_cr(Carried *cr)
{
    uint8_t mode = GsuWait();

    cr->a = (uint16_t)((cr->a & 0xFF00) | mode);
    set_p(cr, (uint8_t)(nz8(cr->p, mode) | P_M));
}

void abi_GsuWait(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    GsuWait_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 7E:E1F5 GsuRun
 * WHAT IT DOES: Runs a Super FX program to completion.
 * HOW: Sets the program bank, gives ROM and RAM to the Super FX, clears its
 * status and writes the start address to R15, which starts it; then waits as
 * GsuWait does. The data bank is 0 inside and restored on return.
 * Entry: M 8-bit, X 16-bit, D = $0000; A = program bank, X = start address.
 * Exit: A low byte = the screen mode value; N, Z from the data bank pulled
 * back on return; X unchanged.
 */
uint8_t GsuRun(uint8_t bank, uint16_t start)
{
    cyc(32);
    hw_wr(GSU_PBR, bank);
    cyc(112);
    hw_wr(GSU_SCMR, cyc_then(40, rd8(GSU_SCMR_SHADOW) | 0x18));
    cyc(24);
    hw_wr(GSU_SFR, 0x00);
    cyc(24);
    hw_wr16(GSU_R15, start);
    gsu_wait(16);                       /* its own copy of the poll, no SEP */
    return gsu_scmr_back();
}

void GsuRun_cr(Carried *cr)
{
    uint8_t mode = GsuRun((uint8_t)cr->a, cr->x);

    cr->a = (uint16_t)((cr->a & 0xFF00) | mode);
    set_p(cr, nz8(cr->p, cr->db));
    cyc(72);                            /* PLB, RTL */
}

void abi_GsuRun(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    GsuRun_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

uint16_t GsuParamsUpdate(void);

/* The body; returns the value the abi_ wrapper needs. */
static uint8_t gsu_ab13_with_params(uint8_t bank, uint16_t *last)
{
    uint32_t b = (uint32_t)bank << 16;
    uint8_t mode;

    *last = (cyc(22), GsuParamsUpdate());
    cyc(108);
    hw_wr(b | 0x3034, 0x01);
    cyc(24);
    hw_wr(b | 0x303A, cyc_then(40, (uint8_t)(rd8(b | W(gsu_scmr_shadow)) | 0x18)));
    cyc(24);
    hw_wr16(b | 0x301E, 0xAB13);
    do {
        cyc(46);
        uint8_t n = (uint8_t)(rd8(b | W(gsu_ab13_polls)) + 1);

        cyc(6);
        wr8(b | W(gsu_ab13_polls), n);
        if (!n)
            { cyc(40); wr8(b | 0x1F22, cyc_then(6, (uint8_t)(rd8(b | 0x1F22) + 1))); } else { cyc(22); }
    } while (cyc_loop(16, hw_rd(cyc_then(24, b | 0x3030)) & 0x20));
    cyc(56);
    mode = rd8(b | W(gsu_scmr_shadow));
    cyc(24);
    hw_wr(b | 0x303A, mode);
    return mode;
}

/*
 * @routine 7F:0919 GsuAB13WithParams
 * WHAT IT DOES: Updates the renderer's parameter block, runs Super FX
 * program $01:AB13 to completion and takes the bus back, counting the polls
 * in $1F21-$1F22.
 * HOW: GsuParamsUpdate, then as GsuRun but written out: program bank 1,
 * screen mode $0374 | $18, R15 = $AB13; each poll steps the 16-bit count
 * (low byte, then the high byte when the low one wraps) before reading the
 * status. Everything but the parameter copy goes through the data bank.
 * Entry: any widths, D = $0000. Exit: A low byte = the screen mode value,
 * high byte from the last parameter copied; X = $AB13 ($13 when P
 * restores 8-bit index registers); P unchanged.
 */
uint8_t GsuAB13WithParams(uint8_t bank)
{
    uint16_t last;

    return gsu_ab13_with_params(bank, &last);
}

void GsuAB13WithParams_cr(Carried *cr)
{
    uint16_t last;
    uint8_t mode = gsu_ab13_with_params(cr->db, &last);

    cr->a = (uint16_t)((last & 0xFF00) | mode);
    cr->x = (cr->p & P_X) ? 0x0013 : (cyc(72), 0xAB13);
}

void abi_GsuAB13WithParams(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    GsuAB13WithParams_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void abi_BucketTriggersFromView(void);
void abi_ObjectDrawListBuild(void);
void abi_BucketTriggersFixedArea(void);
void abi_ObjectGroupsActivate(void);
void abi_GsuRecordsUpload(void);

/*
 * @routine 7E:E1E1 FrameObjectsWhileGsuRuns
 * WHAT IT DOES: The S-CPU's share of a race frame while the Super FX
 * draws: the trigger list for the view (BucketTriggersFromView), the draw
 * list (ObjectDrawListBuild), the fixed area's triggers
 * (BucketTriggersFixedArea), the object groups (ObjectGroupsActivate);
 * then waits for the Super FX (GsuWait) and uploads the records it needs
 * next (GsuRecordsUpload).
 * HOW: Six calls in turn.
 * Entry: X 16-bit, D = $0000. Exit: as GsuRecordsUpload leaves it.
 */
void FrameObjectsWhileGsuRuns(Carried *cr)
{
    (cyc(46), BucketTriggersFromView_cr(cr));
    (cyc(46), w_ObjectDrawListBuild(cr));
    (cyc(46), BucketTriggersFixedArea_cr(cr));
    (cyc(46), ObjectGroupsActivate_cr(cr));
    (cyc(46), GsuWait_cr(cr));
    (cyc(62), GsuRecordsUpload_cr(cr));
}

void abi_FrameObjectsWhileGsuRuns(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    FrameObjectsWhileGsuRuns(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 7E:E221 FrameRunGsu8FCA
 * WHAT IT DOES: Starts Super FX program $04:8FCA, does the frame's object
 * work while it runs (FrameObjectsWhileGsuRuns), and keeps the number of
 * polls the wait took ($FE) in $1F1F.
 * HOW: As GsuRun, without waiting before the object work; the poll count
 * in direct page $FE is cleared first. Data bank 0 inside, restored at the
 * end.
 * Entry: D = $0000. Exit: A = the poll count; X, Y as GsuRecordsUpload
 * leaves them; M 8-bit, X 16-bit; N, Z from the data bank pulled back.
 */
void FrameRunGsu8FCA(Carried *cr)
{
    uint8_t db = cr->db;
    uint16_t n;

    set_p(cr, (uint8_t)((cr->p | P_M) & ~P_X));
    cyc(60);
    wr8(W(gsu_wait_count), 0x00);
    cyc(16);
    wr8(W(gsu_wait_count_hi), 0x00);
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0x04);
    cr->x = 0x8FCA;
    cyc(72);
    hw_wr(GSU_PBR, 0x04);
    cr->db = 0x00;
    cyc(112);
    hw_wr(GSU_SCMR, cyc_then(40, (uint8_t)(rd8(GSU_SCMR_SHADOW) | 0x18)));
    cyc(24);
    hw_wr(GSU_SFR, 0x00);
    cyc(24);
    hw_wr16(GSU_R15, 0x8FCA);
    (cyc(46), FrameObjectsWhileGsuRuns(cr));
    cyc(80);
    n = rd16(W(gsu_wait_count));
    cyc(24);
    wr16(W(gsu_wait_polls), n);
    cr->a = n;
    cr->db = db;
    set_p(cr, (uint8_t)(nz8(cr->p | P_M, db) & ~P_X));
    cyc(94);
}

void abi_FrameRunGsu8FCA(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    FrameRunGsu8FCA(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void abi_SlideStep(void);
void abi_FadeValueStep(void);
void abi_DataBankTest(void);
void abi_HudDigitSprites62(void);
void abi_HudDrawDigits19EC(void);
void abi_BackdropRowsUpdate(void);
void abi_HudDrawValueA(void);
void abi_HudDrawValueB(void);
void abi_HudDrawValueC(void);
void abi_BackdropScrollFromView(void);
void abi_GsuParamsUpdate(void);
void abi_HudGaugesPanelA(void);
void abi_HudGaugesPanelB(void);
void abi_MapMarkersPanelA(void);
void abi_MapMarkersPanelB(void);

/* CMP # (8-bit): N, Z and C. */
static void run_cmp8(Carried *cr, uint8_t v, uint8_t m)
{
    set_p(cr, (uint8_t)((nz8(cr->p, (uint8_t)(v - m)) & ~P_C) | (v >= m ? (cyc(8), P_C) : (cyc(2), 0))));
}

/*
 * @routine 7E:E303 HudFrameWhileGsuRuns
 * WHAT IT DOES: The HUD's share of a frame while the Super FX draws: the
 * slide and the fade value step (SlideStep, FadeValueStep), DataBankTest;
 * then, with $0D62 clear, the HUD digits (HudDigitSprites62 unless the HUD
 * mode $0D2B is 1 or 3; HudDrawDigits19EC unless it is 3), the backdrop
 * rows (BackdropRowsUpdate), value A (HudDrawValueA) and the colour work
 * at $7E:E34F; with $0D62 set, the backdrop scroll (BackdropScrollFromView)
 * and value C for screen buffer 1 ($0D7F), else value B.
 * HOW: 8-bit tests; long loads of $0D2B and $0D7F; $7E:E34F (HudColourStep)
 * through call_routine.
 * Entry: any widths; D = $0000. Exit: as the last call leaves it.
 */
void HudFrameWhileGsuRuns(Carried *cr)
{
    cyc(62);
    uint32_t b;
    uint8_t v;

    w_SlideStep(cr);
    (cyc(62), FadeValueStep_cr(cr));
    (cyc(62), DataBankTest_cr(cr));
    r_sep(cr, P_M | P_X);
    b = (uint32_t)cr->db << 16;
    cyc(24);
    r_lda8(cr, b | W(wram_0D62));
    if ((uint8_t)cr->a) {
        cyc(72);
        r_lda8(cr, W(gsu_buffer_select));
        cr->y = (uint8_t)cr->a;
        cyc(90);
        run_cmp8(cr, (uint8_t)cr->y, 0x01);
        v = (uint8_t)cr->y;
        w_BackdropScrollFromView(cr);
        if (v == 0x01)
            { cyc(46); HudDrawValueC_cr(cr); }
        else
            { cyc(46); HudDrawValueB_cr(cr); }
        cyc(0);
        return;
    }
    cr->x = 0x00;
    cyc(70);
    r_lda8(cr, W(hud_mode));
    v = (uint8_t)cr->a;
    cyc(42);
    run_cmp8(cr, v, 0x01);
    if (v != 0x01) {
        run_cmp8(cr, v, 0x03);
        if (v != 0x03)
            { cyc(64); HudDigitSprites62_cr(cr); }
        cyc(12);
    }
    if (v != 0x03)
        { cyc(34); HudDrawDigits19EC_cr(cr); }
    cyc(46);
    BackdropRowsUpdate_cr(cr);
    (cyc(46), HudDrawValueA_cr(cr));
    cyc(62);
    HudColourStep_cr(cr);
}

void abi_HudFrameWhileGsuRuns(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    HudFrameWhileGsuRuns(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* Super FX program $01:AB13 started (program bank 1, screen mode $0374 |
   $18, the status cleared, R15 = $AB13) with M 8-bit and X 16-bit. */
static void start_ab13(Carried *cr, uint32_t b)
{
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0x01);
    cr->x = 0xAB13;
    set_p(cr, nz16(cr->p, 0xAB13));
    cyc(64);
    hw_wr(b | 0x3034, 0x01);
    cyc(24);
    cr->a = (uint16_t)((cr->a & 0xFF00) | (uint8_t)(rd8(b | W(gsu_scmr_shadow)) | 0x18));
    set_p(cr, nz8(cr->p, (uint8_t)cr->a));
    cyc(40);
    hw_wr(b | 0x303A, (uint8_t)cr->a);
    cyc(24);
    hw_wr(b | 0x3030, 0x00);
    cyc(24);
    hw_wr16(b | 0x301E, 0xAB13);
    cyc(46);
}

/*
 * @routine 7E:E258 FrameRunGsuAB13B
 * WHAT IT DOES: A frame for HUD panel B: clears the poll count $FE,
 * updates the renderer's parameters (GsuParamsUpdate), starts Super FX
 * program $01:AB13, and while it draws does the HUD's frame
 * (HudFrameWhileGsuRuns), the gauges (HudGaugesPanelB) and, with $0D62
 * set, the objects (FrameObjectsWhileGsuRuns); then waits (GsuWait), keeps
 * the poll count in $1F21 and draws the map markers (MapMarkersPanelB).
 * HOW: M 8-bit, X 16-bit; the Super FX started as GsuRun does, without
 * waiting.
 * Entry: D = $0000. Exit: as MapMarkersPanelB leaves it.
 */
void FrameRunGsuAB13B(Carried *cr)
{
    uint32_t b;

    r_sep(cr, P_M);
    r_rep(cr, P_X);
    cyc(38);
    wr8(W(gsu_wait_count), 0x00);
    cyc(16);
    wr8(W(gsu_wait_count_hi), 0x00);
    GsuParamsUpdate_cr(cr);
    b = (uint32_t)cr->db << 16;
    start_ab13(cr, b);
    HudFrameWhileGsuRuns(cr);
    (cyc(88), HudGaugesPanelB_cr(cr));
    r_sep(cr, P_M);
    b = (uint32_t)cr->db << 16;
    cyc(46);
    r_lda8(cr, b | W(wram_0D62));
    if ((uint8_t)cr->a)
        { cyc(62); w_FrameObjectsWhileGsuRuns(cr); cyc(88); }
    else
        cyc(68);                        /* the branch over the call, then the call to GsuWait */
    GsuWait_cr(cr);
    r_rep(cr, P_M);
    b = (uint32_t)cr->db << 16;
    cyc(16);
    r_lda16(cr, W(gsu_wait_count));
    cyc(24);
    wr16(b | W(gsu_ab13_polls), cr->a);
    (cyc(46), w_MapMarkersPanelB(cr));
    cyc(44);                            /* RTL */
}

void abi_FrameRunGsuAB13B(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    FrameRunGsuAB13B(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 7E:E298 FrameRunGsuAB13A
 * WHAT IT DOES: A frame for HUD panel A: updates the renderer's parameters,
 * clears the poll count $1F21, starts Super FX program $01:AB13, and while
 * it draws does the HUD's frame (HudFrameWhileGsuRuns) and the gauges
 * (HudGaugesPanelA); then waits (GsuWait) and draws the map markers
 * (MapMarkersPanelA).
 * HOW: As FrameRunGsuAB13B, without the objects or the poll count kept.
 * Entry: D = $0000. Exit: as MapMarkersPanelA leaves it.
 */
void FrameRunGsuAB13A(Carried *cr)
{
    uint32_t b;

    r_sep(cr, P_M);
    r_rep(cr, P_X);
    (cyc(22), GsuParamsUpdate_cr(cr));
    b = (uint32_t)cr->db << 16;
    cyc(24);
    wr8(b | W(gsu_ab13_polls), 0x00);
    cyc(24);
    wr8(b | 0x1F22, 0x00);
    start_ab13(cr, b);
    HudFrameWhileGsuRuns(cr);
    (cyc(88), HudGaugesPanelA_cr(cr));
    (cyc(46), GsuWait_cr(cr));
    (cyc(46), w_MapMarkersPanelA(cr));
    cyc(44);                            /* RTL */
}

void abi_FrameRunGsuAB13A(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    FrameRunGsuAB13A(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* ObjectDrawListBuild as its adapter leaves the registers. */
static void w_ObjectDrawListBuild(Carried *cr)
{
    uint8_t p = cr->p;
    ObjectDrawListEnd e = ObjectDrawListBuild(cr->y);

    cr->a = 0;
    cr->x = e.x;
    cr->y = e.y;
    set_p(cr, p);
}

/* BackdropScrollFromView as its adapter leaves the registers. */
static void w_BackdropScrollFromView(Carried *cr)
{
    cr->a = BackdropScrollFromView(cr->db, &cr->x, !(cr->p & P_M));
}

/* SlideStep as its adapter leaves the registers. */
static void w_SlideStep(Carried *cr)
{
    cr->a = SlideStep(cr->a);
    cyc(100);
}

/* FrameObjectsWhileGsuRuns as its adapter leaves the registers. */
static void w_FrameObjectsWhileGsuRuns(Carried *cr)
{
    FrameObjectsWhileGsuRuns(cr);
}

/* MapMarkersPanelB as its adapter leaves the registers. */
static void w_MapMarkersPanelB(Carried *cr)
{
    uint16_t x, y;
    uint8_t p0 = cr->p;

    set_p(cr, (uint8_t)((p0 | P_M) & ~(P_N | P_Z)));
    if (!MapMarkersPanelB(cr->db, &x, &y)) {
        set_p(cr, p0);
        cr->a &= 0xFF00;
        set_p(cr, (uint8_t)(nz8(cr->p, 0) | P_M));
        return;
    }
    /* N, Z from the first slot, which is never 0 or negative */
    set_p(cr, (uint8_t)((cr->p | P_M) & ~(P_N | P_Z)));
    cr->x = x;
    cr->y = y;
    cr->a = 0;
    if (cr->p & P_X) {
        cr->x &= 0x00FF;
        cr->y &= 0x00FF;
    }
}

/* MapMarkersPanelA as its adapter leaves the registers. */
static void w_MapMarkersPanelA(Carried *cr)
{
    uint16_t y;

    /* an interrupt inside the markers pushes the P they run under */
    set_p(cr, (uint8_t)nz16((uint8_t)(cr->p & ~P_M), 0x0158));
    cr->x = MapMarkersPanelA(cr->db, &y);
    cr->y = y;
    cr->a = 0;
    if (cr->p & P_X) {
        cr->x &= 0x00FF;
        cr->y &= 0x00FF;
    }
}
