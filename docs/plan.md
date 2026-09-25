# Plan

Milestone order and the workstreams inside each. Tasks are tracked on `BOARD.md`; this file explains why they exist and how they fit. Update it when a milestone's shape changes.

## M0 — Workspace (done 2026-09-23)
AGENTS.md, docs, board, ledger, gitignore, git init.

## M1 — The source builds and runs Blur 1 on PC
Nothing else can be tested until this exists. Retail Blur 1 PC data is the test input.
- W1.1 Build description: resolve D08; produce a build that compiles `amax` WIN32 Speed and Debug.
- W1.2 Stubs: Bink movie player (no-op or skip), Massive ads, Demonware/online (compile-time off or stub), OpenAL (real).
- W1.3 Run against `C:\Program Files (x86)\Activision\Blur(TM)` data: front end appears, Showroom loads, one race runs. Fix crashes caused by the 2012 retail data being newer than the June 2010 source, if any.
- W1.4 Debug affordances: `FileMappingDebug`, logfs, a command-line data-root override (the source has none).

## M2 — Inventory
- W2.1 Inventory tool (C++ under `code/tools/kanzen_inventory/` or Python bootstrap reading Blur2Test's catalogues): walk the five discs (loose files + pak FATs), record path, size, content SHA-1, disc, container, format generation, validity (framing check, zero-block scan, header check). Output: one table (`_work/inventory/inventory.sqlite` or CSV set).
- W2.2 Serving policy: named scenarios (e.g. "nov10-base + aug31-audio + aug31-only-tracks"), rules by path prefix / extension / generation / validity, deterministic resolution to one (disc, path) per virtual path. Written as a manifest the engine can load.
- W2.3 Engine layer: an `I_FileSystem` that maps a virtual path to the converted file chosen by the manifest, mounted above the paks in `CreateFsLayers`. Fallback to retail PC data for anything missing.

## M3 — Converter (Generation A first, Generation B in parallel once formats are pinned)
C++ tool under `code/tools/kanzen_convert/`, one sub-converter per format, each verified by round-tripping retail X360 → PC and diffing against the retail PC files where the same asset exists.
- W3.1 Chunk-file byte-swap framework (FourCC, C_FileBlock headers, per-chunk field descriptors).
- W3.2 `.texture`: untile Xenos, endian-swap, emit PC texture (reference: Blur2Test `b2_extract_textures.py`, retail PC textures).
- W3.3 `.model` (Gen A 4-chunk, Gen B 5-chunk).
- W3.4 `.level` / `.levelphysics` / `.levelrender` / `.levelstream` (Gen A = retail layout; Gen B `Scen` layout needs new reader on the engine side, see M4).
- W3.5 XT/KSLF `.bin` and `.loc`: big-endian flask → PC flask.
- W3.6 Audio `.baf` (XMA2 → whatever the PC audio path consumes; check what retail PC ships).
- W3.7 Paks: either leave everything loose or rebuild with `bizpak`.

## M4 — Engine loaders for Generation B
- W4.1 `.levelrender` `Scen` layout; `.texture` v1; `.model` 5-chunk; `.levelanim`.
- W4.2 XT: load `Tracks.bin`, `Levels.*.bin`, `Ui.bin`; reconcile schema drift between Blur 1 types and Kanzen records.
- W4.3 XEFX v12 technique graphs in the FX importer.

## M5 — Shader cache
- W5.1 Extract microcode per effect per disc (Blur2Test `decode_xbox_fxb_effect.py`, `xenos_dis.py`).
- W5.2 XenosRecomp → HLSL → fxc SM3 → `PCFX` block → `.fxb`. Cache dir `_work/shaders/<disc>/<hash>/`.
- W5.3 Handle effects that exceed SM3 limits: record them, do not guess.

## M6 — First playable (D07)
Showroom + one race with AI on an Aug 31 track, then a Nov 10 track.

## M7 — Time independence
Convert the frame-counted consumers listed in `analysis.md` §2 to elapsed time; keep the fixed 60 Hz step; verify at 30 / 60 / 144 fps with a debug frame limiter.

## Phase layout (D10, 2026-09-24)
The repo is split into `Blur1PC/` (phase 1), `Blur1Multi/` (phase 2) and `Blur2/` (phase 3). A phase builds the one below as a read-only base with its own `code/` laid over it (`gen_cmake.py --overlay`), so Blur1PC fixes flow up and nothing flows down. M1 and M7 work continues in Blur1PC; M9 is the first Blur1Multi milestone; M2-M6 (Blur 2) will move into `Blur2/` when they start, and their converters/loaders are written as Blur2 overlay or new files, never as Blur1PC edits.

## M9 — Phase 2: Blur 1 from Xbox 360 and PS3 retail data (Blur1Multi)
The retail X360 Blur 1 disc is Generation A data (the Aug 31 prototype's layout), so every reader written here is reused by M3/M4 for the Aug 31 disc.
- W9.1 Game source (done 2026-09-24): `--source=<dir>` / `BLUR_SOURCE`, platform detection, PC path unchanged (T070).
- W9.2 360 pak layer: big-endian pak v1 + XMem/LZX codecs on PC (T071).
- W9.3 Platform data layer (D11): files converted in memory when opened and served through a file-system layer, nothing stored (T072); textures (T073); shaders (T074); audio (T075).
- W9.4 Showroom, then one race, from the 360 disc; keep the Blur1PC and Blur1Multi PC smoke runs passing (T077).
- W9.5 PS3 (T076, started 2026-09-25, D14): the 360 layer takes the source as a parameter; PS3 paks, detection, models, levels, textures, audio banks and audio objects convert; RSX effects to HLSL; then the same showroom/race milestones as W9.4.

## Standing goal (D09)
Blur 1 on retail PC data remains a supported configuration after every milestone; re-run the T005 smoke test (boot, start screen, Showroom, one race) before closing any milestone. Readers and converters grow additively per platform (PC, 360, PS3) and per game (Blur 1, Blur 2 Gen A/Gen B).

## Out of scope
Multiplayer, Demonware, Live/PSN, DLC, achievements, avatar awards.
