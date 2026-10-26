#ifndef PORT_SMOKE_H
#define PORT_SMOKE_H
#include <stdint.h>
void smoke_init(int headless);
void smoke_input(long frame, uint16_t *p0, uint16_t *p1);
void smoke_gsu(uint32_t program, int done, const uint16_t *objects, int count);
void smoke_present(long frame, const uint32_t *pixels);
#endif
