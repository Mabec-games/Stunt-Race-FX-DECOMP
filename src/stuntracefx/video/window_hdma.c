/*
 * window_hdma.c: the window HDMA table in bank $7F ($7F:F85B). Each entry is
 * three bytes: a line count and a 16-bit address of the window values for
 * those lines; a zero count ends the table. $F857 is the normal window
 * setting; $FFD1 and $FFDD are other ones.
 */
#include "machine.h"

#define WINDOW_HDMA 0x7FF85Bu

static void entry(unsigned i, uint8_t lines, uint16_t addr)
{
    wr8(WINDOW_HDMA + 3 * i, lines);
    wr16(WINDOW_HDMA + 3 * i + 1, addr);
}

/*
 * @routine 7E:D8A8 WindowHdmaFFDD
 * WHAT IT DOES: Sets the window table to four bands (64, 64, 64, 62 lines),
 * all using the window values at $FFDD.
 * HOW: Writes the four counts and the end marker, then the four addresses.
 * Entry: any widths. Exit: A = $FFDD; M 16-bit; N set, Z clear.
 */
void WindowHdmaFFDD(void)
{
    wr8(WINDOW_HDMA + 0, 0x40);
    wr8(WINDOW_HDMA + 3, 0x40);
    wr8(WINDOW_HDMA + 6, 0x40);
    wr8(WINDOW_HDMA + 9, 0x3E);
    wr8(WINDOW_HDMA + 12, 0x00);
    wr16(WINDOW_HDMA + 1, 0xFFDD);
    wr16(WINDOW_HDMA + 4, 0xFFDD);
    wr16(WINDOW_HDMA + 7, 0xFFDD);
    wr16(WINDOW_HDMA + 10, 0xFFDD);
}

void abi_WindowHdmaFFDD(void)
{
    WindowHdmaFFDD();
    cpu.a = 0xFFDD;
    cpu.p = (uint8_t)((cpu.p & ~(P_Z | P_M)) | P_N);
}

/*
 * @routine 7E:DB10 WindowHdmaFromTable
 * WHAT IT DOES: Sets the window table to three bands from the layout $19B6
 * selects: normal, the selected window values, normal.
 * HOW: The first two counts and the middle address come from tables at
 * $7E:E0B4 and $7E:E0D2; the third band is $30 lines.
 * Entry: any widths. Exit: A low byte = 0 (high byte that of the middle
 * address); X = $19B6; M 8-bit, X 16-bit; Z set, N clear.
 */
static uint16_t window_hdma_from_table(void)
{
    uint16_t x = rd16(0x0019B6), mid = rd16(0x7EE0D2 + x);

    wr16(WINDOW_HDMA + 1, 0xF857);
    wr16(WINDOW_HDMA + 7, 0xF857);
    wr16(WINDOW_HDMA + 4, mid);
    wr8(WINDOW_HDMA + 0, rd8(0x7EE0B4 + x));
    wr8(WINDOW_HDMA + 3, rd8(0x7EE0B5 + x));
    wr8(WINDOW_HDMA + 6, 0x30);
    wr8(WINDOW_HDMA + 9, 0x00);
    return mid;
}

void WindowHdmaFromTable(void)
{
    window_hdma_from_table();
}

void abi_WindowHdmaFromTable(void)
{
    uint16_t mid = window_hdma_from_table();

    cpu.a = (uint16_t)(mid & 0xFF00);
    cpu.x = rd16(0x0019B6);
    cpu.p = (uint8_t)(((cpu.p & ~(P_N | P_X)) | P_Z | P_M));
}

/*
 * @routine 7E:DC8D WindowHdmaFFD1
 * WHAT IT DOES: Sets the window table to four bands (32, 112, 16, 8 lines):
 * normal, $FFD1, $FFD1, normal.
 * HOW: Writes the counts and end marker, then the addresses.
 * Entry: any widths. Exit: A = $FFD1; M 16-bit; N set, Z clear.
 */
void WindowHdmaFFD1(void)
{
    wr8(WINDOW_HDMA + 0, 0x20);
    wr8(WINDOW_HDMA + 3, 0x70);
    wr8(WINDOW_HDMA + 6, 0x10);
    wr8(WINDOW_HDMA + 9, 0x08);
    wr8(WINDOW_HDMA + 12, 0x00);
    wr16(WINDOW_HDMA + 1, 0xF857);
    wr16(WINDOW_HDMA + 10, 0xF857);
    wr16(WINDOW_HDMA + 4, 0xFFD1);
    wr16(WINDOW_HDMA + 7, 0xFFD1);
}

void abi_WindowHdmaFFD1(void)
{
    WindowHdmaFFD1();
    cpu.a = 0xFFD1;
    cpu.p = (uint8_t)((cpu.p & ~(P_Z | P_M)) | P_N);
}

/* Window HDMA in use, and a layout of $FF meaning none: both skip the builders below. */
static int window_hdma_off(void)
{
    return rd8(0x0019D8) != 0;
}

/*
 * @routine 7E:DBCD WindowHdmaNormal
 * WHAT IT DOES: Sets the window table to the normal window over the whole
 * screen (four bands of 64 lines), unless window HDMA is off or no layout is
 * set ($19BC = $FF).
 * HOW: Four entries at $F857 and the end marker.
 * Entry: any widths. Exit: A = $F857 when written; X and Y high bytes clear;
 * P unchanged.
 */
static int window_hdma_normal(void)
{
    unsigned i;

    if (window_hdma_off() || rd8(0x0019BC) == 0xFF)
        return 0;
    for (i = 0; i < 4; i++)
        wr8(WINDOW_HDMA + 3 * i, 0x40);
    wr8(WINDOW_HDMA + 12, 0x00);
    for (i = 0; i < 4; i++)
        wr16(WINDOW_HDMA + 3 * i + 1, 0xF857);
    return 1;
}

void WindowHdmaNormal(void)
{
    window_hdma_normal();
}

void abi_WindowHdmaNormal(void)
{
    uint8_t off = rd8(0x0019D8), layout = rd8(0x0019BC);

    if (window_hdma_normal())
        cpu.a = 0xF857;
    else
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | (off ? off : layout));
    cpu.x &= 0x00FF;
    cpu.y &= 0x00FF;
}

/*
 * @routine 7E:DC18 WindowHdmaBands
 * WHAT IT DOES: Sets the window table to nine normal bands (23, 16, 48, 16,
 * 24, 16, 48, 16, 1 lines), unless window HDMA is off.
 * HOW: Nine entries at $F857 and the end marker.
 * Entry: any widths. Exit: A = $F857 when written, else $19D8; X and Y high
 * bytes clear; P unchanged.
 */
static int window_hdma_bands(void)
{
    static const uint8_t lines[9] = {0x17, 0x10, 0x30, 0x10, 0x18, 0x10, 0x30, 0x10, 0x01};
    unsigned i;

    if (window_hdma_off())
        return 0;
    for (i = 0; i < 9; i++)
        wr8(WINDOW_HDMA + 3 * i, lines[i]);
    wr8(WINDOW_HDMA + 27, 0x00);
    for (i = 0; i < 9; i++)
        wr16(WINDOW_HDMA + 3 * i + 1, 0xF857);
    return 1;
}

void WindowHdmaBands(void)
{
    window_hdma_bands();
}

void abi_WindowHdmaBands(void)
{
    uint8_t off = rd8(0x0019D8);

    if (window_hdma_bands())
        cpu.a = 0xF857;
    else
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | off);
    cpu.x &= 0x00FF;
    cpu.y &= 0x00FF;
}

/*
 * @routine 7E:D85C WindowHdmaAt
 * WHAT IT DOES: Sets the window table to four bands around line A: normal
 * windows in the middle two bands, $FFDD above and below.
 * HOW: Band heights come from A and the offsets at $19E2-$19E5 and the height
 * at $19DC.
 * Entry: M 8-bit; A low byte = line. Exit: A = $FFDD; M 16-bit; N set, Z
 * clear; C and V from the last height sum.
 */
static void window_hdma_at(uint8_t line, uint8_t *p)
{
    uint8_t h = rd8(0x0019DC), e2 = rd8(0x0019E2);
    unsigned sum;

    wr8(WINDOW_HDMA + 3, (uint8_t)(line + rd8(0x0019E5)));
    wr8(WINDOW_HDMA + 6, (uint8_t)(line + rd8(0x0019E3)));
    wr8(WINDOW_HDMA + 0, (uint8_t)(h - 1 + rd8(0x0019E4)));
    sum = (unsigned)h + e2;
    wr8(WINDOW_HDMA + 9, (uint8_t)sum);
    *p = (uint8_t)((*p & ~(P_C | P_V)) | (sum > 0xFF ? P_C : 0)
                   | ((~(h ^ e2) & (h ^ (uint8_t)sum) & 0x80) ? P_V : 0));
    wr8(WINDOW_HDMA + 12, 0x00);
    wr16(WINDOW_HDMA + 4, 0xF857);
    wr16(WINDOW_HDMA + 7, 0xF857);
    wr16(WINDOW_HDMA + 1, 0xFFDD);
    wr16(WINDOW_HDMA + 10, 0xFFDD);
}

void WindowHdmaAt(uint8_t line)
{
    uint8_t p = 0;

    window_hdma_at(line, &p);
}

void abi_WindowHdmaAt(void)
{
    uint8_t p = cpu.p;

    window_hdma_at((uint8_t)cpu.a, &p);
    cpu.a = 0xFFDD;
    cpu.p = (uint8_t)((p & ~(P_Z | P_M)) | P_N);
}

/*
 * @routine 7E:D6E8 WindowHdmaFromList
 * WHAT IT DOES: Sets the window table to one normal band of 32 lines followed
 * by the entries of the list X selects, up to its end marker.
 * HOW: The list's address comes from the table at $7E:D8DE; its bytes are
 * copied (in the data bank) from the fourth table byte on, end marker
 * included.
 * Entry: M 8-bit, X 8-bit; DB the bank the lists are in. Exit: A low byte =
 * 0; X = 3 + bytes copied; Y past the end marker's address... see wrapper;
 * M 8-bit, X 16-bit; Z set.
 */
typedef struct {
    uint16_t a, x, y;
} ListEnd;

static ListEnd window_hdma_from_list(uint8_t db, uint8_t select)
{
    ListEnd end;
    uint16_t y = rd16(0x7ED8DE + select), x = 3;
    uint8_t v;

    end.a = y;
    wr8(WINDOW_HDMA, 0x20);
    wr16(WINDOW_HDMA + 1, 0xF857);
    for (;;) {
        v = rd8((uint32_t)db << 16 | y);
        wr8(WINDOW_HDMA + x, v);
        if (v == 0)
            break;
        y++;
        x++;
    }
    end.x = x;
    end.y = y;
    return end;
}

void WindowHdmaFromList(uint8_t select)
{
    window_hdma_from_list(0x7E, select);
}

void abi_WindowHdmaFromList(void)
{
    ListEnd end = window_hdma_from_list(cpu.db, (uint8_t)cpu.x);

    cpu.a = 0xF800;
    cpu.x = end.x;
    cpu.y = end.y;
    cpu.p = (uint8_t)(((cpu.p & ~(P_N | P_X)) | P_Z | P_M));
}

/*
 * @routine 7E:D718 WindowHdmaPreset
 * WHAT IT DOES: Copies a 9-byte preset into the window HDMA table: from the
 * address at $7E:DA1D + X to entry offset $0F when $0D2E is set, otherwise
 * from the address at $7E:D9C5 + X to offset 3.
 * HOW: The preset is read through the data bank; $19E1 counts the bytes
 * (cleared with a long store, stepped through the data bank).
 * Entry: X 8-bit; X = preset index (bytes). Exit: A = 9, with the high byte
 * of the preset's address; X, Y past the copy; M 8-bit, X 16-bit; Z, C set,
 * N clear.
 */
uint16_t WindowHdmaPreset(uint8_t bank, uint8_t index, uint16_t *x, uint16_t *y)
{
    uint32_t b = (uint32_t)bank << 16;
    uint8_t n;

    wr8(0x0019E1, 0x00);
    if (rd8(0x000D2E)) {
        *y = rd16(0x7EDA1D + index);
        *x = 0x000F;
    } else {
        *y = rd16(0x7ED9C5 + index);
        *x = 0x0003;
    }
    do {
        wr8(0x7FF85B + *x, rd8(b | *y));
        (*y)++;
        (*x)++;
        n = (uint8_t)(rd8(0x0019E1) + 1);
        wr8(b | 0x19E1, n);
    } while (n != 9);
    return (uint16_t)(((uint16_t)(*y - 9) & 0xFF00) | 0x09);
}

void abi_WindowHdmaPreset(void)
{
    cpu.a = WindowHdmaPreset(cpu.db, (uint8_t)cpu.x, &cpu.x, &cpu.y);
    cpu.p = (uint8_t)(((cpu.p & ~(P_N | P_X)) | P_Z | P_C | P_M));
}

/* (0x80 - v) + 0x80 as SEC / SBC then CLC / ADC #$80 do it, with C and V
   from the add. */
static uint8_t mirror80(uint8_t v, uint8_t *p)
{
    uint8_t d = (uint8_t)(0x80 - v);
    unsigned s = (unsigned)d + 0x80;

    *p = (uint8_t)((*p & ~(P_C | P_V)) | (s > 0xFF ? P_C : 0) | ((~(d ^ 0x80) & (d ^ (uint8_t)s) & 0x80) ? P_V : 0));
    return (uint8_t)s;
}

/*
 * @routine 09:F065 WindowEdgesFromPair
 * WHAT IT DOES: Writes a pair of window edges and their mirror images about
 * $80 into the window HDMA table at offset $28 + the byte at $19B4: the
 * first byte at X, its mirror, the second byte at X + 1, its mirror ($80
 * when the byte is $81).
 * HOW: Byte reads through the data bank; long stores at $7F:F883-$7F:F886
 * + the offset.
 * Entry: M 8-bit, X 16-bit; X = the pair's address. Exit: A low byte = the
 * last value; X unchanged; N, Z from X; C, V from the last mirror (C set
 * when the second byte was $81).
 */
uint8_t WindowEdgesFromPair(uint8_t bank, uint16_t pair, uint8_t *p)
{
    uint32_t b = (uint32_t)bank << 16;
    uint8_t v0 = rd8(b | pair), v1, v;

    wr8(0x7FF883 + rd16(b | 0x19B4), v0);
    v = mirror80(rd8(b | pair), p);
    wr8(0x7FF884 + rd16(b | 0x19B4), v);
    v1 = rd8(b | (uint16_t)(pair + 1));
    wr8(0x7FF885 + rd16(b | 0x19B4), v1);
    if (rd8(b | (uint16_t)(pair + 1)) == 0x81) {
        *p |= P_C;
        v = 0x80;
    } else {
        v = mirror80(rd8(b | (uint16_t)(pair + 1)), p);
    }
    wr8(0x7FF886 + rd16(b | 0x19B4), v);
    return v;
}

void abi_WindowEdgesFromPair(void)
{
    uint8_t p = cpu.p, v = WindowEdgesFromPair(cpu.db, cpu.x, &p);

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz16(p, cpu.x);
}

/*
 * @routine 09:F0B2 WindowEdgesFromList
 * WHAT IT DOES: Writes a symmetrical run of window edges for the window
 * selected by $19B6: the pairs of the list it points to (table $09:F98B,
 * read through the data bank, ended by $FF) in order and then in reverse,
 * each with WindowEdgesFromPair at a table offset that starts at the
 * table $09:F155 value and steps by 4; then moves $19B6 on by 2.
 * HOW: The offset is kept at $19B4; Y counts the pairs.
 * Entry: any widths. Exit: A = the new $19B6; Y = 0; X at the first pair
 * minus 2; M 16-bit, X 16-bit; N, Z, C, V from the add.
 */
uint16_t WindowEdgesFromList(uint8_t bank, uint16_t *x, uint8_t *p)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t sel = rd16(b | 0x19B6), n = 0, s;

    wr16(b | 0x19B4, rd16(0x09F155 + sel));
    *x = rd16(0x09F98B + rd16(b | 0x19B6));
    for (;;) {
        WindowEdgesFromPair(bank, *x, p);
        *x = (uint16_t)(*x + 2);
        n++;
        if (rd8(b | *x) == 0xFF)
            break;
        wr16(b | 0x19B4, (uint16_t)(rd16(b | 0x19B4) + 4));
    }
    *x = (uint16_t)(*x - 2);
    for (;;) {
        WindowEdgesFromPair(bank, *x, p);
        *x = (uint16_t)(*x - 2);
        if (!--n)
            break;
        wr16(b | 0x19B4, (uint16_t)(rd16(b | 0x19B4) + 4));
    }
    sel = rd16(b | 0x19B6);
    s = (uint16_t)(sel + 2);
    *p = (uint8_t)((nz16(*p, s) & ~(P_C | P_V)) | ((unsigned)sel + 2 > 0xFFFF ? P_C : 0) | ((~sel & s & 0x8000) ? P_V : 0));
    wr16(b | 0x19B6, s);
    return s;
}

void abi_WindowEdgesFromList(void)
{
    uint8_t p = cpu.p;

    cpu.a = WindowEdgesFromList(cpu.db, &cpu.x, &p);
    cpu.y = 0;
    cpu.p = (uint8_t)(p & ~(P_M | P_X));
}

/*
 * @routine 7E:DB47 WindowHdmaLayout
 * WHAT IT DOES: Writes the window HDMA table's line counts and data for
 * layout $19B6: one screen (two entries) when $0D2E is clear; the split
 * screen (three entries) when it is set, which no script has reached yet.
 * HOW: Long stores into the table at $7F:F85B. One screen: entry 0 lines
 * from $7E:E0C2 + X, entry 1 lines from $7E:E0C3 + X, data pointers $F857
 * and the word at $7E:E0D2 + X, then $10 lines and the end. Split: entries
 * 0 and 1 lines from $7E:E0CA + X, entry 2 from $7E:E0CB + X, pointers
 * $F857, $F857, the word at $7E:E0E0 + X, $F857, then $10 lines and the
 * end.
 * Entry: any widths. Exit: A low byte 0, high byte from the layout's data
 * word; X = $19B6; M 8-bit, X 16-bit; N clear, Z set.
 */
uint16_t WindowHdmaLayout(uint8_t bank, uint16_t *x)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t w;

    if (!rd8(0x000D2E)) {
        *x = rd16(b | 0x19B6);
        wr16(0x7FF85C, 0xF857);
        wr16(0x7FF862, 0xF857);
        w = rd16(0x7EE0D2 + *x);
        wr16(0x7FF85F, w);
        wr8(0x7FF85B, rd8(0x7EE0C2 + *x));
        wr8(0x7FF85E, rd8(0x7EE0C3 + *x));
        wr8(0x7FF861, 0x10);
        wr8(0x7FF864, 0x00);
    } else {
        *x = rd16(b | 0x19B6);
        wr16(0x7FF85C, 0xF857);
        wr16(0x7FF85F, 0xF857);
        wr16(0x7FF865, 0xF857);
        w = rd16(0x7EE0E0 + *x);
        wr16(0x7FF862, w);
        wr8(0x7FF85B, rd8(0x7EE0CA + *x));
        wr8(0x7FF85E, rd8(0x7EE0CA + *x));
        wr8(0x7FF861, rd8(0x7EE0CB + *x));
        wr8(0x7FF864, 0x10);
        wr8(0x7FF867, 0x00);
    }
    return (uint16_t)(w & 0xFF00);
}

void abi_WindowHdmaLayout(void)
{
    cpu.a = WindowHdmaLayout(cpu.db, &cpu.x);
    cpu.p = (uint8_t)(((cpu.p & ~(P_N | P_X)) | P_Z | P_M));
}

void abi_WindowHdmaLayout(void);

/* The two window table builders WindowLayoutStep picks between by Y. */
static void window_layout_apply(uint16_t y, uint8_t bank)
{
    if (y == 0)
        window_hdma_from_table();
    else
        WindowHdmaLayout(bank, &y);
}

void WindowLayoutStep(uint8_t bank, uint16_t y);
void abi_WindowLayoutStep(void);

/*
 * @routine 7E:D28F WindowLayoutStepForScreen
 * WHAT IT DOES: Clears bit 5 of $0D45 and steps the window layout
 * (WindowLayoutStep) with Y = 0, or Y = 2 on the menu's course screen
 * ($0D62 set).
 * HOW: Data bank $7E inside, P and the data bank saved and restored.
 * Entry: any widths. Exit: A, X, Y as WindowLayoutStep leaves them; P and
 * the data bank unchanged.
 */
void WindowLayoutStepForScreen(void)
{
    wr8(0x000D45, (uint8_t)(rd8(0x000D45) & 0xDF));
    WindowLayoutStep(0x7E, rd8(0x7E0D62) ? 0x0002 : 0x0000);
}

void abi_WindowLayoutStepForScreen(void)
{
    uint8_t p = cpu.p, db = cpu.db, v;

    v = (uint8_t)(rd8(0x000D45) & 0xDF);
    wr8(0x000D45, v);
    cpu.db = 0x7E;
    v = rd8(0x7E0D62);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.y = v ? 0x0002 : 0x0000;
    cpu.p &= (uint8_t)~(P_M | P_X);
    abi_WindowLayoutStep();
    cpu.db = db;
    cpu.p = p;
}

/* One step of the window slide: the slide position at $1A14 moves by
   `step`; reaching `stop` ends the slide (bits 1 and 2 of $0DA5 cleared),
   otherwise the window table becomes WindowHdmaFFD1's and the window setup
   $19AC-$19AE = $20, $40, the position. */
static int window_slide(uint8_t bank, uint8_t step, uint8_t stop, uint8_t *a, uint8_t *pos)
{
    uint32_t b = (uint32_t)bank << 16;
    uint8_t v = (uint8_t)(rd8(0x001A14) + step);

    *pos = v;

    if (v == stop) {
        *a = (uint8_t)(rd8(b | 0x0DA5) & 0xF9);
        wr8(b | 0x0DA5, *a);
        return 0;
    }
    wr8(0x001A14, v);
    WindowHdmaFFD1();
    wr8(0x0019AD, 0x40);
    wr8(0x0019AC, 0x20);
    *a = rd8(0x001A14);
    wr8(0x0019AE, *a);
    return 1;
}

static void abi_window_slide(uint8_t step, uint8_t stop)
{
    uint8_t a, v;
    int moved = window_slide(cpu.db, step, stop, &a, &v);

    if (moved)
        cpu.a = (uint16_t)(0xFF00 | a);   /* WindowHdmaFFD1 left $FFD1 */
    else
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | a);
    cpu.p = (uint8_t)((nz8(cpu.p, a) & ~P_C) | P_M | (v >= stop ? P_C : 0));
}

/*
 * @routine 7E:DDF0 WindowSlideDown
 * WHAT IT DOES: Moves the window slide position $1A14 down a line; at $96
 * the slide ends.
 * HOW: window_slide with +1, stopping at $96. Long accesses except $0DA5
 * (data bank).
 * Entry: X 16-bit or 8-bit (unchanged). Exit: M 8-bit; A low byte = $0DA5
 * when the slide ends, else the new position (A high byte $FF); N, Z from
 * it; C from comparing the new position with $96.
 */
void WindowSlideDown(uint8_t bank)
{
    uint8_t a, v;

    window_slide(bank, 0x01, 0x96, &a, &v);
}

void abi_WindowSlideDown(void)
{
    abi_window_slide(0x01, 0x96);
}

/*
 * @routine 7E:DE27 WindowSlideUp
 * WHAT IT DOES: Moves the window slide position $1A14 up a line; at $7F
 * the slide ends.
 * HOW: As WindowSlideDown, with -1, stopping at $7F.
 * Entry: as WindowSlideDown. Exit: as WindowSlideDown, comparing with $7F.
 */
void WindowSlideUp(uint8_t bank)
{
    uint8_t a, v;

    window_slide(bank, 0xFF, 0x7F, &a, &v);
}

void abi_WindowSlideUp(void)
{
    abi_window_slide(0xFF, 0x7F);
}

/*
 * @routine 7E:DA75 WindowLayoutStep
 * WHAT IT DOES: Acts on a request in $19BD to move the window layout: 1
 * starts it moving up by 2 a step ($19B8 = 2, $19BA = 1), $FF down ($19B8 =
 * -2, $19BA = 0); $19BC = $FF marks it moving. While it moves, each call
 * clears the request and steps the layout $19B6 by $19B8, stopping (and
 * clearing $19BC) when that would give -2, and not storing it when it
 * reaches the limit for Y in the table at $7E:DBC5; then resets the window
 * setup ($19A9 = 0, $19AC-$19B1 = $E7) and rebuilds the window table with
 * WindowHdmaFromTable (Y = 0) or WindowHdmaLayout (Y = 2).
 * HOW: Long accesses to $19B6-$19BD except one data-bank read of $19BC and
 * the store to $19A9; the builder is called through the table at
 * $7E:DBC9.
 * Entry: Y = 0 or 2. Exit: when nothing moves, A low byte = $19BC, M 8-bit,
 * X 8-bit, N, Z, C from comparing it with $FF; at the -2 stop A low byte =
 * 0, Z, C set, N clear, M and X 8-bit; otherwise as the builder, with C
 * from the limit compare and V from the step add.
 */
void WindowLayoutStep(uint8_t bank, uint16_t y)
{
    uint32_t b = (uint32_t)bank << 16;
    uint8_t req = rd8(0x0019BD);
    uint16_t pos;
    unsigned i;

    y &= 0x00FF;
    if (req == 0x01 || req == 0xFF) {
        wr8(0x0019BC, 0xFF);
        wr16(0x0019B8, req == 0x01 ? 0x0002 : 0xFFFE);
        wr16(0x0019BA, req == 0x01 ? 0x0001 : 0x0000);
    } else if (rd8(b | 0x19BC) != 0xFF) {
        return;
    }
    wr8(0x0019BD, 0x00);
    pos = (uint16_t)(rd16(0x0019B6) + rd16(0x0019B8));
    if (pos == 0xFFFE) {
        wr8(0x0019BC, 0x00);
        return;
    }
    if (pos != rd16(0x7EDBC5 + y))
        wr16(0x0019B6, pos);
    wr8(b | 0x19A9, 0x00);
    for (i = 0; i < 6; i++)
        wr8(0x0019AC + i, 0xE7);
    window_layout_apply(y, bank);
}

void abi_WindowLayoutStep(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint8_t req = rd8(0x0019BD), v;
    uint16_t y = (uint8_t)cpu.y, s, m, pos, lim;
    unsigned i;

    cpu.x &= 0x00FF;
    cpu.y = y;
    cpu.p |= P_M | P_X;
    if (req == 0x01 || req == 0xFF) {
        wr8(0x0019BC, 0xFF);
        wr16(0x0019B8, req == 0x01 ? 0x0002 : 0xFFFE);
        wr16(0x0019BA, req == 0x01 ? 0x0001 : 0x0000);
    } else {
        v = rd8(b | 0x19BC);
        if (v != 0xFF) {
            cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
            cpu.p = (uint8_t)(nz8(cpu.p, (uint8_t)(v + 1)) & ~P_C);
            return;
        }
    }
    wr8(0x0019BD, 0x00);
    s = rd16(0x0019B6);
    m = rd16(0x0019B8);
    pos = (uint16_t)(s + m);
    cpu.p = (uint8_t)(cpu.p & ~P_V);
    if (~(s ^ m) & (s ^ pos) & 0x8000)
        cpu.p |= P_V;
    if (pos == 0xFFFE) {
        wr8(0x0019BC, 0x00);
        cpu.a = 0x0000 | (pos & 0xFF00);
        cpu.p = (uint8_t)((cpu.p & ~P_N) | P_Z | P_C);
        return;
    }
    lim = rd16(0x7EDBC5 + y);
    cpu.p = (uint8_t)((cpu.p & ~P_C) | (pos >= lim ? P_C : 0));
    if (pos != lim)
        wr16(0x0019B6, pos);
    wr8(b | 0x19A9, 0x00);
    for (i = 0; i < 6; i++)
        wr8(0x0019AC + i, 0xE7);
    cpu.a = (uint16_t)((pos & 0xFF00) | 0xE7);
    cpu.x = y;
    cpu.p = (uint8_t)(nz16((uint8_t)(cpu.p & ~P_X), y) | P_M);
    if (y == 0)
        abi_WindowHdmaFromTable();
    else
        abi_WindowHdmaLayout();
}
