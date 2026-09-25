# [Blur1Multi] Checks Xbox 360 secure pak signatures (gamedata.sig, gamedata_boot.sig) against the
# SHA-1 of the FAT as stored on disk and of its byte-swapped copy. Usage: python sigcheck.py <cache dir>...
# docs/analysis.md "XMem LZX on PC", Secure paks.
import hashlib, os, re, struct, sys

HERE = os.path.dirname(os.path.abspath(__file__))
key_src = open(os.path.join(HERE, '..', '..', '..', '..', 'Blur1PC', 'code', 'amax', 'game', 'Game', 'PublicKey',
                            'amax_public_key.h')).read()
key = bytes(int(x, 16) for x in re.findall(r'0x([0-9a-fA-F]{2})', key_src))
# DER: 30 81 89 | 02 81 81 00 <n 128> | 02 03 <e 3>
assert key[0] == 0x30 and key[3] == 0x02
nlen = key[5]
n = int.from_bytes(key[6:6 + nlen], 'big')
p = 6 + nlen
assert key[p] == 0x02
e = int.from_bytes(key[p + 2:p + 2 + key[p + 1]], 'big')
k = (n.bit_length() + 7) // 8


def mgf1(seed, length):
    out = b''
    c = 0
    while len(out) < length:
        out += hashlib.sha1(seed + struct.pack('>I', c)).digest()
        c += 1
    return out[:length]


def pss_verify(sig, mhash, salt_len=8):
    """EMSA-PSS-VERIFY (PKCS#1 v2.1) with SHA-1, as libtomcrypt pkcs_1_pss_decode"""
    m = pow(int.from_bytes(sig, 'big'), e, n)
    embits = n.bit_length() - 1
    emlen = (embits + 7) // 8
    em = m.to_bytes(emlen, 'big')
    if em[-1] != 0xBC:
        return False
    hlen = 20
    masked = em[:emlen - hlen - 1]
    h = em[emlen - hlen - 1:-1]
    db = bytes(a ^ b for a, b in zip(masked, mgf1(h, len(masked))))
    db = bytes([db[0] & (0xFF >> (8 * emlen - embits))]) + db[1:]
    ps_len = emlen - hlen - salt_len - 2
    if any(db[:ps_len]) or db[ps_len] != 0x01:
        return False
    salt = db[ps_len + 1:]
    return hashlib.sha1(b'\0' * 8 + mhash + salt).digest() == h


def fat_bytes(pak):
    f = open(pak, 'rb')
    d = f.read(16 << 20)
    le = d[:4] == b'2KAP'
    u32 = (lambda b: struct.unpack('<I', b)[0]) if le else (lambda b: struct.unpack('>I', b)[0])
    sector = u32(d[8:12])
    fat_type = d[16 + 21]
    pos = 56
    ssz = u32(d[pos:pos + 4])
    pos += 4 + ssz
    pos += sector - (pos % sector)
    count, size = u32(d[pos:pos + 4]), u32(d[pos + 4:pos + 8])
    return d[pos + 8:pos + 8 + size], fat_type, count


def swap_fat(raw, fat_type, count):
    """host-order copy as the PC reader makes it: every field byte-swapped, hash bytes untouched"""
    es = 56 if fat_type == 1 else 32
    out = bytearray()
    for i in range(count):
        e_ = raw[i * es:(i + 1) * es]
        u = struct.unpack('>IIIIQQ', e_[:32])
        out += struct.pack('<IIIIQQ', *u)
        if fat_type == 1:
            out += struct.pack('<I', struct.unpack('>I', e_[32:36])[0]) + e_[36:56]
    return bytes(out)


for cache in sys.argv[1:]:
    for name in ('gamedata', 'gamedata_boot'):
        try:
            raw, ft, count = fat_bytes(f'{cache}/{name}.pak')
        except FileNotFoundError:
            continue
        sig = open(f'{cache}/{name}.sig', 'rb').read()
        print(f'{cache}/{name}: FAT {len(raw)} bytes, type {ft}; '
              f'signature over on-disk FAT: {pss_verify(sig, hashlib.sha1(raw).digest())}; '
              f'over byte-swapped FAT: {pss_verify(sig, hashlib.sha1(swap_fat(raw, ft, count)).digest())}')
