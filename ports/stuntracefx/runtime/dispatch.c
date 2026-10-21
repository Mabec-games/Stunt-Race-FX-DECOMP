/*
 * dispatch.c: what the decomp's C asks of the machine beyond the bus
 * (machine.h, spc.h): the routines it reaches through computed addresses,
 * the two ends no C follows, the interrupts, and where each processor's C
 * starts.
 *
 *   call_routine, jump_routine   the routine with C that starts at the
 *                                address (the table from the tags,
 *                                gen/routines.c); none is the original's
 *                                own runaway (a table read past its end)
 *   cpu_loop_forever             time passes, interrupts run, for ever
 *   cpu_restart                  every native frame dropped (a jump back
 *                                to the top of the S-CPU's run), then the
 *                                entry's C
 *   interrupts                   the vector's code in bank 0 as the CPU
 *                                runs it: the stubs the game copies to
 *                                $0101 (NOPs, a JSL to the handler's C,
 *                                RTI), with the frame the interrupt pushes
 *   Super FX                     a program's C at PBR:R15
 *   SPC700                       the routine with C at the APU address
 *                                whose code the APU RAM holds (the race and
 *                                battle drivers share addresses)
 *
 * PORT_CALLTRACE=FILE: each routine call_routine and jump_routine reach
 * (and each interrupt handler), once, into FILE as BB:AAAA how NAME: the
 * routines the game reaches through computed addresses.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "machine.h"
#include "routines.h"
#include "snes.h"
#include "jump.h"
#include "dispatch.h"

Regs65816 cpu;

static port_jmp_buf top;
static void (*next_entry)(void);
static int in_interrupt;

static void traced(const char *how, const PortRoutine *r)
{
    static FILE *f;
    static int on = -1;
    static uint8_t seen[0x10000];
    if (on < 0) {
        const char *path = getenv("PORT_CALLTRACE");
        on = path && (f = fopen(path, "a")) != NULL;
    }
    if (!on || seen[(r - port_routines) & 0xFFFF])
        return;
    seen[(r - port_routines) & 0xFFFF] = 1;
    fprintf(f, "%02X:%04X %s %s\n", (unsigned)(r->addr >> 16), (unsigned)(r->addr & 0xFFFF), how, r->name);
    fflush(f);
}

static const PortRoutine *find_any(uint32_t addr)
{
    int lo = 0, hi = port_routine_count - 1;
    addr &= 0xFFFFFF;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        if (port_routines[mid].addr == addr)
            return &port_routines[mid];
        if (port_routines[mid].addr < addr)
            lo = mid + 1;
        else
            hi = mid - 1;
    }
    return NULL;
}

/* the routine that starts at addr, for a computed call: NULL when none
   does; fatal when one does but its C is only called by name (no abi_
   adapter: the plays never reached it this way) */
static const PortRoutine *find(uint32_t addr)
{
    const PortRoutine *r = find_any(addr);
    if (r && !r->abi)
        snes_fatal("the game reaches %s ($%02X:%04X) through an address, and its C has no abi_ adapter for that",
                   r->name, (unsigned)((addr >> 16) & 0xFF), (unsigned)(addr & 0xFFFF));
    return r;
}

/* The game reached an address with no routine: on the console the CPU
   would run whatever bytes are there (the original's own crash). */
static void runaway(const char *how, uint32_t addr)
{
    snes_fatal("the game %s $%02X:%04X, where no routine starts (the original runs off its program here)",
               how, (unsigned)(addr >> 16), (unsigned)(addr & 0xFFFF));
}

void call_routine(uint32_t addr, int long_call)
{
    const PortRoutine *r = find(addr);
    (void)long_call;
    if (!r)
        runaway("calls", addr);
    traced("call", r);
    r->abi();
}

void jump_routine(uint32_t addr)
{
    const PortRoutine *r = find(addr);
    if (!r)
        runaway("jumps to", addr);
    traced("jump", r);
    r->abi();
}

void jump_routine_pushed(uint32_t addr, const uint8_t *pushed, int n)
{
    const PortRoutine *r = find(addr);
    int k;
    if (!r)
        runaway("jumps to", addr);
    traced("jump", r);
    for (k = 0; k < n; k++)
        wr8((uint16_t)(cpu.s - k), pushed[k]);
    cpu.s = (uint16_t)(cpu.s - n);
    r->abi();
}

/* the clocks the fetch of a code byte at `at` takes */
static unsigned fetch(uint32_t at) { return snes_access_clocks(at); }

void cpu_loop_forever(uint32_t at)
{
    (void)at;
    in_interrupt = 0;
    for (;;) {
        /* BRA to itself: its opcode and operand fetched, an internal cycle */
        snes_cpu_clocks(fetch(at) + fetch(at + 1) + 6);
        snes_cpu_interrupts();
    }
}

void cpu_idle(uint32_t clocks)
{
    snes_cpu_wait(clocks);
}

void cyc(uint32_t clocks)
{
    snes_cpu_clocks(clocks);
}

void cpu_restart(uint32_t at, void (*entry)(void))
{
    (void)at;
    next_entry = entry;
    in_interrupt = 0;
    port_longjmp(top);
}

/* ---- interrupts ---- */
static void push8(uint8_t v)
{
    wr8((uint16_t)cpu.s, v);
    cpu.s--;
}

static uint8_t pull8(void)
{
    cpu.s++;
    return rd8((uint16_t)cpu.s);
}

/* The vector's code (the stubs at $0101) has no C: its time is its
   instructions', cycle by cycle as the 65816 runs them in native mode. */
static void take_interrupt(int nmi)
{
    Regs65816 saved = cpu;
    uint32_t vector = nmi ? 0x00FFEA : 0x00FFEE;
    uint16_t at = (uint16_t)(snes_peek(vector) | snes_peek(vector + 1) << 8);
    int steps;
    /* the interrupt's sequence: two internal cycles; the frame it pushes,
       PB, PC, P; the vector read; then I set, D clear */
    snes_cpu_clocks(6 + 6);
    push8((uint8_t)(cpu.pc >> 16));
    push8((uint8_t)(cpu.pc >> 8));
    push8((uint8_t)cpu.pc);
    push8(cpu.p);
    snes_cpu_clocks(fetch(vector) + fetch(vector + 1));
    cpu.p = (uint8_t)((cpu.p | P_I) & ~P_D);
    cpu.pc = at;
    in_interrupt++;
    for (steps = 0; steps < 64; steps++) {
        uint8_t op = snes_peek(at);
        if (op == 0xEA) {                     /* NOP: fetch, internal cycle */
            snes_cpu_clocks(fetch(at) + 6);
            at++;
        } else if (op == 0x22 || op == 0x5C) {  /* JSL, JML to the handler */
            uint32_t t = snes_peek((uint16_t)(at + 1)) | snes_peek((uint16_t)(at + 2)) << 8 |
                         (uint32_t)snes_peek((uint16_t)(at + 3)) << 16;
            const PortRoutine *r = find(t);
            if (!r)
                runaway("takes an interrupt to", t);
            if (op == 0x22) {
                /* JSL: opcode and address fetched, PB pushed, an internal
                   cycle, the bank fetched, PC pushed; the handler's C
                   states its RTL */
                uint16_t ret = (uint16_t)(at + 3);
                snes_cpu_clocks(fetch(at) + fetch(at + 1) + fetch(at + 2));
                push8(0x00);
                snes_cpu_clocks(6 + fetch(at + 3));
                push8((uint8_t)(ret >> 8));
                push8((uint8_t)ret);
                traced("interrupt", r);
                r->abi();
                if (cpu.unwound)
                    snes_fatal("an interrupt handler left through its caller's frame");
                pull8();
                pull8();
                pull8();
                at = (uint16_t)(at + 4);
            } else {
                /* JML: opcode and address fetched */
                snes_cpu_clocks(fetch(at) + fetch(at + 1) + fetch(at + 2) + fetch(at + 3));
                traced("interrupt", r);
                r->abi();
                break;
            }
        } else if (op == 0x40) {              /* RTI */
            /* fetch, two internal cycles, P, PC and PB pulled */
            snes_cpu_clocks(fetch(at) + 6 + 6);
            pull8();
            pull8();
            pull8();
            pull8();
            break;
        } else {
            snes_fatal("interrupt code at $00:%04X: opcode %02X not handled", at, op);
        }
    }
    in_interrupt--;
    cpu = saved;
}

static int irq_masked(void) { return (cpu.p & P_I) != 0; }

/* ---- the Super FX and the SPC700 ---- */
static void gsu_run(uint8_t bank, uint16_t addr)
{
    const PortRoutine *r = find((uint32_t)bank << 16 | addr);
    if (!r)
        snes_fatal("Super FX program at $%02X:%04X has no C", bank, addr);
    traced("gsu", r);
    r->abi();
}

static void spc_run(uint16_t addr)
{
    const uint8_t *aram = snes_aram();
    const uint8_t *rom = port_rom();
    int i;
    for (i = 0; i < port_spc_routine_count; i++) {
        const PortSpcRoutine *r = &port_spc_routines[i];
        const PortSpcUse *u;
        int ok = 1;
        if (r->apu != addr)
            continue;
        for (u = r->uses; ok && u->len; u++)
            ok = !memcmp(aram + u->apu, rom + u->rom, u->len);
        if (ok) {
            r->abi();
            return;
        }
    }
    snes_fatal("SPC700 code at $%04X: no routine with C matches the APU's RAM", addr);
}

/* ---- the S-CPU's run ---- */
void port_start(void)
{
    static const SnesSpcHooks spc_hooks = { spc_run };
    snes_interrupt_hook = take_interrupt;
    snes_irq_masked = irq_masked;
    snes_gsu_run_hook = gsu_run;
    snes_spc_set_hooks(&spc_hooks);
    /* the reset: emulation mode, 8-bit registers, I set, the stack in page
       1, the reset vector's code */
    memset(&cpu, 0, sizeof cpu);
    cpu.e = 1;
    cpu.p = P_M | P_X | P_I;
    cpu.s = 0x01FF;
    {
        uint16_t v = (uint16_t)(snes_peek(0x00FFFC) | snes_peek(0x00FFFD) << 8);
        const PortRoutine *r = find(v);
        if (!r)
            snes_fatal("no C at the reset vector $00:%04X", v);
        cpu.pc = v;
        next_entry = r->abi;
    }
    for (;;) {
        if (port_setjmp(top) == 0) {
            next_entry();
            snes_fatal("the game's top-level routine returned");
        }
    }
}
