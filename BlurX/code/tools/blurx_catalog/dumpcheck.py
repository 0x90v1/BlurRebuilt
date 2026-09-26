#!/usr/bin/env python3
"""dumpcheck.py - check the game's catalog layer against the catalog and the discs.

    python dumpcheck.py list <profile> <out.txt> [--per-group N]
    python dumpcheck.py check <profile> <out.txt>

`list` picks N virtual paths (default 12) from every (disc, container) group the
profile serves from, so every disc and every archive kind the profile uses is
exercised, and writes them one per line for the game's dump hook
(BLUR1MULTI_PAKDUMP; BlurX/_work/run/run_dump.ps1 runs it in catalog mode,
unconverted). `check` reads <out dir>/dump/ and, per path:
  - the file exists and has the catalog's size for the entry the profile chose;
  - its bytes equal an independent reference: the loose file on the disc, or
    Blur2Test's extraction of the same record (_work/disc_cache/<disc>/<archive
    stem>/..., raw_root/ for Nov 10's root.pak), found by archive and record
    index; paths with no reference are reported as size-only;
  - the game's log line for the path (BLURX_CATALOG_TRACE, <out dir>/dump/amax.log) names the same disc.
"""
from __future__ import annotations

import argparse
import glob
import os
import re
import sys
from collections import defaultdict

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import catalog  # noqa: E402

B2T = r'C:\AIProjects\Blur2Test'
MULTI_X360 = os.path.join(os.path.dirname(catalog.BLURX), 'Blur1Multi', '_work', 'extract', 'x360')


def served(profile):
    discs, paks, entries = catalog.load()
    out = []
    for h, rows in entries.items():
        path = next((e['path'] for e in rows if not e['path'].startswith('?')), None)
        if path is None:
            continue
        e = catalog.choose(profile, rows, path)
        if e:
            out.append((h, path, e))
    return discs, paks, out


def cmd_list(a):
    profile = catalog.load_profiles(a.overrides)[a.profile]
    _, _, rows = served(profile)
    groups = defaultdict(list)
    for h, path, e in sorted(rows, key=lambda r: r[0]):
        groups[(e['disc'], e['container'])].append(path)
    with open(a.out, 'w', encoding='utf-8', newline='\n') as f:
        f.write(f'# dumpcheck.py list {a.profile}: {a.per_group} paths per (disc, container) group\n')
        for k in sorted(groups):
            g = groups[k]
            step = max(1, len(g) // a.per_group)
            for p in g[::step][:a.per_group]:
                f.write(p.replace('\\', '/') + '\n')
            print(f'{k[0]:10} {k[1]:20} {len(g):6} served, {min(a.per_group, len(g))} listed')
    return 0


def reference(discs, paks, e):
    """(path of an independent copy of the entry's bytes, what it is) or (None, why)"""
    disc = next(d for d in discs if d['id'] == e['disc'])
    if e['container'] == 'loose':
        for base in [disc['root']] + [os.path.join(disc['root'], l) for l in disc.get('layers', [])]:
            cand = os.path.join(base, e['path'].lstrip('\\'))
            if os.path.isfile(cand):
                return cand, 'disc loose file'
        return None, 'loose file not found on the disc'
    stem = os.path.splitext(e['container'])[0]
    mount, src = next((p[4], p[5]) for p in paks if p[0] == e['disc'] and p[1] == e['container'])
    # the record's own name inside the archive
    rel = e['path'][len(mount.replace('/', '\\').rstrip('\\')):] if not e['path'].startswith('?') else None
    if rel is not None:
        inner = src.replace('/', '\\').rstrip('\\') + rel
        cand = os.path.join(B2T, '_work', 'disc_cache', e['disc'], stem, inner.lstrip('\\'))
        if os.path.isfile(cand):
            return cand, 'Blur2Test disc_cache'
        if e['disc'] == 'blur1x360':
            # Blur1Multi's standalone extraction of the retail 360 paks (code/tools/xmem-verify --extract)
            cand = os.path.join(MULTI_X360, stem, inner.lstrip('\\'))
            if os.path.isfile(cand):
                return cand, 'Blur1Multi xmem_verify extraction'
    leaf = e['path'].rsplit('\\', 1)[-1].lstrip('?')
    rec = int(e['record'])
    pats = [os.path.join(B2T, '_work', 'disc_cache', e['disc'], stem, '*', f'record_{rec:04d}_*')]
    if e['disc'] == 'nov10' and stem == 'root':
        pats.append(os.path.join(B2T, 'raw_root', '*', f'record_{rec:04d}_*'))
    if e['disc'] == 'nov10' and stem == 'gamedata' and rel is not None:
        cand = os.path.join(B2T, 'raw_gamedata', inner.lstrip(chr(92)))
        if os.path.isfile(cand):
            return cand, 'Blur2Test raw_gamedata (Nov 10)'
    for pat in pats:
        for c in glob.glob(pat):
            if os.path.basename(c).lower().endswith(leaf.lower()):
                return c, 'Blur2Test record extraction'
    return None, 'no Blur2Test extraction of this record'


def cmd_check(a):
    profile = catalog.load_profiles(a.overrides)[a.profile]
    discs, paks, entries = catalog.load()
    dump = os.path.join(os.path.dirname(os.path.abspath(a.out)), 'dump')
    trace = {}
    runlog = os.path.join(dump, 'amax.log')   # run_dump.ps1 copies the run's log there
    if os.path.exists(runlog):
        rx = re.compile(r'(\S+) <- (\S+) (\S+)')
        for line in open(runlog, encoding='latin-1'):
            m = rx.search(line)
            if m:
                trace[catalog.pak2.path_hash(m.group(1))] = m.group(2)
    counts = defaultdict(int)
    for raw in open(a.out, encoding='utf-8'):
        p = raw.strip()
        if not p or p.startswith('#'):
            continue
        h = catalog.pak2.path_hash(p)
        e = catalog.choose(profile, entries.get(h, []), p)
        f = os.path.join(dump, p.lstrip('/').replace('/', os.sep))
        tag = f"{e['disc']}/{e['container']}#{e['record']}" if e else 'not served'
        if not os.path.isfile(f):
            print(f'MISSING   {p} (expected from {tag})')
            counts['missing'] += 1
            continue
        data = open(f, 'rb').read()
        problems = []
        if trace.get(h) not in (None, e['disc']):
            problems.append(f"game served it from {trace.get(h)}")
        # the disc may hold several copies (pak and loose); its own layering picks one
        # (non-gold builds: the newest), so the dump must equal one of that disc's copies
        copies = [c for c in entries.get(h, []) if c['disc'] == e['disc']]
        sized = [c for c in copies if c['size'] == len(data)]
        if not sized:
            problems.append(f"size {len(data)} matches no copy on {e['disc']} ({', '.join(str(c['size']) for c in copies)})")
        verdict = 'SIZE-ONLY'
        for c in sized:
            ref, what = reference(discs, paks, c)
            if ref and os.path.isfile(ref):
                if open(ref, 'rb').read() == data:
                    verdict, tag = 'IDENTICAL', f"{c['disc']}/{c['container']}#{c['record']}"
                    break
                problems.append(f'bytes differ from {what} {ref}')
        if problems and verdict != 'IDENTICAL':
            verdict = 'DIFFERENT'
        elif verdict == 'IDENTICAL':
            problems = [p_ for p_ in problems if not p_.startswith('bytes differ')]
        counts[verdict] += 1
        if problems or a.verbose or verdict == 'SIZE-ONLY':
            print(f'{verdict:9} {p} <- {tag}' + (f' ({"; ".join(problems)})' if problems else f' ({what})'))
    print(dict(counts), f'| game trace lines matched: {sum(1 for _ in trace)}')
    return 1 if counts['missing'] or counts['DIFFERENT'] else 0


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--overrides', default=catalog.OVERRIDES)
    sub = ap.add_subparsers(dest='cmd', required=True)
    l_ = sub.add_parser('list')
    l_.add_argument('profile')
    l_.add_argument('out')
    l_.add_argument('--per-group', type=int, default=12)
    c = sub.add_parser('check')
    c.add_argument('profile')
    c.add_argument('out')
    c.add_argument('--verbose', action='store_true')
    a = ap.parse_args()
    return cmd_list(a) if a.cmd == 'list' else cmd_check(a)


if __name__ == '__main__':
    sys.exit(main())
