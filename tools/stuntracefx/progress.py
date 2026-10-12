#!/usr/bin/env python3
"""progress.py: the README's progress bars and the treemap, from one source.

    progress.py             rewrite the generated README blocks and docs/stuntracefx/progress-map.svg
    progress.py --print     print the bars only

Every routine in the catalog (catalog.py) gets a tier: T4 and T3 from the
gates (t3.py), T2 when the C or the assembly is filed but not certified, T1
otherwise. The bars and the map read the same classification, so they cannot
disagree. Bytes are the routines' executed code bytes; the denominator grows
as the scripts reach more code. Do not edit the generated blocks by hand.
"""
import datetime, html, os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, 'tools', 'stuntracefx'))
import catalog  # noqa: E402
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
    for r in rows:
        a = int(r['addr'], 16)
        name, tier = assessed.get(r['addr'], ('', 'T1'))
        r['tier'] = tier
        r['name'] = name or (tags[a][2] if a in tags else '')
        path = (tags.get(a) or [None, None])[0] or (tags.get(a) or [None, None])[1]
        r['area'] = path.split('/')[2] if path else ''
        r['bytes'] = int(r['bytes'])
    return rows, mp, image_ok


def bar(frac, width):
    n = int(round(frac * width))
    return '█' * n + '░' * (width - n)


def pct(a, b):
    return 100.0 * a / b if b else 0.0


def fmt(n):
    return '{:,}'.format(n)


def blocks(rows, mp):
    total = sum(r['bytes'] for r in rows)
    m1 = [r for r in rows if r['tier'] in ('T3', 'T4')]
    m2 = [r for r in rows if r['tier'] == 'T4']
    b1, b2 = sum(r['bytes'] for r in m1), sum(r['bytes'] for r in m2)
    today = datetime.date.today().isoformat()
    lines = ['_Snapshot %s._' % today, '', '```',
             'M1  Behaviour exact (T3 + T4)',
             '    %s  %5.1f%%   %s / %s B   %d / %d routines' % (bar(pct(b1, total) / 100, BAR_W), pct(b1, total), fmt(b1), fmt(total), len(m1), len(rows)),
             'M2  Byte exact (T4)',
             '    %s  %5.1f%%   %s / %s B   %d / %d routines' % (bar(pct(b2, total) / 100, BAR_W), pct(b2, total), fmt(b2), fmt(total), len(m2), len(rows)),
             '```', '', '| Processor | M1: behaviour exact | M2: byte exact |', '|---|---|---|']
    for cpu in ('cpu', 'gsu'):
        rs = [r for r in rows if r['cpu'] == cpu]
        t = sum(r['bytes'] for r in rs)
        x1 = sum(r['bytes'] for r in rs if r['tier'] in ('T3', 'T4'))
        x2 = sum(r['bytes'] for r in rs if r['tier'] == 'T4')
        n1 = sum(1 for r in rs if r['tier'] in ('T3', 'T4'))
        n2 = sum(1 for r in rs if r['tier'] == 'T4')
        lines.append('| **%s** | `%s` %.1f%%: %d/%d routines, %s/%s B | `%s` %.1f%%: %d/%d routines, %s/%s B |' % (
            LANE[cpu], bar(pct(x1, t) / 100, CELL_W), pct(x1, t), n1, len(rs), fmt(x1), fmt(t),
            bar(pct(x2, t) / 100, CELL_W), pct(x2, t), n2, len(rs), fmt(x2), fmt(t)))
    lines.append('| **SPC700 (sound)** | not yet measured | not yet measured |')
    status = '\n'.join(lines)

    code = sum(1 for v in mp if v & 0x33)
    data = sum(1 for v in mp if not v & 0x33 and v & 0x4C)
    untouched = sum(1 for v in mp if not v)
    t2 = [r for r in rows if r['tier'] == 'T2']
    detail = '\n'.join([
        '**What the bars measure.** Both bars count the executed code bytes of every',
        'routine the scripts have reached, on the S-CPU and the Super FX: %s B in %d' % (fmt(total), len(rows)),
        'routines. M1 counts routines whose C is certified behaviour exact (T3) or',
        'whose assembly is also byte exact (T4); M2 counts T4 only. %d routine(s) are' % len(t2),
        'in the tree but not yet certified (T2).',
        '',
        '**The denominator grows.** Code no script has run is not counted yet. Of the',
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


def render_svg(rows, path):
    W, H = 1600, 900
    groups = {}
    for r in rows:
        key = r['area'] or 'bank %02X %s' % (int(r['offset'], 16) // 0x8000, 'Super FX' if r['cpu'] == 'gsu' else 'S-CPU')
        groups.setdefault(key, []).append(r)
    parts = ['<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 %d %d" font-family="sans-serif">' % (W, H + 30),
             '<rect width="%d" height="%d" fill="#161b22"/>' % (W, H + 30)]
    gitems = sorted(((sum(r['bytes'] for r in rs), (k, rs)) for k, rs in groups.items()), key=lambda it: -it[0])
    for gx, gy, gw, gh, (gname, rs) in squarify(gitems, 0, 0, W, H):
        pad = 1.5
        label = gw > 70 and gh > 26
        top = 14 if label else 0
        if label:
            parts.append('<text x="%.1f" y="%.1f" font-size="10" fill="#8b949e">%s</text>' % (gx + 3, gy + 10.5, html.escape(gname)))
        items = sorted(((r['bytes'], r) for r in rs), key=lambda it: -it[0])
        for x, y, w, h, r in squarify(items, gx + pad, gy + pad + top, max(gw - 2 * pad, .1), max(gh - 2 * pad - top, .1)):
            tip = '%s  %s  %dB  %s  [%s]' % (r['name'] or '(unnamed)', r['addr'], r['bytes'], r['tier'], gname)
            parts.append('<rect x="%.1f" y="%.1f" width="%.1f" height="%.1f" fill="%s"><title>%s</title></rect>'
                         % (x, y, max(w - .6, .4), max(h - .6, .4), COLOR[r['tier']], html.escape(tip)))
    total = sum(r['bytes'] for r in rows)
    n4 = sum(1 for r in rows if r['tier'] == 'T4')
    n3 = sum(1 for r in rows if r['tier'] == 'T3')
    b4 = sum(r['bytes'] for r in rows if r['tier'] == 'T4')
    parts.append('<text x="8" y="%d" font-size="13" fill="#e6edf3">T4 byte-exact %d &#183; T3 behaviour-exact %d &#183; '
                 '%s/%s bytes exact (%.1f%%) &#183; green=T4 blue=T3 amber=T2 gray=T1</text></svg>'
                 % (H + 20, n4, n3, fmt(b4), fmt(total), pct(b4, total)))
    os.makedirs(os.path.dirname(path), exist_ok=True)
    open(path, 'w').write('\n'.join(parts) + '\n')


def main(argv):
    rows, mp, image_ok = classify()
    status, detail = blocks(rows, mp)
    if '--print' in argv:
        print(status)
        return 0
    text = open(README).read()
    text = replace_block(text, 'PROGRESS', status)
    text = replace_block(text, 'PROGRESS-DETAIL', detail)
    open(README, 'w').write(text)
    render_svg(rows, SVG)
    print(status.split('```')[1].strip())
    print('README.md and %s updated' % os.path.relpath(SVG, ROOT))
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
