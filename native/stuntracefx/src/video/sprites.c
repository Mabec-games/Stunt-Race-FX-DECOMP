/*
 * sprites.c: the race's sprite table. `sprites` is what the game builds; what
 * is on screen is pic.sprites, which only OamDmaAll and OamDirectStage change.
 * Each sprite's tile counts from the race's art: the sprites' first page is
 * tile $200 on in the shared set, the second $300.
 */
#include <string.h>

#include "audio/sound.h"
#include "data/race_tables.h"
#include "data/video_tables.h"
#include "race/race.h"
#include "system/game.h"
#include "system/state.h"
#include "video/picture.h"
#include "video/video.h"

nshvc_sprite sprites[128];

/* the tile set the table's tile numbers count in, and where its two pages
   start in it: set by each screen; the race's art by default */
static nshvc_tiles sprite_set;
static uint16_t page_first = 0x200, page_second = 0x300;

void sprites_use_tiles(nshvc_tiles set, uint16_t first_page, uint16_t second_page)
{
    int n;

    sprite_set = set;
    page_first = first_page;
    page_second = second_page;
    for (n = 0; n < 128; n++)
        sprites[n].tiles = set;
}

static int on_second_page(int n)
{
    return sprites[n].tile >= page_second;
}

/* A sprite's fields as the original's table bytes hold them. */
static void set_tile_byte(int n, uint8_t t)
{
    sprites[n].tile = (uint16_t)((on_second_page(n) ? page_second : page_first) + t);
    sprites[n].tiles = sprite_set;
}

static uint8_t tile_byte(int n)
{
    return (uint8_t)(sprites[n].tile - (on_second_page(n) ? page_second : page_first));
}

/* The attribute byte: name page, palette, priority, flips */
static void set_attr(int n, uint8_t a)
{
    uint8_t t = tile_byte(n);

    sprites[n].tile = (uint16_t)(((a & 1) ? page_second : page_first) + t);
    sprites[n].palette = (a >> 1) & 7;
    sprites[n].priority = (a >> 4) & 3;
    sprites[n].hflip = (a >> 6) & 1;
    sprites[n].vflip = (a >> 7) & 1;
    sprites[n].tiles = sprite_set;
}

static uint8_t attr_byte(int n)
{
    return (uint8_t)(on_second_page(n) | sprites[n].palette << 1 | sprites[n].priority << 4 |
                     sprites[n].hflip << 6 | sprites[n].vflip << 7);
}

/* The screen position as the table holds it: the X byte, the high X bit apart; Y a byte. */
static void set_x_byte(int n, uint8_t x)
{
    sprites[n].x = (sprites[n].x < 0) ? (int32_t)x - 256 : (int32_t)x;
}

static void set_x_high(int n, int high)
{
    uint8_t x = (uint8_t)sprites[n].x;

    sprites[n].x = high ? (int32_t)x - 256 : (int32_t)x;
}

static void set_y(int n, uint8_t y)
{
    sprites[n].y = y >= 240 ? (int32_t)y - 256 : (int32_t)y;
}

static uint8_t y_byte(int n)
{
    return (uint8_t)sprites[n].y;
}

/* 16 x 16 when large, 8 x 8 otherwise, the race's sprite sizes */
static void set_large(int n, int large)
{
    sprites[n].width = sprites[n].height = large ? 2 : 1;
}

/* A high table byte: four sprites' X-high and size bits, as the original wrote them. */
static void set_high_byte(int byte_index, uint8_t v)
{
    int i;

    for (i = 0; i < 4; i++) {
        int n = byte_index * 4 + i;

        set_x_high(n, (v >> (i * 2)) & 1);
        set_large(n, (v >> (i * 2 + 1)) & 1);
    }
}

static void or_high_byte(int byte_index, uint8_t v)
{
    int i;

    for (i = 0; i < 4; i++) {
        int n = byte_index * 4 + i;

        if ((v >> (i * 2)) & 1)
            set_x_high(n, 1);
        if ((v >> (i * 2 + 1)) & 1)
            set_large(n, 1);
    }
}

/* the whole word of a sprite: X, Y bytes */
static void set_pos_word(int n, uint16_t w)
{
    set_x_byte(n, (uint8_t)w);
    set_y(n, (uint8_t)(w >> 8));
}

/* tile and attribute bytes */
static void set_tile_word(int n, uint16_t w)
{
    set_tile_byte(n, (uint8_t)w);
    set_attr(n, (uint8_t)(w >> 8));
}

/* ---- the table to the screen ---- */

void OamDmaAll(void)
{
    memcpy(pic.sprites, sprites, sizeof pic.sprites);
    pic.sprite_count = 128;
}

/* Bands of sprites written straight to the screen's table in the blank lines:
   stage $0D54 picks them; the data is the table as it stands. */
void OamDirectStage(void)
{
    static const struct { uint8_t first, count; } band[] = {{116, 12}, {75, 12}, {112, 4}, {36, 11}};

    if (g.hud_mode != 1 && g.hud_hold) {
        int n;

        for (n = 116; n < 128; n++) {       /* all four table bytes $F0 */
            pic.sprites[n] = sprites[n];
            pic.sprites[n].x = 0xF0;
            pic.sprites[n].y = 0xF0 - 256;
            pic.sprites[n].tile = (uint16_t)(page_first + 0xF0);
            pic.sprites[n].palette = 0;
            pic.sprites[n].priority = 3;
            pic.sprites[n].hflip = 1;
            pic.sprites[n].vflip = 1;
        }
        return;
    }
    switch (g.start_light) {
    case 0x02:
        if (g.hud_marker_x == 0xAC)
            g.start_light = (uint8_t)(g.start_light + 2);
        /* fall through */
    case 0x00:
        memcpy(&pic.sprites[band[0].first], &sprites[band[0].first], band[0].count * sizeof sprites[0]);
        break;
    case 0x04:
        if (g.gauge_place[4] == 0x1F)
            g.start_light = (uint8_t)(g.start_light + 2);
        memcpy(&pic.sprites[band[1].first], &sprites[band[1].first], band[1].count * sizeof sprites[0]);
        break;
    case 0x06:
        if (g.hud_panel_b_x == 0x2A) {
            g.end_panel_in = 0x2A;
            g.start_light = 0;
        }
        memcpy(&pic.sprites[band[2].first], &sprites[band[2].first], band[2].count * sizeof sprites[0]);
        break;
    case 0x08:
        if (g.hud_slide == 0x10)
            g.start_light = 0;
        memcpy(&pic.sprites[band[3].first], &sprites[band[3].first], band[3].count * sizeof sprites[0]);
        break;
    default:
        if (g.hud_slide == 0xD0)
            g.start_light = 0;
        memcpy(&pic.sprites[band[3].first], &sprites[band[3].first], band[3].count * sizeof sprites[0]);
        memcpy(&pic.sprites[band[0].first], &sprites[band[0].first], band[0].count * sizeof sprites[0]);
        break;
    }
}

/* ---- clearing and hiding ---- */

void OamHideRange(int first, int count)
{
    do {
        set_y(first, 0xF0);
        first++;
    } while (--count);
}

void OamBufferClear(void)
{
    int n;

    for (n = 0; n < 128; n++) {
        sprites[n].tiles = sprite_set;
        set_x_byte(n, 0xF0);
        set_y(n, 0xF0);
        set_x_high(n, 0);
        set_large(n, 0);
    }
}

void OamBufferHideLarge(void)
{
    int n;

    for (n = 0; n < 128; n++) {
        sprites[n].tiles = sprite_set;
        set_y(n, 0xF0);
        set_x_high(n, 0);
        set_large(n, 1);
    }
}

/* Entries from the HUD's sprite lists (data/race_tables), from byte offset
   *list: $FE hides one, else four bytes of X, Y, tile, attributes; $FF ends. */
void OamFromList(int *slot, uint16_t *list)
{
    uint8_t v;

    while ((v = hud_sprite_list_byte(*list)) != 0xFF) {
        if (v == 0xFE) {
            set_y(*slot, 0xFE);
        } else {
            set_x_byte(*slot, v);
            set_y(*slot, hud_sprite_list_byte((uint16_t)(*list + 1)));
            set_tile_byte(*slot, hud_sprite_list_byte((uint16_t)(*list + 2)));
            set_attr(*slot, hud_sprite_list_byte((uint16_t)(*list + 3)));
            *list = (uint16_t)(*list + 3);
        }
        *list = (uint16_t)(*list + 1);
        (*slot)++;
    }
}

uint8_t SpriteYOnScreen(uint8_t y)
{
    return y >= 0xB0 ? 0xF0 : y;
}

void OamSprite0AddY(uint8_t lines)
{
    set_y(0, (uint8_t)(y_byte(0) + lines));
}

/* ---- attribute runs ---- */

void oam_attr_fill(int first, int count, uint8_t attr)
{
    do {
        set_attr(first, attr);
        first++;
    } while (--count);
}

void OamAttr36x4(int first) { oam_attr_fill(first, 4, 0x36); }
void OamAttr36x6(int first) { oam_attr_fill(first, 6, 0x36); }
void OamAttr36x7(int first) { oam_attr_fill(first, 7, 0x36); }
void OamAttr38x4(int first) { oam_attr_fill(first, 4, 0x38); }
void OamAttr38x6(int first) { oam_attr_fill(first, 6, 0x38); }
void OamAttr38x7(int first) { oam_attr_fill(first, 7, 0x38); }

static void hide_at(int n)
{
    sprites[n].x = 0;
    set_y(n, 0xF0);
}

void OamHideEntries3And9(int first)
{
    hide_at(first + 3);
    hide_at(first + 9);
}

void OamHideEntries0To2(int first)
{
    hide_at(first);
    hide_at(first + 1);
    hide_at(first + 2);
}

void OamSetPalette(uint8_t bits, int count, int first)
{
    do {
        uint8_t a = (uint8_t)((attr_byte(first) & 0xF1) | bits);

        set_attr(first, a);
        first++;
    } while (--count);
}

/* @routine 04:E2EC OamPaletteFromTableE30F */
void StuntRecordsPagePalette(void)
{
    OamSetPalette(records_palette_stunt(g.champ_stage), 0x16, g.records_group_slot[4]);
}

/* @routine 04:E23F OamPaletteFromTableE26A */
void SpeedRecordsPagePalette(void)
{
    uint16_t n = g.times_step;

    OamSetPalette(n ? records_palette_speed((uint8_t)((n - 1) & 0xFF)) : 0x0A, 0x16, g.records_group_slot[5] + 0x0D);
}

/* @routine 04:E26E OamPaletteFromTableE26AByMode */
void AllRecordsPagePalette(void)
{
    OamSetPalette(records_palette_speed(g.champ_stage), 0x12, g.records_group_slot[4]);
}

/* @routine 04:E28D OamPalettesE28D */
void BattleRecordsPagePalettes(void)
{
    OamSetPalette(records_palette_battle(g.champ_stage), 0x12, g.records_group_slot[2]);
    OamSetPalette(records_palette_car(g.car[0]), 0x06, g.records_group_slot[2] + 0x16);
    OamSetPalette(records_palette_car(g.car[1]), 0x06, g.records_group_slot[2] + 0x1C);
}

/* The records screens' sprite palettes, by screen: the rows' sprites after
   the class name's. */
/* @routine 04:E227 OamPaletteForD75 */
void SpeedRecordsLinePalette(void)
{
    OamSetPalette(0x0A, 0x16, g.records_group_slot[2] + 13);
}

/* ---- blocks and columns ---- */

static void oam_block(int first, uint8_t x0, uint8_t dx, uint8_t attr_a, uint8_t attr_b)
{
    set_x_byte(first, x0);
    set_x_byte(first + 1, (uint8_t)(x0 + dx));
    set_attr(first + 2, attr_a);
    set_attr(first + 3, attr_a);
    set_attr(first + 4, attr_b);
}

/* The pause's two choices with the left or the right one lit: the cursor's
   two sprites placed and the words' palettes. One player's from sprite 52,
   pad 1's from 59, pad 2's from 123. */
/* @routine 07:C33A OamBlock52Left */
void PauseChoiceLeft(void) { oam_block(52, 0x50, 0x22, 0x0C, 0x06); }
/* @routine 07:C36A OamBlock52Right */
void PauseChoiceRight(void) { oam_block(52, 0x87, 0x1A, 0x06, 0x0C); }
/* @routine 07:C328 OamBlock59Left */
void PauseChoiceLeftPad1(void) { oam_block(59, 0x30, 0x22, 0x0C, 0x06); }
/* @routine 07:C358 OamBlock59Right */
void PauseChoiceRightPad1(void) { oam_block(59, 0x5F, 0x1A, 0x06, 0x0C); }
/* @routine 07:C331 OamBlock123Left */
void PauseChoiceLeftPad2(void) { oam_block(123, 0x7E, 0x22, 0x0C, 0x06); }
/* @routine 07:C361 OamBlock123Right */
void PauseChoiceRightPad2(void) { oam_block(123, 0xAD, 0x1A, 0x06, 0x0C); }

void OamColumn8(int first, uint16_t pos, uint16_t tile)
{
    int i;

    for (i = 0; i < 8; i++) {
        set_pos_word(first + i, pos);
        set_tile_word(first + i, tile);
        pos = (uint16_t)(pos + 0x10);
        tile = (uint16_t)(tile + 2);
    }
}

/* A half's gauge sprites off the screen (their X past the right edge): the
   top's 27-38, the bottom's 91-102; its gauge place parked at $F8. */
/* @routine 07:C502 OamHighBits18 */
void TopGaugeSpritesOff(void)
{
    static const uint8_t bits[4] = {0x40, 0x55, 0x55, 0x15};
    int i;

    g.gauge_place[6] = 0xF8;
    for (i = 0; i < 4; i++)
        or_high_byte(6 + i, bits[i]);
}

/* @routine 07:C50E OamHighBits58 */
void BottomGaugeSpritesOff(void)
{
    static const uint8_t bits[4] = {0x40, 0x55, 0x55, 0x15};
    int i;

    g.gauge_place[7] = 0xF8;
    for (i = 0; i < 4; i++)
        or_high_byte(22 + i, bits[i]);
}

/* The same and the half's panel and map sprites off the left edge: the
   top's 48-63, the bottom's 112-127. */
/* @routine 07:C4E0 OamHighBits18Park48 */
void TopHudSpritesOff(void)
{
    TopGaugeSpritesOff();
    set_high_byte(12, 0x55);
    set_high_byte(13, 0x55);
    set_high_byte(14, 0x55);
    set_high_byte(15, 0x55);
}

/* @routine 07:C4F1 OamHighBits58Park112 */
void BottomHudSpritesOff(void)
{
    BottomGaugeSpritesOff();
    set_high_byte(28, 0x55);
    set_high_byte(29, 0x55);
    set_high_byte(30, 0x55);
    set_high_byte(31, 0x55);
}

/* Sprites 36-56 and 96-111 hidden for the championship's screen. */
/* @routine 07:C45D OamHide36And96 */
void ChampionshipSpritesHide(void)
{
    int i;

    OamHideRange(36, 0x15);
    OamHideRange(96, 0x10);
    for (i = 0; i < 4; i++)
        set_high_byte(0x18 + i, 0);
}

/* The HUD's sprites hidden at the race's end: 75-127 (in the modes with a
   map, 87-95 left alone); the map and gauge places parked at $F8. */
/* @routine 07:C4A4 OamHide75Up */
void HudSpritesHide(void)
{
    g.hud_map_y_b = 0xF8;
    g.gauge_place[6] = 0xF8;
    if (g.hud_mode) {
        OamHideRange(75, 0x0C);
        OamHideRange(96, 0x20);
    } else {
        OamHideRange(75, 0x35);
    }
}

void OamLargeColumns(void)
{
    OamColumn8(40, 0x4840, 0x0660);
    OamColumn8(96, 0x5840, 0x0680);
    OamColumn8(104, 0x6840, 0x06A0);
    set_high_byte(10, 0xAA);
    set_high_byte(11, 0xAA);
    set_high_byte(24, 0xAA);
    set_high_byte(25, 0xAA);
    set_high_byte(26, 0xAA);
    set_high_byte(27, 0xAA);
}

/* The HUD's sprites back on their rows after a replay. */
/* @routine 07:C3DD OamPlaceRows75And112 */
void HudSpritesPlace(void)
{
    static const uint8_t at8c[7] = {116, 117, 118, 120, 122, 124, 126};
    static const uint8_t at94[5] = {119, 121, 123, 125, 127};
    int i;

    if (!g.race_skipped) {
        for (i = 0; i < 7; i++)
            set_y(at8c[i], 0x8C);
        for (i = 0; i < 5; i++)
            set_y(at94[i], 0x94);
    }
    for (i = 75; i <= 86; i++)
        set_y(i, 0x93);
    set_y(112, 0x70);
    set_y(113, 0x70);
    set_y(114, 0x80);
    set_y(115, 0x80);
    set_high_byte(10, 0);
    set_high_byte(11, 0);
    set_high_byte(24, 0);
    set_high_byte(25, 0);
    set_high_byte(26, 0);
    set_high_byte(27, 0);
}

void OamHideForHudMode(void)
{
    int i;

    g.hud_map_y_b = 0xE0;
    g.gauge_place[6] = 0xE0;
    for (i = 36; i < 36 + 0x15; i++)
        set_y(i, 0xE0);
    if (!g.hud_mode) {
        for (i = 75; i < 75 + 0x35; i++)
            set_y(i, 0xE0);
    } else if (g.hud_mode != 2) {
        for (i = 75; i < 75 + 0x0D; i++)
            set_y(i, 0xE0);
        for (i = 96; i < 96 + 0x20; i++)
            set_y(i, 0xE0);
    }
}

/* ---- strips and bars ---- */

static void sprite_strip(int first, uint16_t (*tile)(uint16_t, int), uint16_t selector, uint16_t pos, int step, int n)
{
    int i;

    for (i = 0; i <= n; i++) {
        set_pos_word(first + i, pos);
        pos = (uint16_t)(pos + step);
        set_tile_word(first + i, tile(selector, i));
    }
}

void SpriteStrip4(uint16_t selector, int first, uint16_t pos)
{
    sprite_strip(first, strip4_tile, selector, pos, 0x0010, 3);
}

void SpriteStrip6(uint16_t selector, int first, uint16_t pos)
{
    sprite_strip(first, strip6_tile, selector, pos, 0x0008, 5);
}

static void bar_marker(int slot, uint16_t v, uint16_t pos, uint16_t span, uint16_t tile_far, uint16_t tile_near)
{
    if (v >= span) {
        set_pos_word(slot, (uint16_t)(v + pos - span));
        set_tile_word(slot, tile_far);
    } else {
        set_pos_word(slot, (uint16_t)(v + pos));
        set_tile_word(slot, tile_near);
    }
}

void SpriteBar4WithMarker(uint8_t value, uint16_t pos, int n)
{
    uint16_t v = value < 0x3F ? value : 0x3F;

    SpriteStrip4(v, n + 1, pos);
    bar_marker(n, v, pos, 0x10, 0x3C20, 0x3622);
}

void SpriteBar6WithMarker(uint8_t value, uint16_t pos, int n)
{
    SpriteStrip6(value, n + 1, pos);
    bar_marker(n, value, pos, 0x08, 0x3C1F, 0x3C1E);
}

/* ---- one sprite's fields, by its number ---- */

void sprite_x_set(int n, uint8_t x) { set_x_byte(n & 127, x); }
void sprite_y_set(int n, uint8_t y) { set_y(n & 127, y); }
void sprite_tile_set(int n, uint8_t tile) { set_tile_byte(n & 127, tile); }
void sprite_attributes_set(int n, uint8_t attributes) { set_attr(n & 127, attributes); }
uint8_t sprite_x_get(int n) { return (uint8_t)sprites[n & 127].x; }
uint8_t sprite_y_get(int n) { return y_byte(n & 127); }
uint8_t sprite_tile_get(int n) { return tile_byte(n & 127); }
uint8_t sprite_attributes_get(int n) { return attr_byte(n & 127); }

void sprite_field_set(int n, SpriteField field, uint8_t v)
{
    switch (field) {
    case SPRITE_X: sprite_x_set(n, v); break;
    case SPRITE_Y: sprite_y_set(n, v); break;
    case SPRITE_TILE: sprite_tile_set(n, v); break;
    default: sprite_attributes_set(n, v); break;
    }
}

/* sprites first to first + 3 (first a multiple of 4): each one's X ninth bit, then large */
void sprite_high_set(int first, uint8_t bits)
{
    set_high_byte((first & 127) / 4, bits);
}

uint8_t sprite_high_get(int first)
{
    int i, b = (first & 127) / 4;
    uint8_t v = 0;

    for (i = 0; i < 4; i++) {
        int n = b * 4 + i;

        if (sprites[n].x < 0)
            v |= (uint8_t)(1 << (i * 2));
        if (sprites[n].width > 1)
            v |= (uint8_t)(2 << (i * 2));
    }
    return v;
}

/* ---- metasprites ---- */

int MetaspriteDraw(uint16_t group, int slot)
{
    uint16_t at = sprite_group_first(group);
    int n = (uint8_t)sprite_group_count(group), count = n;

    do {
        SpritePiece p = sprite_group_piece(at);

        set_x_byte(slot, (uint8_t)(p.x + g.sprite_origin[0]));
        set_y(slot, (uint8_t)(p.y + g.sprite_origin[1]));
        set_tile_byte(slot, p.tile);
        set_attr(slot, p.attr);
        if (!(p.size & 1))
            set_large(slot, 0);
        at = (uint16_t)(at + 5);
        slot++;
    } while (--n);
    return count;
}

static uint8_t cycled_attr(uint8_t attr, uint8_t phase)
{
    static const uint8_t pal[2][3] = {{0x04, 0x08, 0x06}, {0x06, 0x04, 0x08}};
    unsigned k = (attr & 0x08) ? 0 : (attr & 0x02) ? 1 : 2;

    return (uint8_t)((attr & 0xF1) | pal[phase == 1][k]);
}

static void flashing_metasprite_draw(uint16_t set)
{
    int i, slot = 0, n;
    uint16_t at;

    g.metasprite_set = set;
    for (i = 0; i < 32; i++)
        set_high_byte(i, 0xAA);
    g.slot[0].first = 0xAA;             /* the word stores run one byte past the table */
    if (!g.flash_sprite_pos)
        return;
    if (!(g.frame_count & 3)) {
        uint16_t ph = (uint16_t)(g.flash_phase + 1);

        g.flash_phase = ph < 3 ? ph : 0;
    }
    at = flash_group_first(g.metasprite_set);
    n = (uint8_t)flash_group_count(g.metasprite_set);
    do {
        SpritePiece p = flash_group_piece(at);

        set_x_byte(slot, (uint8_t)(p.x + (uint8_t)g.flash_sprite_pos));
        set_y(slot, (uint8_t)(p.y + (uint8_t)(g.flash_sprite_pos >> 8)));
        set_tile_byte(slot, p.tile);
        set_attr(slot, g.flash_phase ? cycled_attr(p.attr, (uint8_t)g.flash_phase) : p.attr);
        at = (uint16_t)(at + 5);
        slot++;
    } while (--n);
}

void FlashingMetaspriteDraw(void) { flashing_metasprite_draw(8); }
void FlashingMetaspriteDraw0(void) { flashing_metasprite_draw(0); }
void FlashingMetaspriteDrawByMode(void) { flashing_metasprite_draw(g.time_override ? 0x14 : 0x0E); }

/* ---- the three-piece cars and sprites ---- */

/* Three sprites of a piece set at the piece-set position */
static void sprite_pieces3(int slot)
{
    uint16_t i;
    uint8_t n;

    i = (uint16_t)(piece_set_of((uint8_t)(g.piece_selector & 0xFF)) + g.piece_set);
    i = piece_set_first(i);
    for (n = 3; n; n--) {
        SpritePiece p = piece_set_piece(i);
        uint8_t a;

        set_x_byte(slot, (uint8_t)(g.piece_x + p.x));
        set_y(slot, (uint8_t)(g.piece_y + p.y));
        set_tile_byte(slot, (uint8_t)(g.piece_tile + p.tile));
        a = p.attr;
        if (g.piece_palette)
            a = (uint8_t)((a & 0xF1) | g.piece_palette);
        set_attr(slot, a);
        i = (uint16_t)(i + 5);
        slot++;
    }
}

void SpritePieces3(uint16_t x, uint8_t y, uint16_t set, uint16_t palette, int slot)
{
    g.piece_set = set;
    g.piece_x = (uint8_t)x;
    g.piece_y = y;
    g.piece_palette = (uint8_t)palette;
    sprite_pieces3(slot);
}

static void pieces_at(uint16_t set, uint8_t x, uint8_t y, int slot)
{
    g.piece_set = set;
    g.piece_x = x;
    g.piece_y = y;
    sprite_pieces3(slot);
}

void SpritePiecesAtLeft(uint16_t set, int slot) { pieces_at(set, 0x81 - 0x24, 0x89 - 0x0B, slot); }
void SpritePiecesAtRight(uint16_t set, int slot) { pieces_at(set, 0x81 + 0x24, 0x89 - 0x0B, slot); }
/* The records screens' cars, three sprites each, by row. */
/* @routine 02:FC9A SpritePiecesAt46Y71 */
void StuntRecordsRow1Car(uint16_t set, int slot) { pieces_at(set, 0x81 - 0x3B, 0x89 - 0x18, slot); }
/* @routine 02:FCA9 SpritePiecesAt46Y82 */
void StuntRecordsRow2Car(uint16_t set, int slot) { pieces_at(set, 0x81 - 0x3B, 0x89 - 0x07, slot); }
/* @routine 02:FCB8 SpritePiecesAt46Y93 */
void StuntRecordsRow3Car(uint16_t set, int slot) { pieces_at(set, 0x81 - 0x3B, 0x89 + 0x0A, slot); }
/* @routine 02:FCD6 SpritePiecesAt62Y71 */
void AllRecordsRow1Car(uint16_t set, int slot) { pieces_at(set, 0x89 - 0x27, 0x89 - 0x18, slot); }
/* @routine 02:FCE5 SpritePiecesAt62Y83 */
void AllRecordsRow2Car(uint16_t set, int slot) { pieces_at(set, 0x89 - 0x27, 0x89 - 0x06, slot); }
/* @routine 02:FCF4 SpritePiecesAt62Y95 */
void AllRecordsRow3Car(uint16_t set, int slot) { pieces_at(set, 0x89 - 0x27, 0x89 + 0x0C, slot); }

/* @routine 02:FD12 SpritePiecesCarAt51 */
void RecordsBestTimeCar(uint16_t car, int slot)
{
    g.piece_palette = 0;
    pieces_at((uint16_t)((car * 3) & 0x0F), 0x81 - 0x30, 0x89 - 0x02, slot);
}

static void two_cars(uint8_t step_mask, uint8_t y, uint8_t x1, uint8_t x2)
{
    if ((g.frame_count & step_mask) == step_mask) {
        uint8_t v = (uint8_t)(g.piece_selector + 1);

        g.piece_selector = v >= 4 ? 0 : v;
    }
    g.piece_tile = 0x30;
    g.piece_palette = 0x30;
    g.piece_y = y;
    pieces_at((uint16_t)(g.car[0] * 3), x1, y, 48);
    pieces_at((uint16_t)(g.car[1] * 3), x2, y, 51);
    set_high_byte(0x0C, 0x20);
    set_high_byte(0x0D, 0x58);
}

void SpritePiecesTwoCars(void) { two_cars(3, 0xA0, 0x84, 0xCC); }
void SpritePiecesTwoCarsTop(void) { two_cars(7, 0x38, 0x34, 0x7C); }

/* ---- the overlay of twelve large sprites ---- */

void OverlaySpritesStep(void)
{
    uint8_t s = g.overlay_state, bob, step;
    uint16_t a;
    int i;

    switch (s) {
    case 0:
        g.overlay_x = 0x009A;
        g.overlay_y = 0x006A;
        g.overlay_attr = 0;
        g.overlay_state = 1;
        break;
    case 2:
        a = (uint16_t)(g.overlay_x - 9);
        g.overlay_x = a;
        if ((a & 0x8000) && a < 0xFFB0)
            g.overlay_state = 1;
        break;
    case 3:
        g.overlay_x = 0xFE00;
        g.overlay_attr = 0x40;
        g.overlay_state = 4;
        break;
    case 4:
        a = (uint16_t)(g.overlay_x + 2);
        g.overlay_x = a;
        if (!(a & 0x8000) && a >= 0x0020) {
            g.overlay_x = 0x0020;
            g.overlay_state = 7;
        }
        break;
    case 5:
        g.overlay_x = 0x0100;
        g.overlay_attr = 0;
        g.overlay_state = 6;
        break;
    case 6:
        a = (uint16_t)(g.overlay_x - 3);
        g.overlay_x = a;
        if (a < 0x009A) {
            g.overlay_x = 0x009A;
            g.overlay_state = 7;
        }
        break;
    default:
        break;
    }
    if (g.frame_count & 4) {
        bob = 0;
        step = 0;
    } else {
        bob = 1;
        step = 8;
    }
    {
        uint16_t p = g.overlay_picture;
        uint8_t pal = car_picture_palette(p);
        unsigned at = g.overlay_attr ? 0x24 : 0;

        for (i = 0; i < 12; i++, at += 3) {
            SpritePiece piece = overlay_piece(at);
            uint16_t x = (uint16_t)(piece.x + g.overlay_x);

            if ((x & 0x8000) || x >= 0xF0) {
                set_y(36 + i, 0xF0);
            } else {
                set_x_byte(36 + i, (uint8_t)x);
                set_y(36 + i, (uint8_t)(piece.y + g.overlay_y - bob));
                set_tile_byte(36 + i, (uint8_t)(piece.tile + step));
                set_attr(36 + i, (uint8_t)(0x10 | pal | g.overlay_attr));
            }
        }
    }
    set_high_byte(9, 0xAA);
    set_high_byte(10, 0xAA);
    set_high_byte(11, 0xAA);
}

/* ---- text ---- */

/* A line of text as sprites: from `slot`, centred on x, tiles counted from
   tile_base; a byte with bit 7 moves the pen without a sprite. */
void OamTextLine(int slot, uint8_t tile_base, uint8_t x, uint8_t y, uint8_t attr)
{
    unsigned at = 0;
    uint8_t n = course_name_byte(g.course, at);
    uint8_t pen = (uint8_t)(x - (uint8_t)(n * 4));

    at++;
    for (;;) {
        uint8_t c = course_name_byte(g.course, at);

        at++;
        if (c & 0x80) {
            pen = (uint8_t)(pen + 8);
            n--;
            continue;
        }
        set_tile_byte(slot, (uint8_t)(c + tile_base));
        set_x_byte(slot, pen);
        set_y(slot, y);
        set_attr(slot, attr);
        slot++;
        pen = (uint8_t)(pen + 8);
        if (!--n)
            break;
    }
}

static void sprite_text_line_at(uint8_t x, uint8_t y)
{
    int i;

    OamTextLine(0x70, 0, x, y, 0x36);
    for (i = 0; i < 7; i++)
        set_high_byte(0x19 + i, 0);
    g.slot[0].first = 0;                /* the eighth byte falls on the record after the table */
}

/* The records screens' text line (sprites from $70), placed for each screen. */
/* @routine 02:FBE5 SpriteTextAt895E */
void SpeedRecordsTextLine(void) { sprite_text_line_at(0x89, 0x5E); }
/* @routine 02:FBF1 SpriteTextAt8152 */
void StuntBattleRecordsTextLine(void) { sprite_text_line_at(0x81, 0x52); }
/* @routine 02:FC03 SpriteTextAt8952 */
void AllRecordsTextLine(void) { sprite_text_line_at(0x89, 0x52); }

/* ---- the eight records after the table: effects on the screen's colours ---- */

/* The second record's countdown. */
void SpriteSlot1Countdown(void)
{
    if (g.slot_count == 0xFF) {
        g.slot_mask &= (uint8_t)~0x02;
        return;
    }
    g.slot_mask |= 0x02;
    g.slot[1].unused4 = 0;
    g.slot[1].unused5 = 2;
    g.slot[1].unused6 = 0xA7;
    g.slot[1].first = (uint8_t)(g.slot[1].value | 0xE0);
    if (!g.slot_count) {
        if (g.slot1_finished) {
            if (!g.slot[1].value) {
                g.slot_count--;
                return;
            }
            g.slot[1].value = (uint8_t)(g.slot[1].value - 2);
            return;
        }
        g.slot1_finished = 1;
    }
    g.slot[1].unused4 = 0;
    g.slot[1].unused6 = 0xE7;
    g.slot[1].first = 0xFF;
    g.slot_count--;
}

/* The first record in use, an index 1 on, or 0 for none. */
int SpriteSlotFirst(void)
{
    int i;

    for (i = 0; i < 8; i++)
        if (g.slot_mask & (1 << i))
            return i + 1;
    return 0;
}

/* ---- the one-player race's sprites in the whole window ---- */

/* The original's 9-bit character number of a sprite */
static unsigned sprite_name(const nshvc_sprite *s)
{
    return s->tile >= page_second ? 0x100u + (unsigned)(s->tile - page_second) : (unsigned)(s->tile - page_first);
}

unsigned sprite_name_of(const nshvc_sprite *s)
{
    return sprite_name(s);
}

static int piece_root(int *up, int i)
{
    while (up[i] != i)
        i = up[i] = up[up[i]];
    return i;
}

/*
 * The sprites placed on the larger screen (fullwindow.c's sprites_place):
 * - left out, in the ending, the HUD's sprites in the panel band;
 * - left out, what the original's screen kept out of sight: the bezel's
 *   sides (characters $40-$47, palette 3); its start lamps (characters
 *   $A0-$AF, palette 3) once the race runs (player 1's phase $0DD6 from 6);
 *   those wholly in the lines it blanked (above 16, from 208, or wrapped
 *   into the top); those off its 256 columns; the 3D's (below priority 3)
 *   wholly outside the view, which the bezel covered; the HUD's on the
 *   view's rows but beside it (parked there to slide in);
 * - the 3D's keep their place on the original's picture, the view's centre
 *   staying the screen's;
 * - the HUD's (priority 3) move by piece: the sprites that touch, and those
 *   that touch them, go together. In the panels' band a piece goes with the
 *   box under it (or by its own columns) and down to the bottom; inside the
 *   view, by the view's edges across and by its middle down.
 */
void sprites_place_full(nshvc_sprite *out, const nshvc_sprite *in, uint32_t count)
{
    enum { VIEW_X = 24, VIEW_W = 208, VIEW_Y = 32, VIEW_H = 128, PANEL_TOP = 161, PANEL_END = 208, ROW0 = 16 };
    const ScreenShape *sh = screen_shape();
    const PanelBoxes *boxes = race_panel_boxes();
    int up[128], shown[128], hud[128], lamp[128] = {0}, x0[128], x1[128], y0[128], y1[128];
    int i, b, n = count > 128 ? 128 : (int)count, lamps_gone = g.player_phase[0] >= 6;
    int ending = race_ending_running();     /* the ending shows no panels, so none of their sprites */

    for (i = 0; i < n; i++) {
        const nshvc_sprite *s = &in[i];
        int x = s->x, y = s->y, h = s->height * 8, w = s->width * 8;
        unsigned t = sprite_name(s);

        up[i] = i;
        hud[i] = s->priority == 3;
        shown[i] = !(((t & 0x1F8) == 0x40 && s->palette == 3) ||
                     ((t & 0x1F0) == 0xA0 && s->palette == 3 && (x < 48 || x >= 208) && lamps_gone) ||   /* (the lamps, at the screen's edges: the pause's turning picture has the same characters in its third row, mid-screen) */
                     y < 0 || y >= PANEL_END || y + h <= ROW0 || x + w <= 0 || x >= 256 ||
                     (!hud[i] && (x + w <= VIEW_X || x >= VIEW_X + VIEW_W || y + h <= VIEW_Y || y >= VIEW_Y + VIEW_H)) ||
                     (hud[i] && y >= VIEW_Y && y + h <= VIEW_Y + VIEW_H && (x + w <= VIEW_X || x >= VIEW_X + VIEW_W)) ||
                     (hud[i] && ending && y + h > PANEL_TOP));
        if (!shown[i])
            continue;
        for (b = 0; b < i; b++)
            if (shown[b] && hud[b] == hud[i] &&
                hud_text_near(in[b].x, in[b].x + in[b].width * 8, in[b].y, in[b].y + in[b].height * 8, x, x + w, y, y + h))
                up[piece_root(up, b)] = piece_root(up, i);
    }
    for (i = 0; i < n; i++) {
        x0[i] = y0[i] = 0x7FFF;
        x1[i] = y1[i] = -0x7FFF;
    }
    for (i = 0; i < n; i++)
        if (shown[i]) {
            int p = piece_root(up, i);

            lamp[p] |= (sprite_name(&in[i]) & 0x1F0) == 0xA0 && in[i].palette == 3 && (in[i].x < 48 || in[i].x >= 208);
            x0[p] = in[i].x < x0[p] ? in[i].x : x0[p];
            x1[p] = in[i].x + in[i].width * 8 - 1 > x1[p] ? in[i].x + in[i].width * 8 - 1 : x1[p];
            y0[p] = in[i].y < y0[p] ? in[i].y : y0[p];
            y1[p] = in[i].y + in[i].height * 8 - 1 > y1[p] ? in[i].y + in[i].height * 8 - 1 : y1[p];
        }
    for (i = 0; i < n; i++) {
        int p = piece_root(up, i), dx = sh->origin_x, dy = sh->origin_y;

        out[i] = in[i];
        if (!shown[i]) {
            out[i].y = sh->height + 64;          /* below any screen */
            continue;
        }
        if (hud[i] && in[i].y + in[i].height * 8 > PANEL_TOP) {
            int centre = in[i].x + in[i].width * 4;

            dx = screen_anchor_x(x0[p], x1[p]);
            for (b = 0; b < boxes->count; b++)
                if (centre >= boxes->x0[b] && centre <= boxes->x1[b])
                    dx = screen_anchor_x(boxes->x0[b], boxes->x1[b]);
            dy = screen_bottom_shift();
        } else if (hud[i]) {
            dx = lamp[p] ? screen_lamp_anchor_x(x0[p], x1[p]) : screen_view_anchor_x(x0[p], x1[p]);
            dy = screen_anchor_y(y0[p], y1[p]);
        } else if (lamp[p]) {                /* a lamp below priority 3 is on the view's edge too */
            dx = screen_lamp_anchor_x(x0[p], x1[p]);
        }
        out[i].x = in[i].x + dx;
        out[i].y = in[i].y + dy;
    }
}

/* ---- two players' sprites in the whole window ---- */

/*
 * fullwindow.c's sprites_two: the first 64 sprites are the first player's,
 * the rest the second's. Those off the original's screen, or off their
 * half's rows (the first's from 16 to 111, the second's from 120 to 215),
 * are left out, and so are those in the boxes' columns, which go with the
 * boxes (drawn with them, scaled). So are the 3D's (below priority 3)
 * wholly outside the player's 144 x 80 view: the frame covered them (its
 * tiles there are over them), as the bezel does for one player. In the
 * battle these are only the start lamps (characters $A0-$AF, palette 3):
 * beside the view before the start, parked at the screen's right edge
 * after it. The HUD's (priority 3) move by piece to where
 * screen_two_anchor puts the player's original view; the 3D's keep their
 * place about the rectangle's middle.
 */
void sprites_place_two(nshvc_sprite *out, const nshvc_sprite *in, uint32_t count)
{
    static const int view_x[2] = { 16, 96 }, view_y[2] = { 23, 127 }, box_x[2] = { 168, 8 }, box_y[2] = { 16, 120 };
    const ScreenShape *sh = screen_shape();
    int up[128], shown[128], hud[128], x0[128], x1[128], y0[128], y1[128];
    int i, b, n = count > 128 ? 128 : (int)count;

    for (i = 0; i < n; i++) {
        const nshvc_sprite *s = &in[i];
        int x = s->x, y = s->y, h = s->height * 8, w = s->width * 8, p = i >= 64, cx = x + w / 2;

        up[i] = i;
        hud[i] = s->priority == 3;
        shown[i] = !(y < 0 || y >= 224 || x + w <= 0 || x >= 256 || y + h <= box_y[p] || y >= box_y[p] + 96 ||
                     (cx >= box_x[p] && cx < box_x[p] + 80) ||
                     (!hud[i] && (x + w <= view_x[p] || x >= view_x[p] + 144 || y + h <= view_y[p] || y >= view_y[p] + 80)));
        if (!shown[i])
            continue;
        for (b = p ? 64 : 0; b < i; b++)
            if (shown[b] && hud[b] == hud[i] &&
                hud_text_near(in[b].x, in[b].x + in[b].width * 8, in[b].y, in[b].y + in[b].height * 8, x, x + w, y, y + h))
                up[piece_root(up, b)] = piece_root(up, i);
    }
    for (i = 0; i < n; i++) {
        x0[i] = y0[i] = 0x7FFF;
        x1[i] = y1[i] = -0x7FFF;
    }
    for (i = 0; i < n; i++)
        if (shown[i]) {
            int p = piece_root(up, i);

            x0[p] = in[i].x < x0[p] ? in[i].x : x0[p];
            x1[p] = in[i].x + in[i].width * 8 - 1 > x1[p] ? in[i].x + in[i].width * 8 - 1 : x1[p];
            y0[p] = in[i].y < y0[p] ? in[i].y : y0[p];
            y1[p] = in[i].y + in[i].height * 8 - 1 > y1[p] ? in[i].y + in[i].height * 8 - 1 : y1[p];
        }
    for (i = 0; i < n; i++) {
        int p = i >= 64, g = piece_root(up, i), rx, ry, rw, rh, dx, dy;

        out[i] = in[i];
        if (!shown[i]) {
            out[i].y = sh->height + 64;
            continue;
        }
        screen_two_rect(p, &rx, &ry, &rw, &rh);
        if (hud[i]) {
            screen_two_anchor(p, x0[g] - view_x[p], x1[g] - view_x[p], y0[g] - view_y[p], y1[g] - view_y[p], &dx, &dy);
            out[i].x = dx + in[i].x - view_x[p];
            out[i].y = dy + in[i].y - view_y[p];
        } else {
            out[i].x = rx + rw / 2 + in[i].x - (view_x[p] + 72);
            out[i].y = ry + rh / 2 + in[i].y - (view_y[p] + 40);
        }
    }
}
