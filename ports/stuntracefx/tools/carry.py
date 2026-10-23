#!/usr/bin/env python3
"""carry.py: plain C, step 1, for routines whose callers read the registers
they leave.

    carry.py FILE

The routines of FILE (relative to ports/stuntracefx/game) that still work
in the register file become typed functions on a carried copy of the
registers (include/carried.h, cpu_ops.h's instructions on that copy); each
abi_ adapter loads the copy from the register file and stores it back. A
call of another file's routine goes through a wrapper made from that
routine's adapter (what it leaves in each register); one whose adapter
uses that file's private helpers, or a routine still in registers, is
called with the registers loaded and taken back. Routines that dispatch
(call_routine, jump_routine), touch the stack or pass routines as values
stay in registers ("glue"). Writes FILE in place and prints what it left.
Check the result with the side-by-side harness and same.py.
"""
import os, re, sys
G = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'game')
EXPORT = sys.argv[1] == '--export'
FILE = sys.argv[2] if EXPORT else sys.argv[1]
src = {}
for d, _, fs in os.walk(G):
    for f in fs:
        if f.endswith('.c'):
            p = os.path.join(d, f)
            src[os.path.relpath(p, G)] = open(p).read()
text = src[FILE]

HDR = re.compile(r'^((?:static )?(?:inline )?[A-Za-z_][\w \*]*?\b(\w+)\s*)\(([^;{]*?)\)\s*\n\{\n', re.M | re.S)

def functions(t):
    """(start, end, name, header-prefix, params, body-start) for top-level definitions."""
    out = []
    for m in HDR.finditer(t):
        if m.group(1).startswith(('typedef', 'return', 'else', 'if ', 'for ', 'while')):
            continue
        e = t.index('\n}\n', m.end() - 1) + 3
        out.append((m.start(), e, m.group(2), m.group(1), m.group(3), m.end()))
    return out

REG = re.compile(r'\bcpu\.(a|x|y|d|p|db)\b|\bop_(?!adc16|sbc16|adc8|sbc8)\w+\s*\(|\babi_\w+\s*\(\s*\)')
GLUE = re.compile(r'\b(call_routine|jump_routine|jump_routine_pushed|cpu_restart|cpu_loop_forever)\b|\bcpu\.(s|pc|e|unwound)\b')

_DEFS = None
def all_defs():
    global _DEFS
    if _DEFS is None:
        _DEFS = {'local': {}, 'pub': {}}
        for ff, t in src.items():
            for (s0, e, n, hdr, params, bs) in functions(t):
                b = t[bs:e - 2]
                _DEFS['local'][(ff, n)] = (b, params)
                if not hdr.startswith('static'):
                    _DEFS['pub'].setdefault(n, []).append((ff, b, params))
    return _DEFS

def touches_any(f, b, skip):
    """Whether anything b (in file f) calls, followed into its own file,
    uses the register file or dispatches; names in skip (carried here) and
    functions on carried registers do not count."""
    D = all_defs()
    seen = set()
    todo = [(f, c) for c in set(re.findall(r'(?<![\w>.])([A-Za-z_]\w*)\s*\(', b))]
    while todo:
        ff, c = todo.pop()
        if (ff, c) in seen or (ff == FILE and c in skip):
            continue
        seen.add((ff, c))
        cands = [(ff,) + D['local'][(ff, c)]] if (ff, c) in D['local'] else D['pub'].get(c, [])
        for (fx, cb, params) in cands:
            if 'Carried *cr' in params:
                continue
            if REG.search(cb) or GLUE.search(cb):
                return True
            todo += [(fx, x) for x in set(re.findall(r'(?<![\w>.])([A-Za-z_]\w*)\s*\(', cb))]
    return False

if EXPORT:
    # each adapter abi_X here gets a carried form X_cr beside it: the
    # adapter's body on carried registers (it may use this file's helpers)
    made = []
    fexp = functions(text)
    out, pos = [], 0
    for m in re.finditer(r'^void abi_(\w+)\(void\)\n\{\n(.*?)^\}\n', text, re.M | re.S):
        n, b = m.group(1), m.group(2)
        if 'Carried r' in b:
            continue
        if re.search(r'\bcpu\.(s|pc|e|unwound)\b|call_routine|jump_routine', b):
            continue
        bad = touches_any(FILE, b, set())
        if bad or re.search(r'\babi_\w+\s*\(', b):
            continue
        if re.search(r'^void %s_cr\(Carried \*cr\)' % n, text, re.M):
            continue
        bb = re.sub(r'\bcpu\.(a|x|y|d|p|db)\b', r'cr->\1', b)
        bb = re.sub(r'\bop_(adc16|sbc16|adc8|sbc8)\(', r'alu_\1(', bb)
        bb = re.sub(r'\bop_(\w+)\(\s*\)', r'r_\1(cr)', bb)
        bb = re.sub(r'\bop_(\w+)\(', r'r_\1(cr, ', bb)
        out.append(text[pos:m.end()])
        out.append('\n/* %s on carried registers, as abi_%s leaves them. */\nvoid %s_cr(Carried *cr)\n{\n%s}\n' % (n, n, n, bb))
        pos = m.end()
        made.append(n)
    out.append(text[pos:])
    res = ''.join(out)
    if made and '"carried.h"' not in res:
        inc = list(re.finditer(r'^#include .*\n', res, re.M))[-1]
        res = res[:inc.end()] + '#include "carried.h"\n' + res[inc.end():]
    open(os.path.join(G, FILE), 'w').write(res)
    print('exported %d: %s' % (len(made), ', '.join(made)))
    sys.exit(0)

fns = functions(text)
byname = {f[2]: f for f in fns}
isroutine = {f[2]: f[2].startswith('abi_') and f[4].strip() in ('void', '') for f in fns}
body = {f[2]: text[f[5]:f[1] - 2] for f in fns}

def typed_elsewhere(n):
    pat = re.compile(r'^(?!static)[A-Za-z_][\w \*]*?\b%s\s*\([^;{]*\)\s*\n\{' % n, re.M)
    return [f for f, t in src.items() if pat.search(t)]

def callers_elsewhere(n):
    pat = re.compile(r'(?<![\w>.])%s\s*\(' % n)
    return [f for f, t in src.items() if f != FILE and pat.search(t)]

# classify (a helper that takes a routine as a parameter calls it in
# registers: glue)
fptypes = set(re.findall(r'typedef\s+void\s*\(\s*\*\s*(\w+)\s*\)\s*\(\s*void\s*\)', text))
def takes_routine(params):
    return re.search(r'\(\s*\*\s*\w+\s*\)\s*\(\s*void\s*\)', params) or \
        any(re.search(r'\b%s\b' % t, params) for t in fptypes)
adapter, regfn, glue = set(), set(), set()
for n, b in body.items():
    if isroutine[n] and re.search(r'(?<![\w])%s\s*\(' % n[4:], b):
        adapter.add(n)
for n, b in body.items():
    if n in adapter:
        continue
    if GLUE.search(b):
        glue.add(n)
    elif REG.search(b) or takes_routine(byname[n][4]) or re.search(r'\babi_\w+\b(?!\s*\()', b):
        regfn.add(n)
# transitive: callers of regfn helpers
changed = True
while changed:
    changed = False
    for n, b in body.items():
        if n in adapter or n in regfn or n in glue:
            continue
        for h in list(regfn):
            if not isroutine[h] and re.search(r'(?<![\w>.])%s\s*\(' % h, b):
                regfn.add(n); changed = True; break
# public typed functions with outside callers can't take the new parameter
left = {}
for n in list(regfn):
    hdr = byname[n][3]
    if isroutine[n]:
        t = n[4:]
        if t in byname and t not in regfn:
            left[n] = 'typed %s exists here' % t; regfn.discard(n); glue.add(n); continue
        ext = [f for f in typed_elsewhere(t) if f != FILE]
        if ext:
            left[n] = 'typed %s in %s' % (t, ext); regfn.discard(n); glue.add(n); continue
    elif not hdr.startswith('static'):
        ext = callers_elsewhere(n)
        if ext:
            left[n] = 'called from %s' % ext; regfn.discard(n); glue.add(n)
newname = {n: (n[4:] if isroutine[n] else n) for n in regfn}

# wrappers for calls of other routines' adapters
def find_adapter(n):
    for f, t in src.items():
        m = re.search(r'^void abi_%s\(void\)\n\{\n(.*?)^\}\n' % n, t, re.M | re.S)
        if m:
            return f, m.group(1)
    return None, None

def statics(f):
    t = src[f]
    out = set(re.findall(r'^static [\w\s\*]*?\b(\w+)\s*\(', t, re.M))
    for m in re.finditer(r'^static (?:const )?(?:struct\s+\w+\s+|\w+\s+)+\**\s*([^;(]*);', t, re.M):
        for d in m.group(1).split(','):
            w = re.match(r'\s*\**\s*(\w+)', d)
            if w:
                out.add(w.group(1))
    out |= set(re.findall(r'^static [\w\s\*]*?\b(\w+)\s*(?:\[|=)', t, re.M))
    return out

def touches_regs(f, b):
    return touches_any(f, b, regfn if f == FILE else set())

def _old_touches_regs(f, b):
    """Whether anything b calls in file f (transitively, adapters aside)
    uses the register file or dispatches."""
    t = src[f]
    fl = {d[2]: t[d[5]:d[1] - 2] for d in functions(t)}
    seen, todo = set(), set(re.findall(r'(?<![\w>.])([A-Za-z_]\w*)\s*\(', b))
    while todo:
        c = todo.pop()
        if c in seen or c not in fl:
            continue
        seen.add(c)
        cb = fl[c]
        if f == FILE and c in regfn:
            continue                    # converted here: called carried
        if REG.search(cb) or GLUE.search(cb):
            return True
        todo |= set(re.findall(r'(?<![\w>.])([A-Za-z_]\w*)\s*\(', cb))
    return False

wrap, wrapcode, syncw = {}, [], set()
carried_ext = set()
def sync_wrapper(n):
    wrapcode.append('''/* %s, still in registers: its call with the registers loaded and taken
   back. */
static void w_%s(Carried *cr)
{
    cpu.a = cr->a;
    cpu.x = cr->x;
    cpu.y = cr->y;
    cpu.d = cr->d;
    cpu.p = cr->p;
    cpu.db = cr->db;
    abi_%s();
    cr->a = cpu.a;
    cr->x = cpu.x;
    cr->y = cpu.y;
    cr->d = cpu.d;
    cr->p = cpu.p;
    cr->db = cpu.db;
}
''' % (n, n, n))
    syncw.add(n)

def make_wrapper(n):
    if n in wrap:
        return wrap[n]
    if 'abi_' + n in regfn and isroutine['abi_' + n]:
        wrap[n] = newname['abi_' + n] + '(cr)'
        return wrap[n]
    wrap[n] = 'w_%s(cr)' % n
    for t in src.values():
        if re.search(r'^void %s_cr\(Carried \*cr\)' % n, t, re.M):
            wrap[n] = '%s_cr(cr)' % n
            carried_ext.add(n + '_cr')
            return wrap[n]
    f, b = find_adapter(n)
    if b is not None and re.search(r'Carried r = \{cpu\.a, cpu\.x, cpu\.y, cpu\.d, cpu\.p, cpu\.db\};\s*%s\(&r\);' % n, b):
        wrap[n] = '%s(cr)' % n          # a routine on carried registers already
        carried_ext.add(n)
        return wrap[n]
    ok = b is not None and (f == FILE or re.search(r'(?<![\w])%s\s*\(' % n, b))
    if ok and f != FILE:
        st = statics(f)
        used = set(re.findall(r'\b([A-Za-z_]\w*)\b', b))
        if used & st:
            ok = False
    if ok and GLUE.search(b):
        ok = False
    if ok and touches_regs(f, b):
        ok = False
    if not ok:
        sync_wrapper(n)
        return wrap[n]
    bb = rewrite_body(b) if f == FILE else b
    for c in re.findall(r'\babi_(\w+)\s*\(\s*\)', bb):
        call = make_wrapper(c)
        bb = re.sub(r'\babi_%s\s*\(\s*\)' % c, call, bb)
    bb = re.sub(r'\bcpu\.(a|x|y|d|p|db)\b', r'cr->\1', bb)
    bb = re.sub(r'\bop_(adc16|sbc16|adc8|sbc8)\(', r'alu_\1(', bb)
    bb = re.sub(r'\bop_(\w+)\(\s*\)', r'r_\1(cr)', bb)
    bb = re.sub(r'\bop_(\w+)\(', r'r_\1(cr, ', bb)
    wrapcode.append('/* %s as its adapter leaves the registers. */\nstatic void w_%s(Carried *cr)\n{\n%s}\n' % (n, n, bb))
    return wrap[n]

# other files' public helpers that work on the register file: their
# carried forms, or a stop
EXTMAP = {'approach_regs': 'r_approach', 'approach_regs_byte': 'r_approach_byte'}
EXTREG = set()
for f, t in src.items():
    if f == FILE:
        continue
    for (s0, e, n, hdr, params, bs) in functions(t):
        if not hdr.startswith('static') and not n.startswith('abi_') and REG.search(t[bs:e]):
            EXTREG.add(n)
stops = set()

cvariants = set()
def rewrite_body(b):
    for n in EXTMAP:
        b = re.sub(r'(?<![\w>.])%s\s*\(' % n, EXTMAP[n] + '(cr, ', b)
    for n in EXTREG - set(EXTMAP):
        if re.search(r'(?<![\w>.])%s\s*\(' % n, b):
            stops.add(n)
    # calls of helpers still in registers: through their _c variants, which
    # load the registers and take them back
    for n in glue:
        if not isroutine[n] and n in byname:
            if re.search(r'(?<![\w>.])%s\s*\(' % n, b):
                cvariants.add(n)
                b = re.sub(r'(?<![\w>.])%s\s*\(\s*\)' % n, '%s_c(cr)' % n, b)
                b = re.sub(r'(?<![\w>.])%s\s*\((?!cr\b)' % n, '%s_c(cr, ' % n, b)
    # calls of converted helpers and routines
    for n in regfn:
        nn = newname[n]
        if isroutine[n]:
            b = re.sub(r'\b%s\s*\(\s*\)' % n, '%s(cr)' % nn, b)
        else:
            b = re.sub(r'(?<![\w>.])%s\s*\(\s*\)' % n, '%s(cr)' % n, b)
            b = re.sub(r'(?<![\w>.])%s\s*\((?!cr\b)' % n, '%s(cr, ' % n, b)
    for c in sorted(set(re.findall(r'\babi_(\w+)\s*\(\s*\)', b))):
        b = re.sub(r'\babi_%s\s*\(\s*\)' % c, make_wrapper(c), b)
    b = routine_values(b)
    b = re.sub(r'\bcpu\.(a|x|y|d|p|db)\b', r'cr->\1', b)
    b = re.sub(r'\bop_(adc16|sbc16|adc8|sbc8)\(', r'alu_\1(', b)
    b = re.sub(r'\bop_(\w+)\(\s*\)', r'r_\1(cr)', b)
    b = re.sub(r'\bop_(\w+)\(', r'r_\1(cr, ', b)
    return b

def rparams(params):
    """The names of a parameter list's routine parameters."""
    out = []
    for p in split_params(params) if params.strip() not in ('void', '') else []:
        m = re.search(r'\(\s*\*\s*(\w+)\s*\)\s*\(\s*void\s*\)', p)
        if m:
            out.append(m.group(1))
        elif any(re.search(r'\b%s\b' % t, p) for t in fptypes):
            out.append(re.findall(r'(\w+)\s*$', p)[0])
    return out

def carried_value(c):
    """The carried function for routine abi_c used as a value."""
    return re.sub(r'\(cr\)$', '', make_wrapper(c))

def routine_values(b):
    # routine types: void (*)(void) and its typedefs become carried routines
    b = re.sub(r'\(\s*\*\s*(\w*)\s*(\[[^\]]*\])?\s*\)\s*\(\s*void\s*\)', lambda m: '(*%s%s)(Carried *)' % (m.group(1), m.group(2) or ''), b)
    for t in fptypes:
        b = re.sub(r'\b%s\b' % t, 'CarriedFn', b)
    for c in sorted(set(re.findall(r'\babi_(\w+)\b(?!\s*\()', b))):
        b = re.sub(r'\babi_%s\b(?!\s*\()' % c, carried_value(c), b)
    return b

def glue_body(b):
    # glue keeps the register file; its calls of converted helpers load and
    # take back the registers (see the _g variants); routines it passes to
    # them go as carried routines
    for n in regfn:
        if not isroutine[n]:
            b = re.sub(r'(?<![\w>.])%s\s*\(' % n, '%s_g(' % n, b)
            if rparams(byname[n][4]):
                out, pos = [], 0
                for m in re.finditer(r'(?<![\w>.])%s_g\s*\(' % n, b):
                    depth, i = 1, m.end()
                    while depth:
                        depth += {'(': 1, ')': -1}.get(b[i], 0)
                        i += 1
                    out.append(b[pos:m.end()])
                    args = b[m.end():i]
                    for c in sorted(set(re.findall(r'\babi_(\w+)\b(?!\s*\()', args))):
                        args = re.sub(r'\babi_%s\b(?!\s*\()' % c, carried_value(c), args)
                    out.append(args)
                    pos = i
                out.append(b[pos:])
                b = ''.join(out)
    return b

def find_def(t, name, carried):
    """The end of name's definition in t (the one taking Carried *cr, or not)."""
    for m in HDR.finditer(t):
        if m.group(2) == name and (('Carried *cr' in m.group(3)) == carried):
            return t.index('\n}\n', m.end() - 1) + 3
    raise SystemExit('no definition of %s' % name)

def pname(p):
    m = re.search(r'\(\s*\*\s*(\w+)\s*\)', p)
    return m.group(1) if m else re.findall(r'(\w+)\s*(?:\[\])?$', p)[0]

def split_params(params):
    out, depth, cur = [], 0, ''
    for ch in params:
        if ch == '(':
            depth += 1
        elif ch == ')':
            depth -= 1
        if ch == ',' and depth == 0:
            out.append(cur.strip()); cur = ''
        else:
            cur += ch
    if cur.strip():
        out.append(cur.strip())
    return out

gvariants = set()
out, pos = [], 0
for (s0, e, n, hdr, params, bs) in fns:
    out.append(text[pos:s0])
    pos = e
    seg = text[s0:e]
    if n in regfn:
        b = rewrite_body(body[n])
        if isroutine[n]:
            nn = newname[n]
            out.append('void %s(Carried *cr)\n{\n%s}\n' % (nn, b))
            out.append('''
void %s(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    %s(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}
''' % (n, nn))
        else:
            p = params.strip()
            np = 'Carried *cr' if p in ('void', '') else 'Carried *cr, ' + routine_values(params)
            for fp in rparams(params):
                b = re.sub(r'(?<![\w>.])%s\s*\(\s*\)' % fp, '%s(cr)' % fp, b)
            out.append('%s(%s)\n{\n%s}\n' % (hdr.rstrip(), np, b))
    elif n in glue or n in adapter:
        b = glue_body(body[n])
        if b != body[n]:
            for h in regfn:
                if not isroutine[h] and re.search(r'(?<![\w>.])%s_g\s*\(' % h, b):
                    gvariants.add(h)
        out.append('%s(%s)\n{\n%s}\n' % (hdr.rstrip(), params, b))
    else:
        out.append(seg)
out.append(text[pos:])
res = ''.join(out)

# macros that call converted helpers get a carried twin NAME_c (used in
# converted code); the original calls the helpers' register forms (_g)
macros = {}
def fix_macro(m):
    line = m.group(0)
    mm = re.match(r'#define (\w+)', line)
    hits = [n for n in regfn if not isroutine[n] and re.search(r'(?<![\w>.])%s\s*\(' % n, line)]
    if not mm or not hits:
        return line
    name = mm.group(1)
    macros[name] = hits
    twin = line.replace('#define %s' % name, '#define %s_c' % name, 1)
    orig = line
    for n in hits:
        twin = re.sub(r'(?<![\w>.])%s\s*\(\s*\)' % n, '%s(cr)' % n, twin)
        twin = re.sub(r'(?<![\w>.])%s\s*\((?!cr\b)' % n, '%s(cr, ' % n, twin)
        orig = re.sub(r'(?<![\w>.])%s\s*\(' % n, '%s_g(' % n, orig)
    return orig + '\n' + twin
res = re.sub(r'^#define [^\n]*(?:\\\n[^\n]*)*', fix_macro, res, flags=re.M)
for name, hits in macros.items():
    for n in hits:
        if n not in gvariants:
            gvariants.add(n)
# converted bodies use the twins
def use_twins(t):
    for (s0, e, n, hdr, params, bs) in functions(t):
        pass
    return t
if macros:
    out2, pos2 = [], 0
    for m in HDR.finditer(res):
        e = res.find('\n}\n', m.end() - 1) + 3
        if 'Carried *cr' in m.group(3) or m.group(2).startswith('w_'):
            seg = res[m.start():e]
            for name in macros:
                seg = re.sub(r'(?<![\w>.])%s\s*\(' % name, '%s_c(' % name, seg)
            out2.append(res[pos2:m.start()]); out2.append(seg); pos2 = e
    out2.append(res[pos2:])
    res = ''.join(out2)

# _g variants for glue callers, after each helper's definition
for h in sorted(gvariants):
    f = byname[h]
    hdr, params = f[3], f[4]
    ret = re.sub(r'\b%s\s*$' % h, '', hdr.strip()).strip()
    plist = [] if params.strip() in ('void', '') else split_params(params)
    names = [pname(p) for p in plist]
    call = '%s(&c%s)' % (h, ''.join(', ' + x for x in names))
    params = routine_values(params)
    isvoid = re.sub(r'\bstatic\b|\binline\b', '', ret).strip() == 'void'
    g = '''%s %s_g(%s)
{
    Carried c = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};
    %s%s;

    cpu.a = c.a;
    cpu.x = c.x;
    cpu.y = c.y;
    cpu.d = c.d;
    cpu.p = c.p;
    cpu.db = c.db;%s
}
''' % (ret, h, params, '' if isvoid else re.sub(r'\bstatic\b|\binline\b', '', ret).strip() + ' ret_ = ', call,
       '' if isvoid else '\n    return ret_;')
    e = find_def(res, h, True)
    res = res[:e] + '\n' + g + res[e:]

# _c variants: a helper still in registers called from carried code
for h in sorted(cvariants):
    f = byname[h]
    hdr, params = f[3], f[4]
    ret = re.sub(r'\b%s\s*$' % h, '', hdr.strip()).strip()
    rt = re.sub(r'\bstatic\b|\binline\b', '', ret).strip()
    plist = [] if params.strip() in ('void', '') else split_params(params)
    names = [pname(p) for p in plist]
    call = '%s(%s)' % (h, ', '.join(names))
    np = 'Carried *cr' if not plist else 'Carried *cr, ' + params
    g = '''static %s %s_c(%s)
{
    cpu.a = cr->a;
    cpu.x = cr->x;
    cpu.y = cr->y;
    cpu.d = cr->d;
    cpu.p = cr->p;
    cpu.db = cr->db;
    %s%s;
    cr->a = cpu.a;
    cr->x = cpu.x;
    cr->y = cpu.y;
    cr->d = cpu.d;
    cr->p = cpu.p;
    cr->db = cpu.db;%s
}
''' % (rt, h, np, '' if rt == 'void' else rt + ' ret_ = ', call, '' if rt == 'void' else '\n    return ret_;')
    e = find_def(res, h, False)
    res = res[:e] + '\n' + g + res[e:]

# the header and forward declarations after the includes, the wrappers at
# the end (where every routine of the file is declared)
HEADERS = {}
for hf in os.listdir(os.path.join(G, 'include')):
    if hf.endswith('.h'):
        HEADERS[hf] = open(os.path.join(G, 'include', hf)).read()

def header_of(n):
    for hf, t in HEADERS.items():
        if re.search(r'^[A-Za-z_][\w \*]*\b%s\s*\([^;{]*\)\s*;' % n, t, re.M):
            return hf
    return None

def proto_of(n):
    pat = re.compile(r'^((?!static)[A-Za-z_][\w \*]*?\b%s\s*\([^;{]*\))\s*\n\{' % n, re.M)
    for f, t in src.items():
        m = pat.search(t)
        if m:
            return re.sub(r'\s+', ' ', m.group(1)) + ';'
    return None

if wrapcode or regfn:
    inc = list(re.finditer(r'^#include .*\n', res, re.M))[-1]
    add = '#include "carried.h"\n' if '"carried.h"' not in res else ''
    fwd = ''.join('void %s(Carried *cr);\n' % newname[n] for n in sorted(regfn) if isroutine[n])
    def decl(n):
        for m in HDR.finditer(res):
            if m.group(2) == n and 'Carried *cr' in m.group(3):
                return '%s(%s);\n' % (m.group(1).rstrip(), m.group(3))
        raise SystemExit('no definition of %s' % n)
    fwd += ''.join(decl(n) for n in sorted(regfn) if not isroutine[n])
    # the file's own forward declarations of converted helpers go
    for n in regfn:
        if not isroutine[n]:
            res = re.sub(r'^(?:static )?[A-Za-z_][\w \*]*\b%s\s*\((?!Carried)[^;{)]*\);\n' % n, '', res, flags=re.M)
    fwd += ''.join('static void w_%s(Carried *cr);\n' % n for n in sorted(wrap) if wrap[n].startswith('w_'))
    fwd += ''.join('void %s(Carried *cr);\n' % n for n in sorted(carried_ext))
    wcode = '\n'.join(wrapcode)
    protos = []
    for c in sorted(set(re.findall(r'(?<![\w>.])([A-Za-z_]\w*)\s*\(', wcode + ''.join(body[n] for n in regfn)))):
        if c.startswith(('abi_', 'w_', 'r_', 'alu_')) or c in byname:
            continue
        if re.search(r'^[A-Za-z_][\w \*]*\b%s\s*\([^;{]*\)\s*;' % c, res, re.M):
            continue
        hf = header_of(c)
        if hf:
            if ('"%s"' % hf) not in res:
                protos.append('#include "%s"\n' % hf)
            continue
        pr = proto_of(c)
        if pr:
            protos.append(pr + '\n')
    absyn = ''.join('void abi_%s(void);\n' % n for n in sorted(syncw)
                    if not re.search(r'^void abi_%s\(void\);' % n, res, re.M))
    # the declarations before the first function that uses any of them
    names = [newname[n] for n in regfn] + ['w_' + n for n in wrap if wrap[n].startswith('w_')]
    use = re.compile(r'(?<![\w>.])(%s)\s*\(' % '|'.join(map(re.escape, names))) if names else None
    at = inc.end()
    if use:
        for m in HDR.finditer(res):
            e = res.find('\n}\n', m.end() - 1)
            if use.search(res, m.start(), e if e > 0 else len(res)):
                at = max(inc.end(), m.start())
                c = res.rfind('/*', inc.end(), at)
                if c >= 0 and res.find('*/', c) < at and res[res.find('*/', c) + 2:at].strip() == '':
                    at = c
                break
    res = res[:inc.end()] + add + '\n' + ''.join(protos) + absyn + res[inc.end():at] + fwd + '\n' + res[at:] + '\n' + wcode
# tidy: a macro whose register form nothing uses gives its name back to
# the twin; register variants nothing calls go
for name in list(macros):
    rest = re.sub(r'^#define %s(?:_c)?\b[^\n]*\n' % name, '', res, flags=re.M)
    if not re.search(r'(?<![\w])%s\s*\(' % name, rest):
        res = re.sub(r'^#define %s\b[^\n]*\n' % name, '', res, flags=re.M)
        res = re.sub(r'(?<![\w])%s_c\b' % name, name, res)
changed = True
while changed:
    changed = False
    for m in list(HDR.finditer(res)):
        n = m.group(2)
        if not (n.endswith('_g') or n.endswith('_c')) or not m.group(1).startswith('static'):
            continue
        if len(re.findall(r'(?<![\w])%s\b' % n, res)) == 1:
            e = res.index('\n}\n', m.end() - 1) + 3
            res = res[:m.start()] + res[e:].lstrip('\n')
            changed = True
            break

if stops:
    sys.exit('stopped: converted code calls register helpers of other files: %s' % ', '.join(sorted(stops)))
open(os.path.join(G, FILE), 'w').write(res)
print('converted %d, glue %d (%s), sync wrappers %d (%s)' % (len(regfn), len(glue), ', '.join(sorted(glue)), len(syncw), ', '.join(sorted(syncw))))
for n, w in left.items():
    print('left', n, w)
