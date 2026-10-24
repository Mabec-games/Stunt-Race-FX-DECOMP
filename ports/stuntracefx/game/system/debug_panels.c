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
#include "system_calls.h"
#include "carried.h"
#include "wram_names.h"
#include "wram_struct.h"


void DebugPrint(uint8_t bank);

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

/* The printers a panel uses. */
enum { DEC_WORD, HEX_WORD, DEC_BYTE };

/* N and Z as the last print leaves them, for the adapters. */
static uint8_t pr_nz;

/* LDA #pos, STA $46, LDA #$0000, STA $0307 (16-bit), PLA of the value
   pushed before, and the printer, with the widths p (M 16-bit here).
   Returns A as it leaves it: the value. */
static uint16_t print16(uint8_t bank, int printer, uint16_t v, uint16_t pos, uint8_t p)
{
    cyc(70);
    WR16(dp_46, pos);
    wr16((WA(text_half)), cyc_then(48, 0x0000));
    cyc(98);
    if (printer == DEC_WORD)
        DebugPrintDecimalWord(v);
    else
        DebugPrintHexWord(bank, v, p);
    pr_nz = nz16(0, v);
    return v;
}

/* The same with M 8-bit: $46 and $47 stored as bytes, $0307 a byte, one
   byte pulled into A's low byte (a, A before it). Returns A as the
   printer leaves it. */
static uint16_t print8(uint8_t bank, int printer, uint16_t a, uint8_t v, uint16_t pos, uint8_t p)
{
    cyc(54);
    WR8(dp_46, (uint8_t)pos);
    cyc(32);
    WR8X(dp_46, 1, (uint8_t)(pos >> 8));
    wr8((WA(text_half)), cyc_then(40, 0x00));
    a = (uint16_t)((a & 0xFF00) | v);
    cyc(90);
    if (printer == HEX_WORD) {
        pr_nz = nz8(0, v);
        return DebugPrintHexWord(bank, a, p);
    }
    pr_nz = nz8(0, bank);              /* the long wrapper's last PLB */
    return (uint16_t)((DebugPrintDecimalByte(bank, v, p) & 0xFF00) | v);
}

/* A 16-bit field of a Super FX record (LDA.L $70:field,X). */
static uint16_t fx_field(uint16_t field, uint16_t x)
{
    cyc(16);
    return rd16(GSU_RAM + field + x);
}

/* The same straight after a print: the opcode and operand bytes 16 later. */
static uint16_t fx_field_w(uint16_t field, uint16_t x)
{
    cyc(16);
    return fx_field(field, x);
}

/* What a panel leaves for its adapter: A and X (pn_x_set), C and V
   (pn_cv: bit 0 C, bit 1 V); N and Z are pr_nz. */
static uint16_t pn_a, pn_x;
static int pn_x_set, pn_cv;
static uint8_t pn_c, pn_v;

void DebugBucketsPanel_cr(Carried *cr);
void DebugCarsPanel_cr(Carried *cr);
void DebugCounterPanel_cr(Carried *cr);
void DebugFxTestPanel_cr(Carried *cr);
void DebugFxTestWordPanel_cr(Carried *cr);
void DebugFxWordsPanel_cr(Carried *cr);
void DebugLastCarMotionPanel_cr(Carried *cr);
void DebugLastCarPanel_cr(Carried *cr);
void DebugMatchSpeedPanel_cr(Carried *cr);
void DebugModelChainPanel_cr(Carried *cr);
void DebugModelNamesPanel_cr(Carried *cr);
void DebugPanelSep_cr(Carried *cr);
void DebugTimersPanel_cr(Carried *cr);
void DebugTrafficHexPanel_cr(Carried *cr);
void DebugViewRecordPanel_cr(Carried *cr);
void DebugViewSpeedPanel_cr(Carried *cr);
void DebugWorkBytesPanel_cr(Carried *cr);
static void panel_exit(Carried *cr, uint8_t clear, uint8_t set);

/* The panel's registers over the current ones: the M and X bits it
   clears and sets. */
static void panel_exit(Carried *cr, uint8_t clear, uint8_t set)
{
    cr->a = pn_a;
    if (pn_x_set)
        cr->x = pn_x;
    set_p(cr, (uint8_t)((cr->p & ~(P_N | P_Z | clear)) | pr_nz | set));
    if (pn_cv & 1)
        set_p(cr, (uint8_t)((cr->p & ~P_C) | pn_c));
    if (pn_cv & 2)
        set_p(cr, (uint8_t)((cr->p & ~P_V) | pn_v));
}

/* CLC; ADC (16-bit), binary: C and V to pn_c, pn_v. */
static uint16_t add16(uint16_t a, uint16_t m)
{
    uint16_t r = (uint16_t)(a + m);

    pn_c = (unsigned)a + m > 0xFFFF ? P_C : 0;
    pn_v = (~(a ^ m) & (a ^ r) & 0x8000) ? P_V : 0;
    pn_cv = 3;
    return r;
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
uint16_t DebugTimersPanel(uint8_t bank, uint16_t a)
{
    uint16_t x = 0, pos, v, at;
    uint8_t b, db;

    pn_cv = 0;
    cyc(76);
    WR8(dp_46, 0x00);
    cyc(32);
    WR8X(dp_46, 1, 0x00);
    wr8((WA(text_half)), cyc_then(40, 0x00));
    cyc(54);
    WR8X(dp_54, 2, 0x08);
    cyc(2);
    for (;;) {
        cyc(138);
        b = rd8(TIMER_LIST + x);
        if (!b)
            { cyc(12); break; }
        pos = RD16(dp_46);
        cyc(46);
        WR16(dp_54, x);
        cyc(54);
        v = RD16(dp_54);
        cyc(16);
        WR16(dp_54, (uint16_t)((uint16_t)TIMER_LIST + v));
        DebugPrint(bank);
        x = (uint16_t)(x + 0x0009);
        cyc(366);
        db = rd8(TIMER_LIST + x);
        at = rd16(TIMER_LIST + 1 + x);
        v = rd16(((uint32_t)db << 16) + at);
        (cyc(90), DebugPrintDecimalWord(v));
        x = (uint16_t)(x + 0x0003);
        pos = add16(pos, 0x0040);
        cyc(178);
        WR16(dp_46, pos);
        a = pos;
    }
    pn_a = (uint16_t)(a & 0xFF00);
    pn_x = x;
    pn_x_set = 1;
    pr_nz = P_Z;
    return x;
}

void DebugTimersPanel_cr(Carried *cr)
{
    DebugTimersPanel(cr->db, cr->a);
    panel_exit(cr, P_X, P_M);
}

void abi_DebugTimersPanel(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    DebugTimersPanel_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
uint16_t DebugMatchSpeedPanel(uint8_t bank)
{
    uint16_t x, v;

    pn_cv = 0;
    cyc(54);
    x = rd16(FX_MATCH_2476);
    pn_x = x;
    pn_x_set = 1;
    cyc(30);
    v = fx_field(0x004A, x);
    pn_a = print16(bank, DEC_WORD, v, 0x2840, 0);
    cyc(44);                            /* RTL */
    return pn_a;
}

void DebugMatchSpeedPanel_cr(Carried *cr)
{
    DebugMatchSpeedPanel(cr->db);
    panel_exit(cr, P_M | P_X, 0);
}

void abi_DebugMatchSpeedPanel(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    DebugMatchSpeedPanel_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:F18E DebugLastCarPanel
 * WHAT IT DOES: Prints, in decimal at text position $0440, the word +$6A
 * of the last car's Super FX record (address at $70:2474).
 * HOW: As DebugMatchSpeedPanel.
 * Entry: M and X 16-bit. Exit: X = the record; A = the word, N and Z from
 * it.
 */
uint16_t DebugLastCarPanel(uint8_t bank)
{
    uint16_t x, v;

    pn_cv = 0;
    cyc(32);
    x = rd16(FX_CAR_LAST);
    pn_x = x;
    pn_x_set = 1;
    cyc(30);
    v = fx_field(0x006A, x);
    pn_a = print16(bank, DEC_WORD, v, 0x0440, 0);
    cyc(44);                            /* RTL */
    return pn_a;
}

void DebugLastCarPanel_cr(Carried *cr)
{
    DebugLastCarPanel(cr->db);
    panel_exit(cr, 0, 0);
}

void abi_DebugLastCarPanel(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    DebugLastCarPanel_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:F1A9 DebugPanelNone
 * WHAT IT DOES: Nothing: an empty panel.
 * HOW: RTL.
 * Entry: any. Exit: unchanged.
 */
void DebugPanelNone(void)
{
    cyc(44);
}

/* DebugPanelNone on carried registers: no register work. */
void DebugPanelNone_cr(Carried *cr)
{
    (void)cr;
    DebugPanelNone();
}

void abi_DebugPanelNone(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    DebugPanelNone_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* Fields +$88, +$7A, +$86 and +$84 of the car record at x, printed in
   decimal in four columns of one row. */
static uint16_t car_row(uint8_t bank, uint16_t x, uint16_t row)
{
    cyc(30);
    print16(bank, DEC_WORD, fx_field(0x0088, x), (uint16_t)(0x0400 + row), 0);
    cyc(16);
    print16(bank, DEC_WORD, fx_field(0x007A, x), (uint16_t)(0x1400 + row), 0);
    cyc(16);
    print16(bank, DEC_WORD, fx_field(0x0086, x), (uint16_t)(0x2800 + row), 0);
    cyc(16);
    return print16(bank, DEC_WORD, fx_field(0x0084, x), (uint16_t)(0x3C00 + row), 0);
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
uint16_t DebugCarsPanel(uint8_t bank)
{
    uint16_t x, row;

    pn_cv = 0;
    print16(bank, DEC_WORD, rd16(cyc_then(24, (WA(gsu_wait_polls)))), 0x5040, 0);
    print16(bank, DEC_WORD, rd16(cyc_then(24, (WA(gsu_ab13_polls)))), 0x5080, 0);
    cyc(32);
    x = rd16(FX_CARS);
    pn_a = car_row(bank, x, 0x0040);
    for (row = 0x0080; row <= 0x0100; row = (uint16_t)(row + 0x0040)) {
        cyc(16);
        x = fx_field(0x0074, x);
        pn_a = car_row(bank, x, row);
    }
    pn_x = x;
    pn_x_set = 1;
    cyc(44);
    return pn_a;
}

void DebugCarsPanel_cr(Carried *cr)
{
    DebugCarsPanel(cr->db);
    panel_exit(cr, 0, 0);
}

void abi_DebugCarsPanel(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    DebugCarsPanel_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:F337 DebugFxTestPanel
 * WHAT IT DOES: Prints the Super FX words $70:235C and $70:235E in
 * decimal at $3980 and $39C0.
 * HOW: As DebugCarsPanel.
 * Entry: M and X 16-bit. Exit: A = the second word, N and Z from it.
 */
uint16_t DebugFxTestPanel(uint8_t bank)
{
    pn_cv = pn_x_set = 0;
    cyc(32);
    print16(bank, DEC_WORD, rd16(FX_RAM_TEST), 0x3980, 0);
    cyc(32);
    pn_a = print16(bank, DEC_WORD, rd16(FX_PART_Z_SPEED), 0x39C0, 0);
    cyc(44);                            /* RTL */
    return pn_a;
}

void DebugFxTestPanel_cr(Carried *cr)
{
    DebugFxTestPanel(cr->db);
    panel_exit(cr, 0, 0);
}

void abi_DebugFxTestPanel(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    DebugFxTestPanel_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
void DebugModelNamesPanel(uint8_t bank)
{
    uint16_t x, part, model, pos;

    pn_cv = 0;
    cyc(32);
    x = rd16(FX_CAR_LAST);
    cyc(30);
    x = fx_field(0x003E, x);
    cyc(84);
    WR16(dp_46, 0x0840);
    wr16((WA(text_half)), cyc_then(48, 0x0000));
    cyc(16);
    WR16X(dp_54, 1, 0x0000);
    do {
        cyc(82);
        part = fx_field(0x0018, x);
        cyc(76);
        while (part) {
            model = fx_field(0x000C, part);
            cyc(30);
            WR16(dp_54, cyc_then(54, add16(fx_field(0x0014, model), 0x001C)));
            cyc(16);
            pos = RD16(dp_46);
            (cyc(30), DebugPrint(bank));
            cyc(112);
            WR16(dp_46, add16(pos, 0x0040));
            cyc(52);
            part = fx_field(0x000E, part);
        }
        cyc(68);
        x = fx_field(0x0034, x);
    } while (x);
    cyc(74);
    pn_a = pn_x = 0x0000;
    pn_x_set = 1;
    pr_nz = P_Z;
}

void DebugModelNamesPanel_cr(Carried *cr)
{
    DebugModelNamesPanel(cr->db);
    panel_exit(cr, 0, 0);
}

void abi_DebugModelNamesPanel(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    DebugModelNamesPanel_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
uint16_t DebugModelChainPanel(uint8_t bank)
{
    uint16_t x, pos, v;

    pn_cv = 0;
    cyc(54);
    x = rd16(FX_CAR_LAST);
    cyc(30);
    x = fx_field(0x003E, x);
    cyc(30);
    print16(bank, DEC_WORD, fx_field(0x0042, x), 0x08C0, 0);
    for (pos = 0x0900; pos <= 0x0980; pos = (uint16_t)(pos + 0x0040)) {
        cyc(16);
        x = fx_field(0x0034, x);
        cyc(30);
        print16(bank, DEC_WORD, fx_field(0x0042, x), pos, 0);
    }
    cyc(16);
    fx_field(0x0034, x);
    x = rd16(cyc_then(24, (WA(wram_0346))));
    cyc(32);
    x = RD16X(obj_22_long, x);
    cyc(30);
    v = fx_field(0x0080, x);
    pn_x = x;
    pn_x_set = 1;
    pn_a = v;
    pr_nz = nz16(0, v);
    cyc(44);                            /* RTL */
    return v;
}

void DebugModelChainPanel_cr(Carried *cr)
{
    DebugModelChainPanel(cr->db);
    panel_exit(cr, P_M, 0);
}

void abi_DebugModelChainPanel(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    DebugModelChainPanel_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
uint8_t DebugLastCarMotionPanel(uint8_t bank)
{
    uint16_t x, a;
    uint8_t b;

    pn_cv = 0;
    cyc(32);
    x = rd16(FX_CAR_LAST);
    cyc(30);
    print16(bank, DEC_WORD, fx_field(0x004E, x), 0x4900, 0);
    cyc(16);
    print16(bank, DEC_WORD, fx_field(0x004A, x), 0x4940, 0);
    cyc(16);
    print16(bank, DEC_WORD, fx_field(0x002E, x), 0x4980, 0);
    cyc(16);
    print16(bank, DEC_WORD, fx_field(0x0030, x), 0x49C0, 0);
    cyc(16);
    print16(bank, DEC_WORD, fx_field(0x0032, x), 0x4A00, 0);
    cyc(32);
    print16(bank, DEC_WORD, rd16(FX_TRAFFIC_CLOSE), 0x1C40, 0);
    cyc(32);
    a = print16(bank, DEC_WORD, rd16(FX_TRAFFIC_DIST), 0x1C80, 0);
    b = rd8(cyc_then(46, (WA(wram_1F41))));
    pn_a = (uint16_t)((a & 0xFF00) | b);
    pn_x = x;
    pn_x_set = 1;
    pr_nz = nz8(0, b);
    cyc(44);                            /* RTL */
    return b;
}

void DebugLastCarMotionPanel_cr(Carried *cr)
{
    DebugLastCarMotionPanel(cr->db);
    panel_exit(cr, 0, P_M);
}

void abi_DebugLastCarMotionPanel(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    DebugLastCarMotionPanel_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* X = the viewed object's statistics record: the byte at +$22 of the
   object at $0346 under A's high byte (TAX with M 8-bit). */
static uint16_t view_record8(uint16_t a)
{
    uint16_t x;
    uint8_t b;

    x = rd16(cyc_then(24, (WA(wram_0346))));
    cyc(32);
    b = RD8X(obj_22_long, x);
    return (uint16_t)((a & 0xFF00) | b);
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
 * Entry: M 8-bit, X 16-bit; a = A, p the widths. Exit: X = the record; A
 * and P as the last DebugPrintHexWord leaves them.
 */
uint16_t DebugTrafficHexPanel(uint8_t bank, uint16_t a, uint8_t p)
{
    uint16_t x;

    pn_cv = 0;
    cyc(32);
    a = print8(bank, HEX_WORD, a, rd8(FX_RAM_TEST), 0x0940, p);
    cyc(32);
    a = print8(bank, HEX_WORD, a, rd8(FX_PART_Z_SPEED), 0x0980, p);
    cyc(32);
    a = print8(bank, HEX_WORD, a, rd8(FX_TRAFFIC_CLOSE), 0x1D40, p);
    cyc(32);
    a = print8(bank, HEX_WORD, a, rd8(FX_TRAFFIC_DIST), 0x1D80, p);
    x = view_record8(a);
    cyc(46);
    pn_a = print8(bank, HEX_WORD, a, rd8(GSU_RAM + 0x0046 + x), 0x3140, p);
    pn_x = x;
    pn_x_set = 1;
    cyc(44);                            /* RTL */
    return pn_a;
}

void DebugTrafficHexPanel_cr(Carried *cr)
{
    DebugTrafficHexPanel(cr->db, cr->a, cr->p);
    panel_exit(cr, 0, 0);
}

void abi_DebugTrafficHexPanel(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    DebugTrafficHexPanel_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:F54B DebugWorkBytesPanel
 * WHAT IT DOES: Prints in hexadecimal (a byte each) $1F41 at $4940, the
 * negated byte $70:235E shifted right five places with its sign at $4980,
 * $1F43 at $49C0, then $1F41 and $1F43 again at $0940 and $0980.
 * HOW: EOR #$FF, INC A, then five CMP #$80, ROR A (an arithmetic shift);
 * printing as DebugTrafficHexPanel.
 * Entry: M 8-bit, X 16-bit; a = A, p the widths. Exit: A and P as the last
 * DebugPrintHexWord leaves them, C from the last ROR.
 */
uint16_t DebugWorkBytesPanel(uint8_t bank, uint16_t a, uint8_t p)
{
    uint8_t v, c = 0;
    int i;

    pn_cv = pn_x_set = 0;
    a = print8(bank, HEX_WORD, a, rd8(cyc_then(24, (WA(wram_1F41)))), 0x4940, p);
    cyc(32);
    v = rd8(FX_PART_Z_SPEED);
    v = (uint8_t)((v ^ 0xFF) + 1);
    for (i = 0; i < 5; i++) {
        cyc(36);
        c = v & 1;
        v = (uint8_t)((v >> 1) | (v & 0x80));
    }
    pn_c = c;
    pn_cv = 1;
    a = print8(bank, HEX_WORD, a, v, 0x4980, p);
    a = print8(bank, HEX_WORD, a, rd8(cyc_then(24, (WA(wram_1F43)))), 0x49C0, p);
    a = print8(bank, HEX_WORD, a, rd8(cyc_then(24, (WA(wram_1F41)))), 0x0940, p);
    pn_a = print8(bank, HEX_WORD, a, rd8(cyc_then(24, (WA(wram_1F43)))), 0x0980, p);
    cyc(44);                            /* RTL */
    return pn_a;
}

void DebugWorkBytesPanel_cr(Carried *cr)
{
    DebugWorkBytesPanel(cr->db, cr->a, cr->p);
    panel_exit(cr, 0, 0);
}

void abi_DebugWorkBytesPanel(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    DebugWorkBytesPanel_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:F5CD DebugViewRecordPanel
 * WHAT IT DOES: Prints in hexadecimal (a byte each) the bytes +$10 and
 * +$58 of the viewed object's statistics record at $49C0 and $0880, and
 * the Super FX byte $70:2638 at $0900.
 * HOW: As DebugTrafficHexPanel.
 * Entry: M 8-bit, X 16-bit; a = A, p the widths. Exit: X = the record; A
 * and P as the last DebugPrintHexWord leaves them.
 */
uint16_t DebugViewRecordPanel(uint8_t bank, uint16_t a, uint8_t p)
{
    uint16_t x = view_record8(a);

    pn_cv = 0;
    cyc(46);
    a = print8(bank, HEX_WORD, a, rd8(GSU_RAM + 0x0010 + x), 0x49C0, p);
    cyc(32);
    a = print8(bank, HEX_WORD, a, rd8(GSU_RAM + 0x0058 + x), 0x0880, p);
    cyc(32);
    pn_a = print8(bank, HEX_WORD, a, rd8(FX_TRAFFIC_CLOSE), 0x0900, p);
    pn_x = x;
    pn_x_set = 1;
    cyc(44);                            /* RTL */
    return pn_a;
}

void DebugViewRecordPanel_cr(Carried *cr)
{
    DebugViewRecordPanel(cr->db, cr->a, cr->p);
    panel_exit(cr, 0, 0);
}

void abi_DebugViewRecordPanel(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    DebugViewRecordPanel_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:F61B DebugViewSpeedPanel
 * WHAT IT DOES: Prints in hexadecimal (a byte each) the speed byte +$4A
 * of the viewed object's statistics record at $4B00, the Super FX byte
 * $70:2482 at $4440 and $1F64 at $44C0.
 * HOW: As DebugTrafficHexPanel.
 * Entry: M 8-bit, X 16-bit; a = A, p the widths. Exit: X = the record; A
 * and P as the last DebugPrintHexWord leaves them.
 */
uint16_t DebugViewSpeedPanel(uint8_t bank, uint16_t a, uint8_t p)
{
    uint16_t x = view_record8(a);

    pn_cv = 0;
    cyc(46);
    a = print8(bank, HEX_WORD, a, rd8(GSU_RAM + 0x004A + x), 0x4B00, p);
    cyc(32);
    a = print8(bank, HEX_WORD, a, rd8(FX_RECORD_READY), 0x4440, p);
    pn_a = print8(bank, HEX_WORD, a, rd8(cyc_then(24, (WA(wram_1F64)))), 0x44C0, p);
    pn_x = x;
    pn_x_set = 1;
    cyc(44);                            /* RTL */
    return pn_a;
}

void DebugViewSpeedPanel_cr(Carried *cr)
{
    DebugViewSpeedPanel(cr->db, cr->a, cr->p);
    panel_exit(cr, 0, 0);
}

void abi_DebugViewSpeedPanel(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    DebugViewSpeedPanel_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* Prints the object at the head of bucket x, then up to three more along
   their +$3C links, at pos, pos + $1400, + $2800, + $3C00, + $5000
   (hexadecimal words); stops after the print of a zero (X = A = 0, Z
   set). */
static void bucket_row(uint8_t bank, uint16_t x, uint16_t pos)
{
    uint16_t v;
    int i;

    cyc(46);
    v = RD16X(object_buckets, x);
    pn_a = print16(bank, HEX_WORD, v, pos, 0);
    for (i = 1; i < 5; i++) {
        cyc(60);
        x = pn_a;
        pn_x = x;
        if (!x) {
            pr_nz = P_Z;
            return;
        }
        cyc(2);
        v = RD16X(wram_7E203C, x);
        pn_a = print16(bank, HEX_WORD, v, (uint16_t)(pos + 0x1400 * i), 0);
    }
    cyc(24);
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
 * Entry: any widths (p). Exit: M and X 16-bit; X = the last object printed
 * of the second row, or 0; A and P as the last print or TAX leaves them, C
 * from the last LSR of the height.
 */
uint16_t DebugBucketsPanel(uint8_t bank, uint8_t p)
{
    uint16_t v, x;

    v = (uint16_t)(rd16(cyc_then(46, (WA(view_pos_y_hi)))) & 0x00FF);
    pn_c = (uint8_t)(v >> 3 & 1);
    pn_cv = 1;
    v = (uint16_t)(((v >> 4) + 1) & 0x00FE);
    cyc(118);
    print16(bank, HEX_WORD, v, 0x1C40, (uint8_t)(p & P_X));
    print16(bank, HEX_WORD, rd16(cyc_then(32, FX_PUSH_TURN)), 0x3040, (uint8_t)(p & P_X));
    x = rd16(cyc_then(46, (WA(wram_0346))));
    cyc(32);
    print16(bank, HEX_WORD, RD16X(obj_bucket_long, x), 0x1E80, 0);
    print16(bank, HEX_WORD, rd16(cyc_then(24, (WA(view_area_origin)))), 0x1EC0, 0);
    x = rd16(cyc_then(46, (WA(wram_0346))));
    cyc(32);
    v = RD16X(obj_bucket_long, x);
    bucket_row(bank, v, 0x0900);
    v = rd16((WA(view_area_origin)));
    pn_x = v;
    bucket_row(bank, v, 0x0940);
    pn_x_set = 1;
    cyc(20);                            /* RTL */
    return pn_a;
}

void DebugBucketsPanel_cr(Carried *cr)
{
    DebugBucketsPanel(cr->db, cr->p);
    panel_exit(cr, P_M | P_X, 0);
}

void abi_DebugBucketsPanel(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    DebugBucketsPanel_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:F7C5 DebugFxWordsPanel
 * WHAT IT DOES: Prints in hexadecimal words the Super FX word $70:235C at
 * $4940 and the last car's +$80 at $4A00.
 * HOW: As DebugBucketsPanel.
 * Entry: M and X 16-bit (p the widths). Exit: X = the last car's record; A
 * and P as the last DebugPrintHexWord leaves them.
 */
uint16_t DebugFxWordsPanel(uint8_t bank, uint8_t p)
{
    uint16_t x;

    pn_cv = 0;
    cyc(32);
    print16(bank, HEX_WORD, rd16(FX_RAM_TEST), 0x4940, p);
    cyc(32);
    x = rd16(FX_CAR_LAST);
    cyc(30);
    pn_a = print16(bank, HEX_WORD, fx_field(0x0080, x), 0x4A00, p);
    pn_x = x;
    pn_x_set = 1;
    cyc(44);                            /* RTL */
    return pn_a;
}

void DebugFxWordsPanel_cr(Carried *cr)
{
    DebugFxWordsPanel(cr->db, cr->p);
    panel_exit(cr, 0, 0);
}

void abi_DebugFxWordsPanel(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    DebugFxWordsPanel_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
 * Entry: X 16-bit or 8-bit (p); a = A. Exit: M and X 16-bit; X = the last
 * car's record; A and P as the last DebugPrintHexWord leaves them.
 */
uint16_t DebugCounterPanel(uint8_t bank, uint16_t a, uint8_t p)
{
    uint16_t x;
    uint8_t xw = (uint8_t)(p & P_X);

    pn_cv = 0;
    cyc(54);
    print8(bank, DEC_BYTE, a, rd8(FX_GSU_108E), 0x0440, (uint8_t)(P_M | xw));
    print16(bank, HEX_WORD, rd16(cyc_then(46, (WA(gsu_wait_polls)))), 0x0480, xw);
    a = print16(bank, HEX_WORD, rd16(cyc_then(24, (WA(gsu_ab13_polls)))), 0x04C0, xw);
    x = rd16(cyc_then(46, (WA(wram_0346))));
    cyc(54);
    a = print8(bank, DEC_BYTE, a, RD8X(wram_7E204C, x), 0x0500, P_M);
    cyc(32);
    print8(bank, DEC_BYTE, a, RD8X(wram_7E204E, x), 0x0540, P_M);
    cyc(54);
    x = rd16(FX_CAR_LAST);
    cyc(30);
    print16(bank, HEX_WORD, fx_field(0x0050, x), 0x0600, 0);
    print16(bank, HEX_WORD, fx_field_w(0x004E, x), 0x0640, 0);
    print16(bank, HEX_WORD, fx_field_w(0x004A, x), 0x0680, 0);
    print16(bank, HEX_WORD, fx_field_w(0x003C, x), 0x06C0, 0);
    print16(bank, HEX_WORD, fx_field_w(0x0056, x), 0x0700, 0);
    cyc(32);
    pn_a = print16(bank, HEX_WORD, rd16(FX_RAM_TEST), 0x0740, 0);
    pn_x = x;
    pn_x_set = 1;
    cyc(44);                            /* RTL */
    return pn_a;
}

void DebugCounterPanel_cr(Carried *cr)
{
    DebugCounterPanel(cr->db, cr->a, cr->p);
    panel_exit(cr, P_M | P_X, 0);
}

void abi_DebugCounterPanel(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    DebugCounterPanel_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:F8F3 DebugPanelSep
 * WHAT IT DOES: Sets M 8-bit and returns: an empty panel.
 * HOW: SEP #$20, RTL.
 * Entry: any. Exit: M 8-bit; the rest unchanged.
 */
void DebugPanelSep(void)
{
    cyc(66);
}

void DebugPanelSep_cr(Carried *cr)
{
    DebugPanelSep();
    set_p(cr, (uint8_t)(cr->p | (P_M)));
}

void abi_DebugPanelSep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    DebugPanelSep_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
uint16_t DebugFxTestWordPanel(uint8_t bank)
{
    pn_cv = pn_x_set = 0;
    rd16(cyc_then(46, (WA(wram_1F1B))));
    cyc(32);
    pn_a = print16(bank, HEX_WORD, rd16(FX_RAM_TEST), 0x5040, 0);
    cyc(44);                            /* RTL */
    return pn_a;
}

void DebugFxTestWordPanel_cr(Carried *cr)
{
    DebugFxTestWordPanel(cr->db);
    panel_exit(cr, P_M | P_X, 0);
}

void abi_DebugFxTestWordPanel(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    DebugFxTestWordPanel_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

