#!/usr/bin/env python3
"""gsuwalk.py: the Super FX code the structured C stands for, read from the ROM.

    gsuwalk.py [--only NAME,...]      walk every Super FX routine in the C

The C of the Super FX's routines (not the interpreted programs, which
gsucode.py does) is written as C: ifs, loops, helpers shared by several
copies of the original's code. Each routine is walked from its address
beside the original's instructions, as gsucode.py walks a program: the C's
accesses matched to the instructions that make them, each if to the
branch that decides it (or to none, when the C decides what the original
decided earlier, from a helper's answer), each call to a helper either to
a call in the original (LINK, then a jump) or to the code in line, a
loop the C counts and the original lays out pass by pass walked pass by
pass. Where paths join again (after an if, at a loop's head, at a label)
each states the bytes up to where they meet, so the code after it is the
same on every path. A helper that stands for several copies of the
original's code gets a table of each copy's code, and its callers pass
the copy.

The places in time are gsucode.py's: a jump states its bytes through the
delay slot and the target's first byte, a return its bytes through the
delay slot, and the code returned to its own first byte; a routine is
entered with its first byte fetched.

Where the C and the original part ways (an access the original does not
make there, a branch the C has no if for, a site whose code would differ
between two paths that reach it), the walk stops and says where: the C is
then made to follow the original by hand. The code statements written
use lower-case hex (cycwrite.py --strip --code keeps them).
"""
import json, os, re, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
DECOMP = os.path.join(ROOT, 'src', 'stuntracefx')
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from gsucode import Unit, rom, Mismatch, TIME, C_EV, is_prefix  # noqa: E402

PROGRAMS = ('physics_run', 'model_run', 'frame_run', 'unused_run', 'unused_misc_run', 'fx_unused_run')
FLAG_BRANCH = {0x08: ('z', 0), 0x09: ('z', 1), 0x0A: ('s', 0), 0x0B: ('s', 1), 0x0C: ('cy', 0), 0x0D: ('cy', 1),
               0x0E: ('ov', 0), 0x0F: ('ov', 1)}


class Fail(Mismatch):
    pass


# ---- where the original is ----

class St:
    __slots__ = ('bank', 'pc', 'np', 'seg', 'out', 'val', 'rest', 'done', 'r13')

    def __init__(self, bank, pc):
        self.bank, self.pc, self.np, self.seg = bank, pc, pc + 1, pc
        self.out = []            # bytes fetched since the last code stated: (addr, n) or ('cache', r15)
        self.val = None          # the value of the expression just walked, when the C makes it a constant
        self.rest = []           # an instruction's accesses not yet matched (LM R14: the load, then R14)
        self.done = False        # returned (R15 written, its delay slot fetched) or stopped
        self.r13 = None          # R13 as the code set it (LOOP's target)

    def copy(self):
        s = St.__new__(St)
        s.bank, s.pc, s.np, s.seg = self.bank, self.pc, self.np, self.seg
        s.out, s.val, s.rest, s.done, s.r13 = list(self.out), self.val, list(self.rest), self.done, self.r13
        return s

    def key(self):
        return (self.pc, self.np, tuple(self.out), tuple(self.rest), self.done)

    def unit(self):
        return Unit(self.bank, self.pc)

    def where(self):
        return '%02X:%04X' % (self.bank, self.pc)

    def _code(self, a, n):
        if self.out and self.out[-1][0] != 'cache' and self.out[-1][0] + self.out[-1][1] == a:
            self.out[-1] = (self.out[-1][0], self.out[-1][1] + n)
        else:
            self.out.append((a, n))

    def is_cache(self, a):
        s = self.seg
        while s < a:
            s = Unit(self.bank, s).end + 1
        return s == a and rom(self.bank, a) == 0x02

    def fetch(self, last):
        a = self.np
        while a <= last:
            self._code(a, 1)
            if self.is_cache(a):
                p = self.seg
                while p < a:
                    q = Unit(self.bank, p)
                    if q.end + 1 == a and q.jump:
                        raise Fail('a CACHE in the delay slot of %r' % q)
                    p = q.end + 1
                self.out.append(('cache', a + 1))
            a += 1
        self.np = max(self.np, last + 1)

    def past(self, u):
        """On past instruction u (not a jump)."""
        if u.writes == 13:
            if u.op >> 4 == 0xF and u.alt == 0:
                self.r13 = u.target
            elif 0x10 <= u.op <= 0x1F and u.sreg == 15:
                self.r13 = u.end + 1
            else:
                self.r13 = None
        self.pc = u.end + 1

    def jump_to(self, t):
        """The target fetched: on at t."""
        self._code(t, 1)
        if rom(self.bank, t) == 0x02:
            self.out.append(('cache', t + 1))
        self.pc, self.np, self.seg = t, t + 1, t

    def take(self):
        s, self.out = self.out, []
        return s


def linked(st):
    """A LINK right before pc: the jump there is a call."""
    a = st.pc - 1
    return a >= 0x8000 and 0x91 <= rom(st.bank, a) <= 0x94 and not is_prefix(rom(st.bank, a - 1))


def own_jump(st, u):
    """A jump the original makes alone, the C saying nothing (BRA, IWT R15 to
    a label), with no access in its delay slot."""
    if not (u.jump == 'bra' or (u.jump == 'r15' and u.target is not None and not u.ev and not linked(st))):
        return False
    d = Unit(st.bank, u.end + 1)
    return not (set(d.ev) & set(TIME))


def follow(st, why):
    """On past instructions without accesses or decisions, through jumps the
    original makes alone; the instruction it stops at (an access, a branch,
    a call, a return, STOP)."""
    for _ in range(4000):
        u = st.unit()
        if u.ev or u.stop:
            return u
        if u.jump is None:
            st.past(u)
        elif own_jump(st, u):
            st.fetch(u.end + 1)
            st.jump_to(u.target)
        else:
            return u
    raise Fail('%s: no end to the code after %s' % (why, st.where()))


def path(st):
    """The pcs a state passes through before its next access or decision."""
    p, q = [], st.copy()
    for _ in range(4000):
        p.append(q.pc)
        u = q.unit()
        if u.ev or u.stop:
            break
        if u.jump is None:
            q.past(u)
        elif own_jump(q, u):
            q.fetch(u.end + 1)
            q.jump_to(u.target)
        else:
            break
    return p


def reach(st, x):
    """st brought to x with x's first byte fetched; None if it does not get there."""
    s = st.copy()
    for _ in range(4000):
        if s.pc == x:
            s.fetch(x)
            return s
        u = s.unit()
        if u.ev or u.stop:
            return None
        if u.jump is None:
            s.past(u)
        elif own_jump(s, u):
            s.fetch(u.end + 1)
            s.jump_to(u.target)
        else:
            return None
    return None


# ---- the C ----

def ast(path_):
    p = subprocess.run(['clang', '-x', 'c', '-Xclang', '-ast-dump=json', '-fsyntax-only', '-std=gnu99',
                        '-I', os.path.join(DECOMP, 'include'), path_], capture_output=True, text=True)
    if p.returncode:
        raise SystemExit('gsuwalk: %s does not parse:\n%s' % (path_, p.stderr[-3000:]))
    return json.loads(p.stdout)


def kids(n):
    return [c for c in n.get('inner', []) if c]


def bare(n):
    while n.get('kind') in ('ImplicitCastExpr', 'ParenExpr', 'CStyleCastExpr') and kids(n):
        n = kids(n)[0]
    return n


def callee(n):
    c = bare(kids(n)[0]) if kids(n) else {}
    if c.get('kind') == 'DeclRefExpr':
        return c.get('referencedDecl', {}).get('name')


def loc(r):
    """A location in the file itself (where a macro is used, not where it is defined)."""
    if 'expansionLoc' in r:
        r = r['expansionLoc']
    return r


def off(n, end=False):
    r = loc(n.get('range', {}).get('end' if end else 'begin', {}))
    return r.get('offset'), r.get('tokLen', 0)


def span(n):
    """(start, end) of a node in its file: end one past its last character."""
    b, _ = off(n)
    e, tl = off(n, True)
    return b, e + tl


class Fn:
    def __init__(self, name, path_, node, text):
        self.name, self.path, self.node, self.text = name, path_, node, text
        self.params = [c.get('name') for c in kids(node) if c.get('kind') == 'ParmVarDecl']
        self.body = next(c for c in kids(node) if c.get('kind') == 'CompoundStmt')
        self.keys = []           # its copies: the state each is walked from
        self.sites = {}          # site -> {(copy, passes): code}
        self.nodes = {}          # offset -> node, for the sites


def counted(init, cond, inc):
    """for (i = A; i < B or <= B; i++) with A and B numbers: (name, A, passes)."""
    def lit(n):
        n = bare(n) if n else {}
        return int(n['value']) if n.get('kind') == 'IntegerLiteral' else None
    if not (init and cond and inc):
        return None
    if init.get('kind') == 'DeclStmt':
        v = kids(init)[0]
        name, a = v.get('name'), lit(kids(v)[0]) if kids(v) else None
    elif init.get('kind') == 'BinaryOperator' and init.get('opcode') == '=':
        lhs = bare(kids(init)[0])
        name, a = lhs.get('referencedDecl', {}).get('name'), lit(kids(init)[1])
    else:
        return None
    c = bare(cond)
    if a is None or c.get('kind') != 'BinaryOperator' or c.get('opcode') not in ('<', '<=') or lit(kids(c)[1]) is None:
        return None
    if bare(kids(c)[0]).get('referencedDecl', {}).get('name') != name:
        return None
    if not (inc.get('kind') == 'UnaryOperator' and inc.get('opcode') == '++'):
        return None
    b = lit(kids(c)[1]) + (1 if c.get('opcode') == '<=' else 0)
    return (name, a, b - a) if b > a else None


def none():
    return {'next': [], 'break': [], 'continue': [], 'return': [], 'goto': []}


def add(a, b):
    for k in ('break', 'continue', 'return', 'goto'):
        a[k] = a[k] + b[k]
    return a


def show(code):
    return ' '.join('C%04X' % c[1] if c[0] == 'cache' else '%04X+%d' % c for c in code) or '-'


# ---- the walk ----

class Walker:
    def __init__(self, fns, routines):
        self.fns = fns
        self.routines = routines
        self.choice = {}         # (function, cond offset, branch pc) -> 'fall' | 'taken' | 'c'
        self.order = []
        self.memo = {}
        self.calls = {}          # (caller, call offset) -> {(caller copy, passes): (callee, copy)}
        self.stack = []
        self.probing = 0
        self.passes = {}         # (function, loop offset) -> (variable, first value)
        self.banks = {}          # function -> the bank its code is in
        self.first = {}          # (function, statement offset, passes) -> the first site in it

    def copy_of(self, fn, st, mode):
        k = (st.pc, st.np, tuple(st.out), mode)
        if k not in fn.keys:
            fn.keys.append(k)
        return fn.keys.index(k)

    def walk_fn(self, fn, st, mode):
        st = st.copy()
        if self.banks.setdefault(fn.name, st.bank) != st.bank:
            raise Fail('%s: code in two banks' % fn.name)
        ci = self.copy_of(fn, st, mode)
        mk = (fn.name, ci)
        if mk in self.memo:
            return [s.copy() for s in self.memo[mk]]
        if len(self.stack) > 40 or mk in self.stack:
            raise Fail('%s: recursion' % fn.name)
        self.stack.append(mk)
        try:
            ctx = Ctx(self, fn, ci, mode)
            out = ctx.block(fn.body, [st])
            if out['next']:
                if mode == 'call':
                    out['return'] += [ctx.ret(fn.body, s, None, end=True) for s in out['next']]
                else:
                    out['return'] += out['next']
            if out['break'] or out['continue'] or out['goto']:
                raise Fail('%s: a break, continue or goto out of the function' % fn.name)
            res = out['return']
        finally:
            self.stack.pop()
        self.memo[mk] = [s.copy() for s in res]
        return res


def snapshot(w):
    return ({n: (list(f.keys), {s: dict(d) for s, d in f.sites.items()}) for n, f in w.fns.items()},
            dict(w.memo), {k: dict(v) for k, v in w.calls.items()}, dict(w.first))


def restore(w, snap):
    fs, memo, calls, first = snap
    for n, (keys, sites) in fs.items():
        w.fns[n].keys, w.fns[n].sites = keys, sites
    w.memo, w.calls, w.first = memo, calls, first


class Ctx:
    """One function's walk in one copy."""

    def __init__(self, w, fn, ci, mode):
        self.w, self.fn, self.ci, self.mode = w, fn, ci, mode
        self.c_only = set()
        self.iters = ()          # the passes of loops walked pass by pass: ((loop offset, pass), ...)

    def fail(self, n, msg):
        o = off(n)[0] if n else 0
        line = self.fn.text.count('\n', 0, o or 0) + 1
        raise Fail('%s:%d %s: %s' % (os.path.relpath(self.fn.path, ROOT), line, self.fn.name, msg))

    def site(self, key, st, n=None, stmt=None):
        """The code st fetched since the last site, stated here."""
        code = st.take()
        if self.w.probing:
            return
        if n is not None:
            self.fn.nodes[key[1]] = n
        if stmt is not None:
            self.w.first.setdefault((self.fn.name, off(stmt)[0], self.iters), key)
        d = self.fn.sites.setdefault(key, {})
        k = (self.ci, self.iters)
        if k in d and d[k] != code:
            o = key[1]
            line = self.fn.text.count('\n', 0, o) + 1
            raise Fail('%s:%d %s: the code at %s differs between paths: %s / %s' % (
                os.path.relpath(self.fn.path, ROOT), line, self.fn.name, key[0], show(d[k]), show(code)))
        d[k] = code

    # statements
    def block(self, n, states):
        res = dict(none(), next=states)
        for c in kids(n):
            if c.get('kind') == 'LabelStmt':
                res['next'] = self.label(c, res)
                r = self.stmt(kids(c)[0], res['next']) if kids(c) else dict(none(), next=res['next'])
            else:
                if not res['next']:
                    continue
                r = self.stmt(c, res['next'])
            res['next'] = r['next']
            add(res, r)
        return res

    def label(self, n, res):
        """Paths meeting at a label: the gotos to it and the statement above."""
        name = n.get('name')
        arms = [(('fallin', off(n)[0]), s, n) for s in res['next']]
        rest = []
        for lab, s, g in res['goto']:
            if lab == name:
                arms.append((('goto', off(g)[0]), s, g))
            else:
                rest.append((lab, s, g))
        res['goto'] = rest
        return self.join(arms, n)

    def stmt(self, n, states):
        k = n.get('kind')
        if k == 'CompoundStmt':
            return self.block(n, states)
        if k == 'IfStmt':
            return self.if_(n, states)
        if k in ('ForStmt', 'WhileStmt', 'DoStmt'):
            return self.loop(n, states)
        if k == 'BreakStmt':
            return dict(none(), **{'break': [(s, n) for s in states]})
        if k == 'ContinueStmt':
            return dict(none(), **{'continue': [(s, n) for s in states]})
        if k == 'GotoStmt':
            return dict(none(), goto=[(label_name(self.fn, n), s, n) for s in states])
        if k == 'ReturnStmt':
            e = kids(n)[0] if kids(n) else None
            sts = self.expr(e, states, n) if e else states
            if self.mode == 'call':
                sts = [self.ret(n, s, e) for s in sts]
            return dict(none(), **{'return': sts})
        if k == 'NullStmt':
            return dict(none(), next=states)
        if k == 'DeclStmt':
            sts = states
            for v in kids(n):
                for e in kids(v):
                    sts = self.expr(e, sts, n)
            return dict(none(), next=sts)
        if k in ('SwitchStmt', 'LabelStmt'):
            self.fail(n, '%s not walked' % k)
        return dict(none(), next=self.expr(n, states, n))

    def ret(self, n, st, e, end=False):
        """A return from a routine entered by a jump: the original's return
        jump (or STOP) through its delay slot."""
        st = st.copy()
        if not st.done:
            u = follow(st, self.fn.name)
            if u.stop:
                st.fetch(u.addr)
            elif u.jump == 'r15' and u.target is None and not u.ev:
                st.fetch(u.end + 1)
            else:
                self.fail(n, 'the return: the original has %r at %s' % (u, st.where()))
            st.done = True
        if end:
            self.site(('fnend', off(n, True)[0]), st, n)
        else:
            self.site(('ret', off(n)[0], e is not None), st, n)
        return st

    # expressions
    def expr(self, n, states, stmt):
        if n is None or not states:
            return states
        k = n.get('kind')
        if k == 'CallExpr':
            return self.call(n, states, stmt)
        if k == 'IntegerLiteral':
            out = []
            for s in states:
                s = s.copy()
                s.val = int(n.get('value'))
                out.append(s)
            return out
        if k == 'ConditionalOperator':
            c, a, b = kids(n)
            t, f = self.cond(c, states, stmt)
            ta = self.expr(a, t, stmt) if t else []
            fb = self.expr(b, f, stmt) if f else []
            return self.join([(('qarm', off(a)[0]), s, a) for s in ta] + [(('qarm', off(b)[0]), s, b) for s in fb], n)
        if k == 'BinaryOperator' and n.get('opcode') in ('&&', '||'):
            t, f = self.cond(n, states, stmt)
            if len({s.key() for s in t + f}) > 1:
                self.fail(n, 'a %s outside an if takes different paths in the original' % n.get('opcode'))
            return (t + f)[:1]
        if k == 'UnaryOperator' and n.get('opcode') == '!':
            out = []
            for s in self.expr(kids(n)[0], states, stmt):
                s.val = None if s.val is None else int(not s.val)
                out.append(s)
            return out
        sts = states
        for c in kids(n):
            sts = self.expr(c, sts, stmt)
        for s in sts:
            s.val = None
        return sts

    def call(self, n, states, stmt):
        name = callee(n)
        sts = states
        for a in kids(n)[1:]:
            sts = self.expr(a, sts, stmt)
        if name in C_EV:
            return [self.event(n, s, C_EV[name], stmt) for s in sts]
        fn = self.w.fns.get(name)
        if fn is not None and opaque(fn):
            return [r for s in sts for r in self.gsu_call(n, s, None, stmt)]
        if fn is None:
            for s in sts:
                s.val = None
            return sts
        out = []
        for s in sts:
            s = s.copy()
            u = follow(s, self.fn.name)
            if u.jump == 'r15' and u.target is not None and not u.ev and linked(s):
                out += self.gsu_call(n, s, fn, stmt)
            else:
                # the code fetched so far stated here (by now the GSU has
                # fetched through its next instruction's first byte), so
                # the helper's copy is where it starts, not what came before
                s.fetch(s.pc)
                self.site(('icall', off(n)[0]), s, n, stmt)
                ci = self.w.copy_of(fn, s, 'inline')
                res = self.w.walk_fn(fn, s, 'inline')
                self.note_call(n, fn, ci)
                out += res
        return out

    def note_call(self, n, fn, ci):
        if self.w.probing:
            return
        self.fn.nodes[('callnode', off(n)[0])] = n
        d = self.w.calls.setdefault((self.fn.name, off(n)[0]), {})
        k = (self.ci, self.iters)
        if k in d and d[k] != (fn.name, ci):
            self.fail(n, 'the call reaches different copies of %s (by data): the C must choose the copy' % fn.name)
        d[k] = (fn.name, ci)

    def gsu_call(self, n, s, fn, stmt):
        """A call in the original: LINK, the jump and its delay slot, the
        callee from its target, then back at the LINK's address."""
        s = s.copy()
        u = follow(s, self.fn.name)
        if not (u.jump == 'r15' and u.target is not None and not u.ev and linked(s)):
            self.fail(n, 'a call: the original has %r at %s' % (u, s.where()))
        link = s.pc - 1
        ret = link + 1 + (rom(s.bank, link) & 15)
        s.fetch(u.end + 1)
        s.jump_to(u.target)
        self.site(('call', off(n)[0]), s, n, stmt)
        if fn is None:
            s.done = True
            res = [s]
        else:
            ci = self.w.copy_of(fn, s, 'call')
            res = self.w.walk_fn(fn, s, 'call')
            self.note_call(n, fn, ci)
        out = []
        for r in res:
            if not r.done:
                self.fail(n, 'the call to %s does not return in the original' % fn.name)
            r = r.copy()
            r.done, r.out, r.rest, r.val = False, [], [], None
            r.jump_to(ret)
            out.append(r)
        return out

    def event(self, n, s, kind, stmt):
        s = s.copy()
        s.val = None
        if s.rest:
            if s.rest[0] != kind:
                self.fail(n, 'C %s, the original %s next (in the same instruction)' % (kind, s.rest[0]))
            s.rest.pop(0)
            return s
        if s.done:
            self.fail(n, 'an access after the return jump')
        u = follow(s, self.fn.name)
        if u.jump and not u.ev:
            d = Unit(s.bank, u.end + 1)
            if d.ev and d.ev[0] == kind:
                self.fail(n, 'an access (%s) in the delay slot of %r: the C needs the decision first' % (kind, u))
            self.fail(n, 'C %s, the original %r at %s' % (kind, u, s.where()))
        if u.stop or u.ev[0] != kind:
            self.fail(n, 'C %s, the original %r at %s' % (kind, u, s.where()))
        s.fetch(u.end + 1)
        s.past(u)
        s.rest = list(u.ev[1:])
        if u.jump:
            s.done = True             # an access into R15 (a return pulled): its delay slot fetched with it
        if kind in TIME:
            self.site(('ev', off(n)[0]), s, n, stmt)
        return s

    # conditions: (true states, false states)
    def cond(self, n, states, stmt):
        b = bare(n)
        k = b.get('kind')
        if k == 'UnaryOperator' and b.get('opcode') == '!':
            t, f = self.cond(kids(b)[0], states, stmt)
            return f, t
        if k == 'BinaryOperator' and b.get('opcode') == '&&':
            t1, f1 = self.cond(kids(b)[0], states, stmt)
            t2, f2 = self.cond(kids(b)[1], t1, stmt) if t1 else ([], [])
            return t2, f1 + f2
        if k == 'BinaryOperator' and b.get('opcode') == '||':
            t1, f1 = self.cond(kids(b)[0], states, stmt)
            t2, f2 = self.cond(kids(b)[1], f1, stmt) if f1 else ([], [])
            return t1 + t2, f2
        sts = self.expr(b, states, stmt)
        t, f = [], []
        for s in sts:
            if s.val is not None:
                (t if s.val else f).append(s)
                continue
            s = s.copy()
            u = follow(s, self.fn.name)
            ch = self.decide(b, s, u) if u.jump == 'cond' and not u.ev else 'c'
            if ch == 'c':
                t.append(s.copy())
                f.append(s.copy())
                continue
            tk, fl = self.branch(b, s, u)
            if ch == 'taken':
                t.append(tk)
                f.append(fl)
            else:
                t.append(fl)
                f.append(tk)
        return t, f

    def branch(self, n, s, u):
        """The two ways from branch u: (taken, not taken)."""
        d = Unit(s.bank, u.end + 1)
        if not is_prefix(rom(s.bank, d.addr)) and (d.ev or d.jump or d.stop):
            self.fail(n, 'the delay slot of %r (%r) is not in the C' % (u, d))
        target = u.target
        if u.op == 0x3C:
            target = s.r13
            if target is None:
                self.fail(n, 'LOOP with R13 not known')
        s.fetch(u.end + 1)
        tk = s.copy()
        tk.jump_to(target)
        fl = s.copy()
        fl.pc = u.end + 1
        return tk, fl

    def decide(self, cond, s, u):
        """How the C's condition goes with the original's branch u: 'taken'
        (true when it branches), 'fall', or 'c' (the C decides alone: the
        original decided before)."""
        if off(cond)[0] in self.c_only:
            return 'c'
        key = (self.fn.name, off(cond)[0], s.pc)
        if key in self.w.choice:
            return self.w.choice[key]
        b, e = span(cond)
        txt = self.fn.text[b:e].replace(' ', '')
        first = None
        if u.op in FLAG_BRANCH:
            flag, v = FLAG_BRANCH[u.op]
            if txt == 'gsu.%s' % flag:
                first = 'taken' if v else 'fall'
        if u.op == 0x3C and txt.startswith('gsu_count_down('):
            first = 'taken'
        opts = [first] if first else ['fall', 'taken', 'c']
        self.w.choice[key] = opts[0]
        self.w.order.append((key, opts))
        return opts[0]

    def if_(self, n, states):
        ch = kids(n)
        c, th = ch[0], ch[1]
        el = ch[2] if len(ch) > 2 else None
        t, f = self.cond(c, states, n)
        r1 = self.stmt(th, t) if t else none()
        r2 = self.stmt(el, f) if el is not None and f else dict(none(), next=f)
        res = add(add(none(), r1), r2)
        arms = [(('then', off(n)[0]), s, th) for s in r1['next']] + \
               [(('else', off(n)[0]), s, el if el is not None else n) for s in r2['next']]
        res['next'] = self.join(arms, n)
        return res

    def join(self, arms, n):
        """Paths meeting again: each states the bytes up to where they meet,
        at its own site."""
        if not arms:
            return []
        ps = [path(s) for _, s, _ in arms]
        meet = next((x for x in ps[0] if all(x in p for p in ps[1:])), None)
        if meet is None:
            self.fail(n, 'the paths do not meet in the original (%s)' % ', '.join(s.where() for _, s, _ in arms))
        out = []
        for key, s, node in arms:
            r = reach(s, meet)
            r.val = s.val
            self.site(key, r, node)
            out.append(r)
        if len({s.key() for s in out}) > 1:
            self.fail(n, 'the paths meet at %04X differently' % meet)
        vals = {s.val for s in out}
        out[0].val = vals.pop() if len(vals) == 1 else None
        return out[:1]

    def loop(self, n, states):
        k = n.get('kind')
        ch = n.get('inner', [])
        if k == 'ForStmt':
            init, _, cond, inc, body = (ch + [{}] * 5)[:5]
        elif k == 'WhileStmt':
            init, cond, inc, body = None, ch[0], None, ch[1]
        else:
            init, cond, inc, body = None, ch[1], None, ch[0]
        init, cond, inc = init or None, cond or None, inc or None
        sts = self.stmt(init, states)['next'] if init else states
        cnt = counted(init, cond, inc) if k == 'ForStmt' else None
        if cnt:
            self.c_only.add(off(cond)[0])
        # the head: where the entry and the way back meet, found by a pass
        # from the entry; then the loop from the head itself
        saved = snapshot(self.w)
        self.w.probing += 1
        try:
            head = self.loop_pass(n, k, cond, inc, body, sts)
        except Fail:
            head = None
            if not cnt:
                raise
        finally:
            self.w.probing -= 1
            restore(self.w, saved)
        if head is None:
            return self.passes(n, cnt, body, sts)
        entry = [reach(s, head) for s in sts]
        if None in entry:
            self.fail(n, 'the entry does not reach the loop\'s head %04X' % head)
        for s in entry:
            self.site(('pre', off(n)[0]), s, n)
        if len({s.key() for s in entry}) > 1:
            self.fail(n, 'entered differently')
        return self.loop_pass(n, k, cond, inc, body, entry[:1], head)

    def loop_pass(self, n, k, cond, inc, body, sts, head=None):
        """A pass of the loop from sts: without a head, the pc where the entry
        and every way back meet; with one, the loop's outcome."""
        if k != 'DoStmt' and cond is not None:
            t, f = self.cond(cond, sts, n)
        else:
            t, f = sts, []
        r = self.stmt(body, t) if t else none()
        nxt = r['next']
        if inc is not None and nxt:
            nxt = self.expr(inc, nxt, n)
        conts = r['continue']
        if inc is not None and conts:
            conts = [(s2, c) for s, c in conts for s2 in self.expr(inc, [s], n)]
        if k == 'DoStmt' and cond is not None:
            t2, f2 = self.cond(cond, nxt + [s for s, _ in conts], n) if (nxt or conts) else ([], [])
            back = [(s, ('back', off(n)[0]), n) for s in t2]
            f = f + f2
        else:
            back = [(s, ('back', off(n)[0]), body) for s in nxt] + [(s, ('cont', off(c)[0]), c) for s, c in conts]
        if head is None:
            ps = [path(s) for s in sts + [b[0] for b in back]]
            meet = next((x for x in ps[0] if all(x in p for p in ps[1:])), None)
            if meet is None or not back:
                raise Fail('the loop does not come back in the original')
            return meet
        for s, key, node in back:
            r2 = reach(s, head)
            if r2 is None or r2.key() != sts[0].key():
                self.fail(node, 'the way back to the loop\'s head %04X differs (%s)' % (head, s.where()))
            self.site(key, r2, node)
        exits = [(('exit', off(n)[0]), s, n) for s in f] + [(('brk', off(c)[0]), s, c) for s, c in r['break']]
        res = dict(none(), next=self.join(exits, n) if exits else [])
        res['return'], res['goto'] = r['return'], r['goto']
        return res

    def passes(self, n, cnt, body, sts):
        """A loop the C counts and the original lays out pass by pass: each
        pass walked in turn, its code its own."""
        var, first, count = cnt
        self.w.passes[(self.fn.name, off(n)[0])] = (var, first)
        res = none()
        cur = sts
        breaks = []
        for i in range(count):
            self.iters = self.iters + ((off(n)[0], i),)
            try:
                r = self.stmt(body, cur) if cur else none()
            finally:
                self.iters = self.iters[:-1]
            if r['continue']:
                self.fail(n, 'continue in a loop laid out pass by pass')
            breaks += [(('brk', off(c)[0]), s, c) for s, c in r['break']]
            res['return'] += r['return']
            res['goto'] += r['goto']
            cur = r['next']
        exits = breaks + [(('exit', off(n)[0]), s, n) for s in cur]
        res['next'] = self.join(exits, n) if exits else []
        return res


def label_name(fn, g):
    b, e = span(g)
    m = re.match(r'goto\s+(\w+)', fn.text[b:e])
    return m.group(1) if m else None


def super_fx(fn):
    """The C of a Super FX routine (not the S-CPU's): it works on the Super
    FX's registers."""
    b, e = span(fn.body)
    return bool(re.search(r'gsu\.r\[|\bFxRegs\b|\bR\[|\bgsu_(?:ldw|stw|ldb|stb|sbk|getb|r14|romb|plot_r1|push|pull)\(',
                          fn.text[b:e]))


def opaque(fn):
    """An entry to a program's C (gsucode.py's): not walked here."""
    b, e = span(fn.body)
    return any(re.search(r'\b%s\(' % p, fn.text[b:e]) for p in PROGRAMS)


# ---- the files ----

def load(paths):
    fns, routines = {}, {}
    for p in paths:
        text = open(p).read()
        for m in re.finditer(r'@routine ([0-9A-F]{2}):([0-9A-F]{4}) (\w+)', text):
            routines[m.group(3)] = (int(m.group(1), 16), int(m.group(2), 16))
        for node in ast(p).get('inner', []):
            if node.get('kind') != 'FunctionDecl' or not any(c.get('kind') == 'CompoundStmt' for c in kids(node)):
                continue
            if node.get('loc', {}).get('includedFrom') or node['name'] in PROGRAMS:
                continue
            fns[node['name']] = Fn(node['name'], p, node, text)
    return fns, routines


def entries(fns, routines):
    out = []
    for name, (bank, addr) in sorted(routines.items(), key=lambda kv: kv[1]):
        fn = fns.get('abi_' + name) or fns.get(name)
        if fn is None or opaque(fn) or not super_fx(fn):
            continue
        out.append((fn, bank, addr))
    return out


def walk_all(fns, routines, only=None):
    w = Walker(fns, routines)
    fails = []
    for fn, bank, addr in entries(fns, routines):
        if only and fn.name not in only and fn.name[4:] not in only:
            continue
        w.order = []
        first = None
        for attempt in range(400):
            saved = snapshot(w)
            try:
                w.walk_fn(fn, St(bank, addr), 'call')
                break
            except Mismatch as e:
                restore(w, saved)
                if first is None:
                    first = str(e)
                while w.order:
                    key, opts = w.order.pop()
                    i = opts.index(w.choice[key])
                    if i + 1 < len(opts):
                        w.choice[key] = opts[i + 1]
                        w.order.append((key, opts))
                        break
                    del w.choice[key]
                else:
                    fails.append((fn.name, first))
                    break
        else:
            fails.append((fn.name, 'no way found: ' + first))
    return w, fails


def sources(src):
    return sorted(os.path.join(dp, f) for dp, _, fs in os.walk(src) for f in fs
                  if f.endswith('.c') and 'gsu_' in open(os.path.join(dp, f)).read())


def callers(fns):
    """function -> the functions that call it."""
    out = {}
    for f in fns.values():
        stack = [f.body]
        while stack:
            n = stack.pop()
            if n.get('kind') == 'CallExpr':
                c = callee(n)
                if c in fns:
                    out.setdefault(c, set()).add(f.name)
            stack += kids(n)
    return out


def rewritable(fns):
    """The walked functions whose callers are all walked too (a caller
    left as it is passes the copies as they were numbered)."""
    r = {n for n, f in fns.items() if f.keys}
    cl = callers(fns)
    while True:
        drop = {n for n in r if any(c not in r for c in cl.get(n, ()))}
        if not drop:
            return r
        r -= drop


def unused_tables(text):
    while True:
        changed = False
        for m in re.finditer(r'/\* cyccopy: [^\n]*\*/\nstatic const (?:GsuCode|uint8_t) (\w+)\[[^;]*;\n\n', text):
            if len(re.findall(r'\b%s\b' % m.group(1), text)) == 1:
                text = text[:m.start()] + text[m.end():]
                changed = True
                break
        if not changed:
            return text


def write(out_dir, only=None):
    """Walk, strip the walked functions' old code, walk again, state the
    code; the tree written to out_dir (the decomp when it is the decomp)."""
    import shutil
    fns, routines = load(sources(DECOMP))
    w, fails = walk_all(fns, routines, only)
    keep = rewritable(fns)
    tmp = out_dir + '.strip'
    shutil.rmtree(tmp, ignore_errors=True)
    shutil.copytree(DECOMP, tmp)
    by_file = {}
    for n in keep:
        by_file.setdefault(fns[n].path, []).append(fns[n].node)
    for p, nodes in by_file.items():
        q = os.path.join(tmp, os.path.relpath(p, DECOMP))
        open(q, 'w').write(strip_code(open(p).read(), nodes))
    fns2, routines2 = load(sources(tmp))
    w2, fails2 = walk_all(fns2, routines2, only)
    if {n for n, _ in fails2} != {n for n, _ in fails}:
        raise SystemExit('gsuwalk: the walk differs once the old code is gone: %s' % (
            {n for n, _ in fails2} ^ {n for n, _ in fails}))
    for n, f in fns2.items():
        if n not in keep:
            f.keys, f.sites = [], {}
    w2.calls = {k: v for k, v in w2.calls.items() if k[0] in keep and all(c in keep for c, _ in v.values())}
    em = Emit(w2, fns2)
    em.run()
    texts = em.apply()
    if os.path.abspath(out_dir) != os.path.abspath(DECOMP):
        shutil.rmtree(out_dir, ignore_errors=True)
        shutil.copytree(tmp, out_dir)
    for p in set(by_file) | set(texts):
        q = os.path.join(tmp, os.path.relpath(p, DECOMP)) if p.startswith(DECOMP) else p
        rel = os.path.relpath(q, tmp)
        text = texts.get(q, open(q).read())
        open(os.path.join(out_dir, rel), 'w').write(unused_tables(text))
    shutil.rmtree(tmp, ignore_errors=True)
    return fails, keep


def main(argv):
    if '--write' in argv:
        out = argv[argv.index('--write') + 1]
        fails, keep = write(out)
        for name, e in fails:
            print('FAIL %s: %s' % (name, e))
        print('functions written: %d' % len(keep))
        return 0
    only = None
    if '--only' in argv:
        only = set(argv[argv.index('--only') + 1].split(','))
    fns, routines = load(sources(os.environ.get('GSUWALK_SRC', DECOMP)))
    w, fails = walk_all(fns, routines, only)
    for name, e in fails:
        print('FAIL %s: %s' % (name, e))
    n = len(entries(fns, routines)) if not only else len(only)
    print('routines %d, failed %d; functions walked %d' % (n, len(fails), len([f for f in fns.values() if f.keys])))
    return 1 if fails else 0



# ---- writing the code into the C ----

CODE_FN = ('gsu_code', 'gsu_cache', 'gsu_codes')


def cycwrite_close(text, i):
    """The index of the parenthesis closing one opened before i."""
    depth = 1
    while depth:
        c = text[i]
        if c == '(':
            depth += 1
        elif c == ')':
            depth -= 1
        i += 1
    return i - 1
CODE_THEN = ('gsu_code_then', 'gsu_code_loop', 'gsu_cache_then', 'gsu_cache_loop', 'gsu_codes_then', 'gsu_codes_loop')


def parents(root):
    par = {}
    stack = [root]
    while stack:
        n = stack.pop()
        for c in kids(n):
            par[id(c)] = n
            stack.append(c)
    return par


def stmt_end(text, n):
    """One past a statement's end, its semicolon included."""
    e = span(n)[1]
    j = e
    while j < len(text) and text[j] in ' \t':
        j += 1
    if n.get('kind') != 'CompoundStmt' and j < len(text) and text[j] == ';':
        return j + 1
    return e


def indent_at(text, o):
    ls = text.rfind('\n', 0, o) + 1
    m = re.match(r'[ \t]*', text[ls:])
    return m.group(0)


def is_code_call(n):
    return n.get('kind') == 'CallExpr' and callee(n) in CODE_FN


def strip_code(text, fn_nodes):
    """The text without the code statements in the functions given (their
    syntax trees): every gsu_code, gsu_cache, gsu_codes and their _then and
    _loop forms, and else arms left with nothing in them."""
    edits = []

    def visit(n, par):
        k = n.get('kind')
        if k == 'IfStmt' and len(kids(n)) > 2:
            el = kids(n)[2]
            if el.get('kind') == 'CompoundStmt' and kids(el) and all(is_code_call(bare(c)) for c in kids(el)):
                then_end = stmt_end(text, kids(n)[1])
                edits.append((then_end, span(el)[1], ''))
                visit(kids(n)[0], n)
                visit(kids(n)[1], n)
                return
        if k == 'CompoundStmt':
            for c in kids(n):
                if is_code_call(bare(c)):
                    b = span(c)[0]
                    e = stmt_end(text, c)
                    # the statement and the space after it on its line, or its whole line
                    ls = text.rfind('\n', 0, b) + 1
                    rest = text[e:]
                    m = re.match(r'[ \t]*\n', rest)
                    if text[ls:b].strip() == '' and m:
                        edits.append((ls, e + m.end(), ''))
                    else:
                        m2 = re.match(r'[ \t]*', rest)
                        edits.append((b, e + m2.end(), ''))
                else:
                    visit(c, n)
            return
        if k == 'CallExpr' and callee(n) in CODE_THEN:
            last = kids(n)[-1]
            b, e = span(n)
            lb, le = span(last)
            edits.append((b, lb, ''))
            edits.append((le, e, ''))
            visit(last, n)
            return
        if k == 'BinaryOperator' and n.get('opcode') == ',' and is_code_call(bare(kids(n)[0])):
            b = span(n)[0]
            rb = span(kids(n)[1])[0]
            edits.append((b, rb, ''))
            visit(kids(n)[1], n)
            return
        for c in kids(n):
            visit(c, n)

    for node in fn_nodes:
        visit(node, None)
    edits.sort()
    out, pos = [], 0
    for b, e, r in edits:
        if b < pos:
            continue
        out.append(text[pos:b])
        out.append(r)
        pos = e
    out.append(text[pos:])
    return ''.join(out)


class Emit:
    """The edits that state each walked function's code in its file."""

    def __init__(self, w, fns):
        self.w, self.fns = w, fns
        self.edits = {}          # path -> [(start, end, text)]
        self.tables = {}         # path -> [(offset, text)]

    def edit(self, fn, b, e, t, prio=5):
        """Replace [b, e) with t; insertions at one place go in prio order
        (0 an opening brace, 7 a closing one, 9 an else added)."""
        self.edits.setdefault(fn.path, []).append((b, e, prio, len(self.edits.get(fn.path, [])), t))

    def code_items(self, code):
        return code

    def literal(self, code, form, x=None):
        items = []
        for c in code:
            if c[0] == 'cache':
                items.append(('gsu_cache', '0x%04x' % c[1]))
            else:
                items.append(('gsu_code', '0x%02x%04x, %d' % (self.bank, c[0], c[1])))
        if form == 'stmt':
            return ' '.join('%s(%s);' % it for it in items)
        if form == 'expr':
            return ', '.join('%s(%s)' % it for it in items)
        for name, args in reversed(items):
            x = '%s_then(%s, %s)' % (name, args, x)
        return x

    def run(self):
        for fn in self.fns.values():
            if fn.keys and fn.sites:
                self.function(fn)
        self.copies()

    def function(self, fn):
        text = fn.text
        self.bank = fn.keys[0][0] >> 16 if False else None
        par = parents(fn.node)
        nc = len(fn.keys)
        sites = sorted(fn.sites.items(), key=lambda kv: (kv[0][1], kv[0][0]))
        # the bank: every routine's code is in one bank
        self.bank = self.w.banks[fn.name]
        table = []               # slots: [(site, [rows])]
        slot = 0
        plan = []
        for key, d in sites:
            if not any(d.values()):
                continue
            loops = sorted({lo for (_, it) in d for lo, _ in it})
            counts = [max(i for (_, it) in d for lo, i in it if lo == L) + 1 for L in loops]
            nb = 1
            for c in counts:
                nb *= c
            if nc == 1 and not loops:
                plan.append((key, ('lit', d[(0, ())])))
                continue
            k = max(len(v) for v in d.values())
            rows = [[(0, 0)] * (k * nb) for _ in range(nc)]
            for (ci, it), v in d.items():
                idx = 0
                for L, c in zip(loops, counts):
                    i = dict(it)[L]
                    idx = idx * c + i
                ent = [(c[1], 0) if c[0] == 'cache' else ((self.bank << 16) | c[0], c[1]) for c in v]
                rows[ci][idx * k: idx * k + len(ent)] = ent
            # the passes' index
            idx = None
            for L, c in zip(loops, counts):
                var, first = self.w.passes[(fn.name, L)]
                term = '(%s - %d)' % (var, first) if first else var
                idx = term if idx is None else '(%s) * %d + %s' % (idx, c, term)
            plan.append((key, ('tab', slot, k, idx)))
            table.append(rows)
            slot += k * nb
        if table and nc > 1 and 'copy' not in fn.params:
            raise Fail('%s: %d copies of its code, and no copy parameter' % (fn.name, nc))
        name = '%s_fetch' % fn.name
        if table:
            width = slot
            rows = []
            for ci in range(nc):
                r = []
                for t in table:
                    r += t[ci]
                rows.append('    {%s},' % ', '.join('{0x%06x, %d}' % e if e[1] else ('{0x%04x, 0}' % e[0] if e[0] else '{0, 0}') for e in r))
            decl = '/* gsuwalk: the code of each copy of %s, read from the ROM */\nstatic const GsuCode %s[%d][%d] = {\n%s\n};\n\n' % (
                fn.name, name, nc, width, '\n'.join(rows))
            self.edit(fn, span(fn.node)[0], span(fn.node)[0], decl)
        row = '%s[%s]' % (name, 'copy' if nc > 1 else '0')
        for key, how in plan:
            self.place(fn, par, key, how, row)

    def code(self, how, row, form, x=None):
        if how[0] == 'lit':
            return self.literal(how[1], form, x)
        _, s, k, idx = how
        at = '%s + %d' % (row, s) if s else row
        if idx:
            at += ' + %d * %s' % (k, idx) if k > 1 else ' + %s' % idx
        if form == 'stmt':
            return 'gsu_codes(%s, %d);' % (at, k)
        if form == 'expr':
            return 'gsu_codes(%s, %d)' % (at, k)
        return 'gsu_codes_then(%s, %d, %s)' % (at, k, x)

    def before_stmt(self, fn, par, st, how, row):
        text = fn.text
        p = par.get(id(st), {})
        b = span(st)[0]
        if p.get('kind') in ('CompoundStmt', 'LabelStmt'):
            self.edit(fn, b, b, self.code(how, row, 'stmt') + '\n' + indent_at(text, b))
        else:
            self.edit(fn, b, b, '{ ' + self.code(how, row, 'stmt') + ' ', 0)
            e = stmt_end(text, st)
            self.edit(fn, e, e, ' }', 7)

    def at_end(self, fn, par, arm, how, row):
        """At the end of a statement (an if's arm, a loop's body)."""
        text = fn.text
        if arm.get('kind') == 'CompoundStmt':
            e = span(arm)[1] - 1           # its closing brace
            ls = text.rfind('\n', 0, e) + 1
            if text[ls:e].strip() == '':
                inner = indent_at(text, e) + '    '
                self.edit(fn, ls, ls, inner + self.code(how, row, 'stmt') + '\n')
            else:
                self.edit(fn, e, e, self.code(how, row, 'stmt') + ' ')
        else:
            b = span(arm)[0]
            self.edit(fn, b, b, '{ ', 0)
            e = stmt_end(text, arm)
            self.edit(fn, e, e, ' ' + self.code(how, row, 'stmt') + ' }', 7)

    def place(self, fn, par, key, how, row):
        kind, o = key[0], key[1]
        text = fn.text
        node = fn.nodes.get(o)
        if kind in ('ev', 'call', 'icall'):
            stmt = self.stmt_of(fn, par, node)
            first = any(v == key for (f, so, _), v in self.w.first.items() if f == fn.name and so == span(stmt)[0])
            if first and stmt.get('kind') not in ('ForStmt', 'WhileStmt', 'DoStmt') and stmt.get('kind') != 'ReturnStmt':
                self.before_stmt(fn, par, stmt, how, row)
            elif first and stmt.get('kind') == 'ReturnStmt':
                self.before_stmt(fn, par, stmt, how, row)
            else:
                b, e = span(node)
                self.edit(fn, b, b, '(' + self.code(how, row, 'expr') + ', ')
                self.edit(fn, e, e, ')')
        elif kind == 'ret':
            has_expr = key[2]
            others = [v for (f, so, _), v in self.w.first.items() if f == fn.name and so == o and v != key]
            if has_expr and others:
                e = kids(node)[0]
                b, en = span(e)
                pre, post = self.code(how, row, 'then', '\0').split('\0')
                self.edit(fn, b, b, pre)
                self.edit(fn, en, en, post)
            else:
                self.before_stmt(fn, par, node, how, row)
        elif kind == 'fnend':
            self.at_end(fn, par, fn.body, how, row)
        elif kind == 'then':
            self.at_end(fn, par, kids(node)[1] if node.get('kind') == 'IfStmt' else node, how, row)
        elif kind == 'else':
            if node.get('kind') == 'IfStmt':      # no else: one added
                e = stmt_end(text, kids(node)[1])
                self.edit(fn, e, e, ' else { ' + self.code(how, row, 'stmt') + ' }', 9)
            else:
                self.at_end(fn, par, node, how, row)
        elif kind in ('pre', 'cont', 'brk', 'goto', 'fallin'):
            self.before_stmt(fn, par, node, how, row)
        elif kind == 'back':
            if node.get('kind') in ('DoStmt',):
                raise Fail('%s: code on a do loop\'s way back' % fn.name)
            self.at_end(fn, par, node, how, row)
        elif kind == 'exit':
            # after the loop: only the loop's own ending gets there
            loop = node
            if any(k[0] == 'brk' for k in fn.sites if self.inside(loop, k[1])):
                raise Fail('%s: code at a loop\'s end that breaks too' % fn.name)
            e = stmt_end(text, loop)
            self.edit(fn, e, e, '\n' + indent_at(text, span(loop)[0]) + self.code(how, row, 'stmt'))
        elif kind == 'qarm':
            b, e = span(node)
            self.edit(fn, b, b, '(' + self.code(how, row, 'expr') + ', ')
            self.edit(fn, e, e, ')')
        else:
            raise Fail('%s: a site %s' % (fn.name, kind))

    def inside(self, n, o):
        b, e = span(n)
        return b <= o < e

    def stmt_of(self, fn, par, n):
        """The statement an expression is in."""
        while True:
            p = par.get(id(n))
            if p is None or p.get('kind') in ('CompoundStmt', 'LabelStmt', 'CaseStmt', 'DefaultStmt'):
                return n
            if p.get('kind') in ('IfStmt', 'WhileStmt', 'DoStmt', 'ForStmt', 'SwitchStmt', 'ReturnStmt', 'DeclStmt'):
                if p.get('kind') in ('ReturnStmt', 'DeclStmt'):
                    return self.stmt_of(fn, par, p)
                # the condition (or a for's parts): the statement is the if or loop
                ks = kids(p)
                if p.get('kind') == 'IfStmt' and n is ks[0]:
                    return p
                if p.get('kind') in ('WhileStmt', 'DoStmt', 'ForStmt', 'SwitchStmt') and n is not ks[-1] and not (p.get('kind') == 'DoStmt' and n is ks[0]):
                    return p
                return n
            n = p

    def copies(self):
        """The copy each call passes."""
        made = {}
        for (caller, o), d in sorted(self.w.calls.items()):
            fn = self.fns[caller]
            node = fn.nodes.get(('callnode', o))
            if node is None:
                continue
            cname = {v[0] for v in d.values()}
            if len(cname) != 1:
                continue
            cal = self.fns[cname.pop()]
            if 'copy' not in cal.params:
                continue
            args = kids(node)[1:]
            arg = args[cal.params.index('copy')]
            vals = {k: v[1] for k, v in d.items()}
            if len(set(vals.values())) == 1:
                expr = str(next(iter(vals.values())))
            else:
                nc = len(fn.keys)
                if any(it for (_, it) in vals):
                    raise Fail('%s: the copy of %s it calls depends on the pass' % (caller, cal.name))
                tname = '%s_in_%s' % (cal.name, caller)
                made[tname] = made.get(tname, 0) + 1
                if made[tname] > 1:
                    tname += '_%d' % made[tname]
                tab = [vals.get((ci, ()), 0) for ci in range(nc)]
                decl = '/* gsuwalk: the copy of %s each copy of %s calls */\nstatic const uint8_t %s[%d] = {%s};\n\n' % (
                    cal.name, caller, tname, nc, ', '.join(str(v) for v in tab))
                self.edit(fn, span(fn.node)[0], span(fn.node)[0], decl)
                expr = '%s[copy]' % tname
            b = span(arg)[0]
            m = re.match(r'GSU_COPY\(', fn.text[b:])
            if not m:
                raise Fail('%s: the copy it passes %s is not GSU_COPY(...)' % (caller, cal.name))
            e = cycwrite_close(fn.text, b + m.end()) + 1
            self.edit(fn, b, e, 'GSU_COPY(%s)' % expr)

    def apply(self):
        out = {}
        for path_, edits in self.edits.items():
            text = open(path_).read()
            edits.sort(key=lambda t: (t[0], t[1], t[2], t[3]))
            res, pos = [], 0
            for b, e, _, _, t in edits:
                if b < pos:
                    raise Fail('%s: two edits overlap at %d' % (path_, b))
                res.append(text[pos:b])
                res.append(t)
                pos = e
            res.append(text[pos:])
            out[path_] = ''.join(res)
        return out


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
