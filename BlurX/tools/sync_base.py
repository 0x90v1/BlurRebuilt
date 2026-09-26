#!/usr/bin/env python3
"""sync_base.py - BlurX builds on the committed Blur1Multi, never on its working tree.

Other sessions edit Blur1Multi in place; a half-finished edit there must not
break or silently change the BlurX build (the user, 2026-09-25: "take what's
currently committed"). This exports Blur1Multi as of a commit (default HEAD)
with `git archive` into BlurX/_work/base/Blur1Multi and records the commit in
BlurX/_work/base/Blur1Multi.commit. CMakeLists.txt, the generators' --overlay
and tools/overlay.py all read Blur1Multi from there when it exists.

    python tools/sync_base.py            # export HEAD's Blur1Multi
    python tools/sync_base.py <commit>   # export another commit's
    python tools/sync_base.py --status   # the exported commit, and Blur1Multi commits since

After a sync: re-run gen_cmake.py (BlurX/README.md, Building), then
`python tools/overlay.py check`.
"""
import io
import os
import shutil
import subprocess
import sys
import tarfile

HERE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
REPO = os.path.dirname(HERE)
BASE = os.path.join(HERE, '_work', 'base')
DEST = os.path.join(BASE, 'Blur1Multi')
STAMP = DEST + '.commit'


def git(*args, binary=False):
    r = subprocess.run(['git', '-C', REPO] + list(args), capture_output=True)
    if r.returncode:
        raise SystemExit('git ' + ' '.join(args) + ' failed: ' + r.stderr.decode(errors='replace').strip())
    return r.stdout if binary else r.stdout.decode().strip()


def status():
    if not os.path.exists(STAMP):
        print('no snapshot yet; BlurX builds on ../Blur1Multi (the working tree)')
        return 0
    commit = open(STAMP).read().strip()
    print('Blur1Multi snapshot at', commit[:10], git('log', '-1', '--format=%ad %s', '--date=short', commit))
    since = git('log', '--format=%h %ad %s', '--date=short', f'{commit}..HEAD', '--', 'Blur1Multi')
    print('Blur1Multi commits since:' if since else 'no Blur1Multi commits since')
    for line in since.splitlines():
        print('  ' + line)
    return 0


def sync(commit):
    full = git('rev-parse', '--verify', commit + '^{commit}')
    data = git('archive', '--format=tar', full, 'Blur1Multi', binary=True)
    if os.path.isdir(DEST):
        shutil.rmtree(DEST)
    os.makedirs(BASE, exist_ok=True)
    with tarfile.open(fileobj=io.BytesIO(data)) as tar:
        tar.extractall(BASE, filter="data")
    with open(STAMP, 'w') as f:
        f.write(full + '\n')
    n = sum(len(fs) for _, _, fs in os.walk(DEST))
    print(f'exported Blur1Multi at {full[:10]} ({n} files) into {DEST}')
    return 0


def main(argv):
    if len(argv) > 1 and argv[1] == '--status':
        return status()
    return sync(argv[1] if len(argv) > 1 else 'HEAD')


if __name__ == '__main__':
    sys.exit(main(sys.argv))
