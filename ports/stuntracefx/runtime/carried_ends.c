/*
 * carried_ends.c: where code on carried registers (game/include/carried.h)
 * hands on to the machine: a call or jump through a computed address, a
 * jump with bytes pushed, the branch to itself and the restart. A target
 * with a carried form runs on the carried copy; anything else the machine
 * runs (dispatch.c) takes its registers from the register file, so the
 * copy goes there first and comes back after.
 */
#include <stdint.h>

#include "machine.h"
#include "carried.h"

static void regs_load(const Carried *cr)
{
    cpu.a = cr->a;
    cpu.x = cr->x;
    cpu.y = cr->y;
    cpu.d = cr->d;
    cpu.p = cr->p;
    cpu.db = cr->db;
}

static void regs_take(Carried *cr)
{
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

    if (f) {
        f(cr);
        return;
    }
    regs_load(cr);
    call_routine(addr, long_call);
    regs_take(cr);
}

/* JMP (abs,X) and the like: the routine at addr, whose return is the
   caller's (the C caller returns after it). */
void carried_jump(Carried *cr, uint32_t addr)
{
    CarriedFn f = carried_at(addr);

    if (f) {
        f(cr);
        return;
    }
    regs_load(cr);
    jump_routine(addr);
    regs_take(cr);
}

/* A jump with bytes pushed for the routine it reaches. */
void carried_jump_pushed(Carried *cr, uint32_t addr, const uint8_t *pushed, int n)
{
    regs_load(cr);
    jump_routine_pushed(addr, pushed, n);
    regs_take(cr);
}

/* A branch to itself: the interrupts that run meanwhile find the
   registers as the code left them. Never returns. */
void carried_loop_forever(Carried *cr, uint32_t at)
{
    regs_load(cr);
    cpu_loop_forever(at);
}

/* A jump to code that resets the stack: the entry starts on the
   registers. Never returns. */
void carried_restart(Carried *cr, uint32_t at, void (*entry)(void))
{
    regs_load(cr);
    cpu_restart(at, entry);
}
