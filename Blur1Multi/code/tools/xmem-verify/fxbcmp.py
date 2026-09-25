#!/usr/bin/env python3
"""
fxbcmp.py - compare converted PCFX effects (x360shader_verify --write) with the
retail PC .fxb of the same name, field by field (fx/effect.h, version 9).

  python fxbcmp.py <converted dir> <pc shaders-speed dir>
"""
import collections
import os
import struct
import sys


class R:
    def __init__(s, d):
        s.d, s.at = d, 0

    def u32(s):
        v = struct.unpack_from('<I', s.d, s.at)[0]
        s.at += 4
        return v

    def raw(s, n):
        v = s.d[s.at:s.at + n]
        s.at += n
        return v

    def string(s):
        return s.raw(s.u32()).decode('latin1')

    def vec(s, f):
        return [f() for _ in range(s.u32())]


def effect(d):
    # 'FXB2' LE, root block (16), PCFX block (16)
    r = R(d)
    r.at = 4 + 16 + 16

    def annotation():
        name = r.string()
        t = r.u32()
        v = {1: lambda: r.raw(1), 2: lambda: r.raw(4), 3: lambda: r.raw(4), 4: lambda: r.raw(4),
             5: lambda: r.raw(16), 6: r.string}.get(t, lambda: None)()
        return (name, t, v)

    def binding():
        return tuple(r.u32() for _ in range(6)) + (r.string(),)

    def function():
        n = r.u32()
        b = r.vec(binding)
        code = r.raw(n)
        return {'bindings': b, 'size': n}

    def pass_():
        return {'name': r.string(), 'states': [r.u32() for _ in range(35)], 'vs': function(), 'ps': function(),
                'annotations': r.vec(annotation)}

    def technique():
        return {'name': r.string(), 'passes': r.vec(pass_), 'annotations': r.vec(annotation)}

    def parameter():
        return (r.string(), r.string(), r.u32(), r.u32(), r.u32(), r.raw(1), tuple(r.vec(annotation)))

    def semantic():
        return (r.u32(), r.u32(), tuple(r.vec(annotation)))

    version = r.u32()
    e = {'version': version, 'name': r.string(), 'techniques': r.vec(technique), 'parameters': r.vec(parameter),
         'data': r.vec(r.u32), 'inputs': r.vec(semantic)}
    structs = r.u32()
    for _ in range(structs):
        r.string()
        r.vec(lambda: (r.string(), r.u32(), r.vec(annotation)))
    e['end'] = r.at == len(d)
    return e


def main(conv, pcdir):
    pc = {f.lower(): os.path.join(pcdir, f) for f in os.listdir(pcdir)}
    c = collections.Counter()
    shown = collections.Counter()
    for f in sorted(os.listdir(conv)):
        if f.lower() not in pc:
            c['no PC effect'] += 1
            continue
        a = effect(open(os.path.join(conv, f), 'rb').read())
        b = effect(open(pc[f.lower()], 'rb').read())
        c['effects'] += 1
        c['parsed to end'] += a['end'] and b['end']
        checks = {
            'name': a['name'] == b['name'],
            'technique names': [t['name'] for t in a['techniques']] == [t['name'] for t in b['techniques']],
            'pass names': [[p['name'] for p in t['passes']] for t in a['techniques']] == [[p['name'] for p in t['passes']] for t in b['techniques']],
            'render states': [[p['states'] for p in t['passes']] for t in a['techniques']] == [[p['states'] for p in t['passes']] for t in b['techniques']],
            'pass annotations': [[p['annotations'] for p in t['passes']] for t in a['techniques']] == [[p['annotations'] for p in t['passes']] for t in b['techniques']],
            'technique annotations': [t['annotations'] for t in a['techniques']] == [t['annotations'] for t in b['techniques']],
            'parameters': a['parameters'] == b['parameters'],
            'parameter data': a['data'] == b['data'],
            'inputs': a['inputs'] == b['inputs'],
        }
        for k, v in checks.items():
            c[k] += v
            if not v and shown[k] < 2:
                shown[k] += 1
                print(f'DIFFERS {k}: {f}')
    for k in c:
        print(f'{k:<24} {c[k]}')


if __name__ == '__main__':
    main(sys.argv[1], sys.argv[2])
