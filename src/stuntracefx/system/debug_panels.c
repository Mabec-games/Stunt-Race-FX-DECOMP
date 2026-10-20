/*
 * debug_panels.c: the developers' debug panels of bank 08: numbers and
 * names read from work RAM and from the Super FX's car and model records,
 * printed into the Super FX picture with the printers of bank 03
 * (debug_numbers.c, debug_text.c). Each panel prints at fixed text
 * positions ($46, eight columns of text per $40, the half in $0307 = 0).
 * Nothing in the game calls or points at them; unit_debug_panels_08 calls
 * them from a race.
 */
#include "machine.h"
#include "cpu_ops.h"

void abi_DebugPrint(void);
void abi_DebugPrintDecimalWord(void);
void abi_DebugPrintDecimalByte(void);
void abi_DebugPrintHexWord(void);

#define TEXT_POS 0x000046u
#define TEXT_PTR 0x000054u
#define TEXT_HALF 0x000307u
#define VIEW_OBJECT 0x000346u
#define VIEW_POS_Y_HI 0x000667u
#define VIEW_AREA 0x001EF5u
#define DRAW_SIDE_20 0x001F1Bu
#define GSU_WAIT_POLLS 0x001F1Fu
#define GSU_AB13_POLLS 0x001F21u
#define WORK_1F41 0x001F41u
#define WORK_1F43 0x001F43u
#define WORK_1F64 0x001F64u
#define OBJ_STATS 0x7E2022u
#define OBJ_BUCKET 0x7E2040u
#define OBJ_4C 0x7E204Cu
#define OBJ_4E 0x7E204Eu
#define OBJ_NEXT 0x7E203Cu
#define OBJECT_BUCKETS 0x7F0000u
#define GSU_RAM 0x700000u
#define FX_GSU_108E 0x70108Eu
#define FX_RAM_TEST 0x70235Cu
#define FX_PART_Z_SPEED 0x70235Eu
#define FX_CAR_LAST 0x702474u
#define FX_MATCH_2476 0x702476u
#define FX_PUSH_TURN 0x70247Eu
#define FX_RECORD_READY 0x702482u
#define FX_CARS 0x70262Eu
#define FX_TRAFFIC_CLOSE 0x702638u
#define FX_TRAFFIC_DIST 0x70263Au
#define TIMER_LIST 0x08FE5Eu

/* A data-bank address. */
static uint32_t dbw(uint16_t a)
{
    return (uint32_t)cpu.db << 16 | a;
}

/* LDA #pos, STA $46, LDA #$0000, STA $0307 (16-bit), PLA of the value
   pushed before, and the printer: the value in A with N and Z from it. */
static void print16(void (*print)(void), uint16_t v, uint16_t pos)
{
    cyc(70);
    wr16(TEXT_POS, pos);
    wr16(dbw(TEXT_HALF), cyc_then(48, 0x0000));
    cpu.a = v;
    cpu.p = nz16(cpu.p, v);
    (cyc(98), print());
}

/* The same with M 8-bit: $46 and $47 stored as bytes, $0307 a byte, one
   byte pulled into A's low byte. */
static void print8(void (*print)(void), uint8_t v, uint16_t pos)
{
    cyc(54);
    wr8(TEXT_POS, (uint8_t)pos);
    cyc(32);
    wr8(TEXT_POS + 1, (uint8_t)(pos >> 8));
    wr8(dbw(TEXT_HALF), cyc_then(40, 0x00));
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, v);
    (cyc(90), print());
}

/* A 16-bit field of a Super FX record (LDA.L $70:field,X). */
static uint16_t fx_field(uint16_t field, uint16_t x)
{
    cyc(16);
    return rd16(GSU_RAM + field + x);
}

/*
 * @routine 08:F10D DebugTimersPanel
 * WHAT IT DOES: Prints the list of timers at $08:FE5E down the left of
 * the picture, one a row: each entry's name (GAME_TM, DISP_TM, DYN__TM,
 * STRA_TM, TMP1_TM to TMP3_TM) and the word at its long address in
 * decimal (the frame counters at $1A3B-$1A53).
 * HOW: An entry is nine bytes of zero-terminated name and a long pointer;
 * a zero first byte ends the list. The name is printed with DebugPrint
 * from $08:FE5E + the entry's offset; the word is read with the data bank
 * set to the pointer's bank (PHA, PLB), then DebugPrintDecimalWord. The
 * text position starts at 0 and steps $40 a row.
 * Entry: any widths. Exit: M 8-bit, X 16-bit; X = the offset of the
 * list's end; A low byte 0, its high byte the last text position's; Z
 * set, N clear; C, V from the last step of the text position.
 */
void DebugTimersPanel(void)
{
    uint16_t x = 0, pos, v, at;
    uint8_t p, b, db;

    cpu.p = (uint8_t)((cpu.p | P_M) & ~P_X);
    cyc(76);
    wr8(TEXT_POS, 0x00);
    cyc(32);
    wr8(TEXT_POS + 1, 0x00);
    wr8(dbw(TEXT_HALF), cyc_then(40, 0x00));
    cyc(54);
    wr8(TEXT_PTR + 2, 0x08);
    cyc(2);
    for (;;) {
        cyc(138);
        b = rd8(TIMER_LIST + x);
        if (!b)
            { cyc(12); break; }
        pos = rd16(TEXT_POS);
        cyc(46);
        wr16(TEXT_PTR, x);
        p = (uint8_t)(cpu.p & ~P_C);
        cyc(54);
        v = rd16(TEXT_PTR);
        cyc(16);
        wr16(TEXT_PTR, op_adc16((uint16_t)TIMER_LIST, v, &p));
        cpu.p = p;
        abi_DebugPrint();
        p &= (uint8_t)~P_C;
        x = op_adc16(x, 0x0009, &p);
        cyc(366);
        db = rd8(TIMER_LIST + x);
        at = rd16(TIMER_LIST + 1 + x);
        v = rd16(((uint32_t)db << 16) + at);
        cpu.a = v;
        cpu.x = at;
        cpu.p = nz16(p, v);
        (cyc(90), abi_DebugPrintDecimalWord());
        p &= (uint8_t)~P_C;
        x = op_adc16(x, 0x0003, &p);
        p &= (uint8_t)~P_C;
        pos = op_adc16(pos, 0x0040, &p);
        cyc(178);
        wr16(TEXT_POS, pos);
        cpu.a = pos;
        cpu.p = p;
    }
    cpu.a &= 0xFF00;
    cpu.x = x;
    cpu.p = nz8(cpu.p, 0x00);
}

void abi_DebugTimersPanel(void)
{
    DebugTimersPanel();
}

/*
 * @routine 08:F171 DebugMatchSpeedPanel
 * WHAT IT DOES: Prints, in decimal at text position $2840, the speed word
 * (+$4A) of the Super FX record whose address is at $70:2476.
 * HOW: The record address read into X, the field read and printed with
 * DebugPrintDecimalWord.
 * Entry: any widths. Exit: M and X 16-bit; X = the record; A = the speed,
 * N and Z from it.
 */
void DebugMatchSpeedPanel(void)
{
    uint16_t x, v;

    cpu.p &= (uint8_t)~(P_M | P_X);
    cyc(54);
    x = rd16(FX_MATCH_2476);
    cpu.x = x;
    cyc(30);
    v = fx_field(0x004A, x);
    print16(abi_DebugPrintDecimalWord, v, 0x2840);
}

void abi_DebugMatchSpeedPanel(void)
{
    DebugMatchSpeedPanel();
    cyc(44);                            /* RTL */
}

/*
 * @routine 08:F18E DebugLastCarPanel
 * WHAT IT DOES: Prints, in decimal at text position $0440, the word +$6A
 * of the last car's Super FX record (address at $70:2474).
 * HOW: As DebugMatchSpeedPanel.
 * Entry: M and X 16-bit. Exit: X = the record; A = the word, N and Z from
 * it.
 */
void DebugLastCarPanel(void)
{
    uint16_t x, v;

    cyc(32);
    x = rd16(FX_CAR_LAST);
    cpu.x = x;
    cyc(30);
    v = fx_field(0x006A, x);
    print16(abi_DebugPrintDecimalWord, v, 0x0440);
}

void abi_DebugLastCarPanel(void)
{
    DebugLastCarPanel();
    cyc(44);                            /* RTL */
}

/*
 * @routine 08:F1A9 DebugPanelNone
 * WHAT IT DOES: Nothing: an empty panel.
 * HOW: RTL.
 * Entry: any. Exit: unchanged.
 */
void DebugPanelNone(void)
{
}

void abi_DebugPanelNone(void)
{
    cyc(44);
}

/* Fields +$88, +$7A, +$86 and +$84 of the car record at X, printed in
   decimal in four columns of one row. */
static void car_row(uint16_t x, uint16_t row)
{
    cyc(30);
    print16(abi_DebugPrintDecimalWord, fx_field(0x0088, x), (uint16_t)(0x0400 + row));
    cyc(16);
    print16(abi_DebugPrintDecimalWord, fx_field(0x007A, x), (uint16_t)(0x1400 + row));
    cyc(16);
    print16(abi_DebugPrintDecimalWord, fx_field(0x0086, x), (uint16_t)(0x2800 + row));
    cyc(16);
    print16(abi_DebugPrintDecimalWord, fx_field(0x0084, x), (uint16_t)(0x3C00 + row));
}

/*
 * @routine 08:F1AA DebugCarsPanel
 * WHAT IT DOES: Prints the two Super FX wait counters ($1F1F, $1F21) at
 * $5040 and $5080, then for the first four cars of the car list (from
 * $70:262E, each next one at +$74 of the one before) four fields of the
 * record (+$88, +$7A, +$86, +$84) in decimal, a car a row from $0440.
 * HOW: Each value read, pushed while the text position is set, pulled and
 * printed with DebugPrintDecimalWord.
 * Entry: M and X 16-bit. Exit: X = the fourth car; A = its +$84, N and Z
 * from it.
 */
void DebugCarsPanel(void)
{
    uint16_t x, row;

    print16(abi_DebugPrintDecimalWord, rd16(cyc_then(24, dbw(GSU_WAIT_POLLS))), 0x5040);
    print16(abi_DebugPrintDecimalWord, rd16(cyc_then(24, dbw(GSU_AB13_POLLS))), 0x5080);
    cyc(32);
    x = rd16(FX_CARS);
    cpu.x = x;
    car_row(x, 0x0040);
    for (row = 0x0080; row <= 0x0100; row = (uint16_t)(row + 0x0040)) {
        cyc(16);
        x = fx_field(0x0074, x);
        cpu.x = x;
        car_row(x, row);
    }
    cyc(44);
}

void abi_DebugCarsPanel(void)
{
    DebugCarsPanel();
}

/*
 * @routine 08:F337 DebugFxTestPanel
 * WHAT IT DOES: Prints the Super FX words $70:235C and $70:235E in
 * decimal at $3980 and $39C0.
 * HOW: As DebugCarsPanel.
 * Entry: M and X 16-bit. Exit: A = the second word, N and Z from it.
 */
void DebugFxTestPanel(void)
{
    cyc(32);
    print16(abi_DebugPrintDecimalWord, rd16(FX_RAM_TEST), 0x3980);
    cyc(32);
    print16(abi_DebugPrintDecimalWord, rd16(FX_PART_Z_SPEED), 0x39C0);
}

void abi_DebugFxTestPanel(void)
{
    DebugFxTestPanel();
    cyc(44);                            /* RTL */
}

/*
 * @routine 08:F362 DebugModelNamesPanel
 * WHAT IT DOES: Prints a name a row from $0840 for each part of each model
 * in the chain of the last car's record: from the record at +$3E of the
 * last car, each record's parts list (+$18, each part's next at +$0E) and
 * the next record at +$34. A part's name is the zero-terminated string in
 * bank 0 at $1C past the word at +$14 of the record at the part's +$0C.
 * HOW: The pointer at direct page $54 gets the bank 0 (STZ $55, 16-bit)
 * and each name's address; DebugPrint prints it; the text position steps
 * $40 a name. The record and the part are kept on the stack.
 * Entry: M and X 16-bit. Exit: A = X = 0; Z set, N clear; C, V from the
 * last step of the text position (from entry when nothing was printed).
 */
void DebugModelNamesPanel(void)
{
    uint16_t x, part, model, pos;
    uint8_t p;

    cyc(32);
    x = rd16(FX_CAR_LAST);
    cpu.x = x;
    cyc(30);
    x = fx_field(0x003E, x);
    cyc(84);
    wr16(TEXT_POS, 0x0840);
    wr16(dbw(TEXT_HALF), cyc_then(48, 0x0000));
    cyc(16);
    wr16(TEXT_PTR + 1, 0x0000);
    p = cpu.p;
    do {
        cyc(82);
        part = fx_field(0x0018, x);
        cyc(76);
        while (part) {
            model = fx_field(0x000C, part);
            p &= (uint8_t)~P_C;
            cyc(30);
            wr16(TEXT_PTR, cyc_then(54, op_adc16(fx_field(0x0014, model), 0x001C, &p)));
            cyc(16);
            pos = rd16(TEXT_POS);
            cpu.a = pos;
            cpu.x = model;
            cpu.p = p;
            (cyc(30), abi_DebugPrint());
            p &= (uint8_t)~P_C;
            cyc(112);
            wr16(TEXT_POS, op_adc16(pos, 0x0040, &p));
            cyc(52);
            part = fx_field(0x000E, part);
        }
        cyc(68);
        x = fx_field(0x0034, x);
    } while (x);
    cyc(74);
    cpu.a = 0x0000;
    cpu.x = 0x0000;
    cpu.p = nz16(p, 0x0000);
}

void abi_DebugModelNamesPanel(void)
{
    DebugModelNamesPanel();
}

/*
 * @routine 08:F3B1 DebugModelChainPanel
 * WHAT IT DOES: Prints in decimal the word +$42 of the first four records
 * of the last car's model chain (from +$3E of the last car's record, each
 * next at +$34), from $08C0 down a row each; then reads the viewed
 * object's statistics record (+$22 of the object at $0346) and its +$80.
 * HOW: As DebugCarsPanel. The last reads are left in X and A.
 * Entry: M 16-bit or 8-bit, X 16-bit. Exit: M 16-bit; X = the viewed
 * object's record; A = its +$80, N and Z from it.
 */
void DebugModelChainPanel(void)
{
    uint16_t x, pos, v;

    cpu.p &= (uint8_t)~P_M;
    cyc(54);
    x = rd16(FX_CAR_LAST);
    cpu.x = x;
    cyc(30);
    x = fx_field(0x003E, x);
    cpu.x = x;
    cyc(30);
    print16(abi_DebugPrintDecimalWord, fx_field(0x0042, x), 0x08C0);
    for (pos = 0x0900; pos <= 0x0980; pos = (uint16_t)(pos + 0x0040)) {
        cyc(16);
        x = fx_field(0x0034, x);
        cpu.x = x;
        cyc(30);
        print16(abi_DebugPrintDecimalWord, fx_field(0x0042, x), pos);
    }
    cyc(16);
    v = fx_field(0x0034, x);
    x = rd16(cyc_then(24, dbw(VIEW_OBJECT)));
    cyc(32);
    v = rd16(OBJ_STATS + x);
    x = v;
    cyc(30);
    v = fx_field(0x0080, x);
    cpu.x = x;
    cpu.a = v;
    cpu.p = nz16(cpu.p, v);
}

void abi_DebugModelChainPanel(void)
{
    DebugModelChainPanel();
    cyc(44);                            /* RTL */
}

/*
 * @routine 08:F431 DebugLastCarMotionPanel
 * WHAT IT DOES: Prints in decimal five words of the last car's record
 * (+$4E, +$4A, +$2E, +$30, +$32) down the column at $4900, then the
 * traffic words $70:2638 and $70:263A at $1C40 and $1C80; returns the byte
 * at $1F41.
 * HOW: As DebugCarsPanel; SEP #$20 and LDA $1F41 at the end.
 * Entry: M and X 16-bit. Exit: M 8-bit; X = the last car's record; A low
 * byte = $1F41, its high byte the traffic distance's; N, Z from the byte.
 */
void DebugLastCarMotionPanel(void)
{
    uint16_t x;
    uint8_t b;

    cyc(32);
    x = rd16(FX_CAR_LAST);
    cpu.x = x;
    cyc(30);
    print16(abi_DebugPrintDecimalWord, fx_field(0x004E, x), 0x4900);
    cyc(16);
    print16(abi_DebugPrintDecimalWord, fx_field(0x004A, x), 0x4940);
    cyc(16);
    print16(abi_DebugPrintDecimalWord, fx_field(0x002E, x), 0x4980);
    cyc(16);
    print16(abi_DebugPrintDecimalWord, fx_field(0x0030, x), 0x49C0);
    cyc(16);
    print16(abi_DebugPrintDecimalWord, fx_field(0x0032, x), 0x4A00);
    cyc(32);
    print16(abi_DebugPrintDecimalWord, rd16(FX_TRAFFIC_CLOSE), 0x1C40);
    cyc(32);
    print16(abi_DebugPrintDecimalWord, rd16(FX_TRAFFIC_DIST), 0x1C80);
    cpu.p |= P_M;
    b = rd8(cyc_then(46, dbw(WORK_1F41)));
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | b);
    cpu.p = nz8(cpu.p, b);
}

void abi_DebugLastCarMotionPanel(void)
{
    DebugLastCarMotionPanel();
    cyc(44);                            /* RTL */
}

/*
 * @routine 08:F4CF DebugTrafficHexPanel
 * WHAT IT DOES: Prints in hexadecimal (one byte each: M is 8-bit) the
 * Super FX bytes $70:235C, $70:235E, $70:2638 and $70:263A at $0940,
 * $0980, $1D40 and $1D80, then the byte +$46 of the viewed object's
 * statistics record at $3140.
 * HOW: Each byte pushed while $46, $47 and $0307 are stored a byte at a
 * time, pulled and printed with DebugPrintHexWord. The record address is
 * the byte at +$22 of the object at $0346 with A's high byte above it
 * (TAX with M 8-bit, X 16-bit).
 * Entry: M 8-bit, X 16-bit. Exit: X = the record; A and P as the last
 * DebugPrintHexWord leaves them.
 */
void DebugTrafficHexPanel(void)
{
    uint16_t x;
    uint8_t b;

    cyc(32);
    print8(abi_DebugPrintHexWord, rd8(FX_RAM_TEST), 0x0940);
    cyc(32);
    print8(abi_DebugPrintHexWord, rd8(FX_PART_Z_SPEED), 0x0980);
    cyc(32);
    print8(abi_DebugPrintHexWord, rd8(FX_TRAFFIC_CLOSE), 0x1D40);
    cyc(32);
    print8(abi_DebugPrintHexWord, rd8(FX_TRAFFIC_DIST), 0x1D80);
    x = rd16(cyc_then(24, dbw(VIEW_OBJECT)));
    cyc(32);
    b = rd8(OBJ_STATS + x);
    x = (uint16_t)((cpu.a & 0xFF00) | b);
    cpu.x = x;
    cyc(46);
    print8(abi_DebugPrintHexWord, rd8(GSU_RAM + 0x0046 + x), 0x3140);
}

void abi_DebugTrafficHexPanel(void)
{
    DebugTrafficHexPanel();
    cyc(44);                            /* RTL */
}

/*
 * @routine 08:F54B DebugWorkBytesPanel
 * WHAT IT DOES: Prints in hexadecimal (a byte each) $1F41 at $4940, the
 * negated byte $70:235E shifted right five places with its sign at $4980,
 * $1F43 at $49C0, then $1F41 and $1F43 again at $0940 and $0980.
 * HOW: EOR #$FF, INC A, then five CMP #$80, ROR A (an arithmetic shift);
 * printing as DebugTrafficHexPanel.
 * Entry: M 8-bit, X 16-bit. Exit: A and P as the last DebugPrintHexWord
 * leaves them, C from the last ROR.
 */
void DebugWorkBytesPanel(void)
{
    uint8_t v, c;
    int i;

    print8(abi_DebugPrintHexWord, rd8(cyc_then(24, dbw(WORK_1F41))), 0x4940);
    cyc(32);
    v = rd8(FX_PART_Z_SPEED);
    v = (uint8_t)((v ^ 0xFF) + 1);
    c = 0;
    for (i = 0; i < 5; i++) {
        cyc(36);
        c = v & 1;
        v = (uint8_t)((v >> 1) | (v & 0x80));
    }
    cpu.p = (uint8_t)((cpu.p & ~P_C) | c);
    print8(abi_DebugPrintHexWord, v, 0x4980);
    print8(abi_DebugPrintHexWord, rd8(cyc_then(24, dbw(WORK_1F43))), 0x49C0);
    print8(abi_DebugPrintHexWord, rd8(cyc_then(24, dbw(WORK_1F41))), 0x0940);
    print8(abi_DebugPrintHexWord, rd8(cyc_then(24, dbw(WORK_1F43))), 0x0980);
}

void abi_DebugWorkBytesPanel(void)
{
    DebugWorkBytesPanel();
    cyc(44);                            /* RTL */
}

/* X = the viewed object's statistics record: the byte at +$22 of the
   object at $0346 under A's high byte (TAX with M 8-bit). */
static uint16_t view_record8(void)
{
    uint16_t x;
    uint8_t b;

    x = rd16(cyc_then(24, dbw(VIEW_OBJECT)));
    cyc(32);
    b = rd8(OBJ_STATS + x);
    x = (uint16_t)((cpu.a & 0xFF00) | b);
    cpu.x = x;
    return x;
}

/*
 * @routine 08:F5CD DebugViewRecordPanel
 * WHAT IT DOES: Prints in hexadecimal (a byte each) the bytes +$10 and
 * +$58 of the viewed object's statistics record at $49C0 and $0880, and
 * the Super FX byte $70:2638 at $0900.
 * HOW: As DebugTrafficHexPanel.
 * Entry: M 8-bit, X 16-bit. Exit: X = the record; A and P as the last
 * DebugPrintHexWord leaves them.
 */
void DebugViewRecordPanel(void)
{
    uint16_t x = view_record8();

    cyc(46);
    print8(abi_DebugPrintHexWord, rd8(GSU_RAM + 0x0010 + x), 0x49C0);
    cyc(32);
    print8(abi_DebugPrintHexWord, rd8(GSU_RAM + 0x0058 + x), 0x0880);
    cyc(32);
    print8(abi_DebugPrintHexWord, rd8(FX_TRAFFIC_CLOSE), 0x0900);
}

void abi_DebugViewRecordPanel(void)
{
    DebugViewRecordPanel();
    cyc(44);                            /* RTL */
}

/*
 * @routine 08:F61B DebugViewSpeedPanel
 * WHAT IT DOES: Prints in hexadecimal (a byte each) the speed byte +$4A
 * of the viewed object's statistics record at $4B00, the Super FX byte
 * $70:2482 at $4440 and $1F64 at $44C0.
 * HOW: As DebugTrafficHexPanel.
 * Entry: M 8-bit, X 16-bit. Exit: X = the record; A and P as the last
 * DebugPrintHexWord leaves them.
 */
void DebugViewSpeedPanel(void)
{
    uint16_t x = view_record8();

    cyc(46);
    print8(abi_DebugPrintHexWord, rd8(GSU_RAM + 0x004A + x), 0x4B00);
    cyc(32);
    print8(abi_DebugPrintHexWord, rd8(FX_RECORD_READY), 0x4440);
    print8(abi_DebugPrintHexWord, rd8(cyc_then(24, dbw(WORK_1F64))), 0x44C0);
}

void abi_DebugViewSpeedPanel(void)
{
    DebugViewSpeedPanel();
    cyc(44);                            /* RTL */
}

/* Prints the object at the head of bucket X, then up to three more along
   their +$3C links, at pos, pos + $1400, + $2800, + $3C00, + $5000
   (hexadecimal words); stops after the print of a zero. Returns 0 when it stopped on
   a zero (X = A = 0, Z set), else 1 with A and P as the last print left
   them. */
static int bucket_row(uint16_t x, uint16_t pos)
{
    uint16_t v;
    int i;

    cyc(46);
    v = rd16(OBJECT_BUCKETS + x);
    print16(abi_DebugPrintHexWord, v, pos);
    for (i = 1; i < 5; i++) {
        cyc(44);
        x = cpu.a;
        cpu.x = x;
        cpu.p = nz16(cpu.p, x);
        if (!x)
            return 0;
        cyc(2);
        v = rd16(OBJ_NEXT + x);
        print16(abi_DebugPrintHexWord, v, (uint16_t)(pos + 0x1400 * i));
    }
    cyc(8);
    return 1;
}

/*
 * @routine 08:F668 DebugBucketsPanel
 * WHAT IT DOES: Prints in hexadecimal words: the view's height row
 * ($0667 / 16 + 1, made even) at $1C40, the Super FX turn push $70:247E at
 * $3040, the viewed object's bucket (+$40 of the object at $0346) at
 * $1E80, the view's area bucket $1EF5 at $1EC0; then the objects in each
 * of those two buckets, from $7F:0000 + the bucket and along their +$3C
 * links, up to five a row at $0900 and $0940.
 * HOW: Each value pushed while the text position is set, pulled and
 * printed with DebugPrintHexWord (16-bit: the whole word back in A); a
 * printed zero ends that row.
 * Entry: any widths. Exit: M and X 16-bit; X = the last object printed of
 * the second row, or 0; A and P as the last print or TAX leaves them, C
 * from the last LSR of the height.
 */
void DebugBucketsPanel(void)
{
    uint16_t v, x;

    cpu.p &= (uint8_t)~P_M;
    v = rd16(cyc_then(46, dbw(VIEW_POS_Y_HI))) & 0x00FF;
    cpu.p = (uint8_t)((cpu.p & ~P_C) | (v >> 3 & 1));
    v = (uint16_t)(((v >> 4) + 1) & 0x00FE);
    cyc(118);
    print16(abi_DebugPrintHexWord, v, 0x1C40);
    print16(abi_DebugPrintHexWord, rd16(FX_PUSH_TURN), 0x3040);
    cpu.p &= (uint8_t)~P_X;
    x = rd16(cyc_then(14, dbw(VIEW_OBJECT)));
    cpu.x = x;
    cyc(32);
    print16(abi_DebugPrintHexWord, rd16(OBJ_BUCKET + x), 0x1E80);
    print16(abi_DebugPrintHexWord, rd16(cyc_then(8, dbw(VIEW_AREA))), 0x1EC0);
    x = rd16(cyc_then(30, dbw(VIEW_OBJECT)));
    cyc(32);
    v = rd16(OBJ_BUCKET + x);
    cpu.x = v;
    bucket_row(v, 0x0900);
    v = rd16(dbw(VIEW_AREA));
    cpu.x = v;
    bucket_row(v, 0x0940);
}

void abi_DebugBucketsPanel(void)
{
    DebugBucketsPanel();
    cyc(20);                            /* RTL */
}

/*
 * @routine 08:F7C5 DebugFxWordsPanel
 * WHAT IT DOES: Prints in hexadecimal words the Super FX word $70:235C at
 * $4940 and the last car's +$80 at $4A00.
 * HOW: As DebugBucketsPanel.
 * Entry: M and X 16-bit. Exit: X = the last car's record; A and P as the
 * last DebugPrintHexWord leaves them.
 */
void DebugFxWordsPanel(void)
{
    uint16_t x;

    cyc(32);
    print16(abi_DebugPrintHexWord, rd16(FX_RAM_TEST), 0x4940);
    cyc(16);
    x = rd16(FX_CAR_LAST);
    cpu.x = x;
    cyc(30);
    print16(abi_DebugPrintHexWord, fx_field(0x0080, x), 0x4A00);
}

void abi_DebugFxWordsPanel(void)
{
    DebugFxWordsPanel();
    cyc(28);                            /* RTL */
}

/*
 * @routine 08:F7F5 DebugCounterPanel
 * WHAT IT DOES: Prints a column from $0440: the Super FX byte $70:108E in
 * decimal, the two Super FX wait counters ($1F1F, $1F21) in hexadecimal,
 * the bytes +$4C and +$4E of the object at $0346 in decimal, then six
 * words in hexadecimal: the last car's +$50, +$4E, +$4A, +$3C and +$56,
 * and $70:235C.
 * HOW: Bytes with M 8-bit (text position stored a byte at a time), words
 * with M 16-bit; each value pushed, pulled and printed.
 * Entry: X 16-bit or 8-bit. Exit: M and X 16-bit; X = the last car's
 * record; A and P as the last DebugPrintHexWord leaves them.
 */
void DebugCounterPanel(void)
{
    uint16_t x;

    cpu.p |= P_M;
    cyc(54);
    print8(abi_DebugPrintDecimalByte, rd8(FX_GSU_108E), 0x0440);
    cpu.p &= (uint8_t)~P_M;
    print16(abi_DebugPrintHexWord, rd16(cyc_then(60, dbw(GSU_WAIT_POLLS))), 0x0480);
    print16(abi_DebugPrintHexWord, rd16(cyc_then(8, dbw(GSU_AB13_POLLS))), 0x04C0);
    cpu.p &= (uint8_t)~P_X;
    x = rd16(cyc_then(30, dbw(VIEW_OBJECT)));
    cpu.x = x;
    cpu.p |= P_M;
    cyc(54);
    print8(abi_DebugPrintDecimalByte, rd8(OBJ_4C + x), 0x0500);
    cyc(46);
    print8(abi_DebugPrintDecimalByte, rd8(OBJ_4E + x), 0x0540);
    cpu.p &= (uint8_t)~P_M;
    cyc(68);
    x = rd16(FX_CAR_LAST);
    cpu.x = x;
    cyc(30);
    print16(abi_DebugPrintHexWord, fx_field(0x0050, x), 0x0600);
    print16(abi_DebugPrintHexWord, fx_field(0x004E, x), 0x0640);
    print16(abi_DebugPrintHexWord, fx_field(0x004A, x), 0x0680);
    print16(abi_DebugPrintHexWord, fx_field(0x003C, x), 0x06C0);
    print16(abi_DebugPrintHexWord, fx_field(0x0056, x), 0x0700);
    cyc(16);
    print16(abi_DebugPrintHexWord, rd16(FX_RAM_TEST), 0x0740);
}

void abi_DebugCounterPanel(void)
{
    DebugCounterPanel();
    cyc(28);                            /* RTL */
}

/*
 * @routine 08:F8F3 DebugPanelSep
 * WHAT IT DOES: Sets M 8-bit and returns: an empty panel.
 * HOW: SEP #$20, RTL.
 * Entry: any. Exit: M 8-bit; the rest unchanged.
 */
void DebugPanelSep(void)
{
    cpu.p |= P_M;
}

void abi_DebugPanelSep(void)
{
    (cyc(66), DebugPanelSep());
}

/*
 * @routine 08:F8F6 DebugFxTestWordPanel
 * WHAT IT DOES: Prints the Super FX word $70:235C in hexadecimal at
 * $5040.
 * HOW: Reads $1F1B first (left unused: A is loaded again at once); then
 * as DebugFxWordsPanel.
 * Entry: any widths. Exit: M and X 16-bit; A and P as DebugPrintHexWord
 * leaves them.
 */
void DebugFxTestWordPanel(void)
{
    uint16_t v;

    cpu.p &= (uint8_t)~(P_M | P_X);
    v = rd16(cyc_then(46, dbw(DRAW_SIDE_20)));
    cpu.a = v;
    cyc(32);
    print16(abi_DebugPrintHexWord, rd16(FX_RAM_TEST), 0x5040);
}

void abi_DebugFxTestWordPanel(void)
{
    DebugFxTestWordPanel();
    cyc(28);                            /* RTL */
}
