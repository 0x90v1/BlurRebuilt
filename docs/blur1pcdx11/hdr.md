# HDR display support

Status: assessment only (2026-10-03). No task claimed, no decision recorded. HDR output was never shipped in Blur, so any HDR tone curve is a new feature and needs a user-confirmed decision before work starts.

## Current pipeline

- PC material shaders compile with `HDRU32` (`code/mercury/library/graphics/shader/include/shader/common/Hdr.h:6-10`; `PLATFORM_XENON` selects `HDR`). `GetExposure` writes `sqrt(colour * exposure)` into an 8-bit scene target with luminance in alpha. Exposed values above 1.0 clip at the material write, before bloom or tone mapping. The frame holds no range above SDR white.
- The Xbox 360 `HDR` path ("for use with 7e3 or 6e4 compressed float surfaces") uses the same sqrt encoding into a float surface, so values above 1.0 survive. This retail mechanism is the basis for an unclamped PC scene buffer.
- `PostProcessLayer.cpp:54-58` selects `E_HighDynamicRangeType_Shader` on non-PS3 platforms; the analysis records PC final composition running the `...HDRU32` tonemap variants.
- The post-process manager already maps `E_Format_A2B10G10R10F` views to `E_Format_A16B16G16R16F` textures (`render/source/postprocess/Manager.cpp:184-186, 654-655`); the luminance chain already uses `A16B16G16R16F` (`HDRInterface.cpp:195-199`).
- Swap chain: `DXGI_SWAP_EFFECT_DISCARD`, `DXGI_FORMAT_R8G8B8A8_UNORM` (`code/mercury/library/graphics/display/source/device/d3d11/D3D11Device.cpp:178-188`).

## Option 1: HDR output container

- Flip-model swap chain (`DXGI_SWAP_EFFECT_FLIP_DISCARD`) with `R10G10B10A2_UNORM` + `DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020` (HDR10) or `R16G16B16A16_FLOAT` + `DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709` (scRGB), set through `IDXGISwapChain3::SetColorSpace1`.
- Final encode pass maps the SDR image to a configurable paper-white level.
- Result: correct output on an HDR desktop, no extra highlight range. Windows Auto HDR and RTX HDR already approximate this for the current build.
- Flip model forbids a multisampled back buffer. If the T127 MSAA surfaces include the back buffer, presentation needs an explicit resolve.

## Option 2: real HDR

1. Scene targets `R16G16B16A16_FLOAT`; compile PC shaders with the 360 `HDR` path so exposed values above 1.0 survive.
2. Audit every scene-buffer consumer for 0-1 assumptions: bloom bright pass and luminance chain; final-composition variants (`psToneMapCustom*`, motion blur, crash/extra FX, film grain); the colour-correction lookup volume (a 0-1 LUT).
3. New HDR tone curve and PQ/scRGB encode in final composition; UI composited at paper white. No retail counterpart.
4. Peak-brightness and paper-white settings, seeded from `IDXGIOutput6::GetDesc1`.

Steps 1-2 reuse retail mechanisms. Step 3 is new code and must be labelled as such.

## Risks

- Analysis records byte-level parity of native shaders against PC on the `HDRU32` path. Switching scene format leaves that verified path; option 2 should be a runtime toggle that leaves the current path unchanged when off.
- 64-bit scene targets double scene bandwidth; negligible on DX11 HDR-capable GPUs.
- Must hold at unlocked frame rate like every other change; check luminance adaptation timing if touched.

## Recommendation

Option 2 behind a toggle. Start with step 1 alone and inspect how much highlight detail the 360 path recovers before writing a tone curve.
