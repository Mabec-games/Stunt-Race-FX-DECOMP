/*
 * race_video.c: the race screen's picture as tile sets and maps: what it loads
 * at the start of a course and what changes each frame. The 4bpp art the
 * backdrop and the sprites share is one tile set; the frame around the 3D and
 * the HUD art another, 8bpp in the one-player screen.
 */
#include <math.h>
#include <string.h>

#include "audio/sound.h"
#include "data/menu_tables.h"
#include "data/race_tables.h"
#include "data/video_tables.h"
#include "fx/fx.h"
#include "fx/fx_draw.h"
#include "fx/fx_rt.h"
#include "math/math.h"
#include "race/race.h"
#include "object/object.h"
#include "system/chip.h"
#include "system/game.h"
#include "system/state.h"
#include "video/between.h"
#include "video/picture.h"
#include "video/video.h"

#include <stdio.h>

RaceVideo race_video;

/* the course backdrop's map, as the course data unpacks it */
/* $7F:93D7 on, as one block: the course's two backdrop maps ($93D7, $BBD7),
   then the rows for each picture buffer ($E3D7, $E817). A row read past the
   end of a map goes on into what follows, as the original's did. */
static uint16_t bank7f_words[2 * 0x1400 + 2 * 0x220];
#define backdrop_map(n) (bank7f_words + (n) * 0x1400)
#define backdrop_rows(n) (bank7f_words + 2 * 0x1400 + (n) * 0x220)

/* The full window's hills. A course's backdrop map is 160 tiles a row, 32
   rows: four screens of 32 tiles and the first again (course_map_rows), so
   its round is 128 tiles, 1024 pixels, and the heading turns it once. The
   wide map holds that round whole, so it wraps where the heading does; the
   17 rows the view looks at go 64 rows lower than on the original's map,
   with the sky tile above and below them, so a tall screen shows sky and
   ground (colour 1 by line) and the band once. */
enum { WIDE_COLUMNS = 128, WIDE_ROWS = 192, WIDE_DROP = 64, SKY_TILE = 0x20C0 };
typedef struct { int map; uint16_t heading, pitch, row_adjust; } WideBand;
static WideBand wide_band[2];           /* what BackdropRowsUpdate took, by buffer (written with the update's art) */
static int shown_buffer;

/* The one-player race fills the window: its screen, not the course
   screen's, the split screen's or the results' page. */
/* Two players fill the window: once the 3D draws each player's picture in
   its rectangle (the Enhanced 3D session turns this on with that). */
static int two_full_ready;

/* A half the results or the quit menu blanked (split_picture): the full
   window draws no 3D there until the race's own uploads resume. */
static uint8_t half_blank[2];

void race_half_blank_set(int half, int on)
{
    half_blank[half & 1] = (uint8_t)(on != 0);
}

int race_half_blank(int half)
{
    return half_blank[half & 1];
}

void race_two_full_window_ready(int on)
{
    two_full_ready = on;
}

int race_two_full_window(void)
{
    return two_full_ready && pic.race.on && pic.race.split && !pic.race.course_screen;
}

int race_full_window(void)
{
    return pic.race.on && !pic.race.split && !pic.race.course_screen && !g.results_page_shown;
}

/* The results' page over the one-player race screen (the results, the menu
   after a stunt race, the championship's pages): BG2 shows the page, BG1
   the frame and panel rows (results_bg1_off turns its 3D rows off). */
int race_results_page(void)
{
    return pic.race.on && !pic.race.split && !pic.race.course_screen && g.results_page_shown;
}

/* ---- the objects ---- */

void race_video_init(void)
{
    race_video.tiles = nshvc_tiles_create(4, 1024, 1);
    race_video.backdrop_one = nshvc_map_create(32, 64);
    race_video.backdrop_wide = nshvc_map_create(WIDE_COLUMNS, WIDE_ROWS);
    race_video.frame_tiles_two = nshvc_tiles_create(4, 512, 1);
    race_video.frame_map_two = nshvc_map_create(32, 32);
    race_video.backdrop_image = nshvc_image_create(512, 512, NSHVC_IMAGE_INDEXED);
    race_video.panel_image = nshvc_image_create(256, 256, NSHVC_IMAGE_INDEXED);
    race_video.course_tiles = nshvc_tiles_create(4, 1024, 1);
    race_video.course_map = nshvc_map_create(32, 64);
    race_video.frame_image = nshvc_image_create(256, 224, NSHVC_IMAGE_INDEXED);
    race_video.frame_tiles_one = nshvc_tiles_create(8, 1024, 1);
    race_video.frame_map_one = nshvc_map_create(32, 32);
    sprites_use_tiles(race_video.tiles, 0x200, 0x300);
}

static void frame_art_clear(void);

/* The two-player screen's backdrop and panel are drawn into the picture's
   list as images, so the shared art and their maps are kept here too. */
static uint8_t race_art_bytes[0x8000];
/* the two-player frame's map and art as written, for the full window's boxes */
static uint16_t frame_two_words[32 * 32];
static uint8_t frame_two_bytes[0x4000];
static int two_boxes_stale = 1;
/* the race's sprite art by palette, for sprites drawn into the list */
static nshvc_image sprite_atlas[8];
static uint8_t sprite_atlas_made[8];
static uint16_t backdrop_two_words[4][0x400];
static uint16_t panel_words[0x400];
static int two_images_stale = 1;

/* A copy the pictures are drawn from marked to be cut again; with the
   writes of an update, at the next update's start (video_copy). */
static void stale(int *flag)
{
    static const int one = 1;

    video_copy(flag, &one, sizeof one);
}

/* every tile and map zero, as the original cleared VRAM */
static void clear_all(void)
{
    static const uint8_t zero_art[1024 * 32];
    static const uint16_t zero_words[32 * 32];
    int p;

    video_tiles_write(race_video.tiles, 0, zero_art, 1024, NSHVC_TILES_PLANAR, sizeof zero_art);
    frame_art_clear();
    for (p = 0; p < 2; p++)
        video_map_write(race_video.backdrop_one, 0, (uint32_t)p * 32u, 32, 32, zero_words);
    video_map_write(race_video.frame_map_two, 0, 0, 32, 32, zero_words);
}

/* The race art from `character` on (32 bytes a character): the tile set
   the layers and sprites draw from, and the copy the list's images are cut
   from. */
void race_art_write(unsigned character, const uint8_t *data, uint32_t bytes)
{
    if (character * 32u + bytes > sizeof race_art_bytes)
        return;
    video_tiles_write(race_video.tiles, character, data, bytes / 32u, NSHVC_TILES_PLANAR, bytes);
    video_copy(race_art_bytes + character * 32u, data, bytes);
    stale(&two_images_stale);
    video_fill(sprite_atlas_made, 0, sizeof sprite_atlas_made);
}

static uint8_t frame_art_8[1024 * 64];       /* the one-player frame and HUD art, 8bpp, as loaded */
static uint16_t frame_words_1p[32 * 32];     /* the one-player BG1 map, first buffer's */
static int frame_image_stale = 1;

static void frame_art_clear(void)
{
    static const uint8_t zero_art_8[1024 * 64];
    static const uint16_t zero_words[32 * 32];

    video_tiles_write(race_video.frame_tiles_one, 0, zero_art_8, 1024, NSHVC_TILES_PLANAR, sizeof zero_art_8);
    video_map_write(race_video.frame_map_one, 0, 0, 32, 32, zero_words);
    video_fill(frame_art_8, 0, sizeof frame_art_8);
    video_fill(race_art_bytes, 0, sizeof race_art_bytes);
    video_fill(backdrop_two_words, 0, sizeof backdrop_two_words);
    video_fill(panel_words, 0, sizeof panel_words);
    stale(&two_images_stale);
    video_fill(frame_words_1p, 0, sizeof frame_words_1p);
    stale(&frame_image_stale);
}

/* The one-player frame's 8bpp art from `character` on (64 bytes a character). */
void race_frame_art_write(unsigned character, const uint8_t *data, uint32_t bytes)
{
    if (character * 64u + bytes > sizeof frame_art_8)
        return;
    video_tiles_write(race_video.frame_tiles_one, character, data, bytes / 64u, NSHVC_TILES_PLANAR, bytes);
    video_copy(frame_art_8 + character * 64u, data, bytes);
    stale(&frame_image_stale);
}

/* With two players, BG1's art from `character` on: the halves' 3D characters
   (0 the top's, TWO_HALF_CHARACTERS the bottom's), then the frame's from
   RACE_FRAME_TWO_FRAME; kept too for the full window's boxes. */
void race_frame_two_art_write(unsigned character, const uint8_t *data, uint32_t bytes)
{
    video_tiles_write(race_video.frame_tiles_two, character, data, bytes / 32u, NSHVC_TILES_PLANAR, bytes);
    if (character * 32u + bytes <= sizeof frame_two_bytes)
        video_copy(frame_two_bytes + character * 32u, data, bytes);
    stale(&two_boxes_stale);
}

/* The course screen's art from `character` on, and its map's first `rows` rows. */
void race_course_screen_art_write(unsigned character, const uint8_t *data, uint32_t bytes)
{
    video_tiles_write(race_video.course_tiles, character, data, bytes / 32u, NSHVC_TILES_PLANAR, bytes);
}

void race_course_screen_map_write(const uint16_t *words, unsigned rows)
{
    video_map_write(race_video.course_map, 0, 0, 32, rows, words);
}

/* ---- the results' and the quit menu's uploads, by what they hold ---- */

enum {
    RESULTS_PICTURE = RACE_ART_SPRITES + 0x0C0, /* the sprites' characters $C0-$10F */
    RESULTS_ROWS = RACE_ART_SPRITES + 0x024,    /* $24-$2D, the next row $34-$3D */
    CAR_PICTURE = RACE_ART_SPRITES + 0x060,     /* $60-$BF */
    TWO_HALF_CHARACTERS = 0x0B4,                /* each half's 3D characters with two players */
};

/* The results' big picture: 5 rows of 16 characters, and its fourth and fifth
   rows' first ten characters again as the sprites' $24-$2D and $34-$3D. */
void race_results_picture_write(const uint8_t *picture)
{
    race_art_write(RESULTS_PICTURE, picture, 0x0A00);
    race_art_write(RESULTS_ROWS, picture + 0x0600, 0x0140);
    race_art_write(RESULTS_ROWS + 0x10, picture + 0x0800, 0x0140);
}

/* the results' car picture */
void race_car_picture_write(const uint8_t *picture)
{
    race_art_write(CAR_PICTURE, picture, 0x0C00);
}

/*
 * @routine 7F:0B56 CoursePictureToVram
 * @routine 7F:0BCE VramWriteRowFromGsuRam
 * WHAT IT DOES: Four 64-word rows of the course's picture, as unpacked at
 * FX_UNPACK_AREA, to the sprites' characters $48, $58, $4C and $5C (four
 * each), from the place the course's entry gives
 * (menu_course_picture_place). The race's art load and the two-player
 * course screen both call it.
 */
void race_course_picture_write(void)
{
    static const unsigned to[4] = { RACE_ART_SPRITES + 0x48, RACE_ART_SPRITES + 0x58, RACE_ART_SPRITES + 0x4C, RACE_ART_SPRITES + 0x5C };
    uint16_t w = menu_course_picture_place(g.course), t = (uint16_t)(w & 0xFFFC);
    uint16_t at = (uint16_t)((uint16_t)((uint16_t)(t >> 8 | t << 8) << 1) + ((w & 3) << 7));
    int i;

    for (i = 0; i < 4; i++, at = (uint16_t)(at + 0x0200))
        race_art_write(to[i], fx_ram + FX_UNPACK_AREA + at, 0x80);
}

/* two players: the half's 3D characters (0 the top, 1 the bottom), from the
   map buffer when the quit menu opens over it */
void race_half_characters_write(int half, const uint8_t *characters)
{
    race_frame_two_art_write(half ? TWO_HALF_CHARACTERS : 0, characters, TWO_HALF_CHARACTERS * 32u);
}

/* The panel's map over a half (a finished player's, or the quit menu's);
   drawn as an image into the list, from the shared art at tile $300 on. */
void race_panel_map_write(const uint16_t *words)
{
    video_copy(panel_words, words, sizeof panel_words);
    stale(&two_images_stale);
}

/* BG1 of the two-player screen holds only the frame and panels at their own
   priority; the entries for the 3D are left empty, the list draws those. */
void race_frame_map_two_write(const uint16_t *words)
{
    uint16_t frame_only[32 * 32];
    int i;

    for (i = 0; i < 32 * 32; i++)
        frame_only[i] = (words[i] & 0x3FF) >= 0x168 ? words[i] : 0;
    video_map_write(race_video.frame_map_two, 0, 0, 32, 32, frame_only);
    video_copy(frame_two_words, frame_only, sizeof frame_two_words);
    stale(&two_boxes_stale);
}

/* `n` pages of the two-player backdrop from `first`. */
void race_backdrop_two_pages_write(int first, const uint16_t *words, uint32_t n)
{
    uint32_t i;

    for (i = 0; i < n; i++)
        race_backdrop_two_write(first + (int)i, words + i * 0x400);
}

/* A page of the two-player backdrop's own four. */
void race_backdrop_two_write(int page, const uint16_t *words)
{
    video_copy(backdrop_two_words[page], words, sizeof backdrop_two_words[page]);
    stale(&two_images_stale);
}

/* ---- one unpacked block to the art or a map ---- */

/* the chip's RAM from the unpack area on */
enum { UNPACK_ROOM = 0xC000 };

/* A block unpacked to the chip RAM's $3000; the length its transfer record
   gives (where the original sent it is each caller's named destination). */
static uint32_t unpacked(VideoBlock block)
{
    return video_block_unpack(block, fx_ram + FX_UNPACK_AREA, UNPACK_ROOM);
}

static uint32_t course_unpacked(CourseBlock block)
{
    return course_block_unpack(g.course, block, fx_ram + FX_UNPACK_AREA, UNPACK_ROOM);
}

/* The one-player frame's art by character: its frame and HUD from FRAME_HUD,
   the mode's HUD pieces from FRAME_HUD_PIECES, the car's from FRAME_HUD_CAR. */
enum { FRAME_HUD = 0x2B0, FRAME_HUD_PIECES = 0x2D6, FRAME_HUD_CAR = 0x2DA };

/* A frame row's word and a picture's words to the first (buffer 0) or second
   page. The first is also BG1's map as the results' page shows it: its 3D
   rows name characters below FRAME_HUD, which the 8-bit set never gets
   (the 3D is the list's), so they show nothing. */
void race_map_page_write(int page, const uint16_t *words)
{
    if (page == 0) {
        video_map_write(race_video.frame_map_one, 0, 0, 32, 32, words);
        video_copy(frame_words_1p, words, sizeof frame_words_1p);
        stale(&frame_image_stale);
    }
}

/* ---- the framed panels ---- */

static void frame_rows(uint16_t *out, uint16_t base)
{
    int i;
    uint16_t t = 0x2002;

    if ((g.hud_mode == 1 || g.hud_mode == 3) && !g.race_skipped)
        t = 0x2010;
    for (i = 0; i < 0x20; i++)
        out[i] = (uint16_t)(0x2001 + base);
    for (; i < 0x20 + 0x23; i++)
        out[i] = (uint16_t)(t + base);
}

static void gradient_rows(uint16_t *out, uint16_t start)
{
    int r, c;

    for (r = 0; r < 16; r++)
        for (c = 0; c < 32; c++)
            out[r * 32 + c] = (uint16_t)(start + r + c * 16);
}

static void picture_rows(uint16_t *out, uint16_t base)
{
    int i;

    for (i = 0; i < 192; i++)
        out[i] = (uint16_t)(fx16((uint16_t)(FX_UNPACK_AREA + i * 2)) + base);
}

/* The two panels of frame, gradient and picture rows. */
static void screen_frame_and_pictures(void)
{
    uint16_t page[32 * 32];
    int p;
    static const uint16_t base[2] = {0x02B0, 0x01B0};
    static const uint16_t grad[2] = {0x2000, 0x2010};

    for (p = 0; p < 2; p++) {
        memset(page, 0, sizeof page);
        frame_rows(page, base[p]);
        gradient_rows(page + 0x43, grad[p]);
        picture_rows(page + 0x240, base[p]);
        race_map_page_write(p, page);
    }
}

/* ---- the race screen's setup ---- */

/* Colour 0 black, every screen setting off. */
void VideoRegsReset(void)
{
    pic.main_screen = pic.sub_screen = 0;
    memset(&pic.windows, 0, sizeof pic.windows);
    memset(&pic.blend, 0, sizeof pic.blend);
    pic.blend.use_fixed = 1;
    pic.palette[0] = 0;
    pic.line_tables = 0;
    g.results_page_shown = 0;           /* BG2 on the race's hills again */
    fx16_set(GSU_SCREEN_START, FX_PICTURE_0);
    g.screen_start = FX_PICTURE_0;
}

/* The maps of the course: rows copied out of the unpacked picture, with the priority bit. */
static void course_map_rows(int which)
{
    uint16_t x, y = 0;
    static const uint16_t src[5] = {0x0000, 0x0800, 0x1000, 0x1800, 0x0000};
    int r, n;

    for (x = 0; x < 0x800; x = (uint16_t)(x + 0x40))
        for (n = 0; n < 5; n++)
            for (r = 0; r < 32; r++) {
                backdrop_map(which)[y] = (uint16_t)(fx16((uint16_t)(FX_UNPACK_AREA + src[n] + x + 2 * r)) | 0x2000);
                y++;
            }
    }

static void hud_art_mode0(void)
{
    uint16_t k = (uint16_t)((g.car_index_kept & 0x0F) << 8);
    uint8_t row[0x100];
    int i;

    race_frame_art_write(FRAME_HUD, fx_ram + FX_UNPACK_AREA, unpacked(VIDEO_BLOCK_HUD_FRAME_SPEED));
    unpacked(VIDEO_BLOCK_HUD_CARS);
    for (i = 0; i < 0x100; i++)
        row[i] = fx_ram[FX_UNPACK_AREA + k + (uint16_t)i];
    race_frame_art_write(FRAME_HUD_CAR, row, 0x100);
    unpacked(VIDEO_BLOCK_AFTER_SPEED);
}

static void course_maps(void)
{
    course_unpacked(COURSE_BACKDROP_MAP);
    course_map_rows(0);
    if (course_block_present(g.course, COURSE_BACKDROP_MAP_2)) {
        course_unpacked(COURSE_BACKDROP_MAP_2);
        course_map_rows(1);
    }
}

/* The course's art and palette; with two players the shared art of the split
   picture, else the frames, HUD art and the course map. */
static void race_screen_layers(void);

/* The two-player screen's art: the course's backdrop tiles and its four map
   pages, the frame's tiles and BG1's map. (The original also copies the frame's
   tiles to $3680, for the BG1 base it flips to; this screen has one.) */
static void two_player_art(void)
{
    course_unpacked(COURSE_BACKDROP_ART);
    race_art_write(RACE_ART_TWO_BACKDROP + 0x0C0, fx_ram + FX_UNPACK_AREA, 0x1800);
    course_unpacked(COURSE_BACKDROP_MAP);
    race_backdrop_two_pages_write(0, (const uint16_t *)(const void *)(fx_ram + FX_UNPACK_AREA), 4);
    unpacked(VIDEO_BLOCK_SPLIT_FRAME_ART);
    race_frame_two_art_write(RACE_FRAME_TWO_FRAME, fx_ram + FX_UNPACK_AREA, 0x600);
    unpacked(VIDEO_BLOCK_SPLIT_FRAME_MAP);
    race_frame_map_two_write((const uint16_t *)(const void *)(fx_ram + FX_UNPACK_AREA));
}

void RaceVideoSetup(void)
{
    VideoRegsReset();
    g.fx_screen_mode = 3;
    clear_all();
    SoundIndicesFromHandlers();
    race_art_write(RACE_ART_SPRITES, fx_ram + FX_UNPACK_AREA, unpacked(VIDEO_BLOCK_SPRITE_ART));
    PaletteCourseLoad(g.course);
    CgramDmaAll();
    GradientBuildColours(g.course);
    if (g.split_screen) {
        two_player_art();
        /* $7F:9BD7-$BBD7 cleared: what the results and the quit menu put
           in a half's 3D place (split_picture) is this, a blank picture */
        memset((uint8_t *)bank7f_words + 0x800, 0, 0x2001);
        unpacked(VIDEO_BLOCK_SPLIT_PANEL_MAP);
        race_panel_map_write((const uint16_t *)(const void *)(fx_ram + FX_UNPACK_AREA));
        memcpy(backdrop_map(0), fx_ram + FX_UNPACK_AREA, 0x800);
        unpacked(VIDEO_BLOCK_SPLIT_SPRITE_ART);
        race_art_write(RACE_ART_SPRITES + 0x1C0, fx_ram + FX_UNPACK_AREA, 0x800);
        race_art_write(RACE_ART_SPRITES + 0x060, fx_ram + FX_UNPACK_AREA + 0x600, 0x400);
        race_screen_layers();
        return;
    }
    /* the course's backdrop art: BG2's characters $C0 on (with one player
       BG2's start is the sprites') */
    race_art_write(RACE_ART_SPRITES + 0x0C0, fx_ram + FX_UNPACK_AREA, course_unpacked(COURSE_BACKDROP_ART));
    if (course_block_present(g.course, COURSE_BACKDROP_ART_2))
        race_art_write(RACE_ART_SPRITES + 0x140, fx_ram + FX_UNPACK_AREA, course_unpacked(COURSE_BACKDROP_ART_2));
    race_art_write(RACE_ART_SPRITES + 0x040, fx_ram + FX_UNPACK_AREA, unpacked(VIDEO_BLOCK_SHARED_ART));
    unpacked(VIDEO_BLOCK_COURSE_PICTURES);
    race_course_picture_write();
    switch (g.hud_mode) {
    case 1:
        if (!g.hud_blank) {
            race_frame_art_write(FRAME_HUD, fx_ram + FX_UNPACK_AREA, unpacked(VIDEO_BLOCK_HUD_FRAME_STUNT));
            unpacked(VIDEO_BLOCK_HUD_PIECES);
            race_frame_art_write(FRAME_HUD_PIECES, fx_ram + FX_UNPACK_AREA, 0x180);
            unpacked(VIDEO_BLOCK_AFTER_STUNT);
            break;
        }
        /* fall through */
    case 3:
        if (!g.race_skipped) {
            race_frame_art_write(FRAME_HUD, fx_ram + FX_UNPACK_AREA, unpacked(VIDEO_BLOCK_HUD_FRAME_STUNT));
            unpacked(VIDEO_BLOCK_HUD_PIECES);
            race_frame_art_write(FRAME_HUD_PIECES, fx_ram + FX_UNPACK_AREA + 0x180, 0x100);
            unpacked(VIDEO_BLOCK_AFTER_ALL);
            break;
        }
        /* fall through */
    case 0:
        hud_art_mode0();
        break;
    default:
        break;
    }
    screen_frame_and_pictures();
    course_maps();
    race_screen_layers();
}

void RaceVideoInit(void)
{
    if (g.every_course_open >= 3) {
        memset(g.race_time, 0, sizeof g.race_time);
        g.time_shown = 0;
        g.time_shown_2 = 0;
    }
    RaceVideoSetup();
}

/* ---- the race's backdrop ---- */

/* The player's vehicle is flagged in the air: bit 14 of the mode word of the
   camera its second link names. */
static uint16_t player_vehicle_raised(int player)
{
    const Object *car = obj_at(player ? g.player2_car : g.player_car);
    const Object *next = obj_at(car->link);

    return (camera_of(next->link)->mode & 0x4000) ? 8 : 0;
}

/* two-player: this half's backdrop scroll and gradient position from the view */
uint16_t BackdropScrollFromView(void)
{
    int half = g.fx_buffer;
    uint16_t h, t;
    unsigned s;

    g.backdrop_x[half] = (uint16_t)(g.backdrop_heading >> 2);
    h = (uint16_t)(player_vehicle_raised(half) + g.backdrop_pitch + 0x18);
    s = (unsigned)(uint16_t)(h << 1) + 0xEF99 + (h >> 15);
    t = (uint16_t)s;
    if (t >= 0xF257)
        t = 0xF257;
    gradient.view_offset[half] = (uint16_t)(t - 0xEF57);
    h = (uint16_t)(h + backdrop_two_scroll_base(half));
    g.backdrop_y[half] = h;
    return h;
}

/* one-player: the 17 rows of the backdrop map the view looks at, written to this picture buffer's rows */
void BackdropRowsUpdate(void)
{
    int buf = g.fx_buffer, n;
    uint16_t h = g.backdrop_pitch, s;
    uint32_t x;
    const uint16_t *map = backdrop_map((g.picture_effects & 0x40) ? 1 : 0);
    uint16_t *out = backdrop_rows(buf);
    unsigned at = 0, row_step;

    BackdropPitchForCameraMode();
    s = (uint16_t)((uint16_t)(h << 1) + 0x42 + (h >> 15));
    gradient.view_offset[0] = s < 0x0300 ? s : 0x0300;
    g.backdrop_y[buf] = (uint16_t)((h & 7) - 0x20 + backdrop_one_scroll_base(buf));
    g.backdrop_x[buf] = (uint16_t)(((g.backdrop_heading >> 1) & 7) - 0x18);
    row_step = (unsigned)(((uint16_t)(h + g.backdrop_row_adjust) >> 3) & 0x3F) * 0x140u;
    x = ((g.backdrop_heading >> 3) & 0xFE) + row_step;
    {
        WideBand band;

        band.map = (g.picture_effects & 0x40) ? 1 : 0;
        band.heading = g.backdrop_heading;
        band.pitch = h;
        band.row_adjust = g.backdrop_row_adjust;
        video_copy(&wide_band[buf], &band, sizeof band);
    }
    for (n = 0; n < 17; n++) {
        if (x >= 0x2800) {
            while (at < 0x220)
                out[at++] = 0x20C0;
            return;
        }
        memcpy(out + at, &map[x / 2], 0x40);
        at += 32;
        x += 0x140;
    }
}

/* the backdrop rows for a buffer go to the one-player map's rows */
void race_backdrop_rows_upload(int buffer)
{
    uint16_t adjusted[0x220];
    int i;

    /* the one-player backdrop's tiles are those at $200 on in the shared set;
       its priority bit goes, because a draw list composes as priority 0 and
       the 3D has to stay in front of it (see race_screen_layers) */
    for (i = 0; i < 0x220; i++)
        adjusted[i] = (uint16_t)((backdrop_rows(buffer)[i] & 0xDC00) | (((backdrop_rows(buffer)[i] & 0x3FF) + 0x200) & 0x3FF));
    video_map_write(race_video.backdrop_one, 0, buffer ? 47 : 30, 32, 17, adjusted);
}

void BackdropAnglesFromView(void)
{
    uint16_t v = g.view_angles[0], w;
    int i;

    for (i = 0; i < 6; i++)
        v = (uint16_t)((v >> 1) | (v & 0x8000));
    v = (uint16_t)-v;
    if (!((uint16_t)(v - 0x00E8) & 0x8000))
        v = 0x00E8;
    if ((uint16_t)(v - 0xFFC8) & 0x8000)
        v = 0xFFC8;
    g.backdrop_pitch = (uint16_t)((v + 0x0098) & 0x01FF);
    w = g.view_angles[1];
    for (i = 0; i < 5; i++)
        w = (uint16_t)((w >> 1) | (w & 0x8000));
    g.backdrop_heading = w & 0x07FF;
}

void Bg2ScrollFromTable(int half)
{
    pic.race.backdrop_x[half] = (int16_t)g.backdrop_x[half];
    pic.race.backdrop_y[half] = (int16_t)g.backdrop_y[half];
    pic.race.panel[half] = 0;
}

/* the picture that buffer holds becomes the one shown: its gradient, backdrop scroll and backdrop map */
void race_picture_shown(int buffer)
{
    if (g.split_screen)
        return;                         /* both pictures are always in its list: race_two_player_list */
    pic.bg[0].draw_list = fx_draw_picture(buffer);
    pic.race.gradient_start[0] = gradient.source[buffer];
    pic.race.backdrop_x[0] = (int16_t)g.backdrop_x[buffer];
    pic.race.backdrop_y[0] = (int16_t)g.backdrop_y[buffer];
    pic.bg[1].scroll_x = pic.race.backdrop_x[0];
    pic.bg[1].scroll_y = pic.race.backdrop_y[0];
    race_backdrop_rows_upload(buffer);
    video_copy(&shown_buffer, &buffer, sizeof shown_buffer);
}

/* The full window's hills and sky. The original placed both by the view's
   pitch, one row a pitch step (2 pi / 1024 radians, about 163 rows a
   radian, held past its ends), and showed 17 rows of the 32-row panorama,
   those the pitch looked at. The 3D's horizon is 128 tan of the angle
   (fx_draw_horizon), so the two parted as the view tilted, and in a tall
   window the panorama's top was cut off. Here the whole panorama is in
   the map, and both sit on the 3D's horizon.
   The day and night lists' entry at the horizon: the original's one-player
   band read entry pitch + $21 + line - 32, its two-player halves pitch +
   $18 + $49 + the rows from their middle; on a level view (pitch $98) the
   horizon is at the view's centre row, line 96 or the half's middle, and
   both read entry 249 there. The camera mode's +8 is in the pitch of both
   the sky and the hills, so it falls out. */
enum { SKY_AT_HORIZON = 249 };

/* the sky's place in the day and night lists, in bytes, for a pitch */
static uint16_t sky_offset_of(uint16_t pitch)
{
    uint16_t s = (uint16_t)((uint16_t)(pitch << 1) + 0x42 + (pitch >> 15));

    return s < 0x0300 ? s : 0x0300;
}

/* The pitch of the picture being drawn: its step's, or an in-between
   picture's. */
static uint16_t full_pitch_now(void)
{
    uint16_t heading[2], pitch[2];

    if (fx_tween_backdrop_angles(heading, pitch))
        return pitch[0];
    return wide_band[shown_buffer].pitch;
}

/* The 3D's horizon on the one-player full window, in screen rows, for the
   picture shown: its row at the screen's middle (the view's centre across),
   rounded. 0 while its view isn't known (nothing drawn yet, or a view
   straight up or down): the original's places then. */
static int full_horizon_row(int *row)
{
    float x, r, slope;
    int picture = fx_draw_shown_picture();

    if (picture < 0 || !fx_draw_horizon(picture, &x, &r, &slope))
        return 0;
    if (r < -65536.0f)
        r = -65536.0f;
    if (r > 65536.0f)
        r = 65536.0f;
    *row = (int)(r >= 0.0f ? r + 0.5f : r - 0.5f);
    return 1;
}

/* The whole panorama (32 rows, its round of 128 entries of each 160) as
   the full window's map, panorama row n at map row WIDE_DROP + n, sky
   elsewhere; written when the panorama shown differs from the one in it. */
static void hills_map_write(int which)
{
    static uint16_t map[WIDE_COLUMNS * WIDE_ROWS], in_map[32 * WIDE_COLUMNS], now[32 * WIDE_COLUMNS];
    static int written;
    const uint16_t *panorama = backdrop_map(which);
    int r, c;

    for (r = 0; r < 32; r++)
        memcpy(now + r * WIDE_COLUMNS, panorama + r * 160, WIDE_COLUMNS * sizeof now[0]);
    if (written && !memcmp(now, in_map, sizeof now))
        return;
    for (r = 0; r < WIDE_ROWS; r++) {
        int from = r - WIDE_DROP;

        for (c = 0; c < WIDE_COLUMNS; c++) {
            uint16_t w = from >= 0 && from < 32 ? now[from * WIDE_COLUMNS + c] : SKY_TILE;

            map[r * WIDE_COLUMNS + c] = (uint16_t)((w & 0xDC00) | (((w & 0x3FF) + 0x200) & 0x3FF));
        }
    }
    nshvc_map_write_snes(race_video.backdrop_wide, 0, 0, WIDE_COLUMNS, WIDE_ROWS, map);
    memcpy(in_map, now, sizeof in_map);
    written = 1;
}

/* Across: the heading's column and fine scroll, as BackdropRowsUpdate
   works them out (Jeff: the hills keep the original's turn rate). Down:
   the original's place for the pitch (its scroll over its band's rows,
   the band at its buffer's place), moved by the rows the 3D's horizon is
   below the row the original put entry SKY_AT_HORIZON on. */
static void hills_from_angles(int buffer, uint16_t heading, uint16_t pitch)
{
    int row = (int)(((uint16_t)(pitch + wide_band[buffer].row_adjust) >> 3) & 0x3F);
    int band = buffer ? 47 : 30, horizon;

    hills_map_write(wide_band[buffer].map);
    pic.race.wide_x = (((heading >> 3) & 0xFE) * 4 + ((heading >> 1) & 7) - 0x18) & (WIDE_COLUMNS * 8 - 1);
    pic.race.wide_y = (int16_t)(uint16_t)((pitch & 7) - 0x20 + backdrop_one_scroll_base(buffer))
                      + (WIDE_DROP + row - band) * 8;
    if (full_horizon_row(&horizon))
        pic.race.wide_y -= horizon - (screen_shape()->origin_y + SKY_AT_HORIZON - 1 - (int)pitch);
}

/* The hills of the picture shown: its step's angles, or those of a picture
   drawn between steps. */
void race_hills_full(void)
{
    uint16_t heading[2], pitch[2];

    if (fx_tween_backdrop_angles(heading, pitch))
        hills_from_angles(shown_buffer, heading[0], pitch[0]);
    else
        hills_from_angles(shown_buffer, wide_band[shown_buffer].heading, wide_band[shown_buffer].pitch);
}

/* Colour 1 on a row of the one-player full window's screen, for the
   picture being drawn: the lists' entry at the row's height from the 3D's
   horizon; with no horizon known, at its pitch's place as the original's
   band read it; black while the tunnel's black list is up. */
uint16_t race_full_sky_colour(int row)
{
    int horizon;

    if (pic.race.gradient_start[0] >= GRADIENT_AT_BLACK)
        return 0;
    if (full_horizon_row(&horizon))
        return sky_colour_at(SKY_AT_HORIZON + row - horizon);
    return sky_colour_at(sky_offset_of(full_pitch_now()) / 2 + row - screen_shape()->origin_y - 32);
}

/* The sky outside on a row of the full window, while a tunnel blacks the
   lists: race_full_sky_colour's place in the lists, the colours untinted. */
uint16_t race_open_sky_colour(int row)
{
    int horizon;

    if (full_horizon_row(&horizon))
        return sky_colour_open(SKY_AT_HORIZON + row - horizon);
    return sky_colour_open(sky_offset_of(full_pitch_now()) / 2 + row - screen_shape()->origin_y - 32);
}

/* The race screen handed back: the next screen sets its own layers, and
   none of the race's are left behind it. */
void race_screen_end(void)
{
    int i;

    pic.race.on = 0;
    pic.race.split = 0;
    pic.race.course_screen = 0;
    fx_draw_set_palette_row(0);
    for (i = 0; i < 3; i++) {
        pic.bg[i].draw_list.id = 0;
        pic.bg[i].tiles.id = 0;
        pic.bg[i].map.id = 0;
    }
}

/* the half shows the finished player's panel: scrolls as the original set them */
void race_panel_show(int half, uint8_t sign_of_y, uint8_t y_low, uint8_t y_high_when_plus, uint8_t y_when_minus, uint8_t x_low)
{
    int y = (sign_of_y & 0x80) ? y_when_minus : (y_low | y_high_when_plus << 8);

    pic.race.backdrop_x[half] = (0x100 | x_low) & 0xFF;
    pic.race.backdrop_y[half] = y & 0xFF;
    pic.race.panel[half] = 1;
}

/* ---- frames of the tile-mapped screens ---- */

/* The status row the entries below change (the tiles' palette), in the
   course's map, as the screens above the race have it. */
static uint16_t *status_rows(void)
{
    return backdrop_map(0);
}

/* `count` entries from the attribute byte at `at` (odd: the high byte of an
   entry): each keeps its other bits, with `bits` ORed in where the palette
   field was cleared. */
void TilemapPaletteFill(unsigned at, int count, uint8_t bits)
{
    uint8_t *m = (uint8_t *)status_rows();

    while (count--) {
        m[at] = (uint8_t)((m[at] & 0xE3) | bits);
        at += 2;
    }
}

static void palette_fill(unsigned at, int count, uint8_t bits)
{
    TilemapPaletteFill(at, count, bits);
}

/* Every fourth frame the phase steps 0-5 and its palette goes into the entries. */
void TilemapPaletteCycle(unsigned at, int count)
{
    if (g.frame_count & 3)
        return;
    {
        uint8_t phase = (uint8_t)(palettes.tile_cycle + 1), pal;

        if (phase >= 6)
            phase = 0;
        palettes.tile_cycle = phase;
        pal = results_row_cycle_bits(g.hud_mode == 2, phase);
        palette_fill(at, count, pal);
    }
}

/* The quit menu's three rows: the one under the cursor cycling its colours,
   the others plain. */
/* @routine 07:C293 TilemapPaletteCycle87 */
void QuitMenuRow1Cycle(void) { TilemapPaletteCycle(0x87, 9); }
/* @routine 07:C29F TilemapPaletteCycle107 */
void QuitMenuRow2Cycle(void) { TilemapPaletteCycle(0x107, 13); }
/* @routine 07:C2AB TilemapPaletteCycle187 */
void QuitMenuRow3Cycle(void) { TilemapPaletteCycle(0x187, 10); }

static void row_mark(unsigned at, int count, uint8_t bits, unsigned mark, uint8_t mark_bits)
{
    uint8_t *m = (uint8_t *)status_rows();

    palette_fill(at, count, bits);
    m[mark] = (uint8_t)((m[mark] & 0xE3) | mark_bits);
}

/* @routine 07:C2B7 TilemapRow87Palette5 */
void QuitMenuRow1Plain(void) { row_mark(0x87, 9, 0x14, 0x8D, 0x10); }
/* @routine 07:C2D3 TilemapRow107Palette5 */
void QuitMenuRow2Plain(void) { row_mark(0x107, 13, 0x14, 0x113, 0x10); }
/* @routine 07:C2EF TilemapRow187Palette5 */
void QuitMenuRow3Plain(void) { row_mark(0x187, 10, 0x14, 0x193, 0x10); }
/* The menu after the race: its retry, course and car rows plain. */
/* @routine 07:CE97 TilemapRow187Palette4 */
void ResultsMenuRetryPlain(void) { row_mark(0x187, 9, 0x10, 0x18D, 0x00); }
/* @routine 07:CEAF TilemapRow207Palette4 */
void ResultsMenuCoursePlain(void) { row_mark(0x207, 13, 0x10, 0x213, 0x00); }
/* @routine 07:CEC7 TilemapRow287Palette4 */
void ResultsMenuCarPlain(void) { row_mark(0x287, 10, 0x10, 0x293, 0x00); }

/* Twelve 4-row columns filled to a level, full rows from the ROM, empty ones from the second map. */
void TilemapLevelColumns(void)
{
    int i, k;

    for (i = 0; i < 12; i++) {
        uint16_t x = level_column_place(i), y;
        uint8_t n = g.level_columns[i];

        if (n > 4)
            n = 4;
        y = (uint16_t)((4 - n) * 8);
        for (k = 0; k < n; k++) {
            int c;

            for (c = 0; c < 4; c++)
                backdrop_map(0)[(x + 2 * c) / 2] = level_column_tile((uint16_t)(y + 2 * c));
            y = (uint16_t)(y + 8);
            x = (uint16_t)(x + 0x40);
        }
        for (k = n; k < 4; k++) {
            int c;

            for (c = 0; c < 4; c++)
                backdrop_map(0)[(x + 2 * c) / 2] = backdrop_map(1)[(x + 2 * c) / 2];
            x = (uint16_t)(x + 0x40);
        }
    }
}

/* @routine 07:CFA3 DigitTiles1184
   The car's three settings ($1184) as digit characters in the panel's map,
   row 2 from column 21. */
void CarSettingsDigits(void)
{
    int i;

    for (i = 0; i < 3; i++)
        backdrop_map(0)[(0x0AA + i * 2) / 2] = (uint16_t)(0x20CA + g.car_settings[i]);
}

void TimesKeepAndAdvance(void)
{
    int i;

    for (i = 0; i < 3; i++)
        g.race_time_best[i] = g.race_time_kept[i];
    g.banner_timer[0] = 0;
    g.results_step++;
    g.split_stage = 2;
    windows.bands_kind = 0;
    windows.bands_state = 2;
}

/* The static's random tiles: the static itself is drawn with the chip's
   picture, so only the random numbers it took are kept in step. */
static void image_quad_scaled(nshvc_draw_list list, nshvc_image image, float x, float y, float w, float h,
                              int u0, int v0, int uw, int vh, int hflip, int vflip);

/* The course screen's BG1 map ($0000): each cell one of the static's 32
   characters ($20 + n), at random. */
static uint8_t course_static_cells[32 * 32];

void VramNoiseTiles(void)
{
    uint8_t cells[sizeof course_static_cells];
    int i;

    for (i = 0; i < 0x400; i++) {
        RandomWordStep();
        cells[i] = (uint8_t)(g.random_word & 0x1F);
    }
    video_copy(course_static_cells, cells, sizeof cells);   /* the random numbers now; the static shown with the update's art */
}

/* The course screen fills the window once the static can be laid out
   (fx_draw's noise texture and the static's seeds). */
int race_course_full_window(void)
{
    return pic.race.on && pic.race.course_screen && fx_noise_image().id != 0;
}

/*
 * The course screen's backdrop: BG1's map over the static, drawn over the
 * whole screen. A cell showing character $20 + k shows the block's 8 x 8 at
 * column 8 (k / 16), line 8 (k % 16) (the chip's 128-line layout); each of
 * its lines is that block line's strip of the noise texture, from its seed
 * (across: the seed's low byte and the cell's column; down: its high byte).
 * A screen row shows the map's row + 1, as the hardware counted; on the
 * full window's screen the original's picture is at the origin and the map
 * wraps around it.
 */
static void course_static_draw(nshvc_draw_list list)
{
    const ScreenShape *sh = screen_shape();
    nshvc_image noise = fx_noise_image();
    int full = race_course_full_window(), w = full ? sh->width : 256, h = full ? sh->height : 224;
    int ox = full ? sh->origin_x : 0, oy = full ? sh->origin_y : 0, y, x;

    if (!noise.id)
        return;
    for (y = 0; y < h; y++) {
        int my = (y - oy + 1) & 255, cell_y = my >> 3, j = my & 7;

        for (x = -((-ox) & 7); x < w; x += 8) {
            int mx = (x - ox) & 255, k = course_static_cells[cell_y * 32 + (mx >> 3)];
            uint16_t seed = fx_static_seed(8 * (k % 16) + j);
            int u = ((seed & 0xFF) + 8 * (k / 16)) & 255, first = 256 - u < 8 ? 256 - u : 8;

            image_quad_scaled(list, noise, (float)x, (float)y, (float)first, 1.0f, u, seed >> 8, first, 1, 0, 0);
            if (first < 8)
                image_quad_scaled(list, noise, (float)(x + first), (float)y, (float)(8 - first), 1.0f, 0, seed >> 8,
                                  8 - first, 1, 0, 0);
        }
    }
}

/* ---- what the screen's lines take ---- */

/* The fixed colour from three bytes each naming its channels (bit 7 blue, 6 green, 5 red). */
static uint16_t fixed_from(const uint8_t *c)
{
    int i, r = 0, gc = 0, b = 0;

    for (i = 0; i < 3; i++) {
        if (c[i] & 0x80)
            b = c[i] & 31;
        if (c[i] & 0x40)
            gc = c[i] & 31;
        if (c[i] & 0x20)
            r = c[i] & 31;
    }
    return (uint16_t)(r | gc << 5 | b << 10);
}

/* The layers of a race screen: the 3D picture and the frame around it are one
   draw list on BG1, the backdrop on BG2. The engine composes a draw list as
   priority 0, so both screens use mode 1 with the backdrop at priority 0:
   the order is then the original's, sprites of priority 3 over the picture,
   the backdrop behind it and over sprites of priority 0 and 1. (The original's
   one-player screen is mode 3; in mode 3 the backdrop could not sit between
   the draw list and those sprites. The results' page, with BG1 as tiles and
   no list, shows in mode 3: results_bg1_send in picture.c.) */
static void race_screen_layers(void)
{
    pic.race.on = 1;
    pic.race.course_screen = 0;
    pic.race.split = g.split_screen != 0;
    pic.bg3_priority = 0;
    pic.mode = 1;
    /* the two-player pictures are 4-bit; the original's map shows their
       tiles in palette row 2 (entries $08xx at $1C00); one player's are 8-bit */
    fx_draw_set_palette_row(pic.race.split ? 2 : 0);
    if (pic.race.split) {
        /* the frame at its own priority on BG1, tiles; the 3D, backdrop and
           panel in the list, in BG2's place under it (race_frame_art_draw) */
        pic.bg[0].tiles = race_video.frame_tiles_two;
        pic.bg[0].map = race_video.frame_map_two;
        pic.bg[0].draw_list.id = 0;
        pic.bg[1].tiles.id = 0;
        pic.bg[1].map.id = 0;
        pic.bg[1].draw_list = fx_draw_picture(0);
    } else {
        pic.bg[0].tiles.id = 0;
        pic.bg[0].map.id = 0;
        pic.bg[0].draw_list = fx_draw_picture(0);
        pic.bg[1].tiles = race_video.tiles;
        pic.bg[1].map = race_video.backdrop_one;
        pic.bg[1].draw_list.id = 0;
    }
    pic.bg[2].tiles.id = 0;
    pic.bg[2].map.id = 0;
    pic.bg[2].draw_list.id = 0;        /* the menus' 3D layer off: left on, the engine filled the race's list twice */
    pic.line_tables = 0;
    pic.main_screen = SHOW_BG1 | SHOW_BG2 | SHOW_SPRITES;
    pic.sub_screen = 0;
    sprites_use_tiles(race_video.tiles, 0x200, 0x300);
    GradientSetup();
}

void race_screen_start(void)
{
    race_screen_layers();
}

/* What the one-player screen's blank lines before each picture did: the
   sprites straight to the screen, the cycling colours, the main screen, the
   fixed colour and the window setup. */
void RaceVideoUpload1P(void)
{
    OamDirectStage();
    RasterColoursUnlessSplitStages();
    pic.main_screen = SHOW_BG1 | SHOW_BG2 | SHOW_SPRITES;
    pic.blend.fixed_bgr15 = fixed_from(windows.colours);
    WindowsSetupSelect();
}

/* The one-player screen's window work, run while the picture before was shown. */
void RaceVideoWindows1P(void)
{
    WindowHdmaNormal();
    WindowLayoutStepForScreen();
    WindowSlideOrOscillate();
    WindowSweepIfOn();
    WindowSlideCheck();
    BobWindowsRun();
    WindowBandsAnimate();
    WindowSplitAnimate();
}

/* The two-player screen's work before the picture: the sprites and colours,
   the windows, each half's fixed colour and the top half's backdrop. */
void RaceVideoUpload2P(void)
{
    OamDmaAll();
    WindowLayoutStepForScreen();
    RasterPaletteUpload();
    WindowHdmaBands();
    WindowLayoutStepForScreen();
    BobWindowsRun();
    WindowBandsAnimate();
    if (g.panel_top) {
        pic.race.panel[0] = 1;
        pic.race.backdrop_x[0] = 0xF0;
        pic.race.backdrop_y[0] = (g.panel_top & 0x80) ? 0x68 : 0xE8;
    } else {
        Bg2ScrollFromTable(0);
    }
    pic.main_screen = SHOW_BG1 | SHOW_BG2 | SHOW_SPRITES;
    pic.blend.fixed_bgr15 = fixed_from(windows.colours);
    WindowsSetupSelect();
    GradientStartsFromViews();
}

/* The two-player screen's work at the split: the lower half's backdrop or
   panel, fixed colour and colours as they then stand. */
void RaceVideoSplit2P(void)
{
    if (g.panel_bottom) {
        pic.race.panel[1] = 1;
        pic.race.backdrop_x[1] = 0xA0;
        pic.race.backdrop_y[1] = (g.panel_bottom & 0x80) ? 0x00 : 0x80;
    } else {
        Bg2ScrollFromTable(1);
    }
    pic.race.fixed[1] = fixed_from(windows.colours + 3);
    pic.race.colours_kept[1][0] = palettes.buffer[0xE1];
    pic.race.colours_kept[1][1] = palettes.buffer[0xE2];
    pic.race.colours_kept[1][2] = palettes.buffer[0xE3];
    pic.race.colour_0[1] = palettes.buffer[0];
}

/* ---- the course screen before a race ---- */

uint16_t *race_map_buffer(void)
{
    return backdrop_map(0);
}

/* @routine 7E:B24C TilemapRowsTo5C00
   The results' and championship's BG2 map: 17 rows of 26 entries copied
   from the map buffer ($7F:93D7, rows $40 bytes apart) to the map at $5C00,
   as they stand now; then BG2's scroll from the first half. $5C00 is the
   second page of the one-player backdrop's map, so the rows go there, with
   its tile numbering and priority (race_backdrop_rows_upload). */
void ResultsMapRowsUpload(void)
{
    uint16_t row[26];
    int r, i;

    for (r = 0; r < 17; r++) {
        for (i = 0; i < 26; i++) {
            uint16_t w = backdrop_map(0)[r * 32 + i];

            row[i] = (uint16_t)((w & 0xFC00) | (((w & 0x3FF) + 0x200) & 0x3FF));   /* (its priority bit kept: the text over the car, as the original) */
        }
        video_map_write(race_video.backdrop_one, 0, (uint32_t)(32 + r), 26, 1, row);
    }
    Bg2ScrollFromTable(0);
}

/* The unpacked course picture ($1000 bytes at the chip RAM's $3000) kept as
   both maps, $7F:93D7 and $7F:BBD7. */
void race_course_maps_keep(void)
{
    memcpy(backdrop_map(0), fx_ram + FX_UNPACK_AREA, 0x1000);
    memcpy(backdrop_map(1), fx_ram + FX_UNPACK_AREA, 0x1000);
}

/* The preview's half of the first map put back from the second: $800 bytes
   from $7F:C3D7 to $7F:9BD7. */
void race_course_preview_reset(void)
{
    memcpy(backdrop_map(0) + 0x400, backdrop_map(1) + 0x400, 0x800);
}

/* The per-line BG2 scroll table's values, from the two scrolls; a table of
   three-byte entries (line count, scroll low, high). */
static void table_entry_value(int entry, uint16_t v)
{
    g.course_scroll_table[entry * 3 + 1] = (uint8_t)v;
    g.course_scroll_table[entry * 3 + 2] = (uint8_t)(v >> 8);
}

void ScrollHdmaSet(void)
{
    table_entry_value(1, g.course_scroll_low);
    table_entry_value(4, g.course_scroll_low);
    table_entry_value(2, 0x0130);
    table_entry_value(3, 0x0130);
}

/* The screen with static on BG1 and the course's art on BG2. */
void RaceScreenSetup(void)
{
    static const uint8_t ones[10] = {0x01, 0x04, 0x06, 0x09, 0x0C, 0x15, 0x18, 0x1B, 0x1E, 0x1F};
    static const uint8_t zeros[4] = {0x02, 0x05, 0x20, 0x21};
    uint8_t *t = g.course_scroll_table;
    unsigned i;

    g.fx_buffer = 0;
    for (i = 0; i < 10; i++)
        windows.wipe_table[ones[i]] = 1;
    for (i = 0; i < 4; i++)
        windows.wipe_table[zeros[i]] = 0;
    memset(&windows.maths, 0, sizeof windows.maths);
    memset(&pic.windows, 0, sizeof pic.windows);
    pic.windows.layer_use[5] = NSHVC_WIN1 | NSHVC_WIN1_INVERT;
    for (i = 0; i < 3; i++)
        pic.windows.mask_main[i] = 1;
    pic.blend.clip = 0;
    pic.blend.prevent = 2;
    pic.blend.use_fixed = 1;
    t[0] = 0x38;
    table_entry_value(0, g.course_scroll_top);
    t[3] = 0x4F;
    t[6] = t[9] = t[12] = 1;
    t[15] = 0;
    for (i = 1; i <= 4; i++)
        table_entry_value((int)i, g.course_scroll_low);
    t[6] = t[9] = t[12] = 1;
    line_effects_set(0, 1, 0, 0, 0, 0);     /* $02: the wipe */
    g.cursor_column_x = 0xF0;
    pic.race.on = 1;
    pic.race.split = 0;
    pic.race.course_screen = 1;
    pic.mode = 3;
    pic.bg3_priority = 0;
    pic.bg[0].draw_list = fx_draw_picture(0);
    pic.bg[0].scroll_x = pic.bg[0].scroll_y = 0;
    pic.bg[1].draw_list.id = 0;
    pic.bg[1].tiles = race_video.course_tiles;
    pic.bg[1].map = race_video.course_map;
    pic.bg[1].scroll_x = pic.bg[1].scroll_y = 0;
    pic.bg[2].tiles.id = 0;
    pic.bg[2].map.id = 0;
    pic.bg[2].draw_list.id = 0;        /* the menus' 3D layer off: left on, the engine filled the race's list twice */
    pic.main_screen = SHOW_BG2 | SHOW_BG1;
    pic.sub_screen = 0;
    pic.palette[1] = 0;
    sprites_use_tiles(race_video.tiles, 0x200, 0x300);
}

/* What the blank lines before each picture of the course screen set: the
   sprites, the colour maths, the fixed colour, the map, the wipe. */
void RaceCourseUpload(void)
{
    uint8_t level = windows.fixed_level & 31;

    OamDmaAll();
    pic.blend.subtract = windows.maths.subtract;
    pic.blend.half = windows.maths.half;
    pic.blend.layers = windows.maths.layers;
    pic.blend.fixed_bgr15 = (uint16_t)(level | level << 5 | level << 10);
    pic.bg[1].scroll_y = g.course_scroll_top;
    video_map_write(race_video.course_map, 0, 0, 32, 64, backdrop_map(0));
    CourseWipeTable();
}

/* ---- what the race loads before its first frame ---- */

uint8_t race_pictures[0x3000];          /* $7F:63D7: the three art pictures the results screens switch between */

/* One character of the unpacked HUD picture to the sprites' character `to`. */
static void hud_tile(uint16_t from, uint16_t tile, unsigned to)
{
    race_art_write(RACE_ART_SPRITES + to, fx_ram + (uint16_t)(from + (uint16_t)(tile << 5)), 32);
}

/* The race's sound and HUD art: tiles of the unpacked HUD picture, its big picture for the results, the race time, then the HUD's setup. */
void RaceHudLoad(void)
{
    SoundReset();
    if (g.time_override && g.hud_mode == 0)
        g.car[0] = 4;
    else
        SoundIndicesFromHandlers();
    CourseSoundLoad();
    unpacked(VIDEO_BLOCK_HUD_PICTURE);
    switch (g.hud_mode) {
    case 0:
        hud_tile(FX_UNPACK_AREA + 0xA0, g.selection_derived[1], 0x2F);
        hud_tile(FX_UNPACK_AREA + 0xA0, g.selection_derived[2], 0x3E);
        hud_tile(FX_UNPACK_AREA + 0xA0, g.selection_derived[3], 0x3F);
        /* fall through */
    case 1:
        if (g.hud_blank)
            race_art_write(RACE_ART_SPRITES + 0x3F, fx_ram + FX_UNPACK_AREA + 0x80, 0x20);
        break;
    case 2:
        hud_tile(FX_UNPACK_AREA, g.selection_derived[1], 0x2F);
        break;
    default:
        race_art_write(RACE_ART_SPRITES + 0x3F, fx_ram + FX_UNPACK_AREA + 0x80, 0x20);
        break;
    }
    hud_tile(FX_UNPACK_AREA, g.selection_derived[0], 0x2E);
    race_art_write(RACE_ART_SPRITES + 0x0A0, fx_ram + FX_UNPACK_AREA,
                   race_block_unpack(RACE_BLOCK_SPLIT_ART, fx_ram + FX_UNPACK_AREA, UNPACK_ROOM));
    unpacked(VIDEO_BLOCK_RESULTS_CARS);
    memcpy(race_pictures, fx_ram + FX_UNPACK_AREA, sizeof race_pictures);
    SoundReset();
    RaceTimeInit();
    fx16_set(FXIN_COURSE_MODELS, course_models_word(g.course));
    if (g.split_screen)
        RaceHudSetupTwoPlayers();
    else
        RaceHudSetup();
}

/* The stage with no picture to upload: the sprites out once a step when the
   count allows, then the colours. */
void NmiFrameUploadSprites(void)
{
    if (g.oam_upload_wanted) {
        g.oam_upload_wanted = 0;
        if (!g.start_light)
            OamDmaAll();
    }
    CgramUploadAll();
}

/* ---- the frame and panels around the 3D ---- */

/* One tile into an image of `w` x `h` at (x, y): colour 0 stays clear. */
static void plot_tile(uint8_t *image, int w, int h, int x, int y, const uint8_t *data, int bpp, int palette, int hflip, int vflip)
{
    int px, py, p;

    for (py = 0; py < 8; py++)
        for (px = 0; px < 8; px++) {
            int sy = vflip ? 7 - py : py, sx = hflip ? 7 - px : px, c = 0, ox = x + px, oy = y + py;

            for (p = 0; p < bpp; p++)
                c |= ((data[sy * 2 + (p / 2) * 16 + (p & 1)] >> (7 - sx)) & 1) << p;
            if (c && ox >= 0 && ox < w && oy >= 0 && oy < h)
                image[oy * w + ox] = (uint8_t)(bpp == 4 ? palette * 16 + c : c);
        }
}

/* The full window's panels: the one-player panel row's boxes (SPEED,
   DAMAGE, RANK, REST), each a joined patch of the frame's pixels in the
   row's lines that the row's own background does not reach from its edges;
   a patch that runs to the screen's side is the bezel's, not a box. Their
   pixels alone go into the boxes' image, so a box drawn on its own shows
   none of the bezel around it. */
enum { PANEL_TOP = 161, PANEL_END = 208, PANEL_ROWS = PANEL_END - PANEL_TOP };
static PanelBoxes panel_boxes;
static nshvc_image boxes_image;

const PanelBoxes *race_panel_boxes(void)
{
    return &panel_boxes;
}

static void panel_boxes_find(const uint8_t *image)
{
    static const int step[4][2] = { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } };
    static uint8_t keep[PANEL_ROWS][256], out[PANEL_ROWS * 256];
    static int16_t stack[PANEL_ROWS * 256][2], part[PANEL_ROWS * 256][2];
    const uint8_t *px = image + PANEL_TOP * 256;
    uint8_t bg = px[(PANEL_ROWS / 2) * 256];
    int x, y, n = 0, k;

    if (!boxes_image.id)
        boxes_image = nshvc_image_create(256, PANEL_ROWS, NSHVC_IMAGE_INDEXED);
    for (y = 0; y < PANEL_ROWS; y++)
        for (x = 0; x < 256; x++)
            keep[y][x] = px[y * 256 + x] != 0;
    /* the background, from the row's edges */
    for (y = 0; y < PANEL_ROWS; y++)
        for (x = 0; x < 256; x++)
            if ((x == 0 || x == 255 || y == 0 || y == PANEL_ROWS - 1) && keep[y][x] && px[y * 256 + x] == bg) {
                keep[y][x] = 0;
                stack[n][0] = (int16_t)x;
                stack[n++][1] = (int16_t)y;
            }
    while (n) {
        n--;
        x = stack[n][0];
        y = stack[n][1];
        for (k = 0; k < 4; k++) {
            int a = x + step[k][0], b = y + step[k][1];

            if (a < 0 || a > 255 || b < 0 || b >= PANEL_ROWS || !keep[b][a] || px[b * 256 + a] != bg)
                continue;
            keep[b][a] = 0;
            stack[n][0] = (int16_t)a;
            stack[n++][1] = (int16_t)b;
        }
    }
    /* the patches left */
    memset(out, 0, sizeof out);
    panel_boxes.count = 0;
    for (y = 0; y < PANEL_ROWS; y++)
        for (x = 0; x < 256; x++) {
            int x0 = x, x1 = x, y0 = y, y1 = y, edge = 0, m = 0, q;

            if (keep[y][x] != 1)
                continue;
            keep[y][x] = 2;
            stack[0][0] = (int16_t)x;
            stack[0][1] = (int16_t)y;
            n = 1;
            while (n) {
                int a0, b0;

                n--;
                a0 = stack[n][0];
                b0 = stack[n][1];
                part[m][0] = (int16_t)a0;
                part[m++][1] = (int16_t)b0;
                x0 = a0 < x0 ? a0 : x0;
                x1 = a0 > x1 ? a0 : x1;
                y0 = b0 < y0 ? b0 : y0;
                y1 = b0 > y1 ? b0 : y1;
                edge |= a0 == 0 || a0 == 255;
                for (k = 0; k < 4; k++) {
                    int a = a0 + step[k][0], b = b0 + step[k][1];

                    if (a < 0 || a > 255 || b < 0 || b >= PANEL_ROWS || keep[b][a] != 1)
                        continue;
                    keep[b][a] = 2;
                    stack[n][0] = (int16_t)a;
                    stack[n++][1] = (int16_t)b;
                }
            }
            if (edge || panel_boxes.count == 16)
                continue;
            k = panel_boxes.count++;
            panel_boxes.x0[k] = x0;
            panel_boxes.x1[k] = x1;
            panel_boxes.y0[k] = PANEL_TOP + y0;
            panel_boxes.y1[k] = PANEL_TOP + y1;
            for (q = 0; q < m; q++)
                out[part[q][1] * 256 + part[q][0]] = px[part[q][1] * 256 + part[q][0]];
        }
    nshvc_image_write(boxes_image, out);
}

/* The one-player frame and panels: the art tiles from $2B0 on, the map
   scrolled so that its row 0 is at line 16. */
static void frame_image_build(void)
{
    static uint8_t image[224 * 256];
    int x, y;

    memset(image, 0, sizeof image);
    for (y = 0; y < 32; y++)
        for (x = 0; x < 32; x++) {
            uint16_t w = frame_words_1p[y * 32 + x];
            unsigned t = w & 0x3FF;

            if (t >= 0x2B0)
                plot_tile(image, 256, 224, x * 8, y * 8 + 16, frame_art_8 + t * 64u, 8, 0, (w >> 14) & 1, (w >> 15) & 1);
        }
    panel_boxes_find(image);
    if (race_ending_running()) {
        /* the ending shows the view with no panels (Jeff, 2026-11-14, from the original's
           ending): the boxes' pixels are taken out of the frame and none are drawn over the 3D */
        for (int k = 0; k < panel_boxes.count; k++)
            for (y = panel_boxes.y0[k]; y <= panel_boxes.y1[k]; y++)
                memset(image + y * 256 + panel_boxes.x0[k], 0, (size_t)(panel_boxes.x1[k] - panel_boxes.x0[k] + 1));
        panel_boxes.count = 0;
    }
    nshvc_image_write(race_video.frame_image, image);
    frame_image_stale = 0;
}

/* The two-player backdrop (four pages) and the panel, from the shared art;
   the panel's tile numbers count from its own art, $300 on. */
static nshvc_image backdrop_clear;

static void two_images_build(void)
{
    static uint8_t backdrop[512 * 512], panel[256 * 256];
    int page, x, y;

    memset(backdrop, 0, sizeof backdrop);
    memset(panel, 0, sizeof panel);
    for (page = 0; page < 4; page++)
        for (y = 0; y < 32; y++)
            for (x = 0; x < 32; x++) {
                uint16_t w = backdrop_two_words[page][y * 32 + x];

                plot_tile(backdrop, 512, 512, (page & 1) * 256 + x * 8, (page >> 1) * 256 + y * 8,
                          race_art_bytes + (w & 0x3FFu) * 32u, 4, (w >> 10) & 7, (w >> 14) & 1, (w >> 15) & 1);
            }
    for (y = 0; y < 32; y++)
        for (x = 0; x < 32; x++) {
            uint16_t w = panel_words[y * 32 + x];

            plot_tile(panel, 256, 256, x * 8, y * 8, race_art_bytes + (((w & 0x3FFu) + 0x300u) & 0x3FFu) * 32u, 4,
                      (w >> 10) & 7, (w >> 14) & 1, (w >> 15) & 1);
        }
    nshvc_image_write(race_video.backdrop_image, backdrop);
    nshvc_image_write(race_video.panel_image, panel);
    /* the full window's copy: the sky's colour 1 clear, each player's own
       sky rows behind it */
    if (!backdrop_clear.id)
        backdrop_clear = nshvc_image_create(512, 512, NSHVC_IMAGE_INDEXED);
    for (x = 0; x < 512 * 512; x++)
        if (backdrop[x] == 1)
            backdrop[x] = 0;
    nshvc_image_write(backdrop_clear, backdrop);
    two_images_stale = 0;
}

/* The art drawn over the 3D (race_frame_art_draw_over) goes at the list's
   nearest depth (its own depth, 1 nearest: fx_draw.c depth_list); the
   art drawn before the 3D at the farthest, 0, as all of it was before. */
static int art_over;

static void art_depth(nshvc_vertex *v)
{
    int k;

    for (k = 0; k < 6; k++)
        v[k].z = art_over ? 1.0f : 0.0f;
}

/* A rectangle of the screen from texels (u0, v0), a texel a screen pixel,
   in the list's own pixels (fx_draw_list_scale: the list may be at the
   output's resolution). The corners take the texels' edges, as
   nshvc_draw_image's do: a pixel's centre then falls inside its texel,
   never on the edge between two, where rounding could take the one beside
   it. */
static void image_quad(nshvc_draw_list list, nshvc_image image, int x0, int y0, int w, int h, int u0, int v0)
{
    nshvc_vertex v[6];
    nshvc_material m;
    float sx, sy, l, t, r, b;
    float ul = (float)u0, ur = (float)(u0 + w);
    float vt = (float)v0, vb = (float)(v0 + h);

    fx_draw_list_scale(&sx, &sy);
    l = (float)x0 * sx;
    r = (float)(x0 + w) * sx;
    t = (float)y0 * sy;
    b = (float)(y0 + h) * sy;
    memset(&m, 0, sizeof m);
    m.image = image;
    memset(v, 0, sizeof v);
    v[0].x = l; v[0].y = t; v[0].u = ul; v[0].v = vt; v[0].colour = 1;
    v[1].x = r; v[1].y = t; v[1].u = ur; v[1].v = vt; v[1].colour = 1;
    v[2].x = r; v[2].y = b; v[2].u = ur; v[2].v = vb; v[2].colour = 1;
    v[3] = v[0];
    v[4] = v[2];
    v[5].x = l; v[5].y = b; v[5].u = ul; v[5].v = vb; v[5].colour = 1;
    art_depth(v);
    nshvc_draw_tris(list, v, 6, &m);
}

/* One half's backdrop or panel, wrapping where its image ends: a pixel at
   line r shows the image's row r + 1 + scroll, as the hardware counted. */
static void half_draw(nshvc_draw_list list, int half, int first, int last)
{
    nshvc_image image = pic.race.panel[half] ? race_video.panel_image : race_video.backdrop_image;
    int size = pic.race.panel[half] ? 256 : 512, mask = size - 1, r = first;
    int sx = pic.race.backdrop_x[half], sy = pic.race.backdrop_y[half];

    while (r < last) {
        int v = (r + 1 + sy) & mask, n = size - v, x = 0;

        if (n > last - r)
            n = last - r;
        while (x < 256) {
            int u = (x + sx) & mask, w = size - u;

            if (w > 256 - x)
                w = 256 - x;
            image_quad(list, image, x, r, w, n, u, v);
            x += w;
        }
        r += n;
    }
}

/* ---- two players in the full window ---- */

/* A colour of the palette's 15 bits as the list's direct colour, each
   channel widened as the engine widens a palette colour's. */
static uint32_t direct_of(uint16_t bgr)
{
    uint32_t r = bgr & 31, gc = (bgr >> 5) & 31, b = (bgr >> 10) & 31;

    r = r << 3 | r >> 2;
    gc = gc << 3 | gc >> 2;
    b = b << 3 | b >> 2;
    return 0xFF000000u | r << 16 | gc << 8 | b;
}

/* A rectangle of one colour, in screen pixels, into the list. */
static void colour_rect(nshvc_draw_list list, int x0, int y0, int x1, int y1, uint32_t colour)
{
    nshvc_vertex v[6];
    float sx, sy;

    fx_draw_list_scale(&sx, &sy);
    memset(v, 0, sizeof v);
    v[0].x = (float)x0 * sx; v[0].y = (float)y0 * sy;
    v[1].x = (float)x1 * sx; v[1].y = (float)y0 * sy;
    v[2].x = (float)x1 * sx; v[2].y = (float)y1 * sy;
    v[3] = v[0];
    v[4] = v[2];
    v[5].x = (float)x0 * sx; v[5].y = (float)y1 * sy;
    for (x0 = 0; x0 < 6; x0++)
        v[x0].colour = colour;
    art_depth(v);
    nshvc_draw_tris(list, v, 6, NULL);
}

/* An image of `size` x `size` texels wrapped over a rectangle of the screen:
   its top left shows texel (u, v). */
static void image_wrap(nshvc_draw_list list, nshvc_image image, int size, int x, int y, int w, int h, int u, int v)
{
    int mask = size - 1, r = 0;

    while (r < h) {
        int tv = (v + r) & mask, n = size - tv, c = 0;

        if (n > h - r)
            n = h - r;
        while (c < w) {
            int tu = (u + c) & mask, m = size - tu;

            if (m > w - c)
                m = w - c;
            image_quad(list, image, x + c, y + r, m, n, tu, tv);
            c += m;
        }
        r += n;
    }
}

/*
 * Each player's sky and hills in its rectangle, about its view's centre
 * (the rectangle's middle) as the original's half had them about its
 * view's (columns 88 and 168, lines 63 and 167): each row the day list's
 * colour at its height above that player's horizon, the hills (or the
 * finished player's panel) at that player's scroll, the image's row the
 * original line's + 1 + scroll as the hardware counted.
 */
/* Two players in the full window: colour 1 on screen row `row` for player
   p, the band read from its start by the row's height about the
   rectangle's middle, held at the lists' ends. */
/* Player p's 3D horizon on the two-player full window, in screen rows, as
   full_horizon_row gives the one player's. */
static int two_horizon_row(int p, int *row)
{
    float x, r, slope;

    if (!fx_draw_horizon(p & 1, &x, &r, &slope))
        return 0;
    if (r < -65536.0f)
        r = -65536.0f;
    if (r > 65536.0f)
        r = 65536.0f;
    *row = (int)(r >= 0.0f ? r + 0.5f : r - 0.5f);
    return 1;
}

/* Colour 1 on a row of player p's rectangle: the day list's entry at the
   row's height from that player's 3D horizon (SKY_AT_HORIZON); with none
   known, or the tunnel's black list up, its band's place as the original
   read it about the half's middle. */
uint16_t race_two_sky_colour(int p, int row)
{
    int x, y, w, h, at, horizon;

    screen_two_rect(p & 1, &x, &y, &w, &h);
    if (pic.race.gradient_start[p & 1] < GRADIENT_AT_NIGHT && two_horizon_row(p, &horizon))
        at = GRADIENT_AT_DAY + SKY_AT_HORIZON + row - horizon;
    else
        at = (int)pic.race.gradient_start[p & 1] + (row - (y + h / 2)) + 40;
    if (at < GRADIENT_AT_DAY)
        at = GRADIENT_AT_DAY;
    if (at > GRADIENT_AT_DAY + 511)
        at = GRADIENT_AT_DAY + 511;
    return gradient_colour((uint32_t)at);
}

/* Aqua Tunnel's underwater backdrop as the 3D's water takes it: its ripple
   cells, the course's backdrop map rows 51-60 at 160 entries a row from the
   first map (running on into the second, as the original's did), eight
   entries across, as 4-bit texels of the race art (BG2's characters from
   RACE_ART_SPRITES with one player, flips kept); and palette 0's colours
   from the course's other set (the weather's, $7F:39D7). 0 on any other
   course (those rows hold their own art) and while the one-player race's
   backdrop isn't loaded (two players load other maps). */
enum { AQUA_TUNNEL = 1 };

int race_underwater_cells(uint8_t texels[80][64], uint16_t colours[16])
{
    int x, y, any = 0;

    if (!pic.race.on || pic.race.split || pic.race.course_screen || g.course != AQUA_TUNNEL)
        return 0;
    for (y = 0; y < 80; y++)
        for (x = 0; x < 64; x++) {
            uint16_t e = bank7f_words[(51 + y / 8) * 160 + x / 8];
            unsigned ch = ((e & 0x3FFu) + RACE_ART_SPRITES) & 0x3FFu, p;
            int px = (e & 0x4000) ? 7 - (x & 7) : (x & 7), py = (e & 0x8000) ? 7 - (y & 7) : (y & 7);
            const uint8_t *d = race_art_bytes + ch * 32u;
            uint8_t c = 0;

            for (p = 0; p < 4; p++)
                c = (uint8_t)(c | ((d[py * 2 + (p / 2) * 16 + (p & 1)] >> (7 - px)) & 1) << p);
            texels[y][x] = c;
            any |= c;
        }
    for (x = 0; x < 16; x++)
        colours[x] = palettes.night[x];
    return any != 0;
}

static void two_full_backdrop(nshvc_draw_list list)
{
    static const int centre_x[2] = { 88, 168 }, centre_y[2] = { 63, 167 };
    const ScreenShape *sh = screen_shape();
    int p;

    /* the seam between the players black, as the original's blank lines between its halves */
    colour_rect(list, 0, 0, sh->width, sh->height, 0xFF000000u);
    for (p = 0; p < 2; p++) {
        int x, y, w, h, rcx, rcy, r, sx = pic.race.backdrop_x[p], sy = pic.race.backdrop_y[p], horizon;

        screen_two_rect(p, &x, &y, &w, &h);
        rcx = x + w / 2;
        rcy = y + h / 2;
        if (!pic.race.panel[p] && two_horizon_row(p, &horizon)) {     /* (the hills only: a panel stands where the game put it) */
            /* the hills down by the rows the 3D's horizon is below where the
               original put the half's sky entry SKY_AT_HORIZON: the half's
               middle + SKY_AT_HORIZON - $21 - 40 - its pitch (BackdropScrollFromView's
               h, its scroll less the half's table word) */
            int pitch = (int16_t)(uint16_t)(pic.race.backdrop_y[p] - backdrop_two_scroll_base(p));

            sy -= horizon - (rcy + SKY_AT_HORIZON - 0x21 - 40 - pitch);
        }
        if (pic.race.panel[p] && race_half_blank(p)) {
            /* the results' or quit menu's panel in a blanked half: the original's
               144 x 80 window of it, scaled whole into the rectangle, on colour 0
               (split_picture's); the art around that window is another page's */
            float bx, by, bw, bh, fx = (float)x, fy = (float)y, fw = (float)w, fh = (float)h, k;

            /* clear of the player's boxes: above their row, or left of their column */
            screen_two_box(p, 0, &bx, &by, &bw, &bh);
            if (screen_two_side())
                fh = by - fy;
            else
                fw = bx - fx;
            k = fw / 144.0f < fh / 80.0f ? fw / 144.0f : fh / 80.0f;
            {
                /* the window's diamond pattern carried out to the half's edges:
                   its top left 16 x 16 (the pattern only, a whole repeat) tiled
                   from the window's corner, cut at the rectangle */
                float wx = fx + (fw - 144.0f * k) / 2, wy = fy + (fh - 80.0f * k) / 2, t = 16.0f * k, tx, ty;
                int u0 = (centre_x[p] - 72 + sx) & 255, v0 = (centre_y[p] + 1 - 40 + sy) & 255;

                colour_rect(list, x, y, x + w, y + h, direct_of(pic.palette[0]));
                for (ty = wy - ceilf((wy - (float)y) / t) * t; ty < (float)(y + h); ty += t)
                    for (tx = wx - ceilf((wx - (float)x) / t) * t; tx < (float)(x + w); tx += t) {
                        float l = fmaxf(tx, (float)x), r = fminf(tx + t, (float)(x + w));
                        float tp = fmaxf(ty, (float)y), bt = fminf(ty + t, (float)(y + h));

                        if (r <= l || bt <= tp)
                            continue;
                        image_quad_scaled(list, race_video.panel_image, l, tp, r - l, bt - tp,
                                          u0 + (int)lroundf((l - tx) / k), v0 + (int)lroundf((tp - ty) / k),
                                          (int)lroundf((r - l) / k), (int)lroundf((bt - tp) / k), 0, 0);
                    }
                image_quad_scaled(list, race_video.panel_image, wx, wy, 144.0f * k, 80.0f * k, u0, v0, 144, 80, 0, 0);
            }
            continue;
        }
        for (r = 0; r < h; r++)
            colour_rect(list, x, y + r, x + w, y + r + 1, direct_of(race_two_sky_colour(p, y + r)));
        if (pic.race.panel[p])
            image_wrap(list, race_video.panel_image, 256, x, y, w, h,
                       x - rcx + centre_x[p] + sx, y - rcy + centre_y[p] + 1 + sy);
        else
            image_wrap(list, backdrop_clear, 512, x, y, w, h,
                       x - rcx + centre_x[p] + sx, y - rcy + centre_y[p] + 1 + sy);
    }
}

/* A rectangle of texels (u0, v0, uw x vh) over a rectangle of the screen
   at any scale, in the list's pixels; flipped across or down. */
static void image_quad_scaled(nshvc_draw_list list, nshvc_image image, float x, float y, float w, float h,
                              int u0, int v0, int uw, int vh, int hflip, int vflip)
{
    nshvc_vertex v[6];
    nshvc_material m;
    float sx, sy, l, t, r, b, ul = (float)u0, ur = (float)(u0 + uw), vt = (float)v0, vb = (float)(v0 + vh), k;

    fx_draw_list_scale(&sx, &sy);
    if (hflip) {
        k = ul;
        ul = ur;
        ur = k;
    }
    if (vflip) {
        k = vt;
        vt = vb;
        vb = k;
    }
    l = x * sx;
    r = (x + w) * sx;
    t = y * sy;
    b = (y + h) * sy;
    memset(&m, 0, sizeof m);
    m.image = image;
    memset(v, 0, sizeof v);
    v[0].x = l; v[0].y = t; v[0].u = ul; v[0].v = vt; v[0].colour = 1;
    v[1].x = r; v[1].y = t; v[1].u = ur; v[1].v = vt; v[1].colour = 1;
    v[2].x = r; v[2].y = b; v[2].u = ur; v[2].v = vb; v[2].colour = 1;
    v[3] = v[0];
    v[4] = v[2];
    v[5].x = l; v[5].y = b; v[5].u = ul; v[5].v = vb; v[5].colour = 1;
    art_depth(v);
    nshvc_draw_tris(list, v, 6, &m);
}

/*
 * Each player's boxes as one picture, 80 x 96 of the two-player frame from
 * (168, 16) and (8, 120): the frame drawn as its layer shows it (the map's
 * pixel at the layer's scroll + the line + 1), then the bezel's fill (its
 * character under the map, map row 7 column 21) cleared wherever it is
 * reached from outside the boxes' outlines and in the map's place; its
 * first and last rows are the bezel's rim (fullwindow.c's panels_two).
 */
static nshvc_image two_box_image[2];

static void two_boxes_build(void)
{
    static const int box_x[2] = { 168, 8 }, box_y[2] = { 16, 120 };
    static uint8_t frame[256 * 256], img[96 * 80], keep[96][80];
    static int16_t stack[96 * 80][2];
    int sx = pic.bg[0].scroll_x, sy = pic.bg[0].scroll_y, r, c, p;

    memset(frame, 0, sizeof frame);
    for (r = 0; r < 32; r++)
        for (c = 0; c < 32; c++) {
            uint16_t w = frame_two_words[r * 32 + c];
            unsigned t = w & 0x1FF;
            int x = (c * 8 - sx) & 255, y = (r * 8 - 1 - sy) & 255;

            if (!(w & 0x3FF))
                continue;
            plot_tile(frame, 256, 256, x, y, frame_two_bytes + t * 32u, 4, (w >> 10) & 7, (w >> 14) & 1, (w >> 15) & 1);
            if (x > 248)
                plot_tile(frame, 256, 256, x - 256, y, frame_two_bytes + t * 32u, 4, (w >> 10) & 7, (w >> 14) & 1, (w >> 15) & 1);
            if (y > 248)
                plot_tile(frame, 256, 256, x, y - 256, frame_two_bytes + t * 32u, 4, (w >> 10) & 7, (w >> 14) & 1, (w >> 15) & 1);
        }
    for (p = 0; p < 2; p++) {
        static const int step[4][2] = { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } };
        uint8_t bg = frame[(7 * 8 + 4) * 256 + 21 * 8 + 4];
        int n = 0, x, y, k;

        for (y = 0; y < 96; y++)
            for (x = 0; x < 80; x++) {
                uint8_t v = frame[(box_y[p] + y) * 256 + box_x[p] + x];

                img[y * 80 + x] = v;
                keep[y][x] = v != 0 && y && y != 95;
            }
        for (y = 0; y < 96; y++)
            for (x = 0; x < 80; x++)
                if ((!x || x == 79 || y == 1 || y == 94 || ((p ? x / 8 >= 6 : x / 8 < 4) && y / 8 >= 4 && y / 8 < 8)) &&
                    img[y * 80 + x] == bg && keep[y][x]) {
                    keep[y][x] = 0;
                    stack[n][0] = (int16_t)x;
                    stack[n++][1] = (int16_t)y;
                }
        while (n) {
            n--;
            x = stack[n][0];
            y = stack[n][1];
            for (k = 0; k < 4; k++) {
                int a = x + step[k][0], b = y + step[k][1];

                if (a < 0 || a >= 80 || b < 0 || b >= 96 || !keep[b][a] || img[b * 80 + a] != bg)
                    continue;
                keep[b][a] = 0;
                stack[n][0] = (int16_t)a;
                stack[n++][1] = (int16_t)b;
            }
        }
        for (y = 0; y < 96; y++)
            for (x = 0; x < 80; x++)
                if (!keep[y][x])
                    img[y * 80 + x] = 0;
        if (!two_box_image[p].id)
            two_box_image[p] = nshvc_image_create(80, 96, NSHVC_IMAGE_INDEXED);
        nshvc_image_write(two_box_image[p], img);
    }
    two_boxes_stale = 0;
}

/* The race's sprite art in palette `pal`, 128 x 256: the sprite named n
   (9 bits; 16 x 16 ones take n, n + 1, n + 16, n + 17) at column (n & 15)
   * 8, row (n >> 4) * 8, as the sprites' characters are laid out. */
static nshvc_image sprite_atlas_for(int pal)
{
    static uint8_t img[128 * 256];
    int n;

    pal &= 7;
    if (sprite_atlas_made[pal])
        return sprite_atlas[pal];
    memset(img, 0, sizeof img);
    for (n = 0; n < 512; n++) {
        unsigned tile = n < 256 ? 0x200u + (unsigned)n : 0x300u + (unsigned)(n - 256);

        plot_tile(img, 128, 256, (n & 15) * 8, (n >> 4) * 8, race_art_bytes + tile * 32u, 4, 8 + pal, 0, 0);
    }
    if (!sprite_atlas[pal].id)
        sprite_atlas[pal] = nshvc_image_create(128, 256, NSHVC_IMAGE_INDEXED);
    nshvc_image_write(sprite_atlas[pal], img);
    sprite_atlas_made[pal] = 1;
    return sprite_atlas[pal];
}

/*
 * Each player's four boxes over its 3D at their places and scale, and the
 * HUD's sprites inside them (digits, gauges, the map's dots) drawn with
 * them at the same scale, in the box their middle is in (the sprite layer
 * leaves these out: sprites_place_two).
 */
static void two_full_boxes(nshvc_draw_list list)
{
    static const int box_x[2] = { 168, 8 }, box_y[2] = { 16, 120 };
    int p, b, i;

    if (two_boxes_stale)
        two_boxes_build();
    for (p = 0; p < 2; p++) {
        float s = (float)screen_two_scale(p) / 256.0f;

        for (b = 0; b < 4; b++) {
            int sx, sy, sw, sh;
            float x, y, w, h;

            screen_two_box_source(p, b, &sx, &sy, &sw, &sh);
            screen_two_box(p, b, &x, &y, &w, &h);
            image_quad_scaled(list, two_box_image[p], x, y, w, h, sx, sy, sw, sh, 0, 0);
        }
        for (i = p ? 64 : 0; i < (p ? 128 : 64) && i < (int)pic.sprite_count; i++) {
            const nshvc_sprite *o = &pic.sprites[i];
            int size = o->width * 8, rx = o->x - box_x[p], ry = o->y - box_y[p], cx = rx + size / 2, cy = ry + size / 2;

            if (o->priority != 3 || o->y < 0 || o->y >= 224 || cx < 0 || cx >= 80 || cy < 0 || cy >= 96)
                continue;
            for (b = 0; b < 4; b++) {
                int sx, sy, sw, sh;
                float x, y, w, h;

                screen_two_box_source(p, b, &sx, &sy, &sw, &sh);
                if (cx < sx || cx >= sx + sw || cy < sy || cy >= sy + sh)
                    continue;
                screen_two_box(p, b, &x, &y, &w, &h);
                {
                    unsigned name = sprite_name_of(o);

                    image_quad_scaled(list, sprite_atlas_for(o->palette), x + (float)(rx - sx) * s, y + (float)(ry - sy) * s,
                                      (float)size * s, (float)size * s, (int)(name & 15) * 8, (int)(name >> 4) * 8,
                                      size, size, o->hflip, o->vflip);
                }
                break;
            }
        }
    }
}

/* What goes behind the 3D in its list: one player the frame and panels;
   two players each half's backdrop or panel. */
void race_frame_art_draw(nshvc_draw_list list)
{
    if (!pic.race.on || pic.race.course_screen)
        return;                         /* another screen's picture (the course menu's car) */
    if (g.split_screen) {
        if (two_images_stale)
            two_images_build();
        if (race_two_full_window()) {
            two_full_backdrop(list);
            return;
        }
        half_draw(list, 0, 0, 112);
        half_draw(list, 1, 112, 224);
    } else {
        if (frame_image_stale)
            frame_image_build();
        if (race_full_window())
            return;                     /* no bezel: the 3D fills the screen, the boxes go over it */
        /* as triangles at the farthest depth (art_depth), behind the 3D: an
           image drawn in a list goes at its nearest (nativeshvc 47a4777) */
        image_quad(list, race_video.frame_image, 0, 0, 256, 224, 0, 0);
    }
}

/* What goes over the 3D in its list: in the full window, the panel row's
   boxes, each at the bottom by its own anchor. */
static void art_over_draw(nshvc_draw_list list)
{
    int k, down;

    if (pic.race.on && pic.race.course_screen) {
        course_static_draw(list);       /* over the static's own block, which it lays out */
        return;
    }
    if (race_two_full_window()) {
        two_full_boxes(list);
        return;
    }
    if (!race_full_window())
        return;
    if (frame_image_stale)
        frame_image_build();
    down = screen_bottom_shift();
    for (k = 0; k < panel_boxes.count; k++) {
        int x0 = panel_boxes.x0[k], y0 = panel_boxes.y0[k];

        image_quad(list, boxes_image, x0 + screen_anchor_x(x0, panel_boxes.x1[k]), y0 + down,
                   panel_boxes.x1[k] - x0 + 1, panel_boxes.y1[k] - y0 + 1, x0, y0 - PANEL_TOP);
    }
}

void race_frame_art_draw_over(nshvc_draw_list list)
{
    art_over = 1;
    art_over_draw(list);
    art_over = 0;
}

/* The two-player list, composed again when a half's scroll or panel has moved
   (asking for the other picture number makes the compose run; both are in it). */
/* Two players, a picture between steps: each half's hills and sky from
   the picture's angles (fx_tween's, each panel's), worked out as
   BackdropScrollFromView and GradientStartsFromViews work out the step's
   own, so they turn with the 3D. A half showing its panel keeps its
   scrolls. Only while a picture is drawn: pic is the game's again after. */
static void two_halves_between(void)
{
    uint16_t heading[2], pitch[2];
    int half;

    if (!video_between_drawing() || !race_two_full_window() || !fx_tween_backdrop_angles(heading, pitch))
        return;
    for (half = 0; half < 2; half++) {
        uint16_t h, t;
        unsigned s;

        if (pic.race.panel[half])
            continue;
        h = (uint16_t)(player_vehicle_raised(half) + pitch[half] + 0x18);
        s = (unsigned)(uint16_t)(h << 1) + 0xEF99 + (h >> 15);
        t = (uint16_t)s;
        if (t >= 0xF257)
            t = 0xF257;
        pic.race.backdrop_x[half] = (int16_t)(uint16_t)(heading[half] >> 2);
        pic.race.backdrop_y[half] = (int16_t)(uint16_t)(h + backdrop_two_scroll_base(half));
        pic.race.gradient_start[half] = (uint16_t)(GRADIENT_AT_DAY + (uint16_t)(t - 0xEF57) / 2);
    }
}

void race_two_player_list(void)
{
    static struct { int32_t x[2], y[2]; uint8_t panel[2]; uint16_t sky[2]; } shown;
    static int which;

    two_halves_between();
    if (memcmp(shown.x, pic.race.backdrop_x, sizeof shown.x) || memcmp(shown.y, pic.race.backdrop_y, sizeof shown.y)
        || memcmp(shown.panel, pic.race.panel, sizeof shown.panel)
        || (race_two_full_window() && memcmp(shown.sky, pic.race.gradient_start, sizeof shown.sky))) {
        memcpy(shown.sky, pic.race.gradient_start, sizeof shown.sky);
        memcpy(shown.x, pic.race.backdrop_x, sizeof shown.x);
        memcpy(shown.y, pic.race.backdrop_y, sizeof shown.y);
        memcpy(shown.panel, pic.race.panel, sizeof shown.panel);
        which ^= 1;
    }
    pic.bg[1].draw_list = fx_draw_picture(which);
}

/* ---- the pause screen's turning picture ---- */

/* The next picture when its delay is up: the pace by the buttons held, the
   number back with Select, unpacked and made 4-bit by the chip into the half of
   its RAM the other buffer is not using. It goes to the sprites' art at the
   next vertical blank (VramUploadGsuBuffer). */
void PictureTurnStep(void)
{
    uint16_t held = g.pad_last[0], t = g.picture_turn_number;

    if (g.picture_turn_pending)
        return;
    if (g.picture_turn_delay) {
        g.picture_turn_delay--;
        return;
    }
    g.picture_turn_delay = (held & 0x0040) ? 3 : (held & 0x4000) ? 2 : (held & 0x0020) ? 6 : (held & 0x0010) ? 8 : 4;
    if (frame_state.held[0] & 0x2000) {
        t = (uint16_t)(t - 1);
        if (t & 0x8000)
            t = 7;
    } else {
        t = (uint16_t)(t + 1);
    }
    t = turn_picture_resolve(t);        /* an entry may name the picture to go to instead */
    g.picture_turn_number = t;
    if (g.fx_buffer) {
        turn_picture_unpack(t, fx_ram + FX_UNPACK_AREA, UNPACK_ROOM);
        FxTiles2bppTo4bppLow();
    } else {
        turn_picture_unpack(t, fx_ram + FX_TURN_UNPACK_HIGH, UNPACK_ROOM);
        FxTiles2bppTo4bppHigh();
    }
    g.picture_turn_pending = 0xFF;
}

/* The turned picture, when one is waiting, to the sprites' art. */
void VramUploadGsuBuffer(void)
{
    if (!g.picture_turn_pending)
        return;
    race_art_write(RACE_ART_SPRITES + 0x060, fx_ram + (g.fx_buffer ? FX_TURN_PICTURE_LOW : FX_TURN_PICTURE_HIGH), 0x0C00);
    g.picture_turn_pending = 0;
}
