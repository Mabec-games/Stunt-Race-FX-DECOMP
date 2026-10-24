/*
 * course_script.c: the course script, the byte-coded list of objects and
 * settings a course starts with, run once when the course is set up.
 */
#include "machine.h"
#include "carried.h"
#include "wram_names.h"


void abi_ObjectSpawnFromScript(void);
void abi_ObjectAlloc(void);
void abi_ObjectInit(void);
void abi_ObjectPoolInit(void);
void abi_CrashGreen(void);

#define OBJ_FIELD(o, f) (0x7E0000 | W(obj_base) + (uint16_t)((f) + (o)))

void CourseScriptRun(Carried *cr);
static void bare_object(Carried *cr, uint32_t b, uint16_t y, uint16_t slot, uint16_t link);
static uint16_t spawn(Carried *cr, uint16_t y);
void CrashGreen_cr(Carried *cr);
void ObjectAlloc_cr(Carried *cr);
void ObjectInit_cr(Carried *cr);
void ObjectPoolInit_cr(Carried *cr);
void ObjectSpawnFromScript(Carried *cr);

/* ObjectSpawnFromScript for the entry at `y`; returns the object. */
static uint16_t spawn(Carried *cr, uint16_t y)
{
    cr->y = y;
    (cyc(22), ObjectSpawnFromScript(cr));
    cr->y = y;
    return cr->x;
}

/* Opcodes $16 and $18: a bare object (ObjectAlloc, ObjectInit) kept in
   `slot`, with +$36 = 3, +$4F = `link` and the script pointer +$1A-$1C
   from the entry. With no free object: the stop screen (CrashGreen). */
static void bare_object(Carried *cr, uint32_t b, uint16_t y, uint16_t slot, uint16_t link)
{
    uint16_t o;

    set_p(cr, (uint8_t)(cr->p | (P_M)));
    (cyc(90), ObjectAlloc_cr(cr));
    if (!(cr->p & P_C)) {
        CrashGreen_cr(cr);
        return;
    } else { cyc(98); }
    cr->y = cr->x;
    ObjectInit_cr(cr);
    cr->y = y;
    o = cr->x;
    cyc(60);
    wr16(b | slot, o);
    cyc(48);
    wr8(OBJ_FIELD(o, OBJ_OFF(obj_36)), 0x03);
    cyc(78);
    wr16(OBJ_FIELD(o, OBJ_OFF(obj_link_w)), link);
    cyc(78);
    wr16(OBJ_FIELD(o, OBJ_OFF(obj_1a_w)), rd16(b | (uint16_t)(0x8001 + y)));
    cyc(78);
    wr16(OBJ_FIELD(o, OBJ_OFF(obj_1b_long)), rd16(b | (uint16_t)(0x8002 + y)));
}

/*
 * @routine 08:801C CourseScriptRun
 * WHAT IT DOES: Runs the course script at X in bank A (read at $8000 + X):
 * each entry starts with an opcode byte, the offset of its handler in the
 * table at $08:8000. $02 spawns an object from a $16-byte entry
 * (ObjectSpawnFromScript); $0C does the same and sets bit 0 of +$09; $10
 * spawns from a $18-byte entry with +$27 from the entry and +$29 from a
 * count at $1ADE; $0A spawns from a $19-byte entry with +$48-$4A from it
 * (and +$5A = +$48 OR $80); $0E spawns from a $1A-byte entry (unless HUD
 * mode 3 and bits 2-4 of its flags byte), keeping it as player 0's ($0346)
 * or player 1's ($034A) object by bits 0 and 1, with +$36 = 4, +$15 and
 * +$27 from the entry, and adds it to the list at $7E:FFAE (at most 4); $06
 * empties the buckets and resets the object state (ObjectPoolInit); $08
 * sets $1AF4; $12 writes a 7-byte record into $7E:FF66 + (n - 1) * 8; $14
 * ORs a word into $1AEC (and $70:26F2); $1A sets $1AEE; $16 and $18 make
 * bare objects kept at $1ADA and $1ADC; $04 ends it, leaving the address
 * after it ($8000 + X) and the bank at $70:1092-$70:1094. ($00 starts
 * over at offset 0.)
 * HOW: The data bank is the script bank throughout; JMP (table,X) by the
 * opcode, each handler going back to the loop with X past its entry.
 * Opcode $10 and the no-free-object stops are not reached in the traces
 * so far.
 * Entry: M 8-bit, X 16-bit, D = $0000; A = script bank (PHA, PLB), X =
 * script offset. Exit: A = the script bank; X = 4; Y = the offset after
 * the end opcode; M 8-bit; N, Z from the data bank pulled back; C, V from
 * adding $8000.
 */
/* 16 when a word at a is ROM (banks $00-$3F and $80-$BF from $8000), whose
   clocks the C states; work RAM reads are placed on the original's. */
static uint32_t rom_word_at(uint32_t a)
{
    uint8_t bank = (uint8_t)(a >> 16);

    return (bank < 0x40 || (bank >= 0x80 && bank < 0xC0)) && (a & 0xFFFF) >= 0x8000 ? 16 : 0;
}

void CourseScriptRun(Carried *cr)
{
    uint8_t db = cr->db;
    uint32_t b;
    uint16_t x = cr->x, y, op, o, a, n, i;
    uint8_t v;
    int first = 1;

    cr->db = (uint8_t)cr->a;
    b = (uint32_t)cr->db << 16;
    for (;;) {
        set_p(cr, (uint8_t)(cr->p & ((uint8_t)~(P_M | P_X))));
        cyc(first ? 124 : 108 + rom_word_at(b + 0x8000 + x));
        first = 0;
        op = (uint16_t)(rd16(b | (uint16_t)(0x8000 + x)) & 0x00FF);
        y = x;
        cr->y = y;
        cr->x = op;
        cyc(122);
        switch (rd16(0x088000 + op)) {
        case 0x8049:                                   /* $02 */
            spawn(cr, y);
            x = (uint16_t)(y + 0x16);
            break;
        case 0x8055:                                   /* $0C */
            o = spawn(cr, y);
            cyc(54);
            wr8(OBJ_FIELD(o, OBJ_OFF(obj_09)), cyc_then(48, (uint8_t)(rd8(OBJ_FIELD(o, OBJ_OFF(obj_09))) | 0x01)));
            cyc(22);                                   /* REP */
            x = (uint16_t)(y + 0x16);
            break;
        case 0x806F:                                   /* $10, not reached */
            o = spawn(cr, y);
            cyc(52);
            wr16(OBJ_FIELD(o, OBJ_OFF(obj_27)), cyc_then(32, rd16(b | (uint16_t)(0x8016 + y))));
            cyc(24);
            a = (uint16_t)(rd16(b | W(spawn_counter)) + 1);
            cyc(38);
            wr16(b | W(spawn_counter), a);
            cyc(32);
            wr16(OBJ_FIELD(o, OBJ_OFF(obj_29)), a);
            x = (uint16_t)(y + 0x18);
            break;
        case 0x808F:                                   /* $0A */
            o = spawn(cr, y);
            cyc(92);
            v = rd8(b | (uint16_t)(0x8016 + y));
            wr8(OBJ_FIELD(o, OBJ_OFF(obj_48)), v);
            cyc(48);
            wr8(OBJ_FIELD(o, OBJ_OFF(obj_5a)), (uint8_t)(v | 0x80));
            cyc(70);
            wr8(OBJ_FIELD(o, OBJ_OFF(obj_49)), rd8(b | (uint16_t)(0x8017 + y)));
            cyc(70);
            wr8(OBJ_FIELD(o, OBJ_OFF(obj_4a)), rd8(b | (uint16_t)(0x8018 + y)));
            cyc(22);                                   /* REP */
            x = (uint16_t)(y + 0x19);
            break;
        case 0x80BA:                                   /* $06 */
            cyc(52);
            for (i = 0; i < 0x0800; i = (uint16_t)(i + 2))
                { cyc(96); wr16(W(object_buckets) + i, 0x0000); }
            cyc(128);
            wr8(W(spawn_mirror), 0x00);
            cyc(46);
            wr16(b | W(spawn_counter), 0x0000);
            cyc(56);
            wr16(W(object_list_b_len), 0x0000);
            cyc(32);
            wr16(W(object_list_len), 0x0000);
            cyc(32);
            wr16(0x7026F2, 0x0000);
            cyc(32);
            wr16(W(wram_1AEC), 0x0000);
            cyc(24);
            a = (uint16_t)(rd16(b | W(course)) & 0x00FF);
            cyc(56);
            wr16(0x7026F4, a);
            cyc(24);
            wr16(b | W(wram_0CEE), 0x0000);
            cyc(24);
            wr16(b | W(wram_1AEE), 0x0000);
            cyc(46);
            wr8(b | W(script_byte_1f71), 0x00);
            cyc(24);
            wr8(b | W(script_byte_1f72), 0x00);
            cr->x = 0x0800;
            cr->y = 0x0000;
            cr->a = a;
            (cyc(84), ObjectPoolInit_cr(cr));
            x = (uint16_t)(y + 1);
            break;
        case 0x8112:                                   /* $08 */
            cyc(68);
            wr8(W(spawn_mirror), rd8(b | (uint16_t)(0x8001 + y)));
            cyc(22);
            x = (uint16_t)(y + 2);
            break;
        case 0x8126:                                   /* $0E */
            if (rd16(b | W(hud_mode)) == 0x0003 && (rd16(cyc_then(86, b | (uint16_t)(0x8018 + y))) & 0x001C)) {
                cyc(92);
                x = (uint16_t)(y + 0x1A);
                break;
            } else { cyc(70); }
            o = spawn(cr, y);
            cyc(98);
            if (rd8(b | (uint16_t)(0x8018 + y)) & 0x01)
                { cyc(18); wr16(b | W(wram_0346), o); }
            cyc(76);
            if (rd8(b | (uint16_t)(0x8018 + y)) & 0x02)
                { cyc(18); wr16(b | W(wram_034A), o); }
            cyc(48);
            wr8(OBJ_FIELD(o, OBJ_OFF(obj_36)), 0x04);
            cyc(70);
            wr8(OBJ_FIELD(o, OBJ_OFF(obj_15)), rd8(b | (uint16_t)(0x8019 + y)));
            cyc(100);
            wr16(OBJ_FIELD(o, OBJ_OFF(obj_27)), rd16(b | (uint16_t)(0x8016 + y)));
            cyc(106);
            a = rd16(W(object_list_b_len));
            if (a != 0x0008) {
                cyc(114);
                wr16(W(object_list_b_len), (uint16_t)(a + 2));
                cyc(46);
                wr16(W(object_list_b) + a, o);
                cyc(84);
                wr16(W(object_list_b) + (uint16_t)(a + 2), 0x0000);
            }
            cyc(94);
            x = (uint16_t)(y + 0x1A);
            break;
        case 0x8196:                                   /* $12 */
            cyc(216);
            n = (uint16_t)((uint16_t)((rd16(b | (uint16_t)(0x8001 + y)) & 0x00FF) - 1) << 3);
            wr16(W(script_records) + n, rd16(b | (uint16_t)(0x8002 + y)));
            cyc(78);
            wr16(W(wram_7EFF68) + n, rd16(b | (uint16_t)(0x8004 + y)));
            cyc(78);
            wr16(W(wram_7EFF6A) + n, rd16(b | (uint16_t)(0x8006 + y)));
            cyc(92);
            wr8(W(wram_7EFF6C) + n, rd8(b | (uint16_t)(0x8008 + y)));
            cyc(22);                                   /* REP */
            x = (uint16_t)(y + 9);
            break;
        case 0x81CC:                                   /* $14 */
            cyc(30);
            a = rd16(W(wram_1AEC));
            cyc(78);
            a |= rd16(b | (uint16_t)(0x8001 + y));
            wr16(0x7026F2, a);
            cyc(32);
            wr16(W(wram_1AEC), a);
            x = (uint16_t)(y + 3);
            break;
        case 0x81E6:                                   /* $1A */
            cyc(76);
            wr16(W(wram_1AEE), rd16(b | (uint16_t)(0x8001 + y)));
            x = (uint16_t)(y + 3);
            break;
        case 0x81F8:                                   /* $16 */
            bare_object(cr, b, y, W(bare_object_a), 0x1A64);
            x = (uint16_t)(y + 4);
            break;
        case 0x8233:                                   /* $18 */
            bare_object(cr, b, y, W(bare_object_b), 0x1A9F);
            x = (uint16_t)(y + 4);
            break;
        case 0x802C:                                   /* $04: the end */
            y = (uint16_t)(y + 1);
            a = (uint16_t)(y + 0x8000);
            cyc(96);
            wr16(0x701092, a);
            cyc(150);
            wr16(0x701094, (uint16_t)cr->db);
            cr->a = (uint16_t)cr->db;
            cr->x = op;
            cr->y = y;
            cr->db = db;
            set_p(cr, (uint8_t)((nz8(cr->p, db) & ~(P_C | P_V)) | P_M | ((unsigned)y + 0x8000 > 0xFFFF ? P_C : (cyc(94), 0))
                              | ((y & ~a & 0x8000) ? P_V : 0)));
            return;
        default:                                       /* $00: back to offset 0 */
            x = op;
            break;
        }
        cyc(34);
    }
}

void abi_CourseScriptRun(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CourseScriptRun(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

