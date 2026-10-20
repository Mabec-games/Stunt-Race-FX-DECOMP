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
    cyc(24);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, v);
}

/* INC / DEC of a work-RAM byte: one read, one write; N, Z from it. */
static uint8_t inc8_mem(uint32_t addr)
{
    cyc(24);
    uint8_t v = (uint8_t)(rd8(addr) + 1);

    cyc(6);
    wr8(addr, v);
    cpu.p = nz8(cpu.p, v);
    return v;
}

static uint8_t dec8_mem(uint32_t addr)
{
    cyc(24);
    uint8_t v = (uint8_t)(rd8(addr) - 1);

    cyc(6);
    wr8(addr, v);
    cpu.p = nz8(cpu.p, v);
    return v;
}

/* CMP # (8-bit). */
static void cmp8_imm(uint8_t m)
{
    uint8_t a = (uint8_t)cpu.a;

    cpu.p = (uint8_t)((nz8(cpu.p, (uint8_t)(a - m)) & ~P_C) | (a >= m ? (cyc(32), P_C) : (cyc(32), 0)));
}

/* Both players' phases ($0DD6, $0DD7) = the low byte of A. */
static void phases_both(uint32_t b)
{
    cyc(16);
    wr8(b | 0x0DD6, (uint8_t)cpu.a);
    cyc(24);
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
    cyc(24);
    op_lda8(((uint32_t)cpu.db << 16) | 0x0DD7);
    cpu.x = (uint16_t)(cpu.a & 0x00FF);
    cpu.p = nz8(cpu.p, (uint8_t)cpu.x);
    cyc(60);
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
    cyc(42);
}

/* 07:E299: both phases 2, $0D5A cleared; then with M 16-bit $0D67, $0D69,
   $19EE, $19F0 and $0E74 cleared and sound 5 on port 0. */
static void race_start(uint32_t b)
{
    lda8_imm(0x02);
    phases_both(b);
    cyc(24);
    wr8(b | 0x0D5A, 0x00);
    op_rep(P_M);
    cyc(24);
    wr16(b | 0x0D67, 0x0000);
    cyc(24);
    wr16(b | 0x0D69, 0x0000);
    cyc(24);
    wr16(b | 0x19EE, 0x0000);
    cyc(24);
    wr16(b | 0x19F0, 0x0000);
    cyc(24);
    wr16(b | 0x0E74, 0x0000);
    op_lda16_imm(0x0005);
    (cyc(2), abi_SoundPort0QueuePushRaw());
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
void abi_RaceStartCountdown(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;

    cyc(24);
    wr8(b | 0x0FAE, 0x00);
    cyc(24);
    wr8(b | 0x0FAF, 0x00);
    lda8_imm(0x20);
    cyc(16);
    wr8(b | 0x0E55, 0x20);
    cyc(22);
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
    cyc(8);
    wr8(b | 0x0E55, 0x12);
    race_start(b);
}

/* Sound 5 on port 0, the timer $0E55 = t, both phases 2, $10CB and $10CC
   cleared. */
static void race_ready(uint8_t t, int bra)
{
    uint32_t b = (uint32_t)cpu.db << 16;

    lda8_imm(0x05);
    abi_SoundPort0QueuePushRaw();
    cyc(8);
    lda8_imm(t);
    wr8(b | 0x0E55, t);
    if (bra)
        cyc(22);                                       /* BRA */
    lda8_imm(0x02);
    phases_both(b);
    cyc(24);
    wr8(b | 0x10CB, 0x00);
    cyc(24);
    wr8(b | 0x10CC, 0x00);
    cyc(42);
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
    race_ready(0x1E, 1);
}

/*
 * @routine 07:E2CA RaceReadyLong
 * WHAT IT DOES: As RaceReadyShort with the timer $0E55 = $36.
 * HOW: As RaceReadyShort.
 * Entry: M 8-bit, X 8-bit. Exit: as RaceReadyShort.
 */
void abi_RaceReadyLong(void)
{
    race_ready(0x36, 0);
}

/* 07:E370: REP #$20, GsuTimeDigitsFull. */
static void time_digits_full(void)
{
    op_rep(P_M);
    abi_GsuTimeDigitsFull();
    cyc(42);
}

/* RaceStartStepsDraw's body: LDX the timer, the size byte from the table at
   $07:E33A read as a word and masked, into $70:0210; the time drawn. */
static void start_draw(uint32_t b)
{
    cyc(24);
    cpu.x = rd8(b | 0x0E55);
    cpu.p = nz8(cpu.p, (uint8_t)cpu.x);
    op_rep(P_M);
    cyc(94);
    cpu.a = rd16(b | (uint16_t)(0xE33A + cpu.x));
    op_and16(0x00FF);
    wr16(0x700210, cpu.a);
    (cyc(62), abi_GsuBytes200_BFB5BB());
    (cyc(46), abi_GsuTimeDigits());
    cyc(42);
}

/* RaceStartSteps' sub-state 4: waits on the timer (at once while $0D54 is
   0), then the next sub-state with $0D54 = 8 and the timer 3. */
static void start_hold(uint32_t b)
{
    cyc(24);
    op_lda8(b | 0x0D54);
    if ((uint8_t)cpu.a && (cyc(16), dec8_mem(b | 0x0E55)))
        { cyc(22); return; } else { cyc(22); }
    substate_next(b);
    lda8_imm(0x08);
    cyc(16);
    wr8(b | 0x0D54, 0x08);
    lda8_imm(0x03);
    cyc(16);
    wr8(b | 0x0E55, 0x03);
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
void abi_RaceStartSteps(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;

    cyc(24);
    cpu.x = rd8(b | 0x0D5A);
    cpu.p = nz8(cpu.p, (uint8_t)cpu.x);
    switch (cpu.x) {
    case 0:
        cyc(46);
        if (dec8_mem(b | 0x0E55))
            { cyc(64); return; } else { cyc(16); }
        substate_next(b);
        lda8_imm(0x07);
        cyc(16);
        wr8(b | 0x0E55, 0x07);
        cyc(42);
        return;
    case 2:
        cyc(46);
        if (dec8_mem(b | 0x0E55)) {
            cyc(22);
            start_draw(b);
            return;
        } else { cyc(16); }
        substate_next(b);
        lda8_imm(0x14);
        cyc(16);
        wr8(b | 0x0E55, 0x14);
        lda8_imm(0x04);
        cyc(16);
        wr8(b | 0x0D54, 0x04);
        dec8_mem(b | 0x0D5B);
        (cyc(70), time_digits_full());
        return;
    case 4:
        cyc(46);
        start_hold(b);
        cyc(42);
        return;
    case 6:
        cyc(46);
        if (dec8_mem(b | 0x0E55))
            { cyc(64); return; }
        cyc(40);
        wr8(b | 0x0D56, 0x00);
        lda8_imm(0x04);
        phases_both(b);
        cyc(42);
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

    cyc(24);
    op_lda8(b | 0x0E55);
    if (!(uint8_t)cpu.a) {
        cyc(16);
        lda8_imm(0x04);
        phases_both(b);
        cyc(24);
        wr8(b | 0x0D56, 0x00);
        cyc(42);
        return;
    }
    v = (uint8_t)(cpu.a - 1);
    lda8_imm(v);
    cyc(36);
    wr8(b | 0x0E55, v);
    cmp8_imm(0x1A);
    if (v == 0x1A)
        { cyc(42); return; } else { cyc(12); }
    cmp8_imm(0x11);
    if (v == 0x11) {
        lda8_imm(0x02);
        cyc(10);
        wr8(b | 0x0D54, 0x02);
    }
    cyc(16);
    cmp8_imm(0x06);
    if ((uint8_t)cpu.a != 0x06)
        { cyc(32); return; }
    lda8_imm(0x08);
    wr8(b | 0x0D54, 0x08);
    cyc(42);
}

/* SBC #m (8-bit) on A; 1 when it borrows (C clear). */
static int sbc8_borrows(uint8_t m)
{
    cyc(76);
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

    cyc(24);
    op_lda8(b | 0x0E55);
    v = (uint8_t)(cpu.a - 1);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, v);
    if (!v) {
        cyc(54);
        wr8(b | 0x0D56, 0x00);
        lda8_imm(0x04);
        phases_both(b);
        cyc(42);
        return;
    }
    cyc(60);
    wr8(b | 0x0E55, v);
    cyc(24);
    cpu.x = rd8(b | 0x0D5A);
    cpu.p = nz8(cpu.p, (uint8_t)cpu.x);
    switch (cpu.x) {
    case 0:
        cpu.p |= P_C;
        if (sbc8_borrows(0x35))
            substate_next(b); else { cyc(6); }
        cyc(42);
        return;
    case 2:
        cpu.p |= P_C;
        if (sbc8_borrows(0x21)) {
            inc8_mem(b | 0x0D65);
            inc8_mem(b | 0x0D65);
            substate_next(b);
        } else { cyc(6); }
        cyc(42);
        return;
    case 4:
        cyc(6);
        cpu.p |= P_C;
        if (sbc8_borrows(0x19)) {
            lda8_imm(0x02);
            cyc(10);
            wr8(b | 0x0D54, 0x02);
            substate_next(b);
        }
        cyc(42);
        return;
    case 6:
        cpu.p |= P_C;
        if (sbc8_borrows(0x09))
            substate_next(b); else { cyc(6); }
        cyc(42);
        return;
    case 8:
        cyc(88);
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

    cyc(24);
    op_lda8(b | 0x0D5F);
    cpu.p |= P_C;
    cyc(38);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | op_sbc8((uint8_t)cpu.a, rd8(b | 0x0306), &cpu.p));
    cyc(8);
    wr8(b | 0x0D5F, (uint8_t)cpu.a);
    cyc(42);
}

/* PanelDelayCount; returns 1 when it has run out (BCS not taken). */
static int panel_delay_out(void)
{
    cyc(92);
    abi_PanelDelayCount();
    return !(cpu.p & P_C);
}

/* The panel's next state, the delay $0D5F = d. */
static void panel_next_delay(uint32_t b, uint8_t d)
{
    panel_next(b);
    lda8_imm(d);
    cyc(16);
    wr8(b | 0x0D5F, d);
}

/* A result line shown: with the split screen ($0D62) sprite bytes
   $0400 + at + $20 = $20 and $0500 + at + $20 = $88, else $0400 + at =
   $28; then the next state, the delay $32 and sound $49. */
static void panel_line(uint32_t b, uint16_t at)
{
    cyc(40);
    op_lda8(b | 0x0D62);
    if ((uint8_t)cpu.a) {
        lda8_imm(0x20);
        cyc(32);
        wr8(b | (uint16_t)(0x0420 + at), 0x20);
        lda8_imm(0x88);
        cyc(16);
        wr8(b | (uint16_t)(0x0520 + at), 0x88);
        cyc(22);                        /* BRA */
    } else {
        lda8_imm(0x28);
        cyc(38);
        wr8(b | (uint16_t)(0x0400 + at), 0x28);
    }
    panel_next_delay(b, 0x32);
    cyc(38);
    lda8_imm(0x49);
    abi_SoundPort0QueuePush();
    cyc(26);
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

    cyc(24);
    cpu.x = rd8(b | 0x0D56);
    cpu.p = nz8(cpu.p, (uint8_t)cpu.x);
    switch (cpu.x) {
    case 0x00:
        lda8_imm(0x3C);
        cyc(62);
        wr8(b | 0x0D5F, 0x3C);
        panel_next(b);
        lda8_imm(0x08);
        cyc(16);
        wr8(b | 0x0D54, 0x08);
        cyc(42);
        return;
    case 0x02:
        if (panel_delay_out())
            { cyc(16); panel_next_delay(b, 0x32); } else { cyc(22); }
        cyc(42);
        return;
    case 0x04:
        if (panel_delay_out())
            panel_line(b, 0x06); else { cyc(64); }
        return;
    case 0x06:
        if (panel_delay_out())
            panel_line(b, 0x0A); else { cyc(64); }
        return;
    case 0x08:
        if (panel_delay_out())
            panel_line(b, 0x0E); else { cyc(64); }
        return;
    case 0x0A:
        if (!panel_delay_out())
            { cyc(64); return; }
        cyc(40);
        op_lda8(b | 0x0D62);
        if ((uint8_t)cpu.a) {
            cyc(8);
            lda8_imm(0xA2);
            for (i = 0; i < 6; i++)
                { cyc(24); wr8(b | marks_split[i], 0xA2); }
        cyc(22);
            } else {
            cyc(14);
            lda8_imm(0xA2);
            for (i = 0; i < 3; i++)
                { cyc(24); wr8(b | marks[i], 0xA2); }
        }
        panel_next_delay(b, 0x32);
        cyc(38);
        lda8_imm(0x4A);
        abi_SoundPort0QueuePush();
        lda8_imm(0x01);
        wr8(b | 0x0E5B, 0x01);
        cyc(24);
        wr8(b | 0x0D53, 0x00);
        cyc(24);
        op_lda8(b | 0x0D2B);
        cmp8_imm(0x01);
        if ((uint8_t)cpu.a != 0x01)
            { cyc(48); return; }
        lda8_imm(0x02);
        cyc(16);
        wr8(b | 0x0E41, 0x02);
        lda8_imm(0x0F);
        cyc(16);
        wr8(b | 0x0E3F, 0x0F);
        cyc(42);
        return;
    case 0x0C:
        if (!panel_delay_out())
            { cyc(64); return; } else { cyc(16); }
        panel_next_delay(b, 0x0F);
        lda8_imm(0x01);
        abi_SoundPort0QueuePushRaw();
        cyc(34);
        return;
    case 0x0E:
        if (!panel_delay_out())
            { cyc(64); return; }
        lda8_imm(0x0A);
        cyc(32);
        wr8(b | 0x0D54, 0x0A);
        panel_next(b);
        cyc(42);
        return;
    case 0x10:
        cyc(70);
        op_lda8(b | 0x0D54);
        cyc(32);
        if ((uint8_t)cpu.a)
            { cyc(32); return; }
        lda8_imm(0x02);
        wr8(b | 0x0D67, 0x02);
        lda8_imm(0x06);
        phases_both(b);
        cyc(42);
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
#define PY(b, at) ((b) | (uint16_t)((at) + cpu.y))

/* The car's sound for a finish: 8, or $15 for car 4 or more. */
static void finish_sound(uint8_t car)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | car);
    cyc(44);
    cmp8_imm(0x04);
    lda8_imm(car < 0x04 ? 0x08 : (cyc(16), 0x15));
    abi_SoundPort0QueuePush();
    cyc(12);
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
    cyc(54);
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
    cyc(38);
    wr8(b | 0x1A5D, 0x01);
    cyc(24);
    op_lda8(b | 0x0D8A);
    cyc(22);
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
    cyc(46);
    op_lda16(b | 0x0D2B);
    if (cpu.a == 0x0000) {
        op_sep(P_M);
        cyc(68);
        op_lda8(b | 0x0DF4);
        cyc(24);
        wr8(b | 0x0D5E, (uint8_t)cpu.a);
        cmp8_imm(0x01);
        if ((uint8_t)cpu.a == 0x01) {
            cyc(22);
            lda8_imm(0x02);
            abi_SoundPort0QueuePushRaw();
            (cyc(46), abi_CarFinishSound());
            cyc(54);
            lda8_imm(0x48);
            abi_SoundPort0QueuePush();
        } else {
            cyc(16);
            lda8_imm(0x03);
            abi_SoundPort0QueuePushRaw();
            (cyc(46), abi_CarFinishSound());
        }
        cyc(30);
        op_plp(p0);
        return;
    } else { cyc(104); }
    op_cmp16(0x0002);
    if (cpu.a == 0x0002) {
        abi_PlayerSound35();
        (cyc(88), abi_CarFinishSound());
        op_plp(p0);
        return;
    } else { cyc(18); }
    cpu.x = 0x0006;
    cpu.p = nz16(cpu.p, 0x0006);
    abi_RecordTimeCompare();
    op_sep(P_M);
    cyc(88);
    op_lda8(b | 0x0FA3);
    cmp8_imm(0x01);
    lda8_imm((uint8_t)cpu.a == 0x01 ? (cyc(16), 0x02) : (cyc(22), 0x03));
    abi_SoundPort0QueuePushRaw();
    abi_CarFinishSound();
    op_plp(p0);
}

/* Both phases 8, CarFinishSound, sound 4 on port 1: a finish that ends the
   race for both. */
static void finish_both(uint32_t b)
{
    lda8_imm(0x08);
    cyc(16);
    wr8(b | 0x0DD6, 0x08);
    (cyc(46), abi_CarFinishSound());
    cyc(50);
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

    cyc(24);
    op_lda8(PY(b, 0x0E3F));
    cpu.x = (uint16_t)(cpu.a & 0x00FF);
    cpu.p = nz8(cpu.p, (uint8_t)cpu.x);
    cyc(114);
    op_lda8(b | (uint16_t)(0xE95A + cpu.x));
    op_rep(P_M);
    op_and16(0x00FF);
    draw();
    op_sep(P_M | P_X);
    cyc(96);
    op_lda8(PY(b, 0x0E3F));
    v = (uint8_t)(cpu.a - 1);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, v);
    if (!v) {
        lda8_imm(0x00);
        cyc(52);
        wr8(PY(b, 0x0E41), 0x00);
    } else { cyc(36); }
    cyc(30);
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
    cyc(42);
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

    cyc(24);
    op_lda8(PY(b, 0x0E3F));
    v = (uint8_t)(cpu.a - 1);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, v);
    if (!v) {
        cyc(60);
        wr8(PY(b, 0x0E41), 0x00);
        lda8_imm(0xFF);
        cyc(16);
        wr8(b | 0x0E57, 0xFF);
        lda8_imm(0x03);
        abi_SoundPort0QueuePushRaw();
        lda8_imm(0x00);
    } else { cyc(60); }
    cyc(6);
    wr8(PY(b, 0x0E3F), (uint8_t)cpu.a);
}

/* 07:E608 / 07:E656: a finish without the split screen: the palette
   darkened (PaletteDarkenFour with colour), $0E18 = $F0, $0E5B and $0D5B
   cleared, Countdown0DF8, phase + Y = phase, $0E56 = t, $0E4B = t,
   OamHide75Up, SoundReset and sound $37. */
static void finish_alone(uint32_t b, uint16_t colour, uint8_t phase, uint8_t t)
{
    cyc(70);
    op_rep(P_M);
    op_lda16_imm(colour);
    abi_PaletteDarkenFour();
    op_sep(P_M);
    lda8_imm(0xF0);
    cyc(38);
    wr8(b | 0x0E18, 0xF0);
    cyc(24);
    wr8(b | 0x0E5B, 0x00);
    cyc(24);
    wr8(b | 0x0D5B, 0x00);
    (cyc(62), abi_Countdown0DF8());
    lda8_imm(phase);
    cyc(22);
    wr8(PY(b, 0x0DD6), phase);
    lda8_imm(t);
    cyc(16);
    wr8(b | 0x0E56, t);
    cyc(24);
    wr8(b | 0x0E4B, t);
    abi_OamHide75Up();
    (cyc(62), abi_SoundReset());
    cyc(38);
    lda8_imm(0x37);
    abi_SoundPort0QueuePush();
}

/* 07:E6BD: the lap's time with the override $0D4C: sound $35, $0DEA one
   more, the lap's split (BCD, decimal mode) from the time at $0E63-$0E66
   less the last mark at $0EDB-$0EDD into $0EDE-$0EE0, the mark moved on,
   then the split packed (TimePackBcd) and kept in the course's save entry
   ($70:FC5E + X from RecordOffsetAllCourses) when it is better, with the
   level $0F11 and the car $0D89 (HudBestTimeDigits, SaveSumsRecordsB); $0FA3 = 1
   (kept), $FF (worse) or 0 (equal); $0E5D = $12C. Reached only by
   unit_race_phases. */
static void lap_override(uint32_t b)
{
    uint8_t v, m;
    uint16_t x, t;

    lda8_imm(0x35);
    abi_SoundPort0QueuePush();
    cyc(8);
    op_lda8(b | 0x0DEA);
    v = (uint8_t)(cpu.a + 1);
    lda8_imm(v);
    cyc(14);
    wr8(b | 0x0DEA, v);
    cyc(24);
    op_lda8(b | 0x0E66);
    cyc(16);
    wr8(0x0000F1, (uint8_t)cpu.a);
    cpu.p |= P_D;
    cyc(38);
    op_lda8(b | 0x0E63);
    cpu.p |= P_C;
    cyc(38);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | op_sbc8((uint8_t)cpu.a, rd8(b | 0x0EDB), &cpu.p));
    cyc(8);
    wr8(b | 0x0EDE, (uint8_t)cpu.a);
    cyc(24);
    op_lda8(b | 0x0E64);
    cyc(24);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | op_sbc8((uint8_t)cpu.a, rd8(b | 0x0EDC), &cpu.p));
    if (!(cpu.p & P_C)) {
        cyc(22);
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | op_adc8((uint8_t)cpu.a, 0x60, &cpu.p));
        dec8_mem(0x0000F1);
    } else { cyc(6); }
    cyc(24);
    wr8(b | 0x0EDF, (uint8_t)cpu.a);
    cyc(16);
    op_lda8(0x0000F1);
    cpu.p |= P_C;
    cyc(38);
    m = rd8(b | 0x0EDD);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | op_sbc8((uint8_t)cpu.a, m, &cpu.p));
    cyc(8);
    wr8(b | 0x0EE0, (uint8_t)cpu.a);
    cpu.p &= (uint8_t)~P_D;
    cyc(38);
    op_lda8(b | 0x0E63);
    cyc(24);
    wr8(b | 0x0EDB, (uint8_t)cpu.a);
    cyc(24);
    op_lda8(b | 0x0E64);
    cyc(24);
    wr8(b | 0x0EDC, (uint8_t)cpu.a);
    cyc(24);
    op_lda8(b | 0x0E66);
    cyc(24);
    wr8(b | 0x0EDD, (uint8_t)cpu.a);
    op_rep(P_M | P_X);
    call_routine(0x03DEFD, 1);
    x = cpu.x;                                  /* PHX */
    cyc(112);
    op_lda16(0x70FC5E + x);
    cyc(24);
    wr16(b | 0x1163, cpu.a);
    op_rep(P_M);
    cyc(24);
    op_lda16(b | 0x0EE0);
    op_and16(0x00FF);
    op_tay16();
    cyc(60);
    op_lda16(b | 0x0EDF);
    op_and16(0x00FF);
    op_tax16();
    cyc(30);
    op_lda16(b | 0x0EDE);
    op_and16(0x00FF);
    (cyc(22), abi_TimePackBcd());
    cyc(66);
    wr16(b | 0x1161, cpu.a);
    cpu.x = x;                                  /* PLX */
    cpu.p = nz16(cpu.p, x);
    cyc(68);
    t = rd16(0x70FC5E + cpu.x);
    op_cmp16(t);
    if (cpu.a < t) {
        cyc(44);
        wr16(0x70FC5E + cpu.x, cpu.a);
        call_routine(0x03DF10, 1);
        cpu.x = x;
        cpu.p = nz16(cpu.p, x);
        cyc(88);
        op_lda16(b | 0x0F11);
        cyc(32);
        wr16(0x70FC60 + cpu.x, cpu.a);
        op_sep(P_M);
        cyc(46);
        op_lda8(b | 0x0D89);
        cyc(32);
        wr8(0x70FC62 + cpu.x, (uint8_t)cpu.a);
        abi_SaveSumsRecordsB();
        lda8_imm(0x01);
    } else if (cpu.a != t) {
        cyc(16);
        op_sep(P_M);
        lda8_imm(0xFF);
    } else {
        op_sep(P_M);
        lda8_imm(0x00);
    }
    cyc(78);
    wr8(b | 0x0FA3, (uint8_t)cpu.a);
    op_rep(P_M);
    op_lda16_imm(0x012C);
    cyc(10);
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
    cyc(52);
    wr8(b | 0x0E5B, 0x00);
    dec8_mem(b | 0x0E57);
    dec8_mem(b | 0x0E4B);
    cyc(24);
    op_lda8(b | 0x0D62);
    if ((uint8_t)cpu.a) {
        lda8_imm(0xFF);
        cyc(32);
        wr8(b | 0x0D53, 0xFF);
        (cyc(46), abi_RaceFinishSplit());
        return;
    }
    cyc(46);
    op_lda8(b | 0x0D2B);
    if ((uint8_t)cpu.a)
        { cyc(40); goto hold; }
    cyc(46);
    op_lda8(b | 0x0DF4);
    cmp8_imm(0x04);
    if ((uint8_t)cpu.a >= 0x04) {
        cyc(24);
        wr8(b | 0x0D5B, 0x00);
        lda8_imm(0x23);
        cyc(16);
        wr8(b | 0x0E58, 0x23);
        lda8_imm(0x0A);
        cyc(22);
        wr8(PY(b, 0x0DD6), 0x0A);
        (cyc(46), abi_OamHideForHudMode());
        (cyc(16), abi_Countdown0DF8());
        cyc(108);
        lda8_imm(0x51);
        abi_SoundPort0QueuePush();
        return;
    }
    op_sep(P_M);
    cyc(52);
    op_lda8(b | 0x0D24);
    cmp8_imm(0x03);
    if ((uint8_t)cpu.a == 0x03) {
        finish_both(b);
        return;
    }
    op_rep(P_M);
    cyc(30);
    op_lda16(b | 0x0DF4);
    op_and16(0x00FF);
    op_cmp16(0x0002);
    if (cpu.a < 0x0002)
        { cyc(4); w = 0x8000; }
    else if (cpu.a == 0x0002)
        { cyc(16); w = 0x8006; }
    else
        { cyc(10); w = 0x800C; }
    op_lda16_imm(w);
    cyc(74);
    wr16(b | 0x1010, w);
    op_lda16_imm((uint16_t)(w + 0x0400));
    cyc(10);
    wr16(b | 0x1012, (uint16_t)(w + 0x0400));
    w = w == 0x8000 ? 0x8012 : 0x8018;
    op_lda16_imm(w);
    cyc(10);
    wr16(b | 0x1014, w);
    op_lda16_imm(0x0021);
    cyc(10);
    wr16(b | 0x1040, 0x0021);
    op_lda16_imm(0x001D);
    cyc(10);
    wr16(b | 0x1042, 0x001D);
    op_lda16_imm(0x0019);
    cyc(10);
    wr16(b | 0x1044, 0x0019);
    op_lda16_imm(0x0008);
    cyc(10);
    wr16(b | 0x1070, 0x0008);
    op_lda16_imm(0x001C);
    cyc(10);
    wr16(b | 0x1072, 0x001C);
    op_lda16_imm(0x002F);
    cyc(10);
    wr16(b | 0x1074, 0x002F);
    op_lda16_imm(0x00D7);
    cyc(10);
    wr16(b | 0x0FB0, 0x00D7);
    op_lda16_imm(0x00D3);
    cyc(10);
    wr16(b | 0x0FB2, 0x00D3);
    op_lda16_imm(0x00CF);
    cyc(10);
    wr16(b | 0x0FB4, 0x00CF);
    op_sep(P_M);
    cyc(46);
    op_lda8(b | 0x0D48);
    cmp8_imm(0x03);
    if ((uint8_t)cpu.a == 0x03) {
        cyc(62);
        abi_GlyphSpritesInit();
        goto panel;
    }
hold:
    op_rep(P_M);
    op_lda16_imm(0xFFFF);
    cyc(16);
    wr16(b | 0x0E5D, 0xFFFF);
    op_sep(P_M);
    cyc(46);
    op_lda8(b | 0x10CD);
    if ((uint8_t)cpu.a) {
        cyc(16);
        finish_both(b);
        return;
    }
panel:
    op_sep(P_M);
    lda8_imm(0x08);
    cyc(66);
    wr8(PY(b, 0x0E41), 0x08);
    op_sep(P_M | P_X);
    cpu.x = 0x0006;
    cpu.p = nz8(cpu.p, 0x06);
    (cyc(62), abi_LapSplit());
    (cyc(62), abi_RaceFinishSounds());
    op_sep(P_M);
    lda8_imm(0x14);
    cyc(44);
    wr8(PY(b, 0x0E3F), 0x14);
    lda8_imm(0x08);
    cyc(22);
    wr8(PY(b, 0x0DD6), 0x08);
    cyc(24);
    wr8(b | 0x0D55, 0x00);
    lda8_imm(0x70);
    cyc(16);
    wr8(b | 0x0E58, 0x70);
    (cyc(46), abi_OamHideForHudMode());
    (cyc(46), abi_RaceTimeKeep());
    op_rep(P_M | P_X);
    cyc(24);
    op_lda16(b | 0x0E70);
    op_cmp16(0x0099);
    if (cpu.a >= 0x0099)
        op_lda16_imm(0x0099); else { cyc(20); }
    cyc(50);
    wr16(b | 0x0E6C, cpu.a);
    cyc(24);
    op_lda16(b | 0x0E63);
    cyc(24);
    cpu.x = rd16(b | 0x0E64);
    cpu.p = nz16(cpu.p, cpu.x);
    cyc(24);
    cpu.y = rd16(b | 0x0E66);
    cpu.p = nz16(cpu.p, cpu.y);
    cpu.p = (uint8_t)((nz16(cpu.p, (uint16_t)(cpu.y - 0x0008)) & ~P_C) | (cpu.y >= 0x0008 ? (cyc(46), P_C) : (cyc(86), 0)));
    if (cpu.y >= 0x0008)
        return;
    abi_TimePackBcd();
    (cyc(46), abi_SpecialRecordInsert());
    (cyc(62), abi_RecordsLoadCourseLong());
    cyc(160);
}

/* 07:E6B5: a lap crossed: with the override $0D4C the lap's time
   (lap_override); else $0DEA + Y counted: 1 and 2 add the lap's time
   (RaceTimeAddD814 / D815), $0E5D = $12C, the banner $0E41 + Y = 4 or 6
   with timer $17, sound $48, LapSplit 0 or 3 and RecordCompareTime0 / 1;
   3 the finish (lap_finish); a wrap to 0 only sound $48. */
static void lap_crossed(uint32_t b)
{
    uint8_t v;

    cyc(24);
    op_lda8(b | 0x0D4C);
    if ((uint8_t)cpu.a) {
        cyc(60);
        lap_override(b);
        return;
    }
    cyc(70);
    op_lda8(PY(b, 0x0DEA));
    v = (uint8_t)(cpu.a + 1);
    lda8_imm(v);
    cyc(20);
    wr8(PY(b, 0x0DEA), v);
    cmp8_imm(0x02);
    if (v == 0x02) {
        cyc(68);
        abi_RaceTimeAddD815();
        op_rep(P_M);
        op_lda16_imm(0x012C);
        cyc(10);
        wr16(b | 0x0E5D, 0x012C);
        op_sep(P_M);
        cyc(60);
        lda8_imm(0x48);
        abi_SoundPort0QueuePush();
        lda8_imm(0x06);
        cyc(6);
        wr8(PY(b, 0x0E41), 0x06);
        lda8_imm(0x17);
        cyc(22);
        wr8(PY(b, 0x0E3F), 0x17);
        op_sep(P_M | P_X);
        cpu.x = 0x0003;
        cpu.p = nz8(cpu.p, 0x03);
        (cyc(62), abi_LapSplit());
        (cyc(62), abi_RecordCompareTime1());
        cyc(16);
        return;
    } else { cyc(16); }
    if (v > 0x02) {
        lap_finish(b);
        return;
    } else { cyc(68); }
    cmp8_imm(0x01);
    if (v != 0x01) {
        cyc(38);
        lda8_imm(0x48);
        abi_SoundPort0QueuePush();
        return;
    }
    abi_RaceTimeAddD814();
    op_rep(P_M);
    op_lda16_imm(0x012C);
    cyc(10);
    wr16(b | 0x0E5D, 0x012C);
    op_sep(P_M);
    lda8_imm(0x04);
    cyc(44);
    wr8(PY(b, 0x0E41), 0x04);
    lda8_imm(0x17);
    cyc(22);
    wr8(PY(b, 0x0E3F), 0x17);
    cyc(38);
    lda8_imm(0x48);
    abi_SoundPort0QueuePush();
    op_sep(P_M | P_X);
    cpu.x = 0x0000;
    cpu.p = nz8(cpu.p, 0x00);
    (cyc(46), abi_LapSplit());
    (cyc(62), abi_RecordCompareTime0());
    cyc(16);
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

    cyc(24);
    op_lda8(b | 0x0E66);
    cmp8_imm(0x09);
    if ((uint8_t)cpu.a == 0x09) {
        cyc(24);
        op_lda8(b | 0x0E64);
        cmp8_imm(0x59);
        if ((uint8_t)cpu.a == 0x59) {
            cyc(24);
            op_lda8(b | 0x0E63);
            cmp8_imm(0x99);
            if ((uint8_t)cpu.a == 0x99)
                goto time_up;
        } else { cyc(6); }
    } else { cyc(6); }
    cyc(24);
    op_lda8(b | 0x0D2B);
    cmp8_imm(0x02);
    if ((uint8_t)cpu.a == 0x02)
        goto progress;
    cmp8_imm(0x03);
    if ((uint8_t)cpu.a == 0x03)
        goto progress;
    op_rep(P_M);
    cyc(24);
    op_lda16(b | 0x0E70);
    if (cpu.a)
        { cyc(16); goto progress; } else { cyc(32); }
    op_sep(P_M);
time_up:
    cyc(30);
    op_lda8(b | 0x0D62);
    if (!(uint8_t)cpu.a) {
        lda8_imm(0x20);
        cyc(32);
        wr8(b | 0x0E5A, 0x20);
        lda8_imm(0x0C);
        cyc(22);
        wr8(PY(b, 0x0DD6), 0x0C);
        cyc(38);
        lda8_imm(0x31);
        abi_SoundPort0QueuePush();
        return;
    }
    lda8_imm(0x20);
    cyc(38);
    wr8(b | 0x0E5A, 0x20);
    cyc(24);
    wr8(b | 0x0E58, 0x20);
    lda8_imm(0x14);
    phases_both(b);
    cyc(24);
    wr8(b | 0x0D53, 0x00);
    cyc(38);
    lda8_imm(0x31);
    abi_SoundPort0QueuePush();
    return;
progress:
    op_sep(P_M);
    cyc(52);
    op_lda8(PY(b, 0x0DEE));
    op_xba();
    cyc(44);
    op_lda8(PY(b, 0x0DEC));
    op_rep(P_M);
    w = cpu.a;
    cpu.p = (uint8_t)((cpu.p & ~P_Z) | ((w & 0x0001) ? 0 : P_Z));
    if (!(w & 0x0001)) {
        op_sep(P_M | P_X);
        cpu.p = (uint8_t)((nz8(cpu.p, (uint8_t)cpu.y) & ~P_C) | P_C);     /* CPY #0 */
        cyc(126);
        cpu.x = rd8(b | ((uint8_t)cpu.y ? 0x0E42 : (cyc(6), 0x0E41)));
        cpu.p = nz8(cpu.p, (uint8_t)cpu.x);
        switch (cpu.x) {
        case 0x00:
        case 0x0A:
            op_sep(P_M);
            cyc(92);
            op_lda8(PY(b, 0x0DE8));
            {
                cyc(24);
                uint8_t m = rd8(PY(b, 0x0DEA)), v = (uint8_t)cpu.a;

                cpu.p = (uint8_t)((nz8(cpu.p, (uint8_t)(v - m)) & ~P_C) | (v >= m ? (cyc(38), P_C) : 0));
                if (v > m)
                    lap_crossed(b);
            }
            return;
        case 0x02:
            cyc(20);
            abi_LapBannerNone();
            return;
        case 0x04:
            cyc(46);
            abi_LapBannerGrow();
            cyc(16);
            return;
        case 0x06:
            cyc(46);
            abi_LapBannerGrowLast();
            cyc(16);
            return;
        case 0x08:
            cyc(46);
            abi_LapBannerWait();
            cyc(16);
            return;
        default:
            jump_routine(0x070000 | rd16(0x070000 | (uint16_t)(0xE69C + cpu.x)));
            return;
        }
    }
    cpu.p = (uint8_t)((cpu.p & ~P_Z) | ((w & 0x1000) ? 0 : (cyc(16), P_Z)));
    op_sep(P_M);
    cyc(132);
    op_lda8(b | 0x0D62);
    if (w & 0x1000) {
        if (!(uint8_t)cpu.a) {
            finish_alone(b, 0xFC00, 0x10, 0x0E);
            return;
        }
        lda8_imm(0x10);
        cyc(38);
        wr8(PY(b, 0x0DD6), 0x10);
        lda8_imm(0x0C);
        cyc(16);
        wr8(b | 0x0E56, 0x0C);
        cyc(38);
        lda8_imm(0x37);
        abi_SoundPort0QueuePush();
        lda8_imm(0xFF);
        wr8(b | 0x0D53, 0xFF);
        (cyc(46), abi_RaceFinishPhases10());
        cyc(8);
        return;
    }
    if (!(uint8_t)cpu.a) {
        finish_alone(b, 0x801F, 0x0E, 0x0C);
        return;
    }
    lda8_imm(0x0E);
    cyc(38);
    wr8(PY(b, 0x0DD6), 0x0E);
    lda8_imm(0x0C);
    cyc(16);
    wr8(b | 0x0E56, 0x0C);
    lda8_imm(0xFF);
    cyc(16);
    wr8(b | 0x0D53, 0xFF);
    (cyc(30), abi_RaceFinishPhases0E());
    cyc(8);
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
    cyc(46);
    abi_GsuF441Player1();
    (cyc(118), abi_OamStagePause());
    (cyc(22), race_phase());
    cyc(26);
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
    cyc(46);
    abi_GsuF441Player2();
    (cyc(118), abi_OamStagePause2());
    (cyc(22), race_phase());
    cyc(26);
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
    cyc(46);
    abi_PauseOnStart();
    race_phase();
    cyc(26);
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
void abi_PhaseGlyphsFive(void)
{
    abi_GsuF15BFive();
}

/*
 * @routine 07:E976 PhaseGlyphsSix
 * WHAT IT DOES: Draws six HUD glyphs (HudSixGlyphs): a phase's whole work.
 * HOW: JSR, RTS.
 * Entry: D = $0000. Exit: as HudSixGlyphs leaves it.
 */
void abi_PhaseGlyphsSix(void)
{
    cyc(46);
    abi_HudSixGlyphs();
    cyc(42);
}

/* Once the slide out at $7F:3468 has reached $1F: $0E5F cleared, $0D22 =
   the slide, $0D23 = 6, $0E56 = $78, $0D55 cleared and the race's end
   (RaceEndCountdownFade); before, draw() for the frame. */
static void race_end_wait(void (*draw)(void))
{
    uint32_t b = (uint32_t)cpu.db << 16;

    cyc(32);
    op_lda8(0x7F3468);
    cmp8_imm(0x1F);
    if ((uint8_t)cpu.a < 0x1F) {
        cyc(52);
        draw();
        return;
    }
    cyc(24);
    wr8(b | 0x0E5F, 0x00);
    cyc(24);
    wr8(b | 0x0D22, (uint8_t)cpu.a);
    lda8_imm(0x06);
    cyc(16);
    wr8(b | 0x0D23, 0x06);
    lda8_imm(0x78);
    cyc(16);
    wr8(b | 0x0E56, 0x78);
    cyc(24);
    wr8(b | 0x0D55, 0x00);
    (cyc(24), abi_RaceEndCountdownFade());
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
void abi_RaceEndWaitRow6B(void)
{
    race_end_wait(abi_BigSpritesRow6B);
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
void abi_RaceEndWaitBigSprites(void)
{
    race_end_wait(abi_BigSpritesRow5);
}

/* $0D22 = 1, $0D23 cleared and the window slid up with its sound. */
static void window_up(uint32_t b)
{
    lda8_imm(0x01);
    cyc(16);
    wr8(b | 0x0D22, 0x01);
    cyc(24);
    wr8(b | 0x0D23, 0x00);
    (cyc(62), abi_WindowSlideUpSound());
}

/* The B / START test of the presses $0D1A (bits 15 and 12), 16-bit. */
static int pressed_b_start(uint32_t b)
{
    op_rep(P_M);
    cyc(40);
    op_lda16(b | 0x0D1A);
    cpu.p = (uint8_t)((cpu.p & ~P_Z) | ((cpu.a & 0x9000) ? 0 : (cyc(24), P_Z)));
    return (cpu.a & 0x9000) != 0;
}

void abi_RaceRestart(void);
void abi_RaceEndToAttract(void);
void abi_RaceEndVideoReset(void);
void abi_SoftResetToBootWarm(void);

/* 07:EA6A / EADD: once the split window is in state 5 ($19D8), $0D59
   cleared, OverlayFramesUntilFaded, then on. */
static int window_closed_overlay(uint32_t b)
{
    cyc(30);
    op_lda8(b | 0x19D8);
    cmp8_imm(0x05);
    if ((uint8_t)cpu.a != 0x05)
        { cyc(6); return 0; }
    cyc(30);
    wr8(b | 0x0D59, 0x00);
    (cyc(46), abi_OverlayFramesUntilFaded());
    op_sep(P_M);
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
void abi_ResultsWindowWait(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint8_t v;

    cyc(24);
    op_lda8(b | 0x10CD);
    if ((uint8_t)cpu.a) {
        cyc(40);
        op_lda8(b | 0x0D61);
        if ((uint8_t)cpu.a & 0x80)
            { cyc(40); abi_SoftResetToBootWarm(); } else { cyc(22); }
        return;
    }
    cyc(46);
    op_lda8(b | 0x0E58);
    v = (uint8_t)cpu.a;
    if (!v) {
        cyc(16);
        if (!window_closed_overlay(b))
            return;
        cyc(104);
        op_lda8(b | 0x0DD4);
        if ((uint8_t)cpu.a)
            { cyc(40); abi_RaceRestart(); }
        else
            { cyc(46); abi_RaceEndVideoReset(); }
        return;
    } else { cyc(6); }
    cmp8_imm(0x10);
    if (v == 0x10) {
        cyc(10);
        window_up(b);
    } else {
        cmp8_imm(0x58);
        if (v < 0x58 && pressed_b_start(b)) {
            op_sep(P_M);
            cyc(86);
            op_lda8(b | 0x19D8);
            if ((uint8_t)cpu.a)
                { cyc(22); return; }
            cyc(40);
            wr8(b | 0x0E58, 0x00);
            window_up(b);
            return;
        }
    }
    cyc(44);
    op_sep(P_M);
    dec8_mem(b | 0x0E58);
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
void abi_RaceEndWindowSteps(void)
{
    uint32_t b;
    uint8_t v;

    op_sep(P_M | P_X);
    b = (uint32_t)cpu.db << 16;
    cyc(24);
    op_lda8(b | 0x0D48);
    cmp8_imm(0x03);
    if ((uint8_t)cpu.a == 0x03) {
        cyc(62);
        abi_GlyphSpritesGrowAndTimes();
        return;
    }
    if (window_closed_overlay(b)) {
        op_lda8(b | 0x0D24);
        cmp8_imm(0x03);
        if ((uint8_t)cpu.a != 0x03) {
            op_lda8(b | 0x0D48);
            cmp8_imm(0x01);
            if ((uint8_t)cpu.a == 0x01) {
                lda8_imm(0xFF);
                wr8(b | 0x0D4C, 0xFF);
                op_lda8(b | 0x0D4A);
                cpu.p &= (uint8_t)~P_C;
                cpu.a = (uint16_t)((cpu.a & 0xFF00) | op_adc8((uint8_t)cpu.a, 0x14, &cpu.p));
                wr8(b | 0x0E3D, (uint8_t)cpu.a);
                lda8_imm(0x04);
                wr8(b | 0x0D89, 0x04);
            } else {
                inc8_mem(b | 0x0D48);
            }
        }
        abi_RaceRestart();
        return;
    }
    cyc(18);
    op_lda8(b | 0x0E58);
    v = (uint8_t)cpu.a;
    cmp8_imm(0x10);
    if (v == 0x10) {
        cyc(24);
        wr8(b | 0x19DF, 0x00);
        window_up(b);
        cyc(24);
        op_lda8(b | 0x0E58);
        v = (uint8_t)cpu.a;
    } else { cyc(6); }
    cyc(16);
    cmp8_imm(0x58);
    if (v == 0x58) {
        lda8_imm(0x0A);
        wr8(b | 0x0D65, 0x0A);
        cyc(24);
        op_lda8(b | 0x0E58);
        v = (uint8_t)cpu.a;
        (cyc(90), abi_GlyphSlideThree());
        lda8_imm(v);                            /* PLA */
    } else if (v < 0x58) {
        cyc(74);
        abi_GlyphSlideThree();
        lda8_imm(v);                            /* PLA */
    } else { cyc(8); }
    cyc(10);
    cmp8_imm(0x2B);
    if (v == 0x2B) {
        cyc(18);
        op_lda8(b | 0x0D65);
        if ((uint8_t)cpu.a) {
            lda8_imm(0x10);
            cyc(32);
            wr8(b | 0x0D65, 0x10);
            cyc(24);
            op_lda8(b | 0x0E58);
        }
    }
    dec8_mem(b | 0x0E58);
    cyc(42);
}

/* $0D2F = $0D2D = $A: the pause's stage loop from its start. */
static void pause_stage_ten(uint32_t b)
{
    lda8_imm(0x0A);
    cyc(16);
    wr8(b | 0x0D2F, 0x0A);
    cyc(24);
    wr8(b | 0x0D2D, 0x0A);
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
void abi_PauseResultsP2(void)
{
    cyc(46);
    uint32_t b = (uint32_t)cpu.db << 16;

    abi_GsuF15BThree();
    op_sep(P_M);
    cyc(118);
    op_lda8(b | 0x0E58);
    if ((uint8_t)cpu.a) {
        cyc(16);
        if (dec8_mem(b | 0x0E58))
            { cyc(64); return; }
        lda8_imm(0x01);
        cyc(32);
        wr8(b | 0x0D2E, 0x01);
        (cyc(62), abi_WindowSlideUpSound());
        lda8_imm(0x08);
        cyc(16);
        wr8(b | 0x0D2F, 0x08);
        cyc(24);
        wr8(b | 0x0D5F, 0x08);
        cyc(42);
        return;
    }
    cyc(46);
    op_lda8(b | 0x0D5F);
    if ((uint8_t)cpu.a) {
        cyc(16);
        dec8_mem(b | 0x0D5F);
        cyc(42);
        return;
    } else { cyc(22); }
    pause_stage_ten(b);
    (cyc(24), abi_OamStagePause2Loop());
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
void abi_PhaseGlyphsFourB(void)
{
    abi_GsuF15BFourB();
}

/* 07:EB51: the paused results: counts $0E58 down, at its end sound $51,
   $0D2E cleared, the window slid up, $0D2F = 8, the delay $0D5F = $14;
   then counts the delay down; at its end the time-up sound $30 at 9:59,
   $0D2F = $0D2D = $A and pad 1's pause loop (OamStagePauseLoop, 07:BB16). */
static void pause_results(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;

    op_sep(P_M);
    cyc(52);
    op_lda8(b | 0x0E58);
    if ((uint8_t)cpu.a) {
        cyc(16);
        if (dec8_mem(b | 0x0E58))
            { cyc(38); return; } else { cyc(70); }
        lda8_imm(0x51);
        abi_SoundPort0QueuePush();
        lda8_imm(0x00);
        cyc(16);
        wr8(b | 0x0D2E, 0x00);
        (cyc(62), abi_WindowSlideUpSound());
        lda8_imm(0x08);
        cyc(16);
        wr8(b | 0x0D2F, 0x08);
        lda8_imm(0x14);
        cyc(16);
        wr8(b | 0x0D5F, 0x14);
        cyc(16);
        return;
    }
    cyc(46);
    op_lda8(b | 0x0D5F);
    if ((uint8_t)cpu.a) {
        cyc(16);
        dec8_mem(b | 0x0D5F);
        cyc(16);
        return;
    }
    cyc(46);
    op_lda8(b | 0x0E66);
    cmp8_imm(0x09);
    if ((uint8_t)cpu.a == 0x09) {
        cyc(24);
        op_lda8(b | 0x0E64);
        cmp8_imm(0x59);
        if ((uint8_t)cpu.a == 0x59) {
            cyc(16);
            lda8_imm(0x30);
            abi_SoundPort0QueuePushRaw();
        } else { cyc(6); }
    } else { cyc(6); }
    pause_stage_ten(b);
    (cyc(32), abi_OamStagePauseLoop());
}

/*
 * @routine 07:EB49 PauseResultsFourB
 * WHAT IT DOES: The paused results (see PauseResults), drawing with
 * GsuF15BFourB.
 * HOW: JSR, then PauseResults' code. Reached only by unit_race_phases and unit_entries_07.
 * Entry: D = $0000. Exit: as the path leaves it.
 */
void abi_PauseResultsFourB(void)
{
    abi_GsuF15BFourB();
    pause_results();
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
void abi_PauseResults(void)
{
    cyc(46);
    abi_GsuF15BThree();
    (cyc(66), pause_results());
    cyc(26);
}

/*
 * @routine 07:EBA0 PhaseGlyphsFourA
 * WHAT IT DOES: Draws with GsuF15BFourA: a phase's whole work.
 * HOW: JSR, RTS. Reached only by unit_race_phases and unit_entries_07.
 * Entry: D = $0000. Exit: as GsuF15BFourA leaves it.
 */
void abi_PhaseGlyphsFourA(void)
{
    abi_GsuF15BFourA();
}

/*
 * @routine 07:EBDC PhaseNothingEBDC
 * WHAT IT DOES: Nothing: a phase handler with no work.
 * HOW: RTS (RaceEndWindowNine's last).
 * Entry: any widths. Exit: every register unchanged.
 */
void abi_PhaseNothingEBDC(void)
{
    cyc(42);
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
void abi_RaceEndWindowNine(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint8_t v;

    (cyc(46), abi_BigSpritesNine());
    op_sep(P_M);
    cyc(204);
    op_lda8(b | 0x0E58);
    v = (uint8_t)cpu.a;
    if (!v) {
        cyc(46);
        op_lda8(b | 0x19D8);
        cmp8_imm(0x05);
        if ((uint8_t)cpu.a == 0x05)
            abi_RaceEndLeave();
        return;
    }
    cmp8_imm(0x01);
    if (v != 0x01) {
        cmp8_imm(0x28);
        if (v >= 0x28 || !pressed_b_start(b)) {
            cyc(44);
            op_sep(P_M);
            dec8_mem(b | 0x0E58);
            return;
        } else { cyc(40); }
        op_sep(P_M);
    }
    cyc(46);
    wr8(b | 0x0E58, 0x00);
    lda8_imm(0x01);
    cyc(16);
    wr8(b | 0x19DF, 0x01);
    (cyc(62), abi_WindowSlideUpSound());
}

/* SEI, SEP #$20, the screen blanked ($80 to INIDISP), the NMI, DMA and
   HDMA off. */
static void screen_off(void)
{
    cpu.p |= P_I;
    op_sep(P_M);
    lda8_imm(0x80);
    cyc(52);
    wr8(0x002100, 0x80);
    cyc(24);
    wr8(0x004200, 0x00);
    cyc(24);
    wr8(0x00420B, 0x00);
    cyc(24);
    wr8(0x00420C, 0x00);
}

/* REP #$30, command 5 to APU port 1 (16-bit), the delay of $4000 counts
   of A. */
static void apu_command5(void)
{
    op_rep(P_M | P_X);
    op_lda16_imm(0x0005);
    cyc(26);
    wr16(0x002141, 0x0005);
    cyc(96);
    delay_turns(0x4000);
    cpu.a = 0x0000;
    cpu.p = (uint8_t)((cpu.p & ~P_N) | P_Z);
}

/* ApuUpload of block a from y, then the delay of $4000 counts of A. */
static void apu_upload(uint16_t a, uint16_t y)
{
    op_lda16_imm(a);
    cpu.y = y;
    cpu.p = nz16(cpu.p, y);
    abi_ApuUpload();
    op_rep(P_M);
    cyc(24);
    delay_turns(0x4000);
    cpu.a = 0x0000;
    cpu.p = (uint8_t)((cpu.p & ~P_N) | P_Z);
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
void abi_SoftResetToBootWarm(void)
{
    uint8_t p1;

    screen_off();
    p1 = cpu.p;
    (cyc(14), apu_command5());
    op_lda16_imm(0xF3B5);
    cpu.y = 0x001E;
    cpu.p = nz16(cpu.p, 0x001E);
    abi_SoundDataLoad();
    op_plp(p1);
    p1 = cpu.p;
    apu_command5();
    apu_upload(0x001B, 0x85F8);
    op_plp(p1);
    p1 = cpu.p;
    apu_command5();
    apu_upload(0x0019, 0x9A98);
    op_plp(p1);
    (cyc(48), abi_SoundReset());
    op_sep(P_M);
    lda8_imm(0x01);
    cyc(38);
    wr8(0x002140, 0x01);
    cpu.p &= (uint8_t)~P_I;
    cpu_restart(0x038BAA, abi_BootWarm);
}

/*
 * @routine 07:EC6B RaceRestart
 * WHAT IT DOES: Runs the race again: the screen blanked, the NMI, DMA and
 * HDMA off, then on in BootWarm at 03:8BCD (the stack reset, RaceLoop).
 * HOW: A long jump into BootWarm (BootWarmRace), which resets the stack
 * (cpu_restart).
 * Entry: any widths. Exit: never returns.
 */
void abi_RaceRestart(void)
{
    screen_off();
    cpu_restart(0x038BCD, abi_BootWarmRace);
}

/*
 * @routine 07:EC8F RaceEndToAttract
 * WHAT IT DOES: Leaves the race: the screen blanked, the NMI, DMA and HDMA
 * off, then on in BootWarm at 03:8BF8 (after its race: the stack reset and
 * the attract race).
 * HOW: As RaceRestart.
 * Entry: any widths. Exit: never returns.
 */
void abi_RaceEndToAttract(void)
{
    screen_off();
    cpu_restart(0x038BF8, abi_BootWarmAttract);
}

/*
 * @routine 07:EC80 RaceEndVideoReset
 * WHAT IT DOES: HDMA off, the screen blanked, the video reset
 * (RaceVideoInit), then RaceEndToAttract.
 * HOW: SEP #$20, SEI; the code of RaceEndToAttract follows.
 * Entry: any widths. Exit: never returns.
 */
void abi_RaceEndVideoReset(void)
{
    op_sep(P_M);
    cyc(46);
    wr8(0x00420C, 0x00);
    lda8_imm(0x80);
    cyc(16);
    wr8(0x002100, 0x80);
    cpu.p |= P_I;
    (cyc(76), abi_RaceVideoInit());
    (cyc(0), abi_RaceEndToAttract());
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
void abi_RaceEndLeave(void)
{
    cyc(24);
    op_lda8(((uint32_t)cpu.db << 16) | 0x0E5C);
    if (!(uint8_t)cpu.a) {
        cyc(46);
        abi_RaceEndToAttract();
        return;
    }
    abi_ChampionScreenRun();
    abi_SoftResetToBootWarm();
}

/* SEP #$20, the time-up's end: $0D22 = $0D23 = 6, the timer $0E5A = t,
   $0D55 cleared, sound $30 on port 0, then RaceEndCountdown. */
static void time_up_end(uint32_t b, uint8_t t)
{
    lda8_imm(0x06);
    cyc(20);
    wr8(b | 0x0D22, 0x06);
    cyc(24);
    wr8(b | 0x0D23, 0x06);
    lda8_imm(t);
    cyc(16);
    wr8(b | 0x0E5A, t);
    cyc(24);
    wr8(b | 0x0D55, 0x00);
    cyc(16);
    lda8_imm(0x30);
    abi_SoundPort0QueuePushRaw();
    (cyc(24), abi_RaceEndCountdown());
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
void abi_TimeUpBattle(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint8_t a;

    (cyc(46), abi_BigSpritesRow6());
    op_sep(P_M);
    cyc(204);
    op_lda8(b | 0x0E59);
    if (!(uint8_t)cpu.a) {
        cyc(40);
        op_lda8(b | 0x0E5A);
        if (!(uint8_t)cpu.a) {
            cyc(70);
            lda8_imm(0x51);
            abi_SoundPort0QueuePush();
            abi_OamHide75Up();
            lda8_imm(0x3C);
            cyc(16);
            wr8(b | 0x0E5A, 0x3C);
            cyc(24);
            wr8(b | 0x0E59, 0x3C);
        }
        dec8_mem(b | 0x0E5A);
        return;
    } else { cyc(44); }
    op_sep(P_M);
    if (dec8_mem(b | 0x0E5A)) {
        cyc(22);
        dec8_mem(b | 0x0E5A);
        return;
    }
    a = (uint8_t)cpu.a;                         /* PHA */
    cyc(86);
    wr8(0x001FFE, 0x11);
    lda8_imm(a);                                /* PLA */
    time_up_end(b, 0x3C);
}

/*
 * @routine 07:ECF3 PhaseNothingECF3
 * WHAT IT DOES: Nothing: a phase handler with no work.
 * HOW: RTS (TimeUpBattle's last).
 * Entry: any widths. Exit: every register unchanged.
 */
void abi_PhaseNothingECF3(void)
{
    cyc(16);
}

/* 07:ED10 / 07:ED80: $0E5A = t, $0E59 one less, sound $51, then after(). */
static void time_up_tick(uint32_t b, uint8_t t, void (*after)(void))
{
    lda8_imm(t);
    cyc(22);
    wr8(b | 0x0E5A, t);
    dec8_mem(b | 0x0E59);
    cyc(38);
    lda8_imm(0x51);
    abi_SoundPort0QueuePush();
    (cyc(46), after());
    cyc(10);
}

/* The first part's wait: with $0DDA at $10 or more, phase 6 when the race
   time is not 0, else $0E5A counted down; returns 1 to go on. */
static int time_up_wait(uint32_t b)
{
    cyc(40);
    op_lda8(b | 0x0DDA);
    cmp8_imm(0x10);
    if ((uint8_t)cpu.a < 0x10)
        return 1;
    cyc(24);
    op_lda8(b | 0x0E70);
    if ((uint8_t)cpu.a) {
        lda8_imm(0x06);
        cyc(38);
        wr8(b | 0x0DD6, 0x06);
        return 0;
    } else { cyc(16); }
    return cyc_then(10, dec8_mem(b | 0x0E5A) == 0);
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
void abi_TimeUpStunt(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;

    (cyc(46), abi_BigSpritesRow6());
    op_sep(P_M);
    cyc(118);
    op_lda8(b | 0x0E59);
    if (!(uint8_t)cpu.a) {
        if (time_up_wait(b))
            time_up_tick(b, 0x20, abi_OamHide75Up); else { cyc(16); }
        cyc(32);
        return;
    }
    op_sep(P_M);
    cyc(68);
    op_lda8(b | 0x0E5A);
    if ((uint8_t)cpu.a) {
        cyc(22);
        dec8_mem(b | 0x0E5A);
        cyc(42);
        return;
    } else { cyc(12); }
    time_up_end(b, 0x78);
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
void abi_TimeUpRace(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint8_t v, m;

    (cyc(46), abi_BigSpritesRow6());
    op_sep(P_M | P_X);
    cyc(96);
    op_lda8(b | 0x0E59);
    if (!(uint8_t)cpu.a) {
        cyc(40);
        op_lda8(b | 0x0DE8);
        v = (uint8_t)cpu.a;
        cyc(24);
        m = rd8(b | 0x0DEA);
        cpu.p = (uint8_t)((nz8(cpu.p, (uint8_t)(v - m)) & ~P_C) | (v >= m ? P_C : 0));
        if (v > m) {
            lda8_imm(0x06);
            cyc(48);
            wr8(b | 0x0DD6, 0x06);
            (cyc(24), abi_RaceLapCrossed());
            return;
        } else { cyc(6); }
        if (time_up_wait(b))
            time_up_tick(b, 0x24, abi_Countdown0DF8); else { cyc(16); }
        cyc(32);
        return;
    }
    op_sep(P_M);
    cyc(68);
    op_lda8(b | 0x0E5A);
    v = (uint8_t)cpu.a;
    if (!v) {
        cyc(40);
        op_lda8(b | 0x19D8);
        cmp8_imm(0x05);
        if ((uint8_t)cpu.a == 0x05)
            { cyc(24); abi_RaceEndLeave(); } else { cyc(26); }
        cyc(22);
        return;
    }
    v = (uint8_t)(v - 1);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, v);
    cmp8_imm(0x01);
    if (v == 0x01) {
        cyc(60);
        wr8(b | 0x0E5A, v);
        lda8_imm(0x01);
        cyc(16);
        wr8(b | 0x19DF, 0x01);
        (cyc(62), abi_WindowSlideUpSound());
        cyc(42);
        return;
    }
    cmp8_imm(0x18);
    cyc(v == 0x18 ? 66 : 72);                          /* BNE not taken / taken */
    wr8(b | 0x0E5A, v);
    cyc(42);
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
void abi_RaceStartStepsDraw(void)
{
    start_draw((uint32_t)cpu.db << 16);
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
void abi_RaceStartStepsHold(void)
{
    start_hold((uint32_t)cpu.db << 16);
}

/*
 * @routine 07:E3E6 RaceStartTimerStepsRts
 * WHAT IT DOES: The second RTS ending RaceStartTimerSteps, which no code
 * reaches: returns at once.
 * HOW: RTS.
 * Entry: any. Exit: everything unchanged.
 */
void abi_RaceStartTimerStepsRts(void)
{
    cyc(42);
}

/*
 * @routine 07:E556 ResultsPanelStepsRts
 * WHAT IT DOES: The second RTS ending ResultsPanelSteps, which no code
 * reaches: returns at once.
 * HOW: RTS.
 * Entry: any. Exit: everything unchanged.
 */
void abi_ResultsPanelStepsRts(void)
{
    cyc(42);
}

/*
 * @routine 07:E998 RaceEndWaitRow6BRts
 * WHAT IT DOES: The RTS after RaceEndWaitRow6B's call of
 * RaceEndCountdownFade, which never returns: returns at once.
 * HOW: RTS.
 * Entry: any. Exit: everything unchanged.
 */
void abi_RaceEndWaitRow6BRts(void)
{
    cyc(42);
}

/*
 * @routine 07:E9BB RaceEndWaitBigSpritesRts
 * WHAT IT DOES: The RTS after RaceEndWaitBigSprites' call of
 * RaceEndCountdownFade, which never returns: returns at once.
 * HOW: RTS.
 * Entry: any. Exit: everything unchanged.
 */
void abi_RaceEndWaitBigSpritesRts(void)
{
    cyc(42);
}

/*
 * @routine 07:EB0F RaceEndWindowStepsRts
 * WHAT IT DOES: The RTS after RaceEndWindowSteps' last JMP, which no code
 * reaches: returns at once.
 * HOW: RTS.
 * Entry: any. Exit: everything unchanged.
 */
void abi_RaceEndWindowStepsRts(void)
{
    cyc(42);
}
