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
#define VMAIN 0x002115u    /* VRAM address increment mode */
#define VMADDL 0x002116u   /* VRAM word address */
#define OAMADDL 0x002102u  /* OAM address */
#define OAMADDH 0x002103u
#define CGADD 0x002121u    /* CGRAM (palette) address */
#define MDMAEN 0x00420Bu   /* DMA enable, one bit per channel */
#define DMAP0 0x004300u    /* DMA channel 0: mode, then B-bus register (BBAD0) */
#define A1T0L 0x004302u    /* DMA channel 0: A-bus address */
#define A1B0 0x004304u     /* DMA channel 0: A-bus bank */
#define DAS0L 0x004305u    /* DMA channel 0: byte count */
#define GSU_SCMR 0x00303Au /* Super FX screen mode: who owns ROM (bit 4) and RAM (bit 3) */

#endif
