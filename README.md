# Stunt Race FX: decompilation

A decompilation of **Stunt Race FX** (SNES, 1994), target
`Stunt Race FX (USA) (Rev 1).sfc` (SHA-1 `781e376ff453a83c7bcf72339abe6b842efd9c86`).

**Maintainer:** Jeffrey Wilbur, Strut B, LLC\
**Contact:** [retro@strutb.com](mailto:retro@strutb.com)

## Project purpose

The game was written by hand in assembly for three processors: the SNES CPU
(65816), the Super FX chip (GSU-1) and the sound CPU (SPC700). No compiler
produced its bytes, so no C can rebuild it byte for byte. The project keeps
two lanes:

- **C, the decompilation.** Every routine rewritten as readable C and proven to
  behave exactly like the original: the original ROM and a build with the C
  swapped in run side by side in an emulator, and the machine state must match
  on every frame.
- **Assembly, the byte-exact companion.** Symbolic, commented assembly that
  rebuilds the ROM byte for byte.

| | |
|---|---|
| M1 | every game routine's C is behaviour exact (T3) |
| M2 | M1, and the whole ROM rebuilds byte-exact from the companion assembly (T4) |

Standard, tools and layout: [docs/stuntracefx/DECOMP.md](docs/stuntracefx/DECOMP.md).
Plan: [docs/stuntracefx/PLAN.md](docs/stuntracefx/PLAN.md).

No game data ships here. The build reads your own copy of the ROM, checked by
SHA-1.

## Build

```bash
./setup.sh
python3 tools/stuntracefx/build.py
python3 tools/stuntracefx/image.py
```

`setup.sh` stages the pinned assembler and emulator core inside the repo;
nothing is installed on the host. Put the ROM at
`reference/stuntracefx/Stunt Race FX (USA) (Rev 1).sfc`.
