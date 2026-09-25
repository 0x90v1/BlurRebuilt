# Decision log

Numbered, append-only. Status: `proposed` (needs the user), `accepted`, `superseded by Dnn`. Never edit an accepted decision; add a new one that supersedes it.

## D01 — Approach: file-based conversion, engine reads PC-format files
**Status:** accepted (user, 2026-09-23); "offline into `_work/data/<disc>/`" superseded by D11 (conversion on the fly, only shaders cached)
Disc assets are converted offline into PC-format files under `_work/data/<disc>/`. The engine reads those through its normal loaders. No runtime big-endian parsing in the engine.
**Why:** every reader validates the platform FourCC and never endian-swaps; the conversion step is unavoidable and keeping it offline keeps the engine diff small.

## D02 — Asset inventory manager serves each asset from the best disc
**Status:** accepted (user, 2026-09-23)
An inventory records every asset on every disc (path, size, content hash, validity, format generation, converted-file location) and a serving policy picks the source per scenario. It plugs into the engine at the overlay file system (`C_OverlayFs::GetBestLayer` or a new `I_FileSystem` layer).

## D03 — Per-disc shader recompilation cache
**Status:** accepted (user, 2026-09-23)
Xenos microcode → XenosRecomp HLSL → fxc → PC `.fxb` with a `PCFX` block, cached per disc and keyed by microcode hash, so shaders are rebuilt only when their source bytes change.

## D04 — Toolchain: VS2008 + Jam as shipped
**Status:** superseded by D08

## D05 — Both disc generations supported from the start
**Status:** accepted (user, 2026-09-23)
The inventory and converter handle Generation A (Aug 31) and Generation B (Nov 4-25) from day one. Runtime loaders for Generation B formats are a workstream, not a later phase.

## D06 — Converter is C++ inside this tree
**Status:** accepted (user, 2026-09-23)
The 360-to-PC converter is a C++ tool under `code/tools/`, reusing the source's own serialisers so its output matches the loaders by construction. Blur2Test remains the reference decoder for 360 formats.

## D07 — First playable milestone: Showroom + one race with AI
**Status:** accepted (user, 2026-09-23)
Boot to front end, load one track, race against AI with pickups. No multiplayer, no online, no Demonware.

## D08 — Toolchain: CMake generated from the jamfiles, built with VS 18
**Status:** accepted (user delegated the choice to Claude, 2026-09-23; option 2 below)
Found after D04: the machine has no VS2008 and no DirectX SDK; the tree lacks the custom Jam rules every jamfile depends on (`SupportedTargets`, `IncludePath`, `Define`, …) and the `libs/vendor` tree (Bink, OpenAL, Massive). "As shipped" therefore cannot build anywhere without reconstructing Bizarre's Jambase and installing 2008-era SDKs. Options:
1. Install VS2008 + DirectX SDK (June 2010) and reconstruct the Jam rules (they are simple: include paths, defines, source globs). Closest to shipped; old compiler.
2. Generate a CMake (or vcxproj) description from the jamfile source lists and build with the installed VS 18 toolset, D3D9 headers from the Windows SDK, d3dx9 from the DirectX SDK redistributable or DXVK-native shims. Modern debugger and sanitizers; expect a wave of compile fixes.
Recommendation: option 2, because option 1 still requires reconstructing the rules by hand and then leaves us on a compiler nobody supports. Bink and Massive get stubbed either way; OpenAL 1.1 headers/libs are freely available.

## D09 — Keep Blur 1; target any Blur game from any platform's data
**Status:** accepted (user, 2026-09-23)
When the work moves on to Blur 2, the Blur 1 path is kept working, never replaced. The long-term goal is one PC executable that runs any Blur game (Blur 1 retail, the Blur 2 prototypes) from any platform's data (PC, Xbox 360, PS3).
Consequences:
- Blur 1 on retail PC data stays a supported, regression-tested configuration (the T005 smoke run is the baseline).
- Format support is additive and selected by what the data says (pak version, chunk platform FourCC, header versions, the presence of Gen B chunks), never by replacing a Blur 1 reader with a Blur 2 one. Example: the pak reader handles v1 and v2 side by side.
- Game selection and data source are runtime choices (data root, game/disc profile), not build-time forks. The inventory manager (D01/T010-T012) is the natural place for "which game, which platform, which disc".
- Converters (D06) take any source platform: 360 and PS3 big-endian data, and PC data as the identity case. PS3 data adds its own formats (GCM textures, PS3 chunk FourCC); the PS3 prototype ELFs (analysis.md section 4) become directly relevant for those.
- Shader cache (D03) is keyed by platform as well as disc: Xenos microcode (360), RSX/Cg (PS3), native PCFX (PC).
Architecture (user, 2026-09-23), two switchable layers:
1. **Platform data layer (bottom).** Reads any platform's data behind one interface: container formats (pak v1/v2/v3, codecs raw/zlib/XMem LZX), byte order (PC little-endian, 360/PS3 big-endian), chunk platform FourCC, texture formats (360 tiled/swizzled, PS3 GCM, PC DDS), shader formats (Xenos microcode, RSX, PCFX) via the per-platform shader cache, and audio banks. Everything above it sees engine-native PC data. It sits under or beside `C_OverlayFs` and the resource readers; the inventory manager (T010-T012) decides which source serves each asset.
2. **Game profile layer (top).** Selects Blur 1 or Blur 2 behaviour for scene construction (level pipeline, `.levelrender` LNSc vs Scen, `.levelanim`, XT schemas, Tracks/Levels/Ui databases) and game mechanics (pickups, rules, UI flow), chosen at runtime from the data being loaded.
Open question for the user: D01 says disc assets are converted offline into PC files. The platform layer can either keep doing that (convert once, cache under `_work/`) or decode at load time; both satisfy this architecture. Until decided, converters are written as libraries callable from both an offline tool and a runtime reader. **Answered by D11 (user, 2026-09-24): decode at load time; only shaders are cached.**

## D10 — Workspace split into phase folders; upper phases build the lower ones as an overlay
**Status:** accepted (user asked for the split, 2026-09-24); the overlay mechanism is Claude's implementation of "inherit improvements without affecting the lower phase" and stands unless the user objects
The repo holds one folder per project stage: `Blur1PC/` (phase 1, the source tree and its build), `Blur1Multi/` (phase 2), `Blur2/` (phase 3), with `docs/` and the shared `_work/` (vendor SDKs, Ghidra output) at the root. Moves were done with `git mv`, so history follows the files.
A phase compiles the phase below as a read-only base with its own `code/` laid over it: `gen_cmake.py --overlay <phase> --project-dir <phase>` picks, per source file, header directory, PCH, built-in shader and Lua script, the overlay copy when one exists at the same relative path; new files are declared in `<phase>/cmake/multi.cmake`; `Blur1PC/cmake/blur_common.cmake` (flags, SDKs, shims) and `overrides.cmake` are included as they are. Each phase has its own `_work/` and `_work/run/Amax.exe`.
**Why:** the user's rule that each layer must be removable without touching the ones below, while Blur1PC fixes keep flowing upward. An overlay gives that with zero changes to Blur1PC: no hooks, no `#ifdef`s in the base. Its cost is that an overlay copy of a base file freezes that file; `Blur1Multi/tools/overlay.py` records the base blob hash per copy and `check` reports drift, so the cost is visible and copies are kept few (one so far: `amax/game/GameSystems.cpp`).
Rejected: a patch queue applied at generate time (conflicts fail the build and the compiled file is not the one in the tree); seams/hooks in Blur1PC with a PC default (phase 1 must stay retail-faithful and free of phase 2 code); separate git repositories or submodules (history and cross-phase diffs get harder for no gain).

## D11 — Game source: a directory chosen at run time, served by a platform data layer that converts on the fly; only shaders are cached
**Status:** accepted (user, 2026-09-24: "the per-source disc cache is only for shaders ... The rest needs to be done on the fly"); supersedes the "offline" part of D01 and answers the open question in D09
The phase 2 exe takes its data from a *game source*: a directory laid out like a Blur install or disc (`--source=<dir>`, `BLUR_SOURCE`, default the working directory as retail). `GameSource::Detect` reads the platform from the data itself: `cache/root.pak` magic byte order (`2KAP` little-endian = PC, `PAK2` big-endian = 360/PS3), any `*.xex` (360 disc root), `PS3_GAME/USRDIR` (PS3 disc root). A PC source becomes the working directory, layer 0 of the retail overlay file system, so the PC path is unchanged.
For a big-endian source a *platform data layer* (D09) reads the source's paks natively (big-endian pak v1, XMem/LZX codecs) and converts each file to PC format **when it is opened, in memory**, and serves the result through the normal loaders (`X360Convert::C_ConvertFs`, an `I_FileSystem` wrapped around the pak layers). Nothing but shaders is written to disk: no `_work/data/<source>/` tree, no offline conversion step. The only memory kept is a bounded set of recent conversions, so a `FileInfo` followed by `Open` converts once. The engine's readers stay PC-only (D01's "no big-endian parsing in the engine" holds: they never see big-endian data).
**Shaders are the exception.** Xenos microcode cannot be decompiled and recompiled at load time (XenosRecomp + fxc per effect is far too slow), so shaders go through the per-source cache of D03: microcode → XenosRecomp HLSL → fxc SM3 → `PCFX` `.fxb`, built the first time a source is run and kept per source beside the executable (`<Phase>/_work/run/shadercache/<source-id>/`), keyed by microcode hash.
**Why:** one exe, one data directory choice, no conversion step and no converted copy of the disc for the user. Converters are libraries (D06): the engine calls them at load time and the standalone verifiers (`xmem-verify`, `x360convert-verify`) call them to check output against retail PC files; neither writes game data for the engine to read.

## D14 — PS3 sources in phase 2: one console conversion layer, per-console converters where the formats differ
**Status:** the sampler-dimension rule is accepted (user, 2026-09-25, asked to choose between it, per-bound-texture variants and a table from the engine's shader sources); the rest is proposed (Claude, 2026-09-25, implementing the user's request for PS3 support in Blur1Multi)
**Layer.** A PS3 source goes through the Xbox 360 converting layer (`X360Convert::C_ConvertFs`, library `x360convert`, name kept), told the source once at boot with `X360Convert::SetSource(E_Source_PS3)`. Retail PS3 and 360 files are byte-identical for XT flasks, loc, cycles, skeletons, particle meshes and audio objects, and the model and level grammar is the same Arch archive with a few exporter switches (PlatformType, zlib in place of XMem, U8_4 already in PC order), so those converters take the source as a parameter. Formats that differ get their own converter selected by source: GCM textures (`Ps3Texture.cpp`), audio banks (`BafConvert.cpp`; the 360's XMA banks are T075), effects (`Ps3EffectConvert.cpp`, `RsxTranslate.cpp`). The shader cache is per source kind: `shadercache\ps3\` beside `shadercache\x360\` (D03, D11), with its own converter tag in the key.
**Sampler dimension (user).** RSX fragment programs do not record a sampler's texture type and D3D9 needs it at compile time. Rule: a sampler is a cube when its packed AddressW in the PS3 effect's sampler-state data is not the default (0), 2D otherwise. Only three sampler names set AddressW in the 820 retail effects (`g_tEnvironment`, `g_sVehicleEnvironmentTexture`, `g_sLightEnvironmentTexture`), all three are cubes in the engine's shader sources (`amax/game/Shaders/Common/Lighting.h`, `graphics/vehicle/include/vehicle/shaders/vehicle_common.h`), and no PS3 fragment program samples a volume texture. The rule rests on how Bizarre authored these effects, not on the format; an effect set that breaks it would need the per-bound-texture approach (compile a variant per bound texture types at draw time, as RSX emulators do).
**Why:** one layer keeps one set of fixes (the level-preparation deadlock fixed here was in the 360 path too) and one verification method; duplicating it per console would double code that the data shows is shared.
Rejected: a separate `ps3convert` library copying the 360 layer (two copies of the scene transcoder and the file-system layer for four switches); vendoring RPCS3's RSX decompilers (GPL-2.0, cannot be linked into an exe with D3DX, Demonware and this source; used as a reference only).
