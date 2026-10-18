#!/usr/bin/env python3
"""spccat.py: the catalog of the SPC700's routines, from the traces.

    spccat.py       write build/stuntracefx/spc_routines.csv and summarize

A routine starts where the SPC700 was started ($0800, the start address of
every block list), at the target of a CALL, PCALL or TCALL it ran, or where
a JMP (!a+X) (a dispatch through a table) went. Its row gives the APU
address, the uploaded block (spcmap.lists) and ROM offset of that byte, and
its extent: up to the last traced SPC700 code byte before the next entry
in the same block. Rows are by ROM offset, so the race and battle drivers'
versions of one address are separate routines. The S-CPU and Super FX
catalog (routines.csv, catalog.py) is separate: hybrid.py takes every row
there that is not Super FX as S-CPU code.
"""
import csv, glob, os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import codemap, spcmap  # noqa: E402

ROOT = codemap.ROOT
B = os.path.join(ROOT, 'build', 'stuntracefx')
OUT = os.path.join(B, 'spc_routines.csv')

CALLS = {0x3F: 'call', 0x4F: 'pcall', 0x1F: 'table'} | {k << 4 | 1: 'tcall' for k in range(16)}


def build():
    rom = codemap._rom_bytes()
    ls = spcmap.lists(rom)
    blocks = [b for bl, _ in ls.values() for b in bl]
    mp = codemap.merged()
    entries = {}   # (apu addr) -> kind
    starts = {st for _, st in ls.values() if st is not None}
    for a in starts:
        entries[a] = 'start'
    for path in glob.glob(os.path.join(B, 'box', '*', 'spc_edges.csv')):
        for r in csv.DictReader(open(path)):
            k = CALLS.get(int(r['opcode'], 16))
            if k:
                entries.setdefault(int(r['to'], 16), k)
    rows = []
    for name, (bl, _) in sorted(ls.items()):
        for dst, n, o in bl:
            here = sorted((a, k) for a, k in entries.items() if dst <= a < dst + n
                          and mp[o + a - dst] & spcmap.SPC_OP)
            for i, (a, k) in enumerate(here):
                off = o + a - dst
                lim = o + (here[i + 1][0] - dst if i + 1 < len(here) else n)
                end = off
                for x in range(off, lim):
                    if mp[x] & codemap.SPC_CODE:
                        end = x + 1
                rows.append({'apu': '%04X' % a, 'entry': k, 'block': name, 'offset': '%06X' % off,
                             'end': '%06X' % end, 'bytes': sum(1 for x in range(off, end) if mp[x] & codemap.SPC_CODE)})
    seen, out = set(), []
    for r in sorted(rows, key=lambda r: r['offset']):
        if r['offset'] not in seen:
            seen.add(r['offset'])
            out.append(r)
    return out


def main(argv):
    rows = build()
    with open(OUT, 'w', newline='') as f:
        w = csv.DictWriter(f, fieldnames=['apu', 'entry', 'block', 'offset', 'end', 'bytes'])
        w.writeheader()
        w.writerows(rows)
    print('SPC700: %d routines, %d code bytes in them (%s)' % (len(rows), sum(r['bytes'] for r in rows),
                                                              os.path.relpath(OUT, ROOT)))
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
