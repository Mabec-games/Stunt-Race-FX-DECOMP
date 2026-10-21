/*
 * canvas.c: the Super FX's screen as operations (draw_list.h fx_op_*): the
 * buffer cleared, then every row the drawers make, in order. Today they
 * are only recorded for the check; the GPU's 3D canvas is filled from
 * them (rr.h rr_fill) once they make the whole screen.
 *
 * PORT_CANVASCHECK=1: the operations since each clear filled here
 * (fx_fill_span), against what the S-CPU copies of that buffer into VRAM
 * as the screen (fx_op_upload): each byte a DMA moves from the buffer
 * against the same byte of the operations' pixels at that moment (a bit
 * plane of eight pixels, as PLOT lays them out). At exit: the buffers,
 * those with a byte that differs (and the first: its frame and number),
 * the bytes compared and those that differ, the rows made, and the
 * clears of a buffer other than the one PLOT draws in. PORT_CANVASDUMP=N
 * also writes the N-th buffer as canvas_N.pgm (in the directory the port
 * runs in): the operations' pixels at the next clear, the uploaded ones
 * (0 where not all their planes went up), and white where an uploaded
 * byte differed, side by side.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "snes.h"
#include "gsu.h"
#include "draw_list.h"

extern const uint8_t srfx_rom[], srfx_rom_end[];

static int on = -1, have, uploading;
static FxCanvas *ops;
static uint8_t up_px[256 * 256], up_planes[256 * 256];   /* the uploaded pixels, the planes that went up */
static uint8_t up_bad[256 * 256];                         /* pixels in a byte that differed */
static long buffer_bad;                                   /* this buffer's bytes that differed */
static FxDrawSource src;
static uint8_t ht, md, scbr;        /* the screen the operations were made for */
static long buffers, bad_buffers, bad_bytes, compared, rows, first_bad = -1, first_buffer = -1, elsewhere, other_layout;

static unsigned bpp_of(void) { return 2u << (md - (md >> 1)); }

static void finish(void);

static void report(void)
{
    if (have)
        finish();
    fprintf(stderr, "canvascheck: %ld buffers, %ld differ (first at frame %ld, buffer %ld), %ld of %ld bytes uploaded; "
            "%ld rows made; %ld cleared elsewhere than PLOT draws; %ld bytes uploaded in the sprite layout\n", buffers,
            bad_buffers, first_bad, first_buffer, bad_bytes, compared, rows, elsewhere, other_layout);
}

/* a byte of the buffer copied to VRAM: its eight pixels' bit plane */
static void uploaded(uint32_t a, uint8_t b, uint8_t v)
{
    unsigned base = (unsigned)scbr << 10, bpp = bpp_of(), o, cn, k, plane, rows_of, x0, y, i;
    if (!have || !uploading || (b != 0x18 && b != 0x19) || (a >> 16) != 0x70 || (a & 0xFFFFu) < base ||
        (a & 0xFFFFu) >= base + 0x6800u)
        return;
    if (ht == 3) {
        other_layout++;
        return;
    }
    o = (a & 0xFFFFu) - base;
    cn = o / (bpp * 8u);
    k = o % (bpp * 8u);
    plane = (k >> 4) * 2u + (k & 1u);
    rows_of = ht == 0 ? 16u : ht == 1 ? 20u : 24u;
    x0 = cn / rows_of * 8u;
    y = cn % rows_of * 8u + ((k & 15u) >> 1);
    if (x0 >= 256u)
        return;
    {
        uint8_t want = 0;
        for (i = 0; i < 8; i++)
            want |= (uint8_t)(((ops->px[y << 8 | (x0 + i)] >> plane) & 1u) << (7 - i));
        compared++;
        if (want != v) {
            buffer_bad++;
            for (i = 0; i < 8; i++)
                up_bad[y << 8 | (x0 + i)] = 1;
        }
    }
    for (i = 0; i < 8; i++) {
        unsigned at = y << 8 | (x0 + i);
        up_px[at] = (uint8_t)((up_px[at] & ~(1u << plane)) | ((v >> (7 - i)) & 1u) << plane);
        up_planes[at] |= (uint8_t)(1u << plane);
    }
}

/* the buffer done: its uploaded pixels against the operations' */
static void finish(void)
{
    unsigned full = (1u << bpp_of()) - 1u;
    long d = buffer_bad;
    buffers++;
    if (getenv("PORT_CANVASDUMP") && buffers == atol(getenv("PORT_CANVASDUMP"))) {
        char name[64];
        FILE *f;
        unsigned x, y;
        snprintf(name, sizeof name, "canvas_%ld.pgm", buffers);
        if ((f = fopen(name, "wb"))) {
            fprintf(f, "P5 768 256 255\n");
            for (y = 0; y < 256; y++) {
                for (x = 0; x < 256; x++)
                    fputc(ops->px[y << 8 | x], f);
                for (x = 0; x < 256; x++)
                    fputc(up_planes[y << 8 | x] == full ? up_px[y << 8 | x] : 0, f);
                for (x = 0; x < 256; x++)
                    fputc(up_bad[y << 8 | x] ? 255 : 0, f);
            }
            fclose(f);
        }
        fprintf(stderr, "canvasdump: buffer %ld, frame %ld, layout %u, mode %u, base %u KB, %ld bytes differed\n", buffers,
                (long)snes_frame(), ht, md, scbr, d);
    }
    if (d) {
        if (first_bad < 0) {
            first_bad = (long)snes_frame();
            first_buffer = buffers;
        }
        bad_buffers++;
        bad_bytes += d;
    }
}

void fx_op_clear(void)
{
    if (on < 0) {
        on = getenv("PORT_CANVASCHECK") != NULL;
        if (on) {
            ops = (FxCanvas *)malloc(sizeof *ops);
            snes_dma_hook = uploaded;
            src.rom = srfx_rom;
            src.rom_size = (uint32_t)(srfx_rom_end - srfx_rom);
            atexit(report);
        }
    }
    if (!on)
        return;
    if (have)
        finish();
    memset(up_planes, 0, sizeof up_planes);
    memset(up_bad, 0, sizeof up_bad);
    buffer_bad = 0;
    memset(ops->px, 0, sizeof ops->px);
    memset(ops->set, 1, sizeof ops->set);
    ht = (uint8_t)((gsu.por & 0x10) ? 3 : ((gsu.scmr >> 2) & 1) | ((gsu.scmr >> 4) & 2));
    md = (uint8_t)(gsu.scmr & 3);
    scbr = gsu.scbr;
    {
        const uint8_t *ram = snes_sfx_ram();
        if ((unsigned)(ram[0x021A] | ram[0x021B] << 8) != (unsigned)scbr << 10)
            elsewhere++;
    }
    have = 1;
}

void fx_op_ramp_span(FxSpan *s, uint16_t ramp)
{
    const uint8_t *ram;
    int i;
    if (on <= 0 || !have)
        return;
    ram = snes_sfx_ram();
    s->palette = 0;
    for (i = 0; i < 4; i++)
        s->palette |= (uint32_t)ram[(uint16_t)(ramp + 2 * i)] << (8 * i);
    s->tv = (uint16_t)(ram[(uint16_t)(ramp + 8)] | ram[(uint16_t)(ramp + 10)] << 8);
    s->dv = (uint16_t)(ram[(uint16_t)(ramp + 12)] | ram[(uint16_t)(ramp + 14)] << 8);
    fx_op_span(s);
}

void fx_op_upload(int up) { uploading = up; }

void fx_op_span(const FxSpan *s)
{
    if (on <= 0 || !have)
        return;
    rows++;
    fx_fill_span(&src, ops, s);
}

void fx_op_palette_span(FxSpan *s, uint16_t palette)
{
    const uint8_t *ram;
    if (on <= 0 || !have)
        return;
    ram = snes_sfx_ram();                /* 64 KB: RAMBR's bank is the same bytes */
    s->palette = (uint32_t)ram[(uint16_t)palette] | (uint32_t)ram[(uint16_t)(palette + 1)] << 8 |
                 (uint32_t)ram[(uint16_t)(palette + 2)] << 16 | (uint32_t)ram[(uint16_t)(palette + 3)] << 24;
    fx_op_span(s);
}

static void list_row(void *ctx, const FxSpan *s)
{
    (void)ctx;
    fx_op_span(s);
}

/* a draw list's rows (from fx_list_begin, as FxDrawList starts) */
void canvas_list(const FxDrawSource *s)
{
    if (on <= 0 || !have)
        return;
    fx_list_spans(s, list_row, NULL);
}
