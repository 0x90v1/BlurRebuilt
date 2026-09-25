#!/usr/bin/env python3
"""
overlay.py - manage Blur1Multi's overlay copies of Blur1PC files.

The phase 2 build compiles ../Blur1PC/code with the files under ./code laid
over it: a file at the same relative path replaces the base file. Every such
copy stops inheriting later Blur1PC changes to that file, so this tool records
the base file's git blob hash when the copy is taken (overlay.json) and
reports copies whose base has moved on since.

    python tools/overlay.py take code/amax/game/GameSystems.cpp "why the copy exists"
    python tools/overlay.py check            # exit 1 if any base file changed since its copy
    python tools/overlay.py diff <relpath>   # base -> overlay diff (what the override does)
    python tools/overlay.py sync <relpath>   # mark the current base as merged (after re-basing by hand)

Run from the Blur1Multi directory.
"""
import hashlib
import json
import os
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BASE = os.path.normpath(os.path.join(HERE, '..', 'Blur1PC'))
MANIFEST = os.path.join(HERE, 'overlay.json')


def blob_hash(path):
    """git's blob id of the file as it is on disk (works for untracked files too)"""
    data = open(path, 'rb').read()
    return hashlib.sha1(b'blob %d\0' % len(data) + data).hexdigest()


def load():
    if os.path.exists(MANIFEST):
        return json.load(open(MANIFEST, encoding='utf-8'))
    return {}


def save(m):
    with open(MANIFEST, 'w', encoding='utf-8', newline='\n') as fh:
        json.dump(m, fh, indent=1, sort_keys=True)
        fh.write('\n')


def rel(p):
    return p.replace('\\', '/').strip('/')


def cmd_take(relpath, why):
    relpath = rel(relpath)
    src = os.path.join(BASE, relpath)
    dst = os.path.join(HERE, relpath)
    if not os.path.isfile(src):
        print('no such base file:', src)
        return 1
    if os.path.exists(dst):
        print('overlay copy already exists:', dst)
        return 1
    os.makedirs(os.path.dirname(dst), exist_ok=True)
    shutil.copy2(src, dst)
    m = load()
    m[relpath] = {'base_blob': blob_hash(src), 'why': why}
    save(m)
    print('took', relpath, 'base blob', m[relpath]['base_blob'][:12])
    return 0


def cmd_sync(relpath):
    relpath = rel(relpath)
    src = os.path.join(BASE, relpath)
    m = load()
    if relpath not in m:
        print('not in overlay.json:', relpath)
        return 1
    m[relpath]['base_blob'] = blob_hash(src)
    save(m)
    print('synced', relpath, 'to base blob', m[relpath]['base_blob'][:12])
    return 0


def cmd_check():
    m = load()
    bad = 0
    for relpath, e in sorted(m.items()):
        src = os.path.join(BASE, relpath)
        dst = os.path.join(HERE, relpath)
        if not os.path.exists(dst):
            print('MISSING overlay copy:', relpath)
            bad += 1
            continue
        if not os.path.exists(src):
            print('base file gone:', relpath)
            bad += 1
            continue
        h = blob_hash(src)
        if h != e['base_blob']:
            print(f'CHANGED in base since the copy: {relpath} ({e["base_blob"][:12]} -> {h[:12]}); '
                  f'merge the base change into the overlay copy, then: overlay.py sync {relpath}')
            bad += 1
        else:
            print('ok      ', relpath)
    # overlay files that shadow a base file without being recorded
    for root, dirs, files in os.walk(os.path.join(HERE, 'code')):
        for f in files:
            p = rel(os.path.relpath(os.path.join(root, f), HERE))
            if p not in m and os.path.exists(os.path.join(BASE, p)):
                print('UNRECORDED override (use overlay.py take):', p)
                bad += 1
    print('base:', BASE, '| overrides:', len(m), '| problems:', bad)
    return 1 if bad else 0


def cmd_diff(relpath):
    relpath = rel(relpath)
    src = os.path.join(BASE, relpath)
    dst = os.path.join(HERE, relpath)
    return subprocess.call(['git', 'diff', '--no-index', '--', src, dst])


def main(argv):
    if len(argv) < 2:
        print(__doc__)
        return 2
    c = argv[1]
    if c == 'take' and len(argv) >= 4:
        return cmd_take(argv[2], ' '.join(argv[3:]))
    if c == 'check':
        return cmd_check()
    if c == 'diff' and len(argv) == 3:
        return cmd_diff(argv[2])
    if c == 'sync' and len(argv) == 3:
        return cmd_sync(argv[2])
    print(__doc__)
    return 2


if __name__ == '__main__':
    sys.exit(main(sys.argv))
