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
