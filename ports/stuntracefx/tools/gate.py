#!/usr/bin/env python3
"""gate.py: the decomp's gate on the port's copy of the game.

    gate.py [--full]

The port's game (ports/stuntracefx/game at the last commit) built as the
verification harness's hybrid, then the decomp's two gates on it at once:
A5 (each routine's calls against the original's, every script and unit
call) and A7 (lockstep: the whole run's accesses against the original's,
in order and on time). While the game's C is turned into plain C, each
routine keeps its abi_ adapter as the harness's way in, so every routine
is judged on the paths the plays never reach as well. The records go to
build/stuntracefx/portgate, apart from the decomp's; a run measures again
only what changed since the last (--full: everything).
"""
import os, sys, threading, time

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
os.environ['SRFX_GATE_TREE'] = 'ports/stuntracefx/game'
os.environ['SRFX_GATE_OUT'] = 'portgate'
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
    print('portgate: A5 %s, A7 %s in %.0fs' % (res.get('A5'), res.get('A7'), time.time() - t0))
    return 1 if res.get('A5') or res.get('A7') else 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
