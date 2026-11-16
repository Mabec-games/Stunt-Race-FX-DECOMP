/*
 * screen.c: the screen's size from the window's shape, and where the HUD
 * floats on it.
 *
 * Wider than 4:3, the screen is 224 lines and the nearest even width;
 * narrower, 256 across and the nearest even height; up to 1024 each way.
 * The engine shows it at the console's pixel shape (aspect 0), so a window
 * that no even size fits exactly shows a sliver of border, never a
 * stretched picture. The original's 256 x 224 sits in the middle (the
 * origin), so the original's lines and columns stay about the screen's
 * centre.
 *
 * The HUD's box is the screen's middle, at most 340 wide (16:9 at 224
 * lines), so the HUD does not run to the far edges of a very wide window.
 * It is also kept inside the window's safe area (a notch, rounded corners,
 * a home bar).
 */
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "nshvc/nshvc.h"
#include "video/video.h"

enum {
    SCREEN_MAX = 1024,
    HUD_BOX_MAX = 340,
    MIDDLE_LEFT = 128 - 8,              /* a piece wholly left of this is the left's */
    MIDDLE_RIGHT = 128 + 8,             /* wholly right of this, the right's */
    VIEW_LEFT = 24,                     /* the original's view on its screen */
    VIEW_RIGHT = 232,
    VIEW_EDGE_ZONE = 56,                /* a piece within this of the view's left or right edge is the corner's */
    VIEW_CENTRE_Y = 96,
};

static ScreenShape shape = { 256, 224, 0, 0, { 0, 0, 0, 0 } };
static float check_window[6];          /* --window: width, height, then safe left, top, right, bottom */
static int checking;
static float window_w, window_h;        /* the window as screen_update last took it (a check's, else the display's) */

void screen_window_set(const float *values, int count)
{
    int i;

    for (i = 0; i < 6; i++)
        check_window[i] = i < count ? values[i] : 0.0f;
    checking = values[0] > 0.0f && values[1] > 0.0f;
}

/* SRFX_ASPECT=4:3 (the launcher's ASPECT): the original's 256 x 224 at any
   window, shown whole in its middle; else the screen widens with the window */
static int screen_four_three(void)
{
    static int on = -1;

    if (on < 0) {
        const char *s = getenv("SRFX_ASPECT");

        on = s && !strcmp(s, "4:3");
    }
    return on;
}

void screen_update(void)
{
    nshvc_display d = nshvc_display_info();
    float ww = (float)d.width, wh = (float)d.height, safe[4], a, per;
    int w = 256, h = 224, i;

    safe[0] = d.safe_left;
    safe[1] = d.safe_top;
    safe[2] = d.safe_right;
    safe[3] = d.safe_bottom;
    if (checking) {
        ww = check_window[0];
        wh = check_window[1];
        for (i = 0; i < 4; i++)
            safe[i] = check_window[2 + i];
    }
    if (ww > 0.0f && wh > 0.0f && !screen_four_three()) {
        a = (ww / wh) / (4.0f / 3.0f);
        if (a >= 1.0f) {
            w = (int)(256.0f * a / 2.0f + 0.5f) * 2;
            if (w > SCREEN_MAX)
                w = SCREEN_MAX;
        } else {
            h = (int)(224.0f / a / 2.0f + 0.5f) * 2;
            if (h > SCREEN_MAX)
                h = SCREEN_MAX;
        }
    }
    window_w = ww;
    window_h = wh;
    shape.width = w;
    shape.height = h;
    shape.origin_x = (w - 256) / 2;
    shape.origin_y = (h - 224) / 2;
    /* window pixels to screen pixels: the screen is shown whole, so the
       smaller of the two scales */
    per = ww > 0.0f && wh > 0.0f ? fminf((float)w / ww, (float)h / wh) : 0.0f;
    for (i = 0; i < 4; i++)
        shape.safe[i] = safe[i] > 0.0f ? (int)ceilf(safe[i] * per) : 0;
}

void screen_window(float *width, float *height)
{
    *width = window_w;
    *height = window_h;
}

const ScreenShape *screen_shape(void)
{
    return &shape;
}

/* the HUD box's inset from the screen's left and right edges */
static int box_left(void)
{
    int margin = shape.width > HUD_BOX_MAX ? (shape.width - HUD_BOX_MAX) / 2 : 0;

    return margin > shape.safe[0] ? margin : shape.safe[0];
}

static int box_right(void)
{
    int margin = shape.width > HUD_BOX_MAX ? (shape.width - HUD_BOX_MAX) / 2 : 0;

    return margin > shape.safe[2] ? margin : shape.safe[2];
}

/* A piece of the original's screen spanning columns x0 to x1: left of the
   middle it goes to the HUD box's left edge, right of it to the right edge,
   across it it stays centred. The columns to add to the original's. */
int screen_anchor_x(int x0, int x1)
{
    if (x1 < MIDDLE_LEFT)
        return box_left();
    if (x0 > MIDDLE_RIGHT)
        return shape.width - 256 - box_right();
    return shape.origin_x;
}

/* The same for a piece the original drew inside its view, against the
   view's edges rather than the screen's: the view's left edge goes to the
   box's left edge, its right edge to the box's right edge. */
int screen_view_anchor_x(int x0, int x1)
{
    /* only a piece at the view's own edges goes to the box's: the corner counters. What lies nearer the middle
       (a message, a staircase of marks, a few words) keeps its place about the screen's middle, as the original
       laid it, not thrown to opposite edges by being a little left or right of centre. */
    if (x1 < VIEW_LEFT + VIEW_EDGE_ZONE)
        return box_left() - VIEW_LEFT;
    if (x0 > VIEW_RIGHT - VIEW_EDGE_ZONE)
        return shape.width - box_right() - VIEW_RIGHT;
    return shape.origin_x;
}

/* The start lamps' pieces (the lights and their housings at the view's top
   corners, which the original's bezel cut) go to the screen's own edges, not
   the HUD box's: the box stops at 340 across, so on a very wide window the
   lamps would hang well inside it. Only the safe area keeps them in. */
int screen_lamp_anchor_x(int x0, int x1)
{
    if (x1 < MIDDLE_LEFT)
        return shape.safe[0] - VIEW_LEFT;
    if (x0 > MIDDLE_RIGHT)
        return shape.width - shape.safe[2] - VIEW_RIGHT;
    return shape.origin_x;
}

/* Down the screen by a piece's middle: above the view's centre it stays at
   the top, below it it goes to the bottom, about it it moves with the
   centre (the race's messages). The lines to add to the original's. */
int screen_anchor_y(int y0, int y1)
{
    int c = (y0 + y1) / 2;

    if (c < VIEW_CENTRE_Y - 16)
        return shape.safe[1];
    if (c > VIEW_CENTRE_Y + 16)
        return screen_bottom_shift();
    return shape.origin_y;
}

/* the lines the bottom band (the race's panels) moves down by */
int screen_bottom_shift(void)
{
    return shape.height - 224 - shape.safe[3];
}

/* ---- two players ---- */

/*
 * Each player's rectangle: side by side from 288 across, each (W - 2) / 2
 * wide and the screen's height; above and below otherwise, each the
 * screen's width and (H - 2) / 2 high. Two pixels between them stay clear.
 */
enum { TWO_SEAM = 2, TWO_SIDE_W = 288 };

int screen_two_side(void)
{
    /* SRFX_SPLIT=stacked (the launcher's 2P SPLIT): the players above and
       below at any width; else side by side once the screen is wide enough */
    static int stacked = -1;

    if (stacked < 0) {
        const char *s = getenv("SRFX_SPLIT");

        stacked = s && !strcmp(s, "stacked");
    }
    return !stacked && shape.width >= TWO_SIDE_W;
}

void screen_two_rect(int player, int *x, int *y, int *w, int *h)
{
    if (screen_two_side()) {
        *w = (shape.width - TWO_SEAM) / 2;
        *h = shape.height;
        *x = player ? shape.width - *w : 0;
        *y = 0;
    } else {
        *w = shape.width;
        *h = (shape.height - TWO_SEAM) / 2;
        *x = 0;
        *y = player ? shape.height - *h : 0;
    }
}

/* the safe area's insets on a player's rectangle: only where it meets the screen's edges */
static void two_margins(int player, int m[4])
{
    int x, y, w, h;

    screen_two_rect(player, &x, &y, &w, &h);
    m[0] = x == 0 ? shape.safe[0] : 0;
    m[1] = y == 0 ? shape.safe[1] : 0;
    m[2] = x + w == shape.width ? shape.safe[2] : 0;
    m[3] = y + h == shape.height ? shape.safe[3] : 0;
}

/*
 * A player's boxes (DAMAGE, the map, the speed, TIME: TWO_BOX_W x TWO_BOX_H
 * of the original's screen) float in its rectangle at one scale, in 256ths:
 * at most 0.6; side by side, a row along the bottom within 70% of the
 * rectangle's width; above and below, a column down the right within 80% of
 * its height and 30% of its width; never under a window pixel a source
 * pixel while that fits. The window pixels per screen pixel come from the
 * window (1 with none).
 */
enum { TWO_BOX_W = 80, TWO_BOX_H = 96, TWO_MAP_H = 35, TWO_BOX_GAP = 2, TWO_ROW_W = 240 + 3 * TWO_BOX_GAP };

int screen_two_scale(int player)
{
    nshvc_display d = nshvc_display_info();
    int x, y, w, h, q = 154, a, fit, least = 256;

    screen_two_rect(player, &x, &y, &w, &h);
    if (d.width > 0 && d.height > 0 && !checking) {
        float k = fminf((float)d.width / (float)shape.width, (float)d.height / (float)shape.height);

        least = k > 1.0f ? (int)(256.0f / k) : 256;
    }
    if (screen_two_side()) {
        a = 256 * 7 * w / (10 * TWO_ROW_W);
        fit = 256 * w / TWO_ROW_W;
    } else {
        a = 256 * 8 * h / (10 * TWO_BOX_H);
        if (a > 256 * 3 * w / (10 * TWO_BOX_W))
            a = 256 * 3 * w / (10 * TWO_BOX_W);
        fit = 256 * h / TWO_BOX_H;
    }
    q = q < a ? q : a;
    q = q > least ? q : least;
    q = q < fit ? q : fit;
    return q > 256 ? 256 : q;
}

/* the rows the boxes take at a rectangle's bottom, side by side */
static int two_band(int player)
{
    return screen_two_side() ? (TWO_MAP_H * screen_two_scale(player) + 255) / 256 + TWO_BOX_GAP : 0;
}

/*
 * Where the original's 144 x 80 view of a player goes for a piece of it
 * spanning x0 to x1 and y0 to y1 on that view: by the piece's middle, to
 * the near side of the rectangle or about its middle, inside the safe
 * area; side by side a low piece goes above the boxes' row; above and
 * below, a piece the boxes' column would cover moves to its left.
 */
void screen_two_anchor(int player, int x0, int x1, int y0, int y1, int *dx, int *dy)
{
    int x, y, w, h, m[4], cx = (x0 + x1) / 2, cy = (y0 + y1) / 2;

    screen_two_rect(player, &x, &y, &w, &h);
    two_margins(player, m);
    *dx = cx < 64 ? x + m[0] : cx > 80 ? x + w - 144 - m[2] : x + (w - 144) / 2;
    *dy = cy < 32 ? y + m[1] : cy > 48 ? y + h - 80 - m[3] : y + (h - 80) / 2;
    if (screen_two_side()) {
        if (cy > 48)
            *dy -= two_band(player);
    } else {
        int q = screen_two_scale(player), bx = x + w - m[2] - TWO_BOX_W * q / 256, by = y + m[1];

        if (*dy + y0 < by + TWO_BOX_H * q / 256 && *dy + y1 >= by && *dx + x1 >= bx)
            *dx = bx - 2 - x1;
    }
}

/*
 * A player's box b (0 DAMAGE, 1 the map, 2 the speed, 3 TIME): its place in
 * the player's 80 x 96 of the original's screen (the second player's map
 * and speed the other way round; the map's place runs a row above and two
 * below its box, for the map's sprites), and where it floats, at the
 * player's scale: side by side, in a row along the rectangle's bottom (DAMAGE
 * and the map at the left, the speed and TIME at the right); above and
 * below, as the original's column down the rectangle's right side.
 */
enum { TWO_MAP_TOP = 31, TWO_BOX_ROW = 32 };
static const int box_sx[2][4] = { { 0, 0, 32, 0 }, { 0, 48, 0, 0 } }, box_sy[4] = { 0, TWO_MAP_TOP, 32, 64 };
static const int box_sw[4] = { 80, 32, 48, 80 }, box_sh[4] = { TWO_BOX_ROW, TWO_MAP_H, TWO_BOX_ROW, TWO_BOX_ROW };

void screen_two_box_source(int player, int box, int *x, int *y, int *w, int *h)
{
    *x = box_sx[player & 1][box & 3];
    *y = box_sy[box & 3];
    *w = box_sw[box & 3];
    *h = box_sh[box & 3];
}

void screen_two_box(int player, int box, float *x, float *y, float *w, float *h)
{
    int rx, ry, rw, rh, m[4];
    float s = (float)screen_two_scale(player) / 256.0f, left, right, top, bottom, wide, gap;

    screen_two_rect(player, &rx, &ry, &rw, &rh);
    two_margins(player, m);
    left = (float)(rx + m[0]);
    right = (float)(rx + rw - m[2]);
    top = (float)(ry + m[1]);
    bottom = (float)(ry + rh - m[3]);
    wide = TWO_BOX_W * s;
    gap = TWO_BOX_GAP * s;
    box &= 3;
    *w = (float)box_sw[box] * s;
    *h = (float)box_sh[box] * s;
    if (!screen_two_side()) {
        *x = right - wide + (float)box_sx[player & 1][box] * s;
        *y = top + (float)box_sy[box] * s;
        return;
    }
    *y = bottom - *h;
    *x = box == 0 ? left : box == 1 ? left + wide + gap : box == 2 ? right - wide - gap - *w : right - wide;
}
