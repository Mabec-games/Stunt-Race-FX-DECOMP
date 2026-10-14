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
