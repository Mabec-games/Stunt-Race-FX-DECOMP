#!/usr/bin/env python3
"""ppm2png.py: convert sfxbox frame dumps (P6 PPM) to PNG, no dependencies.

    ppm2png.py FILE.ppm... [--montage OUT.png]
"""
import struct, sys, zlib


def read_ppm(path):
    d = open(path, 'rb').read()
    parts, i = [], 0
    while len(parts) < 4:
        while d[i:i + 1].isspace():
            i += 1
        j = i
        while not d[j:j + 1].isspace():
            j += 1
        parts.append(d[i:j]); i = j
    w, h = int(parts[1]), int(parts[2])
    return w, h, d[i + 1:i + 1 + w * h * 3]


def write_png(path, w, h, rgb):
    raw = b''.join(b'\0' + rgb[y * w * 3:(y + 1) * w * 3] for y in range(h))
    chunk = lambda t, b: struct.pack('>I', len(b)) + t + b + struct.pack('>I', zlib.crc32(t + b))
    open(path, 'wb').write(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 2, 0, 0, 0))
                           + chunk(b'IDAT', zlib.compress(raw, 6)) + chunk(b'IEND', b''))


def main(argv):
    out = None
    if '--montage' in argv:
        k = argv.index('--montage'); out = argv[k + 1]; argv = argv[:k] + argv[k + 2:]
    imgs = [read_ppm(p) for p in argv]
    if out:
        w = max(i[0] for i in imgs); h = max(i[1] for i in imgs); cols = min(4, len(imgs))
        rows = (len(imgs) + cols - 1) // cols
        W, H = cols * w, rows * h
        buf = bytearray(W * H * 3)
        for n, (iw, ih, px) in enumerate(imgs):
            ox, oy = (n % cols) * w, (n // cols) * h
            for y in range(ih):
                buf[((oy + y) * W + ox) * 3:((oy + y) * W + ox + iw) * 3] = px[y * iw * 3:(y + 1) * iw * 3]
        write_png(out, W, H, bytes(buf))
    else:
        for p, (w, h, px) in zip(argv, imgs):
            write_png(p[:-4] + '.png', w, h, px)


if __name__ == '__main__':
    main(sys.argv[1:])
