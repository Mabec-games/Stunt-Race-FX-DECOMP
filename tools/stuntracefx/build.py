#!/usr/bin/env python3
"""build.py: build the companion ROM from source.

    build.py            trace (if needed), regenerate the raw intake, assemble
    build.py --retrace  rerun every trace script first

Steps: verify the reference ROM, stage it as build/stuntracefx/baserom.sfc,
run the trace scripts in tools/stuntracefx/scripts/ through sfxbox (in
parallel), generate the raw intake for every byte no filed source claims,
assemble with the pinned asar. Outputs are deleted before they are rebuilt,
so a failed step can never leave a stale ROM that grades as passing.
"""
import concurrent.futures, glob, hashlib, os, shutil, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
REF = os.path.join(ROOT, 'reference', 'stuntracefx', 'Stunt Race FX (USA) (Rev 1).sfc')
SHA1 = '781e376ff453a83c7bcf72339abe6b842efd9c86'
B = os.path.join(ROOT, 'build', 'stuntracefx')
BASEROM = os.path.join(B, 'baserom.sfc')
OUT = os.path.join(B, 'snes', 'stuntracefx.sfc')
ASAR = os.path.join(ROOT, 'tools', 'toolchains', 'asar', 'asar')
SFXBOX = os.path.join(B, 'box', 'sfxbox')
SCRIPTS = os.path.join(ROOT, 'tools', 'stuntracefx', 'scripts')


def run(cmd, **kw):
    p = subprocess.run(cmd, capture_output=True, text=True, **kw)
    if p.returncode:
        sys.stderr.write(p.stdout + p.stderr)
        raise SystemExit('failed: %s' % ' '.join(cmd))
    return p.stdout


def stage_baserom():
    data = open(REF, 'rb').read()
    if hashlib.sha1(data).hexdigest() != SHA1:
        raise SystemExit('reference ROM does not match SHA-1 %s' % SHA1)
    os.makedirs(B, exist_ok=True)
    if not os.path.exists(BASEROM) or open(BASEROM, 'rb').read() != data:
        shutil.copyfile(REF, BASEROM)


def trace_one(script, retrace):
    name = os.path.splitext(os.path.basename(script))[0]
    d = os.path.join(B, 'box', name)
    stamp = os.path.join(d, 'rommap.bin')
    if not retrace and os.path.exists(stamp) and os.path.getmtime(stamp) >= os.path.getmtime(script):
        return d
    shutil.rmtree(d, ignore_errors=True)
    os.makedirs(d)
    run([SFXBOX, '--rom', BASEROM, '--script', script, '--out', d, '--trace'], cwd=ROOT)
    return d


def main(argv):
    retrace = '--retrace' in argv
    stage_baserom()
    if not os.path.exists(SFXBOX):
        run(['make', '-C', os.path.join(ROOT, 'tools', 'stuntracefx', 'sfxbox')])
    scripts = sorted(glob.glob(os.path.join(SCRIPTS, '*.sfx')))
    with concurrent.futures.ThreadPoolExecutor(max_workers=os.cpu_count()) as ex:
        traces = list(ex.map(lambda s: trace_one(s, retrace), scripts))
    raw = os.path.join(B, 'raw')
    shutil.rmtree(raw, ignore_errors=True)
    args = [sys.executable, os.path.join(ROOT, 'tools', 'stuntracefx', 'disasm.py'), '--rom', BASEROM,
            '--out', raw, '--incbin', '../baserom.sfc']
    for t in traces:
        args += ['--trace', t]
    print(run(args).strip())
    if os.path.exists(OUT):
        os.remove(OUT)
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    run([ASAR, '--fix-checksum=off', os.path.join(raw, 'main.asm'), OUT])
    if not os.path.exists(OUT):
        raise SystemExit('assembler produced no ROM')
    print('built %s' % os.path.relpath(OUT, ROOT))
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
