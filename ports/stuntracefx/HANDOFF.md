# Stunt Race FX native port: where it stands

> **Superseded (2026-11-14).** The state of the first native port, October 21, 2026. Nothing builds from `ports/` now:
> the game is now `native/stuntracefx` on nativeshvc (the README's "The native game" section, and `docs/stuntracefx/native/PLAN.md`).

Read with `PLAN.md` (the plan and its decisions) and `PORT.md` (the port as
first built; partly out of date: it still describes the console model).
State as of commit 99a18db, 2026-10-21. Everything is local; nothing is
pushed.


**State, 2026-10-28.** The game is built as a cartridge on the nativeshvc
console library: `sh ports/stuntracefx/front/build.sh` (needs
`../nativeshvc` and its library built; contract 2.1 and the cost-profile
standard are in nativeshvc's working tree, not yet committed there). The
standard is behaviour exact, not cycle exact: the C states no instruction
time on any processor (cyc/cycs, gsu_code/gsu_cache/gsu_cyc, spc_cyc
removed; tools `decyc.py`, `dropdead.py`). Time passes in the console's
waits (`src/stuntracefx/include/waits.h`) and in what the chips charge:
nativeshvc charges each S-CPU access bus time plus a per-kind weight, and
the Super FX (`ports/common/snes/superfx.c`) charges each program a fixed
part plus weights per observed operation, both from the game's cost
profile (`front/cost_profile.c`, measured from the ROM by nativeshvc's
`tools/nshvc-profile`). Races run at the original's pace. Open: race music
garbled and a later freeze, from the untimed sound driver's upload loop
(fix verified in nativeshvc, awaiting approval: SPC700 accesses cost their
bus time); the attract demo's slide-gate path not yet rechecked. The rules
below predate this and stand only where they don't conflict with it: the
console is nativeshvc, and its register-level contract is the accepted
API. The cycle-exact work is on branch shelf/cycle-exact.

**Original and Enhanced, 2026-10-28.** Tab switches the two C modes
(`RM_OPT_FRAMERATE`, saved in `remaster.cfg`; the env var forces it for a
run; `rm_enhanced()` in remaster.c; demos stay Original), with a badge
(front/art.c). Original mode: the cost profile, approximate, races about
5% fast (5.64 against 5.95 TV frames a race frame; the C makes fewer
S-CPU accesses than the original the weights were fitted to; a per-kind
fit was dropped). Enhanced: the race in the original's whole steps, one
each 6 TV frames, the 3D drawn at j/6 between the step's start and end on
the frames between (remaster.c whole_tween, tween.c; ac89d148; Jeff:
"whole steps, definitely": sharing a step over ticks broke the handling,
its dead zones and tire forces acting on part-way state); two players a
step late, from the last step's pictures to this one's. The menus' 3D
keeps per-tick shares (its turn rate matches; whole steps would turn it
4x fast). Super FX jobs free (superfx_cart.c); S-CPU costs off only in
race frames, menus' 3D frames and the drawing between (remaster.c
cpu_costs, d68f7779), on in the raster IRQ (c56dac1f) and everywhere
else, so the sound handshakes keep the original's timing. The race's
pause shows the race buffers as VRAM would hold them (every race copy
kept, b05fbbb3). The menus' VblankUpdates skip on "picture completed" is
left clear when the picture went to the canvas (f0e023f7). The
pictures on nativeshvc's canvas (contract 2.4, a04400dc), `remaster/
picture.c` with the stages in video/dma.c (1-player buffers at VRAM words
$0000/$2200, 416 8 bpp characters; 2-player sets $0000/$2000, 360 4 bpp)
and system/vblank_updates.c (menus' 3D sets $0000/$0B00, 143 8 bpp). The
choice is per buffer at its first stage; buffers that share characters
narrow the canvas before the last copy to VRAM, so a Tab back is clean;
the canvas goes off when its screen stops uploading and is handed again
when the window's scale changes. Checked headless against the canvas
decode (residue only under sprites), Tab both ways on all three screens,
k 3 and 5, and Original shots byte-identical to before. The race intro is about 15% shorter in Enhanced (its
steps count race frames; the countdown is timed and matches). Later:
high-resolution polygons from the draw list.

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
| `python3 ports/stuntracefx/tools/same.py` | every play from power-on, the port against the reference port (REF in same.py, built once into build/stuntracefx/port/ref-REF): shots, dumps, the pacing record, byte for byte. About 2 minutes. |
| `PORT_AUDIT=1` (headless) | names any rd8/wr8 that reaches a register |
| `PORT_VIDCHECK=1` | each frame drawn from its record (video.h VidFrame) against the picture drawn a line at a time (drawn only under this check) |
| `PORT_FXCHECK=1` | the game's fill lifted to C (game/render/draw_list.c) against the Super FX's plots, every draw list; also counts pixels plotted outside the lists |
| `PORT_GPUCHECK=1` with `RENDER=metal\|gl\|vulkan HOST=null` | the GPU compositor's frame, read back, against vid_draw_frame |
| `PORT_FXGPUCHECK=1` (a GPU RENDER, HOST=null) | the 3D canvas filled on the GPU (render/fill) against draw_list.c's fill: random spans of every kind and layout, then every draw list |
| `PORT_CANVASCHECK=1` (`PORT_CANVASDUMP=N`) | the Super FX screen as operations (runtime/canvas.c) against every byte the S-CPU uploads of it to VRAM, at the moment it is uploaded |
| `PORT_ROMCENSUS=FILE PORT_ROMCENSUS_ROM=ROM` | each ROM byte a run reads, who read it and the C function; `tools/assetmap.py CENSUS...` (numpy) remakes `assets.csv` from these and the decomp's traces |
| `PORT_RRCHECK=N` (windowed) | the presented frame at frame N (composed on the GPU) against the record's picture drawn on the CPU and scaled by the reference |
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

- The game's C is the decomp's own, `src/stuntracefx` (the port's copy,
  `ports/stuntracefx/game`, was folded back into it and removed); link.sh
  and front/build.sh build it (GAME= selects another tree).
- No ROM image is built in: the build extracts the game's data as named
  assets (`assets.csv`: 2,049 assets, 862,346 bytes; names by hand in
  `assets_named.csv`; `tools/assets.py`
  writes gen/assets.S, assets.c and assets.h) and leaves out the 186,230
  bytes of code the C stands for. runtime/cart.c serves the cartridge's
  addresses from the assets; a read of a byte no asset holds stops the
  port. The game's C names the tables it addresses (`ASSET_name`). The
  Super FX's ROM-buffer fill may land on code: it is only an error if a
  GETB or GETC reads it; whether a program starts with CACHE comes from
  routines.py, not from the code's bytes.
- Asset variants (`ASSETS=` in link.sh; `ios` is the iOS host's default):
  `assets.py` edits assets at build time. On iOS the title reads TAP TO
  START: the sprite sheet gets an O (packed again by `tools/fxpack.py`,
  the game's unpacker in Python, which matches the port on every menu
  picture), the menu animation table moves past the ROM's end (the cart
  space is 2 MB there) and the 20-sprite group takes its old place, which
  the boot copies to $7F:3360.
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
- The Super FX screen as operations: the clear and every row each drawer
  makes, as spans (game/include/draw_list.h FxSpan: flat, textured,
  texel runs, 2-bit bitmaps, scaled 2-bit bitmaps, noise), from the draw
  lists (draw_list.c's walks), the scaled sprites (frame.c), the blits
  (bitmap.c), the HUD programs (hud.c) and the particles (fx_record.c);
  PLOT's layout alias for lines past the height included. One canvas
  per RAM buffer (two: races alternate $2C00 and $9400). Every byte of
  the screens uploaded to VRAM in the 21 plays matches them
  (PORT_CANVASCHECK, 649M bytes); render/fill fills them on Metal, OpenGL
  and Vulkan exactly as fx_fill_span does (PORT_FXGPUCHECK).
- The 3D is shown from the GPU: VRAM bytes a screen upload writes are
  marked external in the frame's record (video.h); before a frame is
  composed the runtime (runtime/canvas.c) replays the operations on the
  GPU up to each upload the frame shows and packs the upload's bytes into
  the compositor's own copy (rr_pack), one copy per record version
  (uploads land in blanked stretches inside frames too); the compositor
  reads external bytes from it. PORT_GPUCHECK 0 on all 21 plays on all
  three backends. The Super FX's own plotting still runs: its stated time
  sets the pacing, and its RAM is the checks' reference.
- GPU compositor (render/compose/core.glsl, one source for three shader
  languages via build.sh): on Metal, OpenGL and Vulkan, every frame of
  the 21 plays identical to vid_draw_frame (PORT_GPUCHECK).
  The window draws through it; the CPU draws a picture only for a check
  or a shot (vid_picture: the finished frame from its record). The
  software backend has no compositor (headless only).

## Next, in order

1. The game's C as plain C (PLAN.md phase 1): no cpu/spc/gsu register
   files, abi_ wrappers or rd8/wr8; the frame loop with no interrupts or
   coroutines; Super FX programs as calls; the sound drivers as a native
   sequencer calling mixer.h; pacing from the stated time.
2. Remove every stepping stone (vid_reg, mixer_regs.c, dma.c, hw_rd/hw_wr,
   ports/common/snes), update PORT.md and the READMEs.
3. Widescreen and field of view (PLAN.md phase 5).
4. Last (Jeff, 2026-10-21: a remaster feature): high resolution, the draw
   list's polygons as triangles at the output's resolution, sub-pixel
   vertices from before the projection rounds (the other drawers' rows
   scaled), drawn through the characters' atlas at the canvases' scale;
   the original mode stays the gated one.
