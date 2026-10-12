/*
 * records_7f.c: two tables of 4-byte display records in bank $7F, at
 * $7F:43D7 and $7F:47FF (with more blocks at $7F:45B7-$7F:4BB7), filled
 * from position lists in bank $09 that end with $FF.
 */
#include "machine.h"
#include "hw.h"

/*
 * @routine 09:EFFB Records7FFromF634
 * WHAT IT DOES: For each pair (x, y) in the list at $F634 (data bank), up to
 * the $FF that ends it, writes a record ($10, x - $28, y - $28, $A0) at
 * $7F:43D7 and one ($60, x + $28, y + $28, $F0) at $7F:47FF, four bytes on
 * each time.
 * HOW: 8-bit arithmetic; the stores in the original's order.
 * Entry: M 8-bit. Exit: A low byte = $FF; X, Y past the last record and
 * pair; M 8-bit, X 16-bit; Z, C set, N clear; V from the last add.
 */
void Records7FFromF634(uint8_t bank, uint16_t *x, uint16_t *y, uint8_t *p)
{
    uint32_t b = (uint32_t)bank << 16;
    uint8_t v, d;

    *x = 0;
    *y = 0xF634;
    while ((v = rd8(b | *y)) != 0xFF) {
        d = (uint8_t)(v - 0x28);
        wr8(0x7F43D8 + *x, d);
        wr8(0x7F4800 + *x, (uint8_t)(d + 0x50));
        wr8(0x7F43D7 + *x, 0x10);
        wr8(0x7F47FF + *x, 0x60);
        d = (uint8_t)(rd8(b | (uint16_t)(*y + 1)) - 0x28);
        wr8(0x7F43D9 + *x, d);
        *p = (uint8_t)((*p & ~P_V) | ((~(d ^ 0x50) & (d ^ (uint8_t)(d + 0x50)) & 0x80) ? P_V : 0));
        wr8(0x7F4801 + *x, (uint8_t)(d + 0x50));
        wr8(0x7F43DA + *x, 0xA0);
        wr8(0x7F4802 + *x, 0xF0);
        *x = (uint16_t)(*x + 4);
        *y = (uint16_t)(*y + 2);
    }
    *p = (uint8_t)((*p & ~P_N) | P_Z | P_C);
}

void abi_Records7FFromF634(void)
{
    uint8_t p = cpu.p;

    Records7FFromF634(cpu.db, &cpu.x, &cpu.y, &p);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0xFF);
    cpu.p = (uint8_t)((p & ~P_X) | P_M);
}

/*
 * @routine 09:EF52 Records7FFromF170
 * WHAT IT DOES: Writes a record ($18, x, y, $E7) at $7F:43D7 for each pair
 * in the list at $F170 (data bank) up to its $FF; then copies eight 32-byte
 * blocks from $09:F88B-$09:F96B to $7F:43D7, $45B7, $45D7, $47B7, $47D7,
 * $49B7, $49D7, $4BB7, and copies $40 bytes from $09:F849 to $7F:5187 by
 * DMA channel 0 through WMDATA.
 * HOW: The blocks are copied a byte at a time, all eight per index; the DMA
 * enable is a 16-bit store, so HDMAEN gets 0.
 * Entry: M 8-bit. Exit: A = 1; X = 9; Y = the low byte of the end of the
 * list; M 16-bit, X 8-bit; N, Z clear.
 */
uint16_t Records7FFromF170(uint8_t bank)
{
    static const uint32_t from[8] = {0x09F88B, 0x09F8AB, 0x09F8CB, 0x09F8EB, 0x09F90B, 0x09F92B, 0x09F94B, 0x09F96B};
    static const uint32_t to[8] = {0x7F43D7, 0x7F45B7, 0x7F45D7, 0x7F47B7, 0x7F47D7, 0x7F49B7, 0x7F49D7, 0x7F4BB7};
    uint32_t b = (uint32_t)bank << 16;
    uint16_t x = 0, y = 0xF170;
    uint8_t v;
    unsigned i, k;

    while ((v = rd8(b | y)) != 0xFF) {
        wr8(0x7F43D8 + x, v);
        wr8(0x7F43D7 + x, 0x18);
        wr8(0x7F43D9 + x, rd8(b | (uint16_t)(y + 1)));
        wr8(0x7F43DA + x, 0xE7);
        x = (uint16_t)(x + 4);
        y = (uint16_t)(y + 2);
    }
    for (i = 0; i < 0x20; i++)
        for (k = 0; k < 8; k++)
            wr8(to[k] + i, rd8(from[k] + i));
    wr8(b | (WMADDH & 0xFFFF), 0x7F);
    wr16(b | (WMADDL & 0xFFFF), 0x5187);
    wr8(b | (DMAP0 & 0xFFFF), 0x00);
    wr8(b | (BBAD0 & 0xFFFF), 0x80);
    wr16(b | (A1T0L & 0xFFFF), 0xF849);
    wr8(b | (A1B0 & 0xFFFF), 0x09);
    wr16(b | (DAS0L & 0xFFFF), 0x0040);
    wr16(b | (MDMAEN & 0xFFFF), 0x0001);
    return (uint16_t)(y & 0x00FF);
}

void abi_Records7FFromF170(void)
{
    cpu.y = Records7FFromF170(cpu.db);
    cpu.a = 0x0001;
    cpu.x = 0x0009;
    cpu.p = (uint8_t)((cpu.p & ~(P_N | P_Z | P_M)) | P_X);
}
