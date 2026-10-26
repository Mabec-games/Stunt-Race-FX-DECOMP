/*
 * hires.c: the race's 3D as shapes with exact positions (hires.h), for a
 * renderer at any resolution (hirender.c). Nothing here changes the game:
 * the hooks read registers the Super FX's C already holds, and the RAM
 * through the host's view of it (no emulated access, no time).
 *
 * The Super FX projects each point (screen = centre + x scale / 65536,
 * the scale 128 / depth from a table; cut to whole pixels) and queues each
 * shape's screen points in its draw list at $08AA (raster.c), which
 * FxDrawList fills into its screen. Each projection is kept with its
 * position before the cut (FxProjectVertex and its copy in the model
 * interpreter, and command $0C's own loop: model.c, project.c); when a
 * list is drawn its shapes are read and each point a projection made is
 * given that projection's exact position. Scaled bitmaps (FxProjectScaled,
 * then their rows through fx_op_span) are kept as sprites.
 *
 * PORT_HIRES_DUMP=DIR, PORT_HIRES_AT=F (with the in-between frames off):
 * the first race draw starting at or after video frame F written to DIR
 * as shapes.json, canvas.pgm, palette.bin and pictures (full, no3d, obj),
 * for work/tools/hires_poc.py.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#include "snes.h"
#include "video.h"
#include "hires.h"

extern FxCanvas *canvas_capture;

#define MAXPROJ 4096
typedef struct { int16_t xi, yi; float xf, yf; } Proj;
static Proj proj[MAXPROJ];
static int nproj;

static HrList *next, *cur;
/* the model's projected points ($04E4, 6 bytes each) and where faces'
   corners were copied from them into the draw list (by address, for the
   list being filled: qgen) */
#define TAB_N 256
static Proj tab[TAB_N];
static uint8_t tab_set[TAB_N];
typedef struct { Proj p; uint32_t gen; } QMap;
static QMap *qmap;
static uint32_t qgen = 1;
static int sprite_open;          /* the last op a sprite still taking rows */

static uint16_t rd16(const uint8_t *m, unsigned a) { return (uint16_t)(m[a & 0xFFFF] | m[(a + 1) & 0xFFFF] << 8); }

void hires_capture(HrList *l) { next = l; }
static long hr_dbg_found, hr_dbg_ambiguous, hr_dbg_missing;
static void dbg_report(void)
{
    if (getenv("PORT_HIRES_DEBUG"))
        fprintf(stderr, "hires corners: %ld found, %ld of them with another projection at the same pixel elsewhere, %ld not found\n",
                hr_dbg_found, hr_dbg_ambiguous, hr_dbg_missing);
}
__attribute__((constructor)) static void dbg_init(void) { atexit(dbg_report); }

void hires_capture_begin(void)
{
    cur = next;
    if (cur) {
        cur->nops = cur->nrows = cur->overflow = 0;
        cur->wl = cur->wt = cur->wr = cur->wb = 0;
    }
    nproj = 0;
    sprite_open = 0;
    memset(tab_set, 0, sizeof tab_set);
    qgen++;
    if (!qmap)
        qmap = (QMap *)calloc(0x10000, sizeof *qmap);
}

static void dump_end(void);

static long dbg_sprites, dbg_lists;
static void dbg_sprites_report(void)
{
    if (getenv("PORT_HIRES_DEBUG") && dbg_lists)
        fprintf(stderr, "hires: %ld lists, %.2f sprites a list\n", dbg_lists, (double)dbg_sprites / dbg_lists);
}

void hires_capture_end(void)
{
    if (cur) {
        static int reg;
        int k;
        if (!reg) {
            reg = 1;
            atexit(dbg_sprites_report);
        }
        dbg_lists++;
        for (k = 0; k < cur->nops; k++)
            dbg_sprites += cur->ops[k].kind == HR_SPRITE;
    }
    if (cur)
        dump_end();
    cur = next = NULL;
    sprite_open = 0;
}

static HrOp *new_op(int kind)
{
    HrOp *o;
    if (!cur)
        return NULL;
    if (cur->nops >= HR_MAXOPS) {
        cur->overflow = 1;
        return NULL;
    }
    o = &cur->ops[cur->nops++];
    memset(o, 0, offsetof(HrOp, x));
    o->kind = (uint8_t)kind;
    o->nrows = 0;
    sprite_open = kind == HR_SPRITE;
    return o;
}

/* FxProjectVertex (and its copy in model.c): (x, y) at depth d went to
   (sx, sy) */
void hires_project(int16_t x, int16_t y, int16_t d, uint16_t sx, uint16_t sy)
{
    const uint8_t *ram;
    if (!cur || d <= 0 || nproj >= MAXPROJ)
        return;
    ram = snes_sfx_ram();
    proj[nproj].xi = (int16_t)sx;
    proj[nproj].yi = (int16_t)sy;
    proj[nproj].xf = (float)((int16_t)rd16(ram, 0x34) + 128.0 * x / d);
    proj[nproj].yf = (float)((int16_t)rd16(ram, 0x36) + 128.0 * y / d);
    nproj++;
}

/* command $0C's projection (model.c): (x, y) by the depth's scale (FMULT:
   x scale / 65536, cut) about the centre (cx, cy) went to (sx, sy) */
void hires_tab_last(uint16_t slot)
{
    unsigned i = (uint16_t)(slot - 0x04E4) / 6u;
    if (!cur || !nproj || i >= TAB_N)
        return;
    tab[i] = proj[nproj - 1];
    tab_set[i] = 1;
}

void hires_queue(uint16_t dst, uint16_t slot)
{
    unsigned i = (uint16_t)(slot - 0x04E4) / 6u;
    if (!cur || i >= TAB_N || !tab_set[i])
        return;
    qmap[dst].p = tab[i];
    qmap[dst].gen = qgen;
}

void hires_project_scaled(int16_t x, int16_t y, uint16_t scale, int16_t cx, int16_t cy, uint16_t sx, uint16_t sy, uint16_t slot)
{
    if (!cur || nproj >= MAXPROJ)
        return;
    proj[nproj].xi = (int16_t)sx;
    proj[nproj].yi = (int16_t)sy;
    proj[nproj].xf = (float)(cx + (double)x * (int16_t)scale / 65536.0);
    proj[nproj].yf = (float)(cy + (double)y * (int16_t)scale / 65536.0);
    nproj++;
    hires_tab_last(slot);
}

/* FxProjectScaled: (x, y) and size, halved `shift` times (doubled when
   negative) into the table's depths, by the scale about (cx, cy), went to
   (sx, sy) and ssize: a sprite's place and size */
void hires_sprite_project(int16_t x, int16_t y, int16_t size, int shift, uint16_t scale, int16_t cx, int16_t cy,
                          uint16_t sx, uint16_t sy, uint16_t ssize)
{
    double f = shift >= 0 ? 1.0 / (1 << shift) : (double)(1 << -shift);
    double k = f * (int16_t)scale / 65536.0;
    HrOp *o = new_op(HR_SPRITE);
    if (!o)
        return;
    o->xi = (int16_t)sx;
    o->yi = (int16_t)sy;
    o->si = (int16_t)ssize;
    o->xf = (float)(cx + x * k);
    o->yf = (float)(cy + y * k);
    o->sf = (float)(size * k);
    o->row0 = (uint16_t)cur->nrows;
}

/* a row plotted outside the draw lists: the open sprite's, or a row of its
   own (an effect's, the text's) */
void hires_span(const FxSpan *s)
{
    HrOp *o;
    if (!cur)
        return;
    if (cur->nrows >= HR_MAXROWS) {
        cur->overflow = 1;
        return;
    }
    if (sprite_open)
        o = &cur->ops[cur->nops - 1];
    else {
        if (!(o = new_op(HR_ROWS)))
            return;
        o->row0 = (uint16_t)cur->nrows;
    }
    cur->rows[cur->nrows++] = *s;
    o->nrows++;
}

/* the exact place of a queued point (x, y) at address at in the list:
   the table's point copied there, else the latest projection to (x, y)
   (a face projected just before it was queued) */
static void exact(unsigned at, int x, int y, float *xf, float *yf)
{
    int k;
    const QMap *q = &qmap[at & 0xFFFF];
    if (q->gen == qgen && q->p.xi == x && q->p.yi == y) {
        *xf = q->p.xf;
        *yf = q->p.yf;
        hr_dbg_found++;
        return;
    }
    for (k = nproj - 1; k >= 0 && k >= nproj - 512; k--)
        if (proj[k].xi == x && proj[k].yi == y) {
            int j;
            *xf = proj[k].xf;
            *yf = proj[k].yf;
            hr_dbg_found++;
            for (j = k - 1; j >= 0; j--)
                if (proj[j].xi == x && proj[j].yi == y && (proj[j].xf - *xf > 0.01f || *xf - proj[j].xf > 0.01f ||
                                                            proj[j].yf - *yf > 0.01f || *yf - proj[j].yf > 0.01f)) {
                    hr_dbg_ambiguous++;
                    break;
                }
            return;
        }
    hr_dbg_missing++;
    *xf = (float)x;
    *yf = (float)y;
}

/* FxDrawList about to draw the list at $08AA up to the word at $005C */
void hires_draw_list(void)
{
    const uint8_t *ram;
    unsigned at, end;
    int right;
    if (!cur)
        return;
    ram = snes_sfx_ram();
    end = rd16(ram, 0x005C);
    right = (int16_t)rd16(ram, 0x3A);
    for (at = 0x08AA; at < end;) {
        int t = (int8_t)ram[at], n, k;
        HrOp *o;
        if (t == 0)
            break;
        n = t >= 3 ? t : t == 2 ? 2 : (~t) & 0xFF;
        if (n > HR_MAXV || !(o = new_op(t >= 3 ? HR_FLAT : t == 2 ? HR_LINE : HR_TEX))) {
            at += t >= 2 ? 2 + 4 * (unsigned)n : 8 + 8 * (unsigned)n;
            if (n > HR_MAXV && cur)
                cur->overflow = 1;
            continue;
        }
        o->n = (uint8_t)n;
        if (t >= 2) {
            o->colour = ram[at + 1];
            for (k = 0; k < n; k++) {
                int x = (int16_t)rd16(ram, at + 2 + 4 * k), y = (int16_t)rd16(ram, at + 4 + 4 * k);
                exact(at + 2 + 4 * (unsigned)k, x, y, &o->x[k], &o->y[k]);
            }
            at += 2 + 4 * (unsigned)n;
        } else {
            o->bank = ram[at + 1];
            o->colour = ram[at + 2];
            o->options = ram[at + 3];
            o->base = rd16(ram, at + 4);
            o->mask = rd16(ram, at + 6);
            for (k = 0; k < n; k++) {
                unsigned p = at + 8 + 8 * (unsigned)k;
                int x = (int16_t)rd16(ram, p), y = (int16_t)rd16(ram, p + 2);
                exact(p, x, y, &o->x[k], &o->y[k]);
                o->u[k] = rd16(ram, p + 4);
                o->v[k] = rd16(ram, p + 6);
            }
            at += 8 + 8 * (unsigned)n;
        }
        /* a corner the screen-edge clip put on the right edge (its last
           column) stands for the window's edge: the Super FX fills that
           column too */
        for (k = 0; k < n; k++)
            if (o->x[k] == (float)right)
                o->x[k] = (float)(right + 1);
        sprite_open = 0;
    }
    qgen++;         /* the list's addresses are filled afresh (the projections kept: a
                       model's later faces may use points projected before this) */
}

/* ---- the dump (PORT_HIRES_DUMP) ---- */

static int armed = -1;           /* 1 waiting for the draw, 2 capturing, 3 waiting to dump, 0 done/off */
static long at_frame;
static const char *dir;
static HrList *dump_list;
static FxCanvas *canvas;
static FxCanvas *keep_capture;
static int wait_frames;

static int dump_on(void)
{
    if (armed < 0) {
        dir = getenv("PORT_HIRES_DUMP");
        at_frame = getenv("PORT_HIRES_AT") ? atol(getenv("PORT_HIRES_AT")) : 0;
        armed = dir ? 1 : 0;
    }
    return armed > 0;
}

/* a Super FX program starting (done 0) or at its STOP (1), outside the
   sandbox: the dump's draw */
void hires_gsu(uint32_t a, int done)
{
    if (!dump_on() || a != 0x01AB13)
        return;
    if (armed == 1 && !done && (long)snes_frame() >= at_frame && snes_wram()[0x0E5B] && !next) {
        dump_list = (HrList *)calloc(1, sizeof *dump_list);
        canvas = (FxCanvas *)calloc(1, sizeof *canvas);
        keep_capture = canvas_capture;
        canvas_capture = canvas;
        hires_capture(dump_list);
        hires_capture_begin();
        armed = 2;
    } else if (armed == 2 && done) {
        hires_capture_end();
    }
}

static void put_point(FILE *f, int first, float xf, float yf)
{
    fprintf(f, "%s[%d,%d,%.4f,%.4f,1]", first ? "" : ",", (int)xf, (int)yf, xf, yf);
}

static void dump_end(void)
{
    char path[1024];
    FILE *f;
    int i, k, first = 1;
    if (armed != 2 || cur != dump_list)
        return;
    canvas_capture = keep_capture;
    snprintf(path, sizeof path, "%s/shapes.json", dir);
    if ((f = fopen(path, "w"))) {
        const uint8_t *ram = snes_sfx_ram();
        fprintf(f, "{\"frame\":%u,\"centre\":[%d,%d],\"edges\":[%d,%d,%d,%d],\"shapes\":[", snes_frame(),
                (int16_t)rd16(ram, 0x34), (int16_t)rd16(ram, 0x36), (int16_t)rd16(ram, 0x38), (int16_t)rd16(ram, 0x3A),
                (int16_t)rd16(ram, 0x3C), (int16_t)rd16(ram, 0x3E));
        for (i = 0; i < cur->nops; i++) {
            const HrOp *o = &cur->ops[i];
            if (o->kind == HR_FLAT || o->kind == HR_LINE) {
                fprintf(f, "%s\n{\"type\":\"%s\",\"colour\":%u,\"v\":[", first ? "" : ",", o->kind == HR_FLAT ? "flat" : "line", o->colour);
                for (k = 0; k < o->n; k++)
                    put_point(f, !k, o->x[k], o->y[k]);
                fprintf(f, "]}");
                first = 0;
            } else if (o->kind == HR_TEX) {
                fprintf(f, "%s\n{\"type\":\"tex\",\"bank\":%u,\"colour\":%u,\"options\":%u,\"base\":%u,\"mask\":%u,\"v\":[",
                        first ? "" : ",", o->bank, o->colour, o->options, o->base, o->mask);
                for (k = 0; k < o->n; k++)
                    fprintf(f, "%s[%d,%d,%u,%u,%.4f,%.4f,1]", k ? "," : "", (int)o->x[k], (int)o->y[k], o->u[k], o->v[k], o->x[k],
                            o->y[k]);
                fprintf(f, "]}");
                first = 0;
            } else {
                if (o->kind == HR_SPRITE) {
                    fprintf(f, "%s\n{\"type\":\"project\",\"xi\":%d,\"yi\":%d,\"si\":%d,\"xf\":%.4f,\"yf\":%.4f,\"sf\":%.4f}",
                            first ? "" : ",", o->xi, o->yi, o->si, o->xf, o->yf, o->sf);
                    first = 0;
                }
                for (k = 0; k < o->nrows; k++) {
                    const FxSpan *s = &cur->rows[o->row0 + k];
                    fprintf(f, "%s\n{\"type\":\"span\",\"x\":%u,\"y\":%u,\"n\":%u,\"kind\":%u,\"colour\":%u,\"options\":%u,"
                            "\"bank\":%u,\"tu\":%u,\"du\":%u,\"tv\":%u,\"dv\":%u,\"base\":%u,\"mask\":%u,\"palette\":%u}",
                            first ? "" : ",", s->x, s->y, s->n, s->kind, s->colour, s->options,
                            s->bank, s->tu, s->du, s->tv, s->dv, s->base, s->mask, s->palette);
                    first = 0;
                }
            }
        }
        fprintf(f, "\n],\"matched\":0,\"unmatched\":0,\"overflow\":%d}\n", cur->overflow);
        fclose(f);
    }
    snprintf(path, sizeof path, "%s/canvas.pgm", dir);
    if ((f = fopen(path, "wb"))) {
        fprintf(f, "P5\n256 256\n255\n");
        fwrite(canvas->px, 1, sizeof canvas->px, f);
        fclose(f);
    }
    fprintf(stderr, "hires: %d shapes captured\n", cur->nops);
    armed = 3;
    wait_frames = 3;
}

static void write_ppm(const char *name, const uint32_t *px)
{
    char path[1024];
    FILE *f;
    int i;
    snprintf(path, sizeof path, "%s/%s", dir, name);
    if (!(f = fopen(path, "wb")))
        return;
    fprintf(f, "P6\n%d %d\n255\n", VID_WIDTH, VID_HEIGHT);
    for (i = 0; i < VID_WIDTH * VID_HEIGHT; i++) {
        uint8_t c[3] = { (uint8_t)(px[i] >> 16), (uint8_t)(px[i] >> 8), (uint8_t)px[i] };
        fwrite(c, 1, 3, f);
    }
    fclose(f);
}

/* each video frame: the dump's pictures, once the captured screen is shown */
void hires_frame(void)
{
    static VidFrame *v;
    static uint32_t *px;
    char path[1024];
    FILE *f;
    int l;
    if (armed != 3 || --wait_frames > 0)
        return;
    v = (VidFrame *)malloc(sizeof *v);
    px = (uint32_t *)malloc(VID_WIDTH * VID_HEIGHT * 4);
    memcpy(v, vid_frame(), sizeof *v);
    vid_draw_frame(v, px);
    write_ppm("full.ppm", px);
    for (l = 0; l <= VID_HEIGHT; l++) {
        v->line[l].tm &= (uint8_t)~1;
        v->line[l].ts &= (uint8_t)~1;
    }
    vid_draw_frame(v, px);
    write_ppm("no3d.ppm", px);
    for (l = 0; l <= VID_HEIGHT; l++) {
        v->line[l].tm = 0x10;
        v->line[l].ts = 0;
        v->line[l].cgadsub = 0;
    }
    vid_draw_frame(v, px);
    write_ppm("obj.ppm", px);
    snprintf(path, sizeof path, "%s/palette.bin", dir);
    if ((f = fopen(path, "wb"))) {
        fwrite(vid_frame()->palette[100], 2, 256, f);
        fclose(f);
    }
    fprintf(stderr, "hires: pictures written to %s\n", dir);
    armed = 0;
}
