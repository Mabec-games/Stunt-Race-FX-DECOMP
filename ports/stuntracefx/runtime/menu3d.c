/*
 * menu3d.c: the 3D outside races (the car selection's turning vehicle in
 * Speed, Stunt, Battle and Free Trax) at high resolution.
 *
 * The menus draw with the race's programs ($01:8325, $01:AEF8, $01:AB13),
 * a draw every few frames, nothing in between. Each of the game's own
 * $01:AB13 runs is captured as it happens (hires.c; reading only, the game
 * goes on as without it) into a ring of the last few, with its screen
 * edges ($38-$3E).
 *
 * The picture shown is the game's own, enlarged, with its 3D window drawn
 * again at the high resolution. Where on the picture the Super FX's screen
 * lies is found, not assumed: a capture drawn at the game's own size must
 * match the picture's pixels there (FIND_PCT of them, black ones not
 * counted: a dark fade matches anywhere), the best of the ring's draws at
 * its best place; the place is then kept while the screen stays (the
 * window does not move; a turning, dropping shape's other draws would
 * match in part a few pixels off). Each picture shows the draw that
 * matches best there (the screen goes up a part a frame).
 *
 * In the window (the draw's edges at that place) a pixel is the 3D's when
 * it is the window's background or the colour a recent draw has there or
 * next to it (the game's edges and these a pixel apart; the screen shown
 * may be another draw's); the rest (a name box, the arrows' and the
 * frame's pixels) stays the game's. The colour maths the screen lays on
 * the 3D (the menus darken it by the fixed colour) is applied, with or
 * without as the match says. No match (another screen, a fade to black):
 * the game's own picture.
 *
 * PORT_MENU_HIRES=0 turns it off; PORT_MENU3D_DEBUG=1 reports each
 * picture's choice.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hires.h"
#include "video.h"
#include <time.h>

const uint8_t *port_rom(void);
uint32_t port_rom_size(void);
int interp_in_race(void);           /* interp.c */
int interp_hires_scale(void);       /* interp.c: 0 off */
int interp_hires_aa(void);
const uint32_t *vid_picture(void);
uint32_t snes_frame(void);
uint8_t *snes_sfx_ram(void);

/* a draw matches at a new place when FIND_PCT of its (not black) pixels
   show as it is there; at the kept place KEEP_PCT (the rest: the frame, a
   name box or text over it, edges a pixel apart, another draw shown) */
enum { RING = 4, GW = 256, GH = 224, MIN_PIXELS = 40, STALE = 40, KEEP_PCT = 40, FIND_PCT = 78, REFINE = 30, REFINE_PCT = 75, SEARCH_EVERY = 6 };
/* (a place refined to only when it matches strongly there, the shape
   settled: while it drops in, its draws a few pixels apart make any place
   look half right) */

static HrList ring[RING];
static int ring_ok[RING], ring_next, capturing = -1;
static uint32_t ring_at[RING];              /* the frame each was drawn in */
static int16_t ring_edge[RING][4];          /* its screen's edges: x $38-$3A, y $3C-$3E */
static int on = -1;

static int enabled(void)
{
    if (on < 0) {
        const char *e = getenv("PORT_MENU_HIRES");
        on = !(e && *e == '0');
    }
    return on && interp_hires_scale() > 0 && interp_in_race() <= 0;
}

static int fresh(int k) { return ring_ok[k] && snes_frame() - ring_at[k] <= STALE; }

/* a Super FX program of the game's own (not a sandbox's, not a look-ahead
   copy's) starting (done 0) or at its STOP */
void menu3d_gsu(uint32_t a, int done)
{
    if (a != 0x01AB13)
        return;
    if (!done) {
        const uint8_t *ram = snes_sfx_ram();
        int e;
        capturing = -1;
        if (!enabled())
            return;
        capturing = ring_next;
        ring_ok[capturing] = 0;
        ring[capturing].nops = 0;
        for (e = 0; e < 4; e++)
            ring_edge[capturing][e] = (int16_t)(ram[0x38 + 2 * e] | ram[0x39 + 2 * e] << 8);
        hires_capture(&ring[capturing]);
        hires_capture_begin();
        hires_tee(&ring[capturing]);    /* (interp.c may capture it for its own records: copied here as it ends) */
    } else if (capturing >= 0) {
        hires_capture_end();
        ring_ok[capturing] = !ring[capturing].overflow && ring[capturing].nops > 0;
        ring_at[capturing] = snes_frame();
        ring_next = (capturing + 1) % RING;
        capturing = -1;
    }
}

static uint32_t *lo[RING], *hi, *out;
static int ox, oy, placed;                  /* the Super FX screen's place on the picture, once found */
static uint32_t placed_seen;                /* the last frame it matched there */
static uint32_t searched_at, refined_at;    /* the last search for a place (while none), the last refining of it */
static int math;                            /* the colour maths applied to the 3D (1) or not (0) */
static long shown, fell_back;
static double ms_sum, ms_max;
static double now_ms(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec * 1e3 + t.tv_nsec / 1e6; }
static unsigned long made_for = ~0ul;
static const uint32_t *made_pic;
static int out_w, out_h;

static void report(void)
{
    if (shown)
        fprintf(stderr, "menu 3D: %ld pictures at high resolution, %.2f ms each on average, %.2f at most\n", shown, ms_sum / shown, ms_max);
}

/* the screen's colour maths on the 3D's layer (BG1), line by line */
static uint8_t tint5[VID_HEIGHT + 1][3][32];
static void tint_make(const VidFrame *f)
{
    int L, ch, v;
    for (L = 1; L <= VID_HEIGHT; L++) {
        const VidScreen *s = &f->line[L];
        int use = (s->cgadsub & 0x01) != 0, sub = (s->cgadsub & 0x80) != 0, half = (s->cgadsub & 0x40) != 0;
        int fx[3] = { s->fixed_r & 31, s->fixed_g & 31, s->fixed_b & 31 };
        for (ch = 0; ch < 3; ch++)
            for (v = 0; v < 32; v++) {
                int r = v;
                if (use) {
                    r = sub ? v - fx[ch] : v + fx[ch];
                    r = r < 0 ? 0 : r > 31 ? 31 : r;
                    if (half)
                        r >>= 1;
                }
                tint5[L][ch][v] = (uint8_t)r;
            }
    }
}

/* an 8-bit colour (the renderer's, made from 5 bits) through line L's
   maths: an exact colour exactly, a blend (an edge's coverage) by the
   same step */
static uint32_t tint(uint32_t c, int L)
{
    uint32_t r = 0;
    int ch;
    if (!math || L < 1 || L > VID_HEIGHT)
        return c & 0xFFFFFF;
    for (ch = 0; ch < 3; ch++) {
        unsigned v8 = (c >> (16 - 8 * ch)) & 0xFF, v5 = v8 >> 3, t = tint5[L][ch][v5];
        if (((v5 << 3) | (v5 >> 2)) == v8)
            v8 = (t << 3) | (t >> 2);
        else {
            double x = v8 * 31.0 / 255.0 + ((double)t - v5);
            x = x < 0 ? 0 : x > 31 ? 31 : x;
            v8 = (unsigned)(x * 255.0 / 31.0 + 0.5);
        }
        r |= v8 << (16 - 8 * ch);
    }
    return r;
}

/* how well an own-size drawing lies on the picture at (dx, dy): its
   pixels (not black after the maths) that the picture shows as they are */
static int score(const uint32_t *l, const uint32_t *pic, int dx, int dy, int *covered)
{
    int x, y, n = 0, m = 0;
    for (y = 0; y < GH; y++) {
        int py = y + dy;
        if (py < 0 || py >= VID_HEIGHT)
            continue;
        for (x = 0; x < GW; x++) {
            uint32_t c = l[y * GW + x], t;
            int px = x + dx;
            if (!(c >> 24) || px < 0 || px >= VID_WIDTH || !(t = tint(c, py + 1)))
                continue;
            n++;
            m += (pic[py * VID_WIDTH + px] & 0xFFFFFF) == t;
        }
    }
    *covered = n;
    return m;
}

static int better(int m, int n, int bm, int bn) { return bn <= 0 || m * (long)bn > (long)bm * n; }

/* the best place an own-size drawing lies on the picture (FIND_PCT at
   least), found by a few of its pixels first */
static int search(const uint32_t *l, const uint32_t *pic, int *bx, int *by, int *bm, int *bn)
{
    int sx[64], sy[64], ns = 0, x, y, k, q, dx, dy, found = 0;
    for (y = 0; y < GH && ns < 64; y += 3)
        for (x = (y * 7) % 5; x < GW && ns < 64; x += 11)
            if ((l[y * GW + x] >> 24) && tint(l[y * GW + x], VID_HEIGHT / 2)) {
                sx[ns] = x;
                sy[ns] = y;
                ns++;
            }
    if (ns < 8)
        return 0;
    /* the places to try: where the picture has the first sample's colour
       (that pixel may be under a name box: two others then) */
    for (k = 0; k < 6 && !found; k++) {
    if (k) {
        int t = sx[0], u = sy[0], j = k * ns / 6;
        sx[0] = sx[j], sy[0] = sy[j], sx[j] = t, sy[j] = u;
    }
    {
        /* a colour the picture has all over (a dark one) tells little and
           costs much: another sample */
        uint32_t want = tint(l[sy[0] * GW + sx[0]], VID_HEIGHT / 2);
        int many = 0, i;
        for (i = 0; i < VID_WIDTH * VID_HEIGHT && many <= 1500; i++)
            many += (pic[i] & 0xFFFFFF) == want;
        if (many > 1500)
            continue;
    }
    for (y = 0; y < VID_HEIGHT; y++) {
        uint32_t want = tint(l[sy[0] * GW + sx[0]], y + 1);
        dy = y - sy[0];
        for (x = 0; x < VID_WIDTH; x++) {
            int miss = 0, m, n;
            if ((pic[y * VID_WIDTH + x] & 0xFFFFFF) != want)
                continue;
            dx = x - sx[0];
            for (q = 1; q < ns && miss <= ns / 3; q++) {
                int px = sx[q] + dx, py = sy[q] + dy;
                if (px < 0 || py < 0 || px >= VID_WIDTH || py >= VID_HEIGHT ||
                    (pic[py * VID_WIDTH + px] & 0xFFFFFF) != tint(l[sy[q] * GW + sx[q]], py + 1))
                    miss++;
            }
            if (miss > ns / 3)
                continue;
            m = score(l, pic, dx, dy, &n);
            if (n >= MIN_PIXELS && m * 100 >= n * FIND_PCT && (!found || better(m, n, *bm, *bn))) {
                found = 1;
                *bx = dx;
                *by = dy;
                *bm = m;
                *bn = n;
            }
        }
    }
    }
    return found;
}

/* the picture at high resolution (NULL: the game's own is shown) */
const uint32_t *menu3d_picture(int *w, int *h)
{
    static int reg;
    const VidFrame *f;
    const uint32_t *pic;
    int S, A, k, best = -1, bm = 0, bn = 0, x, y, line, bright = -1, any = 0;
    int wx0, wy0, wx1, wy1;
    uint32_t bg = 0;
    double t0;
    if (!enabled())
        return NULL;
    if (!reg) {
        reg = 1;
        atexit(report);
    }
    if (vid_frames() == made_for) {
        if (!made_pic)
            return NULL;
        *w = out_w;
        *h = out_h;
        return made_pic;
    }
    made_for = vid_frames();
    made_pic = NULL;
    t0 = now_ms();
    for (k = 0; k < RING; k++)
        any |= fresh(k);
    if (!any) {
        placed = 0;             /* no 3D lately: another screen next */
        return NULL;
    }
    if (placed && snes_frame() - placed_seen > STALE)
        placed = 0;
    f = vid_frame();
    pic = vid_picture();
    S = interp_hires_scale();
    A = interp_hires_aa();
    /* one brightness on every line shown (else a fade: the game's picture) */
    for (line = 1; line <= VID_HEIGHT; line++) {
        uint8_t i = f->line[line].inidisp;
        if (i & 0x80)
            continue;
        if (bright < 0)
            bright = i & 0x0F;
        else if ((i & 0x0F) != bright)
            bright = 16;
    }
    if (bright <= 0 || bright > 15)
        goto none;
    tint_make(f);
    line = placed ? oy + GH / 2 : VID_HEIGHT / 2;
    line = line < 1 ? 1 : line > VID_HEIGHT ? VID_HEIGHT : line;
    for (k = 0; k < RING; k++)
        if (fresh(k)) {
            if (!lo[k])
                lo[k] = (uint32_t *)malloc((size_t)GW * GH * 4);
            hires_render(&ring[k], port_rom(), port_rom_size(), f->palette[line], bright, 1, 1, GW, GH, lo[k]);
        }
    if (placed) {
        /* the draw shown at the kept place, the colour maths either way */
        int j, keep = math, bmath = math;
        for (j = 0; j < 2 * RING; j++) {
            int m, n;
            k = j % RING;
            if (!fresh(k))
                continue;
            math = j < RING ? keep : !keep;
            m = score(lo[k], pic, ox, oy, &n);
            if (n >= MIN_PIXELS && m * 100 >= n * KEEP_PCT && (best < 0 || better(m, n, bm, bn))) {
                best = k;
                bm = m;
                bn = n;
                bmath = math;
            }
        }
        math = bmath;
        if (best >= 0 && snes_frame() - refined_at >= REFINE) {
            /* the place a pixel or two off (found while the shape moved
               between draws: one draw a pixel lower matches nearly as
               well): the best of the places round it, over the recent
               draws, taken when clearly better */
            int dx, dy, fx = ox, fy = oy, q;
            long sum0 = 0, n0 = 0, bsum = -1, bnn = 1;
            refined_at = snes_frame();
            for (dy = -2; dy <= 2; dy++)
                for (dx = -2; dx <= 2; dx++) {
                    int m, n;
                    m = score(lo[best], pic, ox + dx, oy + dy, &n);   /* the draw shown (another pose blurs it) */
                    if (!dx && !dy) {
                        sum0 = m;
                        n0 = n;
                    }
                    if (n >= MIN_PIXELS && (bsum < 0 || m * (long)bnn > bsum * n)) {
                        bsum = m;
                        bnn = n;
                        fx = ox + dx;
                        fy = oy + dy;
                    }
                }
            if ((fx != ox || fy != oy) && n0 && bsum * 100 >= bnn * REFINE_PCT && bsum * 100 * n0 > (sum0 * 100 + 3 * n0) * bnn) {
                if (getenv("PORT_MENU3D_DEBUG"))
                    fprintf(stderr, "menu3d f%u: place %d,%d -> %d,%d (%.1f%% -> %.1f%%)\n", snes_frame(), ox, oy, fx, fy,
                            100.0 * sum0 / n0, 100.0 * bsum / bnn);
                ox = fx;
                oy = fy;
                best = -1;      /* chosen again at the new place */
                for (q = 0; q < RING; q++) {
                    int m, n;
                    if (fresh(q) && (m = score(lo[q], pic, ox, oy, &n), n >= MIN_PIXELS) && (best < 0 || better(m, n, bm, bn))) {
                        best = q;
                        bm = m;
                        bn = n;
                    }
                }
            }
        }
    } else if (snes_frame() - searched_at >= SEARCH_EVERY) {
        /* the place: the best of the ring's draws at their best places */
        int j, keep = math, fx = 0, fy = 0, fm = math;
        searched_at = snes_frame();
        for (j = 0; j < 2 * RING; j++) {
            int dx, dy, m = 0, n = 0;
            k = (ring_next + 2 * RING - 1 - j) % RING;
            if (!fresh(k))
                continue;
            math = j < RING ? keep : !keep;
            if (search(lo[k], pic, &dx, &dy, &m, &n) && (best < 0 || better(m, n, bm, bn))) {
                best = k;
                bm = m;
                bn = n;
                fx = dx;
                fy = dy;
                fm = math;
            }
        }
        math = fm;
        if (best >= 0) {
            ox = fx;
            oy = fy;
            placed = 1;
        }
    }
    if (best < 0)
        goto none;
    placed_seen = snes_frame();
    /* the window: the draw's screen edges at its place */
    wx0 = ox + ring_edge[best][0];
    wx1 = ox + ring_edge[best][1];
    wy0 = oy + ring_edge[best][2];
    wy1 = oy + ring_edge[best][3] - 1;
    wx0 = wx0 < 0 ? 0 : wx0;
    wy0 = wy0 < 0 ? 0 : wy0;
    wx1 = wx1 >= VID_WIDTH ? VID_WIDTH - 1 : wx1;
    wy1 = wy1 >= VID_HEIGHT ? VID_HEIGHT - 1 : wy1;
    {
        /* the window's background: its commonest colour where no recent
           draw has anything */
        enum { NC = 64 };
        uint32_t col[NC];
        int cnt[NC], nc = 0, j, q;
        for (y = wy0; y <= wy1; y += 2)
            for (x = wx0; x <= wx1; x += 2) {
                int gx = x - ox, gy = y - oy, drawn = 0;
                uint32_t c;
                if (gx < 0 || gy < 0 || gx >= GW || gy >= GH)
                    continue;
                for (q = 0; q < RING && !drawn; q++)
                    drawn = fresh(q) && (lo[q][gy * GW + gx] >> 24);
                if (drawn)
                    continue;
                c = pic[y * VID_WIDTH + x] | 0xFF000000u;
                for (j = 0; j < nc && col[j] != c; j++)
                    ;
                if (j == nc) {
                    if (nc == NC)
                        continue;
                    col[nc] = c;
                    cnt[nc++] = 0;
                }
                cnt[j]++;
            }
        for (j = 0, k = -1; j < nc; j++)
            if (k < 0 || cnt[j] > cnt[k])
                k = j;
        if (k < 0)
            goto none;
        bg = col[k];
    }
    {
        int W = VID_WIDTH * S, sx, sy;
        if (!hi) {
            hi = (uint32_t *)malloc((size_t)GW * GH * 36 * 4);
            out = (uint32_t *)malloc((size_t)VID_WIDTH * VID_HEIGHT * 36 * 4);
        }
        hires_render(&ring[best], port_rom(), port_rom_size(), f->palette[line], bright, S, A, GW, GH, hi);
        for (y = 0; y < VID_HEIGHT; y++)
            for (x = 0; x < VID_WIDTH; x++) {
                uint32_t p = pic[y * VID_WIDTH + x] | 0xFF000000u;
                int gx = x - ox, gy = y - oy, mine = 0, q, a, b;
                if (x >= wx0 && x <= wx1 && y >= wy0 && y <= wy1 && gx >= 0 && gy >= 0 && gx < GW && gy < GH) {
                    /* the 3D's: the background, or a recent draw's colour
                       here or next to it */
                    mine = p == bg;
                    for (q = 0; q < RING && !mine; q++) {
                        if (!fresh(q))
                            continue;
                        for (b = -1; b <= 1 && !mine; b++)
                            for (a = -1; a <= 1 && !mine; a++) {
                                int qx = gx + a, qy = gy + b;
                                uint32_t c;
                                if (qx < 0 || qy < 0 || qx >= GW || qy >= GH)
                                    continue;
                                c = lo[q][qy * GW + qx];
                                mine = (c >> 24) && (p & 0xFFFFFF) == tint(c, y + 1);
                            }
                    }
                }
                for (sy = 0; sy < S; sy++) {
                    uint32_t *o = out + (size_t)(y * S + sy) * W + x * S;
                    if (!mine) {
                        for (sx = 0; sx < S; sx++)
                            o[sx] = p;
                        continue;
                    }
                    for (sx = 0; sx < S; sx++) {
                        uint32_t c = hi[(size_t)(gy * S + sy) * (GW * S) + gx * S + sx];
                        unsigned al = c >> 24, ch, r = 0;
                        if (!al) {
                            o[sx] = bg;
                            continue;
                        }
                        c = tint(c, y + 1);
                        if (al >= 255) {
                            o[sx] = c | 0xFF000000u;
                            continue;
                        }
                        for (ch = 0; ch < 24; ch += 8)
                            r |= ((((c >> ch) & 0xFF) * al + ((bg >> ch) & 0xFF) * (255 - al) + 127) / 255) << ch;
                        o[sx] = 0xFF000000u | r;
                    }
                }
            }
        out_w = W;
        out_h = VID_HEIGHT * S;
    }
    if (getenv("PORT_MENU3D_DEBUG"))
        fprintf(stderr, "menu3d f%u: draw %d at %d,%d, %d of %d pixels match, math %d, window %d-%d x %d-%d, %.1f ms\n", snes_frame(), best, ox, oy,
                bm, bn, math, wx0, wx1, wy0, wy1, now_ms() - t0);
    shown++;
    {
        double t = now_ms() - t0;
        ms_sum += t;
        ms_max = t > ms_max ? t : ms_max;
    }
    made_pic = out;
    *w = out_w;
    *h = out_h;
    return made_pic;
none:
    if (getenv("PORT_MENU3D_DEBUG"))
        fprintf(stderr, "menu3d f%u: none (bright %d, placed %d), %.1f ms\n", snes_frame(), bright, placed, now_ms() - t0);
    fell_back++;
    return NULL;
}
