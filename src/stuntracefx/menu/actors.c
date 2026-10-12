/*
 * actors.c: the menu screens' objects. 22 records of $2C bytes from $1265
 * ($1265-$162C), each run once a frame through the handler address at +$08.
 * Free records are kept on a stack of addresses at $120D (depth $11F5, in
 * bytes); live ones in the list at $1239 (length $11F7, in bytes), which the
 * dispatcher walks from the end. Other fields used here: +$10 (set to $FFFF
 * when a record is made free), +$12 (an index into the sprite records at
 * $7F:135A), +$1A (the object's kind) and +$24 (a pending palette).
 *
 * The records are reached as direct page offsets indexed by the record
 * address (D = $0000); the lists through the data bank.
 */
#include "machine.h"

#define ACTOR_FIRST 0x1265u
#define ACTOR_SIZE 0x2Cu
#define ACTOR_END 0x162Du
#define FREE_STACK 0x120Du
#define FREE_DEPTH 0x11F5u
#define LIVE_LIST 0x1239u
#define LIVE_LEN 0x11F7u
#define CURRENT 0x11F9u

static uint32_t dbw(uint8_t bank, uint16_t a)
{
    return (uint32_t)bank << 16 | a;
}

/*
 * @routine 0B:B479 MenuActorPoolInit
 * WHAT IT DOES: Empties the menu object pool and sets the menu's sprite
 * defaults.
 * HOW: Pushes all 22 record addresses on the free stack, giving each the
 * idle handler $D8A8 and +$10 = $FFFF; clears both lengths; stores $0200,
 * $0020 and $2881 at $1201, $1203 and $120B; sets OBSEL to 2.
 * Entry: D = $0000. Exit: A = $2802; X = $162D; Y = $2C; P unchanged.
 */
void MenuActorPoolInit(uint8_t bank)
{
    uint16_t x, y = 0;

    for (x = ACTOR_FIRST; x != ACTOR_END; x = (uint16_t)(x + ACTOR_SIZE), y = (uint16_t)(y + 2)) {
        wr16(dbw(bank, (uint16_t)(FREE_STACK + y)), x);
        wr16((uint16_t)(x + 0x08), 0xD8A8);
        wr16((uint16_t)(x + 0x10), 0xFFFF);
    }
    wr16(dbw(bank, FREE_DEPTH), 0x0000);
    wr16(dbw(bank, LIVE_LEN), 0x0000);
    wr16(dbw(bank, 0x1201), 0x0200);
    wr16(dbw(bank, 0x1203), 0x0020);
    wr16(dbw(bank, 0x120B), 0x2881);
    wr8(dbw(bank, 0x2101), 0x02);
}

void abi_MenuActorPoolInit(void)
{
    MenuActorPoolInit(cpu.db);
    cpu.a = 0x2802;
    cpu.x = 0x162D;
    cpu.y = 0x002C;
}

/*
 * @routine 0B:B4C3 MenuActorAlloc
 * WHAT IT DOES: Takes a record from the free stack and adds it to the live
 * list; X = $FFFF when none is free.
 * HOW: Pops the next address at $120D + depth and appends it to $1239.
 * Entry: M 16-bit, X 16-bit. Exit: A = X = the record; N, Z from it; C
 * clear. When none is free: X = $FFFF, A unchanged; N, C set, Z clear.
 */
uint16_t MenuActorAlloc(uint8_t bank, uint8_t *p)
{
    uint16_t depth = rd16(dbw(bank, FREE_DEPTH)), rec, len;

    if (depth == ACTOR_SIZE) {
        *p = (uint8_t)((*p & ~P_Z) | P_N | P_C);
        return 0xFFFF;
    }
    *p = (uint8_t)(*p & ~P_C);
    rec = rd16(dbw(bank, (uint16_t)(FREE_STACK + depth)));
    wr16(dbw(bank, FREE_DEPTH), (uint16_t)(depth + 2));
    len = rd16(dbw(bank, LIVE_LEN));
    wr16(dbw(bank, (uint16_t)(LIVE_LIST + len)), rec);
    wr16(dbw(bank, LIVE_LEN), (uint16_t)(len + 1));
    wr16(dbw(bank, LIVE_LEN), (uint16_t)(len + 2));
    *p = nz16(*p, rec);
    return rec;
}

void abi_MenuActorAlloc(void)
{
    uint8_t p = cpu.p;
    uint16_t rec = MenuActorAlloc(cpu.db, &p);

    if (rec != 0xFFFF)
        cpu.a = rec;
    cpu.x = rec;
    cpu.p = p;
}

/*
 * @routine 0B:B4E5 MenuActorFree
 * WHAT IT DOES: Returns record X to the free stack and removes it from the
 * live list, whose entry Y it occupied, by moving the last entry there.
 * HOW: Push on $120D; shorten $1239 by one and copy its last entry to Y;
 * $11F9 (the record being run) = $FFFF.
 * Entry: M 16-bit, X 16-bit; X = record, Y = its live list offset. Exit: A =
 * the entry moved; X = $FFFF; N set, Z clear.
 */
uint16_t MenuActorFree(uint8_t bank, uint16_t rec, uint16_t slot)
{
    uint16_t depth = (uint16_t)(rd16(dbw(bank, FREE_DEPTH)) - 2), len, moved;

    wr16(dbw(bank, FREE_DEPTH), depth);
    wr16(dbw(bank, (uint16_t)(FREE_STACK + depth)), rec);
    len = (uint16_t)(rd16(dbw(bank, LIVE_LEN)) - 2);
    wr16(dbw(bank, LIVE_LEN), len);
    moved = rd16(dbw(bank, (uint16_t)(LIVE_LIST + len)));
    wr16(dbw(bank, (uint16_t)(LIVE_LIST + slot)), moved);
    wr16(dbw(bank, CURRENT), 0xFFFF);
    return moved;
}

void abi_MenuActorFree(void)
{
    cpu.a = MenuActorFree(cpu.db, cpu.x, cpu.y);
    cpu.x = 0xFFFF;
    cpu.p = (uint8_t)((cpu.p & ~P_Z) | P_N);
}

/* Compare as CMP does: C when a >= b; N, Z from a - b. */
static uint8_t cmp16(uint8_t p, uint16_t a, uint16_t b)
{
    return (uint8_t)((nz16(p, (uint16_t)(a - b)) & ~P_C) | (a >= b ? P_C : 0));
}

/*
 * @routine 0B:E28D MenuActorFind
 * WHAT IT DOES: Finds the live object of kind A (+$1A), searching from the
 * end of the list.
 * HOW: Compares each record's +$1A with A (kept at $11E1).
 * Entry: M 16-bit, X 16-bit, D = $0000; A = kind. Exit: found: X = the
 * record, Y = its list offset, Z and C set. Not found: Y = $FFFE, N set, Z
 * clear, C from the last compare.
 */
int MenuActorFind(uint8_t bank, uint16_t kind, uint16_t *x, uint16_t *y, uint8_t *p)
{
    uint16_t i = (uint16_t)(rd16(dbw(bank, LIVE_LEN)) - 2);

    wr16(dbw(bank, 0x11E1), kind);
    while (!(i & 0x8000)) {
        *x = rd16(dbw(bank, (uint16_t)(LIVE_LIST + i)));
        *p = cmp16(*p, rd16((uint16_t)(*x + 0x1A)), rd16(dbw(bank, 0x11E1)));
        if (*p & P_Z) {
            *y = i;
            return 1;
        }
        i = (uint16_t)(i - 2);
    }
    *y = i;
    *p = nz16(*p, i);
    return 0;
}

void abi_MenuActorFind(void)
{
    uint8_t p = cpu.p;

    MenuActorFind(cpu.db, cpu.a, &cpu.x, &cpu.y, &p);
    cpu.p = p;
}

/*
 * @routine 0B:C9A1 MenuActorsSet10
 * WHAT IT DOES: Sets field +$10 to Y in every live object of kind A.
 * HOW: Y and A are kept at $11DF and $11E1; the list is walked from the end.
 * Entry: M 16-bit, X 16-bit, D = $0000; A = kind, Y = value. Exit: A = the
 * last value loaded; X unchanged; Y = $FFFE; N, Z from X; C from the last
 * compare.
 */
uint16_t MenuActorsSet10(uint8_t bank, uint16_t kind, uint16_t value, uint8_t *p)
{
    uint16_t i, rec, a = kind;

    wr16(dbw(bank, 0x11DF), value);
    wr16(dbw(bank, 0x11E1), kind);
    for (i = (uint16_t)(rd16(dbw(bank, LIVE_LEN)) - 2); !(i & 0x8000); i = (uint16_t)(i - 2)) {
        rec = rd16(dbw(bank, (uint16_t)(LIVE_LIST + i)));
        a = rd16((uint16_t)(rec + 0x1A));
        *p = cmp16(*p, a, rd16(dbw(bank, 0x11E1)));
        if (*p & P_Z) {
            a = rd16(dbw(bank, 0x11DF));
            wr16((uint16_t)(rec + 0x10), a);
        }
    }
    return a;
}

void abi_MenuActorsSet10(void)
{
    uint8_t p = cpu.p;

    cpu.a = MenuActorsSet10(cpu.db, cpu.a, cpu.y, &p);
    cpu.y = 0xFFFE;
    cpu.p = nz16(p, cpu.x);
}

/*
 * @routine 0B:CAB9 MenuActorApplyPalette
 * WHAT IT DOES: Applies object X's pending palette (+$24) to its sprites and
 * clears it.
 * HOW: The object's sprite group starts at $7F:1356 + its +$12: a count
 * (entries - 1), then 8-byte entries from $7F:135A whose attribute word gets
 * bits 9-11 replaced by the palette (kept at $11DF).
 * Entry: M 16-bit, X 16-bit, D = $0000; X = record. Exit: nothing pending: A
 * = 0, Z set. Otherwise A = the last entry offset + 8; Y = $FFFF; X
 * unchanged; N, Z from X; C, V from the last add.
 */
uint16_t MenuActorApplyPalette(uint8_t bank, uint16_t rec, uint16_t *y, uint8_t *p)
{
    uint16_t pal = rd16((uint16_t)(rec + 0x24)), i, n, a = 0;
    unsigned s;

    if (!pal) {
        *p = (uint8_t)((*p & ~P_N) | P_Z);
        return 0;
    }
    wr16(dbw(bank, 0x11DF), pal);
    i = rd16((uint16_t)(rec + 0x12));
    n = rd16(0x7F1356 + i);
    i = (uint16_t)(i + 2);
    do {
        wr16(0x7F135A + i, (uint16_t)((rd16(0x7F135A + i) & 0xF1FF) | rd16(dbw(bank, 0x11DF))));
        s = (unsigned)i + 8;
        *p = (uint8_t)((*p & ~(P_C | P_V)) | (s > 0xFFFF ? P_C : 0) | ((~(i ^ 8) & (i ^ (uint16_t)s) & 0x8000) ? P_V : 0));
        a = i = (uint16_t)s;
        n = (uint16_t)(n - 1);
    } while (!(n & 0x8000));
    *y = n;
    wr16((uint16_t)(rec + 0x24), 0x0000);
    *p = nz16(*p, rec);
    return a;
}

void abi_MenuActorApplyPalette(void)
{
    uint8_t p = cpu.p;
    uint16_t a = MenuActorApplyPalette(cpu.db, cpu.x, &cpu.y, &p);

    cpu.a = a;
    cpu.p = p;
}

/*
 * @routine 0B:C47B MenuActorIdleC47B
 * WHAT IT DOES: A handler that does nothing.
 * HOW: Returns.
 * Entry: any. Exit: nothing changes.
 */
void MenuActorIdleC47B(void)
{
}

void abi_MenuActorIdleC47B(void)
{
}

/*
 * @routine 0B:CE89 MenuActorIdleCE89
 * WHAT IT DOES: A handler that does nothing.
 * HOW: Returns.
 * Entry: any. Exit: nothing changes.
 */
void MenuActorIdleCE89(void)
{
}

void abi_MenuActorIdleCE89(void)
{
}

/*
 * @routine 0B:C576 MenuActorBlink
 * WHAT IT DOES: Every fourth step of the counter at $11DD, flips bit 11 of
 * the attribute word of the sprite entry at $7F:135A + $11E7.
 * HOW: Tests the counter's low two bits; on zero, an EOR with $0800.
 * Entry: M 16-bit, X 16-bit. Exit: A = the counter's low bits or the new
 * attribute word, X = $11E7 when flipped; N, Z from A.
 */
uint16_t MenuActorBlink(uint8_t bank, uint16_t *x)
{
    uint16_t a = rd16((uint32_t)bank << 16 | 0x11DD) & 0x0003;

    if (a)
        return a;
    *x = rd16((uint32_t)bank << 16 | 0x11E7);
    a = (uint16_t)(rd16(0x7F135A + *x) ^ 0x0800);
    wr16(0x7F135A + *x, a);
    return a;
}

void abi_MenuActorBlink(void)
{
    cpu.a = MenuActorBlink(cpu.db, &cpu.x);
    cpu.p = nz16(cpu.p, cpu.a);
}

/*
 * @routine 0B:E250 MenuActorAnimate
 * WHAT IT DOES: Steps object X's sprite animation: when its frame timer
 * (+$18) runs out, moves to the next entry of its animation (+$14 = the
 * animation's offset in the table at $0B:AB60, +$16 = the entry offset),
 * restarting at the first entry when the next one's duration is negative,
 * and sets the sprite group (+$12) and timer from the entry.
 * HOW: Entries are four bytes: a duration word and a sprite group word; the
 * timer is set to duration - 1. No animation when +$14 is negative. The
 * offset is kept at $11DF while it is used.
 * Entry: M 16-bit, X 16-bit, D = $0000; X = the object being run ($11F9).
 * Exit: no animation: A = +$14; N set. Timer running: A = +$14; N, Z from
 * the timer. Stepped: A = the new timer; X = $11F9; N, Z from A; C, V from
 * the entry offset + animation offset.
 */
uint16_t MenuActorAnimate(uint8_t bank, uint16_t rec, uint16_t *x, uint8_t *p)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t anim = rd16((uint16_t)(rec + 0x14)), t, pos, i, d, g;
    unsigned s;

    if (anim & 0x8000) {
        *p = nz16(*p, anim);
        return anim;
    }
    t = (uint16_t)(rd16((uint16_t)(rec + 0x18)) - 1);
    wr16((uint16_t)(rec + 0x18), t);
    if (!(t & 0x8000)) {
        *p = nz16(*p, t);
        return anim;
    }
    wr16(b | 0x11DF, anim);
    pos = (uint16_t)(rd16((uint16_t)(rec + 0x16)) + 4);
    wr16((uint16_t)(rec + 0x16), pos);
    s = (unsigned)pos + anim;
    *p = (uint8_t)((*p & ~(P_C | P_V)) | (s > 0xFFFF ? P_C : 0) | ((~(pos ^ anim) & (pos ^ (uint16_t)s) & 0x8000) ? P_V : 0));
    i = (uint16_t)s;
    d = rd16(0x0BAB60 + i);
    if (d & 0x8000) {
        wr16((uint16_t)(rd16(b | 0x11F9) + 0x16), 0x0000);
        i = rd16(b | 0x11DF);
        d = rd16(0x0BAB60 + i);
    }
    wr16(b | 0x11DF, (uint16_t)(d - 1));
    g = rd16(0x0BAB62 + i);
    *x = rd16(b | 0x11F9);
    wr16((uint16_t)(*x + 0x12), g);
    t = rd16(b | 0x11DF);
    wr16((uint16_t)(*x + 0x18), t);
    *p = nz16(*p, t);
    return t;
}

void abi_MenuActorAnimate(void)
{
    uint8_t p = cpu.p;

    cpu.a = MenuActorAnimate(cpu.db, cpu.x, &cpu.x, &p);
    cpu.p = p;
}

/*
 * @routine 0B:CF9C MenuActorCountdown
 * WHAT IT DOES: Counts object X's +$0A down; when it passes zero, moves the
 * object to handler $CFAB with the count at 0.
 * HOW: A 16-bit memory decrement.
 * Entry: M 16-bit, D = $0000; X = record. Exit: counting: A unchanged; N, Z
 * from the count. Moved on: A = 0; Z set, N clear.
 */
int MenuActorCountdown(uint16_t rec, uint8_t *p)
{
    uint16_t c = (uint16_t)(rd16((uint16_t)(rec + 0x0A)) - 1);

    wr16((uint16_t)(rec + 0x0A), c);
    if (!(c & 0x8000)) {
        *p = nz16(*p, c);
        return 0;
    }
    wr16((uint16_t)(rec + 0x08), 0xCFAB);
    wr16((uint16_t)(rec + 0x0A), 0x0000);
    *p = (uint8_t)((*p & ~P_N) | P_Z);
    return 1;
}

void abi_MenuActorCountdown(void)
{
    uint8_t p = cpu.p;

    if (MenuActorCountdown(cpu.x, &p))
        cpu.a = 0x0000;
    cpu.p = p;
}

/*
 * @routine 0B:C9FD MenuSinePosition
 * WHAT IT DOES: From the angle in object X's +$0A, sets $18BF = (sine of
 * the angle - $40, from the table at $00:96FE) + $81, kept to 8 bits, and
 * $18C1 = the same, but at least $A8.
 * HOW: The table byte is sign-extended, $80 added and the sum kept to 8
 * bits before the + 1.
 * Entry: M 16-bit, X 16-bit, D = $0000; X = record. Exit: A = the value
 * stored at $18C1; X = $11F9; N, Z from X; C = (value before the minimum
 * >= $A8); V from the add of $80.
 */
uint16_t MenuSinePosition(uint8_t bank, uint16_t rec, uint8_t *p)
{
    uint16_t i = (uint16_t)(rd16((uint16_t)(rec + 0x0A)) - 0x40) & 0x00FF, v = rd16(0x0096FE + i) & 0x00FF, s;

    if (v >= 0x80)
        v |= 0xFF00;
    s = (uint16_t)(v + 0x80);
    *p = (uint8_t)((*p & ~(P_C | P_V)) | ((~(v ^ 0x80) & (v ^ s) & 0x8000) ? P_V : 0));
    v = (uint16_t)((s & 0x00FF) + 1);
    wr16(0x0018BF, v);
    if (v >= 0x00A8)
        *p |= P_C;
    else
        v = 0x00A8;
    wr16(0x0018C1, v);
    *p = nz16(*p, rd16((uint32_t)bank << 16 | 0x11F9));
    return v;
}

void abi_MenuSinePosition(void)
{
    uint8_t p = cpu.p;

    cpu.a = MenuSinePosition(cpu.db, cpu.x, &p);
    cpu.x = rd16((uint32_t)cpu.db << 16 | 0x11F9);
    cpu.p = p;
}

/*
 * @routine 0B:C4E4 MenuActorSetTiles
 * WHAT IT DOES: Sets the tile numbers of two sprite entries of object X's
 * group: entry Y (offset in bytes from the group's +$12) gets tile
 * $0B:C528[A], the entry before it that tile + $10.
 * HOW: Each attribute word keeps bits 9-15 and takes the tile in bits 0-8;
 * the entry's offset is kept at $11E7 and the tile at $11DF.
 * Entry: M 16-bit, X 16-bit, D = $0000; X = record, A = tile index, Y =
 * entry offset. Exit: A = the second word written; X unchanged; N, Z from
 * X; C, V from the step back of 8.
 */
uint16_t MenuActorSetTiles(uint8_t bank, uint16_t rec, uint16_t i, uint16_t off, uint8_t *p)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t e = (uint16_t)(off + rd16((uint16_t)(rec + 0x12))), t, w, r;

    wr16(b | 0x11E7, e);
    wr16(b | 0x11DF, rd16(0x0BC528 + (uint16_t)(i << 1)));
    e = rd16(b | 0x11E7);
    wr16(0x7F135A + e, (uint16_t)((rd16(0x7F135A + e) & 0xFE00) | rd16(b | 0x11DF)));
    t = (uint16_t)(rd16(b | 0x11DF) + 0x10);
    wr16(b | 0x11DF, t);
    r = (uint16_t)(e - 8);
    *p = (uint8_t)((*p & ~(P_C | P_V)) | (e >= 8 ? P_C : 0) | (((e ^ 8) & (e ^ r) & 0x8000) ? P_V : 0));
    w = (uint16_t)((rd16(0x7F135A + r) & 0xFE00) | rd16(b | 0x11DF));
    wr16(0x7F135A + r, w);
    *p = nz16(*p, rec);
    return w;
}

void abi_MenuActorSetTiles(void)
{
    uint8_t p = cpu.p;

    cpu.a = MenuActorSetTiles(cpu.db, cpu.x, cpu.a, cpu.y, &p);
    cpu.p = p;
}
