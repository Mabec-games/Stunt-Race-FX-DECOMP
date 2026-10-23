/*
 * time_bonus.c: the race time on screen and the time bonus counted into it,
 * drawn by the Super FX digit program $07:F15B. $0E70 is the race time (BCD,
 * see time.c) and $0E74 a bonus (BCD) counted into it; $0D6B steps an
 * animation whose digit size comes from a table.
 */
#include "machine.h"
#include "cpu_ops.h"
#include "wram.h"

uint8_t GsuRun(uint8_t bank, uint16_t start);
uint16_t GsuParam216A(uint8_t bank, uint16_t index);
void GsuBytes200_BFB5BB(void);
void GsuBytes200_BFB7B6(void);
uint8_t GsuDigitPair0E74(uint8_t bank);
void abi_GsuBytes200_BFB5BB(void);
void abi_GsuBytes200_BFB7B6(void);
void abi_GsuDigitPair0E74(void);
void GsuBytes200_BFBAB9(void);
void GsuBytes200_BFBCBD(void);
void abi_GsuBytes200_BFBAB9(void);
void abi_GsuBytes200_BFBCBD(void);
void abi_SoundPort0QueuePushRaw(void);
void abi_GsuFA5AThree(void);
void abi_GsuFA5AFromTables(void);
void abi_GsuFA5ASix(void);
void abi_GsuFA1BFromTables(void);
void abi_GsuPiecesStep10CB(void);
void abi_GsuFA5AThreeShifted(void);
void abi_GsuFA5ASixLeft(void);
void abi_GsuFA35FromTables(void);
void abi_GsuParam216A(void);
void abi_GsuRun(void);
void abi_SoundPort0QueuePush(void);

#define TIME_BCD 0x000E70u
#define GSU_X 0x70020Au      /* the digit program's inputs */
#define GSU_Y 0x70020Cu
#define GSU_STEP 0x70020Eu
#define GSU_SIZE 0x700210u

/* Digit sizes for the animation steps ($0D6B / 2), words. */
#define SIZE_GROW 0x07918Fu
#define SIZE_BOUNCE 0x0792ADu

/* One digit: $70:0216 from the table at $8E3E, then program $07:F15B. */
static uint8_t digit(uint8_t bank, uint16_t d, uint16_t pre)
{
    cyc(pre);
    GsuParam216A(bank, (uint16_t)(d << 1));
    cyc(188);
    return GsuRun(0x07, 0xF15B);
}

/* A digit draw's end: the digit d's table word read again for A's high
   byte; returns A (that byte and the screen mode value). */
static uint16_t digit_a(uint8_t bank, uint16_t d, uint8_t mode)
{
    cyc(158);
    return (uint16_t)((rd16((uint32_t)bank << 16 | (uint16_t)(0x8E3E + (d << 1))) & 0xFF00) | mode);
}

/* The registers a digit draw leaves besides A: X = $F15B, M 16-bit, X
   16-bit, C clear, N and Z from the data bank. */
static void digit_exit(uint16_t a)
{
    cpu.a = a;
    cpu.x = 0xF15B;
    cpu.p = (uint8_t)(nz8(cpu.p, cpu.db) & ~(P_C | P_M | P_X));
}

/* The body of GsuTimeDigits; returns the last digit drawn, *mode the screen
   mode value. */
static uint16_t time_digits(uint8_t bank, uint8_t *mode, int x8, uint32_t lead)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t v;

    cyc(lead);                          /* 72 more after a setter, whose return is not stated */
    wr16(GSU_X, 0x00AC);
    cyc(56);
    wr16(GSU_Y, 0x0008);
    cyc(56);
    wr16(GSU_STEP, 0x0010);
    cyc(24);
    v = rd16(b | TIME_BCD);
    if (v >= 0x0100) {
        cyc(130);
        GsuParam216A(bank, (uint16_t)(((v >> 8) & 0x000F) << 1));
        cyc(98);
        wr16(GSU_X, 0x009E);
        cyc(146);
        GsuRun(0x07, 0xF15B);
        cyc(172);
        wr16(GSU_X, 0x00AC);
        cyc(24);
        v = rd16(b | TIME_BCD);
    } else { cyc(46); }
    cyc(56);
    digit(bank, (uint16_t)((v >> 4) & 0x000F), x8 ? 64 : 70);
    cyc(162);
    v = (uint16_t)(rd16(b | TIME_BCD) & 0x000F);
    *mode = digit(bank, v, 70);
    return v;
}

/*
 * @routine 07:91B1 GsuTimeDigits
 * WHAT IT DOES: Draws the race time at $0E70 (BCD) with the Super FX digit
 * program: the tens and units digits at X $AC, Y 8, and when the time is
 * $100 or more the hundreds digit first at X $9E.
 * HOW: $70:020A = $AC, $70:020C = 8, $70:020E = $10; each digit sets
 * $70:0216 from the table at $8E3E (GsuParam216A) and runs program
 * $07:F15B (GsuRun). The digit size at $70:0210 is the caller's. The
 * hundreds digit is not reached in the traces so far.
 * Entry: M 16-bit, X 16-bit, D = $0000; DB = the bank of the digit table.
 * Exit: A = the units digit's table word's high byte and the screen mode
 * value; X = $F15B; Y unchanged; M 16-bit, X 16-bit; C clear; N, Z from the
 * data bank; V unchanged.
 * From C: x8 with 8-bit index registers; returns A.
 */
uint16_t GsuTimeDigits(uint8_t bank, int x8)
{
    uint8_t mode;
    uint16_t d = time_digits(bank, &mode, x8, 128);

    return digit_a(bank, d, mode);
}

void abi_GsuTimeDigits(void)
{
    digit_exit(GsuTimeDigits(cpu.db, cpu.p & P_X));
}

/* GsuTimeDigits entered after `lead` clocks of a caller still in
   registers. */
static void abi_time_digits(uint32_t lead)
{
    uint8_t mode;
    uint16_t d = time_digits(cpu.db, &mode, cpu.p & P_X, lead);

    digit_exit(digit_a(cpu.db, d, mode));
}

/*
 * @routine 07:919D GsuTimeDigitsGrow
 * WHAT IT DOES: Draws the race time (GsuTimeDigits) at the digit size of
 * animation step $0D6B, from the table at $07:918F ($08, $10, $18, $1C,
 * then $20), with the renderer bytes 0, $BF, $B5, $BB.
 * HOW: The size word to $70:0210, GsuBytes200_BFB5BB, then the code of
 * GsuTimeDigits, which follows.
 * Entry: M 16-bit, X 16-bit, D = $0000; DB = the bank of the digit table.
 * Exit: as GsuTimeDigits.
 * From C: as GsuTimeDigits.
 */
uint16_t GsuTimeDigitsGrow(uint8_t bank, int x8)
{
    cyc(24);
    uint16_t x = rd16((uint32_t)bank << 16 | 0x0D6B);

    cyc(102);
    wr16(GSU_SIZE, rd16(SIZE_GROW + x));
    (cyc(62), GsuBytes200_BFB5BB());
    return GsuTimeDigits(bank, x8);
}

void abi_GsuTimeDigitsGrow(void)
{
    digit_exit(GsuTimeDigitsGrow(cpu.db, cpu.p & P_X));
}

/*
 * @routine 07:91A6 GsuTimeDigitsFull
 * WHAT IT DOES: As GsuTimeDigitsGrow, at the full digit size $20.
 * HOW: Enters GsuTimeDigitsGrow's tail with A = $20.
 * Entry: as GsuTimeDigitsGrow. Exit: as GsuTimeDigits.
 * From C: as GsuTimeDigits.
 */
uint16_t GsuTimeDigitsFull(uint8_t bank, int x8)
{
    cyc(56);
    wr16(GSU_SIZE, 0x0020);
    (cyc(62), GsuBytes200_BFB5BB());
    return GsuTimeDigits(bank, x8);
}

void abi_GsuTimeDigitsFull(void)
{
    digit_exit(GsuTimeDigitsFull(cpu.db, cpu.p & P_X));
}

uint16_t GsuDigitPair0E74A(uint8_t bank);

static int bd_drawn;                    /* the bonus digits were drawn */

/* The tail of the three GsuBonusDigits entries: the size to $70:0210, then
   when $0E6C is nonzero the renderer bytes 0, $BF, $B7, $B6 and the two
   digits of $0E74 (GsuDigitPair0E74, whose code follows). Returns A: 0, or
   as GsuDigitPair0E74 leaves it when it drew (bd_drawn). */
static uint16_t bonus_digits(uint8_t bank, uint16_t size)
{
    cyc(56);
    wr16(GSU_SIZE, size);
    cyc(24);
    bd_drawn = 0;
    if (!rd16((uint32_t)bank << 16 | 0x0E6C))
        { cyc(58); return 0; }
    cyc(84);
    GsuBytes200_BFB7B6();
    bd_drawn = 1;
    return (cyc(72), GsuDigitPair0E74A(bank));
}

/* The exit: drawn, as GsuDigitPair0E74; else A = 0, Z set, N clear and
   X = x (or unchanged when x is negative). */
static void bonus_exit(uint16_t a, int32_t x)
{
    if (bd_drawn) {
        digit_exit(a);
        return;
    }
    cpu.a = 0x0000;
    if (x >= 0)
        cpu.x = (uint16_t)x;
    cpu.p = (uint8_t)((cpu.p & ~P_N) | P_Z);
}

static uint16_t bd_x;                   /* the step $0D6B the size came by */

/*
 * @routine 07:922F GsuBonusDigitsGrow
 * WHAT IT DOES: Sets the digit size for animation step $0D6B from the table
 * at $07:918F; then, when $0E6C is nonzero, draws the two digits of $0E74
 * with the renderer bytes 0, $BF, $B7, $B6.
 * HOW: The size word to $70:0210; GsuBytes200_BFB7B6 and the code of
 * GsuDigitPair0E74, which follows.
 * Entry: M 16-bit, X 16-bit, D = $0000; DB = the bank of the digit table.
 * Exit: when $0E6C is 0, A = 0, X = $0D6B, Z set, N clear; otherwise as
 * GsuDigitPair0E74.
 * From C: returns A; drawn in bd_drawn, X (not drawn) in bd_x.
 */
uint16_t GsuBonusDigitsGrow(uint8_t bank)
{
    cyc(24);
    bd_x = rd16((uint32_t)bank << 16 | 0x0D6B);
    cyc(46);
    return bonus_digits(bank, rd16(SIZE_GROW + bd_x));
}

void abi_GsuBonusDigitsGrow(void)
{
    uint16_t a = GsuBonusDigitsGrow(cpu.db);

    bonus_exit(a, bd_x);
}

/*
 * @routine 07:9238 GsuBonusDigitsBounce
 * WHAT IT DOES: As GsuBonusDigitsGrow, with the size from the table at
 * $07:92AD ($08, $10, $14, $18, $18, $14, $10, $08).
 * HOW: Enters GsuBonusDigitsGrow's tail. No caller is known; not reached
 * in the traces so far.
 * Entry: as GsuBonusDigitsGrow. Exit: as GsuBonusDigitsGrow.
 */
uint16_t GsuBonusDigitsBounce(uint8_t bank)
{
    cyc(24);
    bd_x = rd16((uint32_t)bank << 16 | 0x0D6B);
    cyc(46);
    return bonus_digits(bank, rd16(SIZE_BOUNCE + bd_x));
}

void abi_GsuBonusDigitsBounce(void)
{
    uint16_t a = GsuBonusDigitsBounce(cpu.db);

    bonus_exit(a, bd_x);
}

/*
 * @routine 07:9241 GsuBonusDigitsFull
 * WHAT IT DOES: As GsuBonusDigitsGrow, at the full digit size $20.
 * HOW: Enters GsuBonusDigitsGrow's tail with A = $20. No caller is known;
 * not reached in the traces so far.
 * Entry: as GsuBonusDigitsGrow. Exit: when $0E6C is 0, A = 0, X
 * unchanged, Z set, N clear; otherwise as GsuDigitPair0E74.
 */
uint16_t GsuBonusDigitsFull(uint8_t bank)
{
    return bonus_digits(bank, 0x0020);
}

void abi_GsuBonusDigitsFull(void)
{
    bonus_exit(GsuBonusDigitsFull(cpu.db), -1);
}

/* The 65816's 16-bit ADC and SBC, binary or decimal as the D flag in *p
   says, as the emulator core computes them: the result, with N, Z, C and V
   in *p. */
static uint16_t adc16p(uint16_t a, uint16_t m, uint8_t *p)
{
    int c = *p & P_C, r;

    if (!(*p & P_D)) {
        r = a + m + c;
    } else {
        r = (a & 0x000F) + (m & 0x000F) + c;
        if (r > 0x0009) r += 0x0006;
        c = r > 0x000F;
        r = (a & 0x00F0) + (m & 0x00F0) + (c << 4) + (r & 0x000F);
        if (r > 0x009F) r += 0x0060;
        c = r > 0x00FF;
        r = (a & 0x0F00) + (m & 0x0F00) + (c << 8) + (r & 0x00FF);
        if (r > 0x09FF) r += 0x0600;
        c = r > 0x0FFF;
        r = (a & 0xF000) + (m & 0xF000) + (c << 12) + (r & 0x0FFF);
    }
    cyc(14);
    *p = (uint8_t)((*p & ~(P_V | P_C)) | ((~(a ^ m) & (a ^ r) & 0x8000) ? P_V : 0));
    if ((*p & P_D) && r > 0x9FFF) r += 0x6000;
    if (r > 0xFFFF) *p |= P_C;
    *p = nz16(*p, (uint16_t)r);
    return (uint16_t)r;
}

static uint16_t sbc16p(uint16_t a, uint16_t m, uint8_t *p)
{
    int c = *p & P_C, r;

    m = (uint16_t)~m;
    if (!(*p & P_D)) {
        r = a + m + c;
    } else {
        r = (a & 0x000F) + (m & 0x000F) + c;
        if (r <= 0x000F) r -= 0x0006;
        c = r > 0x000F;
        r = (a & 0x00F0) + (m & 0x00F0) + (c << 4) + (r & 0x000F);
        if (r <= 0x00FF) r -= 0x0060;
        c = r > 0x00FF;
        r = (a & 0x0F00) + (m & 0x0F00) + (c << 8) + (r & 0x00FF);
        if (r <= 0x0FFF) r -= 0x0600;
        c = r > 0x0FFF;
        r = (a & 0xF000) + (m & 0xF000) + (c << 12) + (r & 0x0FFF);
    }
    *p = (uint8_t)((*p & ~(P_V | P_C)) | ((~(a ^ m) & (a ^ r) & 0x8000) ? P_V : 0));
    if ((*p & P_D) && r <= 0xFFFF) r -= 0x6000;
    if (r > 0xFFFF) *p |= P_C;
    *p = nz16(*p, (uint16_t)r);
    return (uint16_t)r;
}

/* Decimal ADC and SBC with only the carry kept: *c in and out. */
static uint16_t adc_decimal16(uint16_t a, uint16_t m, int *c)
{
    uint8_t p = (uint8_t)(P_D | (*c ? P_C : 0));
    uint16_t r = adc16p(a, m, &p);

    *c = p & P_C;
    return r;
}

static uint16_t sbc_decimal16(uint16_t a, uint16_t m, int *c)
{
    uint8_t p = (uint8_t)(P_D | (*c ? P_C : 0));
    uint16_t r = sbc16p(a, m, &p);

    *c = p & P_C;
    return r;
}

/* The same, 8-bit. */
static uint8_t adc_decimal8(uint8_t a, uint8_t m, int *c)
{
    int r;

    r = (a & 0x0F) + (m & 0x0F) + *c;
    if (r > 0x09) r += 0x06;
    *c = r > 0x0F;
    r = (a & 0xF0) + (m & 0xF0) + (*c << 4) + (r & 0x0F);
    if (r > 0x9F) r += 0x60;
    *c = r > 0xFF;
    return (uint8_t)r;
}

static uint8_t sbc_decimal8(uint8_t a, uint8_t m, int *c)
{
    int r;

    m = (uint8_t)~m;
    r = (a & 0x0F) + (m & 0x0F) + *c;
    if (r <= 0x0F) r -= 0x06;
    *c = r > 0x0F;
    r = (a & 0xF0) + (m & 0xF0) + (*c << 4) + (r & 0x0F);
    if (r <= 0xFF) r -= 0x60;
    *c = r > 0xFF;
    return (uint8_t)r;
}

/* SoundPort0QueuePushRaw called with the decimal flag set, as the tally
   does: the ring slot (count + head, less 16 past the end) is then
   computed with decimal adds. */
static void sound_push_decimal(uint8_t command)
{
    uint8_t n, slot;
    int c;

    cyc(394);
    if (rd8(SOUND_MUTED) != 0)
        return;
    cyc(40);
    n = rd8(SOUND_PORT0_COUNT);
    if (n == 0) {
        cyc(46);
        wr8(SOUND_PORT0_COUNT, cyc_then(6, (uint8_t)(rd8(SOUND_PORT0_COUNT) + 1)));
        cyc(24);
        wr8(SOUND_PORT0_QUEUE + rd8(SOUND_PORT0_HEAD), cyc_then(30, command));
    } else if (n < 0x10) {
        c = 0;
        slot = adc_decimal8(n, rd8(SOUND_PORT0_HEAD), &c);
        if (slot >= 0x10) {
            c = 1;
            slot = sbc_decimal8(slot, 0x10, &c);
        }
        wr8(SOUND_PORT0_QUEUE + slot, command);
        wr8(SOUND_PORT0_COUNT, (uint8_t)(rd8(SOUND_PORT0_COUNT) + 1));
    }
    cyc(162);
}

/*
 * @routine 07:90EE TimeDigitBytes
 * WHAT IT DOES: Chooses the renderer bytes at $70:0200 for the time
 * digits: 0, $BF, $BA, $B9 when $0D4C is nonzero; otherwise by $0E76: $BF,
 * $B5, $BB from $20, $BF, $BC, $BD from $10, else $BF, $B7, $B6.
 * HOW: Compares and one of the GsuBytes200 setters. Not reached in the
 * traces so far.
 * Entry: M 16-bit, X 16-bit; DB = a bank where $0D4C and $0E76 are work
 * RAM. Exit: A = the word read with its low byte the last renderer byte;
 * X, Y unchanged; with $0D4C nonzero N, Z from it; otherwise N, Z, C from
 * the last compare ($0E76 - $20 or - $10).
 */
void TimeDigitBytes(uint8_t bank)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t v;

    if (rd16(b | 0x0D4C)) {
        GsuBytes200_BFBAB9();
        return;
    }
    v = rd16(b | 0x0E76);
    if (v >= 0x0020)
        GsuBytes200_BFB5BB();
    else if (v >= 0x0010)
        GsuBytes200_BFBCBD();
    else
        GsuBytes200_BFB7B6();
}

void abi_TimeDigitBytes(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    cyc(24);
    uint16_t v = rd16(b | 0x0D4C);

    cpu.a = v;
    if (v) {
        cyc(78);
        cpu.p = nz16(cpu.p, v);
        abi_GsuBytes200_BFBAB9();
        cyc(114);                       /* PLP, RTL, RTS */
        return;
    }
    cyc(46);
    v = rd16(b | 0x0E76);
    cpu.a = v;
    if (v >= 0x0020) {
        cyc(102);
        cpu.p = (uint8_t)(nz16(cpu.p, (uint16_t)(v - 0x0020)) | P_C);
        abi_GsuBytes200_BFB5BB();
        cyc(136);                       /* PLP, RTL, BRA, RTS */
        return;
    } else { cyc(86); }
    cpu.p = (uint8_t)((nz16(cpu.p, (uint16_t)(v - 0x0010)) & ~P_C) | (v >= 0x0010 ? P_C : (cyc(6), 0)));
    if (v >= 0x0010)
        { cyc(62); abi_GsuBytes200_BFBCBD(); cyc(114); }     /* its own 22 is the BRA */
    else
        { cyc(62); abi_GsuBytes200_BFB7B6(); cyc(114); }       /* PLP, RTL, RTS */
}

/* A 16-bit INC of a work-RAM word: one read, one write. */
static void inc16(uint32_t addr)
{
    cyc(24);
    wr16(addr, cyc_then(14, (uint16_t)(rd16(addr) + 1)));
}

static void sound(uint16_t command)
{
    cpu.a = command;
    (cyc(56), abi_SoundPort0QueuePushRaw());
    cyc(8);
}

/* The size for the bonus digits when they shrink: $8F less entry $0D6B of
   the table at $07:9806, into $0D6D. */
static void bonus_x_from_table(uint32_t b, uint16_t step)
{
    cpu.x = step;
    cyc(124);
    cpu.a = (uint16_t)(0x008F - rd16(0x079806 + step));
    wr16(b | 0x0D6D, cpu.a);
}

/* The frame drawn while $0E6C is nonzero, mirrored ($07:90DC). */
static void draw_mirrored(void)
{
    cyc(62);
    abi_GsuBytes200_BFB7B6();
    (cyc(118), abi_GsuFA5AThreeShifted());
    (cyc(46), abi_GsuFA5AFromTables());
    abi_GsuFA5ASixLeft();
    (cyc(46), abi_GsuFA35FromTables());
}

/* The bonus digits at the animation's size, then the time at full size
   and the mirrored frame ($07:9036 and $07:90CC). */
static void draw_bonus_then_time(uint32_t b)
{
    (cyc(46), abi_TimeDigitBytes());
    cyc(24);
    cpu.x = rd16(b | 0x0D6B);
    cyc(80);
    cpu.a = rd16(SIZE_GROW + cpu.x);
    wr16(GSU_SIZE, cpu.a);
    (cyc(46), abi_GsuDigitPair0E74());
    (cyc(70), abi_TimeDigitBytes());
    cpu.a = 0x0020;
    cyc(56);
    wr16(GSU_SIZE, cpu.a);
    (cyc(46), abi_TimeDigitBytes());
    cyc(46);                            /* JSR */
    abi_time_digits(56);
    draw_mirrored();
}

/* The frame: the pieces around ($0D6D, $0D6F) when $0E6C is nonzero,
   else the eleven pieces of GsuPiecesStep10CB ($07:8FBD). */
static void draw_frame(uint32_t b)
{
    cyc(24);
    cpu.a = rd16(b | 0x0E6C);
    if (cpu.a) {
        cyc(78);
        abi_GsuBytes200_BFB7B6();
        (cyc(118), abi_GsuFA5AThree());
        (cyc(46), abi_GsuFA5AFromTables());
        abi_GsuFA5ASix();
        (cyc(46), abi_GsuFA35FromTables());
    } else {
        abi_GsuPiecesStep10CB();
    }
}

/* The time at the animation's size, the bonus digits when shown, then the
   frame ($07:8FA6). */
static void draw_time(uint32_t b, int entry)
{
    cpu.a = 0x0030;
    cyc(entry + 48);
    wr16(b | 0x0D6D, cpu.a);
    (cyc(46), abi_GsuTimeDigitsGrow());
    cyc(24);
    cpu.a = rd16(b | 0x0E6C);
    if (cpu.a)
        { cyc(62); abi_GsuBonusDigitsGrow(); } else { cyc(22); }
    cpu.a = 0x0037;
    cyc(48);
    wr16(b | 0x0D6D, cpu.a);
    draw_frame(b);
}

/* The pieces of the second screen ($07:9151). */
static void draw_pieces(uint32_t b)
{
    cpu.a = 0x0030;
    cyc(48);
    wr16(b | 0x0D6D, cpu.a);
    (cyc(46), abi_GsuFA5AThree());
    (cyc(46), abi_GsuFA5AFromTables());
    abi_GsuFA5ASix();
    (cyc(46), abi_GsuFA1BFromTables());
}

/* The counting step of state 6: $0E7C at a time from $0E74 into $0E70, in
   decimal; the rest when less is left. */
static void count_bonus(uint32_t b)
{
    cyc(108);
    uint16_t v = rd16(b | 0x0E74);
    int c;

    cpu.a = v;
    if (!v)
        { cyc(28); return; }
    c = 1;
    cyc(54);
    v = sbc_decimal16(v, rd16(b | 0x0E7C), &c);
    if (!c) {
        c = 0;
        v = rd16(b | TIME_BCD);                 /* LDA, then ADC */
        v = adc_decimal16(v, rd16(b | 0x0E74), &c);
        wr16(b | TIME_BCD, v);
        cpu.a = 0x0000;
        wr16(b | 0x0E74, 0x0000);
    } else {
        cyc(46);
        wr16(b | 0x0E74, v);
        c = 0;
        cyc(24);
        cpu.a = rd16(b | TIME_BCD);             /* LDA, then ADC */
        cyc(38);
        cpu.a = adc_decimal16(cpu.a, rd16(b | 0x0E7C), &c);
        cyc(10);
        wr16(b | TIME_BCD, cpu.a);
    }
    cyc(6);
}

/* State 14: moves 3 (1 below 9) from the time to $0E74, in decimal, up to
   $99, with sound $44 when the time reaches 0 or $0E74 reaches $99. */
static void take_from_time(uint32_t b)
{
    uint16_t v;
    int c;

    cyc(78);
    v = rd16(b | TIME_BCD);
    if (v >= 0x0009) {
        c = 1;
        v = sbc_decimal16(v, 0x0003, &c);
        cyc(108);
        wr16(b | TIME_BCD, v);
        c = 0;
        cyc(24);
        v = adc_decimal16(rd16(b | 0x0E74), 0x0003, &c);
    } else {
        c = 1;
        v = sbc_decimal16(v, 0x0001, &c);
        cyc(102);
        wr16(b | TIME_BCD, v);
        if (v == 0) {
            cpu.a = 0x0044;
            sound_push_decimal(0x44);
            cpu.p = nz8(cpu.p, cpu.db);
        }
        c = 0;
        cyc(70);
        v = adc_decimal16(rd16(b | 0x0E74), 0x0001, &c);
        cyc(22);                                       /* BRA */
    }
    if (v >= 0x0099) {
        cyc(24);
        cpu.a = 0x0044;
        sound_push_decimal(0x44);
        v = 0x0099;
    }
    cpu.a = v;
    cyc(94);
    wr16(b | 0x0E74, v);
}

/*
 * @routine 07:8E90 TimeBonusTally
 * WHAT IT DOES: One frame of the time bonus animation on the race screen,
 * a state machine in $0D65 (0, 2, 4, ... 24): the digits of the race time
 * and of the bonus at $0E74 (shown when $0E6C is nonzero) grow in, $0E74 is
 * counted into the time ($0E7C at a time, by the size of the bonus) with
 * sounds, the digits shrink away; states 18-24 animate a second set of
 * pieces. $0D6B steps the animation (by 2), $0D6D and $0D6F place it.
 * HOW: Sets $0D6F to 6 and jumps through the table at $07:8E9F by $0D65.
 * 0: idle. 2: start (step -2, $0E74 = $0E6C). 4: grow the time; at step 8
 * choose $0E7C (1 below $32, 2 below $48, 3 below $64, else 4) with sound
 * $0B. 6: count, until $0D5A reaches 8, then add the rest, step $0D5B
 * down and sound $27. 8: shrink the bonus digits ($0D6D = $8F less the
 * table at $07:9806), then idle. 10-16: the same for a time taken back
 * into $0E74 (3 at a time, 1 below 9, up to $99) with the bonus digit
 * bytes from TimeDigitBytes and the mirrored frame. 18: start the
 * second animation when $19F0 is set ($0E74 = $28, sound $0B). 20: grow
 * it to step 10. 22: hold it. 24: shrink it, sound $27 each step.
 * Counting runs in decimal mode, so a sound queued from it computes its
 * ring slot in decimal. States 10-16 are not reached in the traces so
 * far, nor the bonus paths ($0E74 or $0E6C nonzero), the counting step of
 * state 6, the idle end of state 18 or the advance of state 20. The 18
 * bytes at $07:917D, a copy of the drawing tail of state 22 that nothing
 * jumps to, stay in the raw intake.
 * Entry: any widths; DB = a bank where the work RAM and the digit tables
 * are reached. Exit: P unchanged; A and X as the last call or store left
 * them; Y as the drawing calls leave it.
 */
void TimeBonusTally(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint16_t a, state;
    int entry;

    cpu.p &= (uint8_t)~(P_M | P_X);
    cpu.a = 0x0006;
    cyc(92);
    wr16(b | 0x0D6F, cpu.a);
    cyc(24);
    state = rd16(b | 0x0D65);
    cpu.x = state;
    cyc(46);
    switch (rd16(0x078E9F + state)) {
    case 0x8ED1:                        /* 0: idle */
        return;
    case 0x8EB9:                        /* 2: start */
        cpu.a = 0x0006;
        cyc(48);
        wr16(b | 0x0D6F, cpu.a);
        inc16(b | 0x0D65);
        inc16(b | 0x0D65);
        cpu.a = 0xFFFE;
        cyc(48);
        wr16(b | 0x0D6B, cpu.a);
        cyc(24);
        cpu.a = rd16(b | 0x0E6C);
        cyc(24);
        wr16(b | 0x0E74, cpu.a);
        return;
    case 0x8ED3:                        /* 4: grow the time */
        cyc(24);
        a = rd16(b | 0x0D6B);
        if (a != 0x0008) {
            cpu.a = (uint16_t)(a + 2);
            cyc(98);
            wr16(b | 0x0D6B, cpu.a);
            entry = 22;                 /* BRA */
        } else {
            entry = 24;                 /* JMP */
            cyc(40);
            inc16(b | 0x0D65);
            inc16(b | 0x0D65);
            cyc(24);
            a = rd16(b | 0x0E74);
            cpu.a = a;
            if (a) {
                cyc(46);
                sound(0x000B);
                cpu.a = a < 0x0032 ? 1 : (cyc(230), a < 0x0048 ? 2 : a < 0x0064 ? 3 : 4);
            }
            cyc(46);
            wr16(b | 0x0E7C, cpu.a);
        }
        draw_time(b, entry);
        return;
    case 0x8F27:                        /* 6: count the bonus into the time */
        cyc(24);
        if ((rd16(b | 0x0D5A) & 0x00FF) >= 0x0008) {
            int c = 0;

            cyc(102);
            cpu.a = rd16(b | TIME_BCD);         /* LDA, then ADC */
            cyc(38);
            cpu.a = adc_decimal16(cpu.a, rd16(b | 0x0E74), &c);
            cyc(10);
            wr16(b | TIME_BCD, cpu.a);
            cyc(24);
            wr16(b | 0x0E74, 0x0000);
            cyc(38);
            wr16(b | 0x0D5B, cyc_then(14, (uint16_t)(rd16(b | 0x0D5B) - 1)));
            inc16(b | 0x0D65);
            inc16(b | 0x0D65);
            cyc(24);
            cpu.a = rd16(b | 0x0E6C);
            if (cpu.a)
                { cyc(16); sound(0x0027); } else { cyc(14); }
        } else {
            count_bonus(b);
        }
        draw_time(b, 30);
        return;
    case 0x8F86:                        /* 8: shrink the bonus digits */
        cyc(24);
        cpu.a = rd16(b | 0x0D6B);
        if (!cpu.a) {
            cyc(46);
            wr16(b | 0x0D65, 0x0000);
            return;
        }
        a = (uint16_t)(cpu.a - 2);
        cyc(68);
        wr16(b | 0x0D6B, a);
        bonus_x_from_table(b, a);
        (cyc(46), abi_GsuBonusDigitsGrow());
        cyc(22);
        draw_frame(b);
        return;
    case 0x8FD9:                        /* 10: start taking time back */
        cpu.a = 0x0006;
        cyc(48);
        wr16(b | 0x0D6F, cpu.a);
        inc16(b | 0x0D65);
        inc16(b | 0x0D65);
        cpu.a = 0xFFFE;
        cyc(48);
        wr16(b | 0x0D6B, cpu.a);
        cpu.a = 0x0000;
        cyc(48);
        wr16(b | 0x0E74, cpu.a);
        cyc(24);
        cpu.a = rd16(b | TIME_BCD);
        if (cpu.a)
            { cyc(40); wr16(b | 0x0E76, cpu.a); }
        else
            wr16(b | 0x0D65, 0x0000);
        cpu.a &= 0xFF00;
        cyc(62);
        wr8(b | 0x0D5B, 0x00);
        return;
    case 0x900E:                        /* 12: grow the bonus digits */
        cyc(24);
        a = rd16(b | 0x0D6B);
        if (a == 0x0008) {
            cyc(40);
            inc16(b | 0x0D65);
            inc16(b | 0x0D65);
            sound(0x0040);
            cyc(30);                    /* BRA, and 8 the callee lacks */
        } else {
            a = (uint16_t)(a + 2);
            cyc(98);
            wr16(b | 0x0D6B, a);
            bonus_x_from_table(b, a);
        }
        draw_bonus_then_time(b);
        return;
    case 0x904A:                        /* 14: take time into $0E74 */
        cyc(24);
        cpu.a = rd16(b | TIME_BCD);
        if (cpu.a) {
            cyc(46);
            cpu.a = rd16(b | 0x0E74);
            if (cpu.a < 0x0099)
                take_from_time(b); else { cyc(10); }
        } else { cyc(2); }
        cyc(36);
        draw_bonus_then_time(b);
        return;
    case 0x90A1:                        /* 16: shrink the bonus digits */
        cyc(24);
        a = rd16(b | 0x0D6B);
        cpu.a = a;
        if (!a) {
            cyc(46);
            wr16(b | 0x0D65, cpu.x);
            return;
        }
        a = (uint16_t)(a - 2);
        cyc(68);
        wr16(b | 0x0D6B, a);
        cpu.a = 0x0037;
        cyc(48);
        wr16(b | 0x0D6D, cpu.a);
        (cyc(46), abi_TimeDigitBytes());
        cyc(24);
        cpu.x = rd16(b | 0x0D6B);
        cyc(80);
        cpu.a = rd16(SIZE_GROW + cpu.x);
        wr16(GSU_SIZE, cpu.a);
        (cyc(46), abi_GsuDigitPair0E74());
        (cyc(46), abi_GsuTimeDigitsGrow());
        (cyc(22), draw_mirrored());
        return;
    case 0x9116:                        /* 18: start the second animation */
        cyc(24);
        cpu.a = rd16(b | 0x19F0);
        if (!cpu.a) {
            cyc(40);
            wr16(b | 0x0D65, 0x0000);
            return;
        }
        cpu.a = 0x0030;
        cyc(70);
        wr16(b | 0x0D6F, cpu.a);
        inc16(b | 0x0D65);
        inc16(b | 0x0D65);
        sound(0x000B);
        cyc(32);
        wr16(b | 0x0D6B, 0x0000);
        cpu.a = 0x0028;
        cyc(48);
        wr16(b | 0x0E74, cpu.a);
        return;
    case 0x913E:                        /* 20: grow it */
        cyc(24);
        a = rd16(b | 0x0D6B);
        cyc(40);
        if (a == 0x0008) {
            inc16(b | 0x0D65);
            inc16(b | 0x0D65);
        } else { cyc(6); }
        cyc(52);
        wr16(b | 0x0D6B, (uint16_t)(a + 2));
        draw_pieces(b);
        return;
    case 0x9151:                        /* 22: hold it */
        draw_pieces(b);
        return;
    case 0x9165:                        /* 24: shrink it */
        cyc(24);
        a = rd16(b | 0x0D6B);
        cpu.a = a;
        if (!a) {
            cyc(40);
            wr16(b | 0x0D65, 0x0000);
            return;
        }
        cyc(74);
        wr16(b | 0x0D6B, (uint16_t)(a - 2));
        sound(0x0027);
        cyc(30);
        draw_pieces(b);
        return;
    }
}

void abi_TimeBonusTally(void)
{
    uint8_t p = cpu.p;

    TimeBonusTally();
    cpu.p = p;
    cyc(70);
}

/* Register-level steps for the routines below, whose exit registers and
   flags follow every instruction: each sets A (or X) and the flags as the
   65816 instruction named does. */
static void lda16(uint32_t addr)
{
    cyc(24);
    cpu.a = rd16(addr);
    cpu.p = nz16(cpu.p, cpu.a);
}

static void lda16_imm(uint16_t v)
{
    cpu.a = v;
    cpu.p = nz16(cpu.p, v);
}

static void cmp16(uint16_t m)
{
    cpu.p = (uint8_t)((nz16(cpu.p, (uint16_t)(cpu.a - m)) & ~P_C) | (cpu.a >= m ? P_C : (cyc(23), 0)));
}

static void and16(uint16_t m)
{
    cyc(24);
    cpu.a &= m;
    cpu.p = nz16(cpu.p, cpu.a);
}

static void lsr16(void)
{
    cyc(14);
    cpu.p = (uint8_t)((cpu.p & ~P_C) | (cpu.a & 1));
    cpu.a >>= 1;
    cpu.p = nz16(cpu.p, cpu.a);
}

static void asl16(void)
{
    cpu.p = (uint8_t)((cpu.p & ~P_C) | (cpu.a >> 15));
    cpu.a = (uint16_t)(cpu.a << 1);
    cpu.p = nz16(cpu.p, cpu.a);
}

/* CLC / ADC # and SEC / SBC #, binary or decimal by D. */
static void add16(uint16_t m)
{
    cpu.p &= (uint8_t)~P_C;
    cpu.a = adc16p(cpu.a, m, &cpu.p);
}

static void sub16(uint16_t m)
{
    cpu.p |= P_C;
    cpu.a = sbc16p(cpu.a, m, &cpu.p);
}

/* INC of a work-RAM word, N and Z from the result. */
static void inc16_nz(uint32_t addr)
{
    cyc(24);
    uint16_t v = (uint16_t)(rd16(addr) + 1);

    cyc(14);
    wr16(addr, v);
    cpu.p = nz16(cpu.p, v);
}

/* SEP #$20 / REP #$10 / LDX #program / LDA #$07 / JSL GsuRun / REP #$20 /
   REP #$10: run Super FX program $07:program. */
static void gsu_run(uint16_t program)
{
    cpu.p = (uint8_t)((cpu.p | P_M) & ~P_X);
    cpu.x = program;
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x07);
    cpu.p = nz8(cpu.p, 0x07);
    (cyc(146), abi_GsuRun());
    cpu.p &= (uint8_t)~(P_M | P_X);
}

/* JSR GsuParam216A with A = the digit: $70:0216 = the digit's word. */
static void param_digit(void)
{
    cyc(46);
    abi_GsuParam216A();
}

/* The first bytes of TimeBonusTransfer and TimeLeftDigits: SEP #$20 and the
   byte at $0D5B; 0 when it is 0. */
static int bonus_flag(uint32_t b)
{
    uint8_t v;

    cpu.p |= P_M;
    cyc(46);
    v = rd8(b | 0x0D5B);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, v);
    return v;
}

/* The bonus digits at the size in $0D69 ($07:8C73). */
static void transfer_digits(uint32_t b)
{
    cyc(62);
    abi_GsuBytes200_BFBAB9();
    cyc(72);
    lda16(b | 0x0D69);
    cyc(32);
    wr16(GSU_SIZE, cpu.a);
    (cyc(46), abi_GsuDigitPair0E74());
}

/*
 * @routine 07:8BB3 TimeBonusTransfer
 * WHAT IT DOES: While the byte at $0D5B is set, moves the time bonus at
 * $19F0 into the race time at $0E70 (3 a frame, 1 below 5, $17 at a time
 * from $18 up), showing what is left at $0E74 and starting TimeBonusTally's
 * second animation (state 18) when it is idle; with no bonus, counts $0E74
 * up towards $19EE and sends a running TimeBonusTally to state 24. Then
 * one step of a small animation in $0D67 that grows the bonus digits ($0D69
 * up by 4 to $20) and draws them.
 * HOW: Decimal mode for the arithmetic. $0D67 indexes the table at
 * $07:8C43: 0 nothing, 2 grow, 4 draw, 6 shrink (which steps $0D69 up, not
 * down, and is not reached in the traces so far). Drawing is
 * GsuBytes200_BFBAB9 and GsuDigitPair0E74 at size $0D69. The $17 step is
 * not reached in the traces so far.
 * Entry: D = $0000; DB = a bank where $0D5B-$19F0 are work RAM and the
 * digit table is at $8E3E. Exit: when $0D5B is 0, A low byte 0, M 8-bit,
 * Z set; otherwise M 16-bit, X 16-bit, D clear, and A, X and the flags as
 * the last step leaves them (GsuDigitPair0E74 when it draws).
 */
void TimeBonusTransfer(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint16_t step;

    if (!bonus_flag(b))
        { cyc(16); return; } else { cyc(58); }
    cpu.p = (uint8_t)((cpu.p & ~(P_M | P_X)) | P_D);
    lda16(b | 0x19F0);
    if (!cpu.a) {
        cyc(40);
        wr16(b | 0x0E7C, 0x0000);
        lda16(b | 0x0D65);
        if (cpu.a) {
            lda16_imm(0x0018);
            cyc(64);
            wr16(b | 0x0D65, cpu.a);
        } else { cyc(6); }
        cyc(16);
        lda16(b | 0x19EE);
        cyc(24);
        cmp16(rd16(b | 0x0E74));
        if (!(cpu.p & P_Z)) {
            cyc(16);
            lda16(b | 0x0E74);
            add16(0x0001);
            cyc(48);
            wr16(b | 0x0E74, cpu.a);
        }
    cyc(22);
        } else {
        cmp16(0x0018);
        if (cpu.p & P_C) {
            step = 0x0017;
        } else {
            cmp16(0x0005);
            step = (cpu.p & P_C) ? (cyc(29), 0x0003) : 0x0001;
        }
        sub16(step);
        cyc(124);
        wr16(b | 0x19F0, cpu.a);
        cyc(24);
        wr16(b | 0x0E74, cpu.a);
        lda16(b | TIME_BCD);
        add16(step);
        cyc(48);
        wr16(b | TIME_BCD, cpu.a);
        if (step != 0x0003)
            cyc(22);                    /* BRA */
        lda16(b | 0x0D65);
        if (!cpu.a) {
            lda16_imm(0x0012);
            cyc(64);
            wr16(b | 0x0D65, cpu.a);
        } else { cyc(22); }
    }
    cpu.p &= (uint8_t)~P_D;
    cyc(38);
    cpu.x = rd16(b | 0x0D67);
    cpu.p = nz16(cpu.p, cpu.x);
    cyc(46);
    switch (rd16(0x078C43 + cpu.x)) {
    case 0x8C4B:                        /* grow */
        lda16(b | 0x0D69);
        cyc(23);
        cmp16(0x0020);
        if (cpu.p & P_C) {
            cyc(17);
            inc16_nz(b | 0x0D67);
            inc16_nz(b | 0x0D67);
        } else {
            lda16(b | 0x0D69);
            add16(0x0004);
            cyc(48);
            wr16(b | 0x0D69, cpu.a);
        }
        cyc(22);
        transfer_digits(b);
        return;
    case 0x8C73:                        /* draw */
        transfer_digits(b);
        return;
    case 0x8C67:                        /* shrink */
        lda16(b | 0x0D69);
        if (!cpu.a) {
            wr16(b | 0x0D67, 0x0000);
            return;
        }
        inc16_nz(b | 0x0D69);
        transfer_digits(b);
        return;
    }
}

void abi_TimeBonusTransfer(void)
{
    TimeBonusTransfer();
    cyc(42);
}

/*
 * @routine 07:8C82 TimeLeftDigits
 * WHAT IT DOES: Draws the race time at $0E70 (BCD) with Super FX program
 * $07:F6DF while the byte at $0D5B is set and player 1's engine phase
 * ($0DD6) is not $0C: two digits at X $AC, Y 8 (renderer bytes $BF, $BC,
 * $BD below $20, else $BF, $B5, $BB); below 10 one large digit with
 * program $07:FAF2, placed and scaled from the table at $07:8DDA by the
 * high four bits of $0E69, with sound $42 (for player 1) each time it
 * changes. $0E78 keeps the value last drawn.
 * HOW: GsuParam216A per digit and GsuRun. The large digit's table entry is
 * four words: $70:020E and $70:0210, $70:0212 and $70:0214, X, Y. A time
 * of 0 only clears $0E78; times of $100 and more draw three digits from X
 * $9E. Neither is reached in the traces so far.
 * Entry: D = $0000; DB = a bank where the work RAM and the tables at $8DDA
 * and $8E3E are reached. Exit: when $0D5B is 0, A low byte 0, M 8-bit, Z
 * set; when the phase is $0C, A low byte $0C, M 8-bit, Z and C set;
 * otherwise M 16-bit, X 16-bit, and A, X and the flags as the last
 * instruction leaves them (after a draw: A = the table word's high byte
 * and the screen mode value, X = the program, N and Z from the data bank).
 */
/* The clocks to state after the work: the RTS and what precedes it. */
static uint32_t time_left_digits(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint8_t v;

    if (!bonus_flag(b))
        return 58;
    cyc(46);
    v = rd8(b | 0x0DD6);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = (uint8_t)((nz8(cpu.p, (uint8_t)(v - 0x0C)) & ~P_C) | (v >= 0x0C ? (cyc(16), P_C) : (cyc(60), 0)));
    if (v == 0x0C)
        return 58;
    cpu.p &= (uint8_t)~(P_M | P_X);
    lda16(b | TIME_BCD);
    if (!cpu.a) {
        cyc(22);
        lda16(b | 0x0E78);
        if (cpu.a) {
            cyc(40);
            wr16(b | 0x0E78, 0x0000);
            return 42;
        }
        return 64;
    }
    cmp16(0x0100);
    if (cpu.p & P_C) {                  /* three digits */
        cyc(104);
        wr16(b | 0x0E78, cpu.a);
        cpu.a = (uint16_t)(cpu.a << 8 | cpu.a >> 8);
        cpu.p = nz8(cpu.p, (uint8_t)cpu.a);
        cyc(20);
        and16(0x000F);
        param_digit();
        (cyc(62), abi_GsuBytes200_BFB5BB());
        lda16_imm(0x009E);
        cyc(128);
        wr16(GSU_X, cpu.a);
        lda16_imm(0x0008);
        cyc(56);
        wr16(GSU_Y, cpu.a);
        gsu_run(0xF6DF);
        lda16_imm(0x00AC);
        cyc(122);
        wr16(GSU_X, cpu.a);
        lda16(b | 0x0E78);
        and16(0x00FF);
        lsr16(); lsr16(); lsr16(); lsr16();
        param_digit();
        gsu_run(0xF6DF);
        cyc(66);
        lda16(b | 0x0E78);
        and16(0x000F);
        param_digit();
        gsu_run(0xF6DF);
        cyc(28);
        return 58;
    }
    cmp16(0x0010);
    if (cpu.p & P_C) {                  /* two digits */
        cyc(109);
        wr16(b | 0x0E78, cpu.a);
        lsr16(); lsr16(); lsr16(); lsr16();
        param_digit();
        lda16(b | 0x0E78);
        cyc(79);
        cmp16(0x0020);
        if (cpu.p & P_C)
            { cyc(29); abi_GsuBytes200_BFB5BB(); }
        else
            abi_GsuBytes200_BFBCBD();
        lda16_imm(0x00AC);
        cyc(128);
        wr16(GSU_X, cpu.a);
        lda16_imm(0x0008);
        cyc(56);
        wr16(GSU_Y, cpu.a);
        gsu_run(0xF6DF);
        cyc(66);
        lda16(b | TIME_BCD);
        and16(0x000F);
        param_digit();
        gsu_run(0xF6DF);
        cyc(28);
        return 58;
    }
    cyc(104);
    cmp16(rd16(b | 0x0E78));            /* one large digit */
    if (!(cpu.p & P_Z)) {
        cyc(17);
        wr16(b | 0x0E78, cpu.a);
        cyc(86);
        lda16_imm(0x0042);
        abi_SoundPort0QueuePush();
    } else { cyc(22); }
    lda16(b | TIME_BCD);
    param_digit();
    cyc(22);
    lda16(b | 0x0E69);
    and16(0x00FF);
    lsr16(); lsr16(); lsr16(); lsr16();
    asl16(); asl16(); asl16();
    cpu.x = cpu.a;
    cpu.p = nz16(cpu.p, cpu.x);
    lda16(b | (uint16_t)(0x8DDA + cpu.x));
    cyc(110);
    wr16(GSU_STEP, cpu.a);
    cyc(32);
    wr16(GSU_SIZE, cpu.a);
    lda16(b | (uint16_t)(0x8DDC + cpu.x));
    cyc(54);
    wr16(0x700212, cpu.a);
    cyc(32);
    wr16(0x700214, cpu.a);
    lda16(b | (uint16_t)(0x8DDE + cpu.x));
    cyc(54);
    wr16(GSU_X, cpu.a);
    lda16(b | (uint16_t)(0x8DE0 + cpu.x));
    cyc(54);
    wr16(GSU_Y, cpu.a);
    (cyc(62), abi_GsuBytes200_BFB7B6());
    cyc(72);
    gsu_run(0xFAF2);
    cyc(28);
    return 58;
}

void TimeLeftDigits(void)
{
    time_left_digits();
}

void abi_TimeLeftDigits(void)
{
    cyc(time_left_digits());
}

void abi_GsuRun(void);

/* SEP #$20, REP #$10, LDX #$F15B, LDA #$07, JSL GsuRun, REP #$20, REP #$10:
   program $07:F15B (a digit). */
static void run_f15b(void)
{
    op_sep(P_M);
    op_rep(P_X);
    cpu.x = 0xF15B;
    cpu.p = nz16(cpu.p, cpu.x);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x07);
    cpu.p = nz8(cpu.p, 0x07);
    (cyc(124), abi_GsuRun());
    op_rep(P_M | P_X);
}

/*
 * @routine 07:925C GsuDigitPair0E74At
 * WHAT IT DOES: Draws the two digits of $0E74 (high, then low nibble) at
 * (A, 8): GsuParam216A for each, then program $07:F15B.
 * HOW: The x stored once; the second digit's x is where the first left it.
 * Entry: M 16-bit, X 16-bit; A = x. Exit: M 16-bit, X 16-bit; X = $F15B;
 * A as GsuRun leaves it.
 */
void abi_GsuDigitPair0E74At(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;

    cyc(32);
    wr16(0x70020A, cpu.a);
    op_lda16_imm(0x0008);
    op_lda16_imm(0x0008);
    cyc(4);
    wr16(0x70020C, 0x0008);
    cyc(24);
    op_lda16(b | 0x0E74);
    op_lsr16();
    op_lsr16();
    op_lsr16();
    op_lsr16();
    op_and16(0x000F);
    (cyc(124), abi_GsuParam216A());
    run_f15b();
    op_rep(P_M | P_X);
    cyc(46);
    op_lda16(b | 0x0E74);
    op_and16(0x000F);
    (cyc(68), abi_GsuParam216A());
    run_f15b();
    cyc(64);
}

/*
 * @routine 07:917D TimeBonusPiecesMirrored
 * WHAT IT DOES: A step of the time bonus screen nothing reaches: the
 * mirrored renderer bytes (GsuBytes200_BFB7B6), then the second screen's
 * pieces as TimeBonusTally draws them at $07:9157 (without setting
 * $0D6D first).
 * HOW: GsuBytes200_BFB7B6, GsuFA5AThree, GsuFA5AFromTables, GsuFA5ASix,
 * GsuFA1BFromTables; ends pulling the P its caller pushed (PLP, RTS) as
 * the tally's steps do.
 * Entry: M 16-bit, X 16-bit; P on the stack above the return address.
 * Exit: P as pulled; A, X, Y as GsuFA1BFromTables leaves them.
 */
void abi_TimeBonusPiecesMirrored(void)
{
    (cyc(62), abi_GsuBytes200_BFB7B6());
    (cyc(118), abi_GsuFA5AThree());
    (cyc(46), abi_GsuFA5AFromTables());
    abi_GsuFA5ASix();
    (cyc(46), abi_GsuFA1BFromTables());
    cpu.s++;
    cyc(20);
    op_plp(rd8((uint16_t)cpu.s));
}

