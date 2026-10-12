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
