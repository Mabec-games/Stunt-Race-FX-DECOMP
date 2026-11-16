#include <stdio.h>
/*
 * fx_face.c: the race's shapes in floats (plan stage 10, "Stage 10's 3D").
 *
 * The frame program decides what is drawn: which objects, which level of
 * detail, which faces face the eye, their order and materials. Where it is
 * drawn is made here, from the view-space points, unrounded and unheld:
 *
 *   FxModelProject   each point's view-space sum (the model's point at
 *                    $0340 + 6i plus the object's place at $0026-$002A, in
 *                    16 bits as the chip adds them) is kept by its record
 *                    $04E4 + 6i (fx_face_point);
 *   FxModelFaces     each face that faces the eye: its points and texels
 *                    (fx_face_begin, fx_face_vertex), then its material
 *                    (fx_face_end), which makes its shape;
 *   FxDrawList       the shapes drawn in order (fx_draw.c).
 *
 * The chip's own clip, projection to whole pixels and queue are not used:
 * whether a face is seen is decided here, in floats.
 *
 * A shape is the face clipped against a near plane in view space,
 * projected x = cx + 128 X / Z, y = cy + 128 Y / Z, clipped against the
 * view's rectangle, and fanned. A textured face is cut by planes of equal
 * depth until each piece's far over near depth is at most 1.25, so each
 * piece's texture, laid on by the GPU across the screen, stays near its
 * place in space. Nothing here makes a pixel.
 */
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "fx/fx.h"
#include "fx/fx_draw.h"
#include "fx/fx_face.h"
#include "fx/fx_ground.h"
#include "system/state.h"
#include "video/picture.h"
#include "fx/fx_draw.h"
#include "fx/fx_far.h"
#include "fx/fx_track.h"

enum {
    POINTS = 256,               /* a model's point records ($04E4 + 6i, i a byte) */
    MOST = 64,                  /* a face's points: the chip's are 2 to 8, a solid's cap up to 60 */
    WORK = MOST + 8,            /* each clip adds at most one */
    SHAPES = 512,
    CORNERS = 32768,
    TABLE = 0x04E4
};

#define FOCAL 128.0             /* FxRatioClamp: (R1 / 2) * 256 / R3; the far table the same */
#define NEAR_Z 1.0              /* the near plane: the chip's clip is at 0, its depth 0 taken as 1 */
#define CUT 1.25                /* a textured piece's far over near depth */
#define CHIP_FAR 17406.0        /* $43FE: the depth FxModelProject's far table stops at */
#define HOLE_LEAST 4096.0       /* a hole in the water: at least this much of the plane (64 x 64) */

/* A word of the chip's RAM, read without moving its last RAM address. */
static uint16_t word(uint16_t a)
{
    return (uint16_t)(fx_ram[fx_ram_at(a)] | fx_ram[fx_ram_at((uint16_t)(a ^ 1))] << 8);
}

/* ---- the points, as the frame program projected them ---- */

static struct {
    double x, y, z;
    uint32_t made;
} point[POINTS];
static uint32_t projection;

static FxSurveyPoint survey_sink;
static FxSurveyFace survey_face;
static void *survey_context;

void fx_face_survey(FxSurveyPoint point, FxSurveyFace face, void *context)
{
    survey_sink = point;
    survey_face = face;
    survey_context = context;
}

/* ---- the corners made exactly (docs/stuntracefx/native/DEPTH.md, 1) ----

   The chip turns a model's points in fixed point (command $04's, below
   detail 3, by a matrix packed into bytes) and rounds each sum to a whole
   unit, and halves a far object's place and turn besides, so faces on one
   plane come out up to 3 apart. Each corner is made again here in double
   from what the chip made it from: the point as its point command read it
   (fx_face_source: its bytes times the model's scale, or its words), turned
   by the object's turn (its angles with their fractions, the rows the
   chip's FxMatrixFromAngles01 makes of them, transposed; flag $2000 takes
   the world's up out after it, as the chip flattens a shadow) and the
   view's (the same rows of the view's angles at $00DE), plus the object's
   place from the eye in the world's axes (its draw record's, whole:
   fx_far_exact_place) turned by the view. The view's turn is taken only
   while the chip's own at $00E4 is it, within VIEW_STEPS of its 32768ths:
   the ground's survey draws with its own (the identity, thousands off).
   The chip's own is the same view with its arithmetic's error
   (FxMatrixFromAngles01): each sine and cosine from a 256-step table
   interpolated in a straight line, at most 2.5 of its 32768ths short of
   the curve, and the table's rounding half more; an entry is two products
   of up to three of them, each product rounded, so up to 19 off. VIEW_STEPS
   was 8: a picture in 21 to 43 had an entry 8 to 10.8 off (the race in
   both views, 2026-11-13: logged 4,877 pictures), its objects then all
   drawn from the chip's rounded corners, the cars unseated and dropped a
   tyre's radius into the road for that picture. An object whose place is
   not known keeps the chip's corners. */

enum { VIEW_ANGLES = 0x00DE, VIEW_MATRIX = 0x00E4, POINTS_AT = 0x0340, VIEW_STEPS = 24 };

static struct {
    double place[3];            /* the object's place in view space */
    int known;                  /* the object FxObjectsDraw took last has an exact place */
    double view[3][3];          /* the view's turn: row k, world axis k in view space */
    double chip_z;              /* its depth as the chip has it (its draw record's, whole): when not known */
    uint16_t object;            /* its object (fx_far_object), 0 none */
} object_now;
static struct {
    int on;                     /* this model's corners are made exactly */
    uint32_t made;              /* the model's count, which its sources carry */
    double turn[3][3];          /* row i, model axis i in view space */
} exact;
static struct {
    double v[3];
    uint32_t made;
} source[POINTS];

/* the car being seated (docs/stuntracefx/native/DEPTH.md, 5: below) */
static struct {
    uint16_t car;               /* the car the object being drawn belongs to (fx_far_car); 0 none */
    int tyre;
} seat_now;
static struct {
    uint16_t car;
    int n;                      /* its tyres seated so far */
    double sum[3];              /* their moves' sum, in view space */
    double radius;              /* their radius */
} seated;


static double turn_of(uint16_t word) { return (double)word * (2.0 * M_PI / 65536.0); }

/* The scale the corners of what is drawn now are at: whole for an object
   made exactly, else the chip's, halved fx_far_shift() times (a far
   object's) */
static int corner_shift(void) { return object_now.known ? 0 : fx_far_shift(); }

/* FxMatrixFromAngles01's rows of the angles at $0020 (z), $0022 (x) and
   $0024 (y): row k is axis k's image */
static void rows_of(double z, double x, double y, double m[3][3])
{
    double sz = sin(z), cz = cos(z), sx = sin(x), cx = cos(x), sy = sin(y), cy = cos(y);

    m[0][0] = sx * sy * sz + cx * cy; m[0][1] = sx * cy * sz - cx * sy; m[0][2] = sx * cz;
    m[1][0] = sy * cz;                m[1][1] = cy * cz;                m[1][2] = -sz;
    m[2][0] = cx * sy * sz - sx * cy; m[2][1] = cx * cy * sz + sx * sy; m[2][2] = cx * cz;
}

/* The angles the chip last built its view matrix at $00E4 from (its
   program reads them at $0020: FxMatrixFromAnglesProgram). With two players
   the race sets the first player's view, sorts it, then puts the second
   player's angles at $00DE (RaceViewForPicture, as the original's order)
   before the first player's picture is drawn: $00DE no longer matches the
   matrix that picture draws with, which left all its corners the chip's
   rounded ones (no layers: its road markings fought the road). */
static uint16_t built_angles[3];
static int built_known;

void fx_face_view_built(uint16_t a, uint16_t b, uint16_t c)
{
    built_angles[0] = a;
    built_angles[1] = b;
    built_angles[2] = c;
    built_known = 1;
}

static int view_matches(uint16_t a, uint16_t b, uint16_t c, double e[3][3])
{
    int i, j;

    rows_of(turn_of(a), turn_of(b), turn_of(c), e);
    for (i = 0; i < 3; i++)
        for (j = 0; j < 3; j++)
            if (fabs(e[i][j] - (int16_t)word((uint16_t)(VIEW_MATRIX + 6 * i + 2 * j)) / 32768.0) > VIEW_STEPS / 32768.0)
                return 0;
    return 1;
}

/* the view's turn from its angles, when the chip's own at $00E4 is it */
static int view_of(double e[3][3])
{
    return view_matches(word(VIEW_ANGLES), word((uint16_t)(VIEW_ANGLES + 2)), word((uint16_t)(VIEW_ANGLES + 4)), e)
        || (built_known && view_matches(built_angles[0], built_angles[1], built_angles[2], e));
}

static int ndefer;
static void defer_flush(void);

void fx_face_object(uint16_t record)
{
    double rel[3];
    int in_view = 0, j, k;

    if (ndefer)                         /* (the last model's held faces, in its own state) */
        defer_flush();
    object_now.known = 0;
    object_now.chip_z = record ? ldexp((int16_t)(fx_ram[(uint16_t)(record + 0x14)] | fx_ram[(uint16_t)(record + 0x15)] << 8),
                                      fx_far_shift()) : 0;     /* its view-space Z, as fx_ground.c reads a record's */
    seat_now.car = 0;
    seat_now.tyre = 0;
    {
        /* a piece that may be seen through about the player's car, known by
           its object (fx_draw.c); no car's */
        int tyre;
        uint16_t object = record ? fx_far_object(record) : 0;

        object_now.object = object;
        fx_draw_piece(object && !(record ? fx_far_car(record, &tyre) : 0) ? object : 0);
    }
    if (!fx_far_exact_place(record, rel, &in_view) || !view_of(object_now.view))
        return;
    for (j = 0; j < 3; j++) {
        object_now.place[j] = in_view ? rel[j] : 0;
        if (!in_view)
            for (k = 0; k < 3; k++)
                object_now.place[j] += rel[k] * object_now.view[k][j];
    }
    object_now.known = 1;
    seat_now.car = 0;
    seat_now.tyre = 0;
    seat_now.car = fx_far_car(record, &seat_now.tyre);
    if (seat_now.car && !seat_now.tyre && seated.car == seat_now.car && seated.n)
        for (j = 0; j < 3; j++)
            object_now.place[j] += seated.sum[j] / seated.n;
}

/* ---- the cars seated on the road (docs/stuntracefx/native/DEPTH.md, 5) ----

   The game places a car's tyres with their middles on the road and builds
   its body and shadow as low (measured 2026-11-12: tyre middles 7 under to
   9 over the road, a radius of 40.25; the body's lowest 1 to 17 under; the
   shadow 31 to 66 under), which the original hid by painting the cars after
   the road. With the depth test the cars are drawn after the course (the
   merged pass's order), so the track under them is known: the collision
   models of the pieces drawn, the ground the game drives them on
   (fx_track.c), never a face only drawn on or near it. Each tyre is moved
   along the normal of the ground under its middle to its radius over it
   (fx_face_seat_tyre); the car's other
   parts are moved by its tyres' mean move; and a face of the car whose
   corners then all lie no more than SHADOW_OVER of a tyre's radius over
   the ground under its middle (the shadow: its two faces measured 39 to 58
   under the tyres' middles, tilted with the car; the body's lowest 15 and
   its one level face, the roof, 130 over) is laid on that ground, each
   corner along the world's up, where the layers draw it over the road as
   the original drew it. */

#define SHADOW_OVER 0.3

/* the world's up in view space (world y is down: its row 1 negated) */
static void up_of(const double e[3][3], double up[3])
{
    int k;

    for (k = 0; k < 3; k++)
        up[k] = -e[1][k];
}

int fx_face_seat_tyre(float pos[3], double radius)
{
    double p[3] = { pos[0], pos[1], pos[2] }, up[3], n[3], d, h, move;
    int k;

    if (!object_now.known || !seat_now.car || !seat_now.tyre)
        return 0;
    if (seated.car != seat_now.car) {
        seated.car = seat_now.car;
        seated.n = 0;
        seated.sum[0] = seated.sum[1] = seated.sum[2] = 0;
    }
    up_of(object_now.view, up);
    if (!fx_track_ground(seat_now.car, object_now.object, p, up, n, &d))
        return 0;
    h = n[0] * p[0] + n[1] * p[1] + n[2] * p[2] - d;
    move = radius - h;
    seated.radius = radius;
    for (k = 0; k < 3; k++) {
        pos[k] = (float)(p[k] + move * n[k]);
        seated.sum[k] += move * n[k];
    }
    seated.n++;
    return 1;
}

void fx_face_picture(void)
{
    fx_track_picture();                 /* each picture's track its own, where its pieces are drawn */
    seated.car = 0;                     /* each picture's seat its own */
    seated.n = 0;
    seated.sum[0] = seated.sum[1] = seated.sum[2] = 0;
}

double fx_face_object_depth(void)
{
    return object_now.known ? object_now.place[2] : object_now.chip_z;
}

int fx_face_exact_object(double place[3], double view[3][3])
{
    if (!object_now.known)
        return 0;
    memcpy(place, object_now.place, sizeof object_now.place);
    memcpy(view, object_now.view, sizeof object_now.view);
    return 1;
}

void fx_face_model(uint16_t flags)
{
    double m[3][3], e[3][3];
    int i, j, k;

    if (ndefer)
        defer_flush();
    exact.made++;
    exact.on = 0;
    if (!object_now.known)
        return;
    if (!view_of(e)) {                  /* not the view its place was made in: the chip's numbers throughout */
        object_now.known = 0;
        return;
    }
    /* each angle byte a word negated, its fraction between steps with it */
    rows_of(turn_of((uint16_t)-(int)((word(0x0020) & 0xFF) << 8 | fx_angle_fraction(0))),
            turn_of((uint16_t)-(int)((word(0x0022) & 0xFF) << 8 | fx_angle_fraction(1))),
            turn_of((uint16_t)-(int)((word(0x0024) & 0xFF) << 8 | fx_angle_fraction(2))), m);
    for (i = 0; i < 3; i++)
        for (j = 0; j < 3; j++)
            for (exact.turn[i][j] = 0, k = 0; k < 3; k++)
                if (k != 1 || !(flags & 0x2000))
                    exact.turn[i][j] += m[k][i] * e[k][j];
    exact.on = 1;
    if (!seat_now.car && !(flags & 0x2000))
        fx_track_piece(object_now.object, object_now.place, (const double (*)[3])exact.turn);   /* the track under the cars (fx_track.c) */
}

void fx_face_source(uint16_t at, int32_t x, int32_t y, int32_t z)
{
    unsigned i = (uint16_t)(at - POINTS_AT) / 6u;

    if ((uint16_t)(at - POINTS_AT) % 6u || i >= POINTS)
        return;
    source[i].v[0] = x;
    source[i].v[1] = y;
    source[i].v[2] = z;
    source[i].made = exact.made;
}

void fx_face_point(uint16_t record, uint16_t x, uint16_t y, uint16_t z)
{
    unsigned i = (uint16_t)(record - TABLE) / 6u;

    if (survey_sink)
        survey_sink((int16_t)x, (int16_t)y, (int16_t)z, survey_context);

    if (i >= POINTS)
        return;
    if (i == 0 && ndefer)               /* (held faces drawn in their own projection) */
        defer_flush();
    if (i == 0)                         /* every projection starts at the first point */
        projection++;
    point[i].made = projection;
    if (exact.on && source[i].made == exact.made) {
        const double *s = source[i].v;
        int j;

        for (j = 0; j < 3; j++) {
            double v = object_now.place[j] + s[0] * exact.turn[0][j] + s[1] * exact.turn[1][j] + s[2] * exact.turn[2][j];

            if (j == 0)
                point[i].x = v;
            else if (j == 1)
                point[i].y = v;
            else
                point[i].z = v;
        }
        return;
    }
    /* a point no point command gave (none known): the chip's, at the
       corners' scale */
    point[i].x = ldexp((int16_t)x, fx_far_shift() - corner_shift());
    point[i].y = ldexp((int16_t)y, fx_far_shift() - corner_shift());
    point[i].z = ldexp((int16_t)z, fx_far_shift() - corner_shift());
}

uint8_t fx_face_facing(uint16_t a, uint16_t b, uint16_t c, uint8_t chip)
{
    uint16_t at[3] = { a, b, c };
    unsigned i[3], k;
    double det;

    for (k = 0; k < 3; k++) {
        if ((uint16_t)(at[k] - TABLE) % 6u)
            return chip;
        i[k] = (uint16_t)(at[k] - TABLE) / 6u;
        if (i[k] >= POINTS || !projection || point[i[k]].made != projection)
            return chip;
    }
    det = point[i[0]].x * (point[i[1]].y * point[i[2]].z - point[i[1]].z * point[i[2]].y)
        - point[i[0]].y * (point[i[1]].x * point[i[2]].z - point[i[1]].z * point[i[2]].x)
        + point[i[0]].z * (point[i[1]].x * point[i[2]].y - point[i[1]].y * point[i[2]].x);
    if (det == 0)
        return chip;
    return (uint8_t)((chip & 0x7F) | (det < 0 ? 0x80 : 0));
}

/* ---- the view: its centre and rectangle in the picture's pixels ---- */

typedef struct {
    double cx, cy, left, right, top, bottom;
} View;

/* The words GsuParamsFromDp gives the chip: the centre, the edges (the
   right one its last column, the bottom one past its last row). */
static View view_now(void)
{
    View v;

    v.cx = (int16_t)word(0x0034);
    v.cy = (int16_t)word(0x0036);
    v.left = (int16_t)word(0x0038);
    v.right = (int16_t)word(0x003A) + 1;
    v.top = (int16_t)word(0x003C);
    v.bottom = (int16_t)word(0x003E);
    return v;
}

void fx_face_view_rect(float *left, float *right, float *top, float *bottom)
{
    View v = view_now();

    *left = (float)v.left;
    *right = (float)v.right;
    *top = (float)v.top;
    *bottom = (float)v.bottom;
}

/* A view-space point's place on the picture: x = cx + 128 X / Z,
   y = cy + 128 Y / Z, unrounded and unheld (Z in front of the near plane). */
static void place(const View *v, double x, double y, double z, double *sx, double *sy)
{
    *sx = v->cx + FOCAL * x / z;
    *sy = v->cy + FOCAL * y / z;
}

/* The view's words for a rectangle of the screen: the edges as the chip
   has them (the right one its last column, the bottom one past its last
   row), the centre, and the frustum's planes as the original's are made
   (view.c): $92 = 128 * 2^14 / the half-width, $94 = 128 * 2^13 / the
   larger part above or below the centre (0x4EC4 and 0x4000 for the
   original's 208 x 128; 0x71C7 and 0x6666 for its 144 x 80). */
FxViewWords fx_view_words(int x, int y, int width, int height, int centre_y)
{
    FxViewWords w;
    int cx = x + width / 2, half_w = cx - x, half_h;
    long a, b;

    if (width - half_w > half_w)
        half_w = width - half_w;
    half_h = centre_y - y;
    if (y + height - centre_y > half_h)
        half_h = y + height - centre_y;
    if (half_w < 33)
        half_w = 33;
    if (half_h < 17)
        half_h = 17;
    a = (128L << 14) / half_w;
    b = (128L << 13) / half_h;
    w.left = (uint16_t)x;
    w.right = (uint16_t)(x + width - 1);
    w.top = (uint16_t)y;
    w.bottom = (uint16_t)(y + height);
    w.centre_x = (uint16_t)cx;
    w.centre_y = (uint16_t)centre_y;
    w.scale_32 = (uint16_t)(a > 0xFFFF ? 0xFFFF : a);
    w.scale_34 = (uint16_t)(b > 0xFFFF ? 0xFFFF : b);
    return w;
}

/* ---- the checks' count ---- */

static int checking;
static FxFaceCheck counted;

void fx_face_check_on(int o) { checking = o; }
FxFaceCheck fx_face_check(void) { return counted; }

/* A point the chip put wholly on the screen, against its own record: the
   chip floors (or for a near point ceils by sign), so within a pixel. */
static void check_point(unsigned i, const View *v)
{
    uint16_t at = (uint16_t)(TABLE + 6 * i);
    double x, y, dx, dy;

    if (point[i].z <= 0 || point[i].z >= CHIP_FAR || (word((uint16_t)(at + 4)) & 0x00FF))
        return;
    place(v, point[i].x, point[i].y, point[i].z, &x, &y);
    dx = fabs(x - (int16_t)word(at));
    dy = fabs(y - (int16_t)word((uint16_t)(at + 2)));
    counted.vertices++;
    if (point[i].z < 256) {
        counted.near++;
        if (dx > 1.02 || dy > 1.02)
            counted.near_apart++;
    }
    /* the chip's own error: its floor, and a near point's halved pair
       (128 / z), or a far point's table entry, truncated and taken at the
       depth rounded down to even (|x| / 65536 + 128 |x| / z^2) */
    {
        double z = point[i].z, bx, by;

        if (z < 256) {
            bx = by = 1.02 + FOCAL / z;
        } else {
            bx = 1.02 + fabs(point[i].x) / 65536 + FOCAL * fabs(point[i].x) / (z * z);
            by = 1.02 + fabs(point[i].y) / 65536 + FOCAL * fabs(point[i].y) / (z * z);
        }
        if (dx > bx || dy > by)
            counted.past_bound++;
    }
    if (dx > 1.02 || dy > 1.02) {
        counted.apart++;
        if (dx > counted.worst || dy > counted.worst) {
            counted.worst = dx > dy ? dx : dy;
            counted.worst_face = counted.faces;
        }
    }
}

/* ---- the face being queued ---- */

typedef struct {
    double x, y, z, u, v;
} Corner3;
static void hole_note(const Corner3 *c, int n);

/* The colour FxModelMaterial chose last (fx_face_material), taken by the
   face queued next. */
typedef struct {
    int8_t kind;
    uint8_t tier;
    uint16_t index;
    float light;
    uint16_t row;               /* a lit face's shade row in its own tier's table, as the chip read it */
    int16_t sum;                /* and its light's sum before the chip's shifts */
} Tint;

static Tint chosen = { -1, 0, 0, 0, 0, 0 };

static struct {
    int open, textured, whole, n;
    View view;
    Tint tint;
    Corner3 c[MOST];
    int back;                   /* begun by fx_face_begin_back: it faces away */
} face;

static FxShape shapes[SHAPES];
static int nshapes;
static FxCorner corners[CORNERS];
static uint32_t ncorners;

static void commit(const FxMaterial *m);
static int commit_opening;      /* the face committed is a tunnel exit's opening (defer_flush) */

/* The model's distance tier is the place of its colour table ($0052) past
   the tables' base ($004E), $88 bytes a tier (FxModelProject), or of its
   light table among the four of its set ($2C bytes each: $A1C5 near, $A275
   far). A lit face's row is kept as the nearest tier's, the haze added
   apart. */
void fx_face_material(int kind, uint16_t index, uint16_t table, uint16_t light)
{
    unsigned tier = (uint16_t)(word(0x0052) - word(0x004E)) / 0x88u;

    chosen.kind = (int8_t)kind;
    chosen.index = index;
    chosen.light = 0;
    if (kind == 1) {
        uint16_t set = table >= 0xA275 ? 0xA275 : 0xA1C5, at = (uint16_t)(set + 2 * index), own = (uint16_t)(table + 2 * index);

        tier = (uint16_t)(table - set) / 0x2Cu;
        chosen.index = (uint16_t)(fx_rom_read(3, at) | fx_rom_read(3, (uint16_t)(at + 1)) << 8);
        chosen.light = (float)((int16_t)light / 1024.0);
        chosen.row = (uint16_t)(fx_rom_read(3, own) | fx_rom_read(3, (uint16_t)(own + 1)) << 8);
        chosen.sum = (int16_t)light;
    }
    chosen.tier = (uint8_t)(tier > 3 ? 3 : tier);
}

void fx_face_begin(int textured)
{
    face.back = 0;
    face.open = 1;
    face.textured = textured != 0;
    face.tint = chosen;
    chosen.kind = -1;
    face.whole = 1;
    face.n = 0;
    face.view = view_now();
    if (checking)
        counted.faces++;
}

void fx_face_vertex(uint8_t p, uint8_t u, uint8_t v)
{
    Corner3 *c;

    if (!face.open)
        return;
    if (face.n == MOST || point[p].made != projection || !projection) {
        face.whole = 0;
        return;
    }
    c = &face.c[face.n++];
    c->x = point[p].x;
    c->y = point[p].y;
    c->z = point[p].z;
    c->u = u;
    c->v = v;
    if (checking)
        check_point(p, &face.view);
}

/* ---- faces seen from behind (docs/stuntracefx/native/DEPTH.md, 4) ----

   A face is known by its corners' places, exactly (a key of each corner's
   three doubles): two faces share an edge when their corners there are the
   same points, whatever the point's number in whichever projection of the
   model. */

enum { WALKED = 512, WALKED_CORNERS = 8, HELD = 192 };

static struct {
    int n;
    uint32_t key[WALKED_CORNERS];
} walked[WALKED];
static int nwalked, walked_lost;        /* the faces the model's walk reached; whether some did not fit */
static struct {
    int n, textured;
    View view;
    Tint tint;
    FxMaterial material;
    Corner3 c[WALKED_CORNERS];
    uint32_t key[WALKED_CORNERS];
} held[HELD];
static int nheld;

static uint32_t corner_key(const Corner3 *c)
{
    double v[3] = { c->x + 0.0, c->y + 0.0, c->z + 0.0 };     /* -0 as 0 */
    const uint8_t *b = (const uint8_t *)v;
    uint32_t h = 2166136261u;
    size_t i;

    for (i = 0; i < sizeof v; i++)
        h = (h ^ b[i]) * 16777619u;
    return h;
}

void fx_face_begin_back(int textured)
{
    fx_face_begin(textured);
    face.back = 1;
}

/* a held face noted for its model's closing (fx_face.c's mouths: a plug,
   not drawn, leaves its model open) */
static void walked_note_of(const Corner3 *c, int n)
{
    int k;

    if (n < 3 || n > WALKED_CORNERS)
        return;
    if (nwalked == WALKED) {
        walked_lost = 1;
        return;
    }
    walked[nwalked].n = n;
    for (k = 0; k < n; k++)
        walked[nwalked].key[k] = corner_key(&c[k]);
    nwalked++;
}

/* the face just ended, noted for its model's closing */
static void walked_note(void)
{
    int k;

    if (face.n < 3 || face.n > WALKED_CORNERS || !face.whole)
        return;
    if (nwalked == WALKED) {
        walked_lost = 1;
        return;
    }
    walked[nwalked].n = face.n;
    for (k = 0; k < face.n; k++)
        walked[nwalked].key[k] = corner_key(&face.c[k]);
    nwalked++;
}


/* ---- the tunnels' mouths (SRFX reboot) ----
   A tunnel piece's entrance is a facade of flat faces laid right across
   the opening, with a dark plug painted over the opening after them in the
   same plane: the original's way of drawing a hole (its inside drawn
   later, nearer, as the car comes). Seen at true depth the plug is a wall.
   A tunnel piece's flat faces are held until its walk is done; a face
   drawn after others in their plane, every corner of it on one of them,
   is a plug: it is cut out of those faces and not drawn, so the opening
   shows the tunnel's inside from any distance. */
static uint16_t shape_now;
enum { DEFER = 64, DEFER_CORNERS = 16, CUT_MOST = MOST };
static struct {
    int n, textured;
    View view;
    Tint tint;
    FxMaterial m;
    int dark, back;
    Corner3 c[DEFER_CORNERS];
} defer[DEFER];
static int deferring;                  /* faces copied for the mouth test */
static int holding;                     /* and held: a model known to have a mouth */
static uint16_t mouth_shapes[64];
static int nmouth;

static int mouth_known(uint16_t shape)
{
    int i;
    for (i = 0; i < nmouth; i++)
        if (mouth_shapes[i] == shape)
            return 1;
    return 0;
}

static void defer_commit(int k, const Corner3 *c, int n)
{
    FxMaterial m = defer[k].m;
    int i;

    if (defer[k].back) {                /* seen from behind: to the model's held faces, as fx_face_end holds them */
        if (n < 3 || n > WALKED_CORNERS || nheld == HELD)
            return;
        held[nheld].n = n;
        held[nheld].textured = defer[k].textured;
        held[nheld].view = defer[k].view;
        held[nheld].tint = defer[k].tint;
        held[nheld].material = m;
        for (i = 0; i < n; i++) {
            held[nheld].c[i] = c[i];
            held[nheld].key[i] = corner_key(&c[i]);
        }
        nheld++;
        return;
    }
    face.open = 0;
    face.back = 0;
    face.whole = 1;
    face.n = n;
    face.textured = defer[k].textured;
    face.view = defer[k].view;
    face.tint = defer[k].tint;
    for (i = 0; i < n; i++)
        face.c[i] = c[i];
    if (fx_face_full())
        fx_draw_queued();
    commit(&m);
}

typedef struct { double n[3], d, tol; int ax, ay; } Plane;

static void plane_of(const Corner3 *c, int n, Plane *p)
{
    double big = 0, len;
    int i, k;

    p->n[0] = p->n[1] = p->n[2] = 0;
    for (i = 0; i < n; i++) {
        const Corner3 *a = &c[i], *b = &c[(i + 1) % n];
        p->n[0] += (a->y - b->y) * (a->z + b->z);
        p->n[1] += (a->z - b->z) * (a->x + b->x);
        p->n[2] += (a->x - b->x) * (a->y + b->y);
    }
    len = sqrt(p->n[0] * p->n[0] + p->n[1] * p->n[1] + p->n[2] * p->n[2]);
    if (len > 0)
        for (k = 0; k < 3; k++)
            p->n[k] /= len;
    p->d = p->n[0] * c[0].x + p->n[1] * c[0].y + p->n[2] * c[0].z;
    for (i = 0; i < n; i++) {
        double m = fmax(fabs(c[i].x), fmax(fabs(c[i].y), fabs(c[i].z)));
        if (m > big)
            big = m;
    }
    p->tol = 4.0 + big * 0.002;
    /* the 2D the cut works in: the two axes the normal leans on least */
    k = fabs(p->n[0]) > fabs(p->n[1]) ? (fabs(p->n[0]) > fabs(p->n[2]) ? 0 : 2) : (fabs(p->n[1]) > fabs(p->n[2]) ? 1 : 2);
    p->ax = k == 0 ? 1 : 0;
    p->ay = k == 2 ? 1 : 2;
}

static double axis(const Corner3 *c, int a) { return a == 0 ? c->x : a == 1 ? c->y : c->z; }

static int on_plane(const Plane *p, const Corner3 *c, int n)
{
    int i;
    for (i = 0; i < n; i++)
        if (fabs(p->n[0] * c[i].x + p->n[1] * c[i].y + p->n[2] * c[i].z - p->d) > p->tol)
            return 0;
    return 1;
}

static double area2(const Plane *p, const Corner3 *c, int n)
{
    double s = 0;
    int i;
    for (i = 0; i < n; i++) {
        const Corner3 *a = &c[i], *b = &c[(i + 1) % n];
        s += axis(a, p->ax) * axis(b, p->ay) - axis(b, p->ax) * axis(a, p->ay);
    }
    return s / 2;
}

/* point q's side of edge a-b (in 2D): > 0 left */
static double side(const Plane *p, const Corner3 *a, const Corner3 *b, const Corner3 *q)
{
    return (axis(b, p->ax) - axis(a, p->ax)) * (axis(q, p->ay) - axis(a, p->ay)) -
           (axis(b, p->ay) - axis(a, p->ay)) * (axis(q, p->ax) - axis(a, p->ax));
}

static int convex(const Plane *p, const Corner3 *c, int n)
{
    double s = area2(p, c, n);
    int i;
    for (i = 0; i < n; i++)
        if (side(p, &c[i], &c[(i + 1) % n], &c[(i + 2) % n]) * s < -1e-6 * fabs(s))
            return 0;
    return s != 0;
}

/* q inside (or on) the face c, n corners, in 2D: by the crossings */
static int inside(const Plane *p, const Corner3 *c, int n, const Corner3 *q, double slack)
{
    double s = area2(p, c, n), len;
    int i, in = 1;
    for (i = 0; i < n; i++) {
        const Corner3 *a = &c[i], *b = &c[(i + 1) % n];
        len = hypot(axis(b, p->ax) - axis(a, p->ax), axis(b, p->ay) - axis(a, p->ay));
        if ((s > 0 ? 1 : -1) * side(p, a, b, q) < -slack * len)
            in = 0;
    }
    return in;
}

static Corner3 lerp3(const Corner3 *a, const Corner3 *b, double t)
{
    Corner3 r;
    r.x = a->x + (b->x - a->x) * t;
    r.y = a->y + (b->y - a->y) * t;
    r.z = a->z + (b->z - a->z) * t;
    r.u = a->u + (b->u - a->u) * t;
    r.v = a->v + (b->v - a->v) * t;
    return r;
}

/* polygon in (n) split by the line a-b: the part on side sgn (> 0 left) */
static int clip_half(const Plane *p, const Corner3 *in, int n, const Corner3 *a, const Corner3 *b, double sgn, Corner3 *out)
{
    int i, m = 0;
    for (i = 0; i < n && m < CUT_MOST - 1; i++) {
        const Corner3 *c = &in[i], *d = &in[(i + 1) % n];
        double sc = sgn * side(p, a, b, c), sd = sgn * side(p, a, b, d);
        if (sc >= 0)
            out[m++] = *c;
        if ((sc >= 0) != (sd >= 0))
            out[m++] = lerp3(c, d, sc / (sc - sd));
    }
    return m >= 3 ? m : 0;
}

/* face k less the convex plug c (n corners), each piece drawn */
static void cut_and_commit(int k, const Plane *p, const Corner3 *plug, int pn)
{
    Corner3 rem[CUT_MOST], tmp[CUT_MOST], outp[CUT_MOST];
    double s = area2(p, plug, pn), in_sgn = s > 0 ? 1 : -1;
    int rn = defer[k].n, i, on;

    for (i = 0; i < rn; i++)
        rem[i] = defer[k].c[i];
    for (i = 0; i < pn && rn; i++) {
        const Corner3 *a = &plug[i], *b = &plug[(i + 1) % pn];
        on = clip_half(p, rem, rn, a, b, -in_sgn, outp);
        if (on && fabs(area2(p, outp, on)) > 1.0)
            defer_commit(k, outp, on);
        rn = clip_half(p, rem, rn, a, b, in_sgn, tmp);
        memcpy(rem, tmp, sizeof(Corner3) * (size_t)rn);
    }
}

static void defer_flush_now(void);

/* the held faces drawn, the face being made (its global state) kept */
static void defer_flush(void)
{
    static __typeof__(face) kept;

    kept = face;
    defer_flush_now();
    face = kept;
}

static void defer_flush_now(void)
{
    int plug[DEFER], i, j, k, n = ndefer;
    Plane pl[DEFER];

    ndefer = 0;
    for (i = 0; i < n; i++) {
        plane_of(defer[i].c, defer[i].n, &pl[i]);
        plug[i] = 0;
    }
    /* the plugs: a convex face whose corners each lie on an earlier face
       of its plane */
    for (i = 0; i < n; i++) {
        int all = 1;
        /* the tunnels' openings: six-cornered (the tunnel's cross-section)
           and upright or leaning (Aqua Tunnel's dives: the eye's up within 0.8); a car's or a piece's dark
           faces are four- or eight-cornered, or lie flat */
        if (!defer[i].dark || defer[i].n != 6 || fabs(pl[i].n[1]) > 0.8 || !convex(&pl[i], defer[i].c, defer[i].n))
            continue;
        for (k = 0; k < defer[i].n && all && 0; k++) {     /* (a facade under it not needed: Aqua's frame is round its opening) */
            int found = 0;
            for (j = 0; j < i && !found; j++)
                found = !plug[j] && fabs(pl[i].n[0] * pl[j].n[0] + pl[i].n[1] * pl[j].n[1] + pl[i].n[2] * pl[j].n[2]) > 0.995 &&
                        on_plane(&pl[i], defer[j].c, defer[j].n) &&
                        inside(&pl[i], defer[j].c, defer[j].n, &defer[i].c[k], pl[i].tol);
            all = found;
        }
        if (all) {
            /* an opening: faces of its model beyond its plane, seen through it */
            double eye = -pl[i].d, x0 = 1e30, x1 = -1e30, y0 = 1e30, y1 = -1e30;
            int behind = 0;
            for (k = 0; k < defer[i].n; k++) {
                double a = axis(&defer[i].c[k], pl[i].ax), b = axis(&defer[i].c[k], pl[i].ay);
                x0 = fmin(x0, a), x1 = fmax(x1, a), y0 = fmin(y0, b), y1 = fmax(y1, b);
            }
            if (fmax(x1 - x0, y1 - y0) < ldexp(2500.0, -corner_shift()) && !holding)
                continue;               /* (a tunnel's cross-section, 3,840 across; a car's dark panel is a tenth of it) */
            for (j = 0; j < n; j++) {
                int far = 1, over = 0;
                if (j == i)
                    continue;
                for (k = 0; k < defer[j].n; k++) {
                    const Corner3 *q = &defer[j].c[k];
                    double h = pl[i].n[0] * q->x + pl[i].n[1] * q->y + pl[i].n[2] * q->z - pl[i].d;
                    double a = axis(q, pl[i].ax), b = axis(q, pl[i].ay);
                    if (defer[i].back ? h * eye < pl[i].tol : h * eye > -pl[i].tol)
                        far = 0;                /* (on the eye's side, or in the plane; a plug seen from
                                                   inside: beyond it, the tunnel being on the eye's side) */
                    if (a > x0 && a < x1 && b > y0 && b < y1)
                        over = 1;
                }
                behind += far && over;
            }
            all = behind >= 2 || holding;      /* (a known mouth's plug: its inside may face away now) */
        }
        plug[i] = all;
    }
    if (!holding) {                     /* drawn already: only learn whether it has a mouth */
        for (i = 0; i < n; i++)
            if (plug[i] && !mouth_known(shape_now) && nmouth < 64)
                mouth_shapes[nmouth++] = shape_now;
        return;
    }
    for (i = 0; i < n; i++) {
        int cut = 0;
        if (plug[i]) {
            hole_note(defer[i].c, defer[i].n);  /* (not drawn; its edge still bounds the water's hole) */
            if (g.picture_effects & 0x08) {     /* in the tunnel: its exit, drawn as the opening to the sky outside */
                defer[i].back = 0;
                commit_opening = 1;
                defer_commit(i, defer[i].c, defer[i].n);
                commit_opening = 0;
            }
            continue;
        }
        walked_note_of(defer[i].c, defer[i].n);
        for (j = i + 1; j < n && !cut; j++)
            if (plug[j] && on_plane(&pl[j], defer[i].c, defer[i].n)) {
                cut_and_commit(i, &pl[j], defer[j].c, defer[j].n);
                cut = 1;
            }
        if (!cut)
            defer_commit(i, defer[i].c, defer[i].n);
    }
}


void fx_face_shape_now(uint16_t shape)
{
    if (ndefer)
        defer_flush();
    shape_now = shape;
    {
        static int off = -1;
        if (off < 0)
            off = getenv("SRFX_NO_MOUTH") != NULL;
        deferring = !off;
        holding = deferring && mouth_known(shape) && !survey_face && !survey_sink;   /* (the ground's survey sees every face as drawn) */
    }
}

void fx_face_end(const FxMaterial *material)
{
    int eligible;

    if (!face.open)
        return;
    face.open = 0;
    /* a face the mouths take: whole, of a size, and facing the eye (or
       either way in a known mouth, whose exit is seen from inside) */
    eligible = face.whole && face.n >= 3 && face.n <= DEFER_CORNERS && ndefer < DEFER;
    if (!(holding && eligible))
        walked_note();                  /* (a held face: noted when drawn, defer_flush) */
    if (deferring && !eligible) {
        if (ndefer && holding)          /* (the faces before it first: the model's order kept) */
            defer_flush();
    } else if (deferring) {
        int k;
        uint16_t bgr = pic.palette[fx_draw_ink(material->colour)];
        int r = (bgr & 31) << 3, gg = (bgr >> 5 & 31) << 3, b = (bgr >> 10 & 31) << 3;
        defer[ndefer].dark = !face.textured && r < 32 && gg < 32 && b < 32;     /* (near black: $0821, as the original paints a mouth) */
        defer[ndefer].n = face.n;
        defer[ndefer].back = face.back;
        defer[ndefer].textured = face.textured;
        defer[ndefer].view = face.view;
        defer[ndefer].tint = face.tint;
        defer[ndefer].m = *material;
        for (k = 0; k < face.n; k++)
            defer[ndefer].c[k] = face.c[k];
        ndefer++;
        if (holding)
            return;
    }
    if (face.back) {                    /* held until its model's walk is done */
        int k;

        if (face.n < 3 || face.n > WALKED_CORNERS || !face.whole || nheld == HELD)
            return;
        held[nheld].n = face.n;
        held[nheld].textured = face.textured;
        held[nheld].view = face.view;
        held[nheld].tint = face.tint;
        held[nheld].material = *material;
        for (k = 0; k < face.n; k++) {
            held[nheld].c[k] = face.c[k];
            held[nheld].key[k] = corner_key(&face.c[k]);
        }
        nheld++;
        return;
    }
    commit(material);
}

static int key_order(const void *a, const void *b)
{
    uint64_t x = *(const uint64_t *)a, y = *(const uint64_t *)b;

    return x < y ? -1 : x > y;
}

/* how many times edge e is in the sorted list */
static int edges_of(const uint64_t *list, int n, uint64_t e)
{
    int lo = 0, hi = n, count = 0;

    while (lo < hi) {
        int mid = (lo + hi) / 2;

        if (list[mid] < e)
            lo = mid + 1;
        else
            hi = mid;
    }
    while (lo + count < n && list[lo + count] == e)
        count++;
    return count;
}

/* whether the faces the walk reached close: every edge between two
   different points is there as often going one way as the other */
static int walked_closed(void)
{
    static uint64_t edge[WALKED * WALKED_CORNERS];
    int i, k, n = 0;

    if (walked_lost)
        return 0;
    for (i = 0; i < nwalked; i++)
        for (k = 0; k < walked[i].n; k++) {
            uint32_t a = walked[i].key[k], b = walked[i].key[(k + 1) % walked[i].n];

            if (a != b)
                edge[n++] = (uint64_t)a << 32 | b;
        }
    if (!n)
        return 0;
    qsort(edge, (size_t)n, sizeof edge[0], key_order);
    for (i = 0; i < n; i++)
        if (edges_of(edge, n, edge[i]) != edges_of(edge, n, (edge[i] << 32) | (edge[i] >> 32)))
            return 0;
    return 1;
}

/* whether held face h has a twin among the faces the walk reached: the
   same corners the other way round */
static int has_twin(int h)
{
    int i, r, k, n = held[h].n;

    for (i = 0; i < nwalked; i++) {
        if (walked[i].n != n)
            continue;
        for (r = 0; r < n; r++) {
            for (k = 0; k < n && walked[i].key[k] == held[h].key[(r - k + n) % n]; k++)
                ;
            if (k == n)
                return 1;
        }
    }
    return 0;
}

/* a lit face's colour turned to the eye: the chip's level for its light's
   sum the other sign (FxModelMaterial: the sum shifted right 2, its high
   byte, kept to 6-15), in its shade row */
static uint8_t colour_behind(Tint *t, uint8_t front)
{
    int16_t sum = (int16_t)-t->sum;
    int level = (int8_t)(uint8_t)((uint16_t)(int16_t)(sum >> 2) >> 8);

    if (t->kind != 1)
        return front;
    level = level < 6 ? 6 : level > 15 ? 15 : level;
    t->sum = sum;
    t->light = -t->light;
    return fx_rom_read(3, (uint16_t)(t->row + level - 6));
}

void fx_face_model_done(void)
{
    int h, k;

    if (ndefer)
        defer_flush();

    if (nheld && !walked_closed())
        for (h = 0; h < nheld; h++) {
            FxMaterial m = held[h].material;

            if (has_twin(h))
                continue;
            face.open = 0;
            face.back = 1;
            face.whole = 1;
            face.n = held[h].n;
            face.textured = held[h].textured;
            face.view = held[h].view;
            face.tint = held[h].tint;
            for (k = 0; k < face.n; k++)
                face.c[k] = held[h].c[k];
            if (!face.textured)
                m.colour = colour_behind(&face.tint, m.colour);
            if (fx_face_full())
                fx_draw_queued();
            commit(&m);
        }
    nheld = 0;
    nwalked = 0;
    walked_lost = 0;
}

void fx_face_reset(void)
{
    face.open = 0;
    nshapes = 0;
    ncorners = 0;
}

uint32_t fx_face_count(void) { return (uint32_t)nshapes; }

const FxShape *fx_face_at(uint32_t i) { return &shapes[i]; }

int fx_face_full(void)
{
    /* a face's most: its clipped polygon fanned, twice over when the water
       cuts it */
    return nshapes >= SHAPES - 1 || ncorners + 2u * 3u * WORK > CORNERS;
}

const FxCorner *fx_face_corners(void) { return corners; }

/* ---- making the shape ---- */

/* The part of the polygon on one side of z = at: above (keep_far) or
   below; the crossing corners interpolated in space. */
static int clip_depth(const Corner3 *in, int n, Corner3 *out, double at, int keep_far)
{
    int i, m = 0;

    for (i = 0; i < n; i++) {
        const Corner3 *a = &in[i], *b = &in[(i + 1) % n];
        int ain = keep_far ? a->z >= at : a->z <= at, bin = keep_far ? b->z >= at : b->z <= at;

        if (ain)
            out[m++] = *a;
        if (ain != bin) {
            double t = (at - a->z) / (b->z - a->z);

            out[m].x = a->x + (b->x - a->x) * t;
            out[m].y = a->y + (b->y - a->y) * t;
            out[m].z = at;
            out[m].u = a->u + (b->u - a->u) * t;
            out[m].v = a->v + (b->v - a->v) * t;
            m++;
        }
    }
    return m;
}

typedef struct {
    double x, y, u, v;
    double iz;                  /* one over its depth, whole: linear across the screen */
} Corner2;

/* The part of the projected polygon on one side of a line x = at (axis 0)
   or y = at (axis 1): keep_high keeps the side above. */
static int clip_side(const Corner2 *in, int n, Corner2 *out, int axis, double at, int keep_high)
{
    int i, m = 0;

    for (i = 0; i < n; i++) {
        const Corner2 *a = &in[i], *b = &in[(i + 1) % n];
        double av = axis ? a->y : a->x, bv = axis ? b->y : b->x;
        int ain = keep_high ? av >= at : av <= at, bin = keep_high ? bv >= at : bv <= at;

        if (ain)
            out[m++] = *a;
        if (ain != bin) {
            double t = (at - av) / (bv - av);

            out[m].x = a->x + (b->x - a->x) * t;
            out[m].y = a->y + (b->y - a->y) * t;
            out[m].u = a->u + (b->u - a->u) * t;
            out[m].v = a->v + (b->v - a->v) * t;
            out[m].iz = a->iz + (b->iz - a->iz) * t;
            if (axis)
                out[m].y = at;
            else
                out[m].x = at;
            m++;
        }
    }
    return m;
}

/* shift: the halvings the points have had (corner_shift for a model's,
   0 for the land's whole units), undone for each corner's true 1 / Z */
static void project(const Corner3 *in, int n, Corner2 *out, const View *v, int shift)
{
    int i;

    for (i = 0; i < n; i++) {
        place(v, in[i].x, in[i].y, in[i].z, &out[i].x, &out[i].y);
        out[i].u = in[i].u;
        out[i].v = in[i].v;
        out[i].iz = 1.0 / ldexp(in[i].z, shift);
    }
}

/* The projected polygon kept to the view's rectangle and fanned into the
   shape's corners. Returns 0 when the corners are full. */
static int fan(const Corner3 *in, int n, const View *v)
{
    Corner2 a[WORK], b[WORK];
    int i;

    project(in, n, a, v, corner_shift());
    n = clip_side(a, n, b, 0, v->left, 1);
    n = clip_side(b, n, a, 0, v->right, 0);
    n = clip_side(a, n, b, 1, v->top, 1);
    n = clip_side(b, n, a, 1, v->bottom, 0);
    if (n < 3)
        return 1;
    if (ncorners + (uint32_t)(n - 2) * 3 > CORNERS)
        return 0;
    for (i = 1; i + 1 < n; i++) {
        const Corner2 *t[3] = { &a[0], &a[i], &a[i + 1] };
        int j;

        for (j = 0; j < 3; j++) {
            FxCorner *c = &corners[ncorners++];

            c->x = (float)t[j]->x;
            c->y = (float)t[j]->y;
            c->u = (float)t[j]->u;
            c->v = (float)t[j]->v;
            c->iz = (float)t[j]->iz;
        }
    }
    return 1;
}

/* A solid's face (fx_solid.c): n view-space points, clipped against the
   near plane, projected and kept to the view's rectangle, fanned into
   `out`. Returns the corners made (0: nothing seen, or no room). WORK
   bounds n. */
uint32_t fx_face_triangles(const float (*p)[3], int n, FxCorner *out, uint32_t room)
{
    double q[MOST][3];
    int i, j;

    if (n < 3 || n > MOST)
        return 0;
    for (i = 0; i < n; i++)
        for (j = 0; j < 3; j++)
            q[i][j] = p[i][j];
    return fx_face_triangles_d((const double (*)[3])q, n, out, room);
}

static uint32_t triangles_at(const double (*p)[3], int n, FxCorner *out, uint32_t room, int shift);

uint32_t fx_face_triangles_d(const double (*p)[3], int n, FxCorner *out, uint32_t room)
{
    return triangles_at(p, n, out, room, corner_shift());
}

uint32_t fx_face_triangles_whole(const double (*p)[3], int n, FxCorner *out, uint32_t room)
{
    return triangles_at(p, n, out, room, 0);
}

static uint32_t triangles_at(const double (*p)[3], int n, FxCorner *out, uint32_t room, int shift)
{
    Corner3 a[WORK], b[WORK];
    Corner2 c[WORK], d[WORK];
    View v = view_now();
    int i, m;
    uint32_t k = 0;

    if (n < 3 || n > MOST)
        return 0;
    for (i = 0; i < n; i++) {
        a[i].x = p[i][0];
        a[i].y = p[i][1];
        a[i].z = p[i][2];
        a[i].u = a[i].v = 0;
    }
    m = clip_depth(a, n, b, NEAR_Z, 1);
    if (m < 3)
        return 0;
    project(b, m, c, &v, shift);
    m = clip_side(c, m, d, 0, v.left, 1);
    m = clip_side(d, m, c, 0, v.right, 0);
    m = clip_side(c, m, d, 1, v.top, 1);
    m = clip_side(d, m, c, 1, v.bottom, 0);
    if (m < 3 || (uint32_t)(m - 2) * 3 > room)
        return 0;
    for (i = 1; i + 1 < m; i++) {
        const Corner2 *t[3] = { &c[0], &c[i], &c[i + 1] };
        int j;

        for (j = 0; j < 3; j++) {
            out[k].x = (float)t[j]->x;
            out[k].y = (float)t[j]->y;
            out[k].u = out[k].v = 0;
            out[k].iz = (float)t[j]->iz;
            k++;
        }
    }
    return k;
}

/* A line: its segment kept in front of the near plane and in the view's
   rectangle (each end clipped on the line's own parameter). */
static int line(const Corner3 *in, const View *v)
{
    Corner3 p[2] = { in[0], in[1] };
    double x[2], y[2], iz[2], t0 = 0, t1 = 1, dx, dy;
    int i;

    for (i = 0; i < 2; i++)
        if (p[i].z < NEAR_Z) {
            const Corner3 *o = &p[!i];
            double t;

            if (o->z < NEAR_Z)
                return 1;
            t = (NEAR_Z - p[i].z) / (o->z - p[i].z);
            p[i].x += (o->x - p[i].x) * t;
            p[i].y += (o->y - p[i].y) * t;
            p[i].z = NEAR_Z;
        }
    for (i = 0; i < 2; i++) {
        place(v, p[i].x, p[i].y, p[i].z, &x[i], &y[i]);
        iz[i] = 1.0 / ldexp(p[i].z, corner_shift());
    }
    dx = x[1] - x[0];
    dy = y[1] - y[0];
    {
        /* Liang-Barsky against the four sides */
        double pq[4][2] = { { -dx, x[0] - v->left }, { dx, v->right - x[0] }, { -dy, y[0] - v->top }, { dy, v->bottom - y[0] } };

        for (i = 0; i < 4; i++) {
            double pp = pq[i][0], q = pq[i][1];

            if (pp == 0) {
                if (q < 0)
                    return 1;
            } else if (pp < 0) {
                if (q / pp > t1)
                    return 1;
                if (q / pp > t0)
                    t0 = q / pp;
            } else {
                if (q / pp < t0)
                    return 1;
                if (q / pp < t1)
                    t1 = q / pp;
            }
        }
    }
    if (ncorners + 2 > CORNERS)
        return 0;
    corners[ncorners].x = (float)(x[0] + dx * t0);
    corners[ncorners].y = (float)(y[0] + dy * t0);
    corners[ncorners].u = corners[ncorners].v = 0;
    corners[ncorners].iz = (float)(iz[0] + (iz[1] - iz[0]) * t0);
    ncorners++;
    corners[ncorners].x = (float)(x[0] + dx * t1);
    corners[ncorners].y = (float)(y[0] + dy * t1);
    corners[ncorners].u = corners[ncorners].v = 0;
    corners[ncorners].iz = (float)(iz[0] + (iz[1] - iz[0]) * t1);
    ncorners++;
    return 1;
}

/* ---- the water's holes (fx_ground.c: Aqua Tunnel's, Harbor City's) ----

   While a picture's water is on (fx_face_cut), its plane n . v = d in view
   space: where a model crosses the plane (a track piece going below the
   water), the region its faces' crossings enclose on the plane is a hole in
   the water, where the water is not drawn (fx_draw.c fx_draw_water).

   A far object (fx_far.c) is drawn with its view-space points halved
   fx_far_shift() times unless its corners are made exactly, whole
   (corner_shift): its points are tested against the plane at their scale
   (cut_d), and its crossings kept at full size for its hole. */

static struct {
    int on;
    double n[3], d;
} cut;

enum { HOLES = 64, HOLE_POINTS = 256, HOLE_MOST = 32 };

static FxHole holes[HOLES];
static int nholes;
static double hole_point[HOLE_POINTS][3];
static int nhole_points;
static uint32_t hole_model;             /* the projection the points are of */

void fx_face_cut(int on, const double n[3], double d)
{
    cut.on = on;
    if (on) {
        memcpy(cut.n, n, sizeof cut.n);
        cut.d = d;
    }
    nholes = 0;
    nhole_points = 0;
    hole_model = 0;
}

/* the plane's distance from the eye at the scale of the points being drawn */
static double cut_d(void)
{
    return ldexp(cut.d, -corner_shift());
}

static double side_of(const Corner3 *c)
{
    double s = cut.n[0] * c->x + cut.n[1] * c->y + cut.n[2] * c->z - cut_d();

    return cut.d > 0 ? s : -s;          /* > 0: past the water */
}

/* The points where a face's edges cross the water, kept for its model's
   hole, at full size (a far object's scaled back). */
static void hole_points_of(const Corner3 *in, int n)
{
    int i, shift = corner_shift();

    for (i = 0; i < n && nhole_points < HOLE_POINTS; i++) {
        const Corner3 *a = &in[i], *b = &in[(i + 1) % n];
        double sa = side_of(a), sb = side_of(b), t;

        if ((sa > 0) == (sb > 0) || sa == sb)
            continue;
        t = sa / (sa - sb);
        hole_point[nhole_points][0] = ldexp(a->x + (b->x - a->x) * t, shift);
        hole_point[nhole_points][1] = ldexp(a->y + (b->y - a->y) * t, shift);
        hole_point[nhole_points][2] = ldexp(a->z + (b->z - a->z) * t, shift);
        nhole_points++;
    }
}

static int hull_turn(const double *o, const double *a, const double *b)
{
    double c = (a[0] - o[0]) * (b[1] - o[1]) - (a[1] - o[1]) * (b[0] - o[0]);

    return c > 0;
}

static int by_x(const void *pa, const void *pb)
{
    const double *a = pa, *b = pb;

    return a[0] < b[0] ? -1 : a[0] > b[0] ? 1 : a[1] < b[1] ? -1 : a[1] > b[1];
}

/* The model's crossings made into a hole: their convex hull on the plane
   (in two directions across it), kept when it encloses more than a sliver
   (a road crossing the water makes a line, not a hole), clipped against the
   near plane and projected. */
static void hole_finish(void)
{
    double e1[3], e2[3], q[HOLE_POINTS][5], hull[2 * HOLE_POINTS][5], area = 0;
    Corner3 h3[HOLE_MOST + 8], c3[HOLE_MOST + 8];
    View v = view_now();
    int n = nhole_points, i, k = 0, t, m;
    FxHole *hole;

    nhole_points = 0;
    if (n < 3 || nholes == HOLES)
        return;
    /* two directions across the plane: n x (an axis not along n) */
    {
        double a[3] = { fabs(cut.n[0]) < 0.9 ? 1 : 0, fabs(cut.n[0]) < 0.9 ? 0 : 1, 0 }, l;

        e1[0] = cut.n[1] * a[2] - cut.n[2] * a[1];
        e1[1] = cut.n[2] * a[0] - cut.n[0] * a[2];
        e1[2] = cut.n[0] * a[1] - cut.n[1] * a[0];
        l = sqrt(e1[0] * e1[0] + e1[1] * e1[1] + e1[2] * e1[2]);
        for (i = 0; i < 3; i++)
            e1[i] /= l;
        e2[0] = cut.n[1] * e1[2] - cut.n[2] * e1[1];
        e2[1] = cut.n[2] * e1[0] - cut.n[0] * e1[2];
        e2[2] = cut.n[0] * e1[1] - cut.n[1] * e1[0];
    }
    for (i = 0; i < n; i++) {
        q[i][0] = hole_point[i][0] * e1[0] + hole_point[i][1] * e1[1] + hole_point[i][2] * e1[2];
        q[i][1] = hole_point[i][0] * e2[0] + hole_point[i][1] * e2[1] + hole_point[i][2] * e2[2];
        q[i][2] = hole_point[i][0];
        q[i][3] = hole_point[i][1];
        q[i][4] = hole_point[i][2];
    }
    qsort(q, (size_t)n, sizeof q[0], by_x);
    for (i = 0; i < n; i++) {           /* the lower hull, then the upper */
        while (k >= 2 && !hull_turn(hull[k - 2], hull[k - 1], q[i]))
            k--;
        memcpy(hull[k++], q[i], sizeof q[0]);
    }
    for (i = n - 2, t = k + 1; i >= 0; i--) {
        while (k >= t && !hull_turn(hull[k - 2], hull[k - 1], q[i]))
            k--;
        memcpy(hull[k++], q[i], sizeof q[0]);
    }
    k--;
    if (k < 3 || k > HOLE_MOST)
        return;
    for (i = 0; i < k; i++)
        area += hull[i][0] * hull[(i + 1) % k][1] - hull[(i + 1) % k][0] * hull[i][1];
    if (fabs(area) / 2 < HOLE_LEAST)
        return;
    for (i = 0; i < k; i++) {
        h3[i].x = hull[i][2];
        h3[i].y = hull[i][3];
        h3[i].z = hull[i][4];
        h3[i].u = h3[i].v = 0;
    }
    m = clip_depth(h3, k, c3, NEAR_Z, 1);
    if (m < 3)
        return;
    hole = &holes[nholes];
    hole->n = 0;
    for (i = 0; i < m; i++) {
        double x, y;
        float fx, fy;

        place(&v, c3[i].x, c3[i].y, c3[i].z, &x, &y);
        fx = (float)x;
        fy = (float)y;
        /* a corner that lands on the one before it (two hull points the near
           plane's clip or the float rounding make one) is left out: its
           zero-length edge has every point on both of its sides, so the
           water's cut (fx_draw.c hole_clip) would keep the whole of what is
           left, the hole's inside, as water */
        if (hole->n && hole->x[hole->n - 1] == fx && hole->y[hole->n - 1] == fy)
            continue;
        hole->x[hole->n] = fx;
        hole->y[hole->n] = fy;
        hole->n++;
    }
    while (hole->n > 1 && hole->x[hole->n - 1] == hole->x[0] && hole->y[hole->n - 1] == hole->y[0])
        hole->n--;                      /* the last on the first */
    if (hole->n >= 3)
        nholes++;
}

/* a face not drawn whose crossings still bound the water's hole (a
   tunnel's plug, fx_face.c's mouths) */
static void hole_note(const Corner3 *c, int n)
{
    if (!cut.on)
        return;
    if (hole_model != projection) {
        if (nhole_points)
            hole_finish();
        hole_model = projection;
    }
    hole_points_of(c, n);
}

const FxHole *fx_face_holes(int *n)
{
    if (cut.on && nhole_points)
        hole_finish();
    *n = cut.on ? nholes : 0;
    return holes;
}

/* A polygon in view space made into the shape's corners (near-clipped, cut
   by depth when textured, fanned). Returns 0 when the corners are full. */
static int polygon_corners(const Corner3 *in, int n0, int textured)
{
    Corner3 a[WORK];
    int n, ok = 1;

    n = clip_depth(in, n0, a, NEAR_Z, 1);
    if (n >= 3 && textured) {
        double zmin = a[0].z, zmax = a[0].z, r;
        int i, m = 1;

        for (i = 1; i < n; i++) {
            if (a[i].z < zmin)
                zmin = a[i].z;
            if (a[i].z > zmax)
                zmax = a[i].z;
        }
        if (zmax > zmin * CUT) {
            m = (int)ceil(log(zmax / zmin) / log(CUT));
            if (m > 16)
                m = 16;
        }
        r = pow(zmax / zmin, 1.0 / m);
        for (i = 0; i < m && ok; i++) {
            Corner3 b[WORK], c[WORK];
            int k = n;

            memcpy(b, a, sizeof(Corner3) * (size_t)n);
            if (i > 0)
                k = clip_depth(a, k, b, zmin * pow(r, i), 1);
            if (i < m - 1) {
                k = clip_depth(b, k, c, zmin * pow(r, i + 1), 0);
                memcpy(b, c, sizeof(Corner3) * (size_t)k);
            }
            if (k >= 3)
                ok = fan(b, k, &face.view);
        }
    } else if (n >= 3) {
        ok = fan(a, n, &face.view);
    }
    return ok;
}

/* The face noted made into a shape with its material. A face not wholly
   known (a point not projected with this model's table) makes none. */
/* a face of a car (not a tyre) whose corners all lie no more than
   SHADOW_OVER of its tyres' radius over the ground under its middle: laid
   on that ground, each corner moved along the world's up onto its plane */
static void lay_on_ground(void)
{
    double up[3], c[3] = { 0, 0, 0 }, n[3], d, nu;
    int i, k;

    if (seated.car != seat_now.car || !seated.n)
        return;                         /* its tyres not seated: no ground known for it */
    up_of(object_now.view, up);
    for (i = 0; i < face.n; i++)
        c[0] += face.c[i].x / face.n, c[1] += face.c[i].y / face.n, c[2] += face.c[i].z / face.n;
    if (!fx_track_ground(seat_now.car, 0, c, up, n, &d))
        return;
    for (i = 0; i < face.n; i++)
        if (n[0] * face.c[i].x + n[1] * face.c[i].y + n[2] * face.c[i].z - d > SHADOW_OVER * seated.radius)
            return;                     /* a part of the body, over the ground */
    nu = n[0] * up[0] + n[1] * up[1] + n[2] * up[2];
    for (i = 0; i < face.n; i++) {
        double p[3] = { face.c[i].x, face.c[i].y, face.c[i].z }, t = (d - (n[0] * p[0] + n[1] * p[1] + n[2] * p[2])) / nu;

        for (k = 0; k < 3; k++)
            p[k] += t * up[k];
        face.c[i].x = p[0], face.c[i].y = p[1], face.c[i].z = p[2];
    }
}

static void commit(const FxMaterial *m)
{
    FxShape *s;
    uint32_t first = ncorners;
    int ok = 1;

    if (!face.whole || face.n < 2 || nshapes == SHAPES)
        return;
    if (survey_face && face.n >= 3) {
        double c[MOST][3];
        int i;

        for (i = 0; i < face.n; i++) {
            c[i][0] = face.c[i].x;
            c[i][1] = face.c[i].y;
            c[i][2] = face.c[i].z;
        }
        survey_face((const double (*)[3])c, face.n, m->colour, face.textured, !face.textured && face.tint.kind == 1,
                    face.tint.light, survey_context);
    }
    s = &shapes[nshapes];
    s->material = *m;
    s->line = face.n == 2;
    s->tint = face.textured ? -1 : face.tint.kind;
    s->tier = face.tint.tier;
    s->tint_index = face.tint.index;
    s->light = face.tint.light;
    if (seat_now.car && !seat_now.tyre && object_now.known && face.n >= 3)
        lay_on_ground();
    s->back = (uint8_t)face.back;
    s->opening = (uint8_t)commit_opening;
    /* the GOAL letters flown at the eye past the line: the original paints
       them after the course, whole over the road (fx_draw.c) */
    s->over = shape_now == 0xA5D4 || shape_now == 0xA660 || shape_now == 0xA67C || shape_now == 0xA6D0;
    s->ncorner3 = 0;
    if (face.n <= 8) {                  /* its corners at full size, for its layer (fx_draw.c) */
        int k;

        for (k = 0; k < face.n; k++) {
            s->corner3[k][0] = ldexp(face.c[k].x, corner_shift());
            s->corner3[k][1] = ldexp(face.c[k].y, corner_shift());
            s->corner3[k][2] = ldexp(face.c[k].z, corner_shift());
        }
        s->ncorner3 = (uint8_t)face.n;
    }
    s->plane[0] = s->plane[1] = s->plane[2] = s->plane[3] = 0;
    if (face.n >= 3) {                  /* its plane, from its first three corners */
        double a[3] = { face.c[1].x - face.c[0].x, face.c[1].y - face.c[0].y, face.c[1].z - face.c[0].z };
        double b[3] = { face.c[2].x - face.c[0].x, face.c[2].y - face.c[0].y, face.c[2].z - face.c[0].z };
        double n[3] = { a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0] };

        s->plane[0] = (float)n[0], s->plane[1] = (float)n[1], s->plane[2] = (float)n[2];
        s->plane[3] = (float)((n[0] * face.c[0].x + n[1] * face.c[0].y + n[2] * face.c[0].z) * (1 << corner_shift()));
    }
    if (s->line) {
        ok = line(face.c, &face.view);
    } else if (cut.on) {
        /* the water is put in place by depth (fx_draw.c fx_draw_water):
           the face whole, its crossings kept for the holes */
        if (hole_model != projection) {
            if (nhole_points)
                hole_finish();
            hole_model = projection;
        }
        hole_points_of(face.c, face.n);
        ok = polygon_corners(face.c, face.n, face.textured);
    } else {
        ok = polygon_corners(face.c, face.n, face.textured);
    }
    if (!ok) {
        ncorners = first;
        return;
    }
    s->first = first;
    s->count = ncorners - first;
    nshapes++;
}

/* ---- scaled sprites ---- */

static struct {
    int set, fixed;
    uint16_t left, top, bx, by;
    double cx, cy, k;
} sprite;
static int mapped_fixed;                /* the sprite fx_sprite_map last gave is of fixed size */

/* FxObjectSprite's place: x, y and depth the object's view-space place,
   size the size it scaled by the depth (0 for a sprite of fixed size), the
   chip's centre and size in whole pixels after its cap. */
void fx_sprite_place(uint16_t x, uint16_t y, uint16_t depth, uint16_t size, uint16_t centre_x, uint16_t centre_y, uint16_t screen_size)
{
    View v = view_now();
    double d = depth ? depth : 1, half = (screen_size >> 1) + (screen_size & 1), px = (int16_t)x, py = (int16_t)y, sz = size;

    /* the object's place made exactly, whole (fx_face_object): a far one's
       size, halved with its place by the chip, whole with it */
    if (object_now.known && object_now.place[2] > 0) {
        px = object_now.place[0];
        py = object_now.place[1];
        d = object_now.place[2];
        sz = (double)size * (1 << fx_far_shift());
    }
    sprite.set = screen_size != 0;
    sprite.fixed = size == 0;
    sprite.cx = v.cx + FOCAL * px / d;
    sprite.cy = v.cy + FOCAL * py / d;
    sprite.k = size ? (FOCAL * sz / d) / screen_size : 1.0;
    sprite.bx = centre_x;
    sprite.by = centre_y;
    sprite.left = (uint16_t)(centre_x - half);
    sprite.top = (uint16_t)(centre_y - half);
}

int fx_sprite_fixed(void) { return mapped_fixed; }

int fx_sprite_map(uint16_t left, uint16_t top, float *cx, float *cy, float *bx, float *by, float *k)
{
    if (!sprite.set || left != sprite.left || top != sprite.top)
        return 0;
    sprite.set = 0;
    mapped_fixed = sprite.fixed;
    *cx = (float)sprite.cx;
    *cy = (float)sprite.cy;
    *bx = (int16_t)sprite.bx;
    *by = (int16_t)sprite.by;
    *k = (float)sprite.k;
    return 1;
}
