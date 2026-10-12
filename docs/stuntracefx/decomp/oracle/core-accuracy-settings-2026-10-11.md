# The emulator core runs with every speed shortcut off

*Recorded 2026-10-11.*

> sfxbox answers the core's option queries itself; a frontend that answers
> none gets the core's fast defaults.

The pinned bsnes core defaults to a scanline PPU, a fast DSP and delayed
coprocessor synchronisation. The first sfxbox answered no option queries, so
the first traces ran with all three. Code and data marks from those traces
are unaffected (which bytes run does not depend on them), but anything that
compares machine state frame by frame must not run on them.

sfxbox now fixes `bsnes_ppu_fast`, `bsnes_dsp_fast` and
`bsnes_coprocessor_delayed_sync` to OFF and every overclock to 100. The
attract script takes 34 s instead of 15 s, which confirms the settings apply.
All traces were rerun; the intake still reassembles to the original ROM.

bsnes is an accurate core, not a proven cycle-exact model of the Super FX.
The plan's second core (snes9x, fetched, not yet built) cross-checks it on the
control scripts before any live verdict is trusted.
