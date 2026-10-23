/*
 * computed.c: calls and jumps carried code makes through an address it
 * computes (a table of routine addresses in the ROM, a value the game
 * stores): the routine that starts there, on the carried registers.
 */
#include "machine.h"
#include "native.h"
#include "carried.h"

/* A routine with no carried form yet: its adapter, with the registers
   loaded and taken back (call_routine or jump_routine find it as the
   original's code would). Goes when every routine reached this way has a
   carried form. */
static void through_registers(Carried *cr, uint32_t addr, int jump, int long_call)
{
    cpu.a = cr->a;
    cpu.x = cr->x;
    cpu.y = cr->y;
    cpu.d = cr->d;
    cpu.p = cr->p;
    cpu.db = cr->db;
    if (jump)
        jump_routine(addr);
    else
        call_routine(addr, long_call);
    cr->a = cpu.a;
    cr->x = cpu.x;
    cr->y = cpu.y;
    cr->d = cpu.d;
    cr->p = cpu.p;
    cr->db = cpu.db;
}

/* JSR (abs,X), JSL through a table and the like: the routine at addr. */
void carried_call(Carried *cr, uint32_t addr, int long_call)
{
    CarriedFn f = carried_at(addr);

    if (f)
        f(cr);
    else
        through_registers(cr, addr, 0, long_call);
}

/* JMP (abs,X) and the like: the routine at addr, whose return is the
   caller's (the C caller returns after it). */
void carried_jump(Carried *cr, uint32_t addr)
{
    CarriedFn f = carried_at(addr);

    if (f)
        f(cr);
    else
        through_registers(cr, addr, 1, 0);
}

/* The registers back: the port's runtime never returns from a restart or
   a branch to itself, but the decomp's gate runs the original there and
   comes back to the C with the original's registers, which the carried
   caller's adapter then hands on. */
static void take(Carried *cr)
{
    cr->a = cpu.a;
    cr->x = cpu.x;
    cr->y = cpu.y;
    cr->d = cpu.d;
    cr->p = cpu.p;
    cr->db = cpu.db;
}

static void load(const Carried *cr)
{
    cpu.a = cr->a;
    cpu.x = cr->x;
    cpu.y = cr->y;
    cpu.d = cr->d;
    cpu.p = cr->p;
    cpu.db = cr->db;
}

/* A jump with bytes pushed for the routine it reaches (jump_routine_pushed):
   the routine runs on the registers, then the caller has them back. */
void carried_jump_pushed(Carried *cr, uint32_t addr, const uint8_t *pushed, int n)
{
    load(cr);
    jump_routine_pushed(addr, pushed, n);
    cr->a = cpu.a;
    cr->x = cpu.x;
    cr->y = cpu.y;
    cr->d = cpu.d;
    cr->p = cpu.p;
    cr->db = cpu.db;
}

/* A branch to itself: interrupts go on taking the registers as they are. */
void carried_loop_forever(Carried *cr, uint32_t at)
{
    load(cr);
    cpu_loop_forever(at);
    take(cr);
}

/* A jump to code that resets the stack: the entry starts on the registers. */
void carried_restart(Carried *cr, uint32_t at, void (*entry)(void))
{
    load(cr);
    cpu_restart(at, entry);
    take(cr);
}
