# Stunt Race FX decomp: plan

Target: `Stunt Race FX (USA) (Rev 1).sfc`, 1,048,576 B, SHA-1
`781e376ff453a83c7bcf72339abe6b842efd9c86`, CRC32 `380C2635`. Header: LoROM,
cart type 0x1A (Super FX + RAM + battery), 8 Mbit ROM, 64 KB GSU RAM, game
code `CQ`, version 1. Native NMI/IRQ vectors point into WRAM (0x0108,
0x010C); reset is 0x00FE88. Witness builds: Europe, Wild Trax (Japan, Rev 1).

The decomp is **C**. A byte-exact companion in assembly rebuilds the ROM.

| | |
|---|---|
| M1 | every game routine T3: its C is behaviour exact, proven by the live oracle |
| M2 | M1, and every routine T4: the whole ROM rebuilt byte-exact from the companion assembly |

## 1. What is different from Top Gear Rally and Boss Rally

Those were compiled C: the right C fed to the original compiler gave the
original bytes, so one source served both tiers. Stunt Race FX is hand-written
assembly for three processors:

| CPU | Code | Notes |
|---|---|---|
| S-CPU (65816) | game logic, NMI/IRQ, DMA, PPU | M/X width flags, DB and D registers, long and short addressing |
| GSU-1 (Super FX) | 3D transform, rasteriser, likely physics helpers | ALT prefixes, branch delay slots, cache execution from 0x3100 |
| SPC700 | sound driver (uploaded at boot) | probably Nintendo's standard driver; confirm |

Evidence so far: zero C-compiler stack frames (`TSC; SEC; SBC #n; TCS`) in the
whole ROM. Confirm in Phase 0.

No compiler made these bytes, so no C can be byte-exact. Two lanes, two
sources per routine:

- **C lane (the decomp, T3).** Readable C per routine, proven to behave
  exactly like the original. Ends as a whole game in C; a native port is built
  from it later and never edits it.
- **Assembly lane (bytes, T4).** Symbolic, commented assembly that rebuilds the
  ROM byte-exact. It is the reading the C is written from, and the byte-exact
  rebuild proves the reading is right.

## 2. Standard

| | |
|---|---|
| T1 | traced; in the raw disassembly (address labels); reassembles exact |
| T2 | C written and filed, assembly named and filed; not yet certified |
| T3 | the C is certified behaviour exact: `t3.py --qualify` passes |
| T4 | T3, and the routine's assembly is symbolic and byte-identical in place in the whole-image gate |
| EXCLUDED | beside the tiers: code the retail game provably never runs, proof in the routine header |

How the C runs against the original (the hybrid harness): the original ROM
runs in an emulator; when the CPU (or the GSU) reaches the entry of a routine
that has C, the harness calls the C instead, which reads and writes the
emulated machine through the same memory and register model the port will
later provide, then returns as the original would. One routine swapped in is
A5; every T3 routine swapped in is A7. This is the brally in-image oracle,
applied to a console.

T3 gates:

- **Gate 0, complete.** `WHAT IT DOES:` and `HOW:` above the C; entry contract
  (M, X, DB, D, inputs, outputs, clobbers) stated; no TODO/FIXME/???/guess.
- **Gate C, covered.** Every instruction of the original routine executed by
  the script suite, or the unreached block proven unreachable in the header.
- **Gate A5, live.** Original and hybrid (this routine in C) run the scripts in
  lockstep; compared at every frame: WRAM, GSU RAM, VRAM, OAM, CGRAM, APU RAM,
  plus the routine's register outputs at return. `IDENTICAL` recorded in
  `config/stuntracefx/t3_live.csv`.
- **Gate A7, whole image.** Every T3 routine in C at once, every script
  identical; recorded in `config/stuntracefx/whole_image.csv`, no older than
  the routine's source.

Timing: C runs in zero emulated cycles, which would move IRQs and lag frames.
In lockstep the original instance executes the same routine at the same
moment, so the hybrid is charged exactly the cycles the original spent. State
then stays comparable frame by frame.

T4 gates (on top of T3):

- **Gate S, symbolic.** No literal ROM or RAM address in the routine's
  assembly. A lint, plus the **shift test**: build with the routine and its
  data moved; every differing byte must be explained by a label relocation.
- **Gate I, image.** The whole ROM built from source differs by 0 bytes.

Shift rules: odd amounts for the static relocation check; whole banks for any
live run of a shifted build (keeps page-crossing cycle counts); GSU code in
16-byte steps (the GSU cache line).

## 3. Repo layout (same standard as brally)

```
src/stuntracefx/<area>/        the C decomp, one file per responsibility
src/stuntracefx/include/       C headers: machine model, WRAM/GSU RAM maps, structs
asm/stuntracefx/<area>/        companion assembly: 65816 (.asm), GSU (.gsu), SPC700 (.spc), same areas as the C
asm/stuntracefx/include/       hardware registers, RAM maps, macros
src/stuntracefx/assets/        extracted data in editable form, each with a 1:1 round-trip converter
config/stuntracefx/            symbols.csv, codemap.csv, filing.csv, fenced.csv, excluded.csv, t3_live.csv, whole_image.csv
tools/stuntracefx/             build, gates, harness, tracer, disassembler, tiers
tools/common/                  hooks, provcheck.py, install_hooks.py
tools/toolchains/              pinned assembler and emulator core, fetched and built by setup.sh
docs/stuntracefx/              README, DECOMP.md, HANDBOOK.md, decomp/{functions,levers,log,rules,toolchain,oracle}
reference/stuntracefx/         ROMs (gitignored)
build/stuntracefx/{snes,shift,box,hybrid,analysis}
```

Candidate areas, settled from the call graph in Phase 3, not guessed now:
`startup`, `system` (NMI, IRQ, DMA, joypad), `gsu` (S-CPU side of the GSU
interface), `render` (GSU transform and raster), `track`, `vehicle`,
`physics`, `opponents`, `race` (rules, laps, timing, battle), `hud`, `menus`,
`save`, `audio`, `text`.

## 4. Tools (names mirror the tgrally lane)

| Tool | Use |
|---|---|
| `setup.sh` | stage the pinned toolchain in the repo; never install to the host |
| `sfxbox` | the emulator core run headless, with hooks: per-instruction trace (S-CPU PC, M, X, DB, D; GSU PC, PBR), state dumps, C routine dispatch, two instances in lockstep |
| `codemap.py` | merge traces into `codemap.csv`: every byte code or data, with proven widths |
| `disasm.py` | emit assembly from the map; explicit operand sizes on every instruction |
| `build.py` | assemble the companion ROM; build the hybrid; deletes outputs first |
| `image.py` | T4 ship gate: 0 differing bytes; exit 1 on any difference, overlap or unbuildable claim |
| `shift.py` | shifted builds and the relocation-explained diff |
| `t3.py` | `--live` (A5), `--image` (A7), `--qualify` |
| `cover.py` | instruction coverage per routine, both CPUs |
| `tiers.py` | T1-T4, M1, M2, rebuilt fresh; routines and bytes as denominators |
| `fileaudit.py` | ratchets: undescribed 0, address-batch files 0, literal addresses in T4 assembly 0 |
| `provcheck.py` | pre-commit and commit-msg hook: provenance vocabulary, typographic dashes, attribution trailers |

Assembler: asar first (65816, Super FX and SPC700 in one tool), WLA-DX the
fallback; accepted only after a control set reassembles byte-exact, including
every GSU opcode and ALT form. Emulator: an accurate core with Super FX
support, built from pinned source; a second core cross-checks it on the
control scripts.

## 5. Phases

**Phase 0: bootstrap.** Layout, hooks and provcheck before the first commit.
Confirm hand assembly. Stage and validate the assembler. Build the emulator
core headless with trace hooks. Survey public prior work (RAM maps) as hints
only. Diff the witness builds for routine boundaries and pointer vs constant
disambiguation.

**Phase 1: raw byte-exact skeleton.** Trace attract mode, menus and races;
build `codemap.csv`; emit the whole ROM as T1 assembly plus data; `image.py`
reports 0 differing bytes. From here every assembly edit is checked against
the whole ROM.

**Phase 2: oracle.** Hybrid dispatch and lockstep in `sfxbox`; cycle charging;
coverage-driven script suite (every mode, course, vehicle, 2P, records, save
and load, attract, pause, crashes, damage, game over, credits); `cover.py`;
negative controls (a one-byte logic change in a C routine must be caught).

**Phase 3: architecture.** Call graph across both CPUs, subsystem map, module
list, RAM maps and structs as C headers. Data formats; asset converters as
faithful 1:1 rips with round-trip checks.

**Phase 4: routines to T3, by hand, one at a time.** M1. Read the listing,
name and file the assembly, state the entry contract, write the C with what
and how, run `t3.py --qualify`, commit.

**Phase 5: T4.** M2. Every routine's assembly symbolic and shift-proof; code
banks hold no unexplained `db` blobs; `image.py` 0 bytes.

## 6. Carried over from brally and tgrally

Standards:

1. T3 is same input, same output, judged by the oracle. Never by a byte diff,
   never by judgement. Never relax a gate to reach a number.
2. Two grades and nothing between. EXCLUDED sits beside them.
3. The whole-image gate is the ship gate. A per-routine pass is not proof
   (placement and preamble traps). The gate exits 1 on anything it cannot
   build and place.
4. Counts are queried from the tree, with denominators. Never a number in
   prose.

Oracle:

5. The oracle proves only what the scripts execute. On brally the first 26
   scripts missed most content and A7 broke two certified routines when more
   were added. Coverage is measured from day one and is a gate here.
6. A7 over A5: whole-image runs found about 25 bugs the per-function oracle
   had passed.
7. Compare only real state. False divergences came from stack scratch,
   argument slots and invalid seeded state; valid-state input profiles fixed
   them. Before trusting a DIFF, check for a half-reverted profile.
8. The oracle finds what review misses: inverted branches, dropped stores.
   Localise a divergence by the ordered write trace; the first differing write
   names the instruction.

Method:

9. Hand work from the listing for every T2 -> T4 step. Generated drafts are
   T1 intake only; several TGR "walls" were draft structure carried into
   source. No sweeps, batches or search loops. Forcing one decision as a
   diagnostic is allowed; forced output is never kept.
10. Walls are dated verdicts. Re-triage when a new lever lands; dead spellings
    stay dead.
11. Entry contracts first. The tgrally analogue: wrong prototypes caused the
    last residue on several giants. Here the M/X/DB/D state at entry decides
    how every byte after it decodes, and what the C receives and returns.
12. Witness builds remove source uncertainty. Measure their yield before
    building on them (string anchoring was overstated 30x on TGR).

Toolchain:

13. In the repo, pinned, checksummed, never on the host.
14. Validate on a control set, never on a hard routine: the wrong compiler gave
    the prettiest number on brally because most of the function was never
    compared.
15. Delete outputs before every build; a failed build once graded stale
    objects as passing.

Bookkeeping:

16. File as you go, by architecture; `WHAT IT DOES:` above each routine; never
    address-batch files. The hook enforces it.
17. Commit each verified routine at once, with pathspecs. Lock a routine
    before working it; parallel sessions split by file.
18. Learnings go into the repo (`docs/stuntracefx/HANDBOOK.md` plus dated
    notes) when found, not only into session notes.
19. No provenance traces anywhere: files, comments, tool output, commit messages. No
    em dashes. No attribution trailers. provcheck runs from the first commit.
20. Ports never edit decomp source (`src/`). The native port lifts its own
    copy (`ports/stuntracefx/game`) and checks it against the original; see
    `ports/stuntracefx/PLAN.md`.
21. Parallelise long runs across the cores; heavy jobs one at a time.

## 7. Risks named now

- 65816 static disassembly is ambiguous (widths, jump tables, `RTS`-dispatch,
  inline arguments after `JSR`). Mitigation: trace-driven code map; static
  decode only for traced entry states.
- Cross-processor addresses: the GSU sees ROM and RAM at its own addresses.
  Symbols carry both views; the shift test catches a missed one.
- Frame pacing depends on GSU completion. Lockstep compares at logical-frame
  sync points; bank-sized shifts keep cycle counts.
- GSU code copied to cache or RAM at run time needs its source address and run
  address both labelled.
- Emulator accuracy. Two cores, control scripts.
- The C writes the machine model directly (WRAM, GSU RAM, PPU registers). The
  first port supplied the same model natively; the native port replaces it
  with its own components (`ports/stuntracefx/PLAN.md`).
