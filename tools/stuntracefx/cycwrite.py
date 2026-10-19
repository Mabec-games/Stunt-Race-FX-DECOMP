#!/usr/bin/env python3
"""cycwrite.py: the solved costs written into the decomp's C, as cyc().

    cycwrite.py COSTS.csv [--skip FUNCS.txt] [--dry]

COSTS.csv is cycsolve.py's (id, ..., cost), numbered as cycprobe.py numbers
the probes of the tree as it is now (so it comes from probes of this tree).
Each probe with a cost becomes the time its place stands for:

    a block's start, an arm, a case, the place paths join:  cyc(N);
    an arm with no block of its own:                         { cyc(N); it }
    the path that skips an arm:                              else { cyc(N); }
    a branch inside an expression (&&, ||, ?:):              (cyc(N), it)
    a bus access (rd8, wr8, a call into code that reaches the bus):
        the first in its statement, its statement always run from its
        start:                                               cyc(N); before the statement
        else (after another access in the statement, in a loop's test,
        in an arm of a condition):                           its last argument as cyc_then(N, it)

Every one is exact: the time comes where the probe stood, on the same
paths. Functions named in FUNCS.txt (lines "path function") are left as
they are: the ones whose C does not tell apart paths of the original that
take different times, fixed by hand first. --dry reports without writing.
"""
import collections, csv, os, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import cycprobe

ROOT = cycprobe.ROOT


def plan(path, text, got, cost, skip):
    """The edits for one file: (offset, rank, text) with rank ordering the
    edits at one offset, and what was left out and why."""
    rel = os.path.relpath(path, ROOT)
    stmt = collections.Counter()           # offset -> clocks before the statement there
    wraps = {}                             # (start, end) -> clocks, a body given braces
    skips = []                             # (offset, wrapped start, clocks)
    exprs = []                             # (start, end, opener text)
    left = collections.Counter()
    for pid, kind, m in got:
        n = cost.get(pid, 0)
        if not n:
            continue
        if (rel, m['fn']) in skip:
            left['in a function fixed by hand first'] += 1
            continue
        form = m['form']
        if form == 'stmt':
            stmt[m['at']] += n
        elif form == 'wrap':
            wraps[(m['at'], m['end'])] = wraps.get((m['at'], m['end']), 0) + n
        elif form == 'skip':
            skips.append((m['at'], m['wrapped'][0], n))
        elif form == 'expr':
            exprs.append((m['at'], m['end'], '(cyc(%d), ' % n))
        elif form == 'arg':
            a = m['anchor']
            if a and not m['unsafe'] and m.get('first'):
                if a[1]:
                    wraps[(a[0], a[2])] = wraps.get((a[0], a[2]), 0) + n
                else:
                    stmt[a[0]] += n
            elif not cycprobe.number_type(m['typ']):
                left['an access inside an expression, its last argument not a number'] += 1
            else:
                exprs.append((m['at'], m['end'], 'cyc_then(%d, ' % n))
    edits = []
    # a statement's time: on a line of its own where the statement starts one
    merged = collections.Counter()
    for at, n in stmt.items():
        j = at
        while j < len(text) and text[j] in ' \t\n':
            j += 1
        merged[j] += n
    for at, n in merged.items():
        ls = text.rfind('\n', 0, at) + 1
        indent = text[ls:at]
        if indent.strip() == '':
            if text[at:at + 1] == '}':
                indent += '    '
            edits.append((at, (1, -2, 0), 'cyc(%d);\n%s' % (n, indent)))
        else:
            edits.append((at, (1, -2, 0), 'cyc(%d); ' % n))
    # braces: at one offset, closings before openings; an inner body's
    # closing before the outer's, an outer opening before the inner's
    for (b, e), n in wraps.items():
        edits.append((b, (1, -3, -e), '{ cyc(%d); ' % n))
        edits.append((e, (0, -b, 0), ' }'))
    for at, b, n in skips:
        edits.append((at, (0, -b, 1), ' else { cyc(%d); }' % n))
    for b, e, opener in exprs:
        edits.append((b, (1, -1, -e), opener))
        edits.append((e, (0, -b, 0), ')'))
    return edits, left


def apply(text, edits):
    out, at = [], 0
    for off, _rank, s in sorted(edits, key=lambda x: (x[0], x[1])):
        out.append(text[at:off])
        out.append(s)
        at = off
    out.append(text[at:])
    return ''.join(out)


def main(argv):
    args = argv[1:]
    dry = '--dry' in args
    if dry:
        args.remove('--dry')
    skip = set()
    if '--skip' in args:
        i = args.index('--skip')
        for line in open(args[i + 1]):
            if line.strip():
                f, fn = line.split()
                skip.add((f, fn))
        del args[i:i + 2]
    if len(args) != 1:
        print(__doc__)
        return 2
    cost = {}
    for r in csv.DictReader(open(args[0])):
        if r['cost'] not in ('', '0'):
            cost[int(r['id'])] = int(r['cost'])
    files, written, left = 0, 0, collections.Counter()
    for path, text, got in cycprobe.all_probes():
        edits, why = plan(path, text, got, cost, skip)
        left.update(why)
        if not edits:
            continue
        files += 1
        written += sum(1 for e in edits if e[2].startswith(('cyc', '{ cyc', ' else', '(cyc')))
        if not dry:
            with open(path, 'w') as f:
                f.write(apply(text, edits))
    print('cycwrite: %d costs in %d files%s' % (written, files, ' (dry run)' if dry else ''))
    for why, n in left.most_common():
        print('  %6d left: %s' % (n, why))
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
