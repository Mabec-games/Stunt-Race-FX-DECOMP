#!/usr/bin/env python3
"""cycsolve.py: what each stretch of the C costs, from the lockstep run.

    cycsolve.py reduce LOG... OUT.eq        the distinct equations in logs
    cycsolve.py solve PROBES.csv EQ... OUT.csv

A log (sfxbox SFXBOX_CYCLOG, a library built by cycprobe.py) says, for every
bus access of the C placed on the original's record, which probes the C
passed since the access before it, how many CPU clocks the original spent
between the two and how many the C states there (cyc, cpu_idle). Each is an
equation: the sum of the probes' costs, each as often as it was passed,
equals the clocks the C does not state yet. `reduce` keeps each distinct one
once (with how often it was seen), and any set of probe counts seen with
two different clock counts: there the C does not tell apart paths of the
original that take different times. `solve` finds the probes' costs as an
integer program: whole clocks, none negative, every equation met that any
costs can meet, the cost on the access site where the equations leave a
choice. It writes each probe's cost (blank when no equation has it) and
reports the equations no costs meet. (`eliminate`, exact elimination over
the rationals, is kept for looking into a set of rows.)
"""
import collections, csv, struct, sys


SITE = 1 << 30          # an access's call site: SITE | its offset in the library
END = 1 << 29           # a call's end: END | the routine's address


def read_log(path):
    """(unknowns with counts, gap) for each interval: the probes passed, and
    at a call's end, the routine's return; an access the probes do not
    reach (one no probe comes before) keys on its call site. A gap of
    0xFFFFFFFF (overflow, a frame left through the caller's) or 0xFFFFFFFE
    (an interrupt taken in between) is skipped."""
    with open(path, 'rb') as f:
        data = f.read()
    i, n, pc, ops = 0, len(data), 0, False
    while i < n:
        t = data[i]
        i += 1
        if t == ord('O'):               # SFXBOX_CYCLOG_OPS: each record ends in its operations
            ops = True
            continue
        if t == ord('S'):
            if i + 4 > n:
                return
            pc = struct.unpack_from('<I', data, i)[0]
            i += 4
            continue
        if i + 16 > n:
            return
        gap, site, said, cnt = struct.unpack_from('<IIII', data, i)
        i += 16
        if i + 4 * cnt > n:
            return
        ids = list(struct.unpack_from('<%dI' % cnt, data, i)) if cnt else []
        i += 4 * cnt
        if ops:
            if i + 4 > n:
                return
            i += 4 + 5 * struct.unpack_from('<I', data, i)[0]
        if gap >= 0xFFFFFFFE:       # interrupted, or left through the caller's frame
            continue
        gap -= said                 # what the C does not state yet
        if t != ord('A'):
            ids.append(END | (pc & 0xFFFFFF))
        elif not ids:
            ids.append(SITE | site)     # an access with no probe before it
        yield tuple(sorted(collections.Counter(ids).items())), gap


def reduce(logs, out):
    eqs = collections.defaultdict(collections.Counter)
    for p in logs:
        for key, gap in read_log(p):
            eqs[key][gap] += 1
    with open(out, 'w') as f:
        for key, gaps in eqs.items():
            f.write('%s|%s\n' % (' '.join('%d:%d' % kv for kv in key), ' '.join('%d:%d' % g for g in gaps.items())))
    bad = sum(1 for g in eqs.values() if len(g) > 1)
    print('reduce: %d distinct equations, %d with more than one clock count' % (len(eqs), bad))


def load_eqs(paths):
    eqs = collections.defaultdict(collections.Counter)
    for p in paths:
        for line in open(p):
            k, g = line.rstrip('\n').split('|')
            key = tuple(tuple(int(x) for x in kv.split(':')) for kv in k.split()) if k else ()
            for kv in g.split():
                a, b = kv.split(':')
                eqs[key][int(a)] += int(b)
    return eqs


def eliminate(rows):
    """Exact Gaussian elimination over the rationals on sparse rows
    ({probe: count}, clocks). Returns (costs, contradictions): a cost for
    every probe a row pins (the free ones 0), and the rows that reduce to
    0 = something."""
    from fractions import Fraction
    pivots = {}                          # var -> (row dict, rhs) with row[var] == 1
    contra = []
    for r0, g0 in rows:
        r = {p: Fraction(c) for p, c in r0.items()}
        g = Fraction(g0)
        # reduce by existing pivots (repeat: substitution can bring others in)
        changed = True
        while changed:
            changed = False
            for p in list(r):
                if p in pivots and r.get(p):
                    c = r.pop(p)
                    pr, pg = pivots[p]
                    for q, v in pr.items():
                        if q != p:
                            r[q] = r.get(q, 0) - c * v
                            if r[q] == 0:
                                del r[q]
                    g -= c * pg
                    changed = True
        r = {p: v for p, v in r.items() if v}
        if not r:
            if g != 0:
                contra.append((r0, g0, g))
            continue
        p = min(r)                       # the lowest id: earlier code first
        c = r[p]
        r = {q: v / c for q, v in r.items()}
        g = g / c
        # keep pivots fully reduced: substitute into the older ones
        for q, (qr, qg) in list(pivots.items()):
            if p in qr:
                f = qr[p]
                nr = dict(qr)
                del nr[p]
                for s_, v in r.items():
                    if s_ != p:
                        nr[s_] = nr.get(s_, 0) - f * v
                        if nr[s_] == 0:
                            del nr[s_]
                pivots[q] = (nr, qg - f * g)
        pivots[p] = (r, g)
    cost = {}
    for p, (r, g) in pivots.items():
        cost[p] = g                      # the free variables are 0
    return cost, contra


def program(rows, kind):
    """The costs as an integer program (scipy's HiGHS): every cost a whole
    number of clocks, none below 0, each equation met or its miss counted
    (slack, weighted far above any cost), and where the equations leave a
    choice the cost goes to the access site, not the block probes around
    it. Returns ({unknown: cost}, [rows missed])."""
    import numpy as np
    import scipy.sparse as sp
    from scipy.optimize import Bounds, LinearConstraint, milp
    unknowns = sorted({p for r, _ in rows for p in r})
    ix = {p: i for i, p in enumerate(unknowns)}
    nv, ne = len(unknowns), len(rows)
    I, J, V = [], [], []
    for e, (r, _) in enumerate(rows):
        for p, n in r.items():
            I.append(e); J.append(ix[p]); V.append(n)
    A = sp.csr_matrix((V, (I, J)), shape=(ne, nv))
    full = sp.hstack([A, sp.identity(ne), -sp.identity(ne)]).tocsr()
    b = np.array([g for _, g in rows], dtype=float)
    w = np.array([0.0 if p & (END | SITE) or kind(p) == 'access' else 1.0 for p in unknowns])
    res = milp(np.concatenate([w, np.full(2 * ne, 1000.0)]), constraints=LinearConstraint(full, b, b),
               integrality=np.concatenate([np.ones(nv), np.zeros(2 * ne)]), bounds=Bounds(0, np.inf))
    if res.status != 0:
        raise SystemExit('cycsolve: the program has no solution: %s' % res.message)
    x = np.round(res.x[:nv]).astype(int)
    miss = res.x[nv:nv + ne] + res.x[nv + ne:]
    return {p: int(x[i]) for i, p in enumerate(unknowns)}, [rows[e] for e in range(ne) if miss[e] > 1e-6]


def solve(probes_csv, eq_paths, out):
    eqs = load_eqs(eq_paths)
    multi = {k: g for k, g in eqs.items() if len(g) > 1}
    rows = [(dict(k), next(iter(g))) for k, g in eqs.items() if len(g) == 1]
    info = {}
    with open(probes_csv) as f:
        for row in csv.DictReader(f):
            info[int(row['id'])] = row
    cost, missed = program(rows, lambda p: info[p]['kind'])
    with open(out, 'w', newline='') as f:
        w = csv.writer(f)
        w.writerow(['id', 'file', 'line', 'col', 'kind', 'cost'])
        for pid in sorted(info):
            r = info[pid]
            v = cost.get(pid)
            w.writerow([pid, r['file'], r['line'], r['col'], r['kind'], '' if v is None else v])
    print('solve: %d probes seen of %d; %d equations, %d no costs meet, %d sets of counts with two clock counts; '
          '%d costs not 0, %d of them off an access' % (
              sum(1 for p in cost if p in info), len(info), len(rows), len(missed), len(multi),
              sum(1 for p, v in cost.items() if v and p in info),
              sum(1 for p, v in cost.items() if v and p in info and info[p]['kind'] != 'access')))
    where = collections.Counter()
    for r, _ in missed:
        for p in r:
            where[info.get(p, {}).get('file', '?')] += 1
    for k in multi:
        for p, _ in k:
            where[info.get(p, {}).get('file', '?') + ' (two clock counts)'] += 1
    for f, n in where.most_common(15):
        print('  %6d  %s' % (n, f))
    return cost, missed, multi


def main(argv):
    if len(argv) >= 4 and argv[1] == 'reduce':
        reduce(argv[2:-1], argv[-1])
        return 0
    if len(argv) >= 5 and argv[1] == 'solve':
        solve(argv[2], argv[3:-1], argv[-1])
        return 0
    print(__doc__)
    return 2


if __name__ == '__main__':
    sys.exit(main(sys.argv))
