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


