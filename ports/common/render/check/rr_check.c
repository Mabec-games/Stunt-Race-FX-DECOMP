/*
 * rr_check.c: a renderer backend held to the reference (rr_size.h's
 * rr_ref_image, the software backend's drawing). Each case draws a test
 * picture over a rectangle of a headless target, reads the frame back and
 * compares every pixel with the reference's; the largest difference in any
 * channel must be within TOL (GPU filtering rounds its blend weights).
 *
 *   rr_check            exit 0 when every case is within TOL
 *   RR_SIZE=WxH         the target (default 1280x800, set by check.sh)
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rr.h"
#include "rr_size.h"

#define TOL 2

/* a picture that shows every filtering fault: single-pixel checks, hard
   colour edges and a smooth ramp */
static void pattern(uint32_t *p, int w, int h, unsigned seed)
{
    int x, y;
    for (y = 0; y < h; y++)
        for (x = 0; x < w; x++) {
            uint32_t c;
            seed = seed * 1103515245u + 12345u;
            if (y < h / 3)
                c = ((x ^ y) & 1) ? 0xFFFFFFu : 0x000000u;
            else if (y < 2 * h / 3)
                c = (seed >> 8) & 0xFFFFFFu;
            else
                c = (uint32_t)(x * 255 / (w - 1)) << 16 | (uint32_t)(y * 255 / (h - 1)) << 8 | 0x40;
            p[(size_t)y * (size_t)w + (size_t)x] = 0xFF000000u | c;
        }
}

int main(void)
{
    static const struct { int w, h; float x0, y0, x1, y1; const char *what; } cs[] = {
        { 256, 224, 0, 0, 1, 1, "fill the target" },
        { 256, 224, -1, -1, -1, -1, "4:3, letterboxed" },
        { 256, 224, 10, 20, 0, 0, "one to one" },
        { 256, 224, 0, 0, 3, 3, "three times" },
        { 341, 224, 0, 0, 1, 1, "wide picture, fill" },
        { 37, 29, 0.13f, 0.21f, 0.77f, 0.93f, "small picture, odd scale" },
    };
    uint32_t *got, *want, *src;
    int tw, th, k, bad = 0;
    if (!getenv("RR_SIZE"))
        setenv("RR_SIZE", "1280x800", 1);
    if (!rr_open()) {
        printf("%s: cannot open\n", rr_name());
        return 2;
    }
    rr_target(&tw, &th);
    got = (uint32_t *)malloc((size_t)tw * (size_t)th * 4);
    want = (uint32_t *)malloc((size_t)tw * (size_t)th * 4);
    src = (uint32_t *)malloc(1024 * 1024 * 4);
    for (k = 0; k < (int)(sizeof cs / sizeof cs[0]); k++) {
        int d[4], i, maxd = 0, over = 0;
        size_t n = (size_t)tw * (size_t)th;
        pattern(src, cs[k].w, cs[k].h, (unsigned)k + 1);
        if (cs[k].x0 < 0) {                    /* 4:3, as tall as fits */
            int ww = th * 4 / 3, hh = th;
            if (ww > tw) { ww = tw; hh = tw * 3 / 4; }
            d[0] = (tw - ww) / 2; d[1] = (th - hh) / 2; d[2] = d[0] + ww; d[3] = d[1] + hh;
        } else if (cs[k].x1 == 0) {            /* one to one at an offset */
            d[0] = (int)cs[k].x0; d[1] = (int)cs[k].y0; d[2] = d[0] + cs[k].w; d[3] = d[1] + cs[k].h;
        } else if (cs[k].x1 > 1) {             /* a whole multiple */
            d[0] = 0; d[1] = 0; d[2] = cs[k].w * (int)cs[k].x1; d[3] = cs[k].h * (int)cs[k].y1;
        } else {
            d[0] = (int)(cs[k].x0 * tw); d[1] = (int)(cs[k].y0 * th);
            d[2] = (int)(cs[k].x1 * tw); d[3] = (int)(cs[k].y1 * th);
        }
        rr_begin(0xFF102030u);
        rr_image(src, cs[k].w, cs[k].h, d);
        rr_present();
        if (!rr_read(got)) {
            printf("%s: no read back\n", rr_name());
            return 2;
        }
        for (i = 0; i < (int)n; i++)
            want[i] = 0xFF102030u;
        rr_ref_image(want, tw, th, src, cs[k].w, cs[k].h, d);
        for (i = 0; i < (int)n; i++) {
            int s, m = 0;
            for (s = 0; s < 24; s += 8) {
                int a = (int)((got[i] >> s) & 0xFF) - (int)((want[i] >> s) & 0xFF);
                if (a < 0) a = -a;
                if (a > m) m = a;
            }
            if (m > maxd) maxd = m;
            if (m > TOL) over++;
        }
        printf("%-6s %-26s %4dx%-4d over (%d,%d)-(%d,%d): largest difference %3d, %d pixels over %d\n",
               rr_name(), cs[k].what, cs[k].w, cs[k].h, d[0], d[1], d[2], d[3], maxd, over, TOL);
        bad += over != 0;
    }
    rr_close();
    return bad ? 1 : 0;
}
