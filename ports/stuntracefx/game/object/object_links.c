/*
 * object_links.c: object helpers of bank $03 that walk the links between
 * object records ($62-byte records from $7E:2000, addressed by their offset
 * in X or Y): the list whose head is at $0325 (next at +$00), and the chain
 * of children through +$27, each child tagged at +$48 with its parent at
 * +$06. No play runs them; unit_objects_03 calls them.
 */
#include "machine.h"
#include "cpu_ops.h"
#include "carried.h"


void abi_ObjectFreeLong(void);
void ObjectInit(uint16_t obj);
int ObjectAlloc(uint16_t *a, uint16_t *x, uint16_t *y);

#define LINKS_SCRATCH 0x00003Eu

void ObjectChildFindTagX_cr(Carried *cr);
void ObjectChildFindTagY_cr(Carried *cr);
void ObjectLinkChild_cr(Carried *cr);
void ObjectListFreeAll_cr(Carried *cr);
void ObjectListFreeAllCopy_cr(Carried *cr);
void ObjectSpawnA040_cr(Carried *cr);
void ObjectListFreeAll(Carried *cr);
void ObjectListFreeAllCopy(Carried *cr);
void ObjectListTableFirst(Carried *cr);
static void free_list(Carried *cr);
void ObjectFreeLong_cr(Carried *cr);

/* ObjectListFreeAll and its copy: free each object of the list at $0325
   in turn, reading the next link before the free rewrites it. */
static void free_list(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;
    uint16_t x = cr->x, obj, next;
    int first = 1;

    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_X)));
    cyc(76);
    obj = rd16(b | 0x0325);
    while (obj) {
        cyc(first ? 46 : 102);  /* later ones: PLY, TYX, BNE taken */
        first = 0;
        next = rd16(b | (uint16_t)(0x2000 + obj));
        set_p(cr, (uint8_t)(cr->p | (P_M)));
        cr->x = obj;
        cr->y = next;
        (cyc(114), ObjectFreeLong_cr(cr));
        cr->y = next;
        obj = next;
    }
    cyc(first ? 102 : 146);
    cr->x = x;
    set_p(cr, nz16(cr->p, x));
}

/*
 * @routine 03:B8E7 ObjectListTableFirst
 * WHAT IT DOES: Clears the count at $0D04 and walks the list at $0325 to
 * its end. Only for its first object, and only when A's low byte is not 0
 * and the object's +$08 has bit 0 clear, it writes a 5-word entry at
 * $7E:96AD: the object, then the words at +$08, +$0A, +$0C and +$0E of the
 * record its +$04 points to, and counts it in $0D04.
 * HOW: 16-bit, P saved and restored. The test that skips an object is a
 * BNE on the flags of the instruction before the loop's PHX: LDY #0 for
 * the first object, the TAX of its next link for the others (never 0
 * there), so the table gets one entry at most. Through the caller's data
 * bank.
 * Entry: any widths; A = the flag tested. Exit: P unchanged; X = 0; list
 * not empty: A = 0, Y = 10 when an entry was written, else 0; empty: A, Y
 * unchanged.
 */
void ObjectListTableFirst(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;
    uint8_t p = cr->p;
    uint16_t x, a = cr->a, at, v, t = 132;

    cyc(68);
    wr16(b | 0x0D04, 0x0000);
    cyc(24);
    x = rd16(b | 0x0325);
    if (x) {
        cr->y = 0x0000;
        if (a & 0x00FF) {
            cyc(156);
            a = rd16(b | (uint16_t)(0x2008 + x));
            if (!(a & 0x0001)) {
                cyc(100);
                at = rd16(b | (uint16_t)(0x2004 + x));
                cyc(96);
                wr16(0x7E96AD, x);
                cyc(30);
                v = rd16(b | (uint16_t)(0x0008 + at));
                cyc(32);
                wr16(0x7E96AF, v);
                cyc(30);
                v = rd16(b | (uint16_t)(0x000A + at));
                cyc(32);
                wr16(0x7E96B1, v);
                cyc(30);
                v = rd16(b | (uint16_t)(0x000C + at));
                cyc(32);
                wr16(0x7E96B3, v);
                cyc(30);
                v = rd16(b | (uint16_t)(0x000E + at));
                cyc(32);
                wr16(0x7E96B5, v);
                cyc(24);
                v = rd16(b | 0x0D04);
                cyc(14);
                wr16(b | 0x0D04, (uint16_t)(v + 1));
                cr->y = 0x000A;
            } else { t = 112; /* BIT, BNE taken */ }
        } else { cyc(66); }
        do {
            cyc(t);
            x = rd16(b | (uint16_t)(0x2000 + x));
            t = 154; /* later objects: BNE taken at the top */
        } while (x);
        cyc(8);
        cr->a = 0x0000;
    }
    cyc(52);
    cr->x = 0x0000;
    r_plp(cr, p);
}

void abi_ObjectListTableFirst(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectListTableFirst(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:CBB0 ObjectListFreeAll
 * WHAT IT DOES: Frees every object of the list whose head is at $0325,
 * following each one's +$00 link (ObjectFreeLong on each).
 * HOW: X 16-bit from the start; X saved and restored; the next link is
 * kept in Y across each free. Through the caller's data bank.
 * Entry: any widths. Exit: X unchanged, X 16-bit; N, Z from X (16-bit); if
 * the list was not empty, M 8-bit, Y = 0 and A from the last free (the free
 * list's old head); otherwise A, Y and M unchanged.
 */
void ObjectListFreeAll(Carried *cr)
{
    free_list(cr);
}

void ObjectListFreeAll_cr(Carried *cr)
{
    free_list(cr);
}

void abi_ObjectListFreeAll(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectListFreeAll_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:CBC8 ObjectListFreeAllCopy
 * WHAT IT DOES: The same bytes as ObjectListFreeAll, at another address.
 * HOW: As ObjectListFreeAll.
 * Entry: as ObjectListFreeAll. Exit: as ObjectListFreeAll.
 */
void ObjectListFreeAllCopy(Carried *cr)
{
    free_list(cr);
}

void ObjectListFreeAllCopy_cr(Carried *cr)
{
    free_list(cr);
}

void abi_ObjectListFreeAllCopy(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectListFreeAllCopy_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:CCEB ObjectLinkChild
 * WHAT IT DOES: Makes object Y a child of object X tagged A: stores the tag
 * at $1F2B and at Y's +$48, X at Y's +$06, clears Y's +$27, then puts Y at
 * the end of the +$27 chain that starts at X.
 * HOW: Y kept in direct page $3E while the chain is walked (16-bit) to the
 * record whose +$27 is 0. Through the caller's data bank.
 * Entry: M 8-bit, X 16-bit, D = $0000; A = tag, X = parent, Y = child.
 * Exit: A = Y = the child (A 16 bits wide in M 8-bit); X unchanged; M 8-bit;
 * N, Z from Y.
 */
void ObjectLinkChild(uint8_t bank, uint8_t tag, uint16_t parent, uint16_t child)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t at = parent, next;

    cyc(24);
    wr8(b | 0x1F2B, tag);
    cyc(24);
    tag = rd8(b | 0x1F2B);
    cyc(30);
    wr8(b | (uint16_t)(0x2048 + child), tag);
    cyc(88);
    wr16(b | (uint16_t)(0x2006 + child), parent);
    cyc(126);
    wr16(b | (uint16_t)(0x2027 + child), 0x0000);
    cyc(60);
    wr16(LINKS_SCRATCH, child);
    for (;;) {
        cyc(58);
        next = rd16(b | (uint16_t)(0x2027 + at));
        if (!next)
            break; else { cyc(8); }
        at = next;
    }
    cyc(32);
    child = rd16(LINKS_SCRATCH);
    cyc(30);
    wr16(b | (uint16_t)(0x2027 + at), child);
    cyc(16);
    (void)rd16(LINKS_SCRATCH);
    cyc(66);                            /* SEP, RTL */
}

void ObjectLinkChild_cr(Carried *cr)
{
    ObjectLinkChild(cr->db, (uint8_t)cr->a, cr->x, cr->y);
    cr->a = cr->y;
    set_p(cr, (uint8_t)(nz16(cr->p, cr->y) | P_M));
}

void abi_ObjectLinkChild(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectLinkChild_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* The two chain searches: from `from`, follow +$27 until a child whose
   +$48 is the tag (stored first at $1A30) or the end of the chain. The
   flags are those of the last compare, then N, Z from the pulled index. */
static uint16_t find_tag(uint8_t bank, uint8_t tag, uint16_t from, uint8_t *a, uint8_t *p)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t at = from, next;
    uint8_t t, want;

    cyc(24);
    wr8(b | 0x1A30, tag);
    for (;;) {
        cyc(60);
        next = rd16(b | (uint16_t)(0x2027 + at));
        if (!next)
            { cyc(102); return 0; }
        cyc(46);
        t = rd8(b | (uint16_t)(0x2048 + next));
        *a = t;
        cyc(24);
        want = rd8(b | 0x1A30);
        *p = (uint8_t)((*p & ~P_C) | (t >= want ? (cyc(102), P_C) : (cyc(22), 0)));
        if (t == want)
            return next;
        at = next;
    }
}

/*
 * @routine 03:D2DA ObjectChildFindTagX
 * WHAT IT DOES: Finds the child of object X tagged A: Y = the first record
 * along X's +$27 chain whose +$48 is A, or 0 when there is none.
 * HOW: The tag kept at $1A30; X saved, walked along the chain, restored.
 * Through the caller's data bank.
 * Entry: M 8-bit, X 16-bit; A = tag, X = object. Exit: Y = the child or 0;
 * X unchanged; A = the last tag read (unchanged if none); C from the last
 * compare; N, Z from X.
 */
uint16_t ObjectChildFindTagX(uint8_t bank, uint8_t tag, uint16_t obj, uint8_t *a, uint8_t *p)
{
    return find_tag(bank, tag, obj, a, p);
}

void ObjectChildFindTagX_cr(Carried *cr)
{
    uint8_t a = (uint8_t)cr->a, p = cr->p;

    cr->y = ObjectChildFindTagX(cr->db, (uint8_t)cr->a, cr->x, &a, &p);
    cr->a = (uint16_t)((cr->a & 0xFF00) | a);
    set_p(cr, nz16(p, cr->x));
}

void abi_ObjectChildFindTagX(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectChildFindTagX_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:D2F0 ObjectChildFindTagY
 * WHAT IT DOES: ObjectChildFindTagX with X and Y the other way: X = the
 * child of object Y tagged A, or 0.
 * HOW: As ObjectChildFindTagX, Y saved and restored.
 * Entry: M 8-bit, X 16-bit; A = tag, Y = object. Exit: X = the child or 0;
 * Y unchanged; A, C as ObjectChildFindTagX; N, Z from Y.
 */
uint16_t ObjectChildFindTagY(uint8_t bank, uint8_t tag, uint16_t obj, uint8_t *a, uint8_t *p)
{
    return find_tag(bank, tag, obj, a, p);
}

void ObjectChildFindTagY_cr(Carried *cr)
{
    uint8_t a = (uint8_t)cr->a, p = cr->p;

    cr->x = ObjectChildFindTagY(cr->db, (uint8_t)cr->a, cr->y, &a, &p);
    cr->a = (uint16_t)((cr->a & 0xFF00) | a);
    set_p(cr, nz16(p, cr->y));
}

void abi_ObjectChildFindTagY(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectChildFindTagY_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:D365 ObjectSpawnA040
 * WHAT IT DOES: Takes a new object (ObjectAlloc) and clears it (ObjectInit),
 * then sets its +$36 to 3 and its +$04 (its handler) to $A040. Carry set
 * and X = Y = the object when it got one; carry clear and X = 0 otherwise.
 * HOW: As ObjectSpawn without the count limit, the fields written with
 * long stores into bank $7E.
 * Entry: X 16-bit. Exit: got one: A = $A040, M 16-bit, N set, Z clear;
 * none: X = 0, N clear, Z set, A, Y and M as ObjectAlloc leaves them.
 */
int ObjectSpawnA040(uint16_t *a, uint16_t *x, uint16_t *y)
{
    cyc(62);
    if (!ObjectAlloc(a, x, y)) {
        cyc(68);
        *x = 0x0000;
        return 0;
    } else { cyc(140); }
    *y = *x;
    ObjectInit(*y);
    cyc(48);
    wr8(0x7E2036 + *x, 0x03);
    *a = 0xA040;
    cyc(78);
    wr16(0x7E2004 + *x, *a);
    return 1;
}

void ObjectSpawnA040_cr(Carried *cr)
{
    if (ObjectSpawnA040(&cr->a, &cr->x, &cr->y))
        set_p(cr, (uint8_t)((nz16(cr->p, cr->a) & ~P_M) | P_C));
    else
        set_p(cr, (uint8_t)(nz16(cr->p, 0) & ~P_C));
    cyc(58);
}

void abi_ObjectSpawnA040(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSpawnA040_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

