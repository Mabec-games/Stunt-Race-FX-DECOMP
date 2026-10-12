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

/* The rest of the register set, by its standard names. */
/* PPU registers */
#define OBSEL 0x002101u
#define BGMODE 0x002105u
#define MOSAIC 0x002106u
#define BG1SC 0x002107u
#define BG2SC 0x002108u
#define BG3SC 0x002109u
#define BG4SC 0x00210Au
#define BG12NBA 0x00210Bu
#define BG34NBA 0x00210Cu
#define BG1HOFS 0x00210Du
#define BG1VOFS 0x00210Eu
#define BG3HOFS 0x002111u
#define BG3VOFS 0x002112u
#define BG4HOFS 0x002113u
#define BG4VOFS 0x002114u
#define VMADDH 0x002117u
#define VMDATAH 0x002119u
#define M7SEL 0x00211Au
#define M7C 0x00211Du
#define M7D 0x00211Eu
#define M7X 0x00211Fu
#define M7Y 0x002120u
#define WH0 0x002126u
#define WH1 0x002127u
#define WH2 0x002128u
#define WH3 0x002129u
#define TM 0x00212Cu
#define TS 0x00212Du
#define TSW 0x00212Fu
#define COLDATA 0x002132u
#define SETINI 0x002133u
#define MPYL 0x002134u
#define MPYH 0x002136u
#define SLHV 0x002137u
#define RDOAM 0x002138u
#define RDVRAML 0x002139u
#define RDVRAMH 0x00213Au
#define RDCGRAM 0x00213Bu
#define OPHCT 0x00213Cu
#define OPVCT 0x00213Du
#define STAT77 0x00213Eu
#define STAT78 0x00213Fu
#define WMDATA 0x002180u
#define WMADDL 0x002181u
#define WMADDM 0x002182u
#define WMADDH 0x002183u
/* S-CPU registers */
#define JOYSER0 0x004016u
#define JOYSER1 0x004017u
#define NMITIMEN 0x004200u
#define WRIO 0x004201u
#define WRMPYA 0x004202u
#define WRMPYB 0x004203u
#define WRDIVL 0x004204u
#define WRDIVH 0x004205u
#define WRDIVB 0x004206u
#define HTIMEL 0x004207u
#define HTIMEH 0x004208u
#define VTIMEL 0x004209u
#define VTIMEH 0x00420Au
#define HDMAEN 0x00420Cu
#define MEMSEL 0x00420Du
#define RDNMI 0x004210u
#define TIMEUP 0x004211u
#define RDIO 0x004213u
#define RDDIVL 0x004214u
#define RDDIVH 0x004215u
#define RDMPYL 0x004216u
#define RDMPYH 0x004217u
#define JOY1H 0x004219u
#define JOY2H 0x00421Bu
#define JOY3L 0x00421Cu
#define JOY3H 0x00421Du
#define JOY4L 0x00421Eu
#define JOY4H 0x00421Fu
/* DMA and HDMA channels 0-7 */
#define BBAD0 0x004301u
#define A1T0H 0x004303u
#define DAS0H 0x004306u
#define DASB0 0x004307u
#define A2A0L 0x004308u
#define A2A0H 0x004309u
#define NTRL0 0x00430Au
#define DMAP1 0x004310u
#define BBAD1 0x004311u
#define A1T1L 0x004312u
#define A1T1H 0x004313u
#define A1B1 0x004314u
#define DAS1L 0x004315u
#define DAS1H 0x004316u
#define DASB1 0x004317u
#define A2A1L 0x004318u
#define A2A1H 0x004319u
#define NTRL1 0x00431Au
#define DMAP2 0x004320u
#define BBAD2 0x004321u
#define A1T2L 0x004322u
#define A1T2H 0x004323u
#define A1B2 0x004324u
#define DAS2L 0x004325u
#define DAS2H 0x004326u
#define DASB2 0x004327u
#define A2A2L 0x004328u
#define A2A2H 0x004329u
#define NTRL2 0x00432Au
#define DMAP3 0x004330u
#define BBAD3 0x004331u
#define A1T3L 0x004332u
#define A1T3H 0x004333u
#define A1B3 0x004334u
#define DAS3L 0x004335u
#define DAS3H 0x004336u
#define DASB3 0x004337u
#define A2A3L 0x004338u
#define A2A3H 0x004339u
#define NTRL3 0x00433Au
#define DMAP4 0x004340u
#define BBAD4 0x004341u
#define A1T4L 0x004342u
#define A1T4H 0x004343u
#define A1B4 0x004344u
#define DAS4L 0x004345u
#define DAS4H 0x004346u
#define DASB4 0x004347u
#define A2A4L 0x004348u
#define A2A4H 0x004349u
#define NTRL4 0x00434Au
#define DMAP5 0x004350u
#define BBAD5 0x004351u
#define A1T5L 0x004352u
#define A1T5H 0x004353u
#define A1B5 0x004354u
#define DAS5L 0x004355u
#define DAS5H 0x004356u
#define DASB5 0x004357u
#define A2A5L 0x004358u
#define A2A5H 0x004359u
#define NTRL5 0x00435Au
#define DMAP6 0x004360u
#define BBAD6 0x004361u
#define A1T6L 0x004362u
#define A1T6H 0x004363u
#define A1B6 0x004364u
#define DAS6L 0x004365u
#define DAS6H 0x004366u
#define DASB6 0x004367u
#define A2A6L 0x004368u
#define A2A6H 0x004369u
#define NTRL6 0x00436Au
#define DMAP7 0x004370u
#define BBAD7 0x004371u
#define A1T7L 0x004372u
#define A1T7H 0x004373u
#define A1B7 0x004374u
#define DAS7L 0x004375u
#define DAS7H 0x004376u
#define DASB7 0x004377u
#define A2A7L 0x004378u
#define A2A7H 0x004379u
#define NTRL7 0x00437Au
/* Super FX registers */
#define GSU_R0 0x003000u
#define GSU_R1 0x003002u
#define GSU_R2 0x003004u
#define GSU_R3 0x003006u
#define GSU_R4 0x003008u
#define GSU_R5 0x00300Au
#define GSU_R6 0x00300Cu
#define GSU_R7 0x00300Eu
#define GSU_R8 0x003010u
#define GSU_R9 0x003012u
#define GSU_R10 0x003014u
#define GSU_R11 0x003016u
#define GSU_R12 0x003018u
#define GSU_R13 0x00301Au
#define GSU_R14 0x00301Cu
#define GSU_BRAMR 0x003033u
#define GSU_ROMBR 0x003036u
#define GSU_CFGR 0x003037u
#define GSU_SCBR 0x003038u
#define GSU_CLSR 0x003039u
#define GSU_VCR 0x00303Bu
#define GSU_RAMBR 0x00303Cu
#define GSU_CBR 0x00303Eu

#endif
