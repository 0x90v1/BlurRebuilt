#!/usr/bin/env python3
"""
texsurvey.py - tabulate Xbox 360 .texture files (Xenos fetch constant fields)
next to their retail PC counterparts (TextureDesc), to scope the texture
converter.

  python texsurvey.py <x360 extract root> <pc extract root>

Fetch constant fields follow Xenia's xe_gpu_texture_fetch_t (dwords big-endian).
"""
import collections
import os
import struct
import sys


def blocks(data, off, end, be):
    """yield (name, payload_start, payload_end) for the child blocks between off and end"""
    fmt = '>' if be else '<'
    while off < end:
        name = data[off:off + 8].split(b'\0')[0].decode('latin-1')
        size = struct.unpack(fmt + 'I', data[off + 8:off + 12])[0]
        yield name, off + 16, off + size
        off += size


def xenos(data):
    root_size = struct.unpack('>I', data[8:12])[0]
    for name, a, b in blocks(data, 16, root_size, True):
        if name == 'Data':
            d = struct.unpack('>13I', data[a:a + 52])
            f = d[7:13]
            size, align = struct.unpack('>II', data[a + 52:a + 60])
            return dict(
                tiled=f[0] >> 31, pitch=((f[0] >> 22) & 0x1ff) * 32, fmt=f[1] & 0x3f, endian=(f[1] >> 6) & 3,
                w=(f[2] & 0x1fff) + 1, h=((f[2] >> 13) & 0x1fff) + 1, depth=((f[2] >> 26) & 0x3f) + 1,
                mipmin=(f[4] >> 2) & 15, mipmax=(f[4] >> 6) & 15, dim=(f[5] >> 9) & 3,
                packed=(f[5] >> 11) & 1, mipaddr=f[5] >> 12, size=size, align=align,
                swizzle=(f[3] >> 1) & 0xfff, signs=(f[0] >> 2) & 0xff, numfmt=f[3] & 1,
                payload=b - (a + 64))
    return None


def pcdesc(data):
    root_size = struct.unpack('<I', data[8:12])[0]
    for name, a, b in blocks(data, 16, root_size, False):
        if name == 'Data':
            w, h, d, lv, fmt, usage, typ = struct.unpack('<iiiiIII', data[a:a + 28])
            fcc = struct.pack('<I', fmt)
            fs = fcc.decode('latin-1') if all(32 <= c < 127 for c in fcc) else str(fmt)
            return dict(w=w, h=h, depth=d, levels=lv, fmt=fs, type=typ, usage=usage, payload=b - (a + 28))
    return None


def main(a):
    xroot, proot = a[1], a[2]
    combos = collections.Counter()
    samples = {}
    for dp, _, files in os.walk(xroot):
        for fn in files:
            if not fn.lower().endswith('.texture'):
                continue
            xp = os.path.join(dp, fn)
            rel = os.path.relpath(xp, xroot)
            rel_in_pak = rel.split(os.sep, 1)[1]
            pc = None
            for pak in ('root', 'boot', 'gamedata', 'lut_rampak'):
                cand = os.path.join(proot, pak, rel_in_pak)
                if os.path.exists(cand):
                    pc = pcdesc(open(cand, 'rb').read())
                    break
            x = xenos(open(xp, 'rb').read())
            key = (x['fmt'], x['endian'], x['dim'], x['tiled'], x['packed'], x['mipmax'] + 1,
                   pc['fmt'] if pc else '-', pc['levels'] if pc else '-', pc['type'] if pc else '-')
            combos[key] += 1
            samples.setdefault(key, (rel, x, pc))
    print('xfmt end dim tiled packed levels | pcfmt pclevels pctype : count  sample')
    for k, n in combos.most_common():
        rel, x, pc = samples[k]
        print(f'0x{k[0]:02x} {k[1]} {k[2]} {k[3]} {k[4]} {k[5]:2} | {k[6]} {k[7]} {k[8]} : {n:4}  {rel} {x["w"]}x{x["h"]}')
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
