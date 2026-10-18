/*
 * flags_record.c: the object of bank $08 whose Super FX record takes the
 * object's own flags (08:CD25-CD83): its set-ups, the record step that ORs
 * the object's +$06 and +$27 words into the record's +$16, and the step
 * that only marks the record. Kept apart from gsu_record.c so a change here
 * reruns only the scripts that reach these routines.
 */
#include "machine.h"
#include "cpu_ops.h"

void abi_GsuRecordNew422(void);
void abi_GsuRecordFlag4(void);

/*
 * @routine 08:CD7E ObjectRecordFlag4Step
 * WHAT IT DOES: A handler that only marks object X's record
 * (GsuRecordFlag4) each frame; also the tail other steps jump to.
 * HOW: JSL to GsuRecordFlag4.
 * Entry: as GsuRecordFlag4. Exit: as GsuRecordFlag4.
 */
void abi_ObjectRecordFlag4Step(void)
{
    abi_GsuRecordFlag4();
}

/*
 * @routine 08:CD55 ObjectFlagsRecord
 * WHAT IT DOES: Gives object X a record from the second pool
 * (GsuRecordNew422), ORs the object's +$06 word and then its +$27 word into
 * the record's flags (+$16), and marks it (ObjectRecordFlag4Step). With no
 * room in the pool it returns at once. Also the +$1D pointer
 * ObjectFlagsRecordSetup leaves.
 * HOW: BPL on GsuRecordNew422's result (BRL to an RTL when negative); the
 * object in Y and the record in X while ORing, two read-OR-write passes
 * over +$16.
 * Entry: M 8-bit, X 16-bit, D = $0000; X = object. Exit: no room: as
 * GsuRecordNew422 (A = $00FF, N set); otherwise as GsuRecordFlag4.
 */
void abi_ObjectFlagsRecord(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint16_t obj, rec, v, w;

    abi_GsuRecordNew422();
    if (cpu.p & P_N)
        return;
    cpu.p &= (uint8_t)~P_M;
    obj = cpu.x;
    cpu.y = obj;
    rec = rd16(b | (uint16_t)(0x2022 + obj));
    cpu.x = rec;
    v = rd16(b | (uint16_t)(0x2006 + obj));
    w = rd16(0x700016 + rec);
    v |= w;
    wr16(0x700016 + rec, v);
    v = rd16(0x700016 + rec);
    w = rd16(b | (uint16_t)(0x2027 + obj));
    v |= w;
    wr16(0x700016 + rec, v);
    cpu.a = v;
    cpu.x = obj;
    cpu.p = (uint8_t)(nz16(cpu.p, obj) | P_M);
    abi_ObjectRecordFlag4Step();
}

/*
 * @routine 08:CD2D ObjectFlagsRecordSetup
 * WHAT IT DOES: Sets up an object whose record takes the object's flags:
 * +$27 = 0, handler $08:CD7E (ObjectRecordFlag4Step), +$1D = $08:CD55
 * (ObjectFlagsRecord), then its record (ObjectFlagsRecord). Entered as an
 * object's handler.
 * HOW: Stores (16-bit then 8-bit), then falls into ObjectFlagsRecord.
 * Entry: M 8-bit, X 16-bit, D = $0000; X = object. Exit: as
 * ObjectFlagsRecord.
 */
void abi_ObjectFlagsRecordSetup(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;

    cpu.p &= (uint8_t)~P_M;
    cpu.a = 0x0000;
    cpu.p = nz16(cpu.p, 0x0000);
    wr16(b | (uint16_t)(0x2027 + cpu.x), 0x0000);
    cpu.p |= P_M;
    cpu.p &= (uint8_t)~P_M;
    cpu.a = 0xCD7E;
    cpu.p = nz16(cpu.p, 0xCD7E);
    wr16(b | (uint16_t)(0x201A + cpu.x), 0xCD7E);
    cpu.p |= P_M;
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x08);
    cpu.p = nz8(cpu.p, 0x08);
    wr8(b | (uint16_t)(0x201C + cpu.x), 0x08);
    cpu.p &= (uint8_t)~P_M;
    cpu.a = 0xCD55;
    cpu.p = nz16(cpu.p, 0xCD55);
    wr16(b | (uint16_t)(0x201D + cpu.x), 0xCD55);
    cpu.p |= P_M;
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x08);
    cpu.p = nz8(cpu.p, 0x08);
    wr8(b | (uint16_t)(0x201F + cpu.x), 0x08);
    abi_ObjectFlagsRecord();
}

/*
 * @routine 08:CD25 ObjectFlagsRecordSetup20
 * WHAT IT DOES: As ObjectFlagsRecordSetup, with bit 5 of object X's +$24
 * set first. Nothing in the game calls it.
 * HOW: An 8-bit OR into +$24, then falls into ObjectFlagsRecordSetup.
 * Entry: as ObjectFlagsRecordSetup. Exit: as ObjectFlagsRecordSetup.
 */
void abi_ObjectFlagsRecordSetup20(void)
{
    uint32_t a = (uint32_t)cpu.db << 16 | (uint16_t)(0x2024 + cpu.x);
    uint8_t v;

    v = rd8(a);
    v |= 0x20;
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, v);
    wr8(a, v);
    abi_ObjectFlagsRecordSetup();
}
