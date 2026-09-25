#!/usr/bin/env python3
"""
modelcmp.py - compare converted .model files (a BLUR1MULTI_PAKDUMP dump) with the
retail PC extraction block by block.

  python modelcmp.py <pc extract root> <dump out dir> [--show N]

For every block (Model/Header, Model/MdlDat, Model/Render, Model/Coll, MtaDbg)
reports how many converted blocks are byte-identical to the PC file's.
"""
import collections
import os
import struct
import sys


def tree(d):
    """(path, offset, size) for every C_FileBlock; 'size' includes the 16-byte header"""
    e = '>' if d[:4] in (b'360 ', b'PS3 ') else '<'
    out = []

    def walk(at, end, path):
        while at + 16 <= end:
            bid = d[at:at + 8].rstrip(b'\0').decode('latin1')
            size, count, typ = struct.unpack(e + 'IHB', d[at + 8:at + 15])
            out.append((path + '/' + bid, at, size))
            if typ == 0x43:
                walk(at + 16, at + size, path + '/' + bid)
            at += max(size, 16)
    start = 4 if d[:4] in (b'360 ', b'PS3 ', b'  CP') else 0
    walk(start, len(d), '')
    return out


def index(root):
    m = {}
    for pak in os.listdir(root):
        base = os.path.join(root, pak)
        for dp, _, fs in os.walk(base):
            for f in fs:
                full = os.path.join(dp, f)
                m.setdefault('/' + os.path.relpath(full, base).replace(os.sep, '/').lower(), full)
    return m


def main(pcroot, out, show):
    pc = index(pcroot)
    listed = [l.strip() for l in open(os.path.join(out, 'list.txt')) if l.strip()]
    same = collections.Counter()
    total = collections.Counter()
    whole = 0
    shown = collections.Counter()
    for v in listed:
        d = os.path.join(out, 'dump', v.lstrip('/').replace('/', os.sep))
        if not os.path.exists(d) or v not in pc:
            continue
        a = open(d, 'rb').read()
        b = open(pc[v], 'rb').read()
        if a[:4] == b'360 ':
            total['(not converted)'] += 1
            continue
        whole += a == b
        ta = {p: (o, s) for p, o, s in tree(a)}
        tb = {p: (o, s) for p, o, s in tree(b)}
        for p in tb:
            total[p] += 1
            if p in ta:
                oa, sa = ta[p]
                ob, sb = tb[p]
                if a[oa:oa + sa] == b[ob:ob + sb]:
                    same[p] += 1
                    continue
                if shown[p] < show:
                    shown[p] += 1
                    blk_a = a[oa:oa + sa]
                    blk_b = b[ob:ob + sb]
                    first = next((i for i in range(min(len(blk_a), len(blk_b))) if blk_a[i] != blk_b[i]), min(len(blk_a), len(blk_b)))
                    print(f'DIFFERS {p} {v}: sizes {sa} / {sb}, first difference at block offset {first}')
    print(f'whole files identical: {whole}')
    for p in sorted(total):
        print(f'{p:<28} identical {same[p]}/{total[p]}')


if __name__ == '__main__':
    a = sys.argv
    show = int(a[a.index('--show') + 1]) if '--show' in a else 3
    main(a[1], a[2], show)
