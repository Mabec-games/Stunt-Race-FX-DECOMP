#!/usr/bin/env python3
"""spccat.py: the catalog of the SPC700's routines, from the traces.

    spccat.py       write build/stuntracefx/spc_routines.csv and summarize

A routine starts where the SPC700 was started ($0800, the start address of
every block list), at the target of a CALL, PCALL or TCALL it ran, or where
a JMP (!a+X) (a dispatch through a table) went: an entry of a block when
the same block holds the call too (its ROM byte at the call's APU address
is that opcode, a CALL's operand that target), so a call the battle
driver makes is not an entry of the race driver at the same address. Its
row gives the APU
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
    apu = {}
    for dst, n, o in blocks:
        for k in range(n):
            apu[o + k] = (dst + k) & 0xFFFF
    mp = codemap.merged()
    # ROM offset -> entry kind: the start address in each block that has it,
    # and each call or dispatch target at the block version its trace ran.
    entries = {}
    for name, (bl, st) in ls.items():
        for dst, n, o in bl:
            if st is not None and dst <= st < dst + n and mp[o + st - dst] & spcmap.SPC_OP:
                entries[o + st - dst] = 'start'
    edges = set()
    for path in glob.glob(os.path.join(B, 'box', '*', 'spc_edges.csv')):
        for r in csv.DictReader(open(path)):
            op = int(r['opcode'], 16)
            if op in CALLS:
                edges.add((op, int(r['from'], 16), int(r['to'], 16)))
    for op, frm, to in sorted(edges):
        for dst, n, o in blocks:
            if not (dst <= frm < dst + n and dst <= to < dst + n):
                continue
            of, ot = o + frm - dst, o + to - dst
            if rom[of] != op or not mp[of] & spcmap.SPC_OP or not mp[ot] & spcmap.SPC_OP:
                continue
            if op == 0x3F and (rom[of + 1] | rom[of + 2] << 8) != to:
                continue
            entries.setdefault(ot, CALLS[op])
    rows = []
    for name, (bl, _) in sorted(ls.items()):
        for dst, n, o in bl:
            here = sorted(x for x in entries if o <= x < o + n)
            for i, off in enumerate(here):
                lim = here[i + 1] if i + 1 < len(here) else o + n
                end = off
                for x in range(off, lim):
                    if mp[x] & codemap.SPC_CODE:
                        end = x + 1
                rows.append({'apu': '%04X' % apu[off], 'entry': entries[off], 'block': name, 'offset': '%06X' % off,
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
