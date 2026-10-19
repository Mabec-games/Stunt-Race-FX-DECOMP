/*
 * snes.h: the Super Nintendo without its processors, for a native port of a
 * decompiled game.
 *
 * A decomp's C makes every access the original code makes, through the
 * S-CPU's 24-bit bus (rd8/wr8, machine.h), the Super FX's bus (gsu_rd8/
 * gsu_wr8, gsu.h) and the SPC700's (spc_rd/spc_wr, spc.h). This layer is
 * what those accesses reach: the work RAM, the cartridge (ROM, the Super FX
 * and its RAM), the PPU, DMA and HDMA, the CPU's registers (interrupts,
 * timers, multiplier, joypads) and the sound unit (its RAM, timers, ports
 * and the DSP). The processors themselves are the game's C.
 *
 * Time. Each access the S-CPU's C makes takes the time that access takes on
 * the bus, plus a share of the instruction fetches and internal cycles the
 * C does not make (snes_cpu_overhead), in master clocks (21.477 MHz). The
 * picture is drawn a line at a time as that clock passes each line's
 * horizontal blank, HDMA runs there, and the interrupts the game asks for
 * are raised when the clock reaches them. The Super FX and the SPC700 run
 * their C on coroutines of their own, each with its own clock, and are
 * brought up to the S-CPU's time whenever the S-CPU could see what they do
 * (their registers, their RAM, the sound ports) and once a line.
 *
 * Interrupts are taken where the S-CPU's C reads the bus (never between
 * the two bytes of one word, nor between a read and its write back), as an
 * instruction boundary is the only place hardware takes them; the port's
 * runtime supplies how (snes_interrupt_hook).
 */
#ifndef PORT_SNES_H
#define PORT_SNES_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SNES_MASTER_HZ     21477272.727       /* NTSC master clock */
#define SNES_LINE_CLOCKS   1364
#define SNES_LINES         262
#define SNES_FRAME_CLOCKS  (SNES_LINE_CLOCKS * SNES_LINES)
#define SNES_WIDTH         256
#define SNES_HEIGHT        224

/* ---- the cartridge and power ----------------------------------------------------- */
typedef struct SnesCart {
    const uint8_t *rom;
    uint32_t rom_size;
    int superfx;             /* a Super FX cartridge (LoROM, RAM at $70-$71) */
    uint32_t sfx_ram_size;   /* its RAM: 32 or 64 KB */
} SnesCart;

/* Power on: everything cleared as the console starts. */
void snes_power(const SnesCart *cart);

/* The Super FX's RAM (the game pak RAM; on a cartridge with a battery,
   the save): sfx_ram_size bytes. */
uint8_t *snes_sfx_ram(void);

/* ---- time ------------------------------------------------------------------------ */
extern uint64_t snes_clock;            /* master clocks since power on */
extern unsigned snes_cpu_overhead;     /* clocks added to each S-CPU access */
uint32_t snes_frame(void);             /* frames begun since power on */
unsigned snes_vcounter(void);
unsigned snes_hcounter(void);          /* dots, 0-339 */

/* ---- what the port's runtime supplies -------------------------------------------- */
/* An interrupt is due (nmi 1: the NMI; else the IRQ line is asserted and the
   CPU's I flag clear): take it. Called from inside the S-CPU's rd8, before
   the read. */
extern void (*snes_interrupt_hook)(int nmi);
/* Whether the CPU masks the IRQ (its I flag): read where an IRQ is due. */
extern int (*snes_irq_masked)(void);
/* A frame's picture is finished (the start of the vertical blank): show it,
   take input, keep time. Called from inside an access. */
extern void (*snes_frame_hook)(void);

/* ---- the picture ----------------------------------------------------------------- */
/* The last finished frame: SNES_WIDTH x SNES_HEIGHT pixels, 0xFFRRGGBB. */
const uint32_t *snes_picture(void);

/* ---- input ----------------------------------------------------------------------- */
/* The buttons held on controller port 1 or 2 (0 or 1), in the joypad's
   own bit order: B Y SELECT START UP DOWN LEFT RIGHT A X L R from bit 15. */
void snes_set_joypad(int port, uint16_t buttons);
enum {
    SNES_B = 0x8000, SNES_Y = 0x4000, SNES_SELECT = 0x2000, SNES_START = 0x1000,
    SNES_UP = 0x0800, SNES_DOWN = 0x0400, SNES_LEFT = 0x0200, SNES_RIGHT = 0x0100,
    SNES_A = 0x0080, SNES_X = 0x0040, SNES_L = 0x0020, SNES_R = 0x0010,
};

/* ---- sound ------------------------------------------------------------------------ */
/* The DSP's output, 32,000 stereo frames a second (left, right), as the
   SPC700's clock produces them; up to max frames into lr, the number taken.
   The samples are made as the SPC700 runs, so this drains what the frames
   so far made. */
int snes_audio_take(int16_t *lr, int max);
/* how many frames are waiting */
int snes_audio_queued(void);

/* The SPC700 runs the C the port's runtime starts: the IPL boot ROM's work,
   then a routine at an APU address (snes_spc_hooks). */
typedef struct SnesSpcHooks {
    /* run the SPC700 code at APU address `addr`; never returns (the code
       loops for ever, or ends by spc_restart or spc_boot_rom, which the layer turns
       into a new call of this) */
    void (*run)(uint16_t addr);
} SnesSpcHooks;
void snes_spc_set_hooks(const SnesSpcHooks *h);
/* the APU's 64 KB of RAM (for the runtime's choice of routine) */
const uint8_t *snes_aram(void);

/* The Super FX starts a program at bank:addr (the S-CPU wrote R15): the
   runtime runs its C, which returns at the program's STOP. */
extern void (*snes_gsu_run_hook)(uint8_t bank, uint16_t addr);

/* ---- for the runtime -------------------------------------------------------------- */
/* A fatal error in the port (not in the game): reported, then exit. */
void snes_fatal(const char *fmt, ...);
/* A read of the bus with no time and no interrupt (for decoding the
   interrupt stubs and such) */
uint8_t snes_peek(uint32_t addr);

/* The 65816's own bus, for running original code (cpu65816.c): accesses
   that take only their own time, internal cycles, and the place between
   two instructions where an interrupt may be taken. */
uint8_t snes_cpu_read(uint32_t addr);
void snes_cpu_write(uint32_t addr, uint8_t v);
void snes_cpu_idle(unsigned cycles);
void snes_cpu_interrupts(void);
/* clocks with no access (a delay loop), interrupts taken along the way */
void snes_cpu_wait(uint32_t clocks);

/* The machine's state at a frame boundary, for comparisons with the
   original: work RAM (128 KB) and the Super FX RAM. */
const uint8_t *snes_wram(void);

#ifdef __cplusplus
}
#endif
#endif
