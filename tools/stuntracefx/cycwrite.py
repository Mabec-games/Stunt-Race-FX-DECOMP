#!/usr/bin/env python3
"""cycwrite.py: the solved costs written into the decomp's C, as cyc().

    cycwrite.py COSTS.csv [--skip FUNCS.txt] [--dry]
    cycwrite.py --strip DIR

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


def plan(path, text, got, cost, skip):
    """The edits for one file: (offset, rank, text) with rank ordering the
    edits at one offset, and what was left out and why."""
    rel = cycprobe.rel_name(path)
    stmt = collections.Counter()           # offset -> clocks before the statement there
    wraps = {}                             # (start, end) -> [clocks at its start, at its end], a body given braces
    skips = []                             # (offset, wrapped start, clocks)
    exprs = []                             # (start, end, opener text)
    left = collections.Counter()
    for pid, kind, m in got:
        n = cost.get(pid, 0)
        if not n:
            continue
        if (rel, m['fn']) in skip:
            left['in a function fixed by hand first'] += 1
            continue
        form = m['form']
        if form == 'stmt':
            stmt[m['at']] += n
        elif form == 'wrap':
            wraps.setdefault((m['at'], m['end']), [0, 0])[0] += n
        elif form == 'back_close':
            wraps.setdefault(m['wrapped'], [0, 0])[1] += n
        elif form == 'again':
            if cycprobe.number_type(m['typ']) or m['typ'] in ('int', '_Bool'):
                exprs.append((m['at'], m['end'], 'cyc_loop(%d, ' % n))
            else:
                left['a loop test that is not a number'] += 1
        elif form == 'ret':
            a = m['anchor']
            if m.get('noacc') and a and m.get('ret_at') == a[0]:
                if a[1]:
                    wraps.setdefault((a[0], a[2]), [0, 0])[0] += n
                else:
                    stmt[a[0]] += n
            elif m['typ'] != 'void' and cycprobe.number_type(m['typ']):
                exprs.append((m['at'], m['end'], 'cyc_then(%d, ' % n))
            else:
                left['a return whose value reaches the bus, not a number'] += 1
        elif form == 'skip':
            skips.append((m['at'], m['wrapped'][0], n))
        elif form == 'expr':
            exprs.append((m['at'], m['end'], '(cyc(%d), ' % n))
        elif form == 'arg':
            a = m['anchor']
            if a and not m['unsafe'] and m.get('first'):
                if a[1]:
                    wraps.setdefault((a[0], a[2]), [0, 0])[0] += n
                else:
                    stmt[a[0]] += n
            elif not cycprobe.number_type(m['typ']):
                left['an access inside an expression, its last argument not a number'] += 1
            else:
                exprs.append((m['at'], m['end'], 'cyc_then(%d, ' % n))
    edits = []
    # a statement's time: on a line of its own where the statement starts one
    merged = collections.Counter()
    for at, n in stmt.items():
        j = at
        while j < len(text) and text[j] in ' \t\n':
            j += 1
        merged[j] += n
    for at, n in merged.items():
        ls = text.rfind('\n', 0, at) + 1
        indent = text[ls:at]
        if indent.strip() == '':
            if text[at:at + 1] == '}':
                indent += '    '
            edits.append((at, (1, -2, 0), 'cyc(%d);\n%s' % (n, indent)))
        else:
            edits.append((at, (1, -2, 0), 'cyc(%d); ' % n))
    # braces: at one offset, closings before openings; an inner body's
    # closing before the outer's, an outer opening before the inner's
    for (b, e), (n, back) in wraps.items():
        edits.append((b, (1, -3, -e), '{ cyc(%d); ' % n if n else '{ '))
        edits.append((e, (0, -b, 0), ' cyc(%d); }' % back if back else ' }'))
    for at, b, n in skips:
        edits.append((at, (0, -b, 1), ' else { cyc(%d); }' % n))
    for b, e, opener in exprs:
        edits.append((b, (1, -1, -e), opener))
        edits.append((e, (0, -b, 0), ')'))
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


def strip(text):
    """The text without what this tool writes: every cyc(N) with N a
    number, in each of its forms (a time written by hand, cyc(t->x), stays;
    braces it added for a time at a body's end only stay too)."""
    import re
    out = re.sub(r' else \{ cyc\(\d+\); \}', '', text)
    # forms around code: { cyc(N); X }, (cyc(N), X), cyc_then(N, X)
    out = re.sub(r' cyc\(\d+\); \}', ' }', out)
    for pat, op, cl, tail in ((r'\{ cyc\(\d+\); ', '{', '}', ' }'), (r'\(cyc\(\d+\), ', '(', ')', ')'),
                              (r'cyc_then\(\d+, ', '(', ')', ')'), (r'cyc_loop\(\d+, ', '(', ')', ')')):
        while True:
            m = re.search(pat, out)
            if not m:
                break
            q = _close(out, m.end(), op, cl)
            if not out[:q + 1].endswith(tail):
                raise ValueError('cycwrite: %r at %d does not close as written' % (m.group(0), m.start()))
            out = out[:m.start()] + out[m.end():q + 1 - len(tail)] + out[q + 1:]
    out = re.sub(r'cyc\(\d+\);\n[ \t]*', '', out)
    out = re.sub(r'cyc\(\d+\); ', '', out)
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
        if '--only' in args:
            only = {tuple(l.split()) for l in open(args[args.index('--only') + 1]) if l.strip()}
        src = cycprobe.SRC
        for f in glob.glob(os.path.join(src, '**', '*.[ch]'), recursive=True):
            o = os.path.join(out, os.path.relpath(f, src))
            os.makedirs(os.path.dirname(o), exist_ok=True)
            text = open(f).read()
            if only is None:
                text = strip(text)
            else:
                spans = function_spans(f, text, {fn for r, fn in only if r == cycprobe.rel_name(f)})
                for b, e in sorted(spans, reverse=True):
                    text = text[:b] + strip(text[b:e]) + text[e:]
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
    if len(args) != 1:
        print(__doc__)
        return 2
    cost = {}
    for r in csv.DictReader(open(args[0])):
        if r['cost'] not in ('', '0'):
            cost[int(r['id'])] = int(r['cost'])
    files, written, left = 0, 0, collections.Counter()
    for path, text, got in cycprobe.all_probes():
        edits, why = plan(path, text, got, cost, skip)
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
        written += sum(1 for e in edits if e[2].startswith(('cyc', '{ cyc', ' else', '(cyc')))
    print('cycwrite: %d costs in %d files%s' % (written, files, ' (dry run)' if dry else ''))
    for why, n in left.most_common():
        print('  %6d left: %s' % (n, why))
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
