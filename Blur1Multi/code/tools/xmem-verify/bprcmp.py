#!/usr/bin/env python3
"""
bprcmp.py - compare two little-endian resource bundles resource by resource
(matched by name): dimensions, format, mips, data size and data bytes.

  python bprcmp.py <converted.bpr> <pc.bpr> [--show N]
"""
import collections
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import bprlist  # noqa: E402


def main(a):
    x, y = bprlist.read(a[1]), bprlist.read(a[2])
    show = int(a[a.index('--show') + 1]) if '--show' in a else 8
    ya = {r['name']: r for r in y['resources']}
    tally = collections.Counter()
    shown = 0
    for r in x['resources']:
        p = ya.get(r['name'])
        if not p:
            tally['only in converted'] += 1
            continue
        if (r['format'], r['mips'], r['w'], r['h']) != (p['format'], p['mips'], p['w'], p['h']):
            tally['different description'] += 1
            if shown < show:
                shown += 1
                print(f"  DESC {r['name']}: {r['format']} {r['mips']} {r['w']}x{r['h']} vs {p['format']} {p['mips']} {p['w']}x{p['h']}")
            continue
        dx = x['data'][r['offset']:r['offset'] + r['size']]
        dy = y['data'][p['offset']:p['offset'] + p['size']]
        if dx == dy:
            tally['identical'] += 1
            continue
        tally[f'different data (fmt {r["format"]})'] += 1
        if shown < show:
            shown += 1
            n = next((i for i in range(min(len(dx), len(dy))) if dx[i] != dy[i]), min(len(dx), len(dy)))
            diff = sum(1 for i in range(min(len(dx), len(dy))) if dx[i] != dy[i])
            print(f"  DATA {r['name']} fmt {r['format']} {r['w']}x{r['h']} mips {r['mips']}: {len(dx)} vs {len(dy)} bytes, "
                  f"{diff} differ, first at 0x{n:x}: {dx[n:n + 8].hex()} vs {dy[n:n + 8].hex()}")
    tally['only in PC'] = len(set(ya) - set(r['name'] for r in x['resources']))
    print('  ' + ', '.join(f'{k} {v}' for k, v in sorted(tally.items())))
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
