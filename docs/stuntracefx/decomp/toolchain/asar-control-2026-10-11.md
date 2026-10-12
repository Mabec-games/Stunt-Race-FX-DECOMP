# asar 1.91 control

*Recorded 2026-10-11.*

> The pinned assembler reproduces every instruction form the decomp can emit.

`tools/stuntracefx/asmcontrol.py` builds an image holding all 256 65816
opcodes (the twelve width-dependent immediates in both widths, 268 forms) and
all 256 Super FX opcode bytes, writes it through the same text forms the raw
intake uses, assembles it with asar 1.91 and compares: 0 bytes differ.

The first raw intake of the game (attract-mode trace: 9,867 S-CPU and 19,752
Super FX instructions, the rest data) also reassembled byte-exact on the first
build.

Super FX prefix bytes (ALT1-3, TO, WITH, FROM) are emitted as their own
instructions in the raw intake; the bytes are the same as asar's combined
forms (`adc`, `move`, `lms`), which the filed source uses for readability.
