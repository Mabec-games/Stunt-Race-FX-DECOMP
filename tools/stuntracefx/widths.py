#!/usr/bin/env python3
"""widths.py: the register widths each routine's first instruction ran with.

    widths.py ADDR [ADDR...]

From the merged trace map: for the first byte of each routine, whether the
accumulator (M) and index registers (X) were seen 8-bit, 16-bit or both,
and whether it ran in emulation mode. Used to write a routine's Entry line.
"""
import os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import codemap  # noqa: E402

M8, M16, X8, X16, EMU = 1 << 8, 1 << 9, 1 << 10, 1 << 11, 1 << 12


def width(flags, b8, b16):
    return {(1, 0): '8', (0, 1): '16', (1, 1): '8/16'}.get((bool(flags & b8), bool(flags & b16)), '?')


def main(argv):
    mp = codemap.merged()
    for a in argv:
        addr = int(a.replace(':', '').replace('$', ''), 16)
        off = codemap.rom_offset(addr)
        f = mp[off] if off is not None else 0
        print('%06X  M %-4s X %-4s%s' % (addr, width(f, M8, M16), width(f, X8, X16), '  emulation' if f & EMU else ''))
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
