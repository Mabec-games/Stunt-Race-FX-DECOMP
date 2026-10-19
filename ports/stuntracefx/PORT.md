# Stunt Race FX: the native port

The decompilation (`src/stuntracefx`, complete at M1 and M2) as a native
64-bit program, built by any C compiler, with a thin platform layer under
it. The same principles as the Boss Rally and Top Gear Rally ports:

1. **The original game first.** The core is the decomp's own C, compiled
   as it is: the port never edits it and has no copy of it. What the game
   draws, plays and computes is what the cartridge does; the layers under
   it only answer what the game's C asks of the console.
2. **Platform agnostic.** The game and the console are built once. Each OS
   supplies the window, input, sound out, time and files behind one small
   interface (`ports/common/host/host.h`).
3. **macOS and Windows** first; the hosts' layout leaves room for Linux and
   an iOS app (`HOST=lib`).
4. **The player's own ROM, at build time.** Nothing derived from the
   cartridge is committed. The build checks the ROM by its SHA-1 and builds
   it into the executable; a built game needs no ROM.

## The layers

| | |
|---|---|
| `src/stuntracefx` | the game: every routine of the three processors as C (the decomp, unchanged) |
| `ports/common/snes` | the Super Nintendo without its processors: the S-CPU's bus and clock (`bus.c`), DMA and HDMA (`dma.c`), the PPU drawn a line at a time (`ppu.c`), the Super FX's registers, RAM, bus and plotting (`superfx.c`), the sound unit's RAM, ports, timers and boot ROM protocol (`apu.c`), the S-DSP (`dsp.c`), coroutines for the two other processors (`coro.c`) |
| `ports/stuntracefx/runtime` | the port's runtime: the routines by address for computed calls (`gen/routines.c` from the decomp's tags, `tools/routines.py`), the interrupts, where each processor's C starts (`dispatch.c`); the front end: controls, sound, save, pace, the headless script runner (`main.c`) |
| `ports/common/host` | the OS: `posix`, `win32` (time, files), `macos` (Cocoa, Core Audio, GameController), `windows` (Win32, WASAPI, XInput), `null` (headless) |

### What the decomp's C asks of the machine

The C was written against a model of the console (`machine.h`, `gsu.h`,
`spc.h`): every access each processor's code makes, through `rd8`/`wr8`
(the S-CPU's 24-bit bus), `gsu_rd8`/`gsu_wr8`/`gsu_plot`/`gsu_rpix` (the
Super FX's) and `spc_rd`/`spc_wr` (the SPC700's), in the order and number
the original makes them; register files in `cpu`, `gsu`, `spc`; and a few
control-flow primitives: `call_routine`/`jump_routine` (an address the
code computes: the routine with C there), `cpu_restart` (a jump to code
that resets the stack: the native frames are dropped and that code's C
runs), `cpu_loop_forever` (a branch to itself), `cpu_idle` (the time of a
delay loop), `spc_restart` and `spc_boot_rom`. In the verification
harness these reach the emulated console; here they reach
`ports/common/snes`.

### Time

The S-CPU's C keeps the master clock (21.477 MHz): each access takes the
bus time the hardware gives it (8 clocks for ROM and RAM, 6 for registers,
12 for the joypads' serial ports) plus a share of the instruction fetches
and internal cycles around it, which the C does not make
(`snes_cpu_overhead`, charged once per instruction: the second byte of a
word follows a bus cycle after the first); a delay loop states its own
time (`cpu_idle`). The picture is drawn a line at a time as the clock
passes each line's horizontal blank, after the line's register writes and
before its HDMA; the timers' IRQ, the vertical blank's NMI flag and the
joypads' automatic read happen where the hardware puts them. Interrupts
are taken where the S-CPU's C reads the bus (never between the bytes of
one word, nor between a read and its write back), running the vector's
code in bank 0: the stubs the game copies to $0101 (NOPs, a JSL to the
raster handler's C, RTI) with the frame the interrupt pushes.

**The timing is being made exact.** The Super FX's is the GSU-1's own
(`gsu_clock.c`): the C says which of the original's code bytes it stands
for (`gsu_code`) and the clock prices each fetch as the hardware does (the
code cache, its line fills, ROM), with the RAM and ROM buffers, MULT and
FMULT, the pixel caches and the waits for the bus while the S-CPU has it.
The S-CPU still adds an overhead of 26 clocks to each access it makes
(`snes_cpu_overhead`, `PORT_CPU_OVERHEAD`) until its C's stated times are
switched on. `PORT_DEBUG=1` prints each frame's state.

The Super FX and the SPC700 run their C on coroutines of their own, each
with its own clock, moved on by their accesses. The S-CPU brings them up to
its time before it touches their registers, RAM or ports and once a line;
each hands back when it gets there. So a running Super FX program's status,
RAM and end-of-program interrupt, and the sound CPU's replies, appear when
the program reaches them, as on the cartridge. A Super FX program starts
when the S-CPU writes R15 (its C at PBR:R15) and ends at its STOP. The
SPC700 starts in its boot ROM, whose transfer protocol `apu.c` carries out
access for access, then runs the routine with C at the address the S-CPU
starts it at (of the race and battle drivers' routines at that address,
the one whose code the APU RAM holds).

### The picture and the sound

The PPU draws modes 0-7 (2, 4 and 8 bits a pixel, 8x8 and 16x16 tiles,
mosaic, offset-per-tile, direct colour, Mode 7 with its wrap modes and
EXTBG; the hires modes at half width), sprites with the hardware's
32-sprite and 34-tile limits, both windows on each layer, colour maths,
brightness and forced blank: 256 x 224 pixels, shown at 4:3. The DSP
decodes BRR, steps each voice at its pitch (with pitch modulation and the
noise generator), runs ADSR and GAIN envelopes on the shared rate counter,
and the echo's delay line, 8-tap FIR and feedback, 32,040 samples a
second. Its interpolation kernel is computed (a tapered Gaussian of the
chip's width), not the chip's own table.

## Building

```sh
HOST=null ports/stuntracefx/link.sh                              # build/stuntracefx/port/null/stuntracefx (headless)
ports/stuntracefx/package_app.sh                                 # build/stuntracefx/port/macos/Stunt Race FX.app
CC=x86_64-w64-mingw32-gcc HOST=windows OUT=build/stuntracefx/port/windows ports/stuntracefx/link.sh
                                                                 # build/stuntracefx/port/windows/stuntracefx.exe
```

`SRFX_ROM` names the ROM (default `reference/stuntracefx/Stunt Race FX
(USA) (Rev 1).sfc`). `HOST=lib` builds `libstuntracefx.a` for an app that
brings its own host.

## Playing

| SNES | keyboard | controller |
|---|---|---|
| D-pad | arrows | d-pad or left stick |
| B, A | Z, X | A, B (the SNES's layout: B below, A right) |
| Y, X | A, S | X, Y |
| L, R | Q, W | shoulders |
| START | Return | menu |
| SELECT | Right Shift, Tab | view |

The keyboard is controller 1; a second game controller is controller 2.
The save is the cartridge's battery RAM, kept as `stuntracefx.srm` in the
OS's application-support folder (`~/Library/Application Support/Stunt Race
FX`, `%APPDATA%\Stunt Race FX`).

## Checking it against the original

`stuntracefx --headless --script FILE --out DIR` runs a script of the
harness's form (`wait`, `hold`, `press`, `shot`, `dump`, `repeat ... end`)
from power-on, so the scripts that build the harness's states
(`tools/stuntracefx/states/*.sfx`) run on both: the shots and the work-RAM
and Super FX RAM dumps compare with sfxbox's.
