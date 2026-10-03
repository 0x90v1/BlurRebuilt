# Ledger (Blur1PCDX11)

Append-only log of sessions and agent runs, newest at the bottom. Every session ends with an entry. Keep facts in `analysis.md` and decisions in `decisions.md`; the ledger says what was done, how it was checked, and what comes next.

Entry template:

```
## YYYY-MM-DD HH:MM — <session or agent name> — <task IDs>
Did: …
Verified: … (how; command or file diffed)
Blocked: … (or none)
Next: … (exact next step for whoever picks this up)
Files touched: …
```

---

## 2026-09-26 19:00 — session Codex DX11 — T127

Did: created `blur1pcdx11` at `Blur1PCDX11/` from `blur1pc` HEAD `547ccfb1`. Added an isolated native D3D11 device module and offscreen `dx11_probe` target, plus conditional SM5 variants of two existing display shaders. The device creates a feature level 11 hardware or WARP context, compiles the repository shaders with nested includes, draws, and reads back. Added branch README and recorded the display/shader boundary in analysis.md. The Amax backend selection and game rendering remain D3D9.

Verified: the probe returned center RGBA `(125,67,63,255)` and corner `(5,10,20,255)` on hardware and WARP. D3D11 debug validation found and helped fix a vertex/pixel interpolator register mismatch. `shadergen.py` produced 693 built-in SM3 variants with zero failures; standard Release `Amax` build had zero errors; `git diff --check` passed. No game was launched during the native probe or build.

Blocked: a playable D3D11 exe needs the Display and Display2 resources/draw-state port, DXGI presentation, and a shader path for the retail `PCFX` effects. No DX11 game exe has been delivered to `Blur1PCDX11/_work/run`. Changes are uncommitted under protocol rule 6.

Next: implement a D3D11 display manager/window and resource bindings in the new branch, then replace the D3D9 backend selection only after the game's shaders and render targets are supported. Keep verifying against the native probe and compile the existing Amax target; run the game through the queued minimized harness when the backend supports it.

Files touched: Blur1PCDX11 `CMakeLists.txt`, `cmake/dx11.cmake`, display `device/d3d11/{D3D11Device.h,D3D11Device.cpp}`, two built-in display shaders, `code/tools/dx11_probe/main.cpp`, `README.Blur1PCDX11.md`; Blur1PC docs/{BOARD,LEDGER,analysis}.md. No commit.

## 2026-09-26 19:43 — session Codex DX11 continuation — T127

Did: continued the native `blur1pcdx11` branch. Added DXGI window swap-chain creation, present and resize; PC texture and vertex/index buffer allocation; Shader Model 5 variants of the repository's 2D textured and 3D coloured shaders; shader reflection and named constant-buffer binding for the engine's clip vector, world matrix and view-projection matrix. Added a native 3D indexed draw path. `BLUR_USE_DX11` now links the new device library when selected, but the `Amax` game cannot compile with that option because its Display and Display2 native backend types are not implemented. No Direct3D 9 API translation layer was added. No DX11 game exe was delivered.

Verified: the D3D11 probe passed on hardware and WARP with the debug layer, drawing 2D colour, 2D texture and translated 3D colour, plus DXGI present/resize; no validation messages. The default Direct3D 9 `Amax` Release build completed with zero errors after 693 SM3 built-ins compiled successfully. `git diff --check` passed. A direct `fx_5_0` compile of an existing game `.fx` file reached a D3D9 effect `shared` declaration error after include paths and profiles were adjusted, confirming that retail effect source needs an SM5 port. No game was launched. No commit.

Blocked: native Display and Display2 resource/device implementations, then the retail `PCFX` effect shader path. The game is still D3D9 in this branch; T127 remains in progress.

Next: implement the native Display/Display2 backend in `Blur1PCDX11/`, port the effect source and shader cache to SM5, select and build `Amax` under `BLUR_USE_DX11=ON`, then deliver and validate the DX11 exe under the game-run protocol.

Files touched: Blur1PCDX11 `CMakeLists.txt`, `cmake/dx11.cmake`, `README.Blur1PCDX11.md`, display `device/d3d11/*`, shader variants, `code/tools/dx11_probe/main.cpp`; Blur1PC docs/{BOARD,analysis,LEDGER}.md. No commit.

## 2026-09-26 21:20 — session Codex DX11 continuation — T127

Did: continued the native `blur1pcdx11` branch without a D3D9 API translation layer. Vendored Microsoft FX11 source; extracted the original Jam effect variant list; compiled all authored PC effects to FX5 with native pass blend/depth/raster states, alpha discard and sampler filter/sRGB metadata. Added a name-indexed FX11 effect library, a reusable indexed material draw with cached input layouts, native 2D/3D/cube texture uploads, compressed BC1/2/3 and sRGB sampling, and a Jam-selected SM5 compiler for every built-in shader. Added one SM5 `SV_Position` correction for `psPolygonPaint`.

Verified: `dx11_fx_all` compiled 1,254/1,254 effects and `dx11_shaders_all` compiled 693/693 built-in variants. `dx11_probe` with the D3D11 debug layer passed on hardware and WARP: preloaded all 1,254 FX11 effects, drew an authored ScreenFacingSprite material through the reusable device path, checked MeshEffect blend/depth/cull state and an alpha-discard instruction, loaded compiled built-in shader binaries, sampled BGRA/sRGB and BC1/2/3 textures, created volume/cube/multisample resources, and presented/resized a DXGI swap chain. Python tool syntax checks and `git diff --check` passed. No game was launched. No commit.

Blocked: `BLUR_USE_DX11=ON` Amax still does not compile because its Display and Display2 native backend types, resource wrappers, context methods and FX::Manager integration remain absent. No native DX11 game exe can be delivered yet.

Next: implement the Display2 D3D11 context/resource/device API and Display C_Device/manager wrappers in this branch, replace the game FX::Manager PCFX/D3D9 shader path with the compiled FX5 index, then build Amax and deliver/test its standard `_work/run/Amax.exe` through the queued minimized game protocol.

Files touched: Blur1PCDX11 CMakeLists.txt, cmake/dx11.cmake, README.Blur1PCDX11.md, display device/d3d11/*, built-in shader variants, ui psPolygonPaint, code/tools/{dx11_fxcompile,dx11_shadercompile,dx11_probe}, code/vendor/fx11; Blur1PC docs/{BOARD,analysis,LEDGER}.md. No commit.


## 2026-09-29 — session Claude DX11 — T127

Did: made the native D3D11 backend run `Amax` and hunted the black screen. Fixes: `ID3D11Multithread::SetMultithreadProtected` on the immediate context (the loading thread and the main thread both used it; the debug layer reported corruption and hardware removed the device); the vertex/pixel stage signature convention (vertex shaders emit interpolants first with `SV_Position` last, pixel shaders declare the same order and no position — applied to 24 built-in shaders under `#if defined(BLUR_DX11)`); `BindRenderTargets` no longer binds a depth view of a different size than the colour target; `C_PCD3D11GameDevice::RestoreWindow` follows the retail branch (quad onto the back buffer, copy onto any other surface) instead of always drawing the quad; the D3D11 info queue filters warnings and a vectored exception handler logs its messages before the crash handler; draws with no bound vertex shader are skipped instead of raising, and name the effect that failed. Found that the eight vehicle effects cannot be provided: the retail `.fxb`s ship ten techniques each (`MaterialPrepass`, the `NoDamage` and `LOD` variants) while the leaked `vehicle_common.h` declares five, so every vehicle pass fails with `ERROR_NOT_FOUND` and 64 draws a frame are dropped at the start screen. `System::OnEffectLoad` now dumps the retail microcode of those passes to `_work/build/fxdump/`.

Verified: `Amax` builds with no errors and runs the start screen on retail PC data; the D3D11 debug layer over a full start-screen run reports zero linkage errors, zero thread-corruption messages and no information-queue exceptions (previously 150k linkage messages, a corruption message and a removed device). `Quad::DrawFullScreenColour` into the back buffer and into the window texture reads back magenta and cyan, proving the quad, target and readback paths. A D3D9 reference run (`Blur1PC/_work/run/Amax.exe`, same automation script) shows the start screen at 20 s for comparison. Screen is still black under DX11; only the block-compressed texture-upload alignment message remains.

Blocked: the start screen renders nothing — the frame never reaches the back buffer, and the composite that should put it there has not been identified. Opening a debug race crashes in `UI::C_UiManager::LoadingTextureClone`. The vehicle effects need the shipped shader revision (not in the source tree) reconstructed from the dumped microcode.

Next: dump and disassemble `_work/build/fxdump/veh_*_MaterialPrepass*` with D3DX (`D3DXDisassembleShader`) and reconstruct the missing vehicle techniques as HLSL under `BLUR_DX11`; separately, trace which pass is supposed to present the frame on the back buffer by instrumenting `Blur1PC`'s D3D9 device for one run and comparing its target bindings with the DX11 ones.

Files touched: Blur1PCDX11 shader sources (24 files), display device `d3d11/{D3D11GameDeviceNative.cpp,PCD3D11GameDevice.cpp}`, `display/source/shader/{VertexShaderUtils.cpp,PixelShaderUtils.cpp}`, `fx/{NativeFX11.cpp,System.cpp}`, `fx/include/fx/NativeFX11.h`, `_work/run/{run_script.ps1,logcfg.xml}`, docs/blur1pcdx11/{analysis.md,LEDGER.md,BOARD.md}. No commit (user instruction).

## 2026-09-29 (later) — session Claude DX11 — T127

Did: got a debug race to run and isolated the black screen to one step. `Display2::CloneTexture2D` crashed on D3D11 (`memcpy` between two null CPU addresses) and the race's loading screen called it; it now copies on the GPU under `DEVICE_D3D11`. `C_PCD3D11GameDevice::RestoreWindow` follows the retail branch (quad onto the back buffer, copy onto any other surface). Counters added to the composition path: per race frame `RenderSplitscreenWithoutAA` binds the back buffer and `RestoreWindow` still takes the copy path, because a layer rebinds a post-process target through `BeginRenderSplitscreenWithoutAA` between the bind and the restore. The window render texture holds a complete frame (road, trees, HUD, tyre marks); the back buffer is black.

Verified: a debug race (Right Ctrl on the start screen, Enter) reaches `Racing` and runs 20 s with zero debug-layer messages, zero rejected draws except the vehicle prepass, and a clean exit; `<shot>.window.tga` mean 104 against `<shot>.tga` (back buffer) mean 0. Build has no errors.

Blocked: the back buffer prime. The D3D9 device has to be instrumented the same way and compared before changing the port, since retail takes the same branch and still shows the frame.

Next: build the `BLUR_USE_DX11=OFF` configuration from this worktree into a second build directory with the bind trace in `display/source/device/d3d9/PCD3D9Device.cpp`, run the same race script, and compare the surface sequence with the DX11 one.

Files touched: `display2/source/utils/Utils.cpp`, display device `d3d11/{D3D11GameDeviceNative.cpp,PCD3D11GameDevice.cpp}`, `ApplicationBase.cpp` (trace only), docs/blur1pcdx11/{analysis.md,LEDGER.md}. No commit (user instruction).

## 2026-09-29 (3) — session Claude DX11 — T127

Did: traced the presentation chain and found the fault's home. Retail presents in `I_PostProcess::DisplayFinal` (`E_Type_Final`): bind the back buffer, draw the last chain texture full screen, clear the manager's active flag; `C_Manager::End`'s own final quad is therefore dead when post-processing runs, which is why the earlier counters looked wrong. Disabling post-processing (`Graphics.Render.PostProcess.Enable=false` in the game's `debug_values.xml`, restored afterwards) makes the same race present: back buffer mean 44.8 against 0.0, window texture unchanged at 104. Removed the investigation-only traces afterwards (`postprocess/Manager.cpp` back to its original bytes, `ApplicationBase.cpp` phase log, the `SetColourSurface`/chain-source probes in the D3D11 device).

Verified: race runs 20 s with zero debug-layer messages and a clean exit after the cleanup; `race2.tga.window.tga` mean 104.6 (the full frame) with `race2.tga` mean 0.0 (post-process on) and 44.8 (post-process off).

Blocked: the post-process chain loses the frame between `Begin` and `DisplayFinal`. The vehicle `MaterialPrepass` hole is the white quad over the car.

Next: capture the chain texture at each `I_PostProcess::DisplayIntermediate` to find the link that goes black; the chain's shaders are the post-process pair set already fixed for stage signatures, so check sampler/texture binding and the resolve/ping-pong of the chain render texture next.

Files touched: `display2/source/utils/Utils.cpp`, display device `d3d11/{D3D11GameDeviceNative.cpp,PCD3D11GameDevice.cpp}`, docs/blur1pcdx11/{analysis.md,LEDGER.md,BOARD.md}; `postprocess/Manager.cpp` and `ApplicationBase.cpp` reverted to their previous state. No commit (user instruction).

## 2026-09-29 (4) — session Claude DX11 — T127

Did: committed the branch's uncommitted state and pushed `blur1pcdx11`, at the user's instruction. No source changes this session. T150's `CloneTexture2D` GPU copy went in on its own commit; everything else, including the T127 and T150 records in this file and on the board, went in on a T127 commit.

Verified: `git status` before staging listed 97 modified tracked files and 109 untracked files, all inside the DX11 scope (display and display2 D3D11 backends, 24 built-in shader sources, `fx/NativeFX11`, `movie/w32/bink`, `code/tools/dx11_*`, `code/vendor/fx11`, `docs/blur1pcdx11`). Largest new file 199 KB, no binary among them, and the only secret-shaped string is `secrets.GITHUB_TOKEN` in Microsoft's own fx11 CI workflow. The tree still carries the T127/T149 investigation traces (`ApplicationBase.cpp`, `deferred/Manager.cpp`, `GameWorld.cpp`, `Lighting/Prepass.cpp`, `ModelResource.cpp`), all behind `#if defined(BLUR_USE_DX11)`.

Blocked: unchanged — the post-process chain loses the frame, and the vehicle effects need the shipped techniques (T148 and T149 open).

Next: as in the entry above — capture the chain texture at each `I_PostProcess::DisplayIntermediate`; `deferred/Manager.cpp` `GetColourTexture` is already instrumented for it.

Files touched: docs/blur1pcdx11/LEDGER.md.

## 2026-09-29 (4) — session Claude DX11 — T127, T149

Did: found and fixed the four faults that kept the DX11 build from presenting (analysis.md, "four faults between the renderer and the screen"): built-in shaders got their textures in the wrong registers (sampler-to-texture register map from reflection in `D3D11GameDeviceNative.cpp`); `C_D3D11GameDevice::Unbind` cleared the whole pipeline (now a port of the D3D9 function); the FX11 bridge read FX texture values as `Display::C_Texture*` when they are `ID3D11Resource*`, and could not resolve texture parameter names (`NativeFX11.cpp`, `D3D11EffectLibrary`); `Display::Quad` applied D3D9's half-pixel shift (now skipped under `BLUR_USE_DX11`, as the engine does on PS3). Added a one-frame draw trace (`BLUR_DX11_TRACE_AT`, `BLUR_DX11_TRACE_PX`, per-draw debug messages with `BLUR_DX11_DEBUG=1`) and `_work/run/run_slot.ps1` (runs an automation script through `tools/game_slot.ps1`). Delivered `_work/run/Amax.exe`.

Verified: builds with no errors. Start screen (`scripts/start.txt`): back buffer mean RGB 28/47/73; showroom street, lights and all UI present, matching the D3D9 reference except the car body. Debug race (`scripts/race.txt`): reaches Racing, runs 18 s, clean exit; HUD, mirror, minimap, buildings and far road present. Before this session both back buffers were mean 0.

Blocked: none for presenting. Open rendering faults: the car body (T148), the near road rendering white in the race (`Road / MaterialLights` output; input not identified), no sRGB decode on texture reads.

Next: trace `Road / MaterialLights` at (0.70, 0.83) of a race frame with each of its textures replaced in turn by a black view (or disassemble the FX11 pass and dump its constant buffer) to find the input that whitens it; then sRGB (TYPELESS texture creation plus `_SRGB` views where `SetTextureGamma` or `effects.samplers` asks for them).

Files touched: display device `d3d11/{D3D11GameDeviceNative.cpp,D3D11GameDevice.cpp,PCD3D11GameDevice.cpp,D3D11EffectLibrary.cpp}`, `display/include/display/device/d3d11/D3D11EffectLibrary.h`, `display/source/render/Render.cpp`, `fx/source/NativeFX11.cpp`, `_work/run/{run_slot.ps1,scripts/race.txt,scripts/start.txt}`, docs/blur1pcdx11/{analysis.md,LEDGER.md,BOARD.md}. No commit (user instruction).

## 2026-09-29 18:35 - Codex context-efficiency - T176
Did: applied the user-authorized context-efficiency patch to this phase worktree and the other active phase worktrees (blur1pc, blur1multi, blurx, blur1pcdx11). Reduced always-loaded AGENTS.md from about 22 KB to 6,054 bytes; preserved every original substantive section in docs/agent-reference/{layout,build,architecture,workflow}.md and added context.md. Added tools/agent_context.py for bounded task/ownership/ledger reads, saved-log excerpts, and recorded Codex token metrics. Global Codex limits: tool_output_token_limit=4000, model_auto_compact_token_limit=140000.
Verified: all phase instruction links resolve; all original sections preserved; helper copies identical; all four context commands expose the requested task, ledger, and ownership within 8,000 characters. Exercised log neighbors/tail, missing-task rejection, malformed usage-event handling, token metrics, and small/default output caps. TOML parsed and installed Codex rendered a fresh prompt successfully. Only this patch's paths were edited; concurrent code work was not changed by this task. No game build/run required for documentation and read-only helper changes.
Blocked: none for applying the patch. Worktree changes remain uncommitted under the project's commit-on-request rule; no branch merges or pushes performed. The untouched master/source snapshot was not modified.
Next: reload instructions/configuration in new sessions, then compare recorded per-request tokens and same-session cumulative usage deltas for comparable completed tasks. Full logs remain on disk for any truncated excerpt. Commit these paths only when requested, preserving the existing branch ancestry and other work.
Files touched: AGENTS.md; docs/agent-reference/; tools/agent_context.py; docs/blur1pcdx11/{BOARD,LEDGER}.md. Global config: C:/Users/uKER/.codex/config.toml. Backups: C:/Users/uKER/.codex/tmp/context-efficiency-20260929-182911.

## 2026-09-29 23:54 — session Codex visual diagnosis — T127

Did: delivered a visual repair batch grounded in the pristine PC mechanisms and actual GPU traces. Fixed omitted sampler addressing defaults, alignment/upload of streamed mip tails, native device default sentinels, five families of shader input interfaces, mutable per-draw alpha testing across authored/recovered/built-in shaders, and D3D9 colour-factor translation into valid D3D11 alpha blend factors. Traced the large white race rectangle to a rejected drop-shadow multiply state and corrected the translation. Restored visible road/building textures, foliage cutouts and the mirror; removed the white rectangle. Retained opt-in diagnostics and improved debug-message deduplication. Updated the board, analysis and README. Found in / Belongs to Blur1PCDX11. No commit or push; no lower-phase changes.

Verified: standard Release build reports zero errors; 693 built-ins and 1,254 base effect outputs compile with 4,913 indexed entries. Hardware and WARP probes each pass 45 GPU alpha comparisons, five GPU RGBA blend-factor checks, 42 compiled shader-interface pairs, actual sampler-default checks, all 1,020 recovered vehicle techniques, alloy MRT readback and existing presentation checks. Python compiler modules compile. Final minimized, queued race and Showroom runs exit zero in 27 and 13 seconds respectively; both logs have no D3D11 severity 0/1/2 messages. Visually inspected one still from each: `_work/build/race_shadow_fixed.png` and `showroom_shadow_fixed.png`. Delivered `_work/run/Amax.exe` and `Amax.pdb` hashes match standard Release outputs; effects are delivered in `_work/run/fx11`. Whitespace checks pass. Streaming-copy correctness was checked against pristine source and game scenes, not a standalone streaming GPU test.

Blocked: none for this repair batch. T127 remains in progress: vehicle/scene lighting and sRGB sampling lack parity, some unidentified draws have no vertex shader, and uncaptured retail variants remain outside recovered coverage. Clean debug logs do not establish full visual parity.

Next: implement the original per-sampler sRGB contract with compatible texture/view formats, verify actual GPU sample values, then compare the same Showroom against the saved D3D9 reference. Separately identify the remaining no-VS draw callers and recover any missing variants from correctly keyed retail captures.

Files touched: display/source/{context/Context_D3D11.cpp,device/d3d11/{D3D11GameDeviceNative.cpp,D3D11States.cpp,PCD3D11GameDevice.cpp},texture/d3d11/D3D11TextureLoad.cpp}; display/include/display/device/d3d11/D3D11States.h; fx/source/{NativeFX11.cpp,System.cpp}; render/deferred/UI/drop-shadow pixel shader interfaces; code/tools/dx11_shadercompile/{alpha_test.py,compile_builtins.py}; code/tools/dx11_fxcompile/{compile_fx.py,compile_manifest.py}; code/tools/dx11_probe/main.cpp; cmake/dx11.cmake; README.Blur1PCDX11.md; docs/blur1pcdx11/{BOARD.md,analysis.md,LEDGER.md}; ignored diagnostic/build/run artifacts. Existing unrelated work preserved.

## 2026-09-30 03:08 — session Codex visual diagnosis — T127

Did: corrected FX11 pass state ownership. D3D9 replay proves the alloy bands are expected; selected native draws locate the actual wheel corruption in the later puddle pass. Removed guessed complete FX11 blend/depth/stencil/raster objects so the imported PC FXB and render stages retain their live state. Added GPU state-preservation checks across ordinary and repeated/immediate effect applies. Found in / Belongs to Blur1PCDX11. No lower-phase source changes or push; follow-up work remains uncommitted.

Verified: standard Release errsum0 (`visual_fx_state_build.txt`). Hardware and WARP regressions pass along with prior probes. Queued minimized Showroom and DebugRb race exit zero, clean debug/missing-shader logs, both stills inspected. Native puddle draw now matches D3D9 GREATER/no-depth-write/stencil-replace state; wheel pixel remains unchanged and the reflection overlay is gone. Delivered standard exe `23C8A1BA1224B0E8DD72D7166C3A292CDCD46F5EBC94D512E5026A624F02A577`, PDB `D606F19B18E50D96217DA23A06A15CDE67479BC3C67F901EC545E589AD894AFF` and regenerated FX5 effects. Exe/PDB match Release outputs.

Blocked: none for build/delivery. Broader visual parity, the unreproduced damage-volume loading crash and uncaptured variants remain open.

Next: recover and verify the live sampler filter/address/border/LOD contract from pristine PC source. FX11 currently binds live gamma but retains compiled sampler defaults for the other properties. Distinguish Display2's encoded LOD bias from the old Display device API; preserve the actual PC anisotropy behavior. Add GPU sampling checks before claiming a fix. Task remains active.

Files touched: dx11_fxcompile/compile_fx.py, fx/FxContext.cpp, dx11_probe/main.cpp, README and phase docs; ignored reference captures/build/runtime logs. Other sessions' pending changes preserved.

## 2026-09-30 03:56 — session Codex visual diagnosis — T127

Did: repaired packed polygon-font inputs using native int2/uint4 followed by the numeric conversion performed by D3D9. Pristine UI declarations/storage establish the contract. GPU capture before repair proves NaN positions and zero alpha; after repair both hardware and WARP return all expected values. Found in / Belongs to Blur1PCDX11. No lower source changes or push.

Verified: standard Release errsum0 (`visual_packed_font_build.txt`), full hardware/WARP probes pass. Queued minimized DebugRb race exits zero with clean debug/missing-shader logs. Inspected `race_packed_font_final.png` confirms the large white position numeral is restored. Standard exe/PDB delivered, hashes match Release outputs: exe `7FF617DA6DF045A2D100B5E5DFB52C6166B8CF7B53EC8FFF1D9D9150C66A8580`, PDB `6BC17EC01B8E482280D6EF8145F43FB9862F389BF9954922CAFBC6BFE7682F7D`.

Blocked: none for this repair. Broader lighting parity, remaining packed UI paths, intermittent damage-volume crash and uncaptured variants remain open.

Next: inspect packed stroke/trail declarations and shaders for the same conversion fault; compare actual native/D3D9 road and vehicle lighting draws from retained race traces. Task active.

Files touched: UI vsPolygonFont.vsh, dx11_probe main.cpp/new polygon_font_checks.h, phase analysis/ledger; ignored build/probe/runtime capture files.

## 2026-09-30 04:42 — session Codex visual diagnosis — T127

Did: recovered the retail road prepass depth interpolant from actual D3D9 draw 7047254 and repaired the native NO_WORLDNORMAL path. Added 128 GPU depth checks across 64 road permutations. Found in / Belongs to Blur1PCDX11; no lower-phase source changed.

Verified: standard Release and probe builds have zero errors; full hardware/WARP probes pass. Standard exe/PDB and regenerated effects delivered to _work/run with matching hashes. Minimized queued race exits zero, no severe D3D11 diagnostics. Per-draw depth and inspected headlight/final race captures confirm removal of the corrupt road wedge and restoration of headlight wash. Logs/captures: _work/build/road_depth_final and road_depth_{hardware,warp}.log.

Blocked: none. T127 remains in progress; remaining NaN depth pixels, packed UI and broader parity are unverified.

Next: commit this verified repair, isolate the first writer of a remaining NaN depth pixel, then verify packed stroke/trail inputs against the PC source.

## 2026-09-30 05:07 — session Codex visual diagnosis — T127

Did: preserved the shipped billboard zero-depth result in native shaders after recovering the loaded D3D9 instructions and verifying zero-multiply arithmetic on the GPU. Found in / Belongs to Blur1PCDX11. Road depth repair is committed as f8344dfd.

Verified: sequential standard/probe rebuilds succeed; hardware and WARP pass eight billboard depth checks plus previous checks. D3D9 reference passes25. Rebuilt standard exe/effects delivered; final queued minimized race exits0 with clean diagnostics and no nonfinite depth pixels. Logs and inspected captures: _work/build/foliage_depth and billboard_depth_final. The initial concurrent generator failure is superseded by the sequential rebuild.

Blocked: none. Full parity remains T127. The new stroke GPU capture reproduces NaN positions/UVs before conversion; UI repair verification follows.

Next: commit the billboard repair; finish hardware/WARP verification and deliver the packed stroke/trail/gradient/multiply conversions, then compare the resulting race UI with PC.

## 2026-09-30 05:15 — session Codex visual diagnosis — T127

Did: restored numeric SHORT2 input conversion for stroke/trail/gradient/multiply while preserving each pristine PC shader's coordinate and UV math. Billboard zero-depth repair committed as4a847184. Found in / Belongs to Blur1PCDX11.

Verified: captured actual stroke failure before repair (NaN/incorrect UV), then 88 output components pass hardware/WARP across all four paths. Full probes, standard Release and probe build pass; standard exe/PDB and regenerated shaders/effects delivered, hashes match. Queued minimized race exits0 with clean debug/missing-shader logs. Inspected final race and PC reference; empty purple power-up boxes match PC, while contact shadow needs a targeted check.

Blocked: none. Broader visual parity remains in progress; screenshot comparison alone does not establish a shadow fault mechanism.

Next: commit packed UI repair, trace native screen-blob shadow draw2809 resources/constants/output and recover its corresponding loaded D3D9 shader/state. Preserve accepted rendering decisions.

## 2026-09-30 05:35 — session Codex visual diagnosis — T127

Did: committed packed UI repair16f389a2; recovered actual PC screen-shadow draw7068883 and isolated native inputs. Fixed pixel parameter/array uploads to use the complete reflected span, faithfully following pristine D3D9. Found in / Belongs to Blur1PCDX11.

Verified: before/after actual GPU matrix binding, matching shadow/stencil texture contents, restored contact shadow in inspected final capture. Standard Release exits0/errsum0; standard exe/PDB/effects delivered, hashes match; queued minimized race exits0 and severe/missing-shader diagnostics are clean. Logs/captures: blob_shadow_before, blob_shadow_trace and blob_shadow_final.

Blocked: none. Full vehicle/reflection and rendering parity remains T127.

Next: commit the reflected-span repair, then compare the corrected race against PC for the next reproducible visual discrepancy. Keep diagnostic captures separate from delivered output.


## 2026-09-30 06:20 — session Codex visual diagnosis — T127

Did: committed reflected pixel-span repair1df3e064. Compared race paint constants, packed inputs and texture contents against actual PC draw7057314. Repaired the retail damage-volume name alias in native reflection/metadata and restored PC's existing vertex-default volume substitution. Found in / Belongs to Blur1PCDX11.

Verified: all98 recovered volume-using effects pass alias/reflection checks; three actual paint VS volume-alpha samples pass hardware/WARP. Standard Release exits0/errsum0; probe builds/full runs pass. Standard exe/PDB/effects delivered, hashes match; minimized queued race exits0 with clean severe/missing-VS diagnostics and actual GPU volume binding now R32G32B32A32_FLOAT / Texture3D. Evidence: paint_lighting_trace and vehicle_volume_final. Paint texture exact; cube/dent/scratch mip0 comparisons within one byte/channel; lighting constants match closely.

Blocked: none. Restored binding/default does not resolve or establish full vehicle paint parity.

Next: commit the volume binding/default repair, then compare environment cube-map mip levels and actual material outputs. Keep broader T127 active.

## 2026-09-30 08:27 — session Codex visual diagnosis — T127

Did: committed the verified native occlusion/particle initialization repair: enable the existing pristine PC setup for DEVICE_PCD3D11 and extend shader linkage checks to the affected stages. Found in / Belongs to Blur1PCDX11.

Verified: hardware and WARP pass all 147 built-in shader interface pairs. Standard Release visual_colour_matrix_build exit0/errsum0, delivered executable/PDB/effects. Queued minimized race colour_matrix_trace exits0 without missing vertex shaders or rejected draws. The colour matrix reflects register2/count4 and current CPU/GPU values agree; the earlier identity result remains unexplained.

Blocked: none; intermittent volume-lock crash and stable paint brightness parity remain open.

Next: consume the latest ordinary baseline, compare the current frame with the PC reference, and recover the first stable differing mechanism. T127 remains active.

Files touched: OcclusionQuery.cpp, particle Manager.cpp, dx11_probe/main.cpp selected linkage additions, phase analysis/board/ledger.

## 2026-09-30 08:38 — session Codex visual diagnosis — T127

Did: committed the verified PC stencil contract: retain CW/CCW banks and the two-sided enable flag, map bank orientation using the rasterizer, apply one-sided state to both faces and retain PC CCW reference/mask no-ops. Found in / Belongs to Blur1PCDX11.

Verified: pristine D3D9Device.inl comparison; hardware/WARP stencil_probe logs each pass 24 GPU stencil-byte cases. Standard Release visual_lut_capture_build exit0/errsum0 and delivered. Queued colour_lut_trace exits0. Current GPU LUT matches the PC's entire 131072-byte upload; current CPU/GPU colour matrix is identity again, so its variability precedes GPU upload and remains unresolved.

Blocked: none; colour/paint stability and intermittent volume-lock crash remain open.

Next: inspect live colour-correction setting in the queued native capture; recover the settings mechanism before proposing a colour fix. T127 remains active.

Files touched: native D3D11States helper and selected game-device stencil paths, dx11_probe selected GPU stencil check, phase analysis/board/ledger.

## 2026-09-30 08:55 — session Codex visual diagnosis — T127

Did: commit the verified PC luminance, per-sampler gamma and live sampler contract, with shared/immediate FX updates and prevention of legacy constants overwriting FX11 buffers. Preserve serialized/locked PC bytes while using expanded native luminance storage. Correct the probe's explicit fstream dependency, committed Get interface calls and effect-count threshold. Found in / Belongs to Blur1PCDX11.

Verified: indexed-source MSVC compilation passes 14 translation units; indexed probe links and queued hardware/WARP runs exit0, including 58 luminance samples, 48 exact round trips, 24 gamma transitions and eleven live samplers each. D3D9 reference passes23. Current standard Release visual_colour_setting_build exit0/errsum0 and delivered; queued race exits0. Both GPU lookup volume readbacks match all131072 PC upload bytes. CPU/GPU colour rows agree; live colour correction enabled; nonidentity correction directions agree with PC at different strengths.

Blocked: none. Matching the active timed post effect remains unverified; paint stability and intermittent volume-lock crash remain open.

Next: compare paint luminance/exposure inputs at the bright and dark draws before final composition; recover the first differing upstream mechanism. T127 remains active.

Files touched: native display/device/context/texture format and sampler paths, Display2 luminance resource/lock transport, FX shared/immediate binding, owned probe sources/headers and CMake reference target, phase docs.

## 2026-09-30 09:31 — session Codex visual diagnosis — T127

Did: isolated the entire bright/dark paint difference to live RearLightColour, using identical-input GPU replay and opt-in CPU upload evidence. Supersedes earlier paint/getter instability claims; no production workaround or shader tuning. Found in / Belongs to Blur1PCDX11 for native diagnostics; upstream reverse-light launch state remains unclassified.

Verified: private actual-mesh/material replay reproduces both RGBA outputs by changing only row7; hardware/WARP agree within one RGB byte. CPU shared upload and GPU agree at zero and .1 in two fresh combined scene captures. Queued runs exit0. Standard Release visual_rear_light_cpu_all_build exit0/errsum0; standard executable/PDB/effects delivered.

Blocked: none. Active timed post effect and neutral-launch reverse-light state are not matched; intermittent volume-lock crash remains open.

Next: compare protected final-composition textures and reflection/road output against PC at matching rear-light state. T127 remains active.

Files touched: NativeFX11.cpp opt-in CPU diagnostic; phase analysis/board/ledger. Private replay and capture harness stay under ignored _work/.


## 2026-10-02 23:08 — session Codex visual diagnosis — T127

Did: verify same-input final composition against D3D9 and capture road material/reflection inputs after matched-resolution PC/native runs. No production tuning. Found in / Belongs to Blur1PCDX11.

Verified: standard Release parity_oct02_build exit0/errsum0 and delivered exe/PDB/effects. Queued PC60, native60 and native unlocked races exit0. Actual final-composition hardware replay differs in five channels by one byte; WARP maximum one byte. Road static material textures within one decoder byte; live road sampler/constant contract agrees with PC.

Blocked: none. Native/PC lightmap residency differs; old road trace resolution and elapsed state differ. Intermittent damage-volume loading crash remains open.

Next: compare road lightmap at matching resident dimensions, then road vertex inputs and reflection producer. T127 remains in progress.

Files touched: phase analysis, board and ledger. Private replay/capture tools remain under ignored _work/.


## 2026-10-02 23:52 — session Codex visual diagnosis — T127

Did: restore retail saturation in the road prepass normal blend and remove the extra RoadTwoSpecN normal-axis swap for DX11. Found in / Belongs to Blur1PCDX11. The foreground noisy paving now renders smoothly.

Verified: actual retail SM3 bytecode versus captured native mesh/texture/constants. Road hardware comparison is byte-exact over 211,463 covered pixels; RoadTwoSpecN controlled-camera comparison is byte-exact over 291,035 pixels. WARP residuals remain explicitly bounded in analysis. Standard road_normal_clamp_build and road_twospec_axis_build exit0/errsum0; 1254 effects/693 built-ins regenerated. Queued native60/unlocked and updated native60 races exit0. Standard exe/PDB/effects delivered; executable hash agrees with build Release. Protected final road lighting inputs in road_paving_lights_oct02.

Blocked: none. Full reflection/lighting and uncaptured retail variant parity remain unverified; intermittent damage-volume loading crash remains open.

Next: compare foreground Road_LN_ND_NG / MaterialLights draw 7059193 against actual retail bytecode using the protected identical inputs. T127 remains in progress.

Files touched: RoadTwoSpecN.fx, Special/RoadSurface.h, phase analysis/board/ledger. Private GPU replay/capture artifacts remain under ignored _work/.


## 2026-10-03 00:16 — session Codex visual diagnosis — T127

Did: verify foreground road final lighting and the road reflection producer against actual retail shader bytecode; protect the next general scenery reflection draw. No production adjustment. Found in / Belongs to Blur1PCDX11.

Verified: road final-lighting hardware replay byte-exact over212,024 covered pixels; road reflection hardware replay byte-exact over56,827 covered pixels. WARP final-lighting max5/reflection max1. Both queued probes exit0. Native road reflection and general scenery reflection capture races exit0. Existing standard Release road repairs remain delivered; this checkpoint adds evidence only.

Blocked: none. Other material/reflection permutations and damage-volume crash remain open.

Next: consume queued default_lod_replay hardware/WARP results and compare the captured general reflection material against retail draw7048564. T127 remains in progress.

Files touched: phase analysis, board and ledger. Private tools and replay artifacts remain under ignored _work/.


## 2026-10-03 00:25 — session Codex visual diagnosis — T127

Did: verify the selected general scenery material through reflection, normal/depth prepass and final lighting using actual retail shaders and identical protected inputs. No production tuning. Found in / Belongs to Blur1PCDX11.

Verified: reflection hardware exact on captured and controlled cameras; prepass hardware normal colour and R32F depth exact; final lighting differs by one byte in one channel. WARP bounds and one reflection coverage residual recorded in analysis. All queued captures/private replays exit0. Existing standard road fixes remain delivered.

Blocked: none. Broader variants and intermittent damage-volume crash remain open.

Next: consume live driving_parity_oct03 batch (PC60/native60/native unlocked), inspect compressed videos and final stills for moving rendering differences, and classify the next reproduced fault. T127 remains in progress.

Files touched: phase analysis, board and ledger. Private artifacts remain under ignored _work/.


## 2026-10-03 00:41 — session Codex visual diagnosis — T127

Did: repair DX11 splash PS and ordinary-particle signature registers, preserving the retail stage selection and shader math. Extend the signature regression to166 actual stage pairs. Add the FX pass name to existing bounded debug-layer diagnostics. Found in / Belongs to Blur1PCDX11.

Verified: standard moving_shader_links_build and moving_binding_diagnostic_build errsum0;693 native built-ins regenerated/embedded; standard exe/PDB/effects delivered. Signature check passes166 pairs. Queued corrected moving native60/unlocked races exit0; both prior linkage errors are gone. PC driving reference reaches133mph/60Hz. Native moving performance remains about6fps; cube/2D binding error persists and is identified as ScrollFade_LN_NL / Material. Native video requests are unsupported; these checks use one final still.

Blocked: none. Performance, ScrollFade binding, broader visual variants and intermittent damage-volume crash remain open.

Next: consume scrollfade_moving_oct03 selected-draw capture and recover the cube sampler's actual resource/default mechanism against retail before changing binding. T127 remains in progress.

Files touched: SplashCamera psSplashCamera.psh, dynamic Particle.h, native PCD3D11GameDevice diagnostics, owned dx11_probe linkage check, phase analysis/board/ledger. Private artifacts remain under ignored _work/.


## 2026-10-03 00:54 — session Codex visual diagnosis — T127

Did: recover PC cube-sampler reads with bound2D textures and repair the native FX dimension mismatch. Add direct PC/native GPU regressions, preserving valid cubes. Found in / Belongs to Blur1PCDX11.

Verified: PC67 GPU checks; native54 checks on hardware and WARP without debug errors; standard scrollfade_cube_binding_build errsum0 and exe/PDB/effects delivered. Queued moving native60/unlocked both exit0/60s/wait0; prior linkage and cube-dimension errors gone. Moving performance remains poor. Owned-process sampling identifies repeated diagnostic getenv calls as a substantial CPU cost.

Blocked: none. Broader visual variants, slowdown and intermittent damage-volume crash remain open.

Next: cache process diagnostic options, build/deliver, repeat moving60/unlocked, and profile remaining hot paths. T127 remains in progress.

Files touched: native D3D11Effect texture binding, owned cube_binding_checks and D3D9 sampling reference, owned probe include/check hunks, phase analysis/board/ledger. Private artifacts remain under ignored _work/.


## 2026-10-03 01:20 — session Codex visual diagnosis — T127

Did: cache hot launch-time native diagnostic options and retain this session's bounded GPU evidence helpers. Found in / Belongs to Blur1PCDX11.

Verified: standard diagnostic_cache_build errsum0 and delivered; indexed2/2 translation units compile. Queued moving60/unlocked exit0; further debug-enabled60 exits0 without validation errors. Environment scanning drops from the leading sampled costs. Car reaches132mph. Dynamic vertex staging maps now dominate the unlocked profile (665/1275 caller stacks).

Blocked: none. Native dynamic locks ignore the pristine caller's discard/no-overwrite flags; visual parity and damage-volume crash remain open.

Next: finish the native dynamic-lock repair and queued GPU retention regression, then consume moving60/unlocked verification and profile remaining cost. T127 remains in progress.

Files touched: owned NativeFX11 cache hunks, owned D3D11GameDeviceNative diagnostics (separate alpha-test edits excluded), phase analysis/board/ledger. Private artifacts remain under ignored _work/.


## 2026-10-03 01:39 — session Codex visual diagnosis — T127

Did: forward retail discard/no-overwrite promises to native dynamic vertex/index maps, removing staging readbacks. Add the standard GPU regression. Repair the reported private delivery/console wrapper. Found in / Belongs to Blur1PCDX11.

Verified: standard builds errsum0; indexed2/2 translation units compile;64 queued GPU draws pass on hardware and WARP without debug errors. Standard EXE/PDB/effects delivery verified. Moving60/unlocked and corrected-wrapper debug60/unlocked all exit0, no previous validation/lock failures. Locked-file failure prevents launch; helper output is private. Final still reaches131mph. Remaining texture-view creation cost profiled.

Blocked: none. Broader visual parity and intermittent damage-volume crash remain open.

Next: build/test the current-view reuse candidate, compare fresh/reused hardware/WARP sampling across resource/gamma/null/reload transitions, then verify moving60/unlocked and profile. T127 remains in progress.

Files touched: owned D3D11Context lock hunks, owned cmake diagnostic target, dynamic_buffer_checks/reference, phase analysis/board/ledger. Private harness remains under ignored _work/.


## 2026-10-03 01:53 — session Codex visual diagnosis — T127

Did: reuse the FX variable's retained native texture view when resource/device/active descriptor match. Preserve gamma and resource changes, null binding and reload. Found in / Belongs to Blur1PCDX11.

Verified: standard build and diagnostic target errsum0/exit0; indexed4/4 source units compile;84 fresh/reused GPU comparisons pass on hardware and WARP without debug errors. Standard EXE/PDB delivery verified. Moving debug60/unlocked exit0/46s/wait0, no previous validation/lock/view failures. Final still reaches134mph. Unlocked engine reports roughly25–32fps; parity remains incomplete.

Blocked: none. Broader variants and intermittent damage-volume crash remain open.

Next: cache repeated reflected variable lookups with effect-reload invalidation; verify uniform/resource/reload behavior on hardware/WARP, then moving60/unlocked and remaining CPU costs. T127 remains in progress.

Files touched: owned D3D11Effect/Formats headers and implementations, owned NativeFX11 binding hunk and cmake diagnostic target, texture_view_reference, phase analysis/board/ledger. Private artifacts remain under ignored _work/.


## 2026-10-03 02:09 — session Codex visual diagnosis — T127

Did: index stable native FX reflection at load; preserve alias and absent-name behavior, clear on reload, keep render-time reads immutable. Found in / Belongs to Blur1PCDX11. The user's reported private copy/visible-console fault was repaired earlier in this continuation.

Verified: standard build/diagnostic target errsum0/exit0; indexed2/2 units compile; hardware/WARP84 paired view comparisons plus12 changed-uniform GPU draws pass, including renamed/missing variables on reload and vehicle alias. EXE/PDB delivered and hashes match. Moving debug60/unlocked exit0/46s/47s/wait0, no validation/lock/view failures; coherent131mph still. GetVariableByName disappears from the owned main-thread sample. Rendering remains25.1–29.0fps in the profiled moving fixture.

Blocked: none. Performance parity, broader variants and intermittent damage-volume crash remain open.

Next: instrument remaining SRV creation reasons in a bounded opt-in fixture, distinguishing live resource/gamma changes from cube/2D defaults; use recovered binding behavior to remove proven redundant work. Then hardware/WARP and moving60/unlocked verification. T127 remains in progress.

Files touched: owned D3D11Effect header/implementation, texture_view_reference, phase analysis/board/ledger. Private artifacts remain under ignored _work/.


## 2026-10-03 03:09 — session Codex visual diagnosis — T127

Did: finish bounded SRV cause statistics and actual-input crash composition replay; focus on clarified intermittent geometry beyond the right barrier. Native reproduction saved; no guessed visual fix.

Verified: clean standard build errsum0 and delivery hashes match; selected PC/native crash replay max1 byte on hardware/WARP, including unlocked capture. PC compressed reference exit0. Three scrape traces exit0; sampled pixels miss the fault. Opt-in colour trigger builds errsum0; refining its first crash-fringe false positive to target horizontal strips.

Blocked: none. Affecting draw/root cause unconfirmed; parity remains incomplete.

Next: catch an affected strip, identify its GPU producer and compare pristine PC mechanism; repair and verify60/unlocked, remove temporary diagnostics, deliver and commit own changes.

Files touched: own temporary native Present diagnostic and phase analysis/board/ledger; private captures/replays/logs under ignored _work/.


## 2026-10-03 — session Codex visual diagnosis — T127 barrier scrape repair

Did: identify stale GPU particle vertices behind the saturated barrier strips; recover retail's true pass-flush default and restore it under the actual native DX11 build guard. Remove temporary diagnostics and the failed shared-range publication experiment. Found in / Belongs to Blur1PCDX11; PC own-executable paired reference remains clean.

Verified: explicit native false/true isolation reproduces/clears malformed geometry; retail constructor/render/flush functions recovered read-only. Corrected standard Release errsum0; compiled native object confirms true and the flush call. Standard EXE/PDB delivered with matching hashes. Four final queued early/late scrape runs, 60-cap/unlocked, exit0/199s/wait0; four inspected stills clean with no debug configuration override or DX11 diagnostic environment. Actual render rate below60; performance gain from this glitch is not established.

Blocked: none for this repair. The initial wrong build macro excluded the attempted default change; its runs were not validation of the final fix. Broader T127 parity, uncaptured variants and intermittent damage-volume crash remain open.

Next: assess existing material SRV creation statistics and recover safe ownership/invalidation before further performance changes; verify any resulting change on hardware/WARP and moving60/unlocked.

Files touched: own dynamic/source/View.cpp gate/default and phase analysis/board/ledger. Private captures, retail recovery and discarded experiment artifacts remain under ignored _work/ only. Commit own repair/docs only, as previously authorized; no push.


## 2026-10-03 — session Codex visual diagnosis — T127 texture ownership and reported recurrence

Did: retain equivalent linear/sRGB native views for the real engine texture lifetime, including serialized assets and streaming replacements. Investigate the user's renewed right-railing glitch; add and then fully remove temporary locked-buffer and compressed-video diagnostics. Preserve the recovered retail pass-flush default; no speculative visual fix added.

Verified: hardware/WARP each pass228 paired GPU comparisons,12 uniform checks,12 destruction markers. Standard asset build and clean diagnostic-removal build errsum0. Moving cap60/unlocked pair exit0/103s; completed locked-buffer diagnostic pair exit0/113s, no still-locked draw comparison triggered. Two compressed videos,720 frames each, ffmpeg/game exit0/143s. Inspected strongest colour candidates do not identify the original malformed strips. Clean standard EXE/PDB delivered through queue and hashes verified. Runtime rates remain variable; no controlled overall speedup established.

Blocked: the returning visual fault is not yet isolated. The earlier clean stills are insufficient to claim it fully repaired. Diagnostic attempts also encountered a loading resource wait timeout and a vehicle-reset AI null crash; the first full-size recorder attempt failed, then was bounded and verified. All diagnostic source changes removed. Broader parity remains open.

Next: obtain an affected frame behind the right railing, capture its producer and compare actual GPU vertex/state data with the PC mechanism. Commit only the verified view ownership change and these owned docs, as already authorized; no push.

Files touched: D3D11Formats.h/cpp, PCD3D11Device.cpp, D3D11TextureLoad.cpp, dx11_probe/texture_view_reference.cpp, two-line native helper dependency in cmake/dx11.cmake, owned phase docs. Temporary diagnostics removed from DynamicVertexBuffer.cpp and PCD3D11GameDevice.cpp; scratch evidence under ignored _work/.


## 2026-10-03 — session Codex visual diagnosis — T127 railing-focused follow-up

Verified: clean delivered executable, queued scrape_recurrence_barrier_oct03 exit0/51s/wait0; existing trace samples(.95,.49) at40s, no saturated producer at that pixel; final still lacks the earlier malformed strips. Texture ownership committed e03536b0. No source changes or diagnostic instrumentation added. Board remains in progress.

Next: await confirmation whether the displayed unlocked-video pink band is the returning fault, then trace its producer with an affected frame. Recurrence remains open; a clean selected pixel is not a complete repair. Evidence under ignored _work/build/scrape_recurrence_barrier_oct03.
