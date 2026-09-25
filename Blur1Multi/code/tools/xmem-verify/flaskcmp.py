#!/usr/bin/env python3
"""
flaskcmp.py - compare two little-endian XT flasks by meaning: schema tables with
names resolved through each file's own string pool, then the ref, record,
component and data blobs. Tells a string-pool reordering from a content change.

  python flaskcmp.py <a.bin> <b.bin>
"""
import struct
import sys


def decode_strings(src):
    """XtFlaskDecodeStrings (common/xt2/source/common/XtFlask.cpp)"""
    out = bytearray(len(src))
    prev = 0xCC
    for i in range(len(src) - 1):
        out[i] = (prev - (src[i] ^ 0xB3)) & 0xFF
        prev = (prev + out[i] - i) & 0xFF
    return bytes(out)


def load(path):
    d = open(path, 'rb').read()
    h = struct.unpack('<18I', d[:72])
    pool = decode_strings(d[h[8]:h[8] + h[9]])

    def s(off):
        return pool[off:pool.index(b'\0', off)].decode('latin-1') if off < len(pool) else f'<{off}>'
    types = [struct.unpack('<6H', d[h[2] + i * 12:h[2] + i * 12 + 12]) for i in range(h[3])]
    bases = [struct.unpack('<2H', d[h[4] + i * 4:h[4] + i * 4 + 4]) for i in range(h[5])]
    fields = [struct.unpack('<3H2B', d[h[6] + i * 8:h[6] + i * 8 + 8]) for i in range(h[7])]
    return dict(
        types=[(s(t[0]),) + t[1:] for t in types],
        bases=bases,
        fields=[(s(f[0]),) + f[1:] for f in fields],
        refs=d[h[10]:h[10] + h[11] * 8],
        records=d[h[12]:h[12] + h[13] * 16],
        components=d[h[14]:h[14] + h[15] * 6],
        data=d[h[16]:h[16] + h[17]])


def main(a):
    x, y = load(a[1]), load(a[2])
    same = True
    for k in ('types', 'bases', 'fields', 'refs', 'records', 'components', 'data'):
        eq = x[k] == y[k]
        same &= eq
        detail = ''
        if not eq and isinstance(x[k], bytes):
            n = next((i for i in range(min(len(x[k]), len(y[k]))) if x[k][i] != y[k][i]), min(len(x[k]), len(y[k])))
            detail = f' ({len(x[k])} vs {len(y[k])} bytes, first difference at +0x{n:x})'
        elif not eq:
            n = next((i for i in range(min(len(x[k]), len(y[k]))) if x[k][i] != y[k][i]), None)
            detail = f' ({len(x[k])} vs {len(y[k])} entries, first difference at #{n}: {x[k][n] if n is not None else ""} vs {y[k][n] if n is not None else ""})'
        print(f'  {k:10} {"same" if eq else "DIFFERENT"}{detail}')
    return 0 if same else 1


if __name__ == '__main__':
    sys.exit(main(sys.argv))
