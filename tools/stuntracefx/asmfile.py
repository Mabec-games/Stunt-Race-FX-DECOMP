#!/usr/bin/env python3
"""asmfile.py: write filed assembly from drafts and the C's headers.

    asmfile.py FILE.asm ADDR [ADDR...]     append the routines to FILE.asm

Each routine's draft comes from asmdraft.py; its header (name, WHAT IT DOES,
HOW, Entry/Exit) is taken from the routine's C, so it is written once. The
file is created with its includes if it does not exist. Review the result:
drafts are intake.
"""
import os, re, subprocess, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import asmdraft  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
TAG = re.compile(r'@routine\s+([0-9A-Fa-f]{2}):([0-9A-Fa-f]{4})\s+(\w+)')


def c_header(addr):
    for dp, dn, fn in os.walk(os.path.join(ROOT, 'src', 'stuntracefx')):
        for f in fn:
            if not f.endswith('.c'):
                continue
            text = open(os.path.join(dp, f)).read()
            for m in TAG.finditer(text):
                if (int(m.group(1), 16) << 16 | int(m.group(2), 16)) == addr:
                    start = text.rfind('/*', 0, m.start())
                    end = text.find('*/', m.start())
                    lines = [re.sub(r'^\s*\*\s?', '', l) for l in text[start + 2:end].splitlines()]
                    return m.group(3), [l for l in lines if l.strip()], os.path.relpath(os.path.join(dp, f), ROOT)
    raise SystemExit('no C for %06X' % addr)


def main(argv):
    path, addrs = argv[0], argv[1:]
    draft = subprocess.run([sys.executable, os.path.join(ROOT, 'tools', 'stuntracefx', 'asmdraft.py')] + addrs,
                           capture_output=True, text=True).stdout
    blocks = [b for b in draft.split('\n\n') if b.startswith('; @routine')]
    out = []
    cfile = None
    for addr, block in zip(addrs, blocks):
        name, header, cfile = c_header(int(addr.replace(':', ''), 16))
        body = block.splitlines()
        place = next(l for l in body if l.startswith('%'))
        code = body[body.index('Name:') + 1:]
        code = [re.sub(r'\$0000,y', '0,y', l) for l in code]
        out.append('\n'.join(['; ' + l for l in header] + [place, name + ':'] + code))
    new = not os.path.exists(path)
    with open(path, 'a') as f:
        if new:
            area = os.path.splitext(os.path.basename(path))[0]
            f.write('; %s.asm: see the C, %s, for what this file covers.\n\n' % (area, cfile))
            f.write('incsrc "../include/macros.inc"\nincsrc "../include/hw.inc"\nincsrc "../include/wram.inc"\n\narch 65816\n')
        f.write('\n' + '\n\n'.join(out) + '\n')
    print('%s: %d routine(s)' % (path, len(out)))
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
