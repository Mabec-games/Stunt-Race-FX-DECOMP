/*
 * fx_draw.c: the Super FX's fill work as shapes for the GPU.
 *
 * The frame program queues its polygons in Super FX RAM from $08AA. Where
 * the original chip walked each polygon's edges and plotted its rows, here
 * each shape in the queue is sent to a draw list with the engine's exact
 * fill rule, so the GPU decides the pixels.
 */
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "fx/fx.h"
#include "fx/fx_draw.h"
#include "fx/fx_far.h"
#include "fx/fx_face.h"
#include "fx/fx_ground.h"
#include "fx/fx_view.h"
#include "fx/fx_weather.h"
#include "system/state.h"
#include "system/perf.h"
#include "video/picture.h"
#include "video/video.h"

uint8_t fx_ram[0x20000];
FxState fxs;

/*
 * A picture is kept as its list of shapes, so that what is on the screen
 * can be put together from it whenever it is wanted: one player shows one
 * picture, two players show both at once in one layer.
 */
typedef struct {
    uint32_t first, count;
    uint32_t image;             /* 0: the vertices' colours */
    uint8_t lines;
    uint32_t shader;            /* 0: none; else the shader and its inputs (the ground's) */
    float in[8];
    uint32_t mesh, draw;        /* an instanced mesh (no vertices), and its draw in the recording's meshes; mesh 0 none */
    uint8_t world;              /* the 3D's world (its frame program's up to its text): magnified with the view's zoom */
    uint8_t layer;              /* faces on one plane: how many layers in front of the first (DEPTH.md, 3) */
    uint8_t back;               /* a face seen from behind: behind a face facing the eye at the same depth */
    uint8_t keep;               /* see-through (the water): tested against the depth, not written */
    uint16_t piece;             /* the piece it is of (its recording's pieces, + 1; 0 none): seen through */
    uint8_t fade;               /* drawn seen through about the player's car, at this fade (of 255); 0 not */
    float fade_z;               /* then its depth, for the order (the farthest first) */
    uint8_t ground;             /* and the ground's (piece_ground): wholly seen through at the middle */
} Shape;

/* A piece of the world that may hide the player's car (seen through, below) */
typedef struct {
    uint32_t key;               /* what it is known by from picture to picture */
    uint32_t first;             /* its first shape */
    float fade;                 /* 0 whole to 1 as seen through as it goes */
} Piece;

/* an instanced mesh's draw: its view in the recording's pixels (cx, cy with
   the picture's place on the screen), its instances in the recording's */
typedef struct {
    nshvc_view view;
    uint32_t first, count;
} MeshDraw;

typedef struct {
    nshvc_vertex *v;
    Shape *s;
    uint32_t nv, ns, cap_v, cap_s;
    uint32_t gen;               /* moves on with every change of its shapes */
    int ox, oy;                 /* the picture's place on the screen its shapes were recorded at */
    uint32_t hud_from;          /* its shapes from here on came after the frame program: the HUD's */
    /* its view's horizon in the screen's pixels: through (horizon_x,
       horizon_row), rising horizon_slope rows a column; known or not */
    float horizon_x, horizon_row, horizon_slope;
    int horizon_known;
    /* its world's shapes magnified `zoom` times about (zoom_x, zoom_y), the
       view's centre where they were recorded (fx_view: a view other than
       the game's); 1 none */
    float zoom, zoom_x, zoom_y;
    /* its instanced meshes' draws and their instances */
    MeshDraw *md;
    nshvc_instance *mi;
    uint32_t nmd, nmi, cap_md, cap_mi;
    /* its pieces (seen through, below), the one open (+ 1; 0 none), and the
       player's car's place and the radius about it in its pixels */
    Piece *pc;
    uint32_t npc, cap_pc, piece_open;
    float see_x, see_y, see_r;
} Recording;

static nshvc_draw_list target;  /* a list drawn into directly (the checks) */
static uint32_t target_w, target_h;
static Recording recordings[2], *recording;
/* The pictures between two steps (fx_tween.c): picture n's 3D drawn again
   from the state between, shown with the step's own HUD; on while a race
   draws them. */
static Recording between[2];
static int between_on[2], tweening, tween_drawn[2];
/* Each panel's HUD (the shapes after the frame program) as the last two
   steps left it, to ease between: the HUD is drawn once a step (every 6
   updates), and its moving words are eased between the two by the picture's
   progress through the step (hud_eased), as the 3D is. Vertices are kept
   with each shape's first relative to the copy. */
typedef struct {
    Shape *s;
    nshvc_vertex *v;
    uint32_t ns, nv, cap_s, cap_v;
    int ox, oy;
} HudCopy;
static HudCopy hud_was[2], hud_now[2];
static float tween_t = -1.0f;           /* the picture between's progress through its step; -1 none */

/* the HUD of recording r into c (empty when it has a mesh: not a flat HUD) */
static void hud_copy_take(HudCopy *c, const Recording *r)
{
    uint32_t i, nv = 0;

    c->ns = c->nv = 0;
    c->ox = r->ox;
    c->oy = r->oy;
    if (r->hud_from >= r->ns)
        return;
    for (i = r->hud_from; i < r->ns; i++) {
        if (r->s[i].mesh)
            return;
        nv += r->s[i].count;
    }
    if (r->ns - r->hud_from > c->cap_s) {
        c->cap_s = r->ns - r->hud_from;
        c->s = realloc(c->s, c->cap_s * sizeof *c->s);
    }
    if (nv > c->cap_v) {
        c->cap_v = nv;
        c->v = realloc(c->v, c->cap_v * sizeof *c->v);
    }
    if (!c->s || !c->v) {
        c->cap_s = c->cap_v = 0;
        return;
    }
    for (i = r->hud_from; i < r->ns; i++) {
        Shape *d = &c->s[c->ns++];

        *d = r->s[i];
        memcpy(c->v + c->nv, r->v + r->s[i].first, r->s[i].count * sizeof *c->v);
        d->first = c->nv;
        c->nv += r->s[i].count;
    }
}

/* the pictures a step drew its panels into: the last two pictures begun
   (a step's panels are its last), and at a step's end its own, in order */
static struct {
    int n, picture[2];
} panels_now, panels_last;
static void blocks_flush(void);
/* The depth-tested 3D's depths (docs/stuntracefx/native/DEPTH.md, 2): the
   list's depth its own, 1 nearest; the world's z = DEPTH_NEAR / Z (its
   near plane, fx_face.c NEAR_Z: z at most 1); the ground's the same times
   GROUND_SCALE, a range of its own (as a depth range per pass is): all of
   it lies farther than anything of the world nearer than DEPTH_NEAR /
   GROUND_SCALE (10,000,000), and a float's steps are as fine relative to
   its depths as to the world's; the 2D 1, the nearest. */
#define DEPTH_NEAR 1.0
#define GROUND_SCALE 1e-7
enum { LAYER_MOST = 63 };       /* the most layers a face is drawn in front of (depth_state) */
static struct {
    Recording *recording;
    int offset_x, offset_y;
} before_tween;
static int offset_x, offset_y;
static int drawing_3d;          /* the recording's 3D is being drawn: from its frame program's start to the HUD's */
static uint8_t layer_now, back_now;     /* the layer and side of what is emitted now (shape_draw) */
static int depth_class = FX_DEPTH_COURSE;   /* the depths of what is emitted now (fx_draw_depth_class) */
static void layers_reset(void);
static void depth_state(nshvc_draw_list list, int layer, int back, int keep, int cull);
static void water_last(Recording *r);
static void piece_close(Recording *r);
static void see_split(Recording *r);
static void see_picture(void);
static int see_on(void);
static nshvc_shader fade_shader;    /* shaders/fade.shade (seen through), once loaded */
static int water_placed;        /* this picture's water is after its solid 3D (fx_draw_water) */
static int world_done;          /* the 3D's world is drawn (fx_draw_world_end): its text follows */
static int held_by_view;        /* the object drawn now is placed in the view's space (fx_draw_held_by_view) */
static int pinned;              /* a fixed-size sprite's quad, emitted where a zoomed view puts it (quad) */
static int origin_x, origin_y;  /* where a screen other than the race's puts the picture; 0, 0: none */
static int palette_row;         /* the 16 colours a 4-bit picture's tiles show it in */
/* The list's pixels per screen pixel (target: the checks' list; screen: the
   layer's, which the frame art drawn into it scales by too). */
static float target_sx = 1, target_sy = 1, screen_sx = 1, screen_sy = 1;

void fx_draw_list_scale(float *sx, float *sy)
{
    *sx = screen_sx;
    *sy = screen_sy;
}

void fx_draw_world_scale(float *sx, float *sy)
{
    float z = recording && drawing_3d && !world_done && !held_by_view && recording->zoom > 0 ? recording->zoom : 1.0f;

    *sx = screen_sx * z;
    *sy = screen_sy * z;
}


void fx_draw_set_palette_row(int row)
{
    palette_row = row;
}

/* The bits a pixel keeps at the screen's depth (the screen mode's low two
   bits: 3 eight, 1 four, 0 two), and the palette entry they show. A pixel
   whose kept bits are 0 is not plotted. */
static uint8_t depth_mask(void)
{
    int depth = g.fx_screen_mode & 3;

    return depth == 3 ? 0xFF : depth == 1 ? 0x0F : 0x03;
}

static uint8_t ink(uint8_t c)
{
    uint8_t mask = depth_mask();

    if (!(c & mask))
        return 0;
    return mask == 0xFF ? c : (uint8_t)(palette_row * 16 + (c & mask));
}

uint8_t fx_draw_ink(uint8_t c)
{
    return ink(c);
}

/* The palette as an image of one row, texel k being entry k. A vertex's
   colour is a colour (engine 6.0), no longer a palette entry, so a shape in
   one entry is drawn as that entry's texel of this image: through the
   palette as the picture has it on each row, as the textures' texels are
   (two players' halves change entries at their line). */
static nshvc_image entries;

static uint32_t entry_image(void)
{
    if (!entries.id) {
        uint8_t t[256];
        int k;

        for (k = 0; k < 256; k++)
            t[k] = (uint8_t)k;
        entries = nshvc_image_create(256, 1, NSHVC_IMAGE_INDEXED);
        if (entries.id)
            nshvc_image_write(entries, t);
    }
    return entries.id;
}

/* a vertex in entry e: its texel's middle, the same at every vertex, so
   the whole shape reads that one texel */
static void at_entry(nshvc_vertex *v, uint8_t e)
{
    v->u = (float)e + 0.5f;
    v->v = 0.5f;
}
static uint64_t composed_key[2];
static nshvc_draw_list screen_list;
static int composed_picture = -1, composed_split = -1;

void fx_draw_set_target(nshvc_draw_list list, uint32_t width, uint32_t height)
{
    target = list;
    target_w = width;
    target_h = height;
    target_sx = width / 256.0f;
    target_sy = height / 224.0f;
    recording = NULL;
    drawing_3d = 0;
    offset_x = offset_y = 0;
}

void fx_draw_init(void)
{
    screen_list = nshvc_draw_list_create();
    race_two_full_window_ready(1);      /* each player's 3D drawn in its rectangle (video's interim switch) */
}

/* Where each picture's pixel (x, y) goes on the screen. */
static int picture_now;

int fx_draw_picture_now(void) { return picture_now; }

void fx_draw_offset(int *x, int *y)
{
    *x = recording ? recording->ox : offset_x;
    *y = recording ? recording->oy : offset_y;
}
int fx_draw_tweening(void) { return tweening != 0; }

void fx_draw_begin(int picture)
{
    picture &= 1;
    picture_now = picture;
    drawing_3d = 0;                     /* its view is known at its frame program's start */
    world_done = 0;
    target.id = 0;
    depth_class = FX_DEPTH_COURSE;
    recording = tweening ? &between[picture] : &recordings[picture];
    recording->zoom = fx_view_zoom();   /* its centre with its horizon (horizon_of_view) */
    recording->zoom_x = recording->zoom_y = 0;
    if (tweening)
        tween_drawn[picture] = 1;
    else {
        between_on[picture] = 0;        /* the game's own picture, until one between replaces it */
        panels_now.picture[0] = panels_now.picture[1];
        panels_now.picture[1] = picture;
        if (panels_now.n < 2)
            panels_now.n++;
    }
    if (race_full_window()) {
        /* the view's words are the screen's (fx_view_words): its pixels */
        offset_x = 0;
        offset_y = 0;
    } else if (race_two_full_window()) {
        /* each player's words are its rectangle's, from its top left */
        int w, h;

        screen_two_rect(picture, &offset_x, &offset_y, &w, &h);
    } else if (!g.split_screen) {
        offset_x = 24;
        offset_y = 32;
    } else if (!picture) {
        offset_x = 16;
        offset_y = 23;
    } else {
        offset_x = 96;
        offset_y = 127;
    }
    if (origin_x || origin_y) {
        offset_x = origin_x;
        offset_y = origin_y;
    }
    recording->ox = offset_x;
    recording->oy = offset_y;
    fx_face_cut(0, NULL, 0);
    recording->gen++;
}

/* fx_tween.c's picture between two steps: drawn into its own recording */
void fx_draw_tween_on(float t)
{
    tween_t = t;
    before_tween.recording = recording;
    before_tween.offset_x = offset_x;
    before_tween.offset_y = offset_y;
    tween_drawn[0] = tween_drawn[1] = 0;
    tweening = 1;
}

void fx_draw_tween_shown(void)
{
    int p;

    blocks_flush();
    tweening = 0;
    for (p = 0; p < 2; p++)
        if (tween_drawn[p])
            between_on[p] = 1;
    recording = before_tween.recording;
    offset_x = before_tween.offset_x;
    offset_y = before_tween.offset_y;
}

/* The screen's list made again for what it shows, with the pictures
   between in it: the screen sets its layer to the list only when a picture
   is shown (race_picture_shown), not at every draw. */
/* The one picture the screen shows (one player: the buffer whose upload
   stages finished last, race_picture_shown), or -1 (two players show both;
   nothing composed yet). */
int fx_draw_shown_picture(void)
{
    return composed_split ? -1 : composed_picture;
}

void fx_draw_recompose(void)
{
    if (composed_picture >= 0)
        (void)fx_draw_picture(composed_picture);
}

void fx_draw_tween_off(void)
{
    between_on[0] = between_on[1] = 0;
    panels_now.n = panels_last.n = 0;
    hud_was[0].ns = hud_was[1].ns = hud_now[0].ns = hud_now[1].ns = 0;
}

/* A step's end: one panel (one player, the course menu), the last picture
   begun; two players' two, the last two in order. */
void fx_draw_step_end(void)
{
    int two = g.split_screen != 0;

    panels_last.n = 0;
    if (panels_now.n >= 1 + two) {
        panels_last.n = 1 + two;
        panels_last.picture[0] = panels_now.picture[two ? 0 : 1];
        panels_last.picture[1] = panels_now.picture[1];
    }
    panels_now.n = 0;
    for (int k = 0; k < 2; k++) {
        HudCopy t = hud_was[k];

        hud_was[k] = hud_now[k];        /* the step before's, the buffers swapped */
        hud_now[k] = t;
        if (k < panels_last.n)
            hud_copy_take(&hud_now[k], &recordings[panels_last.picture[k] & 1]);
        else
            hud_now[k].ns = hud_now[k].nv = 0;
    }
}

int fx_draw_step_picture(int panel)
{
    return panel < panels_last.n ? panels_last.picture[panel] : -1;
}

/* The frame program's shapes are the picture's 3D; what is drawn after it
   (the HUD's programs) is the HUD, which a picture between steps keeps. */
/* The horizon of the view the frame program draws with: where the plane
   through the eye level with the world meets the picture. The world's up
   axis in view space is row 1 of the matrix at $00E4 (n), the view's centre
   ($0034, $0036) and focal length 128 give the line
   n.x (x - cx) + n.y (y - cy) + 128 n.z = 0; magnified by the recording's
   zoom about the view's centre, its focal length 128 zoom. The centre is
   also the zoom's. */
static void horizon_of_view(Recording *r)
{
    double nx = (int16_t)(fx_ram[0x00EA] | fx_ram[0x00EB] << 8) / 32768.0;
    double ny = (int16_t)(fx_ram[0x00EC] | fx_ram[0x00ED] << 8) / 32768.0;
    double nz = (int16_t)(fx_ram[0x00EE] | fx_ram[0x00EF] << 8) / 32768.0;
    double cx = (int16_t)(fx_ram[0x0034] | fx_ram[0x0035] << 8), cy = (int16_t)(fx_ram[0x0036] | fx_ram[0x0037] << 8);

    r->zoom_x = (float)(cx + r->ox);
    r->zoom_y = (float)(cy + r->oy);
    r->horizon_known = fabs(ny) > 1e-6;
    if (!r->horizon_known)
        return;
    r->horizon_x = (float)(cx + r->ox);
    r->horizon_row = (float)(cy - 128.0 * r->zoom * nz / ny + r->oy);
    r->horizon_slope = (float)(-nx / ny);
}

int fx_draw_horizon(int picture, float *x, float *row, float *slope)
{
    const Recording *r;

    picture &= 1;
    r = between_on[picture] ? &between[picture] : &recordings[picture];
    if (!r->horizon_known)
        return 0;
    *x = r->horizon_x;
    *row = r->horizon_row;
    *slope = r->horizon_slope;
    return 1;
}

int fx_draw_horizon_now(int panel, float *x, float *row, float *slope)
{
    int two = g.split_screen != 0, picture;

    panel &= 1;
    /* two players: a panel is its half, whose picture has the half's number
       (fx_draw_begin's offsets); one player: the one view */
    if (drawing_3d && recording && (!two || recording == &recordings[panel] || recording == &between[panel])) {
        if (!recording->horizon_known)
            return 0;
        *x = recording->horizon_x;
        *row = recording->horizon_row;
        *slope = recording->horizon_slope;
        return 1;
    }
    picture = two ? panel : fx_draw_shown_picture();
    return picture >= 0 && fx_draw_horizon(picture, x, row, slope);
}

void fx_draw_part(int hud)
{
    drawing_3d = recording && !hud;
    world_done = 0;
    held_by_view = 0;
    if (!hud) {
        layers_reset();                 /* a picture's own faces on one plane */
        fx_face_picture();              /* and its own seats for the cars */
    }
    if (recording && !hud)
        horizon_of_view(recording);
    if (recording && !hud) {
        water_placed = 0;
        recording->npc = recording->piece_open = 0;
        see_picture();
    }
    if (recording && hud) {
        fx_draw_water();                /* where the frame program has not (fx_draw_water) */
        recording->hud_from = recording->ns;
    }
    if (hud)                            /* the water's holes are the 3D's */
        fx_face_cut(0, NULL, 0);
}

void fx_draw_world_end(void)
{
    blocks_flush();                     /* a block gathered in the world is the world's */
    if (recording) {
        piece_close(recording);
        see_split(recording);           /* what hides the player's car, seen through about it, last */
    }
    world_done = 1;
}

void fx_draw_held_by_view(int held)
{
    held_by_view = held != 0;
}

/* what is emitted now is the 3D's world */
static uint8_t in_world(void)
{
    return (uint8_t)(drawing_3d && !world_done && !held_by_view && !pinned);
}

void fx_draw_set_origin(int x, int y)
{
    if (x != origin_x || y != origin_y)
        fx_draw_tween_off();            /* another screen: no picture between */
    origin_x = x;
    origin_y = y;
    if (recording && (x || y)) {
        offset_x = x;
        offset_y = y;
    }
}

static int held;                /* nothing is drawn (the ground's survey) */

void fx_draw_hold(int on) { held = on; }

static int in_shape;             /* a queued face is being drawn (shape_draw) */

/* a vertex's depth as emitted: the 3D's world's its own, or where it has
   none (0) its object's (depth_default); anything else (the 2D) 1, the
   nearest */
static float depth_default(void);
static void depth_apply(nshvc_vertex *v, int world)
{
    if (!world)
        v->z = 1.0f;
    else if (v->z <= 0)
        v->z = depth_default();
}

void fx_draw_depth_class(int c) { depth_class = c; }

void fx_draw_layer(int layer)
{
    if (!in_shape)
        layer_now = (uint8_t)(layer < 0 ? 0 : layer > LAYER_MOST ? LAYER_MOST : layer);
}

float fx_draw_z(double iz)
{
    if (iz <= 0)
        return 0;
    return (float)((depth_class == FX_DEPTH_GROUND ? GROUND_SCALE : 1.0) * DEPTH_NEAR * iz);
}

/* The depth of a vertex of the 3D's world emitted with none of its own (a
   sprite, the chip's rows; docs/stuntracefx/native/DEPTH.md, 2): its
   object's (fx_face_object_depth), else the nearest. */
static float depth_default(void)
{
    double d = fx_face_object_depth();

    return d >= DEPTH_NEAR ? (float)(DEPTH_NEAR / d) : 1.0f;
}

static void emit_with(const nshvc_vertex *v, uint32_t count, uint32_t image, int lines, uint32_t shader, const float *in)
{
    Recording *r = recording;
    nshvc_material m;
    uint32_t i;
    int world = in_world();

    if (held)
        return;

    if (!r) {
        nshvc_vertex scaled[48];
        uint32_t done = 0;

        if (!target.id)
            return;
        memset(&m, 0, sizeof m);
        m.image.id = image;
        m.shader.id = shader;
        if (shader)
            memcpy(m.inputs, in, sizeof m.inputs);
        depth_state(target, layer_now, back_now, depth_class == FX_DEPTH_WATER, 0);
        while (done < count) {
            uint32_t n = count - done > 48 ? 48 : count - done;

            for (i = 0; i < n; i++) {
                scaled[i] = v[done + i];
                scaled[i].x = (scaled[i].x + (float)offset_x) * target_sx;
                scaled[i].y = (scaled[i].y + (float)offset_y) * target_sy;
                depth_apply(&scaled[i], world);
            }
            if (lines)
                nshvc_draw_lines(target, scaled, n, image ? &m : NULL);
            else
                nshvc_draw_tris(target, scaled, n, image ? &m : NULL);
            perf_draw_call(n);
            done += n;
        }
        return;
    }
    if (r->nv + count > r->cap_v) {
        r->cap_v = r->cap_v ? r->cap_v * 2 : 4096;
        while (r->nv + count > r->cap_v)
            r->cap_v *= 2;
        r->v = realloc(r->v, r->cap_v * sizeof *r->v);
    }
    if (r->ns == r->cap_s) {
        r->cap_s = r->cap_s ? r->cap_s * 2 : 1024;
        r->s = realloc(r->s, r->cap_s * sizeof *r->s);
    }
    if (!r->v || !r->s)
        abort();
    r->s[r->ns].first = r->nv;
    r->s[r->ns].count = count;
    r->s[r->ns].image = image;
    r->s[r->ns].lines = (uint8_t)lines;
    r->s[r->ns].shader = shader;
    r->s[r->ns].mesh = r->s[r->ns].draw = 0;
    r->s[r->ns].world = (uint8_t)world;
    r->s[r->ns].layer = layer_now;
    r->s[r->ns].back = back_now;
    r->s[r->ns].keep = (uint8_t)(depth_class == FX_DEPTH_WATER);
    r->s[r->ns].piece = (uint16_t)r->piece_open;
    r->s[r->ns].fade = 0;
    r->s[r->ns].fade_z = 0;
    r->s[r->ns].ground = 0;
    if (shader)
        memcpy(r->s[r->ns].in, in, sizeof r->s[r->ns].in);
    r->ns++;
    for (i = 0; i < count; i++) {
        r->v[r->nv + i] = v[i];
        r->v[r->nv + i].x += (float)offset_x;
        r->v[r->nv + i].y += (float)offset_y;
        depth_apply(&r->v[r->nv + i], world);
    }
    r->nv += count;
    r->gen++;
}

static void emit(const nshvc_vertex *v, uint32_t count, uint32_t image, int lines)
{
    emit_with(v, count, image, lines, 0, NULL);
}

/* The full window's HUD pieces: the shapes drawn after the frame program
   (the HUD's programs), in the original's picture places, are grouped into
   pieces (shapes whose boxes touch, or that are the words and lines of one
   message: hud_text_near)
   and each piece moved from the original's screen to the full window's by
   the screen's anchors (video/screen.c): the original's view offset (24,
   32), then sideways by where the piece lay against the view's middle and
   down by its middle. A piece larger than the original's view (a screen
   of static) stays where it was drawn. */
enum { PIECES = 1024 };

static void hud_pieces(const Recording *r, uint32_t from, uint32_t to, int player, float *dx, float *dy)
{
    static float box[PIECES][4];
    static int group[PIECES];
    uint32_t n = to - from, i, j, k;

    if (n > PIECES)
        n = PIECES;
    for (i = 0; i < n; i++) {
        const Shape *sh = &r->s[from + i];

        box[i][0] = box[i][2] = 1e9f;
        box[i][1] = box[i][3] = -1e9f;
        for (k = 0; k < sh->count; k++) {
            const nshvc_vertex *v = &r->v[sh->first + k];

            box[i][0] = v->x < box[i][0] ? v->x : box[i][0];
            box[i][1] = v->x > box[i][1] ? v->x : box[i][1];
            box[i][2] = v->y < box[i][2] ? v->y : box[i][2];
            box[i][3] = v->y > box[i][3] ? v->y : box[i][3];
        }
        group[i] = (int)i;
    }
    for (i = 0; i < n; i++)
        for (j = i + 1; j < n; j++)
            if (hud_text_near((int)box[i][0], (int)box[i][1], (int)box[i][2], (int)box[i][3], (int)box[j][0], (int)box[j][1],
                              (int)box[j][2], (int)box[j][3])) {
                int a = group[i], b = group[j];

                while (group[a] != a)
                    a = group[a];
                while (group[b] != b)
                    b = group[b];
                if (a != b)
                    group[b > a ? b : a] = a < b ? a : b;
            }
    for (i = 0; i < n; i++) {
        int root = (int)i;
        float x0, x1, y0, y1;

        while (group[root] != root)
            root = group[root];
        group[i] = root;
        if (root != (int)i)
            continue;
        x0 = box[i][0];
        x1 = box[i][1];
        y0 = box[i][2];
        y1 = box[i][3];
        for (j = i + 1; j < n; j++) {
            int rj = (int)j;

            while (group[rj] != rj)
                rj = group[rj];
            if (rj != root)
                continue;
            x0 = box[j][0] < x0 ? box[j][0] : x0;
            x1 = box[j][1] > x1 ? box[j][1] : x1;
            y0 = box[j][2] < y0 ? box[j][2] : y0;
            y1 = box[j][3] > y1 ? box[j][3] : y1;
        }
        if (player >= 0) {
            /* two players: the piece in its player's original 144 x 80 view
               (its recorded place less the picture's), moved where video's
               anchor puts that view for it */
            int px0 = (int)x0 - r->ox, px1 = (int)x1 - r->ox, py0 = (int)y0 - r->oy, py1 = (int)y1 - r->oy, ax, ay;

            if (x1 - x0 > 144 || y1 - y0 > 80) {
                dx[i] = dy[i] = 0;
            } else {
                screen_two_anchor(player, px0, px1 - 1, py0, py1 - 1, &ax, &ay);
                dx[i] = (float)(ax - r->ox);
                dy[i] = (float)(ay - r->oy);
            }
        } else if (x1 - x0 > 208 || y1 - y0 > 128) {
            dx[i] = dy[i] = 0;
        } else {
            dx[i] = 24.0f + (float)screen_view_anchor_x((int)x0 + 24, (int)x1 + 24 - 1);
            dy[i] = 32.0f + (float)screen_anchor_y((int)y0 + 32, (int)y1 + 32 - 1);
        }
    }
    for (i = 0; i < n; i++) {
        dx[i] = dx[group[i]];
        dy[i] = dy[group[i]];
    }
}

/* An instanced mesh into list d, its view in the picture's pixels moved by
   (ox, oy) and scaled by (sx, sy) to the list's: x = (cx + ox + 128 X / Z)
   sx, its f the y scale's, the x scale's share of it in the rotation's
   first row (the picture's pixels shown 7:6 wide), which moves only X;
   magnified `zoom` times about (zx, zy) in the picture's pixels first (a
   recording's world, Recording.zoom). */
static void mesh_scaled(nshvc_draw_list d, uint32_t mesh, const nshvc_view *pv, const nshvc_instance *in, uint32_t count,
                        float sx, float sy, float ox, float oy, float zoom, float zx, float zy, const nshvc_material *seen)
{
    nshvc_view v = *pv;
    nshvc_material m;
    nshvc_mesh me;
    int k;

    if (!count)
        return;
    v.cx = (zx + (pv->cx - zx) * zoom + ox) * sx;
    v.cy = (zy + (pv->cy - zy) * zoom + oy) * sy;
    v.f = pv->f * zoom * sy;
    for (k = 0; k < 3; k++)
        v.rot[k] = pv->rot[k] * sx / sy;
    memset(&m, 0, sizeof m);
    if (seen)
        m = *seen;
    me.id = mesh;
    nshvc_draw_mesh(d, me, &v, in, count, &m);
    perf_draw_call(count);
}

int fx_draw_mesh_slot(void)
{
    if (recording)
        return recording >= between && recording < between + 2 ? 2 + (int)(recording - between) : (int)(recording - recordings);
    return target.id ? 4 : -1;
}

static float mesh_fade, mesh_fade_z;    /* the next mesh's fade (fx_draw_mesh_seen) */

void fx_draw_mesh(nshvc_mesh mesh, const nshvc_view *view, const nshvc_instance *instances, uint32_t count)
{
    Recording *r = recording;

    if (held || !mesh.id || !count || (!target.id && !r))
        return;
    blocks_flush();
    if (!r) {
        depth_state(target, 0, 0, 0, 1);
        mesh_scaled(target, mesh.id, view, instances, count, target_sx, target_sy, (float)offset_x, (float)offset_y, 1, 0, 0, NULL);
        return;
    }
    if (r->nmi + count > r->cap_mi) {
        r->cap_mi = r->cap_mi ? r->cap_mi * 2 : 4096;
        while (r->nmi + count > r->cap_mi)
            r->cap_mi *= 2;
        r->mi = realloc(r->mi, r->cap_mi * sizeof *r->mi);
    }
    if (r->nmd == r->cap_md) {
        r->cap_md = r->cap_md ? r->cap_md * 2 : 64;
        r->md = realloc(r->md, r->cap_md * sizeof *r->md);
    }
    if (r->ns == r->cap_s) {
        r->cap_s = r->cap_s ? r->cap_s * 2 : 1024;
        r->s = realloc(r->s, r->cap_s * sizeof *r->s);
    }
    if (!r->mi || !r->md || !r->s)
        abort();
    r->md[r->nmd].view = *view;
    r->md[r->nmd].view.cx += (float)offset_x;
    r->md[r->nmd].view.cy += (float)offset_y;
    r->md[r->nmd].first = r->nmi;
    r->md[r->nmd].count = count;
    memcpy(r->mi + r->nmi, instances, count * sizeof *instances);
    r->nmi += count;
    memset(&r->s[r->ns], 0, sizeof r->s[r->ns]);
    r->s[r->ns].first = r->nv;
    r->s[r->ns].mesh = mesh.id;
    r->s[r->ns].draw = r->nmd++;
    r->s[r->ns].world = in_world();
    if (mesh_fade > 0) {                /* an instance seen through (fx_draw_mesh_seen) */
        r->s[r->ns].fade = (uint8_t)(mesh_fade * 254.0f + 1.0f);
        r->s[r->ns].keep = 1;
        r->s[r->ns].fade_z = mesh_fade_z;
    }
    r->ns++;
    r->gen++;
}

/* The draw state of a shape of the depth-tested 3D (docs/stuntracefx/
   native/DEPTH.md, 3, 4, 6): a face's layer over the faces it lies on, as
   the GPU's polygon offset (one step and one depth slope a layer, nearer); a
   face seen from behind one layer behind what faces the eye there; the
   water tested, not written; an instanced mesh one-sided. The engine holds each different offset in a
   pipeline on one backend, so the layers are whole and few (LAYER_MOST). */
static nshvc_draw_state state_now;
static uint32_t state_list;     /* the list state_now was set in; 0: none known (a list cleared sets it back to zero) */

static void depth_state(nshvc_draw_list list, int layer, int back, int keep, int cull)
{
    nshvc_draw_state st;
    float level = back ? -1.0f : (float)layer;

    memset(&st, 0, sizeof st);
    /* the faces the CPU projects are culled already, from exact geometry
       (fx_face.c); the game's instanced meshes by the GPU, each wound
       clockwise on the screen seen from outside (fx_ground.c mesh_wound) */
    st.cull = (uint8_t)(cull ? NSHVC_CULL_BACK : NSHVC_CULL_NONE);
    st.depth_keep = (uint8_t)keep;
    st.depth_bias = level;
    st.depth_slope = level;
    if (state_list == list.id && !memcmp(&st, &state_now, sizeof st))
        return;
    nshvc_draw_state_set(list, &st);
    state_now = st;
    state_list = list.id;
}

/* A list the 3D is drawn into, just cleared (its state all zero): its depth
   its own, 1 nearest (DEPTH.md, 2). */
static void depth_list(nshvc_draw_list list)
{
    if (state_list == list.id)
        state_list = 0;
    nshvc_draw_depth_own(list, 1);
    nshvc_draw_depth_order(list, NSHVC_DEPTH_NEAR_1);
}

/* Shapes [from, to) of a recording into the screen's list, scaled to its
   pixels; anchored: as the full window's HUD pieces, player -1 the one
   player's, else that of two. The world's shapes of a recording drawn from
   a zoomed view (Recording.zoom) are magnified about the view's centre
   first: the same picture as a focal length of 128 zoom. */
static void see_material(const Recording *r, const Shape *sh, nshvc_material *m);

static void compose_recording(const Recording *r, uint32_t from, uint32_t to, int anchored, int player)
{
    static nshvc_vertex *scaled;
    static uint32_t cap;
    static float dx[PIECES], dy[PIECES];
    uint32_t i, j, k;
    nshvc_material m;

    if (r->nv > cap) {
        cap = r->nv * 2;
        scaled = realloc(scaled, cap * sizeof *scaled);
        if (!scaled)
            abort();
    }
    if (to > r->ns)
        to = r->ns;
    if (from >= to)
        return;
    if (anchored)
        hud_pieces(r, from, to, player, dx, dy);
    for (i = from; i < to; i++) {
        const Shape *sh = &r->s[i];
        float ox = 0, oy = 0, zoom = sh->world && r->zoom > 0 ? r->zoom : 1.0f;

        if (anchored && i - from < PIECES) {
            ox = dx[i - from];
            oy = dy[i - from];
        }
        for (k = sh->first; k < sh->first + sh->count; k++) {
            scaled[k] = r->v[k];
            if (zoom != 1.0f) {
                scaled[k].x = r->zoom_x + (scaled[k].x - r->zoom_x) * zoom;
                scaled[k].y = r->zoom_y + (scaled[k].y - r->zoom_y) * zoom;
            }
            scaled[k].x = (scaled[k].x + ox) * screen_sx;
            scaled[k].y = (scaled[k].y + oy) * screen_sy;
        }
    }
    /* Each run of shapes drawn the same way (image, shader and its inputs,
       lines or triangles) whose vertices follow one another is one draw
       call (Jeff, 2026-11-12, the race in the 40s: a picture was 3,700 to
       6,500 calls of about four vertices each, a face of a model a call,
       and the engine's render thread took 6 to 9 ms a picture on them at
       3024 x 1964, a microsecond or so a call). Within a call the later
       triangle is drawn over the earlier at equal depths, as a later call
       is, so the picture is the same. */
    for (i = from; i < to; i = j) {
        const Shape *sh = &r->s[i];
        uint32_t count = sh->count;

        memset(&m, 0, sizeof m);
        j = i + 1;
        depth_state(screen_list, sh->layer, sh->back, sh->keep, sh->mesh != 0);
        if (sh->fade)
            see_material(r, sh, &m);
        if (sh->mesh) {
            const MeshDraw *d = &r->md[sh->draw];

            mesh_scaled(screen_list, sh->mesh, &d->view, r->mi + d->first, d->count, screen_sx, screen_sy, 0, 0,
                        sh->world && r->zoom > 0 ? r->zoom : 1.0f, r->zoom_x, r->zoom_y, sh->fade ? &m : NULL);
            continue;
        }
        for (; j < to; j++) {
            const Shape *next = &r->s[j];

            if (next->mesh || next->first != sh->first + count || next->image != sh->image || next->lines != sh->lines
                || next->shader != sh->shader || (sh->shader && memcmp(next->in, sh->in, sizeof sh->in))
                || next->layer != sh->layer || next->back != sh->back || next->keep != sh->keep || next->fade != sh->fade
                || next->ground != sh->ground)
                break;
            count += next->count;
        }
        if (!sh->fade) {
            m.image.id = sh->image;
            m.shader.id = sh->shader;
            if (sh->shader)
                memcpy(m.inputs, sh->in, sizeof m.inputs);
        }
        if (sh->lines)
            nshvc_draw_lines(screen_list, scaled + sh->first, count, sh->image || m.shader.id ? &m : NULL);
        else
            nshvc_draw_tris(screen_list, scaled + sh->first, count, sh->image || m.shader.id ? &m : NULL);
        perf_draw_call(count);
    }
}

/* The screen's list: over the whole screen (the full window's W x H, else
   256 x 224), at the window's pixels the screen covers (its pixels shown
   7:6 wide, the engine's shape; 4 times the screen when no window is known,
   as in a headless run). */
static void screen_list_size(uint32_t *w, uint32_t *h)
{
    nshvc_display d;
    const ScreenShape *sh = screen_shape();
    /* the race in the full window, a menu screen over the whole window (its
       picture's place carried by the origin), or the course screen (video
       lays its static over the whole screen in this list): the screen's
       shape, as video_draw sets it */
    int full = race_full_window() || race_two_full_window() || (pic.fills_window && !pic.race.on) || race_course_full_window();
    int sw = full ? sh->width : 256, shh = full ? sh->height : 224;
    double pw = 4.0 * sw, ph = 4.0 * shh, k;

    d = nshvc_display_info();
    if (d.width > 0 && d.height > 0) {
        k = (double)d.height / shh;
        if ((double)d.width / (sw * 7.0 / 6.0) < k)
            k = (double)d.width / (sw * 7.0 / 6.0);
        pw = sw * 7.0 / 6.0 * k;
        ph = shh * k;
    }
    if (pw > 16384)
        pw = 16384;
    if (ph > 16384)
        ph = 16384;
    *w = (uint32_t)lround(pw);
    *h = (uint32_t)lround(ph);
    if (*w < (uint32_t)sw)
        *w = (uint32_t)sw;
    if (*h < (uint32_t)shh)
        *h = (uint32_t)shh;
    screen_sx = (float)*w / (float)sw;
    screen_sy = (float)*h / (float)shh;
}

/* The HUD's words move on every picture, not only on the step's. A picture
   between steps at progress t shows the HUD that lies t of the way from the
   step before's to the newest's (the 3D's own t): each flat shape that is
   the same shape in both, found within HUD_EASE_NEAR places of its own in
   the step before's list (a message that grows a glyph a step has the
   shapes it had and more), with the same picture and its corners moved or
   grown, by no more than HUD_EASE_FAR pixels (a word sliding in or out, a
   picture rising, a counter on the move), has its corners t of the way,
   rounded to a picture pixel as the sprites are (video/between.c). Everything else shows as
   the newest step drew it: digits and letters that change, shapes that
   come or go, the banners' rising pictures and the world's shapes. *out is
   r (the picture's recording, for its place and zoom) with the eased HUD
   alone, to compose from its first shape. 0: nothing to ease. */
enum { HUD_EASE_FAR = 64, HUD_EASE_NEAR = 8 };

static int hud_eased(const Recording *r, int panel, Recording *out)
{
    static Shape *es;
    static nshvc_vertex *ev;
    static uint8_t *used;
    static uint32_t cap_s, cap_v, cap_u;
    const HudCopy *was = &hud_was[panel], *now = &hud_now[panel];
    uint32_t i, k;
    int moved = 0;

    if (tween_t < 0.0f || tween_t > 1.0f || !was->ns || !now->ns)
        return 0;
    if (now->ns > cap_s) {
        cap_s = now->ns;
        es = realloc(es, cap_s * sizeof *es);
    }
    if (now->nv > cap_v) {
        cap_v = now->nv;
        ev = realloc(ev, cap_v * sizeof *ev);
    }
    if (!es || !ev) {
        cap_s = cap_v = 0;
        return 0;
    }
    memcpy(es, now->s, now->ns * sizeof *es);
    memcpy(ev, now->v, now->nv * sizeof *ev);
    if (now->ns > cap_u) {
        cap_u = now->ns > was->ns ? now->ns : was->ns;
        used = realloc(used, cap_u);
    }
    if (was->ns > cap_u) {
        cap_u = was->ns;
        used = realloc(used, cap_u);
    }
    if (!used) {
        cap_u = 0;
        return 0;
    }
    memset(used, 0, was->ns);
    for (i = 0; i < now->ns; i++) {
        const Shape *b = &now->s[i];
        const nshvc_vertex *vb = now->v + b->first;
        uint32_t j, best = 0;
        float bd = 1e9f;

        if (!b->count || b->mesh || b->world || b->fade || b->ground || b->piece)
            continue;
        /* the same shape in the step before: the same picture, its corners
           moved or grown (within HUD_EASE_FAR); the nearest such, first */
        for (j = i > HUD_EASE_NEAR ? i - HUD_EASE_NEAR : 0; j < was->ns && j <= i + HUD_EASE_NEAR; j++) {
            const Shape *a = &was->s[j];
            const nshvc_vertex *va = was->v + a->first;
            float d = 0;

            if (used[j] || a->count != b->count || a->image != b->image || a->lines != b->lines || a->shader != b->shader ||
                (a->shader && memcmp(a->in, b->in, sizeof a->in)) || a->world || a->fade || a->ground || a->piece || a->mesh)
                continue;
            for (k = 0; k < b->count; k++) {
                float dx = fabsf(vb[k].x - va[k].x), dy = fabsf(vb[k].y - va[k].y);

                if (dx > HUD_EASE_FAR || dy > HUD_EASE_FAR)
                    break;
                d += dx + dy;
            }
            if (k < b->count)
                continue;
            if (d < bd) {
                bd = d;
                best = j;
            }
        }
        if (bd >= 1e9f)
            continue;
        used[best] = 1;
        if (bd == 0.0f)
            continue;
        {
            const nshvc_vertex *va = was->v + was->s[best].first;

            for (k = 0; k < b->count; k++) {
                nshvc_vertex *e = &ev[b->first + k];

                e->x = va[k].x + roundf((vb[k].x - va[k].x) * tween_t);
                e->y = va[k].y + roundf((vb[k].y - va[k].y) * tween_t);
                e->u = va[k].u + (vb[k].u - va[k].u) * tween_t;
                e->v = va[k].v + (vb[k].v - va[k].v) * tween_t;
            }
        }
        moved++;
    }
    if (!moved)
        return 0;
    *out = *r;
    out->s = es;
    out->v = ev;
    out->ns = now->ns;
    out->nv = now->nv;
    out->hud_from = 0;
    out->ox = now->ox;
    out->oy = now->oy;
    return 1;
}

/* Picture n: the step's own, or the 3D between steps with the step's HUD. */
static void compose_picture(int p)
{
    const Recording *r = &recordings[p];
    int other = origin_x || origin_y, one = race_full_window() && !other, two = race_two_full_window() && !other;

    Recording eased;

    if (between_on[p])
        compose_recording(&between[p], 0, between[p].ns, 0, -1);
    else
        compose_recording(r, 0, r->hud_from, 0, -1);
    if (between_on[p] && hud_eased(r, two ? p : 0, &eased))
        compose_recording(&eased, 0, eased.ns, one || two, two ? p : -1);
    else
        compose_recording(r, r->hud_from, r->ns, one || two, two ? p : -1);
}

static uint64_t picture_key(int p)
{
    return (uint64_t)recordings[p].gen << 33 ^ (uint64_t)between[p].gen << 1 ^ (uint64_t)between_on[p] ^
           (uint64_t)(between_on[p] && tween_t >= 0.0f ? (uint32_t)(tween_t * 4096.0f) : 0) << 12;
}

/* The screen's layer: picture n, or with two players both. */
nshvc_draw_list fx_draw_picture(int picture)
{
    int split = g.split_screen != 0, two = split;

    uint32_t w, h;
    static uint32_t composed_w, composed_h;
    static int composed_origin = -1;
    int origin = origin_x || origin_y;

    picture &= 1;
    screen_list_size(&w, &h);
    if (composed_picture != picture || composed_split != split || composed_w != w || composed_h != h || composed_origin != origin
        || composed_key[picture] != picture_key(picture)
        || (two && composed_key[!picture] != picture_key(!picture))) {
        composed_w = w;
        composed_h = h;
        composed_origin = origin;
        nshvc_draw_list_clear(screen_list, w, h);
        nshvc_draw_rule(screen_list, NSHVC_FILL_SMOOTH);
        depth_list(screen_list);
        /* the race's frame and panels, not on another screen's picture */
        if (!origin_x && !origin_y)
            race_frame_art_draw(screen_list);
        if (two) {
            compose_picture(0);
            compose_picture(1);
            composed_key[0] = picture_key(0);
            composed_key[1] = picture_key(1);
        } else {
            compose_picture(picture);
            composed_key[picture] = picture_key(picture);
        }
        /* the panels over the 3D in the full window, drawn as a list draws
           without a state (at the nearest depth: race_frame_art_draw_over) */
        depth_state(screen_list, 0, 0, 0, 0);
        if (!origin_x && !origin_y)
            race_frame_art_draw_over(screen_list);
        composed_picture = picture;
        composed_split = split;
    }
    return screen_list;
}

void fx_list_begin(void) {}
void fx_list_end(void) {}

void fx_op_clear(void)
{
    if (recording) {
        recording->nv = recording->ns = 0;
        recording->nmd = recording->nmi = 0;
        recording->hud_from = 0;
        recording->gen++;
        recording->npc = recording->piece_open = 0;
    } else if (target.id) {
        nshvc_draw_list_clear(target, target_w, target_h);
        nshvc_draw_rule(target, NSHVC_FILL_SMOOTH);
        depth_list(target);
    }
}

/* ---- the textures: a block of the cartridge seen as 256 x 256 texels ---- */

typedef struct {
    uint8_t bank, colour, options, ink_mask, ink_row;
    uint16_t base, mask;
    nshvc_image image;
} TexKey;

#define KNOWN_TEX 512
static TexKey known[KNOWN_TEX];
static int known_count;

static nshvc_image texture(uint8_t bank, uint16_t base, uint16_t mask, uint8_t colour, uint8_t options)
{
    static uint8_t t[256 * 256];
    int k, x, y;

    for (k = 0; k < known_count; k++)
        if (known[k].bank == bank && known[k].base == base && known[k].mask == mask
            && known[k].colour == colour && known[k].options == options
            && known[k].ink_mask == depth_mask() && known[k].ink_row == palette_row)
            return known[k].image;
    for (y = 0; y < 256; y++)
        for (x = 0; x < 256; x++) {
            uint8_t v = fx_rom_read(bank, (uint16_t)(((uint16_t)(y << 8 | x) & mask) + base)), c = v;

            if (options & 0x04)
                c = (uint8_t)((colour & 0xF0) | v >> 4);
            else if (options & 0x08)
                c = (uint8_t)((colour & 0xF0) | (v & 0x0F));
            if (!(options & 0x01) && ((options & 0x08) ? !(c & 0x0F) : !c))
                c = 0;
            t[y << 8 | x] = ink(c);
        }
    if (known_count == KNOWN_TEX)
        known_count = 0;
    known[known_count].bank = bank;
    known[known_count].base = base;
    known[known_count].mask = mask;
    known[known_count].colour = colour;
    known[known_count].options = options;
    known[known_count].ink_mask = depth_mask();
    known[known_count].ink_row = (uint8_t)palette_row;
    known[known_count].image = nshvc_image_create(256, 256, NSHVC_IMAGE_INDEXED);
    nshvc_image_write(known[known_count].image, t);
    return known[known_count++].image;
}

/* ---- quads ---- */

static void blocks_flush(void);

/* A scaled sprite's quads go to its place in floats (fx_face.c): the
   chip's whole-pixel box moved and scaled onto it, kept to the view. A
   sprite of fixed size (fx_sprite_fixed) in a zoomed view's world goes to
   its place magnified about the view's centre at its own size, kept to the
   view as magnified, and is not magnified again with the world: the same
   pixels on the screen as in the game's view. */
static struct {
    int on, fixed;
    float cx, cy, bx, by, k;
} sprite_map;

/* The vertices of a textured quad over the pixels [x0, x1) x [y0, y1), with
   u = u0 + i du and v = v0 + j dv at the centre of column i and row j: the
   corners take the texels' edges, half a step out from the first and last
   centres. */
static int hide_quads;           /* a solid is drawn in the sprite's place (fx_solid.c) */

/* A sprite's own texels, 0 to its size across and down: its quads are cut
   where their texture would go past them. The chip sampled each pixel at
   one point, always inside; a quad's last pixel drawn finer reaches up to
   a texel beyond, which is the next picture in the cartridge's sheet. */
static struct {
    int on;
    double across, down;
} texels;

/* The part [s0, s1] of [0, 1] along which a goes from a0 to a1 within
   [lo, hi]; 0 when none. */
static int keep_within(double a0, double a1, double lo, double hi, double *s0, double *s1)
{
    double p, q;

    if (a1 == a0) {
        *s0 = 0;
        *s1 = 1;
        return a0 >= lo && a0 <= hi;
    }
    p = (lo - a0) / (a1 - a0);
    q = (hi - a0) / (a1 - a0);
    *s0 = p < q ? p : q;
    *s1 = p < q ? q : p;
    if (*s0 < 0)
        *s0 = 0;
    if (*s1 > 1)
        *s1 = 1;
    return *s1 > *s0;
}

static void quad(nshvc_image image, int x0, int x1, int y0, int y1, double u0, double du, double v0, double dv)
{
    nshvc_vertex v[6];
    nshvc_material m;
    double ul = u0 - 0.5 * du, ur = u0 + ((x1 - x0) - 0.5) * du;
    double vt = v0 - 0.5 * dv, vb = v0 + ((y1 - y0) - 0.5) * dv;
    double l = x0, r = x1, t = y0, b = y1, ql = x0, qr = x1, qt = y0, qb = y1;

    if ((!target.id && !recording) || x1 <= x0 || y1 <= y0 || hide_quads)
        return;
    blocks_flush();
    if (texels.on) {
        double s0, s1, t0, t1, nu0, nu1, nv0, nv1;

        if (!keep_within(ul, ur, 0, texels.across, &s0, &s1) || !keep_within(vt, vb, 0, texels.down, &t0, &t1))
            return;
        ql = x0 + (x1 - x0) * s0;
        qr = x0 + (x1 - x0) * s1;
        qt = y0 + (y1 - y0) * t0;
        qb = y0 + (y1 - y0) * t1;
        nu0 = ul + (ur - ul) * s0;
        nu1 = ul + (ur - ul) * s1;
        nv0 = vt + (vb - vt) * t0;
        nv1 = vt + (vb - vt) * t1;
        ul = nu0;
        ur = nu1;
        vt = nv0;
        vb = nv1;
        l = ql;
        r = qr;
        t = qt;
        b = qb;
    }
    if (sprite_map.on) {
        float vl, vr, vt_, vb_, zoom = recording && in_world() && recording->zoom > 0 ? recording->zoom : 1.0f;
        double nl, nr, nt, nb, cx = sprite_map.cx, cy = sprite_map.cy;

        fx_face_view_rect(&vl, &vr, &vt_, &vb_);
        if (sprite_map.fixed && zoom != 1.0f) {
            double zx = (int16_t)(fx_ram[0x0034] | fx_ram[0x0035] << 8), zy = (int16_t)(fx_ram[0x0036] | fx_ram[0x0037] << 8);

            cx = zx + (cx - zx) * zoom;
            cy = zy + (cy - zy) * zoom;
            vl = (float)(zx + (vl - zx) * zoom);
            vr = (float)(zx + (vr - zx) * zoom);
            vt_ = (float)(zy + (vt_ - zy) * zoom);
            vb_ = (float)(zy + (vb_ - zy) * zoom);
            pinned = 1;
        }
        nl = cx + (ql - sprite_map.bx) * sprite_map.k;
        nr = cx + (qr - sprite_map.bx) * sprite_map.k;
        nt = cy + (qt - sprite_map.by) * sprite_map.k;
        nb = cy + (qb - sprite_map.by) * sprite_map.k;
        l = nl < vl ? vl : nl;
        r = nr > vr ? vr : nr;
        t = nt < vt_ ? vt_ : nt;
        b = nb > vb_ ? vb_ : nb;
        if (r <= l || b <= t) {
            pinned = 0;
            return;
        }
        {
            double u_l = ul + (ur - ul) * (l - nl) / (nr - nl), u_r = ul + (ur - ul) * (r - nl) / (nr - nl);
            double v_t = vt + (vb - vt) * (t - nt) / (nb - nt), v_b = vt + (vb - vt) * (b - nt) / (nb - nt);

            ul = u_l;
            ur = u_r;
            vt = v_t;
            vb = v_b;
        }
    }
    memset(&m, 0, sizeof m);
    m.image = image;
    memset(v, 0, sizeof v);
    v[0].x = (float)l; v[0].y = (float)t; v[0].u = (float)ul; v[0].v = (float)vt; v[0].colour = 1;
    v[1].x = (float)r; v[1].y = (float)t; v[1].u = (float)ur; v[1].v = (float)vt; v[1].colour = 1;
    v[2].x = (float)r; v[2].y = (float)b; v[2].u = (float)ur; v[2].v = (float)vb; v[2].colour = 1;
    v[3] = v[0];
    v[4] = v[2];
    v[5].x = (float)l; v[5].y = (float)b; v[5].u = (float)ul; v[5].v = (float)vb; v[5].colour = 1;
    emit(v, 6, m.image.id, 0);
    pinned = 0;
}

void fx_flat_row(uint16_t x, uint16_t y, uint16_t n)
{
    nshvc_vertex v[6];
    uint8_t colour = ink(fxs.colr);
    int i;

    if ((!target.id && !recording) || !colour)
        return;
    blocks_flush();
    memset(v, 0, sizeof v);
    v[0].x = x; v[0].y = y;
    v[1].x = (float)(x + n); v[1].y = y;
    v[2].x = (float)(x + n); v[2].y = (float)(y + 1);
    v[3] = v[0];
    v[4] = v[2];
    v[5].x = x; v[5].y = (float)(y + 1);
    for (i = 0; i < 6; i++)
        at_entry(&v[i], colour);
    emit(v, 6, entry_image(), 0);
}

/* A sprite smaller than its texture: size x size pixels, the texel at
   column k and row j being (k step / 256, j step / 256). Returns the row
   count of the original's loop. */
static uint16_t sprite_shrink(uint16_t size, uint16_t c8, uint16_t step, uint16_t left, uint16_t top, uint16_t bank, uint16_t base, uint16_t mask);
static uint16_t sprite_magnify(uint16_t c8, uint16_t step, uint16_t left, uint16_t top, uint16_t bank, uint16_t base, uint16_t mask);

uint16_t fx_sprite_shrink(uint16_t size, uint16_t c8, uint16_t step, uint16_t left, uint16_t top, uint16_t bank, uint16_t base, uint16_t mask)
{
    uint16_t r;

    sprite_map.on = fx_sprite_map(left, top, &sprite_map.cx, &sprite_map.cy, &sprite_map.bx, &sprite_map.by, &sprite_map.k);
    sprite_map.fixed = sprite_map.on && fx_sprite_fixed();
    hide_quads = fx_solid_hiding();
    texels.on = 1;
    texels.across = c8;
    texels.down = (mask & 0x8000) ? c8 + 1 : c8;   /* a mirrored sprite's columns read a row on (the chip's) */
    r = sprite_shrink(size, c8, step, left, top, bank, base, mask);
    texels.on = 0;
    hide_quads = 0;
    sprite_map.on = 0;
    return r;
}

uint16_t fx_sprite_magnify(uint16_t c8, uint16_t step, uint16_t left, uint16_t top, uint16_t bank, uint16_t base, uint16_t mask)
{
    uint16_t r;

    sprite_map.on = fx_sprite_map(left, top, &sprite_map.cx, &sprite_map.cy, &sprite_map.bx, &sprite_map.by, &sprite_map.k);
    sprite_map.fixed = sprite_map.on && fx_sprite_fixed();
    hide_quads = fx_solid_hiding();
    texels.on = 1;
    texels.across = c8;
    texels.down = (mask & 0x8000) ? c8 + 1 : c8;   /* a mirrored sprite's columns read a row on (the chip's) */
    r = sprite_magnify(c8, step, left, top, bank, base, mask);
    texels.on = 0;
    hide_quads = 0;
    sprite_map.on = 0;
    return r;
}

static uint16_t sprite_shrink(uint16_t size, uint16_t c8, uint16_t step, uint16_t left, uint16_t top, uint16_t bank, uint16_t base, uint16_t mask)
{
    nshvc_image image = texture((uint8_t)bank, base, 0xFFFF, fxs.colr, fxs.por);
    int x = (int16_t)left, y = (int16_t)top, count = size, skip = 0, bottom = fx_ldw(0x003E);
    int clip_w = (int16_t)fx_ldw(FX_SPRITE_CLIP_W), j0 = 0, j1 = size;
    double du = step / 256.0, u0;
    uint16_t skipped_accum = 0;
    int mirror = (mask & 0x8000) != 0;

    if (x < 0) {
        skip = -x;
        if (skip >= count)
            return 0;
        skipped_accum = (uint16_t)(skip * step);
        count -= skip;
        x = 0;
    }
    if (clip_w - x < 0)
        count += clip_w - x + 1;
    if ((int)top < 0 || (int16_t)top < 0)
        j0 = -(int16_t)top;
    if (y + j1 > bottom)
        j1 = bottom - y;
    if (j0 < j1 && count > 0) {
        if (!mirror) {
            u0 = (skipped_accum >> 8);
            quad(image, x, x + count, y + j0, y + j1, u0, du, j0 * du, du);
        } else {
            /* the original steps the column down through a 16-bit sum whose
               high byte is added to the last texel's address: every column
               but the first reads the next row of the block */
            double c0 = c8 - 1;

            if (skip == 0) {
                quad(image, x, x + 1, y + j0, y + j1, c0, 0, j0 * du, du);
                quad(image, x + 1, x + count, y + j0, y + j1, c0 - du, -du, j0 * du + 1, du);
            } else {
                quad(image, x, x + count, y + j0, y + j1, c0 - (double)(uint16_t)(0 - skipped_accum) / 256.0 + 256, -du, j0 * du, du);
            }
        }
    }
    return (uint16_t)(size * step);
}

/* A sprite larger than its texture: each texel covers about 65536 / step
   pixels. Returns the row error of the original's loop. */
static uint16_t sprite_magnify(uint16_t c8, uint16_t step, uint16_t left, uint16_t top, uint16_t bank, uint16_t base, uint16_t mask)
{
    nshvc_image image = texture((uint8_t)bank, base, 0xFFFF, fxs.colr, fxs.por);
    int x = (int16_t)left, y = (int16_t)top, right = fx_ldw(0x003A), bottom = fx_ldw(0x003E);
    uint32_t visible = (uint32_t)((uint32_t)step * (uint32_t)(uint16_t)(right - x)) >> 16;
    int columns = c8 < visible ? c8 : (int)visible, skipped = 0, rows, j0 = 0, j1, width;
    uint32_t frac = 0;
    double du = step / 65536.0, u0, v0;
    int mirror = (mask & 0x8000) != 0;

    if (x < 0) {
        uint32_t p = (uint32_t)step * (uint32_t)(-x);

        frac = p & 0xFFFF;
        skipped = (int)(p >> 16);
        columns -= skipped;
        x = 0;
    }
    if (columns <= 0 || !step)
        return 0;
    rows = (int)((uint32_t)(c8 - 1) * 65536u / step) + 1;
    j1 = rows;
    if (y + j1 > bottom)
        j1 = bottom - y;
    if (j1 < 0)
        j1 = 0;
    if (y < 0)
        j0 = -y;
    width = (int)(((uint64_t)columns * 65536u - frac + step - 1) / step);
    if (x + width > right + 1)
        width = right + 1 - x;
    if (j0 < j1 && width > 0) {
        v0 = (j0 * (double)step + 65535.0) / 65536.0;
        if (!mirror) {
            u0 = skipped + frac / 65536.0;
            quad(image, x, x + width, y + j0, y + j1, u0, du, v0, du);
        } else {
            u0 = c8 - 1 - skipped - frac / 65536.0 + (1.0 - 1.0 / 65536);
            quad(image, x, x + width, y + j0, y + j1, u0, -du, v0, du);
        }
    }
    return (uint16_t)(0 - (uint32_t)j1 * step);
}

/* ---- glyphs: two bits a pixel, through a table of four colours ---- */

typedef struct {
    uint8_t bank;
    uint16_t base;
    uint16_t width, rows, stride;
    uint8_t colours[4], ink_mask, ink_row;
    nshvc_image image;
} GlyphKey;

#define KNOWN_GLYPHS 4096
#define GLYPH_ROWS 128
static GlyphKey glyph_known[KNOWN_GLYPHS];
static int glyph_count;

static nshvc_image glyph_image(uint8_t bank, uint16_t base, uint16_t width, uint16_t rows, uint16_t stride, const uint8_t colours[4])
{
    static uint8_t t[256 * 256];
    int k, x, y;

    rows = GLYPH_ROWS;
    for (k = 0; k < glyph_count; k++)
        if (glyph_known[k].bank == bank && glyph_known[k].base == base && glyph_known[k].width == width
            && glyph_known[k].rows == rows && glyph_known[k].stride == stride
            && !memcmp(glyph_known[k].colours, colours, 4)
            && glyph_known[k].ink_mask == depth_mask() && glyph_known[k].ink_row == palette_row)
            return glyph_known[k].image;
    for (y = 0; y < rows; y++)
        for (x = 0; x < width; x++) {
            uint8_t b = fx_rom_read(bank, (uint16_t)(base + y * stride + x / 4));

            t[y * width + x] = ink(colours[(b >> (2 * (x & 3))) & 3]);
        }
    if (glyph_count == KNOWN_GLYPHS)
        glyph_count = 0;
    glyph_known[glyph_count].bank = bank;
    glyph_known[glyph_count].base = base;
    glyph_known[glyph_count].width = width;
    glyph_known[glyph_count].rows = rows;
    glyph_known[glyph_count].stride = stride;
    memcpy(glyph_known[glyph_count].colours, colours, 4);
    glyph_known[glyph_count].ink_mask = depth_mask();
    glyph_known[glyph_count].ink_row = (uint8_t)palette_row;
    glyph_known[glyph_count].image = nshvc_image_create(width, rows, NSHVC_IMAGE_INDEXED);
    nshvc_image_write(glyph_known[glyph_count].image, t);
    return glyph_known[glyph_count++].image;
}

/* The block being gathered from consecutive rows of a glyph. */
static struct {
    int open;
    uint8_t bank, colours[4];
    uint16_t x, y, width, rows, base, stride;
} block;

static void blocks_flush(void)
{
    if (block.open) {
        nshvc_image image = glyph_image(block.bank, block.base, block.width, block.rows, block.rows > 1 ? block.stride : 32, block.colours);

        block.open = 0;
        quad(image, block.x, block.x + block.width, block.y, block.y + block.rows, 0, 1, 0, 1);
    }
}

void fx_draw_flush(void) { blocks_flush(); }

/* A glyph row of n pixels at (x, y) from the 2-bit row at `base` in ROM
   bank `bank`, through its four colours: gathered into blocks of rows. */
void fx_glyph_row(uint16_t x, uint16_t y, uint16_t n, uint8_t bank, uint16_t base, const uint8_t colours[4])
{
    uint16_t next;

    if (block.open && block.x == x && block.width == n && block.bank == bank
        && !memcmp(block.colours, colours, 4) && y == (uint16_t)(block.y + block.rows)) {
        next = (uint16_t)(base - block.base);
        if (block.rows == 1)
            block.stride = next;
        if (next == (uint16_t)(block.stride * block.rows)) {
            block.rows++;
            return;
        }
    }
    blocks_flush();
    block.open = 1;
    block.bank = bank;
    memcpy(block.colours, colours, 4);
    block.x = x;
    block.y = y;
    block.width = n;
    block.rows = 1;
    block.base = base;
    block.stride = 0;
}

void fx_bitmap_row(uint16_t x, uint16_t y, uint16_t n, uint16_t palette, uint16_t base)
{
    uint8_t colours[4];
    int i;

    for (i = 0; i < 4; i++)
        colours[i] = fx_ram[(uint16_t)(palette + i)];
    fx_glyph_row(x, y, n, fxs.rombr, base, colours);
}

/* The scaled glyphs call fx_scaled_row at every pixel of a row, each time
   with the whole row's width: the row is sent once, at its first pixel. */
static int scaled_row_open;

void fx_scaled_row_start(void) { scaled_row_open = 1; }

/* One row of a glyph drawn scaled: pixel k shows column ((tu + k) du) / 256
   of the row at base, of the glyphs from `start` in ROM bank `bank`,
   through their four colours. */
void fx_glyph_scaled_row(uint16_t x, uint16_t y, uint16_t n, uint16_t tu, uint16_t du, uint8_t bank, uint16_t start, uint16_t base, const uint8_t colours[4])
{
    int row;
    nshvc_image image;

    if (!scaled_row_open || (!target.id && !recording) || !n)
        return;
    scaled_row_open = 0;
    blocks_flush();
    row = (uint16_t)(base - start) / 32;
    image = glyph_image(bank, start, 128, GLYPH_ROWS, 32, colours);
    quad(image, x, x + n, y, y + 1, (double)((uint32_t)tu * du) / 256.0, du / 256.0, row + 0.5, 0);
}

void fx_scaled_row(uint16_t x, uint16_t y, uint16_t n, uint16_t tu, uint16_t du, uint16_t base, uint16_t palette)
{
    uint8_t colours[4];
    int i;

    for (i = 0; i < 4; i++)
        colours[i] = fx_ram[(uint16_t)(palette + i)];
    fx_glyph_scaled_row(x, y, n, tu, du, fxs.rombr, (uint16_t)(fx_ram[0x0216] | fx_ram[0x0217] << 8), base, colours);
}

/* The static: rows from a square of noise, the original's own generator's,
   made once. */
static nshvc_image noise_image;

static uint16_t noise_step(uint16_t s)
{
    uint16_t t = (uint16_t)(s ^ (s << 8)), r;

    t = (uint16_t)(t >> 8 | t << 8);
    r = (uint16_t)(((uint16_t)((uint16_t)(s & 0xFF) << 1) ^ t) >> 1);
    r ^= 0xFF00;
    if ((((s & 0xFF) << 1) ^ t) & 1)
        r ^= 0x8180;
    else if (r == 0xAA55)
        r = 0;
    return r;
}

/* the seed each line of the static's 16 x 128 block was last drawn with */
static uint16_t static_seed[128];

/* The static's square of noise (video lays the course screen's static out
   from it over the whole window), made once. */
nshvc_image fx_noise_image(void)
{
    if (!noise_image.id) {
        static uint8_t t[256 * 256];
        uint16_t s = 0x1234;
        int i;

        for (i = 0; i < 256 * 256; i++) {
            s = noise_step(s);
            t[i] = (uint8_t)(0x80 + 0x10 * (s & 7));
        }
        noise_image = nshvc_image_create(256, 256, NSHVC_IMAGE_INDEXED);
        nshvc_image_write(noise_image, t);
    }
    return noise_image;
}

uint16_t fx_static_seed(int line)
{
    return static_seed[line & 127];
}

void fx_noise_row(uint16_t x, uint16_t y, uint16_t n, uint16_t seed)
{
    static_seed[y & 127] = seed;
    quad(fx_noise_image(), x, x + n, y, y + 1, seed & 0xFF, 1, (seed >> 8) + 0.5, 0);
}

/* The rows of an object sprite are sent whole by fx_sprite_shrink and
   fx_sprite_magnify. */
void fx_row_open(uint8_t kind, uint16_t x, uint16_t y, uint16_t tu, uint16_t du, uint16_t tv, uint16_t base, uint16_t dv) {}
void fx_row_close(uint16_t x) {}

/* A pixel; the weather's drops draw nothing here, fx_weather.c drawing
   them at the window's pixels. */
void fx_plot(uint8_t x, uint8_t y)
{
    if (fx_weather_drawing())
        return;
    fx_flat_row(x, y, 1);
}

/* Whether the picture is of eight bits a pixel (the whole palette): a
   solid's colours are the 8-bit palette's (two players' 4-bit pictures,
   their colours a palette row's, keep their sprites: seen wrong in a run,
   the tyres' greys showing as the row's pinks and yellows). */
int fx_draw_full_colour(void)
{
    return depth_mask() == 0xFF;
}

/* A solid's triangles (fx_solid.c), flat in palette colour c. */
void fx_draw_flat(const FxCorner *c, uint32_t n, uint8_t colour)
{
    fx_draw_flat_entry(c, n, ink(colour));
}

/* ---- the water (docs/stuntracefx/native/DEPTH.md, 6) ---- */

/* a hole's turn on the screen: 1 or -1 */
static int hole_inside_sign(const FxHole *h)
{
    double a = 0;
    int i;

    for (i = 0; i < h->n; i++)
        a += (double)h->x[i] * h->y[(i + 1) % h->n] - (double)h->x[(i + 1) % h->n] * h->y[i];
    return a > 0 ? 1 : -1;
}

/* The see-through water (FX_DEPTH_WATER: its shapes keep the depth) is
   drawn after the rest of the picture's 3D, at its own depths, tested and
   not written: what lies in front of it covers it, what lies past it shows
   through it, whatever the order the frame program drew them in. Where a
   track piece goes below it (fx_face_holes: the region the piece's
   crossings enclose on the water's plane, convex, on the screen) there is
   no water: each of its triangles is drawn less each hole, in convex
   pieces. A piece outside a hole is cut along one of the hole's edges; a
   point where an edge of the triangle crosses one is worked out from that
   edge's ends taken in one order whichever triangle or piece has it, so
   pieces that meet along a cut meet exactly. Its depth, 1 / Z, is linear
   across the screen, so a piece's corners keep the plane's depths. */

/* a point's side of hole edge e (> 0: inside, sign the hole's turn) */
static double hole_side(const FxHole *h, int sign, int e, float ox, float oy, const nshvc_vertex *p)
{
    double x0 = h->x[e] + ox, y0 = h->y[e] + oy, x1 = h->x[(e + 1) % h->n] + ox, y1 = h->y[(e + 1) % h->n] + oy;

    return sign * ((x1 - x0) * (p->y - y0) - (y1 - y0) * (p->x - x0));
}

/* the polygon in[n]'s part inside hole edge e (inside) or outside it,
   into out (room for 2 n corners: each edge gives at most two); its
   corners, 0 when fewer than 3 */
static uint32_t hole_clip(const nshvc_vertex *in, uint32_t n, nshvc_vertex *out, const FxHole *h, int sign, int e,
                          float ox, float oy, int inside)
{
    uint32_t i, m = 0;

    for (i = 0; i < n; i++) {
        const nshvc_vertex *p = &in[i], *q = &in[(i + 1) % n];
        double sp = hole_side(h, sign, e, ox, oy, p), sq = hole_side(h, sign, e, ox, oy, q);
        int pin = inside ? sp >= 0 : sp <= 0, qin = inside ? sq >= 0 : sq <= 0;

        if (pin)
            out[m++] = *p;
        if (pin != qin && sp != sq) {
            /* the crossing from the edge's ends in one order (by x, then y);
               the water's vertices have no divisor (w), so it is not one to
               carry */
            const nshvc_vertex *a = p, *b = q;
            double sa = sp, sb = sq, t;

            if (q->x < p->x || (q->x == p->x && q->y < p->y))
                a = q, b = p, sa = sq, sb = sp;
            t = sa / (sa - sb);
            out[m] = *a;
            out[m].x = (float)(a->x + ((double)b->x - a->x) * t);
            out[m].y = (float)(a->y + ((double)b->y - a->y) * t);
            out[m].z = (float)(a->z + ((double)b->z - a->z) * t);
            out[m].u = (float)(a->u + ((double)b->u - a->u) * t);
            out[m].v = (float)(a->v + ((double)b->v - a->v) * t);
            m++;
        }
    }
    return m >= 3 ? m : 0;
}

/* Pieces of water: their corners one after another, each piece's first
   and count; grown as needed. */
typedef struct {
    nshvc_vertex *v;
    uint32_t nv, cap_v;
    uint32_t *first, *count;
    uint32_t n, cap;
} PieceSet;

static PieceSet water_set[2];
static nshvc_vertex *rest[2];           /* the part of a piece still to cut, and the next */
static uint32_t rest_cap;

static void *grown(void *p, uint32_t *cap, uint32_t need, size_t size)
{
    if (need <= *cap)
        return p;
    if (!*cap)
        *cap = 64;
    while (*cap < need)
        *cap *= 2;
    p = realloc(p, *cap * size);
    if (!p)
        abort();
    return p;
}

/* where a piece of up to `corners` corners goes next in s */
static nshvc_vertex *piece_room(PieceSet *s, uint32_t corners)
{
    uint32_t cap = s->cap;

    s->v = grown(s->v, &s->cap_v, s->nv + corners, sizeof *s->v);
    s->first = grown(s->first, &cap, s->n + 1, sizeof *s->first);
    s->count = grown(s->count, &s->cap, s->n + 1, sizeof *s->count);
    return s->v + s->nv;
}

/* the piece just written there kept, if it has corners */
static void piece_keep(PieceSet *s, uint32_t n)
{
    if (n < 3)
        return;
    s->first[s->n] = s->nv;
    s->count[s->n] = n;
    s->n++;
    s->nv += n;
}

static void rest_room(uint32_t n)
{
    uint32_t cap = rest_cap;

    rest[0] = grown(rest[0], &cap, n, sizeof *rest[0]);
    rest[1] = grown(rest[1], &rest_cap, n, sizeof *rest[1]);
}

/* A water triangle less every hole: each piece of what is left cut along
   each edge of the next hole in turn, the part outside the edge kept as a
   piece and the part inside cut on (what is inside every edge is the
   hole's). Returns which of water_set holds the pieces. */
static int water_less_holes(const nshvc_vertex *tri, const FxHole *holes, int nh, float ox, float oy)
{
    int cur = 0, h;

    water_set[0].n = water_set[0].nv = 0;
    memcpy(piece_room(&water_set[0], 3), tri, 3 * sizeof *tri);
    piece_keep(&water_set[0], 3);
    for (h = 0; h < nh && water_set[cur].n; h++) {
        PieceSet *from = &water_set[cur], *to = &water_set[!cur];
        const FxHole *hole = &holes[h];
        int sign = hole_inside_sign(hole), e;
        uint32_t p;

        to->n = to->nv = 0;
        for (p = 0; p < from->n; p++) {
            uint32_t n = from->count[p];
            int r = 0;

            rest_room(n);
            memcpy(rest[0], from->v + from->first[p], n * sizeof *rest[0]);
            for (e = 0; e < hole->n && n >= 3; e++) {
                nshvc_vertex *out = piece_room(to, 2 * n);

                piece_keep(to, hole_clip(rest[r], n, out, hole, sign, e, ox, oy, 0));
                rest_room(2 * n);
                n = hole_clip(rest[r], n, rest[!r], hole, sign, e, ox, oy, 1);
                r = !r;
            }
        }
        cur = !cur;
    }
    return cur;
}

void fx_draw_water(void)
{
    if (!recording || water_placed)
        return;
    piece_close(recording);
    water_last(recording);
    water_placed = 1;
}

static void water_last(Recording *r)
{
    const FxHole *holes;
    uint32_t i, k, nwater = 0, nkeep = 0, nadd = 0, cap_add = 0;
    nshvc_vertex *add = NULL;
    Shape *water;
    int nh;

    for (i = 0; i < r->ns; i++)
        nwater += r->s[i].keep && !r->s[i].mesh;
    if (!nwater)
        return;
    holes = fx_face_holes(&nh);
    water = malloc(nwater * sizeof *water);
    if (!water)
        abort();
    /* the rest in its order, the water after it */
    for (i = 0, k = 0; i < r->ns; i++)
        if (r->s[i].keep && !r->s[i].mesh)
            water[k++] = r->s[i];
        else
            r->s[nkeep++] = r->s[i];
    for (k = 0; k < nwater; k++) {
        Shape sh = water[k];
        uint32_t t, first = r->nv + nadd;

        if (!nh || sh.lines) {
            r->s[nkeep++] = sh;
            continue;
        }
        for (t = 0; t + 2 < sh.count; t += 3) {
            const PieceSet *ps = &water_set[water_less_holes(&r->v[sh.first + t], holes, nh, (float)r->ox, (float)r->oy)];
            uint32_t need = 0, p, j;

            for (p = 0; p < ps->n; p++)
                need += 3 * (ps->count[p] - 2);
            add = grown(add, &cap_add, nadd + need, sizeof *add);
            for (p = 0; p < ps->n; p++) {
                const nshvc_vertex *c = ps->v + ps->first[p];

                for (j = 1; j + 1 < ps->count[p]; j++) {
                    add[nadd++] = c[0];
                    add[nadd++] = c[j];
                    add[nadd++] = c[j + 1];
                }
            }
        }
        sh.first = first;
        sh.count = r->nv + nadd - first;
        if (sh.count)
            r->s[nkeep++] = sh;
    }
    if (nadd) {
        if (r->nv + nadd > r->cap_v) {
            r->cap_v = r->cap_v ? r->cap_v : 4096;
            while (r->nv + nadd > r->cap_v)
                r->cap_v *= 2;
            r->v = realloc(r->v, r->cap_v * sizeof *r->v);
            if (!r->v)
                abort();
        }
        memcpy(r->v + r->nv, add, nadd * sizeof *add);
        r->nv += nadd;
    }
    r->ns = nkeep;
    r->gen++;
    free(water);
    free(add);
}

/* ---- seen through: what hides the player's car ----

   Jeff, 2026-11-13, on the overhead view: "make track/terrain entities that
   are 'above' the car - such as going under a bridge or in a tunnel - semi
   transparent while under them so you can still see the car", and "the
   fade should be like a sufficiently/playably sized 'radius' around the
   player car so the entire entity doesn't go transparent just where that
   radius is, with the radius smoothly fading back to opaque at the edges".

   While a view has a focus (the player's car: race/overhead.c) and the
   game's fade shader is in, what the frame program draws is in pieces
   (fx_draw_piece): each object one (fx_face.c, by its object; the cars
   none), the land's mountains one and its fill one (fx_ground.c), each
   instance of the land's trees, buildings and clouds one
   (fx_draw_mesh_seen). A piece hides the car when the line from the eye
   to one of SEE_POINTS points on the car (the middle of its body; the
   middle of its roof and four points SEE_SIDE round it, across the screen
   and along the ground) passes through one of its triangles SEE_MARGIN or
   more short of the point: on the screen, the point inside the triangle,
   the triangle's depth there (z, linear across it) nearer than the
   point's. A piece's fade rises from 0 to 1 over SEE_TIME while it hides
   the car and falls back while it does not, kept by what it is known by
   (its object; the land's by what it is and where it stands), so nothing
   comes or goes at once.

   Once the world is drawn (fx_draw_world_end), each triangle of a piece
   with a fade is cut at the level plane SEE_ABOVE over the car's place
   (along the world's up), in view space from its corners' depths (each
   corner's place on the screen at its depth; what a corner carries is
   linear along an edge in space): the part under it is drawn where it was,
   the part over it after the rest of the world, farthest first, tested
   against the depth and not written (the car and all that lies behind the
   piece are drawn by then and show through it), through fade.shade: its
   own colour shown in quarter steps, as the rest of the game's art is
   (Jeff, 2026-11-13: "it should be stepped since the rest of the game is in
   that art style as well"): within SEE_INNER of SEE_RADIUS (at the car's
   depth) of the car on the screen, SEE_LEAST mixed toward whole by its
   fade, to the nearest quarter, or nothing of it for the ground (the
   land's pieces: its mountains, fill and tunnels' rock, and the water;
   Jeff, 2026-11-13: "Unlike structures, you need to make ground terrain
   fully transparent in the radius (stepped of course). It's too confusing
   otherwise"); from there to SEE_RADIUS in rings of equal width, each
   quarter between that and whole; whole past it. A quarter is
   a whole number of a pixel's samples on every GPU (1 of 4, 2 of 8), so
   each step is exact. A road or floor under the car stays whole, and of
   what hides it only what lies near it on the screen is seen through. The
   ocean's water, drawn by a shader of its own, is a piece known as
   FX_PIECE_WATER that hides the car when its plane lies between the eye
   and the car (fx_draw_see_plane, Jeff, 2026-11-13: "you aren't making
   terrain like water transparent too when under them"): its shader steps
   it the same way, never showing more than the water itself does. */

enum { SEE_POINTS = 6, SEE_KEYS = 8192 };
#define SEE_RADIUS 2400.0       /* about eight of the car's lengths (Jeff, 2026-11-13, at 1,200: "the radius isn't large enough when under things") */
#define SEE_INNER 0.6f
#define SEE_LEAST 0.25f
#define SEE_ABOVE 48.0
#define SEE_TIME 0.25
#define SEE_MARGIN 8.0
#define SEE_BODY 70.0           /* the car's body's middle over its place */
#define SEE_ROOF 130.0          /* its roof's (the measured cars: 130 over the tyres' middles) */
#define SEE_SIDE 120.0          /* round the roof's middle: within the body (about 290 long) */

static struct {
    int ready, on;
    double up[3], level;                /* the world's up in view space; the plane: up . p = level */
    double cx, cy;                      /* the view's centre in the recording's pixels */
    double px[SEE_POINTS], py[SEE_POINTS], pz[SEE_POINTS];  /* the points on the screen; z a face must pass */
    double p[SEE_POINTS][3];            /* in view space */
} see;
static double see_now;                  /* the picture's time (nshvc_seconds) */
static struct {
    uint32_t key;
    float fade;
    double t;
} see_mem[SEE_KEYS];

/* a new picture's 3D: the car's points made when first wanted */
static void see_picture(void)
{
    see.ready = 0;
    see.on = 0;
    see_now = nshvc_seconds();
}

static int see_on(void)
{
    static int loaded;
    double f[3], fg[3], len, m[9];
    Recording *r = recording;
    int i, k;

    if (see.ready)
        return see.on;
    see.ready = 1;
    see.on = 0;
    if (!r || !drawing_3d || !fx_view_focus(f) || f[2] <= SEE_RADIUS)
        return 0;
    if (!loaded) {
        fade_shader = nshvc_shader_load("fade");
        loaded = 1;
    }
    if (!fade_shader.id)
        return 0;
    /* the view's turn (FX $00E4: row k world axis k in view space): world
       y is down, so the world's up is minus row 1 */
    for (i = 0; i < 9; i++)
        m[i] = (int16_t)(fx_ram[0x00E4 + 2 * i] | fx_ram[0x00E5 + 2 * i] << 8) / 32768.0;
    for (k = 0; k < 3; k++)
        see.up[k] = -m[3 + k];
    len = sqrt(see.up[0] * see.up[0] + see.up[1] * see.up[1] + see.up[2] * see.up[2]);
    if (len <= 0)
        return 0;
    for (k = 0; k < 3; k++)
        see.up[k] /= len;
    /* along the ground away from the eye: the view's axis less its up */
    for (k = 0; k < 3; k++)
        fg[k] = (k == 2) - see.up[2] * see.up[k];
    len = sqrt(fg[0] * fg[0] + fg[1] * fg[1] + fg[2] * fg[2]);
    for (k = 0; k < 3; k++)
        fg[k] = len > 0 ? fg[k] / len : 0;
    see.cx = (int16_t)(fx_ram[0x0034] | fx_ram[0x0035] << 8) + r->ox;
    see.cy = (int16_t)(fx_ram[0x0036] | fx_ram[0x0037] << 8) + r->oy;
    for (i = 0; i < SEE_POINTS; i++) {
        /* 0 the body's middle, 1 the roof's, 2-5 round it */
        double h = i ? SEE_ROOF : SEE_BODY, a = i == 2 ? SEE_SIDE : i == 3 ? -SEE_SIDE : 0;
        double b = i == 4 ? SEE_SIDE : i == 5 ? -SEE_SIDE : 0;

        for (k = 0; k < 3; k++)
            see.p[i][k] = f[k] + h * see.up[k] + a * (k == 0) + b * fg[k];
        if (see.p[i][2] <= SEE_MARGIN + 1.0)
            return 0;
        see.px[i] = see.cx + 128.0 * see.p[i][0] / see.p[i][2];
        see.py[i] = see.cy + 128.0 * see.p[i][1] / see.p[i][2];
        see.pz[i] = DEPTH_NEAR / (see.p[i][2] - SEE_MARGIN);
    }
    see.level = see.up[0] * f[0] + see.up[1] * f[1] + see.up[2] * f[2] + SEE_ABOVE;
    r->see_x = (float)(see.cx + 128.0 * f[0] / f[2]);
    r->see_y = (float)(see.cy + 128.0 * f[1] / f[2]);
    r->see_r = (float)(128.0 * SEE_RADIUS / f[2]);
    see.on = 1;
    return 1;
}

/* a piece of the ground (the land's keys, FX_PIECE_*): seen wholly through
   at the middle, where a structure shows SEE_LEAST */
static int piece_ground(uint32_t key) { return (key & 0xF0000000u) == 0x40000000u; }

/* a piece's fade now, from whether it hides the car */
static float see_fade(uint32_t key, int hides)
{
    uint32_t h = (key * 2654435761u) >> 19, i, k, at = SEE_KEYS;
    double oldest = 1e300, dt;

    for (i = 0; i < 16; i++) {
        k = (h + i) % SEE_KEYS;
        if (see_mem[k].key == key || !see_mem[k].key) {
            at = k;
            break;
        }
        if (see_mem[k].t < oldest) {
            oldest = see_mem[k].t;
            at = k;
        }
    }
    if (see_mem[at].key != key || see_now - see_mem[at].t > 1.0) {
        see_mem[at].key = key;          /* new, or not seen for a second: from whole */
        see_mem[at].fade = 0;
        see_mem[at].t = see_now;
    }
    dt = see_now - see_mem[at].t;
    dt = dt < 0 ? 0 : dt > 0.1 ? 0.1 : dt;
    see_mem[at].fade += (float)((hides ? dt : -dt) / SEE_TIME);
    see_mem[at].fade = see_mem[at].fade < 0 ? 0 : see_mem[at].fade > 1 ? 1 : see_mem[at].fade;
    see_mem[at].t = see_now;
    return see_mem[at].fade;
}

/* whether a triangle on the screen (x, y, z) hides one of the car's points */
static int tri_hides(const nshvc_vertex *a, const nshvc_vertex *b, const nshvc_vertex *c)
{
    double area = (b->x - a->x) * (c->y - a->y) - (b->y - a->y) * (c->x - a->x);
    int i;

    if (area == 0)
        return 0;
    for (i = 0; i < SEE_POINTS; i++) {
        double x = see.px[i], y = see.py[i];
        double wa = ((b->x - x) * (c->y - y) - (b->y - y) * (c->x - x)) / area;
        double wb = ((c->x - x) * (a->y - y) - (c->y - y) * (a->x - x)) / area;
        double wc = 1.0 - wa - wb;

        if (wa < 0 || wb < 0 || wc < 0)
            continue;
        if (wa * a->z + wb * b->z + wc * c->z > see.pz[i])
            return 1;
    }
    return 0;
}

/* whether the shapes [from, to) hide the car */
static int shapes_hide(const Recording *r, uint32_t from, uint32_t to)
{
    uint32_t i, t;

    for (i = from; i < to; i++) {
        const Shape *sh = &r->s[i];

        if (sh->mesh || sh->lines || sh->keep || sh->shader || !sh->world)
            continue;
        for (t = 0; t + 2 < sh->count; t += 3)
            if (tri_hides(&r->v[sh->first + t], &r->v[sh->first + t + 1], &r->v[sh->first + t + 2]))
                return 1;
    }
    return 0;
}

static void piece_close(Recording *r)
{
    Piece *p;

    if (!r->piece_open)
        return;
    p = &r->pc[r->piece_open - 1];
    p->fade = see_fade(p->key, shapes_hide(r, p->first, r->ns));
    r->piece_open = 0;
}

void fx_draw_piece(uint32_t key)
{
    Recording *r = recording;

    if (!r)
        return;
    piece_close(r);
    if (!key || !see_on())
        return;
    if (r->npc == r->cap_pc) {
        r->cap_pc = r->cap_pc ? r->cap_pc * 2 : 256;
        r->pc = realloc(r->pc, r->cap_pc * sizeof *r->pc);
        if (!r->pc)
            abort();
    }
    r->pc[r->npc].key = key;
    r->pc[r->npc].first = r->ns;
    r->pc[r->npc].fade = 0;
    r->npc++;
    r->piece_open = r->npc;
}

void fx_draw_mesh_seen(nshvc_mesh mesh, const nshvc_view *view, const nshvc_instance *in, uint32_t count, float lift,
                       float radius, uint32_t kind)
{
    static nshvc_instance *whole;
    static uint32_t cap;
    uint32_t i, n = 0;
    int j, k;

    if (!see_on()) {
        fx_draw_mesh(mesh, view, in, count);
        return;
    }
    whole = grown(whole, &cap, count, sizeof *whole);
    for (i = 0; i < count; i++) {
        /* its sphere in view space: its place `lift` of its size up the
           world (y is down), `radius` of its size round */
        double w[3] = { in[i].x, in[i].y - lift * in[i].size, in[i].z }, c[3], rr = radius * in[i].size;
        uint32_t key = 0x80000000u | (kind & 0x7F) << 24
                       | (((uint32_t)lround(in[i].x) * 73856093u ^ (uint32_t)lround(in[i].z) * 19349663u) & 0xFFFFFFu);
        int hides = 0;
        float fade;

        for (k = 0; k < 3; k++)
            c[k] = view->rot[3 * k] * (w[0] - view->eye[0]) + view->rot[3 * k + 1] * (w[1] - view->eye[1])
                   + view->rot[3 * k + 2] * (w[2] - view->eye[2]);
        for (j = 0; j < SEE_POINTS && !hides; j++) {
            /* the line t p from the eye (t 0) to point j (t 1) meets the
               sphere from t0 - h to t0 + h, t0 its nearest to the middle */
            const double *p = see.p[j];
            double len = sqrt(p[0] * p[0] + p[1] * p[1] + p[2] * p[2]);
            double t0 = (c[0] * p[0] + c[1] * p[1] + c[2] * p[2]) / (len * len), d2 = 0;

            for (k = 0; k < 3; k++)
                d2 += (c[k] - t0 * p[k]) * (c[k] - t0 * p[k]);
            if (d2 <= rr * rr) {
                double h = sqrt(rr * rr - d2) / len;

                hides = t0 + h > 0 && t0 - h < 1.0 - SEE_MARGIN / len;
            }
        }
        fade = see_fade(key, hides);
        if (fade <= 0) {
            whole[n++] = in[i];
            continue;
        }
        mesh_fade = fade;
        mesh_fade_z = c[2] > 1 ? (float)(DEPTH_NEAR / c[2]) : 1.0f;
        fx_draw_mesh(mesh, view, &in[i], 1);
        mesh_fade = 0;
    }
    if (n)
        fx_draw_mesh(mesh, view, whole, n);
}

int fx_draw_see_plane(const double n[3], double d, uint32_t key, float out[5])
{
    double f[3], len = sqrt(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
    float fade;
    int hides = 0, i;

    if (!recording || !see_on() || !fx_view_focus(f) || len <= 0)
        return 0;
    for (i = 0; i < SEE_POINTS && !hides; i++) {
        /* the eye (0, the plane's -d side) and the point on its two sides,
           the point SEE_MARGIN past it */
        double s = n[0] * see.p[i][0] + n[1] * see.p[i][1] + n[2] * see.p[i][2] - d;

        hides = d > 0 ? s > SEE_MARGIN * len : s < -SEE_MARGIN * len;
    }
    fade = see_fade(key, hides);
    out[0] = (float)(see.cx - recording->ox + 128.0 * f[0] / f[2]);
    out[1] = (float)(see.cy - recording->oy + 128.0 * f[1] / f[2]);
    out[2] = (float)(128.0 * SEE_RADIUS / f[2]);
    out[3] = SEE_INNER;
    out[4] = 1.0f - fade;               /* the ground's: wholly seen through at the middle */
    return 1;
}

/* a triangle cut at the plane over the car: its corners in view space
   from their places on the screen and depths */
static void corner_at(const nshvc_vertex *v, double p[3])
{
    double Z = DEPTH_NEAR / v->z;

    p[0] = (v->x - see.cx) * Z / 128.0;
    p[1] = (v->y - see.cy) * Z / 128.0;
    p[2] = Z;
}

static nshvc_vertex corner_between(const nshvc_vertex *a, const nshvc_vertex *b, const double pa[3], const double pb[3], double s)
{
    nshvc_vertex v = *a;
    double p[3];
    int k;

    for (k = 0; k < 3; k++)
        p[k] = pa[k] + (pb[k] - pa[k]) * s;
    v.x = (float)(see.cx + 128.0 * p[0] / p[2]);
    v.y = (float)(see.cy + 128.0 * p[1] / p[2]);
    v.z = (float)(DEPTH_NEAR / p[2]);
    v.w = (float)(a->w + (b->w - a->w) * s);
    v.u = (float)(a->u + (b->u - a->u) * s);
    v.v = (float)(a->v + (b->v - a->v) * s);
    if (a->colour != b->colour) {
        uint32_t c = 0;

        for (k = 0; k < 32; k += 8) {
            double ca = a->colour >> k & 255, cb = b->colour >> k & 255;

            c |= (uint32_t)lround(ca + (cb - ca) * s) << k;
        }
        v.colour = c;
    }
    return v;
}

/* triangle t's parts under (side 0) and over (side 1) the plane, fanned
   into out[side] (at most 2 triangles each) */
static void cut_at_level(const nshvc_vertex *t, nshvc_vertex out[2][6], int n[2])
{
    double p[3][3], h[3];
    nshvc_vertex poly[2][4];
    int m[2] = { 0, 0 }, i, k, side;

    for (i = 0; i < 3; i++) {
        corner_at(&t[i], p[i]);
        h[i] = see.up[0] * p[i][0] + see.up[1] * p[i][1] + see.up[2] * p[i][2] - see.level;
    }
    for (i = 0; i < 3; i++) {
        int j = (i + 1) % 3, si = h[i] > 0, sj = h[j] > 0;

        poly[si][m[si]++] = t[i];
        if (si != sj) {
            nshvc_vertex v = corner_between(&t[i], &t[j], p[i], p[j], h[i] / (h[i] - h[j]));

            poly[0][m[0]++] = v;
            poly[1][m[1]++] = v;
        }
    }
    for (side = 0; side < 2; side++) {
        n[side] = 0;
        for (k = 1; k + 1 < m[side]; k++) {
            out[side][n[side]++] = poly[side][0];
            out[side][n[side]++] = poly[side][k];
            out[side][n[side]++] = poly[side][k + 1];
        }
    }
}

static int fade_order(const void *a, const void *b)
{
    float x = ((const Shape *)a)->fade_z, y = ((const Shape *)b)->fade_z;

    return x < y ? -1 : x > y;
}

/* The pieces with a fade cut at the plane over the car, the parts over it
   last, farthest first; and the instances seen through after them all */
static void see_split(Recording *r)
{
    static Shape *out, *faded;
    static nshvc_vertex *add;
    static uint32_t cap_out, cap_faded, cap_add;
    uint32_t i, t, nout = 0, nfaded = 0, nadd = 0, any = 0;

    for (i = 0; i < r->npc; i++)
        any |= r->pc[i].fade > 0;
    for (i = 0; i < r->ns; i++)
        any |= r->s[i].fade != 0;
    if (!any || !see.on)
        return;
    out = grown(out, &cap_out, r->ns * 2 + 16, sizeof *out);
    faded = grown(faded, &cap_faded, r->ns * 2 + 16, sizeof *faded);
    for (i = 0; i < r->ns; i++) {
        const Shape *sh = &r->s[i];
        float fade = sh->piece && sh->piece <= r->npc ? r->pc[sh->piece - 1].fade : 0;
        Shape under, over;
        double zsum = 0;
        uint32_t zn = 0;

        if (sh->fade) {                 /* an instance seen through */
            faded[nfaded++] = *sh;
            continue;
        }
        if (fade <= 0 || sh->mesh || sh->lines || sh->keep || sh->shader || !sh->world || sh->count < 3) {
            out[nout++] = *sh;
            continue;
        }
        under = over = *sh;
        add = grown(add, &cap_add, nadd + sh->count * 4, sizeof *add);
        under.first = nadd;
        under.count = 0;
        for (t = 0; t + 2 < sh->count; t += 3) {
            nshvc_vertex part[2][6];
            int n[2], k;

            cut_at_level(&r->v[sh->first + t], part, n);
            for (k = 0; k < n[0]; k++)
                add[nadd++] = part[0][k];
            under.count += (uint32_t)n[0];
        }
        over.first = nadd;
        over.count = 0;
        for (t = 0; t + 2 < sh->count; t += 3) {
            nshvc_vertex part[2][6];
            int n[2], k;

            cut_at_level(&r->v[sh->first + t], part, n);
            for (k = 0; k < n[1]; k++) {
                add[nadd++] = part[1][k];
                zsum += part[1][k].z;
                zn++;
            }
            over.count += (uint32_t)n[1];
        }
        if (under.count) {
            under.first |= 0x80000000u;  /* in `add` until it is in the recording's */
            out[nout++] = under;
        }
        if (over.count) {
            over.first |= 0x80000000u;
            over.keep = 1;
            over.fade = (uint8_t)(fade * 254.0f + 1.0f);
            over.fade_z = (float)(zsum / zn);
            over.ground = (uint8_t)piece_ground(r->pc[sh->piece - 1].key);
            faded[nfaded++] = over;
        }
    }
    if (!nfaded)
        return;
    qsort(faded, nfaded, sizeof *faded, fade_order);
    /* the vertices made, after the recording's own */
    r->v = grown(r->v, &r->cap_v, r->nv + nadd, sizeof *r->v);
    memcpy(r->v + r->nv, add, nadd * sizeof *add);
    r->s = grown(r->s, &r->cap_s, nout + nfaded, sizeof *r->s);
    for (i = 0; i < nout + nfaded; i++) {
        Shape sh = i < nout ? out[i] : faded[i - nout];

        if (sh.first & 0x80000000u)
            sh.first = (sh.first & 0x7FFFFFFFu) + r->nv;
        r->s[i] = sh;
    }
    r->nv += nadd;
    r->ns = nout + nfaded;
    r->gen++;
}

/* a shape seen through: fade.shade's inputs, the car's place and the
   radius in the list's pixels (as compose_recording scales the world) */
static void see_material(const Recording *r, const Shape *sh, nshvc_material *m)
{
    float zoom = sh->world && r->zoom > 0 ? r->zoom : 1.0f;
    float x = r->zoom_x + (r->see_x - r->zoom_x) * zoom, y = r->zoom_y + (r->see_y - r->zoom_y) * zoom;
    float fade = (sh->fade - 1) / 254.0f;

    memset(m, 0, sizeof *m);
    m->image.id = sh->image;
    m->shader = fade_shader;
    m->inputs[0] = x * screen_sx;
    m->inputs[1] = y * screen_sy;
    m->inputs[2] = r->see_r * zoom * screen_sx;
    m->inputs[3] = r->see_r * zoom * screen_sy;
    m->inputs[4] = SEE_INNER;
    m->inputs[5] = 1.0f - fade * (1.0f - (sh->ground ? 0.0f : SEE_LEAST));
}

/* The ground's surface (fx_ground.c): triangles drawn by `shader` over
   `image` with its inputs, each vertex's u, v its place on the screen less
   (ox, oy). */
void fx_draw_surface(const FxCorner *c, uint32_t n, nshvc_image image, nshvc_shader shader, const float in[8], float ox, float oy)
{
    nshvc_vertex v[48];
    uint32_t done = 0, i;

    if ((!target.id && !recording) || !shader.id)
        return;
    blocks_flush();
    memset(v, 0, sizeof v);
    while (done < n) {
        uint32_t k = n - done > 48 ? 48 : n - done;

        for (i = 0; i < k; i++) {
            v[i].x = c[done + i].x;
            v[i].y = c[done + i].y;
            v[i].u = c[done + i].x - ox;
            v[i].v = c[done + i].y - oy;
            v[i].z = fx_draw_z(c[done + i].iz);
            v[i].colour = 0xFF000000u;
        }
        emit_with(v, k, image.id, 0, shader.id, in);
        done += k;
    }
}

/* The land's polygons (fx_ground.c): in their vertices' colours, at their
   own depths. */
void fx_draw_land(const nshvc_vertex *v, uint32_t n)
{
    if ((!target.id && !recording) || !n)
        return;
    blocks_flush();
    emit(v, n, 0, 0);
}

/* The same in the palette's entry e as it is, of the whole palette. */
void fx_draw_flat_entry(const FxCorner *c, uint32_t n, uint8_t colour)
{
    nshvc_vertex v[48];
    uint32_t done = 0, i;

    if ((!target.id && !recording) || !colour)
        return;
    blocks_flush();
    memset(v, 0, sizeof v);
    while (done < n) {
        uint32_t k = n - done > 48 ? 48 : n - done;

        for (i = 0; i < k; i++) {
            v[i].x = c[done + i].x;
            v[i].y = c[done + i].y;
            v[i].z = fx_draw_z(c[done + i].iz);
            at_entry(&v[i], colour);
        }
        emit(v, k, entry_image(), 0);
        done += k;
    }
}

uint8_t fx_rpix(uint8_t x, uint8_t y) { (void)x; (void)y; return 0; }

/* ---- the faces ---- */

static uint16_t rd(uint16_t a) { return (uint16_t)(fx_ram[a] | fx_ram[(uint16_t)(a + 1)] << 8); }

/* ---- two players' colours ----

   The original's two players draw 4 bits a pixel: each face one of the 16
   colours of a palette row, from a table ($A545) the same at every
   distance, where one player's ($A325) gives each colour eight shades and
   fades them toward the haze in four distance tiers. The Enhanced pictures
   of two players take the 16 colours as they are and give them one
   player's depth: the shades each colour stands for, rebuilt from one
   player's steps between them, lit faces blended between their light
   levels, and one player's haze by the model's tier. The colour is
   computed and given to the GPU as it is; the palette is not touched. */

#define TABLE_ONE 0xA325        /* one player's distance table, nearest tier */
#define TABLE_TWO 0xA545        /* two players' */
#define TABLE_LEN 0x88
#define STEPS_PALETTE 0x8800    /* bank $1F: one player's palette, where the steps are measured */

/* One player's haze: the colour its far tiers fade toward and how far
   (tier 1, 2, 3), fitted from its distance tables over its palette. */
static const float haze_rgb[3] = { 160, 188, 193 };
static const float haze_part[4] = { 0, 0.447f, 0.687f, 0.687f };

/* a palette colour as the console widens it: 5 bits to 8, the top bits
   repeated below */
static void bgr555(uint16_t c, float rgb[3])
{
    int i;

    for (i = 0; i < 3; i++) {
        unsigned x = c >> (5 * i) & 31;

        rgb[i] = (float)(x << 3 | x >> 2);
    }
}

static float luma(const float rgb[3]) { return 0.299f * rgb[0] + 0.587f * rgb[1] + 0.114f * rgb[2]; }

/* For each place in the distance table, its shade's brightness over the
   mean of the shades that take the same colour in two players: the places
   whose one-player colours are of one palette row and whose two-player
   colour is the same. Only where those one-player colours are a run of
   consecutive palette entries, as a colour's shades are laid out; one
   player's row 1 holds the 16 fixed colours two players have, in no order
   of brightness (places $70-$7F), and those stay as they are. Measured on
   one player's palette. */
static float shade_step[TABLE_LEN];
static int shade_steps_made;

static void make_shade_steps(void)
{
    float lum[256];
    int m, k;

    for (k = 0; k < 256; k++) {
        uint16_t at = (uint16_t)(STEPS_PALETTE + 2 * k);
        float rgb[3];

        bgr555((uint16_t)(fx_rom_read(0x1F, at) | fx_rom_read(0x1F, (uint16_t)(at + 1)) << 8), rgb);
        lum[k] = luma(rgb);
    }
    for (m = 0; m < TABLE_LEN; m++) {
        uint8_t one = fx_rom_read(3, (uint16_t)(TABLE_ONE + m)), two = fx_rom_read(3, (uint16_t)(TABLE_TWO + m)) & 0x0F;
        uint8_t seen[256] = { 0 };
        float sum = 0;
        int count = 0, low = 255, high = 0;

        for (k = 0; k < TABLE_LEN; k++) {
            uint8_t o = fx_rom_read(3, (uint16_t)(TABLE_ONE + k));

            if ((fx_rom_read(3, (uint16_t)(TABLE_TWO + k)) & 0x0F) == two && (o >> 4) == (one >> 4) && !seen[o]) {
                seen[o] = 1;
                sum += lum[o];
                count++;
                if (o < low)
                    low = o;
                if (o > high)
                    high = o;
            }
        }
        shade_step[m] = count > 1 && high - low + 1 == count && sum > 0 ? lum[one] / (sum / count) : 1.0f;
    }
    shade_steps_made = 1;
}

/* A colour made brighter or darker by `step`, its hue kept: past full
   brightness in a channel it goes toward white instead. */
static void shade(float rgb[3], float step)
{
    float top = rgb[0], want = luma(rgb) * step, l;
    int i;

    for (i = 1; i < 3; i++)
        if (rgb[i] > top)
            top = rgb[i];
    if (top * step <= 255 || top <= 0) {
        for (i = 0; i < 3; i++)
            rgb[i] *= step;
        return;
    }
    for (i = 0; i < 3; i++)
        rgb[i] *= 255 / top;
    l = luma(rgb);
    if (l < 255 && want > l) {
        float t = (want - l) / (255 - l);

        if (t > 1)
            t = 1;
        for (i = 0; i < 3; i++)
            rgb[i] += (255 - rgb[i]) * t;
    }
}

static void row_colour(uint8_t c, float rgb[3])
{
    bgr555(pic.palette[(palette_row * 16 + (c & 0x0F)) & 0xFF], rgb);
}

int fx_draw_two_players(void)
{
    return depth_mask() == 0x0F && race_two_full_window();
}

/* The colour of a flat face of two players' Enhanced pictures, the chip's
   colour c: 0xFFRRGGBB, or 0 to draw it as the chip has it. */
static uint32_t two_player_colour(const FxShape *sh, uint8_t c)
{
    float rgb[3];
    int i;

    if (sh->tint < 0 || !(c & 0x0F) || !fx_draw_two_players())
        return 0;
    if (sh->tint == 1) {
        /* the light level unrounded, between the levels' colours: the
           chip's level is the floor, so each level's colour is at its
           middle */
        float x = sh->light - 0.5f, f, b[3];
        int at;

        if (x < 6)
            x = 6;
        if (x > 15)
            x = 15;
        at = (int)x;
        f = x - (float)at;
        row_colour(fx_rom_read(3, (uint16_t)(sh->tint_index + at - 6)), rgb);
        if (at < 15 && f > 0) {
            uint8_t next = fx_rom_read(3, (uint16_t)(sh->tint_index + at - 5));

            if (next & 0x0F) {
                row_colour(next, b);
                for (i = 0; i < 3; i++)
                    rgb[i] += (b[i] - rgb[i]) * f;
            }
        }
    } else {
        row_colour(c, rgb);
        if (sh->tint == 2 && sh->tint_index < TABLE_LEN) {
            if (!shade_steps_made)
                make_shade_steps();
            shade(rgb, shade_step[sh->tint_index]);
        }
    }
    if (sh->tint != 0)
        for (i = 0; i < 3; i++)
            rgb[i] += (haze_rgb[i] - rgb[i]) * haze_part[sh->tier & 3];
    for (i = 0; i < 3; i++) {
        if (rgb[i] < 0)
            rgb[i] = 0;
        if (rgb[i] > 255)
            rgb[i] = 255;
    }
    return 0xFF000000u | (uint32_t)(rgb[0] + 0.5f) << 16 | (uint32_t)(rgb[1] + 0.5f) << 8 | (uint32_t)(rgb[2] + 0.5f);
}

/* A face's shape in floats (fx_face.c) in place of the chip's entries for
   it, in the material of its first entry (p, after the kind byte). */
static void shape_draw_body(const FxShape *sh)
{
    const FxCorner *c = fx_face_corners() + sh->first;
    const FxMaterial *m = &sh->material;
    nshvc_vertex v[48];
    uint32_t image = 0, done = 0, i, tinted = 0;
    uint8_t colour;
    double u_off = 0, v_off = 0, vcx = (int16_t)rd(0x0034), vcy = (int16_t)rd(0x0036);
    float row_at = (float)(recording ? recording->oy : offset_y);
    int verge = 0;

    if (sh->opening) {
        /* a tunnel's exit from inside: the sky outside by row, at the
           farthest depth, so what lies outside is drawn over it and
           nothing past the tunnel's walls shows it */
        if (!target.id && !recording)
            return;
        memset(v, 0, sizeof v);
        while (done < sh->count) {
            uint32_t n = sh->count - done > 48 ? 48 : sh->count - done;

            for (i = 0; i < n; i++) {
                uint16_t bgr = race_open_sky_colour((int)floorf(c[done + i].y + row_at));
                uint32_t r = bgr & 31, gc = bgr >> 5 & 31, b = bgr >> 10 & 31;

                v[i].x = c[done + i].x;
                v[i].y = c[done + i].y;
                v[i].z = 1e-6f;
                v[i].colour = 0xFF000000u | (r << 3 | r >> 2) << 16 | (gc << 3 | gc >> 2) << 8 | (b << 3 | b >> 2);
            }
            emit(v, n, 0, sh->line);
            done += n;
        }
        return;
    }
    if (m->textured) {
        uint8_t bank = m->bank;
        uint16_t word = m->word, base = m->base, mask = m->mask;

        colour = (uint8_t)word;
        fxs.por = (uint8_t)((word >> 8) & 0x1F);
        fxs.colr = colour;
        if (!target.id && !recording)
            return;
        u_off = (int16_t)rd(0x00C0) / 256.0;
        v_off = (int16_t)rd(0x00C2) / 256.0;
        image = texture(bank, base, mask, colour, fxs.por).id;
        colour = 1;
    } else {
        colour = m->colour;
        fxs.por = 0;
        fxs.colr = colour;
        tinted = two_player_colour(sh, colour);
        colour = ink(colour);
        if ((!target.id && !recording) || !colour)
            return;
        /* a course's ground face (Night Cruise's verges), shaded by its
           corners' depths toward the band (fx_ground_verge) */
        verge = !tinted && !sh->line && sh->plane[3] != 0 && fx_draw_full_colour() && fx_ground_verge(colour);
        if (!tinted && !verge)
            image = entry_image();      /* the palette's entry; a tint is a colour */
    }
    memset(v, 0, sizeof v);
    while (done < sh->count) {
        uint32_t n = sh->count - done > 48 ? 48 : sh->count - done;

        for (i = 0; i < n; i++) {
            v[i].x = c[done + i].x;
            v[i].y = c[done + i].y;
            v[i].u = (float)(c[done + i].u + u_off);
            v[i].v = (float)(c[done + i].v + v_off);
            v[i].z = fx_draw_z(c[done + i].iz);
            v[i].colour = tinted;
            if (verge) {
                double inv = (sh->plane[0] * (v[i].x - vcx) / 128.0 + sh->plane[1] * (v[i].y - vcy) / 128.0 + sh->plane[2])
                             / sh->plane[3];

                v[i].colour = fx_ground_verge_colour(colour, inv > 1e-9 ? 1.0 / inv : 1e9, v[i].y + row_at, picture_now);
            } else if (!m->textured && !tinted) {
                at_entry(&v[i], colour);
            }
        }
        emit(v, n, image, sh->line);
        done += n;
    }
}

/* ---- the layers of faces on one plane (docs/stuntracefx/native/DEPTH.md, 3) ----

   The game drew a face over an earlier one in the same plane (a marking on
   a road, a lamp on a wall) by drawing it later. With the depth test, such
   a face is drawn one layer in front of the highest earlier face of the
   picture that it lies on and overlaps on the screen; faces that look the
   same share a layer, so a road cut into pieces does not climb. It lies on
   an earlier face when they face the same way and each of its corners is
   within LAYER_TOL (and LAYER_REL of its distance) of that face's plane:
   the corners are exact (fx_face.c), so faces the game made on one plane
   meet that to the double's rounding, and two real surfaces are always
   farther apart. Overlap is their triangles' on the screen, with a
   thousandth of a pixel's edge, so faces that only touch (a chequered
   line's squares) do not climb either. */

#define LAYER_TOL 0.01
#define LAYER_REL 1e-7
enum { LAYER_SHAPES = 8192, LAYER_TRIS = 65536, LAYER_BUCKETS = 4096 };

static struct {
    double n[3], d;             /* its plane: n . p = d, n of unit length */
    float box[4];               /* its outline's box on the screen: left, right, top, bottom */
    uint32_t tri, ntri;         /* its triangles (a line: one, its two ends and the first again) */
    uint32_t look;
    uint8_t layer;
    int32_t next;               /* the next in its bucket */
} lay[LAYER_SHAPES];
static float lay_tri[LAYER_TRIS][6];
static int32_t lay_bucket[LAYER_BUCKETS];
static uint32_t nlay, nlay_tri;

static void layers_reset(void)
{
    nlay = nlay_tri = 0;
    memset(lay_bucket, 0xFF, sizeof lay_bucket);
}

static int bucket_of(double d) { return (int)(((int64_t)floor(d) % LAYER_BUCKETS + LAYER_BUCKETS) % LAYER_BUCKETS); }

/* whether two convex outlines of n and m points apart on the screen: a
   separating axis among their edges' normals, with an edge of EPS */
static int outlines_apart(const float *a, int n, const float *b, int m)
{
    const float eps = 1e-3f;
    int pass, i, k;

    for (pass = 0; pass < 2; pass++) {
        const float *p = pass ? b : a;
        int np = pass ? m : n;

        for (i = 0; i < np; i++) {
            float ex = p[2 * ((i + 1) % np)] - p[2 * i], ey = p[2 * ((i + 1) % np) + 1] - p[2 * i + 1];
            float nx = -ey, ny = ex, amin = 1e30f, amax = -1e30f, bmin = 1e30f, bmax = -1e30f, len = sqrtf(nx * nx + ny * ny);

            if (len < 1e-9f)
                continue;
            nx /= len, ny /= len;
            for (k = 0; k < n; k++) {
                float t = a[2 * k] * nx + a[2 * k + 1] * ny;

                amin = t < amin ? t : amin, amax = t > amax ? t : amax;
            }
            for (k = 0; k < m; k++) {
                float t = b[2 * k] * nx + b[2 * k + 1] * ny;

                bmin = t < bmin ? t : bmin, bmax = t > bmax ? t : bmax;
            }
            if (amax <= bmin + eps || bmax <= amin + eps)
                return 1;
        }
    }
    return 0;
}

static uint32_t look_of(const FxShape *sh)
{
    const FxMaterial *m = &sh->material;

    if (m->textured)
        return 0x80000000u ^ (uint32_t)m->bank << 24 ^ (uint32_t)m->base << 8 ^ m->mask ^ (uint32_t)m->word * 2654435761u;
    return (uint32_t)m->colour | (uint32_t)(uint8_t)sh->tint << 8 | (uint32_t)sh->tier << 16
           | (uint32_t)((int)(sh->light * 4.0f) & 0x7F) << 20;
}

/* face sh's layer (its triangles at c, count of them), and it noted for
   the faces after it */
static uint8_t layer_of(const FxShape *sh, const FxCorner *c, uint32_t count)
{
    double n[3] = { 0, 0, 0 }, len, d = 0, tol;
    uint32_t i, k, t, ntri;
    int b, db;
    uint8_t level = 0;
    float box[4] = { 1e30f, -1e30f, 1e30f, -1e30f };

    if (sh->ncorner3 < 2 || !count || nlay == LAYER_SHAPES)
        return 0;
    for (i = 0; i < sh->ncorner3; i++) {        /* Newell's normal */
        const double *p = sh->corner3[i], *q = sh->corner3[(i + 1) % sh->ncorner3];

        n[0] += (p[1] - q[1]) * (p[2] + q[2]);
        n[1] += (p[2] - q[2]) * (p[0] + q[0]);
        n[2] += (p[0] - q[0]) * (p[1] + q[1]);
    }
    len = sqrt(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
    if (len <= 0)
        return 0;                       /* a line: no plane of its own */
    for (k = 0; k < 3; k++)
        n[k] /= len;
    for (i = 0; i < sh->ncorner3; i++)
        d += (n[0] * sh->corner3[i][0] + n[1] * sh->corner3[i][1] + n[2] * sh->corner3[i][2]) / sh->ncorner3;
    tol = LAYER_TOL + LAYER_REL * fabs(d);
    ntri = sh->line ? 1 : count / 3;
    if (nlay_tri + ntri > LAYER_TRIS)
        return 0;
    for (t = 0; t < ntri; t++)
        for (k = 0; k < 3; k++) {
            const FxCorner *p = &c[sh->line ? (k < 2 ? k : 0) : 3 * t + k];

            lay_tri[nlay_tri + t][2 * k] = p->x;
            lay_tri[nlay_tri + t][2 * k + 1] = p->y;
            box[0] = p->x < box[0] ? p->x : box[0], box[1] = p->x > box[1] ? p->x : box[1];
            box[2] = p->y < box[2] ? p->y : box[2], box[3] = p->y > box[3] ? p->y : box[3];
        }
    for (db = -1; db <= 1; db++)
        for (b = lay_bucket[bucket_of(d + db)]; b >= 0; b = lay[b].next) {
            uint32_t ta, tb;
            int on = 1, over = 0;

            if (n[0] * lay[b].n[0] + n[1] * lay[b].n[1] + n[2] * lay[b].n[2] < 0.99 || fabs(lay[b].d - d) > tol + 8.0)
                continue;
            if (lay[b].box[0] >= box[1] || box[0] >= lay[b].box[1] || lay[b].box[2] >= box[3] || box[2] >= lay[b].box[3])
                continue;
            for (i = 0; i < sh->ncorner3 && on; i++)
                on = fabs(lay[b].n[0] * sh->corner3[i][0] + lay[b].n[1] * sh->corner3[i][1] + lay[b].n[2] * sh->corner3[i][2]
                          - lay[b].d) <= tol;
            if (!on)
                continue;
            for (ta = 0; ta < ntri && !over; ta++)
                for (tb = 0; tb < lay[b].ntri && !over; tb++)
                    over = !outlines_apart(lay_tri[nlay_tri + ta], 3, lay_tri[lay[b].tri + tb], 3);
            if (!over)
                continue;
            k = lay[b].layer + (lay[b].look != look_of(sh));
            if (k > level)
                level = (uint8_t)(k > LAYER_MOST ? LAYER_MOST : k);
        }
    for (k = 0; k < 3; k++)
        lay[nlay].n[k] = n[k];
    lay[nlay].d = d;
    lay[nlay].tri = nlay_tri;
    lay[nlay].ntri = ntri;
    memcpy(lay[nlay].box, box, sizeof box);
    lay[nlay].look = look_of(sh);
    lay[nlay].layer = level;
    b = bucket_of(d);
    lay[nlay].next = lay_bucket[b];
    lay_bucket[b] = (int32_t)nlay;
    nlay++;
    nlay_tri += ntri;
    return level;
}

/* a queued face, its layer and side */
static void shape_draw(const FxShape *sh)
{
    in_shape = 1;
    layer_now = layer_of(sh, fx_face_corners() + sh->first, sh->count);
    back_now = sh->back;
    shape_draw_body(sh);
    layer_now = back_now = 0;
    in_shape = 0;
}

/* The faces queued so far drawn now, as FxDrawList draws them, the chip's
   state as it was (fx_ground.c: a tunnel piece's faces before its rock). */
void fx_draw_queued(void)
{
    FxState keep = fxs;
    uint32_t i, n = fx_face_count();

    if (!n)
        return;
    blocks_flush();
    for (i = 0; i < n; i++)
        shape_draw(fx_face_at(i));
    fx_face_reset();
    fxs = keep;
}

FxDrawListResult FxDrawList(uint16_t caller_list_end, uint16_t caller_write_ptr, uint16_t *stack, uint16_t rom_at)
{
    uint32_t i, n = fx_face_count();

    (void)stack;
    if (n) {
        blocks_flush();
        for (i = 0; i < n; i++)
            shape_draw(fx_face_at(i));
        fx_face_reset();
        fx_romb(fx_ldw(0x001C));        /* the model's bank, as the chip's walk left it */
    }
    return (FxDrawListResult){caller_list_end, caller_write_ptr, rom_at};
}
