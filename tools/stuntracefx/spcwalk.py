#!/usr/bin/env python3
"""spcwalk.py: the SPC700 time the C stands for, read from the original's code.

    spcwalk.py [--only NAME|ADDR,...] [-v] [FILE.c ...]
                              walk the SPC700 routines of the C (default: the
                              race and battle drivers); -v lists every
                              statement as file, line, column, cycles, kind
    spcwalk.py --write DIR    also write each file with the statements put in
                              (DIR/<file name>); a file that already has them
                              comes out the same
    spcwalk.py --table        every opcode's bus cycles, from the pinned core
    spcwalk.py --table --check    the table against the documented times

The statement is spc_cyc(n), n in SPC700 bus cycles, and the checks add what
is stated since the last access: before each data access the C makes (spc_rd,
spc_wr) it holds the original's cycles from the cycle after the previous
access up to and including this access's own cycle, and a routine's end holds
the cycles after its last access through its return. Each instruction states
its own cycles, so a statement does not depend on the path that reaches it:
 - its first access gets the cycles from its opcode fetch through that access
   (the operand fetches, the dummy read of the byte after the opcode and the
   idle cycles included), each later access the cycles since the one before;
 - the cycles after its last access (or all of them, with no access) follow
   it, as a statement at the end of its C; a branch states its own when taken
   in the goto's braces, `if (c) { spc_cyc(4); goto L; }`, and when not taken
   after the if;
 - a CALL, PCALL or TCALL states its whole instruction before spc_call, a JMP
   to another routine (abi_X(); return;, spc_restart, spc_boot_rom) its
   cycles before the call, and the callee's first interval starts at its own
   opcode fetch;
 - a RET is spc_ret(first, second), its two pulls (spc.h): it ends on the
   second, so there is nothing after.
A statement is written as a line of its own before the statement that holds
the access, or as (spc_cyc(n), access) where the access is not the first in
its statement.

The cycles of an opcode come from the core the checks run (bsnes,
processor/spc700: instruction.cpp names the function of each opcode,
instructions.cpp lists its bus cycles in order): fetch() an operand byte,
idle() an idle cycle, the read(PC) of the byte after an implied instruction
(the dummy read: not a data access), read/load/pull a data read,
write/store/push a data write. The core's own marks say which reads are a
store's read of its target (a data access the C makes: spc_st) and which are
the dummy read(PC). A taken branch adds two idle cycles. --table --check
compares all the times with the SPC700's documented instruction times.

The C is the original's instructions in order, each after a comment with its
address (/* 1156 ret */). Each routine is read from its first instruction: the
instruction's bytes come from the ROM (the block the routine is uploaded in,
as hybrid.py maps it) and must read as the comment says; the C's accesses in
the instruction must be the kinds the opcode makes, in its order; a branch's
goto (or the call that leaves the routine) must be there for its target. A
helper that makes several accesses with no place between them for a statement
(spc_st, spc_rmw, spc_bit_set, spc_tset, spc_incw), and an spc_wr whose value
reads, are first written out as their accesses with the bodies spc_ops.h
gives them (a store is spc_rd(a); spc_wr(a, v)); the C then is the same.
Writing leaves the text it does not touch as it was, and a file with the
statements already in it is read without them first.
"""
import os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import spc700, spcmap  # noqa: E402

CORE = os.path.join(ROOT, 'tools', 'toolchains', 'src', 'bsnes', 'bsnes', 'processor', 'spc700')
AUDIO = os.path.join(ROOT, 'src', 'stuntracefx', 'audio')
FILES = [os.path.join(AUDIO, 'spc_race_driver.c'), os.path.join(AUDIO, 'spc_battle_driver.c')]
ROMPATH = os.path.join(ROOT, 'build', 'stuntracefx', 'baserom.sfc')

# ---- the opcodes' bus cycles, from the core ----

# F opcode fetch, O operand fetch, I idle, D dummy read of PC (not a data
# access), R data read, S a store's read of its target (a data read the C
# makes), W data write.
EVENT = re.compile(r'(?P<D>sfxbox_smp_dummy = 1; read\(PC\))|(?P<S>sfxbox_smp_store = 1; (?:read|load)\()'
                   r'|(?P<O>\bfetch\(\))|(?P<I>\bidle\(\))|(?P<R>\b(?:read|load|pull)\()|(?P<W>\b(?:write|store|push)\()')


class Op:
    """base: the cycles of the instruction not taking its branch; taken: the
    cycles a taken branch adds. bad: a loop with no fixed length."""
    def __init__(self):
        self.base, self.taken, self.bad = ['F'], [], False

    def data(self):
        """[(cycle number, 'R' or 'W')] of the data accesses (1 = the opcode fetch)."""
        return [(i + 1, 'W' if t == 'W' else 'R') for i, t in enumerate(self.base) if t in 'RSW']


_TABLE = None


def table():
    global _TABLE
    if _TABLE:
        return _TABLE
    names, args = {}, {}
    for line in open(os.path.join(CORE, 'instruction.cpp')):
        m = re.match(r'\s*op\(0x([0-9a-f]{2}), (\w+)(?:, (.*))?\)\s*$', line)
        if m:
            names[int(m.group(1), 16)] = m.group(2)
            args[int(m.group(1), 16)] = m.group(3) or ''
    src = open(os.path.join(CORE, 'instructions.cpp')).read()
    bodies = {m.group(1): m.group(2) for m in re.finditer(r'auto SPC700::instruction(\w+)\([^)]*\) -> void \{\n(.*?)\n\}\n', src, re.S)}
    out = {}
    for op in range(256):
        o = Op()
        body = bodies[names[op]]
        if names[op] in ('Wait', 'Stop'):
            o.bad = True
        tgt = o.base
        case = None
        mode = int(args[op].split(',')[0]) if names[op] == 'AbsoluteBitModify' else None
        for line in body.split('\n'):
            line = line.strip()
            if mode is not None:
                # a switch on the operation: only the case of this opcode's mode
                m = re.match(r'case (\d+):', line)
                if m:
                    case = int(m.group(1))
                    continue
                if case is not None and case != mode:
                    continue
            if re.match(r'if\(&flag == &IF\)', line):
                if args[op].startswith('IF'):
                    tgt.append('I')
                continue
            for m in EVENT.finditer(line):
                tgt.append(m.lastgroup)
            if re.match(r'if\(.*\) return;', line):
                tgt = o.taken
        out[op] = o
    _TABLE = out
    return out


# The SPC700's documented instruction times (not taken / taken), by opcode
# mask: a check of the table read from the core.
def _documented():
    d = {}
    def put(ops, n, tk=0):
        for op in ops:
            d[op] = (n, n + tk)
    put([0x00, 0xE8 - 0xE8 + 0x00], 2)
    put([0xE8, 0xCD, 0x8D, 0xC8, 0xAD], 2)                       # mov/cmp #imm
    put([0x04 + 0x20 * k for k in range(6)], 3)   # alu a,dp
    put([0x08 + 0x20 * k for k in range(6)], 2)   # alu a,#i
    for k in range(6):
        put([0x05 + 0x20 * k], 4)
        put([0x06 + 0x20 * k], 3)
        put([0x14 + 0x20 * k], 4)
        put([0x15 + 0x20 * k, 0x16 + 0x20 * k], 5)
        put([0x07 + 0x20 * k, 0x17 + 0x20 * k], 6)
        put([0x09 + 0x20 * k], 6 if k == 3 else 6)
        put([0x18 + 0x20 * k], 5 if k != 3 else 5)
        put([0x19 + 0x20 * k], 5)
    put([0x69], 6); put([0x78], 5)
    put([0x10, 0x30, 0x50, 0x70, 0x90, 0xB0, 0xD0, 0xF0], 2, 2)
    put([0x2F], 4, 0)
    put([0x03 + 0x20 * k for k in range(8)] + [0x13 + 0x20 * k for k in range(8)], 5, 2)
    put([0x2E, 0x6E], 5, 2); put([0xDE], 6, 2); put([0xFE], 4, 2)
    put([k << 4 | 1 for k in range(16)], 8)
    put([0x3F], 8); put([0x4F], 6); put([0x6F], 5); put([0x7F], 6); put([0x0F], 8)
    put([0x5F], 3); put([0x1F], 6)
    put([0x0D, 0x2D, 0x4D, 0x6D], 4); put([0x8E, 0xAE, 0xCE, 0xEE], 4)
    put([0xC4, 0xD8, 0xCB], 4); put([0xC5, 0xC9, 0xCC], 5); put([0xD4, 0xD9, 0xDB], 5)
    put([0xD5, 0xD6], 6); put([0xC7, 0xD7], 7); put([0xC6], 4); put([0xAF], 4)
    put([0xE4, 0xF8, 0xEB], 3); put([0xE5, 0xE9, 0xEC], 4); put([0xF4, 0xF9, 0xFB], 4)
    put([0xF5, 0xF6], 5); put([0xE7, 0xF7], 6); put([0xE6], 3); put([0xBF], 4)
    put([0xFA], 5); put([0x8F], 5); put([0xBA, 0xDA], 5); put([0x7A, 0x9A], 5); put([0x5A], 4)
    put([0x1A, 0x3A], 6)
    put([0x0B, 0x2B, 0x4B, 0x6B, 0x8B, 0xAB], 4); put([0x1B, 0x3B, 0x5B, 0x7B, 0x9B, 0xBB], 5)
    put([0x0C, 0x2C, 0x4C, 0x6C, 0x8C, 0xAC], 5)
    put([0x1C, 0x3C, 0x5C, 0x7C, 0x9C, 0xBC, 0x1D, 0x3D, 0xDC, 0xFC], 2)
    put([0x5D, 0x7D, 0x9D, 0xBD, 0xDD, 0xFD], 2)
    put([0x1E, 0x5E], 4); put([0x3E, 0x7E], 3)
    put([0x20, 0x40, 0x60, 0x80, 0xE0], 2); put([0xED], 3); put([0xA0, 0xC0], 3)
    put([0x02 + 0x20 * k for k in range(8)] + [0x12 + 0x20 * k for k in range(8)], 4)
    put([0x0E, 0x4E], 6); put([0x0A, 0x2A, 0x8A], 5); put([0x4A, 0x6A, 0xAA], 4); put([0xCA], 6); put([0xEA], 5)
    put([0x9E], 12); put([0xCF], 9); put([0x9F], 5); put([0xDF, 0xBE], 3)
    return d


def check_table():
    t, doc, bad = table(), _documented(), 0
    for op in range(256):
        if op not in doc:
            continue
        n, tk = len(t[op].base), len(t[op].base) + len(t[op].taken)
        if op == 0x2F:
            n = tk
        if (n, tk) != doc[op] and not (doc[op][0] == doc[op][1] and n == doc[op][0]):
            bad += 1
            print('%02X %-18s core %d/%d documented %d/%d' % (op, spc700.OPS[op][0], n, tk, doc[op][0], doc[op][1]))
    print('%d of %d opcodes checked against the documented times, %d differ' % (len(doc) - bad, len(doc), bad))
    return bad


def show_table():
    t = table()
    print('opcode instruction                cycles (F fetch, O operand, I idle, D dummy read of PC, R/S data read, W data write)')
    for op in range(256):
        o = t[op]
        print('%02X     %-24s %s%s%s' % (op, spc700.OPS[op][0], ' '.join(o.base),
                                         ('  taken: +' + ' '.join(o.taken)) if o.taken else '',
                                         '  (loops)' if o.bad else ''))
    dm = [op for op in range(256) if 'D' in t[op].base]
    st = [op for op in range(256) if 'S' in t[op].base]
    print('\nopcodes with a dummy read of PC (not data): %d' % len(dm))
    print('opcodes whose store reads the target first (data, the C makes it): %s' % ' '.join('%02X' % o for o in st))


# ---- the C ----

# Every call that makes data accesses, by the kinds in the order it makes them.
READS = {'spc_rd': 'R', 'spc_wr': 'W', 'spc_push': 'W', 'spc_pull': 'R', 'spc_ret': 'RR', 'spc_call': 'WW',
         'spc_st': 'RW', 'spc_rmw': 'RW', 'spc_bit_set': 'RW', 'spc_tset': 'RRW', 'spc_incw': 'RWRW',
         'spc_movw_st': 'RWW', 'spc_movw_ld': 'RR'}
# The helpers that make several accesses with no place between them for a
# statement take the cycles before each (spc_ops.h, spc.h) as arguments after
# their own, one amount per access; spc_call takes the three of a CALL.
# spc_wr_cyc is spc_wr with its amount, for a value that reads.
BASE = {'spc_st': 2, 'spc_rmw': 2, 'spc_bit_set': 3, 'spc_tset': 2, 'spc_incw': 2, 'spc_movw_st': 1, 'spc_movw_ld': 1,
        'spc_call': 2, 'spc_ret': 0, 'spc_wr': 2}
CYCFN = {'spc_st': 2, 'spc_rmw': 2, 'spc_bit_set': 2, 'spc_tset': 3, 'spc_incw': 4, 'spc_movw_st': 3,
         'spc_movw_ld': 2, 'spc_ret': 2, 'spc_call': 3, 'spc_wr': 1}
JUMPS = ('spc_restart', 'spc_boot_rom')
CALLNAME = re.compile(r'\b(spc_\w+|abi_\w+)\s*\(')
INSN = re.compile(r'^[ \t]*/\* ([0-9A-F]{4}) (.+?) \*/[ \t]*$', re.M)
LABEL = re.compile(r'^[ \t]*L[0-9A-F]{4}:[ \t]*$')
GOTO = re.compile(r'\bgoto L([0-9A-F]{4});')
BANNED = ('else', 'while', 'for', 'do', 'case', 'default', 'switch')


def strip(text):
    """The text without the statements this tool writes."""
    text = unsuffix(text)
    text = re.sub(r'\{ spc_cyc\(\d+\); (goto L[0-9A-F]{4};|abi_\w+\(\);|spc_restart\([^;]*\);|spc_boot_rom\(\);) \}', r'\1', text)
    while True:
        i = text.find('(spc_cyc(')
        if i < 0:
            break
        m = re.compile(r'\(spc_cyc\(\d+\), ').match(text, i)
        if not m:
            raise ValueError('a spc_cyc this tool did not write at offset %d' % i)
        j = match_paren(text, i)
        text = text[:i] + text[m.end():j] + text[j + 1:]
    text = re.sub(r'^[ \t]*spc_cyc\(\d+\);[ \t]*\n', '', text, flags=re.M)
    text = re.sub(r'spc_cyc\(\d+\); ?', '', text)
    return text


def unsuffix(text):
    """The amounts taken off the helpers that take them (spc_st(a, v, 4, 1)
    written back as spc_st(a, v)); spc_wr_cyc becomes spc_wr."""
    pat = re.compile(r'\b(spc_wr_cyc|%s)\(' % '|'.join(n for n in CYCFN if n != 'spc_wr'))
    pos = 0
    while True:
        m = pat.search(text, pos)
        if not m:
            return text
        o = m.end() - 1
        c = match_paren(text, o)
        depth, commas = 0, []
        for j in range(o + 1, c):
            ch = text[j]
            depth += ch in '([{'
            depth -= ch in ')]}'
            if ch == ',' and depth == 0:
                commas.append(j)
        name = 'spc_wr' if m.group(1) == 'spc_wr_cyc' else m.group(1)
        nargs = len(commas) + 1 if text[o + 1:c].strip() else 0
        keep = BASE[name]
        if m.group(1) == 'spc_wr_cyc' or nargs > keep:
            inner = text[o + 1:commas[keep - 1]] if keep > 0 else ''
            text = text[:m.start()] + name + '(' + inner + ')' + text[c + 1:]
        pos = m.start() + len(name) + 1


def mask(text):
    """Comments and string literals blanked, same offsets."""
    def blank(m):
        return re.sub(r'[^\n]', ' ', m.group(0))
    return re.sub(r'/\*.*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'', blank, text, flags=re.S)


def match_paren(text, i):
    """Offset of the ) that closes the ( at i."""
    depth = 0
    for j in range(i, len(text)):
        if text[j] == '(':
            depth += 1
        elif text[j] == ')':
            depth -= 1
            if depth == 0:
                return j
    raise ValueError('unclosed (')


def split_args(s):
    out, depth, cur = [], 0, ''
    for ch in s:
        if ch == ',' and depth == 0:
            out.append(cur.strip()); cur = ''
            continue
        depth += ch in '([{'
        depth -= ch in ')]}'
        cur += ch
    out.append(cur.strip())
    return out


class Call:
    def __init__(self, name, start, end, open_):
        self.name, self.start, self.end, self.open, self.inner = name, start, end, open_, []


def calls_in(code, base):
    """The access helpers, jumps and abi_ calls of the code, outermost first
    in text order, each with the ones in its arguments; offsets + base."""
    flat = []
    for m in CALLNAME.finditer(code):
        n = m.group(1)
        if n in READS or n in JUMPS or n.startswith('abi_'):
            o = m.end() - 1
            flat.append(Call(n, base + m.start(), base + match_paren(code, o) + 1, base + o))
    top, stack = [], []
    for c in flat:
        while stack and c.start >= stack[-1].end:
            stack.pop()
        (stack[-1].inner if stack else top).append(c)
        stack.append(c)
    return top


def order(cs):
    """Calls in the order they run: the ones in a call's arguments first."""
    out = []
    for c in cs:
        out += order(c.inner)
        out.append(c)
    return out


class Block:
    def __init__(self, addr, text, start, end, line):
        self.addr, self.text, self.start, self.end, self.line = addr, text, start, end, line
        self.op = self.ln = self.vals = None
        self.calls = []          # in run order
        self.err = None


class Routine:
    def __init__(self, name, rom_addr, apu, file, body_start, body, line):
        self.name, self.rom_addr, self.apu, self.file = name, rom_addr, apu, file
        self.body_start, self.body, self.line = body_start, body, line
        self.blocks = []


def routines(path, text):
    """The @spc routines of a C file."""
    out = []
    for m in re.finditer(r'/\*.*?\*/', text, re.S):
        c = m.group(0)
        t = re.search(r'@routine\s+([0-9A-Fa-f]{2}):([0-9A-Fa-f]{4})\s+(\w+)', c)
        s = re.search(r'@spc\s+([0-9A-Fa-f]{4})\b', c)
        if not (t and s):
            continue
        f = re.compile(r'\s*void abi_%s\(void\)\s*\n\{\n' % t.group(3)).match(text, m.end())
        if not f:
            continue
        e = text.index('\n}\n', f.end() - 1)
        out.append(Routine(t.group(3), int(t.group(1), 16) << 16 | int(t.group(2), 16), int(s.group(1), 16), path,
                           f.end(), text[f.end():e + 1], text.count('\n', 0, f.end()) + 1))
    return out


class Rom:
    def __init__(self):
        self.rom = open(ROMPATH, 'rb').read()
        self.lists = spcmap.lists(self.rom)
        self.maps = {}

    def apu_map(self, rom_addr):
        """APU address -> ROM offset: the race driver's list, over it the
        list that holds the routine's ROM byte (as hybrid.py maps)."""
        o = spcmap.rom_offset(rom_addr >> 16, rom_addr & 0xFFFF)
        own = next((bl for bl, _ in self.lists.values() if any(off <= o < off + n for _, n, off in bl)), [])
        key = id(own)
        if key not in self.maps:
            m = {}
            for bl in (self.lists['$19:C27C'][0], own):
                for d, n, off in bl:
                    for k in range(n):
                        m[(d + k) & 0xFFFF] = off + k
            self.maps[key] = m
        return self.maps[key]


def parse(r, rom, text):
    """Fill r.blocks: the instructions of the C, their opcodes read from the
    ROM (they must read as the comment says) and the C's calls in them."""
    m = rom.apu_map(r.rom_addr)
    ms = list(INSN.finditer(r.body))
    for i, mm in enumerate(ms):
        end = ms[i + 1].start() if i + 1 < len(ms) else len(r.body)
        a = int(mm.group(1), 16)
        b = Block(a, mm.group(2), r.body_start + mm.end(), r.body_start + end, text.count('\n', 0, r.body_start + mm.start()) + 1)
        r.blocks.append(b)
        if a not in m:
            b.err = 'address in no uploaded block'
            continue
        t, ln = spc700.decode(lambda x: rom.rom[m[x & 0xFFFF]], a)
        b.op, b.ln = rom.rom[m[a]], ln
        raw = bytes(rom.rom[m[(a + 1 + k) & 0xFFFF]] for k in range(ln - 1))
        b.vals = spc700.operands(b.op, raw, a)
        if t != b.text:
            b.err = 'the ROM has "%s" there' % t
            continue
        b.calls = order(calls_in(mask(text[b.start:b.end]), b.start))


def accesses(b):
    """What the C's calls in the block make, in order: (Call, k, kind)."""
    return [(c, k, kind) for c in b.calls if c.name in READS for k, kind in enumerate(READS[c.name])]


CONDS = {0x10, 0x30, 0x50, 0x70, 0x90, 0xB0, 0xD0, 0xF0, 0x2E, 0x6E, 0xDE, 0xFE} | {0x03 + 0x20 * k for k in range(8)} | {0x13 + 0x20 * k for k in range(8)}
CALLS = {0x3F, 0x4F} | {k << 4 | 1 for k in range(16)}


# ---- edits ----

class Edits:
    """Insertions and replacements in one text. (start, end, text, rank): rank
    orders insertions at one offset (a closing parenthesis first)."""
    def __init__(self):
        self.e, self.n = [], 0

    def add(self, start, end, text, rank=1):
        self.n += 1
        self.e.append((start, rank, self.n, end, text))

    def apply(self, text):
        out, last = [], 0
        for start, _, _, end, t in sorted(self.e):
            if start < last:
                raise ValueError('overlapping edits at %d' % start)
            out.append(text[last:start]); out.append(t)
            last = end
        out.append(text[last:])
        return ''.join(out)


def line_start(text, off):
    return text.rfind('\n', 0, off) + 1


def line_col(text, off):
    return text.count('\n', 0, off) + 1, off - line_start(text, off) + 1


def indent_of(text, off):
    ls = line_start(text, off)
    return re.match(r'[ \t]*', text[ls:]).group(0)


def at_line_start(code, off):
    return code[line_start(code, off):off].strip() == ''


def is_statement(code, c):
    """The call stands as a statement of its own: after ; { or }, before ;."""
    i = c.start - 1
    while i >= 0 and code[i] in ' \t\n':
        i -= 1
    j = c.end
    while j < len(code) and code[j] in ' \t\n':
        j += 1
    labelled = i >= 0 and code[i] == ':' and re.search(r'\bL[0-9A-F]{4}:$', code[max(0, i - 6):i + 1])
    return i >= 0 and (code[i] in ';{}' or labelled) and j < len(code) and code[j] == ';'


def unit_start(code, pos):
    """Where the statement holding pos begins."""
    i = pos - 1
    while i >= 0 and code[i] not in ';{}':
        i -= 1
    i += 1
    while True:
        while code[i] in ' \t\n':
            i += 1
        m = re.compile(r'L[0-9A-F]{4}:').match(code, i)
        if not m:
            return i
        i = m.end()


class Out:
    def __init__(self):
        self.msgs = []
        self.rows = {}           # file -> [(line, col, n, kind)]
        self.routines = 0
        self.bad = set()
        self.st = dict(instr=0, accesses=0, covered=0, stmts=0, access_stmts=0, tail_stmts=0, edge_stmts=0, call_stmts=0,
                       ret=0, blocks_bad=0, helper_args=0)
        self.cyc = []

    def problem(self, r, b, text):
        line = b.line if b else r.line
        self.msgs.append('%s:%d: %s %04X%s: %s' % (os.path.relpath(r.file, ROOT), line, r.name, r.apu,
                                                    (' ' + '%04X %s' % (b.addr, b.text)) if b else '', text))
        self.bad.add((r.file, r.name))


def annotate(r, text, code, rom, out, ed, rows):
    """The statements of one routine, as edits of text."""
    t = table()
    used = set()
    blocks = {b.addr for b in r.blocks}
    if r.blocks[0].addr != r.apu:
        out.problem(r, r.blocks[0], 'the first instruction is not the entry %04X' % r.apu)

    def row(off, n, kind):
        rows.append((off, n, kind))

    def stmt_at(pos, n, kind):
        """A statement of its own before the statement starting at pos."""
        if at_line_start(code, pos):
            ls = line_start(code, pos)
            ed.add(ls, ls, '%sspc_cyc(%d);\n' % (indent_of(code, pos), n))
            row(pos, n, kind)
        else:
            ed.add(pos, pos, 'spc_cyc(%d); ' % n)
            row(pos, n, kind)

    def place(b, call, n, kind):
        if is_statement(code, call):
            stmt_at(call.start, n, kind)
            return
        s = unit_start(code, call.start)
        ls = line_start(code, s)
        pre = code[ls:s].strip()
        word = re.match(r'\w*', code[s:]).group(0)
        ok = pre in ('', '{') and ls not in used and word not in BANNED
        if ok and word == 'if':
            o = code.index('(', s)
            ok = o < call.start < match_paren(code, o)
        if ok:
            used.add(ls)
            ed.add(ls, ls, '%sspc_cyc(%d);\n' % (indent_of(code, s), n))
            row(s, n, kind)
        else:
            ed.add(call.start, call.start, '(spc_cyc(%d), ' % n)
            ed.add(call.end, call.end, ')', 0)
            row(call.start, n, kind)

    def edge(b, node_start, node_end, k, kind):
        """k cycles on the way to a goto or a call that leaves the routine."""
        if k == 0:
            return
        i = node_start - 1
        while code[i] in ' \t\n':
            i -= 1
        if code[i] in ';{}:':
            stmt_at(node_start, k, kind)
        else:
            ed.add(node_start, node_start, '{ spc_cyc(%d); ' % k)
            ed.add(node_end, node_end, ' }', 0)
            row(node_start, k, kind)

    def after(b, k):
        """k cycles on falling through the end of the block; where the original
        falls into another routine's code the C calls it, and they go before."""
        if k == 0:
            return
        nxt = (b.addr + b.ln) & 0xFFFF
        ext = [c for c in b.calls if c.name.startswith('abi_') or c.name in JUMPS]
        if nxt not in blocks and ext:
            c = ext[0]
            j = c.end
            while code[j] in ' \t':
                j += 1
            edge(b, c.start, j + 1 if code[j] == ';' else c.end, k, 'edge')
            return
        lines = text[b.start:b.end].split('\n')
        # lines[0] is the rest of the comment line; the code follows
        end = len(lines)
        while end > 1 and (not lines[end - 1].strip() or LABEL.match(lines[end - 1])):
            end -= 1
        off = b.start + sum(len(x) + 1 for x in lines[:end])
        if end == 1:
            off = b.start + 1
        ed.add(off, off, '    spc_cyc(%d);\n' % k, 1)
        row(off, k, 'tail')

    for b in r.blocks:
        out.st['instr'] += 1
        if b.err:
            out.problem(r, b, b.err); out.st['blocks_bad'] += 1
            continue
        o = t[b.op]
        if o.bad or b.op in (0x0F,):
            out.problem(r, b, 'the instruction has no fixed length'); out.st['blocks_bad'] += 1
            continue
        data = o.data()
        T, X = len(o.base), len(o.taken)
        out.st['accesses'] += len(data)
        acc = accesses(b)
        gk = ''.join(k for _, _, k in acc)
        want = ''.join(k for _, k in data)
        if any(c.inner and any(i.name in READS for i in c.inner) and c.name not in CYCFN for c in b.calls if c.name in READS):
            out.problem(r, b, 'an access in the arguments of another that cannot take its time'); out.st['blocks_bad'] += 1
            continue
        if b.op in CALLS:
            cl = [c for c in b.calls if c.name == 'spc_call']
            if b.op not in (0x3F, 0x4F) or not cl or gk != 'WW':
                out.problem(r, b, 'a call the C does not make with spc_call (accesses %s)' % (gk or 'none')); out.st['blocks_bad'] += 1
                continue
            c = cl[0]
            n1, n2, n3 = data[0][0], data[1][0] - data[0][0], T - data[1][0]
            ed.add(c.end - 1, c.end - 1, ', %d, %d, %d' % (n1, n2, n3))
            for n in (n1, n2, n3):
                row(c.start, n, 'call')
            out.st['call_stmts'] += 1
            out.st['covered'] += len(data)
            continue
        if gk != want:
            out.problem(r, b, 'the original makes %s, the C %s' % (want or 'no access', gk or 'none')); out.st['blocks_bad'] += 1
            continue
        out.st['covered'] += len(data)
        # the accesses, call by call
        prev, k = 0, 0
        for c in [c for c in b.calls if c.name in READS]:
            m = len(READS[c.name])
            amounts = []
            for j in range(m):
                amounts.append(data[k + j][0] - prev)
                prev = data[k + j][0]
            k += m
            nested = c.inner and any(i.name in READS for i in c.inner)
            if c.name == 'spc_ret':
                ed.add(c.start, c.end, 'spc_ret(%d, %d)' % tuple(amounts))
                out.st['ret'] += 1
            elif m > 1 or (c.name == 'spc_wr' and nested):
                if c.name == 'spc_wr':
                    ed.add(c.start, c.start + len(c.name), 'spc_wr_cyc')
                ed.add(c.end - 1, c.end - 1, ''.join(', %d' % n for n in amounts))
                out.st['helper_args'] += len(amounts)
            else:
                place(b, c, amounts[0], 'access')
                out.st['access_stmts'] += 1
                continue
            for n in amounts:
                row(c.start, n, 'arg')
        tail = T - (data[-1][0] if data else 0)
        # where the instruction goes on
        gotos = [(m.start() + b.start, m.end() + b.start, int(m.group(1), 16)) for m in GOTO.finditer(code[b.start:b.end])]
        ext = [c for c in b.calls if c.name.startswith('abi_') or c.name in JUMPS]

        def jump_to(target, k):
            g = [x for x in gotos if x[2] == target]
            if g:
                edge(b, g[0][0], g[0][1], k, 'edge')
            elif ext and target not in blocks:
                c = ext[0]
                j = c.end
                while code[j] in ' \t':
                    j += 1
                edge(b, c.start, j + 1 if code[j] == ';' else c.end, k, 'edge')
            else:
                out.problem(r, b, 'no goto L%04X and no call that leaves the routine for the jump' % target)
                return
            out.st['edge_stmts'] += k > 0
        if b.op in CONDS:
            jump_to(b.vals['r'], tail + X)
            after(b, tail); out.st['tail_stmts'] += tail > 0
        elif b.op == 0x2F:
            jump_to(b.vals['r'], tail + X)
        elif b.op == 0x5F:
            jump_to(b.vals['a'], tail)
        elif b.op in (0x6F, 0x7F, 0x1F):
            if tail:
                out.problem(r, b, 'cycles after the last access of a return or table jump (%d)' % tail)
        elif b.op in (0xEF, 0xFF):
            out.problem(r, b, 'stop/sleep')
        else:
            after(b, tail); out.st['tail_stmts'] += tail > 0


def main(argv):
    args = argv[1:]
    if '--table' in args:
        if '--check' in args:
            return 1 if check_table() else 0
        show_table()
        return 0
    write = only = None
    verbose = False
    files = []
    i = 0
    while i < len(args):
        a = args[i]
        if a == '--write':
            write = args[i + 1]; i += 1
        elif a == '--only':
            only = set(args[i + 1].split(',')); i += 1
        elif a == '-v':
            verbose = True
        else:
            files.append(a)
        i += 1
    files = files or FILES
    rom = Rom()
    out = Out()
    want = lambda r: not only or r.name in only or '%04X' % r.apu in only or '%06X' % r.rom_addr in only   # noqa: E731
    for f in files:
        text = strip(open(f).read())
        code = mask(text)
        ed, rows = Edits(), []
        for r in routines(f, text):
            if not want(r):
                continue
            out.routines += 1
            parse(r, rom, text)
            annotate(r, text, code, rom, out, ed, rows)
        res = ed.apply(text)
        res_rows = sorted(rows)
        if verbose:
            for off, n, kind in res_rows:
                l, c = line_col(text, off)
                print('%s %d %d %d %s' % (os.path.relpath(f, ROOT), l, c, n, kind))
        out.rows[f] = res_rows
        if write:
            os.makedirs(write, exist_ok=True)
            open(os.path.join(write, os.path.basename(f)), 'w').write(res)
    for m in out.msgs:
        print('PROBLEM  ', m)
    st = out.st
    nstmt = sum(len(v) for v in out.rows.values())
    print('\n%d routines, %d without a problem' % (out.routines, out.routines - len(out.bad)))
    print('%d instructions, %d data accesses of the original, %d covered by a statement (%d instructions not decided)' % (
        st['instr'], st['accesses'], st['covered'], st['blocks_bad']))
    print('%d amounts written: %d statements before accesses, %d as arguments of helpers (%d CALLs, %d RETs), '
          '%d statements after the last access, %d on jump edges'
          % (nstmt, st['access_stmts'], st['helper_args'], st['call_stmts'], st['ret'], st['tail_stmts'], st['edge_stmts']))
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
