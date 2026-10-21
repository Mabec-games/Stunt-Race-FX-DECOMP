/* mixer_regs.h: the drivers' sound chip registers as mixer calls
   (mixer_regs.c), a step on the way to the drivers calling the mixer. */
#ifndef PORT_MIXER_REGS_H
#define PORT_MIXER_REGS_H

#include <stdint.h>

void mix_regs_reset(void);
void mix_reg_write(uint8_t reg, uint8_t v);
uint8_t mix_reg_read(uint8_t reg);

#endif
