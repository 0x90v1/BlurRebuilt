# Blur1Multi — phase 2: Blur 1 from any platform's data

This project builds the Blur 1 engine and game from `../Blur1PC/code` and adds
support for data written by other platforms: Xbox 360 retail Blur first, PS3
later. It holds no copy of the engine. Deleting this folder leaves Blur1PC
untouched; improvements made in Blur1PC reach this build at the next generate.

## How the layering works

- **Base tree**: `../Blur1PC/code`, read-only from here. `BLUR_TOP` in CMake.
- **Overlay**: `./code` mirrors the base layout. A file at the same relative
  path *replaces* the base file in the build (sources, headers through the
  include path, PCH headers, built-in shaders, Lua scripts). A file at a new
  path is a *new* file and must be declared in `cmake/multi.cmake`.
- **Manifest**: `overlay.json` records, for every replaced base file, the git
  blob hash of the base at copy time and why the copy exists. A replaced file
  no longer inherits Blur1PC changes by itself, so keep overrides few and
  small, and run the check before every build:

```
python tools/overlay.py check                       # lists base files that changed since their copy
python tools/overlay.py take <relpath> "<why>"      # start overriding a base file
python tools/overlay.py diff <relpath>              # what the override changes
python tools/overlay.py sync <relpath>              # after merging a base change into the copy
```

- **Define**: every translation unit is compiled with `BLUR1MULTI=1`.
- **Shared build description**: `../Blur1PC/cmake/blur_common.cmake` (flags,
  vendor SDKs, shims, system libs) and `../Blur1PC/cmake/overrides.cmake`
  are included as they are; `cmake/multi.cmake` adds this project's targets.

## Building

From this directory (the generators live in Blur1PC; `--overlay .` makes them
honour the overlay, `--project-dir .` puts generated files under `_work/` here):

```
python ../Blur1PC/code/tools/jam2cmake/jamextract.py --top ../Blur1PC --root code/amax/game/jamfile.jam --target WIN32 --platform PC --build Speed --out _work/build/targets_pc_speed.json
python ../Blur1PC/code/tools/jam2cmake/shadergen.py --json _work/build/targets_pc_speed.json --out _work/build/gen/shaders --overlay .
python ../Blur1PC/code/tools/jam2cmake/luabundle.py --json _work/build/targets_pc_speed.json --out _work/build/gen/blobs/amax_lua_bundle.cpp --overlay .
python ../Blur1PC/code/tools/jam2cmake/gen_cmake.py --json _work/build/targets_pc_speed.json --out cmake/targets.cmake --project-dir . --overlay .
cmake -S . -B _work/build/vs -G "Visual Studio 18 2026" -A Win32
cmake --build _work/build/vs --config Release --target Amax -- -m -v:m -nologo > _work/build/amax.log 2>&1
python ../Blur1PC/code/tools/jam2cmake/errsum.py _work/build/amax.log
```

`cpgen.py` is run in Blur1PC (its headers are generated beside the `.cp`
files there). Re-run `gen_cmake.py` after taking or removing an overlay file:
it decides per file which tree to compile from.

The user's exe for this phase is `_work/run/Amax.exe` here (`play.bat`).

## Game source

`code/mercury/library/util/gamesource/` (new library `gamesource`) detects
what a data directory is: PC (little-endian `cache/root.pak`), Xbox 360
(big-endian pak beside an `.xex`), PS3 (`PS3_GAME/USRDIR`). The overlay
copy of `amax/game/GameSystems.cpp` calls `GameSource::Select` at the start
of `CreateFsLayers`: `--source=<dir>` or `BLUR_SOURCE`, default the working
directory (retail behaviour). PC data becomes the working directory; any other
platform is reported and the game exits with code 3 until its data layer
exists (board tasks T070+). An Xbox 360 source now mounts (below).

## Xbox 360 paks (T071)

An Xbox 360 source (big-endian `cache/root.pak` beside an `.xex`) gets the
Xenon build's mounts in `CreateFsLayers`: `root.pak` and `root_streaming.pak`
at `/`, `shaders-speed.pak`, secure `gamedata.pak`, and the `gamedata_boot.pak`
RAM pak. The engine's own pak reader handles the byte order; the XMem codecs
it needs run on `code/vendor/xmem-pc` (the XDK xcompress API over libmspack's
LZX decoder in `code/vendor/libmspack`, LGPL 2.1, built with
`BLUR_XMEM_LZX`). After mounting, the game stops with exit code 3: the chunk
files inside are still 360 format (T072).

Checks, both against Blur2Test's extractions (`_work/disc_cache/<disc>`):

- `code/tools/xmem-verify/xmem_verify.cpp`: standalone, decodes every entry of
  one pak and matches it by SHA-1 (build line at the top of the file).
- `BLUR1MULTI_PAKDUMP=<list>`: the game reads each listed virtual path through
  its mounted file system and writes it to `<list dir>/dump/`.
  `code/tools/xmem-verify/pakdump_compare.py list|compare` makes the list and
  checks the dump; `_work/run/run_dump.ps1 -Source <dir> -List <list>` runs it
  minimised.

## Xbox 360 asset conversion (T072)

For a 360 source, `CreateFsLayers` wraps the mounted file system in
`X360Convert::C_ConvertFs` (`code/mercury/library/util/x360convert/engine`):
opening a file whose type the converters handle returns the PC bytes.
Converters live in `code/mercury/library/util/x360convert/source` (no engine
dependencies) and are checked against retail PC with
`code/tools/x360convert-verify` over extracted retail paks
(`xmem_verify --extract`, `paklist_pc.py --extract-all`):

```
x360convert_verify <x360 extract root> <pc extract root> [--ext .texture] [--write <dir>]
```

Done: `.texture`, `.font`, `.particlemesh`, XT flasks (`.bin`/`.xt`), resource bundles (`.bpr`), localisation (`.loc`), animation cycles and skeletons (`.cycle`, `.skeleton`, through the engine's own readers in `engine/EngineConverters.cpp`). By
default a 360 source still stops after mounting; `BLUR1MULTI_X360_CONTINUE=1`
boots on (`_work/run/run_x360.ps1`). Details and results: docs/analysis.md
"Xbox 360 -> PC conversion".

## Automated runs

`_work/run/run_script.ps1 -Script scripts/escape.txt [-Source <dir>]` starts
the exe minimised with `BLUR2PC_AUTOMATION` (AGENTS.md rule 7) and
`--source=<dir>`.
