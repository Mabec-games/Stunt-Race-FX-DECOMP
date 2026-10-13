#!/usr/bin/env python3
"""prune.py: drop the play chunks that add no coverage.

    prune.py            report what would be dropped
    prune.py --apply    remove those chunk scripts

Gate time is set by the frames the gate scripts run. The chunks of the
plays (chunk.py) overlap heavily: the same race code runs in every race.
Every script that is not a chunk is kept; then chunks are taken greedily,
each time the one that executes the most ROM bytes (work RAM code folded
in; inside a routine, read as data too) that no kept script does, until no
chunk adds any. The union of the kept traces equals the union of all of
them, so gate C counts the same bytes. Run build.py first so every script
has its trace.
"""
import glob, os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, 'tools', 'stuntracefx'))
import codemap  # noqa: E402

SCRIPTS = os.path.join(ROOT, 'tools', 'stuntracefx', 'scripts')
BOX = os.path.join(ROOT, 'build', 'stuntracefx', 'box')
CHUNK = re.compile(r'_[0-9][0-9]$')


def routine_bytes():
    """ROM offsets inside the catalog's routines (where a byte read as data
    is a table of the routine, which gate C counts)."""
    import csv
    out = set()
    for r in csv.DictReader(open(os.path.join(ROOT, 'build', 'stuntracefx', 'routines.csv'))):
        out.update(range(int(r['offset'], 16), int(r['end'], 16)))
    return out


INSIDE = None


def touched(name):
    """Bytes executed, and bytes inside a routine read as data."""
    global INSIDE
    if INSIDE is None:
        INSIDE = routine_bytes()
    m = codemap.merged([os.path.join(BOX, name)])
    return {i for i, v in enumerate(m) if v & 0x33 or (v & 0x44 and i in INSIDE)}


def main(argv):
    names = sorted(os.path.splitext(os.path.basename(s))[0] for s in glob.glob(os.path.join(SCRIPTS, '*.sfx')))
    missing = [n for n in names if not os.path.exists(os.path.join(BOX, n, 'rommap.bin'))]
    if missing:
        print('no trace for %s; run build.py first' % ' '.join(missing[:5]))
        return 1
    sets = {n: touched(n) for n in names}
    covered = set()
    for n in names:
        if not CHUNK.search(n):
            covered |= sets[n]
    chunks = [n for n in names if CHUNK.search(n)]
    keep = []
    while True:
        best = max(chunks, key=lambda n: len(sets[n] - covered), default=None)
        if best is None or not sets[best] - covered:
            break
        keep.append(best)
        covered |= sets[best]
        chunks.remove(best)
    total = set().union(*sets.values())
    assert covered == total
    print('%d chunks kept, %d dropped; %d bytes covered either way' % (len(keep), len(chunks), len(total)))
    if '--apply' in argv:
        for n in chunks:
            os.remove(os.path.join(SCRIPTS, n + '.sfx'))
    else:
        print('dropped: ' + ' '.join(sorted(chunks)))
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
