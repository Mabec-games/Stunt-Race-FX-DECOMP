/*
 * machine.h: the machine as the decomp's C sees it.
 *
 * The C reads and writes the S-CPU's 24-bit address space through rd8/wr8,
 * exactly as the original code does, so a routine's C has the same inputs
 * and outputs as its assembly. Inside the verification harness these go to
 * the emulated machine; a native port supplies its own implementation.
 *
 * Routines that assembly still calls also need the register file: each such
 * routine has an abi_ wrapper that takes its inputs from `cpu` and leaves its
 * outputs there, as its entry contract states.
 */
#ifndef STUNTRACEFX_MACHINE_H
#define STUNTRACEFX_MACHINE_H

#include <stdint.h>

uint8_t rd8(uint32_t addr);
void wr8(uint32_t addr, uint8_t value);

/* Multi-byte accesses are little-endian and linear in the 24-bit space,
   the low byte first, as the CPU makes them (lockstep places each access
   on the original's, so the order is part of the behaviour). */
static inline uint16_t rd16(uint32_t addr)
{
    uint8_t lo = rd8(addr);

    return (uint16_t)(lo | rd8((addr + 1) & 0xFFFFFF) << 8);
}

static inline void wr16(uint32_t addr, uint16_t value)
{
    wr8(addr, (uint8_t)value);
    wr8((addr + 1) & 0xFFFFFF, (uint8_t)(value >> 8));
}

static inline uint32_t rd24(uint32_t addr)
{
    return rd16(addr) | (uint32_t)rd8((addr + 2) & 0xFFFFFF) << 16;
}

/* The 65816 register file, for abi_ wrappers. */
typedef struct {
    uint16_t a, x, y, s, d;
    uint8_t db, p, e;
    uint32_t pc;
    /* Set by C that leaves through its caller's frame (pulling the
       caller's saved registers and return address): s and pc are then
       where it lands, not the routine's own return. */
    uint8_t unwound;
} Regs65816;

extern Regs65816 cpu;

enum {
    P_C = 0x01, P_Z = 0x02, P_I = 0x04, P_D = 0x08,
    P_X = 0x10, P_M = 0x20, P_V = 0x40, P_N = 0x80,
};

/* N and Z as a 16-bit load sets them. */
static inline uint8_t nz16(uint8_t p, uint16_t v)
{
    p &= (uint8_t)~(P_N | P_Z);
    if (v == 0) p |= P_Z;
    if (v & 0x8000) p |= P_N;
    return p;
}

/* N and Z as an 8-bit load (or pull) sets them. */
static inline uint8_t nz8(uint8_t p, uint8_t v)
{
    p &= (uint8_t)~(P_N | P_Z);
    if (v == 0) p |= P_Z;
    if (v & 0x80) p |= P_N;
    return p;
}

/* Call the routine at `addr` (a JSL when `long_call`, else a JSR) with the
   register file in `cpu` as its entry state, leaving its exit state there:
   how the C makes the calls the original makes through an address it
   computes (handler tables, addresses pushed for an RTL): the abi_
   wrapper of the routine with C that starts there. (Every routine has C;
   the verification harness fails a run in which C reaches an address where
   none starts. A computed address past a table's end, which the game
   never makes, is the original's own runaway.) */
void call_routine(uint32_t addr, int long_call);
/* The original JMPs to addr (a jump table's handler, a tail jump): run it
   in the calling routine's own frame, to wherever it leaves that frame (its
   return, or through its caller's). The calling routine's last action. */
void jump_routine(uint32_t addr);
/* As jump_routine, for code the original reaches with values it pushed
   first (PHP, PHB...) still on the stack: pushed[0] the first pushed. */
void jump_routine_pushed(uint32_t addr, const uint8_t *pushed, int n);
/* A branch to itself (BRA to its own address, at `at`): the CPU never goes
   on. It makes no access, so nothing changes but time; the interrupts the
   machine raises still run between its passes. Never returns. (`at` is
   where the loop is: the verification harness finds it in the original's
   record there.) */
void cpu_loop_forever(uint32_t at);
/* The original jumps to `at`, code that resets the stack (LDX #$02FF,
   TXS): every frame of the calls under it is gone, the C's own included,
   and `entry`, the C of the code at `at`, runs from there with the
   registers in `cpu`. Never returns. (The verification harness ends the call it
   runs in there and follows the original from `at`; `entry` is checked on
   its own, by unit calls.) */
void cpu_restart(uint32_t at, void (*entry)(void));
/* Time the original spends with no access the C makes: a delay loop's
   turns (its fetches and internal cycles), `clocks` master clocks (21.477
   MHz). The rest of the machine runs meanwhile, so the time matters: the
   sound CPU acting on a command before the next one arrives. */
void cpu_idle(uint32_t clocks);
/* The time the original spends here between two bus accesses, besides
   their own: its instruction fetches and internal cycles, `clocks` master
   clocks. The C states it where its code stands for the original's, so
   each access comes at the original's time. */
void cyc(uint32_t clocks);
/* `v`, with the time before an access stated after the access's other
   arguments are worked out: where the access is reached in the middle of
   an expression, so no statement before it stands for that place. */
static inline uint32_t cyc_then(uint32_t clocks, uint32_t v)
{
    cyc(clocks);
    return v;
}

#endif
