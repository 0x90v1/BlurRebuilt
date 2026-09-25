#!/usr/bin/env python3
"""
survey.py - compare retail Xbox 360 and retail PC pak contents by virtual path.

  python survey.py <pc listing dir> <x360 listing dir>

Listing dirs hold <pak>.txt from paklist_pc.py / `xmem_verify --list`
("<size> <first 16 bytes hex> <path>"). Paths are placed where each build
mounts the pak (PC: CreateFsLayers PC branch; 360: the Xenon branch and
C_BootLayer), then grouped by extension: how many files each platform has, how
many exist on both, and for those, how many have the same size and the same
first 16 bytes. The leading 4 bytes (usually a platform FourCC or magic) are
tallied per platform.
"""
import collections
import os
import sys

PC_MOUNTS = {'root': '/', 'shaders-speed': '/graphics/shaders/speed/', 'root_streaming': '/audio/',
             'gamedata': '/', 'lut_rampak': '/', 'boot': '/'}
X360_MOUNTS = {'root': '/', 'root_streaming': '/', 'shaders-speed': '/graphics/shaders/speed/', 'gamedata': '/',
               'gamedata_boot': '/', 'boot': '/', 'boot_rampak': '/', 'lut_rampak': '/'}


def load(d, mounts):
    files = {}
    for pak, dst in mounts.items():
        p = os.path.join(d, pak + '.txt')
        if not os.path.exists(p):
            continue
        for line in open(p, encoding='latin-1'):
            parts = line.rstrip('\n').split(' ', 2)
            if len(parts) != 3 or not parts[0].isdigit():
                continue
            size, head, name = int(parts[0]), parts[1], parts[2]
            v = (dst + name.replace('\\', '/').lstrip('/')).lower()
            files.setdefault(v, (size, head, pak))
    return files


def fourcc(h):
    b = bytes.fromhex(h[:8].replace('-', '0'))
    return ''.join(chr(c) if 32 <= c < 127 else '.' for c in b)


def main(a):
    pc = load(a[1], PC_MOUNTS)
    xb = load(a[2], X360_MOUNTS)
    exts = collections.defaultdict(lambda: collections.Counter())
    magic = collections.defaultdict(lambda: [collections.Counter(), collections.Counter()])
    for v, (s, h, pak) in xb.items():
        e = os.path.splitext(v)[1] or '(none)'
        exts[e]['x360'] += 1
        magic[e][1][fourcc(h)] += 1
        if v in pc:
            ps, ph, _ = pc[v]
            exts[e]['both'] += 1
            exts[e]['same size'] += ps == s
            exts[e]['same head'] += ph == h
            exts[e]['identical-looking'] += ps == s and ph == h
    for v, (s, h, pak) in pc.items():
        e = os.path.splitext(v)[1] or '(none)'
        exts[e]['pc'] += 1
        magic[e][0][fourcc(h)] += 1
    print(f'{"ext":14} {"x360":>6} {"pc":>6} {"both":>6} {"=size":>6} {"=head":>6}  magic pc | magic x360')
    for e in sorted(exts, key=lambda k: -(exts[k]['x360'] + exts[k]['pc'])):
        c = exts[e]
        mp = ', '.join(f'{m!r}x{n}' for m, n in magic[e][0].most_common(2))
        mx = ', '.join(f'{m!r}x{n}' for m, n in magic[e][1].most_common(2))
        print(f'{e:14} {c["x360"]:6} {c["pc"]:6} {c["both"]:6} {c["same size"]:6} {c["same head"]:6}  {mp} | {mx}')
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
