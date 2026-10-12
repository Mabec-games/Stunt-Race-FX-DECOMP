#!/usr/bin/env python3
"""asmcontrol.py: prove the pinned assembler encodes every instruction form.

Builds a synthetic image holding every 65816 opcode (immediates in both
widths) and every Super FX opcode byte, emits it through disasm.py's own
text forms, assembles it, and compares byte for byte. The assembler is
accepted only when this control passes; a game-only check would leave the
forms the game does not use unproven.

    asmcontrol.py
"""
import os, subprocess, sys, tempfile
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import disasm

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
ASAR = os.path.join(ROOT, 'tools', 'toolchains', 'asar', 'asar')


def build():
    rom, mp = bytearray(), []

    def put(bs, flags):
        mp.append(flags)
        mp.extend([disasm.CPU_OPND if flags & disasm.CPU_OP else disasm.GSU_OPND] * (len(bs) - 1))
        rom.extend(bs)

    for op in range(256):
        mode = disasm.OPS[op][1]
        widths = [(disasm.M8 | disasm.X8), (disasm.M16 | disasm.X16)] if mode in ('immm', 'immx') else [disasm.M8 | disasm.X8]
        for w in widths:
            n = disasm.cpu_len(op, w)
            # Branch operands point at themselves so labels never resolve: raw form is tested.
            put(bytes([op]) + bytes((0x12 + i * 0x22) & 0xFF for i in range(n - 1)), disasm.CPU_OP | w)
    for op in range(256):
        n = disasm.gsu_len(op)
        put(bytes([op]) + bytes((0x24 + i * 0x22) & 0xFF for i in range(n - 1)), disasm.GSU_OP)
    # Prefix pairs the disassembler prints as one instruction (alt1-3, with+to, with+from).
    for prefix in (0x3D, 0x3E, 0x3F, 0x21, 0x2A):
        for op in range(256):
            if 0x05 <= op <= 0x0F or op in (0x3D, 0x3E, 0x3F) or (prefix < 0x30 and 0x20 <= op <= 0x2F):
                continue
            put(bytes([prefix]), disasm.GSU_OP)
            n = disasm.gsu_len(op)
            put(bytes([op]) + bytes((0x24 + i * 0x22) & 0xFF for i in range(n - 1)), disasm.GSU_OP)
    pad = (-len(rom)) % disasm.BANK
    rom.extend(b'\xff' * pad)
    mp.extend([0] * pad)
    return bytes(rom), mp


def main():
    rom, mp = build()
    r = disasm.Rom(rom, [mp])
    if r.rejected:
        print('control image decode rejected %d instructions' % len(r.rejected)); return 1
    with tempfile.TemporaryDirectory() as d:
        src = os.path.join(d, 'control.asm')
        out = os.path.join(d, 'control.sfc')
        with open(src, 'w') as f:
            f.write('norom\n' + '\n'.join(r.bank_lines(0)) + '\n')
        p = subprocess.run([ASAR, '--fix-checksum=off', src, out], capture_output=True, text=True)
        if p.returncode or not os.path.exists(out):
            print(p.stdout + p.stderr); return 1
        got = open(out, 'rb').read()[:len(rom)]
    bad = [i for i in range(len(rom)) if i >= len(got) or got[i] != rom[i]]
    n = sum(1 for k in r.kind if k)
    if bad:
        print('assembler control FAILED: %d of %d bytes differ, first at %04X' % (len(bad), len(rom), bad[0])); return 1
    print('assembler control passed: %d instruction forms (%d 65816, %d Super FX), 0 bytes differ' % (
        n, r.kind.count('c'), r.kind.count('g')))
    return 0


if __name__ == '__main__':
    sys.exit(main())
