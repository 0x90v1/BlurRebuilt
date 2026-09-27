# Board (Blur1PCDX11)

Claim a task before working on it. One task per agent. Keep rows short; details go in the ledger. Statuses: `open`, `in progress`, `blocked`, `review`, `done`. Add new tasks at the bottom of the right milestone; never renumber.

| ID | Milestone | Task | Owner | Status | Files / scope | Blocked on |
|---|---|---|---|---|---|---|
| T127 | M11 | Create `blur1pcdx11` from `blur1pc` and port the PC renderer to native Direct3D 11, keeping the PC D3D9 phase intact | session Codex DX11 | in progress (native DXGI/resources, 1,254 FX5 effect variants and 693 SM5 built-in variants; FX11 material and built-in draw pass hardware/WARP probes; `Amax` with DX11 selected still does not compile) | Blur1PCDX11: CMake, display device/d3d11, built-in shader variants, dx11_fxcompile, dx11_shadercompile, dx11_probe, vendor/fx11, README; Blur1PC docs/{BOARD,LEDGER,analysis}.md | Native Display/Display2 backend types and resources, `FX::Manager` game integration |
