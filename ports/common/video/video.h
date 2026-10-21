/*
 * video.h: the picture, native (video.c). Platform agnostic C: the game's
 * screen state, tiles, palette and sprites in, a picture out.
 */
#ifndef PORT_VIDEO_H
#define PORT_VIDEO_H

#include <stdint.h>

#define VID_WIDTH  256
#define VID_HEIGHT 224

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
