/*
 * video.h: the picture, native (video.c). Platform agnostic C: the game's
 * screen state, tiles, palette and sprites in, a picture out.
 */
#ifndef PORT_VIDEO_H
#define PORT_VIDEO_H

#include <stdint.h>

#define VID_WIDTH  256
#define VID_HEIGHT 224

/* The screen state a line is drawn from, as it stood at the line's
   horizontal blank. */
typedef struct {
    uint8_t inidisp, obsel, bgmode, mosaic, bgsc[4], bgnba[2], m7sel;
    uint8_t w12sel, w34sel, wobjsel, wh[4], wbglog, wobjlog, tm, ts, tmw, tsw, cgwsel, cgadsub, setini;
    uint16_t bghofs[4], bgvofs[4];
    uint16_t m7hofs, m7vofs;
    int16_t m7a, m7b, m7c, m7d, m7x, m7y;
    uint8_t fixed_r, fixed_g, fixed_b;
    uint16_t oamaddr;
    uint8_t oam_prio;
} VidScreen;

/* A frame, everything its picture is drawn from: each line's screen state
   and palette, and the copies of the tiles and sprite table its lines show
   (more than one when the game changed them in a blanked stretch of the
   frame); what the GPU compositor draws from. */
#define VID_VERSIONS 4
typedef struct {
    VidScreen line[VID_HEIGHT + 1];         /* lines 1-224 */
    uint16_t palette[VID_HEIGHT + 1][256];
    uint8_t version[VID_HEIGHT + 1];        /* the copy each line shows */
    int versions;
    uint16_t vram[VID_VERSIONS][0x8000];
    uint8_t oam[VID_VERSIONS][544];
} VidFrame;

/* the last finished frame's record */
const VidFrame *vid_frame(void);
/* a frame drawn from its record (the reference the GPU compositor is held
   to): VID_WIDTH x VID_HEIGHT pixels, 0xFFRRGGBB */
void vid_draw_frame(const VidFrame *f, uint32_t *out);

void vid_reset(void);
/* One part of the screen state: the console picture unit's register
   $2100 + reg ($00-$33) given value v, as the game writes it. */
void vid_reg(uint8_t reg, uint8_t v);
void vid_frame_start(void);              /* line 0 */
void vid_render_line(unsigned line);     /* lines 1-224 */
void vid_vblank_start(void);             /* line 225: the frame is done */
int vid_forced_blank(void);
/* The last finished frame: VID_WIDTH x VID_HEIGHT pixels, 0xFFRRGGBB. */
const uint32_t *vid_picture(void);

#endif
