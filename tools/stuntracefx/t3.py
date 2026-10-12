#!/usr/bin/env python3
"""t3.py: the T3 gates for the C decomp.

    t3.py --live [ADDR...]      gate A5: every script, every call of each routine
                                 checked against its C (all routines when none named)

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


def live(only):
    run([sys.executable, os.path.join(ROOT, 'tools', 'stuntracefx', 'hybrid.py')])
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
    rev = subprocess.run(['git', 'rev-parse', '--short', 'HEAD'], capture_output=True, text=True, cwd=ROOT).stdout.strip()
    dirty = subprocess.run(['git', 'status', '--porcelain', '--', 'src/stuntracefx'], capture_output=True, text=True, cwd=ROOT).stdout.strip()
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


def main(argv):
    if argv[:1] == ['--live']:
        return live([a.upper().replace(':', '').replace('$', '') for a in argv[1:]])
    print(__doc__)
    return 2


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
