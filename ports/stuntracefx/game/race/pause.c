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
#include "wram_names.h"


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
void PauseReplayStages(Carried *cr);
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
static void quit_menu(Carried *cr, uint16_t pad, uint16_t which, uint8_t d, void (*park)(Carried *), void (*cars)(Carried *));
static void quit_menu_open(Carried *cr, uint32_t b);
static void quit_menu_take(Carried *cr, uint32_t b, uint16_t which, uint8_t d, void (*park)(Carried *));
static void rows(Carried *cr, void (*r87)(Carried *), void (*r107)(Carried *), void (*r187)(Carried *));
static void slide_up(Carried *cr, uint32_t b, uint8_t v);
static void stage(Carried *cr, uint32_t b, uint8_t s);
static int timer_out(Carried *cr, uint32_t b);
static int window_closed(Carried *cr, uint32_t b);
void CarSelectTwoPlayers_cr(Carried *cr);
void ChampionshipScreenLoop(Carried *cr);
void Countdown0DF8_cr(Carried *cr);
void Gauge6APlace_cr(Carried *cr);
void Gauge6BPlace_cr(Carried *cr);
void OamBlock123Left_cr(Carried *cr);
void OamBlock123Right_cr(Carried *cr);
void OamBlock52Left_cr(Carried *cr);
void OamBlock52Right_cr(Carried *cr);
void OamBlock59Left_cr(Carried *cr);
void OamBlock59Right_cr(Carried *cr);
void OamHighBits18Park48_cr(Carried *cr);
void OamHighBits58Park112_cr(Carried *cr);
void OamPlaceRows75And112_cr(Carried *cr);
void RaceEndToAttract(Carried *cr);
void RaceEndVideoReset(Carried *cr);
void RaceRestart(Carried *cr);
void SoftResetToBootWarm(Carried *cr);
void SoundPort0QueuePush_cr(Carried *cr);
void SoundPort1QueuePushX8(Carried *cr);
void SoundPort1QueuePush_cr(Carried *cr);
void SpritePiecesTwoCarsTop_cr(Carried *cr);
void SpritePiecesTwoCars_cr(Carried *cr);
void TilemapPaletteCycle107_cr(Carried *cr);
void TilemapPaletteCycle187_cr(Carried *cr);
void TilemapPaletteCycle87_cr(Carried *cr);
void TilemapRow107Palette5_cr(Carried *cr);
void TilemapRow187Palette5_cr(Carried *cr);
void TilemapRow87Palette5_cr(Carried *cr);
void WindowSlideUpSound_cr(Carried *cr);

/* LDA #v (8-bit). */
static void lda8_imm(Carried *cr, uint8_t v)
{
    cyc(24);
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    set_p(cr, nz8(cr->p, v));
}

/* DEC of a work-RAM byte; N, Z from it. */
static uint8_t dec8_mem(Carried *cr, uint32_t addr)
{
    cyc(24);
    uint8_t v = (uint8_t)(rd8(addr) - 1);

    cyc(6);
    wr8(addr, v);
    set_p(cr, nz8(cr->p, v));
    return v;
}

/* CMP # (8-bit). */
static void cmp8_imm(Carried *cr, uint8_t m)
{
    uint8_t a = (uint8_t)cr->a;

    set_p(cr, (uint8_t)((nz8(cr->p, (uint8_t)(a - m)) & ~P_C) | (a >= m ? (cyc(38), P_C) : (cyc(32), 0))));
}

/* REP #$20, LDA presses, BIT #mask: Z from the test. */
static int presses(Carried *cr, uint32_t b, uint16_t pad, uint16_t mask)
{
    r_rep(cr, P_M);
    cyc(2);
    r_lda16(cr, b | pad);
    set_p(cr, (uint8_t)((cr->p & ~P_Z) | ((cr->a & mask) ? (cyc(80), 0) : (cyc(46), P_Z))));
    return (cr->a & mask) != 0;
}

/* SEP #$20, the timer $0D5F counted down; 1 at its end. */
static int timer_out(Carried *cr, uint32_t b)
{
    r_sep(cr, P_M);
    cyc(22);
    return dec8_mem(cr, 0x7E0000 | W(wram_0D5F)) == 0;
}

/* The stage $0D2F = s. */
static void stage(Carried *cr, uint32_t b, uint8_t s)
{
    lda8_imm(cr, s);
    wr8(0x7E0000 | W(wram_0D2F), s);
}

/* 07:BC88: the race resumes: sound $16 on port 1, the presses' low bytes
   ($0D1A, $0D1C) and $0D2D cleared, $0E5B and $0E5F one less. */
static void pause_resume(Carried *cr, uint32_t b)
{
    cyc(54);
    lda8_imm(cr, 0x16);
    SoundPort1QueuePush_cr(cr);
    cyc(24);
    wr8(0x7E0000 | W(joy1_pressed_copy), 0x00);
    cyc(24);
    wr8(0x7E0000 | W(joy2_pressed_copy), 0x00);
    r_sep(cr, P_M);
    cyc(46);
    wr8(0x7E0000 | W(wram_0D2D), 0x00);
    dec8_mem(cr, 0x7E0000 | W(wram_0E5B));
    dec8_mem(cr, 0x7E0000 | W(palette_animate));
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
    wr8(0x7E0000 | W(wram_0D5F), 0x0A);
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
    Gauge6BPlace_cr(cr);
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
    Gauge6APlace_cr(cr);
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
        r_lda8(cr, 0x7E0000 | W(wram_0D5F));
        cmp8_imm(cr, 0x01);
        if ((uint8_t)cr->a == 0x01)
            OamPlaceRows75And112_cr(cr);
        cyc(42);
        return;
    }
    lda8_imm(cr, 0x93);
    cyc(38);
    wr8(0x7E0000 | W(wram_0E14), 0x93);
    lda8_imm(cr, 0x7C);
    cyc(16);
    wr8(0x7E0000 | W(wram_0E0C), 0x7C);
    lda8_imm(cr, 0x8C);
    cyc(16);
    wr8(0x7E0000 | W(wram_0E18), 0x8C);
    cyc(24);
    wr8(0x7E0000 | W(wram_0D22), 0x00);
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
    (cyc(62), WindowSlideUpSound_cr(cr));
    lda8_imm(cr, v);
    cyc(16);
    wr8(0x7E0000 | W(wram_19DF), v);
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
    r_lda8(cr, 0x7E0000 | W(hud_mode));
    if (!(uint8_t)cr->a) {
        cyc(32);
        stage(cr, b, 0x0A);
        slide_up(cr, b, 0x01);
        lda8_imm(cr, 0x70);
        cyc(16);
        wr8(0x7E0000 | W(wram_0D5F), 0x70);
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

    r_sep(cr, P_M);
    cyc(46);
    r_lda8(cr, 0x7E0000 | W(wram_10CD));
    if ((uint8_t)cr->a) {
        lda8_imm(cr, 0x01);
        cyc(32);
        wr8(0x7E0000 | W(fade_out), 0x01);
        cyc(24);
        r_lda8(cr, 0x7E0000 | W(fade_level_hi));
        if ((uint8_t)cr->a & 0x80)
            { cyc(40); SoftResetToBootWarm(cr); } else { cyc(64); }
        return;
    }
    cyc(46);
    wr8(0x7E0000 | W(wram_0D59), 0x00);
    (cyc(46), ChampionshipScreenLoop(cr));
    r_sep(cr, P_M);
    cyc(46);
    r_lda8(cr, 0x7E0000 | W(wram_0DD4));
    if ((uint8_t)cr->a)
        { cyc(40); RaceRestart(cr); }
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
    r_lda8(cr, 0x7E0000 | W(wram_0D24));
    if ((uint8_t)cr->a)
        { cyc(40); RaceRestart(cr); }
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
    r_lda8(cr, 0x7E0000 | W(wram_0D3D));
    if ((uint8_t)cr->a) {
        lda8_imm(cr, 0xFF);
        cyc(32);
        wr8(0x7E0000 | W(layout_request), 0xFF);
        cyc(16);
        stage(cr, b, 0x08);
        lda8_imm(cr, 0x08);
        cyc(16);
        wr8(0x7E0000 | W(wram_0D5F), 0x08);
        if (!quit_extra)
            return;
        cyc(24);
        r_lda8(cr, 0x7E0000 | W(hud_mode));
        if ((uint8_t)cr->a)
            { cyc(22); return; }
        cyc(40);
        r_lda8(cr, 0x7E0000 | W(digit62_override));
        if ((uint8_t)cr->a)
            { cyc(22); return; } else { cyc(78); }
        Countdown0DF8_cr(cr);
        cyc(54);
        lda8_imm(cr, 0x22);
        SoundPort1QueuePushX8(cr);
        return;
    }
    r_sep(cr, P_M);
    lda8_imm(cr, 0xFF);
    cyc(60);
    wr8(0x7E0000 | W(layout_request), 0xFF);
    cyc(16);
    stage(cr, b, 0x06);
    lda8_imm(cr, 0x08);
    cyc(16);
    wr8(0x7E0000 | W(wram_0D5F), 0x08);
}

/* Left (bit 9) on continue: sound $45, $0D3D one less, left(); right (bit
   8) on quit: sound $45, $0D3D cleared, right(). `wide` reads $0D3D as a
   word (masked) and tests right with M 8-bit set first. */
static void choice_move(Carried *cr, uint32_t b, uint16_t pad, int wide, void (*left)(Carried *), void (*right)(Carried *))
{
    uint8_t c;

    if (wide) {
        cyc(18);
        r_lda16(cr, 0x7E0000 | W(wram_0D3D));
        r_and16(cr, 0x00FF);
        c = (uint8_t)cr->a;
    } else {
        r_sep(cr, P_M);
        cyc(46);
        r_lda8(cr, 0x7E0000 | W(wram_0D3D));
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
        dec8_mem(cr, 0x7E0000 | W(wram_0D3D));
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
    wr8(0x7E0000 | W(wram_0D3D), 0x00);
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
    if (presses(cr, b, W(joy1_pressed_copy), 0x9000)) {
        cyc(18);
        choice_made(cr, b, 1);
        cyc(42);
        return;
    }
    choice_move(cr, b, 0x0D1A, 0, OamBlock52Left_cr, OamBlock52Right_cr);
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
    if (presses(cr, b, W(joy1_pressed_copy), 0x9000)) {
        choice_made(cr, b, 0);
        cyc(42);
        return;
    }
    choice_move(cr, b, 0x0D1A, 1, OamBlock59Left_cr, OamBlock59Right_cr);
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
    if (presses(cr, b, W(joy2_pressed_copy), 0x9000)) {
        choice_made(cr, b, 0);
        cyc(42);
        return;
    }
    choice_move(cr, b, 0x0D1C, 1, OamBlock123Left_cr, OamBlock123Right_cr);
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
    r_lda8(cr, 0x7E0000 | W(wram_0D5F));
    if ((uint8_t)cr->a) {
        cyc(16);
        dec8_mem(cr, 0x7E0000 | W(wram_0D5F));
        return;
    }
    lda8_imm(cr, owner);
    cyc(38);
    wr8(0x7E0000 | W(wram_0D2E), owner);
    if (owner == 0xFF)
        cyc(22);   /* BRA */
    lda8_imm(cr, 0x00);
    cyc(16);
    wr8(0x7E0000 | W(wram_19DF), 0x00);
    (cyc(62), WindowSlideUpSound_cr(cr));
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
    wr8(0x7E0000 | W(wram_19DF), 0x00);
    lda8_imm(cr, 0x02);
    cyc(16);
    wr8(0x7E0000 | W(window_hdma_off), 0x02);
    cyc(16);
    stage(cr, b, 0x0A);
    cyc(24);
    wr8(0x7E0000 | W(wram_0D50), 0x00);
    cyc(16);
    stage(cr, b, 0x0C);
    cyc(24);
    wr8(0x7E0000 | W(wram_0D22), 0x0C);
    cyc(42);
}

/* LDA $19D8, CMP #5: the split window closed. */
static int window_closed(Carried *cr, uint32_t b)
{
    cyc(24);
    r_lda8(cr, 0x7E0000 | W(window_hdma_off));
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
    wr8(0x7E0000 | W(wram_0D2E), 0xFF);
    lda8_imm(cr, 0x01);
    cyc(16);
    wr8(0x7E0000 | W(wram_0D5D), 0x01);
    lda8_imm(cr, 0x0E);
    cyc(16);
    wr8(0x7E0000 | W(wram_0D23), 0x0E);
    OamHighBits58Park112_cr(cr);
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
    wr8(0x7E0000 | W(wram_0D2E), 0x00);
    lda8_imm(cr, 0x01);
    cyc(16);
    wr8(0x7E0000 | W(wram_0D5C), 0x01);
    lda8_imm(cr, 0x0C);
    cyc(16);
    wr8(0x7E0000 | W(wram_0D23), 0x0C);
    OamHighBits18Park48_cr(cr);
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
    r_lda8(cr, 0x7E0000 | W(wram_0D50));
    if ((uint8_t)cr->a)
        { cyc(40); RaceRestart(cr); }
    else
        { cyc(46); RaceEndToAttract(cr); }
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
    wr8(0x7E0000 | W(wram_0D50), to);
    return 1;
}

/* B or START in the quit menu: cursor 0 closes it (sound $46, stage $E,
   $19DF = $19D8 = 1); 2 or 6 back to 0 (sound $46); 4 the cars (sound $46,
   cursor 6), the first time also $10BA = $FF, `which` = $FF, $0D23 = d and
   park(). */
static void quit_menu_take(Carried *cr, uint32_t b, uint16_t which, uint8_t d, void (*park)(Carried *))
{
    cyc(6);
    r_lda8(cr, 0x7E0000 | W(wram_0D50));
    cmp8_imm(cr, 0x04);
    if ((uint8_t)cr->a == 0x04) {
        cyc(24);
        r_lda8(cr, 0x7E0000 | W(wram_10BA));
        if (!(uint8_t)cr->a) {
            lda8_imm(cr, 0xFF);
            cyc(32);
            wr8(0x7E0000 | W(wram_10BA), 0xFF);
            lda8_imm(cr, 0xFF);
            cyc(16);
            wr8(b | which, 0xFF);
            lda8_imm(cr, d);
            cyc(16);
            wr8(0x7E0000 | W(wram_0D23), d);
            park(cr);
        } else { cyc(22); }                /* BNE taken */
        cyc(38);
        lda8_imm(cr, 0x46);
        cyc(16);
        SoundPort0QueuePush_cr(cr);
        cyc(16);
        lda8_imm(cr, 0x06);
        wr8(0x7E0000 | W(wram_0D50), 0x06);
        return;
    } else { cyc(54); }
    if ((uint8_t)cr->a > 0x04) {
        lda8_imm(cr, 0x46);
        cyc(16);
        SoundPort0QueuePush_cr(cr);
        cyc(24);
        wr8(0x7E0000 | W(wram_0D50), 0x00);
        return;
    }
    lda8_imm(cr, 0x46);
    cyc(16);
    SoundPort0QueuePush_cr(cr);
    cyc(16);
    stage(cr, b, 0x0E);
    lda8_imm(cr, 0x01);
    cyc(16);
    wr8(0x7E0000 | W(wram_19DF), 0x01);
    cyc(24);
    wr8(0x7E0000 | W(window_hdma_off), 0x01);
}

/* The quit menu for pad: B or START takes the cursor; else by the cursor
   $0D50: 0 down to 2; 2 down to 4 or up to 0; 4 up to 2; 6 the cars
   (CarSelectTwoPlayers, then cars()); each with its rows. */
static void quit_menu(Carried *cr, uint16_t pad, uint16_t which, uint8_t d, void (*park)(Carried *), void (*cars)(Carried *))
{
    uint32_t b = (uint32_t)cr->db << 16;

    r_sep(cr, P_M | P_X);
    cyc(22);
    presses(cr, b, pad, 0x9000);
    r_sep(cr, P_M);
    if (cr->a & 0x9000) {
        quit_menu_take(cr, b, which, d, park);
        cyc(42);
        return;
    }
    cyc(46);
    cr->x = rd8(0x7E0000 | W(wram_0D50));
    set_p(cr, nz8(cr->p, (uint8_t)cr->x));
    switch (cr->x) {
    case 0x00:
        cursor_move(cr, b, pad, 0x0400, 0x02);
        cyc(46);
        r_sep(cr, P_M);
        rows(cr, TilemapPaletteCycle87_cr, TilemapRow107Palette5_cr, TilemapRow187Palette5_cr);
        return;
    case 0x02:
        if (!cursor_move(cr, b, pad, 0x0400, 0x04)) {
            presses(cr, b, pad, 0x0800);
            if (cr->a & 0x0800) {
                cyc(20);
                r_sep(cr, P_M);
                lda8_imm(cr, 0x45);
                cyc(16);
                SoundPort0QueuePush_cr(cr);
                cyc(16);
                lda8_imm(cr, 0x00);
                wr8(0x7E0000 | W(wram_0D50), 0x00);
            }
        } else { cyc(22); }
        cyc(46);
        r_sep(cr, P_M);
        rows(cr, TilemapRow87Palette5_cr, TilemapPaletteCycle107_cr, TilemapRow187Palette5_cr);
        return;
    case 0x04:
        cursor_move(cr, b, pad, 0x0800, 0x02);
        cyc(46);
        break;
    case 0x06:
        cyc(92);
        CarSelectTwoPlayers_cr(cr);
        (cyc(46), cars(cr));
        break;
    default:
        carried_jump(cr, 0x070000 | rd16(0x070000 | (uint16_t)((pad == 0x0D1A ? 0xC02D : 0xBF39) + cr->x)));
        return;
    }
    r_sep(cr, P_M);
    rows(cr, TilemapPaletteCycle187_cr, TilemapRow107Palette5_cr, TilemapRow87Palette5_cr);
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
    quit_menu(cr, 0x0D1C, 0x0D5C, 0x0C, OamHighBits18Park48_cr, SpritePiecesTwoCarsTop_cr);
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
    quit_menu(cr, 0x0D1A, 0x0D5D, 0x0E, OamHighBits58Park112_cr, SpritePiecesTwoCars_cr);
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
void PauseReplayStages(Carried *cr)
{
    r_sep(cr, P_M | P_X);
    cyc(24);
    cr->x = rd8(0x7E0000 | W(wram_0D2F));
    set_p(cr, nz8(cr->p, (uint8_t)cr->x));
    cyc(46);
    carried_jump(cr, 0x070000 | rd16(0x07BC24 + cr->x));
}

void abi_PauseReplayStages(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PauseReplayStages(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:BD08 PauseStageLeaveRts
 * WHAT IT DOES: The RTS after PauseStageLeave's last JMP, which no code
 * reaches: returns at once.
 * HOW: RTS.
 * Entry: any. Exit: everything unchanged.
 */
/* PauseStageLeaveRts on carried registers: no register work. */
void PauseStageLeaveRts_cr(Carried *cr)
{
    (void)cr;
    cyc(42);
}

void abi_PauseStageLeaveRts(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PauseStageLeaveRts_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
    wr8(0x7E0000 | W(wram_0D50), 0x00);
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
    wr8(0x7E0000 | W(wram_0D50), 0x00);
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

