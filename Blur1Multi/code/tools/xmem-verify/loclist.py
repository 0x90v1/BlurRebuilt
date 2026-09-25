#!/usr/bin/env python3
"""
loclist.py - list a little-endian .loc file's languages (XtLocFile.h) with their
string counts, and optionally compare one language's strings with another file.

  python loclist.py <a.loc> [<b.loc>]
"""
import struct
import sys


def read(path):
    d = open(path, 'rb').read()
    tag, version, toc, tocsize = struct.unpack('<4sIII', d[:16])
    lang_off, lang_count, lookup = struct.unpack('<III', d[toc:toc + 12])
    langs = {}
    for i in range(lang_count):
        o = toc + lang_off + i * 24
        name = d[o:o + 16].split(b'\0')[0].decode('latin-1')
        off, size = struct.unpack('<II', d[o + 16:o + 24])
        langs[name] = table(d, off, True)
    langs['(lookup)'] = table(d, toc + lookup, False)
    return langs


def table(d, at, utf16):
    count, entries = struct.unpack('<II', d[at:at + 8])
    out = {}
    for i in range(count):
        sid, soff = struct.unpack('<II', d[at + entries + i * 8:at + entries + i * 8 + 8])
        s = at + soff
        if utf16:
            e = s
            while d[e:e + 2] != b'\0\0':
                e += 2
            out[sid] = d[s:e].decode('utf-16-le', 'replace')
        else:
            out[sid] = d[s:d.index(b'\0', s)].decode('latin-1')
    return out


def main(a):
    x = read(a[1])
    y = read(a[2]) if len(a) > 2 else None
    for name, strings in x.items():
        line = f'  {name:12} {len(strings):5} strings'
        if y is not None:
            if name not in y:
                line += '  (not in the other file)'
            else:
                same = sum(1 for k, v in strings.items() if y[name].get(k) == v)
                line += f'  other {len(y[name]):5}, identical {same}'
        print(line)
    if y is not None:
        for name in y:
            if name not in x:
                print(f'  {name:12} only in the other file ({len(y[name])} strings)')
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
