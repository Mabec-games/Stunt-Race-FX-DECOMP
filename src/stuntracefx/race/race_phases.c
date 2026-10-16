/*
 * race_phases.c: the race phase handlers, run each frame for each player
 * from the phase tables at $07:E232, $07:E24F and $07:E26E (phase $0DD6
 * for player 1, $0DD7 for player 2) and from the tables of the phase steps
 * in player_stats.c. They count the start, the race, the finish and the
 * results panels down, through the timer $0E55, the sub-state $0D5A and the
 * panel state $0D56. All run with 8-bit registers and data bank $07.
 */
#include "machine.h"
#include "cpu_ops.h"

void abi_SoundPort0QueuePush(void);
void abi_SoundPort0QueuePushRaw(void);
void abi_GlyphWordAt1C28(void);
void abi_GsuBytes200_BFB5BB(void);
void abi_GsuTimeDigits(void);
void abi_GsuTimeDigitsFull(void);

/* LDA #v (8-bit). */
static void lda8_imm(uint8_t v)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, v);
}

/* INC / DEC of a work-RAM byte: one read, one write; N, Z from it. */
static uint8_t inc8_mem(uint32_t addr)
{
    uint8_t v = (uint8_t)(rd8(addr) + 1);

    wr8(addr, v);
    cpu.p = nz8(cpu.p, v);
    return v;
}

static uint8_t dec8_mem(uint32_t addr)
{
    uint8_t v = (uint8_t)(rd8(addr) - 1);

    wr8(addr, v);
    cpu.p = nz8(cpu.p, v);
    return v;
}

/* CMP # (8-bit). */
static void cmp8_imm(uint8_t m)
{
    uint8_t a = (uint8_t)cpu.a;

    cpu.p = (uint8_t)((nz8(cpu.p, (uint8_t)(a - m)) & ~P_C) | (a >= m ? P_C : 0));
}

/* Both players' phases ($0DD6, $0DD7) = the low byte of A. */
static void phases_both(uint32_t b)
{
    wr8(b | 0x0DD6, (uint8_t)cpu.a);
    wr8(b | 0x0DD7, (uint8_t)cpu.a);
}

/* Two INCs of the sub-state $0D5A: the next one. */
static void substate_next(uint32_t b)
{
    inc8_mem(b | 0x0D5A);
    inc8_mem(b | 0x0D5A);
}

/* Two INCs of the panel state $0D56. */
static void panel_next(uint32_t b)
{
    inc8_mem(b | 0x0D56);
    inc8_mem(b | 0x0D56);
}

/*
 * @routine 07:E265 PlayerTwoFrameWork
 * WHAT IT DOES: Runs the handler for player 2's race phase $0DD7 from the
 * table at $07:E26E.
 * HOW: SEP #$30, then JMP (table,X); the handler returns to the caller.
 * Entry: any widths. Exit: as the handler leaves it.
 */
void abi_PlayerTwoFrameWork(void)
{
    op_sep(P_M | P_X);
    op_lda8(((uint32_t)cpu.db << 16) | 0x0DD7);
    cpu.x = (uint16_t)(cpu.a & 0x00FF);
    cpu.p = nz8(cpu.p, (uint8_t)cpu.x);
    jump_routine(0x070000 | rd16(0x070000 | (uint16_t)(0xE26E + cpu.x)));
}

/*
 * @routine 07:E284 PhaseNothing
 * WHAT IT DOES: Nothing: the handler of the phases with no work.
 * HOW: RTS.
 * Entry: any widths. Exit: every register unchanged.
 */
void abi_PhaseNothing(void)
{
}

/* 07:E299: both phases 2, $0D5A cleared; then with M 16-bit $0D67, $0D69,
   $19EE, $19F0 and $0E74 cleared and sound 5 on port 0. */
static void race_start(uint32_t b)
{
    lda8_imm(0x02);
    phases_both(b);
    wr8(b | 0x0D5A, 0x00);
    op_rep(P_M);
    wr16(b | 0x0D67, 0x0000);
    wr16(b | 0x0D69, 0x0000);
    wr16(b | 0x19EE, 0x0000);
    wr16(b | 0x19F0, 0x0000);
    wr16(b | 0x0E74, 0x0000);
    op_lda16_imm(0x0005);
    abi_SoundPort0QueuePushRaw();
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
void abi_RaceStartCountdown(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;

    wr8(b | 0x0FAE, 0x00);
    wr8(b | 0x0FAF, 0x00);
    lda8_imm(0x20);
    wr8(b | 0x0E55, 0x20);
    race_start(b);
}

/*
 * @routine 07:E292 RaceStartCountdownStunt
 * WHAT IT DOES: As RaceStartCountdown with the timer $0E55 = $12 and
 * $0FAE, $0FAF kept: Stunt Trax's start.
 * HOW: LDA #$07 then LDA #$12 (the first load has no use).
 * Entry: M 8-bit, X 8-bit. Exit: as RaceStartCountdown.
 */
void abi_RaceStartCountdownStunt(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;

    lda8_imm(0x07);
    lda8_imm(0x12);
    wr8(b | 0x0E55, 0x12);
    race_start(b);
}

/* Sound 5 on port 0, the timer $0E55 = t, both phases 2, $10CB and $10CC
   cleared. */
static void race_ready(uint8_t t)
{
    uint32_t b = (uint32_t)cpu.db << 16;

    lda8_imm(0x05);
    abi_SoundPort0QueuePushRaw();
    lda8_imm(t);
    wr8(b | 0x0E55, t);
    lda8_imm(0x02);
    phases_both(b);
    wr8(b | 0x10CB, 0x00);
    wr8(b | 0x10CC, 0x00);
}

/*
 * @routine 07:E2BD RaceReadyShort
 * WHAT IT DOES: Plays sound 5 on port 0; the timer $0E55 = $1E; both
 * players' phases 2; clears $10CB and $10CC.
 * HOW: 8-bit stores.
 * Entry: M 8-bit, X 8-bit. Exit: A low byte = 2; N, Z clear.
 */
void abi_RaceReadyShort(void)
{
    race_ready(0x1E);
}

/*
 * @routine 07:E2CA RaceReadyLong
 * WHAT IT DOES: As RaceReadyShort with the timer $0E55 = $36.
 * HOW: As RaceReadyShort.
 * Entry: M 8-bit, X 8-bit. Exit: as RaceReadyShort.
 */
void abi_RaceReadyLong(void)
{
    race_ready(0x36);
}

/* 07:E370: REP #$20, GsuTimeDigitsFull. */
static void time_digits_full(void)
{
    op_rep(P_M);
    abi_GsuTimeDigitsFull();
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
 * HOW: JMP ($E2EF,X) on $0D5A.
 * Entry: M 8-bit, X 8-bit; D = $0000. Exit: as the step leaves it: M
 * 16-bit, X 8-bit after a drawing.
 */
void abi_RaceStartSteps(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint16_t w;

    cpu.x = rd8(b | 0x0D5A);
    cpu.p = nz8(cpu.p, (uint8_t)cpu.x);
    switch (cpu.x) {
    case 0:
        if (dec8_mem(b | 0x0E55))
            return;
        substate_next(b);
        lda8_imm(0x07);
        wr8(b | 0x0E55, 0x07);
        return;
    case 2:
        if (dec8_mem(b | 0x0E55)) {
            cpu.x = rd8(b | 0x0E55);
            cpu.p = nz8(cpu.p, (uint8_t)cpu.x);
            op_rep(P_M);
            w = rd16(b | (uint16_t)(0xE33A + cpu.x));
            cpu.a = w;
            op_and16(0x00FF);
            wr16(0x700210, cpu.a);
            abi_GsuBytes200_BFB5BB();
            abi_GsuTimeDigits();
            return;
        }
        substate_next(b);
        lda8_imm(0x14);
        wr8(b | 0x0E55, 0x14);
        lda8_imm(0x04);
        wr8(b | 0x0D54, 0x04);
        dec8_mem(b | 0x0D5B);
        time_digits_full();
        return;
    case 4:
        op_lda8(b | 0x0D54);
        if ((uint8_t)cpu.a && dec8_mem(b | 0x0E55))
            return;
        substate_next(b);
        lda8_imm(0x08);
        wr8(b | 0x0D54, 0x08);
        lda8_imm(0x03);
        wr8(b | 0x0E55, 0x03);
        return;
    case 6:
        if (dec8_mem(b | 0x0E55))
            return;
        wr8(b | 0x0D56, 0x00);
        lda8_imm(0x04);
        phases_both(b);
        return;
    default:
        jump_routine(0x070000 | rd16(0x070000 | (uint16_t)(0xE2EF + cpu.x)));
        return;
    }
}

/*
 * @routine 07:E2E4 RaceStartGlyphsSteps
 * WHAT IT DOES: Draws the nine-glyph word (GlyphWordAt1C28), then
 * RaceStartSteps.
 * HOW: JSR, SEP #$30, then RaceStartSteps' code, which follows.
 * Entry: M 8-bit, X 8-bit. Exit: as RaceStartSteps.
 */
void abi_RaceStartGlyphsSteps(void)
{
    abi_GlyphWordAt1C28();
    op_sep(P_M | P_X);
    abi_RaceStartSteps();
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
void abi_RaceStartTimer(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint8_t v;

    op_lda8(b | 0x0E55);
    if (!(uint8_t)cpu.a) {
        lda8_imm(0x04);
        phases_both(b);
        wr8(b | 0x0D56, 0x00);
        return;
    }
    v = (uint8_t)(cpu.a - 1);
    lda8_imm(v);
    wr8(b | 0x0E55, v);
    cmp8_imm(0x1A);
    if (v == 0x1A)
        return;
    cmp8_imm(0x11);
    if (v == 0x11) {
        lda8_imm(0x02);
        wr8(b | 0x0D54, 0x02);
    }
    cmp8_imm(0x06);
    if ((uint8_t)cpu.a != 0x06)
        return;
    lda8_imm(0x08);
    wr8(b | 0x0D54, 0x08);
}

/* SBC #m (8-bit) on A; 1 when it borrows (C clear). */
static int sbc8_borrows(uint8_t m)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | op_sbc8((uint8_t)cpu.a, m, &cpu.p));
    return !(cpu.p & P_C);
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
void abi_RaceStartTimerSteps(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint8_t v;

    op_lda8(b | 0x0E55);
    v = (uint8_t)(cpu.a - 1);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, v);
    if (!v) {
        wr8(b | 0x0D56, 0x00);
        lda8_imm(0x04);
        phases_both(b);
        return;
    }
    wr8(b | 0x0E55, v);
    cpu.x = rd8(b | 0x0D5A);
    cpu.p = nz8(cpu.p, (uint8_t)cpu.x);
    switch (cpu.x) {
    case 0:
        cpu.p |= P_C;
        if (sbc8_borrows(0x35))
            substate_next(b);
        return;
    case 2:
        cpu.p |= P_C;
        if (sbc8_borrows(0x21)) {
            inc8_mem(b | 0x0D65);
            inc8_mem(b | 0x0D65);
            substate_next(b);
        }
        return;
    case 4:
        cpu.p |= P_C;
        if (sbc8_borrows(0x19)) {
            lda8_imm(0x02);
            wr8(b | 0x0D54, 0x02);
            substate_next(b);
        }
        return;
    case 6:
        cpu.p |= P_C;
        if (sbc8_borrows(0x09))
            substate_next(b);
        return;
    case 8:
        return;
    default:
        jump_routine(0x070000 | rd16(0x070000 | (uint16_t)(0xE3BE + cpu.x)));
        return;
    }
}

/*
 * @routine 07:E557 PanelDelayCount
 * WHAT IT DOES: Counts the panel delay $0D5F down by the frames since the
 * last tick ($0306); C clear once it has run out.
 * HOW: SEC, SBC.
 * Entry: M 8-bit. Exit: A low byte = $0D5F; N, Z, C, V from the
 * subtraction.
 */
void abi_PanelDelayCount(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;

    op_lda8(b | 0x0D5F);
    cpu.p |= P_C;
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | op_sbc8((uint8_t)cpu.a, rd8(b | 0x0306), &cpu.p));
    wr8(b | 0x0D5F, (uint8_t)cpu.a);
}

/* PanelDelayCount; returns 1 when it has run out (BCS not taken). */
static int panel_delay_out(void)
{
    abi_PanelDelayCount();
    return !(cpu.p & P_C);
}

/* The panel's next state, the delay $0D5F = d. */
static void panel_next_delay(uint32_t b, uint8_t d)
{
    panel_next(b);
    lda8_imm(d);
    wr8(b | 0x0D5F, d);
}

/* A result line shown: with the split screen ($0D62) sprite bytes
   $0400 + at + $20 = $20 and $0500 + at + $20 = $88, else $0400 + at =
   $28; then the next state, the delay $32 and sound $49. */
static void panel_line(uint32_t b, uint16_t at)
{
    op_lda8(b | 0x0D62);
    if ((uint8_t)cpu.a) {
        lda8_imm(0x20);
        wr8(b | (uint16_t)(0x0420 + at), 0x20);
        lda8_imm(0x88);
        wr8(b | (uint16_t)(0x0520 + at), 0x88);
    } else {
        lda8_imm(0x28);
        wr8(b | (uint16_t)(0x0400 + at), 0x28);
    }
    panel_next_delay(b, 0x32);
    lda8_imm(0x49);
    abi_SoundPort0QueuePush();
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
void abi_ResultsPanelSteps(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    static const uint16_t marks_split[6] = {0x0427, 0x0527, 0x042B, 0x052B, 0x042F, 0x052F};
    static const uint16_t marks[3] = {0x0407, 0x040B, 0x040F};
    unsigned i;

    cpu.x = rd8(b | 0x0D56);
    cpu.p = nz8(cpu.p, (uint8_t)cpu.x);
    switch (cpu.x) {
    case 0x00:
        lda8_imm(0x3C);
        wr8(b | 0x0D5F, 0x3C);
        panel_next(b);
        lda8_imm(0x08);
        wr8(b | 0x0D54, 0x08);
        return;
    case 0x02:
        if (panel_delay_out())
            panel_next_delay(b, 0x32);
        return;
    case 0x04:
        if (panel_delay_out())
            panel_line(b, 0x06);
        return;
    case 0x06:
        if (panel_delay_out())
            panel_line(b, 0x0A);
        return;
    case 0x08:
        if (panel_delay_out())
            panel_line(b, 0x0E);
        return;
    case 0x0A:
        if (!panel_delay_out())
            return;
        op_lda8(b | 0x0D62);
        if ((uint8_t)cpu.a) {
            lda8_imm(0xA2);
            for (i = 0; i < 6; i++)
                wr8(b | marks_split[i], 0xA2);
        } else {
            lda8_imm(0xA2);
            for (i = 0; i < 3; i++)
                wr8(b | marks[i], 0xA2);
        }
        panel_next_delay(b, 0x32);
        lda8_imm(0x4A);
        abi_SoundPort0QueuePush();
        lda8_imm(0x01);
        wr8(b | 0x0E5B, 0x01);
        wr8(b | 0x0D53, 0x00);
        op_lda8(b | 0x0D2B);
        cmp8_imm(0x01);
        if ((uint8_t)cpu.a != 0x01)
            return;
        lda8_imm(0x02);
        wr8(b | 0x0E41, 0x02);
        lda8_imm(0x0F);
        wr8(b | 0x0E3F, 0x0F);
        return;
    case 0x0C:
        if (!panel_delay_out())
            return;
        panel_next_delay(b, 0x0F);
        lda8_imm(0x01);
        abi_SoundPort0QueuePushRaw();
        return;
    case 0x0E:
        if (!panel_delay_out())
            return;
        lda8_imm(0x0A);
        wr8(b | 0x0D54, 0x0A);
        panel_next(b);
        return;
    case 0x10:
        op_lda8(b | 0x0D54);
        if ((uint8_t)cpu.a)
            return;
        lda8_imm(0x02);
        wr8(b | 0x0D67, 0x02);
        lda8_imm(0x06);
        phases_both(b);
        return;
    default:
        jump_routine(0x070000 | rd16(0x070000 | (uint16_t)(0xE40A + cpu.x)));
        return;
    }
}

void abi_GsuF441Player1(void);
void abi_GsuF441Player2(void);
void abi_OamStagePause(void);
void abi_OamStagePause2(void);
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

/* Player y's byte at `at`: work RAM through the data bank, indexed by Y. */
#define PY(b, at) ((b) | (uint16_t)((at) + cpu.y))

/* The car's sound for a finish: 8, or $15 for car 4 or more. */
static void finish_sound(uint8_t car)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | car);
    cmp8_imm(0x04);
    lda8_imm(car < 0x04 ? 0x08 : 0x15);
    abi_SoundPort0QueuePush();
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
void abi_CarFinishSound(void)
{
    uint8_t p0 = cpu.p;

    op_sep(P_M | P_X);
    op_lda8(((uint32_t)cpu.db << 16) | 0x0D89);
    finish_sound((uint8_t)cpu.a);
    op_plp(p0);
}

/*
 * @routine 07:E937 CarFinishSoundP2
 * WHAT IT DOES: As CarFinishSound for player 2: sets the current player
 * $1A5D = 1, then the sound for its car ($0D8A).
 * HOW: Enters CarFinishSound's compare. Not reached in the traces so far.
 * Entry: any widths. Exit: as CarFinishSound.
 */
void abi_CarFinishSoundP2(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint8_t p0 = cpu.p;

    op_sep(P_M | P_X);
    lda8_imm(0x01);
    wr8(b | 0x1A5D, 0x01);
    op_lda8(b | 0x0D8A);
    finish_sound((uint8_t)cpu.a);
    op_plp(p0);
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
void abi_RaceFinishSounds(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint8_t p0 = cpu.p;

    op_rep(P_M | P_X);
    op_lda16(b | 0x0D2B);
    if (cpu.a == 0x0000) {
        op_sep(P_M);
        op_lda8(b | 0x0DF4);
        wr8(b | 0x0D5E, (uint8_t)cpu.a);
        cmp8_imm(0x01);
        if ((uint8_t)cpu.a == 0x01) {
            lda8_imm(0x02);
            abi_SoundPort0QueuePushRaw();
            abi_CarFinishSound();
            lda8_imm(0x48);
            abi_SoundPort0QueuePush();
        } else {
            lda8_imm(0x03);
            abi_SoundPort0QueuePushRaw();
            abi_CarFinishSound();
        }
        op_plp(p0);
        return;
    }
    op_cmp16(0x0002);
    if (cpu.a == 0x0002) {
        abi_PlayerSound35();
        abi_CarFinishSound();
        op_plp(p0);
        return;
    }
    cpu.x = 0x0006;
    cpu.p = nz16(cpu.p, 0x0006);
    abi_RecordTimeCompare();
    op_sep(P_M);
    op_lda8(b | 0x0FA3);
    cmp8_imm(0x01);
    lda8_imm((uint8_t)cpu.a == 0x01 ? 0x02 : 0x03);
    abi_SoundPort0QueuePushRaw();
    abi_CarFinishSound();
    op_plp(p0);
}

/* Both phases 8, CarFinishSound, sound 4 on port 1: a finish that ends the
   race for both. */
static void finish_both(uint32_t b)
{
    lda8_imm(0x08);
    wr8(b | 0x0DD6, 0x08);
    abi_CarFinishSound();
    lda8_imm(0x04);
    abi_SoundPort1QueuePush();
}

/* A banner's size from the table at $07:E95A (by its timer $0E3F + Y)
   through draw (M 16-bit), then the timer counted down; at 0 the banner
   $0E41 + Y is cleared. */
static void lap_banner(void (*draw)(void))
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint8_t v;

    op_lda8(PY(b, 0x0E3F));
    cpu.x = (uint16_t)(cpu.a & 0x00FF);
    cpu.p = nz8(cpu.p, (uint8_t)cpu.x);
    op_lda8(b | (uint16_t)(0xE95A + cpu.x));
    op_rep(P_M);
    op_and16(0x00FF);
    draw();
    op_sep(P_M | P_X);
    op_lda8(PY(b, 0x0E3F));
    v = (uint8_t)(cpu.a - 1);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, v);
    if (!v) {
        lda8_imm(0x00);
        wr8(PY(b, 0x0E41), 0x00);
    }
    wr8(PY(b, 0x0E3F), (uint8_t)cpu.a);
}

/*
 * @routine 07:E9C0 LapBannerNone
 * WHAT IT DOES: Nothing: banner 2 of RacePhaseLap's banners.
 * HOW: RTS. Not reached in the traces so far.
 * Entry: any widths. Exit: every register unchanged.
 */
void abi_LapBannerNone(void)
{
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
void abi_LapBannerGrow(void)
{
    lap_banner(abi_GsuF0A5OrF0B4);
}

/*
 * @routine 07:E9E1 LapBannerGrowLast
 * WHAT IT DOES: As LapBannerGrow with GsuEEF5OrEFCD (the last lap's
 * banner).
 * HOW: As LapBannerGrow.
 * Entry: as LapBannerGrow. Exit: as LapBannerGrow.
 */
void abi_LapBannerGrowLast(void)
{
    lap_banner(abi_GsuEEF5OrEFCD);
}

/*
 * @routine 07:EA01 LapBannerWait
 * WHAT IT DOES: Counts player Y's banner timer $0E3F + Y down; at 0 clears
 * the banner, sets $0E57 = $FF and plays sound 3 on port 0.
 * HOW: DEC A. Not reached in the traces so far.
 * Entry: M 8-bit, X 8-bit; Y = the player. Exit: A low byte = the timer.
 */
void abi_LapBannerWait(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint8_t v;

    op_lda8(PY(b, 0x0E3F));
    v = (uint8_t)(cpu.a - 1);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, v);
    if (!v) {
        wr8(PY(b, 0x0E41), 0x00);
        lda8_imm(0xFF);
        wr8(b | 0x0E57, 0xFF);
        lda8_imm(0x03);
        abi_SoundPort0QueuePushRaw();
        lda8_imm(0x00);
    }
    wr8(PY(b, 0x0E3F), (uint8_t)cpu.a);
}

/* 07:E608 / 07:E656: a finish without the split screen: the palette
   darkened (PaletteDarkenFour with colour), $0E18 = $F0, $0E5B and $0D5B
   cleared, Countdown0DF8, phase + Y = phase, $0E56 = t, $0E4B = t,
   OamHide75Up, SoundReset and sound $37. */
static void finish_alone(uint32_t b, uint16_t colour, uint8_t phase, uint8_t t)
{
    op_rep(P_M);
    op_lda16_imm(colour);
    abi_PaletteDarkenFour();
    op_sep(P_M);
    lda8_imm(0xF0);
    wr8(b | 0x0E18, 0xF0);
    wr8(b | 0x0E5B, 0x00);
    wr8(b | 0x0D5B, 0x00);
    abi_Countdown0DF8();
    lda8_imm(phase);
    wr8(PY(b, 0x0DD6), phase);
    lda8_imm(t);
    wr8(b | 0x0E56, t);
    wr8(b | 0x0E4B, t);
    abi_OamHide75Up();
    abi_SoundReset();
    lda8_imm(0x37);
    abi_SoundPort0QueuePush();
}

/* 07:E6BD: the lap's time with the override $0D4C: sound $35, $0DEA one
   more, the lap's split (BCD, decimal mode) from the time at $0E63-$0E66
   less the last mark at $0EDB-$0EDD into $0EDE-$0EE0, the mark moved on,
   then the split packed (TimePackBcd) and kept in the course's save entry
   ($70:FC5E + X from 03:DEFD) when it is better, with the level $0F11 and
   the car $0D89 (03:DF10, 07:ADDA); $0FA3 = 1 (kept), $FF (worse) or 0
   (equal); $0E5D = $12C. Not reached in the traces so far. */
static void lap_override(uint32_t b)
{
    uint8_t v, m;
    uint16_t x, t;

    lda8_imm(0x35);
    abi_SoundPort0QueuePush();
    op_lda8(b | 0x0DEA);
    v = (uint8_t)(cpu.a + 1);
    lda8_imm(v);
    wr8(b | 0x0DEA, v);
    op_lda8(b | 0x0E66);
    wr8(0x0000F1, (uint8_t)cpu.a);
    cpu.p |= P_D;
    op_lda8(b | 0x0E63);
    cpu.p |= P_C;
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | op_sbc8((uint8_t)cpu.a, rd8(b | 0x0EDB), &cpu.p));
    wr8(b | 0x0EDE, (uint8_t)cpu.a);
    op_lda8(b | 0x0E64);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | op_sbc8((uint8_t)cpu.a, rd8(b | 0x0EDC), &cpu.p));
    if (!(cpu.p & P_C)) {
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | op_adc8((uint8_t)cpu.a, 0x60, &cpu.p));
        dec8_mem(0x0000F1);
    }
    wr8(b | 0x0EDF, (uint8_t)cpu.a);
    op_lda8(0x0000F1);
    cpu.p |= P_C;
    m = rd8(b | 0x0EDD);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | op_sbc8((uint8_t)cpu.a, m, &cpu.p));
    wr8(b | 0x0EE0, (uint8_t)cpu.a);
    cpu.p &= (uint8_t)~P_D;
    op_lda8(b | 0x0E63);
    wr8(b | 0x0EDB, (uint8_t)cpu.a);
    op_lda8(b | 0x0E64);
    wr8(b | 0x0EDC, (uint8_t)cpu.a);
    op_lda8(b | 0x0E66);
    wr8(b | 0x0EDD, (uint8_t)cpu.a);
    op_rep(P_M | P_X);
    call_routine(0x03DEFD, 1);
    x = cpu.x;                                  /* PHX */
    op_lda16(0x70FC5E + x);
    wr16(b | 0x1163, cpu.a);
    op_rep(P_M);
    op_lda16(b | 0x0EE0);
    op_and16(0x00FF);
    op_tay16();
    op_lda16(b | 0x0EDF);
    op_and16(0x00FF);
    op_tax16();
    op_lda16(b | 0x0EDE);
    op_and16(0x00FF);
    abi_TimePackBcd();
    wr16(b | 0x1161, cpu.a);
    cpu.x = x;                                  /* PLX */
    cpu.p = nz16(cpu.p, x);
    t = rd16(0x70FC5E + cpu.x);
    op_cmp16(t);
    if (cpu.a < t) {
        wr16(0x70FC5E + cpu.x, cpu.a);
        call_routine(0x03DF10, 1);
        cpu.x = x;
        cpu.p = nz16(cpu.p, x);
        op_lda16(b | 0x0F11);
        wr16(0x70FC60 + cpu.x, cpu.a);
        op_sep(P_M);
        op_lda8(b | 0x0D89);
        wr8(0x70FC62 + cpu.x, (uint8_t)cpu.a);
        call_routine(0x07ADDA, 0);
        lda8_imm(0x01);
    } else if (cpu.a != t) {
        op_sep(P_M);
        lda8_imm(0xFF);
    } else {
        op_sep(P_M);
        lda8_imm(0x00);
    }
    wr8(b | 0x0FA3, (uint8_t)cpu.a);
    op_rep(P_M);
    op_lda16_imm(0x012C);
    wr16(b | 0x0E5D, 0x012C);
}

/* 07:E7DC: the last lap done (third count): $0E5B cleared, $0E57 and
   $0E4B counted down; the finish by the split screen, mode, player mode
   and $0D24, the sprites of the finishing panel, LapSplit 6,
   RaceFinishSounds, the phase 8 with the banner timer $14, $0D55
   cleared, $0E58 = $70; OamHideForHudMode, RaceTimeKeep, the time (at most
   99) to $0E6C, and below 8 minutes the time kept as a special course's
   record (TimePackBcd, SpecialRecordInsert, RecordsLoadCourseLong). */
static void lap_finish(uint32_t b)
{
    uint16_t w;

    op_sep(P_M);
    wr8(b | 0x0E5B, 0x00);
    dec8_mem(b | 0x0E57);
    dec8_mem(b | 0x0E4B);
    op_lda8(b | 0x0D62);
    if ((uint8_t)cpu.a) {
        lda8_imm(0xFF);
        wr8(b | 0x0D53, 0xFF);
        call_routine(0x07A99A, 0);
        return;
    }
    op_lda8(b | 0x0D2B);
    if ((uint8_t)cpu.a)
        goto hold;
    op_lda8(b | 0x0DF4);
    cmp8_imm(0x04);
    if ((uint8_t)cpu.a >= 0x04) {
        wr8(b | 0x0D5B, 0x00);
        lda8_imm(0x23);
        wr8(b | 0x0E58, 0x23);
        lda8_imm(0x0A);
        wr8(PY(b, 0x0DD6), 0x0A);
        abi_OamHideForHudMode();
        abi_Countdown0DF8();
        lda8_imm(0x51);
        abi_SoundPort0QueuePush();
        return;
    }
    op_sep(P_M);
    op_lda8(b | 0x0D24);
    cmp8_imm(0x03);
    if ((uint8_t)cpu.a == 0x03) {
        finish_both(b);
        return;
    }
    op_rep(P_M);
    op_lda16(b | 0x0DF4);
    op_and16(0x00FF);
    op_cmp16(0x0002);
    if (cpu.a < 0x0002)
        w = 0x8000;
    else if (cpu.a == 0x0002)
        w = 0x8006;
    else
        w = 0x800C;
    op_lda16_imm(w);
    wr16(b | 0x1010, w);
    op_lda16_imm((uint16_t)(w + 0x0400));
    wr16(b | 0x1012, (uint16_t)(w + 0x0400));
    w = w == 0x8000 ? 0x8012 : 0x8018;
    op_lda16_imm(w);
    wr16(b | 0x1014, w);
    op_lda16_imm(0x0021);
    wr16(b | 0x1040, 0x0021);
    op_lda16_imm(0x001D);
    wr16(b | 0x1042, 0x001D);
    op_lda16_imm(0x0019);
    wr16(b | 0x1044, 0x0019);
    op_lda16_imm(0x0008);
    wr16(b | 0x1070, 0x0008);
    op_lda16_imm(0x001C);
    wr16(b | 0x1072, 0x001C);
    op_lda16_imm(0x002F);
    wr16(b | 0x1074, 0x002F);
    op_lda16_imm(0x00D7);
    wr16(b | 0x0FB0, 0x00D7);
    op_lda16_imm(0x00D3);
    wr16(b | 0x0FB2, 0x00D3);
    op_lda16_imm(0x00CF);
    wr16(b | 0x0FB4, 0x00CF);
    op_sep(P_M);
    op_lda8(b | 0x0D48);
    cmp8_imm(0x03);
    if ((uint8_t)cpu.a == 0x03) {
        abi_GlyphSpritesInit();
        goto panel;
    }
hold:
    op_rep(P_M);
    op_lda16_imm(0xFFFF);
    wr16(b | 0x0E5D, 0xFFFF);
    op_sep(P_M);
    op_lda8(b | 0x10CD);
    if ((uint8_t)cpu.a) {
        finish_both(b);
        return;
    }
panel:
    op_sep(P_M);
    lda8_imm(0x08);
    wr8(PY(b, 0x0E41), 0x08);
    op_sep(P_M | P_X);
    cpu.x = 0x0006;
    cpu.p = nz8(cpu.p, 0x06);
    abi_LapSplit();
    abi_RaceFinishSounds();
    op_sep(P_M);
    lda8_imm(0x14);
    wr8(PY(b, 0x0E3F), 0x14);
    lda8_imm(0x08);
    wr8(PY(b, 0x0DD6), 0x08);
    wr8(b | 0x0D55, 0x00);
    lda8_imm(0x70);
    wr8(b | 0x0E58, 0x70);
    abi_OamHideForHudMode();
    abi_RaceTimeKeep();
    op_rep(P_M | P_X);
    op_lda16(b | 0x0E70);
    op_cmp16(0x0099);
    if (cpu.a >= 0x0099)
        op_lda16_imm(0x0099);
    wr16(b | 0x0E6C, cpu.a);
    op_lda16(b | 0x0E63);
    cpu.x = rd16(b | 0x0E64);
    cpu.p = nz16(cpu.p, cpu.x);
    cpu.y = rd16(b | 0x0E66);
    cpu.p = nz16(cpu.p, cpu.y);
    cpu.p = (uint8_t)((nz16(cpu.p, (uint16_t)(cpu.y - 0x0008)) & ~P_C) | (cpu.y >= 0x0008 ? P_C : 0));
    if (cpu.y >= 0x0008)
        return;
    abi_TimePackBcd();
    abi_SpecialRecordInsert();
    abi_RecordsLoadCourseLong();
}

/* 07:E6B5: a lap crossed: with the override $0D4C the lap's time
   (lap_override); else $0DEA + Y counted: 1 and 2 add the lap's time
   (RaceTimeAddD814 / D815), $0E5D = $12C, the banner $0E41 + Y = 4 or 6
   with timer $17, sound $48, LapSplit 0 or 3 and RecordCompareTime0 / 1;
   3 the finish (lap_finish); a wrap to 0 only sound $48. */
static void lap_crossed(uint32_t b)
{
    uint8_t v;

    op_lda8(b | 0x0D4C);
    if ((uint8_t)cpu.a) {
        lap_override(b);
        return;
    }
    op_lda8(PY(b, 0x0DEA));
    v = (uint8_t)(cpu.a + 1);
    lda8_imm(v);
    wr8(PY(b, 0x0DEA), v);
    cmp8_imm(0x02);
    if (v == 0x02) {
        abi_RaceTimeAddD815();
        op_rep(P_M);
        op_lda16_imm(0x012C);
        wr16(b | 0x0E5D, 0x012C);
        op_sep(P_M);
        lda8_imm(0x48);
        abi_SoundPort0QueuePush();
        lda8_imm(0x06);
        wr8(PY(b, 0x0E41), 0x06);
        lda8_imm(0x17);
        wr8(PY(b, 0x0E3F), 0x17);
        op_sep(P_M | P_X);
        cpu.x = 0x0003;
        cpu.p = nz8(cpu.p, 0x03);
        abi_LapSplit();
        abi_RecordCompareTime1();
        return;
    }
    if (v > 0x02) {
        lap_finish(b);
        return;
    }
    cmp8_imm(0x01);
    if (v != 0x01) {
        lda8_imm(0x48);
        abi_SoundPort0QueuePush();
        return;
    }
    abi_RaceTimeAddD814();
    op_rep(P_M);
    op_lda16_imm(0x012C);
    wr16(b | 0x0E5D, 0x012C);
    op_sep(P_M);
    lda8_imm(0x04);
    wr8(PY(b, 0x0E41), 0x04);
    lda8_imm(0x17);
    wr8(PY(b, 0x0E3F), 0x17);
    lda8_imm(0x48);
    abi_SoundPort0QueuePush();
    op_sep(P_M | P_X);
    cpu.x = 0x0000;
    cpu.p = nz8(cpu.p, 0x00);
    abi_LapSplit();
    abi_RecordCompareTime0();
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
void abi_RaceLapCrossed(void)
{
    lap_crossed((uint32_t)cpu.db << 16);
}

/* 07:E575: the race phase's work after the pause tests. */
static void race_phase(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint16_t w;

    op_lda8(b | 0x0E66);
    cmp8_imm(0x09);
    if ((uint8_t)cpu.a == 0x09) {
        op_lda8(b | 0x0E64);
        cmp8_imm(0x59);
        if ((uint8_t)cpu.a == 0x59) {
            op_lda8(b | 0x0E63);
            cmp8_imm(0x99);
            if ((uint8_t)cpu.a == 0x99)
                goto time_up;
        }
    }
    op_lda8(b | 0x0D2B);
    cmp8_imm(0x02);
    if ((uint8_t)cpu.a == 0x02)
        goto progress;
    cmp8_imm(0x03);
    if ((uint8_t)cpu.a == 0x03)
        goto progress;
    op_rep(P_M);
    op_lda16(b | 0x0E70);
    if (cpu.a)
        goto progress;
    op_sep(P_M);
time_up:
    op_lda8(b | 0x0D62);
    if (!(uint8_t)cpu.a) {
        lda8_imm(0x20);
        wr8(b | 0x0E5A, 0x20);
        lda8_imm(0x0C);
        wr8(PY(b, 0x0DD6), 0x0C);
        lda8_imm(0x31);
        abi_SoundPort0QueuePush();
        return;
    }
    lda8_imm(0x20);
    wr8(b | 0x0E5A, 0x20);
    wr8(b | 0x0E58, 0x20);
    lda8_imm(0x14);
    phases_both(b);
    wr8(b | 0x0D53, 0x00);
    lda8_imm(0x31);
    abi_SoundPort0QueuePush();
    return;
progress:
    op_sep(P_M);
    op_lda8(PY(b, 0x0DEE));
    op_xba();
    op_lda8(PY(b, 0x0DEC));
    op_rep(P_M);
    w = cpu.a;
    cpu.p = (uint8_t)((cpu.p & ~P_Z) | ((w & 0x0001) ? 0 : P_Z));
    if (!(w & 0x0001)) {
        op_sep(P_M | P_X);
        cpu.p = (uint8_t)((nz8(cpu.p, (uint8_t)cpu.y) & ~P_C) | P_C);     /* CPY #0 */
        cpu.x = rd8(b | ((uint8_t)cpu.y ? 0x0E42 : 0x0E41));
        cpu.p = nz8(cpu.p, (uint8_t)cpu.x);
        switch (cpu.x) {
        case 0x00:
        case 0x0A:
            op_sep(P_M);
            op_lda8(PY(b, 0x0DE8));
            {
                uint8_t m = rd8(PY(b, 0x0DEA)), v = (uint8_t)cpu.a;

                cpu.p = (uint8_t)((nz8(cpu.p, (uint8_t)(v - m)) & ~P_C) | (v >= m ? P_C : 0));
                if (v > m)
                    lap_crossed(b);
            }
            return;
        case 0x02:
            abi_LapBannerNone();
            return;
        case 0x04:
            abi_LapBannerGrow();
            return;
        case 0x06:
            abi_LapBannerGrowLast();
            return;
        case 0x08:
            abi_LapBannerWait();
            return;
        default:
            jump_routine(0x070000 | rd16(0x070000 | (uint16_t)(0xE69C + cpu.x)));
            return;
        }
    }
    cpu.p = (uint8_t)((cpu.p & ~P_Z) | ((w & 0x1000) ? 0 : P_Z));
    op_sep(P_M);
    op_lda8(b | 0x0D62);
    if (w & 0x1000) {
        if (!(uint8_t)cpu.a) {
            finish_alone(b, 0xFC00, 0x10, 0x0E);
            return;
        }
        lda8_imm(0x10);
        wr8(PY(b, 0x0DD6), 0x10);
        lda8_imm(0x0C);
        wr8(b | 0x0E56, 0x0C);
        lda8_imm(0x37);
        abi_SoundPort0QueuePush();
        lda8_imm(0xFF);
        wr8(b | 0x0D53, 0xFF);
        abi_RaceFinishPhases10();
        return;
    }
    if (!(uint8_t)cpu.a) {
        finish_alone(b, 0x801F, 0x0E, 0x0C);
        return;
    }
    lda8_imm(0x0E);
    wr8(PY(b, 0x0DD6), 0x0E);
    lda8_imm(0x0C);
    wr8(b | 0x0E56, 0x0C);
    lda8_imm(0xFF);
    wr8(b | 0x0D53, 0xFF);
    abi_RaceFinishPhases0E();
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
void abi_RacePhaseLapP1(void)
{
    abi_GsuF441Player1();
    abi_OamStagePause();
    race_phase();
}

/*
 * @routine 07:E56A RacePhaseLapP2
 * WHAT IT DOES: As RacePhaseLapP1 for player 2 (GsuF441Player2,
 * OamStagePause2).
 * HOW: As RacePhaseLapP1.
 * Entry: as RacePhaseLapP1. Exit: as RacePhaseLapP1.
 */
void abi_RacePhaseLapP2(void)
{
    abi_GsuF441Player2();
    abi_OamStagePause2();
    race_phase();
}

/*
 * @routine 07:E572 RacePhaseLap
 * WHAT IT DOES: As RacePhaseLapP1 for one player: the pause on START
 * (PauseOnStart), then the race.
 * HOW: As RacePhaseLapP1.
 * Entry: as RacePhaseLapP1. Exit: as RacePhaseLapP1.
 */
void abi_RacePhaseLap(void)
{
    abi_PauseOnStart();
    race_phase();
}
