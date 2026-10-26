/*
 * dma.c: the CPU's eight DMA channels, as general DMA (MDMAEN) and HDMA.
 *
 * General DMA moves bytes between the A bus (any address but the
 * registers) and one B-bus register ($21xx) in the channel's pattern, 8
 * clocks a byte, the CPU stopped meanwhile (the clock and the lines it
 * passes go on). HDMA runs in each line's horizontal blank, lines 0-224:
 * each channel's table gives a line count (bit 7: transfer every line of
 * the run, else only its first) and the bytes, inline or (indirect) through
 * an address in the table.
 */
#include <string.h>

#include "snes_internal.h"

typedef struct {
    uint8_t dmap, bbad;
    uint16_t a1t;          /* A-bus address */
    uint8_t a1b;           /* its bank */
    uint16_t das;          /* byte count; HDMA's indirect address */
    uint8_t dasb;          /* HDMA's indirect bank */
    uint16_t a2a;          /* HDMA's table address */
    uint8_t ntrl;          /* HDMA's line counter */
    uint8_t unused;
    int done, transfer;    /* HDMA: the table has ended; this line transfers */
} Channel;

static Channel ch[8];
uint8_t dma_hdmaen;

/* the B-bus offsets of each transfer mode's bytes */
static const uint8_t pattern[8][4] = {
    { 0, 0, 0, 0 }, { 0, 1, 0, 1 }, { 0, 0, 0, 0 }, { 0, 0, 1, 1 },
    { 0, 1, 2, 3 }, { 0, 1, 0, 1 }, { 0, 0, 0, 0 }, { 0, 0, 1, 1 },
};
static const uint8_t pattern_len[8] = { 1, 2, 2, 4, 4, 4, 2, 4 };

void dma_reset(void)
{
    int i;
    memset(ch, 0xFF, sizeof ch);
    for (i = 0; i < 8; i++) {
        ch[i].done = 1;
        ch[i].transfer = 0;
    }
    dma_hdmaen = 0;
}

uint8_t dma_read(uint16_t reg)
{
    Channel *c = &ch[(reg >> 4) & 7];
    switch (reg & 0x0F) {
    case 0x0: return c->dmap;
    case 0x1: return c->bbad;
    case 0x2: return (uint8_t)c->a1t;
    case 0x3: return (uint8_t)(c->a1t >> 8);
    case 0x4: return c->a1b;
    case 0x5: return (uint8_t)c->das;
    case 0x6: return (uint8_t)(c->das >> 8);
    case 0x7: return c->dasb;
    case 0x8: return (uint8_t)c->a2a;
    case 0x9: return (uint8_t)(c->a2a >> 8);
    case 0xA: return c->ntrl;
    case 0xB: case 0xF: return c->unused;
    }
    return snes_mdr;
}

void dma_write(uint16_t reg, uint8_t v)
{
    Channel *c = &ch[(reg >> 4) & 7];
    switch (reg & 0x0F) {
    case 0x0: c->dmap = v; return;
    case 0x1: c->bbad = v; return;
    case 0x2: c->a1t = (uint16_t)((c->a1t & 0xFF00) | v); return;
    case 0x3: c->a1t = (uint16_t)((c->a1t & 0x00FF) | v << 8); return;
    case 0x4: c->a1b = v; return;
    case 0x5: c->das = (uint16_t)((c->das & 0xFF00) | v); return;
    case 0x6: c->das = (uint16_t)((c->das & 0x00FF) | v << 8); return;
    case 0x7: c->dasb = v; return;
    case 0x8: c->a2a = (uint16_t)((c->a2a & 0xFF00) | v); return;
    case 0x9: c->a2a = (uint16_t)((c->a2a & 0x00FF) | v << 8); return;
    case 0xA: c->ntrl = v; return;
    case 0xB: case 0xF: c->unused = v; return;
    }
}

/* one byte from A-bus address `a` to B-bus register `b`: to the picture
   (its tiles, palette, sprites and screen state) or work RAM's port */
void (*snes_dma_hook)(uint32_t a, uint8_t b, uint8_t v);

static void move(const Channel *c, uint32_t a, uint8_t b)
{
    uint8_t v;
    if (c->dmap & 0x80)
        snes_fatal("a transfer from register $21%02X: the game makes none", b);
    v = bus_read_a(a);
    if (snes_dma_hook)
        snes_dma_hook(a, b, v);
    if (b < 0x34)
        vid_reg(b, v);
    else
        bus_write_b(b, v);
}

/* the game's calls (native.h) */
void dma_reg(uint8_t reg, uint8_t v) { dma_write((uint16_t)(0x4300 | (reg & 0x7F)), v); }
void dma_go(uint8_t channels) { dma_start(channels); }
void hdma_set(uint8_t channels) { dma_hdmaen = channels; }

/* set: transfers take no time (the port's one-go screen upload, not the
   console's: the CPU is not held, the bytes arrive at once) */
int snes_dma_untimed;

void dma_start(uint8_t channels)
{
    int i;
    if (!snes_dma_untimed)
        snes_tick(18);
    for (i = 0; i < 8; i++) {
        Channel *c = &ch[i];
        unsigned k = 0, mode;
        if (!(channels & (1u << i)))
            continue;
        mode = c->dmap & 7;
        if (!snes_dma_untimed)
            snes_tick(8);
        do {
            move(c, (uint32_t)c->a1b << 16 | c->a1t, (uint8_t)(c->bbad + pattern[mode][k & 3]));
            k++;
            /* bits 3-4: fixed (1, 3), decrement (2) or increment (0) */
            if (!(c->dmap & 0x08))
                c->a1t = (uint16_t)(c->a1t + ((c->dmap & 0x10) ? -1 : 1));
            c->das--;
            if (!snes_dma_untimed)
                snes_tick(8);
        } while (c->das);
    }
}

void hdma_init(void)
{
    int i;
    for (i = 0; i < 8; i++) {
        Channel *c = &ch[i];
        c->done = 1;
        c->transfer = 0;
        if (!(dma_hdmaen & (1u << i)))
            continue;
        c->a2a = c->a1t;
        c->ntrl = bus_read_a((uint32_t)c->a1b << 16 | c->a2a);
        c->a2a++;
        if (c->dmap & 0x40) {
            uint8_t lo = bus_read_a((uint32_t)c->a1b << 16 | c->a2a);
            uint8_t hi = bus_read_a((uint32_t)c->a1b << 16 | (uint16_t)(c->a2a + 1));
            c->das = (uint16_t)(lo | hi << 8);
            c->a2a += 2;
        }
        c->done = c->ntrl == 0;
        c->transfer = !c->done;
    }
}

void hdma_line(void)
{
    int i, any = 0;
    for (i = 0; i < 8; i++) {
        Channel *c = &ch[i];
        unsigned k, mode;
        if (c->done || !(dma_hdmaen & (1u << i)))
            continue;
        any = 1;
        if (c->transfer) {
            mode = c->dmap & 7;
            for (k = 0; k < pattern_len[mode]; k++) {
                uint8_t b = (uint8_t)(c->bbad + pattern[mode][k]);
                if (c->dmap & 0x40) {
                    move(c, (uint32_t)c->dasb << 16 | c->das, b);
                    c->das++;
                } else {
                    move(c, (uint32_t)c->a1b << 16 | c->a2a, b);
                    c->a2a++;
                }
            }
            snes_clock += 8 * k;      /* in the line's event: no events from here */
        }
    }
    for (i = 0; i < 8; i++) {
        Channel *c = &ch[i];
        if (c->done || !(dma_hdmaen & (1u << i)))
            continue;
        c->ntrl--;
        c->transfer = (c->ntrl & 0x80) != 0;
        if (!(c->ntrl & 0x7F)) {
            c->ntrl = bus_read_a((uint32_t)c->a1b << 16 | c->a2a);
            c->a2a++;
            if (c->dmap & 0x40) {
                uint8_t lo = bus_read_a((uint32_t)c->a1b << 16 | c->a2a);
                uint8_t hi = bus_read_a((uint32_t)c->a1b << 16 | (uint16_t)(c->a2a + 1));
                c->das = (uint16_t)(lo | hi << 8);
                c->a2a += 2;
            }
            if (!c->ntrl)
                c->done = 1;
            c->transfer = 1;
        }
    }
    if (any)
        snes_clock += 18;
}
