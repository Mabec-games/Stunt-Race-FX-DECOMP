/*
 * records_screen.c: the course screens' records pages (state 3 of each
 * course select): the best times, names and cars kept in the save, the
 * Speed Trax pages of each course's times, and the menu under them (go on,
 * look at the record, back to the course select).
 *
 * The menu's cursor is $0D50 (0, 2 or 4, an offset into each menu's jump
 * table); $0D41 counts frames before the pad is read; the highlighted
 * choice has attribute byte $36, the others $38, and sprite 0 (the cursor)
 * moves down $0F lines a choice.
 */
#include "machine.h"
#include "cpu_ops.h"
#include "carried.h"

uint16_t OamClassName(uint8_t bank, uint16_t *x, uint16_t *y);

void abi_MetaspriteDraw(void);
void abi_SoundPort0QueuePush(void);
void abi_OamClassName(void);
void abi_SpriteTextAt895E(void);
void abi_SpriteTextAt8152(void);
void abi_SpriteTextAt8152B(void);
void abi_SpriteTextAt8952(void);
void abi_OamPaletteForD75(void);
void abi_OamPaletteFromTableE26A(void);
void abi_OamPaletteFromTableE26AByMode(void);
void abi_OamPaletteFromTableE30F(void);
void abi_OamPalettesE28D(void);
void abi_ColourMathA7Start(void);
void abi_SpeedRecordsLeaveCar(void);
void abi_OamBufferHideLargeLong(void);
void abi_OamHideEntries0To2(void);
void abi_OamHideEntries3And9(void);
void abi_OamSprite0AddY(void);
void abi_OamAttr36x4(void);
void abi_OamAttr36x6(void);
void abi_OamAttr36x7(void);
void abi_OamAttr38x4(void);
void abi_OamAttr38x6(void);
void abi_OamAttr38x7(void);
void abi_SpritePiecesAtLeft(void);
void abi_SpritePiecesAtRight(void);
void abi_SpritePiecesAt46Y71(void);
void abi_SpritePiecesAt46Y82(void);
void abi_SpritePiecesAt46Y93(void);
void abi_SpritePiecesAt62Y71(void);
void abi_SpritePiecesAt62Y83(void);
void abi_SpritePiecesAt62Y95(void);
void abi_SpritePiecesCarAt51(void);
void abi_RecordTagCarSet(void);
void abi_RecordTimeDigitsLarge(void);
void abi_RecordTimeDigitsSmall(void);
void abi_RecordScoreDigits(void);
void abi_RecordNameLetters(void);
void abi_RecordNameLettersLarge(void);
void abi_RecordNameLettersSmall(void);
void abi_RecordRowsFirstTimes(void);
void abi_RecordRowsGroupFirsts(void);
void abi_CourseScreenState8(void);

typedef void (*AbiFn)(void);

void AllRecordsScreen(Carried *cr);
void BattleRecordsScreen(Carried *cr);
void RecordsMenuAllBottom(Carried *cr);
void RecordsMenuAllMiddle(Carried *cr);
void RecordsMenuAllTop(Carried *cr);
void RecordsMenuBack(Carried *cr);
void RecordsMenuGo(Carried *cr);
void RecordsMenuSpeed(Carried *cr);
void RecordsMenuSpeedAlt(Carried *cr);
void RecordsMenuSpeedAltBottom(Carried *cr);
void RecordsMenuSpeedAltTop(Carried *cr);
void RecordsMenuSpeedBottom(Carried *cr);
void RecordsMenuSpeedTop(Carried *cr);
void RecordsMenuThree(Carried *cr);
void RecordsMenuThreeBottom(Carried *cr);
void RecordsMenuThreeMiddle(Carried *cr);
void RecordsMenuThreeTop(Carried *cr);
void RecordsMenuTwo(Carried *cr);
void RecordsMenuTwoBottom(Carried *cr);
void RecordsMenuTwoTop(Carried *cr);
void RecordsPageMarkFlip(Carried *cr);
void RecordsPageMarkFlipHide(Carried *cr);
void SpeedRecordsLeaveCar(Carried *cr);
void SpeedRecordsPage(Carried *cr);
void SpeedRecordsScreen(Carried *cr);
void StuntRecordsScreen(Carried *cr);
static void all_bottom_show(Carried *cr, int pushed);
static void all_menu(Carried *cr);
static void all_middle_show(Carried *cr, int pushed);
static void all_top_show(Carried *cr, int pushed);
static void best_time(Carried *cr, uint32_t save, uint16_t first, int d_time, int d_rest);
static void choice_show(Carried *cr, int pushed, uint8_t choice, CarriedFn f1, CarriedFn f5, CarriedFn f12, uint8_t lines);
static void choice_show_at(Carried *cr, int entry, uint8_t choice, CarriedFn f1, CarriedFn f5, CarriedFn f12, uint8_t lines);
static uint32_t db(Carried *cr);
static uint32_t dp(Carried *cr, uint8_t a);
static void lda8(Carried *cr, uint8_t v);
static void md_first(Carried *cr, uint16_t id, uint8_t bra);
static void md_next(Carried *cr, uint32_t group, uint16_t prev, uint16_t slot, uint16_t id);
static int menu_back(Carried *cr);
static uint16_t menu_handler(Carried *cr, uint16_t table);
static int menu_wait(Carried *cr);
static void origin(Carried *cr, uint8_t x, uint8_t y);
static void page_mark_flip(Carried *cr, uint8_t p);
static uint8_t page_mark_y(Carried *cr);
static int pressed(Carried *cr, uint16_t mask);
static void records_view(Carried *cr);
static void slot_to_y(Carried *cr, uint16_t slot, int d);
static void sound16(Carried *cr, uint16_t v);
static void sound8(Carried *cr, uint8_t v);
static void special_row(Carried *cr, uint16_t slot, uint16_t k, int stunt, CarriedFn place);
static void speed_alt_bottom_show(Carried *cr, int pushed);
static void speed_alt_leave(Carried *cr);
static void speed_alt_top_show(Carried *cr, int pushed);
static void speed_bottom_show(Carried *cr, int pushed);
static void speed_top_show(Carried *cr, int pushed);
static void w_OamClassName(Carried *cr);
static void w_SpriteTextAt8152(Carried *cr);
static void w_SpriteTextAt8152B(Carried *cr);
static void w_SpriteTextAt8952(Carried *cr);
static void w_SpriteTextAt895E(Carried *cr);
void ColourMathA7Start_cr(Carried *cr);
void CourseScreenState8(Carried *cr);
void MetaspriteDraw_cr(Carried *cr);
void OamAttr36x4_cr(Carried *cr);
void OamAttr36x6_cr(Carried *cr);
void OamAttr36x7(Carried *cr);
void OamAttr38x4_cr(Carried *cr);
void OamAttr38x6_cr(Carried *cr);
void OamAttr38x7_cr(Carried *cr);
void OamBufferHideLargeLong_cr(Carried *cr);
void OamHideEntries0To2_cr(Carried *cr);
void OamHideEntries3And9_cr(Carried *cr);
void OamPaletteForD75_cr(Carried *cr);
void OamPaletteFromTableE26AByMode_cr(Carried *cr);
void OamPaletteFromTableE26A_cr(Carried *cr);
void OamPaletteFromTableE30F_cr(Carried *cr);
void OamPalettesE28D_cr(Carried *cr);
void OamSprite0AddY_cr(Carried *cr);
void RecordNameLettersLarge_cr(Carried *cr);
void RecordNameLettersSmall_cr(Carried *cr);
void RecordNameLetters_cr(Carried *cr);
void RecordRowsFirstTimes_cr(Carried *cr);
void RecordRowsGroupFirsts(Carried *cr);
void RecordScoreDigits_cr(Carried *cr);
void RecordTagCarSet_cr(Carried *cr);
void RecordTimeDigitsLarge_cr(Carried *cr);
void RecordTimeDigitsSmall_cr(Carried *cr);
void SoundPort0QueuePush_cr(Carried *cr);
void SpritePiecesAt46Y71_cr(Carried *cr);
void SpritePiecesAt46Y82_cr(Carried *cr);
void SpritePiecesAt46Y93_cr(Carried *cr);
void SpritePiecesAt62Y71_cr(Carried *cr);
void SpritePiecesAt62Y83_cr(Carried *cr);
void SpritePiecesAt62Y95_cr(Carried *cr);
void SpritePiecesAtLeft_cr(Carried *cr);
void SpritePiecesAtRight_cr(Carried *cr);
void SpritePiecesCarAt51_cr(Carried *cr);

static uint32_t db(Carried *cr)
{
    return (uint32_t)cr->db << 16;
}

static uint32_t dp(Carried *cr, uint8_t a)
{
    return (uint16_t)(cr->d + a);
}

/* LDA #v with M 8-bit. */
static void lda8(Carried *cr, uint8_t v)
{
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    set_p(cr, nz8(cr->p, v));
}

/* LDA $0D1A (the pad's new presses, 16-bit), BIT #mask: whether any of
   them is pressed. */
static int pressed(Carried *cr, uint16_t mask)
{
    r_lda16(cr, cyc_then(24, db(cr) | 0x0D1A));
    set_p(cr, (uint8_t)((cr->p & ~P_Z) | ((cr->a & mask) ? (cyc(50), 0) : (cyc(16), P_Z))));
    return (cr->a & mask) != 0;
}

/* LDA #v (16-bit), JSL SoundPort0QueuePush. */
static void sound16(Carried *cr, uint16_t v)
{
    r_lda16_imm(cr, v);
    (cyc(38), SoundPort0QueuePush_cr(cr));
}

/* LDA #v (8-bit), JSL SoundPort0QueuePush. */
static void sound8(Carried *cr, uint8_t v)
{
    cyc(52);
    lda8(cr, v);
    SoundPort0QueuePush_cr(cr);
}

/* SEP #$30, then the metasprites' origin ($0D3B, $0D3C) and REP #$30. */
static void origin(Carried *cr, uint8_t x, uint8_t y)
{
    uint32_t b = db(cr);

    r_sep(cr, P_M | P_X);
    lda8(cr, x);
    cyc(34);
    wr8(b | 0x0D3B, x);
    lda8(cr, y);
    cyc(40);
    wr8(b | 0x0D3C, y);
    r_rep(cr, P_M | P_X);
}

/* LDY #0, LDX #id, STZ $0D71, MetaspriteDraw: a screen's first group; bra
   is the time of a BRA after it (none when the next group follows). */
static void md_first(Carried *cr, uint16_t id, uint8_t bra)
{
    cr->y = 0x0000;
    cr->x = id;
    set_p(cr, nz16(cr->p, id));
    wr16(db(cr) | 0x0D71, cyc_then(72, 0x0000));
    (cyc(46), MetaspriteDraw_cr(cr));
    if (bra)
        cyc(bra);
}

/* LDA.L group (CLC, ADC prev when there is one), TAY, STA slot, LDX #id,
   MetaspriteDraw: the next group from the sprite slot past the last. */
static void md_next(Carried *cr, uint32_t group, uint16_t prev, uint16_t slot, uint16_t id)
{
    uint32_t b = db(cr);

    cyc(48);
    r_lda16(cr, group);
    if (prev)
        { cyc(38); r_add16(cr, rd16(b | prev)); }
    r_tay16(cr);
    cyc(38);
    wr16(b | slot, cr->a);
    cr->x = id;
    set_p(cr, nz16(cr->p, id));
    (cyc(70), MetaspriteDraw_cr(cr));
}

/* LDA slot (INC or DEC by d), ASL, ASL, TAY: the slot's sprite buffer
   offset. */
static void slot_to_y(Carried *cr, uint16_t slot, int d)
{
    r_lda16(cr, cyc_then(24, db(cr) | slot));
    if (d) {
        cyc(14);
        cr->a = (uint16_t)(cr->a + d);
        set_p(cr, nz16(cr->p, cr->a));
    }
    r_asl16(cr);
    r_asl16(cr);
    r_tay16(cr);
}

/*
 * @routine 02:F90B RecordsMenuBack
 * WHAT IT DOES: Leaves a records page for the course select: sound $2F,
 * state 8 (CourseScreenState8), the large sprites hidden
 * (OamBufferHideLargeLong) and $0E92 = $F0.
 * HOW: 16-bit, then an 8-bit long store to $00:0E92.
 * Entry: any widths; data bank $7E or a mirror. Exit: M 16-bit, X 16-bit;
 * A high byte as OamBufferHideLargeLong left it, low byte $F0; N set, Z
 * clear.
 */
void RecordsMenuBack(Carried *cr)
{
    r_rep(cr, P_M | P_X);
    cyc(10);
    sound16(cr, 0x002F);
    (cyc(46), CourseScreenState8(cr));
    (cyc(62), OamBufferHideLargeLong_cr(cr));
    r_sep(cr, P_M);
    lda8(cr, 0xF0);
    cyc(70);
    wr8(0x000E92, 0xF0);
    r_rep(cr, P_M);
    cyc(42);
}

void abi_RecordsMenuBack(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RecordsMenuBack(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 02:F8F5 RecordsMenuGo
 * WHAT IT DOES: Takes a records page's first choice: sound $46, state $0A
 * (the window step), colour maths $A7 (ColourMathA7Start) and the large
 * sprites hidden (OamBufferHideLargeLong).
 * HOW: 8-bit.
 * Entry: any widths; data bank $7E or a mirror. Exit: M 8-bit, X 8-bit; as
 * OamBufferHideLargeLong leaves the rest.
 */
void RecordsMenuGo(Carried *cr)
{
    r_sep(cr, P_M | P_X);
    cyc(26);
    sound8(cr, 0x46);
    lda8(cr, 0x0A);
    wr8(db(cr) | 0x0D40, cyc_then(40, 0x0A));
    (cyc(62), ColourMathA7Start_cr(cr));
    (cyc(106), OamBufferHideLargeLong_cr(cr));
    cyc(42);
}

void abi_RecordsMenuGo(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RecordsMenuGo(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* SEP #$20, cursor $0D50 = `choice`, REP #$30, the attribute fills from
   entries 1, 5 and $0C (the last only for a three-choice menu), SEP #$20,
   OamSprite0AddY by `lines`: the cursor's choice highlighted. */
static void choice_show_at(Carried *cr, int entry, uint8_t choice, CarriedFn f1, CarriedFn f5, CarriedFn f12, uint8_t lines)
{
    r_sep(cr, P_M);
    lda8(cr, choice);
    wr8(db(cr) | 0x0D50, cyc_then(entry, choice));
    r_rep(cr, P_M | P_X);
    r_lda16_imm(cr, 0x0001);
    (cyc(32), f1(cr));
    r_lda16_imm(cr, 0x0005);
    (cyc(32), f5(cr));
    if (f12) {
        cyc(32);
        r_lda16_imm(cr, 0x000C);
        f12(cr);
    }
    r_sep(cr, P_M);
    lda8(cr, lines);
    (cyc(84), OamSprite0AddY_cr(cr));
    cyc(42);
}

static void choice_show(Carried *cr, int pushed, uint8_t choice, CarriedFn f1, CarriedFn f5, CarriedFn f12, uint8_t lines)
{
    choice_show_at(cr, pushed ? 84 : 92, choice, f1, f5, f12, lines);
}

/* LDA #$000C, STA $0D40 (16-bit: state $0C, $0D41 cleared), sound $24:
   the menu's middle choice. */
static void records_view(Carried *cr)
{
    r_lda16_imm(cr, 0x000C);
    wr16(db(cr) | 0x0D40, 0x000C);
    cyc(10);
    sound16(cr, 0x0024);
    cyc(42);                            /* RTS */
}

/* The three-choice menus' highlights. */
static void all_top_show(Carried *cr, int pushed)
{
    choice_show(cr, pushed, 0x00, OamAttr36x4_cr, OamAttr38x7_cr, OamAttr38x6_cr, 0x00);
}

static void all_middle_show(Carried *cr, int pushed)
{
    choice_show(cr, pushed, 0x04, OamAttr38x4_cr, OamAttr36x7, OamAttr38x6_cr, 0x0F);
}

static void all_bottom_show(Carried *cr, int pushed)
{
    choice_show(cr, pushed, 0x02, OamAttr38x4_cr, OamAttr38x7_cr, OamAttr36x6_cr, 0x1E);
}

/*
 * @routine 02:F51F RecordsMenuAllTop
 * WHAT IT DOES: The all-courses records menu with the cursor on its first
 * choice: B or Start takes it (RecordsMenuGo); Down moves to the middle
 * choice (sound $45); otherwise the first choice stays highlighted.
 * HOW: 16-bit tests of the pad's new presses ($9000, $0400).
 * Entry: M 16-bit; data bank $7E or a mirror. Exit: as RecordsMenuGo, or
 * as OamSprite0AddY leaves it (M 8-bit, X 16-bit).
 */
void RecordsMenuAllTop(Carried *cr)
{
    if (pressed(cr, 0x9000)) {
        cyc(14);
        RecordsMenuGo(cr);
        return;
    } else { cyc(30); }
    if (pressed(cr, 0x0400)) {
        sound16(cr, 0x0045);
        all_middle_show(cr, 1);
        return;
    }
    all_top_show(cr, 0);
}

void abi_RecordsMenuAllTop(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RecordsMenuAllTop(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 02:F55E RecordsMenuAllMiddle
 * WHAT IT DOES: The all-courses records menu with the cursor on its middle
 * choice: B or Start, unless every course is opened ($0D24 = 3), takes it
 * (state $0C, sound $24); Up and Down move the cursor (sound $45);
 * otherwise the middle choice stays highlighted.
 * HOW: An 8-bit compare, then 16-bit pad tests.
 * Entry: M 16-bit; data bank $7E or a mirror. Exit: as the push of sound
 * $24 leaves it, or as OamSprite0AddY.
 */
void RecordsMenuAllMiddle(Carried *cr)
{
    uint32_t b = db(cr);
    uint8_t v;

    r_sep(cr, P_M);
    cyc(46);
    r_lda8(cr, b | 0x0D24);
    v = (uint8_t)cr->a;
    set_p(cr, (uint8_t)((nz8(cr->p, (uint8_t)(v - 0x03)) & ~P_C) | (v >= 0x03 ? P_C : (cyc(32), 0))));
    if (v != 0x03) {
        r_rep(cr, P_M);
        if (pressed(cr, 0x9000)) {
            records_view(cr);
            return;
        } else { cyc(30); }
    }
    r_rep(cr, P_M);
    if (pressed(cr, 0x0800)) {
        sound16(cr, 0x0045);
        all_top_show(cr, 1);
        return;
    } else { cyc(30); }
    if (pressed(cr, 0x0400)) {
        sound16(cr, 0x0045);
        all_bottom_show(cr, 1);
        return;
    }
    all_middle_show(cr, 0);
}

void abi_RecordsMenuAllMiddle(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RecordsMenuAllMiddle(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 02:F5C6 RecordsMenuAllBottom
 * WHAT IT DOES: The all-courses records menu with the cursor on its last
 * choice: B or Start goes back to the course select (RecordsMenuBack); Up
 * moves to the middle choice (sound $45); otherwise the last choice stays
 * highlighted.
 * HOW: 16-bit pad tests.
 * Entry: M 16-bit; data bank $7E or a mirror. Exit: as RecordsMenuBack, or
 * as OamSprite0AddY.
 */
void RecordsMenuAllBottom(Carried *cr)
{
    if (pressed(cr, 0x9000)) {
        cyc(14);
        RecordsMenuBack(cr);
        return;
    } else { cyc(30); }
    if (pressed(cr, 0x0800)) {
        sound16(cr, 0x0045);
        all_middle_show(cr, 1);
        return;
    }
    all_bottom_show(cr, 0);
}

void abi_RecordsMenuAllBottom(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RecordsMenuAllBottom(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* SEP #$20; while the countdown $0D41 runs, one step of it and nothing
   else (returns 0). */
static int menu_wait(Carried *cr)
{
    uint32_t b = db(cr);
    uint8_t v;

    r_sep(cr, P_M);
    cyc(22);
    r_lda8(cr, b | 0x0D41);
    if (!(uint8_t)cr->a)
        { cyc(22); return 1; }
    cyc(40);
    v = (uint8_t)(rd8(b | 0x0D41) - 1);
    cyc(6);
    wr8(b | 0x0D41, v);
    set_p(cr, nz8(cr->p, v));
    cyc(42);
    return 0;
}

/* SEP #$10, REP #$20; Y goes back to the course select (returns 1). */
static int menu_back(Carried *cr)
{
    r_sep(cr, P_X);
    r_rep(cr, P_M);
    if (!pressed(cr, 0x4000))
        { cyc(30); return 0; } else { cyc(20); }
    RecordsMenuBack(cr);
    return 1;
}

/* LDX $0D50 (8-bit), the menu's JMP (table,X): the handler's address. */
static uint16_t menu_handler(Carried *cr, uint16_t table)
{
    uint8_t x = rd8(cyc_then(24, db(cr) | 0x0D50));

    cr->x = x;
    set_p(cr, nz8(cr->p, x));
    cyc(46);
    return rd16(0x020000 | (uint16_t)(table + x));
}

/* The all-courses records menu (inline in AllRecordsScreen): the
   countdown, Y back, then the cursor's handler from the table at
   $02:F519. */
static void all_menu(Carried *cr)
{
    uint16_t h;

    if (!(cyc(24), menu_wait(cr)) || menu_back(cr))
        { cyc(0); return; }
    h = menu_handler(cr, 0xF519);
    switch (h) {
    case 0xF51F: RecordsMenuAllTop(cr); break;
    case 0xF5C6: RecordsMenuAllBottom(cr); break;
    case 0xF55E: RecordsMenuAllMiddle(cr); break;
    default: carried_jump(cr, 0x020000 | h); break;
    }
    cyc(0);
}

/* The Speed Trax records menu's highlights. */
static void speed_top_show(Carried *cr, int pushed)
{
    choice_show(cr, pushed, 0x00, OamAttr36x4_cr, OamAttr38x6_cr, 0, 0x00);
}

static void speed_bottom_show(Carried *cr, int pushed)
{
    choice_show(cr, pushed, 0x02, OamAttr38x4_cr, OamAttr36x6_cr, 0, 0x0F);
}

/*
 * @routine 02:F629 RecordsMenuSpeedTop
 * WHAT IT DOES: The Speed Trax records menu with the cursor on its first
 * choice: B or Start takes it (RecordsMenuGo); Down moves to the second
 * (sound $45); otherwise the first stays highlighted.
 * HOW: 16-bit pad tests.
 * Entry: M 16-bit; data bank $7E or a mirror. Exit: as RecordsMenuGo, or
 * as OamSprite0AddY.
 */
void RecordsMenuSpeedTop(Carried *cr)
{
    if (pressed(cr, 0x9000)) {
        cyc(14);
        RecordsMenuGo(cr);
        return;
    } else { cyc(30); }
    if (pressed(cr, 0x0400)) {
        sound16(cr, 0x0045);
        choice_show_at(cr, 84, 0x02, OamAttr38x4_cr, OamAttr36x6_cr, 0, 0x0F);
        return;
    }
    speed_top_show(cr, 0);
}

void abi_RecordsMenuSpeedTop(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RecordsMenuSpeedTop(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 02:F662 RecordsMenuSpeedBottom
 * WHAT IT DOES: The Speed Trax records menu with the cursor on its second
 * choice: B or Start goes back (RecordsMenuBack); Up moves to the first
 * (sound $45); otherwise the second stays highlighted.
 * HOW: 16-bit pad tests.
 * Entry: M 16-bit; data bank $7E or a mirror. Exit: as RecordsMenuBack, or
 * as OamSprite0AddY.
 */
void RecordsMenuSpeedBottom(Carried *cr)
{
    if (pressed(cr, 0x9000)) {
        cyc(14);
        RecordsMenuBack(cr);
        return;
    } else { cyc(30); }
    if (pressed(cr, 0x0800)) {
        sound16(cr, 0x0045);
        speed_top_show(cr, 1);
        return;
    }
    speed_bottom_show(cr, 0);
}

void abi_RecordsMenuSpeedBottom(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RecordsMenuSpeedBottom(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 02:F605 RecordsMenuSpeed
 * WHAT IT DOES: The Speed Trax records menu: after the countdown $0D41, Y
 * goes back (RecordsMenuBack), otherwise the cursor's handler from the
 * table at $02:F625 (RecordsMenuSpeedTop, RecordsMenuSpeedBottom).
 * HOW: JMP (abs,X) with X 8-bit.
 * Entry: any widths; data bank $7E or a mirror. Exit: as the handler
 * leaves it.
 */
void RecordsMenuSpeed(Carried *cr)
{
    cyc(24);
    uint16_t h;

    if (!menu_wait(cr) || menu_back(cr))
        return;
    h = menu_handler(cr, 0xF625);
    switch (h) {
    case 0xF629: RecordsMenuSpeedTop(cr); break;
    case 0xF662: RecordsMenuSpeedBottom(cr); break;
    default: carried_jump(cr, 0x020000 | h); break;
    }
    cyc(0);
}

void abi_RecordsMenuSpeed(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RecordsMenuSpeed(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* The Speed Trax menu with $0D4C set: Y or the second choice's B or Start
   clear $0D4C, play sound $2F, go to state $0E with $0D48 counted up, hide
   the large sprites and set $0E92 to $F0. */
static void speed_alt_leave(Carried *cr)
{
    uint32_t b = db(cr);
    uint8_t v;

    r_sep(cr, P_M);
    cyc(36);
    wr8(b | 0x0D4C, 0x00);
    r_rep(cr, P_M | P_X);
    cyc(10);
    sound16(cr, 0x002F);
    r_sep(cr, P_M);
    lda8(cr, 0x0E);
    cyc(62);
    wr8(b | 0x0D40, 0x0E);
    cyc(24);
    v = (uint8_t)(rd8(b | 0x0D48) + 1);
    cyc(6);
    wr8(b | 0x0D48, v);
    set_p(cr, nz8(cr->p, v));
    (cyc(62), OamBufferHideLargeLong_cr(cr));
    lda8(cr, 0xF0);
    cyc(48);
    wr8(0x000E92, 0xF0);
    cyc(42);                            /* RTS */
}

static void speed_alt_top_show(Carried *cr, int pushed)
{
    choice_show(cr, pushed, 0x00, OamAttr36x4_cr, OamAttr38x4_cr, 0, 0x00);
}

static void speed_alt_bottom_show(Carried *cr, int pushed)
{
    choice_show(cr, pushed, 0x02, OamAttr38x4_cr, OamAttr36x4_cr, 0, 0x0F);
}

/*
 * @routine 02:F6B2 RecordsMenuSpeedAltTop
 * WHAT IT DOES: The Speed Trax records menu with $0D4C set, cursor on its
 * first choice: Y leaves ($0D4C cleared, sound $2F, state $0E, $0D48
 * counted up); B or Start takes the choice (RecordsMenuGo); Down moves to
 * the second (sound $45); otherwise the first stays highlighted.
 * HOW: 16-bit pad tests.
 * Entry: M 8-bit; data bank $7E or a mirror. Exit: as the path's last
 * call leaves it.
 */
void RecordsMenuSpeedAltTop(Carried *cr)
{
    r_rep(cr, P_M);
    if (pressed(cr, 0x4000)) {
        cyc(30);
        speed_alt_leave(cr);
        return;
    } else { cyc(30); }
    if (pressed(cr, 0x9000)) {
        cyc(14);
        RecordsMenuGo(cr);
        return;
    } else { cyc(30); }
    if (pressed(cr, 0x0400)) {
        sound16(cr, 0x0045);
        speed_alt_bottom_show(cr, 1);
        return;
    }
    speed_alt_top_show(cr, 0);
}

void abi_RecordsMenuSpeedAltTop(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RecordsMenuSpeedAltTop(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 02:F6F8 RecordsMenuSpeedAltBottom
 * WHAT IT DOES: As RecordsMenuSpeedAltTop with the cursor on the second
 * choice: Y, B or Start leave ($0D4C cleared, sound $2F, state $0E); Up
 * moves to the first (sound $45); otherwise the second stays highlighted.
 * HOW: 16-bit pad tests.
 * Entry: M 8-bit; data bank $7E or a mirror. Exit: as the path's last
 * call leaves it.
 */
void RecordsMenuSpeedAltBottom(Carried *cr)
{
    r_rep(cr, P_M);
    if (pressed(cr, 0x4000)) {
        cyc(30);                        /* BRL */
        speed_alt_leave(cr);
        return;
    }
    cyc(30);
    if (pressed(cr, 0x9000)) {
        speed_alt_leave(cr);
        return;
    } else { cyc(30); }
    if (pressed(cr, 0x0800)) {
        sound16(cr, 0x0045);
        speed_alt_top_show(cr, 1);
        return;
    }
    speed_alt_bottom_show(cr, 0);
}

void abi_RecordsMenuSpeedAltBottom(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RecordsMenuSpeedAltBottom(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 02:F69B RecordsMenuSpeedAlt
 * WHAT IT DOES: The Speed Trax records menu when $0D4C is set: after the
 * countdown $0D41, the cursor's handler from the table at $02:F6AE
 * (RecordsMenuSpeedAltTop, RecordsMenuSpeedAltBottom); Y is the
 * handlers' to read.
 * HOW: JMP (abs,X) with X 8-bit, M left 8-bit.
 * Entry: any widths; data bank $7E or a mirror. Exit: as the handler
 * leaves it.
 */
void RecordsMenuSpeedAlt(Carried *cr)
{
    cyc(24);
    uint16_t h;

    if (!menu_wait(cr))
        return;
    r_sep(cr, P_X);
    h = menu_handler(cr, 0xF6AE);
    switch (h) {
    case 0xF6B2: RecordsMenuSpeedAltTop(cr); cyc(0); break;
    case 0xF6F8: RecordsMenuSpeedAltBottom(cr); break;
    default: carried_jump(cr, 0x020000 | h); break;
    }
}

void abi_RecordsMenuSpeedAlt(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RecordsMenuSpeedAlt(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 02:F782 RecordsMenuTwoTop
 * WHAT IT DOES: The two-choice records menu (Stunt Trax special, Battle
 * Trax, all-courses special) with the cursor on its first choice: B or
 * Start takes it (RecordsMenuGo); Down moves to the second (sound $45);
 * otherwise the first stays highlighted.
 * HOW: 16-bit pad tests.
 * Entry: M 16-bit; data bank $7E or a mirror. Exit: as RecordsMenuGo, or
 * as OamSprite0AddY.
 */
void RecordsMenuTwoTop(Carried *cr)
{
    if (pressed(cr, 0x9000)) {
        cyc(20);
        RecordsMenuGo(cr);
        return;
    } else { cyc(30); }
    if (pressed(cr, 0x0400)) {
        sound16(cr, 0x0045);
        speed_bottom_show(cr, 1);
        return;
    }
    speed_top_show(cr, 0);
}

void abi_RecordsMenuTwoTop(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RecordsMenuTwoTop(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 02:F7BB RecordsMenuTwoBottom
 * WHAT IT DOES: The two-choice records menu with the cursor on its second
 * choice: B or Start goes back (RecordsMenuBack); Up moves to the first
 * (sound $45); otherwise the second stays highlighted.
 * HOW: 16-bit pad tests.
 * Entry: M 16-bit; data bank $7E or a mirror. Exit: as RecordsMenuBack, or
 * as OamSprite0AddY.
 */
void RecordsMenuTwoBottom(Carried *cr)
{
    if (pressed(cr, 0x9000)) {
        cyc(20);
        RecordsMenuBack(cr);
        return;
    } else { cyc(30); }
    if (pressed(cr, 0x0800)) {
        sound16(cr, 0x0045);
        speed_top_show(cr, 1);
        return;
    }
    speed_bottom_show(cr, 0);
}

void abi_RecordsMenuTwoBottom(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RecordsMenuTwoBottom(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 02:F75E RecordsMenuTwo
 * WHAT IT DOES: The two-choice records menu: after the countdown $0D41, Y
 * goes back (RecordsMenuBack), otherwise the cursor's handler from the
 * table at $02:F77E (RecordsMenuTwoTop, RecordsMenuTwoBottom).
 * HOW: As RecordsMenuSpeed.
 * Entry: any widths; data bank $7E or a mirror. Exit: as the handler
 * leaves it.
 */
void RecordsMenuTwo(Carried *cr)
{
    cyc(24);
    uint16_t h;

    if (!menu_wait(cr) || menu_back(cr))
        { cyc(0); return; }
    h = menu_handler(cr, 0xF77E);
    switch (h) {
    case 0xF782: RecordsMenuTwoTop(cr); break;
    case 0xF7BB: RecordsMenuTwoBottom(cr); break;
    default: carried_jump(cr, 0x020000 | h); break;
    }
    cyc(0);
}

void abi_RecordsMenuTwo(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RecordsMenuTwo(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 02:F81A RecordsMenuThreeTop
 * WHAT IT DOES: The Stunt Trax records menu with the cursor on its first
 * choice: B or Start takes it (RecordsMenuGo); Down moves to the middle
 * choice (sound $45); otherwise the first stays highlighted.
 * HOW: 16-bit pad tests.
 * Entry: M 16-bit; data bank $7E or a mirror. Exit: as RecordsMenuGo, or
 * as OamSprite0AddY.
 */
void RecordsMenuThreeTop(Carried *cr)
{
    if (pressed(cr, 0x9000)) {
        cyc(14);
        RecordsMenuGo(cr);
        return;
    } else { cyc(30); }
    if (pressed(cr, 0x0400)) {
        sound16(cr, 0x0045);
        (cyc(22), all_middle_show(cr, 1));
        return;
    }
    all_top_show(cr, 0);
}

void abi_RecordsMenuThreeTop(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RecordsMenuThreeTop(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 02:F85B RecordsMenuThreeMiddle
 * WHAT IT DOES: The Stunt Trax records menu with the cursor on its middle
 * choice: B or Start takes it (state $0C, sound $24); Up and Down move the
 * cursor (sound $45); otherwise the middle stays highlighted.
 * HOW: 16-bit pad tests.
 * Entry: M 16-bit; data bank $7E or a mirror. Exit: as the push of sound
 * $24 leaves it, or as OamSprite0AddY.
 */
void RecordsMenuThreeMiddle(Carried *cr)
{
    if (pressed(cr, 0x9000)) {
        records_view(cr);
        return;
    } else { cyc(30); }
    if (pressed(cr, 0x0800)) {
        sound16(cr, 0x0045);
        all_top_show(cr, 1);
        return;
    } else { cyc(30); }
    if (pressed(cr, 0x0400)) {
        sound16(cr, 0x0045);
        all_bottom_show(cr, 1);
        return;
    }
    all_middle_show(cr, 0);
}

void abi_RecordsMenuThreeMiddle(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RecordsMenuThreeMiddle(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 02:F8B6 RecordsMenuThreeBottom
 * WHAT IT DOES: The Stunt Trax records menu with the cursor on its last
 * choice: B or Start goes back (RecordsMenuBack); Up moves to the middle
 * (sound $45); otherwise the last stays highlighted.
 * HOW: 16-bit pad tests.
 * Entry: M 16-bit; data bank $7E or a mirror. Exit: as RecordsMenuBack, or
 * as OamSprite0AddY.
 */
void RecordsMenuThreeBottom(Carried *cr)
{
    if (pressed(cr, 0x9000)) {
        cyc(14);
        RecordsMenuBack(cr);
        return;
    } else { cyc(30); }
    if (pressed(cr, 0x0800)) {
        sound16(cr, 0x0045);
        all_middle_show(cr, 1);
        return;
    }
    all_bottom_show(cr, 0);
}

void abi_RecordsMenuThreeBottom(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RecordsMenuThreeBottom(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 02:F7F4 RecordsMenuThree
 * WHAT IT DOES: The Stunt Trax records menu: after the countdown $0D41, Y
 * goes back (RecordsMenuBack), otherwise the cursor's handler from the
 * table at $02:F814 (RecordsMenuThreeTop, RecordsMenuThreeBottom,
 * RecordsMenuThreeMiddle).
 * HOW: As RecordsMenuSpeed.
 * Entry: any widths; data bank $7E or a mirror. Exit: as the handler
 * leaves it.
 */
void RecordsMenuThree(Carried *cr)
{
    cyc(24);
    uint16_t h;

    if (!menu_wait(cr) || menu_back(cr))
        { cyc(0); return; }
    h = menu_handler(cr, 0xF814);
    switch (h) {
    case 0xF81A: RecordsMenuThreeTop(cr); break;
    case 0xF8B6: RecordsMenuThreeBottom(cr); break;
    case 0xF85B: RecordsMenuThreeMiddle(cr); break;
    default: carried_jump(cr, 0x020000 | h); break;
    }
    cyc(0);
}

void abi_RecordsMenuThree(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RecordsMenuThree(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* PHP, REP #$30, Y = ($0D73 & $FF) * 4, SEP #$20: the page mark's sprite
   buffer offset. Returns the P pushed. */
static uint8_t page_mark_y(Carried *cr)
{
    uint8_t p = cr->p;

    r_rep(cr, P_M | P_X);
    r_lda16(cr, cyc_then(46, db(cr) | 0x0D73));
    r_and16(cr, 0x00FF);
    r_asl16(cr);
    r_asl16(cr);
    r_tay16(cr);
    r_sep(cr, P_M);
    return p;
}

/* LDA #$F0 to the tile bytes of the group's sprites 0, 2 and 4, their
   attributes ORA #$40 (flipped across), PLP. */
static void page_mark_flip(Carried *cr, uint8_t p)
{
    uint32_t b = db(cr);
    unsigned i;
    uint8_t v;

    lda8(cr, 0xF0);
    cyc(38);
    for (i = 0; i < 3; i++)
        { cyc(30); wr8(b | (uint16_t)(0x0377 + i * 8 + cr->y), 0xF0); }
    for (i = 0; i < 3; i++) {
        cyc(30);
        v = (uint8_t)(rd8(b | (uint16_t)(0x0378 + i * 8 + cr->y)) | 0x40);
        lda8(cr, v);
        cyc(46);
        wr8(b | (uint16_t)(0x0378 + i * 8 + cr->y), v);
    }
    cyc(28);
    r_plp(cr, p);
}

/*
 * @routine 02:F0FA RecordsPageMarkFlipHide
 * WHAT IT DOES: RecordsPageMarkFlip with the group's sprites 1, 3 and 5
 * hidden (Y = $F0) first: the Speed Trax records' last page.
 * HOW: Shares RecordsPageMarkFlip's tail.
 * Entry: any widths; data bank $7E or a mirror. Exit: as
 * RecordsPageMarkFlip.
 */
void RecordsPageMarkFlipHide(Carried *cr)
{
    uint32_t b = db(cr);
    uint8_t p = page_mark_y(cr);

    lda8(cr, 0xF0);
    cyc(132);
    wr8(b | (uint16_t)(0x037A + cr->y), 0xF0);
    cyc(30);
    wr8(b | (uint16_t)(0x0382 + cr->y), 0xF0);
    cyc(30);
    wr8(b | (uint16_t)(0x038A + cr->y), 0xF0);
    page_mark_flip(cr, p);
}

void abi_RecordsPageMarkFlipHide(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RecordsPageMarkFlipHide(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 02:F115 RecordsPageMarkFlip
 * WHAT IT DOES: Turns the page mark (the metasprite group from slot $0D73)
 * for the Speed Trax records' middle pages: its sprites 0, 2 and 4 get
 * tile $F0 and are flipped across.
 * HOW: Byte stores through the data bank, the attributes read back and
 * ORed with $40; P kept (PHP, PLP).
 * Entry: any widths; data bank $7E or a mirror. Exit: P as on entry; A
 * low byte the last attribute; Y = the group's sprite buffer offset.
 */
void RecordsPageMarkFlip(Carried *cr)
{
    page_mark_flip(cr, cyc_then(64, page_mark_y(cr)));
}

void abi_RecordsPageMarkFlip(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RecordsPageMarkFlip(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 02:EEF2 SpeedRecordsPage
 * WHAT IT DOES: The Speed Trax records pages: Right and Left step the page
 * $10D8 (0-4, sound $45 each step); page 0 draws the best three times ($F7
 * = $24, RecordRowsFirstTimes), pages 1-4 the first times of each group of
 * the course's laps ($F7 = 0, 3, 6, 9, RecordRowsGroupFirsts) with the
 * page mark turned (RecordsPageMarkFlip, on page 4
 * RecordsPageMarkFlipHide); then the page's title text (SpriteTextAt895E)
 * with the course byte $0E3D set for it ($1C on page 0, else $0D4A * 4 +
 * page - 1) and put back.
 * HOW: JMP (abs,X) by page; 8-bit, the pad's word 16-bit.
 * Entry: any widths, X 16-bit; data bank $7E or a mirror. Exit: M 8-bit, X
 * 8-bit; A low byte = the course byte, N and Z from it; the rest as
 * SpriteTextAt895E leaves it.
 */
void SpeedRecordsPage(Carried *cr)
{
    uint32_t b = db(cr);
    uint8_t v, old, p;
    uint16_t h;
    uint32_t join = 68;

    r_rep(cr, P_M);
    if (pressed(cr, 0x0100)) {
        r_sep(cr, P_M);
        cyc(42);
        r_lda8(cr, b | 0x10D8);
        v = (uint8_t)cr->a;
        set_p(cr, (uint8_t)((nz8(cr->p, (uint8_t)(v - 0x04)) & ~P_C) | (v >= 0x04 ? P_C : (cyc(58), 0))));
        if (v < 0x04) {
            sound8(cr, 0x45);
            cyc(24);
            v = (uint8_t)(rd8(b | 0x10D8) + 1);
            cyc(6);
            wr8(b | 0x10D8, v);
            set_p(cr, nz8(cr->p, v));
        }
    } else { cyc(24); if (pressed(cr, 0x0200)) {
        r_sep(cr, P_M);
        cyc(36);
        r_lda8(cr, b | 0x10D8);
        if ((uint8_t)cr->a) {
            cyc(42);
            sound8(cr, 0x45);
            cyc(24);
            v = (uint8_t)(rd8(b | 0x10D8) - 1);
            cyc(6);
            wr8(b | 0x10D8, v);
            set_p(cr, nz8(cr->p, v));
        }
    cyc(22);
        } else { cyc(30); } }
    r_sep(cr, P_M | P_X);
    cyc(24);
    r_lda8(cr, b | 0x10D8);
    v = (uint8_t)cr->a;
    set_p(cr, (uint8_t)((nz8(cr->p, (uint8_t)(v - 0x05)) & ~P_C) | (v >= 0x05 ? P_C : (cyc(38), 0))));
    if (v >= 0x05) {
        v = 0x00;
        lda8(cr, 0x00);
        cyc(72);
        wr8(b | 0x10D8, 0x00);
    }
    v = (uint8_t)(v << 1);
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    set_p(cr, nz8(cr->p, v));
    cr->x = v;
    cyc(106);
    h = rd16(0x020000 | (uint16_t)(0xEF3B + v));
    switch (h) {
    case 0xEF45:
        lda8(cr, 0x24);
        wr8(dp(cr, 0xF7), 0x24);
        (cyc(46), RecordRowsFirstTimes_cr(cr));
        break;
    case 0xEF4E:
    case 0xEF5A:
    case 0xEF66:
        cyc(14);
        RecordsPageMarkFlip(cr);
        v = h == 0xEF4E ? 0x00 : h == 0xEF5A ? 0x03 : 0x06;
        lda8(cr, v);
        wr8(dp(cr, 0xF7), cyc_then(32, v));
        (cyc(46), RecordRowsGroupFirsts(cr));
        break;
    case 0xEF72:
        cyc(14);
        RecordsPageMarkFlipHide(cr);
        lda8(cr, 0x09);
        wr8(dp(cr, 0xF7), cyc_then(32, 0x09));
        (cyc(46), RecordRowsGroupFirsts(cr));
        join = 46;                      /* falls into .title: no BRA */
        break;
    default:
        carried_jump(cr, 0x020000 | h);
        return;
    }
    r_sep(cr, P_M);
    cyc(join);
    r_lda8(cr, b | 0x0E3D);
    old = (uint8_t)cr->a;
    cyc(46);
    r_lda8(cr, b | 0x10D8);
    if ((uint8_t)cr->a) {
        v = (uint8_t)(cr->a - 1);
        wr8(dp(cr, 0xF1), cyc_then(46, v));
        cyc(24);
        r_lda8(cr, b | 0x0D4A);
        p = (uint8_t)(cr->p & ~P_C);
        v = alu_adc8((uint8_t)(cr->a << 2), rd8(cyc_then(58, dp(cr, 0xF1))), &p);
        set_p(cr, p);
        cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    } else {
        cyc(16);
        lda8(cr, 0x1C);
    }
    cyc(46);
    wr8(b | 0x0E3D, (uint8_t)cr->a);
    (cyc(46), w_SpriteTextAt895E(cr));
    r_sep(cr, P_M);
    lda8(cr, old);
    cyc(74);
    wr8(b | 0x0E3D, old);
    cyc(42);
}

void abi_SpeedRecordsPage(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    SpeedRecordsPage(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 02:EDD2 SpeedRecordsScreen
 * WHAT IT DOES: The Speed Trax course select's state 3, the records page.
 * With $0D4C set: three metasprite groups ($1A, $19, 0), the class name
 * (OamClassName), the title (SpriteTextAt895E), palettes
 * (OamPaletteForD75) and RecordsMenuSpeedAlt. Otherwise: six groups (the
 * first $05, or $1B once $0D48 is set; then 1-4 and 0), the class name,
 * the pages of times (Right / Left), palettes (OamPaletteFromTableE26A);
 * while $0D48 is clear the menu (RecordsMenuSpeed), else $0E92 = $F0 and
 * B or Start leaves: sound $46, state $0A, and with $0D4C set the car
 * $0D89 = 4 and the selection handler $00:18DB = $8669, colour maths $A7
 * (ColourMathA7Start).
 * HOW: The groups' slots chain from the sizes at $04:EDCB (MetaspriteDraw
 * from slot 0, origin $89, $89); 16-bit, 8-bit stores. The $0D4C test
 * after B or Start always finds it clear (that path runs only with $0D4C
 * clear), so its car set-up (02:EEDD) runs only as SpeedRecordsLeaveCar.
 * Entry: any widths; data bank $7E or a mirror. Exit: as the last call
 * leaves it.
 */
void SpeedRecordsScreen(Carried *cr)
{
    uint32_t b = db(cr);

    r_sep(cr, P_M | P_X);
    cyc(24);
    r_lda8(cr, b | 0x0D4C);
    if ((uint8_t)cr->a) {
        origin(cr, 0x89, 0x89);
        md_first(cr, 0x001A, 0);
        md_next(cr, 0x04EE35, 0, 0x0D73, 0x0019);
        md_next(cr, 0x04EE31, 0x0D73, 0x0D75, 0x0000);
        cyc(24);
        r_lda16(cr, b | 0x0D75);
        cyc(24);
        wr16(b | 0x0D7B, cr->a);
        (cyc(46), w_OamClassName(cr));
        (cyc(46), w_SpriteTextAt895E(cr));
        (cyc(62), OamPaletteForD75_cr(cr));
        (cyc(24), RecordsMenuSpeedAlt(cr));
        return;
    } else { cyc(6); }
    origin(cr, 0x89, 0x89);
    cyc(24);
    r_lda16(cr, b | 0x0D48);
    r_and16(cr, 0x00FF);
    if (cr->a)
        { cyc(44); md_first(cr, 0x001B, 0); }
    else { cyc(38); md_first(cr, 0x0005, 22 /* BRA */); }
    md_next(cr, 0x04EDE1, 0, 0x0D73, 0x0001);
    md_next(cr, 0x04EDD1, 0x0D73, 0x0D75, 0x0002);
    md_next(cr, 0x04EDD5, 0x0D75, 0x0D77, 0x0003);
    md_next(cr, 0x04EDD9, 0x0D77, 0x0D79, 0x0004);
    md_next(cr, 0x04EDDD, 0x0D79, 0x0D7B, 0x0000);
    (cyc(46), w_OamClassName(cr));
    (cyc(46), SpeedRecordsPage(cr));
    (cyc(62), OamPaletteFromTableE26A_cr(cr));
    r_sep(cr, P_M | P_X);
    cyc(24);
    r_lda8(cr, b | 0x0D48);
    if (!(uint8_t)cr->a) {
        cyc(40);
        RecordsMenuSpeed(cr);
        return;
    }
    lda8(cr, 0xF0);
    cyc(62);
    wr8(b | 0x0E92, 0xF0);
    r_rep(cr, P_M);
    if (!pressed(cr, 0x9000))
        { cyc(72); return; }
    r_sep(cr, P_M | P_X);
    cyc(16);
    sound8(cr, 0x46);
    lda8(cr, 0x0A);
    cyc(40);
    wr8(b | 0x0D40, 0x0A);
    cyc(24);
    r_lda8(cr, b | 0x0D4C);
    if ((uint8_t)cr->a) {
        SpeedRecordsLeaveCar(cr);
        return;
    } else { cyc(84); }
    ColourMathA7Start_cr(cr);
    cyc(86);
}

void abi_SpeedRecordsScreen(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    SpeedRecordsScreen(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 02:EEDD SpeedRecordsLeaveCar
 * WHAT IT DOES: SpeedRecordsScreen's leaving with $0D4C set: the car $0D89
 * = 4, the selection handler $00:18DB = $8669, colour maths $A7
 * (ColourMathA7Start).
 * HOW: 8-bit and 16-bit stores. SpeedRecordsScreen reaches it only with
 * $0D4C clear (nothing on that path writes $0D4C), so no code runs these
 * bytes; a direct call is their only way in.
 * Entry: M 8-bit; data bank $7E or a mirror. Exit: as ColourMathA7Start
 * leaves it.
 */
void SpeedRecordsLeaveCar(Carried *cr)
{
    uint32_t b = db(cr);

    lda8(cr, 0x04);
    cyc(40);
    wr8(b | 0x0D89, 0x04);
    r_rep(cr, P_M);
    r_lda16_imm(cr, 0x8669);
    cyc(18);
    wr16(0x0018DB, 0x8669);
    r_sep(cr, P_M);
    (cyc(84), ColourMathA7Start_cr(cr));
    cyc(86);
}

void abi_SpeedRecordsLeaveCar(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    SpeedRecordsLeaveCar(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* The course's best time, name and car from the save's records at `save`
   (five bytes a course, course - `first`): with no time ($FFFF) the
   group's first three sprites hidden at the time's place (Y = $0F); the
   sprite offsets from slot $0D75 + d_time (time) and + d_rest (name and
   car, or the hidden sprites). */
static void best_time(Carried *cr, uint32_t save, uint16_t first, int d_time, int d_rest)
{
    uint32_t b = db(cr);
    uint16_t x;
    uint8_t p;

    slot_to_y(cr, 0x0D75, d_time);
    cyc(66);
    r_lda16(cr, b | 0x0E3D);
    r_sub16(cr, first);
    wr16(dp(cr, 0xF1), cyc_then(48, cr->a));
    r_asl16(cr);
    r_asl16(cr);
    p = (uint8_t)(cr->p & ~P_C);
    cr->a = alu_adc16(cr->a, rd16(cyc_then(58, dp(cr, 0xF1))), &p);
    set_p(cr, p);
    r_and16(cr, 0x00FF);
    r_tax16(cr);
    x = cr->x;
    cyc(38);
    r_lda16(cr, save + x);
    r_cmp16(cr, 0xFFFF);
    if (cr->a != 0xFFFF) {
        cyc(102);
        r_lda16(cr, save + x);
        (cyc(46), RecordTimeDigitsLarge_cr(cr));
        cr->x = x;
        set_p(cr, nz16(cr->p, x));
        cyc(68);
        r_lda16(cr, save + 2 + x);
        (cyc(46), RecordNameLettersLarge_cr(cr));
        slot_to_y(cr, 0x0D75, d_rest);
        cyc(74);
        r_lda16(cr, save + 4 + x);
        (cyc(46), SpritePiecesCarAt51_cr(cr));
        cyc(68);                        /* BRA, JSR */
        return;
    } else { cyc(46); }
    slot_to_y(cr, 0x0D75, d_rest);
    r_sep(cr, P_M);
    lda8(cr, 0x0F);
    cyc(110);
    wr8(b | (uint16_t)(0x0376 + cr->y), 0x0F);
    cyc(30);
    wr8(b | (uint16_t)(0x037A + cr->y), 0x0F);
    cyc(30);
    wr8(b | (uint16_t)(0x037E + cr->y), 0x0F);
    cyc(46);
}

/* One of the three special records rows (k = 0, 2, 4: $0F2B + k) from
   slot `slot`: Stunt Trax keeps scores (0 = none; RecordScoreDigits,
   RecordNameLettersSmall), the others times ($FFFF = none;
   RecordTimeDigitsSmall, RecordNameLetters); then the car from the tag
   ($0F13 + k, RecordTagCarSet) placed by `place`. With none, the row's
   sprites are hidden (OamHideEntries0To2, for scores OamHideEntries3And9
   too). */
static void special_row(Carried *cr, uint16_t slot, uint16_t k, int stunt, CarriedFn place)
{
    uint16_t y;

    slot_to_y(cr, slot, 0);
    cyc(74);
    r_lda16(cr, 0x000F2B + k);
    if (!stunt)
        { cyc(24); r_cmp16(cr, 0xFFFF); }
    cyc(68);
    if (stunt ? cr->a != 0 : cr->a != 0xFFFF) {
        y = cr->y;
        if (stunt)
            { cyc(24); RecordScoreDigits_cr(cr); }
        else
            { cyc(24); RecordTimeDigitsSmall_cr(cr); }
        cyc(32);   /* the digits routines state their RTS */
        r_lda16(cr, 0x000F43 + k);
        if (stunt)
            { cyc(46); RecordNameLettersSmall_cr(cr); }
        else
            RecordNameLetters_cr(cr);
        cyc(32);
        r_lda16(cr, 0x000F13 + k);
        cr->y = y;
        set_p(cr, nz16(cr->p, y));
        (cyc(128), RecordTagCarSet_cr(cr));
        place(cr);
        cyc(22);                        /* BRA over the hiding calls */
        return;
    }
    OamHideEntries0To2_cr(cr);
    if (stunt)
        { cyc(46); OamHideEntries3And9_cr(cr); }
    cyc(0);
}

/*
 * @routine 02:F148 StuntRecordsScreen
 * WHAT IT DOES: The Stunt Trax course select's state 3, the records page.
 * With $0D4E set: Right and Left step the course $0E3D through $17-$1A
 * (sound $45), three metasprite groups ($15, $13, $14), the course's best
 * time, name and car from the save ($70:FC4A), the title
 * (SpriteTextAt8152) and the two-choice menu (RecordsMenuTwo). Otherwise:
 * five groups ($12, 7, 8, 9, 6), the title, the three special scores
 * ($0F2B-$0F2F) with their names and cars, palettes
 * (OamPaletteFromTableE30F) and the three-choice menu (RecordsMenuThree).
 * HOW: As SpeedRecordsScreen (origin $81, $89).
 * Entry: any widths; data bank $7E or a mirror. Exit: as the menu leaves
 * it.
 */
void StuntRecordsScreen(Carried *cr)
{
    uint32_t b = db(cr);

    r_sep(cr, P_M | P_X);
    cyc(24);
    r_lda8(cr, b | 0x0D4E);
    if ((uint8_t)cr->a) {
        cyc(28);
        origin(cr, 0x81, 0x89);
        if (pressed(cr, 0x0100)) {
            sound16(cr, 0x0045);
            cyc(24);
            r_lda16(cr, b | 0x0E3D);
            cr->a = (uint16_t)(cr->a + 1);
            r_cmp16(cr, 0x001B);
            if (cr->a >= 0x001B)
                { r_lda16_imm(cr, 0x0017); cyc(4); }
            cyc(82);
            wr16(b | 0x0E3D, cr->a);
        } else { cyc(30); if (pressed(cr, 0x0200)) {
            sound16(cr, 0x0045);
            cyc(24);
            r_lda16(cr, b | 0x0E3D);
            cr->a = (uint16_t)(cr->a - 1);
            r_cmp16(cr, 0x0017);
            if (cr->a < 0x0017)
                { r_lda16_imm(cr, 0x001A); cyc(4); }
            cyc(60);
            wr16(b | 0x0E3D, cr->a);
        } else { cyc(30); } }
        md_first(cr, 0x0015, 0);
        md_next(cr, 0x04EE21, 0, 0x0D73, 0x0013);
        r_rep(cr, P_M);
        md_next(cr, 0x04EE19, 0x0D73, 0x0D75, 0x0014);
        best_time(cr, 0x70FC4A, 0x0017, 0, 1);
        w_SpriteTextAt8152(cr);
        (cyc(24), RecordsMenuTwo(cr));
        return;
    } else { cyc(30); }
    origin(cr, 0x81, 0x89);
    md_first(cr, 0x0012, 0);
    md_next(cr, 0x04EDF5, 0, 0x0D73, 0x0007);
    md_next(cr, 0x04EDE9, 0x0D73, 0x0D75, 0x0008);
    md_next(cr, 0x04EDED, 0x0D75, 0x0D77, 0x0009);
    md_next(cr, 0x04EDF1, 0x0D77, 0x0D79, 0x0006);
    (cyc(46), w_SpriteTextAt8152(cr));
    r_rep(cr, P_M | P_X);
    special_row(cr, 0x0D77, 4, 1, SpritePiecesAt46Y93_cr);
    special_row(cr, 0x0D75, 2, 1, SpritePiecesAt46Y82_cr);
    special_row(cr, 0x0D73, 0, 1, SpritePiecesAt46Y71_cr);
    (cyc(62), OamPaletteFromTableE30F_cr(cr));
    (cyc(24), RecordsMenuThree(cr));
}

void abi_StuntRecordsScreen(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    StuntRecordsScreen(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 02:F30A BattleRecordsScreen
 * WHAT IT DOES: The Battle Trax course select's state 3: three metasprite
 * groups ($0D, $0C, $0B), the two players' cars (car * 3 as piece sets,
 * SpritePiecesAtLeft from the second group, SpritePiecesAtRight three
 * sprites on), the title (SpriteTextAt8152B), palettes (OamPalettesE28D)
 * and the two-choice menu (RecordsMenuTwo).
 * HOW: As SpeedRecordsScreen (origin $81, $89).
 * Entry: any widths; data bank $7E or a mirror. Exit: as the menu leaves
 * it.
 */
void BattleRecordsScreen(Carried *cr)
{
    uint32_t b = (cyc(6), db(cr));
    uint8_t p;

    origin(cr, 0x81, 0x89);
    md_first(cr, 0x000D, 0);
    md_next(cr, 0x04EE01, 0, 0x0D73, 0x000C);
    md_next(cr, 0x04EDFD, 0x0D73, 0x0D75, 0x000B);
    slot_to_y(cr, 0x0D73, 0);
    cyc(66);
    r_lda16(cr, b | 0x0D89);
    r_asl16(cr);
    p = (uint8_t)(cr->p & ~P_C);
    cyc(52);
    cr->a = alu_adc16(cr->a, rd16(b | 0x0D89), &p);
    set_p(cr, p);
    r_and16(cr, 0x00FF);
    (cyc(68), SpritePiecesAtLeft_cr(cr));
    cyc(24);
    r_lda16(cr, b | 0x0D73);
    r_add16(cr, 0x0003);
    r_asl16(cr);
    r_asl16(cr);
    r_tay16(cr);
    cyc(104);
    r_lda16(cr, b | 0x0D8A);
    r_asl16(cr);
    p = (uint8_t)(cr->p & ~P_C);
    cyc(52);
    cr->a = alu_adc16(cr->a, rd16(b | 0x0D8A), &p);
    set_p(cr, p);
    r_and16(cr, 0x00FF);
    (cyc(68), SpritePiecesAtRight_cr(cr));
    (cyc(46), w_SpriteTextAt8152B(cr));
    (cyc(62), OamPalettesE28D_cr(cr));
    (cyc(24), RecordsMenuTwo(cr));
}

void abi_BattleRecordsScreen(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    BattleRecordsScreen(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 02:F37A AllRecordsScreen
 * WHAT IT DOES: The all-courses select's state 3. With $0D4C set: three
 * metasprite groups ($18, $16, $17), the course's best time, name and car
 * from the save ($70:FC5E, course - $14), the title (SpriteTextAt8952) and
 * the two-choice menu (RecordsMenuTwo). Otherwise: five groups ($12, $0F,
 * $10, $11, $0E), the title, palettes (OamPaletteFromTableE26AByMode), the
 * three special times ($0F2B-$0F2F) with names and cars, then its own
 * three-choice menu (table $02:F519: RecordsMenuAllTop,
 * RecordsMenuAllBottom, RecordsMenuAllMiddle) after the countdown $0D41, Y
 * going back (RecordsMenuBack).
 * HOW: As SpeedRecordsScreen (origin $89, $89).
 * Entry: any widths; data bank $7E or a mirror. Exit: as the last call
 * leaves it.
 */
void AllRecordsScreen(Carried *cr)
{
    uint32_t b = db(cr);

    r_sep(cr, P_M | P_X);
    cyc(24);
    r_lda8(cr, b | 0x0D4C);
    if ((uint8_t)cr->a) {
        cyc(6);
        origin(cr, 0x89, 0x89);
        md_first(cr, 0x0018, 0);
        md_next(cr, 0x04EE2D, 0, 0x0D73, 0x0016);
        md_next(cr, 0x04EE25, 0x0D73, 0x0D75, 0x0017);
        best_time(cr, 0x70FC5E, 0x0014, -1, 0);
        w_SpriteTextAt8952(cr);
        (cyc(24), RecordsMenuTwo(cr));
        return;
    } else { cyc(30); }
    origin(cr, 0x89, 0x89);
    md_first(cr, 0x0012, 0);
    md_next(cr, 0x04EE05, 0, 0x0D73, 0x000F);
    md_next(cr, 0x04EE09, 0x0D73, 0x0D75, 0x0010);
    md_next(cr, 0x04EE0D, 0x0D75, 0x0D77, 0x0011);
    md_next(cr, 0x04EE11, 0x0D77, 0x0D79, 0x000E);
    (cyc(46), w_SpriteTextAt8952(cr));
    (cyc(62), OamPaletteFromTableE26AByMode_cr(cr));
    r_rep(cr, P_M | P_X);
    cyc(0);
    special_row(cr, 0x0D77, 4, 0, SpritePiecesAt62Y95_cr);
    special_row(cr, 0x0D75, 2, 0, SpritePiecesAt62Y83_cr);
    special_row(cr, 0x0D73, 0, 0, SpritePiecesAt62Y71_cr);
    all_menu(cr);
}

void abi_AllRecordsScreen(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    AllRecordsScreen(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* SpriteTextAt895E, still in registers: its call with the registers loaded and taken
   back. */
static void w_SpriteTextAt895E(Carried *cr)
{
    cpu.a = cr->a;
    cpu.x = cr->x;
    cpu.y = cr->y;
    cpu.d = cr->d;
    cpu.p = cr->p;
    cpu.db = cr->db;
    abi_SpriteTextAt895E();
    cr->a = cpu.a;
    cr->x = cpu.x;
    cr->y = cpu.y;
    cr->d = cpu.d;
    cr->p = cpu.p;
    cr->db = cpu.db;
}

/* OamClassName as its adapter leaves the registers. */
static void w_OamClassName(Carried *cr)
{
    uint16_t x = cr->x, y = cr->y;

    cr->a = OamClassName(cr->db, &x, &y);
    cr->x = (cr->p & P_X) ? (uint8_t)x : (cyc(86), x);
    cr->y = (cr->p & P_X) ? (uint8_t)y : y;
}

/* SpriteTextAt8152, still in registers: its call with the registers loaded and taken
   back. */
static void w_SpriteTextAt8152(Carried *cr)
{
    cpu.a = cr->a;
    cpu.x = cr->x;
    cpu.y = cr->y;
    cpu.d = cr->d;
    cpu.p = cr->p;
    cpu.db = cr->db;
    abi_SpriteTextAt8152();
    cr->a = cpu.a;
    cr->x = cpu.x;
    cr->y = cpu.y;
    cr->d = cpu.d;
    cr->p = cpu.p;
    cr->db = cpu.db;
}

/* SpriteTextAt8152B, still in registers: its call with the registers loaded and taken
   back. */
static void w_SpriteTextAt8152B(Carried *cr)
{
    cpu.a = cr->a;
    cpu.x = cr->x;
    cpu.y = cr->y;
    cpu.d = cr->d;
    cpu.p = cr->p;
    cpu.db = cr->db;
    abi_SpriteTextAt8152B();
    cr->a = cpu.a;
    cr->x = cpu.x;
    cr->y = cpu.y;
    cr->d = cpu.d;
    cr->p = cpu.p;
    cr->db = cpu.db;
}

/* SpriteTextAt8952, still in registers: its call with the registers loaded and taken
   back. */
static void w_SpriteTextAt8952(Carried *cr)
{
    cpu.a = cr->a;
    cpu.x = cr->x;
    cpu.y = cr->y;
    cpu.d = cr->d;
    cpu.p = cr->p;
    cpu.db = cr->db;
    abi_SpriteTextAt8952();
    cr->a = cpu.a;
    cr->x = cpu.x;
    cr->y = cpu.y;
    cr->d = cpu.d;
    cr->p = cpu.p;
    cr->db = cpu.db;
}
