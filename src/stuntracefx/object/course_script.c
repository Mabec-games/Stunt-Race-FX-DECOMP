/*
 * course_script.c: the course script, the byte-coded list of objects and
 * settings a course starts with, run once when the course is set up.
 */
#include "machine.h"

void abi_ObjectSpawnFromScript(void);
void abi_ObjectAlloc(void);
void abi_ObjectInit(void);
void abi_ObjectPoolInit(void);
void abi_CrashGreen(void);

#define OBJ_FIELD(o, f) (0x7E2000u + (uint16_t)((f) + (o)))

/* ObjectSpawnFromScript for the entry at `y`; returns the object. */
static uint16_t spawn(uint16_t y)
{
    cpu.y = y;
    abi_ObjectSpawnFromScript();
    cpu.y = y;
    return cpu.x;
}

/* Opcodes $16 and $18: a bare object (ObjectAlloc, ObjectInit) kept in
   `slot`, with +$36 = 3, +$4F = `link` and the script pointer +$1A-$1C
   from the entry. With no free object: the stop screen (CrashGreen). */
static void bare_object(uint32_t b, uint16_t y, uint16_t slot, uint16_t link)
{
    uint16_t o;

    cpu.p |= P_M;
    abi_ObjectAlloc();
    if (!(cpu.p & P_C)) {
        abi_CrashGreen();
        return;
    }
    cpu.y = cpu.x;
    abi_ObjectInit();
    cpu.y = y;
    o = cpu.x;
    wr16(b | slot, o);
    wr8(OBJ_FIELD(o, 0x36), 0x03);
    wr16(OBJ_FIELD(o, 0x4F), link);
    wr16(OBJ_FIELD(o, 0x1A), rd16(b | (uint16_t)(0x8001 + y)));
    wr16(OBJ_FIELD(o, 0x1B), rd16(b | (uint16_t)(0x8002 + y)));
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
 * Entry: M 16-bit, X 16-bit, D = $0000; A low byte = script bank, X =
 * script offset. Exit: A = the script bank; X = 4; Y = the offset after
 * the end opcode; M 8-bit; N, Z from the data bank pulled back; C, V from
 * adding $8000.
 */
void abi_CourseScriptRun(void)
{
    uint8_t db = cpu.db;
    uint32_t b;
    uint16_t x = cpu.x, y, op, o, a, n, i;
    uint8_t v;

    cpu.db = (uint8_t)cpu.a;
    b = (uint32_t)cpu.db << 16;
    for (;;) {
        cpu.p &= (uint8_t)~(P_M | P_X);
        op = (uint16_t)(rd16(b | (uint16_t)(0x8000 + x)) & 0x00FF);
        y = x;
        cpu.y = y;
        cpu.x = op;
        switch (rd16(0x088000 + op)) {
        case 0x8049:                                   /* $02 */
            spawn(y);
            x = (uint16_t)(y + 0x16);
            break;
        case 0x8055:                                   /* $0C */
            o = spawn(y);
            wr8(OBJ_FIELD(o, 0x09), (uint8_t)(rd8(OBJ_FIELD(o, 0x09)) | 0x01));
            x = (uint16_t)(y + 0x16);
            break;
        case 0x806F:                                   /* $10, not reached */
            o = spawn(y);
            wr16(OBJ_FIELD(o, 0x27), rd16(b | (uint16_t)(0x8016 + y)));
            a = (uint16_t)(rd16(b | 0x1ADE) + 1);
            wr16(b | 0x1ADE, a);
            wr16(OBJ_FIELD(o, 0x29), a);
            x = (uint16_t)(y + 0x18);
            break;
        case 0x808F:                                   /* $0A */
            o = spawn(y);
            v = rd8(b | (uint16_t)(0x8016 + y));
            wr8(OBJ_FIELD(o, 0x48), v);
            wr8(OBJ_FIELD(o, 0x5A), (uint8_t)(v | 0x80));
            wr8(OBJ_FIELD(o, 0x49), rd8(b | (uint16_t)(0x8017 + y)));
            wr8(OBJ_FIELD(o, 0x4A), rd8(b | (uint16_t)(0x8018 + y)));
            x = (uint16_t)(y + 0x19);
            break;
        case 0x80BA:                                   /* $06 */
            for (i = 0; i < 0x0800; i = (uint16_t)(i + 2))
                wr16(0x7F0000 + i, 0x0000);
            wr8(0x001AF4, 0x00);
            wr16(b | 0x1ADE, 0x0000);
            wr16(0x7EFFB8, 0x0000);
            wr16(0x7EFFFD, 0x0000);
            wr16(0x7026F2, 0x0000);
            wr16(0x001AEC, 0x0000);
            a = (uint16_t)(rd16(b | 0x0E3D) & 0x00FF);
            wr16(0x7026F4, a);
            wr16(b | 0x0CEE, 0x0000);
            wr16(b | 0x1AEE, 0x0000);
            wr8(b | 0x1F71, 0x00);
            wr8(b | 0x1F72, 0x00);
            cpu.x = 0x0800;
            cpu.y = 0x0000;
            cpu.a = a;
            abi_ObjectPoolInit();
            x = (uint16_t)(y + 1);
            break;
        case 0x8112:                                   /* $08 */
            wr8(0x001AF4, rd8(b | (uint16_t)(0x8001 + y)));
            x = (uint16_t)(y + 2);
            break;
        case 0x8126:                                   /* $0E */
            if (rd16(b | 0x0D2B) == 0x0003 && (rd16(b | (uint16_t)(0x8018 + y)) & 0x001C)) {
                x = (uint16_t)(y + 0x1A);
                break;
            }
            o = spawn(y);
            if (rd8(b | (uint16_t)(0x8018 + y)) & 0x01)
                wr16(b | 0x0346, o);
            if (rd8(b | (uint16_t)(0x8018 + y)) & 0x02)
                wr16(b | 0x034A, o);
            wr8(OBJ_FIELD(o, 0x36), 0x04);
            wr8(OBJ_FIELD(o, 0x15), rd8(b | (uint16_t)(0x8019 + y)));
            wr16(OBJ_FIELD(o, 0x27), rd16(b | (uint16_t)(0x8016 + y)));
            a = rd16(0x7EFFB8);
            if (a != 0x0008) {
                wr16(0x7EFFB8, (uint16_t)(a + 2));
                wr16(0x7EFFAE + a, o);
                wr16(0x7EFFAE + (uint16_t)(a + 2), 0x0000);
            }
            x = (uint16_t)(y + 0x1A);
            break;
        case 0x8196:                                   /* $12 */
            n = (uint16_t)((uint16_t)((rd16(b | (uint16_t)(0x8001 + y)) & 0x00FF) - 1) << 3);
            wr16(0x7EFF66 + n, rd16(b | (uint16_t)(0x8002 + y)));
            wr16(0x7EFF68 + n, rd16(b | (uint16_t)(0x8004 + y)));
            wr16(0x7EFF6A + n, rd16(b | (uint16_t)(0x8006 + y)));
            wr8(0x7EFF6C + n, rd8(b | (uint16_t)(0x8008 + y)));
            x = (uint16_t)(y + 9);
            break;
        case 0x81CC:                                   /* $14 */
            a = rd16(0x001AEC);
            a |= rd16(b | (uint16_t)(0x8001 + y));
            wr16(0x7026F2, a);
            wr16(0x001AEC, a);
            x = (uint16_t)(y + 3);
            break;
        case 0x81E6:                                   /* $1A */
            wr16(0x001AEE, rd16(b | (uint16_t)(0x8001 + y)));
            x = (uint16_t)(y + 3);
            break;
        case 0x81F8:                                   /* $16 */
            bare_object(b, y, 0x1ADA, 0x1A64);
            x = (uint16_t)(y + 4);
            break;
        case 0x8233:                                   /* $18 */
            bare_object(b, y, 0x1ADC, 0x1A9F);
            x = (uint16_t)(y + 4);
            break;
        case 0x802C:                                   /* $04: the end */
            y = (uint16_t)(y + 1);
            a = (uint16_t)(y + 0x8000);
            wr16(0x701092, a);
            wr16(0x701094, (uint16_t)cpu.db);
            cpu.a = (uint16_t)cpu.db;
            cpu.x = op;
            cpu.y = y;
            cpu.db = db;
            cpu.p = (uint8_t)((nz8(cpu.p, db) & ~(P_C | P_V)) | P_M | ((unsigned)y + 0x8000 > 0xFFFF ? P_C : 0)
                              | ((y & ~a & 0x8000) ? P_V : 0));
            return;
        default:                                       /* $00: back to offset 0 */
            x = op;
            break;
        }
    }
}
