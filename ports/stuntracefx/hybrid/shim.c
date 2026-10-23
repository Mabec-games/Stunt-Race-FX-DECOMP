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
