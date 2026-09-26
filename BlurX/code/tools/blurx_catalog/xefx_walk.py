#!/usr/bin/env python3
"""xefx_walk.py - walk an Xbox 360 FXB2/XEFX effect (version 9 or 12) the way
BlurX's EffectConvert.cpp does, and report the version-12 extras and the
annotation values, so the converter's reading can be checked on whole archives.

    python xefx_walk.py <dir or .fxb> [...]

Field order: Blur1Multi EffectConvert.cpp (version 9, from the engine's readers)
plus the three version 12 additions of Blur2Test _notes/xbox_stage_decode.md
("XEFX v9 decoded: v12 is v9 plus three fields"): a word between each program's
bindings and its microcode, a word between each technique's passes and its
annotations, and a length-prefixed selection script in place of the structs.
"""
import os
import struct
import sys
from collections import Counter


class Walk:
    def __init__(self, b):
        self.b, self.at = b, 0
        self.stats = Counter()
        self.float_annotations = []

    def take(self, n):
        if self.at + n > len(self.b):
            raise ValueError(f'read past the end at {self.at}')
        p = self.b[self.at:self.at + n]
        self.at += n
        return p

    def word(self):
        return struct.unpack('>I', self.take(4))[0]

    def string(self):
        return self.take(self.word())

    def vector(self, f):
        for _ in range(self.word()):
            f()

    def annotation(self):
        self.string()
        t = self.word()
        if t == 1:
            self.take(1)
        elif t in (2, 3, 4):
            raw = self.take(4)
            if t == 3:
                self.float_annotations.append(raw)
        elif t == 5:
            self.take(16)
        elif t == 6:
            self.string()

    def annotations(self):
        self.vector(self.annotation)

    def binding(self):
        for _ in range(6):
            self.word()
        self.string()

    def function(self):
        size = self.word()
        self.vector(self.binding)
        if self.version >= 12:
            self.stats['program_word_nonzero' if self.word() else 'program_word_zero'] += 1
        self.take(size)

    def pass_(self):
        self.string()
        self.take(140)
        self.function()
        self.function()
        self.annotations()

    def technique(self):
        self.string()
        self.vector(self.pass_)
        if self.version >= 12:
            self.stats['technique_word_' + str(self.word())] += 1
        self.annotations()

    def parameter(self):
        self.string()
        self.string()
        self.word()
        self.word()
        self.word()
        self.take(1)
        self.annotations()

    def input_semantic(self):
        self.word()
        self.word()
        self.annotations()

    def member(self):
        self.string()
        self.word()
        self.annotations()

    def struct(self):
        self.string()
        self.vector(self.member)

    def effect(self):
        self.version = self.word()
        self.string()
        self.vector(self.technique)
        self.vector(self.parameter)
        self.vector(self.word)
        self.vector(self.input_semantic)
        if self.version >= 12:
            script = self.string()
            self.stats['script_bytes'] += len(script)
        else:
            for _ in range(self.word()):
                self.struct()
        if self.at != len(self.b):
            raise ValueError(f'{len(self.b) - self.at} bytes left')


def xefx_payload(data):
    """the XEFX block payload of an FXB2 file (block header: 8-byte id, u32 size, u16 count, 2 bytes)"""
    i = data.find(b'XEFX')
    if i < 0:
        raise ValueError('no XEFX block')
    size = struct.unpack('>I', data[i + 8:i + 12])[0]
    return data[i + 16:i + 16 + size - 16] if size > 16 else data[i + 16:]


def main():
    files = []
    for a in sys.argv[1:]:
        if os.path.isdir(a):
            for r, _, fs in os.walk(a):
                files += [os.path.join(r, f) for f in fs if f.lower().endswith('.fxb')]
        else:
            files.append(a)
    total = Counter()
    ok = bad = 0
    floats_be = floats_le = 0
    for f in files:
        w = Walk(xefx_payload(open(f, 'rb').read()))
        try:
            w.effect()
            ok += 1
            total.update(w.stats)
            total['version_%d' % w.version] += 1
            for raw in w.float_annotations:
                be, le = struct.unpack('>f', raw)[0], struct.unpack('<f', raw)[0]
                sane = lambda v: v == 0 or 1e-6 < abs(v) < 1e6
                floats_be += sane(be) and not sane(le)
                floats_le += sane(le) and not sane(be)
        except ValueError as e:
            bad += 1
            if bad <= 5:
                print('FAIL', f, e)
    print(f'{ok} walked to the end, {bad} failed; {dict(total)}')
    print(f'float annotations plausible only as big-endian: {floats_be}, only as little-endian: {floats_le}')


if __name__ == '__main__':
    main()
