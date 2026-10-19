/*
 * angle.c: the Super FX's angles: a full turn is $10000, so $4000 is a
 * quarter turn. The arctangent comes from a table of 256 words in ROM
 * bank $00 at $9E40, indexed by the ratio of the smaller side to the
 * larger.
 */
#include "gsu.h"
#include "gsu_ops.h"

#define FX_ATAN_TABLE 0x009E40u

uint16_t FxDivide32By16(uint16_t hi, uint16_t lo, uint16_t divisor, uint16_t *remainder, int copy);

/* gsuwalk: the code of each copy of magnitude, read from the ROM */
static const GsuCode magnitude_fetch[3][3] = {
    {{0, 0}, {0, 0}, {0, 0}},
    {{0x0181fe, 10}, {0x0181fe, 6}, {0x018207, 1}},
    {{0x018208, 7}, {0x018208, 4}, {0x01820e, 1}},
};

static uint16_t magnitude(uint16_t v, int copy)
{
    return v & 0x8000 ? (gsu_codes(magnitude_fetch[copy], 1), ((((uint16_t)-v)))) : (gsu_codes(magnitude_fetch[copy] + 1, 2), ((v)));
}

/*
 * @routine 01:81F7 FxAtan2
 * WHAT IT DOES: The angle of the vector (R2, R1): 0 along +R2, $4000
 * along +R1, as a word where a full turn is $10000.
 * HOW: Pushes R11 and R14 on the R10 stack. With R6 = |R1| and R0 = |R2|:
 * R2 = 0 gives $4000 and equal sides give $2000; otherwise the smaller
 * side << 14 is divided by the larger (FxDivide32By16), and the quotient
 * >> 5, made even, indexes the arctangent table (GETB/GETBH from ROM bank
 * 0). When |R1| is the larger side the angle is $4000 minus the table's.
 * Then it is negated when R1 and R2 differ in sign, and $8000 is added
 * when R2 is negative. Pulls R14 and returns by pulling R11's value into
 * R15.
 * Entry: R1, R2 = the vector; R10 = the stack; R11 = return address.
 * Exit: R0 = the angle; R4 = $8000 when R2 is negative, else R1 XOR R2;
 * R6 = the larger side (|R1| when R2 = 0 or the sides are equal); R5 =
 * $FFFF when |R2| is the larger side, 0 when |R1| is or they are equal,
 * kept when R2 = 0. When the table was used: R11 = $823D, R12 = 0, R13 =
 * $81D2, the ROM bank 0. R14 (written back, so the ROM buffer refilled
 * from it) and R10 as at entry; the last RAM address
 * R10; S and Z from R10; OV and CY from the ADD of $8000 when R2 is
 * negative, else OV = bit 7 of R2 and CY from the last ADD before (kept
 * when R2 = 0, set when the sides are equal).
 */
uint16_t FxAtan2(uint16_t y, uint16_t x)
{
    uint16_t ay = magnitude(y, GSU_COPY(1)), ax = magnitude(x, GSU_COPY(2));
    uint16_t angle;

    if (ax == 0) {
        angle = 0x4000;
        
        
        gsu_code(0x01820f, 9); gsu_code(0x01825c, 1);
    } else if (ax == ay) {
        angle = 0x2000;
        
        
        gsu_code(0x01820f, 2); gsu_code(0x018218, 14); gsu_code(0x01825c, 1);
    } else {
        uint16_t small = ax, large = ay, rem, ratio, at;
        int x_larger = !((int16_t)ax < (int16_t)ay);

        if (x_larger) {
            small = ay;
            large = ax;
            
            
            gsu_code(0x01820f, 2); gsu_code(0x018218, 7); gsu_code(0x018226, 11);
        } else { gsu_code(0x01820f, 2); gsu_code(0x018218, 7); gsu_code(0x018226, 3); gsu_code(0x018230, 1); }
        
        
        gsu_code(0x018231, 12); gsu_code(0x0181cb, 1);
        ratio = FxDivide32By16(small >> 2, (uint16_t)(small << 14), large, &rem, GSU_COPY(0));
        at = (uint16_t)(FX_ATAN_TABLE + ((ratio >> 5) & 0xFFFE));
        gsu_code(0x01823d, 4);
        gsu_romb(0);
        gsu_code(0x018241, 13);
        gsu_r14(at);
        gsu_code(0x01824e, 1);
        angle = gsu_getb();
        gsu_code(0x01824f, 1);
        gsu_r14((uint16_t)(at + 1));
        gsu_code(0x018250, 2);
        angle = (uint16_t)(angle | gsu_getb() << 8);
        if (!x_larger)
            { angle = (uint16_t)(0x4000 - angle); gsu_code(0x018252, 4); gsu_code(0x01825c, 1); } else { gsu_code(0x018252, 11); }
    }
    if ((x ^ y) & 0x8000)
        { angle = (uint16_t)-angle; gsu_code(0x01825d, 9); } else { gsu_code(0x01825d, 6); gsu_code(0x018265, 1); }
    if (x & 0x8000)
        { angle = (uint16_t)(angle + 0x8000); gsu_code(0x018266, 9); } else { gsu_code(0x018266, 4); gsu_code(0x01826e, 1); }
    return angle;
}

void abi_FxAtan2(void)
{
    uint16_t y = gsu.r[1], x = gsu.r[2];
    uint16_t ay = magnitude(y, GSU_COPY(0)), ax = magnitude(x, GSU_COPY(0));
    uint16_t angle;

    gsu_code(0x0181f8, 2);
    gsu_push(&gsu.r[10], gsu.r[11]);
    gsu_code(0x0181fa, 4);
    gsu_push(&gsu.r[10], gsu.r[14]);
    angle = FxAtan2(y, x);
    gsu.r[6] = ay;
    if (ax == ay && ax != 0) {
        gsu.r[5] = 0;
        gsu.cy = 1;
        gsu_code(0x01826f, 2);
    } else if (ax != 0) {
        int x_larger = !((int16_t)ax < (int16_t)ay);
        uint16_t pre;

        gsu.r[5] = x_larger ? (gsu_code(0x01826f, 2), ((0xFFFF))) : (gsu_code(0x01826f, 2), ((0)));
        if (x_larger)
            gsu.r[6] = ax;
        gsu.r[11] = 0x823D;
        gsu.r[12] = 0;
        gsu.r[13] = 0x81D2;
        /* The table's word, before the sign steps. */
        pre = (uint16_t)((x ^ y) & 0x8000 ? -(x & 0x8000 ? (uint16_t)(angle - 0x8000) : angle)
                                           : (x & 0x8000 ? (uint16_t)(angle - 0x8000) : angle));
        if (!x_larger)
            gsu_add((uint16_t)-(uint16_t)(0x4000 - pre), 0x4000, 0);
        else
            gsu.cy = 0;  /* the ADD of the table's address */
    } else { gsu_code(0x01826f, 2); }
    gsu.r[4] = (uint16_t)(x ^ y);
    gsu_moves(x);
    if (x & 0x8000) {
        gsu.r[4] = 0x8000;
        gsu_add((uint16_t)(angle - 0x8000), 0x8000, 0);
    }
    gsu.r[0] = angle;
    gsu_code(0x018271, 2);
    gsu_r14(gsu_pull(&gsu.r[10]));
    gsu_code(0x018273, 4);
    gsu_pull(&gsu.r[10]);       /* the return address, into R15 */
}

#define FX_STACK 0x0240u    /* where each program starts the R10 stack */
#define FX_ANGLE_IN_Y 0x0068u
#define FX_ANGLE_IN_X 0x002Cu
#define FX_ANGLE_OUT 0x0040u

/*
 * @routine 01:81E3 FxAngleProgram
 * WHAT IT DOES: A program the S-CPU starts: the angle of the vector
 * ($002C, $0068) by FxAtan2, stored at $0040.
 * HOW: Sets the R10 stack to $0240; LMS loads $0068 into R1 and $002C into
 * R2; FxAtan2; SMS stores R0. Then STOP.
 * Entry: started by the S-CPU. Exit: (at the STOP) R10 = $0240; R11 =
 * $81F1; the registers and flags as FxAtan2 leaves them; the last RAM
 * address $0040.
 */
void FxAngleProgram(void)
{
    gsu.r[10] = FX_STACK;
    gsu_code(0x0181e4, 6);
    gsu.r[1] = gsu_ldw(FX_ANGLE_IN_Y);
    gsu_code(0x0181ea, 3);
    gsu.r[2] = gsu_ldw(FX_ANGLE_IN_X);
    /* LINK #4, then into FxAtan2 with its stack traffic and exit registers. */
    gsu.r[11] = 0x81F1;
    
    gsu_code(0x0181ed, 4); gsu_code(0x0181f7, 1);
    abi_FxAtan2();
    gsu_code(0x0181f1, 4);
    gsu_stw(FX_ANGLE_OUT, gsu.r[0]);
    gsu_code(0x0181f5, 1);
}

void abi_FxAngleProgram(void)
{
    FxAngleProgram();
}
