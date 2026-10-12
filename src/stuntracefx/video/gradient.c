/*
 * gradient.c: the colour gradient drawn by HDMA. Channel 2 writes CGADD
 * once a line from the table at $7F:3497 (a line count and a colour number
 * per entry); channel 3 writes CGDATA, indirect, from the table at
 * $7F:3489, whose entries are a line count and the address in bank $7F of
 * the colours for those lines. Moving the addresses at $7F:348D and
 * $7F:3493 (copied from $0D96 and $0D98) scrolls the gradient.
 */
#include "machine.h"
#include "hw.h"

/* Store a word `count` times at $7F:3497 + X, stepping 2. */
static uint16_t fill_cgadd(uint16_t value, uint16_t x, uint16_t count)
{
    do {
        wr16(0x7F3497 + x, value);
        x = (uint16_t)(x + 2);
    } while (--count);
    return x;
}

/*
 * @routine 03:ED7F GradientFillCgadd
 * WHAT IT DOES: Fills Y entries of the CGADD table at $7F:3497 + X with A.
 * HOW: 16-bit stores, X stepping by 2.
 * Entry: M 16-bit, X 16-bit; Y = count (0 means 65536). Exit: X past the
 * last entry; Y = 0; Z set, N clear.
 */
void GradientFillCgadd(uint16_t value, uint16_t x, uint16_t count)
{
    fill_cgadd(value, x, count);
}

void abi_GradientFillCgadd(void)
{
    cpu.x = fill_cgadd(cpu.a, cpu.x, cpu.y);
    cpu.y = 0;
    cpu.p = (uint8_t)((cpu.p & ~P_N) | P_Z);
}

/*
 * @routine 0B:FB1C GradientFillCgaddB
 * WHAT IT DOES: The same fill as GradientFillCgadd, in bank $0B.
 * HOW: As GradientFillCgadd.
 * Entry: as GradientFillCgadd. Exit: as GradientFillCgadd.
 */
void GradientFillCgaddB(uint16_t value, uint16_t x, uint16_t count)
{
    fill_cgadd(value, x, count);
}

void abi_GradientFillCgaddB(void)
{
    cpu.x = fill_cgadd(cpu.a, cpu.x, cpu.y);
    cpu.y = 0;
    cpu.p = (uint8_t)((cpu.p & ~P_N) | P_Z);
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
    uint16_t v = rd16(0x000D96);

    wr16(0x7F348D, v);
    v = rd16(0x000D98);
    wr16(0x7F3493, v);
    return v;
}

void abi_GradientPointersToBank7F(void)
{
    cpu.a = GradientPointersToBank7F();
    cpu.p = (uint8_t)(nz16(cpu.p, cpu.a) & ~P_M);
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

    wr8(b | 0x4320, 0x00);
    wr8(b | 0x4321, 0x21);
    wr8(b | 0x4324, 0x7F);
    wr16(b | 0x4322, 0x3497);
    wr16(b | 0x4330, 0x0042);
    wr16(b | 0x4331, 0x0022);
    wr16(b | 0x4334, 0x007F);
    wr16(b | 0x4332, 0x3489);
    wr16(0x7F3497, 0x0117);
    x = 2;
    GradientFillCgaddB(0x0181, x, 0x0050);
    x = (uint16_t)(x + 0xA0);
    wr16(0x7F3497 + x, 0x0118);
    x = (uint16_t)(x + 2);
    GradientFillCgaddB(0x0181, x, 0x0050);
    x = (uint16_t)(x + 0xA0);
    wr16(0x7F3497 + x, 0x0000);
    for (k = 0; k < 6; k++)
        wr16(ptr[k].a, ptr[k].v);
    for (k = 0; k < 5; k++)
        wr8(cnt[k].a, cnt[k].v);
    wr8(b | 0x4337, 0x7F);
    c = rd16(b | 0x0E3D);
    i = (uint16_t)(((uint16_t)(c << 1) + c) & 0x00FF);
    blk = rd8(0x03F6F3 + i);
    src = rd16(0x03F6F1 + i);
    for (k = 0; k < 0x20; k++)
        rd8((uint32_t)blk << 16 | (uint16_t)(src + k));
    a = (uint16_t)(src & 0xFF00);
    for (k = 0; k < 0x20; k++)
        a = (uint16_t)((a & 0xFF00) | rd8(0x1FA800 + k));
    return a;
}

void abi_GradientSetup(void)
{
    cpu.a = GradientSetup(cpu.db);
    cpu.x = 0x0020;
    cpu.y = 0x0020;
}
