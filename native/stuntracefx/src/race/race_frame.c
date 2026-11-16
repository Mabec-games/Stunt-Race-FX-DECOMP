/*
 * race_frame.c: a race step and the race's start.
 */
#include <string.h>

#include "audio/sound.h"
#include "data/race_tables.h"
#include "fx/fx.h"
#include "fx/fx_draw.h"
#include "hud/hud.h"
#include "menu/menu.h"
#include "object/object.h"
#include "pause/pause.h"
#include "race/race.h"
#include "save/save.h"
#include "system/chip.h"
#include "system/game.h"
#include "system/state.h"
#include "video/video.h"

/*
 * The steps' clock for the pictures drawn between steps (plan stage 10, the
 * fx session's in-between pictures): the kind of the last step that handed
 * a picture over, the update it was taken in, the updates a step covers
 * there, and how many such steps the screen has had since its set-up (each
 * has its state kept by fx_tween_step_end).
 */
enum { STEP_NONE, STEP_COURSE_MENU, STEP_ONE_PLAYER, STEP_TWO_PLAYERS };

static struct {
    int kind;
    uint64_t at;                        /* pause_game_updates() during that step's update */
    uint8_t pace;
    unsigned steps;
} step_clock;

/* A new screen's steps: none before them to draw between; the fx session's
   kept step ends dropped too. */
static void step_clock_restart(void)
{
    memset(&step_clock, 0, sizeof step_clock);
    fx_tween_reset();
    race_overhead_reset();
}

/* The end of a step that handed a picture over: the state and the pictures
   it drew kept for the pictures between (fx_tween), then the clock. A step
   with no picture (RaceStart's two) keeps nothing. */
static void step_end(int kind, uint8_t pace)
{
    if (kind == STEP_NONE)
        return;
    fx_tween_step_end();
    step_clock.steps++;
    step_clock.kind = kind;
    step_clock.at = pause_game_updates();
    step_clock.pace = pace;
}

/* Whether this update ran a step that handed a picture over (for video's
   sprites kept at the steps' ends, video/between.c). */
int race_step_updated(void)
{
    return step_clock.kind != STEP_NONE && step_clock.at == pause_game_updates();
}

/* How far a picture drawn now lies from the last step's end to the next:
   (updates since that step + between) / the step's updates, in [0, 1]. The
   updates are the game's own (pause_game_updates: the engine's less those
   the pause screen held), so a pause stops the clock. The engine counts an
   update once update() returns, so in a draw after the step's own update
   the count is one past `at`. */
float race_step_progress(float between)
{
    float t;

    if (step_clock.kind == STEP_NONE)
        return 0.0f;
    if (step_clock.steps < 2)
        return 1.0f;                    /* the screen's first step: nothing before it */
    t = ((float)((int64_t)(pause_game_updates() - step_clock.at) - 1) + between) / (float)step_clock.pace;
    return t < 0.0f ? 0.0f : t > 1.0f ? 1.0f : t;
}

/*
 * @routine 03:B7C5 RaceFrameStep
 * WHAT IT DOES: One step of the race the course menu shows: the frames
 * since the last one go to frames_last_step; the objects run; the camera
 * moves on (+$37 += $400, CameraFollowStep) and the tied objects face it;
 * then the view, the physics program with the object work, and the view
 * update. Unless step_only is set it then draws the screen and flips it.
 * The original waited here for the vertical blank to take the last
 * picture; here the caller paces the steps. This path's screen base never
 * changes, so its drawing always goes into picture 0.
 */
void RaceFrameStep(int step_only)
{
    Camera *cam = &cameras[0];

    g.frames_last_step = g.frames_since_step;
    g.frames_since_step = 0;
    g.draw_side = 0;
    fx16_set(GSU_VIEW_246C, 0xFFFF);
    ObjectsRunHandlers();
    cam->word37 = (uint16_t)(cam->word37 + 0x0400);
    CameraFollowStep(cam);
    ObjectFaceTied(cam);
    ViewFromRecord(cam);
    FramePhysicsAndObjects();
    ViewUpdateAndSort();
    if (!step_only) {
        fx_draw_begin(0);
        FramePicture();
        fx_draw_flush();
        GsuScreenFlip();
        g.picture_uploaded = 0;
    }
    step_end(step_only ? STEP_NONE : STEP_COURSE_MENU, RACE_MENU_STEP_UPDATES);
}

/*
 * @routine 03:B5DF RaceStart
 * WHAT IT DOES: Starts the race behind the course menu: the chip's screen
 * mode, not the two-player split, the object pools and the view defaults,
 * the screen start, the chip's setup, the course script at $05:8000, two
 * steps not shown, the camera's target, and the chip's RAM above the
 * screen cleared. The chip's clock, config and screen base (CLSR, CFGR,
 * SCBR) need nothing here.
 */
void RaceStart(void)
{
    g.fx_screen_mode = 3;
    g.split_screen = 0;
    fx16_set(FXIN_SHADE_BASE, FX_SHADE_BASE_RACE);
    ObjectPoolInit();
    ViewDefaults();
    fx16_set(GSU_SCREEN_START, FX_PICTURE_0);
    g.screen_start = FX_PICTURE_0;
    fx16_set(GSU_WORD_1C4, 2);
    fx16_set(FXIN_NOT_MENU, 0);
    FxInit();
    g.demo_car_count = 0;
    step_clock_restart();
    CourseScriptRun(course_menu_script());
    RaceFrameStep(1);
    RaceFrameStep(1);
    cameras[0].object = g.demo_cars[0];
    g.picture_uploaded = 1;
    memset(&fx_ram[FX_PICTURE_0], 0, FX_PICTURE_0_CLEARED_END - FX_PICTURE_0);
}

/* The camera a car's links lead to: the object two links on names one. */
static Camera *view_camera(uint16_t car)
{
    const Object *first = obj_at(obj_at(car)->link);

    /* At a race's first step the car is not linked yet (its link is 0), and
       the original takes the record at work RAM $0000 as the camera; the
       objects' camera_at_link builds it from the named state left there. */
    return camera_at_link(first->link);
}

/* The frame's first part: the physics and the timers, the side's flag set. */
static void frame_side0(void)
{
    g.draw_side = 0;
    fx16_set(GSU_VIEW_246C, 0xFFFF);
    FramePhysicsAndObjects();
    FxTimersProgram();
}

/* The step's clock: the frames since the last step taken, and what the
   original kept as time counters; the Super FX is told the frames too. */
static void view_words_follow(void);

static void frame_start(void)
{
    g.frames_last_step = RACE_STEP_UPDATES;
    g.frames_since_step = 0;
    g.frame_stage_00 = 0x02;
    view_words_follow();
}

/* The original also handed the renderer the step's frames ($70:108E), and
   kept $1F3E and a steps-per-second count ($06D5-$06D7): nothing reads any
   of them (no job reads $108E in the five recordings, nor any code), so
   none is kept. */

/*
 * @routine 7E:E303 HudFrameWhileGsuRuns
 * WHAT IT DOES: The HUD's share of a frame while the chip draws: the slide
 * and the fade value; then the HUD digits and backdrop rows, or for two
 * players the backdrop's scroll and the player's value.
 */
static void hud_frame_while_gsu_runs(void)
{
    SlideStep();
    FadeValueStep();
    if (g.split_screen) {
        BackdropScrollFromView();
        if (g.fx_buffer == 1)
            HudDrawValueC();
        else
            HudDrawValueB();
        return;
    }
    if (g.hud_mode != 1 && g.hud_mode != 3)
        HudDigitSprites62();
    if (g.hud_mode != 3)
        HudStuntScoreDigits();
    BackdropRowsUpdate();
    HudDrawValueA();
    HudColourStep();
}

/*
 * @routine 7E:E258 FrameRunGsuAB13B
 * WHAT IT DOES: A frame for panel B: the parameters, the frame program, the
 * HUD's work, the gauges, on two players the objects, the map markers.
 */
static void frame_picture_panel_b(void)
{
    GsuParamsUpdate();
    (void)FxFrameProgram(0);
    hud_frame_while_gsu_runs();
    HudGaugesPanelB();
    if (g.split_screen) {
        BucketTriggersFromView();
        ObjectDrawListBuild();
        BucketTriggersFixedArea();
        ObjectGroupsActivate();
        GsuRecordsUpload();
    }
    MapMarkersPanelB();
}

/* @routine 7E:E298 FrameRunGsuAB13A: a frame for panel A. */
static void frame_picture_panel_a(void)
{
    GsuParamsUpdate();
    (void)FxFrameProgram(0);
    hud_frame_while_gsu_runs();
    HudGaugesPanelA();
    MapMarkersPanelA();
}

/* The picture is handed over at the step's end. A step a pause holds ends
   here when it is resumed. */
static void one_player_finish(void)
{
    fx_draw_flush();
    if (race_flow_stopped())
        return;
    GsuScreenToggle();
    g.picture_uploaded = 0;
    step_end(STEP_ONE_PLAYER, RACE_STEP_UPDATES);
}

/*
 * @routine 02:D7CD RaceFrameRun
 * WHAT IT DOES: One step of a race: the view, the controls, the physics and
 * timers, the objects, the view again and the sort, the backdrop, the frame
 * picture with the HUD, the speed gauge and the near car's sound, the text or
 * the race's phases and HUD; then the picture is handed over.
 */
void RaceFrameRun(void)
{
    frame_start();
    ViewFromRecord(view_camera(g.player_car));
    PlayerControlsFrame();
    frame_side0();
    ObjectsRunHandlers();
    ViewUpdateAndSort();
    BackdropAnglesFromView();
    g.draw_side = 1;
    fx_draw_begin(g.fx_buffer);
    frame_picture_panel_b();
    SpeedGaugeUpdate(fx16(FXIN_PLAYER1_RECORD));
    NearCarSoundUpdate();
    if (fx16(FXIN_TEXT_NOTICE))
        FxTextProgram(0);
    else
        RaceSoundsAndHud();
    one_player_finish();
}

/* The rest of a step a pause held. */
void RaceFrameRunResume(void)
{
    RaceSoundsAndHud();
    one_player_finish();
}

/* From player 2's sounds to the step's end (a held step's too, resumed). */
static void two_players_second_half(void)
{
    RaceSoundsPlayer2();
    fx_draw_flush();
    if (race_flow_stopped())
        return;
    GsuScreenFlipMenu();
    g.picture_uploaded = 0;
    step_end(STEP_TWO_PLAYERS, RACE_STEP_UPDATES);
}

/* From player 1's sounds through player 2's picture. */
static void two_players_first_half(void)
{
    RaceSoundsPlayer1();
    fx_draw_flush();
    if (race_flow_stopped())
        return;
    GsuScreenFlipMenu();
    g.picture_uploaded = 0;
    ViewUpdateAndSort();
    BackdropAnglesFromView();
    g.backdrop_heading = (uint16_t)(g.backdrop_heading - 0x0140);
    fx_draw_begin(g.fx_buffer);
    frame_picture_panel_a();
    SpeedGauge2Update(fx16(FXIN_PLAYER2_RECORD));
    two_players_second_half();
}

/*
 * @routine 0B:FB26 RaceFrameRunTwoPlayers
 * WHAT IT DOES: One step of a two-player race: the objects, then each
 * player's view, controls and picture in turn.
 */
void RaceFrameRunTwoPlayers(void)
{
    frame_start();
    /* The original ran the objects (the cameras among them) first, then the
       controls and the cars' physics, so each player's camera followed where
       its car had been a step before: a step (6 updates) more lag from a
       button to the picture than one player's order, controls, physics,
       objects (RaceFrameRun). Here the two-player step takes that order too. */
    ViewFromRecord(view_camera(g.player_car));
    PlayerControlsFrame();
    frame_side0();
    ObjectsRunHandlers();
    ViewFromRecord(view_camera(g.player_car));
    ViewUpdateAndSort();
    BackdropAnglesFromView();
    ViewFromRecord(view_camera(g.player2_car));
    g.draw_side = 1;
    fx_draw_begin(g.fx_buffer);
    frame_picture_panel_b();
    SpeedGaugeUpdate(fx16(FXIN_PLAYER1_RECORD));
    two_players_first_half();
}

/* The rest of a two-player step a pause held. */
void RaceFrameRunTwoPlayersResume(int second_half)
{
    if (second_half)
        two_players_second_half();
    else
        two_players_first_half();
}

/* The race's screen goes: no steps to draw between until the next set-up. */
void race_steps_end(void)
{
    step_clock_restart();
}

/* The draw list and its records, as FramePhysicsAndObjects and FrameRunGsuAB13B's
   object work make them, without the trigger lists and object groups. */
static void picture_objects(void)
{
    ObjectDrawListBuild();
    GsuRecordsUpload();
}

/*
 * The picture's part of the last step that handed a picture over, for the
 * pictures drawn between steps (plan stage 10): what that step had set when
 * it ran FxFrameProgram for `panel`, in the step's own order; the caller
 * then runs the frame program. Panel 0 is the step's first picture; panel 1
 * is a two-player step's second, and follows panel 0's frame program as in
 * the step (its view is the one panel 0 left: player 2's). Left out: the
 * physics, the controls, the objects' handlers, the trigger lists, the
 * object groups, the camera's own moves, and what the step draws after the
 * frame program (the text notice, the HUD's chip-drawn pieces). Returns 0
 * when the last step has no such panel.
 */
int race_one_player(void) { return step_clock.kind == STEP_ONE_PLAYER; }

int RaceViewForPicture(int panel)
{
    switch (step_clock.kind) {
    case STEP_COURSE_MENU:
        if (panel)
            return 0;
        g.draw_side = 0;
        fx16_set(GSU_VIEW_246C, 0xFFFF);
        ObjectFaceTied(&cameras[0]);
        ViewFromRecord(&cameras[0]);
        picture_objects();
        ViewUpdateAndSort();
        GsuParamsUpdate();
        return 1;
    case STEP_ONE_PLAYER:
        if (panel)
            return 0;
        ViewFromRecord(view_camera(g.player_car));
        race_overhead_view();           /* the view from above in the game's place, when chosen (overhead.c) */
        g.draw_side = 0;
        fx16_set(GSU_VIEW_246C, 0xFFFF);
        picture_objects();
        ViewUpdateAndSort();
        BackdropAnglesFromView();
        g.draw_side = 1;
        GsuParamsUpdate();
        return 1;
    case STEP_TWO_PLAYERS:
        if (panel == 0) {
            ViewFromRecord(view_camera(g.player_car));
            g.draw_side = 0;
            fx16_set(GSU_VIEW_246C, 0xFFFF);
            picture_objects();
            ViewUpdateAndSort();
            BackdropAnglesFromView();
            ViewFromRecord(view_camera(g.player2_car));
            g.draw_side = 1;
            GsuParamsUpdate();
            return 1;
        }
        picture_objects();
        ViewUpdateAndSort();
        BackdropAnglesFromView();
        g.backdrop_heading = (uint16_t)(g.backdrop_heading - 0x0140);
        GsuParamsUpdate();
        return 1;
    default:
        return 0;
    }
}

/* RaceFinishPhasesTail: $0E58 = $30, the mark and palette work cleared, sound 2. */
static void finish_tail(void)
{
    g.end_timer = 0x30;
    g.finish_mark = 0;
    g.palette_animate = 0;
    SoundPort0QueuePushRaw(0x02);
}

/*
 * @routine 07:A9D7 RaceFinishPhases0E, 07:AA0B RaceFinishPhases10
 * WHAT IT DOES: The race phases after a finish on the split screen: sound
 * $37; a player 2 finish sets player 1 $12 and player 2 `phase`; player 1's
 * sets both $14 when bit 0 of the other's flags is set, else its own `phase`
 * and player 2 $12.
 */
void RaceFinishPhases(int player, uint8_t phase)
{
    SoundPort0QueuePush(0x37);
    if (player != 0) {
        g.player_phase[0] = 0x12;
        g.player_phase[1] = phase;
    } else if (g.car_flags_low[1] & 0x01) {
        CarFinishSound();
        g.player_phase[0] = 0x14;
        g.player_phase[1] = 0x14;
    } else {
        g.player_phase[1] = 0x12;
        g.player_phase[0] = phase;
    }
    finish_tail();
}

/*
 * @routine 07:A99A RaceFinishSplit
 * WHAT IT DOES: A finish on the split screen, each player's phases and
 * sound.
 */
void RaceFinishSplit(int player)
{
    int one = 0;

    if (player == 0) {
        one = 1;
        if (g.laps_done[1] == 0x03)
            one = g.player_rank[0] == 0x01;
    }
    if (one) {
        CarFinishSound();
        g.player_phase[0] = 0x12;
        g.player_phase[1] = 0x0A;
    } else {
        CarFinishSoundP2();
        g.player_phase[1] = 0x12;
        g.player_phase[0] = 0x0A;
    }
    SoundPort0QueuePush(0x08);
    finish_tail();
}

/*
 * @routine 03:E838 RaceVarsReset
 * WHAT IT DOES: Resets the race's work variables before a race. The raster
 * interrupt's timers, the interrupt enable and BG3's scroll the original
 * wrote here have no meaning on the engine. Addresses with no reader converted yet are not stored: $0D9E,
 * $0DA0, $0DD8-$0DD9, $0E07, $0E08, $0E0B, $0E1A, $0E4F-$0E53, $0E54, $0E90,
 * $117E, $1180, the $7F:3462, $346A, $346E, $3466, $3476,
 * $345E halves.
 */
void RaceVarsReset(void)
{
    g.backdrop_row_adjust = g.course == 0x0A ? 0xFFF0 : 0x0000;
    g.fade_low = 0;
    g.fade_level = 0;
    g.clock_step = 0x0166;
    g.fx_buffer = 0;
    fx16_set(GSU_SCREEN_START, FX_PICTURE_0);
    g.screen_start = FX_PICTURE_0;
    g.clock_fraction = 0;
    g.clock_seconds = 0;
    g.clock_minutes = 0;
    g.overlay_state = 0;
    g.overlay_x = 0x009A;
    g.overlay_y = 0x006A;
    g.overlay_attr = 0;
    g.view_event[0] = 0;
    g.view_event[1] = 0;
    g.picture_effects = 0;
    g.player_palette_offset[1] = 0x37D7;
    palettes.cycle_phase[0] = palettes.cycle_phase[1] = palettes.cycle_phase[2] = 0;
    palettes.weather = 0;
    palettes.amount_shown = 0;
    palettes.weather_shown = 0;
    palettes.target_shown = 0xFFFF;
    palettes.gradient_was_off = 0xFFFF;
    palettes.blend_request = 0x7FFF;
    palettes.amount = 0x001F;
    g.speed_total[0] = g.speed_total[1] = 0;
    g.speed_count[0] = g.speed_count[1] = 0;
    g.hud_hold = 0;
    g.tally_state = 0;
    g.quit_cars_shown = 0;
    g.pause_choice = 0;
    g.start_light = 0;
    g.speed_gauge[0] = g.speed_gauge[1] = 0;     /* $0E49, $0E47: also what the sound sends on ports 3 and 2 */
    g.panel_top = 0;
    g.panel_bottom = 0;
    g.lap_mark[0] = g.lap_mark[1] = g.lap_mark[2] = 0;
    g.player_phase[0] = g.player_phase[1] = 0;
    g.status_level[0] = g.status_level[1] = 0;
    g.laps_done[0] = g.laps_done[1] = 0;
    g.stunt_seen = 0;
    g.laps_counted[0] = g.laps_counted[1] = 0;
    g.gauge_one[0] = g.gauge_one[1] = 0;
    g.car_flags_low[0] = g.car_flags_low[1] = 0;
    g.car_flags_high[0] = g.car_flags_high[1] = 0;
    g.player_rank[0] = 0x01;
    g.player_rank[1] = 0x01;
    g.frame_stage_00 = 0;
    g.split_stage_on = 0;
    g.split_stage = 0;
    g.pause_stage = 0;
    g.pause_stage_b = 0;
    g.pause_player = 0;
    g.end_panel_in = 0;
    g.lap_shown = 0;
    g.sprites_counted = 0;
    g.time_up_shown = 0;
    g.fade_out = 0;
    g.banner_timer[0] = g.banner_timer[1] = 0;
    g.banner[0] = g.banner[1] = 0;
    g.current_player = 0;
    g.end_ready = 0;
    g.kept_timer[0] = g.kept_timer[1] = 0;
    g.finish_mark = 0;
    g.finish_lines = 0;
    g.start_substate = 0;
    g.palette_animate = 0x17;
    g.freeze = 0xFF;
    line_effects_set(0, 0, 0, 1, 1, 0);     /* $1C: the sky and the window bands */
}

/*
 * @routine 7E:F6E9 ViewParamsByPlayers
 * WHAT IT DOES: The view's window and projection for one player or the
 * split screen.
 */
static void view_words_set(FxViewWords w)
{
    g.view_left = w.left;
    g.view_right = w.right;
    g.view_top = w.top;
    g.view_bottom = w.bottom;
    g.view_centre_x = w.centre_x;
    g.view_centre_y = w.centre_y;
    g.view_scale_34 = w.scale_34;
    g.view_scale_32 = w.scale_32;
}

static void view_params_by_players(void)
{
    if (g.split_screen && race_two_full_window()) {
        /* two players in the full window (plan stage 10): a player's
           rectangle (both are the same size), the horizon at its middle as
           the original's 72, 40 of 144 x 80 */
        int x, y, w, h;

        screen_two_rect(0, &x, &y, &w, &h);
        view_words_set(fx_view_words(0, 0, w, h, h / 2));
    } else if (g.split_screen) {
        g.view_left = 0;
        g.view_right = 0x008F;
        g.view_top = 0;
        g.view_bottom = 0x0050;
        g.view_centre_x = 0x0048;
        g.view_centre_y = 0x0028;
        g.view_scale_34 = 0x6666;
        g.view_scale_32 = 0x71C7;
    } else if (race_full_window()) {
        /* the full window (plan stage 10, step B): the whole screen, the
           horizon at the original's screen line 96 */
        const ScreenShape *s = screen_shape();

        view_words_set(fx_view_words(0, 0, s->width, s->height, 96 + s->origin_y));
    } else {
        g.view_left = 0;
        g.view_right = 0x00CF;
        g.view_top = 0;
        g.view_bottom = 0x0080;
        g.view_centre_x = 0x0068;
        g.view_centre_y = 0x0040;
        g.view_scale_34 = 0x4000;
        g.view_scale_32 = 0x4EC4;
    }
}

/* The view's words follow the screen at each step: the full window turned
   on or off, the window's shape changed. Only the renderer reads them (the
   frame program's projection and clips, the particles' drawing), so this
   changes no game state the race plays on. */
static void view_words_follow(void)
{
    uint16_t was[8] = { g.view_left, g.view_right, g.view_top, g.view_bottom,
                        g.view_centre_x, g.view_centre_y, g.view_scale_34, g.view_scale_32 };

    view_params_by_players();
    if (was[0] != g.view_left || was[1] != g.view_right || was[2] != g.view_top || was[3] != g.view_bottom
        || was[4] != g.view_centre_x || was[5] != g.view_centre_y || was[6] != g.view_scale_34
        || was[7] != g.view_scale_32)
        GsuParamsFromDp();
}

/* @routine 7E:F29F AttractViewStart: the race view's parameters, the setup tail. */
static void attract_view_start(void)
{
    fx16_set(FXIN_EFFECTS_SEED, 0x1234);
    fx16_set(GSU_WORD_1C4, 1);
    fx8_set(FXIN_NOT_MENU, g.split_screen);
    g.view_ef = 0xFF;
    g.frame_stage_00 = 0;
    view_params_by_players();
    ViewSetupTail();
}

/*
 * @routine 7E:F21E AttractCourseSetup
 * WHAT IT DOES: The race's course setup: the held buttons cleared, the view,
 * the sprites' first slot, the HUD loaded, the chip's table program.
 */
void AttractCourseSetup(void)
{
    step_clock_restart();
    frame_state.held[0] = 0;
    attract_view_start();
    (void)SpriteSlotFirst();
    RaceHudLoad();
    (void)FxTable2702Program();
}

/*
 * @routine 03:D8B3 RaceModeSetup
 * WHAT IT DOES: Sets the race up for its mode: in mode 3 with no class won
 * only marks the race as skipped and takes course $1B; otherwise clears the
 * race's flags, then mode 0 unpacks the course's records, the others set the
 * count, mode 2 loading the battle driver.
 */
void RaceModeSetup(void)
{
    if (g.hud_mode == 0x03) {
        if (!(save.progress & 0x07)) {
            g.race_skipped = 0xFF;
            g.course = 0x1B;
            g.course_high = 0;
            RaceModeSetupPickSound();
            return;
        }
        g.course_class = (save.progress & 0x01) ? 0x00 : 0x01;
    } else {
        g.course_class = 0;
    }
    g.race_skipped = 0;
    g.course_scroll_low = 0;
    RaceModeSetupPickSound();
    g.time_override = 0;
    g.hud_blank = 0;
    g.champ_stage = 0;
    g.course_high = 0;
    g.course_page = 0;
    g.records_panel_up = 0;
    memset(g.race_time, 0, sizeof g.race_time);
    g.every_course_open = 0;
    g.course_code_presses = 0;
    memset(g.level_columns, g.hud_mode ? 0x00 : 0x04, sizeof g.level_columns);
    g.time_shown = 0;
    g.time_shown_2 = 0;
    if (g.hud_mode == 0x00) {
        RecordsUnpackCourseRunning();
        g.tries_left = 0x02;
        return;
    }
    if (g.hud_mode == 0x02)
        RaceModeSetupBattleSound();
    g.tries_left = 0xFF;
}

/* @routine 08:B893 Set7EFFA6Sequence */
void PlaceEntriesInOrder(void)
{
    for (int i = 0; i < 4; i++)
        objects.place_entries[i] = (uint16_t)(i + 1);
}

/* @routine 07:EE11 Countdown0DF8: a try used at a race's end: one fewer left; with none, the
   counted sprites' count down instead. */
void TryUsed(void)
{
    if (g.tries_left) {
        g.tries_left--;
        return;
    }
    g.sprites_counted--;
    g.sprite_count = 0;
}
