/*
 * object.c: the object table in work RAM bank $7E. Objects are $62-byte
 * records from $7E:2000; free records form a list linked through their first
 * word. Fields used here: +$0C/+$0E/+$10 position (16-bit each), +$14-$16
 * three bytes copied as a group, +$1A a word, +$2E/+$30/+$32 velocity,
 * +$4F the link to a related object.
 */
#include "machine.h"

/* A field of the object at offset `obj` in the data bank. */
static uint32_t field(uint8_t bank, uint16_t obj, uint16_t off)
{
    return (uint32_t)bank << 16 | (uint16_t)(0x2000 + off + obj);
}

/*
 * @routine 03:89A8 ObjectLinkedSet1A
 * WHAT IT DOES: Stores A in field +$1A of the object that X's object links to.
 * HOW: Reads the link at +$4F, then stores there (bank $7E, long addressing).
 * Entry: M 16-bit, X 16-bit; A = value, X = object. Exit: A unchanged; X =
 * the linked object; N, Z from A.
 */
uint16_t ObjectLinkedSet1A(uint16_t obj, uint16_t value)
{
    uint16_t linked = rd16(0x7E204F + obj);

    wr16(0x7E201A + linked, value);
    return linked;
}

void abi_ObjectLinkedSet1A(void)
{
    cpu.x = ObjectLinkedSet1A(cpu.x, cpu.a);
    cpu.p = nz16(cpu.p, cpu.a);
}

/* Copy n bytes of fields from one object to another, in the data bank. */
static uint8_t copy_bytes(uint8_t bank, uint16_t from, uint16_t to, uint16_t off, unsigned n)
{
    uint8_t v = 0;
    unsigned i;

    for (i = 0; i < n; i++) {
        v = rd8(field(bank, from, (uint16_t)(off + i)));
        wr8(field(bank, to, (uint16_t)(off + i)), v);
    }
    return v;
}

static uint16_t copy_words(uint8_t bank, uint16_t from, uint16_t to)
{
    uint16_t v = 0;
    unsigned i;

    for (i = 0; i < 3; i++) {
        v = rd16(field(bank, from, (uint16_t)(0x0C + 2 * i)));
        wr16(field(bank, to, (uint16_t)(0x0C + 2 * i)), v);
    }
    return v;
}

/*
 * @routine 03:D214 ObjectCopy14To
 * WHAT IT DOES: Copies object X's three bytes at +$14-$16 to object Y.
 * HOW: Three byte copies, in the data bank.
 * Entry: M 8-bit, X 16-bit; DB = the object table's bank. Exit: A low byte =
 * the last byte; N, Z from it.
 */
void ObjectCopy14To(uint8_t bank, uint16_t from, uint16_t to)
{
    copy_bytes(bank, from, to, 0x14, 3);
}

void abi_ObjectCopy14To(void)
{
    uint8_t v = copy_bytes(cpu.db, cpu.x, cpu.y, 0x14, 3);

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, v);
}

/*
 * @routine 03:D227 ObjectCopy14From
 * WHAT IT DOES: Copies object Y's three bytes at +$14-$16 to object X.
 * HOW: As ObjectCopy14To, the other way.
 * Entry: as ObjectCopy14To. Exit: as ObjectCopy14To.
 */
void ObjectCopy14From(uint8_t bank, uint16_t to, uint16_t from)
{
    copy_bytes(bank, from, to, 0x14, 3);
}

void abi_ObjectCopy14From(void)
{
    uint8_t v = copy_bytes(cpu.db, cpu.y, cpu.x, 0x14, 3);

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, v);
}

/*
 * @routine 03:D1E6 ObjectCopyPositionTo
 * WHAT IT DOES: Copies object X's position to object Y.
 * HOW: Three 16-bit copies (+$0C, +$0E, +$10), in the data bank.
 * Entry: M 8-bit, X 16-bit; DB = the object table's bank. Exit: A = the
 * last word; N, Z from it (16-bit); M 8-bit.
 */
void ObjectCopyPositionTo(uint8_t bank, uint16_t from, uint16_t to)
{
    copy_words(bank, from, to);
}

void abi_ObjectCopyPositionTo(void)
{
    cpu.a = copy_words(cpu.db, cpu.x, cpu.y);
    cpu.p = (uint8_t)(nz16(cpu.p, cpu.a) | P_M);
}

/*
 * @routine 03:D1FD ObjectCopyPositionFrom
 * WHAT IT DOES: Copies object Y's position to object X.
 * HOW: As ObjectCopyPositionTo, the other way.
 * Entry: as ObjectCopyPositionTo. Exit: as ObjectCopyPositionTo.
 */
void ObjectCopyPositionFrom(uint8_t bank, uint16_t to, uint16_t from)
{
    copy_words(bank, from, to);
}

void abi_ObjectCopyPositionFrom(void)
{
    cpu.a = copy_words(cpu.db, cpu.y, cpu.x);
    cpu.p = (uint8_t)(nz16(cpu.p, cpu.a) | P_M);
}

/*
 * @routine 03:C06C ObjectApplyVelocity
 * WHAT IT DOES: Moves object X by its velocity.
 * HOW: Adds +$2E, +$30, +$32 to +$0C, +$0E, +$10 (16-bit), in the data bank.
 * Entry: M 8-bit, X 16-bit; DB = the object table's bank. Exit: A = the new
 * +$10; flags from the last add; M 8-bit.
 */
static uint16_t apply_velocity(uint8_t bank, uint16_t obj, uint8_t *p)
{
    uint16_t r = 0;
    unsigned i;

    for (i = 0; i < 3; i++) {
        uint16_t a = rd16(field(bank, obj, (uint16_t)(0x0C + 2 * i)));
        uint16_t b = rd16(field(bank, obj, (uint16_t)(0x2E + 2 * i)));
        uint32_t s = (uint32_t)a + b;
        r = (uint16_t)s;
        wr16(field(bank, obj, (uint16_t)(0x0C + 2 * i)), r);
        *p = (uint8_t)((nz16(*p, r) & ~(P_C | P_V)) | (s > 0xFFFF ? P_C : 0)
                       | ((~(a ^ b) & (a ^ r) & 0x8000) ? P_V : 0));
    }
    return r;
}

void ObjectApplyVelocity(uint8_t bank, uint16_t obj)
{
    uint8_t p = 0;

    apply_velocity(bank, obj, &p);
}

void abi_ObjectApplyVelocity(void)
{
    uint8_t p = cpu.p;

    cpu.a = apply_velocity(cpu.db, cpu.x, &p);
    cpu.p = (uint8_t)(p | P_M);
}

/*
 * @routine 03:D3DE ObjectPoolInit
 * WHAT IT DOES: Empties the object table: links all $136 records into the
 * free list and clears the counters at $0329-$032E.
 * HOW: Starting at offset 1, each record's first word gets the next record's
 * offset ($62 on); the last gets 0. Data bank $7E inside.
 * Entry: any widths. Exit: A = 1 + $136 * $62; X = the last record; Y = 0;
 * P unchanged.
 */
void ObjectPoolInit(void)
{
    uint16_t x = 1;
    unsigned i;

    wr16(0x7E0329, 0);
    wr16(0x7E032B, 0);
    wr16(0x7E032D, 1);
    for (i = 0; i < 0x136; i++) {
        uint16_t next = (uint16_t)(x + 0x62);
        wr16(0x7E2000 + x, next);
        if (i + 1 < 0x136)
            x = next;
    }
    wr16(0x7E2000 + x, 0);
}

void abi_ObjectPoolInit(void)
{
    ObjectPoolInit();
    cpu.a = (uint16_t)(1 + 0x136 * 0x62);
    cpu.x = (uint16_t)(1 + 0x135 * 0x62);
    cpu.y = 0;
    if (cpu.p & P_X) {
        cpu.x &= 0x00FF;
    }
}

/*
 * @routine 7F:12AC ObjectLinkFlag4000Step
 * WHAT IT DOES: When the object two links on from the one at $0346 has bit
 * 14 set in its first word, adds 8 to $06CD.
 * HOW: Follows the link at +$4F twice (bank $7E), reads the record's first
 * word in bank 0.
 * Entry: any widths. Exit: A = the masked word, or the new $06CD; X, P
 * unchanged.
 */
uint16_t ObjectLinkFlag4000Step(void)
{
    uint16_t x = rd16(0x000346), a;

    x = rd16(0x7E204F + x);
    x = rd16(0x7E204F + x);
    a = rd16(0x000000 + x) & 0x4000;
    if (!a)
        return a;
    a = (uint16_t)(rd16(0x0006CD) + 8);
    wr16(0x0006CD, a);
    return a;
}

void abi_ObjectLinkFlag4000Step(void)
{
    cpu.a = ObjectLinkFlag4000Step();
}

/*
 * @routine 08:B830 ObjectFlag40IfMatch
 * WHAT IT DOES: When A equals the word at $70:2476, sets bit 6 of field
 * +$25 of the object at $0346.
 * HOW: 16-bit compare, then an 8-bit OR through the data bank.
 * Entry: M 16-bit, X 16-bit; A = value. Exit: no match: A unchanged; M
 * 16-bit; N, Z, C from the compare. Match: A low byte = the new field; X =
 * the object; M 8-bit; N, Z from it; C set.
 */
int ObjectFlag40IfMatch(uint8_t bank, uint16_t a, uint16_t *x, uint8_t *v, uint8_t *p)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t m = rd16(0x702476);

    *p = (uint8_t)((nz16(*p, (uint16_t)(a - m)) & ~P_C) | (a >= m ? P_C : 0));
    if (a != m)
        return 0;
    *x = rd16(b | 0x0346);
    *v = (uint8_t)(rd8(b | (uint16_t)(0x2025 + *x)) | 0x40);
    wr8(b | (uint16_t)(0x2025 + *x), *v);
    *p = nz8(*p, *v);
    return 1;
}

void abi_ObjectFlag40IfMatch(void)
{
    uint8_t v = 0, p = cpu.p;

    if (ObjectFlag40IfMatch(cpu.db, cpu.a, &cpu.x, &v, &p)) {
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
        p |= P_M;
    }
    cpu.p = p;
}

/* An 8-bit add as ADC with carry clear: the sum, with C and V. */
static uint8_t add8(uint8_t a, uint8_t b, uint8_t *p)
{
    unsigned s = (unsigned)a + b;

    *p = (uint8_t)((*p & ~(P_C | P_V)) | (s > 0xFF ? P_C : 0) | ((~(a ^ b) & (a ^ (uint8_t)s) & 0x80) ? P_V : 0));
    return (uint8_t)s;
}

/*
 * @routine 08:E896 ObjectTableE8AF
 * WHAT IT DOES: Returns the byte of the table at $08:E8AF for (A + object
 * X's +$15 + $1F4C) / 8.
 * HOW: 8-bit adds, three right shifts.
 * Entry: M 8-bit, X 16-bit; X = object, A = value. Exit: A low byte = the
 * table byte; X, Y unchanged; N, Z from X; C from the last shift; V from
 * the second add.
 */
uint8_t ObjectTableE8AF(uint8_t bank, uint8_t a, uint16_t obj, uint8_t *p)
{
    uint32_t b = (uint32_t)bank << 16;

    a = add8(a, rd8(b | (uint16_t)(0x2015 + obj)), p);
    a = add8(a, rd8(b | 0x1F4C), p);
    *p = (uint8_t)((*p & ~P_C) | (a & 0x04 ? P_C : 0));
    return rd8(0x08E8AF + (a >> 3));
}

void abi_ObjectTableE8AF(void)
{
    uint8_t p = cpu.p, v = ObjectTableE8AF(cpu.db, (uint8_t)cpu.a, cpu.x, &p);

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz16(p, cpu.x);
}

/*
 * @routine 08:C03C ObjectTableC056
 * WHAT IT DOES: Sets $1F5A to the byte of the table at $08:C056 for (object
 * Y's +$15 - $1F5A) / 4.
 * HOW: An 8-bit subtract, two right shifts.
 * Entry: M 8-bit, X 16-bit; Y = object. Exit: A low byte = the new $1F5A;
 * X, Y unchanged; N, Z from Y; C from the last shift; V from the subtract.
 */
uint8_t ObjectTableC056(uint8_t bank, uint16_t obj, uint8_t *p)
{
    uint32_t b = (uint32_t)bank << 16;
    uint8_t f = rd8(b | (uint16_t)(0x2015 + obj)), s = rd8(b | 0x1F5A), d = (uint8_t)(f - s), v;

    *p = (uint8_t)((*p & ~(P_C | P_V)) | (d & 0x02 ? P_C : 0) | (((f ^ s) & (f ^ d) & 0x80) ? P_V : 0));
    v = rd8(0x08C056 + (d >> 2));
    wr8(b | 0x1F5A, v);
    return v;
}

void abi_ObjectTableC056(void)
{
    uint8_t p = cpu.p, v = ObjectTableC056(cpu.db, cpu.y, &p);

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz16(p, cpu.y);
}

/*
 * @routine 08:B75B ObjectListAppend
 * WHAT IT DOES: Appends object X to the zero-ended list of up to 32 words at
 * $7E:FFBD, whose length in bytes is at $7E:FFFD; a full list is left alone.
 * HOW: Stores X at the end, steps the length by 2 and stores the 0 that
 * ends the list.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: A = 0, or $40 when full (M
 * 8-bit); X, Y unchanged; N, Z from X; C from the full-length compare.
 */
int ObjectListAppend(uint16_t obj)
{
    uint16_t n = rd16(0x7EFFFD);

    if (n == 0x0040)
        return 0;
    wr16(0x7EFFBD + n, obj);
    n = (uint16_t)(n + 2);
    wr16(0x7EFFFD, n);
    wr16(0x7EFFBD + n, 0x0000);
    return 1;
}

void abi_ObjectListAppend(void)
{
    uint16_t n = rd16(0x7EFFFD);

    cpu.a = ObjectListAppend(cpu.x) ? 0x0000 : 0x0040;
    cpu.p = (uint8_t)((nz16(cpu.p, cpu.x) & ~P_C) | (n >= 0x0040 ? P_C : 0) | P_M);
}

/*
 * @routine 08:8364 ObjectBucketInsert
 * WHAT IT DOES: Puts object X at the head of the list for its bucket: the
 * bucket number is the object's +$40, the list heads are words at $7F:0000
 * + the bucket, and +$3C/+$3E link to the next and previous objects.
 * HOW: The old head becomes the object's next (+$3C, its previous cleared)
 * and gets the object as its previous; the object becomes the head. Direct
 * page $40 holds the object while the old head is updated. Data bank $7E
 * inside.
 * Entry: M 16-bit, X 16-bit, D = $0000; X = object. Exit: A = the object;
 * X, Y unchanged; N, Z from the caller's data bank.
 */
void ObjectBucketInsert(uint16_t obj)
{
    uint16_t bucket = rd16(0x7E2040 + obj), head;

    wr16(0x7E203E + obj, 0x0000);
    head = rd16(0x7F0000 + bucket);
    wr16(0x7E203C + obj, head);
    if (head) {
        wr16(0x000040, obj);
        wr16(0x7E203E + head, rd16(0x000040));
    }
    wr16(0x7F0000 + bucket, obj);
}

void abi_ObjectBucketInsert(void)
{
    ObjectBucketInsert(cpu.x);
    cpu.a = cpu.x;
    cpu.p = nz8(cpu.p, cpu.db);
}

/* An arithmetic shift right by one, as CMP #$8000 / ROR A does it. */
static uint16_t asr1c(uint16_t v, uint8_t *p)
{
    *p = (uint8_t)((*p & ~P_C) | (v & 1 ? P_C : 0));
    return (uint16_t)((v >> 1) | (v & 0x8000));
}

/*
 * @routine 08:D5AB ObjectVelocityFromRecord
 * WHAT IT DOES: Sets object X's X and Z velocity (+$2E, +$32) to an eighth
 * of the values at +$2E and +$32 of the Super FX record of the object named
 * by its +$27.
 * HOW: Each value is shifted right three times keeping its sign.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: A = the Z velocity (M 8-bit);
 * X, Y = the object; N, Z from X; C from the last shift.
 */
uint16_t ObjectVelocityFromRecord(uint8_t bank, uint16_t obj, uint8_t *p)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t rec = rd16(b | (uint16_t)(0x2022 + rd16(b | (uint16_t)(0x2027 + obj)))), v;

    v = asr1c(asr1c(asr1c(rd16(0x70002E + rec), p), p), p);
    wr16(b | (uint16_t)(0x202E + obj), v);
    v = asr1c(asr1c(asr1c(rd16(0x700032 + rec), p), p), p);
    wr16(b | (uint16_t)(0x2032 + obj), v);
    *p = nz16(*p, obj);
    return v;
}

void abi_ObjectVelocityFromRecord(void)
{
    uint8_t p = cpu.p;

    cpu.a = ObjectVelocityFromRecord(cpu.db, cpu.x, &p);
    cpu.y = cpu.x;
    cpu.p = (uint8_t)(p | P_M);
}

/*
 * @routine 08:8392 ObjectBucketRemove
 * WHAT IT DOES: Takes object X out of its bucket's list (see
 * ObjectBucketInsert): its previous object, or the bucket's head when it has
 * none, is linked to its next, and its next gets its previous.
 * HOW: Data bank $7E inside; the head is $7F:0000 + the object's +$40.
 * Entry: X 16-bit; X = object. Exit: A = the previous object (or the next,
 * when there is no next or no previous); X, Y unchanged; M 16-bit; N, Z
 * from the caller's data bank.
 */
uint16_t ObjectBucketRemove(uint16_t obj)
{
    uint16_t prev = rd16(0x7E203E + obj), next;

    if (prev) {
        next = rd16(0x7E203C + obj);
        wr16(0x7E203C + prev, next);
        if (!next)
            return next;
        wr16(0x7E203E + next, prev);
        return prev;
    }
    next = rd16(0x7E203C + obj);
    wr16(0x7F0000 + rd16(0x7E2040 + obj), next);
    if (next)
        wr16(0x7E203E + next, 0x0000);
    return next;
}

void abi_ObjectBucketRemove(void)
{
    cpu.a = ObjectBucketRemove(cpu.x);
    cpu.p = (uint8_t)(nz8(cpu.p, cpu.db) & ~P_M);
}

/*
 * @routine 08:B48E ObjectSet5DByKind
 * WHAT IT DOES: Sets object X's +$5D to $9163, $9335 or $9507 for A = 1, 2 or
 * 3, and to $8F91 otherwise.
 * HOW: Compares in turn; one 16-bit store through the data bank.
 * Entry: M 8-bit, X 16-bit; X = object, A = kind. Exit: A = the value (M
 * 8-bit); N set, Z clear; C from the last compare.
 */
uint16_t ObjectSet5DByKind(uint8_t bank, uint16_t obj, uint8_t kind, uint8_t *p)
{
    uint16_t v = kind == 1 ? 0x9163 : kind == 2 ? 0x9335 : kind == 3 ? 0x9507 : 0x8F91;
    uint8_t last = kind == 1 ? 1 : kind == 2 ? 2 : 3;

    *p = (uint8_t)((*p & ~(P_C | P_Z)) | P_N | (kind >= last ? P_C : 0));
    wr16((uint32_t)bank << 16 | (uint16_t)(0x205D + obj), v);
    return v;
}

void abi_ObjectSet5DByKind(void)
{
    uint8_t p = cpu.p;

    cpu.a = ObjectSet5DByKind(cpu.db, cpu.x, (uint8_t)cpu.a, &p);
    cpu.p = (uint8_t)(p | P_M);
}

/*
 * @routine 08:B8F9 ObjectPlaceFromTable
 * WHAT IT DOES: Sets up the object in entry A (from 1) of the list at
 * $7E:FFAE: its +$27 from $1F2D, its +$49 and +$4A from the bytes at $1F2B
 * and $1F4C, and, unless bit 0 of $1AEE is set, its position (+$0C, +$0E,
 * +$10) and +$15 byte from entry X (from 1) of the 8-byte table at
 * $7E:FF66.
 * HOW: Data bank $7E inside; word copies, then byte copies.
 * Entry: M 16-bit, X 16-bit; A low byte = list entry, X low byte = table
 * entry. Exit: A low byte = the byte from $1F4C; X = the table offset; Y =
 * the object; P unchanged.
 */
uint16_t ObjectPlaceFromTable(uint8_t list, uint8_t entry, uint16_t *x, uint16_t *y)
{
    uint16_t obj = rd16(0x7EFFAE + (uint16_t)((list - 1) << 1)), t = (uint16_t)((uint16_t)(entry - 1) << 3), a, w = 0;
    unsigned i;

    wr16(0x7E2027 + obj, rd16(0x7E1F2D));
    a = rd16(0x7E1AEE) & 0x0001;
    if (!a) {
        for (i = 0; i < 3; i++) {
            w = rd16(0x7EFF66 + 2 * i + t);
            wr16(0x7E200C + 2 * i + obj, w);
        }
        a = (uint16_t)((w & 0xFF00) | rd8(0x7EFF6C + t));
        wr8(0x7E2015 + obj, (uint8_t)a);
    }
    a = (uint16_t)((a & 0xFF00) | rd8(0x7E1F2B));
    wr8(0x7E2049 + obj, (uint8_t)a);
    a = (uint16_t)((a & 0xFF00) | rd8(0x7E1F4C));
    wr8(0x7E204A + obj, (uint8_t)a);
    *x = t;
    *y = obj;
    return a;
}

void abi_ObjectPlaceFromTable(void)
{
    cpu.a = ObjectPlaceFromTable((uint8_t)cpu.a, (uint8_t)cpu.x, &cpu.x, &cpu.y);
}

/*
 * @routine 7E:D397 ObjectLinkEventTrigger
 * WHAT IT DOES: Unless bit 7 of $0DA4 is already set: when the object two
 * links on from the one at $0346 has bit 14 set in its first word and
 * $1A24 is non-zero, clears $1A24, sets $0DA4 to $80 and starts the values
 * at $1A1C-$1A22 ($1A, 0, $FE, 0).
 * HOW: Links through +$4F; everything through the data bank.
 * Entry: any widths. Exit: M 8-bit, X 8-bit (X, Y high bytes clear); A and
 * N, Z as left by the test that ended it (see the code).
 */
uint16_t ObjectLinkEventTrigger(uint8_t bank, uint16_t a, uint16_t *x, uint16_t *y, uint8_t *p)
{
    uint32_t b = (uint32_t)bank << 16;
    uint8_t f = rd8(b | 0x0DA4) & 0x80;

    if (f) {
        *p = (uint8_t)((*p & ~P_Z) | P_N);
        return (uint16_t)((a & 0xFF00) | f);
    }
    *x = rd16(b | 0x0346);
    *y = rd16(b | (uint16_t)(0x204F + *x));
    *y = rd16(b | (uint16_t)(0x204F + *y));
    a = rd16(b | *y) & 0x4000;
    if (!a || !(a = rd16(b | 0x1A24))) {
        *p = (uint8_t)((*p & ~P_N) | P_Z);
        return 0;
    }
    wr16(b | 0x1A24, 0x0000);
    wr8(b | 0x0DA4, 0x80);
    wr8(b | 0x1A1C, 0x1A);
    wr8(b | 0x1A1E, 0x00);
    wr8(b | 0x1A22, 0x00);
    wr8(b | 0x1A20, 0xFE);
    *p = (uint8_t)((*p & ~P_Z) | P_N);
    return 0x00FE;
}

void abi_ObjectLinkEventTrigger(void)
{
    uint8_t p = cpu.p;

    cpu.a = ObjectLinkEventTrigger(cpu.db, cpu.a, &cpu.x, &cpu.y, &p);
    cpu.x &= 0x00FF;
    cpu.y &= 0x00FF;
    cpu.p = (uint8_t)(p | P_M | P_X);
}

void abi_ObjectBucketInsert(void);

/*
 * @routine 03:CB25 ObjectInit
 * WHAT IT DOES: Clears object Y's record from +$04 on (47 words), puts it in
 * bucket 0's list, and sets its +$36 to 0 and its +$59 to $69.
 * HOW: Data bank $7E inside; ObjectBucketInsert; X and Y are swapped while
 * it works and swapped back.
 * Entry: M 8-bit, X 16-bit; Y = object. Exit: A = $69 with the object's
 * high byte; X, Y unchanged; M 8-bit; N, Z from the caller's data bank.
 */
void ObjectInit(uint16_t obj)
{
    unsigned i;

    for (i = 0; i < 0x2F; i++)
        wr16(0x7E2004 + 2 * i + obj, 0x0000);
    wr16(0x7E2040 + obj, 0x0000);
    ObjectBucketInsert(obj);
    wr8(0x7E2036 + obj, 0x00);
    wr8(0x7E2059 + obj, 0x69);
}

void abi_ObjectInit(void)
{
    ObjectInit(cpu.y);
    cpu.a = (uint16_t)((cpu.y & 0xFF00) | 0x69);
    cpu.p = (uint8_t)(nz8(cpu.p, cpu.db) | P_M);
}

/*
 * @routine 08:B8B4 ObjectPlaceAll
 * WHAT IT DOES: Sets up every object in the list at $7E:FFA6 (half the
 * count at $7E:FFB8 of them) with ObjectPlaceFromTable: object n (from 1)
 * takes table entry $7E:FFA6[n], and its $1F2B, $1F2D and $1F4C words from
 * the lists at $18D3, $18DB and $18E3.
 * HOW: Direct page $02 counts down from the count halved (arithmetic shift),
 * $08 counts the entry up from 1; the three words are stored through the
 * data bank before each call.
 * Entry: any widths, D = $0000. Exit: A = the last ObjectPlaceFromTable A;
 * X, Y, P restored, then N, Z from X (pulled at its entry width).
 */
uint16_t ObjectPlaceAll(uint8_t bank)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t n = rd16(0x7EFFB8), i = 0, a = 0, x, y;

    n = (uint16_t)((n >> 1) | (n & 0x8000));
    wr16(0x000002, n);
    wr16(0x000008, 0x0001);
    do {
        wr16(b | 0x1F2B, rd16(0x0018D3 + i));
        wr16(b | 0x1F2D, rd16(0x0018DB + i));
        wr16(b | 0x1F4C, rd16(0x0018E3 + i));
        x = rd16(0x7EFFA6 + i);
        a = ObjectPlaceFromTable((uint8_t)rd16(0x000008), (uint8_t)x, &x, &y);
        i = (uint16_t)(i + 2);
        wr16(0x000008, (uint16_t)(rd16(0x000008) + 1));
        n = (uint16_t)(rd16(0x000002) - 1);
        wr16(0x000002, n);
    } while (n);
    return a;
}

void abi_ObjectPlaceAll(void)
{
    cpu.a = ObjectPlaceAll(cpu.db);
    cpu.p = (cpu.p & P_X) ? nz8(cpu.p, (uint8_t)cpu.x) : nz16(cpu.p, cpu.x);
}

/*
 * @routine 08:E760 ObjectCountStep5F
 * WHAT IT DOES: Steps object X's counter +$49 up by one; while it has not
 * passed the limit at $1F58 its +$5F rises by $1F5A, after that +$5F falls
 * by $1F5A and the routine reports when the counter has reached $1F59.
 * HOW: 8-bit compares (BEQ/BMI on the difference); returns with carry set
 * only when the counter equals $1F59 on the falling side.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: C set when done; rising: A
 * low byte = the new +$5F, N, Z, V from the add; falling: A low byte = the
 * counter, N, Z from the compare with $1F59, V from the subtract.
 */
int ObjectCountStep5F(uint8_t bank, uint16_t obj, uint8_t *a, uint8_t *p)
{
    uint32_t c = field(bank, obj, 0x49), f = field(bank, obj, 0x5F), b = (uint32_t)bank << 16;
    uint8_t n = (uint8_t)(rd8(c) + 1), d = rd8(b | 0x1F5A), v = rd8(f), r;

    wr8(c, n);
    if (n == rd8(b | 0x1F58) || ((uint8_t)(n - rd8(b | 0x1F58)) & 0x80)) {
        r = (uint8_t)(v + d);
        wr8(f, r);
        *p = (uint8_t)((nz8(*p, r) & ~(P_C | P_V)) | ((~(v ^ d) & (v ^ r) & 0x80) ? P_V : 0));
        *a = r;
        return 0;
    }
    r = (uint8_t)(v - d);
    wr8(f, r);
    *p = (uint8_t)((*p & ~P_V) | (((v ^ d) & (v ^ r) & 0x80) ? P_V : 0));
    *a = n;
    r = (uint8_t)(n - rd8(b | 0x1F59));
    *p = (uint8_t)(nz8(*p, r) & ~P_C);
    if (r)
        return 0;
    *p |= P_C;
    return 1;
}

void abi_ObjectCountStep5F(void)
{
    uint8_t a = 0, p = cpu.p;

    ObjectCountStep5F(cpu.db, cpu.x, &a, &p);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | a);
    cpu.p = p;
}

/* A 16-bit subtract as SBC with carry set; V from it. */
static uint16_t sub16v(uint16_t a, uint16_t b, uint8_t *p)
{
    uint16_t r = (uint16_t)(a - b);

    *p = (uint8_t)((*p & ~P_V) | (((a ^ b) & (a ^ r) & 0x8000) ? P_V : 0));
    return r;
}

/* The magnitude as EOR #$FFFF / INC does it ($8000 stays $8000). */
static uint16_t magnitude(uint16_t v)
{
    return (v & 0x8000) ? (uint16_t)(-v) : v;
}

/*
 * @routine 08:B5FA ObjectNearFocus
 * WHAT IT DOES: Switches object X between near and far by its distance
 * from the object at $1A64 (|dX| + |dZ| against $1194). Near: clears bit 3
 * of +$24, zeroes +$5F and sets +$5D by its kind (+$49) with
 * ObjectSet5DByKind. Far: sets bit 3 of +$24, zeroes +$3A, sets +$5F to
 * $32, +$5D to $99A3 + 2 * kind, and puts a direction ((the $1A64 object's
 * +$11 + its own +$15) / 8, low two bits clear, bit 7 set) above the low
 * two bits of +$18.
 * HOW: 16-bit differences made positive by EOR/INC, summed, compared; the
 * rest in 8-bit through the data bank. $1F2B keeps |dX|.
 * Entry: M 8-bit, X 16-bit, D = $0000; X = object. Exit: Y = $1A64; M
 * 8-bit; near: as ObjectSet5DByKind (V from the distance add); far: A = the
 * +$5D value, N, Z, C, V from its add.
 */
uint16_t ObjectNearFocus(uint8_t bank, uint16_t obj, uint8_t *p)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t dx, dz, sum;
    unsigned s;
    uint8_t k;

    dx = magnitude(sub16v(rd16(b | 0x1A6C), rd16(field(bank, obj, 0x0C)), p));
    wr16(b | 0x1F2B, dx);
    dz = magnitude(sub16v(rd16(b | 0x1A70), rd16(field(bank, obj, 0x10)), p));
    s = (unsigned)dz + dx;
    sum = (uint16_t)s;
    *p = (uint8_t)((*p & ~P_V) | ((~(dz ^ dx) & (dz ^ sum) & 0x8000) ? P_V : 0));
    if ((uint16_t)(sum - 0x1194) & 0x8000) {
        wr8(field(bank, obj, 0x24), rd8(field(bank, obj, 0x24)) & 0xF7);
        wr8(field(bank, obj, 0x5F), 0x00);
        return ObjectSet5DByKind(bank, obj, rd8(field(bank, obj, 0x49)), p);
    }
    k = (uint8_t)(rd8(field(bank, obj, 0x18)) & 0x03);
    wr8(field(bank, obj, 0x18), k);
    k = (uint8_t)(((uint8_t)(rd8(b | 0x1A75) + rd8(field(bank, obj, 0x15))) >> 3 & 0xFC) | 0x80 | k);
    wr8(field(bank, obj, 0x18), k);
    wr8(field(bank, obj, 0x24), rd8(field(bank, obj, 0x24)) | 0x08);
    wr8(field(bank, obj, 0x3A), 0x00);
    wr8(field(bank, obj, 0x5F), 0x32);
    dx = (uint16_t)(rd8(field(bank, obj, 0x49)) << 1);
    s = (unsigned)dx + 0x99A3;
    sum = (uint16_t)s;
    *p = (uint8_t)((nz16(*p, sum) & ~(P_C | P_V)) | (s > 0xFFFF ? P_C : 0)
                   | ((~(dx ^ 0x99A3) & (dx ^ sum) & 0x8000) ? P_V : 0));
    wr16(field(bank, obj, 0x5D), sum);
    return sum;
}

void abi_ObjectNearFocus(void)
{
    uint8_t p = cpu.p;

    cpu.a = ObjectNearFocus(cpu.db, cpu.x, &p);
    cpu.y = 0x1A64;
    cpu.p = (uint8_t)(p | P_M);
}

uint16_t ScaleObjectXZ(uint16_t obj, uint8_t *p);

/*
 * @routine 08:83CC ObjectRebucket
 * WHAT IT DOES: Recomputes object X's bucket from its X and Z position
 * (ScaleObjectXZ) and, when it changed, moves the object from its old
 * bucket's list to the new one's.
 * HOW: Compared with +$40 (bank $7E); ObjectBucketRemove, the new +$40,
 * ObjectBucketInsert. P is saved and restored around it.
 * Entry: any M, X 16-bit, D = $0000; X = object. Exit: A = the bucket when
 * unchanged, else the object; X, Y, P unchanged; $02 and $94 as
 * ScaleDp02Dp94 leaves them.
 */
uint16_t ObjectRebucket(uint16_t obj)
{
    uint8_t p = 0;
    uint16_t key = ScaleObjectXZ(obj, &p);

    if (key == rd16(0x7E2040 + obj))
        return key;
    ObjectBucketRemove(obj);
    wr16(0x7E2040 + obj, key);
    ObjectBucketInsert(obj);
    return obj;
}

void abi_ObjectRebucket(void)
{
    cpu.a = ObjectRebucket(cpu.x);
}

void CrashWhite(uint8_t bank);

/*
 * @routine 08:B5B3 ObjectRecordFlag70Step
 * WHAT IT DOES: While $0D53 is clear and object X's countdown +$48 is
 * running, counts it down; otherwise sets bit 2 of the word at +$70 of the
 * object's Super FX record when $0D53 is set, clears it when not.
 * HOW: 8-bit tests through the data bank, 16-bit read-modify-write of the
 * record word. An object with no record branches to $08:B5F6, a JSL to the
 * stop screen CrashWhite ($00:8014) that would fall into ObjectNearFocus;
 * no script reaches that path.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; X, Y unchanged.
 * Counting: A low byte = the count before the step, N, Z from the new
 * count. Otherwise: A = the new record word, N, Z from X.
 */
uint16_t ObjectRecordFlag70Step(uint8_t bank, uint16_t obj, uint16_t a, uint8_t *p)
{
    uint32_t b = (uint32_t)bank << 16;
    uint8_t flag = rd8(b | 0x0D53), n;
    uint16_t rec, w;

    if (!flag) {
        n = rd8(field(bank, obj, 0x48));
        if (n) {
            wr8(field(bank, obj, 0x48), (uint8_t)(n - 1));
            *p = nz8(*p, (uint8_t)(n - 1));
            return (uint16_t)((a & 0xFF00) | n);
        }
    }
    rec = rd16(field(bank, obj, 0x22));
    if (!rec)
        CrashWhite(bank);
    w = rd16(0x700070 + rec);
    w = flag ? (uint16_t)(w | 0x0004) : (uint16_t)(w & 0xFFFB);
    wr16(0x700070 + rec, w);
    *p = nz16(*p, obj);
    return w;
}

void abi_ObjectRecordFlag70Step(void)
{
    uint8_t p = cpu.p;

    cpu.a = ObjectRecordFlag70Step(cpu.db, cpu.x, cpu.a, &p);
    cpu.p = (uint8_t)(p | P_M);
}

uint16_t GsuRecordWords88(void);

/* CMP of two 8-bit values: N, Z, C. */
static uint8_t cmp8(uint8_t p, uint8_t a, uint8_t m)
{
    return (uint8_t)((nz8(p, (uint8_t)(a - m)) & ~P_C) | (a >= m ? P_C : 0));
}

/* Points the linked object (+$4F) at the handler $08:addr. */
static uint16_t set_linked_handler(uint8_t bank, uint16_t obj, uint16_t addr)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t y = rd16(field(bank, obj, 0x4F));

    wr16(b | (uint16_t)(0x201A + y), addr);
    wr8(b | (uint16_t)(0x201C + y), 0x08);
    return y;
}

/*
 * @routine 08:B4C6 ObjectLinkedHandlerByState
 * WHAT IT DOES: Once per object (bit 3 of +$25 marks it done), gives
 * object X's linked object (+$4F) a new handler in bank 08 by the state
 * byte at $0DD6 + Y: 8 sets bits 3 of the record's +$64 and 5 of its +$70
 * and picks $91EB, $93D7 or $92DF by the record's +$88 (1, 4, other);
 * $0A or $0C clears bits 4, 5 and 10 of the record's +$42 and, when $0E59
 * is set, picks $94A1; other states pick $951B when $19ED is 8. Each pick
 * also runs GsuRecordWords88 unless bit 0 of $1AEE is set, and marks the
 * object done.
 * HOW: 8-bit compares; the handler is the linked object's +$1A (address)
 * and +$1C (bank). Only the $0A/$0C path is reached by a script.
 * Entry: M 8-bit, X 16-bit; X = object, Y = the state index. Exit: M
 * 8-bit; X unchanged; Y = the linked object when a handler was picked; A,
 * N, Z, C as the path leaves them; V unchanged.
 */
uint16_t ObjectLinkedHandlerByState(uint8_t bank, uint16_t obj, uint16_t a, uint16_t *y, uint8_t *p)
{
    uint32_t b = (uint32_t)bank << 16;
    uint8_t f = rd8(field(bank, obj, 0x25)), v, k;
    uint16_t rec, w;

    if (f & 0x08) {
        *p = nz8(*p, 0x08);
        return (uint16_t)((a & 0xFF00) | 0x08);
    }
    v = rd8(b | (uint16_t)(0x0DD6 + *y));
    *p = cmp8(*p, v, 0x08);
    if (v == 0x08) {
        rec = rd16(field(bank, obj, 0x22));
        wr16(0x700064 + rec, (uint16_t)(rd16(0x700064 + rec) | 0x0008));
        wr16(0x700070 + rec, (uint16_t)(rd16(0x700070 + rec) | 0x0020));
        k = (uint8_t)rd16(0x700088 + rec);
        wr8(b | 0x1F58, k);
        *p = cmp8(*p, k, 0x01);
        if (k != 0x01)
            *p = cmp8(*p, k, 0x04);
        *y = set_linked_handler(bank, obj, k == 0x01 ? 0x91EB : k == 0x04 ? 0x93D7 : 0x92DF);
    } else {
        *p = cmp8(*p, v, 0x0C);
        if (v != 0x0C)
            *p = cmp8(*p, v, 0x0A);
        if (v == 0x0C || v == 0x0A) {
            rec = rd16(field(bank, obj, 0x22));
            w = (uint16_t)(rd16(0x700042 + rec) & 0xFBCF);
            wr16(0x700042 + rec, w);
            v = rd8(b | 0x0E59);
            if (!v) {
                *p = nz8(*p, 0);
                return (uint16_t)(w & 0xFF00);
            }
            *y = set_linked_handler(bank, obj, 0x94A1);
        } else {
            v = rd8(b | 0x19ED);
            *p = cmp8(*p, v, 0x08);
            if (v != 0x08)
                return (uint16_t)((a & 0xFF00) | v);
            *y = set_linked_handler(bank, obj, 0x951B);
        }
    }
    a = 0x0000;
    if (!(rd16(b | 0x1AEE) & 0x0001))
        a = GsuRecordWords88();
    f = (uint8_t)(rd8(field(bank, obj, 0x25)) | 0x08);
    wr8(field(bank, obj, 0x25), f);
    *p = nz8(*p, f);
    return (uint16_t)((a & 0xFF00) | f);
}

void abi_ObjectLinkedHandlerByState(void)
{
    uint8_t p = cpu.p;

    cpu.a = ObjectLinkedHandlerByState(cpu.db, cpu.x, cpu.a, &cpu.y, &p);
    cpu.p = (uint8_t)(p | P_M);
}
