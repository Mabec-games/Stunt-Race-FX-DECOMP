/*
 * gsu_record.c: each object's record in Super FX RAM, which the renderer
 * reads. An object's +$22 (bank $7E, through the data bank) holds the
 * record's address in bank $70, or 0 when it has none. Fields set here:
 * +$0C/+$0E/+$10 the position, +$16 flags, +$1A a mode word.
 */
#include "machine.h"

/* The object's record address, read through the data bank. */
static uint16_t record_of(uint8_t bank, uint16_t obj)
{
    return rd16((uint32_t)bank << 16 | (uint16_t)(0x2022 + obj));
}

/* Exit of the routines below: M 8-bit; no record: A = 0, Z set, N clear;
   otherwise A = the last value written, N and Z from X (the object). */
static void exit_record(uint16_t rec, uint16_t a)
{
    if (!rec) {
        cpu.a = 0x0000;
        cpu.p = (uint8_t)((cpu.p & ~P_N) | P_Z | P_M);
        return;
    }
    cpu.a = a;
    cpu.p = (uint8_t)(nz16(cpu.p, cpu.x) | P_M);
}

/*
 * @routine 08:CCBE GsuRecordMode2
 * WHAT IT DOES: Sets object X's record mode word (+$1A) to 2.
 * HOW: A 16-bit store at $70:001A + the record address, when there is one.
 * Entry: X 16-bit; X = object. Exit: A = 2 (0 with no record); M 8-bit; Z
 * set with no record, else N, Z from X.
 */
uint16_t GsuRecordMode2(uint8_t bank, uint16_t obj)
{
    uint16_t rec = record_of(bank, obj);

    if (rec)
        wr16(0x70001A + rec, 0x0002);
    return rec;
}

void abi_GsuRecordMode2(void)
{
    exit_record(GsuRecordMode2(cpu.db, cpu.x), 0x0002);
}

/*
 * @routine 08:CCF1 GsuRecordSetFlags
 * WHAT IT DOES: Sets the bits in direct page $02 in object X's record flags
 * (+$16).
 * HOW: 16-bit OR in place, when the object has a record.
 * Entry: X 16-bit, D = $0000; X = object. Exit: A = the new flags (0 with no
 * record); M 8-bit; Z set with no record, else N, Z from X.
 */
uint16_t GsuRecordSetFlags(uint8_t bank, uint16_t obj, uint16_t *a)
{
    uint16_t rec = record_of(bank, obj);

    if (rec) {
        *a = (uint16_t)(rd16(0x700016 + rec) | rd16(0x000002));
        wr16(0x700016 + rec, *a);
    }
    return rec;
}

void abi_GsuRecordSetFlags(void)
{
    uint16_t a = 0, rec = GsuRecordSetFlags(cpu.db, cpu.x, &a);

    exit_record(rec, a);
}

/*
 * @routine 08:CCA3 GsuRecordFlag10
 * WHAT IT DOES: In object X's record flags (+$16), sets bit 4 and clears
 * bit 8.
 * HOW: 16-bit OR and AND in place, when the object has a record.
 * Entry: X 16-bit; X = object. Exit: A = the new flags (0 with no record);
 * M 8-bit; Z set with no record, else N, Z from X.
 */
uint16_t GsuRecordFlag10(uint8_t bank, uint16_t obj, uint16_t *a)
{
    uint16_t rec = record_of(bank, obj);

    if (rec) {
        *a = (uint16_t)((rd16(0x700016 + rec) | 0x0010) & 0xFEFF);
        wr16(0x700016 + rec, *a);
    }
    return rec;
}

void abi_GsuRecordFlag10(void)
{
    uint16_t a = 0, rec = GsuRecordFlag10(cpu.db, cpu.x, &a);

    exit_record(rec, a);
}

/*
 * @routine 08:CCD2 GsuRecordPosition
 * WHAT IT DOES: Copies the position in direct page $02, $08, $94 into object
 * X's record (+$0C, +$0E, +$10).
 * HOW: Three 16-bit stores, when the object has a record.
 * Entry: X 16-bit, D = $0000; X = object. Exit: A = the third word (0 with
 * no record); M 8-bit; Z set with no record, else N, Z from X.
 */
uint16_t GsuRecordPosition(uint8_t bank, uint16_t obj, uint16_t *a)
{
    uint16_t rec = record_of(bank, obj);

    if (rec) {
        wr16(0x70000C + rec, rd16(0x000002));
        wr16(0x70000E + rec, rd16(0x000008));
        *a = rd16(0x000094);
        wr16(0x700010 + rec, *a);
    }
    return rec;
}

void abi_GsuRecordPosition(void)
{
    uint16_t a = 0, rec = GsuRecordPosition(cpu.db, cpu.x, &a);

    exit_record(rec, a);
}

/*
 * @routine 08:E3EE GsuRecordWord88
 * WHAT IT DOES: Returns the word at +$88 of the Super FX record of the
 * object that object X's +$06 names, minus one.
 * HOW: Y = the object's +$06; the record address is that object's +$22.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: A = the word - 1 (16-bit
 * value, M 8-bit); Y = the +$06 object; X unchanged; N, Z from X.
 */
uint16_t GsuRecordWord88(uint8_t bank, uint16_t obj, uint16_t *y)
{
    *y = rd16((uint32_t)bank << 16 | (uint16_t)(0x2006 + obj));
    return (uint16_t)(rd16(0x700088 + record_of(bank, *y)) - 1);
}

void abi_GsuRecordWord88(void)
{
    cpu.a = GsuRecordWord88(cpu.db, cpu.x, &cpu.y);
    cpu.p = (uint8_t)(nz16(cpu.p, cpu.x) | P_M);
}
