# AGENTS.md

This file provides guidance to agents when working with code in this repository.

## What this repository is

Blur2PC turns the leaked Blur (2010, Bizarre Creations, PC) C++ source into a PC build that plays whatever is recoverable from five Xbox 360 prototype discs of the cancelled Blur 2 (codename **Kanzen**). The Blur 1 path is kept working throughout; the long-term goal is one executable that runs any Blur game (Blur 1, the Blur 2 prototypes) from any platform's data (PC, Xbox 360, PS3), see decision D09. Two pieces are new: an **asset inventory manager** that knows every asset on every disc and serves each one from the best source, and a **per-disc shader recompilation cache**. The work is file-based: disc assets are converted offline into PC-format files, and the engine reads those.

This is a multi-session, multi-agent project. **Start every session by reading `docs/BOARD.md` and the last few entries of `docs/LEDGER.md`. End every session by appending a ledger entry.** The protocol is at the bottom of this file.

## Layout

| Path | What |
|---|---|
| `code/` | Blur 1 PC source snapshot (~2010-06-29). Pristine copy at `C:\AIProjects\Blur\code`; `diff -rq` against it shows every port change. |
| `code/amax/` | The game: `game/` (entry `Main.cpp`, `GameApp.cpp`, `GameSystems.cpp`), `library/` (Ui, RaceBook, Network, Kanzen cameras), `tools/` (asset converters). |
| `code/mercury/library/` | The engine: `common/` (system, xt2 gamedata, btl containers), `util/` (logicalFs, resource, input), `graphics/`, `physics/`, `gamesys/`, `ai/`, `audio/`, `ui/`. |
| `code/mercurygame/` | Older tech-test game shell, superseded by `amax/`. |
| `code/tools/` | Build farm, bizpak (pak tool), jam sources, exporters. |
| `code/vendor/` | demonware, lua, zlib, libtom*, cxxtest, tclap. |
| `docs/` | Project docs: analysis, decisions, plan, board, ledger. |
| `_work/` | Scratch and generated output (gitignored). Converted disc data goes under `_work/data/<disc>/`. |

External inputs (never modified, never copied into the repo):

- Prototype discs: `C:\AIProjects\Blur\GameData\Blur 2 X360 prototypes\` (Aug 31, Nov 4, Nov 9, Nov 10, Nov 25 DAMAGED).
- Retail Blur 1 X360: `C:\AIProjects\Blur\GameData\Xbox 360\`. Retail Blur 1 PC: `C:\Program Files (x86)\Activision\Blur(TM)` (native little-endian reference for every format the source reads).
- Extraction tooling and format knowledge: `C:\AIProjects\Blur2Test` (Python 3.12 + QuickBMS; pak, texture, model, level, KSLF/XT and audio decoders; XenosRecomp for shaders; per-disc catalogues in `_work\archive_index\`). Read its `AGENTS.md` and `_notes\disc_resource_map.md` before writing any decoder.

## Building

The build is CMake generated from the original jamfiles (decision D08). From the repo root, in order:

```
python code/tools/jam2cmake/jamextract.py --top . --root code/amax/game/jamfile.jam --target WIN32 --platform PC --build Speed --out _work/build/targets_pc_speed.json
python code/tools/jam2cmake/cpgen.py $(find code/mercury/library code/amax/game -iname "*.cp" -o -iname "*.cv" -o -iname "*.sp")
python code/tools/jam2cmake/shadergen.py --json _work/build/targets_pc_speed.json --out _work/build/gen/shaders
python code/tools/jam2cmake/gen_cmake.py --json _work/build/targets_pc_speed.json --out cmake/targets.cmake
cmake -S . -B _work/build/vs -G "Visual Studio 18 2026" -A Win32
cmake --build _work/build/vs --config Release --target core -- -m -v:m -nologo > _work/build/core.log 2>&1
python code/tools/jam2cmake/errsum.py _work/build/core.log
```

- `cmake` lives at `C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe`; MSBuild prints its summary in the system locale (Spanish), so judge builds by `errsum.py`'s error count.
- Configurations map to the engine's build configs: Debug → `BUILD_CONFIG_DEBUG`, Release → `BUILD_CONFIG_SPEED`, RelWithDebInfo → `BUILD_CONFIG_RELEASE`, MinSizeRel → `BUILD_CONFIG_MASTER`.
- Jam's `[ Batch … : StdAfx.h ]` groups were **unity translation units**; the generator reproduces them as `_work/build/gen/unity/<target>_batchN.cpp`. A file that fails alone but built originally is almost always relying on an earlier file in its group. Run `gen_cmake.py` again after adding or removing sources.
- `cmake/targets.cmake` is generated; never edit it. Per-file or per-target tweaks go in `cmake/overrides.cmake` with a comment saying why. Source edits in `code/` get a `// [Blur2PC]` comment.
- Third-party inputs live in `_work/vendor` (D3DX 9.29 from the `Microsoft.DXSDK.D3DX` NuGet, OpenAL Soft 1.24.3 binaries, IJG libjpeg 6b); `CMakeLists.txt` has cache variables for each. Shims for pieces we do not have: `code/vendor/dxerr` (DXGetErrorString) and `code/vendor/efx-util` (Creative's reverb-preset conversion).
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

## Conventions

- New engine code lives beside what it extends (the inventory layer under `code/mercury/library/util/logicalFs/`, for example). New tools live under `code/tools/<name>/`. Do not restructure the source tree.
- Converted disc data, caches and build output go under `_work/` (gitignored). Never write into the disc folders.
- The C++ source's own serialisers are the ground truth for PC formats; Blur2Test decoders are the ground truth for 360 formats. When they disagree, record it in `docs/analysis.md`.
- Blur2Test's rule also applies here: never guess a value to make something load. Record unknowns as unknowns.
- **Any reported gameplay or engine issue gets fixed by replicating retail's actual code 100% faithfully.** Find the real mechanism (source diff against the pristine snapshot, or Ghidra against the retail binary per `code/tools/ghidra_retail`) and port it exactly. Do not invent a workaround, approximation, or "close enough" substitute. If the real mechanism cannot be recovered (for example, it lives inside DRM-obfuscated code with no source equivalent), say so explicitly, record what part is and isn't verified in `docs/analysis.md`, and treat any substitute as a stopgap, not a fix.

## Multi-session / multi-agent protocol

1. **Read** `docs/BOARD.md` (open tasks and who holds them) and the tail of `docs/LEDGER.md` (what just happened) before doing anything.
2. **Claim** a task on the board before working on it: set `Owner` to your session or agent name and `Status` to `in progress`. One task per agent at a time. Parallel agents claim disjoint tasks and touch disjoint files; list the files you expect to touch in the task row.
3. **Decisions** that change direction go in `docs/decisions.md` as a new numbered entry with status `proposed` until the user confirms. Never silently reverse an `accepted` decision.
4. **Facts** discovered about the source, the discs or the formats go in `docs/analysis.md`, not in the ledger.
5. **Finish** by appending a ledger entry (template at the top of that file): what changed, what was verified and how, what is blocked, and the exact next step. Then update the board row. If you stop mid-task, say where in the ledger so the next agent can resume.
6. Commit only when the user asks.
7. **Do not launch the game in foreground** Every time you launch a foreground process you're interrupting anything the user is doing. Launch the game minimized, never hidden: a minimized window stays on the taskbar, so the user can spot a stray session. `_work/run/run_script.ps1` does this (the game shows its window minimized without activation while `BLUR2PC_AUTOMATION` is set, and keeps rendering).
9. **One exe for the user: `_work/run/Amax.exe`.** It is what `play.bat` and `Blur.lnk` start, so it is the only place the user tests. Build with the standard command into `_work/build/vs/Release`, then copy `Amax.exe` and `Amax.pdb` to `_work/run`. If `_work/run/Amax.exe` is running, wait for it to exit and then copy. Do not build or copy to another output directory, a renamed exe or a private folder because the usual one is busy. A private copy for automated runs is fine, but it does not count as delivery. Do not tell the user a change is ready to test until `_work/run/Amax.exe` contains it; if you have to stop before then, say so plainly.