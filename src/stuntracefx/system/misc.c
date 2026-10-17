/*
 * misc.c: small routines of the frame code that have no other home yet.
 */
#include "machine.h"
#include "cpu_ops.h"
#include "hw.h"

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

void abi_DataBankTest(void)
{
    cpu.p = nz8(cpu.p, cpu.db);
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

void abi_KeepRegisters(void)
{
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

void abi_ReturnOnly(void)
{
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
    wr8(0x000064, 0);
    return v;
}

void abi_ReadTable9022(void)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | ReadTable9022(cpu.db));
    cpu.x = 0x80;
    cpu.y = 0x00;
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
    uint32_t b = (uint32_t)bank << 16;
    uint16_t v;

    *x = (uint16_t)((rd16(b | 0x0D48) & 0x00FF) << 1);
    v = rd16(0x02FC58 + *x);
    wr16(b | 0x0D37, v);
    return v;
}

void abi_Set0D37FromTable(void)
{
    uint16_t x;

    cpu.a = Set0D37FromTable(cpu.db, &x);
    cpu.x = (cpu.p & P_X) ? (uint8_t)x : x;
}

/*
 * @routine 02:D65A Set7EBC2BTrio
 * WHAT IT DOES: Stores A's low byte at $7E:BC2B, $7E:BC2D and $7E:BC2F.
 * HOW: Three long byte stores.
 * Entry: any widths. Exit: every register unchanged.
 */
void Set7EBC2BTrio(uint8_t v)
{
    wr8(0x7EBC2B, v);
    wr8(0x7EBC2D, v);
    wr8(0x7EBC2F, v);
}

void abi_Set7EBC2BTrio(void)
{
    Set7EBC2BTrio((uint8_t)cpu.a);
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
    uint32_t b = (uint32_t)bank << 16;
    uint8_t v = rd8(b | 0x0DF8);

    if (v) {
        wr8(b | 0x0DF8, (uint8_t)(rd8(b | 0x0DF8) - 1));
        return v;
    }
    wr8(b | 0x0E5C, (uint8_t)(rd8(b | 0x0E5C) - 1));
    wr8(b | 0x1160, 0x00);
    return v;
}

void abi_Countdown0DF8(void)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | Countdown0DF8(cpu.db));
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

    for (i = 0; i < 4; i++)
        wr16(0x7EFFA6 + 2 * i, (uint16_t)(i + 1));
}

void abi_Set7EFFA6Sequence(void)
{
    Set7EFFA6Sequence();
    cpu.a = 0x0004;
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
    uint32_t b = (uint32_t)bank << 16;
    uint16_t n;

    *y = rd16(b | 0x0CF6);
    if (!*y) {
        *p = nz16(*p, 0);
        return a;
    }
    *x = rd16(b | 0x0CF2);
    a = rd16(0x02D65A + *x);
    wr16(b | (uint16_t)(0x07A2 + *y), a);
    wr16(b | 0x0CF2, (uint16_t)(*x - 2));
    n = (uint16_t)(*y - 2);
    wr16(b | 0x0CF6, n);
    *p = nz16(*p, n);
    return a;
}

void abi_WordCopyStep(void)
{
    uint8_t p = cpu.p;

    cpu.a = WordCopyStep(cpu.db, cpu.a, &cpu.x, &cpu.y, &p);
    cpu.p = (uint8_t)(p & ~(P_M | P_X));
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
    wr16(0x002181, addr);
    wr8(0x002183, bank);
    wr16(0x004305, count);
    wr16(0x004302, 0x8D26);
    wr8(0x004304, 0x03);
    wr16(0x004300, 0x8008);
    wr8(0x00420B, 0x01);
}

void abi_WramFillZero(void)
{
    WramFillZero((uint8_t)cpu.a, cpu.x, cpu.y);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x01);
    cpu.x = 0x8008;
    cpu.p = (uint8_t)(cpu.p & ~(P_N | P_Z));
}

/*
 * @routine 03:863D DemoRunFirst
 * WHAT IT DOES: Runs the first of the attract demos: $1A57 = 0 (its offset
 * in the demo tables at $03:8004), then the shared demo run at $03:8683
 * (course, settings and the race itself).
 * HOW: A tail JMP into the demo runs of the routine at $03:8648, reached
 * as original code (jump_routine).
 * Entry: D = $0000. Exit: as the demo run leaves it.
 */
void abi_DemoRunFirst(void)
{
    op_rep(P_M | P_X);
    cpu.a = 0x0000;
    cpu.p = nz16(cpu.p, 0);
    wr16((uint32_t)cpu.db << 16 | 0x1A57, 0x0000);
    jump_routine(0x038683);
}

/*
 * @routine 03:8CF2 WramFillZeroLong
 * WHAT IT DOES: WramFillZero for callers in other banks: clears Y bytes of
 * work RAM from bank A, address X.
 * HOW: A near call, then a long return.
 * Entry: as WramFillZero. Exit: as WramFillZero.
 */
void abi_WramFillZeroLong(void)
{
    abi_WramFillZero();
}

/*
 * @routine 03:8648 DemoRunSecond
 * WHAT IT DOES: Runs the second of the attract demos: $1A57 = $6B (its
 * offset in the demo tables), then the shared demo run at $03:8683.
 * HOW: A tail JMP into the demo run, reached as original code
 * (jump_routine).
 * Entry: D = $0000. Exit: as the demo run leaves it.
 */
void abi_DemoRunSecond(void)
{
    op_rep(P_M | P_X);
    cpu.a = 0x006B;
    cpu.p = nz16(cpu.p, cpu.a);
    wr16((uint32_t)cpu.db << 16 | 0x1A57, 0x006B);
    jump_routine(0x038683);
}

/*
 * @routine 03:865E DemoRunThird
 * WHAT IT DOES: Runs the third of the attract demos: $1A57 = $159, then the
 * shared demo run at $03:8683, then two more (DemoRunThirdRest, $03:866A).
 * HOW: A JSL to the demo run, then on into DemoRunThirdRest.
 * Entry: D = $0000. Exit: as the demo run leaves it.
 */
void abi_DemoRunThird(void)
{
    op_rep(P_M | P_X);
    cpu.a = 0x0159;
    cpu.p = nz16(cpu.p, cpu.a);
    wr16((uint32_t)cpu.db << 16 | 0x1A57, 0x0159);
    call_routine(0x038683, 1);
    jump_routine(0x03866A);
}

/*
 * @routine 03:866A DemoRunThirdRest
 * WHAT IT DOES: DemoRunThird after its first demo returns: two more attract
 * demos, $1A57 = $1FA and $287 (the last one waits for a button held,
 * command $0B), each by the shared demo run at $03:8683, then the return
 * to DemoRunThird's caller.
 * HOW: REP #$30, LDA #offset, STA $1A57, JSL to the demo run, twice; RTL.
 * Entry: D = $0000. Exit: as the second demo run leaves it.
 */
void abi_DemoRunThirdRest(void)
{
    op_rep(P_M | P_X);
    cpu.a = 0x01FA;
    cpu.p = nz16(cpu.p, cpu.a);
    wr16((uint32_t)cpu.db << 16 | 0x1A57, 0x01FA);
    call_routine(0x038683, 1);
    op_rep(P_M | P_X);
    cpu.a = 0x0287;
    cpu.p = nz16(cpu.p, cpu.a);
    wr16((uint32_t)cpu.db << 16 | 0x1A57, 0x0287);
    call_routine(0x038683, 1);
}

/*
 * @routine 03:8653 DemoRunFourth
 * WHAT IT DOES: Runs the third demo of the title screen's cycle: $1A57 = $FC
 * (its offset in the demo tables), then the shared demo run at $03:8683.
 * HOW: A tail JMP into the demo run, reached as original code
 * (jump_routine).
 * Entry: D = $0000. Exit: as the demo run leaves it.
 */
void abi_DemoRunFourth(void)
{
    op_rep(P_M | P_X);
    cpu.a = 0x00FC;
    cpu.p = nz16(cpu.p, cpu.a);
    wr16((uint32_t)cpu.db << 16 | 0x1A57, 0x00FC);
    jump_routine(0x038683);
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
static uint32_t dp_addr(uint8_t a)
{
    return (uint16_t)(cpu.d + a);
}

/* LDA #v with M 8-bit. */
static void demo_lda8(uint8_t v)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, v);
}

/* SEP #$20, HDMA off, the screen blanked, SEI. */
static void demo_blank(uint32_t b)
{
    op_sep(P_M);
    wr8(b | HDMAEN, 0x00);
    demo_lda8(0x80);
    wr8(b | INIDISP, 0x80);
    cpu.p |= P_I;
}

/* LDA $0D61 (16-bit), its low byte sign-extended (BIT #$80, ORA #$FF00 or
   AND #$00FF): whether the fade's level has gone negative (the fade is
   over). */
static int demo_fade_done(uint32_t b)
{
    op_lda16(b | 0x0D61);
    if (cpu.a & 0x0080)
        cpu.a |= 0xFF00;
    else
        cpu.a &= 0x00FF;
    cpu.p = nz16(cpu.p, cpu.a);
    return (cpu.a & 0x8000) != 0;
}

/* SEP #$20, $0E3A = 1 (the fade out started), REP #$20. */
static void demo_fade_out(uint32_t b)
{
    op_sep(P_M);
    demo_lda8(0x01);
    wr8(b | 0x0E3A, 0x01);
    op_rep(P_M);
}

/* The camera object $1A64 moved: CameraFollowStep and ObjectFaceTied with
   the data bank $7E (PHB, PLB around them), then REP #$30. */
static void demo_camera(uint8_t db)
{
    op_sep(P_M);
    demo_lda8(0x7E);
    cpu.db = 0x7E;
    cpu.x = 0x1A64;
    cpu.p = nz16(cpu.p, 0x1A64);
    abi_CameraFollowStep();
    abi_ObjectFaceTied();
    cpu.db = db;
    cpu.p = nz8(cpu.p, db);
    op_rep(P_M | P_X);
}

/* X += n (TXA, CLC, ADC #n, TAX). */
static void demo_next(uint16_t n)
{
    cpu.a = (uint16_t)(cpu.x + n);
    cpu.p = (uint8_t)(nz16(cpu.p, cpu.a) & ~(P_C | P_V));
    cpu.x = cpu.a;
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
void abi_DemoRun(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint16_t x, v, w;
    uint8_t db, c;
    uint8_t p;

    op_rep(P_M | P_X);
    cpu.x = rd16(b | 0x1A57);
    op_lda16(0x038006 + cpu.x);
    wr16(b | 0x0E3D, cpu.a);
    op_lda16_imm(0x0020);
    wr16(b | 0x1A5B, 0x0020);
    cpu.x = rd16(b | 0x1A57);
    op_lda16(0x038004 + cpu.x);
    wr16(0x0018DB, cpu.a);
    op_lda16_imm(0x0000);
    wr16(b | 0x0D2B, 0x0000);
    op_sep(P_M);
    demo_lda8(0x01);
    wr8(b | 0x0374, 0x01);
    demo_lda8(0x01);
    wr8(b | 0x3039, 0x01);
    demo_lda8(0x20);
    wr8(b | 0x3037, 0x20);
    demo_lda8(0x0B);
    wr8(b | 0x3038, 0x0B);
    abi_Set7EFFA6Sequence();
    b = (uint32_t)cpu.db << 16;
    demo_blank(b);
    abi_RaceVideoInit();
    cpu.db = 0x03;
    cpu.p = nz8(cpu.p, 0x03);
    b = 0x030000;
    demo_blank(b);
    op_sep(P_M | P_X);
    call_routine(0x02CF3E, 1);
    abi_WindowsInit();
    op_sep(P_M);
    op_rep(P_X);
    b = (uint32_t)cpu.db << 16;
    demo_lda8(0x01);
    wr8(b | 0x3039, 0x01);
    demo_lda8(0x01);
    wr8(0x7EFF65, 0x01);
    cpu.p |= P_I;
    call_routine(0x02CF45, 1);
    op_sep(P_M);
    b = (uint32_t)cpu.db << 16;
    demo_lda8(0x03);
    wr8(b | 0x0D2B, 0x03);
    abi_CourseDataPointer();
    abi_CourseScriptRun();
    abi_ObjectPlaceAll();
    op_sep(P_M);
    op_rep(P_X);
    demo_lda8(0x04);
    cpu.x = 0x8800;
    cpu.p = nz16(cpu.p, 0x8800);
    abi_GsuRun();
    op_sep(P_M);
    b = (uint32_t)cpu.db << 16;
    demo_lda8(0x01);
    wr8(b | 0x0357, 0x01);
    wr8(b | 0x05E9, 0x00);
    op_rep(P_M);
    wr16(b | 0x1A59, 0x0000);
    abi_ObjectsRunHandlers();
    op_sep(P_M);
    b = (uint32_t)cpu.db << 16;
    op_lda8(TIMEUP);
    op_rep(P_M);
    cpu.p &= (uint8_t)~P_I;
    op_rep(P_M | P_X);
    op_lda16(0x702476);
    op_tax16();
    op_lda16(0x700064 + cpu.x);
    cpu.a |= 0x0008;
    cpu.p = nz16(cpu.p, cpu.a);
    wr16(0x700064 + cpu.x, cpu.a);
    op_lda16_imm(0x0000);
    wr16(0x702732, 0x0000);
    op_lda16(b | 0x1A57);
    op_add16(0x0003);
    wr16(b | 0x1A57, cpu.a);
    for (;;) {
        abi_PlayerStatsAndPaletteAnimate();
        op_rep(P_M | P_X);
        b = (uint32_t)cpu.db << 16;
        cpu.x = rd16(b | 0x1A57);
        cpu.p = nz16(cpu.p, cpu.x);
        for (;;) {
            /* $8762: the next command, X kept in $1A57. */
            wr16(b | 0x1A57, cpu.x);
            op_lda16(0x038004 + cpu.x);
            op_and16(0x00FF);
            c = (uint8_t)cpu.a;
            op_cmp16(c);
            if (c == 0x01) {
                x = cpu.x;
                op_lda16(0x038005 + x);
                cpu.x = rd16(b | 0x0346);
                cpu.p = nz16(cpu.p, cpu.x);
                abi_ObjectLinkedSet1A();
                cpu.x = x;
                op_rep(P_M | P_X);
                demo_next(0x0003);
                continue;
            }
            if (c == 0x03) {
                op_lda16(0x038005 + cpu.x);
                op_cmp16(rd16(b | 0x1A59));
                if (!(cpu.p & P_Z))
                    break;
                demo_next(0x0003);
                continue;
            }
            if (c == 0x02) {
                op_lda16(0x038005 + cpu.x);
                wr16(0x702732, cpu.a);
                op_lda16(0x038006 + cpu.x);
                wr16(0x702733, cpu.a);
                demo_next(0x0004);
                continue;
            }
            if (c == 0x05) {
                demo_next(0x0001);
                demo_fade_out(b);
                continue;
            }
            if (c == 0x06) {
                if (!demo_fade_done(b))
                    break;
                demo_next(0x0001);
                continue;
            }
            if (c == 0x07) {
                demo_next(0x0001);
                op_sep(P_M);
                demo_lda8(0x00);
                wr8(b | 0x19DF, 0x00);
                demo_lda8(0x01);
                wr8(b | 0x19D8, 0x01);
                op_rep(P_M);
                continue;
            }
            if (c == 0x08) {
                op_lda16(b | 0x19D8);
                op_and16(0x00FF);
                op_cmp16(0x0005);
                if (cpu.a != 0x0005)
                    break;
                demo_next(0x0001);
                continue;
            }
            if (c == 0x09) {
                op_lda16(0x038005 + cpu.x);
                wr16(dp_addr(0x02), cpu.a);
                op_lda16(0x038007 + cpu.x);
                wr16(dp_addr(0x94), cpu.a);
                x = cpu.x;
                cpu.x = 0x1A64;
                op_lda16(dp_addr(0x02));
                wr16(0x000008 + cpu.x, cpu.a);
                op_lda16(dp_addr(0x94));
                wr16(0x00000C + cpu.x, cpu.a);
                db = cpu.db;
                demo_camera(db);
                cpu.x = x;
                demo_next(0x0005);
                continue;
            }
            if (c == 0x0A) {
                op_lda16(0x038005 + cpu.x);
                wr16(dp_addr(0x08), cpu.a);
                x = cpu.x;
                cpu.x = 0x1A64;
                v = rd16(0x00000A + cpu.x);
                p = (uint8_t)(cpu.p & ~P_C);
                w = op_adc16(v, rd16(dp_addr(0x08)), &p);
                cpu.p = p;
                cpu.a = w;
                wr16(0x00000A + cpu.x, w);
                db = cpu.db;
                demo_camera(db);
                cpu.x = x;
                demo_next(0x0003);
                continue;
            }
            if (c == 0x0B) {
                op_lda16(b | 0x030A);
                if (!cpu.a)
                    goto next_pass;
                demo_fade_out(b);
                while (!demo_fade_done(b))
                    ;
                goto end;
            }
            if (c == 0x0C || c == 0x0D) {
                op_lda16(b | 0x1A5B);
                v = rd16(0x038005 + cpu.x);
                cpu.a = c == 0x0C ? (uint16_t)(cpu.a | v) : (uint16_t)(cpu.a & v);
                cpu.p = nz16(cpu.p, cpu.a);
                wr16(b | 0x1A5B, cpu.a);
                demo_next(0x0003);
                continue;
            }
            goto end;
        }
        /* $892E: a race frame, then the frame count and the end checks. */
        for (;;) {
            abi_RaceFrameRun();
            op_rep(P_M | P_X);
            b = (uint32_t)cpu.db << 16;
            v = (uint16_t)(rd16(b | 0x1A59) + 1);
            wr16(b | 0x1A59, v);
            cpu.p = nz16(cpu.p, v);
            op_lda16(b | 0x1A5B);
            if (!(cpu.a & 0x0001)) {
                cpu.p = (uint8_t)((cpu.p & ~P_Z) | ((cpu.a & 0x0002) ? 0 : P_Z));
                if (cpu.a & 0x0002)
                    goto next_pass;
                op_lda16(b | 0x030A);
                if (!cpu.a)
                    goto next_pass;
                op_lda16(b | 0x1A5B);
                cpu.a |= 0x0041;
                cpu.p = nz16(cpu.p, cpu.a);
                wr16(b | 0x1A5B, cpu.a);
                demo_fade_out(b);
            } else {
                cpu.p &= (uint8_t)~P_Z;
            }
            if (demo_fade_done(b))
                goto end;
        }
next_pass:
        ;
    }
end:
    op_sep(P_M);
    b = (uint32_t)cpu.db << 16;
    demo_lda8(0x80);
    wr8(b | INIDISP, 0x80);
    wr8(b | NMITIMEN, 0x00);
    wr8(b | MDMAEN, 0x00);
    wr8(b | HDMAEN, 0x00);
    op_rep(P_M);
    op_lda16(b | 0x1A5B);
    op_and16(0x0040);
    v = cpu.a;
    wr16(b | 0x1A5B, 0x0000);
    cpu.p = (uint8_t)((cpu.p & ~P_C) | (v ? P_C : 0));
}
