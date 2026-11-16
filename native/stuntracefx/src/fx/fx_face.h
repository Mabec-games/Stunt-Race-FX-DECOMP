/*
 * fx_face.h: the race's shapes in floats (fx/fx_face.c), between the frame
 * program, which says what is drawn, and the draw lists (fx/fx_draw.c).
 */
#ifndef STUNTRACEFX_FX_FACE_H
#define STUNTRACEFX_FX_FACE_H

#include <stdint.h>

/* A corner of a shape: its place on the picture in pixels, unrounded, and
   its texel. */
typedef struct {
    float x, y, u, v;
    float iz;                   /* one over its view-space depth, whole (0: none, a 2D corner) */
} FxCorner;

/* A face's material as FxModelMaterial chose it: a flat face's colour
   byte; a textured face's texture (its bank, base and mask) and its colour
   and options word (the colour in the low byte, the plot options in bits
   8-12). */
typedef struct {
    uint8_t textured;
    uint8_t colour;
    uint8_t bank;
    uint16_t word, base, mask;
} FxMaterial;

/* A face's shape: triangles (or a line's two ends), in the order the faces
   were made, with its material. */
typedef struct {
    FxMaterial material;
    uint8_t line;
    uint32_t first, count;
    /* how the chip chose a flat face's colour (fx_face_material): -1 not
       seen (a textured face), 0 its own index, 1 lit, 2 from the distance
       table; that index (for a lit face its shade row's address, in the
       nearest tier's table), the model's distance tier (0-3), and a lit
       face's light level unrounded */
    int8_t tint;
    uint8_t tier;
    uint16_t tint_index;
    float light;
    /* its plane in view space, n . p = d, a far object's halved place
       taken back to its own, so that one over a corner's depth is n .
       ((x - cx) / 128, (y - cy) / 128, 1) / d (d 0: none, a line) */
    float plane[4];
    /* a face seen from behind (an open model's: fx_face_model_done), drawn
       behind a face facing the eye at the same depth */
    uint8_t back;
    /* a tunnel's exit seen from inside (fx_face.c's mouths): drawn as its
       opening, the sky outside at the farthest depth */
    uint8_t opening;
    /* drawn over the rest of the 3D in its own order (the GOAL letters) */
    uint8_t over;
    /* its corners in view space, whole and exact (at most 8; 0: not kept,
       a solid's or the land's), for the layers of faces on one plane
       (fx_draw.c) */
    uint8_t ncorner3;
    double corner3[8][3];
} FxShape;

/* A face: begun, its points given (fx_face_vertex, fx_rt.h), then ended
   with its material, which makes its shape (none when a point is not
   known, or it is wholly off the view). */
void fx_face_begin(int textured);
void fx_face_end(const FxMaterial *material);
/* Faces seen from behind (docs/stuntracefx/native/DEPTH.md, 4):
   FxModelFaces begins a face facing away with fx_face_begin_back; it is held until its model's walk is
   done (fx_face_model_done, before the model's queue is drawn), then
   drawn, shaded as the chip shades it turned to the eye, unless the faces
   the walk reached close (every edge two faces' going opposite ways) or
   the face has a twin (the same corners the other way round): what faces
   the eye in its place hides it. */
void fx_face_begin_back(int textured);
void fx_face_model_done(void);
/* The shapes made since they were last drawn, in order; whether there is
   room for another face's (else they are drawn first, as the chip drew its
   queue when it was nearly full). */
uint32_t fx_face_count(void);
const FxShape *fx_face_at(uint32_t i);
int fx_face_full(void);
const FxCorner *fx_face_corners(void);
/* The shapes were drawn: they are gone. */
void fx_face_reset(void);

/* What the checks count: vertices of faces wholly on the screen, and those
   more than 1.02 pixels from the chip's own. */
typedef struct {
    uint32_t faces, vertices, apart, worst_face;
    double worst;
    /* the chip's near points (depth below $100, FxProjectPoint), whose
       pair FxRatioClamp halves before dividing; and every vertex past the
       chip's own error (fx_face.c check_point) */
    uint32_t near, near_apart, past_bound;
} FxFaceCheck;
void fx_face_check_on(int on);
FxFaceCheck fx_face_check(void);

/* A solid's face (fx_solid.c) of n view-space points, near-clipped,
   projected and kept to the view: its triangles' corners in `out`; the
   count made, 0 when none (n at most 64). */
uint32_t fx_face_triangles(const float (*p)[3], int n, FxCorner *out, uint32_t room);
/* The same from points in doubles, in the scale of the object drawn now. */
uint32_t fx_face_triangles_d(const double (*p)[3], int n, FxCorner *out, uint32_t room);
/* The same for points in whole units whatever object is drawn (the land's,
   fx_ground.c): each corner's iz their own 1 / Z. */
uint32_t fx_face_triangles_whole(const double (*p)[3], int n, FxCorner *out, uint32_t room);

/* The water's holes (fx_ground.c: Aqua Tunnel's, Harbor City's): on for a
   picture, the water's plane n . v = d in view space (d > 0: the eye above
   it). The holes: where a model crosses the plane, the region its crossings
   enclose, in the picture's pixels (convex, at most 40 corners), where the
   water is not drawn (fx_draw.c fx_draw_water). */
typedef struct {
    int n;
    float x[40], y[40];
} FxHole;
void fx_face_cut(int on, const double n[3], double d);
const FxHole *fx_face_holes(int *n);

/* The ground's survey (fx_ground.c): while sinks are set, every model point
   the frame program projects goes to `point` too, in view space, and every
   face the chip queues to `face`: its corners in view space, its colour
   byte (the draw list's, after the kind; a textured face's is its
   texture's), whether it is textured, and whether the chip shaded it by
   the light (a lit face), with its light level unrounded (NULLs end it). */
typedef void (*FxSurveyPoint)(double x, double y, double z, void *context);
typedef void (*FxSurveyFace)(const double (*corner)[3], int n, uint8_t colour, int textured, int lit, float light,
                             void *context);
void fx_face_survey(FxSurveyPoint point, FxSurveyFace face, void *context);
/* The object FxObjectsDraw took last, made exactly (fx_face_object): 1 and
   its place in view space and the view's turn (row k, world axis k in view
   space) when its place is known; else 0. */
int fx_face_exact_object(double place[3], double view[3][3]);
/* The object being drawn's depth in view space, whole: exact (with its
   car's seat), else its draw record's as the chip made it; for what it
   draws with no depth of its own (a sprite, the chip's rows); 0 for none. */
double fx_face_object_depth(void);
/* A picture's 3D begins (fx_draw_part): the cars' seats its own, none
   carried over from the picture before. */
void fx_face_picture(void);
/* With the depth test, a car's tyre (a solid, fx_solid.c) of `radius`
   whose middle is pos in view space: moved along the normal of the ground
   under it to its radius over it, the move kept for its car's other parts.
   1 when it was seated. */
int fx_face_seat_tyre(float pos[3], double radius);

/* The view's rectangle in the picture's pixels: left, right (past its last
   column), top, bottom (past its last row). */
void fx_face_view_rect(float *left, float *right, float *top, float *bottom);

/* A scaled sprite's place in floats, for fx_draw's sprite quads: its
   picture centre and the scale from the chip's whole-pixel box (whose
   corner is at left, top) to it. Returns 0 when the box is not the one
   fx_sprite_place last saw. */
int fx_sprite_map(uint16_t left, uint16_t top, float *cx, float *cy, float *bx, float *by, float *k);
/* Whether the sprite fx_sprite_map last gave is of fixed size (its size
   byte negative: the same pixels at any depth, as the rank sign's). */
int fx_sprite_fixed(void);

/* the model whose faces are being walked (frame.c), and its face count */
void fx_face_shape_now(uint16_t shape);

#endif
