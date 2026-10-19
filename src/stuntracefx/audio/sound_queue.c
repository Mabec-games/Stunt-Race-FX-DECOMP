/*
 * sound_queue.c: the two sound command queues.
 *
 * Game code asks for sounds by queueing command bytes. Each queue is a
 * 16-byte ring with a head index and a count; once a frame the RAM-resident
 * frame code sends the oldest byte of each queue to its APU port (0 or 1)
 * when the sound CPU has taken the previous one.
 */
#include "machine.h"
#include "cpu_ops.h"
#include "hw.h"
#include "wram.h"

/* Append a byte to a ring, unless sound is muted or the ring is full. The
   count is incremented in place (read again, as INC does): the frame code
   may take a byte from the ring between the first read and the increment. */
static void queue_push(uint32_t queue, uint32_t head, uint32_t count, uint8_t value)
{
    uint8_t n;

    cyc(46);
    if (rd8(SOUND_MUTED) != 0)
        return;
    cyc(40);
    n = rd8(count);
    if (n == 0) {
        cyc(46);
        wr8(count, cyc_then(6, (uint8_t)(rd8(count) + 1)));
        cyc(24);
        wr8(queue + rd8(head), cyc_then(30, value));
    } else if (n < 0x10) {
        cyc(122);
        uint8_t slot = (uint8_t)(n + rd8(head));
        if (slot >= 0x10)
            { cyc(10); slot -= 0x10; }
        cyc(110);
        wr8(queue + slot, value);
        cyc(24);
        wr8(count, cyc_then(6, (uint8_t)(rd8(count) + 1)));
    } else { cyc(54); }
cyc(200);
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
    cyc(252);
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
    cyc(238);
    if (rd8(CURRENT_PLAYER) != 0)
        { cyc(10); command |= 0x80; }
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
    cyc(192);
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
    cyc(92);
    wr8(SOUND_MUTED, 0);
    cyc(32);
    wr8(WRAM_0DD0, 0);
    cyc(32);
    wr8(SOUND_PORT1_LAST, 0);
    cyc(32);
    wr8(SOUND_PORT0_HEAD, 0);
    cyc(32);
    wr8(SOUND_PORT1_HEAD, 0);
    cyc(32);
    wr8(SOUND_PORT1_COUNT, 0);
    cyc(32);
    wr8(SOUND_PORT0_COUNT, 0);
    cyc(32);
    wr8(WRAM_0E1B, 0);
    cyc(32);
    wr8(WRAM_0E1C, 0);
    cyc(32);
    wr8(APUIO3, 0);
    cyc(32);
    wr8(APUIO2, 0);
}

void abi_SoundReset(void)
{
    SoundReset();
    cpu.a &= 0xFF00;
    cpu.x &= 0x00FF;
    cpu.y &= 0x00FF;
}

/* One ring to its port: 0 when the port still shows a command, else the
   oldest byte (the head steps on, wrapping at 16, unless that was the
   last). Returns 0 when the ring is empty and the port was clear (nothing
   written), else 1 with the byte written in A. */
static int ring_send(uint32_t b, uint16_t port, uint16_t count, uint16_t head, uint16_t ring)
{
    uint8_t v, n;

    cyc(24);
    v = rd8(b | port);
    if (v) {
        cyc(16);
        v = 0x00;
    } else {
        cyc(40);
        n = rd8(b | count);
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | n);
        if (!n)
            { cyc(22); return 0; }
        cyc(40);
        cpu.x = rd8(b | head);
        cyc(24);
        v = rd8(b | (uint16_t)(ring + cpu.x));
        cyc(24);
        n = (uint8_t)(rd8(b | count) - 1);
        cyc(6);
        wr8(b | count, n);
        if (n) {
            cpu.x = (uint8_t)(cpu.x + 1);
            if (cpu.x >= 0x10)
                cpu.x = 0x00;
            cyc(92);
            wr8(b | head, (uint8_t)cpu.x);
        } else { cyc(22); }
    }
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cyc(46);
    wr8(b | port, v);
    return 1;
}

/*
 * @routine 7E:B112 SoundPortsSend
 * WHAT IT DOES: The frame's sound output: a direct byte at $10E2 (port 0)
 * or $10E3 (port 1) is sent alone and cleared; otherwise, unless $10DA or
 * $10DC is set, on odd frames ($0E3B) each port gets 0 while it still
 * shows the last command, else the next byte of its queue (port 0 from
 * the ring at $0DA8, port 1 from $0DB8, the port 1 byte also kept at
 * $0DD2); then ports 2 and 3 get $0E47 and $0E49 (0 while $0E4B is set).
 * HOW: 8-bit, through the data bank; a queue's head only moves on while
 * bytes remain after the one taken. The direct bytes are not sent in the
 * traces so far.
 * Entry: any widths; DB a bank where work RAM and the APU ports are
 * mapped. Exit: X and Y high bytes cleared; X = the last head; A as the
 * last write; P restored.
 */
void abi_SoundPortsSend(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint8_t p0 = cpu.p, v;

    op_sep(P_M | P_X);
    cyc(46);
    v = rd8(b | 0x10E2);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    if (v) {
        cyc(40);
        wr8(b | 0x2140, v);
        cyc(24);
        wr8(b | 0x10E2, 0x00);
        op_plp(p0);
        return;
    }
    cyc(46);
    v = rd8(b | 0x10E3);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    if (v) {
        cyc(40);
        wr8(b | 0x2141, v);
        cyc(24);
        wr8(b | 0x10E3, 0x00);
        op_plp(p0);
        return;
    } else { cyc(6); }
    {
        static const uint16_t hold[2] = {0x10DA, 0x10DC};
        int i;

        for (i = 0; i < 2; i++) {
            cyc(40);
            v = rd8(b | hold[i]);
            cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
            if (v) {
                cyc(50);
                op_plp(p0);
                return;
            }
        }
    }
    cyc(40);
    v = rd8(b | 0x0E3B);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    if (!(v & 0x01)) {
        cyc(60);
        op_plp(p0);
        return;
    } else { cyc(38); }
    ring_send(b, 0x2140, 0x0DCC, 0x0DC8, 0x0DA8);
    if (ring_send(b, 0x2141, 0x0DCE, 0x0DCA, 0x0DB8))
        { cyc(24); wr8(b | 0x0DD2, (uint8_t)cpu.a); }
    cyc(24);
    v = rd8(b | 0x0E4B);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    if (v) {
        cpu.a = (uint16_t)(cpu.a & 0xFF00);
        cyc(62);
        wr8(b | 0x2142, 0x00);
        cyc(24);
        wr8(b | 0x2143, 0x00);
    } else {
        cyc(40);
        v = rd8(b | 0x0E47);
        cyc(24);
        wr8(b | 0x2142, v);
        cyc(24);
        v = rd8(b | 0x0E49);
        cyc(24);
        wr8(b | 0x2143, v);
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    }
    cyc(28);
    op_plp(p0);
}
