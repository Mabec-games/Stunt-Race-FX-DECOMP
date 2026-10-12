#!/usr/bin/env python3
"""codemap.py: one view of what every ROM byte is, from all traces.

The ROM map of each trace says how each ROM byte was touched. Code the S-CPU
runs from work RAM was copied there from ROM (config/stuntracefx/ramcode.csv);
its fetches are traced in RAM and folded back onto the ROM bytes they came
from. Every tool that needs the code map or a byte's run address uses this
module, so they agree.

    codemap.py          summary: executed bytes per processor, RAM-run code folded
"""
import csv, glob, os, struct, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
B = os.path.join(ROOT, 'build', 'stuntracefx')
RAMCODE = os.path.join(ROOT, 'config', 'stuntracefx', 'ramcode.csv')
CPU_CODE, GSU_CODE = 0x03, 0x30


def ramcode():
    """[(name, run address, ROM offset, length)]"""
    if not os.path.exists(RAMCODE):
        return []
    return [(r['name'], int(r['run'], 16), int(r['rom'], 16), int(r['length'], 16)) for r in csv.DictReader(open(RAMCODE))]


_RC = None


def _rc():
    global _RC
    if _RC is None:
        _RC = ramcode()
    return _RC


def run_addr(off):
    """The address the S-CPU runs ROM offset `off` at (LoROM, or its RAM copy)."""
    for name, run, rom, n in _rc():
        if rom <= off < rom + n:
            return run + (off - rom)
    return (off // 0x8000) << 16 | 0x8000 | (off % 0x8000)


def wram_offset(addr):
    bank, lo = addr >> 16 & 0xFF, addr & 0xFFFF
    if bank in (0x7E, 0x7F):
        return (bank & 1) << 16 | lo
    if (bank & 0x40) == 0 and lo < 0x2000:
        return lo
    return None


def rom_offset(addr):
    """ROM offset of S-CPU address `addr` (LoROM, linear banks, or a RAM copy), or None."""
    w = wram_offset(addr)
    if w is not None:
        for name, run, rom, n in _rc():
            rw = wram_offset(run)
            if rw <= w < rw + n:
                return rom + (w - rw)
        return None
    bank, lo = addr >> 16 & 0xFF, addr & 0xFFFF
    if (bank & 0x7F) < 0x40 and lo >= 0x8000:
        return (bank & 0x3F) * 0x8000 + (lo - 0x8000)
    if 0x40 <= (bank & 0x7F) < 0x60:
        return addr & 0x3FFFFF
    return None


def traces():
    return sorted(os.path.dirname(p) for p in glob.glob(os.path.join(B, 'box', '*', 'rommap.bin')))


def merged(dirs=None):
    """ROM-indexed flags from every trace, with RAM-run code folded in."""
    out = None
    for d in (dirs if dirs is not None else traces()):
        raw = open(os.path.join(d, 'rommap.bin'), 'rb').read()
        m = struct.unpack('<%dH' % (len(raw) // 2), raw)
        if out is None:
            out = [0] * len(m)
        for i, v in enumerate(m):
            if v:
                out[i] |= v
        wc = os.path.join(d, 'wramcode.bin')
        if os.path.exists(wc):
            raw = open(wc, 'rb').read()
            w = struct.unpack('<%dH' % (len(raw) // 2), raw)
            for name, run, rom, n in _rc():
                rw = wram_offset(run)
                for i in range(n):
                    if w[rw + i]:
                        out[rom + i] |= w[rw + i]
    return out or []


def main():
    mp = merged()
    folded = sum(n for _, _, _, n in _rc())
    print('S-CPU code bytes %d, Super FX code bytes %d; RAM-run blocks %d (%d ROM bytes)' % (
        sum(1 for v in mp if v & CPU_CODE), sum(1 for v in mp if v & GSU_CODE), len(_rc()), folded))
    return 0


if __name__ == '__main__':
    sys.exit(main())
