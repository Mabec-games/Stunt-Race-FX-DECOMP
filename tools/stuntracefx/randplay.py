#!/usr/bin/env python3
"""randplay.py: seeded random-play scripts, for coverage.

    randplay.py menus SEED FRAMES OUT.sfx
        from power-on, random taps on every button, wandering the menus
    randplay.py drive BASE.sfx SEED FRAMES OUT.sfx
        BASE's lines up to its 'shot racestart', then random driving

The hand-written scripts follow one path each; random input reaches menu
entries, options and race events they never do. The same seed always gives
the same script, so a run is repeatable and the gates can use it.
"""
import random, sys

BUTTONS = ['UP', 'DOWN', 'LEFT', 'RIGHT', 'A', 'B', 'X', 'Y', 'L', 'R', 'START', 'SELECT']
DRIVE = ['B', 'B', 'B', 'B+LEFT', 'B+RIGHT', 'B+UP', 'B+DOWN', 'A', 'B+A', 'B+X', 'Y', 'B+Y', 'B+L', 'B+R',
         'LEFT', 'RIGHT', 'X']


def menus(r, frames):
    out, t = ['wait 300'], 300
    while t < frames:
        b = r.choice(BUTTONS + ['A', 'START', 'DOWN', 'RIGHT'])
        w = r.randrange(15, 140)
        out += ['press %s' % b, 'wait %d' % w]
        t += w + 4
    return out


def drive(r, frames):
    out, t = [], 0
    while t < frames:
        b = r.choice(DRIVE)
        n = r.randrange(4, 90)
        out.append('hold %s %d' % (b, n))
        t += n
        if r.random() < 0.01:
            out += ['press START', 'wait 60', 'press START']
            t += 66
    return out


def main(argv):
    if argv[:1] == ['menus'] and len(argv) == 4:
        seed, frames, path = int(argv[1]), int(argv[2]), argv[3]
        head = ['# Random menu play, seed %d (randplay.py menus %d %d).' % (seed, seed, frames)]
        body = menus(random.Random(seed), frames)
    elif argv[:1] == ['drive'] and len(argv) == 5:
        base, seed, frames, path = argv[1], int(argv[2]), int(argv[3]), argv[4]
        head = ['# Random driving after %s, seed %d (randplay.py drive).' % (base.split('/')[-1], seed)]
        for line in open(base):
            head.append(line.rstrip('\n'))
            if line.strip() == 'shot racestart':
                break
        else:
            raise SystemExit('%s has no shot racestart' % base)
        body = drive(random.Random(seed), frames) + ['shot driven']
    else:
        raise SystemExit(__doc__)
    open(path, 'w').write('\n'.join(head + body) + '\n')
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
