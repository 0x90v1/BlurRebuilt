"""2KAP (Bizarre pak) directory reader for the BlurX asset catalog.

Ported from Blur2Test `_tools/b2_list_pak.py` (header, name tables, record
tables, v1 and v2/v3) and `_tools/b2_generate_pak_bms.py` (payload container
framing), which established every field used here across all five Xbox 360
discs (Blur2Test `_notes/disc_resource_map.md`). Diagnostics were dropped;
the validations that make a wrong stride or a damaged table fail loudly were
kept. Nothing here decompresses: the framing walk reads only the container's
size fields, which is what tells a whole payload from a truncated one.

Record words (all versions): w0 identity = ~crc32 of the record's full virtual
path (lower case, backslashes, leading backslash; the engine's pak lookup hash),
w1 name offset, w3 decompressed size (0 = directory), (w6 << 32 | w7) * 0x800
physical start; w9 stored container size on v2/v3.
"""
from __future__ import annotations

import struct
import zlib
from dataclasses import dataclass

MAGIC = 0x50414B32
XOR_KEY = bytes.fromhex("78 56 58 6f 34 30 6a 33 40 24 25 5c 25 60")
SECTOR = 0x800
CHUNK_CAP = 0x10000

# descriptor byte 0: the archive's compression module
SELECTOR_NAMES = {1: "raw", 3: "xmem", 5: "xmem-packet"}


def path_hash(path: str) -> int:
    """The engine's pak lookup hash (C_PakCrc32 over the localised path): CRC-32,
    init 0xFFFFFFFF, no final XOR = ~zlib.crc32, over the lower-case path with
    backslash separators and a leading backslash."""
    p = path.replace("/", "\\").lower()
    if not p.startswith("\\"):
        p = "\\" + p
    if len(p) > 1 and p.endswith("\\"):
        p = p[:-1]
    return (~zlib.crc32(p.encode("latin-1"))) & 0xFFFFFFFF


@dataclass(frozen=True)
class Header:
    endian: str
    version: int
    first_table_offset: int
    descriptor: tuple


@dataclass(frozen=True)
class Record:
    index: int
    words: tuple
    name: str

    @property
    def is_dir(self) -> bool:
        return self.words[3] == 0

    @property
    def size(self) -> int:
        return self.words[3]

    @property
    def start(self) -> int:
        return ((self.words[6] << 32) | self.words[7]) * SECTOR


def u32(data, off, endian):
    return struct.unpack_from(endian + "I", data, off)[0]


def read_header(data) -> Header:
    if data[:4] == b"2KAP":
        endian = "<"
    elif data[:4] == b"PAK2":
        endian = ">"
    else:
        raise ValueError(f"bad 2KAP magic {bytes(data[:4])!r}")
    version = u32(data, 4, endian)
    first = u32(data, 8, endian)
    if version == 1:
        d = tuple(data[0x24:0x28]) + (u32(data, 0x28, endian), u32(data, 0x2C, endian),
                                     u32(data, 0x30, endian), u32(data, 0x34, endian), 0, 0)
    else:
        d = tuple(data[0x10:0x14]) + tuple(u32(data, o, endian) for o in (0x14, 0x18, 0x1C, 0x20, 0x24, 0x28))
    return Header(endian, version, first, d)


def _xor(buf):
    out = bytearray(buf)
    k = 0
    for i in range(len(out)):
        k = (k + 1) % len(XOR_KEY)
        out[i] ^= XOR_KEY[k]
    return bytes(out)


def _xor_v1(data, start, size):
    # v1 name span: the key phase restarts at the first sector boundary
    out = bytearray(size)
    for i, a in enumerate(range(start, start + size)):
        phase = a - (start if a < SECTOR else SECTOR)
        out[i] = data[a] ^ XOR_KEY[(phase + 1) % len(XOR_KEY)]
    return bytes(out)


def _strings_by_offset(names: bytes) -> dict:
    out, cur = {}, 0
    while cur < len(names):
        end = names.find(b"\0", cur)
        if end < 0:
            break
        if end > cur:
            out[cur] = names[cur:end].decode("latin-1")
        cur = end + 1
    return out


def _records_v1(data, h: Header):
    name_size = u32(data, 0x38, h.endian)
    names = _xor_v1(data, 0x3C, name_size)
    offsets = _strings_by_offset(names)
    rec_abs = (0x3C + name_size + SECTOR - 1) & ~(SECTOR - 1)
    count = u32(data, rec_abs, h.endian)
    byte_size = u32(data, rec_abs + 4, h.endian)
    # the stride is declared (0x20 root/boot/shaders, 0x38 gamedata on Aug 31 / retail)
    if count == 0 or byte_size % count:
        raise ValueError(f"v1 record bytes 0x{byte_size:x} not a multiple of {count}")
    stride = byte_size // count
    if count not in (len(offsets), len(offsets) - 1):   # Aug 31 models.pak opens with a build banner
        raise ValueError(f"v1 record count {count} != name count {len(offsets)}")
    recs, seen = [], set()
    for i in range(count):
        off = rec_abs + 8 + i * stride
        w = tuple(u32(data, off + 4 * j, h.endian) for j in range(stride // 4))
        if w[1] not in offsets or w[1] in seen:
            raise ValueError(f"v1 record {i}: name offset 0x{w[1]:x} is not a unique string boundary")
        seen.add(w[1])
        recs.append(Record(i, w, offsets[w[1]]))
    return recs


def _records_v2(data, h: Header):
    base = h.first_table_offset
    block = u32(data, base, h.endian)
    names = _xor(bytes(data[base + 4: base + 4 + block]))
    rec_off = base + 4 + block
    rec_off += (-(block + 4)) % SECTOR
    count = u32(data, rec_off, h.endian)
    byte_size = u32(data, rec_off + 4, h.endian)
    stride = {0: 0x28, 1: 0x40}.get(h.descriptor[1], 0)
    if not stride or byte_size != count * stride:
        raise ValueError(f"v2/v3 record table: flag {h.descriptor[1]} count {count} bytes 0x{byte_size:x}")
    recs = []
    for i in range(count):
        off = rec_off + 8 + i * stride
        w = tuple(u32(data, off + 4 * j, h.endian) for j in range(stride // 4))
        end = names.find(b"\0", w[1])
        recs.append(Record(i, w, names[w[1]:end if end >= 0 else len(names)].decode("latin-1")))
    return recs


def read_records(data, h: Header):
    return _records_v1(data, h) if h.version == 1 else _records_v2(data, h)


# ---- payload container framing (no decompression) ---------------------------

def _be32(data, off):
    if off + 4 > len(data):
        raise ValueError("u32 beyond the archive")
    return struct.unpack_from(">I", data, off)[0]


def _xmem_out(data, pos, end):
    """walk one XMem/LZX frame chain in data[pos:end]; return its declared output size"""
    total = 0
    while pos < end:
        if bytes(data[pos:pos + 5]) == b"\0\0\0\0\0":
            if pos + 5 != end:
                raise ValueError("bytes after the XMem terminator")
            return total
        if data[pos] == 0xFF:
            out, comp = struct.unpack_from(">HH", data, pos + 1)
            pos += 5
            if out == 0 and comp == 0:
                if pos != end:
                    raise ValueError("bytes after the XMem terminator")
                return total
        else:
            comp = struct.unpack_from(">H", data, pos)[0]
            out = 0x8000
            pos += 2
        if out == 0 or comp == 0 or pos + comp > end:
            raise ValueError("bad XMem frame")
        pos += comp
        total += out
    raise ValueError("XMem chain has no terminator")


def _packet_chunks(data, cur, stored_end, left):
    first = True
    while left:
        if first:
            if _be32(data, cur) != CHUNK_CAP:
                raise ValueError("first packet chunk cap is not 0x10000")
            code, payload = _be32(data, cur + 4), cur + 8
            first = False
        else:
            code, payload = _be32(data, cur), cur + 4
        if code == 0xFFFFFFFF:
            n = min(CHUNK_CAP, left)
            out = n
        else:
            n = code
            if payload + n > stored_end:
                raise ValueError("packet chunk beyond the stored span")
            out = _xmem_out(data, payload, payload + n)
        if payload + n > stored_end or out > left:
            raise ValueError("packet chunk overruns")
        left -= out
        cur = payload + n
    return cur


def check_payload(data, h: Header, r: Record) -> str:
    """'ok', or why the record's payload container cannot be whole. Checks the
    stored span lies inside the file and that the container's size fields walk
    exactly to its end with the declared output size."""
    sel = h.descriptor[0]
    start = r.start
    try:
        if start >= len(data):
            return "bad:start-beyond-file"
        if h.version == 1:
            stored = _be32(data, start)
            cur, end = start + 4, start + 4 + stored
        else:
            stored = r.words[9]
            cur, end = start, start + stored
        if end > len(data):
            return "bad:span-beyond-file"
        if sel == 1:
            return "ok" if stored == r.size else "bad:raw-size"
        if sel == 3:
            return "ok" if _xmem_out(data, cur, end) == r.size else "bad:xmem-size"
        if sel == 5:
            cur = _packet_chunks(data, cur, end, r.size)
            if h.version == 1 and cur + 4 == end and _be32(data, cur) == 0:
                cur += 4    # one retail member carries an explicit zero terminator
            return "ok" if cur == end else "bad:packet-span"
        return f"unchecked:selector-{sel}"
    except (ValueError, struct.error) as e:
        return "bad:" + str(e).replace(" ", "-")[:40]
