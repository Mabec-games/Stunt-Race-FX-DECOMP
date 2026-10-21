/*
 * canvas.c: the Super FX's screens as operations (draw_list.h fx_op_*):
 * each buffer cleared, then every row the drawers make in it, in order;
 * and what the S-CPU uploads of them to VRAM as the screen (fx_op_upload).
 * A buffer is known by its start in the Super FX's RAM (SCBR x 1 KB for
 * the rows, the word at $021A for a clear); two are kept (CANVASES).
 *
 * For the GPU (canvas_gpu): the operations are kept as rr.h spans, each
 * upload as runs of bytes (which buffer, where in it, where in VRAM, how
 * far the operations had got, the frame), and VRAM's copies of them are
 * marked as the 3D canvases' (video.h vid_vram_external). Before a frame
 * is composed, canvas_gpu_apply fills the GPU's canvases with the
 * operations up to each of the frame's uploads and packs the upload's
 * bytes from them (rr_fill, rr_pack): the compositor draws them from
 * there.
 *
 * PORT_CANVASCHECK=1: the operations since each clear filled here
 * (fx_fill_span), and each byte a screen upload moves compared with the
 * same byte of its buffer's pixels at that moment (a bit plane of eight
 * pixels, as PLOT lays them out). At exit: the clears, the buffers whose
 * uploads had a byte that differs (and the first: its frame and number),
 * the bytes compared and those that differ, the rows made, the bytes
 * uploaded from a buffer not kept, and those in the sprite layout.
 * PORT_CANVASDUMP=N also writes the N-th clear's buffer as canvas_N.pgm
 * (in the directory the port runs in) as it is cleared again: its
 * pixels, the uploaded ones (0 where not all their planes went up), and
 * white where an uploaded byte differed, side by side.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "snes.h"
#include "video.h"
#include "rr.h"
#include "gsu.h"
#include "draw_list.h"

extern const uint8_t srfx_rom[], srfx_rom_end[];

#define CANVASES 2

typedef struct {
    int used;
    uint16_t base;                   /* the buffer's start in the RAM */
    uint8_t ht, md;                  /* its layout and colour depth, as last cleared */
    long number;                     /* the clear it was made by (the check's count) */
    FxCanvas *ops;                   /* the check: its pixels */
    uint8_t up_px[256 * 256], up_planes[256 * 256], up_bad[256 * 256];
    long bad;                        /* the check: its uploaded bytes that differed */
} Screen;

static Screen scr[CANVASES];
static int check = -1, gpu, uploading;
static long elsewhere, other_layout, lost;

static unsigned bpp_of(const Screen *s) { return 2u << (s->md - (s->md >> 1)); }

/* the screen kept for the buffer at base, made for it when need is set
   (the one used longest ago given up): -1 when none */
static int screen_of(uint16_t base, int need)
{
    static int next;
    int i;
    for (i = 0; i < CANVASES; i++)
        if (scr[i].used && scr[i].base == base)
            return i;
    if (!need)
        return -1;
    for (i = 0; i < CANVASES; i++)
        if (!scr[i].used)
            break;
    if (i == CANVASES) {
        i = next;
        next = (next + 1) % CANVASES;
    }
    scr[i].used = 1;
    scr[i].base = base;
    return i;
}

/* the screen that RAM address a (bank $70) is in, and a's offset in it */
static int screen_at(uint16_t a, unsigned *o)
{
    int i;
    for (i = 0; i < CANVASES; i++)
        if (scr[i].used && a >= scr[i].base && (unsigned)a < scr[i].base + 0x6800u) {
            *o = a - scr[i].base;
            return i;
        }
    return -1;
}

/* where offset o of a screen is: the bit plane, the row's first pixel
   (x0 >= 256: off the canvas), the line */
static void screen_place(const Screen *s, unsigned o, unsigned *plane, unsigned *x0, unsigned *y)
{
    unsigned bpp = bpp_of(s), cn = o / (bpp * 8u), k = o % (bpp * 8u), rows_of = s->ht == 0 ? 16u : s->ht == 1 ? 20u : 24u;
    *plane = (k >> 4) * 2u + (k & 1u);
    *x0 = cn / rows_of * 8u;
    *y = cn % rows_of * 8u + ((k & 15u) >> 1);
}

/* ---- the GPU's operations and uploads ---- */
static uint32_t *log_w;              /* RR_SPAN_WORDS words an entry: word 6 the canvas, word 7 = 1 a clear */
static long log_n, log_cap, log_base;                     /* entries, room, the first one's number */
typedef struct {
    uint32_t w[RR_RUN_WORDS];        /* word 3: the canvas */
    long op;                         /* the operations made before it */
    unsigned long copy;              /* the copies of VRAM made before it (video.h vid_copies) */
} Run;
static Run *runs;
static long runs_n, runs_cap;
static uint32_t last_a, last_vb;      /* the byte before, to join a run */

static void uploaded(uint32_t a, uint8_t b, uint8_t v);

void fx_span_pack(const FxSpan *p, uint32_t *w)
{
    w[0] = p->x | (uint32_t)p->y << 8 | (uint32_t)p->n << 16;
    w[1] = p->colour | (uint32_t)p->options << 8 | (uint32_t)(p->kind & 7) << 13 | (uint32_t)(p->mode & 3) << 16 |
           (uint32_t)p->bank << 18 | (uint32_t)(p->layout & 3) << 26;
    w[2] = p->tu | (uint32_t)p->tv << 16;
    w[3] = p->du | (uint32_t)p->dv << 16;
    w[4] = p->base | (uint32_t)p->mask << 16;
    w[5] = p->palette;
    w[6] = w[7] = 0;
}

static uint32_t *log_add(void)
{
    if (log_n == log_cap) {
        log_cap = log_cap ? log_cap * 2 : 1 << 16;
        log_w = (uint32_t *)realloc(log_w, (size_t)log_cap * RR_SPAN_WORDS * 4);
    }
    return log_w + (size_t)log_n++ * RR_SPAN_WORDS;
}

static void run_byte(int c, uint32_t a, unsigned o, uint8_t b)
{
    const Screen *s = &scr[c];
    uint32_t vb = vid_vram_next(b);
    Run *r = runs_n ? &runs[runs_n - 1] : NULL;
    if (r && a == last_a + 1 && vb == last_vb + 1 && (vb >> 8) == (last_vb >> 8) && r->op == log_base + log_n &&
        r->copy == vid_copies() && r->w[3] == (uint32_t)c) {
        r->w[0] += 1u << 16;
    } else {
        if (runs_n == runs_cap) {
            runs_cap = runs_cap ? runs_cap * 2 : 1024;
            runs = (Run *)realloc(runs, (size_t)runs_cap * sizeof *runs);
        }
        r = &runs[runs_n++];
        r->w[0] = vb | 1u << 16;
        r->w[1] = o;
        r->w[2] = (uint32_t)s->ht | (uint32_t)s->md << 2;
        r->w[3] = (uint32_t)c;
        r->op = log_base + log_n;
        r->copy = vid_copies();
    }
    last_a = a;
    last_vb = vb;
}

void canvas_gpu(void)
{
    gpu = 1;
    snes_dma_hook = uploaded;
}

/* the operations from entry `from` to `to` filled into the GPU's canvases:
   a canvas's run of rows at a time, a clear starting a run of its own */
static void replay(long from, long to)
{
    long i, start = from;
    int clear = 0;
    uint32_t c = 0;
    for (i = from; i <= to; i++) {
        uint32_t *w = i < to ? log_w + (size_t)(i - log_base) * RR_SPAN_WORDS : NULL;
        if (!w || w[7] || w[6] != c) {
            if (i > start || clear)
                rr_fill(log_w + (size_t)(start - log_base) * RR_SPAN_WORDS, (int)(i - start), clear, (int)c);
            if (!w)
                break;
            clear = w[7] != 0;
            c = w[6];
            start = w[7] ? i + 1 : i;
        }
    }
}

static long applied;                  /* the operations the GPU's canvases have */

/* the uploads before copy `made` of VRAM packed into the GPU's copy */
static void apply_before(unsigned long made)
{
    static uint32_t *batch;
    static long batch_cap;
    long i = 0, done;
    while (i < runs_n && runs[i].copy <= made) {
        long j = i, n, k;
        while (j < runs_n && runs[j].copy <= made && runs[j].op == runs[i].op && runs[j].w[3] == runs[i].w[3])
            j++;
        replay(applied, runs[i].op);
        applied = runs[i].op;
        n = j - i;
        if (n > batch_cap) {
            batch_cap = n * 2;
            batch = (uint32_t *)realloc(batch, (size_t)batch_cap * RR_RUN_WORDS * 4);
        }
        for (k = 0; k < n; k++) {
            memcpy(batch + (size_t)k * RR_RUN_WORDS, runs[i + k].w, RR_RUN_WORDS * 4);
            batch[(size_t)k * RR_RUN_WORDS + 3] = 0;
        }
        rr_pack(batch, (int)n, (int)runs[i].w[3]);
        i = j;
    }
    memmove(runs, runs + i, (size_t)(runs_n - i) * sizeof *runs);
    runs_n -= i;
    /* the log's entries the canvases have, and no upload waits for, go */
    done = applied - log_base;
    if (done > 0) {
        memmove(log_w, log_w + (size_t)done * RR_SPAN_WORDS, (size_t)(log_n - done) * RR_SPAN_WORDS * 4);
        log_n -= done;
        log_base += done;
    }
}

/* each copy of VRAM the frame shows: the uploads made before it packed,
   and the GPU's copy kept as that copy's */
void canvas_gpu_apply(const VidFrame *f)
{
    int v;
    for (v = 0; v < f->versions && v < RR_VERSIONS; v++) {
        apply_before(f->made[v]);
        rr_pack_keep(v);
    }
}

/* ---- the check ---- */
static FxDrawSource src;
static long clears, bad_buffers, bad_bytes, compared, rows, first_bad = -1, first_buffer = -1;

static void finish(Screen *s);

static void report(void)
{
    int i;
    for (i = 0; i < CANVASES; i++)
        if (scr[i].used)
            finish(&scr[i]);
    fprintf(stderr, "canvascheck: %ld clears, %ld buffers differ (first at frame %ld, clear %ld), %ld of %ld bytes "
            "uploaded; %ld rows made; %ld cleared elsewhere than PLOT draws; %ld bytes uploaded from a buffer not "
            "kept; %ld in the sprite layout\n", clears, bad_buffers, first_bad, first_buffer, bad_bytes, compared, rows,
            elsewhere, lost, other_layout);
}

static void check_byte(Screen *s, unsigned o, uint8_t v)
{
    unsigned plane, x0, y, i;
    uint8_t want = 0;
    screen_place(s, o, &plane, &x0, &y);
    if (x0 >= 256u)
        return;
    for (i = 0; i < 8; i++)
        want |= (uint8_t)(((s->ops->px[y << 8 | (x0 + i)] >> plane) & 1u) << (7 - i));
    compared++;
    if (want != v) {
        s->bad++;
        for (i = 0; i < 8; i++)
            s->up_bad[y << 8 | (x0 + i)] = 1;
    }
    for (i = 0; i < 8; i++) {
        unsigned at = y << 8 | (x0 + i);
        s->up_px[at] = (uint8_t)((s->up_px[at] & ~(1u << plane)) | ((v >> (7 - i)) & 1u) << plane);
        s->up_planes[at] |= (uint8_t)(1u << plane);
    }
}

/* a byte the DMA moves: a screen byte when an upload is on and it comes
   from a buffer */
static void uploaded(uint32_t a, uint8_t b, uint8_t v)
{
    unsigned o;
    int c;
    if (!uploading || (b != 0x18 && b != 0x19) || (a >> 16) != 0x70)
        return;
    if ((c = screen_at((uint16_t)a, &o)) < 0) {
        lost++;
        return;
    }
    if (scr[c].ht == 3) {
        other_layout++;
        return;
    }
    if (gpu)
        run_byte(c, a, o, b);
    if (check > 0)
        check_byte(&scr[c], o, v);
}

/* a buffer cleared again: its uploads' bytes that differed */
static void finish(Screen *s)
{
    unsigned full = (1u << bpp_of(s)) - 1u;
    if (getenv("PORT_CANVASDUMP") && s->number == atol(getenv("PORT_CANVASDUMP"))) {
        char name[64];
        FILE *f;
        unsigned x, y;
        snprintf(name, sizeof name, "canvas_%ld.pgm", s->number);
        if ((f = fopen(name, "wb"))) {
            fprintf(f, "P5 768 256 255\n");
            for (y = 0; y < 256; y++) {
                for (x = 0; x < 256; x++)
                    fputc(s->ops->px[y << 8 | x], f);
                for (x = 0; x < 256; x++)
                    fputc(s->up_planes[y << 8 | x] == full ? s->up_px[y << 8 | x] : 0, f);
                for (x = 0; x < 256; x++)
                    fputc(s->up_bad[y << 8 | x] ? 255 : 0, f);
            }
            fclose(f);
        }
        fprintf(stderr, "canvasdump: clear %ld, frame %ld, layout %u, mode %u, base $%04X, %ld bytes differed\n", s->number,
                (long)snes_frame(), s->ht, s->md, s->base, s->bad);
    }
    if (s->bad) {
        if (first_bad < 0) {
            first_bad = (long)snes_frame();
            first_buffer = s->number;
        }
        bad_buffers++;
        bad_bytes += s->bad;
    }
}

/* ---- the operations ---- */
void fx_op_clear(void)
{
    const uint8_t *ram = snes_sfx_ram();
    uint16_t base = (uint16_t)(ram[0x021A] | ram[0x021B] << 8);   /* the buffer FxScreenClear clears */
    Screen *s;
    int c;
    if (check < 0) {
        check = getenv("PORT_CANVASCHECK") != NULL;
        if (check) {
            src.rom = srfx_rom;
            src.rom_size = (uint32_t)(srfx_rom_end - srfx_rom);
            atexit(report);
        }
        if (check || gpu)
            snes_dma_hook = uploaded;
    }
    if (!check && !gpu)
        return;
    if (base != (uint16_t)(gsu.scbr << 10))
        elsewhere++;
    c = screen_of(base, 1);
    s = &scr[c];
    if (check && s->ops)
        finish(s);
    s->ht = gsu_layout();
    s->md = (uint8_t)(gsu.scmr & 3);
    s->number = ++clears;
    if (gpu) {
        uint32_t *w = log_add();
        memset(w, 0, RR_SPAN_WORDS * 4);
        w[6] = (uint32_t)c;
        w[7] = 1;
    }
    if (check) {
        if (!s->ops)
            s->ops = (FxCanvas *)malloc(sizeof *s->ops);
        memset(s->up_planes, 0, sizeof s->up_planes);
        memset(s->up_bad, 0, sizeof s->up_bad);
        s->bad = 0;
        memset(s->ops->px, 0, sizeof s->ops->px);
        memset(s->ops->set, 1, sizeof s->ops->set);
    }
}

void fx_op_upload(int up)
{
    uploading = up;
    vid_vram_external(up && (check > 0 || gpu));
}

/* a row, into the buffer PLOT draws in */
void fx_op_span(const FxSpan *s)
{
    int c;
    if ((!check && !gpu) || check < 0 || (c = screen_of((uint16_t)(gsu.scbr << 10), 0)) < 0)
        return;
    rows++;
    if (gpu) {
        uint32_t *w = log_add();
        fx_span_pack(s, w);
        w[6] = (uint32_t)c;
    }
    if (check)
        fx_fill_span(&src, scr[c].ops, s);
}

void fx_op_palette_span(FxSpan *s, uint16_t palette)
{
    const uint8_t *ram;
    if ((!check && !gpu) || check < 0)
        return;
    ram = snes_sfx_ram();                /* 64 KB: RAMBR's bank is the same bytes */
    s->palette = (uint32_t)ram[(uint16_t)palette] | (uint32_t)ram[(uint16_t)(palette + 1)] << 8 |
                 (uint32_t)ram[(uint16_t)(palette + 2)] << 16 | (uint32_t)ram[(uint16_t)(palette + 3)] << 24;
    fx_op_span(s);
}

void fx_op_ramp_span(FxSpan *s, uint16_t ramp)
{
    const uint8_t *ram;
    int i;
    if ((!check && !gpu) || check < 0)
        return;
    ram = snes_sfx_ram();
    s->palette = 0;
    for (i = 0; i < 4; i++)
        s->palette |= (uint32_t)ram[(uint16_t)(ramp + 2 * i)] << (8 * i);
    s->tv = (uint16_t)(ram[(uint16_t)(ramp + 8)] | ram[(uint16_t)(ramp + 10)] << 8);
    s->dv = (uint16_t)(ram[(uint16_t)(ramp + 12)] | ram[(uint16_t)(ramp + 14)] << 8);
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
    if ((!check && !gpu) || check < 0)
        return;
    fx_list_spans(s, list_row, NULL);
}
