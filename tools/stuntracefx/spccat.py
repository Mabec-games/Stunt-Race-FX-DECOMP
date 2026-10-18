#!/usr/bin/env python3
"""spccat.py: the catalog of the SPC700's routines, from the traces.

    spccat.py       write build/stuntracefx/spc_routines.csv and summarize

A routine starts where the SPC700 was started ($0800, the start address of
every block list) and at the target of a CALL, PCALL or TCALL: one it ran,
an entry of a block when the same block holds the call too (its ROM byte at
the call's APU address is that opcode, a CALL's operand that target), so a
call the battle driver makes is not an entry of the race driver at the same
address; a call to an address outside its own block (the race driver's
main loop into the block at $10B0, a driver into the code under or above
it) is an entry of the block there, of the one that trace ran where blocks
of several lists hold it; or a CALL in code no trace ran, found by following the code from
the traced instructions through the untraced bytes they branch, call or
fall to (to a RET, RETI, JMP, BRA or STOP) and the handlers of their
JMP (!a+X) tables (spc700.jump_table). The handlers a JMP (!a+X) reaches
through a table run in the dispatching routine's frame and are part of it,
not routines of their own. Its row gives the APU
address, the uploaded block (spcmap.lists) and ROM offset of that byte, and
its extent: up to the last traced SPC700 code byte before the next entry
in the same block. Rows are by ROM offset, so the race and battle drivers'
versions of one address are separate routines. The S-CPU and Super FX
catalog (routines.csv, catalog.py) is separate: hybrid.py takes every row
there that is not Super FX as S-CPU code.
"""
import csv, glob, os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import codemap, spcmap  # noqa: E402

ROOT = codemap.ROOT
B = os.path.join(ROOT, 'build', 'stuntracefx')
OUT = os.path.join(B, 'spc_routines.csv')

CALLS = {0x3F: 'call', 0x4F: 'pcall'} | {k << 4 | 1: 'tcall' for k in range(16)}
ENDS = {0x6F, 0x7F, 0x5F, 0x2F, 0x1F, 0xFF, 0xEF, 0x0F}   # RET, RETI, JMP, BRA, JMP (!a+X), STOP, SLEEP, BRK


def static_calls(rom, mp, block, apu_of):
    """CALL targets (APU address) in a block's code no trace ran, reached by
    decoding from its traced instructions."""
    import spc700
    dst, n, o = block
    seen, todo, out = set(), [x for x in range(o, o + n) if mp[x] & spcmap.SPC_OP], set()
    while todo:
        x = todo.pop()
        while o <= x < o + n and x not in seen:
            seen.add(x)
            op = rom[x]
            ln = spc700.length(op)
            if x + ln > o + n:
                break
            v = spc700.operands(op, rom[x + 1:x + ln], apu_of[x])
            if op == 0x3F:
                out.add(v['a'])
                t = o + ((v['a'] - dst) & 0xFFFF)
                if t < o + n:
                    todo.append(t)
            if 'r' in v:
                t = x + ((v['r'] - apu_of[x] + 0x8000) & 0xFFFF) - 0x8000
                todo.append(t)
            if op == 0x1F:
                word = lambda a: rom[o + a - dst] | rom[o + a - dst + 1] << 8
                todo += [o + t - dst for t in spc700.jump_table(word, v['a'], dst, dst + n)]
            if op in ENDS:
                break
            x += ln
    return out


def build():
    rom = codemap._rom_bytes()
    ls = spcmap.lists(rom)
    blocks = [b for bl, _ in ls.values() for b in bl]
    apu = {}
    for dst, n, o in blocks:
        for k in range(n):
            apu[o + k] = (dst + k) & 0xFFFF
    mp = codemap.merged()
    # ROM offset -> entry kind: the start address in each block that has it,
    # and each call or dispatch target at the block version its trace ran.
    entries = {}
    for name, (bl, st) in ls.items():
        for dst, n, o in bl:
            if st is not None and dst <= st < dst + n and mp[o + st - dst] & spcmap.SPC_OP:
                entries[o + st - dst] = 'start'
    folds = {}
    def ran(d, x):
        # the SPC700 ran ROM byte x as an opcode in trace d
        if d not in folds:
            folds[d] = spcmap.fold(d, rom, blocks)[0]
        return folds[d].get(x, 0) & spcmap.SPC_OP
    for path in sorted(glob.glob(os.path.join(B, 'box', '*', 'spc_edges.csv'))):
        d = os.path.dirname(path)
        for r in csv.DictReader(open(path)):
            op = int(r['opcode'], 16)
            if op not in CALLS:
                continue
            frm, to = int(r['from'], 16), int(r['to'], 16)
            for bf in blocks:
                if not bf[0] <= frm < bf[0] + bf[1]:
                    continue
                of = bf[2] + frm - bf[0]
                if rom[of] != op or (op == 0x3F and (rom[of + 1] | rom[of + 2] << 8) != to):
                    continue
                if bf[0] <= to < bf[0] + bf[1]:
                    # the target is in the call's own block
                    ot = bf[2] + to - bf[0]
                    ok = mp[of] & spcmap.SPC_OP and mp[ot] & spcmap.SPC_OP
                    if ok:
                        entries.setdefault(ot, CALLS[op])
                    continue
                # a call into another block (the race driver's main loop
                # into the block at $10B0, a driver into the code under or
                # above it); where blocks of several lists hold the target,
                # the one this trace ran there
                bts = [bt for bt in blocks if bt[0] <= to < bt[0] + bt[1]]
                for bt in bts:
                    ot = bt[2] + to - bt[0]
                    ok = mp[of] & spcmap.SPC_OP and (ran(d, ot) if len(bts) > 1 else mp[ot] & spcmap.SPC_OP)
                    if ok:
                        entries.setdefault(ot, CALLS[op])
    for name, (bl, _) in ls.items():
        for b in bl:
            dst, n, o = b
            for a in static_calls(rom, mp, b, apu):
                if dst <= a < dst + n:
                    entries.setdefault(o + a - dst, 'call (static)')
    rows = []
    for name, (bl, _) in sorted(ls.items()):
        for dst, n, o in bl:
            here = sorted(x for x in entries if o <= x < o + n)
            for i, off in enumerate(here):
                lim = here[i + 1] if i + 1 < len(here) else o + n
                end = off
                for x in range(off, lim):
                    if mp[x] & codemap.SPC_CODE:
                        end = x + 1
                rows.append({'apu': '%04X' % apu[off], 'entry': entries[off], 'block': name, 'offset': '%06X' % off,
                             'end': '%06X' % end, 'bytes': sum(1 for x in range(off, end) if mp[x] & codemap.SPC_CODE)})
    seen, out = set(), []
    for r in sorted(rows, key=lambda r: r['offset']):
        if r['offset'] not in seen:
            seen.add(r['offset'])
            out.append(r)
    return out


def main(argv):
    rows = build()
    with open(OUT, 'w', newline='') as f:
        w = csv.DictWriter(f, fieldnames=['apu', 'entry', 'block', 'offset', 'end', 'bytes'])
        w.writeheader()
        w.writerows(rows)
    print('SPC700: %d routines, %d code bytes in them (%s)' % (len(rows), sum(r['bytes'] for r in rows),
                                                              os.path.relpath(OUT, ROOT)))
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
