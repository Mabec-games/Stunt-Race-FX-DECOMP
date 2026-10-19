/*
 * sort.c: the Super FX's draw list. Each frame the objects' slots at $0AAA
 * ($1E bytes each, the count's low byte at $01C8) are turned into view
 * space, culled, and linked into one list in depth order, the farthest
 * first, for the renderer to paint back to front.
 *
 * A slot: +$00 the next slot in the list (0 ends it), +$02 the depth key,
 * +$07 flags (bit 7: already in view space), +$08 the object's shape in
 * ROM bank 0, +$10, +$12, +$14 the position (Y, X, Z once in view space).
 * The shape gives the object's radius at +$10 and a depth bias at +$05.
 * The list's head is the word at $023C.
 */
#include "gsu.h"
#include "gsu_ops.h"

#define FX_SLOTS 0x0AAAu
#define FX_SLOT_SIZE 0x1Eu
#define FX_SLOT_COUNT 0x01C8u
#define FX_LIST_HEAD 0x023Cu
#define FX_MATRIX 0x00E4u

/* R8 = the slot's field at offset (IBT R8, WITH R8, ADD R9). */
static uint16_t field(uint8_t offset, int copy)
{
    gsu.r[8] = gsu_add(offset, gsu.r[9], 0);
    return gsu.r[8];
}

/* One element of the rotated position: R1, R2, R3 times a column of the
   matrix (FMULT, ROL, summed in R5 then R0). */
/* gsuwalk: the code of each copy of rotate_column, read from the ROM */
static const GsuCode rotate_column_fetch[3][3] = {
    {{0x01af2f, 3}, {0x01af32, 7}, {0x01af39, 8}},
    {{0x01af4a, 3}, {0x01af4d, 7}, {0x01af54, 8}},
    {{0x01af65, 3}, {0x01af68, 7}, {0x01af6f, 8}},
};

static uint16_t rotate_column(int col, int copy)
{
    gsu_codes(rotate_column_fetch[copy], 1);
    gsu.r[6] = gsu_ldw((uint16_t)(FX_MATRIX + col * 2));
    gsu.r[5] = gsu_rol(gsu_fmult(gsu.r[1], gsu.r[6]));
    gsu_codes(rotate_column_fetch[copy] + 1, 1);
    gsu.r[6] = gsu_ldw((uint16_t)(FX_MATRIX + 6 + col * 2));
    gsu.r[0] = gsu_rol(gsu_fmult(gsu.r[2], gsu.r[6]));
    gsu.r[5] = gsu_add(gsu.r[5], gsu.r[0], 0);
    gsu_codes(rotate_column_fetch[copy] + 2, 1);
    gsu.r[6] = gsu_ldw((uint16_t)(FX_MATRIX + 12 + col * 2));
    gsu.r[0] = gsu_rol(gsu_fmult(gsu.r[3], gsu.r[6]));
    return gsu.r[0] = gsu_add(gsu.r[0], gsu.r[5], 0);
}

/* |v| as MOVES, then NOT and INC when negative. */
/* gsuwalk: the code of each copy of absolute, read from the ROM */
static const GsuCode absolute_fetch[2][3] = {
    {{0x01af9e, 8}, {0x01af9e, 4}, {0x01afa5, 1}},
    {{0x01afab, 7}, {0x01afab, 4}, {0x01afb1, 1}},
};

static uint16_t absolute(uint16_t v, int copy)
{
    gsu_moves(v);
    return gsu.s ? (gsu_codes(absolute_fetch[copy], 1), ((gsu_inc(gsu_not(v))))) : (gsu_codes(absolute_fetch[copy] + 1, 2), (v));
}

/* Links slot R9 (depth key R1) into the list: walks it from the head with
   R7, R2 the slot before, and puts R9 before the first slot whose key is
   not above R1. Leaves R7 = the slot after R9. */
static void link_slot(void)
{
    gsu.r[2] = 0;
    gsu_code(0x01afd7, 8);
    gsu.r[7] = gsu_ldw(FX_LIST_HEAD);
    gsu_moves(gsu.r[7]);
    if (gsu.z) {
        gsu_code(0x01afdf, 9);
        gsu_stw(FX_LIST_HEAD, gsu.r[9]);
        gsu.r[7] = gsu.r[2];
        
        gsu_code(0x01afe8, 4); gsu_code(0x01b013, 1);
        return;
    } else { gsu_code(0x01afdf, 4); gsu_code(0x01afec, 1); }
    for (;;) {
        int above;

        gsu.r[7] = gsu_inc(gsu_inc(gsu.r[7]));
        gsu_code(0x01afed, 3);
        gsu.r[0] = gsu_ldw(gsu.r[7]);
        gsu.r[7] = gsu_dec(gsu.r[7]);
        gsu.r[0] = gsu_dec(gsu.r[0]);
        gsu_sub(gsu.r[0], gsu.r[1], 0);
        above = !gsu.s;                     /* key - 1 - R1 >= 0 */
        gsu.r[7] = gsu_dec(gsu.r[7]);       /* BPL's delay slot */
        if (!above) {
            gsu_moves(gsu.r[2]);
            if (gsu.z)
                { 
                gsu_code(0x01aff0, 16);
                gsu_stw(FX_LIST_HEAD, gsu.r[9]); gsu_code(0x01b000, 2); gsu_code(0x01b013, 1); }
            else
                { 
                gsu_code(0x01aff0, 11); gsu_code(0x01b002, 3);
                gsu_stw(gsu.r[2], gsu.r[9]); gsu_code(0x01b005, 2); gsu_code(0x01b013, 1); }
            return;
        } else { gsu_code(0x01aff0, 6); gsu_code(0x01b007, 1); }
        gsu.r[2] = gsu.r[7];
        gsu_code(0x01b008, 4);
        gsu.r[7] = gsu_ldw(gsu.r[7]);
        gsu_moves(gsu.r[7]);
        if (gsu.z) {
            
            
            gsu_code(0x01b00c, 7); gsu_code(0x01b002, 3);
            gsu_stw(gsu.r[2], gsu.r[9]);
            
            gsu_code(0x01b005, 2); gsu_code(0x01b013, 1);
            return;
        } else { gsu_code(0x01b00c, 4); gsu_code(0x01afec, 1); }
    }
}

/*
 * @routine 01:AEF8 FxSortObjectsProgram
 * WHAT IT DOES: A program the S-CPU starts: builds the draw list of the
 * object slots at $0AAA, the farthest first, leaving out those behind the
 * view or outside its sides.
 * HOW: Sets the ROM bank to 0 and the list head ($023C) to 0. Stops at
 * once when the count's low byte is 0, else CACHE and a LOOP over the
 * slots. A slot without flag bit 7 has its position multiplied by the
 * matrix at $00E4 (as FxMatrixApply, written back to +$12, +$10, +$14).
 * The radius (shape +$10, GETB/GETBH) culls it when Z + radius is
 * negative or |X| - radius is not below |Z|. Otherwise its key is the
 * old key + Z + the shape's bias (+$05), stored at +$02, and it is linked
 * in before the first slot whose key is not above it (+$00). Then STOP.
 * Entry: started by the S-CPU. Exit: (at the STOP) R9 past the last slot;
 * R12 = 0; R13 = $AF0F (the slot loop); the other registers, the flags
 * and the last RAM address as the last slot left them; the ROM bank 0.
 * R14 and the ROM buffer move as on the Super FX.
 */
void FxSortObjectsProgram(void)
{
    gsu.r[0] = gsu_sub(gsu.r[0], gsu.r[0], 0);
    gsu_code(0x01aef9, 3);
    gsu_romb(0);
    gsu_code(0x01aefc, 4);
    gsu_stw(FX_LIST_HEAD, gsu.r[0]);
    gsu.r[9] = FX_SLOTS;
    gsu_code(0x01af00, 6);
    gsu.r[12] = gsu_lob(gsu_ldw(FX_SLOT_COUNT));
    if (gsu.z)
        { 
        gsu_code(0x01af06, 5);
        return; } else { gsu_code(0x01af06, 4); gsu_code(0x01af0c, 1); gsu_cache(0xaf0d); }
    gsu.r[13] = 0xAF0F;
    gsu_code(0x01af0d, 3);
    do {
        gsu.r[0] = gsu_and((gsu_ldb(gsu_code_then(0x01af10, 6, field(0x07, GSU_COPY(0))))), 0x0080);
        gsu.r[1] = 0x0080;
        if (gsu.z) {
            /* Into view space. */
            gsu_code(0x01af16, 7);
            gsu.r[1] = (gsu_ldw(gsu_code_then(0x01af1d, 6, field(0x12, GSU_COPY(1)))));
            gsu.r[2] = (gsu_ldw(gsu_code_then(0x01af23, 6, field(0x10, GSU_COPY(2)))));
            gsu.r[3] = (gsu_ldw(gsu_code_then(0x01af29, 6, field(0x14, GSU_COPY(3)))));
            rotate_column(0, GSU_COPY(0));
            gsu_code(0x01af41, 2);
            (gsu_stw(gsu_code_then(0x01af43, 7, field(0x12, GSU_COPY(4))), gsu.r[0]));
            rotate_column(1, GSU_COPY(1));
            gsu_code(0x01af5c, 2);
            (gsu_stw(gsu_code_then(0x01af5e, 7, field(0x10, GSU_COPY(5))), gsu.r[0]));
            rotate_column(2, GSU_COPY(2));
            gsu_code(0x01af77, 2);
            (gsu_stw(gsu_code_then(0x01af79, 7, field(0x14, GSU_COPY(6))), gsu.r[0]));
        } else { gsu_code(0x01af16, 6); gsu_code(0x01af7f, 1); }
        /* Cull by the shape's radius. */
        gsu.r[0] = (gsu_ldw(gsu_code_then(0x01af80, 5, field(0x08, GSU_COPY(7)))));
        gsu_code(0x01af85, 2);
        gsu_r14(0x10);
        gsu_code(0x01af87, 2);
        gsu_r14(gsu_add(gsu.r[14], gsu.r[0], 0));
        gsu_code(0x01af89, 2);
        gsu.r[1] = (gsu_getb());
        gsu_code(0x01af8b, 1);
        gsu_r14(gsu_inc(gsu.r[14]));
        gsu_code(0x01af8c, 3);
        gsu.r[1] = gsu_getbh(gsu.r[1]);
        gsu.r[3] = (gsu_ldw(gsu_code_then(0x01af8f, 6, field(0x14, GSU_COPY(8)))));
        gsu.r[0] = gsu_add(gsu.r[3], gsu.r[1], 0);
        if (gsu.s)
            { gsu_code(0x01af95, 8); gsu_code(0x01b01f, 1);
            goto next; } else { gsu_code(0x01af95, 4); gsu_code(0x01af9d, 1); }                  /* culled: each test jumps on alone */
        gsu.r[3] = absolute(gsu.r[3], GSU_COPY(0));
        gsu.r[0] = absolute((gsu_ldw(gsu_code_then(0x01afa6, 5, field(0x12, GSU_COPY(9))))), GSU_COPY(1));
        gsu.r[0] = gsu_sub(gsu.r[0], gsu.r[1], 0);
        gsu_sub(gsu.r[0], gsu.r[3], 0);
        if (!gsu.s)
            { gsu_code(0x01afb2, 9); gsu_code(0x01b01f, 1);
            goto next; } else { gsu_code(0x01afb2, 5); gsu_code(0x01afbb, 1); }
        /* The depth key, and the slot into the list. */
        gsu.r[0] = (gsu_ldw(gsu_code_then(0x01afbc, 5, field(0x08, GSU_COPY(10)))));
        gsu.r[1] = 5;
        gsu_code(0x01afc1, 5);
        gsu_r14(gsu_add(gsu.r[0], gsu.r[1], 0));
        gsu_code(0x01afc6, 1);
        gsu.r[0] = (gsu_getb());
        gsu_code(0x01afc7, 1);
        gsu_r14(gsu_inc(gsu.r[14]));
        gsu.r[1] = (gsu_ldw(gsu_code_then(0x01afc8, 6, field(0x14, GSU_COPY(11)))));
        gsu_code(0x01afce, 2);
        gsu.r[0] = gsu_getbh(gsu.r[0]);
        gsu.r[1] = gsu_add(gsu.r[0], gsu.r[1], 0);
        gsu.r[0] = (gsu_ldw(gsu_code_then(0x01afd0, 7, field(0x02, GSU_COPY(12)))));
        gsu.r[1] = gsu_add(gsu.r[0], gsu.r[1], 0);
        link_slot();
        (gsu_stw(gsu_code_then(0x01b014, 6, field(0x02, GSU_COPY(13))), gsu.r[1]));
        (gsu_stw(gsu_code_then(0x01b01a, 6, field(0x00, GSU_COPY(14))), gsu.r[7]));
next:
        gsu.r[0] = FX_SLOT_SIZE;
        gsu.r[9] = gsu_add(gsu.r[0], gsu.r[9], 0);
    } while (gsu_code_loop(0x01af0f, 1, gsu_code_loop(0x01b020, 5, gsu_count_down(&gsu.r[12]))));
    gsu_code(0x01b020, 5);
    gsu_code(0x01b025, 1);
}

void abi_FxSortObjectsProgram(void)
{
    FxSortObjectsProgram();
}
