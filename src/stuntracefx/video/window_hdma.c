/*
 * window_hdma.c: the window HDMA table in bank $7F ($7F:F85B). Each entry is
 * three bytes: a line count and a 16-bit address of the window values for
 * those lines; a zero count ends the table. $F857 is the normal window
 * setting; $FFD1 and $FFDD are other ones.
 */
#include "machine.h"
#include "hw.h"
#include "cpu_ops.h"

#define WINDOW_HDMA 0x7FF85Bu

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
static uint16_t window_hdma_from_table(uint16_t *sel)
{
    uint16_t x = rd16(0x0019B6), mid;
    uint8_t n;

    *sel = x;
    wr16(WINDOW_HDMA + 1, 0xF857);
    wr16(WINDOW_HDMA + 7, 0xF857);
    mid = rd16(0x7EE0D2 + x);
    wr16(WINDOW_HDMA + 4, mid);
    n = rd8(0x7EE0B4 + x);
    wr8(WINDOW_HDMA + 0, n);
    n = rd8(0x7EE0B5 + x);
    wr8(WINDOW_HDMA + 3, n);
    wr8(WINDOW_HDMA + 6, 0x30);
    wr8(WINDOW_HDMA + 9, 0x00);
    return mid;
}

void WindowHdmaFromTable(void)
{
    uint16_t sel;

    window_hdma_from_table(&sel);
}

void abi_WindowHdmaFromTable(void)
{
    uint16_t sel, mid = window_hdma_from_table(&sel);

    cpu.a = (uint16_t)(mid & 0xFF00);
    cpu.x = sel;
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

/* Window HDMA in use, and a layout of $FF meaning none: both skip the
   builders below. *last gets the byte read, A when a builder skips. */
static int window_hdma_off(uint8_t *last)
{
    *last = rd8(0x0019D8);
    return *last != 0;
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
static int window_hdma_normal(uint8_t *last)
{
    unsigned i;

    if (window_hdma_off(last))
        return 0;
    *last = rd8(0x0019BC);
    if (*last == 0xFF)
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
    uint8_t last;

    window_hdma_normal(&last);
}

void abi_WindowHdmaNormal(void)
{
    uint8_t last;

    if (window_hdma_normal(&last))
        cpu.a = 0xF857;
    else
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | last);
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
static int window_hdma_bands(uint8_t *last)
{
    static const uint8_t lines[9] = {0x17, 0x10, 0x30, 0x10, 0x18, 0x10, 0x30, 0x10, 0x01};
    unsigned i;

    if (window_hdma_off(last))
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
    uint8_t last;

    window_hdma_bands(&last);
}

void abi_WindowHdmaBands(void)
{
    uint8_t last;

    if (window_hdma_bands(&last))
        cpu.a = 0xF857;
    else
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | last);
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
 * @routine 09:F04D WindowEdgesFillOnes
 * WHAT IT DOES: Fills $0800 words from the window edge table $7F:F883 with
 * 1; nothing calls it (no JSR, pointer or table reaches it), so a direct
 * call is its only way in. The run is $1000 bytes long and the long indexed
 * store carries into bank $80 past $7F:FFFF, whose $0000-$1FFF is low work
 * RAM: $7E:0000-$7E:0882 are set to $0001 words too, the stack with them,
 * so the RTS returns through what the fill left there.
 * HOW: REP #$30; X steps by 2 from 0, Y counts $0800 down.
 * Entry: any widths. Exit: A = 1, X = $1000, Y = 0; M and X 16-bit; Z and
 * C set, N clear (CPY #0); the RTS pulls its return address from the
 * filled stack.
 */
void abi_WindowEdgesFillOnes(void)
{
    uint16_t x = 0, y = 0x0800;

    cpu.p &= (uint8_t)~(P_M | P_X);
    do {
        wr16((0x7FF883u + x) & 0xFFFFFF, 0x0001);
        x = (uint16_t)(x + 2);
        y--;
    } while (y);
    cpu.a = 0x0001;
    cpu.x = x;
    cpu.y = 0;
    cpu.p = (uint8_t)((cpu.p & ~(P_N | P_Z | P_C)) | P_Z | P_C);
    /* The RTS, through the return address the fill left on the stack. */
    cpu.pc = (cpu.pc & 0xFF0000) | (uint16_t)(rd16((uint16_t)(cpu.s + 1)) + 1);
    cpu.s = (uint16_t)(cpu.s + 2);
    cpu.unwound = 1;
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
 * screen (three entries) when it is set, reached only by unit_fade_windows.
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
    uint8_t n;

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
        n = rd8(0x7EE0CA + *x);
        wr8(0x7FF85B, n);
        wr8(0x7FF85E, n);
        n = rd8(0x7EE0CB + *x);
        wr8(0x7FF861, n);
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
        WindowHdmaFromTable();
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

    uint8_t old = (uint8_t)(v - step);
    /* V from the step: ADC #1 (clear carry) going up, SBC #1 (set carry)
       going down. */
    int ov = step == 0x01 ? (~(old ^ 0x01) & (old ^ v) & 0x80) != 0 : ((old ^ 0x01) & (old ^ v) & 0x80) != 0;

    if (moved)
        cpu.a = (uint16_t)(0xFF00 | a);   /* WindowHdmaFFD1 left $FFD1 */
    else
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | a);
    cpu.p = (uint8_t)((nz8(cpu.p, a) & ~(P_C | P_V)) | P_M | (v >= stop ? P_C : 0) | (ov ? P_V : 0));
}

/*
 * @routine 7E:DDD9 WindowSlideStep
 * WHAT IT DOES: Steps the window slide the way bits 1-2 of $0DA5 ask: 2
 * slides it down (WindowSlideDown), 4 or 6 up (WindowSlideUp), 0 not at
 * all.
 * HOW: The bits are tested through the data bank.
 * Entry: X 16-bit or 8-bit (unchanged). Exit: M 8-bit; as the slide it
 * runs, or A low byte = 0 with Z set.
 */
void abi_WindowSlideDown(void);
void abi_WindowSlideUp(void);

void abi_WindowSlideStep(void)
{
    uint8_t v;

    cpu.p |= P_M;
    v = (uint8_t)(rd8(((uint32_t)cpu.db << 16) | 0x0DA5) & 0x06);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, v);
    if (!v)
        return;
    cpu.p = (uint8_t)((nz8(cpu.p, (uint8_t)(v - 0x02)) & ~P_C) | (v >= 0x02 ? P_C : 0));
    if (v == 0x02)
        abi_WindowSlideDown();
    else
        abi_WindowSlideUp();
}

/*
 * @routine 7E:DDF0 WindowSlideDown
 * WHAT IT DOES: Moves the window slide position $1A14 down a line; at $96
 * the slide ends.
 * HOW: window_slide with +1, stopping at $96. Long accesses except $0DA5
 * (data bank).
 * Entry: X 16-bit or 8-bit (unchanged). Exit: M 8-bit; A low byte = $0DA5
 * when the slide ends, else the new position (A high byte $FF); N, Z from
 * it; C from comparing the new position with $96; V from the add.
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

void abi_Records7FFromF170(void);
void abi_Records7FFromF634(void);

/* Bytes `n` from ROM $09:src to `dst` (bank $7F), long stores. */
static void copy_rom09(uint16_t src, uint32_t dst, unsigned n)
{
    unsigned i;

    for (i = 0; i < n; i++)
        wr8(dst + i, rd8(0x090000 | (uint16_t)(src + i)));
}

/*
 * @routine 09:ECE0 WindowsInit
 * WHAT IT DOES: Sets up the race's windows: the four-byte HDMA band entries
 * at $7F:F857 and $7F:FFD1-$FFE0 and the 16-byte window table header at
 * $7F:F85B from the lists in bank $09; the course records into bank $7F
 * (Records7FFromF170, or Records7FFromF634 on the menu's course screen);
 * the window edges for every list (WindowEdgesFromList until $19B6 reaches
 * $0E); the word list at $09:FB74 (to its $FFFF) to $7F:FF8B; on the
 * course screen, every edge byte at $7F:F883 moved by -$28 and a copy +$50
 * at $7F:FB03; then the window, slide and layout state ($19A9-$1A29) to
 * its start values, the slide value $19F6 and $19E6 from the tables at
 * $09:F127 and $09:F115 for HUD modes 0 and 3 (and 0), the layout record
 * for $0D89 from $09:F14D, and HDMA channels 4 (to $2126, indirect, from
 * $7F:F85B) and 5 (to $2132 from $7F:FF8B).
 * HOW: Data bank $09 (the program bank) and P saved and restored; the
 * state stores are in the original's order, byte and word as it makes
 * them. The ROM bytes $09:xxxx indexed by the tables are read through the
 * data bank.
 * Entry: any widths, D = $0000. Exit: A = $007F; X = 0; Y as the last
 * callee left it; P and the data bank unchanged.
 */
void WindowsInit(void)
{
    const uint32_t b = 0x090000;
    uint16_t x, w, m;
    unsigned i;
    uint8_t v;

    copy_rom09(0xF999, 0x7FF857, 4);
    copy_rom09(0xF99D, 0x7FFFD1, 4);
    copy_rom09(0xF9A1, 0x7FFFD5, 4);
    copy_rom09(0xF9A5, 0x7FFFD9, 4);
    copy_rom09(0xF9A9, 0x7FFFDD, 4);
    cpu.p |= P_M | P_X;
    cpu.x = 0x0004;
    cpu.y &= 0x00FF;
    v = rd8(b | 0x0D62);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    if (v)
        abi_Records7FFromF634();
    else
        abi_Records7FFromF170();
    cpu.p |= P_M | P_X;
    cpu.x &= 0x00FF;
    cpu.y &= 0x00FF;
    copy_rom09(0xF163, 0x7FF85B, 16);
    wr16(b | 0x19B6, 0x0000);
    do {
        abi_WindowEdgesFromList();
    } while (rd16(b | 0x19B6) != 0x000E);
    for (x = 0; (w = rd16(0x09FB74 + x)) != 0xFFFF; x = (uint16_t)(x + 2))
        wr16(0x7FFF8B + x, w);
    if (rd8(b | 0x0D62)) {
        for (x = 0; x < 0x0280; x++) {
            v = (uint8_t)(rd8(0x7FF883 + x) + 0xD8);
            wr8(0x7FF883 + x, v);
            wr8(0x7FFB03 + x, (uint8_t)(v + 0x50));
        }
    }
    wr16(b | 0x19B6, 0xFFFE);
    wr16(b | 0x19B8, 0xFFFE);
    wr16(b | 0x19BA, 0x0000);
    wr8(b | 0x19BC, 0x00);
    wr8(b | 0x19BE, 0x00);
    wr8(b | 0x19A9, 0x00);
    wr8(b | 0x1A07, 0x00);
    wr8(b | 0x1A0A, 0x00);
    wr8(b | 0x1A0B, 0x00);
    wr8(b | 0x1A17, 0x00);
    wr8(b | 0x1A18, 0x00);
    wr8(b | 0x19BF, 0x00);
    wr8(b | 0x19C0, 0xF9);
    wr8(b | 0x1A0C, 0x01);
    wr8(b | 0x1A0D, 0x01);
    wr8(b | 0x1A08, 0x40);
    wr8(b | 0x1A09, 0x40);
    wr8(b | 0x1A19, 0xFE);
    wr8(b | 0x1A1A, 0xFE);
    wr8(b | 0x1A15, 0xF0);
    wr8(b | 0x1A16, 0xF0);
    for (i = 0; i < 6; i++)
        wr8(b | (uint16_t)(0x19AC + i), 0x00);
    wr8(b | 0x1A0E, 0x00);
    wr8(b | 0x1A0F, 0x00);
    wr8(b | 0x1A1B, 0x00);
    wr8(b | 0x1A1C, 0x1A);
    wr8(b | 0x1A1D, 0x1A);
    wr8(b | 0x1A1E, 0x00);
    wr8(b | 0x1A1F, 0x00);
    wr8(b | 0x1A22, 0x00);
    wr8(b | 0x1A23, 0x00);
    wr8(b | 0x1A20, 0xFE);
    wr8(b | 0x1A21, 0xFE);
    m = rd16(b | 0x0D2B);
    if (m == 0x0000 || m == 0x0003) {
        x = (uint16_t)((rd16(b | 0x0D4A) & 0x00FF) << 1);
        w = (uint16_t)((rd16(b | 0x0D48) & 0x00FF) << 1);
        x = (uint16_t)(w + rd16(0x09F127 + x));
        wr16(b | 0x19F6, rd16(b | x));
    }
    if (rd16(b | 0x0D2B) == 0x0000) {
        x = (uint16_t)((rd16(b | 0x0D4A) & 0x00FF) << 1);
        w = (uint16_t)(rd16(b | 0x0D48) & 0x00FF);
        x = (uint16_t)(w + rd16(0x09F115 + x));
        wr8(b | 0x19E6, rd8(b | x));
    }
    wr16(b | 0x19FE, 0x0000);
    x = (uint8_t)(rd16(b | 0x0D89) << 1);
    wr8(b | 0x19D1, (uint8_t)x);
    wr16(b | 0x19CB, rd16(0x09F14D + x));
    wr16(b | 0x19D3, 0x0000);
    wr16(b | 0x19CF, 0x0017);
    wr16(b | 0x19C5, 0x0020);
    wr16(b | 0x19C3, 0xE020);
    wr16(b | 0x19F3, 0xE600);
    wr8(b | 0x19F5, 0x00);
    wr8(b | 0x1A14, 0x80);
    wr8(b | 0x19D8, 0x00);
    wr8(b | 0x19E0, 0xFE);
    wr8(b | 0x19DF, 0x00);
    wr16(b | 0x19DB, 0x0000);
    wr16(b | 0x19DD, 0x0000);
    wr8(b | 0x1A04, 0xFF);
    {
        static const uint16_t zero[] = {0x19EC, 0x19ED, 0x19E8, 0x19E9, 0x19EA, 0x19EB, 0x19F2,
                                        0x19EE, 0x19EF, 0x19F0, 0x19F1, 0x1A06, 0x1A29};

        for (i = 0; i < sizeof zero / sizeof zero[0]; i++)
            wr8(b | zero[i], 0x00);
    }
    wr8(b | DMAP4, 0x44);
    wr8(b | BBAD4, 0x26);
    wr16(b | A1T4L, 0xF85B);
    wr8(b | A1B4, 0x7F);
    wr16(b | DAS4L, 0x0000);
    wr8(b | DASB4, 0x7F);
    wr8(b | DMAP5, 0x00);
    wr8(b | BBAD5, 0x32);
    wr16(b | A1T5L, 0xFF8B);
    wr8(b | A1B5, 0x7F);
    wr16(b | DAS5L, 0x0000);
    wr8(b | DASB5, 0x7F);
}

void abi_WindowsInit(void)
{
    uint8_t p = cpu.p, db = cpu.db;

    cpu.db = 0x09;
    WindowsInit();
    cpu.a = 0x007F;
    cpu.x = 0x0000;
    cpu.db = db;
    cpu.p = p;
}

void abi_WindowHdmaPreset(void);
void abi_WindowHdmaFromList(void);

/*
 * @routine 7E:D5EC WindowBandsAnimate
 * WHAT IT DOES: Runs the window band animation of state $19D8 (unless
 * $19DF is set): 1 starts an opening (step +$80 from 0 to $0780, or $0C80
 * when $0D62 is clear), 2 a closing (step -$80 from $0700 or $0C00 to 0),
 * both going on to state 4 with $0DA4 cleared; in states 4 and 5 it clears
 * bit 5 of $0D45, selects window setup 8 ($19A9) with $19E0 in
 * $19AC-$19B1, moves $19D4 by the step until it reaches the target (then
 * state 0 when its high byte is 0, else 5) and rebuilds the window table
 * for entry $19D5 (WindowHdmaPreset with $0D62 set, else
 * WindowHdmaFromList). State 3 and others do nothing.
 * HOW: The data bank is the code's own ($7E) throughout; some accesses
 * are long to bank 0. States 3 and above 5 are not reached in the traces
 * so far.
 * Entry: any widths. Exit: P and the data bank restored; X and Y high
 * bytes cleared; A, X as the last step left them.
 */
void abi_WindowBandsAnimate(void)
{
    uint8_t p0 = cpu.p, db0 = cpu.db, v;
    uint32_t b = 0x7E0000;
    uint16_t a;

    cpu.db = 0x7E;
    op_sep(P_M | P_X);
    v = rd8(0x0019DF);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    if (v)
        goto out;
    v = rd8(b | 0x19D8);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    if (!v)
        goto out;
    v = rd8(b | 0x19D8);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    if (v == 0x01 || v == 0x02) {
        uint8_t fast;

        cpu.p &= (uint8_t)~P_M;
        if (v == 0x01) {
            wr16(b | 0x19D6, 0x0080);
            wr16(0x0019D4, 0x0000);
        } else
            wr16(b | 0x19D6, 0xFF80);
        cpu.p |= P_M;
        fast = rd8(b | 0x0D62);
        cpu.p &= (uint8_t)~P_M;
        if (v == 0x01)
            wr16(0x0019D9, fast ? 0x0780 : 0x0C80);
        else {
            wr16(0x0019D4, fast ? 0x0700 : 0x0C00);
            wr16(0x0019D9, 0x0000);
        }
        wr16(0x000DA4, 0x0000);
        op_sep(P_M | P_X);
        wr8(b | 0x19D8, 0x04);
    } else if (v != 0x04 && v != 0x05) {
        goto out;
    }
    cpu.p |= P_M;
    v = (uint8_t)(rd8(0x000D45) & 0xDF);
    wr8(0x000D45, v);
    wr8(b | 0x19A9, 0x08);
    v = rd8(b | 0x19E0);
    {
        unsigned i;

        for (i = 0; i < 6; i++)
            wr8(b | (uint16_t)(0x19AC + i), v);
    }
    cpu.p &= (uint8_t)~P_M;
    a = rd16(0x0019D4);
    cpu.p &= (uint8_t)~P_C;
    a = op_adc16(a, rd16(b | 0x19D6), &cpu.p);
    cpu.a = a;
    op_cmp16(rd16(b | 0x19D9));
    if (!(cpu.p & P_Z))
        wr16(0x0019D4, a);
    else {
        cpu.p |= P_M;
        v = rd8(0x0019D5);
        if (!v) {
            cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
            wr8(b | 0x19D8, 0x00);
            goto out;
        }
        wr8(b | 0x19D8, 0x05);
    }
    op_sep(P_M | P_X);
    v = rd8(0x0019D5);
    cpu.x = (uint8_t)(v << 1);
    v = rd8(b | 0x0D62);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    if (v)
        abi_WindowHdmaPreset();
    else
        abi_WindowHdmaFromList();
    op_sep(P_M | P_X);
out:
    cpu.db = db0;
    op_plp(p0);
}

/* Three bands of 16, 48 and 16 lines at entry offset `at`, all at the
   window address `addr`. */
static void window_bands_3(uint16_t at, uint16_t addr)
{
    wr8(WINDOW_HDMA + at, 0x10);
    wr8(WINDOW_HDMA + at + 3, 0x30);
    wr8(WINDOW_HDMA + at + 6, 0x10);
    wr16(WINDOW_HDMA + at + 1, addr);
    wr16(WINDOW_HDMA + at + 4, addr);
    wr16(WINDOW_HDMA + at + 7, addr);
    cpu.a = addr;
    cpu.p = (uint8_t)((cpu.p & ~(P_Z | P_M)) | P_N);
}

/*
 * @routine 7E:DCC6 WindowHdmaFFD5
 * WHAT IT DOES: Sets the window table's second to fourth bands to 16, 48
 * and 16 lines at $FFD5.
 * HOW: Counts, then addresses. Reached by a unit test
 * (unit_uncalled).
 * Entry: any widths. Exit: as WindowHdmaFFD1, A = $FFD5.
 */
void abi_WindowHdmaFFD5(void)
{
    window_bands_3(3, 0xFFD5);
}

/*
 * @routine 7E:DCEC WindowHdmaFFD9
 * WHAT IT DOES: As WindowHdmaFFD5 for the sixth to eighth bands, at $FFD9.
 * HOW: As WindowHdmaFFD5. Reached by a unit test
 * (unit_uncalled).
 * Entry: any widths. Exit: as WindowHdmaFFD1, A = $FFD9.
 */
void abi_WindowHdmaFFD9(void)
{
    window_bands_3(15, 0xFFD9);
}

void abi_WindowHdmaAt(void);
void abi_WindowHdmaFFDD(void);

/*
 * @routine 7E:D75E WindowSplitAnimate
 * WHAT IT DOES: While $19DF is 1, runs the split window animation of state
 * $19D8: 1 starts an opening ($19DB from 0 by $0400 to $7400, $19E2 =
 * $F50B, $19E4 = 0), 2 a closing (from $8000 by -$0400 to $0800, $19E4 =
 * $0BF5, $19E2 = 0), both going on to state 4 with $0DA4 cleared; in
 * states 4 and 5 it clears bit 5 of $0D45, selects window setup 8 with
 * $19E0 in $19AC-$19B1, and moves $19DB on, setting the bands around line
 * ($FF - 2 * $19DC) / 2 (WindowHdmaAt); at the end, from $7000 up it sets
 * the full bands (WindowHdmaFFDD) and state 5, below it state 0.
 * HOW: The data bank is the code's ($7E); some accesses are long to bank
 * 0. The closing, state 3 and the state 0 end are not reached in the
 * traces so far.
 * Entry: any widths. Exit: P and the data bank restored; X and Y high
 * bytes cleared; A as the last step left it.
 */
void abi_WindowSplitAnimate(void)
{
    uint8_t p0 = cpu.p, db0 = cpu.db, v, h;
    uint32_t b = 0x7E0000;
    uint16_t a;
    unsigned i;

    cpu.db = 0x7E;
    op_sep(P_M | P_X);
    v = rd8(0x0019DF);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    if (v != 0x01)
        goto out;
    v = rd8(b | 0x19D8);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    if (!v)
        goto out;
    v = rd8(b | 0x19D8);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    if (v == 0x01 || v == 0x02) {
        cpu.p &= (uint8_t)~P_M;
        if (v == 0x01) {
            wr16(0x0019DD, 0x0400);
            wr16(0x0019DB, 0x0000);
            wr16(0x0019D9, 0x7400);
            wr16(b | 0x19E2, 0xF50B);
            wr16(b | 0x19E4, 0x0000);
        } else {
            wr16(0x0019DD, 0xFC00);
            wr16(0x0019DB, 0x8000);
            wr16(0x0019D9, 0x0800);
            wr16(b | 0x19E4, 0x0BF5);
            wr16(b | 0x19E2, 0x0000);
        }
        wr16(0x000DA4, 0x0000);
        op_sep(P_M | P_X);
        wr8(0x0019D8, 0x04);
    } else if (v != 0x04 && v != 0x05) {
        goto out;
    }
    cpu.p |= P_M;
    v = (uint8_t)(rd8(0x000D45) & 0xDF);
    wr8(0x000D45, v);
    wr8(b | 0x19A9, 0x08);
    v = rd8(b | 0x19E0);
    for (i = 0; i < 6; i++)
        wr8(b | (uint16_t)(0x19AC + i), v);
    cpu.p &= (uint8_t)~(P_M | P_C);
    a = rd16(0x0019DB);
    a = op_adc16(a, rd16(0x0019DD), &cpu.p);
    cpu.a = a;
    op_cmp16(rd16(b | 0x19D9));
    if (!(cpu.p & P_Z)) {
        wr16(0x0019DB, a);
        cpu.p |= P_M;
        h = rd8(0x0019DC);
        cpu.p |= P_C;
        v = op_sbc8(0xFF, h, &cpu.p);
        h = rd8(0x0019DC);
        cpu.p |= P_C;
        v = op_sbc8(v, h, &cpu.p);
        cpu.p = (uint8_t)((cpu.p & ~P_C) | (v & 0x01));
        v = (uint8_t)(v >> 1);
        cpu.p = nz8(cpu.p, v);
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
        abi_WindowHdmaAt();
        goto out;
    }
    a = rd16(0x0019DB);
    cpu.a = a;
    op_cmp16(0x7000);
    if (a >= 0x7000) {
        abi_WindowHdmaFFDD();
        cpu.p |= P_M;
        wr8(b | 0x19D8, 0x05);
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x05);
    } else {
        cpu.p |= P_M;
        wr8(b | 0x19D8, 0x00);
    }
out:
    cpu.db = db0;
    op_plp(p0);
}

/*
 * @routine 0B:F567 WindowHdmaChannelsSetup
 * WHAT IT DOES: Sets up HDMA channels 3-7 from the tables in bank $7F:
 * 7 to $210F (BG2 x, two writes) from $12D5, 6 to $2110 indirect from
 * $12E2 (bank 0 data), 5 to $212C from $12F2, 4 and 3 to $2127 from
 * $12FD and $1308; the window lines $7F:130D and $7F:130F cleared.
 * HOW: 8-bit register writes, the addresses 16-bit; through the data bank;
 * P saved and restored.
 * Entry: any widths. Exit: P restored; A = 0; X = $1308.
 */
void abi_WindowHdmaChannelsSetup(void)
{
    static const struct { uint8_t dmap, bbad; uint16_t a1t; } ch[5] = {
        {0x02, 0x0F, 0x12D5}, {0x42, 0x10, 0x12E2}, {0x00, 0x2C, 0x12F2}, {0x00, 0x27, 0x12FD}, {0x00, 0x27, 0x1308}};
    uint32_t b = (uint32_t)cpu.db << 16;
    uint8_t p0 = cpu.p;
    int i;

    op_sep(P_M);
    op_rep(P_X);
    for (i = 0; i < 5; i++) {
        uint16_t base = (uint16_t)(0x4370 - 0x10 * i);

        wr8(b | base, ch[i].dmap);
        wr8(b | (uint16_t)(base + 1), ch[i].bbad);
        cpu.x = ch[i].a1t;
        wr16(b | (uint16_t)(base + 2), cpu.x);
        wr8(b | (uint16_t)(base + 4), 0x7F);
        if (i == 1)
            wr8(b | (uint16_t)(base + 7), 0x00);
    }
    cpu.a = (uint16_t)(cpu.a & 0xFF00);
    wr8(0x7F130D, 0x00);
    wr8(0x7F130F, 0x00);
    op_plp(p0);
}

/* CMP (8-bit) of v with m: N, Z and C. */
static void cmp8(uint8_t v, uint8_t m)
{
    cpu.p = (uint8_t)((nz8(cpu.p, (uint8_t)(v - m)) & ~P_C) | (v >= m ? P_C : 0));
}

/* LDA, CLC, ADC (8-bit) of two bytes, then CMP with the limit pair at
   `limits`: the sum is stored back at `at` unless it equals the first limit
   or is below the second. */
static void band_step(uint32_t at, uint32_t step, uint32_t limits)
{
    uint8_t v = rd8(at);

    cpu.p &= (uint8_t)~P_C;
    v = op_adc8(v, rd8(step), &cpu.p);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cmp8(v, rd8(limits));
    if (cpu.p & P_Z)
        return;
    cmp8(v, rd8(limits + 1));
    if (!(cpu.p & P_C))
        return;
    wr8(at, v);
}

/*
 * @routine 7E:D4F6 ColdataTopBandStep
 * WHAT IT DOES: Moves the line count of the COLDATA table's first band
 * ($19CB) by $19C7, unless the new count equals the first byte of the
 * limit pair $19D1 in the list at $7E:D5AA or is below the second.
 * HOW: 8-bit; the pair's offset is $19D1.
 * Entry: any widths; data bank $7E. Exit: M and X 8-bit; A low byte the
 * new count; X = $19D1; N, Z, C from the last compare.
 */
void abi_ColdataTopBandStep(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;

    op_sep(P_M | P_X);
    cpu.x = rd8(b | 0x19D1);
    band_step(0x0019CB, b | 0x19C7, 0x7ED5AA + cpu.x);
}

/*
 * @routine 7E:D514 ColdataLowBandStep
 * WHAT IT DOES: Moves the line count of the COLDATA table's band at
 * $7F:FFAB ($19CF) by $19CD, within the limit pair at $7E:D56E: pair 0
 * normally, pair $02 (from direct page $02) while bit 14 is set in the
 * word that the player object's ($0346) link word ($204F) leads to by two
 * links. Each change of that bit restarts the count, at $20 (clear) or
 * $40 (set), as $19D3 records it.
 * HOW: 16-bit for the links, then 8-bit; the step as ColdataTopBandStep.
 * The bit set, and each restart, are not reached in the traces so far.
 * Entry: any widths; data bank $7E. Exit: M and X 8-bit; A low byte the
 * new count; X the pair's offset; Y the second link (high byte clear); N,
 * Z, C from the last compare.
 */
void abi_ColdataLowBandStep(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint8_t v;

    op_rep(P_M | P_X);
    cpu.x = rd16(b | 0x0346);
    cpu.y = rd16(b + 0x204F + cpu.x);
    cpu.a = rd16(b + 0x204F + cpu.y);
    cpu.y = cpu.a;
    cpu.a = (uint16_t)(rd16(b + cpu.y) & 0x4000);
    cpu.p = nz16(cpu.p, cpu.a);
    op_sep(P_M | P_X);
    v = rd8(b | 0x19D3);
    if (cpu.a) {
        if (!v) {
            wr8(b | 0x19D3, 0x01);
            wr8(0x0019CF, 0x40);
            v = 0x40;
        }
        cpu.x = rd8((uint16_t)(cpu.d + 0x02));
    } else {
        if (v) {
            wr8(b | 0x19D3, 0x00);
            wr8(0x0019CF, 0x20);
            v = 0x20;
        }
        cpu.x = 0x00;
    }
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    band_step(0x0019CF, b | 0x19CD, 0x7ED56E + cpu.x);
}

/*
 * @routine 7E:D572 ColdataLevelStep
 * WHAT IT DOES: Moves the fixed colour level $19C3 (whole part in $19C4,
 * $E0 to $EF: COLDATA for all three colours) by $19C5; unless the sum is
 * $E000 or $EF00 (then nothing is stored), stores it and lowers each band
 * of the COLDATA HDMA table at $7F:FF8E that is at or above the new whole
 * part to it. The bands are every other byte of the list at $7E:D5B3, to
 * its $FF.
 * HOW: A 16-bit add, then 8-bit with X stepping by 2 from 0.
 * Entry: any widths; data bank $7E (for $19C5). Exit: at a limit, M and X
 * 16-bit and A the sum; otherwise M and X 8-bit, A low byte $FF (high byte
 * the new whole part) and X at the list's end; Z and C set, V from the
 * add.
 */
void abi_ColdataLevelStep(void)
{
    uint8_t x, v;

    op_rep(P_M | P_X);
    cpu.a = rd16(0x0019C3);
    cpu.p &= (uint8_t)~P_C;
    cpu.a = op_adc16(cpu.a, rd16((uint32_t)cpu.db << 16 | 0x19C5), &cpu.p);
    op_cmp16(0xE000);
    if (cpu.p & P_Z)
        return;
    op_cmp16(0xEF00);
    if (cpu.p & P_Z)
        return;
    wr16(0x0019C3, cpu.a);
    op_sep(P_M | P_X);
    for (x = 0;; x = (uint8_t)(x + 2)) {
        v = rd8(0x7ED5B3 + x);
        if (v == 0xFF)
            break;
        cmp8(v, rd8(0x0019C4));
        if (cpu.p & P_C)
            wr8(0x7FFF8E + x, rd8(0x0019C4));
    }
    cpu.x = x;
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0xFF);
    cpu.p = (uint8_t)((cpu.p & ~P_N) | P_Z | P_C);
}

/*
 * @routine 7E:D423 WindowSlideCheck
 * WHAT IT DOES: Steps the sliding window when one is under way: with the
 * window's HDMA on ($19D8 = 0), a slide set up ($19BC not $FF) and bit 0 of
 * $0DA5 set: window setup 6 ($19A9) with $E0 in $19AC-$19AE (and
 * WindowHdmaFFD1 once the colour level's whole part $19C4 is $E1 or more),
 * then ColdataLevelStep. When the level would reach $E000 the slide ends
 * (bit 0 of $0DA5 and bit 5 of $0D45 cleared); otherwise, if the player
 * object's ($0346) Y ($200E) has changed since $19C9, the band steps go up
 * ($02 and $FF) or down ($FE and $01) for ColdataTopBandStep and
 * ColdataLowBandStep; then the two band counts go to the COLDATA table at
 * $7F:FF8B and $7F:FFAB, the Y to $19C9, and bit 5 of $0D45 is set.
 * HOW: P and the data bank pushed ($7E) around it. The level's limit check
 * repeats ColdataLevelStep's add. The end of the slide, the $E1 case and a
 * Y below $19C9 are not reached in the traces so far.
 * Entry: any widths. Exit: P and the data bank restored; A, X, Y as the
 * last step left them (A low byte $0D45 after the slide's steps, 0 when no
 * slide).
 */
static void window_slide_check_step(void)
{
    const uint32_t b = 0x7E0000;
    uint8_t v;
    uint16_t m;

    op_sep(P_M);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0xE0);
    wr8(0x0019AC, 0xE0);
    wr8(0x0019AD, 0xE0);
    wr8(0x0019AE, 0xE0);
    op_lda8(0x0019C4);
    cmp8((uint8_t)cpu.a, 0xE1);
    if (cpu.p & P_C)
        abi_WindowHdmaFFD1();
    op_sep(P_M);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x06);
    wr8(b | 0x19A9, 0x06);
    abi_ColdataLevelStep();
    op_rep(P_M);
    cpu.a = rd16(0x0019C3);
    cpu.p &= (uint8_t)~P_C;
    cpu.a = op_adc16(cpu.a, rd16(b | 0x19C5), &cpu.p);
    if (cpu.a == 0xE000) {
        op_sep(P_M | P_X);
        v = (uint8_t)(rd8(0x000DA5) & 0xFE);
        wr8(0x000DA5, v);
        v = (uint8_t)(rd8(0x000D45) & 0xDF);
        wr8(0x000D45, v);
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
        return;
    }
    op_rep(P_M | P_X);
    cpu.x = rd16(b | 0x0346);
    cpu.a = rd16(0x0019C9);
    m = rd16(b + 0x200E + cpu.x);
    if (cpu.a != m) {
        int down = cpu.a >= m;

        op_sep(P_M);
        wr8(0x0019C7, down ? 0xFE : 0x02);
        wr8(0x0019CD, down ? 0x01 : 0xFF);
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | (down ? 0x01 : 0xFF));
        abi_ColdataTopBandStep();
        abi_ColdataLowBandStep();
    }
    op_sep(P_M);
    v = rd8(0x0019CB);
    wr8(0x7FFF8B, v);
    v = rd8(0x0019CF);
    wr8(0x7FFFAB, v);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    op_rep(P_M | P_X);
    cpu.x = rd16(b | 0x0346);
    cpu.a = rd16(b + 0x200E + cpu.x);
    wr16(0x0019C9, cpu.a);
    op_sep(P_M);
    v = (uint8_t)(rd8(0x000D45) | 0x20);
    wr8(0x000D45, v);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
}

void abi_WindowSlideCheck(void)
{
    const uint32_t b = 0x7E0000;
    uint8_t p0 = cpu.p, db0 = cpu.db;

    cpu.db = 0x7E;
    op_sep(P_M);
    op_lda8(b | 0x19D8);
    if (!(uint8_t)cpu.a) {
        op_lda8(0x0019BC);
        if ((uint8_t)cpu.a != 0xFF) {
            op_lda8(b | 0x0DA5);
            if (cpu.a & 0x01)
                window_slide_check_step();
            else
                cpu.a = (uint16_t)(cpu.a & 0xFF00);
        }
    }
    cpu.db = db0;
    op_plp(p0);
}
