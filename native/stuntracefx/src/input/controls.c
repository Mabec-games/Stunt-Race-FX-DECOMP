/*
 * controls.c: the game's controls (controls.h), from each device as the
 * engine reports it.
 *
 * The keyboard and the controllers give the SNES pad's buttons through
 * player 1's bindings (the pause screen changes them): by default arrows to
 * steer, Z the gas (B), X the brake (A), A boost (Y), S X, Q and W the hard
 * turns (L, R), Return Start, Shift Select; a controller by each button's
 * place, the SNES's own layout, its d-pad and left stick steering. With no
 * controller the keyboard is player 1; with one, the controller is player 1
 * and the keyboard player 2; with two, the second controller is player 2.
 *
 * The touch screen is the earlier front end's touch scheme, the race's pad
 * (object/gsu_record.c ctl_player: B accelerates, A brakes and reverses
 * below a crawl, Left and Right steer, L and R steer harder, Y boosts):
 *
 * While the player drives (controls_driving):
 *   the gas    on by itself (Auto-Accelerate, on unless turned off): B held
 *              all the while, let go while the brake is (the pedal wins).
 *              Turned off, the wheel's finger holds it while it stays on the
 *              glass, and lifting coasts.
 *   the wheel  a finger right of the pedals; moving it sideways steers,
 *              full lock at 90 points either side, the neutral point sliding
 *              along when the finger goes past the lock so the thumb never
 *              runs out of room. Past a fifth of the lock it is Left or
 *              Right, at nine tenths also L or R (the hard turn). A wheel
 *              drawn where the finger landed turns with the steering.
 *   the pedals the left 30% of the window, two pedals one above the other:
 *              the lower holds A, the brake; the upper holds Y, the boost.
 * Elsewhere (menus, the title, the pause screen): a swipe is the d-pad, the
 * content following the finger (a swipe to the left is Right, a swipe up is
 * Down); a tap is B and Start together (every menu's confirm); a tap with
 * two fingers is Y (back), or the pause while driving.
 * The pause button, two bars at the top centre, opens and shuts the pause
 * screen (Select and Start together). Beside it on its right, in a
 * one-player race, the view button switches the race's view, behind the
 * car or from above (race/overhead.c), its picture what it switches to: a
 * cube seen from above, or a road going away.
 *
 * A point is the window's shorter side over 430 (the phone in landscape the
 * scheme was made on). Time is counted in updates, 1/60 s each. Nothing is
 * drawn or pressed until a first touch.
 */
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "input/controls.h"
#include "pause/pause.h"
#include "pause/pause_art.h"
#include "race/race.h"
#include "save/save.h"
#include "system/game.h"
#include "system/chip.h"
#include "system/state.h"
#include "object/object.h"
#include "data/cart.h"
#include "fx/fx.h"

/* ---- the keyboard and the controllers ---- */

#define BUTTONS 12
static const uint16_t k_button[BUTTONS] = { PAD_B, PAD_Y, PAD_SELECT, PAD_START, PAD_UP, PAD_DOWN,
                                            PAD_LEFT, PAD_RIGHT, PAD_A, PAD_X, PAD_L, PAD_R };
static const uint16_t k_ctl_default[BUTTONS] = { NSHVC_CTL_SOUTH, NSHVC_CTL_WEST, NSHVC_CTL_BACK, NSHVC_CTL_MENU,
                                                 NSHVC_CTL_UP, NSHVC_CTL_DOWN, NSHVC_CTL_LEFT, NSHVC_CTL_RIGHT,
                                                 NSHVC_CTL_EAST, NSHVC_CTL_NORTH, NSHVC_CTL_LB, NSHVC_CTL_RB };
static int key_of[BUTTONS] = { 'Z', 'A', 0x10, 0x0D, 0x26, 0x28, 0x25, 0x27, 'X', 'S', 'Q', 'W' };
static uint16_t ctl_of[BUTTONS] = { NSHVC_CTL_SOUTH, NSHVC_CTL_WEST, NSHVC_CTL_BACK, NSHVC_CTL_MENU,
                                    NSHVC_CTL_UP, NSHVC_CTL_DOWN, NSHVC_CTL_LEFT, NSHVC_CTL_RIGHT,
                                    NSHVC_CTL_EAST, NSHVC_CTL_NORTH, NSHVC_CTL_LB, NSHVC_CTL_RB };

static int source = CONTROLS_KEYBOARD;
static uint16_t ctl_last;               /* the first controller's buttons at the last update */

static int button_index(uint16_t button)
{
    int b;

    for (b = 0; b < BUTTONS; b++)
        if (k_button[b] == button)
            return b;
    return -1;
}

void controls_bind_key(uint16_t button, int key)
{
    int b = button_index(button);

    if (b >= 0)
        key_of[b] = key;
}

void controls_bind_controller(uint16_t button, uint16_t ctl)
{
    int b = button_index(button);

    if (b >= 0)
        ctl_of[b] = ctl;
}

int controls_source(void) { return source; }

const char *controls_name(uint16_t button)
{
    int b = button_index(button);

    if (b < 0)
        return "";
    if (source == CONTROLS_CONTROLLER)
        return ctl_of[b] ? nshvc_controller_label(0, ctl_of[b]) : "";
    return key_of[b] ? nshvc_key_name((nshvc_key)key_of[b]) : "";
}

/* controller c through `map`; its left stick steers */
static uint16_t controller_pad(const nshvc_input *in, int c, const uint16_t *map)
{
    uint16_t pad = 0;
    int b;

    if (!in->controller[c].on)
        return 0;
    for (b = 0; b < BUTTONS; b++)
        if (in->controller[c].buttons & map[b])
            pad |= k_button[b];
    if (in->controller[c].ly < -0.5f)
        pad |= PAD_UP;
    if (in->controller[c].ly > 0.5f)
        pad |= PAD_DOWN;
    if (in->controller[c].lx < -0.5f)
        pad |= PAD_LEFT;
    if (in->controller[c].lx > 0.5f)
        pad |= PAD_RIGHT;
    return pad;
}

static uint16_t keyboard_pad(const nshvc_input *in)
{
    uint16_t pad = 0;
    int b;

    for (b = 0; b < BUTTONS; b++)
        if (key_of[b] > 0 && key_of[b] < 256 && NSHVC_KEY_HELD(in, key_of[b]))
            pad |= k_button[b];
    return pad;
}

/* ---- the touch screen ---- */

typedef enum { ROLE_NONE, ROLE_WHEEL, ROLE_BRAKE, ROLE_BOOST, ROLE_MENU, ROLE_PAUSE, ROLE_VIEW } Role;

#define FINGERS 10
static struct {
    int used, id;
    Role role;
    float x, y;                         /* where it landed */
    float now_x, now_y;                 /* where it is */
    uint32_t at;                        /* the update it landed */
} finger[FINGERS];

static const float k_points = 430;      /* the window's shorter side in the scheme's points */
static const float k_steer_range = 90;  /* points of sideways travel to full lock */
static const float k_brake_zone = 0.3f; /* the left of the window: the pedals */
static const float k_boost_above = 0.45f;   /* the pedals' split: the boost above this much of the height */
static const float k_wheel_degrees = 120;
static const uint32_t k_tap_updates = 15;   /* 0.25 s */
static const float k_tap_points = 12;
static const float k_swipe_points = 30;
static const uint32_t k_pulse_updates = 5;  /* a press the game's pad read sees */

static uint32_t updates;
static int touched;                     /* a finger has been on the screen */
static int driving, driving_marked, gas;
static float wheel_x, wheel_y, wheel_turn;
static uint32_t gesture_at;
static int gesture_fingers, gesture_moved;
static uint16_t held;                   /* buttons held by fingers */
static uint16_t pulse;                  /* buttons pressed a moment */
static uint32_t pulse_until;

void controls_driving(void) { driving_marked = 1; }

/* the window in pixels, a point in pixels, and the safe area's top */
static void window(float *w, float *h, float *pt, float *safe_top)
{
    nshvc_display d = nshvc_display_info();

    *w = (float)(d.width > 0 ? d.width : 1);
    *h = (float)(d.height > 0 ? d.height : 1);
    *pt = fminf(*w, *h) / k_points;
    *safe_top = d.safe_top;
}

static void press(uint16_t buttons)
{
    if (updates >= pulse_until)         /* the last moment's over */
        pulse = 0;
    pulse |= buttons;
    pulse_until = updates + k_pulse_updates;
}

static int has(Role r)
{
    int i;

    for (i = 0; i < FINGERS; i++)
        if (finger[i].used && finger[i].role == r)
            return 1;
    return 0;
}

static int fingers_down(void)
{
    int i, n = 0;

    for (i = 0; i < FINGERS; i++)
        n += finger[i].used;
    return n;
}

/* the pause button's centre, below the safe area at the top centre */
static void pause_place(float *x, float *y)
{
    float w, h, pt, top;

    window(&w, &h, &pt, &top);
    *x = w / 2;
    *y = fmaxf(top, 6 * pt) + 24 * pt;
}

static int on_pause(float x, float y)
{
    float w, h, pt, top, cx, cy;

    window(&w, &h, &pt, &top);
    pause_place(&cx, &cy);
    return fabsf(x - cx) <= 28 * pt && fabsf(y - cy) <= 22 * pt;
}

/* the view button's centre, beside the pause button on its right */
static void view_place(float *x, float *y)
{
    float w, h, pt, top;

    window(&w, &h, &pt, &top);
    pause_place(x, y);
    *x += 56 * pt;
}

/* the view button shown: in a one-player race (race/overhead.c), not while
   the pause screen holds the game */
static int view_shown(void) { return !pause_holding() && (race_view_switchable() || race_view_moving()); }

static int on_view(float x, float y)
{
    float w, h, pt, top, cx, cy;

    if (!view_shown())
        return 0;
    window(&w, &h, &pt, &top);
    view_place(&cx, &cy);
    return fabsf(x - cx) <= 28 * pt && fabsf(y - cy) <= 22 * pt;
}

static Role role_at(float x, float y)
{
    float w, h, pt, top;

    window(&w, &h, &pt, &top);
    if (on_pause(x, y))
        return ROLE_PAUSE;
    if (on_view(x, y))
        return ROLE_VIEW;
    if (driving) {
        if (x < w * k_brake_zone)
            return y < h * k_boost_above ? ROLE_BOOST : ROLE_BRAKE;
        return has(ROLE_WHEEL) ? ROLE_NONE : ROLE_WHEEL;
    }
    return has(ROLE_MENU) ? ROLE_NONE : ROLE_MENU;
}

/* the steering buttons for s (-1 left .. 1 right): Left or Right past a
   fifth of the lock, and L or R at nine tenths */
static void steer(float s)
{
    held &= (uint16_t)~(PAD_LEFT | PAD_RIGHT | PAD_L | PAD_R);
    if (s <= -0.2f)
        held |= PAD_LEFT;
    if (s >= 0.2f)
        held |= PAD_RIGHT;
    if (s <= -0.9f)
        held |= PAD_L;
    if (s >= 0.9f)
        held |= PAD_R;
    wheel_turn = s;
}

static void began(int id, float x, float y)
{
    int i;
    Role r;

    for (i = 0; i < FINGERS && finger[i].used; i++)
        ;
    if (i == FINGERS)
        return;
    if (!fingers_down()) {
        gesture_at = updates;
        gesture_fingers = 0;
        gesture_moved = 0;
    }
    gesture_fingers++;
    r = role_at(x, y);
    finger[i].used = 1;
    finger[i].id = id;
    finger[i].role = r;
    finger[i].x = finger[i].now_x = x;
    finger[i].y = finger[i].now_y = y;
    finger[i].at = updates;
    switch (r) {
    case ROLE_WHEEL:
        wheel_x = x;
        wheel_y = y;
        steer(0);
        break;
    case ROLE_BRAKE:
        held |= PAD_A;
        break;
    case ROLE_BOOST:
        held |= PAD_Y;
        break;
    default:
        break;
    }
}

static void moved(int i, float x, float y)
{
    float w, h, pt, top, range, raw;

    window(&w, &h, &pt, &top);
    finger[i].now_x = x;
    finger[i].now_y = y;
    if (hypotf(x - finger[i].x, y - finger[i].y) > k_tap_points * pt)
        gesture_moved = 1;
    if (finger[i].role != ROLE_WHEEL)
        return;
    range = k_steer_range * pt;
    raw = x - wheel_x;
    if (raw > range) {
        wheel_x = x - range;
        raw = range;
    } else if (raw < -range) {
        wheel_x = x + range;
        raw = -range;
    }
    steer(raw / range);
}

static void ended(int i)
{
    float w, h, pt, top, dx, dy, dist, x = finger[i].now_x, y = finger[i].now_y;
    Role r = finger[i].role;

    window(&w, &h, &pt, &top);
    finger[i].used = 0;
    switch (r) {
    case ROLE_WHEEL:
        steer(0);
        break;
    case ROLE_BRAKE:
        if (!has(ROLE_BRAKE))
            held &= (uint16_t)~PAD_A;
        break;
    case ROLE_BOOST:
        if (!has(ROLE_BOOST))
            held &= (uint16_t)~PAD_Y;
        break;
    case ROLE_PAUSE:
        if (on_pause(x, y))
            press(PAD_SELECT | PAD_START);
        break;
    case ROLE_VIEW:
        if (on_view(x, y))
            race_view_toggle();         /* the race's view switched (race/overhead.c) */
        break;
    case ROLE_MENU:
        if (gesture_fingers != 1)
            break;
        dx = x - finger[i].x;
        dy = y - finger[i].y;
        dist = hypotf(dx, dy);
        if (dist <= k_tap_points * pt && updates - finger[i].at <= k_tap_updates)
            press(PAD_B | PAD_START);   /* confirm: B or Start, as every menu takes */
        else if (dist >= k_swipe_points * pt)   /* the content follows the finger */
            press(fabsf(dx) >= fabsf(dy) ? (dx < 0 ? PAD_RIGHT : PAD_LEFT) : (dy < 0 ? PAD_DOWN : PAD_UP));
        break;
    default:
        break;
    }
    if (!fingers_down() && !gesture_moved && gesture_fingers == 2 && updates - gesture_at <= k_tap_updates + 9)
        press(driving ? (uint16_t)(PAD_SELECT | PAD_START) : PAD_Y);     /* the pause; in the menus, back */
}

static void touch_reset(void)
{
    int i;

    for (i = 0; i < FINGERS; i++)
        finger[i].used = 0;
    held = 0;
    pulse = 0;
    steer(0);
    gas = 0;
}

static uint16_t touch_pad(const nshvc_input *in)
{
    int i, t, n = in->touches < FINGERS ? in->touches : FINGERS;

    if (n)
        touched = 1;
    if (driving_marked != driving) {
        driving = driving_marked;
        touch_reset();
    }
    driving_marked = 0;
    /* fingers gone, fingers moved, fingers new */
    for (i = 0; i < FINGERS; i++) {
        int still = 0;

        if (!finger[i].used)
            continue;
        for (t = 0; t < n; t++)
            if (in->touch[t].id == finger[i].id) {
                still = 1;
                moved(i, in->touch[t].x, in->touch[t].y);
            }
        if (!still)
            ended(i);
    }
    for (t = 0; t < n; t++) {
        int known = 0;

        for (i = 0; i < FINGERS; i++)
            known |= finger[i].used && finger[i].id == in->touch[t].id;
        if (!known)
            began(in->touch[t].id, in->touch[t].x, in->touch[t].y);
    }
    gas = driving && !has(ROLE_BRAKE) && (option_auto_accelerate() || has(ROLE_WHEEL));
    if (!touched)
        return 0;
    return (uint16_t)(held | (gas ? PAD_B : 0) | (updates < pulse_until ? pulse : 0));
}

/* ---- a check's own driving (SRFX_AUTOPILOT=N, headless test runs) ----
   B held and steered at the course's segment table (its x, z words), a few
   waypoints past the nearest (SRFX_AUTOPILOT_AHEAD, 2), until the nearest
   is segment N; then nothing pressed. Off unless the variable is set. */
static void autopilot_pad(uint16_t *pad)
{
    static int on = -1;
    uint16_t rec, table;
    uint8_t bank;
    int n, i, best = 0, want, diff, ahead;
    double px, pz, bd = 1e30, tx, tz;

    {
        /* SRFX_FINISH (tests): the player's lap count set to 3 every 50
           updates, as the checks' finish script pokes $70:1D52 */
        static int finish = -1, count;
        if (finish < 0)
            finish = getenv("SRFX_FINISH") ? atoi(getenv("SRFX_FINISH")) : 0;
        ++count;
        if (finish == 1 && fx16(FXIN_PLAYER1_RECORD) && !(count % 50))
            fx16_set(0x1D52, 3);
        if (getenv("SRFX_NOTIME") && !(count % 20))     /* (tests: the time kept at 50, as the checks' scripts poke $0E70/$0E78) */
            g.time_left = g.time_left_shown = 0x50;
        if (finish >= 2 && fx16(FXIN_PLAYER1_RECORD) && count == 50)   /* (2, 3: that many laps once, the rest driven to the line) */
            fx16_set(0x1D52, (uint16_t)finish);
    }
    if (on < 0)
        on = getenv("SRFX_AUTOPILOT") ? atoi(getenv("SRFX_AUTOPILOT")) : 0;
    if (!on)
        return;
    rec = fx16(FXIN_PLAYER1_RECORD);
    table = fx16(0x1092);
    bank = (uint8_t)fx16(0x1094);
    if (!rec || table < 0x8000)
        return;
    n = cart16(bank, table);
    if (!n)
        return;
    px = (int16_t)rec_get(rec, FXREC_POS_X);
    pz = (int16_t)rec_get(rec, FXREC_POS_Z);
    for (i = 0; i < n; i++) {
        double dx = (int16_t)cart16(bank, (uint16_t)(table + 2 + i * 8)) - px;
        double dz = (int16_t)cart16(bank, (uint16_t)(table + 6 + i * 8)) - pz;
        if (dx * dx + dz * dz < bd)
            bd = dx * dx + dz * dz, best = i;
    }
    if (best + 1 >= on) {
        *pad = 0;
        return;
    }
    ahead = getenv("SRFX_AUTOPILOT_AHEAD") ? atoi(getenv("SRFX_AUTOPILOT_AHEAD")) : 2;
    i = (best + ahead) % n;
    tx = (int16_t)cart16(bank, (uint16_t)(table + 2 + i * 8));
    tz = (int16_t)cart16(bank, (uint16_t)(table + 6 + i * 8));
    want = (int)lround(atan2(tx - px, tz - pz) * 128 / M_PI) & 0xFF;
    /* the record keeps the heading the other way round (0x40: toward -x) */
    diff = (int8_t)(want + (rec_get(rec, FXREC_HEADING) >> 8));
    *pad = (uint16_t)((*pad & ~(PAD_B | PAD_LEFT | PAD_RIGHT)) | PAD_B | (diff > 4 ? PAD_RIGHT : diff < -4 ? PAD_LEFT : 0));   /* (other buttons as given: a test's Start) */
}

/* ---- each update ---- */

void controls_read(const nshvc_input *in, uint16_t pad[2])
{
    int i, keys = 0, c0 = -1, c1 = -1;
    uint16_t now, kb;

    /* the players' controllers: the first two connected, whatever their slots */
    for (i = 0; i < 8; i++)
        if (in->controller[i].on) {
            if (c0 < 0)
                c0 = i;
            else if (c1 < 0)
                c1 = i;
        }
    now = c0 >= 0 ? in->controller[c0].buttons : 0;
    updates++;
    for (i = 0; i < 8; i++)
        keys |= in->keys_pressed[i] != 0;
    if (keys)
        source = CONTROLS_KEYBOARD;
    if (c0 >= 0 && ((now & ~ctl_last) || fabsf(in->controller[c0].lx) > 0.5f || fabsf(in->controller[c0].ly) > 0.5f))
        source = CONTROLS_CONTROLLER;
    ctl_last = now;
    if (in->touches)
        source = CONTROLS_TOUCH;
    /* no controller: the keyboard is player 1; one: it is player 1 and the
       keyboard player 2; two: the second is player 2 (the keyboard player 1's too) */
    kb = keyboard_pad(in);
    pad[0] = (uint16_t)((c0 >= 0 ? controller_pad(in, c0, ctl_of) : 0) | touch_pad(in));
    pad[1] = c1 >= 0 ? controller_pad(in, c1, k_ctl_default) : 0;
    if (c0 >= 0 && c1 < 0)
        pad[1] |= kb;
    else
        pad[0] |= kb;
    autopilot_pad(&pad[0]);
}

/* ---- the touch controls' pictures ---- */

/* drawn once, DENSITY pixels a point: the steering wheel (a gold rim, a hub
   and three spokes), the two pedals (a ribbed pad with its name) and the
   pause button (two bars); shapes by their distance from each pixel's
   centre, a pixel's edge blended across one pixel */
#define DENSITY 3

typedef struct {
    uint32_t *px;
    int w, h;
} Img;

static Img img_new(float wpt, float hpt)
{
    Img m;

    m.w = (int)(wpt * DENSITY);
    m.h = (int)(hpt * DENSITY);
    m.px = (uint32_t *)calloc((size_t)m.w * (size_t)m.h, 4);
    return m;
}

/* colour c (0xRRGGBB) at coverage a over what is there */
static void blend(Img *m, int x, int y, uint32_t c, float a)
{
    uint32_t *p = &m->px[(size_t)y * (size_t)m->w + (size_t)x], d = *p, out = 0;
    float da = (float)(d >> 24) / 255.0f, oa;
    int k;

    if (a <= 0)
        return;
    if (a > 1)
        a = 1;
    oa = a + da * (1 - a);
    for (k = 0; k < 3; k++) {
        float s = (float)(c >> (16 - 8 * k) & 0xFF), t = (float)(d >> (16 - 8 * k) & 0xFF);
        float v = oa > 0 ? (s * a + t * da * (1 - a)) / oa : 0;
        out |= (uint32_t)(v + 0.5f) << (16 - 8 * k);
    }
    *p = out | (uint32_t)(oa * 255 + 0.5f) << 24;
}

static float cover(float d) { return d < -0.5f ? 1 : d > 0.5f ? 0 : 0.5f - d; }

static float d_circle(float px, float py, float cx, float cy, float r) { return hypotf(px - cx, py - cy) - r; }

static float d_segment(float px, float py, float ax, float ay, float bx, float by)
{
    float vx = bx - ax, vy = by - ay, t = ((px - ax) * vx + (py - ay) * vy) / (vx * vx + vy * vy);

    t = t < 0 ? 0 : t > 1 ? 1 : t;
    return hypotf(px - ax - t * vx, py - ay - t * vy);
}

static float d_roundrect(float px, float py, float x, float y, float w, float h, float r)
{
    float qx = fabsf(px - (x + w / 2)) - (w / 2 - r), qy = fabsf(py - (y + h / 2)) - (h / 2 - r);

    return hypotf(fmaxf(qx, 0), fmaxf(qy, 0)) + fminf(fmaxf(qx, qy), 0) - r;
}

/* a shape over the whole picture: dist(point), in points, and its paint */
#define SHAPE(m, c, alpha, expr)                                                              \
    do {                                                                                      \
        int x_, y_;                                                                           \
        for (y_ = 0; y_ < (m)->h; y_++)                                                       \
            for (x_ = 0; x_ < (m)->w; x_++) {                                                 \
                float px = (x_ + 0.5f) / DENSITY, py = (y_ + 0.5f) / DENSITY;                 \
                (void)px, (void)py;                                                           \
                blend((m), x_, y_, (c), (alpha) * cover((expr) * DENSITY));                   \
            }                                                                                 \
    } while (0)

/* 5 x 7 capitals for the pedals' names */
static const struct {
    char c;
    uint8_t row[7];
} k_font[] = {
    { 'A', { 0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11 } }, { 'B', { 0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E } },
    { 'E', { 0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F } }, { 'K', { 0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11 } },
    { 'O', { 0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E } }, { 'R', { 0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11 } },
    { 'S', { 0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E } }, { 'T', { 0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04 } },
};

/* text centred on (cx, top), each font pixel a square of `dot` points */
static void text(Img *m, const char *s, float cx, float top, float dot, uint32_t c)
{
    int n = (int)strlen(s), i, r, b;
    float x0 = cx - (float)(n * 6 - 1) * dot / 2;

    for (i = 0; i < n; i++) {
        const uint8_t *g = NULL;
        size_t k;

        for (k = 0; k < sizeof k_font / sizeof k_font[0]; k++)
            if (k_font[k].c == s[i])
                g = k_font[k].row;
        if (!g)
            continue;
        for (r = 0; r < 7; r++)
            for (b = 0; b < 5; b++)
                if (g[r] >> (4 - b) & 1) {
                    float x = x0 + (float)(i * 6 + b) * dot, y = top + (float)r * dot;

                    SHAPE(m, c, 1, d_roundrect(px, py, x, y, dot, dot, 0));
                }
    }
}

static Img wheel_picture(void)
{
    const float size = 160, k = size / 100;
    static const float s[3][4] = { { 50, 40, 50, 10 }, { 59, 56, 84, 70 }, { 41, 56, 16, 70 } };
    const uint32_t gold = 0xF5D34A;
    Img m = img_new(size, size);
    int i;

    /* the glow: the rim's, faded over 8 points */
    SHAPE(&m, gold, 0.45f * fmaxf(0, 1 - fabsf(d_circle(px, py, 50 * k, 50 * k, 44 * k)) / (4.5f * k + 8)), -1.0f);
    SHAPE(&m, gold, 1, fabsf(d_circle(px, py, 50 * k, 50 * k, 44 * k)) - 4.5f * k);
    SHAPE(&m, gold, 1, d_circle(px, py, 50 * k, 50 * k, 11 * k));
    for (i = 0; i < 3; i++)
        SHAPE(&m, gold, 1, d_segment(px, py, s[i][0] * k, s[i][1] * k, s[i][2] * k, s[i][3] * k) - 3.5f * k);
    return m;
}

static Img pedal_picture(float w, float h, uint32_t c, const char *name)
{
    Img m = img_new(w, h);
    int i;

    SHAPE(&m, 0x141414, 0.55f, d_roundrect(px, py, 4, 4, w - 8, h - 8, 16));
    SHAPE(&m, c, 1, fabsf(d_roundrect(px, py, 4, 4, w - 8, h - 8, 16)) - 2);
    for (i = 0; i < 4 && 22 + i * 16 < h - 40; i++)
        SHAPE(&m, c, 1, d_segment(px, py, 22, 22 + (float)i * 16, w - 22, 22 + (float)i * 16) - 2.5f);
    text(&m, name, w / 2, h - 32, 1.5f, c);
    return m;
}

static Img pause_picture(void)
{
    Img m = img_new(56, 44);

    /* the shadow, then the bars */
    SHAPE(&m, 0x000000,
          0.6f * fmaxf(0, 1 - fmaxf(0, fminf(d_roundrect(px, py, 17, 10, 8, 24, 2), d_roundrect(px, py, 31, 10, 8, 24, 2))) / 3),
          -1.0f);
    SHAPE(&m, 0xFFFFFF, 1, fminf(d_roundrect(px, py, 17, 10, 8, 24, 2), d_roundrect(px, py, 31, 10, 8, 24, 2)));
    return m;
}

/* the view button: what it switches to (to_overhead: a cube seen from
   above; else a road going away), the corner button's glyphs
   (pause/pause_art.c) in white with the pause button's shadow, 1.1 of
   their points a point about the picture's middle */
static Img view_picture(int to_overhead)
{
    Img m = img_new(56, 44);
    int k;

#define VX(px) (((px) - 28) / 1.1f + 20)
#define VY(py) (((py) - 22) / 1.1f + 20)
    if (to_overhead) {
        static const float shade[3] = { 1, 0.72f, 0.5f };

        SHAPE(&m, 0x000000,
              0.6f * fmaxf(0, 1 - fmaxf(0, fminf(fminf(pause_poly_distance(VX(px), VY(py), pause_view_cube[0], 4),
                                                       pause_poly_distance(VX(px), VY(py), pause_view_cube[1], 4)),
                                                 pause_poly_distance(VX(px), VY(py), pause_view_cube[2], 4)) * 1.1f) / 3),
              -1.0f);
        for (k = 0; k < 3; k++)
            SHAPE(&m, 0xFFFFFF, shade[k], (pause_poly_distance(VX(px), VY(py), pause_view_cube[k], 4) + 0.6f) * 1.1f);
    } else {
        SHAPE(&m, 0x000000,
              0.6f * fmaxf(0, 1 - fmaxf(0, pause_poly_distance(VX(px), VY(py), pause_view_road, 4) * 1.1f) / 3), -1.0f);
        SHAPE(&m, 0xFFFFFF, 1, pause_poly_distance(VX(px), VY(py), pause_view_road, 4) * 1.1f);
        for (k = 0; k < 3; k++)
            SHAPE(&m, 0x000000, 0.55f,
                  (d_segment(VX(px), VY(py), pause_view_dash[k][0], pause_view_dash[k][1], pause_view_dash[k][2],
                             pause_view_dash[k][3]) - pause_view_dash[k][4]) * 1.1f);
    }
#undef VX
#undef VY
    return m;
}

/* the overlays, the lowest ids (the pause screen's start at 12) */
enum { O_WHEEL, O_BRAKE, O_BOOST, O_PAUSE, O_VIEW, O_COUNT };
static nshvc_image picture[O_COUNT];
static float picture_w[O_COUNT], picture_h[O_COUNT];   /* in points */
static nshvc_image view_pictures[2];    /* the view button's: to the view behind the car, to the one from above */

static void pictures_make(void)
{
    Img m[O_COUNT];
    int i;

    m[O_WHEEL] = wheel_picture();
    m[O_BRAKE] = pedal_picture(84, 128, 0xE84A3C, "BRAKE");
    m[O_BOOST] = pedal_picture(84, 112, 0x4AA8F5, "BOOST");
    m[O_PAUSE] = pause_picture();
    m[O_VIEW] = view_picture(1);
    for (i = 0; i < O_COUNT; i++) {
        picture[i] = nshvc_image_create((uint32_t)m[i].w, (uint32_t)m[i].h, NSHVC_IMAGE_RGBA);
        if (picture[i].id)
            nshvc_image_write(picture[i], m[i].px);
        picture_w[i] = (float)m[i].w / DENSITY;
        picture_h[i] = (float)m[i].h / DENSITY;
        free(m[i].px);
    }
    view_pictures[1] = picture[O_VIEW];
    {
        Img v = view_picture(0);

        view_pictures[0] = nshvc_image_create((uint32_t)v.w, (uint32_t)v.h, NSHVC_IMAGE_RGBA);
        if (view_pictures[0].id)
            nshvc_image_write(view_pictures[0], v.px);
        free(v.px);
    }
}

/* picture i centred on (cx, cy) at `scale` points a point, turned by `angle`
   radians, or off */
static void place(int i, float cx, float cy, float pt, float scale, float angle, float alpha, int on)
{
    static const nshvc_image none;
    float hw = picture_w[i] * pt * scale / 2, hh = picture_h[i] * pt * scale / 2;
    float c = cosf(angle), s = sinf(angle), corners[6] = { 0, 0, 0, 0, 0, 0 };

    if (!on || !picture[i].id) {
        nshvc_overlay_set((uint32_t)i, none, corners, 0);
        return;
    }
    /* top left, top right, bottom left */
    corners[0] = cx + (-hw) * c - (-hh) * s;
    corners[1] = cy + (-hw) * s + (-hh) * c;
    corners[2] = cx + hw * c - (-hh) * s;
    corners[3] = cy + hw * s + (-hh) * c;
    corners[4] = cx + (-hw) * c - hh * s;
    corners[5] = cy + (-hw) * s + hh * c;
    nshvc_overlay_set((uint32_t)i, picture[i], corners, alpha);
}

void controls_draw(void)
{
    float w, h, pt, top, x, y;
    int i, brake = has(ROLE_BRAKE), boost = has(ROLE_BOOST);

    if (!touched)
        return;
    if (!picture[O_WHEEL].id)
        pictures_make();
    window(&w, &h, &pt, &top);
    for (i = 0; i < FINGERS && !(finger[i].used && finger[i].role == ROLE_WHEEL); i++)
        ;
    place(O_WHEEL, wheel_x, wheel_y, pt, 1, wheel_turn * k_wheel_degrees * 3.14159265f / 180, 0.9f,
          driving && i < FINGERS);
    x = roundf(w * k_brake_zone * 0.5f);
    place(O_BRAKE, x, roundf(h * 0.7f), pt, brake ? 0.94f : 1, 0, brake ? 0.85f : 0.4f, driving);
    place(O_BOOST, x, roundf(h * 0.27f), pt, boost ? 0.94f : 1, 0, boost ? 0.85f : 0.4f, driving);
    pause_place(&x, &y);
    place(O_PAUSE, x, y, pt, 1, 0, 0.75f, 1);
    picture[O_VIEW] = view_pictures[!race_view_overhead()];    /* what it switches to */
    view_place(&x, &y);
    place(O_VIEW, x, y, pt, 1, 0, race_view_moving() ? 0.4f : 0.75f, view_shown());
}

void controls_hide(void)
{
    int i;

    for (i = 0; i < O_COUNT; i++)
        place(i, 0, 0, 0, 0, 0, 0, 0);
}
