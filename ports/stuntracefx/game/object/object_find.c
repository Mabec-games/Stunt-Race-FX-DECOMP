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
#include "object_calls.h"

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
/* The flags the searches leave for their adapters: C and V of the last
   compare, subtract or add, with which of them were set (fcv_set: bit 0
   C, bit 1 V). N and Z come from the value the routine ends on. */
static uint8_t fc, fv;
static int fcv_set;

/* The flags over p: C and V where set, N and Z from v. */
static uint8_t find_flags(uint8_t p, uint16_t v)
{
    if (fcv_set & 1)
        p = (uint8_t)((p & ~P_C) | fc);
    if (fcv_set & 2)
        p = (uint8_t)((p & ~P_V) | fv);
    return nz16(p, v);
}

/* LDA/LDX/LDY (16-bit): its time. */
static uint16_t ld16(uint16_t v)
{
    cyc(16);
    return v;
}

/* CMP/CPX/CPY (16-bit). */
static void cmp16(uint16_t r, uint16_t m)
{
    cyc(22);
    fc = r >= m ? P_C : 0;
    fcv_set |= 1;
}

/* SEC; SBC (16-bit). */
static uint16_t sub16(uint16_t a, uint16_t m)
{
    uint16_t r = (uint16_t)(a - m);

    cyc(6);                             /* op_sbc16's stated time */
    fc = a >= m ? P_C : 0;
    fv = ((a ^ m) & (a ^ r) & 0x8000) ? P_V : 0;
    fcv_set |= 3;
    return r;
}

/* EOR #$FFFF; INC A when negative (BPL skips it). */
static uint16_t abs16(uint16_t a)
{
    if (a & 0x8000)
        { cyc(16); a = ld16((uint16_t)(~a + 1)); }
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
/* What the searches leave in A and Y when they set them (ex_set: bit 0
   A, bit 1 Y). */
static uint16_t ex_a, ex_y;
static int ex_set;

uint16_t ObjectAtPoint(uint8_t b)
{
    uint16_t x, a, m;

    ex_set = fcv_set = 0;
    x = ld16(rd16(cyc_then(24, w(b, LIST_HEAD))));
    if (!x)
        { cyc(8); goto none; }
loop:
    a = rd16(cyc_then(30, f(b, x, 0x24)));
    a = ld16((uint16_t)(a & 0x0001));
    if (!a)
        { cyc(46); goto next; }
    m = rd16(cyc_then(60, f(b, x, 0x27)));
    cyc(16);
    wr16(0x000004, m);
    a = rd16(cyc_then(30, f(b, x, 0x0C)));
    cyc(30);
    m = rd16(0x000002);
    a = abs16(sub16(a, m));
    cyc(46);
    m = rd16(0x000004);
    a = sub16(a, m);
    if (!(a & 0x8000))
        { cyc(6); goto next; }
    m = rd16(cyc_then(40, f(b, x, 0x29)));
    cyc(16);
    wr16(0x000004, m);
    a = rd16(cyc_then(30, f(b, x, 0x10)));
    cyc(30);
    m = rd16(0x000008);
    a = abs16(sub16(a, m));
    cyc(46);
    m = rd16(0x000004);
    a = sub16(a, m);
    if (a & 0x8000) {
        cyc(60);
        ex_a = a;
        ex_set |= 1;
        return x;
    }
next:
    ex_a = a;
    ex_y = ld16(rd16(cyc_then(40, f(b, x, 0x00))));
    ex_set = 3;
    x = ld16(ex_y);
    if (x)
        { cyc(4); goto loop; }
none:
    cyc(50);
    ld16(0x0000);
    return 0;
}

void abi_ObjectAtPoint(void)
{
    cpu.x = ObjectAtPoint(cpu.db);
    if (ex_set & 1)
        cpu.a = ex_a;
    if (ex_set & 2)
        cpu.y = ex_y;
    cpu.p = find_flags(cpu.p, cpu.x ? cpu.a : 0x0000);
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
static uint16_t ex_x;                   /* X reloaded from $3E */

uint16_t ObjectNextOfKind(uint8_t b, uint16_t a, uint16_t self)
{
    uint16_t x, y, m;

    fcv_set = 0;
    cyc(16);
    wr16(0x00003E, self);
    x = ld16(rd16(cyc_then(24, w(b, RESUME))));
    if (!x)
        goto ended;
    cmp16(a, 0x0000);
    if (!a)
        goto any; else { cyc(12); }
    y = ld16(x);
kind:
    m = rd16(cyc_then(34, f(b, x, 0x04)));
    cmp16(a, m);
    if (a == m)
        { cyc(2); goto found; }
skip:
    y = ld16(rd16(cyc_then(24, f(b, x, 0x00))));
    x = ld16(y);
    if (x)
        goto kind;
found:
    cyc(14);
    m = rd16(0x00003E);
    cyc(6);
    cmp16(x, m);
    if (x == m)
        goto skip;
    y = ld16(rd16(cyc_then(18, f(b, x, 0x00))));
    wr16(w(b, RESUME), cyc_then(8, y));
    y = ld16(x);
    cyc(14);
    ex_x = ld16(cyc_then(28, rd16(0x00003E)));
    return y;
ended:
    y = ld16(0x0000);
    wr16(w(b, RESUME), cyc_then(38, 0x0000));
    cyc(16);
    ex_x = ld16(cyc_then(28, rd16(0x00003E)));
    return y;
any:
    y = ld16(rd16(cyc_then(54, f(b, x, 0x00))));
    if (!y)
        goto ended;
    x = ld16(y);
    cyc(14);
    m = rd16(0x00003E);
    cmp16(x, m);
    if (x == m)
        goto any;
    wr16(w(b, RESUME), cyc_then(18, x));
    cyc(16);
    ex_x = ld16(cyc_then(28, rd16(0x00003E)));
    return y;
}

void abi_ObjectNextOfKind(void)
{
    cpu.y = ObjectNextOfKind(cpu.db, cpu.a, cpu.x);
    cpu.x = ex_x;
    cpu.p = find_flags(cpu.p, ex_x);
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
uint16_t ObjectNextFlagged(uint8_t b, uint16_t a, uint16_t self)
{
    uint16_t x, y, m;
    uint8_t v;

    fcv_set = ex_set = 0;
    cmp16(a, 0x0000);
    if (!a)
        goto any;
    cyc(34);
    wr16(0x00003E, self);
    x = ld16(rd16(cyc_then(24, w(b, RESUME))));
    if (!x)
        goto ended;
    y = ld16(x);
kind:
    m = rd16(cyc_then(28, f(b, x, 0x04)));
    cmp16(a, m);
    if (a == m)
        { cyc(2); goto match; }
skip:
    y = ld16(rd16(cyc_then(24, f(b, x, 0x00))));
    x = ld16(y);
    if (x)
        { cyc(6); goto kind; }
match:
    cyc(14);
    m = rd16(0x00003E);
    cmp16(x, m);
    if (x == m)
        goto skip;
    wr16(w(b, WORK), cyc_then(18, a));
    v = rd8(cyc_then(52, f(b, x, 0x24)));
    cyc(16);
    m = rd8(0x000002);
    v = (uint8_t)(v & m);
    a = (uint16_t)((a & 0xFF00) | v);
    if (!v) {
        a = ld16(cyc_then(12, rd16(cyc_then(62, w(b, WORK)))));
        goto skip;
    }
    y = ld16(rd16(cyc_then(74, f(b, x, 0x00))));
    wr16(w(b, RESUME), cyc_then(8, y));
    y = ld16(x);
    ex_a = a;
    ex_set = 1;
    cyc(14);
    ex_x = ld16(cyc_then(28, rd16(0x00003E)));
    return y;
ended:
    y = ld16(0x0000);
    wr16(w(b, RESUME), cyc_then(38, 0x0000));
    cyc(16);
    ex_x = ld16(cyc_then(28, rd16(0x00003E)));
    return y;
any:
    cyc(40);
    wr16(0x00003E, self);
    x = ld16(rd16(cyc_then(24, w(b, RESUME))));
    if (!x)
        goto ended; else { cyc(16); }
    y = ld16(x);
flagloop:
    cyc(20);
    m = rd16(0x00003E);
    cmp16(x, m);
    if (x == m)
        goto next;
    v = rd8(cyc_then(24, f(b, x, 0x24)));
    cyc(16);
    m = rd8(0x000002);
    v = (uint8_t)(v & m);
    a = (uint16_t)((a & 0xFF00) | v);
    if (v)
        { cyc(24); goto found; } else { cyc(16); }
next:
    y = ld16(rd16(cyc_then(30, f(b, x, 0x00))));
    x = ld16(y);
    if (x)
        goto flagloop;
found:
    y = ld16(rd16(cyc_then(28, f(b, x, 0x00))));
    wr16(w(b, RESUME), cyc_then(8, y));
    y = ld16(x);
    ex_a = a;
    ex_set = 1 | 4;                     /* 4: M 16-bit at the end (the loop ran 8-bit) */
    cyc(14);
    ex_x = ld16(cyc_then(50, rd16(0x00003E)));
    return y;
}

void abi_ObjectNextFlagged(void)
{
    cpu.y = ObjectNextFlagged(cpu.db, cpu.a, cpu.x);
    cpu.x = ex_x;
    if (ex_set & 1)
        cpu.a = ex_a;
    if (ex_set & 4)
        cpu.p &= (uint8_t)~P_M;
    cpu.p = find_flags(cpu.p, ex_x);
}

uint16_t ObjectDistanceApprox(uint8_t b, uint16_t x, uint16_t y);

/* The distance test shared by ObjectNearestOfKind and ObjectFirstWithin:
   ObjectDistanceApprox from object y to x, then CMP $3E and CMP $42. */
static uint16_t distance(uint8_t b, uint16_t x, uint16_t self, int *below_max)
{
    uint16_t d, m;

    (cyc(46), ObjectDistanceApprox(b, x, self));
    d = ld16(rd16(cyc_then(46, w(b, DIST))));
    m = rd16(0x00003E);
    cmp16(d, m);
    *below_max = ((uint16_t)(d - m) & 0x8000) != 0;
    ex_set |= 4;                        /* M 16-bit after it */
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
uint16_t ObjectNearestOfKind(uint8_t b, uint16_t a, uint16_t self)
{
    uint16_t x, y, m, d;
    int below;

    fcv_set = ex_set = 0;
    cyc(16);
    wr16(0x000004, self);
    x = ld16(rd16(cyc_then(24, w(b, RESUME))));
    if (!x)
        goto none;
    cmp16(a, 0x0000);
    if (!a)
        goto any;
    cyc(40);
    wr16(0x00000A, a);
    cyc(16);
    wr16(0x000040, 0x0000);
kind:
    cyc(16);
    m = rd16(0x000004);
    cmp16(x, m);
    if (x == m)
        goto kind_next;
    a = ld16(rd16(cyc_then(24, f(b, x, 0x04))));
    m = rd16(0x00000A);
    cmp16(a, m);
    if (a != m)
        goto kind_next;
    cyc(10);
    self = ld16(rd16(0x000004));
    d = distance(b, x, self, &below);
    a = d;
    if (!below)
        goto kind_next;
    cyc(10);
    m = rd16(0x000042);
    cmp16(d, m);
    if ((uint16_t)(d - m) & 0x8000)
        goto kind_next;
    cyc(10);
    wr16(0x00003E, d);
    cyc(16);
    wr16(0x000040, x);
kind_next:
    y = ld16(rd16(cyc_then(30, f(b, x, 0x00))));
    x = ld16(y);
    if (x)
        { cyc(4); goto kind; }
    y = ld16(rd16(cyc_then(28, f(b, x, 0x00))));
    wr16(w(b, RESUME), cyc_then(8, y));
    goto done;
none:
    y = ld16(0x0000);
    cyc(54);
    ex_x = ld16(cyc_then(28, rd16(0x000004)));
    return y;
any:
    cyc(46);
    wr16(0x000040, 0x0000);
any_loop:
    cyc(16);
    m = rd16(0x000004);
    cmp16(x, m);
    if (x == m)
        goto any_next;
    cyc(10);
    self = ld16(rd16(0x000004));
    d = distance(b, x, self, &below);
    a = d;
    if (!below)
        goto any_next;
    cyc(10);
    m = rd16(0x000042);
    cmp16(d, m);
    if ((uint16_t)(d - m) & 0x8000)
        goto any_next;
    cyc(10);
    wr16(0x00003E, d);
    cyc(16);
    wr16(0x000040, x);
any_next:
    y = ld16(rd16(cyc_then(30, f(b, x, 0x00))));
    x = ld16(y);
    if (x)
        { cyc(4); goto any_loop; }
    wr16(w(b, RESUME), cyc_then(22, y));
done:
    cyc(16);
    y = ld16(rd16(0x000040));
    ex_a = a;
    ex_set |= 1;
    ex_x = ld16(cyc_then(28, rd16(0x000004)));
    return y;
}

/* The registers ObjectNearestOfKind and ObjectFirstWithin leave. */
static void within_exit(uint16_t y)
{
    cpu.y = y;
    cpu.x = ex_x;
    if (ex_set & 1)
        cpu.a = ex_a;
    if (ex_set & 4)
        cpu.p &= (uint8_t)~P_M;
    cpu.p = find_flags(cpu.p, ex_x);
}

void abi_ObjectNearestOfKind(void)
{
    within_exit(ObjectNearestOfKind(cpu.db, cpu.a, cpu.x));
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
uint16_t ObjectFirstWithin(uint8_t b, uint16_t a, uint16_t self)
{
    uint16_t x, y, m, d;
    int below;

    fcv_set = ex_set = 0;
    cyc(16);
    wr16(0x000004, self);
    x = ld16(rd16(cyc_then(24, w(b, RESUME))));
    if (!x) {
        y = ld16(0x0000);
        cyc(30);
        ex_x = ld16(cyc_then(28, rd16(0x000004)));
        return y;
    }
    cmp16(a, 0x0000);
    if (!a)
        { cyc(20); goto any; }
    cyc(34);
    wr16(0x00000A, a);
    cyc(16);
    wr16(0x000040, 0x0000);
kind:
    cyc(16);
    m = rd16(0x000004);
    cmp16(x, m);
    if (x == m)
        { cyc(6); goto kind_next; }
    a = ld16(rd16(cyc_then(24, f(b, x, 0x04))));
    m = rd16(0x00000A);
    cmp16(a, m);
    if (a != m)
        { cyc(6); goto kind_next; }
    cyc(10);
    self = ld16(rd16(0x000004));
    d = distance(b, x, self, &below);
    a = d;
    if (!below)
        goto kind_next;
    cyc(10);
    m = rd16(0x000042);
    cmp16(d, m);
    if (!((uint16_t)(d - m) & 0x8000))
        { cyc(2); goto done; }
kind_next:
    y = ld16(rd16(cyc_then(24, f(b, x, 0x00))));
    x = ld16(y);
    if (x)
        { cyc(4); goto kind; }
    goto done;
any:
    cyc(20);
    m = rd16(0x000004);
    cmp16(x, m);
    if (x == m)
        goto any_next;
    cyc(10);
    self = ld16(rd16(0x000004));
    d = distance(b, x, self, &below);
    a = d;
    if (!below)
        goto any_next;
    cyc(10);
    m = rd16(0x000042);
    cmp16(d, m);
    if (!((uint16_t)(d - m) & 0x8000))
        { cyc(2); goto done; }
any_next:
    y = ld16(rd16(cyc_then(30, f(b, x, 0x00))));
    x = ld16(y);
    if (x)
        goto any;
done:
    y = ld16(rd16(cyc_then(28, f(b, x, 0x00))));
    wr16(w(b, RESUME), cyc_then(8, y));
    y = ld16(x);
    ex_a = a;
    ex_set |= 1;
    cyc(14);
    ex_x = ld16(cyc_then(28, rd16(0x000004)));
    return y;
}

void abi_ObjectFirstWithin(void)
{
    within_exit(ObjectFirstWithin(cpu.db, cpu.a, cpu.x));
}

/*
 * @routine 03:C251 ObjectFirstOfKind
 * WHAT IT DOES: X = the first object of kind A in the used list, or 0.
 * HOW: Walks the list from $0325 by +$00, comparing +$04 with A. P is
 * saved and restored.
 * Entry: M 16-bit, X 16-bit; data bank $7E. Exit: A unchanged; X as
 * above; Y = the object or 0; P unchanged.
 */
uint16_t ObjectFirstOfKind(uint8_t b, uint16_t a)
{
    uint16_t x, y, m;

    y = rd16(cyc_then(46, w(b, LIST_HEAD)));
    for (;;) {
        x = y;
        m = rd16(cyc_then(44, f(b, x, 0x04)));
        if (a == m)
            break;
        y = rd16(cyc_then(46, f(b, x, 0x00)));
        if (!y) {
            cyc(18);
            x = 0x0000;
            break;
        } else { cyc(22); }
    }
    cyc(94);
    ex_y = y;
    return x;
}

void abi_ObjectFirstOfKind(void)
{
    cpu.x = ObjectFirstOfKind(cpu.db, cpu.a);
    cpu.y = ex_y;
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
uint16_t ObjectCountOfKind(uint8_t b, uint16_t a)
{
    uint16_t x, y, m, n;
    int first = 1;

    y = rd16(cyc_then(46, w(b, LIST_HEAD)));
    wr16(w(b, WORK), cyc_then(24, 0x0000));
    do {
        x = y;
        m = rd16(cyc_then(first ? 44 : 66, f(b, x, 0x04)));
        first = 0;
        if (a == m) {
            n = rd16(cyc_then(40, w(b, WORK)));
            wr16(w(b, WORK), cyc_then(14, (uint16_t)(n + 1)));
        } else { cyc(22); }
        y = rd16(cyc_then(30, f(b, x, 0x00)));
    } while (y);
    ex_x = x;
    n = rd16(cyc_then(40, w(b, WORK)));
    cyc(72);                            /* PLP, RTL */
    return n;
}

void abi_ObjectCountOfKind(void)
{
    cpu.a = ObjectCountOfKind(cpu.db, cpu.a);
    cpu.x = ex_x;
    cpu.y = 0x0000;
}

/* LDA; CMP #$8000; ROR: halves keeping the sign, C = the bit shifted out. */
static uint16_t asr1(uint16_t v)
{
    uint16_t c = (v >= 0x8000) ? 0x8000 : 0;

    fc = (uint8_t)(v & 1);
    fcv_set |= 1;
    return (uint16_t)(c | v >> 1);
}

/* CLC; ADC (16-bit). */
static uint16_t add16(uint16_t a, uint16_t m)
{
    uint16_t r = (uint16_t)(a + m);

    cyc(16);
    fc = (unsigned)a + m > 0xFFFF ? P_C : 0;
    fv = (~(a ^ m) & (a ^ r) & 0x8000) ? P_V : 0;
    fcv_set |= 3;
    return r;
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
uint16_t ObjectDistanceApprox(uint8_t b, uint16_t x, uint16_t y)
{
    uint16_t a, m, t;

    a = rd16(cyc_then(52, f(b, y, 0x0C)));
    m = rd16(cyc_then(44, f(b, x, 0x0C)));
    a = abs16(sub16(a, m));
    cyc(32);
    wr16(0x000002, a);
    a = rd16(cyc_then(30, f(b, y, 0x10)));
    m = rd16(cyc_then(44, f(b, x, 0x10)));
    a = abs16(sub16(a, m));
    cyc(32);
    wr16(0x000008, a);
    cyc(38);
    a = ld16(rd16(0x000002));
    wr16(0x000002, cyc_then(38, asr1(a)));
    cyc(16);
    a = ld16(rd16(0x000008));
    wr16(0x000008, cyc_then(38, asr1(a)));
    cyc(16);
    a = ld16(rd16(0x000008));
    cyc(14);
    m = rd16(0x000002);
    a = add16(a, m);
    a = ld16((uint16_t)(a << 1));
    wr16(w(b, DIST), cyc_then(6, a));
    cyc(16);
    a = ld16(rd16(0x000008));
    m = rd16(0x000002);
    cmp16(a, m);
    if ((uint16_t)(a - m) & 0x8000)
        { cyc(16); a = ld16(rd16(0x000002)); }
    else
        { cyc(10); a = ld16(cyc_then(22, rd16(0x000008))); }
    m = rd16(cyc_then(22, w(b, DIST)));
    a = add16(a, m);
    wr16(0x000002, a);
    a = (uint16_t)(a << 1);
    a = ld16((uint16_t)(a << 1));
    cyc(28);
    wr16(0x000008, a);
    cyc(16);
    a = ld16(rd16(0x000002));
    a = asr1(a);
    cyc(52);
    m = rd16(0x000008);
    a = add16(a, m);
    a = asr1(a);
    a = asr1(a);
    t = asr1(a);
    wr16(w(b, DIST), cyc_then(122, t));
    cyc(66);                            /* SEP, RTL */
    return t;
}

/* The distances' exit: A the result, M 8-bit, the flags of its last step. */
static void distance_exit(uint16_t a)
{
    cpu.a = a;
    cpu.p = (uint8_t)(find_flags(cpu.p, a) | P_M);
}

void abi_ObjectDistanceApprox(void)
{
    fcv_set = 0;
    distance_exit(ObjectDistanceApprox(cpu.db, cpu.x, cpu.y));
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
uint16_t ObjectDistanceXY(uint8_t b, uint16_t x, uint16_t y)
{
    uint16_t a, m;

    a = rd16(cyc_then(52, f(b, y, 0x0C)));
    m = rd16(cyc_then(44, f(b, x, 0x0C)));
    a = sub16(a, m);
    cyc(10);
    wr16(0x000002, a);
    if (a & 0x8000) {
        cyc(70);
        m = rd16(0x000002);
        a = sub16(0x0000, m);
    } else { cyc(28); }
    wr16(w(b, 0x1A62), cyc_then(18, a));
    a = rd16(cyc_then(30, f(b, y, 0x0E)));
    m = rd16(cyc_then(44, f(b, x, 0x0E)));
    a = sub16(a, m);
    cyc(10);
    wr16(0x000008, a);
    if (!(a & 0x8000)) {
        m = rd16(cyc_then(54, w(b, 0x1A62)));
        cyc(12);
        a = add16(a, m);
    } else {
        a = rd16(cyc_then(60, w(b, 0x1A62)));
        cyc(16);
        m = rd16(0x000008);
        a = sub16(a, m);
    }
    wr16(w(b, 0x1A62), cyc_then(18, a));
    cyc(66);                            /* SEP, RTL */
    return a;
}

void abi_ObjectDistanceXY(void)
{
    distance_exit(ObjectDistanceXY(cpu.db, cpu.x, cpu.y));
}

uint16_t Gsu81E3Long(void);

/* |a| as SEC; LDA #0; SBC when negative (BPL skips it). */
static uint16_t neg_if(uint16_t a, uint16_t at)
{
    if (a & 0x8000) {
        cyc(70);
        uint16_t m = rd16(at);

        a = sub16(0x0000, m);
    } else { cyc(28); }
    return a;
}

/* dz (already stored at at) folded into $0364: added when not negative,
   else subtracted from it. */
static uint16_t fold(uint8_t b, uint16_t a, uint16_t at)
{
    uint16_t m;

    if (!(a & 0x8000)) {
        m = rd16(cyc_then(54, w(b, DIST)));
        cyc(12);
        return add16(a, m);
    }
    a = rd16(cyc_then(60, w(b, DIST)));
    cyc(16);
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
uint16_t ObjectOffsetDistanceXZ(uint8_t b, uint16_t x, uint16_t y)
{
    uint16_t a, m;

    a = rd16(cyc_then(52, f(b, y, 0x0C)));
    cyc(30);
    m = rd16(0x000004);
    a = add16(a, m);
    m = rd16(cyc_then(28, f(b, x, 0x0C)));
    a = sub16(a, m);
    cyc(10);
    wr16(0x000002, a);
    a = neg_if(a, 0x0002);
    wr16(w(b, DIST), cyc_then(18, a));
    a = rd16(cyc_then(30, f(b, y, 0x10)));
    m = rd16(cyc_then(38, w(b, 0x05F2)));
    a = add16(a, m);
    m = rd16(cyc_then(28, f(b, x, 0x10)));
    a = sub16(a, m);
    cyc(10);
    wr16(0x000008, a);
    a = fold(b, a, 0x0008);
    wr16(w(b, DIST), cyc_then(18, a));
    cyc(66);                            /* SEP, RTL */
    return a;
}

void abi_ObjectOffsetDistanceXZ(void)
{
    distance_exit(ObjectOffsetDistanceXZ(cpu.db, cpu.x, cpu.y));
}

/* ObjectPointDistanceXZ and ObjectPointDistanceXY: from object X to the
   point (direct page $04, the word at second, compared with field2). */
static uint16_t point_distance(uint8_t b, uint16_t x, uint32_t second, uint16_t field2)
{
    uint16_t a, m;

    cyc(38);
    a = rd16(0x000004);
    m = rd16(cyc_then(44, f(b, x, 0x0C)));
    a = sub16(a, m);
    cyc(10);
    wr16(0x000002, a);
    a = neg_if(a, 0x0002);
    wr16(w(b, DIST), cyc_then(18, a));
    cyc(second > 0xFFFF ? 24 : 16);     /* LDA abs or dp */
    a = rd16(second);
    m = rd16(cyc_then(44, f(b, x, field2)));
    a = sub16(a, m);
    cyc(10);
    wr16(0x000008, a);
    a = fold(b, a, 0x0008);
    wr16(w(b, DIST), cyc_then(18, a));
    cyc(66);                            /* SEP, RTL */
    return a;
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
uint16_t ObjectPointDistanceXZ(uint8_t b, uint16_t x)
{
    return point_distance(b, x, w(b, 0x05F2), 0x10);
}

void abi_ObjectPointDistanceXZ(void)
{
    distance_exit(ObjectPointDistanceXZ(cpu.db, cpu.x));
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
uint16_t ObjectPointDistanceXY(uint8_t b, uint16_t x)
{
    return point_distance(b, x, 0x00000A, 0x0E);
}

void abi_ObjectPointDistanceXY(void)
{
    distance_exit(ObjectPointDistanceXY(cpu.db, cpu.x));
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
uint16_t ObjectPointGsu81E3(uint8_t b, uint16_t x)
{
    uint16_t a;

    (cyc(122), ObjectPointDistanceXZ(b, x));
    a = (cyc(62), Gsu81E3Long());
    cyc(226);                           /* XBA, AND, SEP, REP, PLY, PLX, RTL */
    return (uint16_t)(a >> 8);
}

void abi_ObjectPointGsu81E3(void)
{
    cpu.a = ObjectPointGsu81E3(cpu.db, cpu.x);
    cpu.p = (uint8_t)((find_flags(cpu.p, cpu.x) | P_M) & ~P_X);
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
uint16_t ObjectVelocityAddFrom(uint8_t b, uint16_t x, uint16_t y)
{
    uint16_t a = 0, m, i;
    static const uint16_t dp[3] = {0x02, 0x08, 0x94};

    cyc(38);
    wr16(0x00003E, x);
    cyc(46);
    for (i = 0; i < 3; i++) {
        a = rd16(cyc_then(30, f(b, y, (uint16_t)(0x2E + 2 * i))));
        cyc(16);
        wr16(dp[i], a);
    }
    cyc(16);
    wr16(0x00003E, y);
    cyc(46);
    for (i = 0; i < 3; i++) {
        a = rd16(cyc_then(30, f(b, x, (uint16_t)(0x2E + 2 * i))));
        cyc(30);
        m = rd16(dp[i]);
        a = add16(a, m);
        wr16(f(b, x, (uint16_t)(0x2E + 2 * i)), cyc_then(14, a));
    }
    cyc(66);
    return a;
}

void abi_ObjectVelocityAddFrom(void)
{
    distance_exit(ObjectVelocityAddFrom(cpu.db, cpu.x, cpu.y));
}

/* The Gsu81E3Long wrappers: X and Y pushed and pulled, 16-bit after. */
static uint16_t gsu_pair_end(void)
{
    uint16_t a = (cyc(62), Gsu81E3Long());

    cyc(138);                           /* PLY, PLX, RTL */
    return a;
}

/* The pair routines' exit: M and X 16-bit; N, Z from X. */
static void pair_exit(uint16_t a)
{
    cpu.a = a;
    cpu.p = find_flags((uint8_t)(cpu.p & ~(P_M | P_X)), cpu.x);
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
uint16_t ObjectPairGsu81E3(uint8_t b, uint16_t x, uint16_t y)
{
    uint16_t a, m;

    a = rd16(cyc_then(90, f(b, y, 0x0C)));
    m = rd16(cyc_then(44, f(b, x, 0x0C)));
    a = sub16(a, m);
    cyc(10);
    wr16(0x000002, a);
    a = rd16(cyc_then(30, f(b, y, 0x10)));
    m = rd16(cyc_then(44, f(b, x, 0x10)));
    a = sub16(a, m);
    cyc(10);
    wr16(0x000008, a);
    return gsu_pair_end();
}

void abi_ObjectPairGsu81E3(void)
{
    fcv_set = 0;
    pair_exit(ObjectPairGsu81E3(cpu.db, cpu.x, cpu.y));
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
uint16_t ObjectPairPitchGsu81E3(uint8_t b, uint16_t x, uint16_t y)
{
    uint16_t a, m;

    (cyc(122), ObjectDistanceApprox(b, x, y));
    a = rd16(cyc_then(46, w(b, DIST)));
    cyc(16);
    wr16(0x000008, a);
    a = rd16(cyc_then(30, f(b, y, 0x0E)));
    m = rd16(cyc_then(44, f(b, x, 0x0E)));
    a = sub16(a, m);
    cyc(10);
    wr16(0x000002, a);
    return gsu_pair_end();
}

void abi_ObjectPairPitchGsu81E3(void)
{
    pair_exit(ObjectPairPitchGsu81E3(cpu.db, cpu.x, cpu.y));
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
uint16_t ObjectPairOffsetGsu81E3(uint8_t b, uint16_t x, uint16_t y)
{
    uint16_t a, m;

    a = rd16(cyc_then(90, f(b, y, 0x0C)));
    m = rd16(cyc_then(44, f(b, x, 0x0C)));
    a = sub16(a, m);
    cyc(10);
    wr16(0x000002, a);
    a = rd16(cyc_then(30, f(b, y, 0x10)));
    m = rd16(cyc_then(44, f(b, x, 0x10)));
    a = sub16(a, m);
    cyc(10);
    wr16(0x000008, a);
    cyc(38);
    a = rd16(0x000002);
    cyc(30);
    m = rd16(0x000004);
    a = add16(a, m);
    wr16(0x000002, a);
    cyc(16);
    a = rd16(0x000008);
    m = rd16(cyc_then(38, w(b, 0x05F2)));
    a = add16(a, m);
    wr16(0x000008, a);
    return gsu_pair_end();
}

void abi_ObjectPairOffsetGsu81E3(void)
{
    pair_exit(ObjectPairOffsetGsu81E3(cpu.db, cpu.x, cpu.y));
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
uint16_t ObjectPairOffsetPitchGsu81E3(uint8_t b, uint16_t x, uint16_t y)
{
    uint16_t a, m;

    (cyc(122), ObjectOffsetDistanceXZ(b, x, y));
    a = rd16(cyc_then(46, w(b, DIST)));
    cyc(16);
    wr16(0x000008, a);
    a = rd16(cyc_then(30, f(b, y, 0x0E)));
    m = rd16(cyc_then(44, f(b, x, 0x0E)));
    a = sub16(a, m);
    cyc(24);
    m = rd16(0x00000A);
    a = sub16(a, m);
    cyc(10);
    wr16(0x000002, a);
    return gsu_pair_end();
}

void abi_ObjectPairOffsetPitchGsu81E3(void)
{
    pair_exit(ObjectPairOffsetPitchGsu81E3(cpu.db, cpu.x, cpu.y));
}
