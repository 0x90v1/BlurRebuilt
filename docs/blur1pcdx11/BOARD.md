# Board (Blur1PCDX11)

Claim a task before working on it. One task per agent. Keep rows short; details go in the ledger. Statuses: `open`, `in progress`, `blocked`, `review`, `done`. Add new tasks at the bottom of the right milestone; never renumber.

| ID | Milestone | Task | Owner | Status | Files / scope | Blocked on |
|---|---|---|---|---|---|---|
| T127 | M11 | Create `blur1pcdx11` from `blur1pc` and port the PC renderer to native Direct3D 11, keeping the PC D3D9 phase intact | session Claude DX11 | in progress (start screen and debug race present since 2026-09-29; open: car body (T148), near road white, no sRGB reads) | Blur1PCDX11: CMake, display device/d3d11, shader sources, dx11 tools, vendor/fx11, docs/blur1pcdx11 | T148 for the car; road and sRGB open |
| T148 | M11 | Reconstruct the shipped vehicle techniques (`MaterialPrepass`, `MaterialPrepassNoDamage`, `MaterialNoDamage*`, `MaterialAlphaLuminanceNoDamage*`) from the microcode dumped by `System::OnEffectLoad` into `_work/build/fxdump/`, as HLSL under `BLUR_DX11` | — | open | shader sources, `vehicle_common.h`, dump files | T127 |
| T149 | M11 | Find the link of the post-process chain that loses the frame: capture the chain texture at each `I_PostProcess::DisplayIntermediate` (post-process off presents, post-process on is black) | session Claude DX11 | done (2026-09-29: texture registers, `Unbind`, FX texture values, half-pixel offset; analysis.md) | display device d3d11, `Render.cpp`, `NativeFX11.cpp` | — |
| T150 | M11 | Fix `UI::C_UiManager::LoadingTextureClone` crashing on a null source when a debug race's loading screen opens under DX11 | session Claude DX11 | done (2026-09-29: `Display2::CloneTexture2D` copied between two null CPU addresses on D3D11; it now copies on the GPU under `DEVICE_D3D11`) | `display2/source/utils/Utils.cpp` | — |

| T176 | M11 | Reduce agent context inflation; shared instructions, bounded task/log/usage helper | Codex context-efficiency | done | AGENTS.md; docs/agent-reference; tools/agent_context.py; phase board/ledger | none |
