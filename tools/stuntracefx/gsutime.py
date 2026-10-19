#!/usr/bin/env python3
"""gsutime.py: the Super FX C's statements of the original's code, learned
from the lockstep and written into the decomp.

    gsutime.py gather [SCRIPT...]   strip the written code, probe, run each
                                    script with the code log, aggregate
    gsutime.py solve                each probe's code (per call context)
    gsutime.py write [--dry]        give the shared helpers their copies,
                                    write the code into src/stuntracefx
    gsutime.py check [SCRIPT...]    the Super FX timing line of the decomp
                                    as it is, per script and summed

The Super FX's time is nearly all fetching its code, priced by where the
code is (gsu_clock.c), so the C states which of the original's bytes each
stretch of it stands for (gsu_code). The lockstep logs, for every interval
between two of the C's accesses, the probes the C passed and the
original's fetches (sfxbox SFXBOX_GSUCYCLOG, SFXBOX_GSUCYCLOG_OPS);
gsuagg.c reduces each run to its distinct intervals, the probes of
functions called from several places qualified by their call sites. The
solve finds, per byte address, how many times each probe fetches it (an
integer program: every interval's fetches met, the code on the C's own
probes before the headers' and a call's end). cyccopy.py gives functions
whose code depends on where they run a copy parameter and a table of each
copy's code; cycwrite.py --code writes it all. Code written by hand (an
expression or lower-case hex) is kept.

The scripts default to config/stuntracefx/gsu_cover.txt: a greedy cover
of the Super FX code in the traces. Work files in build/stuntracefx/gsutime.
"""
import collections, csv, glob, os, pickle, struct, subprocess, sys, time

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
TOOLS = os.path.join(ROOT, 'tools', 'stuntracefx')
B = os.path.join(ROOT, 'build', 'stuntracefx', 'gsutime')
G = os.path.join(B, 'learn')
SFXBOX = os.path.join(ROOT, 'build', 'stuntracefx', 'box', 'sfxbox')
CORE = os.path.join(ROOT, 'tools', 'toolchains', 'bsnes', 'bsnes_libretro.dylib')
ROM = os.path.join(ROOT, 'build', 'stuntracefx', 'baserom.sfc')
COVER = os.path.join(ROOT, 'config', 'stuntracefx', 'gsu_cover.txt')
CTX = 131072
END = 1 << 29
sys.path.insert(0, TOOLS)


def say(*a):
    print(time.strftime('%H:%M:%S'), *a, flush=True)


def run(cmd, **kw):
    p = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True, **kw)
    if p.returncode:
        raise SystemExit('gsutime: %s failed:\n%s' % (cmd[0], (p.stdout + p.stderr)[-3000:]))
    return p.stdout


def scripts(args):
    return args or [l.strip() for l in open(COVER) if l.strip()]


def lockstep(script, lib, out, log=None):
    """One lockstep run (its log of the Super FX's code, when asked)."""
    os.makedirs(out, exist_ok=True)
    env = dict(os.environ)
    if log:
        env['SFXBOX_GSUCYCLOG'] = log
        env['SFXBOX_GSUCYCLOG_OPS'] = '1'
    subprocess.run(['perl', '-e', 'alarm 1200; exec @ARGV', SFXBOX, '--core', CORE, '--rom', ROM, '--script',
                    os.path.join(TOOLS, 'scripts', script + '.sfx'), '--out', out, '--lockstep', lib],
                   cwd=ROOT, env=env, capture_output=True)
    rep = os.path.join(out, 'lockstep.txt')
    return open(rep).read().splitlines() if os.path.exists(rep) else []


def function_map(probe_dir, src):
    """Each probe's function, as (file, name), from the probed tree's
    syntax (the function whose body holds the probe's offset)."""
    import cycprobe
    rows = list(csv.DictReader(open(os.path.join(probe_dir, 'probes.csv'))))
    by_file = collections.defaultdict(list)
    for r in rows:
        by_file[r['file']].append(r)
    fn = {}
    for f, rs in by_file.items():
        if '/include/' in f:
            continue
        path = os.path.join(src, os.path.relpath(f, 'src/stuntracefx'))
        tree = cycprobe.ast(path)
        text = open(path).read()
        bodies = [(n['name'], n['range']['begin'].get('offset'), n['range']['end'].get('offset'))
                  for n in tree.get('inner', [])
                  if n.get('kind') == 'FunctionDecl' and any(c.get('kind') == 'CompoundStmt' for c in n.get('inner', []))]
        starts = [0]
        for ln in text.split('\n'):
            starts.append(starts[-1] + len(ln) + 1)
        for r in rs:
            off = starts[int(r['line']) - 1] + int(r['col']) - 1
            for name, b, e in bodies:
                if b is not None and e is not None and b <= off <= e:
                    fn[int(r['id'])] = (f, name)
    return fn


def gather(args):
    import cycprobe
    os.makedirs(B, exist_ok=True)
    run(['rm', '-rf', G])
    os.makedirs(os.path.join(G, 'runs'))
    src = os.path.join(G, 'src')
    run([sys.executable, os.path.join(TOOLS, 'cycwrite.py'), '--strip', src, '--code'])
    env = dict(os.environ, CYC_SRC=src)
    lib = os.path.join(G, 'lib.dylib')
    run([sys.executable, os.path.join(TOOLS, 'cycprobe.py'), '--lib', os.path.join(G, 'probe'), lib], env=env)
    run(['cc', '-O2', '-o', os.path.join(B, 'gsuagg'), os.path.join(TOOLS, 'gsuagg.c')])
    # each probe's function, numbered; the copyable ones (cyccopy)
    os.environ['CYC_SRC'] = src
    cycprobe.SRC = src
    fn = function_map(os.path.join(G, 'probe'), src)
    names = sorted(set(fn.values()))
    ix = {n: i for i, n in enumerate(names)}
    nprobe = max(int(r['id']) for r in csv.DictReader(open(os.path.join(G, 'probe', 'probes.csv')))) + 1
    arr = [-1] * nprobe
    for p, k in fn.items():
        arr[p] = ix[k]
    open(os.path.join(G, 'fnmap.bin'), 'wb').write(struct.pack('<%di' % nprobe, *arr))
    open(os.path.join(G, 'fnnames.txt'), 'w').write('\n'.join('%s %s' % n for n in names))
    import cyccopy
    cyccopy.cycprobe.SRC = src
    copyable = {i for i, n in enumerate(names) if cyccopy.copyable(*n)}
    pickle.dump({p for p, f in enumerate(arr) if f in copyable}, open(os.path.join(G, 'copyable.pkl'), 'wb'))
    # the functions called from more than one place in the C: their probes
    # are qualified by call site, the same in every run
    calls = collections.Counter()
    for rel in cyccopy.all_files():
        for b, e, callee, nargs, fn_ in cyccopy.file_calls(rel):
            calls[(rel, callee) if cyccopy.is_static(rel, callee) else callee] += 1
    shared = bytes(1 if calls[(f, n) if cyccopy.is_static(f, n) else n] > 1 else 0 for f, n in names)
    open(os.path.join(G, 'shared.bin'), 'wb').write(shared)
    say('functions called from more than one place: %d' % sum(shared))
    say('probed')
    done = []
    for s in scripts(args):
        out = os.path.join(G, 'runs', s)
        log = os.path.join(out, 'code.log')
        t0 = time.time()
        lines = lockstep(s, lib, out, log)
        verdict = 'IDENTICAL' if lines and lines[-1].endswith('IDENTICAL') else 'not IDENTICAL'
        if os.path.exists(log):
            with open(os.path.join(out, 'agg.txt'), 'w') as fo:
                subprocess.run([os.path.join(B, 'gsuagg'), log, os.path.join(G, 'fnmap.bin'),
                                os.path.join(out, 'ctx.txt'), os.path.join(G, 'shared.bin')],
                               stdout=fo, stderr=subprocess.DEVNULL)
            os.remove(log)
            done.append(s)
        say('%s: %s, %.0f s' % (s, verdict, time.time() - t0))
    merge([(os.path.join(G, 'runs', s, 'agg.txt'), os.path.join(G, 'runs', s, 'ctx.txt')) for s in done],
          os.path.join(G, 'agg.txt'), os.path.join(G, 'ctx.txt'))


def merge(pairs, out_agg, out_ctx):
    """The runs' intervals as one, their contexts renumbered into one table."""
    gidx, rows = {}, collections.Counter()
    for agg, ctx in pairs:
        local = [None] + [l for l in open(ctx).read().split('\n') if l]
        remap = {0: 0}
        for li, s in enumerate(local[1:], 1):
            remap[li] = gidx.setdefault(s, len(gidx) + 1)
        for line in open(agg):
            c, pc, t, ids, plus, minus = line.rstrip('\n').split('\t')
            if ids:
                ids = ','.join(str(int(x) % CTX + CTX * remap[int(x) // CTX]) for x in ids.split(','))
            rows[(pc, t, ids, plus, minus)] += int(c)
    with open(out_agg, 'w') as f:
        for (pc, t, ids, plus, minus), c in rows.items():
            f.write('%d\t%s\t%s\t%s\t%s\t%s\n' % (c, pc, t, ids, plus, minus))
    with open(out_ctx, 'w') as f:
        for s, i in sorted(gidx.items(), key=lambda x: x[1]):
            f.write(s + '\n')
    say('merged %d runs: %d distinct intervals, %d contexts' % (len(pairs), len(rows), len(gidx)))


# ---- solve -------------------------------------------------------------------------

def expand(s):
    a = collections.Counter()
    for r in s.split(',') if s else []:
        x, n = r.split(':')
        x = int(x, 16)
        for k in range(int(n)):
            a[(x & 0xFF0000) | ((x + k) & 0xFFFF)] += 1
    return a


_S = {}


def _solve_addr(item):
    import numpy as np
    import scipy.sparse as sp
    from scipy.optimize import Bounds, LinearConstraint, milp
    rows, appear, weight = _S['rows'], _S['appear'], _S['weight']
    ad, ris = item
    has = set(ris)
    cand = sorted({q for i in ris for q in rows[i][0] if appear[q] <= has})
    if not cand:
        return ad, []
    ix = {q: j for j, q in enumerate(cand)}
    R = sorted(set().union(*(appear[q] for q in cand)))
    I, J, V, b = [], [], [], []
    for e, i in enumerate(R):
        for q, n in rows[i][0].items():
            if q in ix:
                I.append(e); J.append(ix[q]); V.append(n)
        b.append(rows[i][1].get(ad, 0))
    ne, nv = len(R), len(cand)
    A = sp.csr_matrix((V, (I, J)), shape=(ne, nv))
    full = sp.hstack([A, sp.identity(ne), -sp.identity(ne)]).tocsr()
    res = milp(np.concatenate([np.array([weight(q) for q in cand], float), np.full(2 * ne, 1000.0)]),
               constraints=LinearConstraint(full, b, b),
               integrality=np.concatenate([np.ones(nv), np.zeros(2 * ne)]), bounds=Bounds(0, np.inf))
    return ad, [(q, int(v)) for q, v in zip(cand, np.round(res.x[:nv]).astype(int)) if v]


def solve(args):
    import multiprocessing as mp
    info = {int(r['id']): r for r in csv.DictReader(open(os.path.join(G, 'probe', 'probes.csv')))}
    copyable = pickle.load(open(os.path.join(G, 'copyable.pkl'), 'rb'))
    lines = [l.rstrip('\n').split('\t') for l in open(os.path.join(G, 'agg.txt'))]
    by = collections.defaultdict(collections.Counter)
    tot = over = 0
    reached = set()
    for c, pc, t, ids, plus, minus in lines:
        c = int(c)
        tot += c
        p = collections.Counter(int(x) for x in ids.split(',')) if ids else collections.Counter()
        reached.update(p)
        if t != 'A':
            p[END | int(pc, 16)] += 1
        if minus:
            over += c
            continue
        ex = {a: n for a, n in expand(plus).items() if a < 0xE00000}    # CACHE runs: placed by address
        by[tuple(sorted(p.items()))][tuple(sorted(ex.items()))] += c
    conflicts = sum(sum(v.values()) for v in by.values() if len(v) > 1)
    rows = [(dict(k), dict(next(iter(v))), sum(v.values())) for k, v in by.items() if len(v) == 1]
    say('intervals %d: %.2f%% where the probes do not tell the original\'s paths apart, %.2f%% where the C states '
        'code the original did not fetch' % (tot, 100 * conflicts / tot, 100 * over / tot))

    def weight(q):
        if q & END:
            return 40
        if '/include/' in info[q % CTX]['file']:
            return 50
        if q >= CTX:
            # a helper's own probe in its context before its caller's (its
            # copies are written per context); one that cannot take copies after
            return 0.5 if (q % CTX) in copyable else 1.5
        return 1
    addr_rows, appear = collections.defaultdict(list), collections.defaultdict(set)
    for i, (p, a, c) in enumerate(rows):
        for ad in a:
            addr_rows[ad].append(i)
        for q in p:
            appear[q].add(i)
    _S.update(rows=rows, appear=appear, weight=weight)
    code = collections.defaultdict(dict)
    with mp.get_context('fork').Pool(max(1, mp.cpu_count() - 1)) as pool:
        for ad, got in pool.imap_unordered(_solve_addr, sorted(addr_rows.items(), key=lambda x: -len(x[1])), chunksize=8):
            for q, v in got:
                code[q][ad] = v
    ok = 0
    for p, a, c in rows:
        got = collections.Counter()
        for q, n in p.items():
            for ad, k in code.get(q, {}).items():
                got[ad] += n * k
        ok += c if got == collections.Counter(a) else 0
    say('solved: %.2f%% of the intervals met exactly; %d probes given code' % (100 * ok / tot, len(code)))
    pickle.dump(dict(code), open(os.path.join(G, 'code.csv.pkl'), 'wb'))
    pickle.dump(reached, open(os.path.join(G, 'code.csv.reached.pkl'), 'wb'))


# ---- write -------------------------------------------------------------------------

def caches():
    """The original's CACHE instructions: the code map's Super FX opcodes $02."""
    import codemap
    out = os.path.join(B, 'caches.txt')
    if not os.path.exists(out):
        mp_ = codemap.merged()
        rom = open(ROM, 'rb').read()
        with open(out, 'w') as f:
            for o in range(len(rom)):
                if mp_[o] & 16 and rom[o] == 0x02:
                    f.write('%06X\n' % codemap.run_addr(o))
    return out


def write(args):
    srcc = os.path.join(G, 'srcC')
    run(['rm', '-rf', srcc])
    run(['cp', '-R', os.path.join(G, 'src'), srcc])
    # the Super FX's own headers as they are now (the gather's tree has the
    # rest, its prototypes as its sources have them)
    for h in ('gsu.h', 'gsu_ops.h'):
        run(['cp', os.path.join(ROOT, 'src', 'stuntracefx', 'include', h), os.path.join(srcc, 'include')])
    env = dict(os.environ, CYC_SRC=srcc)
    print(run([sys.executable, os.path.join(TOOLS, 'cyccopy.py'), os.path.join(G, 'code.csv.pkl'),
               os.path.join(G, 'ctx.txt'), os.path.join(G, 'fnmap.bin'), os.path.join(G, 'fnnames.txt'),
               caches(), os.path.join(G, 'code.csv')], env=env).split('\n')[0])
    print(run([sys.executable, os.path.join(TOOLS, 'cycwrite.py'), '--code', os.path.join(G, 'code.csv')]
              + (['--dry'] if '--dry' in args else []), env=env).split('\n')[0])


# ---- check -------------------------------------------------------------------------

def check(args):
    d = os.path.join(B, 'check')
    run(['rm', '-rf', d])
    lib = os.path.join(d, 'lib.dylib')
    run([sys.executable, os.path.join(TOOLS, 'cycprobe.py'), '--lib', os.path.join(d, 'probe'), lib])
    on = off = 0
    for s in scripts(args):
        lines = lockstep(s, lib, os.path.join(d, s))
        t = next((l for l in lines if l.startswith('super fx timing:')), '')
        a = int(t.split()[3]) if t else 0
        b = int(t.split(', ')[1].split()[0]) if t else 0
        on, off = on + a, off + b
        print('%-28s %s %d of %d at the original\'s clocks (%.2f%%)' % (
            s, 'IDENTICAL' if lines and lines[-1].endswith('IDENTICAL') else 'NOT IDENTICAL', a, a + b,
            100 * a / max(1, a + b)), flush=True)
    print('all: %d of %d (%.2f%%)' % (on, on + off, 100 * on / max(1, on + off)))


def main(argv):
    cmds = {'gather': gather, 'solve': solve, 'write': write, 'check': check}
    if len(argv) < 2 or argv[1] not in cmds:
        print(__doc__)
        return 2
    cmds[argv[1]](argv[2:])
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
