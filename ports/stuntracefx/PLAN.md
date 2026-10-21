# Stunt Race FX native port: plan

The decomp (`src/stuntracefx`) is complete: byte exact at M2 and cycle exact
on all three processors. The port now stops being a console. Today the
decomp's C runs on `ports/common/snes`, a model of the console under it
(bus, PPU, DMA and HDMA, Super FX registers, sound unit, DSP, coroutines).
This plan replaces every console part with a native component: a 2D
compositor, a polygon renderer, a sequencer and mixer, a frame loop, input
and save APIs, on Metal, Vulkan and OpenGL, as in the Boss Rally
port.

## Decisions

- **Zero emulation when done.** The finished port is the game's own source
  as native C over native components, nothing else: no bus (rd8/wr8), no
  console register files or register-number interfaces, no CPU, SPC700 or
  Super FX register state, no DMA or HDMA engines, no interrupt or chip
  models, no `ports/common/snes`. Each native part takes calls that say what
  the game means (a layer's tiles and scroll, a palette, a sprite list; a
  voice's sample, pitch, envelope; the echo) and does it natively: BRR is
  the samples' codec, the Gaussian kernel a resampler, ADSR an envelope.
  Steps on the way may keep a register-shaped interface for one verified
  move at a time; each such interface is removed before the work is done.
- **The port has its own copy of the game** (`ports/stuntracefx/game`),
  lifted from the decomp. The decomp stays unchanged as the reference; the
  copy is checked against the original at every step. This replaces
  "ports never edit decomp source" with "ports never edit `src/`; the port's
  copy answers to the oracle".
- **Graphics backends:** Metal, Vulkan, OpenGL, behind one small
  interface, as Boss Rally.
- **Frame pacing as the original.** `RaceFrameStep` feeds the frames since
  the last game frame (`$0306` to `$06D4`) into the simulation, and that
  count is set by how long the Super FX takes to draw. The copy keeps a
  counter of the time its C already states (`gsu_cyc`, the S-CPU's `cyc`)
  and derives the count from it. A counter, not emulation. An unlocked rate
  with interpolation can come after parity, on top of it.
- **Any resolution, any aspect, any field of view; the original 4:3 a
  toggle**, as Boss Rally. The native components take the view's size and
  aspect as parameters from the first line written; nothing assumes
  256 x 224. The original mode (256 x 224 at 4:3, the original's field of
  view) is the one the gates judge, and stays selectable at run time.

## The oracle

bsnes, through sfxbox, is the reference for every phase: state, pixels,
samples and pacing are checked against it, never guessed. None of its code
goes into the port. Each gate runs only the scripts that reach the code just
changed; one full run at the end of a phase.

| gate | compares, per game frame, with sfxbox |
|---|---|
| state | the game's variables, object table, Super FX RAM outside the bitmap |
| pacing | `$0306` / `$06D4` on every frame of every race script |
| 2D | the composed frame, 256 x 224, pixel for pixel, mid-frame changes included |
| 3D | the Super FX bitmap, pixel for pixel |
| voice | the sequencer's per-tick voice events against the DSP register writes |
| sound | the mixer's exact mode against the DSP's output, sample for sample |

Frame boundaries come from sfxbox's NMI and game-frame points, so "the same
frame" means the same on both sides.

## Phases

**0. The copy and the gates.** `ports/stuntracefx/game` from `src/`;
the state and pacing gates against sfxbox, each proven able to fail (a one
byte logic change must be caught) before it is relied on.

**1. The C as C.** Area by area, state identical throughout:
- `abi_` wrappers and the `cpu` register file become functions with
  arguments and results;
- work-RAM addresses become a typed `struct game` with the `wram.h` names;
- `call_routine` / `jump_routine` become function tables; `cpu_restart` a
  state transition;
- the Super FX and SPC700 register files and RAM the same way;
- `cyc` goes, but for the pacing counter.

The bulk of the work: every file of the game.

**2. The frame loop.** No interrupts, no coroutines. `main`: game step,
the vertical blank's work as a call, present. Super FX programs are
synchronous calls; the waits (Super FX polls, vertical blank, APU
handshakes) go.

**3. 2D video** (`ports/common/video`, portable C). Tile layers, a
character and tile store, palettes, a sprite list; per-line parameters in
place of HDMA (windows, gradients, scroll splits); colour maths as blends.
DMA to VRAM becomes uploads. Gate: 2D.

**4. 3D.** The render path (`render/polygon.c`, `raster.c`, `bitmap.c`,
`project.c`) emits a polygon list (screen vertices, colour or dither, the
order `sort.c` gives) instead of plotting a planar bitmap for DMA.
(a) A software rasteriser at the original's resolution; gate: 3D.
(b) The four GPU backends, resolution scaling, sub-pixel vertices.

**5. Resolution and field of view.** A toggle between the original 4:3 and
the display's own size and aspect:
- the 3D view widens horizontally (vertical field of view kept), and the
  game's view culling and clipping widen with it, so the sides show the
  world, not blank edges;
- the pacing counter charges the original view's cost, so a wider view
  never changes how the game plays;
- the 2D: the HUD anchored to the screen's edges and corners; the
  backgrounds (sky, horizon) extended across the width; full-screen menus
  and pictures pillarboxed; the 2D layers scaled to the output with sharp
  bilinear filtering (nearest to the largest whole multiple, then bilinear
  for the rest), as Boss Rally, so pixels stay crisp at any size without
  uneven widths;
- the 3D drawn at the output resolution, not scaled up.

Gate: with the toggle on original, every gate above unchanged; the wide
view checked against the original's at the centre 256 columns where both
draw the same.

**6. Sound.** The race and battle drivers become a sequencer on the audio
thread; the DSP a mixer (BRR decoded to PCM at load, envelopes, pitch,
noise, echo and FIR); the `$2140` handshake a command API; the APU uploads
asset loads. Gates: voice, sound. Better interpolation is a mode on top of
the exact one.

**7. Input, save, assets.** The joypad registers become an input API, the
battery RAM a save struct; the ROM's data still comes from the player's
ROM at build time.

**8. Hosts.** `host.h`: surface, audio callback, input, files. macOS and
Windows, then Linux and iOS. `ports/common/snes` is deleted.

**9. After parity.** An unlocked frame rate with interpolation.

## Method

One area at a time, each verified step committed at once, worked in one
session unless parallel work is asked for. Every run's time stated before
it starts.
