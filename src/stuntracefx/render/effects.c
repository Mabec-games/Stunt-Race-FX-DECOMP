/*
 * effects.c: the Super FX's short-lived screen effects. Sixteen records of
 * six bytes at $2688: +$00 the time left (0: the record is free), +$01 a
 * byte, +$02 X and +$04 Y on the screen (words).
 */
#include "gsu.h"
#include "gsu_ops.h"

#define FX_EFFECTS 0x2688u
#define FX_EFFECT_SIZE 6u
#define FX_EFFECT_COUNT 0x10u
#define FX_EDGES 0x0038u        /* the screen's edges: X at $38 and $3A, Y at $3C and $3E */
#define FX_CENTRE_X 0x0034u
#define FX_TIED_RECORD 0x247Au  /* the address of the tied object's record */

/*
 * @routine 01:C060 FxEffectsAge
 * WHAT IT DOES: Counts down the time of each effect in use and frees the
 * ones outside the screen less a margin of 8: X below $38 + 8 or not below
 * $3A - 8, Y below $3C + 8 or not below $3E - 8.
 * HOW: The margins into R8, R7, R4 and R5 (ADD #8, SUB #8). R6 = 8 times
 * the word at +$4A of the record at $247A, or 0 when that is negative
 * (nothing here uses it). A LOOP over the sixteen records, R13 = its
 * start ($C095), R3 stepping by 6: a record whose time (LDB, LOB) is not
 * 0 has it counted down (DEC, STB); then R2 = its byte, R9 = $0034
 * (unused), R1 = X, R2 = Y, and CMP against each margin; one outside has
 * its first word set to 0. WITH then TO is a MOVE, WITH then FROM a
 * MOVES. Returns by a MOVE of R11 to R15.
 * Entry: R11 = return address. Exit: R3 = $26E8; R12 = 0; R13 = $C095;
 * R8, R7, R4, R5 the margins; R6 as above; R0, R1, R2, R9 and the OV and
 * CY flags as the last record in use, or the last free record, left them;
 * S clear, Z set (LOOP).
 */
void FxEffectsAge(int copy)
{
    
    gsu_code(0x01c061, 3);
    gsu.r[0] = gsu_ldw(FX_EDGES);
    gsu.r[8] = gsu_add(gsu.r[0], 8, 0);
    gsu_code(0x01c064, 6);
    gsu.r[0] = gsu_ldw(FX_EDGES + 2);
    gsu.r[7] = gsu_sub(gsu.r[0], 8, 0);
    gsu_code(0x01c06a, 6);
    gsu.r[0] = gsu_ldw(FX_EDGES + 4);
    gsu.r[4] = gsu_add(gsu.r[0], 8, 0);
    gsu_code(0x01c070, 6);
    gsu.r[0] = gsu_ldw(FX_EDGES + 6);
    gsu.r[5] = gsu_sub(gsu.r[0], 8, 0);
    gsu.r[6] = 0x0000;
    gsu_code(0x01c076, 10);
    gsu.r[1] = gsu_ldw(FX_TIED_RECORD);
    gsu.r[0] = gsu_add(0x004A, gsu.r[1], 0);
    gsu_code(0x01c080, 4);
    gsu.r[0] = gsu_ldw(gsu.r[0]);
    gsu_moves(gsu.r[0]);
    if (!gsu.s) {
        gsu.r[0] = gsu_add(gsu.r[0], gsu.r[0], 0);
        gsu.r[0] = gsu_add(gsu.r[0], gsu.r[0], 0);
        gsu.r[0] = gsu_add(gsu.r[0], gsu.r[0], 0);
        gsu.r[6] = gsu.r[0];
        gsu_code(0x01c084, 10);
    } else { gsu_code(0x01c084, 4); gsu_code(0x01c08d, 1); }
    gsu.r[3] = FX_EFFECTS;
    gsu.r[12] = FX_EFFECT_COUNT;
    gsu.r[13] = 0xC095;
    gsu_code(0x01c08e, 8);
    for (;;) {                                  /* each way its own LOOP */
        gsu_code(0x01c096, 2);
        gsu.r[0] = gsu_lob(gsu_ldb(gsu.r[3]));
        gsu_moves(gsu.r[0]);
        if (gsu.z) {
            gsu.r[3] = gsu_add(gsu.r[3], FX_EFFECT_SIZE, 0);
            if (!gsu_count_down(&gsu.r[12]))
                { 
                gsu_code(0x01c098, 5); gsu_code(0x01c0d8, 6);
                break; } else { gsu_code(0x01c098, 5); gsu_code(0x01c0d8, 5); gsu_code(0x01c095, 1); }
            continue;
        } else { gsu_code(0x01c098, 5); }
        gsu.r[0] = gsu_dec(gsu.r[0]);
        gsu_code(0x01c09d, 4);
        gsu_stb(gsu.r[3], (uint8_t)gsu.r[0]);
        gsu.r[3] = gsu_inc(gsu.r[3]);
        gsu_code(0x01c0a1, 4);
        gsu.r[2] = gsu_ldb(gsu.r[3]);
        gsu.r[3] = gsu_inc(gsu.r[3]);
        gsu_code(0x01c0a5, 4);
        gsu.r[9] = gsu_ldw(FX_CENTRE_X);
        gsu_code(0x01c0a9, 2);
        gsu.r[1] = gsu_ldw(gsu.r[3]);
        gsu.r[3] = gsu_inc(gsu_inc(gsu.r[3]));
        gsu.r[0] = gsu_lob(gsu.r[2]);
        gsu_code(0x01c0ab, 7);
        gsu.r[2] = gsu_ldw(gsu.r[3]);
        gsu.r[3] = gsu_inc(gsu_inc(gsu.r[3]));
        gsu.r[0] = gsu.r[1];
        gsu_sub(gsu.r[0], gsu.r[8], 0);         /* off the screen: on to outside */
        if (gsu.s)
            { 
            gsu_code(0x01c0b2, 8); gsu_code(0x01c0cb, 1);
            goto outside; } else { gsu_code(0x01c0b2, 8); }
        gsu_sub(gsu.r[0], gsu.r[7], 0);
        if (!gsu.s)
            { 
            gsu_code(0x01c0ba, 5); gsu_code(0x01c0cb, 1);
            goto outside; } else { gsu_code(0x01c0ba, 5); }
        gsu.r[0] = gsu.r[2];
        gsu_sub(gsu.r[0], gsu.r[4], 0);
        if (gsu.s)
            { 
            gsu_code(0x01c0bf, 7); gsu_code(0x01c0cb, 1);
            goto outside; } else { gsu_code(0x01c0bf, 7); }
        gsu_sub(gsu.r[0], gsu.r[5], 0);
        if (gsu.s)
            { 
            gsu_code(0x01c0c6, 5); gsu_code(0x01c0d3, 1);
            goto next; } else { gsu_code(0x01c0c6, 5); }
gsu_code(0x01c0cb, 1);
outside:
        gsu.r[3] = gsu_sub(gsu.r[3], FX_EFFECT_SIZE, 0);
        gsu.r[0] = gsu_sub(gsu.r[0], gsu.r[0], 0);
        gsu_code(0x01c0cc, 5);
        gsu_stw(gsu.r[3], gsu.r[0]);
        gsu.r[3] = gsu_add(gsu.r[3], FX_EFFECT_SIZE, 0);
gsu_code(0x01c0d1, 3);
next:
        if (!gsu_count_down(&gsu.r[12]))
            { 
            gsu_code(0x01c0d4, 4); gsu_code(0x01c0dd, 1);
            break; } else { gsu_code(0x01c0d4, 1); gsu_code(0x01c095, 1); }
    }
    gsu_code(0x01c0de, 2);
}

void abi_FxEffectsAge(void)
{
    FxEffectsAge(GSU_COPY(0));
}

#define FX_EFFECT_FLAGS 0x2736u /* bit 14: the haze effects are on */
#define FX_EFFECT_MAX 0x26EEu   /* the most effects started at once */
#define FX_RANDOM 0x01D0u       /* the effects' random seed */
#define FX_HAZE_X 0x26F6u       /* the sample's left edge */
#define FX_HAZE_X8 0x26F8u      /* the left edge less 8: where it goes back */
#define FX_HAZE_COUNT 0x0340u   /* the pixels saved, then four bytes each */
#define FX_HAZE_PIXELS 0x0342u
#define FX_HAZE_SHAPES 0xF92Eu  /* $08:F92E: per time, a row-offset list and a shape */

/* COLOR: the plot colour from a byte, as the plot options shape it (high
   nibble from the source's high nibble, or the high nibble kept). */
static void fx_color(uint8_t v)
{
    if (gsu.por & 0x04)
        { gsu.colr = (uint8_t)((gsu.colr & 0xF0) | v >> 4); gsu_code(0x01c04d, 7); }
    else if (gsu.por & 0x08)
        { gsu.colr = (uint8_t)((gsu.colr & 0xF0) | (v & 0x0F)); gsu_code(0x01c04d, 7); }
    else
        { gsu.colr = v; gsu_code(0x01c04d, 7); }
}

/* The random step: SWAP of the seed, ROR (through the carry), ADD the
   seed into R0; then the seed = R0 + the seed + carry (ADC). */
static void random_step(int copy)
{
    gsu.r[0] = gsu_add(gsu_ror(gsu_swap(gsu.r[4])), gsu.r[4], 0);
    gsu.r[4] = gsu_add(gsu.r[0], gsu.r[4], gsu.cy);
}

/*
 * @routine 01:C130 FxEffectsSpawn
 * WHAT IT DOES: Starts an effect in each free one of the first R0 effect
 * records: time 8, its byte 1 or 2 at random, and a random X and Y (the
 * low and high bytes of a random word).
 * HOW: The seed from $01D0 in R4; a LOOP of R0 records from $2688 (R13 =
 * $C13A), each taken by LDB, LOB; a record in use is skipped with ADD #6.
 * Each random number is one random step (SWAP, ROR, ADD, ADC). The seed
 * goes back to $01D0; returns with MOVE R15,R11.
 * Entry: R0 = the number of records to look at; R11 = return address.
 * Exit: R4 = the seed; R1 past the last record looked at; R12 = 0; R13 =
 * $C13A; R0, R2, R3 as the last record left them; S, Z from the LOOP
 * (Z set), OV and CY from the last ADD or ADC; the last RAM address $01D0.
 */
void abi_FxEffectsSpawn(void)
{
    gsu_code(0x01c131, 3);
    gsu.r[4] = gsu_ldw(FX_RANDOM);
    gsu.r[1] = FX_EFFECTS;
    gsu.r[12] = gsu.r[0];
    gsu.r[13] = 0xC13A;
    gsu_code(0x01c134, 7);
    for (;;) {                                  /* each way its own LOOP */
        gsu_code(0x01c13b, 2);
        gsu.r[0] = gsu_lob(gsu_ldb(gsu.r[1]));
        if (!gsu.z) {
            gsu.r[1] = gsu_add(gsu.r[1], FX_EFFECT_SIZE, 0);
            if (!gsu_count_down(&gsu.r[12]))
                { 
                gsu_code(0x01c13d, 3); gsu_code(0x01c16a, 6);
                break; } else { gsu_code(0x01c13d, 3); gsu_code(0x01c16a, 5); gsu_code(0x01c13a, 1); }
            continue;
        } else { gsu_code(0x01c13d, 3); }
        gsu.r[0] = 8;
        gsu_code(0x01c140, 5);
        gsu_stb(gsu.r[1], (uint8_t)gsu.r[0]);
        gsu.r[1] = gsu_inc(gsu.r[1]);
        random_step(GSU_COPY(0));
        gsu.r[0] = gsu_and(gsu.r[0], 1);
        gsu.r[0] = gsu_inc(gsu.r[0]);
        gsu_code(0x01c145, 13);
        gsu_stb(gsu.r[1], (uint8_t)gsu.r[0]);
        gsu.r[1] = gsu_inc(gsu.r[1]);
        random_step(GSU_COPY(1));
        gsu.r[2] = gsu_lob(gsu.r[0]);
        gsu.r[3] = gsu_hib(gsu.r[0]);
        gsu_code(0x01c152, 14);
        gsu_stw(gsu.r[1], gsu.r[2]);
        gsu.r[1] = gsu_inc(gsu_inc(gsu.r[1]));
        gsu_code(0x01c160, 4);
        gsu_stw(gsu.r[1], gsu.r[3]);
        gsu.r[1] = gsu_inc(gsu_inc(gsu.r[1]));
        if (!gsu_count_down(&gsu.r[12]))
            { 
            gsu_code(0x01c164, 6); gsu_code(0x01c16f, 1);
            break; } else { gsu_code(0x01c164, 3); gsu_code(0x01c13a, 1); }
    }
    gsu_code(0x01c170, 3);
    gsu_stw(FX_RANDOM, gsu.r[4]);
    gsu_code(0x01c173, 2);
}

/*
 * @routine 01:BFB7 FxHazeSample
 * WHAT IT DOES: Reads the pixels of a 16 x 16 shape at (R1, R2), each row
 * moved down or up by its own offset, and lists the ones the shape covers
 * at $0342 with where each goes back: X - 8 + its column, Y - 8 + its row
 * (the pixel word, then those two bytes). $0340 counts them.
 * HOW: The shape for the time R0 is found at $08:F92E + 4 * R0: a word
 * pointing to sixteen signed row offsets and one pointing to the shape's
 * 256 bytes (ROM bank 8). CACHE; sixteen rows (R7, BNE), each with a LOOP
 * of sixteen columns (R12, R13 = $C008): a shape byte (GETB, SEX) of 0
 * skips the pixel; otherwise its value moves X and RPIX reads the pixel
 * at that X and the row's Y. R5 and R6 step the return position (INC in
 * the delay slots). The count, (R11 - $0342) / 4 by two DIV2s, is added
 * to $0340 by SBK. Returns by pulling R15.
 * Entry: R0 = the time; R1 = X, R2 = Y; R10 = stack; R11 = return address.
 * Exit: R11 past the list; R7 = R12 = 0; R1 = $0340; R2 its old value; R0
 * = the new count; R3 = X, R4 = the last row's Y, R5, R6 past the last
 * return position, R8 past the row offsets, R9 = Y; R14 past the shape; the
 * ROM bank 8; S and Z from R10; the last RAM address R10. R14 and the ROM
 * buffer move as on the Super FX.
 */
void abi_FxHazeSample(void)
{
    
    int skip, more;

    gsu_code(0x01bfb8, 2);
    gsu_push(&gsu.r[10], gsu.r[11]);
    gsu_code(0x01bfba, 6);
    gsu_stw(FX_HAZE_X, gsu.r[1]);
    gsu.r[1] = gsu_sub(gsu.r[1], 8, 0);
    gsu_code(0x01bfc0, 7);
    gsu_stw(FX_HAZE_X8, gsu.r[1]);
    gsu.r[9] = gsu.r[2];
    gsu.r[6] = gsu.r[2];
    gsu.r[6] = gsu_sub(gsu.r[6], 8, 0);
    gsu.r[0] = gsu_add(gsu.r[0], gsu.r[0], 0);
    gsu.r[1] = gsu_add(gsu.r[0], gsu.r[0], 0);
    gsu.r[0] = 0x0008;
    
    gsu_code(0x01bfc7, 14);
    gsu_romb(0x08);
    gsu_code(0x01bfd5, 3);
    gsu_r14(FX_HAZE_SHAPES);
    gsu_code(0x01bfd8, 2);
    gsu_r14(gsu_add(gsu.r[14], gsu.r[1], 0));
    gsu_code(0x01bfda, 2);
    gsu.r[1] = gsu_getb();
    gsu_code(0x01bfdc, 1);
    gsu_r14(gsu_inc(gsu.r[14]));
    gsu_code(0x01bfdd, 3);
    gsu.r[1] = gsu_getbh(gsu.r[1]);
    gsu_code(0x01bfe0, 1);
    gsu_r14(gsu_inc(gsu.r[14]));
    gsu_code(0x01bfe1, 2);
    gsu.r[8] = gsu_getb();
    gsu_code(0x01bfe3, 1);
    gsu_r14(gsu_inc(gsu.r[14]));
    gsu_code(0x01bfe4, 3);
    gsu.r[8] = gsu_getbh(gsu.r[8]);
    gsu_code(0x01bfe7, 1);
    gsu_r14(gsu_inc(gsu.r[14]));
    gsu_code(0x01bfe8, 2);
    gsu_r14(gsu.r[1]);
    gsu.r[7] = 0x0010;
    gsu.r[11] = FX_HAZE_PIXELS;
    
    
    gsu_code(0x01bfea, 6); gsu_cache(0xbff0); gsu_code(0x01bff0, 1);
    do {
        gsu.r[1] = gsu.r[14];
        gsu_code(0x01bff1, 4);
        gsu_r14(gsu.r[8]);
        gsu_code(0x01bff5, 2);
        gsu.r[0] = (gsu_getbs());
        gsu.r[8] = gsu_inc(gsu.r[8]);
        gsu.r[4] = gsu_add(gsu.r[0], gsu.r[9], 0);
        gsu_code(0x01bff7, 5);
        gsu_r14(gsu.r[1]);
        gsu_code(0x01bffc, 4);
        gsu.r[3] = gsu_ldw(FX_HAZE_X);
        gsu_code(0x01c000, 4);
        gsu.r[5] = gsu_ldw(FX_HAZE_X8);
        gsu.r[12] = 0x0010;
        gsu.r[13] = 0xC008;
        gsu_code(0x01c004, 5);
        do {
            gsu_code(0x01c009, 1);
            gsu.r[0] = (uint16_t)(int16_t)(int8_t)gsu_getb();      /* GETB, SEX */
            gsu_sz(gsu.r[0]);
            skip = gsu.z;
            /* the branch's delay slot */
            /* the branch's delay slot */
            gsu_code(0x01c00a, 3); gsu_code((skip) ? 0x01c01e : 0x01c00d, 1);   /* the branch's delay slot */
            gsu_r14(gsu_inc(gsu.r[14]));    /* BEQ's delay slot */
            if (!skip) {
                gsu.r[1] = gsu_add(gsu.r[0], gsu.r[3], 0);
                gsu.r[2] = gsu.r[4];
                gsu_code(0x01c00e, 6);
                gsu.r[0] = gsu_rpix((uint8_t)gsu.r[1], (uint8_t)gsu.r[2]);
                gsu_sz(gsu.r[0]);
                gsu_code(0x01c014, 1);
                gsu_stw(gsu.r[11], gsu.r[0]);
                gsu.r[11] = gsu_inc(gsu_inc(gsu.r[11]));
                gsu_code(0x01c015, 5);
                gsu_stb(gsu.r[11], (uint8_t)gsu.r[5]);
                gsu.r[11] = gsu_inc(gsu.r[11]);
                gsu_code(0x01c01a, 4);
                gsu_stb(gsu.r[11], (uint8_t)gsu.r[6]);
                gsu.r[11] = gsu_inc(gsu.r[11]);
                gsu_code(0x01c01e, 1);
            }
            more = gsu_count_down(&gsu.r[12]);
            gsu.r[5] = gsu_inc(gsu.r[5]);   /* LOOP's delay slot */
        } while (gsu_code_loop(0x01c008, 1, gsu_code_loop(0x01c01f, 1, more)));
        gsu_code(0x01c01f, 1);
        gsu.r[7] = gsu_dec(gsu.r[7]);
        more = !gsu.z;
        gsu.r[6] = gsu_inc(gsu.r[6]);       /* BNE's delay slot */
    } while (gsu_code_loop(0x01bff0, 1, gsu_code_loop(0x01c020, 4, more)));
    gsu_code(0x01c020, 4);
    gsu.r[1] = FX_HAZE_PIXELS;
    gsu.r[0] = gsu_sub(gsu.r[11], gsu.r[1], 0);
    gsu.r[0] = gsu_div2(gsu.r[0]);
    gsu.r[0] = gsu_div2(gsu.r[0]);
    gsu.r[1] = FX_HAZE_COUNT;
    gsu_code(0x01c024, 15);
    gsu.r[2] = gsu_ldw(gsu.r[1]);
    gsu.r[0] = gsu_add(gsu.r[0], gsu.r[2], 0);
    gsu_code(0x01c033, 2);
    gsu_sbk(gsu.r[0]);
    gsu_code(0x01c035, 4);
    gsu_pull(&gsu.r[10]);       /* the return address, into R15 */
    
}

/*
 * @routine 01:C039 FxHazePlot
 * WHAT IT DOES: Plots back the pixels FxHazeSample listed, each at its
 * return position, so the shape's patch of screen is drawn again moved:
 * a shimmer.
 * HOW: The count from $0340; none returns at once. CMODE 1 (colour 0 is
 * plotted); CACHE; a LOOP (R13 = $C04B) of COLOR from the pixel word, R1
 * and R2 from the two bytes (LDB) and PLOT in the delay slot. Returns
 * with MOVE R15,R11.
 * Entry: R11 = return address. Exit: R3 past the list; R1 one past the
 * last X (PLOT steps it), R2 the last Y; R12 = 0; R13 = $C04B; R0 the
 * last pixel word; the plot options 1; flags from the LOOP; the last RAM
 * address the last Y's.
 */
void abi_FxHazePlot(void)
{
    gsu.r[3] = FX_HAZE_COUNT;
    gsu_code(0x01c03a, 4);
    gsu.r[0] = gsu_ldw(gsu.r[3]);
    gsu.r[3] = gsu_inc(gsu_inc(gsu.r[3]));
    gsu.r[12] = gsu.r[0];
    gsu_moves(gsu.r[0]);
    if (gsu.z)
        { 
        gsu_code(0x01c03e, 6); gsu_code(0x01c05d, 3);
        return; } else { gsu_code(0x01c03e, 6); }
    gsu.r[0] = 0x0001;
    gsu.por = (uint8_t)gsu.r[0];
    gsu.r[13] = 0xC04B;
    
    
    gsu_code(0x01c044, 5); gsu_cache(0xc049); gsu_code(0x01c049, 3);
    do {
        int more;

        gsu_code(0x01c04c, 1);
        gsu.r[0] = gsu_ldw(gsu.r[3]);
        gsu.r[3] = gsu_inc(gsu_inc(gsu.r[3]));
        fx_color((uint8_t)gsu.r[0]);
        gsu.r[1] = gsu_sub(gsu.r[0], gsu.r[0], 0);
        gsu.r[2] = gsu.r[1];
        gsu_code(0x01c054, 3);
        gsu.r[1] = gsu_ldb(gsu.r[3]);
        gsu.r[3] = gsu_inc(gsu.r[3]);
        gsu_code(0x01c057, 4);
        gsu.r[2] = gsu_ldb(gsu.r[3]);
        gsu.r[3] = gsu_inc(gsu.r[3]);
        more = gsu_count_down(&gsu.r[12]);
        /* the branch's delay slot */
        /* the branch's delay slot */
        gsu_code(0x01c05b, 2); gsu_code((more) ? 0x01c04b : 0x01c05d, 1);   /* the branch's delay slot */
        gsu_plot((uint8_t)gsu.r[1], (uint8_t)gsu.r[2]);     /* LOOP's delay slot */
        gsu.r[1] = (uint16_t)(gsu.r[1] + 1);
        if (!more)
            { break; }
    } while (1);
    gsu_code(0x01c05e, 2);
}

/*
 * @routine 01:C175 FxEffectsDraw
 * WHAT IT DOES: Draws each effect in use: FxHazeSample at its X and Y
 * with its time, then FxHazePlot.
 * HOW: Pushes R11 on the R10 stack; sixteen records from $2688 (R4
 * counting, BNE), R4 and R3 kept on the R10 stack around each. For a
 * record in use (its time by LDB, LOB, MOVES) $0340 is cleared first.
 * Returns by pulling R15.
 * Entry: R10 = stack; R11 = return address. Exit: R3 = $26E8, R4 = 0;
 * the rest as the last effect drawn leaves them; S and Z from R10; the
 * last RAM address R10.
 */
void abi_FxEffectsDraw(void)
{
    gsu_code(0x01c176, 2);
    gsu_push(&gsu.r[10], gsu.r[11]);
    gsu.r[3] = FX_EFFECTS;
    gsu.r[4] = FX_EFFECT_COUNT;
    gsu_code(0x01c178, 8);
    do {
        gsu_code(0x01c180, 2);
        gsu_push(&gsu.r[10], gsu.r[4]);
        gsu_code(0x01c182, 4);
        gsu_push(&gsu.r[10], gsu.r[3]);
        gsu_code(0x01c186, 4);
        gsu.r[0] = gsu_lob(gsu_ldb(gsu.r[3]));
        gsu_moves(gsu.r[0]);
        if (!gsu.z) {
            gsu.r[1] = FX_HAZE_COUNT;
            gsu.r[2] = 0;
            gsu_code(0x01c18a, 13);
            gsu_stw(gsu.r[1], gsu.r[2]);
            gsu.r[3] = gsu_inc(gsu_inc(gsu.r[3]));
            gsu_code(0x01c197, 4);
            gsu.r[1] = gsu_ldw(gsu.r[3]);
            gsu.r[3] = gsu_inc(gsu_inc(gsu.r[3]));
            gsu_code(0x01c19b, 4);
            gsu.r[2] = gsu_ldw(gsu.r[3]);
            gsu.r[3] = gsu_inc(gsu_inc(gsu.r[3]));
            gsu.r[11] = 0xC1A5;
            
            
            gsu_code(0x01c19f, 6); gsu_code(0x01bfb7, 1);
            abi_FxHazeSample();
            gsu.r[11] = 0xC1AA;
            
            
            gsu_code(0x01c1a5, 5); gsu_code(0x01c039, 1);
            abi_FxHazePlot();
            gsu_code(0x01c1aa, 1);
        } else { gsu_code(0x01c18a, 5); gsu_code(0x01c1aa, 1); }
        gsu_code(0x01c1ab, 4);
        gsu.r[3] = gsu_pull(&gsu.r[10]);
        gsu_code(0x01c1af, 4);
        gsu.r[4] = gsu_pull(&gsu.r[10]);
        gsu.r[3] = gsu_add(gsu.r[3], FX_EFFECT_SIZE, 0);
        gsu.r[4] = gsu_dec(gsu.r[4]);
    } while (gsu_code_loop(0x01c17f, 1, gsu_code_loop(0x01c1b3, 6, !gsu.z)));
    gsu_code(0x01c1b3, 6);
    gsu_code(0x01c1b9, 5);
    gsu_pull(&gsu.r[10]);       /* the return address, into R15 */
}

/*
 * @routine 01:BF6D FxEffectsRun
 * WHAT IT DOES: With bit 14 of $2736 set, starts new effects (as many as
 * the tied object's speed, +$4A of the record at $247A, over 8, at most
 * $26EE; none when it is negative), ages them and draws them.
 * HOW: Pushes R11 on the R10 stack; three DIV2s; CMP against $26EE keeps
 * the smaller. FxEffectsSpawn (when there are any to start),
 * FxEffectsAge and FxEffectsDraw. Returns by pulling R15.
 * Entry: R10 = stack; R11 = return address. Exit: as FxEffectsDraw, or
 * with the bit clear R0 = 0, R1 = $2736, Z set by the AND then S and Z
 * from R10; the last RAM address R10.
 */
void abi_FxEffectsRun(void)
{
    gsu_code(0x01bf6e, 2);
    gsu_push(&gsu.r[10], gsu.r[11]);
    gsu_code(0x01bf70, 6);
    gsu.r[1] = gsu_ldw(FX_EFFECT_FLAGS);
    gsu.r[0] = 0x4000;
    gsu.r[0] = gsu_and(gsu.r[0], gsu.r[1]);
    if (!gsu.z) {
        gsu.r[2] = 0;
        gsu_code(0x01bf76, 13);
        gsu.r[1] = gsu_ldw(FX_TIED_RECORD);
        gsu.r[0] = 0x004A;
        gsu.r[0] = gsu_add(gsu.r[0], gsu.r[1], 0);
        gsu_code(0x01bf83, 4);
        gsu.r[0] = gsu_ldw(gsu.r[0]);
        gsu_moves(gsu.r[0]);
        if (!gsu.s) {
            gsu.r[0] = gsu_div2(gsu.r[0]);
            gsu.r[0] = gsu_div2(gsu.r[0]);
            gsu.r[0] = gsu_div2(gsu.r[0]);
            gsu.r[2] = gsu.r[0];
            gsu_code(0x01bf87, 17);
            gsu.r[0] = gsu_ldw(FX_EFFECT_MAX);
            gsu_sub(gsu.r[0], gsu.r[2], 0);     /* CMP */
            if (gsu.s)
                { gsu.r[2] = gsu.r[0]; gsu_code(0x01bf98, 7); } else { gsu_code(0x01bf98, 4); gsu_code(0x01bf9e, 1); }
        } else { gsu_code(0x01bf87, 4); gsu_code(0x01bf9e, 1); }
        gsu.r[0] = gsu.r[2];
        gsu_moves(gsu.r[0]);
        if (!gsu.z) {
            gsu.r[11] = 0xBFA8;
            
            
            gsu_code(0x01bf9f, 9); gsu_code(0x01c130, 1);
            abi_FxEffectsSpawn();
            gsu_code(0x01bfa8, 1);
        } else { gsu_code(0x01bf9f, 4); gsu_code(0x01bfa8, 1); }
        gsu.r[11] = 0xBFAD;
        
        
        gsu_code(0x01bfa9, 4); gsu_code(0x01c060, 1);
        abi_FxEffectsAge();
        gsu.r[11] = 0xBFB2;
        
        
        gsu_code(0x01bfad, 5); gsu_code(0x01c175, 1);
        abi_FxEffectsDraw();
        gsu_code(0x01bfb2, 1);
    } else { gsu_code(0x01bf76, 6); gsu_code(0x01bfb2, 1); }
    gsu_code(0x01bfb3, 4);
    gsu_pull(&gsu.r[10]);       /* the return address, into R15 */
}
