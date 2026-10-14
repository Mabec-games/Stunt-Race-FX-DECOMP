# Routine census

How many routines Stunt Race FX has, counting the ones no script has run.
Taken at 6cd8b4b with `tools/stuntracefx/census.py` (about 4 seconds; static,
reads the traces already in build/, runs nothing).

## The answer

**About 1,370 routines; between 1,300 and 1,470.**

| | count | confidence |
|---|---:|---|
| Named: the catalog (952 S-CPU, 159 Super FX) | 1,111 | traced |
| Named: `@routine` tags outside the catalog (jump-table handlers, dead code) | 32 | filed |
| Call targets the catalog has not split out: ran (02CF3E 02CF45 02CFC6) | 3 | certain |
| Call targets in code no script ran | 63 | high |
| Handler addresses stored in data (objects, actors) | 29 | high |
| Interrupt vector with its own handler (00FE90, a lone RTI) | 1 | certain |
| Super FX routines (one ran, 049A9B; 18 reached by LINK + IWT R15) | 19 | high |
| Orphans: code nothing refers to | 81 | real code; some may be dead tails of their neighbour, not routines |
| Jump-table targets in code not yet filed | 126 | about 30 will be routines, see below |
| **Total** | **about 1,370** | |

The 1,130 quoted before was the catalog plus the tags filed so far. The
catalog only counts routines that ran, so it was always a floor.

## Jump tables: entry points that are not always routines

The census finds 274 JMP (abs,X) targets that nothing names. 148 sit inside
routines already filed as C, where the decomp made them `case`s of their
dispatcher. Of all the jump-table targets in filed code, 46 of 194 (24%)
became routines of their own (the course screen's and the stunt panel's
state handlers). The other 126 are in code not filed yet (86 ran, 40 never
ran). At the same rate about 30 of them become routines. That gives 0 to 126,
expected 30.

## How sure

- **Spot checks.** Samples from every class were disassembled by hand. All
  were code: 07C911, 03CDC1, 0BD6F8, 08CBCC (a Super FX start through
  GsuRun), 088F91, 0BBE29, the orphans 03D782, 088A59 (stores its own
  handler, 088A75), 09C0BE, 0BF443, 7EB236 and 08CE79. The Super FX finds
  start right after a delay-slot NOP, and several share the prologue
  `CACHE; IWT R12`.
- **Decoding errors are thrown out.** Code no script ran is decoded with
  guessed register widths. A guessed block counts only if it ends cleanly
  (RTS, RTL, RTI, a jump). If it runs into data the traces read, into a BRK,
  COP, WDM or STP, into the middle of a traced instruction, or calls or jumps
  to an address that is not code, it is dropped with its targets. 10 blocks
  were dropped. Before this check the run reported calls into low RAM
  (08:18A9, 09:20C2), which were width errors.
- **Recall.** Walking from the reset and interrupt vectors alone finds 546 of
  the catalog's 952 S-CPU routines (57%). Of the 406 it misses, 401 are
  reached in the traces through JSR (abs,X) tables or calls inside code the
  vector walk never enters: the NMI and IRQ trees, which start at the RAM
  jumps $0108 and $010C the game writes at boot. Only 5 are reached by
  something no static walk follows: RTL used as a jump (098B3B), an
  interrupt, and 7E/7F RAM code with no recorded caller. The real census
  seeds from all 1,143 named routines, so those trees are covered.
- **What it cannot see.** Dead code that nothing points to and that calls no
  known routine, and handlers listed only in ROM pointer tables that no
  script ever read. The orphan rule catches dead code that calls known code.
  The rest is probably a few dozen at most. 74,608 bytes in the code banks
  are still neither run, read nor reached, and most of that is data (bank 00
  alone holds 17,701).

## Untouched bytes by bank

Counts the bytes in banks with named code that no script ran or read and the
census did not reach.

| bank | 00 | 01 | 02 | 03 | 04 | 07 | 08 | 09 | 0B |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| bytes | 17,701 | 3,997 | 9,136 | 13,364 | 4,609 | 2,132 | 7,560 | 11,430 | 4,679 |

Most new routines are in banks 07, 08 and 09 (menus, objects, results) and
in the tail of 02 (the car select and its sprites, 02F0FA to 02FF60).

## Rerunning

    python3 tools/stuntracefx/census.py            # counts and every address found
    python3 tools/stuntracefx/census.py --recall   # the vectors-only check

The counts move as scripts are added (traced code is certain, and it anchors
the static walk) and as routines are filed (tags). The addresses each class
found are in the tool's output. They are a worklist of routines no script
has reached yet.
