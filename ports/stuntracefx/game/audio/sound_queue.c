/*
 * sound_queue.c: the two sound command queues.
 *
 * Game code asks for sounds by queueing command bytes. Each queue is a
 * 16-byte ring with a head index and a count; once a frame the RAM-resident
 * frame code sends the oldest byte of each queue to its APU port (0 or 1)
 * when the sound CPU has taken the previous one.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "hw.h"
#include "wram.h"
#include "audio_calls.h"
#include "carried.h"

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
    cyc(184);
}

/* The caller's PHA/PLA 8 longer with A 16-bit, PHY/PHX/PLX/PLY 16 with X
   and Y 16-bit, from its P. */
static unsigned push_wide(uint8_t p)
{
    return (p & P_M ? 0 : 8) + (p & P_X ? 0 : 16);
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
    cyc(8);
}

/* SoundPort0QueuePushRaw on carried registers, as abi_SoundPort0QueuePushRaw leaves them. */
void SoundPort0QueuePushRaw_cr(Carried *cr)
{
    SoundPort0QueuePushRaw((uint8_t)cr->a);
    cr->p = nz8(cr->p, cr->db);  /* the last pull restores DB */
    cyc(8);
}

/* From a caller with M and X 8-bit: the pulls take no extra byte. */
void SoundPort0QueuePushRawX8(uint8_t command)
{
    SoundPort0QueuePushRaw(command);
}

void abi_SoundPort0QueuePushRawX8(void)
{
    SoundPort0QueuePushRawX8((uint8_t)cpu.a);
    cpu.p = nz8(cpu.p, cpu.db);
}

/* SoundPort0QueuePushRawX8 on carried registers, as abi_SoundPort0QueuePushRawX8 leaves them. */
void SoundPort0QueuePushRawX8_cr(Carried *cr)
{
    SoundPort0QueuePushRawX8((uint8_t)cr->a);
    cr->p = nz8(cr->p, cr->db);
}

/* With the caller's widths: wide as for SoundPort0QueuePushW (the entry's
   BRA is queue_push's). */
void SoundPort0QueuePushRawWide(uint8_t command, unsigned wide)
{
    cyc(198 + wide);
    queue_push(SOUND_PORT0_QUEUE, SOUND_PORT0_HEAD, SOUND_PORT0_COUNT, command);
    cyc(wide);
}

void abi_SoundPort0QueuePushRawWide(void)
{
    SoundPort0QueuePushRawWide((uint8_t)cpu.a, push_wide(cpu.p));
    cpu.p = nz8(cpu.p, cpu.db);
}

/* SoundPort0QueuePushRawWide on carried registers, as abi_SoundPort0QueuePushRawWide leaves them. */
void SoundPort0QueuePushRawWide_cr(Carried *cr)
{
    SoundPort0QueuePushRawWide((uint8_t)cr->a, push_wide(cr->p));
    cr->p = nz8(cr->p, cr->db);
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
/* wide: push_wide's. */
void SoundPort0QueuePushW(uint8_t command, unsigned wide)
{
    cyc(222 + wide);
    if (rd8(CURRENT_PLAYER) != 0)
        { cyc(10); command |= 0x80; }
    queue_push(SOUND_PORT0_QUEUE, SOUND_PORT0_HEAD, SOUND_PORT0_COUNT, command);
    cyc(wide);
}

/* From C: M 8-bit, X 16-bit. */
void SoundPort0QueuePush(uint8_t command)
{
    SoundPort0QueuePushW(command, 16);
}

void abi_SoundPort0QueuePush(void)
{
    SoundPort0QueuePushW((uint8_t)cpu.a, push_wide(cpu.p));
    cpu.p = nz8(cpu.p, cpu.db);  /* the last pull restores DB */
}

/* SoundPort0QueuePush on carried registers, as abi_SoundPort0QueuePush leaves them. */
void SoundPort0QueuePush_cr(Carried *cr)
{
    SoundPort0QueuePushW((uint8_t)cr->a, push_wide(cr->p));
    cr->p = nz8(cr->p, cr->db);  /* the last pull restores DB */
}

void abi_SoundPort0QueuePushX8(void)
{
    abi_SoundPort0QueuePush();
}

/*
 * @routine 04:D69F SoundPort1QueuePush
 * WHAT IT DOES: Queues a sound command for APU port 1.
 * HOW: queue_push on the port 1 ring with the low byte of A.
 * Entry: any widths; A low byte = command. Exit: every register unchanged
 * except N and Z, which the final pull of DB sets from DB.
 */
void SoundPort1QueuePushW(uint8_t command, unsigned wide)
{
    cyc(176 + wide);
    queue_push(SOUND_PORT1_QUEUE, SOUND_PORT1_HEAD, SOUND_PORT1_COUNT, command);
    cyc(wide);
}

/* From C: M 8-bit, X 16-bit. */
void SoundPort1QueuePush(uint8_t command)
{
    SoundPort1QueuePushW(command, 16);
}

void abi_SoundPort1QueuePush(void)
{
    SoundPort1QueuePushW((uint8_t)cpu.a, push_wide(cpu.p));
    cpu.p = nz8(cpu.p, cpu.db);  /* the last pull restores DB */
}

/* SoundPort1QueuePush on carried registers, as abi_SoundPort1QueuePush leaves them. */
void SoundPort1QueuePush_cr(Carried *cr)
{
    SoundPort1QueuePushW((uint8_t)cr->a, push_wide(cr->p));
    cr->p = nz8(cr->p, cr->db);  /* the last pull restores DB */
}

void abi_SoundPort1QueuePushX8(void)
{
    abi_SoundPort1QueuePush();
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
    hw_wr(APUIO3, 0);
    cyc(32);
    hw_wr(APUIO2, 0);
    cyc(72);
}

void abi_SoundReset(void)
{
    SoundReset();
    cpu.a &= 0xFF00;
    cpu.x &= 0x00FF;
    cpu.y &= 0x00FF;
}

/* SoundReset on carried registers, as abi_SoundReset leaves them. */
void SoundReset_cr(Carried *cr)
{
    SoundReset();
    cr->a &= 0xFF00;
    cr->x &= 0x00FF;
    cr->y &= 0x00FF;
}

/* One ring to its port: 0 when the port still shows a command, else the
   oldest byte (the head steps on, wrapping at 16, unless that was the
   last). Returns 0 when the ring is empty and the port was clear (nothing
   written), else 1 with the byte written in *a (the 8-bit A); *x is the
   8-bit X. */
static int ring_send(uint32_t b, uint16_t port, uint16_t count, uint16_t head, uint16_t ring,
                     uint8_t *a, uint8_t *x)
{
    uint8_t v, n;

    cyc(24);
    v = reg_rd(b | port);
    if (v) {
        cyc(16);
        v = 0x00;
    } else {
        cyc(40);
        n = rd8(b | count);
        *a = n;
        if (!n)
            { cyc(22); return 0; }
        cyc(40);
        *x = rd8(b | head);
        cyc(24);
        v = rd8(b | (uint16_t)(ring + *x));
        cyc(24);
        n = (uint8_t)(rd8(b | count) - 1);
        cyc(6);
        wr8(b | count, n);
        if (n) {
            *x = (uint8_t)(*x + 1);
            if (*x >= 0x10)
                *x = 0x00;
            cyc(92);
            wr8(b | head, *x);
        } else { cyc(22); }
    }
    *a = v;
    cyc(46);
    reg_wr(b | port, v);
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
 * mapped (bank); X's low byte kept unless a head is read. Exit: X and Y high
 * bytes cleared; X = the last head; A (low byte) as the last write; P
 * restored. The function returns the low bytes of A and X.
 */
SoundPortsEnd SoundPortsSend(uint8_t bank, uint8_t x)
{
    uint32_t b = (uint32_t)bank << 16;
    uint8_t a, v;
    SoundPortsEnd end;

    cyc(22);                            /* SEP #$30 */
    cyc(46);
    a = v = rd8(b | 0x10E2);
    if (v) {
        cyc(40);
        hw_wr(b | 0x2140, v);
        cyc(24);
        wr8(b | 0x10E2, 0x00);
        cyc(28);                        /* PLP; the plp's 42 states the RTS */
        goto done;
    }
    cyc(46);
    a = v = rd8(b | 0x10E3);
    if (v) {
        cyc(40);
        hw_wr(b | 0x2141, v);
        cyc(24);
        wr8(b | 0x10E3, 0x00);
        cyc(28);
        goto done;
    } else { cyc(6); }
    {
        static const uint16_t hold[2] = {0x10DA, 0x10DC};
        int i;

        for (i = 0; i < 2; i++) {
            cyc(40);
            a = v = rd8(b | hold[i]);
            if (v) {
                cyc(50);
                goto done;
            }
        }
    }
    cyc(40);
    a = v = rd8(b | 0x0E3B);
    if (!(v & 0x01)) {
        cyc(60);
        goto done;
    } else { cyc(38); }
    ring_send(b, 0x2140, 0x0DCC, 0x0DC8, 0x0DA8, &a, &x);
    if (ring_send(b, 0x2141, 0x0DCE, 0x0DCA, 0x0DB8, &a, &x))
        { cyc(24); wr8(b | 0x0DD2, a); }
    cyc(24);
    a = v = rd8(b | 0x0E4B);
    if (v) {
        a = 0x00;
        cyc(62);
        hw_wr(b | 0x2142, 0x00);
        cyc(24);
        hw_wr(b | 0x2143, 0x00);
    } else {
        cyc(40);
        v = rd8(b | 0x0E47);
        cyc(24);
        hw_wr(b | 0x2142, v);
        cyc(24);
        v = rd8(b | 0x0E49);
        cyc(24);
        hw_wr(b | 0x2143, v);
        a = v;
    }
    cyc(28);
done:
    cyc(42);                            /* PLP */
    end.a = a;
    end.x = x;
    return end;
}

void abi_SoundPortsSend(void)
{
    uint8_t p0 = cpu.p;
    SoundPortsEnd end = SoundPortsSend(cpu.db, (uint8_t)cpu.x);

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | end.a);
    cpu.x = end.x;
    cpu.y &= 0x00FF;
    cpu.p = p0;
}

/* SoundPortsSend on carried registers, as abi_SoundPortsSend leaves them. */
void SoundPortsSend_cr(Carried *cr)
{
    uint8_t p0 = cr->p;
    SoundPortsEnd end = SoundPortsSend(cr->db, (uint8_t)cr->x);

    cr->a = (uint16_t)((cr->a & 0xFF00) | end.a);
    cr->x = end.x;
    cr->y &= 0x00FF;
    cr->p = p0;
}
