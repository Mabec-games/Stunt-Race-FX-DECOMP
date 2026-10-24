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
#include "native.h"
#include "hw.h"
#include "cpu_ops.h"
#include "wram.h"
#include "menu_calls.h"
#include "carried.h"


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

    cyc(0);
    for (x = ACTOR_FIRST; x != ACTOR_END; x = (uint16_t)(x + ACTOR_SIZE), y = (uint16_t)(y + 2)) {
        wr16(dbw(bank, (uint16_t)(FREE_STACK + y)), cyc_then(136, x));
        cyc(46);
        wr16((uint16_t)(x + 0x08), 0xD8A8);
        cyc(46);
        wr16((uint16_t)(x + 0x10), 0xFFFF);
        cyc(48);
    }
    wr16(dbw(bank, FREE_DEPTH), cyc_then(134, 0x0000));
    wr16(dbw(bank, LIVE_LEN), cyc_then(24, 0x0000));
    wr16(dbw(bank, 0x1201), cyc_then(48, 0x0200));
    wr16(dbw(bank, 0x1203), cyc_then(48, 0x0020));
    wr16(dbw(bank, 0x120B), cyc_then(48, 0x2881));
    ppu_wr(dbw(bank, 0x2101), cyc_then(62, 0x02));
    cyc(94);
}

void MenuActorAddX_cr(Carried *cr);
void MenuActorAddY_cr(Carried *cr);
void MenuActorAlloc_cr(Carried *cr);
void MenuActorAnimate_cr(Carried *cr);
void MenuActorAnimateFrame_cr(Carried *cr);
void MenuActorAnimateOnly_cr(Carried *cr);
void MenuActorAnimateThenExit_cr(Carried *cr);
void MenuActorApplyPalette_cr(Carried *cr);
void MenuActorBlink_cr(Carried *cr);
void MenuActorCopyGroup_cr(Carried *cr);
void MenuActorCountdown_cr(Carried *cr);
void MenuActorFind_cr(Carried *cr);
void MenuActorFollowSlide194_cr(Carried *cr);
void MenuActorFollowSlide1CF_cr(Carried *cr);
void MenuActorFree_cr(Carried *cr);
void MenuActorNew_cr(Carried *cr);
void MenuActorPoolInit_cr(Carried *cr);
void MenuActorSetTiles_cr(Carried *cr);
void MenuActorSpawn_cr(Carried *cr);
void MenuActorSpawnList_cr(Carried *cr);
void MenuActorUploadFrames_cr(Carried *cr);
void MenuActorsKind2Lower_cr(Carried *cr);
void MenuActorsSet10_cr(Carried *cr);
void MenuChoiceAnimations_cr(Carried *cr);
void MenuChoiceObject00_cr(Carried *cr);
void MenuChoiceObject00Start_cr(Carried *cr);
void MenuChoiceObject12_cr(Carried *cr);
void MenuChoiceObject12Copy_cr(Carried *cr);
void MenuChoiceObject12Start_cr(Carried *cr);
void MenuChoiceObject24_cr(Carried *cr);
void MenuChoiceObject24Start_cr(Carried *cr);
void MenuChoiceObject48_cr(Carried *cr);
void MenuChoiceObjectFrames_cr(Carried *cr);
void MenuChoiceObjectUpdate_cr(Carried *cr);
void MenuDropAway_cr(Carried *cr);
void MenuDropAwayCheck_cr(Carried *cr);
void MenuDropAwayFinish_cr(Carried *cr);
void MenuDropAwayFinishCurrent_cr(Carried *cr);
void MenuDropBounce_cr(Carried *cr);
void MenuDropStart_cr(Carried *cr);
void MenuFadeInHandler_cr(Carried *cr);
void MenuFadeOutHandler_cr(Carried *cr);
void MenuFadeUpHandler_cr(Carried *cr);
void MenuFixedColourDown_cr(Carried *cr);
void MenuFixedColourUp_cr(Carried *cr);
void MenuOamBuild_cr(Carried *cr);
void MenuRowBlinkSettings_cr(Carried *cr);
void MenuRowPalette5_cr(Carried *cr);
void MenuRowStart_cr(Carried *cr);
void MenuSelectArrive_cr(Carried *cr);
void MenuSelectConfirm_cr(Carried *cr);
void MenuSetPairAnimations_cr(Carried *cr);
void MenuSinePosition_cr(Carried *cr);
void MenuSinePosition7F_cr(Carried *cr);
void MenuSlideInDone_cr(Carried *cr);
void MenuSlideInHandler_cr(Carried *cr);

void MenuActorPoolInit_cr(Carried *cr)
{
    MenuActorPoolInit(cr->db);
    cr->a = 0x2802;
    cr->x = 0x162D;
    cr->y = 0x002C;
}

void abi_MenuActorPoolInit(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuActorPoolInit_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
    uint16_t depth = rd16(cyc_then(24, dbw(bank, FREE_DEPTH))), rec, len;

    if (depth == ACTOR_SIZE) {
        cyc(112);
        *p = (uint8_t)((*p & ~P_Z) | P_N | P_C);
        return 0xFFFF;
    }
    *p = (uint8_t)(*p & ~P_C);
    rec = rd16(cyc_then(70, dbw(bank, (uint16_t)(FREE_STACK + depth))));
    wr16(dbw(bank, FREE_DEPTH), cyc_then(52, (uint16_t)(depth + 2)));
    len = rd16(cyc_then(24, dbw(bank, LIVE_LEN)));
    wr16(dbw(bank, (uint16_t)(LIVE_LIST + len)), cyc_then(30, rec));
    len = rd16(cyc_then(24, dbw(bank, LIVE_LEN)));
    wr16(dbw(bank, LIVE_LEN), cyc_then(14, (uint16_t)(len + 1)));
    len = rd16(cyc_then(24, dbw(bank, LIVE_LEN)));
    wr16(dbw(bank, LIVE_LEN), cyc_then(14, (uint16_t)(len + 1)));
    *p = nz16(*p, rec);
    cyc(56);
    return rec;
}

void MenuActorAlloc_cr(Carried *cr)
{
    uint8_t p = cr->p;
    uint16_t rec = MenuActorAlloc(cr->db, &p);

    if (rec != 0xFFFF)
        cr->a = rec;
    cr->x = rec;
    set_p(cr, p);
}

void abi_MenuActorAlloc(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuActorAlloc_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
    uint16_t depth = (uint16_t)(rd16(cyc_then(38, dbw(bank, FREE_DEPTH))) - 2), len, moved;

    wr16(dbw(bank, FREE_DEPTH), cyc_then(52, depth));
    wr16(dbw(bank, (uint16_t)(FREE_STACK + depth)), cyc_then(30, rec));
    len = (uint16_t)(rd16(cyc_then(24, dbw(bank, LIVE_LEN))) - 2);
    wr16(dbw(bank, LIVE_LEN), cyc_then(52, len));
    moved = rd16(cyc_then(30, dbw(bank, (uint16_t)(LIVE_LIST + len))));
    wr16(dbw(bank, (uint16_t)(LIVE_LIST + slot)), cyc_then(30, moved));
    wr16(dbw(bank, CURRENT), cyc_then(48, 0xFFFF));
    return moved;
}

void MenuActorFree_cr(Carried *cr)
{
    cr->a = MenuActorFree(cr->db, cr->x, cr->y);
    cr->x = 0xFFFF;
    set_p(cr, (uint8_t)((cr->p & ~P_Z) | P_N));
    cyc(42);
}

void abi_MenuActorFree(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuActorFree_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
 * HOW: Compares each record's +$1A with A (kept at $11E1); with M 8-bit
 * (MenuRegionPal) as bytes, A's high byte kept.
 * Entry: X 16-bit, D = $0000; A = kind. Exit: A = the kind of
 * the last record looked at (the kind searched for with an empty list);
 * found: X = the record, Y = its list offset, Z and C set. Not found: Y =
 * $FFFE, N set, Z clear, C from the last compare.
 */
/* The kind MenuActorFind last loaded (its A on exit when the list is not
   empty). */
static uint16_t find_last_kind;

int MenuActorFind(uint8_t bank, uint16_t kind, uint16_t *x, uint16_t *y, uint8_t *p)
{
    uint16_t i;
    int walked = 0;

    find_last_kind = kind;
    wr16(dbw(bank, 0x11E1), cyc_then(24, kind));
    i = (uint16_t)(rd16(cyc_then(24, dbw(bank, LIVE_LEN))) - 2);
    while (!(i & 0x8000)) {
        *x = rd16(cyc_then(74, dbw(bank, (uint16_t)(LIVE_LIST + i))));
        cyc(22);
        find_last_kind = rd16((uint16_t)(*x + 0x1A));
        *p = cmp16(*p, find_last_kind, cyc_then(16, rd16(cyc_then(24, dbw(bank, 0x11E1)))));
        if (*p & P_Z) {
            cyc(42);
            *y = i;
            return 1;
        } else { cyc(12); }
        i = (uint16_t)(i - 2);
        walked = 1;
    }
    cyc(walked ? 80 : 92);
    *y = i;
    *p = nz16(*p, i);
    return 0;
}

/* MenuActorFind with an 8-bit accumulator (MenuRegionCheck's PAL stop):
   the kind is stored and compared as a byte with the low byte of +$1A;
   *a is A's low byte, as the last record looked at left it. */
void MenuActorFind8(uint8_t bank, uint8_t kind, uint16_t *x, uint16_t *y, uint8_t *p, uint8_t *a)
{
    uint8_t v = 0, m;
    uint16_t i;

    wr8(dbw(bank, 0x11E1), kind);
    i = (uint16_t)(rd16(dbw(bank, LIVE_LEN)) - 2);
    *p = nz16(*p, i);
    while (!(i & 0x8000)) {
        *x = rd16(dbw(bank, (uint16_t)(LIVE_LIST + i)));
        v = rd8((uint16_t)(*x + 0x1A));
        *a = v;
        m = rd8(dbw(bank, 0x11E1));
        *p = (uint8_t)((nz8(*p, (uint8_t)(v - m)) & ~P_C) | (v >= m ? P_C : 0));
        if (v == m) {
            *y = i;
            return;
        }
        i = (uint16_t)(i - 2);
        *p = nz16(*p, i);
    }
    *y = i;
}

/* MenuActorFind with the A it leaves (*a: the kind last loaded). */
int MenuActorFindA(uint8_t bank, uint16_t kind, uint16_t *x, uint16_t *y, uint8_t *p, uint16_t *a)
{
    int r = MenuActorFind(bank, kind, x, y, p);

    *a = find_last_kind;
    return r;
}

void MenuActorFind_cr(Carried *cr)
{
    uint16_t x = cr->x, y = cr->y;
    uint8_t p = cr->p, a8 = (uint8_t)cr->a;

    if (cr->p & P_M) {
        MenuActorFind8(cr->db, a8, &x, &y, &p, &a8);
        cr->x = x;
        cr->y = y;
        cr->a = (uint16_t)((cr->a & 0xFF00) | a8);
        set_p(cr, p);
        return;
    }
    MenuActorFind(cr->db, cr->a, &x, &y, &p);
    cr->x = x;
    cr->y = y;
    cr->a = find_last_kind;
    set_p(cr, p);
}

void abi_MenuActorFind(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuActorFind_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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

    wr16(dbw(bank, 0x11DF), cyc_then(24, value));
    wr16(dbw(bank, 0x11E1), cyc_then(24, kind));
    for (i = (uint16_t)(rd16(cyc_then(54, dbw(bank, LIVE_LEN))) - 2); !(i & 0x8000); i = (uint16_t)(i - 2)) {
        rec = rd16(cyc_then(74, dbw(bank, (uint16_t)(LIVE_LIST + i))));
        cyc(22);
        a = rd16((uint16_t)(rec + 0x1A));
        *p = cmp16(*p, a, rd16(cyc_then(24, dbw(bank, 0x11E1))));
        if (*p & P_Z) {
            a = rd16(cyc_then(40, dbw(bank, 0x11DF)));
            cyc(22);
            wr16((uint16_t)(rec + 0x10), a);
        } else { cyc(22); }
        cyc(6);
    }
    cyc(116);
    return a;
}

void MenuActorsSet10_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = MenuActorsSet10(cr->db, cr->a, cr->y, &p);
    cr->y = 0xFFFE;
    set_p(cr, nz16(p, cr->x));
}

void abi_MenuActorsSet10(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuActorsSet10_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
    cyc(68);
    uint16_t pal = rd16((uint16_t)(rec + 0x24)), i, n, a = 0;
    unsigned s;

    if (!pal) {
        cyc(64);
        *p = (uint8_t)((*p & ~P_N) | P_Z);
        return 0;
    }
    wr16(dbw(bank, 0x11DF), cyc_then(40, pal));
    cyc(52);
    i = rd16((uint16_t)(rec + 0x12));
    cyc(46);
    n = rd16(0x7F1356 + i);
    i = (uint16_t)(i + 2);
    do {
        cyc(74);
        wr16(0x7F135A + i, cyc_then(32, (uint16_t)((rd16(0x7F135A + i) & 0xF1FF) | rd16(cyc_then(48, dbw(bank, 0x11DF))))));
        s = (unsigned)i + 8;
        *p = (uint8_t)((*p & ~(P_C | P_V)) | (s > 0xFFFF ? P_C : (cyc(60), 0)) | ((~(i ^ 8) & (i ^ (uint16_t)s) & 0x8000) ? P_V : 0));
        a = i = (uint16_t)s;
        n = (uint16_t)(n - 1);
    } while (!(n & 0x8000));
    *y = n;
    cyc(94);
    wr16((uint16_t)(rec + 0x24), 0x0000);
    *p = nz16(*p, rec);
    cyc(42);
    return a;
}

void MenuActorApplyPalette_cr(Carried *cr)
{
    uint8_t p = cr->p;
    uint16_t a = MenuActorApplyPalette(cr->db, cr->x, &cr->y, &p);

    cr->a = a;
    set_p(cr, p);
}

void abi_MenuActorApplyPalette(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuActorApplyPalette_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:C47B MenuActorIdleC47B
 * WHAT IT DOES: A handler that does nothing.
 * HOW: Returns.
 * Entry: any. Exit: nothing changes.
 */
void MenuActorIdleC47B(void)
{
    cyc(42);
}

/* MenuActorIdleC47B on carried registers: no register work. */
void MenuActorIdleC47B_cr(Carried *cr)
{
    (void)cr;
    cyc(42);
}

void abi_MenuActorIdleC47B(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuActorIdleC47B_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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

/* MenuActorIdleCE89 on carried registers: no register work. */
void MenuActorIdleCE89_cr(Carried *cr)
{
    (void)cr;
    cyc(42);
}

void abi_MenuActorIdleCE89(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuActorIdleCE89_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
    cyc(70);
    uint16_t a = rd16((uint32_t)bank << 16 | 0x11DD) & 0x0003;

    if (a)
        { cyc(88); return a; }
    cyc(64);
    *x = rd16((uint32_t)bank << 16 | 0x11E7);
    cyc(32);
    a = (uint16_t)(rd16(0x7F135A + *x) ^ 0x0800);
    cyc(56);
    wr16(0x7F135A + *x, a);
    cyc(42);
    return a;
}

void MenuActorBlink_cr(Carried *cr)
{
    cr->a = MenuActorBlink(cr->db, &cr->x);
    set_p(cr, nz16(cr->p, cr->a));
}

void abi_MenuActorBlink(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuActorBlink_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
 * offset is kept at $11DF while it is used and read back for the add.
 * Entry: M 16-bit, X 16-bit, D = $0000; X = the object being run ($11F9).
 * Exit: no animation: A = +$14; N set. Timer running: A = +$14; N, Z from
 * the timer. Stepped: A = the new timer; X = $11F9; N, Z from A; C, V from
 * the entry offset + animation offset.
 */
uint16_t MenuActorAnimate(uint8_t bank, uint16_t rec, uint16_t *x, uint8_t *p)
{
    uint32_t b = (uint32_t)bank << 16;
    cyc(22);
    uint16_t anim = rd16((uint16_t)(rec + 0x14)), t, pos, i, d, g, m;
    unsigned s;

    if (anim & 0x8000) {
        cyc(22);
        *p = nz16(*p, anim);
        return anim;
    }
    cyc(38);
    t = (uint16_t)(rd16((uint16_t)(rec + 0x18)) - 1);
    cyc(14);
    wr16((uint16_t)(rec + 0x18), t);
    if (!(t & 0x8000)) {
        cyc(22);
        *p = nz16(*p, t);
        return anim;
    }
    cyc(40);
    wr16(b | 0x11DF, anim);
    cyc(22);
    pos = (uint16_t)(rd16((uint16_t)(rec + 0x16)) + 4);
    cyc(60);
    wr16((uint16_t)(rec + 0x16), pos);
    cyc(38);
    m = rd16(b | 0x11DF);
    s = (unsigned)pos + m;
    *p = (uint8_t)((*p & ~(P_C | P_V)) | (s > 0xFFFF ? P_C : 0) | ((~(pos ^ m) & (pos ^ (uint16_t)s) & 0x8000) ? P_V : 0));
    i = (uint16_t)s;
    cyc(102);
    d = rd16(0x0BAB60 + i);
    if (d & 0x8000) {
        wr16((uint16_t)(rd16(b | 0x11F9) + 0x16), cyc_then(22, 0x0000));
        cyc(24);
        i = rd16(b | 0x11DF);
        cyc(66);
        d = rd16(0x0BAB60 + i);
    }
    cyc(20);
    wr16(b | 0x11DF, (uint16_t)(d - 1));
    cyc(72);
    g = rd16(0x0BAB62 + i);
    *x = rd16(b | 0x11F9);
    cyc(22);
    wr16((uint16_t)(*x + 0x12), g);
    cyc(24);
    t = rd16(b | 0x11DF);
    cyc(22);
    wr16((uint16_t)(*x + 0x18), t);
    *p = nz16(*p, t);
    return t;
}

void MenuActorAnimate_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cyc(88);
    cr->a = MenuActorAnimate(cr->db, cr->x, &cr->x, &p);
    set_p(cr, p);
}

void abi_MenuActorAnimate(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuActorAnimate_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
    cyc(22);
    uint16_t c = (uint16_t)(rd16((uint16_t)(rec + 0x0A)) - 1);

    cyc(14);
    wr16((uint16_t)(rec + 0x0A), c);
    if (!(c & 0x8000)) {
        cyc(22);
        *p = nz16(*p, c);
        return 0;
    }
    cyc(62);
    wr16((uint16_t)(rec + 0x08), 0xCFAB);
    cyc(46);
    wr16((uint16_t)(rec + 0x0A), 0x0000);
    *p = (uint8_t)((*p & ~P_N) | P_Z);
    return 1;
}

void MenuActorCountdown_cr(Carried *cr)
{
    uint8_t p = cr->p;

    if (MenuActorCountdown(cr->x, &p))
        cr->a = 0x0000;
    set_p(cr, p);
    cyc(42);
}

void abi_MenuActorCountdown(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuActorCountdown_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* The two sine positions: the table byte for the angle in +$0A - $40
   ($00:96FE) sign-extended, $80 added, kept to 8 bits, then `step` (+1 or
   -1) added; stored at $18BF, and at $18C1 raised to at least $A8. C is the
   compare with $A8, V the add of $80, N and Z from $11F9, which is read
   once into *x. */
static uint16_t sine_position(uint8_t bank, uint16_t *x, uint16_t step, uint8_t *p)
{
    cyc(22);
    uint16_t i = (uint16_t)(rd16((uint16_t)(*x + 0x0A)) - 0x40) & 0x00FF, v = rd16(cyc_then(302, 0x0096FE + i)) & 0x00FF, s;

    if (v >= 0x80)
        { cyc(18); v |= 0xFF00; }
    s = (uint16_t)(v + 0x80);
    *p = (uint8_t)((*p & ~(P_C | P_V)) | ((~(v ^ 0x80) & (v ^ s) & 0x8000) ? P_V : 0));
    v = (uint16_t)((s & 0x00FF) + step);
    wr16(0x0018BF, v);
    if (v >= 0x00A8)
        *p |= P_C;
    else
        { cyc(18); v = 0x00A8; }
    cyc(78);
    wr16(0x0018C1, v);
    cyc(24);
    *x = rd16((uint32_t)bank << 16 | 0x11F9);
    *p = nz16(*p, *x);
    cyc(42);
    return v;
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
uint16_t MenuSinePosition(uint8_t bank, uint16_t *x, uint8_t *p)
{
    return sine_position(bank, x, 0x0001, p);
}

void MenuSinePosition_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = MenuSinePosition(cr->db, &cr->x, &p);
    set_p(cr, p);
}

void abi_MenuSinePosition(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuSinePosition_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:CA32 MenuSinePosition7F
 * WHAT IT DOES: As MenuSinePosition, but two lower: (sine of the angle - $40)
 * + $7F, kept to 8 bits, at $18BF, and the same, at least $A8, at $18C1.
 * HOW: The same code as MenuSinePosition with a decrement where it has an
 * increment; a value of 0 before it becomes $FFFF.
 * Entry: as MenuSinePosition. Exit: as MenuSinePosition.
 */
uint16_t MenuSinePosition7F(uint8_t bank, uint16_t *x, uint8_t *p)
{
    cyc(46);
    return sine_position(bank, x, 0xFFFF, p);
}

void MenuSinePosition7F_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = MenuSinePosition7F(cr->db, &cr->x, &p);
    set_p(cr, p);
}

void abi_MenuSinePosition7F(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuSinePosition7F_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
    cyc(156);
    uint16_t e = (uint16_t)(off + rd16((uint16_t)(rec + 0x12))), t, w, r;

    cyc(24);
    wr16(b | 0x11E7, e);
    cyc(136);
    wr16(b | 0x11DF, rd16(0x0BC528 + (uint16_t)(i << 1)));
    cyc(24);
    e = rd16(b | 0x11E7);
    cyc(32);
    wr16(0x7F135A + e, cyc_then(32, (uint16_t)((rd16(0x7F135A + e) & 0xFE00) | rd16(cyc_then(48, b | 0x11DF)))));
    cyc(24);
    t = (uint16_t)(rd16(b | 0x11DF) + 0x10);
    cyc(62);
    wr16(b | 0x11DF, t);
    r = (uint16_t)(e - 8);
    *p = (uint8_t)((*p & ~(P_C | P_V)) | (e >= 8 ? P_C : 0) | (((e ^ 8) & (e ^ r) & 0x8000) ? P_V : 0));
    cyc(98);
    w = (uint16_t)((rd16(0x7F135A + r) & 0xFE00) | rd16(cyc_then(48, b | 0x11DF)));
    cyc(32);
    wr16(0x7F135A + r, w);
    *p = nz16(*p, rec);
    cyc(78);
    return w;
}

void MenuActorSetTiles_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = MenuActorSetTiles(cr->db, cr->x, cr->a, cr->y, &p);
    set_p(cr, p);
}

void abi_MenuActorSetTiles(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuActorSetTiles_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* The 32-bit field at rec + to plus the one at rec + from (CLC, two ADCs):
   the new high word, with N, Z, C, V from its add. */
static uint16_t add32(uint16_t rec, uint8_t to, uint8_t from, uint8_t *p)
{
    cyc(22);
    uint16_t a = rd16((uint16_t)(rec + to));

    *p &= (uint8_t)~P_C;
    cyc(36);
    a = alu_adc16(a, rd16((uint16_t)(rec + from)), p);
    cyc(22);
    wr16((uint16_t)(rec + to), a);
    cyc(22);
    a = rd16((uint16_t)(rec + to + 2));
    cyc(22);
    a = alu_adc16(a, rd16((uint16_t)(rec + from + 2)), p);
    cyc(22);
    wr16((uint16_t)(rec + to + 2), a);
    return a;
}

/*
 * @routine 0B:E221 MenuActorAddX
 * WHAT IT DOES: Adds object X's 32-bit +$1C (low word) and +$1E (high
 * word) to its 32-bit +$00 and +$02. Nothing calls it (no JSR, pointer or
 * table reaches it), so a direct call is its only way in.
 * HOW: CLC, then two 16-bit ADCs, the carry passed from the low word to the
 * high.
 * Entry: M 16-bit, D = $0000; X = record. Exit: A = the new +$02; N, Z, C,
 * V from the high word's add.
 */
uint16_t MenuActorAddX(uint16_t rec, uint8_t *p)
{
    uint16_t a = add32(rec, 0x00, 0x1C, p);

    cyc(42);
    return a;
}

void MenuActorAddX_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = MenuActorAddX(cr->x, &p);
    set_p(cr, p);
}

void abi_MenuActorAddX(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuActorAddX_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:E22F MenuActorAddY
 * WHAT IT DOES: As MenuActorAddX for the next pair: object X's 32-bit +$20
 * and +$22 added to its +$04 and +$06. Nothing calls it, so a direct call
 * is its only way in.
 * HOW: As MenuActorAddX.
 * Entry: M 16-bit, D = $0000; X = record. Exit: A = the new +$06; N, Z, C,
 * V from the high word's add.
 */
uint16_t MenuActorAddY(uint16_t rec, uint8_t *p)
{
    uint16_t a = add32(rec, 0x04, 0x20, p);

    cyc(42);
    return a;
}

void MenuActorAddY_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = MenuActorAddY(cr->x, &p);
    set_p(cr, p);
}

void abi_MenuActorAddY(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuActorAddY_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:E2A7 MenuActorsKind2Lower
 * WHAT IT DOES: Moves every live object of kind 2 (+$1A) 12 down: +$06 +=
 * $0C, for the list at $1239 from its end. Nothing calls it, so a direct
 * call is its only way in.
 * HOW: Y walks the list from $11F7 - 2 to 0 by 2 (through the data bank);
 * CMP #2, then CLC and ADC #$000C.
 * Entry: M 16-bit, X 16-bit, D = $0000. Exit: Y = the first negative
 * offset ($FFFE when the list ends at 0); X = the first entry's record and
 * A its last value read or added (both unchanged for an empty list); N set,
 * Z clear (DEY); C from the last CMP or ADC, V from the last ADC.
 */
void MenuActorsKind2Lower(uint8_t bank, uint16_t *x, uint16_t *y_out, uint16_t *a, uint8_t *p)
{
    uint16_t yy = (uint16_t)(rd16(cyc_then(24, dbw(bank, 0x11F7))) - 2);
    int first = 1;

    *p = nz16(*p, yy);
    while (!(*p & P_N)) {
        cyc(first ? 74 : 80);
        first = 0;
        *x = rd16(((uint32_t)bank << 16) + 0x1239u + yy);
        cyc(22);
        *a = rd16((uint16_t)(*x + 0x1A));
        *p = cmp16(*p, *a, 0x0002);
        if (*p & P_Z) {
            cyc(62);
            *a = rd16((uint16_t)(*x + 0x06));
            *p &= (uint8_t)~P_C;
            *a = alu_adc16(*a, 0x000C, p);
            cyc(60);
            wr16((uint16_t)(*x + 0x06), *a);
        } else { cyc(46); }
        yy = (uint16_t)(yy - 2);
        *p = nz16(*p, yy);
    }
    cyc(first ? 92 : 86);
    *y_out = yy;
}

void MenuActorsKind2Lower_cr(Carried *cr)
{
    uint16_t x = cr->x, y = cr->y, a = cr->a;
    uint8_t p = cr->p;

    MenuActorsKind2Lower(cr->db, &x, &y, &a, &p);
    cr->x = x;
    cr->y = y;
    cr->a = a;
    set_p(cr, p);
}

void abi_MenuActorsKind2Lower(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuActorsKind2Lower_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:E23D MenuActorAnimateFrame
 * WHAT IT DOES: Steps object X's sprite animation once a frame, or four
 * times when the fast animation flag $18FD is set.
 * HOW: MenuActorAnimate, then three more calls when $18FD is not zero.
 * Entry: M 16-bit, X 16-bit, D = $0000; X = the object being run ($11F9).
 * Exit: flag clear: A = 0; Z set, N clear; X, C, V from the one step.
 * Flag set: A, X, P from the last step.
 */
uint16_t MenuActorAnimateFrame(uint8_t bank, uint16_t *x, uint8_t *p)
{
    cyc(4);
    uint16_t a;
    int i;

    cyc(88);
    MenuActorAnimate(bank, *x, x, p);
    cyc(74);
    a = rd16(0x0018FD);
    if (!a) {
        cyc(64);
        *p = nz16(*p, a);
        return a;
    }
    /* BEQ and JSR, then the RTS and the next JSR */
    for (i = 0; i < 3; i++)
        { cyc(i ? 88 : 62); a = MenuActorAnimate(bank, *x, x, p); }
    cyc(84);
    return a;
}

void MenuActorAnimateFrame_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = MenuActorAnimateFrame(cr->db, &cr->x, &p);
    set_p(cr, p);
}

void abi_MenuActorAnimateFrame(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuActorAnimateFrame_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* Starts animation a on record rec unless it is already the one playing:
   CMP of a with +$14; on a change +$14 = a and the entry offset (+$16) and
   frame timer (+$18) start from 0. */
static void start_animation(uint16_t rec, uint16_t a, uint8_t *p)
{
    cyc(22);
    *p = cmp16(*p, a, rd16((uint16_t)(rec + 0x14)));
    if (*p & P_Z)
        { cyc(22); return; }
    cyc(38);
    wr16((uint16_t)(rec + 0x14), a);
    cyc(22);
    wr16((uint16_t)(rec + 0x16), 0x0000);
    cyc(22);
    wr16((uint16_t)(rec + 0x18), 0x0000);
}

/*
 * @routine 0B:C82F MenuSetPairAnimations
 * WHAT IT DOES: Starts animation pair A of the table at $0B:C862 on the
 * objects of kinds 3 and 5: the pair's second word on kind 5, its first on
 * kind 3.
 * HOW: Both words are pushed, then pulled in reverse after each
 * MenuActorFind; an object already playing the animation is left alone.
 * Entry: M 16-bit, X 16-bit, D = $0000; A = pair index. Exit: A = the
 * pair's first word; X = $11F9; Y from the second search; N, Z from X; C
 * from the last compare.
 */
uint16_t MenuSetPairAnimations(uint8_t bank, uint16_t pair, uint16_t *x, uint16_t *y, uint8_t *p)
{
    cyc(314);
    uint16_t i = (uint16_t)(pair << 2), first = rd16(0x0BC862 + i), second = rd16(0x0BC864 + i);

    *p = (uint8_t)((*p & ~P_C) | ((pair << 1) & 0x8000 ? P_C : 0));
    MenuActorFind(bank, 0x0005, x, y, p);
    cyc(36);
    start_animation(*x, second, p);
    cyc(70);
    MenuActorFind(bank, 0x0003, x, y, p);
    cyc(36);
    start_animation(*x, first, p);
    *x = rd16(cyc_then(24, dbw(bank, CURRENT)));
    *p = nz16(*p, *x);
    cyc(42);
    return first;
}

void MenuSetPairAnimations_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = MenuSetPairAnimations(cr->db, cr->a, &cr->x, &cr->y, &p);
    set_p(cr, p);
}

void abi_MenuSetPairAnimations(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuSetPairAnimations_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:C164 MenuChoiceAnimations
 * WHAT IT DOES: Starts on each of the three objects whose kinds are listed
 * at $0B:C15E the animation that shows its choice: for choice i, the word
 * at $0B:C1B0 + $18CD[i] * 8 + $18E5[i] * 2.
 * HOW: i runs over 0, 2, 4, kept on the stack and at $11E3; the table
 * offset, then the animation, is built at $11E5. An object already playing
 * the animation is left alone.
 * Entry: M 16-bit, X 16-bit, D = $0000. Exit: A = the last animation; X =
 * $11F9; Y from the last search; N, Z from X; C set; V from the last add of
 * $18E5[i] * 2.
 */
uint16_t MenuChoiceAnimations(uint8_t bank, uint16_t *x, uint16_t *y, uint8_t *p)
{
    uint16_t i, a = 0, s, anim = 0;
    unsigned sum;

    cyc(0);
    for (i = 0; i != 6; i = (uint16_t)(i + 2)) {
        wr16(dbw(bank, 0x11E3), cyc_then(124, i));
        wr16(dbw(bank, 0x11E5), cyc_then(24, 0x0000));
        cyc(32);
        a = (uint16_t)(rd16(0x0018CD + i) << 3);
        wr16(dbw(bank, 0x11E5), cyc_then(24, (uint16_t)(a + rd16(cyc_then(80, dbw(bank, 0x11E5))))));
        cyc(32);
        a = (uint16_t)(rd16(0x0018E5 + i) << 1);
        s = rd16(cyc_then(52, dbw(bank, 0x11E5)));
        sum = (unsigned)a + s;
        *p = (uint8_t)((*p & ~P_V) | ((~(a ^ s) & (a ^ (uint16_t)sum) & 0x8000) ? P_V : 0));
        cyc(86);
        anim = rd16(0x0BC1B0 + (uint16_t)sum);
        wr16(dbw(bank, 0x11E5), anim);
        MenuActorFind(bank, rd16(cyc_then(94, 0x0BC15E + rd16(cyc_then(24, dbw(bank, 0x11E3))))), x, y, p);
        anim = rd16(cyc_then(24, dbw(bank, 0x11E5)));
        start_animation(*x, anim, p);
        cyc(40);
    }
    *p |= P_C;
    *x = rd16(cyc_then(88, dbw(bank, CURRENT)));
    *p = nz16(*p, *x);
    cyc(42);
    return anim;
}

void MenuChoiceAnimations_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = MenuChoiceAnimations(cr->db, &cr->x, &cr->y, &p);
    set_p(cr, p);
}

void abi_MenuChoiceAnimations(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuChoiceAnimations_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* Sets animation a on record rec unless it already has it, comparing the
   field with a (the reverse of start_animation's compare). */
static void start_animation_rev(uint16_t rec, uint16_t a, uint8_t *p)
{
    cyc(22);
    *p = cmp16(*p, rd16((uint16_t)(rec + 0x14)), a);
    if (*p & P_Z)
        return;
    cyc(86);
    wr16((uint16_t)(rec + 0x14), a);
    cyc(22);
    wr16((uint16_t)(rec + 0x16), 0x0000);
    cyc(22);
    wr16((uint16_t)(rec + 0x18), 0x0000);
}

/*
 * @routine 0B:C8BF MenuSlideInDone
 * WHAT IT DOES: Once the sliding sprites' lowest position ($18C1) reaches
 * $100: clears +$10 of the objects of kinds 4 and 5, starts animation $AE
 * on kind 6 and $B8 on kind 7, and frees object X (the one being run).
 * HOW: MenuActorsSet10 twice, MenuActorFind twice with X kept on the stack,
 * then MenuActorFree with Y = its list offset at $11FB. An object already
 * playing its animation is left alone.
 * Entry: M 16-bit, X 16-bit, D = $0000; X = the object being run. Exit:
 * not yet: A = $18C1; N, Z, C from the compare with $100. Done: as
 * MenuActorFree, Y = $11FB, C from the last animation compare.
 */
int MenuSlideInDone(uint8_t bank, uint16_t rec, uint16_t *a, uint16_t *y, uint8_t *p)
{
    uint16_t x = rec;

    cyc(32);
    *a = rd16(0x0018C1);
    *p = cmp16(*p, *a, 0x0100);
    if (!(*p & P_Z))
        { cyc(4); return 0; } else { cyc(134); }
    MenuActorsSet10(bank, 0x0004, 0x0000, p);
    cyc(94);
    MenuActorsSet10(bank, 0x0005, 0x0000, p);
    cyc(100);
    MenuActorFind(bank, 0x0006, &x, y, p);
    start_animation_rev(x, 0x00AE, p);
    cyc(70);
    MenuActorFind(bank, 0x0007, &x, y, p);
    start_animation_rev(x, 0x00B8, p);
    *y = rd16(cyc_then(60, dbw(bank, 0x11FB)));
    cyc(46);
    *a = MenuActorFree(bank, rec, *y);
    *p = (uint8_t)((*p & ~P_Z) | P_N);
    return 1;
}

void MenuSlideInDone_cr(Carried *cr)
{
    uint8_t p = cr->p;

    if (MenuSlideInDone(cr->db, cr->x, &cr->a, &cr->y, &p))
        cr->x = 0xFFFF;
    cyc(42);
    set_p(cr, p);
    cyc(42);
}

void abi_MenuSlideInDone(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuSlideInDone_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:CA67 MenuChoiceObjectUpdate
 * WHAT IT DOES: Keeps object X, the choice numbered +$20 of a four-way
 * selection, in step with the selection ($18C3, previously $18C5): it gets
 * palette $0200 when the selection has just left it and $0A00 when it has
 * just come to it; arriving there, or resting on it once the selection's
 * countdown $18F5 is 0, sets +$22 = 1 and +$0A = 0 (unless +$10 is
 * negative), and arriving also restarts the countdown at $FF.
 * HOW: Three compare chains on the 16-bit words.
 * Entry: M 16-bit, X 16-bit, D = $0000; X = record. Exit: A = the last value
 * loaded; N, Z from it or the last compare; C from the last compare; V
 * unchanged.
 */
uint16_t MenuChoiceObjectUpdate(uint16_t rec, uint8_t *p)
{
    uint16_t a, slot = (uint16_t)(rec + 0x20);

    cyc(32);
    a = rd16(0x0018C5);
    cyc(22);
    *p = cmp16(*p, a, rd16(slot));
    if (*p & P_Z) {
        cyc(48);
        *p = cmp16(*p, a, rd16(0x0018C3));
        if (!(*p & P_Z))
            { cyc(62); wr16((uint16_t)(rec + 0x24), 0x0200); } else { cyc(22); }
    } else { cyc(22); }
    cyc(32);
    a = rd16(0x0018C3);
    cyc(22);
    *p = cmp16(*p, a, rd16(slot));
    if (*p & P_Z) {
        cyc(48);
        *p = cmp16(*p, a, rd16(0x0018C5));
        if (!(*p & P_Z)) {
            a = 0x0A00;
            cyc(62);
            wr16((uint16_t)(rec + 0x24), a);
            cyc(22);
            a = rd16((uint16_t)(rec + 0x10));
            *p = nz16(*p, a);
            if (!(a & 0x8000)) {
                a = 0x0001;
                cyc(62);
                wr16((uint16_t)(rec + 0x22), a);
                cyc(22);
                wr16((uint16_t)(rec + 0x0A), 0x0000);
                a = 0x00FF;
                cyc(56);
                wr16(0x0018F5, a);
            }
        } else { cyc(22); }
    } else { cyc(22); }
    cyc(32);
    a = rd16(0x0018F5);
    *p = nz16(*p, a);
    if (a)
        { cyc(64); return a; }
    cyc(48);
    a = rd16(0x0018C3);
    cyc(22);
    *p = cmp16(*p, a, rd16(slot));
    if (!(*p & P_Z))
        { cyc(64); return a; }
    cyc(38);
    a = rd16((uint16_t)(rec + 0x10));
    *p = nz16(*p, a);
    if (a & 0x8000)
        return a;
    a = 0x0001;
    *p = nz16(*p, a);
    cyc(62);
    wr16((uint16_t)(rec + 0x22), a);
    cyc(22);
    wr16((uint16_t)(rec + 0x0A), 0x0000);
    cyc(42);
    return a;
}

void MenuChoiceObjectUpdate_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = MenuChoiceObjectUpdate(cr->x, &p);
    set_p(cr, p);
}

void abi_MenuChoiceObjectUpdate(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuChoiceObjectUpdate_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

uint16_t MenuRowPosition(uint16_t rec, uint8_t *p);

/* Sets the palette field (bits 9-11) of the sprite entry at $7F:135A + x
   to 5. */
static uint16_t entry_palette5(uint16_t x, uint8_t *p)
{
    cyc(46);
    uint16_t w = (uint16_t)((rd16(0x7F135A + x) & 0xF1FF) | 0x0A00);

    cyc(80);
    wr16(0x7F135A + x, w);
    *p = nz16(*p, w);
    cyc(42);
    return w;
}

/*
 * @routine 0B:C47C MenuRowPalette5
 * WHAT IT DOES: Gives the two sprite entries at the row position of the
 * kind 9 object (MenuRowPosition) palette 5.
 * HOW: MenuActorFind for kind 9; each entry's attribute word keeps all but
 * bits 9-11 and takes $0A00; the second entry is 8 bytes on.
 * Entry: M 16-bit, X 16-bit, D = $0000. Exit: A = the second word written;
 * X = the position + 8; Y from the search; N, Z from A; C, V from the add
 * of 8.
 */
uint16_t MenuRowPalette5(uint8_t bank, uint16_t *x, uint16_t *y, uint8_t *p)
{
    cyc(138);
    uint16_t pos, n;

    MenuActorFind(bank, 0x0009, x, y, p);
    pos = MenuRowPosition(*x, p);
    entry_palette5(pos, p);
    n = (uint16_t)(pos + 8);
    *p = (uint8_t)((*p & ~(P_C | P_V)) | (n < pos ? P_C : (cyc(10), 0)) | ((~(pos ^ 8) & (pos ^ n) & 0x8000) ? P_V : 0));
    *x = n;
    return entry_palette5(n, p);
}

void MenuRowPalette5_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = MenuRowPalette5(cr->db, &cr->x, &cr->y, &p);
    set_p(cr, p);
}

void abi_MenuRowPalette5(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuRowPalette5_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:C4A9 MenuRowBlinkSettings
 * WHAT IT DOES: Blinks the two sprite entries at object X's row position
 * (MenuRowPosition, MenuActorBlink on each), then shows the three saved
 * settings $18EF, $18F1 and $18F3 as tiles in the kind 9 object's sprite
 * entries $2A, $1A and $0A (MenuActorSetTiles).
 * HOW: The entry offset is kept at $11E7, the second 8 bytes on.
 * Entry: M 16-bit, X 16-bit, D = $0000; X = record. Exit: A and P as the
 * last MenuActorSetTiles; X from the search; Y = $000A.
 */
uint16_t MenuRowBlinkSettings(uint8_t bank, uint16_t *x, uint16_t *y, uint8_t *p)
{
    cyc(46);
    uint16_t pos = MenuRowPosition(*x, p), a;

    wr16(dbw(bank, 0x11E7), cyc_then(24, pos));
    MenuActorBlink(bank, x);
    wr16(dbw(bank, 0x11E7), cyc_then(62, (uint16_t)(rd16(cyc_then(24, dbw(bank, 0x11E7))) + 8)));
    MenuActorBlink(bank, x);
    cyc(70);
    MenuActorFind(bank, 0x0009, x, y, p);
    cyc(56);
    MenuActorSetTiles(bank, *x, rd16(0x0018EF), 0x002A, p);
    cyc(56);
    MenuActorSetTiles(bank, *x, rd16(0x0018F1), 0x001A, p);
    *y = 0x000A;
    cyc(56);
    a = MenuActorSetTiles(bank, *x, rd16(0x0018F3), 0x000A, p);
    cyc(42);
    return a;
}

void MenuRowBlinkSettings_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = MenuRowBlinkSettings(cr->db, &cr->x, &cr->y, &p);
    set_p(cr, p);
}

void abi_MenuRowBlinkSettings(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuRowBlinkSettings_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void VramDmaQueueAdd(void);

/* Queues $40 bytes from $7F:src to VRAM word address dest through the
   transfer request words at $16B1-$16B7. */
static void queue_frame_row(uint32_t b, uint16_t dest, uint16_t src)
{
    cyc(46);
    wr16(b | 0x16B7, dest);
    cyc(24);
    wr16(b | 0x16B3, src);
    cyc(48);
    wr16(b | 0x16B1, 0x0040);
    cyc(48);
    wr16(b | 0x16B5, 0x007F);
    (cyc(62), VramDmaQueueAdd());
}

/*
 * @routine 0B:CBC3 MenuActorUploadFrames
 * WHAT IT DOES: Shows animation frame A on every sprite of object X's
 * group: for each sprite, its graphics (two rows of $40 bytes in bank $7F,
 * the second $200 bytes after the first) are queued to its VRAM slot and
 * the slot + $100, and bit 15 of the frame word becomes bit 15 (vertical
 * flip) of the sprite's attribute word.
 * HOW: The group's sprite count is the word at $7F:1356 + +$12, its entries
 * follow at $7F:135A. Per sprite i (table offset $11E3, set by the caller):
 * VRAM slot $0B:CCBC[i], graphics base $0B:CD14[i]; the frame word comes
 * from $0B:CC7C at $11DF = A * 2, which steps by 2 per sprite, wrapping at
 * $40. $11E1 holds the slot (its high byte incremented for the second
 * row), $11E5 the entry offset, $11E7 the flip bit, $11E9 the base.
 * Entry: M 16-bit, X 16-bit, D = $0000; X = record, A = frame, $11E3 =
 * first table offset. Exit: A = the entry offset past the last sprite; X =
 * $11F9; Y = $FFFF; N, Z from X; C, V from the last add of 8.
 */
uint16_t MenuActorUploadFrames(uint8_t bank, uint16_t rec, uint16_t frame, uint16_t *xo, uint8_t *p)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t n, i, v, src, e, s = 0, at;

    cyc(38);
    wr16(b | 0x11DF, (uint16_t)(frame << 1));
    cyc(22);
    e = rd16((uint16_t)(rec + 0x12));
    cyc(46);
    n = rd16(0x7F1356 + e);
    cyc(52);
    wr16(b | 0x11E5, (uint16_t)(e + 2));
    do {
        cyc(38);
        i = rd16(b | 0x11E3);  /* read once, for both tables */
        cyc(72);
        wr16(b | 0x11E1, rd16(0x0BCCBC + i));
        cyc(72);
        wr16(b | 0x11E9, rd16(0x0BCD14 + i));
        cyc(24);
        v = rd16(cyc_then(126, 0x0BCC7C + rd16(b | 0x11DF)));
        wr16(b | 0x11E7, v & 0x8000);
        cyc(24);
        e = rd16(b | 0x11E5);
        cyc(32);
        at = rd16(0x7F135A + e) & 0x7FFF;  /* the attribute word, then $11E7 */
        cyc(48);
        wr16(0x7F135A + e, cyc_then(32, (uint16_t)(at | rd16(b | 0x11E7))));
        cyc(98);
        src = (uint16_t)((v & 0x7FFF) + rd16(b | 0x11E9));
        cyc(54);
        queue_frame_row(b, rd16(b | 0x11E1), src);
        cyc(90);
        wr8(b | 0x11E2, cyc_then(6, (uint8_t)(rd8(b | 0x11E2) + 1)));
        cyc(46);
        queue_frame_row(b, rd16(b | 0x11E1), cyc_then(74, (uint16_t)(src + 0x0200)));
        cyc(68);
        wr16(b | 0x11E3, cyc_then(14, (uint16_t)(rd16(b | 0x11E3) + 1)));  /* two memory increments */
        cyc(24);
        wr16(b | 0x11E3, cyc_then(14, (uint16_t)(rd16(b | 0x11E3) + 1)));
        cyc(24);
        wr16(b | 0x11DF, cyc_then(76, (uint16_t)(rd16(b | 0x11DF) + 2) & 0x003F));
        cyc(24);
        e = rd16(b | 0x11E5);
        s = (uint16_t)(e + 8);
        *p = (uint8_t)((*p & ~(P_C | P_V)) | (s < e ? P_C : 0) | ((~(e ^ 8) & (e ^ s) & 0x8000) ? P_V : 0));
        cyc(62);
        wr16(b | 0x11E5, s);
        n = (uint16_t)(n - 1);
    } while (cyc_loop(46, !(n & 0x8000)));
    cyc(60);
    *xo = rd16(b | CURRENT);
    *p = nz16(*p, *xo);
    cyc(42);
    return s;
}

void MenuActorUploadFrames_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = MenuActorUploadFrames(cr->db, cr->x, cr->a, &cr->x, &p);
    cr->y = 0xFFFF;
    set_p(cr, p);
}

void abi_MenuActorUploadFrames(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuActorUploadFrames_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:D5F6 MenuActorAnimateOnly
 * WHAT IT DOES: The handler of an object that only animates.
 * HOW: MenuActorAnimateFrame.
 * Entry: as MenuActorAnimateFrame. Exit: as MenuActorAnimateFrame.
 */
uint16_t MenuActorAnimateOnly(uint8_t bank, uint16_t *x, uint8_t *p)
{
    return MenuActorAnimateFrame(bank, x, p);
}

void MenuActorAnimateOnly_cr(Carried *cr)
{
    MenuActorAnimateFrame_cr(cr);
    cyc(42);
}

void abi_MenuActorAnimateOnly(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuActorAnimateOnly_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* +$02 = top - $18C1 (SEC; SBC: C, V, N, Z from it), then the animation
   step. */
static uint16_t follow_slide(uint8_t bank, uint16_t top, uint16_t *x, uint8_t *p)
{
    cyc(70);
    uint16_t m = rd16(0x0018C1), d = (uint16_t)(top - m);

    *p = (uint8_t)((nz16(*p, d) & ~(P_C | P_V)) | (top >= m ? P_C : 0) | (((top ^ m) & (top ^ d) & 0x8000) ? P_V : 0));
    cyc(22);
    wr16((uint16_t)(*x + 0x02), d);
    return cyc_then(42, MenuActorAnimateFrame(bank, x, p));
}

/*
 * @routine 0B:C985 MenuActorFollowSlide1CF
 * WHAT IT DOES: The handler of an object that moves with the sliding
 * sprites: +$02 (its position) = $1CF - $18C1, then its animation step.
 * HOW: A 16-bit subtraction, then MenuActorAnimateFrame.
 * Entry: M 16-bit, X 16-bit, D = $0000; X = record. Exit: as
 * MenuActorAnimateFrame; C and V from the subtraction when no step changed
 * them.
 */
uint16_t MenuActorFollowSlide1CF(uint8_t bank, uint16_t *x, uint8_t *p)
{
    return follow_slide(bank, 0x01CF, x, p);
}

void MenuActorFollowSlide1CF_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = MenuActorFollowSlide1CF(cr->db, &cr->x, &p);
    set_p(cr, p);
}

void abi_MenuActorFollowSlide1CF(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuActorFollowSlide1CF_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:C993 MenuActorFollowSlide194
 * WHAT IT DOES: As MenuActorFollowSlide1CF, with +$02 = $194 - $18C1.
 * HOW: As MenuActorFollowSlide1CF.
 * Entry: as MenuActorFollowSlide1CF. Exit: as MenuActorFollowSlide1CF.
 */
uint16_t MenuActorFollowSlide194(uint8_t bank, uint16_t *x, uint8_t *p)
{
    return follow_slide(bank, 0x0194, x, p);
}

void MenuActorFollowSlide194_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = MenuActorFollowSlide194(cr->db, &cr->x, &p);
    set_p(cr, p);
}

void abi_MenuActorFollowSlide194(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuActorFollowSlide194_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:CD6C MenuActorAnimateThenExit
 * WHAT IT DOES: The handler of an object that animates for +$0A + 1 frames
 * and then asks the menu to exit ($11EB = 1) on every later frame.
 * HOW: 16-bit registers forced; MenuActorAnimateFrame; a memory decrement
 * of +$0A.
 * Entry: D = $0000; X = record. Exit: A = 1 when the count is negative,
 * otherwise as MenuActorAnimateFrame; N, Z from the count or A; M 16-bit,
 * X 16-bit.
 */
uint16_t MenuActorAnimateThenExit(uint8_t bank, uint16_t *x, uint8_t *p)
{
    cyc(22);
    uint16_t a, c;

    *p &= (uint8_t)~(P_M | P_X);
    a = MenuActorAnimateFrame(bank, x, p);
    cyc(22);
    c = (uint16_t)(rd16((uint16_t)(*x + 0x0A)) - 1);
    cyc(14);
    wr16((uint16_t)(*x + 0x0A), c);
    *p = nz16(*p, c);
    if (!(c & 0x8000))
        { cyc(22); return a; }
    a = 0x0001;
    wr16(dbw(bank, 0x11EB), cyc_then(64, a));
    *p = nz16(*p, a);
    return a;
}

void MenuActorAnimateThenExit_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = MenuActorAnimateThenExit(cr->db, &cr->x, &p);
    set_p(cr, p);
    cyc(42);
}

void abi_MenuActorAnimateThenExit(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuActorAnimateThenExit_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

uint16_t MenuChoiceUpDownY(uint8_t bank, uint16_t *y);
uint16_t MenuChoiceUpDown(uint8_t bank);

/*
 * @routine 0B:C872 MenuSlideInHandler
 * WHAT IT DOES: The handler that slides the selection sprites in: the angle
 * +$0A falls by 3 a frame to 0 and sets the sprites' height
 * (MenuSinePosition7F); once it is below $60 the selection can be moved
 * (MenuChoiceUpDown); at 0 the handler becomes $B952.
 * HOW: A negative angle is clamped to 0 before it is stored; the fields
 * are read again through $11F9, which MenuSinePosition7F leaves in X.
 * Entry: M 16-bit, X 16-bit, D = $0000; X = record. Exit: at 0: A = $B952;
 * N set, Z clear. Otherwise A = +$0A and P from the compare with $60, or
 * as MenuChoiceUpDown (C clear); X = $11F9.
 */
uint16_t MenuSlideInHandler(uint8_t bank, uint16_t *x, uint16_t *y, uint8_t *p)
{
    cyc(22);
    uint16_t a = (uint16_t)(rd16((uint16_t)(*x + 0x0A)) - 3);

    if (a & 0x8000)
        { cyc(18); a = 0x0000; }
    cyc(86);
    wr16((uint16_t)(*x + 0x0A), a);
    MenuSinePosition7F(bank, x, p);
    cyc(22);
    a = rd16((uint16_t)(*x + 0x0A));
    if (!a) {
        a = 0xB952;
        cyc(62);
        wr16((uint16_t)(*x + 0x08), a);
        *p = nz16(*p, a);
        cyc(42);
        return a;
    }
    cyc(44);
    a = rd16((uint16_t)(*x + 0x0A));
    cyc(46);
    *p = cmp16(*p, a, 0x0060);
    if (*p & P_C)
        { cyc(42); return a; } else { cyc(40); }
    a = MenuChoiceUpDownY(bank, y);
    *p = nz16(*p, a);
    cyc(42);
    return a;
}

void MenuSlideInHandler_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = MenuSlideInHandler(cr->db, &cr->x, &cr->y, &p);
    set_p(cr, p);
}

void abi_MenuSlideInHandler(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuSlideInHandler_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

uint16_t MenuSpritePalettesScale(uint8_t bank, uint16_t factor);

/*
 * @routine 0B:C896 MenuFadeInHandler
 * WHAT IT DOES: The handler that fades sprite palettes 5 and 1 as +$0A
 * counts down by 2 (MenuSpritePalettesScale with the count); at 0 it sets
 * +$10 of kind 2 objects to $FFFF and of kind 3 objects to 0, and moves on
 * to handler $C8BF (MenuSlideInDone) with +$1E = 30.
 * HOW: MenuActorsSet10 twice for the last step.
 * Entry: M 16-bit, X 16-bit, D = $0000; X = record. Exit: fading: as
 * MenuSpritePalettesScale. Done: A = $1E; Y = $FFFE; N, Z clear; C from
 * the last compare.
 */
uint16_t MenuFadeInHandler(uint8_t bank, uint16_t *x, uint16_t *y, uint8_t *p)
{
    cyc(22);
    uint16_t a = rd16((uint16_t)(*x + 0x0A));

    if (a) {
        a = (uint16_t)(a - 2);
        cyc(66);
        wr16((uint16_t)(*x + 0x0A), a);
        cyc(46);
        a = MenuSpritePalettesScale(bank, a);
        *x = rd16(cyc_then(24, dbw(bank, CURRENT)));
        *y = 0xFFFF;
        *p = nz16(*p, *x);
        cyc(42);
        return a;
    } else { cyc(116); }
    MenuActorsSet10(bank, 0x0002, 0xFFFF, p);
    cyc(94);
    MenuActorsSet10(bank, 0x0003, 0x0000, p);
    *y = 0xFFFE;
    cyc(46);
    wr16((uint16_t)(*x + 0x08), 0xC8BF);
    a = 0x001E;
    cyc(46);
    wr16((uint16_t)(*x + 0x1E), a);
    *p = nz16(*p, a);
    return a;
}

void MenuFadeInHandler_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = MenuFadeInHandler(cr->db, &cr->x, &cr->y, &p);
    set_p(cr, p);
    cyc(42);
}

void abi_MenuFadeInHandler(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuFadeInHandler_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:CB81 MenuChoiceObjectFrames
 * WHAT IT DOES: Every +$1C + 1 frames (counted in +$1E), applies object X's
 * pending palette and, while it is selected (+$22), steps its frame count
 * +$0A down through 31-1 (from 2 when +$10 is negative), showing frame
 * +$0A on its sprites (MenuActorUploadFrames); the count reaching 0, or 1
 * when +$10 is negative, ends the selection (+$22 = +$0A = 0).
 * HOW: Memory decrement of +$1E; MenuActorApplyPalette; the count kept to
 * 5 bits. The fields after the frame upload are read through $11F9, which
 * it leaves in X.
 * Entry: M 16-bit, X 16-bit, D = $0000; X = record; $11E3 = the first
 * frame table offset. Exit: waiting: A unchanged; N, Z from +$1E.
 * Otherwise A and P from the last load, compare or call; X = $11F9 after
 * a frame upload or an end.
 */
uint16_t MenuChoiceObjectFrames(uint8_t bank, uint16_t a, uint16_t *x, uint16_t *y, uint8_t *p)
{
    cyc(68);
    uint16_t t = (uint16_t)(rd16((uint16_t)(*x + 0x1E)) - 1);

    cyc(14);
    wr16((uint16_t)(*x + 0x1E), t);
    if (!(t & 0x8000)) {
        cyc(88);
        *p = nz16(*p, t);
        return a;
    }
    cyc(44);
    wr16((uint16_t)(*x + 0x1E), cyc_then(22, rd16((uint16_t)(*x + 0x1C))));
    MenuActorApplyPalette(bank, *x, y, p);
    cyc(22);
    a = rd16((uint16_t)(*x + 0x22));
    *p = nz16(*p, a);
    if (!a)
        { cyc(88); return a; }
    cyc(44);
    a = rd16((uint16_t)(*x + 0x10));
    if (a & 0x8000) {
        a = 0x0002;
        cyc(62);
        wr16((uint16_t)(*x + 0x0A), a);
    } else { cyc(22); }
    cyc(22);
    a = (uint16_t)(rd16((uint16_t)(*x + 0x0A)) - 1) & 0x001F;
    cyc(60);
    wr16((uint16_t)(*x + 0x0A), a);
    *p = nz16(*p, a);
    if (a) {
        cyc(68);
        a = MenuActorUploadFrames(bank, *x, a, x, p);
        *y = 0xFFFF;
        cyc(22);
        a = rd16((uint16_t)(*x + 0x10));
        *p = nz16(*p, a);
        if (!(a & 0x8000))
            { cyc(64); return a; }
        cyc(38);
        a = rd16((uint16_t)(*x + 0x0A));
        *p = cmp16(*p, a, 0x0001);
        if (!(*p & P_Z))
            return a;
    }
    *x = rd16(cyc_then(70, dbw(bank, CURRENT)));
    *p = nz16(*p, *x);
    cyc(22);
    wr16((uint16_t)(*x + 0x22), 0x0000);
    cyc(22);
    wr16((uint16_t)(*x + 0x0A), 0x0000);
    cyc(42);
    return a;
}

void MenuChoiceObjectFrames_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = MenuChoiceObjectFrames(cr->db, cr->a, &cr->x, &cr->y, &p);
    set_p(cr, p);
}

void abi_MenuChoiceObjectFrames(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuChoiceObjectFrames_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* A choice object's frame: MenuChoiceObjectUpdate, $11E3 = the first frame
   table offset, MenuChoiceObjectFrames. The offset 0 form stores it with
   STZ, leaving A as the update returned it. */
static uint16_t choice_object(uint8_t bank, uint16_t first, uint16_t *x, uint16_t *y, uint8_t *p)
{
    cyc(46);
    uint16_t a = MenuChoiceObjectUpdate(*x, p);

    if (first) {
        cyc(24);
        a = first;
        *p = nz16(*p, a);
    }
    wr16(dbw(bank, 0x11E3), cyc_then(24, first));
    return cyc_then(42, MenuChoiceObjectFrames(bank, a, x, y, p));
}

/*
 * @routine 0B:CAEE MenuChoiceObject00
 * WHAT IT DOES: The handler of a choice object whose frames start at frame
 * table offset 0: MenuChoiceObjectUpdate, then MenuChoiceObjectFrames.
 * HOW: $11E3 = 0 between the two.
 * Entry: M 16-bit, X 16-bit, D = $0000; X = record. Exit: as
 * MenuChoiceObjectFrames.
 */
uint16_t MenuChoiceObject00(uint8_t bank, uint16_t *x, uint16_t *y, uint8_t *p)
{
    return choice_object(bank, 0x0000, x, y, p);
}

void MenuChoiceObject00_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = MenuChoiceObject00(cr->db, &cr->x, &cr->y, &p);
    set_p(cr, p);
}

void abi_MenuChoiceObject00(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuChoiceObject00_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:CB06 MenuChoiceObject12
 * WHAT IT DOES: As MenuChoiceObject00, the frames starting at offset $12.
 * HOW: As MenuChoiceObject00.
 * Entry: as MenuChoiceObject00. Exit: as MenuChoiceObject00.
 */
uint16_t MenuChoiceObject12(uint8_t bank, uint16_t *x, uint16_t *y, uint8_t *p)
{
    return choice_object(bank, 0x0012, x, y, p);
}

void MenuChoiceObject12_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = MenuChoiceObject12(cr->db, &cr->x, &cr->y, &p);
    set_p(cr, p);
}

void abi_MenuChoiceObject12(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuChoiceObject12_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:CB1B MenuChoiceObject24
 * WHAT IT DOES: As MenuChoiceObject00, the frames starting at offset $24.
 * HOW: As MenuChoiceObject00.
 * Entry: as MenuChoiceObject00. Exit: as MenuChoiceObject00.
 */
uint16_t MenuChoiceObject24(uint8_t bank, uint16_t *x, uint16_t *y, uint8_t *p)
{
    return choice_object(bank, 0x0024, x, y, p);
}

void MenuChoiceObject24_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = MenuChoiceObject24(cr->db, &cr->x, &cr->y, &p);
    set_p(cr, p);
}

void abi_MenuChoiceObject24(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuChoiceObject24_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:CB74 MenuChoiceObject48
 * WHAT IT DOES: As MenuChoiceObject00, the frames starting at offset $48.
 * HOW: As MenuChoiceObject00.
 * Entry: as MenuChoiceObject00. Exit: as MenuChoiceObject00.
 */
uint16_t MenuChoiceObject48(uint8_t bank, uint16_t *x, uint16_t *y, uint8_t *p)
{
    return choice_object(bank, 0x0048, x, y, p);
}

void MenuChoiceObject48_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = MenuChoiceObject48(cr->db, &cr->x, &cr->y, &p);
    set_p(cr, p);
}

void abi_MenuChoiceObject48(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuChoiceObject48_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:CB01 MenuChoiceObject12Start
 * WHAT IT DOES: First frame of a MenuChoiceObject12 object: makes that its
 * handler and runs it.
 * HOW: +$08 = $CB06, then falls through into MenuChoiceObject12.
 * Entry: as MenuChoiceObject12. Exit: as MenuChoiceObject12.
 */
uint16_t MenuChoiceObject12Start(uint8_t bank, uint16_t *x, uint16_t *y, uint8_t *p)
{
    cyc(46);
    wr16((uint16_t)(*x + 0x08), 0xCB06);
    return MenuChoiceObject12(bank, x, y, p);
}

void MenuChoiceObject12Start_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = MenuChoiceObject12Start(cr->db, &cr->x, &cr->y, &p);
    set_p(cr, p);
}

void abi_MenuChoiceObject12Start(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuChoiceObject12Start_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void Hang(Carried *cr);

/*
 * @routine 0B:E1C5 MenuActorCopyGroup
 * WHAT IT DOES: Gives object X ($11F9) its own copy of the sprite group its
 * animation starts with: the group (a count word, then count + 1 entries
 * of 8 bytes) is copied to the next free place in the group area, whose
 * offset becomes the object's +$12. A copy that would reach $2C81 stops the
 * game (Hang); no script gets there.
 * HOW: The source group is the word at $0B:AB62 + +$14. $11DF = the size
 * in bytes, (count + 1) * 8 + 2; $11E1 the source offset, $11E3 the
 * destination; the next free offset $120B advances by the size. Copied a
 * word at a time, $11DF counting down by 2.
 * Entry: M 16-bit, X 16-bit, D = $0000. Exit: A = the last word copied; X =
 * $11F9; N, Z from X; C clear; V from the add of the size.
 */
uint16_t MenuActorCopyGroup(Carried *cr, uint8_t bank, uint16_t *xo, uint8_t *p)
{
    uint32_t b = (uint32_t)bank << 16;
    cyc(24);
    uint16_t x = rd16(b | CURRENT), src, dst, size, a = 0;
    unsigned s;

    cyc(22);
    src = rd16(cyc_then(108, 0x0BAB62 + rd16((uint16_t)(x + 0x14))));
    size = (uint16_t)(((uint16_t)(rd16(0x7F1356 + src) + 1) << 3) + 2);
    cyc(118);
    wr16(b | 0x11DF, size);
    cyc(24);
    wr16(b | 0x11E1, src);
    cyc(24);
    x = rd16(b | CURRENT);
    cyc(24);
    dst = rd16(b | 0x120B);
    cyc(22);
    wr16((uint16_t)(x + 0x12), dst);
    cyc(24);
    wr16(b | 0x11E3, dst);
    cyc(38);
    s = (unsigned)dst + rd16(b | 0x11DF);
    *p = (uint8_t)((*p & ~P_V) | ((~(dst ^ size) & (dst ^ (uint16_t)s) & 0x8000) ? P_V : 0));
    if ((uint16_t)s >= 0x2C81) {
        /* the stop, with A the sum and X the object as the ADC and the
           reload left them */
        cr->a = (uint16_t)s;
        cr->x = x;
        set_p(cr, *p);
        Hang(cr);
    }
    *p &= (uint8_t)~P_C;
    cyc(64);
    wr16(b | 0x120B, (uint16_t)s);
    do {
        cyc(24);
        a = rd16(cyc_then(32, 0x7F1356 + rd16(b | 0x11E1)));
        cyc(24);
        wr16(0x7F1356 + rd16(b | 0x11E3), cyc_then(32, a));
        cyc(24);
        wr16(b | 0x11E1, cyc_then(14, (uint16_t)(rd16(b | 0x11E1) + 2)));
        cyc(86);
        wr16(b | 0x11E3, cyc_then(14, (uint16_t)(rd16(b | 0x11E3) + 2)));
        cyc(86);
        wr16(b | 0x11DF, cyc_then(14, (uint16_t)(rd16(b | 0x11DF) - 2)));
    } while (cyc_loop(44, rd16(cyc_then(24, b | 0x11DF))));
    cyc(62);
    *xo = rd16(b | CURRENT);
    *p = nz16(*p, *xo);
    cyc(42);
    return a;
}

void MenuActorCopyGroup_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = MenuActorCopyGroup(cr, cr->db, &cr->x, &p);
    set_p(cr, p);
}

void abi_MenuActorCopyGroup(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuActorCopyGroup_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

uint16_t FrameByteWritePush(uint8_t bank, uint16_t addr, uint8_t value);

/* $18F9 = v, and COLDATA = v on all three components in the next vertical
   blank (FrameByteWritePush). Returns A as the original leaves it: the
   high byte of v and the COLDATA value. */
static uint16_t fixed_colour_set(uint8_t bank, uint16_t v)
{
    uint8_t c = (uint8_t)(v | 0xE0);

    cyc(98);
    wr16(0x0018F9, v);
    cyc(198);
    FrameByteWritePush(bank, (uint16_t)COLDATA, c);
    return (uint16_t)((v & 0xFF00) | c);
}

/*
 * @routine 0B:C90F MenuFixedColourDown
 * WHAT IT DOES: Steps the fixed colour's intensity $18F9 down by 2 toward 0
 * (not below 0) and queues it to COLDATA for all three components.
 * HOW: Nothing happens at 0; otherwise fixed_colour_set, with X kept on the
 * stack around FrameByteWritePush.
 * Entry: M 16-bit, X 16-bit. Exit: at 0: A = 0; Z set, N clear. Otherwise
 * A = the COLDATA value (high byte 0); X unchanged; N, Z from X; M 16-bit,
 * X 16-bit. C and V unchanged.
 */
uint16_t MenuFixedColourDown(uint8_t bank, uint16_t x, uint8_t *p)
{
    cyc(32);
    uint16_t v = rd16(0x0018F9);

    if (!v) {
        *p = nz16(*p, v);
        return v;
    }
    v = (uint16_t)(v - 2);
    if (v & 0x8000)
        { cyc(18); v = 0x0000; }
    *p = nz16(*p, x);
    return cyc_then(144, fixed_colour_set(bank, v));
}

void MenuFixedColourDown_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = MenuFixedColourDown(cr->db, cr->x, &p);
    set_p(cr, (uint8_t)(p & ~(P_M | P_X)));
    cyc(64);
}

void abi_MenuFixedColourDown(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuFixedColourDown_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:C934 MenuFixedColourUp
 * WHAT IT DOES: Steps the fixed colour's intensity $18F9 up by 2 toward $1F
 * (not above it) and queues it to COLDATA for all three components.
 * HOW: Nothing happens at $1F; otherwise fixed_colour_set, with X kept on
 * the stack around FrameByteWritePush.
 * Entry: M 16-bit, X 16-bit. Exit: at $1F: A = $1F; Z, C set, N clear.
 * Otherwise A = the COLDATA value (high byte 0); X unchanged; N, Z from X;
 * C = (the raised value >= $1F); M 16-bit, X 16-bit.
 */
uint16_t MenuFixedColourUp(uint8_t bank, uint16_t x, uint8_t *p)
{
    cyc(78);
    uint16_t v = rd16(0x0018F9);

    *p = cmp16(*p, v, 0x001F);
    if (*p & P_Z)
        return v; else { cyc(48); }
    v = (uint16_t)(v + 2);
    *p = cmp16(*p, v, 0x001F);
    if (*p & P_C)
        { cyc(18); v = 0x001F; }
    *p = nz16(*p, x);
    return cyc_then(120, fixed_colour_set(bank, v));
}

void MenuFixedColourUp_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = MenuFixedColourUp(cr->db, cr->x, &p);
    set_p(cr, (uint8_t)(p & ~(P_M | P_X)));
}

void abi_MenuFixedColourUp(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuFixedColourUp_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:C95F MenuFadeUpHandler
 * WHAT IT DOES: The handler that raises the fixed colour each frame
 * (MenuFixedColourUp) until the sliding sprites' lowest position $18C1 is
 * back at $A8; then it sets +$10 of kind 3 objects to $FFFF and of kind 2
 * objects to 0 and moves on to handler $C9C4 with +$0A = 0.
 * HOW: MenuActorsSet10 around the handler change.
 * Entry: M 16-bit, X 16-bit, D = $0000; X = record. Exit: waiting: A =
 * $18C1; N, Z, C from the compare with $A8. Done: A as MenuActorsSet10;
 * Y = $FFFE; N, Z from X; C from its last compare.
 */
uint16_t MenuFadeUpHandler(uint8_t bank, uint16_t x, uint16_t *y, uint8_t *p)
{
    uint16_t a;

    MenuFixedColourUp(bank, x, p);
    cyc(120);
    a = rd16(0x0018C1);
    cyc(42);
    *p = cmp16(*p, a, 0x00A8);
    if (!(*p & P_Z))
        { cyc(46); return a; } else { cyc(78); }
    cyc(14);
    MenuActorsSet10(bank, 0x0003, 0xFFFF, p);
    cyc(46);
    wr16((uint16_t)(x + 0x08), 0xC9C4);
    cyc(22);
    wr16((uint16_t)(x + 0x0A), 0x0000);
    cyc(94);
    a = MenuActorsSet10(bank, 0x0002, 0x0000, p);
    *y = 0xFFFE;
    *p = nz16(*p, x);
    cyc(42);
    return a;
}

void MenuFadeUpHandler_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = MenuFadeUpHandler(cr->db, cr->x, &cr->y, &p);
    set_p(cr, p);
}

void abi_MenuFadeUpHandler(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuFadeUpHandler_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:C9C4 MenuFadeOutHandler
 * WHAT IT DOES: The handler that keeps raising the fixed colour
 * (MenuFixedColourUp) while +$0A counts up by 2 to $20, scaling sprite
 * palettes 5 and 1 by it (MenuSpritePalettesScale); at $20 it frees the
 * object.
 * HOW: MenuActorFree with Y = its list offset at $11FB.
 * Entry: M 16-bit, X 16-bit, D = $0000; X = record. Exit: counting: as
 * MenuSpritePalettesScale. At $20: as MenuActorFree, Y = $11FB, C set.
 */
uint16_t MenuFadeOutHandler(uint8_t bank, uint16_t *x, uint16_t *y, uint8_t *p)
{
    uint16_t a;

    MenuFixedColourUp(bank, *x, p);
    cyc(110);
    a = rd16((uint16_t)(*x + 0x0A));
    *p = cmp16(*p, a, 0x0020);
    if (*p & P_Z) {
        *y = rd16(cyc_then(70, dbw(bank, 0x11FB)));
        cyc(46);
        a = MenuActorFree(bank, *x, *y);
        *x = 0xFFFF;
        *p = (uint8_t)((*p & ~P_Z) | P_N);
        return a;
    }
    a = (uint16_t)(a + 2);
    cyc(90);
    wr16((uint16_t)(*x + 0x0A), a);
    cyc(46);
    a = MenuSpritePalettesScale(bank, a);
    *x = rd16(cyc_then(24, dbw(bank, CURRENT)));
    *y = 0xFFFF;
    *p = nz16(*p, *x);
    return a;
}

void MenuFadeOutHandler_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = MenuFadeOutHandler(cr->db, &cr->x, &cr->y, &p);
    set_p(cr, p);
    cyc(84);
}

void abi_MenuFadeOutHandler(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuFadeOutHandler_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* A choice object's first frame: its own sprite group (MenuActorCopyGroup)
   and the handler `next`. */
static void choice_start(Carried *cr, uint8_t bank, uint16_t next, uint16_t *x, uint8_t *p)
{
    cyc(46);
    MenuActorCopyGroup(cr, bank, x, p);
    cyc(46);
    wr16((uint16_t)(*x + 0x08), next);
    *p = nz16(*p, next);
}

/*
 * @routine 0B:CAE6 MenuChoiceObject00Start
 * WHAT IT DOES: First frame of a MenuChoiceObject00 object: copies its
 * sprite group (MenuActorCopyGroup), makes MenuChoiceObject00 its handler
 * and runs it.
 * HOW: +$08 = $CAEE, then falls through into MenuChoiceObject00.
 * Entry: M 16-bit, X 16-bit, D = $0000. Exit: as MenuChoiceObject00.
 */
uint16_t MenuChoiceObject00Start(Carried *cr, uint8_t bank, uint16_t *x, uint16_t *y, uint8_t *p)
{
    choice_start(cr, bank, 0xCAEE, x, p);
    return MenuChoiceObject00(bank, x, y, p);
}

void MenuChoiceObject00Start_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = MenuChoiceObject00Start(cr, cr->db, &cr->x, &cr->y, &p);
    set_p(cr, p);
}

void abi_MenuChoiceObject00Start(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuChoiceObject00Start_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:CAF8 MenuChoiceObject12Copy
 * WHAT IT DOES: First frame of a MenuChoiceObject12 object: copies its
 * sprite group (MenuActorCopyGroup) and makes MenuChoiceObject12Start
 * ($CB01) its handler for the next frame.
 * HOW: +$08 = $CB01.
 * Entry: M 16-bit, X 16-bit, D = $0000. Exit: A = $CB01; X = $11F9; N set,
 * Z clear; C clear.
 */
void MenuChoiceObject12Copy(Carried *cr, uint8_t bank, uint16_t *x, uint8_t *p)
{
    choice_start(cr, bank, 0xCB01, x, p);
    cyc(42);
}

void MenuChoiceObject12Copy_cr(Carried *cr)
{
    uint8_t p = cr->p;

    MenuChoiceObject12Copy(cr, cr->db, &cr->x, &p);
    cr->a = 0xCB01;
    set_p(cr, p);
}

void abi_MenuChoiceObject12Copy(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuChoiceObject12Copy_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:CB13 MenuChoiceObject24Start
 * WHAT IT DOES: As MenuChoiceObject00Start, for MenuChoiceObject24
 * ($CB1B).
 * HOW: +$08 = $CB1B, then falls through into MenuChoiceObject24.
 * Entry: as MenuChoiceObject00Start. Exit: as MenuChoiceObject24.
 */
uint16_t MenuChoiceObject24Start(Carried *cr, uint8_t bank, uint16_t *x, uint16_t *y, uint8_t *p)
{
    choice_start(cr, bank, 0xCB1B, x, p);
    return MenuChoiceObject24(bank, x, y, p);
}

void MenuChoiceObject24Start_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = MenuChoiceObject24Start(cr, cr->db, &cr->x, &cr->y, &p);
    set_p(cr, p);
}

void abi_MenuChoiceObject24Start(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuChoiceObject24Start_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:C46D MenuRowStart
 * WHAT IT DOES: First frame of the settings row object: copies its sprite
 * group (MenuActorCopyGroup), blinks the row and shows the saved settings
 * (MenuRowBlinkSettings), and leaves it idle (handler $C47B).
 * HOW: +$08 = $C47B, then falls through into MenuActorIdleC47B, which only
 * returns.
 * Entry: M 16-bit, X 16-bit, D = $0000. Exit: A = $C47B; X = $11F9; Y =
 * $000A; N set, Z clear; C, V as MenuRowBlinkSettings left them.
 */
void MenuRowStart(Carried *cr, uint8_t bank, uint16_t *x, uint16_t *y, uint8_t *p)
{
    cyc(46);
    MenuActorCopyGroup(cr, bank, x, p);
    cyc(0);
    MenuRowBlinkSettings(bank, x, y, p);
    *x = rd16(cyc_then(24, dbw(bank, CURRENT)));
    cyc(46);
    wr16((uint16_t)(*x + 0x08), 0xC47B);
    *p = nz16(*p, 0xC47B);
    MenuActorIdleC47B();
}

void MenuRowStart_cr(Carried *cr)
{
    uint8_t p = cr->p;

    MenuRowStart(cr, cr->db, &cr->x, &cr->y, &p);
    cr->a = 0xC47B;
    set_p(cr, p);
}

void abi_MenuRowStart(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuRowStart_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* BG1's vertical scroll = -pos, queued for the next vertical blank as two
   writes to BG1VOFS: the high byte first, then the low byte. */
static void queue_bg1_vscroll(uint8_t bank, uint16_t pos, uint32_t in)
{
    cyc(in);                            /* to the first push: each caller's own */
    uint16_t d = (uint16_t)(0 - pos);

    FrameByteWritePush(bank, (uint16_t)BG1VOFS, (uint8_t)(d >> 8));
    cyc(324);
    FrameByteWritePush(bank, (uint16_t)BG1VOFS, (uint8_t)d);
}

/* SEC; SBC from zero: N, Z, C, V of 0 - v. */
static uint8_t negate_flags(uint8_t p, uint16_t v)
{
    uint16_t d = (uint16_t)(0 - v);

    return (uint8_t)((nz16(p, d) & ~(P_C | P_V)) | (v == 0 ? P_C : (cyc(26), 0)) | ((v & d & 0x8000) ? P_V : 0));
}

/*
 * @routine 0B:CD7C MenuDropStart
 * WHAT IT DOES: The handler that waits +$0A + 1 frames and then starts a
 * drop of BG1: handler $CE19 (MenuDropBounce) with speed +$22 = 1, BG1's
 * vertical scroll set from the height +$20, and for the next vertical
 * blank HDMA channel 4 on, the sub screen window mask and colour math off,
 * BG1 and BG2 maps at VRAM $6800 and $6000 (both 64 tiles wide), BG1
 * tiles at $4000 and BG2 tiles at 0.
 * HOW: A memory decrement of +$0A; the register values go through
 * FrameByteWritePush, X kept on the stack around each.
 * Entry: M 16-bit, X 16-bit, D = $0000; X = record. Exit: waiting: A
 * unchanged; N, Z from the count. Started: A = the scroll's high byte and
 * $04; X unchanged; N, Z from X; C, V from 0 - the height; M 16-bit.
 */
uint16_t MenuDropStart(uint8_t bank, uint16_t rec, uint16_t a, uint8_t *p)
{
    static const struct { uint32_t reg; uint8_t v; } set[] = {
        {HDMAEN, 0x10}, {TSW, 0x00}, {CGWSEL, 0x00}, {CGADSUB, 0x00},
        {BG1SC, 0x69}, {BG2SC, 0x61}, {BG12NBA, 0x04}};
    cyc(22);
    uint16_t c = (uint16_t)(rd16((uint16_t)(rec + 0x0A)) - 1), h;
    unsigned i;

    cyc(14);
    wr16((uint16_t)(rec + 0x0A), c);
    if (!(c & 0x8000)) {
        cyc(46);
        *p = nz16(*p, c);
        return a;
    }
    cyc(68);
    wr16((uint16_t)(rec + 0x08), 0xCE19);
    cyc(46);
    wr16((uint16_t)(rec + 0x22), 0x0001);
    cyc(60);
    h = rd16((uint16_t)(rec + 0x20));
    *p = negate_flags(*p, h);
    queue_bg1_vscroll(bank, h, h ? 176 : 202);  /* negate_flags states 26 for a non-zero height */
    cyc(144);
    for (i = 0; i < sizeof set / sizeof set[0]; i++)
        { cyc(i ? 304 + (set[i].v != set[i - 1].v ? 16 : 0) : 176);
          FrameByteWritePush(bank, (uint16_t)set[i].reg, set[i].v); }
    cyc(166);
    *p = nz16(*p, rec);
    return (uint16_t)((uint16_t)(0 - h) & 0xFF00) | 0x04;
}

void MenuDropStart_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = MenuDropStart(cr->db, cr->x, cr->a, &p);
    set_p(cr, p);
    cyc(42);
}

void abi_MenuDropStart(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuDropStart_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void SoundPort0QueuePush(uint8_t command);

/*
 * @routine 0B:CE19 MenuDropBounce
 * WHAT IT DOES: One frame of BG1 dropping into place and bouncing: the
 * speed +$22 grows by 1 every second frame and moves the height +$20
 * (negative is above the rest position); reaching the rest position, the
 * object bounces back with the speed reversed and reduced by 2 and sound
 * $19 plays; once the speed is below 2 there, the handler becomes $CE89
 * (idle). BG1's vertical scroll follows -height and +$06 = height + $4D.
 * HOW: MenuActorAnimateFrame first; +$0A toggles 0/1 for the every second
 * frame; the scroll goes through queue_bg1_vscroll.
 * Entry: M 16-bit, X 16-bit, D = $0000; X = record. Exit: moving: A =
 * +$06; N, Z, C, V from its add; X = the record after the animation step;
 * M 16-bit, X 16-bit. Stopped: A = $CE89; X = $11F9; N set, Z clear; C, V
 * from the height's add.
 */
uint16_t MenuDropBounce(uint8_t bank, uint16_t *x, uint8_t *p)
{
    uint16_t a, h, v;
    unsigned s;

    MenuActorAnimateFrame(bank, x, p);
    cyc(22);
    a = (uint16_t)(rd16((uint16_t)(*x + 0x0A)) + 1) & 0x0001;
    cyc(60);
    wr16((uint16_t)(*x + 0x0A), a);
    if (!a)
        { cyc(38); wr16((uint16_t)(*x + 0x22), cyc_then(60, (uint16_t)(rd16((uint16_t)(*x + 0x22)) + 1))); } else { cyc(22); }
    cyc(22);
    h = rd16((uint16_t)(*x + 0x20));
    cyc(36);
    v = rd16((uint16_t)(*x + 0x22));
    s = (unsigned)h + v;
    *p = (uint8_t)((*p & ~(P_C | P_V)) | (s > 0xFFFF ? P_C : 0) | ((~(h ^ v) & (h ^ (uint16_t)s) & 0x8000) ? P_V : 0));
    cyc(22);
    wr16((uint16_t)(*x + 0x20), (uint16_t)s);
    cyc(22);
    rd16((uint16_t)(*x + 0x20));        /* LDA again for the BMI */
    if (!(s & 0x8000)) {
        cyc(38);
        a = (uint16_t)(rd16((uint16_t)(*x + 0x22)) - 2);
        if (a & 0x8000) {
            *x = rd16(cyc_then(74, dbw(bank, CURRENT)));
            cyc(46);
            wr16((uint16_t)(*x + 0x08), 0xCE89);
            *p = nz16(*p, 0xCE89);
            return 0xCE89;
        }
        cyc(104);
        wr16((uint16_t)(*x + 0x22), (uint16_t)(0 - a));
        cyc(22);
        wr16((uint16_t)(*x + 0x20), 0x0000);
        cyc(100);
        SoundPort0QueuePush(0x19);
    }
    cyc(82);                            /* BMI taken or REP after the sound; LDA #0, SEC, SBC */
    h = rd16((uint16_t)(*x + 0x20));
    queue_bg1_vscroll(bank, h, 202);
    s = (unsigned)h + 0x4D;
    a = (uint16_t)s;
    *p = (uint8_t)((nz16(*p, a) & ~(P_C | P_V | P_M | P_X)) | (s > 0xFFFF ? P_C : 0) | ((~h & a & 0x8000) ? P_V : 0));
    cyc(264);
    wr16((uint16_t)(*x + 0x06), a);
    return a;
}

void MenuDropBounce_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = MenuDropBounce(cr->db, &cr->x, &p);
    set_p(cr, p);
    cyc(42);
}

void abi_MenuDropBounce(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuDropBounce_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* A list of PPU register writes for the next vertical blank, in order. */
struct reg_write {
    uint32_t reg;
    uint8_t v;
    uint8_t lda;                        /* 1: an LDA # even though the value is unchanged */
};

static void queue_regs(uint8_t bank, const struct reg_write *w, unsigned n)
{
    unsigned i;

    /* between pushes: FrameByteWritePush's return (108), PLX, the LDA #
       (only when the value changes: 16), SEP, REP, PHX, LDX #, JSL */
    cyc(22);
    for (i = 0; i < n; i++)
        { cyc(i ? 304 + (w[i].v != w[i - 1].v || w[i].lda ? 16 : 0) : 176); FrameByteWritePush(bank, (uint16_t)w[i].reg, w[i].v); }
    cyc(190);
}

/*
 * @routine 0B:CF27 MenuDropAway
 * WHAT IT DOES: One frame of BG1 falling out of view: the speed +$22 grows
 * by 1 every second frame and moves the height +$20 down; from $1E on,
 * HDMA channel 5 alone is switched on each frame; at $DC the handler
 * becomes $CE89 (idle). BG1's vertical scroll follows -height and +$06 =
 * height + $4D.
 * HOW: As MenuDropBounce without the bounce; the HDMA enable goes through
 * FrameByteWritePush.
 * Entry: M 16-bit, X 16-bit, D = $0000; X = record. Exit: moving: A =
 * +$06; N, Z, C, V from its add; X = the record after the animation step;
 * M 16-bit, X 16-bit. Gone: A = $CE89; X = $11F9; N, C set, Z clear; V
 * from the height's add.
 */
uint16_t MenuDropAway(uint8_t bank, uint16_t *x, uint8_t *p)
{
    uint16_t a, h, v;
    unsigned s;

    MenuActorAnimateFrame(bank, x, p);
    cyc(22);
    a = (uint16_t)(rd16((uint16_t)(*x + 0x0A)) + 1) & 0x0001;
    cyc(60);
    wr16((uint16_t)(*x + 0x0A), a);
    if (!a)
        { cyc(38); wr16((uint16_t)(*x + 0x22), cyc_then(60, (uint16_t)(rd16((uint16_t)(*x + 0x22)) + 1))); } else { cyc(22); }
    cyc(22);
    h = rd16((uint16_t)(*x + 0x20));
    cyc(36);
    v = rd16((uint16_t)(*x + 0x22));
    s = (unsigned)h + v;
    *p = (uint8_t)((*p & ~(P_C | P_V)) | (s > 0xFFFF ? P_C : 0) | ((~(h ^ v) & (h ^ (uint16_t)s) & 0x8000) ? P_V : 0));
    h = (uint16_t)s;
    cyc(22);
    wr16((uint16_t)(*x + 0x20), h);
    cyc(22);
    rd16((uint16_t)(*x + 0x20));        /* LDA again for the BMI */
    if (!(h & 0x8000)) {
        if (h >= 0x00DC) {
            *x = rd16(cyc_then(86, dbw(bank, CURRENT)));
            cyc(46);
            wr16((uint16_t)(*x + 0x08), 0xCE89);
            *p = (uint8_t)(nz16(*p, 0xCE89) | P_C);
            return 0xCE89;
        }
        if (h >= 0x001E)
            { cyc(294); FrameByteWritePush(bank, (uint16_t)HDMAEN, 0x20); cyc(226); }
        else
            cyc(162);                   /* BMI, CMP, BCS, CMP, BCC taken; LDA #0, SEC, SBC */
    } else
        cyc(82);                        /* BMI taken; LDA #0, SEC, SBC */
    h = rd16((uint16_t)(*x + 0x20));
    queue_bg1_vscroll(bank, h, 202);
    s = (unsigned)h + 0x4D;
    a = (uint16_t)s;
    *p = (uint8_t)((nz16(*p, a) & ~(P_C | P_V | P_M | P_X)) | (s > 0xFFFF ? P_C : 0) | ((~h & a & 0x8000) ? P_V : 0));
    cyc(264);
    wr16((uint16_t)(*x + 0x06), a);
    return a;
}

void MenuDropAway_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = MenuDropAway(cr->db, &cr->x, &p);
    set_p(cr, p);
    cyc(42);
}

void abi_MenuDropAway(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuDropAway_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:D07C MenuDropAwayFinish
 * WHAT IT DOES: Once BG1 has fallen away: for the next vertical blank, BG1's
 * vertical scroll 0, HDMA and the window masks off, colour math adding the
 * sub screen to BG1 only, BG2 on the sub screen, maps BG2 at $6400 and BG1
 * at $6000 (64 x 32), BG1 tiles at 0 and BG2 tiles at $2000, BG1-BG3 and
 * sprites on the main screen. Then the object being run ($11F9) and every
 * kind $11 object get +$10 = $FFFF, and the object counts 64 frames in
 * +$0A under handler $D14A (MenuCrossFadeOut).
 * HOW: FrameByteWritePush for each register; MenuActorsSet10 for the kind
 * $11 objects.
 * Entry: D = $0000. Exit: A = $40; X = $11F9; Y = $FFFE; N, Z clear; C
 * from MenuActorsSet10's last compare; M 16-bit, X 16-bit.
 */
uint16_t MenuDropAwayFinish(uint8_t bank, uint8_t *p)
{
    static const struct reg_write set[] = {
        {BG1VOFS, 0x00}, {BG1VOFS, 0x00}, {HDMAEN, 0x00, 1}, {TSW, 0x00},
        {TMW, 0x00}, {CGADSUB, 0x01}, {CGWSEL, 0x02}, {TS, 0x02},
        {BG2SC, 0x65}, {BG1SC, 0x61}, {BG12NBA, 0x20}, {TM, 0x17}};
    uint16_t x;

    cyc(68);
    queue_regs(bank, set, sizeof set / sizeof set[0]);
    x = rd16(dbw(bank, CURRENT));
    cyc(94);
    MenuActorsSet10(bank, 0x0011, 0xFFFF, p);
    cyc(46);
    wr16((uint16_t)(x + 0x10), 0xFFFF);
    cyc(46);
    wr16((uint16_t)(x + 0x08), 0xD14A);
    cyc(46);
    wr16((uint16_t)(x + 0x0A), 0x0040);
    *p = (uint8_t)(nz16(*p, 0x0040) & ~(P_M | P_X));
    cyc(42);
    return x;
}

void MenuDropAwayFinish_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->x = MenuDropAwayFinish(cr->db, &p);
    cr->a = 0x0040;
    cr->y = 0xFFFE;
    set_p(cr, p);
}

void abi_MenuDropAwayFinish(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuDropAwayFinish_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:D068 MenuDropAwayCheck
 * WHAT IT DOES: Finishes the drop (MenuDropAwayFinish) once the kind $13
 * object has gone idle (its handler is $CE89).
 * HOW: MenuActorFind, then a compare of its +$08.
 * Entry: M 16-bit, X 16-bit, D = $0000. Exit: not yet: A = that object's
 * handler; X = the object; Y from the search; N, Z, C from the compare.
 * Finished: as MenuDropAwayFinish.
 */
uint16_t MenuDropAwayCheck(uint8_t bank, uint16_t *x, uint16_t *y, uint8_t *p)
{
    cyc(70);
    uint16_t a;

    MenuActorFind(bank, 0x0013, x, y, p);
    cyc(22);
    a = rd16((uint16_t)(*x + 0x08));
    *p = cmp16(*p, a, 0xCE89);
    if (!(*p & P_Z))
        { cyc(46); return a; }
    *x = rd16(cyc_then(64, dbw(bank, CURRENT)));
    *x = MenuDropAwayFinish(bank, p);
    *y = 0xFFFE;
    return 0x0040;
}

void MenuDropAwayCheck_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = MenuDropAwayCheck(cr->db, &cr->x, &cr->y, &p);
    set_p(cr, p);
    cyc(42);
}

void abi_MenuDropAwayCheck(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuDropAwayCheck_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

uint16_t MenuPictureDC8CGrid58(uint8_t bank);

/*
 * @routine 0B:B814 MenuSelectArrive
 * WHAT IT DOES: The handler that brings the selection screen in from the
 * side: the speed +$20 grows by 1 a frame and moves the position +$22 (it
 * starts at -$100) toward 0, which sets BG1's horizontal scroll, the
 * sliding sprites' heights ($18BF = position, $18C1 = position + $A8),
 * $11ED and the byte at $7F:1354 (= $100 - position, at most $1FF). On
 * arrival (position >= 0): sound $19, handler $B952 with +$0A = 0, and for
 * the next vertical blank HDMA channels 5-7 on, window 1 at 32-136 on BG1,
 * BG2 and (inverted) the colour window, BG1 on the sub screen and in both
 * window masks, and colour math subtracting the fixed colour (white) from
 * BG1 except inside the colour window; then the picture with the tile grid
 * from $58 is drawn (MenuPictureDC8CGrid58) and $800 bytes of it queued to
 * VRAM $6400.
 * HOW: The scroll is queued as the position's high byte, then its low byte.
 * MenuPictureDC8CGrid58 leaves X = $0700, so on arrival the store of 0
 * meant for the position goes to $0722, and the scroll, heights and
 * $7F:1354 are made from $0722 (0); the object's +$22 keeps its value.
 * Entry: D = $0000; X = record. Exit: A = $100 - position, at most $1FF;
 * X unchanged, or $0700 on arrival; N, Z from the compare with $1FF (or
 * from $1FF); C from the compare; V from the subtraction; M 16-bit, X
 * 16-bit.
 */
uint16_t MenuSelectArrive(uint8_t bank, uint16_t *x, uint8_t *p)
{
    static const struct reg_write set[] = {
        {HDMAEN, 0xE0}, {W12SEL, 0x22}, {WOBJSEL, 0x30}, {WH0, 0x20},
        {WH1, 0x88}, {TMW, 0x01}, {TSW, 0x01}, {TS, 0x01},
        {CGADSUB, 0x81}, {CGWSEL, 0x20}, {COLDATA, 0xFF}};
    uint16_t rec = *x, speed, pos, a, d;

    *p &= (uint8_t)~(P_M | P_X);
    cyc(44);
    speed = (uint16_t)(rd16((uint16_t)(rec + 0x20)) + 1);
    cyc(14);
    wr16((uint16_t)(rec + 0x20), speed);
    cyc(22);
    pos = (uint16_t)(rd16((uint16_t)(rec + 0x22)) + speed);
    cyc(74);
    wr16((uint16_t)(rec + 0x22), pos);
    if (!(pos & 0x8000)) {
        cyc(122);
        SoundPort0QueuePush(0x19);
        cyc(68);
        wr16((uint16_t)(rec + 0x08), 0xB952);
        cyc(22);
        wr16((uint16_t)(rec + 0x0A), 0x0000);
        queue_regs(bank, set, sizeof set / sizeof set[0]);
        MenuPictureDC8CGrid58(bank);
        rec = 0x0700;
        cyc(78);
        wr16(0x0016B1, 0x0800);
        cyc(56);
        wr16(0x0016B5, 0x0070);
        cyc(56);
        wr16(0x0016B3, 0x3000);
        cyc(56);
        wr16(0x0016B7, 0x6400);
        (cyc(62), VramDmaQueueAdd());
        cyc(112);
        wr16((uint16_t)(rec + 0x22), 0x0000);
    } else { cyc(46); }
    cyc(22);
    pos = rd16((uint16_t)(rec + 0x22));
    wr16(dbw(bank, 0x11ED), cyc_then(54, pos));
    cyc(32);
    wr16(0x0018BF, pos);
    cyc(70);
    wr16(0x0018C1, (uint16_t)(pos + 0x00A8));
    cyc(238);
    FrameByteWritePush(bank, (uint16_t)BG1HOFS, (uint8_t)(pos >> 8));
    cyc(324);
    FrameByteWritePush(bank, (uint16_t)BG1HOFS, (uint8_t)pos);
    cyc(226);
    a = rd16((uint16_t)(rec + 0x22));
    d = (uint16_t)(0x0100 - a);
    *p = (uint8_t)((*p & ~P_V) | (((0x0100 ^ a) & (0x0100 ^ d) & 0x8000) ? P_V : 0));
    *p = cmp16(*p, d, 0x01FF);
    if (*p & P_C) {
        cyc(18);
        d = 0x01FF;
        *p = nz16(*p, d);
    }
    cyc(100);
    wr8(0x7F1354, (uint8_t)d);
    *x = rec;
    cyc(64);
    return d;
}

void MenuSelectArrive_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = MenuSelectArrive(cr->db, &cr->x, &p);
    set_p(cr, p);
}

void abi_MenuSelectArrive(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuSelectArrive_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:B506 MenuActorNew
 * WHAT IT DOES: Takes a free object record (MenuActorAlloc) and fills it
 * from the template at $0B:EA0F; X = $FFFF when none is free (no script
 * runs out).
 * HOW: 44 words, $0B:EA0F-$0B:EA66, stored through the data bank from the
 * record on: $58 bytes, twice the record size, so the copy also runs over
 * the next record. P is saved and restored.
 * Entry: X 16-bit. Exit: A = the template's last word, or as
 * MenuActorAlloc when none is free; X = the record or $FFFF; P unchanged.
 */
uint16_t MenuActorNew(uint8_t bank, uint16_t *a)
{
    cyc(90);
    uint8_t p = 0;  /* the flags it leaves are discarded by the PLP */
    uint16_t rec = MenuActorAlloc(bank, &p), i;

    if (rec == 0xFFFF)
        { cyc(140); return rec; } else { cyc(46); }
    for (i = 0; i < 0x58; i = (uint16_t)(i + 2))
        wr16(dbw(bank, (uint16_t)(rec + i)), rd16(cyc_then(78, 0x0BEA0F + i)));
    cyc(70);
    *a = rd16(0x0BEA0F + 0x56);
    return rec;
}

void MenuActorNew_cr(Carried *cr)
{
    cr->x = MenuActorNew(cr->db, &cr->a);
}

void abi_MenuActorNew(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuActorNew_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:E4B5 MenuActorSpawn
 * WHAT IT DOES: Makes a new object (MenuActorNew) and applies the spawn
 * entry list at $0B:E547 + X to it: each entry is a field offset and a
 * value, ended by a negative offset. X = $FFFF when no record is free (no
 * script runs out).
 * HOW: The record is kept at $1209; the field writes go through the data
 * bank. The failure return ($E4E3, LDX #$FFFF) follows the routine.
 * Entry: M 16-bit, X 16-bit; X = the entry list's offset. Exit: A = the
 * terminating offset; X = the record; Y = the last field written (the list
 * offset if none); N, Z from X; C, V from the last step of 4 (C clear if
 * none). None free: A as MenuActorAlloc; X = $FFFF; Y = the list offset; N,
 * C set, Z clear.
 */
uint16_t MenuActorSpawn(uint8_t bank, uint16_t list, uint16_t *a, uint16_t *y, uint8_t *p)
{
    cyc(60);
    uint16_t rec, x, f, n;

    *y = list;
    rec = MenuActorNew(bank, a);
    if (rec == 0xFFFF) {
        cyc(136);
        *p = (uint8_t)((*p & ~P_Z) | P_N | P_C);
        return rec;
    }
    *p &= (uint8_t)~P_C;
    wr16(dbw(bank, 0x1209), cyc_then(70, rec));
    for (x = list;; x = n) {
        cyc(108);
        f = rd16(0x0BE547 + x);
        if (f & 0x8000)
            break;
        *y = (uint16_t)(f + rd16(cyc_then(8, dbw(bank, 0x1209))));
        wr16(dbw(bank, *y), rd16(cyc_then(92, 0x0BE549 + x)));
        n = (uint16_t)(x + 4);
        *p = (uint8_t)((*p & ~(P_C | P_V)) | (n < x ? P_C : (cyc(74), 0)) | ((~(x ^ 4) & (x ^ n) & 0x8000) ? P_V : 0));
    }
    *a = f;
    rec = rd16(dbw(bank, 0x1209));
    *p = nz16(*p, rec);
    cyc(42);
    return rec;
}

void MenuActorSpawn_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->x = MenuActorSpawn(cr->db, cr->x, &cr->a, &cr->y, &p);
    set_p(cr, p);
}

void abi_MenuActorSpawn(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuActorSpawn_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void SystemHaltSpawn(void);

/*
 * @routine 0B:E458 MenuActorSpawnList
 * WHAT IT DOES: Spawns every object of spawn list X: the word list at
 * $0B:E4E7 + X names each object's entry list (MenuActorSpawn), ended by a
 * negative word. A full object pool stops the game (SystemHaltSpawn); no
 * script fills it.
 * HOW: The position in the list is kept at $11FD; P is saved and restored.
 * Entry: X 16-bit; X = the list's offset. Exit: A = the terminating word;
 * X = $11FD (the terminator's offset); Y as the last MenuActorSpawn; P
 * unchanged.
 */
uint16_t MenuActorSpawnList(uint8_t bank, uint16_t list, uint16_t *a, uint16_t *y)
{
    uint16_t x, e;
    uint8_t p = 0;  /* the flags it leaves are discarded by the PLP */

    wr16(dbw(bank, 0x11FD), cyc_then(68, list));
    for (;;) {
        x = rd16(cyc_then(24, dbw(bank, 0x11FD)));
        cyc(124);
        e = rd16(0x0BE4E7 + x);
        if (e & 0x8000)
            { cyc(18); break; }
        if (MenuActorSpawn(bank, e, a, y, &p) == 0xFFFF)
            SystemHaltSpawn();
        x = rd16(cyc_then(64, dbw(bank, 0x11FD)));
        wr16(dbw(bank, 0x11FD), cyc_then(14, (uint16_t)(x + 1)));
        x = rd16(cyc_then(24, dbw(bank, 0x11FD)));
        wr16(dbw(bank, 0x11FD), cyc_then(14, (uint16_t)(x + 1)));
        cyc(22);
    }
    *a = e;
    return x;
}

void MenuActorSpawnList_cr(Carried *cr)
{
    cr->x = MenuActorSpawnList(cr->db, cr->x, &cr->a, &cr->y);
}

void abi_MenuActorSpawnList(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuActorSpawnList_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:D025 MenuSelectConfirm
 * WHAT IT DOES: When B or Start has been pressed ($118B bits 15 and 12):
 * sets +$10 of every kind $11 object to $FFFF, starts the kind $13 object
 * (BG1) moving away (handler $CE8A, speed -10), moves the object being run
 * on to MenuDropAwayCheck ($D068) with +$10 = $FFFF, and plays sound $46.
 * HOW: MenuActorsSet10, MenuActorFind, then the sound with 8-bit A.
 * Entry: M 16-bit, X 16-bit, D = $0000. Exit: not pressed: A = 0; Z set, N
 * clear. Confirmed: A = $FF46; X = $11F9; Y from the search; N, Z from the
 * data bank; C from the search; M 16-bit.
 */
int MenuSelectConfirm(uint8_t bank, uint16_t *x, uint16_t *y, uint8_t *p)
{
    uint16_t pressed = rd16(cyc_then(70, dbw(bank, 0x118B))) & 0x9000;

    if (!pressed) {
        cyc(112);
        *p = nz16(*p, pressed);
        return 0;
    } else { cyc(140); }
    MenuActorsSet10(bank, 0x0011, 0xFFFF, p);
    cyc(70);
    MenuActorFind(bank, 0x0013, x, y, p);
    cyc(46);
    wr16((uint16_t)(*x + 0x08), 0xCE8A);
    cyc(46);
    wr16((uint16_t)(*x + 0x22), 0xFFF6);
    *x = rd16(cyc_then(24, dbw(bank, CURRENT)));
    cyc(46);
    wr16((uint16_t)(*x + 0x08), 0xD068);
    cyc(46);
    wr16((uint16_t)(*x + 0x10), 0xFFFF);
    cyc(100);
    SoundPort0QueuePush(0x46);
    *p = (uint8_t)(nz8(*p, bank) & ~P_M);
    cyc(64);
    return 1;
}

void MenuSelectConfirm_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = MenuSelectConfirm(cr->db, &cr->x, &cr->y, &p) ? 0xFF46 : 0x0000;
    set_p(cr, p);
}

void abi_MenuSelectConfirm(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuSelectConfirm_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:D061 MenuDropAwayFinishCurrent
 * WHAT IT DOES: MenuDropAwayFinish with X = the object being run. Nothing
 * the scripts run reaches it: it sits after MenuSelectConfirm's last RTS
 * and no traced jump or call targets it.
 * HOW: LDX $11F9, then MenuDropAwayFinish.
 * Entry: as MenuDropAwayFinish. Exit: as MenuDropAwayFinish.
 */
void MenuDropAwayFinishCurrent(uint8_t bank, uint8_t *p)
{
    MenuDropAwayFinish(bank, p);
}

void MenuDropAwayFinishCurrent_cr(Carried *cr)
{
    (cyc(40), MenuDropAwayFinish_cr(cr));
    cyc(42);
}

void abi_MenuDropAwayFinishCurrent(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuDropAwayFinishCurrent_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* ROR of a 16-bit word in memory: c in at bit 15, bit 0 out to c. */
static void ror_mem(uint32_t addr, int *c)
{
    cyc(24);
    uint16_t v = rd16(addr);
    int out = v & 1;

    cyc(14);
    wr16(addr, (uint16_t)((v >> 1) | (*c ? 0x8000 : 0)));
    *c = out;
}

/* After a sprite's two high-table bits are shifted into $11E1: every
   eighth sprite the word goes to the high table at $0575 + $1207. */
static void oam_high_step(void)
{
    cyc(24);
    uint16_t n = (uint16_t)(rd16(0x0011DF) - 1), y;

    cyc(14);
    wr16(0x0011DF, n);
    if (!(n & 0x8000))
        { cyc(22); return; }
    cyc(64);
    wr16(0x0011DF, 0x0007);
    cyc(24);
    y = rd16(0x001207);
    cyc(24);
    wr16(OAM_BUFFER_HIGH + y, cyc_then(30, rd16(0x0011E1)));
    cyc(52);
    wr16(0x001207, (uint16_t)(y + 2));
}

/*
 * @routine 0B:B64A MenuOamBuild
 * WHAT IT DOES: Builds the sprite table at $0375 (and its high table at
 * $0575) from the menu objects, last live object first, relative to the
 * scroll position ($11ED, $11EF); entries left over from the previous frame
 * are moved off screen ($EFEF) and their high bits set to $5555.
 * HOW: An object with +$10 = $FFFF is skipped. +$12 = 0 is a single
 * sprite from the record (X +$02, Y +$06, tile +$0E | +$10, size bit 0 of
 * +$0C); +$12 = $FFFF is skipped; otherwise +$12 is a sprite group at
 * $7F:1356 (a count, then 8-byte entries: X, Y, tile and attributes, size
 * in bit 0) placed at the object's position. A sprite is kept when X is
 * -16 to 255 and Y -16 to 223; negative X sets its X high bit. Each sprite
 * shifts its X high bit and size bit into $11E1 (two RORs each); every
 * eighth sprite the word is stored; the last partial word is padded with
 * X-high pairs. $1205 and $1207 count the bytes used in the two tables,
 * $1201 and $1203 hold the previous frame's counts, $11DF the sprites
 * left in the current high word. A full table ($200 bytes) ends the
 * object. The single-sprite path, negative positions and a full table are
 * not reached by any script. DB is 0 throughout.
 * Entry: any widths. Exit: A = the high table's byte count; X = the last
 * record, group offset or entry offset used; Y = the end of the high
 * table fill; P unchanged.
 */
uint16_t MenuOamBuild(uint16_t *x, uint16_t *y)
{
    uint16_t i, rec, n, sx, sy, w;
    uint32_t t;
    int c;

    cyc(178);
    wr16(0x001205, 0x0000);
    cyc(24);
    wr16(0x001207, 0x0000);
    cyc(48);
    wr16(0x0011DF, 0x0007);
    cyc(24);
    wr16(0x0011E1, 0x0000);
    cyc(24);
    /* into the body: BPL 74, or BMI and BRL 98, to the STY */
    i = (uint16_t)(rd16(0x0011F7) - 2);
    if (i & 0x8000)
        cyc(24);
    for (t = 74; cyc_loop(t, !(i & 0x8000));
         i = (uint16_t)(rd16(cyc_then(46, 0x0011FB)) - 2), t = 98) {
        wr16(0x0011FB, i);
        cyc(30);
        rec = *x = rd16(0x001239 + i);
        cyc(22);
        if (rd16((uint16_t)(rec + 0x10)) == 0xFFFF)
            { cyc(38); continue; }
        cyc(58);
        n = rd16((uint16_t)(rec + 0x12));
        if (n == 0x0000) {
            /* one sprite from the record */
            cyc(46);
            wr16(0x0011F1, 0x0000);
            cyc(22);
            wr16(0x0011F3, cyc_then(24, rd16((uint16_t)(rec + 0x0C))));
            cyc(24);
            *y = rd16(0x001205);
            if (*y == 0x0200)
                { cyc(48); continue; }
            cyc(68);
            sx = (uint16_t)(rd16((uint16_t)(rec + 0x02)) - rd16(cyc_then(38, 0x0011ED)));
            if (sx & 0x8000) {
                if (sx < 0xFFF0)
                    { cyc(64); continue; }
                cyc(86);
                wr16(0x0011F1, cyc_then(14, (uint16_t)(rd16(0x0011F1) + 1)));
                cyc(52);
            } else if (sx >= 0x0100) {
                cyc(70);
                continue;
            } else { cyc(98); }
            wr16(OAM_BUFFER + *y, sx);
            cyc(22);
            sy = (uint16_t)(rd16((uint16_t)(rec + 0x06)) - rd16(cyc_then(38, 0x0011EF)));
            if (sy & 0x8000) {
                if (sy < 0xFFF0)
                    { cyc(58); continue; }
                c = 1;
                cyc(92);
            } else {
                if (sy >= 0x00E0)
                    { cyc(70); continue; }
                c = 0;
                cyc(98);
            }
            wr16(OAM_BUFFER + 1 + *y, sy);
            cyc(22);
            wr16(OAM_BUFFER + 2 + *y, cyc_then(30, (uint16_t)(rd16((uint16_t)(rec + 0x0E)) | rd16(cyc_then(22, (uint16_t)(rec + 0x10))))));
            *y = (uint16_t)(*y + 4);
            cyc(80);
            wr16(0x001205, *y);
            ror_mem(0x0011F1, &c);
            ror_mem(0x0011E1, &c);
            ror_mem(0x0011F3, &c);
            ror_mem(0x0011E1, &c);
            oam_high_step();
            cyc(2);
            continue;
        }
        if (n == 0xFFFF)
            { cyc(64); continue; }
        /* a sprite group */
        cyc(110);
        wr16(0x0011E3, n);
        cyc(22);
        wr16(0x0011FD, cyc_then(24, (uint16_t)(rd16((uint16_t)(rec + 0x02)) - rd16(cyc_then(38, 0x0011ED)))));
        cyc(22);
        wr16(0x0011FF, cyc_then(24, (uint16_t)(rd16((uint16_t)(rec + 0x06)) - rd16(cyc_then(38, 0x0011EF)))));
        cyc(24);
        *x = rd16(0x0011E3);
        cyc(32);
        wr16(0x0011E3, cyc_then(24, rd16(0x7F1356 + *x)));
        *x = (uint16_t)(*x + 2);
        for (;;) {
            cyc(52);
            wr16(0x0011F1, 0x0000);
            cyc(24);
            *y = rd16(0x001205);
            if (*y == 0x0200)
                { cyc(48); break; }
            cyc(78);
            sx = (uint16_t)(rd16(0x7F1356 + *x) + rd16(cyc_then(38, 0x0011FD)));
            if (sx & 0x8000) {
                if (sx < 0xFFF0)
                    { cyc(62); goto next; }
                cyc(80);
                wr16(0x0011F1, cyc_then(14, (uint16_t)(rd16(0x0011F1) + 1)));
                cyc(52);
            } else if (sx >= 0x0100) {
                cyc(68);
                goto next;
            } else { cyc(92); }
            wr16(OAM_BUFFER + *y, sx);
            cyc(32);
            sy = (uint16_t)(rd16(0x7F1358 + *x) + rd16(cyc_then(38, 0x0011FF)));
            if (sy & 0x8000 ? sy < 0xFFF0 : sy >= 0x00E0)
                { cyc(sy & 0x8000 ? 78 : 68); goto next; }
            cyc(92);
            wr16(OAM_BUFFER + 1 + *y, sy);
            *y = (uint16_t)(*y + 2);
            cyc(60);
            wr16(OAM_BUFFER + *y, cyc_then(30, rd16(0x7F135A + *x)));
            *y = (uint16_t)(*y + 2);
            cyc(52);
            wr16(0x001205, *y);
            cyc(24);
            w = rd16(0x0011F1);
            c = w & 1;
            cyc(14);
            wr16(0x0011F1, (uint16_t)(w >> 1));
            ror_mem(0x0011E1, &c);
            cyc(32);
            c = rd16(0x7F135C + *x) & 1;
            cyc(14);
            ror_mem(0x0011E1, &c);
            cyc(24);
            w = (uint16_t)(rd16(0x0011DF) - 1);
            cyc(14);
            wr16(0x0011DF, w);
            if (w & 0x8000) {
                /* here the word is stored before $11DF is reset */
                cyc(40);
                *y = rd16(0x001207);
                cyc(24);
                wr16(OAM_BUFFER_HIGH + *y, cyc_then(30, rd16(0x0011E1)));
                *y = (uint16_t)(*y + 2);
                cyc(52);
                wr16(0x001207, *y);
                cyc(48);
                wr16(0x0011DF, 0x0007);
            } else { cyc(22); }
        next:
            *x = (uint16_t)(*x + 8);
            cyc(90);
            w = (uint16_t)(rd16(0x0011E3) - 1);
            cyc(14);
            wr16(0x0011E3, w);
            if (w & 0x8000)
                break; else { cyc(18); }
        }
    }
    cyc(74);
    *y = rd16(0x0011DF);
    if (*y != 0x0007) {
        cyc(64);
        uint16_t a = rd16(0x0011E1);

        do {
            cyc(78);
            a = (uint16_t)((a >> 1) | 0x8000);
            a = (uint16_t)(a >> 1);
            *y = (uint16_t)(*y - 1);
        } while (!(*y & 0x8000));
        cyc(18);
        *y = rd16(0x001207);
        cyc(30);
        wr16(OAM_BUFFER_HIGH + *y, a);
        *y = (uint16_t)(*y + 2);
        cyc(52);
        wr16(0x001207, *y);
    } else { cyc(46); }
    cyc(24);
    *y = rd16(0x001205);
    cyc(24);
    if (*y < rd16(0x001201)) {
        cyc(18);
        do {
            cyc(52);
            wr16(OAM_BUFFER + *y, 0xEFEF);
            *y = (uint16_t)(*y + 2);
        } while (*y < rd16(cyc_then(52, 0x001201)));
    } else { cyc(6); }
    cyc(40);
    wr16(0x001201, cyc_then(24, rd16(0x001205)));
    cyc(24);
    *y = rd16(0x001207);
    cyc(24);
    if (*y < rd16(0x001203)) {
        cyc(18);
        do {
            cyc(52);
            wr16(OAM_BUFFER_HIGH + *y, 0x5555);
            *y = (uint16_t)(*y + 2);
        } while (*y < rd16(cyc_then(52, 0x001203)));
    } else { cyc(6); }
    cyc(40);
    w = rd16(0x001207);
    cyc(24);
    wr16(0x001203, w);
    cyc(100);
    return w;
}

void MenuOamBuild_cr(Carried *cr)
{
    cr->a = MenuOamBuild(&cr->x, &cr->y);
}

void abi_MenuOamBuild(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuOamBuild_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:B450 MenuActorsRun
 * WHAT IT DOES: Runs every live menu object's handler once, from the last
 * in the live list to the first.
 * HOW: With DB = 0: $11FB = the list offset and $11F9 = the record, then
 * JSR (+$08) in bank $0B, the handler running with X = the record and Y =
 * the offset; the offset is reloaded from $11FB afterwards, so a handler
 * may free its object. P and DB are saved and restored.
 * Entry: any widths. Exit: A, X as the last handler left them (A's low
 * byte 0 when there are none); Y = the last offset - 2; P and DB
 * unchanged.
 */
void MenuActorsRun(Carried *cr)
{
    uint8_t p = cr->p, db = cr->db;
    uint16_t y;
    uint32_t t;

    cr->a &= 0xFF00;
    cr->db = 0x00;
    set_p(cr, (uint8_t)((cr->p & ~(P_M | P_X | P_N)) | P_Z));
    cyc(200);
    y = (uint16_t)(rd16(LIVE_LEN) - 2);
    /* into the body: 68 the first time, 74 going round */
    for (t = 68; cyc_loop(t, !(y & 0x8000)); t = 74) {
        wr16(0x0011FB, y);
        cyc(30);
        cr->x = rd16(LIVE_LIST + y);
        cyc(24);
        wr16(CURRENT, cr->x);
        cr->y = y;
        cr->p = nz16(cr->p, cr->x);
        cyc(46);
        carried_call(cr, 0x0B0000 | rd16(0x0B0000 | (uint16_t)(cr->x + 0x08)), 0);
        cyc(24);
        y = (uint16_t)(rd16(0x0011FB) - 2);
    }
    cyc(144);
    cr->y = y;
    cr->db = db;
    set_p(cr, p);
}

void abi_MenuActorsRun(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuActorsRun(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

