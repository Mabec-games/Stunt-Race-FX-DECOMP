#!/usr/bin/env python3
"""show.py: everything needed to read a routine, in one listing.

    show.py ADDR [ADDR...]      routine listing (raw intake), entry widths, callers, callees

Reading aid only: it prints the traced listing; it writes nothing.
"""
import csv, glob, os, re, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import codemap  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
B = os.path.join(ROOT, 'build', 'stuntracefx')


def main(argv):
    rows = {r['addr']: r for r in csv.DictReader(open(os.path.join(B, 'routines.csv')))}
    text = {}
    for f in sorted(glob.glob(os.path.join(B, 'raw', 'bank*.asm'))):
        lines = open(f).read().splitlines()
        for i, l in enumerate(lines):
            m = re.search(r';\s*([0-9A-F]{6})( gsu)?$', l)
            if m:
                lab = lines[i - 1].strip() if i and lines[i - 1].endswith(':') else ''
                text[m.group(1)] = (lab, l.split(';')[0].rstrip())
    mp = codemap.merged()
    edges = []
    for d in codemap.traces():
        edges += list(csv.DictReader(open(os.path.join(d, 'edges.csv'))))
    for a in argv:
        a = a.upper().replace(':', '').replace('$', '')
        r = rows.get(a)
        if not r:
            print('%s: not a catalog routine' % a); continue
        lo, hi = int(r['offset'], 16), int(r['end'], 16)
        v = mp[lo]
        w = [n for b, n in ((8, 'M8'), (9, 'M16'), (10, 'X8'), (11, 'X16'), (12, 'E')) if v >> b & 1]
        addr = int(a, 16)
        callers = {}
        callees = set()
        for e in edges:
            if e['cpu'] != r['cpu']:
                continue
            k = int(e['kind'], 16)
            if int(e['to'], 16) == addr and k in (0x20, 0x22, 0xFC, 0x00, 0xFF):
                callers[e['from']] = callers.get(e['from'], 0) + int(e['count'])
            so = codemap.rom_offset(int(e['from'], 16))
            if so is not None and lo <= so < hi and k in (0x20, 0x22, 0xFC):
                callees.add(e['to'])
        print('=== %s %s %d B, entry %s, %s' % (a, r['cpu'], int(r['bytes']), r['entry'], ' '.join(w)))
        print('callers: %s' % ' '.join('%s(%d)' % (c, n) for c, n in sorted(callers.items())[:12]))
        print('callees: %s' % ' '.join(sorted(callees)))
        for o in range(lo, hi):
            k = '%06X' % codemap.run_addr(o)
            if k in text:
                lab, ins = text[k]
                if lab:
                    print(lab)
                print('%s %s' % (k, ins))
            elif not mp[o] & (0x03 if r['cpu'] == 'cpu' else 0x30) and (o == lo or mp[o - 1] & 0x33):
                print('%s   (not executed)' % k)
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
