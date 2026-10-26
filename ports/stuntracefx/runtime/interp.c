/*
 * interp.c: in-between frames. The game's own frames are untouched: the
 * Super FX programs that draw a race frame (the view matrix $01:8325, the
 * sort $01:AEF8, the frame $01:AB13) are run again in a sandbox
 * (superfx.c gsu_sandbox_*) on inputs between the last two frames' (the
 * view's position and angles, each drawn object's position and angles),
 * and the Super FX is put back as it was.
 *
 * An in-between draw leaves its screen in the sandbox's RAM. The picture
 * is made whole from the last finished picture's record (video.h
 * VidFrame): each VRAM byte that the screen's upload brought from the
 * Super FX's RAM (followed through the DMA, vid_vram_next) taken from the
 * in-between draw's RAM instead, what the S-CPU wrote into the screen after
 * the draw (the map's markers) laid over it, the backdrop (BG2) turned
 * with the view; drawn by the reference compositor (vid_draw_frame).
 *
 * Live (interp_live_set, the windowed game): while frame N is on screen,
 * each video frame shows the picture between frame N - 1 and N at
 * t = (video frames N has been shown + 1) / (video frames N - 1 was shown),
 * N itself once t reaches 1: one game frame later than the game, smooth.
 *
 * Files (headless, the checks):
 *   PORT_INTERP=DIR        pictures written to DIR as PPM files,
 *                          cFFFFFF_K.ppm: K of N steps after game frame
 *                          FFFFFF's picture (K = N: that picture itself,
 *                          drawn again: checked against the game's)
 *   PORT_INTERP_STEPS=N    N - 1 in-between frames per frame (default 4)
 *   PORT_INTERP_FROM=F, PORT_INTERP_TO=F
 *                          only draws ending in video frames F..F write
 *                          files (all are checked)
 *   PORT_INTERP_CANVAS=1   also the bare 3D canvases (fFFFFFF_K.ppm)
 *
 * What it reads of the Super FX's RAM: the records the S-CPU uploads at
 * $0AAA (30 bytes, the count at $01C8: position relative to the view +$12
 * X, +$10 Y, +$14 Z, the depth key +$02, angles +$04-$06, flags +$07, model
 * +$08), the view's position and angles at $00D8-$00E2 (GsuParamsUpdate)
 * and the view matrix program's input angles at $0020-$0024.
 */
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "snes.h"
#include "draw_list.h"
#include "video.h"
#include "gsu.h"
#include "fx_calls.h"
#include "smoke.h"
#include "hires.h"
FxRegs FxCarPose(FxRegs g, int copy);   /* the game's copy (fx_car.c) */
void hires_gsu(uint32_t a, int done);   /* the high-resolution capture (hires.c) */

extern FxCanvas *canvas_capture;
uint32_t vid_vram_next(uint8_t reg);

#define REC_BASE 0x0AAA
#define REC_SIZE 0x1E
#define REC_COUNT 0x01C8
#define MAXREC 64
#define MAXSTEPS 16
#define VIEW_PARAMS 0x00D8      /* X, Y, Z, then the three angles */
#define MATRIX_IN 0x0020        /* the view matrix program's angles */
#define JUMP 0x400              /* a move this big in one frame is a cut */
#define ANGLE_JUMP 0x2000
#define RAM_SIZE 0x10000
#define SCREEN_SIZE 0x6800      /* a race screen: the three thirds the S-CPU uploads */
#define LAST_THIRD 0x4400       /* a byte past here uploaded: the last third, the picture flips to it */
#define SLOTS 4                 /* N - 1 and N on screen, N + 1 uploading, N + 2 being drawn */

/* what each of the two backdrop buffers was last built from
   (BackdropRowsUpdate, the port's copy: interp_note_backdrop) */
typedef struct {
    int set;
    uint16_t cd, cf, da2, da4, vbase;
} Backdrop;

/* where a picture's bytes came from: the VRAM map (vram_src, vram_when),
   the screen bytes as uploaded (upl, upl_when) and the backdrop's buffers;
   the live ones, or a draw's as its screen first stood whole (a snapshot) */
typedef struct {
    const uint32_t *src, *when, *upl_when;
    const uint8_t *upl;
    const Backdrop *bd;
} Maps;

typedef struct {
    int valid, map_ok;
    int far;                    /* its records (pre) the far cars' rebuild's, not the game's: shown redrawn */
    int far_sorted;             /* and post's sorted list theirs (far_sort) */
    long seq;                   /* which draw (1, 2, ...) */
    uint8_t pre[RAM_SIZE];      /* the RAM as the sort starts */
    uint8_t post[RAM_SIZE];     /* the RAM as the frame's draw starts */
    uint8_t done[RAM_SIZE];     /* the RAM as the frame's draw ends */
    void *sb;                   /* the whole Super FX as the draw starts */
    uint16_t obj[MAXREC];       /* the object each record came from */
    uint8_t oam[544];           /* the sprite table's buffer ($7E:0375) as it ended: the HUD */
    /* the text program ($01:D307: a notice, the place badge) the S-CPU ran
       after the draw: each run's string, its bank, its place */
    int ntxt, txt_known;
    uint16_t txt[4][4];
    uint8_t *after;             /* the game's own draws: the RAM after its texts (the overlay's base) */
    int after_ok;
    /* its screen as the S-CPU left it (the place badge, a banner painted
       into it after the draw), taken as the next draw starts */
    uint8_t painted[SCREEN_SIZE];
    volatile int painted_ok;
    int nrec;
    FxCanvas real;              /* what the game drew */
    uint16_t base;              /* its screen buffer (the start at $021A) */
    uint32_t ended;             /* the video frame (snes_frame) it ended in */
    uint32_t visible;           /* the video frame its screen is first shown in, 0 not yet */
    int nb;                     /* live: in-between screens made from the draw before (1..nb-1) */
    uint8_t *between[13];       /* their RAM after the draw, at t = k / nb */
    uint16_t between_angles[13][3];
    int prepared;               /* live: its in-betweens made (at prep_at) */
    uint32_t prep_at;
    int settled;                /* live: its screen stood whole on screen (a snapshot made) */
    uint32_t settle_at;
    VidFrame *spic;             /* the picture then */
    uint32_t *ssrc, *swhen, *supl_when;
    uint8_t *supl;
    Backdrop sbd[2];
    Maps sm;
    HrList hr;                  /* its 3D as shapes (hires.h), for the high-resolution picture */
} Draw;

/* files: a finished draw's in-between screens, waiting for its picture */
typedef struct {
    int ready, n;               /* n: steps made (1: only the frame itself) */
    long frame;
    uint8_t *ram[MAXSTEPS + 1]; /* the RAM after each step's draw (index steps: the frame) */
    uint16_t angles[MAXSTEPS + 1][3];
    Draw d;                     /* the draw's base, end and RAM at its end (a copy) */
} Result;

static int on = -1, files, live, steps = 4, canvases;
/* PORT_PHYS_DUMP=FILE: each car record ($98 bytes, handler FxCarHandler) as
   the physics program $04:8FCA starts and ends, a record per car per run:
   u32 video frame, u16 record, then the bytes before and after */
static FILE *phys_f;
/* PORT_TIMESCALE=K (the 60 a second mode's experiment): after each run of
   the physics program, each car's integrated fields (position, velocity,
   angles, angular rates, steering) and the program's two race timers move
   by 1/K of what the run moved them, the fractions kept: K runs make one
   of the original's */
static double timescale;
static uint16_t boost_rec[8];
static double boost_fraction[8];

/* Called only at the game's actual fuel-consumption instruction. Scaling
   that cost preserves pickups/refills and the game's unlimited-boost flags.
   Keep fractions so a cost smaller than K still eventually consumes fuel. */
uint16_t interp_boost_cost(uint16_t rec, uint16_t cost)
{
    int slot, whole;
    double amount;
    if (!timescale)
        return cost;
    for (slot = 0; slot < 8 && boost_rec[slot] && boost_rec[slot] != rec; slot++)
        ;
    if (slot == 8)
        snes_fatal("timescale: too many boost records");
    boost_rec[slot] = rec;
    amount = cost / timescale + boost_fraction[slot];
    whole = (int)(amount + 1e-9);
    boost_fraction[slot] = amount - whole;
    return (uint16_t)whole;
}

int port_timescale_k;                /* the time scale as a whole number, for the game's copy (race_phases.c) */
static const uint8_t scaled_field[] = { 0x1E, 0x20, 0x22, 0x24, 0x26, 0x28, 0x2E, 0x30, 0x32, 0x34, 0x36, 0x38, 0x3A, 0x82 /* damage */ };
#define NSCALED (sizeof scaled_field)
/* per-tick counters in a car's record (the computer cars' stuck counter
   +$7E and accelerating pause +$7C among them): a step of 1 or 2 counts
   1/K, a jump (set to a new value) counts whole */
static const uint8_t counter_field[] = { 0x4E, 0x72, 0x7C, 0x7E, 0x86 };
#define NCOUNTERS (sizeof counter_field)
static uint16_t tc_before[8][NCOUNTERS];
extern int world_frame;              /* the race frame that runs the phases (race_frame.c's port copy) */
/* each car's wheels (its parts): their speed and suspension words, scaled
   the same way (a jump past 4000 counts whole: a reset) */
static const uint8_t wheel_field[] = { 0x2E, 0x30, 0x32, 0x36, 0x38, 0x3A, 0x42, 0x4C };
#define NWHEEL (sizeof wheel_field)
#define MAXPARTS 6
static uint16_t tw_part[8][MAXPARTS];
static uint16_t tw_before[8][MAXPARTS][NWHEEL];
static double tw_frac[8][MAXPARTS][NWHEEL];
static int wheels_on = 1;
static double tc_frac[8][NCOUNTERS];
static uint16_t ts_rec[8];
static uint16_t ts_before[8][NSCALED];
static double ts_frac[8][NSCALED];
static uint16_t ts_frac_rec[8];
static uint16_t ts_timer_before[2];
static double ts_timer_frac[2];
static int ts_n;
static long ts_runs, ts_cars;
static uint16_t phys_rec[8];
static uint8_t phys_before[8][0x98];
static uint8_t phys_part_before[8][4][0x60];
static uint16_t phys_part[8][4];
static int phys_n;
static long from = 0, to = -1;
static const char *dir;
static Draw *dr[SLOTS];
static int newest = -1, sorted, pending, sandboxed;
static long draws;
static Result *res;
static void *real_state;
static uint16_t build_obj[MAXREC];
static int build_n;
static FxCanvas out;
static uint32_t vram_src[0x10000];   /* VRAM byte: where it came from (bank $70 or $7F) + 1, 0 elsewhere */
static uint32_t vram_when[0x10000];  /* and the video frame it came in */
static uint8_t upl[0x10000];         /* each Super FX RAM byte as last uploaded */
static uint32_t upl_when[0x10000];   /* and the video frame of that */
static void (*dma_prev)(uint32_t a, uint8_t b, uint8_t v);
static Backdrop bd[2];               /* each backdrop buffer: what it was last built from */
static const Maps live_maps = { vram_src, vram_when, upl_when, upl, bd };
static VidFrame *pic;
static uint8_t *fixed, *live_ram, *tmp_ram;
static uint32_t *rgb, *rgb_real, *rgb_live;
static const uint32_t *live_pic;

static uint32_t shown_len = 6;
int interp_active;                   /* in a sandboxed draw: the angles' fractions are used */
int port_fast_upload;                /* the race screen uploaded in one go (dma.c's port copy), 0 in three stages */
int port_steady_ticks;               /* the steady game speed: a race frame every N video frames, 0 the original's */
static uint8_t angle_frac[MAXREC][3];
static long checked, matched, made, recs_lerped, recs_kept, cuts, recs_moved_scenery, recs_moved_physics;
static long pictures, pictures_shown, pictures_matched, pictures_made;
static long live_between, live_own, live_unsure;
static double t_max, t_prep_max, t_sum; static long t_n, t_over8, t_over16;
static double now_ms(void) { struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts); return ts.tv_sec * 1e3 + ts.tv_nsec / 1e6; }

static void write_picture(long frame, int k, const uint32_t *px);
/* the HUD's Super FX drawings (RaceSoundsAndHud: the time left's digits,
   banners, glyphs) recorded in the frames it runs, and drawn again in the
   frames between (time scale): program, registers, the parameter page */
#define HUD_MAX 24
typedef struct {
    uint8_t bank;
    uint16_t addr;
    RegsGSU regs;
    uint8_t params[0x400];
} HudCall;
static HudCall hud[HUD_MAX];
static int hud_n, hud_recording;
static void log_physics(const uint8_t *ram);
static void log_frame(double frame_ms);
int port_lod_near;                   /* near shapes in the redraws (frame.c reads it; see lod_near) */
long port_lod_forced;
static int lod_near(void);
static const Draw *before(const Draw *d);
static void near_learn(const Draw *p, const Draw *d);
static int la_child;
static void la_child_done(const Draw *d);
static void la_check(const Draw *d);
static void la_fork(void);
static int la_enabled(void);
static void log_mode_change(int old_mode);
static long lg_race_frames;
static int in_race;

static uint16_t rd16(const uint8_t *m, unsigned a) { return (uint16_t)(m[a & 0xFFFF] | m[(a + 1) & 0xFFFF] << 8); }
static void wr16(uint8_t *m, unsigned a, uint16_t v) { m[a & 0xFFFF] = (uint8_t)v; m[(a + 1) & 0xFFFF] = (uint8_t)(v >> 8); }
static int s16(int v) { return (int16_t)(uint16_t)v; }
static uint16_t lerp16(uint16_t a, uint16_t b, double t) { return (uint16_t)(a + (int)((double)s16(b - a) * t + (s16(b - a) >= 0 ? 0.5 : -0.5))); }

static void report(void)
{
    if (files) {
        fprintf(stderr, "interp: draws: %ld checked, %ld drawn again exactly; %ld in-between draws; records interpolated %ld, "
                "kept %ld; view cuts %ld\n", checked, matched, made, recs_lerped, recs_kept, cuts);
        fprintf(stderr, "interp: pictures: %ld tried, %ld with the frame's screen fully shown, %ld of those made again exactly; "
                "%ld in-between pictures\n", pictures, pictures_shown, pictures_matched, pictures_made);
    }
    if (timescale)
        fprintf(stderr, "timescale: 1/%.1f over %ld physics runs, %ld car records\n", timescale, ts_runs, ts_cars);
    if (live && getenv("PORT_HIRES_DEBUG"))
        fprintf(stderr, "interp: in-between records: %ld interpolated, %ld moving with physics, %ld moved scenery put where the nearer draw has it\n",
                recs_lerped, recs_moved_physics, recs_moved_scenery);
    if (live)
        fprintf(stderr, "interp: live: %ld in-between pictures shown, %ld game pictures (%ld of them for want of a whole screen); time a frame: mean %.2f ms, max %.2f ms "
                "(making ahead max %.2f ms); frames over 8 ms %ld, over 16 ms %ld\n", live_between, live_own + live_unsure, live_unsure,
                t_n ? t_sum / t_n : 0, t_max, t_prep_max, t_over8, t_over16);
}

/* ---- the DMA: where each VRAM byte came from, and the draw on screen ---- */
static void dma_seen(uint32_t a, uint8_t b, uint8_t v)
{
    if (b == 0x18 || b == 0x19) {
        uint32_t i = vid_vram_next(b) & 0xFFFF, now = snes_frame();
        vram_src[i] = (a >> 16) == 0x70 || (a >> 16) == 0x7F ? (a & 0xFFFFFF) + 1 : 0;
        vram_when[i] = now;
        if ((a >> 16) == 0x70) {
            upl[a & 0xFFFF] = v;
            upl_when[a & 0xFFFF] = now;
            if (newest >= 0) {
                int k;
                for (k = 0; k < SLOTS; k++) {
                    Draw *d = dr[k];
                    uint16_t o = (uint16_t)((a & 0xFFFF) - d->base);
                    if (d->valid && !d->visible && now >= d->ended && o >= LAST_THIRD && o < SCREEN_SIZE)
                        d->visible = now + 1;
                }
            }
        }
    }
    if (dma_prev)
        dma_prev(a, b, v);
}

static void dma_watch(void)
{
    if (snes_dma_hook != dma_seen) {
        dma_prev = snes_dma_hook;
        snes_dma_hook = dma_seen;
    }
}

static int enabled(void)
{
    if (on < 0) {
        const char *e;
        int k;
        dir = getenv("PORT_INTERP");
        files = dir != NULL;
        if ((e = getenv("PORT_INTERP_STEPS")))
            steps = atoi(e) > 0 && atoi(e) <= MAXSTEPS ? atoi(e) : 4;
        if ((e = getenv("PORT_INTERP_FROM")))
            from = atol(e);
        if ((e = getenv("PORT_INTERP_TO")))
            to = atol(e);
        canvases = getenv("PORT_INTERP_CANVAS") != NULL;
        phys_f = getenv("PORT_PHYS_DUMP") ? fopen(getenv("PORT_PHYS_DUMP"), "wb") : NULL;
        timescale = getenv("PORT_TIMESCALE") ? atof(getenv("PORT_TIMESCALE")) : 0;
        if (timescale < 1.0)
            timescale = 0;
        port_timescale_k = (int)(timescale + 0.5);
        on = files || live || phys_f || timescale || getenv("PORT_SMOKE_TRACE");
        if (on) {
            for (k = 0; k < SLOTS; k++) {
                int j;
                dr[k] = (Draw *)calloc(1, sizeof(Draw));
                dr[k]->sb = malloc(gsu_sandbox_size());
                for (j = 0; j < 13; j++)
                    dr[k]->between[j] = (uint8_t *)malloc(RAM_SIZE);
                dr[k]->spic = (VidFrame *)malloc(sizeof(VidFrame));
                dr[k]->ssrc = (uint32_t *)malloc(sizeof vram_src);
                dr[k]->swhen = (uint32_t *)malloc(sizeof vram_when);
                dr[k]->supl_when = (uint32_t *)malloc(sizeof upl_when);
                dr[k]->supl = (uint8_t *)malloc(sizeof upl);
            }
            real_state = malloc(gsu_sandbox_size());
            res = (Result *)calloc(1, sizeof *res);
            for (k = 0; k <= MAXSTEPS && files; k++)
                res->ram[k] = (uint8_t *)malloc(RAM_SIZE);
            pic = (VidFrame *)malloc(sizeof *pic);
            rgb = (uint32_t *)malloc(VID_WIDTH * VID_HEIGHT * 4);
            rgb_real = (uint32_t *)malloc(VID_WIDTH * VID_HEIGHT * 4);
            rgb_live = (uint32_t *)malloc(VID_WIDTH * VID_HEIGHT * 4);
            fixed = (uint8_t *)malloc(RAM_SIZE);
            live_ram = (uint8_t *)malloc(RAM_SIZE);
            tmp_ram = (uint8_t *)malloc(RAM_SIZE);
            atexit(report);
        }
    }
    if (on)
        dma_watch();
    return on;
}

/* the windowed game: the in-between frames shown (before the first frame) */
void interp_live_set(int v)
{
    if (on < 0) {
        live = v;
        enabled();
    } else if (on)
        live = v;
}

int interp_live(void) { return live; }

/* ---- the game's draws ---- */

/* FxFrameProgram reading angle byte `at` of a record ($0AAA + 30 i + 4..6):
   its fraction (in the word's high byte), in a sandboxed draw */
uint16_t interp_angle_frac(uint16_t at)
{
    unsigned o = (uint16_t)(at - REC_BASE), i = o / REC_SIZE, c = o % REC_SIZE - 4;
    if (!interp_active || i >= MAXREC || c > 2)
        return 0;
    return (uint16_t)(angle_frac[i][c] << 8);
}

/* the other cars in 3D farther out (game/object/object.c,
   port_far_cars_list): the draw list as rebuilt with them, for the
   picture's redraws of the draw that follows (the game draws its own) */
extern int snes_untimed;
static uint8_t far_rec[MAXREC * REC_SIZE];
static uint16_t far_obj[MAXREC];
static int far_n, far_pending;
static long far_draws, far_redraws, far_busy;

/* car record rec's parts (wheels) placed on it as it stands (the game's
   FxCarPose, untimed): for the far cars' rebuild, on its copy of the state */
void interp_far_pose(uint16_t rec)
{
    RegsGSU keep = gsu;
    FxRegs g;
    memset(&g, 0, sizeof g);
    g.r[8] = rec;
    g.r[10] = 0x0240;
    gsu_set_untimed(1);
    FxCarPose(g, 0);
    gsu_set_untimed(0);
    gsu = keep;
}

static int la_enabled(void);

void interp_far_take(const uint8_t *rec, uint16_t n)
{
    /* (the look-ahead's pictures are all redraws; without it the game's
       own pictures show between the in-betweens: the far cars would blink) */
    if (n > MAXREC || n != build_n || !la_enabled())
        return;
    memcpy(far_rec, rec, (size_t)n * REC_SIZE);
    memcpy(far_obj, build_obj, sizeof far_obj);
    far_n = n;
    far_pending = 1;
}

/* ObjectDrawListBuild writing record r for object o (the port's copy of it) */
void interp_note_record(uint16_t r, uint16_t o)
{
    int i = (int)(uint16_t)(r - 0xF77F) / REC_SIZE;
    if (on <= 0 || sandboxed)
        return;
    if (i == 0 && !snes_untimed)
        far_pending = 0;        /* the game's next list: the last one's rebuild is old */
    if (i == 0)
        build_n = 0;
    if (i < MAXREC) {
        build_obj[i] = o;
        if (i + 1 > build_n)
            build_n = i + 1;
    }
}

/* With the time scale, a program not run this race frame (it STOPs at
   once): the timed records' program $01:BBC7 but every K-th frame (their
   timers keep the original's pace), and the physics $04:8FCA likewise
   while the cars stand on the grid (the race's clock not running): there
   the original's 10 a second exactly */
int interp_gsu_skip(uint8_t bank, uint16_t addr)
{
    uint32_t a = (uint32_t)bank << 16 | addr;
    if (!timescale || sandboxed || world_frame)
        return 0;
    if (a == 0x01BBC7)
        return 1;
    if (a == 0x048FCA && !snes_wram()[0x0E5B])
        return 1;
    return 0;
}

/* a Super FX program starting (done 0) or at its STOP (done 1) */
static void txt_run(uint8_t *ram, int done);
static void paint_take(Draw *pv, const uint8_t *ram);

void interp_gsu(uint8_t bank, uint16_t addr, int done)
{
    uint32_t a = (uint32_t)bank << 16 | addr;
    const uint8_t *ram;
    Draw *d;
    if (!sandboxed && !la_child)
        hires_gsu(a, done);
    if (!enabled() || sandboxed)
        return;
    ram = snes_sfx_ram();
    if (a == 0x01D307 && live && getenv("PORT_TXT_SMOOTH")) {
        txt_run(ram, done);
        return;
    }
    if (la_child)
        goto draws;             /* a look-ahead copy: its draw only, no traces */
    if (timescale && a == 0x048FCA) {
        static const uint16_t timer[2] = { 0x2470, 0x2472 };
        if (!done) {
            uint16_t r = rd16(ram, 0x2332);
            int guard = 0, c;
            ts_n = 0;
            while (r && guard++ < 64 && ts_n < 8) {
                if (rd16(ram, (uint16_t)(r + 0x0A)) == 0xB1A1) {
                    ts_rec[ts_n] = r;
                    for (c = 0; c < (int)NSCALED; c++)
                        ts_before[ts_n][c] = rd16(ram, (uint16_t)(r + scaled_field[c]));
                    for (c = 0; c < (int)NCOUNTERS; c++)
                        tc_before[ts_n][c] = rd16(ram, (uint16_t)(r + counter_field[c]));
                    {
                        uint16_t part = rd16(ram, (uint16_t)(r + 0x3E));
                        int q, g = 0;
                        for (q = 0; q < MAXPARTS; q++) {
                            tw_part[ts_n][q] = part;
                            if (part) {
                                for (c = 0; c < (int)NWHEEL; c++)
                                    tw_before[ts_n][q][c] = rd16(ram, (uint16_t)(part + wheel_field[c]));
                                part = g++ < 16 ? rd16(ram, (uint16_t)(part + 0x34)) : 0;
                            }
                        }
                    }
                    ts_n++;
                }
                r = rd16(ram, (uint16_t)(r + 0x04));
            }
            for (c = 0; c < 2; c++)
                ts_timer_before[c] = rd16(ram, timer[c]);
            ts_runs++;
            ts_cars += ts_n;
        } else {
            uint8_t *wram_fx = snes_sfx_ram();
            int k, c, slot;
            for (k = 0; k < ts_n; k++) {
                /* a car moved further than it can in a run (put back on
                   the course): this run's changes kept whole */
                int jumped = 0;
                for (c = 0; c < 3; c++)
                    if (abs(s16(rd16(wram_fx, (uint16_t)(ts_rec[k] + scaled_field[c])) - ts_before[k][c])) > 1500)
                        jumped = 1;
                if (jumped)
                    continue;
                /* the car's fractions, by record */
                for (slot = 0; slot < 8 && ts_frac_rec[slot] && ts_frac_rec[slot] != ts_rec[k]; slot++)
                    ;
                if (slot == 8)
                    continue;
                if (!ts_frac_rec[slot]) {
                    ts_frac_rec[slot] = ts_rec[k];
                    memset(ts_frac[slot], 0, sizeof ts_frac[slot]);
                    memset(tc_frac[slot], 0, sizeof tc_frac[slot]);
                    memset(tw_frac[slot], 0, sizeof tw_frac[slot]);
                }
                {
                    int corr[3] = { 0, 0, 0 };
                    uint16_t part;
                    int guard = 0;
                    /* before the race's clock runs (the cars on the grid):
                       the physics at the original's 10 a second, every
                       K-th run kept whole and the others undone (standing,
                       the scaled steps settle elsewhere than the original's) */
                    int grid = !snes_wram()[0x0E5B];
                    for (c = 0; c < (int)NSCALED; c++) {
                        uint16_t at = (uint16_t)(ts_rec[k] + scaled_field[c]);
                        uint16_t now_v = rd16(wram_fx, at), to;
                        if (grid)
                            to = world_frame ? now_v : ts_before[k][c];
                        else if (scaled_field[c] == 0x3A && (ts_before[k][c] == 0 || now_v == 0 ||
                                                              ((ts_before[k][c] ^ now_v) & 0x8000))) {
                            /* the steering's press (from 0), release (to 0)
                               and crossing: whole, as the game sets them; only
                               its build-up while held is scaled */
                            to = now_v;
                            ts_frac[slot][c] = 0;
                        } else {
                            double d = s16(now_v - ts_before[k][c]) / timescale + ts_frac[slot][c];
                            int whole = (int)d;
                            ts_frac[slot][c] = d - whole;
                            to = (uint16_t)(ts_before[k][c] + whole);
                        }
                        wr16(wram_fx, at, to);
                        if (c < 3)
                            corr[c] = s16(to - now_v);   /* how far the car was moved back */
                    }
                    for (c = 0; c < (int)NCOUNTERS; c++) {
                        uint16_t at = (uint16_t)(ts_rec[k] + counter_field[c]);
                        int dlt = s16(rd16(wram_fx, at) - tc_before[k][c]);
                        if (grid) {
                            if (!world_frame)
                                wr16(wram_fx, at, tc_before[k][c]);
                        } else if (dlt && abs(dlt) <= 2) {
                            double d = dlt / timescale + tc_frac[slot][c];
                            int whole = (int)d;
                            tc_frac[slot][c] = d - whole;
                            wr16(wram_fx, at, (uint16_t)(tc_before[k][c] + whole));
                        } else if (dlt)
                            tc_frac[slot][c] = 0;
                    }
                    if (wheels_on && !grid) {
                        int q;
                        for (q = 0; q < MAXPARTS && tw_part[k][q]; q++)
                            for (c = 0; c < (int)NWHEEL; c++) {
                                uint16_t at = (uint16_t)(tw_part[k][q] + wheel_field[c]);
                                int dlt = s16(rd16(wram_fx, at) - tw_before[k][q][c]);
                                if (!dlt)
                                    continue;
                                if (abs(dlt) > 4000) {
                                    tw_frac[slot][q][c] = 0;
                                    continue;
                                }
                                {
                                    double d = dlt / timescale + tw_frac[slot][q][c];
                                    int whole = (int)d;
                                    tw_frac[slot][q][c] = d - whole;
                                    wr16(wram_fx, at, (uint16_t)(tw_before[k][q][c] + whole));
                                }
                            }
                    }
                    /* its parts (wheels; the list at +$3E, linked by +$34)
                       were placed from the car's unscaled pose this run:
                       placed again from its scaled one (the game's own
                       FxCarPose: its matrix from the angles, then
                       FxCarMoveParts), untimed */
                    if (!grid) {
                        RegsGSU keep = gsu;
                        FxRegs g;
                        memset(&g, 0, sizeof g);
                        g.r[8] = ts_rec[k];
                        g.r[10] = 0x0240;
                        gsu_set_untimed(1);
                        FxCarPose(g, 0);
                        gsu_set_untimed(0);
                        gsu = keep;
                    } else
                        for (part = rd16(wram_fx, (uint16_t)(ts_rec[k] + 0x3E)); part && guard++ < 16;
                             part = rd16(wram_fx, (uint16_t)(part + 0x34)))
                            for (c = 0; c < 3; c++) {
                                uint16_t at = (uint16_t)(part + 0x1E + 2 * c);
                                wr16(wram_fx, at, (uint16_t)(rd16(wram_fx, at) + corr[c]));
                            }
                }
            }
            for (c = 0; c < 2; c++) {
                double d = s16(rd16(wram_fx, timer[c]) - ts_timer_before[c]) / timescale + ts_timer_frac[c];
                int whole = (int)d;
                ts_timer_frac[c] = d - whole;
                wr16(wram_fx, timer[c], (uint16_t)(ts_timer_before[c] + whole));
            }
        }
    }
    smoke_gsu(a, done, build_obj, build_n);
    if (a == 0x048FCA && done)
        log_physics(ram);
    if (hud_recording && !done && hud_n < HUD_MAX) {
        hud[hud_n].bank = bank;
        hud[hud_n].addr = addr;
        hud[hud_n].regs = gsu;
        memcpy(hud[hud_n].params, ram, sizeof hud[hud_n].params);
        hud_n++;
    }
    if (getenv("PORT_HANDLERS") && a == 0x048FCA && !done) {
        /* the records in the physics list, by handler (a census, at exit) */
        static long count[0x10000]; static int inited; static long runs;
        uint16_t r = rd16(ram, 0x2332);
        int guard = 0;
        if (!inited) {
            inited = 1;
        }
        runs++;
        while (r && guard++ < 128) {
            count[rd16(ram, (uint16_t)(r + 0x0A))]++;
            r = rd16(ram, (uint16_t)(r + 0x04));
        }
        if (runs == 400) {
            int h;
            for (h = 0; h < 0x10000; h++)
                if (count[h])
                    fprintf(stderr, "handler $%04X: %.2f records a run\n", h, count[h] / (double)runs);
        }
    }
    if (phys_f && a == 0x048FCA) {
        if (!done) {
            uint16_t r = rd16(ram, 0x2332);
            int guard = 0;
            phys_n = 0;
            while (r && guard++ < 64 && phys_n < 8) {
                if (rd16(ram, (uint16_t)(r + 0x0A)) == 0xB1A1) {
                    uint16_t part = rd16(ram, (uint16_t)(r + 0x3E));
                    int q;
                    phys_rec[phys_n] = r;
                    memcpy(phys_before[phys_n], ram + r, 0x98);
                    for (q = 0; q < 4; q++) {
                        phys_part[phys_n][q] = part;
                        if (part) {
                            memcpy(phys_part_before[phys_n][q], ram + part, 0x60);
                            part = rd16(ram, (uint16_t)(part + 0x34));
                        }
                    }
                    phys_n++;
                }
                r = rd16(ram, (uint16_t)(r + 0x04));
            }
        } else {
            uint32_t fr = snes_frame();
            int k;
            for (k = 0; k < phys_n; k++) {
                fwrite(&fr, 4, 1, phys_f);
                fwrite(&phys_rec[k], 2, 1, phys_f);
                fwrite(phys_before[k], 1, 0x98, phys_f);
                fwrite(ram + phys_rec[k], 1, 0x98, phys_f);
                fwrite(snes_wram() + 0xC5, 1, 6, phys_f);   /* the view's position ($C5/$C7/$C9) */
                {
                    /* the car's parts' positions, up to 6 (count first) */
                    uint16_t part = rd16(ram, (uint16_t)(phys_rec[k] + 0x3E)), n = 0, pos[18];
                    memset(pos, 0, sizeof pos);
                    while (part && n < 6) {
                        memcpy(&pos[3 * n], ram + (uint16_t)(part + 0x1E), 6);
                        n++;
                        part = rd16(ram, (uint16_t)(part + 0x34));
                    }
                    fwrite(&n, 2, 1, phys_f);
                    fwrite(pos, 2, 18, phys_f);
                }
                if (getenv("PORT_PARTS_DUMP")) {
                    /* the first four parts' first $60 bytes, before and after */
                    static FILE *pf;
                    int q;
                    if (!pf)
                        pf = fopen(getenv("PORT_PARTS_DUMP"), "wb");
                    for (q = 0; q < 4; q++) {
                        fwrite(&fr, 4, 1, pf);
                        fwrite(&phys_rec[k], 2, 1, pf);
                        fwrite(&phys_part[k][q], 2, 1, pf);
                        fwrite(phys_part_before[k][q], 1, 0x60, pf);
                        fwrite(phys_part[k][q] ? ram + phys_part[k][q] : phys_part_before[k][q], 1, 0x60, pf);
                    }
                }
                if (k == 0 && getenv("PORT_WRAM_TRACE")) {
                    /* the race's phase bytes, once a run */
                    static FILE *wt;
                    if (!wt)
                        wt = fopen(getenv("PORT_WRAM_TRACE"), "wb");
                    fwrite(&fr, 4, 1, wt);
                    fwrite(snes_wram() + 0x0D50, 1, 16, wt);
                    fwrite(snes_wram() + 0x0DD0, 1, 16, wt);
                    fwrite(snes_wram() + 0x0E50, 1, 16, wt);
                }
            }
        }
    }
draws:
    if (!files && !live)
        return;
    d = dr[(newest + 1) % SLOTS];
    if (a == 0x01AEF8 && !done && newest >= 0 && dr[newest]->valid)
        paint_take(dr[newest], ram);
    if (a == 0x01AEF8 && !done) {
        const uint8_t *w = snes_wram();
        int n = rd16(ram, REC_COUNT);
        d->valid = 0;
        memcpy(d->pre, ram, sizeof d->pre);
        memcpy(d->obj, build_obj, sizeof d->obj);
        d->nrec = n;
        /* the records the sort gets are the list last built */
        d->map_ok = n == build_n && n <= MAXREC && !memcmp(ram + REC_BASE, w + 0xF77F, (size_t)n * REC_SIZE);
        d->far = 0;
        if (far_pending && d->map_ok) {
            /* the list with the far cars in 3D, in place of the game's: the
               pictures redraw it (show_draw), the in-betweens move it */
            memcpy(d->pre + REC_BASE, far_rec, (size_t)far_n * REC_SIZE);
            wr16(d->pre, REC_COUNT, (uint16_t)far_n);
            memcpy(d->obj, far_obj, sizeof d->obj);
            d->nrec = far_n;
            d->far = 1;
            d->far_sorted = 0;
            far_draws++;
        }
        far_pending = 0;
        sorted = 1;
    } else if (a == 0x01AB13 && !done) {
        if (!sorted)
            return;
        memcpy(d->post, ram, sizeof d->post);
        if (getenv("PORT_DIFF_DEBUG") && !la_child)
            fprintf(stderr, "TXT f%u 2732 %04X 2734 %04X 2738 %04X 273A %04X 273C %04X\n", snes_frame(), rd16(ram, 0x2732), rd16(ram, 0x2734),
                    rd16(ram, 0x2738), rd16(ram, 0x273A), rd16(ram, 0x273C));
        gsu_sandbox_save(d->sb);
        canvas_capture = &d->real;
        hires_capture(&d->hr);
        hires_capture_begin();
    } else if (a == 0x01AB13 && done && sorted) {
        canvas_capture = NULL;
        hires_capture_end();
        memcpy(d->done, ram, sizeof d->done);
        memcpy(d->oam, snes_wram() + 0x0375, sizeof d->oam);
        d->base = rd16(d->post, 0x021A);
        d->ended = snes_frame();
        d->visible = 0;
        d->nb = 0;
        d->settled = 0;
        d->prepared = 0;
        d->seq = ++draws;
        d->painted_ok = 0;
        d->ntxt = 0;
        d->txt_known = !la_child;   /* (a copy's: known once its next draw comes) */
        d->after_ok = 0;
        d->valid = 1;
        sorted = 0;
        newest = (newest + 1) % SLOTS;
        pending = 1;
        if (!la_child)
            near_learn(before(d), d);
        if (la_child)
            la_child_done(d);
        if (la_enabled()) {
            la_check(d);
            la_fork();          /* the copy for the next frames, from here */
        }
    }
}

/* ---- the backdrop (BG2) ---- */


void interp_note_backdrop(uint16_t sel)
{
    const uint8_t *w = snes_wram();
    Backdrop *k = &bd[sel ? 1 : 0];
    if (on <= 0 || sandboxed)
        return;
    k->set = 1;
    k->cd = rd16(w, 0x06CD);
    k->cf = rd16(w, 0x06CF);
    k->da2 = rd16(w, 0x0DA2);
    k->da4 = rd16(w, 0x0DA4);
    k->vbase = rd16(w, 0xE811 + 2 * (sel ? 1 : 0));
}

/* the backdrop's angles from the view's (BackdropAnglesFromView): $06CD
   from $0664, $06CF from $0666 */
static void backdrop_angles(const uint16_t *a, uint16_t *cd, uint16_t *cf)
{
    uint16_t v = a[0], w = a[1];
    int i;
    for (i = 0; i < 6; i++)
        v = (uint16_t)((v >> 1) | (v & 0x8000));
    v = (uint16_t)-v;
    if (!((uint16_t)(v - 0x00E8) & 0x8000))
        v = 0x00E8;
    if ((uint16_t)(v - 0xFFC8) & 0x8000)
        v = 0xFFC8;
    *cd = (uint16_t)((v + 0x0098) & 0x01FF);
    for (i = 0; i < 5; i++)
        w = (uint16_t)((w >> 1) | (w & 0x8000));
    *cf = (uint16_t)(w & 0x07FF);
}

/* a buffer's 17 rows of tilemap as BackdropRowsUpdate makes them for
   backdrop angles cd, cf */
static void backdrop_rows(const Backdrop *k, uint16_t cd, uint16_t cf, uint8_t *rows)
{
    const uint8_t *w7f = snes_wram() + 0x10000;
    uint16_t src = (k->da4 & 0x4000) ? 0xBBD7 : 0x93D7, x, y;
    int n, i;
    x = (uint16_t)((((uint16_t)(cd + k->da2) >> 3) & 0x3F) * 0x140 + ((cf >> 3) & 0xFE));
    for (y = 0, n = 0; n < 17 && y < 0x440; n++) {
        if (x >= 0x2800) {
            for (; y < 0x440; y += 2)
                wr16(rows, y, 0x20C0);
            break;
        }
        for (i = 0; i < 32; i++, y += 2)
            wr16(rows, y, rd16(w7f, (uint16_t)(src + x + 2 * i)));
        x = (uint16_t)(x + 0x140);
    }
}

/* the backdrop moved on by the view's turn from angles own (the frame's)
   to a: each buffer rebuilt from what the game built it from plus that
   turn (the game's own lag between backdrop and 3D kept), into the VRAM
   bytes uploaded from it since the draw ended; and the fine scroll on the
   lines that have a buffer's own */
static void backdrop(VidFrame *p, const Maps *m, const uint16_t *a, const uint16_t *own, uint32_t ended)
{
    uint8_t rows[2][0x440];
    uint16_t cd, cf, ocd, ocf, ncd[2], ncf[2];
    unsigned i, line;
    int sel;
    backdrop_angles(a, &cd, &cf);
    backdrop_angles(own, &ocd, &ocf);
    for (sel = 0; sel < 2; sel++) {
        ncd[sel] = (uint16_t)((m->bd[sel].cd + cd - ocd) & 0x01FF);
        ncf[sel] = (uint16_t)((m->bd[sel].cf + cf - ocf) & 0x07FF);
        if (m->bd[sel].set)
            backdrop_rows(&m->bd[sel], ncd[sel], ncf[sel], rows[sel]);
    }
    for (i = 0; i < 0x10000; i++) {
        uint32_t s = m->src[i];
        uint16_t o;
        int v;
        if (!s || ((s - 1) >> 16) != 0x7F || (o = (uint16_t)(s - 1 - 0xE3D7)) >= 0x880 || m->when[i] < ended)
            continue;
        sel = o >= 0x440;
        if (!m->bd[sel].set)
            continue;
        o %= 0x440;
        for (v = 0; v < p->versions; v++) {
            uint16_t *wd = &p->vram[v][i >> 1];
            uint8_t b = rows[sel][o];
            *wd = (i & 1) ? (uint16_t)((*wd & 0x00FF) | b << 8) : (uint16_t)((*wd & 0xFF00) | b);
        }
    }
    for (line = 1; line <= VID_HEIGHT; line++) {
        VidScreen *l = &p->line[line];
        for (sel = 0; sel < 2; sel++) {
            uint16_t ho = (uint16_t)(((m->bd[sel].cf >> 1) & 7) - 0x18), hn = (uint16_t)(((ncf[sel] >> 1) & 7) - 0x18);
            uint16_t vo = (uint16_t)((m->bd[sel].cd & 7) - 0x20 + m->bd[sel].vbase), vn = (uint16_t)((ncd[sel] & 7) - 0x20 + m->bd[sel].vbase);
            if (!m->bd[sel].set || ((l->bghofs[1] ^ ho) & 0x3FF) || ((l->bgvofs[1] ^ vo) & 0x3FF))
                continue;
            l->bghofs[1] = (uint16_t)((l->bghofs[1] & ~0x3FF) | (hn & 0x3FF));
            l->bgvofs[1] = (uint16_t)((l->bgvofs[1] & ~0x3FF) | (vn & 0x3FF));
            break;
        }
    }
}

/* the wider view: the backdrop's panorama for the lines above line lw
   (the window's first), into picture p's BG2 tilemap rows those lines
   show: the panorama's rows above the buffer's (BackdropRowsUpdate's
   map, $7F:93D7/$BBD7, $140 bytes a row, 32 rows; past its bottom the
   ground tile $20C0). 1: done */
static int backdrop_above(VidFrame *p, const Maps *m, const uint16_t *a, const uint16_t *own, int lw)
{
    const uint8_t *w7f = snes_wram() + 0x10000;
    const VidScreen *l = &p->line[lw];
    uint16_t cd, cf, ocd, ocf;
    unsigned vofs = l->bgvofs[1] & 0x3FF, sc = l->bgsc[1], rows = (sc & 2) ? 64 : 32, base = (unsigned)(sc & 0xFC) << 9;
    unsigned prw = ((unsigned)(lw - 1) + vofs) % (rows * 8), trw = prw >> 3, tr, byte;
    uint32_t src;
    int sel, bw, line, k, maprow, done = 0;
    uint16_t ncd, ncf, x;
#define TM_BYTE(t, c) (base + ((t) >= 32 ? ((sc & 1) ? 0x1000u : 0x800u) : 0u) + ((t) & 31) * 64u + (unsigned)(c) * 2u)
    byte = TM_BYTE(trw, 0) & 0xFFFF;
    src = m->src[byte];
    if (!src || ((src - 1) >> 16) != 0x7F || (uint16_t)(src - 1 - 0xE3D7) >= 0x880)
        return 0;
    sel = (uint16_t)(src - 1 - 0xE3D7) >= 0x440;
    if (!m->bd[sel].set)
        return 0;
    bw = (int)((uint16_t)(src - 1 - 0xE3D7) % 0x440) / 64;
    backdrop_angles(a, &cd, &cf);
    backdrop_angles(own, &ocd, &ocf);
    ncd = (uint16_t)((m->bd[sel].cd + cd - ocd) & 0x01FF);
    ncf = (uint16_t)((m->bd[sel].cf + cf - ocf) & 0x07FF);
    {
        uint16_t srcm = (m->bd[sel].da4 & 0x4000) ? 0xBBD7 : 0x93D7;
        int r0 = (int)((((uint16_t)(ncd + m->bd[sel].da2) >> 3) & 0x3F));
        unsigned last = 0xFFFF;
        for (line = lw - 1; line >= 1; line--) {
            unsigned pr = ((unsigned)(line - 1) + vofs) % (rows * 8);
            tr = pr >> 3;
            if (tr == trw || tr == last)
                continue;
            last = tr;
            k = (int)((trw - tr + rows) % rows);
            maprow = r0 + bw - k;
            if (maprow < 0)
                maprow = 0;
            x = (uint16_t)(maprow * 0x140 + ((ncf >> 3) & 0xFE));
            for (k = 0; k < 32; k++) {
                uint16_t e = maprow * 0x140 >= 0x2800 ? 0x20C0 : rd16(w7f, (uint16_t)(srcm + x + 2 * k));
                unsigned b = TM_BYTE(tr, k) & 0xFFFF;
                int v;
                for (v = 0; v < p->versions; v++)
                    p->vram[v][b >> 1] = e;
            }
            done = 1;
        }
        /* the columns left of the window: the layer's first column starts
           at the window's left edge (its fine scroll -24), so the picture's
           first three columns show the tilemap's last three, wrapped (the
           buffer's far right): those made the panorama's three columns
           before the buffer's, on every row the lines show */
        last = 0xFFFF;
        for (line = 1; line <= VID_HEIGHT; line++) {
            const VidScreen *ll = &p->line[line];
            unsigned pr;
            int c;
            if (ll->bgsc[1] != sc || (ll->bgvofs[1] & 0x3FF) != vofs)
                continue;
            pr = ((unsigned)(line - 1) + vofs) % (rows * 8);
            tr = pr >> 3;
            if (tr == last)
                continue;
            last = tr;
            maprow = r0 + bw + (int)(((int)tr - (int)trw + (int)rows + (int)rows / 2) % (int)rows) - (int)rows / 2;
            if (maprow < 0)
                maprow = 0;
            for (c = 0; c < 3; c++) {
                int off = (int)((ncf >> 3) & 0xFE) - 6 + 2 * c;
                uint16_t e;
                unsigned b = TM_BYTE(tr, 29 + c) & 0xFFFF;
                int v;
                if (off < 0)
                    off += 0x140;
                e = maprow * 0x140 >= 0x2800 ? 0x20C0 : rd16(w7f, (uint16_t)(srcm + maprow * 0x140 + off));
                for (v = 0; v < p->versions; v++)
                    p->vram[v][b >> 1] = e;
            }
            done = 1;
        }
    }
#undef TM_BYTE
    return done;
}

/* ---- making a picture ---- */

/* ram (a step's, or the game's draw) into fixed, with what the S-CPU
   wrote into d's screen after the draw (the map's markers, notices: the
   bytes uploaded differing from the draw's) laid over it */
static const uint8_t *overlay(const Maps *m, const Draw *d, const uint8_t *ram)
{
    /* by pixel: the screen is 8 bits a pixel, 64 bytes a character, an
       8-pixel row's eight bit planes at +0 +1 +16 +17 +32 +33 +48 +49 (+2
       a row); a pixel the S-CPU changed (any plane differing) is taken as
       uploaded, every other as ram has it */
    static const unsigned plane[8] = { 0, 1, 16, 17, 32, 33, 48, 49 };
    unsigned c, r, p;
    memcpy(fixed, ram, RAM_SIZE);
    for (c = 0; c < SCREEN_SIZE / 64; c++)
        for (r = 0; r < 8; r++) {
            uint8_t mask = 0;
            uint16_t a[8];
            for (p = 0; p < 8; p++) {
                a[p] = (uint16_t)(d->base + c * 64 + plane[p] + 2 * r);
                if (m->upl_when[a[p]] >= d->ended)
                    mask |= (uint8_t)(m->upl[a[p]] ^ (d->after_ok ? d->after : d->done)[a[p]]);
            }
            if (!mask)
                continue;
            for (p = 0; p < 8; p++)
                if (m->upl_when[a[p]] >= d->ended)
                    fixed[a[p]] = (uint8_t)((ram[a[p]] & ~mask) | (m->upl[a[p]] & mask));
        }
    return fixed;
}

/* the picture with each VRAM byte that came from d's screen since it was
   drawn taken from ram instead (inv: inverted, to see whether the picture
   shows any of them); the count of such bytes */
static int patch(VidFrame *p, const Maps *m, const uint8_t *ram, const Draw *d, int inv)
{
    int v, n = 0;
    unsigned i;
    for (i = 0; i < 0x10000; i++) {
        uint32_t s = m->src[i];
        uint8_t b;
        if (!s || ((s - 1) >> 16) != 0x70 || (uint16_t)(s - 1 - d->base) >= SCREEN_SIZE || m->when[i] < d->ended)
            continue;
        n++;
        b = ram[(s - 1) & 0xFFFF];
        if (inv)
            b = (uint8_t)~b;
        for (v = 0; v < p->versions; v++) {
            uint16_t *w = &p->vram[v][i >> 1];
            *w = (i & 1) ? (uint16_t)((*w & 0x00FF) | b << 8) : (uint16_t)((*w & 0xFF00) | b);
        }
    }
    return n;
}

/* picture f with d's screen as ram has it and the backdrop at angles a,
   drawn into px */
static void picture(const VidFrame *f, const Maps *m, const Draw *d, const uint8_t *ram, const uint16_t *a, uint32_t *px)
{
    uint16_t own[3];
    int c;
    for (c = 0; c < 3; c++)
        own[c] = rd16(d->post, VIEW_PARAMS + 6 + 2 * c);
    memcpy(pic, f, sizeof *pic);
    patch(pic, m, overlay(m, d, ram), d, 0);
    backdrop(pic, m, a, own, d->ended);
    vid_draw_frame(pic, px);
}

/* ---- an in-between draw ---- */

/* ---- far objects as their near selves (with the near shapes forced) ----
   The game draws a far object (an opponent's car, a lane marking) as a
   flat sprite: its draw record (+$07 bit 3, the shape at +$08) as the
   S-CPU built it. In the redraws such a record is given the object's 3D
   shape instead, learnt from the game's own draws: when an object is a
   sprite (shape A) in one draw and a model (shape B) in the next (or the
   other way), A -> B for it, with B's flags and its kind, frame and
   material bytes (+$16 on: a car's own colours). Only an object still
   showing shape A is changed (an object slot used again for another is
   left alone). */
#define NEAR_MAX 400
typedef struct { uint16_t obj, sprite, model; uint8_t flags, tail[8]; } NearOf;
static NearOf near_of[NEAR_MAX];
static int near_n;
static long near_learnt, near_used;

static void near_learn(const Draw *p, const Draw *d)
{
    int i, j, k;
    if (!p || !d->map_ok || !p->map_ok)
        return;
    for (i = 0; i < d->nrec && i < MAXREC; i++) {
        unsigned r = REC_BASE + (unsigned)i * REC_SIZE;
        for (j = 0; j < p->nrec && j < MAXREC; j++) {
            unsigned q = REC_BASE + (unsigned)j * REC_SIZE;
            const uint8_t *sr, *mr;
            uint16_t sa, mb;
            if (p->obj[j] != d->obj[i])
                continue;
            if ((d->pre[r + 7] & 0x08) == (p->pre[q + 7] & 0x08))
                break;
            /* sr the sprite record, mr the model's */
            sr = (d->pre[r + 7] & 0x08) ? d->pre + r : p->pre + q;
            mr = (d->pre[r + 7] & 0x08) ? p->pre + q : d->pre + r;
            sa = rd16(sr, 8);
            mb = rd16(mr, 8);
            if (!sa || !mb)
                break;
            for (k = 0; k < near_n && near_of[k].obj != d->obj[i]; k++)
                ;
            if (k == near_n) {
                if (near_n == NEAR_MAX)
                    break;
                near_n++;
            }
            near_of[k].obj = d->obj[i];
            near_of[k].sprite = sa;
            near_of[k].model = mb;
            near_of[k].flags = mr[7];
            memcpy(near_of[k].tail, mr + 0x16, 8);
            near_learnt++;
            break;
        }
    }
}

/* the redraw's records (in ram, now's objects): sprites with a learnt 3D
   self made it */
static void near_apply(const Draw *now, uint8_t *ram)
{
    int i, k;
    for (i = 0; i < now->nrec && i < MAXREC; i++) {
        unsigned r = REC_BASE + (unsigned)i * REC_SIZE;
        if (!(ram[r + 7] & 0x08))
            continue;
        for (k = 0; k < near_n; k++)
            if (near_of[k].obj == now->obj[i] && near_of[k].sprite == rd16(ram, r + 8)) {
                wr16(ram, r + 8, near_of[k].model);
                ram[r + 7] = (uint8_t)((near_of[k].flags & 0x7F) | (ram[r + 7] & 0x80));
                memcpy(ram + r + 0x16, near_of[k].tail, 8);
                near_used++;
                break;
            }
    }
}

/* moved scenery not slid (PORT_NO_SLIDE=1, an experiment, off): an object
   that moves without a physics record taken as put somewhere new. Off:
   too many moving things have no physics record (a pickup's eye effect
   around the car, shadows, lamps), which then lagged behind their car */
static int slide_scenery_off(void)
{
    static int v = -1;
    if (v < 0)
        v = getenv("PORT_NO_SLIDE") && strcmp(getenv("PORT_NO_SLIDE"), "0");
    return v;
}

/* the sorted draw list (head $023C, linked by +$00) in RAM m as record
   numbers, farthest first; the count */
static int sorted_order(const uint8_t *m, int *out, int max)
{
    int n = 0;
    uint16_t at = rd16(m, 0x023C);
    while (at && n < max) {
        unsigned i = (uint16_t)(at - REC_BASE) / REC_SIZE;
        if ((uint16_t)(at - REC_BASE) % REC_SIZE || i >= MAXREC)
            break;
        out[n++] = (int)i;
        at = rd16(m, at);
    }
    return n;
}

static long sort_checked, sort_flips, sort_frames_flipped, sort_dropped, sort_added, sort_frames_d, sort_frames_a, sort_kept, sort_hidden;
static void sort_report(void)
{
    static int done;
    if (sort_checked && !done) {
        done = 1;
        fprintf(stderr, "sort check (%ld objects kept shown, %ld kept hidden): %ld in-betweens, %ld of them with object pairs in an order neither draw had (%ld pairs); "
                "%ld leave out objects both draws show (%ld objects), %ld show objects neither draw does (%ld)\n",
                sort_kept, sort_hidden, sort_checked, sort_frames_flipped, sort_flips, sort_frames_d, sort_dropped, sort_frames_a, sort_added);
    }
}

/* an in-between's order (records of now, objects by now->obj) against
   prev's and now's own (PORT_SORTCHECK) */
static void sort_check(const Draw *prev, const Draw *now, const uint8_t *ram)
{
    int ob[MAXREC], op[MAXREC], on[MAXREC], nb, np, nn, i, j, flips = 0;
    int pos_p[MAXREC], pos_n[MAXREC], ids_b[MAXREC];
    static int reg;
    if (!reg) {
        reg = 1;
        atexit(sort_report);
    }
    nb = sorted_order(ram, ob, MAXREC);
    np = sorted_order(prev->post, op, MAXREC);
    nn = sorted_order(now->post, on, MAXREC);
    for (i = 0; i < nb; i++)
        ids_b[i] = now->obj[ob[i]];
    for (i = 0; i < nb; i++) {
        pos_p[i] = pos_n[i] = -1;
        for (j = 0; j < np; j++)
            if (prev->obj[op[j]] == ids_b[i])
                pos_p[i] = j;
        for (j = 0; j < nn; j++)
            if (now->obj[on[j]] == ids_b[i])
                pos_n[i] = j;
    }
    for (i = 0; i < nb; i++)
        for (j = i + 1; j < nb; j++) {
            /* i before j in the in-between: before in at least one draw? */
            int in_p = pos_p[i] >= 0 && pos_p[j] >= 0, in_n = pos_n[i] >= 0 && pos_n[j] >= 0;
            if (!in_p || !in_n)
                continue;
            if (pos_p[i] > pos_p[j] && pos_n[i] > pos_n[j])
                flips++;
        }
    sort_checked++;
    sort_flips += flips;
    if (flips)
        sort_frames_flipped++;
    {
        /* objects both draws show but the in-between leaves out, and ones it
           shows that neither does */
        static long dropped, added, frames_d, frames_a;
        static int reg2;
        int d = 0, ad = 0;
        if (!reg2) {
            reg2 = 1;
            atexit(sort_report);
        }
        for (i = 0; i < np; i++) {
            int id = prev->obj[op[i]], in_n = 0, in_b = 0;
            for (j = 0; j < nn; j++)
                if (now->obj[on[j]] == id)
                    in_n = 1;
            for (j = 0; j < nb; j++)
                if (ids_b[j] == id)
                    in_b = 1;
            if (in_n && !in_b)
                d++;
        }
        for (i = 0; i < nb; i++)
            if (pos_p[i] < 0 && pos_n[i] < 0)
                ad++;
        dropped += d;
        added += ad;
        frames_d += d > 0;
        frames_a += ad > 0;
        sort_dropped = dropped;
        sort_added = added;
        sort_frames_d = frames_d;
        sort_frames_a = frames_a;
    }
}

/* the in-between's sorted list (in ram) put in the order of the draw
   nearer in time (ref: prev before half way, else now), by object; an
   object only the in-between has keeps its place after the one before it.
   Depth keys are not linear between two draws (the view turns, the key
   has a |Y| term): two objects near the same depth could swap for one
   in-between and something behind the road show in front of it */
static long sort_fixed;
static int wide_pending;     /* the redraw being made is the wider view's (draw_between) */

static void sort_like(const Draw *ref, const Draw *prev, const Draw *now, uint8_t *ram)
{
    int ob[MAXREC], orf[MAXREC], op[MAXREC], on[MAXREC], nb, nr, np, nn, i, j, k;
    double rank[MAXREC];
    if ((ref->far && !ref->far_sorted) || (prev->far && !prev->far_sorted) || (now->far && !now->far_sorted))
        return;
    nb = sorted_order(ram, ob, MAXREC);
    nr = sorted_order(ref->post, orf, MAXREC);
    np = sorted_order(prev->post, op, MAXREC);
    nn = sorted_order(now->post, on, MAXREC);
    /* an object both draws show stays shown: the sort's culling (behind
       the view, outside its sides) on the moved positions could drop it
       for this one picture (a road marking beside the car blinking out);
       its position is the one the sort turned (it turns every record
       before it culls) */
    for (i = 0; i < nn && nb < MAXREC; i++) {
        int rec = on[i], id = now->obj[rec], in_p = 0, in_b = 0;
        for (j = 0; j < np; j++)
            if (prev->obj[op[j]] == id)
                in_p = 1;
        for (k = 0; k < nb; k++)
            if (ob[k] == rec)
                in_b = 1;
        if (in_p && !in_b) {
            ob[nb++] = rec;
            sort_kept++;
        }
    }
    /* and one neither draw shows stays hidden (it would flash for a
       picture) */
    for (k = 0; k < nb;) {
        int id = now->obj[ob[k]], seen = 0;
        for (j = 0; j < np && !seen; j++)
            seen = prev->obj[op[j]] == id;
        for (j = 0; j < nn && !seen; j++)
            seen = now->obj[on[j]] == id;
        if (!seen && wide_pending) {
            /* the wider view: past the game's 45 degrees the game's own
               draws never show it (its |X| at least near |Z|): kept */
            unsigned r = REC_BASE + (unsigned)ob[k] * REC_SIZE;
            int ax = abs((int16_t)rd16(ram, r + 0x12)), az = abs((int16_t)rd16(ram, r + 0x14));
            seen = ax * 8 >= az * 7;
        }
        if (!seen) {
            ob[k] = ob[--nb];
            sort_hidden++;
        } else
            k++;
    }
    if (nb < 1) {
        wr16(ram, 0x023C, 0);
        return;
    }
    if (nb < 2) {
        wr16(ram, 0x023C, (uint16_t)(REC_BASE + ob[0] * REC_SIZE));
        wr16(ram, (uint16_t)(REC_BASE + ob[0] * REC_SIZE), 0);
        return;
    }
    for (i = 0; i < nb; i++) {
        rank[i] = -1;
        for (j = 0; j < nr; j++)
            if (ref->obj[orf[j]] == now->obj[ob[i]]) {
                rank[i] = j;
                break;
            }
        if (rank[i] < 0)
            rank[i] = i ? rank[i - 1] + 1e-3 : -1;   /* after the one before it */
    }
    /* a stable insertion sort by rank (few, and nearly in order) */
    for (i = 1; i < nb; i++) {
        double r = rank[i];
        int o = ob[i];
        j = i - 1;
        while (j >= 0 && rank[j] > r) {
            rank[j + 1] = rank[j];
            ob[j + 1] = ob[j];
            j--;
        }
        rank[j + 1] = r;
        ob[j + 1] = o;
        if (j + 1 != i)
            sort_fixed++;
    }
    wr16(ram, 0x023C, (uint16_t)(REC_BASE + ob[0] * REC_SIZE));
    for (i = 0; i < nb; i++)
        wr16(ram, (uint16_t)(REC_BASE + ob[i] * REC_SIZE), i + 1 < nb ? (uint16_t)(REC_BASE + ob[i + 1] * REC_SIZE) : 0);
}

/* one draw between prev's and now's at t (1 with interp 0: now's own), its
   RAM after the draw kept in keep and its view angles in angles */
/* ---- the wider view (PORT_WIDE=1 / F8): the 3D past the game's window,
   over the bezel, in the redraws only. The game culls objects at 45
   degrees (|X| < |Z|), which its projection (128 * X / Z from the centre)
   puts 128 pixels from the centre: 24 past each side of its 208-pixel
   window, the picture's whole width. The redraw's centre moved and its
   edges widened by these, its pixels past the game's buffer left out
   (only the high-resolution capture draws them) ---- */
#define WIDE_L 24
#define WIDE_R 24
#define WIDE_T 31
#define WIDE_B 8
extern int gsu_plot_clip_w, gsu_plot_clip_h;
extern int port_sort_wide;          /* game/render/sort.c */
static int wide_on = -1;
static long wide_repeats;
static uint32_t wide_m1[VID_HEIGHT], wide_m2[VID_HEIGHT];

int interp_wide(void)
{
    if (wide_on < 0)
        wide_on = getenv("PORT_WIDE") && strcmp(getenv("PORT_WIDE"), "0");
    return wide_on;
}

void interp_wide_set(int on) { interp_wide(); wide_on = on; }

/* a draw with the far cars' list: its sorted list (post's head $023C and
   the records' links) made from that list, in the sandbox (the game's sort
   had the game's list), for the in-betweens' order and showing rules
   (sort_like). Done when the Super FX is idle; till then sort_like leaves
   the in-between as its own sort made it */
static void far_sort(const Draw *cd)
{
    Draw *d = (Draw *)cd;
    uint8_t *ram;
    int c;
    if (!d->far || d->far_sorted || !d->map_ok || !gsu_idle())
        return;
    gsu_sandbox_save(real_state);
    sandboxed = 1;
    gsu_sandbox_load(d->sb, 1);
    ram = snes_sfx_ram();
    memcpy(ram + REC_BASE, d->pre + REC_BASE, (size_t)d->nrec * REC_SIZE);
    wr16(ram, REC_COUNT, (uint16_t)d->nrec);
    for (c = 0; c < 6; c++)
        wr16(ram, VIEW_PARAMS + 2 * c, rd16(d->post, VIEW_PARAMS + 2 * c));
    for (c = 0; c < 3; c++)
        wr16(ram, MATRIX_IN + 2 * c, rd16(d->post, VIEW_PARAMS + 6 + 2 * c));
    gsu_sandbox_run(0x01, 0x8325);
    gsu_sandbox_run(0x01, 0xAEF8);
    memcpy(d->post + 0x023C, ram + 0x023C, 2);
    memcpy(d->post + REC_BASE, ram + REC_BASE, (size_t)d->nrec * REC_SIZE);
    wr16(d->post, REC_COUNT, (uint16_t)d->nrec);
    gsu_sandbox_load(real_state, 0);
    sandboxed = 0;
    d->far_sorted = 1;
}

/* ---- the texts after a draw (the place badge, notices) in the redraws ----
   The game runs the text program ($01:D307) after its draw, its place set
   once a race frame; a redraw runs it too, its place t of the way from
   prev's to now's (the same string in both), so it moves on every video
   frame. Which screen bytes it drew is kept by the redraw's buffer (keep:
   the screen before the texts), for the high-resolution picture's mark */
#define TXT_REG 24
static struct { const uint8_t *keep; uint8_t *pre; int on; uint16_t base; } txt_reg[TXT_REG];
static long txt_drawn;

/* the registry entry for redraw buffer keep, the screen before the paint
   and texts kept there (once a redraw) */
static int reg_of(const Draw *now, const uint8_t *ram, const uint8_t *keep, int fresh)
{
    int r;
    for (r = 0; r < TXT_REG && txt_reg[r].keep && txt_reg[r].keep != keep; r++)
        ;
    if (r == TXT_REG)
        r = 0;
    if (fresh || txt_reg[r].keep != keep) {
        unsigned o;
        txt_reg[r].keep = keep;
        txt_reg[r].on = 0;
        if (!txt_reg[r].pre)
            txt_reg[r].pre = (uint8_t *)malloc(SCREEN_SIZE);
        txt_reg[r].base = now->base;
        for (o = 0; o < SCREEN_SIZE; o++)
            txt_reg[r].pre[o] = ram[(uint16_t)(now->base + o)];
    }
    return r;
}

/* ---- the S-CPU's paint (the place badge, the banner) in the redraws ----
   A draw's paint: the pixels its painted screen has that its own draw
   (done) has not. Between two draws each piece of it (a column cluster of
   a band of lines) is moved t of the way by its move from the one to the
   other (found by matching its pixels); a piece that is not the same
   picture moved takes the nearer draw's. Screen: 8 bits a pixel, a
   character 64 bytes, 16 characters a column (128 lines) */
#define PW 208
#define PH 128
static long paint_slid, paint_held;

static int scr_get(const uint8_t *scr, int x, int y)
{
    unsigned off = (unsigned)(((x >> 3) * 16 + (y >> 3)) * 64 + (y & 7) * 2), n;
    int bit = 7 - (x & 7), v = 0;
    for (n = 0; n < 8; n++)
        v |= ((scr[off + ((n >> 1) << 4) + (n & 1)] >> bit) & 1) << n;
    return v;
}

static void scr_set(uint8_t *scr, int x, int y, int v)
{
    unsigned off = (unsigned)(((x >> 3) * 16 + (y >> 3)) * 64 + (y & 7) * 2), n;
    int bit = 7 - (x & 7);
    for (n = 0; n < 8; n++) {
        uint8_t *b = &scr[off + ((n >> 1) << 4) + (n & 1)];
        *b = (uint8_t)((*b & ~(1 << bit)) | (((v >> n) & 1) << bit));
    }
}

/* draw d's paint into pt (-1 none, else the colour) */
static int paint_of(const Draw *d, int16_t *pt)
{
    int x, y, n = 0;
    for (y = 0; y < PH; y++)
        for (x = 0; x < PW; x++) {
            int a = scr_get(d->painted, x, y), b = scr_get(d->done + d->base, x, y);
            pt[y * PW + x] = (int16_t)(a != b ? a : -1);
            n += a != b;
        }
    return n;
}

static void paint_redraw(const Draw *prev, const Draw *now, double t, uint8_t *ram, const uint8_t *keep)
{
    static int16_t P[PW * PH], N[PW * PH];
    int np = 0, nn = 0, y0, y1, x, y, r;
    uint8_t *scr = ram + now->base;
    int pok = prev && prev != now && prev->painted_ok, nok = now->painted_ok;
    if (!nok && !pok)
        return;
    if (nok)
        nn = paint_of(now, N);
    if (pok)
        np = paint_of(prev, P);
    if (!nn && !np)
        return;
    r = reg_of(now, ram, keep, 0);
    txt_reg[r].on = 1;
    if (!pok || !nok || t >= 1.0) {
        /* one draw's paint as it stands */
        const int16_t *src = nok && (t >= 0.5 || !pok) ? N : P;
        for (y = 0; y < PH; y++)
            for (x = 0; x < PW; x++)
                if (src[y * PW + x] >= 0)
                    scr_set(scr, x, y, src[y * PW + x]);
        return;
    }
    /* bands of lines with paint in either, then clusters of columns */
    for (y0 = 0; y0 < PH; y0 = y1) {
        int any = 0, x0, x1;
        static uint8_t col[PW];
        for (; y0 < PH; y0++) {
            for (x = 0; x < PW && !any; x++)
                any = P[y0 * PW + x] >= 0 || N[y0 * PW + x] >= 0;
            if (any)
                break;
        }
        if (y0 >= PH)
            break;
        for (y1 = y0; y1 < PH; y1++) {
            int has = 0;
            for (x = 0; x < PW && !has; x++)
                has = P[y1 * PW + x] >= 0 || N[y1 * PW + x] >= 0;
            if (!has)
                break;
        }
        memset(col, 0, sizeof col);
        for (y = y0; y < y1; y++)
            for (x = 0; x < PW; x++)
                if (P[y * PW + x] >= 0 || N[y * PW + x] >= 0)
                    col[x] = 1;
        for (x0 = 0; x0 < PW; x0 = x1) {
            int cp = 0, cn = 0, best = 0, bdx = 0, dx, gap;
            for (; x0 < PW && !col[x0]; x0++)
                ;
            if (x0 >= PW)
                break;
            for (x1 = x0, gap = 0; x1 < PW && gap < 8; x1++)
                gap = col[x1] ? 0 : gap + 1;
            x1 -= gap;
            for (y = y0; y < y1; y++)
                for (x = x0; x < x1; x++) {
                    cp += P[y * PW + x] >= 0;
                    cn += N[y * PW + x] >= 0;
                }
            /* the best move other than none (what stands still matches at 0) */
            if (cp && cn)
                for (dx = -64; dx <= 64; dx++) {
                    int m = 0;
                    if (!dx)
                        continue;
                    for (y = y0; y < y1; y++)
                        for (x = x0; x < x1; x++) {
                            int v = P[y * PW + x], xx = x + dx;
                            if (v >= 0 && N[y * PW + x] != v && xx >= 0 && xx < PW && N[y * PW + xx] == v)
                                m++;
                        }
                    if (m > best) {
                        best = m;
                        bdx = dx;
                    }
                }
            if (cp && cn) {
                /* the moving pixels' edge's move: a piece that changes its
                   picture a little as it flies keeps its edge's pace (the
                   edge not on the screen's border) */
                int pl = PW, pr = -1, nl = PW, nr = -1, edx = 0, m = 0;
                for (y = y0; y < y1; y++)
                    for (x = x0; x < x1; x++) {
                        int v = P[y * PW + x], w = N[y * PW + x];
                        if (v >= 0 && w != v) { if (x < pl) pl = x; if (x > pr) pr = x; }
                        if (w >= 0 && w != v) { if (x < nl) nl = x; if (x > nr) nr = x; }
                    }
                if (pr >= 0 && nr >= 0) {
                    if (pl > 2 && nl > 2)
                        edx = nl - pl;
                    else if (pr < PW - 3 && nr < PW - 3)
                        edx = nr - pr;
                }
                if (edx && edx != bdx && abs(edx) <= 64) {
                    for (y = y0; y < y1; y++)
                        for (x = x0; x < x1; x++) {
                            int v = P[y * PW + x], xx = x + edx;
                            if (v >= 0 && N[y * PW + x] != v && xx >= 0 && xx < PW && N[y * PW + xx] == v)
                                m++;
                        }
                    if (m * 10 >= best * 6) {
                        bdx = edx;
                        best = m > best ? m : best;
                    }
                }
            }
            {
                int mp = 0, mn = 0, moving;
                for (y = y0; y < y1; y++)
                    for (x = x0; x < x1; x++) {
                        int v = P[y * PW + x], w = N[y * PW + x];
                        mp += v >= 0 && w != v;
                        mn += w >= 0 && w != v;
                    }
                /* (a large piece: the badge, the banner; small ones are
                   digits changing where they stand) */
                moving = bdx && mp >= 250 && mn >= 250 && best * 10 >= (mp < mn ? mp : mn) * 5;
                for (y = y0; y < y1; y++)
                    for (x = x0; x < x1; x++) {
                        int v = P[y * PW + x], w = N[y * PW + x];
                        if (moving) {
                            /* standing still: a whole 8 x 8 cell the same in
                               both (the timer beside the badge: a moving
                               picture's pixels that only happen to match do
                               not stand still); else the new picture brought
                               back along the move (what comes into view slides
                               in with it); the old one's leftovers dropped */
                            int cx = x & ~7, cy = y & ~7, still = 1, ax, ay;
                            for (ay = cy; ay < cy + 8 && ay < PH && still; ay++)
                                for (ax = cx; ax < cx + 8 && ax < PW && still; ax++)
                                    still = P[ay * PW + ax] == N[ay * PW + ax];
                            if (still) {
                                if (w >= 0)
                                    scr_set(scr, x, y, w);
                            } else if (w >= 0) {
                                int xx = x - (int)lround(bdx * (1.0 - t));
                                if (xx >= 0 && xx < PW)
                                    scr_set(scr, xx, y, w);
                            }
                            (void)v;
                        } else if (w >= 0 && w == v) {
                            scr_set(scr, x, y, w);      /* standing still */
                        } else {
                            const int16_t *src = t >= 0.5 ? N : P;
                            if (src[y * PW + x] >= 0)
                                scr_set(scr, x, y, src[y * PW + x]);
                        }
                    }
                if (moving)
                    paint_slid++;
                else if (mp || mn)
                    paint_held++;
            }
        }
    }
}

static void txt_redraw(const Draw *prev, const Draw *now, double t, uint8_t *ram, const uint8_t *keep)
{
    const Draw *tx = (now->txt_known || now->ntxt) ? now : prev;
    int k, r;
    r = reg_of(now, ram, keep, 0);
    if (!tx || !tx->ntxt)
        return;
    txt_reg[r].on = 1;
    for (k = 0; k < tx->ntxt; k++) {
        uint16_t x = tx->txt[k][2], y = tx->txt[k][3];
        int j;
        if (tx == now && prev != now)
            for (j = 0; j < prev->ntxt; j++)
                if (prev->txt[j][0] == now->txt[k][0] && prev->txt[j][1] == now->txt[k][1]) {
                    int dx = (int16_t)(now->txt[k][2] - prev->txt[j][2]), dy = (int16_t)(now->txt[k][3] - prev->txt[j][3]);
                    if (abs(dx) < 96 && abs(dy) < 96) {
                        x = (uint16_t)(prev->txt[j][2] + (int)lround(dx * t));
                        y = (uint16_t)(prev->txt[j][3] + (int)lround(dy * t));
                    }
                    break;
                }
        wr16(ram, 0x2732, tx->txt[k][0]);
        wr16(ram, 0x2734, tx->txt[k][1]);
        wr16(ram, 0x2738, x);
        wr16(ram, 0x273A, y);
        gsu_sandbox_run(0x01, 0xD307);
        txt_drawn++;
    }
}

static void draw_between(const Draw *prev, const Draw *now, double t, int interp, uint8_t *keep, uint16_t *angles, HrList *hr)
{
    uint8_t *ram;
    FxCanvas *capture = canvas_capture;
    int n = now->nrec, i, j, c;
    uint16_t vp[6], vn[6], vi[6];
    static const unsigned field[3] = { 0x12, 0x10, 0x14 };   /* X, Y, Z */

    far_sort(prev);
    far_sort(now);
    gsu_sandbox_save(real_state);
    sandboxed = 1;
    gsu_sandbox_load(now->sb, 1);
    ram = snes_sfx_ram();
    memset(angle_frac, 0, sizeof angle_frac);
    /* the records as the sort got them, then moved */
    memcpy(ram + REC_BASE, now->pre + REC_BASE, (size_t)n * REC_SIZE);
    wr16(ram, REC_COUNT, (uint16_t)n);
    if (hr && lod_near() && now->map_ok)
        near_apply(now, ram);
    for (c = 0; c < 6; c++) {
        vp[c] = rd16(prev->post, VIEW_PARAMS + 2 * c);
        vn[c] = rd16(now->post, VIEW_PARAMS + 2 * c);
        vi[c] = interp ? lerp16(vp[c], vn[c], t) : vn[c];
    }
    if (interp) {
        for (i = 0; i < n && i < MAXREC; i++) {
            unsigned r = REC_BASE + (unsigned)i * REC_SIZE, rp = 0;
            int abs_pos = now->pre[r + 7] & 0x80, jump = 0;
            int wn[3], wp[3], wi[3];
            for (j = 0; j < prev->nrec && j < MAXREC; j++)
                if (prev->obj[j] == now->obj[i] && rd16(prev->pre, REC_BASE + (unsigned)j * REC_SIZE + 8) == rd16(now->pre, r + 8))
                    break;
            if (j < prev->nrec && j < MAXREC && (prev->pre[REC_BASE + (unsigned)j * REC_SIZE + 7] & 0x80) == abs_pos) {
                rp = REC_BASE + (unsigned)j * REC_SIZE;
                int moved = 0;
                for (c = 0; c < 3; c++) {
                    wn[c] = s16(rd16(now->pre, r + field[c]) + (abs_pos ? 0 : vn[c]));
                    wp[c] = s16(rd16(prev->pre, rp + field[c]) + (abs_pos ? 0 : vp[c]));
                    if (abs(s16(wn[c] - wp[c])) > JUMP)
                        jump = 1;
                    if (abs(s16(wn[c] - wp[c])) > 2)
                        moved = 1;
                }
                /* an object that moved but has no physics record (+$22: the
                   cars, their wheels) is not moving: the game put it
                   somewhere new (a finish line's square, a lane marking
                   passed by, used again further on); slid between its two
                   places it would cross the road. It is where the nearer
                   draw has it */
                if (moved && !jump && slide_scenery_off()) {
                    const uint8_t *w = snes_wram();
                    uint16_t ob = now->obj[i];
                    uint32_t hnd = (uint32_t)w[(uint16_t)(0x2000 + ob + 0x1C)] << 16 | rd16(w, (uint16_t)(0x2000 + ob + 0x1A));
                    /* followers (ObjectAttachFollow, ObjectLampOnParent: a car's
                       shadow, lamps) move with their car: moving */
                    if (!rd16(w, (uint16_t)(0x2000 + ob + 0x22)) && hnd != 0x08DDB4 && hnd != 0x08E653) {
                        jump = 1;
                        recs_moved_scenery++;
                        if (getenv("PORT_HIRES_DEBUG")) {
                            static uint32_t hk[64]; static long hn[64], hd[64]; static int nh, reg2;
                            uint32_t h = (uint32_t)w[(uint16_t)(0x2000 + ob + 0x1C)] << 16 | rd16(w, (uint16_t)(0x2000 + ob + 0x1A));
                            long dist = labs((long)s16(wn[0] - wp[0])) + labs((long)s16(wn[1] - wp[1])) + labs((long)s16(wn[2] - wp[2]));
                            int q;
                            for (q = 0; q < nh && hk[q] != h; q++)
                                ;
                            if (q == nh && nh < 64)
                                hk[nh++] = h;
                            if (q < 64) {
                                hn[q]++;
                                hd[q] += dist;
                            }
                            if (!reg2 && recs_moved_scenery > 5000) {
                                reg2 = 1;
                                for (q = 0; q < nh; q++)
                                    fprintf(stderr, "moved without physics: handler $%06X %ld times, mean move %ld\n", hk[q], hn[q], hd[q] / hn[q]);
                            }
                        }
                        if (t < 0.5) {
                            /* the earlier draw's place, relative to the moved view */
                            recs_kept++;
                            if (!abs_pos)
                                for (c = 0; c < 3; c++)
                                    wr16(ram, r + field[c], (uint16_t)(wp[c] - vi[c]));
                            else
                                for (c = 0; c < 3; c++)
                                    wr16(ram, r + field[c], (uint16_t)wp[c]);
                            continue;
                        }
                    } else
                        recs_moved_physics++;
                }
            } else
                jump = 1;
            if (jump) {
                /* kept where it is in the world: relative to the moved view */
                recs_kept++;
                if (!abs_pos)
                    for (c = 0; c < 3; c++)
                        wr16(ram, r + field[c], (uint16_t)(rd16(now->pre, r + field[c]) + vn[c] - vi[c]));
                continue;
            }
            for (c = 0; c < 3; c++) {
                int d = s16(wn[c] - wp[c]);
                wi[c] = s16(wp[c] + (int)((double)d * t + (d >= 0 ? 0.5 : -0.5)));
                wr16(ram, r + field[c], (uint16_t)(wi[c] - (abs_pos ? 0 : vi[c])));
            }
            if (!abs_pos) {
                /* the depth key: twice |Y| plus what the build added */
                int yn = s16(rd16(now->pre, r + 0x10)), yi = s16(rd16(ram, r + 0x10));
                uint16_t base = (uint16_t)(rd16(now->pre, r + 2) - (abs(yn) << 1));
                wr16(ram, r + 2, (uint16_t)(base + (abs(yi) << 1)));
            }
            for (c = 0; c < 3; c++) {
                /* in 8.8: the byte, and its fraction for the matrix */
                int d = (int8_t)(uint8_t)(now->pre[r + 4 + c] - prev->pre[rp + 4 + c]);
                uint16_t v = (uint16_t)((prev->pre[rp + 4 + c] << 8) + (int)(d * 256.0 * t + (d >= 0 ? 0.5 : -0.5)));
                ram[r + 4 + c] = (uint8_t)(v >> 8);
                angle_frac[i][c] = (uint8_t)v;
            }
            recs_lerped++;
        }
    }
    for (c = 0; c < 6; c++)
        wr16(ram, VIEW_PARAMS + 2 * c, vi[c]);
    for (c = 0; c < 3; c++) {
        wr16(ram, MATRIX_IN + 2 * c, vi[3 + c]);
        angles[c] = vi[3 + c];
    }
    canvas_capture = files ? &out : NULL;   /* the bare canvas: the files' checks only */
    if (files)
        memset(&out, 0, sizeof out);
    interp_active = interp;
    {
        /* the wider view: a race's window (centre 104,64, edges 0-207,
           0-128) widened */
        int wide = hr && interp_wide() && la_enabled() && !rd16(ram, 0x38) && rd16(ram, 0x3A) == 207 && !rd16(ram, 0x3C) &&
                   rd16(ram, 0x3E) == 128 && rd16(ram, 0x34) == 104 && rd16(ram, 0x36) == 64;
        if (wide) {
            wr16(ram, 0x34, 104 + WIDE_L);
            wr16(ram, 0x36, 64 + WIDE_T);
            wr16(ram, 0x3A, 207 + WIDE_L + WIDE_R);
            wr16(ram, 0x3E, 128 + WIDE_T + WIDE_B);
            gsu_plot_clip_w = 208;
            gsu_plot_clip_h = 128;
        }
        wide_pending = wide;
    }
    gsu_sandbox_run(0x01, 0x8325);
    port_sort_wide = wide_pending;
    gsu_sandbox_run(0x01, 0xAEF8);
    port_sort_wide = 0;
    if (interp && prev != now && !getenv("PORT_SORT_FREE"))
        sort_like(t < 0.5 ? prev : now, prev, now, ram);
    else if (now->far && !getenv("PORT_SORT_FREE"))
        sort_like(now, now, now, ram);  /* a far cars' draw as it stands: its own list's order and showing (far_sort), as its in-betweens */
    if (interp && getenv("PORT_SORTCHECK"))
        sort_check(prev, now, ram);
    hires_capture(hr);
    hires_capture_begin();
    if (hr && wide_pending) {
        hr->wl = WIDE_L;
        hr->wt = WIDE_T;
        hr->wr = WIDE_R;
        hr->wb = WIDE_B;
    }
    port_lod_near = hr && lod_near();
    gsu_sandbox_run(0x01, 0xAB13);
    port_lod_near = 0;
    hires_capture_end();
    gsu_plot_clip_w = gsu_plot_clip_h = 0;
    reg_of(now, ram, keep, 1);    /* (the screen before paint and texts) */
    paint_redraw(prev, now, interp ? t : 1.0, ram, keep);
    txt_redraw(prev, now, interp ? t : 1.0, ram, keep);
    interp_active = 0;
    canvas_capture = capture;
    memcpy(keep, ram, RAM_SIZE);
    gsu_sandbox_load(real_state, 0);
    sandboxed = 0;
}

/* whether the view jumped from prev to now (a cut: shown as it is) */
static int cut(const Draw *prev, const Draw *now)
{
    int c;
    for (c = 0; c < 3; c++)
        if (abs(s16(rd16(now->post, VIEW_PARAMS + 2 * c) - rd16(prev->post, VIEW_PARAMS + 2 * c))) > JUMP ||
            abs(s16(rd16(now->post, VIEW_PARAMS + 6 + 2 * c) - rd16(prev->post, VIEW_PARAMS + 6 + 2 * c))) > ANGLE_JUMP)
            return 1;
    return 0;
}

static const Draw *before(const Draw *d)
{
    int k;
    for (k = 0; k < SLOTS; k++)
        if (dr[k]->valid && dr[k]->seq == d->seq - 1)
            return dr[k];
    return NULL;
}

/* whether picture f shows d's screen whole, as drawn: its bytes in VRAM
   all d's, and seen (inverting them changes the picture) */
static int whole(const VidFrame *f, const Maps *m, const Draw *d)
{
    memcpy(pic, f, sizeof *pic);
    if (!patch(pic, m, overlay(m, d, d->done), d, 0) || memcmp(pic->vram, f->vram, sizeof pic->vram))
        return 0;
    patch(pic, m, overlay(m, d, d->done), d, 1);
    vid_draw_frame(pic, rgb_live);
    vid_draw_frame(f, rgb_real);
    return memcmp(rgb_live, rgb_real, VID_WIDTH * VID_HEIGHT * 4) != 0;
}

/* d's screen standing whole now: the picture and its maps kept, its
   in-between pictures made from them (later uploads, the next screen's
   thirds and its switch part way down a picture, leave them alone) */
static void settle(Draw *d, uint32_t now)
{
    memcpy(d->spic, vid_frame(), sizeof *d->spic);
    memcpy(d->ssrc, vram_src, sizeof vram_src);
    memcpy(d->swhen, vram_when, sizeof vram_when);
    memcpy(d->supl, upl, sizeof upl);
    memcpy(d->supl_when, upl_when, sizeof upl_when);
    memcpy(d->sbd, bd, sizeof bd);
    d->sm.src = d->ssrc;
    d->sm.when = d->swhen;
    d->sm.upl = d->supl;
    d->sm.upl_when = d->supl_when;
    d->sm.bd = d->sbd;
    d->settled = 1;
    d->settle_at = now;
}

/* ---- live ---- */

/* the newest draw's in-between screens made ahead, as soon as the Super FX
   is idle after it (its programs' C keeps state of its own between steps:
   none can run beside one of the game's): nb - 1 of them, nb the time
   between the last two draws' making */
static void live_prepare(void)
{
    static uint32_t last_prep;
    if (in_race <= 0)
        return;                 /* races only: a menu's 3D window is laid out otherwise */
    Draw *d = dr[newest];
    const Draw *p = before(d);
    uint32_t now = snes_frame();
    int k, n;
    if (last_prep) {
        shown_len = now - last_prep;
        if (shown_len < 2)
            shown_len = 2;
        if (shown_len > 12)
            shown_len = 12;
    }
    last_prep = now;
    n = port_steady_ticks ? port_steady_ticks : (int)shown_len;
    d->nb = 0;
    d->prep_at = now;
    d->prepared = 1;
    if (!p || !p->map_ok || !d->map_ok || cut(p, d))
        return;
    for (k = 1; k < n; k++)
        draw_between(p, d, (double)k / n, 1, d->between[k], d->between_angles[k], NULL);
    d->nb = n;
}

/* a picture on settled draw b's snapshot, its screen the one at `at` in
   ram (a later draw's in-between, laid where b's screen is: the same
   layout) and the backdrop at angles a */
/* ---- the high-resolution picture (PORT_HIRES=S: S x, PORT_HIRES_AA=A:
   A x A samples a pixel) ----
   The race's 3D drawn again from its shapes (hires.h, hirender.c) into the
   picture made S times larger: the picture composed as ever (p0), again
   with the 3D screen clear (the sky behind it) and again with it all one
   colour (where it shows: the pixels that differ from the sky's, not
   under the HUD's sprites or the bezel). There the 3D drawn at S x lies
   over the sky, by its coverage; everywhere else p0, each pixel S x S. */
static int hires_scale, hires_aa = 2;
/* the redraws (the sandbox's: in-betweens, and the frames that would show
   a draw as the game made it) with every object's near shape, whatever its
   depth (frame.c: no far models, no flat sprites for far cars); with the
   high-resolution picture only, and an experiment (PORT_HIRES_NEAR=1): no
   visible change from the shapes, and far cars as their 3D selves lack
   their wheels (separate objects the game only makes near) */
static int lod_near_on = -1;
static int lod_near(void)
{
    if (lod_near_on < 0)
        lod_near_on = getenv("PORT_HIRES_NEAR") && strcmp(getenv("PORT_HIRES_NEAR"), "0");
    return lod_near_on && hires_scale;
}
static int hr_shown;                 /* the last picture made has a high-resolution one */
static int hr_last_x0, hr_last_y0;     /* the game's window in the last picture */
static uint32_t *hr_out, *hr_3d, *hr_sky, *hr_mask, *hr_mask2, *hr_hud;
static VidFrame *pic_base;
static uint8_t *hr_ram;
static long hr_made;
static double hr_ms_sum, hr_ms_max;
uint32_t port_rom_size(void);
const uint8_t *port_rom(void);

void interp_hires_set(int scale)
{
    hires_scale = scale < 0 ? 0 : scale > 6 ? 6 : scale;
    /* 2 x 2 samples a pixel up to 3x; from 4x one (the pixels already
       fine: the same time a picture as 3x's with them) */
    hires_aa = hires_scale >= 4 ? 1 : 2;
    if (getenv("PORT_HIRES_AA"))
        hires_aa = atoi(getenv("PORT_HIRES_AA")) < 1 ? 1 : atoi(getenv("PORT_HIRES_AA")) > 4 ? 4 : atoi(getenv("PORT_HIRES_AA"));
}
int interp_hires(void) { return hires_scale; }

/* the last picture's high-resolution one (w x h), or NULL */
const uint32_t *interp_picture_hr(int *w, int *h)
{
    if (!live || !live_pic || !hr_shown)
        return NULL;
    *w = VID_WIDTH * hires_scale;
    *h = VID_HEIGHT * hires_scale;
    return hr_out;
}

static void hr_report(void)
{
    if (hr_made)
        fprintf(stderr, "hires: %ld pictures at %dx (%dx%d samples), %.2f ms each on average, %.2f at most; "
                "%ld far objects drawn with their near shape; %d objects' 3D selves learnt (%ld times), %ld sprites drawn as them\n", hr_made,
                hires_scale, hires_aa, hires_aa, hr_ms_sum / hr_made, hr_ms_max, port_lod_forced, near_n, near_learnt, near_used);
}

/* the rows the 3D window was found in last (the extra pictures are drawn
   there only, a few rows around), all of them first and when it reaches
   their edge */
static int hr_row0 = 0, hr_row1 = VID_HEIGHT - 1;

/* b's picture (pic_base, before its screen was laid in) with the screen
   region of b's RAM all v, drawn into px (rows hr_row0-hr_row1). mark 0:
   the sky alone (BG2 and the backdrop); 1: all but the sky, colour index
   v made colour markc (BGR555): where the 3D screen shows, that colour as
   the screen shows it (its brightness, its colour maths: a tunnel's
   tint) */
#define HR_MARK1 (24u << 10 | 24u << 5 | 24u)
#define HR_MARK2 (8u << 10 | 8u << 5 | 8u)
static int sky_built;   /* the sky pass laid the panorama above the window (backdrop_above) */
static const uint8_t *pf_txt_pre, *pf_txt_ram;     /* the shown redraw's texts (picture_on): its screen before them, and after */
static uint16_t pf_txt_at;
static int sky_from;    /* the wider view's sky pass: lines above this one drawn as this one (0: as they are) */
static int hud_x0, hud_y0, hud_x1, hud_y1;     /* mark 2: the window (a HUD sprite is over it) */

static void picture_fill(const Draw *b, const uint16_t *a, const uint16_t *own, uint8_t v, int mark, uint16_t markc, uint32_t *px)
{
    unsigned o;
    memcpy(hr_ram, b->done, RAM_SIZE);
    for (o = 0; o < SCREEN_SIZE; o++)
        hr_ram[(uint16_t)(b->base + o)] = v;
    memcpy(pic, pic_base, sizeof *pic);
    {
        int l;
        for (l = 0; l <= VID_HEIGHT; l++) {
            if (mark == 2) {
                /* the HUD's sprites alone (those reaching into the
                   window's inside: not the bezel's), on a backdrop of
                   markc */
                pic->palette[l][0] = markc;
                pic->line[l].tm &= 0x10;
                pic->line[l].ts = 0;
                pic->line[l].cgadsub = 0;
                pic->line[l].inidisp = 0x0F;
            } else if (mark) {
                pic->palette[l][v] = markc;
                pic->line[l].tm &= (uint8_t)~0x02;
                pic->line[l].ts &= (uint8_t)~0x02;
            } else {
                pic->line[l].tm &= 0x02;
                pic->line[l].ts &= 0x02;
            }
        }
    }
    if (mark == 1 && pf_txt_pre) {
        /* the redraw's texts stay the picture's (not the 3D's mark): its
           pixels the text program changed, by the overlay's rule */
        static const unsigned plane[8] = { 0, 1, 16, 17, 32, 33, 48, 49 };
        unsigned c, r, q;
        for (c = 0; c < SCREEN_SIZE / 64; c++)
            for (r = 0; r < 8; r++) {
                uint8_t m = 0;
                for (q = 0; q < 8; q++) {
                    unsigned off = c * 64 + plane[q] + 2 * r;
                    m |= (uint8_t)(pf_txt_pre[off] ^ pf_txt_ram[(uint16_t)(pf_txt_at + off)]);
                }
                if (!m)
                    continue;
                for (q = 0; q < 8; q++) {
                    unsigned off = c * 64 + plane[q] + 2 * r;
                    uint8_t *h = &hr_ram[(uint16_t)(b->base + off)];
                    *h = (uint8_t)((*h & ~m) | (pf_txt_ram[(uint16_t)(pf_txt_at + off)] & m));
                }
            }
    }
    patch(pic, &b->sm, mark == 1 && pf_txt_pre ? hr_ram : overlay(&b->sm, b, hr_ram), b, 0);
    backdrop(pic, &b->sm, a, own, b->ended);
    if (mark == 2) {
        /* the HUD's sprites (over the window), the rest (the bezel's) off
           the picture */
        int k, sp;
        for (k = 0; k < pic->versions && k < VID_VERSIONS; k++) {
            uint8_t *o = pic->oam[k], hud[128];
            int sx[128], sy[128], sw[128], sh[128];
            for (sp = 0; sp < 128; sp++) {
                uint8_t hi = (uint8_t)(o[0x200 + (sp >> 2)] >> ((sp & 3) * 2));
                unsigned w, h;
                vid_obj_size(pic->line[hud_y0 + 1].obsel, (hi >> 1) & 1, &w, &h);
                sx[sp] = o[sp * 4] | ((hi & 1) << 8);
                sy[sp] = o[sp * 4 + 1];
                if (sx[sp] >= 256)
                    sx[sp] -= 512;
                if (sy[sp] >= 224)
                    sy[sp] -= 256;
                sw[sp] = (int)w;
                sh[sp] = (int)h;
                /* over the window at all; not the bezel's own (its side
                   columns, corner brackets included: 16x16 tiles $040-$047,
                   drawn over the HUD where they cross: a light panel's end) */
                hud[sp] = !(sx[sp] + sw[sp] <= hud_x0 || sx[sp] >= hud_x1 || sy[sp] + sh[sp] <= hud_y0 || sy[sp] >= hud_y1);
                if (((o[sp * 4 + 2] | (o[sp * 4 + 3] & 1) << 8) & 0x1F8) == 0x040)
                    hud[sp] = 0;
            }
            for (sp = 0; sp < 128; sp++)
                if (!hud[sp])
                    o[sp * 4 + 1] = 0xF0;
        }
    }
    if (sky_from > 1) {
        /* the window's first line's way of drawing the sky (its scroll,
           its layers) for the lines above it: the sky's plane goes on up */
        int l;
        for (l = 1; l < sky_from; l++) {
            pic->line[l] = pic->line[sky_from];
            memcpy(pic->palette[l], pic->palette[sky_from], sizeof pic->palette[l]);
        }
        sky_built = backdrop_above(pic, &b->sm, a, own, sky_from);
    }
    vid_draw_lines(pic, px, (unsigned)hr_row0, (unsigned)hr_row1);
}

static void compose_hires(const Draw *b, const uint16_t *a, const uint16_t *own, const uint32_t *p0, const HrList *hr)
{
    enum { CW = 208, CH = 128 };
    int S = hires_scale, A = hires_aa, x, y, x0 = VID_WIDTH, y0 = VID_HEIGHT, x1 = -1, y1 = -1, W = VID_WIDTH * S;
    int rx0, ry0, RW, RH;
    double t0 = now_ms(), t;
    static int reported;
    if (!hr_out) {
        hr_out = (uint32_t *)malloc((size_t)VID_WIDTH * VID_HEIGHT * 36 * 4);
        hr_3d = (uint32_t *)malloc((size_t)VID_WIDTH * VID_HEIGHT * 36 * 4);
        hr_sky = (uint32_t *)malloc(VID_WIDTH * VID_HEIGHT * 4);
        hr_mask = (uint32_t *)malloc(VID_WIDTH * VID_HEIGHT * 4);
        hr_mask2 = (uint32_t *)malloc(VID_WIDTH * VID_HEIGHT * 4);
        hr_hud = (uint32_t *)malloc(VID_WIDTH * VID_HEIGHT * 4);
        hr_ram = (uint8_t *)malloc(RAM_SIZE);
    }
    if (!reported) {
        reported = 1;
        atexit(hr_report);
    }
    if (!(hr->wl | hr->wt | hr->wr | hr->wb))
        picture_fill(b, a, own, 0x00, 0, 0, hr_sky);     /* (the wider view draws its own, below) */
    /* where the 3D shows: where two marks of different greys come out
       different (whatever the screen does to colours there: brightness,
       a tint by colour maths; none in a picture faded to black) */
    picture_fill(b, a, own, 0xFF, 1, HR_MARK1, hr_mask);
    {
        /* the mark shown as it is somewhere (no tint, no fade on the 3D:
           most pictures): one pass, the second mark's picture made up;
           else (a tunnel's tint, a fade) the second mark drawn */
        uint32_t m1 = 0xFF000000u | 198u << 16 | 198u << 8 | 198u, m2 = 0xFF000000u | 66u << 16 | 66u << 8 | 66u;
        int i, plain = 0;
        for (i = hr_row0 * VID_WIDTH; i < (hr_row1 + 1) * VID_WIDTH && !plain; i++)
            plain = hr_mask[i] == m1;
        if (plain) {
            for (i = hr_row0 * VID_WIDTH; i < (hr_row1 + 1) * VID_WIDTH; i++)
                hr_mask2[i] = hr_mask[i] == m1 ? m2 : hr_mask[i];
        } else
            picture_fill(b, a, own, 0xFF, 1, HR_MARK2, hr_mask2);
    }
    for (y = hr_row0; y <= hr_row1; y++)
        for (x = 0; x < VID_WIDTH; x++)
            if (hr_mask[y * VID_WIDTH + x] != hr_mask2[y * VID_WIDTH + x]) {
                if (x < x0) x0 = x;
                if (x > x1) x1 = x;
                if (y < y0) y0 = y;
                if (y > y1) y1 = y;
            }
    if (x1 < 0 || x1 - x0 >= CW || y1 - y0 >= CH) {
        hr_row0 = 0;                /* none, or not a race screen's layout: all rows next time */
        hr_row1 = VID_HEIGHT - 1;
        return;
    }
    {
        /* the window's rows for next time; all of them when it reached
           the edge of those drawn (it may reach further) */
        int r0 = y0 - 4 < 0 ? 0 : y0 - 4, r1 = y1 + 4 >= VID_HEIGHT ? VID_HEIGHT - 1 : y1 + 4;
        int edge = (y0 == hr_row0 && hr_row0 > 0) || (y1 == hr_row1 && hr_row1 < VID_HEIGHT - 1);
        hr_row0 = edge ? 0 : r0;
        hr_row1 = edge ? VID_HEIGHT - 1 : r1;
        if (edge)
            return;                 /* drawn again whole next time */
    }
    hr_last_x0 = x0;
    hr_last_y0 = y0;
    /* the region drawn in 3D: the game's window, or wider (over the
       bezel, to the picture's edges) */
    rx0 = x0 - hr->wl;
    ry0 = y0 - hr->wt;
    RW = CW + hr->wl + hr->wr;
    RH = CH + hr->wt + hr->wb;
    if (rx0 < 0 || ry0 < 0 || rx0 + RW > VID_WIDTH || ry0 + RH > VID_HEIGHT) {
        rx0 = x0;               /* (not where a race's window is: the game's) */
        ry0 = y0;
        RW = CW;
        RH = CH;
    }
    if (RW != CW) {
        /* the wider view: each row's way with colours (a tunnel's tint) as
           the window's 3D shows it, for the 3D past the window on that row
           (rows above and below it: the window's first and last) */
        int row;
        for (row = 0; row < VID_HEIGHT; row++) {
            int r = row < y0 ? y0 : row > y0 + CH - 1 ? y0 + CH - 1 : row, k;
            wide_m1[row] = 0xFFC6C6C6u;
            wide_m2[row] = 0xFF424242u;
            if (r < hr_row0 || r > hr_row1)
                continue;
            for (k = x0; k < x0 + CW; k++)
                if (hr_mask[r * VID_WIDTH + k] != hr_mask2[r * VID_WIDTH + k]) {
                    wide_m1[row] = hr_mask[r * VID_WIDTH + k];
                    wide_m2[row] = hr_mask2[r * VID_WIDTH + k];
                    break;
                }
        }
    }
    if (RW != CW) {
        /* the wider view: the sky wherever the 3D may reach; above the
           window the sky's plane drawn on up (as its first line) */
        int r0 = hr_row0, r1 = hr_row1;
        hr_row0 = ry0;
        hr_row1 = ry0 + RH - 1;
        sky_from = y0 + 1;
        picture_fill(b, a, own, 0x00, 0, 0, hr_sky);
        sky_from = 0;
        hud_x0 = x0;
        hud_x1 = x0 + CW;
        hud_y0 = y0;
        hud_y1 = y0 + CH;
        picture_fill(b, a, own, 0x00, 2, HR_MARK1, hr_hud);     /* the HUD's sprites (rows ry0 on, as set above) */
        hr_row0 = r0;
        hr_row1 = r1;
        if (ry0 < y0 && !sky_built) {
            /* the sky's plane past its sky band holds other tiles: going up
               each column, once it is the sky's colour (the window's top
               row's commonest) it stays that */
            /* the sky's colour: a line of the plane all one colour (the
               band's top) is the sky; kept for pictures whose band runs
               into other tiles first (the road dipping: the window's top
               the ground) */
            static uint32_t sky_seen;
            uint32_t sky = sky_seen ? sky_seen : hr_sky[y0 * VID_WIDTH + x0 + CW / 2];
            /* and a colour the sky never shows in the window is another
               tile's: the one below it goes on up instead */
            static uint32_t seen[512];
            memset(seen, 0, sizeof seen);
            for (y = y0; y < y0 + CH; y++)
                for (x = x0; x < x0 + CW; x++) {
                    uint32_t c = hr_sky[y * VID_WIDTH + x] | 0xFF000000u, h = (c * 2654435761u) >> 23;
                    while (seen[h] && seen[h] != c)
                        h = (h + 1) & 511;
                    seen[h] = c;
                }
            {
                /* the sky layer is a band (the sky, clouds, hills) with the
                   plain sky at its top; past that its other tiles. Going
                   up from the window: each line as the layer draws it,
                   until a line all the sky's colour or one with a colour
                   the window's sky never shows; the plain sky from there
                   (48 lines at most) */
                /* (a line all one colour is the sky only above the band's
                   detail, its hills; below them it is the ground's fill,
                   drawn on as the layer has it: the road dipping shows the
                   ground at the window's top) */
                int stop = 0, detail = 0;
                {
                    const uint32_t *top = hr_sky + y0 * VID_WIDTH;
                    int uniform = 1;
                    for (x = rx0; x < rx0 + RW && uniform; x++)
                        uniform = top[x] == top[rx0];
                    if (uniform && sky_seen && top[rx0] == sky_seen)
                        stop = 1;       /* the window's top already the plain sky */
                    detail = !uniform;
                }
                for (y = y0 - 1; y >= ry0; y--) {
                    uint32_t *row = hr_sky + y * VID_WIDTH;
                    if (!stop) {
                        int all_one = 1, foreign = 0;
                        for (x = rx0; x < rx0 + RW && !foreign; x++) {
                            uint32_t c = row[x] | 0xFF000000u, hh = (c * 2654435761u) >> 23;
                            all_one &= row[x] == row[rx0];
                            while (seen[hh] && seen[hh] != c)
                                hh = (hh + 1) & 511;
                            foreign = !seen[hh];
                        }
                        if (foreign) {
                            /* past the band: the line below goes on up when
                               it is one colour (the sky's gradient's top),
                               else the sky */
                            const uint32_t *below = row + VID_WIDTH;
                            int one = 1;
                            for (x = rx0; x < rx0 + RW && one; x++)
                                one = below[x] == below[rx0];
                            if (one)
                                sky = below[rx0];
                            stop = 1;
                        }
                        else if (!all_one)
                            detail = 1;
                        else if (detail) {
                            sky = sky_seen = row[rx0];
                            stop = 1;
                        }
                    }
                    if (stop)
                        for (x = rx0; x < rx0 + RW; x++)
                            row[x] = sky;
                }
            }
        }
    }
    {
        int line = y0 + CH / 2 + 1;
        if (line > VID_HEIGHT)
            line = VID_HEIGHT;
        /* (at full brightness: the screen's own way with colours is laid
           on below, pixel by pixel) */
        hires_render(hr, port_rom(), port_rom_size(), pic_base->palette[line], 15, S, A, RW, RH, hr_3d);
    }
    for (y = 0; y < VID_HEIGHT * S; y++) {
        int sy = y / S, k;
        uint32_t *o = hr_out + (size_t)y * W;
        const uint32_t *src = p0 + sy * VID_WIDTH;
        if (y % S) {
            memcpy(o, o - W, (size_t)W * 4);        /* the row above's again (no 3D rows only) */
            if (sy < ry0 || sy > ry0 + RH - 1)
                continue;
        } else
            for (x = 0; x < VID_WIDTH; x++)
                for (k = 0; k < S; k++)
                    o[x * S + k] = src[x];
        if (sy < ry0 || sy > ry0 + RH - 1)
            continue;
        for (x = rx0 * S; x < (rx0 + RW) * S && x < W; x++) {
            int sx = x / S, i = sy * VID_WIDTH + sx, cx = x - rx0 * S, cy = y - ry0 * S;
            int in_window = sx >= x0 && sx < x0 + CW && sy >= y0 && sy < y0 + CH;
            uint32_t c = p0[i];
            int shows3d = in_window && sy >= hr_row0 && sy <= hr_row1 && hr_mask[i] != hr_mask2[i];
            if (RW != CW && !shows3d) {
                /* the wider view: the bezel's place all 3D; in the window,
                   its rounded corners too (not the HUD: what the window
                   shows besides the 3D) */
                int cxw = sx - x0 < 8 ? sx - x0 : x0 + CW - 1 - sx, cyw = sy - y0 < 8 ? sy - y0 : y0 + CH - 1 - sy;
                shows3d = (!in_window || (cxw < 8 && cyw < 8)) && (hr_hud[i] & 0xFFFFFF) == 0xC6C6C6;
            }
            if (RW != CW && !shows3d && (!in_window || ((sx - x0 < 8 || x0 + CW - 1 - sx < 8) && (sy - y0 < 8 || y0 + CH - 1 - sy < 8))) &&
                (hr_hud[i] & 0xFFFFFF) != 0xC6C6C6) {
                o[x] = hr_hud[i];   /* a HUD sprite's part the bezel covered: as the sprite is */
                continue;
            }
            if (shows3d) {
                uint32_t h = hr_3d[(size_t)cy * RW * S + cx], al = h >> 24, bg = hr_sky[i];
                if (!in_window && RW != CW && ((wide_m1[sy] & 0xFFFFFF) != 0xC6C6C6 || (wide_m2[sy] & 0xFFFFFF) != 0x424242)) {
                    /* a line the game tints in its window (a flash, a
                       tunnel): its sky past the window untinted, the
                       window's edge sky carried out instead */
                    int ex = sx < x0 ? x0 : sx > x0 + CW - 1 ? x0 + CW - 1 : sx, ey = sy < y0 ? y0 : sy > y0 + CH - 1 ? y0 + CH - 1 : sy;
                    bg = hr_sky[ey * VID_WIDTH + ex];
                }
                {
                    /* the screen's colours there, per channel a line through
                       the two marks (grey 24 -> m1, grey 8 -> m2: a fade's
                       scale, a tint's offset) */
                    int own_marks = in_window && hr_mask[i] != hr_mask2[i];
                    uint32_t m1 = own_marks ? hr_mask[i] : RW != CW ? wide_m1[sy] : 0xFFC6C6C6u;
                    uint32_t m2 = own_marks ? hr_mask2[i] : RW != CW ? wide_m2[sy] : 0xFF424242u, o = h & 0xFF000000u;
                    int sh = (m1 & 0xFFFFFF) == 0xC6C6C6 && (m2 & 0xFFFFFF) == 0x424242 ? 24 : 0;   /* as it is */
                    if (sh)
                        o = h;
                    for (; sh < 24; sh += 8) {
                        int c1 = (int)(m1 >> sh & 0xFF), c2 = (int)(m2 >> sh & 0xFF), v = (int)(h >> sh & 0xFF);
                        int out = c2 + (c1 - c2) * (v - 66) / (198 - 66);
                        o |= (uint32_t)(out < 0 ? 0 : out > 255 ? 255 : out) << sh;
                    }
                    h = o;
                }
                if (al == 255)
                    c = h;
                else if (!al)
                    c = bg;
                else
                    c = 0xFF000000u | ((((h >> 16 & 0xFF) * al + (bg >> 16 & 0xFF) * (255 - al)) / 255) << 16) |
                        ((((h >> 8 & 0xFF) * al + (bg >> 8 & 0xFF) * (255 - al)) / 255) << 8) |
                        (((h & 0xFF) * al + (bg & 0xFF) * (255 - al)) / 255);
            }
            o[x] = c;
        }
    }
    hr_shown = 1;
    hr_made++;
    t = now_ms() - t0;
    hr_ms_sum += t;
    if (t > hr_ms_max)
        hr_ms_max = t;
}

/* the HUD (the race's sprites) in step with the picture (PORT_HUD_SMOOTH,
   default on): the look-ahead's pictures are ahead of the game, so their
   HUD is the copies' sprite table of the same video frame (hud_at);
   NULL: the game's sprites as they stand now */
static const uint8_t *hud_a, *hud_b;
static double hud_t;

static int hud_smooth(void)
{
    static int v = -1;
    if (v < 0)
        v = !getenv("PORT_HUD_SMOOTH") || strcmp(getenv("PORT_HUD_SMOOTH"), "0");
    return v;
}

/* the HUD as the game will show it at video frame fr (the copies'; the
   sprite table a frame is shown with is the buffer as the frame before
   ended) */
static const uint8_t *la_hud_at(uint32_t fr);
static long hud_found, hud_missed;
/* sprite sp of table t: on screen, its place and its tile and attributes */
static int hud_spr(const uint8_t *t, int sp, int *x, int *y, uint16_t *ta)
{
    uint8_t hi = (uint8_t)(t[0x200 + (sp >> 2)] >> ((sp & 3) * 2));
    *x = t[sp * 4] | (hi & 1) << 8;
    if (*x >= 256)
        *x -= 512;
    *y = t[sp * 4 + 1];
    *ta = (uint16_t)(t[sp * 4 + 2] | t[sp * 4 + 3] << 8 | (hi & 2) << 15);
    return *y < 224;
}

/* HUD pieces the game moves once a race frame (the place badge after
   the finish, a banner): moved on every video frame instead, from where
   the piece last jumped to where it jumps next (both in the copies'
   tables: the frames around fr), when it jumps with two others the same
   way (a group: not a sprite slot used again for another) */
#define HUD_SPAN 14
static long hud_glided;
static void hud_glide(uint32_t fr, uint8_t *out)
{
    const uint8_t *tab[2 * HUD_SPAN + 1];
    int k, sp;
    for (k = -HUD_SPAN; k <= HUD_SPAN; k++)
        tab[k + HUD_SPAN] = la_hud_at(fr - 1 + (uint32_t)k);
    for (sp = 0; sp < 128; sp++) {
        int x0, y0, x1, y1, xb, yb, back = 0, fwd = 0, dx, dy, same = 0, other;
        uint16_t t0, t1;
        if (!hud_spr(out, sp, &x0, &y0, &t0))
            continue;
        /* the next jump */
        for (k = 1; k <= HUD_SPAN && tab[HUD_SPAN + k]; k++) {
            if (!hud_spr(tab[HUD_SPAN + k], sp, &x1, &y1, &t1) || t1 != t0)
                break;
            if (x1 != x0 || y1 != y0) {
                fwd = k;
                break;
            }
        }
        if (getenv("PORT_HUD_DEBUG") && y0 > 110 && y0 < 160 && fr > 2680 && fr < 2720)
            fprintf(stderr, "CAND f%u s%d t%03X at %d,%d fwd %d -> %d,%d\n", fr, sp, t0 & 0x1FF, x0, y0, fwd, fwd ? x1 : 0, fwd ? y1 : 0);
        if (!fwd)
            continue;
        dx = x1 - x0;
        dy = y1 - y0;
        if (abs(dx) > 64 || abs(dy) > 64)
            continue;
        /* how long it has stood (where it last jumped) */
        for (k = 1; k <= HUD_SPAN && tab[HUD_SPAN - k]; k++) {
            if (!hud_spr(tab[HUD_SPAN - k], sp, &xb, &yb, &t1) || t1 != t0 || xb != x0 || yb != y0)
                break;
            back = k;
        }
        {
            /* the jump after: stepping (it too more than a frame later), or
               the piece moving on every frame from here (not glided) */
            int fwd2 = 0, x2, y2;
            uint16_t t2;
            for (k = fwd + 1; k <= HUD_SPAN && tab[HUD_SPAN + k]; k++) {
                if (!hud_spr(tab[HUD_SPAN + k], sp, &x2, &y2, &t2) || t2 != t0)
                    break;
                if (x2 != x1 || y2 != y1) {
                    fwd2 = k;
                    break;
                }
            }
            if (fwd2 && fwd2 - fwd < 2)
                continue;
            if (back >= HUD_SPAN) {
                /* from rest: a step's time of lead-in (the next step's, or
                   the game's race frame) */
                if (!fwd2)
                    continue;
                back = fwd2 - fwd - fwd;
                if (back < 0)
                    continue;       /* (not yet: it stands until a step before) */
            }
        }
        if (back + fwd < 2)
            continue;               /* (moving every frame already) */
        /* the group: two more sprites making the same jump at the same frame */
        for (other = 0; other < 128 && same < 2; other++) {
            int ox0, oy0, ox1, oy1;
            uint16_t ot0, ot1;
            if (other == sp || !hud_spr(out, other, &ox0, &oy0, &ot0) ||
                !hud_spr(tab[HUD_SPAN + fwd], other, &ox1, &oy1, &ot1) || ot1 != ot0)
                continue;
            if (ox1 - ox0 == dx && oy1 - oy0 == dy)
                same++;
        }
        if (same < 2)
            continue;
        {
            double t = (double)back / (double)(back + fwd);
            int x = (int)lround(x0 + dx * t), y = (int)lround(y0 + dy * t);
            out[sp * 4] = (uint8_t)x;
            out[sp * 4 + 1] = (uint8_t)y;
            out[0x200 + (sp >> 2)] = (uint8_t)((out[0x200 + (sp >> 2)] & ~(1 << ((sp & 3) * 2))) | ((x < 0) << ((sp & 3) * 2)));
            hud_glided++;
            if (getenv("PORT_HUD_DEBUG"))
                fprintf(stderr, "GLIDE f%u s%d tile %03X %d,%d -> %d,%d back %d fwd %d at %d,%d\n", fr, sp, t0 & 0x1FF, x0, y0, x1, y1, back, fwd, x, y);
        }
    }
}

static void hud_at(uint32_t fr)
{
    static uint8_t last[544], glided[544];
    static int have;
    hud_a = hud_b = la_hud_at(fr - 1);
    hud_t = 0;
    if (hud_a) {
        memcpy(glided, hud_a, sizeof glided);
        hud_glide(fr, glided);
        hud_a = hud_b = glided;
        memcpy(last, hud_a, sizeof last);
        have = 1;
        hud_found++;
    } else {
        hud_missed++;
        if (have)
            hud_a = hud_b = last;   /* (the last shown again: the game's own is behind) */
    }
}

static void hud_make(uint8_t *out)
{
    memcpy(out, hud_t < 0.5 ? hud_a : hud_b, 544);
}

static void picture_on(const Draw *b, const uint8_t *ram, uint16_t at, const uint16_t *a, uint32_t *px, const HrList *hr)
{
    uint16_t own[3];
    unsigned o;
    int c;
    memcpy(tmp_ram, b->done, RAM_SIZE);
    for (o = 0; o < SCREEN_SIZE; o++)
        tmp_ram[(uint16_t)(b->base + o)] = ram[(uint16_t)(at + o)];
    for (c = 0; c < 3; c++)
        own[c] = rd16(b->post, VIEW_PARAMS + 6 + 2 * c);
    memcpy(pic, b->spic, sizeof *pic);
    {
        /* the sprites (the HUD's digits and marks) as the game shows them
           now, not as they stood at the snapshot: no time running back */
        const VidFrame *f = vid_frame();
        int v;
        if (hud_a && hud_b && hud_smooth()) {
            uint8_t o[544];
            hud_make(o);
            for (v = 0; v < pic->versions; v++)
                memcpy(pic->oam[v], o, sizeof pic->oam[v]);
        } else
            for (v = 0; v < pic->versions; v++)
                memcpy(pic->oam[v], f->oam[f->versions - 1], sizeof pic->oam[v]);

    }
    if (hr && hires_scale) {
        if (!pic_base)
            pic_base = (VidFrame *)malloc(sizeof *pic_base);
        memcpy(pic_base, pic, sizeof *pic);
    }
    {
        /* a redraw with the paint and texts of its own: those, not the
           game's (the overlay: its own, a race frame at a time) */
        int r, own_paint = 0;
        for (r = 0; r < TXT_REG; r++)
            if (txt_reg[r].keep == ram && txt_reg[r].on && txt_reg[r].base == at)
                own_paint = 1;
        patch(pic, &b->sm, own_paint ? tmp_ram : overlay(&b->sm, b, tmp_ram), b, 0);
    }
    backdrop(pic, &b->sm, a, own, b->ended);
    vid_draw_frame(pic, px);
    hr_shown = 0;
    if (hr && hires_scale && !hr->overflow) {
        int r;
        pf_txt_pre = NULL;
        for (r = 0; r < TXT_REG; r++)
            if (txt_reg[r].keep == ram && txt_reg[r].on && txt_reg[r].base == at) {
                pf_txt_pre = txt_reg[r].pre;
                pf_txt_ram = ram;
                pf_txt_at = at;
            }
        compose_hires(b, a, own, px, hr);
        pf_txt_pre = NULL;
    }
}

/* ---- the look-ahead ----
   With a steady speed (a race frame every n video frames) the game waits
   between race frames. As each of its draws ends, and again whenever the
   controls change before its next race frame begins, the whole process
   is copied (fork): the copy runs on with the controls held as they are
   now, through the next race frame's draw, hands that draw over (pre,
   post, done, the Super FX) in shared memory and ends. The game itself is
   untouched: it runs the next frame on its own when the wait ends, with
   the controls of that moment (the same draw when they have not changed:
   counted, la_exact).

   Shown: in the j-th video frame after draw N ended (j from 0), the
   picture between N and the copy's N + 1 at t = j / n, so N at once and
   N + 1 as the game's own takes over: smooth, and a race frame sooner
   than the in-betweens from N - 1 to N. Without the copy's draw: N. */
#include <signal.h>
#include <stddef.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

#define LA_HUD 40                /* a copy's video frames' HUD kept */
#define LA_SLOTS 3               /* copies' results kept: the running one's, the last finished, one before */
#define LA_DEPTH_MAX 3
#define LA_DRAWS (LA_DEPTH_MAX + 1) /* a copy's draws: one past the depth, so that with steady controls
                                       the next race frame's pair is in long before it is shown */
typedef struct {
    volatile int nready;        /* its draws handed over so far (each whole) */
    long gen;                   /* which copy (1, 2, ...) */
    long base_seq;              /* the game's newest draw as it was copied: the copy's draws follow it */
    uint16_t pad[2];            /* the controls it ran on */
    double started, finished;   /* the copy's start and its last draw's end (now_ms: the system's clock) */
    Draw d[LA_DRAWS];           /* its draws, base_seq + 1 on (sb: shared too) */
    volatile int nhud;          /* its video frames' HUD (the sprite table's buffer, $7E:0375), each with its frame */
    uint32_t hud_frame[LA_HUD];
    uint8_t hud[LA_HUD][544];
} LaSlot;
static LaSlot *la_slot;          /* shared with the copies */
static int la_on = -1;           /* PORT_LOOKAHEAD: 0 off, 1 to 3 race frames ahead (default 3) */
/* la_child (declared above): this process is a copy */
static pid_t la_pid;             /* the running copy, 0 none */
static int la_cur = -1;          /* the slot the running (or last) copy writes */
static long la_gen;
static long la_child_frames;
static uint16_t la_pad_now[2];   /* the controls set last video frame */
static uint16_t la_pad_tick[2];  /* the controls as the game's frame began */
static long la_forks, la_exact, la_inexact, la_changed, la_missing, la_shown, la_stale, la_held, la_failed;
static double la_fork_ms_max, la_ready_ms_sum, la_ready_ms_max, la_wait_sum, la_wait_max;
static long la_ready_n, la_waits;
static long la_last_gen, la_back;    /* the copy the last picture came from; pictures from an older one than that */
static double la_last_time;          /* the last picture's place in the game's time: a draw's number, + the way to the next */
static long la_time_back;            /* pictures earlier in the game's time than the one before */
static long la_repeats;              /* pictures repeated for want of a pair */
static int live_pic_last, hr_shown_last;  /* the last frame's picture was the look-ahead's (rgb_live), and high-res */
static const char *la_path = "";
static void la_time(double t)
{
    if (t < la_last_time - 1e-6) {
        la_time_back++;
        if (getenv("PORT_LA_DEBUG"))
            fprintf(stderr, "back in time at f%u: %.3f after %.3f (%s)\n", snes_frame(), t, la_last_time, la_path);
    }
    la_last_time = t;
}
static void la_used(const LaSlot *sl)
{
    if (!sl)
        return;
    if (sl->gen < la_last_gen)
        la_back++;
    la_last_gen = sl->gen;
}
static int la_waited;            /* this video frame waited for a copy: no drawing ahead in it */

int interp_lookahead_child(void) { return la_child; }

static int la_depth(void)
{
    if (la_on < 0) {
        const char *e = getenv("PORT_LOOKAHEAD");
        la_on = !e ? LA_DEPTH_MAX : atoi(e) < 0 ? 0 : atoi(e) > LA_DEPTH_MAX ? LA_DEPTH_MAX : atoi(e);
    }
    return la_on;
}

static int la_enabled(void)
{
    return la_depth() && live && in_race > 0 && port_steady_ticks > 1 && !timescale && on > 0;
}

/* F4: off, 1 to 3 race frames ahead */
void interp_lookahead_set(int v) { la_depth(); la_on = v < 0 ? 0 : v > LA_DEPTH_MAX ? LA_DEPTH_MAX : v; }
int interp_lookahead(void) { return la_depth(); }
/* how long a video frame waits for the copy's draw (ms): the window a
   little, headless runs until it is in (the same pictures every run) */

static double la_wait_ms = 1e9;
void interp_lookahead_wait(double ms) { la_wait_ms = ms; }

static void la_reap(void);

/* the running copy ended (killed; headless, with no limit to the waits,
   let finish: which copies' draws are in must not hang on the host's
   speed, the pictures the same every run) */
static void la_kill(void)
{
    if (la_pid > 0) {
        if (la_wait_ms >= 1e8 && !la_child) {
            la_reap();
            if (la_pid > 0)
                waitpid(la_pid, NULL, 0);
        } else {
            kill(la_pid, SIGKILL);
            waitpid(la_pid, NULL, 0);
        }
    }
    la_pid = 0;
}

/* the running copy ended? (its numbers counted) */
static void la_reap(void)
{
    int st = 0;
    if (la_pid > 0 && waitpid(la_pid, &st, WNOHANG) == la_pid) {
        LaSlot *s = &la_slot[la_cur];
        if (!s->nready && (!WIFEXITED(st) || WEXITSTATUS(st) != 1))
            la_failed++;        /* not given up for want of a draw: a fault in the copy */
        la_pid = 0;
        if (s->nready) {
            double t = s->finished - s->started;
            la_ready_ms_sum += t;
            if (t > la_ready_ms_max)
                la_ready_ms_max = t;
            la_ready_n++;
        }
    }
}

static void la_fork(void)
{
    double t0;
    pid_t p;
    int k;
    LaSlot *s;
    if (la_child || newest < 0 || !dr[newest]->valid)
        return;
    if (!la_slot) {
        size_t sb = gsu_sandbox_size();
        uint8_t *m = mmap(NULL, sizeof(LaSlot) * LA_SLOTS + sb * LA_SLOTS * LA_DRAWS, PROT_READ | PROT_WRITE,
                          MAP_SHARED | MAP_ANONYMOUS, -1, 0);
        int q;
        if (m == MAP_FAILED) {
            la_on = 0;
            return;
        }
        la_slot = (LaSlot *)m;
        for (k = 0; k < LA_SLOTS; k++)
            for (q = 0; q < LA_DRAWS; q++)
                la_slot[k].d[q].sb = m + sizeof(LaSlot) * LA_SLOTS + sb * (size_t)(k * LA_DRAWS + q);
    }
    la_reap();
    la_kill();
    /* the oldest copy's slot */
    la_cur = 0;
    for (k = 1; k < LA_SLOTS; k++)
        if (la_slot[k].gen < la_slot[la_cur].gen)
            la_cur = k;
    s = &la_slot[la_cur];
    s->nready = 0;
    s->nhud = 0;
    s->gen = ++la_gen;
    s->base_seq = dr[newest]->seq;
    s->pad[0] = la_pad_now[0];
    s->pad[1] = la_pad_now[1];
    fflush(NULL);                /* nothing buffered copied into the copy */
    t0 = now_ms();
    s->started = t0;
    p = fork();
    if (p == 0) {
        la_child = 1;
        la_child_frames = 0;
        return;                 /* the copy runs on from here */
    }
    if (p < 0) {
        la_on = 0;
        return;
    }
    la_pid = p;
    la_forks++;
    if (now_ms() - t0 > la_fork_ms_max)
        la_fork_ms_max = now_ms() - t0;
}

/* the copy: a draw done (interp_gsu), handed over; ended after the
   look-ahead's depth */
/* the text program ($01:D307) starting (done 0) or ending: its string
   and place kept with the draw just made (a copy's: in the shared slot,
   its draw handed over already); the game's own: its RAM after (the
   overlay's base: those pixels are the redraws' to make) */
static void txt_run(uint8_t *ram, int done)
{
    Draw *d;
    if (la_child) {
        if (!la_slot || la_cur < 0 || la_slot[la_cur].nready < 1)
            return;
        d = &la_slot[la_cur].d[la_slot[la_cur].nready - 1];
    } else {
        if (newest < 0 || !dr[newest]->valid)
            return;
        d = dr[newest];
    }
    if (!done) {
        if (d->ntxt < 4) {
            d->txt[d->ntxt][0] = rd16(ram, 0x2732);
            d->txt[d->ntxt][1] = rd16(ram, 0x2734);
            d->txt[d->ntxt][2] = rd16(ram, 0x2738);
            d->txt[d->ntxt][3] = rd16(ram, 0x273A);
            d->ntxt++;
        }
    } else if (!la_child) {
        if (!d->after)
            d->after = (uint8_t *)malloc(RAM_SIZE);
        memcpy(d->after, ram, RAM_SIZE);
        d->after_ok = 1;
    }
}

/* the draw before this one (pv): its screen as the S-CPU left it, now that
   the next draw starts (the screens alternate: pv's still stands); a
   copy's, into its handed-over draw in the shared slot */
static void paint_take(Draw *pv, const uint8_t *ram)
{
    Draw *to = pv;
    unsigned o;
    if (la_child) {
        LaSlot *sl;
        int q;
        if (!la_slot || la_cur < 0)
            return;
        sl = &la_slot[la_cur];
        q = __atomic_load_n(&sl->nready, __ATOMIC_ACQUIRE);
        if (q < 1 || sl->d[q - 1].seq != pv->seq)
            return;
        to = &sl->d[q - 1];
    }
    if (to->painted_ok)
        return;
    for (o = 0; o < SCREEN_SIZE; o++)
        to->painted[o] = ram[(uint16_t)(pv->base + o)];
    __atomic_store_n(&to->painted_ok, 1, __ATOMIC_RELEASE);
}

static void la_child_done(const Draw *d)
{
    LaSlot *s = &la_slot[la_cur];
    int q = s->nready;
    Draw *o;
    if (q >= LA_DRAWS)
        _exit(0);
    o = &s->d[q];
    memcpy(o->pre, d->pre, RAM_SIZE);
    memcpy(o->post, d->post, RAM_SIZE);
    memcpy(o->done, d->done, RAM_SIZE);
    memcpy(o->sb, d->sb, gsu_sandbox_size());
    memcpy(o->obj, d->obj, sizeof d->obj);
    memcpy(&o->hr, &d->hr, offsetof(HrList, ops) + sizeof d->hr.ops[0] * (size_t)d->hr.nops);
    memcpy(o->hr.rows, d->hr.rows, sizeof d->hr.rows[0] * (size_t)d->hr.nrows);
    o->nrec = d->nrec;
    o->map_ok = d->map_ok;
    o->far = d->far;
    o->far_sorted = d->far_sorted;
    o->base = d->base;
    o->ended = d->ended;
    memcpy(o->oam, d->oam, sizeof o->oam);
    if (q > 0)
        s->d[q - 1].txt_known = 1;  /* (its texts, if any, came before this draw) */
    o->ntxt = 0;
    o->txt_known = 0;
    o->after = NULL;
    o->after_ok = 0;
    o->painted_ok = 0;
    o->seq = d->seq;
    o->valid = 1;
    s->finished = now_ms();
    __atomic_store_n(&s->nready, q + 1, __ATOMIC_RELEASE);
    if (q + 1 >= la_on + 1)
        _exit(0);
}

/* the copy's video frames: nothing shown, nothing read; given up if no
   draw comes (the race left) */
void interp_lookahead_child_frame(void)
{
    LaSlot *s = &la_slot[la_cur];
    int k = s->nhud;
    if (k < LA_HUD) {
        s->hud_frame[k] = snes_frame();
        memcpy(s->hud[k], snes_wram() + 0x0375, 544);
        __atomic_store_n(&s->nhud, k + 1, __ATOMIC_RELEASE);
    }
    if (++la_child_frames > 4 * MAXSTEPS)
        _exit(1);
}

/* the HUD the copies saw at video frame fr (the newest copy's that has
   it); NULL none */
static const uint8_t *la_hud_at(uint32_t fr)
{
    const LaSlot *best = NULL;
    int k, q = 0;
    if (!la_slot)
        return NULL;
    for (k = 0; k < LA_SLOTS; k++) {
        const LaSlot *s = &la_slot[k];
        int n = __atomic_load_n(&s->nhud, __ATOMIC_ACQUIRE), i;
        for (i = 0; i < n; i++)
            if (s->hud_frame[i] == fr && (!best || s->gen > best->gen)) {
                best = s;
                q = i;
            }
    }
    return best ? best->hud[q] : NULL;
}

/* the controls set for the next video frame (main.c): a copy on them
   if the last one ran on others (at any time: a copy made after the
   game's frame has read them predicts that frame's draw all the same) */
void interp_lookahead_input(uint16_t b0, uint16_t b1)
{
    la_pad_now[0] = b0;
    la_pad_now[1] = b1;
    if (la_child || !la_enabled() || la_cur < 0)
        return;
    if (la_slot[la_cur].pad[0] != b0 || la_slot[la_cur].pad[1] != b1)
        la_fork();
}

void port_wait_begin(void)
{
    /* no copy since the game's last draw (the look-ahead just turned on) */
    if (!la_child && la_enabled() && newest >= 0 && (la_cur < 0 || la_slot[la_cur].base_seq != dr[newest]->seq))
        la_fork();
}

void port_wait_end(void)
{
    la_pad_tick[0] = la_pad_now[0];
    la_pad_tick[1] = la_pad_now[1];
}

/* the copies' draw `seq` (the newest copy's that has it), its slot in
   *from; NULL none. With wait: the running copy, if it is to make it,
   waited for (a while) */
static const Draw *la_get(long seq, int wait, const LaSlot **from)
{
    const LaSlot *best = NULL;
    int k, q = 0;
    if (!la_slot)
        return NULL;
    la_reap();
    if (wait && la_pid > 0) {
        LaSlot *s = &la_slot[la_cur];
        long want = seq - s->base_seq;
        if (want >= 1 && want <= la_on && __atomic_load_n(&s->nready, __ATOMIC_ACQUIRE) < want) {
            double t0 = now_ms(), until = t0 + la_wait_ms, w;
            while (__atomic_load_n(&s->nready, __ATOMIC_ACQUIRE) < want && now_ms() < until &&
                   waitpid(la_pid, NULL, WNOHANG) == 0)
                usleep(100);
            w = now_ms() - t0;
            la_waits++;
            la_wait_sum += w;
            if (w > la_wait_max)
                la_wait_max = w;
            la_reap();
        }
    }
    for (k = 0; k < LA_SLOTS; k++) {
        const LaSlot *s = &la_slot[k];
        long i = seq - s->base_seq - 1;
        if (i >= 0 && i < __atomic_load_n(&s->nready, __ATOMIC_ACQUIRE) && (!best || s->gen > best->gen)) {
            best = s;
            q = (int)i;
        }
    }
    if (from)
        *from = best;
    return best ? &best->d[q] : NULL;
}

/* the pair shown: draws N + d - 1 and N + d (N + 0: n itself), both
   from one copy, the newest that has them; with wait, if none has them,
   the running copy waited for (a while). 0: none */
static int la_pair(const Draw *n, int d, int wait, const Draw **pa, const LaSlot **psa, const Draw **pb, const LaSlot **psb)
{
    const LaSlot *best = NULL;
    int k, pass;
    if (!la_slot)
        return 0;
    for (pass = 0; pass < 2; pass++) {
        best = NULL;
        la_reap();
        for (k = 0; k < LA_SLOTS; k++) {
            const LaSlot *s = &la_slot[k];
            long hi = n->seq + d - s->base_seq, lo = n->seq + d - 1 - s->base_seq;
            int got = __atomic_load_n(&s->nready, __ATOMIC_ACQUIRE);
            /* lo 0: n itself, the game's (a copy made after n) */
            if (hi >= 1 && hi <= got && lo >= 0 && (!best || s->gen > best->gen))
                best = s;
        }
        if (pass || !wait || la_pid <= 0)
            break;
        {
            /* the running copy, to make this pair: waited for when no copy
               has it, when its controls are new (a press, a release: shown
               as soon as it can be, a few ms), or always headless (no limit:
               the same pictures every run) */
            LaSlot *s = &la_slot[la_cur];
            long want = n->seq + d - s->base_seq;
            int sync = la_wait_ms >= 1e8;
            if (best == s || want < 1 || want > LA_DRAWS || n->seq + d - 1 < s->base_seq)
                break;
            if (best && !sync && s->pad[0] == best->pad[0] && s->pad[1] == best->pad[1])
                break;
            {
                /* (the high-resolution picture's time comes out of the same
                   frame: less wait with it) */
                double t0 = now_ms(), until = t0 + (hires_scale && la_wait_ms < 1e8 ? la_wait_ms / 2 : la_wait_ms), w;
                while (__atomic_load_n(&s->nready, __ATOMIC_ACQUIRE) < want && now_ms() < until &&
                       waitpid(la_pid, NULL, WNOHANG) == 0)
                    usleep(100);
                w = now_ms() - t0;
                la_waits++;
                la_waited = 1;
                la_wait_sum += w;
                if (w > la_wait_max)
                    la_wait_max = w;
            }
        }
    }
    if (!best)
        return 0;
    {
        long lo = n->seq + d - 1 - best->base_seq;
        *pa = lo == 0 ? (best->base_seq == n->seq ? n : NULL) : &best->d[lo - 1];
        *pb = &best->d[n->seq + d - best->base_seq - 1];
        *psa = *pa == n ? NULL : best;
        *psb = best;
    }
    return *pa != NULL;
}

/* the game's own draw d just ended: against the copies' latest of it */
static void la_check(const Draw *d)
{
    const LaSlot *s;
    const Draw *p = la_get(d->seq, 0, &s);
    if (!p) {
        la_missing++;
        return;
    }
    if (s->pad[0] != la_pad_tick[0] || s->pad[1] != la_pad_tick[1])
        la_changed++;
    else if (!memcmp(p->done, d->done, RAM_SIZE) && !memcmp(p->pre, d->pre, RAM_SIZE))
        la_exact++;
    else
        la_inexact++;
}

static void la_report_to(FILE *f)
{
    if (la_child || !la_forks)
        return;
    fprintf(f, "look-ahead (%d ahead): %ld copies run (fork max %.2f ms; a copy's last draw in %.1f ms on average, %.1f at most; "
            "%ld video frames waited for one, %.2f ms on average, %.1f at most); "
            "the game's draws against the copies': %ld exact, %ld differing with the same controls, %ld with the "
            "controls changed since, %ld with none in time; %ld pictures shown from them (%ld from an earlier copy), "
            "%ld held on the game's last draw; %ld copies failed; %ld pictures from an older copy than the one before, "
            "%ld earlier in the game's time than the one before, %ld repeated for want of a pair\n", la_on, la_forks, la_fork_ms_max,
            la_ready_n ? la_ready_ms_sum / la_ready_n : 0, la_ready_ms_max, la_waits, la_waits ? la_wait_sum / la_waits : 0,
            la_wait_max, la_exact, la_inexact, la_changed, la_missing, la_shown,
            la_stale, la_held, la_failed, la_back, la_time_back, la_repeats);
}

static void la_report(void)
{
    if (la_child)
        return;
    la_kill();
    la_report_to(stderr);
    if (paint_slid + paint_held)
        fprintf(stderr, "paint: %ld pieces slid between race frames, %ld shown as the nearer one\n", paint_slid, paint_held);
    if (hud_found + hud_missed)
        fprintf(stderr, "hud: %ld pictures with the copies' HUD of their frame, %ld with the game's own (none recorded); %ld sprites glided between jumps\n", hud_found, hud_missed, hud_glided);
    if (far_draws)
        fprintf(stderr, "far cars: %ld draws with the far cars' list, %ld redrawn; %ld times shown as the game's (the Super FX busy)\n",
                far_draws, far_redraws, far_busy);
}

/* the in-betweens from draw A to draw B (the game's or the copies'),
   made ahead while the Super FX is idle (the game's next frame has it
   busy at the wait's end): k / n for k = 1 .. n - 1, keyed by the two
   draws (their sequence and the copies that made them) */
typedef struct { long seq, gen_a, gen_b; int len; } LaKey;
static uint8_t *la_between[MAXSTEPS];
static HrList *la_between_hr[MAXSTEPS];
static uint16_t la_between_angles[MAXSTEPS][3];
static LaKey la_between_key[MAXSTEPS];

static int la_key_eq(const LaKey *x, const LaKey *y)
{
    return x->seq == y->seq && x->gen_a == y->gen_a && x->gen_b == y->gen_b && x->len == y->len;
}

static void la_make(const Draw *a, const Draw *b, const LaKey *key, int from)
{
    int k;
    /* this frame's and the next two's (spread over the frames; ahead of
       the game's next race frame taking the Super FX) */
    for (k = from < 1 ? 1 : from; k < key->len && k < MAXSTEPS && k <= from + (la_waited ? 0 : hires_scale ? 1 : 2); k++) {
        if (la_key_eq(&la_between_key[k], key))
            continue;
        if (!gsu_idle())
            return;
        if (!la_between[k]) {
            la_between[k] = (uint8_t *)malloc(RAM_SIZE);
            la_between_hr[k] = (HrList *)calloc(1, sizeof(HrList));
        }
        draw_between(a, b, (double)k / key->len, 1, la_between[k], la_between_angles[k], la_between_hr[k]);
        la_between_key[k] = *key;
    }
}

/* draw x shown as it stands (on b's snapshot): with the near shapes
   forced, or with the far cars' list, drawn again in the sandbox; a few
   kept (the same draw by its sequence, end and records), made ahead when
   the Super FX is idle (la_prefetch) for the frames that show them */
#define RD_KEEP 4
typedef struct {
    uint8_t *ram;
    HrList *hr;
    uint16_t angles[3];
    uint32_t key;
    long seq, used;
} Redraw;
static Redraw rd[RD_KEEP];
static long rd_clock;

static const Redraw *rd_get(const Draw *x, int make)
{
    uint32_t key = 2166136261u;
    unsigned i;
    int k, old = 0;
    for (i = 0; i < (unsigned)x->nrec * REC_SIZE && i < MAXREC * REC_SIZE; i++)
        key = (key ^ x->pre[REC_BASE + i]) * 16777619u;
    for (i = 0; i < 12; i++)
        key = (key ^ x->post[VIEW_PARAMS + i]) * 16777619u;
    key ^= x->ended;
    for (i = 0; i < (unsigned)x->ntxt; i++)
        key = (key ^ x->txt[i][2] ^ (uint32_t)x->txt[i][3] << 16) * 16777619u;
    key ^= (uint32_t)x->painted_ok << 31;
    for (k = 0; k < RD_KEEP; k++)
        if (rd[k].ram && rd[k].seq == x->seq && rd[k].key == key)
            return &rd[k];
    if (!make || !gsu_idle())
        return NULL;
    /* the one used longest ago (never the one this frame shows) */
    for (k = 1; k < RD_KEEP; k++)
        if (rd[k].used < rd[old].used)
            old = k;
    if (!rd[old].ram) {
        rd[old].ram = (uint8_t *)malloc(RAM_SIZE);
        rd[old].hr = (HrList *)calloc(1, sizeof *rd[old].hr);
    }
    far_redraws += x->far;
    draw_between(x, x, 1.0, 0, rd[old].ram, rd[old].angles, rd[old].hr);
    rd[old].seq = x->seq;
    rd[old].key = key;
    rd[old].used = ++rd_clock;
    return &rd[old];
}

static void show_draw(const Draw *b, const Draw *x)
{
    uint16_t a[3];
    int c;
    if ((lod_near() || x->far || x->ntxt || x->painted_ok || (interp_wide() && hires_scale && la_enabled())) && x->map_ok) {
        Redraw *e = (Redraw *)rd_get(x, 1);
        if (e) {
            e->used = ++rd_clock;
            picture_on(b, e->ram, x->base, e->angles, rgb_live, e->hr);
            return;
        }
        if (x->far)
            far_busy++;         /* the Super FX busy, none made ahead: the game's */
        if (interp_wide() && hires_scale && la_enabled() && hr_shown_last && live_pic_last) {
            hr_shown = 1;       /* (the wider view: the picture before again, not the game's window) */
            wide_repeats++;
            return;
        }
    }
    for (c = 0; c < 3; c++)
        a[c] = rd16(x->post, VIEW_PARAMS + 6 + 2 * c);
    picture_on(b, x->done, x->base, a, rgb_live, &x->hr);
}

/* the redraws the next frames' first and last pictures show (draws N + d
   - 1 and N + d), made now if the Super FX is idle */
static void la_prefetch(void)
{
    const Draw *n = NULL, *x;
    int k, d;
    if (!gsu_idle())
        return;
    for (k = 0; k < SLOTS; k++)
        if (dr[k]->valid && (!n || dr[k]->seq > n->seq))
            n = dr[k];
    if (!n)
        return;
    for (d = la_on; d >= 1 && d >= la_on - 1; d--)
        if ((x = la_get(n->seq + d, 0, NULL)) && (x->far || x->ntxt || x->painted_ok || lod_near() || (interp_wide() && hires_scale)) && x->map_ok)
            rd_get(x, 1);
}

/* the look-ahead's picture this video frame: 1 shown (live_pic), 0 the
   game's own. With the game's newest draw N, the j-th video frame shown
   since N ended (j from 0) shows the picture j / n of the way from draw
   N + d - 1 to N + d (d the depth; N + 0 the game's own N, the others
   the copies'), n the video frames from N's end to N + 1's (the copy ran
   it: a race frame that takes one more than the steady speed's is known
   ahead; the shown pair keeps the game's pace whatever its own length). Counted by frames shown, not by the frame
   count at N's end: a draw ending before or after the count moves on is
   followed by the same first frame (only a moment later, the window's
   wait being after it), and that one shows draw N + d - 1 while the new
   copy runs (one ahead: the last copy's). Short of the copies' draws, a
   shallower pair; short of any, N itself */
/* the race paused: the game draws no more 3D; its pause panel (drawn
   into its screen) over the picture as it stood. The last high-resolution
   picture kept, and each pixel of the game's window where the game's
   picture now differs from its picture at the pause's start (the panel,
   its animation, its marks) laid over it. 1: shown */
static uint32_t *pz_hr;
static VidFrame *pz_game;           /* the game's own frame, the last one not paused */
static int pz_on, pz_game_ok;
static uint32_t pz_obj0[VID_WIDTH * VID_HEIGHT];   /* its sprites alone, at the pause's start */
static uint32_t pz_obj[VID_WIDTH * VID_HEIGHT];    /* the paused frame's */

/* frame f's layers but the sprites (obj 0), or its sprites alone on the
   mark (1) */
static void pz_layers(const VidFrame *f, int obj, uint32_t *px)
{
    int l;
    memcpy(pic, f, sizeof *pic);
    for (l = 0; l <= VID_HEIGHT; l++) {
        if (obj) {
            pic->palette[l][0] = HR_MARK1;
            pic->line[l].tm &= 0x10;
            pic->line[l].ts = 0;
            pic->line[l].cgadsub = 0;
            pic->line[l].inidisp = 0x0F;
        } else {
            pic->line[l].tm &= (uint8_t)~0x10;
            pic->line[l].ts &= (uint8_t)~0x10;
        }
    }
    vid_draw_frame(pic, px);
}

static int pause_frame(const Draw *n)
{
    (void)n;
    const uint32_t *now = vid_picture();
    int S = hires_scale, W = VID_WIDTH * S, x, y, k, m;
    if (!S || !hr_out || !hr_last_y0)
        return 0;
    if (!pz_on) {
        if (!hr_shown_last)
            return 0;
        if (!pz_hr)
            pz_hr = (uint32_t *)malloc((size_t)VID_WIDTH * VID_HEIGHT * 36 * 4);
        if (!pz_game_ok)
            return 0;
        memcpy(pz_hr, hr_out, (size_t)W * VID_HEIGHT * S * 4);
        pz_layers(pz_game, 1, pz_obj0);
        pz_on = 1;
    }
    pz_layers(vid_frame(), 1, pz_obj);
    memcpy(hr_out, pz_hr, (size_t)W * VID_HEIGHT * S * 4);
    for (y = hr_last_y0; y < hr_last_y0 + 128 && y < VID_HEIGHT; y++)
        for (x = hr_last_x0; x < hr_last_x0 + 208 && x < VID_WIDTH; x++) {
            int i = y * VID_WIDTH + x;
            uint32_t c = now[i];
            /* the panel: the line's window 1 span (WH0-WH1: the oval the
               pause opens, its ring and inside); or a sprite the pause put
               up (not one it took down: the HUD stays as shown) */
            const VidScreen *ln = &vid_frame()->line[y + 1];
            int in_panel = ln->wh[0] <= ln->wh[1] && x >= ln->wh[0] && x <= ln->wh[1];
            if (!in_panel && ((pz_obj[i] & 0xFFFFFF) == 0xC6C6C6 || pz_obj[i] == pz_obj0[i]))
                continue;
            for (k = 0; k < S; k++)
                for (m = 0; m < S; m++)
                    hr_out[(size_t)(y * S + k) * W + x * S + m] = c;
        }
    memcpy(rgb_live, now, VID_WIDTH * VID_HEIGHT * 4);
    live_pic = rgb_live;
    hr_shown = 1;
    live_between++;
    la_path = "paused";
    return 1;
}

static int la_frame(void)
{
    const Draw *n = NULL, *b = NULL, *p, *da = NULL, *db = NULL, *d1;
    const LaSlot *sa = NULL, *sb = NULL;
    int j, k, nn, depth = la_on, gap;
    static int reported, shown_since;
    static long shown_of;
    uint16_t a[3];
    int c;
    if (!reported) {
        reported = 1;
        atexit(la_report);
    }
    for (k = 0; k < SLOTS; k++)
        if (dr[k]->valid && (!n || dr[k]->seq > n->seq))
            n = dr[k];
    if (!n)
        return 0;
    hud_a = hud_b = NULL;
    if (!snes_wram()[0x0D2F]) {
        pz_on = 0;
        if (!pz_game)
            pz_game = (VidFrame *)malloc(sizeof *pz_game);
        memcpy(pz_game, vid_frame(), sizeof *pz_game);
        pz_game_ok = 1;
    }
    if (n->settled)
        b = n;
    else if ((p = before(n)) && p->settled)
        b = p;
    if (!b)
        return 0;
    if (n->seq != shown_of) {
        shown_of = n->seq;
        shown_since = 0;
    }
    j = shown_since++;
    la_waited = 0;
    /* no new draw for longer than a race frame (the race over, a panel
       or results screen up): the game's own picture, N is old news */
    d1 = la_get(n->seq + 1, 0, NULL);
    gap = d1 ? (int)(d1->ended - n->ended) : port_steady_ticks;
    if (j > gap + 1) {
        /* the race paused ($0D2F stage 2 or 4, 6 while it resumes; the
           race phase $0DD6 6): the game draws no more; its picture now
           (the pause's panel) as the base, the 3D last shown on it, so the
           pause keeps the view (the wider one too) */
        const uint8_t *w = snes_wram();
        if (w[0x0DD6] == 0x06 && (w[0x0D2F] == 2 || w[0x0D2F] == 4 || w[0x0D2F] == 6) && pause_frame(n))
            return 1;
        return 0;
    }
    if (j == 0) {
        /* the first frame: draw N + d - 1 (the last copy's; N itself at
           depth 1) */
        /* the copy that has the pair the next frames show (the newest with
           both), so that this frame and those come from one prediction;
           else the newest with this draw */
        da = NULL;
        if (depth >= 2) {
            const Draw *pb;
            const LaSlot *psb;
            if (!la_pair(n, depth, 0, &da, &sa, &pb, &psb))
                da = la_get(n->seq + depth - 1, 0, &sa);
        }
        if (!da || !da->map_ok)
            da = n;
        else
            la_used(sa);
        la_path = "first frame";
        if (da->seq < la_last_time - 1e-6 && live_pic_last) {
            /* earlier than the last picture (no copy has the draw ahead
               yet): the last picture again */
            la_repeats++;
            live_pic = rgb_live;
            hr_shown = hr_shown_last;
            live_between++;
            return 1;
        }
        la_time((double)da->seq);
        hud_at(da->ended);
        show_draw(b, da);
        if (da != n)
            la_shown++;
        live_pic = rgb_live;
        live_between++;
        return 1;
    }
    for (; depth >= 1 && !db; depth--) {
        if (!la_pair(n, depth, 1, &da, &sa, &db, &sb)) {
            db = NULL;
            da = depth >= 2 ? la_get(n->seq + depth - 1, 0, &sa) : NULL;
        }
        if (!da)
            db = NULL;
        else if (!db && depth >= 2 && da->map_ok) {
            /* the farther draw not in yet: the nearer one held (a
               shallower pair would step back a race frame) */
            la_path = "farther draw not in";
            if (da->seq < la_last_time - 1e-6 && live_pic_last) {
                la_repeats++;
                live_pic = rgb_live;
                hr_shown = hr_shown_last;
                live_between++;
                return 1;
            }
            la_time((double)da->seq);
            hud_at(da->ended + (uint32_t)j);
            show_draw(b, da);
            la_held++;
            live_pic = rgb_live;
            live_between++;
            return 1;
        }
    }
    if (db && da->map_ok && db->map_ok && !cut(da, db)) {
        LaKey key;
        /* paced by the game's own way from N to N + 1 (the shown pair
           moves as the game does, whatever its own length) */
        nn = gap;
        if (nn < 2)
            nn = 2;
        if (nn >= MAXSTEPS)
            nn = MAXSTEPS - 1;
        key.seq = db->seq;
        key.gen_a = da == n || !sa ? 0 : sa->gen;
        key.gen_b = sb->gen;
        key.len = nn;
        {
            la_used(sb);
            la_path = j >= nn ? "pair end" : "between";
            la_time(j >= nn ? (double)db->seq : da->seq + (double)j / nn);
            la_make(da, db, &key, j);
            hud_at(da->ended + (uint32_t)j);
            if (j >= nn) {
                show_draw(b, db);
                la_shown++;
            } else if (la_between[j] && la_between_key[j].seq == key.seq) {
                /* these copies', or (the Super FX busy) earlier copies' of the same draws */
                picture_on(b, la_between[j], db->base, la_between_angles[j], rgb_live, la_between_hr[j]);
                la_shown++;
                if (!la_key_eq(&la_between_key[j], &key))
                    la_stale++;
            } else
                db = NULL;
        }
    } else
        db = NULL;
    if (!db && j > 0 && la_last_time > n->seq - 1e-6 && live_pic_last) {
        /* no pair to move between part way through (a cut, or the copies'
           draws not in): the last picture again (any draw shown now would
           be earlier in the game's time than it; a cut shows with the next
           race frame's first picture) */
        la_held++;
        la_repeats++;
        live_pic = rgb_live;
        hr_shown = hr_shown_last;
        live_between++;
        return 1;
    }
    if (!db) {
        /* no pair to move between (a cut, or the copies' draws not in):
           the newest draw ahead held, the one the first frame showed (N
           itself would step back a race frame or two, and forward again
           the frame after) */
        const Draw *h = la_on >= 2 ? la_get(n->seq + la_on - 1, 0, &sa) : NULL;
        if (!h || !h->map_ok)
            h = la_on >= 2 ? la_get(n->seq + 1, 0, &sa) : NULL;
        if (!h || !h->map_ok)
            h = n;
        else
            la_used(sa);
        la_path = "no pair";
        hud_at(h->ended + (uint32_t)j);
        la_time((double)h->seq);
        show_draw(b, h);
        if (j > 0)
            la_held++;
    }
    live_pic = rgb_live;
    live_between++;
    return 1;
}

/* each video frame: the picture to show (live_pic, NULL: the game's). A
   draw's in-betweens (from the draw before) start as soon as they are
   made, a few frames before the game shows the draw, on the snapshot of
   the draw before until its own screen stands whole */
static void live_frame(void)
{
    uint32_t now = snes_frame();
    Draw *d = NULL;
    const Draw *b = NULL, *p;
    uint32_t k;
    int j;
    static unsigned last_lead;
    static int was_race, lit;
    unsigned lead = 0;
    live_pic = NULL;
    hr_shown = 0;
    /* a race's start: the game's own pictures until its screen has once
       been at full brightness (the dark lead-in before the white flash
       shows its first 3D screens to nobody: drawn ahead here, they would
       flash up in the dark) */
    if (in_race > 0 && was_race <= 0)
        lit = 0;
    was_race = in_race;
    if (in_race <= 0)
        return;
    if (!lit) {
        uint8_t i = vid_frame()->line[VID_HEIGHT / 2].inidisp;
        lit = !(i & 0x80) && (i & 0x0F) == 0x0F;
        if (!lit)
            return;
    }
    /* the blanked lines at the picture's top: deeper than the last
       picture's in the one the game uploads a last third in; no snapshot
       taken of that one */
    while (lead < VID_HEIGHT && ((vid_frame()->line[lead + 1].inidisp & 0x80) || !(vid_frame()->line[lead + 1].inidisp & 0x0F)))
        lead++;
    for (j = 0; j < SLOTS; j++) {
        Draw *c = dr[j];
        if (c->valid && !c->settled && now > c->ended && lead <= last_lead && whole(vid_frame(), &live_maps, c))
            settle(c, now);
    }
    last_lead = lead;
    if (la_enabled()) {
        if (!la_frame())
            live_own++;
        la_prefetch();
        live_pic_last = live_pic != NULL;
        hr_shown_last = hr_shown;
        return;
    }
    live_pic_last = 0;
    for (j = 0; j < SLOTS; j++)
        if (dr[j]->valid && dr[j]->prepared && (!d || dr[j]->seq > d->seq))
            d = dr[j];
    if (!d) {
        live_own++;
        return;
    }
    k = now - d->prep_at + 1;
    if (d->settled)
        b = d;
    else if ((p = before(d)) && p->settled)
        b = p;
    if ((int)k >= d->nb || !b) {
        live_own++;
        return;
    }
    picture_on(b, d->between[k], d->base, d->between_angles[k], rgb_live, NULL);
    live_pic = rgb_live;
    live_between++;
}

const uint32_t *interp_picture(void)
{
    return live ? live_pic : NULL;
}

/* files and live together (a check): what live shows, each video frame */
static void live_file(long frame)
{
    if (!(frame >= from && (to < 0 || frame <= to)))
        return;
    write_picture(frame, -1, live_pic && !getenv("PORT_INTERP_REAL") ? live_pic : vid_picture());
    if (hr_shown && !getenv("PORT_INTERP_REAL")) {
        /* the high-resolution picture beside it: hFFFFFF.ppm */
        char path[1024];
        FILE *f;
        int w = VID_WIDTH * hires_scale, h = VID_HEIGHT * hires_scale, i;
        snprintf(path, sizeof path, "%s/h%06ld.ppm", dir, frame);
        if ((f = fopen(path, "wb"))) {
            fprintf(f, "P6\n%d %d\n255\n", w, h);
            for (i = 0; i < w * h; i++) {
                uint8_t c[3] = { (uint8_t)(hr_out[i] >> 16), (uint8_t)(hr_out[i] >> 8), (uint8_t)hr_out[i] };
                fwrite(c, 1, 3, f);
            }
            fclose(f);
        }
    }
}


/* ---- files ---- */
static void put_rgb(uint8_t *p, uint32_t c) { p[0] = (uint8_t)(c >> 16); p[1] = (uint8_t)(c >> 8); p[2] = (uint8_t)c; }

/* the 3D canvas as a PPM, coloured by the palette of the last finished
   picture's middle line (the canvas is 8 bits a pixel: CGRAM indices) */
static void write_canvas(long frame, int k)
{
    char path[1024];
    FILE *f;
    int i, x;
    uint8_t row[256 * 3];
    const uint16_t *pal = vid_frame()->palette[112];
    snprintf(path, sizeof path, "%s/f%06ld_%d.ppm", dir, frame, k);
    if (!(f = fopen(path, "wb")))
        return;
    fprintf(f, "P6\n256 256\n255\n");
    for (i = 0; i < 256 * 256; i += 256) {
        for (x = 0; x < 256; x++) {
            uint16_t c = out.set[i + x] ? pal[out.px[i + x]] : 0;
            row[3 * x] = (uint8_t)((c & 31) << 3 | (c & 31) >> 2);
            row[3 * x + 1] = (uint8_t)((c >> 5 & 31) << 3 | (c >> 5 & 31) >> 2);
            row[3 * x + 2] = (uint8_t)((c >> 10 & 31) << 3 | (c >> 10 & 31) >> 2);
        }
        fwrite(row, 1, sizeof row, f);
    }
    fclose(f);
}

static void write_picture(long frame, int k, const uint32_t *px)
{
    char path[1024];
    FILE *f;
    int i, x;
    uint8_t row[VID_WIDTH * 3];
    if (k < 0)
        snprintf(path, sizeof path, "%s/l%06ld.ppm", dir, frame);
    else
        snprintf(path, sizeof path, "%s/c%06ld_%d.ppm", dir, frame, k);
    if (!(f = fopen(path, "wb")))
        return;
    fprintf(f, "P6\n%d %d\n255\n", VID_WIDTH, VID_HEIGHT);
    for (i = 0; i < VID_HEIGHT; i++) {
        for (x = 0; x < VID_WIDTH; x++)
            put_rgb(row + 3 * x, px[i * VID_WIDTH + x]);
        fwrite(row, 1, sizeof row, f);
    }
    fclose(f);
}

/* the waiting draw's pictures, from the last finished picture (its own
   frame on screen by now) */
static void compose(void)
{
    const VidFrame *f = vid_frame();
    const Draw *d = &res->d;
    int k;
    if (!res->ready)
        return;
    res->ready = 0;
    pictures++;
    vid_draw_frame(f, rgb_real);
    /* the game's own screen must be what the picture shows: the same
       bytes in VRAM, and seen (inverting them changes the picture) */
    memcpy(pic, f, sizeof *pic);
    if (!patch(pic, &live_maps, overlay(&live_maps, d, d->done), d, 0) || memcmp(pic->vram, f->vram, sizeof pic->vram))
        return;
    patch(pic, &live_maps, overlay(&live_maps, d, d->done), d, 1);
    vid_draw_frame(pic, rgb);
    if (!memcmp(rgb, rgb_real, VID_WIDTH * VID_HEIGHT * 4))
        return;
    pictures_shown++;
    picture(f, &live_maps, d, res->ram[steps], res->angles[steps], rgb);
    if (!memcmp(rgb, rgb_real, VID_WIDTH * VID_HEIGHT * 4))
        pictures_matched++;
    if (!(res->frame >= from && (to < 0 || res->frame <= to)))
        return;
    write_picture(res->frame, steps, rgb);
    for (k = 1; k < res->n; k++) {
        picture(f, &live_maps, d, res->ram[k], res->angles[k], rgb);
        write_picture(res->frame, k, rgb);
        pictures_made++;
    }
}

static void files_frame(long frame)
{
    const Draw *now, *prev;
    int k, write;
    compose();
    now = dr[newest];
    if (!now->valid)
        return;
    prev = before(now);
    write = frame >= from && (to < 0 || frame <= to);
    /* the frame itself, drawn again: it must be what the game drew */
    draw_between(now, now, 1.0, 0, res->ram[steps], res->angles[steps], NULL);
    checked++;
    if (!memcmp(out.px, now->real.px, sizeof out.px) && !memcmp(out.set, now->real.set, sizeof out.set))
        matched++;
    else if (checked - matched <= 5) {
        long diff = 0;
        for (k = 0; k < 256 * 256; k++)
            diff += out.px[k] != now->real.px[k] || out.set[k] != now->real.set[k];
        fprintf(stderr, "interp: frame %ld drawn again differs from the game's in %ld pixels\n", frame, diff);
    }
    if (write && canvases)
        write_canvas(frame, steps);
    res->d.base = now->base;
    res->d.ended = now->ended;
    memcpy(res->d.done, now->done, RAM_SIZE);
    memcpy(res->d.post, now->post, RAM_SIZE);
    res->frame = frame;
    res->n = 1;
    res->ready = 1;
    if (!prev || !prev->map_ok || !now->map_ok)
        return;
    if (cut(prev, now)) {
        cuts++;
        return;
    }
    for (k = 1; k < steps; k++) {
        draw_between(prev, now, (double)k / steps, 1, res->ram[k], res->angles[k], NULL);
        if (write && canvases)
            write_canvas(frame, k);
        made++;
    }
    res->n = steps;
}

/* each video frame */
static void interp_frame_(long frame);
void interp_frame(long frame)
{
    double t0 = now_ms(), t;
    interp_frame_(frame);
    log_frame(now_ms() - t0);
    if (live) {
        t = now_ms() - t0;
        t_sum += t;
        t_n++;
        if (t > t_max) t_max = t;
        if (t > 8) t_over8++;
        if (t > 16) t_over16++;
    }
}

static void interp_frame_(long frame)
{
    if (getenv("PORT_OAM_TRACE")) {
        /* the sprites on screen in the last picture (y below 224), a line a frame */
        static FILE *of;
        const VidFrame *f = vid_frame();
        const uint8_t *oam = f->oam[f->versions - 1];
        int k, n = 0;
        if (!of)
            of = fopen(getenv("PORT_OAM_TRACE"), "w");
        for (k = 0; k < 128; k++)
            if (oam[4 * k + 1] < 224)
                n++;
        fprintf(of, "%ld %d %d\n", frame, n, world_frame);
    }
    if (!enabled())
        return;
    if (pending && gsu_idle()) {
        pending = 0;
        if (files)
            files_frame(frame);
        if (live && newest >= 0 && dr[newest]->valid && !la_enabled()) {
            double t0 = now_ms();
            live_prepare();
            if (now_ms() - t0 > t_prep_max)
                t_prep_max = now_ms() - t0;
        }
    }
    if (live) {
        live_frame();
        if (files)
            live_file(frame);
    }
}

/* the game's speed: 0 the original's (a race frame when the Super FX is
   done: 7 to 15 a second), else a steady 60 / ticks race frames a second,
   the Super FX overclocked to keep it */
/* the time scale on (K > 1) or off (0), its fractions started afresh */
static void timescale_set(double k)
{
    if (on < 0)
        enabled();
    timescale = k > 1.0 ? k : 0;
    port_timescale_k = (int)(timescale + 0.5);
    memset(ts_frac, 0, sizeof ts_frac);
    memset(ts_frac_rec, 0, sizeof ts_frac_rec);
    memset(ts_timer_frac, 0, sizeof ts_timer_frac);
    memset(tc_frac, 0, sizeof tc_frac);
    memset(boost_rec, 0, sizeof boost_rec);
    memset(boost_fraction, 0, sizeof boost_fraction);
    memset(tw_frac, 0, sizeof tw_frac);
    wheels_on = !getenv("PORT_TS_NOWHEELS");
    if (timescale && !on)
        on = 1;                 /* the hooks on; the in-between frames' work stays off (files, live) */
}

/* the chosen speed (interp_speed_set), and whether a race frame runs: the
   speeds hold in race frames only; menus (their 3D previews) and the rest
   run as on the console */
static int speed_mode;

static void speed_apply(void)
{
    int ticks = in_race > 0 ? speed_mode : 0;
    if (ticks == 1 || ticks == -60) {
        /* 60 a second: every video frame a race frame; -60 natural 60: the
           cars' physics at a sixth a frame, the race's phases at the
           original's 10 a second */
        port_steady_ticks = 1;
        port_fast_upload = 1;
        gsu_set_speed(8.0);
        snes_set_cpu_speed(3.5);
        timescale_set(ticks == -60 ? 6.0 : 0);
        return;
    }
    timescale_set(0);
    /* -1: as fast as it goes (one-go upload, overclocked, no wait) */
    port_steady_ticks = ticks > 0 ? ticks : 0;
    port_fast_upload = ticks != 0;
    gsu_set_speed(ticks == 0 ? 1.0 : ticks >= 4 ? 6.0 : 8.0);
    /* the S-CPU's share of a frame is about 1M clocks: 15 a second fit,
       20 nearly, 30 need it twice as fast */
    snes_set_cpu_speed(ticks == 0 ? 1.0 : ticks >= 4 ? 1.0 : ticks == 3 ? 1.25 : 2.0);
}

void interp_speed_set(int ticks)
{
    if (ticks != speed_mode)
        log_mode_change(speed_mode);
    speed_mode = ticks;
    speed_apply();
}

/* a race frame starting (1), or a menu's 3D frame (0) */
static long mode_flips;
static uint32_t mode_flip_frames[64];

void port_race_mode(int race)
{
    if (getenv("PORT_MODE_TRACE") && race != in_race) {
        if (mode_flips < 64)
            mode_flip_frames[mode_flips] = snes_frame();
        if (++mode_flips == 64 || mode_flips % 500 == 0) {
            int k;
            fprintf(stderr, "race/menu switches: %ld; first at frames:", mode_flips);
            for (k = 0; k < 64 && k < mode_flips; k++)
                fprintf(stderr, " %u", mode_flip_frames[k]);
            fprintf(stderr, "\n");
        }
    }
    if (race && getenv("PORT_CAM_DUMP")) {
        /* the race camera's record (two links on from $0346, bank 0) and
           its tied object, $40 bytes each, at each race frame's start */
        static FILE *cf;
        const uint8_t *w = snes_wram();
        uint16_t x = rd16(w, 0x0346), cam, obj;
        uint32_t fr = snes_frame();
        if (!cf)
            cf = fopen(getenv("PORT_CAM_DUMP"), "wb");
        x = rd16(w, (uint16_t)(0x204F + x));
        cam = rd16(w, (uint16_t)(0x204F + x));
        obj = x;
        fwrite(&fr, 4, 1, cf);
        fwrite(&cam, 2, 1, cf);
        fwrite(w + cam, 1, 0x40, cf);
        fwrite(&obj, 2, 1, cf);
        fwrite(w + 0x2000 + obj, 1, 0x40, cf);
    }
    if (race && getenv("PORT_OBJ_CENSUS")) {
        /* the used objects (list from $0329, linked by +$00) by handler A
           (+$1A/+$1C), with whether they have a Super FX record (+$22) */
        static long seen[4096][3]; static uint32_t key[4096]; static int nk; static long frames;
        const uint8_t *w = snes_wram();
        uint16_t o = rd16(w, 0x0329);
        int g = 0, k;
        frames++;
        while (o && g++ < 400) {
            const uint8_t *ob = w + 0x2000 + o;
            uint32_t h = (uint32_t)ob[0x1C] << 16 | rd16(ob, 0x1A);
            for (k = 0; k < nk && key[k] != h; k++)
                ;
            if (k == nk && nk < 4096)
                key[nk++] = h;
            if (k < 4096) {
                seen[k][0]++;
                if (rd16(ob, 0x22))
                    seen[k][1]++;
            }
            o = rd16(ob, 0x00);
        }
        if (frames == 300)
            for (k = 0; k < nk; k++)
                fprintf(stderr, "object handler $%06X: %.2f a frame, %.0f%% with a Super FX record\n", key[k],
                        seen[k][0] / 300.0, 100.0 * seen[k][1] / seen[k][0]);
    }
    if (race)
        lg_race_frames++;
    if (race != in_race) {
        in_race = race;
        speed_apply();
    }
}

/* ---- the session log (PORT_LOG_DIR: the window's play) ----
   For each speed mode played, every ten seconds of racing and at the end:
   the game's race frames a second, the pictures shown a second and how many
   were in-betweens, and how soon a steering press reaches the player's car
   (its steering +$3A, then its heading +$26), from the press's video frame
   to the physics run that shows it. */
typedef struct {
    double secs, worst_ms;
    long race_frames, pictures, betweens, presses, steer_n, turn_n;
    double steer_ms, turn_ms;
} LogStats;
static FILE *log_f;
static LogStats lg_win, lg_mode[7];
static long lg_race_frames;            /* race frames counted (port_race_mode) */
static uint32_t press_frame;           /* a steering press waiting for the car, 0 none */
static int steer_seen, turn_seen;

static const char *mode_name(int m)
{
    switch (m) {
    case -60: return "natural 60";
    case 6: return "natural 10";
    case 4: return "steady 15";
    case 3: return "steady 20";
    case 2: return "steady 30";
    case 0: return "original";
    default: return "max";
    }
}
static int mode_idx(int m) { return m == -60 ? 0 : m == 6 ? 1 : m == 4 ? 2 : m == 3 ? 3 : m == 2 ? 4 : m == 0 ? 5 : 6; }
static const int mode_of_idx[7] = { -60, 6, 4, 3, 2, 0, -1 };

static void log_line(int mode, const char *what, const LogStats *t)
{
    char a[32] = "-", b[32] = "-";
    if (!log_f || t->secs < 0.5)
        return;
    if (t->steer_n)
        snprintf(a, sizeof a, "%.0f ms", t->steer_ms / t->steer_n);
    if (t->turn_n)
        snprintf(b, sizeof b, "%.0f ms", t->turn_ms / t->turn_n);
    fprintf(log_f, "%-10s %-8s %6.1f s racing | game %5.1f frames/s | shown %5.1f pictures/s, %3.0f%% in-between | "
            "%3ld steering presses: to the car's steering %s, to its turning %s | worst frame %.1f ms\n",
            mode_name(mode), what, t->secs, t->race_frames / t->secs, t->pictures / t->secs,
            t->pictures ? 100.0 * t->betweens / t->pictures : 0.0, t->presses, a, b, t->worst_ms);
    fflush(log_f);
}

static void log_add(LogStats *to, const LogStats *t)
{
    to->secs += t->secs;
    to->race_frames += t->race_frames;
    to->pictures += t->pictures;
    to->betweens += t->betweens;
    to->presses += t->presses;
    to->steer_n += t->steer_n;
    to->turn_n += t->turn_n;
    to->steer_ms += t->steer_ms;
    to->turn_ms += t->turn_ms;
    if (t->worst_ms > to->worst_ms)
        to->worst_ms = t->worst_ms;
}

static void log_flush_window(void)
{
    log_line(speed_mode, "10 s", &lg_win);
    log_add(&lg_mode[mode_idx(speed_mode)], &lg_win);
    memset(&lg_win, 0, sizeof lg_win);
}

static void log_end(void)
{
    int k;
    if (!log_f)
        return;
    log_flush_window();
    fprintf(log_f, "\n-- each mode over the session --\n");
    for (k = 0; k < 7; k++)
        log_line(mode_of_idx[k], "total", &lg_mode[k]);
    la_report_to(log_f);
    fclose(log_f);
    log_f = NULL;
}

/* the window's play: the log opened in dir */
void interp_log_open(const char *dir)
{
    char path[1024];
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    snprintf(path, sizeof path, "%s/session-%04d%02d%02d-%02d%02d%02d.txt", dir, t->tm_year + 1900, t->tm_mon + 1,
             t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
    if (!(log_f = fopen(path, "w")))
        return;
    fprintf(log_f, "Stunt Race FX session %04d-%02d-%02d %02d:%02d:%02d (one line per ten seconds of racing; "
            "latency from a LEFT/RIGHT press to the player's car)\n\n", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
            t->tm_hour, t->tm_min, t->tm_sec);
    atexit(log_end);
    fprintf(stderr, "session log: %s\n", path);
}

/* the speed changed (F3): the window so far logged under the old one */
static void log_mode_change(int old_mode)
{
    if (!log_f)
        return;
    log_line(old_mode, "10 s", &lg_win);
    log_add(&lg_mode[mode_idx(old_mode)], &lg_win);
    memset(&lg_win, 0, sizeof lg_win);
}

/* the buttons each video frame (main.c): a new LEFT or RIGHT press */
void interp_log_input(uint16_t buttons)
{
    static uint16_t last;
    uint16_t fresh = (uint16_t)(buttons & ~last & 0x0300);   /* SNES_LEFT 0x0200, SNES_RIGHT 0x0100 */
    last = buttons;
    if (!log_f || !fresh || in_race <= 0)
        return;
    lg_win.presses++;
    press_frame = snes_frame();
    steer_seen = turn_seen = 0;
}

/* each physics run: the player's car (the one nearest the view) */
static void log_physics(const uint8_t *ram)
{
    uint16_t r = rd16(ram, 0x2332), best = 0;
    int guard = 0;
    long bestd = 0x7FFFFFFF;
    const uint8_t *w = snes_wram();
    static uint16_t last_steer, last_turn, last_rec;
    if (!log_f || !press_frame)
        return;
    while (r && guard++ < 64) {
        if (rd16(ram, (uint16_t)(r + 0x0A)) == 0xB1A1) {
            long dx = s16(rd16(ram, (uint16_t)(r + 0x1E)) - rd16(w, 0xC5)), dz = s16(rd16(ram, (uint16_t)(r + 0x22)) - rd16(w, 0xC9));
            long dd = labs(dx) + labs(dz);
            if (dd < bestd) {
                bestd = dd;
                best = r;
            }
        }
        r = rd16(ram, (uint16_t)(r + 0x04));
    }
    if (!best)
        return;
    if (best != last_rec) {
        last_rec = best;
        last_steer = rd16(ram, (uint16_t)(best + 0x3A));
        last_turn = rd16(ram, (uint16_t)(best + 0x26));
        return;
    }
    {
        uint16_t st = rd16(ram, (uint16_t)(best + 0x3A)), tu = rd16(ram, (uint16_t)(best + 0x26));
        double ms = (snes_frame() - press_frame) * 1000.0 / 60.0988;
        if (!steer_seen && st != last_steer) {
            steer_seen = 1;
            lg_win.steer_n++;
            lg_win.steer_ms += ms;
        }
        if (steer_seen && !turn_seen && abs(s16(tu - last_turn)) > 8) {
            turn_seen = 1;
            lg_win.turn_n++;
            lg_win.turn_ms += ms;
            press_frame = 0;
        }
        if (ms > 1000)
            press_frame = 0;
        last_steer = st;
        last_turn = tu;
    }
}

/* each video frame (interp_frame): the counts */
static void log_frame(double frame_ms)
{
    static long last_race_frames;
    if (!log_f)
        return;
    if (in_race > 0) {
        lg_win.secs += 1.0 / 60.0988;
        lg_win.race_frames += lg_race_frames - last_race_frames;
        lg_win.pictures++;
        if (live_pic)
            lg_win.betweens++;
        if (frame_ms > lg_win.worst_ms)
            lg_win.worst_ms = frame_ms;
        if (lg_win.secs >= 10.0)
            log_flush_window();
    }
    last_race_frames = lg_race_frames;
}

/* RaceFrameRun's HUD (race_frame.c's port copy): in a frame that runs it
   (world 1) its Super FX calls are recorded; in a frame between (0) they
   are drawn again into this frame's screen (the Super FX idle there) */
void port_hud_begin(int world)
{
    int k;
    if (on <= 0)
        return;
    if (world) {
        hud_n = 0;
        hud_recording = 1;
        return;
    }
    for (k = 0; k < hud_n; k++) {
        RegsGSU keep = gsu;
        uint8_t *ram = snes_sfx_ram();
        uint8_t params[0x400];
        memcpy(params, ram, sizeof params);
        memcpy(ram, hud[k].params, sizeof params);
        gsu.r[0] = hud[k].regs.r[0];
        memcpy(gsu.r, hud[k].regs.r, sizeof gsu.r);
        gsu.z = hud[k].regs.z; gsu.cy = hud[k].regs.cy; gsu.s = hud[k].regs.s; gsu.ov = hud[k].regs.ov;
        gsu.rombr = hud[k].regs.rombr; gsu.rambr = hud[k].regs.rambr;
        gsu.colr = hud[k].regs.colr; gsu.por = hud[k].regs.por;
        sandboxed = 1;
        gsu_sandbox_run(hud[k].bank, hud[k].addr);
        sandboxed = 0;
        memcpy(ram, params, sizeof params);
        gsu = keep;
    }
}

void port_hud_end(void)
{
    hud_recording = 0;
}
