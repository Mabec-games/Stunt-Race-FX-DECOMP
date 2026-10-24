/*
 * race_phases.c: the race phase handlers, run each frame for each player
 * from the phase tables at $07:E232, $07:E24F and $07:E26E (phase $0DD6
 * for player 1, $0DD7 for player 2) and from the tables of the phase steps
 * in player_stats.c. They count the start, the race, the finish and the
 * results panels down, through the timer $0E55, the sub-state $0D5A and the
 * panel state $0D56. All run with 8-bit registers and data bank $07.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "carried.h"

#include "audio_calls.h"
#include "race_calls.h"
#include "wram_names.h"
#include "wram_struct.h"
void abi_HudBestTimeDigits(void);
void abi_RecordOffsetAllCourses(void);

void abi_SoundPort0QueuePush(void);
void abi_SoundPort0QueuePushX8(void);
void abi_SoundPort1QueuePushX8(void);
void abi_SoundPort0QueuePushRaw(void);
void abi_GlyphWordAt1C28(void);
void abi_GsuBytes200_BFB5BB(void);
void abi_GsuTimeDigits(void);
void abi_GsuTimeDigitsFull(void);

void CarFinishSound(Carried *cr);
void CarFinishSoundP2(Carried *cr);
void LapBannerGrow(Carried *cr);
void LapBannerGrowLast(Carried *cr);
void LapBannerWait(Carried *cr);
void PanelDelayCount(Carried *cr);
void PauseResults(Carried *cr);
void PauseResultsFourB(Carried *cr);
void PauseResultsP2(Carried *cr);
void PhaseGlyphsFive(Carried *cr);
void PhaseGlyphsFourA(Carried *cr);
void PhaseGlyphsFourB(Carried *cr);
void PhaseGlyphsSix(Carried *cr);
void PlayerTwoFrameWork(Carried *cr);
void RaceEndLeave(Carried *cr);
void RaceEndToAttract(Carried *cr);
void RaceEndVideoReset(Carried *cr);
void RaceEndWaitBigSprites(Carried *cr);
void RaceEndWaitRow6B(Carried *cr);
void RaceEndWindowNine(Carried *cr);
void RaceEndWindowSteps(Carried *cr);
void RaceFinishSounds(Carried *cr);
void RaceLapCrossed(Carried *cr);
void RacePhaseLap(Carried *cr);
void RacePhaseLapP1(Carried *cr);
void RacePhaseLapP2(Carried *cr);
void RaceReadyLong(Carried *cr);
void RaceReadyShort(Carried *cr);
void RaceRestart(Carried *cr);
void RaceStartCountdown(Carried *cr);
void RaceStartCountdownStunt(Carried *cr);
void RaceStartGlyphsSteps(Carried *cr);
void RaceStartSteps(Carried *cr);
void RaceStartStepsDraw(Carried *cr);
void RaceStartStepsHold(Carried *cr);
void RaceStartTimer(Carried *cr);
void RaceStartTimerSteps(Carried *cr);
void ResultsPanelSteps(Carried *cr);
void ResultsWindowWait(Carried *cr);
void SoftResetToBootWarm(Carried *cr);
void TimeUpBattle(Carried *cr);
void TimeUpRace(Carried *cr);
void TimeUpStunt(Carried *cr);
static void apu_command5(Carried *cr);
static void apu_upload(Carried *cr, uint16_t a, uint16_t y);
static void cmp8_imm(Carried *cr, uint8_t m);
static uint8_t dec8_mem(Carried *cr, uint32_t addr);
static void finish_alone(Carried *cr, uint32_t b, uint16_t colour, uint8_t phase, uint8_t t);
static void finish_both(Carried *cr);
static void finish_sound(Carried *cr, uint8_t car);
static uint8_t inc8_mem(Carried *cr, uint32_t addr);
static void lap_banner(Carried *cr, void (*draw)(Carried *));
static void lap_crossed(Carried *cr, uint32_t b);
static void lap_finish(Carried *cr, uint32_t b);
static void lap_override(Carried *cr, uint32_t b);
static void lda8_imm(Carried *cr, uint8_t v);
static int panel_delay_out(Carried *cr);
static void panel_line(Carried *cr, uint16_t at);
static void panel_next(Carried *cr);
static void panel_next_delay(Carried *cr, uint8_t d);
static void pause_results(Carried *cr);
static void pause_stage_ten(Carried *cr);
static void phases_both(Carried *cr);
static int pressed_b_start(Carried *cr);
static void race_end_wait(Carried *cr, void (*draw)(Carried *));
static int race_phase(Carried *cr);
static void race_ready(Carried *cr, uint8_t t, int bra);
static void race_start(Carried *cr);
static int sbc8_borrows(Carried *cr, uint8_t m);
static void screen_off(Carried *cr);
static void start_draw(Carried *cr, uint32_t b);
static void start_hold(Carried *cr);
static void substate_next(Carried *cr);
static void time_digits_full(Carried *cr);
static void time_up_end(Carried *cr, uint8_t t);
static void time_up_tick(Carried *cr, uint8_t t, void (*after)(Carried *), uint8_t push_x);
static int time_up_wait(Carried *cr);
static int window_closed_overlay(Carried *cr, int beq);
static void window_up(Carried *cr);
static void w_ApuUpload(Carried *cr);
static void w_LapBannerNone(Carried *cr);
static void w_LapSplit(Carried *cr);
static void w_RaceTimeKeep(Carried *cr);
void BigSpritesNine_cr(Carried *cr);
void BigSpritesRow5_cr(Carried *cr);
void BigSpritesRow6B_cr(Carried *cr);
void BigSpritesRow6_cr(Carried *cr);
void ChampionScreenRun(Carried *cr);
void Countdown0DF8_cr(Carried *cr);
void GlyphSlideThree_cr(Carried *cr);
void GlyphSpritesGrowAndTimes_cr(Carried *cr);
void GlyphSpritesInit_cr(Carried *cr);
void GlyphWordAt1C28_cr(Carried *cr);
void GsuBytes200_BFB5BB_cr(Carried *cr);
void GsuEEF5OrEFCD_cr(Carried *cr);
void GsuF0A5OrF0B4_cr(Carried *cr);
void GsuF15BFive_cr(Carried *cr);
void GsuF15BFourA_cr(Carried *cr);
void GsuF15BFourB_cr(Carried *cr);
void GsuF15BThree_cr(Carried *cr);
void GsuF441Player1_cr(Carried *cr);
void GsuF441Player2_cr(Carried *cr);
void GsuTimeDigitsFull_cr(Carried *cr);
void GsuTimeDigits_cr(Carried *cr);
void HudBestTimeDigits(Carried *cr);
void HudSixGlyphs_cr(Carried *cr);
void OamHide75Up_cr(Carried *cr);
void OamHideForHudMode_cr(Carried *cr);
void OamStagePause(Carried *cr);
void OamStagePause2(Carried *cr);
void OamStagePause2Loop(Carried *cr);
void OamStagePauseLoop(Carried *cr);
void OverlayFramesUntilFaded(Carried *cr);
void PaletteDarkenFour_cr(Carried *cr);
void PauseOnStart(Carried *cr);
void PlayerSound35_cr(Carried *cr);
void RaceEndCountdown(Carried *cr);
void RaceEndCountdownFade(Carried *cr);
void RaceFinishPhases0E(Carried *cr);
void RaceFinishPhases10(Carried *cr);
void RaceFinishSplit(Carried *cr);
void RaceTimeAddD814_cr(Carried *cr);
void RaceTimeAddD815_cr(Carried *cr);
void RaceVideoInit_cr(Carried *cr);
void RecordCompareTime0_cr(Carried *cr);
void RecordCompareTime1_cr(Carried *cr);
void RecordOffsetAllCourses(Carried *cr);
void RecordTimeCompare_cr(Carried *cr);
void RecordsLoadCourseLong_cr(Carried *cr);
void SaveSumsRecordsB_cr(Carried *cr);
void SoundDataLoad_cr(Carried *cr);
void SoundPort0QueuePushRaw_cr(Carried *cr);
void SoundPort0QueuePushX8(Carried *cr);
void SoundPort0QueuePush_cr(Carried *cr);
void SoundPort1QueuePushX8(Carried *cr);
void SoundReset_cr(Carried *cr);
void SpecialRecordInsert_cr(Carried *cr);
void TimePackBcd_cr(Carried *cr);
void WindowSlideUpSound_cr(Carried *cr);

/* LDA #v (8-bit). */
static void lda8_imm(Carried *cr, uint8_t v)
{
    cyc(24);
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    set_p(cr, nz8(cr->p, v));
}

/* INC / DEC of a work-RAM byte: one read, one write; N, Z from it. */
static uint8_t inc8_mem(Carried *cr, uint32_t addr)
{
    cyc(24);
    uint8_t v = (uint8_t)(rd8(addr) + 1);

    cyc(6);
    wr8(addr, v);
    set_p(cr, nz8(cr->p, v));
    return v;
}

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

    set_p(cr, (uint8_t)((nz8(cr->p, (uint8_t)(a - m)) & ~P_C) | (a >= m ? (cyc(32), P_C) : (cyc(32), 0))));
}

/* Both players' phases ($0DD6, $0DD7) = the low byte of A. */
static void phases_both(Carried *cr)
{
    cyc(16);
    WR8(wram_0DD6, (uint8_t)cr->a);
    cyc(24);
    WR8(engine_phase_p2, (uint8_t)cr->a);
}

/* Two INCs of the sub-state $0D5A: the next one. */
static void substate_next(Carried *cr)
{
    inc8_mem(cr, WA(wram_0D5A));
    inc8_mem(cr, WA(wram_0D5A));
}

/* Two INCs of the panel state $0D56. */
static void panel_next(Carried *cr)
{
    inc8_mem(cr, WA(wram_0D56));
    inc8_mem(cr, WA(wram_0D56));
}

/*
 * @routine 07:E265 PlayerTwoFrameWork
 * WHAT IT DOES: Runs the handler for player 2's race phase $0DD7 from the
 * table at $07:E26E.
 * HOW: SEP #$30, then JMP (table,X); the handler returns to the caller.
 * Entry: any widths. Exit: as the handler leaves it.
 */
void PlayerTwoFrameWork(Carried *cr)
{
    r_sep(cr, P_M | P_X);
    cyc(24);
    r_lda8(cr, WA(engine_phase_p2));
    cr->x = (uint16_t)(cr->a & 0x00FF);
    set_p(cr, nz8(cr->p, (uint8_t)cr->x));
    cyc(60);
    carried_jump(cr, 0x070000 | rd16(0x070000 | (uint16_t)(0xE26E + cr->x)));
}

void abi_PlayerTwoFrameWork(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PlayerTwoFrameWork(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:E284 PhaseNothing
 * WHAT IT DOES: Nothing: the handler of the phases with no work.
 * HOW: RTS.
 * Entry: any widths. Exit: every register unchanged.
 */
/* PhaseNothing on carried registers: no register work. */
void PhaseNothing_cr(Carried *cr)
{
    (void)cr;
    cyc(42);
}

void abi_PhaseNothing(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PhaseNothing_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* 07:E299: both phases 2, $0D5A cleared; then with M 16-bit $0D67, $0D69,
   $19EE, $19F0 and $0E74 cleared and sound 5 on port 0. */
static void race_start(Carried *cr)
{
    lda8_imm(cr, 0x02);
    phases_both(cr);
    cyc(24);
    WR8(wram_0D5A, 0x00);
    r_rep(cr, P_M);
    cyc(24);
    WR16(bonus_digit_state, 0x0000);
    cyc(24);
    WR16(bonus_digit_size, 0x0000);
    cyc(24);
    WR16(wram_19EE, 0x0000);
    cyc(24);
    WR16(wram_19F0, 0x0000);
    cyc(24);
    WR16(wram_0E74, 0x0000);
    r_lda16_imm(cr, 0x0005);
    (cyc(2), SoundPort0QueuePushRaw_cr(cr));
    cyc(42);
}

/*
 * @routine 07:E285 RaceStartCountdown
 * WHAT IT DOES: Starts the race's countdown: clears $0FAE and $0FAF, the
 * timer $0E55 = $20, both players' phases 2, the sub-state $0D5A, the
 * time bonus ($0D67, $0D69), $19EE, $19F0 and the bonus $0E74; plays
 * sound 5 on port 0.
 * HOW: 8-bit stores, the clears of words 16-bit.
 * Entry: M 8-bit, X 8-bit. Exit: M 16-bit; A = 5, or as
 * SoundPort0QueuePushRaw leaves it.
 */
void RaceStartCountdown(Carried *cr)
{

    cyc(24);
    WR8(wram_0FAE, 0x00);
    cyc(24);
    WR8(wram_0FAF, 0x00);
    lda8_imm(cr, 0x20);
    cyc(16);
    WR8(wram_0E55, 0x20);
    cyc(22);
    race_start(cr);
}

void abi_RaceStartCountdown(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RaceStartCountdown(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:E292 RaceStartCountdownStunt
 * WHAT IT DOES: As RaceStartCountdown with the timer $0E55 = $12 and
 * $0FAE, $0FAF kept: Stunt Trax's start.
 * HOW: LDA #$07 then LDA #$12 (the first load has no use).
 * Entry: M 8-bit, X 8-bit. Exit: as RaceStartCountdown.
 */
void RaceStartCountdownStunt(Carried *cr)
{

    lda8_imm(cr, 0x07);
    lda8_imm(cr, 0x12);
    cyc(8);
    WR8(wram_0E55, 0x12);
    race_start(cr);
}

void abi_RaceStartCountdownStunt(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RaceStartCountdownStunt(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* Sound 5 on port 0, the timer $0E55 = t, both phases 2, $10CB and $10CC
   cleared. */
static void race_ready(Carried *cr, uint8_t t, int bra)
{

    lda8_imm(cr, 0x05);
    SoundPort0QueuePushRaw_cr(cr);
    cyc(8);
    lda8_imm(cr, t);
    WR8(wram_0E55, t);
    if (bra)
        cyc(22);                                       /* BRA */
    lda8_imm(cr, 0x02);
    phases_both(cr);
    cyc(24);
    WR8(piece_counter, 0x00);
    cyc(24);
    WR8(wram_10CC, 0x00);
    cyc(42);
}

/*
 * @routine 07:E2BD RaceReadyShort
 * WHAT IT DOES: Plays sound 5 on port 0; the timer $0E55 = $1E; both
 * players' phases 2; clears $10CB and $10CC.
 * HOW: 8-bit stores.
 * Entry: M 8-bit, X 8-bit. Exit: A low byte = 2; N, Z clear.
 */
void RaceReadyShort(Carried *cr)
{
    race_ready(cr, 0x1E, 1);
}

void abi_RaceReadyShort(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RaceReadyShort(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:E2CA RaceReadyLong
 * WHAT IT DOES: As RaceReadyShort with the timer $0E55 = $36.
 * HOW: As RaceReadyShort.
 * Entry: M 8-bit, X 8-bit. Exit: as RaceReadyShort.
 */
void RaceReadyLong(Carried *cr)
{
    race_ready(cr, 0x36, 0);
}

void abi_RaceReadyLong(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RaceReadyLong(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* 07:E370: REP #$20, GsuTimeDigitsFull. */
static void time_digits_full(Carried *cr)
{
    r_rep(cr, P_M);
    GsuTimeDigitsFull_cr(cr);
    cyc(42);
}

/* RaceStartStepsDraw's body: LDX the timer, the size byte from the table at
   $07:E33A read as a word and masked, into $70:0210; the time drawn. */
static void start_draw(Carried *cr, uint32_t b)
{
    cyc(24);
    cr->x = RD8(wram_0E55);
    set_p(cr, nz8(cr->p, (uint8_t)cr->x));
    r_rep(cr, P_M);
    cyc(94);
    cr->a = rd16(b | (uint16_t)(0xE33A + cr->x));
    r_and16(cr, 0x00FF);
    wr16(0x700210, cr->a);
    (cyc(62), GsuBytes200_BFB5BB_cr(cr));
    (cyc(46), GsuTimeDigits_cr(cr));
    cyc(42);
}

/* RaceStartSteps' sub-state 4: waits on the timer (at once while $0D54 is
   0), then the next sub-state with $0D54 = 8 and the timer 3. */
static void start_hold(Carried *cr)
{
    cyc(24);
    r_lda8(cr, WA(wram_0D54));
    if ((uint8_t)cr->a) {
        cyc(16);
        if (dec8_mem(cr, WA(wram_0E55)))
            { cyc(22); return; }
        cyc(16);
    } else { cyc(22); }
    substate_next(cr);
    lda8_imm(cr, 0x08);
    cyc(16);
    WR8(wram_0D54, 0x08);
    lda8_imm(cr, 0x03);
    cyc(16);
    WR8(wram_0E55, 0x03);
}

/*
 * @routine 07:E2E9 RaceStartSteps
 * WHAT IT DOES: Steps the start's sub-state $0D5A, the timer $0E55
 * counting each: 0 waits, then the timer 7; 2 draws the race time at the
 * digit size from the table at $07:E33A (by the timer), and at the end
 * the timer $14, $0D54 = 4, $0D5B one less and the time drawn full size
 * (GsuTimeDigitsFull); 4 waits (at once while $0D54 is 0), then $0D54 =
 * 8 and the timer 3; 6 waits, then clears $0D56 and sets both players'
 * phases 4.
 * HOW: JMP ($E2EF,X) on $0D5A; the drawing at the timer is
 * RaceStartStepsDraw (07:E323).
 * Entry: M 8-bit, X 8-bit; D = $0000. Exit: as the step leaves it: M
 * 16-bit, X 8-bit after a drawing.
 */
void RaceStartSteps(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;

    cyc(24);
    cr->x = RD8(wram_0D5A);
    set_p(cr, nz8(cr->p, (uint8_t)cr->x));
    switch (cr->x) {
    case 0:
        cyc(46);
        if (dec8_mem(cr, WA(wram_0E55)))
            { cyc(64); return; } else { cyc(16); }
        substate_next(cr);
        lda8_imm(cr, 0x07);
        cyc(16);
        WR8(wram_0E55, 0x07);
        cyc(42);
        return;
    case 2:
        cyc(46);
        if (dec8_mem(cr, WA(wram_0E55))) {
            cyc(22);
            start_draw(cr, b);
            return;
        } else { cyc(16); }
        substate_next(cr);
        lda8_imm(cr, 0x14);
        cyc(16);
        WR8(wram_0E55, 0x14);
        lda8_imm(cr, 0x04);
        cyc(16);
        WR8(wram_0D54, 0x04);
        dec8_mem(cr, WA(wram_0D5B));
        (cyc(70), time_digits_full(cr));
        return;
    case 4:
        cyc(46);
        start_hold(cr);
        cyc(42);
        return;
    case 6:
        cyc(46);
        if (dec8_mem(cr, WA(wram_0E55)))
            { cyc(64); return; }
        cyc(40);
        WR8(wram_0D56, 0x00);
        lda8_imm(cr, 0x04);
        phases_both(cr);
        cyc(42);
        return;
    default:
        carried_jump(cr, 0x070000 | rd16(0x070000 | (uint16_t)(0xE2EF + cr->x)));
        return;
    }
}

void abi_RaceStartSteps(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RaceStartSteps(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:E2E4 RaceStartGlyphsSteps
 * WHAT IT DOES: Draws the nine-glyph word (GlyphWordAt1C28), then
 * RaceStartSteps.
 * HOW: JSR, SEP #$30, then RaceStartSteps' code, which follows.
 * Entry: M 8-bit, X 8-bit. Exit: as RaceStartSteps.
 */
void RaceStartGlyphsSteps(Carried *cr)
{
    GlyphWordAt1C28_cr(cr);
    r_sep(cr, P_M | P_X);
    RaceStartSteps(cr);
}

void abi_RaceStartGlyphsSteps(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RaceStartGlyphsSteps(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:E376 RaceStartTimer
 * WHAT IT DOES: Counts the timer $0E55 down: at 0 both players' phases 4
 * and $0D56 cleared; on the way, $0D54 = 2 as it reaches $11 and 8 as it
 * reaches 6.
 * HOW: DEC A and CMPs on the new value.
 * Entry: M 8-bit, X 8-bit. Exit: A low byte = the timer (or 4, 2, 8 as
 * stored); N, Z, C from the last compare or load.
 */
void RaceStartTimer(Carried *cr)
{
    uint8_t v;

    cyc(24);
    r_lda8(cr, WA(wram_0E55));
    if (!(uint8_t)cr->a) {
        cyc(16);
        lda8_imm(cr, 0x04);
        phases_both(cr);
        cyc(24);
        WR8(wram_0D56, 0x00);
        cyc(42);
        return;
    }
    v = (uint8_t)(cr->a - 1);
    lda8_imm(cr, v);
    cyc(36);
    WR8(wram_0E55, v);
    cmp8_imm(cr, 0x1A);
    if (v == 0x1A)
        { cyc(42); return; } else { cyc(12); }
    cmp8_imm(cr, 0x11);
    if (v == 0x11) {
        lda8_imm(cr, 0x02);
        cyc(10);
        WR8(wram_0D54, 0x02);
    }
    cyc(16);
    cmp8_imm(cr, 0x06);
    if ((uint8_t)cr->a != 0x06)
        { cyc(32); return; }
    lda8_imm(cr, 0x08);
    WR8(wram_0D54, 0x08);
    cyc(42);
}

void abi_RaceStartTimer(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RaceStartTimer(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* SBC #m (8-bit) on A; 1 when it borrows (C clear). */
static int sbc8_borrows(Carried *cr, uint8_t m)
{
    cyc(76);
    cr->a = (uint16_t)((cr->a & 0xFF00) | alu_sbc8((uint8_t)cr->a, m, &cr->p));
    return !(cr->p & P_C);
}

/*
 * @routine 07:E3A3 RaceStartTimerSteps
 * WHAT IT DOES: Counts the timer $0E55 down: at 1 (its last) clears $0D56
 * and sets both players' phases 4; otherwise steps the sub-state $0D5A on
 * as the timer passes $35, $21 (also stepping $0D65), $19 (with $0D54 =
 * 2) and 9.
 * HOW: JMP ($E3BE,X) on $0D5A with A = the new timer; SEC, SBC #n, BCS.
 * The last step (07:E403) is a bare RTS.
 * Entry: M 8-bit, X 8-bit. Exit: A low byte = the timer less the step's
 * value, or 4; X = $0D5A; C from the subtraction.
 */
void RaceStartTimerSteps(Carried *cr)
{
    uint8_t v;

    cyc(24);
    r_lda8(cr, WA(wram_0E55));
    v = (uint8_t)(cr->a - 1);
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    set_p(cr, nz8(cr->p, v));
    if (!v) {
        cyc(54);
        WR8(wram_0D56, 0x00);
        lda8_imm(cr, 0x04);
        phases_both(cr);
        cyc(42);
        return;
    }
    cyc(60);
    WR8(wram_0E55, v);
    cyc(24);
    cr->x = RD8(wram_0D5A);
    set_p(cr, nz8(cr->p, (uint8_t)cr->x));
    switch (cr->x) {
    case 0:
        set_p(cr, (uint8_t)(cr->p | (P_C)));
        if (sbc8_borrows(cr, 0x35))
            substate_next(cr); else { cyc(6); }
        cyc(42);
        return;
    case 2:
        set_p(cr, (uint8_t)(cr->p | (P_C)));
        if (sbc8_borrows(cr, 0x21)) {
            inc8_mem(cr, WA(time_bonus_state));
            inc8_mem(cr, WA(time_bonus_state));
            substate_next(cr);
        } else { cyc(6); }
        cyc(42);
        return;
    case 4:
        cyc(6);
        set_p(cr, (uint8_t)(cr->p | (P_C)));
        if (sbc8_borrows(cr, 0x19)) {
            lda8_imm(cr, 0x02);
            cyc(10);
            WR8(wram_0D54, 0x02);
            substate_next(cr);
        }
        cyc(42);
        return;
    case 6:
        set_p(cr, (uint8_t)(cr->p | (P_C)));
        if (sbc8_borrows(cr, 0x09))
            substate_next(cr); else { cyc(6); }
        cyc(42);
        return;
    case 8:
        cyc(88);
        return;
    default:
        carried_jump(cr, 0x070000 | rd16(0x070000 | (uint16_t)(0xE3BE + cr->x)));
        return;
    }
}

void abi_RaceStartTimerSteps(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RaceStartTimerSteps(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:E557 PanelDelayCount
 * WHAT IT DOES: Counts the panel delay $0D5F down by the frames since the
 * last tick ($0306); C clear once it has run out.
 * HOW: SEC, SBC.
 * Entry: M 8-bit. Exit: A low byte = $0D5F; N, Z, C, V from the
 * subtraction.
 */
void PanelDelayCount(Carried *cr)
{

    cyc(24);
    r_lda8(cr, WA(wram_0D5F));
    set_p(cr, (uint8_t)(cr->p | (P_C)));
    cyc(38);
    cr->a = (uint16_t)((cr->a & 0xFF00) | alu_sbc8((uint8_t)cr->a, RD8(frames_since_tick), &cr->p));
    cyc(8);
    WR8(wram_0D5F, (uint8_t)cr->a);
    cyc(42);
}

void abi_PanelDelayCount(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PanelDelayCount(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* PanelDelayCount; returns 1 when it has run out (BCS not taken). */
static int panel_delay_out(Carried *cr)
{
    cyc(92);
    PanelDelayCount(cr);
    return !(cr->p & P_C);
}

/* The panel's next state, the delay $0D5F = d. */
static void panel_next_delay(Carried *cr, uint8_t d)
{
    panel_next(cr);
    lda8_imm(cr, d);
    cyc(16);
    WR8(wram_0D5F, d);
}

/* A result line shown, at the sprite byte at ($06, $0A, $0E: the lines'
   Y bytes, +4 each from results_line_y = $0400 + 6): with the split screen
   ($0D62) results_split_y's and results_split2_y's line = $20 and $88,
   else results_line_y's = $28; then the next state, the delay $32 and
   sound $49. */
static void panel_line(Carried *cr, uint16_t at)
{
    cyc(40);
    r_lda8(cr, WA(wram_0D62));
    if ((uint8_t)cr->a) {
        lda8_imm(cr, 0x20);
        cyc(32);
        wr8(0x7E0000 | (uint16_t)(W(results_split_y) - 6 + at), 0x20);
        lda8_imm(cr, 0x88);
        cyc(16);
        wr8(0x7E0000 | (uint16_t)(W(results_split2_y) - 6 + at), 0x88);
        cyc(22);                        /* BRA */
    } else {
        lda8_imm(cr, 0x28);
        cyc(38);
        wr8(0x7E0000 | (uint16_t)(W(results_line_y) - 6 + at), 0x28);
    }
    panel_next_delay(cr, 0x32);
    cyc(54);
    lda8_imm(cr, 0x49);
    SoundPort0QueuePush_cr(cr);
    cyc(42);
}

/*
 * @routine 07:E404 ResultsPanelSteps
 * WHAT IT DOES: Steps the results panel by $0D56, each step after the
 * delay $0D5F has run out (PanelDelayCount): 0 starts it (delay $3C,
 * $0D54 = 8); 2 waits; 4, 6, 8 show its three lines (sprite bytes $06,
 * $0A, $0E of the buffer at $0400, or for the split screen of the two at
 * $0420 and $0520; delay $32, sound $49); $A shows the marks ($A2 at
 * bytes $07, $0B, $0F; sound $4A), sets $0E5B = 1, clears $0D53, and in
 * mode 1 ($0D2B) sets $0E41 = 2 and $0E3F = $0F; $C plays sound 1 on port
 * 0 (delay $0F); $E sets $0D54 = $A; $10, once $0D54 is 0, sets $0D67 = 2
 * and both players' phases 6.
 * HOW: JMP ($E40A,X) on $0D56. The split-screen lines are not reached in
 * the traces so far.
 * Entry: M 8-bit, X 8-bit; D = $0000. Exit: as the step leaves it.
 */
void ResultsPanelSteps(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;
    static const uint16_t marks_split[6] = {0x0427, 0x0527, 0x042B, 0x052B, 0x042F, 0x052F};
    static const uint16_t marks[3] = {0x0407, 0x040B, 0x040F};
    unsigned i;

    cyc(24);
    cr->x = RD8(wram_0D56);
    set_p(cr, nz8(cr->p, (uint8_t)cr->x));
    switch (cr->x) {
    case 0x00:
        lda8_imm(cr, 0x3C);
        cyc(62);
        WR8(wram_0D5F, 0x3C);
        panel_next(cr);
        lda8_imm(cr, 0x08);
        cyc(16);
        WR8(wram_0D54, 0x08);
        cyc(42);
        return;
    case 0x02:
        if (panel_delay_out(cr))
            { cyc(16); panel_next_delay(cr, 0x32); } else { cyc(22); }
        cyc(42);
        return;
    case 0x04:
        if (panel_delay_out(cr))
            panel_line(cr, 0x06); else { cyc(64); }
        return;
    case 0x06:
        if (panel_delay_out(cr))
            panel_line(cr, 0x0A); else { cyc(64); }
        return;
    case 0x08:
        if (panel_delay_out(cr))
            panel_line(cr, 0x0E); else { cyc(64); }
        return;
    case 0x0A:
        if (!panel_delay_out(cr))
            { cyc(64); return; }
        cyc(40);
        r_lda8(cr, WA(wram_0D62));
        if ((uint8_t)cr->a) {
            cyc(8);
            lda8_imm(cr, 0xA2);
            for (i = 0; i < 6; i++)
                { cyc(24); wr8(b | marks_split[i], 0xA2); }
        cyc(22);
            } else {
            cyc(14);
            lda8_imm(cr, 0xA2);
            for (i = 0; i < 3; i++)
                { cyc(24); wr8(b | marks[i], 0xA2); }
        }
        panel_next_delay(cr, 0x32);
        cyc(54);
        lda8_imm(cr, 0x4A);
        SoundPort0QueuePush_cr(cr);
        cyc(16);
        lda8_imm(cr, 0x01);
        WR8(wram_0E5B, 0x01);
        cyc(24);
        WR8(wram_0D53, 0x00);
        cyc(24);
        r_lda8(cr, WA(hud_mode));
        cmp8_imm(cr, 0x01);
        if ((uint8_t)cr->a != 0x01)
            { cyc(48); return; }
        lda8_imm(cr, 0x02);
        cyc(16);
        WR8(wram_0E41, 0x02);
        lda8_imm(cr, 0x0F);
        cyc(16);
        WR8(wram_0E3F, 0x0F);
        cyc(42);
        return;
    case 0x0C:
        if (!panel_delay_out(cr))
            { cyc(64); return; } else { cyc(16); }
        panel_next_delay(cr, 0x0F);
        lda8_imm(cr, 0x01);
        SoundPort0QueuePushRaw_cr(cr);
        cyc(34);
        return;
    case 0x0E:
        if (!panel_delay_out(cr))
            { cyc(64); return; }
        lda8_imm(cr, 0x0A);
        cyc(32);
        WR8(wram_0D54, 0x0A);
        panel_next(cr);
        cyc(42);
        return;
    case 0x10:
        cyc(70);
        r_lda8(cr, WA(wram_0D54));
        cyc(32);
        if ((uint8_t)cr->a)
            { cyc(32); return; }
        lda8_imm(cr, 0x02);
        WR8(bonus_digit_state, 0x02);
        lda8_imm(cr, 0x06);
        phases_both(cr);
        cyc(42);
        return;
    default:
        carried_jump(cr, 0x070000 | rd16(0x070000 | (uint16_t)(0xE40A + cr->x)));
        return;
    }
}

void abi_ResultsPanelSteps(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ResultsPanelSteps(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void abi_GsuF441Player1(void);
void abi_GsuF441Player2(void);
void abi_OamStagePause(void);
void abi_OamStagePause2(void);
void abi_OamStagePauseLoop(void);
void abi_OamStagePause2Loop(void);
void abi_PauseOnStart(void);
void abi_RaceFinishPhases10(void);
void abi_RaceFinishPhases0E(void);
void abi_PaletteDarkenFour(void);
void abi_Countdown0DF8(void);
void abi_OamHide75Up(void);
void abi_SoundReset(void);
void abi_RaceTimeAddD814(void);
void abi_RaceTimeAddD815(void);
void abi_LapSplit(void);
void abi_RecordCompareTime0(void);
void abi_RecordCompareTime1(void);
void abi_OamHideForHudMode(void);
void abi_SoundPort1QueuePush(void);
void abi_GlyphSpritesInit(void);
void abi_RaceTimeKeep(void);
void abi_TimePackBcd(void);
void abi_SpecialRecordInsert(void);
void abi_RecordsLoadCourseLong(void);
void abi_GsuF0A5OrF0B4(void);
void abi_GsuEEF5OrEFCD(void);
void abi_RecordTimeCompare(void);
void abi_PlayerSound35(void);
void abi_SaveSumsRecordsB(void);
void abi_RaceFinishSplit(void);

/* Player y's byte at `at`: work RAM through the data bank, indexed by Y. */
#define PY(b, at) ((b) | (uint16_t)((at) + cr->y))

/* The car's sound for a finish: 8, or $15 for car 4 or more. */
static void finish_sound(Carried *cr, uint8_t car)
{
    cr->a = (uint16_t)((cr->a & 0xFF00) | car);
    cyc(44);
    cmp8_imm(cr, 0x04);
    lda8_imm(cr, car < 0x04 ? 0x08 : (cyc(16), 0x15));
    cyc(16);
    SoundPort0QueuePush_cr(cr);
    cyc(28);
}

/*
 * @routine 07:E944 CarFinishSound
 * WHAT IT DOES: Plays the finish's sound for player 1's car ($0D89): 8, or
 * $15 for car 4 or more.
 * HOW: PHP, SEP #$30; PLP at the end. Car 4 and up is not reached in the
 * traces so far.
 * Entry: any widths. Exit: P unchanged; A low byte = the sound, or as
 * SoundPort0QueuePush leaves it.
 */
void CarFinishSound(Carried *cr)
{
    uint8_t p0 = cr->p;

    r_sep(cr, P_M | P_X);
    cyc(46);
    r_lda8(cr, WA(wram_0D89));
    finish_sound(cr, (uint8_t)cr->a);
    r_plp(cr, p0);
}

void abi_CarFinishSound(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CarFinishSound(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:E937 CarFinishSoundP2
 * WHAT IT DOES: As CarFinishSound for player 2: sets the current player
 * $1A5D = 1, then the sound for its car ($0D8A).
 * HOW: Enters CarFinishSound's compare. Not reached in the traces so far.
 * Entry: any widths. Exit: as CarFinishSound.
 */
void CarFinishSoundP2(Carried *cr)
{
    uint8_t p0 = cr->p;

    r_sep(cr, P_M | P_X);
    lda8_imm(cr, 0x01);
    cyc(38);
    WR8(current_player, 0x01);
    cyc(24);
    r_lda8(cr, WA(wram_0D8A));
    cyc(22);
    finish_sound(cr, (uint8_t)cr->a);
    r_plp(cr, p0);
}

void abi_CarFinishSoundP2(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CarFinishSoundP2(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:D672 RaceFinishSounds
 * WHAT IT DOES: The sounds of a race's finish: by mode $0D2B, 0: $0D5E =
 * the player mode $0DF4, sound 2 on port 0 for mode 1 (then $48 on port 0
 * after the car's sound) or 3 for the others; 2: PlayerSound35; others:
 * compares record time 2 with the best (RecordTimeCompare), sound 2 on
 * port 0 when it is worse ($0FA3 = 1), else 3; then the car's sound
 * (CarFinishSound).
 * HOW: PHP, REP #$30; PLP, RTL. Only mode 0 with player mode 1 is reached
 * in the traces so far.
 * Entry: any widths. Exit: P unchanged; A as the last sound call leaves it.
 */
void RaceFinishSounds(Carried *cr)
{
    uint8_t p0 = cr->p;
    int worse;

    r_rep(cr, P_M | P_X);
    cyc(46);
    r_lda16(cr, WA(hud_mode));
    if (cr->a == 0x0000) {
        r_sep(cr, P_M);
        cyc(68);
        r_lda8(cr, WA(wram_0DF4));
        cyc(24);
        WR8(wram_0D5E, (uint8_t)cr->a);
        cmp8_imm(cr, 0x01);
        if ((uint8_t)cr->a == 0x01) {
            cyc(22);
            lda8_imm(cr, 0x02);
            SoundPort0QueuePushRaw_cr(cr);
            (cyc(54), CarFinishSound(cr));
            cyc(54);
            lda8_imm(cr, 0x48);
            SoundPort0QueuePush_cr(cr);
        } else {
            cyc(16);
            lda8_imm(cr, 0x03);
            SoundPort0QueuePushRaw_cr(cr);
            (cyc(54), CarFinishSound(cr));
        }
        cyc(30);
        r_plp(cr, p0);
        return;
    } else { cyc(104); }
    r_cmp16(cr, 0x0002);
    if (cr->a == 0x0002) {
        cyc(4);                         /* BEQ taken, JSR: 108 in all after the load */
        PlayerSound35_cr(cr);
        (cyc(88), CarFinishSound(cr));
        cyc(30);
        r_plp(cr, p0);
        return;
    } else { cyc(22); }
    cr->x = 0x0006;
    set_p(cr, nz16(cr->p, 0x0006));
    RecordTimeCompare_cr(cr);
    r_sep(cr, P_M);
    cyc(46);
    r_lda8(cr, WA(p1_lead));
    cmp8_imm(cr, 0x01);
    worse = (uint8_t)cr->a == 0x01;
    lda8_imm(cr, worse ? (cyc(16), 0x02) : (cyc(22), 0x03));
    SoundPort0QueuePushRaw_cr(cr);
    if (worse)
        cyc(22);   /* BRA */
    (cyc(54), CarFinishSound(cr));
    cyc(30);
    r_plp(cr, p0);
}

void abi_RaceFinishSounds(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RaceFinishSounds(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* Both phases 8, CarFinishSound, sound 4 on port 1: a finish that ends the
   race for both. */
static void finish_both(Carried *cr)
{
    lda8_imm(cr, 0x08);
    cyc(16);
    WR8(wram_0DD6, 0x08);
    (cyc(46), CarFinishSound(cr));
    cyc(54);
    lda8_imm(cr, 0x04);
    SoundPort1QueuePushX8(cr);
    cyc(42);
}

/* A banner's size from the table at $07:E95A (by its timer $0E3F + Y)
   through draw (M 16-bit), then the timer counted down; at 0 the banner
   $0E41 + Y is cleared. */
static void lap_banner(Carried *cr, void (*draw)(Carried *))
{
    uint32_t b = (uint32_t)cr->db << 16;
    uint8_t v;

    cyc(24);
    r_lda8(cr, PY(b, 0x0E3F));
    cr->x = (uint16_t)(cr->a & 0x00FF);
    set_p(cr, nz8(cr->p, (uint8_t)cr->x));
    cyc(114);
    r_lda8(cr, b | (uint16_t)(0xE95A + cr->x));
    r_rep(cr, P_M);
    r_and16(cr, 0x00FF);
    draw(cr);
    r_sep(cr, P_M | P_X);
    cyc(96);
    r_lda8(cr, PY(b, 0x0E3F));
    v = (uint8_t)(cr->a - 1);
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    set_p(cr, nz8(cr->p, v));
    if (!v) {
        lda8_imm(cr, 0x00);
        cyc(52);
        wr8(PY(b, 0x0E41), 0x00);
    } else { cyc(36); }
    cyc(30);
    wr8(PY(b, 0x0E3F), (uint8_t)cr->a);
    cyc(42);
}

/*
 * @routine 07:E9C0 LapBannerNone
 * WHAT IT DOES: Nothing: banner 2 of RacePhaseLap's banners.
 * HOW: RTS. Not reached in the traces so far.
 * Entry: any widths. Exit: every register unchanged.
 */
/* LapBannerNone on carried registers: no register work. */
void LapBannerNone_cr(Carried *cr)
{
    (void)cr;
    cyc(42);
}

void abi_LapBannerNone(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    LapBannerNone_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:E9C1 LapBannerGrow
 * WHAT IT DOES: Shows player Y's lap banner at the size its timer $0E3F +
 * Y picks from the table at $07:E95A (GsuF0A5OrF0B4), and counts the
 * timer down; at 0 clears the banner $0E41 + Y.
 * HOW: The size masked 16-bit; SEP #$30 after.
 * Entry: M 8-bit, X 8-bit; Y = the player. Exit: M 8-bit, X 8-bit; A low
 * byte = the timer; X = the old timer.
 */
void LapBannerGrow(Carried *cr)
{
    lap_banner(cr, GsuF0A5OrF0B4_cr);
}

void abi_LapBannerGrow(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    LapBannerGrow(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:E9E1 LapBannerGrowLast
 * WHAT IT DOES: As LapBannerGrow with GsuEEF5OrEFCD (the last lap's
 * banner).
 * HOW: As LapBannerGrow.
 * Entry: as LapBannerGrow. Exit: as LapBannerGrow.
 */
void LapBannerGrowLast(Carried *cr)
{
    lap_banner(cr, GsuEEF5OrEFCD_cr);
}

void abi_LapBannerGrowLast(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    LapBannerGrowLast(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:EA01 LapBannerWait
 * WHAT IT DOES: Counts player Y's banner timer $0E3F + Y down; at 0 clears
 * the banner, sets $0E57 = $FF and plays sound 3 on port 0.
 * HOW: DEC A. Not reached in the traces so far.
 * Entry: M 8-bit, X 8-bit; Y = the player. Exit: A low byte = the timer.
 */
void LapBannerWait(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;
    uint8_t v;

    cyc(24);
    r_lda8(cr, PY(b, 0x0E3F));
    v = (uint8_t)(cr->a - 1);
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    set_p(cr, nz8(cr->p, v));
    if (!v) {
        cyc(60);
        wr8(PY(b, 0x0E41), 0x00);
        lda8_imm(cr, 0xFF);
        cyc(16);
        WR8(wram_0E57, 0xFF);
        lda8_imm(cr, 0x03);
        SoundPort0QueuePushRaw_cr(cr);
        cyc(8);                         /* the push's pulls at this caller's widths */
        lda8_imm(cr, 0x00);
    } else { cyc(60); }
    cyc(6);
    wr8(PY(b, 0x0E3F), (uint8_t)cr->a);
    cyc(42);
}

void abi_LapBannerWait(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    LapBannerWait(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* 07:E608 / 07:E656: a finish without the split screen: the palette
   darkened (PaletteDarkenFour with colour), $0E18 = $F0, $0E5B and $0D5B
   cleared, Countdown0DF8, phase + Y = phase, $0E56 = t, $0E4B = t,
   OamHide75Up, SoundReset and sound $37. */
static void finish_alone(Carried *cr, uint32_t b, uint16_t colour, uint8_t phase, uint8_t t)
{
    cyc(70);
    r_rep(cr, P_M);
    r_lda16_imm(cr, colour);
    PaletteDarkenFour_cr(cr);
    r_sep(cr, P_M);
    lda8_imm(cr, 0xF0);
    cyc(38);
    WR8(wram_0E18, 0xF0);
    cyc(24);
    WR8(wram_0E5B, 0x00);
    cyc(24);
    WR8(wram_0D5B, 0x00);
    (cyc(62), Countdown0DF8_cr(cr));
    lda8_imm(cr, phase);
    cyc(22);
    wr8(PY(b, 0x0DD6), phase);
    lda8_imm(cr, t);
    cyc(16);
    WR8(wram_0E56, t);
    cyc(24);
    WR8(wram_0E4B, t);
    (cyc(46), OamHide75Up_cr(cr));
    (cyc(62), SoundReset_cr(cr));
    cyc(54);
    lda8_imm(cr, 0x37);
    SoundPort0QueuePush_cr(cr);
}

/* 07:E6BD: the lap's time with the override $0D4C: sound $35, $0DEA one
   more, the lap's split (BCD, decimal mode) from the time at $0E63-$0E66
   less the last mark at $0EDB-$0EDD into $0EDE-$0EE0, the mark moved on,
   then the split packed (TimePackBcd) and kept in the course's save entry
   ($70:FC5E + X from RecordOffsetAllCourses) when it is better, with the
   level $0F11 and the car $0D89 (HudBestTimeDigits, SaveSumsRecordsB); $0FA3 = 1
   (kept), $FF (worse) or 0 (equal); $0E5D = $12C. Reached only by
   unit_race_phases. */
static void lap_override(Carried *cr, uint32_t b)
{
    (void)b;
    uint8_t v, m;
    uint16_t x, t;

    lda8_imm(cr, 0x35);
    cyc(16);
    SoundPort0QueuePush_cr(cr);
    cyc(24);
    r_lda8(cr, WA(wram_0DEA));
    v = (uint8_t)(cr->a + 1);
    lda8_imm(cr, v);
    cyc(14);
    WR8(wram_0DEA, v);
    cyc(24);
    r_lda8(cr, WA(race_time_minutes));
    cyc(16);
    WR8(dp_f1, (uint8_t)cr->a);
    set_p(cr, (uint8_t)(cr->p | (P_D)));
    cyc(38);
    r_lda8(cr, WA(race_time_fraction));
    set_p(cr, (uint8_t)(cr->p | (P_C)));
    cyc(38);
    cr->a = (uint16_t)((cr->a & 0xFF00) | alu_sbc8((uint8_t)cr->a, RD8(lap_mark), &cr->p));
    cyc(8);
    WR8(lap_splits, (uint8_t)cr->a);
    cyc(24);
    r_lda8(cr, WA(race_time_seconds));
    cyc(24);
    cr->a = (uint16_t)((cr->a & 0xFF00) | alu_sbc8((uint8_t)cr->a, RD8(wram_0EDC), &cr->p));
    if (!(cr->p & P_C)) {
        cyc(22);
        cr->a = (uint16_t)((cr->a & 0xFF00) | alu_adc8((uint8_t)cr->a, 0x60, &cr->p));
        dec8_mem(cr, W(dp_f1));
    } else { cyc(6); }
    cyc(24);
    WR8(wram_0EDF, (uint8_t)cr->a);
    cyc(16);
    r_lda8(cr, W(dp_f1));
    set_p(cr, (uint8_t)(cr->p | (P_C)));
    cyc(38);
    m = RD8(wram_0EDD);
    cr->a = (uint16_t)((cr->a & 0xFF00) | alu_sbc8((uint8_t)cr->a, m, &cr->p));
    cyc(8);
    WR8(wram_0EE0, (uint8_t)cr->a);
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_D)));
    cyc(38);
    r_lda8(cr, WA(race_time_fraction));
    cyc(24);
    WR8(lap_mark, (uint8_t)cr->a);
    cyc(24);
    r_lda8(cr, WA(race_time_seconds));
    cyc(24);
    WR8(wram_0EDC, (uint8_t)cr->a);
    cyc(24);
    r_lda8(cr, WA(race_time_minutes));
    cyc(24);
    WR8(wram_0EDD, (uint8_t)cr->a);
    r_rep(cr, P_M | P_X);
    RecordOffsetAllCourses(cr);
    x = cr->x;                                  /* PHX */
    cyc(112);
    r_lda16(cr, 0x70FC5E + x);
    cyc(24);
    WR16(packed_time_a, cr->a);
    r_rep(cr, P_M);
    cyc(24);
    r_lda16(cr, WA(wram_0EE0));
    r_and16(cr, 0x00FF);
    r_tay16(cr);
    cyc(60);
    r_lda16(cr, WA(wram_0EDF));
    r_and16(cr, 0x00FF);
    r_tax16(cr);
    cyc(30);
    r_lda16(cr, WA(lap_splits));
    r_and16(cr, 0x00FF);
    (cyc(68), TimePackBcd_cr(cr));
    cyc(24);
    WR16(packed_time_b, cr->a);
    cr->x = x;                                  /* PLX */
    set_p(cr, nz16(cr->p, x));
    cyc(68);
    t = rd16(0x70FC5E + cr->x);
    r_cmp16(cr, t);
    if (cr->a < t) {
        cyc(48);
        wr16(0x70FC5E + cr->x, cr->a);
        cyc(92);                                /* PHX, JSL */
        HudBestTimeDigits(cr);
        cr->x = x;
        set_p(cr, nz16(cr->p, x));
        cyc(60);
        r_lda16(cr, WA(wram_0F11));
        cyc(32);
        wr16(0x70FC60 + cr->x, cr->a);
        r_sep(cr, P_M);
        cyc(46);
        r_lda8(cr, WA(wram_0D89));
        cyc(32);
        wr8(0x70FC62 + cr->x, (uint8_t)cr->a);
        SaveSumsRecordsB_cr(cr);
        lda8_imm(cr, 0x01);
    } else if (cr->a != t) {
        cyc(20);
        r_sep(cr, P_M);
        lda8_imm(cr, 0xFF);
    } else {
        cyc(4);
        r_sep(cr, P_M);
        lda8_imm(cr, 0x00);
    }
    cyc(78);
    WR8(p1_lead, (uint8_t)cr->a);
    r_rep(cr, P_M);
    r_lda16_imm(cr, 0x012C);
    cyc(10);
    WR16(hud_hold, 0x012C);
    cyc(42);
}

/* 07:E7DC: the last lap done (third count): $0E5B cleared, $0E57 and
   $0E4B counted down; the finish by the split screen, mode, player mode
   and $0D24, the sprites of the finishing panel, LapSplit 6,
   RaceFinishSounds, the phase 8 with the banner timer $14, $0D55
   cleared, $0E58 = $70; OamHideForHudMode, RaceTimeKeep, the time (at most
   99) to $0E6C, and below 8 minutes the time kept as a special course's
   record (TimePackBcd, SpecialRecordInsert, RecordsLoadCourseLong). */
static void lap_finish(Carried *cr, uint32_t b)
{
    uint16_t w;

    r_sep(cr, P_M);
    cyc(52);
    WR8(wram_0E5B, 0x00);
    dec8_mem(cr, WA(wram_0E57));
    dec8_mem(cr, WA(wram_0E4B));
    cyc(24);
    r_lda8(cr, WA(wram_0D62));
    if ((uint8_t)cr->a) {
        lda8_imm(cr, 0xFF);
        cyc(32);
        WR8(wram_0D53, 0xFF);
        (cyc(46), RaceFinishSplit(cr));
        cyc(42);
        return;
    }
    cyc(46);
    r_lda8(cr, WA(hud_mode));
    if ((uint8_t)cr->a)
        { cyc(40); goto hold; }
    cyc(46);
    r_lda8(cr, WA(wram_0DF4));
    cmp8_imm(cr, 0x04);
    if ((uint8_t)cr->a >= 0x04) {
        cyc(24);
        WR8(wram_0D5B, 0x00);
        lda8_imm(cr, 0x23);
        cyc(16);
        WR8(wram_0E58, 0x23);
        lda8_imm(cr, 0x0A);
        cyc(22);
        wr8(PY(b, 0x0DD6), 0x0A);
        (cyc(46), OamHideForHudMode_cr(cr));
        (cyc(62), Countdown0DF8_cr(cr));
        cyc(54);
        lda8_imm(cr, 0x51);
        SoundPort0QueuePushX8(cr);
        cyc(42);
        return;
    }
    r_sep(cr, P_M);
    cyc(52);
    r_lda8(cr, WA(wram_0D24));
    cmp8_imm(cr, 0x03);
    if ((uint8_t)cr->a == 0x03) {
        finish_both(cr);
        return;
    }
    r_rep(cr, P_M);
    cyc(30);
    r_lda16(cr, WA(wram_0DF4));
    r_and16(cr, 0x00FF);
    r_cmp16(cr, 0x0002);
    if (cr->a < 0x0002)
        w = 0x8000;
    else if (cr->a == 0x0002)
        { cyc(16); w = 0x8006; }
    else
        { cyc(10); w = 0x800C; }
    r_lda16_imm(cr, w);
    cyc(78);
    WR16(glyph_spr_glyph, w);
    r_lda16_imm(cr, (uint16_t)(w + 0x0400));
    cyc(10);
    WR16(wram_1012, (uint16_t)(w + 0x0400));
    w = w == 0x8000 ? 0x8012 : 0x8018;
    r_lda16_imm(cr, w);
    cyc(10);
    WR16(wram_1014, w);
    if (w == 0x8018)
        cyc(22);   /* BRA */
    r_lda16_imm(cr, 0x0021);
    cyc(10);
    WR16(glyph_spr_1040, 0x0021);
    r_lda16_imm(cr, 0x001D);
    cyc(10);
    WR16(wram_1042, 0x001D);
    r_lda16_imm(cr, 0x0019);
    cyc(10);
    WR16(wram_1044, 0x0019);
    r_lda16_imm(cr, 0x0008);
    cyc(10);
    WR16(wram_1070, 0x0008);
    r_lda16_imm(cr, 0x001C);
    cyc(10);
    WR16(wram_1072, 0x001C);
    r_lda16_imm(cr, 0x002F);
    cyc(10);
    WR16(wram_1074, 0x002F);
    r_lda16_imm(cr, 0x00D7);
    cyc(10);
    WR16(glyph_spr_x, 0x00D7);
    r_lda16_imm(cr, 0x00D3);
    cyc(10);
    WR16(wram_0FB2, 0x00D3);
    r_lda16_imm(cr, 0x00CF);
    cyc(10);
    WR16(wram_0FB4, 0x00CF);
    r_sep(cr, P_M);
    cyc(46);
    r_lda8(cr, WA(wram_0D48));
    cmp8_imm(cr, 0x03);
    if ((uint8_t)cr->a == 0x03) {
        cyc(62);
        GlyphSpritesInit_cr(cr);
        goto panel;
    }
hold:
    r_rep(cr, P_M);
    r_lda16_imm(cr, 0xFFFF);
    cyc(16);
    WR16(hud_hold, 0xFFFF);
    r_sep(cr, P_M);
    cyc(46);
    r_lda8(cr, WA(wram_10CD));
    if ((uint8_t)cr->a) {
        cyc(16);
        finish_both(cr);
        return;
    }
panel:
    r_sep(cr, P_M);
    lda8_imm(cr, 0x08);
    cyc(66);
    wr8(PY(b, 0x0E41), 0x08);
    r_sep(cr, P_M | P_X);
    cr->x = 0x0006;
    set_p(cr, nz8(cr->p, 0x06));
    (cyc(62), w_LapSplit(cr));
    (cyc(62), RaceFinishSounds(cr));
    r_sep(cr, P_M);
    lda8_imm(cr, 0x14);
    cyc(44);
    wr8(PY(b, 0x0E3F), 0x14);
    lda8_imm(cr, 0x08);
    cyc(22);
    wr8(PY(b, 0x0DD6), 0x08);
    cyc(24);
    WR8(panel_b_shown, 0x00);
    lda8_imm(cr, 0x70);
    cyc(16);
    WR8(wram_0E58, 0x70);
    (cyc(46), OamHideForHudMode_cr(cr));
    (cyc(46), w_RaceTimeKeep(cr));
    r_rep(cr, P_M | P_X);
    cyc(24);
    r_lda16(cr, WA(race_time_bcd));
    r_cmp16(cr, 0x0099);
    if (cr->a >= 0x0099)
        r_lda16_imm(cr, 0x0099); else { cyc(20); }
    cyc(50);
    WR16(wram_0E6C, cr->a);
    cyc(24);
    r_lda16(cr, WA(race_time_fraction));
    cyc(24);
    cr->x = RD16(race_time_seconds);
    set_p(cr, nz16(cr->p, cr->x));
    cyc(24);
    cr->y = RD16(race_time_minutes);
    set_p(cr, nz16(cr->p, cr->y));
    set_p(cr, (uint8_t)((nz16(cr->p, (uint16_t)(cr->y - 0x0008)) & ~P_C) | (cr->y >= 0x0008 ? (cyc(46), P_C) : (cyc(86), 0))));
    if (cr->y >= 0x0008)
        { cyc(42); return; }
    TimePackBcd_cr(cr);
    (cyc(46), SpecialRecordInsert_cr(cr));
    (cyc(62), RecordsLoadCourseLong_cr(cr));
    cyc(42);                            /* RTS */
}

/* 07:E6B5: a lap crossed: with the override $0D4C the lap's time
   (lap_override); else $0DEA + Y counted: 1 and 2 add the lap's time
   (RaceTimeAddD814 / D815), $0E5D = $12C, the banner $0E41 + Y = 4 or 6
   with timer $17, sound $48, LapSplit 0 or 3 and RecordCompareTime0 / 1;
   3 the finish (lap_finish); a wrap to 0 only sound $48. */
static void lap_crossed(Carried *cr, uint32_t b)
{
    uint8_t v;

    cyc(24);
    r_lda8(cr, WA(digit62_override));
    if ((uint8_t)cr->a) {
        cyc(60);
        lap_override(cr, b);
        return;
    }
    cyc(70);
    r_lda8(cr, PY(b, 0x0DEA));
    v = (uint8_t)(cr->a + 1);
    lda8_imm(cr, v);
    cyc(20);
    wr8(PY(b, 0x0DEA), v);
    cmp8_imm(cr, 0x02);
    if (v == 0x02) {
        cyc(68);
        RaceTimeAddD815_cr(cr);
        r_rep(cr, P_M);
        r_lda16_imm(cr, 0x012C);
        cyc(10);
        WR16(hud_hold, 0x012C);
        r_sep(cr, P_M);
        cyc(76);
        lda8_imm(cr, 0x48);
        SoundPort0QueuePush_cr(cr);
        lda8_imm(cr, 0x06);
        cyc(22);
        wr8(PY(b, 0x0E41), 0x06);
        lda8_imm(cr, 0x17);
        cyc(22);
        wr8(PY(b, 0x0E3F), 0x17);
        r_sep(cr, P_M | P_X);
        cr->x = 0x0003;
        set_p(cr, nz8(cr->p, 0x03));
        (cyc(62), w_LapSplit(cr));
        (cyc(62), RecordCompareTime1_cr(cr));
        cyc(42);
        return;
    } else { cyc(16); }
    if (v > 0x02) {
        lap_finish(cr, b);
        return;
    }
    cmp8_imm(cr, 0x01);
    if (v != 0x01) {
        cyc(54);
        lda8_imm(cr, 0x48);
        SoundPort0QueuePushX8(cr);
        cyc(42);
        return;
    }
    cyc(68);
    RaceTimeAddD814_cr(cr);
    r_rep(cr, P_M);
    r_lda16_imm(cr, 0x012C);
    cyc(10);
    WR16(hud_hold, 0x012C);
    r_sep(cr, P_M);
    lda8_imm(cr, 0x04);
    cyc(44);
    wr8(PY(b, 0x0E41), 0x04);
    lda8_imm(cr, 0x17);
    cyc(22);
    wr8(PY(b, 0x0E3F), 0x17);
    cyc(54);
    lda8_imm(cr, 0x48);
    SoundPort0QueuePush_cr(cr);
    r_sep(cr, P_M | P_X);
    cr->x = 0x0000;
    set_p(cr, nz8(cr->p, 0x00));
    (cyc(62), w_LapSplit(cr));
    (cyc(62), RecordCompareTime0_cr(cr));
    cyc(42);
}

/*
 * @routine 07:E6B5 RaceLapCrossed
 * WHAT IT DOES: A lap crossed by player Y (see lap_crossed): its lap
 * count, time and banner, or the finish after the third.
 * HOW: The code after RacePhaseLap's lap test; also the target of a JMP
 * from the time-up phase (07:ED65).
 * Entry: M 8-bit, X 8-bit; Y = the player; D = $0000. Exit: as the path
 * leaves it.
 */
void RaceLapCrossed(Carried *cr)
{
    lap_crossed(cr, (uint32_t)cr->db << 16);
}

void abi_RaceLapCrossed(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RaceLapCrossed(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* 07:E575: the race phase's work after the pause tests. */
static int race_phase(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;
    uint16_t w;

    cyc(24);
    r_lda8(cr, WA(race_time_minutes));
    cmp8_imm(cr, 0x09);
    if ((uint8_t)cr->a == 0x09) {
        cyc(24);
        r_lda8(cr, WA(race_time_seconds));
        cmp8_imm(cr, 0x59);
        if ((uint8_t)cr->a == 0x59) {
            cyc(24);
            r_lda8(cr, WA(race_time_fraction));
            cmp8_imm(cr, 0x99);
            if ((uint8_t)cr->a == 0x99)
                goto time_up;
        } else { cyc(6); }
    } else { cyc(6); }
    cyc(24);
    r_lda8(cr, WA(hud_mode));
    cmp8_imm(cr, 0x02);
    if ((uint8_t)cr->a == 0x02)
        goto progress;
    cmp8_imm(cr, 0x03);
    if ((uint8_t)cr->a == 0x03)
        goto progress;
    r_rep(cr, P_M);
    cyc(24);
    r_lda16(cr, WA(race_time_bcd));
    if (cr->a)
        { cyc(16); goto progress; } else { cyc(32); }
    r_sep(cr, P_M);
time_up:
    cyc(30);
    r_lda8(cr, WA(wram_0D62));
    if (!(uint8_t)cr->a) {
        lda8_imm(cr, 0x20);
        cyc(32);
        WR8(wram_0E5A, 0x20);
        lda8_imm(cr, 0x0C);
        cyc(22);
        wr8(PY(b, 0x0DD6), 0x0C);
        cyc(54);
        lda8_imm(cr, 0x31);
        SoundPort0QueuePush_cr(cr);
        return 2;
    }
    lda8_imm(cr, 0x20);
    cyc(38);
    WR8(wram_0E5A, 0x20);
    cyc(24);
    WR8(wram_0E58, 0x20);
    lda8_imm(cr, 0x14);
    phases_both(cr);
    cyc(24);
    WR8(wram_0D53, 0x00);
    cyc(54);
    lda8_imm(cr, 0x31);
    SoundPort0QueuePush_cr(cr);
    return 2;
progress:
    r_sep(cr, P_M);
    cyc(52);
    r_lda8(cr, PY(b, 0x0DEE));
    r_xba(cr);
    cyc(44);
    r_lda8(cr, PY(b, 0x0DEC));
    r_rep(cr, P_M);
    w = cr->a;
    set_p(cr, (uint8_t)((cr->p & ~P_Z) | ((w & 0x0001) ? 0 : P_Z)));
    if (!(w & 0x0001)) {
        r_sep(cr, P_M | P_X);
        set_p(cr, (uint8_t)((nz8(cr->p, (uint8_t)cr->y) & ~P_C) | P_C));     /* CPY #0 */
        cyc(126);
        cr->x = rd8(b | ((uint8_t)cr->y ? 0x0E42 : (cyc(6), 0x0E41)));
        set_p(cr, nz8(cr->p, (uint8_t)cr->x));
        switch (cr->x) {
        case 0x00:
        case 0x0A:
            r_sep(cr, P_M);
            cyc(92);
            r_lda8(cr, PY(b, 0x0DE8));
            {
                cyc(24);
                uint8_t m = rd8(PY(b, 0x0DEA)), v = (uint8_t)cr->a;

                set_p(cr, (uint8_t)((nz8(cr->p, (uint8_t)(v - m)) & ~P_C) | (v >= m ? (cyc(38), P_C) : 0)));
                if (v > m) {
                    lap_crossed(cr, b);
                    return 0;
                }
            }
            return 1;
        case 0x02:
            cyc(20);
            w_LapBannerNone(cr);
            return 1;
        case 0x04:
            cyc(46);
            LapBannerGrow(cr);
            return 0;
        case 0x06:
            cyc(46);
            LapBannerGrowLast(cr);
            return 0;
        case 0x08:
            cyc(46);
            LapBannerWait(cr);
            return 0;
        default:
            carried_jump(cr, 0x070000 | rd16(0x070000 | (uint16_t)(0xE69C + cr->x)));
            return 1;
        }
    }
    set_p(cr, (uint8_t)((cr->p & ~P_Z) | ((w & 0x1000) ? 0 : (cyc(16), P_Z))));
    r_sep(cr, P_M);
    cyc(132);
    r_lda8(cr, WA(wram_0D62));
    if (w & 0x1000) {
        if (!(uint8_t)cr->a) {
            finish_alone(cr, b, 0xFC00, 0x10, 0x0E);
            return 2;
        }
        lda8_imm(cr, 0x10);
        cyc(38);
        wr8(PY(b, 0x0DD6), 0x10);
        lda8_imm(cr, 0x0C);
        cyc(16);
        WR8(wram_0E56, 0x0C);
        cyc(54);
        lda8_imm(cr, 0x37);
        SoundPort0QueuePush_cr(cr);
        cyc(16);
        lda8_imm(cr, 0xFF);
        WR8(wram_0D53, 0xFF);
        (cyc(46), RaceFinishPhases10(cr));
        cyc(16);
        return 1;
    }
    if (!(uint8_t)cr->a) {
        finish_alone(cr, b, 0x801F, 0x0E, 0x0C);
        return 2;
    }
    lda8_imm(cr, 0x0E);
    cyc(38);
    wr8(PY(b, 0x0DD6), 0x0E);
    lda8_imm(cr, 0x0C);
    cyc(16);
    WR8(wram_0E56, 0x0C);
    lda8_imm(cr, 0xFF);
    cyc(16);
    WR8(wram_0D53, 0xFF);
    (cyc(30), RaceFinishPhases0E(cr));
    cyc(8);
    return 1;
}

/*
 * @routine 07:E562 RacePhaseLapP1
 * WHAT IT DOES: Player Y's race phase on the split screen, for player 1:
 * Super FX program $07:F441 (GsuF441Player1) and the pause by pad 1
 * (OamStagePause); then the race: the time up at 9:59.99, or in modes 0
 * and 1 when the time at $0E70 runs out (phase $0C, or for the split
 * screen both phases $14; sound $31); the car's crash or fall (bits 12 and
 * 0 of $0DEE/$0DEC + Y: phase $10 or $0E, with the split screen
 * RaceFinishPhases10 / 0E, else the palette darkened and the sound
 * reset); else the banner step $0E41 (+1 for player 2): 0 and $A test a
 * lap crossed ($0DE8 + Y above $0DEA + Y: RaceLapCrossed), 4 and 6 grow
 * the lap banners, 8 waits.
 * HOW: JMP ($E69C,X) on the banner step. The time-up at 9:59.99, the
 * split-screen time up and the crash without the split screen are not
 * reached in the traces so far.
 * Entry: M 8-bit, X 8-bit; Y = the player; D = $0000. Exit: as the path
 * leaves it.
 */
void RacePhaseLapP1(Carried *cr)
{
    int r;

    cyc(46);
    GsuF441Player1_cr(cr);
    (cyc(118), OamStagePause(cr));
    cyc(22);
    r = race_phase(cr);
    if (r)
        cyc(r == 2 ? 42 : 26);
}

void abi_RacePhaseLapP1(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RacePhaseLapP1(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:E56A RacePhaseLapP2
 * WHAT IT DOES: As RacePhaseLapP1 for player 2 (GsuF441Player2,
 * OamStagePause2).
 * HOW: As RacePhaseLapP1.
 * Entry: as RacePhaseLapP1. Exit: as RacePhaseLapP1.
 */
void RacePhaseLapP2(Carried *cr)
{
    int r;

    cyc(46);
    GsuF441Player2_cr(cr);
    (cyc(118), OamStagePause2(cr));
    cyc(22);
    r = race_phase(cr);
    if (r)
        cyc(r == 2 ? 42 : 26);
}

void abi_RacePhaseLapP2(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RacePhaseLapP2(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:E572 RacePhaseLap
 * WHAT IT DOES: As RacePhaseLapP1 for one player: the pause on START
 * (PauseOnStart), then the race.
 * HOW: As RacePhaseLapP1.
 * Entry: as RacePhaseLapP1. Exit: as RacePhaseLapP1.
 */
void RacePhaseLap(Carried *cr)
{
    int r;

    cyc(46);
    PauseOnStart(cr);
    r = race_phase(cr);
    if (r)
        cyc(r == 2 ? 42 : 26);
}

void abi_RacePhaseLap(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RacePhaseLap(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void abi_GsuF15BFive(void);
void abi_HudSixGlyphs(void);
void abi_GsuF15BThree(void);
void abi_BigSpritesRow5(void);
void abi_BigSpritesRow6(void);
void abi_RaceEndCountdownFade(void);
void abi_RaceEndCountdown(void);
void abi_WindowSlideUpSound(void);
void abi_OverlayFramesUntilFaded(void);
void abi_GlyphSpritesGrowAndTimes(void);
void abi_GlyphSlideThree(void);
void abi_ChampionScreenRun(void);
void abi_SoundDataLoad(void);
void abi_ApuUpload(void);
void abi_RaceVideoInit(void);

/*
 * @routine 07:E972 PhaseGlyphsFive
 * WHAT IT DOES: Draws five glyphs with Super FX program $07:F15B
 * (GsuF15BFive): a phase's whole work.
 * HOW: JSR, RTS.
 * Entry: D = $0000. Exit: as GsuF15BFive leaves it.
 */
void PhaseGlyphsFive(Carried *cr)
{
    GsuF15BFive_cr(cr);
}

void abi_PhaseGlyphsFive(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PhaseGlyphsFive(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:E976 PhaseGlyphsSix
 * WHAT IT DOES: Draws six HUD glyphs (HudSixGlyphs): a phase's whole work.
 * HOW: JSR, RTS.
 * Entry: D = $0000. Exit: as HudSixGlyphs leaves it.
 */
void PhaseGlyphsSix(Carried *cr)
{
    cyc(46);
    HudSixGlyphs_cr(cr);
    cyc(42);
}

void abi_PhaseGlyphsSix(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PhaseGlyphsSix(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* Once the slide out at $7F:3468 has reached $1F: $0E5F cleared, $0D22 =
   the slide, $0D23 = 6, $0E56 = $78, $0D55 cleared and the race's end
   (RaceEndCountdownFade); before, draw() for the frame. */
static void race_end_wait(Carried *cr, void (*draw)(Carried *))
{

    cyc(32);
    r_lda8(cr, W(slide_out));
    cmp8_imm(cr, 0x1F);
    if ((uint8_t)cr->a < 0x1F) {
        cyc(52);
        draw(cr);
        return;
    }
    cyc(24);
    WR8(palette_animate, 0x00);
    cyc(24);
    WR8(wram_0D22, (uint8_t)cr->a);
    lda8_imm(cr, 0x06);
    cyc(16);
    WR8(wram_0D23, 0x06);
    lda8_imm(cr, 0x78);
    cyc(16);
    WR8(wram_0E56, 0x78);
    cyc(24);
    WR8(panel_b_shown, 0x00);
    (cyc(46), RaceEndCountdownFade(cr));
}

void abi_BigSpritesRow6B(void);

/*
 * @routine 07:E97A RaceEndWaitRow6B
 * WHAT IT DOES: Draws with BigSpritesRow6B each frame until the slide out at
 * $7F:3468 reaches $1F; then $0E5F cleared, $0D22 = the slide, $0D23 = 6,
 * $0E56 = $78, $0D55 cleared and the race's end (RaceEndCountdownFade).
 * HOW: An 8-bit compare of the long byte. Reached only by
 * unit_race_phases.
 * Entry: M 8-bit, X 8-bit. Exit: as BigSpritesRow6B leaves it, or does not return.
 */
void RaceEndWaitRow6B(Carried *cr)
{
    race_end_wait(cr, BigSpritesRow6B_cr);
}

void abi_RaceEndWaitRow6B(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RaceEndWaitRow6B(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:E99D RaceEndWaitBigSprites
 * WHAT IT DOES: As RaceEndWaitRow6B, drawing the five big sprites
 * (BigSpritesRow5) each frame.
 * HOW: As RaceEndWaitRow6B. The race's end is not reached in the traces so
 * far.
 * Entry: M 8-bit, X 8-bit. Exit: as BigSpritesRow5 leaves it, or does not
 * return.
 */
void RaceEndWaitBigSprites(Carried *cr)
{
    race_end_wait(cr, BigSpritesRow5_cr);
}

void abi_RaceEndWaitBigSprites(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RaceEndWaitBigSprites(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* $0D22 = 1, $0D23 cleared and the window slid up with its sound. */
static void window_up(Carried *cr)
{
    lda8_imm(cr, 0x01);
    cyc(16);
    WR8(wram_0D22, 0x01);
    cyc(24);
    WR8(wram_0D23, 0x00);
    (cyc(62), WindowSlideUpSound_cr(cr));
}

/* The B / START test of the presses $0D1A (bits 15 and 12), 16-bit. */
static int pressed_b_start(Carried *cr)
{
    r_rep(cr, P_M);
    cyc(40);
    r_lda16(cr, WA(joy1_pressed_copy));
    set_p(cr, (uint8_t)((cr->p & ~P_Z) | ((cr->a & 0x9000) ? 0 : (cyc(24), P_Z))));
    return (cr->a & 0x9000) != 0;
}

void abi_RaceRestart(void);
void abi_RaceEndToAttract(void);
void abi_RaceEndVideoReset(void);
void abi_SoftResetToBootWarm(void);

/* 07:EA6A / EADD: once the split window is in state 5 ($19D8), $0D59
   cleared, OverlayFramesUntilFaded, then on. */
static int window_closed_overlay(Carried *cr, int beq)
{
    cyc(30);
    r_lda8(cr, WA(window_hdma_off));
    cmp8_imm(cr, 0x05);
    if ((uint8_t)cr->a != 0x05)
        { cyc(6); return 0; }
    cyc(beq ? 30 : 24);   /* BEQ taken to .lEADD (from 07:EA93), or BNE not taken (07:EA6A) */
    WR8(wram_0D59, 0x00);
    (cyc(46), OverlayFramesUntilFaded(cr));
    r_sep(cr, P_M);
    return 1;
}

/*
 * @routine 07:EA1B ResultsWindowWait
 * WHAT IT DOES: The results' window: with $10CD set, the soft reset
 * (SoftResetToBootWarm) once the fade is done ($0D61 negative); else
 * counts $0E58 down: at $10, or below $58 on B or START (unless the split
 * window is moving), slides the window up; at 0, once the window is
 * closed, runs the overlay frames and goes on to the race again
 * (RaceRestart, with $0DD4) or the video reset (RaceEndVideoReset).
 * HOW: 8-bit tests; the presses 16-bit. Reached only by unit_race_phases.
 * Entry: M 8-bit, X 8-bit. Exit: as the path leaves it, or does not
 * return.
 */
void ResultsWindowWait(Carried *cr)
{
    uint8_t v;

    cyc(24);
    r_lda8(cr, WA(wram_10CD));
    if ((uint8_t)cr->a) {
        cyc(40);
        r_lda8(cr, WA(fade_level_hi));
        if ((uint8_t)cr->a & 0x80)
            { cyc(40); SoftResetToBootWarm(cr); } else { cyc(64); }
        return;
    }
    cyc(46);
    r_lda8(cr, WA(wram_0E58));
    v = (uint8_t)cr->a;
    if (!v) {
        cyc(16);
        if (!window_closed_overlay(cr, 0))
            { cyc(42); return; }
        cyc(46);
        r_lda8(cr, WA(wram_0DD4));
        if ((uint8_t)cr->a)
            { cyc(40); RaceRestart(cr); }
        else
            { cyc(46); RaceEndVideoReset(cr); }
        return;
    } else { cyc(6); }
    cmp8_imm(cr, 0x10);
    if (v == 0x10) {
        cyc(10);
        window_up(cr);
    } else {
        cmp8_imm(cr, 0x58);
        if (v < 0x58 && pressed_b_start(cr)) {
            r_sep(cr, P_M);
            cyc(86);
            r_lda8(cr, WA(window_hdma_off));
            if ((uint8_t)cr->a)
                { cyc(64); return; }
            cyc(40);
            WR8(wram_0E58, 0x00);
            window_up(cr);
            cyc(42);
            return;
        }
    }
    cyc(44);
    r_sep(cr, P_M);
    dec8_mem(cr, WA(wram_0E58));
    cyc(42);
}

void abi_ResultsWindowWait(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ResultsWindowWait(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:EA85 RaceEndWindowSteps
 * WHAT IT DOES: The race's end window: with $0D48 = 3 the glyph sprites
 * grow (GlyphSpritesGrowAndTimes); else counts $0E58 down: at $10 slides
 * the window up ($19DF cleared), at $58 sets $0D65 = $A, below $58 slides
 * the three glyphs (GlyphSlideThree), at $2B with $0D65 set sets it to
 * $10; once the window is closed, the overlay frames, then: with $0D24 =
 * 3 or $0D48 other than 1 ($0D48 one more) the race again (RaceRestart);
 * with $0D48 = 1 the override $0D4C = $FF, the course $0D4A + $14, car 4
 * and the race again.
 * HOW: SEP #$30; A kept on the stack around GlyphSlideThree.
 * Entry: any widths. Exit: as the path leaves it, or does not return.
 */
void RaceEndWindowSteps(Carried *cr)
{
    uint8_t v;

    r_sep(cr, P_M | P_X);
    cyc(24);
    r_lda8(cr, WA(wram_0D48));
    cmp8_imm(cr, 0x03);
    if ((uint8_t)cr->a == 0x03) {
        cyc(62);
        GlyphSpritesGrowAndTimes_cr(cr);
        return;
    }
    if (window_closed_overlay(cr, 1)) {
        r_lda8(cr, WA(wram_0D24));
        cmp8_imm(cr, 0x03);
        if ((uint8_t)cr->a != 0x03) {
            r_lda8(cr, WA(wram_0D48));
            cmp8_imm(cr, 0x01);
            if ((uint8_t)cr->a == 0x01) {
                lda8_imm(cr, 0xFF);
                WR8(digit62_override, 0xFF);
                r_lda8(cr, WA(wram_0D4A));
                set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_C)));
                cr->a = (uint16_t)((cr->a & 0xFF00) | alu_adc8((uint8_t)cr->a, 0x14, &cr->p));
                WR8(course, (uint8_t)cr->a);
                lda8_imm(cr, 0x04);
                WR8(wram_0D89, 0x04);
            } else {
                inc8_mem(cr, WA(wram_0D48));
            }
        }
        RaceRestart(cr);
        return;
    }
    cyc(18);
    r_lda8(cr, WA(wram_0E58));
    v = (uint8_t)cr->a;
    cmp8_imm(cr, 0x10);
    if (v == 0x10) {
        cyc(24);
        WR8(wram_19DF, 0x00);
        window_up(cr);
        cyc(24);
        r_lda8(cr, WA(wram_0E58));
        v = (uint8_t)cr->a;
    } else { cyc(6); }
    cyc(16);
    cmp8_imm(cr, 0x58);
    if (v == 0x58) {
        lda8_imm(cr, 0x0A);
        WR8(time_bonus_state, 0x0A);
        cyc(24);
        r_lda8(cr, WA(wram_0E58));
        v = (uint8_t)cr->a;
        (cyc(90), GlyphSlideThree_cr(cr));
        lda8_imm(cr, v);                            /* PLA */
    } else if (v < 0x58) {
        cyc(74);
        GlyphSlideThree_cr(cr);
        lda8_imm(cr, v);                            /* PLA */
    } else { cyc(8); }
    cyc(10);
    cmp8_imm(cr, 0x2B);
    if (v == 0x2B) {
        cyc(18);
        r_lda8(cr, WA(time_bonus_state));
        if ((uint8_t)cr->a) {
            lda8_imm(cr, 0x10);
            cyc(32);
            WR8(time_bonus_state, 0x10);
            cyc(24);
            r_lda8(cr, WA(wram_0E58));
        }
    }
    dec8_mem(cr, WA(wram_0E58));
    cyc(42);
}

void abi_RaceEndWindowSteps(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RaceEndWindowSteps(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* $0D2F = $0D2D = $A: the pause's stage loop from its start. */
static void pause_stage_ten(Carried *cr)
{
    lda8_imm(cr, 0x0A);
    cyc(16);
    WR8(wram_0D2F, 0x0A);
    cyc(24);
    WR8(wram_0D2D, 0x0A);
}

/*
 * @routine 07:EB10 PauseResultsP2
 * WHAT IT DOES: The paused results for player 2: three glyphs
 * (GsuF15BThree); counts $0E58 down, at its end sets $0D2E = 1, slides the
 * window up and sets $0D2F and the delay $0D5F to 8; then counts the delay
 * down, and at its end sets $0D2F = $0D2D = $A and runs pad 2's pause loop
 * (OamStagePause2Loop, 07:BB7C).
 * HOW: 8-bit; the pause loop is a jump (its end leaves through the
 * caller's frame).
 * Entry: D = $0000. Exit: as the path leaves it.
 */
void PauseResultsP2(Carried *cr)
{
    cyc(46);

    GsuF15BThree_cr(cr);
    r_sep(cr, P_M);
    cyc(118);
    r_lda8(cr, WA(wram_0E58));
    if ((uint8_t)cr->a) {
        cyc(16);
        if (dec8_mem(cr, WA(wram_0E58)))
            { cyc(64); return; }
        lda8_imm(cr, 0x01);
        cyc(32);
        WR8(wram_0D2E, 0x01);
        (cyc(62), WindowSlideUpSound_cr(cr));
        lda8_imm(cr, 0x08);
        cyc(16);
        WR8(wram_0D2F, 0x08);
        cyc(24);
        WR8(wram_0D5F, 0x08);
        cyc(42);
        return;
    }
    cyc(46);
    r_lda8(cr, WA(wram_0D5F));
    if ((uint8_t)cr->a) {
        cyc(16);
        dec8_mem(cr, WA(wram_0D5F));
        cyc(42);
        return;
    } else { cyc(22); }
    pause_stage_ten(cr);
    (cyc(24), OamStagePause2Loop(cr));
}

void abi_PauseResultsP2(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PauseResultsP2(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void abi_GsuF15BFourA(void);
void abi_GsuF15BFourB(void);
void abi_BigSpritesNine(void);
void abi_BigSpritesRow6B(void);

/*
 * @routine 07:EB45 PhaseGlyphsFourB
 * WHAT IT DOES: Draws with GsuF15BFourB: a phase's whole work.
 * HOW: JSR, RTS. Reached only by unit_race_phases.
 * Entry: D = $0000. Exit: as GsuF15BFourB leaves it.
 */
void PhaseGlyphsFourB(Carried *cr)
{
    GsuF15BFourB_cr(cr);
    cyc(42);
}

void abi_PhaseGlyphsFourB(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PhaseGlyphsFourB(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* 07:EB51: the paused results: counts $0E58 down, at its end sound $51,
   $0D2E cleared, the window slid up, $0D2F = 8, the delay $0D5F = $14;
   then counts the delay down; at its end the time-up sound $30 at 9:59,
   $0D2F = $0D2D = $A and pad 1's pause loop (OamStagePauseLoop, 07:BB16). */
static void pause_results(Carried *cr)
{

    r_sep(cr, P_M);
    cyc(52);
    r_lda8(cr, WA(wram_0E58));
    if ((uint8_t)cr->a) {
        cyc(16);
        if (dec8_mem(cr, WA(wram_0E58)))
            { cyc(38); return; } else { cyc(70); }
        lda8_imm(cr, 0x51);
        SoundPort0QueuePush_cr(cr);
        lda8_imm(cr, 0x00);
        cyc(16);
        WR8(wram_0D2E, 0x00);
        (cyc(62), WindowSlideUpSound_cr(cr));
        lda8_imm(cr, 0x08);
        cyc(16);
        WR8(wram_0D2F, 0x08);
        lda8_imm(cr, 0x14);
        cyc(16);
        WR8(wram_0D5F, 0x14);
        cyc(16);
        return;
    }
    cyc(46);
    r_lda8(cr, WA(wram_0D5F));
    if ((uint8_t)cr->a) {
        cyc(16);
        dec8_mem(cr, WA(wram_0D5F));
        cyc(16);
        return;
    }
    cyc(46);
    r_lda8(cr, WA(race_time_minutes));
    cmp8_imm(cr, 0x09);
    if ((uint8_t)cr->a == 0x09) {
        cyc(24);
        r_lda8(cr, WA(race_time_seconds));
        cmp8_imm(cr, 0x59);
        if ((uint8_t)cr->a == 0x59) {
            cyc(16);
            lda8_imm(cr, 0x30);
            SoundPort0QueuePushRaw_cr(cr);
            cyc(8);                     /* the push's pulls at this caller's widths */
        } else { cyc(6); }
    } else { cyc(6); }
    pause_stage_ten(cr);
    (cyc(32), OamStagePauseLoop(cr));
}

/*
 * @routine 07:EB49 PauseResultsFourB
 * WHAT IT DOES: The paused results (see PauseResults), drawing with
 * GsuF15BFourB.
 * HOW: JSR, then PauseResults' code. Reached only by unit_race_phases and unit_entries_07.
 * Entry: D = $0000. Exit: as the path leaves it.
 */
void PauseResultsFourB(Carried *cr)
{
    GsuF15BFourB_cr(cr);
    cyc(16);                            /* BRA (22) for the 6 pause_results states from PauseResults' entry */
    pause_results(cr);
    cyc(26);
}

void abi_PauseResultsFourB(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PauseResultsFourB(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:EB4E PauseResults
 * WHAT IT DOES: The paused results for player 1: three glyphs
 * (GsuF15BThree); counts $0E58 down, at its end sound $51, $0D2E cleared,
 * the window slid up, $0D2F = 8 and the delay $0D5F = $14; then counts the
 * delay down; at its end sound $30 on port 0 when the time is 9:59,
 * $0D2F = $0D2D = $A and pad 1's pause loop (OamStagePauseLoop, 07:BB16).
 * HOW: 8-bit; the pause loop is a long jump (its end leaves through the
 * caller's frame). The 9:59 sound is not reached in the traces so far.
 * Entry: D = $0000. Exit: as the path leaves it.
 */
void PauseResults(Carried *cr)
{
    cyc(46);
    GsuF15BThree_cr(cr);
    (cyc(66), pause_results(cr));
    cyc(26);
}

void abi_PauseResults(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PauseResults(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:EBA0 PhaseGlyphsFourA
 * WHAT IT DOES: Draws with GsuF15BFourA: a phase's whole work.
 * HOW: JSR, RTS. Reached only by unit_race_phases and unit_entries_07.
 * Entry: D = $0000. Exit: as GsuF15BFourA leaves it.
 */
void PhaseGlyphsFourA(Carried *cr)
{
    GsuF15BFourA_cr(cr);
    cyc(42);
}

void abi_PhaseGlyphsFourA(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PhaseGlyphsFourA(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:EBDC PhaseNothingEBDC
 * WHAT IT DOES: Nothing: a phase handler with no work.
 * HOW: RTS (RaceEndWindowNine's last).
 * Entry: any widths. Exit: every register unchanged.
 */
/* PhaseNothingEBDC on carried registers: its cycles, nothing else. */
void PhaseNothingEBDC_cr(Carried *cr)
{
    (void)cr;
    cyc(42);
}

void abi_PhaseNothingEBDC(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PhaseNothingEBDC_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void abi_RaceEndLeave(void);

/*
 * @routine 07:EBA4 RaceEndWindowNine
 * WHAT IT DOES: Draws with BigSpritesNine; counts $0E58 down: at 1, or below $28
 * on B or START, clears it, sets $19DF = 1 and slides the window up; at 0,
 * once the window is closed, leaves the race (RaceEndLeave).
 * HOW: 8-bit; the presses 16-bit. Reached only by unit_race_phases.
 * Entry: D = $0000. Exit: as the path leaves it, or does not return.
 */
void RaceEndWindowNine(Carried *cr)
{
    uint8_t v;

    (cyc(46), BigSpritesNine_cr(cr));
    r_sep(cr, P_M);
    cyc(132);
    r_lda8(cr, WA(wram_0E58));
    v = (uint8_t)cr->a;
    if (!v) {
        cyc(46);
        r_lda8(cr, WA(window_hdma_off));
        cmp8_imm(cr, 0x05);
        if ((uint8_t)cr->a == 0x05)
            RaceEndLeave(cr);
        else
            cyc(42);
        return;
    }
    cmp8_imm(cr, 0x01);
    if (v != 0x01) {
        cmp8_imm(cr, 0x28);
        if (v >= 0x28 || !pressed_b_start(cr)) {
            cyc(44);
            r_sep(cr, P_M);
            dec8_mem(cr, WA(wram_0E58));
            cyc(42);
            return;
        } else { cyc(40); }
        r_sep(cr, P_M);
    }
    cyc(46);
    WR8(wram_0E58, 0x00);
    lda8_imm(cr, 0x01);
    cyc(16);
    WR8(wram_19DF, 0x01);
    (cyc(62), WindowSlideUpSound_cr(cr));
    cyc(42);
}

void abi_RaceEndWindowNine(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RaceEndWindowNine(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* SEI, SEP #$20, the screen blanked ($80 to INIDISP), the NMI, DMA and
   HDMA off. */
static void screen_off(Carried *cr)
{
    set_p(cr, (uint8_t)(cr->p | (P_I)));
    r_sep(cr, P_M);
    lda8_imm(cr, 0x80);
    cyc(52);
    ppu_wr(0x002100, 0x80);
    cyc(24);
    hw_wr(0x004200, 0x00);
    cyc(24);
    mdmaen_wr(0x00420B, 0x00);
    cyc(24);
    hdmaen_wr(0x00420C, 0x00);
}

/* REP #$30, command 5 to APU port 1 (16-bit), the delay of $4000 counts
   of A. */
static void apu_command5(Carried *cr)
{
    r_rep(cr, P_M | P_X);
    r_lda16_imm(cr, 0x0005);
    cyc(26);
    hw_wr16(0x002141, 0x0005);
    cyc(96);
    delay_turns(0x4000);
    cr->a = 0x0000;
    set_p(cr, (uint8_t)((cr->p & ~P_N) | P_Z));
}

/* ApuUpload of block a from y, then the delay of $4000 counts of A. */
static void apu_upload(Carried *cr, uint16_t a, uint16_t y)
{
    r_lda16_imm(cr, a);
    cr->y = y;
    set_p(cr, nz16(cr->p, y));
    w_ApuUpload(cr);
    r_rep(cr, P_M);
    cyc(24);
    delay_turns(0x4000);
    cr->a = 0x0000;
    set_p(cr, (uint8_t)((cr->p & ~P_N) | P_Z));
}

void abi_BootWarm(void);
void abi_BootWarmRace(void);
void abi_BootWarmAttract(void);

/*
 * @routine 07:EBE9 SoftResetToBootWarm
 * WHAT IT DOES: Restarts the game after the race: the screen blanked, the
 * NMI, DMA and HDMA off; the sound driver's data reloaded (SoundDataLoad
 * of $F3B5, $1E) and blocks $1B ($85F8) and $19 ($9A98) uploaded, each
 * after command 5 on APU port 1 and a delay; SoundReset, 1 to APU port 0;
 * then BootWarm.
 * HOW: Delay loops of $4000 counts of A; P saved around each step; a long
 * jump to BootWarm, which resets the stack (cpu_restart).
 * Entry: any widths. Exit: never returns.
 */
void SoftResetToBootWarm(Carried *cr)
{
    uint8_t p1;

    screen_off(cr);
    p1 = cr->p;
    (cyc(14), apu_command5(cr));
    r_lda16_imm(cr, 0xF3B5);
    cr->y = 0x001E;
    set_p(cr, nz16(cr->p, 0x001E));
    SoundDataLoad_cr(cr);
    r_plp(cr, p1);
    p1 = cr->p;
    apu_command5(cr);
    apu_upload(cr, 0x001B, 0x85F8);
    r_plp(cr, p1);
    p1 = cr->p;
    apu_command5(cr);
    apu_upload(cr, 0x0019, 0x9A98);
    r_plp(cr, p1);
    (cyc(48), SoundReset_cr(cr));
    r_sep(cr, P_M);
    lda8_imm(cr, 0x01);
    cyc(38);
    hw_wr(0x002140, 0x01);
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_I)));
    cyc(46); /* CLI, JML */
    carried_restart(cr, 0x038BAA, abi_BootWarm);
}

void abi_SoftResetToBootWarm(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    SoftResetToBootWarm(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:EC6B RaceRestart
 * WHAT IT DOES: Runs the race again: the screen blanked, the NMI, DMA and
 * HDMA off, then on in BootWarm at 03:8BCD (the stack reset, RaceLoop).
 * HOW: A long jump into BootWarm (BootWarmRace), which resets the stack
 * (cpu_restart).
 * Entry: any widths. Exit: never returns.
 */
void RaceRestart(Carried *cr)
{
    screen_off(cr);
    cyc(32); /* JML */
    carried_restart(cr, 0x038BCD, abi_BootWarmRace);
}

void abi_RaceRestart(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RaceRestart(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:EC8F RaceEndToAttract
 * WHAT IT DOES: Leaves the race: the screen blanked, the NMI, DMA and HDMA
 * off, then on in BootWarm at 03:8BF8 (after its race: the stack reset and
 * the attract race).
 * HOW: As RaceRestart.
 * Entry: any widths. Exit: never returns.
 */
void RaceEndToAttract(Carried *cr)
{
    screen_off(cr);
    cyc(32); /* JML */
    carried_restart(cr, 0x038BF8, abi_BootWarmAttract);
}

void abi_RaceEndToAttract(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RaceEndToAttract(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:EC80 RaceEndVideoReset
 * WHAT IT DOES: HDMA off, the screen blanked, the video reset
 * (RaceVideoInit), then RaceEndToAttract.
 * HOW: SEP #$20, SEI; the code of RaceEndToAttract follows.
 * Entry: any widths. Exit: never returns.
 */
void RaceEndVideoReset(Carried *cr)
{
    r_sep(cr, P_M);
    cyc(46);
    hdmaen_wr(0x00420C, 0x00);
    lda8_imm(cr, 0x80);
    cyc(16);
    ppu_wr(0x002100, 0x80);
    set_p(cr, (uint8_t)(cr->p | (P_I)));
    (cyc(76), RaceVideoInit_cr(cr));
    (cyc(0), RaceEndToAttract(cr));
}

void abi_RaceEndVideoReset(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RaceEndVideoReset(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:EBDD RaceEndLeave
 * WHAT IT DOES: Leaves the race: with $0E5C set, the championship's
 * closing screen (ChampionScreenRun) and the restart
 * (SoftResetToBootWarm); else on in BootWarm (RaceEndToAttract).
 * HOW: A long branch to RaceEndToAttract's code. The championship's screen is
 * not reached in the traces so far.
 * Entry: M 8-bit. Exit: never returns.
 */
void RaceEndLeave(Carried *cr)
{
    cyc(24);
    r_lda8(cr, WA(wram_0E5C));
    if (!(uint8_t)cr->a) {
        cyc(46);
        RaceEndToAttract(cr);
        return;
    }
    ChampionScreenRun(cr);
    SoftResetToBootWarm(cr);
}

void abi_RaceEndLeave(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RaceEndLeave(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* SEP #$20, the time-up's end: $0D22 = $0D23 = 6, the timer $0E5A = t,
   $0D55 cleared, sound $30 on port 0, then RaceEndCountdown. */
static void time_up_end(Carried *cr, uint8_t t)
{
    lda8_imm(cr, 0x06);
    cyc(20);
    WR8(wram_0D22, 0x06);
    cyc(24);
    WR8(wram_0D23, 0x06);
    lda8_imm(cr, t);
    cyc(16);
    WR8(wram_0E5A, t);
    cyc(24);
    WR8(panel_b_shown, 0x00);
    cyc(16);
    lda8_imm(cr, 0x30);
    SoundPort0QueuePushRaw_cr(cr);
    cyc(8);                             /* the push's pulls at this caller's widths */
    (cyc(24), RaceEndCountdown(cr));
}

/*
 * @routine 07:ECA4 TimeUpBattle
 * WHAT IT DOES: The time up of a battle: the six big sprites
 * (BigSpritesRow6); the first time sound $51, OamHide75Up and both timers
 * $0E59, $0E5A = $3C; counts $0E5A down; once $0E59 is set, at the end of
 * $0E5A the save's bad flag $00:1FFE = $11, $0D22 = $0D23 = 6, the timer
 * $3C, $0D55 cleared, sound $30 and the race's end (RaceEndCountdown).
 * HOW: 8-bit; the second part counts $0E5A down twice a frame. The second
 * part is reached only by unit_race_phases.
 * Entry: D = $0000. Exit: as the path leaves it, or does not return.
 */
void TimeUpBattle(Carried *cr)
{
    uint8_t a;

    (cyc(46), BigSpritesRow6_cr(cr));
    r_sep(cr, P_M);
    cyc(46);
    r_lda8(cr, WA(wram_0E59));
    if (!(uint8_t)cr->a) {
        cyc(40);
        r_lda8(cr, WA(wram_0E5A));
        if (!(uint8_t)cr->a) {
            cyc(70);
            lda8_imm(cr, 0x51);
            SoundPort0QueuePush_cr(cr);
            (cyc(46), OamHide75Up_cr(cr));
            lda8_imm(cr, 0x3C);
            cyc(16);
            WR8(wram_0E5A, 0x3C);
            cyc(24);
            WR8(wram_0E59, 0x3C);
        }
        dec8_mem(cr, WA(wram_0E5A));
        cyc(42);
        return;
    } else { cyc(44); }
    r_sep(cr, P_M);
    if (dec8_mem(cr, WA(wram_0E5A))) {
        cyc(22);
        dec8_mem(cr, WA(wram_0E5A));
        cyc(42);
        return;
    }
    a = (uint8_t)cr->a;                         /* PHA */
    cyc(86);
    WR8(save_bad_flag, 0x11);
    lda8_imm(cr, a);                                /* PLA */
    time_up_end(cr, 0x3C);
}

void abi_TimeUpBattle(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    TimeUpBattle(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:ECF3 PhaseNothingECF3
 * WHAT IT DOES: Nothing: a phase handler with no work.
 * HOW: RTS (TimeUpBattle's last).
 * Entry: any widths. Exit: every register unchanged.
 */
/* PhaseNothingECF3 on carried registers: no register work. */
void PhaseNothingECF3_cr(Carried *cr)
{
    (void)cr;
    cyc(42);
}

void abi_PhaseNothingECF3(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PhaseNothingECF3_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* 07:ED10 / 07:ED80: $0E5A = t, $0E59 one less, sound $51, then after(). */
static void time_up_tick(Carried *cr, uint8_t t, void (*after)(Carried *), uint8_t push_x)
{
    lda8_imm(cr, t);
    cyc(22);
    WR8(wram_0E5A, t);
    dec8_mem(cr, WA(wram_0E59));
    cyc(54);
    lda8_imm(cr, 0x51);
    SoundPort0QueuePush_cr(cr);
    (cyc(62 - push_x), after(cr));      /* push_x 16: X 16-bit, the push states those pulls */
    cyc(10);
}

/* The first part's wait: with $0DDA at $10 or more, phase 6 when the race
   time is not 0 (returns 2, the RTS stated), else $0E5A counted down;
   returns 1 to go on, 0 when it only counted down. */
static int time_up_wait(Carried *cr)
{
    cyc(40);
    r_lda8(cr, WA(wram_0DDA));
    cmp8_imm(cr, 0x10);
    if ((uint8_t)cr->a < 0x10)
        return 1;
    cyc(24);
    r_lda8(cr, WA(race_time_bcd));
    if ((uint8_t)cr->a) {
        lda8_imm(cr, 0x06);
        cyc(38);
        WR8(wram_0DD6, 0x06);
        cyc(42);
        return 2;
    } else { cyc(16); }
    return cyc_then(10, dec8_mem(cr, WA(wram_0E5A)) == 0);
}

/*
 * @routine 07:ECF4 TimeUpStunt
 * WHAT IT DOES: Stunt Trax's time up: the six big sprites
 * (BigSpritesRow6); while $0E59 is 0: with $0DDA at $10 or more, phase 6
 * when the time $0E70 is not 0, else counts $0E5A down; then $0E5A = $20,
 * $0E59 one less, sound $51 and OamHide75Up. Once $0E59 is set: at $0E5A
 * = 0, $0D22 = $0D23 = 6, the timer $78, $0D55 cleared, sound $30 and the
 * race's end (RaceEndCountdown); else counts $0E5A down.
 * HOW: 8-bit. Phase 6 is reached only by unit_race_phases.
 * Entry: D = $0000. Exit: as the path leaves it, or does not return.
 */
void TimeUpStunt(Carried *cr)
{
    int w;

    (cyc(46), BigSpritesRow6_cr(cr));
    r_sep(cr, P_M);
    cyc(46);
    r_lda8(cr, WA(wram_0E59));
    if (!(uint8_t)cr->a) {
        w = time_up_wait(cr);
        if (w == 1)
            time_up_tick(cr, 0x20, OamHide75Up_cr, 16);
        else if (w == 0)
            cyc(16);
        if (w != 2)
            cyc(32);
        return;
    }
    r_sep(cr, P_M);
    cyc(68);
    r_lda8(cr, WA(wram_0E5A));
    if ((uint8_t)cr->a) {
        cyc(22);
        dec8_mem(cr, WA(wram_0E5A));
        cyc(42);
        return;
    } else { cyc(12); }
    time_up_end(cr, 0x78);
}

void abi_TimeUpStunt(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    TimeUpStunt(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:ED4C TimeUpRace
 * WHAT IT DOES: A race's time up: the six big sprites (BigSpritesRow6);
 * while $0E59 is 0: a lap crossed meanwhile ($0DE8 above $0DEA) goes to
 * phase 6 and RaceLapCrossed; with $0DDA at $10 or more, phase 6 when the
 * time is not 0, else counts $0E5A down; then $0E5A = $24, $0E59 one less,
 * sound $51 and Countdown0DF8. Once $0E59 is set: at $0E5A = 0, once the
 * window is closed, leaves the race (RaceEndLeave); else counts $0E5A
 * down, at 1 setting $19DF = 1 and sliding the window up.
 * HOW: SEP #$30; DEC A and compares. The lap crossed and phase 6 are
 * reached only by unit_race_phases.
 * Entry: Y = the player; D = $0000. Exit: as the path leaves it, or does
 * not return.
 */
void TimeUpRace(Carried *cr)
{
    uint8_t v, m;
    int w;

    (cyc(46), BigSpritesRow6_cr(cr));
    r_sep(cr, P_M | P_X);
    cyc(24);
    r_lda8(cr, WA(wram_0E59));
    if (!(uint8_t)cr->a) {
        cyc(40);
        r_lda8(cr, WA(player_rec_7a));
        v = (uint8_t)cr->a;
        cyc(24);
        m = RD8(wram_0DEA);
        set_p(cr, (uint8_t)((nz8(cr->p, (uint8_t)(v - m)) & ~P_C) | (v >= m ? P_C : 0)));
        if (v > m) {
            lda8_imm(cr, 0x06);
            cyc(48);
            WR8(wram_0DD6, 0x06);
            (cyc(24), RaceLapCrossed(cr));
            return;
        } else { cyc(6); }
        w = time_up_wait(cr);
        if (w == 1)
            time_up_tick(cr, 0x24, Countdown0DF8_cr, 0);
        else if (w == 0)
            cyc(16);
        if (w != 2)
            cyc(32);
        return;
    }
    r_sep(cr, P_M);
    cyc(68);
    r_lda8(cr, WA(wram_0E5A));
    v = (uint8_t)cr->a;
    if (!v) {
        cyc(40);
        r_lda8(cr, WA(window_hdma_off));
        cmp8_imm(cr, 0x05);
        if ((uint8_t)cr->a == 0x05)
            { cyc(24); RaceEndLeave(cr); } else { cyc(26); }
        cyc(22);
        return;
    }
    v = (uint8_t)(v - 1);
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    set_p(cr, nz8(cr->p, v));
    cmp8_imm(cr, 0x01);
    if (v == 0x01) {
        cyc(60);
        WR8(wram_0E5A, v);
        lda8_imm(cr, 0x01);
        cyc(16);
        WR8(wram_19DF, 0x01);
        (cyc(62), WindowSlideUpSound_cr(cr));
        cyc(42);
        return;
    }
    cmp8_imm(cr, 0x18);
    cyc(v == 0x18 ? 66 : 72);                          /* BNE not taken / taken */
    WR8(wram_0E5A, v);
    cyc(42);
}

void abi_TimeUpRace(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    TimeUpRace(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:E323 RaceStartStepsDraw
 * WHAT IT DOES: RaceStartSteps' drawing at the timer $0E55: the race time
 * drawn (GsuBytes200_BFB5BB, GsuTimeDigits) at the digit size from the
 * table at $07:E33A by the timer.
 * HOW: The size byte read as a 16-bit word, ANDed to its low byte, into
 * $70:0210. The timer counts down from 7, so the word read at timer 7
 * takes the table's ninth byte and its tenth ($07:E343) is never read:
 * that byte is left to the raw intake as data, and the sub-state 4 code
 * after it is RaceStartStepsHold.
 * Entry: M 8-bit, X 8-bit; D = $0000. Exit: M 16-bit, X = the timer; as
 * GsuTimeDigits leaves the rest.
 */
void RaceStartStepsDraw(Carried *cr)
{
    start_draw(cr, (uint32_t)cr->db << 16);
}

void abi_RaceStartStepsDraw(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RaceStartStepsDraw(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:E344 RaceStartStepsHold
 * WHAT IT DOES: RaceStartSteps' sub-state 4, entered from its jump table:
 * waits on the timer $0E55 (not at all while $0D54 is 0), then the next
 * sub-state with $0D54 = 8 and the timer 3.
 * HOW: LDA/BEQ, DEC/BNE, two INCs of $0D5A, byte stores.
 * Entry: M 8-bit, X 8-bit; D = $0000. Exit: A low byte = 3, or $0D54 or
 * the timer as last loaded; N, Z from it.
 */
void RaceStartStepsHold(Carried *cr)
{
    start_hold(cr);
    cyc(42);
}

void abi_RaceStartStepsHold(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RaceStartStepsHold(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:E3E6 RaceStartTimerStepsRts
 * WHAT IT DOES: The second RTS ending RaceStartTimerSteps, which no code
 * reaches: returns at once.
 * HOW: RTS.
 * Entry: any. Exit: everything unchanged.
 */
/* RaceStartTimerStepsRts on carried registers: no register work. */
void RaceStartTimerStepsRts_cr(Carried *cr)
{
    (void)cr;
    cyc(42);
}

void abi_RaceStartTimerStepsRts(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RaceStartTimerStepsRts_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:E556 ResultsPanelStepsRts
 * WHAT IT DOES: The second RTS ending ResultsPanelSteps, which no code
 * reaches: returns at once.
 * HOW: RTS.
 * Entry: any. Exit: everything unchanged.
 */
/* ResultsPanelStepsRts on carried registers: no register work. */
void ResultsPanelStepsRts_cr(Carried *cr)
{
    (void)cr;
    cyc(42);
}

void abi_ResultsPanelStepsRts(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ResultsPanelStepsRts_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:E998 RaceEndWaitRow6BRts
 * WHAT IT DOES: The RTS after RaceEndWaitRow6B's call of
 * RaceEndCountdownFade, which never returns: returns at once.
 * HOW: RTS.
 * Entry: any. Exit: everything unchanged.
 */
/* RaceEndWaitRow6BRts on carried registers: no register work. */
void RaceEndWaitRow6BRts_cr(Carried *cr)
{
    (void)cr;
    cyc(42);
}

void abi_RaceEndWaitRow6BRts(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RaceEndWaitRow6BRts_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:E9BB RaceEndWaitBigSpritesRts
 * WHAT IT DOES: The RTS after RaceEndWaitBigSprites' call of
 * RaceEndCountdownFade, which never returns: returns at once.
 * HOW: RTS.
 * Entry: any. Exit: everything unchanged.
 */
/* RaceEndWaitBigSpritesRts on carried registers: no register work. */
void RaceEndWaitBigSpritesRts_cr(Carried *cr)
{
    (void)cr;
    cyc(42);
}

void abi_RaceEndWaitBigSpritesRts(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RaceEndWaitBigSpritesRts_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:EB0F RaceEndWindowStepsRts
 * WHAT IT DOES: The RTS after RaceEndWindowSteps' last JMP, which no code
 * reaches: returns at once.
 * HOW: RTS.
 * Entry: any. Exit: everything unchanged.
 */
/* RaceEndWindowStepsRts on carried registers: no register work. */
void RaceEndWindowStepsRts_cr(Carried *cr)
{
    (void)cr;
    cyc(42);
}

void abi_RaceEndWindowStepsRts(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RaceEndWindowStepsRts_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* LapSplit as its adapter leaves the registers. */
static void w_LapSplit(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = (uint16_t)((cr->a & 0xFF00) | LapSplit(cr->db, (uint8_t)cr->x, &p));
    set_p(cr, p);
}

/* RaceTimeKeep as its adapter leaves the registers. */
static void w_RaceTimeKeep(Carried *cr)
{
    uint8_t v;

    set_p(cr, (uint8_t)(cr->p | (P_M)));                               /* what an interrupt pushes */
    v = RaceTimeKeep(cr->db);
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    set_p(cr, nz8(cr->p, v));
}

/* LapBannerNone as its adapter leaves the registers. */
static void w_LapBannerNone(Carried *cr)
{
    cyc(42);
}

/* ApuUpload as its adapter leaves the registers. */
static void w_ApuUpload(Carried *cr)
{
    ApuUploadEnd end = ApuUpload((uint8_t)cr->a, cr->y);

    cr->a = end.kick;
    cr->x = 0;
    cr->y = end.offset;
    set_p(cr, (uint8_t)((cr->p & ~(P_N | P_V)) | P_Z | P_C | P_M));
}
