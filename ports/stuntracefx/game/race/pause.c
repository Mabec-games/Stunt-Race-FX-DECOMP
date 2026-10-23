/*
 * pause.c: the stages of the race's pause screens, one step a frame: the
 * pause by pad 1 or pad 2 (OamStagePause, OamStagePause2, through the
 * tables at $07:BB3C and $07:BBA2) and the replay's pause (PauseReplayStages,
 * through $07:BC24). The stage is $0D2F, its timer $0D5F; the continue / quit
 * choice is $0D3D and the quit menu's cursor $0D50 (0, 2, 4; 6 for the
 * cars). All run with 8-bit registers and data bank $07.
 */
#include "machine.h"
#include "cpu_ops.h"
#include "carried.h"

uint8_t Countdown0DF8(uint8_t bank);
#include "hud_calls.h"
uint8_t OamBlock123Left(uint8_t bank, uint8_t *p);
uint8_t OamBlock123Right(uint8_t bank, uint8_t *p);
uint8_t OamBlock52Left(uint8_t bank, uint8_t *p);
uint8_t OamBlock52Right(uint8_t bank, uint8_t *p);
uint8_t OamBlock59Left(uint8_t bank, uint8_t *p);
uint8_t OamBlock59Right(uint8_t bank, uint8_t *p);
void OamHighBits18Park48(uint8_t bank);
void OamHighBits58Park112(uint8_t bank);
void OamPlaceRows75And112(uint8_t bank);
uint8_t WindowSlideUpSound(uint8_t bank, int x16);

void abi_SoundPort0QueuePush(void);
void abi_SoundPort1QueuePush(void);
void abi_SoundPort1QueuePushX8(void);
void abi_WindowSlideUpSound(void);
void abi_Gauge6BPlace(void);
void abi_Gauge6APlace(void);
void abi_OamPlaceRows75And112(void);
void abi_ChampionshipScreenLoop(void);
void abi_RaceRestart(void);
void abi_RaceEndVideoReset(void);
void abi_RaceEndToAttract(void);
void abi_SoftResetToBootWarm(void);
void abi_OamBlock52Left(void);
void abi_OamBlock52Right(void);
void abi_OamBlock59Left(void);
void abi_OamBlock59Right(void);
void abi_OamBlock123Left(void);
void abi_OamBlock123Right(void);
void abi_Countdown0DF8(void);
void abi_OamHighBits58Park112(void);
void abi_OamHighBits18Park48(void);
void abi_CarSelectTwoPlayers(void);
void abi_SpritePiecesTwoCars(void);
void abi_SpritePiecesTwoCarsTop(void);
void abi_TilemapPaletteCycle87(void);
void abi_TilemapPaletteCycle107(void);
void abi_TilemapPaletteCycle187(void);
void abi_TilemapRow87Palette5(void);
void abi_TilemapRow107Palette5(void);
void abi_TilemapRow187Palette5(void);

void PauseQuitMenu(Carried *cr);
void PauseQuitMenuBack(Carried *cr);
void PauseQuitMenuCursor0(Carried *cr);
void PauseQuitMenuP2(Carried *cr);
void PauseStageChoice(Carried *cr);
void PauseStageChoiceP1(Carried *cr);
void PauseStageChoiceP2(Carried *cr);
void PauseStageContinue(Carried *cr);
void PauseStageContinueP2(Carried *cr);
void PauseStageLeave(Carried *cr);
void PauseStageQuit(Carried *cr);
void PauseStageQuitMenu(Carried *cr);
void PauseStageQuitMenuP2(Carried *cr);
void PauseStageQuitTaken(Carried *cr);
void PauseStageQuitWait(Carried *cr);
void PauseStageReplayEnd(Carried *cr);
void PauseStageResumeA(Carried *cr);
void PauseStageResumeB(Carried *cr);
void PauseStageStart(Carried *cr);
void PauseStageWait(Carried *cr);
static void choice_made(Carried *cr, uint32_t b, int quit_extra);
static void choice_move(Carried *cr, uint32_t b, uint16_t pad, int wide, void (*left)(Carried *), void (*right)(Carried *));
static void cmp8_imm(Carried *cr, uint8_t m);
static void continue_wait(Carried *cr, uint8_t owner);
static int cursor_move(Carried *cr, uint32_t b, uint16_t pad, uint16_t bit, uint8_t to);
static uint8_t dec8_mem(Carried *cr, uint32_t addr);
static void lda8_imm(Carried *cr, uint8_t v);
static void pause_resume(Carried *cr, uint32_t b);
static int presses(Carried *cr, uint32_t b, uint16_t pad, uint16_t mask);
static void quit_menu_open(Carried *cr, uint32_t b);
static void quit_menu_take(Carried *cr, uint32_t b, uint16_t which, uint8_t d, void (*park)(Carried *));
static void rows(Carried *cr, void (*r87)(Carried *), void (*r107)(Carried *), void (*r187)(Carried *));
static void slide_up(Carried *cr, uint32_t b, uint8_t v);
static void stage(Carried *cr, uint32_t b, uint8_t s);
static int timer_out(Carried *cr, uint32_t b);
static int window_closed(Carried *cr, uint32_t b);
static void w_ChampionshipScreenLoop(Carried *cr);
static void w_Countdown0DF8(Carried *cr);
static void w_Gauge6APlace(Carried *cr);
static void w_Gauge6BPlace(Carried *cr);
static void w_OamBlock123Left(Carried *cr);
static void w_OamBlock123Right(Carried *cr);
static void w_OamBlock52Left(Carried *cr);
static void w_OamBlock52Right(Carried *cr);
static void w_OamBlock59Left(Carried *cr);
static void w_OamBlock59Right(Carried *cr);
static void w_OamHighBits18Park48(Carried *cr);
static void w_OamHighBits58Park112(Carried *cr);
static void w_OamPlaceRows75And112(Carried *cr);
static void w_RaceEndToAttract(Carried *cr);
static void w_RaceRestart(Carried *cr);
static void w_SoftResetToBootWarm(Carried *cr);
static void w_SoundPort1QueuePushX8(Carried *cr);
static void w_SpritePiecesTwoCars(Carried *cr);
static void w_SpritePiecesTwoCarsTop(Carried *cr);
static void w_TilemapPaletteCycle107(Carried *cr);
static void w_TilemapPaletteCycle187(Carried *cr);
static void w_TilemapPaletteCycle87(Carried *cr);
static void w_TilemapRow107Palette5(Carried *cr);
static void w_TilemapRow187Palette5(Carried *cr);
static void w_TilemapRow87Palette5(Carried *cr);
static void w_WindowSlideUpSound(Carried *cr);
void RaceEndVideoReset(Carried *cr);
void SoundPort0QueuePush_cr(Carried *cr);
void SoundPort1QueuePush_cr(Carried *cr);

/* LDA #v (8-bit). */
static void lda8_imm(Carried *cr, uint8_t v)
{
    cyc(24);
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    cr->p = nz8(cr->p, v);
}

static void lda8_imm_g(uint8_t v)
{
    Carried c = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};
    lda8_imm(&c, v);

    cpu.a = c.a;
    cpu.x = c.x;
    cpu.y = c.y;
    cpu.d = c.d;
    cpu.p = c.p;
    cpu.db = c.db;
}

/* DEC of a work-RAM byte; N, Z from it. */
static uint8_t dec8_mem(Carried *cr, uint32_t addr)
{
    cyc(24);
    uint8_t v = (uint8_t)(rd8(addr) - 1);

    cyc(6);
    wr8(addr, v);
    cr->p = nz8(cr->p, v);
    return v;
}

/* CMP # (8-bit). */
static void cmp8_imm(Carried *cr, uint8_t m)
{
    uint8_t a = (uint8_t)cr->a;

    cr->p = (uint8_t)((nz8(cr->p, (uint8_t)(a - m)) & ~P_C) | (a >= m ? (cyc(38), P_C) : (cyc(32), 0)));
}

/* REP #$20, LDA presses, BIT #mask: Z from the test. */
static int presses(Carried *cr, uint32_t b, uint16_t pad, uint16_t mask)
{
    r_rep(cr, P_M);
    cyc(2);
    r_lda16(cr, b | pad);
    cr->p = (uint8_t)((cr->p & ~P_Z) | ((cr->a & mask) ? (cyc(80), 0) : (cyc(46), P_Z)));
    return (cr->a & mask) != 0;
}

static int presses_g(uint32_t b, uint16_t pad, uint16_t mask)
{
    Carried c = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};
    int ret_ = presses(&c, b, pad, mask);

    cpu.a = c.a;
    cpu.x = c.x;
    cpu.y = c.y;
    cpu.d = c.d;
    cpu.p = c.p;
    cpu.db = c.db;
    return ret_;
}

/* SEP #$20, the timer $0D5F counted down; 1 at its end. */
static int timer_out(Carried *cr, uint32_t b)
{
    r_sep(cr, P_M);
    cyc(22);
    return dec8_mem(cr, b | 0x0D5F) == 0;
}

/* The stage $0D2F = s. */
static void stage(Carried *cr, uint32_t b, uint8_t s)
{
    lda8_imm(cr, s);
    wr8(b | 0x0D2F, s);
}

/* 07:BC88: the race resumes: sound $16 on port 1, the presses' low bytes
   ($0D1A, $0D1C) and $0D2D cleared, $0E5B and $0E5F one less. */
static void pause_resume(Carried *cr, uint32_t b)
{
    cyc(54);
    lda8_imm(cr, 0x16);
    SoundPort1QueuePush_cr(cr);
    cyc(24);
    wr8(b | 0x0D1A, 0x00);
    cyc(24);
    wr8(b | 0x0D1C, 0x00);
    r_sep(cr, P_M);
    cyc(46);
    wr8(b | 0x0D2D, 0x00);
    dec8_mem(cr, b | 0x0E5B);
    dec8_mem(cr, b | 0x0E5F);
    cyc(42);
}

/*
 * @routine 07:BC32 PauseStageStart
 * WHAT IT DOES: The pause's first stage: the timer $0D5F = $A, stage 2.
 * HOW: 8-bit stores.
 * Entry: any widths. Exit: M 8-bit; A low byte = 2.
 */
void PauseStageStart(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;

    r_sep(cr, P_M);
    lda8_imm(cr, 0x0A);
    cyc(38);
    wr8(b | 0x0D5F, 0x0A);
    cyc(16);
    stage(cr, b, 0x02);
    cyc(42);
}

void abi_PauseStageStart(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PauseStageStart(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:BC3F PauseStageWait
 * WHAT IT DOES: Counts the timer $0D5F down; at its end stage 4.
 * HOW: DEC of the byte.
 * Entry: any widths. Exit: M 8-bit.
 */
void PauseStageWait(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;

    if (timer_out(cr, b))
        { cyc(32); stage(cr, b, 0x04); } else { cyc(22); }
    cyc(42);
}

void abi_PauseStageWait(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PauseStageWait(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:BC4C PauseStageResumeB
 * WHAT IT DOES: Counts the timer $0D5F down; at its end gauge 6B placed
 * (Gauge6BPlace) and the race resumed: sound $16 on port 1, the presses'
 * low bytes and $0D2D cleared, $0E5B and $0E5F one less.
 * HOW: 8-bit.
 * Entry: any widths. Exit: M 8-bit.
 */
void PauseStageResumeB(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;

    if (!timer_out(cr, b))
        { cyc(64); return; } else { cyc(62); }
    w_Gauge6BPlace(cr);
    cyc(22);
    pause_resume(cr, b);
}

void abi_PauseStageResumeB(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PauseStageResumeB(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:BC58 PauseStageResumeA
 * WHAT IT DOES: As PauseStageResumeB with gauge 6A (Gauge6APlace).
 * HOW: As PauseStageResumeB. Reached only by unit_pause.
 * Entry: any widths. Exit: M 8-bit.
 */
void PauseStageResumeA(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;

    if (!timer_out(cr, b))
        { cyc(64); return; } else { cyc(62); }
    w_Gauge6APlace(cr);
    cyc(22);
    pause_resume(cr, b);
}

void abi_PauseStageResumeA(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PauseStageResumeA(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:BC64 PauseStageReplayEnd
 * WHAT IT DOES: Counts the timer $0D5F down: at 1 the sprite rows
 * (OamPlaceRows75And112); at its end $0E14 = $93, $0E0C = $7C, $0E18 =
 * $8C, $0D22 cleared and the race resumed (as PauseStageResumeB).
 * HOW: 8-bit.
 * Entry: any widths. Exit: M 8-bit.
 */
void PauseStageReplayEnd(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;

    if (!timer_out(cr, b)) {
        cyc(40);
        r_lda8(cr, b | 0x0D5F);
        cmp8_imm(cr, 0x01);
        if ((uint8_t)cr->a == 0x01)
            w_OamPlaceRows75And112(cr);
        cyc(42);
        return;
    }
    lda8_imm(cr, 0x93);
    cyc(38);
    wr8(b | 0x0E14, 0x93);
    lda8_imm(cr, 0x7C);
    cyc(16);
    wr8(b | 0x0E0C, 0x7C);
    lda8_imm(cr, 0x8C);
    cyc(16);
    wr8(b | 0x0E18, 0x8C);
    cyc(24);
    wr8(b | 0x0D22, 0x00);
    pause_resume(cr, b);
}

void abi_PauseStageReplayEnd(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PauseStageReplayEnd(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* WindowSlideUpSound, then $19DF = v. */
static void slide_up(Carried *cr, uint32_t b, uint8_t v)
{
    (cyc(62), w_WindowSlideUpSound(cr));
    lda8_imm(cr, v);
    cyc(16);
    wr8(b | 0x19DF, v);
}

/*
 * @routine 07:BCA0 PauseStageQuit
 * WHAT IT DOES: Counts the timer $0D5F down; at its end slides the window
 * up: in mode 0 ($0D2B) stage $A with $19DF = 1 and the timer $70; in
 * other modes stage $C with $19DF cleared.
 * HOW: 8-bit.
 * Entry: any widths. Exit: M 8-bit.
 */
void PauseStageQuit(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;

    if (!timer_out(cr, b))
        { cyc(58); return; }
    cyc(46);
    r_lda8(cr, b | 0x0D2B);
    if (!(uint8_t)cr->a) {
        cyc(32);
        stage(cr, b, 0x0A);
        slide_up(cr, b, 0x01);
        lda8_imm(cr, 0x70);
        cyc(16);
        wr8(b | 0x0D5F, 0x70);
        cyc(42);
        return;
    } else { cyc(38); }
    stage(cr, b, 0x0C);
    slide_up(cr, b, 0x00);
    cyc(42);
}

void abi_PauseStageQuit(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PauseStageQuit(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:BCE0 PauseStageLeave
 * WHAT IT DOES: Leaves the race from the pause: with $10CD set, the fade
 * out ($0E3A = 1) and, once it is done ($0D61 negative), the restart
 * (SoftResetToBootWarm); else $0D59 cleared, the championship screen's
 * loop, then the race again (RaceRestart, with $0DD4) or the video reset
 * (RaceEndVideoReset).
 * HOW: 8-bit; jumps on. The race again is not reached in the traces so far.
 * Entry: any widths. Exit: M 8-bit, or does not return.
 */
void PauseStageLeave(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;

    r_sep(cr, P_M);
    cyc(46);
    r_lda8(cr, b | 0x10CD);
    if ((uint8_t)cr->a) {
        lda8_imm(cr, 0x01);
        cyc(32);
        wr8(b | 0x0E3A, 0x01);
        cyc(24);
        r_lda8(cr, b | 0x0D61);
        if ((uint8_t)cr->a & 0x80)
            { cyc(40); w_SoftResetToBootWarm(cr); } else { cyc(64); }
        return;
    }
    cyc(46);
    wr8(b | 0x0D59, 0x00);
    (cyc(46), w_ChampionshipScreenLoop(cr));
    r_sep(cr, P_M);
    cyc(46);
    r_lda8(cr, b | 0x0DD4);
    if ((uint8_t)cr->a)
        { cyc(40); w_RaceRestart(cr); }
    else
        RaceEndVideoReset(cr);
}

void abi_PauseStageLeave(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PauseStageLeave(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:BCD0 PauseStageQuitWait
 * WHAT IT DOES: Counts the timer $0D5F down; at its end the race again
 * (RaceRestart) with $0D24 set, else PauseStageLeave.
 * HOW: 8-bit; jumps on.
 * Entry: any widths. Exit: M 8-bit, or does not return.
 */
void PauseStageQuitWait(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;

    if (!timer_out(cr, b))
        { cyc(64); return; }
    cyc(40);
    r_lda8(cr, b | 0x0D24);
    if ((uint8_t)cr->a)
        { cyc(40); w_RaceRestart(cr); }
    else
        { cyc(22); PauseStageLeave(cr); }
}

void abi_PauseStageQuitWait(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PauseStageQuitWait(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* B or START: the choice made: $19BD = $FF and the timer 8; quit ($0D3D
   set): stage 8, and in mode 0 without the override $0D4C, Countdown0DF8
   and sound $22 on port 1; continue: stage 6. */
static void choice_made(Carried *cr, uint32_t b, int quit_extra)
{
    r_sep(cr, P_M);
    cyc(12);
    r_lda8(cr, b | 0x0D3D);
    if ((uint8_t)cr->a) {
        lda8_imm(cr, 0xFF);
        cyc(32);
        wr8(b | 0x19BD, 0xFF);
        cyc(16);
        stage(cr, b, 0x08);
        lda8_imm(cr, 0x08);
        cyc(16);
        wr8(b | 0x0D5F, 0x08);
        if (!quit_extra)
            return;
        cyc(24);
        r_lda8(cr, b | 0x0D2B);
        if ((uint8_t)cr->a)
            { cyc(22); return; }
        cyc(40);
        r_lda8(cr, b | 0x0D4C);
        if ((uint8_t)cr->a)
            { cyc(22); return; } else { cyc(78); }
        w_Countdown0DF8(cr);
        cyc(54);
        lda8_imm(cr, 0x22);
        w_SoundPort1QueuePushX8(cr);
        return;
    }
    r_sep(cr, P_M);
    lda8_imm(cr, 0xFF);
    cyc(60);
    wr8(b | 0x19BD, 0xFF);
    cyc(16);
    stage(cr, b, 0x06);
    lda8_imm(cr, 0x08);
    cyc(16);
    wr8(b | 0x0D5F, 0x08);
}

/* Left (bit 9) on continue: sound $45, $0D3D one less, left(); right (bit
   8) on quit: sound $45, $0D3D cleared, right(). `wide` reads $0D3D as a
   word (masked) and tests right with M 8-bit set first. */
static void choice_move(Carried *cr, uint32_t b, uint16_t pad, int wide, void (*left)(Carried *), void (*right)(Carried *))
{
    uint8_t c;

    if (wide) {
        cyc(18);
        r_lda16(cr, b | 0x0D3D);
        r_and16(cr, 0x00FF);
        c = (uint8_t)cr->a;
    } else {
        r_sep(cr, P_M);
        cyc(46);
        r_lda8(cr, b | 0x0D3D);
        c = (uint8_t)cr->a;
    }
    cyc(38);
    if (!c) {
        if (!presses(cr, b, pad, 0x0200))
            return;
        r_lda16_imm(cr, 0x0045);
        cyc(8);
        SoundPort0QueuePush_cr(cr);
        cyc(8);
        r_sep(cr, P_M);
        cyc(14);
        dec8_mem(cr, b | 0x0D3D);
        (cyc(46), left(cr));
        return;
    } else { cyc(6); }
    if (wide) {
        cyc(22);
        presses(cr, b, pad, 0x0100);
        r_sep(cr, P_M);
        if (!(cr->a & 0x0100))
            { cyc(22); return; } else { cyc(20); }
        lda8_imm(cr, 0x45);
    } else {
        if (!presses(cr, b, pad, 0x0100))
            return;
        r_lda16_imm(cr, 0x0045);
    }
    cyc(wide ? 16 : 8);
    SoundPort0QueuePush_cr(cr);
    r_sep(cr, P_M);
    cyc(46);
    wr8(b | 0x0D3D, 0x00);
    (cyc(46), right(cr));
}

/*
 * @routine 07:BD09 PauseStageChoice
 * WHAT IT DOES: The replay pause's continue / quit choice for pad 1:
 * left and right move it ($0D3D, sprites 52-56: OamBlock52Left /
 * OamBlock52Right, sound $45); B or START takes it (see choice_made, with
 * the countdown and sound $22 for quit in mode 0).
 * HOW: The presses $0D1A 16-bit.
 * Entry: any widths. Exit: M 8-bit, or 16-bit when nothing moved.
 */
void PauseStageChoice(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;

    cyc(22);
    if (presses(cr, b, 0x0D1A, 0x9000)) {
        cyc(18);
        choice_made(cr, b, 1);
        cyc(42);
        return;
    }
    choice_move(cr, b, 0x0D1A, 0, w_OamBlock52Left, w_OamBlock52Right);
    cyc(42);
}

void abi_PauseStageChoice(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PauseStageChoice(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:BD8D PauseStageChoiceP1
 * WHAT IT DOES: The pause's continue / quit choice for pad 1: left and
 * right move it ($0D3D, sprites 59-63: OamBlock59Left / OamBlock59Right,
 * sound $45); B or START takes it: $19BD = $FF, the timer 8, stage 8 for
 * quit or 6 for continue.
 * HOW: The presses $0D1A 16-bit; $0D3D read as a word, masked. The move
 * right is not reached in the traces so far.
 * Entry: any widths. Exit: M 8-bit, or 16-bit when nothing moved.
 */
void PauseStageChoiceP1(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;

    cyc(22);
    if (presses(cr, b, 0x0D1A, 0x9000)) {
        choice_made(cr, b, 0);
        cyc(42);
        return;
    }
    choice_move(cr, b, 0x0D1A, 1, w_OamBlock59Left, w_OamBlock59Right);
    cyc(42);
}

void abi_PauseStageChoiceP1(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PauseStageChoiceP1(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:BDFB PauseStageChoiceP2
 * WHAT IT DOES: As PauseStageChoiceP1 for pad 2 ($0D1C; sprites 123-127:
 * OamBlock123Left / OamBlock123Right).
 * HOW: As PauseStageChoiceP1. Reached only by unit_pause.
 * Entry: any widths. Exit: as PauseStageChoiceP1.
 */
void PauseStageChoiceP2(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;

    cyc(22);
    if (presses(cr, b, 0x0D1C, 0x9000)) {
        choice_made(cr, b, 0);
        cyc(42);
        return;
    }
    choice_move(cr, b, 0x0D1C, 1, w_OamBlock123Left, w_OamBlock123Right);
    cyc(42);
}

void abi_PauseStageChoiceP2(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PauseStageChoiceP2(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* Counts the timer $0D5F down; at 0 $0D2E = owner, $19DF cleared, the
   window slid up and stage $A. */
static void continue_wait(Carried *cr, uint8_t owner)
{
    uint32_t b = (uint32_t)cr->db << 16;

    r_sep(cr, P_M);
    cyc(46);
    r_lda8(cr, b | 0x0D5F);
    if ((uint8_t)cr->a) {
        cyc(16);
        dec8_mem(cr, b | 0x0D5F);
        return;
    }
    lda8_imm(cr, owner);
    cyc(38);
    wr8(b | 0x0D2E, owner);
    if (owner == 0xFF)
        cyc(22);   /* BRA */
    lda8_imm(cr, 0x00);
    cyc(16);
    wr8(b | 0x19DF, 0x00);
    (cyc(62), w_WindowSlideUpSound(cr));
    cyc(16);
    stage(cr, b, 0x0A);
}

/*
 * @routine 07:BE40 PauseStageContinueP2
 * WHAT IT DOES: Counts the timer $0D5F down; at its end $0D2E = $FF (pad
 * 2's), $19DF cleared, the window slid up and stage $A.
 * HOW: 8-bit. Reached only by unit_pause.
 * Entry: any widths. Exit: M 8-bit.
 */
void PauseStageContinueP2(Carried *cr)
{
    continue_wait(cr, 0xFF);
    cyc(42);
}

void abi_PauseStageContinueP2(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PauseStageContinueP2(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:BE52 PauseStageContinue
 * WHAT IT DOES: As PauseStageContinueP2 with $0D2E cleared (pad 1's).
 * HOW: 8-bit.
 * Entry: any widths. Exit: M 8-bit.
 */
void PauseStageContinue(Carried *cr)
{
    continue_wait(cr, 0x00);
    cyc(42);
}

void abi_PauseStageContinue(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PauseStageContinue(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* 07:BEAB: the quit menu opened: $19DF cleared, the split window state 2,
   the cursor $0D50 cleared, stage $C and $0D22 = $C. */
static void quit_menu_open(Carried *cr, uint32_t b)
{
    lda8_imm(cr, 0x00);
    cyc(16);
    wr8(b | 0x19DF, 0x00);
    lda8_imm(cr, 0x02);
    cyc(16);
    wr8(b | 0x19D8, 0x02);
    cyc(16);
    stage(cr, b, 0x0A);
    cyc(24);
    wr8(b | 0x0D50, 0x00);
    cyc(16);
    stage(cr, b, 0x0C);
    cyc(24);
    wr8(b | 0x0D22, 0x0C);
    cyc(42);
}

/* LDA $19D8, CMP #5: the split window closed. */
static int window_closed(Carried *cr, uint32_t b)
{
    cyc(24);
    r_lda8(cr, b | 0x19D8);
    cmp8_imm(cr, 0x05);
    return (uint8_t)cr->a == 0x05;
}

/*
 * @routine 07:BE71 PauseStageQuitMenuP2
 * WHAT IT DOES: Once the split window is closed: $0D2E = $FF, $0D5D = 1,
 * $0D23 = $E, sprites 58 up parked (OamHighBits58Park112), and the quit
 * menu opened ($19DF cleared, window state 2, cursor $0D50 cleared, stage
 * $C, $0D22 = $C).
 * HOW: 8-bit.
 * Entry: M 8-bit. Exit: M 8-bit.
 */
void PauseStageQuitMenuP2(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;

    if (!window_closed(cr, b))
        { cyc(48); return; }   /* BNE taken, RTS */
    r_sep(cr, P_M);
    lda8_imm(cr, 0xFF);
    cyc(32);
    wr8(b | 0x0D2E, 0xFF);
    lda8_imm(cr, 0x01);
    cyc(16);
    wr8(b | 0x0D5D, 0x01);
    lda8_imm(cr, 0x0E);
    cyc(16);
    wr8(b | 0x0D23, 0x0E);
    w_OamHighBits58Park112(cr);
    cyc(22);
    quit_menu_open(cr, b);
}

void abi_PauseStageQuitMenuP2(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PauseStageQuitMenuP2(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:BE8F PauseStageQuitMenu
 * WHAT IT DOES: As PauseStageQuitMenuP2 for pad 1: $0D2E cleared, $0D5C =
 * 1, $0D23 = $C, sprites 18 up parked (OamHighBits18Park48).
 * HOW: 8-bit.
 * Entry: M 8-bit. Exit: M 8-bit.
 */
void PauseStageQuitMenu(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;

    if (!window_closed(cr, b))
        { cyc(42); return; }
    r_sep(cr, P_M);
    lda8_imm(cr, 0x00);
    cyc(38);
    wr8(b | 0x0D2E, 0x00);
    lda8_imm(cr, 0x01);
    cyc(16);
    wr8(b | 0x0D5C, 0x01);
    lda8_imm(cr, 0x0C);
    cyc(16);
    wr8(b | 0x0D23, 0x0C);
    w_OamHighBits18Park48(cr);
    quit_menu_open(cr, b);
}

void abi_PauseStageQuitMenu(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PauseStageQuitMenu(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:BEC6 PauseStageQuitTaken
 * WHAT IT DOES: Once the split window is closed: the race again
 * (RaceRestart) with the cursor $0D50 not 0, else RaceEndToAttract.
 * HOW: 8-bit; jumps on.
 * Entry: any widths. Exit: M 8-bit, or does not return.
 */
void PauseStageQuitTaken(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;

    r_sep(cr, P_M);
    cyc(22);
    if (!window_closed(cr, b))
        { cyc(48); return; }
    cyc(18);
    r_lda8(cr, b | 0x0D50);
    if ((uint8_t)cr->a)
        { cyc(40); w_RaceRestart(cr); }
    else
        { cyc(46); w_RaceEndToAttract(cr); }
}

void abi_PauseStageQuitTaken(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PauseStageQuitTaken(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* The quit menu's rows: the cursor's row cycles its palette, the others
   palette 5 (rows 87, 107, 187 on the screen). */
static void rows(Carried *cr, void (*r87)(Carried *), void (*r107)(Carried *), void (*r187)(Carried *))
{
    (cyc(22), r87(cr));
    (cyc(46), r107(cr));
    (cyc(46), r187(cr));
    cyc(42);
}

static void rows_g(void (*r87)(Carried *), void (*r107)(Carried *), void (*r187)(Carried *))
{
    Carried c = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};
    rows(&c, r87, r107, r187);

    cpu.a = c.a;
    cpu.x = c.x;
    cpu.y = c.y;
    cpu.d = c.d;
    cpu.p = c.p;
    cpu.db = c.db;
}

/* Down (bit 10) or up (bit 11) on pad: sound $45 and the cursor $0D50 =
   to; returns 1 when taken. */
static int cursor_move(Carried *cr, uint32_t b, uint16_t pad, uint16_t bit, uint8_t to)
{
    cyc(68);
    if (!presses(cr, b, pad, bit))
        return 0; else { cyc(20); }
    r_sep(cr, P_M);
    lda8_imm(cr, 0x45);
    cyc(16);
    SoundPort0QueuePush_cr(cr);
    cyc(16);
    lda8_imm(cr, to);
    wr8(b | 0x0D50, to);
    return 1;
}

static int cursor_move_g(uint32_t b, uint16_t pad, uint16_t bit, uint8_t to)
{
    Carried c = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};
    int ret_ = cursor_move(&c, b, pad, bit, to);

    cpu.a = c.a;
    cpu.x = c.x;
    cpu.y = c.y;
    cpu.d = c.d;
    cpu.p = c.p;
    cpu.db = c.db;
    return ret_;
}

/* B or START in the quit menu: cursor 0 closes it (sound $46, stage $E,
   $19DF = $19D8 = 1); 2 or 6 back to 0 (sound $46); 4 the cars (sound $46,
   cursor 6), the first time also $10BA = $FF, `which` = $FF, $0D23 = d and
   park(). */
static void quit_menu_take(Carried *cr, uint32_t b, uint16_t which, uint8_t d, void (*park)(Carried *))
{
    cyc(6);
    r_lda8(cr, b | 0x0D50);
    cmp8_imm(cr, 0x04);
    if ((uint8_t)cr->a == 0x04) {
        cyc(24);
        r_lda8(cr, b | 0x10BA);
        if (!(uint8_t)cr->a) {
            lda8_imm(cr, 0xFF);
            cyc(32);
            wr8(b | 0x10BA, 0xFF);
            lda8_imm(cr, 0xFF);
            cyc(16);
            wr8(b | which, 0xFF);
            lda8_imm(cr, d);
            cyc(16);
            wr8(b | 0x0D23, d);
            park(cr);
        } else { cyc(22); }                /* BNE taken */
        cyc(38);
        lda8_imm(cr, 0x46);
        cyc(16);
        SoundPort0QueuePush_cr(cr);
        cyc(16);
        lda8_imm(cr, 0x06);
        wr8(b | 0x0D50, 0x06);
        return;
    } else { cyc(54); }
    if ((uint8_t)cr->a > 0x04) {
        lda8_imm(cr, 0x46);
        cyc(16);
        SoundPort0QueuePush_cr(cr);
        cyc(24);
        wr8(b | 0x0D50, 0x00);
        return;
    }
    lda8_imm(cr, 0x46);
    cyc(16);
    SoundPort0QueuePush_cr(cr);
    cyc(16);
    stage(cr, b, 0x0E);
    lda8_imm(cr, 0x01);
    cyc(16);
    wr8(b | 0x19DF, 0x01);
    cyc(24);
    wr8(b | 0x19D8, 0x01);
}

static void quit_menu_take_g(uint32_t b, uint16_t which, uint8_t d, void (*park)(Carried *))
{
    Carried c = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};
    quit_menu_take(&c, b, which, d, park);

    cpu.a = c.a;
    cpu.x = c.x;
    cpu.y = c.y;
    cpu.d = c.d;
    cpu.p = c.p;
    cpu.db = c.db;
}

/* The quit menu for pad: B or START takes the cursor; else by the cursor
   $0D50: 0 down to 2; 2 down to 4 or up to 0; 4 up to 2; 6 the cars
   (CarSelectTwoPlayers, then cars()); each with its rows. */
static void quit_menu(uint16_t pad, uint16_t which, uint8_t d, void (*park)(Carried *), void (*cars)(Carried *))
{
    uint32_t b = (uint32_t)cpu.db << 16;

    op_sep(P_M | P_X);
    cyc(22);
    presses_g(b, pad, 0x9000);
    op_sep(P_M);
    if (cpu.a & 0x9000) {
        quit_menu_take_g(b, which, d, park);
        cyc(42);
        return;
    }
    cyc(46);
    cpu.x = rd8(b | 0x0D50);
    cpu.p = nz8(cpu.p, (uint8_t)cpu.x);
    switch (cpu.x) {
    case 0x00:
        cursor_move_g(b, pad, 0x0400, 0x02);
        cyc(46);
        op_sep(P_M);
        rows_g(w_TilemapPaletteCycle87, w_TilemapRow107Palette5, w_TilemapRow187Palette5);
        return;
    case 0x02:
        if (!cursor_move_g(b, pad, 0x0400, 0x04)) {
            presses_g(b, pad, 0x0800);
            if (cpu.a & 0x0800) {
                cyc(20);
                op_sep(P_M);
                lda8_imm_g(0x45);
                cyc(16);
                abi_SoundPort0QueuePush();
                cyc(16);
                lda8_imm_g(0x00);
                wr8(b | 0x0D50, 0x00);
            }
        } else { cyc(22); }
        cyc(46);
        op_sep(P_M);
        rows_g(w_TilemapRow87Palette5, w_TilemapPaletteCycle107, w_TilemapRow187Palette5);
        return;
    case 0x04:
        cursor_move_g(b, pad, 0x0800, 0x02);
        cyc(46);
        break;
    case 0x06:
        cyc(92);
        abi_CarSelectTwoPlayers();
        (cyc(46), cr_call(cars));
        break;
    default:
        jump_routine(0x070000 | rd16(0x070000 | (uint16_t)((pad == 0x0D1A ? 0xC02D : 0xBF39) + cpu.x)));
        return;
    }
    op_sep(P_M);
    rows_g(w_TilemapPaletteCycle187, w_TilemapRow107Palette5, w_TilemapRow87Palette5);
}

static void quit_menu_c(Carried *cr, uint16_t pad, uint16_t which, uint8_t d, void (*park)(Carried *), void (*cars)(Carried *))
{
    cpu.a = cr->a;
    cpu.x = cr->x;
    cpu.y = cr->y;
    cpu.d = cr->d;
    cpu.p = cr->p;
    cpu.db = cr->db;
    quit_menu(pad, which, d, park, cars);
    cr->a = cpu.a;
    cr->x = cpu.x;
    cr->y = cpu.y;
    cr->d = cpu.d;
    cr->p = cpu.p;
    cr->db = cpu.db;
}

/*
 * @routine 07:BEDB PauseQuitMenuP2
 * WHAT IT DOES: The quit menu for pad 2 ($0D1C): see PauseQuitMenu; the
 * cars first opened set $0D5C = $FF, $0D23 = $C and park sprites 18 up
 * (OamHighBits18Park48); the cars drawn with SpritePiecesTwoCarsTop.
 * HOW: JMP ($BF39,X) on the cursor. Reached only by unit_pause.
 * Entry: any widths. Exit: as the path leaves it.
 */
void PauseQuitMenuP2(Carried *cr)
{
    quit_menu_c(cr, 0x0D1C, 0x0D5C, 0x0C, w_OamHighBits18Park48, w_SpritePiecesTwoCarsTop);
}

void abi_PauseQuitMenuP2(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PauseQuitMenuP2(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:BFC9 PauseQuitMenu
 * WHAT IT DOES: The quit menu for pad 1 ($0D1A): B or START takes the
 * cursor $0D50 (0 closes the menu: stage $E, $19DF = $19D8 = 1; 4 shows
 * the cars, cursor 6, the first time also $10BA = $FF, $0D5D = $FF, $0D23
 * = $E and sprites 58 up parked; 2 and 6 go back to 0; sound $46); else
 * down and up move the cursor (sound $45) and the rows show it (the
 * cursor's row cycling its palette); at 6 the cars (CarSelectTwoPlayers,
 * SpritePiecesTwoCars).
 * HOW: JMP ($C02D,X) on the cursor. The move up from 2 is not reached in
 * the traces so far.
 * Entry: any widths. Exit: as the path leaves it.
 */
void PauseQuitMenu(Carried *cr)
{
    quit_menu_c(cr, 0x0D1A, 0x0D5D, 0x0E, w_OamHighBits58Park112, w_SpritePiecesTwoCars);
}

void abi_PauseQuitMenu(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PauseQuitMenu(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:BC1C PauseReplayStages
 * WHAT IT DOES: Runs the replay pause's stage $0D2F from the table at
 * $07:BC24: 0 PauseStageStart, 2 PauseStageWait, 4 PauseStageChoice, 6
 * PauseStageReplayEnd, 8 PauseStageQuit, $A PauseStageQuitWait, $C
 * PauseStageLeave.
 * HOW: SEP #$30, JMP (table,X).
 * Entry: any widths. Exit: as the stage leaves it.
 */
void abi_PauseReplayStages(void)
{
    op_sep(P_M | P_X);
    cyc(24);
    cpu.x = rd8(((uint32_t)cpu.db << 16) | 0x0D2F);
    cpu.p = nz8(cpu.p, (uint8_t)cpu.x);
    cyc(46);
    jump_routine(0x070000 | rd16(0x07BC24 + cpu.x));
}

/*
 * @routine 07:BD08 PauseStageLeaveRts
 * WHAT IT DOES: The RTS after PauseStageLeave's last JMP, which no code
 * reaches: returns at once.
 * HOW: RTS.
 * Entry: any. Exit: everything unchanged.
 */
void abi_PauseStageLeaveRts(void)
{
    cyc(42);
}

/*
 * @routine 07:C021 PauseQuitMenuCursor0
 * WHAT IT DOES: Bytes in PauseQuitMenu after an RTS that no code reaches:
 * the quit menu's cursor $0D50 = 0.
 * HOW: LDA #0, STA, RTS.
 * Entry: M 8-bit. Exit: A low byte 0; Z set, N clear.
 */
void PauseQuitMenuCursor0(Carried *cr)
{
    lda8_imm(cr, 0x00);
    cyc(16);
    wr8((uint32_t)cr->db << 16 | 0x0D50, 0x00);
    cyc(42);
}

void abi_PauseQuitMenuCursor0(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PauseQuitMenuCursor0(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:C03E PauseQuitMenuBack
 * WHAT IT DOES: Bytes in PauseQuitMenu that no code reaches: sound $46 and
 * the quit menu's cursor $0D50 = 0.
 * HOW: SEP #$20, LDA #$46, JSL SoundPort0QueuePush, STZ, RTS.
 * Entry: any widths. Exit: M 8-bit; as SoundPort0QueuePush leaves the
 * rest.
 */
void PauseQuitMenuBack(Carried *cr)
{
    r_sep(cr, P_M);
    cyc(76);
    lda8_imm(cr, 0x46);
    SoundPort0QueuePush_cr(cr);
    cyc(24);
    wr8((uint32_t)cr->db << 16 | 0x0D50, 0x00);
    cyc(42);
}

void abi_PauseQuitMenuBack(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PauseQuitMenuBack(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* Gauge6BPlace as its adapter leaves the registers. */
static void w_Gauge6BPlace(Carried *cr)
{
    uint8_t v = Gauge6BPlace(cr->db);

    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    cr->x = 0x00;
    cr->y &= 0x00FF;
}

/* Gauge6APlace as its adapter leaves the registers. */
static void w_Gauge6APlace(Carried *cr)
{
    uint8_t v = Gauge6APlace(cr->db);

    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    cr->x = 0x10;
    cr->y &= 0x00FF;
}

/* OamPlaceRows75And112 as its adapter leaves the registers. */
static void w_OamPlaceRows75And112(Carried *cr)
{
    OamPlaceRows75And112(cr->db);
    cr->a &= 0xFF00;
}

/* WindowSlideUpSound as its adapter leaves the registers. */
static void w_WindowSlideUpSound(Carried *cr)
{
    cr->a = (uint16_t)((cr->a & 0xFF00) | WindowSlideUpSound(cr->db, !(cr->p & P_X)));
}

/* ChampionshipScreenLoop, still in registers: its call with the registers loaded and taken
   back. */
static void w_ChampionshipScreenLoop(Carried *cr)
{
    cpu.a = cr->a;
    cpu.x = cr->x;
    cpu.y = cr->y;
    cpu.d = cr->d;
    cpu.p = cr->p;
    cpu.db = cr->db;
    abi_ChampionshipScreenLoop();
    cr->a = cpu.a;
    cr->x = cpu.x;
    cr->y = cpu.y;
    cr->d = cpu.d;
    cr->p = cpu.p;
    cr->db = cpu.db;
}

/* RaceRestart, still in registers: its call with the registers loaded and taken
   back. */
static void w_RaceRestart(Carried *cr)
{
    cpu.a = cr->a;
    cpu.x = cr->x;
    cpu.y = cr->y;
    cpu.d = cr->d;
    cpu.p = cr->p;
    cpu.db = cr->db;
    abi_RaceRestart();
    cr->a = cpu.a;
    cr->x = cpu.x;
    cr->y = cpu.y;
    cr->d = cpu.d;
    cr->p = cpu.p;
    cr->db = cpu.db;
}

/* SoftResetToBootWarm, still in registers: its call with the registers loaded and taken
   back. */
static void w_SoftResetToBootWarm(Carried *cr)
{
    cpu.a = cr->a;
    cpu.x = cr->x;
    cpu.y = cr->y;
    cpu.d = cr->d;
    cpu.p = cr->p;
    cpu.db = cr->db;
    abi_SoftResetToBootWarm();
    cr->a = cpu.a;
    cr->x = cpu.x;
    cr->y = cpu.y;
    cr->d = cpu.d;
    cr->p = cpu.p;
    cr->db = cpu.db;
}

/* Countdown0DF8 as its adapter leaves the registers. */
static void w_Countdown0DF8(Carried *cr)
{
    cr->a = (uint16_t)((cr->a & 0xFF00) | Countdown0DF8(cr->db));
    cyc(16);
}

/* SoundPort1QueuePushX8, still in registers: its call with the registers loaded and taken
   back. */
static void w_SoundPort1QueuePushX8(Carried *cr)
{
    cpu.a = cr->a;
    cpu.x = cr->x;
    cpu.y = cr->y;
    cpu.d = cr->d;
    cpu.p = cr->p;
    cpu.db = cr->db;
    abi_SoundPort1QueuePushX8();
    cr->a = cpu.a;
    cr->x = cpu.x;
    cr->y = cpu.y;
    cr->d = cpu.d;
    cr->p = cpu.p;
    cr->db = cpu.db;
}

/* OamBlock52Left as its adapter leaves the registers. */
static void w_OamBlock52Left(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = (uint16_t)((cr->a & 0xFF00) | OamBlock52Left(cr->db, &p));
    cr->x = 0x0010;
    cr->p = (uint8_t)(p & ~(P_N | P_Z | P_X));
}

/* OamBlock52Right as its adapter leaves the registers. */
static void w_OamBlock52Right(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = (uint16_t)((cr->a & 0xFF00) | OamBlock52Right(cr->db, &p));
    cr->x = 0x0010;
    cr->p = (uint8_t)(p & ~(P_N | P_Z | P_X));
}

/* OamBlock59Left as its adapter leaves the registers. */
static void w_OamBlock59Left(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = (uint16_t)((cr->a & 0xFF00) | OamBlock59Left(cr->db, &p));
    cr->x = 0x002C;
    cr->p = (uint8_t)(p & ~(P_N | P_Z | P_X));
}

/* OamBlock59Right as its adapter leaves the registers. */
static void w_OamBlock59Right(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = (uint16_t)((cr->a & 0xFF00) | OamBlock59Right(cr->db, &p));
    cr->x = 0x002C;
    cr->p = (uint8_t)(p & ~(P_N | P_Z | P_X));
}

/* OamBlock123Left as its adapter leaves the registers. */
static void w_OamBlock123Left(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = (uint16_t)((cr->a & 0xFF00) | OamBlock123Left(cr->db, &p));
    cr->x = 0x012C;
    cr->p = (uint8_t)(p & ~(P_N | P_Z | P_X));
}

/* OamBlock123Right as its adapter leaves the registers. */
static void w_OamBlock123Right(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = (uint16_t)((cr->a & 0xFF00) | OamBlock123Right(cr->db, &p));
    cr->x = 0x012C;
    cr->p = (uint8_t)(p & ~(P_N | P_Z | P_X));
}

/* OamHighBits58Park112 as its adapter leaves the registers. */
static void w_OamHighBits58Park112(Carried *cr)
{
    OamHighBits58Park112(cr->db);
    cr->a = 0x5555;
    cr->x = 0x0010;
    cr->y &= 0x00FF;
}

/* OamHighBits18Park48 as its adapter leaves the registers. */
static void w_OamHighBits18Park48(Carried *cr)
{
    OamHighBits18Park48(cr->db);
    cr->a = 0x5555;
    cr->x = 0x0000;
    cr->y &= 0x00FF;
}

/* RaceEndToAttract, still in registers: its call with the registers loaded and taken
   back. */
static void w_RaceEndToAttract(Carried *cr)
{
    cpu.a = cr->a;
    cpu.x = cr->x;
    cpu.y = cr->y;
    cpu.d = cr->d;
    cpu.p = cr->p;
    cpu.db = cr->db;
    abi_RaceEndToAttract();
    cr->a = cpu.a;
    cr->x = cpu.x;
    cr->y = cpu.y;
    cr->d = cpu.d;
    cr->p = cpu.p;
    cr->db = cpu.db;
}

/* TilemapPaletteCycle87, still in registers: its call with the registers loaded and taken
   back. */
static void w_TilemapPaletteCycle87(Carried *cr)
{
    cpu.a = cr->a;
    cpu.x = cr->x;
    cpu.y = cr->y;
    cpu.d = cr->d;
    cpu.p = cr->p;
    cpu.db = cr->db;
    abi_TilemapPaletteCycle87();
    cr->a = cpu.a;
    cr->x = cpu.x;
    cr->y = cpu.y;
    cr->d = cpu.d;
    cr->p = cpu.p;
    cr->db = cpu.db;
}

/* TilemapRow107Palette5, still in registers: its call with the registers loaded and taken
   back. */
static void w_TilemapRow107Palette5(Carried *cr)
{
    cpu.a = cr->a;
    cpu.x = cr->x;
    cpu.y = cr->y;
    cpu.d = cr->d;
    cpu.p = cr->p;
    cpu.db = cr->db;
    abi_TilemapRow107Palette5();
    cr->a = cpu.a;
    cr->x = cpu.x;
    cr->y = cpu.y;
    cr->d = cpu.d;
    cr->p = cpu.p;
    cr->db = cpu.db;
}

/* TilemapRow187Palette5, still in registers: its call with the registers loaded and taken
   back. */
static void w_TilemapRow187Palette5(Carried *cr)
{
    cpu.a = cr->a;
    cpu.x = cr->x;
    cpu.y = cr->y;
    cpu.d = cr->d;
    cpu.p = cr->p;
    cpu.db = cr->db;
    abi_TilemapRow187Palette5();
    cr->a = cpu.a;
    cr->x = cpu.x;
    cr->y = cpu.y;
    cr->d = cpu.d;
    cr->p = cpu.p;
    cr->db = cpu.db;
}

/* TilemapPaletteCycle107, still in registers: its call with the registers loaded and taken
   back. */
static void w_TilemapPaletteCycle107(Carried *cr)
{
    cpu.a = cr->a;
    cpu.x = cr->x;
    cpu.y = cr->y;
    cpu.d = cr->d;
    cpu.p = cr->p;
    cpu.db = cr->db;
    abi_TilemapPaletteCycle107();
    cr->a = cpu.a;
    cr->x = cpu.x;
    cr->y = cpu.y;
    cr->d = cpu.d;
    cr->p = cpu.p;
    cr->db = cpu.db;
}

/* TilemapRow87Palette5, still in registers: its call with the registers loaded and taken
   back. */
static void w_TilemapRow87Palette5(Carried *cr)
{
    cpu.a = cr->a;
    cpu.x = cr->x;
    cpu.y = cr->y;
    cpu.d = cr->d;
    cpu.p = cr->p;
    cpu.db = cr->db;
    abi_TilemapRow87Palette5();
    cr->a = cpu.a;
    cr->x = cpu.x;
    cr->y = cpu.y;
    cr->d = cpu.d;
    cr->p = cpu.p;
    cr->db = cpu.db;
}

/* TilemapPaletteCycle187, still in registers: its call with the registers loaded and taken
   back. */
static void w_TilemapPaletteCycle187(Carried *cr)
{
    cpu.a = cr->a;
    cpu.x = cr->x;
    cpu.y = cr->y;
    cpu.d = cr->d;
    cpu.p = cr->p;
    cpu.db = cr->db;
    abi_TilemapPaletteCycle187();
    cr->a = cpu.a;
    cr->x = cpu.x;
    cr->y = cpu.y;
    cr->d = cpu.d;
    cr->p = cpu.p;
    cr->db = cpu.db;
}

/* SpritePiecesTwoCarsTop, still in registers: its call with the registers loaded and taken
   back. */
static void w_SpritePiecesTwoCarsTop(Carried *cr)
{
    cpu.a = cr->a;
    cpu.x = cr->x;
    cpu.y = cr->y;
    cpu.d = cr->d;
    cpu.p = cr->p;
    cpu.db = cr->db;
    abi_SpritePiecesTwoCarsTop();
    cr->a = cpu.a;
    cr->x = cpu.x;
    cr->y = cpu.y;
    cr->d = cpu.d;
    cr->p = cpu.p;
    cr->db = cpu.db;
}

/* SpritePiecesTwoCars, still in registers: its call with the registers loaded and taken
   back. */
static void w_SpritePiecesTwoCars(Carried *cr)
{
    cpu.a = cr->a;
    cpu.x = cr->x;
    cpu.y = cr->y;
    cpu.d = cr->d;
    cpu.p = cr->p;
    cpu.db = cr->db;
    abi_SpritePiecesTwoCars();
    cr->a = cpu.a;
    cr->x = cpu.x;
    cr->y = cpu.y;
    cr->d = cpu.d;
    cr->p = cpu.p;
    cr->db = cpu.db;
}
