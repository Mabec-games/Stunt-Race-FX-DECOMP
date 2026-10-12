# Stunt Race FX decomp: standard, tools, layout

## Standard

| | |
|---|---|
| T1 | traced; in the raw intake (address labels); reassembles exact |
| T2 | C written and filed, assembly named and filed; not yet certified |
| T3 | the C is certified behaviour exact: `t3.py --qualify` passes |
| T4 | T3, and the routine's assembly is symbolic and byte-identical in place in the whole-image gate |
| EXCLUDED | beside the tiers: code the retail game provably never runs, proof in the routine header |
| M1 | every game routine T3 |
| M2 | every game routine T4, and the whole ROM rebuilt byte-exact |

Gates are defined in [PLAN.md](PLAN.md) section 2. Query the tree for counts;
do not trust a number written in a file.

## Every routine

The header comment above each routine (C and assembly) carries:

```
@routine <bank>:<addr> <Name>
WHAT IT DOES: its purpose, in game terms
HOW: how it does it
```

plus its entry contract (M, X, DB, D, inputs, outputs, clobbers). The
pre-commit hook refuses a new `@routine` without `WHAT IT DOES:` and `HOW:`,
and refuses address-named files outside the raw intake. Routines are filed by
responsibility, never by address range.

## Layout

| | |
|---|---|
| `src/stuntracefx/<area>/` | the C decomp |
| `src/stuntracefx/include/` | C headers: machine model, RAM maps, structs |
| `asm/stuntracefx/<area>/` | the companion assembly, same areas as the C |
| `asm/stuntracefx/include/` | hardware registers, RAM maps, macros |
| `config/stuntracefx/` | symbols, filing, verdict records |
| `tools/stuntracefx/` | build, gates, tracer, disassembler |
| `tools/common/` | hooks, provcheck |
| `tools/toolchains/` | pinned assembler and emulator core (fetched by `setup.sh`), local patches |
| `docs/stuntracefx/` | this file, the plan, the handbook, dated notes in `decomp/` |
| `reference/stuntracefx/` | ROMs (not tracked) |
| `build/stuntracefx/` | everything generated |

## Tools

| | |
|---|---|
| `setup.sh` | stage the pinned asar and bsnes core, apply local patches, build sfxbox |
| `tools/stuntracefx/sfxbox/` | the original ROM run headless on the pinned core, driven by scripts in `tools/stuntracefx/scripts/`; `--trace` records every ROM byte executed or read, by which processor, at which register widths |
| `tools/stuntracefx/disasm.py` | raw intake from the traces: traced bytes as instructions with explicit operand sizes, the rest included from the reference ROM |
| `tools/stuntracefx/build.py` | trace (in parallel), regenerate the raw intake, assemble; deletes outputs first |
| `tools/stuntracefx/image.py` | ship gate: 0 differing bytes or exit 1 |
| `tools/stuntracefx/asmcontrol.py` | assembler control: every 65816 and Super FX instruction form round-trips |
| `tools/stuntracefx/ppm2png.py` | frame dumps to PNG |
| `tools/stuntracefx/precommit.py` | filing rules on staged files |

```bash
python3 tools/stuntracefx/build.py
python3 tools/stuntracefx/image.py
python3 tools/stuntracefx/asmcontrol.py
```
