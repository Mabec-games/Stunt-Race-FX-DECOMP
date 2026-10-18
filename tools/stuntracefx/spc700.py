#!/usr/bin/env python3
"""spc700.py: the SPC700 instruction set, as asar's arch spc700 writes it.

    spc700.py --check     assemble one instruction of every opcode with the
                          pinned asar and compare the bytes with the table

OPS[opcode] = (template, length). Template fields: {d} a direct page byte,
{d2} the second one (the destination of a two-address operation), {a} an
absolute word, {i} an immediate byte, {r} a relative branch target, {m} a
13-bit address with its bit number (addr.bit), {u} the PCALL page byte.
Instructions that take a direct page byte or an absolute word through the
same mnemonic carry asar's .b/.w width so the bytes come out the same.
decode(mem, addr) gives (text, length) for the instruction at addr of an
SPC700 address space given as a function or a bytes-like 64K image.
"""
import os, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

_ALU = ['or', 'and', 'eor', 'cmp', 'adc', 'sbc']
OPS = {}


def _alu(base, name):
    OPS[base + 0x04] = (name + '.b a,{d}', 2)
    OPS[base + 0x05] = (name + '.w a,{a}', 3)
    OPS[base + 0x06] = (name + ' a,(x)', 1)
    OPS[base + 0x07] = (name + ' a,({d}+x)', 2)
    OPS[base + 0x08] = (name + ' a,#{i}', 2)
    OPS[base + 0x09] = (name + ' {d2},{d}', 3)
    OPS[base + 0x14] = (name + '.b a,{d}+x', 2)
    OPS[base + 0x15] = (name + '.w a,{a}+x', 3)
    OPS[base + 0x16] = (name + '.w a,{a}+y', 3)
    OPS[base + 0x17] = (name + ' a,({d})+y', 2)
    OPS[base + 0x18] = (name + ' {d2},#{i}', 3)
    OPS[base + 0x19] = (name + ' (x),(y)', 1)


for n, base in zip(_ALU, (0x00, 0x20, 0x40, 0x60, 0x80, 0xA0)):
    _alu(base, n)
for k in range(16):
    OPS[k << 4 | 0x01] = ('tcall %d' % k, 1)
for b in range(8):
    OPS[b << 5 | 0x02] = ('set1 {d}.%d' % b, 2)
    OPS[b << 5 | 0x12] = ('clr1 {d}.%d' % b, 2)
    OPS[b << 5 | 0x03] = ('bbs {d}.%d,{r}' % b, 3)
    OPS[b << 5 | 0x13] = ('bbc {d}.%d,{r}' % b, 3)
for op, name in ((0x10, 'bpl'), (0x30, 'bmi'), (0x50, 'bvc'), (0x70, 'bvs'), (0x90, 'bcc'), (0xB0, 'bcs'),
                 (0xD0, 'bne'), (0xF0, 'beq'), (0x2F, 'bra')):
    OPS[op] = (name + ' {r}', 2)
OPS.update({
    0x00: ('nop', 1), 0x0A: ('or1 c,{m}', 3), 0x0B: ('asl.b {d}', 2), 0x0C: ('asl.w {a}', 3),
    0x0D: ('push p', 1), 0x0E: ('tset {a},a', 3), 0x0F: ('brk', 1),
    0x1A: ('decw {d}', 2), 0x1B: ('asl.b {d}+x', 2), 0x1C: ('asl a', 1), 0x1D: ('dec x', 1),
    0x1E: ('cmp.w x,{a}', 3), 0x1F: ('jmp ({a}+x)', 3),
    0x20: ('clrp', 1), 0x2A: ('or1 c,!{m}', 3), 0x2B: ('rol.b {d}', 2), 0x2C: ('rol.w {a}', 3),
    0x2D: ('push a', 1), 0x2E: ('cbne {d},{r}', 3),
    0x3A: ('incw {d}', 2), 0x3B: ('rol.b {d}+x', 2), 0x3C: ('rol a', 1), 0x3D: ('inc x', 1),
    0x3E: ('cmp.b x,{d}', 2), 0x3F: ('call {a}', 3),
    0x40: ('setp', 1), 0x4A: ('and1 c,{m}', 3), 0x4B: ('lsr.b {d}', 2), 0x4C: ('lsr.w {a}', 3),
    0x4D: ('push x', 1), 0x4E: ('tclr {a},a', 3), 0x4F: ('pcall {u}', 2),
    0x5A: ('cmpw ya,{d}', 2), 0x5B: ('lsr.b {d}+x', 2), 0x5C: ('lsr a', 1), 0x5D: ('mov x,a', 1),
    0x5E: ('cmp.w y,{a}', 3), 0x5F: ('jmp {a}', 3),
    0x60: ('clrc', 1), 0x6A: ('and1 c,!{m}', 3), 0x6B: ('ror.b {d}', 2), 0x6C: ('ror.w {a}', 3),
    0x6D: ('push y', 1), 0x6E: ('dbnz {d},{r}', 3), 0x6F: ('ret', 1),
    0x7A: ('addw ya,{d}', 2), 0x7B: ('ror.b {d}+x', 2), 0x7C: ('ror a', 1), 0x7D: ('mov a,x', 1),
    0x7E: ('cmp.b y,{d}', 2), 0x7F: ('reti', 1),
    0x80: ('setc', 1), 0x8A: ('eor1 c,{m}', 3), 0x8B: ('dec.b {d}', 2), 0x8C: ('dec.w {a}', 3),
    0x8D: ('mov y,#{i}', 2), 0x8E: ('pop p', 1), 0x8F: ('mov {d2},#{i}', 3),
    0x9A: ('subw ya,{d}', 2), 0x9B: ('dec.b {d}+x', 2), 0x9C: ('dec a', 1), 0x9D: ('mov x,sp', 1),
    0x9E: ('div ya,x', 1), 0x9F: ('xcn a', 1),
    0xA0: ('ei', 1), 0xAA: ('mov1 c,{m}', 3), 0xAB: ('inc.b {d}', 2), 0xAC: ('inc.w {a}', 3),
    0xAD: ('cmp y,#{i}', 2), 0xAE: ('pop a', 1), 0xAF: ('mov (x+),a', 1),
    0xBA: ('movw ya,{d}', 2), 0xBB: ('inc.b {d}+x', 2), 0xBC: ('inc a', 1), 0xBD: ('mov sp,x', 1),
    0xBE: ('das a', 1), 0xBF: ('mov a,(x+)', 1),
    0xC0: ('di', 1), 0xC4: ('mov.b {d},a', 2), 0xC5: ('mov.w {a},a', 3), 0xC6: ('mov (x),a', 1),
    0xC7: ('mov ({d}+x),a', 2), 0xC8: ('cmp x,#{i}', 2), 0xC9: ('mov.w {a},x', 3), 0xCA: ('mov1 {m},c', 3),
    0xCB: ('mov.b {d},y', 2), 0xCC: ('mov.w {a},y', 3), 0xCD: ('mov x,#{i}', 2), 0xCE: ('pop x', 1),
    0xCF: ('mul ya', 1),
    0xD4: ('mov.b {d}+x,a', 2), 0xD5: ('mov.w {a}+x,a', 3), 0xD6: ('mov.w {a}+y,a', 3),
    0xD7: ('mov ({d})+y,a', 2), 0xD8: ('mov.b {d},x', 2), 0xD9: ('mov.b {d}+y,x', 2), 0xDA: ('movw {d},ya', 2),
    0xDB: ('mov.b {d}+x,y', 2), 0xDC: ('dec y', 1), 0xDD: ('mov a,y', 1), 0xDE: ('cbne {d}+x,{r}', 3),
    0xDF: ('daa a', 1),
    0xE0: ('clrv', 1), 0xE4: ('mov.b a,{d}', 2), 0xE5: ('mov.w a,{a}', 3), 0xE6: ('mov a,(x)', 1),
    0xE7: ('mov a,({d}+x)', 2), 0xE8: ('mov a,#{i}', 2), 0xE9: ('mov.w x,{a}', 3), 0xEA: ('not1 {m}', 3),
    0xEB: ('mov.b y,{d}', 2), 0xEC: ('mov.w y,{a}', 3), 0xED: ('notc', 1), 0xEE: ('pop y', 1),
    0xEF: ('sleep', 1),
    0xF4: ('mov.b a,{d}+x', 2), 0xF5: ('mov.w a,{a}+x', 3), 0xF6: ('mov.w a,{a}+y', 3),
    0xF7: ('mov a,({d})+y', 2), 0xF8: ('mov.b x,{d}', 2), 0xF9: ('mov.b x,{d}+y', 2), 0xFA: ('mov {d2},{d}', 3),
    0xFB: ('mov.b y,{d}+x', 2), 0xFC: ('inc y', 1), 0xFD: ('mov y,a', 1), 0xFE: ('dbnz y,{r}', 2),
    0xFF: ('stop', 1),
})
assert len(OPS) == 256, len(OPS)

# The operand bytes in order, by template field: two-address forms put the
# source first; {d2},#{i} puts the immediate first.
def _fields(t):
    if '{d2},{d}' in t:
        return ['d', 'd2']
    if '{d2},#{i}' in t:
        return ['i', 'd2']
    out = []
    for f in ('{d}', '{a}', '{i}', '{m}', '{u}', '{r}'):
        if f in t:
            out.append((t.index(f), f[1:-1]))
    return [f for _, f in sorted(out)]


def length(op):
    return OPS[op][1]


def operands(op, raw, addr):
    """Field values of the instruction with operand bytes raw at addr."""
    t, n = OPS[op]
    vals, k = {}, 0
    for f in _fields(t):
        if f in ('d', 'd2', 'i', 'u'):
            vals[f] = raw[k]; k += 1
        elif f in ('a', 'm'):
            vals[f] = raw[k] | raw[k + 1] << 8; k += 2
        elif f == 'r':
            vals[f] = (addr + n + (raw[k] - 256 if raw[k] >= 128 else raw[k])) & 0xFFFF; k += 1
    return vals


def text(op, vals, names=None):
    """The instruction as asar writes it; names(kind, value) can give a name."""
    t = OPS[op][0]
    def fmt(f, v):
        nm = names(f, v) if names else None
        if nm:
            return nm
        if f in ('d', 'd2', 'i', 'u'):
            return '$%02X' % v
        if f == 'm':
            return '$%04X.%d' % (v & 0x1FFF, v >> 13)
        return '$%04X' % v
    for f, v in vals.items():
        t = t.replace('{%s}' % f, fmt(f, v))
    return t


def decode(mem, addr, names=None):
    rd = mem if callable(mem) else (lambda a: mem[a & 0xFFFF])
    op = rd(addr)
    n = OPS[op][1]
    raw = [rd((addr + 1 + k) & 0xFFFF) for k in range(n - 1)]
    return text(op, operands(op, raw, addr), names), n


def jump_table(word, base, lo, hi):
    """The handlers of a JMP (!a+X) table at base: its words while each is in
    the code lo..hi-1 and after the table, and the table has not reached the
    lowest of them. The drivers' tables end so: the port command tables (61
    entries, then the first handler), the voice command table (32, then a
    word outside the code) and the sound-special tables, where the next
    word is the byte table the index came from."""
    out, k, low = [], 0, hi
    while base + 2 * k + 1 < low:
        t = word(base + 2 * k)
        if not (lo <= t < hi and t > base):
            break
        out.append(t)
        low = min(low, t)
        k += 1
    return out


def check():
    """Every opcode with sample operands through asar, against the table."""
    import tempfile
    asar = os.path.join(ROOT, 'tools', 'toolchains', 'asar', 'asar')
    lines, want, base = ['arch spc700', 'norom', 'org $0000', 'base $1000'], b'', 0x1000
    for op in range(256):
        n = OPS[op][1]
        raw = bytes([0x34, 0x12][:n - 1]) if '{r}' not in OPS[op][0] else bytes([0x56, 0x02][:n - 1])
        if OPS[op][0].endswith(',{r}') and n == 3:
            raw = bytes([0x34, 0x02])
        if '{m}' in OPS[op][0]:
            raw = bytes([0x34, 0xB2])
        t, _ = decode(lambda a, o=op, r=raw, b=base: o if a == b else r[(a - b - 1) & 0xFFFF], base)
        lines.append(t)
        want += bytes([op]) + raw
        base += n
    d = tempfile.mkdtemp()
    src, img = os.path.join(d, 't.asm'), os.path.join(d, 't.bin')
    open(src, 'w').write('\n'.join(lines) + '\n')
    open(img, 'wb').write(bytes(len(want)))
    p = subprocess.run([asar, '--no-title-check', '--fix-checksum=off', src, img], capture_output=True, text=True)
    if p.returncode:
        print(p.stdout + p.stderr)
        return 1
    got = open(img, 'rb').read()[:len(want)]
    bad, o = 0, 0
    for op in range(256):
        n = OPS[op][1]
        if got[o:o + n] != want[o:o + n]:
            bad += 1
            print('%02X %-24s want %s got %s' % (op, lines[4 + op], want[o:o + n].hex(), got[o:o + n].hex()))
        o += n
    print('%d of 256 opcodes assemble to their own bytes' % (256 - bad))
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(check() if '--check' in sys.argv else 0)
