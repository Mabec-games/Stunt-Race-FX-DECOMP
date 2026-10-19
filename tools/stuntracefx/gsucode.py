#!/usr/bin/env python3
"""gsucode.py: the Super FX code the labelled C stands for, read from the ROM.

    gsucode.py FILE.c FUNC [FILE.c FUNC ...]   write each function's code
    gsucode.py --dry FILE.c FUNC ...            check only, write nothing

The interpreted programs (physics_run, model_run, frame_run and the
like) are the original's code statement for statement, with a label
lXXXX at every address the code jumps to. So the bytes the GSU fetches
between two of the C's accesses follow from the ROM alone: walk the
function's statements and the original's instructions side by side from
each label, the C's accesses (loads, stores, ROM-buffer reads, R14
writes, ROMB, RAMB, PLOT, RPIX, the multiplies) matched to the
instructions that make them, and state before each access the bytes
fetched since the last: the instruction's own and the one after it (the
pipeline fetches a byte ahead).

Where the code goes (the same everywhere, so no stretch depends on who
ran before it):
  a jump, a branch taken, a call: the jumper states the bytes through
    its delay slot and the target's first byte;
  a return: the returner states the bytes through its delay slot, and
    the code it returns to states its own first byte;
  a label reached from the statement above: that statement states the
    bytes through the label's first byte;
  a program: superfx.c states its first byte when it starts.
So every label is entered with its first byte fetched. A CACHE fetched
is stated where it is (gsu_cache), and the code it marks applies after
the next byte, as in gsu_clock.c.

The code statements written use lower-case hex, so cycwrite.py --strip
--code and gsutime.py leave them alone.
"""
import os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
ROM = os.path.join(ROOT, 'build', 'stuntracefx', 'baserom.sfc')
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import cycwrite  # noqa: E402

_rom = None


def rom(bank, addr):
    global _rom
    if _rom is None:
        _rom = open(ROM, 'rb').read()
    assert addr >= 0x8000, hex(addr)
    return _rom[(bank & 0x3F) * 0x8000 + (addr & 0x7FFF)]


PRE = {}


def is_prefix(op):
    return op in (0x3D, 0x3E, 0x3F) or 0x10 <= op <= 0x2F or 0xB0 <= op <= 0xBF


def prefixed(bank, p, x):
    """The instruction at x run after the prefix byte at p."""
    op = rom(bank, p)
    key = (bank, x)
    old = PRE.get(key)
    if op in (0x3D, 0x3E, 0x3F):
        PRE[key] = (op - 0x3C, 0, 0, False)
    elif 0x20 <= op <= 0x2F:
        PRE[key] = (0, op & 15, op & 15, True)
    elif 0x10 <= op <= 0x1F:
        PRE[key] = (0, 0, op & 15, False)
    else:
        PRE[key] = (0, op & 15, 0, False)
    u = Unit(bank, x)
    if old is None:
        del PRE[key]
    else:
        PRE[key] = old
    return u


class Unit:
    """One instruction as the assembler writes it: its prefixes and the
    opcode they modify. ev: what the C does for it, in order (L load,
    S store, G ROM-buffer read, R an R14 write, B ROMB, A RAMB, P PLOT,
    X RPIX, M MULT, F FMULT/LMULT); jump: 'cond' (a branch or LOOP),
    'bra', 'r15' (any other write of R15); target: a jump's address when
    the code says it; cache, stop."""

    def __init__(self, bank, addr):
        self.addr = addr
        # a branch keeps the prefixes before it for its delay slot
        alt, sreg, dreg, b = PRE.get((bank, addr), (0, 0, 0, False))
        a = addr
        while True:
            op = rom(bank, a)
            if op in (0x3D, 0x3E, 0x3F):
                alt = op - 0x3C
                a += 1
                continue
            if 0x20 <= op <= 0x2F:
                sreg = dreg = op & 15
                b = True
                a += 1
                continue
            if 0x10 <= op <= 0x1F and not b:
                dreg = op & 15
                a += 1
                continue
            if 0xB0 <= op <= 0xBF and not b:
                sreg = op & 15
                a += 1
                continue
            break
        self.op, self.alt = op, alt
        hi, lo = op >> 4, op & 15
        n = 1
        ev = []
        writes = None            # the register the instruction writes
        self.jump = self.target = None
        self.cache = op == 0x02
        self.stop = op == 0x00
        if 0x05 <= op <= 0x0F:
            n = 2
            d = rom(bank, a + 1)
            d = d - 256 if d >= 128 else d
            self.target = (a + 2 + d) & 0xFFFF
            self.jump = 'bra' if op == 0x05 else 'cond'
            if alt or sreg or dreg or b:
                PRE[(bank, a + 2)] = (alt, sreg, dreg, b)
        elif op == 0x3C:
            self.jump = 'cond'   # LOOP: to R13
            writes = 12
        elif op in (0x03, 0x04, 0x4D, 0x4F, 0x95, 0x96, 0x97, 0x9E, 0xC0):
            writes = dreg
        elif 0x10 <= op <= 0x1F:          # MOVE (WITH sreg; TO rn)
            writes = lo
        elif 0xB0 <= op <= 0xBF:          # MOVES (WITH dreg; FROM rn)
            writes = dreg
        elif 0x30 <= op <= 0x3B:
            ev.append('S')
        elif 0x40 <= op <= 0x4B:
            ev.append('L')
            writes = dreg
        elif op == 0x4C:
            if alt & 1:
                ev.append('X')
                writes = dreg
            else:
                ev.append('P')
        elif op == 0x4E:
            pass                           # COLOR, CMODE
        elif hi in (0x5, 0x6, 0x7, 0xC):
            if not (hi == 0x6 and alt == 3):   # CMP writes nothing
                writes = dreg
        elif hi == 0x8:
            ev.append('M')
            writes = dreg
        elif op == 0x90:
            ev.append('S')
        elif 0x91 <= op <= 0x94:
            writes = 11
        elif 0x98 <= op <= 0x9D:
            writes = 15                    # JMP, LJMP
        elif op == 0x9F:
            ev.append('F')
            writes = dreg
        elif hi == 0xA:
            n = 2
            if alt == 1:
                ev.append('L')
                writes = lo
            elif alt == 2:
                ev.append('S')
            else:
                writes = lo
                d = rom(bank, a + 1)
                self.target = (d - 256 if d >= 128 else d) & 0xFFFF
        elif 0xD0 <= op <= 0xDE or 0xE0 <= op <= 0xEE:
            writes = lo
        elif op == 0xDF:
            ev.append({0: 'G', 1: 'G', 2: 'A', 3: 'B'}[alt])
        elif op == 0xEF:
            ev.append('G')
            writes = dreg
        elif hi == 0xF:
            n = 3
            if alt == 1:
                ev.append('L')
                writes = lo
            elif alt == 2:
                ev.append('S')
            else:
                writes = lo
                self.target = rom(bank, a + 1) | rom(bank, a + 2) << 8
        elif op in (0x00, 0x01, 0x02):
            pass
        else:
            raise ValueError('%02X:%04X: opcode %02X' % (bank, addr, op))
        if writes == 14:
            ev.append('R')
        if writes == 15 and self.jump is None:
            self.jump = 'r15'
        if self.jump == 'r15' and not (hi == 0xF and alt == 0) and not (hi == 0xA and alt == 0):
            self.target = None             # computed
        self.end = a + n - 1
        self.ev = ev

    def __repr__(self):
        return '<%04X-%04X %02X%s%s>' % (self.addr, self.end, self.op, ''.join(self.ev), self.jump or '')


C_EV = {'gsu_ldw': 'L', 'gsu_ldb': 'L', 'gsu_pull': 'L', 'gsu_stw': 'S', 'gsu_stb': 'S', 'gsu_sbk': 'S',
        'gsu_push': 'S', 'gsu_getb': 'G', 'gsu_getbh': 'G', 'gsu_getbs': 'G', 'gsu_getbl': 'G', 'gsu_getc': 'G',
        'gsu_r14': 'R', 'gsu_romb': 'B', 'gsu_ramb': 'A', 'gsu_plot_r1': 'P', 'gsu_plot': 'P',
        'gsu_rpix_r': 'X', 'gsu_rpix': 'X', 'gsu_mult': 'M', 'gsu_umult': 'M', 'gsu_fmult': 'F', 'gsu_lmult': 'F'}
TIME = 'LSGRBAPX'          # the C's places in time; M and F only keep the two in step


def c_events(stmt):
    """The accesses a statement makes, in the order C makes them (a call's
    arguments before the call)."""
    out, stack = [], []
    i = 0
    while i < len(stmt):
        m = re.match(r'(\w+)\s*\(', stmt[i:])
        if m and (i == 0 or not (stmt[i - 1].isalnum() or stmt[i - 1] == '_')):
            stack.append(m.group(1))
            i += m.end()
            continue
        c = stmt[i]
        if c == '(':
            stack.append(None)
        elif c == ')':
            name = stack.pop()
            if name in C_EV:
                out.append(C_EV[name])
        i += 1
    return out


class Mismatch(Exception):
    pass


def timed(u):
    return bool(set(u.ev) & set(TIME))


class Walk:
    """The original's instructions from a label on, as the C passes them:
    pc the next instruction, np the next byte the GSU fetches, seg where
    the instructions now run in a line from (a label, a jump's target)."""

    def __init__(self, bank):
        self.bank = bank
        self.pc = self.np = self.seg = None
        self.out = []            # what to state before the C statement now: (addr, n) or ('cache', r15)

    def unit(self):
        return Unit(self.bank, self.pc)

    def enter(self, x):
        """At x with its first byte fetched (a label, a jump's target)."""
        self.pc, self.np, self.seg = x, x + 1, x

    def is_cache(self, a):
        """A CACHE instruction at a, the instructions read in a line from seg."""
        s = self.seg
        while s < a:
            s = Unit(self.bank, s).end + 1
        return s == a and rom(self.bank, a) == 0x02

    def fetch(self, last):
        """The bytes from np through last, a CACHE among them stated after its byte."""
        a = self.np
        while a <= last:
            self._code(a, 1)
            if self.is_cache(a):
                p = self.seg
                while p < a:
                    q = Unit(self.bank, p)
                    if q.end + 1 == a and q.jump:
                        raise Mismatch('a CACHE in the delay slot of %r (R15 is its target then)' % q)
                    p = q.end + 1
                self.out.append(('cache', a + 1))
            a += 1
        self.np = max(self.np, last + 1)

    def target(self, x):
        """A jump's target fetched (the code at x)."""
        self._code(x, 1)
        if rom(self.bank, x) == 0x02:
            self.out.append(('cache', x + 1))
        self.enter(x)

    def _code(self, a, n):
        if self.out and self.out[-1][0] != 'cache' and self.out[-1][0] + self.out[-1][1] == a:
            self.out[-1] = (self.out[-1][0], self.out[-1][1] + n)
        else:
            self.out.append((a, n))

    def take(self):
        s, self.out = self.out, []
        return s

    def skip_to(self, stop, what):
        """Past instructions without accesses or jumps until stop(u)."""
        u = self.unit()
        while not stop(u):
            if timed(u) or u.jump or u.stop:
                raise Mismatch('%s: the original has %r first' % (what, u))
            self.pc = u.end + 1
            u = self.unit()
        return u


def parse_body(text, start):
    """The function's body: (text, start, end) inside its braces."""
    i = text.index('{', start)
    j = cycwrite._close(text, i + 1, '{', '}')
    return text[i + 1:j], i + 1, j


LABEL = re.compile(r'^l([0-9A-F]{4}):$')
GOTO_IF = re.compile(r'^if \(t\) goto l([0-9A-F]{4});$')
GOTO = re.compile(r'^goto l([0-9A-F]{4});$')
CALL = re.compile(r'^(abi_\w+)\(\);$')
QUIET = ('int t;', 'uint16_t nx;', '(void)t;', '(void)nx;', 'uint16_t base = R[10];', 'at = nx;')


def gen(c_text, fn, bank):
    """The text with fn's code stated (fn as cycwrite.py --strip --code leaves it)."""
    m = re.search(r'^static void %s\(uint16_t at, uint16_t ret(, int copy)?\)\n' % fn, c_text, re.M)
    if not m:
        raise SystemExit('gsucode: no %s' % fn)
    body, b0, b1 = parse_body(c_text, m.end() - 1)
    lines = body.split('\n')
    out = []
    w = Walk(bank)
    falls = False            # the statement before falls into the next
    ind = '    '

    def code(items):
        s = []
        for it in items:
            if it[0] == 'cache':
                s.append('gsu_cache(0x%04x);' % it[1])
            else:
                s.append('gsu_code(0x%02x%04x, %d);' % (bank, it[0], it[1]))
        return ' '.join(s)

    def emit():
        s = code(w.take())
        if s:
            out.append(ind + s)

    def ahead(k):
        """The next statement that is not a comment."""
        while k < len(lines):
            s = lines[k].strip()
            if s and not s.startswith('/*') and not s.startswith('*'):
                return s
            k += 1
        return ''

    k = 0
    in_dispatch = False
    while k < len(lines):
        line = lines[k]
        s = line.strip()
        k += 1
        if in_dispatch:
            if s.startswith('switch (at)'):
                out.append(ind + 'gsu_code(0x%02x0000 | at, 1);   /* where it jumps: its first byte */' % bank)
                out.append('fetched:')
                cases = [int(c, 16) for c in re.findall(r'case 0x([0-9A-F]{4}):', '\n'.join(lines[k:]))]
                cached = ['at == 0x%04X' % c for c in cases if rom(bank, c) == 0x02]
                if cached:
                    out.append(ind + 'if (%s)' % ' || '.join(cached))
                    out.append(ind + '    gsu_cache((uint16_t)(at + 1));   /* the code there starts with a CACHE */')
                out.append('enter:')
            out.append(line)
            continue
        if not s or s.startswith('/*') or s.startswith('*') or s in QUIET:
            out.append(line)
            continue
        if s == 'goto dispatch;' and w.pc is None:
            out.append(line.replace('goto dispatch;', 'goto enter;   /* the caller fetched the first byte */'))
            continue
        mm = LABEL.match(s)
        if mm:
            x = int(mm.group(1), 16)
            if falls:
                w.skip_to(lambda u: u.addr == x or u.addr > x, 'into l%04X' % x)
                if w.pc != x:
                    raise Mismatch('into l%04X: the original is at %04X' % (x, w.pc))
                w.fetch(x)
                emit()
            w.enter(x)
            out.append(line)
            falls = True
            continue
        if s == 'dispatch:':
            if falls:
                raise Mismatch('falls into dispatch')
            out.append(line)
            in_dispatch = True
            continue
        if s == 'if (t) {':
            # LOOP to R13 in another routine: the jump through dispatch
            blk = []
            while lines[k].strip() != '}':
                blk.append(lines[k])
                k += 1
            k += 1
            inner = [b.strip() for b in blk if b.strip() and not b.strip().startswith('/*')]
            if len(inner) != 2 or not inner[0].startswith('at = ') or inner[1] != 'goto dispatch;':
                raise Mismatch('if (t) { %s }' % ' '.join(inner))
            u = w.skip_to(lambda u: u.jump, 'if (t) { goto dispatch; }')
            if u.jump != 'cond':
                raise Mismatch('if (t) { goto dispatch; }: %r' % u)
            w.fetch(u.end + 1)                  # through the delay slot
            emit()
            out.append(line)
            out.extend(blk)
            out.append(lines[k - 1])
            w.pc = u.end + 1
            d = w.unit()
            if not is_prefix(rom(bank, d.addr)) and (timed(d) or d.jump or d.stop):
                raise Mismatch('if (t) { goto dispatch; }: the delay slot %r' % d)
            falls = True
            continue
        mm = GOTO_IF.match(s)
        if mm:
            x = int(mm.group(1), 16)
            if getattr(w, 'delayed', None) is not None:
                if w.delayed != x:
                    raise Mismatch('if (t) goto l%04X: the delay slot\'s branch goes to %04X' % (x, w.delayed))
                w.delayed = None
                out.append(line)
                if w.delayed_cond:
                    w.pc = Unit(bank, w.pc).end + 1     # not taken: on past the delay slot
                    falls = True
                else:
                    falls = False
                continue
            u = w.skip_to(lambda u: u.jump, 'if (t) goto l%04X' % x)
            if u.jump not in ('cond', 'bra') and u.target != x:
                raise Mismatch('if (t) goto l%04X: %r' % (x, u))
            if u.target is not None and u.op != 0x3C and u.target != x:
                raise Mismatch('if (t) goto l%04X: the original goes to %04X' % (x, u.target))
            w.fetch(u.end + 1)                  # through the delay slot
            emit()
            if u.jump == 'cond':
                out.append(line.replace('goto l%04X;' % x, '{ %s goto l%04X; }' % (code([(x, 1)] + ([('cache', x + 1)] if rom(bank, x) == 2 else [])), x)))
                w.pc = u.end + 1                # not taken: the delay slot runs on
                d = w.unit()
                if not is_prefix(rom(bank, d.addr)) and (timed(d) or d.jump or d.stop):
                    raise Mismatch('if (t) goto l%04X: the delay slot %r is not in the C' % (x, d))
                if is_prefix(rom(bank, d.addr)) and timed(Unit(bank, x)) != timed(prefixed(bank, d.addr, x)):
                    raise Mismatch('if (t) goto l%04X: the delay slot\'s prefix changes it' % x)
                falls = True
            else:
                w.target(x)
                emit()
                out.append(line)
                falls = False
            continue
        mm = GOTO.match(s)
        if mm:
            x = int(mm.group(1), 16)
            if getattr(w, 'delayed', None) is not None:
                if w.delayed != x or w.delayed_cond:
                    raise Mismatch('goto l%04X after a delay slot\'s jump to %04X' % (x, w.delayed))
                w.delayed = None
                out.append(line)
                falls = False
                continue
            u = w.skip_to(lambda u: u.jump or u.addr == x, 'goto l%04X' % x)
            if u.addr == x:
                w.fetch(x)                      # the original runs into it
            else:
                if u.target != x:
                    raise Mismatch('goto l%04X: %r' % (x, u))
                w.fetch(u.end + 1)
                w.target(x)
            emit()
            out.append(line)
            falls = False
            continue
        if s == 'goto dispatch;':
            if not getattr(w, 'jumped', False):
                u = w.skip_to(lambda u: u.jump, 'goto dispatch')
                if u.jump != 'r15' or u.target is not None:
                    raise Mismatch('goto dispatch: %r' % u)
                w.fetch(u.end + 1)
            w.jumped = False
            emit()
            out.append(line)
            falls = False
            continue
        if s.startswith('return;'):
            u = w.skip_to(lambda u: u.stop, 'STOP')
            w.fetch(u.addr)
            emit()
            out.append(line)
            falls = False
            continue
        mm = CALL.match(s)
        if mm:
            r11 = re.match(r'\s*R\[11\] = 0x([0-9A-F]{4});$', out[-1])
            if not r11:
                raise Mismatch('%s: no return address before it' % s)
            ret = int(r11.group(1), 16)
            link = []
            def stop(u):
                if 0x91 <= u.op <= 0x94 and not u.alt:
                    link.append(u.addr + 1 + (u.op & 15))   # R15 is one past it
                    return False
                return u.jump
            u = w.unit()
            while not stop(u):
                if timed(u) or u.stop:
                    raise Mismatch('%s: the original has %r first' % (s, u))
                w.pc = u.end + 1
                u = w.unit()
            if u.jump != 'r15' or u.target is None:
                raise Mismatch('%s: %r' % (s, u))
            if link[-1:] != [ret]:
                raise Mismatch('%s: LINK to %s, the C %04X' % (s, [hex(a) for a in link], ret))
            w.fetch(u.end + 1)
            w.target(u.target)                  # the callee's first byte
            emit()
            out.append(line)
            w.enter(ret)                        # back: the return's first byte, here
            w.out = [(ret, 1)] + ([('cache', ret + 1)] if rom(bank, ret) == 2 else [])
            emit()
            falls = True
            continue
        ev = c_events(s)
        if ev:
            i = 0
            first = True
            while i < len(ev):
                u = w.unit()
                while not u.ev:
                    if u.stop:
                        raise Mismatch('%s: the original stops at %04X' % (s, u.addr))
                    if u.jump:
                        u = delay_access(w, u, ev[i], lines, k, out, code)
                        break
                    w.pc = u.end + 1
                    u = w.unit()
                else:
                    if u.ev != ev[i:i + len(u.ev)]:
                        raise Mismatch('%s: C %s, the original %r' % (s, ''.join(ev[i:]), u))
                    w.fetch(u.end + 1)
                    w.pc = u.end + 1
                    if u.jump:                  # TO R15; LDW: the delay slot fetched with it
                        if u.jump != 'r15':
                            raise Mismatch('%s: %r jumps' % (s, u))
                        if ahead(k) == 'at = nx;':
                            w.jumped = True
                        else:
                            w.delay_jump = u    # an access in its delay slot: below
                if first:
                    emit()
                    first = False
                elif w.out:
                    if any(e in TIME for e in u.ev):
                        raise Mismatch('%s: code between its accesses (%r)' % (s, u))
                    out.append(ind + code(w.take()))   # before the statement: no access between
                i += len(u.ev)
            out.append(line)
            falls = True
            if getattr(w, 'delay_jump', None) is not None:
                # the target is fetched before the delay slot's access: where
                # it jumps is worked out first, the code at it stated, then
                # the delay slot, then on through dispatch's switch
                j, w.delay_jump = w.delay_jump, None
                rest = []
                while len(rest) < 3:
                    r = lines[k].strip()
                    k += 1
                    if r and not r.startswith('/*'):
                        rest.append((r, lines[k - 1]))
                (ds, dl), (at_s, at_l), (gs, gl) = rest
                d = Unit(bank, j.end + 1)
                if (not at_s.startswith('at = ') or gs != 'goto dispatch;' or c_events(ds) != d.ev
                        or d.end != d.addr or re.search(r'R\[1[45]\]', at_s)):
                    raise Mismatch('%s: then %s / %s / %s' % (s, ds, at_s, gs))
                out.append(at_l)
                out.append(ind + 'gsu_code(0x%02x0000 | at, 1);   /* where it jumps, fetched before the delay slot runs */' % bank)
                out.append(dl)
                out.append(gl.replace('goto dispatch;', 'goto fetched;'))
                falls = False
            continue
        out.append(line)
        if s.startswith('t = ') or re.match(r'R\[\d+\] = ', s) or s.startswith('nx = '):
            falls = True
    text = c_text[:b0] + '\n'.join(out) + c_text[b1:]
    if m.group(1):          # the copies' tables and the copy parameter go
        text = text.replace('static void %s(uint16_t at, uint16_t ret, int copy)' % fn,
                            'static void %s(uint16_t at, uint16_t ret)' % fn)
        text = re.sub(r'\b%s\(([^;]*?), GSU_COPY\(\w+\)\);' % fn, r'%s(\1);' % fn, text)
    return text


def delay_access(w, j, kind, lines, k, out, code):
    """An access in a jump's delay slot: the jump's target (or, a branch
    not taken, the next byte) is fetched before it runs."""
    d = Unit(w.bank, j.end + 1)
    if not d.ev or d.ev[0] != kind or d.end != d.addr:
        raise Mismatch('jump %r, then %r' % (j, d))
    nxt = ''
    for l in lines[k:]:
        if l.strip() and not l.strip().startswith('/*'):
            nxt = l.strip()
            break
    w.fetch(d.addr)
    if j.jump == 'cond':
        mm = GOTO_IF.match(nxt)
        if not mm:
            raise Mismatch('%r: its delay slot %r, then %s' % (j, d, nxt))
        x = int(mm.group(1), 16)
        if rom(w.bank, x) == 0x02:
            raise Mismatch('%r: a CACHE at the target of a branch with an access in its delay slot' % j)
        s = code(w.take())
        if s:
            out.append('    ' + s)
        out.append('    gsu_code(t ? 0x%02x%04x : 0x%02x%04x, 1);   /* the delay slot runs after the next fetch */'
                   % (w.bank, x, w.bank, d.end + 1))
        w.np = d.end + 2                        # not taken (taken: at the label)
        w.pc = d.addr
        w.delayed = x
        w.delayed_cond = True
    else:
        if j.target is None:
            raise Mismatch('%r: a computed jump with an access in its delay slot' % j)
        w.target(j.target)                     # on at the target after the delay slot
        w.delayed = j.target
        w.delayed_cond = False
    return d


def bank_of(text, fn):
    """The bank fn's code is in: of a routine it runs (its @routine tag)."""
    for m in re.finditer(r'\b%s\(0x([0-9A-F]{4}),' % fn, text):
        r = re.search(r'@routine ([0-9A-F]{2}):%s ' % m.group(1), text)
        if r:
            return int(r.group(1), 16)
    raise SystemExit('gsucode: %s: no @routine for its bank' % fn)


def main(argv):
    dry = '--dry' in argv
    argv = [a for a in argv if a != '--dry']
    for f, fn in zip(argv[0::2], argv[1::2]):
        text = open(f).read()
        # fn's learned code goes (its copies' table and its statements); the
        # rest of the file is left as it is
        text = re.sub(r"/\* cyccopy: [^\n]*\*/\nstatic const GsuCode %s_code\[[^;]*;\n\n" % fn, '', text)
        m = re.search(r'^static void %s\(uint16_t at, uint16_t ret(?:, int copy)?\)\n' % fn, text, re.M)
        if not m:
            raise SystemExit('gsucode: %s: no %s' % (f, fn))
        _, b0, b1 = parse_body(text, m.end() - 1)
        text = text[:b0] + cycwrite.strip(text[b0:b1], True) + text[b1:]
        bank = bank_of(text, fn)
        try:
            new = gen(text, fn, bank)
        except Mismatch as e:
            raise SystemExit('gsucode: %s %s: %s' % (f, fn, e))
        if not dry:
            open(f, 'w').write(new)
        print('%s %s: done' % (f, fn))


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
