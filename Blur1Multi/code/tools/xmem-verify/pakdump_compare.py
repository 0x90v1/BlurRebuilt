#!/usr/bin/env python3
"""
pakdump_compare.py - check what the engine reads from an Xbox 360 source
against Blur2Test's extraction (T071).

  python pakdump_compare.py list <reference disc dir> <out dir> [--per-archive N]
      writes <out dir>/list.txt: virtual paths the Xenon mount sequence serves
      (CreateFsLayers: root.pak and root_streaming.pak at /, shaders-speed.pak at
      /graphics/shaders/speed/, gamedata.pak at /gamedata/, the gamedata_boot RAM
      pak at /), sampled from the reference tree <disc>/<archive>/<pak path>.
      Every file of the small archives, N from root (largest first, then spread).
  python pakdump_compare.py compare <reference disc dir> <out dir>
      compares <out dir>/dump/<path> (written by the game with
      BLUR1MULTI_PAKDUMP=<out dir>/list.txt) with every reference archive that
      carries that virtual path. Exit 1 on any mismatch or missing file.

Reference disc dir: C:/AIProjects/Blur2Test/_work/disc_cache/<disc>.
"""
import hashlib
import os
import sys

# archive -> (mount destination, pak source prefix), from CreateFsLayers (Xenon branch)
MOUNTS = {
    'root': ('/', ''),
    'root_streaming': ('/', ''),
    'shaders-speed': ('/graphics/shaders/speed/', ''),
    'gamedata': ('/gamedata/', 'gamedata/'),
    'gamedata_boot': ('/', ''),
}


def ref_files(disc, archive):
    base = os.path.join(disc, archive)
    out = []
    for root, _, files in os.walk(base):
        for f in files:
            full = os.path.join(root, f)
            rel = os.path.relpath(full, base).replace('\\', '/')
            out.append((rel, full))
    return out


def virtual(archive, rel):
    dst, src = MOUNTS[archive]
    if src and not rel.lower().startswith(src):
        return None
    return dst + rel[len(src):]


def cmd_list(disc, out, per):
    paths = []
    for archive in MOUNTS:
        files = ref_files(disc, archive)
        if not files:
            continue
        if archive == 'root' and len(files) > per:
            files.sort(key=lambda x: -os.path.getsize(x[1]))
            pick = files[:per // 4] + files[per // 4::max(1, len(files) // (per - per // 4))]
            files = pick[:per]
        n = 0
        for rel, _ in files:
            v = virtual(archive, rel)
            if v:
                paths.append(v)
                n += 1
        print(f'{archive}: {n} paths')
    os.makedirs(out, exist_ok=True)
    seen = set()
    with open(os.path.join(out, 'list.txt'), 'w', newline='\n') as fh:
        for p in paths:
            if p.lower() not in seen:
                seen.add(p.lower())
                fh.write(p + '\n')
    print(f'{len(seen)} distinct virtual paths -> {os.path.join(out, "list.txt")}')
    return 0


def sha(p):
    return hashlib.sha1(open(p, 'rb').read()).hexdigest()


def cmd_compare(disc, out):
    refs = {}
    for archive in MOUNTS:
        for rel, full in ref_files(disc, archive):
            v = virtual(archive, rel)
            if v:
                refs.setdefault(v.lower(), []).append((archive, full))
    listed = [l.strip() for l in open(os.path.join(out, 'list.txt')) if l.strip()]
    ok = bad = missing = 0
    total = 0
    for v in listed:
        d = os.path.join(out, 'dump', v.lstrip('/').replace('/', os.sep))
        if not os.path.exists(d):
            missing += 1
            if missing <= 10:
                print('missing', v)
            continue
        h = sha(d)
        cands = refs.get(v.lower(), [])
        if any(sha(full) == h for _, full in cands):
            ok += 1
            total += os.path.getsize(d)
        else:
            bad += 1
            if bad <= 10:
                print('MISMATCH', v, os.path.getsize(d), 'bytes; reference in', [a for a, _ in cands])
    print(f'listed {len(listed)}: identical {ok} ({total} bytes), mismatched {bad}, missing {missing}')
    return 1 if (bad or missing) else 0


def main(a):
    if len(a) >= 4 and a[1] == 'list':
        per = int(a[a.index('--per-archive') + 1]) if '--per-archive' in a else 400
        return cmd_list(a[2], a[3], per)
    if len(a) == 4 and a[1] == 'compare':
        return cmd_compare(a[2], a[3])
    print(__doc__)
    return 2


if __name__ == '__main__':
    sys.exit(main(sys.argv))
