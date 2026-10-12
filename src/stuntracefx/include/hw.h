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
#define INIDISP 0x002100u   /* screen brightness and forced blank */
#define OAMDATA 0x002104u   /* OAM data write */
#define BG2HOFS 0x00210Fu   /* BG2 horizontal scroll (written twice) */
#define BG2VOFS 0x002110u   /* BG2 vertical scroll (written twice) */
#define M7A 0x00211Bu       /* Mode 7 matrix A, also the multiplier's 16-bit factor (written twice) */
#define M7B 0x00211Cu       /* Mode 7 matrix B, also the multiplier's 8-bit factor */
#define CGDATA 0x002122u    /* CGRAM data write */
#define W12SEL 0x002123u    /* window masks for BG1/BG2 */
#define W34SEL 0x002124u    /* window masks for BG3/BG4 */
#define WOBJSEL 0x002125u   /* window masks for sprites and colour */
#define WBGLOG 0x00212Au    /* window logic for BGs */
#define WOBJLOG 0x00212Bu   /* window logic for sprites and colour */
#define TMW 0x00212Eu       /* main screen window mask enables */
#define CGWSEL 0x002130u    /* colour maths control */
#define CGADSUB 0x002131u   /* colour maths add/subtract and layers */
#define MPYM 0x002135u      /* multiplier result, middle byte (then high at $2136) */
#define JOY1L 0x004218u     /* joypad 1 auto-read result */
#define JOY2L 0x00421Au     /* joypad 2 auto-read result */
#define GSU_R15 0x00301Eu   /* Super FX program counter; writing it starts the Super FX */
#define GSU_SFR 0x003030u   /* Super FX status (bit 5: running) */
#define GSU_PBR 0x003034u   /* Super FX program bank */

#endif
