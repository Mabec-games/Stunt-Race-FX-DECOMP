#!/usr/bin/env python3
"""progress.py: the README's progress bars and the treemap, from one source.

    progress.py             rewrite the generated README blocks and docs/stuntracefx/progress-map.svg
    progress.py --print     print the bars only

Every routine in the catalog (catalog.py) gets a tier: T4 and T3 from the
gates (t3.py), T2 when the C or the assembly is filed but not certified, T1
otherwise. The bars and the map read the same classification, so they cannot
disagree. The bars measure bytes, not routines: the denominator is every code
byte of the game, the ones the scripts ran and the ones the routine census
(census.py) finds statically; the numerator is the code bytes inside
certified routines (their executed bytes and the untraced paths between
them). A routine count is no measure of the whole: every entry point filed
or traced splits the code into more routines, while its bytes stay put.

Each code byte takes the tier of the tagged routine whose filed span
(t3.span_of: from its address through the filed bytes up to the next tag)
holds it; a byte no filed span holds takes the tier of the catalog routine
whose traced extent holds it. A catalog routine can run over several tagged
ones (an entry no script calls has no catalog row of its own), so crediting
its whole extent with the tier at its start would count the others' bytes
at the wrong tier. Do not edit the generated blocks by hand.
"""
import datetime, html, os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, 'tools', 'stuntracefx'))
import build    # noqa: E402
import catalog  # noqa: E402
import codemap  # noqa: E402
import census   # noqa: E402
import t3       # noqa: E402

README = os.path.join(ROOT, 'README.md')
SVG = os.path.join(ROOT, 'docs', 'stuntracefx', 'progress-map.svg')
GREEN, BLUE, AMBER, GRAY = '#3fb950', '#58a6ff', '#d29922', '#c8ccd2'
COLOR = {'T4': GREEN, 'T3': BLUE, 'T2': AMBER, 'T1': GRAY}
LANE = {'cpu': 'S-CPU (65816)', 'gsu': 'Super FX (GSU-1)'}
BAR_W, CELL_W = 40, 20


def tagged():
    """Address -> (C path, assembly path) for every @routine in the tree."""
    out = {}
    for sub, ext, slot in (('src', '.c', 0), ('asm', '.asm', 1)):
        root = os.path.join(ROOT, sub, 'stuntracefx')
        for dp, dn, fn in os.walk(root):
            if os.path.relpath(dp, root).split(os.sep)[0] in ('include', 'raw'):
                continue
            for f in fn:
                if f.endswith(ext):
                    p = os.path.join(dp, f)
                    for m in t3.TAG.finditer(open(p).read()):
                        a = int(m.group(1), 16) << 16 | int(m.group(2), 16)
                        out.setdefault(a, [None, None, m.group(3)])[slot] = os.path.relpath(p, ROOT)
    return out


def classify():
    rows, mp = catalog.build()
    tags = tagged()
    image_ok = os.system('%s %s >/dev/null' % (sys.executable, os.path.join(ROOT, 'tools', 'stuntracefx', 'image.py'))) == 0
    assessed = {k: (name, tier) for k, name, tier, _ in t3.assess(['%06X' % a for a in sorted(tags)], image_ok)}
    # The filed bytes as build.py claims them from the tree now (the build's
    # claimed.txt can be older than the tree).
    owned = set(build.claims(build.filed_sources(), os.path.getsize(build.BASEROM))) if build.filed_sources() else set()
    others = t3.asm_tag_offsets()
    have = {int(r['addr'], 16) for r in rows}
    for a in sorted(tags):
        if tags[a][1] and a not in have:
            # A tagged routine no script has called: no catalog row, but its
            # filed span holds code bytes.
            o = codemap.rom_offset(a)
            rows.append({'addr': '%06X' % a, 'offset': '%06X' % o, 'end': '%06X' % o, 'cpu': None, 'bytes': 0})
    for r in rows:
        a = int(r['addr'], 16)
        name, tier = assessed.get(r['addr'], ('', 'T1'))
        r['tier'] = tier
        r['name'] = name or (tags[a][2] if a in tags else '')
        path = (tags.get(a) or [None, None])[0] or (tags.get(a) or [None, None])[1]
        r['area'] = path.split('/')[2] if path else ''
        r['bytes'] = int(r['bytes'])
        # Only an assembly-tagged routine has a filed span (as in t3.assess).
        r['span'] = t3.filed_span(a, owned, others) if a in tags and tags[a][1] else []
    return rows, mp, image_ok


def bar(frac, width):
    n = int(round(frac * width))
    return '█' * n + '░' * (width - n)


def pct(a, b):
    return 100.0 * a / b if b else 0.0


def fmt(n):
    return '{:,}'.format(n)


def code_bytes(rows, c):
    """Each routine's code bytes (traced or found by the census, of its own
    processor) and the game's code bytes per processor. A byte belongs to the
    routine whose filed span holds it, else to the catalog routine whose
    extent (its start to its last traced byte) holds it."""
    mp = c.mp
    gsu = bytearray(len(mp))
    for o in range(len(mp)):
        if mp[o] & 0x30 or (c.gcode[o] and not mp[o] & 0x03):
            gsu[o] = 2
        elif mp[o] & 0x03 or c.code[o]:
            gsu[o] = 1
    owner = [None] * len(mp)
    for i, r in enumerate(rows):
        for o in range(int(r['offset'], 16), int(r['end'], 16)):
            owner[o] = i
    for i, r in enumerate(rows):
        for o in r['span']:
            owner[o] = i
        if r['cpu'] is None:
            r['cpu'] = 'gsu' if gsu[int(r['offset'], 16)] == 2 else 'cpu'
    for r in rows:
        r['code'] = 0
    for o, i in enumerate(owner):
        if i is not None and gsu[o] == (2 if rows[i]['cpu'] == 'gsu' else 1):
            rows[i]['code'] += 1
    return {'cpu': gsu.count(1), 'gsu': gsu.count(2)}


def blocks(rows, mp, est, game):
    total = game['cpu'] + game['gsu']
    m1 = [r for r in rows if r['tier'] in ('T3', 'T4')]
    m2 = [r for r in rows if r['tier'] == 'T4']
    b1, b2 = sum(r['code'] for r in m1), sum(r['code'] for r in m2)
    traced = sum(r['bytes'] for r in rows)
    today = datetime.date.today().isoformat()
    lines = ['_Snapshot %s._' % today, '', '```',
             'M1  Behaviour exact (T3 + T4)',
             '    %s  %5.1f%%   %s / %s B of the game\'s code' % (bar(pct(b1, total) / 100, BAR_W), pct(b1, total), fmt(b1), fmt(total)),
             'M2  Byte exact (T4)',
             '    %s  %5.1f%%   %s / %s B of the game\'s code' % (bar(pct(b2, total) / 100, BAR_W), pct(b2, total), fmt(b2), fmt(total)),
             '```', '',
             'The bars measure the whole game\'s S-CPU and Super FX code, %s B: %s B the scripts have run and %s B the '
             '[routine census](docs/stuntracefx/ROUTINE_CENSUS.md) finds that no script has run yet. '
             'Certified so far: %d routines behaviour exact, %d of them byte exact.' % (
                 fmt(total), fmt(traced), fmt(total - traced), len(m1), len(m2)),
             '', '| Processor | M1: behaviour exact | M2: byte exact |', '|---|---|---|']
    for cpu in ('cpu', 'gsu'):
        rs = [r for r in rows if r['cpu'] == cpu]
        t = game[cpu]
        x1 = sum(r['code'] for r in rs if r['tier'] in ('T3', 'T4'))
        x2 = sum(r['code'] for r in rs if r['tier'] == 'T4')
        lines.append('| **%s** | `%s` %.1f%%: %s/%s B | `%s` %.1f%%: %s/%s B |' % (
            LANE[cpu], bar(pct(x1, t) / 100, CELL_W), pct(x1, t), fmt(x1), fmt(t),
            bar(pct(x2, t) / 100, CELL_W), pct(x2, t), fmt(x2), fmt(t)))
    lines.append('| **SPC700 (sound)** | not yet measured | not yet measured |')
    status = '\n'.join(lines)

    code = sum(1 for v in mp if v & 0x33)
    data = sum(1 for v in mp if not v & 0x33 and v & 0x4C)
    untouched = sum(1 for v in mp if not v)
    t2 = [r for r in rows if r['tier'] == 'T2']
    detail = '\n'.join([
        '**What the bars measure.** Bytes of code, not routines. The denominator is',
        'every S-CPU and Super FX code byte of the game, %s B: the %s B the scripts' % (fmt(total), fmt(traced)),
        'have run and the %s B the routine census (`tools/stuntracefx/census.py`) finds' % fmt(total - traced),
        'by following the code statically (calls, jumps, tables, handler pointers,',
        'Super FX calls, orphans) where no script has gone. The numerator is the code',
        'bytes inside routines whose C is certified behaviour exact (T3) or whose',
        'assembly is also byte exact (T4), for M1, and T4 only for M2. %d routine(s)' % len(t2),
        'are in the tree but not yet certified (T2).',
        '',
        '**Why not routines.** A routine count keeps growing as the work goes on:',
        'every entry point filed in C or reached by a new script splits code already',
        'counted into more routines (the census now counts about %s, %s of them' % (fmt(est['expected']), fmt(est['named'])),
        'named), while the bytes stay put: tracing a new script moves bytes from',
        '"found" to "run" and barely changes the total. Of the',
        'ROM\'s %s B, the scripts have run %s B as code and read %s B as data;' % (fmt(len(mp)), fmt(code), fmt(data)),
        '%s B have not been touched by any script. The sound CPU\'s program is' % fmt(untouched),
        'uploaded from ROM and is not measured yet.',
        '',
        '**Byte-exact rebuild.** Independently of the tiers, the whole ROM already',
        'rebuilds byte for byte from source: filed, commented assembly where it',
        'exists and the raw intake for everything else (`tools/stuntracefx/image.py`).',
    ])
    return status, detail


def replace_block(text, tag, body):
    pat = re.compile(r'(<!-- %s:BEGIN[^>]*-->\n)(?:.*?\n)?(<!-- %s:END -->)' % (tag, tag), re.S)
    if not pat.search(text):
        raise SystemExit('README has no %s block' % tag)
    return pat.sub(lambda m: m.group(1) + body + '\n' + m.group(2), text)


def squarify(items, x, y, w, h):
    items = [it for it in items if it[0] > 0]
    total = sum(it[0] for it in items)
    if not items or total <= 0 or w <= 0 or h <= 0:
        return
    scale = w * h / total
    i = 0
    while i < len(items):
        vertical = w < h
        side = w if vertical else h
        row, rs, worst, j = [], 0.0, None, i
        while j < len(items):
            nrs = rs + items[j][0] * scale
            thick = nrs / side
            wr = max(max(items[k][0] * scale / thick / thick, thick * thick / (items[k][0] * scale)) for k in range(i, j + 1))
            if worst is not None and wr > worst:
                break
            worst, rs = wr, nrs
            row.append(items[j])
            j += 1
        thick = rs / side
        off = 0.0
        for wt, payload in row:
            length = wt * scale / thick
            yield (x + off, y, length, thick, payload) if vertical else (x, y + off, thick, length, payload)
            off += length
        if vertical:
            y += thick; h -= thick
        else:
            x += thick; w -= thick
        i = j


def render_svg(rows, path, game):
    W, H = 1600, 900
    groups = {}
    for r in rows:
        key = r['area'] or 'bank %02X %s' % (int(r['offset'], 16) // 0x8000, 'Super FX' if r['cpu'] == 'gsu' else 'S-CPU')
        groups.setdefault(key, []).append(r)
    parts = ['<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 %d %d" font-family="sans-serif">' % (W, H + 30),
             '<rect width="%d" height="%d" fill="#161b22"/>' % (W, H + 30)]
    # Code no script has run (the census found it) is one gray area, so the
    # map's areas are the bars' bytes.
    unrun = game['cpu'] + game['gsu'] - sum(r['code'] for r in rows)
    groups['not run by any script yet'] = [{'name': 'code the census found', 'addr': '', 'code': unrun, 'tier': 'T1'}]
    gitems = sorted(((sum(r['code'] for r in rs), (k, rs)) for k, rs in groups.items()), key=lambda it: -it[0])
    for gx, gy, gw, gh, (gname, rs) in squarify(gitems, 0, 0, W, H):
        pad = 1.5
        label = gw > 70 and gh > 26
        top = 14 if label else 0
        if label:
            parts.append('<text x="%.1f" y="%.1f" font-size="10" fill="#8b949e">%s</text>' % (gx + 3, gy + 10.5, html.escape(gname)))
        items = sorted(((r['code'], r) for r in rs), key=lambda it: -it[0])
        for x, y, w, h, r in squarify(items, gx + pad, gy + pad + top, max(gw - 2 * pad, .1), max(gh - 2 * pad - top, .1)):
            tip = '%s  %s  %dB  %s  [%s]' % (r['name'] or '(unnamed)', r['addr'], r['code'], r['tier'], gname)
            parts.append('<rect x="%.1f" y="%.1f" width="%.1f" height="%.1f" fill="%s"><title>%s</title></rect>'
                         % (x, y, max(w - .6, .4), max(h - .6, .4), COLOR[r['tier']], html.escape(tip)))
    total = game['cpu'] + game['gsu']
    n4 = sum(1 for r in rows if r['tier'] == 'T4')
    n3 = sum(1 for r in rows if r['tier'] == 'T3')
    b4 = sum(r['code'] for r in rows if r['tier'] == 'T4')
    parts.append('<text x="8" y="%d" font-size="13" fill="#e6edf3">T4 byte-exact %d &#183; T3 behaviour-exact %d &#183; '
                 '%s/%s bytes exact (%.1f%%) &#183; green=T4 blue=T3 amber=T2 gray=T1</text></svg>'
                 % (H + 20, n4, n3, fmt(b4), fmt(total), pct(b4, total)))
    os.makedirs(os.path.dirname(path), exist_ok=True)
    open(path, 'w').write('\n'.join(parts) + '\n')


def main(argv):
    rows, mp, image_ok = classify()
    state = census.census()
    game = code_bytes(rows, state[0])
    status, detail = blocks(rows, mp, census.estimate(state), game)
    if '--print' in argv:
        print(status)
        return 0
    text = open(README).read()
    text = replace_block(text, 'PROGRESS', status)
    text = replace_block(text, 'PROGRESS-DETAIL', detail)
    open(README, 'w').write(text)
    render_svg(rows, SVG, game)
    print(status.split('```')[1].strip())
    print('README.md and %s updated' % os.path.relpath(SVG, ROOT))
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
