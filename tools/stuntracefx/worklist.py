#!/usr/bin/env python3
"""worklist.py: routines ready to transcribe next, bottom up.

    worklist.py [--cpu cpu|gsu] [--limit N]

A routine is ready when every byte from its start to its last executed byte
was executed by the scripts (no untraced holes, so Gate C can pass) and every
routine it calls already has C. Listed smallest first within each bank.
"""
import argparse, csv, os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import codemap, progress  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--cpu', default=None)
    ap.add_argument('--limit', type=int, default=40)
    a = ap.parse_args()
    rows = list(csv.DictReader(open(os.path.join(ROOT, 'build', 'stuntracefx', 'routines.csv'))))
    mp = codemap.merged()
    done = {k for k, v in progress.tagged().items() if v[0]}
    starts = {int(r['offset'], 16): r for r in rows}
    edges = []
    for d in codemap.traces():
        edges += list(csv.DictReader(open(os.path.join(d, 'edges.csv'))))
    calls = {}
    rom = open(os.path.join(ROOT, 'build', 'stuntracefx', 'baserom.sfc'), 'rb').read()
    for e in edges:
        so, to = codemap.rom_offset(int(e['from'], 16)), codemap.rom_offset(int(e['to'], 16))
        if so is None or to is None:
            continue
        if e['cpu'] == 'cpu' and int(e['kind'], 16) in (0x20, 0x22, 0xFC):
            calls.setdefault(so, set()).add(to)
        elif e['cpu'] == 'gsu' and not (so >= 1 and rom[so - 1] == 0x00) and int(e['kind'], 16) != 0x00:
            # Super FX: a LINK call or a branch or jump into another routine
            # (a jump from the NOP after a STOP is the S-CPU starting a program).
            calls.setdefault(so, set()).add(to)
    ready = []
    for r in rows:
        if a.cpu and r['cpu'] != a.cpu:
            continue
        addr = int(r['addr'], 16)
        if addr in done:
            continue
        lo, hi = int(r['offset'], 16), int(r['end'], 16)
        mask = 0x03 if r['cpu'] == 'cpu' else 0x30
        if any(not mp[o] & mask for o in range(lo, hi)):
            continue
        callees = set()
        for o in range(lo, hi):
            callees |= calls.get(o, set())
        pending = [c for c in callees if c in starts and codemap.run_addr(c) not in done and c != lo]
        if pending:
            continue
        ready.append((int(r['bytes']), r['addr'], r['cpu'], len(callees)))
    ready.sort()
    for b, addr, cpu, n in ready[:a.limit]:
        print('%s %-3s %5d B  calls %d' % (addr, cpu, b, n))
    print('%d ready' % len(ready))
    return 0


if __name__ == '__main__':
    sys.exit(main())
