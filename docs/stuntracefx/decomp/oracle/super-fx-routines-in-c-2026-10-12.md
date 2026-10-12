# Super FX routines in the oracles

*Recorded 2026-10-12.*

> The harness runs the C of a Super FX routine in place of the routine,
> per call (A5) and in lockstep (A7), the same way it runs S-CPU routines.
> Proved on FxTableEntryWords (04:CD83): A5 IDENTICAL on every call, A7
> IDENTICAL on all 18 scripts with it swapped in alongside every S-CPU
> routine.

**Pieces.** The core patch `tools/toolchains/patches/bsnes/0002-sfxbox-gsu-hooks.patch`
adds Super FX events (each instruction about to run, with the address just
past the last byte the previous instruction fetched and its opcode; each
clock step; each game pak RAM access; each code-cache line filled and each
cache flush), a run-in-place hook in `SuperFX::main`, and calls to read and
write the whole Super FX state, spend clocks on its timeline, make one bus
access, fill a cache line, flush the cache and fetch a code byte without
time. The harness side is `tools/stuntracefx/sfxbox/gsucall.c`, driven by
`check.c`. The C sees the Super FX through `src/stuntracefx/include/gsu.h`:
the register file `gsu` (R0-R15, Z, CY, S, OV, ROMBR, RAMBR, COLR, POR,
SCMR, SCBR, the last RAM address SBK writes back to, the ROM buffer, and the
prefix the caller left pending), `gsu_rd8`/`gsu_wr8` on the Super FX's bus,
`gsu_plot`/`gsu_rpix` (the harness keeps the two pixel caches exactly as
the core does), and helpers for LDW/STW (low byte at a, high at a ^ 1),
writing R14 (`gsu_r14`), GETB and the flags of ADD, SUB, MOVES and LMULT.
`hybrid.py` puts routines the catalog marks `gsu` in a table of their own.

**Where a routine starts and ends.** The Super FX has no stack and no call
instruction. A call is `LINK #n` (R11 = return address) and a jump (`IWT
R15` or `JMP Rn`) whose delay slot runs before the callee's first
instruction; a return is a jump to R11 (`JMP R11`, `MOVE R15,R11`, or a
saved R11 pulled into R15) with its own delay slot. A routine with C is
entered when the Super FX is about to run its first instruction, arrived at
by a jump (not by falling through), with R11 equal to the address just past
the instruction that ran last (the delay slot of the LINK call). A prefix in
that delay slot (WITH, TO, FROM, ALT1-3) applies to the routine's first
instruction: 01:D0F3 is always called with `WITH R4` there, so its first
`SUB R4` clears R4. The C gets the pending prefix in `gsu.pre_b`,
`pre_alt1`, `pre_alt2`, `pre_sreg`, `pre_dreg`, and the hybrid restores the
prefix state the original has at the return. The call ends when the Super FX is
about to run the instruction at that R11 in the same program bank; R15 is
then one past it and the pipeline already holds its opcode. A program the
S-CPU starts by writing R15 is entered at the instruction after the NOP that
the last STOP left in the pipeline and ends when it is about to run its
STOP: the C does everything before the STOP, the core runs the STOP itself
(its IRQ and the halt). A tail branch into a routine is not an entry: the C
of the routine that branches calls the C of the one it branches into.

**A5.** At the entry the Super FX's state is taken; its RAM writes are
recorded with the values they replaced; at the return the C runs against
the entry state and must leave the same R0-R14, flags, COLR, POR, ROMBR,
RAMBR, last RAM address, pixel caches, ROM buffer and RAM. Two buffers need
care. A store still in the RAM buffer at entry lands during the call: it
counts as done at entry. A store still in it at the return lands after: it
counts as done at the return. The ROM buffer is a value of its own. In the
core every write of R14, even of the value it already holds, starts a fill
of 5 or 6 clocks after the instruction; the fill lands in a later clock
step with ROMBR:R14 as they are then and costs nothing itself; a GETB
before it lands waits for the rest; ROMB completes a pending fill before it
changes the bank, so ROMB alone never refills. The model therefore keeps
the buffer's value: taken at entry (a pending fill counted as done),
refilled by `gsu_r14`, read by `gsu_getb`. Two shortcuts failed first:
computing it from the final ROMBR:R14 (31 of 1,334 attract-mode calls of
04:CD83 differed, all after a ROMB), and keeping the entry value when R14
ends as it started (210 of 1,976 battle calls of 01:81F7 differed: it
switches bank, reads, and writes the saved R14 back).

**A7.** The original side records each call as what the Super FX did: clock
steps, RAM reads and writes, cache fills and flushes, and at the return
the buffers' state and any pending prefix. The hybrid runs the C on the live
Super FX at the entry. Each RAM access of the C first replays the
original's steps up to the original's matching access (same kind, same
byte), then is made for real, so Super FX RAM changes at the same Super FX
clock and the S-CPU (which polls the status register), a screen DMA and the
frame comparison see the same bytes at the same time; a frame that ends
inside a Super FX call needs no special case. After the C, the rest of the
steps are replayed with the cache fills and flushes, and the Super FX is
left as the original was at the return: the C's registers and flags, R15
and the pipeline at the return address, the buffers as recorded. Before the
C runs, a ROM-buffer fill pending at entry is completed at once and a
pending store is made at the original's matching write, so no buffered
access is left in the core while the C runs. Frames also compare the number
of Super FX calls each side made.

**Proof beyond 04:CD83.** Throwaway C (not filed) for program 04:BF75,
01:D0DB (four GETBs and R14 writes) and 01:D0F3 (entered with a pending
WITH R4; a bit reader that refills from ROM): A5 IDENTICAL on every call
(69,956 battle calls of 01:D0F3), lockstep IDENTICAL with them swapped in.

**Negative controls.** Adding 2 to the result in the C of 04:CD83 fails
lockstep on attract (Super FX RAM differs from frame 1584 and the S-CPU
later falls off the original's timeline).

**Limits.**

- The C reads ROM at once and ROM is constant; RAM it reads at the matching
  original read. A RAM access the original did not make is made at once.
- While the C runs, the core's registers keep their entry values: an S-CPU
  read of R0-R15 in that window would see them. The game polls only the
  status register while the Super FX runs, and the bits it tests (GO) are
  the same.
- A call that never returns (its program STOPs inside it) is reported, not
  run: lockstep counts it as "not returning" and fails.
- A routine entered through a jump table (catalog entry `table`) passes
  none of the entry tests; its C would never run. None has C yet.
- C must write R14 only through `gsu_r14`; a plain assignment leaves the
  ROM buffer stale, which A5 reports as `romdr`.
- C that calls a Super FX routine without C has no equivalent of the
  S-CPU's call_routine yet: write callees first.
- The core's Thread stack is 32 KiB; the C runs on the Super FX's thread,
  so it must not use large locals.
- `sfxbox` refuses to start when the library has Super FX routines and the
  core lacks the hooks (rebuild with `./setup.sh bsnes`).
