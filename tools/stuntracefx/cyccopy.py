#!/usr/bin/env python3
"""cyccopy.py: the Super FX's helpers that stand for several copies of the
original's code, each told which copy it runs as.

    cyccopy.py CODE.pkl CTX.txt FNMAP.bin FNNAMES.txt CACHES.txt OUT.csv

The original has many small routines in several copies (one beside each
caller, or one for each case of a table); the C has one helper for each,
called from each place. The code the helper stands for then depends on the
call. It is learned per call (the lockstep's code log aggregated with each
probe qualified by the call sites of the shared functions on the stack:
CTX.txt, one line per context, "function:call-site-probe" frames from the
outermost; CODE.pkl {probe + 131072 * context: {address: fetches}}, and
CODE.reached.pkl beside it, the qualified probes reached). A function
whose code is the same wherever it runs keeps its literal code. One whose
code depends on where it runs gets a last parameter `int copy`, the copy
it runs as, and a table of each copy's code beside it, F_code[copy]; each
of its places states its stretch of the table (gsu_codes). Each call passes
its copy as GSU_COPY(k), or, from a function with copies of its own, the
copy that goes with its own (GSU_COPY(F_from_G[copy])).

Edits the tree probed (CYC_SRC) in place: the parameters, the arguments
and the tables. Writes OUT.csv for cycwrite.py --code (each probe's code;
a copy function's as "codes:TABLE:OFFSET:COUNT") and reports what it could
not give a copy (one call whose copy the data picks; calls no run reached,
given copy 0).
"""
import collections, csv, os, pickle, struct, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import cycprobe
import cycwrite

CTX = 131072
END = 1 << 29
CACHE_AT = set()


def load(code_pkl, ctx_txt, fnmap_bin, fnnames_txt):
    code = pickle.load(open(code_pkl, 'rb'))
    rp = code_pkl.replace('.csv.pkl', '.csv.reached.pkl')
    reached = pickle.load(open(rp, 'rb')) if os.path.exists(rp) else set(code)
    raw = open(fnmap_bin, 'rb').read()
    fnmap = struct.unpack('<%di' % (len(raw) // 4), raw)
    names = [tuple(l.split(' ', 1)) for l in open(fnnames_txt).read().split('\n') if l]
    ctxs = [()]
    for l in open(ctx_txt).read().split('\n'):
        if l:
            ctxs.append(tuple(tuple(int(y) for y in x.split(':')) for x in l.split()))
    return code, reached, fnmap, names, ctxs


def items(c):
    """A probe's code as (kind, addr, n) items, split where a CACHE runs
    (after the byte after it is fetched): a sequence in its order, a set of
    addresses in address order."""
    if isinstance(c, (list, tuple)):
        out = []
        for a in c:
            if out and out[-1][0] == 'code' and (out[-1][1] & 0xFF0000) == (a & 0xFF0000) and \
                    ((out[-1][1] + out[-1][2]) & 0xFFFF) == (a & 0xFFFF):
                out[-1] = ('code', out[-1][1], out[-1][2] + 1)
            else:
                out.append(('code', a, 1))
            if a in CACHE_AT:
                out.append(('cache', (a + 1) & 0xFFFF, 0))
        return out
    out, left = [], collections.Counter({a: n for a, n in c.items() if a < 0xE00000})
    while left:
        ks = sorted(left)
        run = [ks[0]]
        for a in ks[1:]:
            if a == run[-1] + 1:
                run.append(a)
            else:
                break
        for a in run:
            left[a] -= 1
            if not left[a]:
                del left[a]
        s = run[0]
        for a in run:
            # a CACHE: stated as its own byte is fetched (the clock runs it
            # after the next fetch, as the pipeline does)
            if a in CACHE_AT:
                out.append(('code', s, a - s + 1))
                out.append(('cache', (a + 1) & 0xFFFF, 0))
                s = a + 1
        if s <= run[-1]:
            out.append(('code', s, run[-1] - s + 1))
    return out


def text_item(x):
    return 'code:%06X:%d' % (x[1], x[2]) if x[0] == 'code' else 'cache:%04X' % x[1]


def main(argv):
    global CACHE_AT
    code, reached, fnmap, names, ctxs = load(*argv[1:5])
    CACHE_AT = {int(l, 16) for l in open(argv[5]) if l.strip()}       # the CACHE instructions' addresses
    out_csv = argv[6]
    probes = {int(r['id']): r for r in csv.DictReader(open(os.path.join(os.path.dirname(argv[2]), 'probe', 'probes.csv')))}
    fn_index = {n: i for i, n in enumerate(names)}
    by = collections.defaultdict(lambda: collections.defaultdict(dict))   # function -> context -> {probe: code}
    plain = {}
    seen_ctx = collections.defaultdict(set)                                # function -> contexts it ran in
    for pid in reached:
        if pid & END:
            continue
        p, ci = pid % CTX, pid // CTX
        if ci and fnmap[p] >= 0:
            seen_ctx[fnmap[p]].add(ci)
    for pid, c in code.items():
        if pid & END:
            continue
        p, ci = pid % CTX, pid // CTX
        if not ci:
            plain[p] = c
        elif fnmap[p] >= 0:
            by[fnmap[p]][ci][p] = c
    rows, report = {}, []
    for p, c in plain.items():
        rows[p] = [text_item(x) for x in items(c)]
    funcs = sorted(set(by) | set(seen_ctx))
    ran = {f: sorted(seen_ctx[f] | set(by.get(f, {}))) for f in funcs}
    # each function's call into another, per context of the callee: (call
    # site, the caller and the caller's context)
    callin = {}           # (callee, callee context) -> (site, caller, caller context)
    for g in funcs:
        for ci in ran[g]:
            frames = ctxs[ci]
            own = [i for i, fr in enumerate(frames) if fr[0] == g]
            if own:
                site = frames[own[-1]][1]
                outer = frames[:own[-1]]
                caller = fnmap[site] if site < len(fnmap) else -1
                oc = ctxs.index(outer) if outer in ctxs else 0
            else:
                found = unique_call(names[g][0], names[g][1])
                if not found:
                    continue
                site = ('unique', g)
                caller = fn_index.get((names[g][0], found[0]), -1)
                oc = ci
            callin[(g, ci)] = (site, caller, oc)

    def own_code(f, ci):
        cs = by.get(f, {})
        return tuple(sorted((p, tuple(items(c))) for p, c in cs.get(ci, {}).items()))
    # copies: a function's distinct (own code, the copies it calls) in its
    # contexts, until they settle
    index = {(f, ci): 0 for f in funcs for ci in ran[f]}
    for _ in range(50):
        calls_of = collections.defaultdict(set)
        for (g, ci), (site, caller, oc) in callin.items():
            calls_of[(caller, oc)].add((str(site), g, index[(g, ci)]))
        changed = False
        for f in funcs:
            sigs = {}
            for ci in ran[f]:
                sig = (own_code(f, ci), tuple(sorted(calls_of.get((f, ci), ()))))
                k = sigs.setdefault(sig, len(sigs))
                if index[(f, ci)] != k:
                    index[(f, ci)] = k
                    changed = True
        if not changed:
            break
    ncopies = {f: 1 + max(index[(f, ci)] for ci in ran[f]) for f in funcs}
    copies = {}
    for f in funcs:
        if ncopies[f] == 1:
            for pc in by.get(f, {}).values():
                for p, c in pc.items():
                    rows[p] = [text_item(x) for x in items(c)]
            continue
        if not copyable(*names[f]):
            cs = by.get(f, {})
            ref = max(cs, key=lambda ci: len(cs[ci])) if cs else None
            for p, c in (cs[ref].items() if ref else ()):
                rows[p] = [text_item(x) for x in items(c)]
            report.append('%s %s: %d copies, but an entry or called by address; its most complete copy written'
                          % (names[f] + (ncopies[f],)))
            continue
        copies[f] = ncopies[f]
    # each copy function's table of its own code, one row per copy
    tables = {}
    for f, k in copies.items():
        cs = by.get(f, {})
        slots = sorted({p for pc in cs.values() for p in pc})
        if not slots:
            continue
        rep = {}
        for ci in ran[f]:
            rep.setdefault(index[(f, ci)], ci)
        per = {p: [items(cs.get(rep[c], {}).get(p, {})) for c in range(k)] for p in slots}
        width = {p: max(len(x) for x in per[p]) for p in slots}
        tname = '%s_code' % names[f][1]
        off = 0
        table = [[] for _ in range(k)]
        for p in slots:
            for c in range(k):
                table[c] += per[p][c] + [('none', 0, 0)] * (width[p] - len(per[p][c]))
            rows[p] = ['codes:%s:%d:%d' % (tname, off, width[p])]
            off += width[p]
        tables[f] = (tname, table)
    # the calls: what each passes
    args = {}
    arrays = collections.defaultdict(dict)
    pairs = collections.defaultdict(set)
    for (g, ci), (site, caller, oc) in callin.items():
        if g not in copies:
            continue
        cc = index.get((caller, oc)) if caller in copies else None
        pairs[(site, g)].add((cc, index[(g, ci)], caller))
    for (site, g), ps in pairs.items():
        caller = next(iter(ps))[2]
        ks = {k for _, k, _ in ps}
        if caller in copies and len(ks) > 1:
            arr = '%s_from_%s' % (names[g][1], names[caller][1])
            vals = arrays[caller].setdefault(arr, [0] * copies[caller])
            for cc, k, _ in ps:
                vals[cc] = k
            args[(site, g)] = 'GSU_COPY(%s[copy])' % arr
            continue
        if len(ks) > 1:
            report.append('%s %s: one call, several copies (the data picks the copy); copy %d given'
                          % (names[g] + (min(ks),)))
        args[(site, g)] = 'GSU_COPY(%d)' % min(ks)
    edit_tree(copies, tables, args, arrays, names, probes)
    with open(out_csv, 'w', newline='') as fo:
        w = csv.writer(fo)
        w.writerow(['id', 'code'])
        for p in sorted(rows):
            w.writerow([p, ' '.join(rows[p])])
    print('cyccopy: functions in shared code: %d the same wherever they run, %d with copies (%d copies in all)'
          % (len(funcs) - len(copies), len(copies), sum(copies.values())))
    for r in report:
        print('  ' + r)
    return 0


_calls = {}
_static = {}
_taken = None


def is_static(rel, name):
    """A function of the file with internal linkage (its calls are all there)."""
    if rel not in _static:
        path = os.path.join(cycprobe.SRC, os.path.relpath(rel, 'src/stuntracefx'))
        tree = cycprobe.ast(path)
        _static[rel] = {n.get('name') for n in tree.get('inner', [])
                        if n.get('kind') == 'FunctionDecl' and n.get('storageClass') == 'static'}
    return name in _static[rel]


def all_files():
    import glob
    return sorted(os.path.relpath(p, cycprobe.ROOT) if False else
                  os.path.join('src/stuntracefx', os.path.relpath(p, cycprobe.SRC))
                  for p in glob.glob(os.path.join(cycprobe.SRC, '**', '*.c'), recursive=True))


def taken():
    """Functions some file refers to other than by calling them (their
    address taken: a table, a pointer)."""
    global _taken
    if _taken is None:
        _taken = set()
        for rel in all_files():
            path = os.path.join(cycprobe.SRC, os.path.relpath(rel, 'src/stuntracefx'))
            tree = cycprobe.ast(path)

            def walk(node, callee_ok):
                if isinstance(node, dict):
                    k = node.get('kind')
                    if k == 'DeclRefExpr' and not callee_ok and \
                            node.get('referencedDecl', {}).get('kind') == 'FunctionDecl':
                        _taken.add(node['referencedDecl'].get('name'))
                    if k == 'CallExpr' and node.get('inner'):
                        walk(node['inner'][0], True)
                        for v in node['inner'][1:]:
                            walk(v, False)
                        return
                    for key, v in node.items():
                        walk(v, callee_ok and k in ('ImplicitCastExpr', 'ParenExpr'))
                elif isinstance(node, list):
                    for v in node:
                        walk(v, callee_ok)
            walk(tree, False)
    return _taken


def copyable(rel, name):
    """A function every use of which is a direct call: a copy can be passed."""
    return not name.startswith('abi_') and name not in taken()


def file_calls(rel):
    """Every call in a file of the tree probed: (start, end, callee, number
    of arguments, the function it is in)."""
    if rel in _calls:
        return _calls[rel]
    path = os.path.join(cycprobe.SRC, os.path.relpath(rel, 'src/stuntracefx'))
    tree = cycprobe.ast(path)
    cycprobe.annotate(tree, [None])
    own = os.path.abspath(path)
    out = []

    def find(node, fn):
        if isinstance(node, dict):
            if node.get('kind') == 'FunctionDecl':
                fn = node.get('name')
            if node.get('kind') == 'CallExpr' and node.get('inner'):
                callee = cycprobe.callee(node['inner'][0])
                r = node.get('range', {})
                b, e = r.get('begin', {}).get('offset'), r.get('end', {}).get('offset')
                if callee and b is not None and e is not None and \
                        os.path.abspath(r['begin'].get('_file') or path) == own:
                    out.append((b, e, callee, len(node['inner']) - 1, fn))
            for v in node.values():
                find(v, fn)
        elif isinstance(node, list):
            for v in node:
                find(v, fn)
    find(tree, None)
    _calls[rel] = out
    return out


def unique_call(rel, name):
    """The one call of a static function in its file: (caller, start, end, arguments)."""
    cs = [c for c in file_calls(rel) if c[2] == name]
    return (cs[0][4],) + cs[0][:2] + (cs[0][3],) if len(cs) == 1 else None


def table_text(name, table):
    def one(x):
        if x[0] == 'code':
            return '{0x%06X, %d}' % (x[1], x[2])
        if x[0] == 'cache':
            return '{0x%04X, 0}' % x[1]
        return '{0, 0}'
    lines = ["/* cyccopy: the code of each of the original's copies, by where it runs */",
             'static const GsuCode %s[%d][%d] = {' % (name, len(table), max(1, len(table[0])))]
    for row in table:
        lines.append('    {%s},' % (', '.join(one(x) for x in row) or '{0, 0}'))
    return '\n'.join(lines) + '\n};\n\n'


def array_text(name, vals):
    return ("/* cyccopy: the copy each of the caller's copies calls */\n"
            'static const uint8_t %s[%d] = {%s};\n\n' % (name, len(vals), ', '.join(str(v) for v in vals)))


def before_comment(text, at):
    """Where to put a block before the declaration starting at `at`: before
    the comments just above it."""
    j = text.rfind('\n', 0, at) + 1
    while True:
        prev = text[:j].rstrip()
        if prev.endswith('*/'):
            cs = prev.rfind('/*')
            ls = text.rfind('\n', 0, cs) + 1
            if text[ls:cs].strip() == '':
                j = ls
                continue
        return j


def call_arg(text, b, e, nargs, a):
    """The edit giving a call (text[b:e], its closing parenthesis at e)
    the argument a: replacing a GSU_COPY() it has."""
    k = text.rfind('GSU_COPY(', b, e)
    if k >= 0:
        q = cycwrite._close(text, k + len('GSU_COPY('), '(', ')')
        if text[q + 1:e].strip() == '':
            return (k, q + 1, a)
    return (e, e, (', ' if nargs else '') + a)


def edit_tree(copies, tables, args, arrays, names, probes):
    """`int copy` added to each copy function's declarations and to its
    calls (a static one's in its file, a public one's in every file and
    header); the tables put before the first declaration of the function
    that uses them."""
    import glob, re
    statics = {(names[f][0], names[f][1]) for f in copies if is_static(*names[f])}
    publics = {names[f][1] for f in copies if not is_static(*names[f])}

    def mine(rel, name, static_decl):
        """Whether a declaration or call of name in rel is a copy function's."""
        if static_decl is None:
            static_decl = is_static(rel, name)
        return (rel, name) in statics if static_decl else name in publics
    callers = collections.defaultdict(list)
    for (site, f), a in args.items():
        if isinstance(site, tuple):
            callers[names[f][0]].append((None, names[f][1], a))
        else:
            callers[probes[site]['file']].append((int(probes[site]['offset']), names[f][1], a))
    blocks = collections.defaultdict(list)    # file -> [(function name, text)]
    for f, (tname, table) in tables.items():
        blocks[names[f][0]].append((names[f][1], table_text(tname, table)))
    for g, arrs in arrays.items():
        for arr, vals in arrs.items():
            blocks[names[g][0]].append((names[g][1], array_text(arr, vals)))
    files = set(callers) | set(blocks) | {r for r, _ in statics} | (set(all_files()) if publics else set())
    for rel in sorted(files):
        path = os.path.join(cycprobe.SRC, os.path.relpath(rel, 'src/stuntracefx'))
        text = open(path).read()
        calls = file_calls(rel)
        tree = cycprobe.ast(path)
        cycprobe.annotate(tree, [None])
        own = os.path.abspath(path)
        edits = []
        first_def = {}
        for node in tree.get('inner', []):
            if node.get('kind') != 'FunctionDecl' or os.path.abspath(node.get('loc', {}).get('_file') or path) != own:
                continue
            name = node.get('name')
            b = node.get('range', {}).get('begin', {}).get('offset')
            if name not in first_def and b is not None:
                first_def[name] = b
            if mine(rel, name, node.get('storageClass') == 'static'):
                off = node['loc']['offset']
                op = text.index('(', off)
                cl = cycwrite._close(text, op + 1, '(', ')')
                inside = text[op + 1:cl].strip()
                if not inside.endswith('int copy'):
                    edits.append((op + 1, cl, 'int copy' if inside in ('', 'void') else inside + ', int copy'))
        for off, name, a in callers.get(rel, ()):
            cs = [c for c in calls if c[2] == name]
            cand = sorted((c for c in cs if off is not None and c[0] <= off <= c[1]), key=lambda c: -c[0]) or \
                sorted(c for c in cs if off is None or c[0] >= off)
            if not cand:
                print('  cyccopy: no call of %s to give its copy in %s' % (name, rel))
                continue
            b, e, _, nargs, _fn = cand[0]
            if not any(x[1] == e or x[0] == e for x in edits):
                edits.append(call_arg(text, b, e, nargs, a))
        for b, e, callee, nargs, _fn in calls:
            if mine(rel, callee, None) and not any(x[1] == e or x[0] == e for x in edits):
                edits.append(call_arg(text, b, e, nargs, 'GSU_COPY(0)'))
                print('  cyccopy: %s %s: a call no run reached, given copy 0' % (rel, callee))
        for fname, block in blocks.get(rel, ()):
            at = before_comment(text, first_def[fname])
            edits.append((at, at, block))
        for b, e, s in sorted(edits, key=lambda x: (x[0], x[1]), reverse=True):
            text = text[:b] + s + text[e:]
        with open(path, 'w') as fo:
            fo.write(text)
    # the headers' prototypes of the public ones
    for h in sorted(glob.glob(os.path.join(cycprobe.SRC, 'include', '*.h'))):
        text = open(h).read()
        out = text
        for name in sorted(publics):
            for m in reversed(list(re.finditer(r'\b%s\s*\(' % re.escape(name), out))):
                line = out[out.rfind('\n', 0, m.start()) + 1:m.start()]
                if not re.search(r'[\w\*]\s*$', line) or 'static' in line:
                    continue            # a call or a static, not a prototype
                op = m.end() - 1
                cl = cycwrite._close(out, op + 1, '(', ')')
                if not out[cl + 1:].lstrip().startswith(';'):
                    continue
                inside = out[op + 1:cl].strip()
                if not inside.endswith('int copy'):
                    out = out[:op + 1] + ('int copy' if inside in ('', 'void') else inside + ', int copy') + out[cl:]
        if out != text:
            with open(h, 'w') as fo:
                fo.write(out)


if __name__ == '__main__':
    sys.exit(main(sys.argv))
