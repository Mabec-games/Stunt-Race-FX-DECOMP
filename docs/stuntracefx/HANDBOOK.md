# Stunt Race FX handbook

What this decomp learns about the hardware, the code and the method, one rule
per finding, each linked to the dated note that measured it. New findings go
here when they are made.

## Toolchain

- asar 1.91 encodes every 65816 instruction form (268, both immediate widths)
  and every Super FX opcode byte (256) exactly, given an explicit operand size
  on every 65816 instruction ([decomp/toolchain/asar-control-2026-10-11](decomp/toolchain/asar-control-2026-10-11.md)).
- asar reads a bare number as a branch operand as a raw displacement, not a
  target address. Branches use labels or raw displacements, never addresses.

## Code

- No C-compiler stack frames (`TSC; SEC; SBC #n; TCS`) anywhere in the ROM:
  the game is hand-written assembly.
- The S-CPU runs code from ROM banks 00-0B and from WRAM; the Super FX runs
  from ROM banks 01 and 04 (LoROM view), first attract-mode trace.

## Running the game

- Menu flow, controls and what each script reaches:
  [decomp/log/phase1-skeleton-2026-10-11](decomp/log/phase1-skeleton-2026-10-11.md).

## Oracle

- The core runs with its speed shortcuts off; a frontend that answers no
  option queries gets the fast defaults
  ([decomp/oracle/core-accuracy-settings-2026-10-11](decomp/oracle/core-accuracy-settings-2026-10-11.md)).
- A5 checks each call against its C with no timing involved; A7 runs the C in
  place in lockstep and must repeat the original's exact bus cycles, not just
  its clock count; a frame can end inside a call; a replay can outlive its
  frame ([decomp/oracle/call-and-lockstep-oracles-2026-10-11](decomp/oracle/call-and-lockstep-oracles-2026-10-11.md)).

- Super FX routines run as C too: entered when the Super FX is about to run
  a routine's first instruction after a LINK call (R11 = just past the
  delay slot) or a program's first after the S-CPU starts it, ended at the
  instruction at R11 or at the program's STOP; lockstep places each RAM
  access of the C on the original's and replays its clock steps and cache
  events ([decomp/oracle/super-fx-routines-in-c-2026-10-12](decomp/oracle/super-fx-routines-in-c-2026-10-12.md)).

## Method

- Routines that assembly still calls carry an abi_ wrapper: the readable C
  takes arguments and returns values; the wrapper maps the 65816 registers and
  flags to and from them and is the entry contract in code.
- Order of work is bottom up: a routine's C calls its callees' C, so callees
  reach T3 first.
