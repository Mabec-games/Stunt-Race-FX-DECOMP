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

void SoundPort1QueuePush(uint8_t command);

/* Start a held sound: announce it first if nothing was held, then send it. */
static void held_sound_set(uint32_t held, uint8_t announce, uint8_t sound)
{
    if (rd8(held) == 0)
        SoundPort1QueuePush(announce);
    wr8(held, sound);
    SoundPort1QueuePush(sound);
}

/* Hold sound p1 (player 1) or p2 (player 2) unless it is already held.
   Returns the command chosen, which is what the original leaves in A. */
static uint8_t player_hold_sound(uint8_t p1, uint8_t p2)
{
    if (rd8(CURRENT_PLAYER) == 0) {
        if (rd8(PLAYER1_HELD_SOUND) != p1)
            held_sound_set(PLAYER1_HELD_SOUND, 0x09, p1);
        return p1;
    }
    if (rd8(PLAYER2_HELD_SOUND) != p2)
        held_sound_set(PLAYER2_HELD_SOUND, 0x14, p2);
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
 * Entry, exit: as PlayerHoldSound32.
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
    uint32_t held = rd8(CURRENT_PLAYER) == 0 ? PLAYER1_HELD_SOUND : PLAYER2_HELD_SOUND;
    uint8_t stop = rd8(CURRENT_PLAYER) == 0 ? 0x0A : 0x1A;

    if (rd8(held) == 0)
        return 0;
    wr8(held, 0);
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
 * Entry, exit: as PlayerHoldSound32.
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
 * Entry, exit: as PlayerHoldSound32.
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
    if (rd8(WRAM_0E1A) == 0)
        return 0;
    wr8(WRAM_0E1A, 0);
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
    uint8_t command = rd8(CURRENT_PLAYER) != 0 ? p2 : p1;
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
 * Entry, exit: as PlayerSound23.
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
 * Entry, exit: as PlayerSound23.
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
 * Entry, exit: as PlayerSound23.
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
 * Entry, exit: as PlayerSound23.
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
 * Entry, exit: as PlayerSound23.
 */
void PlayerSound0C(void)
{
    player_sound(0x0C, 0x1C);
}

void abi_PlayerSound0C(void)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | player_sound(0x0C, 0x1C));
}
