#!/usr/bin/env python3
"""t3.py: the T3 gates for the C decomp.

    t3.py --live [ADDR...]      gate A5: every script, every call of each routine
                                 checked against its C (all routines when none named)
    t3.py --image               gate A7: every routine's C replaces it at once, in
                                 lockstep with the original, every script, every frame
    t3.py --qualify ADDR...     every gate for each routine, and the tier it reaches
    --full                      with --live or --image: run everything; by default
                                 only what changed since its last result runs

Gates for T3: 0 (the C says what it does, how, its entry and exit, and has
no unfinished markers), C (every byte of the routine executed by the
scripts, or, for a table inside it, read), A5 and A7 (IDENTICAL, measured on
committed source that includes the C's last change). T4 adds: the routine's
assembly is filed with the same header, its operands are symbolic, and the
whole ROM rebuilds byte-exact. Symbolic means no hexadecimal literal outside
an immediate: an address is a name, and a place inside a named table or
record is the name plus a decimal index (!oam_tile+4*5, !scroll_hdma+3).

A5 runs sfxbox --check over every script in tools/stuntracefx/scripts/ in
parallel and merges the results per routine:
    IDENTICAL   called, every call matched
    DIVERGENT   at least one call differed with no interrupt or DMA overlapping it
    UNVERIFIED  some differing calls were overlapped (cannot be judged)
    UNCOVERED   never called by any script
Verdicts are written to config/stuntracefx/t3_live.csv with the date and the
source revision they were measured on. Only IDENTICAL passes.
"""
import concurrent.futures, csv, datetime, functools, glob, hashlib, json, os, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
B = os.path.join(ROOT, 'build', 'stuntracefx')
SFXBOX = os.path.join(B, 'box', 'sfxbox')
LIB = os.path.join(B, 'hybrid', 'libsrfx.dylib')
SCRIPTS = os.path.join(ROOT, 'tools', 'stuntracefx', 'scripts')
LIVE = os.path.join(ROOT, 'config', 'stuntracefx', 't3_live.csv')
IMAGE = os.path.join(ROOT, 'config', 'stuntracefx', 'whole_image.csv')


def run(cmd):
    p = subprocess.run(cmd, capture_output=True, text=True, cwd=ROOT)
    if p.returncode:
        sys.stderr.write(p.stdout + p.stderr)
        raise SystemExit('failed: %s' % ' '.join(cmd))
    return p.stdout


def one(script, only):
    name = os.path.splitext(os.path.basename(script))[0]
    d = os.path.join(B, 'check', name)
    os.makedirs(d, exist_ok=True)
    rep = os.path.join(d, 'check.txt')
    if os.path.exists(rep):
        os.remove(rep)
    cmd = [SFXBOX, '--rom', os.path.join(B, 'baserom.sfc'), '--script', script, '--out', d, '--check', LIB]
    if only:
        cmd += ['--only', ','.join(only)]
    run(cmd)
    rows, notes = [], []
    for line in open(rep):
        if line.startswith(('DIFF', 'INCONCLUSIVE')):
            notes.append('%s: %s' % (name, line.rstrip()))
        elif not line.startswith('routine,') and line.count(',') == 7:
            rows.append(line.rstrip().split(','))
    return name, rows, notes


# Incremental gates. A script's lockstep result depends only on the C of the
# routines it reaches (every other routine's C is never called), the shared
# headers, the harness and the core patches, and the script itself; a
# routine's per-call result only on its own C and those shared paths. So a
# gate run reruns a script, or a routine, only when one of those changed
# since the revision its last passing result was measured on, and carries
# the other results forward to the new revision. --full reruns everything.
SHARED = ('src/stuntracefx/include/', 'tools/stuntracefx/sfxbox/', 'tools/toolchains/patches/',
          'tools/stuntracefx/hybrid.py')


@functools.lru_cache(maxsize=None)
def changed_since(rev):
    """Paths changed from committed source `rev` to HEAD; None when unknown."""
    if not rev or rev.endswith('+dirty'):
        return None
    r = git('diff', '--name-only', rev, 'HEAD')
    return None if r.returncode else frozenset(l for l in r.stdout.splitlines() if l)


def shared_changed(paths):
    return any(p.startswith(SHARED) or (p.startswith('src/stuntracefx/') and not p.endswith('.c'))
               for p in paths)


@functools.lru_cache(maxsize=None)
def tags_in(rev, path):
    """Routine addresses tagged in the C file `path` at `rev` (HEAD: the work tree)."""
    if rev == 'HEAD':
        full = os.path.join(ROOT, path)
        text = open(full).read() if os.path.exists(full) else ''
    else:
        r = git('show', '%s:%s' % (rev, path))
        text = r.stdout if r.returncode == 0 else ''
    return frozenset('%02X%s' % (int(m.group(1), 16), m.group(2).upper()) for m in TAG.finditer(text))


def changed_routines(rev, paths):
    """Every routine whose C was added, changed or removed between `rev` and HEAD."""
    out = set()
    for p in paths:
        if p.startswith('src/stuntracefx/') and p.endswith('.c'):
            out |= tags_in(rev, p) | tags_in('HEAD', p)
    return out


@functools.lru_cache(maxsize=None)
def reached():
    """Script name -> addresses of the catalogued routines its trace entered."""
    sys.path.insert(0, os.path.join(ROOT, 'tools', 'stuntracefx'))
    import codemap
    rows = list(csv.DictReader(open(os.path.join(B, 'routines.csv'))))
    out = {}
    for s in glob.glob(os.path.join(SCRIPTS, '*.sfx')):
        name = os.path.splitext(os.path.basename(s))[0]
        d = os.path.join(B, 'box', name)
        if not os.path.exists(os.path.join(d, 'rommap.bin')):
            continue
        mp = codemap.merged([d])
        out[name] = frozenset(r['addr'] for r in rows if int(r['offset'], 16) < len(mp) and mp[int(r['offset'], 16)] & 0x33)
    return out


def script_stale(name, rev, prev):
    """Why `name` must run again for HEAD, given its last row; '' when its result carries."""
    if not prev or prev['verdict'] not in ('IDENTICAL', 'UNCOVERED'):
        return 'no passing result'
    paths = changed_since(prev['source'])
    if paths is None:
        return 'no committed revision to compare'
    if shared_changed(paths):
        return 'shared code changed'
    if 'tools/stuntracefx/scripts/%s.sfx' % name in paths:
        return 'script changed'
    if name not in reached():
        return 'no trace'
    hit = reached()[name] & changed_routines(prev['source'], paths)
    return 'C changed for %s' % ' '.join(sorted(hit)[:4]) if hit else ''


GATE_LIB = os.path.join(B, 'hybrid', 'gate', 'libsrfx.dylib')


def build_gate_lib():
    """Build the hybrid from the last commit; return its revision."""
    sys.path.insert(0, os.path.join(ROOT, 'tools', 'stuntracefx'))
    import hybrid
    src, rev = hybrid.export_head()
    if hybrid.main(src, GATE_LIB):
        raise SystemExit('hybrid build of %s failed' % rev)
    return rev


def stale_routines(rev):
    """Routines whose per-call result must be measured again for HEAD."""
    have = {}
    for dp, dn, fn in os.walk(os.path.join(ROOT, 'src', 'stuntracefx')):
        for f in fn:
            if f.endswith('.c'):
                for a in tags_in('HEAD', os.path.relpath(os.path.join(dp, f), ROOT)):
                    have[a] = os.path.relpath(os.path.join(dp, f), ROOT)
    old = {r['routine']: r for r in csv.DictReader(open(LIVE))} if os.path.exists(LIVE) else {}
    out = []
    for a, path in sorted(have.items()):
        prev = old.get(a)
        paths = changed_since(prev['source']) if prev else None
        if paths is None or shared_changed(paths) or path in paths:
            out.append(a)
    return out


def _cache_path(name):
    return os.path.join(B, 'check', name, 'rows.json')


def _load_cache(name):
    try:
        return json.load(open(_cache_path(name)))
    except (OSError, ValueError):
        return None


def _script_sha(script):
    return hashlib.sha1(open(script, 'rb').read()).hexdigest()


def live(only, full=False):
    """A5. Each script's per-routine results are kept with the revision and
    the script they were measured on (build/stuntracefx/check/NAME/rows.json);
    a run measures again only what can have changed: a new or changed script,
    or shared code changed since, runs every routine; otherwise a script runs
    only the routines whose C changed (and that it reaches). The verdicts are
    then summed over every script's kept results."""
    global LIB
    rev = build_gate_lib()
    LIB = GATE_LIB
    scripts = sorted(glob.glob(os.path.join(SCRIPTS, '*.sfx')))
    have = set()
    for dp, dn, fn in os.walk(os.path.join(ROOT, 'src', 'stuntracefx')):
        for f in fn:
            if f.endswith('.c'):
                have |= tags_in('HEAD', os.path.relpath(os.path.join(dp, f), ROOT))
    stale = set(only) if only else set() if full else set(stale_routines(rev))
    rch = reached()
    jobs = []
    for sc in scripts:
        name = os.path.splitext(os.path.basename(sc))[0]
        c = _load_cache(name)
        why = ''
        if full or not c:
            why = 'all'
        elif c.get('script') != _script_sha(sc):
            why = 'all'
        else:
            paths = changed_since(c.get('rev'))
            if paths is None or shared_changed(paths):
                why = 'all'
        if why == 'all' and only:
            why = ''  # explicit routines: measure just those
        if why == 'all':
            jobs.append((sc, None))
            continue
        want = stale & (rch.get(name, have) | {a for a in stale if c and a not in c.get('rows', {})})
        if not want and only:
            want = stale & rch.get(name, have)
        if want:
            jobs.append((sc, sorted(want)))
    print('%d script run(s): %d whole, %d for changed routines' % (
        len(jobs), sum(1 for _, w in jobs if w is None), sum(1 for _, w in jobs if w is not None)))
    with concurrent.futures.ThreadPoolExecutor(max_workers=os.cpu_count()) as ex:
        results = list(ex.map(lambda j: (j,) + one(j[0], j[1]), jobs))
    for (sc, want), name, rows, notes in results:
        c = _load_cache(name) if want is not None else None
        kept = dict(c['rows']) if c else {}
        for addr, rname, calls, match, diff, inc, skipped, _ in rows:
            kept[addr] = [rname, int(calls), int(match), int(diff), int(inc), int(skipped)]
        os.makedirs(os.path.dirname(_cache_path(name)), exist_ok=True)
        json.dump({'rev': rev, 'script': _script_sha(sc), 'rows': kept}, open(_cache_path(name), 'w'))
        for n in notes[:20]:
            print(n)
    # Results that were not measured again hold for HEAD: carry them.
    agg = {}
    for sc in scripts:
        name = os.path.splitext(os.path.basename(sc))[0]
        c = _load_cache(name)
        if not c:
            continue
        if c['rev'] != rev:
            c['rev'] = rev
            json.dump(c, open(_cache_path(name), 'w'))
        for addr, (rname, calls, match, diff, inc, skipped) in c['rows'].items():
            if addr not in have:
                continue
            a = agg.setdefault(addr, {'name': rname, 'calls': 0, 'diff': 0, 'inc': 0, 'scripts': 0})
            a['calls'] += calls; a['diff'] += diff; a['inc'] += inc
            a['scripts'] += calls > 0
    stamp = datetime.date.today().isoformat()
    old = {r['routine']: r for r in csv.DictReader(open(LIVE))} if os.path.exists(LIVE) else {}
    out, failed = {}, 0
    for addr in sorted(agg):
        a = agg[addr]
        v = 'DIVERGENT' if a['diff'] else 'UNCOVERED' if a['calls'] == 0 else 'UNVERIFIED' if a['inc'] else 'IDENTICAL'
        failed += v != 'IDENTICAL'
        prev = old.get(addr)
        same = prev and prev['verdict'] == v and int(prev['calls']) == a['calls'] and int(prev['diff']) == a['diff']
        if not same or addr in stale:
            print('%s %-28s calls %d in %d/%d scripts, diff %d, inconclusive %d: %s' % (
                addr, a['name'], a['calls'], a['scripts'], len(scripts), a['diff'], a['inc'], v))
        out[addr] = {'routine': addr, 'name': a['name'], 'verdict': v, 'calls': a['calls'], 'scripts': a['scripts'],
                     'diff': a['diff'], 'inconclusive': a['inc'], 'date': stamp if not same else prev['date'],
                     'source': rev}
    os.makedirs(os.path.dirname(LIVE), exist_ok=True)
    with open(LIVE, 'w', newline='') as f:
        wr = csv.DictWriter(f, ['routine', 'name', 'verdict', 'calls', 'scripts', 'diff', 'inconclusive', 'date', 'source'])
        wr.writeheader()
        for k in sorted(out):
            wr.writerow(out[k])
    print('%d routines: %s' % (len(out), ', '.join('%d %s' % (n, v) for v, n in sorted(
        {v: sum(1 for r in out.values() if r['verdict'] == v) for v in set(r['verdict'] for r in out.values())}.items()))))
    return 1 if failed else 0


def source_rev():
    rev = subprocess.run(['git', 'rev-parse', '--short', 'HEAD'], capture_output=True, text=True, cwd=ROOT).stdout.strip()
    dirty = subprocess.run(['git', 'status', '--porcelain', '--', 'src/stuntracefx'], capture_output=True, text=True, cwd=ROOT).stdout.strip()
    return rev + ('+dirty' if dirty else '')


def image_one(script):
    name = os.path.splitext(os.path.basename(script))[0]
    d = os.path.join(B, 'image', name)
    os.makedirs(d, exist_ok=True)
    rep = os.path.join(d, 'lockstep.txt')
    if os.path.exists(rep):
        os.remove(rep)
    # A run that stops (a side stuck waiting) is that script's failure, not
    # the whole gate's: the other scripts' results still count.
    p = subprocess.run([SFXBOX, '--rom', os.path.join(B, 'baserom.sfc'), '--script', script, '--out', d, '--lockstep', LIB],
                       capture_output=True, text=True, cwd=ROOT)
    lines = open(rep).read().splitlines() if os.path.exists(rep) else []
    if p.returncode or not lines or ': ' not in lines[-1]:
        why = (p.stderr.strip().splitlines() or ['no report'])[-1]
        lines.append('stopped (%s): FAILED' % why)
    return name, lines


def image(full=False):
    global LIB
    rev = build_gate_lib()
    LIB = GATE_LIB
    scripts = sorted(glob.glob(os.path.join(SCRIPTS, '*.sfx')))
    old = {r['script']: r for r in csv.DictReader(open(IMAGE))} if os.path.exists(IMAGE) else {}
    stamp, verdicts = datetime.date.today().isoformat(), []
    rows, run_these = [], []
    for s in scripts:
        name = os.path.splitext(os.path.basename(s))[0]
        why = 'full run' if full else script_stale(name, rev, old.get(name))
        if why:
            run_these.append(s)
            print('%-22s runs: %s' % (name, why))
            continue
        r = dict(old[name])
        print('%-22s %s (carried from %s)' % (name, r['verdict'], r['source']))
        r['source'] = rev
        verdicts.append(r['verdict'])
        rows.append(r)
    with concurrent.futures.ThreadPoolExecutor(max_workers=max(1, os.cpu_count() // 2)) as ex:
        results = list(ex.map(image_one, run_these))
    for name, lines in results:
        last = lines[-1] if lines else 'FAILED'
        v = last.rsplit(': ', 1)[-1] if ': ' in last else 'FAILED'
        verdicts.append(v)
        for l in lines[:-1][:3]:
            print('%s: %s' % (name, l))
        print('%-22s %s' % (name, last))
        rows.append({'script': name, 'verdict': v, 'detail': last, 'date': stamp, 'source': rev})
    os.makedirs(os.path.dirname(IMAGE), exist_ok=True)
    with open(IMAGE, 'w', newline='') as f:
        wr = csv.DictWriter(f, ['script', 'verdict', 'detail', 'date', 'source'])
        wr.writeheader()
        wr.writerows(sorted(rows, key=lambda r: r['script']))
    ok = all(v in ('IDENTICAL', 'UNCOVERED') for v in verdicts) and 'IDENTICAL' in verdicts
    print('whole image: %s' % ('IDENTICAL' if ok else 'DIVERGENT'))
    return 0 if ok else 1


import re, struct

TAG = re.compile(r'@routine\s+([0-9A-Fa-f]{2}):([0-9A-Fa-f]{4})\s+(\w+)')
MARKERS = re.compile(r'\b(TODO|FIXME|XXX|HACK|guess|placeholder)\b|\?\?\?|#if 0', re.I)


def git(*a):
    return subprocess.run(['git'] + list(a), capture_output=True, text=True, cwd=ROOT)


def find_tagged(root, exts, addr):
    for dp, dn, fn in os.walk(root):
        for f in fn:
            if f.endswith(exts):
                path = os.path.join(dp, f)
                text = open(path).read()
                for m in TAG.finditer(text):
                    if (int(m.group(1), 16) << 16 | int(m.group(2), 16)) == addr:
                        return path, text, m
    return None, None, None


def header_of(text, m, asm):
    """The tag's line and the header comment lines that follow it."""
    lines = text[text.rfind('\n', 0, m.start()) + 1:].splitlines()
    out = [lines[0]]
    for l in lines[1:]:
        t = l.strip()
        if asm and not t.startswith(';'):
            break
        out.append(t)
        if not asm and '*/' in t:
            break
    return '\n'.join(out)


def rel(path):
    return os.path.relpath(os.path.join(ROOT, path), ROOT)


@functools.lru_cache(maxsize=None)
def dirty_paths():
    """Every path with uncommitted changes, from one git status."""
    out, paths = iter(git('status', '--porcelain', '-z', '--untracked-files=all').stdout.split('\0')), set()
    for e in out:
        if len(e) > 3:
            paths.add(e[3:])
            if e[0] in 'RC':  # a rename or copy names its source next
                paths.add(next(out, ''))
    return frozenset(paths)


def dirty(path):
    return rel(path) in dirty_paths()


@functools.lru_cache(maxsize=None)
def last_change(path):
    return git('log', '-1', '--format=%H', '--', path).stdout.strip()


@functools.lru_cache(maxsize=None)
def is_ancestor(a, b):
    return git('merge-base', '--is-ancestor', a, b).returncode == 0


def contains(rev, path):
    """True when committed source `rev` includes the last change to `path`."""
    if not rev or rev.endswith('+dirty'):
        return False
    last = last_change(rel(path))
    return bool(last) and is_ancestor(last, rev)


def merged_map():
    sys.path.insert(0, os.path.join(ROOT, 'tools', 'stuntracefx'))
    import codemap
    return codemap.merged()


def span_of(addr, asm_path):
    """ROM offsets of the routine: from its address through the filed bytes that follow."""
    sys.path.insert(0, os.path.join(ROOT, 'tools', 'stuntracefx'))
    import codemap
    start = codemap.rom_offset(addr)
    owned = set()
    for line in open(os.path.join(B, 'claimed.txt')):
        lo, hi = (int(x, 16) for x in line.split('-'))
        owned.update(range(lo, hi))
    others = set()
    for dp, dn, fn in os.walk(os.path.join(ROOT, 'asm', 'stuntracefx')):
        for f in fn:
            if f.endswith('.asm'):
                for m in TAG.finditer(open(os.path.join(dp, f)).read()):
                    a = int(m.group(1), 16) << 16 | int(m.group(2), 16)
                    others.add(codemap.rom_offset(a))
    span, o = [], start
    while o in owned and (o == start or o not in others):
        span.append(o); o += 1
    return span


def assess(addrs, image_ok=None):
    """Every gate for each routine: [(key, name, tier, [(gate, ok, why)])]."""
    live = {r['routine']: r for r in csv.DictReader(open(LIVE))} if os.path.exists(LIVE) else {}
    image_rows = list(csv.DictReader(open(IMAGE))) if os.path.exists(IMAGE) else []
    mp, out = None, []
    for a in addrs:
        addr = int(a, 16)
        key = '%06X' % addr
        cpath, ctext, cm = find_tagged(os.path.join(ROOT, 'src', 'stuntracefx'), ('.c',), addr)
        apath, atext, am = find_tagged(os.path.join(ROOT, 'asm', 'stuntracefx'), ('.asm',), addr)
        if not cpath:
            out.append((key, am.group(3) if am else '', 'T2' if apath else 'T1', [('C', False, 'no C')]))
            continue
        gates = []
        name = cm.group(3)
        h = header_of(ctext, cm, False)
        g0 = all(k in h for k in ('WHAT IT DOES:', 'HOW:', 'Entry:', 'Exit:')) and not MARKERS.search(ctext)
        gates.append(('0 complete', g0, os.path.relpath(cpath, ROOT)))
        span = span_of(addr, apath) if apath else []
        if mp is None:
            mp = merged_map()
        # Executed, or (a table inside the routine, such as a JMP (abs,X)
        # list) read as data: each entry counts once its case is taken.
        covered = bool(span) and all(mp[o] & 0x77 for o in span)
        gates.append(('C covered', covered, '%d of %d bytes executed or read' % (sum(1 for o in span if mp[o] & 0x77), len(span))
                      if span else 'no filed assembly gives the routine\'s extent'))
        lr = live.get(key)
        # A call an interrupt overlapped cannot be judged per call (the handler can
        # change what the routine reads mid-call); A7 judges those. A5 passes with
        # no divergence and at least one identical call.
        # A routine every call of which is inconclusive (the Super FX ran, an
        # interrupt overlapped it, or it reached code without C) has no call
        # A5 can judge; with no divergent call it is left to A7.
        a5ok = bool(lr) and (lr['verdict'] == 'IDENTICAL' or (lr['verdict'] == 'UNVERIFIED' and int(lr['diff']) == 0))
        g5 = a5ok and contains(lr['source'], cpath) and not dirty(cpath)
        gates.append(('A5 live', g5, ('%s on %s' % (lr['verdict'], lr['source']) if lr else 'never run')
                      + ('; the C has uncommitted changes' if dirty(cpath) else '')))
        vs = [r['verdict'] for r in image_rows]
        g7 = bool(image_rows) and all(v in ('IDENTICAL', 'UNCOVERED') for v in vs) and 'IDENTICAL' in vs \
            and all(contains(r['source'], cpath) for r in image_rows) and not dirty(cpath)
        gates.append(('A7 image', g7, '%d scripts, %s' % (len(vs), image_rows[0]['source'] if image_rows else 'never run')))
        t3 = all(g for _, g, _ in gates)
        t4 = False
        if apath:
            ah = header_of(atext, am, True)
            ga = all(k in ah for k in ('WHAT IT DOES:', 'HOW:', 'Entry:', 'Exit:')) and not MARKERS.search(atext)
            body = atext[am.end():]
            nxt = re.search(r'@routine\s', body)
            if nxt:  # this routine only, up to the next one's header
                body = body[:body.rfind('\n', 0, nxt.start()) + 1]
            literal = [l.strip() for l in body.splitlines()
                       if l.strip() and not l.strip().startswith((';', '%', '!', 'db ', 'dw ', 'dl '))
                       and not l.strip().endswith(':')
                       and re.search(r'(?<!#)\$[0-9A-Fa-f]', l.split(';')[0])]
            gs = not literal
            if image_ok is None:
                image_ok = subprocess.run([sys.executable, os.path.join(ROOT, 'tools', 'stuntracefx', 'image.py')],
                                          capture_output=True, text=True, cwd=ROOT).returncode == 0
            gates += [('T4 header', ga, os.path.relpath(apath, ROOT)),
                      ('T4 symbolic', gs, 'literal addresses: %s' % literal[:3] if literal else 'no literal addresses'),
                      ('T4 image', image_ok, 'whole ROM byte-exact' if image_ok else 'image gate fails')]
            t4 = t3 and ga and gs and image_ok
        out.append((key, name, 'T4' if t4 else 'T3' if t3 else 'T2', gates))
    return out


def qualify(addrs):
    worst = 0
    for key, name, tier, gates in assess(addrs):
        print('%s %s: %s' % (key, name, tier if tier in ('T3', 'T4') else 'below T3 (%s)' % tier))
        for g, ok, why in gates:
            print('  %-12s %s  %s' % (g, 'pass' if ok else 'FAIL', why))
        worst = max(worst, 0 if tier in ('T3', 'T4') else 1)
    return worst


def gate_lock():
    """One gate run at a time: two runs would share output files and the
    machine. A lock left by a run that died is taken over."""
    lock = os.path.join(B, 'gate.lock')
    if os.path.exists(lock):
        try:
            pid = int(open(lock).read().strip())
            os.kill(pid, 0)
            raise SystemExit('another gate run (pid %d) holds %s' % (pid, lock))
        except (ValueError, ProcessLookupError):
            pass
    open(lock, 'w').write(str(os.getpid()))
    import atexit
    atexit.register(lambda: os.path.exists(lock) and os.remove(lock))


def main(argv):
    if argv[:1] in (['--live'], ['--image']) or argv[1:2] in (['--live'], ['--image']):
        gate_lock()
    full = '--full' in argv
    argv = [a for a in argv if a != '--full']
    if argv[:1] == ['--live']:
        return live([a.upper().replace(':', '').replace('$', '') for a in argv[1:]], full)
    if argv[:1] == ['--image']:
        return image(full)
    if argv[:1] == ['--qualify'] and argv[1:]:
        return qualify([x.replace(':', '').replace('$', '') for x in argv[1:]])
    print(__doc__)
    return 2


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
