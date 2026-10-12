/*
 * hw.h: S-CPU hardware registers used by the code, by their standard names,
 * as 24-bit S-CPU addresses. The same names as asm/stuntracefx/include/hw.inc.
 */
#ifndef STUNTRACEFX_HW_H
#define STUNTRACEFX_HW_H

#define APUIO0 0x002140u  /* APU ports 0-3: S-CPU <-> SPC700 mailbox */
#define APUIO1 0x002141u
#define APUIO2 0x002142u
#define APUIO3 0x002143u
#define HVBJOY 0x004212u  /* status: V-blank (bit 7), H-blank (bit 6), joypad busy (bit 0) */

#endif
