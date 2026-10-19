/*
 * points.c: the Super FX's point records at $0826, ten bytes each: two
 * words at +0 and +2, then a point's X, Y, Z at +4, +6, +8. The points are
 * taken from the table at $0340 (three words each), picked by index bytes
 * in ROM and moved by the vector at $0026.
 */
#include "gsu.h"
#include "gsu_ops.h"

#define FX_RECORDS 0x0826u
#define FX_POINTS 0x0340u
#define FX_OFFSET 0x0026u   /* X, Y, Z added to each point */
#define FX_PAIRS 0x005Au    /* the address of the pair list, plus 4 */

/*
 * @routine 01:C1BE FxPointsGather
 * WHAT IT DOES: Fills R9 point records: the halved word pairs of the
 * 8-byte entries at the word at $005A (less 4), and the points named by R9
 * index bytes at R14, each moved by the vector at $0026.
 * HOW: CACHE. A LOOP of R9 passes from the last entry back: two LDWs from
 * entry i, DIV2 of each, STW to record i at +0 and +2 (records 1 to R9).
 * Then a LOOP of R9 passes forward: GETB, INC R14, UMULT #6 picks a point
 * at $0340; its three words plus the vector go to record k at +4, +6, +8
 * (records 0 to R9 - 1). R9 = 0 runs each loop $10000 times.
 * Entry: R9 = the count; R14 = the first index byte, in the ROM bank;
 * R11 = return address. Exit: R14 past the last index byte; R2 = $082A
 * plus ten per record; R1 = the last point's Z address; R0 = the last Z
 * stored; R3 = record 1's first word as stored; R5, R6, R7 = the
 * vector; R12 = 0; R13 = $C1FD (the second loop); the last RAM address
 * the last Z's; OV and CY from the last ADD #6 to R2; Z set, S clear.
 * R14 and the ROM buffer move as on the Super FX.
 */
/* What FxPointsGather leaves in registers besides R14. */
typedef struct { uint16_t first_word, offset[3], last_z, last_point; } FxPointsLeft;

/* gsuwalk: the code of each copy of FxPointsGather, read from the ROM */
static const GsuCode FxPointsGather_fetch[1][9] = {
    {{0x01c1ed, 4}, {0x01c1f1, 3}, {0x01c1f4, 3}, {0x01c200, 8}, {0x01c20c, 3}, {0x01c213, 3}, {0x01c208, 4}, {0x01c20f, 4}, {0x01c216, 2}},
};

void FxPointsGather(uint16_t count, FxPointsLeft *left)
{
    
    uint32_t n = count ? count : 0x10000u;
    gsu_code(0x01c1c0, 3);
    uint16_t src = (uint16_t)(gsu_ldw(FX_PAIRS) - 4);
    uint16_t dst;

    src = (uint16_t)(src + gsu_umult(count, 8));            /* FROM R9; UMULT #8 */
    dst = (uint16_t)(FX_RECORDS + gsu_umult(count, 10));    /* FROM R9; UMULT #10 */
    uint32_t i;
    int k;

    /* UMULT takes the count's low byte only: the starting entry and record
       are those of count & $FF. */
    i = 0;
    gsu_code(0x01c1c3, 20);
    do {                                       /* LOOP at the bottom */
        gsu_code(0x01c1d7, 2);
        uint16_t a = gsu_div2(gsu_ldw(src));
        gsu_code(0x01c1d9, 3);
        uint16_t b = gsu_div2(gsu_ldw((uint16_t)(src + 2)));

        gsu_code(0x01c1dc, 7);
        gsu_stw(dst, a);
        gsu_code(0x01c1e3, 3);
        gsu_stw((uint16_t)(dst + 2), b);
        left->first_word = a;
        src = (uint16_t)(src - 8);
        dst = (uint16_t)(dst - 10);
    } while (gsu_code_loop(0x01c1d6, 1, gsu_code_loop(0x01c1e6, 7, ++i < n)));
    gsu_code(0x01c1e6, 7);
    for (k = 0; k < 3; k++)
        { gsu_codes(FxPointsGather_fetch[0] + k, 1);
        left->offset[k] = gsu_ldw((uint16_t)(FX_OFFSET + k * 2)); }
    dst = FX_RECORDS + 4;
    i = 0;
    gsu_code(0x01c1f7, 7);
    do {                                       /* LOOP at the bottom */
        gsu_code(0x01c1fe, 1);
        uint16_t p = gsu_getb();

        gsu_code(0x01c1ff, 1);
        gsu_r14((uint16_t)(gsu.r[14] + 1));
        p = (uint16_t)(FX_POINTS + gsu_umult(p, 6));     /* UMULT #6 */
        for (k = 0; k < 3; k++) {
            gsu_codes(FxPointsGather_fetch[0] + 3 + k, 1);
            left->last_z = (uint16_t)(gsu_ldw((uint16_t)(p + k * 2)) + left->offset[k]);
            gsu_codes(FxPointsGather_fetch[0] + 6 + k, 1);
            gsu_stw((uint16_t)(dst + k * 2), left->last_z);
        }
        left->last_point = p;
        dst = (uint16_t)(dst + 10);
    } while (gsu_code_loop(0x01c1fd, 1, gsu_code_loop(0x01c218, 4, ++i < n)));
    gsu_code(0x01c218, 4);
    
}

void abi_FxPointsGather(void)
{
    uint16_t count = gsu.r[9];
    uint32_t n = count ? (gsu_code(0x01c1bf, 1), count) : (gsu_code(0x01c1bf, 1), 0x10000u);
    FxPointsLeft left;

    FxPointsGather(count, &left);
    gsu.r[0] = left.last_z;
    gsu.r[1] = (uint16_t)(left.last_point + 4);
    gsu.r[2] = gsu_add((uint16_t)(0x082A + (n - 1) * 10 + 4), 6, 0);
    gsu.r[3] = left.first_word;
    gsu.r[5] = left.offset[0];
    gsu.r[6] = left.offset[1];
    gsu.r[7] = left.offset[2];
    gsu.r[12] = 0;
    gsu.r[13] = 0xC1FD;
    gsu.z = 1;
    gsu.s = 0;
    gsu_code(0x01c21c, 2);
}
