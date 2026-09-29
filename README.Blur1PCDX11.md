# Blur1PCDX11

This branch started at `blur1pc` commit `547ccfb1`. It contains an incomplete
native Direct3D 11 renderer. This branch defaults to `BLUR_USE_DX11=ON`, but
`Amax` does not compile with that option because Display and Display2 are not
fully implemented for D3D11. The option can be switched off to verify that
the base D3D9 game build remains intact.

The module at `code/mercury/library/graphics/display/{include,source}/device/d3d11/`
creates a feature level 11 device (hardware, then WARP), offscreen and DXGI
window targets with depth, native vertex/index buffers, PC texture uploads,
and Shader Model 5 programs. It draws with the repository's 2D colour,
textured and 3D colour shaders, including reflected named shader constants,
and reads pixels back. The shader changes are gated by `BLUR_DX11`, so the
existing Shader Model 3 build stays intact. Nested shader includes resolve
relative to the including file.

`dx11_fx_all` extracts the original PC Jam shader variants and compiles all
1,254 of them from the authored `.fx` sources into native FX5 effects. The
effects are indexed by their game filenames, with sampler sRGB metadata.
The FX5 conversion emits native blend, depth and rasterizer states and inserts
the engine's alpha test into affected pixel shaders. The vendored Microsoft
FX11 runtime loads and applies the effects. The probe draws a triangle with
the game's `ScreenFacingSprite` material and checks the `MeshEffect` state
objects. This shader path is not yet connected to the game's `FX::Manager`.
The probe also preloads all 1,254 FX11 binaries on hardware and WARP.
`dx11_shaders_all` compiles all 693 Jam-selected built-in shader variants to
SM5 `.cso` files. The probe draws using those compiled binaries; its source
shader compiler is retained for diagnosis.

From this worktree root, run:

```powershell
$cmake = 'C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
& $cmake -S . -B _work/build/vs -G 'Visual Studio 18 2026' -A Win32 -DBLUR_DX11_ONLY=ON
& $cmake --build _work/build/vs --config Release --target dx11_probe
& .\_work\build\vs\Release\dx11_probe.exe
```

`BLUR_DX11_WARP=1` forces the software device. `BLUR_DX11_DEBUG=1` requests
the D3D11 debug layer and prints validation messages. Set
`-DBLUR_DX11_ONLY=OFF` before building `Amax` in the same build directory.

## Work remaining before Amax can render in DX11

1. Add D3D11 implementations for the display manager and device API that
   `Display::C_Device` exposes. The current D3D9 implementation includes
   buffers, textures, surfaces, shaders, render states, queries and reset logic.
2. Port `Display2` resources and draw calls, including the game's render targets,
   dynamic buffers, texture formats and multisampling. Connect the native DXGI
   swap chain to the game's window and resize handling.
3. Bind the compiled built-in shaders and their constants/samplers in Display.
   The retail `.fxb` effects contain PC D3D9 shader bytecode; `FX::ImportFXB`
   and its consumers need to resolve the compiled FX5 effects by name and bind
   their reflected parameters through the native renderer.
4. Select the new backend only after its resources and shaders work. Verify it
   against retail PC rendering, then replace the user's executable through the
   normal phase delivery process.

No game run or DX11 game executable has been delivered from this branch.
