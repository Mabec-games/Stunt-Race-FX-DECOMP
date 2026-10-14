#!/usr/bin/env python3
"""census.py: how many routines the game has, traced or not.

The catalog (catalog.py) knows the routines some script has run. This finds
the code no script has run by following the code statically from what is
known: every S-CPU instruction is decoded from its routine's start with the
register widths the traces saw (REP/SEP tracked where they did not), and
these lead to more routines:

  calls       JSR, JSL (abs, long) and their targets' own calls in turn
  jumps       JMP, JML, BRL, branches: the targets become part of the code
  tables      JMP/JSR (abs,X): words from the instruction's table address
              while the traces read them or each points at code that
              decodes, the table not running into code or doubling back
  pointers    16-bit immediates (LDA/LDX/LDY/PEA) the next instruction
              stores into a field some other instruction fills with a known
              routine's address: handlers kept in data (objects, actors)
  Super FX    programs the S-CPU starts with LDX #address, LDA #bank, JSL
              GsuRun; inside them IWT R15 (jumps), LINK + IWT R15 (calls)
              and the branches
  orphans     untouched gaps that follow the end of code (RTS, RTL, RTI, a
              jump), decode cleanly and call or jump to known code: routines
              nothing found refers to; walked in turn until none are left

Code the traces ran is decoded with the widths they saw, and its targets are
certain. Code decoded statically counts only block by block: a block that
runs into data the traces read, a BRK, COP, WDM or STP, the middle of a
traced instruction, or a jump or call to nowhere is thrown out with its
targets.

An entry point is a call target, a table entry, a handler pointer, a
vector, a Super FX start or call target, an orphan, or anything the catalog
or a source tag already names. Branch and jump targets inside code already
reached are part of it. docs/stuntracefx/ROUTINE_CENSUS.md turns the entry
points into a routine count.

    census.py               the counts and what is new
    census.py --recall      the same walk from the vectors alone: how many of
                            the catalog's routines it finds (its blind spots)
"""
import csv, os, re, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import codemap, disasm  # noqa: E402

ROOT = codemap.ROOT
B = codemap.B
M8, M16, X8, X16 = disasm.M8, disasm.M16, disasm.X8, disasm.X16
GSU_RUN = 0x7EE1F5
STOPS = {0x60, 0x6B, 0x40, 0xDB}           # RTS RTL RTI STP
SUSPECT = {0x00, 0x02, 0x42, 0xDB}         # BRK COP WDM STP: data, not code
STORES = {0x85, 0x95, 0x8D, 0x9D, 0x99, 0x8F, 0x9F, 0x86, 0x8E, 0x84, 0x8C, 0x96, 0x94, 0x91, 0x92, 0x97, 0x87}


class Census:
    def __init__(self):
        self.rom = open(os.path.join(B, 'baserom.sfc'), 'rb').read()
        self.mp = codemap.merged()
        self.code = bytearray(len(self.rom))     # 1: S-CPU code decoded here
        self.gcode = bytearray(len(self.rom))    # 1: Super FX code decoded here
        self.entries = {}                        # S-CPU routine start -> how found
        self.gentries = {}                       # Super FX routine start -> how found
        self.bad = 0
        self.stored = {}

    def flags_at(self, o, f):
        v = self.mp[o] & 0xF00
        return v if v else f

    def off(self, addr):
        try:
            return codemap.rom_offset(addr)
        except Exception:
            return None

    def plausible(self, addr):
        """Code can start here: ROM (or a RAM copy of it) whose first bytes
        decode without the opcodes data is made of."""
        o = self.off(addr)
        if o is None or o >= len(self.rom):
            return False
        if self.mp[o] & 0x33:
            return True
        if self.mp[o] & 0x44:
            return False                          # read as data
        f = M8 | X8
        for _ in range(3):
            op = self.rom[o]
            if op in SUSPECT:
                return False
            o += disasm.cpu_len(op, f)
        return True

    def walk(self, seeds):
        work = list(seeds)
        while work:
            addr, f = work.pop()
            self.run(addr, f, work)

    def block(self, pc, f):
        """Decode from pc to the first end. Returns (instructions, how it
        ended): each instruction (pc, offset, length, widths); ended 'end'
        (RTS RTL RTI JMP JML BRL BRA or indirect jump), 'joins' (runs into
        code already decoded or traced), or 'bad' (data, a suspect opcode,
        the middle of a traced instruction, off the bank)."""
        out = []
        o = self.off(pc)
        if o is None:
            return out, 'bad'
        for _ in range(400):
            if o >= len(self.rom) - 4 or (o & 0x7FFF) == 0 and out:
                return out, 'bad'
            m = self.mp[o]
            if out and (self.code[o] == 2 or m & 2 and not m & 1):
                return out, 'bad'
            if out and self.code[o]:
                return out, 'joins'
            if not m & 1:
                if m & 2 or m & 0x44 or self.code[o] == 2 or self.gcode[o]:
                    return out, 'bad'
            op = self.rom[o]
            f = self.flags_at(o, f) if m & 1 else f
            if op in SUSPECT and not m & 1:
                return out, 'bad'
            n = disasm.cpu_len(op, f)
            if not m & 1 and any(self.mp[o + k] & 0x47 or self.code[o + k] for k in range(1, n)):
                return out, 'bad'
            out.append((pc, o, n, f))
            a1 = self.rom[o + 1]
            if not m & 1:
                t = self.static_target(pc, o, op, f)
                if t is not None and not self.sane(t):
                    return out, 'bad'
            if op == 0xC2:
                if a1 & 0x20: f = f & ~M8 | M16
                if a1 & 0x10: f = f & ~X8 | X16
            elif op == 0xE2:
                if a1 & 0x20: f = f & ~M16 | M8
                if a1 & 0x10: f = f & ~X16 | X8
            if op in STOPS or op in (0x4C, 0x5C, 0x82, 0x80, 0x7C, 0x6C, 0xDC):
                return out, 'end'
            o += n
            pc = (pc & 0xFF0000) | ((pc + n) & 0xFFFF)
        return out, 'bad'

    def static_target(self, pc, o, op, f):
        w = self.rom[o + 1] | self.rom[o + 2] << 8
        bank = pc & 0xFF0000
        if op in (0x20, 0x4C):
            return bank | w
        if op in (0x22, 0x5C):
            return w | self.rom[o + 3] << 16
        if op == 0x82:
            return bank | ((pc + 3 + (w - 0x10000 if w & 0x8000 else w)) & 0xFFFF)
        if disasm.OPS[op][1] == 'rel':
            a1 = self.rom[o + 1]
            return bank | ((pc + 2 + (a1 - 0x100 if a1 & 0x80 else a1)) & 0xFFFF)
        return None

    def sane(self, t):
        """A jump or call target: ROM or a RAM copy of it, not data the
        traces read, not inside a traced instruction."""
        o = self.off(t)
        if o is None or o >= len(self.rom):
            return False
        m = self.mp[o]
        if m & 1:
            return True
        return not (m & 0x76) and self.code[o] != 2

    def run(self, addr, f, work):
        """Decode a routine block by block. Instructions the traces ran are
        certain and so are their targets; a block decoded statically counts
        (its code and its targets) only when it ends cleanly."""
        todo = [(addr, f)]
        while todo:
            pc, f = todo.pop()
            o = self.off(pc)
            if o is None or self.code[o] or (self.mp[o] & 1 and self.code[o]):
                continue
            ins, how = self.block(pc, f)
            ok = how != 'bad'
            if not ok:
                self.bad += 1
            for pc, o, n, f in ins:
                traced = bool(self.mp[o] & 1)
                if not (ok or traced):
                    continue
                for k in range(n):
                    self.code[o + k] = 1 if k == 0 else 2
                self.targets(pc, o, n, f, traced, work, todo)

    def targets(self, pc, o, n, f, traced, work, todo):
        op = self.rom[o]
        a1 = self.rom[o + 1]
        w = a1 | self.rom[o + 2] << 8
        bank = pc & 0xFF0000
        mode = disasm.OPS[op][1]
        how = 'traced' if traced else 'static'
        if op == 0x20:
            self.add(bank | w, 'call', how, f, work)
        elif op == 0x22:
            t = w | self.rom[o + 3] << 16
            self.add(t, 'call', how, f, work)
            if t == GSU_RUN:
                self.gsu_start(o)
        elif op in (0x4C, 0x5C, 0x82):
            if op == 0x4C:
                t = bank | w
            elif op == 0x5C:
                t = w | self.rom[o + 3] << 16
            else:
                t = bank | ((pc + 3 + (w - 0x10000 if w & 0x8000 else w)) & 0xFFFF)
            self.jump(t, f, todo)
        elif op in (0x7C, 0xFC):
            self.table(bank, w, f, how, work)
        elif mode == 'rel':
            self.jump(bank | ((pc + 2 + (a1 - 0x100 if a1 & 0x80 else a1)) & 0xFFFF), f, todo)
        elif (mode in ('immm', 'immx') and n == 3) or op == 0xF4:
            self.pointer(bank, w, o + n, how, work)

    def add(self, addr, kind, how, f, work):
        if addr not in self.entries:
            self.entries[addr] = (kind, how)
            work.append((addr, f))

    def jump(self, t, f, todo):
        o = self.off(t)
        if o is not None and not self.code[o]:
            todo.append((t, f))

    def clean(self, t, f):
        """A start that decodes to an end, under some register widths."""
        o = self.off(t)
        if o is None or o >= len(self.rom):
            return None
        if self.mp[o] & 1:
            return self.flags_at(o, f)
        if self.mp[o] & 0x76 or self.code[o] == 2:
            return None
        for g in (f, M8 | X8, M16 | X16, M8 | X16, M16 | X8):
            ins, how = self.block(t, g)
            if how != 'bad' and ins:
                return g
        return None

    def table(self, bank, at, f, how, work):
        o = self.off(bank | at)
        if o is None:
            return
        seen = []
        for i in range(64):
            p = o + 2 * i
            if p + 1 >= len(self.rom) or self.mp[p] & 0x33 or self.code[p]:
                break
            t = bank | self.rom[p] | self.rom[p + 1] << 8
            if (t & 0xFFFF) < 0x8000 and bank not in (0x7E0000, 0x7F0000):
                break
            if any(abs(t - s) < 2 for s in seen):
                break
            read = bool(self.mp[p] & 4)
            g = self.clean(t, f)
            if g is None and not read:
                break
            seen.append(t)
            self.add(t, 'table', 'traced' if read else how, g if g is not None else f, work)

    def pointer(self, bank, w, nxt, how, work):
        """An immediate stored by the next instruction: noted with the field
        it goes to. fields() decides which fields hold code addresses."""
        if w < 0x8000 or nxt + 3 >= len(self.rom) or self.rom[nxt] not in STORES:
            return
        st = self.rom[nxt]
        field = (st, self.rom[nxt + 1] if disasm.cpu_len(st, M8 | X8) == 2 else
                 self.rom[nxt + 1] | self.rom[nxt + 2] << 8)
        self.stored.setdefault(field, set()).add((bank | w, how))

    def fields(self, known, work):
        """Fields some instruction fills with a known routine or a traced
        instruction start are code-pointer fields; the other addresses stored
        there are handlers, if they decode."""
        added = 0
        for field, vals in self.stored.items():
            codey = [t for t, _ in vals if t in known or (self.off(t) is not None and self.mp[self.off(t)] & 1)]
            if not codey:
                continue
            for t, how in vals:
                o = self.off(t)
                if o is None or t in self.entries or self.mp[o] & 0x77 or self.code[o]:
                    continue
                g = self.clean(t, M8 | X8)
                if g is not None:
                    self.add(t, 'pointer', how, g, work)
                    added += 1
        return added

    def gsu_start(self, o):
        """LDX #address and LDA #bank in the few instructions before a JSL
        GsuRun: a Super FX program start."""
        x = b = None
        for back in range(3, 16):
            p = o - back
            if p < 0:
                break
            if self.rom[p] == 0xA2 and x is None:
                x = self.rom[p + 1] | self.rom[p + 2] << 8
            if self.rom[p] == 0xA9 and b is None:
                b = self.rom[p + 1]
        if x is not None and b is not None and x >= 0x8000:
            self.gentries.setdefault(b << 16 | x, 'started')

    def gsu_walk(self):
        work = list(self.gentries)
        while work:
            t = work.pop()
            o = self.off(t)
            while o is not None and o < len(self.rom) - 3 and not self.gcode[o]:
                op = self.rom[o]
                n = disasm.gsu_len(op)
                for k in range(n):
                    self.gcode[o + k] = 1
                pc = codemap.run_addr(o) if (t >> 16) < 0x40 else t
                if 0x05 <= op <= 0x0F:            # branches
                    d = self.rom[o + 1]
                    tgt = ((codemap.run_addr(o) + 2 + (d - 0x100 if d & 0x80 else d)) & 0xFFFF) | (t & 0xFF0000)
                    to = self.off(tgt)
                    if to is not None and not self.gcode[to]:
                        work.append(tgt)
                    if op == 0x05:                # BRA
                        break
                elif op == 0xFF:                  # IWT R15: a jump, a call after LINK
                    tgt = (self.rom[o + 1] | self.rom[o + 2] << 8) | (t & 0xFF0000)
                    called = o > 0 and 0x91 <= self.rom[o - 1] <= 0x94
                    if called:
                        if tgt not in self.gentries:
                            self.gentries[tgt] = 'call'
                            work.append(tgt)
                    elif self.off(tgt) is not None and not self.gcode[self.off(tgt)]:
                        work.append(tgt)
                    if not called:
                        o += n
                        o += 1                    # the delay slot
                        break
                elif op == 0x00:                  # STOP
                    break
                o += n


def orphans(c, known):
    """Untouched, unreached gaps in code banks that look like routines no
    reference reaches: the gap follows an end (RTS RTL RTI JMP JML BRA BRL)
    of code, decodes cleanly to an end under some widths, and calls or jumps
    to a known routine or traced code. Counted per gap, first routine only."""
    found = []
    rom, mp = c.rom, c.mp
    n = len(rom)
    o = 0
    while o < n - 4:
        if mp[o] & 0x77 or c.code[o] or c.gcode[o]:
            o += 1
            continue
        g = o
        while g < n and not (mp[g] & 0x77 or c.code[g] or c.gcode[g]):
            g += 1
        start, o = o, g
        if g - start < 6 or start == 0:
            continue
        # Does known code end right before the gap?
        p = start - 1
        ends = False
        for back in range(1, 5):
            q = start - back
            if q >= 0 and (mp[q] & 1 or c.code[q] == 1):
                op = rom[q]
                fl = mp[q] & 0xF00 or (M8 | X8)
                if q + disasm.cpu_len(op, fl) == start and (op in STOPS or op in (0x4C, 0x5C, 0x80, 0x82, 0x6C, 0x7C, 0xDC)):
                    ends = True
                break
        if not ends:
            continue
        t = codemap.run_addr(start)
        for fl in (M8 | X8, M16 | X16, M8 | X16, M16 | X8):
            ins, how = c.block(t, fl)
            if how == 'bad' or len(ins) < 3:
                continue
            refs = 0
            for pc, q, ln, f in ins:
                tt = c.static_target(pc, q, rom[q], f)
                if tt is not None and rom[q] in (0x20, 0x22, 0x4C, 0x5C):
                    to = c.off(tt)
                    if tt in known or (to is not None and mp[to] & 1):
                        refs += 1
            if refs:
                found.append((t, g - start))
                break
    return found


def known():
    cat = list(csv.DictReader(open(os.path.join(B, 'routines.csv'))))
    tags = {}
    for dirpath, _, files in os.walk(os.path.join(ROOT, 'src', 'stuntracefx')):
        for fn in files:
            if fn.endswith('.c'):
                for m in re.finditer(r'@routine ([0-9A-F]{2}):([0-9A-F]{4}) (\w+)', open(os.path.join(dirpath, fn)).read()):
                    tags[int(m.group(1) + m.group(2), 16)] = m.group(3)
    return cat, tags


def main(argv):
    cat, tags = known()
    cpu_cat = {int(r['addr'], 16) for r in cat if r['cpu'] == 'cpu'}
    gsu_cat = {int(r['addr'], 16) for r in cat if r['cpu'] == 'gsu'}
    c = Census()
    rom = c.rom
    seeds = []
    for v in (0x7FE4, 0x7FE6, 0x7FE8, 0x7FEA, 0x7FEE, 0x7FF4, 0x7FFC, 0x7FFE):
        a = rom[v] | rom[v + 1] << 8
        if a >= 0x8000:                       # $0108/$010C are RAM: jumps the game writes
            c.entries[a] = ('vector', 'traced')
            seeds.append((a, M8 | X8))
    recall = '--recall' in argv
    if not recall:
        for a in sorted(cpu_cat | set(tags)):
            o = c.off(a)
            if a not in c.entries and o is not None and not (c.mp[o] & 0x30):
                c.entries[a] = ('known', 'traced')
                seeds.append((a, c.mp[o] & 0xF00 or (M8 | X8)))

    def settle(work):
        c.walk(work)
        while True:
            work = []
            if not c.fields(set(cpu_cat) | set(tags) | set(c.entries), work):
                break
            c.walk(work)

    settle(seeds)
    found_cat = sum(1 for a in cpu_cat if a in c.entries)
    if recall:
        print('walk from the vectors alone reaches %d of the catalog\'s %d S-CPU routines (%.0f%%)' % (
            found_cat, len(cpu_cat), 100.0 * found_cat / max(1, len(cpu_cat))))
        return 0
    for a in gsu_cat:
        c.gentries.setdefault(a, 'known')
    c.gsu_walk()
    rounds = 0
    while rounds < 30:
        rounds += 1
        orph = [a for a, _ in orphans(c, set(cpu_cat) | set(tags) | set(c.entries)) if a not in c.entries]
        if not orph:
            break
        work = []
        for a in orph:
            c.add(a, 'orphan', 'static', c.clean(a, M8 | X8) or (M8 | X8), work)
        settle(work)
        c.gsu_walk()

    cpu_new = sorted(a for a in c.entries if a not in cpu_cat and a not in tags)
    gsu_new = sorted(a for a in c.gentries if a not in gsu_cat and a not in tags)
    named = cpu_cat | gsu_cat | set(tags)
    print('named: catalog %d (S-CPU %d, Super FX %d); source tags outside the catalog %d; together %d' % (
        len(cpu_cat) + len(gsu_cat), len(cpu_cat), len(gsu_cat), len(set(tags) - cpu_cat - gsu_cat), len(named)))
    by = {}
    for a in cpu_new:
        by.setdefault(c.entries[a], []).append(a)
    print('S-CPU entry points found that nothing names: %d' % len(cpu_new))
    for kind in ('call', 'table', 'pointer', 'orphan', 'vector'):
        for how in ('traced', 'static'):
            if by.get((kind, how)):
                print('  %-8s %-7s %3d  %s' % (kind, how, len(by[kind, how]), ' '.join('%06X' % a for a in by[kind, how])))
    print('Super FX entry points found that nothing names: %d  %s' % (len(gsu_new), ' '.join('%06X' % a for a in gsu_new)))
    print('static blocks rejected %d; orphan rounds %d' % (c.bad, rounds))
    print('all entry points (named and found): %d' % len(named | set(cpu_new) | set(gsu_new)))
    banks = {}
    for o in range(len(rom)):
        if not (c.mp[o] & 0x77) and not c.code[o] and not c.gcode[o]:
            banks[o >> 15] = banks.get(o >> 15, 0) + 1
    code_banks = sorted({c.off(a) >> 15 for a in named if c.off(a) is not None})
    print('bytes in code banks neither run, read nor reached: %d (%s)' % (
        sum(banks.get(b, 0) for b in code_banks), ', '.join('%02X:%d' % (b, banks.get(b, 0)) for b in code_banks)))
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
