#!/usr/bin/env python3
"""image.py: the ship gate. The built ROM must equal the original, every byte.

    image.py            compare build/stuntracefx/snes/stuntracefx.sfc with the reference
Exit 0 only when the sizes match and 0 bytes differ; otherwise report the
differing bytes per 32 KB bank and exit 1. A missing build is a failure,
never a pass.
"""
import hashlib, os, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
REF = os.path.join(ROOT, 'build', 'stuntracefx', 'baserom.sfc')
OUT = os.path.join(ROOT, 'build', 'stuntracefx', 'snes', 'stuntracefx.sfc')


def main():
    if not os.path.exists(OUT) or not os.path.exists(REF):
        print('image: FAILED (no build; run tools/stuntracefx/build.py)'); return 1
    a, b = open(REF, 'rb').read(), open(OUT, 'rb').read()
    if len(a) != len(b):
        print('image: FAILED (size %d, expected %d)' % (len(b), len(a))); return 1
    banks = {}
    for i in range(len(a)):
        if a[i] != b[i]:
            banks[i >> 15] = banks.get(i >> 15, 0) + 1
    if banks:
        for k in sorted(banks):
            print('  bank %02X: %d bytes differ' % (k, banks[k]))
        print('image: FAILED (%d of %d bytes differ)' % (sum(banks.values()), len(a))); return 1
    print('image: IDENTICAL (%d of %d bytes, sha1 %s)' % (len(a), len(a), hashlib.sha1(b).hexdigest()))
    return 0


if __name__ == '__main__':
    sys.exit(main())
