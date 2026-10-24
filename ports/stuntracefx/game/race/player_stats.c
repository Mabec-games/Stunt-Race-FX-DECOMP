/*
 * player_stats.c: the per-player race statistics the HUD and the results
 * show, taken each frame from the player car's Super FX record, and the
 * race phase dispatchers that run after them.
 */
#include "machine.h"
#include "native.h"
#include "hw.h"
#include "cpu_ops.h"
#include "carried.h"

#include "gsu_programs_calls.h"
#include "wram_names.h"
uint8_t GsuF15BTwoLines(uint16_t input);
uint16_t GsuRecordBits60(void);
uint8_t SaveChecksum(void);
void abi_GlyphRowsAt32(void);
void abi_GlyphRowsAt56(void);
void abi_GlyphSpritesSlideIn(void);
void abi_GlyphSpritesSlideOut(void);
void abi_PhaseNothingEBDC(void);
void abi_PlayerTwoFrameWork(void);
void abi_RaceEndLeave(void);
void abi_RaceEndVideoReset(void);
void abi_RaceEndWaitBigSprites(void);
void abi_RaceRestart(void);
void abi_RaceStartCountdown(void);
void abi_RaceStartCountdownStunt(void);
void abi_RaceStartGlyphsSteps(void);
void abi_RaceStartSteps(void);
void abi_RecordsLoadSpecialCourseLong(void);
void abi_ResultsPanelSteps(void);
void abi_SoftResetToBootWarm(void);
void abi_StuntRecordTime(void);
void abi_TimeUpBattle(void);
void abi_TimeUpRecordUpdate(void);
void abi_TimeUpStunt(void);

void abi_CarFinishSound(void);

void abi_GsuRecordBits60(void);

/*
 * @routine 07:DDCE SpeedSampleAdd
 * WHAT IT DOES: Turns speed A into a speedometer sample, |A| * $3C / 256
 * kept to $FF, stores it as player Y's sample ($0DDA + Y), and adds it to
 * that player's 32-bit total (player 0: $0DDC-$0DDF, else $0DE0-$0DE3),
 * counting the samples in $0DE4 (player 0) or $0DE6. A total that would
 * pass $FFFFFFFF stays at $FFFFFFFF and the sample is not counted (for
 * player 1 only the low word is set to $FFFF, stored twice).
 * HOW: The product comes from the PPU multiplier ($211B twice, $211C, then
 * $2135-$2136). The clamp and the overflow cases are not reached in the
 * traces so far.
 * Entry: M 16-bit, X 16-bit, D = $0000; A = speed (signed), Y = player.
 * Exit: A = the total's high word; M 16-bit; X, Y unchanged. Counted: N, Z
 * from the new count, C clear, V from the high word's add. Kept at the
 * limit: A = $FFFF, N, C set, Z, V clear.
 */
void SpeedSampleAdd(uint8_t bank, uint16_t speed, uint16_t player, uint8_t *p, uint16_t *a)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t sum, cnt, lo, hi, m;
    unsigned s, h;
    uint8_t v;

    if (speed >= 0x8000)
        { cyc(32); speed = (uint16_t)(~speed + 1); }
    cyc(92);
    io_wr8(b | M7A, (uint8_t)speed);
    cyc(44);
    io_wr8(b | M7A, (uint8_t)(speed >> 8));
    ppu_mul_factor = (int16_t)speed;
    cyc(40);
    io_wr8(b | M7B, 0x3C);
    cyc(46);
    m = io_rd16(b | MPYM, (uint16_t)(ppu_mul(0x3C) >> 8));
    v = m < 0x00FF ? (uint8_t)m : (cyc(18), 0xFF);
    cyc(98);
    wr8(0x7E0000 | (uint16_t)(W(speed_sample) + player), v);
    sum = player == 0 ? 0x0DDC : (cyc(6), 0x0DE0);
    cnt = player == 0 ? 0x0DE4 : 0x0DE6;
    cyc(124);
    s = (unsigned)v + rd16(b | sum);
    lo = (uint16_t)s;
    cyc(24);
    wr16(b | sum, lo);
    cyc(24);
    hi = rd16(b | (uint16_t)(sum + 2));
    h = (unsigned)hi + (s >> 16);
    *p = (uint8_t)((*p & ~(P_V | P_C)) | ((~hi & (uint16_t)h & 0x8000) ? P_V : 0));
    if (h > 0xFFFF) {
        cyc(88);
        wr16(b | sum, 0xFFFF);
        cyc(24);
        wr16(b | (uint16_t)(player == 0 ? sum + 2 : sum), 0xFFFF);
        *a = 0xFFFF;
        *p = (uint8_t)(nz16(*p, 0xFFFF) | P_C);
        cyc(42);
        return;
    }
    cyc(70);
    wr16(b | (uint16_t)(sum + 2), (uint16_t)h);
    *a = (uint16_t)h;
    cyc(24);
    m = (uint16_t)(rd16(b | cnt) + 1);
    cyc(14);
    wr16(b | cnt, m);
    *p = nz16(*p, m);
    cyc(42);
}

void PhaseDispatchE232(Carried *cr);
void PhaseDispatchE24F(Carried *cr);
void PlayerOnePhaseEnd(Carried *cr);
void PlayerOnePhaseRace(Carried *cr);
void PlayerOnePhaseStep(Carried *cr);
void PlayerOnePhaseTally(Carried *cr);
void PlayerStatsThenPhaseDD35(Carried *cr);
void PlayerStatsThenPhaseE232(Carried *cr);
void RaceEndCountdown(Carried *cr);
void RaceEndCountdownFade(Carried *cr);
void RaceEndCountdownFadeOn(Carried *cr);
void RaceEndCountdownOn(Carried *cr);
void RaceHudModeTwo(Carried *cr);
void RaceSoundsAndHud(Carried *cr);
void RaceSoundsPlayer1(Carried *cr);
void RaceSoundsPlayer2(Carried *cr);
void RaceXPressPlayer1(Carried *cr);
void RaceXPressPlayer2(Carried *cr);
void SpeedSampleAdd_cr(Carried *cr);
void StuntCourseMark(Carried *cr);
void StuntPanelCount(Carried *cr);
void StuntPanelDone(Carried *cr);
void StuntPanelShow(Carried *cr);
void StuntPanelWait(Carried *cr);
void StuntPhaseEnd(Carried *cr);
void StuntPhaseRace(Carried *cr);
void StuntPhaseStep(Carried *cr);
void TimeUpPhaseEnd(Carried *cr);
void TimeUpPhaseRace(Carried *cr);
void TimeUpPhaseSlideIn(Carried *cr);
void TimeUpPhaseSlideOut(Carried *cr);
void TimeUpPhaseStart(Carried *cr);
void TimeUpPhaseStep(Carried *cr);
void PlayerStatsAndPaletteAnimate(Carried *cr);
void PlayerStatsFromRecord(Carried *cr);
static void hud_car_word(Carried *cr, uint16_t set, uint16_t clear);
static void hud_end(Carried *cr, int both, int inner, uint8_t p0, uint8_t db0);
static void hud_mode_one(Carried *cr, uint8_t p0, uint8_t db0);
static void hud_mode_two(Carried *cr, int inner, uint8_t p0, uint8_t db0);
static void hud_mode_zero(Carried *cr, uint8_t p0, uint8_t db0);
static void p1_cmp8(Carried *cr, uint8_t m);
static void p1_digit_glyph(Carried *cr, uint32_t b, unsigned shift, uint16_t at);
static void p1_lda8(Carried *cr, uint8_t v);
static void p1_next(Carried *cr, uint16_t end);
static uint8_t p1_step8(Carried *cr, uint32_t at, int d);
static void phase_jump(Carried *cr, uint16_t table);
static void player0_stats(Carried *cr);
static void race_end_go_on(Carried *cr, uint32_t b);
static void race_end_vblank(Carried *cr, uint32_t b, uint32_t branch);
static void race_flag_p1(Carried *cr, uint32_t b);
static void race_flag_p2(Carried *cr, uint32_t b);
static void race_frame_end(Carried *cr, uint32_t b, uint16_t pressed, int inner, uint8_t p0, uint8_t db0);
static void race_stats(Carried *cr, uint16_t player, uint32_t rec);
static void race_x_block(Carried *cr, uint32_t b, uint8_t stage, void (*flag)(Carried *, uint32_t), void (*park)(Carried *));
static void race_x_test(Carried *cr, uint32_t b, uint16_t pressed);
static void stats_player0(Carried *cr);
static void stunt_count_byte(Carried *cr, uint32_t b);
static void stunt_count_down(Carried *cr, uint32_t b, uint32_t lead);
static int time_cmp8(Carried *cr, uint8_t m);
static void w_GlyphRowsAt32(Carried *cr);
static void w_GlyphRowsAt56(Carried *cr);
static void w_GsuF15BTwoLines(Carried *cr);
static void w_GsuRecordBits60(Carried *cr);
void PhaseNothingEBDC_cr(Carried *cr);
static void w_PhaseNothingEBDC(Carried *cr);
static void w_PlayerStatsFromRecord(Carried *cr);
static void w_SaveChecksum(Carried *cr);
void CarFinishSound(Carried *cr);
void ChampionshipScreenLoop(Carried *cr);
void CoursePaletteCycle_cr(Carried *cr);
void EngineSoundsPlayer1_cr(Carried *cr);
void EngineSoundsPlayer2_cr(Carried *cr);
void GlyphSpritesDrawAll_cr(Carried *cr);
void GlyphSpritesSlideIn_cr(Carried *cr);
void GlyphSpritesSlideOut_cr(Carried *cr);
void GsuBytes200_BFBAB9_cr(Carried *cr);
void GsuDigitPair0E74At_cr(Carried *cr);
void HudBlankDraw_cr(Carried *cr);
void HudHoldDraw_cr(Carried *cr);
void HudLapDigitsDraw_cr(Carried *cr);
void HudLeadWordDraw_cr(Carried *cr);
void HudTimesDraw_cr(Carried *cr);
void OamHide75Up_cr(Carried *cr);
void OamHideForHudMode_cr(Carried *cr);
void OamHighBits18Park48_cr(Carried *cr);
void OamHighBits58Park112_cr(Carried *cr);
void OverlayFramesUntilFaded(Carried *cr);
void PaletteDarkenFour_cr(Carried *cr);
void PauseOnStart(Carried *cr);
void PlayerTwoFrameWork(Carried *cr);
void RaceEndLeave(Carried *cr);
void RaceEndVideoReset(Carried *cr);
void RaceEndWaitBigSprites(Carried *cr);
void RaceRestart(Carried *cr);
void RaceStartCountdown(Carried *cr);
void RaceStartCountdownStunt(Carried *cr);
void RaceStartGlyphsSteps(Carried *cr);
void RaceStartSteps(Carried *cr);
void RecordsLoadSpecialCourseLong_cr(Carried *cr);
void ResultsPanelSteps(Carried *cr);
void SoftResetCheck_cr(Carried *cr);
void SoftResetToBootWarm(Carried *cr);
void SoundPort0QueuePushRaw_cr(Carried *cr);
void SoundPort0QueuePush_cr(Carried *cr);
void SoundPort1QueuePush_cr(Carried *cr);
void SoundReset_cr(Carried *cr);
void StuntRecordTime_cr(Carried *cr);
void TimeBonusTally_cr(Carried *cr);
void TimeBonusTransfer_cr(Carried *cr);
void TimeLeftDigits_cr(Carried *cr);
void TimeUpBattle(Carried *cr);
void TimeUpRecordUpdate_cr(Carried *cr);
void TimeUpStunt(Carried *cr);
void WindowSlideUpSound_cr(Carried *cr);

void SpeedSampleAdd_cr(Carried *cr)
{
    uint8_t p = cr->p;

    SpeedSampleAdd(cr->db, cr->a, cr->y, &p, &cr->a);
    set_p(cr, (uint8_t)(p & ~P_M));
}

void abi_SpeedSampleAdd(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    SpeedSampleAdd_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:DD60 PlayerStatsFromRecord
 * WHAT IT DOES: Unless $0E57 is set, copies player Y's figures from its
 * car's Super FX record X: the bytes of +$40-$43 to $0DEC, $0DEE, $0DF0 and
 * $0DF2 (+ Y), +$4E to $0DF6, +$82 / 8 to $0DF9 and its gauge position
 * ($30 + 2 * that, kept to $3F first) to $0DFB, a speedometer sample of
 * +$4A (SpeedSampleAdd), +$6A / 8 to $0E05, +$7A to $0DE8 and the low
 * nibble of +$88 to $0DF4. Then GsuRecordBits60.
 * HOW: Long reads of Super FX RAM in that order, byte stores through the
 * data bank.
 * Entry: M 8-bit, X 16-bit, D = $0000; X = record, Y = player. Exit: A as
 * GsuRecordBits60 leaves it; X, Y unchanged; M 8-bit; skipped: N, Z from
 * $0E57; else N, Z from the +$88 nibble, C from the last shift of +$6A,
 * V as SpeedSampleAdd leaves it.
 */
void PlayerStatsFromRecord(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;
    uint16_t x = cr->x, y = cr->y, w;
    uint8_t v;

    cyc(24);
    v = rd8(0x7E0000 | W(wram_0E57));
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    set_p(cr, nz8(cr->p, v));
    if (!v) {
        cyc(48);
        wr8(0x7E0000 | (uint16_t)(W(player_rec_40) + y), cyc_then(30, rd8(0x700040 + x)));
        cyc(32);
        wr8(0x7E0000 | (uint16_t)(W(player_rec_41) + y), cyc_then(30, rd8(0x700041 + x)));
        cyc(32);
        wr8(0x7E0000 | (uint16_t)(W(player_rec_42) + y), cyc_then(30, rd8(0x700042 + x)));
        cyc(32);
        wr8(0x7E0000 | (uint16_t)(W(player_rec_43) + y), cyc_then(30, rd8(0x700043 + x)));
        cyc(32);
        wr8(0x7E0000 | (uint16_t)(W(player_rec_4e) + y), cyc_then(30, rd8(0x70004E + x)));
        cyc(54);
        w = rd16(0x700082 + x) >> 3;
        v = (uint8_t)w;
        cyc(94);
        wr8(0x7E0000 | (uint16_t)(W(gauge_b4_value) + y), v);
        if (v >= 0x3F)
            { cyc(10); v = 0x3F; }
        cyc(98);
        wr8(0x7E0000 | (uint16_t)(W(kept_sub) + y), (uint8_t)((v << 1) + 0x30));
        cyc(54);
        cr->a = rd16(0x70004A + x);
        set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
        (cyc(46), SpeedSampleAdd_cr(cr));
        cyc(32);
        w = rd16(0x70006A + x);
        set_p(cr, (uint8_t)((cr->p & ~P_C) | (w & 0x0004 ? P_C : 0)));
        w >>= 3;
        cyc(94);
        wr8(0x7E0000 | (uint16_t)(W(kept_enable) + y), (uint8_t)w);
        cyc(32);
        wr8(0x7E0000 | (uint16_t)(W(player_rec_7a) + y), cyc_then(46, rd8(0x70007A + x)));
        cyc(32);
        v = (uint8_t)(rd8(0x700088 + x) & 0x0F);
        cyc(46);
        wr8(0x7E0000 | (uint16_t)(W(player_mode) + y), v);
        cr->a = (uint16_t)((w & 0xFF00) | v);
        set_p(cr, (uint8_t)(nz8(cr->p, v) | P_M));
    } else { cyc(22); }
    cyc(62);
    w_GsuRecordBits60(cr);
    cyc(42);
}

void abi_PlayerStatsFromRecord(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PlayerStatsFromRecord(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* Player 0's figures from its record at $70:2476: Y = 0, X = the record,
   M 8-bit, X 16-bit. */
static void stats_player0(Carried *cr)
{
    uint16_t r;

    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~(P_M | P_X))));
    cr->y = 0x0000;
    cyc(100);
    r = rd16(0x702476);
    cr->a = r;
    cr->x = r;
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    (cyc(82), PlayerStatsFromRecord(cr));
}

/*
 * @routine 07:DD47 PlayerStatsAndPaletteAnimate
 * WHAT IT DOES: Player 0's figures from its record (PlayerStatsFromRecord),
 * then $0E5F = $FF.
 * HOW: P saved and restored.
 * Entry: any widths, D = $0000. Exit: A low byte = $FF; X = the record;
 * Y = 0; P unchanged.
 */
void PlayerStatsAndPaletteAnimate(Carried *cr)
{
    uint8_t p = cr->p;

    stats_player0(cr);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0xFF);
    cyc(62);
    wr8(0x7E0000 | W(palette_animate), 0xFF);
    set_p(cr, p);
    cyc(72);                            /* its return */
}

void abi_PlayerStatsAndPaletteAnimate(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PlayerStatsAndPaletteAnimate(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void abi_PlayerStatsFromRecord(void);

/* LDA $0DD6 (in the current width), TAX, JMP (table,X): the race phase's
   handler (jump_routine); it returns to the caller, or through the
   caller's frame. */
static void phase_jump(Carried *cr, uint16_t table)
{
    uint32_t b = (uint32_t)cr->db << 16;

    if (cr->p & P_M)
        { cyc(24); r_lda8(cr, 0x7E0000 | W(wram_0DD6)); }
    else
        r_lda16(cr, 0x7E0000 | W(wram_0DD6));
    cr->x = (cr->p & P_X) ? (uint16_t)(cr->a & 0x00FF) : cr->a;
    set_p(cr, (cr->p & P_X) ? nz8(cr->p, (uint8_t)cr->x) : nz16(cr->p, cr->x));
    cyc(60);
    carried_jump(cr, 0x070000 | rd16(0x070000 | (uint16_t)(table + cr->x)));
}

/*
 * @routine 07:E22B PhaseDispatchE232
 * WHAT IT DOES: Runs the handler for race phase $0DD6 from the table at
 * $07:E232.
 * HOW: JMP (table,X); the handler returns to the caller, or through its
 * caller's frame (jump_routine).
 * Entry: any widths. Exit: as the handler leaves it.
 */
void PhaseDispatchE232(Carried *cr)
{
    cyc(0);
    phase_jump(cr, 0xE232);
}

void abi_PhaseDispatchE232(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PhaseDispatchE232(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:E246 PhaseDispatchE24F
 * WHAT IT DOES: Runs the handler for race phase $0DD6 from the table at
 * $07:E24F, with 8-bit registers.
 * HOW: SEP #$30, then JMP (table,X).
 * Entry: any widths. Exit: as the handler leaves it.
 */
void PhaseDispatchE24F(Carried *cr)
{
    r_sep(cr, P_M | P_X);
    phase_jump(cr, 0xE24F);
}

void abi_PhaseDispatchE24F(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PhaseDispatchE24F(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* REP #$30, LDY #0, LDA.L $70:2476, TAX, SEP #$20, JSR
   PlayerStatsFromRecord, SEP #$10: player 0's figures. */
static void player0_stats(Carried *cr)
{
    r_rep(cr, P_M | P_X);
    cr->y = 0;
    set_p(cr, nz16(cr->p, 0));
    cyc(56);
    r_lda16(cr, 0x702476);
    r_tax16(cr);
    r_sep(cr, P_M);
    (cyc(52), w_PlayerStatsFromRecord(cr));
    r_sep(cr, P_X);
}

/*
 * @routine 07:DCEC PlayerStatsThenPhaseE232
 * WHAT IT DOES: Player 0's figures from its record, then the race phase
 * handler from the table at $07:E232 with Y = 0.
 * HOW: PlayerStatsFromRecord, SEP #$10, then PhaseDispatchE232.
 * Entry: any widths, D = $0000. Exit: as the phase handler leaves it.
 */
void PlayerStatsThenPhaseE232(Carried *cr)
{
    player0_stats(cr);
    cr->y = 0;
    set_p(cr, nz8(cr->p, 0));
    (cyc(62), PhaseDispatchE232(cr));
}

void abi_PlayerStatsThenPhaseE232(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PlayerStatsThenPhaseE232(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:DD03 PlayerStatsThenPhaseDD35
 * WHAT IT DOES: Player 0's figures from its record; then, when $10CD is set
 * and the race phase $0DD6 is 8 or more, starts the fade out ($0E3A = 1)
 * and, while bit 7 of $0D61 is set, goes on to $07:EBE9; otherwise runs the
 * race phase handler from the table at $07:DD35.
 * HOW: PlayerStatsFromRecord, SEP #$10, then the checks; the handlers and
 * $07:EBE9 are entered by JMP and return to the caller.
 * Entry: any widths, D = $0000. Exit: fading with bit 7 of $0D61 clear: A
 * low byte = $0D61, N, Z from it, C set; otherwise as the handler leaves it.
 */
void PlayerStatsThenPhaseDD35(Carried *cr)
{
    uint32_t b;
    uint8_t v;

    (cyc(0), player0_stats(cr));
    b = (uint32_t)cr->db << 16;
    cyc(24);
    r_lda8(cr, 0x7E0000 | W(wram_10CD));
    if ((uint8_t)cr->a) {
        cyc(40);
        r_lda8(cr, 0x7E0000 | W(wram_0DD6));
        v = (uint8_t)cr->a;
        set_p(cr, (uint8_t)((nz8(cr->p, (uint8_t)(v - 0x08)) & ~P_C) | (v >= 0x08 ? P_C : (cyc(38), 0))));
        if (v >= 0x08) {
            cr->a = (uint16_t)((cr->a & 0xFF00) | 0x01);
            set_p(cr, nz8(cr->p, 0x01));
            cyc(72);
            wr8(0x7E0000 | W(fade_out), 0x01);
            cyc(24);
            r_lda8(cr, 0x7E0000 | W(fade_level_hi));
            if (!((uint8_t)cr->a & 0x80))
                { cyc(64); return; } else { cyc(40); }
            SoftResetToBootWarm(cr);
            return;
        }
    } else { cyc(22); }                 /* BEQ taken */
    phase_jump(cr, 0xDD35);
}

void abi_PlayerStatsThenPhaseDD35(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PlayerStatsThenPhaseDD35(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void abi_SoftResetCheck(void);
void abi_PlayerStatsFromRecord(void);
void abi_EngineSoundsPlayer1(void);
void abi_EngineSoundsPlayer2(void);
void abi_OamHighBits18Park48(void);
void abi_OamHighBits58Park112(void);

/* The X-press block of a player's frame, which nothing reaches (the code
   before it loads 0 after its BIT, so its BEQ always branches past): the
   palette's first colour $1CC4, $0D22 = $0D23 = `stage`, the player's own
   flag and the sprites parked. */
static void race_x_block(Carried *cr, uint32_t b, uint8_t stage, void (*flag)(Carried *, uint32_t), void (*park)(Carried *))
{
    cr->a = 0x1CC4;
    set_p(cr, nz16(cr->p, cr->a));
    cyc(56);
    wr16(W(wram_7EFD65), 0x1CC4);
    r_sep(cr, P_M);
    cr->a = (uint16_t)((cr->a & 0xFF00) | stage);
    set_p(cr, nz8(cr->p, stage));
    cyc(62);
    wr8(0x7E0000 | W(wram_0D22), stage);
    cyc(24);
    wr8(0x7E0000 | W(wram_0D23), stage);
    flag(cr, b);
    park(cr);
    r_rep(cr, P_M);
}

/* The end of a player's frame: `pressed` cleared, then the data bank and
   P pulled: from the stack for an inner entry, else as kept. */
static void race_frame_end(Carried *cr, uint32_t b, uint16_t pressed, int inner, uint8_t p0, uint8_t db0)
{
    cyc(inner ? 24 : 56);
    wr16(b | pressed, 0x0000);
    if (inner) {
        cpu.s++;
        cyc(20);
        db0 = rd8((uint16_t)cpu.s);
        cpu.s++;
        cyc(20);
        p0 = rd8((uint16_t)cpu.s);
    } else { cyc(56); }
    cyc(2);
    cr->db = db0;
    set_p(cr, nz8(cr->p, db0));
    r_plp(cr, p0);
}

/* The test before the X-press block: LDA pressed, BIT #$40, LDA #0. */
static void race_x_test(Carried *cr, uint32_t b, uint16_t pressed)
{
    r_rep(cr, P_M);
    cyc(24);
    r_lda16(cr, b | pressed);
    r_lda16_imm(cr, 0x0000);
}

static void race_flag_p1(Carried *cr, uint32_t b)
{
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0xFF);
    set_p(cr, nz8(cr->p, 0xFF));
    cyc(40);
    wr8(0x7E0000 | W(wram_0D5C), 0xFF);
}

static void race_flag_p2(Carried *cr, uint32_t b)
{
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0x01);
    set_p(cr, nz8(cr->p, 0x01));
    cyc(40);
    wr8(0x7E0000 | W(wram_0D5D), 0x01);
}

/* A player's stats: Y = player, X = the record word at `rec`. */
static void race_stats(Carried *cr, uint16_t player, uint32_t rec)
{
    r_rep(cr, P_M | P_X);
    cr->y = player;
    set_p(cr, nz16(cr->p, player));
    r_lda16(cr, rec);
    r_tax16(cr);
    r_sep(cr, P_M);
    (cyc(52), w_PlayerStatsFromRecord(cr));
}

/*
 * @routine 07:DA02 RaceSoundsPlayer1
 * WHAT IT DOES: Player 1's share of a two-player race frame: the soft reset
 * check, both players' stats from their records (PlayerStatsFromRecord, Y =
 * 0 with X = $70:2476, Y = 1 with X = $70:2478), player 1's engine sounds
 * ($1A5D = 0), the race phase (PhaseDispatchE24F); then $0D1A cleared. Its
 * X-press block (RaceXPressPlayer1) is never run: the test loads 0 after its
 * BIT, so the branch always passes it.
 * HOW: Data bank $07, its own; P and the data bank put back.
 * Entry: any widths. Exit: P and the data bank unchanged; A = 0.
 */
void RaceSoundsPlayer1(Carried *cr)
{
    uint8_t p0 = cr->p, db0 = cr->db;
    uint32_t b = 0x070000;

    cr->db = 0x07;
    set_p(cr, nz8(cr->p, 0x07));
    r_sep(cr, P_M | P_X);
    (cyc(140), SoftResetCheck_cr(cr));
    cyc(56);
    race_stats(cr, 0x0000, 0x702476);
    cyc(56);
    race_stats(cr, 0x0001, 0x702478);
    r_sep(cr, P_X);
    cr->y = 0x00;
    set_p(cr, nz8(cr->p, 0x00));
    cyc(40);
    wr8(0x7E0000 | W(current_player), 0x00);
    (cyc(62), EngineSoundsPlayer1_cr(cr));
    (cyc(46), PhaseDispatchE24F(cr));
    race_x_test(cr, b, W(joy1_pressed_copy));
    race_frame_end(cr, b, W(joy1_pressed_copy), 0, p0, db0);
}

void abi_RaceSoundsPlayer1(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RaceSoundsPlayer1(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:DA65 RaceSoundsPlayer2
 * WHAT IT DOES: Player 2's share of a two-player race frame: player 2's
 * engine sounds ($1A5D = 1) and 07:E265; then $0D1C cleared. Its X-press
 * block (RaceXPressPlayer2) is never run, as RaceSoundsPlayer1's.
 * HOW: As RaceSoundsPlayer1.
 * Entry: any widths. Exit: P and the data bank unchanged; A = 0.
 */
void RaceSoundsPlayer2(Carried *cr)
{
    uint8_t p0 = cr->p, db0 = cr->db;
    uint32_t b = 0x070000;

    cr->db = 0x07;
    set_p(cr, nz8(cr->p, 0x07));
    r_sep(cr, P_M | P_X);
    cr->y = 0x01;
    set_p(cr, nz8(cr->p, 0x01));
    cyc(134);
    wr8(0x7E0000 | W(current_player), 0x01);
    (cyc(62), EngineSoundsPlayer2_cr(cr));
    r_sep(cr, P_X);
    cyc(46);
    PlayerTwoFrameWork(cr);
    race_x_test(cr, b, W(joy2_pressed_copy));
    race_frame_end(cr, b, W(joy2_pressed_copy), 0, p0, db0);
}

void abi_RaceSoundsPlayer2(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RaceSoundsPlayer2(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:DA44 RaceXPressPlayer1
 * WHAT IT DOES: RaceSoundsPlayer1's X-press block, which nothing reaches:
 * the palette's first colour $1CC4, $0D22 = $0D23 = $0C, $0D5C = $FF, the
 * sprites parked (OamHighBits18Park48); then $0D1A cleared and the frame's
 * end.
 * HOW: As inside RaceSoundsPlayer1; ends pulling the data bank and P its
 * start pushed.
 * Entry: M 16-bit, X 8-bit, data bank $07; the data bank and P on the
 * stack above the return address. Exit: as RaceSoundsPlayer1.
 */
void RaceXPressPlayer1(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;

    race_x_block(cr, b, 0x0C, race_flag_p1, OamHighBits18Park48_cr);
    race_frame_end(cr, b, W(joy1_pressed_copy), 1, 0, 0);
}

void abi_RaceXPressPlayer1(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RaceXPressPlayer1(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:DA86 RaceXPressPlayer2
 * WHAT IT DOES: RaceSoundsPlayer2's X-press block, which nothing reaches:
 * the palette's first colour $1CC4, $0D22 = $0D23 = $0E, $0D5D = 1, the
 * sprites parked (OamHighBits58Park112); then $0D1C cleared and the frame's
 * end.
 * HOW: As RaceXPressPlayer1.
 * Entry: as RaceXPressPlayer1. Exit: as RaceSoundsPlayer2.
 */
void RaceXPressPlayer2(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;

    race_x_block(cr, b, 0x0E, race_flag_p2, OamHighBits58Park112_cr);
    race_frame_end(cr, b, W(joy2_pressed_copy), 1, 0, 0);
}

void abi_RaceXPressPlayer2(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RaceXPressPlayer2(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void abi_PauseOnStart(void);
void abi_SoundPort0QueuePush(void);
void abi_SoundPort0QueuePushRaw(void);
void abi_SoundPort1QueuePush(void);
void abi_OamHideForHudMode(void);
void abi_GsuBytes200_BFBAB9(void);
void abi_GsuDigitPair0E74At(void);
void abi_GlyphSpritesDrawAll(void);
void abi_WindowSlideUpSound(void);

/* LDA # (8-bit). */
static void p1_lda8(Carried *cr, uint8_t v)
{
    cyc(40);
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    set_p(cr, nz8(cr->p, v));
}

/* CMP (8-bit) of A's low byte. */
static void p1_cmp8(Carried *cr, uint8_t m)
{
    uint8_t a = (uint8_t)cr->a;

    set_p(cr, (uint8_t)((nz8(cr->p, (uint8_t)(a - m)) & ~P_C) | (a >= m ? (cyc(22), P_C) : (cyc(22), 0))));
}

/* INC / DEC abs (8-bit). */
static uint8_t p1_step8(Carried *cr, uint32_t at, int d)
{
    cyc(8);
    uint8_t v = (uint8_t)(rd8(at) + d);

    cyc(6);
    wr8(at, v);
    set_p(cr, nz8(cr->p, v));
    return v;
}

/* INX, INX and CPX # (16-bit). */
static void p1_next(Carried *cr, uint16_t end)
{
    cr->x = (uint16_t)(cr->x + 2);
    set_p(cr, (uint8_t)((nz16(cr->p, (uint16_t)(cr->x - end)) & ~P_C) | (cr->x >= end ? P_C : (cyc(50), 0))));
}

/* One glyph of $0E74 (shift 0 or 4): glyph $1400 + 2 * digit. */
static void p1_digit_glyph(Carried *cr, uint32_t b, unsigned shift, uint16_t at)
{
    unsigned i;

    cyc(24);
    r_lda16(cr, 0x7E0000 | W(wram_0E74));
    for (i = 0; i < shift; i++)
        { cyc(14); r_lsr16(cr); }
    r_and16(cr, 0x000F);
    r_asl16(cr);
    r_add16(cr, 0x1400);
    cyc(98);
    wr16(b | at, cr->a);
}

/*
 * @routine 07:DE6E PlayerOnePhaseRace
 * WHAT IT DOES: The race phase: the pause check (PauseOnStart); while the
 * race time $0E70 runs, a new lap ($0DE8 against $0DEA) plays sound $35 and
 * counts $0DF8 and the lead $0FAE up. At time 0: sounds $02 (raw) and $15,
 * the sprites hidden for the HUD mode, phase $0A ($0DD6, $0D53), $0D5B and
 * $0E5B cleared, and the result's glyph sprites laid out: positions $0FB0
 * and $0FE0 + X for three rows of 7, 8 and 9 glyphs (y $34, $4C, $60, x
 * $38) from the glyphs at $07:DFA2, the two digits of $0E74 as glyphs
 * $1400 + 2n at $1016 and $1014, and the lead's words by $0FAE (0, 1, more).
 * HOW: 8-bit A for the flags, 16-bit for the sprites; ends with RTS.
 * Entry: M 8-bit, X 8-bit; data bank $07. Exit: A, X, Y as the last step
 * left them.
 */
void PlayerOnePhaseRace(Carried *cr)
{
    cyc(46);
    static const struct { uint16_t n, y; } rows[3] = {{7, 0x0034}, {8, 0x004C}, {9, 0x0060}};
    uint32_t b = (uint32_t)cr->db << 16;
    unsigned r;

    PauseOnStart(cr);
    r_rep(cr, P_M);
    cyc(24);
    r_lda16(cr, 0x7E0000 | W(race_time_bcd));
    if (cr->a) {
        r_sep(cr, P_M);
        cyc(62);
        r_lda8(cr, 0x7E0000 | W(player_rec_7a));
        cyc(24);
        p1_cmp8(cr, rd8(0x7E0000 | W(wram_0DEA)));
        if (cr->p & P_Z)
            { cyc(42); return; }
        cyc(18);
        wr8(0x7E0000 | W(wram_0DEA), (uint8_t)cr->a);
        cyc(38);
        p1_lda8(cr, 0x35);
        SoundPort0QueuePush_cr(cr);
        cyc(16);
        p1_step8(cr, 0x7E0000 | W(wram_0DF8), 1);
        cyc(16);
        p1_step8(cr, 0x7E0000 | W(wram_0FAE), 1);
        cyc(42);
        return;
    } else { cyc(28); }
    r_sep(cr, P_M);
    p1_lda8(cr, 0x02);
    SoundPort0QueuePushRaw_cr(cr);
    cyc(30);
    p1_lda8(cr, 0x15);
    SoundPort0QueuePush_cr(cr);
    (cyc(46), OamHideForHudMode_cr(cr));
    p1_lda8(cr, 0x0A);
    wr8(0x7E0000 | W(wram_0DD6), 0x0A);
    cyc(24);
    wr8(0x7E0000 | W(wram_0D53), 0x0A);
    cyc(24);
    wr8(0x7E0000 | W(wram_0D5B), 0x00);
    cyc(24);
    wr8(0x7E0000 | W(wram_0E5B), 0x00);
    r_rep(cr, P_M);
    r_lda16_imm(cr, 0x0004);
    cyc(10);
    wr16(0x7E0000 | W(glyph_spr_x), 0x0004);
    r_lda16_imm(cr, 0x0008);
    cyc(10);
    wr16(0x7E0000 | W(glyph_spr_y), 0x0008);
    r_rep(cr, P_M | P_X);
    cr->x = 0x0002;
    set_p(cr, nz16(cr->p, cr->x));
    for (r = 0; r < 3; r++) {
        cr->y = rows[r].n;
        set_p(cr, nz16(cr->p, cr->y));
        do {
            r_lda16_imm(cr, rows[r].y);
            cyc(64);
            wr16(0x7E0000 | (uint16_t)(W(glyph_spr_y) + cr->x), cr->a);
            r_lda16_imm(cr, 0x0038);
            cyc(16);
            wr16(0x7E0000 | (uint16_t)(W(glyph_spr_x) + cr->x), cr->a);
            cyc(78);
            r_lda16(cr, 0x07DFA0 + cr->x);
            wr16(0x7E0000 | (uint16_t)(W(glyph_spr_glyph) + cr->x), cr->a);
            cr->x = (uint16_t)(cr->x + 2);
            cr->y--;
            set_p(cr, nz16(cr->p, cr->y));
        } while (cyc_loop(16, cr->y));
        cyc(34);
    }
    cyc(24);
    p1_digit_glyph(cr, b, 0, W(wram_1016));
    p1_digit_glyph(cr, b, 4, W(wram_1014));
    cyc(24);
    r_lda16(cr, 0x7E0000 | W(wram_0FAE));
    if (!cr->a) {
        r_lda16_imm(cr, 0x960E);
        cyc(32);
        wr16(0x7E0000 | W(wram_1022), 0x960E);
        r_lda16_imm(cr, 0x9610);
        cyc(10);
        wr16(0x7E0000 | W(wram_1024), 0x9610);
        r_lda16_imm(cr, 0x0000);
        cyc(10);
        wr16(0x7E0000 | W(wram_1026), 0x0000);
        cyc(42);
        return;
    } else { cyc(4); }
    r_cmp16(cr, 0x0002);
    if (cr->p & P_C)
        return;
    r_lda16_imm(cr, 0x9610);
    cyc(62);
    wr16(0x7E0000 | W(wram_1022), 0x9610);
    r_lda16_imm(cr, 0x960E);
    cyc(10);
    wr16(0x7E0000 | W(wram_1024), 0x960E);
    r_lda16_imm(cr, 0x941C);
    cyc(10);
    wr16(0x7E0000 | W(wram_1026), 0x941C);
    r_lda16_imm(cr, 0x0000);
    cyc(10);
    wr16(0x7E0000 | W(wram_102E), 0x0000);
    r_lda16_imm(cr, 0x980E);
    cyc(10);
    wr16(0x7E0000 | W(glyph_spr_1040), 0x980E);
    cyc(64);
}

void abi_PlayerOnePhaseRace(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PlayerOnePhaseRace(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:DFD2 PlayerOnePhaseTally
 * WHAT IT DOES: The result's tally phase: slides the glyph sprites' x
 * ($0FB0 + 2..$30) right by $0B up to their limits at $07:DF72; once the
 * first passes $AC, sets the time $0E70 from $0E74, $0E58 = $40, phase $0E,
 * $0D65 = $0C, $0D6F = 6, $0D6B = $FFFE and $0E74 from $0E6C. Then draws:
 * $70:0210 = $20, GsuBytes200_BFBAB9, the digit pair at the first x and
 * every glyph sprite (GlyphSpritesDrawAll).
 * HOW: 16-bit registers; the limits read with long loads.
 * Entry: M 8-bit, X 8-bit; data bank $07. Exit: as GlyphSpritesDrawAll.
 */
void PlayerOnePhaseTally(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;

    r_rep(cr, P_M | P_X);
    cr->x = 0x0002;
    set_p(cr, nz16(cr->p, cr->x));
    do {
        cyc(54);
        r_lda16(cr, 0x7E0000 | (uint16_t)(W(glyph_spr_x) + cr->x));
        r_add16(cr, 0x000B);
        cyc(134);
        r_cmp16(cr, rd16(0x07DF70 + cr->x));
        if (cr->p & P_C)
            { cyc(42); r_lda16(cr, 0x07DF70 + cr->x); }
        cyc(4);
        wr16(0x7E0000 | (uint16_t)(W(glyph_spr_x) + cr->x), cr->a);
        p1_next(cr, 0x0032);
    } while (!(cr->p & P_C));
    cyc(92);
    r_lda16(cr, 0x7E0000 | W(glyph_spr_x));
    r_add16(cr, 0x000B);
    r_cmp16(cr, 0x00AC);
    if (cr->p & P_C) {
        r_sep(cr, P_M);
        cyc(124);
        r_lda8(cr, 0x7E0000 | W(wram_0E74));
        cyc(24);
        wr8(0x7E0000 | W(race_time_bcd), (uint8_t)cr->a);
        p1_lda8(cr, 0x40);
        wr8(0x7E0000 | W(wram_0E58), 0x40);
        p1_lda8(cr, 0x0E);
        wr8(0x7E0000 | W(wram_0DD6), 0x0E);
        p1_lda8(cr, 0x0C);
        wr8(0x7E0000 | W(time_bonus_state), 0x0C);
        r_rep(cr, P_M);
        r_lda16_imm(cr, 0x0006);
        cyc(10);
        wr16(0x7E0000 | W(wram_0D6F), 0x0006);
        r_lda16_imm(cr, 0xFFFE);
        cyc(10);
        wr16(0x7E0000 | W(wram_0D6B), 0xFFFE);
        cyc(24);
        r_lda16(cr, 0x7E0000 | W(wram_0E6C));
        cyc(24);
        wr16(0x7E0000 | W(wram_0E74), cr->a);
        r_lda16_imm(cr, 0x00AC);
    } else { cyc(98); }
    cyc(10);
    wr16(0x7E0000 | W(glyph_spr_x), cr->a);
    r_lda16_imm(cr, 0x0020);
    cyc(18);
    wr16(0x700210, 0x0020);
    (cyc(62), GsuBytes200_BFBAB9_cr(cr));
    cyc(96);
    r_lda16(cr, 0x7E0000 | W(glyph_spr_x));
    (cyc(46), GsuDigitPair0E74At_cr(cr));
    (cyc(46), GlyphSpritesDrawAll_cr(cr));
    cyc(42);
}

void abi_PlayerOnePhaseTally(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PlayerOnePhaseTally(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:E044 PlayerOnePhaseEnd
 * WHAT IT DOES: The last phase: counts $0E58 down (sound $04 on port 1 at
 * $0A; at 0 $19DF = 1 and WindowSlideUpSound); once it is 0 and the window
 * slide is done ($19D8 = 5), keeps $0E74 at $0E6C, clears $0D4C, sets $0D48
 * = 2, $0D89 from $0D8B and $18DB from $0D8C, and goes on at 07:EC6B.
 * Otherwise draws the glyph sprites (GlyphSpritesDrawAll).
 * HOW: 8-bit A; a long store to $18DB; a JMP to finish.
 * Entry: M 8-bit, X 8-bit; data bank $07. Exit: as GlyphSpritesDrawAll or
 * 07:EC6B leaves it.
 */
void PlayerOnePhaseEnd(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;

    cyc(24);
    r_lda8(cr, 0x7E0000 | W(wram_0E58));
    if ((uint8_t)cr->a) {
        cyc(48);
        p1_cmp8(cr, 0x0A);
        if (cr->p & P_Z) {
            p1_lda8(cr, 0x04);
            cyc(16);
            SoundPort1QueuePush_cr(cr);
            cyc(16);
        }
        if (!p1_step8(cr, 0x7E0000 | W(wram_0E58), -1)) {
            p1_lda8(cr, 0x01);
            cyc(16);
            wr8(0x7E0000 | W(wram_19DF), 0x01);
            (cyc(62), WindowSlideUpSound_cr(cr));
        } else { cyc(22); }
    cyc(12);
        } else {
        cyc(46);
        r_lda8(cr, 0x7E0000 | W(window_hdma_off));
        p1_cmp8(cr, 0x05);
        if (cr->p & P_Z) {
            cyc(40);
            r_lda8(cr, 0x7E0000 | W(wram_0E74));
            cyc(24);
            wr8(0x7E0000 | W(wram_0E6C), (uint8_t)cr->a);
            cyc(24);
            wr8(0x7E0000 | W(digit62_override), 0x00);
            p1_lda8(cr, 0x02);
            wr8(0x7E0000 | W(wram_0D48), 0x02);
            cyc(24);
            r_lda8(cr, 0x7E0000 | W(wram_0D8B));
            cyc(24);
            wr8(0x7E0000 | W(wram_0D89), (uint8_t)cr->a);
            r_rep(cr, P_M);
            cyc(24);
            r_lda16(cr, 0x7E0000 | W(wram_0D8C));
            cyc(32);
            wr16(W(wram_18DB), cr->a);
            cyc(24);
            RaceRestart(cr);
            return;
        }
    }
    cyc(56);
    GlyphSpritesDrawAll_cr(cr);
    cyc(42);
}

void abi_PlayerOnePhaseEnd(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PlayerOnePhaseEnd(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:E08E PlayerOnePhaseReturn
 * WHAT IT DOES: A lone RTS after PlayerOnePhaseEnd's jump, which nothing
 * reaches.
 * HOW: Returns.
 * Entry: any. Exit: nothing changes.
 */
/* PlayerOnePhaseReturn on carried registers: no register work. */
void PlayerOnePhaseReturn_cr(Carried *cr)
{
    (void)cr;
    cyc(42);
}

void abi_PlayerOnePhaseReturn(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PlayerOnePhaseReturn_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:DE46 PlayerOnePhaseStep
 * WHAT IT DOES: Player 1's race phase each frame: the stats from the record
 * (PlayerStatsFromRecord, Y = 0, X = $70:2476), then the handler for the
 * phase $0DD6 from the table after the dispatch: 0, 2 and 4 into 07:E265's
 * handlers ($E285, $E2E4, $E404), 6 and 8 PlayerOnePhaseRace, 10
 * PlayerOnePhaseTally, 12 07:ECA4, 14 PlayerOnePhaseEnd.
 * HOW: JMP (abs,X) through the table of eight words at $07:DE5E; M and X
 * 8-bit for the dispatch.
 * Entry: any widths; data bank $07. Exit: as the handler leaves it.
 */
void PlayerOnePhaseStep(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;
    uint16_t h;

    r_rep(cr, P_M | P_X);
    cr->y = 0x0000;
    set_p(cr, nz16(cr->p, 0));
    cyc(56);
    r_lda16(cr, 0x702476);
    r_tax16(cr);
    r_sep(cr, P_M);
    (cyc(52), w_PlayerStatsFromRecord(cr));
    r_sep(cr, P_M | P_X);
    cyc(24);
    r_lda8(cr, 0x7E0000 | W(wram_0DD6));
    cr->x = (uint8_t)cr->a;
    set_p(cr, nz8(cr->p, (uint8_t)cr->x));
    cyc(60);
    h = rd16(0x07DE5E + cr->x);
    switch (h) {
    case 0xE285:
        RaceStartCountdown(cr);
        cyc(0);
        break;
    case 0xE2E4:
        RaceStartGlyphsSteps(cr);
        cyc(0);
        break;
    case 0xE404:
        ResultsPanelSteps(cr);
        break;
    case 0xDE6E:
        PlayerOnePhaseRace(cr);
        cyc(0);
        break;
    case 0xDFD2:
        PlayerOnePhaseTally(cr);
        break;
    case 0xECA4:
        TimeUpBattle(cr);
        break;
    case 0xE044:
        PlayerOnePhaseEnd(cr);
        break;
    }
    cyc(0);
}

void abi_PlayerOnePhaseStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PlayerOnePhaseStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void abi_PauseOnStart(void);
void abi_SoundPort0QueuePushRaw(void);
void abi_OamHideForHudMode(void);
void abi_GsuF15BTwoLines(void);
void abi_OverlayFramesUntilFaded(void);
void abi_WindowSlideUpSound(void);
void abi_PaletteDarkenFour(void);
void abi_SoundReset(void);
void abi_OamHide75Up(void);
void abi_SaveChecksum(void);

/* LDA $0E3F, TAX, LDA $E115,X, REP #$20, AND #$00FF: the panel step's
   byte for the count $0E3F, through the data bank. */
static void stunt_count_byte(Carried *cr, uint32_t b)
{
    cyc(24);
    r_lda8(cr, 0x7E0000 | W(wram_0E3F));
    cr->x = (uint8_t)cr->a;
    set_p(cr, nz8(cr->p, (uint8_t)cr->x));
    cyc(82);
    r_lda8(cr, b | (uint16_t)(0xE115 + cr->x));
    r_rep(cr, P_M);
    r_and16(cr, 0x00FF);
}

/* DEC $0E3F (8-bit); at 0, $0E41 = 0. */
static void stunt_count_down(Carried *cr, uint32_t b, uint32_t lead)
{
    cyc(lead);
    uint8_t v = (uint8_t)(rd8(0x7E0000 | W(wram_0E3F)) - 1);

    cyc(6);
    wr8(0x7E0000 | W(wram_0E3F), v);
    set_p(cr, nz8(cr->p, v));
    if (!v)
        { cyc(40); wr8(0x7E0000 | W(wram_0E41), 0x00); } else { cyc(22); }
    cyc(42);
}

/*
 * @routine 07:E17D StuntCourseMark
 * WHAT IT DOES: StuntPanelWait's end once all 8 stunts are done: for
 * courses $0C-$0F the course's bit (1, 2, 4, 8) set in the save byte
 * $70:FC06, then SaveChecksum; for any other course nothing (which no run
 * reaches: off the stunt courses the calls before it never return).
 * HOW: 8-bit compares of $0E3D; a long OR into the save.
 * Entry: any widths; data bank $07. Exit: M 8-bit; A low byte the save
 * byte, N, Z from it, C set; or N, Z, C from the compare with $0F.
 */
void StuntCourseMark(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;
    uint8_t v, bit;

    r_sep(cr, P_M);
    cyc(46);
    r_lda8(cr, 0x7E0000 | W(course));
    switch ((uint8_t)cr->a) {
    case 0x0C: bit = 0x01; break;
    case 0x0D: cyc(54); bit = 0x02; break;
    case 0x0E: cyc(86); bit = 0x04; break;
    case 0x0F: cyc(118); bit = 0x08; break;
    default:
        cyc(170);                       /* the four compares, RTS */
        set_p(cr, (uint8_t)((nz8(cr->p, (uint8_t)((uint8_t)cr->a - 0x0F)) & ~P_C) | ((uint8_t)cr->a >= 0x0F ? P_C : 0)));
        return;
    }
    set_p(cr, (uint8_t)(cr->p | (P_C)));  /* the compare that matched */
    cyc(86);
    v = (uint8_t)(rd8(0x70FC06) | bit);
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    set_p(cr, nz8(cr->p, v));
    cyc(32);
    wr8(0x70FC06, v);
    (cyc(62), w_SaveChecksum(cr));
    cyc(42);                            /* RTS */
}

void abi_StuntCourseMark(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    StuntCourseMark(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:E137 StuntPanelWait
 * WHAT IT DOES: The Stunt Trax panel's first state: the pause
 * (PauseOnStart); then, when the stunt count $19ED is new (not 0 and not
 * $1A2D), keeps it in $1A2D and, below 8, shows it (state 4, $0E3F = 15);
 * at 8 (all done), sound 2, CarFinishSound, state 6 with $0E3F = $20,
 * $0E5B and $0D55 cleared, the HUD's sprites hidden (OamHideForHudMode),
 * StuntRecordTime and RecordsLoadSpecialCourseLong, and the course's
 * mark (StuntCourseMark).
 * HOW: 8-bit registers.
 * Entry: any widths; data bank $07. Exit: M and X 8-bit; A as the last
 * step leaves it.
 */
void StuntPanelWait(Carried *cr)
{
    cyc(46);
    uint32_t b = (uint32_t)cr->db << 16;
    uint8_t v, m;

    PauseOnStart(cr);
    r_sep(cr, P_M | P_X);
    cyc(24);
    r_lda8(cr, 0x7E0000 | W(wram_19ED));
    v = (uint8_t)cr->a;
    if (!v)
        { cyc(64); return; }
    cyc(40);
    m = rd8(0x7E0000 | W(wram_1A2D));
    set_p(cr, (uint8_t)((nz8(cr->p, (uint8_t)(v - m)) & ~P_C) | (v >= m ? P_C : 0)));
    if (v == m)
        { cyc(64); return; }            /* BEQ taken, RTS */
    cyc(40);
    wr8(0x7E0000 | W(wram_1A2D), v);
    set_p(cr, (uint8_t)((nz8(cr->p, (uint8_t)(v - 0x08)) & ~P_C) | (v >= 0x08 ? (cyc(62), P_C) : 0)));
    if (v != 0x08) {
        cr->a = (uint16_t)((cr->a & 0xFF00) | 0x04);
        cyc(72);
        wr8(0x7E0000 | W(wram_0E41), 0x04);
        cr->a = (uint16_t)((cr->a & 0xFF00) | 0x0F);
        set_p(cr, nz8(cr->p, 0x0F));
        cyc(40);
        wr8(0x7E0000 | W(wram_0E3F), 0x0F);
        cyc(42);                        /* RTS */
        return;
    }
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0x02);
    set_p(cr, nz8(cr->p, 0x02));
    SoundPort0QueuePushRaw_cr(cr);
    (cyc(38), CarFinishSound(cr));
    r_sep(cr, P_M | P_X);
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0x06);
    cyc(40);
    wr8(0x7E0000 | W(wram_0E41), 0x06);
    cyc(24);
    wr8(0x7E0000 | W(wram_0E5B), 0x00);
    cyc(24);
    wr8(0x7E0000 | W(panel_b_shown), 0x00);
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0x20);
    set_p(cr, nz8(cr->p, 0x20));
    cyc(40);
    wr8(0x7E0000 | W(wram_0E3F), 0x20);
    (cyc(46), OamHideForHudMode_cr(cr));
    cyc(46);
    StuntRecordTime_cr(cr);
    cyc(224);
    RecordsLoadSpecialCourseLong_cr(cr);
    (cyc(122), StuntCourseMark(cr));
}

void abi_StuntPanelWait(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    StuntPanelWait(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:E1AE StuntPanelCount
 * WHAT IT DOES: The Stunt Trax panel's state 2: draws the line for the
 * count $0E3F (the byte at $07:E115 + $0E3F, GsuF15BTwoLines) and counts
 * it down; at 0 the panel goes back to state 0.
 * HOW: The byte through the data bank; 8-bit count.
 * Entry: any widths; data bank $07. Exit: M and X 8-bit; N, Z from the
 * count.
 */
void StuntPanelCount(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;

    stunt_count_byte(cr, b);
    w_GsuF15BTwoLines(cr);
    r_sep(cr, P_M | P_X);
    stunt_count_down(cr, b, 66);
}

void abi_StuntPanelCount(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    StuntPanelCount(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:E1C8 StuntPanelShow
 * WHAT IT DOES: The Stunt Trax panel's state 4: as StuntPanelCount with
 * GlyphRowsAt32 drawing the line.
 * HOW: As StuntPanelCount; M 8-bit after the draw.
 * Entry: any widths; data bank $07. Exit: M 8-bit; X as GlyphRowsAt32
 * leaves it; N, Z from the count.
 */
void StuntPanelShow(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;

    stunt_count_byte(cr, b);
    w_GlyphRowsAt32(cr);
    r_sep(cr, P_M);
    stunt_count_down(cr, b, 46);
}

void abi_StuntPanelShow(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    StuntPanelShow(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:E1E2 StuntPhaseEnd
 * WHAT IT DOES: The Stunt Trax panel's state 8 and the race's phase 8:
 * once the split window is in state 5 ($19D8), clears $0D59, $0E80 and
 * $0E81, waits until the overlay has faded (OverlayFramesUntilFaded) and
 * goes on at RaceRestart with $0DD4 set, else at
 * RaceEndVideoReset.
 * HOW: 8-bit; the two continuations are jumps (jump_routine).
 * Entry: any widths. Exit: M and X 8-bit; A low byte $19D8 with nothing
 * to do; otherwise as the continuation leaves it.
 */
void StuntPhaseEnd(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;
    uint8_t v;

    r_sep(cr, P_M | P_X);
    cyc(24);
    r_lda8(cr, 0x7E0000 | W(window_hdma_off));
    v = (uint8_t)cr->a;
    set_p(cr, (uint8_t)((nz8(cr->p, (uint8_t)(v - 0x05)) & ~P_C) | (v >= 0x05 ? P_C : 0)));
    if (v != 0x05)
        { cyc(80); return; }            /* CMP, BNE taken, RTS */
    cyc(56);
    wr8(0x7E0000 | W(wram_0D59), 0x00);
    cyc(24);
    wr8(0x7E0000 | W(wram_0E80), 0x00);
    cyc(24);
    wr8(0x7E0000 | W(wram_0E81), 0x00);
    (cyc(46), OverlayFramesUntilFaded(cr));
    r_sep(cr, P_M);
    cyc(46);
    r_lda8(cr, 0x7E0000 | W(wram_0DD4));
    if ((uint8_t)cr->a)
        { cyc(40); RaceRestart(cr); }
    else
        { cyc(46); RaceEndVideoReset(cr); }
}

void abi_StuntPhaseEnd(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    StuntPhaseEnd(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:E205 StuntPanelDone
 * WHAT IT DOES: The Stunt Trax panel's state 6: GlyphRowsAt56 with A =
 * $20, then counts $0E3F down; at 0, state 8, $19DF cleared, the split
 * screen on ($0D22 = 1, $0D23 = 0) and the window slid up
 * (WindowSlideUpSound).
 * HOW: 16-bit for the call, 8-bit for the count.
 * Entry: any widths; data bank $07. Exit: M 8-bit, X 16-bit; N, Z from
 * the count, or as WindowSlideUpSound leaves them.
 */
void StuntPanelDone(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;
    uint8_t v;

    r_rep(cr, P_M | P_X);
    r_lda16_imm(cr, 0x0020);
    w_GlyphRowsAt56(cr);
    r_sep(cr, P_M);
    cyc(182);
    v = (uint8_t)(rd8(0x7E0000 | W(wram_0E3F)) - 1);
    cyc(6);
    wr8(0x7E0000 | W(wram_0E3F), v);
    set_p(cr, nz8(cr->p, v));
    if (v)
        { cyc(64); return; }            /* BNE taken, RTS */
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0x08);
    set_p(cr, nz8(cr->p, 0x08));
    cyc(56);
    wr8(0x7E0000 | W(wram_0E41), 0x08);
    r_sep(cr, P_M);
    cyc(46);
    wr8(0x7E0000 | W(wram_19DF), 0x00);
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0x01);
    set_p(cr, nz8(cr->p, 0x01));
    cyc(40);
    wr8(0x7E0000 | W(wram_0D22), 0x01);
    cyc(24);
    wr8(0x7E0000 | W(wram_0D23), 0x00);
    (cyc(62), WindowSlideUpSound_cr(cr));
    cyc(42);                            /* RTS */
}

void abi_StuntPanelDone(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    StuntPanelDone(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:E0B7 StuntPhaseRace
 * WHAT IT DOES: The Stunt Trax race's phases 6 and 8 (and the panel): when
 * the time $0E70 has run out, phase $0C ($0DD6 + Y), $0D55 cleared,
 * $0E5A = $80 and sound $31; with bit 0 of the player's record +$40 set
 * (never in the traces), the screen darkened (PaletteDarkenFour, A =
 * $801F), $0D5B and $0E5B cleared, $0E18 = $F0, phase $0E, $0E56 = $0E4B
 * = $0C, sprites 75 on hidden (OamHide75Up), the sounds reset and sound
 * $37; otherwise the panel's state $0E41 from the table at $07:E12D
 * (StuntPanelWait, StuntPanelCount, StuntPanelShow, StuntPanelDone,
 * StuntPhaseEnd).
 * HOW: 16-bit for the time and the record word, 8-bit stores; JMP
 * (abs,X) for the panel.
 * Entry: M 8-bit, X 8-bit; data bank $07; Y = player (0). Exit: as the
 * step taken leaves it.
 */
void StuntPhaseRace(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;
    uint16_t w;

    r_rep(cr, P_M);
    cyc(24);
    r_lda16(cr, 0x7E0000 | W(race_time_bcd));
    if (!cr->a) {
        r_sep(cr, P_M);
        cr->a = (uint16_t)((cr->a & 0xFF00) | 0x0C);
        cyc(84);
        wr8(0x7E0000 | (uint16_t)(W(engine_phase_p1) + cr->y), 0x0C);
        cyc(24);
        wr8(0x7E0000 | W(panel_b_shown), 0x00);
        cr->a = (uint16_t)((cr->a & 0xFF00) | 0x80);
        cyc(40);
        wr8(0x7E0000 | W(wram_0E5A), 0x80);
        cr->a = (uint16_t)((cr->a & 0xFF00) | 0x31);
        set_p(cr, nz8(cr->p, 0x31));
        (cyc(78), SoundPort0QueuePush_cr(cr));
        cyc(42);
        return;
    }
    r_sep(cr, P_M);
    cyc(68);
    w = (uint16_t)(rd8(0x7E0000 | (uint16_t)(W(player_rec_41) + cr->y)) << 8);
    cyc(44);
    w |= rd8(0x7E0000 | (uint16_t)(W(player_rec_40) + cr->y));
    r_rep(cr, P_M);
    cr->a = (uint16_t)(w & 0x0001);
    set_p(cr, nz16(cr->p, cr->a));
    if (cr->a) {
        cyc(40);
        r_rep(cr, P_M);
        r_lda16_imm(cr, 0x801F);
        (cyc(48), PaletteDarkenFour_cr(cr));
        r_sep(cr, P_M);
        cyc(46);
        wr8(0x7E0000 | W(wram_0D5B), 0x00);
        cr->a = (uint16_t)((cr->a & 0xFF00) | 0xF0);
        cyc(40);
        wr8(0x7E0000 | W(wram_0E18), 0xF0);
        cyc(24);
        wr8(0x7E0000 | W(wram_0E5B), 0x00);
        cr->a = (uint16_t)((cr->a & 0xFF00) | 0x0E);
        cyc(46);
        wr8(0x7E0000 | (uint16_t)(W(engine_phase_p1) + cr->y), 0x0E);
        cr->a = (uint16_t)((cr->a & 0xFF00) | 0x0C);
        set_p(cr, nz8(cr->p, 0x0C));
        cyc(40);
        wr8(0x7E0000 | W(wram_0E56), 0x0C);
        cyc(24);
        wr8(0x7E0000 | W(wram_0E4B), 0x0C);
        (cyc(46), OamHide75Up_cr(cr));
        (cyc(62), SoundReset_cr(cr));
        cr->a = (uint16_t)((cr->a & 0xFF00) | 0x37);
        set_p(cr, nz8(cr->p, 0x37));
        (cyc(78), SoundPort0QueuePush_cr(cr));
        cyc(42);
        return;
    }
    r_sep(cr, P_M | P_X);
    cyc(70);
    cr->x = rd8(0x7E0000 | W(wram_0E41));
    set_p(cr, nz8(cr->p, (uint8_t)cr->x));
    cyc(46);
    switch (rd16(0x070000 | (uint16_t)(0xE12D + cr->x))) {
    case 0xE137: StuntPanelWait(cr); break;
    case 0xE1AE: StuntPanelCount(cr); break;
    case 0xE1C8: StuntPanelShow(cr); break;
    case 0xE205: StuntPanelDone(cr); break;
    case 0xE1E2: StuntPhaseEnd(cr); break;
    }
}

void abi_StuntPhaseRace(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    StuntPhaseRace(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:E08F StuntPhaseStep
 * WHAT IT DOES: The Stunt Trax (HUD mode 1) race phase each frame, as
 * PlayerOnePhaseStep with its own table at $07:E0A7: 0, 2 and 4 into
 * 07:E265's handlers ($E292, $E2E9, $E404), 6 StuntPhaseRace, 8
 * StuntPhaseEnd, 10 $07:EBDC, 12 $07:ECF4, 14 $07:E99D.
 * HOW: As PlayerOnePhaseStep.
 * Entry: any widths; data bank $07. Exit: as the handler leaves it.
 */
void StuntPhaseStep(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;

    r_rep(cr, P_M | P_X);
    cr->y = 0x0000;
    set_p(cr, nz16(cr->p, 0));
    cyc(56);
    r_lda16(cr, 0x702476);
    r_tax16(cr);
    r_sep(cr, P_M);
    (cyc(52), w_PlayerStatsFromRecord(cr));
    r_sep(cr, P_M | P_X);
    cyc(24);
    r_lda8(cr, 0x7E0000 | W(wram_0DD6));
    cr->x = (uint8_t)cr->a;
    set_p(cr, nz8(cr->p, (uint8_t)cr->x));
    cyc(60);
    switch (rd16(0x07E0A7 + cr->x)) {
    case 0xE292: RaceStartCountdownStunt(cr); cyc(0); break;
    case 0xE2E9: RaceStartSteps(cr); cyc(0); break;
    case 0xE404: ResultsPanelSteps(cr); break;
    case 0xE0B7: StuntPhaseRace(cr); break;
    case 0xE1E2: StuntPhaseEnd(cr); break;
    case 0xEBDC: w_PhaseNothingEBDC(cr); break;
    case 0xECF4: TimeUpStunt(cr); cyc(0); break;
    case 0xE99D: RaceEndWaitBigSprites(cr); cyc(42); break;
    }
}

void abi_StuntPhaseStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    StuntPhaseStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void abi_SoftResetCheck(void);
void abi_EngineSoundsPlayer1(void);
void abi_PlayerStatsThenPhaseE232(void);
void abi_PlayerStatsThenPhaseDD35(void);
void abi_TimeBonusTally(void);
void abi_TimeLeftDigits(void);
void abi_TimeBonusTransfer(void);
void abi_HudHoldDraw(void);
void abi_HudTimesDraw(void);
void abi_CoursePaletteCycle(void);
void abi_ChampionshipScreenLoop(void);
void abi_HudLapDigitsDraw(void);
void abi_HudLeadWordDraw(void);

/* PHB, LDA #$7E, PHA, PLB, REP #$30, then the player's car ($0346): its
   link's link's first word, bit 14 picking the word for $0D29 (set or
   clear); SEP #$30, PLB. */
static void hud_car_word(Carried *cr, uint16_t set, uint16_t clear)
{
    uint16_t x, y, w;

    r_rep(cr, P_M | P_X);
    cyc(134);
    x = rd16(0x7E0000 | W(wram_0346));
    cyc(30);
    y = rd16(W(wram_bank_7e) | (uint16_t)(W(obj_link_w) + x));
    cyc(30);
    y = rd16(W(wram_bank_7e) | (uint16_t)(W(obj_link_w) + y));
    cyc(44);
    w = (uint16_t)(rd16(W(wram_bank_7e) | y) & 0x4000);
    cr->x = x;
    cr->y = y;
    w = w ? (cyc(16), set) : clear;
    cr->a = w;
    cyc(102);
    wr16(W(wram_0D29), w);
    r_sep(cr, P_M | P_X);
    set_p(cr, nz8(cr->p, cr->db));
}

/* The end of a HUD mode's frame: REP #$20, the presses cleared ($0D1A,
   and $0D1C with both), then the data bank and P pulled: from the stack
   for an inner entry, else as kept. */
static void hud_end(Carried *cr, int both, int inner, uint8_t p0, uint8_t db0)
{
    uint32_t b = (uint32_t)cr->db << 16;

    r_rep(cr, P_M);
    cyc(4);
    wr16(0x7E0000 | W(joy1_pressed_copy), 0x0000);
    if (both)
        { cyc(24); wr16(0x7E0000 | W(joy2_pressed_copy), 0x0000); }
    if (inner) {
        cpu.s++;
        cyc(20);
        db0 = rd8((uint16_t)cpu.s);
        cpu.s++;
        cyc(20);
        p0 = rd8((uint16_t)cpu.s);
    }
    cr->db = db0;
    set_p(cr, nz8(cr->p, db0));
    r_plp(cr, p0);
    cyc(inner ? 2 : 58);                        /* RTL; PLB, RTL */
}

/*
 * @routine 07:DC47 RaceHudModeTwo
 * WHAT IT DOES: RaceSoundsAndHud's frame for HUD modes 2 and 3: $0D29 =
 * $0214 or $0182 by bit 14 of the player's car's link's link's first
 * word, player 0's stats and phase (PlayerStatsThenPhaseDD35, Y = 0), the
 * time rows (HudTimesDraw) and the course's palette cycle; then both
 * players' presses cleared.
 * HOW: Data bank $7E for the car, then back; ends pulling the data bank
 * and P RaceSoundsAndHud pushed.
 * Entry: M 8-bit, X 8-bit, data bank $07; the data bank and P on the
 * stack above the return address. Exit: P and the data bank pulled; A as
 * the clears leave it (M 16-bit before the PLP).
 */
static void hud_mode_two(Carried *cr, int inner, uint8_t p0, uint8_t db0)
{
    uint8_t db = cr->db;

    r_sep(cr, P_M);
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0x7E);
    cr->db = 0x7E;
    hud_car_word(cr, 0x0214, 0x0182);
    cr->db = db;
    set_p(cr, nz8(cr->p, db));
    cr->y = 0x0000;
    set_p(cr, nz8(cr->p, 0));
    (cyc(90), PlayerStatsThenPhaseDD35(cr));
    (cyc(46), HudTimesDraw_cr(cr));
    (cyc(62), CoursePaletteCycle_cr(cr));
    cyc(20);
    hud_end(cr, 1, inner, p0, db0);
}

void RaceHudModeTwo(Carried *cr)
{
    hud_mode_two(cr, 1, 0, 0);
}

void abi_RaceHudModeTwo(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RaceHudModeTwo(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* HUD mode 0: $0D29 = $0177 or $0166; with $0D24 = 3 and the phase at 8
   or more (never in the traces), the fade out started and, once it is
   done, 07:EC6B; otherwise, with $0D4C set, player 1's phase step, the
   time bonus and digits and its transfer, else the stats and phase
   (PlayerStatsThenPhaseE232), the time bonus and digits, the hold rows and
   the palette cycle; then player 1's presses cleared. */
static void hud_mode_zero(Carried *cr, uint8_t p0, uint8_t db0)
{
    uint32_t b = (uint32_t)cr->db << 16;
    uint8_t db = cr->db, v;
    static const uint8_t pushed_none = 0;

    (void)pushed_none;
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0x7E);
    cr->db = 0x7E;
    hud_car_word(cr, 0x0177, 0x0166);
    cr->db = db;
    set_p(cr, nz8(cr->p, db));
    cyc(52);
    r_lda8(cr, 0x7E0000 | W(wram_0D24));
    v = (uint8_t)cr->a;
    if (v == 0x03) {
        cyc(56);
        r_lda8(cr, 0x7E0000 | W(wram_0DD6));
        if ((uint8_t)cr->a >= 0x08) {
            uint8_t pushed[2];

            cr->a = (uint16_t)((cr->a & 0xFF00) | 0x01);
            cyc(72);
            wr8(0x7E0000 | W(fade_out), 0x01);
            cyc(24);
            r_lda8(cr, 0x7E0000 | W(fade_level_hi));
            if (!((uint8_t)cr->a & 0x80)) {
                cyc(42);                /* BPL taken, and the 20 the others state before */
                hud_end(cr, 0, 0, p0, db0);
                return;
            }
            pushed[0] = p0;
            pushed[1] = db0;
            carried_jump_pushed(cr, 0x07EC6B, pushed, 2);
            return;
        }
    }
    cr->x = 0x0000;
    set_p(cr, nz8(cr->p, 0));
    cyc(78);
    r_lda8(cr, 0x7E0000 | W(digit62_override));
    if ((uint8_t)cr->a) {
        cyc(68);
        PlayerOnePhaseStep(cr);
        (cyc(46), TimeBonusTally_cr(cr));
        (cyc(46), TimeLeftDigits_cr(cr));
        (cyc(46), TimeBonusTransfer_cr(cr));
        cyc(42);
    } else {
        cyc(62);
        PlayerStatsThenPhaseE232(cr);
        (cyc(88), TimeBonusTally_cr(cr));
        (cyc(46), TimeLeftDigits_cr(cr));
        (cyc(46), HudHoldDraw_cr(cr));
        (cyc(62), CoursePaletteCycle_cr(cr));
        cyc(20);
    }
    hud_end(cr, 0, 0, p0, db0);
}

void abi_TimeUpPhaseStep(void);
void abi_HudBlankDraw(void);

/* HUD mode 1 (Stunt Trax): with $0D4E clear, the Stunt Trax phase
   (StuntPhaseStep, Y = 0) and, unless $0E59 is set, the time bonus, its
   digits and transfer; with it set, $0D53 cleared, TimeUpPhaseStep and
   07:A6DD; then both players' presses cleared. */
static void hud_mode_one(Carried *cr, uint8_t p0, uint8_t db0)
{
    uint32_t b = (uint32_t)cr->db << 16;

    cyc(46);
    r_lda8(cr, 0x7E0000 | W(hud_blank));
    if ((uint8_t)cr->a) {
        cyc(46);
        wr8(0x7E0000 | W(wram_0D53), 0x00);
        cr->y = 0x0000;
        set_p(cr, nz8(cr->p, 0));
        (cyc(62), TimeUpPhaseStep(cr));
        cyc(46);
        HudBlankDraw_cr(cr);
        cyc(20);
        hud_end(cr, 1, 0, p0, db0);
        return;
    } else { cyc(78); }
    cr->y = 0x0000;
    set_p(cr, nz8(cr->p, 0));
    StuntPhaseStep(cr);
    r_sep(cr, P_M);
    cyc(46);
    r_lda8(cr, 0x7E0000 | W(wram_0E59));
    if (!(uint8_t)cr->a) {
        cyc(62);
        TimeBonusTally_cr(cr);
        (cyc(46), TimeLeftDigits_cr(cr));
        (cyc(46), TimeBonusTransfer_cr(cr));
    } else { cyc(22); }
    cyc(20);
    hud_end(cr, 1, 0, p0, db0);
}

/*
 * @routine 07:DAA7 RaceSoundsAndHud
 * WHAT IT DOES: The race frame's sounds and HUD: the soft reset check,
 * player 1's engine sounds (EngineSoundsPlayer1, Y = 0), then the HUD
 * mode $0D2B's work from the table at $07:DABE: mode 0 the race (the
 * stats and phase, time bonus and digits, hold rows, palette cycle; or
 * PlayerOnePhaseStep's with $0D4C set), mode 1 Stunt Trax (StuntPhaseStep,
 * or TimeUpPhaseStep with $0D4E set), modes 2 and 3 RaceHudModeTwo.
 * HOW: P and the data bank pushed, data bank $07, 8-bit registers; JMP
 * (abs,X); each mode ends pulling them and returning (RTL).
 * Entry: any widths. Exit: P and the data bank unchanged; A, X, Y as the
 * mode's work leaves them.
 */
void RaceSoundsAndHud(Carried *cr)
{
    uint8_t p0 = cr->p, db0 = cr->db;

    cr->db = 0x07;
    set_p(cr, nz8(cr->p, 0x07));
    r_sep(cr, P_M | P_X);
    (cyc(140), SoftResetCheck_cr(cr));
    cr->y = 0x0000;
    set_p(cr, nz8(cr->p, 0));
    (cyc(78), EngineSoundsPlayer1_cr(cr));
    cyc(24);
    r_lda8(cr, 0x070D2B);
    cr->a = (uint16_t)((cr->a & 0xFF00) | (uint8_t)(cr->a << 1));
    cr->x = (uint8_t)cr->a;
    set_p(cr, nz8(cr->p, (uint8_t)cr->x));
    cyc(52);
    switch (rd16(0x07DABE + cr->x)) {
    case 0xDAC6: hud_mode_zero(cr, p0, db0); break;
    case 0xDB36: hud_mode_one(cr, p0, db0); break;
    case 0xDC47: cyc(22); hud_mode_two(cr, 0, p0, db0); break;
    }
}

void abi_RaceSoundsAndHud(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RaceSoundsAndHud(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:DB97 TimeUpPhaseStart
 * WHAT IT DOES: TimeUpPhaseStep's phase 0: the first two glyph sprites
 * placed ($0FB0/$0FE0 = $DA, $14; $0FB2/$0FE2 = -$62, $40) and the phase
 * word $0DD6 stepped twice.
 * HOW: 16-bit stores and INCs through the data bank.
 * Entry: X 8-bit; data bank $07. Exit: M 16-bit; A = $0040; N, Z from the
 * phase word.
 */
void TimeUpPhaseStart(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;
    uint16_t v;
    int i;

    r_rep(cr, P_M);
    r_lda16_imm(cr, 0x00DA);
    cyc(10);
    wr16(0x7E0000 | W(glyph_spr_x), 0x00DA);
    r_lda16_imm(cr, 0x0014);
    cyc(10);
    wr16(0x7E0000 | W(glyph_spr_y), 0x0014);
    r_lda16_imm(cr, 0xFF9E);
    cyc(10);
    wr16(0x7E0000 | W(wram_0FB2), 0xFF9E);
    r_lda16_imm(cr, 0x0040);
    cyc(10);
    wr16(0x7E0000 | W(wram_0FE2), 0x0040);
    for (i = 0; i < 2; i++) {
        cyc(24);
        v = (uint16_t)(rd16(0x7E0000 | W(wram_0DD6)) + 1);
        cyc(14);
        wr16(0x7E0000 | W(wram_0DD6), v);
        set_p(cr, nz16(cr->p, v));
    }
    cyc(42);
}

void abi_TimeUpPhaseStart(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    TimeUpPhaseStart(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:DBB8 TimeUpPhaseSlideIn
 * WHAT IT DOES: TimeUpPhaseStep's phase 2: 07:A492 (GlyphSpritesSlideIn).
 * HOW: JSR, RTS.
 * Entry: as GlyphSpritesSlideIn. Exit: as GlyphSpritesSlideIn.
 */
void TimeUpPhaseSlideIn(Carried *cr)
{
    cyc(46);
    GlyphSpritesSlideIn_cr(cr);
    cyc(42);                            /* RTS */
}

void abi_TimeUpPhaseSlideIn(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    TimeUpPhaseSlideIn(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:DBBC TimeUpPhaseSlideOut
 * WHAT IT DOES: TimeUpPhaseStep's phase 4: 07:A451 (GlyphSpritesSlideOut).
 * HOW: JSR, RTS.
 * Entry: as GlyphSpritesSlideOut. Exit: as GlyphSpritesSlideOut.
 */
void TimeUpPhaseSlideOut(Carried *cr)
{
    cyc(46);
    GlyphSpritesSlideOut_cr(cr);
    cyc(42);                            /* RTS */
}

void abi_TimeUpPhaseSlideOut(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    TimeUpPhaseSlideOut(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* CMP # (8-bit): N, Z, C as the compare sets them; whether equal. */
static int time_cmp8(Carried *cr, uint8_t m)
{
    uint8_t a = (uint8_t)cr->a;

    set_p(cr, (uint8_t)((nz8(cr->p, (uint8_t)(a - m)) & ~P_C) | (a >= m ? P_C : 0)));
    return a == m;
}

/*
 * @routine 07:DBC0 TimeUpPhaseRace
 * WHAT IT DOES: TimeUpPhaseStep's phase 6: the pause (PauseOnStart); at
 * the time's limit 9:59.99 ($0E66, $0E64, $0E63), $0E5A = $20, phase $0C
 * and sound $31; otherwise, unless $1F71 is set, phase 8, $0D53 = 8,
 * $0E5B cleared, $0E58 = $30, sound $3E, $0592-$0594 = $55 (the sprites'
 * high bits) and the time's record (TimeUpRecordUpdate).
 * HOW: 8-bit compares and stores through the data bank.
 * Entry: any widths; data bank $07. Exit: M 8-bit; A as the last step
 * leaves it.
 */
void TimeUpPhaseRace(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;

    (cyc(46), PauseOnStart(cr));
    r_sep(cr, P_M);
    cyc(46);
    r_lda8(cr, 0x7E0000 | W(race_time_minutes));
    if (time_cmp8(cr, 0x09)) {
        cyc(56);
        r_lda8(cr, 0x7E0000 | W(race_time_seconds));
        if (time_cmp8(cr, 0x59)) {
            cyc(56);
            r_lda8(cr, 0x7E0000 | W(race_time_fraction));
            if (time_cmp8(cr, 0x99)) {
                cr->a = (uint16_t)((cr->a & 0xFF00) | 0x20);
                cyc(78);
                wr8(0x7E0000 | W(wram_0E5A), 0x20);
                cr->a = (uint16_t)((cr->a & 0xFF00) | 0x0C);
                cyc(40);
                wr8(0x7E0000 | W(wram_0DD6), 0x0C);
                cr->a = (uint16_t)((cr->a & 0xFF00) | 0x31);
                set_p(cr, nz8(cr->p, 0x31));
                (cyc(78), SoundPort0QueuePush_cr(cr));
                cyc(42);                /* RTS */
                return;
            }
        }
    }
    cyc(62);
    r_lda8(cr, 0x7E0000 | W(script_byte_1f71));
    if ((uint8_t)cr->a)
        { cyc(64); return; }            /* BNE taken, RTS */
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0x08);
    cyc(56);
    wr8(0x7E0000 | W(wram_0DD6), 0x08);
    cyc(24);
    wr8(0x7E0000 | W(wram_0D53), 0x08);
    cyc(24);
    wr8(0x7E0000 | W(wram_0E5B), 0x00);
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0x30);
    cyc(40);
    wr8(0x7E0000 | W(wram_0E58), 0x30);
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0x3E);
    set_p(cr, nz8(cr->p, 0x3E));
    (cyc(78), SoundPort0QueuePush_cr(cr));
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0x55);
    set_p(cr, nz8(cr->p, 0x55));
    cyc(40);
    wr8(0x7E0000 | W(wram_0592), 0x55);
    cyc(24);
    wr8(0x7E0000 | W(wram_0593), 0x55);
    cyc(24);
    wr8(0x7E0000 | W(wram_0594), 0x55);
    cyc(46);
    TimeUpRecordUpdate_cr(cr);
    cyc(42);                            /* RTS */
}

void abi_TimeUpPhaseRace(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    TimeUpPhaseRace(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* SEP #$20, LDA $0DD4: on at RaceRestart when set, else at
   RaceEndVideoReset (jumps). */
static void race_end_go_on(Carried *cr, uint32_t b)
{
    r_sep(cr, P_M);
    cyc(46);
    r_lda8(cr, 0x7E0000 | W(wram_0DD4));
    if ((uint8_t)cr->a)
        { cyc(40); RaceRestart(cr); }
    else
        { cyc(46); RaceEndVideoReset(cr); }
}

/*
 * @routine 07:DC15 TimeUpPhaseEnd
 * WHAT IT DOES: TimeUpPhaseStep's phase 8: while $0E58 counts down, the
 * lap digits (HudLapDigitsDraw) and, with player 1 behind, the word
 * (HudLeadWordDraw); at 0, $0D59 cleared, the championship screen's loop
 * and on at RaceRestart ($0DD4 set) or RaceEndVideoReset.
 * HOW: 8-bit count through the data bank; the CMP #$20 before the draw
 * branches to the next instruction either way.
 * Entry: any widths; data bank $07. Exit: M 8-bit; as the last step
 * leaves it.
 */
void TimeUpPhaseEnd(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;
    uint8_t v;

    r_sep(cr, P_M);
    cyc(46);
    r_lda8(cr, 0x7E0000 | W(wram_0E58));
    v = (uint8_t)cr->a;
    if (!v) {
        r_sep(cr, P_M);
        cyc(68);
        wr8(0x7E0000 | W(wram_0D59), 0x00);
        (cyc(46), ChampionshipScreenLoop(cr));
        race_end_go_on(cr, b);
        return;
    }
    v = (uint8_t)(v - 1);
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    cyc(54);
    wr8(0x7E0000 | W(wram_0E58), v);
    set_p(cr, (uint8_t)((nz8(cr->p, (uint8_t)(v - 0x20)) & ~P_C) | (v >= 0x20 ? (cyc(16), P_C) : (cyc(22), 0))));
    (cyc(62), HudLapDigitsDraw_cr(cr));      /* CMP, JSR */
    r_sep(cr, P_M);
    cyc(46);
    r_lda8(cr, 0x7E0000 | W(p1_lead));
    if ((uint8_t)cr->a & 0x80) {
        cyc(124);
        HudLeadWordDraw_cr(cr);
        cyc(42);                        /* RTS */
    } else { cyc(64); }                 /* BPL taken, RTS */
}

void abi_TimeUpPhaseEnd(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    TimeUpPhaseEnd(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:DB71 TimeUpPhaseStep
 * WHAT IT DOES: The race phase each frame for Stunt Trax with $0D4E set,
 * as PlayerOnePhaseStep with the table at $07:DB89: 0 TimeUpPhaseStart, 2
 * TimeUpPhaseSlideIn, 4 TimeUpPhaseSlideOut, 6 TimeUpPhaseRace, 8
 * TimeUpPhaseEnd, 10 $07:EBDC, 12 $07:ECA4.
 * HOW: As PlayerOnePhaseStep, with only X 8-bit for the dispatch.
 * Entry: any widths; data bank $07. Exit: as the handler leaves it.
 */
void TimeUpPhaseStep(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;

    r_rep(cr, P_M | P_X);
    cr->y = 0x0000;
    set_p(cr, nz16(cr->p, 0));
    cyc(56);
    r_lda16(cr, 0x702476);
    cr->x = cr->a;
    set_p(cr, nz16(cr->p, cr->x));
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    (cyc(82), w_PlayerStatsFromRecord(cr));     /* TAX, SEP, JSR */
    r_sep(cr, P_X);
    if (cr->p & P_M)
        { cyc(24); r_lda8(cr, 0x7E0000 | W(wram_0DD6)); }
    else
        r_lda16(cr, 0x7E0000 | W(wram_0DD6));
    cr->x = (uint8_t)cr->a;
    set_p(cr, nz8(cr->p, (uint8_t)cr->x));
    cyc(60);
    switch (rd16(0x07DB89 + cr->x)) {
    case 0xDB97: TimeUpPhaseStart(cr); break;
    case 0xDBB8: TimeUpPhaseSlideIn(cr); break;
    case 0xDBBC: TimeUpPhaseSlideOut(cr); break;
    case 0xDBC0: TimeUpPhaseRace(cr); break;
    case 0xDC15: TimeUpPhaseEnd(cr); break;
    case 0xEBDC: w_PhaseNothingEBDC(cr); break;
    case 0xECA4: TimeUpBattle(cr); break;
    }
}

void abi_TimeUpPhaseStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    TimeUpPhaseStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void abi_RaceEndCountdownFadeOn(void);
void abi_RaceEndCountdownOn(void);

/* LDA $0D52 until set (the vertical blank), then clear it. */
static void race_end_vblank(Carried *cr, uint32_t b, uint32_t branch)
{
    cyc(branch);                        /* the branch back to the top, if any */
    do {
        r_sep(cr, P_M);
        cyc(46);
        r_lda8(cr, 0x7E0000 | W(wram_0D52));
    } while (cyc_loop(22, !(uint8_t)cr->a));
    cyc(40);
    wr8(0x7E0000 | W(wram_0D52), 0x00);
}

/*
 * @routine 07:DC88 RaceEndCountdownFade
 * WHAT IT DOES: The race's end, frame by frame: counts $0E56 down; at 0 in
 * HUD mode 0 starts the fade out ($0E3A = 1) and, once it is done ($0D61
 * negative), goes on at RaceEndLeave; in other modes $0D59
 * cleared, sound $30, the championship screen's loop and on at
 * RaceRestart ($0DD4 set) or RaceEndVideoReset.
 * HOW: Waits for $0D52 each frame; 8-bit through the data bank.
 * Entry: any widths; data bank $07. Exit: does not return (jumps on).
 */
void RaceEndCountdownFade(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;
    uint8_t v;
    uint32_t branch = 0;

    for (;;) {
        race_end_vblank(cr, b, branch);
        branch = 22;
        cyc(24);
        r_lda8(cr, 0x7E0000 | W(wram_0E56));
        v = (uint8_t)cr->a;
        if (!v) {
            cyc(46);
            r_lda8(cr, 0x7E0000 | W(fade_level_hi));
            if ((uint8_t)cr->a & 0x80) {
                cyc(40);
                RaceEndLeave(cr);
                return;
            }
            continue;
        }
        v = (uint8_t)(v - 1);
        cr->a = (uint16_t)((cr->a & 0xFF00) | v);
        set_p(cr, nz8(cr->p, v));
        cyc(54);
        wr8(0x7E0000 | W(wram_0E56), v);
        if (v)
            continue;
        cyc(40);
        r_lda8(cr, 0x7E0000 | W(hud_mode));
        if ((uint8_t)cr->a) {
            cyc(40);
            wr8(0x7E0000 | W(wram_0D59), 0x00);
            cr->a = (uint16_t)((cr->a & 0xFF00) | 0x30);
            set_p(cr, nz8(cr->p, 0x30));
            (cyc(24), SoundPort0QueuePushRaw_cr(cr));
            (cyc(38), ChampionshipScreenLoop(cr));    /* JSR, less the push's pulls it states */
            RaceEndCountdownFadeOn(cr);
            return;
        }
        cr->a = (uint16_t)((cr->a & 0xFF00) | 0x01);
        set_p(cr, nz8(cr->p, 0x01));
        cyc(62);
        wr8(0x7E0000 | W(fade_out), 0x01);
    }
}

void abi_RaceEndCountdownFade(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RaceEndCountdownFade(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:DCCA RaceEndCountdown
 * WHAT IT DOES: The race's end, frame by frame: counts $0E5A down; at 0,
 * $0D59 cleared, the championship screen's loop and on at
 * RaceRestart ($0DD4 set) or RaceEndVideoReset.
 * HOW: Waits for $0D52 each frame; 8-bit through the data bank.
 * Entry: any widths; data bank $07. Exit: does not return (jumps on).
 */
void RaceEndCountdown(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;
    uint8_t v;
    uint32_t branch = 0;

    do {
        race_end_vblank(cr, b, branch);
        branch = 0;
        cyc(24);
        v = (uint8_t)(rd8(0x7E0000 | W(wram_0E5A)) - 1);
        cyc(6);
        wr8(0x7E0000 | W(wram_0E5A), v);
        set_p(cr, nz8(cr->p, v));
    } while (cyc_loop(22, v));
    cyc(40);
    wr8(0x7E0000 | W(wram_0D59), 0x00);
    (cyc(46), ChampionshipScreenLoop(cr));
    RaceEndCountdownOn(cr);
}

void abi_RaceEndCountdown(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RaceEndCountdown(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:DCAE RaceEndCountdownFadeOn
 * WHAT IT DOES: RaceEndCountdownFade's last step as an entry: on at
 * RaceRestart with $0DD4 set, else at RaceEndVideoReset.
 * HOW: SEP #$20, LDA $0DD4, then the jump.
 * Entry: any widths; data bank $07. Exit: does not return (jumps on).
 */
void RaceEndCountdownFadeOn(Carried *cr)
{
    race_end_go_on(cr, (uint32_t)cr->db << 16);
}

void abi_RaceEndCountdownFadeOn(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RaceEndCountdownFadeOn(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:DCDF RaceEndCountdownOn
 * WHAT IT DOES: RaceEndCountdown's last step as an entry: on at
 * RaceRestart with $0DD4 set, else at RaceEndVideoReset.
 * HOW: SEP #$20, LDA $0DD4, then the jump.
 * Entry: any widths; data bank $07. Exit: does not return (jumps on).
 */
void RaceEndCountdownOn(Carried *cr)
{
    race_end_go_on(cr, (uint32_t)cr->db << 16);
}

void abi_RaceEndCountdownOn(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RaceEndCountdownOn(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* GsuRecordBits60 as its adapter leaves the registers. */
static void w_GsuRecordBits60(Carried *cr)
{
    cr->a = GsuRecordBits60();
}

/* PlayerStatsFromRecord as its adapter leaves the registers. */
static void w_PlayerStatsFromRecord(Carried *cr)
{
    PlayerStatsFromRecord(cr);
}

/* SaveChecksum as its adapter leaves the registers. */
static void w_SaveChecksum(Carried *cr)
{
    cr->a = (uint16_t)((cr->a & 0xFF00) | SaveChecksum());
}

/* GsuF15BTwoLines as its adapter leaves the registers. */
static void w_GsuF15BTwoLines(Carried *cr)
{
    cr->a = (uint16_t)(0x8C00 | GsuF15BTwoLines(cr->a));
    cr->x = 0xF15B;
    set_p(cr, (uint8_t)(nz8(cr->p, cr->db) & ~(P_M | P_X)));
}

/* GlyphRowsAt32 as its adapter leaves the registers. */
static void w_GlyphRowsAt32(Carried *cr)
{
    cr->a = (uint16_t)(0x8400 | GlyphRowsAt32(cr->a));
    cr->x = 0xF15B;
    set_p(cr, (uint8_t)(nz8(cr->p, cr->db) & ~(P_M | P_X)));
}

/* GlyphRowsAt56 as its adapter leaves the registers. */
static void w_GlyphRowsAt56(Carried *cr)
{
    cr->a = (uint16_t)(0x8800 | GlyphRowsAt56(cr->a));
    cr->x = 0xF15B;
    set_p(cr, (uint8_t)(nz8(cr->p, cr->db) & ~(P_M | P_X)));
}

/* PhaseNothingEBDC on the carried registers. */
static void w_PhaseNothingEBDC(Carried *cr)
{
    PhaseNothingEBDC_cr(cr);
}
