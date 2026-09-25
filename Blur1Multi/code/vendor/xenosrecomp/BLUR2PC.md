# XenosRecomp in Blur1Multi

Upstream: https://github.com/hedge-dev/XenosRecomp, commit
`990d03b28a27b50277ee5d8d942e1c5f873869d1` (2025-08-03), MIT licence
(`LICENSE.md`), the same revision Blur2Test builds in `_work/XenosRecomp`.

Vendored: the recompiler only (`shader_recompiler.{h,cpp}`, `shader.h`,
`shader_code.h`, `constant_table.h`, `shader_common.h` for reference). Not
vendored: `main.cpp` and `dxc_compiler.*` (its own cache writer and DXC).

Local changes, all marked `[Blur1Multi]`:

- `pch.h` keeps only what the recompiler uses; `fmt/core.h` maps the {fmt}
  calls onto C++20 `std::format`.
- `XENOS_RECOMP_D3D9`: HLSL for Direct3D 9 shader model 3 (`vs_3_0` /
  `ps_3_0`), the target of the Blur1Multi shader cache (decisions D03, D11,
  board T074): constants and samplers on the registers the Xenos constant
  table assigns, `bool` constants on `b` registers, only the interpolators the
  program declares, `VPOS`/`VFACE`, literal constants as float literals, the
  vertex fetch decoded from the instruction's own format against the PC vertex
  formats the converter writes, and no D3D12 half-pixel or alpha-test
  specialisation (Direct3D 9 has both in fixed function).
