/*
 * player_stats.c: the per-player race statistics the HUD and the results
 * show, taken each frame from the player car's Super FX record, and the
 * race phase dispatchers that run after them.
 */
#include "machine.h"
#include "hw.h"
#include "cpu_ops.h"

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
        speed = (uint16_t)(~speed + 1);
    wr8(b | M7A, (uint8_t)speed);
    wr8(b | M7A, (uint8_t)(speed >> 8));
    wr8(b | M7B, 0x3C);
    m = rd16(b | MPYM);
    v = m < 0x00FF ? (uint8_t)m : 0xFF;
    wr8(b | (uint16_t)(0x0DDA + player), v);
    sum = player == 0 ? 0x0DDC : 0x0DE0;
    cnt = player == 0 ? 0x0DE4 : 0x0DE6;
    s = (unsigned)v + rd16(b | sum);
    lo = (uint16_t)s;
    wr16(b | sum, lo);
    hi = rd16(b | (uint16_t)(sum + 2));
    h = (unsigned)hi + (s >> 16);
    *p = (uint8_t)((*p & ~(P_V | P_C)) | ((~hi & (uint16_t)h & 0x8000) ? P_V : 0));
    if (h > 0xFFFF) {
        wr16(b | sum, 0xFFFF);
        wr16(b | (uint16_t)(player == 0 ? sum + 2 : sum), 0xFFFF);
        *a = 0xFFFF;
        *p = (uint8_t)(nz16(*p, 0xFFFF) | P_C);
        return;
    }
    wr16(b | (uint16_t)(sum + 2), (uint16_t)h);
    *a = (uint16_t)h;
    m = (uint16_t)(rd16(b | cnt) + 1);
    wr16(b | cnt, m);
    *p = nz16(*p, m);
}

void abi_SpeedSampleAdd(void)
{
    uint8_t p = cpu.p;

    SpeedSampleAdd(cpu.db, cpu.a, cpu.y, &p, &cpu.a);
    cpu.p = (uint8_t)(p & ~P_M);
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
void PlayerStatsFromRecord(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint16_t x = cpu.x, y = cpu.y, w;
    uint8_t v;

    v = rd8(b | 0x0E57);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, v);
    if (!v) {
        wr8(b | (uint16_t)(0x0DEC + y), rd8(0x700040 + x));
        wr8(b | (uint16_t)(0x0DEE + y), rd8(0x700041 + x));
        wr8(b | (uint16_t)(0x0DF0 + y), rd8(0x700042 + x));
        wr8(b | (uint16_t)(0x0DF2 + y), rd8(0x700043 + x));
        wr8(b | (uint16_t)(0x0DF6 + y), rd8(0x70004E + x));
        w = rd16(0x700082 + x) >> 3;
        v = (uint8_t)w;
        wr8(b | (uint16_t)(0x0DF9 + y), v);
        if (v >= 0x3F)
            v = 0x3F;
        wr8(b | (uint16_t)(0x0DFB + y), (uint8_t)((v << 1) + 0x30));
        cpu.a = rd16(0x70004A + x);
        cpu.p &= (uint8_t)~P_M;
        abi_SpeedSampleAdd();
        w = rd16(0x70006A + x);
        cpu.p = (uint8_t)((cpu.p & ~P_C) | (w & 0x0004 ? P_C : 0));
        w >>= 3;
        wr8(b | (uint16_t)(0x0E05 + y), (uint8_t)w);
        wr8(b | (uint16_t)(0x0DE8 + y), rd8(0x70007A + x));
        v = (uint8_t)(rd8(0x700088 + x) & 0x0F);
        wr8(b | (uint16_t)(0x0DF4 + y), v);
        cpu.a = (uint16_t)((w & 0xFF00) | v);
        cpu.p = (uint8_t)(nz8(cpu.p, v) | P_M);
    }
    abi_GsuRecordBits60();
}

void abi_PlayerStatsFromRecord(void)
{
    PlayerStatsFromRecord();
}

/* Player 0's figures from its record at $70:2476: Y = 0, X = the record,
   M 8-bit, X 16-bit. */
static void stats_player0(void)
{
    uint16_t r;

    cpu.p &= (uint8_t)~(P_M | P_X);
    cpu.y = 0x0000;
    r = rd16(0x702476);
    cpu.a = r;
    cpu.x = r;
    cpu.p |= P_M;
    PlayerStatsFromRecord();
}

/*
 * @routine 07:DD47 PlayerStatsAndPaletteAnimate
 * WHAT IT DOES: Player 0's figures from its record (PlayerStatsFromRecord),
 * then $0E5F = $FF.
 * HOW: P saved and restored.
 * Entry: any widths, D = $0000. Exit: A low byte = $FF; X = the record;
 * Y = 0; P unchanged.
 */
void PlayerStatsAndPaletteAnimate(void)
{
    uint8_t p = cpu.p;

    stats_player0();
    cpu.p |= P_M;
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0xFF);
    wr8(((uint32_t)cpu.db << 16) | 0x0E5F, 0xFF);
    cpu.p = p;
}

void abi_PlayerStatsAndPaletteAnimate(void)
{
    PlayerStatsAndPaletteAnimate();
}

void abi_PlayerStatsFromRecord(void);

/* LDA $0DD6 (in the current width), TAX, JMP (table,X): the race phase's
   handler, reached as original code; it returns to the caller, or through
   the caller's frame. */
static void phase_jump(uint16_t table)
{
    uint32_t b = (uint32_t)cpu.db << 16;

    if (cpu.p & P_M)
        op_lda8(b | 0x0DD6);
    else
        op_lda16(b | 0x0DD6);
    cpu.x = (cpu.p & P_X) ? (uint16_t)(cpu.a & 0x00FF) : cpu.a;
    cpu.p = (cpu.p & P_X) ? nz8(cpu.p, (uint8_t)cpu.x) : nz16(cpu.p, cpu.x);
    jump_routine(0x070000 | rd16(0x070000 | (uint16_t)(table + cpu.x)));
}

/*
 * @routine 07:E22B PhaseDispatchE232
 * WHAT IT DOES: Runs the handler for race phase $0DD6 from the table at
 * $07:E232.
 * HOW: JMP (table,X); the handler returns to the caller, or through its
 * caller's frame (jump_routine).
 * Entry: any widths. Exit: as the handler leaves it.
 */
void abi_PhaseDispatchE232(void)
{
    phase_jump(0xE232);
}

/*
 * @routine 07:E246 PhaseDispatchE24F
 * WHAT IT DOES: Runs the handler for race phase $0DD6 from the table at
 * $07:E24F, with 8-bit registers.
 * HOW: SEP #$30, then JMP (table,X).
 * Entry: any widths. Exit: as the handler leaves it.
 */
void abi_PhaseDispatchE24F(void)
{
    op_sep(P_M | P_X);
    phase_jump(0xE24F);
}

/* REP #$30, LDY #0, LDA.L $70:2476, TAX, SEP #$20, JSR
   PlayerStatsFromRecord, SEP #$10: player 0's figures. */
static void player0_stats(void)
{
    op_rep(P_M | P_X);
    cpu.y = 0;
    cpu.p = nz16(cpu.p, 0);
    op_lda16(0x702476);
    op_tax16();
    op_sep(P_M);
    abi_PlayerStatsFromRecord();
    op_sep(P_X);
}

/*
 * @routine 07:DCEC PlayerStatsThenPhaseE232
 * WHAT IT DOES: Player 0's figures from its record, then the race phase
 * handler from the table at $07:E232 with Y = 0.
 * HOW: PlayerStatsFromRecord, SEP #$10, then PhaseDispatchE232.
 * Entry: any widths, D = $0000. Exit: as the phase handler leaves it.
 */
void abi_PlayerStatsThenPhaseE232(void)
{
    player0_stats();
    cpu.y = 0;
    cpu.p = nz8(cpu.p, 0);
    abi_PhaseDispatchE232();
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
void abi_PlayerStatsThenPhaseDD35(void)
{
    uint32_t b;
    uint8_t v;

    player0_stats();
    b = (uint32_t)cpu.db << 16;
    op_lda8(b | 0x10CD);
    if ((uint8_t)cpu.a) {
        op_lda8(b | 0x0DD6);
        v = (uint8_t)cpu.a;
        cpu.p = (uint8_t)((nz8(cpu.p, (uint8_t)(v - 0x08)) & ~P_C) | (v >= 0x08 ? P_C : 0));
        if (v >= 0x08) {
            cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x01);
            cpu.p = nz8(cpu.p, 0x01);
            wr8(b | 0x0E3A, 0x01);
            op_lda8(b | 0x0D61);
            if (!((uint8_t)cpu.a & 0x80))
                return;
            jump_routine(0x07EBE9);
            return;
        }
    }
    phase_jump(0xDD35);
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
static void race_x_block(uint32_t b, uint8_t stage, void (*flag)(uint32_t), void (*park)(void))
{
    cpu.a = 0x1CC4;
    cpu.p = nz16(cpu.p, cpu.a);
    wr16(0x7EFD65, 0x1CC4);
    op_sep(P_M);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | stage);
    cpu.p = nz8(cpu.p, stage);
    wr8(b | 0x0D22, stage);
    wr8(b | 0x0D23, stage);
    flag(b);
    park();
    op_rep(P_M);
}

/* The end of a player's frame: `pressed` cleared, then the data bank and
   P pulled: from the stack for an inner entry, else as kept. */
static void race_frame_end(uint32_t b, uint16_t pressed, int inner, uint8_t p0, uint8_t db0)
{
    wr16(b | pressed, 0x0000);
    if (inner) {
        cpu.s++;
        db0 = rd8((uint16_t)cpu.s);
        cpu.s++;
        p0 = rd8((uint16_t)cpu.s);
    }
    cpu.db = db0;
    cpu.p = nz8(cpu.p, db0);
    op_plp(p0);
}

/* The test before the X-press block: LDA pressed, BIT #$40, LDA #0. */
static void race_x_test(uint32_t b, uint16_t pressed)
{
    op_rep(P_M);
    op_lda16(b | pressed);
    op_lda16_imm(0x0000);
}

static void race_flag_p1(uint32_t b)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0xFF);
    cpu.p = nz8(cpu.p, 0xFF);
    wr8(b | 0x0D5C, 0xFF);
}

static void race_flag_p2(uint32_t b)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x01);
    cpu.p = nz8(cpu.p, 0x01);
    wr8(b | 0x0D5D, 0x01);
}

/* A player's stats: Y = player, X = the record word at `rec`. */
static void race_stats(uint16_t player, uint32_t rec)
{
    op_rep(P_M | P_X);
    cpu.y = player;
    cpu.p = nz16(cpu.p, player);
    op_lda16(rec);
    op_tax16();
    op_sep(P_M);
    abi_PlayerStatsFromRecord();
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
void abi_RaceSoundsPlayer1(void)
{
    uint8_t p0 = cpu.p, db0 = cpu.db;
    uint32_t b = 0x070000;

    cpu.db = 0x07;
    cpu.p = nz8(cpu.p, 0x07);
    op_sep(P_M | P_X);
    abi_SoftResetCheck();
    race_stats(0x0000, 0x702476);
    race_stats(0x0001, 0x702478);
    op_sep(P_X);
    cpu.y = 0x00;
    cpu.p = nz8(cpu.p, 0x00);
    wr8(b | 0x1A5D, 0x00);
    abi_EngineSoundsPlayer1();
    abi_PhaseDispatchE24F();
    race_x_test(b, 0x0D1A);
    race_frame_end(b, 0x0D1A, 0, p0, db0);
}

/*
 * @routine 07:DA65 RaceSoundsPlayer2
 * WHAT IT DOES: Player 2's share of a two-player race frame: player 2's
 * engine sounds ($1A5D = 1) and 07:E265; then $0D1C cleared. Its X-press
 * block (RaceXPressPlayer2) is never run, as RaceSoundsPlayer1's.
 * HOW: As RaceSoundsPlayer1.
 * Entry: any widths. Exit: P and the data bank unchanged; A = 0.
 */
void abi_RaceSoundsPlayer2(void)
{
    uint8_t p0 = cpu.p, db0 = cpu.db;
    uint32_t b = 0x070000;

    cpu.db = 0x07;
    cpu.p = nz8(cpu.p, 0x07);
    op_sep(P_M | P_X);
    cpu.y = 0x01;
    cpu.p = nz8(cpu.p, 0x01);
    wr8(b | 0x1A5D, 0x01);
    abi_EngineSoundsPlayer2();
    op_sep(P_X);
    call_routine(0x07E265, 0);
    race_x_test(b, 0x0D1C);
    race_frame_end(b, 0x0D1C, 0, p0, db0);
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
void abi_RaceXPressPlayer1(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;

    race_x_block(b, 0x0C, race_flag_p1, abi_OamHighBits18Park48);
    race_frame_end(b, 0x0D1A, 1, 0, 0);
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
void abi_RaceXPressPlayer2(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;

    race_x_block(b, 0x0E, race_flag_p2, abi_OamHighBits58Park112);
    race_frame_end(b, 0x0D1C, 1, 0, 0);
}
