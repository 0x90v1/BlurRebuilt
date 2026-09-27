# AGENTS.md

This file provides guidance to agents when working with code in this repository.

## What this repository is

This is a multi-session, multi-agent project. **Start every session in the worktree of the phase you work on, and read that phase's board and the last few entries of its ledger: Blur 1 PC in `Blur1PC/docs/` (`BOARD.md`, `LEDGER.md`), each other phase in its own worktree's `docs/<phase>/` (`Blur1Multi/docs/blur1multi/`, `BlurX/docs/blurx/`, `Blur1PCDX11/docs/blur1pcdx11/`). End every session by appending an entry to that ledger.** The protocol is at the bottom of this file.
Before starting work on any of the stages, see to read the goal statement an definitions in the "Project stages/layers" section below.

## Layout

`C:\AIProjects\Blur2PC\` is a container: a bare git repository (`.git`) and one worktree per branch, one branch per phase (D16, since 2026-09-25). Every worktree has the same layout, `code/` at its root. Before D16 (2026-09-24 to 2026-09-25, D10) the phases were folders of one working tree stacked at build time, and before D10 the Blur1PC contents were at the repo root; older ledger entries and analysis notes use those paths.

| Worktree | Branch | What |
|---|---|---|
| `Blur1PCSource/` | `master` | The leaked Blur 1 PC source snapshot (~2010-06-29), byte for byte as at `C:\AIProjects\Blur\code`. Never edited. `git diff master blur1pc` is every port change. |
| `Blur1PC/` | `blur1pc` | **Phase 1**: Blur 1 from retail PC data. The port of `master`: the source tree, the CMake build, the launchers (`play.bat`, `Blur.lnk`), `docs/`, this file, `tools/`, and its `_work/` (build output, the user's exe, and the scratch every phase shares: `vendor/`, `ghidra/`, `ghidra_proj/`, `retail_lua*/`). |
| `Blur1Multi/` | `blur1multi` | A higher phase; see its own AGENTS file. |
| `BlurX/` | `blurx` | A higher phase; see its own AGENTS file. |
| `Blur1PCDX11/` | `blur1pcdx11` | A higher phase; see its own AGENTS file. |

Inside a worktree:

| Path | What |
|---|---|
| `code/` | The source tree. `amax/`: the game (`game/`: entry `Main.cpp`, `GameApp.cpp`, `GameSystems.cpp`; `library/`: Ui, RaceBook, Network, Kanzen cameras; `tools/`: asset converters). `mercury/library/`: the engine (`common/`: system, xt2 gamedata, btl containers; `util/`: logicalFs, resource, input; `graphics/`, `physics/`, `gamesys/`, `ai/`, `audio/`, `ui/`). `mercurygame/`: older tech-test shell, superseded by `amax/`. `tools/`: build farm, bizpak, jam sources, exporters; ours: `jam2cmake/` (build generators), `ghidra_retail/`, and the phases' verification tools. `vendor/`: demonware, lua, zlib, libtom*, cxxtest, tclap; our shims (dxerr, bink-stub, efx-util, crt-compat, crash-handler). |
| `cmake/` | `blur_common.cmake` (the build description), `overrides.cmake` (hand tweaks), `targets.cmake` (generated, never edited). |
| `CMakeLists.txt` | The same file on every branch except its marked phase lines. |
| `docs/` | Blur 1 PC's docs (analysis, decisions, plan, board, ledger), about Blur 1 on PC only. Each higher phase keeps its own in `docs/<phase>/` on its branch (`docs/blur1multi/`, `docs/blurx/`, `docs/blur1pcdx11/`), which exists on that branch and the ones above it only. |
| `tools/` | `build.sh` (regenerate and build the phase this worktree holds). |
| `_work/` | Scratch (gitignored, so never inherited): `build/` (targets JSON, generated units, VS tree), `run/` (the user's exe, automation harness). Third-party SDKs are read from `Blur1PC/_work/vendor` by every phase (`blur_common.cmake`). |

External inputs (never modified, never copied into the repo):

- Retail Blur 1 PC: `C:\Program Files (x86)\Activision\Blur(TM)` (native little-endian reference for every format the source reads).

## Project stages/layers

We're dividing the project's lifetime into stages/layers.
1. **PC Blur 1**: Get a 100% faithful reproduction of the retail Blur. During this stage, it is of the utmost importance that any corrections we make will 100% be made based on decompiling Blur 1 PC retail code, especially if it's for issues that are known not to exist in the retail game. This decompile will mainly be performed using Ghidra. For any code extracted using this method, for which we have no symbols, try your best to give code entities proper names to try to keep the code human-readable, adding a tag to each of them indicating this fact.
Each of them will be organized and implemented in such a way that they're independent layers, and at any given time the upper ones can be removed with no interference to the lower ones.

**How the layers are kept apart (D16).** Each phase is a git branch checked out in its own worktree, with its own CMake project, `_work/` and exe: `master` (untouched source) → `blur1pc` → `blur1multi` → `blurx`, each the one below plus its own commits. Nothing flows down; a lower branch reaches the ones above by `git merge`. Rules that follow:
- **Work in the worktree of the phase the change belongs to.** A fix for Blur 1 on PC data is committed on `blur1pc` in `Blur1PC/`, then merged down: `git merge blur1pc` in `Blur1Multi/`, then `git merge blur1multi` in `BlurX/`. Whoever commits on a lower branch does the merges as part of finishing the task; merges only, never rebases (shared, long-lived branches).
- **Where a fault belongs is decided by reproduction**: the lowest phase whose exe shows it on its own data owns it (`Blur1PC/_work/run/Amax.exe` on retail PC data). A base file that compiles only because a unity batch-mate includes its header is the base's fault even if only a higher phase's batches expose it. Record it on the board (`Found in` / `Belongs to`), fix it in the owning worktree, merge down. **Never patch a lower phase's fault in a higher branch**: the patch conflicts with the real fix when it merges, and hides the bug from that phase. If a stopgap is unavoidable it lives in a file that already belongs to the higher phase, marked as such, with a board task to remove it.
- Deleting a branch (and its worktree) must leave the branches below building and running unchanged.
- Each phase has its own docs (user, 2026-09-27) and documents only what it adds to the phase below it: `Blur1PC/docs/` holds nothing but Blur 1 PC (nothing about the Blur 2 prototypes, Xbox 360 or PS3); Blur1Multi, BlurX and Blur1PCDX11 facts, decisions, board rows and ledger entries go in that phase's `docs/<phase>/`, edited in that phase's worktree. A phase's folder merges up with its branch like any other file. This file and `tools/` are edited in `Blur1PC/` only; the copies in the other worktrees are as of their last merge.

## Future goals

In the future we'll want to:
* Move the project to 64-bit compilation
* Implement HDR support
* Move to a newer version of DirectX (probably 11 to avoid unnecessary complications)

## Building

The build is CMake generated from the original jamfiles (decision D08). Each phase builds from its own worktree with the same commands (`tools/build.sh` runs them all), in order:

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

Run `gen_cmake.py` again after adding or removing sources, including after a merge that does, and commit the regenerated `cmake/targets.cmake`. `cpgen.py`'s headers land beside the `.cp` files and are gitignored, so it runs in every worktree.

- `cmake` lives at `C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe`; MSBuild prints its summary in the system locale (Spanish), so judge builds by `errsum.py`'s error count.
- Configurations map to the engine's build configs: Debug → `BUILD_CONFIG_DEBUG`, Release → `BUILD_CONFIG_SPEED`, RelWithDebInfo → `BUILD_CONFIG_RELEASE`, MinSizeRel → `BUILD_CONFIG_MASTER`.
- Jam's `[ Batch … : StdAfx.h ]` groups were **unity translation units**; the generator reproduces them as `_work/build/gen/unity/<target>_batchN.cpp`. A file that fails alone but built originally is almost always relying on an earlier file in its group. Run `gen_cmake.py` again after adding or removing sources.
- `cmake/targets.cmake` is generated; never edit it. Per-file or per-target tweaks go in `cmake/overrides.cmake` (on `blur1pc`, so every phase inherits it) with a comment saying why; the shared flags, vendor SDKs and shims are in `cmake/blur_common.cmake`. Source edits on `blur1pc` get a `// [Blur2PC]` comment.
- Third-party inputs live in `Blur1PC/_work/vendor` (D3DX 9.29 from the `Microsoft.DXSDK.D3DX` NuGet, OpenAL Soft 1.24.3 binaries, IJG libjpeg 6b); `blur_common.cmake` has cache variables for each and the other phases resolve them there. Shims for pieces we do not have: `code/vendor/dxerr` (DXGetErrorString) and `code/vendor/efx-util` (Creative's reverb-preset conversion).
- Built-in shaders (`.psh`/`.vsh`, 647 variants) are compiled by `shadergen.py` with the Windows SDK `fxc` into `_work/build/gen/shaders/<target>_shaders.cpp` in the blob layout `C_D3D9Device::CreatePixelShader` reads. Re-run it after editing any of them.

What the tree shipped with, for reference:

- `code/amax/game/amax.sln` is a VS2008 (vcproj 9.00) **makefile project**. Its build command is `z:\tools\common_bin\jam -sTARGET=WIN32 $(JAMOPTS) -sBUILD=<Debug|Release|Speed|Profile|Master>`, output `bin\Win32\<Build>\Amax.exe`. Targets: WIN32.
- Jam sources are in `code/tools/jam` and `code/tools/jamplus`, but the custom rules the jamfiles call (`SupportedTargets`, `IncludePath`, `Define`, `CheckXDKVersion`, `LINKLIBS`, `PCLIBS`) are **not in the tree**; they lived in Bizarre's Jambase.
- Required externals that are **missing on this machine**: `$(TOP)/libs/vendor` (Bink 1.9r, OpenAL 1.1, Massive ad client), `$(DXSDK_PATH)` (DirectX SDK: d3dx9, dinput8, xinput, dxerr), VS2008. Demonware libs for win32-vs8 are present under `code/vendor/demonware`.
- Source lists and defines per target are authoritative in the jamfiles: `code/amax/game/jamfile.jam` (plus `graphics.jam`, `RaceLogic.jam`, `air_amax.jam`, `demonware.jam`) and `code/mercury/library/jamfile.jam`. `code/tools/jam2cmake/jamextract.py` evaluates them (see `docs/analysis.md` §1 for the command) and writes `_work/build/targets_pc_<build>.json`: 70 targets, 2,449 sources. Generate build files from that JSON; do not hand-maintain source lists.
- The build must define exactly one of `BUILD_CONFIG_DEBUG | RELEASE | PROFILE | SPEED | MASTER`; `core/Core.h` derives the `CFG_*` macros and `PLATFORM_PC` from it and `_WIN32`.
- PC preprocessor identity: `PLATFORM_PC` (the vcproj adds `_MSC_VER=1400`). Graphics is forced to D3D9 + HLSL in `code/mercury/library/graphics/display/include/display/device/DeviceDefines.h`.
- Tests: cxxtest in `code/vendor/cxxtest`; engine test sections in `code/mercury/library/gamesys/testsections` (`TestSectionFs.cpp` shows how pak, memory and overlay mounts combine). No test runner is wired yet.

## Architecture (what you need across many files)

Paths in this section and in `docs/analysis.md` are relative to a phase worktree (the layout is the same in all of them).

**Layering.** `amax` (game) → `mercury/library` (engine) → `common` + `vendor`. Namespaces map to folders: `Sys::` (common/system), `LogicalFs::` (util/logicalFs), `Resource::` (util/resource), `GameResource::` (gamesys/game_resource), `WorldRender::` and `RenderingData::` (graphics/world_render, renderingdata), `FX::` (graphics/fx), `Display::` (graphics/display), `Game::` (amax/game), `Xt`/`XtDb` (common/xt2).

**All file I/O goes through one virtual file system.** `Sys::GetDefaultFileSystem()` returns a `LogicalFs::C_OverlayFs` (util/logicalFs/overlayfs). Layer 0 is the loose working directory; paks are mounted as layers in `C_GameSystems::CreateFsLayers` (`code/amax/game/GameSystems.cpp` ~201-265): `cache/root.pak` at `/`, `cache/models.pak` at `/models/`, `cache/shaders-speed.pak` at `/graphics/shaders/speed/`, `cache/gamedata.pak` at `/gamedata/` (RSA-signed; loose files bypass the check). Layer types: Overlay (first hit wins, top-down), Opaque, Newer (timestamp). **`C_OverlayFs::GetBestLayer()` is the single choke point** for "which source serves this path". Pak lookup is a CRC32 (poly 0xEDB88320, init 0xFFFFFFFF, no final XOR) of the lowercase path with a leading separator. `tools/bizpak` builds paks. Data root is the process cwd; there is no command-line or config override.

**Resources.** `Resource::C_Manager` (util/resource) keys resources by normalised filename, ref-counts, loads on `C_LoadingThread`, then `PostLoadResourceData` on the main thread. Textures, models, levels, skeletons and cycles go through `C_ResourceBase::OpenResourceFile`. Fonts, UI, loc, XT, display textures and FX bypass the manager but still use the same overlay FS.

**Chunk files.** Every asset starts with a platform FourCC (`PC  `; see common/system `Platform.h`) followed by nested `Sys::C_FileBlock` chunks (`C_BlockHeader` = 8-byte id + U32 size + U16 count). `GameResource::ValidatePlatformId` rejects the wrong platform and readers never endian-swap. Chunk `Header` versions are only used for `>=` branches; the Arch archive (`misc_shared/archive/binaryarchive.h`) asserts `version <= max` in debug.

**Level pipeline.** `C_LevelState::Finished` (amax/game/GameLogic/Structures) builds `levels\<citytag>\level.level` and `gamedata\levels\<citytag>\` from the XT City record. `C_AmaxWorldLayer::Create` loads the level resource, the AI data (`<route>.aid`), instantiates it, then `LoadActorStudioActors` opens `actorpack.xt`. `C_LevelResource` (gamesys/game_resource) reads `.level` (Level: Header/MdlDat/NavDat) and, by suffix, `.levelphysics` (`C_PhysicsLevel`), `.levelrender` (`WorldRender::C_RenderLevel::Read`: Header/Scene/Lights/LS/PVSData/CubePt2/Portals/Crowd) and `.levelstream` (opened for `StreamManager`; chunked reads decompressed by scheduler jobs). Level geometry and textures are baked into the render/stream files and referenced by `ResourceIndex{block,index}`; props, vehicles and shared textures are loaded by path string through XT `GameModel` / `GameTexture` records.

**XT gamedata.** `XtPackOpen` loads self-describing "flask" databases (`gamedata/xt/*.bin`, tag `FLSK`, versions 1-2). The pack list is hard-coded in `C_RaceBookLayer` (amax/game/GameLogic/Layers/RaceBookLayer.cpp ~277-298) through `GameXtLoader::AddPack`. Cities, routes and vehicles are data-driven records (`AmaxCity`, `AmaxRoute`, `AmaxVehicle` in `GameData.cpp`) collected from any loaded pack and keyed by `GAMEDATA_ID` or tag. The source never opens `Tracks.bin`, `Levels.*.bin` or `Ui.bin` and it has no `.levelanim` loader.

**Shaders.** `FX::Manager::LoadEffectInternal` (graphics/fx) opens `.fxb` effects from `graphics\shaders\speed\`; `ImportFXB` expects a `PCFX` block of D3D9 bytecode. There is no runtime HLSL compile and no on-disk cache.

**Timing.** `C_GameWorld::CalcNumberOfSteps` (amax/game/Game/GameWorld/GameWorld.cpp ~2438) accumulates frame time into 1/60 s steps; the step loop clamps at `Game.MaxPhysicsUpdates` (2 or 4). Car physics integrates a fixed 1/60 s per step, so the simulation stays at 60 Hz. On PC, `C_Application::IdleLoop` runs the game update at 60 Hz ticks and renders every loop with matrix and camera interpolation between the last two ticks (details in `docs/analysis.md`, "Frame rate").

## Conventions

- New engine code lives beside what it extends, on the branch of the phase it belongs to. New tools live under `code/tools/<name>/`. Do not restructure the source tree.
- Build output and scratch output go under the phase's `_work/` (gitignored).
- The C++ source's own serialisers are the ground truth for PC formats.
- Never guess a value to make something load. Record unknowns as unknowns.
- **Any reported gameplay or engine issue gets fixed by replicating retail's actual code 100% faithfully.** Find the real mechanism (source diff against the pristine snapshot, or Ghidra against the retail binary per `code/tools/ghidra_retail`) and port it exactly. Do not invent a workaround, approximation, or "close enough" substitute. If the real mechanism cannot be recovered (for example, it lives inside DRM-obfuscated code with no source equivalent), say so explicitly, record what part is and isn't verified in `docs/analysis.md`, and treat any substitute as a stopgap, not a fix.

## Multi-session / multi-agent protocol

1. **Read** the board (open tasks and who holds them) and the tail of the ledger (what just happened) of the phase you work on before doing anything: `Blur1PC/docs/` for Blur 1 PC, `<Phase>/docs/<phase>/` for the others, read and edited in that phase's worktree. Blur1PC's docs are about Blur 1 PC only.
2. **Claim** a task on the board before working on it: set `Owner` to your session or agent name and `Status` to `in progress`. One task per agent at a time. Parallel agents claim disjoint tasks and touch disjoint files; list the files you expect to touch in the task row. A task names the phase it belongs to (its branch); work on it in that phase's worktree. A fault found in a higher phase that belongs to a lower one gets its own task on the lower phase (`Found in` / `Belongs to` in the row) and the finder's task is `blocked` on it.
3. **Decisions** that change direction go in the phase's `decisions.md` as a new numbered entry with status `proposed` until the user confirms. Never silently reverse an `accepted` decision.
4. **Facts** discovered about the source, the discs or the formats go in the phase's `analysis.md`, not in the ledger.
5. **Finish** by appending a ledger entry (template at the top of that file): what changed, what was verified and how, what is blocked, and the exact next step. Then update the board row. If you stop mid-task, say where in the ledger so the next agent can resume.
6. Commit only when the user asks, and commit your changes only, staging by hunk if needed. Commit on the branch of the phase the change belongs to, and when it is a lower branch, merge it down through every branch above (`git merge`, never rebase) before calling the task done: the upper phases see committed, merged changes only. Never push until the user asks.
7. **Do not launch the game in foreground** Every time you launch a foreground process you're interrupting anything the user is doing. Launch the game minimized, never hidden: a minimized window stays on the taskbar, so the user can spot a stray session. `<Phase>/_work/run/run_script.ps1` does this (the game shows its window minimized without activation while `BLUR2PC_AUTOMATION` is set, and keeps rendering).
8. **Never kill processes you don't own**.
   **Queue your game runs**: the game allows one running copy (a second exits at once), and several sessions test at the same time. Start every automated run through `tools/game_slot.ps1` (in `Blur1PC/`; any phase): it takes the machine-wide slot `Global\Blur2PC.GameSlot` (a blocking wait on a named mutex, woken when the holder releases it), waits on any game started outside the queue, runs the game and releases the slot. Do not poll for a free moment or ask other sessions to pause.
9. **One exe per phase for the user: `<Phase>/_work/run/Amax.exe`** (`Blur1PC/_work/run/Amax.exe`). It is what that phase's `play.bat` and `Blur.lnk` start, so it is the only place the user tests that phase. Build in that phase's worktree with the standard command into `<Phase>/_work/build/vs/Release`, then copy `Amax.exe` and `Amax.pdb` to `<Phase>/_work/run`. A build contains what is committed on that branch plus your own worktree edits, nobody else's. If that `Amax.exe` is running, wait for it to exit and then copy. Do not build or copy to another output directory, a renamed exe or a private folder because the usual one is busy. A private copy for automated runs is fine, but it does not count as delivery. Do not tell the user a change is ready to test until that phase's `_work/run/Amax.exe` contains it; if you have to stop before then, say so plainly.
10. **Do not stop just to report progress**: When asked for a task, only stop if either the task is considered complete, user input is required, or an error or some other unrecoverable condition was encountered.
11. **In post-commit summaries, don't mention other agents' pending uncommitted work**. Uncommitted files are a given and you're only adding noise for no good reason.
12. **Never capture PNG bursts from the game** (user, 2026-09-26). The game writes each PNG on its render thread, so a burst of `shot` lines or a `record` of many frames freezes it for tens of seconds with its sound looping, in plain view on the user's taskbar. Record video: a `record` run writes one compressed video file. Take a single `shot` only when one still frame is all the check needs.
13. **Start test races from the debug menu** (user, 2026-09-26). It is the preferred way to launch a race in every phase: no career menus, no garage, no intro fly-by, any track the phase's data has. With `Ui.NewLaunchMode` = `DebugRb` in the `debug_values.xml` the game reads, Right Ctrl on the start screen opens the menu and Enter starts the race it shows. What the menu reads, the per-phase setup, the automation script and the limits: `docs/analysis.md`, "Debug menu race launch".
