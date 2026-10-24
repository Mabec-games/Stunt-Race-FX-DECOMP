/*
 * gradient.c: the colour gradient drawn by HDMA. Channel 2 writes CGADD
 * once a line from the table at $7F:3497 (a line count and a colour number
 * per entry); channel 3 writes CGDATA, indirect, from the table at
 * $7F:3489, whose entries are a line count and the address in bank $7F of
 * the colours for those lines. Moving the addresses at $7F:348D and
 * $7F:3493 (copied from $0D96 and $0D98) scrolls the gradient.
 */
#include "machine.h"
#include "native.h"
#include "hw.h"
#include "video_calls.h"
#include "carried.h"
#include "wram_names.h"

/* The run expansion GradientExpandFirst and GradientExpandSecond share,
   from run table index x: each run byte n stores the colour word at the
   data bank's Y + $20 n times at $7F:EF57 + $F3, counting n down in $F1.
   Returns the ending index; *y is left past the last colour read. */
static uint16_t expand_runs(uint8_t bank, uint16_t x, uint16_t *y)
{
    for (;;) {
        cyc(198);
        uint16_t n = (uint16_t)(rd16(0x1FB44E + x) & 0x00FF), colour, out;

        if (!n)
            { cyc(32); return x; }
        wr16(0x0000F1, n);
        cyc(92);
        colour = rd16(((uint32_t)bank << 16) + 0x0020 + *y);
        out = rd16(0x0000F3);
        do {
            cyc(32);
            wr16((W(gradient_colours) + out) & 0xFFFFFF, colour);
            out = (uint16_t)(out + 2);
            cyc(44);
            n = (uint16_t)(rd16(0x0000F1) - 1);
            cyc(14);
            wr16(0x0000F1, n);
        } while (cyc_loop(22, n));
        cyc(32);
        wr16(0x0000F3, out);
        x = (uint16_t)(x + 1);
        *y = (uint16_t)(*y + 2);
        cyc(6);
    }
}

/* The course test both expansions start with: course $0A (first list) or
   1 (second list) uses the run table from `special`, any other from 0. */
static uint16_t expand_list(uint8_t bank, uint16_t *y, uint16_t course, uint16_t special, int *carry)
{
    cyc(92);
    uint16_t c = (uint16_t)(rd16((uint32_t)bank << 16 | 0x0E3D) & 0x00FF);

    *carry = c >= course;
    return expand_runs(bank, c == course ? (cyc(16), special) : 0, y);
}

/*
 * @routine 03:ED34 GradientExpandFirst
 * WHAT IT DOES: Expands run-length colours into the list at $7F:EF57 + $F3:
 * each run byte n repeats the next colour word n times.
 * HOW: Run lengths from $1F:B44E (from +$28 for course $0A); colours from
 * the data bank at Y + $20; $F1 counts each run down. Shares its loop with
 * GradientExpandSecond. The course $0A case is not reached in the traces so
 * far.
 * Entry: X 16-bit, D = $0000; DB = the colours' bank; Y = their block;
 * $F3 = the write offset. Exit: A = 0; X = the ending run's index; Y past
 * the last colour; $F3 past the last write; M 16-bit; Z set, N clear; C
 * from the course compare.
 */
uint16_t GradientExpandFirst(uint8_t bank, uint16_t *y, int *carry)
{
    return expand_list(bank, y, 0x000A, 0x0028, carry);
}

/* GradientExpandFirst on carried registers. */
void GradientExpandFirst_cr(Carried *cr)
{
    uint16_t y = cr->y;
    int carry;

    cr->x = GradientExpandFirst(cr->db, &y, &carry);
    cr->y = y;
    cr->a = 0;
    set_p(cr, (uint8_t)((nz16(cr->p, 0) & ~(P_M | P_C)) | (carry ? P_C : 0)));
}

void abi_GradientExpandFirst(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    GradientExpandFirst_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:ED46 GradientExpandSecond
 * WHAT IT DOES: As GradientExpandFirst, for the second list: run lengths
 * from +$50 for course 1, from +0 otherwise.
 * HOW: As GradientExpandFirst. The course 1 case is not reached in the
 * traces so far.
 * Entry: as GradientExpandFirst. Exit: as GradientExpandFirst.
 */
uint16_t GradientExpandSecond(uint8_t bank, uint16_t *y, int *carry)
{
    return expand_list(bank, y, 0x0001, 0x0050, carry);
}

/* GradientExpandSecond on carried registers. */
void GradientExpandSecond_cr(Carried *cr)
{
    uint16_t y = cr->y;
    int carry;

    cr->x = GradientExpandSecond(cr->db, &y, &carry);
    cr->y = y;
    cr->a = 0;
    set_p(cr, (uint8_t)((nz16(cr->p, 0) & ~(P_M | P_C)) | (carry ? P_C : 0)));
}

void abi_GradientExpandSecond(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    GradientExpandSecond_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:ECF5 GradientBuildColours
 * WHAT IT DOES: Builds the gradient's colour lists in bank $7F for the
 * course: the first list at $7F:EF57 from the course block's colours, the
 * second at $7F:F357 from the course's second colour block (if it has one),
 * then clears the $100 bytes at $7F:F757.
 * HOW: X indexes the course block pointers ($03:F6F1: address word, bank
 * byte); with the block's bank as the data bank, GradientExpandFirst writes
 * from offset 0, GradientExpandSecond from offset $400.
 * Entry: M 8-bit, X 16-bit, D = $0000; X = course * 3. Exit: A = 0; X =
 * $FFFE; Y past the last colour read; M 8-bit; N, Z from the restored data
 * bank; DB unchanged; C from the last expansion's course compare; V
 * unchanged.
 */
uint16_t GradientBuildColours(uint16_t x, int *carry)
{
    cyc(288);
    uint8_t bank = rd8(0x03F6F3 + x);
    uint16_t y = rd16(0x03F6F1 + x), second;
    int i;

    wr16(0x0000F3, 0x0000);
    GradientExpandFirst(bank, &y, carry);
    cyc(176);
    second = rd16(0x03F745 + x);
    if (second) {
        y = second;
        wr16(0x0000F3, 0x0400);
        GradientExpandSecond(bank, &y, carry);
    }
    cyc(52);
    for (i = 0xFE; i >= 0; i -= 2)
        { cyc(50); wr16(W(gradient_colours_clear) + (uint32_t)i, 0x0000); cyc(32); }
    return y;
}

/* GradientBuildColours on carried registers. */
void GradientBuildColours_cr(Carried *cr)
{
    int carry;

    cr->y = GradientBuildColours(cr->x, &carry);
    cr->a = 0;
    cr->x = 0xFFFE;
    set_p(cr, (uint8_t)((nz8(cr->p, cr->db) & ~P_C) | P_M | (carry ? P_C : 0)));
    cyc(104);
}

void abi_GradientBuildColours(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    GradientBuildColours_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* Store a word `count` times at $7F:3497 + X, stepping 2. */
static uint16_t fill_cgadd(uint16_t value, uint16_t x, uint16_t count)
{
    cyc(0);
    do {
        cyc(32);
        wr16(W(gradient_cgadd) + x, value);
        x = (uint16_t)(x + 2);
    } while (cyc_loop(64, --count));
    cyc(100);
    return x;
}

/*
 * @routine 03:ED7F GradientFillCgadd
 * WHAT IT DOES: Fills Y entries of the CGADD table at $7F:3497 + X with A.
 * HOW: 16-bit stores, X stepping by 2.
 * Entry: M 16-bit, X 16-bit; Y = count (0 means 65536); A = value, X =
 * the start, as arguments. Exit: X past the last entry (the result); Y = 0; Z set, N clear.
 */
uint16_t GradientFillCgadd(uint16_t value, uint16_t x, uint16_t count)
{
    return fill_cgadd(value, x, count);
}

/* GradientFillCgadd on carried registers. */
void GradientFillCgadd_cr(Carried *cr)
{
    cr->x = GradientFillCgadd(cr->a, cr->x, cr->y);
    cr->y = 0;
    set_p(cr, (uint8_t)((cr->p & ~P_N) | P_Z));
}

void abi_GradientFillCgadd(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    GradientFillCgadd_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:FB1C GradientFillCgaddB
 * WHAT IT DOES: The same fill as GradientFillCgadd, in bank $0B.
 * HOW: As GradientFillCgadd.
 * Entry: as GradientFillCgadd. Exit: as GradientFillCgadd.
 */
uint16_t GradientFillCgaddB(uint16_t value, uint16_t x, uint16_t count)
{
    return fill_cgadd(value, x, count);
}

/* GradientFillCgaddB on carried registers. */
void GradientFillCgaddB_cr(Carried *cr)
{
    cr->x = GradientFillCgaddB(cr->a, cr->x, cr->y);
    cr->y = 0;
    set_p(cr, (uint8_t)((cr->p & ~P_N) | P_Z));
}

void abi_GradientFillCgaddB(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    GradientFillCgaddB_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 7E:B2A5 GradientPointersToBank7F
 * WHAT IT DOES: Copies the two colour addresses at $0D96 and $0D98 into the
 * CGDATA table's entries ($7F:348D, $7F:3493), scrolling the gradient.
 * HOW: Two 16-bit copies.
 * Entry: any widths, DB a bank where $0D96 is work RAM. Exit: A = the second
 * address; N, Z from it; M 16-bit.
 */
uint16_t GradientPointersToBank7F(void)
{
    cyc(46);
    uint16_t v = rd16(0x000D96);

    cyc(32);
    wr16(W(gradient_ptr_a), v);
    cyc(24);
    v = rd16(W(gradient_src_b));
    cyc(32);
    wr16(W(gradient_ptr_b), v);
    cyc(42);
    return v;
}

/* GradientPointersToBank7F on carried registers. */
void GradientPointersToBank7F_cr(Carried *cr)
{
    cr->a = GradientPointersToBank7F();
    set_p(cr, (uint8_t)(nz16(cr->p, cr->a) & ~P_M));
}

void abi_GradientPointersToBank7F(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    GradientPointersToBank7F_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:FE12 GradientSetup
 * WHAT IT DOES: Sets up the gradient: HDMA channel 2 to CGADD from
 * $7F:3497 and channel 3 to CGDATA (indirect, mode 2) from $7F:3489; fills
 * the CGADD table ($17 lines of colour 1, $50 single lines of colour 1,
 * then $18 lines and $50 more, then the end) and the CGDATA table (entries
 * of $17, $50, $18 and $50 lines with colour addresses $EC57, $EC57, $EC57
 * and $ED57 in bank $7F). Then
 * it reads the course's 32-byte block (pointer table $03:F6F1) and 32 bytes
 * at $1F:A800 without using them.
 * HOW: Channel 3's mode, B-bus register and bank are stored as words, so
 * $4331, $4332 and $4335 get a zero byte first; the table is written in the
 * original's order, including two addresses later overwritten.
 * Entry: any widths, D = $0000. Exit: A = the high byte of the course's
 * block address and the last byte read; X = Y = $20; P unchanged.
 */
uint16_t GradientSetup(uint8_t bank)
{
    static const struct { uint32_t a; uint16_t v; } ptr[6] = {
        {0x7F348A, 0xEC57}, {0x7F348D, 0xECD7}, {0x7F3490, 0xEC57},
        {0x7F3493, 0xECD7}, {0x7F348D, 0xEC57}, {0x7F3493, 0xED57}};
    static const struct { uint32_t a; uint8_t v; } cnt[5] = {
        {0x7F3489, 0x17}, {0x7F348C, 0xD0}, {0x7F348F, 0x18}, {0x7F3492, 0xD0}, {0x7F3494, 0x00}};
    uint32_t b = (uint32_t)bank << 16;
    uint16_t x, i, c, src, a;
    uint8_t blk;
    unsigned k;

    cyc(152);
    dma_wr(b | 0x4320, 0x00);
    cyc(40);
    dma_wr(b | 0x4321, 0x21);
    cyc(40);
    dma_wr(b | 0x4324, 0x7F);
    cyc(70);
    dma_wr16(b | 0x4322, 0x3497);
    cyc(48);
    dma_wr16(b | 0x4330, 0x0042);
    cyc(48);
    dma_wr16(b | 0x4331, 0x0022);
    cyc(48);
    dma_wr16(b | 0x4334, 0x007F);
    cyc(70);
    dma_wr16(b | 0x4332, 0x3489);
    cyc(102);
    wr16(W(gradient_cgadd), 0x0117);
    x = 2;
    cyc(94);
    (cyc(0), GradientFillCgaddB(0x0181, x, 0x0050));
    x = (uint16_t)(x + 0xA0);
    cyc(56);
    wr16(W(gradient_cgadd) + x, 0x0118);
    x = (uint16_t)(x + 2);
    cyc(122);
    (cyc(0), GradientFillCgaddB(0x0181, x, 0x0050));
    x = (uint16_t)(x + 0xA0);
    cyc(56);
    wr16(W(gradient_cgadd) + x, 0x0000);
    for (k = 0; k < 6; k++)
        { cyc(56); wr16(ptr[k].a, ptr[k].v); }
    cyc(22);
    for (k = 0; k < 5; k++)
        { cyc(48); wr8(cnt[k].a, cnt[k].v); }
    cyc(40);
    dma_wr(b | 0x4337, 0x7F);
    cyc(90);
    c = rd8(b | 0x0E3D);
    i = (uint16_t)(((uint16_t)(c << 1) + rd8(cyc_then(52, b | 0x0E3D))) & 0x00FF);
    /* the rest reads only ROM: 8 a byte read, and the code between */
    cyc(114);
    blk = rd8(0x03F6F3 + i);
    cyc(8 + 104);
    src = rd16(0x03F6F1 + i);
    cyc(16 + 90);
    for (k = 0; k < 0x20; k++) {
        rd8((uint32_t)blk << 16 | (uint16_t)(src + k));
        cyc(8 + (k < 0x1F ? 104 : 152));
    }
    a = (uint16_t)(src & 0xFF00);
    for (k = 0; k < 0x20; k++) {
        a = (uint16_t)((a & 0xFF00) | rd8(0x1FA800 + k));
        cyc(8 + (k < 0x1F ? 92 : 124));
    }
    return a;
}

/* GradientSetup on carried registers. */
void GradientSetup_cr(Carried *cr)
{
    cr->a = GradientSetup(cr->db);
    cr->x = 0x0020;
    cr->y = 0x0020;
}

void abi_GradientSetup(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    GradientSetup_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}
