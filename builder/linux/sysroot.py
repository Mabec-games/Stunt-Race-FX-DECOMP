#!/usr/bin/env python3
"""sysroot.py: Ubuntu 22.04's C library and headers for one CPU, unpacked into a folder.

The Linux game and builder are compiled against these (gcc --sysroot), so they
need only glibc 2.35, and run on Linux from 2022 on, whatever system builds
them. No root: the packages are fetched from Ubuntu's archive and unpacked
with dpkg-deb.

    sysroot.py ARCH DIR      ARCH: amd64 or arm64
"""
import gzip
import os
import subprocess
import sys
import urllib.request

PACKAGES = ['libc6', 'libc6-dev', 'linux-libc-dev', 'libgcc-s1']     # Vulkan's headers: brew's, as for Windows
MIRROR = {'amd64': 'http://archive.ubuntu.com/ubuntu', 'arm64': 'http://ports.ubuntu.com/ubuntu-ports'}
SUITES = ['jammy-updates', 'jammy']           # the newer first


def index(mirror, suite, arch):
    url = '%s/dists/%s/main/binary-%s/Packages.gz' % (mirror, suite, arch)
    text = gzip.decompress(urllib.request.urlopen(url, timeout=60).read()).decode('utf-8', 'replace')
    found = {}
    for stanza in text.split('\n\n'):
        fields = {}
        for line in stanza.splitlines():
            if ':' in line and not line.startswith(' '):
                k, v = line.split(':', 1)
                fields[k] = v.strip()
        if fields.get('Package') in PACKAGES:
            found[fields['Package']] = fields['Filename']
    return found


def main():
    arch, out = sys.argv[1], os.path.abspath(sys.argv[2])
    mirror = MIRROR[arch]
    files = {}
    for suite in reversed(SUITES):            # jammy, then jammy-updates over it
        files.update(index(mirror, suite, arch))
    missing = [p for p in PACKAGES if p not in files]
    if missing:
        sys.exit('sysroot.py: not in the archive: ' + ' '.join(missing))
    debs = os.path.join(out, '.debs')
    os.makedirs(debs, exist_ok=True)
    for p in PACKAGES:
        deb = os.path.join(debs, os.path.basename(files[p]))
        if not os.path.exists(deb):
            urllib.request.urlretrieve('%s/%s' % (mirror, files[p]), deb)
        subprocess.check_call(['dpkg-deb', '-x', deb, out])
        print('%s: %s' % (arch, os.path.basename(files[p])))
    # absolute links inside the tree (libm.so -> /lib/...) made relative to it
    for root, dirs, names in os.walk(out):
        for n in names + dirs:
            p = os.path.join(root, n)
            if os.path.islink(p) and os.readlink(p).startswith('/'):
                target = out + os.readlink(p)
                os.unlink(p)
                os.symlink(os.path.relpath(target, root), p)


main()
