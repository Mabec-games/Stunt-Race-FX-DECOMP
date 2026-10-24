/*
 * sound_effects.c: sound commands the game sends for each player's car.
 *
 * Most effects have one command for player 1 and another for player 2; the
 * routines here pick by the player whose car is being processed. A car can
 * also hold one looping sound, remembered per player so it is only re-sent
 * when it changes.
 */
#include "machine.h"
#include "cpu_ops.h"
#include "carried.h"
#include "wram_names.h"
#include "wram_struct.h"


void SoundPort1QueuePush(uint8_t command);
void SoundPort1QueuePushW(uint8_t command, unsigned wide);
void SoundPort0QueuePushW(uint8_t command, unsigned wide);

/* Start a held sound: announce it first if nothing was held, then send it. */
static void held_sound_set(uint32_t held, uint8_t announce, uint8_t sound)
{
    cyc(76);
    if (rd8(held) == 0)
        { cyc(94); SoundPort1QueuePush(announce); } else { cyc(22); }
    cyc(60);
    wr8(held, sound);
    cyc(62);
    SoundPort1QueuePush(sound);
}

/* Hold sound p1 (player 1) or p2 (player 2) unless it is already held.
   Returns the command chosen, which is what the original leaves in A. */
static uint8_t player_hold_sound(uint8_t p1, uint8_t p2)
{
    cyc(107);
    if (RD8(current_player) == 0) {
        cyc(64);
        if (RD8(player1_held_sound) != p1)
            held_sound_set(W(player1_held_sound), 0x09, p1); else { cyc(16); }
        cyc(72);
        return p1;
    }
    cyc(70);
    if (RD8(player2_held_sound) != p2)
        held_sound_set(W(player2_held_sound), 0x14, p2); else { cyc(16); }
    cyc(72);
    return p2;
}

/*
 * @routine 04:D463 PlayerHoldSound32
 * WHAT IT DOES: Makes the current player's car hold sound $32 (player 1) or
 * $36 (player 2).
 * HOW: If that sound is not already the player's held sound: when the player
 * held nothing, first sends $09 (player 1) or $14 (player 2) on port 1; then
 * records the sound as held and sends it on port 1.
 * Entry: M any, DB a bank where $1A5D is work RAM. Exit: A low byte = the
 * sound chosen; P unchanged.
 */
void PlayerHoldSound32(void)
{
    player_hold_sound(0x32, 0x36);
}

void EngineSoundsPlayer1_cr(Carried *cr);
void EngineSoundsPlayer2_cr(Carried *cr);
void HandlerSoundIndex_cr(Carried *cr);
void NearCarSoundUpdate_cr(Carried *cr);
void PlayerHoldSound32_cr(Carried *cr);
void PlayerHoldSound33_cr(Carried *cr);
void PlayerHoldSound34_cr(Carried *cr);
void PlayerHoldSound35_cr(Carried *cr);
void PlayerSound0B_cr(Carried *cr);
void PlayerSound0C_cr(Carried *cr);
void PlayerSound0D_cr(Carried *cr);
void PlayerSound12_cr(Carried *cr);
void PlayerSound23_cr(Carried *cr);
void PlayerSound24_cr(Carried *cr);
void PlayerSound35_cr(Carried *cr);
void PlayerStopHeldSound_cr(Carried *cr);
void Sound0E1AStop_cr(Carried *cr);
void SoundAtRateForLevel_cr(Carried *cr);
void SoundFromTableD89_cr(Carried *cr);
void SoundHoldByFlags_cr(Carried *cr);
void SoundIfFlag40_cr(Carried *cr);
void SoundIndicesFromHandlers_cr(Carried *cr);
void SoundNearCarOnce_cr(Carried *cr);
void SoundPairFromTable_cr(Carried *cr);
void WindowSlideUpSound_cr(Carried *cr);
static void engine_sounds_exit(Carried *cr, uint16_t a);

void PlayerHoldSound32_cr(Carried *cr)
{
    cr->a = (uint16_t)((cr->a & 0xFF00) | player_hold_sound(0x32, 0x36));
}

void abi_PlayerHoldSound32(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PlayerHoldSound32_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 04:D47F PlayerHoldSound33
 * WHAT IT DOES: Makes the current player's car hold sound $33 (player 1) or
 * $37 (player 2).
 * HOW: As PlayerHoldSound32.
 * Entry: as PlayerHoldSound32. Exit: as PlayerHoldSound32.
 */
void PlayerHoldSound33(void)
{
    player_hold_sound(0x33, 0x37);
}

void PlayerHoldSound33_cr(Carried *cr)
{
    cr->a = (uint16_t)((cr->a & 0xFF00) | player_hold_sound(0x33, 0x37));
}

void abi_PlayerHoldSound33(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PlayerHoldSound33_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 04:D49B PlayerStopHeldSound
 * WHAT IT DOES: Stops the current player's held sound, if any.
 * HOW: If the player holds a sound, clears it and sends $0A (player 1) or
 * $1A (player 2) on port 1.
 * Entry: M any, DB a bank where $1A5D is work RAM. Exit: A low byte = the
 * stop command, or 0 when nothing was held; P unchanged.
 */
static uint8_t player_stop_held_sound(void)
{
    cyc(68);
    uint8_t two = RD8(current_player) != 0;
    uint32_t held = two ? W(player2_held_sound) : W(player1_held_sound);
    uint8_t stop = two ? (cyc(6), 0x1A) : 0x0A;

    cyc(48);
    if (rd8(held) == 0)
        { cyc(22); return 0; }
    cyc(64);
    wr8(held, 0);
    cyc(78);
    SoundPort1QueuePush(stop);
    return stop;
}

void PlayerStopHeldSound(void)
{
    player_stop_held_sound();
}

void PlayerStopHeldSound_cr(Carried *cr)
{
    cr->a = (uint16_t)((cr->a & 0xFF00) | player_stop_held_sound());
    cyc(72);
}

void abi_PlayerStopHeldSound(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PlayerStopHeldSound_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 04:D4FB PlayerHoldSound34
 * WHAT IT DOES: Makes the current player's car hold sound $34 (player 1) or
 * $38 (player 2).
 * HOW: As PlayerHoldSound32.
 * Entry: as PlayerHoldSound32. Exit: as PlayerHoldSound32.
 */
void PlayerHoldSound34(void)
{
    player_hold_sound(0x34, 0x38);
}

void PlayerHoldSound34_cr(Carried *cr)
{
    cr->a = (uint16_t)((cr->a & 0xFF00) | player_hold_sound(0x34, 0x38));
}

void abi_PlayerHoldSound34(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PlayerHoldSound34_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 04:D517 PlayerHoldSound35
 * WHAT IT DOES: Makes the current player's car hold sound $35 (player 1) or
 * $39 (player 2).
 * HOW: As PlayerHoldSound32.
 * Entry: as PlayerHoldSound32. Exit: as PlayerHoldSound32.
 */
void PlayerHoldSound35(void)
{
    player_hold_sound(0x35, 0x39);
}

void PlayerHoldSound35_cr(Carried *cr)
{
    cr->a = (uint16_t)((cr->a & 0xFF00) | player_hold_sound(0x35, 0x39));
}

void abi_PlayerHoldSound35(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PlayerHoldSound35_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 04:D5A2 Sound0E1AStop
 * WHAT IT DOES: Ends the sound marked by $0E1A: clears the mark and sends $08
 * on port 1, if the mark was set.
 * HOW: Tests $0E1A; when nonzero, stores 0 there and queues $08 for port 1.
 * Entry: M 8-bit, DB a bank where $0E1A is work RAM. Exit: A low byte = $08,
 * or 0 when the mark was clear; P unchanged.
 */
static uint8_t sound_0e1a_stop(void)
{
    cyc(46);
    if (RD8(wram_0E1A) == 0)
        { cyc(72); return 0; }
    cyc(40);
    WR8(wram_0E1A, 0);
    cyc(78);
    SoundPort1QueuePush(0x08);
    cyc(50);
    return 0x08;
}

uint8_t Sound0E1AStop(void)
{
    return sound_0e1a_stop();
}

void Sound0E1AStop_cr(Carried *cr)
{
    cr->a = (uint16_t)((cr->a & 0xFF00) | sound_0e1a_stop());
    cyc(22);
}

void abi_Sound0E1AStop(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    Sound0E1AStop_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* Send p1 (player 1) or p2 (player 2) on port 1; returns the one sent. */
static uint8_t player_sound(uint8_t p1, uint8_t p2)
{
    cyc(208);
    uint8_t command = RD8(current_player) != 0 ? (cyc(100), p2) : (cyc(94), p1);
    SoundPort1QueuePush(command);
    return command;
}

/*
 * @routine 04:D5B3 PlayerSound23
 * WHAT IT DOES: Sends sound $23 (player 1) or $25 (player 2) on port 1 for
 * the player whose car is being processed.
 * HOW: Picks the command by $1A5D and queues it for port 1.
 * Entry: any widths. Exit: A low byte = the command sent; P unchanged.
 */
void PlayerSound23(void)
{
    player_sound(0x23, 0x25);
}

void PlayerSound23_cr(Carried *cr)
{
    cr->a = (uint16_t)((cr->a & 0xFF00) | player_sound(0x23, 0x25));
    cyc(72);
}

void abi_PlayerSound23(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PlayerSound23_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 04:D5CC PlayerSound24
 * WHAT IT DOES: Sends sound $24 (player 1) or $26 (player 2) on port 1.
 * HOW: As PlayerSound23.
 * Entry: as PlayerSound23. Exit: as PlayerSound23.
 */
void PlayerSound24(void)
{
    player_sound(0x24, 0x26);
}

void PlayerSound24_cr(Carried *cr)
{
    cr->a = (uint16_t)((cr->a & 0xFF00) | player_sound(0x24, 0x26));
    cyc(72);
}

void abi_PlayerSound24(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PlayerSound24_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 04:D5E5 PlayerSound0D
 * WHAT IT DOES: Sends sound $0D (player 1) or $18 (player 2) on port 1.
 * HOW: As PlayerSound23.
 * Entry: as PlayerSound23. Exit: as PlayerSound23.
 */
void PlayerSound0D(void)
{
    player_sound(0x0D, 0x18);
}

void PlayerSound0D_cr(Carried *cr)
{
    cr->a = (uint16_t)((cr->a & 0xFF00) | player_sound(0x0D, 0x18));
    cyc(72);
}

void abi_PlayerSound0D(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PlayerSound0D_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 04:D5FE PlayerSound0B
 * WHAT IT DOES: Sends sound $0B (player 1) or $1B (player 2) on port 1.
 * HOW: As PlayerSound23.
 * Entry: as PlayerSound23. Exit: as PlayerSound23.
 */
void PlayerSound0B(void)
{
    player_sound(0x0B, 0x1B);
}

void PlayerSound0B_cr(Carried *cr)
{
    cr->a = (uint16_t)((cr->a & 0xFF00) | player_sound(0x0B, 0x1B));
    cyc(72);
}

void abi_PlayerSound0B(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PlayerSound0B_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 04:D617 PlayerSound12
 * WHAT IT DOES: Sends sound $12 (player 1) or $19 (player 2) on port 1.
 * HOW: As PlayerSound23.
 * Entry: as PlayerSound23. Exit: as PlayerSound23.
 */
void PlayerSound12(void)
{
    player_sound(0x12, 0x19);
}

void PlayerSound12_cr(Carried *cr)
{
    cr->a = (uint16_t)((cr->a & 0xFF00) | player_sound(0x12, 0x19));
    cyc(72);
}

void abi_PlayerSound12(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PlayerSound12_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 04:D630 PlayerSound0C
 * WHAT IT DOES: Sends sound $0C (player 1) or $1C (player 2) on port 1.
 * HOW: As PlayerSound23.
 * Entry: as PlayerSound23. Exit: as PlayerSound23.
 */
void PlayerSound0C(void)
{
    player_sound(0x0C, 0x1C);
}

void PlayerSound0C_cr(Carried *cr)
{
    cr->a = (uint16_t)((cr->a & 0xFF00) | player_sound(0x0C, 0x1C));
    cyc(72);
}

void abi_PlayerSound0C(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PlayerSound0C_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void SoundPort0QueuePushRaw(uint8_t command);

/*
 * @routine 03:EDDF SoundFromTableD89
 * WHAT IT DOES: Queues, exactly as given, the APU port 0 command that the
 * table at $03:EDEF holds for the value in $0D89.
 * HOW: Byte lookup indexed by $0D89, then SoundPort0QueuePushRaw.
 * Entry: any widths. Exit: A low byte = the command; X = $0D89 (high byte
 * zero); P unchanged.
 */
/* The lookup and push, with the index read once (LDX) and returned. */
static uint8_t sound_d89(uint8_t *index)
{
    cyc(68);
    uint8_t i = RD8(wram_0D89), command;

    cyc(48);
    command = rd8(0x03EDEF + i);
    SoundPort0QueuePushRaw(command);
    *index = i;
    cyc(72);
    return command;
}

uint8_t SoundFromTableD89(void)
{
    uint8_t i;

    return sound_d89(&i);
}

void SoundFromTableD89_cr(Carried *cr)
{
    uint8_t i, command = sound_d89(&i);

    cr->x = i;
    cr->a = (uint16_t)((cr->a & 0xFF00) | command);
}

void abi_SoundFromTableD89(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    SoundFromTableD89_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:FAF9 SoundPairFromTable
 * WHAT IT DOES: Queues two APU port 0 commands from the table at $0B:FB18:
 * the one for $0D8A with bit 7 set (player 2's), then the one for $0D89.
 * HOW: Two byte lookups, each pushed with SoundPort0QueuePushRaw.
 * Entry: any widths. Exit: A low byte = the second command; X = $0D89 (high
 * byte zero); P unchanged.
 */
static uint8_t sound_pair_from_table(uint8_t *x)
{
    uint8_t c;

    cyc(68);
    *x = RD8(wram_0D8A);
    cyc(64);
    c = (uint8_t)(rd8(0x0BFB18 + *x) | 0x80);
    SoundPort0QueuePushRaw(c);
    cyc(46);
    *x = RD8(wram_0D89);
    cyc(48);
    c = rd8(0x0BFB18 + *x);
    SoundPort0QueuePushRaw(c);
    cyc(72);
    return c;
}

uint8_t SoundPairFromTable(void)
{
    uint8_t x;

    return sound_pair_from_table(&x);
}

void SoundPairFromTable_cr(Carried *cr)
{
    uint8_t x, c = sound_pair_from_table(&x);

    cr->a = (uint16_t)((cr->a & 0xFF00) | c);
    cr->x = x;
}

void abi_SoundPairFromTable(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    SoundPairFromTable_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:D580 SoundAtRateForLevel
 * WHAT IT DOES: Queues APU port 0 command $23 at a rate set by the level A:
 * none below $28, every fourth frame (bits 0-1 of the counter at $05E9
 * clear) up to $9F, every other frame from $A0.
 * HOW: Signed compares with $28 and $A0; the counter is read through the
 * data bank; SoundPort0QueuePushRaw.
 * Entry: M 16-bit; A = level. Exit: below $28: A unchanged, N, Z, C from
 * the compare. Skipped: A = the counter bits; N, Z from them. Queued: A =
 * $23; N, Z from the data bank. C from the compare with $A0 in both.
 */
uint16_t SoundAtRateForLevel(uint8_t bank, uint16_t level, int *queued, uint8_t *p)
{
    (void)bank;
    uint16_t m, d = (uint16_t)(level - 0x28);

    *queued = 0;
    *p = (uint8_t)((nz16(*p, d) & ~P_C) | (level >= 0x28 ? P_C : (cyc(68), 0)));
    if (d & 0x8000)
        return level;
    d = (uint16_t)(level - 0xA0);
    *p = (uint8_t)((*p & ~P_C) | (level >= 0xA0 ? (cyc(6), P_C) : 0));
    cyc(172);
    m = RD16(wram_05E9) & (d & 0x8000 ? 0x0003 : 0x0001);
    if (m) {
        *p = nz16(*p, m);
        return m;
    } else { cyc(96); }
    SoundPort0QueuePushRaw(0x23);
    *queued = 1;
    return 0x0023;
}

void SoundAtRateForLevel_cr(Carried *cr)
{
    int queued;
    uint8_t p = cr->p;

    cr->a = SoundAtRateForLevel(cr->db, cr->a, &queued, &p);
    set_p(cr, queued ? nz8(p, cr->db) : (cyc(50), p));
    cyc(16);
}

void abi_SoundAtRateForLevel(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    SoundAtRateForLevel_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void SoundPort0QueuePush(uint8_t command);

/*
 * @routine 04:D0BE SoundIfFlag40
 * WHAT IT DOES: When bit 6 of A is set, queues for the current player the
 * APU port 0 command the table at $04:D0D6 holds for the byte at $0D89 + Y.
 * HOW: An immediate BIT test; byte lookups; SoundPort0QueuePush. A is saved
 * and restored.
 * Entry: M 16-bit, X 8-bit; A = flags, Y = offset. Exit: A unchanged; X =
 * the $0D89 byte when it queued; M 16-bit, X 8-bit; N, Z from A.
 */
int SoundIfFlag40(uint8_t bank, uint16_t flags, uint8_t y, uint8_t *x)
{
    (void)bank;
    if (!(flags & 0x0040))
        { cyc(138); return 0; }
    cyc(116);
    *x = RD8I(wram_0D89, y);
    cyc(100);
    SoundPort0QueuePush(rd8(0x04D0D6 + *x));
    cyc(46);
    return 1;
}

void SoundIfFlag40_cr(Carried *cr)
{
    uint8_t x;

    if (SoundIfFlag40(cr->db, cr->a, (uint8_t)cr->y, &x)) {
        cr->x = x;
        cr->y &= 0x00FF;
    }
    set_p(cr, (uint8_t)((nz16(cr->p, cr->a) & ~P_M) | P_X));
    cyc(38);
}

void abi_SoundIfFlag40(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    SoundIfFlag40_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 04:D082 SoundHoldByFlags
 * WHAT IT DOES: Starts or stops a held sound for player Y (0 or 1): when bit
 * 15 or 14 of A is set and the player's byte at $0DD8 + Y is 0, sets it to
 * $FF and queues port 1 command $3A (player 1) or $3C (player 2); when both
 * bits are clear and the byte is set, clears it and queues $3B or $3D.
 * HOW: Immediate BIT tests; SoundPort1QueuePush.
 * Entry: M 16-bit, X 8-bit; A = flags, Y = player. Exit: M 8-bit. Nothing
 * to do: A low byte = the byte; N, Z from it. Queued: A low byte = the
 * command; C set; N, Z from the data bank.
 */
int SoundHoldByFlags(uint8_t bank, uint16_t flags, uint8_t y, uint8_t *a)
{
    (void)bank;
    uint32_t at = WAI(sound_hold, y);

    /* BIT #$8000 and BNE (taken with bit 15), BIT #$4000 and BEQ, LDA */
    cyc(flags & 0x8000 ? 92 : flags & 0x4000 ? 126 : 132);
    uint8_t v = rd8(at);

    if (flags & 0xC000) {
        if (v) {
            *a = v;
            return 0;
        }
        cyc(60);
        wr8(at, 0xFF);
        *a = y ? (cyc(22), 0x3C) : (cyc(16), 0x3A);
    } else {
        if (!v) {
            *a = v;
            return 0;
        }
        cyc(62);
        wr8(at, 0x00);
        *a = y ? 0x3D : (cyc(16), 0x3B);
    }
    cyc(100);
    SoundPort1QueuePush(*a);
    return 1;
}

void SoundHoldByFlags_cr(Carried *cr)
{
    uint8_t a;

    if (SoundHoldByFlags(cr->db, cr->a, (uint8_t)cr->y, &a))
        set_p(cr, (uint8_t)(nz8(cr->p, cr->db) | P_C | P_M));
    else
        { cyc(38); set_p(cr, (uint8_t)(nz8(cr->p, a) | P_M)); }
    cyc(26);
    cr->a = (uint16_t)((cr->a & 0xFF00) | a);
}

void abi_SoundHoldByFlags(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    SoundHoldByFlags_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:D6D1 PlayerSound35
 * WHAT IT DOES: Queues sound command $35 on port 0 for the current player.
 * HOW: Tests whether the byte at $0DF4 + Y is 1, but both branches make
 * the same SoundPort0QueuePush call.
 * Entry: M 16-bit, X 16-bit; Y = player index (bytes). Exit: A low byte =
 * $35; M 8-bit; C set when the byte at $0DF4 + Y is 1 or more; N, Z from
 * the data bank.
 * From C: returns C.
 */
int PlayerSound35(uint8_t bank, uint16_t y)
{
    (void)bank;
    cyc(52);
    uint8_t v = RD8I(player_mode, y);

    cyc(v == 1 ? 116 : 110);            /* BEQ taken (v = 1) 6 longer than not */
    SoundPort0QueuePush(0x35);
    if (v < 1)
        cyc(42);
    return v >= 1;
}

void PlayerSound35_cr(Carried *cr)
{
    int c = PlayerSound35(cr->db, cr->y);

    cr->a = (uint16_t)((cr->a & 0xFF00) | 0x35);
    set_p(cr, (uint8_t)((nz8(cr->p, cr->db) & ~P_C) | P_M | (c ? P_C : 0)));
}

void abi_PlayerSound35(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PlayerSound35_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

static int es_full;                     /* the engine sounds ran: A whole */
static int es_xset;                     /* SoundIfFlag40 queued: X its byte */
static uint8_t es_x;

/* The engine sounds for one player: when $0E4B is 0 and the player's byte
   at `phase` is 6, SoundIfFlag40 with the player's newly pressed buttons
   and SoundHoldByFlags with the buttons held. Returns A (its low byte
   only, the byte that failed the test, unless es_full). */
static uint16_t engine_sounds(uint8_t bank, uint8_t player, uint16_t phase, uint16_t pressed, uint16_t held)
{
    uint32_t b = (uint32_t)bank << 16;
    uint8_t v, x, a;
    uint16_t w;

    es_full = 0;
    es_xset = 0;
    cyc(68);
    v = RD8(wram_0E4B);
    if (!v) {
        cyc(40);
        v = rd8(b | phase);
        if (v == 0x06) {
            cyc(78);
            w = rd16(b | pressed);
            cyc(46);
            if (SoundIfFlag40(bank, w, player, &x))
                { es_x = x; es_xset = 1; }
            cyc(38 + 46);
            w = rd16(b | held);
            cyc(46);                    /* JSR */
            if (!SoundHoldByFlags(bank, w, player, &a))
                cyc(38);
            cyc(26 + 72);
            es_full = 1;
            return (uint16_t)((w & 0xFF00) | a);
        } else { cyc(16); }
    }
    cyc(94);
    return v;
}

static void engine_sounds_exit(Carried *cr, uint16_t a)
{
    cr->x &= 0x00FF;
    cr->y &= 0x00FF;
    if (es_xset)
        cr->x = es_x;
    cr->a = es_full ? a : (uint16_t)((cr->a & 0xFF00) | (uint8_t)a);
}

/*
 * @routine 04:D040 EngineSoundsPlayer2
 * WHAT IT DOES: Runs player 2's engine sounds: when $0E4B is 0 and $0DD7 is
 * 6, SoundIfFlag40 with the buttons newly pressed ($0D1C) and
 * SoundHoldByFlags with the buttons held ($030D).
 * HOW: 8-bit tests through the data bank; Y is the caller's.
 * Entry: any widths; Y = the player. Exit: P unchanged; X, Y high bytes
 * clear; A and X as the sound routines leave them, or A low byte = the byte
 * that failed the test.
 */
uint16_t EngineSoundsPlayer2(uint8_t bank, uint8_t player)
{
    return engine_sounds(bank, player, W(engine_phase_p2), W(joy2_pressed_copy), W(joy2_held));
}

void EngineSoundsPlayer2_cr(Carried *cr)
{
    engine_sounds_exit(cr, EngineSoundsPlayer2(cr->db, (uint8_t)cr->y));
}

void abi_EngineSoundsPlayer2(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    EngineSoundsPlayer2_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 04:D061 EngineSoundsPlayer1
 * WHAT IT DOES: As EngineSoundsPlayer2, for player 1: $0DD6, the buttons
 * newly pressed at $0D1A and held at $0309.
 * HOW: As EngineSoundsPlayer2.
 * Entry: as EngineSoundsPlayer2. Exit: as EngineSoundsPlayer2.
 */
uint16_t EngineSoundsPlayer1(uint8_t bank, uint8_t player)
{
    return engine_sounds(bank, player, W(wram_0DD6), W(joy1_pressed_copy), W(joy1_held));
}

void EngineSoundsPlayer1_cr(Carried *cr)
{
    engine_sounds_exit(cr, EngineSoundsPlayer1(cr->db, (uint8_t)cr->y));
}

void abi_EngineSoundsPlayer1(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    EngineSoundsPlayer1_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:EC43 HandlerSoundIndex
 * WHAT IT DOES: Maps a selection handler address to a sound index: $81B9 to
 * 0, $853D to 1, $8411 to 2, $82E5 to 3, any other to 4.
 * HOW: A chain of 16-bit compares. The $82E5 case is not reached in the
 * traces so far.
 * Entry: M 16-bit; A = the handler address. Exit: A = the index; N clear, Z
 * from it; C set on a match, else from the last compare; X, Y unchanged.
 */
uint16_t HandlerSoundIndex(uint16_t handler, uint8_t *p)
{
    static const uint16_t known[4] = {0x81B9, 0x853D, 0x8411, 0x82E5};
    uint16_t i;

    cyc(60);
    for (i = 0; i < 4; i++)
        { cyc(46); if (handler == known[i]) {
            *p = (uint8_t)(nz16(*p, i) | P_C);
            return i;
        } }
    cyc(6);
    *p = (uint8_t)((nz16(*p, 4) & ~P_C) | (handler >= 0x82E5 ? P_C : 0));
    return 4;
}

void HandlerSoundIndex_cr(Carried *cr)
{
    cr->a = HandlerSoundIndex(cr->a, &cr->p);
}

void abi_HandlerSoundIndex(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    HandlerSoundIndex_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:DCEF SoundIndicesFromHandlers
 * WHAT IT DOES: Sets the sound indices from the two selection handlers:
 * $0D89 from handler A ($18DB), also $0D8B when $10DA or $10DC is nonzero,
 * and $0D8A from handler B ($18DD).
 * HOW: HandlerSoundIndex on each (the handlers read in bank 0, the rest
 * through the data bank), with 8-bit registers for the stores and tests.
 * Entry: any widths. Exit: A = handler B's index; X = the byte that decided
 * $0D8B ($10DA, or $10DC when it is 0); Y high byte cleared; P unchanged.
 */
uint8_t SoundIndicesFromHandlers(uint8_t bank, uint8_t *x)
{
    (void)bank;
    uint8_t p = 0, i;

    cyc(76);
    i = (uint8_t)HandlerSoundIndex(RD16(wram_18DB), &p);
    cyc(92);
    WR8(wram_0D89, i);
    cyc(24);
    *x = RD8(wram_10DA);
    if (!*x)
        { cyc(40); *x = RD8(wram_10DC); } else { cyc(6); }
    cyc(22);
    if (*x)
        { cyc(18); WR8(wram_0D8B, i); }
    cyc(54);
    i = (uint8_t)HandlerSoundIndex(RD16(selection_handler_b), &p);
    cyc(92);
    WR8(wram_0D8A, i);
    cyc(72);
    return i;
}

void SoundIndicesFromHandlers_cr(Carried *cr)
{
    uint8_t x;

    cr->a = SoundIndicesFromHandlers(cr->db, &x);
    cr->x = x;
    cr->y &= 0x00FF;
}

void abi_SoundIndicesFromHandlers(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    SoundIndicesFromHandlers_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

static int sn_full;                     /* SoundNearCarOnce's A is whole */

/*
 * @routine 04:D533 SoundNearCarOnce
 * WHAT IT DOES: The first time it runs (mark $0E1A clear), marks it, sends
 * $1D on port 1 and, unless $0D53 is set, a sound for the car nearest the
 * player when that car's record word $88 is not below the player's: $1D,
 * $1F or $1E on port 0 for car scripts $876E, $8AA8 and $8990.
 * HOW: DEC marks $0E1A ($FF). The car records are in Super FX RAM at
 * $70:0000 + the offsets in $70:2476 (player) and $70:2648 (nearest car);
 * the player's word is kept in $11DF for the compare. The $876E sound and
 * the no-match path are not reached in the traces so far.
 * Entry: DB a bank where $0E1A is work RAM. Exit: X and Y high bytes
 * cleared (SEP #$30 before the PHX); A as the path left it; P restored.
 * From C: returns A (its low byte only unless sn_full).
 */
uint16_t SoundNearCarOnce(uint8_t bank)
{
    (void)bank;
    uint8_t v;
    uint16_t x, a;

    sn_full = 0;
    cyc(22 + 46);
    v = RD8(wram_0E1A);
    if (v != 0)
        { cyc(72); return v; }
    cyc(54);
    WR8(wram_0E1A, 0xFF);
    cyc(78);
    SoundPort1QueuePushW(0x1D, 0);
    cyc(54);
    v = RD8(wram_0D53);
    if (v != 0)
        { cyc(72); return v; }
    cyc(100);                                       /* PHX, REP #$30, LDX */
    x = rd16(0x702476);
    cyc(46);
    a = rd16(0x700088 + x);
    cyc(32);
    WR16(scratch_11df, a);
    cyc(32);
    x = rd16(0x702648);
    cyc(46);
    a = rd16(0x700088 + x);
    cyc(32);
    sn_full = 1;
    if (a < RD16(scratch_11df))
        { cyc(108); return a; }
    cyc(48);
    a = rd16(0x700066 + x);
    if (a == 0x876E)
        v = 0x1D;
    else if (a == 0x8AA8)
        { cyc(40); v = 0x1F; }
    else if (a == 0x8990)
        { cyc(80); v = 0x1E; }
    else
        { cyc(228); return a; }
    cyc(182);
    SoundPort0QueuePushW(v, 16);                    /* M 8-bit, X 16-bit */
    cyc(v == 0x1E ? 50 : 72);                       /* BRA .done except .sound1E */
    return (uint16_t)((a & 0xFF00) | v);
}

void SoundNearCarOnce_cr(Carried *cr)
{
    uint16_t a = SoundNearCarOnce(cr->db);

    cr->a = sn_full ? a : (uint16_t)((cr->a & 0xFF00) | (uint8_t)a);
    cr->x &= 0x00FF;
    cr->y &= 0x00FF;
}

void abi_SoundNearCarOnce(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    SoundNearCarOnce_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

static int nc_low;                      /* NearCarSoundUpdate's A: low byte only */
static int nc_once;                     /* SoundNearCarOnce ran */

/*
 * @routine 07:EDC1 NearCarSoundUpdate
 * WHAT IT DOES: Unless $0E4B is set, follows the car nearest the player
 * with sound: packs its level and side ($70:2642 / 16, $70:2644 * 16) into
 * $0E47; when its distance band ($70:2646) changes, sends $31 - band on
 * port 1 (kept in $115E), clears $117C and runs SoundNearCarOnce. With no
 * nearest car (level negative) it sets $117C and stops the $0E1A sound.
 * HOW: Direct page $02 holds the level part for the OR; the band compare
 * is 16-bit against $115E.
 * Entry: any widths, DB a bank where $0E4B is work RAM. Exit: X, Y as
 * SoundNearCarOnce leaves them (high bytes cleared) when it runs; A as the
 * path left it; P restored.
 * From C: d = the direct page; m16 with 16-bit A; returns A (its low byte
 * only when nc_low).
 */
uint16_t NearCarSoundUpdate(uint8_t bank, uint16_t d, int m16)
{
    uint32_t dp = (uint16_t)(d + 2);
    uint16_t v, lv, a;

    nc_low = 0;
    nc_once = 0;
    if (!m16)
        v = RD8(wram_0E4B);
    else
        { cyc(46); v = RD16(wram_0E4B); }
    if (v) {
        cyc(94);
        nc_low = !m16;
        return v;
    }
    cyc(70);
    lv = rd16(0x702642);
    if (lv & 0x8000) {
        cyc(84);
        WR8(near_car_none, 0x01);
        a = (uint16_t)((lv & 0xFF00) | (cyc(62), Sound0E1AStop()));
        cyc(22 + 72);
        return a;
    }
    cyc(88);
    wr16(dp, (uint16_t)(lv >> 4));
    cyc(32);
    v = (uint16_t)(rd16(0x702644) << 4);
    cyc(72);
    v |= rd16(dp);
    cyc(54);
    WR8(near_car_pos, (uint8_t)v);
    cyc(92);
    v = (uint16_t)(0x0031 - rd16(0x702646));
    cyc(24);
    if (v == RD16(near_car_band_sent))
        { cyc(94); return v; }
    cyc(40);
    WR16(near_car_band_sent, v);
    cyc(62);
    SoundPort1QueuePushW((uint8_t)v, 24);           /* M and X 16-bit */
    cyc(24);
    WR16(near_car_none, 0x0000);
    a = (cyc(62), SoundNearCarOnce(bank));
    if (!sn_full)
        a = (uint16_t)((v & 0xFF00) | (uint8_t)a);
    nc_once = 1;
    cyc(94);
    return a;
}

void NearCarSoundUpdate_cr(Carried *cr)
{
    uint16_t a = NearCarSoundUpdate(cr->db, cr->d, !(cr->p & P_M));

    cr->a = nc_low ? (uint16_t)((cr->a & 0xFF00) | (uint8_t)a) : a;
    if (nc_once) {
        cr->x &= 0x00FF;
        cr->y &= 0x00FF;
    }
}

void abi_NearCarSoundUpdate(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    NearCarSoundUpdate_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 04:DA20 WindowSlideUpSound
 * WHAT IT DOES: Sends sound $10 on port 1 while the window slide is going
 * up (bit 2 of $0DA5), then sets $19D8 (window HDMA off) if it is clear.
 * HOW: BIT #$04 on $0DA5 through the data bank. The sound is not sent in
 * the traces so far.
 * Entry: any widths. Exit: A low byte = $19D8's old value, or 1 when it
 * was clear; P unchanged.
 * From C: x16 with 16-bit index registers.
 */
uint8_t WindowSlideUpSound(uint8_t bank, int x16)
{
    (void)bank;
    uint8_t v;

    cyc(68);
    if (RD8(slide_flags) & 0x04) {
        cyc(110);
        SoundPort1QueuePushW(0x10, x16 ? 16 : 0);
    } else { cyc(38); }
    cyc(24);
    v = RD8(window_hdma_off);
    if (!v) {
        v = 0x01;
        cyc(56);
        WR8(window_hdma_off, 0x01);
    } else { cyc(22); }
    cyc(72);
    return v;
}

void WindowSlideUpSound_cr(Carried *cr)
{
    cr->a = (uint16_t)((cr->a & 0xFF00) | WindowSlideUpSound(cr->db, !(cr->p & P_X)));
}

void abi_WindowSlideUpSound(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    WindowSlideUpSound_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

