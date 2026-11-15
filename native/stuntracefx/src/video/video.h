/*
 * video.h: the picture's routines other areas call, by their original names.
 *
 * Order within an update: the work of a race frame runs from the vertical
 * blank before its picture to the lines inside it, so the stage that uploads
 * (RasterPaletteUpload and the windows' set-up) runs first and the steps the
 * original ran during the picture's lines run after it. video_draw then
 * draws that one picture.
 */
#ifndef STUNTRACEFX_VIDEO_H
#define STUNTRACEFX_VIDEO_H

#include <stdint.h>

#include "data/cart.h"
#include "nshvc/nshvc.h"
#include "system/state.h"

/* The per-line effects all set at once, as a store of the whole of $0D45 set them. */
static inline void line_effects_set(int results_bg1_off, int course_wipe, int course_scroll, int sky, int window_bands, int fixed_bands)
{
    g.line_effects.results_bg1_off = (uint8_t)results_bg1_off;
    g.line_effects.course_wipe = (uint8_t)course_wipe;
    g.line_effects.course_scroll = (uint8_t)course_scroll;
    g.line_effects.sky = (uint8_t)sky;
    g.line_effects.window_bands = (uint8_t)window_bands;
    g.line_effects.fixed_bands = (uint8_t)fixed_bands;
}

/* ---- the screen (screen.c): its size from the window, the HUD's places ---- */

typedef struct {
    int width, height;                  /* the screen, from the window's shape */
    int origin_x, origin_y;             /* where the original's 256 x 224 sits in it */
    int safe[4];                        /* the window's safe area in screen pixels: left, top, right, bottom */
} ScreenShape;

void screen_update(void);                      /* from the window, once a picture */
const ScreenShape *screen_shape(void);
void screen_window(float *width, float *height);   /* the window in pixels the shape was made from */
void screen_window_set(const float *values, int count); /* a check's window: width, height, safe area */
int screen_anchor_x(int x0, int x1);           /* columns to add to a piece of the original's screen */
int screen_view_anchor_x(int x0, int x1);      /* the same for a piece inside the original's view */
int screen_lamp_anchor_x(int x0, int x1);      /* the same for the start lamps: to the screen's edges, not the HUD box's */
/* When two shapes of the HUD (boxes x0..x1, y0..y1 in picture pixels) are one piece to move onto the larger
   screen. They touch (as the original drew them); or they are words of one line, boxes of the same height on
   the same rows with no more than a space between (a space is 8, a double space 16: HUD_TEXT_GAP); or they are
   lines of one message, one above the other with columns in common and no more than that between. Without
   the last two each word was anchored by itself, the left one to the left edge and the right one to the right.
   Anything else beside it (the digits, the banner, the icons) is not one piece with a line of text. */
enum { HUD_TEXT_GAP = 16 };
static inline int hud_text_near(int ax0, int ax1, int ay0, int ay1, int bx0, int bx1, int by0, int by1)
{
    int dy0 = ay0 > by0 ? ay0 - by0 : by0 - ay0, dy1 = ay1 > by1 ? ay1 - by1 : by1 - ay1;

    if (ax0 <= bx1 + 1 && bx0 <= ax1 + 1 && ay0 <= by1 + 1 && by0 <= ay1 + 1)
        return 1;
    if (dy0 <= 1 && dy1 <= 1 && ax0 <= bx1 + HUD_TEXT_GAP && bx0 <= ax1 + HUD_TEXT_GAP)
        return 1;
    return ax0 <= bx1 && bx0 <= ax1 && ay0 <= by1 + HUD_TEXT_GAP && by0 <= ay1 + HUD_TEXT_GAP;
}
int screen_anchor_y(int y0, int y1);           /* lines to add, by the piece's middle */
int screen_bottom_shift(void);                 /* lines the panels' band moves down by */
int screen_two_side(void);                     /* two players side by side (else above and below) */
void screen_two_rect(int player, int *x, int *y, int *w, int *h); /* a player's rectangle of the screen */
int screen_two_scale(int player);              /* its boxes' scale, 256ths */
void screen_two_anchor(int player, int x0, int x1, int y0, int y1, int *dx, int *dy); /* where its original 144 x 80 view goes for a piece */
void screen_two_box_source(int player, int box, int *x, int *y, int *w, int *h); /* a box's place in the player's 80 x 96 */
void screen_two_box(int player, int box, float *x, float *y, float *w, float *h);   /* and where it floats, scaled */
int race_full_window(void);                    /* the one-player race fills the window */
int race_results_page(void);                   /* the one-player race screen shows the results' page */
int race_two_full_window(void);                /* two players fill the window */
int race_course_full_window(void);             /* the course screen fills the window */
/* The course screen's wipe laid over the whole window (course_wipe.c): the
   edges window 1 has by original line, and the blend that gives its level; off. */
void course_wipe_overlay(const int32_t left[224], const int32_t right[224], const nshvc_blend *blend);
void course_wipe_overlay_off(void);
nshvc_image fx_noise_image(void);              /* fx_draw.c: the static's noise texture */
uint16_t fx_static_seed(int line);             /* fx_draw.c: the seed line 0-127 of the static's block was drawn with */
void race_two_full_window_ready(int on);       /* interim: the 3D's side turns it on with its two-player rectangles */
void race_hills_full(void);                    /* its hills and sky for the picture shown, or one between steps */
uint16_t sky_colour_open(int at);               /* the same untinted: the sky outside a tunnel */
uint16_t sky_colour_at(int at);                /* colour 1 at place `at` of the day and night lists */
void video_newest_lines_take(void);                 /* picture.c, from video_between_begin */
uint16_t race_ground_colour(int panel, float row);   /* colour 1 shown on a screen row of a player's view (picture.c) */
uint16_t race_open_sky_colour(int row);            /* the sky outside on a row, while in a tunnel (an exit's opening) */
uint16_t race_full_sky_colour(int row);             /* one player's full window: colour 1 on a screen row, by the 3D's horizon */
uint16_t race_two_sky_colour(int player, int row);  /* two players' colour 1 on a screen row */
int race_underwater_cells(uint8_t texels[80][64], uint16_t colours[16]); /* Aqua Tunnel's ripple cells; 0 if not loaded */
void sprites_place_full(nshvc_sprite *out, const nshvc_sprite *in, uint32_t count); /* the race's sprites on the larger screen */
void sprites_place_two(nshvc_sprite *out, const nshvc_sprite *in, uint32_t count);  /* two players' */
unsigned sprite_name_of(const nshvc_sprite *s);  /* a sprite's 9-bit character name, as the original's table holds it */

/* The race screen's engine objects, by role. */
typedef struct {
    nshvc_tiles tiles;                  /* the 4bpp art the backdrop and sprites share */
    nshvc_map backdrop_one;             /* 32 x 64 */
    nshvc_map backdrop_wide;            /* the full window's hills: 128 x 192, the panorama's whole round */
    nshvc_tiles frame_tiles_two;        /* the two-player frame and panels, 4bpp */
    nshvc_map frame_map_two;            /* and its map */
    nshvc_image backdrop_image, panel_image; /* the two-player backdrop and panel, drawn into the list */
    nshvc_tiles course_tiles;           /* the course screen's backdrop art */
    nshvc_map course_map;               /* and its map, 32 x 64 */
    nshvc_image frame_image;            /* the frame and panels around the 3D, at screen positions */
    nshvc_tiles frame_tiles_one;        /* the one-player frame and panels as the original's BG1 art, 8bpp */
    nshvc_map frame_map_one;            /* and its BG1 map (the results' page shows BG1 so) */
} RaceVideo;

extern RaceVideo race_video;

void race_video_init(void);
/* The race's tile sets and maps, written by what they are and by character
   or row: never by the original's video addresses. The race art is one 4bpp
   set of 1024 characters; with two players BG2 draws from its start, with
   one from RACE_ART_SPRITES, where the sprites' characters begin. */
enum { RACE_ART_TWO_BACKDROP = 0x000, RACE_ART_SPRITES = 0x200, RACE_FRAME_TWO_FRAME = 0x168 };
void race_art_write(unsigned character, const uint8_t *data, uint32_t bytes);           /* the race art */
void race_frame_art_write(unsigned character, const uint8_t *data, uint32_t bytes);     /* one player: the frame's 8bpp art */
void race_frame_two_art_write(unsigned character, const uint8_t *data, uint32_t bytes); /* two players: BG1's art (the halves' 3D, then the frame's) */
void race_course_screen_art_write(unsigned character, const uint8_t *data, uint32_t bytes);
void race_course_screen_map_write(const uint16_t *words, unsigned rows);               /* 32 entries a row */
void race_course_maps_keep(void);              /* the unpacked course picture to both maps */
void race_course_preview_reset(void);          /* $7F:C3D7 to $7F:9BD7, $800 bytes */
void race_frame_art_draw(nshvc_draw_list list);    /* the frame and panels, at screen positions, after the list is cleared */
void race_frame_art_draw_over(nshvc_draw_list list); /* the full window's panel boxes, after the 3D */
typedef struct { int count, x0[16], x1[16], y0[16], y1[16]; } PanelBoxes;
const PanelBoxes *race_panel_boxes(void);          /* the one-player panel row's boxes, on the original's screen */
void race_backdrop_two_pages_write(int first, const uint16_t *words, uint32_t n);
void VideoRegsReset(void);
void race_map_page_write(int page, const uint16_t *words);
void race_panel_map_write(const uint16_t *words);  /* two players: the panel's map over a half (a finished player's, the quit menu's), $400 words */
/* the results screens' pictures, by what they hold (the art the sprites draw from) */
void race_results_picture_write(const uint8_t *picture); /* the big picture, $A00 bytes: 5 rows of 16 characters */
void race_car_picture_write(const uint8_t *picture);     /* the car's picture, $C00 bytes */
void race_course_picture_write(void);   /* the course's picture, unpacked at FX_UNPACK_AREA: its four rows to the sprites' art */
void race_half_characters_write(int half, const uint8_t *characters); /* two players: a half's 3D characters, $1680 bytes, for the quit menu */
void race_frame_map_two_write(const uint16_t *words);
void race_two_player_list(void);
void race_backdrop_two_write(int page, const uint16_t *words);
void race_picture_shown(int buffer);
void race_backdrop_rows_upload(int buffer);
void race_panel_show(int half, uint8_t sign_of_y, uint8_t y_low, uint8_t y_high_when_plus, uint8_t y_when_minus, uint8_t x_low);
void RaceVideoSetup(void);
void PictureTurnStep(void);
void VramUploadGsuBuffer(void);
void RaceCourseUpload(void);
uint16_t *race_map_buffer(void);
void RaceHudLoad(void);
void NmiFrameUploadSprites(void);
extern uint8_t race_pictures[0x3000];
void race_screen_start(void);
void race_screen_end(void);                    /* the race hands the screen back: its layers cleared */
void RaceVideoUpload1P(void);
void RaceVideoWindows1P(void);
void RaceVideoUpload2P(void);
void RaceVideoSplit2P(void);
void RaceScreenSetup(void);

/* ---- palettes ---- */

/* the colours as the game builds them; what is on screen is pic.palette, which
   only the named upload routines below change */
typedef struct {
    uint16_t buffer[256];               /* $7E:FD65: the colours built for the screen */
    uint16_t blended[256];              /* $7F:35D7: the course's colours as the fog has left them */
    uint16_t day[256];                  /* $7F:37D7: the course's colours */
    uint16_t night[256];                /* $7F:39D7: the course's other set, the weather's */
    /* the fog and gradient step's values */
    uint16_t band_end;                  /* $7F:345A */
    uint16_t blend_request;             /* $7F:345C: < 0 the target's low 15 bits and 2 more of amount, > 0 3 less */
    uint16_t weather;                   /* $7F:3460: how far the weather has gone, 0-31 */
    uint16_t weather_shown;             /* $7F:3464 */
    uint16_t weather_value;             /* $19FE: its fraction, $19FF the whole part */
    uint16_t weather_step, weather_goal; /* $1A00, $1A02 */
    uint16_t amount;                    /* $7F:3468: the blend toward the target, 0-31 */
    uint16_t amount_shown;              /* $7F:346C */
    uint16_t target;                    /* $7F:3470: the fog's colour; bit 15 none */
    uint16_t target_shown;              /* $7F:3474 */
    uint16_t gradient_was_off;          /* $7F:3478 */
    uint16_t band;                      /* $7F:347C: which of the three gradient bands is rebuilt */
    uint16_t hud_colours[6];            /* $0E21: the HUD's colours, blended */
    uint8_t cycle_phase[3];             /* $7E:FD5B, $FD5D, $FD5F: 0-15 */
    uint8_t course_cycle;               /* $0D30 */
    uint8_t tile_cycle;                 /* $0DD5 */
} Palettes;

extern Palettes palettes;

void PaletteCourseLoad(int course);
void PaletteDarkenFour(uint16_t request);

void RasterColours(void);
void RasterPaletteUpload(void);
void RasterColoursUnlessSplitStages(void);
void PaletteKeptColoursStep(int player, int pad_index);
void PlayerColoursFlash(uint8_t level, int player);
void StatusColourPick(int item);
void PaletteCycle(void);
void PalettesStep(void);
void CoursePaletteCycle(void);
void HudColourStep(void);
void FadeValueStep(void);
void PaletteFadeStep(void);
void CgramDmaAll(void);                 /* the whole buffer to the screen */
void CgramUploadAll(void);              /* the same, and notes the player's offset */
void palette_shown_all(void);

uint16_t FadeStep(void);
uint16_t FadeStepFixed(void);

/* ---- the gradient ---- */

/* Colour 1 by line: three bands of 128 and the lists they are made from. */
#define GRADIENT_BAND_COLOURS 128
typedef struct {
    uint16_t band[3 * GRADIENT_BAND_COLOURS];   /* $7F:EC57 */
    uint16_t day[512];                  /* $7F:EF57 */
    uint16_t night[512];                /* $7F:F357 */
    uint16_t black[128];                /* $7F:F757 */
    uint16_t view_offset[2];            /* $0D96: byte offset into the lists, by view */
    uint16_t source[2];                 /* $0D9A: each view's start, a colour number into this block */
} Gradient;

extern Gradient gradient;

enum { GRADIENT_AT_BANDS = 0, GRADIENT_AT_DAY = 3 * GRADIENT_BAND_COLOURS,
       GRADIENT_AT_NIGHT = GRADIENT_AT_DAY + 512, GRADIENT_AT_BLACK = GRADIENT_AT_NIGHT + 512 };

/* colour number `n` from the block's start, past the black list counted as black */
uint16_t gradient_colour(uint32_t n);

void GradientBuildColours(int course);
void GradientSetup(void);
void GradientStartsFromViews(void);

/* ---- windows and colour maths ---- */

/* a window setting: both windows' edges */
typedef struct { uint8_t left1, right1, left2, right2; } WindowEdges;

enum { WINDOW_BANDS_MAX = 16 };
typedef struct { uint8_t lines; const WindowEdges *edges; } WindowBand;   /* a band's line count and its edges */

typedef struct {
    WindowBand band[WINDOW_BANDS_MAX];  /* $7F:F85B: a table of line counts and edges */
    uint8_t bands;
    WindowEdges normal;                 /* $7F:F857 */
    WindowEdges view_edges, top_view_edges, bottom_view_edges, split_edges; /* the view's window, each half's, the split's (the original's $7F:FFD1, $FFD5, $FFD9, $FFDD) */
    uint8_t setup;                      /* $19A9: 0, 2, 4, 6 or 8 */
    uint8_t colours[6];                 /* $19AC-$19B1: the fixed colour's three parts for each half */
    uint8_t layout_request;             /* $19BD */
    uint8_t layout_moving;              /* $19BC: $FF while it moves */
    int16_t layout;                     /* $19B6 */
    int16_t layout_step;                /* $19B8 */
    /* the sliding, sweeping and bobbing windows */
    uint8_t bands_state;                /* $19D8 */
    uint8_t bands_kind;                 /* $19DF */
    uint16_t bands_pos, bands_step, bands_end;  /* $19D4 ($19D5 its high byte), $19D6, $19D9 */
    uint16_t split_pos, split_step;     /* $19DB ($19DC its high byte), $19DD */
    uint8_t split_tops[2];              /* $19E2, $19E3 */
    uint8_t split_bottoms[2];           /* $19E4, $19E5 */
    uint8_t windows_colour;             /* $19E0 */
    uint8_t slide_pos;                  /* $1A14 */
    uint8_t oscillator[2], oscillator_step[2], oscillator_count[2], oscillator_rate, oscillator_level[2]; /* $1A08, $1A0C, $1A0A, $1A10, $1A0E */
    uint8_t oscillator_input;           /* $1A11 */
    uint8_t sweep[2], sweep_step[2], sweep_halve[2], sweep_steps; /* $1A15, $1A19, $1A17, $1A1B */
    /* the fixed colour by line, for the weather and water */
    uint8_t level_table[64];            /* $7F:FF8B */
    uint8_t level_table_bytes;
    uint16_t level;                     /* $19C3 */
    int16_t level_step;                 /* $19C5 */
    int8_t top_step, low_step;          /* $19C7, $19CD */
    uint16_t player_y;                  /* $19C9 */
    uint8_t top_band, low_band;         /* $19CB, $19CF */
    uint8_t band_limits;                /* $19D1 */
    uint8_t player_raised;              /* $19D3 */
    /* the wipe that opens the race */
    uint8_t wipe_table[36];             /* $7F:3410 */
    uint8_t wipe_left, wipe_a, wipe_top, wipe_right, wipe_b, wipe_bottom; /* $7F:3433, $3434, $3435, $3437, $3438, $3439 */
    uint8_t wipe_width, wipe_middle;    /* $7F:3402, $3406 */
    /* the course screen's colour maths ($0D47): the fixed colour taken from
       (or added to) these layers, halved or not */
    struct {
        uint8_t subtract, half;
        uint8_t layers;                 /* bits: BG1-4, sprites, backdrop */
    } maths;
    uint8_t fixed_step;                 /* $7F:3408 */
    uint8_t fixed_level;                /* $7F:340D */
} Windows;

extern Windows windows;

void WindowsInit(void);
void WindowHdmaNormal(void);
void WindowHdmaBands(void);
void WindowBandsView(void);
void WindowLayoutStepForScreen(void);
void WindowSlideOrOscillate(void);
void WindowSweepIfOn(void);
void WindowSlideCheck(void);
void BobWindowsRun(void);
void WindowBandsAnimate(void);
void WindowSplitAnimate(void);
void WindowsSetupSelect(void);
const WindowEdges *window_edges_next(const WindowEdges *e, int k);   /* a repeating band's edges k lines on */
void WindowsColourAddBg12(void);
void WindowsColourWindowOnly(void);
void WindowsAllInverted(void);
void CourseWipeTable(void);
void WindowEdgesStep(void);
void ColourFadeBg12(void);
void ColourFadeBg123Start(void);
void ColourMathOff(void);
void WindowBandsTopView(void);
void WindowBandsBottomView(void);
void WindowBandsSplit(void);
uint8_t FixedColourFromStep(void);

/* ---- the backdrop and its scroll ---- */

void RaceScreenSetup(void);
uint16_t BackdropScrollFromView(void);
void BackdropRowsUpdate(void);
void BackdropAnglesFromView(void);
void ScrollHdmaSet(void);
void Bg2ScrollFromTable(int half);

/* ---- tile sets and maps ---- */

void RaceVideoInit(void);
void QuitMenuRow1Cycle(void);
void QuitMenuRow2Cycle(void);
void QuitMenuRow3Cycle(void);
void QuitMenuRow1Plain(void);
void QuitMenuRow2Plain(void);
void QuitMenuRow3Plain(void);
void ResultsMenuRetryPlain(void);
void ResultsMenuCoursePlain(void);
void ResultsMenuCarPlain(void);
void TilemapLevelColumns(void);
void CarSettingsDigits(void);
void VramNoiseTiles(void);
void TimesKeepAndAdvance(void);
void ChampionshipStageStart(void);
void ChampionshipStageWaitOpen(void);
void ChampionshipStagePicture(void);
void ChampionshipStageMap(void);
void ChampionshipStageWaitClosed(void);
int WindowClosedDigitStep(void);

/* ---- sprites ---- */

extern nshvc_sprite sprites[128];
/* the tile set and page starts sprite tile numbers count in: each screen sets its own */
void sprites_use_tiles(nshvc_tiles set, uint16_t first_page, uint16_t second_page);
/* One sprite's fields by its number (0-127), never by the original's table
   addresses: X's low eight bits (the ninth with the size, four sprites a
   call: sprite_high), Y, the character in its page, and the attributes as
   the original's byte (page, palette, priority, flips). */
void sprite_x_set(int n, uint8_t x);
void sprite_y_set(int n, uint8_t y);
void sprite_tile_set(int n, uint8_t tile);
void sprite_attributes_set(int n, uint8_t attributes);
void sprite_high_set(int first, uint8_t bits);  /* sprites first..first+3, two bits each: X's ninth, large */
uint8_t sprite_x_get(int n);
uint8_t sprite_y_get(int n);
uint8_t sprite_tile_get(int n);
uint8_t sprite_attributes_get(int n);
uint8_t sprite_high_get(int first);
typedef enum { SPRITE_X, SPRITE_Y, SPRITE_TILE, SPRITE_ATTRIBUTES } SpriteField;
void sprite_field_set(int n, SpriteField field, uint8_t v);  /* for tables of sprites' fields */
void OamDmaAll(void);
void OamDirectStage(void);
void OamHideRange(int first, int count);
void OamBufferClear(void);
void OamBufferHideLarge(void);
void SpriteSlot1Countdown(void);
int SpriteSlotFirst(void);
void OamFromList(int *slot, uint16_t *list);   /* the HUD's sprite lists from byte offset *list */
void OamSprite0AddY(uint8_t lines);
void oam_attr_fill(int first, int count, uint8_t attr);
void OamAttr36x4(int first);
void OamAttr36x6(int first);
void OamAttr36x7(int first);
void OamAttr38x4(int first);
void OamAttr38x6(int first);
void OamAttr38x7(int first);
void OamHideEntries3And9(int first);
void OamHideEntries0To2(int first);
void OamSetPalette(uint8_t bits, int count, int first);
void StuntRecordsPagePalette(void);
void SpeedRecordsPagePalette(void);
void AllRecordsPagePalette(void);
void BattleRecordsPagePalettes(void);
void SpeedRecordsLinePalette(void);
void PauseChoiceLeft(void);
void PauseChoiceRight(void);
void PauseChoiceLeftPad1(void);
void PauseChoiceRightPad1(void);
void PauseChoiceLeftPad2(void);
void PauseChoiceRightPad2(void);
void OamColumn8(int first, uint16_t pos, uint16_t tile);
void TopGaugeSpritesOff(void);
void BottomGaugeSpritesOff(void);
void TopHudSpritesOff(void);
void BottomHudSpritesOff(void);
void ChampionshipSpritesHide(void);
void OamLargeColumns(void);
void HudSpritesPlace(void);
void SpriteStrip4(uint16_t selector, int first, uint16_t pos);
void SpriteStrip6(uint16_t selector, int first, uint16_t pos);
int MetaspriteDraw(uint16_t group, int slot);              /* returns the sprites used */
void FlashingMetaspriteDraw(void);
void FlashingMetaspriteDraw0(void);
void FlashingMetaspriteDrawByMode(void);
void SpritePiecesAtLeft(uint16_t set, int slot);
void SpritePiecesAtRight(uint16_t set, int slot);
void StuntRecordsRow1Car(uint16_t set, int slot);
void StuntRecordsRow2Car(uint16_t set, int slot);
void StuntRecordsRow3Car(uint16_t set, int slot);
void AllRecordsRow1Car(uint16_t set, int slot);
void AllRecordsRow2Car(uint16_t set, int slot);
void AllRecordsRow3Car(uint16_t set, int slot);
void RecordsBestTimeCar(uint16_t car, int slot);
void SpritePiecesTwoCars(void);
void SpritePiecesTwoCarsTop(void);
void OverlaySpritesStep(void);
void OamTextLine(int slot, uint8_t tile_base, uint8_t x, uint8_t y, uint8_t attr);
void SpeedRecordsTextLine(void);
void StuntBattleRecordsTextLine(void);
void AllRecordsTextLine(void);

/* wanted (menu/records_screen.c): a row's car, three pieces */
void SpritePieces3(uint16_t x, uint8_t y, uint16_t set, uint16_t palette, int slot);
/* wanted by the race (race/phases.c, the lap and results panels) */
uint8_t SpriteYOnScreen(uint8_t y);                /* 7E:B005 */
void SpriteBar4WithMarker(uint8_t value, uint16_t pos, int first);  /* from sprite `first` */
void SpriteBar6WithMarker(uint8_t value, uint16_t pos, int first);
void HudSpritesHide(void);
void OamHideForHudMode(void);

#endif
