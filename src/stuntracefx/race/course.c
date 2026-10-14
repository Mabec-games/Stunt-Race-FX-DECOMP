/*
 * course.c: the course number $0E3D, from the menu's selections.
 */
#include "machine.h"
#include "cpu_ops.h"

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
        a = rd8(dbw(bank, 0x0D4A));
        b = 0x14;
    } else {
        a = rd8(dbw(bank, 0x0D4A));
        a = (uint8_t)(a << 2);
        b = rd8(dbw(bank, 0x0D48));
    }
    s = (unsigned)a + b;
    *p = (uint8_t)((nz8(*p, (uint8_t)s) & ~(P_C | P_V)) | (s > 0xFF ? P_C : 0)
                   | ((~(a ^ b) & (a ^ s) & 0x80) ? P_V : 0));
    wr8(dbw(bank, 0x0E3D), (uint8_t)s);
    return (uint8_t)s;
}

void abi_CourseFromSelection(void)
{
    uint8_t p = cpu.p;

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | CourseFromSelection(cpu.db, &p));
    cpu.p = p;
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

void abi_CourseFromSelectionLong(void)
{
    abi_CourseFromSelection();
}

void abi_SoundPort0QueuePush(void);
void abi_ColourMathA3Long(void);
void abi_BlendStepUp(void);
void abi_FixedColourFromStep(void);
void abi_RecordsLoadForCourse(void);

/* LDA $0D1A, BIT #mask (16-bit): whether a button in mask was pressed; Z
   from the BIT, N from the load. */
static int cursor_pressed(uint32_t b, uint16_t mask)
{
    op_lda16(b | 0x0D1A);
    cpu.p = (cpu.a & mask) ? (uint8_t)(cpu.p & ~P_Z) : (uint8_t)(cpu.p | P_Z);
    return (cpu.a & mask) != 0;
}

/* SEP #$20, sound $45, then the cursor $0D48 = to. */
static void cursor_to(uint32_t b, uint8_t to)
{
    op_sep(P_M);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x45);
    cpu.p = nz8(cpu.p, 0x45);
    abi_SoundPort0QueuePush();
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | to);
    cpu.p = nz8(cpu.p, to);
    wr8(b | 0x0D48, to);
}

/* Up to the fourth course when it is open (the low four bits of $70:FC06
   all set): sound $45 and $0D63 = $FF; 1 when it went there. */
static int cursor_up_open(uint32_t b)
{
    uint8_t v;

    op_sep(P_M);
    v = (uint8_t)(rd8(0x70FC06) & 0x0F);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = (uint8_t)((nz8(cpu.p, (uint8_t)(v - 0x0F)) & ~P_C) | (v >= 0x0F ? P_C : 0));
    if (v != 0x0F)
        return 0;
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x45);
    cpu.p = nz8(cpu.p, 0x45);
    abi_SoundPort0QueuePush();
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0xFF);
    cpu.p = nz8(cpu.p, 0xFF);
    wr8(b | 0x0D63, 0xFF);
    return 1;
}

/* REP #$20, then the first of two buttons pressed moves the cursor. */
static void cursor_two(uint32_t b, uint16_t m1, uint8_t to1, uint16_t m2, uint8_t to2)
{
    op_rep(P_M);
    if (cursor_pressed(b, m1))
        cursor_to(b, to1);
    else if (cursor_pressed(b, m2))
        cursor_to(b, to2);
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
void abi_CourseCursorAt0(void)
{
    cursor_two((uint32_t)cpu.db << 16, 0x0100, 0x01, 0x0800, 0x02);
}

/*
 * @routine 02:ECD9 CourseCursorAt1
 * WHAT IT DOES: The course cursor at 1 (bottom right): Left ($0200) moves
 * it to 0, Up ($0800) to 3.
 * HOW: As CourseCursorAt0.
 * Entry: any widths. Exit: as CourseCursorAt0.
 */
void abi_CourseCursorAt1(void)
{
    cursor_two((uint32_t)cpu.db << 16, 0x0200, 0x00, 0x0800, 0x03);
}

/*
 * @routine 02:ECFA CourseCursorAt2
 * WHAT IT DOES: The course cursor at 2 (top left): Right ($0100) moves it
 * to 3, Down ($0400) to 0.
 * HOW: As CourseCursorAt0.
 * Entry: any widths. Exit: as CourseCursorAt0.
 */
void abi_CourseCursorAt2(void)
{
    cursor_two((uint32_t)cpu.db << 16, 0x0100, 0x03, 0x0400, 0x00);
}

/*
 * @routine 02:ED1B CourseCursorAt3
 * WHAT IT DOES: The course cursor at 3 (top right): Left ($0200) moves it
 * to 2, Down ($0400) to 1.
 * HOW: As CourseCursorAt0. The move to 1 is not reached in the traces so
 * far.
 * Entry: any widths. Exit: as CourseCursorAt0.
 */
void abi_CourseCursorAt3(void)
{
    cursor_two((uint32_t)cpu.db << 16, 0x0200, 0x02, 0x0400, 0x01);
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
void abi_CourseCursorAt2Open(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;

    op_rep(P_M);
    if (cursor_pressed(b, 0x0800) && cursor_up_open(b))
        return;
    cursor_two(b, 0x0100, 0x03, 0x0400, 0x00);
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
void abi_CourseCursorAt3Open(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;

    op_rep(P_M);
    if (cursor_pressed(b, 0x0800) && cursor_up_open(b))
        return;
    cursor_two(b, 0x0200, 0x02, 0x0400, 0x01);
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
void abi_CourseCursorStep(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;

    if (cpu.p & P_M) {
        uint8_t v = rd8(b | 0x0D48);

        cpu.p = (uint8_t)((cpu.p & ~P_C) | (v >> 7));
        v = (uint8_t)(v << 1);
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    } else {
        uint16_t v = rd16(b | 0x0D48);

        cpu.p = (uint8_t)((cpu.p & ~P_C) | (v >> 15));
        cpu.a = (uint16_t)(v << 1);
    }
    cpu.x = (cpu.p & P_X) ? (uint8_t)cpu.a : cpu.a;
    cpu.p = (cpu.p & P_X) ? nz8(cpu.p, (uint8_t)cpu.x) : nz16(cpu.p, cpu.x);
    call_routine(0x020000 | rd16(0x020000 | (uint16_t)(0xEC28 + cpu.x)), 0);
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
void abi_CourseZoomInStep(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint8_t a8, m8;

    abi_ColourMathA3Long();
    op_rep(P_M | P_X);
    abi_BlendStepUp();
    abi_FixedColourFromStep();
    op_lda8(0x7F3408);
    a8 = (uint8_t)cpu.a;
    m8 = rd8(0x7F340A);
    cpu.p = (uint8_t)((nz8(cpu.p, (uint8_t)(a8 - m8)) & ~P_C) | (a8 >= m8 ? P_C : 0));
    if (a8 != m8)
        return;
    op_sep(P_M);
    wr8(b | 0x0D50, 0x00);
    abi_RecordsLoadForCourse();
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x20);
    cpu.p = nz8(cpu.p, 0x20);
    wr8(b | 0x0D41, 0x20);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x06);
    cpu.p = nz8(cpu.p, 0x06);
    wr8(b | 0x0D40, 0x06);
    wr8(b | 0x10D8, 0x00);
}

/*
 * @routine 02:EC10 RaceFadeStageDispatch
 * WHAT IT DOES: Runs the handler for stage $0D48 from the table at
 * $02:EC18 ($ECB8, $ECD9, $EC30, $EC73).
 * HOW: ASL, TAX, JMP (table,X) with 8-bit registers; the handler returns
 * to the caller.
 * Entry: M 8-bit, X 8-bit. Exit: as the handler leaves it.
 */
void abi_RaceFadeStageDispatch(void)
{
    uint8_t v;

    op_lda8((uint32_t)cpu.db << 16 | 0x0D48);
    v = (uint8_t)cpu.a;
    cpu.p = (uint8_t)((cpu.p & ~P_C) | (v >> 7));
    v = (uint8_t)(v << 1);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.x = v;
    cpu.p = nz8(cpu.p, v);
    call_routine(0x020000 | rd16(0x020000 | (uint16_t)(0xEC18 + v)), 0);
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
void abi_CourseWindowZoomSetup(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    unsigned i;

    for (i = 0; i < 4; i++) {
        cpu.a = rd16(b + 0xFB8E + cpu.y + 2 * i);
        wr16(0x7F343A + 2 * i, cpu.a);
    }
    for (i = 0; i < 4; i++) {
        cpu.a = rd16(b + 0xFC11 + cpu.x + 2 * i);
        wr16(0x7F3432 + 2 * i, cpu.a);
        wr16(0x7F3442 + 2 * i, cpu.a);
    }
    wr16(0x7F340A, 0x000F);
    cpu.a = 0x0000;
    cpu.p = nz16(cpu.p, 0x0000);
    wr16(0x7F3408, 0x0000);
    abi_WindowTable7FLong();
}

/* 03:FB64, the zoom's start from X and Y (the offsets for
   CourseWindowZoomSetup): with the data bank $03, the window set up,
   colour maths $A3, the large sprites hidden, $0D40 = 4 and sound $24. */
static void course_zoom(uint16_t x, uint16_t y)
{
    uint8_t db0 = cpu.db;

    cpu.x = x;
    cpu.y = y;
    op_sep(P_M);
    cpu.db = 0x03;
    cpu.p = nz8(cpu.p, 0x03);
    op_rep(P_M | P_X);
    abi_CourseWindowZoomSetup();
    abi_ColourMathA3();
    abi_OamBufferHideLargeLong();
    op_sep(P_M);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x04);
    wr8(0x030D40, 0x04);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x24);
    cpu.p = nz8(cpu.p, 0x24);
    abi_SoundPort0QueuePush();
    op_sep(P_M);
    cpu.db = db0;
    cpu.p = nz8(cpu.p, db0);
}

/* CLC, ADC #add, TAX: the start box's offset. */
static uint16_t box_offset(uint16_t a, uint16_t add)
{
    cpu.p &= (uint8_t)~P_C;
    cpu.a = op_adc16(a, add, &cpu.p);
    return cpu.a;
}

/* ASL A three times (16-bit): C the last bit out. */
static uint16_t asl3(uint16_t a)
{
    cpu.p = (uint8_t)((cpu.p & ~P_C) | (a & 0x2000 ? P_C : 0));
    a = (uint16_t)(a << 3);
    cpu.p = nz16(cpu.p, a);
    return a;
}

/* The course chosen from the cursor $0D48 in the four-entry list at
   `list` (a word load, the high byte dropped), then the zoom from box
   $0D48 * 8 + $80 to target y. */
static void course_choose(uint16_t list, uint16_t y)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint16_t a;

    op_rep(P_M | P_X);
    cpu.x = rd16(b | 0x0D48);
    a = (uint16_t)(rd16(0x030000 + list + cpu.x) & 0x00FF);
    cpu.a = a;
    cpu.p = nz16(cpu.p, a);
    wr16(b | 0x0E3D, a);
    a = asl3(rd16(b | 0x0D48));
    course_zoom(box_offset(a, 0x0080), y);
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
void abi_CourseZoomStunt(void)
{
    course_choose(0xFB86, 0x0008);
}

/*
 * @routine 03:FB47 CourseZoomBattle
 * WHAT IT DOES: As CourseZoomStunt with the list at $03:FB8A (courses
 * $10-$13) and target $10.
 * HOW: As CourseZoomStunt; the zoom's tail at $FB64 is also where the
 * other course choices go.
 * Entry: any widths. Exit: as CourseZoomStunt.
 */
void abi_CourseZoomBattle(void)
{
    course_choose(0xFB8A, 0x0010);
}

/* Courses below $0C: box course * 8 (+ $40 with a HUD mode, target $18;
   else target 0). */
static void course_zoom_race(uint16_t a)
{
    a = asl3(a);
    cpu.x = rd16(((uint32_t)cpu.db << 16) | 0x0D2B);
    cpu.p = nz16(cpu.p, cpu.x);
    if (cpu.x)
        course_zoom(box_offset(a, 0x0040), 0x0018);
    else
        course_zoom(box_offset(a, 0x0000), 0x0000);
}

/* 03:FA88 and 03:FA98: box $B0 or $A8, target $20. */
static void course_zoom_box(uint16_t a)
{
    op_rep(P_M | P_X);
    cpu.a = a;
    course_zoom(box_offset(a, 0x00A8), 0x0020);
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
void abi_CourseZoomFromSelection4(void)
{
    uint8_t v;
    uint16_t a;

    op_sep(P_M);
    abi_CourseFromSelectionLong();
    v = (uint8_t)cpu.a;
    cpu.p = (uint8_t)((nz8(cpu.p, (uint8_t)(v - 0x0C)) & ~P_C) | (v >= 0x0C ? P_C : 0));
    if (v < 0x0C) {
        op_rep(P_M | P_X);
        a = (uint16_t)(cpu.a & 0x00FF);
        cpu.p |= P_C;
        a = op_sbc16(a, 0x0004, &cpu.p);
        course_zoom_race(a);
    } else if (v == 0x15)
        course_zoom_box(0x0000);
    else
        course_zoom_box(0x0008);
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
void abi_CourseZoomFromSelection(void)
{
    uint8_t v;

    op_sep(P_M | P_X);
    abi_CourseFromSelectionLong();
    v = (uint8_t)cpu.a;
    cpu.p = (uint8_t)((nz8(cpu.p, (uint8_t)(v - 0x0C)) & ~P_C) | (v >= 0x0C ? P_C : 0));
    if (v < 0x0C) {
        op_rep(P_M | P_X);
        course_zoom_race((uint16_t)(cpu.a & 0x00FF));
    } else if (v == 0x15)
        course_zoom_box(0x0008);
    else
        course_zoom_box(0x0000);
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
void abi_LevelColumnsSet4(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    int i;

    for (i = 0; i < 4; i++) {
        if (cpu.p & P_M)
            wr8(b | (uint16_t)(0x0F05 + i + cpu.x), (uint8_t)cpu.a);
        else
            wr16(b | (uint16_t)(0x0F05 + i + cpu.x), cpu.a);
    }
}

/* LDA.L $70:FC0B, BIT #bit: whether that course group is open; N, V from
   the save byte, Z from the test. */
static int sel_open(uint8_t bit)
{
    uint8_t v = rd8(0x70FC0B);

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = (uint8_t)((cpu.p & ~(P_N | P_V | P_Z)) | (v & 0xC0) | ((v & bit) ? 0 : P_Z));
    return (v & bit) != 0;
}

/* REP #$20, LDA $0D1A, BIT #mask, SEP #$20: whether pressed. */
static int sel_pressed(uint32_t b, uint16_t mask)
{
    uint16_t v;

    op_rep(P_M);
    v = rd16(b | 0x0D1A);
    cpu.a = v;
    cpu.p = (uint8_t)((cpu.p & ~(P_N | P_V | P_Z)) | ((v >> 8) & 0xC0) | ((v & mask) ? 0 : P_Z));
    op_sep(P_M);
    return (v & mask) != 0;
}

/* REP #$20, LDA $0D1A, BIT #mask (left 16-bit): whether pressed. */
static int sel_pressed16(uint32_t b, uint16_t mask)
{
    op_rep(P_M);
    op_lda16(b | 0x0D1A);
    cpu.p = (uint8_t)((cpu.p & ~(P_N | P_V | P_Z)) | ((cpu.a >> 8) & 0xC0) | ((cpu.a & mask) ? 0 : P_Z));
    return (cpu.a & mask) != 0;
}

/* LDA #$45, JSL SoundPort0QueuePush: the move sound. */
static void sel_sound(void)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x45);
    cpu.p = nz8(cpu.p, 0x45);
    abi_SoundPort0QueuePush();
}

/* LDA #v, STA $0D4A: the cursor's row. */
static void sel_row(uint32_t b, uint8_t v)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, v);
    wr8(b | 0x0D4A, v);
}

/* The scroll $0E37 moved 4 toward $50 (in) or 0 (out), then $0E92 = $F0
   and the large sprites hidden. */
static void sel_scroll(uint32_t b, uint8_t v)
{
    wr8(b | 0x0E37, v);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0xF0);
    cpu.p = nz8(cpu.p, 0xF0);
    wr8(b | 0x0E92, 0xF0);
    abi_OamBufferHideLargeLong();
}

/* LDA #y, STA $0D38, JSR MenuCursorFourWay. */
static void sel_cursor(uint32_t b, uint8_t y)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | y);
    cpu.p = nz8(cpu.p, y);
    wr8(b | 0x0D38, y);
    abi_MenuCursorFourWay();
}

/* 02:E954: Start or A ($9000) zooms into the course (the page's own
   zoom); otherwise the four-way cursor with its sprite at y $B0 on the
   page's first row, $60 on the other. */
static void sel_choose(uint32_t b)
{
    uint8_t page = rd8(b | 0x0D63), v;

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | page);
    cpu.p = nz8(cpu.p, page);
    if (sel_pressed16(b, 0x9000)) {
        if (page)
            abi_CourseZoomFromSelection4();
        else
            abi_CourseZoomFromSelection();
        return;
    }
    op_sep(P_M);
    op_lda8(b | 0x0D4A);
    v = (uint8_t)cpu.a;
    if (page) {
        cpu.p = (uint8_t)((nz8(cpu.p, (uint8_t)(v - 0x01)) & ~P_C) | (v >= 0x01 ? P_C : 0));
        sel_cursor(b, v == 0x01 ? 0xB0 : 0x60);
    } else {
        sel_cursor(b, v ? 0x60 : 0xB0);
    }
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
void abi_SpeedCourseSelectStep(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    static const uint8_t bits[3] = {0x01, 0x02, 0x04};
    uint8_t v;
    int i, open;

    for (i = 0; i < 3; i++) {
        open = sel_open(bits[i]);
        v = open ? 0x00 : 0x04;
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
        cpu.p = nz8(cpu.p, v);
        cpu.x = (uint16_t)(i * 4);
        cpu.p = nz8(cpu.p, (uint8_t)cpu.x);
        abi_LevelColumnsSet4();
    }
    abi_TilemapLevelColumns();
    op_lda8(b | 0x0D61);
    v = (uint8_t)cpu.a;
    cpu.p = (uint8_t)((nz8(cpu.p, (uint8_t)(v - 0x0F)) & ~P_C) | (v >= 0x0F ? P_C : 0));
    if (v != 0x0F)
        return;
    op_sep(P_M);
    op_lda8(b | 0x0D63);
    if (!(uint8_t)cpu.a) {
        op_lda8(b | 0x0E37);
        v = (uint8_t)cpu.a;
        if (v != 0x50) {
            v = (uint8_t)(v + 4);
            cpu.p = (uint8_t)((cpu.p & ~P_C) | (v >= 0x50 ? P_C : 0));
            if (v >= 0x50)
                v = 0x50;
            sel_scroll(b, v);
            return;
        }
        abi_FlashingMetaspriteDrawByMode();
        op_lda8(b | 0x0D4A);
        if (!(uint8_t)cpu.a) {
            op_sep(P_M);
            if (sel_open(0x02) && sel_pressed(b, 0x0800)) {
                sel_sound();
                sel_row(b, 0x01);
                return;
            }
            sel_choose(b);
            return;
        }
        op_sep(P_M);
        if (sel_open(0x04) && sel_pressed(b, 0x0800)) {
            sel_sound();
            v = (uint8_t)(rd8(b | 0x0D63) - 1);
            wr8(b | 0x0D63, v);
            sel_row(b, 0x02);
            return;
        }
        op_sep(P_M);
        if (sel_open(0x01) && sel_pressed(b, 0x0400)) {
            sel_sound();
            wr8(b | 0x0D4A, 0x00);
            return;
        }
        sel_choose(b);
        return;
    }
    op_lda8(b | 0x0E37);
    v = (uint8_t)cpu.a;
    if (v) {
        v = (uint8_t)(v - 4);
        if (v & 0x80)
            v = 0x00;
        sel_scroll(b, v);
        return;
    }
    abi_FlashingMetaspriteDrawByMode();
    op_lda8(b | 0x0D4A);
    v = (uint8_t)cpu.a;
    cpu.p = (uint8_t)((nz8(cpu.p, (uint8_t)(v - 0x01)) & ~P_C) | (v >= 0x01 ? P_C : 0));
    if (v == 0x01) {
        if (sel_pressed(b, 0x0400)) {
            op_sep(P_M);
            if (sel_open(0x01)) {
                sel_sound();
                wr8(b | 0x0D63, 0x00);
                wr8(b | 0x0D4A, 0x00);
                return;
            }
        }
        if (sel_pressed(b, 0x0800)) {
            op_sep(P_M);
            if (sel_open(0x04)) {
                sel_sound();
                sel_row(b, 0x02);
                return;
            }
        }
        sel_choose(b);
        return;
    }
    if (sel_pressed(b, 0x0400)) {
        sel_sound();
        sel_row(b, 0x01);
        return;
    }
    sel_choose(b);
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
void abi_StuntCourseSelectStep(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint8_t v;

    op_lda8(b | 0x0D61);
    v = (uint8_t)cpu.a;
    cpu.p = (uint8_t)((nz8(cpu.p, (uint8_t)(v - 0x0F)) & ~P_C) | (v >= 0x0F ? P_C : 0));
    if (v != 0x0F)
        return;
    op_lda8(b | 0x0D63);
    if (!(uint8_t)cpu.a) {
        op_lda8(b | 0x0E37);
        v = (uint8_t)cpu.a;
        if (v != 0x50) {
            v = (uint8_t)(v + 4);
            cpu.p = (uint8_t)((cpu.p & ~P_C) | (v >= 0x50 ? P_C : 0));
            if (v >= 0x50)
                v = 0x50;
            sel_scroll(b, v);
            return;
        }
        abi_Set0D37FromTable();
        abi_FlashingMetaspriteDraw();
        if (sel_pressed16(b, 0x9000)) {
            abi_CourseZoomStunt();
            return;
        }
        op_sep(P_M);
        abi_RaceFadeStageDispatch();
        return;
    }
    op_lda8(b | 0x0E37);
    v = (uint8_t)cpu.a;
    if (v) {
        v = (uint8_t)(v - 4);
        if (v & 0x80)
            v = 0x00;
        sel_scroll(b, v);
        return;
    }
    call_routine(0x02FC60, 0);  /* TimeUpSelectDraw */
    abi_FlashingMetaspriteDraw();
    if (sel_pressed16(b, 0x9000)) {
        op_sep(P_M);
        op_lda8(b | 0x0D4E);
        if ((uint8_t)cpu.a)
            call_routine(0x03FADB, 1);  /* CourseZoomTimeUpNext */
        else
            call_routine(0x03FAFF, 1);  /* CourseZoomTimeUp */
        return;
    }
    op_sep(P_M);
    call_routine(0x02EB6D, 0);  /* TimeUpCursorStep */
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
void abi_BattleCourseSelectStep(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint8_t v;

    op_lda8(b | 0x0D61);
    v = (uint8_t)cpu.a;
    cpu.p = (uint8_t)((nz8(cpu.p, (uint8_t)(v - 0x0F)) & ~P_C) | (v >= 0x0F ? P_C : 0));
    if (v != 0x0F)
        return;
    abi_Set0D37FromTable();
    abi_FlashingMetaspriteDraw();
    if (sel_pressed16(b, 0x9000)) {
        abi_CourseZoomBattle();
        return;
    }
    abi_CourseCursorStep();
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
void abi_TimeUpSelectDraw(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint8_t p0 = cpu.p;
    uint16_t v;

    op_sep(P_X);
    op_rep(P_M);
    cpu.x = rd8(b | 0x0D4E);
    if (cpu.x) {
        v = 0x6080;
    } else {
        cpu.x = rd8(b | 0x0D48);
        v = cpu.x == 0x02 ? 0xB048 : 0xB0B8;
    }
    cpu.a = v;
    wr16(b | 0x0D37, v);
    op_plp(p0);
}

/* SEP #$20, LDA #$45, JSL SoundPort0QueuePush: the move sound. */
static void time_up_sound(void)
{
    op_sep(P_M);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x45);
    cpu.p = nz8(cpu.p, 0x45);
    abi_SoundPort0QueuePush();
}

/* LDA #v, STA addr (8-bit). */
static void time_up_set(uint32_t at, uint8_t v)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, v);
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
void abi_TimeUpCursorStep(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint8_t v;

    op_lda8(b | 0x0D4E);
    if ((uint8_t)cpu.a) {
        if (sel_pressed16(b, 0x0400)) {
            time_up_sound();
            wr8(b | 0x0D4E, 0x00);
            time_up_set(b | 0x0D48, 0x02);
        }
        return;
    }
    if (sel_pressed16(b, 0x0800)) {
        time_up_sound();
        time_up_set(b | 0x0D4E, 0xFF);
        return;
    }
    op_sep(P_M);
    op_lda8(b | 0x0D48);
    v = (uint8_t)cpu.a;
    cpu.p = (uint8_t)((nz8(cpu.p, (uint8_t)(v - 0x02)) & ~P_C) | (v >= 0x02 ? P_C : 0));
    if (v != 0x02) {
        if (sel_pressed16(b, 0x0400)) {
            time_up_sound();
            wr8(b | 0x0D63, 0x00);
            time_up_set(b | 0x0D48, 0x01);
            return;
        }
        op_lda16(b | 0x0D1A);
        cpu.p = (uint8_t)((cpu.p & ~(P_N | P_V | P_Z)) | ((cpu.a >> 8) & 0xC0) | ((cpu.a & 0x0200) ? 0 : P_Z));
        if (cpu.a & 0x0200) {
            time_up_sound();
            time_up_set(b | 0x0D48, 0x02);
        }
        return;
    }
    if (sel_pressed16(b, 0x0400)) {
        time_up_sound();
        wr8(b | 0x0D63, 0x00);
        time_up_set(b | 0x0D48, 0x00);
        return;
    }
    op_lda16(b | 0x0D1A);
    cpu.p = (uint8_t)((cpu.p & ~(P_N | P_V | P_Z)) | ((cpu.a >> 8) & 0xC0) | ((cpu.a & 0x0100) ? 0 : P_Z));
    if (cpu.a & 0x0100) {
        time_up_sound();
        time_up_set(b | 0x0D48, 0x03);
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
void abi_CourseZoomTimeUp(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint16_t a;

    op_rep(P_M | P_X);
    cpu.x = rd16(b | 0x0D48);
    a = (uint16_t)(rd16(0x03FB86 + cpu.x) & 0x00FF);
    cpu.a = a;
    cpu.p = nz16(cpu.p, a);
    wr16(b | 0x0E3D, a);
    op_lda16(b | 0x0D48);
    cpu.p |= P_C;
    a = op_sbc16(cpu.a, 0x0002, &cpu.p);
    a = asl3(a);
    course_zoom(box_offset(a, 0x0080), 0x0008);
}

/*
 * @routine 03:FADB CourseZoomTimeUpNext
 * WHAT IT DOES: The time-up select's zoom with $0D4E set: the course
 * $0E3D stepped on within $17-$1A ($17 below or past them), then the zoom
 * from box $A0 to target 8.
 * HOW: 16-bit; a jump to CourseZoomBattle's zoom.
 * Entry: any widths. Exit: as CourseZoomStunt.
 */
void abi_CourseZoomTimeUpNext(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint16_t a;

    op_rep(P_M | P_X);
    op_lda16(b | 0x0E3D);
    a = cpu.a;
    op_cmp16(0x0017);
    if (a >= 0x0017) {
        a = (uint16_t)(a + 1);
        cpu.a = a;
        cpu.p = nz16(cpu.p, a);
        op_cmp16(0x001B);
        if (a < 0x001B)
            goto store;
    }
    a = 0x0017;
    op_lda16_imm(a);
store:
    wr16(b | 0x0E3D, a);
    op_lda16_imm(0x0000);
    course_zoom(box_offset(0x0000, 0x00A0), 0x0008);
}
