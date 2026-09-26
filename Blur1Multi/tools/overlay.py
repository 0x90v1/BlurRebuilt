#!/usr/bin/env python3
"""
overlay.py - manage a phase's overlay copies of the files of the phases below it.

The phase 2 build compiles ../Blur1PC/code with the files under ./code laid
over it: a file at the same relative path replaces the base file. Every such
copy stops inheriting later Blur1PC changes to that file, so overlay.json
records, per copy, the Blur1PC commit and blob it was last brought in line
with (base_commit, base_blob), when (synced), and every take and sync since
(history). From those, git answers what Blur1PC changed afterwards.

    python tools/overlay.py take <relpath> "<why>"     # start overriding a base file
    python tools/overlay.py check                      # exit 1 if any base file changed since its last sync
    python tools/overlay.py status                     # every copy: last sync, base commit, base commits since
    python tools/overlay.py changes <relpath>          # Blur1PC commits and diff to the base file since the last sync
    python tools/overlay.py history <relpath>          # the take and every sync of one copy
    python tools/overlay.py diff <relpath>             # base -> overlay diff (what the override does)
    python tools/overlay.py sync <relpath> "<note>"    # after merging the base changes into the copy by hand

take and sync record the current Blur1PC commit, so the base file must be
committed first (no uncommitted edits), or the record would point at nothing.
Blob ids are git's (line endings normalised), so they match `git log --raw`.
Paths are relative to the phase folder (code/...). Run from anywhere.

[Blur2PC] Stacked phases (2026-09-25): this file serves Blur1Multi, whose one
base is Blur1PC. BlurX/tools/overlay.py imports it and calls configure() with
its own folder and the bases Blur1Multi, Blur1PC (highest first): a BlurX copy
is recorded against the highest base that has the file (its `base_layer`), so
it can shadow a Blur1Multi overlay copy, a Blur1Multi-only file or a Blur1PC
file. Entries without `base_layer` are against Blur1PC.
"""
import datetime
import json
import os
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
REPO = os.path.dirname(HERE)
BASES = ['Blur1PC']            # phase folders below this one, highest first
MANIFEST = os.path.join(HERE, 'overlay.json')


BASE_DIRS = {}                 # base phase -> folder read in its place (a committed snapshot)


def configure(here, bases, base_dirs=None):
    """point the tool at another phase folder and its bases (highest first); base_dirs maps a
    base to the folder that stands for it (BlurX reads Blur1Multi from a committed snapshot)"""
    global HERE, REPO, BASES, MANIFEST, BASE_DIRS
    HERE = os.path.abspath(here)
    REPO = os.path.dirname(HERE)
    BASES = list(bases)
    MANIFEST = os.path.join(HERE, 'overlay.json')
    BASE_DIRS = dict(base_dirs or {})


def base_dir(layer):
    return BASE_DIRS.get(layer, os.path.join(REPO, layer))


def base_layer(relpath, entry=None):
    """the base phase a copy is recorded against: the entry's, else the highest base holding the file"""
    if entry is not None:
        return entry.get('base_layer', 'Blur1PC')
    for b in BASES:
        if os.path.isfile(os.path.join(base_dir(b), relpath)):
            return b
    return BASES[-1]


def base_file(relpath, entry=None):
    return os.path.join(base_dir(base_layer(relpath, entry)), relpath)


def git(*args, check=True):
    r = subprocess.run(['git', '-C', REPO] + list(args), capture_output=True, text=True)
    if check and r.returncode:
        raise SystemExit('git ' + ' '.join(args) + ' failed: ' + r.stderr.strip())
    return r.stdout.strip()


def base_git_path(relpath, entry=None):
    return base_layer(relpath, entry) + '/' + relpath


def blob_hash(relpath, entry=None):
    """git's blob id of the base file in the working tree, with git's line-ending filters applied"""
    return git('hash-object', '--path', base_git_path(relpath, entry), base_file(relpath, entry))


def committed_blob(commit, relpath, entry=None):
    return git('rev-parse', '--verify', '-q', f'{commit}:{base_git_path(relpath, entry)}', check=False)


def now():
    return datetime.datetime.now().astimezone().isoformat(timespec='seconds')


def load():
    if os.path.exists(MANIFEST):
        return json.load(open(MANIFEST, encoding='utf-8'))
    return {}


def save(m):
    with open(MANIFEST, 'w', encoding='utf-8', newline='\n') as fh:
        json.dump(m, fh, indent=1, sort_keys=True)
        fh.write('\n')


def rel(p):
    p = p.replace('\\', '/').strip('/')
    for prefix in [os.path.basename(HERE) + '/'] + [b + '/' for b in BASES]:
        if p.startswith(prefix):
            p = p[len(prefix):]
    return p


def current_base(relpath, entry=None):
    """(commit, blob) for the base file as committed at HEAD; exits if the working copy differs"""
    head = git('rev-parse', 'HEAD')
    blob = blob_hash(relpath, entry)
    if committed_blob(head, relpath, entry) != blob:
        raise SystemExit(f'{base_git_path(relpath, entry)} has uncommitted changes (or is untracked); '
                         'commit the base change first so the record can name its commit')
    return head, blob


def record(entry, action, commit, blob, note):
    entry['base_commit'] = commit
    entry['base_blob'] = blob
    entry['synced'] = now()
    entry.setdefault('history', []).append(
        {'action': action, 'date': entry['synced'], 'base_commit': commit, 'base_blob': blob, 'note': note})


def commits_since(entry, relpath):
    out = git('log', '--format=%h %ad %s', '--date=short', f'{entry["base_commit"]}..HEAD', '--', base_git_path(relpath, entry))
    return [l for l in out.splitlines() if l]


def cmd_take(relpath, why):
    relpath = rel(relpath)
    src = base_file(relpath)
    dst = os.path.join(HERE, relpath)
    if not os.path.isfile(src):
        print('no such base file:', src)
        return 1
    if os.path.exists(dst):
        print('overlay copy already exists:', dst)
        return 1
    layer = base_layer(relpath)
    entry = {'why': why}
    if layer != 'Blur1PC':
        entry['base_layer'] = layer
    commit, blob = current_base(relpath, entry)
    os.makedirs(os.path.dirname(dst), exist_ok=True)
    shutil.copy2(src, dst)
    m = load()
    m[relpath] = entry
    record(m[relpath], 'take', commit, blob, why)
    save(m)
    print('took', relpath, 'from', layer, 'at commit', commit[:10])
    return 0


def cmd_sync(relpath, note):
    relpath = rel(relpath)
    m = load()
    if relpath not in m:
        print('not in overlay.json:', relpath)
        return 1
    e = m[relpath]
    since = commits_since(e, relpath)
    commit, blob = current_base(relpath, e)
    if blob == e['base_blob']:
        print('base unchanged since the last sync; nothing to record')
        return 0
    if not note:
        note = 'merged ' + ', '.join(c.split()[0] for c in since) if since else 'merged base changes'
    record(e, 'sync', commit, blob, note)
    save(m)
    print('synced', relpath, 'to', base_layer(relpath, e), 'commit', commit[:10], '-', note)
    return 0


def cmd_check():
    m = load()
    bad = 0
    for relpath, e in sorted(m.items()):
        src = base_file(relpath, e)
        dst = os.path.join(HERE, relpath)
        if not os.path.exists(dst):
            print('MISSING overlay copy:', relpath)
            bad += 1
            continue
        if not os.path.exists(src):
            print('base file gone:', relpath)
            bad += 1
            continue
        if blob_hash(relpath, e) != e['base_blob']:
            since = commits_since(e, relpath)
            what = f'{len(since)} commit(s)' if since else 'uncommitted edits'
            print(f'CHANGED in base since {e["synced"][:10]} ({what}): {relpath}; '
                  f'see overlay.py changes {relpath}, merge, then overlay.py sync {relpath} "<note>"')
            bad += 1
        else:
            print('ok      ', relpath)
    # overlay files that shadow a base file without being recorded
    for root, dirs, files in os.walk(os.path.join(HERE, 'code')):
        for f in files:
            p = rel(os.path.relpath(os.path.join(root, f), HERE))
            if p not in m and any(os.path.exists(os.path.join(base_dir(b), p)) for b in BASES):
                print('UNRECORDED override (use overlay.py take):', p)
                bad += 1
    print('bases:', ', '.join(BASES), '| overrides:', len(m), '| problems:', bad)
    return 1 if bad else 0


def cmd_status():
    m = load()
    for relpath, e in sorted(m.items()):
        since = commits_since(e, relpath)
        dirty = blob_hash(relpath, e) != committed_blob('HEAD', relpath, e)
        state = f'{len(since)} base commit(s) since' if since else 'base unchanged'
        if dirty:
            state += ', base has uncommitted edits'
        print(f'{relpath}\n    last sync {e["synced"]} at {e["base_commit"][:10]} '
              f'({len(e["history"])} record(s)); {state}')
    return 0


def cmd_changes(relpath):
    relpath = rel(relpath)
    e = load().get(relpath)
    if not e:
        print('not in overlay.json:', relpath)
        return 1
    print(f'# {relpath}: last synced {e["synced"]} at {base_layer(relpath, e)} commit {e["base_commit"][:10]}')
    since = commits_since(e, relpath)
    print('# base commits to this file since:' if since else '# no base commits to this file since')
    for c in since:
        print('#   ' + c)
    sys.stdout.flush()
    # commit -> working tree, so uncommitted base edits show too
    return subprocess.call(['git', '-C', REPO, '--no-pager', 'diff', e['base_commit'], '--', base_git_path(relpath, e)])


def cmd_history(relpath):
    relpath = rel(relpath)
    e = load().get(relpath)
    if not e:
        print('not in overlay.json:', relpath)
        return 1
    print(relpath, '-', e['why'])
    for h in e['history']:
        print(f'  {h["date"]}  {h["action"]:<4}  base {h["base_commit"][:10]}  {h["note"]}')
    return 0


def cmd_diff(relpath):
    relpath = rel(relpath)
    e = load().get(relpath)
    return subprocess.call(['git', 'diff', '--no-index', '--', base_file(relpath, e), os.path.join(HERE, relpath)])


def main(argv):
    if len(argv) < 2:
        print(__doc__)
        return 2
    c, a = argv[1], argv[2:]
    if c == 'take' and len(a) >= 2:
        return cmd_take(a[0], ' '.join(a[1:]))
    if c == 'sync' and len(a) >= 1:
        return cmd_sync(a[0], ' '.join(a[1:]))
    if c in ('check', 'status') and not a:
        return cmd_check() if c == 'check' else cmd_status()
    if c in ('changes', 'history', 'diff') and len(a) == 1:
        return {'changes': cmd_changes, 'history': cmd_history, 'diff': cmd_diff}[c](a[0])
    print(__doc__)
    return 2


if __name__ == '__main__':
    sys.exit(main(sys.argv))
