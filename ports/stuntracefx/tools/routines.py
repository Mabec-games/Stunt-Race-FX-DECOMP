#!/usr/bin/env python3
"""routines.py: the port's table of the decomp's routines, by address.

    routines.py ROM OUT.c

Every `@routine BB:AAAA Name` tag in the port's game (ports/stuntracefx/game,
lifted from the decomp) names a routine and the
address the original enters it at; `@alias BB:AAAA` in its header is the
same code entered at another address (work RAM code the game also runs from
its ROM copy). The port's runtime looks a routine up by that address when
the C reaches code through an address it computes (call_routine and
jump_routine), when an interrupt's vector code calls a handler, and when
the S-CPU starts a Super FX program. S-CPU and Super FX routines share one
table: no address is both (hybrid.py checks the same). A routine whose C
has no abi_ adapter (one the C only calls by its typed name) is in the
table with none: the runtime names it if the game reaches it through an
address after all.

SPC700 routines (`@spc AAAA` in the header: the routine runs at APU RAM
AAAA) go in a table of their own, with the APU bytes each depends on
(`@spcuses`, else the code reachable from its entry) as runs of ROM bytes:
the race and battle drivers put different code at the same addresses, so
the runtime picks the routine whose bytes the APU RAM holds. This is the
same reading of the tags as tools/stuntracefx/hybrid.py's.
"""
import glob, os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
SRC = os.path.join(ROOT, os.environ.get('SRFX_GAME', 'ports/stuntracefx/game'))
sys.path.insert(0, os.path.join(ROOT, 'tools', 'stuntracefx'))
import spc700, spcmap  # noqa: E402

TAG = re.compile(r'@routine\s+([0-9A-Fa-f]{2}):([0-9A-Fa-f]{4})\s+(\w+)')
ALIAS = re.compile(r'@alias\s+([0-9A-Fa-f]{2}):([0-9A-Fa-f]{4})')
SPC = re.compile(r'@spc\s+([0-9A-Fa-f]{4})\b')
USES = re.compile(r'@spcuses\s+([0-9A-Fa-f,\s-]+)')


def walk(rom, m, entry):
    """The APU bytes reachable from entry in the code m maps (APU address
    -> ROM offset): fall-through, branches, JMP and CALL/PCALL, up to a
    RET, RETI, an indexed JMP or STOP. As hybrid.py's walk."""
    got, todo = set(), [entry]
    while todo:
        x = todo.pop()
        while x in m and x not in got:
            op = rom[m[x]]
            ln = spc700.length(op)
            if any(((x + k) & 0xFFFF) not in m for k in range(ln)):
                break
            for k in range(ln):
                got.add((x + k) & 0xFFFF)
            v = spc700.operands(op, bytes(rom[m[(x + k) & 0xFFFF]] for k in range(1, ln)), x)
            if op in (0x3F, 0x5F):
                todo.append(v['a'])
            if op == 0x4F:
                todo.append(0xFF00 | v['u'])
            if 'r' in v:
                todo.append(v['r'])
            if op in (0x6F, 0x7F, 0x5F, 0x1F, 0xFF, 0xEF, 0x0F):
                break
            x = (x + ln) & 0xFFFF
    out, lo, prev = [], None, None
    for x in sorted(got) + [None]:
        if lo is not None and (x is None or x != prev + 1):
            out.append((lo, prev + 1))
            lo = None
        if x is not None and lo is None:
            lo = x
        prev = x
    return out


def main(argv):
    if len(argv) != 3:
        print(__doc__)
        return 2
    rom = open(argv[1], 'rb').read()
    files = sorted(glob.glob(os.path.join(SRC, '**', '*.c'), recursive=True))
    rows, spc, uses, adapters, carried = [], {}, {}, set(), {}
    for f in files:
        text = open(f).read()
        adapters |= set(re.findall(r'^void abi_(\w+)\(void\)\s*\{', text, re.M))
        # the routine's carried form: what its adapter calls on the loaded
        # registers, or the X_cr beside a routine still in registers
        for m in re.finditer(r'^void abi_(\w+)\(void\)\n\{\n(.*?)^\}\n', text, re.M | re.S):
            c = re.match(r'\s*Carried r = \{cpu\.a, cpu\.x, cpu\.y, cpu\.d, cpu\.p, cpu\.db\};\s*(\w+)\(&r\);', m.group(2))
            if c:
                carried[m.group(1)] = c.group(1)
        for m in re.finditer(r'^void (\w+)_cr\(Carried \*cr\)\n\{', text, re.M):
            carried.setdefault(m.group(1), m.group(1) + '_cr')
        for m in TAG.finditer(text):
            addr = int(m.group(1), 16) << 16 | int(m.group(2), 16)
            name = m.group(3)
            head = text[m.end():text.find('*/', m.end())]
            sp = SPC.search(head)
            if sp:
                spc[addr] = (int(sp.group(1), 16), name)
                us = USES.search(head)
                if us:
                    uses[addr] = [tuple(int(x, 16) for x in r.split('-'))
                                  for r in re.split(r'[,\s]+', us.group(1).strip()) if '-' in r]
                continue
            rows.append((addr, name))
            for al in ALIAS.finditer(head):
                rows.append((int(al.group(1), 16) << 16 | int(al.group(2), 16), name))
    seen = {}
    for a, n in rows:
        if a in seen and seen[a] != n:
            print('routines: two routines at %06X: %s, %s' % (a, seen[a], n))
            return 1
        seen[a] = n
    lists = spcmap.lists(rom)

    def apu_rom(addr):
        o = spcmap.rom_offset(addr >> 16, addr & 0xFFFF)
        own = next((bl for bl, _ in lists.values() if any(off <= o < off + n for _, n, off in bl)), [])
        m = {}
        for bl in (lists.get('$19:C27C', ([], 0))[0], own):
            for d, n, off in bl:
                for k in range(n):
                    m[(d + k) & 0xFFFF] = off + k
        return m

    names = sorted({n for _, n in rows} | {n for _, n in spc.values()})
    out = ['/* Generated by ports/stuntracefx/tools/routines.py from the decomp\'s tags. */',
           '#include <stddef.h>', '#include <stdint.h>', '#include "routines.h"']
    out += ['void abi_%s(void);' % n for n in names if n in adapters]
    out.append('const PortRoutine port_routines[] = {')
    for a, n in sorted(set(rows)):
        out.append('    {0x%06X, %s, "%s"},' % (a, 'abi_' + n if n in adapters else 'NULL', n))
    out.append('};')
    out.append('const int port_routine_count = %d;' % len(set(rows)))
    crow = [(a, carried[n]) for a, n in sorted(set(rows)) if n in carried]
    out += ['void %s(struct Carried *cr);' % c for c in sorted({c for _, c in crow})]
    out.append('const PortCarried port_carried[] = {')
    out += ['    {0x%06X, %s},' % (a, c) for a, c in crow]
    out.append('};')
    out.append('const int port_carried_count = %d;' % len(crow))
    srows = sorted(spc.items(), key=lambda kv: (kv[1][0], kv[0]))
    for a, (apu, n) in srows:
        m = apu_rom(a)
        runs = []
        for lo, hi in uses.get(a) or walk(rom, m, apu):
            for x in range(lo, hi):
                if x not in m:
                    print('routines: @spcuses of %s: APU $%04X is in no uploaded block' % (n, x))
                    return 1
                if runs and runs[-1][0] + runs[-1][1] == x and runs[-1][2] + runs[-1][1] == m[x]:
                    runs[-1][1] += 1
                else:
                    runs.append([x, 1, m[x]])
        out.append('static const PortSpcUse uses_%s[] = {%s{0, 0, 0}};'
                   % (n, ''.join('{0x%04X, %d, 0x%06X}, ' % tuple(r) for r in runs)))
    out.append('const PortSpcRoutine port_spc_routines[] = {')
    for a, (apu, n) in srows:
        out.append('    {0x%04X, abi_%s, "%s", uses_%s},' % (apu, n, n, n))
    out.append('};')
    out.append('const int port_spc_routine_count = %d;' % len(srows))
    os.makedirs(os.path.dirname(os.path.abspath(argv[2])), exist_ok=True)
    with open(argv[2], 'w') as t:
        t.write('\n'.join(out) + '\n')
    print('routines: %d S-CPU and Super FX entries, %d SPC700 routines' % (len(set(rows)), len(srows)))
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
