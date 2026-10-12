#!/usr/bin/env python3
"""precommit.py -- the filing rules, checked on what a commit adds.

Every decompiled routine carries a header comment with three things:

    @routine <bank>:<addr> <Name>     where it lives in the original ROM
    WHAT IT DOES:                     its purpose, in game terms
    HOW:                              how it does it

In C the header is the comment block directly above the definition; in the
companion assembly it is the run of `;` comment lines directly above the
label. A routine is filed by responsibility (src/stuntracefx/<area>/,
asm/stuntracefx/<area>/), never in a file named for an address range. The
only address-named files allowed are the raw intake under asm/stuntracefx/raw/,
which the audit ratchets down to nothing.

The hook judges only staged files that the commit adds or modifies, so a
backlog never blocks an unrelated commit.
"""
import os, re, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
TAG = re.compile(r'@routine\s+([0-9A-Fa-f]{2}:[0-9A-Fa-f]{4})\b')
ADDR_NAME = re.compile(r'(^|[_/-])[0-9a-f]{2}[_:-]?[0-9a-f]{4}(\.|_|$)|^(slice|batch|bank|chunk)\w*\.(c|h|asm|gsu|spc|inc)$', re.I)
CODE_DIRS = ('src/stuntracefx/', 'asm/stuntracefx/')
RAW_DIR = 'asm/stuntracefx/raw/'


def git(*a):
    return subprocess.run(['git', '-C', ROOT] + list(a), capture_output=True).stdout


def header_block(lines, i, asm):
    """Collect the comment block that holds line i (the @routine line)."""
    if asm:
        lo = i
        while lo > 0 and lines[lo - 1].lstrip().startswith(';'):
            lo -= 1
        hi = i
        while hi + 1 < len(lines) and lines[hi + 1].lstrip().startswith(';'):
            hi += 1
        return '\n'.join(lines[lo:hi + 1])
    lo = i
    while lo > 0 and '/*' not in lines[lo]:
        lo -= 1
    hi = i
    while hi + 1 < len(lines) and '*/' not in lines[hi]:
        hi += 1
    return '\n'.join(lines[lo:hi + 1])


def main():
    bad = []
    staged = [p for p in git('diff', '--cached', '--name-only', '--diff-filter=ACMR', '-z').decode().split('\0') if p]
    added = set(p for p in git('diff', '--cached', '--name-only', '--diff-filter=A', '-z').decode().split('\0') if p)
    for rel in staged:
        if not rel.startswith(CODE_DIRS):
            continue
        base = os.path.basename(rel)
        if rel in added and not rel.startswith(RAW_DIR) and ADDR_NAME.search(base):
            bad.append('%s: address-named file outside %s; file by responsibility' % (rel, RAW_DIR))
        asm = rel.endswith(('.asm', '.gsu', '.spc', '.inc'))
        if not (asm or rel.endswith(('.c', '.h'))):
            continue
        text = git('show', ':' + rel).decode('utf-8', 'replace')
        lines = text.splitlines()
        seen = {}
        for i, line in enumerate(lines):
            m = TAG.search(line)
            if not m:
                continue
            where = m.group(1).upper()
            if where in seen and not rel.startswith(RAW_DIR):
                bad.append('%s:%d: @routine %s also tagged at line %d' % (rel, i + 1, where, seen[where]))
            seen[where] = i + 1
            if rel.startswith(RAW_DIR):
                continue
            block = header_block(lines, i, asm)
            for need in ('WHAT IT DOES:', 'HOW:'):
                if need not in block:
                    bad.append('%s:%d: @routine %s has no %s' % (rel, i + 1, where, need))
    for b in bad:
        print(b)
    if bad:
        print('precommit: %d filing finding(s)' % len(bad), file=sys.stderr)
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
