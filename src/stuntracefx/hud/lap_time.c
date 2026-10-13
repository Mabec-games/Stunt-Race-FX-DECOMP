/*
 * lap_time.c: race times as the HUD and the records show them. Times are
 * kept packed in a word: hundredths in bits 0-6, seconds in bits 7-12,
 * minutes in bits 13-15.
 */
#include "machine.h"
#include "cpu_ops.h"

/*
 * @routine 07:ACD9 BinaryToBcd
 * WHAT IT DOES: Converts A (below 10000) to binary-coded decimal.
 * HOW: Counts thousands, hundreds and tens by repeated subtraction into
 * direct page $F1 ($1000, $100, $10 per step), then adds the units. Not
 * reached in the traces so far.
 * Entry: M 16-bit, D = $0000; A = the value. Exit: A = the BCD value; $F1
 * = A less its units digit; X, Y unchanged; N, Z, C, V from the last add.
 */
uint16_t BinaryToBcd(uint16_t v)
{
    static const uint16_t unit[3] = {1000, 100, 10}, digit[3] = {0x1000, 0x0100, 0x0010};
    unsigned i;

    wr16(0x0000F1, 0x0000);
    for (i = 0; i < 3; i++) {
        while (v >= unit[i]) {
            v = (uint16_t)(v - unit[i]);
            wr16(0x0000F1, (uint16_t)(rd16(0x0000F1) + digit[i]));
        }
    }
    return (uint16_t)(v + rd16(0x0000F1));
}

void abi_BinaryToBcd(void)
{
    static const uint16_t unit[3] = {1000, 100, 10}, digit[3] = {0x1000, 0x0100, 0x0010};
    uint16_t a;
    unsigned i;

    wr16(0x0000F1, 0x0000);
    for (i = 0; i < 3; i++) {
        for (;;) {
            op_cmp16(unit[i]);
            if (!(cpu.p & P_C))
                break;
            cpu.a = op_sbc16(cpu.a, unit[i], &cpu.p);
            a = cpu.a;                  /* PHA, the count, PLA */
            op_lda16(0x0000F1);
            op_add16(digit[i]);
            wr16(0x0000F1, cpu.a);
            op_lda16_imm(a);
        }
    }
    op_add16(rd16(0x0000F1));
}

/*
 * @routine 07:AC46 TimeUnpackBcd
 * WHAT IT DOES: Unpacks a packed time word into BCD: A = the minutes (bits
 * 13-15), X = the seconds (bits 7-12), Y = the hundredths (bits 0-6); $FFFF
 * (no time) gives $FFFF in all three.
 * HOW: Masks and shifts, each part through BinaryToBcd. Only the $FFFF
 * case is reached in the traces so far.
 * Entry: M 16-bit, X 16-bit, D = $0000; A = the packed time. Exit: A, X, Y
 * as above; $F1 as BinaryToBcd leaves it; N, Z, C, V from its last add, or
 * N set, Z and C set by the compare for $FFFF.
 */
void TimeUnpackBcd(uint16_t packed, uint16_t *min, uint16_t *sec, uint16_t *hund)
{
    if (packed == 0xFFFF) {
        *min = *sec = *hund = 0xFFFF;
        return;
    }
    *hund = BinaryToBcd(packed & 0x007F);
    *sec = BinaryToBcd((uint16_t)(packed << 1) >> 8 & 0x003F);
    *min = BinaryToBcd(packed >> 13);
}

void abi_TimeUnpackBcd(void)
{
    uint16_t v;

    op_cmp16(0xFFFF);
    if (cpu.p & P_Z) {
        op_lda16_imm(0xFFFF);
        cpu.x = cpu.y = 0xFFFF;
        return;
    }
    op_tax16();
    op_and16(0x007F);
    abi_BinaryToBcd();
    op_tay16();
    op_txa16();
    v = cpu.a;                          /* PHA */
    op_asl16();
    op_xba();
    op_and16(0x003F);
    abi_BinaryToBcd();
    op_tax16();
    op_lda16_imm(v);                    /* PLA */
    op_rol16();
    op_rol16();
    op_rol16();
    op_rol16();
    op_and16(0x0007);
    abi_BinaryToBcd();
}
