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
