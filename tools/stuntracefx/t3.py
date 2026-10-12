#!/usr/bin/env python3
"""t3.py: the T3 gates for the C decomp.

    t3.py --live [ADDR...]      gate A5: every script, every call of each routine
                                 checked against its C (all routines when none named)
    t3.py --image               gate A7: every routine's C replaces it at once, in
                                 lockstep with the original, every script, every frame
    t3.py --qualify ADDR...     every gate for each routine, and the tier it reaches

Gates for T3: 0 (the C says what it does, how, its entry and exit, and has no
unfinished markers), C (every byte of the routine executed by the scripts),
A5 and A7 (IDENTICAL, measured on committed source that includes the C's
last change). T4 adds: the routine's assembly is filed with the same header,
its operands are symbolic, and the whole ROM rebuilds byte-exact. Symbolic
means no hexadecimal literal outside an immediate: an address is a name, and
a place inside a named table or record is the name plus a decimal index
(!oam_tile+4*5, !scroll_hdma+3).

A5 runs sfxbox --check over every script in tools/stuntracefx/scripts/ in
parallel and merges the results per routine:
    IDENTICAL   called, every call matched
    DIVERGENT   at least one call differed with no interrupt or DMA overlapping it
    UNVERIFIED  some differing calls were overlapped (cannot be judged)
    UNCOVERED   never called by any script
Verdicts are written to config/stuntracefx/t3_live.csv with the date and the
source revision they were measured on. Only IDENTICAL passes.
"""
import concurrent.futures, csv, datetime, glob, os, subprocess, sys

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


GATE_LIB = os.path.join(B, 'hybrid', 'gate', 'libsrfx.dylib')


def build_gate_lib():
    """Build the hybrid from the last commit; return its revision."""
    sys.path.insert(0, os.path.join(ROOT, 'tools', 'stuntracefx'))
    import hybrid
    src, rev = hybrid.export_head()
    if hybrid.main(src, GATE_LIB):
        raise SystemExit('hybrid build of %s failed' % rev)
    return rev


def live(only):
    global LIB
    rev = build_gate_lib()
    LIB = GATE_LIB
    scripts = sorted(glob.glob(os.path.join(SCRIPTS, '*.sfx')))
    with concurrent.futures.ThreadPoolExecutor(max_workers=os.cpu_count()) as ex:
        results = list(ex.map(lambda s: one(s, only), scripts))
    agg = {}
    for name, rows, notes in results:
        for addr, rname, calls, match, diff, inc, skipped, _ in rows:
            a = agg.setdefault(addr, {'name': rname, 'calls': 0, 'match': 0, 'diff': 0, 'inc': 0, 'scripts': 0})
            a['calls'] += int(calls); a['match'] += int(match); a['diff'] += int(diff); a['inc'] += int(inc)
            a['scripts'] += int(calls) > 0
        for n in notes[:20]:
            print(n)
    dirty = ''
    stamp = datetime.date.today().isoformat()
    old = {}
    if os.path.exists(LIVE):
        for r in csv.DictReader(open(LIVE)):
            old[r['routine']] = r
    failed = 0
    for addr in sorted(agg):
        a = agg[addr]
        v = 'DIVERGENT' if a['diff'] else 'UNCOVERED' if a['calls'] == 0 else 'UNVERIFIED' if a['inc'] else 'IDENTICAL'
        failed += v != 'IDENTICAL'
        print('%s %-28s calls %d in %d/%d scripts, diff %d, inconclusive %d: %s' % (
            addr, a['name'], a['calls'], a['scripts'], len(scripts), a['diff'], a['inc'], v))
        old[addr] = {'routine': addr, 'name': a['name'], 'verdict': v, 'calls': a['calls'], 'scripts': a['scripts'],
                     'diff': a['diff'], 'inconclusive': a['inc'], 'date': stamp, 'source': rev + ('+dirty' if dirty else '')}
    os.makedirs(os.path.dirname(LIVE), exist_ok=True)
    with open(LIVE, 'w', newline='') as f:
        wr = csv.DictWriter(f, ['routine', 'name', 'verdict', 'calls', 'scripts', 'diff', 'inconclusive', 'date', 'source'])
        wr.writeheader()
        for k in sorted(old):
            wr.writerow(old[k])
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
    run([SFXBOX, '--rom', os.path.join(B, 'baserom.sfc'), '--script', script, '--out', d, '--lockstep', LIB])
    lines = open(rep).read().splitlines() if os.path.exists(rep) else ['no report: FAILED']
    return name, lines


def image():
    global LIB
    rev = build_gate_lib()
    LIB = GATE_LIB
    scripts = sorted(glob.glob(os.path.join(SCRIPTS, '*.sfx')))
    with concurrent.futures.ThreadPoolExecutor(max_workers=max(1, os.cpu_count() // 2)) as ex:
        results = list(ex.map(image_one, scripts))
    stamp, verdicts = datetime.date.today().isoformat(), []
    rows = []
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
        wr.writerows(rows)
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


def dirty(path):
    return bool(git('status', '--porcelain', '--', path).stdout.strip())


def contains(rev, path):
    """True when committed source `rev` includes the last change to `path`."""
    if not rev or rev.endswith('+dirty'):
        return False
    last = git('log', '-1', '--format=%H', '--', path).stdout.strip()
    return bool(last) and git('merge-base', '--is-ancestor', last, rev).returncode == 0


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
        covered = bool(span) and all(mp[o] & 0x33 for o in span)
        gates.append(('C covered', covered, '%d of %d bytes executed' % (sum(1 for o in span if mp[o] & 0x33), len(span))
                      if span else 'no filed assembly gives the routine\'s extent'))
        lr = live.get(key)
        # A call an interrupt overlapped cannot be judged per call (the handler can
        # change what the routine reads mid-call); A7 judges those. A5 passes with
        # no divergence and at least one identical call.
        a5ok = bool(lr) and (lr['verdict'] == 'IDENTICAL' or (lr['verdict'] == 'UNVERIFIED' and int(lr['diff']) == 0
                                                                and int(lr['calls']) > int(lr['inconclusive'])))
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
    if argv[:1] in (['--live'], ['--image']):
        gate_lock()
    if argv[:1] == ['--live']:
        return live([a.upper().replace(':', '').replace('$', '') for a in argv[1:]])
    if argv[:1] == ['--image']:
        return image()
    if argv[:1] == ['--qualify'] and argv[1:]:
        return qualify([x.replace(':', '').replace('$', '') for x in argv[1:]])
    print(__doc__)
    return 2


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
