/*
 * window_hdma.c: the window HDMA table in bank $7F ($7F:F85B). Each entry is
 * three bytes: a line count and a 16-bit address of the window values for
 * those lines; a zero count ends the table. $F857 is the normal window
 * setting; $FFD1 and $FFDD are other ones.
 */
#include "machine.h"
#include "native.h"
#include "hw.h"
#include "cpu_ops.h"
#include "video_calls.h"
#include "carried.h"
#include "wram_names.h"

uint16_t Records7FFromF170(uint8_t bank);
void Records7FFromF634(uint8_t bank, uint16_t *x, uint16_t *y, uint8_t *p);


/*
 * @routine 7E:D8A8 WindowHdmaFFDD
 * WHAT IT DOES: Sets the window table to four bands (64, 64, 64, 62 lines),
 * all using the window values at $FFDD.
 * HOW: Writes the four counts and the end marker, then the four addresses.
 * Entry: any widths. Exit: A = $FFDD; M 16-bit; N set, Z clear.
 */
void WindowHdmaFFDD(void)
{
    cyc(70);
    wr8(W(window_hdma) + 0, 0x40);
    cyc(48);
    wr8(W(window_hdma) + 3, 0x40);
    cyc(48);
    wr8(W(window_hdma) + 6, 0x40);
    cyc(48);
    wr8(W(window_hdma) + 9, 0x3E);
    cyc(48);
    wr8(W(window_hdma) + 12, 0x00);
    cyc(78);
    wr16(W(window_hdma) + 1, 0xFFDD);
    cyc(32);
    wr16(W(window_hdma) + 4, 0xFFDD);
    cyc(32);
    wr16(W(window_hdma) + 7, 0xFFDD);
    cyc(32);
    wr16(W(window_hdma) + 10, 0xFFDD);
    cyc(42);
}

void ColdataLevelStep_cr(Carried *cr);
void ColdataLowBandStep_cr(Carried *cr);
void ColdataTopBandStep_cr(Carried *cr);
void WindowBandsAnimate_cr(Carried *cr);
void WindowEdgesFromList_cr(Carried *cr);
void WindowEdgesFromPair_cr(Carried *cr);
void WindowHdmaAt_cr(Carried *cr);
void WindowHdmaBands_cr(Carried *cr);
void WindowHdmaChannelsSetup_cr(Carried *cr);
void WindowHdmaFFD1_cr(Carried *cr);
void WindowHdmaFFD5_cr(Carried *cr);
void WindowHdmaFFD9_cr(Carried *cr);
void WindowHdmaFFDD_cr(Carried *cr);
void WindowHdmaFromList_cr(Carried *cr);
void WindowHdmaFromTable_cr(Carried *cr);
void WindowHdmaLayout_cr(Carried *cr);
void WindowHdmaNormal_cr(Carried *cr);
void WindowHdmaPreset_cr(Carried *cr);
void WindowLayoutStep_cr(Carried *cr);
void WindowLayoutStepForScreen_cr(Carried *cr);
void WindowSlideCheck_cr(Carried *cr);
void WindowSlideDown_cr(Carried *cr);
void WindowSlideStep_cr(Carried *cr);
void WindowSlideUp_cr(Carried *cr);
void WindowSplitAnimate_cr(Carried *cr);
void WindowsInit_cr(Carried *cr);
void WindowsInit(Carried *cr);
static void records_7f(Carried *cr, int course_screen);
static void w_Records7FFromF170(Carried *cr);
static void w_Records7FFromF634(Carried *cr);

void WindowHdmaFFDD_cr(Carried *cr)
{
    WindowHdmaFFDD();
    cr->a = 0xFFDD;
    set_p(cr, (uint8_t)((cr->p & ~(P_Z | P_M)) | P_N));
}

void abi_WindowHdmaFFDD(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    WindowHdmaFFDD_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
uint16_t WindowHdmaFromTable(uint16_t *sel)
{
    cyc(46);
    uint16_t x = rd16(W(window_hdma_select)), mid;
    uint8_t n;

    *sel = x;
    cyc(56);
    wr16(W(window_hdma) + 1, 0xF857);
    cyc(32);
    wr16(W(window_hdma) + 7, 0xF857);
    cyc(32);
    mid = rd16(W(window_hdma_addrs) + x);
    cyc(32);
    wr16(W(window_hdma) + 4, mid);
    cyc(54);
    n = rd8(W(window_hdma_counts) + x);
    cyc(32);
    wr8(W(window_hdma) + 0, n);
    cyc(32);
    n = rd8(W(wram_7EE0B5) + x);
    cyc(32);
    wr8(W(window_hdma) + 3, n);
    cyc(48);
    wr8(W(window_hdma) + 6, 0x30);
    cyc(48);
    wr8(W(window_hdma) + 9, 0x00);
    cyc(42);
    return mid;
}

void WindowHdmaFromTable_cr(Carried *cr)
{
    uint16_t sel, mid = WindowHdmaFromTable(&sel);

    cr->a = (uint16_t)(mid & 0xFF00);
    cr->x = sel;
    set_p(cr, (uint8_t)(((cr->p & ~(P_N | P_X)) | P_Z | P_M)));
}

void abi_WindowHdmaFromTable(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    WindowHdmaFromTable_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
    cyc(70);
    wr8(W(window_hdma) + 0, 0x20);
    cyc(48);
    wr8(W(window_hdma) + 3, 0x70);
    cyc(48);
    wr8(W(window_hdma) + 6, 0x10);
    cyc(48);
    wr8(W(window_hdma) + 9, 0x08);
    cyc(48);
    wr8(W(window_hdma) + 12, 0x00);
    cyc(78);
    wr16(W(window_hdma) + 1, 0xF857);
    cyc(32);
    wr16(W(window_hdma) + 10, 0xF857);
    cyc(56);
    wr16(W(window_hdma) + 4, 0xFFD1);
    cyc(32);
    wr16(W(window_hdma) + 7, 0xFFD1);
    cyc(42);
}

void WindowHdmaFFD1_cr(Carried *cr)
{
    WindowHdmaFFD1();
    cr->a = 0xFFD1;
    set_p(cr, (uint8_t)((cr->p & ~(P_Z | P_M)) | P_N));
}

void abi_WindowHdmaFFD1(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    WindowHdmaFFD1_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* Window HDMA in use, and a layout of $FF meaning none: both skip the
   builders below. *last gets the byte read, A when a builder skips. */
static int window_hdma_off(uint8_t *last)
{
    cyc(140);
    *last = rd8(W(window_hdma_off));
    return *last != 0;
}

/*
 * @routine 7E:DBCD WindowHdmaNormal
 * WHAT IT DOES: Sets the window table to the normal window over the whole
 * screen (four bands of 64 lines), unless window HDMA is off or no layout is
 * set ($19BC = $FF).
 * HOW: Four entries at $F857 and the end marker.
 * Entry: any widths. Exit: A = $F857 when written (the result), else A's
 * low byte = *last, the byte that stopped it; X and Y high bytes clear; P
 * unchanged.
 */
static int window_hdma_normal(uint8_t *last)
{
    unsigned i;

    if (window_hdma_off(last))
        return 0;
    cyc(48);
    *last = rd8(W(window_hdma_19bc));
    if (*last == 0xFF)
        { cyc(16); return 0; } else { cyc(54); }
    for (i = 0; i < 4; i++)
        { cyc(48); wr8(W(window_hdma) + 3 * i, 0x40); }
    cyc(48);
    wr8(W(window_hdma) + 12, 0x00);
    cyc(46);
    for (i = 0; i < 4; i++)
        { cyc(32); wr16(W(window_hdma) + 3 * i + 1, 0xF857); }
    return 1;
}

int WindowHdmaNormal(uint8_t *last)
{
    int written = window_hdma_normal(last);

    if (!written)
        cyc(22);
    cyc(100);
    return written;
}

void WindowHdmaNormal_cr(Carried *cr)
{
    uint8_t last;

    if (WindowHdmaNormal(&last))
        cr->a = 0xF857;
    else
        cr->a = (uint16_t)((cr->a & 0xFF00) | last);
    cr->x &= 0x00FF;
    cr->y &= 0x00FF;
}

void abi_WindowHdmaNormal(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    WindowHdmaNormal_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
        { cyc(22); return 0; } else { cyc(38); }
    for (i = 0; i < 9; i++)
        { cyc(48); wr8(W(window_hdma) + 3 * i, lines[i]); }
    cyc(48);
    wr8(W(window_hdma) + 27, 0x00);
    cyc(46);
    for (i = 0; i < 9; i++)
        { cyc(32); wr16(W(window_hdma) + 3 * i + 1, 0xF857); }
    return 1;
}

int WindowHdmaBands(uint8_t *last)
{
    int written = window_hdma_bands(last);

    cyc(100);
    return written;
}

void WindowHdmaBands_cr(Carried *cr)
{
    uint8_t last;

    if (WindowHdmaBands(&last))
        cr->a = 0xF857;
    else
        cr->a = (uint16_t)((cr->a & 0xFF00) | last);
    cr->x &= 0x00FF;
    cr->y &= 0x00FF;
}

void abi_WindowHdmaBands(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    WindowHdmaBands_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 7E:D85C WindowHdmaAt
 * WHAT IT DOES: Sets the window table to four bands around line A: normal
 * windows in the middle two bands, $FFDD above and below.
 * HOW: Band heights come from A and the offsets at $19E2-$19E5 and the height
 * at $19DC.
 * Entry: M 8-bit; A low byte = line. Exit: A = $FFDD; M 16-bit; N set, Z
 * clear; C and V (in *p) from the last height sum.
 */
void WindowHdmaAt(uint8_t line, uint8_t *p)
{
    cyc(268);
    uint8_t h = rd8(W(window_lines)), e2 = rd8(cyc_then(186, W(window_19e2)));
    unsigned sum;

    cyc(32);
    wr8(W(window_hdma) + 3, (uint8_t)(line + rd8(W(window_19e5))));
    wr8(W(window_hdma) + 6, (uint8_t)(line + rd8(W(window_19e3))));
    wr8(W(window_hdma) + 0, (uint8_t)(h - 1 + rd8(W(window_19e4))));
    sum = (unsigned)h + e2;
    wr8(W(window_hdma) + 9, (uint8_t)sum);
    *p = (uint8_t)((*p & ~(P_C | P_V)) | (sum > 0xFF ? P_C : 0)
                   | ((~(h ^ e2) & (h ^ (uint8_t)sum) & 0x80) ? P_V : 0));
    cyc(48);
    wr8(W(window_hdma) + 12, 0x00);
    cyc(78);
    wr16(W(window_hdma) + 4, 0xF857);
    cyc(32);
    wr16(W(window_hdma) + 7, 0xF857);
    cyc(56);
    wr16(W(window_hdma) + 1, 0xFFDD);
    cyc(32);
    wr16(W(window_hdma) + 10, 0xFFDD);
    cyc(42);
}

void WindowHdmaAt_cr(Carried *cr)
{
    uint8_t p = cr->p;

    WindowHdmaAt((uint8_t)cr->a, &p);
    cr->a = 0xFFDD;
    set_p(cr, (uint8_t)((p & ~(P_Z | P_M)) | P_N));
}

void abi_WindowHdmaAt(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    WindowHdmaAt_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
WindowListEnd WindowHdmaFromList(uint8_t db, uint8_t select)
{
    WindowListEnd end;
    cyc(76);
    uint16_t y = rd16(W(window_hdma_lists) + select), x = 3;
    uint8_t v;

    end.a = y;
    cyc(106);
    wr8(W(window_hdma), 0x20);
    cyc(78);
    wr16(W(window_hdma) + 1, 0xF857);
    for (;;) {
        cyc(76);
        v = rd8((uint32_t)db << 16 | y);
        cyc(32);
        wr8(W(window_hdma) + x, v);
        if (v == 0)
            break; else { cyc(22); }
        y++;
        x++;
    }
    end.x = x;
    end.y = y;
    return end;
}

void WindowHdmaFromList_cr(Carried *cr)
{
    WindowListEnd end = WindowHdmaFromList(cr->db, (uint8_t)cr->x);

    cr->a = 0xF800;
    cr->x = end.x;
    cr->y = end.y;
    set_p(cr, (uint8_t)(((cr->p & ~(P_N | P_X)) | P_Z | P_M)));
}

void abi_WindowHdmaFromList(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    WindowHdmaFromList_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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

    cyc(70);
    wr8(W(window_preset_count_w), 0x00);
    cyc(32);
    if (rd8(W(wram_0D2E))) {
        *y = rd16(W(window_presets_b) + index);
        *x = 0x000F;
    } else {
        cyc(98);
        *y = rd16(W(window_presets_a) + index);
        *x = 0x0003;
    }
    cyc(44);
    do {
        cyc(68);
        wr8(W(window_hdma) + *x, cyc_then(32, rd8(b | *y)));
        (*y)++;
        (*x)++;
        cyc(60);
        n = (uint8_t)(rd8(W(window_preset_count_w)) + 1);
        cyc(54);
        wr8(0x7E0000 | W(window_preset_count_w), n);
    } while (n != 9);
    cyc(34);
    return (uint16_t)(((uint16_t)(*y - 9) & 0xFF00) | 0x09);
}

void WindowHdmaPreset_cr(Carried *cr)
{
    cr->a = WindowHdmaPreset(cr->db, (uint8_t)cr->x, &cr->x, &cr->y);
    set_p(cr, (uint8_t)(((cr->p & ~(P_N | P_X)) | P_Z | P_C | P_M)));
}

void abi_WindowHdmaPreset(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    WindowHdmaPreset_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
void WindowEdgesFillOnes(void)
{
    uint16_t x = 0, y = 0x0800;

    do {
        cyc(x == 0 ? 126 : 144);
        wr16((W(window_edges) + x) & 0xFFFFFF, 0x0001);
        x = (uint16_t)(x + 2);
        y--;
    } while (y);
}

/* WindowEdgesFillOnes on carried registers. */
void WindowEdgesFillOnes_cr(Carried *cr)
{
    WindowEdgesFillOnes();
    cr->a = 0x0001;
    cr->x = 0x1000;
    cr->y = 0;
    set_p(cr, (uint8_t)((cr->p & ~(P_M | P_X | P_N)) | P_Z | P_C));
    /* The RTS, through the return address the fill left on the stack,
       in the program bank $09. */
    cyc(102);
    cpu_leave_to(0x090000 | (uint16_t)(rd16((uint16_t)(cpu.s + 1)) + 1));
    cpu.s = (uint16_t)(cpu.s + 2);
    cyc(6);                             /* the RTS's last cycle */
}

void abi_WindowEdgesFillOnes(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    WindowEdgesFillOnes_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
    cyc(136);
    uint8_t v0 = rd8(b | pair), v1, v;

    wr8(W(window_edges) + rd16(0x7E0000 | W(wram_19B4)), cyc_then(32, v0));
    cyc(188);
    v = mirror80(rd8(b | pair), p);
    wr8(W(wram_7FF884) + rd16(0x7E0000 | W(wram_19B4)), cyc_then(32, v));
    cyc(128);
    v1 = rd8(b | (uint16_t)(pair + 1));
    wr8(W(wram_7FF885) + rd16(0x7E0000 | W(wram_19B4)), cyc_then(32, v1));
    cyc(200);
    if (rd8(b | (uint16_t)(pair + 1)) == 0x81) {
        *p |= P_C;
        v = 0x80;
    } else {
        cyc(64);
        v = mirror80(rd8(b | (uint16_t)(pair + 1)), p);
    }
    wr8(W(wram_7FF886) + rd16(0x7E0000 | W(wram_19B4)), cyc_then(32, v));
    return v;
}

void WindowEdgesFromPair_cr(Carried *cr)
{
    uint8_t p = cr->p, v = WindowEdgesFromPair(cr->db, cr->x, &p);

    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    set_p(cr, nz16(p, cr->x));
}

void abi_WindowEdgesFromPair(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    WindowEdgesFromPair_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
    cyc(46);
    uint16_t sel = rd16(0x7E0000 | W(window_hdma_select)), n = 0, s;

    cyc(86);
    wr16(0x7E0000 | W(wram_19B4), rd16(0x09F155 + sel));
    cyc(24);
    *x = rd16(cyc_then(62, 0x09F98B + rd16(0x7E0000 | W(window_hdma_select))));
    for (;;) {
        cyc(92);
        WindowEdgesFromPair(bank, *x, p);
        *x = (uint16_t)(*x + 2);
        n++;
        cyc(178);
        if (rd8(b | *x) == 0xFF)
            break;
        cyc(58);
        wr16(0x7E0000 | W(wram_19B4), cyc_then(62, (uint16_t)(rd16(0x7E0000 | W(wram_19B4)) + 4)));
    }
    *x = (uint16_t)(*x - 2);
    for (;;) {
        cyc(92);
        WindowEdgesFromPair(bank, *x, p);
        *x = (uint16_t)(*x - 2);
        if (!--n)
            break;
        cyc(206);
        wr16(0x7E0000 | W(wram_19B4), cyc_then(62, (uint16_t)(rd16(0x7E0000 | W(wram_19B4)) + 4)));
    }
    cyc(212);
    sel = rd16(0x7E0000 | W(window_hdma_select));
    s = (uint16_t)(sel + 2);
    *p = (uint8_t)((nz16(*p, s) & ~(P_C | P_V)) | ((unsigned)sel + 2 > 0xFFFF ? P_C : 0) | ((~sel & s & 0x8000) ? P_V : 0));
    cyc(62);
    wr16(0x7E0000 | W(window_hdma_select), s);
    cyc(42);
    return s;
}

void WindowEdgesFromList_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = WindowEdgesFromList(cr->db, &cr->x, &p);
    cr->y = 0;
    set_p(cr, (uint8_t)(p & ~(P_M | P_X)));
}

void abi_WindowEdgesFromList(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    WindowEdgesFromList_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
    (void)bank;
    uint16_t w;
    uint8_t n;

    cyc(54);
    if (!rd8(W(wram_0D2E))) {
        cyc(62);
        *x = rd16(0x7E0000 | W(window_hdma_select));
        cyc(56);
        wr16(W(wram_7FF85C), 0xF857);
        cyc(32);
        wr16(W(wram_7FF862), 0xF857);
        cyc(32);
        w = rd16(W(window_hdma_addrs) + *x);
        cyc(32);
        wr16(W(wram_7FF85F), w);
        cyc(54);
        wr8(W(window_hdma), cyc_then(32, rd8(W(wram_7EE0C2) + *x)));
        cyc(32);
        wr8(W(wram_7FF85E), cyc_then(32, rd8(W(wram_7EE0C3) + *x)));
        cyc(48);
        wr8(W(wram_7FF861), 0x10);
        cyc(48);
        wr8(W(wram_7FF864), 0x00);
    } else {
        cyc(68);
        *x = rd16(0x7E0000 | W(window_hdma_select));
        cyc(56);
        wr16(W(wram_7FF85C), 0xF857);
        cyc(32);
        wr16(W(wram_7FF85F), 0xF857);
        cyc(32);
        wr16(W(wram_7FF865), 0xF857);
        cyc(32);
        w = rd16(W(wram_7EE0E0) + *x);
        cyc(32);
        wr16(W(wram_7FF862), w);
        cyc(54);
        n = rd8(W(wram_7EE0CA) + *x);
        cyc(32);
        wr8(W(window_hdma), n);
        cyc(32);
        wr8(W(wram_7FF85E), n);
        cyc(32);
        n = rd8(W(wram_7EE0CB) + *x);
        cyc(32);
        wr8(W(wram_7FF861), n);
        cyc(48);
        wr8(W(wram_7FF864), 0x10);
        cyc(48);
        wr8(W(wram_7FF867), 0x00);
    }
    cyc(42);
    return (uint16_t)(w & 0xFF00);
}

void WindowHdmaLayout_cr(Carried *cr)
{
    cr->a = WindowHdmaLayout(cr->db, &cr->x);
    set_p(cr, (uint8_t)(((cr->p & ~(P_N | P_X)) | P_Z | P_M)));
}

void abi_WindowHdmaLayout(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    WindowHdmaLayout_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 7E:D28F WindowLayoutStepForScreen
 * WHAT IT DOES: Clears bit 5 of $0D45 and steps the window layout
 * (WindowLayoutStep) with Y = 0, or Y = 2 on the menu's course screen
 * ($0D62 set).
 * HOW: Data bank $7E inside, P and the data bank saved and restored.
 * Entry: any widths; a_in = A (its high byte may come back), *x = X. Exit:
 * the result is A, *x = X and *y = Y as WindowLayoutStep leaves them; P
 * and the data bank unchanged.
 */
uint16_t WindowLayoutStepForScreen(uint16_t a_in, uint16_t *x, uint16_t *y)
{
    uint8_t v, p = 0;
    int keep;
    uint16_t a;

    cyc(148);
    v = (uint8_t)(rd8(W(wram_0D45)) & 0xDF);
    cyc(48);
    wr8(W(wram_0D45), v);
    cyc(24);
    v = rd8(0x7E0000 | W(wram_0D62));
    *y = v ? (cyc(114), 0x0002) : (cyc(108), 0x0000);
    a = WindowLayoutStep(0x7E, (uint8_t)*y, x, &p, &keep);
    cyc(v ? 100 : 124);                 /* (JMP,) PLB, PLP, RTL */
    return keep ? (uint16_t)((a_in & 0xFF00) | a) : a;
}

void WindowLayoutStepForScreen_cr(Carried *cr)
{
    uint16_t x = cr->x, y;

    cr->a = WindowLayoutStepForScreen(cr->a, &x, &y);
    cr->x = x;
    cr->y = y;
}

void abi_WindowLayoutStepForScreen(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    WindowLayoutStepForScreen_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* One step of the window slide: the slide position at $1A14 moves by
   `step`; reaching `stop` ends the slide (bits 1 and 2 of $0DA5 cleared),
   otherwise the window table becomes WindowHdmaFFD1's and the window setup
   $19AC-$19AE = $20, $40, the position. */
static int window_slide(uint8_t bank, uint8_t step, uint8_t stop, uint8_t *a, uint8_t *pos)
{
    (void)bank;
    cyc(164);
    uint8_t v = (uint8_t)(rd8(W(window_slide_pos)) + step);

    *pos = v;

    if (v == stop) {
        cyc(92);
        *a = (uint8_t)(rd8(0x7E0000 | W(slide_flags)) & 0xF9);
        cyc(40);
        wr8(0x7E0000 | W(slide_flags), *a);
        return 0;
    }
    cyc(94);
    wr8(W(window_slide_pos), v);
    (cyc(step == 0x01 ? 70 : 62), WindowHdmaFFD1());  /* JMP, JSR */
    cyc(70);
    wr8(W(wram_19AD), 0x40);
    cyc(48);
    wr8(W(window_setup_bytes), 0x20);
    cyc(32);
    *a = rd8(W(window_slide_pos));
    cyc(32);
    wr8(W(wram_19AE), *a);
    return 1;
}

/* The slide and the flags it leaves in *p; *moved when it moved (then A's
   high byte is $FF, WindowHdmaFFD1's). */
static uint8_t window_slide_flags(uint8_t bank, uint8_t step, uint8_t stop, uint8_t *p, int *moved)
{
    uint8_t a, v;

    *moved = window_slide(bank, step, stop, &a, &v);

    uint8_t old = (uint8_t)(v - step);
    /* V from the step: ADC #1 (clear carry) going up, SBC #1 (set carry)
       going down. */
    int ov = step == 0x01 ? (cyc(24), (~(old ^ 0x01) & (old ^ v) & 0x80) != 0) : ((old ^ 0x01) & (old ^ v) & 0x80) != 0;

    *p = (uint8_t)((nz8(*p, a) & ~(P_C | P_V)) | P_M | (v >= stop ? P_C : 0) | (ov ? P_V : 0));
    return a;
}

/*
 * @routine 7E:DDF0 WindowSlideDown
 * WHAT IT DOES: Moves the window slide position $1A14 down a line; at $96
 * the slide ends.
 * HOW: window_slide with +1, stopping at $96. Long accesses except $0DA5
 * (data bank).
 * Entry: X 16-bit or 8-bit (unchanged); bank = DB, *p = P. Exit: M 8-bit;
 * the result is A's low byte = $0DA5 when the slide ends, else the new
 * position (A high byte $FF, *moved set); N, Z from it; C from comparing
 * the new position with $96; V from the add (all in *p).
 */
uint8_t WindowSlideDown(uint8_t bank, uint8_t *p, int *moved)
{
    return window_slide_flags(bank, 0x01, 0x96, p, moved);
}

void WindowSlideDown_cr(Carried *cr)
{
    uint8_t p = cr->p;
    int moved;
    uint8_t a = WindowSlideDown(cr->db, &p, &moved);

    cr->a = moved ? (uint16_t)(0xFF00 | a) : (uint16_t)((cr->a & 0xFF00) | a);
    set_p(cr, p);
}

void abi_WindowSlideDown(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    WindowSlideDown_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 7E:DE27 WindowSlideUp
 * WHAT IT DOES: Moves the window slide position $1A14 up a line; at $7F
 * the slide ends.
 * HOW: As WindowSlideDown, with -1, stopping at $7F.
 * Entry: as WindowSlideDown. Exit: as WindowSlideDown, comparing with $7F.
 */
uint8_t WindowSlideUp(uint8_t bank, uint8_t *p, int *moved)
{
    return window_slide_flags(bank, 0xFF, 0x7F, p, moved);
}

void WindowSlideUp_cr(Carried *cr)
{
    uint8_t p = cr->p;
    int moved;
    uint8_t a = WindowSlideUp(cr->db, &p, &moved);

    cr->a = moved ? (uint16_t)(0xFF00 | a) : (uint16_t)((cr->a & 0xFF00) | a);
    set_p(cr, p);
}

void abi_WindowSlideUp(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    WindowSlideUp_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 7E:DDD9 WindowSlideStep
 * WHAT IT DOES: Steps the window slide the way bits 1-2 of $0DA5 ask: 2
 * slides it down (WindowSlideDown), 4 or 6 up (WindowSlideUp), 0 not at
 * all.
 * HOW: The bits are tested through the data bank.
 * Entry: X 16-bit or 8-bit (unchanged); bank = DB, *p = P. Exit: M 8-bit;
 * as the slide it runs (the result, *moved, *p), or A low byte = 0 with Z
 * set.
 */
uint8_t WindowSlideStep(uint8_t bank, uint8_t *p, int *moved)
{
    uint8_t v;

    *moved = 0;
    *p |= P_M;
    cyc(92);
    v = (uint8_t)(rd8(0x7E0000 | W(slide_flags)) & 0x06);
    *p = nz8(*p, v);
    if (!v)
        return v;
    *p = (uint8_t)((nz8(*p, (uint8_t)(v - 0x02)) & ~P_C) | (v >= 0x02 ? P_C : 0));
    if (v == 0x02)
        return WindowSlideDown(bank, p, moved);
    cyc(6);
    return WindowSlideUp(bank, p, moved);
}

void WindowSlideStep_cr(Carried *cr)
{
    uint8_t p = cr->p;
    int moved;
    uint8_t a = WindowSlideStep(cr->db, &p, &moved);

    cr->a = moved ? (uint16_t)(0xFF00 | a) : (uint16_t)((cr->a & 0xFF00) | a);
    set_p(cr, p);
}

void abi_WindowSlideStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    WindowSlideStep_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
 * Entry: bank = DB; y = Y (its low byte, 0 or 2); *x = X; *p = P. Exit:
 * when nothing moves, A low byte = $19BC (the result, *keep_hi set: A's
 * high byte is the caller's), M 8-bit, X 8-bit, N, Z, C from comparing it
 * with $FF; at the -2 stop A low byte = 0, Z, C set, N clear, M and X
 * 8-bit; otherwise as the builder, with C from the limit compare and V from
 * the step add. *x is X's exit value, *p the flags.
 */
uint16_t WindowLayoutStep(uint8_t bank, uint8_t y8, uint16_t *x, uint8_t *p, int *keep_hi)
{
    cyc(54);
    uint8_t req = rd8(W(layout_request)), v;
    uint16_t y = y8, s, m, pos, lim, a;
    unsigned i;

    *keep_hi = 0;
    *x &= 0x00FF;
    *p |= P_M | P_X;
    if (req == 0x01 || (cyc(54), req == 0xFF)) {
        cyc(86);
        wr8(W(window_hdma_19bc), 0xFF);
        cyc(78);
        wr16(W(layout_step), req == 0x01 ? 0x0002 : 0xFFFE);
        cyc(56);
        wr16(W(layout_dir), req == 0x01 ? 0x0001 : 0x0000);
    } else {
        cyc(34);
        v = rd8(0x7E0000 | W(window_hdma_19bc));
        if (v != 0xFF) {
            cyc(74);
            *keep_hi = 1;
            *p = (uint8_t)(nz8(*p, (uint8_t)(v + 1)) & ~P_C);
            return v;
        } else { cyc(38); }
    }
    cyc(req == 0x01 ? 94 : 70);         /* the request 1 JMPs to the common part */
    wr8(W(layout_request), 0x00);
    cyc(54);
    s = rd16(W(window_hdma_select));
    cyc(46);
    m = rd16(W(layout_step));
    pos = (uint16_t)(s + m);
    *p = (uint8_t)(*p & ~P_V);
    if (~(s ^ m) & (s ^ pos) & 0x8000)
        *p |= P_V;
    if (pos == 0xFFFE) {
        cyc(110);
        wr8(W(window_hdma_19bc), 0x00);
        *p = (uint8_t)((*p & ~P_N) | P_Z | P_C);
        cyc(42);
        return (uint16_t)(0x0000 | (pos & 0xFF00));
    }
    cyc(114);
    lim = rd16(W(layout_limits) + y);
    *p = (uint8_t)((*p & ~P_C) | (pos >= lim ? P_C : 0));
    if (pos != lim)
        { cyc(48); wr16(W(window_hdma_select), pos); } else { cyc(22); }
    cyc(62);
    wr8(0x7E0000 | W(windows_setup_index), 0x00);
    cyc(16);
    for (i = 0; i < 6; i++)
        { cyc(32); wr8(W(window_setup_bytes) + i, 0xE7); }
    *x = y;
    if (y == 0)
        { cyc(76); a = (uint16_t)(WindowHdmaFromTable(x) & 0xFF00); }
    else
        { cyc(76); a = WindowHdmaLayout(bank, x); }
    *p = (uint8_t)((*p & ~(P_N | P_X)) | P_Z | P_M);
    cyc(42);
    return a;
}

void WindowLayoutStep_cr(Carried *cr)
{
    uint16_t x = cr->x;
    uint8_t p = cr->p, y = (uint8_t)cr->y;
    int keep;
    uint16_t a = WindowLayoutStep(cr->db, y, &x, &p, &keep);

    cr->a = keep ? (uint16_t)((cr->a & 0xFF00) | a) : a;
    cr->x = x;
    cr->y = y;
    set_p(cr, p);
}

void abi_WindowLayoutStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    WindowLayoutStep_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void abi_Records7FFromF170(void);
void abi_Records7FFromF634(void);

/* The course records builders, still in registers: they read the data bank
   (bank $09, where the lists are) and leave A, X, Y and P as they like, so
   the registers WindowsInit's caller has are saved and put back. */
static void records_7f(Carried *cr, int course_screen)
{
    uint8_t db = cr->db, p = cr->p;

    cr->db = 0x09;
    if (course_screen)
        w_Records7FFromF634(cr);
    else
        w_Records7FFromF170(cr);
    cr->db = db;
    set_p(cr, p);
}

/* Bytes `n` from ROM $09:src to `dst` (bank $7F), long stores. */
static void copy_rom09(uint16_t src, uint32_t dst, unsigned n)
{
    unsigned i;

    cyc(10);
    for (i = 0; i < n; i++)
        { cyc(124); wr8(dst + i, rd8(0x090000 | (uint16_t)(src + i))); }
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
 * Entry: any widths, D = $0000. Exit: A = $007F; X = 0; Y = 0 (the
 * adapter's: WindowEdgesFromList leaves it so); P and the data bank
 * unchanged.
 */
void WindowsInit(Carried *cr)
{
    const uint32_t b = 0x090000;
    uint16_t x, w, m, ex;
    unsigned i;
    uint8_t v, ep = 0;

    cyc(70);
    copy_rom09(0xF999, W(window_band_first), 4);
    copy_rom09(0xF99D, W(window_bands), 4);
    copy_rom09(0xF9A1, W(wram_7FFFD5), 4);
    copy_rom09(0xF9A5, W(wram_7FFFD9), 4);
    copy_rom09(0xF9A9, W(wram_7FFFDD), 4);
    cyc(92);
    v = rd8(0x7E0000 | W(wram_0D62));
    if (v)
        { cyc(68); records_7f(cr, 1); }
    else
        { cyc(62); records_7f(cr, 0); }
    cyc(42);
    copy_rom09(0xF163, W(window_hdma), 16);
    cyc(92);
    wr16(0x7E0000 | W(window_hdma_select), 0x0000);
    do {
        cyc(46);
        WindowEdgesFromList(0x09, &ex, &ep);
    } while (cyc_loop(46, rd16(cyc_then(46, 0x7E0000 | W(window_hdma_select))) != 0x000E));
    cyc(34);
    for (x = 0; (w = rd16(cyc_then(172, 0x09FB74 + x))) != 0xFFFF; x = (uint16_t)(x + 2))
        wr16(W(window_colour_hdma) + x, w);
    cyc(42);
    if (rd8(0x7E0000 | W(wram_0D62))) {
        for (x = 0; x < 0x0280; x++) {
            cyc(72);
            v = (uint8_t)(rd8(W(window_edges) + x) + 0xD8);
            cyc(62);
            wr8(W(window_edges) + x, v);
            cyc(62);
            wr8(W(window_edges_copy) + x, (uint8_t)(v + 0x50));
            cyc(20);
        }
        cyc(12);
    }
    cyc(92);
    wr16(0x7E0000 | W(window_hdma_select), 0xFFFE);
    cyc(48);
    wr16(0x7E0000 | W(layout_step), 0xFFFE);
    cyc(24);
    wr16(0x7E0000 | W(layout_dir), 0x0000);
    cyc(46);
    wr8(0x7E0000 | W(window_hdma_19bc), 0x00);
    cyc(40);
    wr8(0x7E0000 | W(wram_19BE), 0x00);
    cyc(24);
    wr8(0x7E0000 | W(windows_setup_index), 0x00);
    cyc(24);
    wr8(0x7E0000 | W(wram_1A07), 0x00);
    cyc(40);
    wr8(0x7E0000 | W(wram_1A0A), 0x00);
    cyc(24);
    wr8(0x7E0000 | W(wram_1A0B), 0x00);
    cyc(24);
    wr8(0x7E0000 | W(wram_1A17), 0x00);
    cyc(24);
    wr8(0x7E0000 | W(wram_1A18), 0x00);
    cyc(24);
    wr8(0x7E0000 | W(wram_19BF), 0x00);
    cyc(40);
    wr8(0x7E0000 | W(wram_19C0), 0xF9);
    cyc(40);
    wr8(0x7E0000 | W(wram_1A0C), 0x01);
    cyc(24);
    wr8(0x7E0000 | W(wram_1A0D), 0x01);
    cyc(40);
    wr8(0x7E0000 | W(wram_1A08), 0x40);
    cyc(24);
    wr8(0x7E0000 | W(wram_1A09), 0x40);
    cyc(40);
    wr8(0x7E0000 | W(wram_1A19), 0xFE);
    cyc(24);
    wr8(0x7E0000 | W(wram_1A1A), 0xFE);
    cyc(40);
    wr8(0x7E0000 | W(wram_1A15), 0xF0);
    cyc(24);
    wr8(0x7E0000 | W(wram_1A16), 0xF0);
    cyc(38);
    for (i = 0; i < 6; i++)
        { cyc(24); wr8(0x7E0000 | (uint16_t)(W(window_setup_bytes) + i), 0x00); }
    cyc(24);
    wr8(0x7E0000 | W(wram_1A0E), 0x00);
    cyc(24);
    wr8(0x7E0000 | W(wram_1A0F), 0x00);
    cyc(24);
    wr8(0x7E0000 | W(wram_1A1B), 0x00);
    cyc(62);
    wr8(0x7E0000 | W(wram_1A1C), 0x1A);
    cyc(24);
    wr8(0x7E0000 | W(wram_1A1D), 0x1A);
    cyc(24);
    wr8(0x7E0000 | W(wram_1A1E), 0x00);
    cyc(24);
    wr8(0x7E0000 | W(wram_1A1F), 0x00);
    cyc(24);
    wr8(0x7E0000 | W(wram_1A22), 0x00);
    cyc(24);
    wr8(0x7E0000 | W(wram_1A23), 0x00);
    cyc(40);
    wr8(0x7E0000 | W(wram_1A20), 0xFE);
    cyc(24);
    wr8(0x7E0000 | W(wram_1A21), 0xFE);
    cyc(46);
    m = rd16(0x7E0000 | W(hud_mode));
    if (m == 0x0000 || (cyc(34), m == 0x0003)) {
        cyc(46);
        x = (uint16_t)((rd16(0x7E0000 | W(wram_0D4A)) & 0x00FF) << 1);
        cyc(76);
        w = (uint16_t)((rd16(0x7E0000 | W(wram_0D48)) & 0x00FF) << 1);
        cyc(184);
        x = (uint16_t)(w + rd16(0x09F127 + x));
        wr16(0x7E0000 | W(slide_value), rd16(b | x));
    } else { cyc(28); }
    cyc(46);
    if (rd16(0x7E0000 | W(hud_mode)) == 0x0000) {
        cyc(40);
        x = (uint16_t)((rd16(0x7E0000 | W(wram_0D4A)) & 0x00FF) << 1);
        cyc(76);
        w = (uint16_t)(rd16(0x7E0000 | W(wram_0D48)) & 0x00FF);
        cyc(184);
        x = (uint16_t)(w + rd16(0x09F115 + x));
        wr8(0x7E0000 | W(wram_19E6), rd8(b | x));
    } else { cyc(22); }
    cyc(70);
    wr16(0x7E0000 | W(wram_19FE), 0x0000);
    cyc(46);
    x = (uint8_t)(rd16(0x7E0000 | W(wram_0D89)) << 1);
    cyc(52);
    wr8(0x7E0000 | W(wram_19D1), (uint8_t)x);
    cyc(72);
    wr16(0x7E0000 | W(wram_19CB), rd16(0x09F14D + x));
    cyc(24);
    wr16(0x7E0000 | W(wram_19D3), 0x0000);
    cyc(70);
    wr16(0x7E0000 | W(wram_19CF), 0x0017);
    cyc(48);
    wr16(0x7E0000 | W(wram_19C5), 0x0020);
    cyc(48);
    wr16(0x7E0000 | W(wram_19C3), 0xE020);
    cyc(48);
    wr16(0x7E0000 | W(wram_19F3), 0xE600);
    cyc(46);
    wr8(0x7E0000 | W(wram_19F5), 0x00);
    cyc(62);
    wr8(0x7E0000 | W(window_slide_pos), 0x80);
    cyc(40);
    wr8(0x7E0000 | W(window_hdma_off), 0x00);
    cyc(40);
    wr8(0x7E0000 | W(wram_19E0), 0xFE);
    cyc(40);
    wr8(0x7E0000 | W(wram_19DF), 0x00);
    cyc(70);
    wr16(0x7E0000 | W(wram_19DB), 0x0000);
    cyc(24);
    wr16(0x7E0000 | W(wram_19DD), 0x0000);
    cyc(62);
    wr8(0x7E0000 | W(wram_1A04), 0xFF);
    {
        cyc(38);
        static const uint16_t zero[] = {0x19EC, 0x19ED, 0x19E8, 0x19E9, 0x19EA, 0x19EB, 0x19F2,
                                        0x19EE, 0x19EF, 0x19F0, 0x19F1, 0x1A06, 0x1A29};

        for (i = 0; i < sizeof zero / sizeof zero[0]; i++)
            { cyc(24); wr8(b | zero[i], 0x00); }
    }
    cyc(84);
    dma_wr(b | DMAP4, 0x44);
    cyc(40);
    dma_wr(b | BBAD4, 0x26);
    cyc(48);
    dma_wr16(b | A1T4L, 0xF85B);
    cyc(40);
    dma_wr(b | A1B4, 0x7F);
    cyc(48);
    dma_wr16(b | DAS4L, 0x0000);
    cyc(40);
    dma_wr(b | DASB4, 0x7F);
    cyc(40);
    dma_wr(b | DMAP5, 0x00);
    cyc(40);
    dma_wr(b | BBAD5, 0x32);
    cyc(48);
    dma_wr16(b | A1T5L, 0xFF8B);
    cyc(40);
    dma_wr(b | A1B5, 0x7F);
    cyc(48);
    dma_wr16(b | DAS5L, 0x0000);
    cyc(40);
    dma_wr(b | DASB5, 0x7F);
    cyc(100);
}

void WindowsInit_cr(Carried *cr)
{
    WindowsInit(cr);
    cr->a = 0x007F;
    cr->x = 0x0000;
    cr->y = 0x0000;
}

void abi_WindowsInit(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    WindowsInit_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* The registers a routine leaves for callers that still work in them: the
   caller's A, X and Y in, the routine's out. */

/* SEP #$30's effect on the index registers: 8-bit X and Y clear their high
   bytes. */
static void index8(WindowRegs *r)
{
    cyc(22);
    r->x &= 0x00FF;
    r->y &= 0x00FF;
}

/* The PLP that ends a routine which saved P: p0's 8-bit index flag clears
   the high bytes. */
static void restore_p(WindowRegs *r, uint8_t p0)
{
    cyc(42);
    if (p0 & P_X) {
        r->x &= 0x00FF;
        r->y &= 0x00FF;
    }
}

/* CMP (16-bit) of v with m: N, Z and C. */
static void cmp16(uint16_t v, uint16_t m, uint8_t *p)
{
    cyc(0);
    *p = (uint8_t)((nz16(*p, (uint16_t)(v - m)) & ~P_C) | (v >= m ? P_C : 0));
}

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
 * Entry: any widths; r = A, X, Y; p0 = P. Exit: P and the data bank
 * restored; X and Y high bytes cleared; the result is A, X, Y as the last
 * step left them.
 */
WindowRegs WindowBandsAnimate(WindowRegs r, uint8_t p0)
{
    uint8_t v, pp = p0;
    uint16_t a, m;

    index8(&r);
    cyc(126);
    v = rd8(W(wram_19DF));
    r.a = (uint16_t)((r.a & 0xFF00) | v);
    if (v)
        { cyc(22); goto out; }
    cyc(40);
    v = rd8(0x7E0000 | W(window_hdma_off));
    r.a = (uint16_t)((r.a & 0xFF00) | v);
    if (!v)
        { cyc(16); goto out; }
    cyc(68);
    v = rd8(0x7E0000 | W(window_hdma_off));
    r.a = (uint16_t)((r.a & 0xFF00) | v);
    if (v == 0x01 || (cyc(38), v == 0x02)) {
        uint8_t fast;

        if (v == 0x01) {
            cyc(172);
            wr16(0x7E0000 | W(window_band_step), 0x0080);
            cyc(56);
            wr16(W(window_band_pos), 0x0000);
        } else
            { cyc(166); wr16(0x7E0000 | W(window_band_step), 0xFF80); }
        cyc(46);
        fast = rd8(0x7E0000 | W(wram_0D62));
        if (v == 0x01)
            { cyc(100); wr16(W(window_band_target), fast ? (cyc(16), 0x0780) : 0x0C80); }
        else {
            cyc(100);
            wr16(W(window_band_pos), fast ? (cyc(16), 0x0700) : 0x0C00);
            cyc(56);
            wr16(W(window_band_target), 0x0000);
        }
        cyc(v == 0x01 ? 102 : 78);      /* JMP for the open side, REP, LDA # */
        wr16(W(wram_0DA4), 0x0000);
        index8(&r);
        cyc(40);
        wr8(0x7E0000 | W(window_hdma_off), 0x04);
    } else if (v != 0x04 && (cyc(32), v != 0x05)) {
        cyc(v == 0x03 ? 96 : 90);       /* the compares to CMP #3, BEQ */
        goto out;
    }
    cyc(54);
    v = (uint8_t)(rd8(W(wram_0D45)) & 0xDF);
    cyc(48);
    wr8(W(wram_0D45), v);
    cyc(40);
    wr8(0x7E0000 | W(windows_setup_index), 0x08);
    cyc(24);
    v = rd8(0x7E0000 | W(wram_19E0));
    {
        unsigned i;

        for (i = 0; i < 6; i++)
            { cyc(24); wr8(0x7E0000 | (uint16_t)(W(window_setup_bytes) + i), v); }
    }
    cyc(54);
    a = rd16(W(window_band_pos));
    pp &= (uint8_t)~P_C;
    cyc(38);
    a = alu_adc16(a, rd16(0x7E0000 | W(window_band_step)), &pp);
    r.a = a;
    cyc(24);
    m = rd16(0x7E0000 | W(window_band_target));
    cyc(0);
    if (a != m)
        { cyc(48); wr16(W(window_band_pos), a); cyc(24); /* JMP */ }
    else {
        cyc(76);
        v = rd8(W(wram_19D5));
        if (!v) {
            r.a = (uint16_t)((r.a & 0xFF00) | v);
            cyc(40);
            wr8(0x7E0000 | W(window_hdma_off), 0x00);
            goto out;
        }
        cyc(62);
        wr8(0x7E0000 | W(window_hdma_off), 0x05);
    }
    index8(&r);
    cyc(32);
    v = rd8(W(wram_19D5));
    r.x = (uint8_t)(v << 1);
    cyc(52);
    v = rd8(0x7E0000 | W(wram_0D62));
    r.a = (uint16_t)((r.a & 0xFF00) | v);
    if (v)
        { cyc(62); r.a = WindowHdmaPreset(0x7E, (uint8_t)r.x, &r.x, &r.y); }
    else {
        WindowListEnd end;

        cyc(68);
        end = WindowHdmaFromList(0x7E, (uint8_t)r.x);
        r.a = 0xF800;
        r.x = end.x;
        r.y = end.y;
    }
    cyc(64);
    index8(&r);
out:
    cyc(58);
    restore_p(&r, p0);
    return r;
}

void WindowBandsAnimate_cr(Carried *cr)
{
    WindowRegs r = {cr->a, cr->x, cr->y};

    r = WindowBandsAnimate(r, cr->p);
    cr->a = r.a;
    cr->x = r.x;
    cr->y = r.y;
}

void abi_WindowBandsAnimate(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    WindowBandsAnimate_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* Three bands of 16, 48 and 16 lines at entry offset `at`, all at the
   window address `addr`. */
static void window_bands_3(uint16_t at, uint16_t addr)
{
    cyc(70);
    wr8(W(window_hdma) + at, 0x10);
    cyc(48);
    wr8(W(window_hdma) + at + 3, 0x30);
    cyc(48);
    wr8(W(window_hdma) + at + 6, 0x10);
    cyc(78);
    wr16(W(window_hdma) + at + 1, addr);
    cyc(32);
    wr16(W(window_hdma) + at + 4, addr);
    cyc(32);
    wr16(W(window_hdma) + at + 7, addr);
    cyc(42); /* RTS */
}

/*
 * @routine 7E:DCC6 WindowHdmaFFD5
 * WHAT IT DOES: Sets the window table's second to fourth bands to 16, 48
 * and 16 lines at $FFD5.
 * HOW: Counts, then addresses. Reached by a unit test
 * (unit_uncalled).
 * Entry: any widths. Exit: as WindowHdmaFFD1, A = $FFD5 (the result).
 */
uint16_t WindowHdmaFFD5(void)
{
    window_bands_3(3, 0xFFD5);
    return 0xFFD5;
}

void WindowHdmaFFD5_cr(Carried *cr)
{
    cr->a = WindowHdmaFFD5();
    set_p(cr, (uint8_t)((cr->p & ~(P_Z | P_M)) | P_N));
}

void abi_WindowHdmaFFD5(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    WindowHdmaFFD5_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 7E:DCEC WindowHdmaFFD9
 * WHAT IT DOES: As WindowHdmaFFD5 for the sixth to eighth bands, at $FFD9.
 * HOW: As WindowHdmaFFD5. Reached by a unit test
 * (unit_uncalled).
 * Entry: any widths. Exit: as WindowHdmaFFD1, A = $FFD9 (the result).
 */
uint16_t WindowHdmaFFD9(void)
{
    window_bands_3(15, 0xFFD9);
    return 0xFFD9;
}

void WindowHdmaFFD9_cr(Carried *cr)
{
    cr->a = WindowHdmaFFD9();
    set_p(cr, (uint8_t)((cr->p & ~(P_Z | P_M)) | P_N));
}

void abi_WindowHdmaFFD9(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    WindowHdmaFFD9_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

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
 * Entry: any widths; r = A, X, Y; p0 = P. Exit: P and the data bank
 * restored; X and Y high bytes cleared; the result is A as the last step
 * left it.
 */
WindowRegs WindowSplitAnimate(WindowRegs r, uint8_t p0)
{
    uint8_t v, h, pp = p0, pj = 0;
    uint16_t a, m;
    unsigned i;

    index8(&r);
    cyc(126);
    v = rd8(W(wram_19DF));
    r.a = (uint16_t)((r.a & 0xFF00) | v);
    if (v != 0x01)
        { cyc(16); goto out; }
    cyc(56);
    v = rd8(0x7E0000 | W(window_hdma_off));
    r.a = (uint16_t)((r.a & 0xFF00) | v);
    if (!v)
        { cyc(16); goto out_fall; }          /* BNE not taken */
    cyc(68);
    v = rd8(0x7E0000 | W(window_hdma_off));
    r.a = (uint16_t)((r.a & 0xFF00) | v);
    if (v == 0x01 || (cyc(38), v == 0x02)) {
        if (v == 0x01) {
            cyc(180);
            wr16(W(wram_19DD), 0x0400);
            cyc(56);
            wr16(W(wram_19DB), 0x0000);
            cyc(56);
            wr16(W(window_band_target), 0x7400);
            cyc(48);
            wr16(0x7E0000 | W(window_19e2), 0xF50B);
            cyc(48);
            wr16(0x7E0000 | W(window_19e4), 0x0000);
        } else {
            cyc(174);
            wr16(W(wram_19DD), 0xFC00);
            cyc(56);
            wr16(W(wram_19DB), 0x8000);
            cyc(56);
            wr16(W(window_band_target), 0x0800);
            cyc(48);
            wr16(0x7E0000 | W(window_19e4), 0x0BF5);
            cyc(48);
            wr16(0x7E0000 | W(window_19e2), 0x0000);
        }
        cyc(v == 0x01 ? 102 : 78);     /* JMP for the open side, REP, LDA # */
        wr16(W(wram_0DA4), 0x0000);
        index8(&r);
        cyc(48);
        wr8(W(window_hdma_off), 0x04);
    } else if (v != 0x04 && (cyc(32), v != 0x05)) {
        cyc(v == 0x03 ? 96 : 68);       /* the compares to CMP #3, BEQ; SEP at state 3 */
        goto out;
    }
    cyc(54);
    v = (uint8_t)(rd8(W(wram_0D45)) & 0xDF);
    cyc(48);
    wr8(W(wram_0D45), v);
    cyc(40);
    wr8(0x7E0000 | W(windows_setup_index), 0x08);
    cyc(24);
    v = rd8(0x7E0000 | W(wram_19E0));
    for (i = 0; i < 6; i++)
        { cyc(24); wr8(0x7E0000 | (uint16_t)(W(window_setup_bytes) + i), v); }
    pp &= (uint8_t)~P_C;
    cyc(54);
    a = rd16(W(wram_19DB));
    cyc(46);
    a = alu_adc16(a, rd16(W(wram_19DD)), &pp);
    r.a = a;
    cyc(24);
    m = rd16(0x7E0000 | W(window_band_target));
    cyc(0);
    if (a != m) {
        cyc(48);
        wr16(W(wram_19DB), a);
        cyc(84);
        h = rd8(W(window_lines));
        pp |= P_C;
        v = alu_sbc8(0xFF, h, &pp);
        cyc(30);
        h = rd8(W(window_lines));
        pp |= P_C;
        v = alu_sbc8(v, h, &pp);
        v = (uint8_t)(v >> 1);
        (cyc(44), WindowHdmaAt(v, &pj));
        r.a = 0xFFDD;
        goto out;
    }
    cyc(76);
    a = rd16(W(wram_19DB));
    r.a = a;
    cmp16(a, 0x7000, &pp);
    if (a >= 0x7000) {
        cyc(86);
        WindowHdmaFFDD();
        cyc(62);
        wr8(0x7E0000 | W(window_hdma_off), 0x05);
        r.a = 0xFF05;
    } else {
        cyc(92);
        wr8(0x7E0000 | W(window_hdma_off), 0x00);
        cyc(58);                        /* falls into the pulls: no BRA */
        restore_p(&r, p0);
        return r;
    }
out:
    cyc(22);                            /* a taken branch */
out_fall:
    cyc(58);                            /* PLB, PLP, RTL with op_plp */
    restore_p(&r, p0);
    return r;
}

void WindowSplitAnimate_cr(Carried *cr)
{
    WindowRegs r = {cr->a, cr->x, cr->y};

    r = WindowSplitAnimate(r, cr->p);
    cr->a = r.a;
    cr->x = r.x;
    cr->y = r.y;
}

void abi_WindowSplitAnimate(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    WindowSplitAnimate_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:F567 WindowHdmaChannelsSetup
 * WHAT IT DOES: Sets up HDMA channels 3-7 from the tables in bank $7F:
 * 7 to $210F (BG2 x, two writes) from $12D5, 6 to $2110 indirect from
 * $12E2 (bank 0 data), 5 to $212C from $12F2, 4 and 3 to $2127 from
 * $12FD and $1308; the window lines $7F:130D and $7F:130F cleared.
 * HOW: 8-bit register writes, the addresses 16-bit; through the data bank;
 * P saved and restored.
 * Entry: any widths; bank = DB. Exit: P restored; A = 0 (low byte); X =
 * $1308 (the adapter's: its low byte when X is 8-bit).
 */
void WindowHdmaChannelsSetup(uint8_t bank)
{
    static const struct { uint8_t dmap, bbad; uint16_t a1t; } ch[5] = {
        {0x02, 0x0F, 0x12D5}, {0x42, 0x10, 0x12E2}, {0x00, 0x2C, 0x12F2}, {0x00, 0x27, 0x12FD}, {0x00, 0x27, 0x1308}};
    uint32_t b = (uint32_t)bank << 16;
    int i;

    cyc(22);                            /* REP #$10 */
    cyc(90);
    for (i = 0; i < 5; i++) {
        uint16_t base = (uint16_t)(0x4370 - 0x10 * i);

        cyc(40);
        dma_wr(b | base, ch[i].dmap);
        cyc(40);
        dma_wr(b | (uint16_t)(base + 1), ch[i].bbad);
        cyc(48);
        dma_wr16(b | (uint16_t)(base + 2), ch[i].a1t);
        cyc(40);
        dma_wr(b | (uint16_t)(base + 4), 0x7F);
        if (i == 1)
            { cyc(40); dma_wr(b | (uint16_t)(base + 7), 0x00); }
    }
    cyc(48);
    wr8(W(wram_7F130D), 0x00);
    cyc(32);
    wr8(W(wram_7F130F), 0x00);
    cyc(42);                            /* PLP */
    cyc(28);
}

void WindowHdmaChannelsSetup_cr(Carried *cr)
{
    WindowHdmaChannelsSetup(cr->db);
    cr->a = (uint16_t)(cr->a & 0xFF00);
    cr->x = 0x1308;
    if (cr->p & P_X) {
        cr->x &= 0x00FF;
        cr->y &= 0x00FF;
    }
}

void abi_WindowHdmaChannelsSetup(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    WindowHdmaChannelsSetup_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* CMP (8-bit) of v with m: N, Z and C. */
static void cmp8(uint8_t v, uint8_t m, uint8_t *p)
{
    *p = (uint8_t)((nz8(*p, (uint8_t)(v - m)) & ~P_C) | (v >= m ? (cyc(16), P_C) : 0));
}

/* LDA, CLC, ADC (8-bit) of two bytes, then CMP with the limit pair at
   `limits`: the sum is stored back at `at` unless it equals the first limit
   or is below the second. The new A (its low byte the sum). */
static uint16_t band_step(uint16_t a, uint32_t at, uint32_t step, uint32_t limits, uint8_t *p)
{
    cyc(32);
    uint8_t v = rd8(at);

    *p &= (uint8_t)~P_C;
    cyc(38);
    v = alu_adc8(v, rd8(step), p);
    a = (uint16_t)((a & 0xFF00) | v);
    cyc(32);
    cmp8(v, rd8(limits), p);
    if (*p & P_Z)
        { cyc(6); return a; }
    cyc(48);
    cmp8(v, rd8(limits + 1), p);
    if (!(*p & P_C))
        { cyc(22); return a; }
    cyc(32);
    wr8(at, v);
    return a;
}

/*
 * @routine 7E:D4F6 ColdataTopBandStep
 * WHAT IT DOES: Moves the line count of the COLDATA table's first band
 * ($19CB) by $19C7, unless the new count equals the first byte of the
 * limit pair $19D1 in the list at $7E:D5AA or is below the second.
 * HOW: 8-bit; the pair's offset is $19D1.
 * Entry: any widths; bank = DB ($7E); r = A, X, Y; *p = P. Exit: M and X
 * 8-bit; the result is A (low byte the new count), X = $19D1, Y with its
 * high byte clear; N, Z, C from the last compare (in *p).
 */
WindowRegs ColdataTopBandStep(WindowRegs r, uint8_t bank, uint8_t *p)
{
    (void)bank;

    *p |= P_M | P_X;
    index8(&r);
    cyc(70);
    r.x = rd8(0x7E0000 | W(wram_19D1));
    r.a = band_step(r.a, W(wram_19CB), 0x7E0000 | W(wram_19C7), W(wram_7ED5AA) + r.x, p);
    return r;
}

void ColdataTopBandStep_cr(Carried *cr)
{
    WindowRegs r = {cr->a, cr->x, cr->y};
    uint8_t p = cr->p;

    r = ColdataTopBandStep(r, cr->db, &p);
    cr->a = r.a;
    cr->x = r.x;
    cr->y = r.y;
    set_p(cr, p);
}

void abi_ColdataTopBandStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ColdataTopBandStep_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
 * Entry: any widths; bank = DB ($7E), dp = D; *p = P. Exit: M and X 8-bit;
 * the result is A (low byte the new count; high byte $40 with the bit
 * set), X the pair's offset, Y the second link (high byte clear); N, Z, C
 * from the last compare (in *p).
 */
WindowRegs ColdataLowBandStep(uint8_t bank, uint16_t dp, uint8_t *p)
{
    uint32_t b = (uint32_t)bank << 16;
    WindowRegs r;
    uint8_t v;
    uint32_t tail = 46;

    cyc(22);                            /* REP #$30 */
    *p &= (uint8_t)~(P_M | P_X);
    cyc(112);
    r.x = rd16(0x7E0000 | W(wram_0346));
    cyc(30);
    r.y = rd16(b + 0x204F + r.x);
    cyc(30);
    r.a = rd16(b + 0x204F + r.y);
    r.y = r.a;
    cyc(44);
    r.a = (uint16_t)(rd16(b + r.y) & 0x4000);
    *p = nz16(*p, r.a);
    *p |= P_M | P_X;
    index8(&r);
    cyc(r.a ? 64 : 70);                 /* BEQ taken or not */
    v = rd8(0x7E0000 | W(wram_19D3));
    if (r.a) {
        if (!v) {
            cyc(56);
            wr8(0x7E0000 | W(wram_19D3), 0x01);
            cyc(48);
            wr8(W(wram_19CF), 0x40);
            v = 0x40;
        } else { cyc(22); }
        cyc(16);
        r.x = rd8((uint16_t)(dp + 0x02));
    } else {
        if (v) {
            cyc(56);
            wr8(0x7E0000 | W(wram_19D3), 0x00);
            cyc(48);
            wr8(W(wram_19CF), 0x20);
            v = 0x20;
            tail = 38;                  /* LDX #imm, SEP: no BEQ before */
        } else { cyc(14); }
        r.x = 0x00;
    }
    cyc(tail);
    r.a = (uint16_t)((r.a & 0xFF00) | v);
    r.a = band_step(r.a, W(wram_19CF), 0x7E0000 | W(wram_19CD), W(wram_7ED56E) + r.x, p);
    return r;
}

void ColdataLowBandStep_cr(Carried *cr)
{
    uint8_t p = cr->p;
    WindowRegs r = ColdataLowBandStep(cr->db, cr->d, &p);

    cr->a = r.a;
    cr->x = r.x;
    cr->y = r.y;
    set_p(cr, p);
}

void abi_ColdataLowBandStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ColdataLowBandStep_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
 * Entry: any widths; bank = DB ($7E, for $19C5); r = X, Y (A is not read);
 * *p = P. Exit: at a limit, M and X 16-bit and A the sum, X and Y
 * unchanged; otherwise M and X 8-bit, A low byte $FF (high byte the new
 * whole part), X at the list's end and Y with its high byte clear; Z and C
 * set, V from the add (in *p).
 */
WindowRegs ColdataLevelStep(WindowRegs r, uint8_t bank, uint8_t *p)
{
    (void)bank;
    uint8_t x, v;

    cyc(22);                            /* REP #$30 */
    *p &= (uint8_t)~(P_M | P_X);
    cyc(78);
    r.a = rd16(W(wram_19C3));
    *p &= (uint8_t)~P_C;
    cyc(38);
    r.a = alu_adc16(r.a, rd16(0x7E0000 | W(wram_19C5)), p);
    cmp16(r.a, 0xE000, p);
    if (*p & P_Z)
        { cyc(8); return r; }
    cmp16(r.a, 0xEF00, p);
    if (*p & P_Z)
        { cyc(48); return r; }
    cyc(112);
    wr16(W(wram_19C3), r.a);
    *p |= P_M | P_X;
    index8(&r);
    for (x = 0;; x = (uint8_t)(x + 2)) {
        cyc(48);
        v = rd8(W(wram_7ED5B3) + x);
        if (v == 0xFF)
            break;
        cyc(64);
        cmp8(v, rd8(W(wram_19C4)), p);
        if (*p & P_C)
            { cyc(32); wr8(W(wram_7FFF8E) + x, cyc_then(32, rd8(W(wram_19C4)))); } else { cyc(22); }
        cyc(36);
    }
    r.x = x;
    r.a = (uint16_t)((r.a & 0xFF00) | 0xFF);
    *p = (uint8_t)((*p & ~P_N) | P_Z | P_C);
    return r;
}

void ColdataLevelStep_cr(Carried *cr)
{
    WindowRegs r = {cr->a, cr->x, cr->y};
    uint8_t p = cr->p;

    r = ColdataLevelStep(r, cr->db, &p);
    cr->a = r.a;
    cr->x = r.x;
    cr->y = r.y;
    set_p(cr, p);
}

void abi_ColdataLevelStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ColdataLevelStep_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
 * Entry: any widths; r = A, X, Y; p0 = P; dp = D. Exit: P and the data
 * bank restored; the result is A, X, Y as the last step left them (A low
 * byte $0D45 after the slide's steps, 0 when no slide).
 */
static WindowRegs window_slide_check_step(WindowRegs r, uint8_t p0, uint16_t dp)
{
    const uint32_t b = 0x7E0000;
    uint8_t v, pp = p0;
    uint16_t m;
    int called, same;

    r.a = (uint16_t)((r.a & 0xFF00) | 0xE0);
    cyc(108);
    wr8(W(window_setup_bytes), 0xE0);
    cyc(48);
    wr8(W(wram_19AD), 0xE0);
    cyc(48);
    wr8(W(wram_19AE), 0xE0);
    cyc(32);
    v = rd8(W(wram_19C4));
    r.a = (uint16_t)((r.a & 0xFF00) | v);
    cmp8(v, 0xE1, &pp);
    called = pp & P_C;
    if (called) {
        cyc(62);                        /* CMP, JSR */
        WindowHdmaFFD1();
        r.a = 0xFFD1;
    }
    r.a = (uint16_t)((r.a & 0xFF00) | 0x06);
    cyc(called ? 62 : 100);
    wr8(0x7E0000 | W(windows_setup_index), 0x06);
    r = ColdataLevelStep(r, 0x7E, &pp);
    cyc(22);                            /* REP #$20 */
    cyc(112);
    r.a = rd16(W(wram_19C3));
    pp &= (uint8_t)~P_C;
    cyc(38);
    r.a = alu_adc16(r.a, rd16(0x7E0000 | W(wram_19C5)), &pp);
    if (r.a == 0xE000) {
        index8(&r);
        cyc(78);
        v = (uint8_t)(rd8(W(slide_flags)) & 0xFE);
        cyc(48);
        wr8(W(slide_flags), v);
        cyc(32);
        v = (uint8_t)(rd8(W(wram_0D45)) & 0xDF);
        cyc(48);
        wr8(W(wram_0D45), v);
        r.a = (uint16_t)((r.a & 0xFF00) | v);
        return r;
    }
    cyc(22);                            /* REP #$30 */
    cyc(88);
    r.x = rd16(0x7E0000 | W(wram_0346));
    cyc(32);
    r.a = rd16(W(wram_19C9));
    cyc(30);
    m = rd16(b + 0x200E + r.x);
    same = r.a == m;
    if (!same) {
        int down = r.a >= m;

        cyc(102);
        wr8(W(wram_19C7), down ? (cyc(6), 0xFE) : 0x02);
        cyc(48);
        wr8(W(wram_19CD), down ? 0x01 : 0xFF);
        r.a = (uint16_t)((r.a & 0xFF00) | (down ? 0x01 : (cyc(24), 0xFF)));
        r = ColdataTopBandStep(r, 0x7E, &pp);
        r = ColdataLowBandStep(0x7E, dp, &pp);
    }
    cyc(same ? 76 : 96);
    v = rd8(W(wram_19CB));
    cyc(32);
    wr8(W(window_colour_hdma), v);
    cyc(32);
    v = rd8(W(wram_19CF));
    cyc(32);
    wr8(W(wram_7FFFAB), v);
    r.a = (uint16_t)((r.a & 0xFF00) | v);
    cyc(22);                            /* REP #$30 */
    cyc(24);
    r.x = rd16(0x7E0000 | W(wram_0346));
    cyc(30);
    r.a = rd16(b + 0x200E + r.x);
    cyc(32);
    wr16(W(wram_19C9), r.a);
    cyc(54);
    v = (uint8_t)(rd8(W(wram_0D45)) | 0x20);
    cyc(48);
    wr8(W(wram_0D45), v);
    r.a = (uint16_t)((r.a & 0xFF00) | v);
    return r;
}

WindowRegs WindowSlideCheck(WindowRegs r, uint8_t p0, uint16_t dp)
{
    uint8_t v;

    cyc(140);
    v = rd8(0x7E0000 | W(window_hdma_off));
    r.a = (uint16_t)((r.a & 0xFF00) | v);
    if (!v) {
        cyc(48);
        v = rd8(W(window_hdma_19bc));
        r.a = (uint16_t)((r.a & 0xFF00) | v);
        if (v != 0xFF) {
            cyc(56);
            v = rd8(0x7E0000 | W(slide_flags));
            r.a = (uint16_t)((r.a & 0xFF00) | v);
            if (v & 0x01)
                r = window_slide_check_step(r, p0, dp);
            else
                { cyc(32); r.a = (uint16_t)(r.a & 0xFF00); }
        } else { cyc(38); }
    } else { cyc(22); }
    cyc(58);
    restore_p(&r, p0);
    return r;
}

void WindowSlideCheck_cr(Carried *cr)
{
    WindowRegs r = {cr->a, cr->x, cr->y};

    r = WindowSlideCheck(r, cr->p, cr->d);
    cr->a = r.a;
    cr->x = r.x;
    cr->y = r.y;
}

void abi_WindowSlideCheck(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    WindowSlideCheck_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* Records7FFromF170 as its adapter leaves the registers. */
static void w_Records7FFromF170(Carried *cr)
{
    cr->y = Records7FFromF170(cr->db);
    cr->a = 0x0001;
    cr->x = 0x0009;
    set_p(cr, (uint8_t)((cr->p & ~(P_N | P_Z | P_M)) | P_X));
}

/* Records7FFromF634 as its adapter leaves the registers. */
static void w_Records7FFromF634(Carried *cr)
{
    uint8_t p = cr->p;

    Records7FFromF634(cr->db, &cr->x, &cr->y, &p);
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0xFF);
    set_p(cr, (uint8_t)((p & ~P_X) | P_M));
}
