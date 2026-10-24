/*
 * course.c: the course number $0E3D, from the menu's selections.
 */
#include "machine.h"
#include "cpu_ops.h"
#include "carried.h"
#include "wram_names.h"

uint8_t SaveChecksum(void);
void abi_AllRecordsScreen(void);
void abi_BattleRecordsScreen(void);
void abi_CourseScreenFadeBack(void);
void abi_CourseScreenFadeLeave(void);
void abi_CourseZoomTimeUp(void);
void abi_CourseZoomTimeUpNext(void);
void abi_PreviewRowDrawAll(void);
void abi_PreviewRowDrawStunt(void);
void abi_SpeedRecordsScreen(void);
void abi_StuntRecordsScreen(void);
void abi_TimeUpCursorStep(void);
void abi_TimeUpSelectDraw(void);

static uint32_t dbw(uint8_t bank, uint16_t a)
{
    return (uint32_t)bank << 16 | a;
}

/*
 * @routine 02:EB51 CourseFromSelection
 * WHAT IT DOES: Sets the course number $0E3D: $0D4A * 4 + $0D48, or $0D4A
 * + $14 when $0D4C is set (not reached in the traces so far).
 * HOW: 8-bit arithmetic through the data bank.
 * Entry: M 8-bit. Exit: A low byte = the course; N, Z, C, V from the add.
 */
uint8_t CourseFromSelection(uint8_t bank, uint8_t *p)
{
    uint8_t a, b;
    unsigned s;

    if (rd8(dbw(bank, 0x0D4C))) {
        a = rd8(cyc_then(40, dbw(bank, 0x0D4A)));
        b = 0x14;
        cyc(30);                        /* CLC, ADC # */
    } else {
        a = rd8(cyc_then(46, dbw(bank, 0x0D4A)));
        a = (uint8_t)(a << 2);
        b = rd8(cyc_then(66, dbw(bank, 0x0D48)));
    }
    s = (unsigned)a + b;
    *p = (uint8_t)((nz8(*p, (uint8_t)s) & ~(P_C | P_V)) | (s > 0xFF ? P_C : 0)
                   | ((~(a ^ b) & (a ^ s) & 0x80) ? P_V : 0));
    wr8(dbw(bank, 0x0E3D), cyc_then(24, (uint8_t)s));
    cyc(42);
    return (uint8_t)s;
}

void BattleCourseSelectStep(Carried *cr);
void CourseCursorAt0(Carried *cr);
void CourseCursorAt1(Carried *cr);
void CourseCursorAt2(Carried *cr);
void CourseCursorAt2Open(Carried *cr);
void CourseCursorAt3(Carried *cr);
void CourseCursorAt3Open(Carried *cr);
void CourseCursorStep(Carried *cr);
void CourseFromSelection_cr(Carried *cr);
void CourseFromSelectionLong_cr(Carried *cr);
void CoursePreviewReset(Carried *cr);
void CoursePreviewRowsAll(Carried *cr);
void CoursePreviewRowsStunt(Carried *cr);
void CourseRecordsClosing(Carried *cr);
void CourseRecordsDispatch(Carried *cr);
void CourseRecordsOpening(Carried *cr);
void CourseRecordsReopen(Carried *cr);
void CourseRecordsSelectStep(Carried *cr);
void CourseRecordsShown(Carried *cr);
void CourseScreenFadeBack(Carried *cr);
void CourseScreenFadeLeave(Carried *cr);
void CourseScreenStartAll(Carried *cr);
void CourseScreenStartBattle(Carried *cr);
void CourseScreenStartSpeed(Carried *cr);
void CourseScreenStartStunt(Carried *cr);
void CourseScreenState8(Carried *cr);
void CourseScreenStep(Carried *cr);
void CourseScreenWindowStep(Carried *cr);
void CourseScreenWipeIn(Carried *cr);
void CourseScreenWipeOut(Carried *cr);
void CourseScreenZoomIn(Carried *cr);
void CourseScreenZoomOut(Carried *cr);
void CourseWindowZoomSetup(Carried *cr);
void CourseZoomBattle(Carried *cr);
void CourseZoomFromSelection(Carried *cr);
void CourseZoomFromSelection4(Carried *cr);
void CourseZoomInStep(Carried *cr);
void CourseZoomStunt(Carried *cr);
void CourseZoomTimeUp(Carried *cr);
void CourseZoomTimeUpNext(Carried *cr);
void LevelColumnsSet4(Carried *cr);
void RaceFadeStageDispatch(Carried *cr);
void SpeedCourseSelectStep(Carried *cr);
void StuntCourseSelectStep(Carried *cr);
void TimeUpCursorStep(Carried *cr);
void TimeUpSelectDraw(Carried *cr);
static uint16_t asl3(Carried *cr, uint16_t a);
static uint16_t box_offset(Carried *cr, uint16_t a, uint16_t add);
static uint16_t box_offset_in(Carried *cr, uint16_t a, uint16_t add, uint32_t clocks);
static void course_choose(Carried *cr, uint16_t list, uint16_t y);
static void course_zoom(Carried *cr, uint16_t x, uint16_t y);
static void course_zoom_box(Carried *cr, uint16_t a);
static void course_zoom_race(Carried *cr, uint16_t a);
static int cursor_pressed(Carried *cr, uint32_t b, uint16_t mask);
static void cursor_to(Carried *cr, uint32_t b, uint8_t to);
static void cursor_two(Carried *cr, uint32_t b, uint16_t m1, uint8_t to1, uint16_t m2, uint8_t to2, unsigned idle);
static int cursor_up_open(Carried *cr, uint32_t b);
static void fade_back_body(Carried *cr);
static void fade_done(Carried *cr, uint32_t b, uint8_t panel, uint8_t state);
static int fade_step(Carried *cr);
static void preview_rows(Carried *cr, int all, uint16_t last);
static int records_key(Carried *cr, uint32_t b);
static int records_key_inner(Carried *cr, uint32_t b);
static void records_pulse(Carried *cr, uint32_t b);
static void records_reopen_body(Carried *cr);
static void records_sprite(Carried *cr, uint32_t b, uint8_t y);
static void screen_state(Carried *cr, uint32_t b, uint16_t table);
static void screen_state2(Carried *cr, uint32_t b);
static void sel_choose(Carried *cr, uint32_t b);
static void sel_cursor(Carried *cr, uint32_t b, uint8_t y);
static int sel_open(Carried *cr, uint8_t bit);
static int sel_pressed(Carried *cr, uint32_t b, uint16_t mask);
static int sel_pressed16(Carried *cr, uint32_t b, uint16_t mask);
static void sel_row(Carried *cr, uint32_t b, uint8_t v);
static void sel_scroll(Carried *cr, uint32_t b, uint8_t v);
static void sel_sound(Carried *cr);
static void speed_select_body(Carried *cr);
static void time_up_cursor_body(Carried *cr);
static void time_up_set(Carried *cr, uint32_t at, uint8_t v);
static void time_up_sound(Carried *cr);
static void wipe_in_body(Carried *cr);
static void wipe_out_body(Carried *cr);
static void zoom_in_body(Carried *cr);
static void zoom_out_body(Carried *cr);
static void w_CourseScreenNothing(Carried *cr);
static void w_SaveChecksum(Carried *cr);
void AllRecordsScreen(Carried *cr);
void BattleRecordsScreen(Carried *cr);
void BlendStepDown_cr(Carried *cr);
void BlendStepUp_cr(Carried *cr);
void ColourMathA3Long_cr(Carried *cr);
void ColourMathA3_cr(Carried *cr);
void ColourMathOffLong_cr(Carried *cr);
void FixedColourFromStep_cr(Carried *cr);
void FlashingMetaspriteDraw0_cr(Carried *cr);
void FlashingMetaspriteDrawByMode_cr(Carried *cr);
void FlashingMetaspriteDraw_cr(Carried *cr);
void MenuCursorFourWay_cr(Carried *cr);
void OamBufferHideLargeLong_cr(Carried *cr);
void PreviewRowDrawAll_cr(Carried *cr);
void PreviewRowDrawStunt_cr(Carried *cr);
void RecordsLoadForCourse_cr(Carried *cr);
void RecordsUnpackCourseTotals(Carried *cr);
void Set0D37FromTable_cr(Carried *cr);
void SoundPort0QueuePush_cr(Carried *cr);
void SpeedRecordsScreen(Carried *cr);
void StuntRecordsScreen(Carried *cr);
void TilemapLevelColumns_cr(Carried *cr);
void WindowEdgesStep_cr(Carried *cr);
void WindowTable7FLong(Carried *cr);

void CourseFromSelection_cr(Carried *cr)
{
    cyc(24);
    uint8_t p = cr->p;

    cr->a = (uint16_t)((cr->a & 0xFF00) | CourseFromSelection(cr->db, &p));
    set_p(cr, p);
}

void abi_CourseFromSelection(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CourseFromSelection_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 02:EB4D CourseFromSelectionLong
 * WHAT IT DOES: CourseFromSelection for callers in other banks.
 * HOW: JSR to it, then RTL.
 * Entry: as CourseFromSelection. Exit: as CourseFromSelection.
 */
uint8_t CourseFromSelectionLong(uint8_t bank, uint8_t *p)
{
    return CourseFromSelection(bank, p);
}

void CourseFromSelectionLong_cr(Carried *cr)
{
    cyc(46);
    CourseFromSelection_cr(cr);
    cyc(44);
}

void abi_CourseFromSelectionLong(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CourseFromSelectionLong_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void abi_SoundPort0QueuePush(void);
void abi_ColourMathA3Long(void);
void abi_BlendStepUp(void);
void abi_FixedColourFromStep(void);
void abi_RecordsLoadForCourse(void);

/* LDA $0D1A, BIT #mask (16-bit): whether a button in mask was pressed; Z
   from the BIT, N from the load. */
static int cursor_pressed(Carried *cr, uint32_t b, uint16_t mask)
{
    cyc(24);
    r_lda16(cr, b | W(joy1_pressed_copy));
    set_p(cr, (cr->a & mask) ? (cyc(94), (uint8_t)(cr->p & ~P_Z)) : (cyc(40), (uint8_t)(cr->p | P_Z)));
    return (cr->a & mask) != 0;
}

/* SEP #$20, sound $45, then the cursor $0D48 = to. */
static void cursor_to(Carried *cr, uint32_t b, uint8_t to)
{
    r_sep(cr, P_M);
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0x45);
    set_p(cr, nz8(cr->p, 0x45));
    cyc(16);
    SoundPort0QueuePush_cr(cr);
    cr->a = (uint16_t)((cr->a & 0xFF00) | to);
    set_p(cr, nz8(cr->p, to));
    cyc(40);
    wr8(b | W(wram_0D48), to);
}

/* Up to the fourth course when it is open (the low four bits of $70:FC06
   all set): sound $45 and $0D63 = $FF; 1 when it went there. */
static int cursor_up_open(Carried *cr, uint32_t b)
{
    uint8_t v;

    r_sep(cr, P_M);
    v = (uint8_t)(rd8(0x70FC06) & 0x0F);
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    set_p(cr, (uint8_t)((nz8(cr->p, (uint8_t)(v - 0x0F)) & ~P_C) | (v >= 0x0F ? (cyc(110), P_C) : (cyc(48), 0))));
    if (v != 0x0F)
        return 0;
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0x45);
    set_p(cr, nz8(cr->p, 0x45));
    cyc(16);
    SoundPort0QueuePush_cr(cr);
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0xFF);
    set_p(cr, nz8(cr->p, 0xFF));
    cyc(40);
    wr8(b | W(wram_0D63), 0xFF);
    return 1;
}

/* REP #$20, then the first of two buttons pressed moves the cursor; with
   neither, `idle` more than the not-taken branch (the cursors at 3 end with
   a taken BEQ). */
static void cursor_two(Carried *cr, uint32_t b, uint16_t m1, uint8_t to1, uint16_t m2, uint8_t to2, unsigned idle)
{
    r_rep(cr, P_M);
    if (cursor_pressed(cr, b, m1))
        { cyc(36); cursor_to(cr, b, to1); }
    else if (cursor_pressed(cr, b, m2))
        { cyc(idle ? 54 : 36); cursor_to(cr, b, to2); }   /* at 3: BEQ not taken, JMP */
    else
        cyc(idle);
    cyc(42);
}

/*
 * @routine 02:ECB8 CourseCursorAt0
 * WHAT IT DOES: The course cursor at 0 (bottom left): Right ($0100 in the
 * pressed buttons $0D1A) moves it to 1, Up ($0800) to 2, each with sound
 * $45.
 * HOW: 16-bit BIT tests; the move is 8-bit.
 * Entry: any widths. Exit: M 8-bit after a move (A low byte the new
 * place), else M 16-bit with A = the buttons and Z set.
 */
void CourseCursorAt0(Carried *cr)
{
    cursor_two(cr, (uint32_t)cr->db << 16, 0x0100, 0x01, 0x0800, 0x02, 0);
}

void abi_CourseCursorAt0(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CourseCursorAt0(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 02:ECD9 CourseCursorAt1
 * WHAT IT DOES: The course cursor at 1 (bottom right): Left ($0200) moves
 * it to 0, Up ($0800) to 3.
 * HOW: As CourseCursorAt0.
 * Entry: any widths. Exit: as CourseCursorAt0.
 */
void CourseCursorAt1(Carried *cr)
{
    cursor_two(cr, (uint32_t)cr->db << 16, 0x0200, 0x00, 0x0800, 0x03, 0);
}

void abi_CourseCursorAt1(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CourseCursorAt1(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 02:ECFA CourseCursorAt2
 * WHAT IT DOES: The course cursor at 2 (top left): Right ($0100) moves it
 * to 3, Down ($0400) to 0.
 * HOW: As CourseCursorAt0.
 * Entry: any widths. Exit: as CourseCursorAt0.
 */
void CourseCursorAt2(Carried *cr)
{
    cursor_two(cr, (uint32_t)cr->db << 16, 0x0100, 0x03, 0x0400, 0x00, 0);
}

void abi_CourseCursorAt2(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CourseCursorAt2(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 02:ED1B CourseCursorAt3
 * WHAT IT DOES: The course cursor at 3 (top right): Left ($0200) moves it
 * to 2, Down ($0400) to 1.
 * HOW: As CourseCursorAt0. The move to 1 is not reached in the traces so
 * far.
 * Entry: any widths. Exit: as CourseCursorAt0.
 */
void CourseCursorAt3(Carried *cr)
{
    cursor_two(cr, (uint32_t)cr->db << 16, 0x0200, 0x02, 0x0400, 0x01, 6);
}

void abi_CourseCursorAt3(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CourseCursorAt3(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 02:EC30 CourseCursorAt2Open
 * WHAT IT DOES: As CourseCursorAt2 where a fourth course can open: first,
 * Up ($0800) with the low four bits of $70:FC06 all set gives sound $45
 * and $0D63 = $FF; otherwise Right ($0100) moves the cursor to 3, Down
 * ($0400) to 0.
 * HOW: As CourseCursorAt0; the Up test is 8-bit. Up is not reached in the
 * traces so far.
 * Entry: any widths. Exit: as CourseCursorAt0; after Up, A low byte $FF.
 */
void CourseCursorAt2Open(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;

    r_rep(cr, P_M);
    if (cursor_pressed(cr, b, 0x0800) && cursor_up_open(cr, b))
        { cyc(42); return; } else { cyc(6); }
    cursor_two(cr, b, 0x0100, 0x03, 0x0400, 0x00, 0);
}

void abi_CourseCursorAt2Open(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CourseCursorAt2Open(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 02:EC73 CourseCursorAt3Open
 * WHAT IT DOES: As CourseCursorAt3 where a fourth course can open: Up as
 * in CourseCursorAt2Open; otherwise Left ($0200) moves the cursor to 2,
 * Down ($0400) to 1.
 * HOW: As CourseCursorAt2Open. Up and Down are not reached in the traces
 * so far.
 * Entry: any widths. Exit: as CourseCursorAt2Open.
 */
void CourseCursorAt3Open(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;

    r_rep(cr, P_M);
    if (cursor_pressed(cr, b, 0x0800) && cursor_up_open(cr, b))
        { cyc(42); return; } else { cyc(6); }
    cursor_two(cr, b, 0x0200, 0x02, 0x0400, 0x01, 6);
}

void abi_CourseCursorAt3Open(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CourseCursorAt3Open(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 02:EC20 CourseCursorStep
 * WHAT IT DOES: Moves the course cursor $0D48 by the pressed buttons: the
 * handler for its place from the table at $02:EC28 (CourseCursorAt0 to
 * CourseCursorAt3).
 * HOW: LDA, ASL, TAX in the current widths (with M 8-bit and X 16-bit, TAX
 * takes A's high byte too), then JMP (table,X).
 * Entry: any widths. Exit: as the handler leaves it.
 */
void CourseCursorStep(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;

    if (cr->p & P_M) {
        uint8_t v = rd8(b | W(wram_0D48));

        set_p(cr, (uint8_t)((cr->p & ~P_C) | (v >> 7)));
        v = (uint8_t)(v << 1);
        cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    } else {
        cyc(24);
        uint16_t v = rd16(b | W(wram_0D48));

        set_p(cr, (uint8_t)((cr->p & ~P_C) | (v >> 15)));
        cr->a = (uint16_t)(v << 1);
    }
    cr->x = (cr->p & P_X) ? (uint8_t)cr->a : cr->a;
    set_p(cr, (cr->p & P_X) ? nz8(cr->p, (uint8_t)cr->x) : nz16(cr->p, cr->x));
    cyc(74);
    carried_call(cr, 0x020000 | rd16(0x020000 | (uint16_t)(0xEC28 + cr->x)), 0);
}

void abi_CourseCursorStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CourseCursorStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 02:ED3E CourseZoomInStep
 * WHAT IT DOES: A step of the course window's zoom: colour maths $A3
 * (ColourMathA3Long), the blend one step on (BlendStepUp) and the fixed
 * colour from it (FixedColourFromStep); at the last step, $0D50 = 0, the
 * course's records loaded (RecordsLoadForCourse), $0D41 = $20, $0D40 = 6
 * and $10D8 = 0.
 * HOW: FixedColourFromStep leaves M and X 8-bit, so the compare of
 * $7F:3408 with $7F:340A is of their low bytes; 8-bit stores.
 * Entry: any widths. Exit: M, X 8-bit; before the last step A low byte the
 * blend factor's (high byte as FixedColourFromStep left it), Z clear;
 * then A low byte 6.
 */
void CourseZoomInStep(Carried *cr)
{
    cyc(62);
    uint32_t b = (uint32_t)cr->db << 16;
    uint8_t a8, m8;

    ColourMathA3Long_cr(cr);
    r_rep(cr, P_M | P_X);
    (cyc(62), BlendStepUp_cr(cr));
    (cyc(62), FixedColourFromStep_cr(cr));
    cyc(32);
    r_lda8(cr, W(fixed_colour_step));
    a8 = (uint8_t)cr->a;
    cyc(32);
    m8 = rd8(W(scale_divisor));
    set_p(cr, (uint8_t)((nz8(cr->p, (uint8_t)(a8 - m8)) & ~P_C) | (a8 >= m8 ? P_C : (cyc(64), 0))));
    if (a8 != m8)
        return;
    r_sep(cr, P_M);
    cyc(62);
    wr8(b | W(wram_0D50), 0x00);
    (cyc(62), RecordsLoadForCourse_cr(cr));
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0x20);
    set_p(cr, nz8(cr->p, 0x20));
    cyc(40);
    wr8(b | W(wram_0D41), 0x20);
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0x06);
    set_p(cr, nz8(cr->p, 0x06));
    cyc(40);
    wr8(b | W(wram_0D40), 0x06);
    cyc(24);
    wr8(b | W(wram_10D8), 0x00);
    cyc(42);
}

void abi_CourseZoomInStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CourseZoomInStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 02:EC10 RaceFadeStageDispatch
 * WHAT IT DOES: Runs the handler for stage $0D48 from the table at
 * $02:EC18 ($ECB8, $ECD9, $EC30, $EC73).
 * HOW: ASL, TAX, JMP (table,X) with 8-bit registers; the handler returns
 * to the caller.
 * Entry: M 8-bit, X 8-bit. Exit: as the handler leaves it.
 */
void RaceFadeStageDispatch(Carried *cr)
{
    uint8_t v;

    cyc(24);
    r_lda8(cr, (uint32_t)cr->db << 16 | 0x0D48);
    v = (uint8_t)cr->a;
    set_p(cr, (uint8_t)((cr->p & ~P_C) | (v >> 7)));
    v = (uint8_t)(v << 1);
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    cr->x = v;
    set_p(cr, nz8(cr->p, v));
    cyc(74);
    carried_call(cr, 0x020000 | rd16(0x020000 | (uint16_t)(0xEC18 + v)), 0);
}

void abi_RaceFadeStageDispatch(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RaceFadeStageDispatch(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void abi_ColourMathA3(void);
void abi_OamBufferHideLargeLong(void);
void abi_SoundPort0QueuePush(void);
void abi_WindowTable7FLong(void);

/*
 * @routine 03:FBB6 CourseWindowZoomSetup
 * WHAT IT DOES: Sets up the course window's zoom: the four words at
 * $7F:3432 and their bases at $7F:3442 from the four words at $03:FC11 +
 * X, their targets at $7F:343A from the four at $03:FB8E + Y, the blend
 * factor $7F:3408 = 0 and its limit $7F:340A = 15; then builds the window
 * table from them (WindowTable7FLong).
 * HOW: 16-bit loads through the data bank ($03), long stores.
 * Entry: M and X 16-bit; data bank $03. Exit: as WindowTable7FLong.
 */
void CourseWindowZoomSetup(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;
    unsigned i;

    cyc(0);
    for (i = 0; i < 4; i++) {
        cyc(78);
        cr->a = rd16(b + 0xFB8E + cr->y + 2 * i);
        wr16(W(scaled_words_target) + 2 * i, cr->a);
    }
    for (i = 0; i < 4; i++) {
        cyc(78);
        cr->a = rd16(b + 0xFC11 + cr->x + 2 * i);
        wr16(W(scaled_words) + 2 * i, cr->a);
        cyc(32);
        wr16(W(scaled_words_base) + 2 * i, cr->a);
    }
    cyc(56);
    wr16(W(scale_divisor), 0x000F);
    cr->a = 0x0000;
    set_p(cr, nz16(cr->p, 0x0000));
    cyc(56);
    wr16(W(fixed_colour_step), 0x0000);
    (cyc(62), WindowTable7FLong(cr));
    cyc(150);   /* WindowTable7FLong states its RTL */
}

void abi_CourseWindowZoomSetup(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CourseWindowZoomSetup(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* 03:FB64, the zoom's start from X and Y (the offsets for
   CourseWindowZoomSetup): with the data bank $03, the window set up,
   colour maths $A3, the large sprites hidden, $0D40 = 4 and sound $24. */
static void course_zoom(Carried *cr, uint16_t x, uint16_t y)
{
    uint8_t db0 = cr->db;

    cr->x = x;
    cr->y = y;
    r_sep(cr, P_M);
    cr->db = 0x03;
    set_p(cr, nz8(cr->p, 0x03));
    r_rep(cr, P_M | P_X);
    CourseWindowZoomSetup(cr);
    ColourMathA3_cr(cr);
    (cyc(62), OamBufferHideLargeLong_cr(cr));
    r_sep(cr, P_M);
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0x04);
    cyc(62);
    wr8(0x030D40, 0x04);
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0x24);
    set_p(cr, nz8(cr->p, 0x24));
    (cyc(78), SoundPort0QueuePush_cr(cr));
    r_sep(cr, P_M);
    cr->db = db0;
    set_p(cr, nz8(cr->p, db0));
}

/* CLC, ADC #add, TAX: the start box's offset. */
static uint16_t box_offset_in(Carried *cr, uint16_t a, uint16_t add, uint32_t clocks)
{
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_C)));
    cr->a = alu_adc16(a, add, &cr->p);
    cyc(clocks);
    return cr->a;
}

static uint16_t box_offset(Carried *cr, uint16_t a, uint16_t add)
{
    return box_offset_in(cr, a, add, 262);
}

/* ASL A three times (16-bit): C the last bit out. */
static uint16_t asl3(Carried *cr, uint16_t a)
{
    set_p(cr, (uint8_t)((cr->p & ~P_C) | (a & 0x2000 ? P_C : 0)));
    a = (uint16_t)(a << 3);
    set_p(cr, nz16(cr->p, a));
    return a;
}

/* The course chosen from the cursor $0D48 in the four-entry list at
   `list` (a word load, the high byte dropped), then the zoom from box
   $0D48 * 8 + $80 to target y. */
static void course_choose(Carried *cr, uint16_t list, uint16_t y)
{
    cyc(24);
    uint32_t b = (uint32_t)cr->db << 16;
    uint16_t a;

    r_rep(cr, P_M | P_X);
    cr->x = rd16(b | W(wram_0D48));
    cyc(96);
    a = (uint16_t)(rd16(0x030000 + list + cr->x) & 0x00FF);
    cr->a = a;
    set_p(cr, nz16(cr->p, a));
    wr16(b | W(course), a);
    cyc(list == 0xFB86 ? 48 : 24);      /* the stunt list: AND #$00FF before the LDA */
    a = asl3(cr, rd16(b | W(wram_0D48)));
    /* TAX, LDY #, then the stunt list's BRA to the zoom (the battle list falls into it) */
    course_zoom(cr, box_offset_in(cr, a, 0x0080, list == 0xFB86 ? 280 : 258), y);
    cyc(94);
}

/*
 * @routine 03:FB25 CourseZoomStunt
 * WHAT IT DOES: Sets the course $0E3D from the cursor $0D48 in the list at
 * $03:FB86 (courses $0C-$0F), then starts the course window's zoom from
 * box $0D48 * 8 + $80 at $03:FC11 to target 8 at $03:FB8E: colour maths
 * $A3, the large sprites hidden, $0D40 = 4 and sound $24.
 * HOW: 16-bit; the zoom is the tail of CourseZoomBattle (BRA $FB64), with
 * the data bank $03 around it.
 * Entry: any widths. Exit: M 8-bit, X 16-bit; the data bank restored; N,
 * Z from it; A, X, Y as SoundPort0QueuePush leaves them.
 */
void CourseZoomStunt(Carried *cr)
{
    course_choose(cr, 0xFB86, 0x0008);
}

void abi_CourseZoomStunt(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CourseZoomStunt(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:FB47 CourseZoomBattle
 * WHAT IT DOES: As CourseZoomStunt with the list at $03:FB8A (courses
 * $10-$13) and target $10.
 * HOW: As CourseZoomStunt; the zoom's tail at $FB64 is also where the
 * other course choices go.
 * Entry: any widths. Exit: as CourseZoomStunt.
 */
void CourseZoomBattle(Carried *cr)
{
    course_choose(cr, 0xFB8A, 0x0010);
}

void abi_CourseZoomBattle(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CourseZoomBattle(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* Courses below $0C: box course * 8 (+ $40 with a HUD mode, target $18;
   else target 0). */
static void course_zoom_race(Carried *cr, uint16_t a)
{
    a = asl3(cr, a);
    cr->x = rd16(((uint32_t)cr->db << 16) | 0x0D2B);
    set_p(cr, nz16(cr->p, cr->x));
    if (cr->x)
        course_zoom(cr, box_offset_in(cr, a, 0x0040, 256), 0x0018);   /* BEQ not taken */
    else
        course_zoom(cr, box_offset(cr, a, 0x0000), 0x0000);
}

/* 03:FA88 and 03:FA98: box $B0 or $A8, target $20. */
static void course_zoom_box(Carried *cr, uint16_t a)
{
    r_rep(cr, P_M | P_X);
    cr->a = a;
    course_zoom(cr, box_offset(cr, a, 0x00A8), 0x0020);
}

/*
 * @routine 03:FA51 CourseZoomFromSelection4
 * WHAT IT DOES: Sets the course from the menu's selections
 * (CourseFromSelectionLong) and starts the course window's zoom: for a
 * course below $0C from box (course - 4) * 8 (+ $40, target $18, with a
 * HUD mode $0D2B; else target 0), for course $15 from box $A8 and for the
 * others from box $B0, both to target $20.
 * HOW: As CourseZoomStunt, through the zoom's tail at $03:FB64. A HUD mode
 * is not reached in the traces so far.
 * Entry: any widths. Exit: as CourseZoomStunt.
 */
void CourseZoomFromSelection4(Carried *cr)
{
    cyc(84);
    uint8_t v;
    uint16_t a;

    r_sep(cr, P_M);
    CourseFromSelectionLong_cr(cr);
    v = (uint8_t)cr->a;
    set_p(cr, (uint8_t)((nz8(cr->p, (uint8_t)(v - 0x0C)) & ~P_C) | (v >= 0x0C ? (cyc(88), P_C) : (cyc(160), 0))));
    if (v < 0x0C) {
        r_rep(cr, P_M | P_X);
        a = (uint16_t)(cr->a & 0x00FF);
        set_p(cr, (uint8_t)(cr->p | (P_C)));
        a = alu_sbc16(a, 0x0004, &cr->p);
        course_zoom_race(cr, a);
    } else if (v == 0x15)
        course_zoom_box(cr, 0x0000);
    else
        course_zoom_box(cr, 0x0008);
    cyc(94);
}

void abi_CourseZoomFromSelection4(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CourseZoomFromSelection4(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:FAA8 CourseZoomFromSelection
 * WHAT IT DOES: As CourseZoomFromSelection4 with box course * 8 for the
 * courses below $0C, and boxes $B0 for course $15 and $A8 for the others.
 * HOW: 8-bit index registers for CourseFromSelectionLong; the other
 * choices go to CourseZoomFromSelection4's. A HUD mode is not reached in
 * the traces so far.
 * Entry: any widths. Exit: as CourseZoomStunt.
 */
void CourseZoomFromSelection(Carried *cr)
{
    uint8_t v;

    r_sep(cr, P_M | P_X);
    (cyc(62), CourseFromSelectionLong_cr(cr));
    v = (uint8_t)cr->a;
    set_p(cr, (uint8_t)((nz8(cr->p, (uint8_t)(v - 0x0C)) & ~P_C) | (v >= 0x0C ? (cyc(88), P_C) : (cyc(128), 0))));
    if (v < 0x0C) {
        r_rep(cr, P_M | P_X);
        course_zoom_race(cr, (uint16_t)(cr->a & 0x00FF));
    } else if (v == 0x15)
        course_zoom_box(cr, 0x0008);
    else
        course_zoom_box(cr, 0x0000);
    cyc(94);
}

void abi_CourseZoomFromSelection(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CourseZoomFromSelection(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void abi_TilemapLevelColumns(void);
void abi_OamBufferHideLargeLong(void);
void abi_FlashingMetaspriteDrawByMode(void);
void abi_FlashingMetaspriteDraw(void);
void abi_MenuCursorFourWay(void);
void abi_Set0D37FromTable(void);
void abi_SoundPort0QueuePush(void);

/*
 * @routine 02:E7E8 LevelColumnsSet4
 * WHAT IT DOES: Stores A in the four bytes of $0F05 + X (a level column
 * group).
 * HOW: Four STA.W abs,X, as wide as M.
 * Entry: M 8-bit; X = the offset; data bank $7E or a mirror. Exit: all
 * unchanged.
 */
void LevelColumnsSet4(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;
    int i;

    cyc(0);
    for (i = 0; i < 4; i++) {
        if (cr->p & P_M)
            { cyc(30); wr8(b | (uint16_t)(0x0F05 + i + cr->x), (uint8_t)cr->a); }
        else
            wr16(b | (uint16_t)(0x0F05 + i + cr->x), cr->a);
    }
    cyc(42);
}

void abi_LevelColumnsSet4(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    LevelColumnsSet4(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* LDA.L $70:FC0B, BIT #bit: whether that course group is open; N, V from
   the save byte, Z from the test. */
static int sel_open(Carried *cr, uint8_t bit)
{
    cyc(32);
    uint8_t v = rd8(0x70FC0B);

    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    set_p(cr, (uint8_t)((cr->p & ~(P_N | P_V | P_Z)) | (v & 0xC0) | ((v & bit) ? (cyc(54), 0) : P_Z)));
    return (v & bit) != 0;
}

/* REP #$20, LDA $0D1A, BIT #mask, SEP #$20: whether pressed. */
static int sel_pressed(Carried *cr, uint32_t b, uint16_t mask)
{
    uint16_t v;

    r_rep(cr, P_M);
    v = rd16(b | W(joy1_pressed_copy));
    cr->a = v;
    set_p(cr, (uint8_t)((cr->p & ~(P_N | P_V | P_Z)) | ((v >> 8) & 0xC0) | ((v & mask) ? (cyc(84), 0) : (cyc(24), P_Z))));
    r_sep(cr, P_M);
    return (v & mask) != 0;
}

/* REP #$20, LDA $0D1A, BIT #mask (left 16-bit): whether pressed. */
static int sel_pressed16(Carried *cr, uint32_t b, uint16_t mask)
{
    r_rep(cr, P_M);
    r_lda16(cr, b | W(joy1_pressed_copy));
    set_p(cr, (uint8_t)((cr->p & ~(P_N | P_V | P_Z)) | ((cr->a >> 8) & 0xC0) | ((cr->a & mask) ? 0 : (cyc(46), P_Z))));
    return (cr->a & mask) != 0;
}

/* LDA #$45, JSL SoundPort0QueuePush: the move sound. */
static void sel_sound(Carried *cr)
{
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0x45);
    set_p(cr, nz8(cr->p, 0x45));
    (cyc(56), SoundPort0QueuePush_cr(cr));
    cyc(16);
}

/* LDA #v, STA $0D4A: the cursor's row. */
static void sel_row(Carried *cr, uint32_t b, uint8_t v)
{
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    set_p(cr, nz8(cr->p, v));
    cyc(24);
    wr8(b | W(wram_0D4A), v);
}

/* The scroll $0E37 moved 4 toward $50 (in) or 0 (out), then $0E92 = $F0
   and the large sprites hidden. */
static void sel_scroll(Carried *cr, uint32_t b, uint8_t v)
{
    cyc(118);
    wr8(b | W(scroll_value), v);
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0xF0);
    set_p(cr, nz8(cr->p, 0xF0));
    cyc(40);
    wr8(b | W(wram_0E92), 0xF0);
    (cyc(62), OamBufferHideLargeLong_cr(cr));
}

/* LDA #y, STA $0D38, JSR MenuCursorFourWay. */
static void sel_cursor(Carried *cr, uint32_t b, uint8_t y)
{
    cr->a = (uint16_t)((cr->a & 0xFF00) | y);
    set_p(cr, nz8(cr->p, y));
    cyc(56);
    wr8(b | W(flash_sprite_y), y);
    (cyc(46), MenuCursorFourWay_cr(cr));
}

/* 02:E954: Start or A ($9000) zooms into the course (the page's own
   zoom); otherwise the four-way cursor with its sprite at y $B0 on the
   page's first row, $60 on the other. */
static void sel_choose(Carried *cr, uint32_t b)
{
    uint8_t page = rd8(b | W(wram_0D63)), v;

    cr->a = (uint16_t)((cr->a & 0xFF00) | page);
    set_p(cr, nz8(cr->p, page));
    cyc(page ? 46 : 40);                /* BNE taken or not */
    if (sel_pressed16(cr, b, 0x9000)) {
        if (page)
            { cyc(102); CourseZoomFromSelection4(cr); }
        else
            { cyc(102); CourseZoomFromSelection(cr); }
        return;
    }
    r_sep(cr, P_M);
    cyc(46);
    r_lda8(cr, b | W(wram_0D4A));
    v = (uint8_t)cr->a;
    if (page) {
        cyc(16);
        set_p(cr, (uint8_t)((nz8(cr->p, (uint8_t)(v - 0x01)) & ~P_C) | (v >= 0x01 ? P_C : 0)));
        sel_cursor(cr, b, v == 0x01 ? 0xB0 : (cyc(6), 0x60));
    } else {
        sel_cursor(cr, b, v ? (cyc(6), 0x60) : 0xB0);
    }
    cyc(42);
}

/* The body of SpeedCourseSelectStep, up to its RTS. */
static void speed_select_body(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;
    static const uint8_t bits[3] = {0x01, 0x02, 0x04};
    uint8_t v;
    int i, open;


    for (i = 0; i < 3; i++) {
        open = sel_open(cr, bits[i]);
        /* the first copy branches with BNE, the others with BEQ */
        v = open ? (cyc(i ? 78 : 62), 0x00) : (cyc(i ? 116 : 132), 0x04);
        cr->a = (uint16_t)((cr->a & 0xFF00) | v);
        set_p(cr, nz8(cr->p, v));
        cr->x = (uint16_t)(i * 4);
        set_p(cr, nz8(cr->p, (uint8_t)cr->x));
        LevelColumnsSet4(cr);
    }
    (cyc(46), TilemapLevelColumns_cr(cr));
    cyc(24);                           /* its end is TilemapLevelColumns' now */
    r_lda8(cr, b | W(fade_level_hi));
    v = (uint8_t)cr->a;
    set_p(cr, (uint8_t)((nz8(cr->p, (uint8_t)(v - 0x0F)) & ~P_C) | (v >= 0x0F ? P_C : (cyc(32), 0))));
    if (v != 0x0F)
        return;
    r_sep(cr, P_M);
    cyc(84);
    r_lda8(cr, b | W(wram_0D63));
    if (!(uint8_t)cr->a) {
        cyc(40);
        r_lda8(cr, b | W(scroll_value));
        v = (uint8_t)cr->a;
        if (v != 0x50) {
            cyc(32);
            v = (uint8_t)(v + 4);
            set_p(cr, (uint8_t)((cr->p & ~P_C) | (v >= 0x50 ? (cyc(10), P_C) : 0)));
            if (v >= 0x50)
                v = 0x50;
            sel_scroll(cr, b, v);
            return;
        }
        (cyc(84), FlashingMetaspriteDrawByMode_cr(cr));
        cyc(24);
        r_lda8(cr, b | W(wram_0D4A));
        if (!(uint8_t)cr->a) {
            cyc(38);
            r_sep(cr, P_M);
            if (sel_open(cr, 0x02) && (cyc(2), sel_pressed(cr, b, 0x0800))) {
                cyc(6);
                sel_sound(cr);
                sel_row(cr, b, 0x01);
                return;
            } else { cyc(86); }
            sel_choose(cr, b);
            return;
        } else { cyc(44); }
        r_sep(cr, P_M);
        open = sel_open(cr, 0x04);
        if (open && (cyc(2), sel_pressed(cr, b, 0x0800))) {
            sel_sound(cr);
            cyc(8);
            v = (uint8_t)(rd8(b | W(wram_0D63)) - 1);
            cyc(6);
            wr8(b | W(wram_0D63), v);
            cyc(16);
            sel_row(cr, b, 0x02);
            return;
        } else { cyc(open ? 66 : 60); }  /* BEQ taken at once: 6 less */
        r_sep(cr, P_M);
        open = sel_open(cr, 0x01);
        if (open && (cyc(2), sel_pressed(cr, b, 0x0400))) {
            sel_sound(cr);
            cyc(8);
            wr8(b | W(wram_0D4A), 0x00);
            return;
        } else { cyc(open ? 68 : 62); }
        sel_choose(cr, b);
        return;
    }
    cyc(46);
    r_lda8(cr, b | W(scroll_value));
    v = (uint8_t)cr->a;
    if (v) {
        cyc(6);
        v = (uint8_t)(v - 4);
        if (v & 0x80)
            { cyc(10); v = 0x00; }
        sel_scroll(cr, b, v);
        return;
    } else { cyc(92); }
    FlashingMetaspriteDrawByMode_cr(cr);
    cyc(24);
    r_lda8(cr, b | W(wram_0D4A));
    v = (uint8_t)cr->a;
    set_p(cr, (uint8_t)((nz8(cr->p, (uint8_t)(v - 0x01)) & ~P_C) | (v >= 0x01 ? P_C : 0)));
    cyc(56);
    if (v == 0x01) {
        if (sel_pressed(cr, b, 0x0400)) {
            r_sep(cr, P_M);
            if (sel_open(cr, 0x01)) {
                sel_sound(cr);
                cyc(8);
                wr8(b | W(wram_0D63), 0x00);
                cyc(24);
                wr8(b | W(wram_0D4A), 0x00);
                return;
            } else { cyc(62); }
        } else { cyc(68); }
        if (sel_pressed(cr, b, 0x0800)) {
            r_sep(cr, P_M);
            if (sel_open(cr, 0x04)) {
                sel_sound(cr);
                sel_row(cr, b, 0x02);
                return;
            } else { cyc(62); }
        } else { cyc(68); }
        sel_choose(cr, b);
        return;
    } else { cyc(6); }
    if (sel_pressed(cr, b, 0x0400)) {
        sel_sound(cr);
        sel_row(cr, b, 0x01);
        return;
    } else { cyc(68); }
    sel_choose(cr, b);
}

/*
 * @routine 02:E7F5 SpeedCourseSelectStep
 * WHAT IT DOES: The Speed Trax course select's frame: the level columns
 * for groups 0-2 by the save's opened bits ($70:FC0B bits 0-2: 0 open, 4
 * closed; group 0 open with bit 0 set, groups 1 and 2 with theirs) drawn
 * (TilemapLevelColumns); once the fade is full ($0D61 = $0F), on page 0
 * ($0D63 = 0) the scroll $0E37 runs up to $50 by 4, on page 1 down to 0;
 * there, the cursor's rows: Up ($0800) and Down ($0400) move it between
 * the open groups (sound $45, $0D4A, and $0D63 down on page 1's top), and
 * otherwise Start or A zooms into the course and the cursor moves.
 * HOW: 8-bit; the pads' word 16-bit.
 * Entry: M 8-bit, X 8-bit; data bank $7E or a mirror. Exit: M 8-bit; as
 * the last step leaves it.
 */
void SpeedCourseSelectStep(Carried *cr)
{
    speed_select_body(cr);
    cyc(42);                            /* RTS */
}

void abi_SpeedCourseSelectStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    SpeedCourseSelectStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void abi_StuntSelectMarks(void);
void abi_StuntSelectCursor(void);

/*
 * @routine 02:E9AB StuntCourseSelectStep
 * WHAT IT DOES: The Stunt Trax course select's frame, once the fade is
 * full ($0D61 = $0F): on page 0 ($0D63 = 0) the scroll $0E37 runs up to
 * $50 by 4, then the course's picture setting (Set0D37FromTable), the
 * flashing sprite, and Start or A zooms in (CourseZoomStunt), otherwise
 * the cursor (RaceFadeStageDispatch); on page 1 (which no run shows) the
 * scroll runs down to 0, then TimeUpSelectDraw, the flashing sprite, and
 * Start or A zooms by $0D4E (CourseZoomTimeUp or CourseZoomTimeUpNext),
 * otherwise TimeUpCursorStep.
 * HOW: 8-bit; the pads' word 16-bit.
 * Entry: M 8-bit, X 8-bit; data bank $7E or a mirror. Exit: M 8-bit, or
 * 16-bit after a zoom; as the last step leaves it.
 */
void StuntCourseSelectStep(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;
    uint8_t v;

    cyc(24);
    r_lda8(cr, b | W(fade_level_hi));
    v = (uint8_t)cr->a;
    set_p(cr, (uint8_t)((nz8(cr->p, (uint8_t)(v - 0x0F)) & ~P_C) | (v >= 0x0F ? P_C : 0)));
    if (v != 0x0F)
        { cyc(74); return; }
    cyc(62);
    r_lda8(cr, b | W(wram_0D63));
    if (!(uint8_t)cr->a) {
        cyc(40);
        r_lda8(cr, b | W(scroll_value));
        v = (uint8_t)cr->a;
        if (v != 0x50) {
            cyc(32);
            v = (uint8_t)(v + 4);
            set_p(cr, (uint8_t)((cr->p & ~P_C) | (v >= 0x50 ? (cyc(10), P_C) : 0)));
            if (v >= 0x50)
                v = 0x50;
            sel_scroll(cr, b, v);
            cyc(42);
            return;
        }
        (cyc(84), Set0D37FromTable_cr(cr));
        (cyc(46), FlashingMetaspriteDraw_cr(cr));
        cyc(24);
        if (sel_pressed16(cr, b, 0x9000)) {
            cyc(102);
            CourseZoomStunt(cr);
            cyc(42);
            return;
        } else { cyc(68); }
        r_sep(cr, P_M);
        RaceFadeStageDispatch(cr);
        cyc(42);
        return;
    }
    cyc(46);
    r_lda8(cr, b | W(scroll_value));
    v = (uint8_t)cr->a;
    if (v) {
        v = (uint8_t)(v - 4);
        if (v & 0x80)
            { cyc(10); v = 0x00; }
        sel_scroll(cr, b, v);
        cyc(42);
        return;
    } else { cyc(68); }
    TimeUpSelectDraw(cr);  /* TimeUpSelectDraw */
    (cyc(46), FlashingMetaspriteDraw_cr(cr));
    cyc(24);
    if (sel_pressed16(cr, b, 0x9000)) {
        r_sep(cr, P_M);
        cyc(86);                        /* BIT, BEQ, SEP, LDA abs */
        r_lda8(cr, b | W(hud_blank));
        if ((uint8_t)cr->a)
            { cyc(84); CourseZoomTimeUpNext(cr); }  /* BNE taken, JSL: CourseZoomTimeUpNext */
        else
            { cyc(78); CourseZoomTimeUp(cr); }  /* BEQ not taken, JSL: CourseZoomTimeUp */
        cyc(42);
        return;
    } else { cyc(68); }
    r_sep(cr, P_M);
    TimeUpCursorStep(cr);  /* TimeUpCursorStep */
    cyc(42);
}

void abi_StuntCourseSelectStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    StuntCourseSelectStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 02:EA32 BattleCourseSelectStep
 * WHAT IT DOES: The Battle Trax course select's frame, once the fade is
 * full ($0D61 = $0F): the course's picture setting (Set0D37FromTable),
 * the flashing sprite, and Start or A zooms in (CourseZoomBattle),
 * otherwise the cursor (CourseCursorStep).
 * HOW: The pads' word 16-bit.
 * Entry: M 8-bit; data bank $7E or a mirror. Exit: M 16-bit; as the last
 * step leaves it.
 */
void BattleCourseSelectStep(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;
    uint8_t v;

    cyc(24);
    r_lda8(cr, b | W(fade_level_hi));
    v = (uint8_t)cr->a;
    set_p(cr, (uint8_t)((nz8(cr->p, (uint8_t)(v - 0x0F)) & ~P_C) | (v >= 0x0F ? P_C : 0)));
    if (v != 0x0F)
        { cyc(74); return; }
    (cyc(84), Set0D37FromTable_cr(cr));
    (cyc(46), FlashingMetaspriteDraw_cr(cr));
    cyc(24);
    if (sel_pressed16(cr, b, 0x9000)) {
        cyc(102);
        CourseZoomBattle(cr);
        cyc(42);
        return;
    } else { cyc(46); }
    CourseCursorStep(cr);
    cyc(42);
}

void abi_BattleCourseSelectStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    BattleCourseSelectStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 02:FC60 TimeUpSelectDraw
 * WHAT IT DOES: The time-up course select's flashing sprite position
 * $0D37: ($80, $60) with $0D4E set; else ($48, $B0) with the cursor $0D48
 * at 2 and ($B8, $B0) otherwise.
 * HOW: The bytes tested with an 8-bit X; a 16-bit store; P saved and
 * restored.
 * Entry: any widths; data bank $7E or a mirror. Exit: P restored; A = the
 * position word; X = the last byte tested.
 */
void TimeUpSelectDraw(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;
    uint8_t p0 = cr->p;
    uint16_t v;

    r_sep(cr, P_X);
    r_rep(cr, P_M);
    cyc(46);
    cr->x = rd8(b | W(hud_blank));
    if (cr->x) {
        v = 0x6080;
    } else {
        cyc(40);
        cr->x = rd8(b | W(wram_0D48));
        v = cr->x == 0x02 ? (cyc(38), 0xB048) : (cyc(32), 0xB0B8);
    }
    cr->a = v;
    cyc(70);
    wr16(b | W(wram_0D37), v);
    cyc(28);                            /* PLP; op_plp states the RTS */
    r_plp(cr, p0);
}

void abi_TimeUpSelectDraw(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    TimeUpSelectDraw(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* SEP #$20, LDA #$45, JSL SoundPort0QueuePush: the move sound. */
static void time_up_sound(Carried *cr)
{
    r_sep(cr, P_M);
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0x45);
    set_p(cr, nz8(cr->p, 0x45));
    (cyc(140), SoundPort0QueuePush_cr(cr));
}

/* LDA #v, STA addr (8-bit). */
static void time_up_set(Carried *cr, uint32_t at, uint8_t v)
{
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    set_p(cr, nz8(cr->p, v));
    cyc(40);
    wr8(at, v);
}


/*
 * @routine 02:EB6D TimeUpCursorStep
 * WHAT IT DOES: The time-up course select's cursor: with $0D4E set, Down
 * ($0400) clears it and puts the cursor $0D48 at 2; otherwise Up ($0800)
 * sets $0D4E to $FF; with the cursor at 2, Down puts it at 0 and Right
 * ($0100) at 3 ($0D63 cleared with Down); elsewhere Down puts it at 1
 * ($0D63 cleared) and Left ($0200) at 2. Each move with sound $45.
 * HOW: The pads' word 16-bit, the stores 8-bit.
 * Entry: M 8-bit; data bank $7E or a mirror. Exit: M 8-bit after a move,
 * else 16-bit; A as the last step leaves it.
 */
void TimeUpCursorStep(Carried *cr)
{
    time_up_cursor_body(cr);
    cyc(42);                            /* RTS */
}

void abi_TimeUpCursorStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    TimeUpCursorStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* The body of TimeUpCursorStep, up to its RTS. */
static void time_up_cursor_body(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;
    uint8_t v;

    cyc(24);
    r_lda8(cr, b | W(hud_blank));
    if ((uint8_t)cr->a) {
        cyc(40);
        if (sel_pressed16(cr, b, 0x0400)) {
            time_up_sound(cr);
            cyc(24);
            wr8(b | W(hud_blank), 0x00);
            time_up_set(cr, b | W(wram_0D48), 0x02);
        }
        return;
    } else { cyc(46); }
    if (sel_pressed16(cr, b, 0x0800)) {
        time_up_sound(cr);
        time_up_set(cr, b | W(hud_blank), 0xFF);
        return;
    }
    r_sep(cr, P_M);
    cyc(46);
    r_lda8(cr, b | W(wram_0D48));
    v = (uint8_t)cr->a;
    set_p(cr, (uint8_t)((nz8(cr->p, (uint8_t)(v - 0x02)) & ~P_C) | (v >= 0x02 ? (cyc(62), P_C) : (cyc(56), 0))));
    if (v != 0x02) {
        if (sel_pressed16(cr, b, 0x0400)) {
            time_up_sound(cr);
            cyc(24);
            wr8(b | W(wram_0D63), 0x00);
            time_up_set(cr, b | W(wram_0D48), 0x01);
            return;
        }
        cyc(24);
        r_lda16(cr, b | W(joy1_pressed_copy));
        set_p(cr, (uint8_t)((cr->p & ~(P_N | P_V | P_Z)) | ((cr->a >> 8) & 0xC0) | ((cr->a & 0x0200) ? 0 : (cyc(46), P_Z))));
        if (cr->a & 0x0200) {
            time_up_sound(cr);
            time_up_set(cr, b | W(wram_0D48), 0x02);
        }
        return;
    }
    if (sel_pressed16(cr, b, 0x0400)) {
        time_up_sound(cr);
        cyc(24);
        wr8(b | W(wram_0D63), 0x00);
        time_up_set(cr, b | W(wram_0D48), 0x00);
        return;
    }
    cyc(24);
    r_lda16(cr, b | W(joy1_pressed_copy));
    set_p(cr, (uint8_t)((cr->p & ~(P_N | P_V | P_Z)) | ((cr->a >> 8) & 0xC0) | ((cr->a & 0x0100) ? 0 : (cyc(46), P_Z))));
    if (cr->a & 0x0100) {
        time_up_sound(cr);
        time_up_set(cr, b | W(wram_0D48), 0x03);
    }
}

/*
 * @routine 03:FAFF CourseZoomTimeUp
 * WHAT IT DOES: As CourseZoomStunt for the time-up select: the course
 * $0E3D from the cursor $0D48 in the list at $03:FB86, then the zoom from
 * box ($0D48 - 2) * 8 + $80 to target 8.
 * HOW: As CourseZoomStunt (BRA to CourseZoomBattle's zoom).
 * Entry: any widths. Exit: as CourseZoomStunt.
 */
void CourseZoomTimeUp(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;
    uint16_t a;

    r_rep(cr, P_M | P_X);
    cyc(24);
    cr->x = rd16(b | W(wram_0D48));
    cyc(96);
    a = (uint16_t)(rd16(0x03FB86 + cr->x) & 0x00FF);
    cr->a = a;
    set_p(cr, nz16(cr->p, a));
    wr16(b | W(course), a);
    cyc(48);
    r_lda16(cr, b | W(wram_0D48));
    set_p(cr, (uint8_t)(cr->p | (P_C)));
    a = alu_sbc16(cr->a, 0x0002, &cr->p);
    cyc(50);
    a = asl3(cr, a);
    course_zoom(cr, box_offset(cr, a, 0x0080), 0x0008);
    cyc(94);
}

void abi_CourseZoomTimeUp(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CourseZoomTimeUp(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:FADB CourseZoomTimeUpNext
 * WHAT IT DOES: The time-up select's zoom with $0D4E set: the course
 * $0E3D stepped on within $17-$1A ($17 below or past them), then the zoom
 * from box $A0 to target 8.
 * HOW: 16-bit; a jump to CourseZoomBattle's zoom.
 * Entry: any widths. Exit: as CourseZoomStunt.
 */
void CourseZoomTimeUpNext(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;
    uint16_t a;

    r_rep(cr, P_M | P_X);
    cyc(24);
    r_lda16(cr, b | W(course));
    a = cr->a;
    cyc(24);
    r_cmp16(cr, 0x0017);
    if (a >= 0x0017) {
        cyc(16 + 14);                   /* BCC not taken, INC */
        a = (uint16_t)(a + 1);
        cr->a = a;
        set_p(cr, nz16(cr->p, a));
        cyc(24);
        r_cmp16(cr, 0x001B);
        if (a < 0x001B)
            { cyc(22); goto store; }    /* BCC taken */
        cyc(16);
    } else { cyc(22); }
    a = 0x0017;
    cr->a = a;
    set_p(cr, nz16(cr->p, a));
    cyc(24);                            /* LDA # */
store:
    cyc(24);
    wr16(b | W(course), a);
    cr->a = 0x0000;
    set_p(cr, nz16(cr->p, 0x0000));
    course_zoom(cr, box_offset_in(cr, 0x0000, 0x00A0, 264), 0x0008);
    cyc(94);
}

void abi_CourseZoomTimeUpNext(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CourseZoomTimeUpNext(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void abi_RecordsUnpackCourseTotals(void);
void abi_FlashingMetaspriteDraw0(void);
void abi_CourseZoomFromSelection(void);
void abi_CourseZoomFromSelection4(void);

/* 02:E60D's work: $0E91 = $30; with the records panel already up ($0D44)
   or Start or A pressed (sound $46), the course's totals unpacked
   (RecordsUnpackCourseTotals), $0D44 = $0D43 = 2 and $0D42 = $1E, and it
   leaves through its caller's frame (returns 1); otherwise the flashing
   sprite of set 0 (FlashingMetaspriteDraw0). */
static int records_key(Carried *cr, uint32_t b)
{
    r_sep(cr, P_M);
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0x30);
    set_p(cr, nz8(cr->p, 0x30));
    cyc(62);
    wr8(b | W(wram_0E91), 0x30);
    cyc(24);
    r_lda8(cr, b | W(wram_0D44));
    if (!(uint8_t)cr->a) {
        cyc(40);
        if (!sel_pressed(cr, b, 0x9000)) {
            cr->a = (uint16_t)(cr->a & 0xFF00);
            set_p(cr, nz8(cr->p, 0));
            cyc(84);
            wr8(b | W(metasprite_set_sel), 0x00);
            (cyc(46), FlashingMetaspriteDraw0_cr(cr));
            return 0;
        } else { cyc(56); }
        cr->a = (uint16_t)((cr->a & 0xFF00) | 0x46);
        set_p(cr, nz8(cr->p, 0x46));
        SoundPort0QueuePush_cr(cr);
        cyc(16);
    } else { cyc(38); }
    cyc(46);
    RecordsUnpackCourseTotals(cr);
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0x02);
    cyc(40);
    wr8(b | W(wram_0D44), 0x02);
    cyc(24);
    wr8(b | W(wram_0D43), 0x02);
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0x1E);
    set_p(cr, nz8(cr->p, 0x1E));
    cyc(40);
    wr8(b | W(wram_0D42), 0x1E);
    return 1;
}

/*
 * @routine 02:E60D CourseRecordsKey
 * WHAT IT DOES: The course select's records key: $0E91 = $30; with the
 * records panel already up ($0D44) or Start or A pressed (sound $46), the
 * course's totals unpacked (RecordsUnpackCourseTotals), $0D44 = $0D43 = 2,
 * $0D42 = $1E, and a return through its caller's frame (its own return
 * address pulled); otherwise the flashing sprite of set 0
 * (FlashingMetaspriteDraw0).
 * HOW: 8-bit; PLA, PLA, RTS to leave.
 * Entry: M 8-bit; data bank $7E or a mirror. Exit: as
 * FlashingMetaspriteDraw0; or at the caller's return with A low byte its
 * return address's high byte.
 */
void CourseRecordsKey(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;
    uint8_t v;

    if (!records_key(cr, b))
        { cyc(42); return; }
    cpu.s = (uint16_t)(cpu.s + 1);
    cpu.s = (uint16_t)(cpu.s + 1);
    v = rd8((uint16_t)cpu.s);
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    cr->p = nz8(cr->p, v);
    cpu_leave_to(0x020000 | (uint16_t)(rd16((uint16_t)(cpu.s + 1)) + 1));
    cpu.s = (uint16_t)(cpu.s + 2);
}

void abi_CourseRecordsKey(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CourseRecordsKey(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* C-to-C: CourseRecordsKey from a routine of this file, its leaving being
   that routine's own return; A as its PLAs leave it (the JSR's return
   address high byte, $E6). */
static int records_key_inner(Carried *cr, uint32_t b)
{
    if (!records_key(cr, b))
        { cyc(42); return 0; } else { cyc(98); }
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0xE6);
    set_p(cr, nz8(cr->p, 0xE6));
    return 1;
}

/* LDA #$80, STA $0D37, LDA #y, STA $0D38: the flashing sprite. */
static void records_sprite(Carried *cr, uint32_t b, uint8_t y)
{
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0x80);
    cyc(56);
    wr8(b | W(wram_0D37), 0x80);
    cr->a = (uint16_t)((cr->a & 0xFF00) | y);
    set_p(cr, nz8(cr->p, y));
    cyc(40);
    wr8(b | W(flash_sprite_y), y);
}

/*
 * @routine 02:E648 CourseRecordsSelectStep
 * WHAT IT DOES: The course select's frame where the records can be shown
 * (state 0 of $0D43): on page 0 ($0D63 = 0) the scroll $0E37 runs up to
 * $50 by 4, on page 1 down to 0; there, the records key
 * (CourseRecordsKey, which leaves when it opens them), the flashing
 * sprite at ($80, $B0) on the cursor's first row and ($80, $60) on the
 * other, and Up ($0800) and Down ($0400) move the cursor $0D4A (on page
 * 0's second row Up, with group 1 open, also pages down) with sound $45.
 * HOW: 8-bit; the pads' word 16-bit.
 * Entry: any widths; data bank $7E or a mirror. Exit: M and X 8-bit; as
 * the last step leaves it.
 */
void CourseRecordsSelectStep(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;
    uint8_t v;
    int open;

    r_rep(cr, P_M | P_X);
    r_sep(cr, P_M | P_X);
    cyc(24);
    r_lda8(cr, b | W(wram_0D63));
    if (!(uint8_t)cr->a) {
        cyc(40);
        r_lda8(cr, b | W(scroll_value));
        v = (uint8_t)cr->a;
        set_p(cr, (uint8_t)((nz8(cr->p, (uint8_t)(v - 0x50)) & ~P_C) | (v >= 0x50 ? (cyc(84), P_C) : 0)));
        if (v != 0x50) {
            v = (uint8_t)(v + 4);
            set_p(cr, (uint8_t)((nz8(cr->p, (uint8_t)(v - 0x50)) & ~P_C) | (v >= 0x50 ? (cyc(10), P_C) : 0)));
            if (v >= 0x50) {
                v = 0x50;
                set_p(cr, nz8(cr->p, v));
            }
            cr->a = (uint16_t)((cr->a & 0xFF00) | v);
            cyc(150);
            wr8(b | W(scroll_value), v);
            cyc(42);
            return;
        }
        if (records_key_inner(cr, b))
            return;
        cyc(24);
        r_lda8(cr, b | W(wram_0D4A));
        if (!(uint8_t)cr->a) {
            records_sprite(cr, b, 0xB0);
            cyc(24);
            if (sel_pressed(cr, b, 0x0800)) {
                sel_sound(cr);
                sel_row(cr, b, 0x01);
            } else { cyc(44); }
            cyc(42);
            return;
        } else { cyc(6); }
        records_sprite(cr, b, 0x60);
        r_sep(cr, P_M);
        cyc(22);
        open = sel_open(cr, 0x02);
        if (open && (cyc(2), sel_pressed(cr, b, 0x0800))) {
            sel_sound(cr);
            cyc(8);
            v = (uint8_t)(rd8(b | W(wram_0D63)) - 1);
            cyc(6);
            wr8(b | W(wram_0D63), v);
            cyc(16);
            sel_row(cr, b, 0x02);
            cyc(42);
            return;
        } else { cyc(open ? 68 : 62); }
        if (sel_pressed(cr, b, 0x0400)) {
            sel_sound(cr);
            cyc(8);
            wr8(b | W(wram_0D4A), 0x00);
        } else { cyc(44); }
        cyc(42);
        return;
    }
    cyc(46);
    r_lda8(cr, b | W(scroll_value));
    v = (uint8_t)cr->a;
    if (v) {
        v = (uint8_t)(v - 4);
        set_p(cr, nz8(cr->p, v));
        if (v & 0x80) {
            cyc(10);
            v = 0x00;
            set_p(cr, nz8(cr->p, v));
        }
        cr->a = (uint16_t)((cr->a & 0xFF00) | v);
        cyc(124);
        wr8(b | W(scroll_value), v);
        cyc(42);
        return;
    } else { cyc(86); }
    if (records_key_inner(cr, b))
        return;
    cyc(24);
    r_lda8(cr, b | W(wram_0D4A));
    v = (uint8_t)cr->a;
    set_p(cr, (uint8_t)((nz8(cr->p, (uint8_t)(v - 0x01)) & ~P_C) | (v >= 0x01 ? (cyc(16), P_C) : (cyc(22), 0))));
    if (v == 0x01) {
        records_sprite(cr, b, 0xB0);
        cyc(24);
        if (sel_pressed(cr, b, 0x0400)) {
            sel_sound(cr);
            cyc(8);
            wr8(b | W(wram_0D63), 0x00);
            cyc(24);
            wr8(b | W(wram_0D4A), 0x00);
            cyc(42);
            return;
        } else { cyc(68); }
        if (sel_pressed(cr, b, 0x0800)) {
            sel_sound(cr);
            sel_row(cr, b, 0x02);
        } else { cyc(44); }
        cyc(42);
        return;
    }
    records_sprite(cr, b, 0x60);
    cyc(24);
    if (sel_pressed(cr, b, 0x0400)) {
        sel_sound(cr);
        sel_row(cr, b, 0x01);
    } else { cyc(44); }
    cyc(42);
}

void abi_CourseRecordsSelectStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CourseRecordsSelectStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 02:E750 CourseRecordsOpening
 * WHAT IT DOES: The records panel opening (state 2 of $0D43): counts
 * $0D42 down; at 0, $0D42 = $2A, state 4 and, unless $0D4C is set, sound
 * $2D.
 * HOW: 8-bit through the data bank.
 * Entry: M 8-bit; data bank $7E or a mirror. Exit: A low byte as the last
 * load leaves it; N, Z from it.
 */
void CourseRecordsOpening(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;
    cyc(24);
    uint8_t v = (uint8_t)(rd8(b | W(wram_0D42)) - 1);

    cyc(6);
    wr8(b | W(wram_0D42), v);
    set_p(cr, nz8(cr->p, v));
    if (v)
        { cyc(64); return; }
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0x2A);
    cyc(56);
    wr8(b | W(wram_0D42), 0x2A);
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0x04);
    cyc(40);
    wr8(b | W(wram_0D43), 0x04);
    cyc(24);
    r_lda8(cr, b | W(digit62_override));
    if ((uint8_t)cr->a)
        { cyc(64); return; } else { cyc(94); }
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0x2D);
    set_p(cr, nz8(cr->p, 0x2D));
    SoundPort0QueuePush_cr(cr);
    cyc(42);
}

void abi_CourseRecordsOpening(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CourseRecordsOpening(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}


/*
 * @routine 02:E76B CourseRecordsReopen
 * WHAT IT DOES: State 8 of $0D43 (no run reaches it): counts $0D42 up; at
 * $2A, $0D44 = 2 and state 0.
 * HOW: 8-bit through the data bank.
 * Entry: M 8-bit; data bank $7E or a mirror. Exit: N, Z, C from the
 * compare with $2A, or A low byte 0.
 */
void CourseRecordsReopen(Carried *cr)
{
    records_reopen_body(cr);
    cyc(42);                            /* RTS */
}

void abi_CourseRecordsReopen(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CourseRecordsReopen(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* The body of CourseRecordsReopen, up to its RTS. */
static void records_reopen_body(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;
    cyc(24);
    uint8_t v = (uint8_t)(rd8(b | W(wram_0D42)) + 1);

    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    cyc(38);
    wr8(b | W(wram_0D42), v);
    set_p(cr, (uint8_t)((nz8(cr->p, (uint8_t)(v - 0x2A)) & ~P_C) | (v >= 0x2A ? P_C : (cyc(38), 0))));
    if (v < 0x2A)
        return;
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0x02);
    cyc(72);
    wr8(b | W(wram_0D44), 0x02);
    cr->a = (uint16_t)(cr->a & 0xFF00);
    set_p(cr, nz8(cr->p, 0));
    cyc(40);
    wr8(b | W(wram_0D43), 0x00);
}

/* 02:E7B4: the cursor's level column ($0F05 + $0D4A * 4 + $0D48) set by
   the count $0D42: 0 below $10, 1 to $17, 2 to $1F, 3 at $20-$21, 4
   from $22. */
static void records_pulse(Carried *cr, uint32_t b)
{
    uint8_t v;
    uint16_t x;

    cyc(46);
    r_lda8(cr, b | W(wram_0D4A));
    x = (uint16_t)(((uint8_t)cr->a << 2) & 0xFF);
    cyc(66);
    x = (uint8_t)(x + rd8(b | W(wram_0D48)));
    cr->x = x;
    cyc(38);
    r_lda8(cr, b | W(wram_0D42));
    v = (uint8_t)cr->a;
    set_p(cr, (uint8_t)((nz8(cr->p, (uint8_t)(v - 0x22)) & ~P_C) | (v >= 0x22 ? P_C : (cyc(54), 0))));
    if (v >= 0x22)
        v = 0x04;
    else {
        set_p(cr, (uint8_t)((nz8(cr->p, (uint8_t)(v - 0x20)) & ~P_C) | (v >= 0x20 ? P_C : (cyc(32), 0))));
        if (v >= 0x20)
            v = 0x03;
        else {
            set_p(cr, (uint8_t)((nz8(cr->p, (uint8_t)(v - 0x18)) & ~P_C) | (v >= 0x18 ? P_C : (cyc(26), 0))));
            if (v >= 0x18)
                v = 0x02;
            else {
                set_p(cr, (uint8_t)((nz8(cr->p, (uint8_t)(v - 0x10)) & ~P_C) | (v >= 0x10 ? (cyc(6), P_C) : 0)));
                v = v >= 0x10 ? 0x01 : 0x00;
            }
        }
    }
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    set_p(cr, nz8(cr->p, v));
    cyc(86);
    wr8(W(level_columns) + (uint32_t)x, v);
    cyc(42);
}

/*
 * @routine 02:E781 CourseRecordsClosing
 * WHAT IT DOES: The records panel closing (state 6 of $0D43): counts $0D42
 * up; at $2A, state 0 and $0D44 = 0; otherwise the cursor's level column
 * pulses with the count (as CourseRecordsShown's).
 * HOW: 8-bit through the data bank; the pulse is a branch into
 * CourseRecordsShown.
 * Entry: M 8-bit, X 8-bit; data bank $7E or a mirror. Exit: A low byte 0
 * at the end, else the column's level.
 */
void CourseRecordsClosing(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;
    cyc(24);
    uint8_t v = (uint8_t)(rd8(b | W(wram_0D42)) + 1);

    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    cyc(38);
    wr8(b | W(wram_0D42), v);
    set_p(cr, (uint8_t)((nz8(cr->p, (uint8_t)(v - 0x2A)) & ~P_C) | (v >= 0x2A ? P_C : (cyc(16), 0))));
    if (v < 0x2A) {
        records_pulse(cr, b);
        return;
    }
    cr->a = (uint16_t)(cr->a & 0xFF00);
    set_p(cr, nz8(cr->p, 0));
    cyc(72);
    wr8(b | W(wram_0D43), 0x00);
    cyc(24);
    wr8(b | W(wram_0D44), 0x00);
    cyc(42);
}

void abi_CourseRecordsClosing(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CourseRecordsClosing(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 02:E795 CourseRecordsShown
 * WHAT IT DOES: The records panel up (state 4 of $0D43): counts $0D42
 * down; at 0, $0D40 = 2 and the zoom into the course
 * (CourseZoomFromSelection, or CourseZoomFromSelection4 on page 1);
 * otherwise, unless $0D4C is set, the cursor's level column ($0F05 +
 * $0D4A * 4 + $0D48) pulses with the count: 0 below $10, 1 to $17, 2 to
 * $1F, 3 at $20-$21, 4 from $22.
 * HOW: 8-bit through the data bank; the column by a long store.
 * Entry: M 8-bit, X 8-bit; data bank $7E or a mirror. Exit: as the zoom
 * leaves it, or A low byte the column's level.
 */
void CourseRecordsShown(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;
    cyc(24);
    uint8_t v = (uint8_t)(rd8(b | W(wram_0D42)) - 1);

    cyc(6);
    wr8(b | W(wram_0D42), v);
    set_p(cr, nz8(cr->p, v));
    if (!v) {
        cr->a = (uint16_t)((cr->a & 0xFF00) | 0x02);
        set_p(cr, nz8(cr->p, 0x02));
        cyc(56);
        wr8(b | W(wram_0D40), 0x02);
        cyc(24);
        r_lda8(cr, b | W(wram_0D63));
        if ((uint8_t)cr->a)
            { cyc(84); CourseZoomFromSelection4(cr); }
        else
            { cyc(78); CourseZoomFromSelection(cr); }
        cyc(42);
        return;
    }
    cyc(46);
    r_lda8(cr, b | W(digit62_override));
    if ((uint8_t)cr->a)
        { cyc(58); return; }
    records_pulse(cr, b);
}

void abi_CourseRecordsShown(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CourseRecordsShown(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void abi_WindowEdgesStep(void);
void abi_ColourMathOffLong(void);
void abi_ColourMathA3Long(void);
void abi_SaveChecksum(void);
void abi_CourseRecordsSelectStep(void);
void abi_CourseRecordsOpening(void);
void abi_CourseRecordsShown(void);
void abi_CourseRecordsClosing(void);
void abi_CourseRecordsReopen(void);
void abi_SpeedCourseSelectStep(void);
void abi_StuntCourseSelectStep(void);
void abi_BattleCourseSelectStep(void);

/*
 * @routine 02:E3F1 CourseScreenWindowStep
 * WHAT IT DOES: The course screens' state 10: the window's edges
 * (WindowEdgesStep).
 * HOW: JSL, RTS.
 * Entry: as WindowEdgesStep. Exit: as WindowEdgesStep.
 */
void CourseScreenWindowStep(Carried *cr)
{
    (cyc(62), WindowEdgesStep_cr(cr));
    cyc(42);
}

void abi_CourseScreenWindowStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CourseScreenWindowStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}


/*
 * @routine 02:E414 CourseScreenWipeOut
 * WHAT IT DOES: State 14 (Stunt Trax and the all-courses select): $0D45 =
 * 6, the scroll HDMA's first count ($10AD) down by 4 and the next two
 * ($10B0, $10B3) up by 4; once the first would pass 0 it is 1, $10B3 goes
 * up by 2 and state $10 follows.
 * HOW: 8-bit through the data bank; the adds chain the carry.
 * Entry: any widths; data bank $7E or a mirror. Exit: M 8-bit; A as the
 * last store leaves it.
 */
void CourseScreenWipeOut(Carried *cr)
{
    wipe_out_body(cr);
    cyc(42);                            /* RTS */
}

void abi_CourseScreenWipeOut(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CourseScreenWipeOut(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* The body of CourseScreenWipeOut, up to its RTS. */
static void wipe_out_body(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;
    uint8_t v, p;

    r_sep(cr, P_M);
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0x06);
    set_p(cr, nz8(cr->p, 0x06));
    cyc(62);
    wr8(b | W(wram_0D45), 0x06);
    p = (uint8_t)(cr->p | P_C);
    cyc(24);
    v = alu_sbc8(rd8(b | W(wram_10AD)), 0x04, &p);
    set_p(cr, p);
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    if (cr->p & P_C) {
        cyc(54);
        wr8(b | W(wram_10AD), v);
        p = (uint8_t)(cr->p & ~P_C);
        cyc(24);
        v = alu_adc8(rd8(b | W(wram_10B0)), 0x04, &p);
        cyc(54);
        wr8(b | W(wram_10B0), v);
        cyc(24);
        v = alu_adc8(rd8(b | W(wram_10B3)), 0x04, &p);
        set_p(cr, p);
        cr->a = (uint16_t)((cr->a & 0xFF00) | v);
        cyc(40);
        wr8(b | W(wram_10B3), v);
        return;
    }
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0x01);
    cyc(76);
    wr8(b | W(wram_10AD), 0x01);
    cyc(24);
    v = (uint8_t)(rd8(b | W(wram_10B3)) + 1);
    cyc(6);
    wr8(b | W(wram_10B3), v);
    cyc(24);
    v = (uint8_t)(rd8(b | W(wram_10B3)) + 1);
    cyc(6);
    wr8(b | W(wram_10B3), v);
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0x10);
    set_p(cr, nz8(cr->p, 0x10));
    cyc(62);
    wr8(b | W(wram_0D40), 0x10);
}


/*
 * @routine 02:E44B CourseScreenWipeIn
 * WHAT IT DOES: State 18: $0D45 = 6, the scroll HDMA's first count ($10AD)
 * up by 4 and the next two ($10B0, $10B3) down by 4; at $4F the counts are
 * $4F, 1 and 1, state $14 and $0D45 = 2.
 * HOW: 8-bit through the data bank; the subtracts chain the borrow.
 * Entry: any widths; data bank $7E or a mirror. Exit: M 8-bit; A as the
 * last store leaves it.
 */
void CourseScreenWipeIn(Carried *cr)
{
    wipe_in_body(cr);
    cyc(42);                            /* RTS */
}

void abi_CourseScreenWipeIn(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CourseScreenWipeIn(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* The body of CourseScreenWipeIn, up to its RTS. */
static void wipe_in_body(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;
    uint8_t v, p;

    r_sep(cr, P_M);
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0x06);
    set_p(cr, nz8(cr->p, 0x06));
    cyc(62);
    wr8(b | W(wram_0D45), 0x06);
    p = (uint8_t)(cr->p & ~P_C);
    cyc(24);
    v = alu_adc8(rd8(b | W(wram_10AD)), 0x04, &p);
    set_p(cr, (uint8_t)((nz8(p, (uint8_t)(v - 0x4F)) & ~P_C) | (v >= 0x4F ? P_C : 0)));
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    if (v < 0x4F) {
        cyc(86);
        wr8(b | W(wram_10AD), v);
        p = (uint8_t)(cr->p | P_C);
        cyc(24);
        v = alu_sbc8(rd8(b | W(wram_10B0)), 0x04, &p);
        cyc(38);
        wr8(b | W(wram_10B0), v);
        cyc(24);
        v = alu_sbc8(rd8(b | W(wram_10B3)), 0x04, &p);
        set_p(cr, p);
        cr->a = (uint16_t)((cr->a & 0xFF00) | v);
        cyc(24);
        wr8(b | W(wram_10B3), v);
        return;
    }
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0x4F);
    cyc(130);
    wr8(b | W(wram_10AD), 0x4F);
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0x01);
    cyc(40);
    wr8(b | W(wram_10B0), 0x01);
    cyc(24);
    wr8(b | W(wram_10B3), 0x01);
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0x14);
    cyc(40);
    wr8(b | W(wram_0D40), 0x14);
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0x02);
    set_p(cr, nz8(cr->p, 0x02));
    cyc(40);
    wr8(b | W(wram_0D45), 0x02);
}

/* 02:E4BD / 02:E51E: a pressed button leaves for state $12 (sound $2F);
   otherwise, until $0FAA reaches 3, the four preview rows
   (PreviewRowDrawStunt, or PreviewRowDrawAll with `all`, at $0FA8 = 0-3),
   then $0FA6 counts to `last` and $0FAA steps. */
static void preview_rows(Carried *cr, int all, uint16_t last)
{
    uint32_t b = (uint32_t)cr->db << 16;
    uint16_t v;
    int first;

    r_rep(cr, P_M | P_X);
    cyc(24);
    r_lda16(cr, b | W(joy1_pressed_copy));
    if (cr->a) {
        r_sep(cr, P_M);
        cr->a = (uint16_t)((cr->a & 0xFF00) | 0x12);
        cyc(84);
        wr8(b | W(wram_0D40), 0x12);
        cr->a = (uint16_t)((cr->a & 0xFF00) | 0x2F);
        set_p(cr, nz8(cr->p, 0x2F));
        (cyc(78), SoundPort0QueuePush_cr(cr));
        return;
    }
    cyc(40);
    r_lda16(cr, b | W(records_place));
    r_cmp16(cr, 0x0003);
    if (cr->a >= 0x0003)
        { cyc(40); return; }
    cyc(70);
    wr16(b | W(records_row), 0x0000);
    for (first = 1;; first = 0) {
        cyc(first ? 62 : 84);               /* JSL; after the first, BRA back too */
        if (all)
            PreviewRowDrawAll_cr(cr);
        else
            PreviewRowDrawStunt_cr(cr);
        cyc(24);   /* PreviewRowDraw* state their tail */
        r_lda16(cr, b | W(records_row));
        v = (uint16_t)(cr->a + 1);
        cr->a = v;
        set_p(cr, nz16(cr->p, v));
        r_cmp16(cr, 0x0004);
        if (v >= 0x0004)
            break;
        cyc(78);
        wr16(b | W(records_row), v);
    }
    cyc(84);
    r_lda16(cr, b | W(records_char));
    r_cmp16(cr, last);
    if (cr->a < last) {
        cyc(64);
        v = (uint16_t)(rd16(b | W(records_char)) + 1);
        cyc(14);
        wr16(b | W(records_char), v);
        set_p(cr, nz16(cr->p, v));
        return;
    }
    cyc(70);
    wr16(b | W(records_char), 0x0000);
    cyc(24);
    v = (uint16_t)(rd16(b | W(records_place)) + 1);
    cyc(14);
    wr16(b | W(records_place), v);
    set_p(cr, nz16(cr->p, v));
}

/*
 * @routine 02:E4BD CoursePreviewRowsStunt
 * WHAT IT DOES: Stunt Trax's state 16: any button pressed ($0D1A) leaves
 * for state $12 with sound $2F; otherwise, until $0FAA reaches 3, the
 * four preview rows (PreviewRowDrawStunt with $0FA8 = 0-3), then $0FA6
 * counts to 6 and $0FAA steps.
 * HOW: 16-bit through the data bank.
 * Entry: any widths; data bank $7E or a mirror. Exit: M 16-bit, X 16-bit
 * or M 8-bit after the sound; A as the last step leaves it.
 */
void CoursePreviewRowsStunt(Carried *cr)
{
    preview_rows(cr, 0, 0x0006);
    cyc(42);
}

void abi_CoursePreviewRowsStunt(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CoursePreviewRowsStunt(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 02:E51E CoursePreviewRowsAll
 * WHAT IT DOES: The all-courses select's state 16: as
 * CoursePreviewRowsStunt with PreviewRowDrawAll and $0FA6 counting to 7.
 * HOW: As CoursePreviewRowsStunt.
 * Entry: as CoursePreviewRowsStunt. Exit: as CoursePreviewRowsStunt.
 */
void CoursePreviewRowsAll(Carried *cr)
{
    preview_rows(cr, 1, 0x0007);
    cyc(42);
}

void abi_CoursePreviewRowsAll(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CoursePreviewRowsAll(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 02:E503 CoursePreviewReset
 * WHAT IT DOES: The preview's counts $0FAA and $0FA6 cleared and $800
 * bytes copied from $7F:C3D7 to $7F:9BD7.
 * HOW: MVN, with P and the data bank kept (MVN leaves the data bank $7F).
 * Entry: any widths. Exit: P and the data bank unchanged; A = $FFFF, X =
 * $CBD7, Y = $A3D7.
 */
void CoursePreviewReset(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;
    uint8_t p0, db0;
    unsigned i;

    r_rep(cr, P_M);
    cyc(24);
    wr16(b | W(records_place), 0x0000);
    cyc(24);
    wr16(b | W(records_char), 0x0000);
    db0 = cr->db;
    p0 = cr->p;
    r_rep(cr, P_M | P_X);
    cyc(104);
    for (i = 0; i < 0x0800; i++)
        { cyc(36); wr8(W(tilemap_7f9bd7) + i, rd8(W(wram_7FC3D7) + i)); }
    cyc(68);
    cr->a = 0xFFFF;
    cr->x = 0xCBD7;
    cr->y = 0xA3D7;
    r_plp(cr, p0);
    cr->db = db0;
    set_p(cr, nz8(cr->p, db0));
}

void abi_CoursePreviewReset(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CoursePreviewReset(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* LDA #2, STA $0D40: the select's state 2. */
static void screen_state2(Carried *cr, uint32_t b)
{
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0x02);
    set_p(cr, nz8(cr->p, 0x02));
    cyc(40);
    wr8(b | W(wram_0D40), 0x02);
}

/*
 * @routine 02:E564 CourseScreenStartSpeed
 * WHAT IT DOES: The Speed Trax course select's state 0: state 2 and $10D8
 * cleared.
 * HOW: 8-bit stores through the data bank.
 * Entry: M 8-bit; data bank $7E or a mirror. Exit: A low byte 2.
 */
void CourseScreenStartSpeed(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;

    screen_state2(cr, b);
    cyc(24);
    wr8(b | W(wram_10D8), 0x00);
    cyc(42);
}

void abi_CourseScreenStartSpeed(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CourseScreenStartSpeed(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 02:E56D CourseScreenStartAll
 * WHAT IT DOES: The all-courses select's state 0: state 2.
 * HOW: An 8-bit store through the data bank.
 * Entry: M 8-bit; data bank $7E or a mirror. Exit: A low byte 2.
 */
void CourseScreenStartAll(Carried *cr)
{
    screen_state2(cr, (uint32_t)cr->db << 16);
    cyc(42);
}

void abi_CourseScreenStartAll(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CourseScreenStartAll(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 02:E573 CourseScreenStartBattle
 * WHAT IT DOES: The Battle Trax course select's state 0: state 2.
 * HOW: As CourseScreenStartAll.
 * Entry: as CourseScreenStartAll. Exit: as CourseScreenStartAll.
 */
void CourseScreenStartBattle(Carried *cr)
{
    screen_state2(cr, (uint32_t)cr->db << 16);
    cyc(42);
}

void abi_CourseScreenStartBattle(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CourseScreenStartBattle(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 02:E579 CourseScreenStartStunt
 * WHAT IT DOES: The Stunt Trax course select's state 0: state 2.
 * HOW: As CourseScreenStartAll.
 * Entry: as CourseScreenStartAll. Exit: as CourseScreenStartAll.
 */
void CourseScreenStartStunt(Carried *cr)
{
    screen_state2(cr, (uint32_t)cr->db << 16);
    cyc(42);
}

void abi_CourseScreenStartStunt(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CourseScreenStartStunt(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 02:E57F CourseScreenNothing
 * WHAT IT DOES: The Speed Trax course select's state 12, which nothing
 * reaches: returns.
 * HOW: RTS.
 * Entry: any. Exit: all unchanged.
 */
/* CourseScreenNothing on carried registers: no register work. */
void CourseScreenNothing_cr(Carried *cr)
{
    (void)cr;
    cyc(42);
}

void abi_CourseScreenNothing(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CourseScreenNothing_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}


/*
 * @routine 02:E580 CourseScreenZoomOut
 * WHAT IT DOES: State 12 (Stunt Trax and the all-courses select): the
 * window's scaled words $7F:3434 up by $300 and $7F:3438 down by $300;
 * once they are less than $700 apart, state $0E, colour maths off
 * (ColourMathOffLong) and the preview reset (CoursePreviewReset).
 * HOW: 16-bit long accesses.
 * Entry: any widths. Exit: M 16-bit, X 16-bit, or M 8-bit at the end; as
 * the last step leaves it.
 */
void CourseScreenZoomOut(Carried *cr)
{
    zoom_out_body(cr);
    cyc(42);                            /* RTS */
}

void abi_CourseScreenZoomOut(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CourseScreenZoomOut(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* The body of CourseScreenZoomOut, up to its RTS. */
static void zoom_out_body(Carried *cr)
{
    uint16_t v, w;
    uint8_t p;

    r_rep(cr, P_M | P_X);
    p = (uint8_t)(cr->p & ~P_C);
    cyc(32);
    v = alu_adc16(rd16(W(wram_7F3434)), 0x0300, &p);
    cyc(70);
    wr16(W(wram_7F3434), v);
    p = (uint8_t)(p | P_C);
    cyc(32);
    w = alu_sbc16(rd16(W(wram_7F3438)), 0x0300, &p);
    cyc(64);
    wr16(W(wram_7F3438), w);
    p = (uint8_t)(p | P_C);
    cyc(46);
    w = alu_sbc16(w, rd16(W(wram_7F3434)), &p);
    set_p(cr, (uint8_t)((nz16(p, (uint16_t)(w - 0x0700)) & ~P_C) | (w >= 0x0700 ? (cyc(34), P_C) : 0)));
    cr->a = w;
    if (w >= 0x0700)
        return;
    r_sep(cr, P_M);
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0x0E);
    set_p(cr, nz8(cr->p, 0x0E));
    cyc(102);
    wr8(((uint32_t)cr->db << 16) | 0x0D40, 0x0E);
    (cyc(62), ColourMathOffLong_cr(cr));
    (cyc(46), CoursePreviewReset(cr));
}


/*
 * @routine 02:E5B4 CourseScreenZoomIn
 * WHAT IT DOES: State 20: colour maths $A3 (ColourMathA3Long), the scaled
 * words $7F:3438 up by $300 and $7F:3434 down by $300; once $7F:3434 is
 * below its target $7F:343C, both are set to their targets ($7F:343C,
 * $7F:3440) and state 6 follows.
 * HOW: 16-bit long accesses.
 * Entry: any widths. Exit: M 16-bit, X 16-bit, or M 8-bit at the end; as
 * the last step leaves it.
 */
void CourseScreenZoomIn(Carried *cr)
{
    zoom_in_body(cr);
    cyc(42);                            /* RTS */
}

void abi_CourseScreenZoomIn(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CourseScreenZoomIn(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* The body of CourseScreenZoomIn, up to its RTS. */
static void zoom_in_body(Carried *cr)
{
    uint16_t v, t;
    uint8_t p;

    (cyc(62), ColourMathA3Long_cr(cr));
    r_rep(cr, P_M | P_X);
    p = (uint8_t)(cr->p & ~P_C);
    cyc(32);
    v = alu_adc16(rd16(W(wram_7F3438)), 0x0300, &p);
    cyc(70);
    wr16(W(wram_7F3438), v);
    p = (uint8_t)(p | P_C);
    cyc(32);
    v = alu_sbc16(rd16(W(wram_7F3434)), 0x0300, &p);
    cyc(64);
    wr16(W(wram_7F3434), v);
    cyc(32);
    t = rd16(W(wram_7F343C));
    set_p(cr, (uint8_t)((nz16(p, (uint16_t)(v - t)) & ~P_C) | (v >= t ? (cyc(22), P_C) : 0)));
    cr->a = v;
    if (v >= t)
        return;
    cyc(48);
    r_lda16(cr, W(wram_7F343C));
    cyc(32);
    wr16(W(wram_7F3434), cr->a);
    cyc(32);
    r_lda16(cr, W(wram_7F3440));
    cyc(32);
    wr16(W(wram_7F3438), cr->a);
    r_sep(cr, P_M);
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0x06);
    set_p(cr, nz8(cr->p, 0x06));
    cyc(62);
    wr8(((uint32_t)cr->db << 16) | 0x0D40, 0x06);
}

/*
 * @routine 02:E5F0 CourseRecordsDispatch
 * WHAT IT DOES: The Speed Trax course select's state 2: the level columns
 * drawn (TilemapLevelColumns); once the fade is full ($0D61 = $0F), the
 * records panel's state $0D43 from the table at $02:E603
 * (CourseRecordsSelectStep, CourseRecordsOpening, CourseRecordsShown,
 * CourseRecordsClosing, CourseRecordsReopen).
 * HOW: JMP (abs,X) with X 8-bit as the caller left it.
 * Entry: X 8-bit; data bank $7E or a mirror. Exit: as the state's step
 * leaves it.
 */
void CourseRecordsDispatch(Carried *cr)
{
    cyc(46);
    uint32_t b = (uint32_t)cr->db << 16;
    uint8_t v;

    TilemapLevelColumns_cr(cr);
    r_sep(cr, P_M);
    cyc(46);                           /* its end is TilemapLevelColumns' now */
    r_lda8(cr, b | W(fade_level_hi));
    v = (uint8_t)cr->a;
    set_p(cr, (uint8_t)((nz8(cr->p, (uint8_t)(v - 0x0F)) & ~P_C) | (v >= 0x0F ? P_C : (cyc(74), 0))));
    if (v != 0x0F)
        return;
    cyc(62);
    cr->x = rd8(b | W(wram_0D43));
    set_p(cr, nz8(cr->p, (uint8_t)cr->x));
    cyc(46);
    switch (rd16(0x02E603 + cr->x)) {
    case 0xE648: CourseRecordsSelectStep(cr); cyc(0); break;
    case 0xE750: CourseRecordsOpening(cr); break;
    case 0xE795: CourseRecordsShown(cr); cyc(0); break;
    case 0xE781: CourseRecordsClosing(cr); cyc(0); break;
    case 0xE76B: CourseRecordsReopen(cr); break;
    }
    cyc(0);
}

void abi_CourseRecordsDispatch(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CourseRecordsDispatch(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* JMP (table,X) with X = $0D40 (8-bit): the state's step; any other word
   (a state past the table's end) through jump_routine. */
static void screen_state(Carried *cr, uint32_t b, uint16_t table)
{
    cyc(62);
    cr->x = rd8(b | W(wram_0D40));
    set_p(cr, nz8(cr->p, (uint8_t)cr->x));
    cyc(46);
    switch (rd16(0x020000 | (uint16_t)(table + cr->x))) {
    case 0xE564: CourseScreenStartSpeed(cr); cyc(0); break;
    case 0xE56D: CourseScreenStartAll(cr); break;
    case 0xE573: CourseScreenStartBattle(cr); cyc(0); break;
    case 0xE579: CourseScreenStartStunt(cr); cyc(0); break;
    case 0xE5F0: CourseRecordsDispatch(cr); break;
    case 0xE7F5: SpeedCourseSelectStep(cr); break;
    case 0xE9AB: StuntCourseSelectStep(cr); cyc(0); break;
    case 0xEA32: BattleCourseSelectStep(cr); cyc(0); break;
    case 0xED3E: CourseZoomInStep(cr); cyc(0); break;
    case 0xE3F1: CourseScreenWindowStep(cr); cyc(0); break;
    case 0xE57F: w_CourseScreenNothing(cr); break;
    case 0xE580: CourseScreenZoomOut(cr); break;
    case 0xE414: CourseScreenWipeOut(cr); break;
    case 0xE4BD: CoursePreviewRowsStunt(cr); break;
    case 0xE51E: CoursePreviewRowsAll(cr); break;
    case 0xE44B: CourseScreenWipeIn(cr); break;
    case 0xE5B4: CourseScreenZoomIn(cr); break;
    case 0xED98: CourseScreenFadeBack(cr); break;
    case 0xEDD2: SpeedRecordsScreen(cr); cyc(0); break;
    case 0xED75: CourseScreenFadeLeave(cr); break;
    case 0xF148: StuntRecordsScreen(cr); break;
    case 0xF30A: BattleRecordsScreen(cr); break;
    case 0xF37A: AllRecordsScreen(cr); break;
    default: carried_jump(cr, 0x020000 | rd16(0x020000 | (uint16_t)(table + cr->x))); break;
    }
}

/*
 * @routine 02:E356 CourseScreenStep
 * WHAT IT DOES: The course screens' frame by HUD mode $0D2B, each running
 * its state $0D40 from its table: mode 0 Speed Trax ($02:E3E1; with all
 * courses opened, the all-courses table), 1 Stunt Trax ($02:E3FE), 2
 * Battle Trax ($02:E493), 3 the all-courses select ($02:E4A7). In mode 0
 * player 2 holding L ($0020) with nothing else and pressing Select seven
 * times opens every course: $0D24 = 3, the level columns cleared, sound
 * $36, the save's course bytes $0F and $0F and its progress 7
 * (SaveChecksum); another button clears the count ($0D25).
 * HOW: JMP (abs,X) by mode, then by state; 8-bit, the pads' words
 * 16-bit.
 * Entry: any widths; data bank $7E or a mirror. Exit: as the state's step
 * leaves it.
 */
void CourseScreenStep(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;
    uint8_t v;
    int i;

    r_sep(cr, P_M | P_X);
    cyc(24);
    r_lda8(cr, b | W(hud_mode));
    v = (uint8_t)(cr->a << 1);
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    cr->x = v;
    set_p(cr, nz8(cr->p, v));
    cyc(36);
    switch (rd16(0x02E360 + cr->x)) {
    case 0xE3F6:
        r_sep(cr, P_M | P_X);
        screen_state(cr, b, 0xE3FE);
        return;
    case 0xE48B:
        r_sep(cr, P_M | P_X);
        screen_state(cr, b, 0xE493);
        return;
    case 0xE49F:
        r_sep(cr, P_M | P_X);
        screen_state(cr, b, 0xE4A7);
        return;
    }
    r_sep(cr, P_M);
    cyc(84);
    r_lda8(cr, b | W(wram_0D24));
    if ((uint8_t)cr->a != 0x03) {
        r_rep(cr, P_M);
        cyc(56);
        r_lda16(cr, b | W(joy2_held));
        r_and16(cr, 0xDFD0);
        if (cr->a) {
            r_sep(cr, P_M);
            cyc(90);
            wr8(b | W(wram_0D24), 0x00);
            cyc(24);
            wr8(b | W(wram_0D25), 0x00);
            cyc(22);                    /* BRA */
        } else {
            cyc(62);
            r_lda16(cr, b | W(joy2_pressed_copy));
            set_p(cr, (uint8_t)((cr->p & ~(P_N | P_V | P_Z)) | ((cr->a >> 8) & 0xC0) | ((cr->a & 0x2000) ? 0 : (cyc(46), P_Z))));
            if (cr->a & 0x2000) {
                cyc(64);
                r_lda16(cr, b | W(joy2_held));
                set_p(cr, (uint8_t)((cr->p & ~(P_N | P_V | P_Z)) | ((cr->a >> 8) & 0xC0) | ((cr->a & 0x0020) ? 0 : P_Z)));
                if (!(cr->a & 0x0020)) {
                    r_sep(cr, P_M);
                    cyc(92);
                    wr8(b | W(wram_0D24), 0x00);
                    cyc(24);
                    wr8(b | W(wram_0D25), 0x00);
                    cyc(22);            /* BRA */
                } else {
                    r_sep(cr, P_M);
                    cyc(86);
                    v = (uint8_t)(rd8(b | W(wram_0D25)) + 1);
                    cyc(6);
                    wr8(b | W(wram_0D25), v);
                    cyc(24);
                    r_lda8(cr, b | W(wram_0D25));
                    v = (uint8_t)cr->a;
                    set_p(cr, (uint8_t)((nz8(cr->p, (uint8_t)(v - 0x07)) & ~P_C) | (v >= 0x07 ? P_C : (cyc(38), 0))));
                    if (v >= 0x07) {
                        cr->a = (uint16_t)((cr->a & 0xFF00) | 0x03);
                        cyc(72);
                        wr8(b | W(wram_0D24), 0x03);
                        cr->a = (uint16_t)(cr->a & 0xFF00);
                        cyc(18);
                        for (i = 0x0B; i >= 0; i--)
                            { cyc(68); wr8(W(level_columns) + (uint32_t)i, 0x00); }
                        cyc(108);
                        cr->x = 0xFF;
                        cr->a = (uint16_t)((cr->a & 0xFF00) | 0x36);
                        set_p(cr, nz8(cr->p, 0x36));
                        SoundPort0QueuePush_cr(cr);
                        cyc(16);
                        r_sep(cr, P_M | P_X);
                        cr->a = (uint16_t)((cr->a & 0xFF00) | 0x0F);
                        cyc(32);
                        wr8(0x70FC06, 0x0F);
                        cyc(32);
                        wr8(0x70FC07, 0x0F);
                        cr->a = (uint16_t)((cr->a & 0xFF00) | 0x07);
                        set_p(cr, nz8(cr->p, 0x07));
                        cyc(48);
                        wr8(0x70FC0B, 0x07);
                        (cyc(62), w_SaveChecksum(cr));
                    }
                    cyc(0);
                }
            }
        }
    } else { cyc(38); }
    r_sep(cr, P_M | P_X);
    cyc(24);
    r_lda8(cr, b | W(wram_0D24));
    v = (uint8_t)cr->a;
    set_p(cr, (uint8_t)((nz8(cr->p, (uint8_t)(v - 0x03)) & ~P_C) | (v >= 0x03 ? (cyc(18), P_C) : 0)));
    if (v == 0x03) {
        r_sep(cr, P_M | P_X);
        screen_state(cr, b, 0xE4A7);
        return;
    }
    screen_state(cr, b, 0xE3E1);
}

void abi_CourseScreenStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CourseScreenStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void abi_BlendStepDown(void);

/*
 * @routine 02:ED6D CourseScreenState8
 * WHAT IT DOES: The course screens' state 8 ($0D40).
 * HOW: An 8-bit store through the data bank.
 * Entry: any widths; data bank $7E or a mirror. Exit: M 8-bit; A low byte
 * 8.
 */
void CourseScreenState8(Carried *cr)
{
    r_sep(cr, P_M);
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0x08);
    set_p(cr, nz8(cr->p, 0x08));
    cyc(62);
    wr8(((uint32_t)cr->db << 16) | 0x0D40, 0x08);
    cyc(42);
}

void abi_CourseScreenState8(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CourseScreenState8(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* REP #$20, BlendStepDown, REP #$20, LDA.L $7F:3408: whether the blend is
   down to 0. */
static int fade_step(Carried *cr)
{
    r_rep(cr, P_M);
    (cyc(62), BlendStepDown_cr(cr));
    r_rep(cr, P_M);
    cyc(32);
    r_lda16(cr, W(fixed_colour_step));
    return cr->a == 0;
}

/* SEP #$20, $0D43 = panel, $0D42 = 0, $0D40 = state, colour maths off. */
static void fade_done(Carried *cr, uint32_t b, uint8_t panel, uint8_t state)
{
    r_sep(cr, P_M);
    cr->a = (uint16_t)((cr->a & 0xFF00) | panel);
    cyc(84);
    wr8(b | W(wram_0D43), panel);
    cyc(24);
    wr8(b | W(wram_0D42), 0x00);
    cr->a = (uint16_t)((cr->a & 0xFF00) | state);
    set_p(cr, nz8(cr->p, state));
    cyc(40);
    wr8(b | W(wram_0D40), state);
    (cyc(62), ColourMathOffLong_cr(cr));
}

/*
 * @routine 02:ED75 CourseScreenFadeLeave
 * WHAT IT DOES: The Speed Trax course select's state 14: the blend one
 * step down (BlendStepDown); at 0, the records panel's state 8 ($0D43,
 * $0D42 = 0), state 0 and colour maths off (ColourMathOffLong).
 * HOW: 16-bit for the blend, 8-bit stores.
 * Entry: any widths; data bank $7E or a mirror. Exit: M 16-bit with A the
 * blend factor; or M 8-bit as ColourMathOffLong leaves it.
 */
void CourseScreenFadeLeave(Carried *cr)
{
    if (fade_step(cr)) {
        fade_done(cr, (uint32_t)cr->db << 16, 0x08, 0x00);
        cyc(42);
    } else { cyc(58); }
}

void abi_CourseScreenFadeLeave(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CourseScreenFadeLeave(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}


/*
 * @routine 02:ED98 CourseScreenFadeBack
 * WHAT IT DOES: The course screens' state 8: the blend one step down
 * (BlendStepDown); at 0, the records panel's state 6 ($0D43, $0D42 = 0),
 * state 2, colour maths off (ColourMathOffLong) and, in HUD mode 0 with
 * the courses not all opened ($0D24 not 3) and $0D4C clear, sound $2D.
 * HOW: As CourseScreenFadeLeave.
 * Entry: as CourseScreenFadeLeave. Exit: as CourseScreenFadeLeave, or as
 * the sound's push leaves it.
 */
void CourseScreenFadeBack(Carried *cr)
{
    fade_back_body(cr);
    cyc(42);                            /* RTS */
}

void abi_CourseScreenFadeBack(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CourseScreenFadeBack(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* The body of CourseScreenFadeBack, up to its RTS. */
static void fade_back_body(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;

    if (!fade_step(cr))
        { cyc(16); return; }
    fade_done(cr, b, 0x06, 0x02);
    cyc(24);
    r_lda8(cr, b | W(hud_mode));
    if ((uint8_t)cr->a)
        { cyc(22); return; }
    cyc(40);
    r_lda8(cr, b | W(wram_0D24));
    if ((uint8_t)cr->a == 0x03) {
        cyc(38);
        set_p(cr, (uint8_t)((nz8(cr->p, 0) | P_C)));
        return;
    }
    set_p(cr, (uint8_t)((nz8(cr->p, (uint8_t)((uint8_t)cr->a - 0x03)) & ~P_C) | ((uint8_t)cr->a >= 0x03 ? P_C : 0)));
    cyc(56);
    r_lda8(cr, b | W(digit62_override));
    if ((uint8_t)cr->a)
        { cyc(22); return; } else { cyc(94); }
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0x2D);
    set_p(cr, nz8(cr->p, 0x2D));
    SoundPort0QueuePush_cr(cr);
}

/* CourseScreenNothing as its adapter leaves the registers. */
static void w_CourseScreenNothing(Carried *cr)
{
    cyc(42);
}

/* SaveChecksum as its adapter leaves the registers. */
static void w_SaveChecksum(Carried *cr)
{
    cr->a = (uint16_t)((cr->a & 0xFF00) | SaveChecksum());
}
