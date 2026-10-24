/*
 * shim.c: the port's native interfaces (game/include/native.h, draw_list.h)
 * as the verification harness needs them, for the decomp's gate on the
 * port's copy of the game (ports/stuntracefx/tools/gate.py). The harness
 * runs the console, so each access goes to its bus as the original made
 * it: a timed register read or write is that read or write (a read gives
 * the bus's value), and the native parts the port feeds from those
 * accesses (the picture, the transfers, the 3D canvas) do nothing here:
 * the write that stands for them has gone to the bus already. Built into
 * the harness's library only, never into the port.
 */
#include <stdint.h>

#include "machine.h"
#include "draw_list.h"
#include "carried.h"

/* computed calls from carried code: none taken as carried here, so each
   goes through the routine's adapter and the harness's own dispatch (the
   same C: an adapter loads the registers and calls the carried form) */
CarriedFn carried_at(uint32_t addr)
{
    (void)addr;
    return 0;
}

/* the emulation flag, as the port's runtime keeps it */
uint8_t cpu_mode(void) { return cpu.e; }
void cpu_mode_set(uint8_t e) { cpu.e = e; }
void cpu_leave_to(uint32_t pc) { cpu.pc = pc; cpu.unwound = 1; }

/* the interrupt mask is P's I flag, as the port's runtime keeps it */
void irq_mask_set(int masked)
{
    if (masked)
        cpu.p |= P_I;
    else
        cpu.p &= (uint8_t)~P_I;
}

uint8_t io_read(uint32_t addr, uint8_t value)
{
    (void)value;
    return rd8(addr);
}

void io_write_time(uint32_t addr, uint8_t value) { wr8(addr, value); }
uint8_t hw_rd(uint32_t addr) { return rd8(addr); }
void hw_wr(uint32_t addr, uint8_t v) { wr8(addr, v); }

/* the joypads: the value io_read takes from the bus is the one used */
uint16_t input_pad(int port)
{
    (void)port;
    return 0;
}

void vid_reg(uint8_t reg, uint8_t v) { (void)reg; (void)v; }
void dma_reg(uint8_t reg, uint8_t v) { (void)reg; (void)v; }
void dma_go(uint8_t channels) { (void)channels; }
void hdma_set(uint8_t channels) { (void)channels; }

void fx_list_begin(void) {}
void fx_list_end(void) {}
void fx_op_clear(void) {}
void fx_op_span(const FxSpan *s) { (void)s; }
void fx_op_palette_span(FxSpan *s, uint16_t palette) { (void)s; (void)palette; }
void fx_op_ramp_span(FxSpan *s, uint16_t ramp) { (void)s; (void)ramp; }
void fx_op_upload(int on) { (void)on; }

/* The ends of carried code (carried.h), as the harness needs them: the
   machine it hands on to is the original's, on the register file; the
   harness runs the original at a restart or a branch to itself and comes
   back to the C. */
/* A computed call or jump: carried_at gives none here, so each goes
   through the routine's adapter with the registers loaded and taken back
   (call_routine or jump_routine find it as the original's code would). */
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
