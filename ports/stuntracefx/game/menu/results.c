/*
 * results.c: the objects of the screens shown after a championship race
 * (the standings), run once a frame through their +$08 like the other menu
 * objects (handlers.c): X = the object, M 16-bit, X 16-bit, D = $0000, so
 * the object's fields are direct page offsets indexed by X. Fields used
 * here: +$08 (the handler), +$0A (a count), +$10, +$12 (the sprite group),
 * +$14 (the animation) and +$22 (a step). $0DD6 is the race phase (8 once
 * the championship is over) and $0FAC the leader's place.
 */
#include "machine.h"
#include "cpu_ops.h"

#include "alu.h"
#include "carried.h"
#include "wram_names.h"

uint16_t MenuWaitThenAnimate(uint8_t bank, uint16_t a, uint16_t *x, uint8_t *p);
uint16_t MenuActorCopyGroup(Carried *cr, uint8_t bank, uint16_t *xo, uint8_t *p);
int MenuActorFindA(uint8_t bank, uint16_t kind, uint16_t *x, uint16_t *y, uint8_t *p, uint16_t *a);
void MenuActorFind8(uint8_t bank, uint8_t kind, uint16_t *x, uint16_t *y, uint8_t *p, uint8_t *a);
uint16_t MenuActorFree(uint8_t bank, uint16_t rec, uint16_t slot);
uint16_t FrameByteWritePush(uint8_t bank, uint16_t addr, uint8_t value);
void SoundPort0QueuePushW(uint8_t command, unsigned wide);

/* The registers these routines carry: the object (X), A, Y and P, in and
   out, as the original leaves them for its callers. */

/* A field of object x: direct page (D = 0) + X, bank 0. */
static uint16_t fld(uint16_t x, uint16_t off)
{
    return (uint16_t)(x + off);
}

/* An absolute address through the data bank. */
static uint32_t abs_db(uint8_t bank, uint16_t a)
{
    return (uint32_t)bank << 16 | a;
}

/* LDA (16-bit): N, Z from the value. */
static uint16_t lda(uint16_t *a, uint8_t *p, uint16_t v)
{
    *a = v;
    *p = nz16(*p, v);
    return v;
}

/* LDA #v (16-bit). */
static void lda_imm(uint16_t *a, uint8_t *p, uint16_t v)
{
    cyc(38);
    lda(a, p, v);
}

/* CLC, ADC m and SEC, SBC m (16-bit). */
static void add(uint16_t *a, uint8_t *p, uint16_t m)
{
    *p &= (uint8_t)~P_C;
    *a = alu_adc16(*a, m, p);
}

static void sub(uint16_t *a, uint8_t *p, uint16_t m)
{
    *p |= P_C;
    *a = alu_sbc16(*a, m, p);
}

/* CMP m (16-bit): C when a >= m; N, Z from a - m. */
static void cmp(uint16_t a, uint8_t *p, uint16_t m)
{
    *p = (uint8_t)((nz16(*p, (uint16_t)(a - m)) & ~P_C) | (a >= m ? P_C : 0));
}

/* AND #m (16-bit). */
static void and_imm(uint16_t *a, uint8_t *p, uint16_t m)
{
    cyc(2);
    lda(a, p, (uint16_t)(*a & m));
}

/* ASL A, LSR A (16-bit). */
static void asl(uint16_t *a, uint8_t *p)
{
    *p = (uint8_t)((*p & ~P_C) | (*a >> 15));
    lda(a, p, (uint16_t)(*a << 1));
}

static void lsr(uint16_t *a, uint8_t *p)
{
    *p = (uint8_t)((*p & ~P_C) | (*a & 1));
    lda(a, p, (uint16_t)(*a >> 1));
}

/* TAX, TXA (16-bit). */
static void tax(uint16_t a, uint16_t *x, uint8_t *p)
{
    cyc(30);
    *x = a;
    *p = nz16(*p, a);
}

static void txa(uint16_t x, uint16_t *a, uint8_t *p)
{
    cyc(30);
    lda(a, p, x);
}

/* MenuActorFind for kind (A): with M set at the call, its 8-bit form. */
static void find_kind(uint8_t bank, uint16_t kind, uint16_t *x, uint16_t *y, uint8_t *p, uint16_t *a)
{
    uint8_t a8 = (uint8_t)*a;

    if (*p & P_M) {
        MenuActorFind8(bank, (uint8_t)kind, x, y, p, &a8);
        *a = (uint16_t)((*a & 0xFF00) | a8);
        return;
    }
    MenuActorFindA(bank, kind, x, y, p, a);
}

/* LDA #v; STA field. */
static void set_field(uint16_t x, uint16_t *a, uint8_t *p, uint16_t off, uint16_t v)
{
    lda_imm(a, p, v);
    wr16(fld(x, off), cyc_then(8, v));
}

/* DEC of a field (16-bit): N, Z from the result. */
static uint16_t dec_fld(uint16_t x, uint8_t *p, uint16_t off)
{
    uint16_t v = (uint16_t)(rd16(cyc_then(22, fld(x, off))) - 1);

    wr16(fld(x, off), cyc_then(14, v));
    *p = nz16(*p, v);
    return v;
}

/* SEP #$20, LDA #value, (SEP #$20, REP #$10,) PHX, LDX #reg, JSL
   FrameByteWritePush, PLX: a byte for register reg on the frame's write
   list; X and its N, Z back. */
static void push_reg_byte(uint8_t bank, uint16_t x, uint16_t *a, uint8_t *p, uint8_t value, uint16_t reg)
{
    cyc(176);
    *p |= P_M;
    *a = (uint16_t)((*a & 0xFF00) | value);
    cyc(22);
    *p &= (uint8_t)~P_X;
    FrameByteWritePush(bank, reg, value);
    *p = nz16(*p, x);
}

/* SEP #$20, LDA #$46, JSL SoundPort0QueuePush, REP #$20: the menu's
   select sound. */
static void sound_46(uint8_t bank, uint16_t *a, uint8_t *p)
{
    *p |= P_M;
    *a = (uint16_t)((*a & 0xFF00) | 0x46);
    cyc(100);
    SoundPort0QueuePushW(0x46, (*p & P_X) ? 0 : 16);
    *p = nz8(*p, bank);                 /* the last pull restores DB */
    cyc(22);
    *p &= (uint8_t)~P_M;
    cyc(42);
}

/* LDA.L long, AND #$FF: a byte of bank 0 as a word. */
static uint16_t byte_of(uint16_t *a, uint8_t *p, uint32_t addr)
{
    cyc(32);
    lda(a, p, rd16(addr));
    and_imm(a, p, 0x00FF);
    cyc(4);
    return *a;
}

/*
 * @routine 0B:D8A9 ResultsWaitThenRise
 * WHAT IT DOES: Counts object X's +$0A down; when it runs out, the object
 * goes on to ResultsRise (+$08 = $D8B5) with +$0A = 0.
 * HOW: DEC and the sign.
 * Entry: M 16-bit, X 16-bit, D = $0000; X = the object. Exit: counting: N,
 * Z from the count; else A = $D8B5, N set, Z clear.
 * From C: *x, *a, *y, *p the registers in and out.
 */
void ResultsWaitThenRise(uint16_t x, uint16_t *a, uint8_t *p)
{
    if (!(dec_fld(x, p, 0x0A) & 0x8000))
        { cyc(64); return; } else { cyc(16); }
    set_field(x, a, p, 0x08, 0xD8B5);
    wr16(fld(x, 0x0A), cyc_then(22, 0x0000));
    cyc(42);
}

/* ResultsWaitThenRise on carried registers. */
void ResultsWaitThenRise_cr(Carried *cr)
{
    uint8_t p = cr->p;

    ResultsWaitThenRise(cr->x, &cr->a, &p);
    set_p(cr, p);
}

void abi_ResultsWaitThenRise(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ResultsWaitThenRise_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* The rise shared by ResultsRise and ResultsRiseAgain: +$0A up by 4 to at
   most $66, kept at $1921, the scroll $11EF = $66 - it; 1 when it is $66. */
static int rise(uint8_t bank, uint16_t x, uint16_t *a, uint8_t *p)
{
    uint16_t kept;

    cyc(22);
    *p &= (uint8_t)~(P_M | P_X);
    lda(a, p, rd16(cyc_then(22, fld(x, 0x0A))));
    add(a, p, 0x0004);
    cmp(*a, p, 0x0066);
    if (*p & P_C)
        lda_imm(a, p, 0x0066); else { cyc(20); }
    wr16(fld(x, 0x0A), cyc_then(86, *a));
    cyc(32);
    wr16(0x001921, *a);
    kept = *a;
    lda_imm(a, p, 0x0066);
    sub(a, p, rd16(cyc_then(52, fld(x, 0x0A))));
    wr16(abs_db(bank, 0x11EF), cyc_then(18, *a));
    lda(a, p, kept);
    cmp(*a, p, 0x0066);
    cyc(4);
    return (*p & P_Z) != 0;
}

/*
 * @routine 0B:D8B5 ResultsRise
 * WHAT IT DOES: Raises the standings panel: +$0A goes up by 4 to $66 (kept
 * at $1921), the scroll $11EF is $66 less it; at $66 the object goes on to
 * ResultsOpenTop (+$08 = $D8FA, +$0A = 0) and HDMA channels 3, 5, 6 and 7
 * are asked for ($E8 for $420C on the frame's write list).
 * HOW: The add is held at $66 with CMP and the carry; A is kept on the
 * stack around the scroll.
 * Entry: X = the object, D = $0000. Exit: M 16-bit, X 16-bit; rising: A =
 * +$0A, N, Z, C from the compare with $66; done: X = the object, N, Z from
 * it, A low byte $E8.
 * From C: *x, *a, *y, *p the registers in and out.
 */
void ResultsRise(uint8_t bank, uint16_t x, uint16_t *a, uint8_t *p)
{
    if (!rise(bank, x, a, p))
        { cyc(114); return; } else { cyc(78); }
    set_field(x, a, p, 0x08, 0xD8FA);
    set_field(x, a, p, 0x0A, 0x0000);
    push_reg_byte(bank, x, a, p, 0xE8, 0x420C);
    cyc(22);
    *p &= (uint8_t)~P_M;
    cyc(186);
}

/* ResultsRise on carried registers. */
void ResultsRise_cr(Carried *cr)
{
    uint8_t p = cr->p;

    ResultsRise(cr->db, cr->x, &cr->a, &p);
    set_p(cr, p);
}

void abi_ResultsRise(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ResultsRise_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* The window opening shared by ResultsOpenTop and ResultsOpenBottom: +$0A
   up by 8; below $100 its low byte goes to the window line at `line`;
   1 at $100. */
static int open_window(uint16_t x, uint16_t *a, uint8_t *p, uint32_t line)
{
    lda(a, p, rd16(cyc_then(22, fld(x, 0x0A))));
    add(a, p, 0x0008);
    wr16(fld(x, 0x0A), cyc_then(60, *a));
    cmp(*a, p, 0x0100);
    if (*p & P_Z)
        { cyc(46); return 1; }
    cyc(94);
    wr8(line, (uint8_t)*a);
    cyc(42);
    return 0;
}

/*
 * @routine 0B:D8FA ResultsOpenTop
 * WHAT IT DOES: Opens the window's top band: +$0A up by 8, its low byte to
 * $7F:130D; at $100 the object goes on to ResultsOpenBottom (+$08 = $D91B,
 * +$0A = 0).
 * HOW: An 8-bit long store between SEP and REP.
 * Entry: M 16-bit, X 16-bit, D = $0000; X = the object. Exit: M 16-bit; A
 * = +$0A (0 when done); N, Z, C from the compare with $100, or from 0.
 * From C: *x, *a, *y, *p the registers in and out.
 */
void ResultsOpenTop(uint16_t x, uint16_t *a, uint8_t *p)
{
    if (!open_window(x, a, p, 0x7F130D))
        { cyc(22); return; }
    set_field(x, a, p, 0x08, 0xD91B);
    set_field(x, a, p, 0x0A, 0x0000);
    cyc(42);
}

/* ResultsOpenTop on carried registers. */
void ResultsOpenTop_cr(Carried *cr)
{
    uint8_t p = cr->p;

    ResultsOpenTop(cr->x, &cr->a, &p);
    set_p(cr, p);
}

void abi_ResultsOpenTop(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ResultsOpenTop_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:D91B ResultsOpenBottom
 * WHAT IT DOES: As ResultsOpenTop for the bottom band ($7F:130F); at $100
 * the object goes on to ResultsWaitButton (+$08 = $DA8A, +$0A = 0), HDMA
 * channels 5, 6 and 7 are asked for ($E0 for $420C) with the window mask
 * $212E cleared, and, when the leader's place $0FAC is not 0 and the
 * championship is over ($0DD6 = 8), the kind $18 object (MenuActorFind)
 * starts ResultsLeaderBlink (+$08 = $DA33, +$0A = +$22 = 0).
 * HOW: Two bytes on the frame's write list; the tests through 8-bit values
 * read as words and masked.
 * Entry: M 16-bit, X 16-bit, D = $0000; X = the object. Exit: M 16-bit;
 * opening: as ResultsOpenTop; done: A, X and the flags from the last step
 * (X = the kind $18 object when it was started).
 * From C: *x, *a, *y, *p the registers in and out.
 */
void ResultsOpenBottom(uint8_t bank, uint16_t *x, uint16_t *a, uint16_t *y, uint8_t *p)
{
    if (!open_window(*x, a, p, 0x7F130F))
        { cyc(22); return; }
    set_field(*x, a, p, 0x08, 0xDA8A);
    set_field(*x, a, p, 0x0A, 0x0000);
    push_reg_byte(bank, *x, a, p, 0xE0, 0x420C);
    *a &= 0xFF00;
    *p = nz8(*p, 0x00);
    cyc(122);
    push_reg_byte(bank, *x, a, p, 0x00, 0x212E);
    cyc(22);
    *p &= (uint8_t)~P_M;
    cyc(144);
    byte_of(a, p, 0x000FAC);
    cmp(*a, p, 0x0000);
    if (*p & P_Z)
        return; else { cyc(58); }
    byte_of(a, p, 0x000DD6);
    cmp(*a, p, 0x0008);
    if (!(*p & P_Z))
        return; else { cyc(90); }
    lda_imm(a, p, 0x0018);
    find_kind(bank, 0x0018, x, y, p, a);
    set_field(*x, a, p, 0x08, 0xDA33);
    wr16(fld(*x, 0x0A), cyc_then(22, 0x0000));
    wr16(fld(*x, 0x22), cyc_then(22, 0x0000));
    cyc(42);
}

/* ResultsOpenBottom on carried registers. */
void ResultsOpenBottom_cr(Carried *cr)
{
    uint8_t p = cr->p;

    ResultsOpenBottom(cr->db, &cr->x, &cr->a, &cr->y, &p);
    set_p(cr, p);
}

void abi_ResultsOpenBottom(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ResultsOpenBottom_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:D985 ResultsPlaceWait
 * WHAT IT DOES: The place marker for place +$22: when the leader's place
 * $0FAC is it and the championship is over ($0DD6 = 8), copies its sprite
 * group (MenuActorCopyGroup) and starts ResultsLeaderBlink (+$08 = $DA33,
 * +$22 = +$0A = 0); otherwise MenuWaitThenAnimate.
 * HOW: Byte compares through words masked with $FF; X reloaded from $11F9
 * after the copy.
 * Entry: M 16-bit, X 16-bit, D = $0000; X = the object. Exit: started: X =
 * $11F9; A = $DA33; N, Z from it; otherwise as MenuWaitThenAnimate.
 * From C: *x, *a, *y, *p the registers in and out.
 */
void ResultsPlaceWait(Carried *cr, uint8_t bank, uint16_t *x, uint16_t *a, uint8_t *p)
{
    byte_of(a, p, 0x000FAC);
    cmp(*a, p, rd16(cyc_then(40, fld(*x, 0x22))));
    if (*p & P_Z) {
        cyc(16);
        byte_of(a, p, 0x000DD6);
        cmp(*a, p, 0x0008);
        if (*p & P_Z) {
            cyc(104);
            *a = MenuActorCopyGroup(cr, bank, x, p);
            *x = rd16(cyc_then(24, abs_db(bank, 0x11F9)));
            *p = nz16(*p, *x);
            set_field(*x, a, p, 0x08, 0xDA33);
            wr16(fld(*x, 0x22), cyc_then(22, 0x0000));
            wr16(fld(*x, 0x0A), cyc_then(22, 0x0000));
            cyc(42);
            return;
        } else { cyc(42); }            /* CMP; the BNE taken is in the 46 below */
    }
    cyc(46);
    *a = MenuWaitThenAnimate(bank, *a, x, p);
}

/* ResultsPlaceWait on carried registers. */
void ResultsPlaceWait_cr(Carried *cr)
{
    uint8_t p = cr->p;

    ResultsPlaceWait(cr, cr->db, &cr->x, &cr->a, &p);
    set_p(cr, p);
}

void abi_ResultsPlaceWait(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ResultsPlaceWait_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:D9EA ResultsCarIndex
 * WHAT IT DOES: X = the index of the car for place +$22 - 1 in the course
 * $0D4A's results: course * 9 + (place - 1) * 3.
 * HOW: Shifts and adds; $11DF and $11E1 hold the parts.
 * Entry: M 16-bit, X 16-bit, D = $0000; X = the object. Exit: X = A = the
 * index; N, Z, C, V from the last add.
 * From C: *x, *a, *y, *p the registers in and out.
 */
void ResultsCarIndex(uint8_t bank, uint16_t *x, uint16_t *a, uint8_t *p)
{
    uint16_t v;

    byte_of(a, p, W(wram_0D4A));
    wr16(abs_db(bank, 0x11DF), cyc_then(42, *a));
    asl(a, p);
    asl(a, p);
    asl(a, p);
    add(a, p, rd16(cyc_then(80, abs_db(bank, 0x11DF))));
    wr16(abs_db(bank, 0x11E1), cyc_then(24, *a));
    lda(a, p, rd16(cyc_then(22, fld(*x, 0x22))));
    v = lda(a, p, (uint16_t)(*a - 1));
    wr16(abs_db(bank, 0x11DF), cyc_then(38, v));
    asl(a, p);
    add(a, p, rd16(cyc_then(52, abs_db(bank, 0x11DF))));
    add(a, p, rd16(cyc_then(38, abs_db(bank, 0x11E1))));
    tax(*a, x, p);
    cyc(26);
}

/* ResultsCarIndex on carried registers. */
void ResultsCarIndex_cr(Carried *cr)
{
    uint8_t p = cr->p;

    ResultsCarIndex(cr->db, &cr->x, &cr->a, &p);
    set_p(cr, p);
}

void abi_ResultsCarIndex(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ResultsCarIndex_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:D9AF ResultsPlaceCar
 * WHAT IT DOES: The car picture for place +$22: the car's kind (the low two
 * bits of the byte at $70:FC6F + ResultsCarIndex) picks its animation from
 * the table at $0B:DA17 for the leader's place ($0FAC) or $0B:DA0F for the
 * others, stored at +$14; then MenuWaitThenAnimate.
 * HOW: The table read through X, then X reloaded from $11F9.
 * Entry: M 16-bit, X 16-bit, D = $0000; X = the object. Exit: as
 * MenuWaitThenAnimate.
 * From C: *x, *a, *y, *p the registers in and out.
 */
void ResultsPlaceCar(uint8_t bank, uint16_t *x, uint16_t *a, uint8_t *p)
{
    uint32_t table;

    byte_of(a, p, 0x000FAC);
    cmp(*a, p, rd16(cyc_then(40, fld(*x, 0x22))));
    table = (*p & P_Z) ? (cyc(12), 0x0BDA17) : (cyc(18), 0x0BDA0F);
    (cyc(50), ResultsCarIndex(bank, x, a, p));
    cyc(32);
    lda(a, p, rd16(0x70FC6F + (uint32_t)*x));
    and_imm(a, p, 0x0003);
    asl(a, p);
    tax(*a, x, p);
    cyc(92);
    lda(a, p, rd16(table + (uint32_t)*x));
    *x = rd16(abs_db(bank, 0x11F9));
    *p = nz16(*p, *x);
    wr16(fld(*x, 0x14), cyc_then(22, *a));
    *a = (cyc(24), MenuWaitThenAnimate(bank, *a, x, p));
}

/* ResultsPlaceCar on carried registers. */
void ResultsPlaceCar_cr(Carried *cr)
{
    uint8_t p = cr->p;

    ResultsPlaceCar(cr->db, &cr->x, &cr->a, &p);
    set_p(cr, p);
}

void abi_ResultsPlaceCar(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ResultsPlaceCar_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:DA62 ResultsGroupPalette
 * WHAT IT DOES: Gives every sprite of object X's group the palette bits
 * $11DF: each attribute word (the group's sprite count at $7F:1356 + +$12,
 * its entries from $7F:135A) has bits 9-11 replaced.
 * HOW: AND #$F1FF, ORA $11DF per entry, 8 bytes apart; X kept on the stack.
 * Entry: M 16-bit, X 16-bit, D = $0000; X = the object. Exit: X as on
 * entry, N, Z from it (PLX); Y = $FFFF; A = the last entry offset + 8;
 * C, V from the last add.
 * From C: *x, *a, *y, *p the registers in and out.
 */
void ResultsGroupPalette(uint8_t bank, uint16_t x, uint16_t *a, uint16_t *y, uint8_t *p)
{
    uint16_t obj = x;

    lda(a, p, rd16(cyc_then(52, fld(x, 0x12))));
    tax(*a, &x, p);
    cyc(16);
    lda(a, p, rd16(W(menu_sprite_groups) + (uint32_t)x));
    *y = *a;
    *p = nz16(*p, *y);
    x = (uint16_t)(x + 2);
    do {
        cyc(74);
        lda(a, p, rd16(W(menu_sprite_attr) + (uint32_t)x));
        and_imm(a, p, 0xF1FF);
        lda(a, p, (uint16_t)(*a | rd16(cyc_then(46, abs_db(bank, 0x11DF)))));
        cyc(32);
        wr16(W(menu_sprite_attr) + (uint32_t)x, *a);
        txa(x, a, p);
        add(a, p, 0x0008);
        tax(*a, &x, p);
        *y = (uint16_t)(*y - 1);
        *p = nz16(*p, *y);
    } while (!(*y & 0x8000));
    cyc(114);
    *p = nz16(*p, obj);
}

/* ResultsGroupPalette on carried registers. */
void ResultsGroupPalette_cr(Carried *cr)
{
    uint8_t p = cr->p;

    ResultsGroupPalette(cr->db, cr->x, &cr->a, &cr->y, &p);
    set_p(cr, p);
}

void abi_ResultsGroupPalette(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ResultsGroupPalette_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:DA33 ResultsLeaderBlink
 * WHAT IT DOES: The leader's place marker blinks: every sixth frame (+$0A
 * run down from 5) its step +$22 flips between 0 and 1, which picks the
 * palette bits $200 or $600 (table $0B:DA86) for its group
 * (ResultsGroupPalette); the count and step are kept at $190F and $1911.
 * HOW: The palette word through $11DF; X reloaded from $11F9.
 * Entry: M 16-bit, X 16-bit, D = $0000; X = the object. Exit: as
 * ResultsGroupPalette.
 * From C: *x, *a, *y, *p the registers in and out.
 */
void ResultsLeaderBlink(uint8_t bank, uint16_t *x, uint16_t *a, uint16_t *y, uint8_t *p)
{
    if (dec_fld(*x, p, 0x0A) & 0x8000) {
        cyc(16);
        set_field(*x, a, p, 0x0A, 0x0005);
        lda(a, p, rd16(cyc_then(22, fld(*x, 0x22))));
        *a = (uint16_t)(*a + 1);
        and_imm(a, p, 0x0001);
        wr16(fld(*x, 0x22), cyc_then(58, *a));
    } else { cyc(22); }
    lda(a, p, rd16(cyc_then(22, fld(*x, 0x22))));
    asl(a, p);
    tax(*a, x, p);
    cyc(70);
    lda(a, p, rd16(0x0BDA86 + (uint32_t)*x));
    wr16(abs_db(bank, 0x11DF), *a);
    *x = rd16(cyc_then(24, abs_db(bank, 0x11F9)));
    *p = nz16(*p, *x);
    lda(a, p, rd16(cyc_then(22, fld(*x, 0x0A))));
    cyc(32);
    wr16(0x00190F, *a);
    lda(a, p, rd16(cyc_then(22, fld(*x, 0x22))));
    cyc(32);
    wr16(0x001911, *a);
    (cyc(46), ResultsGroupPalette(bank, *x, a, y, p));
    cyc(42);
}

/* ResultsLeaderBlink on carried registers. */
void ResultsLeaderBlink_cr(Carried *cr)
{
    uint8_t p = cr->p;

    ResultsLeaderBlink(cr->db, &cr->x, &cr->a, &cr->y, &p);
    set_p(cr, p);
}

void abi_ResultsLeaderBlink(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ResultsLeaderBlink_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:DA1F ResultsLeaderBlinkResume
 * WHAT IT DOES: Starts the leader's place marker blinking where it was:
 * copies its sprite group (MenuActorCopyGroup), takes the count and step
 * back from $190F and $1911, makes ResultsLeaderBlink its handler and runs
 * it.
 * HOW: Falls into ResultsLeaderBlink.
 * Entry: M 16-bit, X 16-bit, D = $0000; X = the object. Exit: as
 * ResultsLeaderBlink.
 * From C: *x, *a, *y, *p the registers in and out.
 */
void ResultsLeaderBlinkResume(Carried *cr, uint8_t bank, uint16_t *x, uint16_t *a, uint16_t *y, uint8_t *p)
{
    cyc(46);
    *a = MenuActorCopyGroup(cr, bank, x, p);
    cyc(32);
    lda(a, p, rd16(0x00190F));
    wr16(fld(*x, 0x0A), cyc_then(22, *a));
    cyc(32);
    lda(a, p, rd16(0x001911));
    wr16(fld(*x, 0x22), cyc_then(22, *a));
    set_field(*x, a, p, 0x08, 0xDA33);
    ResultsLeaderBlink(bank, x, a, y, p);
}

/* ResultsLeaderBlinkResume on carried registers. */
void ResultsLeaderBlinkResume_cr(Carried *cr)
{
    uint8_t p = cr->p;

    ResultsLeaderBlinkResume(cr, cr->db, &cr->x, &cr->a, &cr->y, &p);
    set_p(cr, p);
}

void abi_ResultsLeaderBlinkResume(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ResultsLeaderBlinkResume_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* Y = the A, B or Start presses ($118B AND $9000), with N, Z (TYA, AND). */
static uint16_t pressed(uint8_t bank, uint16_t *a, uint16_t *y, uint8_t *p)
{
    *y = rd16(cyc_then(24, abs_db(bank, 0x118B)));
    *a = *y;
    and_imm(a, p, 0x9000);
    return *a;
}

/* LDA #1, STA $11EB, LDY $11FB, JSR MenuActorFree, then the sound: the
   screen ends and the object is freed. */
static void end_screen(uint8_t bank, uint16_t *x, uint16_t *a, uint16_t *y, uint8_t *p)
{
    lda_imm(a, p, 0x0001);
    wr16(abs_db(bank, 0x11EB), cyc_then(62, 0x0001));
    *y = rd16(cyc_then(24, abs_db(bank, 0x11FB)));
    *p = nz16(*p, *y);
    *a = (cyc(46), MenuActorFree(bank, *x, *y));
    *x = 0xFFFF;
    *p = (uint8_t)((*p & ~P_Z) | P_N);
    cyc(42);
    sound_46(bank, a, p);
}

/*
 * @routine 0B:DA8A ResultsWaitButton
 * WHAT IT DOES: Waits for A, B or Start: then, when the championship is
 * over ($0DD6 = 8), the panel goes down again (ResultsLower: +$08 = $DAD1,
 * +$0A = $66), otherwise the screen ends ($11EB = 1, the object freed,
 * reached by a unit test, unit_menus); sound
 * $46 either way.
 * HOW: The leader's place $0FAC is read and masked but not used.
 * Entry: M 16-bit, X 16-bit, D = $0000; X = the object. Exit: no press: Y
 * = the presses, A = 0, Z set; else M 16-bit, A low byte $46 and the
 * flags as SoundPort0QueuePush leaves them.
 * From C: *x, *a, *y, *p the registers in and out.
 */
void ResultsWaitButton(uint8_t bank, uint16_t *x, uint16_t *a, uint16_t *y, uint8_t *p)
{
    if (!pressed(bank, a, y, p))
        { cyc(100); return; } else { cyc(52); }
    byte_of(a, p, 0x000DD6);
    cmp(*a, p, 0x0008);
    if (!(*p & P_Z)) {
        cyc(12);
        end_screen(bank, x, a, y, p);
        return;
    }
    lda(a, p, rd16(cyc_then(82, abs_db(bank, 0x0FAC))));
    and_imm(a, p, 0x00FF);
    cyc(22);
    set_field(*x, a, p, 0x08, 0xDAD1);
    set_field(*x, a, p, 0x0A, 0x0066);
    sound_46(bank, a, p);
}

/* ResultsWaitButton on carried registers. */
void ResultsWaitButton_cr(Carried *cr)
{
    uint8_t p = cr->p;

    ResultsWaitButton(cr->db, &cr->x, &cr->a, &cr->y, &p);
    set_p(cr, p);
}

void abi_ResultsWaitButton(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ResultsWaitButton_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:DC6D ResultsRiseAgain
 * WHAT IT DOES: As ResultsRise, then goes on to ResultsWaitEnd (+$08 =
 * $DC9F, +$0A = 0) with no HDMA change.
 * HOW: As ResultsRise.
 * Entry: X = the object, D = $0000. Exit: M 16-bit, X 16-bit; rising: as
 * ResultsRise; done: A = 0, Z set, N clear.
 * From C: *x, *a, *y, *p the registers in and out.
 */
void ResultsRiseAgain(uint8_t bank, uint16_t x, uint16_t *a, uint8_t *p)
{
    if (!rise(bank, x, a, p))
        { cyc(114); return; } else { cyc(78); }
    set_field(x, a, p, 0x08, 0xDC9F);
    set_field(x, a, p, 0x0A, 0x0000);
    cyc(42);
}

/* ResultsRiseAgain on carried registers. */
void ResultsRiseAgain_cr(Carried *cr)
{
    uint8_t p = cr->p;

    ResultsRiseAgain(cr->db, cr->x, &cr->a, &p);
    set_p(cr, p);
}

void abi_ResultsRiseAgain(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ResultsRiseAgain_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:DC9F ResultsWaitEnd
 * WHAT IT DOES: Waits for A, B or Start, then ends the screen ($11EB = 1),
 * frees the object (MenuActorFree) and plays sound $46.
 * HOW: As ResultsWaitButton's other case.
 * Entry: M 16-bit, X 16-bit, D = $0000; X = the object. Exit: as
 * ResultsWaitButton.
 * From C: *x, *a, *y, *p the registers in and out.
 */
void ResultsWaitEnd(uint8_t bank, uint16_t *x, uint16_t *a, uint16_t *y, uint8_t *p)
{
    if (!pressed(bank, a, y, p))
        { cyc(100); return; }
    end_screen(bank, x, a, y, p);
}

/* ResultsWaitEnd on carried registers. */
void ResultsWaitEnd_cr(Carried *cr)
{
    uint8_t p = cr->p;

    ResultsWaitEnd(cr->db, &cr->x, &cr->a, &cr->y, &p);
    set_p(cr, p);
}

void abi_ResultsWaitEnd(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ResultsWaitEnd_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:DD01 ResultsOverOrHide
 * WHAT IT DOES: When the championship is over ($0DD6 = 8),
 * MenuWaitThenAnimate; otherwise the object is hidden (+$10 = $FFFF) and
 * does nothing more (+$08 = MenuActorNoStep; reached by a unit test,
 * unit_menus).
 * HOW: The other case is the code after the JMP ($0B:DD10), reloading X
 * from $11F9.
 * Entry: M 16-bit, X 16-bit, D = $0000; X = the object. Exit: as
 * MenuWaitThenAnimate; hidden: X = $11F9, A = $DF7B, N set, Z clear.
 * From C: *x, *a, *y, *p the registers in and out.
 */
void ResultsOverOrHide(uint8_t bank, uint16_t *x, uint16_t *a, uint8_t *p)
{
    byte_of(a, p, 0x000DD6);
    cmp(*a, p, 0x0008);
    if (*p & P_Z) {
        cyc(82);
        *a = MenuWaitThenAnimate(bank, *a, x, p);
        return;
    }
    *x = rd16(cyc_then(88, abs_db(bank, 0x11F9)));
    *p = nz16(*p, *x);
    set_field(*x, a, p, 0x10, 0xFFFF);
    set_field(*x, a, p, 0x08, 0xDF7B);
    cyc(42);
}

/* ResultsOverOrHide on carried registers. */
void ResultsOverOrHide_cr(Carried *cr)
{
    uint8_t p = cr->p;

    ResultsOverOrHide(cr->db, &cr->x, &cr->a, &p);
    set_p(cr, p);
}

void abi_ResultsOverOrHide(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ResultsOverOrHide_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

uint16_t MenuActorsSet10(uint8_t bank, uint16_t kind, uint16_t value, uint8_t *p);
uint16_t MenuActorAnimateFrame(uint8_t bank, uint16_t *x, uint8_t *p);

/*
 * @routine 0B:DAD1 ResultsLower
 * WHAT IT DOES: Lowers the standings panel: +$0A goes down by 4 to 0 (kept
 * at $1921), the scroll $11EF is $66 less it; at 0 the object goes on to
 * ResultsRiseAgain (+$08 = $DC6D, +$0A = 0), $1925 = $5F, the objects of
 * kinds $14-$17 are hidden and those of kinds $1A-$25 shown
 * (MenuActorsSet10 with $FFFF and 0), and for the leader's place $0FAC
 * (1-3; places 2 and 3 reached by unit tests,
 * unit_menus) the kind $18 object gets animation $262, $268 or $26E,
 * +$02 = $88 and ResultsLeaderBlinkResume, the kind $19 object animation
 * $250, $256 or $25C, +$02 = $58 and MenuWaitThenAnimate.
 * HOW: An animation is restarted (+$16 = +$18 = 0) only when it changes.
 * The place tests are byte compares through words masked with $FF.
 * Entry: X = the object, D = $0000. Exit: M 16-bit, X 16-bit; lowering: A
 * = +$0A, N, Z, C from the compare with 0; done: X = the kind $19 object
 * (or as MenuActorFind left it), A and the flags from the last step.
 * From C: *x, *a, *y, *p the registers in and out.
 */
static void place_object(uint8_t bank, uint16_t *x, uint16_t *a, uint16_t *y, uint8_t *p,
                         uint16_t kind, uint16_t anim0, uint16_t pos, uint16_t handler)
{
    uint16_t place, anim;

    lda_imm(a, p, kind);
    find_kind(bank, kind, x, y, p, a);
    place = byte_of(a, p, 0x000FAC);
    if (!place)
        return;
    cmp(*a, p, 0x0001);
    anim = anim0;
    if (!(*p & P_Z)) {
        cyc(42);
        cmp(*a, p, 0x0002);
        anim = (*p & P_Z) ? (uint16_t)(anim0 + 6) : (cyc(6), (uint16_t)(anim0 + 12));
    }
    lda(a, p, rd16(cyc_then(place == 1 ? 96 : 100, fld(*x, 0x14))));
    cmp(*a, p, anim);
    if (!(*p & P_Z)) {
        cyc(40);
        set_field(*x, a, p, 0x14, anim);
        wr16(fld(*x, 0x16), cyc_then(22, 0x0000));
        wr16(fld(*x, 0x18), cyc_then(22, 0x0000));
    }
    set_field(*x, a, p, 0x02, pos);
    set_field(*x, a, p, 0x08, handler);
    if (place <= 2)
        cyc(22);   /* BRA past the next place's code */
}

void ResultsLower(uint8_t bank, uint16_t *x, uint16_t *a, uint16_t *y, uint8_t *p)
{
    static const uint16_t kinds[16] = {0x14, 0x15, 0x16, 0x17, 0x1A, 0x1B, 0x1C, 0x1D,
                                       0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25};
    uint16_t kept;
    unsigned i;

    cyc(22);
    *p &= (uint8_t)~(P_M | P_X);
    lda(a, p, rd16(cyc_then(22, fld(*x, 0x0A))));
    sub(a, p, 0x0004);
    if (*p & P_N)
        lda_imm(a, p, 0x0000); else { cyc(20); }
    wr16(fld(*x, 0x0A), cyc_then(56, *a));
    cyc(32);
    wr16(0x001921, *a);
    kept = *a;
    lda_imm(a, p, 0x0066);
    sub(a, p, rd16(cyc_then(52, fld(*x, 0x0A))));
    wr16(abs_db(bank, 0x11EF), cyc_then(18, *a));
    lda(a, p, kept);
    cmp(*a, p, 0x0000);
    if (!(*p & P_Z))
        { cyc(118); return; } else { cyc(82); }
    set_field(*x, a, p, 0x08, 0xDC6D);
    set_field(*x, a, p, 0x0A, 0x0000);
    lda_imm(a, p, 0x005F);
    cyc(18);
    wr16(0x001925, 0x005F);
    for (i = 0; cyc_loop(56, i < 16); i++) {
        lda_imm(a, p, kinds[i]);
        *y = i < 4 ? 0xFFFF : 0x0000;
        *p = nz16(*p, *y);
        *a = MenuActorsSet10(bank, kinds[i], *y, p);
        *y = 0xFFFE;
        *p = nz16(*p, *x);
    }
    cyc(32);
    place_object(bank, x, a, y, p, 0x0018, 0x0262, 0x0088, 0xDA1F);
    cyc(32);
    place_object(bank, x, a, y, p, 0x0019, 0x0250, 0x0058, 0xD5EC);
    cyc(42);
}

/* ResultsLower on carried registers. */
void ResultsLower_cr(Carried *cr)
{
    uint8_t p = cr->p;

    ResultsLower(cr->db, &cr->x, &cr->a, &cr->y, &p);
    set_p(cr, p);
}

void abi_ResultsLower(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ResultsLower_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* The sprite tile at $11E3 into the next entry of the group, $11E1 or
   $11E5 (`at`) stepping by 8: the attribute word's low 9 bits replaced. */
static void tile_at(uint8_t bank, uint16_t *x, uint16_t *a, uint8_t *p, uint16_t at)
{
    *x = rd16(cyc_then(24, abs_db(bank, at)));
    *p = nz16(*p, *x);
    cyc(32);
    lda(a, p, rd16(W(menu_sprite_attr) + (uint32_t)*x));
    and_imm(a, p, 0xFE00);
    lda(a, p, (uint16_t)(*a | rd16(cyc_then(46, abs_db(bank, 0x11E3)))));
    cyc(32);
    wr16(W(menu_sprite_attr) + (uint32_t)*x, *a);
    lda(a, p, rd16(cyc_then(24, abs_db(bank, at))));
    add(a, p, 0x0008);
    wr16(abs_db(bank, at), cyc_then(62, *a));
}

/* A character two sprites high: tile $11E3 and the one $10 on, through
   the entries at `at`. */
static void tall_tile(uint8_t bank, uint16_t *x, uint16_t *a, uint8_t *p, uint16_t at)
{
    tile_at(bank, x, a, p, at);
    lda(a, p, rd16(cyc_then(24, abs_db(bank, 0x11E3))));
    add(a, p, 0x0010);
    wr16(abs_db(bank, 0x11E3), cyc_then(62, *a));
    tile_at(bank, x, a, p, at);
    cyc(78);
}

/*
 * @routine 0B:DD87 ResultsLetterTiles
 * WHAT IT DOES: Puts letter A (0-31) in the next two sprites of the group
 * at $11E5: its tile from the table at $0B:DDD3 and the tile $10 on below
 * it.
 * HOW: The tile through $11E3; X kept on the stack.
 * Entry: M 16-bit, X 16-bit; A = the letter. Exit: X as on entry, N, Z
 * from it; A = the new $11E5; C, V from its add.
 * From C: x the object (unchanged); *a, *p in and out.
 */
void ResultsLetterTiles(uint8_t bank, uint16_t x, uint16_t *a, uint8_t *p)
{
    uint16_t x0 = x;

    asl(a, p);
    tax(*a, &x, p);
    cyc(100);
    lda(a, p, rd16(0x0BDDD3 + (uint32_t)x));
    wr16(abs_db(bank, 0x11E3), *a);
    tall_tile(bank, &x, a, p, 0x11E5);
    *p = nz16(*p, x0);
}

/* ResultsLetterTiles on carried registers. */
void ResultsLetterTiles_cr(Carried *cr)
{
    uint8_t p = cr->p;

    ResultsLetterTiles(cr->db, cr->x, &cr->a, &p);
    set_p(cr, p);
}

void abi_ResultsLetterTiles(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ResultsLetterTiles_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:DE83 ResultsDigitTiles
 * WHAT IT DOES: Puts digit A (0-9; 15 for a blank, tile $94) in the next
 * two sprites of the group at $11E1: tile A + $50 and the one $10 on.
 * HOW: As ResultsLetterTiles with the tile computed.
 * Entry: M 16-bit, X 16-bit; A = the digit. Exit: X as on entry, N, Z
 * from it; A = the new $11E1; C, V from its add.
 * From C: x the object (unchanged); *a, *p in and out.
 */
void ResultsDigitTiles(uint8_t bank, uint16_t x, uint16_t *a, uint8_t *p)
{
    uint16_t x0 = x;

    cmp(*a, p, 0x000F);
    if (*p & P_Z)
        lda_imm(a, p, 0x0044); else { cyc(20); }
    add(a, p, 0x0050);
    wr16(abs_db(bank, 0x11E3), cyc_then(118, *a));
    tall_tile(bank, &x, a, p, 0x11E1);
    *p = nz16(*p, x0);
}

/* ResultsDigitTiles on carried registers. */
void ResultsDigitTiles_cr(Carried *cr)
{
    uint8_t p = cr->p;

    ResultsDigitTiles(cr->db, cr->x, &cr->a, &p);
    set_p(cr, p);
}

void abi_ResultsDigitTiles(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ResultsDigitTiles_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:DE69 ResultsBcdPair
 * WHAT IT DOES: Puts the two decimal digits of the byte A (high, then low)
 * in the group at $11E1 (ResultsDigitTiles).
 * HOW: The byte kept at $11E5.
 * Entry: M 16-bit, X 16-bit; A low byte = the digits. Exit: as
 * ResultsDigitTiles.
 * From C: x the object (unchanged); *a, *p in and out.
 */
void ResultsBcdPair(uint8_t bank, uint16_t x, uint16_t *a, uint8_t *p)
{
    and_imm(a, p, 0x00FF);
    wr16(abs_db(bank, 0x11E5), cyc_then(46, *a));
    lsr(a, p);
    lsr(a, p);
    lsr(a, p);
    lsr(a, p);
    and_imm(a, p, 0x000F);
    (cyc(124), ResultsDigitTiles(bank, x, a, p));
    lda(a, p, rd16(cyc_then(24, abs_db(bank, 0x11E5))));
    and_imm(a, p, 0x000F);
    (cyc(68), ResultsDigitTiles(bank, x, a, p));
    cyc(42);
}

/* ResultsBcdPair on carried registers. */
void ResultsBcdPair_cr(Carried *cr)
{
    uint8_t p = cr->p;

    ResultsBcdPair(cr->db, cr->x, &cr->a, &p);
    set_p(cr, p);
}

void abi_ResultsBcdPair(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ResultsBcdPair_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:DCBF ResultsTimeDigits
 * WHAT IT DOES: When the championship is over ($0DD6 = 8), shows the time
 * $0EEF, $0EEE, $0EED (two digits each) on object X's sprite group and
 * stops it (+$08 = MenuActorNoStep); otherwise hides it (+$10 = $FFFF;
 * reached by unit tests, unit_menus).
 * HOW: MenuActorCopyGroup, then the digits from the group's first entry
 * ($11E1); X reloaded from $11F9.
 * Entry: M 16-bit, X 16-bit, D = $0000; X = the object. Exit: X = $11F9; A
 * = $DF7B; N set, Z clear.
 * From C: *x, *a, *y, *p the registers in and out.
 */
void ResultsTimeDigits(Carried *cr, uint8_t bank, uint16_t *x, uint16_t *a, uint8_t *p)
{
    byte_of(a, p, 0x000DD6);
    cmp(*a, p, 0x0008);
    if (*p & P_Z) {
        cyc(104);
        *a = MenuActorCopyGroup(cr, bank, x, p);
        lda(a, p, rd16(cyc_then(22, fld(*x, 0x12))));
        lda(a, p, (uint16_t)(*a + 2));
        wr16(abs_db(bank, 0x11E1), cyc_then(52, *a));
        cyc(32);
        lda(a, p, rd16(W(wram_0EEF)));
        (cyc(46), ResultsBcdPair(bank, *x, a, p));
        cyc(32);
        lda(a, p, rd16(W(wram_0EEE)));
        (cyc(46), ResultsBcdPair(bank, *x, a, p));
        cyc(32);
        lda(a, p, rd16(W(wram_0EED)));
        (cyc(46), ResultsBcdPair(bank, *x, a, p));
        *x = rd16(cyc_then(24, abs_db(bank, 0x11F9)));
        *p = nz16(*p, *x);
    } else {
        *x = rd16(cyc_then(88, abs_db(bank, 0x11F9)));
        *p = nz16(*p, *x);
        set_field(*x, a, p, 0x10, 0xFFFF);
    }
    set_field(*x, a, p, 0x08, 0xDF7B);
    cyc(42);
}

/* ResultsTimeDigits on carried registers. */
void ResultsTimeDigits_cr(Carried *cr)
{
    uint8_t p = cr->p;

    ResultsTimeDigits(cr, cr->db, &cr->x, &cr->a, &p);
    set_p(cr, p);
}

void abi_ResultsTimeDigits(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ResultsTimeDigits_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* The leader's place test of ResultsInitials and ResultsLapTimes: when
   $0FAC is the object's field `field` and the championship is over, the
   object starts ResultsLeaderBlink; otherwise it stops. */
static void leader_or_stop(uint8_t bank, uint16_t *x, uint16_t *a, uint8_t *p, uint16_t field)
{
    *x = rd16(cyc_then(24, abs_db(bank, 0x11F9)));
    *p = nz16(*p, *x);
    byte_of(a, p, 0x000FAC);
    cmp(*a, p, rd16(cyc_then(40, fld(*x, field))));
    if (*p & P_Z) {
        cyc(16);
        byte_of(a, p, 0x000DD6);
        cmp(*a, p, 0x0008);
        if (*p & P_Z) {
            *x = rd16(cyc_then(82, abs_db(bank, 0x11F9)));
            *p = nz16(*p, *x);
            set_field(*x, a, p, 0x08, 0xDA33);
            wr16(fld(*x, 0x22), cyc_then(22, 0x0000));
            wr16(fld(*x, 0x0A), cyc_then(22, 0x0000));
            cyc(42);
            return;
        } else { cyc(46); }
    } else { cyc(4); }
    cyc(18);
    set_field(*x, a, p, 0x08, 0xDF7B);
    cyc(42);
}

/*
 * @routine 0B:DD1E ResultsInitials
 * WHAT IT DOES: Shows the initials of the car for place +$22 (three 5-bit
 * letters in the word at $70:FC6D + ResultsCarIndex) on object X's sprite
 * group, then starts ResultsLeaderBlink when it is the leader's place and
 * the championship is over, otherwise stops (MenuActorNoStep).
 * HOW: MenuActorCopyGroup; the word shifted right 5 at a time through
 * $11DF; ResultsLetterTiles from the group's first entry ($11E5).
 * Entry: M 16-bit, X 16-bit, D = $0000; X = the object. Exit: X = $11F9;
 * A and the flags from the last step.
 * From C: *x, *a, *y, *p the registers in and out.
 */
void ResultsInitials(Carried *cr, uint8_t bank, uint16_t *x, uint16_t *a, uint8_t *p)
{
    unsigned i;

    cyc(46);
    *a = MenuActorCopyGroup(cr, bank, x, p);
    lda(a, p, rd16(cyc_then(22, fld(*x, 0x12))));
    lda(a, p, (uint16_t)(*a + 2));
    wr16(abs_db(bank, 0x11E5), cyc_then(52, *a));
    (cyc(46), ResultsCarIndex(bank, x, a, p));
    cyc(32);
    lda(a, p, rd16(0x70FC6D + (uint32_t)*x));
    wr16(abs_db(bank, 0x11DF), cyc_then(24, *a));
    for (i = 0; i < 3; i++) {
        if (i) {
            lda(a, p, rd16(cyc_then(24, abs_db(bank, 0x11DF))));
            lsr(a, p);
            lsr(a, p);
            lsr(a, p);
            lsr(a, p);
            lsr(a, p);
            wr16(abs_db(bank, 0x11DF), cyc_then(94, *a));
        }
        cyc(68);
        and_imm(a, p, 0x001F);
        ResultsLetterTiles(bank, *x, a, p);
    }
    leader_or_stop(bank, x, a, p, 0x22);
}

/* ResultsInitials on carried registers. */
void ResultsInitials_cr(Carried *cr)
{
    uint8_t p = cr->p;

    ResultsInitials(cr, cr->db, &cr->x, &cr->a, &p);
    set_p(cr, p);
}

void abi_ResultsInitials(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ResultsInitials_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:DF61 ResultsBlankLetters
 * WHAT IT DOES: Puts three blank letters (26) in object X's sprite group.
 * HOW: ResultsLetterTiles from the group's first entry ($11E5).
 * Entry: M 16-bit, X 16-bit; X = the object. Exit: as ResultsLetterTiles.
 * From C: x the object (unchanged); *a, *p in and out.
 */
void ResultsBlankLetters(uint8_t bank, uint16_t x, uint16_t *a, uint8_t *p)
{
    unsigned i;

    lda(a, p, rd16(cyc_then(22, fld(x, 0x12))));
    lda(a, p, (uint16_t)(*a + 2));
    wr16(abs_db(bank, 0x11E5), cyc_then(52, *a));
    for (i = 0; cyc_loop(32, i < 3); i++) {
        lda_imm(a, p, 0x001A);
        ResultsLetterTiles(bank, x, a, p);
    }
    cyc(42);
}

/* ResultsBlankLetters on carried registers. */
void ResultsBlankLetters_cr(Carried *cr)
{
    uint8_t p = cr->p;

    ResultsBlankLetters(cr->db, cr->x, &cr->a, &p);
    set_p(cr, p);
}

void abi_ResultsBlankLetters(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ResultsBlankLetters_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:DED5 ResultsNoTime
 * WHAT IT DOES: For a place with no time: by the current object's kind
 * ($20, $21 or another; $20 reached by a unit test, unit_menus) blanks
 * the initials of the kind $23, $24 or $25 object (ResultsBlankLetters)
 * and hides the kind $1D, $1E or $1F object (+$10 = $FFFF, +$02 = $200,
 * MenuActorNoStep).
 * HOW: $11DF and $11E1 kept on the stack.
 * Entry: M 16-bit, X 16-bit, D = $0000. Exit: X = the hidden object; A =
 * $11DF; N, Z from it.
 * From C: *x, *a, *y, *p the registers in and out.
 */
void ResultsNoTime(uint8_t bank, uint16_t *x, uint16_t *a, uint16_t *y, uint8_t *p)
{
    uint16_t k11df = rd16(cyc_then(24, abs_db(bank, 0x11DF))), k11e1, letters, other;

    lda(a, p, k11df);
    k11e1 = rd16(cyc_then(54, abs_db(bank, 0x11E1)));
    *x = rd16(cyc_then(54, abs_db(bank, 0x11F9)));
    *p = nz16(*p, *x);
    lda(a, p, rd16(cyc_then(22, fld(*x, 0x1A))));
    cmp(*a, p, 0x0020);
    if (*p & P_Z) {
        letters = 0x0023;
        other = 0x001D;
        cyc(78);
    } else {
        cyc(30);
        cmp(*a, p, 0x0021);
        letters = (*p & P_Z) ? (cyc(6), 0x0024) : 0x0025;
        other = (*p & P_Z) ? 0x001E : 0x001F;
        cyc(82);
    }
    lda_imm(a, p, letters);
    find_kind(bank, letters, x, y, p, a);
    (cyc(46), ResultsBlankLetters(bank, *x, a, p));
    lda_imm(a, p, other);
    (cyc(32), find_kind(bank, other, x, y, p, a));
    set_field(*x, a, p, 0x10, 0xFFFF);
    set_field(*x, a, p, 0x02, 0x0200);
    set_field(*x, a, p, 0x08, 0xDF7B);
    wr16(abs_db(bank, 0x11E1), cyc_then(60, k11e1));
    lda_imm(a, p, k11df);
    wr16(abs_db(bank, 0x11DF), cyc_then(22, k11df));
    cyc(42);
}

/* ResultsNoTime on carried registers. */
void ResultsNoTime_cr(Carried *cr)
{
    uint8_t p = cr->p;

    ResultsNoTime(cr->db, &cr->x, &cr->a, &cr->y, &p);
    set_p(cr, p);
}

void abi_ResultsNoTime(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ResultsNoTime_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:DE0B ResultsLapTimes
 * WHAT IT DOES: Shows the time of place +$22 (the bytes at $0F78, $0F77 and
 * $0F76 + it, two digits each) on object X's sprite group; a place with no
 * time ($FF) first blanks its line (ResultsNoTime). Then starts
 * ResultsLeaderBlink when +$20 is the leader's place and the championship
 * is over, otherwise stops (MenuActorNoStep).
 * HOW: MenuActorCopyGroup; ResultsBcdPair from the group's first entry
 * ($11E1); the place and its first byte kept on the stack.
 * Entry: M 16-bit, X 16-bit, D = $0000; X = the object. Exit: X = $11F9;
 * A and the flags from the last step.
 * From C: *x, *a, *y, *p the registers in and out.
 */
void ResultsLapTimes(Carried *cr, uint8_t bank, uint16_t *x, uint16_t *a, uint16_t *y, uint8_t *p)
{
    uint16_t place, first;

    cyc(46);
    *a = MenuActorCopyGroup(cr, bank, x, p);
    lda(a, p, rd16(cyc_then(22, fld(*x, 0x12))));
    lda(a, p, (uint16_t)(*a + 2));
    wr16(abs_db(bank, 0x11E1), cyc_then(52, *a));
    lda(a, p, rd16(cyc_then(22, fld(*x, 0x22))));
    tax(*a, x, p);
    place = *x;
    cyc(16);
    first = lda(a, p, rd16(0x000F78 + (uint32_t)place));
    and_imm(a, p, 0x00FF);
    cmp(*a, p, 0x00FF);
    if (*p & P_Z)
        { cyc(168); ResultsNoTime(bank, x, a, y, p); } else { cyc(128); }
    cyc(118);
    *x = place;
    lda(a, p, first);
    ResultsBcdPair(bank, *x, a, p);
    cyc(32);
    lda(a, p, rd16(0x000F77 + (uint32_t)*x));
    (cyc(46), ResultsBcdPair(bank, *x, a, p));
    cyc(32);
    lda(a, p, rd16(W(record_times) + (uint32_t)*x));
    (cyc(46), ResultsBcdPair(bank, *x, a, p));
    leader_or_stop(bank, x, a, p, 0x20);
}

/* ResultsLapTimes on carried registers. */
void ResultsLapTimes_cr(Carried *cr)
{
    uint8_t p = cr->p;

    ResultsLapTimes(cr, cr->db, &cr->x, &cr->a, &cr->y, &p);
    set_p(cr, p);
}

void abi_ResultsLapTimes(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ResultsLapTimes_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:E12E ResultsPointsTile
 * WHAT IT DOES: Puts tile A in the next sprite of the group at $11E1.
 * HOW: The tile through $11E3; X kept on the stack.
 * Entry: M 16-bit, X 16-bit; A = the tile. Exit: X as on entry, N, Z from
 * it; A = the new $11E1; C, V from its add.
 * From C: x the object (unchanged); *a, *p in and out.
 */
void ResultsPointsTile(uint8_t bank, uint16_t x, uint16_t *a, uint8_t *p)
{
    uint16_t x0 = x;

    wr16(abs_db(bank, 0x11E3), cyc_then(54, *a));
    tile_at(bank, &x, a, p, 0x11E1);
    *p = nz16(*p, x0);
    cyc(78);
}

/* ResultsPointsTile on carried registers. */
void ResultsPointsTile_cr(Carried *cr)
{
    uint8_t p = cr->p;

    ResultsPointsTile(cr->db, cr->x, &cr->a, &p);
    set_p(cr, p);
}

void abi_ResultsPointsTile(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ResultsPointsTile_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:E114 ResultsPointsPair
 * WHAT IT DOES: Puts the two decimal digits of the byte A (high, then low)
 * in the group at $11E1 as single tiles 0-9 (ResultsPointsTile).
 * HOW: The byte kept at $11E5.
 * Entry: M 16-bit, X 16-bit; A low byte = the digits. Exit: as
 * ResultsPointsTile.
 * From C: x the object (unchanged); *a, *p in and out.
 */
void ResultsPointsPair(uint8_t bank, uint16_t x, uint16_t *a, uint8_t *p)
{
    and_imm(a, p, 0x00FF);
    wr16(abs_db(bank, 0x11E5), cyc_then(46, *a));
    lsr(a, p);
    lsr(a, p);
    lsr(a, p);
    lsr(a, p);
    and_imm(a, p, 0x000F);
    (cyc(124), ResultsPointsTile(bank, x, a, p));
    lda(a, p, rd16(cyc_then(24, abs_db(bank, 0x11E5))));
    and_imm(a, p, 0x000F);
    (cyc(68), ResultsPointsTile(bank, x, a, p));
    cyc(42);
}

/* ResultsPointsPair on carried registers. */
void ResultsPointsPair_cr(Carried *cr)
{
    uint8_t p = cr->p;

    ResultsPointsPair(cr->db, cr->x, &cr->a, &p);
    set_p(cr, p);
}

void abi_ResultsPointsPair(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ResultsPointsPair_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:E0E0 ResultsPoints
 * WHAT IT DOES: Shows the points of car A (five digits: the low digit of
 * $0EFB + 3A, then the bytes $0EFA and $0EF9 + 3A) on object X's sprite
 * group.
 * HOW: MenuActorCopyGroup with A kept on the stack, then at $11DF; the
 * digits from the group's first entry ($11E1).
 * Entry: M 16-bit, X 16-bit, D = $0000; A = the car; X = the object. Exit:
 * X = $11F9, N, Z from it; A and C, V from the last step.
 * From C: *x, *a, *y, *p the registers in and out.
 */
void ResultsPoints(Carried *cr, uint8_t bank, uint16_t *x, uint16_t *a, uint8_t *p)
{
    uint16_t car = *a, at;

    cyc(76);
    *a = MenuActorCopyGroup(cr, bank, x, p);
    lda(a, p, car);
    wr16(abs_db(bank, 0x11DF), cyc_then(60, car));
    lda(a, p, rd16(cyc_then(22, fld(*x, 0x12))));
    lda(a, p, (uint16_t)(*a + 2));
    wr16(abs_db(bank, 0x11E1), cyc_then(52, *a));
    lda(a, p, rd16(cyc_then(24, abs_db(bank, 0x11DF))));
    asl(a, p);
    add(a, p, rd16(cyc_then(52, abs_db(bank, 0x11DF))));
    tax(*a, x, p);
    at = *x;
    cyc(16);
    lda(a, p, rd16(0x000EFB + (uint32_t)at));
    and_imm(a, p, 0x000F);
    (cyc(68), ResultsPointsTile(bank, *x, a, p));
    cyc(32);
    lda(a, p, rd16(0x000EFA + (uint32_t)*x));
    (cyc(46), ResultsPointsPair(bank, *x, a, p));
    cyc(32);
    lda(a, p, rd16(0x000EF9 + (uint32_t)*x));
    (cyc(46), ResultsPointsPair(bank, *x, a, p));
    *x = rd16(cyc_then(24, abs_db(bank, 0x11F9)));
    *p = nz16(*p, *x);
    cyc(42);
}

/* ResultsPoints on carried registers. */
void ResultsPoints_cr(Carried *cr)
{
    uint8_t p = cr->p;

    ResultsPoints(cr, cr->db, &cr->x, &cr->a, &p);
    set_p(cr, p);
}

void abi_ResultsPoints(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ResultsPoints_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:E14F ResultsPhaseAnim
 * WHAT IT DOES: Gives object X the animation for the race phase $0DD6: $1A4
 * for 6, $17C for $0A, $19A for $0C, $190 for $0E, $186 for $10; none for
 * the others. Reached by unit tests (unit_menus).
 * HOW: A compare chain; the animation is restarted (+$16 = +$18 = 0) only
 * when it changes.
 * Entry: M 16-bit, X 16-bit, D = $0000; X = the object. Exit: A = the
 * animation (or the phase), N, Z, C from the last compare.
 * From C: x the object (unchanged); *a, *p in and out.
 */
void ResultsPhaseAnim(uint16_t x, uint16_t *a, uint8_t *p)
{
    static const uint16_t phase[5] = {0x0006, 0x000A, 0x000C, 0x000E, 0x0010};
    static const uint16_t anim[5] = {0x01A4, 0x017C, 0x019A, 0x0190, 0x0186};
    unsigned i;

    byte_of(a, p, 0x000DD6);
    for (i = 0; i < 5; i++) {
        cyc(40);
        cmp(*a, p, phase[i]);
        if (*p & P_Z)
            break;
    }
    if (i == 5)
        { cyc(60); return; }
    lda(a, p, rd16(cyc_then(46, fld(x, 0x14))));
    cmp(*a, p, anim[i]);
    if (*p & P_Z)
        { cyc(64); return; } else { cyc(40); }
    set_field(x, a, p, 0x14, anim[i]);
    wr16(fld(x, 0x16), cyc_then(22, 0x0000));
    wr16(fld(x, 0x18), cyc_then(22, 0x0000));
    cyc(42);
}

/* ResultsPhaseAnim on carried registers. */
void ResultsPhaseAnim_cr(Carried *cr)
{
    uint8_t p = cr->p;

    ResultsPhaseAnim(cr->x, &cr->a, &p);
    set_p(cr, p);
}

void abi_ResultsPhaseAnim(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ResultsPhaseAnim_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* ResultsPlace0-3: place n's points object: after its count +$0A runs
   out it shows (+$10 = 0, +$08 = its animate handler) and, when the
   championship is over or place n is past $0D48, shows car n's points
   (ResultsPoints) and stops; at $0D48 = n it animates by the phase
   (ResultsPhaseAnim); before it, it hides again. The $0D48 cases are
   reached by unit tests (unit_menus), except place 0's hiding: $0D48 is
   never below 0, so the BCC after CMP #0 never branches. */
static void place_points(Carried *cr, uint8_t bank, uint16_t *x, uint16_t *a, uint8_t *p, uint16_t n, uint16_t handler)
{
    if (!(dec_fld(*x, p, 0x0A) & 0x8000))
        { cyc(64); return; }
    wr16(fld(*x, 0x10), cyc_then(38, 0x0000));
    set_field(*x, a, p, 0x08, handler);
    byte_of(a, p, 0x000DD6);
    cmp(*a, p, 0x0008);
    if (!(*p & P_Z)) {
        cyc(58);
        byte_of(a, p, W(wram_0D48));
        cmp(*a, p, n);
        if (*p & P_Z) {
            cyc(110);
            ResultsPhaseAnim(*x, a, p);
            cyc(42);
            return;
        } else { cyc(*p & P_C ? 50 : 80); }
        if (!(*p & P_C)) {
            set_field(*x, a, p, 0x10, 0xFFFF);
            cyc(42);
            return;
        }
    } else { cyc(18); }
    cyc(78);
    lda_imm(a, p, n);
    ResultsPoints(cr, bank, x, a, p);
    set_field(*x, a, p, 0x08, 0xE0DF);
    cyc(42);
}

/*
 * @routine 0B:DFDB ResultsPlace0
 * WHAT IT DOES: Place 0's points object (see place_points in the C):
 * counts +$0A down, then shows, with ResultsPlace0Animate as its handler,
 * and shows car 0's points or animates by $0D48 (the hiding below it can
 * never run: the BCC after CMP #0 never branches).
 * HOW: As the other places, with 0.
 * Entry: M 16-bit, X 16-bit, D = $0000; X = the object. Exit: counting: N,
 * Z from the count; else as ResultsPoints, ResultsPhaseAnim or the hiding
 * store (A = $FFFF).
 */
void ResultsPlace0(Carried *cr, uint8_t bank, uint16_t *x, uint16_t *a, uint8_t *p)
{
    place_points(cr, bank, x, a, p, 0x0000, 0xE018);
}

/* ResultsPlace0 on carried registers. */
void ResultsPlace0_cr(Carried *cr)
{
    uint8_t p = cr->p;

    ResultsPlace0(cr, cr->db, &cr->x, &cr->a, &p);
    set_p(cr, p);
}

void abi_ResultsPlace0(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ResultsPlace0_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:E006 ResultsPlace0Hide
 * WHAT IT DOES: ResultsPlace0's hiding store, which nothing reaches (the
 * BCC after CMP #0 before it never branches): +$10 = $FFFF.
 * HOW: A 16-bit direct page store indexed by the object.
 * Entry: M 16-bit, D = $0000; X = the object. Exit: A = $FFFF; N set, Z
 * clear.
 */
void ResultsPlace0Hide(uint16_t x, uint16_t *a, uint8_t *p)
{
    set_field(x, a, p, 0x10, 0xFFFF);
    cyc(42);
}

/* ResultsPlace0Hide on carried registers. */
void ResultsPlace0Hide_cr(Carried *cr)
{
    uint8_t p = cr->p;

    ResultsPlace0Hide(cr->x, &cr->a, &p);
    set_p(cr, p);
}

void abi_ResultsPlace0Hide(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ResultsPlace0Hide_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:E01C ResultsPlace1
 * WHAT IT DOES: As ResultsPlace0 for place 1 (handler ResultsPlace1Animate).
 * HOW: As ResultsPlace0, with 1.
 * Entry: as ResultsPlace0. Exit: as ResultsPlace0.
 */
void ResultsPlace1(Carried *cr, uint8_t bank, uint16_t *x, uint16_t *a, uint8_t *p)
{
    place_points(cr, bank, x, a, p, 0x0001, 0xE059);
}

/* ResultsPlace1 on carried registers. */
void ResultsPlace1_cr(Carried *cr)
{
    uint8_t p = cr->p;

    ResultsPlace1(cr, cr->db, &cr->x, &cr->a, &p);
    set_p(cr, p);
}

void abi_ResultsPlace1(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ResultsPlace1_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:E05D ResultsPlace2
 * WHAT IT DOES: As ResultsPlace0 for place 2 (handler ResultsPlace2Animate).
 * HOW: As ResultsPlace0, with 2.
 * Entry: as ResultsPlace0. Exit: as ResultsPlace0.
 */
void ResultsPlace2(Carried *cr, uint8_t bank, uint16_t *x, uint16_t *a, uint8_t *p)
{
    place_points(cr, bank, x, a, p, 0x0002, 0xE09A);
}

/* ResultsPlace2 on carried registers. */
void ResultsPlace2_cr(Carried *cr)
{
    uint8_t p = cr->p;

    ResultsPlace2(cr, cr->db, &cr->x, &cr->a, &p);
    set_p(cr, p);
}

void abi_ResultsPlace2(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ResultsPlace2_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:E09E ResultsPlace3
 * WHAT IT DOES: As ResultsPlace0 for place 3 (handler ResultsPlace3Animate).
 * HOW: As ResultsPlace0, with 3.
 * Entry: as ResultsPlace0. Exit: as ResultsPlace0.
 */
void ResultsPlace3(Carried *cr, uint8_t bank, uint16_t *x, uint16_t *a, uint8_t *p)
{
    place_points(cr, bank, x, a, p, 0x0003, 0xE0DB);
}

/* ResultsPlace3 on carried registers. */
void ResultsPlace3_cr(Carried *cr)
{
    uint8_t p = cr->p;

    ResultsPlace3(cr, cr->db, &cr->x, &cr->a, &p);
    set_p(cr, p);
}

void abi_ResultsPlace3(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ResultsPlace3_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:E018 ResultsPlace0Animate
 * WHAT IT DOES: Place 0's points object's frame while animating:
 * MenuActorAnimateFrame.
 * HOW: JSR, RTS.
 * Entry: as MenuActorAnimateFrame. Exit: as MenuActorAnimateFrame.
 */
uint16_t ResultsPlace0Animate(uint8_t bank, uint16_t *x, uint8_t *p)
{
    uint16_t a = MenuActorAnimateFrame(bank, x, p);

    cyc(42);
    return a;
}

/* ResultsPlace0Animate on carried registers. */
void ResultsPlace0Animate_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = ResultsPlace0Animate(cr->db, &cr->x, &p);
    set_p(cr, p);
}

void abi_ResultsPlace0Animate(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ResultsPlace0Animate_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:E059 ResultsPlace1Animate
 * WHAT IT DOES: As ResultsPlace0Animate, for place 1.
 * HOW: JSR, RTS.
 * Entry: as MenuActorAnimateFrame. Exit: as MenuActorAnimateFrame.
 */
uint16_t ResultsPlace1Animate(uint8_t bank, uint16_t *x, uint8_t *p)
{
    uint16_t a = MenuActorAnimateFrame(bank, x, p);

    cyc(42);
    return a;
}

/* ResultsPlace1Animate on carried registers. */
void ResultsPlace1Animate_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = ResultsPlace1Animate(cr->db, &cr->x, &p);
    set_p(cr, p);
}

void abi_ResultsPlace1Animate(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ResultsPlace1Animate_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:E09A ResultsPlace2Animate
 * WHAT IT DOES: As ResultsPlace0Animate, for place 2.
 * HOW: JSR, RTS.
 * Entry: as MenuActorAnimateFrame. Exit: as MenuActorAnimateFrame.
 */
uint16_t ResultsPlace2Animate(uint8_t bank, uint16_t *x, uint8_t *p)
{
    uint16_t a = MenuActorAnimateFrame(bank, x, p);

    cyc(42);
    return a;
}

/* ResultsPlace2Animate on carried registers. */
void ResultsPlace2Animate_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = ResultsPlace2Animate(cr->db, &cr->x, &p);
    set_p(cr, p);
}

void abi_ResultsPlace2Animate(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ResultsPlace2Animate_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 0B:E0DB ResultsPlace3Animate
 * WHAT IT DOES: As ResultsPlace0Animate, for place 3.
 * HOW: JSR, RTS.
 * Entry: as MenuActorAnimateFrame. Exit: as MenuActorAnimateFrame.
 */
uint16_t ResultsPlace3Animate(uint8_t bank, uint16_t *x, uint8_t *p)
{
    uint16_t a = MenuActorAnimateFrame(bank, x, p);

    cyc(42);
    return a;
}

/* ResultsPlace3Animate on carried registers. */
void ResultsPlace3Animate_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = ResultsPlace3Animate(cr->db, &cr->x, &p);
    set_p(cr, p);
}

void abi_ResultsPlace3Animate(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ResultsPlace3Animate_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}
