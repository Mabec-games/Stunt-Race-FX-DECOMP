/*
 * records_7f.c: two tables of 4-byte display records in bank $7F, at
 * $7F:43D7 and $7F:47FF (with more blocks at $7F:45B7-$7F:4BB7), filled
 * from position lists in bank $09 that end with $FF.
 */
#include "machine.h"
#include "native.h"
#include "hw.h"
#include "carried.h"

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
    cyc(0);
    while ((v = rd8(cyc_then(144, b | *y))) != 0xFF) {
        d = (uint8_t)(v - 0x28);
        cyc(80);
        wr8(0x7F43D8 + *x, d);
        cyc(62);
        wr8(0x7F4800 + *x, (uint8_t)(d + 0x50));
        cyc(48);
        wr8(0x7F43D7 + *x, 0x10);
        cyc(48);
        wr8(0x7F47FF + *x, 0x60);
        cyc(100);
        d = (uint8_t)(rd8(b | (uint16_t)(*y + 1)) - 0x28);
        wr8(0x7F43D9 + *x, d);
        *p = (uint8_t)((*p & ~P_V) | ((~(d ^ 0x50) & (d ^ (uint8_t)(d + 0x50)) & 0x80) ? P_V : 0));
        cyc(62);
        wr8(0x7F4801 + *x, (uint8_t)(d + 0x50));
        cyc(48);
        wr8(0x7F43DA + *x, 0xA0);
        cyc(48);
        wr8(0x7F4802 + *x, 0xF0);
        *x = (uint16_t)(*x + 4);
        *y = (uint16_t)(*y + 2);
        cyc(16);
    }
    *p = (uint8_t)((*p & ~P_N) | P_Z | P_C);
}

/* Records7FFromF634 on carried registers. */
void Records7FFromF634_cr(Carried *cr)
{
    uint8_t p = cr->p;

    Records7FFromF634(cr->db, &cr->x, &cr->y, &p);
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0xFF);
    set_p(cr, (uint8_t)((p & ~P_X) | P_M));
}

void abi_Records7FFromF634(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    Records7FFromF634_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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

    cyc(46);
    while ((v = rd8(cyc_then(108, b | y))) != 0xFF) {
        cyc(40);
        wr8(0x7F43D8 + x, v);
        cyc(48);
        wr8(0x7F43D7 + x, 0x18);
        cyc(70);
        wr8(0x7F43D9 + x, rd8(b | (uint16_t)(y + 1)));
        cyc(48);
        wr8(0x7F43DA + x, 0xE7);
        x = (uint16_t)(x + 4);
        y = (uint16_t)(y + 2);
        cyc(62);
    }
    for (i = 0; i < 0x20; i++)
        { cyc(52); for (k = 0; k < 8; k++)
            { cyc(72); wr8(to[k] + i, rd8(from[k] + i)); } }
    cyc(86);
    hw_wr(b | (WMADDH & 0xFFFF), 0x7F);
    cyc(70);
    hw_wr16(b | (WMADDL & 0xFFFF), 0x5187);
    cyc(40);
    dma_wr(b | (DMAP0 & 0xFFFF), 0x00);
    cyc(40);
    dma_wr(b | (BBAD0 & 0xFFFF), 0x80);
    cyc(48);
    dma_wr16(b | (A1T0L & 0xFFFF), 0xF849);
    cyc(40);
    dma_wr(b | (A1B0 & 0xFFFF), 0x09);
    cyc(48);
    dma_wr16(b | (DAS0L & 0xFFFF), 0x0040);
    cyc(48);
    mdmaen_wr(b | (MDMAEN & 0xFFFF), 0x01);   /* a 16-bit store: HDMAEN after */
    hdmaen_wr(b | (HDMAEN & 0xFFFF), 0x00);
    return (uint16_t)(y & 0x00FF);
}

/* Records7FFromF170 on carried registers. */
void Records7FFromF170_cr(Carried *cr)
{
    cr->y = Records7FFromF170(cr->db);
    cr->a = 0x0001;
    cr->x = 0x0009;
    set_p(cr, (uint8_t)((cr->p & ~(P_N | P_Z | P_M)) | P_X));
}

void abi_Records7FFromF170(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    Records7FFromF170_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}
