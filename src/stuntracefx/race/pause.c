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

void abi_SoundPort0QueuePush(void);
void abi_SoundPort1QueuePush(void);
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

/* LDA #v (8-bit). */
static void lda8_imm(uint8_t v)
{
    cyc(24);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, v);
}

/* DEC of a work-RAM byte; N, Z from it. */
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

    cpu.p = (uint8_t)((nz8(cpu.p, (uint8_t)(a - m)) & ~P_C) | (a >= m ? (cyc(38), P_C) : (cyc(32), 0)));
}

/* REP #$20, LDA presses, BIT #mask: Z from the test. */
static int presses(uint32_t b, uint16_t pad, uint16_t mask)
{
    op_rep(P_M);
    cyc(2);
    op_lda16(b | pad);
    cpu.p = (uint8_t)((cpu.p & ~P_Z) | ((cpu.a & mask) ? (cyc(80), 0) : (cyc(46), P_Z)));
    return (cpu.a & mask) != 0;
}

/* SEP #$20, the timer $0D5F counted down; 1 at its end. */
static int timer_out(uint32_t b)
{
    op_sep(P_M);
    cyc(22);
    return dec8_mem(b | 0x0D5F) == 0;
}

/* The stage $0D2F = s. */
static void stage(uint32_t b, uint8_t s)
{
    lda8_imm(s);
    wr8(b | 0x0D2F, s);
}

/* 07:BC88: the race resumes: sound $16 on port 1, the presses' low bytes
   ($0D1A, $0D1C) and $0D2D cleared, $0E5B and $0E5F one less. */
static void pause_resume(uint32_t b)
{
    cyc(38);
    lda8_imm(0x16);
    abi_SoundPort1QueuePush();
    cyc(8);
    wr8(b | 0x0D1A, 0x00);
    cyc(24);
    wr8(b | 0x0D1C, 0x00);
    op_sep(P_M);
    cyc(46);
    wr8(b | 0x0D2D, 0x00);
    dec8_mem(b | 0x0E5B);
    dec8_mem(b | 0x0E5F);
}

/*
 * @routine 07:BC32 PauseStageStart
 * WHAT IT DOES: The pause's first stage: the timer $0D5F = $A, stage 2.
 * HOW: 8-bit stores.
 * Entry: any widths. Exit: M 8-bit; A low byte = 2.
 */
void abi_PauseStageStart(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;

    op_sep(P_M);
    lda8_imm(0x0A);
    cyc(38);
    wr8(b | 0x0D5F, 0x0A);
    cyc(16);
    stage(b, 0x02);
}

/*
 * @routine 07:BC3F PauseStageWait
 * WHAT IT DOES: Counts the timer $0D5F down; at its end stage 4.
 * HOW: DEC of the byte.
 * Entry: any widths. Exit: M 8-bit.
 */
void abi_PauseStageWait(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;

    if (timer_out(b))
        { cyc(32); stage(b, 0x04); } else { cyc(22); }
}

/*
 * @routine 07:BC4C PauseStageResumeB
 * WHAT IT DOES: Counts the timer $0D5F down; at its end gauge 6B placed
 * (Gauge6BPlace) and the race resumed: sound $16 on port 1, the presses'
 * low bytes and $0D2D cleared, $0E5B and $0E5F one less.
 * HOW: 8-bit.
 * Entry: any widths. Exit: M 8-bit.
 */
void abi_PauseStageResumeB(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;

    if (!timer_out(b))
        { cyc(22); return; } else { cyc(62); }
    abi_Gauge6BPlace();
    cyc(22);
    pause_resume(b);
}

/*
 * @routine 07:BC58 PauseStageResumeA
 * WHAT IT DOES: As PauseStageResumeB with gauge 6A (Gauge6APlace).
 * HOW: As PauseStageResumeB. Reached only by unit_pause.
 * Entry: any widths. Exit: M 8-bit.
 */
void abi_PauseStageResumeA(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;

    if (!timer_out(b))
        { cyc(22); return; } else { cyc(62); }
    abi_Gauge6APlace();
    cyc(22);
    pause_resume(b);
}

/*
 * @routine 07:BC64 PauseStageReplayEnd
 * WHAT IT DOES: Counts the timer $0D5F down: at 1 the sprite rows
 * (OamPlaceRows75And112); at its end $0E14 = $93, $0E0C = $7C, $0E18 =
 * $8C, $0D22 cleared and the race resumed (as PauseStageResumeB).
 * HOW: 8-bit.
 * Entry: any widths. Exit: M 8-bit.
 */
void abi_PauseStageReplayEnd(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;

    if (!timer_out(b)) {
        cyc(40);
        op_lda8(b | 0x0D5F);
        cmp8_imm(0x01);
        if ((uint8_t)cpu.a == 0x01)
            abi_OamPlaceRows75And112();
        return;
    }
    lda8_imm(0x93);
    cyc(38);
    wr8(b | 0x0E14, 0x93);
    lda8_imm(0x7C);
    cyc(16);
    wr8(b | 0x0E0C, 0x7C);
    lda8_imm(0x8C);
    cyc(16);
    wr8(b | 0x0E18, 0x8C);
    cyc(24);
    wr8(b | 0x0D22, 0x00);
    pause_resume(b);
}

/* WindowSlideUpSound, then $19DF = v. */
static void slide_up(uint32_t b, uint8_t v)
{
    (cyc(62), abi_WindowSlideUpSound());
    lda8_imm(v);
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
void abi_PauseStageQuit(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;

    if (!timer_out(b))
        { cyc(16); return; }
    cyc(46);
    op_lda8(b | 0x0D2B);
    if (!(uint8_t)cpu.a) {
        cyc(32);
        stage(b, 0x0A);
        slide_up(b, 0x01);
        lda8_imm(0x70);
        cyc(16);
        wr8(b | 0x0D5F, 0x70);
        return;
    } else { cyc(38); }
    stage(b, 0x0C);
    slide_up(b, 0x00);
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
void abi_PauseStageLeave(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;

    op_sep(P_M);
    cyc(46);
    op_lda8(b | 0x10CD);
    if ((uint8_t)cpu.a) {
        lda8_imm(0x01);
        cyc(32);
        wr8(b | 0x0E3A, 0x01);
        cyc(24);
        op_lda8(b | 0x0D61);
        if ((uint8_t)cpu.a & 0x80)
            { cyc(40); abi_SoftResetToBootWarm(); } else { cyc(22); }
        return;
    }
    cyc(46);
    wr8(b | 0x0D59, 0x00);
    (cyc(16), abi_ChampionshipScreenLoop());
    op_sep(P_M);
    cyc(46);
    op_lda8(b | 0x0DD4);
    if ((uint8_t)cpu.a)
        { cyc(40); abi_RaceRestart(); }
    else
        abi_RaceEndVideoReset();
}

/*
 * @routine 07:BCD0 PauseStageQuitWait
 * WHAT IT DOES: Counts the timer $0D5F down; at its end the race again
 * (RaceRestart) with $0D24 set, else PauseStageLeave.
 * HOW: 8-bit; jumps on.
 * Entry: any widths. Exit: M 8-bit, or does not return.
 */
void abi_PauseStageQuitWait(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;

    if (!timer_out(b))
        { cyc(22); return; }
    cyc(40);
    op_lda8(b | 0x0D24);
    if ((uint8_t)cpu.a)
        { cyc(40); abi_RaceRestart(); }
    else
        { cyc(22); abi_PauseStageLeave(); }
}

/* B or START: the choice made: $19BD = $FF and the timer 8; quit ($0D3D
   set): stage 8, and in mode 0 without the override $0D4C, Countdown0DF8
   and sound $22 on port 1; continue: stage 6. */
static void choice_made(uint32_t b, int quit_extra)
{
    op_sep(P_M);
    cyc(12);
    op_lda8(b | 0x0D3D);
    if ((uint8_t)cpu.a) {
        lda8_imm(0xFF);
        cyc(32);
        wr8(b | 0x19BD, 0xFF);
        cyc(16);
        stage(b, 0x08);
        lda8_imm(0x08);
        cyc(16);
        wr8(b | 0x0D5F, 0x08);
        if (!quit_extra)
            return;
        cyc(24);
        op_lda8(b | 0x0D2B);
        if ((uint8_t)cpu.a)
            { cyc(22); return; }
        cyc(40);
        op_lda8(b | 0x0D4C);
        if ((uint8_t)cpu.a)
            { cyc(22); return; } else { cyc(32); }
        abi_Countdown0DF8();
        cyc(108);
        lda8_imm(0x22);
        abi_SoundPort1QueuePush();
        return;
    }
    op_sep(P_M);
    lda8_imm(0xFF);
    cyc(60);
    wr8(b | 0x19BD, 0xFF);
    cyc(16);
    stage(b, 0x06);
    lda8_imm(0x08);
    cyc(16);
    wr8(b | 0x0D5F, 0x08);
}

/* Left (bit 9) on continue: sound $45, $0D3D one less, left(); right (bit
   8) on quit: sound $45, $0D3D cleared, right(). `wide` reads $0D3D as a
   word (masked) and tests right with M 8-bit set first. */
static void choice_move(uint32_t b, uint16_t pad, int wide, void (*left)(void), void (*right)(void))
{
    uint8_t c;

    if (wide) {
        cyc(18);
        op_lda16(b | 0x0D3D);
        op_and16(0x00FF);
        c = (uint8_t)cpu.a;
    } else {
        op_sep(P_M);
        cyc(46);
        op_lda8(b | 0x0D3D);
        c = (uint8_t)cpu.a;
    }
    cyc(38);
    if (!c) {
        if (!presses(b, pad, 0x0200))
            return;
        op_lda16_imm(0x0045);
        abi_SoundPort0QueuePush();
        op_sep(P_M);
        cyc(14);
        dec8_mem(b | 0x0D3D);
        (cyc(46), left());
        return;
    } else { cyc(6); }
    if (wide) {
        cyc(22);
        presses(b, pad, 0x0100);
        op_sep(P_M);
        if (!(cpu.a & 0x0100))
            { cyc(22); return; } else { cyc(20); }
        lda8_imm(0x45);
    } else {
        if (!presses(b, pad, 0x0100))
            return;
        op_lda16_imm(0x0045);
    }
    abi_SoundPort0QueuePush();
    op_sep(P_M);
    wr8(b | 0x0D3D, 0x00);
    (cyc(46), right());
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
void abi_PauseStageChoice(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;

    cyc(22);
    if (presses(b, 0x0D1A, 0x9000)) {
        cyc(18);
        choice_made(b, 1);
        return;
    }
    choice_move(b, 0x0D1A, 0, abi_OamBlock52Left, abi_OamBlock52Right);
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
void abi_PauseStageChoiceP1(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;

    cyc(22);
    if (presses(b, 0x0D1A, 0x9000)) {
        choice_made(b, 0);
        return;
    }
    choice_move(b, 0x0D1A, 1, abi_OamBlock59Left, abi_OamBlock59Right);
}

/*
 * @routine 07:BDFB PauseStageChoiceP2
 * WHAT IT DOES: As PauseStageChoiceP1 for pad 2 ($0D1C; sprites 123-127:
 * OamBlock123Left / OamBlock123Right).
 * HOW: As PauseStageChoiceP1. Reached only by unit_pause.
 * Entry: any widths. Exit: as PauseStageChoiceP1.
 */
void abi_PauseStageChoiceP2(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;

    cyc(22);
    if (presses(b, 0x0D1C, 0x9000)) {
        choice_made(b, 0);
        return;
    }
    choice_move(b, 0x0D1C, 1, abi_OamBlock123Left, abi_OamBlock123Right);
}

/* Counts the timer $0D5F down; at 0 $0D2E = owner, $19DF cleared, the
   window slid up and stage $A. */
static void continue_wait(uint8_t owner)
{
    uint32_t b = (uint32_t)cpu.db << 16;

    op_sep(P_M);
    cyc(46);
    op_lda8(b | 0x0D5F);
    if ((uint8_t)cpu.a) {
        cyc(16);
        dec8_mem(b | 0x0D5F);
        return;
    }
    lda8_imm(owner);
    cyc(38);
    wr8(b | 0x0D2E, owner);
    lda8_imm(0x00);
    wr8(b | 0x19DF, 0x00);
    (cyc(62), abi_WindowSlideUpSound());
    cyc(16);
    stage(b, 0x0A);
}

/*
 * @routine 07:BE40 PauseStageContinueP2
 * WHAT IT DOES: Counts the timer $0D5F down; at its end $0D2E = $FF (pad
 * 2's), $19DF cleared, the window slid up and stage $A.
 * HOW: 8-bit. Reached only by unit_pause.
 * Entry: any widths. Exit: M 8-bit.
 */
void abi_PauseStageContinueP2(void)
{
    continue_wait(0xFF);
}

/*
 * @routine 07:BE52 PauseStageContinue
 * WHAT IT DOES: As PauseStageContinueP2 with $0D2E cleared (pad 1's).
 * HOW: 8-bit.
 * Entry: any widths. Exit: M 8-bit.
 */
void abi_PauseStageContinue(void)
{
    continue_wait(0x00);
}

/* 07:BEAB: the quit menu opened: $19DF cleared, the split window state 2,
   the cursor $0D50 cleared, stage $C and $0D22 = $C. */
static void quit_menu_open(uint32_t b)
{
    lda8_imm(0x00);
    cyc(86);
    wr8(b | 0x19DF, 0x00);
    lda8_imm(0x02);
    cyc(16);
    wr8(b | 0x19D8, 0x02);
    cyc(16);
    stage(b, 0x0A);
    cyc(24);
    wr8(b | 0x0D50, 0x00);
    cyc(16);
    stage(b, 0x0C);
    cyc(24);
    wr8(b | 0x0D22, 0x0C);
}

/* LDA $19D8, CMP #5: the split window closed. */
static int window_closed(uint32_t b)
{
    cyc(24);
    op_lda8(b | 0x19D8);
    cmp8_imm(0x05);
    return (uint8_t)cpu.a == 0x05;
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
void abi_PauseStageQuitMenuP2(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;

    if (!window_closed(b))
        { cyc(6); return; }
    op_sep(P_M);
    lda8_imm(0xFF);
    cyc(32);
    wr8(b | 0x0D2E, 0xFF);
    lda8_imm(0x01);
    cyc(16);
    wr8(b | 0x0D5D, 0x01);
    lda8_imm(0x0E);
    cyc(16);
    wr8(b | 0x0D23, 0x0E);
    abi_OamHighBits58Park112();
    cyc(22);
    quit_menu_open(b);
}

/*
 * @routine 07:BE8F PauseStageQuitMenu
 * WHAT IT DOES: As PauseStageQuitMenuP2 for pad 1: $0D2E cleared, $0D5C =
 * 1, $0D23 = $C, sprites 18 up parked (OamHighBits18Park48).
 * HOW: 8-bit.
 * Entry: M 8-bit. Exit: M 8-bit.
 */
void abi_PauseStageQuitMenu(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;

    if (!window_closed(b))
        return;
    op_sep(P_M);
    lda8_imm(0x00);
    cyc(38);
    wr8(b | 0x0D2E, 0x00);
    lda8_imm(0x01);
    cyc(16);
    wr8(b | 0x0D5C, 0x01);
    lda8_imm(0x0C);
    cyc(16);
    wr8(b | 0x0D23, 0x0C);
    abi_OamHighBits18Park48();
    quit_menu_open(b);
}

/*
 * @routine 07:BEC6 PauseStageQuitTaken
 * WHAT IT DOES: Once the split window is closed: the race again
 * (RaceRestart) with the cursor $0D50 not 0, else RaceEndToAttract.
 * HOW: 8-bit; jumps on.
 * Entry: any widths. Exit: M 8-bit, or does not return.
 */
void abi_PauseStageQuitTaken(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;

    op_sep(P_M);
    cyc(22);
    if (!window_closed(b))
        { cyc(6); return; }
    cyc(18);
    op_lda8(b | 0x0D50);
    if ((uint8_t)cpu.a)
        { cyc(40); abi_RaceRestart(); }
    else
        { cyc(46); abi_RaceEndToAttract(); }
}

/* The quit menu's rows: the cursor's row cycles its palette, the others
   palette 5 (rows 87, 107, 187 on the screen). */
static void rows(void (*r87)(void), void (*r107)(void), void (*r187)(void))
{
    (cyc(44), r87());
    (cyc(64), r107());
    (cyc(64), r187());
}

/* Down (bit 10) or up (bit 11) on pad: sound $45 and the cursor $0D50 =
   to; returns 1 when taken. */
static int cursor_move(uint32_t b, uint16_t pad, uint16_t bit, uint8_t to)
{
    cyc(68);
    if (!presses(b, pad, bit))
        return 0; else { cyc(20); }
    op_sep(P_M);
    lda8_imm(0x45);
    abi_SoundPort0QueuePush();
    lda8_imm(to);
    wr8(b | 0x0D50, to);
    return 1;
}

/* B or START in the quit menu: cursor 0 closes it (sound $46, stage $E,
   $19DF = $19D8 = 1); 2 or 6 back to 0 (sound $46); 4 the cars (sound $46,
   cursor 6), the first time also $10BA = $FF, `which` = $FF, $0D23 = d and
   park(). */
static void quit_menu_take(uint32_t b, uint16_t which, uint8_t d, void (*park)(void))
{
    cyc(6);
    op_lda8(b | 0x0D50);
    cmp8_imm(0x04);
    if ((uint8_t)cpu.a == 0x04) {
        cyc(24);
        op_lda8(b | 0x10BA);
        if (!(uint8_t)cpu.a) {
            lda8_imm(0xFF);
            cyc(32);
            wr8(b | 0x10BA, 0xFF);
            lda8_imm(0xFF);
            cyc(16);
            wr8(b | which, 0xFF);
            lda8_imm(d);
            cyc(16);
            wr8(b | 0x0D23, d);
            park();
        }
        cyc(60);
        lda8_imm(0x46);
        abi_SoundPort0QueuePush();
        lda8_imm(0x06);
        wr8(b | 0x0D50, 0x06);
        return;
    } else { cyc(54); }
    if ((uint8_t)cpu.a > 0x04) {
        lda8_imm(0x46);
        abi_SoundPort0QueuePush();
        cyc(8);
        wr8(b | 0x0D50, 0x00);
        return;
    }
    lda8_imm(0x46);
    abi_SoundPort0QueuePush();
    stage(b, 0x0E);
    lda8_imm(0x01);
    cyc(16);
    wr8(b | 0x19DF, 0x01);
    cyc(24);
    wr8(b | 0x19D8, 0x01);
}

/* The quit menu for pad: B or START takes the cursor; else by the cursor
   $0D50: 0 down to 2; 2 down to 4 or up to 0; 4 up to 2; 6 the cars
   (CarSelectTwoPlayers, then cars()); each with its rows. */
static void quit_menu(uint16_t pad, uint16_t which, uint8_t d, void (*park)(void), void (*cars)(void))
{
    uint32_t b = (uint32_t)cpu.db << 16;

    op_sep(P_M | P_X);
    cyc(22);
    presses(b, pad, 0x9000);
    op_sep(P_M);
    if (cpu.a & 0x9000) {
        quit_menu_take(b, which, d, park);
        return;
    }
    cyc(46);
    cpu.x = rd8(b | 0x0D50);
    cpu.p = nz8(cpu.p, (uint8_t)cpu.x);
    switch (cpu.x) {
    case 0x00:
        cursor_move(b, pad, 0x0400, 0x02);
        cyc(46);
        op_sep(P_M);
        rows(abi_TilemapPaletteCycle87, abi_TilemapRow107Palette5, abi_TilemapRow187Palette5);
        return;
    case 0x02:
        if (!cursor_move(b, pad, 0x0400, 0x04)) {
            presses(b, pad, 0x0800);
            if (cpu.a & 0x0800) {
                cyc(20);
                op_sep(P_M);
                lda8_imm(0x45);
                abi_SoundPort0QueuePush();
                lda8_imm(0x00);
                wr8(b | 0x0D50, 0x00);
            }
        } else { cyc(22); }
        op_sep(P_M);
        rows(abi_TilemapRow87Palette5, abi_TilemapPaletteCycle107, abi_TilemapRow187Palette5);
        return;
    case 0x04:
        cursor_move(b, pad, 0x0800, 0x02);
        cyc(46);
        break;
    case 0x06:
        cyc(70);
        abi_CarSelectTwoPlayers();
        cars();
        break;
    default:
        jump_routine(0x070000 | rd16(0x070000 | (uint16_t)((pad == 0x0D1A ? 0xC02D : 0xBF39) + cpu.x)));
        return;
    }
    op_sep(P_M);
    rows(abi_TilemapPaletteCycle187, abi_TilemapRow107Palette5, abi_TilemapRow87Palette5);
}

/*
 * @routine 07:BEDB PauseQuitMenuP2
 * WHAT IT DOES: The quit menu for pad 2 ($0D1C): see PauseQuitMenu; the
 * cars first opened set $0D5C = $FF, $0D23 = $C and park sprites 18 up
 * (OamHighBits18Park48); the cars drawn with SpritePiecesTwoCarsTop.
 * HOW: JMP ($BF39,X) on the cursor. Reached only by unit_pause.
 * Entry: any widths. Exit: as the path leaves it.
 */
void abi_PauseQuitMenuP2(void)
{
    quit_menu(0x0D1C, 0x0D5C, 0x0C, abi_OamHighBits18Park48, abi_SpritePiecesTwoCarsTop);
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
void abi_PauseQuitMenu(void)
{
    quit_menu(0x0D1A, 0x0D5D, 0x0E, abi_OamHighBits58Park112, abi_SpritePiecesTwoCars);
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
void abi_PauseQuitMenuCursor0(void)
{
    lda8_imm(0x00);
    cyc(16);
    wr8((uint32_t)cpu.db << 16 | 0x0D50, 0x00);
}

/*
 * @routine 07:C03E PauseQuitMenuBack
 * WHAT IT DOES: Bytes in PauseQuitMenu that no code reaches: sound $46 and
 * the quit menu's cursor $0D50 = 0.
 * HOW: SEP #$20, LDA #$46, JSL SoundPort0QueuePush, STZ, RTS.
 * Entry: any widths. Exit: M 8-bit; as SoundPort0QueuePush leaves the
 * rest.
 */
void abi_PauseQuitMenuBack(void)
{
    op_sep(P_M);
    cyc(60);
    lda8_imm(0x46);
    abi_SoundPort0QueuePush();
    cyc(8);
    wr8((uint32_t)cpu.db << 16 | 0x0D50, 0x00);
}
