#!/usr/bin/env python3
"""catalog.py: the routine catalog, built from the traces.

    catalog.py          write build/stuntracefx/routines.csv and print a summary

A routine starts at every place the traces saw code entered as a routine:
  S-CPU   targets of JSR, JSL and JSR (abs,X), interrupt entries, and reset
  Super FX  where the S-CPU started the GSU (the instruction after a STOP),
          targets of LINK + IWT R15 calls, and the entries of IWT R15 jump
          tables
Its bytes are the executed code bytes of the same processor from its start
up to the next routine start. Code before the first entry in a bank (reached
only by jumps) starts a routine of its own. Code no trace has run is not in the catalog
yet, so the catalog (and every total built on it) grows with coverage.
Code the S-CPU runs from work RAM is copied there from ROM; it is counted
once its source in ROM is known, not before.
"""
import csv, glob, os, struct, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
B = os.path.join(ROOT, 'build', 'stuntracefx')
OUT = os.path.join(B, 'routines.csv')
CPU_CODE, GSU_CODE = 0x03, 0x30
RESET = 0x00FE88


def rom_offset(addr):
    bank, lo = addr >> 16 & 0xFF, addr & 0xFFFF
    if (bank & 0x7F) < 0x40 and lo >= 0x8000:
        return (bank & 0x3F) * 0x8000 + (lo - 0x8000)
    if 0x40 <= (bank & 0x7F) < 0x60:
        return addr & 0x3FFFFF
    return None


def cpu_addr(off):
    return (off // 0x8000) << 16 | 0x8000 | (off % 0x8000)


def load():
    rom = open(os.path.join(B, 'baserom.sfc'), 'rb').read()
    mp = [0] * len(rom)
    edges = []
    for d in glob.glob(os.path.join(B, 'box', '*', 'rommap.bin')):
        raw = open(d, 'rb').read()
        for i, v in enumerate(struct.unpack('<%dH' % (len(raw) // 2), raw)):
            mp[i] |= v
        e = os.path.join(os.path.dirname(d), 'edges.csv')
        if os.path.exists(e):
            edges += list(csv.DictReader(open(e)))
    return rom, mp, edges


def build():
    rom, mp, edges = load()
    starts = {}  # offset -> (cpu, how)
    o = rom_offset(RESET)
    starts[o] = ('cpu', 'reset')
    for r in edges:
        kind, src, dst = int(r['kind'], 16), int(r['from'], 16), int(r['to'], 16)
        o = rom_offset(dst)
        if o is None or o >= len(rom):
            continue
        if r['cpu'] == 'cpu':
            if kind in (0x20, 0x22, 0xFC):
                starts.setdefault(o, ('cpu', 'call'))
            elif kind == 0x100:
                starts.setdefault(o, ('cpu', 'interrupt'))
        else:
            so = rom_offset(src)
            if kind == 0x00:
                starts.setdefault(o, ('gsu', 'started'))
            elif so is not None and so >= 4 and rom[so - 3] == 0xFF:
                if 0x91 <= rom[so - 4] <= 0x94:
                    starts.setdefault(o, ('gsu', 'call'))
                elif rom[so - 1] == 0x01 or rom[so + 1:so + 2] == b'\xff':
                    starts.setdefault(o, ('gsu', 'table'))
    # Code reached only by jumps (for example the interrupt handlers the RAM
    # vectors jump to) and found before any known entry in its bank.
    for bank in range(len(rom) // 0x8000):
        lo, hi = bank * 0x8000, (bank + 1) * 0x8000
        for mask, cpu in ((CPU_CODE, 'cpu'), (GSU_CODE, 'gsu')):
            first = next((k for k in range(lo, hi) if mp[k] & mask), None)
            if first is None:
                continue
            known = [k for k, (c, _) in starts.items() if c == cpu and lo <= k < hi]
            if not known or first < min(known):
                starts.setdefault(first, (cpu, 'jump'))
    order = sorted(starts)
    rows = []
    for i, o in enumerate(order):
        cpu, how = starts[o]
        mask = CPU_CODE if cpu == 'cpu' else GSU_CODE
        if not mp[o] & mask:
            continue
        end = order[i + 1] if i + 1 < len(order) else len(rom)
        end = min(end, (o // 0x8000 + 1) * 0x8000)
        size = sum(1 for k in range(o, end) if mp[k] & mask)
        last = max((k for k in range(o, end) if mp[k] & mask), default=o)
        rows.append({'addr': '%06X' % cpu_addr(o), 'cpu': cpu, 'entry': how, 'offset': '%06X' % o,
                     'end': '%06X' % (last + 1), 'bytes': size})
    os.makedirs(B, exist_ok=True)
    with open(OUT, 'w', newline='') as f:
        w = csv.DictWriter(f, ['addr', 'cpu', 'entry', 'offset', 'end', 'bytes'])
        w.writeheader()
        w.writerows(rows)
    return rows, mp


def main():
    rows, mp = build()
    for cpu in ('cpu', 'gsu'):
        rs = [r for r in rows if r['cpu'] == cpu]
        mask = CPU_CODE if cpu == 'cpu' else GSU_CODE
        print('%s: %d routines, %d bytes (%d executed code bytes in ROM)' % (
            'S-CPU' if cpu == 'cpu' else 'Super FX', len(rs), sum(r['bytes'] for r in rs),
            sum(1 for v in mp if v & mask)))
    return 0


if __name__ == '__main__':
    sys.exit(main())
