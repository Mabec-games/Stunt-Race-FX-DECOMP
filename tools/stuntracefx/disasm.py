#!/usr/bin/env python3
"""disasm.py: emit the raw (T1) assembly for the whole ROM from sfxbox traces.

    disasm.py --rom ROM --trace DIR [--trace DIR...] --out asm/stuntracefx/raw

Only bytes the traces saw executed become instructions; everything else is
data, included from the reference ROM at build time (--incbin) so the tree
never carries the original's data bytes. Every instruction carries an explicit operand size so the assembler
never chooses one. Branch targets that are traced instruction starts in the
same bank get labels; any other displacement is written raw.

SPC700 code the traces saw run in APU RAM (folded back onto the uploaded ROM
bytes, spcmap.py) is written as arch spc700 at its APU RAM address.

Output: one file per 32 KB LoROM bank (raw/bankNN.asm) plus raw/main.asm,
which assembles to the ROM with asar in norom mode. This is intake: address
names only, nothing here is filed or described.
"""
import argparse, csv, os, struct, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import codemap, spc700, spcmap  # noqa: E402

CPU_OP, CPU_OPND, GSU_OP, GSU_OPND = 1, 2, 16, 32
SPC_OP = spcmap.SPC_OP
M8, M16, X8, X16 = 1 << 8, 1 << 9, 1 << 10, 1 << 11
BANK = 0x8000

# 65816 opcode table: (mnemonic, mode). Modes and operand bytes:
#   imp 0, acc 0, imm 1 (fixed 8-bit), immm 1|2 (M), immx 1|2 (X),
#   dp, dpx, dpy, dpi, dpxi, dpiy, dpil, dpily, sr, sriy 1,
#   abs, absx, absy, absi, absxi, absil, blk 2, rel 1, rell 2,
#   long, longx 3
_T = """
00 brk imm   01 ora dpxi  02 cop imm   03 ora sr    04 tsb dp    05 ora dp    06 asl dp    07 ora dpil
08 php imp   09 ora immm  0A asl acc   0B phd imp   0C tsb abs   0D ora abs   0E asl abs   0F ora long
10 bpl rel   11 ora dpiy  12 ora dpi   13 ora sriy  14 trb dp    15 ora dpx   16 asl dpx   17 ora dpily
18 clc imp   19 ora absy  1A inc acc   1B tcs imp   1C trb abs   1D ora absx  1E asl absx  1F ora longx
20 jsr abs   21 and dpxi  22 jsl long  23 and sr    24 bit dp    25 and dp    26 rol dp    27 and dpil
28 plp imp   29 and immm  2A rol acc   2B pld imp   2C bit abs   2D and abs   2E rol abs   2F and long
30 bmi rel   31 and dpiy  32 and dpi   33 and sriy  34 bit dpx   35 and dpx   36 rol dpx   37 and dpily
38 sec imp   39 and absy  3A dec acc   3B tsc imp   3C bit absx  3D and absx  3E rol absx  3F and longx
40 rti imp   41 eor dpxi  42 wdm imm   43 eor sr    44 mvp blk   45 eor dp    46 lsr dp    47 eor dpil
48 pha imp   49 eor immm  4A lsr acc   4B phk imp   4C jmp abs   4D eor abs   4E lsr abs   4F eor long
50 bvc rel   51 eor dpiy  52 eor dpi   53 eor sriy  54 mvn blk   55 eor dpx   56 lsr dpx   57 eor dpily
58 cli imp   59 eor absy  5A phy imp   5B tcd imp   5C jml long  5D eor absx  5E lsr absx  5F eor longx
60 rts imp   61 adc dpxi  62 per rell  63 adc sr    64 stz dp    65 adc dp    66 ror dp    67 adc dpil
68 pla imp   69 adc immm  6A ror acc   6B rtl imp   6C jmp absi  6D adc abs   6E ror abs   6F adc long
70 bvs rel   71 adc dpiy  72 adc dpi   73 adc sriy  74 stz dpx   75 adc dpx   76 ror dpx   77 adc dpily
78 sei imp   79 adc absy  7A ply imp   7B tdc imp   7C jmp absxi 7D adc absx  7E ror absx  7F adc longx
80 bra rel   81 sta dpxi  82 brl rell  83 sta sr    84 sty dp    85 sta dp    86 stx dp    87 sta dpil
88 dey imp   89 bit immm  8A txa imp   8B phb imp   8C sty abs   8D sta abs   8E stx abs   8F sta long
90 bcc rel   91 sta dpiy  92 sta dpi   93 sta sriy  94 sty dpx   95 sta dpx   96 stx dpy   97 sta dpily
98 tya imp   99 sta absy  9A txs imp   9B txy imp   9C stz abs   9D sta absx  9E stz absx  9F sta longx
A0 ldy immx  A1 lda dpxi  A2 ldx immx  A3 lda sr    A4 ldy dp    A5 lda dp    A6 ldx dp    A7 lda dpil
A8 tay imp   A9 lda immm  AA tax imp   AB plb imp   AC ldy abs   AD lda abs   AE ldx abs   AF lda long
B0 bcs rel   B1 lda dpiy  B2 lda dpi   B3 lda sriy  B4 ldy dpx   B5 lda dpx   B6 ldx dpy   B7 lda dpily
B8 clv imp   B9 lda absy  BA tsx imp   BB tyx imp   BC ldy absx  BD lda absx  BE ldx absy  BF lda longx
C0 cpy immx  C1 cmp dpxi  C2 rep imm   C3 cmp sr    C4 cpy dp    C5 cmp dp    C6 dec dp    C7 cmp dpil
C8 iny imp   C9 cmp immm  CA dex imp   CB wai imp   CC cpy abs   CD cmp abs   CE dec abs   CF cmp long
D0 bne rel   D1 cmp dpiy  D2 cmp dpi   D3 cmp sriy  D4 pei dpi   D5 cmp dpx   D6 dec dpx   D7 cmp dpily
D8 cld imp   D9 cmp absy  DA phx imp   DB stp imp   DC jml absil DD cmp absx  DE dec absx  DF cmp longx
E0 cpx immx  E1 sbc dpxi  E2 sep imm   E3 sbc sr    E4 cpx dp    E5 sbc dp    E6 inc dp    E7 sbc dpil
E8 inx imp   E9 sbc immm  EA nop imp   EB xba imp   EC cpx abs   ED sbc abs   EE inc abs   EF sbc long
F0 beq rel   F1 sbc dpiy  F2 sbc dpi   F3 sbc sriy  F4 pea abs   F5 sbc dpx   F6 inc dpx   F7 sbc dpily
F8 sed imp   F9 sbc absy  FA plx imp   FB xce imp   FC jsr absxi FD sbc absx  FE inc absx  FF sbc longx
"""
OPS = {}
_w = _T.split()
for _i in range(0, len(_w), 3):
    OPS[int(_w[_i], 16)] = (_w[_i + 1], _w[_i + 2])
assert len(OPS) == 256

FIXED = {'imp': 0, 'acc': 0, 'imm': 1, 'dp': 1, 'dpx': 1, 'dpy': 1, 'dpi': 1, 'dpxi': 1, 'dpiy': 1,
         'dpil': 1, 'dpily': 1, 'sr': 1, 'sriy': 1, 'rel': 1, 'abs': 2, 'absx': 2, 'absy': 2,
         'absi': 2, 'absxi': 2, 'absil': 2, 'blk': 2, 'rell': 2, 'long': 3, 'longx': 3}


def cpu_len(op, flags):
    mode = OPS[op][1]
    if mode == 'immm':
        return 1 + (1 if flags & M8 else 2)
    if mode == 'immx':
        return 1 + (1 if flags & X8 else 2)
    return 1 + FIXED[mode]


def gsu_len(op):
    if 0x05 <= op <= 0x0F or 0xA0 <= op <= 0xAF:
        return 2
    if op >= 0xF0:
        return 3
    return 1


def addr_of(off):
    """The address code at ROM offset `off` runs at: LoROM banks 00-3F, or its RAM copy."""
    return codemap.run_addr(off)


def h2(v):
    return '$%02X' % v


def h4(v):
    return '$%04X' % v


def h6(v):
    return '$%06X' % v


class Rom:
    def __init__(self, rom, maps, incbin=None, claimed=None):
        self.rom = rom
        self.claimed = claimed or bytearray(len(rom))  # 1 where filed source owns the byte
        self.incbin = incbin            # data comes from this file at build time
        self.map = [0] * len(rom)
        for m in maps:
            for i, v in enumerate(m):
                self.map[i] |= v
        self.kind = [None] * len(rom)   # 'c' / 'g' instruction start
        self.length = [0] * len(rom)
        self.labels = {}                # offset -> name
        self.rejected = []
        # ROM offset -> APU RAM address, for the uploaded SPC700 blocks.
        self.apu = {}
        for bl, _ in spcmap.lists(rom).values():
            for dst, n, o in bl:
                for k in range(n):
                    self.apu.setdefault(o + k, (dst + k) & 0xFFFF)
        self.decode()

    def decode(self):
        rom, mp = self.rom, self.map
        owner = [False] * len(rom)
        for off in range(len(rom)):
            v = mp[off]
            if self.claimed[off]:
                continue
            if v & CPU_OP:
                n = cpu_len(rom[off], v)
                kind = 'c'
            elif v & GSU_OP:
                n = gsu_len(rom[off])
                kind = 'g'
            elif v & SPC_OP and off in self.apu:
                n = spc700.length(rom[off])
                kind = 's'
            else:
                continue
            span = range(off, off + n)
            if any(self.claimed[i] for i in span):
                self.rejected.append((off, kind, 'runs into filed source'))
                continue
            if off + n > len(rom) or (off // BANK) != ((off + n - 1) // BANK) or any(owner[i] for i in span) \
                    or any(mp[i] & (CPU_OP | GSU_OP | SPC_OP) for i in span[1:]) \
                    or (kind == 's' and any(self.apu.get(i) != (self.apu[off] + i - off) & 0xFFFF for i in span)):
                self.rejected.append((off, kind, 'overlap or bank crossing'))
                continue
            for i in span:
                owner[i] = True
            self.kind[off] = kind
            self.length[off] = n
        # Branch targets.
        for off in range(len(rom)):
            k = self.kind[off]
            if k == 'c':
                t = self.cpu_target(off)
                if t is not None and self.kind[t] == 'c':
                    self.labels.setdefault(t, 'loc_%06X' % addr_of(t))
            elif k == 'g':
                t = self.gsu_target(off)
                if t is not None and self.kind[t] == 'g':
                    self.labels.setdefault(t, 'gsu_%06X' % addr_of(t))
            elif k == 's':
                t = self.spc_target(off)
                if t is not None and self.kind[t] == 's':
                    self.labels.setdefault(t, 'spc_%04X_%06X' % (self.apu[t], t))

    def spc_vals(self, off):
        n = self.length[off]
        return spc700.operands(self.rom[off], self.rom[off + 1:off + n], self.apu[off])

    def spc_target(self, off):
        """ROM offset of a relative branch's target in the same block, or None."""
        v = self.spc_vals(off)
        if 'r' not in v:
            return None
        d = (v['r'] - self.apu[off]) & 0xFFFF
        t = off + (d - 0x10000 if d >= 0x8000 else d)
        if t >= len(self.rom) or self.apu.get(t) != v['r'] or t // BANK != off // BANK:
            return None
        return t

    def spc_text(self, off):
        t = self.spc_target(off)
        lab = self.labels.get(t) if t is not None else None
        return spc700.text(self.rom[off], self.spc_vals(off), (lambda f, v: lab if f == 'r' else None) if lab else None)

    def rel_target(self, off, n, disp):
        a = addr_of(off)
        t = codemap.rom_offset((a & 0xFF0000) | ((a + n + disp) & 0xFFFF))
        if t is None or t // BANK != off // BANK or addr_of(t) & 0xFF0000 != a & 0xFF0000:
            return None
        return t

    def cpu_target(self, off):
        op = self.rom[off]
        mode = OPS[op][1]
        if mode == 'rel':
            d = self.rom[off + 1]
            return self.rel_target(off, 2, d - 256 if d >= 128 else d)
        if mode == 'rell':
            d = self.rom[off + 1] | self.rom[off + 2] << 8
            return self.rel_target(off, 3, d - 65536 if d >= 32768 else d)
        return None

    def gsu_target(self, off):
        op = self.rom[off]
        if 0x05 <= op <= 0x0F:
            d = self.rom[off + 1]
            return self.rel_target(off, 2, d - 256 if d >= 128 else d)
        return None

    def cpu_text(self, off):
        rom = self.rom
        op = rom[off]
        mn, mode = OPS[op]
        n = self.length[off]
        b = rom[off + 1:off + n]
        v = int.from_bytes(b, 'little') if b else 0
        if mode == 'imp':
            return mn
        if mode == 'acc':
            return mn + ' a'
        if mode in ('imm',):
            return '%s.b #%s' % (mn, h2(v))
        if mode in ('immm', 'immx'):
            return '%s.%s #%s' % (mn, 'b' if n == 2 else 'w', h2(v) if n == 2 else h4(v))
        if mode in ('rel', 'rell'):
            t = self.cpu_target(off)
            if t is not None and t in self.labels:
                return '%s %s' % (mn, self.labels[t])
            return '%s %s' % (mn, h2(v) if mode == 'rel' else h4(v))
        if mode == 'blk':
            return '%s %s,%s' % (mn, h2(b[0]), h2(b[1]))
        fmt = {'dp': '%s.b %s', 'dpx': '%s.b %s,x', 'dpy': '%s.b %s,y', 'dpi': '%s.b (%s)',
               'dpxi': '%s.b (%s,x)', 'dpiy': '%s.b (%s),y', 'dpil': '%s.b [%s]', 'dpily': '%s.b [%s],y',
               'sr': '%s.b %s,s', 'sriy': '%s.b (%s,s),y', 'abs': '%s.w %s', 'absx': '%s.w %s,x',
               'absy': '%s.w %s,y', 'absi': '%s.w (%s)', 'absxi': '%s.w (%s,x)', 'absil': '%s.w [%s]',
               'long': '%s.l %s', 'longx': '%s.l %s,x'}[mode]
        hx = {1: h2, 2: h4, 3: h6}[n - 1]
        return fmt % (mn, hx(v))

    def gsu_text(self, off):
        rom = self.rom
        op = rom[off]
        lo = op & 15
        hi = op >> 4
        fixed = {0x00: 'stop', 0x01: 'nop', 0x02: 'cache', 0x03: 'lsr', 0x04: 'rol', 0x3C: 'loop',
                 0x3D: 'alt1', 0x3E: 'alt2', 0x3F: 'alt3', 0x4C: 'plot', 0x4D: 'swap', 0x4E: 'color',
                 0x4F: 'not', 0x70: 'merge', 0x90: 'sbk', 0x95: 'sex', 0x96: 'asr', 0x97: 'ror',
                 0x9E: 'lob', 0x9F: 'fmult', 0xC0: 'hib', 0xDF: 'getc', 0xEF: 'getb'}
        if op in fixed:
            return fixed[op]
        if 0x05 <= op <= 0x0F:
            mn = ['bra', 'bge', 'blt', 'bne', 'beq', 'bpl', 'bmi', 'bcc', 'bcs', 'bvc', 'bvs'][op - 5]
            t = self.gsu_target(off)
            if t is not None and t in self.labels:
                return '%s %s' % (mn, self.labels[t])
            return '%s %s' % (mn, h2(rom[off + 1]))
        r = 'r%d' % lo
        if hi == 0x1: return 'to ' + r
        if hi == 0x2: return 'with ' + r
        if hi == 0x3: return 'stw (%s)' % r
        if hi == 0x4: return 'ldw (%s)' % r
        if hi == 0x5: return 'add ' + r
        if hi == 0x6: return 'sub ' + r
        if hi == 0x7: return 'and ' + r
        if hi == 0x8: return 'mult ' + r
        if 0x91 <= op <= 0x94: return 'link #%d' % lo
        if 0x98 <= op <= 0x9D: return 'jmp ' + r
        if hi == 0xA: return 'ibt %s,#%s' % (r, h2(rom[off + 1]))
        if hi == 0xB: return 'from ' + r
        if hi == 0xC: return 'or ' + r
        if hi == 0xD: return 'inc ' + r
        if hi == 0xE: return 'dec ' + r
        if hi == 0xF: return 'iwt %s,#%s' % (r, h4(rom[off + 1] | rom[off + 2] << 8))
        raise ValueError(hex(op))

    def gsu_combined(self, off):
        """A prefix and the instruction it modifies as one mnemonic (same bytes),
        or None. Only when the second byte is not a branch target."""
        rom = self.rom
        a = rom[off]
        nxt = off + self.length[off]
        if self.kind[nxt] != 'g' or nxt in self.labels or nxt // BANK != off // BANK:
            return None
        op = rom[nxt]
        hi, lo = op >> 4, op & 15
        r = 'r%d' % lo
        if a in (0x3D, 0x3E, 0x3F):
            alt = a - 0x3C
            fixed = {(1, 0x4C): 'rpix', (1, 0x4E): 'cmode', (1, 0x96): 'div2', (1, 0x9F): 'lmult',
                     (1, 0xEF): 'getbh', (2, 0xEF): 'getbl', (3, 0xEF): 'getbs', (2, 0xDF): 'ramb', (3, 0xDF): 'romb'}
            if (alt, op) in fixed:
                return fixed[(alt, op)], nxt + 1
            if hi == 0x3 and lo < 12 and alt == 1:
                return 'stb (%s)' % r, nxt + 1
            if hi == 0x4 and lo < 12 and alt == 1:
                return 'ldb (%s)' % r, nxt + 1
            table = {0x5: ('adc ' + r, 'add #%d' % lo, 'adc #%d' % lo),
                     0x6: ('sbc ' + r, 'sub #%d' % lo, 'cmp ' + r),
                     0x8: ('umult ' + r, 'mult #%d' % lo, 'umult #%d' % lo)}
            if lo >= 1:
                table[0x7] = ('bic ' + r, 'and #%d' % lo, 'bic #%d' % lo)
                table[0xC] = ('xor ' + r, 'or #%d' % lo, 'xor #%d' % lo)
            if hi in table:
                return table[hi][alt - 1], nxt + 1
            if 0x98 <= op <= 0x9D and alt == 1:
                return 'ljmp ' + r, nxt + 1
            if hi == 0xA and alt in (1, 2):
                v = rom[nxt + 1] * 2
                return ('lms %s,(%s)' % (r, h4(v)) if alt == 1 else 'sms (%s),%s' % (h4(v), r)), nxt + 2
            if hi == 0xF and alt in (1, 2):
                v = rom[nxt + 1] | rom[nxt + 2] << 8
                return ('lm %s,(%s)' % (r, h4(v)) if alt == 1 else 'sm (%s),%s' % (h4(v), r)), nxt + 3
            return None
        if a >> 4 == 0x2 and hi in (0x1, 0xB):
            src = 'r%d' % (a & 15)
            return ('move %s,%s' % (r, src) if hi == 0x1 else 'moves %s,%s' % (src, r)), nxt + 1
        return None

    def bank_lines(self, b):
        rom = self.rom
        start, end = b * BANK, min((b + 1) * BANK, len(rom))
        out = ['; bank %02X: ROM %06X-%06X, runs at %06X' % (b, start, end - 1, addr_of(start)),
               'org %s' % h6(start), 'base %s' % h6(addr_of(start)), '']
        data = []
        arch = ['65816']

        def setarch(a):
            if arch[0] != a:
                out.append('arch %s' % a)
                arch[0] = a

        def flush():
            if data and self.incbin:
                out.append('    incbin "%s":%s..%s' % (self.incbin, h6(data[0]), h6(data[-1] + 1)))
            else:
                for i in range(0, len(data), 16):
                    out.append('    db ' + ','.join(h2(rom[x]) for x in data[i:i + 16]))
            data.clear()

        off = start
        gap = False
        prev = start
        spc_next, spc_pc = [None], [None]
        while off < end:
            if not self.claimed[off] and not gap and arch[0] != 'spc700' and addr_of(off) - addr_of(prev) != off - prev:
                flush()
                setarch('65816')
                out.append('base %s' % h6(addr_of(off)))
            prev = off
            if self.claimed[off]:
                flush()
                gap = True
                off += 1
                continue
            if gap:
                setarch('65816')
                out += ['', '; resumes after filed source', 'org %s' % h6(off), 'base %s' % h6(addr_of(off))]
                gap = False
            k = self.kind[off]
            # SPC700 code runs at its APU RAM address: the base is set before
            # its label, and kept across the data between its instructions
            # (incbin or db need no address) until a 65816 or Super FX one.
            if k == 's' and (arch[0] != 'spc700' or spc_next[0] != off or spc_pc[0] != self.apu[off]):
                flush()
                setarch('spc700')
                out.append('base $%04X' % self.apu[off])
            if off in self.labels:
                flush()
                out.append('%s:' % self.labels[off])
            if k == 's':
                flush()
                text = self.spc_text(off)
                out.append('    %-28s ; %06X spc %04X' % (text, addr_of(off), self.apu[off]))
                spc_next[0] = off + self.length[off]
                spc_pc[0] = (self.apu[off] + self.length[off]) & 0xFFFF
                off += self.length[off]
                continue
            if arch[0] == 'spc700' and (k or off in self.labels):
                setarch('65816')
                out.append('base %s' % h6(addr_of(off)))
            if k:
                flush()
                setarch('65816' if k == 'c' else 'superfx')
                comb = self.gsu_combined(off) if k == 'g' else None
                if comb:
                    text, after = comb
                    if any(self.claimed[i] for i in range(off, after)):
                        comb = None
                if comb:
                    out.append('    %-28s ; %06X gsu' % (text, addr_of(off)))
                    off = after
                    continue
                text = self.cpu_text(off) if k == 'c' else self.gsu_text(off)
                out.append('    %-28s ; %06X%s' % (text, addr_of(off), '' if k == 'c' else ' gsu'))
                off += self.length[off]
            else:
                data.append(off)
                off += 1
        flush()
        setarch('65816')
        out += ['', 'base off', '']
        return out



def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--rom', required=True)
    ap.add_argument('--trace', action='append', required=True)
    ap.add_argument('--out', required=True)
    ap.add_argument('--incbin', help='data bytes come from this file (path as the assembler sees it)')
    ap.add_argument('--claimed', help='file of start-end offset ranges (hex) owned by filed source')
    a = ap.parse_args()
    rom = open(a.rom, 'rb').read()
    maps = [codemap.merged(a.trace)]
    claimed = bytearray(len(rom))
    if a.claimed:
        for line in open(a.claimed):
            if line.strip():
                lo, hi = (int(x, 16) for x in line.split('-'))
                claimed[lo:hi] = b'\x01' * (hi - lo)
    r = Rom(rom, maps, a.incbin, claimed)
    os.makedirs(a.out, exist_ok=True)
    banks = (len(rom) + BANK - 1) // BANK
    for b in range(banks):
        with open(os.path.join(a.out, 'bank%02X.asm' % b), 'w') as f:
            f.write('\n'.join(r.bank_lines(b)) + '\n')
    with open(os.path.join(a.out, 'main.asm'), 'w') as f:
        f.write('; Raw intake: every byte no filed source claims.\n')
        for b in range(banks):
            f.write('incsrc "bank%02X.asm"\n' % b)
    ninsn = sum(1 for k in r.kind if k)
    print('instructions %d (cpu %d, gsu %d, spc %d), labels %d, rejected %d' % (
        ninsn, r.kind.count('c'), r.kind.count('g'), r.kind.count('s'), len(r.labels), len(r.rejected)))
    for off, kind, why in r.rejected[:20]:
        print('  rejected %06X %s: %s' % (off, kind, why))
    return 0


if __name__ == '__main__':
    sys.exit(main())
