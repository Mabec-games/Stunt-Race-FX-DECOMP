/*
 * routines.h: the decomp's routines by address (gen/routines.c, from the
 * tags: ports/stuntracefx/tools/routines.py).
 */
#ifndef SRFX_PORT_ROUTINES_H
#define SRFX_PORT_ROUTINES_H

#include <stdint.h>

typedef struct {
    uint32_t addr;
    void (*abi)(void);
    const char *name;
} PortRoutine;

/* sorted by address */
extern const PortRoutine port_routines[];
extern const int port_routine_count;

/* the routines with C on carried registers (carried.h), by address */
struct Carried;
typedef struct {
    uint32_t addr;
    void (*fn)(struct Carried *);
} PortCarried;

/* sorted by address */
extern const PortCarried port_carried[];
extern const int port_carried_count;

/* the ROM bytes an SPC700 routine's C depends on: runs of len bytes at
   APU RAM apu, uploaded from ROM offset rom; ends with len 0 */
typedef struct {
    uint16_t apu, len;
    uint32_t rom;
} PortSpcUse;

typedef struct {
    uint16_t apu;
    void (*abi)(void);
    const char *name;
    const PortSpcUse *uses;
} PortSpcRoutine;

/* sorted by APU address */
extern const PortSpcRoutine port_spc_routines[];
extern const int port_spc_routine_count;

#endif
