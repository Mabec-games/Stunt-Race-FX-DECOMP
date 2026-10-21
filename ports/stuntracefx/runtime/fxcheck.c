/*
 * fxcheck.c: the check of the game's fill lifted to plain C
 * (game/render/draw_list.c) against the Super FX's own (PORT_FXCHECK=1).
 * As FxDrawList starts, the lifted fill draws the same list into one
 * canvas; while it runs, each pixel the Super FX plots goes into another;
 * as it ends, the two are compared. At exit: the lists drawn and the
 * pixels the Super FX wrote in them, those that differed, and the pixels it
 * plotted outside any list.
 *
 * PORT_FXGPUCHECK=1 (headless, a GPU renderer): each list's spans filled
 * by the renderer's 3D canvas (render/fill/core.glsl), read back and
 * compared with the same list drawn here (fx_draw_list). At exit: the
 * lists, the pixels written, the lists and pixels that differed, and the
 * rows that wrapped past x 255. First, a list of random spans for each
 * colour depth (every option, rows that wrap, textures anywhere in the ROM)
 * the same way: the cases the plays do not reach.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "snes.h"
#include "gsu.h"
#include "draw_list.h"
#include "dispatch.h"
#include "rr.h"

extern const uint8_t srfx_rom[], srfx_rom_end[];

static int on = -1, open_now;
static FxCanvas *mine, *chip;
static long lists, bad_lists, bad_pixels, first_bad = -1, pixels, outside;

static void chip_plot(uint8_t x, uint8_t y, uint8_t c)
{
    if (!open_now) {
        outside++;
        return;
    }
    chip->px[y << 8 | x] = c;
    chip->set[y << 8 | x] = 1;
}

int port_rr_headless(void);

static int gpu_on = -1;
static long g_lists, g_pixels, g_bad, g_bad_pixels, g_first = -1, g_wraps;
static uint32_t *g_spans;
static int g_n, g_cap;
static uint16_t g_canvas[256 * 256];

static void g_report(void)
{
    fprintf(stderr, "fxgpucheck: %s, %ld lists (%ld pixels), %ld differ (first at frame %ld), %ld pixels; %ld rows wrapped past x 255\n",
            rr_name(), g_lists, g_pixels, g_bad, g_first, g_bad_pixels, g_wraps);
}

void fx_span_pack(const FxSpan *p, uint32_t *w);

/* a span packed as rr.h's span words */
static void g_span(void *ctx, const FxSpan *p)
{
    (void)ctx;
    if (g_n == g_cap) {
        g_cap = g_cap ? g_cap * 2 : 4096;
        g_spans = (uint32_t *)realloc(g_spans, (size_t)g_cap * RR_SPAN_WORDS * 4);
    }
    if (p->x + p->n > 256)
        g_wraps++;
    fx_span_pack(p, g_spans + (size_t)g_n++ * RR_SPAN_WORDS);
}

/* the random lists: the pixels that differ */
static long packed_bad;

static long g_random(FxCanvas *ref)
{
    static uint8_t ram[0x10000];
    uint32_t seed = 12345;
    long d = 0;
    int mode, i, k;
    for (mode = 0; mode < 4; mode++) {
        FxDrawSource s;
        s.ram = ram;
        s.rom = srfx_rom;
        s.rom_size = (uint32_t)(srfx_rom_end - srfx_rom);
        s.mode = mode;
        s.layout = 3;
        memset(ref, 0, sizeof *ref);
        g_n = 0;
        for (i = 0; i < 4000; i++) {
            FxSpan p;
            uint32_t r[6];
            for (k = 0; k < 6; k++) {
                seed = seed * 1103515245u + 12345u;
                r[k] = seed >> 8;
            }
            p.x = (uint8_t)r[0];
            p.y = (uint8_t)(r[0] >> 8);
            p.n = (uint16_t)(1 + (r[0] >> 16) % 256);
            p.colour = (uint8_t)r[1];
            p.options = (uint8_t)((r[1] >> 8) & 0x1F);
            p.kind = (uint8_t)((r[1] >> 13) % 6);
            p.palette = r[2] ^ r[5] << 8;
            p.bank = (uint8_t)(r[1] >> 16);
            p.tu = (uint16_t)r[2];
            p.tv = (uint16_t)r[3];
            p.du = (uint16_t)r[4];
            p.dv = (uint16_t)(p.kind == 2 ? (r[4] >> 8) & 1 : r[4] >> 8);
            p.base = (uint16_t)r[5];
            p.mask = (uint16_t)(r[5] >> 4);
            p.mode = (uint8_t)mode;
            p.layout = (uint8_t)(r[3] >> 20 & 3);
            g_span(&mode, &p);
            fx_fill_span(&s, ref, &p);
        }
        rr_fill(g_spans, g_n, 1, 0);
        rr_fill_read(g_canvas);
        for (i = 0; i < 256 * 256; i++)
            if (g_canvas[i] != (ref->set[i] ? (uint16_t)(0x100 | ref->px[i]) : 0))
                d++;
        /* the canvas packed as each layout lays it out at this depth: the
           buffer's first 64 KB (or less) as runs of 256 bytes, against the
           same bytes from the canvas here */
        for (k = 0; k < 3; k++) {
            static uint32_t runs[256 * RR_RUN_WORDS];
            static uint8_t got[65536];
            unsigned bpp = 2u << (mode - (mode >> 1)), rows_of = k == 0 ? 16u : k == 1 ? 20u : 24u, r;
            for (r = 0; r < 256; r++) {
                runs[r * RR_RUN_WORDS + 0] = r << 8 | 256u << 16;
                runs[r * RR_RUN_WORDS + 1] = r << 8;
                runs[r * RR_RUN_WORDS + 2] = (uint32_t)k | (uint32_t)mode << 2;
                runs[r * RR_RUN_WORDS + 3] = 0;
            }
            rr_pack(runs, 256, 0);
            if (!rr_pack_read(got))
                break;
            for (i = 0; i < 65536; i++) {
                unsigned o = (unsigned)i, cn = o / (bpp * 8u), kk = o % (bpp * 8u), plane = (kk >> 4) * 2u + (kk & 1u);
                unsigned x0 = cn / rows_of * 8u, y = cn % rows_of * 8u + ((kk & 15u) >> 1), b;
                uint8_t want = 0;
                if (x0 < 256u && y < 256u)
                    for (b = 0; b < 8; b++)
                        want |= (uint8_t)(((g_canvas[y << 8 | (x0 + b)] >> plane) & 1u) << (7 - b));
                if (got[i] != want)
                    packed_bad++;
            }
        }
    }
    g_wraps = 0;
    return d;
}

static void gpu_list(const FxDrawSource *s)
{
    static FxCanvas *ref;
    long d = 0;
    int i;
    if (gpu_on < 0) {
        gpu_on = getenv("PORT_FXGPUCHECK") != NULL;
        if (gpu_on) {
            if (!port_rr_headless() || !rr_fill_rom(srfx_rom, (uint32_t)(srfx_rom_end - srfx_rom))) {
                fprintf(stderr, "fxgpucheck: the renderer has no 3D canvas\n");
                exit(2);
            }
            ref = (FxCanvas *)malloc(sizeof *ref);
            {
                long d = g_random(ref);
                fprintf(stderr, "fxgpucheck: %s, random spans: %ld pixels differ; packed: %ld bytes differ\n", rr_name(), d,
                        packed_bad);
            }
            atexit(g_report);
        }
    }
    if (!gpu_on)
        return;
    g_n = 0;
    fx_list_spans(s, g_span, NULL);
    rr_fill(g_spans, g_n, 1, 0);
    memset(ref, 0, sizeof *ref);
    fx_draw_list(s, ref);
    rr_fill_read(g_canvas);
    g_lists++;
    for (i = 0; i < 256 * 256; i++) {
        uint16_t want = ref->set[i] ? (uint16_t)(0x100 | ref->px[i]) : 0;
        g_pixels += ref->set[i];
        if (g_canvas[i] != want)
            d++;
    }
    if (d) {
        if (g_first < 0)
            g_first = (long)snes_frame();
        g_bad++;
        g_bad_pixels += d;
    }
}

static void report(void)
{
    fprintf(stderr, "fxcheck: %ld lists (%ld pixels), %ld differ (first at frame %ld), %ld pixels; %ld pixels plotted outside the lists\n",
            lists, pixels, bad_lists, first_bad, bad_pixels, outside);
}

void canvas_list(const FxDrawSource *s);

void fx_list_begin(void)
{
    FxDrawSource s;
    s.ram = snes_sfx_ram();
    s.rom = srfx_rom;
    s.rom_size = (uint32_t)(srfx_rom_end - srfx_rom);
    s.mode = gsu.scmr & 3;
    s.layout = gsu_layout();
    canvas_list(&s);
    if (gpu_on)
        gpu_list(&s);
    if (on < 0) {
        on = getenv("PORT_FXCHECK") != NULL;
        if (on) {
            mine = (FxCanvas *)malloc(sizeof *mine);
            chip = (FxCanvas *)malloc(sizeof *chip);
            snes_gsu_plot_hook = chip_plot;
            atexit(report);
        }
    }
    if (!on)
        return;
    memset(mine, 0, sizeof *mine);
    memset(chip, 0, sizeof *chip);
    s.ram = snes_sfx_ram();
    s.rom = srfx_rom;
    s.rom_size = (uint32_t)(srfx_rom_end - srfx_rom);
    s.mode = gsu.scmr & 3;
    s.layout = 3;                        /* PLOT's own (x, y), as the hook gets them */
    fx_draw_list(&s, mine);
    open_now = 1;
}

void fx_list_end(void)
{
    long d = 0;
    int i;
    if (!on || !open_now)
        return;
    open_now = 0;
    lists++;
    for (i = 0; i < 256 * 256; i++)
        pixels += chip->set[i];
    for (i = 0; i < 256 * 256; i++)
        if (mine->set[i] != chip->set[i] || (mine->set[i] && mine->px[i] != chip->px[i]))
            d++;
    if (d) {
        if (first_bad < 0)
            first_bad = (long)snes_frame();
        bad_lists++;
        bad_pixels += d;
    }
}
