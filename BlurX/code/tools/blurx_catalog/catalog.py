#!/usr/bin/env python3
"""catalog.py - the BlurX asset catalog: every asset on every disc, built off line.

The BlurX game asks the catalog, per file it opens, which disc serves it
(AGENTS.md "Asset catalog", docs/decisions.md D12). The catalog's contents are
maintained here, off line: this tool reads the discs listed in
BlurX/catalog/discs.json and writes BlurX/catalog/catalog.tsv, which the game
loads at boot. The choice of disc is made by the game when a file is opened,
from the catalog and a profile (BlurX/catalog/profiles.txt); `resolve` and
`report` here run the same policy so it can be inspected without the game.

    python catalog.py build  [--scan] [--only nov10,...]   # read the discs, write catalog.tsv
    python catalog.py stats                                 # counts per disc / container / status
    python catalog.py which <path>                          # every disc's copy of one virtual path
    python catalog.py resolve <profile> <path> ...          # the disc a profile serves each path from
    python catalog.py report <profile>                      # how many paths each disc serves
    (resolve/report take --overrides <file>; default BlurX/catalog/profiles.local.txt when present)

What is read: each pak's name and record tables (about 0.03% of its bytes) and
each payload container's size fields, which prove whether the stored payload
is whole (pak2.check_payload); loose files by directory walk. Nothing is
decompressed and nothing is written outside BlurX/catalog. `--scan` also reads
every loose file to count zero-filled, 32 KB-aligned blocks: on a disc marked
damaged (Nov 25) a loose file with any is marked suspect.

Keys. Every row is keyed by the engine's pak lookup hash of its virtual path
(pak2.path_hash; the virtual path is where the engine sees the file once the
archive is mounted, discs.json "mounts"). For an archive mounted where its
own paths live (dst == src) that is the record's identity word w0, so records
whose table keeps only a leaf name (the v3 root.pak of the Nov discs) are keyed
exactly; their full path is filled in from any other disc whose record has the
same w0, else the row keeps the leaf name and marks it with a leading '?'.

catalog.tsv format (tab separated; lines starting '#' are comments):
    @disc   <id> <generation> <game> <damaged 0|1> <root> <cache dir> <title> <layers, comma separated, or ->
    @pak    <disc> <archive> <version> <selector> <mount> <src> <records> <status>
            (per disc in the order the engine mounts them; later mounts win)
    <hash>  <disc> <container> <record> <size> <status> <path>
container is the archive file name or 'loose'; record is the pak record index
('-' for a loose file), so every answer names its disc, archive and record;
size is the decompressed size;
status is ok, suspect:<why> or bad:<why>.
"""
from __future__ import annotations

import argparse
import fnmatch
import json
import mmap
import os
import sys
import time
from collections import Counter, defaultdict

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import pak2  # noqa: E402

HERE = os.path.dirname(os.path.abspath(__file__))
BLURX = os.path.normpath(os.path.join(HERE, '..', '..', '..'))
CATDIR = os.path.join(BLURX, 'catalog')
DISCS = os.path.join(CATDIR, 'discs.json')
CATALOG = os.path.join(CATDIR, 'catalog.tsv')
PROFILES = os.path.join(CATDIR, 'profiles.txt')
OVERRIDES = os.path.join(CATDIR, 'profiles.local.txt')   # optional, not tracked

SKIP_LOOSE_EXT = {'.xex', '.pak', '.sig'}
ZERO_BLOCK = 0x8000


def vpath(mount, src, name):
    """the engine-visible path of a pak record named 'name' in an archive mounted at mount from src"""
    n = name.replace('/', '\\')
    if not n.startswith('\\'):
        return None                     # leaf name only: the path is unknown
    s = src.replace('/', '\\').rstrip('\\')
    if s and not n.lower().startswith(s.lower() + '\\'):
        return None
    rest = n[len(s):]
    return mount.replace('/', '\\').rstrip('\\') + rest


def zero_blocks(path):
    n = 0
    z = bytes(ZERO_BLOCK)
    with open(path, 'rb') as f:
        while True:
            b = f.read(ZERO_BLOCK)
            if len(b) < ZERO_BLOCK:
                return n
            if b == z:
                n += 1


def build(args):
    cfg = json.load(open(DISCS, encoding='utf-8'))
    mounts = {m[0].lower(): (m[1], m[2]) for m in cfg['mounts']}
    only = set(args.only.split(',')) if args.only else None
    discs, paks, rows = [], [], []
    by_w0 = {}                          # identity word -> a full path seen on any disc
    t0 = time.time()
    for d in cfg['discs']:
        if only and d['id'] not in only:
            continue
        root = d['root']
        if not os.path.isdir(root):
            print(f"{d['id']}: {root} not found; skipped", file=sys.stderr)
            continue
        discs.append(d)
        cache = os.path.join(root, d['cache'])
        # the engine's mount order (discs.json "mounts"), then any archive it does not mount
        mount_order = [m[0].lower() for m in cfg['mounts']]
        arcs = sorted(os.listdir(cache), key=lambda n: (mount_order.index(n.lower()) if n.lower() in mount_order
                                                        else len(mount_order), n.lower()))
        for arc in arcs:
            if not arc.lower().endswith('.pak'):
                continue
            mnt = mounts.get(arc.lower())
            if mnt is None:
                paks.append((d['id'], arc, '', '', '', '', 0, 'unmounted'))
                continue
            with open(os.path.join(cache, arc), 'rb') as fh:
                size = os.fstat(fh.fileno()).st_size
                if size == 0:
                    continue
                with mmap.mmap(fh.fileno(), 0, access=mmap.ACCESS_READ) as data:
                    try:
                        h = pak2.read_header(data)
                        recs = pak2.read_records(data, h)
                    except Exception as e:  # noqa: BLE001 - reported per archive
                        paks.append((d['id'], arc, '', '', mnt[0], mnt[1], 0, 'bad:' + str(e)[:60]))
                        continue
                    files = [r for r in recs if not r.is_dir]
                    paks.append((d['id'], arc, h.version, h.descriptor[0], mnt[0], mnt[1], len(files), 'ok'))
                    for r in files:
                        vp = vpath(mnt[0], mnt[1], r.name)
                        if mnt[0] == mnt[1]:
                            key = r.words[0]
                        elif vp is not None:
                            key = pak2.path_hash(vp)
                        else:
                            key = None
                        if vp is not None and mnt[0] == mnt[1]:
                            by_w0.setdefault(r.words[0], vp)
                        status = pak2.check_payload(data, h, r)
                        rows.append([key, d['id'], arc, r.size, status, vp, r.name, r.index])
            print(f"{d['id']}/{arc}: v{h.version} sel {h.descriptor[0]}, {len(files)} files "
                  f"({time.time() - t0:.0f} s)", flush=True)
        # loose files: everything under the disc root except the paks' folder and executables
        # (a layer folder, discs.json "layers", is overlaid at / as media layer 1: its files
        # have the same virtual paths as the root's; its own cache folder is not read)
        nloose = 0
        layers = [os.path.normpath(os.path.join(root, l)) for l in d.get('layers', [])]
        caches = {os.path.normcase(os.path.normpath(p)) for p in [cache] + [os.path.join(l, 'cache') for l in layers]}
        for dirpath, dirnames, filenames in os.walk(root):
            if os.path.normcase(os.path.normpath(dirpath)) in caches:
                dirnames[:] = []
                continue
            here = os.path.normcase(os.path.normpath(dirpath)) + os.sep
            base = next((l for l in layers if here.startswith(os.path.normcase(l) + os.sep)), root)
            for f in filenames:
                if os.path.splitext(f)[1].lower() in SKIP_LOOSE_EXT:
                    continue
                full = os.path.join(dirpath, f)
                vp = '\\' + os.path.relpath(full, base).replace('/', '\\')
                status = 'ok'
                if args.scan and d.get('damaged'):
                    z = zero_blocks(full)
                    if z:
                        status = f'suspect:{z}-zero-blocks'
                rows.append([pak2.path_hash(vp), d['id'], 'loose', os.path.getsize(full), status, vp, f, '-'])
                nloose += 1
        print(f"{d['id']}: {nloose} loose files ({time.time() - t0:.0f} s)", flush=True)

    # leaf-only records: key by w0 when mounted in place; name from any disc with that w0
    unresolved = 0
    for r in rows:
        if r[5] is None:
            known = by_w0.get(r[0]) if r[0] is not None else None
            if known:
                r[5] = known
            else:
                r[5] = '?' + r[6]
                unresolved += 1
    dropped = sum(1 for r in rows if r[0] is None)
    rows = [r for r in rows if r[0] is not None]
    order = {d['id']: i for i, d in enumerate(discs)}
    rows.sort(key=lambda r: (r[0], order[r[1]], r[2]))
    with open(CATALOG, 'w', encoding='utf-8', newline='\n') as out:
        out.write('# BlurX asset catalog. Generated by BlurX/code/tools/blurx_catalog/catalog.py build'
                  f'{" --scan" if args.scan else ""} on {time.strftime("%Y-%m-%d")}; do not edit.\n')
        out.write('# Format: see catalog.py. Rows: <hash> <disc> <container> <record> <size> <status> <path>\n')
        for d in discs:
            out.write('\t'.join(['@disc', d['id'], d['generation'], d['game'], '1' if d.get('damaged') else '0',
                                 d['root'], d['cache'], d['title'], ','.join(d.get('layers', [])) or '-']) + '\n')
        for p in paks:
            out.write('\t'.join(['@pak'] + [str(x) for x in p]) + '\n')
        for r in rows:
            out.write(f'{r[0]:08x}\t{r[1]}\t{r[2]}\t{r[7]}\t{r[3]}\t{r[4]}\t{r[5]}\n')
    print(f'wrote {CATALOG}: {len(discs)} discs, {len(paks)} archives, {len(rows)} rows, '
          f'{len({r[0] for r in rows})} distinct paths, {unresolved} leaf-only names unresolved, '
          f'{dropped} rows without a derivable key, {time.time() - t0:.0f} s')
    return 0


# ---- reading the catalog and the profiles (the game's C_Catalog/C_Profile do the same) ----

def load():
    discs, paks, entries = [], [], defaultdict(list)
    for line in open(CATALOG, encoding='utf-8'):
        if line.startswith('#'):
            continue
        f = line.rstrip('\n').split('\t')
        if f[0] == '@disc':
            discs.append({'id': f[1], 'generation': f[2], 'game': f[3], 'damaged': f[4] == '1',
                          'root': f[5], 'cache': f[6], 'title': f[7], 'layers': [] if len(f) < 9 or f[8] == '-' else f[8].split(',')})
        elif f[0] == '@pak':
            paks.append(f[1:])
        else:
            entries[int(f[0], 16)].append({'disc': f[1], 'container': f[2], 'record': f[3],
                                           'size': int(f[4]), 'status': f[5], 'path': f[6]})
    return discs, paks, entries


def load_profiles(overrides=None):
    """profiles.txt: 'profile <name> <description...>' starts a profile; inside it
    'order <disc> ...' is the preference order, 'prefer <glob> <disc> ...' puts discs
    first for matching paths, 'pin <glob> <disc> ...' serves matching paths only from
    those discs, 'accept <status-prefix> ...' lists statuses that may be served (default ok).
    An overrides file in the same syntax is applied on top (AssetCatalog::C_Profile::Load):
    for a profile it names, 'order'/'accept' replace and its rules go before the profile's own."""
    profiles = {}
    for path, is_override in ((PROFILES, False), (overrides, True)):
        if not path or (is_override and not os.path.exists(path)):
            continue
        cur, new_rules = None, []

        def close():
            if cur is not None and new_rules:
                cur['rules'][0:0] = new_rules if is_override else []
                if not is_override:
                    cur['rules'].extend(new_rules)

        for raw in open(path, encoding='utf-8'):
            line = raw.split('#', 1)[0].strip()
            if not line:
                continue
            w = line.split()
            if w[0] == 'profile':
                close()
                new_rules = []
                cur = profiles.get(w[1])
                if cur is None:
                    cur = profiles[w[1]] = {'name': w[1], 'order': [], 'rules': [], 'accept': ['ok'],
                                            'description': ''}
                if w[2:] or not is_override:
                    cur['description'] = ' '.join(w[2:])
            elif cur is None:
                raise SystemExit(f'{path}: {line!r} outside a profile')
            elif w[0] == 'order':
                cur['order'] = w[1:]
            elif w[0] in ('prefer', 'pin'):
                new_rules.append((w[0], w[1].replace('/', '\\').lower(), w[2:]))
            elif w[0] == 'accept':
                cur['accept'] = w[1:]
            else:
                raise SystemExit(f'{path}: unknown keyword {w[0]!r}')
        close()
    return profiles


def candidates(profile, path):
    p = path.replace('/', '\\').lower()
    if not p.startswith('\\'):
        p = '\\' + p
    for kind, glob, ds in profile['rules']:
        if fnmatch.fnmatchcase(p, glob):
            return ds if kind == 'pin' else ds + [d for d in profile['order'] if d not in ds]
    return profile['order']


def servable(profile, e):
    return any(e['status'] == a or e['status'].startswith(a + ':') for a in profile['accept'])


def choose(profile, rows, path):
    """the entry the profile serves from: the first candidate disc holding the file whose every
    copy is servable (its own mount order picks the container), as C_Profile::Choose"""
    for d in candidates(profile, path):
        mine = [e for e in rows if e['disc'] == d]
        if mine and all(servable(profile, e) for e in mine):
            return mine[0]
    return None


def resolve_one(profile, entries, path):
    """(disc, entry) the profile serves 'path' from, or (None, None)"""
    e = choose(profile, entries.get(pak2.path_hash(path), []), path)
    return (e['disc'], e) if e else (None, None)


def cmd_stats(_):
    discs, paks, entries = load()
    c = Counter()
    for rows in entries.values():
        for e in rows:
            c[(e['disc'], e['container'], e['status'].split(':')[0])] += 1
    for k in sorted(c):
        print(f'{k[0]:10} {k[1]:22} {k[2]:8} {c[k]:6}')
    print(len(entries), 'distinct paths')
    return 0


def cmd_which(a):
    _, _, entries = load()
    for path in a.paths:
        print(f'{path}  (hash {pak2.path_hash(path):08x})')
        for e in entries.get(pak2.path_hash(path), []):
            print(f"  {e['disc']:10} {e['container']:20} {e['record']:>5} {e['size']:>10} {e['status']:10} {e['path']}")
    return 0


def cmd_resolve(a):
    profile = load_profiles(a.overrides)[a.profile]
    _, _, entries = load()
    for path in a.paths:
        d, e = resolve_one(profile, entries, path)
        print(f"{path}: {d or 'not served'}" + (f" ({e['container']} record {e['record']}, {e['size']} bytes)" if e else ''))
    return 0


def cmd_report(a):
    profile = load_profiles(a.overrides)[a.profile]
    _, _, entries = load()
    served, refused = Counter(), Counter()
    for h, rows in entries.items():
        path = next((e['path'] for e in rows if not e['path'].startswith('?')), rows[0]['path'].lstrip('?'))
        # a path known only as a leaf name matches no path rule; the hash still resolves it
        best = choose(profile, rows, path)
        if best:
            served[best['disc']] += 1
        else:
            refused[','.join(sorted({e['disc'] for e in rows}))] += 1
    print(f"profile {profile['name']}: {profile['description']}")
    for d, n in served.most_common():
        print(f'  served from {d:10} {n:6}')
    for d, n in refused.most_common():
        print(f'  not served (only on {d}): {n}')
    return 0


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--overrides', default=OVERRIDES, help='profile overrides file (default catalog/profiles.local.txt, if present)')
    sub = ap.add_subparsers(dest='cmd', required=True)
    b = sub.add_parser('build')
    b.add_argument('--scan', action='store_true', help='count zero-filled blocks in loose files of damaged discs')
    b.add_argument('--only', help='comma-separated disc ids (default: every disc in discs.json)')
    sub.add_parser('stats')
    w = sub.add_parser('which')
    w.add_argument('paths', nargs='+')
    r = sub.add_parser('resolve')
    r.add_argument('profile')
    r.add_argument('paths', nargs='+')
    p = sub.add_parser('report')
    p.add_argument('profile')
    a = ap.parse_args()
    return {'build': build, 'stats': cmd_stats, 'which': cmd_which, 'resolve': cmd_resolve,
            'report': cmd_report}[a.cmd](a)


if __name__ == '__main__':
    sys.exit(main())
