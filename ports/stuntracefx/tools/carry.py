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
FILE = sys.argv[1]
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
GLUE = re.compile(r'\b(call_routine|jump_routine|jump_routine_pushed|cpu_restart|cpu_loop_forever)\b|\bcpu\.(s|pc|e|unwound)\b|\babi_\w+\b(?!\s*\()')

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

# classify
adapter, regfn, glue = set(), set(), set()
for n, b in body.items():
    if isroutine[n] and re.search(r'(?<![\w])%s\s*\(' % n[4:], b):
        adapter.add(n)
for n, b in body.items():
    if n in adapter:
        continue
    if GLUE.search(b):
        glue.add(n)
    elif REG.search(b):
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
    return set(re.findall(r'^static [\w\s\*]*?\b(\w+)\s*\(', t, re.M)) | \
           set(re.findall(r'^static [\w\s\*]*?\b(\w+)\s*(?:\[|=|;)', t, re.M))

wrap, wrapcode, syncw = {}, [], set()
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
    f, b = find_adapter(n)
    ok = b is not None and (f == FILE or re.search(r'(?<![\w])%s\s*\(' % n, b))
    if ok and f != FILE:
        st = statics(f)
        used = set(re.findall(r'\b([A-Za-z_]\w*)\b', b))
        if used & st:
            ok = False
    if ok and GLUE.search(b):
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

cvariants = set()
def rewrite_body(b):
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
    b = re.sub(r'\bcpu\.(a|x|y|d|p|db)\b', r'cr->\1', b)
    b = re.sub(r'\bop_(adc16|sbc16|adc8|sbc8)\(', r'alu_\1(', b)
    b = re.sub(r'\bop_(\w+)\(\s*\)', r'r_\1(cr)', b)
    b = re.sub(r'\bop_(\w+)\(', r'r_\1(cr, ', b)
    return b

def glue_body(b):
    # glue keeps the register file; its calls of converted helpers load and
    # take back the registers (see the _g variants)
    for n in regfn:
        if not isroutine[n]:
            b = re.sub(r'(?<![\w>.])%s\s*\(' % n, '%s_g(' % n, b)
    return b

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
            np = 'Carried *cr' if p in ('void', '') else 'Carried *cr, ' + params
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

# _g variants for glue callers, after each helper's definition
for h in sorted(gvariants):
    f = byname[h]
    hdr, params = f[3], f[4]
    ret = re.sub(r'\b%s\s*$' % h, '', hdr.strip()).strip()
    plist = [] if params.strip() in ('void', '') else [p.strip() for p in params.split(',')]
    names = [re.findall(r'(\w+)\s*(?:\[\])?$', p)[0] for p in plist]
    call = '%s(&c%s)' % (h, ''.join(', ' + x for x in names))
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
''' % (ret, h, params, '' if isvoid else re.sub(r'\bstatic\b|\binline\b', '', ret).strip() + ' v = ', call,
       '' if isvoid else '\n    return v;')
    m = re.search(r'^[^\n]*\b%s\(Carried \*cr[^)]*\)\n\{\n.*?^\}\n' % h, res, re.M | re.S)
    res = res[:m.end()] + '\n' + g + res[m.end():]

# _c variants: a helper still in registers called from carried code
for h in sorted(cvariants):
    f = byname[h]
    hdr, params = f[3], f[4]
    ret = re.sub(r'\b%s\s*$' % h, '', hdr.strip()).strip()
    rt = re.sub(r'\bstatic\b|\binline\b', '', ret).strip()
    plist = [] if params.strip() in ('void', '') else [p.strip() for p in params.split(',')]
    names = [re.findall(r'(\w+)\s*(?:\[\])?$', p)[0] for p in plist]
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
''' % (rt, h, np, '' if rt == 'void' else rt + ' v = ', call, '' if rt == 'void' else '\n    return v;')
    m = re.search(r'^[^\n]*\b%s\((?!Carried)[^)]*\)\n\{\n.*?^\}\n' % h, res, re.M | re.S)
    res = res[:m.end()] + '\n' + g + res[m.end():]

# the header and forward declarations after the includes, the wrappers at
# the end (where every routine of the file is declared)
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
    fwd += ''.join(re.search(r'^([^\n]*\b%s\(Carried \*cr[^)]*\))\n\{\n' % n, res, re.M).group(1) + ';\n'
                   for n in sorted(regfn) if not isroutine[n])
    # the file's own forward declarations of converted helpers go
    for n in regfn:
        if not isroutine[n]:
            res = re.sub(r'^(?:static )?[A-Za-z_][\w \*]*\b%s\s*\((?!Carried)[^;{)]*\);\n' % n, '', res, flags=re.M)
    fwd += ''.join('static void w_%s(Carried *cr);\n' % n for n in sorted(wrap) if wrap[n].startswith('w_'))
    wcode = '\n'.join(wrapcode)
    protos = []
    for c in sorted(set(re.findall(r'(?<![\w>.])([A-Za-z_]\w*)\s*\(', wcode + ''.join(body[n] for n in regfn)))):
        if c.startswith(('abi_', 'w_', 'r_', 'alu_')) or c in byname:
            continue
        if re.search(r'^[^\n]*\b%s\s*\([^;{]*\)\s*;' % c, res, re.M):
            continue
        pr = proto_of(c)
        if pr:
            protos.append(pr + '\n')
    absyn = ''.join('void abi_%s(void);\n' % n for n in sorted(syncw)
                    if not re.search(r'^void abi_%s\(void\);' % n, res, re.M))
    res = res[:inc.end()] + add + '\n' + ''.join(protos) + absyn + fwd + res[inc.end():] + '\n' + wcode
open(os.path.join(G, FILE), 'w').write(res)
print('converted %d, glue %d (%s), sync wrappers %d (%s)' % (len(regfn), len(glue), ', '.join(sorted(glue)), len(syncw), ', '.join(sorted(syncw))))
for n, w in left.items():
    print('left', n, w)
