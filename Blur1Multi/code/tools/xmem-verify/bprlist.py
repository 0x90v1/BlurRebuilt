#!/usr/bin/env python3
"""
bprlist.py - list the resources of a BZR2 resource bundle (.bpr), either byte
order: name, type, flags, data offset and size, format, mips, dimensions.

  python bprlist.py <file.bpr> [--limit N]

Layout: display/bundler/BundleHeader.h, ResourceHeader.h, ResourceTexture*.h.
"""
import struct
import sys


def read(path):
    d = open(path, 'rb').read()
    be = d[:4] == b'BZR2'
    e = '>' if be else '<'
    version, platform = d[4], d[5]
    info, strtab, flags = struct.unpack(e + 'HHH', d[6:12])
    unc, comp = struct.unpack(e + 'II', d[12:20])
    sysmem = d[20:20 + info]
    res = []
    off = 0
    while True:
        nxt, soff, doff, typ, rflags, runc, rcomp = struct.unpack(e + 'HHIHHII', sysmem[off:off + 20])
        fmt, mips, a, b = struct.unpack(e + 'HHHH', sysmem[off + 20:off + 28])
        name = sysmem[strtab + soff:sysmem.index(b'\0', strtab + soff)].decode('latin-1')
        res.append(dict(name=name, type=typ, flags=rflags, offset=doff, size=runc, csize=rcomp, format=fmt, mips=mips, w=a, h=b))
        if not nxt:
            break
        off += nxt
    return dict(be=be, version=version, platform=platform, info=info, strtab=strtab, flags=flags, unc=unc, comp=comp,
                data=d[20 + info:], resources=res)


def main(a):
    b = read(a[1])
    limit = int(a[a.index('--limit') + 1]) if '--limit' in a else 20
    print(f"{'BE' if b['be'] else 'LE'} v{b['version']} platform {b['platform']} info {b['info']} strtab {b['strtab']} "
          f"flags {b['flags']} data {b['unc']} (compressed {b['comp']}), {len(b['resources'])} resources")
    for r in b['resources'][:limit]:
        print(f"  {r['name']:50} t{r['type']} f0x{r['flags']:04x} @0x{r['offset']:08x} size 0x{r['size']:x} fmt {r['format']} "
              f"mips {r['mips']} {r['w']}x{r['h']}")
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
