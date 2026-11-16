/*
 * flow.c: the race from the menus' hand-over to its end: the session's
 * states, the work the original's raster interrupt did each video frame, the
 * race's pacing, the waits the code did inside a step and the attract demos.
 *
 * Each update is one video frame. A step of the race runs every 6 updates
 * (race_pace); between steps only the raster's work runs.
 */
#include <stdio.h>
#include <string.h>

#include "audio/sound.h"
#include "data/race_tables.h"
#include "fx/fx.h"
#include "fx/fx_draw.h"
#include "hud/hud.h"
#include "menu/menu.h"
#include "object/object.h"
#include "race/race.h"
#include "save/save.h"
#include "system/chip.h"
#include "system/game.h"
#include "system/state.h"
#include "video/picture.h"
#include "video/video.h"

/* The screen goes back to the menus: the race's per-frame work off and none
   of its layers left behind the next screen (video's race_screen_end). */
static void screen_handed_back(void)
{
    g.raster_chain = RASTER_NONE;
    race_steps_end();
    race_screen_end();
}

/* What the race is doing while no step runs. */
typedef enum {
    WAIT_NONE, WAIT_OVERLAY, WAIT_CHAMPION, WAIT_END_FADE, WAIT_END_COUNT,
    WAIT_SOFT_RESET, WAIT_PAUSE, WAIT_PAUSE_LOOP, WAIT_CHAMPION_RUN
} RaceWait;

typedef enum {
    FLOW_OFF, FLOW_RACE_SETUP, FLOW_RACE_LOAD, FLOW_COURSE, FLOW_VIDEO_INIT, FLOW_ATTRACT_SETUP,
    FLOW_RACE_RUN, FLOW_DEMO, FLOW_ENDING
} FlowState;

/* The demo's mode: its commands, or running to the fade's end. */
enum { DEMO_COMMANDS, DEMO_TO_FADE, DEMO_WAIT_FADE };

static struct {
    FlowState state;
    RaceWait wait;
    void (*after)(void);
    int stopped;                        /* the step in progress was left */
    int finished;                       /* the session is over; back to the menus */
    int resuming;                       /* a step is going on after a pause */
    uint8_t pace_count;
    /* the demo's script */
    uint16_t demo_at, demo_frame, demo_flags;
    int demo_mode;
    int demo_ended;                     /* the step a pause held ended the demo */
    int ending_at, ending_running;      /* the ending's demo and whether it has started */
} flow;

int race_flow_stopped(void) { return flow.stopped; }
int race_ending_running(void) { return flow.state == FLOW_ENDING; }

void race_wait_overlay(void (*after)(void))
{
    flow.wait = WAIT_OVERLAY;
    flow.after = after;
    flow.stopped = 1;
}

void race_wait_champion(void (*after)(void))
{
    flow.wait = WAIT_CHAMPION;
    flow.after = after;
    flow.stopped = 1;
}

/* @routine 07:DCAE RaceEndCountdownFadeOn, 07:DCDF RaceEndCountdownOn */
void race_go_on(void)
{
    if (g.race_again)
        RaceRestart();
    else
        RaceEndVideoReset();
}

/* @routine 07:DC88 RaceEndCountdownFade */
void RaceEndCountdownFade(void)
{
    flow.wait = WAIT_END_FADE;
    flow.stopped = 1;
}

/* @routine 07:DCCA RaceEndCountdown */
void RaceEndCountdown(void)
{
    flow.wait = WAIT_END_COUNT;
    flow.stopped = 1;
}

/* The race's own video-frame work, one player: the chain $06, $04, $14 in
   the order the frame sees it, the blank lines before the picture first. */
static void raster_one_player(void)
{
    race_frame_upload();
    RaceVideoUpload1P();
    pic.brightness = g.fade_level & 0x0F;
    pic.blank = (g.fade_level & 0x80) != 0;
    g.frame_count++;
    g.frames_since_step++;
    FadeStep();
    JoypadAccumulate();
    sound_frame();
    RaceClockStep();
    g.time_sprite_base = 0x01D8;
    HudDrawTime();
    HudPanelsStep();
    PalettesStep();
    RaceVideoWindows1P();
}

/* Two players: $0E, $08, $0A, $0C. */
static void raster_two_players(void)
{
    race_frame_upload();
    JoypadAccumulate();
    RaceVideoUpload2P();
    pic.brightness = g.fade_level & 0x0F;
    pic.blank = (g.fade_level & 0x80) != 0;
    g.frame_count++;
    g.frames_since_step++;
    FadeStep();
    RaceClockStep();
    g.time_sprite_base = 0x0100;
    HudDrawTime();
    HudPanelASlide();
    HudPanelBSlide();
    sound_frame();
    PaletteKeptColoursStep(1, 2);
    RaceVideoSplit2P();
    g.time_sprite_base = 0x0000;
    HudDrawTime();
    PaletteKeptColoursStep(0, 0);
    WindowSplitAnimate();
}

/* One video frame of the raster's work. */
static void raster_frame(void)
{
    switch (g.raster_chain) {
    case RASTER_ONE_PLAYER:
        raster_one_player();
        break;
    case RASTER_TWO_PLAYERS:
        raster_two_players();
        break;
    default:
        break;
    }
}

/* forward */
static void soft_reset_finish(void);

/* @routine 7E:A9B1, A9D3, A9FA, AA1C (the two-player rows stages): with the
   buffer as `want` the next stage, the rows up. Both players' pictures are
   in the engine's draw list (race_two_player_list), so only the stages'
   order is kept. */
static void two_player_rows(int want, uint8_t next)
{
    if ((g.fx_buffer != 0) != want)
        return;
    g.fx_upload_stage = next;
    g.picture_uploaded = 1;
}

/* 7E:AA91, AA9A: the map buffer's $9BD7 part to the half's 3D characters
   (0 the top, 1 the bottom), stage $10, colour 0 = $1CC4. */
static void split_picture(int half)
{
    race_half_characters_write(half, (const uint8_t *)(const void *)race_map_buffer() + 0x0800);
    race_half_blank_set(half, 1);       /* the full window's half: its 3D off too, as the characters are */
    g.split_stage = 0x10;
    palettes.buffer[0] = 0x1CC4;
    pic.palette[0] = 0x1CC4;            /* CGRAM's colour 0 written with it */
    g.palette_animate = 0;
}

/*
 * @routine 7E:AA4B (the split screen's stages, NmiFrameUpload with $0D22 set)
 * WHAT IT DOES: The results', championship's and pause's uploads by the stage
 * $0D23: the map rows, sprites, BG2's scroll and the palette; the results'
 * big picture; the turning picture; the menu's map; the car's picture.
 */
/* The car picture table's words are where the original kept the three art
   pictures in work RAM ($7F:63D7, race_pictures here). */
enum { RACE_PICTURES_ORIGINAL = 0x63D7 };

static void split_upload(void)
{
    switch (g.split_stage) {
    case 0x02:
        ResultsMapRowsUpload();
        /* fall through */
    case 0x08:
        OamDmaAll();
        Bg2ScrollFromTable(0);
        CgramUploadAll();
        break;
    case 0x04:
        race_results_picture_write(fx_ram + FX_RESULTS_PICTURE);
        g.split_stage = 0x00;
        break;
    case 0x0A:
        VramUploadGsuBuffer();
        OamDmaAll();
        /* fall through */
    case 0x06:
        CgramUploadAll();
        break;
    case 0x0C:
        split_picture(0);
        break;
    case 0x0E:
        split_picture(1);
        break;
    case 0x10:
        race_panel_map_write(race_map_buffer()); /* only after $0C and $0E: the split screen's quit menu */
        break;
    case 0x12:
        race_car_picture_write(race_pictures + (uint16_t)(g.overlay_picture - RACE_PICTURES_ORIGINAL));
        g.split_stage = 0x02;
        break;
    default:
        break;                          /* 0: nothing; past $12 not reached */
    }
}

/* The upload of the finished picture, three stages a picture: the race's
   part of the vertical blank's frame upload (NmiFrameUpload); the split
   screen's stages instead while $0D22 is set. Stage $1C (CgramUploadAll)
   and $1E (stage 6's copy) are never set by the game. */
void race_frame_upload(void)
{
    int shown;

    if (g.split_stage_on) {
        split_upload();
        return;
    }
    race_half_blank_set(0, 0);          /* the race's pictures uploaded again */
    race_half_blank_set(1, 0);
    switch (g.fx_upload_stage) {
    case 0x00:
        if (!g.fx_buffer) {
            NmiFrameUploadSprites();
            return;
        }
        g.fx_upload_stage = 0x02;
        return;
    case 0x02:
        g.fx_upload_stage = 0x04;
        g.picture_uploaded = 1;
        return;
    case 0x04:
        g.fx_upload_stage = 0x06;
        shown = 0;
        break;
    case 0x06:
        if (g.fx_buffer) {
            NmiFrameUploadSprites();
            return;
        }
        g.fx_upload_stage = 0x08;
        return;
    case 0x08:
        g.fx_upload_stage = 0x0A;
        g.picture_uploaded = 1;
        return;
    case 0x0A:
        g.fx_upload_stage = 0x00;
        shown = 1;
        break;
    case 0x0C: two_player_rows(1, 0x10); return;
    case 0x10: two_player_rows(0, 0x14); return;
    case 0x14: two_player_rows(1, 0x18); return;
    case 0x18: two_player_rows(0, 0x0C); return;
    default:
        return;                         /* $0E, $12, $16, $1A: nothing */
    }
    race_picture_shown(shown);
    g.frame_stage_00 = 0;
    g.oam_upload_wanted--;
}

/* One update of a wait; the race's steps resume when it is over. */
static void wait_step(void)
{
    uint8_t v;

    switch (flow.wait) {
    case WAIT_END_FADE:
        if (!g.finish_timer) {
            if (g.fade_level & 0x80) {
                flow.wait = WAIT_NONE;
                RaceEndLeave();
            }
            return;
        }
        v = --g.finish_timer;
        if (v)
            return;
        if (g.hud_mode) {
            g.results_step = 0;
            SoundPort0QueuePushRaw(0x30);
            race_wait_champion(race_go_on);
            return;
        }
        g.fade_out = 0x01;
        return;
    case WAIT_END_COUNT:
        if (--g.time_up_timer)
            return;
        g.results_step = 0;
        race_wait_champion(race_go_on);
        return;
    case WAIT_OVERLAY:
        if (g.fade_level & 0x80) {
            flow.wait = WAIT_NONE;
            if (flow.after)
                flow.after();
            return;
        }
        flow.stopped = 0;
        OverlayFrameStep();
        frame_state.pressed_copy[0] = frame_state.pressed_copy[1] = 0;
        frame_state.held[0] = frame_state.held[1] = 0;
        return;
    case WAIT_CHAMPION:
        flow.stopped = 0;
        if (ChampionshipScreenFrame()) {
            flow.wait = WAIT_NONE;
            if (flow.after)
                flow.after();
        }
        return;
    case WAIT_SOFT_RESET:
        if (--g.panel_delay)
            return;
        soft_reset_finish();
        return;
    case WAIT_PAUSE:
    case WAIT_PAUSE_LOOP:
        PauseFrameStep();
        return;
    case WAIT_CHAMPION_RUN:
        if (ChampionScreenRun()) {
            flow.wait = WAIT_NONE;
            if (flow.after)
                flow.after();
        }
        return;
    default:
        return;
    }
}

/* One step of the race. */
static void race_step(void)
{
    flow.stopped = 0;
    SpriteSlot1Countdown();
    if (g.split_screen)
        RaceFrameRunTwoPlayers();
    else
        RaceFrameRun();
}

/* forward */
static int demo_after_step(void);
static void ending_step(void);

void race_pause_begin(int leaves)
{
    flow.wait = leaves ? WAIT_PAUSE_LOOP : WAIT_PAUSE;
    flow.stopped = 1;
}

int race_resuming(void) { return flow.resuming; }
void race_resume_done(void) { flow.resuming = 0; }

/* The pause is over: the step it held goes on from there. */
void race_pause_over(int second_half)
{
    if (flow.wait != WAIT_PAUSE)
        return;
    flow.wait = WAIT_NONE;
    flow.stopped = 0;
    flow.resuming = 1;
    if (g.split_screen)
        RaceFrameRunTwoPlayersResume(second_half);
    else
        RaceFrameRunResume();
    flow.resuming = 0;
    if ((flow.state == FLOW_DEMO || flow.state == FLOW_ENDING) && !flow.stopped && demo_after_step())
        flow.demo_ended = 1;
}

static uint8_t race_pace(void)
{
    return RACE_STEP_UPDATES;
}

/* @routine 03:EC24 CourseDataPointer: the course script's start. */
static void course_script_start(void)
{
    CourseScriptRun(course_script(g.course));
}

/* @routine 03:8BF8 BootWarmAttract's start and BootWarm's end: the race's
   setup. Its clear of the chip's byte at $023A is left out: only a screen
   the game never shows reads that byte. */
static void attract_setup(void)
{
    WindowsInit();
    AttractCourseSetup();
    course_script_start();
    ObjectPlaceAll();
    g.object_frames = 0;
    FxInit();
    g.picture_uploaded = 1;
}

/* A launch straight into a course's race (race_launch_course): -1 none. */
static int launch_course = -1;

void race_launch_course(int course) { launch_course = course; }

/* the course screen's choice of the launch's course, as its zooms make it
   (CourseFromSelection, course_choose): Speed Trax's class and stage (from
   $14 the courses its time override picks by class), Stunt and Battle
   Trax's slot in their lists */
static void launch_choose(int course)
{
    uint16_t slot;

    g.course = (uint8_t)course;
    g.course_high = 0;
    if (g.hud_mode == 1 || g.hud_mode == 2) {
        for (slot = 0; slot < 4; slot++)
            if (course_in_list(g.hud_mode == 1 ? COURSE_LIST_STUNT : COURSE_LIST_BATTLE, slot) == course)
                g.champ_stage = (uint8_t)slot;
    } else if (course >= 0x14) {
        g.time_override = 1;
        g.course_class = (uint8_t)(course - 0x14);
    } else {
        g.course_class = (uint8_t)(course / 4);
        g.champ_stage = (uint8_t)(course % 4);
    }
}

/* @routine 03:8BCD BootWarmRace: the screen blanked, sound on, the course
   screen before the race (RaceLoop). A launch's course screen is begun (the
   race's load) and left at once, as its fade out leaves it, the course
   chosen as data. */
static void race_load(void)
{
    pic.blank = 1;
    sound_muted = 0;                    /* $0E4E */
    if (launch_course >= 0)
        launch_choose(launch_course);   /* the course as the race's load reads it (RaceVarsReset) */
    if (!CourseScreenBegin()) {
        flow.state = FLOW_VIDEO_INIT;
        return;
    }
    flow.state = FLOW_COURSE;
    if (launch_course >= 0) {
        launch_choose(launch_course);   /* again: the load clears Battle Trax's slot */
        launch_course = -1;
        g.raster_chain = RASTER_NONE;
        flow.state = FLOW_VIDEO_INIT;
    }
}

/* One update of the course screen: its blank lines, its loop's frame, then
   the work at line 55; once its fade out is done the race's video is set up. */
static void course_update(void)
{
    CourseScreenRasterBlank();
    if (flow.wait != WAIT_NONE) {
        wait_step();
    } else {
        flow.stopped = 0;
        if (CourseScreenUpdate()) {
            g.raster_chain = RASTER_NONE;
            pic.blank = 1;
            flow.state = FLOW_VIDEO_INIT;
            return;
        }
    }
    if (flow.state == FLOW_COURSE && !flow.finished)
        CourseScreenRasterLine55();
}

void RaceRestart(void)
{
    pic.blank = 1;
    flow.wait = WAIT_NONE;
    flow.stopped = 1;
    flow.state = FLOW_RACE_LOAD;
}

void RaceEndToAttract(void)
{
    pic.blank = 1;
    flow.wait = WAIT_NONE;
    flow.stopped = 1;
    flow.state = FLOW_ATTRACT_SETUP;
}

void RaceEndVideoReset(void)
{
    pic.blank = 1;
    flow.wait = WAIT_NONE;
    flow.stopped = 1;
    flow.state = FLOW_VIDEO_INIT;
}

/* @routine 07:EBE9 SoftResetToBootWarm: the sound data reloaded, back to the course menu. */
void SoftResetToBootWarm(void)
{
    pic.blank = 1;
    SoftResetToBootWarmSound();
    flow.wait = WAIT_NONE;
    flow.stopped = 1;
    flow.finished = 1;
    session_race_ended(1);
}

/* The champion's screen, a frame an update (ChampionScreenRun, the
   menus'), then `after`; the race's raster work is off meanwhile. */
void race_champion_screen(void (*after)(void))
{
    screen_handed_back();
    flow.wait = WAIT_CHAMPION_RUN;
    flow.after = after;
    flow.stopped = 1;
}

/* 07:EBDD RaceEndLeave's championship: the champion's screen, then SoftResetToBootWarm. */
void race_leave_to_champion(void)
{
    race_champion_screen(SoftResetToBootWarm);
}

/*
 * @routine 03:8C86 BootSoftRestart
 * WHAT IT DOES: The ending after the last class: the sound holds set
 * ($10DC = $FFFF, $10DE = 0), the three demos of DemoRunThird
 * (03:865E, 03:866A), then the game from its start.
 */
void BootSoftRestart(void)
{
    sound_hold_b = 0xFF;
    sound_alt_set_loaded = 0;
    flow.wait = WAIT_NONE;
    flow.stopped = 1;
    flow.state = FLOW_ENDING;
    flow.ending_at = 0;
    flow.ending_running = 0;
}

/* @routine 07:AADC SoftResetCheck's finish: the game restarted from the start. */
static void soft_reset_finish(void)
{
    pic.blank = 1;
    g.reset_state = 0;
    SoftResetApu();
    memset(&g, 0, sizeof g);
    memset(&objects, 0, sizeof objects);
    memset(fx_ram, 0, 0x2800);
    flow.wait = WAIT_NONE;
    flow.stopped = 1;
    flow.finished = 1;
    session_race_ended(0);
}

/*
 * @routine 07:AADC SoftResetCheck
 * WHAT IT DOES: The soft reset: player 1 holding L, R, START and SELECT and
 * nothing else starts the fade out and a wait of $70 frames, then restarts
 * the game.
 */
void SoftResetCheck(void)
{
    if (g.reset_state)
        return;
    if ((frame_state.held[0] & 0x3030) != 0x3030 || (frame_state.held[0] & 0xCFCF))
        return;
    g.reset_state = 0xFF;
    SoundPort1QueuePush(0x04);
    g.fade_out = 0x01;
    g.panel_delay = 0x70;
    flow.wait = WAIT_SOFT_RESET;
    flow.stopped = 1;
}

void race_session_begin(void)
{
    race_steps_end();                   /* the course menu's race is over */
    flow.state = FLOW_RACE_SETUP;
    flow.wait = WAIT_NONE;
    flow.stopped = 0;
    flow.finished = 0;
    flow.pace_count = 0;
}

/* One update of the race's session; nonzero when it hands back. */
int race_session_update(void)
{
    switch (flow.state) {
    case FLOW_RACE_SETUP:
        RaceModeSetup();
        PlaceEntriesInOrder();
        flow.state = FLOW_RACE_LOAD;
        /* fall through */
    case FLOW_RACE_LOAD:
        race_load();
        break;
    case FLOW_COURSE:
        course_update();
        break;
    case FLOW_ENDING:
        ending_step();
        break;
    case FLOW_VIDEO_INIT:
        RaceVideoInit();
        flow.state = FLOW_ATTRACT_SETUP;
        break;
    case FLOW_ATTRACT_SETUP:
        attract_setup();
        flow.state = FLOW_RACE_RUN;
        flow.pace_count = 0;
        break;
    case FLOW_RACE_RUN:
        if (flow.wait == WAIT_NONE)
            JoypadAutoAccelerate();     /* the player drives: the race reads their pad */
        raster_frame();
        if (flow.wait != WAIT_NONE) {
            wait_step();
        } else if (++flow.pace_count >= race_pace()) {
            flow.pace_count = 0;
            race_step();
        }
        break;
    default:
        break;
    }
    if (flow.finished)
        screen_handed_back();
    return flow.finished;
}

/* ---- the attract demos ---- */

/*
 * @routine 03:8683 DemoRun
 * WHAT IT DOES: One attract demo: the setup, then the script's commands
 * between race steps until it ends.
 */
static void demo_setup(uint16_t offset)
{
    uint16_t at = offset;

    g.course = (uint8_t)demo_script_word((uint16_t)(at + 2));
    flow.demo_flags = 0x0020;
    g.selection_handler[0] = demo_script_word(at);
    g.hud_mode = 0;
    PlaceEntriesInOrder();
    RaceVideoInit();
    WindowsInit();
    AttractCourseSetup();
    g.hud_mode = 3;
    course_script_start();
    ObjectPlaceAll();
    FxInit();
    g.picture_uploaded = 1;
    g.object_frames = 0;
    flow.demo_frame = 0;
    ObjectsRunHandlers();
    {
        uint16_t rec = fx16(FXIN_PLAYER1_RECORD);

        rec_or(rec, FXREC_FLAGS64, 0x0008);
    }
    fx16_set(FXIN_TEXT_NOTICE, 0);
    flow.demo_at = (uint16_t)(at + 3);
}

static int demo_fade_done(void) { return (g.fade_level & 0x80) != 0; }

static void demo_fade_out(void) { g.fade_out = 0x01; }

/* The camera moved: CameraFollowStep and ObjectFaceTied. */
static void demo_camera(void)
{
    CameraFollowStep(&cameras[0]);
    ObjectFaceTied(&cameras[0]);
}

/* The commands before a step; returns 1 when the demo ended. */
static int demo_commands(void)
{
    for (;;) {
        uint16_t at = flow.demo_at;
        uint8_t c = demo_script_byte(at);

        switch (c) {
        case 0x01:
            ObjectLinkedHandlerSet(obj_at(g.player_car), demo_script_word((uint16_t)(at + 1)));
            flow.demo_at = (uint16_t)(at + 3);
            continue;
        case 0x02:
            /* the text's address and its bank (the original's second word
               store, at $2733, puts the bank at $2734) */
            fx16_set(FXIN_TEXT_NOTICE, demo_script_word((uint16_t)(at + 1)));
            fx8_set(FX_TEXT_BANK, demo_script_byte((uint16_t)(at + 3)));
            flow.demo_at = (uint16_t)(at + 4);
            continue;
        case 0x03:
            if (demo_script_word((uint16_t)(at + 1)) != flow.demo_frame)
                return 0;
            flow.demo_at = (uint16_t)(at + 3);
            continue;
        case 0x05:
            flow.demo_at = (uint16_t)(at + 1);
            demo_fade_out();
            continue;
        case 0x06:
            if (!demo_fade_done())
                return 0;
            flow.demo_at = (uint16_t)(at + 1);
            continue;
        case 0x07:
            flow.demo_at = (uint16_t)(at + 1);
            windows.bands_kind = 0;
            windows.bands_state = 0x01;
            continue;
        case 0x08:
            if (windows.bands_state != 0x05)
                return 0;
            flow.demo_at = (uint16_t)(at + 1);
            continue;
        case 0x09:
            cameras[0].x = demo_script_word((uint16_t)(at + 1));
            cameras[0].z = demo_script_word((uint16_t)(at + 3));
            demo_camera();
            flow.demo_at = (uint16_t)(at + 5);
            continue;
        case 0x0A:
            cameras[0].y = (uint16_t)(cameras[0].y + demo_script_word((uint16_t)(at + 1)));
            demo_camera();
            flow.demo_at = (uint16_t)(at + 3);
            continue;
        case 0x0B:
            if (!(frame_state.held[0] >> 8))
                return 2;
            demo_fade_out();
            flow.demo_mode = DEMO_WAIT_FADE;
            return 0;
        case 0x0C:
            flow.demo_flags |= demo_script_word((uint16_t)(at + 1));
            flow.demo_at = (uint16_t)(at + 3);
            continue;
        case 0x0D:
            flow.demo_flags &= demo_script_word((uint16_t)(at + 1));
            flow.demo_at = (uint16_t)(at + 3);
            continue;
        default:
            return 1;
        }
    }
}

/* What follows a race step in the demo; returns 1 when it ended. */
static int demo_after_step(void)
{
    flow.demo_frame++;
    if (flow.demo_mode == DEMO_TO_FADE)
        return demo_fade_done();
    if (!(flow.demo_flags & 0x0001)) {
        if (flow.demo_flags & 0x0002)
            return 0;
        if (!(frame_state.held[0] >> 8))
            return 0;
        flow.demo_flags |= 0x0041;
        demo_fade_out();
    }
    if (demo_fade_done())
        return 1;
    flow.demo_mode = DEMO_TO_FADE;
    return 0;
}

/* One step of the demo; returns 1 when it ended. */
static int demo_step(void)
{
    int r;

    if (flow.demo_mode == DEMO_TO_FADE) {
        flow.stopped = 0;               /* a step begins, as above */
        RaceFrameRun();
        return flow.stopped ? 0 : demo_after_step();
    }
    PlayerStatsAndPaletteAnimate();
    r = demo_commands();
    if (r == 1)
        return 1;
    if (r == 2) {
        /* $0B: waiting for a button held, no race step taken */
        flow.pace_count = (uint8_t)(RACE_STEP_UPDATES - 1);
        return 0;
    }
    if (flow.demo_mode == DEMO_WAIT_FADE)
        return 0;
    /* a step begins, as race_step's: a demo comes in with it still set by
       what ran before (BootSoftRestart, a race's end), and a step run with
       it set stops at its first check (one_player_finish): the picture is
       never handed over, no frame is counted, and the script never reaches
       its next command */
    flow.stopped = 0;
    RaceFrameRun();
    return flow.stopped ? 0 : demo_after_step();
}

/* One update of attract demo `which`; 0 while it runs, 1 when a button
   ended it, -1 when it ran out. */
static void demo_start(uint16_t offset)
{
    g.fx_screen_mode = 1;
    demo_setup(offset);
    flow.pace_count = 0;
    flow.wait = WAIT_NONE;
    flow.demo_mode = DEMO_COMMANDS;
    flow.finished = 0;
    flow.demo_ended = 0;
}

/* One update of a running demo: 0 while it runs, 1 when a button ended it,
   -1 when it ran out, -2 when a soft reset left the game's session. */
static int demo_frame(void)
{
    raster_frame();
    if (flow.wait != WAIT_NONE) {
        wait_step();
        if (flow.finished) {
            screen_handed_back();
            return -2;
        }
        if (!flow.demo_ended)
            return 0;
    } else if (flow.demo_mode == DEMO_WAIT_FADE) {
        if (!demo_fade_done())
            return 0;
    } else if (++flow.pace_count < RACE_STEP_UPDATES) {
        return 0;
    } else {
        flow.pace_count = 0;
        if (!demo_step())
            return 0;
    }
    {
        int button = (flow.demo_flags & 0x0040) != 0;

        pic.blank = 1;
        flow.demo_flags = 0;
        screen_handed_back();
        return button ? 1 : -1;
    }
}

int DemoRun(int which)
{
    static const uint16_t offsets[3] = {0x0000, 0x006B, 0x00FC};
    int r;

    if (flow.state != FLOW_DEMO) {
        demo_start(offsets[which & 3]);
        flow.state = FLOW_DEMO;
        return 0;
    }
    r = demo_frame();
    if (r)
        flow.state = FLOW_OFF;
    return r == -2 ? -1 : r;
}

/* The ending's demos, one after another (03:865E DemoRunThird, 03:866A
   DemoRunThirdRest), then 03:8C95's restart: the port 1 command 6 the
   original sends first is followed at once by the sound's reset. */
static void ending_step(void)
{
    static const uint16_t offsets[3] = {0x0159, 0x01FA, 0x0287};
    int r;

    if (!flow.ending_running) {
        demo_start(offsets[flow.ending_at]);
        flow.ending_running = 1;
        return;
    }
    r = demo_frame();
    if (!r || r == -2)
        return;
    flow.ending_running = 0;
    if (++flow.ending_at < 3)
        return;
    pic.blank = 1;
    soft_reset_finish();
}
