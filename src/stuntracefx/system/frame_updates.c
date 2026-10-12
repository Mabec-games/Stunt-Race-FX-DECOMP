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
static uint8_t oam_dma_if_pending(void)
{
    uint8_t flags = rd8(FRAME_UPDATE_FLAGS);

    if (!(flags & 0x04))
        return 0;
    wr8(DMAP0, 0x00);
    wr8(DMAP0 + 1, 0x04);
    wr16(A1T0L, OAM_BUFFER);
    wr8(A1B0, 0x00);
    wr16(DAS0L, 0x0220);
    wr16(OAMADDL, 0x0000);
    wr8(MDMAEN, 0x01);
    flags = rd8(FRAME_UPDATE_FLAGS) & 0xFB;
    wr8(FRAME_UPDATE_FLAGS, flags);
    return flags;
}

void OamDmaIfPending(void)
{
    oam_dma_if_pending();
}

void abi_OamDmaIfPending(void)
{
    int pending = rd8(FRAME_UPDATE_FLAGS) & 0x04;

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | oam_dma_if_pending());
    if (pending)
        cpu.x = 0;
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
static uint8_t cgram_dma_if_pending(void)
{
    uint8_t flags = rd8(FRAME_UPDATE_FLAGS);

    if (!(flags & 0x08))
        return 0;
    wr8(DMAP0, 0x00);
    wr8(DMAP0 + 1, 0x22);
    wr16(A1T0L, (uint16_t)PALETTE_BUFFER);
    wr8(A1B0, (uint8_t)(PALETTE_BUFFER >> 16));
    wr16(DAS0L, 0x0200);
    wr8(CGADD, 0x00);
    wr8(MDMAEN, 0x01);
    flags = rd8(FRAME_UPDATE_FLAGS) & 0xF7;
    wr8(FRAME_UPDATE_FLAGS, flags);
    return flags;
}

void CgramDmaIfPending(void)
{
    cgram_dma_if_pending();
}

void abi_CgramDmaIfPending(void)
{
    int pending = rd8(FRAME_UPDATE_FLAGS) & 0x08;

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | cgram_dma_if_pending());
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
    uint16_t x = rd16(0x0016AF);

    if (x == 0x0200)
        return 0;
    wr16(0x0016B9 + x, rd16(0x0016B3));
    wr8(0x0016BB + x, rd8(0x0016B5));
    wr16(0x0016BC + x, rd16(0x0016B7));
    wr16(0x0016BE + x, rd16(0x0016B1));
    *size_out = (uint16_t)(x + 8);
    wr16(0x0016AF, *size_out);
    return 1;
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

/* Append A to a list of up to eight words; X keeps the new size. */
static void word_list_append(uint32_t size_addr, uint32_t list, uint16_t value)
{
    uint16_t x = rd16(size_addr);

    if (x == 0x0010)
        return;
    wr16(list + x, value);
    wr16(size_addr, (uint16_t)(x + 2));
}

static void abi_word_list(uint32_t size_addr, uint32_t list)
{
    uint16_t x = rd16(size_addr);

    word_list_append(size_addr, list, cpu.a);
    if (x != 0x0010)
        x = (uint16_t)(x + 2);
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
