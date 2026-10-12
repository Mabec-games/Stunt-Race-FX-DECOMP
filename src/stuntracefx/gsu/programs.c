/*
 * programs.c: S-CPU wrappers that set a Super FX program's inputs in Super
 * FX RAM, run it with GsuRun, and collect what it leaves there.
 *
 * Program $01:CF42 draws into a buffer whose address it takes at $70:002C
 * (with inputs at $70:0068, $70:006A and a zero at $70:00A2); $01:80BC and
 * $01:82CE take a vector and return results; $01:8325 fills nine words at
 * $70:00E4; $07:EE88 reads an eight-word ramp at $70:3C00.
 */
#include "machine.h"
#include "hw.h"

uint8_t GsuRun(uint8_t bank, uint16_t start);

#define GSU_IN_68 0x700068u
#define GSU_IN_6A 0x70006Au
#define GSU_DEST 0x70002Cu
#define GSU_IN_A2 0x7000A2u

static void cf42_inputs(uint16_t in68, uint16_t dest)
{
    wr16(GSU_IN_68, in68);
    wr16(GSU_DEST, dest);
    wr16(GSU_IN_A2, 0x0000);
}

static uint8_t cf42(uint16_t in68, uint16_t dest, uint16_t in6a)
{
    cf42_inputs(in68, dest);
    wr16(GSU_IN_6A, in6a);
    return GsuRun(0x01, 0xCF42);
}

static void abi_cf42(uint16_t dest)
{
    uint16_t x = cpu.x;
    uint8_t mode = cf42(cpu.a, dest, x);

    cpu.a = (uint16_t)((x & 0xFF00) | mode);
    cpu.x = 0xCF42;
    cpu.p = (uint8_t)(nz8(cpu.p, cpu.db) | P_M);
}

/*
 * @routine 03:EBEF GsuCF42ToC000
 * WHAT IT DOES: Runs Super FX program $01:CF42 drawing to $C000, with inputs
 * A and X.
 * HOW: $70:0068 = A, $70:002C = $C000, $70:00A2 = 0, $70:006A = X, then
 * GsuRun.
 * Entry: M 16-bit, X 16-bit, D = $0000. Exit: A = X's high byte and the
 * screen mode value; X = $CF42; M 8-bit; N, Z from the data bank.
 */
uint8_t GsuCF42ToC000(uint16_t in68, uint16_t in6a)
{
    return cf42(in68, 0xC000, in6a);
}

void abi_GsuCF42ToC000(void)
{
    abi_cf42(0xC000);
}

/*
 * @routine 03:EBF8 GsuCF42To4000
 * WHAT IT DOES: As GsuCF42ToC000, drawing to $4000.
 * HOW: As GsuCF42ToC000.
 * Entry: as GsuCF42ToC000. Exit: as GsuCF42ToC000.
 */
uint8_t GsuCF42To4000(uint16_t in68, uint16_t in6a)
{
    return cf42(in68, 0x4000, in6a);
}

void abi_GsuCF42To4000(void)
{
    abi_cf42(0x4000);
}

/*
 * @routine 03:EC01 GsuCF42To3000
 * WHAT IT DOES: As GsuCF42ToC000, drawing to $3000.
 * HOW: As GsuCF42ToC000.
 * Entry: as GsuCF42ToC000. Exit: as GsuCF42ToC000.
 */
uint8_t GsuCF42To3000(uint16_t in68, uint16_t in6a)
{
    return cf42(in68, 0x3000, in6a);
}

void abi_GsuCF42To3000(void)
{
    abi_cf42(0x3000);
}

/*
 * @routine 03:EB83 GsuCF42ToVram
 * WHAT IT DOES: Runs program $01:CF42 drawing to $70:3000 and copies the
 * result to VRAM by DMA.
 * HOW: The inputs come from the record at X in the data bank (word at +0 to
 * $70:0068, byte at +2 to $70:006A); after GsuRun, the transfer comes from
 * the record at Y: VMAIN, the VRAM address, DMA mode and B-bus register, and
 * the size, with the source fixed at $70:3000.
 * Entry: M 16-bit, X 16-bit; X = input record, Y = transfer record. Exit: A =
 * 1; X = Y; M 8-bit, X 16-bit; N, Z clear.
 */
void GsuCF42ToVram(uint8_t bank, uint16_t in, uint16_t out)
{
    uint32_t b = (uint32_t)bank << 16;

    cf42_inputs(rd16(b | in), 0x3000);
    wr8(GSU_IN_6A, rd8(b | (uint16_t)(in + 2)));
    GsuRun(0x01, 0xCF42);
    wr8(VMAIN, rd8(b | out));
    wr8(VMADDL, rd8(b | (uint16_t)(out + 1)));
    wr8(VMADDH, rd8(b | (uint16_t)(out + 2)));
    wr8(DMAP0, rd8(b | (uint16_t)(out + 3)));
    wr8(BBAD0, rd8(b | (uint16_t)(out + 4)));
    wr8(A1T0L, 0x00);
    wr8(A1T0H, 0x30);
    wr8(A1B0, 0x70);
    wr8(DAS0L, rd8(b | (uint16_t)(out + 5)));
    wr8(DAS0H, rd8(b | (uint16_t)(out + 6)));
    wr8(MDMAEN, 0x01);
}

void abi_GsuCF42ToVram(void)
{
    GsuCF42ToVram(cpu.db, cpu.x, cpu.y);
    cpu.a = 0x0001;
    cpu.x = cpu.y;
    cpu.p = (uint8_t)((cpu.p & ~(P_N | P_Z | P_X)) | P_M);
}

/*
 * @routine 03:C9EE ObjectGsu80BC
 * WHAT IT DOES: Runs Super FX program $01:80BC on object X's offset from a
 * point and returns its result.
 * HOW: $70:0068 = direct page $04 - the object's +$0C; $70:002E = $05F2 -
 * its +$10 (both through the data bank); GsuRun; the result is $70:006A.
 * Entry: M 16-bit, X 16-bit, D = $0000; X = object. Exit: A = the result;
 * X unchanged; M 16-bit, X 16-bit; N, Z from A; C, V from the second
 * subtract.
 */
static uint16_t sub16(uint16_t a, uint16_t b, uint8_t *p)
{
    uint16_t r = (uint16_t)(a - b);

    *p = (uint8_t)((*p & ~(P_C | P_V)) | (a >= b ? P_C : 0) | (((a ^ b) & (a ^ r) & 0x8000) ? P_V : 0));
    return r;
}

uint16_t ObjectGsu80BC(uint8_t bank, uint16_t obj, uint8_t *p)
{
    uint32_t b = (uint32_t)bank << 16;

    wr16(GSU_IN_68, sub16(rd16(0x000004), rd16(b | (uint16_t)(0x200C + obj)), p));
    wr16(0x70002E, sub16(rd16(b | 0x05F2), rd16(b | (uint16_t)(0x2010 + obj)), p));
    GsuRun(0x01, 0x80BC);
    return rd16(GSU_IN_6A);
}

void abi_ObjectGsu80BC(void)
{
    uint8_t p = cpu.p;

    cpu.a = ObjectGsu80BC(cpu.db, cpu.x, &p);
    cpu.p = (uint8_t)(nz16(p, cpu.a) & ~(P_M | P_X));
}

/*
 * @routine 03:AFD7 Gsu82CE
 * WHAT IT DOES: Runs Super FX program $01:82CE on the vector in direct page
 * $02, $08, $94 and puts its three results in direct page $B7, $B9, $BB.
 * HOW: Inputs to $70:0068, $70:002C, $70:002E; GsuRun; results from
 * $70:0026, $70:0028, $70:002A.
 * Entry: any widths, D = $0000. Exit: A = the third result; X, Y, P
 * unchanged.
 */
uint16_t Gsu82CE(void)
{
    uint16_t r = 0;
    unsigned i;

    wr16(GSU_IN_68, rd16(0x000002));
    wr16(0x70002C, rd16(0x000008));
    wr16(0x70002E, rd16(0x000094));
    GsuRun(0x01, 0x82CE);
    for (i = 0; i < 3; i++) {
        r = rd16(0x700026 + 2 * i);
        wr16(0x0000B7 + 2 * i, r);
    }
    return r;
}

void abi_Gsu82CE(void)
{
    cpu.a = Gsu82CE();
}

/*
 * @routine 03:B011 Gsu8325
 * WHAT IT DOES: Runs Super FX program $01:8325 on the three words at $065E
 * and copies the nine words it leaves at $70:00E4 to $0608.
 * HOW: Inputs to $70:0020-$70:0024; GsuRun; nine word copies. Work RAM is
 * reached through the data bank.
 * Entry: X 16-bit, D = $0000. Exit: A = the last word; X = $25, Y high byte
 * clear; M 8-bit, X 8-bit; N, Z from A (16-bit).
 */
uint16_t Gsu8325(uint8_t bank)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t v = 0;
    unsigned i;

    for (i = 0; i < 3; i++)
        wr16(0x700020 + 2 * i, rd16(b | (0x065E + 2 * i)));
    GsuRun(0x01, 0x8325);
    for (i = 0; i < 9; i++) {
        v = rd16(0x7000E4 + 2 * i);
        wr16(b | (0x0608 + 2 * i), v);
    }
    return v;
}

void abi_Gsu8325(void)
{
    cpu.a = Gsu8325(cpu.db);
    cpu.x = 0x0025;
    cpu.y &= 0x00FF;
    cpu.p = (uint8_t)(nz16(cpu.p, cpu.a) | P_M | P_X);
}

/*
 * @routine 03:FE8D GsuEE88Ramp
 * WHAT IT DOES: Writes the ramp $80, $90, ... $F0 to $70:3C00 and runs Super
 * FX program $07:EE88.
 * HOW: Eight word stores, then GsuRun.
 * Entry: any widths. Exit: A = the screen mode value (high byte clear); X =
 * $88, Y high byte clear; M 16-bit, X 8-bit; N, Z from the data bank.
 */
uint8_t GsuEE88Ramp(void)
{
    unsigned i;

    for (i = 0; i < 8; i++)
        wr16(0x703C00 + 2 * i, (uint16_t)(0x80 + 0x10 * i));
    return GsuRun(0x07, 0xEE88);
}

void abi_GsuEE88Ramp(void)
{
    cpu.a = GsuEE88Ramp();
    cpu.x = 0x0088;
    cpu.y &= 0x00FF;
    cpu.p = (uint8_t)((nz8(cpu.p, cpu.db) & ~P_M) | P_X);
}
