/*
 * hud_setup.c: the race HUD's sprites and registers set up after its
 * graphics are loaded (RaceHudLoad goes on here): the sprite lists, the
 * HUD mode's own sprites (best time, special time or score, the players'
 * ranks), the backgrounds' scroll, the HUD's positions and the HDMA
 * channels 1-3 of the race's gradient; for two players the split screen's
 * sprites instead.
 *
 * The sprite set-up is written as runs of one macro: SEP #$20, an 8-bit
 * LDA # / STA.L for each byte set of one sprite (X, Y, tile, attributes,
 * some left out), SEP #$20, then SEP or REP #$10. The C keeps each run as
 * a table of the bytes in the order they are stored.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "hw.h"
#include "carried.h"

#include "video_calls.h"
#include "wram_names.h"
uint16_t MenuCourseScreenSetup(uint8_t bank);

void abi_SoundFromTableD89(void);
void abi_OamBufferClear(void);
void abi_OamFromList(void);
void abi_OamDmaAll(void);
void abi_GradientFillCgadd(void);
void abi_MenuCourseScreenSetup(void);
void abi_PaletteCourseLoad(void);
void abi_GsuCF42ToVram(void);
void abi_RaceVarsReset(void);
void abi_TimeUnpackBcdLong(void);

typedef struct { uint16_t a; uint8_t v; } HudByte;

/* $03:DE39-$DEC8: 12 sprite macros (x8) */
static const HudByte hide_sprites[12] = {
    {0x0546, 0xF0}, {0x054A, 0xF0}, {0x054E, 0xF0}, {0x0552, 0xF0},
    {0x0556, 0xF0}, {0x055A, 0xF0}, {0x055E, 0xF0}, {0x0562, 0xF0},
    {0x0566, 0xF0}, {0x056A, 0xF0}, {0x056E, 0xF0}, {0x0572, 0xF0},
};
/* $03:DF61-$E050: 10 sprite macros (x8) */
static const HudByte best_time_sprites[30] = {
    {0x0465, 0xAC}, {0x0466, 0xB8}, {0x0468, 0x36}, {0x0469, 0xAC},
    {0x046A, 0xC0}, {0x046C, 0x36}, {0x046D, 0xBC}, {0x046E, 0xB8},
    {0x0470, 0x36}, {0x0471, 0xBC}, {0x0472, 0xC0}, {0x0474, 0x36},
    {0x04D5, 0xC4}, {0x04D6, 0xB8}, {0x04D8, 0x36}, {0x04D9, 0xC4},
    {0x04DA, 0xC0}, {0x04DC, 0x36}, {0x04DD, 0xD4}, {0x04DE, 0xB8},
    {0x04E0, 0x36}, {0x04E1, 0xD4}, {0x04E2, 0xC0}, {0x04E4, 0x36},
    {0x04E5, 0xDC}, {0x04E6, 0xB8}, {0x04E8, 0x36}, {0x04E9, 0xDC},
    {0x04EA, 0xC0}, {0x04EC, 0x36},
};
/* $03:E0F8-$E223: 10 sprite macros (x16) */
static const HudByte no_time_sprites[40] = {
    {0x0465, 0xAB}, {0x0466, 0xBC}, {0x0467, 0x3F}, {0x0468, 0x36},
    {0x0469, 0xAB}, {0x046A, 0xBC}, {0x046B, 0x3F}, {0x046C, 0x36},
    {0x046D, 0xBB}, {0x046E, 0xBC}, {0x046F, 0x3F}, {0x0470, 0x36},
    {0x0471, 0xBB}, {0x0472, 0xBC}, {0x0473, 0x3F}, {0x0474, 0x36},
    {0x04D5, 0xC3}, {0x04D6, 0xBC}, {0x04D7, 0x3F}, {0x04D8, 0x36},
    {0x04D9, 0xC3}, {0x04DA, 0xBC}, {0x04DB, 0x3F}, {0x04DC, 0x36},
    {0x04DD, 0xD5}, {0x04DE, 0xBC}, {0x04DF, 0x3F}, {0x04E0, 0x36},
    {0x04E9, 0xD5}, {0x04EA, 0xBC}, {0x04EB, 0x3F}, {0x04EC, 0x36},
    {0x04E5, 0xDD}, {0x04E6, 0xBC}, {0x04E7, 0x3F}, {0x04E8, 0x36},
    {0x04E9, 0xDD}, {0x04EA, 0xBC}, {0x04EB, 0x3F}, {0x04EC, 0x36},
};
/* $03:E224-$E25F: 2 sprite macros (x16) */
static const HudByte flag_sprites[8] = {
    {0x04ED, 0xB4}, {0x04EE, 0xB8}, {0x04EF, 0x14}, {0x04F0, 0x36},
    {0x04F1, 0xCC}, {0x04F2, 0xB8}, {0x04F3, 0x15}, {0x04F4, 0x36},
};
/* $03:E282-$E311: 12 sprite macros (x16) */
static const HudByte stunt_record_sprites[12] = {
    {0x0545, 0xB4}, {0x0549, 0xCA}, {0x054D, 0xD2}, {0x0551, 0xD2},
    {0x0555, 0xDA}, {0x0559, 0xDA}, {0x055D, 0xBA}, {0x0561, 0xBA},
    {0x0565, 0xC2}, {0x0569, 0xC2}, {0x056D, 0xAC}, {0x0571, 0xAC},
};
/* $03:E38D-$E4D0: 12 sprite macros (x8) */
static const HudByte stunt_score_sprites[42] = {
    {0x0465, 0xD0}, {0x0466, 0xF0}, {0x0467, 0x3F}, {0x0468, 0x36},
    {0x046A, 0xF0}, {0x046D, 0xD8}, {0x046E, 0xF0}, {0x046F, 0x3F},
    {0x0470, 0x36}, {0x0472, 0xF0}, {0x04D5, 0xBD}, {0x04D6, 0xB8},
    {0x04D7, 0x00}, {0x04D8, 0x36}, {0x04D9, 0xBD}, {0x04DA, 0xC0},
    {0x04DB, 0x01}, {0x04DC, 0x36}, {0x04DD, 0xC5}, {0x04DE, 0xB8},
    {0x04DF, 0x00}, {0x04E0, 0x36}, {0x04E1, 0xC5}, {0x04E2, 0xC0},
    {0x04E3, 0x01}, {0x04E4, 0x36}, {0x04E5, 0xDD}, {0x04E6, 0xB8},
    {0x04E7, 0x08}, {0x04E8, 0x36}, {0x04E9, 0xDD}, {0x04EA, 0xC0},
    {0x04EB, 0x09}, {0x04EC, 0x36}, {0x04ED, 0xE5}, {0x04EE, 0xB8},
    {0x04EF, 0x00}, {0x04F0, 0x36}, {0x04F1, 0xE5}, {0x04F2, 0xC0},
    {0x04F3, 0x01}, {0x04F4, 0x36},
};
/* $03:E4F4-$E523: 2 sprite macros (x8) */
static const HudByte battle_alt_sprites[6] = {
    {0x046D, 0xAA}, {0x046E, 0xC0}, {0x0470, 0x36}, {0x0465, 0xE6},
    {0x0466, 0xC0}, {0x0468, 0x36},
};
/* $03:E537-$E566: 2 sprite macros (x8) */
static const HudByte battle_p1_sprites[6] = {
    {0x046D, 0xAA}, {0x046E, 0xB8}, {0x0470, 0x36}, {0x0471, 0xAA},
    {0x0472, 0xC0}, {0x0474, 0x36},
};
/* $03:E572-$E5A1: 2 sprite macros (x8) */
static const HudByte battle_p2_sprites[6] = {
    {0x0465, 0xE6}, {0x0466, 0xB8}, {0x0468, 0x36}, {0x0469, 0xE6},
    {0x046A, 0xC0}, {0x046C, 0x36},
};

void HudBestTimeDigits(Carried *cr);
void HudBestTimeSprites(Carried *cr);
void RaceHudSetup_cr(Carried *cr);
void RaceHudSetupTwoPlayers_cr(Carried *cr);
void RaceHudSpritesHide(Carried *cr);
void RecordOffsetAllCourses(Carried *cr);
static void digit_tiles(Carried *cr, uint32_t at, uint8_t d);
static void flags(Carried *cr);
static void hud_bytes(Carried *cr, const HudByte *t, unsigned n, int x8);
static void hud_end(Carried *cr);
static void lda8(Carried *cr, uint8_t v);
static int mode_all(Carried *cr, uint32_t b);
static void mode_speed_battle(Carried *cr, uint32_t b, int speed);
static void mode_stunt(Carried *cr, uint32_t b);
static void no_time(Carried *cr);
static void player_tiles(Carried *cr, uint32_t b, uint16_t from, uint16_t at);
static void race_hud_setup(Carried *cr);
static void race_hud_setup_two(Carried *cr);
static void slide_word(Carried *cr, uint16_t v);
static void time_tiles(Carried *cr, uint8_t min, uint8_t sec, uint8_t hund);
static void w_GradientFillCgadd(Carried *cr);
static void w_MenuCourseScreenSetup(Carried *cr);
void GsuCF42ToVram_cr(Carried *cr);
void OamBufferClear_cr(Carried *cr);
void OamDmaAll_cr(Carried *cr);
void OamFromList_cr(Carried *cr);
void PaletteCourseLoad_cr(Carried *cr);
void RaceVarsReset(Carried *cr);
void SoundFromTableD89_cr(Carried *cr);
void TimeUnpackBcdLong_cr(Carried *cr);

/* LDA #v with M 8-bit. */
static void lda8(Carried *cr, uint8_t v)
{
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    set_p(cr, nz8(cr->p, v));
}

/* A run of sprite macros: each byte LDA # / STA.L to $00:addr, the index
   registers 8-bit (SEP #$10) or 16-bit (REP #$10) after each. */
static void hud_bytes(Carried *cr, const HudByte *t, unsigned n, int x8)
{
    unsigned i;

    r_sep(cr, P_M);
    cyc(22);
    for (i = 0; i < n; i++) {
        lda8(cr, t[i].v);
        /* a new sprite macro (its bytes lie in one 4-byte group) comes after
           SEP #$20, SEP/REP #$10 and its own SEP #$20 */
        cyc(48 + (i && ((t[i].a - 0x465) >> 2) != ((t[i - 1].a - 0x465) >> 2) ? 66 : 0));
        wr8(t[i].a, t[i].v);
    }
    if (x8)
        { cyc(40); r_sep(cr, P_X); }
    else
        r_rep(cr, P_X);
}

/* A digit's two tiles (ASL, STA.L at, INC, STA.L at + 4): the digit's top
   tile d * 2 and its bottom tile d * 2 + 1. */
static void digit_tiles(Carried *cr, uint32_t at, uint8_t d)
{
    uint8_t v = (uint8_t)(d << 1);

    set_p(cr, (uint8_t)((cr->p & ~P_C) | (d >> 7)));
    lda8(cr, v);
    cyc(62);
    wr8(at, v);
    v++;
    lda8(cr, v);
    cyc(46);
    wr8(at + 4, v);
}

/* The five digits of a BCD time (minutes, seconds, hundredths) into the
   best time's sprites. */
static void time_tiles(Carried *cr, uint8_t min, uint8_t sec, uint8_t hund)
{
    cyc(min == 0xFF ? 86 : 0);          /* no time ($FFFF) or the unpacked BCD, after TimeUnpackBcdLong's RTL */
    digit_tiles(cr, W(wram_0467), (uint8_t)(min & 0x0F));
    cyc(54);
    digit_tiles(cr, W(wram_046F), (uint8_t)(sec >> 4));
    cyc(14);
    digit_tiles(cr, W(wram_04D7), (uint8_t)(sec & 0x0F));
    cyc(54);
    digit_tiles(cr, W(wram_04DF), (uint8_t)(hund >> 4));
    cyc(14);
    digit_tiles(cr, W(wram_04E7), (uint8_t)(hund & 0x0F));
}

/*
 * @routine 03:DE39 RaceHudSpritesHide
 * WHAT IT DOES: Hides sprites 116 to 127 (Y = $F0).
 * HOW: Twelve sprite macros, one byte each.
 * Entry: any widths. Exit: M 8-bit, X 8-bit; A low byte $F0; N set, Z
 * clear.
 */
void RaceHudSpritesHide(Carried *cr)
{
    cyc(0);
    hud_bytes(cr, hide_sprites, 12, 1);
    cyc(28);
}

void abi_RaceHudSpritesHide(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RaceHudSpritesHide(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:DF61 HudBestTimeSprites
 * WHAT IT DOES: Places the best time's ten sprites (X, Y, attributes $36):
 * two rows at Y $B8 and $C0 from X $AC to $DC; the tiles are the digits'.
 * HOW: Ten sprite macros.
 * Entry: any widths. Exit: M 8-bit, X 8-bit; A low byte $36; N, Z clear.
 */
void HudBestTimeSprites(Carried *cr)
{
    cyc(62);
    hud_bytes(cr, best_time_sprites, 30, 1);
}

void abi_HudBestTimeSprites(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    HudBestTimeSprites(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:DF10 HudBestTimeDigits
 * WHAT IT DOES: Shows the packed time A as the best time: its five digits
 * as two tiles each, then the sprites placed (HudBestTimeSprites).
 * HOW: TimeUnpackBcdLong, then 8-bit (X, Y high bytes cleared); P kept
 * (PHP, PLP).
 * Entry: M 16-bit, X 16-bit, D = $0000; A = the packed time. Exit: P as on
 * entry; A low byte $36; X = the seconds, Y = the hundredths (BCD, 8
 * bits).
 */
void HudBestTimeDigits(Carried *cr)
{
    uint8_t p = cr->p;

    (cyc(84), TimeUnpackBcdLong_cr(cr));
    r_sep(cr, P_M | P_X);
    time_tiles(cr, (uint8_t)cr->a, (uint8_t)cr->x, (uint8_t)cr->y);
    HudBestTimeSprites(cr);
    r_plp(cr, p);
    cyc(56);                            /* the rest of the SEPs, RTL and the RTL */
}

void abi_HudBestTimeDigits(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    HudBestTimeDigits(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:DEFD RecordOffsetAllCourses
 * WHAT IT DOES: The all-courses records' offset of course $0E3D: (course -
 * $14) * 5, in X.
 * HOW: 16-bit, the course kept in direct page $F1.
 * Entry: M 16-bit, X 16-bit, D = $0000. Exit: A = X = the offset; N
 * clear, Z from it; C, V from the add.
 */
void RecordOffsetAllCourses(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;
    uint8_t p;

    cyc(86);
    r_lda16(cr, b | W(course));
    r_sub16(cr, 0x0014);
    cyc(48);
    wr16((uint16_t)(cr->d + 0xF1), cr->a);
    r_asl16(cr);
    r_asl16(cr);
    p = (uint8_t)(cr->p & ~P_C);
    cyc(58);
    cr->a = alu_adc16(cr->a, rd16((uint16_t)(cr->d + 0xF1)), &p);
    set_p(cr, p);
    r_and16(cr, 0x00FF);
    r_tax16(cr);
}

void abi_RecordOffsetAllCourses(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RecordOffsetAllCourses(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* The best time's ten sprites with no time: tile $3F (blank) at Y $BC. */
static void no_time(Carried *cr)
{
    cyc(68);
    hud_bytes(cr, no_time_sprites, 40, 0);
}

/* The two flag sprites (tiles $14, $15 at Y $B8). */
static void flags(Carried *cr)
{
    cyc(22);
    hud_bytes(cr, flag_sprites, 8, 0);
}

/* $7F:3470 = v (16-bit). */
static void slide_word(Carried *cr, uint16_t v)
{
    r_rep(cr, P_M);
    r_lda16_imm(cr, v);
    wr16(W(slide_done), v);
}

/* HUD mode 3 (all courses): with $10CD set as mode 0. Otherwise the HUD's
   $0E16 = $EC, $0E18 = $E8, $7F:3470 = $7FFF and the best time: with
   $0D4C set the course's record from the save ($70:FC5E,
   RecordOffsetAllCourses, HudBestTimeDigits), else the best time at
   $0F61-$0F63 when $0F2B holds one; no time shows the dashes. Returns 1
   to go on as mode 0. */
static int mode_all(Carried *cr, uint32_t b)
{
    cyc(24);
    r_lda8(cr, b | W(wram_10CD));
    if ((uint8_t)cr->a)
        { cyc(46); return 1; }
    lda8(cr, 0xEC);
    cyc(62);
    wr8(b | W(wram_0E16), 0xEC);
    lda8(cr, 0xE8);
    cyc(40);
    wr8(b | W(wram_0E18), 0xE8);
    r_rep(cr, P_M | P_X);
    r_lda16_imm(cr, 0x7FFF);
    cyc(18);
    wr16(W(slide_done), 0x7FFF);
    cyc(24);
    r_lda16(cr, b | W(digit62_override));
    r_and16(cr, 0x00FF);
    if (cr->a) {
        cyc(38);
        RecordOffsetAllCourses(cr);
        cyc(82);
        r_lda16(cr, 0x70FC5E + cr->x);
        r_cmp16(cr, 0xFFFF);
        if (cr->a == 0xFFFF) {
            no_time(cr);
        } else {
            cyc(102);
            HudBestTimeDigits(cr);
            cyc(2);             /* JMP RaceHudFlags */
        }
        flags(cr);
        return 0;
    }
    r_rep(cr, P_M);
    cyc(68);
    r_lda16(cr, b | W(wram_0F2B));
    r_cmp16(cr, 0xFFFF);
    if (cr->a == 0xFFFF) {
        cyc(18);
        no_time(cr);
        flags(cr);
        return 0;
    }
    r_sep(cr, P_M);
    {
        uint8_t m, s, h;

        cyc(92);
        r_lda8(cr, b | W(wram_0F63));
        m = (uint8_t)cr->a;
        digit_tiles(cr, W(wram_0467), (uint8_t)(m & 0x0F));
        cyc(24);
        r_lda8(cr, b | W(wram_0F62));
        s = (uint8_t)cr->a;
        cyc(40);
        digit_tiles(cr, W(wram_046F), (uint8_t)(s >> 4));
        cyc(24);
        r_lda8(cr, b | W(wram_0F62));
        s = (uint8_t)cr->a;
        digit_tiles(cr, W(wram_04D7), (uint8_t)(s & 0x0F));
        cyc(24);
        r_lda8(cr, b | W(wram_0F61));
        h = (uint8_t)cr->a;
        cyc(40);
        digit_tiles(cr, W(wram_04DF), (uint8_t)(h >> 4));
        cyc(24);
        r_lda8(cr, b | W(wram_0F61));
        h = (uint8_t)cr->a;
        digit_tiles(cr, W(wram_04E7), (uint8_t)(h & 0x0F));
    }
    HudBestTimeSprites(cr);
    cyc(28);                    /* its SEPs and RTL, then JMP RaceHudFlags */
    flags(cr);
    return 0;
}

/* HUD mode 1 (Stunt Trax). With $0D4E set (a course's records run):
   $7F:3470 = $7FFF, $0E16 = $AC, $0E18 = $8C, sprites 116-127 placed, and
   the course's record time from the save ($70:FC4A + (course - $17) * 5)
   as the best time (or the dashes).
   Otherwise: $0E18 = $F0, sprites 116-127 hidden, the score's sprites and
   $7F:3470 = $2127. */
static void mode_stunt(Carried *cr, uint32_t b)
{
    uint16_t x;
    uint8_t p;

    cyc(24);
    r_lda8(cr, b | W(hud_blank));
    if ((uint8_t)cr->a) {
        cyc(40);
        slide_word(cr, 0x7FFF);
        r_lda16_imm(cr, 0x00AC);
        cyc(10);
        wr16(b | W(wram_0E16), 0x00AC);
        r_lda16_imm(cr, 0x008C);
        cyc(10);
        wr16(b | W(wram_0E18), 0x008C);
        cyc(22);
        hud_bytes(cr, stunt_record_sprites, 12, 0);
        r_rep(cr, P_M | P_X);
        cyc(46);
        r_lda16(cr, b | W(course));
        r_and16(cr, 0x00FF);
        r_sub16(cr, 0x0017);
        cyc(70);
        wr16((uint16_t)(cr->d + 0xF1), cr->a);
        r_asl16(cr);
        r_asl16(cr);
        p = (uint8_t)(cr->p & ~P_C);
        cyc(58);
        cr->a = alu_adc16(cr->a, rd16((uint16_t)(cr->d + 0xF1)), &p);
        set_p(cr, p);
        r_tax16(cr);
        x = cr->x;
        cyc(16);
        r_lda16(cr, 0x70FC4A + x);
        r_cmp16(cr, 0xFFFF);
        if (cr->a == 0xFFFF) {
            cyc(24);
            no_time(cr);
        } else {
            cyc(108);
            TimeUnpackBcdLong_cr(cr);
            r_sep(cr, P_M | P_X);
            time_tiles(cr, (uint8_t)cr->a, (uint8_t)cr->x, (uint8_t)cr->y);
            HudBestTimeSprites(cr);
            cyc(28);            /* its SEPs and RTL, then JMP RaceHudFlags */
        }
        flags(cr);
        cyc(22);
        return;
    }
    lda8(cr, 0xF0);
    cyc(86);
    wr8(b | W(wram_0E18), 0xF0);
    (cyc(46), RaceHudSpritesHide(cr));
    cyc(18);
    hud_bytes(cr, stunt_score_sprites, 42, 1);
    slide_word(cr, 0x2127);
}

/* LDA $addr, ASL, STA.W at, INC, STA.W at + 4: a player's number as its
   two tiles. */
static void player_tiles(Carried *cr, uint32_t b, uint16_t from, uint16_t at)
{
    uint8_t v;

    r_lda8(cr, b | from);
    v = (uint8_t)cr->a;
    set_p(cr, (uint8_t)((cr->p & ~P_C) | (v >> 7)));
    v = (uint8_t)(v << 1);
    lda8(cr, v);
    cyc(38);
    wr8(b | at, v);
    v++;
    lda8(cr, v);
    cyc(38);
    wr8(b | (uint16_t)(at + 4), v);
}

/* HUD modes 0 (Speed Trax: first $0E18 = $8C, $0E16 = $EC) and 2 (Battle
   Trax). With $10CD set: two sprites with tile $3F, sprites 116-127
   hidden, $7F:3470 = $7FFF. Otherwise the players' tiles from $0DF4 and
   $0DF8 with their sprites, and $7F:3470 = $3947 with $0D4C set, else
   $7FFF. */
static void mode_speed_battle(Carried *cr, uint32_t b, int speed)
{
    if (speed) {
        lda8(cr, 0x8C);
        cyc(40);
        wr8(b | W(wram_0E18), 0x8C);
        lda8(cr, 0xEC);
        cyc(40);
        wr8(b | W(wram_0E16), 0xEC);
    }
    cyc(24);
    r_lda8(cr, b | W(wram_10CD));
    if ((uint8_t)cr->a) {
        lda8(cr, 0x3F);
        cyc(56);
        wr8(b | W(wram_046F), 0x3F);
        cyc(24);
        wr8(b | W(wram_0467), 0x3F);
        hud_bytes(cr, battle_alt_sprites, 6, 1);
        r_sep(cr, P_M);
        (cyc(50), RaceHudSpritesHide(cr));
    } else {
        cyc(46);
        player_tiles(cr, b, 0x0DF4, 0x046F);
        hud_bytes(cr, battle_p1_sprites, 6, 1);
        cyc(6);
        player_tiles(cr, b, 0x0DF8, 0x0467);
        hud_bytes(cr, battle_p2_sprites, 6, 1);
        r_sep(cr, P_M);
        cyc(28);
        r_lda8(cr, b | W(digit62_override));
        if ((uint8_t)cr->a) {
            cyc(34);
            slide_word(cr, 0x3947);
            cyc(6);                     /* BRA */
            return;
        } else { cyc(2); }
    }
    cyc(38);
    slide_word(cr, 0x7FFF);
    r_sep(cr, P_M);
    cyc(46);
    r_lda8(cr, b | W(wram_10CD));
    if (!(uint8_t)cr->a)
        cyc(6);                         /* BEQ taken */
}

/* The end of both set-ups: RaceVarsReset, SEP #$20. */
static void hud_end(Carried *cr)
{
    (cyc(62), RaceVarsReset(cr));
    r_sep(cr, P_M);
    cyc(8);
}

/* The one-player HUD (from $03:DECA up to the end's PLB, PLP). */
static void race_hud_setup(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;
    uint16_t h;
    unsigned i;
    static const uint16_t hdma_bytes[9] = {0x20, 0x13, 0x40, 0x12, 0x40, 0x12, 0x01, 0x13, 0x00};

    r_sep(cr, P_M);
    (cyc(84), SoundFromTableD89_cr(cr));
    r_rep(cr, P_M | P_X);
    r_lda16_imm(cr, 0xA325);
    cyc(18);
    wr16(0x70004E, 0xA325);
    (cyc(62), OamBufferClear_cr(cr));
    r_sep(cr, P_M);
    r_rep(cr, P_X);
    cr->x = 0x0000;
    cr->y = 0x0000;
    set_p(cr, nz16(cr->p, 0));
    (cyc(116), OamFromList_cr(cr));
    cyc(42);                            /* RTS */
    cr->x = 0x0200;
    cr->y = 0x01AA;
    set_p(cr, nz16(cr->p, 0x01AA));
    (cyc(52), OamFromList_cr(cr));
    r_sep(cr, P_M | P_X);
    b = (uint32_t)cr->db << 16;
    cyc(24);
    r_lda8(cr, b | W(hud_mode));
    cr->a = (uint16_t)((cr->a & 0xFF00) | (uint8_t)(cr->a << 1));
    set_p(cr, nz8(cr->p, (uint8_t)cr->a));
    cr->x = (uint8_t)cr->a;
    cyc(74);
    h = rd16(0x030000 | (uint16_t)(0xE052 + cr->x));
    switch (h) {
    case 0xE05A:
        if (mode_all(cr, b))
            mode_speed_battle(cr, b, 1); else { cyc(30); }
        break;
    case 0xE263: mode_stunt(cr, b); cyc(8); break;
    case 0xE4DD: mode_speed_battle(cr, b, 1); break;
    case 0xE4E7: mode_speed_battle(cr, b, 0); break;
    default: carried_jump(cr, 0x030000 | h); return;
    }
    /* $03:E5C4: the sprite tables out, scrolls, HUD positions, HDMA. */
    cyc(100);
    r_sep(cr, P_M);
    r_rep(cr, P_X);
    OamDmaAll_cr(cr);
    r_sep(cr, P_M | P_X);
    b = (uint32_t)cr->db << 16;
    cyc(24);
    ppu_wr(b | BG1HOFS, 0x00);
    cyc(24);
    ppu_wr(b | BG1HOFS, 0x00);
    lda8(cr, 0xEF);
    cyc(40);
    ppu_wr(b | BG1VOFS, 0xEF);
    cr->x = 0xFF;
    set_p(cr, nz8(cr->p, 0xFF));
    cyc(40);
    ppu_wr(b | BG1VOFS, 0xFF);
    cyc(24);
    ppu_wr(b | BG2HOFS, 0x00);
    cyc(24);
    ppu_wr(b | BG2HOFS, 0x00);
    lda8(cr, 0x80);
    cyc(40);
    ppu_wr(b | BG2VOFS, 0x80);
    cyc(24);
    ppu_wr(b | BG2VOFS, 0x00);
    lda8(cr, 0x04);
    cyc(40);
    wr8(b | W(wram_0D3F), 0x04);
    lda8(cr, 0xFF);
    cyc(40);
    wr8(b | W(wram_0E46), 0xFF);
    cyc(24);
    wr8(b | W(wram_0E45), 0xFF);
    cyc(24);
    wr8(b | W(gsu_buffer_offset), 0x00);
    lda8(cr, 0xC4);
    cyc(40);
    wr8(b | W(gauge_b6_x), 0xC4);
    lda8(cr, 0x93);
    cyc(40);
    wr8(b | W(wram_0E14), 0x93);
    lda8(cr, 0x2A);
    cyc(40);
    wr8(b | W(map_b_x), 0x2A);
    lda8(cr, 0x7A);
    cyc(56);
    wr8(b | W(wram_0E0C), 0x7A);
    lda8(cr, 0x00);
    cyc(40);
    wr8(b | W(map_b_y_w), 0x00);
    lda8(cr, 0xE0);
    cyc(40);
    wr8(b | W(hud_slide), 0xE0);
    lda8(cr, 0x20);
    cyc(40);
    wr8(b | W(wram_0D58), 0x20);
    lda8(cr, 0x50);
    cyc(40);
    wr8(b | W(gauge_b4_x), 0x50);
    lda8(cr, 0xB8);
    cyc(40);
    wr8(b | W(gauge_b4_y), 0xB8);
    lda8(cr, 0x00);
    cyc(62);
    dma_wr(b | DMAP1, 0x00);
    lda8(cr, 0x2C);
    cyc(40);
    dma_wr(b | BBAD1, 0x2C);
    lda8(cr, 0x7F);
    cyc(40);
    dma_wr(b | A1B1, 0x7F);
    r_rep(cr, P_M);
    r_lda16_imm(cr, 0x3480);
    cyc(10);
    dma_wr16(b | A1T1L, 0x3480);
    r_sep(cr, P_M);
    cyc(22);
    for (i = 0; i < 9; i++) {
        lda8(cr, (uint8_t)hdma_bytes[i]);
        cyc(48);
        wr8(W(wram_7F3480) + i, (uint8_t)hdma_bytes[i]);
    }
    lda8(cr, 0x00);
    cyc(40);
    dma_wr(b | DMAP2, 0x00);
    lda8(cr, 0x21);
    cyc(40);
    dma_wr(b | BBAD2, 0x21);
    lda8(cr, 0x7F);
    cyc(40);
    dma_wr(b | A1B2, 0x7F);
    r_rep(cr, P_M);
    r_lda16_imm(cr, 0x3497);
    cyc(10);
    dma_wr16(b | A1T2L, 0x3497);
    r_lda16_imm(cr, 0x0042);
    cyc(10);
    dma_wr16(b | DMAP3, 0x0042);
    r_lda16_imm(cr, 0x0022);
    cyc(10);
    dma_wr16(b | BBAD3, 0x0022);
    r_lda16_imm(cr, 0x007F);
    cyc(10);
    dma_wr16(b | A1B3, 0x007F);
    r_lda16_imm(cr, 0x3489);
    cyc(32);
    dma_wr16(b | A1T3L, 0x3489);
    r_rep(cr, P_M | P_X);
    r_lda16_imm(cr, 0x0120);
    cyc(18);
    wr16(W(gradient_cgadd), 0x0120);
    cr->y = 0x0080;
    cr->x = 0x0002;
    r_lda16_imm(cr, 0x0181);
    (cyc(80), w_GradientFillCgadd(cr));
    r_lda16_imm(cr, 0x0000);
    cyc(18);
    wr16(W(gradient_cgadd) + cr->x, 0x0000);
    r_lda16_imm(cr, 0xEC57);
    cyc(18);
    wr16(W(wram_7F348A), 0xEC57);
    r_sep(cr, P_M);
    lda8(cr, 0x20);
    cyc(70);
    wr8(W(gradient_cgdata), 0x20);
    lda8(cr, 0xFF);
    cyc(48);
    wr8(W(wram_7F348C), 0xFF);
    lda8(cr, 0x00);
    cyc(48);
    wr8(W(wram_7F348F), 0x00);
    lda8(cr, 0x7F);
    cyc(40);
    dma_wr(b | DASB3, 0x7F);
    lda8(cr, 0x03);
    cyc(40);
    wr8(b | W(gsu_scmr_shadow), 0x03);
    r_rep(cr, P_M);
    r_lda16_imm(cr, 0x0001);
    cyc(18);
    wr16(0x7001C4, 0x0001);
    (cyc(24), hud_end(cr));
}

/* The two-player HUD (from $03:E6FA up to the end's PLB, PLP): the Super
   FX's $70:004E = $A545 and $70:01C4 = 2, $0374 = 1, the course screen's
   set-up (MenuCourseScreenSetup, PaletteCourseLoad); then the split
   screen's sprites from the tables at $03:E7EA (eight, each into both
   halves' sprite lists 64 sprites apart: Y - 8 and + $60 lines), six
   blank sprites ($0425-$042F, $0525-$052F: position 0, tile $26A0), nine
   from $03:E80F at offsets $2C28 and $9476, the size bits from $03:E80A
   (five bytes into $0580 and $0590); the Super FX blocks $F845/$F804 to
   VRAM (GsuCF42ToVram). */
static void race_hud_setup_two(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;
    uint16_t v;

    r_rep(cr, P_M | P_X);
    r_lda16_imm(cr, 0xA545);
    cyc(18);
    wr16(0x70004E, 0xA545);
    r_sep(cr, P_M);
    lda8(cr, 0x01);
    cyc(62);
    wr8(b | W(gsu_scmr_shadow), 0x01);
    r_rep(cr, P_M);
    r_lda16_imm(cr, 0x0002);
    cyc(18);
    wr16(0x7001C4, 0x0002);
    (cyc(62), w_MenuCourseScreenSetup(cr));
    (cyc(46), PaletteCourseLoad_cr(cr));
    r_rep(cr, P_M | P_X);
    b = (uint32_t)cr->db << 16;
    cr->x = 0x00BC;
    for (cr->y = 0x0000; cr->y < 0x0020; ) {
        cyc(164);
        v = (uint16_t)(rd16(b | (uint16_t)(0xE7EA + cr->y)) - 0x0800);
        wr16(W(oam_buffer) + cr->x, v);
        v = (uint16_t)(v + 0x6850);
        cyc(70);
        wr16(W(wram_0475) + cr->x, v);
        cyc(78);
        v = rd16(b | (uint16_t)(0xE7EC + cr->y));
        wr16(W(wram_0377) + cr->x, v);
        cyc(32);
        wr16(W(wram_0477) + cr->x, v);
        cr->x = (uint16_t)(cr->x + 4);
        cr->y = (uint16_t)(cr->y + 4);
        cyc(110);
    }
    r_lda16_imm(cr, 0x0000);
    cyc(52);
    wr16(b | W(wram_0425), 0x0000);
    cyc(24);
    wr16(b | W(wram_0429), 0x0000);
    cyc(24);
    wr16(b | W(wram_042D), 0x0000);
    cyc(24);
    wr16(b | W(wram_0525), 0x0000);
    cyc(24);
    wr16(b | W(wram_0529), 0x0000);
    cyc(24);
    wr16(b | W(wram_052D), 0x0000);
    r_lda16_imm(cr, 0x26A0);
    cyc(10);
    wr16(b | W(results_split_tile), 0x26A0);
    cyc(24);
    wr16(b | W(wram_042B), 0x26A0);
    cyc(24);
    wr16(b | W(wram_042F), 0x26A0);
    cyc(24);
    wr16(b | W(results_split2_tile), 0x26A0);
    cyc(24);
    wr16(b | W(wram_052B), 0x26A0);
    cyc(24);
    wr16(b | W(wram_052F), 0x26A0);
    cr->x = 0x00DC;
    for (cr->y = 0x0000; cr->y < 0x0024; ) {
        cyc(162);
        wr16(b | (uint16_t)(0x0375 + cr->x), (uint16_t)(rd16(b | (uint16_t)(0xE80F + cr->y)) + 0x2C28));
        cyc(76);
        wr16(b | (uint16_t)(0x0377 + cr->x), rd16(b | (uint16_t)(0xE811 + cr->y)));
        cr->x = (uint16_t)(cr->x + 4);
        cr->y = (uint16_t)(cr->y + 4);
        cyc(110);
    }
    cr->x = 0x01DC;
    for (cr->y = 0x0000; cr->y < 0x0024; ) {
        cyc(204);
        wr16(b | (uint16_t)(0x0375 + cr->x), (uint16_t)(rd16(b | (uint16_t)(0xE80F + cr->y)) + 0x9476));
        cyc(76);
        v = rd16(b | (uint16_t)(0xE811 + cr->y));
        cr->a = v;
        wr16(b | (uint16_t)(0x0377 + cr->x), v);
        cr->x = (uint16_t)(cr->x + 4);
        cr->y = (uint16_t)(cr->y + 4);
        cyc(68);
    }
    cyc(80);
    r_sep(cr, P_M);
    cr->x = 0x000B;
    for (cr->y = 0x0000; cr->y < 0x0005; ) {
        cyc(142);
        uint8_t s = rd8(b | (uint16_t)(0xE80A + cr->y));

        lda8(cr, s);
        wr8(b | (uint16_t)(0x0575 + cr->x), s);
        cyc(30);
        wr8(b | (uint16_t)(0x0585 + cr->x), s);
        cr->x++;
        cr->y++;
    }
    cyc(178);
    set_p(cr, (uint8_t)((cr->p & ~(P_N | P_Z)) | P_Z | P_C));
    r_rep(cr, P_M | P_X);
    cr->y = 0xF845;
    cr->x = 0xF804;
    set_p(cr, nz16(cr->p, 0xF804));
    GsuCF42ToVram_cr(cr);
    (cyc(0), hud_end(cr));
}

/* PLB, PLP (the data bank and P RaceHudLoad pushed), RTL: for an entry
   from original code, both pulled from the stack and the return taken
   through it. */
static void hud_leave_stack(Carried *cr)
{
    uint16_t lo;
    uint8_t p;

    cyc(34);                            /* SEP and PLB, after hud_end's 8 */
    cr->db = rd8((uint16_t)(cpu.s + 1));
    cpu.s = (uint16_t)(cpu.s + 1);
    cr->p = nz8(cr->p, cr->db);
    cyc(20);
    p = rd8((uint16_t)(cpu.s + 1));
    set_p(cr, p);
    if (p & P_X) {
        cr->x &= 0x00FF;
        cr->y &= 0x00FF;
    }
    cpu.s = (uint16_t)(cpu.s + 1);
    cyc(20);                            /* RTL */
    lo = (uint16_t)(rd16((uint16_t)(cpu.s + 1)) + 1);
    cpu_leave_to((uint32_t)rd8((uint16_t)(cpu.s + 3)) << 16 | lo);
    cpu.s = (uint16_t)(cpu.s + 3);
}

/*
 * @routine 03:DECA RaceHudSetup
 * WHAT IT DOES: The one-player race HUD after RaceHudLoad: sounds
 * (SoundFromTableD89), the Super FX's $70:004E = $A325, the sprite table
 * cleared (OamBufferClear) and filled from the lists at $F883 and
 * $F883 + $01AA (OamFromList); the HUD mode's sprites by HUD mode $0D2B
 * (the table at $03:E052): 0 Speed Trax and 2 Battle Trax the players'
 * numbers (or, with $10CD set, two blank sprites), 1 Stunt Trax the score
 * (or, with $0D4E, the course's record time), 3 all courses the best time
 * (or as mode 0 with $10CD); then the sprite tables out (OamDmaAll), the
 * backgrounds' scroll (BG1 V $FFEF, BG2 V $0080), the HUD's positions
 * ($0D3F = 4, $0E09-$0E14, $0D57), HDMA channels 1-3 (the table at
 * $7F:3480, $7F:3497 filled by GradientFillCgadd, $7F:3489), $0374 = 3,
 * $70:01C4 = 1, RaceVarsReset; then the data bank and P pushed by
 * RaceHudLoad are pulled and it returns to RaceHudLoad's caller.
 * HOW: Sprite bytes by long stores in runs of a macro; JMP (abs,X) by HUD
 * mode.
 * Entry: data bank $03; RaceHudLoad's P and data bank on the stack. Exit:
 * P and the data bank as RaceHudLoad found them; A, X, Y as
 * RaceVarsReset leaves them.
 */
void RaceHudSetup(Carried *cr, uint8_t p, uint8_t db)
{
    race_hud_setup(cr);
    cr->db = db;
    cr->p = nz8(cr->p, db);
    r_plp(cr, p);
}

void RaceHudSetup_cr(Carried *cr)
{
    race_hud_setup(cr);
    hud_leave_stack(cr);
}

void abi_RaceHudSetup(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RaceHudSetup_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:E6FA RaceHudSetupTwoPlayers
 * WHAT IT DOES: The two-player race HUD after RaceHudLoad: the Super FX's
 * $70:004E = $A545 and $70:01C4 = 2, $0374 = 1, the course screen's set-up
 * (MenuCourseScreenSetup, PaletteCourseLoad), the split screen's sprites
 * (from the tables at $03:E7EA, $03:E80F and $03:E80A, into both halves),
 * the Super FX blocks $F845/$F804 to VRAM (GsuCF42ToVram), RaceVarsReset;
 * then as RaceHudSetup, the pushed data bank and P pulled.
 * HOW: 16-bit copy loops, the size bits 8-bit.
 * Entry: as RaceHudSetup. Exit: as RaceHudSetup.
 */
void RaceHudSetupTwoPlayers(Carried *cr, uint8_t p, uint8_t db)
{
    race_hud_setup_two(cr);
    cr->db = db;
    cr->p = nz8(cr->p, db);
    r_plp(cr, p);
}

void RaceHudSetupTwoPlayers_cr(Carried *cr)
{
    race_hud_setup_two(cr);
    hud_leave_stack(cr);
}

void abi_RaceHudSetupTwoPlayers(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RaceHudSetupTwoPlayers_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* GradientFillCgadd as its adapter leaves the registers. */
static void w_GradientFillCgadd(Carried *cr)
{
    cr->x = GradientFillCgadd(cr->a, cr->x, cr->y);
    cr->y = 0;
    set_p(cr, (uint8_t)((cr->p & ~P_N) | P_Z));
}

/* MenuCourseScreenSetup as its adapter leaves the registers. */
static void w_MenuCourseScreenSetup(Carried *cr)
{
    cyc(106);
    cr->a = MenuCourseScreenSetup(cr->db);
    cr->x = 0x0020;
    cr->y = 0x0020;
}
