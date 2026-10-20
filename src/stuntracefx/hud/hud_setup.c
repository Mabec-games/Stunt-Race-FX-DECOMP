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
#include "cpu_ops.h"
#include "hw.h"

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

/* LDA #v with M 8-bit. */
static void lda8(uint8_t v)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, v);
}

/* A run of sprite macros: each byte LDA # / STA.L to $00:addr, the index
   registers 8-bit (SEP #$10) or 16-bit (REP #$10) after each. */
static void hud_bytes(const HudByte *t, unsigned n, int x8)
{
    unsigned i;

    op_sep(P_M);
    cyc(22);
    for (i = 0; i < n; i++) {
        lda8(t[i].v);
        /* a new sprite macro (its bytes lie in one 4-byte group) comes after
           SEP #$20, SEP/REP #$10 and its own SEP #$20 */
        cyc(48 + (i && ((t[i].a - 0x465) >> 2) != ((t[i - 1].a - 0x465) >> 2) ? 66 : 0));
        wr8(t[i].a, t[i].v);
    }
    if (x8)
        { cyc(40); op_sep(P_X); }
    else
        op_rep(P_X);
}

/* A digit's two tiles (ASL, STA.L at, INC, STA.L at + 4): the digit's top
   tile d * 2 and its bottom tile d * 2 + 1. */
static void digit_tiles(uint32_t at, uint8_t d)
{
    uint8_t v = (uint8_t)(d << 1);

    cpu.p = (uint8_t)((cpu.p & ~P_C) | (d >> 7));
    lda8(v);
    cyc(62);
    wr8(at, v);
    v++;
    lda8(v);
    cyc(46);
    wr8(at + 4, v);
}

/* The five digits of a BCD time (minutes, seconds, hundredths) into the
   best time's sprites. */
static void time_tiles(uint8_t min, uint8_t sec, uint8_t hund)
{
    cyc(min == 0xFF ? 86 : 0);          /* no time ($FFFF) or the unpacked BCD, after TimeUnpackBcdLong's RTL */
    digit_tiles(0x000467, (uint8_t)(min & 0x0F));
    cyc(54);
    digit_tiles(0x00046F, (uint8_t)(sec >> 4));
    cyc(14);
    digit_tiles(0x0004D7, (uint8_t)(sec & 0x0F));
    cyc(54);
    digit_tiles(0x0004DF, (uint8_t)(hund >> 4));
    cyc(14);
    digit_tiles(0x0004E7, (uint8_t)(hund & 0x0F));
}

/*
 * @routine 03:DE39 RaceHudSpritesHide
 * WHAT IT DOES: Hides sprites 116 to 127 (Y = $F0).
 * HOW: Twelve sprite macros, one byte each.
 * Entry: any widths. Exit: M 8-bit, X 8-bit; A low byte $F0; N set, Z
 * clear.
 */
void abi_RaceHudSpritesHide(void)
{
    cyc(0);
    hud_bytes(hide_sprites, 12, 1);
    cyc(28);
}

/*
 * @routine 03:DF61 HudBestTimeSprites
 * WHAT IT DOES: Places the best time's ten sprites (X, Y, attributes $36):
 * two rows at Y $B8 and $C0 from X $AC to $DC; the tiles are the digits'.
 * HOW: Ten sprite macros.
 * Entry: any widths. Exit: M 8-bit, X 8-bit; A low byte $36; N, Z clear.
 */
void abi_HudBestTimeSprites(void)
{
    cyc(62);
    hud_bytes(best_time_sprites, 30, 1);
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
void abi_HudBestTimeDigits(void)
{
    uint8_t p = cpu.p;

    (cyc(84), abi_TimeUnpackBcdLong());
    op_sep(P_M | P_X);
    time_tiles((uint8_t)cpu.a, (uint8_t)cpu.x, (uint8_t)cpu.y);
    abi_HudBestTimeSprites();
    op_plp(p);
}

/*
 * @routine 03:DEFD RecordOffsetAllCourses
 * WHAT IT DOES: The all-courses records' offset of course $0E3D: (course -
 * $14) * 5, in X.
 * HOW: 16-bit, the course kept in direct page $F1.
 * Entry: M 16-bit, X 16-bit, D = $0000. Exit: A = X = the offset; N
 * clear, Z from it; C, V from the add.
 */
void abi_RecordOffsetAllCourses(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint8_t p;

    cyc(86);
    op_lda16(b | 0x0E3D);
    op_sub16(0x0014);
    cyc(48);
    wr16((uint16_t)(cpu.d + 0xF1), cpu.a);
    op_asl16();
    op_asl16();
    p = (uint8_t)(cpu.p & ~P_C);
    cyc(58);
    cpu.a = op_adc16(cpu.a, rd16((uint16_t)(cpu.d + 0xF1)), &p);
    cpu.p = p;
    op_and16(0x00FF);
    op_tax16();
}

/* The best time's ten sprites with no time: tile $3F (blank) at Y $BC. */
static void no_time(void)
{
    cyc(68);
    hud_bytes(no_time_sprites, 40, 0);
}

/* The two flag sprites (tiles $14, $15 at Y $B8). */
static void flags(void)
{
    cyc(22);
    hud_bytes(flag_sprites, 8, 0);
}

/* $7F:3470 = v (16-bit). */
static void slide_word(uint16_t v)
{
    op_rep(P_M);
    op_lda16_imm(v);
    wr16(0x7F3470, v);
}

/* HUD mode 3 (all courses): with $10CD set as mode 0. Otherwise the HUD's
   $0E16 = $EC, $0E18 = $E8, $7F:3470 = $7FFF and the best time: with
   $0D4C set the course's record from the save ($70:FC5E,
   RecordOffsetAllCourses, HudBestTimeDigits), else the best time at
   $0F61-$0F63 when $0F2B holds one; no time shows the dashes. Returns 1
   to go on as mode 0. */
static int mode_all(uint32_t b)
{
    cyc(24);
    op_lda8(b | 0x10CD);
    if ((uint8_t)cpu.a)
        { cyc(46); return 1; }
    lda8(0xEC);
    cyc(62);
    wr8(b | 0x0E16, 0xEC);
    lda8(0xE8);
    cyc(40);
    wr8(b | 0x0E18, 0xE8);
    op_rep(P_M | P_X);
    op_lda16_imm(0x7FFF);
    cyc(18);
    wr16(0x7F3470, 0x7FFF);
    cyc(24);
    op_lda16(b | 0x0D4C);
    op_and16(0x00FF);
    if (cpu.a) {
        cyc(38);
        abi_RecordOffsetAllCourses();
        cyc(82);
        op_lda16(0x70FC5E + cpu.x);
        op_cmp16(0xFFFF);
        if (cpu.a == 0xFFFF) {
            no_time();
        } else {
            cyc(102);
            abi_HudBestTimeDigits();
            cyc(58);            /* HudBestTimeSprites' SEPs, RTL, PLP, RTL; JMP RaceHudFlags */
        }
        flags();
        return 0;
    }
    op_rep(P_M);
    cyc(68);
    op_lda16(b | 0x0F2B);
    op_cmp16(0xFFFF);
    if (cpu.a == 0xFFFF) {
        cyc(18);
        no_time();
        flags();
        return 0;
    }
    op_sep(P_M);
    {
        uint8_t m, s, h;

        cyc(92);
        op_lda8(b | 0x0F63);
        m = (uint8_t)cpu.a;
        digit_tiles(0x000467, (uint8_t)(m & 0x0F));
        cyc(24);
        op_lda8(b | 0x0F62);
        s = (uint8_t)cpu.a;
        cyc(40);
        digit_tiles(0x00046F, (uint8_t)(s >> 4));
        cyc(24);
        op_lda8(b | 0x0F62);
        s = (uint8_t)cpu.a;
        digit_tiles(0x0004D7, (uint8_t)(s & 0x0F));
        cyc(24);
        op_lda8(b | 0x0F61);
        h = (uint8_t)cpu.a;
        cyc(40);
        digit_tiles(0x0004DF, (uint8_t)(h >> 4));
        cyc(24);
        op_lda8(b | 0x0F61);
        h = (uint8_t)cpu.a;
        digit_tiles(0x0004E7, (uint8_t)(h & 0x0F));
    }
    abi_HudBestTimeSprites();
    cyc(28);                    /* its SEPs and RTL, then JMP RaceHudFlags */
    flags();
    return 0;
}

/* HUD mode 1 (Stunt Trax). With $0D4E set (a course's records run):
   $7F:3470 = $7FFF, $0E16 = $AC, $0E18 = $8C, sprites 116-127 placed, and
   the course's record time from the save ($70:FC4A + (course - $17) * 5)
   as the best time (or the dashes).
   Otherwise: $0E18 = $F0, sprites 116-127 hidden, the score's sprites and
   $7F:3470 = $2127. */
static void mode_stunt(uint32_t b)
{
    uint16_t x;
    uint8_t p;

    cyc(24);
    op_lda8(b | 0x0D4E);
    if ((uint8_t)cpu.a) {
        cyc(40);
        slide_word(0x7FFF);
        op_lda16_imm(0x00AC);
        cyc(10);
        wr16(b | 0x0E16, 0x00AC);
        op_lda16_imm(0x008C);
        cyc(10);
        wr16(b | 0x0E18, 0x008C);
        cyc(22);
        hud_bytes(stunt_record_sprites, 12, 0);
        op_rep(P_M | P_X);
        cyc(46);
        op_lda16(b | 0x0E3D);
        op_and16(0x00FF);
        op_sub16(0x0017);
        cyc(70);
        wr16((uint16_t)(cpu.d + 0xF1), cpu.a);
        op_asl16();
        op_asl16();
        p = (uint8_t)(cpu.p & ~P_C);
        cyc(58);
        cpu.a = op_adc16(cpu.a, rd16((uint16_t)(cpu.d + 0xF1)), &p);
        cpu.p = p;
        op_tax16();
        x = cpu.x;
        cyc(16);
        op_lda16(0x70FC4A + x);
        op_cmp16(0xFFFF);
        if (cpu.a == 0xFFFF) {
            cyc(24);
            no_time();
        } else {
            cyc(108);
            abi_TimeUnpackBcdLong();
            op_sep(P_M | P_X);
            time_tiles((uint8_t)cpu.a, (uint8_t)cpu.x, (uint8_t)cpu.y);
            abi_HudBestTimeSprites();
            cyc(28);            /* its SEPs and RTL, then JMP RaceHudFlags */
        }
        flags();
        return;
    }
    lda8(0xF0);
    cyc(86);
    wr8(b | 0x0E18, 0xF0);
    (cyc(46), abi_RaceHudSpritesHide());
    cyc(18);
    hud_bytes(stunt_score_sprites, 42, 1);
    slide_word(0x2127);
}

/* LDA $addr, ASL, STA.W at, INC, STA.W at + 4: a player's number as its
   two tiles. */
static void player_tiles(uint32_t b, uint16_t from, uint16_t at)
{
    uint8_t v;

    op_lda8(b | from);
    v = (uint8_t)cpu.a;
    cpu.p = (uint8_t)((cpu.p & ~P_C) | (v >> 7));
    v = (uint8_t)(v << 1);
    lda8(v);
    cyc(38);
    wr8(b | at, v);
    v++;
    lda8(v);
    cyc(38);
    wr8(b | (uint16_t)(at + 4), v);
}

/* HUD modes 0 (Speed Trax: first $0E18 = $8C, $0E16 = $EC) and 2 (Battle
   Trax). With $10CD set: two sprites with tile $3F, sprites 116-127
   hidden, $7F:3470 = $7FFF. Otherwise the players' tiles from $0DF4 and
   $0DF8 with their sprites, and $7F:3470 = $3947 with $0D4C set, else
   $7FFF. */
static void mode_speed_battle(uint32_t b, int speed)
{
    if (speed) {
        lda8(0x8C);
        cyc(40);
        wr8(b | 0x0E18, 0x8C);
        lda8(0xEC);
        cyc(40);
        wr8(b | 0x0E16, 0xEC);
    }
    cyc(24);
    op_lda8(b | 0x10CD);
    if ((uint8_t)cpu.a) {
        lda8(0x3F);
        cyc(56);
        wr8(b | 0x046F, 0x3F);
        cyc(24);
        wr8(b | 0x0467, 0x3F);
        hud_bytes(battle_alt_sprites, 6, 1);
        op_sep(P_M);
        (cyc(50), abi_RaceHudSpritesHide());
    } else {
        cyc(46);
        player_tiles(b, 0x0DF4, 0x046F);
        hud_bytes(battle_p1_sprites, 6, 1);
        cyc(6);
        player_tiles(b, 0x0DF8, 0x0467);
        hud_bytes(battle_p2_sprites, 6, 1);
        op_sep(P_M);
        cyc(28);
        op_lda8(b | 0x0D4C);
        if ((uint8_t)cpu.a) {
            cyc(34);
            slide_word(0x3947);
            return;
        } else { cyc(2); }
    }
    cyc(38);
    slide_word(0x7FFF);
    op_sep(P_M);
    cyc(46);
    op_lda8(b | 0x10CD);
}

/* The end of both set-ups: RaceVarsReset, SEP #$20. */
static void hud_end(void)
{
    (cyc(62), abi_RaceVarsReset());
    op_sep(P_M);
    cyc(8);
}

/* The one-player HUD (from $03:DECA up to the end's PLB, PLP). */
static void race_hud_setup(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint16_t h;
    unsigned i;
    static const uint16_t hdma_bytes[9] = {0x20, 0x13, 0x40, 0x12, 0x40, 0x12, 0x01, 0x13, 0x00};

    op_sep(P_M);
    (cyc(84), abi_SoundFromTableD89());
    op_rep(P_M | P_X);
    op_lda16_imm(0xA325);
    cyc(18);
    wr16(0x70004E, 0xA325);
    (cyc(62), abi_OamBufferClear());
    op_sep(P_M);
    op_rep(P_X);
    cpu.x = 0x0000;
    cpu.y = 0x0000;
    cpu.p = nz16(cpu.p, 0);
    (cyc(116), abi_OamFromList());
    cyc(42);                            /* RTS */
    cpu.x = 0x0200;
    cpu.y = 0x01AA;
    cpu.p = nz16(cpu.p, 0x01AA);
    (cyc(52), abi_OamFromList());
    op_sep(P_M | P_X);
    b = (uint32_t)cpu.db << 16;
    cyc(24);
    op_lda8(b | 0x0D2B);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | (uint8_t)(cpu.a << 1));
    cpu.p = nz8(cpu.p, (uint8_t)cpu.a);
    cpu.x = (uint8_t)cpu.a;
    cyc(74);
    h = rd16(0x030000 | (uint16_t)(0xE052 + cpu.x));
    switch (h) {
    case 0xE05A:
        if (mode_all(b))
            mode_speed_battle(b, 1); else { cyc(30); }
        break;
    case 0xE263: mode_stunt(b); cyc(8); break;
    case 0xE4DD: mode_speed_battle(b, 1); cyc(6); break;
    case 0xE4E7: mode_speed_battle(b, 0); break;
    default: jump_routine(0x030000 | h); return;
    }
    /* $03:E5C4: the sprite tables out, scrolls, HUD positions, HDMA. */
    cyc(100);
    op_sep(P_M);
    op_rep(P_X);
    abi_OamDmaAll();
    op_sep(P_M | P_X);
    b = (uint32_t)cpu.db << 16;
    cyc(24);
    wr8(b | BG1HOFS, 0x00);
    cyc(24);
    wr8(b | BG1HOFS, 0x00);
    lda8(0xEF);
    cyc(40);
    wr8(b | BG1VOFS, 0xEF);
    cpu.x = 0xFF;
    cpu.p = nz8(cpu.p, 0xFF);
    cyc(40);
    wr8(b | BG1VOFS, 0xFF);
    cyc(24);
    wr8(b | BG2HOFS, 0x00);
    cyc(24);
    wr8(b | BG2HOFS, 0x00);
    lda8(0x80);
    cyc(40);
    wr8(b | BG2VOFS, 0x80);
    cyc(24);
    wr8(b | BG2VOFS, 0x00);
    lda8(0x04);
    cyc(40);
    wr8(b | 0x0D3F, 0x04);
    lda8(0xFF);
    cyc(40);
    wr8(b | 0x0E46, 0xFF);
    cyc(24);
    wr8(b | 0x0E45, 0xFF);
    cyc(24);
    wr8(b | 0x0D7D, 0x00);
    lda8(0xC4);
    cyc(40);
    wr8(b | 0x0E12, 0xC4);
    lda8(0x93);
    cyc(40);
    wr8(b | 0x0E14, 0x93);
    lda8(0x2A);
    cyc(40);
    wr8(b | 0x0E09, 0x2A);
    lda8(0x7A);
    cyc(56);
    wr8(b | 0x0E0C, 0x7A);
    lda8(0x00);
    cyc(40);
    wr8(b | 0x0E0B, 0x00);
    lda8(0xE0);
    cyc(40);
    wr8(b | 0x0D57, 0xE0);
    lda8(0x20);
    cyc(40);
    wr8(b | 0x0D58, 0x20);
    lda8(0x50);
    cyc(40);
    wr8(b | 0x0E0E, 0x50);
    lda8(0xB8);
    cyc(40);
    wr8(b | 0x0E10, 0xB8);
    lda8(0x00);
    cyc(62);
    wr8(b | DMAP1, 0x00);
    lda8(0x2C);
    cyc(40);
    wr8(b | BBAD1, 0x2C);
    lda8(0x7F);
    cyc(40);
    wr8(b | A1B1, 0x7F);
    op_rep(P_M);
    op_lda16_imm(0x3480);
    cyc(10);
    wr16(b | A1T1L, 0x3480);
    op_sep(P_M);
    cyc(22);
    for (i = 0; i < 9; i++) {
        lda8((uint8_t)hdma_bytes[i]);
        cyc(48);
        wr8(0x7F3480 + i, (uint8_t)hdma_bytes[i]);
    }
    lda8(0x00);
    cyc(40);
    wr8(b | DMAP2, 0x00);
    lda8(0x21);
    cyc(40);
    wr8(b | BBAD2, 0x21);
    lda8(0x7F);
    cyc(40);
    wr8(b | A1B2, 0x7F);
    op_rep(P_M);
    op_lda16_imm(0x3497);
    cyc(10);
    wr16(b | A1T2L, 0x3497);
    op_lda16_imm(0x0042);
    cyc(10);
    wr16(b | DMAP3, 0x0042);
    op_lda16_imm(0x0022);
    cyc(10);
    wr16(b | BBAD3, 0x0022);
    op_lda16_imm(0x007F);
    cyc(10);
    wr16(b | A1B3, 0x007F);
    op_lda16_imm(0x3489);
    cyc(32);
    wr16(b | A1T3L, 0x3489);
    op_rep(P_M | P_X);
    op_lda16_imm(0x0120);
    cyc(18);
    wr16(0x7F3497, 0x0120);
    cpu.y = 0x0080;
    cpu.x = 0x0002;
    op_lda16_imm(0x0181);
    (cyc(80), abi_GradientFillCgadd());
    op_lda16_imm(0x0000);
    cyc(18);
    wr16(0x7F3497 + cpu.x, 0x0000);
    op_lda16_imm(0xEC57);
    cyc(18);
    wr16(0x7F348A, 0xEC57);
    op_sep(P_M);
    lda8(0x20);
    cyc(70);
    wr8(0x7F3489, 0x20);
    lda8(0xFF);
    cyc(48);
    wr8(0x7F348C, 0xFF);
    lda8(0x00);
    cyc(48);
    wr8(0x7F348F, 0x00);
    lda8(0x7F);
    cyc(40);
    wr8(b | DASB3, 0x7F);
    lda8(0x03);
    cyc(40);
    wr8(b | 0x0374, 0x03);
    op_rep(P_M);
    op_lda16_imm(0x0001);
    cyc(18);
    wr16(0x7001C4, 0x0001);
    (cyc(24), hud_end());
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
static void race_hud_setup_two(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint16_t v;

    op_rep(P_M | P_X);
    op_lda16_imm(0xA545);
    cyc(18);
    wr16(0x70004E, 0xA545);
    op_sep(P_M);
    lda8(0x01);
    cyc(62);
    wr8(b | 0x0374, 0x01);
    op_rep(P_M);
    op_lda16_imm(0x0002);
    cyc(18);
    wr16(0x7001C4, 0x0002);
    (cyc(62), abi_MenuCourseScreenSetup());
    (cyc(46), abi_PaletteCourseLoad());
    op_rep(P_M | P_X);
    b = (uint32_t)cpu.db << 16;
    cpu.x = 0x00BC;
    for (cpu.y = 0x0000; cpu.y < 0x0020; ) {
        cyc(164);
        v = (uint16_t)(rd16(b | (uint16_t)(0xE7EA + cpu.y)) - 0x0800);
        wr16(0x000375 + cpu.x, v);
        v = (uint16_t)(v + 0x6850);
        cyc(70);
        wr16(0x000475 + cpu.x, v);
        cyc(78);
        v = rd16(b | (uint16_t)(0xE7EC + cpu.y));
        wr16(0x000377 + cpu.x, v);
        cyc(32);
        wr16(0x000477 + cpu.x, v);
        cpu.x = (uint16_t)(cpu.x + 4);
        cpu.y = (uint16_t)(cpu.y + 4);
        cyc(110);
    }
    op_lda16_imm(0x0000);
    cyc(52);
    wr16(b | 0x0425, 0x0000);
    cyc(24);
    wr16(b | 0x0429, 0x0000);
    cyc(24);
    wr16(b | 0x042D, 0x0000);
    cyc(24);
    wr16(b | 0x0525, 0x0000);
    cyc(24);
    wr16(b | 0x0529, 0x0000);
    cyc(24);
    wr16(b | 0x052D, 0x0000);
    op_lda16_imm(0x26A0);
    cyc(10);
    wr16(b | 0x0427, 0x26A0);
    cyc(24);
    wr16(b | 0x042B, 0x26A0);
    cyc(24);
    wr16(b | 0x042F, 0x26A0);
    cyc(24);
    wr16(b | 0x0527, 0x26A0);
    cyc(24);
    wr16(b | 0x052B, 0x26A0);
    cyc(24);
    wr16(b | 0x052F, 0x26A0);
    cpu.x = 0x00DC;
    for (cpu.y = 0x0000; cpu.y < 0x0024; ) {
        cyc(162);
        wr16(b | (uint16_t)(0x0375 + cpu.x), (uint16_t)(rd16(b | (uint16_t)(0xE80F + cpu.y)) + 0x2C28));
        cyc(76);
        wr16(b | (uint16_t)(0x0377 + cpu.x), rd16(b | (uint16_t)(0xE811 + cpu.y)));
        cpu.x = (uint16_t)(cpu.x + 4);
        cpu.y = (uint16_t)(cpu.y + 4);
        cyc(110);
    }
    cpu.x = 0x01DC;
    for (cpu.y = 0x0000; cpu.y < 0x0024; ) {
        cyc(204);
        wr16(b | (uint16_t)(0x0375 + cpu.x), (uint16_t)(rd16(b | (uint16_t)(0xE80F + cpu.y)) + 0x9476));
        cyc(76);
        v = rd16(b | (uint16_t)(0xE811 + cpu.y));
        cpu.a = v;
        wr16(b | (uint16_t)(0x0377 + cpu.x), v);
        cpu.x = (uint16_t)(cpu.x + 4);
        cpu.y = (uint16_t)(cpu.y + 4);
        cyc(68);
    }
    cyc(80);
    op_sep(P_M);
    cpu.x = 0x000B;
    for (cpu.y = 0x0000; cpu.y < 0x0005; ) {
        cyc(142);
        uint8_t s = rd8(b | (uint16_t)(0xE80A + cpu.y));

        lda8(s);
        wr8(b | (uint16_t)(0x0575 + cpu.x), s);
        cyc(30);
        wr8(b | (uint16_t)(0x0585 + cpu.x), s);
        cpu.x++;
        cpu.y++;
    }
    cyc(178);
    cpu.p = (uint8_t)((cpu.p & ~(P_N | P_Z)) | P_Z | P_C);
    op_rep(P_M | P_X);
    cpu.y = 0xF845;
    cpu.x = 0xF804;
    cpu.p = nz16(cpu.p, 0xF804);
    abi_GsuCF42ToVram();
    (cyc(0), hud_end());
}

/* PLB, PLP (the data bank and P RaceHudLoad pushed), RTL: for an entry
   from original code, both pulled from the stack and the return taken
   through it. */
static void hud_leave_stack(void)
{
    uint16_t lo;
    uint8_t p;

    cyc(34);                            /* SEP and PLB, after hud_end's 8 */
    cpu.db = rd8((uint16_t)(cpu.s + 1));
    cpu.s = (uint16_t)(cpu.s + 1);
    cpu.p = nz8(cpu.p, cpu.db);
    cyc(20);
    p = rd8((uint16_t)(cpu.s + 1));
    cpu.p = p;
    if (p & P_X) {
        cpu.x &= 0x00FF;
        cpu.y &= 0x00FF;
    }
    cpu.s = (uint16_t)(cpu.s + 1);
    cyc(20);                            /* RTL */
    lo = (uint16_t)(rd16((uint16_t)(cpu.s + 1)) + 1);
    cpu.pc = (uint32_t)rd8((uint16_t)(cpu.s + 3)) << 16 | lo;
    cpu.s = (uint16_t)(cpu.s + 3);
    cpu.unwound = 1;
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
void RaceHudSetup(uint8_t p, uint8_t db)
{
    race_hud_setup();
    cpu.db = db;
    cpu.p = nz8(cpu.p, db);
    op_plp(p);
}

void abi_RaceHudSetup(void)
{
    race_hud_setup();
    hud_leave_stack();
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
void RaceHudSetupTwoPlayers(uint8_t p, uint8_t db)
{
    race_hud_setup_two();
    cpu.db = db;
    cpu.p = nz8(cpu.p, db);
    op_plp(p);
}

void abi_RaceHudSetupTwoPlayers(void)
{
    race_hud_setup_two();
    hud_leave_stack();
}
