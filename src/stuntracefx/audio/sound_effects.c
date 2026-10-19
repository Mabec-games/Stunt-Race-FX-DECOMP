/*
 * sound_effects.c: sound commands the game sends for each player's car.
 *
 * Most effects have one command for player 1 and another for player 2; the
 * routines here pick by the player whose car is being processed. A car can
 * also hold one looping sound, remembered per player so it is only re-sent
 * when it changes.
 */
#include "machine.h"
#include "wram.h"
#include "cpu_ops.h"

void SoundPort1QueuePush(uint8_t command);

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
    cyc(179);
    if (rd8(CURRENT_PLAYER) == 0) {
        cyc(64);
        if (rd8(PLAYER1_HELD_SOUND) != p1)
            held_sound_set(PLAYER1_HELD_SOUND, 0x09, p1); else { cyc(16); }
        return p1;
    }
    cyc(70);
    if (rd8(PLAYER2_HELD_SOUND) != p2)
        held_sound_set(PLAYER2_HELD_SOUND, 0x14, p2); else { cyc(16); }
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

void abi_PlayerHoldSound32(void)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | player_hold_sound(0x32, 0x36));
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

void abi_PlayerHoldSound33(void)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | player_hold_sound(0x33, 0x37));
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
    cyc(166);
    uint8_t two = rd8(CURRENT_PLAYER) != 0;
    uint32_t held = two ? PLAYER2_HELD_SOUND : PLAYER1_HELD_SOUND;
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

void abi_PlayerStopHeldSound(void)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | player_stop_held_sound());
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

void abi_PlayerHoldSound34(void)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | player_hold_sound(0x34, 0x38));
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

void abi_PlayerHoldSound35(void)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | player_hold_sound(0x35, 0x39));
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
    cyc(108);
    if (rd8(WRAM_0E1A) == 0)
        { cyc(72); return 0; }
    cyc(40);
    wr8(WRAM_0E1A, 0);
    cyc(78);
    SoundPort1QueuePush(0x08);
    return 0x08;
}

void Sound0E1AStop(void)
{
    sound_0e1a_stop();
}

void abi_Sound0E1AStop(void)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | sound_0e1a_stop());
}

/* Send p1 (player 1) or p2 (player 2) on port 1; returns the one sent. */
static uint8_t player_sound(uint8_t p1, uint8_t p2)
{
    cyc(208);
    uint8_t command = rd8(CURRENT_PLAYER) != 0 ? (cyc(100), p2) : (cyc(94), p1);
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

void abi_PlayerSound23(void)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | player_sound(0x23, 0x25));
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

void abi_PlayerSound24(void)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | player_sound(0x24, 0x26));
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

void abi_PlayerSound0D(void)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | player_sound(0x0D, 0x18));
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

void abi_PlayerSound0B(void)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | player_sound(0x0B, 0x1B));
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

void abi_PlayerSound12(void)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | player_sound(0x12, 0x19));
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

void abi_PlayerSound0C(void)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | player_sound(0x0C, 0x1C));
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
    cyc(152);
    uint8_t i = rd8(0x000D89), command;

    cyc(48);
    command = rd8(0x03EDEF + i);
    SoundPort0QueuePushRaw(command);
    *index = i;
    return command;
}

uint8_t SoundFromTableD89(void)
{
    uint8_t i;

    return sound_d89(&i);
}

void abi_SoundFromTableD89(void)
{
    uint8_t i, command = sound_d89(&i);

    cpu.x = i;
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | command);
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

    cyc(236);
    *x = rd8(0x000D8A);
    cyc(64);
    c = (uint8_t)(rd8(0x0BFB18 + *x) | 0x80);
    SoundPort0QueuePushRaw(c);
    cyc(30);
    *x = rd8(0x000D89);
    cyc(48);
    c = rd8(0x0BFB18 + *x);
    SoundPort0QueuePushRaw(c);
    return c;
}

uint8_t SoundPairFromTable(void)
{
    uint8_t x;

    return sound_pair_from_table(&x);
}

void abi_SoundPairFromTable(void)
{
    uint8_t x, c = sound_pair_from_table(&x);

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | c);
    cpu.x = x;
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
    uint16_t m, d = (uint16_t)(level - 0x28);

    *queued = 0;
    *p = (uint8_t)((nz16(*p, d) & ~P_C) | (level >= 0x28 ? P_C : (cyc(68), 0)));
    if (d & 0x8000)
        return level;
    d = (uint16_t)(level - 0xA0);
    *p = (uint8_t)((*p & ~P_C) | (level >= 0xA0 ? (cyc(6), P_C) : 0));
    cyc(172);
    m = rd16((uint32_t)bank << 16 | 0x05E9) & (d & 0x8000 ? 0x0003 : 0x0001);
    if (m) {
        *p = nz16(*p, m);
        return m;
    } else { cyc(96); }
    SoundPort0QueuePushRaw(0x23);
    *queued = 1;
    return 0x0023;
}

void abi_SoundAtRateForLevel(void)
{
    int queued;
    uint8_t p = cpu.p;

    cpu.a = SoundAtRateForLevel(cpu.db, cpu.a, &queued, &p);
    cpu.p = queued ? nz8(p, cpu.db) : (cyc(66), p);
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
    if (!(flags & 0x0040))
        { cyc(138); return 0; }
    cyc(162);
    *x = rd8((uint32_t)bank << 16 | (uint16_t)(0x0D89 + y));
    cyc(100);
    SoundPort0QueuePush(rd8(0x04D0D6 + *x));
    return 1;
}

void abi_SoundIfFlag40(void)
{
    uint8_t x;

    if (SoundIfFlag40(cpu.db, cpu.a, (uint8_t)cpu.y, &x)) {
        cpu.x = x;
        cpu.y &= 0x00FF;
    }
    cpu.p = (uint8_t)((nz16(cpu.p, cpu.a) & ~P_M) | P_X);
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
    uint32_t at = (uint32_t)bank << 16 | (uint16_t)(0x0DD8 + y);
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

void abi_SoundHoldByFlags(void)
{
    uint8_t a;

    if (SoundHoldByFlags(cpu.db, cpu.a, (uint8_t)cpu.y, &a))
        cpu.p = (uint8_t)(nz8(cpu.p, cpu.db) | P_C | P_M);
    else
        { cyc(38); cpu.p = (uint8_t)(nz8(cpu.p, a) | P_M); }
    cyc(122);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | a);
}

/*
 * @routine 07:D6D1 PlayerSound35
 * WHAT IT DOES: Queues sound command $35 on port 0 for the current player.
 * HOW: Tests whether the byte at $0DF4 + Y is 1, but both branches make
 * the same SoundPort0QueuePush call.
 * Entry: M 16-bit, X 16-bit; Y = player index (bytes). Exit: A low byte =
 * $35; M 8-bit; C set when the byte at $0DF4 + Y is 1 or more; N, Z from
 * the data bank.
 */
void PlayerSound35(void)
{
    SoundPort0QueuePush(0x35);
}

void abi_PlayerSound35(void)
{
    cyc(52);
    uint8_t v = rd8((uint32_t)cpu.db << 16 | (uint16_t)(0x0DF4 + cpu.y));

    PlayerSound35();
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x35);
    cpu.p = (uint8_t)((nz8(cpu.p, cpu.db) & ~P_C) | P_M | (v >= 1 ? P_C : (cyc(42), 0)));
}

void abi_SoundIfFlag40(void);
void abi_SoundHoldByFlags(void);

/* The engine sounds for one player: when $0E4B is 0 and the player's byte
   at `phase` is 6, SoundIfFlag40 with the player's newly pressed buttons
   and SoundHoldByFlags with the buttons held (Y = the player). */
static void abi_engine_sounds(uint16_t phase, uint16_t pressed, uint16_t held)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint8_t p = cpu.p, v;

    cpu.x &= 0x00FF;
    cpu.y &= 0x00FF;
    cyc(90);
    v = rd8(b | 0x0E4B);
    if (!v) {
        cyc(40);
        v = rd8(b | phase);
        if (v == 0x06) {
            cpu.p = (uint8_t)((cpu.p & ~P_M) | P_X);
            cyc(78);
            cpu.a = rd16(b | pressed);
            abi_SoundIfFlag40();
            cpu.p &= (uint8_t)~P_M;
            cyc(130);
            cpu.a = rd16(b | held);
            abi_SoundHoldByFlags();
            cpu.p = p;
            return;
        } else { cyc(16); }
    }
    cyc(118);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = p;
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
static void engine_sounds(uint8_t bank, uint8_t player, uint16_t phase, uint16_t pressed, uint16_t held)
{
    uint32_t b = (uint32_t)bank << 16;
    uint8_t x, a;

    if (rd8(b | 0x0E4B) || rd8(b | phase) != 0x06)
        return;
    SoundIfFlag40(bank, rd16(b | pressed), player, &x);
    SoundHoldByFlags(bank, rd16(b | held), player, &a);
}

void EngineSoundsPlayer2(uint8_t bank, uint8_t player)
{
    engine_sounds(bank, player, 0x0DD7, 0x0D1C, 0x030D);
}

void abi_EngineSoundsPlayer2(void)
{
    cyc(40);
    abi_engine_sounds(0x0DD7, 0x0D1C, 0x030D);
}

/*
 * @routine 04:D061 EngineSoundsPlayer1
 * WHAT IT DOES: As EngineSoundsPlayer2, for player 1: $0DD6, the buttons
 * newly pressed at $0D1A and held at $0309.
 * HOW: As EngineSoundsPlayer2.
 * Entry: as EngineSoundsPlayer2. Exit: as EngineSoundsPlayer2.
 */
void EngineSoundsPlayer1(uint8_t bank, uint8_t player)
{
    engine_sounds(bank, player, 0x0DD6, 0x0D1A, 0x0309);
}

void abi_EngineSoundsPlayer1(void)
{
    abi_engine_sounds(0x0DD6, 0x0D1A, 0x0309);
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

void abi_HandlerSoundIndex(void)
{
    cpu.a = HandlerSoundIndex(cpu.a, &cpu.p);
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
    uint32_t b = (uint32_t)bank << 16;
    uint8_t p = 0, i;

    cyc(76);
    i = (uint8_t)HandlerSoundIndex(rd16(0x0018DB), &p);
    cyc(92);
    wr8(b | 0x0D89, i);
    cyc(24);
    *x = rd8(b | 0x10DA);
    if (!*x)
        { cyc(40); *x = rd8(b | 0x10DC); } else { cyc(6); }
    cyc(22);
    if (*x)
        { cyc(18); wr8(b | 0x0D8B, i); }
    cyc(54);
    i = (uint8_t)HandlerSoundIndex(rd16(0x0018DD), &p);
    cyc(92);
    wr8(b | 0x0D8A, i);
    return i;
}

void abi_SoundIndicesFromHandlers(void)
{
    uint8_t x;

    cpu.a = SoundIndicesFromHandlers(cpu.db, &x);
    cpu.x = x;
    cpu.y &= 0x00FF;
}

void abi_SoundPort0QueuePush(void);
void abi_SoundPort1QueuePush(void);

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
 */
void abi_SoundNearCarOnce(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint8_t p0 = cpu.p, v;
    uint16_t x0, w;

    op_sep(P_M | P_X);
    cyc(108);
    v = rd8(b | 0x0E1A);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    if (v != 0) {
        cyc(72);
        cpu.p = p0;
        return;
    }
    cyc(54);
    wr8(b | 0x0E1A, (uint8_t)(v - 1));
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x1D);
    (cyc(62), abi_SoundPort1QueuePush());
    cyc(38);
    v = rd8(0x000D53);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    if (v != 0) {
        cyc(72);
        cpu.p = p0;
        return;
    }
    x0 = cpu.x;                                     /* PHX */
    cyc(100);
    cpu.x = rd16(0x702476);
    cyc(46);
    w = rd16(0x700088 + cpu.x);
    cyc(32);
    wr16(0x0011DF, w);
    cyc(32);
    cpu.x = rd16(0x702648);
    cyc(46);
    cpu.a = rd16(0x700088 + cpu.x);
    cyc(32);
    if (cpu.a < rd16(0x0011DF)) {
        cyc(108);
        cpu.x = x0;
        cpu.p = p0;
        return;
    }
    cyc(48);
    cpu.a = rd16(0x700066 + cpu.x);
    cpu.x = x0;                                     /* PLX */
    if (cpu.a == 0x876E)
        v = 0x1D;
    else if (cpu.a == 0x8AA8)
        { cyc(40); v = 0x1F; }
    else if (cpu.a == 0x8990)
        { cyc(80); v = 0x1E; }
    else {
        cyc(228);
        cpu.p = p0;
        return;
    }
    cyc(182);
    cpu.p |= P_M;
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    abi_SoundPort0QueuePush();
    cpu.p = p0;
}

void abi_Sound0E1AStop(void);

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
 */
void abi_NearCarSoundUpdate(void)
{
    uint32_t b = (uint32_t)cpu.db << 16, dp = (uint16_t)(cpu.d + 2);
    uint8_t p0 = cpu.p;
    uint16_t v, lv;

    if (cpu.p & P_M) {
        v = rd8(b | 0x0E4B);
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    } else {
        cyc(46);
        v = rd16(b | 0x0E4B);
        cpu.a = v;
    }
    if (v) {
        cpu.p = p0;
        return;
    }
    cpu.p &= (uint8_t)~(P_M | P_X);
    cyc(70);
    lv = rd16(0x702642);
    if (lv & 0x8000) {
        cpu.p |= P_M;
        cpu.a = (uint16_t)((lv & 0xFF00) | 0x01);
        cyc(84);
        wr8(b | 0x117C, 0x01);
        abi_Sound0E1AStop();
        cpu.p = p0;
        return;
    }
    cyc(88);
    wr16(dp, (uint16_t)(lv >> 4));
    cyc(32);
    v = (uint16_t)(rd16(0x702644) << 4);
    cyc(72);
    v |= rd16(dp);
    cyc(54);
    wr8(0x000E47, (uint8_t)v);
    cyc(92);
    v = (uint16_t)(0x0031 - rd16(0x702646));
    cpu.a = v;
    cyc(24);
    if (v == rd16(b | 0x115E)) {
        cpu.p = p0;
        return;
    }
    cyc(40);
    wr16(b | 0x115E, v);
    (cyc(70), abi_SoundPort1QueuePush());
    cyc(32);
    wr16(b | 0x117C, 0x0000);
    abi_SoundNearCarOnce();
    cpu.p = p0;
}

/*
 * @routine 04:DA20 WindowSlideUpSound
 * WHAT IT DOES: Sends sound $10 on port 1 while the window slide is going
 * up (bit 2 of $0DA5), then sets $19D8 (window HDMA off) if it is clear.
 * HOW: BIT #$04 on $0DA5 through the data bank. The sound is not sent in
 * the traces so far.
 * Entry: any widths. Exit: A low byte = $19D8's old value, or 1 when it
 * was clear; P unchanged.
 */
void abi_WindowSlideUpSound(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint8_t p0 = cpu.p, v;

    cpu.p |= P_M;
    cyc(68);
    v = rd8(b | 0x0DA5);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    if (v & 0x04) {
        cyc(110);
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x10);
        abi_SoundPort1QueuePush();
    } else { cyc(38); }
    cyc(24);
    v = rd8(b | 0x19D8);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    if (!v) {
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x01);
        cyc(56);
        wr8(b | 0x19D8, 0x01);
    } else { cyc(22); }
    cyc(72);
    cpu.p = p0;
}
