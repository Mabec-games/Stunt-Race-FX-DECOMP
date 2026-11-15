/*
 * picture.c: the screen as the game has described it, handed to the engine
 * once per picture.
 */
#include <math.h>
#include <string.h>

#include "fx/fx_draw.h"
#include "race/race.h"
#include "system/state.h"
#include "video/between.h"
#include "video/picture.h"
#include "video/video.h"

void menu_lines_send(void);

Picture pic;
MenuVideo menu_video;

void video_init(void)
{
    menu_video.tiles_a = nshvc_tiles_create(4, 1024, 1);
    menu_video.tiles_b = nshvc_tiles_create(4, 1024, 1);
    menu_video.tiles_text = nshvc_tiles_create(2, 512, 1);
    menu_video.sprites = nshvc_tiles_create(4, 512, 1);
    menu_video.page_0 = nshvc_map_create(32, 32);
    menu_video.pages_0_1 = nshvc_map_create(64, 32);
    menu_video.pages_1_2 = nshvc_map_create(64, 32);
    menu_video.pages_2_3 = nshvc_map_create(64, 32);
    menu_video.text = nshvc_map_create(32, 32);
    race_video_init();
    pic.blank = 1;
}

/* a menu map write: with the update's other writes once the menus draw
   whole frames of the update before (video_between_menu_was_set) */
static void menu_map_write(nshvc_map m, uint32_t x, const uint16_t *w)
{
    if (video_between_menus_whole())
        video_map_write(m, x, 0, 32, 32, w);
    else
        nshvc_map_write_snes(m, x, 0, 32, 32, w);
}

void menu_page_write(int page, const void *words)
{
    const uint16_t *w = (const uint16_t *)words;

    if (page == 0) {
        menu_map_write(menu_video.page_0, 0, w);
        menu_map_write(menu_video.pages_0_1, 0, w);
    } else if (page == 1) {
        menu_map_write(menu_video.pages_0_1, 32, w);
        menu_map_write(menu_video.pages_1_2, 0, w);
    } else if (page == 2) {
        menu_map_write(menu_video.pages_1_2, 32, w);
        menu_map_write(menu_video.pages_2_3, 0, w);
    } else {
        menu_map_write(menu_video.pages_2_3, 32, w);
    }
}

static void layer_send(int n, const Background *b)
{
    nshvc_layer l;

    memset(&l, 0, sizeof l);
    l.enabled_main = (pic.main_screen >> n) & 1;
    l.enabled_sub = (pic.sub_screen >> n) & 1;
    l.tiles = b->tiles;
    l.map = b->map;
    l.draw_list = b->draw_list;
    l.tile_size = 8;
    l.scroll_x = b->scroll_x;
    l.scroll_y = b->scroll_y;
    l.mosaic = 1;
    l.wrap = 1;
    nshvc_layer_set((uint8_t)n, &l);
}

static void set_background(int n)
{
    layer_send(n, &pic.bg[n]);
}

/* The results' page (race_results_page) shows BG1 as the original did: the
   frame and panel rows as 8-bit tiles at their own priority, the map's row 0
   at line 16 as the frame's image has it, in the original's mode 3 (see
   draw_picture). The 3D's list is not shown: its rows are off. */
static void results_bg1_send(void)
{
    Background b = pic.bg[0];

    b.tiles = race_video.frame_tiles_one;
    b.map = race_video.frame_map_one;
    b.draw_list.id = 0;
    b.scroll_x = 0;
    b.scroll_y = 256 - 17;              /* line r shows the layer's line r + 1 + scroll, as the SNES's did */
    layer_send(0, &b);
}

/* ---- the race screen's lines ---- */

/* The rows the screen is lit on, by screen: one player one band, two players two. */
static void blank_lines_send(void)
{
    int32_t v[224];
    int r;

    for (r = 0; r < 224; r++) {
        if (pic.race.split)
            v[r] = !((r >= 15 && r <= 110) || (r >= 119 && r <= 214));
        else
            v[r] = !(r >= 17 && r <= 207);
    }
    nshvc_lines(0, 224, NSHVC_LINE_BLANK, v);
}

/* The gradient: colour 1 by row. One player: row 0's value held to row 31,
   then the second list row by row to row 158, its last held. Two players:
   the first band's start held, each half's list row by row, the last held. */
static uint16_t band_line_colour(int r)
{
    uint32_t a = pic.race.gradient_start[0], b = pic.race.gradient_start[1];

    r = r < 0 ? 0 : r > 223 ? 223 : r;
    if (pic.race.split) {
        if (r < 23 || (r >= 103 && r < 127))
            return gradient_colour(0);
        if (r < 103)
            return gradient_colour(a + (uint32_t)(r - 23));
        return gradient_colour(b + (uint32_t)((r < 207 ? r : 206) - 127));
    }
    if (r < 32)
        return gradient_colour(0);
    return gradient_colour(a + (uint32_t)((r < 159 ? r : 158) - 32));
}

static void gradient_lines_send(void)
{
    int32_t v[224];
    int r;

    for (r = 0; r < 224; r++)
        v[r] = band_line_colour(r);
    nshvc_lines(0, 224, (nshvc_line_field)(NSHVC_LINE_PALETTE + 1), v);
}

/* a window edge moving further between updates: set anew */
enum { LINE_EDGE_FAR = 64 };

/* The window bands' edges, a row's for each line of the table. */
static void window_band_values(int32_t l1[224], int32_t r1[224], int32_t l2[224], int32_t r2[224])
{
    int band, line = 0;

    memset(l1, 0, 224 * sizeof *l1);
    memset(r1, 0, 224 * sizeof *r1);
    memset(l2, 0, 224 * sizeof *l2);
    memset(r2, 0, 224 * sizeof *r2);
    for (band = 0; band < WINDOW_BANDS_MAX && windows.band[band].lines && line < 224; band++) {
        uint8_t c = windows.band[band].lines;
        int repeat = c >= 0x81, n = repeat ? c - 0x80 : c, i;
        const WindowEdges *e = windows.band[band].edges;

        for (i = 0; i < n && line < 224; i++, line++) {
            l1[line] = e->left1;
            r1[line] = e->right1;
            l2[line] = e->left2;
            r2[line] = e->right2;
            if (repeat)
                e = window_edges_next(windows.band[band].edges, i + 1);
        }
    }
    for (; line < 224; line++) {
        l1[line] = line ? l1[line - 1] : 0;
        r1[line] = line ? r1[line - 1] : 0;
        l2[line] = line ? l2[line - 1] : 0;
        r2[line] = line ? r2[line - 1] : 0;
    }
}

/* The course screen's BG2 scroll by line: in bands, by its table when the
   table is on. */
static void course_scroll_values(int32_t v[224])
{
    int r, at = 0, line = 0;
    int32_t scroll = g.course_scroll_top;

    for (r = 0; r < 55; r++)
        v[r] = scroll;
    scroll = (g.course_scroll_low & 0xFF) | (g.course_scroll_top & 0xFF00);
    for (; r < 224; r++)
        v[r] = scroll;
    if (g.line_effects.course_scroll) {
        const uint8_t *t = g.course_scroll_table;

        while (t[at] && line < 224) {
            int32_t value = t[at + 1] | t[at + 2] << 8;
            int n = t[at];

            for (r = line; r < line + n && r < 224; r++)
                v[r] = value;
            line += n;
            at += 3;
        }
        for (r = line; line && r < 224; r++)
            v[r] = v[line - 1];
    }
}

/* The course screen's wipe: window 1's edges by line from its table, in
   the original's 256 x 224 columns and lines. */
static void course_wipe_values(int32_t l1[224], int32_t r1[224])
{
    const uint8_t *t = windows.wipe_table;
    int r, at = 0, line = 0;

    while (at + 2 < (int)sizeof windows.wipe_table && t[at] && line < 224) {
        int n = t[at];

        for (r = line; r < line + n && r < 224; r++) {
            l1[r] = t[at + 1];
            r1[r] = t[at + 2];
        }
        line += n;
        at += 3;
    }
    for (r = line; r < 224; r++) {
        l1[r] = line ? l1[line - 1] : 1;
        r1[r] = line ? r1[line - 1] : 0;
    }
}

/* A picture between two updates draws the update before's world; its
   moving per-line tables are eased towards the newest world's, worked out
   here at the picture's start, before that world goes out
   (video_between_begin). Each is eased only when the newest world has it
   too. */
static struct {
    int ready, bands, course, wipe;
    int32_t band[4][224], scroll[224], wipe_edges[2][224];
} newest_lines;

void video_newest_lines_take(void)
{
    const Picture *now = video_between_newest();

    newest_lines.ready = 0;
    if (!video_between_eased())
        return;
    screen_update();
    newest_lines.bands = now->race.on && (now->race.split || g.line_effects.window_bands);
    if (newest_lines.bands)
        window_band_values(newest_lines.band[0], newest_lines.band[1], newest_lines.band[2], newest_lines.band[3]);
    newest_lines.course = now->race.on && now->race.course_screen;
    newest_lines.wipe = newest_lines.course && g.line_effects.course_wipe;
    if (newest_lines.course)
        course_scroll_values(newest_lines.scroll);
    if (newest_lines.wipe)
        course_wipe_values(newest_lines.wipe_edges[0], newest_lines.wipe_edges[1]);
    newest_lines.ready = 1;
}

/* The window bands: a row's edges for each line of the table. */
static void window_lines_send(void)
{
    int32_t l1[224], r1[224], l2[224], r2[224];

    window_band_values(l1, r1, l2, r2);
    if (newest_lines.ready && newest_lines.bands) {     /* the bands' edges as they move, between updates */
        tween_lines(l1, newest_lines.band[0], 224, LINE_EDGE_FAR, 0);
        tween_lines(r1, newest_lines.band[1], 224, LINE_EDGE_FAR, 0);
        tween_lines(l2, newest_lines.band[2], 224, LINE_EDGE_FAR, 0);
        tween_lines(r2, newest_lines.band[3], 224, LINE_EDGE_FAR, 0);
    }
    nshvc_lines(0, 224, NSHVC_LINE_WIN1_LEFT, l1);
    nshvc_lines(0, 224, NSHVC_LINE_WIN1_RIGHT, r1);
    nshvc_lines(0, 224, NSHVC_LINE_WIN2_LEFT, l2);
    nshvc_lines(0, 224, NSHVC_LINE_WIN2_RIGHT, r2);
}

/* The fixed colour by line from its table: each entry a line count and a
   byte naming colour channels, added on top of what the rows above left. */
static void fixed_line_values(uint16_t start, int32_t *v)
{
    int i = 0, line = 0;
    int r = start & 31, gc = (start >> 5) & 31, b = (start >> 10) & 31;

    memset(v, 0, 224 * sizeof *v);
    while (i + 1 < windows.level_table_bytes && windows.level_table[i] && line < 224) {
        int n = windows.level_table[i];
        uint8_t d = windows.level_table[i + 1];

        if (d & 0x80)
            b = d & 31;
        if (d & 0x40)
            gc = d & 31;
        if (d & 0x20)
            r = d & 31;
        while (n-- && line < 224)
            v[line++] = r | gc << 5 | b << 10;
        i += 2;
    }
    for (; line < 224; line++)
        v[line] = line ? v[line - 1] : (r | gc << 5 | b << 10);
}

static void fixed_lines_send(uint16_t start)
{
    int32_t v[224];

    fixed_line_values(start, v);
    nshvc_lines(0, 224, NSHVC_LINE_FIXED, v);
}

/* The lower half of a two-player screen: from row 112 its colours and fixed
   colour (its backdrop is in the draw list); the sprites from row 15's column 239. */
static void split_half_send(void)
{
    int r;

    nshvc_line(0, NSHVC_LINE_SPRITES_MAIN, 0);
    nshvc_line_from(15, 239, NSHVC_LINE_SPRITES_MAIN, 1);
    for (r = 0; r < 3; r++)
        nshvc_line(112, (nshvc_line_field)(NSHVC_LINE_PALETTE + 0xE1 + r), pic.race.colours_kept[1][r]);
    nshvc_line(112, (nshvc_line_field)(NSHVC_LINE_PALETTE + 0), pic.race.colour_0[1]);
    nshvc_line(112, NSHVC_LINE_FIXED, pic.race.fixed[1]);
}

/* The course screen: rows to $D8 lit; BG2's scroll in bands, by its table
   when the table is on; the sprites from row 55; window 1 from the wipe table. */
static void course_lines_send(int full)
{
    int32_t v[224], l1[224], r1[224];
    int r;

    if (!full) {                        /* in the whole window no line is blanked for the uploads */
        for (r = 0; r < 224; r++)
            v[r] = r >= 216;
        nshvc_lines(0, 224, NSHVC_LINE_BLANK, v);
    }
    nshvc_line(0, NSHVC_LINE_SPRITES_MAIN, 0);
    nshvc_line(55, NSHVC_LINE_SPRITES_MAIN, 1);
    course_scroll_values(v);
    if (newest_lines.ready && newest_lines.course)     /* the bands' scroll as it moves, between updates */
        tween_lines(v, newest_lines.scroll, 224, 0, TWEEN_SCROLL);
    nshvc_lines(0, 224, (nshvc_line_field)(NSHVC_LINE_SCROLL_Y + 1), v);
    if (g.line_effects.course_wipe) {
        course_wipe_values(l1, r1);
        if (newest_lines.ready && newest_lines.wipe) {  /* the wipe as it moves, between updates */
            tween_lines(l1, newest_lines.wipe_edges[0], 224, LINE_EDGE_FAR, 0);
            tween_lines(r1, newest_lines.wipe_edges[1], 224, LINE_EDGE_FAR, 0);
        }
        if (full && g.course_screen_state == 0x0A) {
            /* the wipe out of the screen (state $0A, WindowEdgesStep), whose
               rectangle grows from the records' box to the whole picture:
               laid over the whole window by overlays (course_wipe.c), as the
               picture's window and colour maths reach only the 256 x 224;
               the maths is left off for it */
            nshvc_blend b = pic.blend;

            course_wipe_overlay(l1, r1, &pic.blend);
            b.layers = 0;
            nshvc_blend_set(&b);
        } else {
            /* the zoom to the records' box, the box and the previews' bands
               stay about the cards they frame: window 1 and the colour maths
               at the original's place, which darken only the maths' layers
               (the records' text and the sprites stay lit) */
            int origin_x = full ? screen_shape()->origin_x : 0;

            course_wipe_overlay_off();
            for (r = 0; r < 224; r++) {
                l1[r] += origin_x;
                r1[r] += origin_x;
            }
            nshvc_lines(0, 224, NSHVC_LINE_WIN1_LEFT, l1);
            nshvc_lines(0, 224, NSHVC_LINE_WIN1_RIGHT, r1);
        }
    }
}

/* One player, the per-line tables the original turned on in $0D45: BG1 off
   in the middle rows (channel 1, the table RaceHudSetup builds: $20 lines
   $13, $80 lines $12, then $13), the gradient (channels 2 and 3), the window
   bands (4), the fixed colour's bands (5). Two players have them all on. */
static void race_lines_send(void)
{
    int one = !pic.race.split;

    if (pic.race.course_screen) {
        course_lines_send(race_course_full_window());
        return;
    }
    blank_lines_send();
    if (!one || g.line_effects.sky)
        gradient_lines_send();
    if (!one || g.line_effects.window_bands)
        window_lines_send();
    if (g.line_effects.fixed_bands)
        fixed_lines_send(pic.blend.fixed_bgr15);
    if (one && g.line_effects.results_bg1_off) {
        nshvc_line(0, NSHVC_LINE_MAIN + 0, 1);
        nshvc_line(32, NSHVC_LINE_MAIN + 0, 0);
        nshvc_line(160, NSHVC_LINE_MAIN + 0, 1);
    }
    if (one && g.results_page_shown) {
        /* BG2's map is the 32 x 32 page at $5C00: the backdrop map's second
           page, wrapping at 256 lines and columns */
        int32_t y[224];
        int r, sy = pic.race.backdrop_y[0];

        for (r = 0; r < 224; r++)
            y[r] = 256 + ((sy + r + 1) & 255) - (r + 1);
        nshvc_lines(0, 224, (nshvc_line_field)(NSHVC_LINE_SCROLL_Y + 1), y);
        nshvc_line(0, (nshvc_line_field)(NSHVC_LINE_SCROLL_X + 1), pic.race.backdrop_x[0] & 255);
    }
    if (!one)
        split_half_send();
}

/* ---- the one-player race in the whole window ---- */

/* The engine's origin stays at the screen's top left: the original's 256 x
   224 is placed here, in the layers' scrolls and the lines' numbers, so
   that every row of the screen can take its own sky. */

/* The sky (colour 1) on a screen row: race_full_sky_colour, by the row's
   height from the 3D's horizon. */
static int32_t full_sky_at(int row)
{
    return (int32_t)race_full_sky_colour(row);
}

/*
 * Colour 1 as the race screen shows it on screen row `row` (the screen's
 * pixels, as screen_shape gives them) in player `panel`'s view, for the
 * picture being drawn: what the 3D's ground lays its marks over. The full
 * windows read the day and night lists by the row's height from the 3D's
 * horizon (race_full_sky_colour, race_two_sky_colour), the one player's
 * showing colour 1 itself while the sky's lines are off; the original's
 * screens read their bands by line. Rows past a band's end give its last.
 */
uint16_t race_ground_colour(int panel, float row)
{
    const Game *w = video_between_g();  /* the picture's game state, asked during the 3D's compose too */
    int r = (int)floorf(row), line = r - screen_shape()->origin_y;

    if (race_two_full_window())
        return race_two_sky_colour(panel, r);
    if (pic.race.split)
        return band_line_colour(line);
    if (!w->line_effects.sky)
        return pic.palette[1];
    if (pic.race.on && !pic.race.course_screen && !w->results_page_shown)    /* race_full_window's, with the picture's state */
        return race_full_sky_colour(r);
    return band_line_colour(line);
}

/* Whether each window of the band table covers any of the screen: the
   original's windows were the view's opening (or nothing), and with no
   bezel the view is the whole screen, so a window of the view covers
   everything and one of nothing covers nothing. */
static void full_windows(nshvc_windows *w, int width)
{
    int band, line = 0, cover[2] = { 0, 0 }, i;

    for (band = 0; band < WINDOW_BANDS_MAX && windows.band[band].lines && line < 224; band++) {
        uint8_t c = windows.band[band].lines;
        int repeat = c >= 0x81, n = repeat ? c - 0x80 : c, k;
        const WindowEdges *e = windows.band[band].edges;

        for (k = 0; k < n && line < 224; k++, line++) {
            cover[0] |= e->left1 <= e->right1;
            cover[1] |= e->left2 <= e->right2;
            if (repeat)
                e = window_edges_next(windows.band[band].edges, k + 1);
        }
    }
    for (i = 0; i < 2; i++) {
        w->w[i].left = cover[i] ? 0 : 1;
        w->w[i].right = cover[i] ? width - 1 : 0;
    }
}

/* A window of the band table that is not the view's opening (its edges
   moving from line to line: RETIRE?'s oval) is drawn as the original drew
   it, line by line where the original's 256 x 224 lies in the window, and
   covers nothing beyond; returns 0 for the view's opening (full_windows). */
static int full_window_shaped(const ScreenShape *sh)
{
    static int32_t l1[224], r1[224], l2[224], r2[224], rows[4][1024];
    int line, r, k, shaped = 0, first[2] = { -1, -1 };

    window_band_values(l1, r1, l2, r2);
    for (line = 0; line < 224; line++) {
        int32_t lw[2] = { l1[line], l2[line] }, rw[2] = { r1[line], r2[line] };
        for (k = 0; k < 2; k++) {
            if (lw[k] > rw[k])
                continue;
            if (first[k] < 0)
                first[k] = line;
            else if (lw[k] != (k ? l2 : l1)[first[k]] || rw[k] != (k ? r2 : r1)[first[k]])
                shaped = 1;
        }
    }
    if (!shaped)
        return 0;
    for (r = 0; r < sh->height && r < 1024; r++) {
        line = r - sh->origin_y;
        if (line < 0 || line > 223) {
            rows[0][r] = rows[2][r] = 1;
            rows[1][r] = rows[3][r] = 0;
            continue;
        }
        rows[0][r] = l1[line] <= r1[line] ? l1[line] + sh->origin_x : 1;
        rows[1][r] = l1[line] <= r1[line] ? r1[line] + sh->origin_x : 0;
        rows[2][r] = l2[line] <= r2[line] ? l2[line] + sh->origin_x : 1;
        rows[3][r] = l2[line] <= r2[line] ? r2[line] + sh->origin_x : 0;
    }
    k = sh->height < 1024 ? sh->height : 1024;
    nshvc_lines(0, (uint32_t)k, NSHVC_LINE_WIN1_LEFT, rows[0]);
    nshvc_lines(0, (uint32_t)k, NSHVC_LINE_WIN1_RIGHT, rows[1]);
    nshvc_lines(0, (uint32_t)k, NSHVC_LINE_WIN2_LEFT, rows[2]);
    nshvc_lines(0, (uint32_t)k, NSHVC_LINE_WIN2_RIGHT, rows[3]);
    return 1;
}

static void full_window_draw(const ScreenShape *sh)
{
    static uint16_t palette[256];
    static int32_t rows[1024];
    nshvc_windows w = pic.windows;
    Background hills = pic.bg[1];
    static nshvc_sprite placed[128];
    int sky = g.line_effects.sky, r;

    race_hills_full();
    layer_send(0, &pic.bg[0]);
    hills.map = race_video.backdrop_wide;
    hills.scroll_x = pic.race.wide_x - sh->origin_x;
    hills.scroll_y = pic.race.wide_y - sh->origin_y;
    layer_send(1, &hills);
    layer_send(2, &pic.bg[2]);
    if (g.line_effects.window_bands && !full_window_shaped(sh))
        full_windows(&w, sh->width);
    nshvc_windows_set(&w);
    nshvc_blend_set(&pic.blend);
    memcpy(palette, pic.palette, sizeof palette);
    if (sky)
        palette[1] = (uint16_t)full_sky_at(0);
    nshvc_palette_write(0, 256, palette);
    nshvc_sprites_show((pic.main_screen >> 4) & 1, (pic.sub_screen >> 4) & 1);
    sprites_place_full(placed, pic.sprites, pic.sprite_count);
    nshvc_sprites_set(placed, pic.sprite_count, 0);
    if (sky) {
        for (r = 0; r < sh->height; r++)
            rows[r] = full_sky_at(r);
        nshvc_lines(0, (uint32_t)sh->height, (nshvc_line_field)(NSHVC_LINE_PALETTE + 1), rows);
    }
    if (g.line_effects.fixed_bands) {
        int32_t lines[224];

        fixed_line_values(pic.blend.fixed_bgr15, lines);
        for (r = 0; r < sh->height; r++) {
            int line = r - sh->origin_y;

            rows[r] = lines[line < 0 ? 0 : line > 223 ? 223 : line];
        }
        nshvc_lines(0, (uint32_t)sh->height, NSHVC_LINE_FIXED, rows);
    }
}

/* Two players in the whole window: the frame's layer off (the boxes are in
   the list), each player's sky, hills and 3D in the list on BG2, the
   sprites placed by player. Above and below, the second player's colours
   $E1-$E3, colour 0 and fixed colour from its rectangle's first row, as
   the original changed them at line 112. */
static void full_two_draw(const ScreenShape *sh)
{
    static nshvc_sprite placed[128];
    nshvc_windows w = pic.windows;
    Background none;
    int r;

    memset(&none, 0, sizeof none);
    race_two_player_list();
    layer_send(0, &none);
    layer_send(1, &pic.bg[1]);
    layer_send(2, &none);
    full_windows(&w, sh->width);
    nshvc_windows_set(&w);
    nshvc_blend_set(&pic.blend);
    nshvc_palette_write(0, 256, pic.palette);
    nshvc_sprites_show(1, 0);
    sprites_place_two(placed, pic.sprites, pic.sprite_count);
    nshvc_sprites_set(placed, pic.sprite_count, 0);
    if (!screen_two_side()) {
        int x, y, wd, ht;

        screen_two_rect(1, &x, &y, &wd, &ht);
        for (r = 0; r < 3; r++)
            nshvc_line((uint32_t)y, (nshvc_line_field)(NSHVC_LINE_PALETTE + 0xE1 + r), pic.race.colours_kept[1][r]);
        nshvc_line((uint32_t)y, (nshvc_line_field)(NSHVC_LINE_PALETTE + 0), pic.race.colour_0[1]);
        nshvc_line((uint32_t)y, NSHVC_LINE_FIXED, pic.race.fixed[1]);
    }
}

static void draw_picture(void);

/* The picture: between two updates the update before's game state drawn
   (video_between_world_in), its moving per-line tables eased towards the
   newest world's (video_newest_lines_take). */
void video_draw(void)
{
    video_between_world_in();
    draw_picture();
    video_between_world_out();
}

static void draw_picture(void)
{
    nshvc_screen s;
    const ScreenShape *sh;
    int i, full, menu_full, course_full, results;

    screen_update();
    sh = screen_shape();
    full = race_full_window() || race_two_full_window();
    menu_full = pic.fills_window && !pic.race.on;
    course_full = race_course_full_window();
    if (pic.race.on && pic.race.course_screen)
        pic.bg[0].draw_list = fx_draw_picture(0);   /* the static as this frame's loop drew it */
    memset(&s, 0, sizeof s);
    s.width = full || menu_full || course_full ? sh->width : 256;
    s.height = full || menu_full || course_full ? sh->height : 224;
    s.origin_x = menu_full || course_full ? sh->origin_x : 0;   /* the race places its own (full_window_draw) */
    s.origin_y = menu_full || course_full ? sh->origin_y : 0;
    s.brightness = pic.brightness;
    s.blank = pic.blank;
    nshvc_screen_set(&s);
    if (!(course_full && g.line_effects.course_wipe))
        course_wipe_overlay_off();      /* the wipe's overlays only on its screen, blank or not */
    if (pic.blank)
        return;
    /* The race keeps mode 1 for its draw list (race_screen_layers); the
       results' page has no list and BG2's page at priority 0, so it takes the
       original's mode 3: the page's sprites (the car, priority 1) over the
       page and under the frame, as they showed. */
    results = race_results_page();
    nshvc_mode_set(results ? 3 : pic.mode, pic.bg3_priority, 0);
    {
        nshvc_limits l;

        memset(&l, 0, sizeof l);
        if (menu_full)
            memcpy(l.layer_clip, pic.layer_clip, sizeof l.layer_clip);
        if (course_full)
            l.layer_clip[1] = l.layer_clip[4] = 1;      /* the cards and sprites kept to the original's picture */
        nshvc_limits_set(&l);
    }
    if (full) {
        if (pic.race.split)
            full_two_draw(sh);
        else
            full_window_draw(sh);
        return;
    }
    if (pic.race.on && pic.race.split && !pic.race.course_screen)
        race_two_player_list();
    for (i = 0; i < 3; i++)
        set_background(i);
    if (results)
        results_bg1_send();
    nshvc_windows_set(&pic.windows);
    nshvc_blend_set(&pic.blend);
    nshvc_palette_write(0, 256, pic.palette);
    nshvc_sprites_show((pic.main_screen >> 4) & 1, (pic.sub_screen >> 4) & 1);
    if (menu_full || course_full) {
        static nshvc_sprite moved[128];
        uint32_t k, n = pic.sprite_count > 128 ? 128 : pic.sprite_count;

        for (k = 0; k < n; k++) {
            moved[k] = pic.sprites[k];
            moved[k].x += sh->origin_x;
            moved[k].y += sh->origin_y;
        }
        nshvc_sprites_set(moved, n, 0);
    } else if (pic.race.on && race_ending_running()) {
        /* the ending shows no panels: the HUD's sprites in their band (from line 161) are left out */
        static nshvc_sprite kept[128];
        uint32_t k, n = pic.sprite_count > 128 ? 128 : pic.sprite_count;

        for (k = 0; k < n; k++) {
            kept[k] = pic.sprites[k];
            if (kept[k].priority == 3 && kept[k].y + kept[k].height * 8 > 161)
                kept[k].y = 0x7FF;
        }
        nshvc_sprites_set(kept, n, 0);
    } else
        nshvc_sprites_set(pic.sprites, pic.sprite_count, 0);
    if (pic.line_tables)
        menu_lines_send();
    if (pic.race.on)
        race_lines_send();
}
