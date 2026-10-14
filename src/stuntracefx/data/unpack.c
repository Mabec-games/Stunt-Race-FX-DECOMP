/*
 * unpack.c: the Super FX's decompressor. Packed data is read backward from
 * ROM (R14, in the ROM bank) through a 32-bit bit buffer, R2:R3 (R2 the
 * high word), shifted out from bit 0. A refill loads the next four bytes
 * and sets a marker bit above them, so the buffer is empty when R2:R3
 * shifts down to 0.
 */
#include "gsu.h"
#include "gsu_ops.h"

/*
 * @routine 01:D0DB FxUnpackRefill
 * WHAT IT DOES: Refills the bit buffer from the four ROM bytes at R14 and
 * below and takes its first bit.
 * HOW: GETB/GETBH with R14 stepping down: R3 = byte at R14 with the byte
 * below as its high byte, R2 = the next two the same way. Then ROR of R0
 * (just set to 1) puts 1 in CY, and ROR R2, ROR R3 shift the 32 bits down
 * one with that 1 as the marker on top; the bit shifted out is the first
 * bit. The caller's delay slot steps R14 down to the first byte.
 * Entry: R14 = the address of the first byte; R11 = return address.
 * Exit: R2:R3 = the bit buffer; CY = the first bit; R14 = the address of
 * the fourth byte; R0 = R2 | R3 (not 0), S and Z from it; OV kept.
 * Returns the first bit. R14 and the ROM buffer move as on the Super FX.
 */
int FxUnpackRefill(uint16_t *hi, uint16_t *lo)
{
    uint8_t b[4];
    uint16_t w3, w2;
    int i;

    for (i = 0; i < 4; i++) {
        b[i] = gsu_getb();
        if (i < 3)
            gsu_r14((uint16_t)(gsu.r[14] - 1));
    }
    w3 = (uint16_t)(b[0] | b[1] << 8);
    w2 = (uint16_t)(b[2] | b[3] << 8);

    *hi = (uint16_t)(0x8000 | w2 >> 1);
    *lo = (uint16_t)((w2 & 1) << 15 | w3 >> 1);
    return w3 & 1;
}

void abi_FxUnpackRefill(void)
{
    gsu.cy = (uint8_t)FxUnpackRefill(&gsu.r[2], &gsu.r[3]);
    gsu.r[0] = (uint16_t)(gsu.r[2] | gsu.r[3]);
    gsu_sz(gsu.r[0]);
}

/* The routine's first instruction, SUB R4, as the caller's pending prefix
   makes it (every call seen has WITH R4 there: R4 = R4 - R4 = 0). */
static void first_sub_r4(void)
{
    uint8_t s = gsu.pre_sreg, d = gsu.pre_dreg;

    if (gsu.pre_alt1 && gsu.pre_alt2)
        gsu_sub(gsu.r[s], gsu.r[4], 0);                 /* CMP */
    else if (gsu.pre_alt2)
        gsu.r[d] = gsu_sub(gsu.r[s], 4, 0);             /* SUB #4 */
    else
        gsu.r[d] = gsu_sub(gsu.r[s], gsu.r[4], gsu.pre_alt1 ? !gsu.cy : 0);
}

/* FxUnpackBits after its first instruction. */
static void unpack_bits_loop(void)
{
    gsu.r[13] = gsu.r[7];
    do {
        gsu.r[2] = gsu_lsr(gsu.r[2]);
        gsu.r[3] = gsu_ror(gsu.r[3]);
        gsu.r[0] = gsu_or(gsu.r[2], gsu.r[3]);
        if (gsu.z) {
            uint8_t marker = gsu.cy;

            gsu_r14(gsu_dec(gsu.r[14]));
            gsu.cy = (uint8_t)FxUnpackRefill(&gsu.r[2], &gsu.r[3]);
            gsu.r[0] = (uint16_t)(marker << 15);        /* IBT R0,#1 then ROR */
        }
        gsu.r[4] = gsu_rol(gsu.r[4]);
    } while (gsu_count_down(&gsu.r[12]));
}

/*
 * @routine 01:D0F3 FxUnpackBits
 * WHAT IT DOES: Reads R12 bits from the bit buffer into R4, the first
 * read ending as the highest.
 * HOW: Its first SUB R4 runs with the WITH R4 the caller leaves in its
 * call's delay slot, clearing R4. R13 = R7, the loop address the caller
 * sets ($D0F7, the LSR after a WITH R2, so LOOP's delay slot WITH R2
 * belongs to the next pass). Each pass: LSR R2, ROR R3 take the next bit
 * into CY; when R2:R3 is then 0 the bit was the marker, so the buffer is
 * refilled (FxUnpackRefill's code, inline after a DEC R14) and its first
 * bit taken; ROL R4 shifts the bit in. R12 = 0 reads $10000 bits.
 * Entry: R12 = the count; R7 = $D0F7; R2:R3 = the bit buffer, R14 its
 * source; R11 = return address; WITH R4 pending. Exit: R4 = the bits;
 * R2:R3 and R14 moved on; R0 = R2 | R3 after the last bit (or $8000 when
 * that bit needed a refill); R12 = 0; R13 = R7; OV from the first SUB; CY
 * from the last ROL R4; Z set, S clear; WITH R2 pending at the return.
 * R14 and the ROM buffer move as on the Super FX.
 */
void FxUnpackBits(void)
{
    first_sub_r4();
    unpack_bits_loop();
}

void abi_FxUnpackBits(void)
{
    FxUnpackBits();
}

#define FX_UNPACK_DEST 0x002Cu      /* the output's start */
#define FX_UNPACK_SRC 0x0068u       /* the packed data's end, in the ROM bank */
#define FX_UNPACK_BANK 0x006Au
#define FX_UNPACK_END 0x0060u       /* the output's end */
#define FX_UNPACK_ADD 0x00A2u       /* added to each output word when not 0 */
#define FX_UNPACK_TOP 0xCF78u       /* the command loop, R8 */
#define FX_UNPACK_BITS_LOOP 0xD0F7u /* FxUnpackBits' loop, R7 */

/* LINK #4 into FxUnpackBits with WITH R4 in the delay slot: R12 bits into
   R4. `ret` is the LINK's return address. */
static uint16_t unpack_bits(uint16_t count, uint16_t ret)
{
    gsu.r[12] = count;
    gsu.r[11] = ret;
    gsu.r[4] = gsu_sub(gsu.r[4], gsu.r[4], 0);
    unpack_bits_loop();
    return gsu.r[4];
}

/* One bit into CY: LSR R2, ROR R3, then R0 = R2 | R3; when that is 0, a
   LINK #4 into FxUnpackRefill with DEC R14 in the delay slot. */
static uint8_t unpack_bit(uint16_t ret)
{
    gsu.r[2] = gsu_lsr(gsu.r[2]);
    gsu.r[3] = gsu_ror(gsu.r[3]);
    gsu.r[0] = gsu_or(gsu.r[2], gsu.r[3]);
    if (gsu.z) {
        gsu.r[11] = ret;
        gsu_r14(gsu_dec(gsu.r[14]));
        abi_FxUnpackRefill();
    }
    return gsu.cy;
}

/* A word from the ROM bytes at R14 - 1 (low) and R14 - 2 (high), R14
   stepping down to the second. */
static uint16_t unpack_rom_word(void)
{
    uint16_t v;

    gsu_r14(gsu_dec(gsu.r[14]));
    v = gsu_getb();
    gsu_r14(gsu_dec(gsu.r[14]));
    return gsu_getbh(v);
}

/* A run of R4 bytes of 8 bits each (R4 = 0 runs $10000), each one
   shifted through R5:R6 and stored at R1 - 1. The bit reads are inline
   here: on an empty buffer the four bytes below R14 refill it. */
static void unpack_literals(void)
{
    do {
        gsu.r[12] = 8;
        gsu.r[13] = 0xCFC5;
        do {
            gsu.r[2] = gsu_lsr(gsu.r[2]);
            gsu.r[3] = gsu_ror(gsu.r[3]);
            gsu.r[0] = gsu_or(gsu.r[2], gsu.r[3]);
            if (gsu.z) {
                gsu.r[3] = unpack_rom_word();
                gsu.r[2] = unpack_rom_word();
                gsu.r[0] = gsu_ror(1);              /* CY = 1, the marker */
                gsu.r[2] = gsu_ror(gsu.r[2]);
                gsu.r[3] = gsu_ror(gsu.r[3]);
            }
            gsu.r[6] = gsu_rol(gsu.r[6]);
            gsu.r[5] = gsu_rol(gsu.r[5]);
        } while (gsu_count_down(&gsu.r[12]));
        gsu.r[1] = gsu_dec(gsu.r[1]);
        gsu_stb(gsu.r[1], (uint8_t)gsu.r[6]);
        gsu.r[4] = gsu_dec(gsu.r[4]);
    } while (!gsu.z);
}

/* A copy's length into R6 and its offset's width into R12, from the
   codes that follow a run. */
static void unpack_copy_code(void)
{
    gsu.r[4] = unpack_bits(2, 0xD02D);
    gsu.r[6] = 2;
    gsu.r[5] = 0;
    gsu.r[12] = 8;
    gsu_moves(gsu.r[4]);
    if (gsu.z)
        return;                                     /* length 2, offset 8 bits */
    gsu.r[6] = 4;
    gsu.r[0] = 2;
    gsu_sub(gsu.r[0], gsu.r[4], 0);
    if (!gsu.z) {
        gsu.r[6] = 3;
        gsu.r[0] = 1;
        gsu_sub(gsu.r[0], gsu.r[4], 0);
        if (gsu.z) {
            /* Length 3: offset of 8 bits after a 1, else 14. */
            gsu.r[12] = 8;
            if (!unpack_bit(0xD096))
                gsu.r[12] = 0x0E;
            return;
        }
        gsu.r[4] = unpack_bits(2, 0xD055);
        gsu.r[0] = 3;
        gsu_sub(gsu.r[0], gsu.r[4], 0);
        if (gsu.z) {
            gsu.r[4] = unpack_bits(8, 0xD081);      /* length of 8 bits */
        } else {
            gsu.r[0] = 2;
            gsu_sub(gsu.r[0], gsu.r[4], 0);
            if (gsu.z) {
                gsu.r[4] = unpack_bits(2, 0xD072);
                gsu.r[4] = gsu_add(gsu.r[4], 7, 0); /* 7 to 10 */
            } else {
                gsu.r[4] = gsu_add(gsu.r[4], 5, 0); /* 5 or 6 */
            }
        }
        gsu.r[6] = gsu.r[4];
    }
    /* Length 4 or more: offset of 16 bits after a 0, 8 after 1 1, 12
       after 1 0. */
    gsu.r[12] = 0x10;
    if (!unpack_bit(0xD0AF))
        return;
    gsu.r[12] = 8;
    if (unpack_bit(0xD0C2))
        return;
    gsu.r[12] = 0x0C;
}

/*
 * @routine 01:CF42 FxUnpackProgram
 * WHAT IT DOES: A program the S-CPU starts: unpacks the data that ends at
 * $70:0068 in the ROM bank at $70:006A into game pak RAM bank 0 from
 * $70:002C up, writing from the output's end down; then, when $70:00A2 is
 * not 0, adds it to the words from $70:002C up.
 * HOW: Reads backward from the packed data's end: the unpacked size (a
 * word), then two words that fill the bit buffer R2:R3. The output's end
 * (the start + the size) goes to $0060 and R1 steps down from it, one byte
 * per store. Each command (from R8 = $CF78): a run of literal bytes, its
 * length in 3 bits (0: none; 7: 4 more bits + 7, or after a 1, 10 bits,
 * or 18 when those are 0), each byte 8 bits; then, until R1 is back at the
 * start, a copy from R1 + offset - 1 down, its length from a 2-bit code
 * (0: 2; 1: 3; 2: 4; 3: 2 more bits, giving 5 or 6, 7 + 2 bits, or 8
 * bits) and its offset of 8, 12, 14 or 16 bits. Bits come through
 * FxUnpackBits and FxUnpackRefill, and inline in the literal loop. At the
 * end R14 goes back to $0068. The pass for $00A2 runs one LOOP pass per
 * byte of the size, a word each, so it covers twice the size.
 * Entry: started by the S-CPU. Exit: (at the STOP) $70:0068 = the address
 * of the last packed byte read; $70:0060 = the output's end; R9 = the
 * start; R7 = $D0F7, R8 = $CF78; R12 = 2, and R1 = the start, or after the
 * pass R12 = 0 and R1 past it; R11, R13 and the flags as the last call or
 * the pass leaves them; RAM bank 0. R14 and the ROM buffer move as on the
 * Super FX.
 */
void FxUnpackProgram(void)
{
    gsu.r[0] = gsu_ldw(FX_UNPACK_BANK);
    gsu.rombr = (uint8_t)gsu.r[0];
    gsu.r[0] = gsu_sub(gsu.r[0], gsu.r[0], 0);
    gsu.rambr = (uint8_t)gsu.r[0];
    gsu_r14(gsu_ldw(FX_UNPACK_SRC));
    gsu.r[1] = unpack_rom_word();                   /* the size */
    gsu_r14(gsu_dec(gsu.r[14]));
    gsu_r14(gsu_dec(gsu.r[14]));
    gsu.r[3] = unpack_rom_word();
    gsu.r[2] = unpack_rom_word();
    gsu.r[9] = gsu_ldw(FX_UNPACK_DEST);
    gsu.r[1] = gsu_add(gsu.r[1], gsu.r[9], 0);
    gsu_stw(FX_UNPACK_END, gsu.r[1]);
    gsu.r[4] = 0;
    gsu.r[6] = 0;
    gsu.r[5] = 0;
    gsu.r[7] = FX_UNPACK_BITS_LOOP;
    gsu.r[8] = FX_UNPACK_TOP;
    for (;;) {
        gsu.r[4] = unpack_bits(3, 0xCF7F);
        gsu.r[0] = gsu_and(0x00FF, gsu.r[4]);
        if (!gsu.z) {
            gsu.r[0] = 7;
            gsu_sub(gsu.r[0], gsu.r[4], 0);
            if (gsu.z) {
                if (unpack_bit(0xCF9E)) {
                    gsu.r[4] = unpack_bits(0x0A, 0xCFA8);
                    gsu_moves(gsu.r[4]);
                    if (gsu.z)
                        gsu.r[4] = unpack_bits(0x12, 0xCFB4);
                } else {
                    gsu.r[4] = unpack_bits(4, 0xCFBE);
                    gsu.r[4] = gsu_add(gsu.r[4], 7, 0);
                }
            }
            unpack_literals();
        }
        gsu_sub(gsu.r[9], gsu.r[1], 0);
        gsu.r[12] = 2;
        if (gsu.z)
            break;
        unpack_copy_code();
        gsu.r[4] = unpack_bits(gsu.r[12], 0xD0CC);
        gsu.r[6] = gsu_dec(gsu.r[6]);
        do {
            gsu.r[0] = gsu_add(gsu.r[1], gsu.r[4], 0);
            gsu.r[0] = gsu_dec(gsu.r[0]);
            gsu.r[0] = gsu_ldb(gsu.r[0]);
            gsu.r[1] = gsu_dec(gsu.r[1]);
            gsu_stb(gsu.r[1], (uint8_t)gsu.r[0]);
            gsu.r[6] = gsu_dec(gsu.r[6]);
        } while (!gsu.s);
    }
    gsu_stw(FX_UNPACK_SRC, gsu.r[14]);
    gsu.r[3] = gsu_ldw(FX_UNPACK_ADD);
    gsu_moves(gsu.r[3]);
    if (gsu.z)
        return;
    gsu.r[1] = gsu_ldw(FX_UNPACK_DEST);
    gsu.r[2] = gsu_ldw(FX_UNPACK_END);
    gsu.r[12] = gsu_sub(gsu.r[2], gsu.r[1], 0);
    gsu.r[13] = 0xD01F;
    do {
        int again;

        gsu.r[0] = gsu_ldw(gsu.r[1]);
        gsu.r[0] = gsu_add(gsu.r[0], gsu.r[3], 0);
        gsu_stw(gsu.r[1], gsu.r[0]);
        gsu.r[1] = gsu_inc(gsu.r[1]);
        again = gsu_count_down(&gsu.r[12]);
        gsu.r[1] = gsu_inc(gsu.r[1]);              /* LOOP's delay slot */
        if (!again)
            break;
    } while (1);
}

void abi_FxUnpackProgram(void)
{
    FxUnpackProgram();
}
