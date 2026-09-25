#!/usr/bin/env python3
"""
fxbcmp.py - compare converted PCFX effects (x360shader_verify --write) with the
retail PC .fxb of the same name, field by field (fx/effect.h, version 9).

  python fxbcmp.py <converted dir> <pc shaders-speed dir> [--by-technique]

--by-technique (for converted PS3 effects, T076, whose technique lists differ from
PC's in 508 of 820 effects): also compares the techniques present on both sides by
name: pass names, render states, and each program's bindings by parameter name
(the parameter set, then name + type + register + count).
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
        return {'bindings': b, 'size': n, 'code': code}

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


def bindings(e, fn, exact):
    names = [p[0] for p in e['parameters']]
    if exact:
        return sorted((names[b[0]], b[2], b[1], b[5]) for b in fn['bindings'])
    return sorted(set((names[b[0]], b[2]) for b in fn['bindings']))


def by_technique(a, b, c, shown, f):
    tb = {t['name']: t for t in b['techniques']}
    for t in a['techniques']:
        if t['name'] not in tb:
            c['tech: only in converted'] += 1
            continue
        u = tb[t['name']]
        c['tech: in both'] += 1
        checks = {'tech: pass names': [p['name'] for p in t['passes']] == [p['name'] for p in u['passes']]}
        if checks['tech: pass names']:
            checks['tech: render states'] = [p['states'] for p in t['passes']] == [p['states'] for p in u['passes']]
            for k in ('vs', 'ps'):
                checks[f'tech: {k} bound parameters'] = all(bindings(a, p[k], False) == bindings(b, q[k], False) for p, q in zip(t['passes'], u['passes']))
                checks[f'tech: {k} bindings exact'] = all(bindings(a, p[k], True) == bindings(b, q[k], True) for p, q in zip(t['passes'], u['passes']))
        for k, v in checks.items():
            c[k] += v
            if not v and shown[k] < 2:
                shown[k] += 1
                print(f'DIFFERS {k}: {f} {t["name"]}')


def main(conv, pcdir, technique_mode=False):
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
        if technique_mode:
            by_technique(a, b, c, shown, f)
    for k in c:
        print(f'{k:<24} {c[k]}')


if __name__ == '__main__':
    main(sys.argv[1], sys.argv[2], '--by-technique' in sys.argv[3:])
