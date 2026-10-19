#!/usr/bin/env python3
"""cycprobe.py: the decomp's C with a probe at the start of every block.

    cycprobe.py OUTDIR [FILE.c ...]
    cycprobe.py --lib OUTDIR LIB      then build the hybrid library from it

Writes a copy of src/stuntracefx (every .c, or the files named) into OUTDIR
with CYC_PROBE(n) inserted wherever straight-line code starts: a function's
body, each arm of an if (and an else for one that has none), each loop body, each case and default, each label
(a goto's target), the statement after any control statement (where the
paths join again; the closing brace when nothing follows it), the entry to each loop (its head is also reached from
its own end), and the right side of && and || and both sides of ?:
(the branches a condition makes without a block), and just before each
bus access (rd8, wr8: after its arguments, so the code between the access
and the probe before it is the access's own). The headers' inline
functions get theirs too. Between two probes the
C runs the same statements every time, so the time the original spends
between two of the C's bus accesses is a sum of what the probes passed
stand for: cycsolve.py finds those amounts from the lockstep run.

OUTDIR/probes.csv lists each probe: id, file, line, column, the kind of
place, its offset in the file, and for an access the function called
(with * when its last argument is not a number). The tree under src/ is not touched. The
positions come from clang's syntax tree (-ast-dump=json); code inside
macros is left as it is.
"""
import csv, glob, json, os, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SRC = os.path.join(ROOT, 'src', 'stuntracefx')
TIMING = ('cyc', 'cyc_then')      # machine.h's stated time: no access, no probes
CONTROL = {'IfStmt', 'WhileStmt', 'ForStmt', 'DoStmt', 'SwitchStmt', 'LabelStmt', 'CaseStmt', 'DefaultStmt'}


def ast(path):
    p = subprocess.run(['clang', '-x', 'c', '-Xclang', '-ast-dump=json', '-fsyntax-only', '-std=gnu99',
                        '-I', os.path.join(SRC, 'include'), path], capture_output=True, text=True)
    if p.returncode:
        raise SystemExit('cycprobe: %s does not parse:\n%s' % (path, p.stderr[-2000:]))
    return json.loads(p.stdout)


def callee(node):
    """The name a call's callee expression refers to directly, if any."""
    while node.get('kind') in ('ImplicitCastExpr', 'ParenExpr') and node.get('inner'):
        node = node['inner'][0]
    if node.get('kind') == 'DeclRefExpr':
        return node.get('referencedDecl', {}).get('name')


def last_param(node):
    """The type of a directly called function's last parameter, from the
    callee's type ('uint8_t (uint32_t)'); None for a call through a pointer,
    no parameters or a variable list."""
    while node.get('kind') in ('ImplicitCastExpr', 'ParenExpr') and node.get('inner'):
        node = node['inner'][0]
    if node.get('kind') != 'DeclRefExpr':
        return None
    t = node.get('type', {}).get('qualType', '')
    if not t.endswith(')'):
        return None
    depth, i = 0, len(t) - 1
    while i >= 0:
        depth += {')': 1, '(': -1}.get(t[i], 0)
        if depth == 0:
            break
        i -= 1
    params, depth, cur = [], 0, ''
    for ch in t[i + 1:-1]:
        depth += {'(': 1, ')': -1}.get(ch, 0)
        if ch == ',' and depth == 0:
            params.append(cur.strip()); cur = ''
        else:
            cur += ch
    params.append(cur.strip())
    last = params[-1]
    if last in ('', 'void', '...') or '(' in last:
        return None
    return last


def number_type(t):
    """A parameter type a number passes through unchanged (cyc_then's)."""
    return bool(t) and '*' not in t and not t.startswith(('struct', 'const struct')) and not t[:1].isupper()


def annotate(node, state):
    """Every location dict in document order gets '_file': clang prints a
    file only where it changes from the last location printed."""
    if isinstance(node, dict):
        if 'offset' in node or 'line' in node:
            if 'file' in node:
                state[0] = node['file']
            node['_file'] = state[0]
        for v in node.values():
            annotate(v, state)
    elif isinstance(node, list):
        for v in node:
            annotate(v, state)


class Walker:
    """Collects insertions (offset, text, kind) for one file's own code."""

    def __init__(self, path, text, pure, calls=None):
        self.path = path
        self.text = text
        self.pure = pure       # functions whose calls need no probe
        self.calls = calls     # headers: each inline function's callees
        self.fn = None
        self.cur_file = None
        self.ins = []          # (offset, order, text, kind)
        # for cycwrite.py: where each probe stands, without changing what
        # is inserted. meta[i] belongs to ins[i] (a probe's own piece).
        self.meta = {}
        self.anchor = None     # the statement being walked: [start, needs braces, end, accesses so far]
        self.unsafe = 0        # inside a condition that is not always worked out where its statement starts
        self.depth = 0         # bodies wrapped so far (nesting)

    def _file_of(self, loc):
        if not loc:
            return
        if 'spellingLoc' in loc:
            loc = loc['spellingLoc']
        if 'file' in loc:
            self.cur_file = loc['file']

    def mine(self, node):
        """A node of this file, not from a macro: its begin and end offsets."""
        r = node.get('range')
        if not r:
            return None
        b, e = r.get('begin', {}), r.get('end', {})
        if 'spellingLoc' in b or 'expansionLoc' in b or 'spellingLoc' in e or 'expansionLoc' in e:
            return None
        if 'offset' not in b or 'offset' not in e:
            return None
        if os.path.abspath(b.get('_file') or '') != os.path.abspath(self.path):
            return None
        return b['offset'], e['offset'] + e.get('tokLen', 0)

    def mine_arg(self, node):
        """An argument of a call that is this file's own: where a macro
        gives (part of) it, the macro's use as written here."""
        r = node.get('range')
        if not r:
            return None
        b, e = r.get('begin', {}), r.get('end', {})
        macro_end = 'expansionLoc' in e
        b, e = b.get('expansionLoc', b), e.get('expansionLoc', e)
        if 'offset' not in b or 'offset' not in e:
            return None
        for x in (b, e):
            if os.path.abspath(x.get('_file') or '') != os.path.abspath(self.path):
                return None
        end = e['offset'] + e.get('tokLen', 0)
        if macro_end:
            # a function-like macro's use ends at its own ')': clang gives
            # its name
            j = end
            while j < len(self.text) and self.text[j] in ' \t':
                j += 1
            if j < len(self.text) and self.text[j] == '(':
                depth = 0
                while j < len(self.text):
                    depth += {'(': 1, ')': -1}.get(self.text[j], 0)
                    j += 1
                    if depth == 0:
                        break
                end = j
        return b['offset'], end

    def _meta(self, **m):
        m['fn'] = self.fn
        self.meta[len(self.ins) - 1] = m

    def _stmt_end(self, end):
        # a statement's range ends at its last token; take the ';' too
        while end < len(self.text) and self.text[end] in ' \t':
            end += 1
        if end < len(self.text) and self.text[end] == ';':
            end += 1
        return end

    def probe_before(self, node, kind):
        span = self.mine(node)
        if span:
            self.ins.append((span[0], 1, 'CYC_PROBE(@); ', kind))
            self._meta(form='stmt', at=span[0])

    def wrap(self, node, kind, close=' }', close_kind=None):
        """A body that is a single statement: { probe; it }"""
        span = self.mine(node)
        if not span:
            return
        end = self._stmt_end(span[1])
        self.ins.append((span[0], 1, '{ CYC_PROBE(@); ', kind))
        self._meta(form='wrap', at=span[0], end=end, depth=self.depth)
        self.ins.append((end, 0, close, close_kind))
        if close_kind:
            self._meta(form='skip', at=end, wrapped=(span[0], end), depth=self.depth)

    def expr_probe(self, node, kind):
        span = self.mine(node)
        if span:
            self.ins.append((span[0], 1, '(CYC_PROBE(@), ', kind))
            self._meta(form='expr', at=span[0], end=span[1])
            self.ins.append((span[1], 0, ')', None))

    def after_arg(self, call, node, typ):
        """The probe of a bus access: after its last argument is worked out
        (clang evaluates a call's arguments in order), just before the
        access, so it stands for the code between the two."""
        span = self.mine(call) and self.mine_arg(node)
        if span:
            self.ins.append((span[0], 1, '({ %s cyc_v_ = (' % typ, None))
            self.ins.append((span[1], 0, '); CYC_PROBE(@); cyc_v_; })', 'access'))
            # whether a statement before can stand for it is known once its
            # arguments are walked (walk_stmt sets 'first')
            self._meta(form='arg', at=span[0], end=span[1], typ=typ, anchor=self.anchor, unsafe=self.unsafe > 0,
                       callee=callee(call.get('inner', [{}])[0]))
            return len(self.ins) - 1

    def body(self, node, kind):
        if node is None:
            return
        if node.get('kind') == 'CompoundStmt':
            self.compound(node, kind)
        else:
            self.wrap(node, kind)

    def compound(self, node, kind):
        span = self.mine(node)
        kids = node.get('inner', [])
        if span:
            # just inside the brace: covers an empty block too
            self.ins.append((span[0] + 1, 1, ' CYC_PROBE(@);', kind))
            self._meta(form='stmt', at=span[0] + 1)
        prev = None
        for k in kids:
            self.visit_loc(k)
            if prev is not None and prev.get('kind') in CONTROL:
                self.probe_before(k, 'join')
            elif prev is not None and k.get('kind') in ('WhileStmt', 'ForStmt', 'DoStmt'):
                # a loop's head is reached from above and from its own end:
                # its entry gets a probe of its own
                self.probe_before(k, 'entry')
            prev = k
        if span and prev is not None and prev.get('kind') in CONTROL:
            # a block that ends in a control statement: its paths join
            # again at the closing brace
            self.ins.append((span[1] - 1, 1, 'CYC_PROBE(@); ', 'join'))
            self._meta(form='stmt', at=span[1] - 1)

    def visit_loc(self, node):
        self._file_of(node.get('loc'))
        self._file_of(node.get('range', {}).get('begin'))
        self.visit(node)

    def visit(self, node):
        k = node.get('kind')
        inner = node.get('inner', [])
        if k == 'FunctionDecl':
            if node.get('name') in TIMING:
                return                         # stated time, no probes of its own
            body = next((c for c in inner if c.get('kind') == 'CompoundStmt'), None)
            for c in inner:
                if c is not body:
                    self.visit_loc(c)
            if body is not None:
                self.fn = node.get('name')
                if self.calls is not None:
                    self.calls.setdefault(self.fn, set())
                self._file_of(body.get('range', {}).get('begin'))
                self.compound(body, 'function')
                for c in body.get('inner', []):
                    self.walk_kid(c)
            return
        for c in inner:
            self.visit_loc(c)

    def walk_stmt(self, node):
        """Statements and expressions inside a function body."""
        self._file_of(node.get('loc'))
        self._file_of(node.get('range', {}).get('begin'))
        k = node.get('kind')
        inner = node.get('inner', [])
        if k == 'IfStmt':
            # inner: [init?, cond, then, else?]; the AST marks hasElse
            stmts = [c for c in inner]
            has_else = node.get('hasElse', False)
            then = stmts[-2] if has_else else stmts[-1]
            els = stmts[-1] if has_else else None
            for c in stmts:
                if c is not then and c is not els:
                    self.walk_stmt(c)
            if els is None and then.get('kind') != 'CompoundStmt' and self.mine(node):
                # the path that skips the arm gets a probe of its own
                self.wrap(then, 'then', ' } else { CYC_PROBE(@); }', 'skip')
            else:
                self.body(then, 'then')
                span = self.mine(then)
                if els is None and span and self.mine(node):
                    self.ins.append((span[1], 0, ' else { CYC_PROBE(@); }', 'skip'))
                    self._meta(form='skip', at=span[1], wrapped=span, depth=self.depth)
            self.walk_children(then)
            if els is not None:
                if els.get('kind') == 'IfStmt':
                    self.walk_body(els)
                else:
                    self.body(els, 'else')
                    self.walk_children(els)
            return
        if k in ('WhileStmt', 'ForStmt'):
            body = inner[-1] if inner else None
            for i, c in enumerate(inner[:-1]):
                # a for's first part runs once, before the loop
                if k == 'ForStmt' and i == 0:
                    self.walk_stmt(c)
                else:
                    self.walk_unsafe(c)
            self.body(body, 'loop')
            self.walk_children(body)
            return
        if k == 'DoStmt':
            body = inner[0] if inner else None
            self.body(body, 'loop')
            self.walk_children(body)
            for c in inner[1:]:
                self.walk_unsafe(c)
            return
        if k in ('CaseStmt', 'DefaultStmt', 'LabelStmt'):
            sub = inner[-1] if inner else None
            if sub is not None and sub.get('kind') not in ('CaseStmt', 'DefaultStmt'):
                self.probe_before(sub, {'CaseStmt': 'case', 'DefaultStmt': 'case', 'LabelStmt': 'label'}[k])
            if sub is not None:
                if sub.get('kind') in ('CaseStmt', 'DefaultStmt'):
                    self.walk_stmt(sub)
                else:
                    self.walk_kid(sub)
            return
        if k == 'CompoundStmt':
            self.compound(node, 'block')
            for c in inner:
                self.walk_kid(c)
            return
        if k == 'CallExpr' and inner:
            name = callee(inner[0])
            if self.calls is not None and name:
                self.calls[self.fn].add(name)
            if name not in self.pure:
                args = inner[1:]
                typ = last_param(inner[0])
                for c in inner[:-1] if args else inner:
                    self.walk_stmt(c)
                if args and typ:
                    at = self.after_arg(node, args[-1], typ)
                    self.walk_stmt(args[-1])
                    if at is not None:
                        self.meta[at]['first'] = not self.anchor or self.anchor[3] == 0
                    if self.anchor:
                        self.anchor[3] += 1
                else:
                    if self.anchor:
                        self.anchor[3] += 1
                    self.expr_probe(node, 'access')
                    if self.ins and self.ins[-1][2] == ')' and len(self.ins) - 2 in self.meta:
                        self.meta[len(self.ins) - 2]['callee'] = name
                    if args:
                        self.walk_stmt(args[-1])
                return
        if k == 'BinaryOperator' and node.get('opcode') in ('&&', '||'):
            self.walk_stmt(inner[0])
            self.expr_probe(inner[1], node['opcode'])
            self.walk_unsafe(inner[1])
            return
        if k == 'ConditionalOperator':
            self.walk_stmt(inner[0])
            self.expr_probe(inner[1], '?')
            self.walk_unsafe(inner[1])
            self.expr_probe(inner[2], ':')
            self.walk_unsafe(inner[2])
            return
        for c in inner:
            self.walk_stmt(c)

    def walk_children(self, node):
        if node is None:
            return
        if node.get('kind') == 'CompoundStmt':
            for c in node.get('inner', []):
                self.walk_kid(c)
        else:
            self.walk_body(node)

    def walk_kid(self, node):
        """A statement of a block: code before it runs once, as it does."""
        span = self.mine(node)
        old = self.anchor
        self.anchor = [span[0], False, None, 0] if span else None
        self.walk_stmt(node)
        self.anchor = old

    def walk_body(self, node):
        """A body that is one statement: code before it needs braces."""
        span = self.mine(node)
        old = self.anchor
        self.anchor = [span[0], True, self._stmt_end(span[1]), 0] if span else None
        self.depth += 1
        self.walk_stmt(node)
        self.depth -= 1
        self.anchor = old

    def walk_unsafe(self, node):
        """Code not always reached where its statement starts (a loop's
        test, the right of && or ||, an arm of ?:)."""
        self.unsafe += 1
        self.walk_stmt(node)
        self.unsafe -= 1


def walk_file(path, pure, calls=None):
    """A file's insertions, in the order they go in: (index in w.ins,
    (offset, order, text, kind)); and the walker (w.meta by that index)."""
    text = open(path).read()
    w = Walker(path, text, pure, calls)
    tree = ast(path)
    annotate(tree, [None])
    for node in tree.get('inner', []):
        w.visit_loc(node)
    # every insertion kept (two closings can share an offset); openers at an
    # offset go after the closings there, in the order they were found
    return text, w, sorted(enumerate(w.ins), key=lambda it: (it[1][0], it[1][1], it[0] if it[1][1] else -it[0]))


def instrument(path, out, next_id, rows, pure, calls=None):
    text, w, order = walk_file(path, pure, calls)
    pieces, at = [], 0
    rel = os.path.relpath(path, ROOT)
    for i, (off, _order, s, kind) in order:
        pieces.append(text[at:off])
        if '@' in s:
            line = text.count('\n', 0, off) + 1
            col = off - text.rfind('\n', 0, off)
            m = w.meta.get(i, {})
            who = m.get('callee') or ''
            if who and m.get('form') == 'arg' and not number_type(m.get('typ', '')):
                who += '*'
            rows.append((next_id, rel, line, col, kind, off, who))
            s = s.replace('@', str(next_id))
            next_id += 1
        pieces.append(s)
        at = off
    pieces.append(text[at:])
    os.makedirs(os.path.dirname(out), exist_ok=True)
    with open(out, 'w') as f:
        f.write('#include "cycprobe.h"\n' + ''.join(pieces))
    return next_id


def pure_functions(heads):
    """The headers' inline functions that reach no bus access (nor any
    function outside the headers): a call to one needs no probe."""
    calls = {}
    for h in heads:
        Walker_scan(h, calls)
    pure = set(TIMING)                     # time stated, no access
    while True:
        more = {f for f, cs in calls.items() if f not in pure and cs <= pure}
        if not more:
            break
        pure |= more
    return pure


def all_probes():
    """Every file's probes as main() numbers them (the headers, then every
    .c): (path, text, [(id, kind, meta)])."""
    heads = sorted(glob.glob(os.path.join(SRC, 'include', '*.h')))
    files = sorted(glob.glob(os.path.join(SRC, '**', '*.c'), recursive=True))
    pure = pure_functions(heads)
    nid = 1
    for path in heads + files:
        text, w, order = walk_file(path, pure)
        got = []
        for i, (_off, _order, s, kind) in order:
            if '@' in s:
                got.append((nid, kind, w.meta.get(i)))
                nid += 1
        yield path, text, got


def Walker_scan(path, calls):
    """Each inline function of a header, with the functions it calls."""
    w = Walker(path, open(path).read(), set(), calls)
    tree = ast(path)
    annotate(tree, [None])
    for node in tree.get('inner', []):
        w.visit_loc(node)


def build_lib(outdir, lib):
    sys.path.insert(0, os.path.join(ROOT, 'tools', 'stuntracefx'))
    import contextlib, hybrid, io
    with contextlib.redirect_stdout(io.StringIO()):
        hybrid.main(outdir, lib)                   # its routine table
    table = os.path.join(os.path.dirname(lib), 'routines.c')
    files = sorted(glob.glob(os.path.join(outdir, '**', '*.c'), recursive=True))
    p = subprocess.run(['cc', '-O2', '-w', '-std=gnu99', '-shared', '-fPIC', '-undefined', 'dynamic_lookup',
                        '-I', os.path.join(outdir, 'include'), '-o', lib, table] + files, capture_output=True, text=True)
    if p.returncode:
        raise SystemExit('cycprobe: the library does not build:\n' + p.stderr[-3000:])
    print('cycprobe: %s' % os.path.relpath(lib, ROOT))


def main(argv):
    if len(argv) == 4 and argv[1] == '--lib':
        main(['', argv[2]])
        build_lib(os.path.abspath(argv[2]), os.path.abspath(argv[3]))
        return 0
    if len(argv) < 2:
        print(__doc__)
        return 2
    outdir = os.path.abspath(argv[1])
    files = [os.path.abspath(f) for f in argv[2:]] or sorted(glob.glob(os.path.join(SRC, '**', '*.c'), recursive=True))
    rows, nid = [], 1
    inc = os.path.join(outdir, 'include')
    os.makedirs(inc, exist_ok=True)
    # the headers' inline functions first
    heads = sorted(glob.glob(os.path.join(SRC, 'include', '*.h')))
    pure = pure_functions(heads)
    for h in heads:
        nid = instrument(h, os.path.join(inc, os.path.basename(h)), nid, rows, pure)
    for f in files:
        out = os.path.join(outdir, os.path.relpath(f, SRC))
        nid = instrument(f, out, nid, rows, pure)
    with open(os.path.join(inc, 'cycprobe.h'), 'w') as o:
        o.write('/* cycprobe.h: the probes (cycprobe.py). */\n#ifndef CYCPROBE_H\n#define CYCPROBE_H\n'
                'void sfx_probe(unsigned id);\n#define CYC_PROBE(n) sfx_probe(n)\n#endif\n')
    with open(os.path.join(outdir, 'probes.csv'), 'w', newline='') as f:
        wr = csv.writer(f)
        wr.writerow(['id', 'file', 'line', 'col', 'kind', 'offset', 'callee'])
        wr.writerows(rows)
    print('cycprobe: %d probes in %d files under %s' % (len(rows), len(files), os.path.relpath(outdir, ROOT)))
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
