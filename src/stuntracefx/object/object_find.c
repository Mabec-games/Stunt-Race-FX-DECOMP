/*
 * object_find.c: searches of the object lists that nothing in the game
 * calls: the object whose box holds a point, the next object of a kind or
 * with a flag after a resume point kept at $1F30, the nearest object of a
 * kind, the first object of a kind and the count of a kind, and two
 * distances between objects. Objects are $62-byte records from $2000 in
 * the data bank (bank $7E) linked through their first word; the used list
 * starts at $0325. Fields: +$04 kind, +$0C/+$0E/+$10 position, +$24 flags,
 * +$27/+$29 half sizes.
 */
#include "machine.h"
#include "cpu_ops.h"

#define LIST_HEAD 0x0325u
#define RESUME 0x1F30u
#define WORK 0x1F2Bu
#define DIST 0x0364u

static uint32_t f(uint8_t bank, uint16_t obj, uint16_t off)
{
    return (uint32_t)bank << 16 | (uint16_t)(0x2000 + off + obj);
}

static uint32_t w(uint8_t bank, uint16_t a)
{
    return (uint32_t)bank << 16 | a;
}

/* LDA/LDX/LDY: N, Z from the 16-bit value. */
static uint16_t ld16(uint16_t v)
{
    cpu.p = nz16(cpu.p, v);
    return v;
}

/* CMP/CPX/CPY (16-bit). */
static void cmp16(uint16_t r, uint16_t m)
{
    cpu.p = (uint8_t)((nz16(cpu.p, (uint16_t)(r - m)) & ~P_C) | (r >= m ? P_C : 0));
}

/* SEC; SBC (16-bit). */
static uint16_t sub16(uint16_t a, uint16_t m)
{
    cpu.p |= P_C;
    return op_sbc16(a, m, &cpu.p);
}

/* EOR #$FFFF; INC A when negative (BPL skips it). */
static uint16_t abs16(uint16_t a)
{
    if (a & 0x8000)
        a = ld16((uint16_t)(~a + 1));
    return a;
}

/*
 * @routine 03:C08F ObjectAtPoint
 * WHAT IT DOES: Finds the first object in the used list with bit 0 of +$24
 * set whose box (half sizes +$27 in X, +$29 in Z) holds the point in
 * direct page $02 (X) and $08 (Z); X = that object, or 0.
 * HOW: For each object, |+$0C - $02| - +$27 and then |+$10 - $08| - +$29
 * must both be negative; the half size is put in direct page $04. The
 * skip to the next object is a JML within the routine.
 * Entry: M 16-bit, X 16-bit, D = $0000; data bank $7E. Exit: X = the
 * object or 0; Y = the last link read; A, N, V, C from the last subtract
 * (Z from X when none is found).
 */
void abi_ObjectAtPoint(void)
{
    uint8_t b = cpu.db;
    uint16_t x, a, m;

    x = ld16(rd16(w(b, LIST_HEAD)));
    if (!x)
        goto none;
loop:
    a = rd16(f(b, x, 0x24));
    a = ld16((uint16_t)(a & 0x0001));
    if (!a)
        goto next;
    m = rd16(f(b, x, 0x27));
    wr16(0x000004, m);
    a = rd16(f(b, x, 0x0C));
    m = rd16(0x000002);
    a = abs16(sub16(a, m));
    m = rd16(0x000004);
    a = sub16(a, m);
    if (!(a & 0x8000))
        goto next;
    m = rd16(f(b, x, 0x29));
    wr16(0x000004, m);
    a = rd16(f(b, x, 0x10));
    m = rd16(0x000008);
    a = abs16(sub16(a, m));
    m = rd16(0x000004);
    a = sub16(a, m);
    if (a & 0x8000) {
        cpu.a = a;
        cpu.x = x;
        return;
    }
next:
    cpu.a = a;
    cpu.y = ld16(rd16(f(b, x, 0x00)));
    x = ld16(cpu.y);
    if (x)
        goto loop;
none:
    cpu.x = ld16(0x0000);
}

/*
 * @routine 03:C0D6 ObjectNextOfKind
 * WHAT IT DOES: Steps the search kept at $1F30 to the next object of kind
 * A (any kind when A is 0), skipping the object X; Y = the object found
 * and $1F30 = the one after it. When the search has ended ($1F30 = 0) Y =
 * 0 and $1F30 stays 0. With a kind and no match left, Y = 0 and $1F30 =
 * object 0's link.
 * HOW: X is kept in direct page $3E; the list is walked by +$00 from
 * $1F30, comparing +$04 with A.
 * Entry: M 16-bit, X 16-bit, D = $0000; data bank $7E. Exit: A, X
 * unchanged; Y as above; N, Z from X; C from the last compare.
 */
void abi_ObjectNextOfKind(void)
{
    uint8_t b = cpu.db;
    uint16_t a = cpu.a, x, y = cpu.y, m;

    wr16(0x00003E, cpu.x);
    x = ld16(rd16(w(b, RESUME)));
    if (!x)
        goto ended;
    cmp16(a, 0x0000);
    if (!a)
        goto any;
    y = ld16(x);
kind:
    m = rd16(f(b, x, 0x04));
    cmp16(a, m);
    if (cpu.p & P_Z)
        goto found;
skip:
    y = ld16(rd16(f(b, x, 0x00)));
    x = ld16(y);
    if (x)
        goto kind;
found:
    m = rd16(0x00003E);
    cmp16(x, m);
    if (cpu.p & P_Z)
        goto skip;
    y = ld16(rd16(f(b, x, 0x00)));
    wr16(w(b, RESUME), y);
    cpu.y = ld16(x);
    cpu.x = ld16(rd16(0x00003E));
    return;
ended:
    cpu.y = ld16(0x0000);
    wr16(w(b, RESUME), 0x0000);
    cpu.x = ld16(rd16(0x00003E));
    return;
any:
    y = ld16(rd16(f(b, x, 0x00)));
    if (!y) {
        cpu.y = y;
        goto ended;
    }
    x = ld16(y);
    m = rd16(0x00003E);
    cmp16(x, m);
    if (cpu.p & P_Z)
        goto any;
    wr16(w(b, RESUME), x);
    cpu.y = y;
    cpu.x = ld16(rd16(0x00003E));
}

/*
 * @routine 03:C115 ObjectNextFlagged
 * WHAT IT DOES: As ObjectNextOfKind, but the object must also have one of
 * the flag bits in direct page $02 set in its +$24 byte; with A = 0 any
 * kind does. Y = the object found and $1F30 = the one after it.
 * HOW: X kept in direct page $3E; A kept in $1F2B around the 8-bit flag
 * test. The ended case shares ObjectNextOfKind's code.
 * Entry: M 16-bit, X 16-bit, D = $0000; data bank $7E. Exit: X unchanged;
 * Y as above; A = the kind with the flag test's byte in its low byte (as
 * entered when the search ended); M 16-bit; N, Z from X.
 */
void abi_ObjectNextFlagged(void)
{
    uint8_t b = cpu.db;
    uint16_t a = cpu.a, x, y = cpu.y, m;
    uint8_t v;

    cmp16(a, 0x0000);
    if (!a)
        goto any;
    wr16(0x00003E, cpu.x);
    x = ld16(rd16(w(b, RESUME)));
    if (!x)
        goto ended;
    y = ld16(x);
kind:
    m = rd16(f(b, x, 0x04));
    cmp16(a, m);
    if (cpu.p & P_Z)
        goto match;
skip:
    y = ld16(rd16(f(b, x, 0x00)));
    x = ld16(y);
    if (x)
        goto kind;
match:
    m = rd16(0x00003E);
    cmp16(x, m);
    if (cpu.p & P_Z)
        goto skip;
    wr16(w(b, WORK), a);
    v = rd8(f(b, x, 0x24));
    m = rd8(0x000002);
    v = (uint8_t)(v & m);
    a = (uint16_t)((a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, v);
    if (!v) {
        a = ld16(rd16(w(b, WORK)));
        goto skip;
    }
    y = ld16(rd16(f(b, x, 0x00)));
    wr16(w(b, RESUME), y);
    cpu.y = ld16(x);
    cpu.a = a;
    cpu.x = ld16(rd16(0x00003E));
    return;
ended:
    cpu.y = ld16(0x0000);
    wr16(w(b, RESUME), 0x0000);
    cpu.x = ld16(rd16(0x00003E));
    return;
any:
    wr16(0x00003E, cpu.x);
    x = ld16(rd16(w(b, RESUME)));
    if (!x)
        goto ended;
    y = ld16(x);
    cpu.p |= P_M;
flagloop:
    m = rd16(0x00003E);
    cmp16(x, m);
    if (cpu.p & P_Z)
        goto next;
    v = rd8(f(b, x, 0x24));
    m = rd8(0x000002);
    v = (uint8_t)(v & m);
    a = (uint16_t)((a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, v);
    if (v)
        goto found;
next:
    y = ld16(rd16(f(b, x, 0x00)));
    x = ld16(y);
    if (x)
        goto flagloop;
found:
    y = ld16(rd16(f(b, x, 0x00)));
    wr16(w(b, RESUME), y);
    cpu.y = ld16(x);
    cpu.a = a;
    cpu.x = ld16(rd16(0x00003E));
    cpu.p &= (uint8_t)~P_M;
}

void abi_ObjectDistanceApprox(void);

/* The distance test shared by ObjectNearestOfKind and ObjectFirstWithin:
   ObjectDistanceApprox from object y to x, then CMP $3E and CMP $42. */
static uint16_t distance(uint16_t x, uint16_t self, int *below_max)
{
    uint16_t d, m;

    cpu.x = x;
    cpu.y = self;
    abi_ObjectDistanceApprox();
    cpu.p &= (uint8_t)~P_M;
    d = ld16(rd16(w(cpu.db, DIST)));
    m = rd16(0x00003E);
    cmp16(d, m);
    *below_max = (cpu.p & P_N) != 0;
    return d;
}

/*
 * @routine 03:C175 ObjectNearestOfKind
 * WHAT IT DOES: From the search point $1F30, finds the object of kind A
 * (any kind when A is 0) other than X nearest to X by
 * ObjectDistanceApprox, counting only distances from $42 up to below $3E;
 * Y = that object (0 if none), $3E = its distance. The search is used up:
 * $1F30 = object 0's link (any kind: 0).
 * HOW: X kept in direct page $04, the kind in $0A, the best object in $40;
 * the compares are by the sign of the difference.
 * Entry: M 16-bit, X 16-bit, D = $0000; data bank $7E. Exit: X unchanged;
 * Y as above; A = the last distance (or the kind); M 16-bit; N, Z from X.
 */
void abi_ObjectNearestOfKind(void)
{
    uint8_t b = cpu.db;
    uint16_t a = cpu.a, x, y, m, self, d;
    int below;

    self = cpu.x;
    wr16(0x000004, self);
    x = ld16(rd16(w(b, RESUME)));
    if (!x)
        goto none;
    cmp16(a, 0x0000);
    if (!a)
        goto any;
    wr16(0x00000A, a);
    wr16(0x000040, 0x0000);
kind:
    m = rd16(0x000004);
    cmp16(x, m);
    if (cpu.p & P_Z)
        goto kind_next;
    a = ld16(rd16(f(b, x, 0x04)));
    m = rd16(0x00000A);
    cmp16(a, m);
    if (!(cpu.p & P_Z))
        goto kind_next;
    self = ld16(rd16(0x000004));
    d = distance(x, self, &below);
    a = d;
    if (!below)
        goto kind_next;
    m = rd16(0x000042);
    cmp16(d, m);
    if (cpu.p & P_N)
        goto kind_next;
    wr16(0x00003E, d);
    wr16(0x000040, x);
kind_next:
    y = ld16(rd16(f(b, x, 0x00)));
    x = ld16(y);
    if (x)
        goto kind;
    y = ld16(rd16(f(b, x, 0x00)));
    wr16(w(b, RESUME), y);
    cpu.y = ld16(rd16(0x000040));
    cpu.a = a;
    cpu.x = ld16(rd16(0x000004));
    return;
none:
    cpu.y = ld16(0x0000);
    cpu.x = ld16(rd16(0x000004));
    return;
any:
    wr16(0x000040, 0x0000);
any_loop:
    m = rd16(0x000004);
    cmp16(x, m);
    if (cpu.p & P_Z)
        goto any_next;
    self = ld16(rd16(0x000004));
    d = distance(x, self, &below);
    a = d;
    if (!below)
        goto any_next;
    m = rd16(0x000042);
    cmp16(d, m);
    if (cpu.p & P_N)
        goto any_next;
    wr16(0x00003E, d);
    wr16(0x000040, x);
any_next:
    y = ld16(rd16(f(b, x, 0x00)));
    x = ld16(y);
    if (x)
        goto any_loop;
    wr16(w(b, RESUME), y);
    cpu.y = ld16(rd16(0x000040));
    cpu.a = a;
    cpu.x = ld16(rd16(0x000004));
}

/*
 * @routine 03:C1EC ObjectFirstWithin
 * WHAT IT DOES: From the search point $1F30, finds the first object of
 * kind A (any kind when A is 0) other than X whose ObjectDistanceApprox
 * from X is from $42 up to below $3E; Y = that object and $1F30 = the one
 * after it. With none, Y = 0 and $1F30 = object 0's link.
 * HOW: X kept in direct page $04, the kind in $0A ($40 is cleared and not
 * used); the compares are by the sign of the difference. The ended case
 * shares ObjectNearestOfKind's code.
 * Entry: M 16-bit, X 16-bit, D = $0000; data bank $7E. Exit: X unchanged;
 * Y as above; A = the last distance (or the kind); M 16-bit; N, Z from X.
 */
void abi_ObjectFirstWithin(void)
{
    uint8_t b = cpu.db;
    uint16_t a = cpu.a, x, y, m, self, d;
    int below;

    self = cpu.x;
    wr16(0x000004, self);
    x = ld16(rd16(w(b, RESUME)));
    if (!x) {
        cpu.y = ld16(0x0000);
        cpu.x = ld16(rd16(0x000004));
        return;
    }
    cmp16(a, 0x0000);
    if (!a)
        goto any;
    wr16(0x00000A, a);
    wr16(0x000040, 0x0000);
kind:
    m = rd16(0x000004);
    cmp16(x, m);
    if (cpu.p & P_Z)
        goto kind_next;
    a = ld16(rd16(f(b, x, 0x04)));
    m = rd16(0x00000A);
    cmp16(a, m);
    if (!(cpu.p & P_Z))
        goto kind_next;
    self = ld16(rd16(0x000004));
    d = distance(x, self, &below);
    a = d;
    if (!below)
        goto kind_next;
    m = rd16(0x000042);
    cmp16(d, m);
    if (!(cpu.p & P_N))
        goto done;
kind_next:
    y = ld16(rd16(f(b, x, 0x00)));
    x = ld16(y);
    if (x)
        goto kind;
    goto done;
any:
    m = rd16(0x000004);
    cmp16(x, m);
    if (cpu.p & P_Z)
        goto any_next;
    self = ld16(rd16(0x000004));
    d = distance(x, self, &below);
    a = d;
    if (!below)
        goto any_next;
    m = rd16(0x000042);
    cmp16(d, m);
    if (!(cpu.p & P_N))
        goto done;
any_next:
    y = ld16(rd16(f(b, x, 0x00)));
    x = ld16(y);
    if (x)
        goto any;
done:
    y = ld16(rd16(f(b, x, 0x00)));
    wr16(w(b, RESUME), y);
    cpu.y = ld16(x);
    cpu.a = a;
    cpu.x = ld16(rd16(0x000004));
}

/*
 * @routine 03:C251 ObjectFirstOfKind
 * WHAT IT DOES: X = the first object of kind A in the used list, or 0.
 * HOW: Walks the list from $0325 by +$00, comparing +$04 with A. P is
 * saved and restored.
 * Entry: M 16-bit, X 16-bit; data bank $7E. Exit: A unchanged; X as
 * above; Y = the object or 0; P unchanged.
 */
void abi_ObjectFirstOfKind(void)
{
    uint8_t b = cpu.db;
    uint16_t x, y, m;

    y = rd16(w(b, LIST_HEAD));
    for (;;) {
        x = y;
        m = rd16(f(b, x, 0x04));
        if (cpu.a == m)
            break;
        y = rd16(f(b, x, 0x00));
        if (!y) {
            x = 0x0000;
            break;
        }
    }
    cpu.x = x;
    cpu.y = y;
}

/*
 * @routine 03:C265 ObjectCountOfKind
 * WHAT IT DOES: Counts the objects of kind A in the used list into
 * $1F2B, and returns the count.
 * HOW: Walks the list from $0325 by +$00, comparing +$04 with A; INC of
 * the word $1F2B per match. P is saved and restored.
 * Entry: M 16-bit, X 16-bit; data bank $7E. Exit: A = the count; X = the
 * last object; Y = 0; P unchanged.
 */
void abi_ObjectCountOfKind(void)
{
    uint8_t b = cpu.db;
    uint16_t x, y, m, n;

    y = rd16(w(b, LIST_HEAD));
    wr16(w(b, WORK), 0x0000);
    do {
        x = y;
        m = rd16(f(b, x, 0x04));
        if (cpu.a == m) {
            n = rd16(w(b, WORK));
            wr16(w(b, WORK), (uint16_t)(n + 1));
        }
        y = rd16(f(b, x, 0x00));
    } while (y);
    cpu.a = rd16(w(b, WORK));
    cpu.x = x;
    cpu.y = y;
}

/* LDA; CMP #$8000; ROR: halves keeping the sign, C = the bit shifted out. */
static uint16_t asr1(uint16_t v)
{
    uint16_t c = (v >= 0x8000) ? 0x8000 : 0;

    cpu.p = (uint8_t)((cpu.p & ~P_C) | (v & 1));
    v = (uint16_t)(c | v >> 1);
    cpu.p = nz16(cpu.p, v);
    return v;
}

/* CLC; ADC (16-bit). */
static uint16_t add16(uint16_t a, uint16_t m)
{
    cpu.p &= (uint8_t)~P_C;
    return op_adc16(a, m, &cpu.p);
}

/*
 * @routine 03:CA19 ObjectDistanceApprox
 * WHAT IT DOES: Sets $0364 to an estimate of the distance in X and Z
 * between objects Y and X: with dx, dz their halved absolute differences
 * in direct page $02 and $08, it is ((2 (dx + dz) + max) * 9 / 2) / 8,
 * about 1.1 times the true distance, which no square root would give
 * faster.
 * HOW: 16-bit subtracts and absolute values; the halvings are CMP #$8000,
 * ROR; times 9/2 as a / 2 + a * 4. Work: direct page $02 and $08.
 * Entry: X 16-bit, D = $0000; data bank $7E; X, Y = objects. Exit: M
 * 8-bit; A = the estimate (both bytes); X, Y unchanged; N, Z, C from the
 * last halving, V from the last add.
 */
void abi_ObjectDistanceApprox(void)
{
    uint8_t b = cpu.db;
    uint16_t a, m, t;

    cpu.p &= (uint8_t)~P_M;
    a = rd16(f(b, cpu.y, 0x0C));
    m = rd16(f(b, cpu.x, 0x0C));
    a = abs16(sub16(a, m));
    wr16(0x000002, a);
    a = rd16(f(b, cpu.y, 0x10));
    m = rd16(f(b, cpu.x, 0x10));
    a = abs16(sub16(a, m));
    wr16(0x000008, a);
    a = ld16(rd16(0x000002));
    wr16(0x000002, asr1(a));
    a = ld16(rd16(0x000008));
    wr16(0x000008, asr1(a));
    a = ld16(rd16(0x000008));
    m = rd16(0x000002);
    a = add16(a, m);
    cpu.p = (uint8_t)((cpu.p & ~P_C) | (a >> 15));
    a = ld16((uint16_t)(a << 1));
    wr16(w(b, DIST), a);
    a = ld16(rd16(0x000008));
    m = rd16(0x000002);
    cmp16(a, m);
    if (cpu.p & P_N)
        a = ld16(rd16(0x000002));
    else
        a = ld16(rd16(0x000008));
    m = rd16(w(b, DIST));
    a = add16(a, m);
    wr16(0x000002, a);
    cpu.p = (uint8_t)((cpu.p & ~P_C) | (a >> 15));
    a = (uint16_t)(a << 1);
    cpu.p = (uint8_t)((cpu.p & ~P_C) | (a >> 15));
    a = ld16((uint16_t)(a << 1));
    wr16(0x000008, a);
    a = ld16(rd16(0x000002));
    a = asr1(a);
    m = rd16(0x000008);
    a = add16(a, m);
    a = asr1(a);
    a = asr1(a);
    t = asr1(a);
    wr16(w(b, DIST), t);
    cpu.a = t;
    cpu.p |= P_M;
}

/*
 * @routine 03:CA85 ObjectDistanceXY
 * WHAT IT DOES: Sets $1A62 to |dx| + |dy| between objects Y and X (their
 * +$0C and +$0E), leaving dx in direct page $02 and dy in $08.
 * HOW: 16-bit subtracts; |dx| as 0 - dx; dy added or subtracted by its
 * sign.
 * Entry: X 16-bit, D = $0000; data bank $7E; X, Y = objects. Exit: M
 * 8-bit; A = the sum (both bytes); X, Y unchanged; N, Z, C, V from the
 * last add or subtract.
 */
void abi_ObjectDistanceXY(void)
{
    uint8_t b = cpu.db;
    uint16_t a, m;

    cpu.p &= (uint8_t)~P_M;
    a = rd16(f(b, cpu.y, 0x0C));
    m = rd16(f(b, cpu.x, 0x0C));
    a = sub16(a, m);
    wr16(0x000002, a);
    if (a & 0x8000) {
        m = rd16(0x000002);
        a = sub16(0x0000, m);
    }
    wr16(w(b, 0x1A62), a);
    a = rd16(f(b, cpu.y, 0x0E));
    m = rd16(f(b, cpu.x, 0x0E));
    a = sub16(a, m);
    wr16(0x000008, a);
    if (!(a & 0x8000)) {
        m = rd16(w(b, 0x1A62));
        a = add16(a, m);
    } else {
        a = rd16(w(b, 0x1A62));
        m = rd16(0x000008);
        a = sub16(a, m);
    }
    wr16(w(b, 0x1A62), a);
    cpu.a = a;
    cpu.p |= P_M;
}

void abi_Gsu81E3Long(void);

/* |a| as SEC; LDA #0; SBC when negative (BPL skips it). */
static uint16_t neg_if(uint16_t a, uint16_t at)
{
    if (a & 0x8000) {
        uint16_t m = rd16(at);

        a = sub16(0x0000, m);
    }
    return a;
}

/* dz (already stored at at) folded into $0364: added when not negative,
   else subtracted from it. */
static uint16_t fold(uint8_t b, uint16_t a, uint16_t at)
{
    uint16_t m;

    if (!(a & 0x8000)) {
        m = rd16(w(b, DIST));
        return add16(a, m);
    }
    a = rd16(w(b, DIST));
    m = rd16(at);
    return sub16(a, m);
}

/*
 * @routine 03:C836 ObjectOffsetDistanceXZ
 * WHAT IT DOES: Sets $0364 to |dx| + |dz| from object X to object Y moved
 * by direct page $04 in X and by $05F2 in Z, leaving dx in $02 and dz in
 * $08. Nothing in the game calls it.
 * HOW: 16-bit adds and subtracts, as ObjectDistanceXY.
 * Entry: X 16-bit, D = $0000; data bank $7E. Exit: M 8-bit; A = the sum;
 * X, Y unchanged; N, Z, C, V from the last add or subtract.
 */
void abi_ObjectOffsetDistanceXZ(void)
{
    uint8_t b = cpu.db;
    uint16_t a, m;

    cpu.p &= (uint8_t)~P_M;
    a = rd16(f(b, cpu.y, 0x0C));
    m = rd16(0x000004);
    a = add16(a, m);
    m = rd16(f(b, cpu.x, 0x0C));
    a = sub16(a, m);
    wr16(0x000002, a);
    a = neg_if(a, 0x0002);
    wr16(w(b, DIST), a);
    a = rd16(f(b, cpu.y, 0x10));
    m = rd16(w(b, 0x05F2));
    a = add16(a, m);
    m = rd16(f(b, cpu.x, 0x10));
    a = sub16(a, m);
    wr16(0x000008, a);
    a = fold(b, a, 0x0008);
    wr16(w(b, DIST), a);
    cpu.a = a;
    cpu.p |= P_M;
}

/* ObjectPointDistanceXZ and ObjectPointDistanceXY: from object X to the
   point (direct page $04, the word at second, compared with field2). */
static void point_distance(uint32_t second, uint16_t field2)
{
    uint8_t b = cpu.db;
    uint16_t a, m;

    cpu.p &= (uint8_t)~P_M;
    a = rd16(0x000004);
    m = rd16(f(b, cpu.x, 0x0C));
    a = sub16(a, m);
    wr16(0x000002, a);
    a = neg_if(a, 0x0002);
    wr16(w(b, DIST), a);
    a = rd16(second);
    m = rd16(f(b, cpu.x, field2));
    a = sub16(a, m);
    wr16(0x000008, a);
    a = fold(b, a, 0x0008);
    wr16(w(b, DIST), a);
    cpu.a = a;
    cpu.p |= P_M;
}

/*
 * @routine 03:C870 ObjectPointDistanceXZ
 * WHAT IT DOES: Sets $0364 to |dx| + |dz| from object X to the point in
 * direct page $04 (X) and $05F2 (Z), leaving dx in $02 and dz in $08.
 * Nothing in the game calls it but ObjectPointGsu81E3.
 * HOW: 16-bit subtracts, as ObjectDistanceXY; $05F2 through the data
 * bank.
 * Entry: X 16-bit, D = $0000; data bank $7E. Exit: M 8-bit; A = the sum;
 * X, Y unchanged; N, Z, C, V from the last add or subtract.
 */
void abi_ObjectPointDistanceXZ(void)
{
    point_distance(w(cpu.db, 0x05F2), 0x10);
}

/*
 * @routine 03:C8A2 ObjectPointDistanceXY
 * WHAT IT DOES: Sets $0364 to |dx| + |dy| from object X to the point in
 * direct page $04 (X) and $0A (Y), leaving dx in $02 and dy in $08.
 * Nothing in the game calls it.
 * HOW: 16-bit subtracts, as ObjectDistanceXY.
 * Entry: X 16-bit, D = $0000; data bank $7E. Exit: M 8-bit; A = the sum;
 * X, Y unchanged; N, Z, C, V from the last add or subtract.
 */
void abi_ObjectPointDistanceXY(void)
{
    point_distance(0x00000A, 0x0E);
}

/*
 * @routine 03:C81F ObjectPointGsu81E3
 * WHAT IT DOES: ObjectPointDistanceXZ, then Super FX program $01:81E3 on
 * the dx, dz it leaves (Gsu81E3Long); returns the result's high byte.
 * Nothing in the game calls it.
 * HOW: X and Y pushed and pulled around the calls; XBA, AND #$00FF.
 * Entry: X 16-bit, D = $0000; data bank $7E. Exit: M 8-bit, X 16-bit; A
 * = the high byte of Gsu81E3Long's result (high byte 0); X, Y unchanged;
 * N, Z from X.
 */
void abi_ObjectPointGsu81E3(void)
{
    uint16_t x = cpu.x, y = cpu.y;

    abi_ObjectPointDistanceXZ();
    abi_Gsu81E3Long();
    cpu.a = (uint16_t)(cpu.a >> 8);
    cpu.p = (uint8_t)((cpu.p | P_M) & ~P_X);
    cpu.y = y;
    cpu.x = x;
    cpu.p = nz16(cpu.p, x);
}

/*
 * @routine 03:C8D3 ObjectVelocityAddFrom
 * WHAT IT DOES: Adds object Y's velocity (+$2E, +$30, +$32) to object
 * X's, keeping Y's in direct page $02, $08, $94. Nothing in the game
 * calls it.
 * HOW: X and Y swapped through direct page $3E (twice); 16-bit adds.
 * Entry: X 16-bit, D = $0000; data bank $7E. Exit: M 8-bit; A = X's new
 * +$32; X, Y unchanged; $3E = Y; N, Z, C, V from the last add.
 */
void abi_ObjectVelocityAddFrom(void)
{
    uint8_t b = cpu.db;
    uint16_t a, m, i;
    static const uint16_t dp[3] = {0x02, 0x08, 0x94};

    cpu.p &= (uint8_t)~P_M;
    wr16(0x00003E, cpu.x);
    for (i = 0; i < 3; i++) {
        a = rd16(f(b, cpu.y, (uint16_t)(0x2E + 2 * i)));
        wr16(dp[i], a);
    }
    wr16(0x00003E, cpu.y);
    for (i = 0; i < 3; i++) {
        a = rd16(f(b, cpu.x, (uint16_t)(0x2E + 2 * i)));
        m = rd16(dp[i]);
        a = add16(a, m);
        wr16(f(b, cpu.x, (uint16_t)(0x2E + 2 * i)), a);
    }
    cpu.a = a;
    cpu.p = (uint8_t)(cpu.p | P_M);
}

/* The Gsu81E3Long wrappers: X and Y pushed and pulled, 16-bit after. */
static void gsu_pair_end(uint16_t x, uint16_t y)
{
    abi_Gsu81E3Long();
    cpu.p &= (uint8_t)~(P_M | P_X);
    cpu.y = y;
    cpu.x = x;
    cpu.p = nz16(cpu.p, x);
}

/*
 * @routine 03:C90C ObjectPairGsu81E3
 * WHAT IT DOES: Runs Super FX program $01:81E3 (Gsu81E3Long) on the X and
 * Z differences from object X to object Y, put in direct page $02 and
 * $08. Nothing in the game calls it.
 * HOW: X and Y pushed and pulled around it; subtracts in the entry width.
 * Entry: M 16-bit, X 16-bit, D = $0000; data bank $7E. Exit: M, X 16-bit;
 * A as Gsu81E3Long leaves it; X, Y unchanged; N, Z from X.
 */
void abi_ObjectPairGsu81E3(void)
{
    uint8_t b = cpu.db;
    uint16_t x = cpu.x, y = cpu.y, a, m;

    a = rd16(f(b, y, 0x0C));
    m = rd16(f(b, x, 0x0C));
    a = sub16(a, m);
    wr16(0x000002, a);
    a = rd16(f(b, y, 0x10));
    m = rd16(f(b, x, 0x10));
    a = sub16(a, m);
    wr16(0x000008, a);
    gsu_pair_end(x, y);
}

/*
 * @routine 03:C929 ObjectPairPitchGsu81E3
 * WHAT IT DOES: Runs Super FX program $01:81E3 (Gsu81E3Long) on the
 * height difference from object X to object Y (direct page $02) and
 * their ObjectDistanceApprox (direct page $08). Nothing in the game calls
 * it.
 * HOW: X and Y pushed and pulled around it; 16-bit subtract.
 * Entry: X 16-bit, D = $0000; data bank $7E. Exit: M, X 16-bit; A as
 * Gsu81E3Long leaves it; X, Y unchanged; N, Z from X.
 */
void abi_ObjectPairPitchGsu81E3(void)
{
    uint8_t b = cpu.db;
    uint16_t x = cpu.x, y = cpu.y, a, m;

    abi_ObjectDistanceApprox();
    cpu.p &= (uint8_t)~P_M;
    a = rd16(w(b, DIST));
    wr16(0x000008, a);
    a = rd16(f(b, y, 0x0E));
    m = rd16(f(b, x, 0x0E));
    a = sub16(a, m);
    wr16(0x000002, a);
    gsu_pair_end(x, y);
}

/*
 * @routine 03:C948 ObjectPairOffsetGsu81E3
 * WHAT IT DOES: As ObjectPairGsu81E3 with object Y moved by direct page
 * $04 in X and $05F2 in Z. Nothing in the game calls it.
 * HOW: The differences in the entry width, then the offsets added in 16
 * bits; X and Y pushed and pulled around it.
 * Entry: M 16-bit, X 16-bit, D = $0000; data bank $7E. Exit: M, X 16-bit;
 * A as Gsu81E3Long leaves it; X, Y unchanged; N, Z from X.
 */
void abi_ObjectPairOffsetGsu81E3(void)
{
    uint8_t b = cpu.db;
    uint16_t x = cpu.x, y = cpu.y, a, m;

    a = rd16(f(b, y, 0x0C));
    m = rd16(f(b, x, 0x0C));
    a = sub16(a, m);
    wr16(0x000002, a);
    a = rd16(f(b, y, 0x10));
    m = rd16(f(b, x, 0x10));
    a = sub16(a, m);
    wr16(0x000008, a);
    a = rd16(0x000002);
    m = rd16(0x000004);
    a = add16(a, m);
    wr16(0x000002, a);
    a = rd16(0x000008);
    m = rd16(w(b, 0x05F2));
    a = add16(a, m);
    wr16(0x000008, a);
    gsu_pair_end(x, y);
}

/*
 * @routine 03:C976 ObjectPairOffsetPitchGsu81E3
 * WHAT IT DOES: Runs Super FX program $01:81E3 (Gsu81E3Long) on the
 * height difference from object X to object Y less direct page $0A
 * (direct page $02) and ObjectOffsetDistanceXZ's sum (direct page $08).
 * Nothing in the game calls it.
 * HOW: X and Y pushed and pulled around it; 16-bit subtracts.
 * Entry: X 16-bit, D = $0000; data bank $7E. Exit: M, X 16-bit; A as
 * Gsu81E3Long leaves it; X, Y unchanged; N, Z from X.
 */
void abi_ObjectPairOffsetPitchGsu81E3(void)
{
    uint8_t b = cpu.db;
    uint16_t x = cpu.x, y = cpu.y, a, m;

    abi_ObjectOffsetDistanceXZ();
    cpu.p &= (uint8_t)~P_M;
    a = rd16(w(b, DIST));
    wr16(0x000008, a);
    a = rd16(f(b, y, 0x0E));
    m = rd16(f(b, x, 0x0E));
    a = sub16(a, m);
    m = rd16(0x00000A);
    a = sub16(a, m);
    wr16(0x000002, a);
    gsu_pair_end(x, y);
}
