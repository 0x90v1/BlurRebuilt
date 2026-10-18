# Blur1PCDX11

This branch started at `blur1pc` commit `547ccfb1`. It contains an incomplete
native Direct3D 11 renderer. This branch defaults to `BLUR_USE_DX11=ON`.
`Amax` builds and reaches the start screen and debug races on retail PC data.
Rendering remains incomplete; see `docs/blur1pcdx11/analysis.md`
for fresh visual comparisons and `BOARD.md` for remaining work. The option
can be switched off to build the inherited D3D9 backend.

The module at `code/mercury/library/graphics/display/{include,source}/device/d3d11/`
creates a feature level 11 device (hardware, then WARP), offscreen and DXGI
window targets with depth, native vertex/index buffers, PC texture uploads,
and Shader Model 5 programs. It draws with the repository's 2D colour,
textured and 3D colour shaders, including reflected named shader constants,
and reads pixels back. The shader changes are gated by `BLUR_DX11`, so the
existing Shader Model 3 build stays intact. Nested shader includes resolve
relative to the including file.

`dx11_fx_all` extracts the original PC Jam shader variants and compiles all
1,254 of them into native FX5 effects. Authored sources are used except for
102 loaded vehicle permutations recovered from retail Shader Model 3 bytecode. The
effects are indexed by their game filenames, with sampler sRGB metadata.
The FX5 conversion preserves the engine's blend/depth/stencil/raster state,
keeps PC sampler address defaults, and inserts the engine's mutable alpha test into
pixel shader exits. The vendored Microsoft
FX11 runtime loads and applies the effects. The probe draws a triangle with
the game's `ScreenFacingSprite` material and checks that `MeshEffect` and
`Puddle` preserve engine state across repeated applies. This shader path is
connected to the game's `FX::Manager`.
Technique `Define` annotations produce separate native effects, following the
original exporter's preprocessing. The 1,254 source variants currently produce
4,913 native entries; the runtime selects them by the loaded FXB filename
(including permutation suffixes) and technique name. Internal effect names
are not unique across permutations.
The probe preloads the complete set on hardware and WARP and checks that the
road's lighting technique contains its dynamic-light texture binding. It also
checks all ten techniques of every recovered vehicle permutation and reads
back the alloy prepass's packed normal and negative clip-depth outputs.
It checks actual WRAP/CLAMP sampler bindings and alpha-test pixel rejection on
hardware and WARP, including every comparison function and reference boundaries.
It also checks 147 compiled shader interfaces and reads back RGBA results for
the PC colour blend factors used in native alpha blending.
It verifies per-sampler sRGB decoding with 24 GPU view transitions across six
colour formats, cube and volume textures; alpha and HDR values stay unchanged.
Twenty-four GPU stencil checks cover both triangle windings and one/two-sided state.
Legacy L8/A8L8/L16 storage preserves the PC sampled channels and original CPU
layout. Fifty-eight GPU samples cover mips, cube faces, volume textures and
gamma decoding before filtering; 48 GPU readbacks preserve the PC bytes.
Eleven GPU checks verify live FX11 sampler address/filter/border/LOD settings,
including immediate reapply, encoded PC bias and disabled mip selection.
The separate `d3d9_sampling_reference` target verifies 23 corresponding
legacy and live-sampler cases on D3D9. It is not linked into the native game.
(The x64 child branch removes this D3D9 probe with the rest of the D3D9 code;
see `docs/blur1pcdx11/x64.md`.)
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

## Current delivery and remaining work

The user's executable is `_work/run/Amax.exe`, with native effects beneath
`_work/run/fx11/`. Build `Amax` in `_work/build/vs`; it now depends on
`dx11_fx_all`. Delivery requires copying both `Amax.exe`/`Amax.pdb` and the
generated FX11 binaries and index/sampler/state metadata to the run directory.

The delivered build restores road/building textures, foliage cutouts and the
rear-view mirror, and removes the large white race overlay. The fixes preserve
PC sampler wrapping, streamed mip tails, runtime alpha testing and shadow
blend factors, and align the native shader interfaces. Per-sampler sRGB views
now honor live PC sampler flags, including immediate overrides, and remove the
washed-out road lighting. Occlusion and particle shaders initialize through the
existing PC path, restoring their missing draws and light effects. Native stencil
state preserves the PC winding rules and enable flag. Luminance textures expand
into compatible RGB storage while serialized data and lock layouts remain PC format.
Vehicle/reflection differences,
an intermittent damage-volume loading crash and uncaptured retail variants remain
open; complete parity is not claimed.
Use Showroom first for fast visual checks. The D3D9 reference is
`_work/build/dx9_compare/showroom_current_dx9.png`; the older September 29
reference is stale for focus comparison. Native captures are
`_work/build/showroom_live_sampler_final.png` and `race_live_sampler_final.png`.
Automated races must use the debug menu and the machine-wide
game slot, minimized without activation. Capture one still per check or record
compressed video; do not capture PNG bursts.

## Vehicle shader recovery

`BLUR_DX11_DUMP_VEHICLE_FX=1` captures all loaded vehicle passes into
`_work/build/fxdump_variants/` using their actual FXB filenames. The old
`fxdump/` files used internal names and could overwrite different permutations;
do not use them as permutation-specific evidence. No retail files are changed.
`code/tools/dx11_fxcompile/recover_vehicle.py` disassembles those captures with
FXC and generates the native-only sources under
`vehicle/include/vehicle/shaders/retail_dx11/`, retaining CTAB parameter names,
instruction comments and input SHA256 hashes. It rejects unsupported operations
instead of substituting an approximate shader. Only exact captured filenames
are substituted by `compile_manifest.py`; authored variants remain separate.
