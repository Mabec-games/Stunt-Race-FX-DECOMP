#!/usr/bin/env python3
"""cycwrite.py: the solved costs written into the decomp's C, as cyc().

    cycwrite.py COSTS.csv [--skip FUNCS.txt] [--dry]
    cycwrite.py --code CODE.csv [--skip FUNCS.txt] [--dry]
    cycwrite.py --strip DIR [--code] [--only FUNCS.txt]

COSTS.csv is cycsolve.py's (id, ..., cost), numbered as cycprobe.py numbers
the probes of the tree probed: the decomp, or with CYC_SRC a copy of it
(--strip DIR makes one without the times this tool wrote, so a solve from
it is written whole, replacing them).
Each probe with a cost becomes the time its place stands for:

    a block's start, an arm, a case, the place paths join:  cyc(N);
    an arm with no block of its own:                         { cyc(N); it }
    the path that skips an arm:                              else { cyc(N); }
    a branch inside an expression (&&, ||, ?:):              (cyc(N), it)
    a loop going round once more:                            its test as cyc_loop(N, test)
    a while's or for's pass ending:                          cyc(N); before its closing brace
    a function's end, a return:                              cyc(N); before it (after the value
                                                             when that reaches the bus: cyc_then)
    a bus access (rd8, wr8, a call into code that reaches the bus):
        the first in its statement, its statement always run from its
        start:                                               cyc(N); before the statement
        else (after another access in the statement, in a loop's test,
        in an arm of a condition):                           its last argument as cyc_then(N, it)

Every one is exact: the time comes where the probe stood, on the same
paths. Functions named in FUNCS.txt (lines "path function") are left as
they are: the ones whose C does not tell apart paths of the original that
take different times, fixed by hand first. --dry reports without writing.
"""
import collections, csv, os, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import cycprobe

ROOT = cycprobe.ROOT


class Clocks:
    """A probe's value as clocks: cyc(N) and its forms."""
    zero = 0

    @staticmethod
    def stmt(n): return 'cyc(%d); ' % n
    @staticmethod
    def expr(n): return '(cyc(%d), ' % n, ')'
    @staticmethod
    def loop(n): return 'cyc_loop(%d, ' % n, ')'
    @staticmethod
    def then(n): return 'cyc_then(%d, ' % n, ')'


class Code:
    """A probe's value as the original's code it stands for: a list of
    ('code', addr, n) runs fetched and ('cache', r15) CACHE instructions run,
    as gsu_code() and gsu_cache() (gsu.h)."""
    zero = []

    @staticmethod
    def addr(a):
        """An address: a number, or ('at', offset) from a helper's copy."""
        if isinstance(a, tuple):
            return COPY if not a[1] else ('%s + 0x%X' % (COPY, a[1]) if a[1] > 0 else '%s - 0x%X' % (COPY, -a[1]))
        return '0x%06X' % a

    @classmethod
    def one(cls, x):
        if x[0] == 'codes':
            return 'gsu_codes(%s, %d)' % (cls.row(x), x[3])
        return 'gsu_code(%s, %d)' % (cls.addr(x[1]), x[2]) if x[0] == 'code' else 'gsu_cache(0x%04X)' % x[1]

    @staticmethod
    def row(x):
        """A copy function's stretch of its table (cyccopy.py)."""
        return '%s[copy] + %d' % (x[1], x[2]) if x[2] else '%s[copy]' % x[1]

    @classmethod
    def stmt(cls, v): return ''.join(cls.one(x) + '; ' for x in v)
    @classmethod
    def expr(cls, v): return '(' + ''.join(cls.one(x) + ', ' for x in v), ')'
    @classmethod
    def nested(cls, v, how):
        return ''.join('gsu_code_%s(%s, %d, ' % (how, cls.addr(x[1]), x[2]) if x[0] == 'code'
                       else 'gsu_codes_%s(%s, %d, ' % (how, cls.row(x), x[3]) if x[0] == 'codes'
                       else 'gsu_cache_%s(0x%04X, ' % (how, x[1]) for x in v), ')' * len(v)
    @classmethod
    def loop(cls, v): return cls.nested(v, 'loop')
    @classmethod
    def then(cls, v): return cls.nested(v, 'then')


def plan(path, text, got, cost, skip, form=Clocks):
    """The edits for one file: (offset, rank, text) with rank ordering the
    edits at one offset, and what was left out and why."""
    rel = cycprobe.rel_name(path)
    stmt = collections.defaultdict(lambda: type(form.zero)())     # offset -> the time before the statement there
    wraps = {}                             # (start, end) -> [time at its start, at its end], a body given braces
    skips = []                             # (offset, wrapped start, time)
    exprs = []                             # (start, end, opener text, closer text)
    left = collections.Counter()

    def opener(make, n, why):
        oc = make(n)
        if oc is None:
            left[why] += 1
        return oc
    for pid, kind, m in got:
        n = cost.get(pid, form.zero)
        if not n:
            continue
        if (rel, m['fn']) in skip:
            left['in a function fixed by hand first'] += 1
            continue
        where = m['form']
        z = lambda: [type(form.zero)(), type(form.zero)()]
        if where == 'stmt':
            stmt[m['at']] += n
        elif where == 'wrap':
            wraps.setdefault((m['at'], m['end']), z())[0] += n
        elif where == 'back_close':
            wraps.setdefault(m['wrapped'], z())[1] += n
        elif where == 'again':
            if cycprobe.number_type(m['typ']) or m['typ'] in ('int', '_Bool'):
                oc = opener(form.loop, n, 'a loop test with a CACHE')
                if oc:
                    exprs.append((m['at'], m['end']) + oc)
            else:
                left['a loop test that is not a number'] += 1
        elif where == 'ret':
            a = m['anchor']
            if m.get('noacc') and a and m.get('ret_at') == a[0]:
                if a[1]:
                    wraps.setdefault((a[0], a[2]), z())[0] += n
                else:
                    stmt[a[0]] += n
            elif m['typ'] != 'void' and cycprobe.number_type(m['typ']):
                oc = opener(form.then, n, 'a return value with a CACHE')
                if oc:
                    exprs.append((m['at'], m['end']) + oc)
            else:
                left['a return whose value reaches the bus, not a number'] += 1
        elif where == 'skip':
            skips.append((m['at'], m['wrapped'][0], n))
        elif where == 'expr':
            exprs.append((m['at'], m['end']) + form.expr(n))
        elif where == 'arg':
            a = m['anchor']
            if a and not m['unsafe'] and m.get('first'):
                if a[1]:
                    wraps.setdefault((a[0], a[2]), z())[0] += n
                else:
                    stmt[a[0]] += n
            elif not cycprobe.number_type(m['typ']):
                left['an access inside an expression, its last argument not a number'] += 1
            else:
                oc = opener(form.then, n, 'an access argument with a CACHE')
                if oc:
                    exprs.append((m['at'], m['end']) + oc)
    edits = []
    # a statement's time: on a line of its own where the statement starts one
    merged = collections.defaultdict(lambda: type(form.zero)())
    for at, n in stmt.items():
        j = at
        while j < len(text) and text[j] in ' \t\n':
            j += 1
        merged[j] += n
    for at, n in merged.items():
        ls = text.rfind('\n', 0, at) + 1
        indent = text[ls:at]
        if indent.strip() == '':
            # before a closing brace on its own line: inside the block, at
            # the indent of its statements
            inner = '    ' if text[at:at + 1] == '}' else ''
            edits.append((at, (1, -2, 0), inner + form.stmt(n).rstrip(' ') + '\n' + indent))
        else:
            edits.append((at, (1, -2, 0), form.stmt(n)))
    # braces: at one offset, closings before openings; an inner body's
    # closing before the outer's, an outer opening before the inner's
    for (b, e), (n, back) in wraps.items():
        edits.append((b, (1, -3, -e), '{ ' + form.stmt(n) if n else '{ '))
        edits.append((e, (0, -b, 0), ' ' + form.stmt(back) + '}' if back else ' }'))
    for at, b, n in skips:
        edits.append((at, (0, -b, 1), ' else { ' + form.stmt(n) + '}'))
    for b, e, op, cl in exprs:
        edits.append((b, (1, -1, -e), op))
        edits.append((e, (0, -b, 0), cl))
    return edits, left


def apply(text, edits):
    out, at = [], 0
    for off, _rank, s in sorted(edits, key=lambda x: (x[0], x[1])):
        out.append(text[at:off])
        out.append(s)
        at = off
    out.append(text[at:])
    return ''.join(out)


def _close(text, i, op, cl):
    """The index of the bracket closing the one opened just before i."""
    depth, q = 1, i
    while q < len(text):
        ch = text[q]
        if ch in '"\'':
            end = q + 1
            while end < len(text) and text[end] != ch:
                end += 2 if text[end] == '\\' else 1
            q = end + 1
            continue
        if text.startswith('/*', q):
            q = text.index('*/', q) + 2
            continue
        if text.startswith('//', q):
            q = text.index('\n', q)
            continue
        if ch == op:
            depth += 1
        elif ch == cl:
            depth -= 1
            if depth == 0:
                return q
        q += 1
    raise ValueError('unbalanced')


# a time this tool writes: clocks, or the original's code (it writes
# numbers, the addresses in upper-case hex: a time written by hand, cyc(t->x),
# gsu_code(go ? a : b, 1), gsu_code(0x01aa49, 1), is not one)
COPY = 'copy_at'     # a helper's parameter: the address of the copy of the original's code it runs as
CODE_ADDR = r'(?:0x[0-9A-F]{6}|%s|%s [+-] 0x[0-9A-F]+)' % (COPY, COPY)
CODE_ROW = r'\w+_code\[copy\](?: \+ \d+)?'
CODE_ITEM = r'(?:gsu_code\(%s, \d+\)|gsu_cache\(0x[0-9A-F]{4}\)|gsu_codes\(%s, \d+\))' % (CODE_ADDR, CODE_ROW)
ITEM = r'(?:cyc\(\d+\)|%s)' % CODE_ITEM[3:-1]


def _statements(s):
    """How many statements end at the top level of s (the body this tool
    puts braces around is one)."""
    depth, n = 0, 0
    for ch in s:
        if ch in '({[':
            depth += 1
        elif ch in ')}]':
            depth -= 1
            if depth == 0 and ch == '}':
                n += 1
        elif ch == ';' and depth == 0:
            n += 1
    return n


def strip(text, code_only=False):
    """The text without what this tool writes, in each of its forms (a time
    written by hand stays; braces it added for a time at a body's end only
    stay too). code_only: the original's code only (gsu_code, gsu_cache),
    the clocks kept."""
    import re
    ITEM = CODE_ITEM if code_only else globals()['ITEM']
    # cyccopy.py's tables of each copy's code (made again with the code)
    text = re.sub(r"/\* cyccopy: [^\n]*\*/\nstatic const (?:GsuCode|uint8_t) [^;]*;\n\n", '', text)
    text = re.sub(r'GSU_COPY\(\w+\[copy\]\)', 'GSU_COPY(0)', text)      # their arrays go with them
    out = re.sub(r' else \{ (?:%s; )+\}' % ITEM, '', text)
    out = re.sub(r' (?:%s; )+\}' % ITEM, ' }', out)
    # forms around code: { T; X }, (T, X), cyc_then(N, X), cyc_loop(N, X)
    forms = ((r'\{ (?:%s; )+' % ITEM, '{', '}', ' }'), (r'\((?:%s, )+' % ITEM, '(', ')', ')'),
             (r'gsu_code_then\(%s, \d+, ' % CODE_ADDR, '(', ')', ')'),
             (r'gsu_code_loop\(%s, \d+, ' % CODE_ADDR, '(', ')', ')'),
             (r'gsu_cache_then\(0x[0-9A-F]{4}, ', '(', ')', ')'), (r'gsu_cache_loop\(0x[0-9A-F]{4}, ', '(', ')', ')'),
             (r'gsu_codes_then\(%s, \d+, ' % CODE_ROW, '(', ')', ')'), (r'gsu_codes_loop\(%s, \d+, ' % CODE_ROW, '(', ')', ')'))
    if not code_only:
        forms += ((r'cyc_then\(\d+, ', '(', ')', ')'), (r'cyc_loop\(\d+, ', '(', ')', ')'))
    for pat, op, cl, tail in forms:
        while True:
            m = re.search(pat, out)
            if not m:
                break
            q = _close(out, m.end(), op, cl)
            if not out[:q + 1].endswith(tail):
                raise ValueError('cycwrite: %r at %d does not close as written' % (m.group(0), m.start()))
            if op == '{' and _statements(out[m.end():q + 1 - len(tail)]) > 1:
                # the block's own braces, a time at its start: the time only
                out = out[:m.start() + 2] + out[m.end():]
                continue
            out = out[:m.start()] + out[m.end():q + 1 - len(tail)] + out[q + 1:]
    # statements: before a block's closing brace, on a line of their own, in a line
    out = re.sub(r'(?m)^( *)    (?:%s; )*%s;\n\1\}' % (ITEM, ITEM), r'\1}', out)
    out = re.sub(r'(?:%s; )*%s;\n[ \t]*' % (ITEM, ITEM), '', out)
    out = re.sub(r'(?:%s; )+' % ITEM, '', out)
    return out


def merge(text):
    """Times written next to times already there, as one: cyc(a); cyc(b);
    as cyc(a+b);, and (cyc(a), (cyc(b), X)), cyc_then(a, cyc_then(b, X)) as
    one of each."""
    import re
    stmt = re.compile(r'cyc\((\d+)\);(\s*)cyc\((\d+)\);')
    while True:
        m = stmt.search(text)
        if not m:
            break
        text = text[:m.start()] + 'cyc(%d);' % (int(m.group(1)) + int(m.group(3))) + text[m.end():]
    for form in (r'\(cyc\((\d+)\), \(cyc\((\d+)\), ', r'cyc_then\((\d+), cyc_then\((\d+), '):
        pat = re.compile(form)
        while True:
            m = pat.search(text)
            if not m:
                break
            inner = m.start() + m.group(0).rindex('(cyc' if form.startswith(r'\(') else 'cyc_then')
            open_at = text.index('(', inner) + 1
            q = _close(text, open_at, '(', ')')
            outer = _close(text, text.index('(', m.start()) + 1, '(', ')')
            if outer != q + 1:
                raise ValueError('cycwrite: %r at %d does not close as written' % (m.group(0), m.start()))
            head = ('(cyc(%d), ' if form.startswith(r'\(') else 'cyc_then(%d, ') % (int(m.group(1)) + int(m.group(2)))
            text = text[:m.start()] + head + text[m.end():q] + text[q + 1:]
    return text


def function_spans(path, text, names):
    """The bodies of the functions named, as (start, end) offsets."""
    if not names:
        return []
    tree = cycprobe.ast(path)
    cycprobe.annotate(tree, [None])
    w = cycprobe.Walker(path, text, set())
    spans = []
    for node in tree.get('inner', []):
        if node.get('kind') == 'FunctionDecl' and node.get('name') in names:
            body = next((c for c in node.get('inner', []) if c.get('kind') == 'CompoundStmt'), None)
            span = body and w.mine(body)
            if span:
                spans.append(span)
    return spans


def main(argv):
    args = argv[1:]
    if args[:1] == ['--strip']:
        # a copy of the decomp without the written times (or, --only FUNCS.txt,
        # without those of the functions named), for probing
        import glob
        out = os.path.abspath(args[1])
        only = None
        code_only = '--code' in args
        if '--only' in args:
            only = {tuple(l.split()) for l in open(args[args.index('--only') + 1]) if l.strip()}
        src = cycprobe.SRC
        for f in glob.glob(os.path.join(src, '**', '*.[ch]'), recursive=True):
            o = os.path.join(out, os.path.relpath(f, src))
            os.makedirs(os.path.dirname(o), exist_ok=True)
            text = open(f).read()
            if only is None:
                text = strip(text, code_only)
            else:
                spans = function_spans(f, text, {fn for r, fn in only if r == cycprobe.rel_name(f)})
                for b, e in sorted(spans, reverse=True):
                    text = text[:b] + strip(text[b:e], code_only) + text[e:]
            with open(o, 'w') as w:
                w.write(text)
        print('cycwrite: the decomp without %s written times in %s' % ('its' if only is None else 'those functions\'', out))
        return 0
    dry = '--dry' in args
    if dry:
        args.remove('--dry')
    skip = set()
    if '--skip' in args:
        i = args.index('--skip')
        for line in open(args[i + 1]):
            if line.strip():
                f, fn = line.split()
                skip.add((f, fn))
        del args[i:i + 2]
    form = Clocks
    if '--code' in args:
        # CODE.csv (id, code): each probe's code as "code:ADDR:N" and
        # "cache:R15" items, space-separated, in the order fetched
        i = args.index('--code')
        form, cost = Code, {}
        for r in csv.DictReader(open(args[i + 1])):
            items = []
            for x in r['code'].split():
                p = x.split(':')
                if p[0] == 'codes':
                    items.append(('codes', p[1], int(p[2]), int(p[3])))
                    continue
                if p[0] == 'code' and p[1].startswith('at'):
                    off = p[1][2:]
                    items.append(('code', ('at', int(off.replace('+', ''), 16) if off else 0), int(p[2])))
                else:
                    items.append(('code', int(p[1], 16), int(p[2])) if p[0] == 'code' else ('cache', int(p[1], 16)))
            if items:
                cost[int(r['id'])] = items
        del args[i:i + 2]
        args.append(None)
    if len(args) != 1:
        print(__doc__)
        return 2
    if form is Clocks:
        cost = {}
        for r in csv.DictReader(open(args[0])):
            if r['cost'] not in ('', '0'):
                cost[int(r['id'])] = int(r['cost'])
    files, written, left = 0, 0, collections.Counter()
    for path, text, got in cycprobe.all_probes():
        edits, why = plan(path, text, got, cost, skip, form)
        left.update(why)
        # into the decomp, from the tree probed (the decomp itself, or its
        # copy without written times: CYC_SRC); a file with no times is
        # written too, so a time written before goes
        dest = os.path.join(cycprobe.DECOMP, os.path.relpath(path, cycprobe.SRC))
        new = merge(apply(text, edits))
        if not dry and new != open(dest).read():
            with open(dest, 'w') as f:
                f.write(new)
        if not edits:
            continue
        files += 1
        written += sum(1 for e in edits if e[2].startswith(('cyc', '{ cyc', ' else', '(cyc', 'gsu_', '{ gsu_', '(gsu_')))
    print('cycwrite: %d costs in %d files%s' % (written, files, ' (dry run)' if dry else ''))
    for why, n in left.most_common():
        print('  %6d left: %s' % (n, why))
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
