/*
 * player_stats.c: the per-player race statistics the HUD and the results
 * show, taken each frame from the player car's Super FX record, and the
 * race phase dispatchers that run after them.
 */
#include "machine.h"
#include "hw.h"

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
