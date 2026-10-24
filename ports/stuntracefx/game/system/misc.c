/*
 * misc.c: small routines of the frame code that have no other home yet.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "hw.h"
#include "carried.h"
#include "wram_names.h"
#include "wram_struct.h"

uint16_t CourseDataPointer(uint8_t *bank, uint8_t *p);
uint8_t GsuRun(uint8_t bank, uint16_t start);
uint16_t ObjectLinkedSet1A(uint16_t obj, uint16_t value);
uint16_t ObjectPlaceAll(uint8_t bank);
void abi_AttractCourseSetup(void);
void abi_DemoRun(void);
void abi_DemoRunThirdRest(void);
void abi_KeepRegisters(void);

/*
 * @routine 7E:E18B DataBankTest
 * WHAT IT DOES: Leaves every register as it was except N and Z, which it
 * sets from the data bank.
 * HOW: Pushes DB and PB, pulls PB into DB and then the old DB back.
 * Entry: any widths. Exit: DB unchanged; N, Z from DB.
 */
void DataBankTest(void)
{
}

void ChampionshipFinish(Carried *cr);
void Countdown0DF8_cr(Carried *cr);
void DataBankTest_cr(Carried *cr);
void DelayLoop13(Carried *cr);
void DemoRun(Carried *cr);
void DemoRunFirst(Carried *cr);
void DemoRunFourth(Carried *cr);
void DemoRunSecond(Carried *cr);
void DemoRunThird(Carried *cr);
void DemoRunThirdRest(Carried *cr);
void ReadTable9022_cr(Carried *cr);
void Set0D37FromTable_cr(Carried *cr);
void Set7EBC2BTrio_cr(Carried *cr);
void Set7EFFA6Sequence_cr(Carried *cr);
void WordCopyStep_cr(Carried *cr);
void WramFillZero_cr(Carried *cr);
void WramFillZeroLong(Carried *cr);
static void demo_blank(Carried *cr, uint32_t b);
static void demo_camera(Carried *cr, uint8_t db);
static int demo_fade_done(Carried *cr);
static void demo_fade_out(Carried *cr);
static void demo_lda8(Carried *cr, uint8_t v);
static void demo_next(Carried *cr, uint16_t n);
static void demo_next_before(Carried *cr, uint16_t n);
static uint32_t dp_addr(Carried *cr, uint8_t a);
static void w_CourseDataPointer(Carried *cr);
static void w_GsuRun(Carried *cr);
static void w_KeepRegisters(Carried *cr);
static void w_ObjectLinkedSet1A(Carried *cr);
static void w_ObjectPlaceAll(Carried *cr);
void ObjectsRunHandlers(Carried *cr);
static void w_ObjectsRunHandlers(Carried *cr);
void PlayerStatsAndPaletteAnimate(Carried *cr);
static void w_PlayerStatsAndPaletteAnimate(Carried *cr);
void AttractCourseSetup(Carried *cr);
void BootSoftRestart(Carried *cr);
void CameraFollowStep_cr(Carried *cr);
void ChampionScreenRun(Carried *cr);
void CourseScriptRun(Carried *cr);
void ObjectFaceTied_cr(Carried *cr);
void OverlayFramesUntilFaded(Carried *cr);
void RaceFrameRun(Carried *cr);
void RaceVideoInit_cr(Carried *cr);
void RecordTimeInsert_cr(Carried *cr);
void RecordsUnpackCourseRunning(Carried *cr);
void SoftResetToBootWarm(Carried *cr);
void WindowsInit_cr(Carried *cr);

void DataBankTest_cr(Carried *cr)
{
    cyc(144);
    set_p(cr, nz8(cr->p, cr->db));
}

void abi_DataBankTest(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    DataBankTest_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 7E:D2BA DelayLoop13
 * WHAT IT DOES: Waits a short while: $13 turns of a NOP loop. Nothing
 * calls it (no JSL, pointer or table reaches it), so a direct call is its
 * only way in.
 * HOW: PHP, PHB; the data bank set to its own (PHK, PLB); SEP #$30, Y
 * counts $13 down; PLB, PLP.
 * Entry: any widths. Exit: P, A and the data bank unchanged; Y = 0; X's
 * high byte 0 (8-bit index on the way).
 */
void DelayLoop13(Carried *cr)
{
    /* NOP, DEY, BNE: 14 + 14 + 22 clocks a turn, 16 for the last BNE */
    cyc(232);
    cpu_idle(0x13u * 50u - 6u);
    cr->x &= 0x00FF;
    cr->y = 0;
}

void abi_DelayLoop13(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    DelayLoop13(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 7E:F217 KeepRegisters
 * @alias 02:CF3E
 * WHAT IT DOES: Nothing: it saves P, changes the widths and restores P.
 * HOW: PHP, SEP #$20, REP #$30, PLP.
 * Entry: any widths. Exit: every register unchanged.
 */
void KeepRegisters(void)
{
}

/* KeepRegisters on carried registers: its cycles, nothing else. */
void KeepRegisters_cr(Carried *cr)
{
    (void)cr;
    cyc(138);
}

void abi_KeepRegisters(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    KeepRegisters_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:F329 ReturnOnly
 * WHAT IT DOES: Nothing: a lone RTS that callers use as an empty handler.
 * HOW: RTS.
 * Entry: any widths. Exit: every register unchanged.
 */
void ReturnOnly(void)
{
}

/* ReturnOnly on carried registers: its cycles, nothing else. */
void ReturnOnly_cr(Carried *cr)
{
    (void)cr;
    cyc(42);
}

void abi_ReturnOnly(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ReturnOnly_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:AEEE ReadTable9022
 * WHAT IT DOES: Reads 128 words' low bytes from the table at $9022 in the
 * data bank and keeps none of them; it changes only registers and $64.
 * HOW: A loop of 128 loads stepping Y by 2, X by 1, counting down $64.
 * Entry: any widths, D = $0000. Exit: A low byte = the last byte read; X =
 * $80, Y = 0; $64 = 0; P unchanged.
 */
uint8_t ReadTable9022(uint8_t bank)
{
    uint8_t v = 0;
    unsigned i;

    for (i = 0; i < 0x80; i++)
        v = rd8((uint32_t)bank << 16 | (uint16_t)(0x9022 + 2 * i));
    cyc(108);
    wr8(W(wram_0064), 0);
    return v;
}

void ReadTable9022_cr(Carried *cr)
{
    cr->a = (uint16_t)((cr->a & 0xFF00) | ReadTable9022(cr->db));
    cr->x = 0x80;
    cr->y = 0x00;
}

void abi_ReadTable9022(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ReadTable9022_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 02:FC44 Set0D37FromTable
 * WHAT IT DOES: Sets $0D37 to the word table $02:FC58 entry for the byte at
 * $0D48.
 * HOW: 16-bit lookup; work RAM through the data bank.
 * Entry: any widths. Exit: A = the entry; X = the byte * 2 (low byte only
 * when X is 8-bit); P unchanged.
 */
uint16_t Set0D37FromTable(uint8_t bank, uint16_t *x)
{
    (void)bank;
    uint16_t v;

    cyc(68);
    *x = (uint16_t)((RD16(wram_0D48) & 0x00FF) << 1);
    cyc(124);
    v = rd16(0x02FC58 + *x);
    WR16(wram_0D37, v);
    return v;
}

void Set0D37FromTable_cr(Carried *cr)
{
    uint16_t x;

    cr->a = Set0D37FromTable(cr->db, &x);
    cr->x = (cr->p & P_X) ? (cyc(70), (uint8_t)x) : x;
}

void abi_Set0D37FromTable(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    Set0D37FromTable_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 02:D65A Set7EBC2BTrio
 * WHAT IT DOES: Stores A's low byte at $7E:BC2B, $7E:BC2D and $7E:BC2F.
 * HOW: Three long byte stores.
 * Entry: any widths. Exit: every register unchanged.
 */
void Set7EBC2BTrio(uint8_t v)
{
    cyc(76);
    wr8(W(wram_7EBC2B), v);
    cyc(32);
    wr8(W(wram_7EBC2D), v);
    cyc(32);
    wr8(W(wram_7EBC2F), v);
    cyc(72);
}

void Set7EBC2BTrio_cr(Carried *cr)
{
    Set7EBC2BTrio((uint8_t)cr->a);
}

void abi_Set7EBC2BTrio(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    Set7EBC2BTrio_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:EE11 Countdown0DF8
 * WHAT IT DOES: Counts $0DF8 down by one; once it is zero, instead counts
 * $0E5C down and clears $1160.
 * HOW: 8-bit memory decrements through the data bank.
 * Entry: any widths. Exit: A low byte = $0DF8's value before; P unchanged.
 */
uint8_t Countdown0DF8(uint8_t bank)
{
    (void)bank;
    cyc(52);
    uint8_t v = RD8(wram_0DF8);

    if (v) {
        cyc(46);
        WR8(wram_0DF8, cyc_then(6, (uint8_t)(RD8(wram_0DF8) - 1)));
        cyc(54);
        return v;
    }
    cyc(40);
    WR8(wram_0E5C, cyc_then(6, (uint8_t)(RD8(wram_0E5C) - 1)));
    cyc(24);
    WR8(wram_1160, 0x00);
    cyc(54);
    return v;
}

void Countdown0DF8_cr(Carried *cr)
{
    cr->a = (uint16_t)((cr->a & 0xFF00) | Countdown0DF8(cr->db));
    cyc(16);
}

void abi_Countdown0DF8(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    Countdown0DF8_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void abi_OverlayFramesUntilFaded(void);
void abi_RecordTimeInsert(void);
void abi_RecordsUnpackCourseRunning(void);
void abi_ChampionScreenRun(void);
void abi_BootSoftRestart(void);
void abi_SoftResetToBootWarm(void);

/*
 * @routine 07:EE26 ChampionshipFinish
 * WHAT IT DOES: The end of a championship after its last times screen:
 * $0D59 = 0, the overlay faded out (OverlayFramesUntilFaded), $0D48 = 0,
 * the run's time put in the records (RecordTimeInsert,
 * RecordsUnpackCourseRunning), the champion's screen (ChampionScreenRun);
 * then the game restarts: class 2 ($0D4A) by BootSoftRestart, the others
 * by SoftResetToBootWarm. GlyphTimesSequence jumps here.
 * HOW: Calls in turn through the data bank; JML or JMP to the restart.
 * Entry: data bank $07, D = $0000. Exit: never returns.
 */
void ChampionshipFinish(Carried *cr)
{
    uint8_t v;

    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cyc(46);
    WR8(wram_0D59, 0x00);
    (cyc(46), OverlayFramesUntilFaded(cr));
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cyc(46);
    WR8(wram_0D48, 0x00);
    (cyc(46), RecordTimeInsert_cr(cr));
    (cyc(62), RecordsUnpackCourseRunning(cr));
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    (cyc(0), ChampionScreenRun(cr));
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cyc(46);
    v = RD8(wram_0D4A);
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    set_p(cr, nz8(cr->p, (uint8_t)(v - 0x02)));
    set_p(cr, (uint8_t)(v >= 0x02 ? (cyc(64), cr->p | P_C) : (cyc(62), cr->p & ~P_C)));
    if (v == 0x02)
        BootSoftRestart(cr);
    else
        SoftResetToBootWarm(cr);
}

void abi_ChampionshipFinish(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ChampionshipFinish(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:B893 Set7EFFA6Sequence
 * WHAT IT DOES: Stores the words 1, 2, 3, 4 at $7E:FFA6-$7E:FFAD.
 * HOW: Four 16-bit long stores.
 * Entry: any widths. Exit: A = 4 (16-bit); P unchanged.
 */
void Set7EFFA6Sequence(void)
{
    unsigned i;

    cyc(44);
    for (i = 0; i < 4; i++)
        { cyc(56); wr16(W(wram_7EFFA6) + 2 * i, (uint16_t)(i + 1)); }
    cyc(72);
}

void Set7EFFA6Sequence_cr(Carried *cr)
{
    Set7EFFA6Sequence();
    cr->a = 0x0004;
}

void abi_Set7EFFA6Sequence(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    Set7EFFA6Sequence_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 02:D53D WordCopyStep
 * WHAT IT DOES: While the count at $0CF6 is non-zero, copies one word from
 * $02:D65A + $0CF2 to $07A2 + $0CF6 and steps both offsets down by 2. No
 * script has reached the copy yet: $0CF6 has always been 0.
 * HOW: 16-bit offsets and stores through the data bank; the source is a
 * long read, and each offset is stepped with two DECs.
 * Entry: any widths. Exit: when $0CF6 is 0, Y = 0 and A unchanged; else A
 * = the word copied, X and Y the offsets before the step; M 16-bit, X
 * 16-bit; N, Z from the last step (Z set when there was nothing to copy).
 */
uint16_t WordCopyStep(uint8_t bank, uint16_t a, uint16_t *x, uint16_t *y, uint8_t *p)
{
    (void)bank;
    uint16_t n;

    cyc(46);
    *y = RD16(copy_count);
    if (!*y) {
        *p = nz16(*p, 0);
        return a;
    }
    cyc(46);
    *x = RD16(copy_src_pos);
    cyc(118);
    a = rd16(0x02D65A + *x);
    WR16I(copy_dest, *y, a);
    cyc(54);
    WR16(copy_src_pos, (uint16_t)(*x - 2));
    n = (uint16_t)(*y - 2);
    cyc(116);
    WR16(copy_count, n);
    *p = nz16(*p, n);
    cyc(46);
    return a;
}

void WordCopyStep_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = WordCopyStep(cr->db, cr->a, &cr->x, &cr->y, &p);
    set_p(cr, (uint8_t)(p & ~(P_M | P_X)));
    cyc(60);
}

void abi_WordCopyStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    WordCopyStep_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:8CF6 WramFillZero
 * WHAT IT DOES: Clears Y bytes of work RAM from bank A, address X.
 * HOW: DMA channel 0, fixed source (the zero byte at $03:8D26), to WMDATA
 * ($2180) with the work RAM address set at $2181-$2183; starts it.
 * Entry: M 8-bit, X 16-bit; A = bank, X = address, Y = byte count (0 means
 * 65536). Exit: A low byte = 1; X = $8008; N, Z clear.
 */
void WramFillZero(uint8_t bank, uint16_t addr, uint16_t count)
{
    cyc(24);
    hw_wr16(0x002181, addr);
    cyc(24);
    hw_wr(0x002183, bank);
    cyc(24);
    dma_wr16(0x004305, count);
    cyc(48);
    dma_wr16(0x004302, 0x8D26);
    cyc(40);
    dma_wr(0x004304, 0x03);
    cyc(48);
    dma_wr16(0x004300, 0x8008);
    cyc(40);
    mdmaen_wr(0x00420B, 0x01);
    cyc(42);
}

void WramFillZero_cr(Carried *cr)
{
    WramFillZero((uint8_t)cr->a, cr->x, cr->y);
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0x01);
    cr->x = 0x8008;
    set_p(cr, (uint8_t)(cr->p & ~(P_N | P_Z)));
}

void abi_WramFillZero(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    WramFillZero_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:863D DemoRunFirst
 * WHAT IT DOES: Runs the first of the attract demos: $1A57 = 0 (its offset
 * in the demo tables at $03:8004), then the shared demo run at $03:8683
 * (course, settings and the race itself).
 * HOW: A tail JMP into the demo runs of the routine at $03:8648 (DemoRun,
 * jump_routine).
 * Entry: D = $0000. Exit: as the demo run leaves it.
 */
void DemoRunFirst(Carried *cr)
{
    r_rep(cr, P_M | P_X);
    cr->a = 0x0000;
    set_p(cr, nz16(cr->p, 0));
    cyc(48);
    WR16(wram_1A57, 0x0000);
    cyc(24);
    DemoRun(cr);
}

void abi_DemoRunFirst(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    DemoRunFirst(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:8CF2 WramFillZeroLong
 * WHAT IT DOES: WramFillZero for callers in other banks: clears Y bytes of
 * work RAM from bank A, address X.
 * HOW: A near call, then a long return.
 * Entry: as WramFillZero. Exit: as WramFillZero.
 */
void WramFillZeroLong(Carried *cr)
{
    (cyc(46), WramFillZero_cr(cr));
    cyc(44); /* RTL */
}

void abi_WramFillZeroLong(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    WramFillZeroLong(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:8648 DemoRunSecond
 * WHAT IT DOES: Runs the second of the attract demos: $1A57 = $6B (its
 * offset in the demo tables), then the shared demo run at $03:8683.
 * HOW: A tail JMP into the demo run (DemoRun, jump_routine).
 * Entry: D = $0000. Exit: as the demo run leaves it.
 */
void DemoRunSecond(Carried *cr)
{
    r_rep(cr, P_M | P_X);
    cr->a = 0x006B;
    set_p(cr, nz16(cr->p, cr->a));
    cyc(48);
    WR16(wram_1A57, 0x006B);
    cyc(24);
    DemoRun(cr);
}

void abi_DemoRunSecond(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    DemoRunSecond(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:865E DemoRunThird
 * WHAT IT DOES: Runs the third of the attract demos: $1A57 = $159, then the
 * shared demo run at $03:8683, then two more (DemoRunThirdRest, $03:866A).
 * HOW: A JSL to the demo run, then on into DemoRunThirdRest.
 * Entry: D = $0000. Exit: as the demo run leaves it.
 */
void DemoRunThird(Carried *cr)
{
    r_rep(cr, P_M | P_X);
    cr->a = 0x0159;
    set_p(cr, nz16(cr->p, cr->a));
    cyc(48);
    WR16(wram_1A57, 0x0159);
    cyc(62);
    DemoRun(cr);
    DemoRunThirdRest(cr);
}

void abi_DemoRunThird(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    DemoRunThird(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:866A DemoRunThirdRest
 * WHAT IT DOES: DemoRunThird after its first demo returns: two more attract
 * demos, $1A57 = $1FA and $287 (the last one waits for a button held,
 * command $0B), each by the shared demo run at $03:8683, then the return
 * to DemoRunThird's caller.
 * HOW: REP #$30, LDA #offset, STA $1A57, JSL to the demo run, twice; RTL.
 * Play does not reach it; unit_demo_third_rest_03 calls it (about 6,050
 * frames, B pressed once the second demo waits for it).
 * Entry: D = $0000. Exit: as the second demo run leaves it.
 */
void DemoRunThirdRest(Carried *cr)
{
    r_rep(cr, P_M | P_X);
    cr->a = 0x01FA;
    set_p(cr, nz16(cr->p, cr->a));
    cyc(48);
    WR16(wram_1A57, 0x01FA);
    cyc(62);
    DemoRun(cr);
    r_rep(cr, P_M | P_X);
    cr->a = 0x0287;
    set_p(cr, nz16(cr->p, cr->a));
    cyc(48);
    WR16(wram_1A57, 0x0287);
    cyc(62);
    DemoRun(cr);
    cyc(44);
}

void abi_DemoRunThirdRest(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    DemoRunThirdRest(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:8653 DemoRunFourth
 * WHAT IT DOES: Runs the third demo of the title screen's cycle: $1A57 = $FC
 * (its offset in the demo tables), then the shared demo run at $03:8683.
 * HOW: A tail JMP into the demo run (DemoRun, jump_routine).
 * Entry: D = $0000. Exit: as the demo run leaves it.
 */
void DemoRunFourth(Carried *cr)
{
    r_rep(cr, P_M | P_X);
    cr->a = 0x00FC;
    set_p(cr, nz16(cr->p, cr->a));
    WR16(wram_1A57, 0x00FC);
    DemoRun(cr);
}

void abi_DemoRunFourth(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    DemoRunFourth(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}


void abi_Set7EFFA6Sequence(void);
void abi_RaceVideoInit(void);
void abi_WindowsInit(void);
void abi_CourseDataPointer(void);
void abi_CourseScriptRun(void);
void abi_ObjectPlaceAll(void);
void abi_GsuRun(void);
void abi_ObjectsRunHandlers(void);
void abi_PlayerStatsAndPaletteAnimate(void);
void abi_ObjectLinkedSet1A(void);
void abi_CameraFollowStep(void);
void abi_ObjectFaceTied(void);
void abi_RaceFrameRun(void);

/* The direct page address of `a`. */
static uint32_t dp_addr(Carried *cr, uint8_t a)
{
    return (uint16_t)(cr->d + a);
}

/* LDA #v with M 8-bit. */
static void demo_lda8(Carried *cr, uint8_t v)
{
    cyc(40);
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    set_p(cr, nz8(cr->p, v));
}

/* SEP #$20, HDMA off, the screen blanked, SEI. */
static void demo_blank(Carried *cr, uint32_t b)
{
    r_sep(cr, P_M);
    cyc(46);
    hdmaen_wr(b | HDMAEN, 0x00);
    demo_lda8(cr, 0x80);
    ppu_wr(b | INIDISP, 0x80);
    set_p(cr, (uint8_t)(cr->p | (P_I)));
}

/* LDA $0D61 (16-bit), its low byte sign-extended (BIT #$80, ORA #$FF00 or
   AND #$00FF): whether the fade's level has gone negative (the fade is
   over). */
static int demo_fade_done(Carried *cr)
{
    cyc(24);
    r_lda16(cr, WA(fade_level_hi));
    if (cr->a & 0x0080)
        cr->a |= 0xFF00;
    else
        { cyc(92); cr->a &= 0x00FF; }
    set_p(cr, nz16(cr->p, cr->a));
    return (cr->a & 0x8000) != 0;
}

/* SEP #$20, $0E3A = 1 (the fade out started), REP #$20. */
static void demo_fade_out(Carried *cr)
{
    r_sep(cr, P_M);
    demo_lda8(cr, 0x01);
    cyc(44);
    WR8(fade_out, 0x01);
    r_rep(cr, P_M);
}

/* The camera object $1A64 moved: CameraFollowStep and ObjectFaceTied with
   the data bank $7E (PHB, PLB around them), then REP #$30. */
static void demo_camera(Carried *cr, uint8_t db)
{
    r_sep(cr, P_M);
    cyc(156);
    demo_lda8(cr, 0x7E);
    cr->db = 0x7E;
    cr->x = 0x1A64;
    set_p(cr, nz16(cr->p, 0x1A64));
    CameraFollowStep_cr(cr);
    (cyc(62), ObjectFaceTied_cr(cr));
    cr->db = db;
    set_p(cr, nz8(cr->p, db));
    r_rep(cr, P_M | P_X);
}

/* X += n (TXA, CLC, ADC #n, TAX). */
static void demo_next(Carried *cr, uint16_t n)
{
    cyc(90);
    cr->a = (uint16_t)(cr->x + n);
    set_p(cr, (uint8_t)(nz16(cr->p, cr->a) & ~(P_C | P_V)));
    cr->x = cr->a;
}

/* As demo_next without its closing JMP (24): for a command that stores
   before it jumps back. */
static void demo_next_before(Carried *cr, uint16_t n)
{
    cyc(66);
    cr->a = (uint16_t)(cr->x + n);
    set_p(cr, (uint8_t)(nz16(cr->p, cr->a) & ~(P_C | P_V)));
    cr->x = cr->a;
}

/*
 * @routine 03:8683 DemoRun
 * WHAT IT DOES: Runs one attract demo from its script at $03:8004 +
 * $1A57: the course ($0E3D, script byte 2) and the selection handler
 * ($00:18DB, byte 0) from the script's head, $1A5B = $20, HUD mode 0, the
 * Super FX set (clock, config $20, screen base $0B), the race set up
 * (Set7EFFA6Sequence, RaceVideoInit, 02:CF3E, WindowsInit, the palette
 * flag, 02:CF45 as AttractCourseSetup, HUD mode 3, CourseDataPointer,
 * CourseScriptRun, ObjectPlaceAll, Super FX program $04:8800,
 * ObjectsRunHandlers), the player object's flag 8 set and $70:2732
 * cleared; then the script's commands, one race frame (RaceFrameRun,
 * $1A59 counting them) wherever a command waits:
 *   1 word: ObjectLinkedSet1A with it on object $0346; 2 word: to
 *   $70:2732 (and its high byte again to $70:2733); 3 word: wait until
 *   frame $1A59 is the word; 5: fade out; 6: wait for the fade's end; 7:
 *   $19DF = 0, $19D8 = 1; 8: wait until $19D8 is 5; 9 word word: the
 *   camera object's +$08 and +$0C set, then moved (CameraFollowStep,
 *   ObjectFaceTied); $0A word: added to its +$0A, then moved; $0B: wait
 *   for a button held, then fade out to the end; $0C word, $0D word: ORA,
 *   AND into $1A5B; any other byte ends the script.
 * Each frame, with $1A5B bit 0 set the demo runs on to the fade's end;
 * with bit 1 clear a button held ($030A) ends it (bits $41, fade out).
 * At the end the screen is blanked, NMI, DMA and HDMA off.
 * HOW: Compare chain on the command byte, every script read long from
 * bank $03; PlayerStatsAndPaletteAnimate before each command; data bank
 * $03 from RaceVideoInit on (PHK, PLB), $7E around the camera's calls.
 * Entry: M 16-bit, X 16-bit; $1A57 = the demo's script offset. Exit: C set
 * when a button ended the demo; A = $1A5B & $40, $1A5B cleared; M 16-bit,
 * X 16-bit.
 */
void DemoRun(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;
    uint16_t x, v, w;
    uint8_t db, c;
    uint8_t p;

    r_rep(cr, P_M | P_X);
    cyc(24);
    cr->x = RD16(wram_1A57);
    cyc(72);
    r_lda16(cr, 0x038006 + cr->x);
    WR16(course, cr->a);
    r_lda16_imm(cr, 0x0020);
    cyc(10);
    WR16(wram_1A5B, 0x0020);
    cyc(24);
    cr->x = RD16(wram_1A57);
    cyc(80);
    r_lda16(cr, 0x038004 + cr->x);
    wr16(W(wram_18DB), cr->a);
    r_lda16_imm(cr, 0x0000);
    cyc(10);
    WR16(hud_mode, 0x0000);
    r_sep(cr, P_M);
    demo_lda8(cr, 0x01);
    cyc(22);
    WR8(gsu_scmr_shadow, 0x01);
    demo_lda8(cr, 0x01);
    hw_wr(b | 0x3039, 0x01);
    demo_lda8(cr, 0x20);
    hw_wr(b | 0x3037, 0x20);
    demo_lda8(cr, 0x0B);
    hw_wr(b | 0x3038, 0x0B);
    (cyc(62), Set7EFFA6Sequence_cr(cr));
    b = (uint32_t)cr->db << 16;
    demo_blank(cr, b);
    (cyc(76), RaceVideoInit_cr(cr));
    cr->db = 0x03;
    set_p(cr, nz8(cr->p, 0x03));
    b = 0x030000;
    cyc(50);
    demo_blank(cr, b);
    r_sep(cr, P_M | P_X);
    cyc(138);
    w_KeepRegisters(cr);
    WindowsInit_cr(cr);
    r_sep(cr, P_M);
    r_rep(cr, P_X);
    b = (uint32_t)cr->db << 16;
    demo_lda8(cr, 0x01);
    cyc(22);
    hw_wr(b | 0x3039, 0x01);
    demo_lda8(cr, 0x01);
    cyc(8);
    wr8(W(palette_saved), 0x01);
    set_p(cr, (uint8_t)(cr->p | (P_I)));
    cyc(76);
    AttractCourseSetup(cr);
    r_sep(cr, P_M);
    b = (uint32_t)cr->db << 16;
    demo_lda8(cr, 0x03);
    cyc(22);
    WR8(hud_mode, 0x03);
    (cyc(62), w_CourseDataPointer(cr));
    (cyc(62), CourseScriptRun(cr));
    (cyc(62), w_ObjectPlaceAll(cr));
    r_sep(cr, P_M);
    r_rep(cr, P_X);
    cyc(84);
    demo_lda8(cr, 0x04);
    cr->x = 0x8800;
    set_p(cr, nz16(cr->p, 0x8800));
    w_GsuRun(cr);
    r_sep(cr, P_M);
    b = (uint32_t)cr->db << 16;
    demo_lda8(cr, 0x01);
    cyc(22);
    WR8(wram_0357, 0x01);
    cyc(24);
    WR8(wram_05E9, 0x00);
    r_rep(cr, P_M);
    cyc(24);
    WR16(wram_1A59, 0x0000);
    (cyc(62), w_ObjectsRunHandlers(cr));
    r_sep(cr, P_M);
    b = (uint32_t)cr->db << 16;
    cyc(54);
    r_lda8(cr, TIMEUP);
    r_rep(cr, P_M);
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_I)));
    r_rep(cr, P_M | P_X);
    cyc(46);
    r_lda16(cr, 0x702476);
    r_tax16(cr);
    cyc(16);
    r_lda16(cr, 0x700064 + cr->x);
    cr->a |= 0x0008;
    set_p(cr, nz16(cr->p, cr->a));
    cyc(56);
    wr16(0x700064 + cr->x, cr->a);
    r_lda16_imm(cr, 0x0000);
    cyc(18);
    wr16(0x702732, 0x0000);
    cyc(24);
    r_lda16(cr, WA(wram_1A57));
    r_add16(cr, 0x0003);
    cyc(62);
    WR16(wram_1A57, cr->a);
    for (;;) {
        cyc(62);
        w_PlayerStatsAndPaletteAnimate(cr);
        r_rep(cr, P_M | P_X);
        b = (uint32_t)cr->db << 16;
        cyc(24);
        cr->x = RD16(wram_1A57);
        set_p(cr, nz16(cr->p, cr->x));
        for (;;) {
            /* $8762: the next command, X kept in $1A57. */
            cyc(24);
            WR16(wram_1A57, cr->x);
            cyc(242);
            r_lda16(cr, 0x038004 + cr->x);
            r_and16(cr, 0x00FF);
            c = (uint8_t)cr->a;
            r_cmp16(cr, c);
            if (c == 0x01) {
                x = cr->x;
                r_lda16(cr, 0x038005 + x);
                cr->x = RD16(wram_0346);
                set_p(cr, nz16(cr->p, cr->x));
                (cyc(62), w_ObjectLinkedSet1A(cr));
                cr->x = x;
                r_rep(cr, P_M | P_X);
                cyc(36);
                demo_next(cr, 0x0003);
                continue;
            } else { cyc(16); }
            if (c == 0x03) {
                r_lda16(cr, 0x038005 + cr->x);
                r_cmp16(cr, RD16(wram_1A59));
                if (!(cr->p & P_Z))
                    break; else { cyc(22); }
                demo_next(cr, 0x0003);
                continue;
            } else { cyc(54); }
            if (c == 0x02) {
                r_lda16(cr, 0x038005 + cr->x);
                wr16(0x702732, cr->a);
                cyc(80);
                r_lda16(cr, 0x038006 + cr->x);
                wr16(0x702733, cr->a);
                demo_next(cr, 0x0004);
                continue;
            } else { cyc(80); }
            if (c == 0x05) {
                demo_next(cr, 0x0001);
                demo_fade_out(cr);
                continue;
            }
            if (c == 0x06) {
                if (!demo_fade_done(cr))
                    break;
                demo_next(cr, 0x0001);
                continue;
            }
            if (c == 0x07) {
                demo_next_before(cr, 0x0001);
                r_sep(cr, P_M);
                demo_lda8(cr, 0x00);
                WR8(wram_19DF, 0x00);
                demo_lda8(cr, 0x01);
                WR8(window_hdma_off, 0x01);
                r_rep(cr, P_M);
                cyc(24);                        /* JMP */
                continue;
            } else { cyc(48); }
            if (c == 0x08) {
                r_lda16(cr, WA(window_hdma_off));
                r_and16(cr, 0x00FF);
                r_cmp16(cr, 0x0005);
                if (cr->a != 0x0005)
                    { cyc(46); break; } else { cyc(68); }
                demo_next(cr, 0x0001);
                continue;
            } else { cyc(34); }
            if (c == 0x09) {
                cyc(52);
                r_lda16(cr, 0x038005 + cr->x);
                wr16(dp_addr(cr, 0x02), cr->a);
                cyc(64);
                r_lda16(cr, 0x038007 + cr->x);
                wr16(dp_addr(cr, 0x94), cr->a);
                x = cr->x;
                cr->x = 0x1A64;
                r_lda16(cr, cyc_then(70, dp_addr(cr, 0x02)));
                cyc(32);
                wr16(W(dp_08) + cr->x, cr->a);
                r_lda16(cr, cyc_then(16, dp_addr(cr, 0x94)));
                cyc(32);
                wr16(W(dp_0c) + cr->x, cr->a);
                db = cr->db;
                demo_camera(cr, db);
                cr->x = x;
                cyc(64);
                demo_next(cr, 0x0005);
                continue;
            }
            if (c == 0x0A) {
                cyc(98);
                r_lda16(cr, 0x038005 + cr->x);
                wr16(dp_addr(cr, 0x08), cr->a);
                x = cr->x;
                cr->x = 0x1A64;
                cyc(86);
                v = rd16(W(dp_0a) + cr->x);
                p = (uint8_t)(cr->p & ~P_C);
                w = alu_adc16(v, rd16(cyc_then(30, dp_addr(cr, 0x08))), &p);
                set_p(cr, p);
                cr->a = w;
                cyc(32);
                wr16(W(dp_0a) + cr->x, w);
                db = cr->db;
                demo_camera(cr, db);
                cr->x = x;
                cyc(64);
                demo_next(cr, 0x0003);
                continue;
            }
            if (c == 0x0B) {
                cyc(104);
                r_lda16(cr, WA(wram_030A));
                if (!cr->a)
                    goto next_pass;
                demo_fade_out(cr);
                while (!demo_fade_done(cr))
                    ;
                goto end;
            }
            if (c == 0x0C || (cyc(46), c == 0x0D)) {
                cyc(150);
                r_lda16(cr, WA(wram_1A5B));
                cyc(72);
                v = rd16(0x038005 + cr->x);
                cr->a = c == 0x0C ? (uint16_t)(cr->a | v) : (uint16_t)(cr->a & v);
                set_p(cr, nz16(cr->p, cr->a));
                WR16(wram_1A5B, cr->a);
                demo_next(cr, 0x0003);
                continue;
            }
            goto end;
        }
        /* $892E: a race frame, then the frame count and the end checks. */
        cyc(108);
        for (;;) {
            RaceFrameRun(cr);
            r_rep(cr, P_M | P_X);
            b = (uint32_t)cr->db << 16;
            cyc(24);
            v = (uint16_t)(RD16(wram_1A59) + 1);
            cyc(14);
            WR16(wram_1A59, v);
            set_p(cr, nz16(cr->p, v));
            cyc(24);
            r_lda16(cr, WA(wram_1A5B));
            if (!(cr->a & 0x0001)) {
                set_p(cr, (uint8_t)((cr->p & ~P_Z) | ((cr->a & 0x0002) ? (cyc(64), 0) : P_Z)));
                if (cr->a & 0x0002)
                    goto next_pass;
                cyc(110);
                r_lda16(cr, WA(wram_030A));
                if (!cr->a)
                    goto next_pass;
                r_lda16(cr, WA(wram_1A5B));
                cr->a |= 0x0041;
                set_p(cr, nz16(cr->p, cr->a));
                WR16(wram_1A5B, cr->a);
                demo_fade_out(cr);
            } else {
                set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_Z)));
            }
            if (demo_fade_done(cr))
                goto end;
        }
next_pass:
        cyc(46);
        ;
    }
end:
    r_sep(cr, P_M);
    b = (uint32_t)cr->db << 16;
    demo_lda8(cr, 0x80);
    cyc(148);
    ppu_wr(b | INIDISP, 0x80);
    cyc(24);
    hw_wr(b | NMITIMEN, 0x00);
    cyc(24);
    mdmaen_wr(b | MDMAEN, 0x00);
    cyc(24);
    hdmaen_wr(b | HDMAEN, 0x00);
    r_rep(cr, P_M);
    cyc(24);
    r_lda16(cr, WA(wram_1A5B));
    r_and16(cr, 0x0040);
    v = cr->a;
    cyc(62);
    WR16(wram_1A5B, 0x0000);
    set_p(cr, (uint8_t)((cr->p & ~P_C) | (v ? P_C : (cyc(58), 0))));
}

void abi_DemoRun(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    DemoRun(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* CourseDataPointer as its adapter leaves the registers. */
static void w_CourseDataPointer(Carried *cr)
{
    uint8_t bank, p = cr->p;
    uint16_t x = CourseDataPointer(&bank, &p);

    cr->x = x;
    cr->y = x;
    cr->a = (uint16_t)((x & 0xFF00) | bank);
    set_p(cr, (uint8_t)((nz16(p, x) & ~P_X) | P_M));
}

/* GsuRun as its adapter leaves the registers. */
static void w_GsuRun(Carried *cr)
{
    uint8_t mode = GsuRun((uint8_t)cr->a, cr->x);

    cr->a = (uint16_t)((cr->a & 0xFF00) | mode);
    set_p(cr, nz8(cr->p, cr->db));
    cyc(72);                            /* PLB, RTL */
}

/* KeepRegisters as its adapter leaves the registers. */
static void w_KeepRegisters(Carried *cr)
{
    KeepRegisters_cr(cr);
}

/* ObjectLinkedSet1A as its adapter leaves the registers. */
static void w_ObjectLinkedSet1A(Carried *cr)
{
    cr->x = ObjectLinkedSet1A(cr->x, cr->a);
    set_p(cr, nz16(cr->p, cr->a));
}

/* ObjectPlaceAll as its adapter leaves the registers. */
static void w_ObjectPlaceAll(Carried *cr)
{
    cr->a = ObjectPlaceAll(cr->db);
    set_p(cr, (cr->p & P_X) ? nz8(cr->p, (uint8_t)cr->x) : nz16(cr->p, cr->x));
}

/* ObjectsRunHandlers on the carried registers. */
static void w_ObjectsRunHandlers(Carried *cr)
{
    ObjectsRunHandlers(cr);
}

/* PlayerStatsAndPaletteAnimate on the carried registers. */
static void w_PlayerStatsAndPaletteAnimate(Carried *cr)
{
    PlayerStatsAndPaletteAnimate(cr);
}
