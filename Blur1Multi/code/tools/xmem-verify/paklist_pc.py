#!/usr/bin/env python3
"""
paklist_pc.py - list a retail PC pak (version 2, little-endian) or a PS3 pak
(version 1, big-endian, same layout in its own byte order): one line per
file with its logical size, first 16 bytes and path, the same format as
`xmem_verify <pak> --list` prints for Xbox 360 paks, so the two can be joined.

  python paklist_pc.py <file.pak> [--extract <virtual path> <out file>] [--extract-all <dir>]

Layout (docs/analysis.md "Pak version 2"): pre-header PAK2/version/sector,
U32 header size 0x20, packed 32-byte header, then at sector 1 the XOR-obfuscated
string table and the 40-byte (+24 secure) FAT entries. Payloads have no size
prefix; the stored size is in the FAT. Version 1 (retail PC shaders-speed.pak) has
a U32 stored-size prefix per payload. Codecs: 1 raw, 2 raw-deflate stream,
4 packet ([u32 packet size][chunks: u32 compressed, raw deflate; 0xFFFFFFFF raw]).
"""
import struct
import sys
import zlib

E = '<'  # byte order of the pak being read: '>' for PS3 (and Xbox 360)
CIPHER = b"xVXo40j3@$%\\%`"


def decrypt(b):
    out = bytearray(b)
    idx = 0
    for i in range(len(out)):
        idx = (idx + 1) % 14
        out[i] ^= CIPHER[idx]
    return bytes(out)


def read_pak(path):
    f = open(path, 'rb')
    head = f.read(12 + 4 + 32)
    global E
    E = '>' if head[:4] == b'PAK2' else '<'
    magic, version, sector = struct.unpack(E + '4sII', head[:12])
    if magic not in (b'2KAP', b'PAK2') or version not in (1, 2):
        raise SystemExit(f'{path}: not a v1/v2 pak ({magic}, v{version})')
    if version == 2:
        codec, fat_type = head[16], head[17]
        strpos = sector
    else:
        # v1: U32 header size 0x28, C_PakHeader {sha1[20], codec, fatType, ...}, string table right after
        codec, fat_type = head[16 + 20], head[16 + 21]
        strpos = 16 + 40
    f.seek(strpos)
    ssz = struct.unpack(E + 'I', f.read(4))[0]
    strings = decrypt(f.read(ssz))
    pos = strpos + 4 + ssz
    pos += sector - (pos % sector)
    f.seek(pos)
    count, fat_size = struct.unpack(E + 'II', f.read(8))
    es = (40 + (24 if fat_type == 1 else 0)) if version == 2 else (56 if fat_type == 1 else 32)
    fat = f.read(fat_size)
    entries = []
    for i in range(count):
        e = fat[i * es:(i + 1) * es]
        if version == 2:
            crc, soff, parent, logical, ftime, start, unk, physical = struct.unpack(E + 'IIIIQQII', e[:40])
            prefix = 0
        else:
            crc, soff, parent, logical, ftime, start = struct.unpack(E + 'IIIIQQ', e[:32])
            physical, prefix = None, 4
        if start == 0xFFFFFFFFFFFFFFFF:
            continue
        name = strings[soff:strings.index(b'\0', soff)].decode('latin-1')
        at = start * sector
        if physical is None:          # v1: [U32 stored size][payload]
            f.seek(at)
            physical = struct.unpack(E + 'I', f.read(4))[0]
        entries.append((name, logical, at + prefix, physical))
    return f, codec, entries


def payload(f, codec, logical, at, physical, limit=None):
    f.seek(at)
    raw = f.read(physical)
    want = logical if limit is None else min(logical, limit)
    if codec == 1:
        return raw[:want]
    if codec == 2:
        return zlib.decompressobj().decompress(raw, want)[:want]      # inflateInit: zlib header
    if codec == 4:
        packet = struct.unpack(E + 'I', raw[:4])[0]
        q, out = 4, bytearray()
        while len(out) < want and q + 4 <= len(raw):
            c = struct.unpack(E + 'I', raw[q:q + 4])[0]
            q += 4
            n = min(packet, logical - len(out))
            if c == 0xFFFFFFFF:
                out += raw[q:q + n]
                q += n
            else:
                out += zlib.decompress(raw[q:q + c], -15)
                q += c
        return bytes(out[:want])
    raise SystemExit(f'codec {codec} not handled')


def main(a):
    f, codec, entries = read_pak(a[1])
    if '--extract-all' in a:
        import os
        root = a[a.index('--extract-all') + 1]
        for n, logical, at, physical in entries:
            out = os.path.join(root, n.replace('\\', '/').lstrip('/'))
            os.makedirs(os.path.dirname(out), exist_ok=True)
            open(out, 'wb').write(payload(f, codec, logical, at, physical))
        return 0
    if '--extract' in a:
        name, out = a[a.index('--extract') + 1], a[a.index('--extract') + 2]
        for n, logical, at, physical in entries:
            if n.lower() == name.lower():
                open(out, 'wb').write(payload(f, codec, logical, at, physical))
                return 0
        print('not found:', name)
        return 1
    for name, logical, at, physical in entries:
        head = payload(f, codec, logical, at, physical, 16)
        print(logical, head.hex() + '--' * (16 - len(head)), name)
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
