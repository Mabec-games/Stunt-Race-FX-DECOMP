#!/usr/bin/env python3
"""wramname.py: plain C, step 2, the work-RAM names.

    wramname.py header        write game/include/wram_names.h
    wramname.py FILE...       name FILE's work-RAM addresses (relative to game/)
    wramname.py --meter       the work-RAM addresses the C still spells as numbers

The names are the decomp's (asm/stuntracefx/include/wram.inc), for S-CPU work
RAM only (direct page, $0000-$1FFF, banks $7E and $7F): WRAM_<name>, written
W(name) in the C, as the asm spells it ($0D2B a bank-0 address, $2001 the
data bank's, $7F3408 a long one). An address the C spells as a number
becomes its name where exactly one name has that address: `b | 0x0D2B` (the
data bank's) and 0x000D2B, 0x7E0D2B or 0x7F3408 (long) as accesses spell
them. Addresses with two names, and those inside a named record, stay
numbers for now (the meter counts them). The rewrite changes no compiled
byte; check it so (cc -c before and after, cmp).
"""
import glob, os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
G = os.path.join(ROOT, 'ports', 'stuntracefx', 'game')
INC = os.path.join(ROOT, 'asm', 'stuntracefx', 'include', 'wram.inc')
HDR = os.path.join(G, 'include', 'wram_names.h')


def entries():
    """(name, value, comment lines, line comment, asm files) for each S-CPU
    work-RAM name; the asm files are those its comment block cites."""
    out, com, files, prev_com = [], [], [], False
    short = set()
    for line in open(INC):
        m = re.match(r'!\w+\s*=\s*\$([0-9A-Fa-f]{1,4})\b', line.strip())
        if m:
            short.add(int(m.group(1), 16))
    for line in open(INC):
        s = line.strip()
        if s.startswith(';'):
            if not prev_com:
                files = []
            com.append(s[1:].strip())
            files += re.findall(r'\b(\w+/\w+)\.asm\b', s)
            prev_com = True
            continue
        prev_com = False
        m = re.match(r'!(\w+)\s*=\s*\$([0-9A-Fa-f]+)\s*(?:;\s*(.*))?$', s)
        if not m:
            com = []
            continue
        v = int(m.group(2), 16)
        bank = v >> 16
        # bank 0 written long ($0010AA, for the asm's long addressing) is the
        # same low work RAM, when no short spelling names it already
        long0 = bank == 0 and len(m.group(2)) > 4 and 0 < v < 0x2000 and v not in short
        if (bank == 0 and len(m.group(2)) <= 4) or long0 or bank in (0x7E, 0x7F):
            out.append((m.group(1), v, com, m.group(3), list(files)))
        com = []
    return out


def is_wram(v, db):
    """Work RAM: below $2000 in any form; the data bank's $2000-$7FFF but
    the registers ($2100-$21FF, $3000-$34FF, $4000-$43FF: the data bank
    the game runs with there is a system bank); banks $7E and $7F."""
    if v < 0x2000:
        return True
    if db:
        return v < 0x8000 and not (0x2100 <= v < 0x2200 or 0x3000 <= v < 0x3500 or 0x4000 <= v < 0x4400)
    return v >> 16 in (0x7E, 0x7F)


def auto_name(v):
    if 0x2000 <= v < 0x2062:
        return 'obj_%02x' % (v - 0x2000)
    if v < 0x100:
        return 'dp_%02x' % v
    return 'wram_%04X' % v if v < 0x10000 else 'wram_%06X' % v


NUM_DB = re.compile(r'\bb \| 0x([0-9A-Fa-f]{4})\b(?!u)')
# the same through the data bank's other spellings: db(cr) | 0x0D2B,
# (uint32_t)bank << 16 | 0x0D2B, a file's dbw(bank, 0x0D2B) helper
NUM_DBX = re.compile(r'(\bdb\(cr\)\)? \| |\(uint32_t\)(?:bank|cr->db) << 16\)? \| |\b(?:dbw|w|abs_db)\([^,()]+, *)0x([0-9A-Fa-f]{4})\b(?!u)')
# (not a bank's base, $xx0000: a base an offset or pointer is added to)
NUM_LONG = re.compile(r'(\(|, )0x((?:00|7E|7F)(?!0000)[0-9A-Fa-f]{4})u?\b(?! *[*<>]| \| W\()')


def numbers(t):
    """The work-RAM addresses text t spells as numbers, in the form the
    names take ($0D2B, $2025 for the data bank's, $7F3408)."""
    out = []
    for m in NUM_DB.finditer(t):
        v = int(m.group(1), 16)
        if is_wram(v, True):
            out.append(v)
    for m in NUM_DBX.finditer(t):
        v = int(m.group(2), 16)
        if is_wram(v, True):
            out.append(v)
    for m in NUM_LONG.finditer(t):
        v = int(m.group(2), 16)
        if v >> 16 == 0 and v < 0x2000:
            out.append(v)
        elif v >> 16 == 0x7E and (v & 0xFFFF) < 0x2000:
            out.append(v & 0xFFFF)
        elif v >> 16 in (0x7E, 0x7F):
            out.append(v)
    return out


def write_header():
    known = {v for _, v, _, _, _ in entries()}
    names = {n for n, _, _, _, _ in entries()}
    extra = set()
    for p in glob.glob(os.path.join(G, '*', '*.c')):
        t = re.sub(r'/\*.*?\*/', '', open(p).read(), flags=re.S)
        extra |= {v for v in numbers(t) if v not in known}
        extra |= {int(x, 16) for x in re.findall(r'\bW\(wram_([0-9A-F]{4,6})\)', t)
                  if 'wram_' + x not in names}
        extra |= {int(x, 16) for x in re.findall(r'\bW\(dp_([0-9a-f]{2})\)', t)
                  if 'dp_' + x not in names}
        extra |= {0x2000 + int(x, 16) for x in re.findall(r'\b(?:W|OBJ_OFF)\(obj_([0-9a-f]{2})\)', t)
                  if 'obj_' + x not in names}
    lines = ['/*', ' * wram_names.h: the S-CPU\'s work RAM by the decomp\'s names (generated by',
             ' * ports/stuntracefx/tools/wramname.py from asm/stuntracefx/include/wram.inc).',
             ' * W(name) is the address as the asm spells it: a bank-0 address below $2000',
             ' * (direct page and low work RAM, which every bank the game runs in mirrors),',
             ' * an address in the data bank from $2000, or a long one in bank $7E or $7F.',
             ' */', '#ifndef STUNTRACEFX_WRAM_NAMES_H', '#define STUNTRACEFX_WRAM_NAMES_H', '',
             '#include <stdint.h>', '',
             '#define W(name) WRAM_##name',
             '/* An object-table field\'s offset in its record (the name of record 0\'s field). */',
             '#define OBJ_OFF(name) ((uint16_t)W(name) - W(obj_base))', '']
    for n, v, com, tail, _ in entries():
        for c in com:
            lines.append('/* %s */' % c.replace('*/', '* /') if c else '')
        lines.append('#define WRAM_%s 0x%06Xu%s' % (n, v, ('  /* %s */' % tail.replace('*/', '* /')) if tail else ''))
    if extra:
        lines += ['', '/* Work RAM the C uses that wram.inc does not name, or names only for other']
        lines += ['   files\' use: the address as its name. */']
        for v in sorted(extra):
            if auto_name(v) not in names:
                lines.append('#define WRAM_%s 0x%06Xu' % (auto_name(v), v))
    lines += ['', '#endif', '']
    text = re.sub(r'\n\n\n+', '\n\n', '\n'.join(lines))
    open(HDR, 'w').write(text)


def table(cfile):
    """Address -> name for C file cfile (relative to game/): the one name an
    address has; of several, the one whose comment cites cfile's asm, else
    its generic name (dp_XX, wram_XXXX: scratch the other files name for
    their own use); an address wram.inc does not name, its address as its
    name."""
    asm = os.path.splitext(cfile)[0]
    by = {}
    for n, v, _, _, files in entries():
        by.setdefault(v, []).append((n, files))
    names = {}
    for v, ns in by.items():
        if len(ns) == 1:
            names[v] = ns[0][0]
        else:
            mine = [n for n, files in ns if asm in files]
            generic = [n for n, _ in ns if re.fullmatch(r'(dp_[0-9a-f]{2}|wram_[0-9A-Fa-f]{4,6})', n)]
            if len(mine) == 1:
                names[v] = mine[0]
            elif not mine and len(generic) == 1:
                names[v] = generic[0]        # scratch others name for their own use
            elif not mine and len([n for n, files in ns if not files]) == 1:
                names[v] = [n for n, files in ns if not files][0]   # the global one
            elif not mine:
                names[v] = auto_name(v)      # scratch only others name
    return names, set(by)


ACCESS = r'(?:rd8|rd16|rd24|wr8|wr16|reg_rd|reg_rd16|r_\w+|cyc_then|io_read_time|hw_rd|hw_wr)'


def name_text(t, names, known):
    def pick(v):
        if v in names:
            return names[v]
        if v not in known:
            return auto_name(v)
        return None                          # several names, none this file's

    def db(m):
        v = int(m.group(1), 16)
        n = pick(v) if is_wram(v, True) else None
        return 'b | W(%s)' % n if n else m.group(0)
    t = NUM_DB.sub(db, t)

    def dbx(m):
        v = int(m.group(2), 16)
        n = pick(v) if is_wram(v, True) else None
        return m.group(1) + 'W(%s)' % n if n else m.group(0)
    t = NUM_DBX.sub(dbx, t)

    def lg(m):
        v = int(m.group(2), 16)
        bank, off = v >> 16, v & 0xFFFF
        if bank == 0 and off < 0x2000:
            n = pick(off)
            name = 'W(%s)' % n if n else None
        elif bank == 0x7E and off < 0x2000:
            n = pick(off)
            name = '(0x7E0000 | W(%s))' % n if n else None
        elif bank == 0x7E and off in names and v not in names and v not in known:
            name = '(0x7E0000 | W(%s))' % names[off]
        elif bank in (0x7E, 0x7F):
            n = pick(v)
            name = 'W(%s)' % n if n else None
        else:
            name = None
        return m.group(1) + name if name else m.group(0)
    return NUM_LONG.sub(lg, t)


def ensure_include(t):
    if '"wram_names.h"' in t:
        return t
    inc = list(re.finditer(r'^#include ".*"\n', t, re.M))
    if not inc:
        return t
    return t[:inc[-1].end()] + '#include "wram_names.h"\n' + t[inc[-1].end():]


def meter(paths):
    total, rows = 0, []
    for p in paths:
        text = re.sub(r'/\*.*?\*/', '', open(p).read(), flags=re.S)
        n = len(numbers(text))
        if n:
            rows.append((n, os.path.relpath(p, G)))
            total += n
    for n, p in sorted(rows, reverse=True):
        print('%6d  %s' % (n, p))
    print('%6d  in %d files: work-RAM addresses spelled as numbers' % (total, len(rows)))
    fields = sum(len(FIELD.findall(re.sub(r'/\*.*?\*/', '', open(p).read(), flags=re.S))) for p in paths)
    fields += sum(len(OFFSET.findall(re.sub(r'/\*.*?\*/', '', open(p).read(), flags=re.S))) for p in paths)
    print('%6d  object-table fields spelled as numbers' % fields)


MACRO = re.compile(r'^(?:/\*[^\n]*\*/\n)?#define\s+([A-Z_][A-Z0-9_]*)\s+(0x[0-9A-Fa-f]+)u?\b[ \t]*(?:/\*[^\n]*\*/)?[ \t]*\n', re.M)


def macro_value_name(v, digits, names, known):
    """The W() spelling for a macro's value, or None if it is not work RAM."""
    def pick(x):
        if x in names:
            return names[x]
        return auto_name(x) if x not in known else None
    if v >> 16 in (0x7E, 0x7F) and digits == 6:
        if v >> 16 == 0x7E and (v & 0xFFFF) < 0x2000:
            n = pick(v & 0xFFFF)
            return '(0x7E0000 | W(%s))' % n if n else None
        n = pick(v)
        return 'W(%s)' % n if n else None
    if v < 0x2000 and (digits == 6 or v >= 0x100 or digits == 4):
        n = pick(v)
        return 'W(%s)' % n if n else None
    if digits <= 4 and is_wram(v, True):
        n = pick(v)
        return 'W(%s)' % n if n else None
    return None


def name_macros(t, names, known):
    """A file's own #define of a work-RAM address: its uses become the name,
    the define goes."""
    for m in list(MACRO.finditer(t)):
        mac, num = m.group(1), m.group(2)
        v = int(num, 16)
        if v == 0 or len(num) - 2 not in (4, 6):
            continue
        spell = macro_value_name(v, len(num) - 2, names, known)
        if not spell:
            continue
        body = t.replace(m.group(0), '', 1)
        body = re.sub(r'(?<![\w.>])%s\b(?!\s*\()' % mac, spell, body)
        t = body
    return t


FIELD = re.compile(r'(\(uint16_t\)\(|\bF\()0x20([0-5][0-9A-Fa-f]|6[01])(?= \+ |, )')


def name_fields(t, names, known):
    """Object-table fields as the asm spells them (LDA !obj_pos_x,X): the
    record pointer plus record 0's field address, $2000 + the field, in
    the two spellings the C has for it, (uint16_t)(0x200C + x) and
    F(0x200C, o). A field wram.inc does not name: obj_xx."""
    def f(m):
        v = 0x2000 + int(m.group(2), 16)
        n = names.get(v) or names.get(0x7E0000 | v)
        if not n:
            n = 'obj_%02x' % (v - 0x2000)
        return m.group(1) + 'W(%s)' % n
    return FIELD.sub(f, t)


OFFSET = re.compile(r'\b(field\(bank, [\w>-]+, |OBJ\([\w>-]+, |OBJ_FIELD\([\w>-]+, )0x([0-5][0-9A-Fa-f]|6[01])\)')


def name_offsets(t, names, known):
    """Object-table fields the C's helpers take as an offset in the record
    (field(bank, obj, 0x48), OBJ(o, 0x26), OBJ_FIELD(o, 0x0C)): OBJ_OFF(name)."""
    def f(m):
        v = 0x2000 + int(m.group(2), 16)
        n = names.get(v) or names.get(0x7E0000 | v) or 'obj_%02x' % (v - 0x2000)
        return m.group(1) + 'OBJ_OFF(%s))' % n
    return OFFSET.sub(f, t)


RECORD = re.compile(r'(\(uint16_t\)\((?:\*?x|rec|cr->x|r|obj) \+ |\b(?:ACTOR|CAM|REC)\((?:x|r), )0x([0-9A-Fa-f]{1,2})(?=\))')
# Records other than the object table, by the prefix of their fields'
# names; names on the same offsets that are not fields are left out.
FAMILIES = {'actor': 'actor_', 'cam': 'cam_', 'draw': 'draw_', 'tie': 'tie_'}
NOT_FIELDS = {'cam_tmp_a', 'cam_tmp_b', 'cam_tmp_c', 'cam_target_obj', 'cam_mode', 'cam_lag',
              'cam_offset_x', 'cam_offset_y', 'cam_offset_z', 'draw_count', 'draw_object',
              'draw_next', 'draw_side'}


def name_records(t, family):
    """Fields of a record table the asm spells as names on an index
    (LDA !actor_0a,X; LDA !cam_turn,X): the record pointer plus the field's
    name, (uint16_t)(x + W(actor_0a)), CAM(r, W(cam_turn)). An offset with
    no name of its own is a word or byte into the field below it, as the
    asm writes it (!cam_lead+2,X): W(cam_lead) + 2."""
    by = {}
    for n, v, *_ in entries():
        if n.startswith(FAMILIES[family]) and n not in NOT_FIELDS and v < 0x100:
            by.setdefault(v, []).append(n)

    def f(m):
        v = int(m.group(2), 16)
        for k in (0, 1, 2, 3, 4):
            ns = by.get(v - k)
            if ns:
                break
        if len(ns) != 1:
            raise SystemExit('no single %s field at %s: %s' % (family, m.group(0), ns))
        return m.group(1) + 'W(%s)' % ns[0] + (' + %d' % k if k else '')
    return RECORD.sub(f, t)


DP = re.compile(r'\(uint16_t\)\(((?:cr->)?d) \+ 0x([0-9A-Fa-f]{2})\)')


def name_dp(t, names, known):
    """Direct-page addresses taken from D (LDA $F1 with D the page):
    (uint16_t)(d + W(digit_count)), by the name the address has in this
    file's asm."""
    def f(m):
        v = int(m.group(2), 16)
        n = names.get(v)
        if not n:
            raise SystemExit('no name for direct page %s' % m.group(0))
        print('  $%02X %s' % (v, n))
        return '(uint16_t)(%s + W(%s))' % (m.group(1), n)
    return DP.sub(f, t)


INDEXED = re.compile(r'\(uint16_t\)\(0x([0-9A-Fa-f]{4}) \+ ')


def asm_spellings(cfile):
    """Value -> the spellings (name, k) cfile's asm gives work-RAM operands:
    !oam_buffer+1 is (oam_buffer, 1) at $0375, !oam_sprite52_x-16 is
    (oam_sprite52_x, -16)."""
    path = os.path.join(ROOT, 'asm', 'stuntracefx', os.path.splitext(cfile)[0] + '.asm')
    if not os.path.exists(path):
        return {}
    val = {n: v for n, v, *_ in entries()}
    out = {}
    for line in open(path):
        code = line.split(';')[0]
        for m in re.finditer(r'!(\w+)(?:([+-])(\$[0-9A-Fa-f]+|\d+))?', code):
            n = m.group(1)
            if n not in val:
                continue
            k = m.group(3)
            k = int(k[1:], 16) if k and k.startswith('$') else int(k or 0)
            k = -k if m.group(2) == '-' else k
            out.setdefault((val[n] + k) & 0xFFFF, set()).add((n, k))
    return out


def name_indexed(t, names, known, spell):
    """A work-RAM table read at an index, (uint16_t)(0x0375 + y): the base
    as the file's asm spells it (!oam_buffer+1,Y), W(oam_buffer) + 1; a
    base the asm does not spell, by its own name if it has one, from $0100
    (below, a number added to an index is a field's offset from a pointer:
    (0x0001 + x) with X a ROM string's address is no table at $0001)."""
    left = []

    def f(m):
        v = int(m.group(1), 16)
        if not is_wram(v, True):
            return m.group(0)
        s = spell.get(v, set())
        if len(s) == 1:
            n, k = next(iter(s))
            return '(uint16_t)(W(%s)%s + ' % (n, (' + %d' % k if k > 0 else ' - %d' % -k) if k else '')
        if v in names and not s and v >= 0x100:
            return '(uint16_t)(W(%s) + ' % names[v]
        left.append('$%04X %s' % (v, sorted(s)))
        return m.group(0)
    t = INDEXED.sub(f, t)
    return t, left


AUTO = re.compile(r'\bW\((wram_[0-9A-F]{4,6}|dp_[0-9a-f]{2})\)')


def repick(t, names, known):
    """Auto names (the address as its name) whose address now has a name
    for this file: that name."""
    real = {n for n, *_ in entries()}

    def f(m):
        a = m.group(1)
        if a in real:
            return m.group(0)
        v = int(a.split('_')[1], 16)
        n = names.get(v)
        return 'W(%s)' % n if n and n != a else m.group(0)
    return AUTO.sub(f, t)


def address_params(t):
    """Function -> the indexes of its parameters text t uses as a work-RAM
    address (b | at, (uint16_t)(at + i), rd16(at), dbw(bank, at))."""
    out = {}
    for m in re.finditer(r'^(?:static )?(?:inline )?[\w ]+?\*?\b(\w+)\(([^)]*)\)\n\{(.*?)^\}', t, re.M | re.S):
        fn, params, body = m.groups()
        for i, p in enumerate(params.split(',')):
            p = p.strip()
            if p in ('', 'void'):
                continue
            P = re.escape(p.split()[-1].lstrip('*'))
            if re.search(r'\b(?:b|db\(cr\)|0x7E0000|bank << 16\)?) \| (?:\(uint16_t\)\()?%s\b' % P, body) or \
               re.search(r'\b(?:rd8|rd16|wr8|wr16|rd24)\(\(uint16_t\)\(%s\b' % P, body) or \
               re.search(r'\b(?:rd8|rd16|wr8|wr16|dbw|w)\((?:bank, )?%s\b[,)]' % P, body):
                out.setdefault(fn, set()).add(i)
    return out


def name_args(t, names, known):
    """Work-RAM addresses passed as numbers to this file's helpers that take
    an address: swap_words(b, W(special_times), ...)."""
    params = address_params(re.sub(r'/\*.*?\*/', lambda m: ' ' * len(m.group(0)), t, flags=re.S))
    left = []

    def call(m):
        fn = m.group(1)
        if fn not in params:
            return m.group(0)
        parts = re.split(r'(,(?![^()]*\)))', m.group(2))
        args = parts[0::2]
        for i in params[fn]:
            if i < len(args):
                a = args[i]
                s = a.strip()
                if re.fullmatch(r'0x[0-9A-Fa-f]{4}', s) and 0 < int(s, 16) < 0x2000:
                    v = int(s, 16)
                    n = names.get(v) or (auto_name(v) if v not in known else None)
                    if n:
                        args[i] = a.replace(s, 'W(%s)' % n)
                    else:
                        left.append('%s %s' % (fn, s))
        parts[0::2] = args
        return '%s(%s)' % (fn, ''.join(parts))
    lines = t.split('\n')
    for k, line in enumerate(lines):
        if re.match(r'^(?:static )?(?:inline )?[\w ]+?\*?\b\w+\([^)]*\)$', line):
            continue                         # a definition's own line
        lines[k] = re.sub(r'\b(\w+)\(([^()]*(?:\([^()]*\)[^()]*)*)\)', call, line)
    return '\n'.join(lines), left


def main(argv):
    if argv == ['header']:
        write_header()
        return 0
    if argv == ['--meter']:
        meter(sorted(glob.glob(os.path.join(G, '*', '*.c'))))
        return 0
    if argv and argv[0] == '--args':
        for f in argv[1:]:
            p = os.path.join(G, f)
            t = open(p).read()
            n, left = name_args(t, *table(f))
            for s in left:
                print('  left', f, s)
            if n != t:
                open(p, 'w').write(ensure_include(n))
                print('arguments named', f)
        return 0
    if argv and argv[0] == '--repick':
        for f in argv[1:]:
            p = os.path.join(G, f)
            t = open(p).read()
            n = repick(t, *table(f))
            if n != t:
                open(p, 'w').write(n)
                print('repicked', f)
        return 0
    if argv and argv[0] == '--indexed':
        for f in argv[1:]:
            p = os.path.join(G, f)
            t = open(p).read()
            n, left = name_indexed(t, *table(f), asm_spellings(f))
            for s in left:
                print('  left', f, s)
            if n != t:
                open(p, 'w').write(ensure_include(n))
                print('indexed named', f)
        return 0
    if argv and argv[0] == '--dp':
        for f in argv[1:]:
            p = os.path.join(G, f)
            t = open(p).read()
            n = name_dp(t, *table(f))
            if n != t:
                open(p, 'w').write(ensure_include(n))
                print('direct page named', f)
        return 0
    if argv and argv[0] == '--records':
        for f in argv[2:]:
            p = os.path.join(G, f)
            t = open(p).read()
            n = name_records(t, argv[1])
            if n != t:
                open(p, 'w').write(ensure_include(n))
                print('records named', f)
        return 0
    if argv and argv[0] == '--offsets':
        for f in argv[1:]:
            p = os.path.join(G, f)
            t = open(p).read()
            n = name_offsets(t, *table(f))
            if n != t:
                open(p, 'w').write(ensure_include(n))
                print('offsets named', f)
        return 0
    if argv and argv[0] == '--fields':
        for f in argv[1:]:
            p = os.path.join(G, f)
            t = open(p).read()
            n = name_fields(t, *table(f))
            if n != t:
                open(p, 'w').write(ensure_include(n))
                print('fields named', f)
        return 0
    if argv and argv[0] == '--macros':
        for f in argv[1:]:
            p = os.path.join(G, f)
            t = open(p).read()
            n = name_macros(t, *table(f))
            if n != t:
                open(p, 'w').write(ensure_include(n))
                print('macros named', f)
        return 0
    for f in argv:
        p = os.path.join(G, f)
        t = open(p).read()
        n = name_text(t, *table(f))
        if n != t:
            open(p, 'w').write(ensure_include(n))
            print('named', f)
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
