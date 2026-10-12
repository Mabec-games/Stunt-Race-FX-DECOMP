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
