/*
 * results_screen.c: the screens after a race that OverlayFramesUntilFaded
 * runs a frame at a time (ResultsScreenStep): by HUD mode the step $0D59
 * picks a handler that draws a panel with the Super FX (a picture drawn to
 * $70:3000 and copied into the tilemap buffer at $7F:93D7), shows the
 * records of a special course (scores or times, the cars as three-piece
 * sprites), waits for B or START, or runs the menu after the race (retry,
 * course, car: $0DD4) and the car choice.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "carried.h"

uint16_t DigitTiles1184(void);
void OamTextLine(uint8_t bank, uint16_t *xp, uint16_t *yp, uint16_t *ap);
uint16_t SelectionSave(uint8_t bank, uint16_t *x);
uint8_t SpritePieces3(uint8_t bank, uint16_t *x, uint16_t *y, uint8_t *p, uint16_t *set);
void TilemapPaletteFill(uint16_t *x, uint16_t *y, uint16_t *a, uint8_t entry);
#include "hud_calls.h"

void abi_SoundPort0QueuePush(void);
void abi_SoundPort0QueuePushX8(void);
void abi_TilemapPaletteCycle(void);
void abi_SpritePieces3(void);
void abi_RecordsUnpackCourseRunning(void);
void abi_GsuCF42To3000(void);
void abi_GsuCF42To4000(void);
void abi_DigitTiles1184(void);
void abi_TimeUnpackBcd(void);
void abi_TilemapRow187Palette4(void);
void abi_TilemapRow207Palette4(void);
void abi_TilemapRow287Palette4(void);
void abi_SelectionSave(void);
void abi_ResultsTotalTiles(void);
void abi_RecordTimeTiles(void);

#define TILES 0x7F93D7                  /* the tilemap buffer */

void KeepTimeDigits(Carried *cr);
void LapCharsLap1(Carried *cr);
void LapCharsLap2(Carried *cr);
void LapCharsLap3(Carried *cr);
void LapCharsTotal(Carried *cr);
void NameTiles16(Carried *cr);
void NameTiles26(Carried *cr);
void PlaceFrameFromValue(Carried *cr);
void RaceTimeTiles(Carried *cr);
void RecordTimeTiles(Carried *cr);
void ResultsBlinkWait(Carried *cr);
void ResultsBonusCount(Carried *cr);
void ResultsCarPicture(Carried *cr);
void ResultsChampionScreen(Carried *cr);
void ResultsMenu(Carried *cr);
void ResultsNext(Carried *cr);
void ResultsPanelF38E(Carried *cr);
void ResultsPanelFA39(Carried *cr);
void ResultsPanelFAB5(Carried *cr);
void ResultsPanelFB99(Carried *cr);
void ResultsPicture4000(Carried *cr);
void ResultsRecordCompare(Carried *cr);
void ResultsTotalLine(Carried *cr);
void ResultsTotalTiles(Carried *cr);
void ResultsUnpackNext(Carried *cr);
void ResultsWaitWindow(Carried *cr);
void ScoreTiles(Carried *cr);
void StuntRecordsChoice(Carried *cr);
void StuntRecordsDraw(Carried *cr);
void StuntRecordsWait(Carried *cr);
void StuntResultsCount(Carried *cr);
void TimeTiles(Carried *cr);
void TimedRecordsChoice(Carried *cr);
void TimedRecordsDraw(Carried *cr);
void TimedRecordsWait(Carried *cr);
static void asl8(Carried *cr);
static uint8_t bcd_add(Carried *cr, uint32_t b, uint16_t at, uint8_t m, int clc);
static void bcd_add_mem(Carried *cr, uint32_t b, uint16_t at, uint16_t from, int clc);
static uint8_t bcd_sub(Carried *cr, uint32_t b, uint16_t at, uint8_t m, int sec);
static void bcd_tiles(Carried *cr, uint32_t at, uint32_t hi, uint32_t lo);
static void bonus_move(Carried *cr, uint32_t b, uint8_t lo, uint8_t mid);
static void car_change(Carried *cr, uint32_t b, int right);
static void car_picture(Carried *cr, uint32_t b);
static uint8_t car_times(Carried *cr, uint32_t b);
static void cmp8_imm(Carried *cr, uint8_t m);
static void count_digits(Carried *cr, uint32_t b, uint16_t at, uint32_t hi, uint32_t lo);
static uint8_t high_digit_tile(Carried *cr, uint8_t v);
static void inc8_mem(Carried *cr, uint32_t addr);
static void lap_chars(Carried *cr, uint16_t y);
static void lda8_imm(Carried *cr, uint8_t v);
static uint8_t low_digit_tile(Carried *cr, uint8_t v);
static void menu_rows(Carried *cr, uint16_t x, uint16_t y, void (*a)(Carried *), void (*c)(Carried *));
static void minutes_tile(Carried *cr, uint8_t mins, uint32_t at);
static void name_tiles(Carried *cr, uint16_t at);
static void palette_cycle(Carried *cr, uint16_t x, uint16_t y);
static void palette_fill(Carried *cr, uint16_t x, uint16_t y, uint8_t f, unsigned t);
static void panel(Carried *cr, uint16_t x, uint16_t pic);
static int pressed(Carried *cr, uint32_t b, uint16_t mask);
static void record_cars(Carried *cr, uint32_t b, uint16_t empty);
static void records_choice(Carried *cr, uint32_t b, uint8_t skip, int check_10cd);
static void records_wait(Carried *cr, uint8_t next, const uint16_t *rows, uint16_t count, uint8_t x, uint16_t empty);
static void rows_car(Carried *cr);
static void rows_course(Carried *cr);
static void rows_retry(Carried *cr);
static void sound(Carried *cr, uint8_t s);
static void total_line(Carried *cr, uint32_t b);
static void total_line_end(Carried *cr, uint32_t b, uint16_t at);
static void w_DigitTiles1184(Carried *cr);
static void w_GsuCF42To3000(Carried *cr);
static void w_GsuCF42To4000(Carried *cr);
static void w_OamTextLine(Carried *cr);
static void w_RecordsUnpackCourseRunning(Carried *cr);
static void w_SelectionSave(Carried *cr);
static void w_SoundPort0QueuePushX8(Carried *cr);
static void w_SpritePieces3(Carried *cr);
static void w_TilemapPaletteCycle(Carried *cr);
static void w_TilemapPaletteFill(Carried *cr);
static void w_TilemapRow187Palette4(Carried *cr);
static void w_TilemapRow207Palette4(Carried *cr);
static void w_TilemapRow287Palette4(Carried *cr);
static void w_TimeUnpackBcd(Carried *cr);
void SoundPort0QueuePushRawWide_cr(Carried *cr);
void SoundPort0QueuePushRawX8_cr(Carried *cr);
void SoundPort0QueuePushRaw_cr(Carried *cr);

/* LDA #v (8-bit). */
static void lda8_imm(Carried *cr, uint8_t v)
{
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    cr->p = nz8(cr->p, v);
}

static void lda8_imm_g(uint8_t v)
{
    Carried c = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};
    lda8_imm(&c, v);

    cpu.a = c.a;
    cpu.x = c.x;
    cpu.y = c.y;
    cpu.d = c.d;
    cpu.p = c.p;
    cpu.db = c.db;
}

/* INC of a work-RAM byte; N, Z from it. */
static void inc8_mem(Carried *cr, uint32_t addr)
{
    cyc(24);
    uint8_t v = (uint8_t)(rd8(addr) + 1);

    cyc(6);
    wr8(addr, v);
    cr->p = nz8(cr->p, v);
}

static void inc8_mem_g(uint32_t addr)
{
    Carried c = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};
    inc8_mem(&c, addr);

    cpu.a = c.a;
    cpu.x = c.x;
    cpu.y = c.y;
    cpu.d = c.d;
    cpu.p = c.p;
    cpu.db = c.db;
}

/* CMP # (8-bit). */
static void cmp8_imm(Carried *cr, uint8_t m)
{
    uint8_t a = (uint8_t)cr->a;

    cr->p = (uint8_t)((nz8(cr->p, (uint8_t)(a - m)) & ~P_C) | (a >= m ? P_C : 0));
}

static void cmp8_imm_g(uint8_t m)
{
    Carried c = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};
    cmp8_imm(&c, m);

    cpu.a = c.a;
    cpu.x = c.x;
    cpu.y = c.y;
    cpu.d = c.d;
    cpu.p = c.p;
    cpu.db = c.db;
}

/* ASL A (8-bit). */
static void asl8(Carried *cr)
{
    uint8_t v = (uint8_t)cr->a;

    cr->p = (uint8_t)((cr->p & ~P_C) | (v >> 7));
    v = (uint8_t)(v << 1);
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    cr->p = nz8(cr->p, v);
}

static void asl8_g(void)
{
    Carried c = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};
    asl8(&c);

    cpu.a = c.a;
    cpu.x = c.x;
    cpu.y = c.y;
    cpu.d = c.d;
    cpu.p = c.p;
    cpu.db = c.db;
}

/* LSR A four times (8-bit), CLC, ADC #$C0: a digit's tile from the high
   nibble. */
static uint8_t high_digit_tile(Carried *cr, uint8_t v)
{
    unsigned i;

    cyc(2);
    for (i = 0; i < 4; i++) {
        cyc(13);
        cr->p = (uint8_t)((cr->p & ~P_C) | (v & 1));
        v >>= 1;
    }
    cr->p &= (uint8_t)~P_C;
    return alu_adc8(v, 0xC0, &cr->p);
}

static uint8_t high_digit_tile_g(uint8_t v)
{
    Carried c = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};
    uint8_t ret_ = high_digit_tile(&c, v);

    cpu.a = c.a;
    cpu.x = c.x;
    cpu.y = c.y;
    cpu.d = c.d;
    cpu.p = c.p;
    cpu.db = c.db;
    return ret_;
}

/* AND #$0F, CLC, ADC #$C0: a digit's tile from the low nibble. */
static uint8_t low_digit_tile(Carried *cr, uint8_t v)
{
    cyc(2);
    v &= 0x0F;
    cr->p = nz8(cr->p, v);
    cr->p &= (uint8_t)~P_C;
    return alu_adc8(v, 0xC0, &cr->p);
}

static uint8_t low_digit_tile_g(uint8_t v)
{
    Carried c = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};
    uint8_t ret_ = low_digit_tile(&c, v);

    cpu.a = c.a;
    cpu.x = c.x;
    cpu.y = c.y;
    cpu.d = c.d;
    cpu.p = c.p;
    cpu.db = c.db;
    return ret_;
}

/* REP #$20, LDA presses, BIT #mask: Z from the test. */
static int pressed(Carried *cr, uint32_t b, uint16_t mask)
{
    r_rep(cr, P_M);
    cyc(2);
    r_lda16(cr, b | 0x0D1A);
    cr->p = (uint8_t)((cr->p & ~P_Z) | ((cr->a & mask) ? (cyc(40), 0) : (cyc(40), P_Z)));
    return (cr->a & mask) != 0;
}

/* Sound $45 or $46 on port 0, 8-bit; the push states its widths' time. */
static void sound(Carried *cr, uint8_t s)
{
    cyc(62);
    lda8_imm(cr, s);
    w_SoundPort0QueuePushX8(cr);
}

/* TilemapPaletteCycle on y entries from offset x, X 16-bit. */
static void palette_cycle(Carried *cr, uint16_t x, uint16_t y)
{
    cyc(94);
    r_rep(cr, P_X);
    cr->x = x;
    cr->p = nz16(cr->p, x);
    cr->y = y;
    cr->p = nz16(cr->p, y);
    w_TilemapPaletteCycle(cr);
}

/* The step $0D59 by the table at t: JMP (t,X) with X = $0D59 * 2. */
static uint16_t step_dispatch(uint32_t b, uint16_t t)
{
    uint16_t target;

    cyc(98);
    op_lda8(b | 0x0D59);
    asl8_g();
    cpu.x = (uint16_t)(cpu.a & 0x00FF);
    cpu.p = nz8(cpu.p, (uint8_t)cpu.x);
    cyc(74);
    target = rd16(0x070000 | (uint16_t)(t + cpu.x));
    jump_routine(0x070000 | target);
    return target;
}

/* The steps ChampionshipScreenStep (tilemap.c) also runs: it states their
   RTS; the others state it themselves. */
static unsigned step_rts(uint16_t target)
{
    (void)target;
    return 0;
}

/*
 * @routine 07:C6AA ResultsScreenStep
 * WHAT IT DOES: Runs the results screens' step $0D59 for HUD mode $0D2B:
 * mode 0 from the table at $07:C6C3 (16 steps), mode 1 from $07:C715 (14),
 * mode 3 from $07:C6EB (17); mode 2 has none.
 * HOW: Two JMP (table,X): by the mode, then by the step. Mode 3 is reached
 * only by unit_results_screen.
 * Entry: M 8-bit, X 8-bit. Exit: as the step's handler leaves it.
 */
void abi_ResultsScreenStep(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;

    cyc(24);
    op_lda8(b | 0x0D2B);
    asl8_g();
    cpu.x = (uint16_t)(cpu.a & 0x00FF);
    cpu.p = nz8(cpu.p, (uint8_t)cpu.x);
    switch (cpu.x) {
    case 0:
        cyc(step_rts(step_dispatch(b, 0xC6C3)));
        return;
    case 2:
        cyc(step_rts(step_dispatch(b, 0xC715)));
        return;
    case 4:
        cyc(116);
        return;
    case 6:
        cyc(step_rts(step_dispatch(b, 0xC6EB)));
        return;
    default:
        jump_routine(0x070000 | rd16(0x070000 | (uint16_t)(0xC6B2 + cpu.x)));
        return;
    }
}

/*
 * @routine 07:C911 PlaceFrameFromValue
 * WHAT IT DOES: The car sprite's frame for a record's tag A: 0 below $0C,
 * 3 below $18, 6 below $24, else 9; kept at $10C3.
 * HOW: SEP #$20, compares; REP #$20 at the end.
 * Entry: A low byte = the tag. Exit: M 16-bit; A low byte = the frame; C
 * from the last compare.
 */
void PlaceFrameFromValue(Carried *cr)
{
    uint8_t v, f;

    r_sep(cr, P_M);
    v = (uint8_t)cr->a;
    cmp8_imm(cr, 0x24);
    if (v >= 0x24)
        f = 0x09;
    else {
        cyc(54);
        cmp8_imm(cr, 0x18);
        if (v >= 0x18)
            f = 0x06;
        else {
            cyc(26);
            cmp8_imm(cr, 0x0C);
            f = v >= 0x0C ? (cyc(6), 0x03) : 0x00;
        }
    }
    lda8_imm(cr, f);
    cyc(146);
    wr8(((uint32_t)cr->db << 16) | 0x10C3, f);
    r_rep(cr, P_M);
}

void abi_PlaceFrameFromValue(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PlaceFrameFromValue(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* The car sprites of the three records: those whose time (or score) word
   at $0F2B + 2n is not `empty`, from their tags at $0F13 + 2n
   (PlaceFrameFromValue), at Y $54, $6C, $84 into sprite offsets $130,
   $140, $150 (SpritePieces3), each sprite's size byte ($0588 + n) = $60. */
static void record_cars(Carried *cr, uint32_t b, uint16_t empty)
{
    static const uint8_t ys[3] = {0x54, 0x6C, 0x84};
    unsigned n;

    for (n = 0; n < 3; n++) {
        r_rep(cr, P_M);
        cyc(24);
        r_lda16(cr, b | (uint16_t)(0x0F2B + 2 * n));
        if (empty == 0x0000) {
            cyc(22);
            if (!cr->a)
                continue;
        } else {
            cyc(46);
            r_cmp16(cr, empty);
            if (cr->a == empty)
                continue;
        }
        cyc(18);
        r_lda16(cr, b | (uint16_t)(0x0F13 + 2 * n));
        PlaceFrameFromValue(cr);
        r_sep(cr, P_M);
        if (n == 0)
            { cyc(88); wr8(b | 0x10C3, (uint8_t)cr->a); } else { cyc(64); }
        lda8_imm(cr, ys[n]);
        cyc(40);
        wr8(b | 0x10BF, ys[n]);
        r_rep(cr, P_M);
        cr->y = (uint16_t)(0x0130 + 0x10 * n);
        cr->p = nz16(cr->p, cr->y);
        (cyc(86), w_SpritePieces3(cr));
        r_sep(cr, P_M);
        lda8_imm(cr, 0x60);
        cyc(62);
        wr8(b | (uint16_t)(0x0588 + n), 0x60);
    }
}

/* The records' wait: B or START leaves for step `next` (sound $46; the
   size bytes $0588-$058A = $55, $10A2 = 5); else the player's place row
   ($10C9: 1, 2, 3 at offsets rows[0..2], `count` entries) cycles its
   palette, the piece selector steps every eighth frame and the cars are
   drawn at X `x`. */
static void records_wait(Carried *cr, uint8_t next, const uint16_t *rows, uint16_t count, uint8_t x, uint16_t empty)
{
    uint32_t b = (uint32_t)cr->db << 16;
    uint16_t place;
    uint8_t v;

    cyc(22);
    if (pressed(cr, b, 0x9000)) {
        cyc(38);
        r_sep(cr, P_M);
        sound(cr, 0x46);
        lda8_imm(cr, next);
        cyc(40);
        wr8(b | 0x0D59, next);
        lda8_imm(cr, 0x55);
        cyc(40);
        wr8(b | 0x0588, 0x55);
        cyc(24);
        wr8(b | 0x0589, 0x55);
        cyc(24);
        wr8(b | 0x058A, 0x55);
        lda8_imm(cr, 0x05);
        cyc(40);
        wr8(b | 0x10A2, 0x05);
        return;
    }
    r_rep(cr, P_M | P_X);
    cyc(30);
    r_lda16(cr, b | 0x10C9);
    place = cr->a;
    if (place) {
        cyc(56);
        r_cmp16(cr, 0x0002);
        palette_cycle(cr, place == 0x0002 ? (cyc(6), rows[1]) : place > 0x0002 ? rows[2] : (cyc(16), rows[0]), count);
    } else { cyc(22); }
    r_sep(cr, P_M);
    cyc(46);
    r_lda8(cr, b | 0x0E3B);
    v = (uint8_t)(cr->a & 0x07);
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    cmp8_imm(cr, 0x07);
    if (v == 0x07) {
        cyc(72);
        v = (uint8_t)(rd8(b | 0x10BB) + 1);
        cr->a = (uint16_t)((cr->a & 0xFF00) | v);
        cmp8_imm(cr, 0x04);
        if (v >= 0x04)
            lda8_imm(cr, 0x00);
        cyc(86);
        wr8(b | 0x10BB, (uint8_t)cr->a);
    } else { cyc(54); }
    lda8_imm(cr, 0x3E);
    cyc(40);
    wr8(b | 0x10C5, 0x3E);
    lda8_imm(cr, x);
    cyc(40);
    wr8(b | 0x10BD, x);
    lda8_imm(cr, 0xF4);
    cyc(40);
    wr8(b | 0x10C1, 0xF4);
    record_cars(cr, b, empty);
}

/*
 * @routine 07:C731 StuntRecordsWait
 * WHAT IT DOES: The Stunt Trax records after the race: B or START goes on
 * to step $A (sound $46, the three car sprites hidden, $10A2 = 5); else the
 * player's place row cycles its palette (TilemapPaletteCycle, 7 entries)
 * and the cars of the three records with a score are drawn at X $40.
 * HOW: The presses 16-bit, then 8-bit. Reached only by
 * unit_results_screen.
 * Entry: M 8-bit, X 8-bit; D = $0000. Exit: as the path leaves it.
 */
void StuntRecordsWait(Carried *cr)
{
    static const uint16_t rows[3] = {0x018F, 0x024F, 0x09AF};

    records_wait(cr, 0x0A, rows, 0x0007, 0x40, 0x0000);
    cyc(42);
}

void abi_StuntRecordsWait(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    StuntRecordsWait(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:C811 TimedRecordsWait
 * WHAT IT DOES: As StuntRecordsWait for the timed records ($FFFF for none):
 * step $D, 11 palette entries, the cars at X $58.
 * HOW: As StuntRecordsWait. Reached only by unit_results_screen.
 * Entry: as StuntRecordsWait. Exit: as StuntRecordsWait.
 */
void TimedRecordsWait(Carried *cr)
{
    static const uint16_t rows[3] = {0x0195, 0x0255, 0x0315};

    records_wait(cr, 0x0D, rows, 0x000B, 0x58, 0xFFFF);
    cyc(42);
}

void abi_TimedRecordsWait(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    TimedRecordsWait(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:C8FE ResultsNext
 * WHAT IT DOES: $0E5F cleared, $0D22 = 6, $0D23 cleared, the next step.
 * HOW: 8-bit stores.
 * Entry: any widths. Exit: M 8-bit; A low byte = 0.
 */
void ResultsNext(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;

    r_sep(cr, P_M);
    cyc(46);
    wr8(b | 0x0E5F, 0x00);
    lda8_imm(cr, 0x06);
    cyc(40);
    wr8(b | 0x0D22, 0x06);
    lda8_imm(cr, 0x00);
    cyc(40);
    wr8(b | 0x0D23, 0x00);
    inc8_mem(cr, b | 0x0D59);
    cyc(42);
}

void abi_ResultsNext(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ResultsNext(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:C8FA ResultsUnpackNext
 * WHAT IT DOES: Unpacks the course's records (RecordsUnpackCourseRunning),
 * then ResultsNext.
 * HOW: JSL, then ResultsNext's code, which follows.
 * Entry: as RecordsUnpackCourseRunning. Exit: as ResultsNext.
 */
void ResultsUnpackNext(Carried *cr)
{
    cyc(62);
    w_RecordsUnpackCourseRunning(cr);
    (cyc(0), ResultsNext(cr));
}

void abi_ResultsUnpackNext(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ResultsUnpackNext(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:C933 KeepTimeDigits
 * WHAT IT DOES: Keeps a time unpacked to BCD (TimeUnpackBcd: Y hundredths,
 * X seconds, A minutes) at direct page $F1, $F3 and $F5.
 * HOW: Three stores, 16-bit.
 * Entry: M 16-bit, X 16-bit; D = $0000. Exit: every register unchanged.
 */
void KeepTimeDigits(Carried *cr)
{
    cyc(62);
    wr16(0x0000F1, cr->y);
    cyc(16);
    wr16(0x0000F3, cr->x);
    cyc(16);
    wr16(0x0000F5, cr->a);
}

void abi_KeepTimeDigits(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    KeepTimeDigits(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:C93A ScoreTiles
 * WHAT IT DOES: Writes score A (BCD) into the tilemap buffer at X: the low
 * byte's digits at +14 and +16; when the high byte is below $40 its digits
 * at +0 and +2 and the tiles $C4, $C0 at +6 and +8; else the word spelled
 * by $D9, $CE, $DB, $CF, $CE, $CC, $DD from -2 with attribute $24.
 * HOW: A kept at $F1; 8-bit stores; digits are tile $C0 + the digit.
 * Entry: M 16-bit; X = the buffer offset; D = $0000. Exit: M 16-bit.
 */
void ScoreTiles(Carried *cr)
{
    static const uint8_t word[7] = {0xD9, 0xCE, 0xDB, 0xCF, 0xCE, 0xCC, 0xDD};
    uint16_t x = cr->x;
    uint8_t lo, hi;
    unsigned i;

    cyc(102);
    wr16(0x0000F1, cr->a);
    r_sep(cr, P_M);
    lo = (uint8_t)cr->a;
    wr8((uint32_t)(TILES + 16 + x), cyc_then(98, low_digit_tile(cr, lo)));
    cyc(16);
    lo = rd8(0x0000F1);
    wr8((uint32_t)(TILES + 14 + x), cyc_then(64, high_digit_tile(cr, lo)));
    cyc(16);
    hi = rd8(0x0000F2);
    cr->a = (uint16_t)((cr->a & 0xFF00) | hi);
    cmp8_imm(cr, 0x40);
    if (hi < 0x40) {
        wr8((uint32_t)(TILES + 2 + x), cyc_then(124, low_digit_tile(cr, hi)));
        cyc(16);
        hi = rd8(0x0000F2);
        wr8((uint32_t)(TILES + x), cyc_then(64, high_digit_tile(cr, hi)));
        lda8_imm(cr, 0xC4);
        cyc(48);
        wr8((uint32_t)(TILES + 6 + x), 0xC4);
        lda8_imm(cr, 0xC0);
        cyc(48);
        wr8((uint32_t)(TILES + 8 + x), 0xC0);
        r_rep(cr, P_M);
        return;
    } else { cyc(76); }
    r_sep(cr, P_M);
    for (i = 0; i < 7; i++)
        { cyc(48); wr8((uint32_t)(TILES - 2 + 2 * i + x), word[i]); }
    cyc(16);
    lda8_imm(cr, 0x24);
    for (i = 0; i < 7; i++)
        { cyc(32); wr8((uint32_t)(TILES - 1 + 2 * i + x), 0x24); }
    r_rep(cr, P_M);
}

void abi_ScoreTiles(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ScoreTiles(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:C9CE TimeTiles
 * WHAT IT DOES: Writes the time kept by KeepTimeDigits into the tilemap
 * buffer at X: the minutes' digit at +0, the seconds' at +4 and +6, the
 * hundredths' at +10 and +12.
 * HOW: 8-bit; digits are tile $C0 + the digit.
 * Entry: X = the buffer offset; D = $0000. Exit: M 16-bit.
 */
void TimeTiles(Carried *cr)
{
    uint16_t x = cr->x;
    uint8_t v;

    r_sep(cr, P_M);
    cyc(150);
    v = rd8(0x0000F5);
    cr->p = nz8(cr->p, v);
    cr->p &= (uint8_t)~P_C;
    cyc(78);
    wr8((uint32_t)(TILES + x), alu_adc8(v, 0xC0, &cr->p));
    cyc(16);
    wr8((uint32_t)(TILES + 4 + x), cyc_then(64, high_digit_tile(cr, rd8(0x0000F3))));
    cyc(16);
    wr8((uint32_t)(TILES + 6 + x), cyc_then(76, low_digit_tile(cr, rd8(0x0000F3))));
    cyc(16);
    wr8((uint32_t)(TILES + 10 + x), cyc_then(64, high_digit_tile(cr, rd8(0x0000F1))));
    cyc(16);
    v = low_digit_tile(cr, rd8(0x0000F1));
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    cyc(76);
    wr8((uint32_t)(TILES + 12 + x), v);
    r_rep(cr, P_M);
}

void abi_TimeTiles(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    TimeTiles(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* The three letters of name word A (bits 0-4, 5-9, 10-14) as tiles $CA +
   the letter at the buffer offset X + at, + 2, + 4. A kept on the stack
   between them. */
static void name_tiles(Carried *cr, uint16_t at)
{
    uint16_t a = cr->a, x = cr->x, v;

    r_sep(cr, P_M);
    v = (uint16_t)(a & 0x001F);
    cr->p &= (uint8_t)~P_C;
    cyc(176);
    wr8((uint32_t)(TILES + at + x), alu_adc8((uint8_t)v, 0xCA, &cr->p));
    r_rep(cr, P_M);
    v = (uint16_t)(a << 3);
    v = (uint16_t)(v << 8 | v >> 8);
    r_sep(cr, P_M);
    cr->p &= (uint8_t)~P_C;
    cyc(228);
    wr8((uint32_t)(TILES + at + 2 + x), alu_adc8((uint8_t)(v & 0x1F), 0xCA, &cr->p));
    r_rep(cr, P_M);
    v = (uint16_t)(a >> 2);
    v = (uint16_t)(v << 8 | v >> 8);
    r_sep(cr, P_M);
    cr->p &= (uint8_t)~P_C;
    v = alu_adc8((uint8_t)(v & 0x1F), 0xCA, &cr->p);
    cr->a = (uint16_t)((uint16_t)(a >> 2) << 8 & 0xFF00) | v;
    cyc(184);
    wr8((uint32_t)(TILES + at + 4 + x), (uint8_t)v);
    r_rep(cr, P_M);
}

/*
 * @routine 07:CA0E NameTiles16
 * WHAT IT DOES: Writes the three letters of name word A as tiles $CA + the
 * letter into the tilemap buffer at X + 16, + 18, + 20.
 * HOW: A kept on the stack; each letter AND #$1F after shifts and XBA.
 * Entry: M 16-bit; X = the buffer offset. Exit: M 16-bit.
 */
void NameTiles16(Carried *cr)
{
    name_tiles(cr, 16);
}

void abi_NameTiles16(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    NameTiles16(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:CA41 NameTiles26
 * WHAT IT DOES: As NameTiles16 at X + 26, + 28, + 30.
 * HOW: As NameTiles16.
 * Entry: as NameTiles16. Exit: as NameTiles16.
 */
void NameTiles26(Carried *cr)
{
    name_tiles(cr, 26);
}

void abi_NameTiles26(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    NameTiles26(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* A panel: Super FX picture `pic` with X = `x` (GsuCF42To3000), copied to
   the tilemap buffer ($440 bytes from $70:3000, MVN), then DigitTiles1184. */
static void panel(Carried *cr, uint16_t x, uint16_t pic)
{
    cyc(72);
    uint8_t db = cr->db, p;
    unsigned i;

    r_rep(cr, P_M | P_X);
    cr->x = x;
    cr->p = nz16(cr->p, x);
    r_lda16_imm(cr, pic);
    w_GsuCF42To3000(cr);
    p = cr->p;
    r_rep(cr, P_M | P_X);
    cyc(220);
    for (i = 0; i < 0x0440; i++)
        { cyc(36); wr8((uint32_t)(TILES + i), rd8((uint32_t)(0x703000 + i))); }
    cyc(72);
    cr->a = 0xFFFF;
    cr->x = 0x3440;
    cr->y = 0x9817;
    cr->db = db;
    r_plp(cr, p);
    w_DigitTiles1184(cr);
}

/*
 * @routine 07:CA74 StuntRecordsDraw
 * WHAT IT DOES: Draws the Stunt Trax records panel (picture $FB21) and the
 * three records with a score: their scores (ScoreTiles) at $190, $250,
 * $310 and their names (NameTiles26); the next step, $10A2 = 2.
 * HOW: GsuCF42To3000, MVN of $440 bytes, DigitTiles1184. Reached only by
 * unit_results_screen.
 * Entry: D = $0000. Exit: M 8-bit, X 16-bit.
 */
void StuntRecordsDraw(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;
    unsigned n;

    int broke = 0;

    panel(cr, 0x001F, 0xFB21);
    r_rep(cr, P_M | P_X);
    for (n = 0; n < 3; n++) {
        cyc(n ? 66 : 24);
        r_lda16(cr, b | (uint16_t)(0x0F2B + 2 * n));
        if (!cr->a)
            { broke = 1; break; }
        cr->x = (uint16_t)(0x0190 + 0xC0 * n);
        cr->p = nz16(cr->p, cr->x);
        ScoreTiles(cr);
        cyc(66);
        r_lda16(cr, b | (uint16_t)(0x0F43 + 2 * n));
        NameTiles26(cr);
    }
    cyc(broke ? 44 : 64);               /* BEQ taken, or the RTS; SEP */
    r_sep(cr, P_M);
    inc8_mem(cr, b | 0x0D59);
    lda8_imm(cr, 0x02);
    cyc(40);
    wr8(b | 0x10A2, 0x02);
    cyc(42);
}

void abi_StuntRecordsDraw(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    StuntRecordsDraw(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:CAD5 ResultsPanelFAB5
 * WHAT IT DOES: Draws the panel of picture $FAB5; the next step.
 * HOW: GsuCF42To3000, MVN of $440 bytes, DigitTiles1184. Reached only by
 * unit_results_screen.
 * Entry: D = $0000. Exit: M 8-bit, X 16-bit.
 */
void ResultsPanelFAB5(Carried *cr)
{
    panel(cr, 0x001F, 0xFAB5);
    r_sep(cr, P_M);
    cyc(22);
    inc8_mem(cr, ((uint32_t)cr->db << 16) | 0x0D59);
    cyc(42);
}

void abi_ResultsPanelFAB5(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ResultsPanelFAB5(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:CAFC TimedRecordsDraw
 * WHAT IT DOES: Draws the timed records panel (picture $F3F2, X $1C) and
 * the three records with a time: their times (TimeUnpackBcd,
 * KeepTimeDigits, TimeTiles) at $194, $254, $314 and their names
 * (NameTiles16); the next step, $10A2 = 2.
 * HOW: As StuntRecordsDraw. Reached only by unit_results_screen.
 * Entry: D = $0000. Exit: M 8-bit, X 16-bit.
 */
void TimedRecordsDraw(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;
    unsigned n;

    int broke = 0;

    panel(cr, 0x001C, 0xF3F2);
    r_rep(cr, P_M);
    for (n = 0; n < 3; n++) {
        cyc(n ? 66 : 24);
        r_lda16(cr, b | (uint16_t)(0x0F2B + 2 * n));
        r_cmp16(cr, 0xFFFF);
        if (cr->a == 0xFFFF)
            { broke = 1; break; } else { cyc(86); }
        w_TimeUnpackBcd(cr);
        KeepTimeDigits(cr);
        if (n)
            r_rep(cr, P_X);
        cr->x = (uint16_t)(0x0194 + 0xC0 * n);
        cr->p = nz16(cr->p, cr->x);
        TimeTiles(cr);
        cyc(66);
        r_lda16(cr, b | (uint16_t)(0x0F43 + 2 * n));
        NameTiles16(cr);
    }
    cyc(broke ? 68 : 64);               /* CMP, BEQ taken, or the RTS; SEP */
    r_sep(cr, P_M);
    inc8_mem(cr, b | 0x0D59);
    lda8_imm(cr, 0x02);
    cyc(40);
    wr8(b | 0x10A2, 0x02);
    cyc(42);
}

void abi_TimedRecordsDraw(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    TimedRecordsDraw(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:CB7C ResultsPanelF38E
 * WHAT IT DOES: Draws the panel of picture $F38E (X $1C); the next step.
 * HOW: As ResultsPanelFAB5. Reached only by unit_results_screen and unit_entries_07.
 * Entry: D = $0000. Exit: M 8-bit, X 16-bit.
 */
void ResultsPanelF38E(Carried *cr)
{
    panel(cr, 0x001C, 0xF38E);
    r_sep(cr, P_M);
    cyc(22);
    inc8_mem(cr, ((uint32_t)cr->db << 16) | 0x0D59);
    cyc(42);
}

void abi_ResultsPanelF38E(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ResultsPanelF38E(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* B or START on the records: step `skip` (sound $46) for a placed player
   ($10C9) when `skip` applies, else the next step (sound $46). */
static void records_choice(Carried *cr, uint32_t b, uint8_t skip, int check_10cd)
{
    if (!pressed(cr, b, 0x9000))
        { cyc(6 + 42); return; }
    r_rep(cr, P_M | P_X);
    if (check_10cd) {
        cyc(2);
        r_lda16(cr, b | 0x10CD);
        r_and16(cr, 0x00FF);
        if (cr->a)
            { cyc(22); goto next; } else { cyc(60); }
    }
    cyc(2);
    r_lda16(cr, b | 0x10C9);
    if (!cr->a)
        goto next;
    r_sep(cr, P_M);
    lda8_imm(cr, skip);
    cyc(78);
    wr8(b | 0x0D59, skip);
    cyc(16);
    sound(cr, 0x46);
    cyc(42);
    return;
next:
    cyc(44);
    r_sep(cr, P_M);
    inc8_mem(cr, b | 0x0D59);
    cyc(16);
    sound(cr, 0x46);
    cyc(42);
}

/*
 * @routine 07:CBA3 StuntRecordsChoice
 * WHAT IT DOES: With $19EC at $40 the place row at $199 cycles its palette
 * (TilemapPaletteCycle, 7 entries); B or START goes on: to step $C for a
 * placed player ($10C9) without $10CD, else the next step; sound $46.
 * HOW: 8-bit, the presses 16-bit. Reached only by unit_results_screen.
 * Entry: M 8-bit. Exit: as the path leaves it.
 */
void StuntRecordsChoice(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;

    r_sep(cr, P_M);
    cyc(46);
    r_lda8(cr, b | 0x19EC);
    cmp8_imm(cr, 0x40);
    if ((uint8_t)cr->a == 0x40)
        { cyc(32); palette_cycle(cr, 0x0199, 0x0007); } else { cyc(38); }
    r_rep(cr, P_M | P_X);
    records_choice(cr, b, 0x0C, 1);
}

void abi_StuntRecordsChoice(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    StuntRecordsChoice(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:CBE8 TimedRecordsChoice
 * WHAT IT DOES: B or START goes on: to step $F for a placed player
 * ($10C9), else the next step; sound $46.
 * HOW: The presses 16-bit. Reached only by unit_results_screen.
 * Entry: any widths. Exit: as the path leaves it.
 */
void TimedRecordsChoice(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;

    r_rep(cr, P_M | P_X);
    records_choice(cr, b, 0x0F, 0);
}

void abi_TimedRecordsChoice(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    TimedRecordsChoice(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:CC11 ResultsPanelFB99
 * WHAT IT DOES: $0D23, the menu choice $0DD4 and the palette phase $0DD5
 * cleared; the panel of picture $FB99; the next step, $0D23 = 2.
 * HOW: As ResultsPanelFAB5. Reached only by unit_results_screen and unit_entries_07.
 * Entry: D = $0000. Exit: M 8-bit, X 16-bit.
 */
void ResultsPanelFB99(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;

    r_sep(cr, P_M);
    lda8_imm(cr, 0x00);
    cyc(62);
    wr8(b | 0x0D23, 0x00);
    cyc(24);
    wr8(b | 0x0DD4, 0x00);
    cyc(24);
    wr8(b | 0x0DD5, 0x00);
    panel(cr, 0x001F, 0xFB99);
    r_sep(cr, P_M);
    cyc(22);
    inc8_mem(cr, b | 0x0D59);
    lda8_imm(cr, 0x02);
    cyc(40);
    wr8(b | 0x0D23, 0x02);
    cyc(42);
}

void abi_ResultsPanelFB99(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ResultsPanelFB99(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:CC4A ResultsPanelFA39
 * WHAT IT DOES: Draws the panel of picture $FA39; with $0EEF set 07:D3F6,
 * and with a record time at $0F81 other than $FF 07:D4F8; the next step.
 * HOW: As ResultsPanelFAB5. The two calls are not reached in the traces so
 * far.
 * Entry: D = $0000. Exit: M 8-bit, X 16-bit.
 */
void ResultsPanelFA39(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;

    panel(cr, 0x001F, 0xFA39);
    int total, record;

    r_sep(cr, P_M);
    cyc(46);
    r_lda8(cr, b | 0x0EEF);
    total = (uint8_t)cr->a != 0;
    if (total)
        { cyc(62); ResultsTotalTiles(cr); }
    cyc(total ? 24 : 46);               /* LDA abs, or BEQ taken first */
    r_lda8(cr, b | 0x0F81);
    cmp8_imm(cr, 0xFF);
    record = (uint8_t)cr->a != 0xFF;
    if (record)
        { cyc(78); RecordTimeTiles(cr); }
    cyc(record ? 0 : 38);
    inc8_mem(cr, b | 0x0D59);
    cyc(42);
}

void abi_ResultsPanelFA39(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ResultsPanelFA39(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:CC83 ResultsPicture4000
 * WHAT IT DOES: Draws Super FX picture $C770 (X $18) to $4000
 * (GsuCF42To4000); the next step; $10A2 and $10A3 cleared.
 * HOW: 16-bit set-up, 8-bit after.
 * Entry: D = $0000. Exit: M 8-bit, X 16-bit.
 */
void ResultsPicture4000(Carried *cr)
{
    cyc(72);
    uint32_t b = (uint32_t)cr->db << 16;

    r_rep(cr, P_M | P_X);
    cr->x = 0x0018;
    cr->p = nz16(cr->p, 0x0018);
    r_lda16_imm(cr, 0xC770);
    w_GsuCF42To4000(cr);
    r_sep(cr, P_M);
    cyc(94);
    inc8_mem(cr, b | 0x0D59);
    cyc(24);
    wr8(b | 0x10A2, 0x00);
    cyc(24);
    wr8(b | 0x10A3, 0x00);
    cyc(42);
}

void abi_ResultsPicture4000(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ResultsPicture4000(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* $10A0 = the car $0D89's word from the table at $07:CCB7. */
static void car_picture(Carried *cr, uint32_t b)
{
    r_rep(cr, P_M | P_X);
    cyc(24);
    r_lda16(cr, b | 0x0D89);
    r_and16(cr, 0x00FF);
    r_asl16(cr);
    r_tax16(cr);
    cyc(92);
    r_lda16(cr, 0x07CCB7 + cr->x);
    wr16(b | 0x10A0, cr->a);
}

/*
 * @routine 07:CC9B ResultsCarPicture
 * WHAT IT DOES: $0D23 = $12, the next step, and $10A0 = the car $0D89's
 * picture word from the table at $07:CCB7 ($63D7, $6FD7, $7BD7, $87D7).
 * HOW: 8-bit, then 16-bit.
 * Entry: any widths. Exit: M 16-bit, X 16-bit.
 */
void ResultsCarPicture(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;

    r_sep(cr, P_M);
    lda8_imm(cr, 0x12);
    cyc(62);
    wr8(b | 0x0D23, 0x12);
    inc8_mem(cr, b | 0x0D59);
    car_picture(cr, b);
    cyc(42);
}

void abi_ResultsCarPicture(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ResultsCarPicture(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* The menu's rows: the chosen row (offset x, y entries) cycles its
   palette, the other two palette 4. */
static void menu_rows(Carried *cr, uint16_t x, uint16_t y, void (*a)(Carried *), void (*c)(Carried *))
{
    r_sep(cr, P_M);
    cyc(22);
    palette_cycle(cr, x, y);
    (cyc(46), a(cr));
    (cyc(88), c(cr));
    cyc(84);                            /* c's RTS, then the menu's */
}

static void rows_retry(Carried *cr)
{
    (cyc(6), menu_rows(cr, 0x0187, 0x0009, w_TilemapRow207Palette4, w_TilemapRow287Palette4));
}

static void rows_course(Carried *cr)
{
    (cyc(6), menu_rows(cr, 0x0207, 0x000D, w_TilemapRow187Palette4, w_TilemapRow287Palette4));
}

static void rows_car(Carried *cr)
{
    menu_rows(cr, 0x0287, 0x000A, w_TilemapRow187Palette4, w_TilemapRow207Palette4);
}

/* The car one on or back (right or left), skipping car 3 unless HUD mode
   3 or bit 2 of the save's progress byte $70:FC0B allows it; then the
   selection ($18CB = the car's code from $07:CE93) saved
   (SelectionSave), $0D23 = $12 and its picture word. */
static void car_change(Carried *cr, uint32_t b, int right)
{
    uint8_t v;

    if (right) {
        r_sep(cr, P_M | P_X);
        cyc(16);
        sound(cr, 0x45);
        cyc(24);
        r_lda8(cr, b | 0x0D89);
        v = (uint8_t)(cr->a + 1);
        lda8_imm(cr, v);
        cmp8_imm(cr, 0x03);
        if (v == 0x03) {
            cyc(76);
            cr->x = rd8(b | 0x0D2B);
            cr->p = (uint8_t)((nz8(cr->p, (uint8_t)(cr->x - 0x03)) & ~P_C) | ((uint8_t)cr->x >= 0x03 ? P_C : 0));
            if ((uint8_t)cr->x != 0x03) {
                cyc(64);
                r_lda8(cr, 0x70FC0B);
                cr->p = (uint8_t)((cr->p & ~P_Z) | (((uint8_t)cr->a & 0x04) ? (cyc(32), 0) : (cyc(16), P_Z)));
                lda8_imm(cr, (uint8_t)cr->a & 0x04 ? 0x03 : 0x00);
            }
        } else if (v > 0x03) {
            cyc(46);
            lda8_imm(cr, 0x00);
        } else { cyc(46); }
    cyc(2);
        } else {
        cyc(38);
        r_sep(cr, P_M);
        sound(cr, 0x45);
        cyc(24);
        r_lda8(cr, b | 0x0D89);
        v = (uint8_t)(cr->a - 1);
        lda8_imm(cr, v);
        if (v & 0x80) {
            lda8_imm(cr, 0x03);
            cyc(70);
            cr->x = rd8(b | 0x0D2B);
            cr->p = (uint8_t)((nz8(cr->p, (uint8_t)(cr->x - 0x03)) & ~P_C) | ((uint8_t)cr->x >= 0x03 ? P_C : 0));
            if ((uint8_t)cr->x != 0x03) {
                cyc(64);
                r_lda8(cr, 0x70FC0B);
                cr->p = (uint8_t)((cr->p & ~P_Z) | (((uint8_t)cr->a & 0x04) ? (cyc(32), 0) : (cyc(16), P_Z)));
                lda8_imm(cr, (uint8_t)cr->a & 0x04 ? 0x03 : 0x02);
            }
            cyc(2);
        }
    }
    cyc(60);
    wr8(b | 0x0D89, (uint8_t)cr->a);
    r_sep(cr, P_M | P_X);
    cyc(right ? 46 : 24);               /* BRA, or not; LDA abs */
    r_lda8(cr, b | 0x0D89);
    cr->x = (uint16_t)(cr->a & 0x00FF);
    cr->p = nz8(cr->p, (uint8_t)cr->x);
    cyc(86);
    r_lda8(cr, 0x07CE93 + cr->x);
    wr8(0x0018CB, (uint8_t)cr->a);
    (cyc(62), w_SelectionSave(cr));
    r_sep(cr, P_M);
    lda8_imm(cr, 0x12);
    cyc(62);
    wr8(b | 0x0D23, 0x12);
    car_picture(cr, b);
}

/*
 * @routine 07:CCBF ResultsMenu
 * WHAT IT DOES: The menu after the race, unless the fade out ($0E3A) has
 * started: the choice $0DD4 (0 retry, 1 course, 2 car, 3 the car's choice)
 * moves with up and down (sound $45; down from 0 not with $10CD) and its
 * row cycles its palette; B or START: on 0 or 1 starts the fade out (sound
 * $46); on 2 the car's choice (3); on 3 back to 0 (the car sprites'
 * size byte $0582 = $55). In the car's choice, the car sprites 52 and 53
 * blink by bit 3 of the frame counter beside $10A4, $10A6, and right and
 * left change the car $0D89 (car_change).
 * HOW: 8-bit; the presses 16-bit; the rows by TilemapPaletteCycle and the
 * TilemapRow...Palette4 routines. The move up from 1 and 2 and the car
 * back are not reached in the traces so far.
 * Entry: M 8-bit. Exit: as the path leaves it.
 */
void ResultsMenu(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;
    uint8_t c, f, v;

    r_sep(cr, P_M);
    cyc(46);
    r_lda8(cr, b | 0x0E3A);
    if ((uint8_t)cr->a)
        { cyc(58); return; } else { cyc(44); }
    if (pressed(cr, b, 0x9000)) {
        r_sep(cr, P_M);
        cyc(46);
        r_lda8(cr, b | 0x0DD4);
        c = (uint8_t)cr->a;
        cmp8_imm(cr, 0x02);
        if (c < 0x02) {
            lda8_imm(cr, 0x01);
            cyc(72);
            wr8(b | 0x0E3A, 0x01);
            cyc(16);
            sound(cr, 0x46);
            cyc(42);
            return;
        }
        if (c == 0x02) {
            cyc(60);
            inc8_mem(cr, b | 0x0DD4);
            cyc(16);
            sound(cr, 0x46);
            (cyc(24), rows_car(cr));
            return;
        }
        lda8_imm(cr, 0x00);
        cyc(94);
        wr8(b | 0x0DD4, 0x00);
        cyc(16);
        sound(cr, 0x46);
        lda8_imm(cr, 0x55);
        cyc(40);
        wr8(b | 0x0582, 0x55);
        (cyc(18), rows_retry(cr));
        return;
    }
    r_sep(cr, P_M);
    lda8_imm(cr, 0x02);
    cyc(68);
    wr8(b | 0x0D23, 0x02);
    cyc(24);
    r_lda8(cr, b | 0x0DD4);
    c = (uint8_t)cr->a;
    if (c == 0x00) {
        cyc(44);
        if (pressed(cr, b, 0x0400)) {
            r_sep(cr, P_M);
            cyc(46);
            r_lda8(cr, b | 0x10CD);
            if (!(uint8_t)cr->a) {
                cyc(16);
                inc8_mem(cr, b | 0x0DD4);
                cyc(16);
                sound(cr, 0x45);
                cyc(16);
                rows_course(cr);
                return;
            } else { cyc(16); }
        }
        rows_retry(cr);
        return;
    } else { cyc(76); }
    cmp8_imm(cr, 0x02);
    if (c == 0x01) {
        cyc(16);
        if (pressed(cr, b, 0x0800)) {
            r_sep(cr, P_M);
            cyc(52);
            wr8(b | 0x0DD4, 0x00);
            cyc(16);
            sound(cr, 0x45);
            cyc(16);
            rows_retry(cr);
            return;
        }
        if (pressed(cr, b, 0x0400)) {
            cyc(22);
            r_sep(cr, P_M);
            inc8_mem(cr, b | 0x0DD4);
            cyc(16);
            sound(cr, 0x45);
            (cyc(24), rows_car(cr));
            return;
        }
        rows_course(cr);
        return;
    }
    if (c == 0x02) {
        if (!pressed(cr, b, 0x0800)) {
            cyc(30);
            rows_car(cr);
            return;
        } else { cyc(44); }
        r_sep(cr, P_M);
        sound(cr, 0x45);
        lda8_imm(cr, 0x01);
        cyc(40);
        wr8(b | 0x0DD4, 0x01);
        (cyc(16), rows_course(cr));
        return;
    }
    r_sep(cr, P_M);
    cyc(64);
    r_lda8(cr, b | 0x0E3B);
    f = (uint8_t)cr->a;
    cr->p = (uint8_t)((cr->p & ~P_Z) | ((f & 0x08) ? 0 : (cyc(54), P_Z)));
    if (!(f & 0x08)) {
        lda8_imm(cr, 0x55);
    } else {
        cyc(62);
        v = rd8(b | 0x10A4);
        cr->p |= P_C;
        v = alu_sbc8(v, 0x08, &cr->p);
        cyc(38);
        wr8(b | 0x0445, v);
        cyc(24);
        v = rd8(b | 0x10A4);
        cr->p &= (uint8_t)~P_C;
        v = alu_adc8(v, 0x40, &cr->p);
        cyc(54);
        wr8(b | 0x0449, v);
        cyc(24);
        v = rd8(b | 0x10A6);
        cr->p &= (uint8_t)~P_C;
        v = alu_adc8(v, 0x18, &cr->p);
        cyc(54);
        wr8(b | 0x0446, v);
        cyc(24);
        wr8(b | 0x044A, v);
        lda8_imm(cr, 0x19);
        cyc(40);
        wr8(b | 0x0447, 0x19);
        cyc(24);
        wr8(b | 0x044B, 0x19);
        lda8_imm(cr, 0x76);
        cyc(40);
        wr8(b | 0x0448, 0x76);
        lda8_imm(cr, 0x36);
        cyc(40);
        wr8(b | 0x044C, 0x36);
        lda8_imm(cr, 0x50);
    }
    cyc(40);
    wr8(b | 0x0582, (uint8_t)cr->a);
    cyc(22);
    if (pressed(cr, b, 0x0100))
        car_change(cr, b, 1);
    else { cyc(28); if (pressed(cr, b, 0x0200))
        car_change(cr, b, 0); else { cyc(6); } }
    rows_car(cr);
}

void abi_ResultsMenu(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ResultsMenu(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void abi_SoundPort0QueuePushRaw(void);
void abi_SoundPort0QueuePushRawX8(void);
void abi_SoundPort0QueuePushRawWide(void);
void abi_OamTextLine(void);
void abi_OamWordRow5(void);
void abi_TilemapPaletteFill(void);

/*
 * @routine 07:CF32 ResultsChampionScreen
 * WHAT IT DOES: Loads the championship screen's 256 colours ($1F:8600) into
 * the palette buffer at $7E:FD65, sets BG2's scroll to (-24, -32) ($0D92,
 * $0D8E) and its tilemap to $5C (BG2SC); the next step.
 * HOW: A word copy from the end; BG2SC written 16-bit.
 * Entry: any widths. Exit: M 8-bit, X 16-bit; X = $FFFE.
 */
void ResultsChampionScreen(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;
    int x;

    r_rep(cr, P_M | P_X);
    for (x = 0x01FE; x >= 0; x -= 2) {
        cyc(104);
        r_lda16(cr, (uint32_t)(0x1F8600 + x));
        wr16((uint32_t)(0x7EFD65 + x), cr->a);
        cyc(26);
    }
    cr->x = 0xFFFE;
    cr->p = nz16(cr->p, 0xFFFE);
    r_lda16_imm(cr, 0xFFE0);
    cyc(28);
    wr16(b | 0x0D8E, 0xFFE0);
    r_lda16_imm(cr, 0xFFE8);
    cyc(10);
    wr16(b | 0x0D92, 0xFFE8);
    r_lda16_imm(cr, 0x005C);
    cyc(10);
    ppu_wr16(b | 0x2108, 0x005C);
    r_sep(cr, P_M);
    cyc(22);
    inc8_mem(cr, b | 0x0D59);
    cyc(42);
}

void abi_ResultsChampionScreen(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ResultsChampionScreen(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:CF5B ResultsWaitWindow
 * WHAT IT DOES: Once the split window state $19D8 is 0, the next step and
 * sound $28 on port 0; each frame $0D23 = 6.
 * HOW: 8-bit.
 * Entry: M 8-bit. Exit: A low byte = 6.
 */
void ResultsWaitWindow(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;

    cyc(24);
    r_lda8(cr, b | 0x19D8);
    if (!(uint8_t)cr->a) {
        cyc(16);
        inc8_mem(cr, b | 0x0D59);
        cyc(24);
        lda8_imm(cr, 0x28);
        SoundPort0QueuePushRaw_cr(cr);
    } else { cyc(30); }
    lda8_imm(cr, 0x06);
    cyc(32);
    wr8(b | 0x0D23, 0x06);
    cyc(42);
}

void abi_ResultsWaitWindow(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ResultsWaitWindow(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* The results' text line: $0D45's low nibble 2; direct page $F2 = $60,
   $F3 = $67, $F4 = $2F, $F1 = $C0, $F5 = $38 and OamTextLine. */
static void total_line(Carried *cr, uint32_t b)
{
    r_sep(cr, P_M);
    cyc(46);
    r_lda8(cr, b | 0x0D45);
    lda8_imm(cr, (uint8_t)(((uint8_t)cr->a & 0xF0) | 0x02));
    cyc(56);
    wr8(b | 0x0D45, (uint8_t)cr->a);
    lda8_imm(cr, 0x60);
    cyc(32);
    wr8(0x0000F2, 0x60);
    lda8_imm(cr, 0x67);
    cyc(32);
    wr8(0x0000F3, 0x67);
    lda8_imm(cr, 0x2F);
    cyc(32);
    wr8(0x0000F4, 0x2F);
    lda8_imm(cr, 0xC0);
    cyc(32);
    wr8(0x0000F1, 0xC0);
    lda8_imm(cr, 0x38);
    cyc(32);
    wr8(0x0000F5, 0x38);
    (cyc(46), w_OamTextLine(cr));
}

static void total_line_g(uint32_t b)
{
    Carried c = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};
    total_line(&c, b);

    cpu.a = c.a;
    cpu.x = c.x;
    cpu.y = c.y;
    cpu.d = c.d;
    cpu.p = c.p;
    cpu.db = c.db;
}

/* $0D23 = 4, the next step, $0E43 = at (16-bit). */
static void total_line_end(Carried *cr, uint32_t b, uint16_t at)
{
    lda8_imm(cr, 0x04);
    cyc(40);
    wr8(b | 0x0D23, 0x04);
    inc8_mem(cr, b | 0x0D59);
    r_rep(cr, P_M);
    r_lda16_imm(cr, at);
    cyc(10);
    wr16(b | 0x0E43, at);
}

static void total_line_end_g(uint32_t b, uint16_t at)
{
    Carried c = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};
    total_line_end(&c, b, at);

    cpu.a = c.a;
    cpu.x = c.x;
    cpu.y = c.y;
    cpu.d = c.d;
    cpu.p = c.p;
    cpu.db = c.db;
}

/*
 * @routine 07:CF6F ResultsTotalLine
 * WHAT IT DOES: The results' text line ($0D45's low nibble 2; OamTextLine
 * with $F1-$F5 = $C0, $60, $67, $2F, $38); $0D23 = 4, the next step and the
 * digit position $0E43 = $190.
 * HOW: 8-bit, the position 16-bit.
 * Entry: any widths; D = $0000. Exit: M 16-bit; A = $190.
 */
void ResultsTotalLine(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;

    total_line(cr, b);
    total_line_end(cr, b, 0x0190);
    cyc(42);
}

void abi_ResultsTotalLine(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ResultsTotalLine(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void abi_ResultsTotalTiles(void);

/*
 * @routine 07:CFE8 ResultsTotalLineWords
 * WHAT IT DOES: As ResultsTotalLine, with the word row (OamWordRow5) and
 * the position $18E; then with minutes at $0EEC the total's tiles
 * (ResultsTotalTiles).
 * HOW: As ResultsTotalLine; the last is a jump. The minutes are not
 * reached in the traces so far.
 * Entry: any widths; D = $0000. Exit: M 8-bit.
 */
void abi_ResultsTotalLineWords(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;

    total_line_g(b);
    (cyc(46), abi_OamWordRow5());
    total_line_end_g(b, 0x018E);
    op_sep(P_M);
    cyc(46);
    op_lda8(b | 0x0EEC);
    if ((uint8_t)cpu.a)
        { cyc(40); jump_routine(0x07D3F6); } else { cyc(22 + 42); }
}

/* A BCD count byte at `at` one more (SED, CLC, ADC #1, CLD) shown as two
   digit tiles at hi and lo in the buffer. */
static void count_digits(Carried *cr, uint32_t b, uint16_t at, uint32_t hi, uint32_t lo)
{
    uint8_t v;

    cr->p |= P_D;
    cyc(54);
    r_lda8(cr, b | at);
    cr->p &= (uint8_t)~P_C;
    v = alu_adc8((uint8_t)cr->a, 0x01, &cr->p);
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    cyc(54);
    wr8(b | at, v);
    cr->p &= (uint8_t)~P_D;
    v = low_digit_tile(cr, v);
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    cyc(90);
    wr8(lo, v);
    cyc(24);
    v = high_digit_tile(cr, rd8(b | at));
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    cyc(64);
    wr8(hi, v);
}

/*
 * @routine 07:D055 StuntResultsCount
 * WHAT IT DOES: Counts the Stunt Trax results up a step a frame: while
 * the score's high byte $19EC is not $40, the count $0E80 one more (BCD)
 * up to it, shown at $7F:956F/$9571; at $40 the word spelled by $D9, $CE,
 * $DB, $CF, $CE, $CC, $DD there and the place row cycling; then the count
 * $0E81 up to the race time $0E70, shown at $7F:95EF/$95F1; with both
 * reached, sound $3F and the next step. $0D23 = 2.
 * HOW: 8-bit; the counts in decimal mode. Reached only by
 * unit_results_screen.
 * Entry: any widths. Exit: M 8-bit.
 */
void StuntResultsCount(Carried *cr)
{
    static const uint8_t word[7] = {0xD9, 0xCE, 0xDB, 0xCF, 0xCE, 0xCC, 0xDD};
    uint32_t b = (uint32_t)cr->db << 16;
    unsigned i;
    uint8_t v;

    r_sep(cr, P_M);
    lda8_imm(cr, 0x02);
    cyc(62);
    wr8(b | 0x0D23, 0x02);
    cyc(24);
    r_lda8(cr, b | 0x19EC);
    v = (uint8_t)cr->a;
    cmp8_imm(cr, 0x40);
    if (v == 0x40) {
        cyc(38);
        for (i = 0; i < 7; i++) {
            lda8_imm(cr, word[i]);
            cyc(48);
            wr8((uint32_t)(0x7F956F + 2 * i), word[i]);
        }
        palette_cycle(cr, 0x0199, 0x0007);
    } else {
        cyc(56);
        uint8_t m = rd8(b | 0x0E80);

        cr->p = (uint8_t)((nz8(cr->p, (uint8_t)(v - m)) & ~P_C) | (v >= m ? P_C : 0));
        if (v != m) {
            count_digits(cr, b, 0x0E80, 0x7F956F, 0x7F9571);
            cyc(42);
            return;
        } else { cyc(22); }
    }
    cyc(24);
    r_lda8(cr, b | 0x0E70);
    v = (uint8_t)cr->a;
    {
        cyc(24);
        uint8_t m = rd8(b | 0x0E81);

        cr->p = (uint8_t)((nz8(cr->p, (uint8_t)(v - m)) & ~P_C) | (v >= m ? P_C : 0));
        if (v != m) {
            count_digits(cr, b, 0x0E81, 0x7F95EF, 0x7F95F1);
            cyc(42);
            return;
        }
    }
    cyc(100);                           /* BEQ taken, LDA #, JSL */
    lda8_imm(cr, 0x3F);
    SoundPort0QueuePushRawWide_cr(cr);
    inc8_mem(cr, b | 0x0D59);
    cyc(42);
}

void abi_StuntResultsCount(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    StuntResultsCount(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* One character of lap Y's split ($0EDE + Y: hundredths, seconds, minutes)
   at the digit position $0E43, by the character $0E3F (0-12, even): the
   minutes' digit, ':', the seconds' two, '.', the hundredths' two; the
   last moves the position to the next line ($74 on, $F4 after the fourth
   lap's) with sound $3F at the end ($0D59 = $B) and the step on. */
static void lap_char(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint16_t y = cpu.y, at;
    uint8_t v;
    unsigned tail = 86;

    cyc(48);
    cpu.x = rd8(b | 0x0E3F);
    cpu.x |= (uint16_t)(rd8(b | 0x0E40) << 8);
    cpu.p = nz16(cpu.p, cpu.x);
    switch (cpu.x) {
    case 0x00:
        cyc(70);
        cpu.x = rd16(b | 0x0E43);
        cpu.p = nz16(cpu.p, cpu.x);
        cyc(30);
        op_lda8(b | (uint16_t)(0x0EE0 + y));
        cpu.p &= (uint8_t)~P_C;
        v = op_adc8((uint8_t)cpu.a, 0xC0, &cpu.p);
        break;
    case 0x02:
        cyc(70);
        cpu.x = rd16(b | 0x0E43);
        cpu.p = nz16(cpu.p, cpu.x);
        v = 0xE6;
        cpu.p = nz8(cpu.p, v);
        tail = 70;                      /* LDA #, BRA, STA long */
        break;
    case 0x04:
        cyc(70);
        cpu.x = rd16(b | 0x0E43);
        cpu.p = nz16(cpu.p, cpu.x);
        cyc(30);
        op_lda8(b | (uint16_t)(0x0EDF + y));
        {
            uint8_t t = (uint8_t)cpu.a;
            unsigned i;

            for (i = 0; i < 4; i++) {
                cyc(17);
                cpu.p = (uint8_t)((cpu.p & ~P_C) | (t & 1));
                t >>= 1;
            }
            v = low_digit_tile_g(t);
        }
        break;
    case 0x06:
        cyc(70);
        cpu.x = rd16(b | 0x0E43);
        cpu.p = nz16(cpu.p, cpu.x);
        cyc(30);
        op_lda8(b | (uint16_t)(0x0EDF + y));
        cyc(12);
        v = low_digit_tile_g((uint8_t)cpu.a);
        break;
    case 0x08:
        cyc(70);
        cpu.x = rd16(b | 0x0E43);
        cpu.p = nz16(cpu.p, cpu.x);
        v = 0xE7;
        cpu.p = nz8(cpu.p, v);
        tail = 70;
        break;
    case 0x0A:
        cyc(70);
        cpu.x = rd16(b | 0x0E43);
        cpu.p = nz16(cpu.p, cpu.x);
        cyc(30);
        op_lda8(b | (uint16_t)(0x0EDE + y));
        v = high_digit_tile_g((uint8_t)cpu.a);
        break;
    case 0x0C:
        cyc(70);
        cpu.x = rd16(b | 0x0E43);
        cpu.p = nz16(cpu.p, cpu.x);
        cyc(30);
        op_lda8(b | (uint16_t)(0x0EDE + y));
        v = low_digit_tile_g((uint8_t)cpu.a);
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
        cyc(76);
        wr8((uint32_t)(TILES + cpu.x), v);
        cyc(24);
        op_lda8(b | 0x0D59);
        v = (uint8_t)(cpu.a + 1);
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
        cpu.p = nz8(cpu.p, v);
        cyc(38);
        wr8(b | 0x0D59, v);
        cyc(30);
        cmp8_imm_g(0x0B);
        if (v == 0x0B) {
            cyc(34);
            lda8_imm_g(0x3F);
            (cyc(8), abi_SoundPort0QueuePushRaw());
            at = 0x00F4;
        } else
            at = 0x0074;
        op_rep(P_M);
        cyc(32);
        op_lda16(b | 0x0E43);
        op_add16(at);
        cyc(at == 0x00F4 ? 84 : 62);    /* CLC, ADC #; the sound path: BRA */
        wr16(b | 0x0E43, cpu.a);
        cyc(24);
        wr16(b | 0x0E3F, 0x0000);
        cyc(42);
        return;
    default:
        jump_routine(0x070000 | rd16(0x07D191 + cpu.x));
        return;
    }
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cyc(tail);
    wr8((uint32_t)(TILES + cpu.x), v);
    inc8_mem_g(b | 0x0E3F);
    inc8_mem_g(b | 0x0E3F);
    inc8_mem_g(b | 0x0E43);
    inc8_mem_g(b | 0x0E43);
    cyc(42);
}

static void lap_char_c(Carried *cr)
{
    cpu.a = cr->a;
    cpu.x = cr->x;
    cpu.y = cr->y;
    cpu.d = cr->d;
    cpu.p = cr->p;
    cpu.db = cr->db;
    lap_char();
    cr->a = cpu.a;
    cr->x = cpu.x;
    cr->y = cpu.y;
    cr->d = cpu.d;
    cr->p = cpu.p;
    cr->db = cpu.db;
}

/* SEP #$20, REP #$10, Y = the lap's offset, lap_char. */
static void lap_chars(Carried *cr, uint16_t y)
{
    r_sep(cr, P_M);
    r_rep(cr, P_X);
    cr->y = y;
    cr->p = nz16(cr->p, y);
    (cyc(22), lap_char_c(cr));
}

/*
 * @routine 07:D0F5 LapCharsLap1
 * WHAT IT DOES: $0D23 = 2, then one character of the first lap's split
 * ($0EDE-$0EE0) at the digit position $0E43, by the character $0E3F: the
 * minutes, ':', the seconds' two digits, '.', the hundredths' two; after
 * the last the position moves to the next line and the step goes on.
 * HOW: JMP ($D191,X) on $0E3F (16-bit X); digits are tile $C0 + the digit.
 * Entry: any widths. Exit: as the character leaves it.
 */
void LapCharsLap1(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;

    r_sep(cr, P_M);
    r_rep(cr, P_X);
    lda8_imm(cr, 0x02);
    cyc(62);
    wr8(b | 0x0D23, 0x02);
    cr->y = 0x0000;
    cr->p = nz16(cr->p, 0);
    lap_char_c(cr);
}

void abi_LapCharsLap1(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    LapCharsLap1(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:D107 LapCharsLap2
 * WHAT IT DOES: As LapCharsLap1 for the second lap's split ($0EE1).
 * HOW: As LapCharsLap1, Y = 3.
 * Entry: any widths. Exit: as LapCharsLap1.
 */
void LapCharsLap2(Carried *cr)
{
    lap_chars(cr, 0x0003);
}

void abi_LapCharsLap2(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    LapCharsLap2(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:D114 LapCharsLap3
 * WHAT IT DOES: As LapCharsLap1 for the third lap's split ($0EE4).
 * HOW: As LapCharsLap1, Y = 6.
 * Entry: any widths. Exit: as LapCharsLap1.
 */
void LapCharsLap3(Carried *cr)
{
    lap_chars(cr, 0x0006);
}

void abi_LapCharsLap3(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    LapCharsLap3(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:D121 LapCharsTotal
 * WHAT IT DOES: As LapCharsLap1 for the split at $0EE7; its last
 * character ends the laps ($0D59 = $B: sound $3F, the position $F4 on).
 * HOW: As LapCharsLap1, Y = 9.
 * Entry: any widths. Exit: as LapCharsLap1.
 */
void LapCharsTotal(Carried *cr)
{
    lap_chars(cr, 0x0009);
}

void abi_LapCharsTotal(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    LapCharsTotal(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:D12E RaceTimeTiles
 * WHAT IT DOES: Shows the race time at $0EE7-$0EE9 (hundredths, seconds,
 * minutes, BCD) at $7F:9765-$9771 and keeps it for car $0D48 (three bytes
 * at $0EF9 + $0D48 * 3); the next step.
 * HOW: 8-bit; digits are tile $C0 + the digit.
 * Entry: any widths. Exit: M 8-bit, X 8-bit.
 */
void RaceTimeTiles(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;
    uint8_t d;

    r_sep(cr, P_M);
    cyc(46);
    wr8(0x7F9765, cyc_then(76, low_digit_tile(cr, rd8(b | 0x0EE9))));
    cyc(24);
    wr8(0x7F9769, cyc_then(64, high_digit_tile(cr, rd8(b | 0x0EE8))));
    cyc(24);
    wr8(0x7F976B, cyc_then(76, low_digit_tile(cr, rd8(b | 0x0EE8))));
    cyc(24);
    wr8(0x7F976F, cyc_then(64, high_digit_tile(cr, rd8(b | 0x0EE7))));
    cyc(24);
    d = low_digit_tile(cr, rd8(b | 0x0EE7));
    cr->a = (uint16_t)((cr->a & 0xFF00) | d);
    cyc(76);
    wr8(0x7F9771, d);
    r_sep(cr, P_M | P_X);
    cyc(24);
    r_lda8(cr, b | 0x0D48);
    d = (uint8_t)cr->a;
    cr->p &= (uint8_t)~P_C;
    cyc(52);
    d = alu_adc8((uint8_t)(d << 1), rd8(b | 0x0D48), &cr->p);
    cr->a = (uint16_t)((cr->a & 0xFF00) | d);
    cr->x = d;
    cr->p = nz8(cr->p, d);
    cyc(38);
    r_lda8(cr, b | 0x0EE7);
    cyc(30);
    wr8(b | (uint16_t)(0x0EF9 + cr->x), (uint8_t)cr->a);
    cyc(24);
    r_lda8(cr, b | 0x0EE8);
    cyc(30);
    wr8(b | (uint16_t)(0x0EFA + cr->x), (uint8_t)cr->a);
    cyc(24);
    r_lda8(cr, b | 0x0EE9);
    cyc(30);
    wr8(b | (uint16_t)(0x0EFB + cr->x), (uint8_t)cr->a);
    inc8_mem(cr, b | 0x0D59);
    cyc(42);
}

void abi_RaceTimeTiles(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RaceTimeTiles(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* SEP #$20, REP #$10, TilemapPaletteFill on y entries from x with the
   palette bits f at $F1. */
static void palette_fill(Carried *cr, uint16_t x, uint16_t y, uint8_t f, unsigned t)
{
    r_sep(cr, P_M);
    r_rep(cr, P_X);
    cr->x = x;
    cr->p = nz16(cr->p, x);
    cr->y = y;
    cr->p = nz16(cr->p, y);
    lda8_imm(cr, f);
    cyc(t);
    wr8(0x0000F1, f);
    w_TilemapPaletteFill(cr);
}

/*
 * @routine 07:D232 ResultsBlinkWait
 * WHAT IT DOES: Counts $0E3F up; below $3C the row at $38F (7 entries)
 * cycles its palette; at $3C it clears $0E3F, goes to the next step with
 * sound $28 on port 0 and gives the row palette 0.
 * HOW: 8-bit; TilemapPaletteCycle or TilemapPaletteFill.
 * Entry: M 8-bit. Exit: as the palette routine leaves it.
 */
void ResultsBlinkWait(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;

    inc8_mem(cr, b | 0x0E3F);
    cyc(24);
    r_lda8(cr, b | 0x0E3F);
    cmp8_imm(cr, 0x3C);
    if ((uint8_t)cr->a < 0x3C) {
        cyc(60);
        r_sep(cr, P_M);
        palette_cycle(cr, 0x038F, 0x0007);
        cyc(42);
        return;
    }
    cyc(56);
    wr8(b | 0x0E3F, 0x00);
    inc8_mem(cr, b | 0x0D59);
    cyc(24);
    lda8_imm(cr, 0x28);
    SoundPort0QueuePushRaw_cr(cr);
    palette_fill(cr, 0x038F, 0x0007, 0x00, 94);
    cyc(42);
}

void abi_ResultsBlinkWait(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ResultsBlinkWait(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* The digit tiles of the byte at `at` (read for each): its high nibble at
   hi and low nibble at lo. */
static void bcd_tiles(Carried *cr, uint32_t at, uint32_t hi, uint32_t lo)
{
    cyc(24);
    wr8(hi, cyc_then(64, high_digit_tile(cr, rd8(at))));
    cyc(24);
    wr8(lo, cyc_then(76, low_digit_tile(cr, rd8(at))));
}

/* A time's minutes digit at `at`: blank (tile $0F, with bit 0 of the
   attribute byte after it set) for 0, else the digit with attribute bits
   other than 0, 2-4 cleared ($E2 kept). */
static void minutes_tile(Carried *cr, uint8_t mins, uint32_t at)
{
    uint8_t t = high_digit_tile(cr, mins), v;

    cr->a = (uint16_t)((cr->a & 0xFF00) | t);
    cmp8_imm(cr, 0xC0);
    if (t == 0xC0) {
        lda8_imm(cr, 0x0F);
        cyc(112);
        wr8(at, 0x0F);
        cyc(32);
        v = (uint8_t)(rd8(at + 1) | 0x01);
        lda8_imm(cr, v);
        cyc(48);
        wr8(at + 1, v);
        cyc(22);                        /* BRA */
    } else {
        cyc(102);
        wr8(at, t);
        cyc(32);
        v = (uint8_t)(rd8(at + 1) & 0xE2);
        lda8_imm(cr, v);
        cyc(48);
        wr8(at + 1, v);
    }
}

/*
 * @routine 07:D3F6 ResultsTotalTiles
 * WHAT IT DOES: Shows the total time at $0EED-$0EEF (hundredths, seconds,
 * minutes, BCD) at $7F:95F7-$9605: the minutes' tens (blank for 0), units,
 * ':', the seconds, '.', the hundredths.
 * HOW: 8-bit; digits are tile $C0 + the digit.
 * Entry: M 8-bit. Exit: A low byte = the last tile.
 */
void ResultsTotalTiles(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;
    uint8_t v;

    cyc(24);
    minutes_tile(cr, rd8(b | 0x0EEF), 0x7F95F7);
    cyc(24);
    wr8(0x7F95F9, cyc_then(76, low_digit_tile(cr, rd8(b | 0x0EEF))));
    lda8_imm(cr, 0xE6);
    cyc(48);
    wr8(0x7F95FB, 0xE6);
    bcd_tiles(cr, b | 0x0EEE, 0x7F95FD, 0x7F95FF);
    lda8_imm(cr, 0xE7);
    cyc(48);
    wr8(0x7F9601, 0xE7);
    cyc(24);
    wr8(0x7F9603, cyc_then(64, high_digit_tile(cr, rd8(b | 0x0EED))));
    cyc(24);
    v = low_digit_tile(cr, rd8(b | 0x0EED));
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    cyc(76);
    wr8(0x7F9605, v);
    cyc(42);
}

void abi_ResultsTotalTiles(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ResultsTotalTiles(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* A decimal-mode byte at `at` less m with borrow (SEC first when sec). */
static uint8_t bcd_sub(Carried *cr, uint32_t b, uint16_t at, uint8_t m, int sec)
{
    uint8_t v;

    cyc(24);
    r_lda8(cr, b | at);
    if (sec)
        { cyc(14); cr->p |= P_C; }
    v = alu_sbc8((uint8_t)cr->a, m, &cr->p);
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    cyc(24);
    wr8(b | at, v);
    return v;
}

/* A decimal-mode byte at `at` plus m with carry (CLC first when clc). */
static uint8_t bcd_add(Carried *cr, uint32_t b, uint16_t at, uint8_t m, int clc)
{
    uint8_t v;

    cyc(8);
    r_lda8(cr, b | at);
    if (clc)
        { cyc(14); cr->p &= (uint8_t)~P_C; }
    v = alu_adc8((uint8_t)cr->a, m, &cr->p);
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    cyc(40);
    wr8(b | at, v);
    return v;
}

/* A decimal-mode byte at `at` plus the byte at `from` with carry: LDA at,
   (CLC,) ADC from, STA at. */
static void bcd_add_mem(Carried *cr, uint32_t b, uint16_t at, uint16_t from, int clc)
{
    uint8_t v;

    cyc(24);
    r_lda8(cr, b | at);
    if (clc)
        { cyc(14); cr->p &= (uint8_t)~P_C; }
    cyc(24);
    v = alu_adc8((uint8_t)cr->a, rd8(b | from), &cr->p);
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    cyc(24);
    wr8(b | at, v);
}

/* The bonus time $0EEA-$0EEC moved by `lo`, `mid` (BCD hundredths and
   seconds) into the total $0EED-$0EEE. */
static void bonus_move(Carried *cr, uint32_t b, uint8_t lo, uint8_t mid)
{
    bcd_sub(cr, b, 0x0EEA, lo, 1);
    bcd_sub(cr, b, 0x0EEB, mid, 0);
    cyc(16);
    bcd_add(cr, b, 0x0EED, lo, 1);
    cyc(16);
    bcd_add(cr, b, 0x0EEE, mid, 0);
}

/*
 * @routine 07:D268 ResultsBonusCount
 * WHAT IT DOES: Counts the time bonus $0EEA-$0EEC into the total
 * $0EED-$0EEF (BCD) a step a frame: $0E3F counts the frames (at most $FA);
 * from frame $12 B or START adds all of it at once (the step on, $10A2 =
 * 2, sound $3F); otherwise a step of 1, $57 or $73 hundredths or 2.17
 * seconds by the bonus's size, the step on with sound $3F when the
 * hundredths run out; seconds past 60 carry into the minutes; then the
 * bonus's digits at $7F:9765-$9771 and the total's (ResultsTotalTiles).
 * HOW: Decimal mode (SED ... CLD). B or START, the minutes' carry and the
 * bonus of a minute or more are not reached in the traces so far.
 * Entry: M 8-bit. Exit: M 8-bit; as ResultsTotalTiles leaves it.
 */
void ResultsBonusCount(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;
    uint8_t v;

    cyc(24);
    r_lda8(cr, b | 0x0E3F);
    v = (uint8_t)(cr->a + 1);
    lda8_imm(cr, v);
    cmp8_imm(cr, 0xFA);
    if (v >= 0xFA)
        { cyc(10); lda8_imm(cr, 0xFA); }
    cyc(76);
    wr8(b | 0x0E3F, (uint8_t)cr->a);
    cyc(32);
    cmp8_imm(cr, 0x12);
    if ((uint8_t)cr->a >= 0x12 && (cyc(22), pressed(cr, b, 0x9000))) {
        cyc(36);
        r_sep(cr, P_M);
        cr->p |= P_D;
        bcd_add_mem(cr, b, 0x0EED, 0x0EEA, 1);
        cyc(24);
        r_lda8(cr, b | 0x0EEE);
        cyc(24);
        v = alu_adc8((uint8_t)cr->a, rd8(b | 0x0EEB), &cr->p);
        cr->a = (uint16_t)((cr->a & 0xFF00) | v);
        if (cr->p & P_C) {
            cr->p |= P_C;
            v = alu_sbc8(v, 0x60, &cr->p);
            cr->a = (uint16_t)((cr->a & 0xFF00) | v);
            cyc(54);
            wr8(b | 0x0EEE, v);
            cyc(16);
            bcd_add(cr, b, 0x0EEF, 0x01, 1);
            cyc(24);
            r_lda8(cr, b | 0x0EEE);
        } else { cyc(22); }
        cyc(24);
        wr8(b | 0x0EEE, (uint8_t)cr->a);
        cmp8_imm(cr, 0x60);
        if ((uint8_t)cr->a >= 0x60) {
            v = alu_sbc8((uint8_t)cr->a, 0x60, &cr->p);
            cr->a = (uint16_t)((cr->a & 0xFF00) | v);
            cyc(56);
            wr8(b | 0x0EEE, v);
            cyc(16);
            bcd_add(cr, b, 0x0EEF, 0x01, 1);
        } else { cyc(38); }
        bcd_add_mem(cr, b, 0x0EEF, 0x0EEC, 0);
        cyc(24);
        wr8(b | 0x0EEA, 0x00);
        cyc(24);
        wr8(b | 0x0EEB, 0x00);
        cyc(24);
        wr8(b | 0x0EEC, 0x00);
        inc8_mem(cr, b | 0x0D59);
        lda8_imm(cr, 0x02);
        cyc(40);
        wr8(b | 0x10A2, 0x02);
        cyc(24);
        lda8_imm(cr, 0x3F);
        SoundPort0QueuePushRawX8_cr(cr);
        cyc(24);                        /* JMP */
        goto tiles;
    }
    r_sep(cr, P_M);
    cr->p |= P_D;
    cyc(66);
    r_lda8(cr, b | 0x0EEC);
    if ((uint8_t)cr->a) {
        cyc(46);
        bcd_sub(cr, b, 0x0EEA, 0x17, 1);
        bcd_sub(cr, b, 0x0EEB, 0x02, 0);
        if (!(cr->p & P_C)) {
            cr->p &= (uint8_t)~P_C;
            v = alu_adc8((uint8_t)cr->a, 0x60, &cr->p);
            cr->a = (uint16_t)((cr->a & 0xFF00) | v);
            cyc(70);
            wr8(b | 0x0EEB, v);
            bcd_sub(cr, b, 0x0EEC, 0x01, 1);
        } else { cyc(22); }
        cyc(16);
        bcd_add(cr, b, 0x0EED, 0x17, 1);
        cyc(16);
        bcd_add(cr, b, 0x0EEE, 0x02, 0);
    } else {
        cyc(46);
        r_lda8(cr, b | 0x0EEB);
        v = (uint8_t)cr->a;
        if (!v) {
            cyc(22);
            if (!bcd_sub(cr, b, 0x0EEA, 0x01, 1)) {
                cyc(16);
                inc8_mem(cr, b | 0x0D59);
                lda8_imm(cr, 0x02);
                cyc(40);
                wr8(b | 0x10A2, 0x02);
                cyc(24);
                lda8_imm(cr, 0x3F);
                SoundPort0QueuePushRaw_cr(cr);
            } else { cyc(30); }
            cyc(8);
            bcd_add(cr, b, 0x0EED, 0x01, 1);
            cyc(16);
            bcd_add(cr, b, 0x0EEE, 0x00, 0);
        } else {
            cyc(54);
            cmp8_imm(cr, 0x01);
            if (v == 0x01)
                bonus_move(cr, b, 0x73, 0x00);
            else
                { cyc(18); bonus_move(cr, b, 0x57, 0x00); }
        }
        cyc(22);
    }
    cyc(24);
    r_lda8(cr, b | 0x0EEE);
    cmp8_imm(cr, 0x60);
    if ((uint8_t)cr->a >= 0x60) {
        v = alu_sbc8((uint8_t)cr->a, 0x60, &cr->p);
        cr->a = (uint16_t)((cr->a & 0xFF00) | v);
        cyc(56);
        wr8(b | 0x0EEE, v);
        cyc(30);                        /* CLC, LDA */
        cr->p &= (uint8_t)~P_C;
        bcd_add(cr, b, 0x0EEF, 0x01, 0);
    } else { cyc(38); }
tiles:
    cr->p &= (uint8_t)~P_D;
    cyc(38);
    wr8(0x7F9765, cyc_then(76, low_digit_tile(cr, rd8(b | 0x0EEC))));
    bcd_tiles(cr, b | 0x0EEB, 0x7F9769, 0x7F976B);
    bcd_tiles(cr, b | 0x0EEA, 0x7F976F, 0x7F9771);
    ResultsTotalTiles(cr);
}

void abi_ResultsBonusCount(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ResultsBonusCount(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* X = car $0D48 * 3 (CLC, ADC, ADC; 8-bit). */
static uint8_t car_times(Carried *cr, uint32_t b)
{
    uint8_t c;

    cr->p &= (uint8_t)~P_C;
    cyc(38);
    r_lda8(cr, b | 0x0D48);
    cyc(24);
    c = alu_adc8((uint8_t)cr->a, rd8(b | 0x0D48), &cr->p);
    cyc(24);
    c = alu_adc8(c, rd8(b | 0x0D48), &cr->p);
    cr->a = (uint16_t)((cr->a & 0xFF00) | c);
    cr->x = c;
    cr->p = nz8(cr->p, c);
    return c;
}

/*
 * @routine 07:D471 ResultsRecordCompare
 * WHAT IT DOES: Compares the total $0EED-$0EEF with car $0D48's record time
 * ($0F76 + car * 3): when slower the row at $221 gets palette 4; when
 * faster (or equal is not) the record's row at $361 palette 4 (if there is
 * a record) and the total's row at $221 cycles; B or START starts the fade
 * out ($0E3A = 1); $10A2 = 3 when it was 1.
 * HOW: SEP #$30; byte compares from the minutes; TilemapPaletteFill and
 * TilemapPaletteCycle. Slower and equal are not reached in the traces so
 * far.
 * Entry: any widths. Exit: M 8-bit, X 16-bit or 8-bit.
 */
void ResultsRecordCompare(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;
    static const uint16_t at[3] = {0x0F78, 0x0F77, 0x0F76};
    static const uint16_t mine[3] = {0x0EEF, 0x0EEE, 0x0EED};
    uint8_t x, v, m;
    int r = 0;
    unsigned i, last = 3;

    r_sep(cr, P_M | P_X);
    x = car_times(cr, b);
    for (i = 0; i < 3; i++) {
        cyc(38);
        r_lda8(cr, b | mine[i]);
        v = (uint8_t)cr->a;
        cyc(24);
        m = rd8(b | (uint16_t)(at[i] + x));
        cr->p = (uint8_t)((nz8(cr->p, (uint8_t)(v - m)) & ~P_C) | (v >= m ? (cyc(18), P_C) : 0));
        if (v < m) {
            r = -1;
            break;
        }
        if (v != m) {
            r = 1;
            last = i;
            break;
        }
    }
    cyc(r > 0 && last == 2 ? 14 : 20);  /* BCC, BEQ not taken at the last compare */
    if (r > 0) {
        palette_fill(cr, 0x0221, 0x0008, 0x10, 102);
        cyc(22);                        /* BRA */
    } else if (r < 0) {
        cyc(26);
        r_lda8(cr, b | (uint16_t)(0x0F78 + x));
        cmp8_imm(cr, 0xFF);
        if ((uint8_t)cr->a != 0xFF)
            { cyc(32); palette_fill(cr, 0x0361, 0x0008, 0x10, 102); } else { cyc(38); }
        cyc(22);
        r_sep(cr, P_M);
        palette_cycle(cr, 0x0221, 0x0008);
    }
    cyc(22);
    if (pressed(cr, b, 0x9000)) {
        r_sep(cr, P_M);
        lda8_imm(cr, 0x01);
        cyc(62);
        wr8(b | 0x0E3A, 0x01);
    } else { cyc(6); }
    r_sep(cr, P_M);
    cyc(46);
    r_lda8(cr, b | 0x10A2);
    cyc(38);
    cmp8_imm(cr, 0x07);
    if ((uint8_t)cr->a == 0x07)
        { cyc(42); return; } else { cyc(32); }
    cmp8_imm(cr, 0x01);
    if ((uint8_t)cr->a != 0x01)
        { cyc(42); return; }
    lda8_imm(cr, 0x03);
    cyc(34);
    wr8(b | 0x10A2, 0x03);
    cyc(42);
}

void abi_ResultsRecordCompare(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ResultsRecordCompare(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:D4F8 RecordTimeTiles
 * WHAT IT DOES: Shows car $0D48's record time ($0F76 + car * 3) at
 * $7F:9737-$9745: the minutes (blank tens for 0), ':', the seconds, '.',
 * the hundredths.
 * HOW: SEP #$10; 8-bit. Reached only by unit_results_screen.
 * Entry: M 8-bit. Exit: X 8-bit.
 */
void RecordTimeTiles(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;
    uint8_t x, t, v;

    r_sep(cr, P_X);
    x = car_times(cr, b);
    cyc(38);
    t = high_digit_tile(cr, rd8(b | (uint16_t)(0x0F78 + x)));
    cr->a = (uint16_t)((cr->a & 0xFF00) | t);
    cmp8_imm(cr, 0xC0);
    if (t == 0xC0) {
        lda8_imm(cr, 0x0F);
        cyc(112);
        wr8(0x7F9737, 0x0F);
        cyc(32);
        v = (uint8_t)(rd8(0x7F9738) | 0x01);
        lda8_imm(cr, v);
        cyc(48);
        wr8(0x7F9738, v);
    } else {
        cyc(102);
        wr8(0x7F9737, t);
    }
    cyc(t == 0xC0 ? 46 : 24);           /* BRA, or not; LDA abs,X */
    wr8(0x7F9739, cyc_then(76, low_digit_tile(cr, rd8(b | (uint16_t)(0x0F78 + x)))));
    lda8_imm(cr, 0xE6);
    cyc(48);
    wr8(0x7F973B, 0xE6);
    bcd_tiles(cr, b | (uint16_t)(0x0F77 + x), 0x7F973D, 0x7F973F);
    lda8_imm(cr, 0xE7);
    cyc(48);
    wr8(0x7F9741, 0xE7);
    cyc(24);
    wr8(0x7F9743, cyc_then(64, high_digit_tile(cr, rd8(b | (uint16_t)(0x0F76 + x)))));
    cyc(24);
    v = low_digit_tile(cr, rd8(b | (uint16_t)(0x0F76 + x)));
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    cyc(76);
    wr8(0x7F9745, v);
    cyc(42);
}

void abi_RecordTimeTiles(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RecordTimeTiles(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* SoundPort0QueuePushX8, still in registers: its call with the registers loaded and taken
   back. */
static void w_SoundPort0QueuePushX8(Carried *cr)
{
    cpu.a = cr->a;
    cpu.x = cr->x;
    cpu.y = cr->y;
    cpu.d = cr->d;
    cpu.p = cr->p;
    cpu.db = cr->db;
    abi_SoundPort0QueuePushX8();
    cr->a = cpu.a;
    cr->x = cpu.x;
    cr->y = cpu.y;
    cr->d = cpu.d;
    cr->p = cpu.p;
    cr->db = cpu.db;
}

/* TilemapPaletteCycle, still in registers: its call with the registers loaded and taken
   back. */
static void w_TilemapPaletteCycle(Carried *cr)
{
    cpu.a = cr->a;
    cpu.x = cr->x;
    cpu.y = cr->y;
    cpu.d = cr->d;
    cpu.p = cr->p;
    cpu.db = cr->db;
    abi_TilemapPaletteCycle();
    cr->a = cpu.a;
    cr->x = cpu.x;
    cr->y = cpu.y;
    cr->d = cpu.d;
    cr->p = cpu.p;
    cr->db = cpu.db;
}

/* SpritePieces3 as its adapter leaves the registers. */
static void w_SpritePieces3(Carried *cr)
{
    uint8_t p = cr->p, attr;
    uint16_t set;

    attr = SpritePieces3(cr->db, &cr->x, &cr->y, &p, &set);
    cr->a = (uint16_t)((set & 0xFF00) | attr);
    cr->p = p;
}

/* RecordsUnpackCourseRunning, still in registers: its call with the registers loaded and taken
   back. */
static void w_RecordsUnpackCourseRunning(Carried *cr)
{
    cpu.a = cr->a;
    cpu.x = cr->x;
    cpu.y = cr->y;
    cpu.d = cr->d;
    cpu.p = cr->p;
    cpu.db = cr->db;
    abi_RecordsUnpackCourseRunning();
    cr->a = cpu.a;
    cr->x = cpu.x;
    cr->y = cpu.y;
    cr->d = cpu.d;
    cr->p = cpu.p;
    cr->db = cpu.db;
}

/* DigitTiles1184 as its adapter leaves the registers. */
static void w_DigitTiles1184(Carried *cr)
{
    cr->a = DigitTiles1184();
    cr->p = (uint8_t)((cr->p & ~(P_N | P_Z | P_C | P_V)) | P_M);
}

/* GsuCF42To3000, still in registers: its call with the registers loaded and taken
   back. */
static void w_GsuCF42To3000(Carried *cr)
{
    cpu.a = cr->a;
    cpu.x = cr->x;
    cpu.y = cr->y;
    cpu.d = cr->d;
    cpu.p = cr->p;
    cpu.db = cr->db;
    abi_GsuCF42To3000();
    cr->a = cpu.a;
    cr->x = cpu.x;
    cr->y = cpu.y;
    cr->d = cpu.d;
    cr->p = cpu.p;
    cr->db = cpu.db;
}

/* TimeUnpackBcd as its adapter leaves the registers. */
static void w_TimeUnpackBcd(Carried *cr)
{
    uint16_t min, sec, hund;
    uint8_t p = cr->p;

    TimeUnpackBcd(cr->a, &min, &sec, &hund, &p);
    cr->a = min;
    cr->x = sec;
    cr->y = hund;
    cr->p = p;
}

/* GsuCF42To4000, still in registers: its call with the registers loaded and taken
   back. */
static void w_GsuCF42To4000(Carried *cr)
{
    cpu.a = cr->a;
    cpu.x = cr->x;
    cpu.y = cr->y;
    cpu.d = cr->d;
    cpu.p = cr->p;
    cpu.db = cr->db;
    abi_GsuCF42To4000();
    cr->a = cpu.a;
    cr->x = cpu.x;
    cr->y = cpu.y;
    cr->d = cpu.d;
    cr->p = cpu.p;
    cr->db = cpu.db;
}

/* TilemapRow207Palette4, still in registers: its call with the registers loaded and taken
   back. */
static void w_TilemapRow207Palette4(Carried *cr)
{
    cpu.a = cr->a;
    cpu.x = cr->x;
    cpu.y = cr->y;
    cpu.d = cr->d;
    cpu.p = cr->p;
    cpu.db = cr->db;
    abi_TilemapRow207Palette4();
    cr->a = cpu.a;
    cr->x = cpu.x;
    cr->y = cpu.y;
    cr->d = cpu.d;
    cr->p = cpu.p;
    cr->db = cpu.db;
}

/* TilemapRow287Palette4, still in registers: its call with the registers loaded and taken
   back. */
static void w_TilemapRow287Palette4(Carried *cr)
{
    cpu.a = cr->a;
    cpu.x = cr->x;
    cpu.y = cr->y;
    cpu.d = cr->d;
    cpu.p = cr->p;
    cpu.db = cr->db;
    abi_TilemapRow287Palette4();
    cr->a = cpu.a;
    cr->x = cpu.x;
    cr->y = cpu.y;
    cr->d = cpu.d;
    cr->p = cpu.p;
    cr->db = cpu.db;
}

/* TilemapRow187Palette4, still in registers: its call with the registers loaded and taken
   back. */
static void w_TilemapRow187Palette4(Carried *cr)
{
    cpu.a = cr->a;
    cpu.x = cr->x;
    cpu.y = cr->y;
    cpu.d = cr->d;
    cpu.p = cr->p;
    cpu.db = cr->db;
    abi_TilemapRow187Palette4();
    cr->a = cpu.a;
    cr->x = cpu.x;
    cr->y = cpu.y;
    cr->d = cpu.d;
    cr->p = cpu.p;
    cr->db = cpu.db;
}

/* SelectionSave as its adapter leaves the registers. */
static void w_SelectionSave(Carried *cr)
{
    uint16_t x;

    cr->a = SelectionSave(cr->db, &x);
    cr->x = x;
    if (cr->p & P_X) {
        cr->x &= 0x00FF;
        cr->y &= 0x00FF;
    }
}

/* OamTextLine as its adapter leaves the registers. */
static void w_OamTextLine(Carried *cr)
{
    uint16_t x, y, a;

    OamTextLine(cr->db, &x, &y, &a);
    cr->a = a;
    cr->x = cr->p & P_X ? (uint16_t)(x & 0xFF) : x;
    cr->y = cr->p & P_X ? (uint16_t)(y & 0xFF) : y;
    cyc(86);
}

/* TilemapPaletteFill as its adapter leaves the registers. */
static void w_TilemapPaletteFill(Carried *cr)
{
    TilemapPaletteFill(&cr->x, &cr->y, &cr->a, 78);
    cr->p = (uint8_t)((cr->p & ~P_N) | P_Z);
}
