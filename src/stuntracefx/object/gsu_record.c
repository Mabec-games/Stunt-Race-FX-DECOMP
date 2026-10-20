/*
 * gsu_record.c: each object's record in Super FX RAM, which the renderer
 * reads. An object's +$22 (bank $7E, through the data bank) holds the
 * record's address in bank $70, or 0 when it has none. Fields set here:
 * +$0C/+$0E/+$10 the position, +$16 flags, +$1A a mode word.
 */
#include "machine.h"
#include "cpu_ops.h"

void abi_ObjectChildFollow(void);


/* The object's record address, read through the data bank. */
static uint16_t record_of(uint8_t bank, uint16_t obj)
{
    cyc(46);
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
    } else { cyc(102); }
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
    cyc(6);
    uint16_t rec = record_of(bank, obj);

    if (rec)
        { cyc(116); wr16(0x70001A + rec, 0x0002); }
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
    cyc(6);
    uint16_t rec = record_of(bank, obj);

    if (rec) {
        cyc(92);
        *a = (uint16_t)(rd16(0x700016 + rec) | rd16(cyc_then(16, 0x000002)));
        cyc(32);
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
    cyc(6);
    uint16_t rec = record_of(bank, obj);

    if (rec) {
        cyc(92);
        *a = (uint16_t)((rd16(0x700016 + rec) | 0x0010) & 0xFEFF);
        cyc(80);
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
    cyc(6);
    uint16_t rec = record_of(bank, obj);

    if (rec) {
        cyc(76);
        wr16(0x70000C + rec, cyc_then(32, rd16(0x000002)));
        cyc(16);
        wr16(0x70000E + rec, cyc_then(32, rd16(0x000008)));
        cyc(16);
        *a = rd16(0x000094);
        cyc(32);
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
    cyc(30);
    *y = rd16((uint32_t)bank << 16 | (uint16_t)(0x2006 + obj));
    cyc(36);
    return cyc_then(114, (uint16_t)(rd16(cyc_then(46, 0x700088 + record_of(bank, *y))) - 1));
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

    cyc(30);
    *y = rd16((uint32_t)bank << 16 | (uint16_t)(0x2006 + obj));
    cyc(36);
    rec = record_of(bank, *y);
    cyc(46);
    *clear = !(rd16(0x700042 + rec) & 0x0410);
    cyc(*clear ? 78 : 72);              /* AND #, BEQ (taken 22, or 16), LDA long */
    return rd16(0x700058 + rec);
}

void abi_GsuRecordWord58(void)
{
    int clear;

    cpu.a = GsuRecordWord58(cpu.db, cpu.x, &cpu.y, &clear);
    cpu.p = (uint8_t)((nz16(cpu.p, cpu.x) & ~P_C) | (clear ? P_C : 0) | P_M);
    cyc(114);
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

    cyc(30);
    *has = (rd8(b | (uint16_t)(0x2022 + obj)) | rd8(cyc_then(30, b | (uint16_t)(0x2023 + obj)))) != 0;
    if (!*has) {
        cyc(62);
        wr8(b | (uint16_t)(0x2036 + obj), 0xFF);
        cyc(8);
        return 0x00FF;
    } else { cyc(28); }
    rec = record_of(bank, obj);
    cyc(76);
    w = (uint16_t)(rd16(0x70001C + rec) | 0x0004);
    cyc(56);
    wr16(0x70001C + rec, w);
    return (uint16_t)(w & 0xFF00);
}

void abi_GsuRecordFlag4(void)
{
    int has;
    uint16_t a = GsuRecordFlag4(cpu.db, cpu.x, &has);

    if (has) {
        cyc(8);
        cpu.a = a;
        cpu.y = cpu.x;
        cpu.p = (uint8_t)((cpu.p & ~P_N) | P_Z | P_M);
    } else {
        cyc(8);
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0xFF);
        cpu.p = (uint8_t)((cpu.p & ~P_Z) | P_N | P_M);
    }
    cyc(88);
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
    cyc(6);
    uint16_t rec = record_of(bank, obj), v = 0, w;
    unsigned i;

    *has = rec != 0;
    if (!rec)
        { cyc(8); return 0x00FF; } else { cyc(44); }
    for (i = 0; i < 3; i++) {
        cyc(30);
        w = rd16(b | (uint16_t)(0x202E + 2 * i + obj));
        *p = (uint8_t)((*p & ~P_C) | (w & 0x2000 ? P_C : 0));
        v = (uint16_t)(w << 3);
        cyc(74);
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
    cyc(96);
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
    cyc(136);
    uint16_t n = rd16(0x7EFFB8), x = 0, v = 0, c;

    cyc(54);
    wr16(0x000002, (uint16_t)((n >> 1) | (n & 0x8000)));
    cyc(2);
    do {
        cyc(54);
        uint16_t obj = rd16(0x7EFFAE + x);

        cyc(76);
        v = rd16(cyc_then(46, 0x700088 + rd16(0x7E2022 + obj)));
        cyc(68);
        wr16(0x7EFFA6 + x, v);
        x = (uint16_t)(x + 2);
        cyc(44);
        c = rd16(0x000002);
        c = (uint16_t)(c - 1);
        cyc(14);
        wr16(0x000002, c);
    } while (c);
    cyc(160);
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

    cyc(24);
    *ran = rd8(b | 0x0D62) != 0;
    if (!*ran)
        { cyc(64); return 0; } else { cyc(52); }
    rec = record_of(bank, obj);
    cyc(46);
    f = rd16(0x700042 + rec);
    if (f & 0x0400)
        { cyc(46); return f; }
    cyc(88);
    wr16(b | 0x1F2B, 0x0001);
    cyc(32);
    if (rd16(0x700088 + rec) != 0x0001)
        { cyc(88); wr16(b | 0x1F2B, 0x0003); } else { cyc(46); }
    cyc(32);
    a = rd16(0x70006A + rec);
    cyc(38);
    step = rd16(b | 0x1F2B);
    s = (uint16_t)(a + step);
    *p = (uint8_t)((*p & ~(P_C | P_V)) | ((~(a ^ step) & (a ^ s) & 0x8000) ? P_V : 0) | (s >= 0x0180 ? (cyc(18), P_C) : 0));
    if (!((uint16_t)(s - 0x0180) & 0x8000))
        s = 0x0180;
    cyc(78);
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
    } else { cyc(100); }
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
    cyc(66);
    uint16_t rec = record_of(bank, from), v = 0;
    unsigned i;

    cyc(28);
    for (i = 0; i < 3; i++) {
        cyc(32);
        v = asr3(rd16(0x70002E + 2 * i + rec), p);
        cyc(144);
        wr16(b | (uint16_t)(0x202E + 2 * i + obj), v);
    }
    cyc(138);
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
    cyc(104);
    uint16_t rec = record_of(bank, obj), v;
    unsigned i;

    if (!rec)
        return 0; else { cyc(30); }
    for (i = 0; i < 7; i++)
        { cyc(30); wr16(0x700000 + copy[i].to + rec, cyc_then(32, rd16(b | (uint16_t)(copy[i].from + obj)))); }
    cyc(30);
    wr16(0x700024 + rec, cyc_then(76, (uint16_t)((rd16(b | (uint16_t)(0x2015 + obj)) & 0x00FF) << 8)));
    cyc(30);
    v = rd16(b | (uint16_t)(0x205A + obj)) & 0x007F;
    cyc(56);
    wr16(0x700012 + rec, v);
    cyc(80);
    return v;
}

void abi_GsuRecordFromObject(void)
{
    cpu.a = GsuRecordFromObject(cpu.db, cpu.x);
    cpu.y = cpu.x;
    cpu.p = (uint8_t)(nz16(cpu.p, cpu.x) | P_M);
}

/*
 * @routine 08:D8C2 GsuRecordPositionFromObject
 * WHAT IT DOES: GsuRecordFromObject's position copy alone: object X's
 * position (+$0C, +$0E, +$10) to its Super FX record's +$1E, +$20, +$22.
 * Nothing in the game calls it; a direct call is its only way in.
 * HOW: 16-bit copies, when the object has a record.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: A = the last value (0 with no
 * record); X = Y = the object; M 8-bit; N, Z from X.
 */
void abi_GsuRecordPositionFromObject(void)
{
    static const uint16_t from[3] = {0x200C, 0x200E, 0x2010};
    uint32_t b = (uint32_t)cpu.db << 16;
    cyc(20);
    uint16_t rec = record_of(cpu.db, cpu.x), v = 0;
    unsigned i;

    cyc(30);
    for (i = 0; rec && (cyc(8), i < 3); i++) {
        cyc(22);
        v = rd16(b | (uint16_t)(from[i] + cpu.x));
        cyc(32);
        wr16(0x70001E + 2 * i + rec, v);
    }
    cyc(72);
    cpu.a = v;
    cpu.y = cpu.x;
    cpu.p = (uint8_t)(nz16(cpu.p, cpu.x) | P_M);
}

uint16_t ScaleThreeQuarters(uint16_t v);

/* ScaleThreeQuarters with the flags of its final add. */
static uint16_t three_quarters(uint16_t v, uint8_t *p)
{
    uint16_t half = (uint16_t)((v >> 1) | (v & 0x8000)), quarter = (uint16_t)((half >> 1) | (half & 0x8000));
    cyc(176);
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
    cyc(30);
    uint16_t rec = record_of(bank, cyc_then(6, rd16(b | (uint16_t)(0x2027 + obj)))), v;

    cyc(60);
    v = three_quarters((uint16_t)((int16_t)rd16(0x70002E + rec) >> 3), p);
    cyc(30);
    wr16(b | (uint16_t)(0x202E + obj), v);
    cyc(32);
    v = three_quarters((uint16_t)((int16_t)rd16(0x700032 + rec) >> 3), p);
    cyc(30);
    wr16(b | (uint16_t)(0x2032 + obj), v);
    cyc(78);
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

    cyc(178);
    v = rd16(cyc_then(46, 0x700040 + rd16(0x702476))) & 0x0060;
    cyc(48);
    wr16(0x091A24, v);
    cyc(32);
    v = rd16(cyc_then(46, 0x700040 + rd16(0x702478))) & 0x0060;
    cyc(48);
    wr16(0x091A26, v);
    cyc(136);
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
static uint8_t record_new(uint8_t bank, uint16_t obj, uint16_t program, uint16_t word0a, uint16_t join)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t rec;

    cyc(100);
    GsuRun(0x04, program);
    cyc(214);
    wr16(b | RECORD_0A_WORD, word0a);
    cyc(join); /* SEP, BRL/JMP/BRA, REP, LDA long */
    rec = rd16(GSU_RECORD_ARG);
    cyc(30);
    wr16(b | (uint16_t)(0x2022 + obj), rec);
    if (!rec)
        { cyc(68); return 0xFF; }
    cyc(104);
    wr16(0x70000A + rec, cyc_then(32, rd16(b | RECORD_0A_WORD)));
    cyc(36);
    GsuRecordFromObject(bank, obj);
    return 0x00;
}

/* Exit: A = 0 (made) or $00FF (no room), N and Z from that byte; M 8-bit;
   with a record Y = the object, otherwise Y unchanged; X unchanged. */
static void abi_record_new(uint16_t program, uint16_t word0a, uint16_t join)
{
    uint8_t r = record_new(cpu.db, cpu.x, program, word0a, join);

    if (!r)
        { cyc(0); cpu.y = cpu.x; }
    cyc(60);
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
    return record_new(bank, obj, 0x8B64, 0x0000, 106);
}

void abi_GsuRecordNew422(void)
{
    cyc(62);
    abi_record_new(0x8B64, 0x0000, 106);
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
    return record_new(bank, obj, 0x8C95, 0x90E8, 100);
}

void abi_GsuRecordNew42D(void)
{
    cyc(62);
    abi_record_new(0x8C95, 0x90E8, 100);
    cyc(62);
}

/*
 * @routine 08:CBCC GsuRecordNew424Piece
 * WHAT IT DOES: As GsuRecordNew424, with +$0A = $907A: the record of a
 * flying piece or a firework shell (bank 09's course objects).
 * HOW: Runs Super FX program $04:8C0A; then the shared tail at $08:CC7C.
 * Entry: as GsuRecordNew422. Exit: as GsuRecordNew422.
 */
uint8_t GsuRecordNew424Piece(uint8_t bank, uint16_t obj)
{
    return record_new(bank, obj, 0x8C0A, 0x907A, 100);
}

void abi_GsuRecordNew424Piece(void)
{
    cyc(62);
    abi_record_new(0x8C0A, 0x907A, 100);
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
    return record_new(bank, obj, 0x8C0A, 0x910A, 98);
}

void abi_GsuRecordNew424(void)
{
    cyc(62);
    abi_record_new(0x8C0A, 0x910A, 98);
}

/*
 * @routine 08:CC18 GsuRecordNew424Alt
 * WHAT IT DOES: As GsuRecordNew424, with +$0A = $90E8.
 * HOW: Runs Super FX program $04:8C0A; then the shared tail at $08:CC7C.
 * Entry: as GsuRecordNew422. Exit: as GsuRecordNew422.
 */
uint8_t GsuRecordNew424Alt(uint8_t bank, uint16_t obj)
{
    return record_new(bank, obj, 0x8C0A, 0x90E8, 98);
}

void abi_GsuRecordNew424Alt(void)
{
    cyc(62);
    abi_record_new(0x8C0A, 0x90E8, 98);
}

/*
 * @routine 08:CC4A GsuRecordNewCamera
 * WHAT IT DOES: As GsuRecordNew424, from Super FX program $04:8C6C, with
 * +$0A = $916D: the record of a replay camera's follower.
 * HOW: Runs Super FX program $04:8C6C; then the shared tail at $08:CC7C.
 * Entry: as GsuRecordNew422. Exit: as GsuRecordNew422.
 */
uint8_t GsuRecordNewCamera(uint8_t bank, uint16_t obj)
{
    return record_new(bank, obj, 0x8C6C, 0x916D, 98);
}

void abi_GsuRecordNewCamera(void)
{
    cyc(62);
    abi_record_new(0x8C6C, 0x916D, 98);
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
    return record_new(bank, obj, 0x8B33, 0x910A, 98);
}

void abi_GsuRecordNew426(void)
{
    cyc(62);
    abi_record_new(0x8B33, 0x910A, 98);
}

int ObjectFlag40IfMatch(uint8_t bank, uint16_t a, uint16_t *x, uint8_t *v, uint8_t *p);

/* The shared part of the two routines below: the record and the amount (as
   the high byte) go to the Super FX, ObjectFlag40IfMatch flags the object
   at $0346 when the record is $70:2476's, and the program runs. Returns A
   as the original leaves it: Y's high byte over the screen mode value. */
static uint16_t record_amount_go(uint8_t bank, uint16_t rec, uint16_t program, uint8_t *p)
{
    cyc(46);
    uint16_t x = 0;
    uint8_t v = 0;

    ObjectFlag40IfMatch(bank, rec, &x, &v, p);
    cyc(166);
    return cyc_then(146, (uint16_t)((rec & 0xFF00) | GsuRun(0x04, program)));
}

static uint16_t record_amount_run(uint8_t bank, uint16_t rec, uint8_t amount, uint16_t program, uint8_t *p)
{
    cyc(128);
    wr16(GSU_IN_AMOUNT, (uint16_t)(amount << 8));
    cyc(46);
    wr16(GSU_RECORD_ARG, rec);
    return record_amount_go(bank, rec, program, p);
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

/* PHX, PHY, TYX, REP #$20, AND #$00FF, then the record's +$8A word set or
   cleared by the bits in A, SEP #$20, PLY, PLX: A = the new word; N, Z
   from X. */
static void flags8a(int set)
{
    uint32_t at = (0x70008A + cpu.y) & 0xFFFFFF;
    uint16_t a = cpu.a & 0x00FF;

    cpu.p &= (uint8_t)~P_M;
    if (set) {
        cyc(210);
        a = (uint16_t)(a | rd16(at));
    } else {
        a ^= 0x00FF;
        cyc(246);
        a = (uint16_t)(a & rd16(at));
    }
    cyc(32);
    wr16(at, a);
    cpu.a = a;
    cpu.p |= P_M;
    cpu.p = (cpu.p & P_X) ? nz8(cpu.p, (uint8_t)cpu.x) : (cyc(160), nz16(cpu.p, cpu.x));
}

/*
 * @routine 08:B72E GsuRecordFlags8ASet
 * WHAT IT DOES: Sets the bits in A's low byte in the word at +$8A of the
 * Super FX record Y.
 * HOW: The record in X (pushed and pulled around it); 16-bit long
 * accesses.
 * Entry: X 16-bit; Y = record; A low byte = bits. Exit: A = the new word;
 * M 8-bit; X, Y unchanged; N, Z from X.
 */
void abi_GsuRecordFlags8ASet(void)
{
    flags8a(1);
}

/*
 * @routine 08:B743 GsuRecordFlags8AClear
 * WHAT IT DOES: Clears the bits in A's low byte in the word at +$8A of the
 * Super FX record Y.
 * HOW: As GsuRecordFlags8ASet, with EOR #$00FF and AND.
 * Entry: X 16-bit; Y = record; A low byte = bits. Exit: A = the new word;
 * M 8-bit; X, Y unchanged; N, Z from X.
 */
void abi_GsuRecordFlags8AClear(void)
{
    flags8a(0);
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
    /* the inputs are stored before the flag is read, and the record is
       read back from $70:238E for the match */
    cyc(128);
    wr16(GSU_IN_AMOUNT, (uint16_t)(amount << 8));
    cyc(46);
    wr16(GSU_RECORD_ARG, rec);
    cyc(46);
    if (rd16(0x700040 + rec) & 0x0001)
        return 0x0001;
    cyc(72);
    return record_amount_go(bank, rd16(GSU_RECORD_ARG), 0xBF51, p);
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

    cyc(54);
    wr8(b | (uint16_t)(0x2016 + obj), cyc_then(30, rd8(0x700029 + rec)));
    cyc(32);
    wr8(b | (uint16_t)(0x2015 + obj), cyc_then(30, rd8(0x700025 + rec)));
    cyc(76);
    wr8(b | 0x1F5A, cyc_then(54, (uint8_t)-rd8(0x001A75)));
    cyc(22);
    wr8(b | 0x1F2B, cyc_then(24, rd8(0x001A77)));
    cyc(66);
    sum = (uint8_t)(rd8(b | (uint16_t)(0x2016 + obj)) + rd8(cyc_then(38, b | 0x1F2B)));
    a = (uint8_t)((sum >> 1) | (sum & 0x80));
    if (a & 0x80)
        { cyc(70); a = (uint8_t)-(uint8_t)((uint8_t)-a & 0xFC); }
    a &= 0xFC;
    if (!((uint8_t)(a - 0x08) & 0x80))
        { cyc(10); a = 0x08; }
    if ((uint8_t)(a - 0xF8) & 0x80)
        { cyc(10); a = 0xF8; }
    cyc(168);
    wr8(b | 0x1F58, a);
    cyc(30);
    t = (uint8_t)(rd8(b | (uint16_t)(0x2015 + obj)) - rd8(cyc_then(38, b | 0x1F5A)));
    if (t & 0x80)
        { cyc(24); t = (uint8_t)-t; }
    cyc(60);
    if (!((uint8_t)(t - 0x40) & 0x80))
        { cyc(18); wr8(b | 0x1F58, cyc_then(54, (uint8_t)-rd8(b | 0x1F58))); }
    cyc(46);
    w = rd16(b | 0x1F58);
    w = (w & 0x0080) ? (cyc(16), (uint16_t)(w | 0xFF00)) : (uint16_t)(w & 0x00FF);
    cyc(152);
    w = (uint16_t)(w + 8 + rd16(b | (uint16_t)(0x2029 + obj)));
    cyc(30);
    wr16(b | (uint16_t)(0x205D + obj), w);
    ObjectTableC056(bank, obj, p);
    cyc(24);
    a = (uint8_t)(rd8(b | 0x1F5A) | 0x80);
    cyc(46);
    wr8(b | (uint16_t)(0x2018 + obj), a);
    cyc(24);
    a = rd8(b | 0x0D62);
    if (!a) {
        cyc(40);
        *p = nz8(*p, 0);
        return (uint16_t)(w & 0xFF00);
    }
    cyc(92);
    wr8(b | 0x1F5A, cyc_then(54, (uint8_t)-rd8(0x001AB0)));
    cyc(14);
    ObjectTableC056(bank, obj, p);
    cyc(24);
    a = (uint8_t)(rd8(b | 0x1F5A) | 0x80);
    cyc(46);
    wr8(b | (uint16_t)(0x2019 + obj), a);
    *p = nz8(*p, a);
    cyc(18);
    return (uint16_t)((w & 0xFF00) | a);
}

void abi_ObjectTurnFromRecord(void)
{
    uint8_t p = cpu.p;

    cpu.a = ObjectTurnFromRecord(cpu.db, cpu.x, cpu.y, &p);
    cpu.p = (uint8_t)(p | P_M);
    cyc(24);
}

uint16_t ObjectRebucket(uint16_t obj);

/* The tail of the two routines below ($08:CE60): Y = $1F3C (the record's
   +$18); A's low byte becomes 1 when it is non-zero. */
static uint16_t sync_tail(uint8_t bank, uint16_t a, uint16_t *y, uint8_t *p)
{
    cyc(24);
    *y = rd16((uint32_t)bank << 16 | 0x1F3C);
    if (!*y) {
        *p = nz16(*p, 0);
        return a;
    } else { cyc(10); }
    *p = nz8(*p, 0x01);
    return (uint16_t)((a & 0xFF00) | 0x01);
}

/* No record ($08:CE69): $1F3C = 0, A = 0, carry set. */
static uint16_t sync_none(uint8_t bank, uint8_t *p)
{
    cyc(92);
    wr16((uint32_t)bank << 16 | 0x1F3C, 0x0000);
    *p = (uint8_t)(nz8(*p, 0) | P_C);
    cyc(38);
    return 0x0000;
}

/* The steps both routines start with: ObjectRebucket, GsuRecordFlag4. */
static uint16_t sync_start(uint8_t bank, uint16_t obj)
{
    int has;

    cyc(62);
    ObjectRebucket(obj);
    cyc(62);
    GsuRecordFlag4(bank, obj, &has);
    cyc(116);
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
    uint16_t rec, a;
    uint8_t f;
    int has;

    /* Rebucket, GsuRecordFlag4, then +$24 bit 1 cleared before the record
       (+$22) is read, in the asm's order. */
    cyc(62);
    ObjectRebucket(obj);
    cyc(62);
    GsuRecordFlag4(bank, obj, &has);
    cyc(126);
    wr8(b | (uint16_t)(0x2024 + obj), cyc_then(46, rd8(b | (uint16_t)(0x2024 + obj)) & 0xFD));
    cyc(20);
    rec = record_of(bank, obj);
    if (!rec) {
        *y = obj;
        return sync_none(bank, p);
    }
    cyc(62);
    wr16(b | 0x1F3C, cyc_then(24, rd16(0x700018 + rec)));
    cyc(30);
    wr16(0x700024 + rec, cyc_then(84, (uint16_t)(rd8(b | (uint16_t)(0x2015 + obj)) << 8)));
    cyc(30);
    wr16(0x700012 + rec, cyc_then(56, rd16(b | (uint16_t)(0x205A + obj)) & 0x007F));
    cyc(32);
    wr16(b | (uint16_t)(0x200C + obj), cyc_then(30, rd16(0x70001E + rec)));
    cyc(32);
    wr16(b | (uint16_t)(0x200E + obj), cyc_then(30, rd16(0x700020 + rec)));
    cyc(32);
    wr16(b | (uint16_t)(0x2010 + obj), cyc_then(30, rd16(0x700022 + rec)));
    cyc(32);
    a = rd16(0x70001C + rec) & 0x0002;
    if (a) {
        cyc(106);
        f = (uint8_t)(rd8(b | (uint16_t)(0x2024 + obj)) | 0x02);
        cyc(46);
        wr8(b | (uint16_t)(0x2024 + obj), f);
        a = f;
    } else { cyc(82); }
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
    cyc(0);
    uint16_t rec = sync_start(bank, obj), a;

    if (!rec) {
        *y = obj;
        return sync_none(bank, p);
    }
    cyc(60);
    wr16(0x700024 + rec, cyc_then(84, (uint16_t)(rd8(b | (uint16_t)(0x2015 + obj)) << 8)));
    cyc(30);
    wr16(0x700012 + rec, cyc_then(56, rd16(b | (uint16_t)(0x205A + obj)) & 0x007F));
    cyc(32);
    wr16(b | 0x1F3C, cyc_then(24, rd16(0x700018 + rec)));
    cyc(30);
    wr16(0x70001E + rec, cyc_then(32, rd16(b | (uint16_t)(0x200C + obj))));
    cyc(30);
    wr16(0x700020 + rec, cyc_then(32, rd16(b | (uint16_t)(0x200E + obj))));
    cyc(30);
    a = rd16(b | (uint16_t)(0x2010 + obj));
    cyc(32);
    wr16(0x700022 + rec, a);
    *p &= (uint8_t)~P_C;
    cyc(60);
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
    cyc(24);
    uint16_t c = rd16(b | 0x1F3A), f;

    if (c) {
        if (c & 0x0001)
            c = 0x0001;
        else if (c & 0x0002)
            { cyc(46); c = 0x0002; }
        else if (c & 0x0004)
            { cyc(92); c = 0x0004; } else { cyc(52); }
        cyc(150);
        wr16(b | 0x1F38, cyc_then(24, (uint16_t)(c | 0x0004 | rd16(b | 0x1F38))));
    } else { cyc(22); }
    cyc(24);
    f = rd16(b | 0x1F36);
    if (f & 0x0001)
        { cyc(64); wr16(b | 0x1F38, cyc_then(48, (uint16_t)(rd16(b | 0x1F38) | 0x0008))); } else { cyc(46); }
    cyc(86);
    if (f & 0x0800)
        { cyc(18); wr16(b | 0x1F38, cyc_then(48, (uint16_t)(rd16(b | 0x1F38) | 0x0080))); }
    cyc(24);
    return cyc_then(42, rd16(b | 0x1F38));
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

    cyc(130);
    if (!(a = rd16(0x702482)))
        { cyc(22); return a; }
    rec = record_of(bank, obj);
    cyc(46);
    if (!(a = rd16(0x70002C + rec) & 0x0040))
        { cyc(46); return a; }
    cyc(72);
    if (!(a = rd16(0x70008A + rec) & 0x0010))
        { cyc(46); return a; }
    cyc(72);
    if ((a = rd16(0x700040 + rec) & 0x0100))
        { cyc(46); return a; } else { cyc(64); }
    for (i = 0; i < 6; i++)
        { cyc(32); wr16(0x700000 + stop[i] + rec, 0x0000); }
    cyc(32);
    a = rd16(0x700082 + rec);
    *p = (uint8_t)((*p & ~P_C) | (a >= 0x0200 ? (cyc(46), P_C) : 0));
    if (a == 0x0200)
        return a;
    cyc(96);
    wr16(0x700082 + rec, 0x0200);
    cyc(32);
    a = (uint16_t)((rd16(0x700040 + rec) & 0xFF9F) | 0x1001);
    cyc(80);
    wr16(0x700040 + rec, a);
    return a;
}

void abi_GsuRecordLand(void)
{
    uint8_t p = cpu.p;

    cpu.a = GsuRecordLand(cpu.db, cpu.x, &p);
    cpu.p = (uint8_t)(nz16(p, cpu.x) | P_M);
    cyc(100);
}

/*
 * @routine 08:CF0B ObjectSyncAngles
 * WHAT IT DOES: As ObjectSyncToRecord without the position: the object is
 * rebucketed, its record marked (GsuRecordFlag4) and given its heading
 * (+$15 as the high byte of +$24) and +$12 (+$5A's low 7 bits), $1F3C =
 * the record's +$18.
 * HOW: As ObjectSyncToRecord, ending in the tail at $08:CE60 (or $08:CE69
 * with no record).
 * Entry: as ObjectSyncFromRecord. Exit: as ObjectSyncFromRecord, except
 * that with a record and $1F3C zero A = the record's +$18.
 */
uint16_t ObjectSyncAngles(uint8_t bank, uint16_t obj, uint16_t *y, uint8_t *p)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t rec = sync_start(bank, obj), a;

    if (!rec) {
        cyc(24);
        *y = obj;
        return sync_none(bank, p);
    }
    cyc(66);
    wr16(0x700024 + rec, cyc_then(84, (uint16_t)(rd8(b | (uint16_t)(0x2015 + obj)) << 8)));
    cyc(30);
    wr16(0x700012 + rec, cyc_then(56, rd16(b | (uint16_t)(0x205A + obj)) & 0x007F));
    cyc(32);
    a = rd16(0x700018 + rec);
    cyc(24);
    wr16(b | 0x1F3C, a);
    *p &= (uint8_t)~P_C;
    cyc(60);
    return sync_tail(bank, a, y, p);
}

void abi_ObjectSyncAngles(void)
{
    uint8_t p = cpu.p;

    cpu.a = ObjectSyncAngles(cpu.db, cpu.x, &cpu.y, &p);
    cpu.p = (uint8_t)(p | P_M);
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
    cyc(46);
    uint16_t e = rd16(b | 0x1F3C), r;

    if (!e) {
        cyc(96);
        *a = 0;
        *p = (uint8_t)(nz16(*p, 0) & ~P_C);
        return 0;
    } else { cyc(24); }
    while (!(rd16(cyc_then(68, 0x700010 + e)) & 0x0002)) {
        cyc(72);
        e = rd16(0x70000E + e);
        if (!e) {
            *a = 0;
            *p = (uint8_t)(nz16(*p, x) & ~P_C);
            return 0;
        }
    }
    cyc(78);
    r = rd16(0x70000C + e);
    cyc(46);
    wr16(b | 0x1F3C, cyc_then(24, rd16(0x70000E + e)));
    cyc(38);
    wr16(b | 0x1F3A, 0x0000);
    cyc(32);
    if (rd16(0x700016 + r) & 0x0002)
        { cyc(72); wr16(b | 0x1F3A, cyc_then(24, rd16(0x700064 + r))); } else { cyc(46); }
    cyc(32);
    wr16(b | 0x1F36, cyc_then(24, rd16(0x700016 + r)));
    cyc(24);
    wr16(b | 0x1F38, 0x0000);
    cyc(46);
    *a = RecordContactFlags(bank);
    *y = r;
    *p = (uint8_t)(nz16(*p, x) | P_C);
    cyc(116);
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
    uint32_t b = (uint32_t)bank << 16, t;
    cyc(6);
    uint16_t rec = record_of(bank, obj), e, r, f;

    if (!rec)
        { cyc(62); return 0; }
    cyc(90);
    wr16(b | 0x1F38, 0x0000);
    cyc(32);
    if (rd16(0x70001C + rec) & 0x0001) {
        cyc(64);
        wr16(b | 0x1F38, cyc_then(48, (uint16_t)(rd16(b | 0x1F38) | 0x0010)));
        cyc(32);
        if (rd16(0x7026F2) & 0x0020)
            { cyc(64); wr16(b | 0x1F38, cyc_then(48, (uint16_t)(rd16(b | 0x1F38) | 0x0080))); } else { cyc(46); }
    } else { cyc(46); }
    cyc(32);
    f = rd16(0x70002C + rec) & 0x0020;
    cyc(48);
    wr16(b | 0x1F38, cyc_then(24, (uint16_t)(f | rd16(b | 0x1F38))));
    cyc(24);
    wr16(b | 0x1F36, 0x0000);
    cyc(24);
    wr16(b | 0x1F3A, 0x0000);
    cyc(32);
    for (e = rd16(0x700018 + rec), t = 0; ; e = rd16(cyc_then(46, 0x70000E + e)), t = 22) {
        /* BRA back (22), TAX and BEQ (22 taken), the LDA long's opcode */
        cyc(t + (e ? 62 : 36));
        if (!e)
            break;
        if (!(rd16(0x700010 + e) & 0x0002))
            { cyc(32); continue; }
        cyc(72);
        r = rd16(0x70000C + e);
        /* the flags are read again only after +$64, as the original does */
        cyc(60);
        f = rd16(0x700016 + r);
        if (f & 0x0002) {
            cyc(72);
            f = rd16(0x700064 + r);
            cyc(24);
            wr16(b | 0x1F3A, cyc_then(24, (uint16_t)(f | rd16(b | 0x1F3A))));
            cyc(32);
            f = rd16(0x700016 + r);
        } else { cyc(46); }
        cyc(24);
        wr16(b | 0x1F36, cyc_then(24, (uint16_t)(f | rd16(b | 0x1F36))));
        *y = e;
    }
    cyc(82);
    return RecordContactFlags(bank);
}

void abi_RecordContactSummary(void)
{
    cpu.a = RecordContactSummary(cpu.db, cpu.x, &cpu.y);
    cpu.p = (uint8_t)(nz16(cpu.p, cpu.a) | P_M);
    cyc(66);
}

void SoundPort0QueuePush(uint8_t command);

/* CMP of two 16-bit values: N, Z, C. */
static uint8_t cmp16(uint8_t p, uint16_t a, uint16_t m)
{
    return (uint8_t)((nz16(p, (uint16_t)(a - m)) & ~P_C) | (a >= m ? (cyc(24), P_C) : 0));
}

#define SEQ_COUNT 0x1A33u
#define SEQ_STAGE 0x1A35u
#define SEQ_INPUT 0x1AF0u

void abi_RecordButtonSequence(void);
void abi_RandomStepLong(void);
void abi_SoundPort0QueuePush(void);
void abi_ObjectLinkedHandlerByState(void);

#define SEQ_PREV 0x1AF2u
#define CTL_PLAYER 0x1A5Du

/* An object's field at DB:at + X, as abs,X reaches it ($2000 + offset). */
static uint32_t ctl_field(uint32_t b, uint16_t obj, uint16_t off)
{
    return b + off + obj;
}

/* BIT # (16-bit or 8-bit): Z from A AND m. */
static void ctl_bit(uint16_t m)
{
    cpu.p = (uint8_t)((cpu.p & ~P_Z) | ((cpu.a & m) ? (cyc(38), 0) : (cyc(38), P_Z)));
}

/* LDA abs; BIT #m: is button m held (in $1AF0, or $1AF2 last frame)? */
static int ctl_held(uint32_t b, uint16_t at, uint16_t m)
{
    cyc(24);
    op_lda16(b | at);
    cyc(8);
    ctl_bit(m);
    return (cpu.a & m) != 0;
}

/* LDA long; ORA #m; STA long: a record word gains bits. */
static void ctl_rec_or(uint32_t w, uint16_t m)
{
    cyc(32);
    cpu.a = (uint16_t)(rd16(w) | m);
    cpu.p = nz16(cpu.p, cpu.a);
    cyc(56);
    wr16(w, cpu.a);
}

/* LDA abs,Y; ORA #m; STA abs,Y (8-bit): object Y's +$25 gains bits. */
static void ctl_flags_or(uint32_t b, uint8_t m)
{
    uint32_t w = ctl_field(b, cpu.y, 0x2025);
    cyc(30);
    uint8_t v = (uint8_t)(rd8(w) | m);

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, v);
    cyc(46);
    wr8(w, v);
}

/* PHY; the linked object (+$4F) into Y; its +$25 flags; PLY. */
static void ctl_linked_flags(uint32_t b, uint8_t m1, uint8_t m2)
{
    uint16_t y = cpu.y;

    op_lda16(cyc_then(60, ctl_field(b, cpu.y, 0x204F)));
    op_tay16();
    op_sep(P_M);
    cyc(36);
    ctl_flags_or(b, m1);
    if (m2)
        ctl_flags_or(b, m2);
    cyc(28);
    op_rep(P_M);
    cpu.y = y;
    cpu.p = nz16(cpu.p, y);
}

/* The player's flag word: $1A60 for player 2 ($1A5D set), else $1A5E. */
static uint16_t ctl_player_word(uint32_t b)
{
    cyc(46);
    op_lda16(b | CTL_PLAYER);
    op_and16(0x00FF);
    return cpu.a ? 0x1A60 : (cyc(6), 0x1A5E);
}

/* PHA ... PLA around the player test, then the word stored back. */
static void ctl_player_store(uint32_t b, uint16_t v)
{
    cyc(76);
    uint16_t at = ctl_player_word(b);

    cpu.a = v;
    cpu.p = nz16(cpu.p, v);
    cyc(98);
    wr16(b | at, v);
}

/*
 * @routine 08:C60F CarControlsToRecord
 * WHAT IT DOES: Turns a player's buttons ($1AF0; last frame's at $1AF2) into
 * car X's controls in its Super FX record (+$22; none: nothing). With bit 2
 * of the car's +$25 set it only marks the record: +$40 bit 9, and +$42 kept
 * to $D800 unless it has bit 12. Otherwise, unless the record's +$1C has bit
 * 4: SELECT newly pressed runs the button sequence (RecordButtonSequence) and
 * sets bit 0 of the linked object's (+$4F) +$25, and bit 1 with L and R held;
 * in a two-player race ($0D62) the car's +$29 counts up to a random limit
 * $6E-$71 ($1F2B), and is reset to $FFFF by B, Y, A or X once it is past 9;
 * at the limit record +$64 gets bit 3 and the linked object bits 0 and 1.
 * Then, with $0D53 clear and +$42 bit 12 clear: +$40 loses bit 9, +$42 keeps
 * only $D800, and the buttons held set +$42 bits: LEFT 0 (else RIGHT 1), L 2
 * (else R 3), A $2040 (and $20 below speed $32 at +$4A), Y $400 when +$6A is
 * not 0 (else sound $5E and the player's flag $10, at $1A5E or $1A60 by
 * $1A5D, once), B $10; X newly pressed 7 when +$5E is 0 and +$40 bit 8 is
 * clear. Without Y the player's flag $10 is cleared. With +$40 bit 8 set,
 * DOWN takes $600 from +$26 and UP adds it.
 * HOW: 16-bit A for the button and record words, 8-bit for the +$25 flags;
 * the record through long accesses to bank $70, the rest through the data
 * bank. Most tests branch over a long jump to the next one.
 * Entry: data bank $7E; X = the car's object. Exit: X, Y = the object; M
 * 8-bit, X 16-bit; N, Z from X; A, C and V as the last test or sum left
 * them.
 */
void abi_CarControlsToRecord(void)
{
    static const uint16_t clear_timer[4] = {0x8000, 0x4000, 0x0080, 0x0040};
    uint32_t b = (uint32_t)cpu.db << 16, r;
    uint16_t obj = cpu.x, v, pw;
    uint8_t f;
    int i;

    op_sep(P_M);
    op_rep(P_X);
    cyc(46);
    wr16(b | 0x1F18, cpu.x);
    f = (uint8_t)(rd8(cyc_then(30, ctl_field(b, obj, 0x2025))) & 0x04);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | f);
    cpu.p = nz8(cpu.p, f);
    op_rep(P_M);
    cpu.y = obj;
    cpu.p = nz16(cpu.p, obj);
    op_lda16(cyc_then(f ? 112 : 138, ctl_field(b, obj, 0x2022)));
    if (f) {
        if (cpu.a) {
            op_tax16();
            r = 0x700000u + cpu.x;
            ctl_rec_or(r + 0x40, 0x0200);
            cyc(32);
            op_lda16(r + 0x42);
            ctl_bit(0x1000);
            if (!(cpu.a & 0x1000)) {
                op_and16(0xD800);
                cyc(56);
                wr16(r + 0x42, cpu.a);
            }
        }
        goto out;
    }
    if (!cpu.a)
        { cyc(46); goto out; }
    op_tax16();
    r = 0x700000u + cpu.x;
    cyc(38);
    op_lda16(r + 0x1C);
    op_and16(0x0010);
    if (cpu.a)
        { cyc(68); goto out; } else { cyc(44); }
    if (ctl_held(b, SEQ_INPUT, 0x2000)) {
        cyc(46);
        abi_RecordButtonSequence();
        cyc(42);
        if (!ctl_held(b, SEQ_PREV, 0x2000)) {
            uint16_t y = cpu.y;

            op_lda16(cyc_then(60, ctl_field(b, cpu.y, 0x204F)));
            op_tay16();
            op_sep(P_M);
            cyc(36);
            ctl_flags_or(b, 0x01);
            cyc(24);
            op_lda8(b | SEQ_INPUT);
            ctl_bit(0x20);
            if (cpu.a & 0x20) {
                cyc(24);
                op_lda8(b | SEQ_INPUT);
                ctl_bit(0x10);
                if (cpu.a & 0x10)
                    ctl_flags_or(b, 0x02); else { cyc(26); }
            } else { cyc(26); }
            cyc(10);
            op_rep(P_M);
            cpu.y = y;
            cpu.p = nz16(cpu.p, y);
        }
    }
    cyc(50);
    op_lda16(b | 0x0D62);
    op_and16(0x00FF);
    if (cpu.a) {
        cyc(106);
        abi_RandomStepLong();
        op_and16(0x0003);
        op_add16(0x006E);
        cyc(84);
        wr16(b | 0x1F2B, cpu.a);
        op_lda16(cyc_then(30, ctl_field(b, cpu.y, 0x2029)));
        if (!(cpu.a & 0x8000)) {
            cyc(40);
            op_cmp16(rd16(b | 0x1F2B));
            if (cpu.p & P_N) {
                cyc(4);
                cpu.a++;
                cpu.p = nz16(cpu.p, cpu.a);
                wr16(ctl_field(b, cpu.y, 0x2029), cyc_then(56, cpu.a));
                op_cmp16(0x000A);
                if (!(cpu.p & P_N)) {
                    cyc(40);
                    for (i = 0; i < 4; i++)
                        if (ctl_held(b, SEQ_INPUT, clear_timer[i]))
                            break;
                    if (i < 4) {
                        op_lda16_imm(0xFFFF);
                        wr16(ctl_field(b, cpu.y, 0x2029), cyc_then(42, 0xFFFF));
                    }
                } else { cyc(24); }
            cyc(14);
                } else {
                cyc(54);
                op_lda16(r + 0x64);
                ctl_bit(0x0008);
                if (!(cpu.a & 0x0008)) {
                    cpu.a |= 0x0008;
                    cpu.p = nz16(cpu.p, cpu.a);
                    cyc(58);
                    wr16(r + 0x64, cpu.a);
                    ctl_linked_flags(b, 0x01, 0x02);
                }
            }
        } else { cyc(14); }
    } else { cyc(60); }
    cyc(32);
    op_lda16(b | 0x0D53);
    op_and16(0x00FF);
    if (cpu.a)
        { cyc(68); goto out; }
    cyc(76);
    op_lda16(r + 0x40);
    op_and16(0xFDFF);
    cyc(54);
    wr16(r + 0x40, cpu.a);
    cyc(32);
    op_lda16(r + 0x42);
    ctl_bit(0x1000);
    if (cpu.a & 0x1000)
        { cyc(32); goto out; }
    cyc(40);
    op_lda16(r + 0x42);
    op_and16(0xD800);
    cyc(54);
    wr16(r + 0x42, cpu.a);
    if (ctl_held(b, SEQ_INPUT, 0x0200))
        { ctl_rec_or(r + 0x42, 0x0001); cyc(22); /* BRA */ }
    else { cyc(26); if (ctl_held(b, SEQ_INPUT, 0x0100))
        ctl_rec_or(r + 0x42, 0x0002); else { cyc(26); } }
    if (ctl_held(b, SEQ_INPUT, 0x0020))
        { ctl_rec_or(r + 0x42, 0x0004); cyc(22); /* BRA */ }
    else { cyc(26); if (ctl_held(b, SEQ_INPUT, 0x0010))
        ctl_rec_or(r + 0x42, 0x0008); else { cyc(26); } }
    if (ctl_held(b, SEQ_INPUT, 0x0080)) {
        ctl_rec_or(r + 0x42, 0x2040);
        cyc(32);
        op_lda16(r + 0x4A);
        op_sub16(0x0032);
        if (cpu.p & P_N)
            { cyc(48); ctl_rec_or(r + 0x42, 0x0020); } else { cyc(54); }
    } else { cyc(26); }
    if (ctl_held(b, SEQ_INPUT, 0x4000)) {
        cyc(32);
        op_lda16(r + 0x6A);
        if (cpu.a)
            { cyc(16); ctl_rec_or(r + 0x42, 0x0400); goto player_clear; }
        else {
            op_lda16(cyc_then(62, b | ctl_player_word(b)));
            ctl_bit(0x0010);
            if (!(cpu.a & 0x0010)) {
                cyc(88);
                v = cpu.a;
                op_lda16_imm(0x005E);
                abi_SoundPort0QueuePush();
                v |= 0x0010;
                ctl_player_store(b, v);
            } else { cyc(30); }
        }
    cyc(22);
        } else {
        cyc(4);
    player_clear:
        pw = ctl_player_word(b);
        cyc(62);
        op_lda16(b | pw);
        ctl_bit(0x0010);
        if (cpu.a & 0x0010)
            { cyc(24); ctl_player_store(b, (uint16_t)(cpu.a & 0xFFEF)); }
        else cyc(pw == 0x1A60 ? 30 : 8);
    }
    if (ctl_held(b, SEQ_INPUT, 0x0040) && !ctl_held(b, SEQ_PREV, 0x0040)) {
        cyc(32);
        op_lda16(r + 0x5E);
        if (!cpu.a) {
            cyc(48);
            op_lda16(r + 0x40);
            op_and16(0x0100);
            if (!cpu.a)
                { cyc(38); ctl_rec_or(r + 0x42, 0x0080); } else { cyc(44); }
        } else { cyc(22); }
    } else { cyc(26); }
    if (ctl_held(b, SEQ_INPUT, 0x8000))
        ctl_rec_or(r + 0x42, 0x0010); else { cyc(26); }
    cyc(32);
    op_lda16(r + 0x40);
    op_and16(0x0100);
    if (cpu.a) {
        cyc(38);
        if (ctl_held(b, SEQ_INPUT, 0x0400)) {
            cyc(32);
            op_lda16(r + 0x26);
            op_sub16(0x0600);
            cyc(64);
            wr16(r + 0x26, cpu.a);
        } else { cyc(26); }
        if (ctl_held(b, SEQ_INPUT, 0x0800)) {
            cyc(32);
            op_lda16(r + 0x26);
            op_add16(0x0600);
            cyc(70);
            wr16(r + 0x26, cpu.a);
        } else { cyc(26); }
    } else { cyc(44); }
out:
    cyc(102);
    cpu.x = obj;
    cpu.p = nz16(cpu.p, obj);
    op_sep(P_M);
}

/* One player's controls: buttons, player, the car's controls and its
   linked object's handler. */
static void ctl_player(uint16_t held, uint16_t prev, uint8_t player, uint16_t car)
{
    op_rep(P_M);
    cyc(40);
    op_lda16(0x7E0000u | held);
    cyc(24);
    wr16(0x7E0000u | SEQ_INPUT, cpu.a);
    cyc(24);
    op_lda16(0x7E0000u | prev);
    cyc(24);
    wr16(0x7E0000u | SEQ_PREV, cpu.a);
    op_sep(P_M);
    if (player) {
        cyc(16);
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | player);
        cpu.p = nz8(cpu.p, player);
    }
    cyc(46);
    wr8(0x7E0000u | CTL_PLAYER, player);
    cyc(24);
    cpu.x = rd16(0x7E0000u | car);
    cpu.p = nz16(cpu.p, cpu.x);
    (cyc(62), abi_CarControlsToRecord());
    cpu.y = player;
    cpu.p = nz16(cpu.p, cpu.y);
    (cyc(86), abi_ObjectLinkedHandlerByState());
}

/*
 * @routine 08:C5A5 PlayerControlsFrame
 * WHAT IT DOES: The players' controls for a race frame: for player 1, and
 * player 2 when $0D62 is set, puts the buttons held this frame ($0309 or
 * $030D) and last frame ($1F23 or $1F25) at $1AF0 and $1AF2, the player at
 * $1A5D, and runs the car's controls (CarControlsToRecord on the object at
 * $0346 or $034A) and its linked object's handler (ObjectLinkedHandlerByState
 * with Y = the player); then keeps this frame's buttons as last frame's and
 * clears $0309 and $030D.
 * HOW: Data bank $7E, the caller's put back at the end; 8-bit A except for
 * the button words.
 * Entry: any widths. Exit: A = player 2's buttons (read before $030D is
 * cleared); M 8-bit, X 16-bit; X, Y as the last handler left them; N, Z from
 * the caller's data bank.
 */
void abi_PlayerControlsFrame(void)
{
    uint8_t db = cpu.db;

    op_sep(P_M);
    op_rep(P_X);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x7E);
    cpu.db = 0x7E;
    cyc(94);
    ctl_player(0x0309, 0x1F23, 0, 0x0346);
    cyc(24);
    op_lda8(0x7E0D62);
    if ((uint8_t)cpu.a)
        ctl_player(0x030D, 0x1F25, 1, 0x034A); else { cyc(22); }
    op_rep(P_M);
    cyc(24);
    op_lda16(0x7E0309);
    cyc(24);
    wr16(0x7E1F23, cpu.a);
    cyc(24);
    op_lda16(0x7E030D);
    cyc(24);
    wr16(0x7E1F25, cpu.a);
    cyc(24);
    wr16(0x7E0309, 0x0000);
    cyc(24);
    wr16(0x7E030D, 0x0000);
    op_sep(P_M);
    cpu.db = db;
    cpu.p = nz8(cpu.p, db);
    cyc(94);
}

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

    cyc(24);
    n = (uint16_t)(rd16(b | SEQ_COUNT) + 1);
    cyc(14);
    wr16(b | SEQ_COUNT, n);
    cyc(24);
    s = rd16(b | SEQ_STAGE);
    *p = nz16(*p, s);
    if (s != 0)
        { cyc(16); *p = cmp16(*p, s, 0x0001); }
    if (s != 0 && s != 1)
        { cyc(16); *p = cmp16(*p, s, 0x0002); }
    if (s <= 2) {
        cyc(46);
        n = rd16(b | SEQ_COUNT);
        *p = cmp16(*p, n, 0x0005);
        if (!(*p & P_N))
            goto reset;
        cyc(64);
        in = rd16(b | SEQ_INPUT);
        *p = nz16(*p, in);
        if (!(in & press[s])) {
            cyc(68);
            *p |= P_Z;
            return in;
        }
        cyc(96);
        wr16(b | SEQ_COUNT, 0x0000);
        cyc(24);
        s = (uint16_t)(rd16(b | SEQ_STAGE) + 1);
        cyc(14);
        wr16(b | SEQ_STAGE, s);
        *p = nz16(*p, s);
        cyc(22);
        return in;
    }
    *p = cmp16(*p, s, 0x0003);
    if (s == 3) {
        cyc(62);
        n = rd16(b | SEQ_COUNT);
        *p = cmp16(*p, n, 0x0011);
        if (!(*p & P_N))
            goto reset;
        cyc(64);
        in = rd16(b | SEQ_INPUT);
        *p = nz16(*p, in);
        if (!(in & 0x0800)) {
            cyc(72);
            *p |= P_Z;
            return in;
        }
        cyc(70);
        n = rd16(b | SEQ_COUNT);
        *p = cmp16(*p, n, 0x000C);
        if (*p & P_N)
            { cyc(24); goto reset; } else { cyc(8); }
    }
    cyc(126);
    SoundPort0QueuePush(0x48);
    cyc(40);
    wr16(0x700040 + rec, cyc_then(56, (uint16_t)(rd16(0x700040 + rec) | 0x2000)));
    cyc(56);
    wr16(0x70006A + rec, 0x0180);
    cyc(56);
    wr16(0x700400 + rec, 0x1000);
    *p = nz16(*p, 0x1000);
    n = 0x1000;
reset:
    cyc(46);
    wr16(b | SEQ_COUNT, 0x0000);
    cyc(24);
    wr16(b | SEQ_STAGE, 0x0000);
    return n;
}

void abi_RecordButtonSequence(void)
{
    uint8_t p = cpu.p;

    cpu.a = RecordButtonSequence(cpu.db, cpu.x, &p);
    cpu.p = p;
}

/*
 * @routine 08:C9A9 ObjectPoseFromRecord
 * WHAT IT DOES: Takes object X's pose back from its Super FX record:
 * position (+$1E/+$20/+$22 to +$0C/+$0E/+$10) and the angle bytes +$27,
 * +$29, +$25 to +$14, +$16, +$15; copies the record's +$40 to its +$90.
 * Before that clears bits 5 and 6 of the object's +$25, and when its +$36
 * is 1 makes it 2 and sets bit 10 of the record's flags (+$16), else
 * clears that bit.
 * HOW: The record is the object's +$22 (no zero test); X is the record and
 * Y the object while copying.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: X unchanged; Y = X; M 8-bit;
 * A = the high byte of its +$22 and the +$25 byte; N, Z from X; C from the
 * +$36 compare.
 */
uint16_t ObjectPoseFromRecord(uint8_t bank, uint16_t obj, uint8_t *p)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t rec, z;
    uint8_t s, v;

    cyc(52);
    wr8(b | (uint16_t)(0x2025 + obj), cyc_then(46, rd8(b | (uint16_t)(0x2025 + obj)) & 0xDF));
    cyc(30);
    wr8(b | (uint16_t)(0x2025 + obj), cyc_then(46, rd8(b | (uint16_t)(0x2025 + obj)) & 0xBF));
    cyc(60);
    s = rd8(b | (uint16_t)(0x2036 + obj));
    *p = (uint8_t)((*p & ~P_C) | (s >= 0x01 ? P_C : 0));
    if (s == 0x01)
        { cyc(78); wr8(b | (uint16_t)(0x2036 + obj), 0x02); } else { cyc(38); }
    cyc(6);
    rec = record_of(bank, obj);
    if (s == 0x01)
        { cyc(46); wr16(0x700016 + rec, cyc_then(56, (uint16_t)(rd16(0x700016 + rec) | 0x0400))); cyc(22); }   /* BRA */
    else
        { cyc(46); wr16(0x700016 + rec, cyc_then(56, (uint16_t)(rd16(0x700016 + rec) & 0xFBFF))); }
    cyc(108);
    rec = record_of(bank, obj);
    cyc(46);
    wr16(0x700090 + rec, cyc_then(32, rd16(0x700040 + rec)));
    cyc(32);
    wr16(b | (uint16_t)(0x200C + obj), cyc_then(30, rd16(0x70001E + rec)));
    cyc(32);
    wr16(b | (uint16_t)(0x200E + obj), cyc_then(30, rd16(0x700020 + rec)));
    cyc(32);
    z = rd16(0x700022 + rec);
    cyc(30);
    wr16(b | (uint16_t)(0x2010 + obj), z);
    cyc(54);
    wr8(b | (uint16_t)(0x2014 + obj), cyc_then(30, rd8(0x700027 + rec)));
    cyc(32);
    wr8(b | (uint16_t)(0x2016 + obj), cyc_then(30, rd8(0x700029 + rec)));
    cyc(32);
    v = rd8(0x700025 + rec);
    cyc(30);
    wr8(b | (uint16_t)(0x2015 + obj), v);
    *p = nz16(*p, obj);
    cyc(80);
    return (uint16_t)((z & 0xFF00) | v);
}

void abi_ObjectPoseFromRecord(void)
{
    uint8_t p = cpu.p;

    cpu.a = ObjectPoseFromRecord(cpu.db, cpu.x, &p);
    cpu.y = cpu.x;
    cpu.p = (uint8_t)(p | P_M);
}

/*
 * @routine 08:CD08 GsuRecordSet2AHigh
 * WHAT IT DOES: Sets the word at +$2A of object X's record to A's low byte
 * as the high byte (and keeps that word in $1F36), when the object has a
 * record.
 * HOW: AND #$FF / XBA, then a long store.
 * Entry: M 8-bit, X 16-bit; X = object, A low byte = value. Exit: M 8-bit;
 * X unchanged; record: A = the word, N, Z from X; no record: A = 0, Z set.
 */
uint16_t GsuRecordSet2AHigh(uint8_t bank, uint16_t obj, uint8_t value, uint8_t *p)
{
    uint16_t w = (uint16_t)(value << 8), rec;

    cyc(90);
    wr16((uint32_t)bank << 16 | 0x1F36, w);
    cyc(6);
    rec = record_of(bank, obj);
    if (!rec) {
        *p = nz16(*p, 0);
        return 0;
    }
    cyc(132);
    wr16(0x70002A + rec, w);
    *p = nz16(*p, obj);
    return w;
}

void abi_GsuRecordSet2AHigh(void)
{
    uint8_t p = cpu.p;

    cpu.a = GsuRecordSet2AHigh(cpu.db, cpu.x, (uint8_t)cpu.a, &p);
    cpu.p = (uint8_t)(p | P_M);
}

void abi_ObjectStubZero(void);
void abi_ObjectRebucket(void);

/* The handler of object X: +$1A = h, +$1C = $08 (16-bit then 8-bit
   immediate stores). */
static void set_handler08(uint16_t h)
{
    uint32_t b = (uint32_t)cpu.db << 16;

    cpu.p &= (uint8_t)~P_M;
    cpu.a = h;
    cpu.p = nz16(cpu.p, h);
    cyc(76);
    wr16(b | (uint16_t)(0x201A + cpu.x), h);
    cpu.p |= P_M;
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x08);
    cpu.p = nz8(cpu.p, 0x08);
    cyc(68);
    wr8(b | (uint16_t)(0x201C + cpu.x), 0x08);
}

/*
 * @routine 08:CDA4 ObjectRecordMarkStep
 * WHAT IT DOES: A handler that only rebuckets object X and marks its
 * record (GsuRecordFlag4) each frame.
 * HOW: ObjectRebucket, GsuRecordFlag4.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as GsuRecordFlag4.
 */
void abi_ObjectRecordMarkStep(void)
{
    (cyc(62), abi_ObjectRebucket());
    (cyc(62), abi_GsuRecordFlag4());
}

/*
 * @routine 08:CD84 ObjectRecordMarkSetup
 * WHAT IT DOES: Sets up an object that keeps a Super FX record and only
 * marks it: handler $08:CDA4 (ObjectRecordMarkStep), ObjectStubZero, a
 * record (GsuRecordNew422), then +$09 bit 0 set. No course in the scripts
 * places it.
 * HOW: Stores and calls in turn; a failed record is not checked.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; A low byte = the
 * new +$09, N, Z from it.
 */
void abi_ObjectRecordMarkSetup(void)
{
    uint32_t b;
    uint8_t v;

    set_handler08(0xCDA4);
    (cyc(62), abi_ObjectStubZero());
    abi_GsuRecordNew422();
    b = (uint32_t)cpu.db << 16 | (uint16_t)(0x2009 + cpu.x);
    cyc(30);
    v = (uint8_t)(rd8(b) | 0x01);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, v);
    cyc(46);
    wr8(b, v);
}

/*
 * @routine 08:CE79 ObjectSyncFromRecordFall
 * WHAT IT DOES: As ObjectSyncFromRecord for an object whose record is
 * known to be there, with its Y speed (+$30) added to the height taken
 * back from the record, and +$24 bit 1 neither cleared nor set.
 * HOW: Rebucket, GsuRecordFlag4; the object in Y and the record in X while
 * copying; $1F3C = the record's +$18 after the heading and +$12; ends in
 * ObjectSyncFromRecord's tail at $08:CE60. The record address is not
 * tested.
 * Entry: M 8-bit, X 16-bit, D = $0000; X = object. Exit: X = the object; M
 * 8-bit; carry clear; V from the height's add; Y = $1F3C; A low byte = 1
 * when it is not zero (N, Z from that), else A = the record's +$22 and Z
 * set.
 */
uint16_t ObjectSyncFromRecordFall(uint8_t bank, uint16_t obj, uint16_t *y, uint8_t *p)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t rec, v;
    int has;

    cyc(62);
    ObjectRebucket(obj);
    cyc(62);
    GsuRecordFlag4(bank, obj, &has);
    cyc(116);
    rec = record_of(bank, obj);
    cyc(44);
    v = rd16(b | (uint16_t)(0x2015 + obj));
    cyc(76);
    wr16(0x700024 + rec, (uint16_t)((v & 0x00FF) << 8));
    cyc(30);
    v = rd16(b | (uint16_t)(0x205A + obj));
    cyc(56);
    wr16(0x700012 + rec, v & 0x007F);
    cyc(32);
    v = rd16(0x700018 + rec);
    cyc(24);
    wr16(b | 0x1F3C, v);
    cyc(32);
    v = rd16(0x70001E + rec);
    cyc(30);
    wr16(b | (uint16_t)(0x200C + obj), v);
    cyc(32);
    v = rd16(0x700020 + rec);
    *p &= (uint8_t)~P_C;
    cyc(44);
    v = op_adc16(v, rd16(b | (uint16_t)(0x2030 + obj)), p);
    cyc(30);
    wr16(b | (uint16_t)(0x200E + obj), v);
    cyc(32);
    v = rd16(0x700022 + rec);
    cyc(30);
    wr16(b | (uint16_t)(0x2010 + obj), v);
    *p &= (uint8_t)~P_C;
    cyc(60);
    return sync_tail(bank, v, y, p);
}

void abi_ObjectSyncFromRecordFall(void)
{
    uint8_t p = cpu.p;

    cpu.a = ObjectSyncFromRecordFall(cpu.db, cpu.x, &cpu.y, &p);
    cpu.p = (uint8_t)(p | P_M);
}

/*
 * @routine 08:CDAD ObjectSyncFallStep
 * WHAT IT DOES: A handler: ObjectSyncFromRecordFall.
 * HOW: JSL to it.
 * Entry: as ObjectSyncFromRecordFall. Exit: as ObjectSyncFromRecordFall.
 */
void abi_ObjectSyncFallStep(void)
{
    (cyc(62), abi_ObjectSyncFromRecordFall());
}

/*
 * @routine 08:CDDE ObjectSpinStep
 * WHAT IT DOES: The spinning object's handler: its heading +$15 turns by
 * one a frame and goes to its record's +$24 (low byte), then the object
 * follows its record (ObjectSyncFromRecord).
 * HOW: An 8-bit add to +$15 (CLC, ADC, which sets V); with a record, a
 * 16-bit read of +$15 masked to its low byte stored at the record's +$24.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectSyncFromRecord, V
 * from the add.
 */
void abi_ObjectSpinStep(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint16_t obj = cpu.x, rec, v;
    uint8_t h;

    cyc(30);
    h = rd8(b | (uint16_t)(0x2015 + obj));
    cpu.p &= (uint8_t)~P_C;
    h = op_adc8(h, 0x01, &cpu.p);
    cyc(60);
    wr8(b | (uint16_t)(0x2015 + obj), h);
    cyc(20);
    rec = record_of(cpu.db, obj);
    if (rec) {
        cyc(60);
        v = rd16(b | (uint16_t)(0x2015 + obj));
        cyc(56);
        wr16(0x700024 + rec, v & 0x00FF);
    } else { cyc(8); }
    cyc(98);
    abi_ObjectSyncFromRecord();
    cyc(124);
}

/*
 * @routine 08:CDD0 ObjectSpinRecord
 * WHAT IT DOES: Gives the spinning object its Super FX record
 * (GsuRecordNew424) in mode 2 with $20 in the high byte of +$2A, then its
 * first step (ObjectSpinStep). Also its +$1D pointer.
 * HOW: GsuRecordNew424, GsuRecordMode2, LDA #$20, GsuRecordSet2AHigh;
 * falls into ObjectSpinStep.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectSyncFromRecord.
 */
void abi_ObjectSpinRecord(void)
{
    abi_GsuRecordNew424();
    abi_GsuRecordMode2();
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x20);
    cpu.p = nz8(cpu.p, 0x20);
    (cyc(20), abi_GsuRecordSet2AHigh());
    (cyc(102), abi_ObjectSpinStep());
}

/*
 * @routine 08:CDB2 ObjectSpinSetup
 * WHAT IT DOES: Sets up a spinning object: +$1D = $08:CDD0
 * (ObjectSpinRecord), handler $08:CDDE (ObjectSpinStep), then its record
 * (ObjectSpinRecord). No course in the scripts places it.
 * HOW: Stores, then falls into ObjectSpinRecord.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectSyncFromRecord.
 */
void abi_ObjectSpinSetup(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;

    cpu.p &= (uint8_t)~P_M;
    cpu.a = 0xCDD0;
    cyc(76);
    wr16(b | (uint16_t)(0x201D + cpu.x), 0xCDD0);
    cpu.p |= P_M;
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x08);
    cyc(68);
    wr8(b | (uint16_t)(0x201F + cpu.x), 0x08);
    set_handler08(0xCDDE);
    abi_ObjectSpinRecord();
}

/*
 * @routine 08:CBFF GsuRecordNew424Zero
 * WHAT IT DOES: As GsuRecordNew424Alt, with +$0A = 0. Nothing in the game
 * calls it.
 * HOW: Runs Super FX program $04:8C0A; then the shared tail at $08:CC7C.
 * Entry: as GsuRecordNew422. Exit: as GsuRecordNew422.
 */
void abi_GsuRecordNew424Zero(void)
{
    abi_record_new(0x8C0A, 0x0000, 98);
}

/*
 * @routine 08:CC31 GsuRecordNew424B
 * WHAT IT DOES: As GsuRecordNew424, from Super FX program $04:8C3B, with
 * +$0A = $9185. Nothing in the game calls it.
 * HOW: Runs Super FX program $04:8C3B; then the shared tail at $08:CC7C.
 * Entry: as GsuRecordNew422. Exit: as GsuRecordNew422.
 */
void abi_GsuRecordNew424B(void)
{
    abi_record_new(0x8C3B, 0x9185, 98);
}

/*
 * @routine 08:B467 GsuRecordListFlag80
 * WHAT IT DOES: Sets bit 7 of the flags (+$16) of every record in object
 * X's record list (its record's +$3E, linked through +$34). Nothing in
 * the game calls it; a direct call is its only way in.
 * HOW: 16-bit walk with X; the object kept on the stack (PHX, PLX). Called
 * by JSR.
 * Entry: M 8-bit, X 16-bit; X = object (with a record). Exit: X unchanged,
 * N and Z from it; A = 0; M 8-bit.
 */
void abi_GsuRecordListFlag80(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint16_t x0 = cpu.x, v;
    int first;

    cpu.p &= (uint8_t)~P_M;
    cyc(82);
    op_lda16(b | (uint16_t)(0x2022 + cpu.x));
    op_tax16();
    cyc(16);
    op_lda16(0x70003E + cpu.x);
    for (first = 1;; first = 0) {
        cyc(first ? 32 : 54);           /* later rounds: after the BRA */
        op_tax16();
        if (!cpu.x)
            { cyc(74); break; }
        v = (uint16_t)(rd16(0x700016 + cpu.x) | 0x0080);
        cpu.a = v;
        cpu.p = nz16(cpu.p, v);
        cyc(56);
        wr16(0x700016 + cpu.x, v);
        cyc(32);
        op_lda16(0x700034 + cpu.x);
    }
    cpu.x = x0;
    cpu.p = (uint8_t)(nz16(cpu.p, cpu.x) | P_M);
}

/*
 * @routine 08:C096 ReturnLongC096
 * WHAT IT DOES: A lone RTL nothing reaches, between two routines; a direct
 * call is its only way in.
 * HOW: RTL.
 * Entry: any. Exit: nothing changed.
 */
void abi_ReturnLongC096(void)
{
    cyc(44);
}

/* LDA addr, ORA #$20, STA addr (8-bit). */
static void or20(uint32_t addr)
{
    uint8_t v;

    cyc(60);
    op_lda8(addr);
    v = (uint8_t)(cpu.a | 0x20);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, v);
    cyc(46);
    wr8(addr, v);
}

/* LDA addr at the width M has: ObjectChildFollow returns with M 8-bit, so
   after its first call ObjectRecordChildrenMark's 16-bit walk goes on with
   8-bit loads (A's high byte kept) and 16-bit transfers. */
static void lda_m(uint32_t addr, int back)
{
    cyc(back ? 36 : 14);                /* back: after the BRA to the top */
    if (cpu.p & P_M)
        op_lda8(addr);
    else
        op_lda16(addr);
}

/*
 * @routine 08:C097 ObjectRecordChildrenMark
 * WHAT IT DOES: Keeps A in $1F1A and object X in $1F18; then, when bit 3
 * of the object's +$24 is set, sets bit 5 of +$24 of each of its parents
 * (+$06 up to 0). Otherwise $1F58 and $1F59 take its record's +$27 and
 * +$29 bytes, and for each parent of the object (+$06 up to 0), with X
 * stepping through the record's list (+$3E, then each entry's +$34): when
 * the record's flags (+$16) have $0400, the parent's +$24 gets bit 5 and
 * the list entry's flags $0480; otherwise ObjectChildFollow runs on the
 * list entry. Nothing in the game calls it; a direct call is its only way
 * in.
 * HOW: The object kept on the stack (PHY, PLX; PHX, PLX on the bit 3
 * path); 16-bit walks, the +$24 stores 8-bit; JSR to ObjectChildFollow,
 * which returns with M 8-bit, so the rest of that walk loads 8-bit into a
 * 16-bit X and Y (A's high byte kept).
 * Entry: M 8-bit, X 16-bit; X = object (with a record unless bit 3 is
 * set). Exit: M 8-bit; X = the object, N and Z from it; Y = 0 (the end of
 * the parents); A as the last step left it.
 */
void abi_ObjectRecordChildrenMark(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint16_t x0 = cpu.x, v;
    uint8_t f;
    int back;

    op_sep(P_M);
    cyc(46);
    wr8(b | 0x1F1A, (uint8_t)cpu.a);
    cyc(24);
    wr16(b | 0x1F18, cpu.x);
    cyc(30);
    op_lda8(b | (uint16_t)(0x2024 + cpu.x));
    f = (uint8_t)(cpu.a & 0x08);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | f);
    cpu.p = nz8(cpu.p, f);
    if (f) {
        cyc(94);
        for (;;) {
            cyc(52);
            cpu.y = rd16(b | (uint16_t)(0x2006 + cpu.x));
            cpu.p = nz16(cpu.p, cpu.y);
            if (!cpu.y)
                { cyc(102); break; }
            cpu.x = cpu.y;
            cpu.p = nz16(cpu.p, cpu.x);
            or20(b | (uint16_t)(0x2024 + cpu.x));
        }
        cpu.x = x0;
        cpu.p = nz16(cpu.p, cpu.x);
        return;
    }
    cpu.y = cpu.x;
    cpu.p = nz16(cpu.p, cpu.y);
    cpu.p &= (uint8_t)~P_M;
    cyc(104);
    op_lda16(b | (uint16_t)(0x2022 + cpu.x));
    op_tax16();
    cpu.p |= P_M;
    cyc(38);
    op_lda8(0x700027 + cpu.x);
    cyc(24);
    wr8(b | 0x1F58, (uint8_t)cpu.a);
    cyc(32);
    op_lda8(0x700029 + cpu.x);
    cyc(24);
    wr8(b | 0x1F59, (uint8_t)cpu.a);
    cpu.p &= (uint8_t)~P_M;
    cyc(84);
    op_lda16(0x700016 + cpu.x);
    op_and16(0x0400);
    f = cpu.a != 0;
    cyc(f ? 76 : 70);                   /* BNE taken or not */
    op_lda16(0x70003E + cpu.x);
    op_tax16();
    for (back = 0;; back = 1) {
        lda_m(b | (uint16_t)(0x2006 + cpu.y), back);
        op_tay16();
        if (!cpu.y)
            { cyc(138); break; } else { cyc(22); }
        if (f) {
            cpu.p |= P_M;
            or20(b | (uint16_t)(0x2024 + cpu.y));
            cpu.p &= (uint8_t)~P_M;
            cyc(54);
            v = (uint16_t)(rd16(0x700016 + cpu.x) | 0x0480);
            cpu.a = v;
            cpu.p = nz16(cpu.p, v);
            cyc(56);
            wr16(0x700016 + cpu.x, v);
        } else {
            cyc(54);
            abi_ObjectChildFollow();
        }
        cyc(32);
        if (cpu.p & P_M)
            op_lda8(0x700034 + cpu.x);
        else
            op_lda16(0x700034 + cpu.x);
        op_tax16();
    }
    cpu.p |= P_M;
    cpu.x = x0;
    cpu.p = nz16(cpu.p, cpu.x);
}
