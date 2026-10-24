/*
 * frame_updates.c: updates the game logic asks for and the menu frame code
 * applies in the next vertical blank.
 *
 * $11D7 holds one flag per pending update: bit 0 a list of byte writes,
 * bit 1 the VRAM transfer queue, bit 2 the sprite table, bit 3 the palette. Each routine here applies its
 * update only when its flag is set, then clears the flag. Other lists collect
 * values for the frame code: two word lists ($1195, $11B5, eight words each)
 * and the VRAM transfer queue at $16B9 (bit 1).
 */
#include "machine.h"
#include "native.h"
#include "hw.h"
#include "system_calls.h"
#include "carried.h"
#include "wram_names.h"
#include "wram_struct.h"



/*
 * @routine 7F:0EC9 OamDmaIfPending
 * WHAT IT DOES: Copies the whole sprite table to OAM if an update is pending.
 * HOW: With bit 2 of the update flags set: channel 0 in mode 0 from $00:0375
 * to OAMDATA for $220 bytes from OAM address 0; then clears the bit.
 * Entry: X 16-bit. Exit: A low byte = the flags after clearing, or 0 when
 * nothing was pending; X = 0 after a copy; P unchanged.
 */
static uint8_t oam_dma_if_pending(int *ran)
{
    cyc(90);
    uint8_t flags = RD8(frame_update_flags);

    *ran = (flags & 0x04) != 0;
    if (!*ran)
        { cyc(38); return 0; }
    cyc(56);
    dma_wr(DMAP0, 0x00);
    cyc(40);
    dma_wr(DMAP0 + 1, 0x04);
    cyc(48);
    dma_wr16(A1T0L, W(oam_buffer));
    cyc(40);
    dma_wr(A1B0, 0x00);
    cyc(48);
    dma_wr16(DAS0L, 0x0220);
    cyc(48);
    ppu_wr16(OAMADDL, 0x0000);
    cyc(40);
    mdmaen_wr(MDMAEN, 0x01);
    cyc(24);
    flags = RD8(frame_update_flags) & 0xFB;
    cyc(40);
    WR8(frame_update_flags, flags);
    return flags;
}

/* What the call left, for the adapter: the flags after clearing (0 when
   nothing was pending) and whether it copied. */
static uint8_t oam_flags;
static int oam_ran;

void OamDmaIfPending(void)
{
    oam_flags = oam_dma_if_pending(&oam_ran);
}

void ByteWritesIfPending_cr(Carried *cr);
void CgramDmaIfPending_cr(Carried *cr);
void FrameByteWritePush_cr(Carried *cr);
void FrameCountAndPortsEnd_cr(Carried *cr);
void FrameInputAndCountEnd_cr(Carried *cr);
void OamDmaIfPending_cr(Carried *cr);
void VramDmaQueueAdd_cr(Carried *cr);
void VramDmaQueueFlush_cr(Carried *cr);
void WordListAAppend_cr(Carried *cr);
void WordListBAppend_cr(Carried *cr);

void OamDmaIfPending_cr(Carried *cr)
{
    OamDmaIfPending();
    cr->a = (uint16_t)((cr->a & 0xFF00) | oam_flags);
    if (oam_ran)
        cr->x = 0;
    cyc(70);
}

void abi_OamDmaIfPending(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    OamDmaIfPending_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 7F:0F03 CgramDmaIfPending
 * WHAT IT DOES: Copies the whole palette buffer to the palette if an update
 * is pending.
 * HOW: With bit 3 of the update flags set: channel 0 in mode 0 from
 * $7E:FD65 to CGDATA for $200 bytes from colour 0; then clears the bit.
 * Entry: X 16-bit. Exit: A low byte = the flags after clearing, or 0 when
 * nothing was pending; Y = $0200 after a copy; P unchanged.
 */
static uint8_t cgram_dma_if_pending(int *ran)
{
    cyc(90);
    uint8_t flags = RD8(frame_update_flags);

    *ran = (flags & 0x08) != 0;
    if (!*ran)
        { cyc(38); return 0; }
    cyc(56);
    dma_wr(DMAP0, 0x00);
    cyc(40);
    dma_wr(DMAP0 + 1, 0x22);
    cyc(48);
    dma_wr16(A1T0L, (uint16_t)W(wram_7EFD65));
    cyc(40);
    dma_wr(A1B0, (uint8_t)(W(wram_7EFD65) >> 16));
    cyc(48);
    dma_wr16(DAS0L, 0x0200);
    cyc(24);
    ppu_wr(CGADD, 0x00);
    cyc(40);
    mdmaen_wr(MDMAEN, 0x01);
    cyc(24);
    flags = RD8(frame_update_flags) & 0xF7;
    cyc(40);
    WR8(frame_update_flags, flags);
    return flags;
}

/* As oam_flags and oam_ran. */
static uint8_t cgram_flags;
static int cgram_ran;

void CgramDmaIfPending(void)
{
    cgram_flags = cgram_dma_if_pending(&cgram_ran);
}

void CgramDmaIfPending_cr(Carried *cr)
{
    CgramDmaIfPending();
    cr->a = (uint16_t)((cr->a & 0xFF00) | cgram_flags);
    if (cgram_ran)
        cr->y = 0x0200;
    cyc(70);
}

void abi_CgramDmaIfPending(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CgramDmaIfPending_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 7F:0F3A ByteWritesIfPending
 * WHAT IT DOES: Applies the queued byte writes if any are pending.
 * HOW: With bit 0 of the update flags set: walks the list from its last
 * entry to its first, storing each entry's low value byte at its address in
 * the data bank; then empties the list and clears the bit.
 * Entry: X 16-bit; DB the bank the addresses are in. Exit: A low byte = the
 * flags after clearing, or 0 when nothing was pending; after a pass A's high
 * byte is that of the last value (or of the size minus 4), X is
 * negative (one entry before the first) and Y the last address used; P
 * unchanged.
 */
typedef struct {
    uint8_t flags;
    uint16_t x, y;
    uint16_t a;     /* the last 16-bit value A held: its high byte survives */
    int ran, looped;
} ByteWritesEnd;

static ByteWritesEnd byte_writes_if_pending(uint8_t bank, uint16_t x0, uint16_t y0)
{
    ByteWritesEnd end = {0, x0, y0, 0, 0, 0};

    cyc(90);
    if (!(RD8(frame_update_flags) & 0x01))
        { cyc(38); return end; }
    end.ran = 1;
    {
        cyc(78);
        uint16_t x = (uint16_t)(RD16(byte_writes_size) - 4);
        end.a = x;
        if (!(x & 0x8000)) {   /* X and Y are only touched when there is an entry */
            do {
                cyc(100);
                end.y = RD16X(byte_writes, x);
                cyc(46);
                end.a = RD16X(byte_writes, 2 + x);
                cyc(52);
                reg_wr((uint32_t)bank << 16 | end.y, (uint8_t)end.a);
                x = (uint16_t)(x - 4);
            } while (cyc_loop(32, !(x & 0x8000)));
            cyc(34);
            end.x = x;
            end.looped = 1;
        }
    }
    cyc(84);
    WR16(byte_writes_size, 0);
    cyc(46);
    end.flags = RD8(frame_update_flags) & 0xFE;
    cyc(40);
    WR8(frame_update_flags, end.flags);
    return end;
}

/* What the call left, for the adapter. */
static ByteWritesEnd byte_writes_end;

void ByteWritesIfPending(uint8_t bank)
{
    byte_writes_end = byte_writes_if_pending(bank, 0, 0);
}

void ByteWritesIfPending_cr(Carried *cr)
{
    ByteWritesEnd end;

    ByteWritesIfPending(cr->db);
    end = byte_writes_end;
    cr->a = (uint16_t)(((end.ran ? end.a : cr->a) & 0xFF00) | end.flags);
    if (end.looped) {
        cr->x = end.x;
        cr->y = end.y;
    }
    cyc(70);
}

void abi_ByteWritesIfPending(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ByteWritesIfPending_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 7F:1007 VramDmaQueueAdd
 * WHAT IT DOES: Queues the VRAM transfer staged at $16B1-$16B8 for the
 * next vertical blank, unless the queue is full ($200 bytes, 64 transfers).
 * HOW: Copies the source address ($16B3), source bank ($16B5), VRAM address
 * ($16B7) and byte count ($16B1) into the next 8-byte slot, in that order,
 * and advances the queue size by 8. VramDmaQueueFlush runs the queue.
 * Entry: any widths. Exit: A = the new size when appended; P, X unchanged.
 */
static int queue_added;     /* whether the last add appended (the adapter's) */

static int vram_dma_queue_add(uint16_t *size_out)
{
    cyc(98);
    uint16_t x = RD16(vram_queue_size);

    queue_added = 0;
    if (x == 0x0200)
        return 0;
    queue_added = 1;
    cyc(64);
    WR16X(vram_queue, x, cyc_then(32, RD16(wram_16B3)));
    cyc(46);
    WR8X(wram_16BB, x, cyc_then(32, RD8(wram_16B5)));
    cyc(46);
    WR16X(wram_16BC, x, cyc_then(32, RD16(wram_16B7)));
    cyc(24);
    WR16X(wram_16BE, x, cyc_then(32, RD16(vram_queue_staged)));
    *size_out = (uint16_t)(x + 8);
    cyc(76);
    WR16(vram_queue_size, *size_out);
    return 1;
}

/* As VramDmaQueueAdd; returns A as it leaves it from `a` on entry. */
uint16_t VramDmaQueueAddA(uint16_t a)
{
    uint16_t size;

    return vram_dma_queue_add(&size) ? size : a;
}

void VramDmaQueueAdd(void)
{
    cyc(0);
    uint16_t size;

    vram_dma_queue_add(&size);
    cyc(108);
}

/* As VramDmaQueueAdd with its whole time: A from `a` on entry as it
   leaves it. */
uint16_t VramDmaQueueAddTimed(uint16_t a)
{
    a = VramDmaQueueAddA(a);
    if (queue_added)
        cyc(108);
    return a;
}

void VramDmaQueueAdd_cr(Carried *cr)
{
    cr->a = VramDmaQueueAddTimed(cr->a);
}

void abi_VramDmaQueueAdd(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    VramDmaQueueAdd_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* Append A to a list of up to eight words; returns X, the new size (the
   old one when the list is full). */
static uint16_t word_list_append(uint32_t size_addr, uint32_t list, uint16_t value)
{
    cyc(114);
    uint16_t x = rd16(size_addr);

    if (x == 0x0010)
        { cyc(46); return x; }
    cyc(72);
    wr16(list + x, value);
    x = (uint16_t)(x + 2);
    cyc(52);
    wr16(size_addr, x);
    return x;
}

/*
 * @routine 7F:10A1 WordListAAppend
 * WHAT IT DOES: Appends A to the eight-word list at $1195, unless it is full.
 * HOW: Size in bytes at $1191; stores at $1195 + size and adds 2.
 * Entry: any widths. Exit: X = the size (its low byte with 8-bit index
 * registers); P unchanged.
 */
uint16_t WordListAAppend(uint16_t value)
{
    return word_list_append(WA(word_list_a_size), WA(word_list_a), value);
}

void WordListAAppend_cr(Carried *cr)
{
    uint16_t x = WordListAAppend(cr->a);

    cr->x = (cr->p & P_X) ? (uint8_t)x : x;
}

void abi_WordListAAppend(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    WordListAAppend_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 7F:10B7 WordListBAppend
 * WHAT IT DOES: Appends A to the eight-word list at $11B5, unless it is full.
 * HOW: Size in bytes at $1193; stores at $11B5 + size and adds 2.
 * Entry: as WordListAAppend. Exit: as WordListAAppend.
 */
uint16_t WordListBAppend(uint16_t value)
{
    return word_list_append(WA(word_list_b_size), WA(word_list_b), value);
}

void WordListBAppend_cr(Carried *cr)
{
    uint16_t x = WordListBAppend(cr->a);

    cr->x = (cr->p & P_X) ? (uint8_t)x : x;
}

void abi_WordListBAppend(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    WordListBAppend_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 7F:0F77 VramDmaQueueFlush
 * WHAT IT DOES: Runs the queued VRAM transfers if any are pending.
 * HOW: With bit 1 of the update flags set: sets VRAM to increment after the
 * high byte and channel 0's target to VMDATA, then for each queued transfer,
 * last first, sets the source address, VRAM address, count and bank and
 * starts it (mode 1); then empties the queue and clears the bit.
 * Entry: X 16-bit. Exit: A low byte = the flags after clearing, or 0 when
 * nothing was pending; when pending, A's high byte is that of the last
 * 16-bit value it held and X, after a pass, is negative; P unchanged.
 */
typedef struct {
    uint8_t flags;
    uint16_t a, x;
    int ran, looped;
} FlushEnd;

static FlushEnd vram_dma_queue_flush(void)
{
    FlushEnd end = {0, 0, 0, 0, 0};
    uint16_t x;

    cyc(90);
    if (!(RD8(frame_update_flags) & 0x02))
        { cyc(38); return end; }
    end.ran = 1;
    cyc(72);
    ppu_wr(VMAIN, 0x80);
    cyc(40);
    dma_wr(DMAP0 + 1, 0x18);
    cyc(46);
    x = (uint16_t)(RD16(vram_queue_size) - 8);
    end.a = x;
    if (!(x & 0x8000)) {
        do {
            cyc(122);
            dma_wr16(A1T0L, cyc_then(24, RD16X(vram_queue, x)));
            cyc(32);
            ppu_wr16(VMADDL, cyc_then(24, RD16X(wram_16BC, x)));
            cyc(32);
            dma_wr16(DAS0L, cyc_then(24, RD16X(wram_16BE, x)));
            cyc(54);
            dma_wr(A1B0, cyc_then(24, RD8X(wram_16BB, x)));
            cyc(40);
            dma_wr(DMAP0, 0x01);
            cyc(24);
            mdmaen_wr(MDMAEN, 0x01);
            x = (uint16_t)(x - 8);
        } while (cyc_loop(42, !(x & 0x8000)));
        cyc(44);
        end.a = x;
        end.x = x;
        end.looped = 1;
    }
    cyc(106);
    WR16(vram_queue_size, 0);
    cyc(46);
    end.flags = RD8(frame_update_flags) & 0xFD;
    cyc(40);
    WR8(frame_update_flags, end.flags);
    return end;
}

/* What the call left, for the adapter. */
static FlushEnd flush_end;

void VramDmaQueueFlush(void)
{
    flush_end = vram_dma_queue_flush();
}

void VramDmaQueueFlush_cr(Carried *cr)
{
    FlushEnd end;

    VramDmaQueueFlush();
    end = flush_end;
    cr->a = (uint16_t)(((end.ran ? end.a : cr->a) & 0xFF00) | end.flags);
    if (end.looped)
        cr->x = end.x;
    cyc(70);
}

void abi_VramDmaQueueFlush(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    VramDmaQueueFlush_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void SystemHalt(void);

/*
 * @routine 0B:E3F7 FrameByteWritePush
 * WHAT IT DOES: Adds a byte write to the list ByteWritesIfPending applies in
 * the next vertical blank: address X, value the low byte of A. A full list
 * (32 entries) stops the game (SystemHalt); no script fills it.
 * HOW: The entry (address word, value word) goes in at the list's size,
 * which grows by 4. A, X and P are pushed first; A and P come back, X does
 * not.
 * Entry: any widths; A low byte = value, X = address (16 bits are used).
 * Exit: A and P unchanged; X = the entry's offset; Y unchanged. A full
 * list does not return.
 */
uint16_t FrameByteWritePush(uint8_t bank, uint16_t addr, uint8_t value)
{
    (void)bank;
    cyc(182);
    uint16_t x = rd16(0x7E0000 | (uint16_t)W(byte_writes_size));

    if (x == 0x0080)
        SystemHalt();
    cyc(108);
    WR16X(byte_writes, 2 + x, value);
    cyc(68);
    WR16X(byte_writes, x, addr);
    cyc(76);
    wr16(0x7E0000 | (uint16_t)W(byte_writes_size), (uint16_t)(x + 4));
    return x;
}

void FrameByteWritePush_cr(Carried *cr)
{
    cr->x = FrameByteWritePush(cr->db, cr->x, (uint8_t)cr->a);
}

void abi_FrameByteWritePush(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    FrameByteWritePush_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* The body; returns the value the abi_ wrapper needs. */
static uint8_t frame_input_and_count_end(uint8_t bank, uint16_t *hi, uint16_t *x)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t w;
    uint8_t v;

    cyc(54);
    w = RD16(menu_sine_y);
    cyc(32);
    WR16(menu_pos_18bb, w);
    cyc(32);
    w = RD16(menu_sine_y_min);
    cyc(32);
    WR16(menu_pos_18bd, w);
    cyc(24);
    w = RD16(joy1_pressed);
    word_list_append(WA(word_list_a_size), WA(word_list_a), w);
    cyc(94);
    w = RD16(joy2_pressed);
    *hi = (uint16_t)(w & 0xFF00);
    *x = word_list_append(WA(word_list_b_size), WA(word_list_b), w);
    cyc(116);
    WR8(frame_count, cyc_then(6, (uint8_t)(RD8(frame_count) + 1)));
    cyc(24);
    WR8(frames_since_tick, cyc_then(6, (uint8_t)(RD8(frames_since_tick) + 1)));
    cyc(16);
    WR8(dp_06, cyc_then(6, (uint8_t)(RD8(dp_06) + 1)));
    cyc(24);
    v = RD8(wram_11DA);
    cyc(24);
    ppu_wr(b | 0x2100, v);
    return v;
}

/*
 * @routine 7F:1072 FrameInputAndCountEnd
 * WHAT IT DOES: Ends a menu frame: copies $18BF-$18C2 to $18BB-$18BE,
 * appends player 1's and player 2's new presses to the two word lists,
 * steps the frame counters ($0E3B, $0306, direct page $06) and writes the
 * brightness value at $11DA to INIDISP.
 * HOW: Two long word copies; $0311 to WordListAAppend, $0313 to
 * WordListBAppend; three 8-bit increments; the rest through the data bank.
 * Entry: M 16-bit, D = $0000. Exit: A low byte = $11DA, high byte from
 * $0314; X = list B's size; M 8-bit; N, Z from $11DA.
 */
/* What the call left besides the return value, for the adapter: A's high
   byte and list B's size. */
static uint16_t input_hi, input_x;

uint8_t FrameInputAndCountEnd(uint8_t bank)
{
    return frame_input_and_count_end(bank, &input_hi, &input_x);
}

void FrameInputAndCountEnd_cr(Carried *cr)
{
    uint8_t v = FrameInputAndCountEnd(cr->db);
    uint16_t hi = input_hi, x = input_x;

    cr->a = (uint16_t)(hi | v);
    cr->x = (cr->p & P_X) ? (cyc(44), (uint8_t)x) : x;
    set_p(cr, (uint8_t)(nz8(cr->p, v) | P_M));
}

void abi_FrameInputAndCountEnd(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    FrameInputAndCountEnd_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

int JoypadPortCheck(uint8_t bank, uint8_t port);

/*
 * @routine 7F:1042 FrameCountAndPortsEnd
 * WHAT IT DOES: Ends a frame of the main game loop: steps the frame
 * counters (direct page $06, $0E3B, $0306), clears $18FB and, when $1903
 * is set, checks both joypad ports for a device (JoypadPortCheck, port 0
 * then 1); then writes the brightness value at $11DA to INIDISP.
 * HOW: Three 8-bit increments; $18FB and $1903 with long addressing; the
 * rest through the data bank.
 * Entry: M 16-bit, X 8-bit, D = $0000. Exit: A low byte = $11DA, high
 * byte 0, or $1903's high byte when the ports were checked and none set
 * $18FB; X = 1 and Y = $FF when the ports were checked, else unchanged;
 * M 8-bit; N, Z from $11DA; C, V unchanged.
 */
uint8_t FrameCountAndPortsEnd(uint8_t bank, uint16_t *high, int *checked)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t v;
    uint8_t lum;

    cyc(38);
    WR8(dp_06, cyc_then(6, (uint8_t)(RD8(dp_06) + 1)));
    cyc(24);
    WR8(frame_count, cyc_then(6, (uint8_t)(RD8(frame_count) + 1)));
    cyc(24);
    WR8(frames_since_tick, cyc_then(6, (uint8_t)(RD8(frames_since_tick) + 1)));
    cyc(78);
    WR16(wram_18FB, 0x0000);
    cyc(32);
    v = RD16(wram_1903);
    *high = (uint16_t)(v & 0xFF00);
    *checked = 0;
    if (v) {
        int found = JoypadPortCheck(bank, 0);

        cyc(34);
        found |= JoypadPortCheck(bank, 1);
        if (found)
            *high = 0x0000; else { cyc(28); }
        *checked = 1;
    }
    cyc(68);
    lum = RD8(wram_11DA);
    cyc(24);
    ppu_wr(b | INIDISP, lum);
    return lum;
}

void FrameCountAndPortsEnd_cr(Carried *cr)
{
    uint16_t high;
    int checked;
    uint8_t lum = FrameCountAndPortsEnd(cr->db, &high, &checked);

    cr->a = (uint16_t)(high | lum);
    if (checked) {
        cr->x = 0x0001;
        cr->y = 0x00FF;
    }
    cyc(44);
    set_p(cr, (uint8_t)(nz8(cr->p, lum) | P_M));
}

void abi_FrameCountAndPortsEnd(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    FrameCountAndPortsEnd_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

