/*
 * programs_vector.c: two more S-CPU wrappers of Super FX programs on the
 * vector in direct page $02, $08, $94, beside those in programs.c. No play
 * runs them; unit_gsu_vector calls them.
 */
#include "machine.h"

uint8_t GsuRun(uint8_t bank, uint16_t start);
void abi_Gsu81E3Long(void);

/*
 * @routine 03:AF9D Gsu8277
 * WHAT IT DOES: Runs Super FX program $01:8277 on the vector in direct
 * page $02, $08, $94 and puts its three results in direct page $B7, $B9,
 * $BB.
 * HOW: As Gsu82CE: inputs to $70:0068, $70:002C, $70:002E; GsuRun; results
 * from $70:0026, $70:0028, $70:002A. X, Y and P saved and restored.
 * Entry: any widths, D = $0000. Exit: A = the third result (16 bits); X, Y
 * unchanged; P unchanged but N, Z, which the final pull of X sets.
 */
uint16_t Gsu8277(void)
{
    uint16_t v;
    unsigned i;

    v = rd16(0x000002);
    wr16(0x700068, v);
    v = rd16(0x000008);
    wr16(0x70002C, v);
    v = rd16(0x000094);
    wr16(0x70002E, v);
    GsuRun(0x01, 0x8277);
    for (i = 0; i < 3; i++) {
        v = rd16(0x700026 + 2 * i);
        wr16(0x0000B7 + 2 * i, v);
    }
    return v;
}

void abi_Gsu8277(void)
{
    uint8_t p = cpu.p;
    uint16_t x = cpu.x, y = cpu.y;

    cpu.a = Gsu8277();
    cpu.x = x;
    cpu.y = y;
    cpu.p = (p & P_X) ? nz8(p, (uint8_t)x) : nz16(p, x);
}

/*
 * @routine 03:C9D2 VectorGsu81E3
 * WHAT IT DOES: Puts direct page $04 - $02 in $02 and $05F2 - $94 in $08,
 * then runs Gsu81E3Long: ObjectGsu81E3 for a point in $02, $94 instead of
 * an object.
 * HOW: 16-bit subtracts, $05F2 through the data bank; X and Y saved and
 * restored.
 * Entry: X 16-bit, D = $0000. Exit: A as Gsu81E3Long leaves it; X, Y
 * unchanged; M 16-bit; N, Z from X.
 */
void VectorGsu81E3(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint16_t x = cpu.x, y = cpu.y, u, v;

    cpu.p &= (uint8_t)~P_M;
    u = rd16(0x000004);
    v = rd16(0x000002);
    wr16(0x000002, (uint16_t)(u - v));
    u = rd16(b | 0x05F2);
    v = rd16(0x000094);
    cpu.a = (uint16_t)(u - v);
    cpu.p = (uint8_t)((cpu.p & ~(P_C | P_V)) | (u >= v ? P_C : 0) | (((u ^ v) & (u ^ cpu.a) & 0x8000) ? P_V : 0));
    wr16(0x000008, cpu.a);
    abi_Gsu81E3Long();
    cpu.y = y;
    cpu.x = x;
    cpu.p = nz16(cpu.p & ~P_M, x);
}

void abi_VectorGsu81E3(void)
{
    VectorGsu81E3();
}
