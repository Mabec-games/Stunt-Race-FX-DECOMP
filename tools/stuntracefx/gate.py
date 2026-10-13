#!/usr/bin/env python3
"""gate.py: complete validation in one run.

    gate.py [--full]

Builds the hybrid once, then runs A5 (t3.py --live) and A7 (t3.py --image)
at the same time, so the machine's cores are shared by both instead of
each gate waiting on its own slowest script; then the image gate and the
progress refresh. Traces are not rerun: they depend only on the ROM, which
is byte-exact, and the scripts (build.py retraces a changed script).
"""
import os, subprocess, sys, threading, time

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, 'tools', 'stuntracefx'))
import t3  # noqa: E402


def main(argv):
    full = '--full' in argv
    t0 = time.time()
    t3.gate_lock()
    t3.build_gate_lib()
    res = {}
    th = [threading.Thread(target=lambda: res.__setitem__('A5', t3.live([], full))),
          threading.Thread(target=lambda: res.__setitem__('A7', t3.image(full)))]
    for t in th:
        t.start()
    for t in th:
        t.join()
    img = subprocess.run([sys.executable, os.path.join(ROOT, 'tools', 'stuntracefx', 'image.py')], cwd=ROOT).returncode
    subprocess.run([sys.executable, os.path.join(ROOT, 'tools', 'stuntracefx', 'progress.py')], cwd=ROOT)
    print('gate: A5 %s, A7 %s, image %s in %.0fs' % (res.get('A5'), res.get('A7'), img, time.time() - t0))
    return 1 if res.get('A5') or res.get('A7') or img else 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
