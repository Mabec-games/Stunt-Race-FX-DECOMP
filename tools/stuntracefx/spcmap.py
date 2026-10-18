#!/usr/bin/env python3
"""spcmap.py: the SPC700's traced code and data, folded back onto the ROM.

    spcmap.py [TRACE_DIR...]    summarize the fold of each trace (default:
                                every trace under build/stuntracefx/box)

The S-CPU uploads the SPC700's program and data as block lists (ApuUpload,
04:D720): a byte count, an APU RAM address and the bytes, until a count of
0 and the address to start at. sfxbox --trace writes spc_exec.bin: for each
APU RAM address, flags (1 opcode fetched, 2 operand fetched, 4 read as
data, 8 more than two different bytes fetched) and the byte fetched first,
then the flags and byte of a second, different byte fetched there.
A traced address belongs to the uploaded ROM byte that put that byte
there. Blocks overlap (the battle driver replaces most of the race one, the
music lists rewrite their areas), so a block counts in a trace only when
every byte the trace fetched in its range equals the block's ROM byte; of
several such blocks, those the S-CPU read from the ROM in that trace (it
uploaded them) are taken when there are any. Code from Super FX RAM (blocks
the Super FX unpacked) has no ROM byte and stays unfolded.
"""
import glob, os, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
B = os.path.join(ROOT, 'build', 'stuntracefx')

SPC_OP, SPC_OPND, SPC_DATA = 1 << 7, 1 << 13, 1 << 14


def _rom():
    return open(os.path.join(B, 'baserom.sfc'), 'rb').read()


def rom_offset(bank, addr):
    return (bank & 0x7F) * 0x8000 + (addr & 0x7FFF)


def block_list(rom, bank, addr):
    """[(apu address, length, rom offset of its first byte)], start address."""
    o, out = rom_offset(bank, addr), []
    while True:
        n = rom[o] | rom[o + 1] << 8
        dst = rom[o + 2] | rom[o + 3] << 8
        o += 4
        if n == 0:
            return out, dst
        out.append((dst, n, o))
        o += n


# The lists the S-CPU code uploads by address (LDA #bank; LDY #address;
# JSL or JSR ApuUpload), by call site.
FIXED = [
    (0x19, 0xC27C),   # 04:D44F ApuUploadBank19, 07:ABA6: the driver
    (0x1A, 0xF091),   # 03:D96F: the battle driver ($10B0-$260E)
    (0x1B, 0xB8A0),   # 03:F123, 04:D16A
    (0x1B, 0xD5FC),   # 03:F146
    (0x19, 0x8D4C),   # 04:D10E
    (0x1B, 0x85F8),   # 07:EC25, 0B:EABA, 0B:EEA0
    (0x19, 0x9A98),   # 07:EC48, 0B:EADD, 0B:EEC3
]


def lists(rom=None):
    """Every block list the S-CPU uploads from the ROM: FIXED and the music
    lists of the tables at $04:D41E and $04:D42D (an address, $FFFF for
    none, and a bank)."""
    rom = rom or _rom()
    out = {}
    for bank, a in FIXED:
        out['$%02X:%04X' % (bank, a)] = block_list(rom, bank, a)
    for t in range(0xD41E, 0xD43C, 3):
        a = rom[rom_offset(4, t)] | rom[rom_offset(4, t + 1)] << 8
        if a != 0xFFFF:
            bank = rom[rom_offset(4, t + 2)]
            out.setdefault('$%02X:%04X' % (bank, a), block_list(rom, bank, a))
    return out


def versions(trace_dir):
    """Per APU address: [(byte, flags)] for up to two different bytes the
    SPC700 fetched there (flags 1 opcode, 2 operand; 4 read as data, 8 more
    versions than two, both on the first), or None without SPC700 trace."""
    path = os.path.join(trace_dir, 'spc_exec.bin')
    if not os.path.exists(path):
        return None
    raw = open(path, 'rb').read()
    f1, v1 = raw[:0x10000], raw[0x10000:0x20000]
    f2, v2 = (raw[0x20000:0x30000], raw[0x30000:0x40000]) if len(raw) >= 0x40000 else (bytes(0x10000),) * 2
    return f1, v1, f2, v2


def fold(trace_dir, rom=None, blocks=None):
    """ROM offset -> SPC flags for one trace, and the traced APU addresses no
    uploaded ROM byte matches."""
    rom = rom or _rom()
    if blocks is None:
        blocks = [b for bl, _ in lists(rom).values() for b in bl]
    vs = versions(trace_dir)
    if vs is None:
        return {}, []
    f1, v1, f2, v2 = vs
    rm = open(os.path.join(trace_dir, 'rommap.bin'), 'rb').read()
    def cpu_read(o):
        return rm[2 * o] & 0x04
    def fits(a, b):
        """The ROM byte b could be what the SPC700 ran at a."""
        return (not f1[a] & 3 or v1[a] == b) or (f2[a] & 3 and v2[a] == b) or f1[a] & 8
    # Blocks resident in this trace: those the trace uploaded, and any other
    # whose bytes fit every fetch in its range outside the addresses an
    # uploaded block rewrote (another version ran there).
    upl = [(dst, n, o) for dst, n, o in blocks if any(cpu_read(o + k) for k in range(0, n, max(1, n // 16)))]
    rewritten = bytearray(0x10000)
    for dst, n, o in upl:
        for k in range(n):
            rewritten[(dst + k) & 0xFFFF] = 1
    good = []
    for dst, n, o in blocks:
        up = (dst, n, o) in upl
        hit = bad = False
        for k in range(n):
            a = (dst + k) & 0xFFFF
            if not (f1[a] & 7 or f2[a] & 3):
                continue
            hit = True
            if f1[a] & 3 and not rewritten[a] and not fits(a, rom[o + k]):
                bad = True
        if hit and (up or not bad):
            good.append((dst, n, o, up))
    cover = {}
    for dst, n, o, up in good:
        for k in range(n):
            cover.setdefault((dst + k) & 0xFFFF, []).append((o + k, up))
    def pick(cands):
        return [c for c in cands if c[1]] if any(c[1] for c in cands) else cands
    out, unfolded = {}, []
    for a in range(0x10000):
        if not (f1[a] & 7 or f2[a] & 3):
            continue
        cands = cover.get(a, [])
        found = False
        for val, fl in ((v1[a], f1[a]), (v2[a], f2[a])):
            if not fl & 3:
                continue
            bits = (SPC_OP if fl & 1 else 0) | (SPC_OPND if fl & 2 else 0)
            got = pick([c for c in cands if rom[c[0]] == val])
            found = found or bool(got)
            for o, _ in got:
                out[o] = out.get(o, 0) | bits
        if f1[a] & 4:
            for o, _ in pick(cands):
                out[o] = out.get(o, 0) | SPC_DATA
            found = found or bool(cands)
        if not found:
            unfolded.append(a)
    return out, unfolded


def main(argv):
    rom = _rom()
    ls = lists(rom)
    blocks = [b for bl, _ in ls.values() for b in bl]
    dirs = argv or sorted(os.path.dirname(p) for p in glob.glob(os.path.join(B, 'box', '*', 'spc_exec.bin')))
    total = {}
    for d in dirs:
        m, un = fold(d, rom, blocks)
        for o, v in m.items():
            total[o] = total.get(o, 0) | v
        f1, _, f2, _ = versions(d)
        code = [a for a in un if f1[a] & 3 or f2[a] & 3]
        print('%-28s %5d ROM bytes folded, %4d APU code bytes unfolded%s' % (
            os.path.basename(d), len(m), len(code), (' (%04X-%04X)' % (min(code), max(code))) if code else ''))
    ops = sum(1 for v in total.values() if v & SPC_OP)
    code = sum(1 for v in total.values() if v & (SPC_OP | SPC_OPND))
    print('all: %d ROM bytes of SPC700 code (%d opcodes), %d read as data' % (
        code, ops, sum(1 for v in total.values() if v & SPC_DATA and not v & (SPC_OP | SPC_OPND))))
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
