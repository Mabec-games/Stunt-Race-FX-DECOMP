/*
 * sound_queue.c: the two sound command queues.
 *
 * Game code asks for sounds by queueing command bytes. Each queue is a
 * 16-byte ring with a head index and a count; once a frame the RAM-resident
 * frame code sends the oldest byte of each queue to its APU port (0 or 1)
 * when the sound CPU has taken the previous one.
 */
#include "machine.h"
#include "hw.h"
#include "wram.h"

/* Append a byte to a ring, unless sound is muted or the ring is full. The
   count is incremented in place (read again, as INC does): the frame code
   may take a byte from the ring between the first read and the increment. */
static void queue_push(uint32_t queue, uint32_t head, uint32_t count, uint8_t value)
{
    uint8_t n;

    if (rd8(SOUND_MUTED) != 0)
        return;
    n = rd8(count);
    if (n == 0) {
        wr8(count, (uint8_t)(rd8(count) + 1));
        wr8(queue + rd8(head), value);
    } else if (n < 0x10) {
        uint8_t slot = (uint8_t)(n + rd8(head));
        if (slot >= 0x10)
            slot -= 0x10;
        wr8(queue + slot, value);
        wr8(count, (uint8_t)(rd8(count) + 1));
    }
}

/*
 * @routine 04:D649 SoundPort0QueuePushRaw
 * WHAT IT DOES: Queues a sound command for APU port 0 exactly as given.
 * HOW: queue_push on the port 0 ring with the low byte of A.
 * Entry: any widths; A low byte = command. Exit: every register unchanged
 * except N and Z, which the final pull of DB sets from DB.
 */
void SoundPort0QueuePushRaw(uint8_t command)
{
    queue_push(SOUND_PORT0_QUEUE, SOUND_PORT0_HEAD, SOUND_PORT0_COUNT, command);
}

void abi_SoundPort0QueuePushRaw(void)
{
    SoundPort0QueuePushRaw((uint8_t)cpu.a);
    cpu.p = nz8(cpu.p, cpu.db);  /* the last pull restores DB */
}

/*
 * @routine 04:D656 SoundPort0QueuePush
 * WHAT IT DOES: Queues a sound command for APU port 0 on behalf of the player
 * whose car is being processed.
 * HOW: Sets bit 7 of the command when that is player 2, then queue_push on
 * the port 0 ring.
 * Entry: any widths; A low byte = command. Exit: every register unchanged
 * except N and Z, which the final pull of DB sets from DB.
 */
void SoundPort0QueuePush(uint8_t command)
{
    if (rd8(CURRENT_PLAYER) != 0)
        command |= 0x80;
    queue_push(SOUND_PORT0_QUEUE, SOUND_PORT0_HEAD, SOUND_PORT0_COUNT, command);
}

void abi_SoundPort0QueuePush(void)
{
    SoundPort0QueuePush((uint8_t)cpu.a);
    cpu.p = nz8(cpu.p, cpu.db);  /* the last pull restores DB */
}

/*
 * @routine 04:D69F SoundPort1QueuePush
 * WHAT IT DOES: Queues a sound command for APU port 1.
 * HOW: queue_push on the port 1 ring with the low byte of A.
 * Entry: any widths; A low byte = command. Exit: every register unchanged
 * except N and Z, which the final pull of DB sets from DB.
 */
void SoundPort1QueuePush(uint8_t command)
{
    queue_push(SOUND_PORT1_QUEUE, SOUND_PORT1_HEAD, SOUND_PORT1_COUNT, command);
}

void abi_SoundPort1QueuePush(void)
{
    SoundPort1QueuePush((uint8_t)cpu.a);
    cpu.p = nz8(cpu.p, cpu.db);  /* the last pull restores DB */
}

/*
 * @routine 04:D6E1 SoundReset
 * WHAT IT DOES: Empties both sound queues, unmutes, clears the sound state
 * bytes and puts 0 on APU ports 3 and 2.
 * HOW: Stores 0 to the mute flag, $0DD0, the last-port-1 byte, both heads and
 * counts, $0E1B and $0E1C, then to ports 3 and 2 in that order.
 * Entry: any widths. Exit: A low byte = 0; X and Y high bytes cleared (the
 * routine switches to 8-bit index registers); P unchanged.
 */
void SoundReset(void)
{
    wr8(SOUND_MUTED, 0);
    wr8(WRAM_0DD0, 0);
    wr8(SOUND_PORT1_LAST, 0);
    wr8(SOUND_PORT0_HEAD, 0);
    wr8(SOUND_PORT1_HEAD, 0);
    wr8(SOUND_PORT1_COUNT, 0);
    wr8(SOUND_PORT0_COUNT, 0);
    wr8(WRAM_0E1B, 0);
    wr8(WRAM_0E1C, 0);
    wr8(APUIO3, 0);
    wr8(APUIO2, 0);
}

void abi_SoundReset(void)
{
    SoundReset();
    cpu.a &= 0xFF00;
    cpu.x &= 0x00FF;
    cpu.y &= 0x00FF;
}
