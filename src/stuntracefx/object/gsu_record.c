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

uint8_t GsuRun(uint8_t bank, uint16_t start);

/* The record argument of the record programs: the record-making ones
   leave the new record's address there (0 when the pool had no room). The
   amount argument is a high byte. $1F4C is the word the S-CPU passes to the
   new record's +$0A. */
#define GSU_RECORD_ARG 0x70238Eu
#define GSU_IN_AMOUNT 0x702390u
#define RECORD_0A_WORD 0x1F4Cu

/* The tail every routine below ends in ($08:CC7C): the new record becomes
   object X's +$22; when there is one, its +$0A is set and GsuRecordFromObject
   fills it from the object. */
static uint8_t record_new(uint8_t bank, uint16_t obj, uint16_t program, uint16_t word0a)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t rec;

    GsuRun(0x04, program);
    wr16(b | RECORD_0A_WORD, word0a);
    rec = rd16(GSU_RECORD_ARG);
    wr16(b | (uint16_t)(0x2022 + obj), rec);
    if (!rec)
        return 0xFF;
    wr16(0x70000A + rec, rd16(b | RECORD_0A_WORD));
    GsuRecordFromObject(bank, obj);
    return 0x00;
}

/* Exit: A = 0 (made) or $00FF (no room), N and Z from that byte; M 8-bit;
   with a record Y = the object, otherwise Y unchanged; X unchanged. */
static void abi_record_new(uint16_t program, uint16_t word0a)
{
    uint8_t r = record_new(cpu.db, cpu.x, program, word0a);

    if (!r)
        cpu.y = cpu.x;
    cpu.a = r;
    cpu.p = (uint8_t)(nz8(cpu.p, r) | P_M);
}

/*
 * @routine 08:CB98 GsuRecordNew422
 * WHAT IT DOES: Gives object X a new Super FX record from the second pool
 * (mode $0422, linked into the list at $70:2398), with +$0A = 0, filled from
 * the object.
 * HOW: Runs Super FX program $04:8B64, which takes $2A bytes from the pool
 * whose free list is at $70:1D70 and sets the record's flags and mode; then
 * the shared tail at $08:CC7C ($1F4C = the +$0A word).
 * Entry: M 8-bit, X 16-bit, D = $0000; X = object. Exit: A = 0 when the
 * object got a record, $00FF when the pool had no room; N, Z from A's low
 * byte; M 8-bit; X unchanged; Y = X with a record, else unchanged; $1F4C =
 * the +$0A word.
 */
uint8_t GsuRecordNew422(uint8_t bank, uint16_t obj)
{
    return record_new(bank, obj, 0x8B64, 0x0000);
}

void abi_GsuRecordNew422(void)
{
    abi_record_new(0x8B64, 0x0000);
}

/*
 * @routine 08:CBB2 GsuRecordNew42D
 * WHAT IT DOES: Gives object X a new Super FX record from the first pool
 * (mode $042D, flags $20), with +$0A = $90E8, filled from the object.
 * HOW: Runs Super FX program $04:8C95, which takes $34 bytes from the pool
 * whose free list is at $70:1098; then the shared tail at $08:CC7C.
 * Entry: as GsuRecordNew422. Exit: as GsuRecordNew422.
 */
uint8_t GsuRecordNew42D(uint8_t bank, uint16_t obj)
{
    return record_new(bank, obj, 0x8C95, 0x90E8);
}

void abi_GsuRecordNew42D(void)
{
    abi_record_new(0x8C95, 0x90E8);
}

/*
 * @routine 08:CBE6 GsuRecordNew424
 * WHAT IT DOES: Gives object X a new Super FX record from the first pool
 * (mode $0424, flags 8, +$2A = $40), with +$0A = $910A, filled from the
 * object.
 * HOW: Runs Super FX program $04:8C0A; then the shared tail at $08:CC7C.
 * Entry: as GsuRecordNew422. Exit: as GsuRecordNew422.
 */
uint8_t GsuRecordNew424(uint8_t bank, uint16_t obj)
{
    return record_new(bank, obj, 0x8C0A, 0x910A);
}

void abi_GsuRecordNew424(void)
{
    abi_record_new(0x8C0A, 0x910A);
}

/*
 * @routine 08:CC18 GsuRecordNew424Alt
 * WHAT IT DOES: As GsuRecordNew424, with +$0A = $90E8.
 * HOW: Runs Super FX program $04:8C0A; then the shared tail at $08:CC7C.
 * Entry: as GsuRecordNew422. Exit: as GsuRecordNew422.
 */
uint8_t GsuRecordNew424Alt(uint8_t bank, uint16_t obj)
{
    return record_new(bank, obj, 0x8C0A, 0x90E8);
}

void abi_GsuRecordNew424Alt(void)
{
    abi_record_new(0x8C0A, 0x90E8);
}

/*
 * @routine 08:CC63 GsuRecordNew426
 * WHAT IT DOES: Gives object X a new Super FX record from the first pool
 * (mode $0426, flags 8, +$2A = $40), with +$0A = $910A, filled from the
 * object.
 * HOW: Runs Super FX program $04:8B33; then falls into the shared tail at
 * $08:CC7C, which the four routines above branch to.
 * Entry: as GsuRecordNew422. Exit: as GsuRecordNew422.
 */
uint8_t GsuRecordNew426(uint8_t bank, uint16_t obj)
{
    return record_new(bank, obj, 0x8B33, 0x910A);
}

void abi_GsuRecordNew426(void)
{
    abi_record_new(0x8B33, 0x910A);
}

int ObjectFlag40IfMatch(uint8_t bank, uint16_t a, uint16_t *x, uint8_t *v, uint8_t *p);

/* The shared part of the two routines below: the record and the amount (as
   the high byte) go to the Super FX, ObjectFlag40IfMatch flags the object
   at $0346 when the record is $70:2476's, and the program runs. Returns A
   as the original leaves it: Y's high byte over the screen mode value. */
static uint16_t record_amount_run(uint8_t bank, uint16_t rec, uint8_t amount, uint16_t program, uint8_t *p)
{
    uint16_t x = 0;
    uint8_t v = 0;

    wr16(GSU_IN_AMOUNT, (uint16_t)(amount << 8));
    wr16(GSU_RECORD_ARG, rec);
    ObjectFlag40IfMatch(bank, rec, &x, &v, p);
    return (uint16_t)((rec & 0xFF00) | GsuRun(0x04, program));
}

/*
 * @routine 08:B80A GsuRecordLower82
 * WHAT IT DOES: Lowers the word at +$82 of the Super FX record Y by an
 * amount scaled from A, not below 0; when Y is the record at $70:2476, sets
 * bit 6 of +$25 of the object at $0346.
 * HOW: $70:238E = Y and $70:2390 = A as the high byte are the inputs of
 * Super FX program $04:BF75 (the amount times 2 times $200 by FMULT,
 * subtracted); ObjectFlag40IfMatch before it runs.
 * Entry: M 8-bit, X 16-bit; A low byte = amount, Y = record. Exit: A = Y's
 * high byte and the screen mode value; X, Y unchanged; M 8-bit, X 16-bit;
 * N, Z from X; C from ObjectFlag40IfMatch's compare.
 */
uint16_t GsuRecordLower82(uint8_t bank, uint16_t rec, uint8_t amount, uint8_t *p)
{
    return record_amount_run(bank, rec, amount, 0xBF75, p);
}

void abi_GsuRecordLower82(void)
{
    uint8_t p = cpu.p;

    cpu.a = GsuRecordLower82(cpu.db, cpu.y, (uint8_t)cpu.a, &p);
    cpu.p = (uint8_t)((nz16(p, cpu.x) | P_M) & ~P_X);
}

/*
 * @routine 08:B7D6 GsuRecordRaise6A
 * WHAT IT DOES: Unless bit 0 of +$40 of the Super FX record Y is set,
 * raises its word at +$6A by an amount scaled from A, to at most $180; when
 * Y is the record at $70:2476, sets bit 6 of +$25 of the object at $0346.
 * HOW: As GsuRecordLower82, with Super FX program $04:BF51 (the amount
 * times 2 times $180 by FMULT, added and clamped).
 * Entry: M 8-bit, X 16-bit; A low byte = amount, Y = record. Exit: bit set:
 * A = 1; else as GsuRecordLower82. X, Y unchanged; M 8-bit, X 16-bit; N, Z
 * from X; C unchanged when skipped, else from ObjectFlag40IfMatch.
 */
uint16_t GsuRecordRaise6A(uint8_t bank, uint16_t rec, uint8_t amount, uint8_t *p)
{
    if (rd16(0x700040 + rec) & 0x0001) {
        wr16(GSU_IN_AMOUNT, (uint16_t)(amount << 8));
        wr16(GSU_RECORD_ARG, rec);
        return 0x0001;
    }
    return record_amount_run(bank, rec, amount, 0xBF51, p);
}

void abi_GsuRecordRaise6A(void)
{
    uint8_t p = cpu.p;

    cpu.a = GsuRecordRaise6A(cpu.db, cpu.y, (uint8_t)cpu.a, &p);
    cpu.p = (uint8_t)((nz16(p, cpu.x) | P_M) & ~P_X);
}

uint8_t ObjectTableC056(uint8_t bank, uint16_t obj, uint8_t *p);

/*
 * @routine 08:BF94 ObjectTurnFromRecord
 * WHAT IT DOES: Takes object Y's angles from its Super FX record X (+$29 to
 * +$16, +$25 to +$15) and sets its turn values for drawing: +$5D = +$29 +
 * 8 + a lean step, and +$18 (and +$19 when $0D62 is set) = $80 + the
 * direction byte from the table at $08:C056 relative to the view object at
 * $1A64 (and the one at $1A9F).
 * HOW: The lean step is (+$16 + the view's +$13) / 2 with its low two bits
 * cleared toward zero, limited to $F8..8 by signed compares, negated when
 * +$15 is more than $40 either way from the view's heading; ObjectTableC056
 * turns the negated view heading into the direction byte. The view
 * objects are read through the direct page (bank 0).
 * Entry: M 16-bit, X 16-bit, D = $0000; X = record, Y = object. Exit: M
 * 8-bit; X, Y unchanged; A high byte from +$5D; A low byte and N, Z: $0D62
 * clear: 0, Z set; else the +$19 value, N set; C, V from ObjectTableC056.
 */
uint16_t ObjectTurnFromRecord(uint8_t bank, uint16_t rec, uint16_t obj, uint8_t *p)
{
    uint32_t b = (uint32_t)bank << 16;
    uint8_t a, sum, t;
    uint16_t w;

    wr8(b | (uint16_t)(0x2016 + obj), rd8(0x700029 + rec));
    wr8(b | (uint16_t)(0x2015 + obj), rd8(0x700025 + rec));
    wr8(b | 0x1F5A, (uint8_t)-rd8(0x001A75));
    wr8(b | 0x1F2B, rd8(0x001A77));
    sum = (uint8_t)(rd8(b | (uint16_t)(0x2016 + obj)) + rd8(b | 0x1F2B));
    a = (uint8_t)((sum >> 1) | (sum & 0x80));
    if (a & 0x80)
        a = (uint8_t)-(uint8_t)((uint8_t)-a & 0xFC);
    a &= 0xFC;
    if (!((uint8_t)(a - 0x08) & 0x80))
        a = 0x08;
    if ((uint8_t)(a - 0xF8) & 0x80)
        a = 0xF8;
    wr8(b | 0x1F58, a);
    t = (uint8_t)(rd8(b | (uint16_t)(0x2015 + obj)) - rd8(b | 0x1F5A));
    if (t & 0x80)
        t = (uint8_t)-t;
    if (!((uint8_t)(t - 0x40) & 0x80))
        wr8(b | 0x1F58, (uint8_t)-rd8(b | 0x1F58));
    w = rd16(b | 0x1F58);
    w = (w & 0x0080) ? (uint16_t)(w | 0xFF00) : (uint16_t)(w & 0x00FF);
    w = (uint16_t)(w + 8 + rd16(b | (uint16_t)(0x2029 + obj)));
    wr16(b | (uint16_t)(0x205D + obj), w);
    ObjectTableC056(bank, obj, p);
    a = (uint8_t)(rd8(b | 0x1F5A) | 0x80);
    wr8(b | (uint16_t)(0x2018 + obj), a);
    a = rd8(b | 0x0D62);
    if (!a) {
        *p = nz8(*p, 0);
        return (uint16_t)(w & 0xFF00);
    }
    wr8(b | 0x1F5A, (uint8_t)-rd8(0x001AB0));
    ObjectTableC056(bank, obj, p);
    a = (uint8_t)(rd8(b | 0x1F5A) | 0x80);
    wr8(b | (uint16_t)(0x2019 + obj), a);
    *p = nz8(*p, a);
    return (uint16_t)((w & 0xFF00) | a);
}

void abi_ObjectTurnFromRecord(void)
{
    uint8_t p = cpu.p;

    cpu.a = ObjectTurnFromRecord(cpu.db, cpu.x, cpu.y, &p);
    cpu.p = (uint8_t)(p | P_M);
}

uint16_t ObjectRebucket(uint16_t obj);

/* The tail of the two routines below ($08:CE60): Y = $1F3C (the record's
   +$18); A's low byte becomes 1 when it is non-zero. */
static uint16_t sync_tail(uint8_t bank, uint16_t a, uint16_t *y, uint8_t *p)
{
    *y = rd16((uint32_t)bank << 16 | 0x1F3C);
    if (!*y) {
        *p = nz16(*p, 0);
        return a;
    }
    *p = nz8(*p, 0x01);
    return (uint16_t)((a & 0xFF00) | 0x01);
}

/* No record ($08:CE69): $1F3C = 0, A = 0, carry set. */
static uint16_t sync_none(uint8_t bank, uint8_t *p)
{
    wr16((uint32_t)bank << 16 | 0x1F3C, 0x0000);
    *p = (uint8_t)(nz8(*p, 0) | P_C);
    return 0x0000;
}

/* The steps both routines start with: ObjectRebucket, GsuRecordFlag4. */
static uint16_t sync_start(uint8_t bank, uint16_t obj)
{
    int has;

    ObjectRebucket(obj);
    GsuRecordFlag4(bank, obj, &has);
    return record_of(bank, obj);
}

/*
 * @routine 08:CE02 ObjectSyncFromRecord
 * WHAT IT DOES: Rebuckets object X, marks its record (GsuRecordFlag4), and
 * takes its position back from the record (+$1E/+$20/+$22 to +$0C/+$0E/
 * +$10), after giving the record its heading (+$15 as the high byte of
 * +$24) and +$12 (+$5A's low 7 bits); bit 1 of +$24 follows bit 1 of the
 * record's +$1C. Returns whether the record's +$18 is set.
 * HOW: The object is in Y and the record in X while copying; $1F3C keeps
 * the record's +$18; ends in the tail at $08:CE60 (shared with
 * ObjectSyncToRecord) or, with no record, $08:CE69.
 * Entry: M 8-bit, X 16-bit, D = $0000; X = object. Exit: X = the object; M
 * 8-bit; V unchanged. Record: carry clear; Y = $1F3C; A low byte = 1 when
 * it is non-zero (N, Z from that), else A = the new +$24 when the bit was
 * set, 0 when not (Z set). No record: A = 0, Y = X, $1F3C = 0, Z and carry
 * set.
 */
uint16_t ObjectSyncFromRecord(uint8_t bank, uint16_t obj, uint16_t *y, uint8_t *p)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t rec = sync_start(bank, obj), a;
    uint8_t f;

    wr8(b | (uint16_t)(0x2024 + obj), rd8(b | (uint16_t)(0x2024 + obj)) & 0xFD);
    if (!rec) {
        *y = obj;
        return sync_none(bank, p);
    }
    wr16(b | 0x1F3C, rd16(0x700018 + rec));
    wr16(0x700024 + rec, (uint16_t)(rd8(b | (uint16_t)(0x2015 + obj)) << 8));
    wr16(0x700012 + rec, rd16(b | (uint16_t)(0x205A + obj)) & 0x007F);
    wr16(b | (uint16_t)(0x200C + obj), rd16(0x70001E + rec));
    wr16(b | (uint16_t)(0x200E + obj), rd16(0x700020 + rec));
    wr16(b | (uint16_t)(0x2010 + obj), rd16(0x700022 + rec));
    a = rd16(0x70001C + rec) & 0x0002;
    if (a) {
        f = (uint8_t)(rd8(b | (uint16_t)(0x2024 + obj)) | 0x02);
        wr8(b | (uint16_t)(0x2024 + obj), f);
        a = f;
    }
    *p &= (uint8_t)~P_C;
    return sync_tail(bank, a, y, p);
}

void abi_ObjectSyncFromRecord(void)
{
    uint8_t p = cpu.p;

    cpu.a = ObjectSyncFromRecord(cpu.db, cpu.x, &cpu.y, &p);
    cpu.p = (uint8_t)(p | P_M);
}

/*
 * @routine 08:CEC3 ObjectSyncToRecord
 * WHAT IT DOES: As ObjectSyncFromRecord, the other way: after the heading
 * and +$12, the object's position goes to the record (+$0C/+$0E/+$10 to
 * +$1E/+$20/+$22); +$24 is left alone.
 * HOW: As ObjectSyncFromRecord, jumping to its tail at $08:CE60 (or
 * $08:CE69 with no record).
 * Entry: as ObjectSyncFromRecord. Exit: as ObjectSyncFromRecord, except
 * that with a record and $1F3C zero A = the object's +$10.
 */
uint16_t ObjectSyncToRecord(uint8_t bank, uint16_t obj, uint16_t *y, uint8_t *p)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t rec = sync_start(bank, obj), a;

    if (!rec) {
        *y = obj;
        return sync_none(bank, p);
    }
    wr16(0x700024 + rec, (uint16_t)(rd8(b | (uint16_t)(0x2015 + obj)) << 8));
    wr16(0x700012 + rec, rd16(b | (uint16_t)(0x205A + obj)) & 0x007F);
    wr16(b | 0x1F3C, rd16(0x700018 + rec));
    wr16(0x70001E + rec, rd16(b | (uint16_t)(0x200C + obj)));
    wr16(0x700020 + rec, rd16(b | (uint16_t)(0x200E + obj)));
    a = rd16(b | (uint16_t)(0x2010 + obj));
    wr16(0x700022 + rec, a);
    *p &= (uint8_t)~P_C;
    return sync_tail(bank, a, y, p);
}

void abi_ObjectSyncToRecord(void)
{
    uint8_t p = cpu.p;

    cpu.a = ObjectSyncToRecord(cpu.db, cpu.x, &cpu.y, &p);
    cpu.p = (uint8_t)(p | P_M);
}

/*
 * @routine 08:D01E RecordContactFlags
 * WHAT IT DOES: Adds to the flags in $1F38 what a record's contact word
 * ($1F3A, its +$64) and flags ($1F36, its +$16) say: when the contact word
 * is non-zero, 4 plus its lowest set bit of bits 0-2 (or the whole word
 * when none of them is set); 8 for flags bit 0; $80 for flags bit 11.
 * HOW: BIT #imm tests (Z only), ORs into $1F38 through the data bank.
 * Entry: M 16-bit, X 16-bit. Exit: A = $1F38; N, Z from it; C, V
 * unchanged.
 */
uint16_t RecordContactFlags(uint8_t bank)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t c = rd16(b | 0x1F3A), f;

    if (c) {
        if (c & 0x0001)
            c = 0x0001;
        else if (c & 0x0002)
            c = 0x0002;
        else if (c & 0x0004)
            c = 0x0004;
        wr16(b | 0x1F38, (uint16_t)(c | 0x0004 | rd16(b | 0x1F38)));
    }
    f = rd16(b | 0x1F36);
    if (f & 0x0001)
        wr16(b | 0x1F38, (uint16_t)(rd16(b | 0x1F38) | 0x0008));
    if (f & 0x0800)
        wr16(b | 0x1F38, (uint16_t)(rd16(b | 0x1F38) | 0x0080));
    return rd16(b | 0x1F38);
}

void abi_RecordContactFlags(void)
{
    cpu.a = RecordContactFlags(cpu.db);
    cpu.p = nz16(cpu.p, cpu.a);
}

/*
 * @routine 08:B402 GsuRecordLand
 * WHAT IT DOES: When $70:2482 is non-zero and object X's record has bit 6
 * of +$2C and bit 4 of +$8A set, and not bit 8 of +$40: stops the record
 * (+$2E, +$30, +$32, +$36, +$34, +$38 zeroed) and, unless +$82 is already
 * $200, sets +$82 to $200 and in +$40 clears bits 5-6 and sets bits 0 and
 * 12.
 * HOW: 16-bit tests and stores on the record (the object's +$22, used
 * without a zero test). The stopping part is not reached by any script.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: X unchanged; M 8-bit; N, Z
 * from X; A = the last value tested or written; C set by the +$82 compare
 * when that is reached, else unchanged.
 */
uint16_t GsuRecordLand(uint8_t bank, uint16_t obj, uint8_t *p)
{
    uint16_t rec, a;
    unsigned i;
    static const uint8_t stop[6] = {0x2E, 0x30, 0x32, 0x36, 0x34, 0x38};

    if (!(a = rd16(0x702482)))
        return a;
    rec = record_of(bank, obj);
    if (!(a = rd16(0x70002C + rec) & 0x0040))
        return a;
    if (!(a = rd16(0x70008A + rec) & 0x0010))
        return a;
    if ((a = rd16(0x700040 + rec) & 0x0100))
        return a;
    for (i = 0; i < 6; i++)
        wr16(0x700000 + stop[i] + rec, 0x0000);
    a = rd16(0x700082 + rec);
    *p = (uint8_t)((*p & ~P_C) | (a >= 0x0200 ? P_C : 0));
    if (a == 0x0200)
        return a;
    wr16(0x700082 + rec, 0x0200);
    a = (uint16_t)((rd16(0x700040 + rec) & 0xFF9F) | 0x1001);
    wr16(0x700040 + rec, a);
    return a;
}

void abi_GsuRecordLand(void)
{
    uint8_t p = cpu.p;

    cpu.a = GsuRecordLand(cpu.db, cpu.x, &p);
    cpu.p = (uint8_t)(nz16(p, cpu.x) | P_M);
}

/*
 * @routine 08:CF41 RecordNextContact
 * WHAT IT DOES: Walks the contact list from the entry at $1F3C (entries
 * are records linked through +$0E) to the next one with bit 1 of +$10
 * set; for it, takes the touched record (its +$0C) and makes $1F3A that
 * record's contact word (+$64, when bit 1 of its +$16 is set, else 0),
 * $1F36 its flags (+$16), $1F38 their RecordContactFlags, and leaves $1F3C
 * at the following entry. Carry set when one was found.
 * HOW: 16-bit walk with X; Y = the touched record.
 * Entry: M 8-bit, X 16-bit. Exit: M 8-bit; X unchanged. Found: carry set,
 * A = $1F38, Y = the touched record, N, Z from X. Not found: carry clear,
 * A = 0, Y unchanged; N, Z from X after a walk, Z set when $1F3C was 0.
 */
int RecordNextContact(uint8_t bank, uint16_t *a, uint16_t *y, uint8_t *p, uint16_t x)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t e = rd16(b | 0x1F3C), r;

    if (!e) {
        *a = 0;
        *p = (uint8_t)(nz16(*p, 0) & ~P_C);
        return 0;
    }
    while (!(rd16(0x700010 + e) & 0x0002)) {
        e = rd16(0x70000E + e);
        if (!e) {
            *a = 0;
            *p = (uint8_t)(nz16(*p, x) & ~P_C);
            return 0;
        }
    }
    r = rd16(0x70000C + e);
    wr16(b | 0x1F3C, rd16(0x70000E + e));
    wr16(b | 0x1F3A, 0x0000);
    if (rd16(0x700016 + r) & 0x0002)
        wr16(b | 0x1F3A, rd16(0x700064 + r));
    wr16(b | 0x1F36, rd16(0x700016 + r));
    wr16(b | 0x1F38, 0x0000);
    *a = RecordContactFlags(bank);
    *y = r;
    *p = (uint8_t)(nz16(*p, x) | P_C);
    return 1;
}

void abi_RecordNextContact(void)
{
    uint8_t p = cpu.p;

    RecordNextContact(cpu.db, &cpu.a, &cpu.y, &p, cpu.x);
    cpu.p = (uint8_t)(p | P_M);
}

/*
 * @routine 08:CF92 RecordContactSummary
 * WHAT IT DOES: Sums up every contact of object X's record into $1F38:
 * $10 when bit 0 of the record's +$1C is set ($80 more when bit 5 of
 * $70:26F2 is too), bit 5 of its +$2C, then RecordContactFlags over the
 * OR of the contact words ($1F3A) and flags ($1F36) of every touched record
 * (+$0C) in the contact list from its +$18 whose entries have bit 1 of +$10.
 * HOW: 16-bit walk with X through +$0E; Y keeps the entry while X is the
 * touched record. With no record it returns 0 at once.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; X unchanged; C, V
 * unchanged. Record: A = $1F38, N, Z from it; Y = the last entry with bit 1
 * set (unchanged when none). No record: A = 0, Z set.
 */
uint16_t RecordContactSummary(uint8_t bank, uint16_t obj, uint16_t *y)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t rec = record_of(bank, obj), e, r;

    if (!rec)
        return 0;
    wr16(b | 0x1F38, 0x0000);
    if (rd16(0x70001C + rec) & 0x0001) {
        wr16(b | 0x1F38, (uint16_t)(rd16(b | 0x1F38) | 0x0010));
        if (rd16(0x7026F2) & 0x0020)
            wr16(b | 0x1F38, (uint16_t)(rd16(b | 0x1F38) | 0x0080));
    }
    wr16(b | 0x1F38, (uint16_t)((rd16(0x70002C + rec) & 0x0020) | rd16(b | 0x1F38)));
    wr16(b | 0x1F36, 0x0000);
    wr16(b | 0x1F3A, 0x0000);
    for (e = rd16(0x700018 + rec); e; e = rd16(0x70000E + e)) {
        if (!(rd16(0x700010 + e) & 0x0002))
            continue;
        r = rd16(0x70000C + e);
        if (rd16(0x700016 + r) & 0x0002)
            wr16(b | 0x1F3A, (uint16_t)(rd16(0x700064 + r) | rd16(b | 0x1F3A)));
        wr16(b | 0x1F36, (uint16_t)(rd16(0x700016 + r) | rd16(b | 0x1F36)));
        *y = e;
    }
    return RecordContactFlags(bank);
}

void abi_RecordContactSummary(void)
{
    cpu.a = RecordContactSummary(cpu.db, cpu.x, &cpu.y);
    cpu.p = (uint8_t)(nz16(cpu.p, cpu.a) | P_M);
}

void SoundPort0QueuePush(uint8_t command);

/* CMP of two 16-bit values: N, Z, C. */
static uint8_t cmp16(uint8_t p, uint16_t a, uint16_t m)
{
    return (uint8_t)((nz16(p, (uint16_t)(a - m)) & ~P_C) | (a >= m ? P_C : 0));
}

#define SEQ_COUNT 0x1A33u
#define SEQ_STAGE 0x1A35u
#define SEQ_INPUT 0x1AF0u

/*
 * @routine 08:C8FF RecordButtonSequence
 * WHAT IT DOES: Watches the buttons in $1AF0 for a four-press sequence: bit
 * 6, then bit 7, then bit 14, each within 5 frames of the last, then bit 11
 * between 12 and 16 frames later. The full sequence plays sound $48 and
 * gives record X bit 13 of +$40, $180 at +$6A and $1000 at +$400. Too slow,
 * too early or done: back to the start.
 * HOW: $1A33 counts frames and $1A35 is the stage, both reset together;
 * the limits are signed compares (BPL/BMI). A stage above 3 gives the
 * reward at once. Only stages 0 and 1 are reached by a script.
 * Entry: M 16-bit, X 16-bit; X = record. Exit: X, Y unchanged; A and N, Z,
 * C as the last load or compare on the path taken; V unchanged.
 */
uint16_t RecordButtonSequence(uint8_t bank, uint16_t rec, uint8_t *p)
{
    static const uint16_t press[3] = {0x0040, 0x0080, 0x4000};
    uint32_t b = (uint32_t)bank << 16;
    uint16_t n, s, in;

    n = (uint16_t)(rd16(b | SEQ_COUNT) + 1);
    wr16(b | SEQ_COUNT, n);
    s = rd16(b | SEQ_STAGE);
    *p = nz16(*p, s);
    if (s != 0)
        *p = cmp16(*p, s, 0x0001);
    if (s != 0 && s != 1)
        *p = cmp16(*p, s, 0x0002);
    if (s <= 2) {
        *p = cmp16(*p, n, 0x0005);
        if (!(*p & P_N))
            goto reset;
        in = rd16(b | SEQ_INPUT);
        *p = nz16(*p, in);
        if (!(in & press[s])) {
            *p |= P_Z;
            return in;
        }
        wr16(b | SEQ_COUNT, 0x0000);
        wr16(b | SEQ_STAGE, (uint16_t)(rd16(b | SEQ_STAGE) + 1));
        *p = nz16(*p, rd16(b | SEQ_STAGE));
        return in;
    }
    *p = cmp16(*p, s, 0x0003);
    if (s == 3) {
        *p = cmp16(*p, n, 0x0011);
        if (!(*p & P_N))
            goto reset;
        in = rd16(b | SEQ_INPUT);
        *p = nz16(*p, in);
        if (!(in & 0x0800)) {
            *p |= P_Z;
            return in;
        }
        n = rd16(b | SEQ_COUNT);
        *p = cmp16(*p, n, 0x000C);
        if (*p & P_N)
            goto reset;
    }
    SoundPort0QueuePush(0x48);
    wr16(0x700040 + rec, (uint16_t)(rd16(0x700040 + rec) | 0x2000));
    wr16(0x70006A + rec, 0x0180);
    wr16(0x700400 + rec, 0x1000);
    *p = nz16(*p, 0x1000);
    n = 0x1000;
reset:
    wr16(b | SEQ_COUNT, 0x0000);
    wr16(b | SEQ_STAGE, 0x0000);
    return n;
}

void abi_RecordButtonSequence(void)
{
    uint8_t p = cpu.p;

    cpu.a = RecordButtonSequence(cpu.db, cpu.x, &p);
    cpu.p = p;
}
