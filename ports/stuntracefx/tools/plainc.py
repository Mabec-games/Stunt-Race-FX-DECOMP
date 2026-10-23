#!/usr/bin/env python3
"""plainc.py: how much of the port's game still works in the 65816's
registers.

    plainc.py [FILE|DIR...]

The S-CPU's C becomes plain C one routine at a time (HANDOFF, plain C,
step 1): a routine's C takes its inputs as arguments and returns what its
callers read, and only its abi_ adapter (the verification harness's way in)
touches the register file (cpu.a, cpu.x, cpu.y, cpu.p, cpu.db, cpu.d,
cpu.e, cpu.pc, and cpu_ops.h's op_ helpers; cpu.s stays until the work RAM
step). An abi_X counts as an
adapter when it calls X (or X_cr, the carried form of a routine whose name
a typed function already has); one that does not is still the routine
itself, in registers. Per file: the references to the register file outside the
adapters, and the functions that make them (glue for callers still in
registers among them). Default: the whole game.
"""
import os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
GAME = os.path.join(ROOT, 'ports', 'stuntracefx', 'game')
REG = re.compile(r'\bcpu\.(a|x|y|p|db|d|e|pc)\b|\bop_\w+\s*\(')   # op_*: cpu_ops.h's register helpers
FUNC = re.compile(r'^[A-Za-z_][\w \*]*?\b(\w+)\s*\([^;]*$')


def functions(text):
    """(name, body) for each function defined at column 0."""
    lines = text.split('\n')
    i = 0
    while i < len(lines):
        m = FUNC.match(lines[i])
        if m and not lines[i].startswith(('static const', 'const', 'typedef', '#')):
            j = i
            while j < len(lines) and not lines[j].startswith('{') and not lines[j].rstrip().endswith(';'):
                j += 1
            if j < len(lines) and lines[j].startswith('{'):
                k = j + 1
                while k < len(lines) and not lines[k].startswith('}'):
                    k += 1
                yield m.group(1), '\n'.join(lines[j:k + 1])
                i = k + 1
                continue
        i += 1


def measure(path):
    text = re.sub(r'/\*.*?\*/', lambda m: '\n' * m.group(0).count('\n'), open(path).read(), flags=re.S)
    text = re.sub(r'//[^\n]*', '', text)
    refs, funcs = 0, []
    for name, body in functions(text):
        if name.startswith('abi_') and re.search(r'\b%s(?:_cr)?\s*\(' % name[4:], body):
            continue                            # an adapter: it calls the C function
        n = len(REG.findall(body))
        if n:
            refs += n
            funcs.append(name)
    return refs, funcs


def main(argv):
    paths = []
    for a in argv or [GAME]:
        if os.path.isdir(a):
            for d, _, fs in os.walk(a):
                paths += [os.path.join(d, f) for f in fs if f.endswith('.c')]
        else:
            paths.append(a)
    total = nfun = 0
    rows = []
    for p in sorted(paths):
        refs, funcs = measure(p)
        if refs:
            rows.append((refs, os.path.relpath(p, GAME), len(funcs)))
            total += refs
            nfun += len(funcs)
    for refs, p, n in sorted(rows, reverse=True):
        print('%6d %4d  %s' % (refs, n, p))
    print('%6d %4d  in %d files: register-file references, functions outside the adapters' % (total, nfun, len(rows)))
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
