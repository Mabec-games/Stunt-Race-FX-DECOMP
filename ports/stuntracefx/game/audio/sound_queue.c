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
#include "audio_calls.h"
#include "carried.h"
#include "wram_names.h"
#include "wram_struct.h"


/* Append a byte to a ring, unless sound is muted or the ring is full. The
   count is incremented in place (read again, as INC does): the frame code
   may take a byte from the ring between the first read and the increment. */
static void queue_push(uint32_t queue, uint32_t head, uint32_t count, uint8_t value)
{
    uint8_t n;

    cyc(46);
    if (RD8(sound_muted) != 0)
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
    queue_push(W(sound_port0_queue), W(sound_port0_head), W(sound_port0_count), command);
}

void SoundPort0QueuePush_cr(Carried *cr);
void SoundPort0QueuePushRaw_cr(Carried *cr);
void SoundPort0QueuePushRawWide_cr(Carried *cr);
void SoundPort0QueuePushRawX8_cr(Carried *cr);
void SoundPort0QueuePushX8(Carried *cr);
void SoundPort1QueuePush_cr(Carried *cr);
void SoundPort1QueuePushX8(Carried *cr);
void SoundPortsSend_cr(Carried *cr);
void SoundReset_cr(Carried *cr);

void SoundPort0QueuePushRaw_cr(Carried *cr)
{
    SoundPort0QueuePushRaw((uint8_t)cr->a);
    set_p(cr, nz8(cr->p, cr->db));  /* the last pull restores DB */
    cyc(8);
}

void abi_SoundPort0QueuePushRaw(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    SoundPort0QueuePushRaw_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* From a caller with M and X 8-bit: the pulls take no extra byte. */
void SoundPort0QueuePushRawX8(uint8_t command)
{
    SoundPort0QueuePushRaw(command);
}

void SoundPort0QueuePushRawX8_cr(Carried *cr)
{
    SoundPort0QueuePushRawX8((uint8_t)cr->a);
    set_p(cr, nz8(cr->p, cr->db));
}

void abi_SoundPort0QueuePushRawX8(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    SoundPort0QueuePushRawX8_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* With the caller's widths: wide as for SoundPort0QueuePushW (the entry's
   BRA is queue_push's). */
void SoundPort0QueuePushRawWide(uint8_t command, unsigned wide)
{
    cyc(198 + wide);
    queue_push(W(sound_port0_queue), W(sound_port0_head), W(sound_port0_count), command);
    cyc(wide);
}

void SoundPort0QueuePushRawWide_cr(Carried *cr)
{
    SoundPort0QueuePushRawWide((uint8_t)cr->a, push_wide(cr->p));
    set_p(cr, nz8(cr->p, cr->db));
}

void abi_SoundPort0QueuePushRawWide(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    SoundPort0QueuePushRawWide_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
    if (RD8(current_player) != 0)
        { cyc(10); command |= 0x80; }
    queue_push(W(sound_port0_queue), W(sound_port0_head), W(sound_port0_count), command);
    cyc(wide);
}

/* From C: M 8-bit, X 16-bit. */
void SoundPort0QueuePush(uint8_t command)
{
    SoundPort0QueuePushW(command, 16);
}

void SoundPort0QueuePush_cr(Carried *cr)
{
    SoundPort0QueuePushW((uint8_t)cr->a, push_wide(cr->p));
    set_p(cr, nz8(cr->p, cr->db));  /* the last pull restores DB */
}

void abi_SoundPort0QueuePush(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    SoundPort0QueuePush_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void SoundPort0QueuePushX8(Carried *cr)
{
    SoundPort0QueuePush_cr(cr);
}

void abi_SoundPort0QueuePushX8(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    SoundPort0QueuePushX8(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
    queue_push(W(sound_port1_queue), W(sound_port1_head), W(sound_port1_count), command);
    cyc(wide);
}

/* From C: M 8-bit, X 16-bit. */
void SoundPort1QueuePush(uint8_t command)
{
    SoundPort1QueuePushW(command, 16);
}

void SoundPort1QueuePush_cr(Carried *cr)
{
    SoundPort1QueuePushW((uint8_t)cr->a, push_wide(cr->p));
    set_p(cr, nz8(cr->p, cr->db));  /* the last pull restores DB */
}

void abi_SoundPort1QueuePush(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    SoundPort1QueuePush_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void SoundPort1QueuePushX8(Carried *cr)
{
    SoundPort1QueuePush_cr(cr);
}

void abi_SoundPort1QueuePushX8(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    SoundPort1QueuePushX8(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
    WR8(sound_muted, 0);
    cyc(32);
    WR8(wram_0DD0, 0);
    cyc(32);
    WR8(sound_port1_last, 0);
    cyc(32);
    WR8(sound_port0_head, 0);
    cyc(32);
    WR8(sound_port1_head, 0);
    cyc(32);
    WR8(sound_port1_count, 0);
    cyc(32);
    WR8(sound_port0_count, 0);
    cyc(32);
    WR8(wram_0E1B, 0);
    cyc(32);
    WR8(wram_0E1C, 0);
    cyc(32);
    hw_wr(APUIO3, 0);
    cyc(32);
    hw_wr(APUIO2, 0);
    cyc(72);
}

void SoundReset_cr(Carried *cr)
{
    SoundReset();
    cr->a &= 0xFF00;
    cr->x &= 0x00FF;
    cr->y &= 0x00FF;
}

void abi_SoundReset(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    SoundReset_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
    a = v = RD8(sound_direct0);
    if (v) {
        cyc(40);
        hw_wr(b | 0x2140, v);
        cyc(24);
        WR8(sound_direct0, 0x00);
        cyc(28);                        /* PLP; the plp's 42 states the RTS */
        goto done;
    }
    cyc(46);
    a = v = RD8(sound_direct1);
    if (v) {
        cyc(40);
        hw_wr(b | 0x2141, v);
        cyc(24);
        WR8(sound_direct1, 0x00);
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
    a = v = RD8(frame_count);
    if (!(v & 0x01)) {
        cyc(60);
        goto done;
    } else { cyc(38); }
    ring_send(b, 0x2140, W(sound_port0_count), W(sound_port0_head), W(sound_port0_queue), &a, &x);
    if (ring_send(b, 0x2141, W(sound_port1_count), W(sound_port1_head), W(sound_port1_queue), &a, &x))
        { cyc(24); WR8(sound_port1_last, a); }
    cyc(24);
    a = v = RD8(wram_0E4B);
    if (v) {
        a = 0x00;
        cyc(62);
        hw_wr(b | 0x2142, 0x00);
        cyc(24);
        hw_wr(b | 0x2143, 0x00);
    } else {
        cyc(40);
        v = RD8(near_car_pos);
        cyc(24);
        hw_wr(b | 0x2142, v);
        cyc(24);
        v = RD8(wram_0E49);
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

void SoundPortsSend_cr(Carried *cr)
{
    uint8_t p0 = cr->p;
    SoundPortsEnd end = SoundPortsSend(cr->db, (uint8_t)cr->x);

    cr->a = (uint16_t)((cr->a & 0xFF00) | end.a);
    cr->x = end.x;
    cr->y &= 0x00FF;
    set_p(cr, p0);
}

void abi_SoundPortsSend(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    SoundPortsSend_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

