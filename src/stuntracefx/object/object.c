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
