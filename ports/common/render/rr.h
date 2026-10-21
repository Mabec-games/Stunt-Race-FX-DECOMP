/*
 * rr.h: the renderer backend interface, neutral between graphics APIs (the
 * same design as the Boss Rally port's).
 *
 * A frame is drawn into a target the size of the window's drawable, one
 * target pixel per window pixel: pictures of the game's pixels go onto it
 * where the port places them, scaled with sharp bilinear filtering (whole
 * pixels at any size, blended only across the one target pixel a pixel's
 * edge falls in), then the target is shown. A backend (Metal, OpenGL,
 * Vulkan, the software one, or the null one) only has to do that.
 *
 *   RR_SIZE=WxH   headless: the target's size (a window sets it otherwise)
 */
#ifndef PORT_RR_H
#define PORT_RR_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* the backend, ready to draw; 0 when it cannot run here */
int  rr_open(void);
void rr_close(void);
/* the name of this backend ("metal", "gl", "vulkan", "soft", "null") */
const char *rr_name(void);
/* the target's size in pixels, for the frame about to be drawn */
void rr_target(int *w, int *h);
/* the frame starts: the whole target cleared to argb (0xAARRGGBB) */
void rr_begin(uint32_t argb);
/* w x h pixels (0xAARRGGBB, top row first) drawn over the target's
   rectangle d (x0, y0, x1, y1, target pixels, top left origin): sharp
   bilinear, or one to one when d is w x h */
void rr_image(const uint32_t *argb, int w, int h, const int d[4]);
/* A picture to compose on the GPU (render/compose/core.glsl), a frame's
   record (ports/common/video/video.h's VidFrame): the copies of the tiles
   (versions x 32,768 words) and sprite tables (versions x 544 bytes), each
   line's palette (225 x 256 colours, line 0 unused) and packed state (225
   x RR_LINE_WORDS words). */
#define RR_LINE_WORDS 20
#define RR_VERSIONS 4
typedef struct {
    const uint16_t *vram;
    const uint8_t *oam;
    int versions;
    const uint16_t *palette;
    const uint32_t *lines;
} RrPicture;
/* the picture composed (256 x 224) and drawn over rectangle d as rr_image
   draws */
void rr_compose(const RrPicture *p, const int d[4]);

/* The 3D canvas (render/fill/core.glsl): the Super FX's 256 x 256 screen
   of colour indices, filled on the GPU from a draw list's spans (the
   game's FxSpan, packed in RR_SPAN_WORDS words each:
     0  x | row << 8 | count (1-256) << 16
     1  colour | options << 8 | textured << 13 | colour depth << 14 |
        bank << 16
     2  u | v << 16          3  u step | v step << 16
     4  base | mask << 16    5-7 0).
   rr_fill_rom: the cartridge ROM the textures come from (size a whole
   number of KB), once; 0 when this backend has no canvas. rr_fill: the
   canvas cleared, then the n spans filled in order. rr_fill_read: the
   canvas read back, 256 x 256 (row 0 first): 0x100 | colour where a
   pixel was written, 0 elsewhere; 0 when this backend keeps none. */
#define RR_SPAN_WORDS 8
int  rr_fill_rom(const uint8_t *rom, uint32_t size);
void rr_fill(const uint32_t *spans, int n);
int  rr_fill_read(uint16_t *canvas);

/* the frame done: shown in the window, when there is one */
void rr_present(void);
/* the last presented frame, read back: rr_target's w x h pixels into argb;
   0 when this backend keeps none */
int  rr_read(uint32_t *argb);

#ifdef __cplusplus
}
#endif
#endif
