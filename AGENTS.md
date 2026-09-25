# AGENTS.md

This file provides guidance to agents when working with code in this repository.

## What this repository is

Blur2PC turns the leaked Blur (2010, Bizarre Creations, PC) C++ source into a PC build that plays whatever is recoverable from five Xbox 360 prototype discs of the cancelled Blur 2 (codename **Kanzen**). The Blur 1 path is kept working throughout; the long-term goal is one executable that runs any Blur game (Blur 1, the Blur 2 prototypes) from any platform's data (PC, Xbox 360, PS3), see decision D09. Two pieces are new: an **asset inventory manager** that knows every asset on every disc and serves each one from the best source, and a **per-disc shader recompilation cache**. The engine only ever reads PC-format data: disc assets are converted to PC format on the fly as they are opened (D11), and only shaders, which cannot be recompiled at load time, are cached per source.

This is a multi-session, multi-agent project. **Start every session by reading `docs/BOARD.md` and the last few entries of `docs/LEDGER.md`. End every session by appending a ledger entry.** The protocol is at the bottom of this file.

## Layout

Three phase folders (see "Project stages/layers"), one shared docs folder, one shared scratch folder. Since 2026-09-24 (D10); before that the Blur1PC contents were at the repo root, which is what older ledger entries and analysis notes mean by `code/`.

| Path | What |
|---|---|
| `Blur1PC/` | **Phase 1**: Blur 1 from retail PC data. The source tree, its CMake build, its launchers (`play.bat`, `Blur.lnk`) and its `_work/` (build output, the user's exe). |
| `Blur1PC/code/` | Blur 1 PC source snapshot (~2010-06-29). Pristine copy at `C:\AIProjects\Blur\code`; `diff -rq` against it shows every port change. |
| `Blur1PC/code/amax/` | The game: `game/` (entry `Main.cpp`, `GameApp.cpp`, `GameSystems.cpp`), `library/` (Ui, RaceBook, Network, Kanzen cameras), `tools/` (asset converters). |
| `Blur1PC/code/mercury/library/` | The engine: `common/` (system, xt2 gamedata, btl containers), `util/` (logicalFs, resource, input), `graphics/`, `physics/`, `gamesys/`, `ai/`, `audio/`, `ui/`. |
| `Blur1PC/code/mercurygame/` | Older tech-test game shell, superseded by `amax/`. |
| `Blur1PC/code/tools/` | Build farm, bizpak (pak tool), jam sources, exporters; ours: `jam2cmake/` (build generators, used by every phase) and `ghidra_retail/` (retail Blur.exe decompilation scripts). |
| `Blur1PC/code/vendor/` | demonware, lua, zlib, libtom*, cxxtest, tclap; our shims (dxerr, bink-stub, efx-util, crt-compat, crash-handler). |
| `Blur1PC/cmake/` | `blur_common.cmake` (the build description every phase includes), `overrides.cmake` (hand tweaks), `targets.cmake` (generated, never edited). |
| `Blur1Multi/` | **Phase 2**: Blur 1 from any platform's data: retail PC, Xbox 360 and PS3. Holds no engine copy: it builds `Blur1PC/code` with `Blur1Multi/code` laid over it. Read `Blur1Multi/README.md` before touching it. |
| `Blur2/` | **Phase 3**: the Blur 2 prototypes. Placeholder until phase 2 serves 360 data. |
| `docs/` | Project docs for all phases: analysis, decisions, plan, board, ledger. |
| `_work/` | Shared scratch (gitignored): `vendor/` (third-party SDKs), `ghidra/`, `ghidra_proj/`, `retail_lua*/`, `tools/`. Each phase has its own `<Phase>/_work/`: `build/` (targets JSON, generated units, VS tree), `run/` (the user's exe, automation harness, `shadercache/<source>/` recompiled shaders). No other converted data is stored (D11). |

External inputs (never modified, never copied into the repo):

- Prototype discs: `C:\AIProjects\Blur\GameData\Blur 2 X360 prototypes\` (Aug 31, Nov 4, Nov 9, Nov 10, Nov 25 DAMAGED).
- Retail Blur 1 X360: `C:\AIProjects\Blur\GameData\Xbox 360\`.
- Retail Blur 1 PS3: `C:\AIProjects\Blur\GameData\PlayStation 3\` (disc root; data under `PS3_GAME\USRDIR`). Phase 2 plays it; formats in `docs/analysis.md` "PS3 -> PC conversion".
- Retail Blur 1 PC: `C:\Program Files (x86)\Activision\Blur(TM)` (native little-endian reference for every format the source reads).
- X360/Blur 2 extraction tooling and format knowledge: `C:\AIProjects\Blur2Test` (Python 3.12 + QuickBMS; pak, texture, model, level, KSLF/XT and audio decoders; XenosRecomp for shaders; per-disc catalogues in `_work\archive_index\`). Read its `AGENTS.md` and `_notes\disc_resource_map.md` before writing any decoder.

## Project stages/layers

We're dividing the project's lifetime into stages/layers.
1. **PC Blur 1**: Get a 100% faithful reproduction of the retail Blur. During this stage, it is of the utmost importance that any corrections we make will 100% be made based on decompiling Blur 1 PC retail code, especially if it's for issues that are known not to exist in the retail game. This decompile will mainly be performed using Ghidra. For any code extracted using this method, for which we have no symbols, try your best to give code entities proper names to try to keep the code human-readable, adding a tag to each of them indicating this fact.
2. **Multiplatform Blur 1**: Get the project to run on Xbox 360 and PlayStation 3 retail data (PS3 support added 2026-09-25, board task T076). See game source abstraction below. For this stage #1 priority is not to reinvent the wheel as most if not all the asset loading from X360 data is already resolved by Blur2Test.
3. **Blur 2 support**: Get the project to run on Blur 2 prototype data (see asset catalog section below)
Each of them will be organized and implemented in such a way that they're independent layers, and at any given time the upper ones can be removed with no interference to the lower ones.
For example, we should have a 100% faithful Blur 1 PC source, then a standalone version that's able to run just Blur 1 from PC and Xbox with no sign of Blur 2 support.

**How the layers are kept apart (D10).** Each phase is a folder with its own CMake project, `_work/` and exe. A phase never edits the folder below it: Blur1Multi compiles `Blur1PC/code` as a read-only base with its own `Blur1Multi/code` laid over it (a file at the same relative path replaces the base file, a new path is a new file declared in `Blur1Multi/cmake/multi.cmake`), and Blur2 will do the same over both. Rules that follow:
- A fix for Blur 1 on PC data goes in `Blur1PC/`, and every phase inherits it at its next generate.
- Code that exists only for another platform or for Blur 2 goes in that phase's folder, as new files where possible. An overlay copy of a base file is the last resort: it stops inheriting Blur1PC changes to that file, so it is recorded in `<Phase>/overlay.json` (`python tools/overlay.py take ...`) and `python tools/overlay.py check` must report no drift before a build is called done. When it reports drift, merge the base change into the copy (`overlay.py changes <relpath>` lists the Blur1PC commits and diff since the last sync) and record it with `overlay.py sync <relpath> "<what was merged>"`: each copy keeps the Blur1PC commit and date it was last synced to, and its full take/sync history, so later questions of the form "what has this copy absorbed" are answered from `overlay.py history`.
- Phase-specific translation units can test `BLUR1MULTI` (phase 2); base files must not, since the base must build without it.
- Deleting a phase folder must leave the phases below building and running unchanged.

## Future goals

In the future we'll want to:
* Move the project to 64-bit compilation
* Implement HDR support
* Move to a newer version of DirectX (probably 11 to avoid unnecessary complications)

## Building

The build is CMake generated from the original jamfiles (decision D08). Each phase builds from its own folder. **Blur1PC**, from `Blur1PC/`, in order:

```
python code/tools/jam2cmake/jamextract.py --top . --root code/amax/game/jamfile.jam --target WIN32 --platform PC --build Speed --out _work/build/targets_pc_speed.json
python code/tools/jam2cmake/cpgen.py $(find code/mercury/library code/amax/game -iname "*.cp" -o -iname "*.cv" -o -iname "*.sp")
python code/tools/jam2cmake/shadergen.py --json _work/build/targets_pc_speed.json --out _work/build/gen/shaders
python code/tools/jam2cmake/luabundle.py --json _work/build/targets_pc_speed.json --out _work/build/gen/blobs/amax_lua_bundle.cpp
python code/tools/jam2cmake/gen_cmake.py --json _work/build/targets_pc_speed.json --out cmake/targets.cmake
cmake -S . -B _work/build/vs -G "Visual Studio 18 2026" -A Win32
cmake --build _work/build/vs --config Release --target Amax -- -m -v:m -nologo > _work/build/amax.log 2>&1
python code/tools/jam2cmake/errsum.py _work/build/amax.log
```

**Blur1Multi**, from `Blur1Multi/`: the same generators from `../Blur1PC/code/tools/jam2cmake/` with `--top ../Blur1PC`, `--overlay .` (shadergen, luabundle, gen_cmake) and `--project-dir .` (gen_cmake); the exact lines are in `Blur1Multi/README.md` and at the top of `Blur1Multi/CMakeLists.txt`. Run `gen_cmake.py` again after taking or removing an overlay file. `cpgen.py` runs in Blur1PC only (its headers land beside the `.cp` files).

- `cmake` lives at `C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe`; MSBuild prints its summary in the system locale (Spanish), so judge builds by `errsum.py`'s error count.
- Configurations map to the engine's build configs: Debug → `BUILD_CONFIG_DEBUG`, Release → `BUILD_CONFIG_SPEED`, RelWithDebInfo → `BUILD_CONFIG_RELEASE`, MinSizeRel → `BUILD_CONFIG_MASTER`.
- Jam's `[ Batch … : StdAfx.h ]` groups were **unity translation units**; the generator reproduces them as `_work/build/gen/unity/<target>_batchN.cpp`. A file that fails alone but built originally is almost always relying on an earlier file in its group. Run `gen_cmake.py` again after adding or removing sources.
- `cmake/targets.cmake` is generated; never edit it. Per-file or per-target tweaks go in `Blur1PC/cmake/overrides.cmake` (every phase includes it) with a comment saying why; the shared flags, vendor SDKs and shims are in `Blur1PC/cmake/blur_common.cmake`. Source edits in `Blur1PC/code/` get a `// [Blur2PC]` comment; Blur1Multi files get `// [Blur1Multi]`.
- Third-party inputs live in the shared `_work/vendor` at the repo root (D3DX 9.29 from the `Microsoft.DXSDK.D3DX` NuGet, OpenAL Soft 1.24.3 binaries, IJG libjpeg 6b); `blur_common.cmake` has cache variables for each. Shims for pieces we do not have: `code/vendor/dxerr` (DXGetErrorString) and `code/vendor/efx-util` (Creative's reverb-preset conversion).
- Built-in shaders (`.psh`/`.vsh`, 647 variants) are compiled by `shadergen.py` with the Windows SDK `fxc` into `_work/build/gen/shaders/<target>_shaders.cpp` in the blob layout `C_D3D9Device::CreatePixelShader` reads. Re-run it after editing any of them.

What the tree shipped with, for reference:

- `code/amax/game/amax.sln` is a VS2008 (vcproj 9.00) **makefile project**. Its build command is `z:\tools\common_bin\jam -sTARGET=WIN32 $(JAMOPTS) -sBUILD=<Debug|Release|Speed|Profile|Master>`, output `bin\Win32\<Build>\Amax.exe`. Targets: WIN32, XBOX_360, PS3.
- Jam sources are in `code/tools/jam` and `code/tools/jamplus`, but the custom rules the jamfiles call (`SupportedTargets`, `IncludePath`, `Define`, `CheckXDKVersion`, `LINKLIBS`, `PCLIBS`) are **not in the tree**; they lived in Bizarre's Jambase.
- Required externals that are **missing on this machine**: `$(TOP)/libs/vendor` (Bink 1.9r, OpenAL 1.1, Massive ad client), `$(DXSDK_PATH)` (DirectX SDK: d3dx9, dinput8, xinput, dxerr), VS2008. Demonware libs for win32-vs8 are present under `code/vendor/demonware`.
- Source lists and defines per target are authoritative in the jamfiles: `code/amax/game/jamfile.jam` (plus `graphics.jam`, `RaceLogic.jam`, `air_amax.jam`, `demonware.jam`) and `code/mercury/library/jamfile.jam`. `code/tools/jam2cmake/jamextract.py` evaluates them (see `docs/analysis.md` §1 for the command) and writes `_work/build/targets_pc_<build>.json`: 70 targets, 2,449 sources. Generate build files from that JSON; do not hand-maintain source lists.
- The build must define exactly one of `BUILD_CONFIG_DEBUG | RELEASE | PROFILE | SPEED | MASTER`; `core/Core.h` derives the `CFG_*` macros and `PLATFORM_PC` from it and `_WIN32`.
- PC preprocessor identity: `PLATFORM_PC` (the vcproj adds `_MSC_VER=1400`). Graphics is forced to D3D9 + HLSL in `code/mercury/library/graphics/display/include/display/device/DeviceDefines.h`.
- Tests: cxxtest in `code/vendor/cxxtest`; engine test sections in `code/mercury/library/gamesys/testsections` (`TestSectionFs.cpp` shows how pak, memory and overlay mounts combine). No test runner is wired yet.

## Architecture (what you need across many files)

Paths in this section and in `docs/analysis.md` are relative to `Blur1PC/`.

**Layering.** `amax` (game) → `mercury/library` (engine) → `common` + `vendor`. Namespaces map to folders: `Sys::` (common/system), `LogicalFs::` (util/logicalFs), `Resource::` (util/resource), `GameResource::` (gamesys/game_resource), `WorldRender::` and `RenderingData::` (graphics/world_render, renderingdata), `FX::` (graphics/fx), `Display::` (graphics/display), `Game::` (amax/game), `Xt`/`XtDb` (common/xt2).

**All file I/O goes through one virtual file system.** `Sys::GetDefaultFileSystem()` returns a `LogicalFs::C_OverlayFs` (util/logicalFs/overlayfs). Layer 0 is the loose working directory; paks are mounted as layers in `C_GameSystems::CreateFsLayers` (`code/amax/game/GameSystems.cpp` ~201-265): `cache/root.pak` at `/`, `cache/models.pak` at `/models/`, `cache/shaders-speed.pak` at `/graphics/shaders/speed/`, `cache/gamedata.pak` at `/gamedata/` (RSA-signed; loose files bypass the check). Layer types: Overlay (first hit wins, top-down), Opaque, Newer (timestamp). **`C_OverlayFs::GetBestLayer()` is the single choke point** for "which source serves this path"; the inventory manager plugs in there or as a new `I_FileSystem` layer. Pak lookup is a CRC32 (poly 0xEDB88320, init 0xFFFFFFFF, no final XOR) of the lowercase path with a leading separator. `tools/bizpak` builds paks. Data root is the process cwd; there is no command-line or config override.

**Resources.** `Resource::C_Manager` (util/resource) keys resources by normalised filename, ref-counts, loads on `C_LoadingThread`, then `PostLoadResourceData` on the main thread. Textures, models, levels, skeletons and cycles go through `C_ResourceBase::OpenResourceFile`. Fonts, UI, loc, XT, display textures and FX bypass the manager but still use the same overlay FS.

**Chunk files.** Every asset starts with a platform FourCC (`PC  `, `360 `, `PS3 `; see common/system `Platform.h`) followed by nested `Sys::C_FileBlock` chunks (`C_BlockHeader` = 8-byte id + U32 size + U16 count). `GameResource::ValidatePlatformId` rejects the wrong platform and readers never endian-swap, so **360 disc data must be converted to PC little-endian files before the engine can read it.** Chunk `Header` versions are only used for `>=` branches; the Arch archive (`misc_shared/archive/binaryarchive.h`) asserts `version <= max` in debug.

**Level pipeline.** `C_LevelState::Finished` (amax/game/GameLogic/Structures) builds `levels\<citytag>\level.level` and `gamedata\levels\<citytag>\` from the XT City record. `C_AmaxWorldLayer::Create` loads the level resource, the AI data (`<route>.aid`), instantiates it, then `LoadActorStudioActors` opens `actorpack.xt`. `C_LevelResource` (gamesys/game_resource) reads `.level` (Level: Header/MdlDat/NavDat) and, by suffix, `.levelphysics` (`C_PhysicsLevel`), `.levelrender` (`WorldRender::C_RenderLevel::Read`: Header/Scene/Lights/LS/PVSData/CubePt2/Portals/Crowd) and `.levelstream` (opened for `StreamManager`; chunked reads decompressed by scheduler jobs). Level geometry and textures are baked into the render/stream files and referenced by `ResourceIndex{block,index}`; props, vehicles and shared textures are loaded by path string through XT `GameModel` / `GameTexture` records.

**XT gamedata.** `XtPackOpen` loads self-describing "flask" databases (`gamedata/xt/*.bin`, tag `FLSK`, versions 1-2). The pack list is hard-coded in `C_RaceBookLayer` (amax/game/GameLogic/Layers/RaceBookLayer.cpp ~277-298) through `GameXtLoader::AddPack`. Cities, routes and vehicles are data-driven records (`AmaxCity`, `AmaxRoute`, `AmaxVehicle` in `GameData.cpp`) collected from any loaded pack and keyed by `GAMEDATA_ID` or tag. The source never opens `Tracks.bin`, `Levels.*.bin` or `Ui.bin`, which every disc ships, and it has no `.levelanim` loader.

**Shaders.** `FX::Manager::LoadEffectInternal` (graphics/fx) opens `.fxb` effects from `graphics\shaders\speed\`; `ImportFXB` expects a `PCFX` block of D3D9 bytecode. There is no runtime HLSL compile and no on-disk cache. The discs carry Xenos microcode only (FXB2 / XEFX v9 on Aug 31, v12 on Nov). The per-disc shader cache is: microcode → XenosRecomp HLSL → fxc SM3 → PC `.fxb`, keyed by disc + microcode hash.

**Timing.** `C_GameWorld::CalcNumberOfSteps` (amax/game/Game/GameWorld/GameWorld.cpp ~2438) accumulates frame time into 1/60 s steps; the step loop clamps at `Game.MaxPhysicsUpdates` (2 or 4). Car physics integrates a fixed 1/60 s per step, so the simulation stays at 60 Hz. On PC, `C_Application::IdleLoop` runs the game update at 60 Hz ticks and renders every loop with matrix and camera interpolation between the last two ticks (details in `docs/analysis.md`, "Frame rate").

## Disc generations (the fact that shapes everything)

| Generation | Discs | Paks | Formats | Audio |
|---|---|---|---|---|
| A (Blur 1 era) | Aug 31 | v1, retail cache layout (boot, models, root, root_streaming, rampaks) | Identical to retail Blur 1: render header 0x1a `LNSc`, texture header 0x18, model 4 chunks | 330 MB loose `.baf` |
| B (Kanzen) | Nov 4, 9, 10, 25 | v3 root/gamedata/shaders only; `root_streaming.pak` empty; levelstreams loose | render `Scen`, texture header v1 + `360 Data`, model 5 chunks, new `.levelanim` | Nov 4 and Nov 9 only |

Nov 25 is a bad rip: pak payloads truncated past a point (all 48 `.bin` databases lost), zero-filled 32 KB blocks in several level files, 60-byte stub `.levelanim` files. It alone has `mp_la_docksbowl`, `user_feargal_p` and a full `liverpool`. Full details and per-disc counts: `docs/analysis.md`.

## Game source abstraction

From the "multi-platform Blur 1" layer and upwards, we'll want to have a component that creates an abstraction layer for a game's disc source, independent of platform, and allows the game to play off of it.

Started 2026-09-24 in `Blur1Multi/code/mercury/library/util/gamesource/` (`GameSource::Select`: `--source=<dir>` or `BLUR_SOURCE`, platform detection from `cache/root.pak` byte order plus `default.xex` / `PS3_GAME`). The platform data layers behind it are board tasks T071-T076; design in D11. Xbox 360 and PS3 sources share one converting layer (`X360Convert::C_ConvertFs`, told the source with `X360Convert::SetSource`): the formats both consoles write alike (chunk trees, XT flasks, loc, animation, audio objects, the scene grammar) go through the same code; textures, audio banks and effects have per-console converters (D14).

## Asset catalog

Extending on the game source abstraction, when the time comes to consume from the 5 Blur 2 prototype discs, we'll want to have a component that we can ask for game assets and it will deliver them from the disc that's a better fit.
For this, the component will hold a persisted catalog of every asset present in all of the game discs, which it will use for the decision.

## Shader cache

For non-PC platforms, the first time the game is run on a given source, the game's runtime will perform a disassemble+recompile of the shaders to run on PC.
Recompiled shaders will be cached per each game source alongside the executable.

## Conventions

- New engine code lives beside what it extends (the inventory layer under `code/mercury/library/util/logicalFs/`, for example), inside the phase folder it belongs to (`Blur1Multi/code/mercury/library/util/gamesource/` is the model). New tools live under `code/tools/<name>/`. Do not restructure the source tree.
- Build output, the shader cache and scratch output go under the phase's `_work/` (gitignored). Other disc assets are converted in memory when opened and never stored (D11). Never write into the disc folders.
- The C++ source's own serialisers are the ground truth for PC formats; Blur2Test decoders are the ground truth for 360 formats. When they disagree, record it in `docs/analysis.md`.
- Blur2Test's rule also applies here: never guess a value to make something load. Record unknowns as unknowns.
- **Any reported gameplay or engine issue gets fixed by replicating retail's actual code 100% faithfully.** Find the real mechanism (source diff against the pristine snapshot, or Ghidra against the retail binary per `code/tools/ghidra_retail`) and port it exactly. Do not invent a workaround, approximation, or "close enough" substitute. If the real mechanism cannot be recovered (for example, it lives inside DRM-obfuscated code with no source equivalent), say so explicitly, record what part is and isn't verified in `docs/analysis.md`, and treat any substitute as a stopgap, not a fix.

## Multi-session / multi-agent protocol

1. **Read** `docs/BOARD.md` (open tasks and who holds them) and the tail of `docs/LEDGER.md` (what just happened) before doing anything.
2. **Claim** a task on the board before working on it: set `Owner` to your session or agent name and `Status` to `in progress`. One task per agent at a time. Parallel agents claim disjoint tasks and touch disjoint files; list the files you expect to touch in the task row.
3. **Decisions** that change direction go in `docs/decisions.md` as a new numbered entry with status `proposed` until the user confirms. Never silently reverse an `accepted` decision.
4. **Facts** discovered about the source, the discs or the formats go in `docs/analysis.md`, not in the ledger.
5. **Finish** by appending a ledger entry (template at the top of that file): what changed, what was verified and how, what is blocked, and the exact next step. Then update the board row. If you stop mid-task, say where in the ledger so the next agent can resume.
6. Commit only when the user asks, and commit your changes only, staging by hunk if needed.
7. **Do not launch the game in foreground** Every time you launch a foreground process you're interrupting anything the user is doing. Launch the game minimized, never hidden: a minimized window stays on the taskbar, so the user can spot a stray session. `<Phase>/_work/run/run_script.ps1` does this (the game shows its window minimized without activation while `BLUR2PC_AUTOMATION` is set, and keeps rendering).
8. **Never kill processes you don't own**.
9. **One exe per phase for the user: `<Phase>/_work/run/Amax.exe`** (`Blur1PC/_work/run/Amax.exe`, `Blur1Multi/_work/run/Amax.exe`). It is what that phase's `play.bat` and `Blur.lnk` start, so it is the only place the user tests that phase. Build with the standard command into `<Phase>/_work/build/vs/Release`, then copy `Amax.exe` and `Amax.pdb` to `<Phase>/_work/run`. If that `Amax.exe` is running, wait for it to exit and then copy. Do not build or copy to another output directory, a renamed exe or a private folder because the usual one is busy. A private copy for automated runs is fine, but it does not count as delivery. Do not tell the user a change is ready to test until that phase's `_work/run/Amax.exe` contains it; if you have to stop before then, say so plainly.