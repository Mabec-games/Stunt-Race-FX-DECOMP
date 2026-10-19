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
#include "hw.h"
#include "wram.h"

#define FRAME_UPDATE_FLAGS 0x0011D7u
#define BYTE_WRITES_SIZE 0x00162Du   /* bytes used in the list below */
#define BYTE_WRITES 0x00162Fu        /* entries: 16-bit address, 16-bit value */
#define PALETTE_BUFFER 0x7EFD65u

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
    uint8_t flags = rd8(FRAME_UPDATE_FLAGS);

    *ran = (flags & 0x04) != 0;
    if (!*ran)
        { cyc(38); return 0; }
    cyc(56);
    wr8(DMAP0, 0x00);
    cyc(40);
    wr8(DMAP0 + 1, 0x04);
    cyc(48);
    wr16(A1T0L, OAM_BUFFER);
    cyc(40);
    wr8(A1B0, 0x00);
    cyc(48);
    wr16(DAS0L, 0x0220);
    cyc(48);
    wr16(OAMADDL, 0x0000);
    cyc(40);
    wr8(MDMAEN, 0x01);
    cyc(24);
    flags = rd8(FRAME_UPDATE_FLAGS) & 0xFB;
    cyc(40);
    wr8(FRAME_UPDATE_FLAGS, flags);
    return flags;
}

void OamDmaIfPending(void)
{
    int ran;

    oam_dma_if_pending(&ran);
}

void abi_OamDmaIfPending(void)
{
    int pending;

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | oam_dma_if_pending(&pending));
    if (pending)
        cpu.x = 0;
cyc(32);
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
    uint8_t flags = rd8(FRAME_UPDATE_FLAGS);

    *ran = (flags & 0x08) != 0;
    if (!*ran)
        { cyc(38); return 0; }
    cyc(56);
    wr8(DMAP0, 0x00);
    cyc(40);
    wr8(DMAP0 + 1, 0x22);
    cyc(48);
    wr16(A1T0L, (uint16_t)PALETTE_BUFFER);
    cyc(40);
    wr8(A1B0, (uint8_t)(PALETTE_BUFFER >> 16));
    cyc(48);
    wr16(DAS0L, 0x0200);
    cyc(24);
    wr8(CGADD, 0x00);
    cyc(40);
    wr8(MDMAEN, 0x01);
    cyc(24);
    flags = rd8(FRAME_UPDATE_FLAGS) & 0xF7;
    cyc(40);
    wr8(FRAME_UPDATE_FLAGS, flags);
    return flags;
}

void CgramDmaIfPending(void)
{
    int ran;

    cgram_dma_if_pending(&ran);
}

void abi_CgramDmaIfPending(void)
{
    int pending;

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | cgram_dma_if_pending(&pending));
    if (pending)
        cpu.y = 0x0200;
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
    int ran;
} ByteWritesEnd;

static ByteWritesEnd byte_writes_if_pending(uint8_t bank, uint16_t x0, uint16_t y0)
{
    ByteWritesEnd end = {0, x0, y0, 0, 0};

    if (!(rd8(FRAME_UPDATE_FLAGS) & 0x01))
        return end;
    end.ran = 1;
    {
        uint16_t x = (uint16_t)(rd16(BYTE_WRITES_SIZE) - 4);
        end.a = x;
        if (!(x & 0x8000)) {   /* X and Y are only touched when there is an entry */
            do {
                end.y = rd16(BYTE_WRITES + x);
                end.a = rd16(BYTE_WRITES + 2 + x);
                wr8((uint32_t)bank << 16 | end.y, (uint8_t)end.a);
                x = (uint16_t)(x - 4);
            } while (!(x & 0x8000));
            end.x = x;
        }
    }
    wr16(BYTE_WRITES_SIZE, 0);
    end.flags = rd8(FRAME_UPDATE_FLAGS) & 0xFE;
    wr8(FRAME_UPDATE_FLAGS, end.flags);
    return end;
}

void ByteWritesIfPending(uint8_t bank)
{
    byte_writes_if_pending(bank, 0, 0);
}

void abi_ByteWritesIfPending(void)
{
    ByteWritesEnd end = byte_writes_if_pending(cpu.db, cpu.x, cpu.y);

    cpu.a = (uint16_t)(((end.ran ? end.a : cpu.a) & 0xFF00) | end.flags);
    if (end.ran) {
        cpu.x = end.x;
        cpu.y = end.y;
    }
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
static int vram_dma_queue_add(uint16_t *size_out)
{
    cyc(98);
    uint16_t x = rd16(0x0016AF);

    if (x == 0x0200)
        return 0;
    cyc(64);
    wr16(0x0016B9 + x, cyc_then(32, rd16(0x0016B3)));
    cyc(46);
    wr8(0x0016BB + x, cyc_then(32, rd8(0x0016B5)));
    cyc(46);
    wr16(0x0016BC + x, cyc_then(32, rd16(0x0016B7)));
    cyc(24);
    wr16(0x0016BE + x, cyc_then(32, rd16(0x0016B1)));
    *size_out = (uint16_t)(x + 8);
    cyc(76);
    wr16(0x0016AF, *size_out);
    return 1;
}

/* As VramDmaQueueAdd; returns A as it leaves it from `a` on entry. */
uint16_t VramDmaQueueAddA(uint16_t a)
{
    uint16_t size;

    cyc(62);
    return vram_dma_queue_add(&size) ? size : a;
}

void VramDmaQueueAdd(void)
{
    uint16_t size;

    vram_dma_queue_add(&size);
}

void abi_VramDmaQueueAdd(void)
{
    uint16_t size;

    if (vram_dma_queue_add(&size))
        cpu.a = size;
}

/* Append A to a list of up to eight words; returns X, the new size (the
   old one when the list is full). */
static uint16_t word_list_append(uint32_t size_addr, uint32_t list, uint16_t value)
{
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

static void abi_word_list(uint32_t size_addr, uint32_t list)
{
    uint16_t x = word_list_append(size_addr, list, cpu.a);

    cpu.x = (cpu.p & P_X) ? (uint8_t)x : x;
}

/*
 * @routine 7F:10A1 WordListAAppend
 * WHAT IT DOES: Appends A to the eight-word list at $1195, unless it is full.
 * HOW: Size in bytes at $1191; stores at $1195 + size and adds 2.
 * Entry: any widths. Exit: X = the size (its low byte with 8-bit index
 * registers); P unchanged.
 */
void WordListAAppend(uint16_t value)
{
    word_list_append(0x001191, 0x001195, value);
}

void abi_WordListAAppend(void)
{
    abi_word_list(0x001191, 0x001195);
}

/*
 * @routine 7F:10B7 WordListBAppend
 * WHAT IT DOES: Appends A to the eight-word list at $11B5, unless it is full.
 * HOW: Size in bytes at $1193; stores at $11B5 + size and adds 2.
 * Entry: as WordListAAppend. Exit: as WordListAAppend.
 */
void WordListBAppend(uint16_t value)
{
    word_list_append(0x001193, 0x0011B5, value);
}

void abi_WordListBAppend(void)
{
    abi_word_list(0x001193, 0x0011B5);
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

    if (!(rd8(FRAME_UPDATE_FLAGS) & 0x02))
        return end;
    end.ran = 1;
    wr8(VMAIN, 0x80);
    wr8(DMAP0 + 1, 0x18);
    x = (uint16_t)(rd16(0x0016AF) - 8);
    end.a = x;
    if (!(x & 0x8000)) {
        do {
            wr16(A1T0L, rd16(0x0016B9 + x));
            wr16(VMADDL, rd16(0x0016BC + x));
            wr16(DAS0L, rd16(0x0016BE + x));
            wr8(A1B0, rd8(0x0016BB + x));
            wr8(DMAP0, 0x01);
            wr8(MDMAEN, 0x01);
            x = (uint16_t)(x - 8);
        } while (!(x & 0x8000));
        end.a = x;
        end.x = x;
        end.looped = 1;
    }
    wr16(0x0016AF, 0);
    end.flags = rd8(FRAME_UPDATE_FLAGS) & 0xFD;
    wr8(FRAME_UPDATE_FLAGS, end.flags);
    return end;
}

void VramDmaQueueFlush(void)
{
    vram_dma_queue_flush();
}

void abi_VramDmaQueueFlush(void)
{
    FlushEnd end = vram_dma_queue_flush();

    cpu.a = (uint16_t)(((end.ran ? end.a : cpu.a) & 0xFF00) | end.flags);
    if (end.looped)
        cpu.x = end.x;
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
    uint32_t b = (uint32_t)bank << 16;
    uint16_t x = rd16(b | (uint16_t)BYTE_WRITES_SIZE);

    if (x == 0x0080)
        SystemHalt();
    wr16(BYTE_WRITES + 2 + x, value);
    wr16(BYTE_WRITES + x, addr);
    wr16(b | (uint16_t)BYTE_WRITES_SIZE, (uint16_t)(x + 4));
    return x;
}

void abi_FrameByteWritePush(void)
{
    cpu.x = FrameByteWritePush(cpu.db, cpu.x, (uint8_t)cpu.a);
}

/* The body; returns the value the abi_ wrapper needs. */
static uint8_t frame_input_and_count_end(uint8_t bank, uint16_t *hi, uint16_t *x)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t w;
    uint8_t v;

    cyc(54);
    w = rd16(0x0018BF);
    cyc(32);
    wr16(0x0018BB, w);
    cyc(32);
    w = rd16(0x0018C1);
    cyc(32);
    wr16(0x0018BD, w);
    cyc(24);
    w = rd16(b | 0x0311);
    cyc(114);
    word_list_append(0x001191, 0x001195, w);
    cyc(94);
    w = rd16(b | 0x0313);
    *hi = (uint16_t)(w & 0xFF00);
    cyc(114);
    *x = word_list_append(0x001193, 0x0011B5, w);
    cyc(116);
    wr8(b | 0x0E3B, cyc_then(6, (uint8_t)(rd8(b | 0x0E3B) + 1)));
    cyc(24);
    wr8(b | 0x0306, cyc_then(6, (uint8_t)(rd8(b | 0x0306) + 1)));
    cyc(16);
    wr8(0x000006, cyc_then(6, (uint8_t)(rd8(0x000006) + 1)));
    cyc(24);
    v = rd8(b | 0x11DA);
    cyc(24);
    wr8(b | 0x2100, v);
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
void FrameInputAndCountEnd(uint8_t bank)
{
    uint16_t hi, x;

    frame_input_and_count_end(bank, &hi, &x);
}

void abi_FrameInputAndCountEnd(void)
{
    uint16_t hi, x;
    uint8_t v = frame_input_and_count_end(cpu.db, &hi, &x);

    cpu.a = (uint16_t)(hi | v);
    cpu.x = (cpu.p & P_X) ? (uint8_t)x : x;
    cpu.p = (uint8_t)(nz8(cpu.p, v) | P_M);
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
    wr8(0x000006, cyc_then(6, (uint8_t)(rd8(0x000006) + 1)));
    cyc(24);
    wr8(b | 0x0E3B, cyc_then(6, (uint8_t)(rd8(b | 0x0E3B) + 1)));
    cyc(24);
    wr8(b | 0x0306, cyc_then(6, (uint8_t)(rd8(b | 0x0306) + 1)));
    cyc(78);
    wr16(0x0018FB, 0x0000);
    cyc(32);
    v = rd16(0x001903);
    *high = (uint16_t)(v & 0xFF00);
    *checked = 0;
    if (v) {
        cyc(198);
        int found = JoypadPortCheck(bank, 0);

        cyc(232);
        found |= JoypadPortCheck(bank, 1);
        if (found)
            *high = 0x0000; else { cyc(28); }
        *checked = 1;
    }
    cyc(68);
    lum = rd8(b | 0x11DA);
    cyc(24);
    wr8(b | INIDISP, lum);
    return lum;
}

void abi_FrameCountAndPortsEnd(void)
{
    uint16_t high;
    int checked;
    uint8_t lum = FrameCountAndPortsEnd(cpu.db, &high, &checked);

    cpu.a = (uint16_t)(high | lum);
    if (checked) {
        cpu.x = 0x0001;
        cpu.y = 0x00FF;
    }
    cpu.p = (uint8_t)(nz8(cpu.p, lum) | P_M);
}
