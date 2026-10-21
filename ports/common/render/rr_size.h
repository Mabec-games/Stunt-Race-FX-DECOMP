/*
 * rr_size.h: what every backend shares: the headless target size
 * (RR_SIZE=WxH, else 256 x 224) and sharp bilinear, the one definition the
 * backends' shaders and the software backend all compute.
 */
#ifndef PORT_RR_SIZE_H
#define PORT_RR_SIZE_H

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static inline void rr_headless_size(int *w, int *h)
{
    const char *e = getenv("RR_SIZE");
    int a, b;
    *w = 256;
    *h = 224;
    if (e && sscanf(e, "%dx%d", &a, &b) == 2 && a > 0 && b > 0 && a <= 16384 && b <= 16384) {
        *w = a;
        *h = b;
    }
}

/* Sharp bilinear along one axis: for the sample point p (in texels, the
   texel centre at i + 0.5) and d texels to one target pixel, the texel i
   and the weight of texel i + 1 (0 inside a texel, a ramp one target pixel
   wide across its edge). */
static inline float rr_sharp(float p, float d, int *i)
{
    float px = p - 0.5f, fl = floorf(px), f = px - fl;
    if (d < 1e-5f)
        d = 1e-5f;
    f = (f - 0.5f) / d + 0.5f;
    *i = (int)fl;
    return f < 0 ? 0 : f > 1 ? 1 : f;
}

static inline uint32_t rr_texel(const uint32_t *p, int w, int h, int x, int y)
{
    x = x < 0 ? 0 : x >= w ? w - 1 : x;
    y = y < 0 ? 0 : y >= h ? h - 1 : y;
    return p[(size_t)y * (size_t)w + (size_t)x];
}

static inline uint32_t rr_mix(uint32_t a, uint32_t b, float t)
{
    uint32_t o = 0xFF000000u;
    int s;
    for (s = 0; s < 24; s += 8) {
        float ca = (float)((a >> s) & 0xFF), cb = (float)((b >> s) & 0xFF);
        o |= (uint32_t)(int)(ca + (cb - ca) * t + 0.5f) << s;
    }
    return o;
}

/* The reference: w x h pixels drawn over rectangle d of the tw x th
   target, as rr_image draws them (the software backend's drawing, and what
   rr_check holds the GPU backends to). */
static inline void rr_ref_image(uint32_t *dst, int tw, int th, const uint32_t *src, int w, int h, const int d[4])
{
    int x, y, x0 = d[0], y0 = d[1], x1 = d[2], y1 = d[3];
    float dx, dy;
    if (w <= 0 || h <= 0 || x1 <= x0 || y1 <= y0)
        return;
    dx = (float)w / (float)(x1 - x0);
    dy = (float)h / (float)(y1 - y0);
    for (y = y0 < 0 ? 0 : y0; y < y1 && y < th; y++) {
        int ty;
        float fy = rr_sharp(((float)(y - y0) + 0.5f) * dy, dy, &ty);
        for (x = x0 < 0 ? 0 : x0; x < x1 && x < tw; x++) {
            int tx;
            float fx = rr_sharp(((float)(x - x0) + 0.5f) * dx, dx, &tx);
            uint32_t top = rr_mix(rr_texel(src, w, h, tx, ty), rr_texel(src, w, h, tx + 1, ty), fx);
            uint32_t bot = rr_mix(rr_texel(src, w, h, tx, ty + 1), rr_texel(src, w, h, tx + 1, ty + 1), fx);
            dst[(size_t)y * (size_t)tw + (size_t)x] = rr_mix(top, bot, fy);
        }
    }
}

#endif
