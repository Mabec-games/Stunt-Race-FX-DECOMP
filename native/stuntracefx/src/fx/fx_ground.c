/*
 * fx_ground.c: the race's ground and water as a surface (plan stage 10,
 * "The ground and water"; the earlier work is be60bbb9, 8931a216..c68f1373
 * and af4032ea, remaster/fullwindow.c).
 *
 * Each course's surface is land or water, by its band's ground colours and
 * what its scenery is (the table below).
 *
 * Land, on the courses done so far (Easy Ride; Jeff, 2026-11-11, track by
 * track), is solid polygons in the course's own style: a plane at the
 * course's ground level out to the horizon and a ring of polygon mountains
 * round the course, each face shaded as the chip shades the course's own
 * lit faces ("the land as polygons", below). On the rest, until each is
 * done, land follows the course's own heights: once a course, the survey
 * (below) takes where its shapes meet the ground into a grid of heights,
 * and terrain.shade colours a few planes over it on the GPU, the land's
 * colour by distance (the band's, or the course's rock's), lit a little by
 * its slope, with the kind's marks.
 *
 * Water is a plane at the course's water level. Per picture and player it
 * is one convex polygon: the plane turned to the view
 * (the matrix at $00E4, the eye's place from the draw list), cut against the
 * near plane and the view's sides, out to where it meets the horizon within
 * a hundredth of a pixel (fx_face_triangles_whole). The GPU works out each of
 * its pixels with ground.shade: the vertices' u, v are their screen places
 * less an origin inside the polygon, and the shader's eight inputs are the
 * projective map from such a place to the plane's texels, so each pixel's
 * texel and depth are exact in perspective. Its image holds the cell of
 * marks repeated across the plane, the band's colour on each screen row
 * (the colour between the marks, as the console shows the band) and the
 * numbers the shader needs (fx_ground_image below).
 *
 * Drawn first, before the far and near objects; the ground in a range of
 * depths of its own behind all of the 3D, so the track covers it and a road
 * below it is never left out (docs/stuntracefx/native/DEPTH.md, 2); not
 * drawn while the eye is below it. Between its marks
 * the band shows through, the console's own colours; marks, as the game has
 * no ground art: on land a tuft (a 3-wide V) in two of each three 8 x 4
 * spots, on water a glint (a 4-wide dash) in one of each two, a darker shade
 * of the band's colour on their row (be60bbb9 took the band's colour 24 rows
 * nearer, which below the band's last line is the same colour), on cells
 * the size of Aqua Tunnel's ripple cells, left out where a texel is smaller
 * than an output pixel.
 *
 * Aqua Tunnel's water is its underwater backdrop's art (video's
 * race_underwater_cells): the nearest row of ripple cells, 64 x 16 texels,
 * repeated across the plane; each ripple in the colour its strip's rows
 * take at the pixel's depth (a strip row's commonest colour, the rows'
 * depths as the original shows them, WATER_DEPTH); the water between them
 * the band's colour on the row, drawn, not left to show through; where an
 * output pixel spans more than a texel, the ripples go to their mean cover.
 * Its water cuts the picture (fx_face_cut): what lies past it from the eye
 * is drawn before it, the rest after, so from above the glass tube is under
 * the water and from inside the tube the water is overhead; where a track
 * piece crosses the water the water has a hole the size of the piece's
 * crossing, and what lies past the water shows through it (the road going
 * down inside its tunnel). In the sealed tunnel (bit 3 of $0DA5) there is
 * no water (the earlier water's; the ocean, below, is drawn there too).
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "data/video_tables.h"
#include "fx/fx.h"
#include "fx/fx_draw.h"
#include "fx/fx_face.h"
#include "fx/fx_far.h"
#include "fx/fx_ground.h"
#include "fx/fx_rt.h"
#include "fx/fx_view.h"
#include "system/state.h"
#include "video/picture.h"
#include "video/video.h"

/* the surface's cell: 64 x 16 texels laid at Aqua Tunnel's ripples' size,
   33.6 and 53.7 world units a texel (the underwater backdrop's nearest row
   of cells against the plane, read off its most level frame) */
#define CELL_U 64
#define CELL_V 16
#define TEXEL_U 33.6
#define TEXEL_V 53.7
#define INK 0.8                 /* the marks' colour: the band's on their row, this much as bright */
/* A lit face's level against the chip's light (fixed in view space, y
   down, z ahead: FxModelProject's $0100-$0104, the picture's from
   fx_view_light_vector) where a course has no lit faces to fit it from
   (land as polygons, below): the chip's level is (its normal . the light,
   each as 127 a unit) / 1024, its normals facing in */
#define LIGHT_K (-127.0 * 127.0 / 1024.0)
#define HORIZON_GAP 0.01        /* the polygon's far edge this far from the horizon, in output pixels */

enum { SURF_NONE, SURF_GRASS, SURF_ROCK, SURF_CEMENT, SURF_SNOW, SURF_ICE, SURF_DIRT, SURF_WATER, SURF_AQUA, SURF_CLOUD };

#define IS_LAND(k) (((k) >= SURF_GRASS && (k) <= SURF_DIRT) || (k) == SURF_CLOUD)
#define CLS_ROCK 0xFF           /* the surface table's class for the course's own rock (the survey's fit): mesas */
#define CLS_WHITE 0xFD          /* class 0, the chip's snow whites (the table's 0 is no class) */
#define CLS_BAND 0xFC           /* a sea in the band's own sea's colours (band_shade): Lake Side */
#define CLS_BAND_WHOLE 0xFB     /* land or a sea in all of them: the battle courses */
#define WATER_DEEPEST 32767     /* water at the lowest place the course's shapes meet the ground (the survey's) */

/* Each course's surface (g.course, $0E3D), from pictures of every course
   (2026-11-09): its kind, and for water the height of its level (world y
   is down). Land takes its heights from the course's own shapes (the
   survey below). None: the checkered test arenas. Sky Ramp (its band is
   sky all the way down, the track meant to float) has a sea of clouds
   under it (the clouds, below). The airport is the
   night city (Jeff). Water is at the height where its lowest road's pieces
   end (a road piece reaches 640 below the road), measured from the
   course's shapes, so the road stands in it and nothing of the road hangs
   above it. Land with a shade class (cls, a class of the chip's lit faces'
   shade rows; for water `under` the class seen from below, the eye under
   the water) is drawn as polygons in that class's shades (the land as
   polygons, below); the rest as before, until each course is done (Jeff,
   2026-11-11: track by track, Easy Ride first). A flat course's land is
   the plane alone, with no ring of mountains round it: a city, whose
   skyline is on its backdrop at the horizon, which mountains would hide.
   A forest course's land has the course's own trees standing round it
   (the forest, below).

   The battle courses ($10-$13) are drawn by two players, whose palettes
   hold no shade rows (their colours $10-$FF are the two players' sixteen-
   colour rows: a class's row reads blacks, reds and yellows), so their
   land and water take their bands' own colours, all of them
   (CLS_BAND_WHOLE: the original's ground under the horizon, the two
   players' bands reading the same lists). The stunt arenas ($0C-$0F) keep
   their floors as they are (Jeff, 2026-11-12): the floor is what the car
   drives on, inside the stadium on the backdrop, and its marks show the
   speed. */
static const struct {
    uint8_t kind;
    int16_t y;
    uint8_t cls;
    uint8_t under;          /* water seen from below: its shade class (0: cls's) */
    uint8_t flat;           /* land: the plane alone, no mountains */
    uint8_t forest;         /* land: a forest of the course's own trees round it */
    uint8_t first;          /* water: the class's level its sea's darkest shade takes (0: 6, the whole row) */
    uint8_t city;           /* land: a city round the course (the city, below) */
    uint8_t mesas;          /* land: a ring of mesas (CLS_ROCK's, without its fill) in place of the mountains */
} surface[0x20] = {
    [0x00] = { SURF_GRASS, 0, 10, 0, 0, 0, 0, 0, 0 },  /* Easy Ride: class 10, the chip's grass green (82,131,0 to 148,197,57) */
    [0x01] = { SURF_AQUA, -1280, 12, 9, 0, 0, 0, 0, 0 }, /* Aqua Tunnel: under the road, over the glass tube, where its
                                           road's pieces end (the road -1920, the pieces 640 below) */
    [0x02] = { SURF_ROCK, 0, CLS_ROCK, 0, 0, 0, 0, 0, 0 }, /* Sunset Valley, the canyon: mesas in its own rock's shades */
    [0x03] = { SURF_CEMENT, 0, 21, 0, 1, 0, 0, 0, 0 }, /* Night Owl, the night city, the airport: flat, class 21, the
                                           greys its own road is shaded in (its colours 63-56: 49,49,41
                                           to 107,107,99 by its palette; the road ahead 66,66,57 and
                                           74,74,66, levels 9 and 10, the band's under the eye) */
    [0x04] = { SURF_GRASS, 0, 10, 0, 0, 1, 0, 0, 0 },  /* King's Forest, the foggy forest: Easy Ride's land, class 10,
                                           its own hills' grass (their faces 173,206,132 and 189,214,140:
                                           the class's top levels in its palette blended 15 of 32 toward
                                           the fog, which the palette carries, so the land is fogged as
                                           they are), and a forest of its own trees round it */
    [0x05] = { SURF_WATER, -48, 9, 0, 0, 0, 11, 0, 0 }, /* Sea Breeze, the coast: a sea, Aqua Tunnel's ocean, at the level
                                           and in the colours of the water the road crosses (Jeff,
                                           2026-11-12: the sea to match it, and at its level, where it had
                                           gone from sight as the road went down to it): the crossing is
                                           shape $E5B4 (three of it), its face level at y -96 to 0 (the sea
                                           at its middle), its texture ($13:$C040, colour $50, the high
                                           nibble held) drawing entries $58-$5B, class 9's four lightest
                                           (8,140,239 to 90,222,255): the class's levels 11-15 */
    [0x06] = { SURF_SNOW, 0, CLS_WHITE, 0, 0, 0, 0, 0, 0 }, /* White Land, the snowfield: Easy Ride's land in class 0, the
                                           whites its own snow banks are in (214,231,231 to 255,255,255;
                                           its road class 21's icy greys), its fog over it as the
                                           palette carries it (amount_step's 10 of 32 for it) */
    [0x07] = { SURF_CEMENT, 0, 21, 0, 1, 0, 0, 1, 0 }, /* Night Cruise, the sunset city: a race track in the middle of
                                           a city (Jeff, 2026-11-12: "flat pavement terrain like you've done on
                                           some other tracks. like it's a race track in the middle of the
                                           city"): flat (no mountains before its skyline), pavement in class
                                           21, its road's own mauve greys (Night Owl's land in its road's
                                           class), a city round it as the backdrop's skyline is ("It should
                                           read as a city scape. Look at the skymap"); its khaki verges, the
                                           course's own faces, shaded toward the pavement by distance
                                           (fx_ground_verge), the weather's blend to night as the palette
                                           carries it */
    [0x08] = { SURF_WATER, WATER_DEEPEST, CLS_BAND, 0, 0, 0, 0, 0, 0 }, /* Lake Side: a lake, Aqua Tunnel's ocean in
                                           the band's own lake's colours (its ground half Sea Breeze's,
                                           teal to sea-green, the lake light teal by the road); its road
                                           crosses no water (none of its faces is a level textured one, a
                                           dump of 2026-11-12), so the lake is at the lowest place its
                                           shapes meet the ground, where the road never goes under it */
    [0x09] = { SURF_ROCK, 0, CLS_ROCK, 0, 0, 0, 0, 0, 0 }, /* Big Ravine, the wild-west canyon: Sunset Valley's land (mesas,
                                           the rock filling its empty space, rock round its tunnels) in its
                                           own rock's run (its walls 90,49,0 to 41,8,0; the survey's 5
                                           shades) */
    [0x0A] = { SURF_CLOUD, 0, CLS_WHITE, 0, 1, 0, 0, 0, 0 }, /* Sky Ramp, the track in the sky: a sea of clouds under it (Jeff,
                                           2026-11-12), flat, in class 0, the whites of its own white
                                           shapes (its start's gate and pillars, d5e6e6 to ffffff by its
                                           palette), cloud banks on it (the clouds, below) */
    [0x0B] = { SURF_WATER, -656, 9, 9, 0, 0, 11, 0, 0 }, /* Harbor City, the night harbour (Jeff,
                                           2026-11-12: its blue channel "is clearly meant to be underwater",
                                           "the water around it appears nonexistent"): its road dives
                                           through a channel of water-textured faces (shapes $FA60 and
                                           $FA7C: the road from y -672 at its ends to -161, walls to -640;
                                           a dump of its textured faces), the texture Sea Breeze's crossing
                                           has (entries $58-$5B, here 006ad5 to 001073, class 9's levels
                                           11-15); the harbour is Aqua Tunnel's ocean at the walls' tops,
                                           16 over them so they stay under it at a far piece's halved
                                           precision and no hole opens over the channel, in those levels,
                                           seen from below as well (under) and cutting the picture as
                                           Aqua Tunnel's does, so the channel lies under the harbour's
                                           water; at -3840, where it was, it lay over the channel and the
                                           camera, going down to it, passed under the water, where a
                                           sea is not drawn */
    [0x0C] = { SURF_DIRT, 0, 0, 0, 0, 0, 0, 0, 0 },    /* the stunt arenas: dirt, grass, ice, water */
    [0x0D] = { SURF_GRASS, 0, 0, 0, 0, 0, 0, 0, 0 },
    [0x0E] = { SURF_ICE, 0, 0, 0, 0, 0, 0, 0, 0 },
    [0x0F] = { SURF_WATER, 0, 0, 0, 0, 0, 0, 0, 0 },   /* its floor */
    [0x10] = { SURF_GRASS, 0, CLS_BAND_WHOLE, 0, 0, 0, 0, 0, 0 }, /* the battle arenas: the countryside, Easy
                                           Ride's sky and backdrop; Easy Ride's land (the plane, the ring
                                           of mountains) in its band's greens, 5a9c9c at the horizon to
                                           20d562 under the eye */
    [0x11] = { SURF_ROCK, 0, CLS_BAND_WHOLE, 0, 0, 0, 0, 0, 1 }, /* the red-rock desert, Sunset Valley's sky and
                                           backdrop (its buttes): a ring of mesas in its band's red-
                                           browns, 520800 to 734120 */
    [0x12] = { SURF_WATER, WATER_DEEPEST, CLS_BAND_WHOLE, 0, 0, 0, 0, 0, 0 }, /* the beach, Sea Breeze's sky
                                           and band: its band under the horizon is the sea (8beee6 to
                                           39c58b), a car off the arena lands in it ("SPLASH"); Aqua
                                           Tunnel's ocean in those colours at the lowest place its shapes
                                           meet the ground, so the arena stands in it (Lake Side's) */
    [0x13] = { SURF_WATER, WATER_DEEPEST, CLS_BAND_WHOLE, 0, 0, 0, 0, 0, 0 }, /* the night city, Harbor City's sky
                                           and band: Aqua Tunnel's ocean in its band's navies, at the
                                           lowest place its shapes meet the ground (as the beach's): at 0
                                           its start's road, lower, showed the water over it */
    [0x14] = { SURF_GRASS, 0, 10, 0, 0, 0, 0, 0, 0 }, /* the bonus courses (Speed Trax's truck runs, one a class):
                                           Easy Ride's palette, sky and backdrop, so Easy Ride's land,
                                           class 10 */
    [0x15] = { SURF_GRASS, 0, 10, 0, 0, 0, 0, 0, 0 },
    [0x16] = { SURF_GRASS, 0, 10, 0, 0, 0, 0, 0, 0 },
};

/* the land's marks: the size of a texel on the ground, and each kind's
   brightness against the band (a shade darker; snow and ice a shade
   lighter) */
#define LAND_TEXEL 32.0
static const float land_ink[] = { [SURF_GRASS] = 0.8f, [SURF_ROCK] = 0.7f, [SURF_CEMENT] = 0.84f,
                                  [SURF_SNOW] = 1.12f, [SURF_ICE] = 1.1f, [SURF_DIRT] = 0.78f, [SURF_CLOUD] = 1.12f };

/* Aqua Tunnel's strip rows' depths from the view, as the original shows the
   underwater backdrop on its most level frame (rows 51-60 over lines 1 to
   the horizon): a ripple at a depth takes the colour of the first row as
   deep (be60bbb9's WATER_DEPTH) */
static const float water_depth[80] = {
    2152, 2180, 2208, 2236, 2266, 2296, 2327, 2359, 2392, 2426, 2461, 2497, 2534, 2572, 2611, 2651,
    2693, 2736, 2780, 2826, 2873, 2922, 2973, 3025, 3079, 3135, 3194, 3254, 3317, 3383, 3451, 3522,
    3595, 3672, 3753, 3837, 3924, 4016, 4112, 4213, 4319, 4431, 4548, 4672, 4803, 4941, 5088, 5243,
    5409, 5585, 5773, 5974, 6189, 6421, 6670, 6940, 7233, 7551, 7898, 8280, 8699, 9164, 9681, 10260,
    10912, 11653, 12503, 13485, 14635, 16000, 17645, 19668, 22214, 25518, 29976, 36321, 46074, 62987, 99520, 236952,
};

/* The image's layout (ground.shade reads it the same way): RGBA texels,
   IMAGE_W across, each a word. Row 0: the header's words (H_*); rows 1-16:
   the cell, alpha set where a mark is; from word STRIP: the band's colour
   on each screen row from the header's first row on; from RIPPLES: Aqua
   Tunnel's ripple colour by strip row, then from DEPTHS each strip row's
   depth (floats), RIPPLE_ROWS of each. */
enum {
    IMAGE_W = 160,
    STRIP = 17 * IMAGE_W,
    STRIP_MAX = 2048,
    RIPPLE_ROWS = 80,
    RIPPLES = STRIP + STRIP_MAX,
    DEPTHS = RIPPLES + RIPPLE_ROWS,
    IMAGE_H = (DEPTHS + RIPPLE_ROWS + IMAGE_W - 1) / IMAGE_W,
    H_ORIGIN_X = 0, H_ORIGIN_Y, H_FIRST_ROW, H_ROWS, H_KIND, H_COVER, H_DEPTH, H_INK, H_TEXEL_SCALE
};

static nshvc_image image[4];            /* one a picture: two, and two between steps */
static nshvc_shader shader;
static uint32_t words[IMAGE_W * IMAGE_H];

static uint16_t ram_word(uint32_t a)
{
    return (uint16_t)(fx_ram[a] | fx_ram[a + 1] << 8);
}

static uint32_t float_word(float f)
{
    uint32_t w;

    memcpy(&w, &f, sizeof w);
    return w;
}

static uint32_t rgb_of(uint16_t c)
{
    unsigned r = c & 31, gg = c >> 5 & 31, b = c >> 10 & 31;

    return 0xFF000000u | (r << 3 | r >> 2) << 16 | (gg << 3 | gg >> 2) << 8 | (b << 3 | b >> 2);
}

/* the marks: a tuft on land, a glint on water (be60bbb9's) */
static int tuft(unsigned x, unsigned y)
{
    unsigned cx = x / 8, cy = y / 4, h = (cx * 2654435761u) ^ (cy * 40503u), ox = h % 5, oy = (h >> 8) % 2;
    unsigned lx = x % 8, ly = y % 4;
    int on = (ly == oy + 1 && (lx == ox || lx == ox + 2)) || (ly == oy && lx == ox + 1);

    return on && (h >> 16) % 3 != 0;
}

/* a stone: in one of each two 8 x 4 spots, a rounded blob 3 to 5 texels
   across at a hashed place, and a crack (a 1-texel line) in one of each
   five */
static int stone(unsigned x, unsigned y)
{
    unsigned cx = x / 8, cy = y / 4, h = (cx * 2654435761u) ^ (cy * 40503u), ox = 2 + h % 4, oy = 1 + (h >> 8) % 2, r = 1 + (h >> 12) % 2;
    int dx = (int)(x % 8) - (int)ox, dy = (int)(y % 4) - (int)oy;

    if ((h >> 16) % 2 && dx * dx + 2 * dy * dy <= (int)(r * r + 1))
        return 1;
    return (h >> 20) % 5 == 0 && y % 4 == oy && x % 8 < 6;
}

/* cement: slabs 16 texels across and 8 down, the joints a texel wide */
static int slab(unsigned x, unsigned y)
{
    return x % 16 == 0 || y % 8 == 0;
}

/* snow and ice: sparkles, a texel in one of each 8 x 4 spot in three;
   ice also a streak 6 long in one spot in four */
static int sparkle(unsigned x, unsigned y, int streaks)
{
    unsigned cx = x / 8, cy = y / 4, h = (cx * 2654435761u) ^ (cy * 40503u), ox = h % 8, oy = (h >> 8) % 4;

    if (streaks && (h >> 20) % 4 == 0)
        return y % 4 == oy && x % 8 >= ox / 4 && x % 8 < ox / 4 + 6;
    return (h >> 16) % 3 == 0 && x % 8 == ox && y % 4 == oy;
}

/* dirt: specks, a texel in each 4 x 2 spot in two */
static int speck(unsigned x, unsigned y)
{
    unsigned cx = x / 4, cy = y / 2, h = (cx * 2654435761u) ^ (cy * 40503u);

    return (h >> 16) % 2 && x % 4 == h % 4 && y % 2 == (h >> 8) % 2;
}

static int land_mark(int kind, unsigned x, unsigned y)
{
    switch (kind) {
    case SURF_GRASS: return tuft(x, y);
    case SURF_ROCK: return stone(x, y);
    case SURF_CEMENT: return slab(x, y);
    case SURF_SNOW: return sparkle(x, y, 0);
    case SURF_ICE: return sparkle(x, y, 1);
    case SURF_DIRT: return speck(x, y);
    default: return 0;
    }
}

static int glint(unsigned x, unsigned y)
{
    unsigned cx = x / 8, cy = y / 4, h = (cx * 2654435761u) ^ (cy * 40503u), ox = h % 4, oy = (h >> 8) % 4;

    return y % 4 == oy && x % 8 >= ox && x % 8 < ox + 4 && (h >> 16) % 2;
}

/* x mod m, from 0 up */
static double wrap(double x, double m)
{
    return x - m * floor(x / m);
}

/* ---- the ground's heights: the course's own shapes ----

   Once a course, every course object's model points in world space,
   through the chip's own object walk: each record alone in the first slot,
   placed a little ahead of an eye whose view matrix ($00E4) is the
   identity, its points caught as the frame program projects them
   (fx_face_survey), nothing drawn (fx_draw_hold), the chip's RAM and state
   put back after. Sprites, effects and objects placed in view space are
   left out; of each shape only its lowest part counts, the points within
   FOOT of its lowest (a pillar's foot, a road's surface, a wall's base).
   The near records are in view space after the sort (turned back by the
   matrix's rows), the far ones from the view in 32 bits. A grid of cells
   takes in each the lowest point that falls in it (world y is down: the
   largest), where the shapes meet the ground; cells with none take the
   nearest such cell's height, then are smoothed from their neighbours, the
   known cells held and the grid's edge free, so past the course the land
   goes on at the height of the course beside it (no height of its own
   pulls it up or down). Its rock, from the course's own untextured faces
   as the chip queues them: the colour with the most area on steep faces
   (cliffs, mesas, banks) and the palette entries next to it that its
   faces also use, the rock's shades (the canyons' are runs of four or five
   reds and browns, the shade baked into each face by the way it faces);
   their order by brightness, and how a face's shade goes with the way it
   faces, fitted by least squares over those faces (shade = light . n +
   bias, n its outward normal), so that rock land at any slope takes the
   shade the course gives a rock face facing that way. */

enum { GRID_MOST = 128, GRID_MARGIN = 4, SURVEY_POINTS = 65536, SURVEY_AHEAD = 512, FOOT = 32, ROCK_MOST = 8 };

static struct {
    int course;                 /* the course it is of; -1 none */
    double x0, z0, cell;        /* the grid's corner in the world and its cell's size */
    int nx, nz;
    int shades;                 /* its rock's shades, darkest first; 0 none */
    uint8_t shade[ROCK_MOST];   /* their colour bytes */
    double light[3], bias;      /* a face with outward normal n has shade light . n + bias */
    float h[GRID_MOST * GRID_MOST];
    double level;               /* the course's ground level: the grid's median height */
    double xmin, xmax, zmin, zmax;      /* the course's footprint: where its shapes meet the ground */
    double light_k;             /* a lit face's level: light_k * (its outward normal . the light) */
    double deepest;             /* the lowest place a shape meets the ground (y is down: the largest) */
} grid = { -1, 0, 0, 0, 0, 0, 0, { 0 }, { 0, 0, 0 }, 0, { 0 }, 0, 0, 0, 0, 0, 0, 0 };

/* the grid's cells where a course's shape meets the ground */
static uint8_t grid_known[GRID_MOST * GRID_MOST];

/* the survey's untextured faces by colour byte: area on steep faces (the
   normal less than 45 degrees from level; y is the world's up axis, the
   survey's view being the identity) and on all, and the sums for the
   fit: area-weighted normals (Newell's, as the corners go round) and
   their products; which way Newell's normals point (out of the shapes or
   into them) is found from all faces after */
static struct {
    double steep[256], area[256], n[256][3], nn[256][3][3];
    double facing;
    double lit_ln, lit_nn, lit_area;    /* lit faces: sums of area * level * (n . light), area * (n . light)^2, area */
} rock_survey;

static float survey_point[SURVEY_POINTS][3];
static int survey_points;

/* The record being surveyed: its faces' area by the way their Newell
   normals point (down, up, across x, across z); and each record's shape,
   place, box and those areas, to find the course's tunnel pieces (the
   tunnels' rock, below). */
static double rec_area[4];
enum { CANDIDATES = 0x31 + FX_FAR_MAX };
static struct {
    uint16_t shape;
    double at[3], lo[3], hi[3], area[4];
} cand[CANDIDATES];
static int ncand, surveying;

static void survey_sink(double x, double y, double z, void *context)
{
    const double *at = (const double *)context;

    if (survey_points == SURVEY_POINTS)
        return;
    survey_point[survey_points][0] = (float)(at[0] + x);
    survey_point[survey_points][1] = (float)(at[1] + y);
    survey_point[survey_points][2] = (float)(at[2] + z - SURVEY_AHEAD);
    survey_points++;
}

/* a face the chip queued, into the sums of its colour; a lit face also
   into the fit of its light level against its facing (the land as
   polygons, below): the survey's view is the identity, so its normal is
   in the world's axes, and the light the chip gave it the view's light
   in the same axes */
static void survey_face(const double (*c)[3], int n, uint8_t colour, int textured, int lit, float light, void *context)
{
    double nv[3] = { 0, 0, 0 }, len, area, u[3];
    int i, j;

    (void)context;
    for (i = 0; i < n; i++) {           /* Newell's normal: its length is twice the area */
        const double *a = c[i], *b = c[(i + 1) % n];

        nv[0] += (a[1] - b[1]) * (a[2] + b[2]);
        nv[1] += (a[2] - b[2]) * (a[0] + b[0]);
        nv[2] += (a[0] - b[0]) * (a[1] + b[1]);
    }
    len = sqrt(nv[0] * nv[0] + nv[1] * nv[1] + nv[2] * nv[2]);
    if (len <= 0)
        return;
    area = 0.5 * len;
    for (i = 0; i < 3; i++)
        u[i] = nv[i] / len;
    rock_survey.facing += u[2];
    if (u[1] > 0.7)
        rec_area[0] += area;
    else if (u[1] < -0.7)
        rec_area[1] += area;
    else
        rec_area[fabs(u[0]) >= fabs(u[2]) ? 2 : 3] += area;
    if (lit) {
        double l[3], x;

        fx_view_light_vector(l);        /* the light the chip lit the face by (fx_view.h) */
        x = u[0] * l[0] + u[1] * l[1] + u[2] * l[2];

        rock_survey.lit_ln += area * light * x;
        rock_survey.lit_nn += area * x * x;
        rock_survey.lit_area += area;
    }
    if (textured)
        return;
    rock_survey.area[colour] += area;
    if (fabs(u[1]) < 0.70710678)
        rock_survey.steep[colour] += area;
    for (i = 0; i < 3; i++) {
        rock_survey.n[colour][i] += area * u[i];
        for (j = 0; j < 3; j++)
            rock_survey.nn[colour][i][j] += area * u[i] * u[j];
    }
}

/* m x = b, m n x n, by elimination with the largest pivot; 0 when m has
   no inverse */
static int solve(int n, double m[4][4], double b[4], double x[4])
{
    int c, r, k;

    for (c = 0; c < n; c++) {
        int best = c;

        for (r = c + 1; r < n; r++)
            if (fabs(m[r][c]) > fabs(m[best][c]))
                best = r;
        if (fabs(m[best][c]) < 1e-12)
            return 0;
        for (k = 0; k < n; k++) {
            double t = m[c][k];

            m[c][k] = m[best][k];
            m[best][k] = t;
        }
        {
            double t = b[c];

            b[c] = b[best];
            b[best] = t;
        }
        for (r = c + 1; r < n; r++) {
            double f = m[r][c] / m[c][c];

            for (k = c; k < n; k++)
                m[r][k] -= f * m[c][k];
            b[r] -= f * b[c];
        }
    }
    for (c = n - 1; c >= 0; c--) {
        double t = b[c];

        for (k = c + 1; k < n; k++)
            t -= m[c][k] * x[k];
        x[c] = t / m[c][c];
    }
    return 1;
}

static double luma_of(uint8_t c)
{
    uint32_t rgb = rgb_of(pic.palette[fx_draw_ink(c)]);

    return 0.299 * (rgb >> 16 & 255) + 0.587 * (rgb >> 8 & 255) + 0.114 * (rgb & 255);
}

/* the rock's shades and their fit, from the survey's sums */
static void rock_shades(void)
{
    double m[4][4] = { { 0 } }, b[4] = { 0 }, x[4], most = 0;
    int start = -1, lo, hi, c, i, j, k;

    grid.shades = 0;
    for (c = 1; c < 256; c++)
        if (rock_survey.steep[c] > most && fx_draw_ink((uint8_t)c)) {
            most = rock_survey.steep[c];
            start = c;
        }
    if (start < 0)
        return;
    /* its run: the entries next to it the course's faces use (each with
       at least a hundredth of its steep area), up to ROCK_MOST */
    lo = hi = start;
    while (hi - lo + 1 < ROCK_MOST) {
        int below = lo > 1 && rock_survey.area[lo - 1] >= 0.01 * most && fx_draw_ink((uint8_t)(lo - 1));
        int above = hi < 255 && rock_survey.area[hi + 1] >= 0.01 * most && fx_draw_ink((uint8_t)(hi + 1));

        if (!below && !above)
            break;
        if (below && (!above || rock_survey.area[lo - 1] >= rock_survey.area[hi + 1]))
            lo--;
        else
            hi++;
    }
    /* darkest first */
    for (c = lo; c <= hi; c++) {
        for (i = grid.shades; i > 0 && luma_of(grid.shade[i - 1]) > luma_of((uint8_t)c); i--)
            grid.shade[i] = grid.shade[i - 1];
        grid.shade[i] = (uint8_t)c;
        grid.shades++;
    }
    /* shade k = light . n + bias, least squares over the run's faces by
       area: the normal equations from each colour's sums */
    for (k = 0; k < grid.shades; k++) {
        c = grid.shade[k];
        for (i = 0; i < 3; i++) {
            for (j = 0; j < 3; j++)
                m[i][j] += rock_survey.nn[c][i][j];
            m[i][3] += rock_survey.n[c][i];
            m[3][i] += rock_survey.n[c][i];
            b[i] += k * rock_survey.n[c][i];
        }
        m[3][3] += rock_survey.area[c];
        b[3] += k * rock_survey.area[c];
    }
    if (grid.shades < 2 || !solve(4, m, b, x)) {
        /* one shade, or no fit: all the commonest */
        grid.shades = 1;
        grid.shade[0] = (uint8_t)start;
        x[0] = x[1] = x[2] = x[3] = 0;
    }
    /* outward normals: the faces the survey's eye sees face it, back along
       z, so theirs go to negative z when Newell's point outward */
    for (i = 0; i < 3; i++)
        grid.light[i] = rock_survey.facing < 0 ? x[i] : -x[i];
    grid.bias = x[3];
}

/* one record (30 bytes) in the first slot, its place in the world `at` */
static void survey_record(const uint8_t *record, double at[3], uint16_t *stack)
{
    uint8_t flags = record[0x07];

    if (flags & 0x8C)                   /* placed in view space, an effect, a sprite */
        return;
    memcpy(&fx_ram[0x0AAA], record, 0x1E);
    fx_ram[0x0AAA] = fx_ram[0x0AAB] = 0;
    fx_ram[0x0AAA + 0x12] = fx_ram[0x0AAA + 0x13] = 0;
    fx_ram[0x0AAA + 0x10] = fx_ram[0x0AAA + 0x11] = 0;
    fx_ram[0x0AAA + 0x14] = (uint8_t)SURVEY_AHEAD;
    fx_ram[0x0AAA + 0x15] = (uint8_t)(SURVEY_AHEAD >> 8);
    fx_ram[GSU_DRAW_LIST] = 0xAA;
    fx_ram[GSU_DRAW_LIST + 1] = 0x0A;
    {
        int from = survey_points, i, kept_n = from;
        float lowest = -1e9f;

        memset(rec_area, 0, sizeof rec_area);
        fx_face_survey(survey_sink, survey_face, at);
        *stack = 0x0240;
        (void)FxObjectsDraw(0, stack);
        fx_face_survey(NULL, NULL, NULL);
        if (survey_points > from && ncand < CANDIDATES) {
            int q;

            cand[ncand].shape = (uint16_t)(record[0x08] | record[0x09] << 8);
            for (q = 0; q < 3; q++) {
                cand[ncand].at[q] = at[q];
                cand[ncand].lo[q] = 1e9;
                cand[ncand].hi[q] = -1e9;
            }
            for (i = from; i < survey_points; i++)
                for (q = 0; q < 3; q++) {
                    cand[ncand].lo[q] = fmin(cand[ncand].lo[q], survey_point[i][q]);
                    cand[ncand].hi[q] = fmax(cand[ncand].hi[q], survey_point[i][q]);
                }
            memcpy(cand[ncand].area, rec_area, sizeof rec_area);
            ncand++;
        }
        /* where the shape meets the ground: its lowest part (world y is
           down: the largest y), the points within FOOT of it; the rest of
           it (a gantry's beam, a banner) stands above the ground */
        for (i = from; i < survey_points; i++)
            lowest = fmaxf(lowest, survey_point[i][1]);
        for (i = from; i < survey_points; i++)
            if (survey_point[i][1] >= lowest - FOOT)
                memcpy(survey_point[kept_n++], survey_point[i], sizeof survey_point[0]);
        survey_points = kept_n;
    }
}


static int float_order(const void *a, const void *b)
{
    float x = *(const float *)a, y = *(const float *)b;

    return (x > y) - (x < y);
}

/* ---- the tunnels' rock (Jeff, 2026-11-11: "give it rocky sides") ----

   A tunnel piece's model is its inside only (roof, walls, lights): seen
   from the side, across open ground, its inside showed. Found by the
   survey's records (Sunset Valley's, dumped 2026-11-11: seven of shape
   $E384, 3,840 x 3,903 and 960 high, a roof's area (16.6 million) and two
   walls' (6.6), nothing facing up; one $E480 the same with its lights): a
   piece no more than TUNNEL_HIGH high, at least TUNNEL_WIDE across both
   ways, the area facing down (outward) at least half its box's ground and
   the area facing sideways at least a quarter of that. Its open ends are
   where no walls face: across x when its walls face x. Each gets a shell
   of rock: walls from the ground (the plane, or its floor where that is
   lower) widening out by a third of their height, SHELL_THICK outside its
   walls, and a ridged roof SHELL_THICK above its ceiling, open at both
   ends; drawn right after the piece's own faces in the 3D's order (at its
   depth: 0, or a far object's), and only the faces that look at the eye,
   so from outside the tunnel is rock with its mouths in it and from inside
   nothing changes. Shaded as the mesas are. What the 3D draws after the
   piece (a car inside it nearer the eye than it) still shows over its
   rock. */

#define TUNNEL_HIGH 2000.0
#define TUNNEL_WIDE 2000.0
#define SHELL_THICK 256.0
enum { TUNNELS = 64, SHELL_VERTICES = 3 * 8 * 7 };

static struct {
    int course, n;
    struct {
        uint16_t shape;
        double at[3], lo[3], hi[3];
        int along;                      /* the world axis its open ends lie along: 0 x, 2 z */
    } t[TUNNELS];
} tunnels = { -1, 0, { { 0, { 0 }, { 0 }, { 0 }, 0 } } };

static void tunnels_find(unsigned course)
{
    int i;

    tunnels.course = (int)course;
    tunnels.n = 0;
    for (i = 0; i < ncand && tunnels.n < TUNNELS; i++) {
        double w = cand[i].hi[0] - cand[i].lo[0], h = cand[i].hi[1] - cand[i].lo[1], d = cand[i].hi[2] - cand[i].lo[2];
        double roof = rock_survey.facing < 0 ? cand[i].area[0] : cand[i].area[1], side = cand[i].area[2] + cand[i].area[3];

        if (h > TUNNEL_HIGH || w < TUNNEL_WIDE || d < TUNNEL_WIDE || roof < 0.5 * w * d || side < 0.25 * roof)
            continue;
        tunnels.t[tunnels.n].shape = cand[i].shape;
        memcpy(tunnels.t[tunnels.n].at, cand[i].at, sizeof cand[i].at);
        memcpy(tunnels.t[tunnels.n].lo, cand[i].lo, sizeof cand[i].lo);
        memcpy(tunnels.t[tunnels.n].hi, cand[i].hi, sizeof cand[i].hi);
        tunnels.t[tunnels.n].along = cand[i].area[2] >= cand[i].area[3] ? 2 : 0;
        tunnels.n++;
    }
}

static void ground_survey(unsigned course, const double eye[3], const double m[9])
{
    static uint8_t kept[0x10000];
    static uint8_t known[GRID_MOST * GRID_MOST];
    static float next[GRID_MOST * GRID_MOST];
    static int queue[GRID_MOST * GRID_MOST];
    FxState state = fxs;
    uint16_t stack_top = 0x0240;
    unsigned n = fx_ram[GSU_RECORDS_N], far_n = (uint16_t)(fx_ram[FX_FAR_BANK + FX_FAR_COUNT] | fx_ram[FX_FAR_BANK + FX_FAR_COUNT + 1] << 8);
    unsigned i, k;
    double xmin = 1e9, xmax = -1e9, zmin = 1e9, zmax = -1e9;
    int x, z, it, head = 0, tail = 0;

    survey_points = 0;
    ncand = 0;
    surveying = 1;
    memset(&rock_survey, 0, sizeof rock_survey);
    memcpy(kept, fx_ram, sizeof kept);
    fx_draw_hold(1);
    for (i = 0; i < 9; i++) {           /* the identity, as the chip's fractions */
        uint16_t v = (i % 4 == 0) ? 0x7FFF : 0;

        fx_ram[0x00E4 + 2 * i] = (uint8_t)v;
        fx_ram[0x00E4 + 2 * i + 1] = (uint8_t)(v >> 8);
    }
    for (i = 0; i < n && i < 0x31; i++) {   /* the near records, in view space after the sort */
        const uint8_t *r = &kept[0x0AAA + i * 0x1E];
        double v[3] = { (int16_t)(r[0x12] | r[0x13] << 8), (int16_t)(r[0x10] | r[0x11] << 8), (int16_t)(r[0x14] | r[0x15] << 8) }, at[3];

        for (k = 0; k < 3; k++)
            at[k] = eye[k] + m[3 * k] * v[0] + m[3 * k + 1] * v[1] + m[3 * k + 2] * v[2];
        survey_record(r, at, &stack_top);
    }
    if (far_n > FX_FAR_MAX)
        far_n = FX_FAR_MAX;
    for (i = 0; i < far_n; i++) {       /* the far records, from the view in 32 bits */
        uint16_t rec = (uint16_t)(FX_FAR_RECORDS + i * FX_FAR_SIZE);
        const uint8_t *r = &fx_ram[FX_FAR_BANK + rec];
        double at[3];

        for (k = 0; k < 3; k++)
            at[k] = eye[k] + (int32_t)((uint32_t)(r[FX_FAR_PLACE + 4 * k] | r[FX_FAR_PLACE + 4 * k + 1] << 8)
                                       | (uint32_t)(r[FX_FAR_PLACE + 4 * k + 2] | r[FX_FAR_PLACE + 4 * k + 3] << 8) << 16);
        survey_record(r, at, &stack_top);
    }
    fx_draw_hold(0);
    surveying = 0;
    memcpy(fx_ram, kept, sizeof kept);
    fxs = state;
    rock_shades();
    tunnels_find(course);
    for (i = 0; i < (unsigned)survey_points; i++) {
        xmin = fmin(xmin, survey_point[i][0]);
        xmax = fmax(xmax, survey_point[i][0]);
        zmin = fmin(zmin, survey_point[i][2]);
        zmax = fmax(zmax, survey_point[i][2]);
        if (i == 0 || survey_point[i][1] > grid.deepest)
            grid.deepest = survey_point[i][1];
    }
    grid.course = (int)course;
    grid.xmin = xmin;
    grid.xmax = xmax;
    grid.zmin = zmin;
    grid.zmax = zmax;
    /* a lit face's level against its facing, fitted by least squares over
       the course's lit faces by area: level = k (n . light), n as Newell's
       normals point; then turned to outward normals (rock_shades says
       which way Newell's point) */
    grid.light_k = LIGHT_K;
    if (rock_survey.lit_area > 0 && rock_survey.lit_nn > 1e-6 * rock_survey.lit_area) {
        double k = rock_survey.lit_ln / rock_survey.lit_nn;

        if (fabs(k) >= 4 && fabs(k) <= 32)     /* the chip's own is 15.75; else too few faces to tell */
            grid.light_k = rock_survey.facing < 0 ? k : -k;
    }
    if (!survey_points) {
        grid.nx = grid.nz = 0;
        return;
    }
    grid.cell = 512;
    while ((xmax - xmin) / grid.cell + 2 * GRID_MARGIN + 1 > GRID_MOST || (zmax - zmin) / grid.cell + 2 * GRID_MARGIN + 1 > GRID_MOST)
        grid.cell *= 2;
    grid.x0 = floor(xmin / grid.cell) * grid.cell - GRID_MARGIN * grid.cell;
    grid.z0 = floor(zmin / grid.cell) * grid.cell - GRID_MARGIN * grid.cell;
    grid.nx = (int)((xmax - grid.x0) / grid.cell) + GRID_MARGIN + 1;
    grid.nz = (int)((zmax - grid.z0) / grid.cell) + GRID_MARGIN + 1;
    memset(known, 0, sizeof known);
    for (i = 0; i < (unsigned)survey_points; i++) {
        int cx = (int)((survey_point[i][0] - grid.x0) / grid.cell), cz = (int)((survey_point[i][2] - grid.z0) / grid.cell), at;

        if (cx < 0 || cz < 0 || cx >= grid.nx || cz >= grid.nz)
            continue;
        at = cz * grid.nx + cx;
        if (!known[at] || survey_point[i][1] > grid.h[at])      /* the lowest: y is down */
            grid.h[at] = survey_point[i][1];
        if (!known[at])
            queue[tail++] = at;
        known[at] = 1;
    }
    /* the rest: first the nearest known cell's height (spread out from the
       known cells a step at a time), then smoothed, each the mean of its
       neighbours in the grid, the known cells held */
    {
        static uint8_t reached[GRID_MOST * GRID_MOST];

        memcpy(reached, known, sizeof reached);
        while (head < tail) {
            int at = queue[head++], ax = at % grid.nx, az = at / grid.nx, d;
            static const int step[4][2] = { { -1, 0 }, { 1, 0 }, { 0, -1 }, { 0, 1 } };

            for (d = 0; d < 4; d++) {
                int bx = ax + step[d][0], bz = az + step[d][1], b;

                if (bx < 0 || bz < 0 || bx >= grid.nx || bz >= grid.nz || reached[b = bz * grid.nx + bx])
                    continue;
                reached[b] = 1;
                grid.h[b] = grid.h[at];
                queue[tail++] = b;
            }
        }
    }
    for (it = 0; it < 4 * GRID_MOST; it++) {
        for (z = 0; z < grid.nz; z++)
            for (x = 0; x < grid.nx; x++) {
                int at = z * grid.nx + x, count = 0;
                float sum = 0;

                if (known[at]) {
                    next[at] = grid.h[at];
                    continue;
                }
                if (x > 0) { sum += grid.h[at - 1]; count++; }
                if (x < grid.nx - 1) { sum += grid.h[at + 1]; count++; }
                if (z > 0) { sum += grid.h[at - grid.nx]; count++; }
                if (z < grid.nz - 1) { sum += grid.h[at + grid.nx]; count++; }
                next[at] = sum / (float)count;
            }
        memcpy(grid.h, next, sizeof(float) * (size_t)(grid.nx * grid.nz));
    }
    /* the course's ground level: the median of its heights (most of a
       course's grid is its open ground) */
    memcpy(next, grid.h, sizeof(float) * (size_t)(grid.nx * grid.nz));
    qsort(next, (size_t)(grid.nx * grid.nz), sizeof(float), float_order);
    grid.level = next[grid.nx * grid.nz / 2];
    memcpy(grid_known, known, sizeof grid_known);
    fprintf(stderr, "ground: course %u, level %.0f, footprint x %.0f to %.0f, z %.0f to %.0f, lit level %.2f a unit of"
            " facing (%s), %d rock shades, %d tunnel pieces\n", course, grid.level, grid.xmin, grid.xmax, grid.zmin, grid.zmax,
            grid.light_k, grid.light_k == LIGHT_K ? "no lit faces: the chip's own scale" : "fitted from the course's lit faces",
            grid.shades, tunnels.n);
}

/* ---- the land drawn: a few planes, terrain.shade colouring them ----

   The land is 24 triangles: a 3 x 3 lattice of the survey's heights over
   the course's grid (8), and a ring from the lattice's edge out to near the
   horizon at the edge's heights (16), each clipped and projected as the
   water's plane is, drawn farthest first, the course's own polygons over
   them. Each triangle's plane goes to the shader as 1/depth = a x + b y + c
   over the view's pixels, so each pixel's ground point is exact.

   The image (RGBA, T_W across, each texel a word; terrain.shade reads it
   the same way): the header's words (T_*), floats; the kind's 64 x 16 cell
   of marks from word T_CELL; the land's colour for each screen row of the
   view from T_STRIP; rock's from T_ROCK (word T_ROCKAT says where, 0
   none): its shades' colours, darkest first (the palette entries as shown
   now), from word 8 the light's three numbers, the bias and the count of
   shades; the grid's heights from T_GRID, nx a row. Land other than rock is in the band's
   colour on the row a flat ground at the course's level (grid.level) shows
   its depth on, below the 3D's horizon (fx_draw_horizon_now) at the pixel's
   column (race_ground_colour), lit a little by its slope. Rock is in the
   shade the course gives a rock face facing its way, between its shades'
   colours. */

enum {
    T_W = 256, T_CELL = 32, T_STRIP = 1056, T_STRIP_MAX = 2048, T_ROCK = T_STRIP + T_STRIP_MAX, T_GRID = T_ROCK + 16,
    T_H = (T_GRID + GRID_MOST * GRID_MOST + T_W - 1) / T_W,
    T_MATRIX = 0, T_CX = 9, T_CY = 10, T_ROCKAT = 11, T_X0 = 12, T_Z0, T_CELLSIZE, T_NX, T_NZ, T_GRIDAT, T_HZ_SLOPE, T_EYE = 19,
    T_FAR = 22, T_EYE_HEIGHT, T_ROWS, T_FIRST_ROW, T_TEXEL_U, T_TEXEL_V, T_SCALE, T_INK, T_HZ_X, T_HZ_ROW
};

static nshvc_image land_image[4];
static nshvc_shader land_shader;
static uint32_t land_words[T_W * T_H];

/* the ground's height at world (x, z), as terrain.shade takes it: past
   the grid, its edge's */
static double grid_height(double x, double z)
{
    double gx = fmin(fmax((x - grid.x0) / grid.cell - 0.5, 0), grid.nx - 1);
    double gz = fmin(fmax((z - grid.z0) / grid.cell - 0.5, 0), grid.nz - 1), fx, fz;
    int ix, iz, a;

    ix = (int)gx < grid.nx - 2 ? (int)gx : grid.nx - 2;
    iz = (int)gz < grid.nz - 2 ? (int)gz : grid.nz - 2;
    fx = gx - ix;
    fz = gz - iz;
    a = iz * grid.nx + ix;
    return (grid.h[a] * (1 - fx) + grid.h[a + 1] * fx) * (1 - fz) + (grid.h[a + grid.nx] * (1 - fx) + grid.h[a + grid.nx + 1] * fx) * fz;
}

/* The land's triangles in world space, 24 of them (see above). */
enum { LAND_TRIS = 24 };

static int land_triangles(double reach, double t[LAND_TRIS][3][3])
{
    static const int ring[8] = { 0, 1, 2, 5, 8, 7, 6, 3 };       /* the lattice's edge, in order round */
    double lx0 = grid.x0 + 0.5 * grid.cell, lx1 = grid.x0 + (grid.nx - 0.5) * grid.cell;
    double lz0 = grid.z0 + 0.5 * grid.cell, lz1 = grid.z0 + (grid.nz - 0.5) * grid.cell;
    double cx = (lx0 + lx1) / 2, cz = (lz0 + lz1) / 2, pt[9][3], far_[8][3];
    int i, j, k, n = 0;

    for (j = 0; j < 3; j++)
        for (i = 0; i < 3; i++) {
            double x = lx0 + (lx1 - lx0) * i / 2, z = lz0 + (lz1 - lz0) * j / 2;

            pt[j * 3 + i][0] = x;
            pt[j * 3 + i][1] = grid_height(x, z);
            pt[j * 3 + i][2] = z;
        }
    for (k = 0; k < 8; k++) {           /* each edge point carried out from the lattice's middle */
        const double *b = pt[ring[k]];
        double dx = b[0] - cx, dz = b[2] - cz, len = sqrt(dx * dx + dz * dz);

        if (len < 1)
            return 0;
        far_[k][0] = b[0] + dx / len * reach;
        far_[k][1] = b[1];
        far_[k][2] = b[2] + dz / len * reach;
    }
#define TRI(a, b, c) (memcpy(t[n][0], a, sizeof t[n][0]), memcpy(t[n][1], b, sizeof t[n][1]), memcpy(t[n][2], c, sizeof t[n][2]), n++)
    for (j = 0; j < 2; j++)
        for (i = 0; i < 2; i++) {
            const double *a = pt[j * 3 + i], *b = pt[j * 3 + i + 1], *c = pt[(j + 1) * 3 + i], *d = pt[(j + 1) * 3 + i + 1];

            TRI(a, b, d);
            TRI(a, d, c);
        }
    for (k = 0; k < 8; k++) {
        const double *b0 = pt[ring[k]], *b1 = pt[ring[(k + 1) % 8]], *f0 = far_[k], *f1 = far_[(k + 1) % 8];

        TRI(b0, b1, f1);
        TRI(b0, f1, f0);
    }
#undef TRI
    return n;
}

/* The land's height as drawn (its triangles t, n of them) at world (x, z):
   the triangle over the place, else the grid's. Not the grid's where a
   triangle is over it: the grid keeps the lowest place a course's shapes
   meet the ground, so it has a trench where a road runs under the ground
   (a tunnel), which the triangles, as the ground over a tunnel is, do not. */
static double land_height(const double (*t)[3][3], int n, double x, double z)
{
    int i;

    for (i = 0; i < n; i++) {
        const double *a = t[i][0], *b = t[i][1], *c = t[i][2];
        double d = (b[2] - c[2]) * (a[0] - c[0]) + (c[0] - b[0]) * (a[2] - c[2]), l1, l2, l3;

        if (fabs(d) < 1e-9)
            continue;
        l1 = ((b[2] - c[2]) * (x - c[0]) + (c[0] - b[0]) * (z - c[2])) / d;
        l2 = ((c[2] - a[2]) * (x - c[0]) + (a[0] - c[0]) * (z - c[2])) / d;
        l3 = 1 - l1 - l2;
        if (l1 >= -1e-9 && l2 >= -1e-9 && l3 >= -1e-9)
            return l1 * a[1] + l2 * b[1] + l3 * c[1];
    }
    return grid_height(x, z);
}

static void land_draw(int kind, const double eye[3], const double m[9], int p)
{
    double top = (int16_t)ram_word(0x003C), bottom = (int16_t)ram_word(0x003E);
    double vcx = (int16_t)ram_word(0x0034), vcy = (int16_t)ram_word(0x0036);
    double h_eye;
    static double world[LAND_TRIS][3][3], view[LAND_TRIS][3][3];
    static FxCorner c[64];
    double depth[LAND_TRIS];
    int order[LAND_TRIS], ntri;
    float in[8] = { 0 }, sx, sy, scale, hz_x, hz_row, hz_slope;
    unsigned i, x, y, first, rows;
    int slot, ox, oy, t, a, b, k;

    if (!grid.nx)
        return;
    /* the eye over the land as drawn (> 0: above it); not while it is under
       it (a tunnel), where the land would be seen from below */
    ntri = land_triangles(1e7, world);
    h_eye = land_height((const double (*)[3][3])world, ntri, eye[0], eye[2]) - eye[1];
    if (h_eye < 16)
        return;
    fx_draw_world_scale(&sx, &sy);
    scale = sx > sy ? sx : sy;
    memset(land_words, 0, sizeof(uint32_t) * (size_t)(T_GRID + grid.nx * grid.nz));
    for (i = 0; i < 9; i++)
        land_words[T_MATRIX + i] = float_word((float)m[i]);
    land_words[T_CX] = float_word((float)(int16_t)ram_word(0x0034));
    land_words[T_CY] = float_word((float)(int16_t)ram_word(0x0036));
    land_words[T_X0] = float_word((float)grid.x0);
    land_words[T_Z0] = float_word((float)grid.z0);
    land_words[T_CELLSIZE] = float_word((float)grid.cell);
    land_words[T_NX] = float_word((float)grid.nx);
    land_words[T_NZ] = float_word((float)grid.nz);
    land_words[T_GRIDAT] = float_word((float)T_GRID);
    /* the horizon in the view's pixels; a view with none (straight up or
       down), its centre */
    fx_draw_offset(&ox, &oy);
    if (fx_draw_horizon_now(p, &hz_x, &hz_row, &hz_slope)) {
        hz_x -= (float)ox;
        hz_row -= (float)oy;
    } else {
        hz_x = (float)(int16_t)ram_word(0x0034);
        hz_row = (float)(int16_t)ram_word(0x0036);
        hz_slope = 0;
    }
    land_words[T_HZ_X] = float_word(hz_x);
    land_words[T_HZ_ROW] = float_word(hz_row);
    land_words[T_HZ_SLOPE] = float_word(hz_slope);
    for (i = 0; i < 3; i++)
        land_words[T_EYE + i] = float_word((float)eye[i]);
    /* the eye over the course's level: the land's colour for a depth is the
       band's on the row flat ground at that level shows it on, so flat
       ground takes the band's colour of its own row, as the original
       shows it, however high the road runs over the ground */
    land_words[T_EYE_HEIGHT] = float_word((float)fmax(grid.level - eye[1], 16.0));
    first = top > 0 ? (unsigned)top : 0;
    rows = (unsigned)(bottom - first) + 1;
    if (rows > T_STRIP_MAX)
        rows = T_STRIP_MAX;
    for (y = 0; y < rows; y++)
        land_words[T_STRIP + y] = rgb_of(race_ground_colour(p, (float)(first + y + oy)));
    if (kind == SURF_ROCK && grid.shades) {
        for (i = 0; i < (unsigned)grid.shades; i++)
            land_words[T_ROCK + i] = rgb_of(pic.palette[fx_draw_ink(grid.shade[i])]);
        for (i = 0; i < 3; i++)
            land_words[T_ROCK + 8 + i] = float_word((float)grid.light[i]);
        land_words[T_ROCK + 11] = float_word((float)grid.bias);
        land_words[T_ROCK + 12] = float_word((float)grid.shades);
        land_words[T_ROCKAT] = float_word((float)T_ROCK);
    }
    land_words[T_ROWS] = float_word((float)rows);
    land_words[T_FIRST_ROW] = float_word((float)first);
    land_words[T_TEXEL_U] = float_word((float)LAND_TEXEL);
    land_words[T_TEXEL_V] = float_word((float)LAND_TEXEL);
    land_words[T_SCALE] = float_word(1.0f / scale);
    land_words[T_INK] = float_word(land_ink[kind]);
    for (y = 0; y < CELL_V; y++)
        for (x = 0; x < CELL_U; x++)
            if (land_mark(kind, x, y))
                land_words[T_CELL + y * CELL_U + x] = 0xFF000000u;
    for (i = 0; i < (unsigned)(grid.nx * grid.nz); i++)
        land_words[T_GRID + i] = float_word(grid.h[i]);
    slot = fx_draw_tweening() * 2 + (p & 1);
    if (!land_image[slot].id)
        land_image[slot] = nshvc_image_create(T_W, T_H, NSHVC_IMAGE_RGBA);
    if (!land_shader.id)
        land_shader = nshvc_shader_load("terrain");
    nshvc_image_write(land_image[slot], land_words);
    /* the triangles in view space (world axis k turns to row k, as the
       water's plane does), farthest first */
    ntri = land_triangles(128.0 * h_eye * scale / HORIZON_GAP, world);
    for (t = 0; t < ntri; t++) {
        depth[t] = 0;
        for (a = 0; a < 3; a++) {
            double w[3] = { world[t][a][0] - eye[0], world[t][a][1] - eye[1], world[t][a][2] - eye[2] };

            for (k = 0; k < 3; k++)
                view[t][a][k] = w[0] * m[k] + w[1] * m[3 + k] + w[2] * m[6 + k];
            depth[t] += view[t][a][2];
        }
        order[t] = t;
    }
    for (a = 1; a < ntri; a++)
        for (b = a; b > 0 && depth[order[b - 1]] < depth[order[b]]; b--) {
            int s = order[b];

            order[b] = order[b - 1];
            order[b - 1] = s;
        }
    for (a = 0; a < ntri; a++) {
        const double (*v)[3] = (const double (*)[3])view[order[a]];
        double e1[3], e2[3], n[3], d;
        uint32_t nc;

        /* the plane n . V = d; along the ray V = Z ((x - cx) / 128, (y - cy) / 128, 1),
           1/Z = (n . ((x - cx) / 128, (y - cy) / 128, 1)) / d */
        for (k = 0; k < 3; k++) {
            e1[k] = v[1][k] - v[0][k];
            e2[k] = v[2][k] - v[0][k];
        }
        n[0] = e1[1] * e2[2] - e1[2] * e2[1];
        n[1] = e1[2] * e2[0] - e1[0] * e2[2];
        n[2] = e1[0] * e2[1] - e1[1] * e2[0];
        d = n[0] * v[0][0] + n[1] * v[0][1] + n[2] * v[0][2];
        if (fabs(d) < 1e-9)
            continue;                   /* edge on through the eye */
        nc = fx_face_triangles_whole(v, 3, c, sizeof c / sizeof c[0]);
        if (!nc)
            continue;
        in[0] = (float)(n[0] / (128.0 * d));
        in[1] = (float)(n[1] / (128.0 * d));
        in[2] = (float)((n[2] - (n[0] * vcx + n[1] * vcy) / 128.0) / d);
        fx_draw_surface(c, nc, land_image[slot], land_shader, in, 0, 0);
    }
}

/* ---- the land as polygons: a plane to the horizon and a ring of mountains ----

   Jeff (2026-11-11): the ground as clear solid polygons in the style of the
   course's own design, out to the horizon with no gaps. Courses whose
   surface names a shade class (the table's cls; Easy Ride first, the rest
   track by track) draw their land so; the rest as above.

   The ground is one plane at the course's level (grid.level), from under
   the eye out to where it meets the horizon within HORIZON_GAP of an output
   pixel in every direction, so nothing of the backdrop shows below the
   horizon. It is never above the eye: where the eye is lower than
   EYE_CLEAR over the level (a tunnel), it lies EYE_CLEAR under the eye, so
   it is never seen from below. Round the course, unless it is flat (the
   table's flat: Night Owl), stands a ring of MOUNTAINS pyramids of SIDES
   faces, made once a course from the survey's footprint
   (their places, sizes and shapes from a hash of the course and their
   number; their size in proportion to the footprint's), each base wholly
   GAP of the footprint's half size outside it, so no mountain can stand
   between the eye and the course's shapes; their bases on the plane.

   Each face is shaded as the chip shades a lit face of a course's model
   (FxModelMaterial): its light level its normal against the light (fixed in
   view space), floored and kept to 6-15; its colour the byte at that level
   in the class's shade row, the nearest tier's (bank 3, SHADE_ROWS); and
   that byte's colour in the palette (two players' pictures, whose palette
   is in 16-colour rows: the course's own colour, course_day_colour, as one
   player's picture has it; the fixed palette at $1F:8800 is only some
   courses' own, Night Owl's matching it in 48 of its 240 colours). The level's scale and
   sign come from the survey's fit of the course's own lit faces
   (grid.light_k), so the land takes the shades the course's own faces take
   facing the same way.

   Depths (docs/stuntracefx/native/DEPTH.md, 2): the plane lies at the
   median of where the course's shapes meet the ground, so much of the
   course is below it: it is in the ground's depths, behind all of the 3D.
   The mountains and mesas stand outside the course on it, at the world's
   true depths (each corner's own 1 / Z), so they hide each other, and the
   course where they stand in front of it, as they stand. Triangles: the plane's quad (2, at most 6 once kept
   to the view) and MOUNTAINS x SIDES faces (50), those facing away left
   out. No image and no shader: the GPU fills each in its colour. */

enum { MOUNTAINS = 10, SIDES = 5, LAND_VERTICES = 3 * (6 + MOUNTAINS * (SIDES + 1) * 7) };    /* each face kept to the view in at most 7 */
#define SHADE_ROWS 0xA1C5       /* bank 3: the lit faces' shade rows by class, two bytes each, the nearest tier's */
#define EYE_CLEAR 64.0
#define GAP 0.3

static struct {
    int course, mesas;
    double base[MOUNTAINS][SIDES][2];   /* each base corner's x, z (on the plane) */
    double peak[MOUNTAINS][3];          /* a mountain's peak in the world */
    double top[MOUNTAINS][SIDES][3];    /* a mesa's top's corners in the world */
} range = { -1, 0, { { { 0 } } }, { { 0 } }, { { { 0 } } } };

/* a number in [0, 1) from the course, the mountain and which of its numbers */
static double hash01(unsigned course, unsigned i, unsigned k)
{
    uint32_t h = (course + 1) * 0x9E3779B9u ^ (i + 1) * 0x85EBCA6Bu ^ (k + 1) * 0xC2B2AE35u;

    h ^= h >> 16;
    h *= 0x7FEB352Du;
    h ^= h >> 15;
    h *= 0x846CA68Bu;
    h ^= h >> 16;
    return (double)(h & 0xFFFFFF) / 16777216.0;
}

/* how far (x, z) is outside the course's footprint (0 inside) */
static double footprint_distance(double x, double z)
{
    double dx = fmax(fabs(x - (grid.xmin + grid.xmax) / 2) - (grid.xmax - grid.xmin) / 2, 0);
    double dz = fmax(fabs(z - (grid.zmin + grid.zmax) / 2) - (grid.zmax - grid.zmin) / 2, 0);

    return sqrt(dx * dx + dz * dz);
}

/* The ring for the course: mountain i at angle 2 pi i / MOUNTAINS round the
   footprint's middle (give or take a third of a step), its base's mean
   radius 0.55-0.75 of the footprint's larger half size, its peak 0.45-0.75
   of its radius over the plane (faces 25-40 degrees steep), a little off
   its middle; its base's corners
   round it at 0.8-1.15 of the mean radius (each angle give or take 0.3 of
   a step), so its peak stands inside its base and every face looks up. Its
   middle is carried out along its angle until its base, at the most
   1.15 its mean radius, is GAP of the half size clear of the footprint. */
static void range_make(unsigned course, int mesas)
{
    double cx = (grid.xmin + grid.xmax) / 2, cz = (grid.zmin + grid.zmax) / 2;
    double size = fmax(grid.xmax - grid.xmin, grid.zmax - grid.zmin) / 2;
    const double turn = 6.283185307179586;
    int i, k, step;

    range.course = (int)course;
    range.mesas = mesas;
    for (i = 0; i < MOUNTAINS; i++) {
        double angle = turn * (i + 0.6 * (hash01(course, i, 0) - 0.5)) / MOUNTAINS;
        double r = size * (0.55 + 0.2 * hash01(course, i, 1)), height = r * (0.45 + 0.3 * hash01(course, i, 2));
        double clear = 1.15 * r + GAP * size, t = clear, x = cx, z = cz, turned = turn * hash01(course, i, 3);
        double off_angle = turn * hash01(course, i, 4), off = 0.2 * r * hash01(course, i, 5);

        for (step = 0; step < 32; step++) {
            double d;

            x = cx + cos(angle) * t;
            z = cz + sin(angle) * t;
            d = footprint_distance(x, z);
            if (d >= clear - 1)
                break;
            t += clear - d;
        }
        for (k = 0; k < SIDES; k++) {
            double a = turned + turn * (k + 0.6 * (hash01(course, i, 6 + k) - 0.5)) / SIDES;
            double rr = r * (0.8 + 0.35 * hash01(course, i, 6 + SIDES + k));

            range.base[i][k][0] = x + cos(a) * rr;
            range.base[i][k][1] = z + sin(a) * rr;
        }
        range.peak[i][0] = x + cos(off_angle) * off;
        range.peak[i][1] = grid.level - height;         /* y is down */
        range.peak[i][2] = z + sin(off_angle) * off;
        /* a mesa: its top the base drawn in to 0.6-0.75 about its middle and
           a little off it, flat, 0.35-0.55 of the base's radius high, so
           its sides are steep */
        if (mesas) {
            double in = 0.6 + 0.15 * hash01(course, i, 7 + 2 * SIDES), h = r * (0.35 + 0.2 * hash01(course, i, 8 + 2 * SIDES));

            for (k = 0; k < SIDES; k++) {
                range.top[i][k][0] = x + (range.base[i][k][0] - x) * in + cos(off_angle) * off * 0.5;
                range.top[i][k][1] = grid.level - h;
                range.top[i][k][2] = z + (range.base[i][k][1] - z) * in + sin(off_angle) * off * 0.5;
            }
        }
    }
}

/* the colour of a face of class cls whose outward normal is n in view
   space, as the chip shades a lit face */
/* class cls's colour at light level `level` (6-15) */
static uint32_t band_shade(int level, int whole);

static uint32_t row_colour(int cls, int level)
{
    int k = cls == CLS_WHITE ? 0 : cls;
    uint16_t row, bgr;
    uint8_t c;

    if (cls == CLS_BAND || cls == CLS_BAND_WHOLE)       /* the band's own (Lake Side's lake, the battle courses') */
        return band_shade(level, cls == CLS_BAND_WHOLE);
    row = (uint16_t)(fx_rom_read(3, (uint16_t)(SHADE_ROWS + 2 * k)) | fx_rom_read(3, (uint16_t)(SHADE_ROWS + 2 * k + 1)) << 8);
    c = fx_rom_read(3, (uint16_t)(row + level - 6));

    if (fx_draw_full_colour())
        bgr = pic.palette[c];
    else
        bgr = course_day_colour(g.course, c);
    return rgb_of(bgr);
}

/* the light level the chip gives a lit face whose normal in view space is n:
   floor(k (n . the light)), kept to 6-15 */
static int chip_level(const double n[3])
{
    double len = sqrt(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]), l[3], x, level;

    fx_view_light_vector(l);            /* the picture's light (fx_view.h): the chip's own in the game's view */
    x = len > 0 ? (n[0] * l[0] + n[1] * l[1] + n[2] * l[2]) / len : 0;
    level = floor(grid.light_k * x);

    return level < 6 ? 6 : level > 15 ? 15 : (int)level;
}

static uint32_t land_colour(int cls, const double n[3])
{
    return row_colour(cls, chip_level(n));
}

/* a world point to view space (world axis k turns to row k of m) */
static void to_view(const double w[3], const double eye[3], const double m[9], double v[3])
{
    double d[3] = { w[0] - eye[0], w[1] - eye[1], w[2] - eye[2] };
    int k;

    for (k = 0; k < 3; k++)
        v[k] = d[0] * m[k] + d[1] * m[3 + k] + d[2] * m[6 + k];
}

/* A level plane at height y under the eye, in one colour, at its own
   depths in those drawn now (fx_draw_depth_class: the ground's for the
   land's plane, the world's for the sea bed): a quad
   about the eye, far enough out that its far edges lie HORIZON_GAP output
   pixels from the horizon, kept to the view; its vertices into v from *nv. */
static void plane_vertices(double y, uint32_t colour, const double eye[3], const double m[9], nshvc_vertex *v,
                           uint32_t *nv, uint32_t room)
{
    static FxCorner c[64];
    double reach, q[4][3];
    float sx, sy, scale;
    uint32_t nc, j;
    int i;

    if (y - eye[1] <= 0)
        return;
    fx_draw_world_scale(&sx, &sy);
    scale = sx > sy ? sx : sy;
    reach = 128.0 * (y - eye[1]) * scale / HORIZON_GAP;
    for (i = 0; i < 4; i++) {
        double w[3] = { eye[0] + ((i == 1 || i == 2) ? reach : -reach), y, eye[2] + (i >= 2 ? reach : -reach) };

        to_view(w, eye, m, q[i]);
    }
    nc = fx_face_triangles_whole((const double (*)[3])q, 4, c, sizeof c / sizeof c[0]);
    for (j = 0; j < nc && *nv < room; j++, (*nv)++) {
        memset(&v[*nv], 0, sizeof v[*nv]);
        v[*nv].x = c[j].x;
        v[*nv].y = c[j].y;
        v[*nv].z = fx_draw_z(c[j].iz);
        v[*nv].colour = colour;
    }
}

/* A face's colour: class cls's as the chip shades a lit face (by its normal
   in view space, nv: the light is fixed in view space, so a face's shade
   turns with the view). The course's own rock (CLS_ROCK): its colours, the
   survey's run of the shades of its steep faces, darkest first (a canyon's
   rock is painted face by face in them, not lit by the chip); a mesa's face
   takes the one at the chip's light level for it, level 6 the darkest and
   15 the lightest, so its faces are shaded by the way they face as the
   chip's lit faces are (Jeff, 2026-11-11: one colour for every face looked
   wrong; the class of the most steep lit area was a pink one, not the
   rock); the ground (ground) the one the survey's fit of shade against
   facing gives a level face (nw, in the world's axes). With one shade, it;
   with none, the browns of class 11. */
static uint32_t ground_now;             /* the land's plane's colour this picture (0: none yet) */
static uint32_t face_colour(int cls, const double nv[3], const double nw[3], int ground)
{
    if (cls == CLS_ROCK && grid.shades > 1 && !ground) {
        int k = (int)floor((chip_level(nv) - 6) / 9.0 * (grid.shades - 1) + 0.5);

        return rgb_of(pic.palette[fx_draw_ink(grid.shade[k < 0 ? 0 : k >= grid.shades ? grid.shades - 1 : k])]);
    }
    if (cls == CLS_ROCK && grid.shades) {
        double len = sqrt(nw[0] * nw[0] + nw[1] * nw[1] + nw[2] * nw[2]), s;
        int k;

        s = len > 0 ? (grid.light[0] * nw[0] + grid.light[1] * nw[1] + grid.light[2] * nw[2]) / len + grid.bias : grid.bias;
        k = (int)floor(s + 0.5);
        k = k < 0 ? 0 : k >= grid.shades ? grid.shades - 1 : k;
        return rgb_of(pic.palette[fx_draw_ink(grid.shade[k])]);
    }
    if (cls == CLS_BAND_WHOLE && !ground && surface[g.course < 0x20 ? g.course : 0].mesas && ground_now) {
        /* a battle course's mesa (the red-rock desert): the two players'
           palettes hold no shade rows and the band a view shows is nearly
           one colour, so each face came out the ground's flat brown; the
           ground's colour instead, its tops as it is, its walls darker by
           a sun fixed in the world (its sides each their own shade: the
           chip's light, fixed to the view, gave walls facing the eye the
           ground's own level, so they showed no shade) */
        static const double sun[3] = { 0.7, -0.5, 0.5 };    /* toward it; world y is down */
        double len = sqrt(nw[0] * nw[0] + nw[1] * nw[1] + nw[2] * nw[2]);
        double d = len > 0 ? (nw[0] * sun[0] + nw[1] * sun[1] + nw[2] * sun[2]) / (len * sqrt(0.99)) : 0;
        double f = len > 0 && nw[1] / len < -0.99 ? 1.0 : 0.72 + 0.22 * d;    /* (nw not of unit length) */
        uint32_t out = 0xFF000000u;
        int k;

        (void)nv;
        for (k = 0; k < 24; k += 8) {
            double c = (ground_now >> k & 255) * f;

            out |= (uint32_t)(c > 255 ? 255 : c) << k;
        }
        return out;
    }
    return land_colour(cls == CLS_ROCK ? 11 : cls, nv);
}

/* One flat face of the land, its corners pts (world, in order round it)
   and its outward normal nw: left out when it faces away from the eye;
   else turned to the view, kept to it, each corner at its depth (from the
   face's own 1 / Z, the world's true depth), into v from *nv. */

static void land_face(const double (*pts)[3], int n, const double nw[3], int cls, const double eye[3], const double m[9],
                      nshvc_vertex *v, uint32_t *nv, uint32_t room)
{
    static FxCorner c[64];
    double vp[8][3], nview[3], d;
    uint32_t nc, j, colour;
    int k;

    if (n > 8 || nw[0] * (eye[0] - pts[0][0]) + nw[1] * (eye[1] - pts[0][1]) + nw[2] * (eye[2] - pts[0][2]) <= 0)
        return;
    for (k = 0; k < n; k++)
        to_view(pts[k], eye, m, vp[k]);
    for (k = 0; k < 3; k++)
        nview[k] = nw[0] * m[k] + nw[1] * m[3 + k] + nw[2] * m[6 + k];
    /* the face's plane nview . V = d: none edge on through the eye */
    d = nview[0] * vp[0][0] + nview[1] * vp[0][1] + nview[2] * vp[0][2];
    if (fabs(d) < 1e-9)
        return;
    nc = fx_face_triangles_whole((const double (*)[3])vp, n, c, sizeof c / sizeof c[0]);
    if (!nc || *nv + nc > room)
        return;
    colour = face_colour(cls, nview, nw, 0);
    for (j = 0; j < nc; j++, (*nv)++) {
        memset(&v[*nv], 0, sizeof v[*nv]);
        v[*nv].x = c[j].x;
        v[*nv].y = c[j].y;
        v[*nv].z = fx_draw_z(c[j].iz);
        v[*nv].colour = colour;
    }
}

/* the normal of the plane through a, b and c ((b - a) x (c - a)) */
static void normal_of(const double *a, const double *b, const double *c, double n[3])
{
    double e1[3] = { b[0] - a[0], b[1] - a[1], b[2] - a[2] }, e2[3] = { c[0] - a[0], c[1] - a[1], c[2] - a[2] };

    n[0] = e1[1] * e2[2] - e1[2] * e2[1];
    n[1] = e1[2] * e2[0] - e1[0] * e2[2];
    n[2] = e1[0] * e2[1] - e1[1] * e2[0];
}

/* ---- the land filling a course's empty space (Jeff, 2026-11-11) ----

   Where nothing of the course stands, rock rises, so the far track and a
   tunnel's open sides are not seen across empty ground: heights on a
   lattice over the survey's grid, FILL_STEP of its cells a step, each point
   rising with its distance from the nearest cell where a shape of the course
   meets the ground: flat within FILL_CLEAR steps of one, rising over the
   next FILL_RISE steps to FILL_HEIGHT (give or take a quarter, by a hash).
   A lattice triangle over a cell of the course has its three corners within
   a step and a half of it (its longest side is the square's diagonal), so
   with FILL_CLEAR at 1.5 it is flat at the course's level there: the rock
   never stands over the course. Each triangle with a raised corner is a
   face, shaded as the mesas are; those all at the level are the plane's.
   At the world's true depths, it hides what lies behind it. */

enum { FILL_STEP = 4, FILL_MOST = GRID_MOST / FILL_STEP + 2, FILL_VERTICES = 3 * 2 * FILL_MOST * FILL_MOST * 2 };
#define FILL_CLEAR 1.5
#define FILL_RISE 3.0
#define FILL_HEIGHT 3500.0

static struct {
    int course, nx, nz;
    double x0, z0, step;
    float y[FILL_MOST * FILL_MOST];     /* each lattice point's height (y is down) */
    uint8_t raised[FILL_MOST * FILL_MOST];
} fill = { -1, 0, 0, 0, 0, 0, { 0 }, { 0 } };

static void fill_make(unsigned course)
{
    double clear, rise;
    int i, j, reach;

    fill.course = (int)course;
    fill.step = FILL_STEP * grid.cell;
    fill.x0 = grid.x0;
    fill.z0 = grid.z0;
    fill.nx = grid.nx / FILL_STEP + 2;
    fill.nz = grid.nz / FILL_STEP + 2;
    clear = FILL_CLEAR * fill.step;
    rise = FILL_RISE * fill.step;
    reach = (int)ceil((clear + rise) / grid.cell) + 1;
    for (j = 0; j < fill.nz; j++)
        for (i = 0; i < fill.nx; i++) {
            double px = fill.x0 + i * fill.step, pz = fill.z0 + j * fill.step, best = clear + rise, t, h;
            int cx = (int)floor((px - grid.x0) / grid.cell), cz = (int)floor((pz - grid.z0) / grid.cell), x, z;

            for (z = cz - reach; z <= cz + reach; z++)
                for (x = cx - reach; x <= cx + reach; x++) {
                    double dx, dz, d;

                    if (x < 0 || z < 0 || x >= grid.nx || z >= grid.nz || !grid_known[z * grid.nx + x])
                        continue;
                    dx = grid.x0 + (x + 0.5) * grid.cell - px;
                    dz = grid.z0 + (z + 0.5) * grid.cell - pz;
                    d = sqrt(dx * dx + dz * dz);
                    if (d < best)
                        best = d;
                }
            t = (best - clear) / rise;
            t = t < 0 ? 0 : t > 1 ? 1 : t;
            h = FILL_HEIGHT * (0.75 + 0.5 * hash01(course, 1000 + (unsigned)i, (unsigned)j)) * t * t * (3 - 2 * t);
            fill.y[j * fill.nx + i] = (float)(grid.level - h);
            fill.raised[j * fill.nx + i] = h > 0;
        }
}

/* the fill's faces into v from *nv */
static void fill_faces(unsigned course, int cls, double plane_y, const double eye[3], const double m[9], nshvc_vertex *v,
                       uint32_t *nv, uint32_t room)
{
    int i, j, t, k;

    for (j = 0; j + 1 < fill.nz; j++)
        for (i = 0; i + 1 < fill.nx; i++) {
            int at[4] = { j * fill.nx + i, j * fill.nx + i + 1, (j + 1) * fill.nx + i + 1, (j + 1) * fill.nx + i };
            static const int split[2][2][3] = { { { 0, 1, 2 }, { 0, 2, 3 } }, { { 0, 1, 3 }, { 1, 2, 3 } } };
            int diag = hash01(course, 2000 + (unsigned)i, (unsigned)j) < 0.5;
            double c[4][3];

            if (!fill.raised[at[0]] && !fill.raised[at[1]] && !fill.raised[at[2]] && !fill.raised[at[3]])
                continue;
            for (k = 0; k < 4; k++) {
                int ix = i + (k == 1 || k == 2), iz = j + (k >= 2);

                c[k][0] = fill.x0 + ix * fill.step;
                c[k][1] = fill.raised[at[k]] ? fill.y[at[k]] : plane_y;
                c[k][2] = fill.z0 + iz * fill.step;
            }
            for (t = 0; t < 2; t++) {
                const int *s = split[diag][t];
                double tri[3][3], n[3];

                if (!fill.raised[at[s[0]]] && !fill.raised[at[s[1]]] && !fill.raised[at[s[2]]])
                    continue;
                for (k = 0; k < 3; k++)
                    memcpy(tri[k], c[s[k]], sizeof tri[k]);
                normal_of(tri[0], tri[1], tri[2], n);
                if (n[1] > 0)                   /* outward: up (y is down) */
                    for (k = 0; k < 3; k++)
                        n[k] = -n[k];
                land_face((const double (*)[3])tri, 3, n, cls, eye, m, v, nv, room);
            }
        }
}

/* The view the tunnels' rock is drawn in this picture (land_polygons sets
   it; off where the course has none), and the tunnel piece being drawn. */
static struct {
    int on, cls;
    double eye[3], m[9], plane_y;
} shell_view;
static int shell_pending = -1;

void fx_ground_object_begin(uint16_t record, int shift)
{
    uint16_t shape;
    double v[3], at[3];
    int i, k;

    shell_pending = -1;
    if (!shell_view.on || surveying || tunnels.course != (int)g.course)
        return;
    shape = (uint16_t)(fx_ram[(uint16_t)(record + 0x08)] | fx_ram[(uint16_t)(record + 0x09)] << 8);
    for (i = 0; i < tunnels.n; i++)
        if (tunnels.t[i].shape == shape)
            break;
    if (i == tunnels.n)
        return;
    /* its place in the world: its place in view space (halved `shift`
       times for a far one) turned back by the view's rows */
    v[0] = ldexp((int16_t)(fx_ram[(uint16_t)(record + 0x12)] | fx_ram[(uint16_t)(record + 0x13)] << 8), shift);
    v[1] = ldexp((int16_t)(fx_ram[(uint16_t)(record + 0x10)] | fx_ram[(uint16_t)(record + 0x11)] << 8), shift);
    v[2] = ldexp((int16_t)(fx_ram[(uint16_t)(record + 0x14)] | fx_ram[(uint16_t)(record + 0x15)] << 8), shift);
    for (k = 0; k < 3; k++)
        at[k] = shell_view.eye[k] + shell_view.m[3 * k] * v[0] + shell_view.m[3 * k + 1] * v[1] + shell_view.m[3 * k + 2] * v[2];
    for (; i < tunnels.n; i++) {
        double dx = at[0] - tunnels.t[i].at[0], dz = at[2] - tunnels.t[i].at[2];

        if (tunnels.t[i].shape == shape && dx * dx + dz * dz < 512.0 * 512.0 * (1 << (2 * shift)))
            break;
    }
    if (i < tunnels.n)
        shell_pending = i;
}

void fx_ground_object_end(void)
{
    static nshvc_vertex sv[SHELL_VERTICES];
    uint32_t ns = 0;
    double p0, p1, cp, half, a0, a1, top, base, ridge, spread, mid_y;
    double prof[5][2];
    int i = shell_pending, s, p, a, k;

    if (i < 0)
        return;
    shell_pending = -1;
    fx_draw_queued();                   /* the piece's own faces first */
    fx_draw_piece(FX_PIECE_ROCK + (uint32_t)i);     /* the rock a piece of the ground of its own (fx_draw.c) */
    a = tunnels.t[i].along;
    p = 2 - a;
    p0 = tunnels.t[i].lo[p];
    p1 = tunnels.t[i].hi[p];
    cp = (p0 + p1) / 2;
    half = (p1 - p0) / 2;
    a0 = tunnels.t[i].lo[a];
    a1 = tunnels.t[i].hi[a];
    top = tunnels.t[i].lo[1] - SHELL_THICK;                     /* y is down */
    base = fmax(tunnels.t[i].hi[1], shell_view.plane_y);
    ridge = top - 0.3 * half;
    spread = (base - top) / 3;
    mid_y = (tunnels.t[i].lo[1] + tunnels.t[i].hi[1]) / 2;
    /* its cross-section across the tunnel: ground, wall top, ridge, wall
       top, ground */
    prof[0][0] = cp - half - SHELL_THICK - spread;
    prof[0][1] = base;
    prof[1][0] = cp - half - SHELL_THICK;
    prof[1][1] = top;
    prof[2][0] = cp;
    prof[2][1] = ridge;
    prof[3][0] = cp + half + SHELL_THICK;
    prof[3][1] = top;
    prof[4][0] = cp + half + SHELL_THICK + spread;
    prof[4][1] = base;
    for (s = 0; s < 4; s++) {
        double quad[4][3], n[3], mp = (prof[s][0] + prof[s + 1][0]) / 2 - cp, my = (prof[s][1] + prof[s + 1][1]) / 2 - mid_y;
        static const int end[4] = { 0, 0, 1, 1 }, edge[4] = { 0, 1, 1, 0 };

        for (k = 0; k < 4; k++) {
            quad[k][p] = prof[s + edge[k]][0];
            quad[k][1] = prof[s + edge[k]][1];
            quad[k][a] = end[k] ? a1 : a0;
        }
        normal_of(quad[0], quad[1], quad[2], n);
        if (n[p] * mp + n[1] * my < 0)      /* outward: away from the tunnel's middle */
            for (k = 0; k < 3; k++)
                n[k] = -n[k];
        land_face((const double (*)[3])quad, 4, n, shell_view.cls, shell_view.eye, shell_view.m, sv, &ns, SHELL_VERTICES);
    }
    fx_draw_land(sv, ns);
    fx_draw_piece(0);
}

/* ---- the forest: the course's own trees round it ----

   Jeff (2026-11-12, King's Forest): "It says 'forest' so you should place
   a ton of the trees around the terrain too." The trees are the course's
   own, copied (fx_solid_forest_tree): none until the race has drawn one
   of its trees near, trunk and canopy together.

   They stand only outside the course's footprint, their feet at least
   FOREST_CLEAR beyond it past their canopies' half width, so none stands
   on the course or, from the game's camera (in the footprint's box: it
   trails the car by far less than FOREST_CLEAR, and no line from a point
   in a box to another leaves it), between the eye and it. On a lattice a canopy's width a step, each place moved
   by up to FOREST_JITTER of a step (a hash of the course and the place),
   left out where the groves' noise (value noise on a lattice of
   FOREST_GROVE steps) is under FOREST_OPEN, a clearing; of those the
   FOREST_MOST nearest the footprint are kept, so the forest is a belt as
   deep as that many trees make it; each 0.85 to 1.2 the course's tree's
   size, its foot on the ground (the plane's level, or a mountain's face
   over it, the highest) sunk by its trunk's half width, so none stands on
   a slope with its foot in the air.

   Drawn after the plane and the mountains as instanced meshes (nshvc
   6.2, fx_solid_forest_*), every tree the course's 3D tree at the detail
   its size asks, placed and projected on the GPU at the world's true
   depths (exact per pixel), so they and the mountains hide what stands
   behind them, and the plane, in the ground's depths, lies behind them all; in FOREST_BLOCK x
   FOREST_BLOCK lattice blocks, a block left out whole where its sphere is
   behind the eye or past FOREST_WIDE of the view's sides. One player's
   pictures only (two players' keep the course's trees as sprites, and so
   the land none). */

enum { FOREST_MOST = 3000, FOREST_BLOCK = 8 };
#define FOREST_CLEAR 1024.0
#define FOREST_JITTER 0.3
#define FOREST_GROVE 6.0
#define FOREST_OPEN 0.3
#define FOREST_WIDE 3.0

static struct {
    int course, n, blocks;
    double step;
    float site[FOREST_MOST][4];         /* each tree's foot (x, y, z) and size */
    struct {
        int first, n;
        double mid[3], r;
    } block[FOREST_MOST];
    FxForestShape shape;                /* the course's tree's, at its own size */
} forest = { -1, 0, 0, 0, { { 0 } }, { { 0, 0, { 0 }, 0 } }, { 0, 0, 0, 0, 0 } };

typedef struct {
    float x, z, d;
    int bx, bz;
} ForestPlace;

static int forest_place_near(const void *a, const void *b)
{
    float x = ((const ForestPlace *)a)->d, y = ((const ForestPlace *)b)->d;

    return x < y ? -1 : x > y;
}

static int forest_place_block(const void *a, const void *b)
{
    const ForestPlace *p = a, *q = b;

    return p->bz != q->bz ? (p->bz < q->bz ? -1 : 1) : p->bx != q->bx ? (p->bx < q->bx ? -1 : 1) : 0;
}

/* the groves' noise at lattice place (i, j): smooth, 0 to 1 */
static double forest_noise(unsigned course, double i, double j)
{
    double gi = floor(i / FOREST_GROVE), gj = floor(j / FOREST_GROVE), fi = i / FOREST_GROVE - gi, fj = j / FOREST_GROVE - gj;
    double a, b, c, d;
    unsigned ui = (unsigned)(int)gi + 0x8000u, uj = (unsigned)(int)gj + 0x8000u;

    fi = fi * fi * (3 - 2 * fi);
    fj = fj * fj * (3 - 2 * fj);
    a = hash01(course, 3000 + ui, uj);
    b = hash01(course, 3000 + ui + 1, uj);
    c = hash01(course, 3000 + ui, uj + 1);
    d = hash01(course, 3000 + ui + 1, uj + 1);
    return (a + (b - a) * fi) + ((c + (d - c) * fi) - (a + (b - a) * fi)) * fj;
}

/* the ground's height at (x, z): the highest mountain face over it, else
   the plane's level (y is down) */
static double forest_ground(double x, double z)
{
    double y = grid.level;
    int i, k;

    if (range.mesas)
        return y;
    for (i = 0; i < MOUNTAINS; i++)
        for (k = 0; k < SIDES; k++) {
            const double *p = range.peak[i], *b0 = range.base[i][k], *b1 = range.base[i][(k + 1) % SIDES];
            double v0x = b0[0] - p[0], v0z = b0[1] - p[2], v1x = b1[0] - p[0], v1z = b1[1] - p[2];
            double qx = x - p[0], qz = z - p[2], det = v0x * v1z - v1x * v0z, a, b, h;

            if (fabs(det) < 1e-9)
                continue;
            a = (qx * v1z - v1x * qz) / det;
            b = (v0x * qz - qx * v0z) / det;
            if (a < 0 || b < 0 || a + b > 1)
                continue;
            h = p[1] + (a + b) * (grid.level - p[1]);
            if (h < y)
                y = h;
        }
    return y;
}

static void forest_make(unsigned course)
{
    float half, height, trunk_half;
    double clear, area, perimeter, depth, x0, z0;
    int nx, nz, i, j, n = 0, b;
    ForestPlace *place;

    forest.course = (int)course;
    forest.n = forest.blocks = 0;
    fx_solid_forest_shape(&forest.shape);
    half = forest.shape.half, height = forest.shape.height, trunk_half = forest.shape.trunk_half;
    if (half <= 0 || height <= 0)
        return;
    forest.step = 2 * half;
    clear = FOREST_CLEAR + 1.2 * half;
    /* the belt the kept trees fill: FOREST_MOST of them, a step's square
       each, 1 - FOREST_OPEN of them kept; its depth past the clearing, and
       the lattice out half as far again */
    perimeter = 2 * ((grid.xmax - grid.xmin) + (grid.zmax - grid.zmin));
    area = FOREST_MOST * forest.step * forest.step / (1 - FOREST_OPEN);
    depth = (-perimeter + sqrt(perimeter * perimeter + 4 * 3.141592653589793 * area)) / (2 * 3.141592653589793);
    depth = clear + 1.5 * depth + forest.step;
    while ((grid.xmax - grid.xmin + 2 * depth) / forest.step * (grid.zmax - grid.zmin + 2 * depth) / forest.step > 2e6)
        forest.step *= 1.25;            /* a lattice of at most two million places */
    x0 = grid.xmin - depth;
    z0 = grid.zmin - depth;
    nx = (int)((grid.xmax - grid.xmin + 2 * depth) / forest.step) + 1;
    nz = (int)((grid.zmax - grid.zmin + 2 * depth) / forest.step) + 1;
    if (!(place = malloc(sizeof *place * (size_t)nx * (size_t)nz)))
        return;
    for (j = 0; j < nz; j++)
        for (i = 0; i < nx; i++) {
            unsigned ui = (unsigned)i, uj = (unsigned)j;
            double x = x0 + (i + FOREST_JITTER * 2 * (hash01(course, 4000 + ui, uj) - 0.5)) * forest.step;
            double z = z0 + (j + FOREST_JITTER * 2 * (hash01(course, 5000 + ui, uj) - 0.5)) * forest.step;
            double d = footprint_distance(x, z);

            if (d < clear || forest_noise(course, i, j) < FOREST_OPEN)
                continue;
            place[n].x = (float)x, place[n].z = (float)z, place[n].d = (float)d;
            place[n].bx = i / FOREST_BLOCK, place[n].bz = j / FOREST_BLOCK;
            n++;
        }
    qsort(place, (size_t)n, sizeof *place, forest_place_near);
    if (n > FOREST_MOST)
        n = FOREST_MOST;
    qsort(place, (size_t)n, sizeof *place, forest_place_block);
    for (i = 0; i < n; i++) {
        double k = 0.85 + 0.35 * hash01(course, 6000 + (unsigned)(place[i].x / forest.step + 0x8000), (unsigned)(place[i].z / forest.step + 0x8000));

        forest.site[i][0] = place[i].x;
        forest.site[i][1] = (float)(forest_ground(place[i].x, place[i].z) + trunk_half * k);
        forest.site[i][2] = place[i].z;
        forest.site[i][3] = (float)k;
        if (!i || place[i].bx != place[i - 1].bx || place[i].bz != place[i - 1].bz) {
            forest.block[forest.blocks].first = i;
            forest.block[forest.blocks].n = 0;
            forest.blocks++;
        }
        forest.block[forest.blocks - 1].n++;
    }
    forest.n = n;
    free(place);
    for (b = 0; b < forest.blocks; b++) {       /* each block's sphere, its trees whole */
        double *mid = forest.block[b].mid, r = 0;
        int first = forest.block[b].first, count = forest.block[b].n;

        mid[0] = mid[1] = mid[2] = 0;
        for (i = first; i < first + count; i++) {
            mid[0] += forest.site[i][0] / count;
            mid[1] += (forest.site[i][1] - height * forest.site[i][3] / 2) / count;
            mid[2] += forest.site[i][2] / count;
        }
        for (i = first; i < first + count; i++) {
            double dx = forest.site[i][0] - mid[0], dy = forest.site[i][1] - mid[1], dz = forest.site[i][2] - mid[2];
            double e = sqrt(dx * dx + dy * dy + dz * dz) + (height + half) * forest.site[i][3];

            if (e > r)
                r = e;
        }
        forest.block[b].r = r;
    }
    fprintf(stderr, "forest: course %u, %d trees in %d blocks, a step %.0f (the course's tree %.0f across, %.0f high)\n", course,
            forest.n, forest.blocks, forest.step, 2.0 * half, (double)height);
}

/* ---- the trees the mountains hide ----

   A tree wholly behind a mountain from the eye is not handed to the GPU
   (Jeff, 2026-11-12: "heavily optimize"). A mountain is convex (a pyramid
   on the plane: its SIDES faces and its base), so a point is hidden by it
   when the segment from the eye to the point passes inside it: inside all
   its faces' planes at some place between the two (a point inside it, a
   foot sunk in its slope, is hidden too). A tree is hidden when every one
   of FOREST_SAMPLES points of its outline as the eye sees it is: its
   canopy's outline (an ellipse of its half width across the view and its
   half height up it, eight points, 1.1 times out for its points' jag),
   its top, and its trunk's foot either side; a mountain is tried for a
   point only where the point lies in the box its corners make on the
   view (all in front of the eye) and past its nearest corner. */
#define FOREST_SAMPLES 11

static struct {
    int ok, boxed;
    double n[SIDES + 1][3], c[SIDES + 1];       /* its faces' planes, outward: n . x <= c inside */
    double lo[2], hi[2], nearest;               /* its corners' X / Z and Y / Z on the view, its nearest Z */
} hill[MOUNTAINS];

static void hills_make(const double eye[3], const double m[9], double plane_y)
{
    int i, k, j;

    for (i = 0; i < MOUNTAINS; i++) {
        double corner[SIDES + 1][3], mid[3] = { 0, 0, 0 };

        hill[i].ok = !range.mesas && range.course == forest.course;
        if (!hill[i].ok)
            continue;
        memcpy(corner[SIDES], range.peak[i], sizeof corner[SIDES]);
        for (k = 0; k < SIDES; k++) {
            corner[k][0] = range.base[i][k][0], corner[k][1] = plane_y, corner[k][2] = range.base[i][k][1];
            for (j = 0; j < 3; j++)
                mid[j] += corner[k][j] / SIDES;
        }
        mid[1] = (plane_y + range.peak[i][1]) / 2;
        for (k = 0; k < SIDES; k++) {
            const double *a = corner[SIDES], *b = corner[k], *c = corner[(k + 1) % SIDES];
            double *n = hill[i].n[k], f[3];

            normal_of(a, b, c, n);
            for (j = 0; j < 3; j++)
                f[j] = (a[j] + b[j] + c[j]) / 3 - mid[j];
            if (n[0] * f[0] + n[1] * f[1] + n[2] * f[2] < 0)
                for (j = 0; j < 3; j++)
                    n[j] = -n[j];
            hill[i].c[k] = n[0] * a[0] + n[1] * a[1] + n[2] * a[2];
        }
        hill[i].n[SIDES][0] = 0, hill[i].n[SIDES][1] = 1, hill[i].n[SIDES][2] = 0;     /* its base, facing down */
        hill[i].c[SIDES] = plane_y;
        hill[i].boxed = 1;
        hill[i].nearest = 1e30;
        hill[i].lo[0] = hill[i].lo[1] = 1e30;
        hill[i].hi[0] = hill[i].hi[1] = -1e30;
        for (k = 0; k <= SIDES; k++) {
            double v[3];

            to_view(corner[k], eye, m, v);
            if (v[2] <= 1) {
                hill[i].boxed = 0;
                break;
            }
            for (j = 0; j < 2; j++) {
                if (v[j] / v[2] < hill[i].lo[j])
                    hill[i].lo[j] = v[j] / v[2];
                if (v[j] / v[2] > hill[i].hi[j])
                    hill[i].hi[j] = v[j] / v[2];
            }
            if (v[2] < hill[i].nearest)
                hill[i].nearest = v[2];
        }
    }
}

/* whether mountain i hides world point p from the eye */
static int hill_hides(int i, const double eye[3], const double p[3])
{
    double d[3] = { p[0] - eye[0], p[1] - eye[1], p[2] - eye[2] }, t_in = 0, t_out = 1;
    int f;

    for (f = 0; f <= SIDES; f++) {
        const double *n = hill[i].n[f];
        double num = hill[i].c[f] - (n[0] * eye[0] + n[1] * eye[1] + n[2] * eye[2]), den = n[0] * d[0] + n[1] * d[1] + n[2] * d[2];

        if (fabs(den) < 1e-12) {
            if (num < 0)
                return 0;
            continue;
        }
        if (den > 0) {
            if (num / den < t_out)
                t_out = num / den;
        } else if (num / den > t_in) {
            t_in = num / den;
        }
        if (t_in >= t_out)
            return 0;
    }
    return 1;
}

/* whether the mountains hide the tree whose foot is at world point foot, k
   times the course's tree's size */
static int forest_hidden(const double eye[3], const double m[9], const double foot[3], double k)
{
    const FxForestShape *sh = &forest.shape;
    double p[FOREST_SAMPLES][3], c[3];
    int s, i, j;

    for (j = 0; j < 3; j++)             /* the canopy's middle; the view's across (row 0) and down (row 1) in the world */
        c[j] = foot[j] - (j == 1 ? sh->canopy_rise * k : 0);
    for (s = 0; s < 8; s++) {
        double a = 6.283185307179586 * s / 8, x = 1.1 * sh->half * k * cos(a), y = 1.1 * sh->canopy_half_height * k * sin(a);

        for (j = 0; j < 3; j++)
            p[s][j] = c[j] + x * m[3 * j] + y * m[3 * j + 1];
    }
    for (j = 0; j < 3; j++) {
        p[8][j] = foot[j] - (j == 1 ? sh->height * k : 0);
        p[9][j] = foot[j] - sh->trunk_half * k * m[3 * j];
        p[10][j] = foot[j] + sh->trunk_half * k * m[3 * j];
    }
    for (s = 0; s < FOREST_SAMPLES; s++) {
        double v[3];
        int hidden = 0;

        to_view(p[s], eye, m, v);
        for (i = 0; i < MOUNTAINS && !hidden; i++) {
            if (!hill[i].ok)
                continue;
            if (hill[i].boxed && (v[2] <= hill[i].nearest || v[0] / v[2] < hill[i].lo[0] || v[0] / v[2] > hill[i].hi[0]
                                  || v[1] / v[2] < hill[i].lo[1] || v[1] / v[2] > hill[i].hi[1]))
                continue;
            hidden = hill_hides(i, eye, p[s]);
        }
        if (!hidden)
            return 0;
    }
    return 1;
}

/* A land mesh's view's depths: the world's true depths (fx_draw_z: z =
   near / Z, 1 nearest), no nearer than the view's near. */
static void view_depth(nshvc_view *view)
{
    view->depth_a = 0;
    view->depth_b = -fx_draw_z(1.0);
    view->depth_max = fx_draw_z(1.0 / view->near);
}

static void forest_draw(unsigned course, const double eye[3], const double m[9], double plane_y)
{
    nshvc_view view;
    float left, right, top, bottom;
    int b, i, k;

    if (forest.course != (int)course)
        forest_make(course);
    /* the picture's view: world to view (world axis i is the view's row i
       of m), its centre and 128 a unit in the picture's pixels, the
       world's depths (view_depth) */
    memset(&view, 0, sizeof view);
    for (i = 0; i < 3; i++) {
        view.eye[i] = (float)eye[i];
        for (k = 0; k < 3; k++)
            view.rot[3 * k + i] = (float)m[3 * i + k];
    }
    view.f = 128;
    view.cx = (int16_t)ram_word(0x0034);
    view.cy = (int16_t)ram_word(0x0036);
    view.near = 1;
    view_depth(&view);
    fx_face_view_rect(&left, &right, &top, &bottom);
    fx_solid_forest_begin(&view, left, right, top, bottom);
    hills_make(eye, m, plane_y);
    for (b = 0; b < forest.blocks; b++) {
        double v[3], r = forest.block[b].r;

        to_view(forest.block[b].mid, eye, m, v);
        if (v[2] < -r || fabs(v[0]) > v[2] * FOREST_WIDE + r || fabs(v[1]) > v[2] * FOREST_WIDE + r)
            continue;
        for (i = forest.block[b].first; i < forest.block[b].first + forest.block[b].n; i++) {
            double foot[3] = { forest.site[i][0], forest.site[i][1], forest.site[i][2] };

            if (!forest_hidden(eye, m, foot, forest.site[i][3]))
                fx_solid_forest_add(foot, forest.site[i][3]);
        }
    }
    fx_solid_forest_end();
}

/* ---- the land's view for instanced meshes ----

   The picture's view as nshvc 6.2's meshes take it: world to view (world
   axis i is the view's row i of m), its centre and 128 a unit in the
   picture's pixels, the world's depths (view_depth); and the view's sides
   in view space, inward, unit, for
   culling a sphere (land_sphere_seen). */
static void land_view(const double eye[3], const double m[9], nshvc_view *view, float side[4][3])
{
    float left, right, top, bottom;
    int i, k;

    memset(view, 0, sizeof *view);
    for (i = 0; i < 3; i++) {
        view->eye[i] = (float)eye[i];
        for (k = 0; k < 3; k++)
            view->rot[3 * k + i] = (float)m[3 * i + k];
    }
    view->f = 128;
    view->cx = (int16_t)ram_word(0x0034);
    view->cy = (int16_t)ram_word(0x0036);
    view->near = 1;
    view_depth(view);
    fx_face_view_rect(&left, &right, &top, &bottom);
    {
        float p[4][3] = { { 128, 0, -(left - view->cx) }, { -128, 0, right - view->cx },
                          { 0, 128, -(top - view->cy) }, { 0, -128, bottom - view->cy } };

        for (i = 0; i < 4; i++) {
            float l = sqrtf(p[i][0] * p[i][0] + p[i][1] * p[i][1] + p[i][2] * p[i][2]);

            for (k = 0; k < 3; k++)
                side[i][k] = l > 0 ? p[i][k] / l : 0;
        }
    }
}

/* whether a sphere (world centre c, radius r) may be seen: its view-space
   centre into v */
static int land_sphere_seen(const double eye[3], const double m[9], const float side[4][3], const double c[3], double r,
                            double v[3])
{
    int i;

    to_view(c, eye, m, v);
    if (v[2] < 1 - r)
        return 0;
    for (i = 0; i < 4; i++)
        if (side[i][0] * v[0] + side[i][1] * v[1] + side[i][2] * v[2] < -r)
            return 0;
    return 1;
}

/* ---- the city ----

   Jeff (2026-11-12, Night Cruise: its ground "should read as a city
   scape. Look at the skymap for the track"): its backdrop's skyline is
   towers of near black and dark brown with grids of lit windows, in its
   backdrop's colours 5-10 (5-8 the walls, 080000 to 211010; 9 290800; 10
   the windows, 7b5a21 by day and bd9c63 in its night set, so they light up
   as the course goes to night). The land round the course is a city of
   such buildings in those same colours, read from the palette as the
   screen shows it (with the weather), on a street grid: a lot each
   CITY_LOT, a building on it from CITY_KINDS shapes (walls and a roof, a
   grid of windows on each wall, some lit), most low or middling, a few
   towers, CITY_EMPTY of the lots left open, each lot wholly CITY_CLEAR or
   more past the footprint (so nothing of it stands on the course or, from
   the game's camera in the footprint's box, between the eye and it: no
   line from a point in a box to another leaves it) and none past
   CITY_REACH. Each shape is an instanced mesh (nshvc
   6.2), written again only when its colours change, each building one
   instance of it, its size its lot's 0.8 to 1 (a size is uniform, so the
   shapes give the proportions), drawn at the world's depths nearest first,
   a building whose sphere is outside the view left out. A wall's colour
   by the way it faces (the world's: +x 8, +z 7, -x 6, -z 5), the roof 9,
   a lit window 10, set 0.004 of the building out from its wall. */
enum { CITY_KINDS = 12, CITY_MOST = 2048 };
#define CITY_LOT 3200.0
#define CITY_BASE 2400.0            /* a building's size at most: its shape's 1 */
#define CITY_CLEAR 2500.0
#define CITY_REACH 26000.0
#define CITY_EMPTY 0.12
#define CITY_LIT 0.55
static const struct {
    float w, d, h;                  /* across x, across z, up: in its size */
} city_shape[CITY_KINDS] = {
    { 1.0f, 1.0f, 0.5f }, { 0.9f, 0.7f, 0.8f }, { 0.7f, 0.9f, 1.0f }, { 1.0f, 0.6f, 1.2f },
    { 0.8f, 0.8f, 1.5f }, { 0.8f, 0.5f, 0.7f }, { 1.0f, 0.8f, 1.0f }, { 0.6f, 0.6f, 2.0f },
    { 0.9f, 0.9f, 1.8f }, { 0.7f, 0.7f, 2.6f }, { 0.5f, 0.5f, 3.2f }, { 0.6f, 0.8f, 4.0f },
};

static struct {
    int course, n;
    struct {
        float at[3], size;
        uint8_t kind;
    } b[CITY_MOST];
    nshvc_mesh mesh[CITY_KINDS];
    uint32_t vertices[CITY_KINDS];
    uint32_t colour[6];             /* the colours its meshes were written in: walls +x, +z, -x, -z, roof, windows */
} city = { -1, 0, { { { 0 }, 0, 0 } }, { { 0 } }, { 0 }, { 0 } };

static void city_make(unsigned course)
{
    double x0 = grid.xmin - CITY_REACH, z0 = grid.zmin - CITY_REACH;
    int nx = (int)((grid.xmax - grid.xmin + 2 * CITY_REACH) / CITY_LOT), nz = (int)((grid.zmax - grid.zmin + 2 * CITY_REACH) / CITY_LOT);
    int i, j;

    city.course = (int)course;
    city.n = 0;
    for (j = 0; j < nz; j++)
        for (i = 0; i < nx && city.n < CITY_MOST; i++) {
            double cx = x0 + (i + 0.5) * CITY_LOT, cz = z0 + (j + 0.5) * CITY_LOT, u = hash01(course, 9000 + (unsigned)i, (unsigned)j);
            double d = footprint_distance(cx, cz);
            int kind;

            if (d < CITY_CLEAR + CITY_LOT * 0.71 || d > CITY_REACH || u < CITY_EMPTY)
                continue;
            u = hash01(course, 9100 + (unsigned)i, (unsigned)j);       /* most low or middling, a few towers */
            kind = u < 0.7 ? (int)(u / 0.7 * 7) : u < 0.93 ? 7 + (int)((u - 0.7) / 0.23 * 2) : 9 + (int)((u - 0.93) / 0.07 * 3);
            if (kind >= CITY_KINDS)
                kind = CITY_KINDS - 1;
            city.b[city.n].at[0] = (float)cx;
            city.b[city.n].at[1] = (float)grid.level;
            city.b[city.n].at[2] = (float)cz;
            city.b[city.n].size = (float)(CITY_BASE * (0.8 + 0.2 * hash01(course, 9200 + (unsigned)i, (unsigned)j)));
            city.b[city.n].kind = (uint8_t)kind;
            city.n++;
        }
}

/* a quad of a building's mesh (corners in order round it) into v */
/* Whether triangle a, b, c is wound as the game's instanced meshes are for
   the GPU's cull mode (fx_draw.c: the back faces culled, a front face
   clockwise on the screen, x right and y down, the engine's default): seen
   from the side `out` faces, clockwise. View space is x right, y down, z
   ahead, and a turn keeps it so, so a face is clockwise on the screen when
   (b - a) x (c - a) points away from the eye: into the solid, against out. */
static int mesh_wound(const float *a, const float *b, const float *c, const float out[3])
{
    float e1[3] = { b[0] - a[0], b[1] - a[1], b[2] - a[2] }, e2[3] = { c[0] - a[0], c[1] - a[1], c[2] - a[2] };
    float n[3] = { e1[1] * e2[2] - e1[2] * e2[1], e1[2] * e2[0] - e1[0] * e2[2], e1[0] * e2[1] - e1[1] * e2[0] };

    return n[0] * out[0] + n[1] * out[1] + n[2] * out[2] <= 0;
}

/* A quad of a building, facing out along `out`, as two triangles wound
   clockwise on the screen seen from outside (mesh_wound) */
static uint32_t city_quad(nshvc_mesh_vertex *v, uint32_t n, const float (*q)[3], uint32_t colour, const float out[3])
{
    static const int tri[2][3] = { { 0, 1, 2 }, { 0, 2, 3 } };
    int t, i;

    for (t = 0; t < 2; t++) {
        int at[3] = { tri[t][0], tri[t][1], tri[t][2] };

        if (!mesh_wound(q[at[0]], q[at[1]], q[at[2]], out))
            at[1] = tri[t][2], at[2] = tri[t][1];
        for (i = 0; i < 3; i++) {
            v[n].x = q[at[i]][0], v[n].y = q[at[i]][1], v[n].z = q[at[i]][2];
            v[n].u = v[n].v = 0;
            v[n].colour = colour;
            n++;
        }
    }
    return n;
}

/* shape k's mesh, its base's middle at the origin (y is down), in colours c */
static uint32_t city_mesh(int k, const uint32_t c[6], nshvc_mesh_vertex *v)
{
    float w = city_shape[k].w / 2, d = city_shape[k].d / 2, h = city_shape[k].h, out = 0.004f;
    uint32_t n = 0;
    int side;

    for (side = 0; side < 4; side++) {
        /* the wall: +x, +z, -x, -z; along it (ax, az) from one end to the
           other (an x wall across z, a z wall across x), its normal (nx, nz) */
        float span = side & 1 ? 2 * w : 2 * d, nx = side == 0 ? 1.0f : side == 2 ? -1.0f : 0, nz = side == 1 ? 1.0f : side == 3 ? -1.0f : 0;
        float ox = nx * w + (side & 1 ? -w : 0), oz = nz * d + (side & 1 ? 0 : -d);
        float ax = side & 1 ? 1.0f : 0, az = side & 1 ? 0 : 1.0f;
        float q[4][3] = { { ox, 0, oz }, { ox + ax * span, 0, oz + az * span }, { ox + ax * span, -h, oz + az * span }, { ox, -h, oz } };
        int cols = (int)fmaxf(1, roundf(span / 0.14f)), rows = (int)fmaxf(1, roundf(h / 0.16f)), r, col;

        float facing[3] = { nx, 0, nz };

        n = city_quad(v, n, (const float (*)[3])q, c[side], facing);
        for (r = 0; r < rows; r++)
            for (col = 0; col < cols; col++) {
                float s0 = (col + 0.3f) / cols * span, s1 = (col + 0.7f) / cols * span;
                float y0 = -(r + 0.3f) / rows * h, y1 = -(r + 0.7f) / rows * h;
                float wq[4][3] = { { ox + ax * s0 + nx * out, y0, oz + az * s0 + nz * out },
                                   { ox + ax * s1 + nx * out, y0, oz + az * s1 + nz * out },
                                   { ox + ax * s1 + nx * out, y1, oz + az * s1 + nz * out },
                                   { ox + ax * s0 + nx * out, y1, oz + az * s0 + nz * out } };

                if (hash01((unsigned)k, 9300 + (unsigned)(side * 64 + col), (unsigned)r) < CITY_LIT)
                    n = city_quad(v, n, (const float (*)[3])wq, c[5], facing);
            }
    }
    {
        float roof[4][3] = { { -w, -h, -d }, { w, -h, -d }, { w, -h, d }, { -w, -h, d } };
        static const float up[3] = { 0, -1, 0 };

        n = city_quad(v, n, (const float (*)[3])roof, c[4], up);
    }
    return n;
}

typedef struct {
    float z;
    nshvc_instance in;
} CityInstance;

static int city_nearer(const void *a, const void *b)
{
    float x = ((const CityInstance *)a)->z, y = ((const CityInstance *)b)->z;

    return x < y ? -1 : x > y;
}

static void city_draw(unsigned course, const double eye[3], const double m[9])
{
    static const uint8_t entry[6] = { 8, 7, 6, 5, 9, 10 };
    static nshvc_mesh_vertex v[6 * 2048];
    static CityInstance in[CITY_KINDS][CITY_MOST];
    static nshvc_instance sorted[CITY_MOST];
    uint32_t colour[6], count[CITY_KINDS] = { 0 }, k, i;
    nshvc_view view;
    float side[4][3];
    int b, changed = 0;

    if (city.course != (int)course)
        city_make(course);
    for (k = 0; k < 6; k++) {               /* the backdrop's colours as the screen shows them */
        colour[k] = rgb_of(fx_draw_full_colour() ? pic.palette[entry[k]] : course_day_colour(g.course, entry[k]));
        changed |= colour[k] != city.colour[k];
    }
    for (k = 0; k < CITY_KINDS; k++)
        changed |= !city.mesh[k].id;
    if (changed) {                          /* the meshes in the colours now (each made the first time at its size) */
        memcpy(city.colour, colour, sizeof colour);
        for (k = 0; k < CITY_KINDS; k++) {
            city.vertices[k] = city_mesh((int)k, colour, v);
            if (!city.mesh[k].id)
                city.mesh[k] = nshvc_mesh_create(city.vertices[k]);
            if (city.mesh[k].id)
                nshvc_mesh_write(city.mesh[k], v, city.vertices[k]);
        }
    }
    land_view(eye, m, &view, side);
    for (b = 0; b < city.n; b++) {
        const float *at = city.b[b].at;
        int kind = city.b[b].kind;
        double s = city.b[b].size, h = city_shape[kind].h * s, w = city_shape[kind].w * s, d = city_shape[kind].d * s;
        double c[3] = { at[0], at[1] - h / 2, at[2] }, vv[3], r = 0.5 * sqrt(w * w + d * d + h * h);

        if (!land_sphere_seen(eye, m, (const float (*)[3])side, c, r, vv))
            continue;
        in[kind][count[kind]].z = (float)vv[2];
        in[kind][count[kind]].in.x = at[0], in[kind][count[kind]].in.y = at[1], in[kind][count[kind]].in.z = at[2];
        in[kind][count[kind]].in.size = (float)s;
        count[kind]++;
    }
    for (k = 0; k < CITY_KINDS; k++) {
        if (!count[k] || !city.mesh[k].id || !city.vertices[k])
            continue;
        qsort(in[k], count[k], sizeof in[k][0], city_nearer);
        for (i = 0; i < count[k]; i++)
            sorted[i] = in[k][i].in;
        /* each building seen through about the player's car where it hides
           it: its sphere half its height up, as land_sphere_seen's */
        fx_draw_mesh_seen(city.mesh[k], &view, sorted, count[k], city_shape[k].h / 2,
                          0.5f * sqrtf(city_shape[k].w * city_shape[k].w + city_shape[k].d * city_shape[k].d
                                       + city_shape[k].h * city_shape[k].h), FX_PIECE_KIND_CITY + k);
    }
}

/* ---- the clouds ----

   Jeff (2026-11-12, Sky Ramp: "Sea of clouds"): the track floats in the
   sky, its band sky all the way down. Under it a sea of clouds: the land's
   plane CLOUD_BELOW under the lowest place the course's shapes reach (the
   survey's deepest), out to the horizon, in the class's level face's
   colour, and on it cloud banks, each a few domes (puffs) of CLOUD_SEGS
   sides and CLOUD_RINGS rings, flat at the base on the plane, from
   CLOUD_KINDS shapes; a bank to a cell of CLOUD_CELL (CLOUD_EMPTY of the
   cells none), set off in it by up to a third of a cell, 0.6 to 1 of
   CLOUD_BASE, out to CLOUD_REACH past the footprint. A bank's top stays
   under the course's lowest place (its shapes at most 0.31 of their size
   high, CLOUD_BASE 0.31 = 2,790, less than CLOUD_BELOW less CLOUD_TOP_GAP),
   so with the eye above that place no line from it to the course passes
   through a bank, and banks may lie under the course as well as round
   it. A face's colour is the chip's for a lit face
   of the class facing as it does in the view (the light is fixed in view
   space, so the same for every bank: one colour a face, the mesh written
   again when one changes; a mesh set for each picture being made,
   fx_draw_mesh_slot, so a picture shown keeps the colours written for
   it). Each shape an instanced mesh (nshvc 6.2), each bank one instance
   of it, drawn at the world's depths nearest first, a bank whose sphere is
   outside the view left out. Triangles: a puff CLOUD_SEGS (2 CLOUD_RINGS -
   1) = 40, a bank 3-5 puffs, 120-200. */
enum { CLOUD_KINDS = 6, CLOUD_MOST = 2048, CLOUD_PUFFS = 5, CLOUD_SEGS = 8, CLOUD_RINGS = 3, CLOUD_SLOTS = 5,
       CLOUD_TRIS = CLOUD_PUFFS * CLOUD_SEGS * (2 * CLOUD_RINGS - 1) };
#define CLOUD_BELOW 4096.0
#define CLOUD_TOP_GAP 1024.0
#define CLOUD_BASE 9000.0
#define CLOUD_CELL 9000.0
#define CLOUD_REACH 120000.0
#define CLOUD_EMPTY 0.2

static struct {
    int made;                       /* the shapes made */
    uint32_t tris[CLOUD_KINDS];
    float tri[CLOUD_KINDS][CLOUD_TRIS][3][3];   /* in its size, its base's middle at the origin, y down */
    double normal[CLOUD_KINDS][CLOUD_TRIS][3];  /* outward, the world's axes */
    int course, n;
    struct {
        float x, z, size;
        uint8_t kind;
    } b[CLOUD_MOST];
    nshvc_mesh mesh[CLOUD_SLOTS][CLOUD_KINDS];
    uint32_t colour[CLOUD_SLOTS][CLOUD_KINDS][CLOUD_TRIS];
} clouds = { 0, { 0 }, { { { { 0 } } } }, { { { 0 } } }, -1, 0, { { 0, 0, 0, 0 } }, { { { 0 } } }, { { { 0 } } } };

/* shape k: its puffs, the first in the middle (0.32-0.38 across its
   radius, 0.7-0.8 its radius high), the rest round it (0.15-0.3 out, 0.18-
   0.3, 0.55-0.8 high); each a dome of rings at 0, 30 and 60 degrees up
   and its top */
static void clouds_shapes(void)
{
    const double turn = 6.283185307179586;
    int k, i, r, s, t;

    for (k = 0; k < CLOUD_KINDS; k++) {
        int puffs = 3 + (int)(hash01(0x40 + (unsigned)k, 9500, 0) * 3);
        uint32_t n = 0;

        for (i = 0; i < puffs && i < CLOUD_PUFFS; i++) {
            double a = turn * hash01(0x40 + (unsigned)k, 9510 + (unsigned)i, 1);
            double out = i ? 0.15 + 0.15 * hash01(0x40 + (unsigned)k, 9510 + (unsigned)i, 2) : 0;
            double rad = i ? 0.18 + 0.12 * hash01(0x40 + (unsigned)k, 9510 + (unsigned)i, 3) : 0.32 + 0.06 * hash01(0x40 + (unsigned)k, 9510, 3);
            double high = i ? 0.55 + 0.25 * hash01(0x40 + (unsigned)k, 9510 + (unsigned)i, 4) : 0.7 + 0.1 * hash01(0x40 + (unsigned)k, 9510, 4);
            double spin = turn * hash01(0x40 + (unsigned)k, 9510 + (unsigned)i, 5);
            double mid[3] = { cos(a) * out, 0, sin(a) * out }, ring[CLOUD_RINGS][CLOUD_SEGS][3], top[3];

            for (r = 0; r < CLOUD_RINGS; r++)
                for (s = 0; s < CLOUD_SEGS; s++) {
                    double up = turn / 4 * r / CLOUD_RINGS, round = spin + turn * s / CLOUD_SEGS;

                    ring[r][s][0] = mid[0] + rad * cos(up) * cos(round);
                    ring[r][s][1] = -rad * high * sin(up);
                    ring[r][s][2] = mid[2] + rad * cos(up) * sin(round);
                }
            top[0] = mid[0], top[1] = -rad * high, top[2] = mid[2];
            for (r = 0; r < CLOUD_RINGS; r++)
                for (s = 0; s < CLOUD_SEGS; s++) {
                    int s1 = (s + 1) % CLOUD_SEGS, faces = r + 1 < CLOUD_RINGS ? 2 : 1, f;

                    for (f = 0; f < faces; f++) {
                        const double *p[3];
                        double nv[3], c[3], len;

                        if (r + 1 == CLOUD_RINGS)
                            p[0] = ring[r][s], p[1] = ring[r][s1], p[2] = top;
                        else if (!f)
                            p[0] = ring[r][s], p[1] = ring[r][s1], p[2] = ring[r + 1][s1];
                        else
                            p[0] = ring[r][s], p[1] = ring[r + 1][s1], p[2] = ring[r + 1][s];
                        normal_of(p[0], p[1], p[2], nv);
                        for (t = 0; t < 3; t++)
                            c[t] = (p[0][t] + p[1][t] + p[2][t]) / 3 - mid[t];
                        if (nv[0] * c[0] + nv[1] * c[1] + nv[2] * c[2] < 0) {  /* outward: away from the puff's middle */
                            for (t = 0; t < 3; t++)
                                nv[t] = -nv[t];
                        } else {                /* its corners the other way round: clockwise seen from outside (mesh_wound) */
                            const double *swap = p[1];

                            p[1] = p[2];
                            p[2] = swap;
                        }
                        len = sqrt(nv[0] * nv[0] + nv[1] * nv[1] + nv[2] * nv[2]);
                        if (len <= 0)
                            continue;
                        for (t = 0; t < 3; t++) {
                            clouds.normal[k][n][t] = nv[t] / len;
                            clouds.tri[k][n][t][0] = (float)p[t][0];
                            clouds.tri[k][n][t][1] = (float)p[t][1];
                            clouds.tri[k][n][t][2] = (float)p[t][2];
                        }
                        n++;
                    }
                }
        }
        clouds.tris[k] = n;
    }
    clouds.made = 1;
}

static void clouds_make(unsigned course)
{
    double x0 = grid.xmin - CLOUD_REACH, z0 = grid.zmin - CLOUD_REACH;
    int nx = (int)((grid.xmax - grid.xmin + 2 * CLOUD_REACH) / CLOUD_CELL) + 1;
    int nz = (int)((grid.zmax - grid.zmin + 2 * CLOUD_REACH) / CLOUD_CELL) + 1;
    int i, j;

    clouds.course = (int)course;
    clouds.n = 0;
    for (j = 0; j < nz; j++)
        for (i = 0; i < nx && clouds.n < CLOUD_MOST; i++) {
            double x = x0 + (i + 0.5 + 0.67 * (hash01(course, 9600 + (unsigned)i, (unsigned)j) - 0.5)) * CLOUD_CELL;
            double z = z0 + (j + 0.5 + 0.67 * (hash01(course, 9700 + (unsigned)i, (unsigned)j) - 0.5)) * CLOUD_CELL;
            int kind = (int)(hash01(course, 9800 + (unsigned)i, (unsigned)j) * CLOUD_KINDS);

            if (hash01(course, 9900 + (unsigned)i, (unsigned)j) < CLOUD_EMPTY || footprint_distance(x, z) > CLOUD_REACH)
                continue;
            clouds.b[clouds.n].x = (float)x;
            clouds.b[clouds.n].z = (float)z;
            clouds.b[clouds.n].size = (float)(CLOUD_BASE * (0.6 + 0.4 * hash01(course, 9950 + (unsigned)i, (unsigned)j)));
            clouds.b[clouds.n].kind = (uint8_t)(kind < CLOUD_KINDS ? kind : CLOUD_KINDS - 1);
            clouds.n++;
        }
}

static void clouds_draw(unsigned course, int cls, double deck, const double eye[3], const double m[9])
{
    static nshvc_mesh_vertex v[3 * CLOUD_TRIS];
    static CityInstance in[CLOUD_KINDS][CLOUD_MOST];   /* the city's: a bank's depth and instance */
    static nshvc_instance sorted[CLOUD_MOST];
    uint32_t count[CLOUD_KINDS] = { 0 }, i, t;
    int slot = fx_draw_mesh_slot(), k, b, c;
    nshvc_view view;
    float side[4][3];

    if (slot < 0 || slot >= CLOUD_SLOTS)
        return;
    if (!clouds.made)
        clouds_shapes();
    if (clouds.course != (int)course)
        clouds_make(course);
    for (k = 0; k < CLOUD_KINDS; k++) {         /* each face's colour as it faces in the view now */
        int changed = !clouds.mesh[slot][k].id;
        uint32_t n = 0;

        for (t = 0; t < clouds.tris[k]; t++) {
            const double *nw = clouds.normal[k][t];
            double nview[3];
            uint32_t colour;

            for (c = 0; c < 3; c++)
                nview[c] = nw[0] * m[c] + nw[1] * m[3 + c] + nw[2] * m[6 + c];
            colour = face_colour(cls, nview, nw, 0);
            changed |= colour != clouds.colour[slot][k][t];
            clouds.colour[slot][k][t] = colour;
        }
        if (!changed || !clouds.tris[k])
            continue;
        for (t = 0; t < clouds.tris[k]; t++)
            for (c = 0; c < 3; c++, n++) {
                v[n].x = clouds.tri[k][t][c][0], v[n].y = clouds.tri[k][t][c][1], v[n].z = clouds.tri[k][t][c][2];
                v[n].u = v[n].v = 0;
                v[n].colour = clouds.colour[slot][k][t];
            }
        if (!clouds.mesh[slot][k].id)
            clouds.mesh[slot][k] = nshvc_mesh_create(3 * CLOUD_TRIS);
        if (clouds.mesh[slot][k].id)
            nshvc_mesh_write(clouds.mesh[slot][k], v, n);
    }
    land_view(eye, m, &view, side);
    for (b = 0; b < clouds.n; b++) {
        double s = clouds.b[b].size, ctr[3] = { clouds.b[b].x, deck - 0.15 * s, clouds.b[b].z }, vv[3];
        int kind = clouds.b[b].kind;

        if (!land_sphere_seen(eye, m, (const float (*)[3])side, ctr, 0.65 * s, vv))
            continue;
        in[kind][count[kind]].z = (float)vv[2];
        in[kind][count[kind]].in.x = clouds.b[b].x, in[kind][count[kind]].in.y = (float)deck, in[kind][count[kind]].in.z = clouds.b[b].z;
        in[kind][count[kind]].in.size = (float)s;
        count[kind]++;
    }
    for (k = 0; k < CLOUD_KINDS; k++) {
        if (!count[k] || !clouds.mesh[slot][k].id || !clouds.tris[k])
            continue;
        qsort(in[k], count[k], sizeof in[k][0], city_nearer);
        for (i = 0; i < count[k]; i++)
            sorted[i] = in[k][i].in;
        /* each bank seen through about the player's car where it hides it:
           its sphere as land_sphere_seen's */
        fx_draw_mesh_seen(clouds.mesh[slot][k], &view, sorted, count[k], 0.15f, 0.65f, FX_PIECE_KIND_CLOUD + k);
    }
}

static void land_polygons(unsigned course, int cls, const double eye[3], const double m[9])
{
    static nshvc_vertex v[LAND_VERTICES];
    int clouded = surface[course].kind == SURF_CLOUD;
    /* the plane at the course's level, or a sea of clouds' under its lowest place */
    double deck = grid.deepest + CLOUD_BELOW, plane_y = fmax(clouded ? deck : grid.level, eye[1] + EYE_CLEAR), up[3];
    static const double up_world[3] = { 0, -1, 0 };
    uint32_t nv = 0, j;
    int i, k, mesas = cls == CLS_ROCK || surface[course].mesas;

    if (!grid.nx)
        return;
    if (!surface[course].flat && (range.course != (int)course || range.mesas != mesas))
        range_make(course, mesas);
    /* the plane, its colour up its normal */
    for (k = 0; k < 3; k++)
        up[k] = -m[3 + k];
    ground_now = face_colour(cls, up, up_world, 1);
    fx_draw_depth_class(FX_DEPTH_GROUND);       /* the ground: behind all the 3D (fx_ground_draw) */
    plane_vertices(plane_y, ground_now, eye, m, v, &nv, LAND_VERTICES);
    fx_draw_land(v, nv);
    nv = 0;
    fx_draw_depth_class(FX_DEPTH_COURSE);
    fx_draw_piece(FX_PIECE_MOUNTAINS);             /* seen through about the player's car where they hide it (fx_draw.c) */
    /* the mountains' or mesas' faces that look toward the eye, their bases
       on the plane (none on a flat course) */
    for (i = 0; i < (surface[course].flat ? 0 : MOUNTAINS); i++) {
        double mid[3] = { 0, plane_y, 0 };

        for (k = 0; k < SIDES; k++) {
            mid[0] += range.base[i][k][0] / SIDES;
            mid[2] += range.base[i][k][1] / SIDES;
        }
        for (k = 0; k < SIDES; k++) {
            int k1 = (k + 1) % SIDES;
            double b0[3] = { range.base[i][k][0], plane_y, range.base[i][k][1] };
            double b1[3] = { range.base[i][k1][0], plane_y, range.base[i][k1][1] };
            double n[3];

            if (!mesas) {
                const double *p = range.peak[i];
                double tri[3][3];

                normal_of(p, b0, b1, n);
                if (n[1] > 0)                   /* outward: up (y is down) */
                    for (j = 0; j < 3; j++)
                        n[j] = -n[j];
                memcpy(tri[0], p, sizeof tri[0]);
                memcpy(tri[1], b0, sizeof tri[1]);
                memcpy(tri[2], b1, sizeof tri[2]);
                land_face((const double (*)[3])tri, 3, n, cls, eye, m, v, &nv, LAND_VERTICES);
            } else {
                double quad[4][3];

                memcpy(quad[0], b0, sizeof quad[0]);
                memcpy(quad[1], b1, sizeof quad[1]);
                memcpy(quad[2], range.top[i][k1], sizeof quad[2]);
                memcpy(quad[3], range.top[i][k], sizeof quad[3]);
                normal_of(quad[0], quad[1], quad[3], n);
                /* outward: away from the mesa's middle */
                if (n[0] * (b0[0] + b1[0] - 2 * mid[0]) + n[2] * (b0[2] + b1[2] - 2 * mid[2]) < 0)
                    for (j = 0; j < 3; j++)
                        n[j] = -n[j];
                land_face((const double (*)[3])quad, 4, n, cls, eye, m, v, &nv, LAND_VERTICES);
            }
        }
        if (mesas)                              /* its flat top */
            land_face((const double (*)[3])range.top[i], SIDES, up_world, cls, eye, m, v, &nv, LAND_VERTICES);
    }
    fx_draw_land(v, nv);
    fx_draw_piece(0);
    /* a city round the course */
    if (surface[course].city && grid.nx)
        city_draw(course, eye, m);
    /* cloud banks on a sea of clouds */
    if (clouded)
        clouds_draw(course, cls, deck, eye, m);
    /* the course's own trees round it (one player's pictures, once the
       race has drawn one of its trees) */
    if (surface[course].forest && !surface[course].flat && !mesas && fx_draw_full_colour() && fx_solid_forest_ready())
        forest_draw(course, eye, m, plane_y);
    /* the canyon's empty space filled with rock, which hides the far
       objects behind it (the course's own rock only) */
    if (cls == CLS_ROCK) {
        static nshvc_vertex fv[FILL_VERTICES];
        uint32_t nf = 0;

        shell_view.on = 1;
        shell_view.cls = cls;
        shell_view.plane_y = plane_y;
        memcpy(shell_view.eye, eye, sizeof shell_view.eye);
        memcpy(shell_view.m, m, sizeof shell_view.m);

        if (fill.course != (int)course)
            fill_make(course);
        fill_faces(course, cls, plane_y, eye, m, fv, &nf, FILL_VERTICES);
        fx_draw_piece(FX_PIECE_FILL);
        fx_draw_land(fv, nf);
        fx_draw_piece(0);
    }
}

/* ---- the ocean: a plane of facets, partly see-through, over a sea bed ----

   Jeff (2026-11-11): Aqua Tunnel an ocean with a polygonal water effect, a
   shader allowed, simple and clear, in the game's look. The water is one
   plane at the course's water level out to the horizon (as the land's), at
   the world's true depths, drawn after the solid 3D, tested and not
   written (fx_draw.c fx_draw_water), with holes where the road goes down
   (fx_face_cut). ocean.shade draws it as a lattice of OCEAN_CELL facets, each lattice
   point rising and falling OCEAN_WAVE of a cell, each facet shaded as the
   chip shades a lit face in the course's water class (the image's texels
   0-9: the class's colours at levels 6-15, texel 10 the flat sea's), the
   facets giving way to the flat sea where they are smaller than a pixel;
   partly see-through, the more the steeper the eye looks down, whole toward
   the horizon. It is drawn as the earlier water was (fx_draw_surface): its
   vertices' u and v their screen places less an origin inside it, and no
   depth divisor (a divisor, for u and v in perspective, came back from the
   GPU's divide a rounding off the 3D's depth and the water showed through
   the road in speckles, Jeff's picture of 2026-11-11); the shader's inputs
   are the projective map from a screen place to the water in cells (the
   eye's place taken modulo OCEAN_PERIOD cells, the lattice's pattern, so
   the numbers stay small), its other numbers in its image.

   From below (the eye under the water: the glass tube) it is whole, and it
   is drawn in the sealed tunnel too, so that the original's underwater
   backdrop shows nowhere: the water overhead, the sea bed below.

   Under it a sea bed, so that nothing of the backdrop shows through the
   water: a plane SEABED_BELOW under the lowest place the course's shapes
   meet the ground (or the water), opaque, in the class's darkest colour
   darkened to SEABED_DARK, at the world's true depths, under all of the
   course.
   Triangles: the water's quad and the bed's, each kept to the view in at
   most 6. */

#define OCEAN_CELL 512.0
#define OCEAN_PERIOD 64         /* ocean.shade's N */
#define OCEAN_WAVE 0.08
#define SEABED_BELOW 512.0
#define SEABED_DARK 0.55
#define OCEAN_IMAGE_W 24        /* ocean.shade's image: 11 colours, then 13 floats */

static nshvc_image ocean_image[4];
static nshvc_shader ocean_shader;

/* The band's own sea (CLS_BAND): the band's ground half as the screen
   shows it in the picture's view (band_read, once a picture: colour 1 on
   the screen rows from BAND_FROM under the view's horizon to as far under
   it as the view's bottom is under its centre, the rows a level view
   shows of it, as the earlier ground took them, race_ground_colour; the
   one player's full window reads its lists' places from 256 there, its
   horizon at place 249, with the weather and the fog; two players'
   pictures their own bands, which the one player's lists do not give:
   black on the beach and in the night city where the lists' places
   256-511 were read, in check runs of 2026-11-12; and only the rows a
   view shows: the battle desert's band is tan, bdbd8c, under its horizon
   and red-brown far past a half's bottom), its colours each once, from the
   darkest (under the eye) to the lightest (at the horizon), and of those
   the lightest third, the water as the original shows it beside the road
   (Lake Side's lake light cyan, 95,255,227 in its pictures: it lies out
   toward the horizon's rows; the rest lies under the road near the eye);
   a lit face's level 6-15 takes the one as far along them (6 the
   darkest), so a level face seen from the race's camera (level 9 or 10)
   is the band's light teal (Lake Side's 74,239,198 to 90,247,214). With
   whole (CLS_BAND_WHOLE), all of them (the battle courses). */
#define BAND_FROM 7              /* rows under the horizon the band's ground half begins: 256 - 249 */

static struct {
    int n;
    uint16_t seen[256];         /* its colours each once, under the eye first */
} band;

static void band_read(int p)
{
    float x, row, slope;
    int have = fx_draw_horizon_now(p, &x, &row, &slope), i, k;
    int rows = (int16_t)ram_word(0x003E) - (int16_t)ram_word(0x0036) - BAND_FROM;      /* the view's bottom less its centre */

    if (rows < 1)
        rows = 1;
    if (rows > 256)
        rows = 256;
    band.n = 0;
    for (i = rows - 1; i >= 0; i--) {   /* under the eye first */
        uint16_t c = have ? race_ground_colour(p, row + BAND_FROM + i) : sky_colour_at(256 + i);

        for (k = 0; k < band.n && band.seen[k] != c; k++)
            ;
        if (k == band.n)
            band.seen[band.n++] = c;
    }
}

static uint32_t band_shade(int level, int whole)
{
    int k, from;

    if (!band.n)
        return 0xFF000000u;
    from = whole ? 0 : 2 * band.n / 3;
    k = from + (int)lround((level < 6 ? 0 : level > 15 ? 9 : level - 6) / 9.0 * (band.n - 1 - from));
    return rgb_of(band.seen[k]);
}

/* ---- the course's ground faces shaded by distance ----

   Jeff (2026-11-12, Night Cruise: its land "like a solid... taupe
   something"; shown that the flat khaki by the road is the course's own
   verges, he chose to shade them): Night Cruise's verges and pads are its
   own flat faces in class 17's khaki (palette entries 139-143), one colour
   a face to the horizon. Such a face is drawn with each corner's colour
   its own toward the ground's by the corner's depth (the land's plane's
   colour this picture, Night Cruise's pavement; before any, the band's on
   that corner's screen row): its own nearer than VERGE_NEAR, the ground's
   from VERGE_FAR (eased between), so a verge goes into the ground toward
   the horizon. */
#define VERGE_NEAR 4000.0
#define VERGE_FAR 40000.0

int fx_ground_verge(uint8_t entry)
{
    return g.course == 0x07 && entry >= 139 && entry <= 143;
}

uint32_t fx_ground_verge_colour(uint8_t entry, double depth, float row, int panel)
{
    uint32_t own = rgb_of(pic.palette[entry]), out = 0xFF000000u;
    uint32_t band = ground_now ? ground_now : rgb_of(race_ground_colour(panel, row));
    double t = (depth - VERGE_NEAR) / (VERGE_FAR - VERGE_NEAR);
    int k;

    t = t < 0 ? 0 : t > 1 ? 1 : t;
    t = t * t * (3 - 2 * t);
    for (k = 0; k < 24; k += 8) {
        double a = own >> k & 255, b = band >> k & 255;

        out |= (uint32_t)lround(a + (b - a) * t) << k;
    }
    return out;
}

/* A sea's shade at light level `level` (6-15): class cls's, its levels
   from `first` (0: 6) to 15 spread over 6-15, so a sea drawn in a class's
   lighter shades alone (Sea Breeze's, its crossing's colours) is lit as
   the whole row would be. */
static uint32_t sea_shade(int cls, int first, int level)
{
    if (cls == CLS_BAND || cls == CLS_BAND_WHOLE)
        return band_shade(level, cls == CLS_BAND_WHOLE);
    if (first > 6)
        level = first + (int)lround((level < 6 ? 0 : level > 15 ? 9 : level - 6) * (15 - first) / 9.0);
    return row_colour(cls, level);
}

static void ocean_draw(int cls, int under, int first, const double eye[3], const double m[9], double water_y, int p)
{
    static nshvc_vertex bed[24];
    static FxCorner c[64];
    uint32_t words[OCEAN_IMAGE_W] = { 0 }, nb = 0, nc, j, deep;
    double dh = water_y - eye[1], reach, q[4][3], up[3], light[3], l[3], ex, ez, bed_y, ox = 0, oy = 0;
    double vcx = (int16_t)ram_word(0x0034), vcy = (int16_t)ram_word(0x0036), ra[3], rb[3], rc[3], c1;
    float in[8], sx, sy, scale;
    int i, k, slot;

    /* seen from below (the glass tube), the water and the bed in the class
       of the underwater view (Aqua Tunnel's blue, as the original's
       underwater backdrop is, Jeff's picture of 2026-11-11) */
    if (dh < 0 && under)
        cls = under;
    /* the sea bed */
    bed_y = (grid.nx ? fmax(grid.deepest, water_y) : water_y) + SEABED_BELOW;
    deep = sea_shade(cls, first, 6);
    deep = 0xFF000000u | (uint32_t)((deep >> 16 & 255) * SEABED_DARK) << 16 | (uint32_t)((deep >> 8 & 255) * SEABED_DARK) << 8
           | (uint32_t)((deep & 255) * SEABED_DARK);
    plane_vertices(bed_y, deep, eye, m, bed, &nb, sizeof bed / sizeof bed[0]);
    fx_draw_land(bed, nb);
    /* the water's quad about the eye, out to HORIZON_GAP of the horizon
       (from above or below) */
    fx_draw_world_scale(&sx, &sy);
    scale = sx > sy ? sx : sy;
    reach = 128.0 * fabs(dh) * scale / HORIZON_GAP;
    for (i = 0; i < 4; i++) {
        double w[3] = { eye[0] + ((i == 1 || i == 2) ? reach : -reach), water_y, eye[2] + (i >= 2 ? reach : -reach) };

        to_view(w, eye, m, q[i]);
    }
    nc = fx_face_triangles_whole((const double (*)[3])q, 4, c, sizeof c / sizeof c[0]);
    if (!nc)
        return;
    for (j = 0; j < nc; j++) {
        ox += c[j].x;
        oy += c[j].y;
    }
    ox /= nc;
    oy /= nc;
    /* the map from a screen place (x, y), less (ox, oy), to the water in
       cells: the view's ray d = ((x - cx) / 128, (y - cy) / 128, 1) meets
       the water at depth dh / (row 1 . d); world x and z from rows 0 and 2,
       each row k . d = ra_k x + rb_k y + rc_k (as the water's earlier map) */
    for (k = 0; k < 3; k++) {
        ra[k] = m[3 * k] / 128.0;
        rb[k] = m[3 * k + 1] / 128.0;
        rc[k] = m[3 * k + 2] - (m[3 * k] * vcx + m[3 * k + 1] * vcy) / 128.0 + ra[k] * ox + rb[k] * oy;
    }
    c1 = rc[1];
    if (fabs(c1) < 1e-12)
        return;
    ex = wrap(eye[0], OCEAN_PERIOD * OCEAN_CELL);
    ez = wrap(eye[2], OCEAN_PERIOD * OCEAN_CELL);
    in[0] = (float)((ex * ra[1] + dh * ra[0]) / (OCEAN_CELL * c1));
    in[1] = (float)((ex * rb[1] + dh * rb[0]) / (OCEAN_CELL * c1));
    in[2] = (float)((ex * c1 + dh * rc[0]) / (OCEAN_CELL * c1));
    in[3] = (float)((ez * ra[1] + dh * ra[2]) / (OCEAN_CELL * c1));
    in[4] = (float)((ez * rb[1] + dh * rb[2]) / (OCEAN_CELL * c1));
    in[5] = (float)((ez * c1 + dh * rc[2]) / (OCEAN_CELL * c1));
    in[6] = (float)(ra[1] / c1);
    in[7] = (float)(rb[1] / c1);
    /* the image: the class's colours at levels 6-15, the flat sea's (up its
       normal), then the floats: the light by the level's scale in the
       world's axes (world axis k's view components are m[3k..3k+2]), the
       time, the eye's place and height in cells, the waves' height */
    for (i = 0; i < 10; i++)
        words[i] = sea_shade(cls, first, 6 + i);
    for (k = 0; k < 3; k++)
        up[k] = -m[3 + k];
    words[10] = sea_shade(cls, first, chip_level(up));
    fx_view_light_vector(l);
    for (k = 0; k < 3; k++)
        light[k] = grid.light_k * (m[3 * k] * l[0] + m[3 * k + 1] * l[1] + m[3 * k + 2] * l[2]);
    words[11] = float_word((float)light[0]);
    words[12] = float_word((float)light[1]);
    words[13] = float_word((float)light[2]);
    words[14] = float_word((float)fmod(nshvc_seconds(), 600.0));
    words[15] = float_word((float)(ex / OCEAN_CELL));
    words[16] = float_word((float)(ez / OCEAN_CELL));
    words[17] = float_word((float)(dh / OCEAN_CELL));
    words[18] = float_word((float)OCEAN_WAVE);
    /* seen through about the player's car while it is under the water
       (fx_draw.c, "seen through"): the car's place less the water's origin,
       the radius, its inner part, how much of the water shows within it */
    {
        double plane[3] = { m[3], m[4], m[5] };
        float see[5] = { 0, 0, 1, 0, 1 };

        if (fx_draw_see_plane(plane, dh, FX_PIECE_WATER, see)) {
            see[0] -= (float)ox;
            see[1] -= (float)oy;
        }
        for (k = 0; k < 5; k++)
            words[19 + k] = float_word(see[k]);
    }
    slot = fx_draw_tweening() * 2 + (p & 1);
    if (!ocean_image[slot].id)
        ocean_image[slot] = nshvc_image_create(OCEAN_IMAGE_W, 1, NSHVC_IMAGE_RGBA);
    if (!ocean_shader.id)
        ocean_shader = nshvc_shader_load("ocean");
    nshvc_image_write(ocean_image[slot], words);
    fx_draw_depth_class(FX_DEPTH_WATER);        /* with the depth test: drawn after the solid 3D (fx_draw.c) */
    fx_draw_surface(c, nc, ocean_image[slot], ocean_shader, in, (float)ox, (float)oy);
    fx_draw_depth_class(FX_DEPTH_COURSE);
}

static void ground_draw(void);

/* With the depth test (docs/stuntracefx/native/DEPTH.md, 2) the land's
   mountains, mesas, rock, trees, buildings, clouds and sea bed are at the
   world's true depths, so they hide the course where they stand in front
   of it (the overhead view's eye can be outside the course's footprint)
   and it hides them. The ground (the plane at the course's level, the
   median of where its shapes meet the ground, and a course's floor) stands
   in for the original's ground band: much of the course lies below its
   level, so it is in the ground's depths, behind all of the 3D. The
   ocean's water at true depths, tested and not written, after the solid
   3D. */
void fx_ground_draw(void)
{
    ground_draw();
    fx_draw_depth_class(FX_DEPTH_COURSE);
}

static void ground_draw(void)
{
    unsigned course = g.course;
    int kind = course < 0x20 ? surface[course].kind : SURF_NONE, p = fx_draw_picture_now(), slot, i, k;
    double m[9], eye[3], dh, water_y, cx, cy, top, bottom, a[3], b[3], c[3], c1, ox = 0, oy = 0, scale, reach;
    double q[4][3];
    static FxCorner corners[64];
    uint32_t n;
    float in[8], sx, sy;
    unsigned x, y, marks = 0, rows, first;
    static uint8_t cells[80][64];
    uint16_t colours[16];
    int aqua, at_x, at_y;

    /* in the sealed tunnel (bit 3 of $0DA5) the earlier water left the
       original's underwater backdrop to show; the ocean is drawn there too,
       seen from below, over that backdrop (Jeff, 2026-11-11) */
    shell_view.on = 0;                  /* the tunnels' rock only where the land sets it */
    shell_pending = -1;
    if (kind == SURF_NONE || (kind == SURF_AQUA && (g.picture_effects & 0x08) && !surface[course].cls))
        return;
    {
        int32_t whole[3];               /* a picture's view kept whole (fx_view.h), past 16 bits */

        if (fx_view_eye(whole))
            for (i = 0; i < 3; i++)
                eye[i] = whole[i];
        else
            for (i = 0; i < 3; i++)
                eye[i] = (int16_t)ram_word(FX_FAR_BANK + FX_FAR_VIEW + 2 * i);
    }
    for (i = 0; i < 9; i++)
        m[i] = (int16_t)ram_word((uint32_t)(0x00E4 + 2 * i)) / 32768.0;
    if (surface[course].cls == CLS_BAND || surface[course].cls == CLS_BAND_WHOLE)
        band_read(p);                   /* the band's colours as this picture's view shows them */
    if ((IS_LAND(kind) || surface[course].cls) && grid.course != (int)course) {
        ground_survey(course, eye, m);  /* the course's shapes, once a course */
        range.course = -1;
        fill.course = -1;
        forest.course = -1;
        city.course = -1;
    }
    /* land: none in a tunnel (bit 3 of $0DA5, which the tunnel's gates set
       and clear: the original turns the backdrop's colours black there, so
       its openings show black, not the land outside; Jeff, 2026-11-11) */
    if (IS_LAND(kind) && (g.picture_effects & 0x08))
        return;
    if (IS_LAND(kind)) {                /* land: polygons, or the terrain over the view */
        if (surface[course].cls) {
            land_polygons(course, surface[course].cls, eye, m);
        } else {
            fx_draw_depth_class(FX_DEPTH_GROUND);   /* the terrain over the view: a course's floor */
            land_draw(kind, eye, m, p);
            fx_draw_depth_class(FX_DEPTH_COURSE);
        }
        return;
    }
    water_y = surface[course].y == WATER_DEEPEST ? (grid.nx ? grid.deepest : 0) : surface[course].y;
    dh = water_y - eye[1];                      /* world y is down: > 0, the surface below the eye */
    /* water seen from below too (Aqua Tunnel's, a sea with an under class:
       Harbor City's) is drawn with the eye under it, and cuts the picture */
    if (dh == 0 || (dh < 0 && kind != SURF_AQUA && !surface[course].under))
        return;
    if (surface[course].cls) {          /* the ocean; Aqua Tunnel's cuts the picture as below */
        ocean_draw(surface[course].cls, surface[course].under, surface[course].first, eye, m, water_y, p);
        if (kind == SURF_AQUA || surface[course].under) {
            double plane[3] = { m[3], m[4], m[5] };

            fx_face_cut(1, plane, dh);
        }
        return;
    }
    cx = (int16_t)ram_word(0x0034);
    cy = (int16_t)ram_word(0x0036);
    top = (int16_t)ram_word(0x003C);
    bottom = (int16_t)ram_word(0x003E);
    fx_draw_world_scale(&sx, &sy);
    scale = sx > sy ? sx : sy;
    /* the plane's corners in view space: world axis k turns to row k
       (m[3k..3k+2], as the sort turns places), the quad far enough out
       that its far edge lies HORIZON_GAP output pixels from the horizon */
    reach = 128.0 * fabs(dh) * scale / HORIZON_GAP;
    for (i = 0; i < 4; i++) {
        double w[3] = { (i == 1 || i == 2) ? reach : -reach, dh, i >= 2 ? reach : -reach };

        for (k = 0; k < 3; k++)
            q[i][k] = w[0] * m[k] + w[1] * m[3 + k] + w[2] * m[6 + k];
    }
    n = fx_face_triangles_whole((const double (*)[3])q, 4, corners, sizeof corners / sizeof corners[0]);
    if (!n)
        return;
    for (i = 0; i < (int)n; i++) {
        ox += corners[i].x;
        oy += corners[i].y;
    }
    ox /= n;
    oy /= n;
    /* the map from a screen place (x, y), less (ox, oy), to the plane's
       texels: the view's ray d = ((x - cx) / 128, (y - cy) / 128, 1) meets
       the plane at t = dh / (row1 . d); world x = eye x + t (row0 . d),
       z = eye z + t (row2 . d). Row k . d = a_k x + b_k y + c_k. The eye's
       place taken modulo the cell, so the numbers stay small. */
    for (k = 0; k < 3; k++) {
        a[k] = m[3 * k] / 128.0;
        b[k] = m[3 * k + 1] / 128.0;
        c[k] = m[3 * k + 2] - (m[3 * k] * cx + m[3 * k + 1] * cy) / 128.0 + a[k] * ox + b[k] * oy;
    }
    c1 = c[1];
    if (fabs(c1) < 1e-12)
        return;
    {
        double ex = wrap(eye[0], CELL_U * TEXEL_U), ez = wrap(eye[2], CELL_V * TEXEL_V);

        in[0] = (float)((ex * a[1] + dh * a[0]) / (TEXEL_U * c1));
        in[1] = (float)((ex * b[1] + dh * b[0]) / (TEXEL_U * c1));
        in[2] = (float)((ex * c1 + dh * c[0]) / (TEXEL_U * c1));
        in[3] = (float)((ez * a[1] + dh * a[2]) / (TEXEL_V * c1));
        in[4] = (float)((ez * b[1] + dh * b[2]) / (TEXEL_V * c1));
        in[5] = (float)((ez * c1 + dh * c[2]) / (TEXEL_V * c1));
        in[6] = (float)(a[1] / c1);
        in[7] = (float)(b[1] / c1);
    }
    /* the image: the cell, the band's rows, the header */
    memset(words, 0, sizeof words);
    aqua = kind == SURF_AQUA && race_underwater_cells(cells, colours);
    for (y = 0; y < CELL_V; y++)
        for (x = 0; x < CELL_U; x++)
            if (aqua ? cells[y][x] >= 2 : glint(x, y)) {
                words[(1 + y) * IMAGE_W + x] = 0xFF000000u;
                marks++;
            }
    if (aqua)                           /* each strip row's ripple colour: its commonest of 2-15 */
        for (y = 0; y < 80; y++) {
            unsigned count[16] = { 0 }, best = 2, c;

            for (x = 0; x < CELL_U; x++)
                count[cells[y][x] & 15]++;
            for (c = 2; c < 16; c++)
                if (count[c] > count[best])
                    best = c;
            words[RIPPLES + y] = rgb_of(colours[best]);
            words[DEPTHS + y] = float_word(water_depth[y]);
        }
    first = top > 0 ? (unsigned)top : 0;
    rows = (unsigned)(bottom - first) + 1;
    if (rows > STRIP_MAX)
        rows = STRIP_MAX;
    fx_draw_offset(&at_x, &at_y);       /* the band's rows are the screen's */
    for (y = 0; y < rows; y++)
        words[STRIP + y] = rgb_of(race_ground_colour(p, (float)(first + y + at_y)));
    words[H_ORIGIN_X] = float_word((float)ox);
    words[H_ORIGIN_Y] = float_word((float)oy);
    words[H_FIRST_ROW] = float_word((float)first);
    words[H_ROWS] = float_word((float)rows);
    words[H_KIND] = float_word(aqua ? 1.0f : 0.0f);
    words[H_COVER] = float_word((float)marks / (CELL_U * CELL_V));
    words[H_DEPTH] = float_word((float)(dh / c1));
    words[H_INK] = float_word((float)INK);
    words[H_TEXEL_SCALE] = float_word(1.0f / scale);
    slot = fx_draw_tweening() * 2 + (p & 1);
    if (!image[slot].id)
        image[slot] = nshvc_image_create(IMAGE_W, IMAGE_H, NSHVC_IMAGE_RGBA);
    if (!shader.id)
        shader = nshvc_shader_load("ground");
    nshvc_image_write(image[slot], words);
    fx_draw_depth_class(FX_DEPTH_GROUND);       /* a course's floor (the stunt arenas'): the ground */
    fx_draw_surface(corners, n, image[slot], shader, in, (float)ox, (float)oy);
    fx_draw_depth_class(FX_DEPTH_COURSE);
    if (kind == SURF_AQUA) {            /* its water cuts the picture: the plane in view space, row 1 . v = dh */
        double plane[3] = { m[3], m[4], m[5] };

        fx_face_cut(1, plane, dh);
    }
}
