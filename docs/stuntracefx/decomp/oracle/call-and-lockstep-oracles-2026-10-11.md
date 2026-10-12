# The two oracles, and what it took to make the lockstep run exact

*Recorded 2026-10-11.*

> A5 checks each call against its C on the untouched original; A7 runs the
> game with the C really in place, in lockstep with the original, and
> compares every frame. Both proved on ApuUploadNextByte (04:D788).

**A5, per call** (`sfxbox --check`, `t3.py --live`). The original runs
untouched. On entry to a routine that has C, its writes are recorded with the
values they replaced and its hardware reads in order. At the return the C runs
on the same inputs (entry memory reconstructed by undoing the writes, entry
registers, the hardware reads replayed) and must leave the same registers,
memory and hardware writes. No timing is involved. Interrupt-handler writes
are undone but not compared; calls an interrupt or DMA overlapped are
reported, never judged.

**A7, lockstep** (`sfxbox --lockstep`, `t3.py --image`). After load the
process forks: the parent is the original, the child the hybrid. The parent
records every call's bus cycles; the child runs the C on the live machine,
returns as the routine would, and repeats the original call's bus cycles
without their effects. The parent stays one frame ahead so the child always
has the cycles it needs. WRAM, GSU RAM, VRAM, OAM, CGRAM and APU RAM are
compared after every frame.

Three things had to be right before A7 matched:

1. **Charging a lump of clocks is not enough.** The first version spent the
   original call's clock count in idle cycles. A memory-refresh pause landing
   in the last idle cycle made the hybrid up to 40 clocks late per call, about
   460 clocks per frame during the sound upload, and the SPC700 handshake lost
   a byte per frame. Replaying the exact sequence of reads, writes, idle
   cycles and interrupt polls puts refresh, DMA and interrupts where they fell
   in the original. Frame-end clocks then matched exactly.
2. **A frame can end inside a call.** The original is then mid-routine with
   scratch on its stack; the hybrid has already finished the call. The stack
   is judged at the caller's level for that frame.
3. **A replay can outlive its frame.** The original reuses its cycle buffer
   two frames later while the hybrid can still be replaying a call that
   straddles the frame boundary; the hybrid copies a call's cycles before
   replaying them. Found by tracing every instruction on both sides and
   diffing: the first divergence was a 12-clock slip on exactly such a call.

Negative controls: dropping the bank step from the C fails A5 on the 12 calls
that cross a bank and fails A7 on every script (the boot upload crosses one);
reading the wrong byte fails 1,054,844 of 1,078,556 calls.
