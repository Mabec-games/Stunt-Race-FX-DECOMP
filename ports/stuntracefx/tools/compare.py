#!/usr/bin/env python3
"""compare.py: the port against the original, shot by shot.

    compare.py SCRIPT [OUTDIR]

Runs SCRIPT (sfxbox's form, from power-on: wait, hold, press, shot, dump,
repeat) through the original (build/stuntracefx/box/sfxbox, bsnes) and
through the headless port (build/stuntracefx/port/null/stuntracefx), then
compares each shot and dump. sfxbox writes 512 x 448 shots (bsnes doubles
the 256 x 224 picture): every second pixel of every second line is the
picture. Reports, for each shot, the pixels that differ (and the first),
and for each dump the bytes of work RAM and Super FX RAM that differ.
"""
import os, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
B = os.path.join(ROOT, 'build', 'stuntracefx')
SFXBOX = os.path.join(B, 'box', 'sfxbox')
PORT = os.path.join(B, 'port', 'null', 'stuntracefx')


def ppm(path):
    d = open(path, 'rb').read()
    parts, i = [], 0
    while len(parts) < 4:
        while d[i:i + 1].isspace():
            i += 1
        j = i
        while not d[j:j + 1].isspace():
            j += 1
        parts.append(d[i:j])
        i = j
    w, h = int(parts[1]), int(parts[2])
    return w, h, d[i + 1:i + 1 + w * h * 3]


def bsnes_level(c5):
    """bsnes's 8-bit output for a 5-bit level: expanded to 16 bits, gamma 1.5."""
    v = c5 << 3 | c5 >> 2
    v = v << 8 | v
    if v <= 32767:
        v = int(32767 * (v / 32767.0) ** 1.5)
    return v >> 8


CURVE = {((c << 3) | (c >> 2)): bsnes_level(c) for c in range(32)}


def port_picture(path):
    return [tuple(CURVE[c] for c in px) for px in picture(path)]


def picture(path):
    """256 x 224 RGB triples."""
    w, h, px = ppm(path)
    sx, sy = w // 256, h // 224
    return [tuple(px[((y * sy) * w + x * sx) * 3:((y * sy) * w + x * sx) * 3 + 3]) for y in range(224) for x in range(256)]


def run(cmd, timeout):
    try:
        p = subprocess.run(cmd, capture_output=True, text=True, timeout=timeout, cwd=ROOT)
        return p.returncode, p.stderr
    except subprocess.TimeoutExpired:
        return -1, 'timeout'


def main(argv):
    if len(argv) < 2:
        print(__doc__)
        return 2
    script = os.path.abspath(argv[1])
    name = os.path.splitext(os.path.basename(script))[0]
    out = os.path.abspath(argv[2]) if len(argv) > 2 else os.path.join(B, 'port', 'compare', name)
    o, p = os.path.join(out, 'original'), os.path.join(out, 'port')
    os.makedirs(o, exist_ok=True)
    os.makedirs(p, exist_ok=True)
    rc, err = run([SFXBOX, '--rom', os.path.join(B, 'baserom.sfc'), '--script', script, '--out', o], 600)
    if rc:
        print('original: failed (%s)' % err.strip()[-300:])
        return 1
    rc, err = run([PORT, '--headless', '--script', script, '--out', p], 600)
    if rc:
        print('port: failed (%s)' % err.strip()[-300:])
        return 1
    bad = 0
    for f in sorted(os.listdir(o)):
        if f.endswith('.ppm'):
            a, b = picture(os.path.join(o, f)), port_picture(os.path.join(p, f))
            diff = [i for i in range(len(a)) if a[i] != b[i]]
            first = '' if not diff else ' first at (%d, %d): original %s, port %s' % (
                diff[0] % 256, diff[0] // 256, a[diff[0]], b[diff[0]])
            print('%-24s %6d of %d pixels differ%s' % (f, len(diff), len(a), first))
            bad += bool(diff)
        elif f.endswith('.wram') or f.endswith('.gsuram'):
            a, b = open(os.path.join(o, f), 'rb').read(), open(os.path.join(p, f), 'rb').read()
            diff = [i for i in range(min(len(a), len(b))) if a[i] != b[i]]
            print('%-24s %6d of %d bytes differ%s' % (f, len(diff), len(a), (' first at $%05X' % diff[0]) if diff else ''))
            bad += bool(diff)
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
