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

/*
 * @routine 08:E401 GsuRecordWord58
 * WHAT IT DOES: Returns the word at +$58 of the Super FX record of object X's
 * +$06 object, with carry set when bits 4 and 10 of the record's +$42 are
 * both clear.
 * HOW: Y = the object's +$06; the record address is that object's +$22.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: A = the word (M 8-bit); Y =
 * the +$06 object; X unchanged; N, Z from X; C as above.
 */
uint16_t GsuRecordWord58(uint8_t bank, uint16_t obj, uint16_t *y, int *clear)
{
    uint16_t rec;

    *y = rd16((uint32_t)bank << 16 | (uint16_t)(0x2006 + obj));
    rec = record_of(bank, *y);
    *clear = !(rd16(0x700042 + rec) & 0x0410);
    return rd16(0x700058 + rec);
}

void abi_GsuRecordWord58(void)
{
    int clear;

    cpu.a = GsuRecordWord58(cpu.db, cpu.x, &cpu.y, &clear);
    cpu.p = (uint8_t)((nz16(cpu.p, cpu.x) & ~P_C) | (clear ? P_C : 0) | P_M);
}

/*
 * @routine 08:D070 GsuRecordFlag4
 * WHAT IT DOES: Sets bit 2 of object X's record word at +$1C and returns 0;
 * an object without a record gets $FF in its +$36 and returns $FF.
 * HOW: The record test reads both bytes of +$22 (8-bit OR).
 * Entry: M 8-bit, X 16-bit; X = object. Exit: A low byte = 0 or $FF; X
 * unchanged; Y = X when there is a record; M 8-bit; N, Z from A.
 */
uint16_t GsuRecordFlag4(uint8_t bank, uint16_t obj, int *has)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t rec, w;

    *has = (rd8(b | (uint16_t)(0x2022 + obj)) | rd8(b | (uint16_t)(0x2023 + obj))) != 0;
    if (!*has) {
        wr8(b | (uint16_t)(0x2036 + obj), 0xFF);
        return 0x00FF;
    }
    rec = record_of(bank, obj);
    w = (uint16_t)(rd16(0x70001C + rec) | 0x0004);
    wr16(0x70001C + rec, w);
    return (uint16_t)(w & 0xFF00);
}

void abi_GsuRecordFlag4(void)
{
    int has;
    uint16_t a = GsuRecordFlag4(cpu.db, cpu.x, &has);

    if (has) {
        cpu.a = a;
        cpu.y = cpu.x;
        cpu.p = (uint8_t)((cpu.p & ~P_N) | P_Z | P_M);
    } else {
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0xFF);
        cpu.p = (uint8_t)((cpu.p & ~P_Z) | P_N | P_M);
    }
}

/*
 * @routine 08:D83D GsuRecordVelocity
 * WHAT IT DOES: Copies object X's velocity (+$2E, +$30, +$32), times 8, into
 * its record (+$2E, +$30, +$32) and returns 0; an object without a record
 * returns $FF.
 * HOW: Three left shifts of each word.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: A low byte = 0 (high byte
 * from the last word) or A = $00FF; Y = X when there is a record; M 8-bit;
 * N, Z from A; C from the last shift when there is a record.
 */
uint16_t GsuRecordVelocity(uint8_t bank, uint16_t obj, int *has, uint8_t *p)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t rec = record_of(bank, obj), v = 0, w;
    unsigned i;

    *has = rec != 0;
    if (!rec)
        return 0x00FF;
    for (i = 0; i < 3; i++) {
        w = rd16(b | (uint16_t)(0x202E + 2 * i + obj));
        *p = (uint8_t)((*p & ~P_C) | (w & 0x2000 ? P_C : 0));
        v = (uint16_t)(w << 3);
        wr16(0x70002E + 2 * i + rec, v);
    }
    return (uint16_t)(v & 0xFF00);
}

void abi_GsuRecordVelocity(void)
{
    int has;
    uint8_t p = cpu.p;
    uint16_t a = GsuRecordVelocity(cpu.db, cpu.x, &has, &p);

    cpu.a = a;
    if (has) {
        cpu.y = cpu.x;
        cpu.p = (uint8_t)((p & ~P_N) | P_Z | P_M);
    } else {
        cpu.p = (uint8_t)((p & ~P_Z) | P_N | P_M);
    }
}

/*
 * @routine 08:B863 GsuRecordWords88
 * WHAT IT DOES: For each object in the list at $7E:FFAE (count = half the
 * word at $7E:FFB8), copies the word at +$88 of its Super FX record into
 * the list at $7E:FFA6.
 * HOW: The record address is each object's +$22 (bank $7E); direct page $02
 * counts the entries.
 * Entry: any widths, D = $0000. Exit: A = the last word; X, Y unchanged; P
 * unchanged except N, Z, which the final pull of X sets from X.
 */
uint16_t GsuRecordWords88(void)
{
    uint16_t n = rd16(0x7EFFB8), x = 0, v = 0;

    wr16(0x000002, (uint16_t)((n >> 1) | (n & 0x8000)));
    do {
        uint16_t obj = rd16(0x7EFFAE + x);

        v = rd16(0x700088 + rd16(0x7E2022 + obj));
        wr16(0x7EFFA6 + x, v);
        x = (uint16_t)(x + 2);
        wr16(0x000002, (uint16_t)(rd16(0x000002) - 1));
    } while (rd16(0x000002));
    return v;
}

void abi_GsuRecordWords88(void)
{
    cpu.a = GsuRecordWords88();
    cpu.p = (cpu.p & P_X) ? nz8(cpu.p, (uint8_t)cpu.x) : nz16(cpu.p, cpu.x);
}

/*
 * @routine 08:B2E9 GsuRecordStep6A
 * WHAT IT DOES: While $0D62 is set, advances object X's record word at +$6A
 * by 1 (when the record's +$88 is 1) or 3, up to $180, unless bit 10 of the
 * record's +$42 is set.
 * HOW: The step is kept at $1F2B; the limit test is a signed compare.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: $0D62 clear: A low byte = 0,
 * Z set. Otherwise A = the last word read or stored (M 8-bit); X unchanged;
 * N, Z from X; C, V from the limit compare and the add when it stepped.
 */
uint16_t GsuRecordStep6A(uint8_t bank, uint16_t obj, int *ran, uint8_t *p)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t rec, f, step, a, s;

    *ran = rd8(b | 0x0D62) != 0;
    if (!*ran)
        return 0;
    rec = record_of(bank, obj);
    f = rd16(0x700042 + rec);
    if (f & 0x0400)
        return f;
    wr16(b | 0x1F2B, 0x0001);
    if (rd16(0x700088 + rec) != 0x0001)
        wr16(b | 0x1F2B, 0x0003);
    a = rd16(0x70006A + rec);
    step = rd16(b | 0x1F2B);
    s = (uint16_t)(a + step);
    *p = (uint8_t)((*p & ~(P_C | P_V)) | ((~(a ^ step) & (a ^ s) & 0x8000) ? P_V : 0) | (s >= 0x0180 ? P_C : 0));
    if (!((uint16_t)(s - 0x0180) & 0x8000))
        s = 0x0180;
    wr16(0x70006A + rec, s);
    return s;
}

void abi_GsuRecordStep6A(void)
{
    int ran;
    uint8_t p = cpu.p;
    uint16_t a = GsuRecordStep6A(cpu.db, cpu.x, &ran, &p);

    if (!ran) {
        cpu.a &= 0xFF00;
        cpu.p = (uint8_t)((cpu.p & ~P_N) | P_Z);
        return;
    }
    cpu.a = a;
    cpu.p = (uint8_t)(nz16(p, cpu.x) | P_M);
}

/* An arithmetic shift right by three, as three CMP #$8000 / ROR A do it;
   C is the last bit shifted out. */
static uint16_t asr3(uint16_t v, uint8_t *p)
{
    *p = (uint8_t)((*p & ~P_C) | (v & 0x0004 ? P_C : 0));
    return (uint16_t)((int16_t)v >> 3);
}

/*
 * @routine 08:DC6D ObjectVelocityFromRecordOf
 * WHAT IT DOES: Sets object X's velocity (+$2E, +$30, +$32) to an eighth of
 * the values at +$2E, +$30, +$32 of object Y's Super FX record.
 * HOW: Each value is shifted right three times keeping its sign.
 * Entry: M 8-bit, X 16-bit; X = the object to set, Y = the object whose
 * record is read. Exit: A = the Z velocity (M 8-bit); X, Y unchanged; N, Z
 * from X; C from the last shift.
 */
uint16_t ObjectVelocityFromRecordOf(uint8_t bank, uint16_t obj, uint16_t from, uint8_t *p)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t rec = record_of(bank, from), v = 0;
    unsigned i;

    for (i = 0; i < 3; i++) {
        v = asr3(rd16(0x70002E + 2 * i + rec), p);
        wr16(b | (uint16_t)(0x202E + 2 * i + obj), v);
    }
    return v;
}

void abi_ObjectVelocityFromRecordOf(void)
{
    uint8_t p = cpu.p;

    cpu.a = ObjectVelocityFromRecordOf(cpu.db, cpu.x, cpu.y, &p);
    cpu.p = (uint8_t)(nz16(p, cpu.x) | P_M);
}

/*
 * @routine 08:D86F GsuRecordFromObject
 * WHAT IT DOES: Copies object X's state into its Super FX record: +$53,
 * +$55, +$57 to +$0C, +$0E, +$10; +$20 to +$14; its position (+$0C, +$0E,
 * +$10) to +$1E, +$20, +$22; its +$15 byte, as the high byte, to +$24; and
 * the low 7 bits of +$5A to +$12.
 * HOW: 16-bit copies in that order, when the object has a record.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: A = the last value (0 with no
 * record); X = Y = the object; M 8-bit; N, Z from X.
 */
uint16_t GsuRecordFromObject(uint8_t bank, uint16_t obj)
{
    static const struct { uint16_t from, to; } copy[7] = {
        {0x2053, 0x0C}, {0x2055, 0x0E}, {0x2057, 0x10}, {0x2020, 0x14},
        {0x200C, 0x1E}, {0x200E, 0x20}, {0x2010, 0x22}};
    uint32_t b = (uint32_t)bank << 16;
    uint16_t rec = record_of(bank, obj), v;
    unsigned i;

    if (!rec)
        return 0;
    for (i = 0; i < 7; i++)
        wr16(0x700000 + copy[i].to + rec, rd16(b | (uint16_t)(copy[i].from + obj)));
    wr16(0x700024 + rec, (uint16_t)((rd16(b | (uint16_t)(0x2015 + obj)) & 0x00FF) << 8));
    v = rd16(b | (uint16_t)(0x205A + obj)) & 0x007F;
    wr16(0x700012 + rec, v);
    return v;
}

void abi_GsuRecordFromObject(void)
{
    cpu.a = GsuRecordFromObject(cpu.db, cpu.x);
    cpu.y = cpu.x;
    cpu.p = (uint8_t)(nz16(cpu.p, cpu.x) | P_M);
}

uint16_t ScaleThreeQuarters(uint16_t v);

/* ScaleThreeQuarters with the flags of its final add. */
static uint16_t three_quarters(uint16_t v, uint8_t *p)
{
    uint16_t half = (uint16_t)((v >> 1) | (v & 0x8000)), quarter = (uint16_t)((half >> 1) | (half & 0x8000));
    uint16_t r = ScaleThreeQuarters(v);

    *p = (uint8_t)((*p & ~(P_C | P_V)) | ((unsigned)half + quarter > 0xFFFF ? P_C : 0)
                   | ((~(half ^ quarter) & (half ^ r) & 0x8000) ? P_V : 0));
    return r;
}

/*
 * @routine 08:D5DF ObjectVelocityFromRecordScaled
 * WHAT IT DOES: As ObjectVelocityFromRecord, with each value then scaled by
 * three quarters: X and Z velocity = 3/32 of the record values of the
 * object named by object X's +$27.
 * HOW: Shift right three times keeping the sign, then ScaleThreeQuarters.
 * Entry: M 8-bit, X 16-bit, D = $0000; X = object. Exit: A = the Z velocity
 * (M 8-bit); X, Y = the object; $3E = the last half; N, Z from X; C, V from
 * the last scaling add.
 */
uint16_t ObjectVelocityFromRecordScaled(uint8_t bank, uint16_t obj, uint8_t *p)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t rec = record_of(bank, rd16(b | (uint16_t)(0x2027 + obj))), v;

    v = three_quarters((uint16_t)((int16_t)rd16(0x70002E + rec) >> 3), p);
    wr16(b | (uint16_t)(0x202E + obj), v);
    v = three_quarters((uint16_t)((int16_t)rd16(0x700032 + rec) >> 3), p);
    wr16(b | (uint16_t)(0x2032 + obj), v);
    return v;
}

void abi_ObjectVelocityFromRecordScaled(void)
{
    uint8_t p = cpu.p;

    cpu.a = ObjectVelocityFromRecordScaled(cpu.db, cpu.x, &p);
    cpu.y = cpu.x;
    cpu.p = (uint8_t)(nz16(p, cpu.x) | P_M);
}

/*
 * @routine 09:FBB2 GsuRecordBits60
 * WHAT IT DOES: Keeps bits 5-6 of the word at +$40 of the two records whose
 * addresses are at $70:2476 and $70:2478, at $1A24 and $1A26.
 * HOW: The data bank is $09 inside (so $1A24 is work RAM).
 * Entry: any widths. Exit: A = the second value (16-bit); X, P unchanged.
 */
uint16_t GsuRecordBits60(void)
{
    uint16_t v;

    v = rd16(0x700040 + rd16(0x702476)) & 0x0060;
    wr16(0x091A24, v);
    v = rd16(0x700040 + rd16(0x702478)) & 0x0060;
    wr16(0x091A26, v);
    return v;
}

void abi_GsuRecordBits60(void)
{
    cpu.a = GsuRecordBits60();
}
