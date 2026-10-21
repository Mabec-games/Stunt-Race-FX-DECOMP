# Stunt Race FX native port: where it stands

Read with `PLAN.md` (the plan and its decisions) and `PORT.md` (the port as
first built; partly out of date: it still describes the console model).
State as of commit 99a18db, 2026-10-21. Everything is local; nothing is
pushed.

## The rules (Jeff's, all standing)

- **Zero emulation when done.** The finished port is the game's own source
  as native C over native parts: no bus (rd8/wr8), no console register
  files or register-number interfaces, no CPU/SPC700/Super FX register
  state, no DMA/HDMA engines, no chip models, no `ports/common/snes`.
  Register-shaped steps are allowed one verified move at a time and must be
  removed before the work is done.
- **Nothing shown is drawn in software.** Metal, Vulkan and OpenGL (no
  Direct3D) draw everything, the 2D picture included. A remastered settings
  screen will toggle original and high-resolution polygons; the GPU draws
  both. CPU drawing code exists only as a check's reference.
- **No bsnes code or logic in the port.** bsnes, through sfxbox, is the
  oracle to check against. The old DSP model followed bsnes's code and was
  deleted for that reason.
- **Original pacing.** The game's race frames take as many video frames as
  the original's did (`$0306` to `$06D4`), counted from the time the C
  states; never emulate to get it.
- **Resolution and field of view agnostic**, the original 4:3 a run-time
  toggle; sharp bilinear for 2D, as in the Boss Rally port.
- Verify in code before handing anything over: a check that can fail
  (prove it with a planted break), run, numbers reported. No grinding:
  measure, find the biggest cause, fix once. Say how long each run takes.
  Commit each verified step; never push. Commits say what changed and
  why, and nothing about who or what wrote them.

## The checks

| command | what it holds |
|---|---|
| `python3 ports/stuntracefx/tools/same.py` | every play from power-on, the port against the reference port (a3f26e3, built once into build/stuntracefx/port/ref): shots, dumps, the pacing record, byte for byte. About 2 minutes. |
| `PORT_AUDIT=1` (headless) | names any rd8/wr8 that reaches a register |
| `PORT_VIDCHECK=1` | each frame drawn from its record (video.h VidFrame) against the picture drawn a line at a time |
| `PORT_FXCHECK=1` | the game's fill lifted to C (game/render/draw_list.c) against the Super FX's plots, every draw list; also counts pixels plotted outside the lists |
| `PORT_GPUCHECK=1` with `RENDER=metal HOST=null` | the GPU compositor's frame, read back, against vid_draw_frame |
| `PORT_RRCHECK=N` (windowed) | the presented frame at frame N against the reference |
| `ports/common/render/check/check.sh` | each backend's sharp bilinear against the reference |
| `ports/common/audio/check/mix_replay LOG SOUND` (VOX=) | the native mixer fed bsnes's DSP inputs, against bsnes's samples |
| `ports/common/audio/check/resample_check` | the resampler against exact tones |

sfxbox (the bsnes harness, tools/stuntracefx/sfxbox) records `--pacing`,
`--sound` (raw DSP samples), `--apulog` (each SPC700 write with the DSP's
cycle) and `--voices` (each voice's state); harness patches 0011-0013 add
the hooks (tools/toolchains/patches/bsnes; rebuild with make in
tools/toolchains/src/bsnes/bsnes target=libretro binary=library and copy
the dylib to tools/toolchains/bsnes).

Two of the 23 plays (speed_all_courses, speed_championship) use `poke`,
which the port's script runner does not do: they run on nothing, so every
check above covers 21 plays.

## Done

- `ports/stuntracefx/game`: the port's own copy of the decomp, built by
  link.sh (GAME= selects another tree).
- Every console register access in the game is an explicit call
  (game/include/native.h): the multiply units are C arithmetic; joypads
  through input_pad; the picture through ppu_wr/vid_reg; DMA through
  dma_wr/mdmaen_wr/hdmaen_wr; everything else hw_rd/hw_wr (still answered
  by ports/common/snes); computed addresses through reg_rd/reg_wr.
- Picture: `ports/common/video` (out of the console model), and each
  frame's record: every line's state and palette and at most a few copies
  of the tiles and sprite table (the game changes them only in blanked
  stretches; the palette changes per line on race screens).
- Renderer: `ports/common/render` (rr.h), Metal, OpenGL, Vulkan (MoltenVK
  on the Mac), software and null; the window's frame at 4:3 with sharp
  bilinear (RENDER= in link.sh: metal default on the Mac, gl on Windows).
- Sound: `ports/common/audio/mixer.c`, a native mixer from fullsnes with
  calls that say what changes (mixer.h); mixer_regs.c translates the
  drivers' register writes (a stepping stone). Matches bsnes's DSP output
  exactly for the attract's first 126,206 samples; then 1-2 levels off on
  one channel (open). Windowed-sinc resampler to the device.
- 3D: the Super FX queues each frame's shapes in a draw list ($08AA);
  game/render/draw_list.c is the game's fill (flat, textured, lines) in
  plain C, identical to the Super FX on every list of the 21 plays.
- GPU compositor (render/compose/core.glsl, one source for three shader
  languages via build.sh): on Metal, every frame of the 21 plays identical
  to vid_draw_frame (PORT_GPUCHECK).

## Next, in order

1. GPU compositor exact on Metal (PORT_GPUCHECK to 0 on all 21 plays),
   then the same on OpenGL and Vulkan (their rr_compose is a stub); then
   the window draws through it and the line-by-line drawing becomes the
   check's reference only.
2. The 3D layer on the GPU: original mode from the draw list (edge setup
   as draw_list.c does it, spans filled in the shader), high resolution as
   triangles with sub-pixel vertices from before the projection rounds;
   the 3D picture composited where the game shows its tiles. Lift the other
   Super FX drawers (objects, effects, weather, bitmaps: 11% of a race's
   pixels, a third of a stunt course's) into draw operations too.
3. Widescreen and field of view (PLAN.md phase 5).
4. The game's C as plain C (PLAN.md phase 1): no cpu/spc/gsu register
   files, abi_ wrappers or rd8/wr8; the frame loop with no interrupts or
   coroutines; Super FX programs as calls; the sound drivers as a native
   sequencer calling mixer.h; pacing from the stated time.
5. Remove every stepping stone (vid_reg, mixer_regs.c, dma.c, hw_rd/hw_wr,
   ports/common/snes), update PORT.md and the READMEs.
