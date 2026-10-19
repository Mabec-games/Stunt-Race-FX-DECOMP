/*
 * dispatch.h: the port's runtime under the decomp's C (dispatch.c) and the
 * front end over it (main.c).
 */
#ifndef SRFX_PORT_DISPATCH_H
#define SRFX_PORT_DISPATCH_H

#include <stdint.h>

/* the cartridge's ROM (1 MB) */
const uint8_t *port_rom(void);
/* power on and run the game's C from the reset vector; never returns */
void port_start(void);

#endif
