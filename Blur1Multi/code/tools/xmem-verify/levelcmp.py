#!/usr/bin/env python3
"""
levelcmp.py - compare converted level files (a BLUR1MULTI_PAKDUMP dump of
/levels/<track>/level.{level,levelphysics,levelrender}) with the retail PC files,
block by block.

  python levelcmp.py <pc levels root> <dump out dir>
"""
import collections
import os
import struct
import sys

# blocks whose payload is child blocks although their type byte says data
CONTAINERS = {'Level', 'Physic', 'Render', 'Routes', 'LS', 'MdlDat'}


def tree(d):
    e = '>' if d[:4] in (b'360 ', b'PS3 ') else '<'
    out = []

    def walk(at, end, path):
        while at + 16 <= end:
            bid = d[at:at + 8].rstrip(b'\0').decode('latin1')
            size, count, typ = struct.unpack(e + 'IHB', d[at + 8:at + 15])
            if size < 16 or at + size > end:
                return
            out.append((path + '/' + bid, at, size))
            if typ == 0x43 or bid in CONTAINERS:
                walk(at + 16, at + size, path + '/' + bid)
            at += size
    walk(4 if d[:4] in (b'360 ', b'PS3 ', b'  CP') else 0, len(d), '')
    return out


def main(pcroot, out):
    listed = [l.strip() for l in open(os.path.join(out, 'list.txt')) if l.strip()]
    same = collections.Counter()
    total = collections.Counter()
    for v in listed:
        d = os.path.join(out, 'dump', v.lstrip('/').replace('/', os.sep))
        p = os.path.join(pcroot, *v.split('/')[2:])
        if not os.path.exists(d) or not os.path.exists(p):
            print('missing', v)
            continue
        a = open(d, 'rb').read()
        b = open(p, 'rb').read()
        if a[:4] == b'360 ':
            print('not converted', v)
            continue
        ta = {q: (o, s) for q, o, s in tree(a)}
        tb = {q: (o, s) for q, o, s in tree(b)}
        ext = v.rsplit('.', 1)[1]
        for q in tb:
            key = ext + q
            total[key] += 1
            if q in ta and a[ta[q][0]:ta[q][0] + ta[q][1]] == b[tb[q][0]:tb[q][0] + tb[q][1]]:
                same[key] += 1
            elif q in ta:
                x = a[ta[q][0]:ta[q][0] + ta[q][1]]
                y = b[tb[q][0]:tb[q][0] + tb[q][1]]
                first = next((i for i in range(min(len(x), len(y))) if x[i] != y[i]), min(len(x), len(y)))
                print(f'DIFFERS {v} {q}: sizes {len(x)} / {len(y)}, first difference at {first}')
    for k in sorted(total):
        print(f'{k:<40} identical {same[k]}/{total[k]}')


if __name__ == '__main__':
    main(sys.argv[1], sys.argv[2])
