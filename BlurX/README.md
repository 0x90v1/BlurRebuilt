# BlurX — phase 3: the Blur 2 (Kanzen) prototypes

Phase 3 of Blur2PC (AGENTS.md "Project stages/layers"): the engine running the
Xbox 360 Blur 2 prototype discs, several at once, each file served by the disc
that suits it best through the **asset catalog** (docs/decisions.md D12). The
test base is the Nov 10 prototype; Nov 25 is a damaged rip.

This project holds no copy of the engine or of phase 2. It builds
`../Blur1PC/code` with `../Blur1Multi/code` and `./code` laid over it, so every
fix made in phase 1 or phase 2 reaches this build at its next generate.
Deleting this folder leaves Blur1Multi and Blur1PC untouched.

## How the layering works

- **Bases**: `../Blur1PC/code` (`BLUR_TOP`), then Blur1Multi **as committed**
  (`BLUR1MULTI_DIR` = `_work/base/Blur1Multi`, exported from git by
  `python tools/sync_base.py`; `--status` shows the commit and what Blur1Multi
  has committed since). Unfinished edits in `../Blur1Multi` never reach this
  build; run `sync_base.py` and regenerate to take in new Blur1Multi commits.
- **Overlay**: `./code` (`BLUR_OVERLAY`). A file at the same relative path as a
  Blur1PC file or a Blur1Multi file replaces it; the highest layer wins. A file
  at a new path is new and is declared in `cmake/blurx.cmake`.
- **Blur1Multi's own additions** (`../Blur1Multi/cmake/multi.cmake`) are read as
  they are; `blurx_take_multi_sources()` swaps in a BlurX copy of a file that
  exists only in Blur1Multi.
- **Defines**: every unit gets `BLUR1MULTI=1` (phase 2 code tests it) and `BLURX=1`.
- **Manifest**: `overlay.json`, kept with `python tools/overlay.py ...` (the
  commands of `../Blur1Multi/README.md`). A copy is recorded against the highest
  base that holds the file (`base_layer`), and `check` reports when that base
  changes. Current copies:
  - `code/amax/game/GameSystems.cpp` (from Blur1Multi): catalog mode.
  - `code/mercury/library/util/logicalFs/include/pakfs/impl/PakDefinitions.h`
    (from Blur1PC): reads pak version 3 (the Nov discs' `root.pak`) with the
    version 2 path.
  - `code/mercury/library/util/logicalFs/source/pakfs/impl/PakCompressXMem.cpp`,
    `PakCompressPacketXMem.cpp` (from Blur1PC; compiled through Blur1Multi's
    `multi.cmake`): version 2/3 payloads have no size prefix (stored size from the
    FAT), as Blur1PC's ZLib codecs already handle for PC version 2.
  - `code/mercury/library/util/x360convert/source/EffectConvert.cpp` (from
    Blur1Multi): XEFX version 12 (Nov discs) as well as 9; emits PCFX 9.

## Building

From this directory:

```
python tools/sync_base.py
python ../Blur1PC/code/tools/jam2cmake/jamextract.py --top ../Blur1PC --root code/amax/game/jamfile.jam --target WIN32 --platform PC --build Speed --out _work/build/targets_pc_speed.json
python ../Blur1PC/code/tools/jam2cmake/shadergen.py --json _work/build/targets_pc_speed.json --out _work/build/gen/shaders --overlay _work/base/Blur1Multi --overlay .
python ../Blur1PC/code/tools/jam2cmake/luabundle.py --json _work/build/targets_pc_speed.json --out _work/build/gen/blobs/amax_lua_bundle.cpp --overlay _work/base/Blur1Multi --overlay .
python ../Blur1PC/code/tools/jam2cmake/gen_cmake.py --json _work/build/targets_pc_speed.json --out cmake/targets.cmake --project-dir . --overlay _work/base/Blur1Multi=BLUR1MULTI_DIR --overlay .
cmake -S . -B _work/build/vs -G "Visual Studio 18 2026" -A Win32
cmake --build _work/build/vs --config Release --target Amax -- -m -v:m -nologo > _work/build/amax.log 2>&1
python ../Blur1PC/code/tools/jam2cmake/errsum.py _work/build/amax.log
```

Re-run `gen_cmake.py` after taking or removing an overlay file here or in
Blur1Multi. The user's exe for this phase is `_work/run/Amax.exe` (`play.bat`).

## The asset catalog

Two halves: the catalog's **contents** are built off line; the **choice** of
disc is made by the game when it opens a file, from the catalog and a profile.

| File | What |
|---|---|
| `catalog/discs.json` | the discs covered: id, where they are on this machine, generation, damaged flag; the mount table |
| `catalog/catalog.tsv` | generated, tracked: every file on every disc (hash of the virtual path, disc, container, record, size, status, path) |
| `catalog/profiles.txt` | serving profiles (format at the top of the file); defaults from Blur2Test's documented policy |
| `catalog/profiles.local.txt` | untracked local overrides in the same syntax (or `--profile-overrides=<file>`) |
| `code/tools/blurx_catalog/catalog.py` | `build` (reads the discs, seconds), `stats`, `which <path>`, `resolve <profile> <path>`, `report <profile>` |
| `code/tools/blurx_catalog/pak2.py` | pak directory reader and container framing check, ported from Blur2Test |
| `code/tools/blurx_catalog/dumpcheck.py` | `list`/`check`: verify what the game serves against the catalog and independent copies |
| `code/mercury/library/util/assetcatalog/` | the game side: `AssetCatalog::C_Catalog`, `C_Profile` (no engine dependencies), `engine/CatalogFs` (the file-system layer) |

Rebuild the catalog after a disc or `discs.json` changes:
`python code/tools/blurx_catalog/catalog.py build --scan` (`--scan` also counts
zero-filled blocks in the loose files of discs marked damaged).

Statuses: `ok`; `bad:<why>` when the pak record's stored container does not
frame (its size fields do not walk to its declared end: a truncated or
overwritten payload); `suspect:<n>-zero-blocks` for a loose file with
zero-filled 32 KB blocks on a damaged disc (the healthy discs' level files have
none). Profiles serve `ok` only unless they say otherwise.

## Running

`play.bat [profile]` (default `nov10`). Command-line options and variables:

| Option | Variable | Meaning |
|---|---|---|
| `--profile=<name>` | `BLURX_PROFILE` | catalog mode with this profile |
| `--catalog=<dir>` | `BLURX_CATALOG` | where `catalog.tsv` and `profiles.txt` are (default `<exe>\catalog`, then `<exe>\..\..\catalog`) |
| `--profile-overrides=<file>` | `BLURX_PROFILE_OVERRIDES` | overrides file (default `<catalog>\profiles.local.txt`) |
| `--disc=<id>=<dir>` | `BLURX_DISC_<ID>` | where a disc is, when not where the catalog was built |
| | `BLURX_CATALOG_TRACE=1` | log every file served and its disc |

In catalog mode the working directory stays the writable layer 0 (run from
`_work/run`). Each disc is served through its own chain: its mounts, then for a
Generation B disc the Kanzen layer (`engine/KanzenFs`: rewrites Kanzen layouts
to Blur 1's with `source/KanzenNormalise.cpp` - cycles 16/17, skeletons class 8
so far - and refuses what it cannot rewrite, and every Kanzen level file), then
Blur1Multi's converters (PC data, D11). A copy its chain refuses or cannot
convert is served by the profile's next disc; the log names each refusal.
Without `--profile`, the exe behaves exactly as Blur1Multi's (`--source=<dir>`).

Profiles: `nov10` (faithful: Blur 2 data only; stops in the race book because
Blur 1's game code cannot run Kanzen gameplay data), `nov10-b1rules` (the D13
stopgap: Nov 10 content with retail Blur 1's `\gamedata\`; boots to the front
end; `play.bat`'s default), `aug31`, `nov25`, `blur1x360` (regression).

`code/tools/kanzen-verify` (build: `_work/tools/build_kanzen_verify.bat`) runs
the normalisers over a folder of extracted files (e.g. Blur2Test's `raw_root`)
and reports what normalises and why the rest does not. `BLURX_OOM_STACK=1`
turns the engine's out-of-memory panic into an exception, so `crash.txt` has
the allocating stack.

Verified (2026-09-25): `dumpcheck.py` over 166 paths (profile `nov10`, 12 per
disc/container group) and 144 (profile `blur1x360`): every file served from the
disc the policy names; 153 and 144 byte-identical to Blur2Test's or Blur1Multi's
independent extractions or to the loose file on the disc, the rest size-checked
only (no independent copy). A boot with `nov10` loads Kanzen's UI database and
stops in the race book (Blur 2 pickup data); see the ledger.

Automation (minimised, AGENTS.md rule 7): `_work/run/run_catalog.ps1 -Profile nov10`
boots and prints the catalog's log; `_work/run/run_dump.ps1 -List <list>` dumps
listed files through the catalog layer (unconverted) for `dumpcheck.py check`.
