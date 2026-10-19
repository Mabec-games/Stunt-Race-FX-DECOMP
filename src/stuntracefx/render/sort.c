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
static uint16_t field(uint8_t offset)
{
    gsu.r[8] = gsu_add(offset, gsu.r[9], 0);
    return gsu.r[8];
}

/* One element of the rotated position: R1, R2, R3 times a column of the
   matrix (FMULT, ROL, summed in R5 then R0). */
static uint16_t rotate_column(int col)
{
    gsu.r[6] = gsu_ldw((uint16_t)(FX_MATRIX + col * 2));
    gsu.r[5] = gsu_rol(gsu_fmult(gsu.r[1], gsu.r[6]));
    gsu.r[6] = gsu_ldw((uint16_t)(FX_MATRIX + 6 + col * 2));
    gsu.r[0] = gsu_rol(gsu_fmult(gsu.r[2], gsu.r[6]));
    gsu.r[5] = gsu_add(gsu.r[5], gsu.r[0], 0);
    gsu.r[6] = gsu_ldw((uint16_t)(FX_MATRIX + 12 + col * 2));
    gsu.r[0] = gsu_rol(gsu_fmult(gsu.r[3], gsu.r[6]));
    return gsu.r[0] = gsu_add(gsu.r[0], gsu.r[5], 0);
}

/* |v| as MOVES, then NOT and INC when negative. */
static uint16_t absolute(uint16_t v)
{
    gsu_moves(v);
    return gsu.s ? gsu_inc(gsu_not(v)) : v;
}

/* Links slot R9 (depth key R1) into the list: walks it from the head with
   R7, R2 the slot before, and puts R9 before the first slot whose key is
   not above R1. Leaves R7 = the slot after R9. */
static void link_slot(void)
{
    gsu_code(0x01AFD7, 8);
    gsu.r[2] = 0;
    gsu.r[7] = gsu_ldw(FX_LIST_HEAD);
    gsu_moves(gsu.r[7]);
    if (gsu.z) {
        gsu_code(0x01AFDF, 9);
        gsu_stw(FX_LIST_HEAD, gsu.r[9]);
        gsu.r[7] = gsu.r[2];
        gsu_code(0x01AFE8, 4);
        return;
    } else { gsu_code(0x01AFDF, 4); }
    for (;;) {
        gsu_code(0x01AFEC, 4);
        int above;

        gsu.r[7] = gsu_inc(gsu_inc(gsu.r[7]));
        gsu.r[0] = gsu_ldw(gsu.r[7]);
        gsu.r[7] = gsu_dec(gsu.r[7]);
        gsu.r[0] = gsu_dec(gsu.r[0]);
        gsu_sub(gsu.r[0], gsu.r[1], 0);
        above = !gsu.s;                     /* key - 1 - R1 >= 0 */
        gsu.r[7] = gsu_dec(gsu.r[7]);       /* BPL's delay slot */
        if (!above) {
            gsu_code(0x01AFF0, 11);
            gsu_moves(gsu.r[2]);
            if (gsu.z)
                { gsu_code(0x01AFFB, 5); gsu_stw(FX_LIST_HEAD, gsu.r[9]); }
            else
                { gsu_code(0x01B002, 3); gsu_stw(gsu.r[2], gsu.r[9]); }
            return;
        } else { gsu_code(0x01AFF0, 6); gsu_code(0x01B007, 5); }
        gsu.r[2] = gsu.r[7];
        gsu.r[7] = gsu_ldw(gsu.r[7]);
        gsu_moves(gsu.r[7]);
        if (gsu.z) {
            gsu_code(0x01B002, 3); gsu_code(0x01B00C, 7);
            gsu_stw(gsu.r[2], gsu.r[9]);
            gsu_code(0x01B005, 2);
            return;
        } else { gsu_code(0x01B00C, 4); }
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
    gsu_code(0x01AEF9, 7);
    gsu.r[0] = gsu_sub(gsu.r[0], gsu.r[0], 0);
    gsu.rombr = 0;
    gsu_stw(FX_LIST_HEAD, gsu.r[0]);
    gsu.r[9] = FX_SLOTS;
    gsu_code(0x01AF00, 6);
    gsu.r[12] = gsu_lob(gsu_ldw(FX_SLOT_COUNT));
    if (gsu.z)
        return; else { gsu_code(0x01AF06, 4); gsu_code(0x01AF0C, 2); gsu_cache(0xAF0D); gsu_code(0x01AF0E, 1); }
    gsu.r[13] = 0xAF0F;
    do {
        gsu_code(0x01AF0F, 7);
        int culled = 0;

        gsu.r[0] = gsu_and(gsu_ldb(field(0x07)), 0x0080);
        gsu.r[1] = 0x0080;
        if (gsu.z) {
            gsu_code(0x01AF16, 13);
            /* Into view space. */
            gsu.r[1] = gsu_ldw(field(0x12));
            gsu_code(0x01AF23, 6);
            gsu.r[2] = gsu_ldw(field(0x10));
            gsu_code(0x01AF29, 6);
            gsu.r[3] = gsu_ldw(field(0x14));
            gsu_code(0x01AF2F, 3);
            rotate_column(0);
            gsu_code(0x01AF41, 9);
            gsu_stw(field(0x12), gsu.r[0]);
            gsu_code(0x01AF4A, 3);
            rotate_column(1);
            gsu_code(0x01AF5C, 9);
            gsu_stw(field(0x10), gsu.r[0]);
            gsu_code(0x01AF65, 3);
            rotate_column(2);
            gsu_code(0x01AF77, 9);
            gsu_stw(field(0x14), gsu.r[0]);
        }
        /* Cull by the shape's radius. */
        gsu_code(0x01AF80, 5);
        gsu.r[0] = gsu_ldw(field(0x08));
        gsu_code(0x01AF85, 2);
        gsu_r14(0x10);
        gsu_code(0x01AF87, 2);
        gsu_r14(gsu_add(gsu.r[14], gsu.r[0], 0));
        gsu.r[1] = (gsu_code(0x01AF89, 2), gsu_getb());
        gsu_code(0x01AF8B, 1);
        gsu_r14(gsu_inc(gsu.r[14]));
        gsu_code(0x01AF8C, 3);
        gsu.r[1] = gsu_getbh(gsu.r[1]);
        gsu_code(0x01AF8F, 6);
        gsu.r[3] = gsu_ldw(field(0x14));
        gsu.r[0] = gsu_add(gsu.r[3], gsu.r[1], 0);
        if (gsu.s) {
            gsu_code(0x01AF95, 8);
            culled = 1;
        } else {
            gsu_code(0x01AF95, 4); gsu_code(0x01AF9D, 5); gsu_code(0x01AFA5, 6);
            gsu.r[3] = absolute(gsu.r[3]);
            gsu.r[0] = absolute(gsu_code_then(0x01AFAB, 4, gsu_code_then(0x01AFB1, 6, gsu_ldw(field(0x12)))));
            gsu.r[0] = gsu_sub(gsu.r[0], gsu.r[1], 0);
            gsu_sub(gsu.r[0], gsu.r[3], 0);
            culled = !gsu.s;
        }
        if (!culled) {
            gsu_code(0x01AFBB, 6);
            /* The depth key, and the slot into the list. */
            gsu.r[0] = gsu_ldw(field(0x08));
            gsu.r[1] = 5;
            gsu_code(0x01AFC1, 5);
            gsu_r14(gsu_add(gsu.r[0], gsu.r[1], 0));
            gsu.r[0] = (gsu_code(0x01AFC6, 1), gsu_getb());
            gsu_code(0x01AFC7, 1);
            gsu_r14(gsu_inc(gsu.r[14]));
            gsu_code(0x01AFC8, 6);
            gsu.r[1] = gsu_ldw(field(0x14));
            gsu_code(0x01AFCE, 2);
            gsu.r[0] = gsu_getbh(gsu.r[0]);
            gsu.r[1] = gsu_add(gsu.r[0], gsu.r[1], 0);
            gsu_code(0x01AFD0, 7);
            gsu.r[0] = gsu_ldw(field(0x02));
            gsu.r[1] = gsu_add(gsu.r[0], gsu.r[1], 0);
            link_slot();
            gsu_code(0x01B013, 7);
            gsu_stw(field(0x02), gsu.r[1]);
            gsu_code(0x01B01A, 6);
            gsu_stw(field(0x00), gsu.r[7]);
        } else { gsu_code(0x01B01F, 1); }
        gsu.r[0] = FX_SLOT_SIZE;
        gsu.r[9] = gsu_add(gsu.r[0], gsu.r[9], 0);
    } while (gsu_code_loop(0x01B020, 5, gsu_count_down(&gsu.r[12])));
    gsu_code(0x01B020, 6);
}

void abi_FxSortObjectsProgram(void)
{
    FxSortObjectsProgram();
}
