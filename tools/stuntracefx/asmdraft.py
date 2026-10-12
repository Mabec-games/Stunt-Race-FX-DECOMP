#!/usr/bin/env python3
"""asmdraft.py: a starting point for filing routines in the companion assembly.

    asmdraft.py ADDR [ADDR...]      print a draft for each routine

The draft is the traced listing with the placement directive, operands
replaced by the names in hw.inc and wram.inc (exact addresses, or a name
plus a small offset inside the sprite table), calls to filed routines by
their labels, branch targets as local labels, and a header to fill in.
Names it could not find are listed at the end. It is intake: every draft is
read, named and commented by hand before it is filed.
"""
import csv, os, re, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import codemap  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
B = os.path.join(ROOT, 'build', 'stuntracefx')
INC = os.path.join(ROOT, 'asm', 'stuntracefx', 'include')
TAG = re.compile(r'@routine\s+([0-9A-Fa-f]{2}):([0-9A-Fa-f]{4})\s+(\w+)')


def defines():
    names = {}
    for f in ('hw.inc', 'wram.inc', 'rom.inc'):
        for line in open(os.path.join(INC, f)):
            m = re.match(r'!(\w+)\s*=\s*\$([0-9A-Fa-f]+)', line)
            if m:
                names.setdefault(int(m.group(2), 16), m.group(1))
    return names


def filed_labels():
    out = {}
    for dp, dn, fn in os.walk(os.path.join(ROOT, 'asm', 'stuntracefx')):
        for f in fn:
            if f.endswith('.asm'):
                for m in TAG.finditer(open(os.path.join(dp, f)).read()):
                    out[int(m.group(1), 16) << 16 | int(m.group(2), 16)] = m.group(3)
    return out


def listing():
    text = {}
    for f in sorted(os.listdir(os.path.join(B, 'raw'))):
        if not f.startswith('bank'):
            continue
        for l in open(os.path.join(B, 'raw', f)):
            m = re.search(r';\s*([0-9A-F]{6})( gsu)?$', l.rstrip())
            if m:
                text[int(m.group(1), 16)] = (l.split(';')[0].strip(), bool(m.group(2)))
    return text


def main(argv):
    rows = {r['addr']: r for r in csv.DictReader(open(os.path.join(B, 'routines.csv')))}
    names, labels, text = defines(), filed_labels(), listing()
    missing = set()
    for a in argv:
        a = a.upper().replace(':', '').replace('$', '')
        r = rows[a]
        lo, hi = int(r['offset'], 16), int(r['end'], 16)
        run = int(a, 16)
        place = ('%%ramcode($%06X, $%06X)' % (run, lo)) if (run >> 16) in (0x7E, 0x7F) else ('%%at($%06X)' % run)
        local = {}
        jumps = {}
        for o in range(lo, hi):
            pc0 = codemap.run_addr(o)
            t = text.get(pc0)
            if t:
                for m in re.finditer(r'\b(loc|gsu)_([0-9A-F]{6})\b', t[0]):
                    local[m.group(0)] = '.l%s' % m.group(2)[2:]
                m = re.match(r'jmp\.w \$([0-9A-F]{4})$', t[0])
                if m:
                    tgt = (pc0 & 0xFF0000) | int(m.group(1), 16)
                    to = codemap.rom_offset(tgt)
                    if to is not None and lo <= to < hi:
                        jumps[m.group(1)] = '.l%s' % m.group(1)
                        local['loc_%06X' % tgt] = '.l%s' % m.group(1)
        out = ['; @routine %s:%s Name' % (a[:2], a[2:]), '; WHAT IT DOES: ', '; HOW: ', '; Entry: . Exit: .', place,
               'Name:']
        for o in range(lo, hi):
            pc = codemap.run_addr(o)
            t = text.get(pc)
            if not t:
                continue
            ins, gsu = t
            lab = 'loc_%06X' % pc if not gsu else 'gsu_%06X' % pc
            if lab in local and pc != run:
                out.append(local[lab] + ':')
            for k, v in local.items():
                ins = re.sub(r'\b%s\b' % k, v, ins)
            jm = re.match(r'jmp\.w \$([0-9A-F]{4})$', ins)
            if jm and jm.group(1) in jumps:
                ins = 'jmp.w ' + jumps[jm.group(1)]
            m = re.match(r'(\w+)(\.[bwl])?\s+(.*)$', ins)
            if m and not gsu and m.group(1) not in ('rep', 'sep', 'brk', 'cop', 'wdm', 'mvn', 'mvp'):
                op = m.group(3)
                n = re.search(r'(?<!#)\$([0-9A-F]+)', op)
                if n:
                    v = int(n.group(1), 16)
                    if m.group(1) in ('jsr', 'jmp') and '(' not in op:
                        full = (pc & 0xFF0000) | v
                        rep = labels.get(full)
                    elif m.group(1) in ('jsl', 'jml') and '[' not in op:
                        rep = labels.get(v)
                    else:
                        key = v & 0xFFFF if (m.group(2) != '.l' or (v >> 16) in (0x00, 0x7E) and v < 0x7E2000) else v
                        rep = names.get(v) or names.get(key)
                        if rep:
                            rep = '!' + rep
                        elif 0x0375 <= key < 0x0595:
                            rep = '!oam_buffer+$%X' % (key - 0x0375)
                        else:
                            near = [(v - k2, n2) for k2, n2 in names.items() if 0 < v - k2 <= 0x40 and (k2 >> 16) == (v >> 16)]
                            if near:
                                d, n2 = min(near)
                                rep = '!%s+$%X' % (n2, d)
                    if rep:
                        op = op[:n.start()] + rep + op[n.end():]
                    else:
                        missing.add((m.group(1), n.group(0)))
                ins = '%s%s %s' % (m.group(1), m.group(2) or '', op)
            out.append('    ' + ins)
        print('\n'.join(out) + '\n')
    if missing:
        print('; unnamed: ' + ' '.join(sorted({x[1] for x in missing})))
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
