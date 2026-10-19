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
import copy, json, os, re, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
DECOMP = os.path.join(ROOT, 'src', 'stuntracefx')
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from gsucode import Unit, rom, Mismatch, TIME, C_EV, is_prefix  # noqa: E402

# the interpreted programs (gsucode.py's): static void NAME_run(uint16_t at, uint16_t ret)
PROGRAMS = ('physics_run', 'model_run', 'frame_run', 'unused_run', 'unused_misc_run', 'fx_unused_run',
            'fx_unused2_run', 'fx_unused8b_run', 'unused84_run', 'unusedb4_run', 'unused80_run')
FLAG_BRANCH = {0x08: ('z', 0), 0x09: ('z', 1), 0x0A: ('s', 0), 0x0B: ('s', 1), 0x0C: ('cy', 0), 0x0D: ('cy', 1),
               0x0E: ('ov', 0), 0x0F: ('ov', 1)}


class Fail(Mismatch):
    pass


DEBUG = bool(os.environ.get('GSUWALK_DEBUG'))
DEBUG2 = os.environ.get('GSUWALK_DEBUG') == '2'


# ---- where the original is ----

class St:
    __slots__ = ('bank', 'pc', 'np', 'seg', 'out', 'val', 'rest', 'done', 'regs', 'lk', 'pend', 'called')

    def __init__(self, bank, pc):
        self.bank, self.pc, self.np, self.seg = bank, pc, pc + 1, pc
        self.out = []            # bytes fetched since the last code stated: (addr, n) or ('cache', r15)
        self.val = None          # the value of the expression just walked, when the C makes it a constant
        self.rest = []           # an instruction's accesses not yet matched (LM R14: the load, then R14)
        self.done = False        # returned (R15 written, its delay slot fetched) or stopped
        self.regs = {}           # registers the code set to a known value (R13: LOOP's target)
        self.lk = None           # the pc right after a LINK just passed (a call's jump is there)
        self.pend = None         # a branch whose delay slot ran: (its pc, 'taken' or 'fall', the site)
        self.called = None       # a call whose delay slot's access ran, the C's call to come: its return address

    @property
    def r13(self):
        return self.regs.get(13)

    def copy(self):
        s = St.__new__(St)
        s.bank, s.pc, s.np, s.seg = self.bank, self.pc, self.np, self.seg
        s.out, s.val, s.rest, s.done, s.regs = list(self.out), self.val, list(self.rest), self.done, dict(self.regs)
        s.lk, s.pend, s.called = self.lk, self.pend, self.called
        return s

    def key(self):
        return (self.pc, self.np, tuple(self.out), tuple(self.rest), self.done, self.pend, self.called)

    def unit(self):
        """The instruction at pc; a JMP Rn to a register the code set takes
        it as its target."""
        u = Unit(self.bank, self.pc)
        if u.jump == 'r15' and u.target is None and 0x98 <= u.op <= 0x9D and u.alt == 0 and (u.op & 15) in self.regs:
            u = copy.copy(u)
            u.target = self.regs[u.op & 15]
        return u

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
        w = u.writes
        if w is not None:
            v = None
            if u.op >> 4 == 0xF and u.alt == 0:                 # IWT
                v = u.target
            elif 0x10 <= u.op <= 0x1F:                          # MOVE
                v = u.end + 1 if u.sreg == 15 else self.regs.get(u.sreg)
            if v is None:
                self.regs.pop(w, None)
            else:
                self.regs[w] = v
        self.pc = u.end + 1
        if 0x91 <= u.op <= 0x94 and u.alt == 0:
            self.lk = self.pc

    def jump_to(self, t):
        """The target fetched: on at t. A delay slot with an operand (the
        last byte fetched, an IWT, say; a prefix is an instruction alone) takes the rest of its operand from
        the target's bytes, and the code goes on after them."""
        d, k = self.np - 1, 0
        s = self.seg
        while s < d:
            s = Unit(self.bank, s).end + 1
        if s == d and not is_prefix(rom(self.bank, d)):
            k = Unit(self.bank, d).end - d
        self._code(t, 1)
        if rom(self.bank, t) == 0x02:
            self.out.append(('cache', t + 1))
        self.pc, self.np, self.seg = t + k, t + 1, t + k

    def take(self):
        s, self.out = self.out, []
        return s


def linked(st):
    """A LINK right before pc: the jump there is a call."""
    return st.lk == st.pc


def own_jump(st, u):
    """A jump the original makes alone, the C saying nothing (BRA, IWT R15 to
    a label, or a branch to just past its own delay slot, which fetches the
    same bytes either way), with no access in its delay slot."""
    nowhere = u.jump == 'cond' and u.op != 0x3C and u.target == u.end + 2
    if not (nowhere or u.jump == 'bra' or (u.jump == 'r15' and u.target is not None and not u.ev and not linked(st))):
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


def returns(st):
    """The next thing the state does is its return jump or STOP (no access
    before it)."""
    s = st.copy()
    if s.done:
        return False
    u = follow(s, 'returns')
    return u.stop or (u.jump == 'r15' and u.target is None and not u.ev)


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


CODE_WRAP = ('gsu_code_then', 'gsu_code_loop', 'gsu_cache_then', 'gsu_cache_loop', 'gsu_codes_then', 'gsu_codes_loop')


def bare(n):
    """The expression itself: casts, parentheses and code statements around it gone."""
    while True:
        k = n.get('kind')
        if k in ('ImplicitCastExpr', 'ParenExpr', 'CStyleCastExpr') and kids(n):
            n = kids(n)[0]
        elif k == 'CallExpr' and callee(n) in CODE_WRAP:
            n = kids(n)[-1]
        elif k == 'BinaryOperator' and n.get('opcode') == ',' and kids(n)[0].get('kind') == 'CallExpr' and \
                callee(kids(n)[0]) in ('gsu_code', 'gsu_cache', 'gsu_codes'):
            n = kids(n)[1]
        else:
            return n


def callee(n):
    c = kids(n)[0] if kids(n) else {}
    while c.get('kind') in ('ImplicitCastExpr', 'ParenExpr') and kids(c):
        c = kids(c)[0]
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


TEXT = {}     # the text the offsets are into, set by load()


def span(n):
    """(start, end) of a node in its file: end one past its last character
    (past a macro's closing parenthesis when it ends in a macro's use)."""
    b, _ = off(n)
    e, tl = off(n, True)
    end = e + tl
    r = n.get('range', {}).get('end', {})
    text = TEXT.get('cur')
    if text and 'expansionLoc' in r:
        m = re.match(r'[A-Za-z_]\w*\s*\(', text[e:])
        if m:
            depth, i = 1, e + m.end()
            while depth:
                if text[i] == '(':
                    depth += 1
                elif text[i] == ')':
                    depth -= 1
                i += 1
            end = i
    return b, end


class Fn:
    def __init__(self, name, path_, node, text):
        # name: unique (a static function's with its file); cname: as in the C
        self.cname = node['name']
        self.name, self.path, self.node, self.text = name, path_, node, text
        self.params = [c.get('name') for c in kids(node) if c.get('kind') == 'ParmVarDecl']
        self.body = next(c for c in kids(node) if c.get('kind') == 'CompoundStmt')
        self.keys = []           # its copies: the state each is walked from
        self.sites = {}          # site -> {(copy, passes): code}
        self.nodes = {}          # offset -> node, for the sites


def counted(init, cond, inc, const=None):
    """for (i = A; i < B or <= B; i++) with A and B numbers (or parameters
    given numbers): (name, A, passes)."""
    def lit(n):
        if not n:
            return None
        if const is not None:
            return const(n)
        n = bare(n)
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
    if a is None or c.get('kind') != 'BinaryOperator' or c.get('opcode') not in ('<', '<=', '>', '>=') or lit(kids(c)[1]) is None:
        return None
    if bare(kids(c)[0]).get('referencedDecl', {}).get('name') != name:
        return None
    if not inc.get('kind') == 'UnaryOperator':
        return None
    b = lit(kids(c)[1])
    if inc.get('opcode') == '++' and c['opcode'] in ('<', '<='):
        b += 1 if c['opcode'] == '<=' else 0
        return (name, a, b - a, 1) if b >= a else None
    if inc.get('opcode') == '--' and c['opcode'] in ('>', '>='):
        b -= 1 if c['opcode'] == '>=' else 0
        return (name, a, a - b, -1) if a >= b else None
    return None


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
        self.progress = 0        # sites passed this attempt: the furthest attempt's failure is the one told
        self.passes = {}         # (function, loop offset) -> (variable, first value)
        self.banks = {}          # function -> the bank its code is in
        self.first = {}          # (function, statement offset, passes) -> the first site in it
        self.duals = {}          # (function, delay slot site) -> (the C's condition, 'taken' or 'fall')

    def copy_of(self, fn, st, mode):
        k = (st.bank, st.pc, st.np, tuple(st.out), mode)
        if k not in fn.keys:
            fn.keys.append(k)
        return fn.keys.index(k)

    def walk_fn(self, fn, st, mode, env=None):
        st = st.copy()
        ci = self.copy_of(fn, st, mode)
        env = env or {}
        mk = (fn.name, ci, tuple(sorted(env.items())))
        if mk in self.memo:
            return [s.copy() for s in self.memo[mk]]
        if mk in self.stack and mode == 'call':
            # a routine calling itself: to its caller it has returned, whatever
            # its code (walked once, in the call already under way)
            r = st.copy()
            r.done = True
            return [r]
        if len(self.stack) > 40 or mk in self.stack:
            raise Fail('%s: recursion' % fn.name)
        self.stack.append(mk)
        mark = len(self.order)
        prev = TEXT.get('cur')
        TEXT['cur'] = fn.text
        try:
            ctx = Ctx(self, fn, ci, mode, env)
            out = ctx.block(fn.body, [st])
            if any(ctx.pending.values()):
                raise Fail('%s: a goto to a label the walk never reached' % fn.name)
            if out['break'] or out['continue'] or out['goto']:
                raise Fail('%s: a break, continue or goto out of the function' % fn.name)
            if mode == 'call':
                res = out['return'] + [ctx.ret(fn.body, s, None, end=True) for s in out['next']]
            else:
                arms = [a for a in out['return'] if not a[1].done]
                for s in out['next']:
                    s.val = None
                arms += [(('fnend', off(fn.body, True)[0]), s, fn.body) for s in out['next'] if not s.done]
                res = [a[1] for a in out['return'] if a[1].done] + [s for s in out['next'] if s.done]
                # paths that return different values stay apart: the caller may
                # decide on them; so do the two ways out of a delay slot the
                # caller's C has still to decide
                byval = {}
                for a in arms:
                    if a[1].pend:
                        if a[0][0] != 'fnend':
                            raise Fail('%s: returns before the C decides a branch' % fn.name)
                        res.append(a[1])
                        continue
                    byval.setdefault(a[1].val, []).append(a)
                for v, group in byval.items():
                    if len(group) == 1:          # one way back: its bytes go on with the caller
                        s = group[0][1].copy()
                        s.val = v
                        res.append(s)
                        continue
                    for s in ctx.join(group, fn.body):
                        s.val = v
                        res.append(s)
        finally:
            self.stack.pop()
            TEXT['cur'] = prev
        # a function walked through: how its ifs pair is settled (a later
        # failure is its caller's to look into)
        if not self.probing:
            del self.order[mark:]
        self.memo[mk] = [s.copy() for s in res]
        return res


def snapshot(w):
    return ({n: (list(f.keys), {s: dict(d) for s, d in f.sites.items()}) for n, f in w.fns.items()},
            dict(w.memo), {k: dict(v) for k, v in w.calls.items()}, dict(w.first), dict(w.duals))


def restore(w, snap):
    fs, memo, calls, first, duals = snap
    for n, (keys, sites) in fs.items():
        w.fns[n].keys, w.fns[n].sites = keys, sites
    w.memo, w.calls, w.first, w.duals = memo, calls, first, duals


class Ctx:
    """One function's walk in one copy."""

    def __init__(self, w, fn, ci, mode, env=None):
        self.w, self.fn, self.ci, self.mode = w, fn, ci, mode
        self.env = env or {}     # parameters given as numbers (and never changed in it)
        self.c_only = set()
        self.pending = {}        # forward gotos: label -> [(state, the goto)]
        self.loop_r13 = []       # R13 as each loop (innermost last) was entered
        self.iters = ()          # the passes of loops walked pass by pass: ((loop offset, pass), ...)

    def fail(self, n, msg):
        o = off(n)[0] if n else 0
        line = self.fn.text.count('\n', 0, o or 0) + 1
        raise Fail('%s:%d %s: %s' % (os.path.relpath(self.fn.path, ROOT), line, self.fn.name, msg))

    def site(self, key, st, n=None, stmt=None):
        """The code st fetched since the last site, stated here."""
        code = st.take()
        self.w.progress += 1
        if self.w.probing:
            return
        if n is not None:
            self.fn.nodes[key[:2]] = n
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
                if not res['next'] and not self.goto_into(c):
                    continue
                r = self.stmt(c, res['next'])
            res['next'] = r['next']
            add(res, r)
        return res

    def goto_into(self, n):
        """A forward goto waits for a label inside statement n."""
        if not any(self.pending.values()):
            return False
        want = {k for k, v in self.pending.items() if v}
        stack = [n]
        while stack:
            x = stack.pop()
            if x.get('kind') == 'LabelStmt' and x.get('name') in want:
                return True
            stack += kids(x)
        return False

    def label(self, n, res):
        """Paths meeting at a label: the gotos to it and the statement above."""
        name = n.get('name')
        arms = [(('fallin', off(n)[0]), s, n) for s in res['next']]
        for s, g, it in self.pending.pop(name, []):
            arms.append((('goto', off(g)[0]), s, g, it))
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
            lab = label_name(self.fn, n)
            # forward (to a label further on, perhaps inside a block): it
            # joins there when the walk gets to it
            self.pending.setdefault(lab, []).extend((s, n, self.iters) for s in states)
            return none()
        if k == 'ReturnStmt':
            e = kids(n)[0] if kids(n) else None
            sts = self.expr(e, states, n) if e else states
            if self.mode == 'call':
                sts = [self.ret(n, s, e) for s in sts]
                return dict(none(), **{'return': sts})
            # in line: where the caller goes on (joined at the function's end)
            if e is None:
                for s in sts:
                    s.val = None                         # a return with no value has none
            return dict(none(), **{'return': [(('ret', off(n)[0], e is not None), s, n) for s in sts]})
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
        if k == 'IntegerLiteral' or (k == 'DeclRefExpr' and isinstance(self.env.get(n.get('referencedDecl', {}).get('name')), int)) \
                or (k == 'MemberExpr' and isinstance(self.member(n), int)):
            v = int(n.get('value')) if k == 'IntegerLiteral' else self.const(n)
            out = []
            for s in states:
                s = s.copy()
                s.val = v
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
        if k == 'BinaryOperator' and n.get('opcode') == '=':
            # a local set to a number the C knows here (an int the walk can fold)
            lhs = bare(kids(n)[0])
            if lhs.get('kind') == 'DeclRefExpr' and lhs.get('referencedDecl', {}).get('kind') == 'VarDecl':
                v = self.const(kids(n)[1])
                name = lhs['referencedDecl']['name']
                if v is not None and not loop_writes(self.fn, lhs['referencedDecl'].get('id')):
                    self.env[name] = v              # set outside any loop, never stepped
                else:
                    self.env.pop(name, None)
        return sts

    def call(self, n, states, stmt):
        name = callee(n)
        if name in ('gsu_code', 'gsu_cache', 'gsu_codes'):
            return states                           # code stated before: not the C's own work
        if name in CODE_WRAP:
            return self.expr(kids(n)[-1], states, stmt)
        if isinstance(self.env.get(name), tuple) and self.env[name][0] == 'fn':   # a function passed in
            name = self.env[name][1]
        elif name is None and kids(n):
            m = self.member(kids(n)[0])                 # a function in a constant struct
            if isinstance(m, tuple):
                name = m[1]
        sts = states
        for a in kids(n)[1:]:
            sts = self.expr(a, sts, stmt)
        if name in C_EV:
            return [r for s in sts for r in self.event(n, s, C_EV[name], stmt)]
        fn = resolve(self.w.fns, name, self.fn.path)
        if fn is not None and opaque(fn):
            return [r for s in sts for r in self.gsu_call(n, s, None, stmt)]
        if fn is None:
            for s in sts:
                s.val = None
            return sts
        out = []
        for s in sts:
            s = s.copy()
            if s.called is not None:
                # the call's jump gone through already (an access in its delay slot)
                if not self.is_call(fn, s.pc, self.constants(n, fn)):
                    self.fail(n, 'the call at %s is not %s' % (s.where(), fn.name))
                out += self.gsu_call(n, s, fn, stmt)
                continue
            ahead_ = s.copy()                         # looked ahead on a copy: a helper starts where s is
            u = follow(ahead_, self.fn.name)
            if u.jump == 'r15' and u.target is not None and not u.ev and linked(ahead_) and \
                    self.is_call(fn, u.target, self.constants(n, fn)):
                out += self.gsu_call(n, s, fn, stmt)
            else:
                # the code fetched so far stated here (by now the GSU has
                # fetched through its next instruction's first byte), so
                # the helper's copy is where it starts, not what came before
                # a jump (or a run on) into a routine at its entry: it runs as
                # the routine (its return is the caller's)
                r = self.w.routines.get(fn.cname[4:] if fn.cname.startswith('abi_') else fn.cname)
                at = reach(s, r[1]) if r is not None and r[0] == s.bank and not s.rest else None
                if at is not None:
                    s = at
                else:
                    while rom(s.bank, s.pc) == 0x01 and not s.rest:   # a NOP left (a delay slot run on)
                        s.pc += 1
                    s.fetch(s.pc)
                self.site(('icall', off(n)[0]), s, n, stmt)
                mode = 'call' if at is not None else 'inline'
                ci = self.w.copy_of(fn, s, mode)
                self.dbg(n, '%s %s at %s' % (mode, fn.name, s.where()))
                res = self.w.walk_fn(fn, s, mode, self.constants(n, fn))
                self.note_call(n, fn, ci)
                out += res
        return out

    def const(self, n):
        """An expression's value when it is made of numbers and parameters
        given numbers (no calls), else None."""
        n = bare(n)
        k = n.get('kind')
        if k == 'IntegerLiteral':
            return int(n['value'])
        if k == 'DeclRefExpr':
            v = self.env.get(n.get('referencedDecl', {}).get('name'))
            return v if isinstance(v, int) else None
        if k == 'MemberExpr':
            v = self.member(n)
            return v if isinstance(v, int) else None
        if k == 'ArraySubscriptExpr':
            i = self.const(kids(n)[1])
            a = bare(kids(n)[0])
            if i is None or a.get('kind') != 'DeclRefExpr':
                return None
            x, init = self.const_var(a.get('referencedDecl', {}).get('id'))
            if init is None or i >= len(kids(init)):
                return None
            el = bare(kids(init)[i])
            return int(el['value']) if el.get('kind') == 'IntegerLiteral' else None
        if k == 'ConditionalOperator':
            c = self.const(kids(n)[0])
            return None if c is None else self.const(kids(n)[1] if c else kids(n)[2])
        if k == 'UnaryOperator' and n.get('opcode') in ('!', '-', '~'):
            v = self.const(kids(n)[0])
            if v is None:
                return None
            return {'!': int(not v), '-': -v, '~': ~v}[n['opcode']]
        if k == 'BinaryOperator':
            ops = {'==': lambda a, b: int(a == b), '!=': lambda a, b: int(a != b), '<': lambda a, b: int(a < b),
                   '>': lambda a, b: int(a > b), '<=': lambda a, b: int(a <= b), '>=': lambda a, b: int(a >= b),
                   '+': lambda a, b: a + b, '-': lambda a, b: a - b, '*': lambda a, b: a * b,
                   '<<': lambda a, b: a << b, '>>': lambda a, b: a >> b, '&': lambda a, b: a & b,
                   '|': lambda a, b: a | b, '^': lambda a, b: a ^ b,
                   '&&': lambda a, b: int(bool(a) and bool(b)), '||': lambda a, b: int(bool(a) or bool(b))}
            op = ops.get(n.get('opcode'))
            if op is None:
                return None
            a, b = self.const(kids(n)[0]), self.const(kids(n)[1])
            return None if a is None or b is None else op(a, b)
        return None

    def const_var(self, vid):
        """A static const variable of this function or file: (its decl, its InitListExpr)."""
        stack = [self.fn.body] + [x for x in self.fn.root.get('inner', []) if x.get('kind') == 'VarDecl']
        while stack:
            x = stack.pop()
            if x.get('kind') == 'VarDecl' and x.get('id') == vid:
                if 'const' not in x.get('type', {}).get('qualType', ''):
                    return None, None
                init = next((c for c in kids(x) if c.get('kind') == 'InitListExpr'), None)
                return x, init
            stack += kids(x)
        return None, None

    def struct_value(self, init, ty):
        names = record_fields(self.fn.root, ty)
        vals = {}
        for f, v in zip(names, kids(init)):
            c = self.const(v)
            v = bare(v)
            if c is not None:
                vals[f] = c
            elif v.get('kind') == 'DeclRefExpr' and v.get('referencedDecl', {}).get('kind') == 'FunctionDecl':
                vals[f] = ('fn', v['referencedDecl']['name'])
        return ('struct', tuple(sorted(vals.items())))

    def struct_const(self, b):
        """&v or &a[k] for a static const struct (array) with numbers and
        functions for fields: ('struct', {field: value})."""
        if not (b.get('kind') == 'UnaryOperator' and b.get('opcode') == '&'):
            return None
        d = bare(kids(b)[0])
        idx = None
        if d.get('kind') == 'ArraySubscriptExpr':
            idx = self.const(kids(d)[1])
            if idx is None:
                return None
            d = bare(kids(d)[0])
        if d.get('kind') != 'DeclRefExpr':
            return None
        x, init = self.const_var(d.get('referencedDecl', {}).get('id'))
        if init is None:
            return None
        ty = x.get('type', {}).get('qualType', '').replace('const ', '')
        if idx is not None:
            ty = re.sub(r'\s*\[\d*\]$', '', ty)
            els = kids(init)
            if idx >= len(els) or els[idx].get('kind') != 'InitListExpr':
                return None
            init = els[idx]
        return self.struct_value(init, ty.strip())

    def member(self, n):
        """p->field of a parameter bound to a constant struct: its value, or None."""
        n = bare(n)
        if n.get('kind') != 'MemberExpr':
            return None
        base = bare(kids(n)[0])
        if base.get('kind') == 'ArraySubscriptExpr':
            # a[k].field of a static const array of structs
            k = self.const(kids(base)[1])
            a = bare(kids(base)[0])
            if k is None or a.get('kind') != 'DeclRefExpr':
                return None
            x, init = self.const_var(a.get('referencedDecl', {}).get('id'))
            if init is None or k >= len(kids(init)) or kids(init)[k].get('kind') != 'InitListExpr':
                return None
            el = kids(init)[k]
            names = [f.get('name') for f in kids(record_of(self.fn.root, x)) if f.get('kind') == 'FieldDecl']
            vals = {}
            for f, v in zip(names, kids(el)):
                c = self.const(v)
                if c is not None:
                    vals[f] = c
            return vals.get(n.get('name'))
        v = self.env.get(base.get('referencedDecl', {}).get('name')) if base.get('kind') == 'DeclRefExpr' else None
        if not (isinstance(v, tuple) and v[0] == 'struct'):
            return None
        return dict(v[1]).get(n.get('name'))

    def is_call(self, fn, target, env=None):
        """A call in the original to target is this C call (not a call the
        helper makes itself, in line, or through a function passed to it)."""
        own = self.w.routines.get(fn.cname[4:] if fn.cname.startswith('abi_') else fn.cname)
        if own is not None:
            return own[1] == target
        passed = []
        for v in (env or {}).values():
            if isinstance(v, tuple) and v[0] == 'fn':
                passed.append(v[1])
            elif isinstance(v, tuple) and v[0] == 'struct':
                passed += [x[1] for _, x in v[1] if isinstance(x, tuple) and x[0] == 'fn']
        for cn in passed:
            if True:
                r = self.w.routines.get(cn[4:] if cn.startswith('abi_') else cn)
                if r is not None and r[1] == target:
                    return False
        # a helper without a routine of its own: the call is its when target
        # is no routine, or a routine whose C reaches this helper
        for rname, (bank, addr) in self.w.routines.items():
            if addr == target:
                rf = self.w.fns.get('abi_' + rname) or self.w.fns.get(rname) or \
                    next((f for f in self.w.fns.values() if f.cname == rname), None)
                if rf is None or (rf.name != fn.name and fn.name not in calls_in(rf, self.w.fns)):
                    return False
        # in line when it makes the call to target itself
        for name in calls_in(fn, self.w.fns):
            cn = self.w.fns[name].cname
            r = self.w.routines.get(cn[4:] if cn.startswith('abi_') else cn)
            if r is not None and r[1] == target:
                return False
        return True

    def constants(self, n, fn):
        """The callee's parameters given numbers here (or numbers known
        here), when it never changes them."""
        env = {}
        for name, a in zip(fn.params, kids(n)[1:]):
            b = bare(a)
            sv = self.struct_const(b)
            if sv is not None:
                env[name] = sv
                continue
            if b.get('kind') == 'DeclRefExpr' and b.get('referencedDecl', {}).get('kind') == 'FunctionDecl':
                env[name] = ('fn', b['referencedDecl']['name'])
                continue
            if b.get('kind') == 'DeclRefExpr' and isinstance(self.env.get(b.get('referencedDecl', {}).get('name')), tuple):
                env[name] = self.env[b['referencedDecl']['name']]
                continue
            mv = self.member(b)
            if isinstance(mv, tuple):
                env[name] = mv
                continue
            v = self.const(a)
            if v is not None and name != 'copy' and not assigned(fn, name):
                env[name] = v
        return env

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
        self.dbg(n, 'call %s at %s' % (fn.name if fn else '?', s.where()))
        """A call in the original: LINK, the jump and its delay slot, the
        callee from its target, then back at the LINK's address."""
        s = s.copy()
        if s.called is not None:
            ret, s.called = s.called, None       # its jump and delay slot gone through with the access there
        else:
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

    def dbg(self, n, msg):
        if DEBUG and (not self.w.probing or DEBUG2):
            o = off(n)[0] if n else 0
            print('  %s:%d %s' % (self.fn.name, self.fn.text.count('\n', 0, o or 0) + 1, msg))

    def event(self, n, s, kind, stmt):
        self.dbg(n, 'event %s at %s' % (kind, s.where()))
        s = s.copy()
        s.val = None
        if s.pend:
            self.fail(n, 'an access before the C decides the branch at %04X' % s.pend[0])
        if s.rest:
            if s.rest[0] != kind:
                self.fail(n, 'C %s, the original %s next (in the same instruction)' % (kind, s.rest[0]))
            s.rest.pop(0)
            return [s]
        if s.done:
            self.fail(n, 'an access after the return jump')
        if s.called is not None:
            self.fail(n, 'an access between a call\'s delay slot and the C\'s call')
        u = follow(s, self.fn.name)
        if u.jump and not u.ev:
            d = Unit(s.bank, u.end + 1)
            if d.ev and d.ev[0] == kind and u.jump == 'cond' and d.end == d.addr:
                return self.delay_event(n, s, u, d, kind, stmt)
            if d.ev and d.ev[0] == kind and d.end == d.addr and (u.jump == 'bra' or (u.jump == 'r15' and u.target is not None and not linked(s))):
                # a jump the original makes alone, an access in its delay slot:
                # the target is fetched before the access runs
                s.fetch(d.addr)
                s.jump_to(u.target)
                s.rest = list(d.ev[1:])
                if kind in TIME:
                    self.site(('ev', off(n)[0]), s, n, stmt)
                return [s]
            if d.ev and d.ev[0] == kind and d.end == d.addr and u.jump == 'r15' and u.target is not None and linked(s):
                # a call with an access in its delay slot (the C makes it
                # before the call): the target fetched before the access runs
                link = s.pc - 1
                s.fetch(d.addr)
                s.jump_to(u.target)
                s.rest = list(d.ev[1:])
                s.called = link + 1 + (rom(s.bank, link) & 15)
                if kind in TIME:
                    self.site(('ev', off(n)[0]), s, n, stmt)
                return [s]
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
        return [s]

    def delay_event(self, n, s, u, d, kind, stmt):
        """An access in a branch's delay slot: the branch's target (taken) or
        the byte after the slot (not) is fetched before it runs. Both ways
        go on, each to the C's arm the branch decides; the code is stated
        with the C's condition choosing the byte."""
        target = u.target
        if u.op == 0x3C:
            target = s.r13
            if target is None:
                self.fail(n, 'LOOP with R13 not known')
        s.fetch(d.addr)
        key = ('ev', off(n)[0])
        pre = s.take()
        out = []
        for way in ('taken', 'fall'):
            r = s.copy()
            if way == 'taken':
                r.out = []
                r.jump_to(target)
            else:
                r.out = []
                r._code(d.end + 1, 1)
                r.pc, r.np = d.end + 1, d.end + 2
            r.out = []                             # the byte either way is in the site's statement
            r.rest = list(d.ev[1:])
            r.pend = (u.addr, way, key)
            out.append(r)
        if not self.w.probing:
            self.fn.nodes[key[:2]] = n
            self.w.first.setdefault((self.fn.name, off(stmt)[0], self.iters), key)
            d0 = self.fn.sites.setdefault(key, {})
            k = (self.ci, self.iters)
            v = ('dual', tuple(pre), target, d.end + 1)
            if k in d0 and d0[k] != v:
                self.fail(n, 'the code at a delay slot differs between paths')
            d0[k] = v
        return out

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
        cv = self.const(b)
        if cv is not None:                       # the C knows it here: no branch of the original's
            return (list(states), []) if cv else ([], list(states))
        sts = self.expr(b, states, stmt)
        t, f = [], []
        for s in sts:
            if s.pend:
                # the branch whose delay slot already ran: the C's test is its
                bpc, way, key = s.pend
                fake = s.copy()
                fake.pc = bpc
                ch = self.decide(b, fake, Unit(s.bank, bpc))
                if ch == 'c':
                    self.fail(b, 'the branch at %04X ran its delay slot: the C must decide it here' % bpc)
                s = s.copy()
                s.pend = None
                (t if (ch == way) else f).append(s)
                if not self.w.probing:
                    self.w.duals[(self.fn.name, key)] = (b, ch)
                continue
            if s.val is not None:
                (t if s.val else f).append(s)
                continue
            s = s.copy()
            u = follow(s, self.fn.name)
            ch = self.decide(b, s, u) if u.jump == 'cond' and not u.ev else 'c'
            self.dbg(b, 'cond at %s: %s' % (s.where(), ch))
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
            if target is None and self.loop_r13 and self.loop_r13[-1] is not None:
                target = self.loop_r13[-1]       # R13 kept (pushed and pulled) since the loop began
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
        c0 = bare(cond)
        if c0.get('kind') == 'DeclRefExpr':
            # a variable the C set from a flag: the flag it holds
            txt = self.flag_of(c0.get('referencedDecl', {}).get('id')) or txt
        first = None
        if u.op in FLAG_BRANCH:
            flag, v = FLAG_BRANCH[u.op]
            if txt == 'gsu.%s' % flag:
                first = 'taken' if v else 'fall'
            elif txt == '!gsu.%s' % flag:
                first = 'fall' if v else 'taken'
        if u.op == 0x3C and txt.startswith('gsu_count_down('):
            first = 'taken'
        if first:
            opts = [first]
        elif u.op == 0x3C:
            opts = ['taken', 'fall', 'c']      # LOOP: the C's test is mostly 'go again'
        else:
            opts = ['fall', 'taken', 'c']
        self.w.choice[key] = opts[0]
        self.w.order.append((key, opts))
        return opts[0]

    def flag_of(self, vid):
        """The flag expression ('gsu.s', '!gsu.z', ...) a variable is set from
        and only from, else None."""
        found = set()
        stack = [self.fn.body]
        while stack:
            x = stack.pop()
            if x.get('kind') == 'BinaryOperator' and x.get('opcode') == '=':
                lhs = bare(kids(x)[0])
                if lhs.get('kind') == 'DeclRefExpr' and lhs.get('referencedDecl', {}).get('id') == vid:
                    b, e = span(kids(x)[1])
                    found.add(self.fn.text[b:e].replace(' ', ''))
            elif x.get('kind') == 'VarDecl' and x.get('id') == vid and kids(x):
                b, e = span(kids(x)[0])
                found.add(self.fn.text[b:e].replace(' ', ''))
            stack += kids(x)
        if len(found) == 1:
            f = found.pop()
            if re.match(r'^!?gsu\.(z|s|cy|ov)$', f):
                return f
        return None

    def if_(self, n, states):
        ch = kids(n)
        c, th = ch[0], ch[1]
        el = ch[2] if len(ch) > 2 else None
        t, f = self.cond(c, states, n)
        r1 = self.stmt(th, t) if t or self.goto_into(th) else none()
        r2 = self.stmt(el, f) if el is not None and (f or self.goto_into(el)) else dict(none(), next=f)
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
        done = [a[1] for a in arms if a[1].done]       # returned already: nothing to meet
        arms = [a for a in arms if not a[1].done]
        if not arms:
            return done[:1]
        if done:
            self.fail(n, 'a path that has returned meets one that has not')
        ps = [path(a[1]) for a in arms]
        meet = next((x for x in ps[0] if all(x in p for p in ps[1:])), None)
        if meet is None and all(returns(a[1]) for a in arms):
            # each path returns on its own in the original (the return copied
            # into each): each states its bytes through its return's delay
            # slot; what the C does after this, up to its return, has no access
            out = []
            for a in arms:
                key, s, node = a[:3]
                r = s.copy()
                u = follow(r, self.fn.name)
                r.fetch(u.addr if u.stop else u.end + 1)
                r.done = True
                self.arm_site(a, key, r, node)
                r.val = None
                out.append(r)
            return out[:1]
        if meet is None:
            self.fail(n, 'the paths do not meet in the original (%s)' % ', '.join(a[1].where() for a in arms))
        out = []
        for a in arms:
            key, s, node = a[:3]
            r = reach(s, meet)
            r.val = s.val
            self.arm_site(a, key, r, node)
            out.append(r)
        if len({s.key() for s in out}) > 1:
            self.fail(n, 'the paths meet at %04X differently: %s' % (meet, ' / '.join(str(s.key()) for s in out)))
        out[0].regs = {r: v for r, v in out[0].regs.items() if all(s.regs.get(r) == v for s in out[1:])}
        vals = {s.val for s in out}
        out[0].val = vals.pop() if len(vals) == 1 else None
        return out[:1]

    def arm_site(self, a, key, r, node):
        """A joining path's site, in the passes it was in (a goto from a pass)."""
        if len(a) > 3:
            saved, self.iters = self.iters, a[3]
            try:
                self.site(key, r, node)
            finally:
                self.iters = saved
        else:
            self.site(key, r, node)

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
        if k == 'DoStmt' and cond is not None and self.const(cond) == 0:
            # do { ... } while (0): the body once
            r = self.stmt(body, states)
            res = dict(none(), next=r['next'] + [s for s, _ in r['break']])
            res['return'], res['goto'] = r['return'], r['goto']
            if r['continue']:
                self.fail(n, 'continue in do ... while (0)')
            if len(res['next']) > 1:
                res['next'] = self.join([(('exit', off(n)[0]), s, n) for s in res['next']], n)
            return res
        sts = self.stmt(init, states)['next'] if init else states
        cnt = counted(init, cond, inc, self.const) if k == 'ForStmt' else None
        if cnt:
            self.c_only.add(off(cond)[0])
            if cnt[2] == 0:                      # no passes at all
                return self.passes(n, cnt, body, sts)
        # the head: where the entry and the way back meet, found by a pass
        # from the entry; then the loop from the head itself
        saved = snapshot(self.w)
        pend = {k_: list(v) for k_, v in self.pending.items()}
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
            self.pending = pend
        if head is None:
            return self.passes(n, cnt, body, sts)
        entry = [reach(s, head) for s in sts]
        if None in entry:
            self.fail(n, 'the entry does not reach the loop\'s head %04X' % head)
        for s in entry:
            self.site(('pre', off(n)[0]), s, n)
        if len({s.key() for s in entry}) > 1:
            self.fail(n, 'entered differently')
        if not entry:
            # entered only by a goto into it: the way back still starts at the head
            bank = next(v for v in self.pending.values() if v)[0][0].bank if any(self.pending.values()) else None
            if bank is None:
                self.fail(n, 'the loop is not entered')
            entry = [St(bank, head)]
        return self.loop_pass(n, k, cond, inc, body, entry[:1], head)

    def loop_pass(self, n, k, cond, inc, body, sts, head=None):
        r13 = None
        if sts:
            a = sts[0].copy()
            try:
                follow(a, self.fn.name)            # R13 as set on the way in (MOVE R13,R15 before the head)
            except Mismatch:
                pass
            r13 = a.r13 if a.r13 is not None else sts[0].r13
        self.loop_r13.append(r13)
        try:
            return self.loop_pass_(n, k, cond, inc, body, sts, head)
        finally:
            self.loop_r13.pop()

    def loop_pass_(self, n, k, cond, inc, body, sts, head=None):
        """A pass of the loop from sts: without a head, the pc where the entry
        and every way back meet; with one, the loop's outcome."""
        if k != 'DoStmt' and cond is not None:
            t, f = self.cond(cond, sts, n)
        else:
            t, f = sts, []
        r = self.stmt(body, t) if t or self.goto_into(body) else none()
        nxt = r['next']
        conts = r['continue']
        if inc is not None and (nxt or conts):
            # the ways to the step meet first (each its own code), then the step
            arms = [(('back', off(n)[0]), s, body) for s in nxt] + [(('cont', off(c)[0]), s, c) for s, c in conts]
            joined = self.join(arms, n)
            nxt = self.expr(inc, joined, n)
            conts = []
            for s in nxt:
                s.val = None
            inc_back = True
        else:
            inc_back = False
        if k == 'DoStmt' and cond is not None:
            t2, f2 = self.cond(cond, nxt + [s for s, _ in conts], n) if (nxt or conts) else ([], [])
            back = [(s, ('back', off(n)[0]), n) for s in t2]
            f = f + f2
        elif inc_back:
            back = [(s, ('incend', off(inc)[0]), inc) for s in nxt]
        else:
            back = [(s, ('back', off(n)[0]), body) for s in nxt] + [(s, ('cont', off(c)[0]), c) for s, c in conts]
        if head is None:
            ps = [path(s) for s in sts + [b[0] for b in back]]
            if not ps:
                self.fail(n, 'the loop is not entered')
            meet = next((x for x in ps[0] if all(x in p for p in ps[1:])), None)
            if meet is None or not back:
                self.fail(n, 'the loop does not come back in the original (%s)' % ', '.join(b[0].where() for b in back))
            return meet
        for s, key, node in back:
            r2 = reach(s, head)
            if r2 is None:
                self.fail(node, 'the way back does not reach the loop\'s head %04X (%s)' % (head, s.where()))
            self.site(key, r2, node)
            if r2.key() != sts[0].key():
                self.fail(node, 'the way back to the loop\'s head %04X differs (%s)' % (head, s.where()))
        exits = [(('exit', off(n)[0]), s, n) for s in f] + [(('brk', off(c)[0]), s, c) for s, c in r['break']]
        res = dict(none(), next=self.join(exits, n) if exits else [])
        res['return'], res['goto'] = r['return'], r['goto']
        return res

    def passes(self, n, cnt, body, sts):
        """A loop the C counts and the original lays out pass by pass: each
        pass walked in turn, its code its own."""
        var, first, count, step = cnt
        old = self.w.passes.get((self.fn.name, off(n)[0]))
        self.w.passes[(self.fn.name, off(n)[0])] = (var, first, step, max(count, old[3] if old else 0))
        res = none()
        cur = sts
        breaks = []
        had = self.env.get(var)
        for i in range(count):
            self.iters = self.iters + ((off(n)[0], i),)
            self.env[var] = first + i * step          # the pass's count, known to the walk
            try:
                r = self.stmt(body, cur) if cur or self.goto_into(body) else none()
            finally:
                self.iters = self.iters[:-1]
            if r['continue']:
                self.fail(n, 'continue in a loop laid out pass by pass')
            breaks += [(('brk', off(c)[0]), s, c) for s, c in r['break']]
            res['return'] += r['return']
            res['goto'] += r['goto']
            cur = r['next']
        if had is None:
            self.env.pop(var, None)
        else:
            self.env[var] = had
        exits = breaks + [(('exit', off(n)[0]), s, n) for s in cur]
        res['next'] = self.join(exits, n) if exits else []
        return res


def record_fields(root, ty):
    """The field names, in order, of the struct typedef'd as ty."""
    rid = None
    for x in root.get('inner', []):
        if x.get('kind') == 'TypedefDecl' and x.get('name') == ty:
            for c in kids(x):
                for cc in [c] + kids(c):
                    if cc.get('kind') in ('ElaboratedType', 'RecordType'):
                        d = cc.get('decl') or (kids(cc)[0].get('decl') if kids(cc) else None)
                        if d:
                            rid = d.get('id')
    for x in root.get('inner', []):
        if x.get('kind') == 'RecordDecl' and (x.get('id') == rid or x.get('name') == ty):
            return [f.get('name') for f in kids(x) if f.get('kind') == 'FieldDecl']
    return []


def record_of(root, var):
    """The struct declaration an array variable's elements are: declared with
    it (a struct {...} v[] = ...), or by name."""
    stack = [root]
    while stack:
        x = stack.pop()
        if x.get('kind') == 'DeclStmt' and any(c.get('id') == var.get('id') for c in kids(x)):
            for c in kids(x):
                if c.get('kind') == 'RecordDecl':
                    return c
        stack += kids(x)
    m = re.search(r'struct (\w+)', var.get('type', {}).get('qualType', ''))
    if m:
        for x in root.get('inner', []):
            if x.get('kind') == 'RecordDecl' and x.get('name') == m.group(1) and kids(x):
                return x
    return {}


_calls_in = {}


def calls_in(fn, fns):
    """The functions fn's C calls, through its in-line helpers."""
    ck = (id(fns), fn.name)
    if ck in _calls_in:
        return _calls_in[ck]
    _calls_in[ck] = set()
    out = set()
    stack = [fn.body]
    while stack:
        n = stack.pop()
        if n.get('kind') == 'CallExpr':
            f = resolve(fns, callee(n), fn.path)
            if f is not None:
                out.add(f.name)
                out |= calls_in(f, fns)
        stack += kids(n)
    _calls_in[ck] = out
    return out


def loop_writes(fn, vid):
    """The variable vid is written in a loop, or stepped (++, --, op=, &)."""
    def walk(n, inloop):
        k = n.get('kind')
        if k in ('BinaryOperator', 'CompoundAssignOperator') and (k == 'CompoundAssignOperator' or n.get('opcode') == '='):
            if bare(kids(n)[0]).get('referencedDecl', {}).get('id') == vid and (inloop or k == 'CompoundAssignOperator'):
                return True
        if k == 'UnaryOperator' and n.get('opcode') in ('++', '--', '&'):
            if bare(kids(n)[0]).get('referencedDecl', {}).get('id') == vid:
                return True
        sub = inloop or k in ('ForStmt', 'WhileStmt', 'DoStmt')
        return any(walk(c, sub) for c in kids(n))
    return walk(fn.body, False)


def writes(fn, vid):
    """How many times the function writes its variable vid (=, op=, ++, --, &)."""
    n_ = 0
    stack = [fn.body]
    while stack:
        n = stack.pop()
        k = n.get('kind')
        if k in ('BinaryOperator', 'CompoundAssignOperator') and (k == 'CompoundAssignOperator' or n.get('opcode') == '='):
            if bare(kids(n)[0]).get('referencedDecl', {}).get('id') == vid:
                n_ += 1
        if k == 'UnaryOperator' and n.get('opcode') in ('++', '--', '&', 'postfix ++'):
            if bare(kids(n)[0]).get('referencedDecl', {}).get('id') == vid:
                n_ += 2
        stack += kids(n)
    return n_


def assigned(fn, name):
    """The function changes its variable name (=, op=, ++, --, or its address taken)."""
    stack = [fn.body]
    while stack:
        n = stack.pop()
        k = n.get('kind')
        if k in ('BinaryOperator', 'CompoundAssignOperator') and (k == 'CompoundAssignOperator' or n.get('opcode') == '='):
            lhs = bare(kids(n)[0])
            if lhs.get('referencedDecl', {}).get('name') == name:
                return True
        if k == 'UnaryOperator' and n.get('opcode') in ('++', '--', '&'):
            x = bare(kids(n)[0])
            if x.get('referencedDecl', {}).get('name') == name:
                return True
        stack += kids(n)
    return False


def label_name(fn, g):
    TEXT['cur'] = fn.text
    b, e = span(g)
    m = re.match(r'goto\s+(\w+)', fn.text[b:e])
    return m.group(1) if m else None


def super_fx(fn):
    """The C of a Super FX routine (not the S-CPU's): it works on the Super
    FX's registers."""
    TEXT['cur'] = fn.text
    b, e = span(fn.body)
    return bool(re.search(r'gsu\.r\[|\bFxRegs\b|\bR\[|\bgsu_(?:ldw|stw|ldb|stb|sbk|getb|r14|romb|plot_r1|push|pull)\(',
                          fn.text[b:e]))


def opaque(fn):
    """An entry to a program's C (gsucode.py's): not walked here."""
    TEXT['cur'] = fn.text
    b, e = span(fn.body)
    return any(re.search(r'\b%s\(' % p, fn.text[b:e]) for p in PROGRAMS)


# ---- the files ----

def load(paths):
    fns, routines = {}, {}
    for p in paths:
        text = open(p).read()
        for m in re.finditer(r'@routine ([0-9A-F]{2}):([0-9A-F]{4}) (\w+)', text):
            routines[m.group(3)] = (int(m.group(1), 16), int(m.group(2), 16))
        root = ast(p)
        for node in root.get('inner', []):
            if node.get('kind') != 'FunctionDecl' or not any(c.get('kind') == 'CompoundStmt' for c in kids(node)):
                continue
            if node.get('loc', {}).get('includedFrom') or node['name'] in PROGRAMS:
                continue
            key = node['name']
            if node.get('storageClass') == 'static':
                key += '@' + tree_rel(p)
            fns[key] = Fn(key, p, node, text)
            fns[key].root = root
    return fns, routines


def tree_rel(p):
    """A file's path within the decomp's tree (src/stuntracefx or a copy of it)."""
    p = os.path.abspath(p)
    for top in ('audio', 'data', 'gsu', 'hud', 'include', 'input', 'math', 'menu', 'object', 'race', 'render',
                'save', 'system', 'video'):
        i = p.rfind(os.sep + top + os.sep)
        if i >= 0:
            return p[i + 1:]
    return os.path.basename(p)


def resolve(fns, name, path_):
    """The function a call by name reaches from a file: its own static one, or the one outside."""
    if name is None:
        return None
    return fns.get(name + '@' + tree_rel(path_)) or fns.get(name)


def kinds():
    """routine (bank, addr) -> how the original enters it: 'started' by the
    S-CPU, a 'call' from Super FX code, a 'table' entry."""
    out = {}
    with open(os.path.join(ROOT, 'build', 'stuntracefx', 'routines.csv')) as f:
        for line in f.read().split('\n')[1:]:
            c = line.split(',')
            if len(c) > 2 and c[1] == 'gsu':
                out[(int(c[0][:2], 16), int(c[0][2:], 16))] = c[2]
    return out


def entries(fns, routines):
    """The routines walked from their own entry: all but a routine only ever
    called by Super FX code whose C nothing runs (its callers' walks cover
    it where it runs)."""
    out = []
    kind = kinds()
    used = None
    for name, (bank, addr) in sorted(routines.items(), key=lambda kv: kv[1]):
        fn = fns.get('abi_' + name) or fns.get(name) or next((f for f in fns.values() if f.cname == name), None)
        if fn is None or opaque(fn):
            continue
        if not (super_fx(fn) or any(super_fx(fns[c]) for c in calls_in(fn, fns))):
            continue
        if kind.get((bank, addr)) == 'call':
            if used is None:
                # every name a function body uses (an indented line, the
                # comments taken out: a call or a function pointer), the
                # interpreted programs' too
                used = {}
                for text in {f.text for f in fns.values()}:
                    text = re.sub(r'/\*.*?\*/|//[^\n]*', '', text, flags=re.S)
                    cur = None
                    for line in text.split('\n'):
                        m = re.match(r'\w[\w \t\*]*?\b(\w+)\s*\(', line)
                        if m:
                            cur = m.group(1)
                        elif line[:1] in (' ', '\t'):
                            for c in re.findall(r'\b(\w+)\b', line):
                                used.setdefault(c, set()).add(cur)
            own = {'abi_' + name, name, fn.name}
            if not any(used.get(x, set()) - own for x in own):
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
        best = -1
        for attempt in range(400):
            saved = snapshot(w)
            w.progress = 0
            try:
                w.walk_fn(fn, St(bank, addr), 'call')
                break
            except Mismatch as e:
                restore(w, saved)
                if w.progress > best:
                    best, first = w.progress, str(e)
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
                c = resolve(fns, callee(n), f.path)
                if c is not None:
                    out.setdefault(c.name, set()).add(f.name)
            stack += kids(n)
    return out


def reachable(fns, routines):
    """The functions the routines' C can run."""
    calls = {}
    for c, fs in callers(fns).items():
        for f in fs:
            calls.setdefault(f, set()).add(c)
    seen = set()
    todo = [fn.name for fn, _, _ in entries(fns, routines)]
    while todo:
        n = todo.pop()
        if n in seen:
            continue
        seen.add(n)
        todo += calls.get(n, ())
    return seen


def rewritable(fns, routines):
    """The walked functions whose callers that run are all walked too (a
    caller left as it is passes the copies as they were numbered)."""
    r = {n for n, f in fns.items() if f.keys}
    cl = callers(fns)
    live = reachable(fns, routines)
    while True:
        drop = {n for n in r if any(c not in r and c in live for c in cl.get(n, ()))}
        if not drop:
            return r
        r -= drop


def unused_tables(text):
    while True:
        changed = False
        for m in re.finditer(r'/\* cyccopy: [^\n]*\*/\nstatic const (?:GsuCode|uint8_t|uint16_t) (\w+)\[[^;]*;\n\n', text):
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
    keep = rewritable(fns, routines)
    tmp = out_dir + '.strip'
    shutil.rmtree(tmp, ignore_errors=True)
    shutil.copytree(DECOMP, tmp)
    by_file = {}
    for n in keep:
        by_file.setdefault(fns[n].path, []).append(fns[n].node)
    for p, nodes in by_file.items():
        q = os.path.join(tmp, os.path.relpath(p, DECOMP))
        text = strip_code(open(p).read(), nodes)
        # the tables this tool wrote before, for the functions written again
        names = {fns[n].cname for n in keep if fns[n].path == p}
        text = re.sub(r'/\* gsuwalk: the code of each copy of (\w+), read from the ROM \*/\nstatic const GsuCode \w+\[[^;]*;\n\n',
                      lambda m: '' if m.group(1) in names else m.group(0), text)
        gone = set()

        def drop(m):
            if m.group(1) not in names:
                return m.group(0)
            gone.add(m.group(2))
            return ''
        text = re.sub(r"/\* gsuwalk: the copy of \w+ each copy of (\w+) calls(?:, by pass)? \*/\nstatic const uint(?:8|16)_t (\w+)\[[^;]*;\n\n",
                      drop, text)
        # and the copies passed from them: numbered again below
        for m in reversed(list(re.finditer(r'GSU_COPY\((\w+)\[', text))):
            if m.group(1) in gone:
                text = text[:m.start()] + 'GSU_COPY(0)' + text[cycwrite_close(text, m.start() + len('GSU_COPY(')) + 1:]
        open(q, 'w').write(text)
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


def copy_type(top):
    """The type of a table of copy numbers up to top."""
    return 'uint8_t' if top < 256 else 'uint16_t'


def strip_code(text, fn_nodes):
    """The text without the code statements in the functions given (their
    syntax trees): every gsu_code, gsu_cache, gsu_codes and their _then and
    _loop forms, and else arms left with nothing in them."""
    edits = []
    TEXT['cur'] = text

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
        if k == 'LabelStmt' and kids(n) and is_code_call(bare(kids(n)[0])):
            c = kids(n)[0]
            edits.append((span(c)[0], stmt_end(text, c), ';'))
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

    def literal(self, code, bank, form, x=None):
        items = []
        for c in code:
            if c[0] == 'cache':
                items.append(('gsu_cache', '0x%04x' % c[1]))
            else:
                items.append(('gsu_code', '0x%02x%04x, %d' % (bank, c[0], c[1])))
        if form == 'stmt':
            return ' '.join('%s(%s);' % it for it in items)
        if form == 'expr':
            return ', '.join('%s(%s)' % it for it in items)
        if form == 'loop':
            for name, args in items:              # the first stated innermost: it runs first
                x = '%s_loop(%s, %s)' % (name, args, x)
            return x
        for name, args in items:                  # the first stated innermost: it runs first
            x = '%s_then(%s, %s)' % (name, args, x)
        return x

    def run(self):
        self.handled = set()
        for fn in self.fns.values():
            if fn.keys and fn.sites:
                self.function(fn)
        self.copies()
        # calls the walk never reached, to a function given a copy parameter
        for f in self.fns.values():
            TEXT['cur'] = f.text
            stack = [f.body]
            while stack:
                n = stack.pop()
                if n.get('kind') == 'CallExpr':
                    c = resolve(self.fns, callee(n), f.path)
                    if c is not None and getattr(c, 'added_copy', False) and (f.path, span(n)[0]) not in self.handled:
                        e = span(n)[1] - 1
                        self.edit(f, e, e, (', ' if kids(n)[1:] else '') + 'GSU_COPY(0)')
                stack += kids(n)

    def function(self, fn):
        text = fn.text
        TEXT['cur'] = text
        par = parents(fn.node)
        nc = len(fn.keys)
        sites = sorted(fn.sites.items(), key=lambda kv: (kv[0][1], kv[0][0]))
        table = []               # slots: [(site, [rows])]
        slot = 0
        plan = []
        for key, d in sites:
            if not any(d.values()):
                continue
            loops = sorted({lo for (_, it) in d for lo, _ in it})
            counts = [self.w.passes[(fn.name, L)][3] for L in loops]     # every pass, walked or not
            nb = 1
            for c in counts:
                nb *= c
            if any(v and v[0] == 'dual' for v in d.values()):
                if loops:
                    raise Fail('%s: a delay slot\'s code in a loop walked by passes' % fn.name)
                cond = self.w.duals.get((fn.name, key))
                if cond is None:
                    raise Fail('%s: a delay slot\'s branch the C never decides' % fn.name)
                b, e = span(cond[0])
                ctext = fn.text[b:e]
                if nc == 1:
                    _, pre, tk, fl = d[(0, ())]
                    plan.append((key, ('dual', list(pre), fn.keys[0][0], ctext, cond[1], tk, fl)))
                    continue
                # each copy's bytes before the slot, then the byte after it
                # when the C's condition holds and when it does not
                if not all(v and v[0] == 'dual' for v in d.values()):
                    raise Fail('%s: a delay slot in some copies only' % fn.name)
                kp = max(len(v[1]) for v in d.values())
                rows = [[(0, 0)] * (kp + 2) for _ in range(nc)]
                for (ci, _), (_, pre, tk, fl) in d.items():
                    bank = fn.keys[ci][0]
                    ent = [(c[1], 0) if c[0] == 'cache' else ((bank << 16) | c[0], c[1]) for c in pre]
                    yes, no = (tk, fl) if cond[1] == 'taken' else (fl, tk)
                    rows[ci][:len(ent)] = ent
                    rows[ci][kp:] = [((bank << 16) | yes, 1), ((bank << 16) | no, 1)]
                plan.append((key, ('dualtab', slot, kp, ctext)))
                table.append(rows)
                slot += kp + 2
                continue
            if nc == 1 and not loops:
                plan.append((key, ('lit', d[(0, ())], fn.keys[0][0])))
                continue
            k = max(len(v) for v in d.values())
            rows = [[(0, 0)] * (k * nb) for _ in range(nc)]
            for (ci, it), v in d.items():
                idx = 0
                for L, c in zip(loops, counts):
                    i = dict(it)[L]
                    idx = idx * c + i
                bank = fn.keys[ci][0]
                ent = [(c[1], 0) if c[0] == 'cache' else ((bank << 16) | c[0], c[1]) for c in v]
                rows[ci][idx * k: idx * k + len(ent)] = ent
            # the passes' index
            idx = None
            for L, c in zip(loops, counts):
                var, first, step, _ = self.w.passes[(fn.name, L)]
                if step > 0:
                    term = '(%s - %d)' % (var, first) if first else var
                else:
                    term = '(%d - %s)' % (first, var)
                idx = term if idx is None else '(%s) * %d + %s' % (idx, c, term)
            plan.append((key, ('tab', slot, k, idx)))
            table.append(rows)
            slot += k * nb
        if table and nc > 1 and 'copy' not in fn.params:
            self.add_copy_param(fn)
        name = '%s_fetch' % fn.cname
        if table:
            width = slot
            rows = []
            for ci in range(nc):
                r = []
                for t in table:
                    r += t[ci]
                rows.append('    {%s},' % ', '.join('{0x%06x, %d}' % e if e[1] else ('{0x%04x, 0}' % e[0] if e[0] else '{0, 0}') for e in r))
            decl = '/* gsuwalk: the code of each copy of %s, read from the ROM */\nstatic const GsuCode %s[%d][%d] = {\n%s\n};\n\n' % (
                fn.cname, name, nc, width, '\n'.join(rows))
            self.edit(fn, span(fn.node)[0], span(fn.node)[0], decl)
        row = '%s[%s]' % (name, 'copy' if nc > 1 else '0')
        for key, how in plan:
            self.place(fn, par, key, how, row)

    def add_copy_param(self, fn):
        """Give fn an int copy parameter (its definition and its declarations)."""
        for node in [fn.node] + [x for x in fn.root.get('inner', []) if x.get('kind') == 'FunctionDecl'
                                 and x.get('name') == fn.cname and x is not fn.node
                                 and not any(c.get('kind') == 'CompoundStmt' for c in kids(x))]:
            if node.get('loc', {}).get('includedFrom'):
                continue
            ps = [c for c in kids(node) if c.get('kind') == 'ParmVarDecl']
            if ps:
                e = span(ps[-1])[1]
                self.edit(fn, e, e, ', int copy')
            else:
                b = span(node)[0]
                i = fn.text.index('(', b + len(fn.cname))
                j = fn.text.index(')', i)
                self.edit(fn, i + 1, j, 'int copy')
        fn.params.append('copy')
        fn.added_copy = True

    def code(self, how, row, form, x=None):
        if how[0] == 'lit':
            return self.literal(how[1], how[2], form, x)
        if how[0] == 'dual':
            _, pre, bank, ctext, ch, tk, fl = how
            a, b = (tk, fl) if ch == 'taken' else (fl, tk)
            last = 'gsu_code((%s) ? 0x%02x%04x : 0x%02x%04x, 1)' % (ctext, bank, a, bank, b)
            if form == 'stmt':
                return (self.literal(pre, bank, 'stmt') + ' ' if pre else '') + last + ';   /* the branch\'s delay slot */'
            if form == 'expr':
                return (self.literal(pre, bank, 'expr') + ', ' if pre else '') + last
            raise Fail('a delay slot\'s code after a value')
        if how[0] == 'dualtab':
            _, s, kp, ctext = how
            at = '%s + %d' % (row, s) if s else row
            parts = (['gsu_codes(%s, %d)' % (at, kp)] if kp else []) + \
                ['gsu_codes(%s + %d + !(%s), 1)' % (row, s + kp, ctext)]
            if form == 'stmt':
                return '; '.join(parts) + ';   /* the branch\'s delay slot */'
            if form == 'expr':
                return ', '.join(parts)
            raise Fail('a delay slot\'s code after a value')
        _, s, k, idx = how
        at = '%s + %d' % (row, s) if s else row
        if idx:
            at += ' + %d * %s' % (k, idx) if k > 1 else ' + %s' % idx
        if form == 'stmt':
            return 'gsu_codes(%s, %d);' % (at, k)
        if form == 'expr':
            return 'gsu_codes(%s, %d)' % (at, k)
        if form == 'loop':
            return 'gsu_codes_loop(%s, %d, %s)' % (at, k, x)
        return 'gsu_codes_then(%s, %d, %s)' % (at, k, x)

    def before_stmt(self, fn, par, st, how, row):
        text = fn.text
        p = par.get(id(st), {})
        b = span(st)[0]
        if p.get('kind') in ('CompoundStmt', 'LabelStmt'):
            self.edit(fn, b, b, self.code(how, row, 'stmt') + '\n' + indent_at(text, b), 1)
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
        node = fn.nodes.get(key[:2])
        if kind == 'ev' and node.get('kind') == 'CallExpr' and self.timed_arg(fn, node) is not None:
            # an access whose arguments run code of their own: its code
            # stated once they have (around the last of them)
            a = self.timed_arg(fn, node)
            b, e = span(a)
            pre, post = self.code(how, row, 'then', '\0').split('\0')
            self.edit(fn, b, b, pre)
            self.edit(fn, e, e, post)
            return
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
                # the return's bytes after its value is made: kept in a
                # variable while they are stated
                e = kids(node)[0]
                ty = e.get('type', {}).get('qualType', 'uint16_t')
                b = span(node)[0]
                en = stmt_end(text, node)
                eb, ee = span(e)
                self.edit(fn, b, eb, '{ %s ret_ = ' % ty, 0)
                self.edit(fn, ee, en, '; %s return ret_; }' % self.code(how, row, 'stmt'), 7)
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
            if node.get('kind') == 'DoStmt':
                # stated in the test, when it goes round again
                c = kids(node)[1]
                b, e = span(c)
                pre, post = self.code(how, row, 'loop', '\0').split('\0')
                self.edit(fn, b, b, pre)
                self.edit(fn, e, e, post)
            else:
                self.at_end(fn, par, node, how, row)
        elif kind == 'exit':
            # after the loop: only the loop's own ending gets there
            loop = node
            def own(k):
                # the break's loop: the innermost one around it
                x = par.get(id(fn.nodes.get(k[:2])))
                while x is not None and x.get('kind') not in ('ForStmt', 'WhileStmt', 'DoStmt', 'SwitchStmt'):
                    x = par.get(id(x))
                return x is loop
            if any(k[0] == 'brk' and own(k) for k in fn.sites if self.inside(loop, k[1])):
                raise Fail('%s: code at a loop\'s end that breaks too' % fn.name)
            e = stmt_end(text, loop)
            self.edit(fn, e, e, '\n' + indent_at(text, span(loop)[0]) + self.code(how, row, 'stmt'))
        elif kind == 'incend':
            e = span(node)[1]
            self.edit(fn, e, e, ', ' + self.code(how, row, 'expr'), 8)   # after any wrap that closes there
        elif kind == 'qarm':
            b, e = span(node)
            self.edit(fn, b, b, '(' + self.code(how, row, 'expr') + ', ')
            self.edit(fn, e, e, ')')
        else:
            raise Fail('%s: a site %s' % (fn.name, kind))

    def timed_arg(self, fn, call):
        """The last argument of a call that runs timed code itself (an
        access, or a function the walk gave code)."""
        last = None
        for a in kids(call)[1:]:
            stack = [a]
            while stack:
                x = stack.pop()
                if x.get('kind') == 'CallExpr':
                    name = callee(x)
                    f = resolve(self.fns, name, fn.path) if name else None
                    if name in C_EV or (f is not None and (f.keys or f.sites)):
                        last = a
                        break
                stack += kids(x)
        return last

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
            TEXT['cur'] = fn.text
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
            vals = {k: v[1] for k, v in d.items()}
            if getattr(cal, 'added_copy', False):
                arg = None
            else:
                arg = args[cal.params.index('copy')]
            if len(set(vals.values())) == 1:
                expr = str(next(iter(vals.values())))
            elif any(it for (_, it) in vals):
                # by the pass of loops laid out pass by pass: a table by copy and pass
                nc = len(fn.keys)
                loops = sorted({lo for (_, it) in vals for lo, _ in it})
                counts = [self.w.passes[(fn.name, L)][3] for L in loops]
                nb = 1
                for c in counts:
                    nb *= c
                tab = [[0] * nb for _ in range(nc)]
                for (ci, it), v in vals.items():
                    idx = 0
                    for L, c in zip(loops, counts):
                        idx = idx * c + dict(it)[L]
                    if idx >= nb:
                        raise Fail('%s: a call in pass %s beyond the loop\'s %s passes' % (caller, it, counts))
                    tab[ci][idx] = v
                idx = None
                for L, c in zip(loops, counts):
                    var, first, step, _ = self.w.passes[(fn.name, L)]
                    term = ('(%s - %d)' % (var, first) if first else var) if step > 0 else '(%d - %s)' % (first, var)
                    idx = term if idx is None else '(%s) * %d + %s' % (idx, c, term)
                tname = '%s_in_%s' % (cal.cname, fn.cname)
                made[tname] = made.get(tname, 0) + 1
                if made[tname] > 1:
                    tname += '_%d' % made[tname]
                decl = '/* gsuwalk: the copy of %s each copy of %s calls, by pass */\nstatic const %s %s[%d][%d] = {%s};\n\n' % (
                    cal.cname, fn.cname, copy_type(max(max(r) for r in tab)), tname, nc, nb,
                    ', '.join('{%s}' % ', '.join(str(x) for x in r) for r in tab))
                self.edit(fn, span(fn.node)[0], span(fn.node)[0], decl)
                expr = '%s[%s][%s]' % (tname, 'copy' if nc > 1 else '0', idx)
            else:
                nc = len(fn.keys)
                if False:
                    pass
                tname = '%s_in_%s' % (cal.cname, fn.cname)
                made[tname] = made.get(tname, 0) + 1
                if made[tname] > 1:
                    tname += '_%d' % made[tname]
                tab = [vals.get((ci, ()), 0) for ci in range(nc)]
                decl = '/* gsuwalk: the copy of %s each copy of %s calls */\nstatic const %s %s[%d] = {%s};\n\n' % (
                    cal.cname, fn.cname, copy_type(max(tab)), tname, nc, ', '.join(str(v) for v in tab))
                self.edit(fn, span(fn.node)[0], span(fn.node)[0], decl)
                expr = '%s[copy]' % tname
            if arg is None:
                e = span(node)[1] - 1                       # the call's closing parenthesis
                self.edit(fn, e, e, (', ' if args else '') + 'GSU_COPY(%s)' % expr)
                self.handled.add((fn.path, span(node)[0]))
                continue
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
