#!/usr/bin/env python3
"""same.py: the port against the reference port, file for file.

    same.py [SCRIPT...]

While the console's parts are replaced, each step that only moves code
behind a new interface must leave the port's run exactly as it was. The
reference is the port as of REF (built once, from that commit, into
build/stuntracefx/port/ref); the port is build/stuntracefx/port/null. Each
script (default: every play in tools/stuntracefx/plays, without its save
and load lines, a shot and a dump added at its end) runs on both from
power-on with --pacing; every shot, dump and the pacing record must be the
same, byte for byte. Exit 1 on any
difference.
"""
import concurrent.futures, filecmp, glob, os, shutil, subprocess, sys, tarfile, io

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
B = os.path.join(ROOT, 'build', 'stuntracefx', 'port')
REF = 'a3f26e3'
ROM = os.path.join(ROOT, 'reference', 'stuntracefx', 'Stunt Race FX (USA) (Rev 1).sfc')
REF_EXE = os.path.join(B, 'ref', 'stuntracefx')
PORT_EXE = os.path.join(B, 'null', 'stuntracefx')
WORK = os.path.join(B, 'same')


def build_ref():
    if os.path.exists(REF_EXE):
        return
    src = os.path.join(WORK, 'refsrc')
    shutil.rmtree(src, ignore_errors=True)
    os.makedirs(src)
    data = subprocess.run(['git', 'archive', REF], cwd=ROOT, capture_output=True, check=True).stdout
    tarfile.open(fileobj=io.BytesIO(data)).extractall(src)
    env = dict(os.environ, HOST='null', OUT=os.path.join('build', 'ref'), SRFX_ROM=ROM)
    subprocess.run(['sh', 'ports/stuntracefx/link.sh'], cwd=src, env=env, check=True, capture_output=True)
    os.makedirs(os.path.dirname(REF_EXE), exist_ok=True)
    shutil.copy(os.path.join(src, 'build', 'ref', 'stuntracefx'), REF_EXE)


def script_for(path):
    out = os.path.join(WORK, 'scripts', os.path.basename(path))
    os.makedirs(os.path.dirname(out), exist_ok=True)
    with open(path) as f, open(out, 'w') as g:
        for line in f:
            if line.split()[:1] not in (['save'], ['load']):
                g.write(line)
        g.write('\nshot same_end\ndump same_end\n')
    return out


def run(exe, script, out):
    shutil.rmtree(out, ignore_errors=True)
    os.makedirs(out)
    p = subprocess.run([exe, '--headless', '--script', script, '--out', out,
                        '--pacing', os.path.join(out, 'pacing.txt')],
                       capture_output=True, text=True, timeout=1200)
    if p.returncode:
        open(os.path.join(out, 'failed.txt'), 'w').write(p.stderr[-2000:])


def one(path):
    name = os.path.splitext(os.path.basename(path))[0]
    s = script_for(path)
    a, b = os.path.join(WORK, name, 'ref'), os.path.join(WORK, name, 'port')
    run(REF_EXE, s, a)
    run(PORT_EXE, s, b)
    fa, fb = sorted(os.listdir(a)), sorted(os.listdir(b))
    if fa != fb:
        return name, 'files differ: %s' % sorted(set(fa) ^ set(fb))
    bad = [f for f in fa if not filecmp.cmp(os.path.join(a, f), os.path.join(b, f), shallow=False)]
    return name, ('%d files differ: %s' % (len(bad), ' '.join(bad[:6]))) if bad else 'same (%d files)' % len(fa)


def main(argv):
    build_ref()
    scripts = argv[1:] or sorted(glob.glob(os.path.join(ROOT, 'tools', 'stuntracefx', 'plays', '*.sfx')))
    workers = max(1, (os.cpu_count() or 2) // 2)
    bad = 0
    with concurrent.futures.ThreadPoolExecutor(workers) as ex:
        for name, verdict in ex.map(one, scripts):
            print('%-26s %s' % (name, verdict), flush=True)
            bad += not verdict.startswith('same')
    print('%d of %d scripts differ' % (bad, len(scripts)))
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
