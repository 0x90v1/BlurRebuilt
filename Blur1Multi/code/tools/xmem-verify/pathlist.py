#!/usr/bin/env python3
"""
pathlist.py - write a BLUR1MULTI_PAKDUMP list of every virtual path with one of
the given extensions in an extracted pak tree, and compare a dump against the
matching PC extraction.

  python pathlist.py list <x360 extract root> <out dir> .cycle [.skeleton ...]
  python pathlist.py compare <pc extract root> <out dir>
"""
import hashlib
import os
import sys


def cmd_list(root, out, exts):
    paths = set()
    for pak in os.listdir(root):
        base = os.path.join(root, pak)
        for dp, _, files in os.walk(base):
            for f in files:
                if os.path.splitext(f)[1].lower() in exts:
                    rel = os.path.relpath(os.path.join(dp, f), base).replace(os.sep, '/')
                    paths.add('/' + rel.lower())
    os.makedirs(out, exist_ok=True)
    with open(os.path.join(out, 'list.txt'), 'w', newline='\n') as fh:
        fh.write('\n'.join(sorted(paths)) + '\n')
    print(len(paths), 'paths')
    return 0


def cmd_compare(pcroot, out):
    pc = {}
    for pak in os.listdir(pcroot):
        base = os.path.join(pcroot, pak)
        for dp, _, files in os.walk(base):
            for f in files:
                full = os.path.join(dp, f)
                pc.setdefault('/' + os.path.relpath(full, base).replace(os.sep, '/').lower(), full)
    listed = [l.strip() for l in open(os.path.join(out, 'list.txt')) if l.strip()]
    same = diff = nopc = missing = 0
    for v in listed:
        d = os.path.join(out, 'dump', v.lstrip('/').replace('/', os.sep))
        if not os.path.exists(d):
            missing += 1
            continue
        if v not in pc:
            nopc += 1
            continue
        if open(d, 'rb').read() == open(pc[v], 'rb').read():
            same += 1
        else:
            diff += 1
            if diff <= 8:
                print('DIFFERS', v, os.path.getsize(d), os.path.getsize(pc[v]))
    print(f'listed {len(listed)}: identical {same}, differing {diff}, no PC file {nopc}, not dumped {missing}')
    return 1 if diff or missing else 0


if __name__ == '__main__':
    a = sys.argv
    if len(a) >= 5 and a[1] == 'list':
        sys.exit(cmd_list(a[2], a[3], [e.lower() for e in a[4:]]))
    if len(a) == 4 and a[1] == 'compare':
        sys.exit(cmd_compare(a[2], a[3]))
    print(__doc__)
    sys.exit(2)
