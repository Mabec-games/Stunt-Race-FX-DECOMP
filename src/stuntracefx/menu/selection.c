/*
 * selection.c: small steps of the menu screens' selection logic: the
 * choices at $18C9-$18D3, values derived from them at $18E3-$1907, and the
 * menu's exit flag at $11EB.
 */
#include "machine.h"
#include "cpu_ops.h"

/*
 * @routine 0B:C58D MenuRowPosition
 * WHAT IT DOES: Returns a position from object X's +$12 and the value at
 * $18ED: (2 - $18ED) * 16 + the field + 2.
 * HOW: 16-bit arithmetic; the field is read as direct page $12 + X.
 * Entry: M 16-bit, D = $0000; X = record. Exit: A = the position; N, Z from
 * it; C, V from the add.
 */
uint16_t MenuRowPosition(uint16_t rec, uint8_t *p)
{
    uint16_t a = (uint16_t)((uint16_t)(2 - rd16(0x0018ED)) << 4), b = rd16((uint16_t)(rec + 0x12));
    unsigned s = (unsigned)a + b;

    *p = (uint8_t)((*p & ~(P_C | P_V)) | (s > 0xFFFF ? P_C : 0) | ((~(a ^ b) & (a ^ (uint16_t)s) & 0x8000) ? P_V : 0));
    a = (uint16_t)(s + 2);
    *p = nz16(*p, a);
    return a;
}

void abi_MenuRowPosition(void)
{
    uint8_t p = cpu.p;

    cpu.a = MenuRowPosition(cpu.x, &p);
    cpu.p = p;
}

/*
 * @routine 0B:C736 MenuChoiceRepeat
 * WHAT IT DOES: Sets $18D3 to the table value for $18C9 when the choice at
 * $18CB equals it, and to 0 otherwise.
 * HOW: Clears $18D3; on a match reads word table $0B:C758[$18C9].
 * Entry: any widths. Exit: M 16-bit, X 16-bit. No match: A = $18CB; N, Z, C
 * from the compare. Match: A = the value, X = $18C9 * 2; N, Z from A; C =
 * bit 15 of $18C9 (shifted out by the doubling).
 */
uint16_t MenuChoiceRepeat(uint16_t *x, uint8_t *p)
{
    uint16_t a = rd16(0x0018CB), b;

    wr16(0x0018D3, 0x0000);
    b = rd16(0x0018C9);
    *p = (uint8_t)((nz16(*p, (uint16_t)(a - b)) & ~P_C) | (a >= b ? P_C : 0));
    if (a != b)
        return a;
    b = rd16(0x0018C9);
    *x = (uint16_t)(b << 1);
    a = rd16(0x0BC758 + *x);
    wr16(0x0018D3, a);
    *p = (uint8_t)((nz16(*p, a) & ~P_C) | (b & 0x8000 ? P_C : 0));
    return a;
}

void abi_MenuChoiceRepeat(void)
{
    uint8_t p = cpu.p;

    cpu.a = MenuChoiceRepeat(&cpu.x, &p);
    cpu.p = (uint8_t)(p & ~(P_M | P_X));
}

/*
 * @routine 0B:BCD2 MenuWaitFor0357
 * WHAT IT DOES: An object handler that waits for the byte at $0357 to become
 * non-zero, then sets $1905 = 2, $1907 = 0 and moves on to handler $BCF1.
 * HOW: Byte test through the data bank; the handler is object X's +$08.
 * Entry: D = $0000; X = record. Exit: M 16-bit. Waiting: A low byte = 0; Z
 * set, N clear. Moved on: A = $BCF1; N set, Z clear.
 */
int MenuWaitFor0357(uint8_t bank, uint16_t rec)
{
    if (!rd8((uint32_t)bank << 16 | 0x0357))
        return 0;
    wr16(0x001905, 0x0002);
    wr16(0x001907, 0x0000);
    wr16((uint16_t)(rec + 0x08), 0xBCF1);
    return 1;
}

void abi_MenuWaitFor0357(void)
{
    if (MenuWaitFor0357(cpu.db, cpu.x)) {
        cpu.a = 0xBCF1;
        cpu.p = (uint8_t)((cpu.p & ~(P_Z | P_M)) | P_N);
    } else {
        cpu.a &= 0xFF00;
        cpu.p = (uint8_t)((cpu.p & ~(P_N | P_M)) | P_Z);
    }
}

/*
 * @routine 0B:D4A5 MenuSetExit
 * WHAT IT DOES: Sets the menu's exit flag at $11EB.
 * HOW: Stores 1 (16-bit) through the data bank.
 * Entry: any widths. Exit: A = 1; M 16-bit; N, Z clear.
 */
void MenuSetExit(uint8_t bank)
{
    wr16((uint32_t)bank << 16 | 0x11EB, 0x0001);
}

void abi_MenuSetExit(void)
{
    MenuSetExit(cpu.db);
    cpu.a = 0x0001;
    cpu.p = (uint8_t)(cpu.p & ~(P_N | P_Z | P_M));
}

/*
 * @routine 0B:C723 MenuSet1A66
 * WHAT IT DOES: Copies entry A of the word table at $00:116C to $1A66.
 * HOW: Long read at $116C + A * 2; the store goes through the data bank.
 * Entry: any widths; A = entry (16 bits). Exit: A unchanged; X = A * 2; Y =
 * $1A64 (low bytes only when X is 8-bit); P unchanged.
 */
void MenuSet1A66(uint8_t bank, uint16_t i)
{
    wr16((uint32_t)bank << 16 | 0x1A66, rd16(0x00116C + (uint16_t)(i << 1)));
}

void abi_MenuSet1A66(void)
{
    MenuSet1A66(cpu.db, cpu.a);
    cpu.x = (uint16_t)(cpu.a << 1);
    cpu.y = 0x1A64;
    if (cpu.p & P_X) {
        cpu.x &= 0x00FF;
        cpu.y &= 0x00FF;
    }
}

/*
 * @routine 0B:D4C3 MenuStateInit
 * WHAT IT DOES: Sets the menu's selection state to its defaults: the four
 * choices at $18CB-$18D1 to 0, 0, 1, 2, the derived values and positions to
 * their starting values, the draw-buffer offset $0D7D to 6, and three
 * values at $18EF-$18F3 from the save data ($70:FC08-$70:FC0A).
 * HOW: 16-bit long stores in the original's order.
 * Entry: any widths. Exit: A = $001F; P unchanged.
 */
void MenuStateInit(void)
{
    static const struct { uint32_t addr; uint16_t v; } set[] = {
        {0x000D7D, 0x0006}, {0x0018FD, 0x0000}, {0x001903, 0x0000}, {0x0018B9, 0x0001},
        {0x001901, 0x0000}, {0x0018FF, 0x0000}, {0x0018F7, 0x0000}, {0x0018C3, 0x0000},
        {0x0018CB, 0x0000}, {0x0018C9, 0x0000}, {0x0018ED, 0x0000}, {0x0018CD, 0x0000},
        {0x0018F5, 0x0000}, {0x0018D3, 0x0000}, {0x0018D5, 0x0000}, {0x0018D7, 0x0000},
        {0x0018D9, 0x0003}, {0x0018C7, 0x0001}, {0x0018EB, 0x0001}, {0x0018CF, 0x0001},
        {0x0018D1, 0x0002}};
    unsigned i;

    for (i = 0; i < sizeof set / sizeof set[0]; i++)
        wr16(set[i].addr, set[i].v);
    wr16(0x0018EF, rd16(0x70FC08) & 0x00FF);
    wr16(0x0018F1, rd16(0x70FC09) & 0x00FF);
    wr16(0x0018F3, rd16(0x70FC0A) & 0x00FF);
    wr16(0x0018C5, 0xFFFF);
    wr16(0x0018F9, 0x001F);
}

void abi_MenuStateInit(void)
{
    MenuStateInit();
    cpu.a = 0x001F;
}

uint16_t HandlerTableA(uint16_t i);
uint8_t SaveChecksum(void);

/*
 * @routine 03:B548 SelectionCommit
 * WHAT IT DOES: Records the two choices at $18CB and $18C9: their handler
 * addresses (HandlerTableA) at $18DB and $18DD; both packed as a byte (the
 * table at $03:B540 for each low two bits, $18C9's in the high nibble) into
 * the save data at $70:FC0E, with the checksum renewed; and the derived
 * values $18E3 and $18E5 from the table at $03:B46C, where $18D5 is the
 * table $03:B464 entry for $18C9 when the two choices are the same, else 0.
 * HOW: 16-bit lookups; $11DF holds the high nibble.
 * Entry: any widths. Exit: A = $18E5's value; X = its table offset; P
 * unchanged.
 */
uint16_t SelectionCommit(uint8_t bank, uint16_t *x)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t v;

    wr16(0x0018DB, HandlerTableA(rd16(0x0018CB)));
    wr16(0x0018DD, HandlerTableA(rd16(0x0018C9)));
    wr16(b | 0x11DF, (uint16_t)(rd16(0x03B540 + ((rd16(0x0018C9) & 3) << 1)) << 4));
    v = (uint16_t)(rd16(0x03B540 + ((rd16(0x0018CB) & 3) << 1)) | rd16(b | 0x11DF));
    wr8(0x70FC0E, (uint8_t)v);
    SaveChecksum();
    wr16(0x0018D3, 0x0000);
    wr16(0x0018D5, 0x0000);
    if (rd16(0x0018CB) == rd16(0x0018C9))
        wr16(b | 0x18D5, rd16(0x03B464 + (uint16_t)(rd16(0x0018C9) << 1)));
    *x = (uint16_t)((uint16_t)((uint16_t)(rd16(0x0018CB) << 2) + rd16(0x0018D3)) << 1);
    wr16(0x0018E3, rd16(0x03B46C + *x));
    *x = (uint16_t)((uint16_t)((uint16_t)(rd16(0x0018C9) << 2) + rd16(0x0018D5)) << 1);
    v = rd16(0x03B46C + *x);
    wr16(0x0018E5, v);
    return v;
}

void abi_SelectionCommit(void)
{
    uint16_t x;

    cpu.a = SelectionCommit(cpu.db, &x);
    cpu.x = (cpu.p & P_X) ? (uint8_t)x : x;
}

uint16_t HandlerTableB(uint16_t i);
uint16_t SelectionTables18CB(uint8_t bank);
void abi_HandlerTableA(void);
void abi_HandlerTableB(void);
void abi_SelectionTables18CB(void);
void abi_SaveChecksum(void);

/* The save byte for the selection's mode ($18C3): 3, the low byte of the
   $03:B540 entry for $18CB's low two bits at $70:FC0D; 1, that entry for
   $18CB, shifted into the high nibble, kept with bits 0-1 of $70:FC0C;
   otherwise the entry kept with bits 4-5 of $70:FC0C. Returns A as the
   store leaves it. */
static uint16_t save_selection(uint8_t bank, uint16_t *x)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t mode = rd16(0x0018C3), v;

    if (mode == 3) {
        *x = (uint16_t)((rd16(0x0018CB) & 3) << 1);
        v = rd16(0x03B540 + *x);
        wr8(0x70FC0D, (uint8_t)v);
        return v;
    }
    wr16(b | 0x11DF, rd16(0x70FC0C) & (mode == 1 ? 0x0003 : 0x0030));
    *x = (uint16_t)(rd16(0x0018CB) << 1);
    v = rd16(0x03B540 + *x);
    if (mode == 1)
        v = (uint16_t)(v << 4);
    v = (uint16_t)(v | rd16(b | 0x11DF));
    wr8(0x70FC0C, (uint8_t)v);
    return v;
}

/*
 * @routine 03:B48C SelectionSave
 * WHAT IT DOES: Records the choices: the handler addresses for $18CB and
 * $18C9 (HandlerTableA) at $18DB and $18DD, then for $18CD, $18CF, $18D1
 * (HandlerTableB) at $18DD, $18DF, $18E1; derives the values from them
 * (SelectionTables18CB); and packs the choice into the save data by mode
 * (see save_selection), renewing the checksum.
 * HOW: 16-bit work, a byte store into the save data, SaveChecksum.
 * Entry: any widths. Exit: A low byte = the checksum, high byte from the
 * packed value; X = the table offset (low byte when X is 8-bit); P
 * unchanged.
 */
void SelectionSave(uint8_t bank)
{
    uint16_t x;

    wr16(0x0018DB, HandlerTableA(rd16(0x0018CB)));
    wr16(0x0018DD, HandlerTableA(rd16(0x0018C9)));
    wr16(0x0018DD, HandlerTableB(rd16(0x0018CD)));
    wr16(0x0018DF, HandlerTableB(rd16(0x0018CF)));
    wr16(0x0018E1, HandlerTableB(rd16(0x0018D1)));
    SelectionTables18CB(bank);
    save_selection(bank, &x);
    SaveChecksum();
}

void abi_SelectionSave(void)
{
    static const struct { uint32_t from, to; int b; } h[5] = {
        {0x0018CB, 0x0018DB, 0}, {0x0018C9, 0x0018DD, 0}, {0x0018CD, 0x0018DD, 1},
        {0x0018CF, 0x0018DF, 1}, {0x0018D1, 0x0018E1, 1}};
    uint8_t p = cpu.p;
    uint16_t x;
    unsigned i;

    cpu.p &= (uint8_t)~(P_M | P_X);
    for (i = 0; i < 5; i++) {
        cpu.a = rd16(h[i].from);
        if (h[i].b)
            abi_HandlerTableB();
        else
            abi_HandlerTableA();
        wr16(h[i].to, cpu.a);
    }
    abi_SelectionTables18CB();
    cpu.a = save_selection(cpu.db, &x);
    cpu.x = x;
    abi_SaveChecksum();
    cpu.p = p;
    if (p & P_X) {
        cpu.x &= 0x00FF;
        cpu.y &= 0x00FF;
    }
}

void SoundPort0QueuePush(uint8_t command);

/*
 * @routine 0B:C59F MenuChoiceUpDown
 * WHAT IT DOES: Moves the four-way selection $18C3 up or down with the
 * buttons player 1 has pressed since the menu last cleared $118B (bit 11
 * up, bit 10 down), wrapping, with sound $45 for each move; keeps the
 * previous selection at $18C5 and counts $18F5 down by one in 8 bits.
 * HOW: Up is handled before down, so both pressed move twice and play the
 * sound twice.
 * Entry: M 16-bit, X 16-bit. Exit: A = $18F5; Y = $118B; N, Z from A; M
 * 16-bit.
 */
uint16_t MenuChoiceUpDown(uint8_t bank)
{
    uint16_t pad, a;

    wr16(0x0018C5, rd16(0x0018C3));
    pad = rd16((uint32_t)bank << 16 | 0x118B);
    if (pad & 0x0800) {
        SoundPort0QueuePush(0x45);
        wr16(0x0018C3, (uint16_t)(rd16(0x0018C3) - 1) & 0x0003);
    }
    if (pad & 0x0400) {
        SoundPort0QueuePush(0x45);
        wr16(0x0018C3, (uint16_t)(rd16(0x0018C3) + 1) & 0x0003);
    }
    a = (uint16_t)(rd16(0x0018F5) - 1) & 0x00FF;
    wr16(0x0018F5, a);
    return a;
}

void abi_MenuChoiceUpDown(void)
{
    cpu.a = MenuChoiceUpDown(cpu.db);
    cpu.y = rd16((uint32_t)cpu.db << 16 | 0x118B);
    cpu.p = (uint8_t)(nz16(cpu.p, cpu.a) & ~P_M);
}

/*
 * @routine 0B:D4AE MenuSelectStart
 * WHAT IT DOES: First frame of the selection screen's controlling object X:
 * resets the selection state (MenuStateInit), then sets the object's
 * handler to $B814, +$22 to $FF00 and +$20 to 0.
 * HOW: 16-bit registers forced first; the fields are direct page offsets
 * from the record.
 * Entry: D = $0000; X = record. Exit: A = 0; M 16-bit, X 16-bit; Z set, N
 * clear.
 */
void MenuSelectStart(uint16_t rec)
{
    MenuStateInit();
    wr16((uint16_t)(rec + 0x08), 0xB814);
    wr16((uint16_t)(rec + 0x22), 0xFF00);
    wr16((uint16_t)(rec + 0x20), 0x0000);
}

void abi_MenuSelectStart(void)
{
    MenuSelectStart(cpu.x);
    cpu.a = 0x0000;
    cpu.p = (uint8_t)((cpu.p & ~(P_M | P_X | P_N)) | P_Z);
}

void abi_SoundPort0QueuePush(void);

/* REP #$20, LDA $0D1A, BIT #bit, SEP #$20: whether that button is held. */
static int pad_bit(uint32_t b, uint16_t bit)
{
    uint16_t a;

    cpu.p &= (uint8_t)~P_M;
    a = rd16(b | 0x0D1A);
    cpu.a = a;
    cpu.p = nz16(cpu.p, a);
    cpu.p = (uint8_t)((cpu.p & ~P_Z) | ((a & bit) ? 0 : P_Z));
    cpu.p |= P_M;
    return (a & bit) != 0;
}

/* A byte of work RAM moved by d (INC or DEC), then sound $45 on port 0. */
static void step_with_sound(uint32_t b, uint16_t at, int d)
{
    uint8_t v = (uint8_t)(rd8(b | at) + d);

    wr8(b | at, v);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x45);
    cpu.p = nz8(cpu.p, 0x45);
    abi_SoundPort0QueuePush();
}

/*
 * @routine 02:EA53 MenuCursorFourWay
 * WHAT IT DOES: Moves the menu cursor by the held buttons ($0D1A: $0100
 * right, $0200 left): on the extra row ($0D4C set) right goes to position
 * 2 and left to 1, leaving the row; otherwise position $0D48 (0-3) moves
 * right from 0 and 2, left from 1 and 3, and from 1 (left not held) and 2
 * (right not held) the other button goes up to the extra row ($0D4C - 1);
 * each move plays sound $45. Then the cursor X $0D37 is $88 on the extra
 * row, else the position's byte of the table at $EB49 (also in $0E91).
 * HOW: An internal jump table by position; the buttons are tested 16-bit
 * and read again for each test. Several moves are not reached in the
 * traces so far.
 * Entry: any widths. Exit: M 8-bit, X 8-bit; A low byte = the cursor X.
 */
void abi_MenuCursorFourWay(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint8_t v;

    op_sep(P_M | P_X);
    v = rd8(b | 0x0D4C);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    if (v) {
        if (pad_bit(b, 0x0100)) {
            wr8(b | 0x0D48, 0x02);
            wr8(b | 0x0D4C, 0x00);
            cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x45);
            abi_SoundPort0QueuePush();
        } else if (pad_bit(b, 0x0200)) {
            wr8(b | 0x0D48, 0x01);
            wr8(b | 0x0D4C, 0x00);
            cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x45);
            abi_SoundPort0QueuePush();
        }
    } else {
        op_sep(P_M | P_X);
        v = rd8(b | 0x0D48);
        cpu.p = (uint8_t)((cpu.p & ~P_C) | (v >> 7));     /* ASL A */
        cpu.x = (uint8_t)(v << 1);
        switch (cpu.x) {
        case 0:
            if (pad_bit(b, 0x0100))
                step_with_sound(b, 0x0D48, +1);
            break;
        case 2:
            if (pad_bit(b, 0x0200))
                step_with_sound(b, 0x0D48, -1);
            else if (pad_bit(b, 0x0100))
                step_with_sound(b, 0x0D4C, -1);
            break;
        case 4:
            if (pad_bit(b, 0x0100))
                step_with_sound(b, 0x0D48, +1);
            else if (pad_bit(b, 0x0200))
                step_with_sound(b, 0x0D4C, -1);
            break;
        default:
            if (pad_bit(b, 0x0200))
                step_with_sound(b, 0x0D48, -1);
            break;
        }
    }
    v = rd8(b | 0x0D4C);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    if (v) {
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x88);
        cpu.p = nz8(cpu.p, 0x88);
        wr8(b | 0x0D37, 0x88);
        return;
    }
    cpu.x = rd8(b | 0x0D48);
    v = rd8(b | (uint16_t)(0xEB49 + cpu.x));
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, v);
    wr8(b | 0x0E91, v);
    wr8(b | 0x0D37, v);
}

/* The fourth car is there in HUD mode 3 ($0D2B, read as X) or once bit 2
   of $70:FC0B is set: LDX, CPX #3, then LDA.L, BIT #4. */
static int fourth_car(uint32_t b)
{
    uint8_t v;

    cpu.x = rd8(b | 0x0D2B);
    cpu.p = (uint8_t)((nz8(cpu.p, (uint8_t)(cpu.x - 0x03)) & ~P_C) | (cpu.x >= 0x03 ? P_C : 0));
    if (cpu.x == 0x03)
        return 2;
    v = rd8(0x70FC0B);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, v);
    cpu.p = (v & 0x04) ? (uint8_t)(cpu.p & ~P_Z) : (uint8_t)(cpu.p | P_Z);
    return (v & 0x04) != 0;
}

/* Sound $45, then the car at `at` one on (Right) or back (Left): 0 to 2
   in turn, 3 after 2 (Right) or before 0 (Left) only with the fourth car;
   Right from 3 wraps to 0, Left from 0 without it to 2. */
static void car_step(uint32_t b, uint16_t at, int right)
{
    uint8_t v;
    int four;

    op_sep(right ? P_M | P_X : P_M);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x45);
    cpu.p = nz8(cpu.p, 0x45);
    abi_SoundPort0QueuePush();
    v = rd8(b | at);
    if (right) {
        v = (uint8_t)(v + 1);
        cpu.p = (uint8_t)((nz8(cpu.p, (uint8_t)(v - 0x03)) & ~P_C) | (v >= 0x03 ? P_C : 0));
        if (v == 0x03) {
            four = fourth_car(b);
            v = four ? 0x03 : 0x00;
        } else if (v > 0x03)
            v = 0x00;
    } else {
        v = (uint8_t)(v - 1);
        cpu.p = nz8(cpu.p, v);
        if (v & 0x80) {
            four = fourth_car(b);
            v = four ? 0x03 : 0x02;
        }
    }
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, v);
    wr8(b | at, v);
}

/*
 * @routine 07:C0CA CarSelectTwoPlayers
 * WHAT IT DOES: The two players' car choices: Right ($0100) or Left
 * ($0200) pressed on pad 1 ($0D1A) steps player 1's car $0D89, on pad 2
 * ($0D1C) player 2's $0D8A, each with sound $45 (0 to 2 in turn, 3 only in
 * HUD mode 3 or with bit 2 of $70:FC0B); then both are recorded:
 * player 2's as $18C9 and player 1's as $18CB from the table at $07:C1BF
 * (2, 0, 1, 3), each through SelectionCommit.
 * HOW: 16-bit BIT tests of the pads; 8-bit steps (X 8-bit only on the way
 * Right). Left on pad 1 and both buttons on pad 2 are not reached in the
 * traces so far.
 * Entry: X 8-bit. Exit: M, X 8-bit; as SelectionCommit leaves them.
 */
void abi_CarSelectTwoPlayers(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    static const uint16_t pad[2] = {0x0D1A, 0x0D1C}, car[2] = {0x0D89, 0x0D8A};
    int i;

    for (i = 0; i < 2; i++) {
        op_rep(P_M);
        op_lda16(b | pad[i]);
        if (cpu.a & 0x0100) {
            cpu.p &= (uint8_t)~P_Z;
            car_step(b, car[i], 1);
            continue;
        }
        cpu.p |= P_Z;
        op_rep(P_M);
        op_lda16(b | pad[i]);
        cpu.p = (cpu.a & 0x0200) ? (uint8_t)(cpu.p & ~P_Z) : (uint8_t)(cpu.p | P_Z);
        if (cpu.a & 0x0200)
            car_step(b, car[i], 0);
    }
    op_sep(P_M | P_X);
    cpu.x = rd8(b | 0x0D8A);
    op_lda8(0x07C1BF + cpu.x);
    wr8(0x0018C9, (uint8_t)cpu.a);
    abi_SelectionCommit();
    cpu.x = rd8(b | 0x0D89);
    op_lda8(0x07C1BF + cpu.x);
    wr8(0x0018CB, (uint8_t)cpu.a);
    abi_SelectionCommit();
}
