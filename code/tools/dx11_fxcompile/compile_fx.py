"""Compile a Blur PC effect source into native Direct3D 11 FX5 bytecode.

The original effect files use FX9's ``shared`` globals and SM3 compile
profiles. FX5 requires those globals in ordinary constant buffers and SM5
profiles. This tool transforms only the generated preprocessed source; it
never edits the authored effect files or retail data.
"""

from __future__ import annotations

import argparse
from pathlib import Path
import re
import subprocess
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "dx11_shadercompile"))
from alpha_test import inject_effect_alpha_test


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--fxc", required=True, type=Path)
    parser.add_argument("--source", required=True, type=Path)
    parser.add_argument("--out", required=True, type=Path)
    parser.add_argument("--include", action="append", default=[], type=Path)
    parser.add_argument("--define", action="append", default=[])
    args = parser.parse_args()

    output = args.out.resolve()
    output.parent.mkdir(parents=True, exist_ok=True)
    preprocessed = output.with_suffix(".preprocessed.fx")
    transformed = output.with_suffix(".fx5.fx")
    command = [str(args.fxc), "/nologo", "/P", str(preprocessed)]
    for directory in args.include:
        command.extend(("/I", str(directory.resolve())))
    for definition in args.define:
        if not re.fullmatch(r"[A-Za-z_]\w*", definition):
            parser.error(f"invalid effect define: {definition}")
        command.append(f"/D{definition}=1")
    command.extend(("/DPLATFORM_PC=1", "/DBUILD_CONFIG_SPEED=1",
                    "/Dvs_3_0=vs_5_0", "/Dps_3_0=ps_5_0",
                    str(args.source.resolve())))
    if subprocess.run(command).returncode != 0:
        return 1

    source = preprocessed.read_text(encoding="utf-8-sig").rstrip("\0")
    source = re.sub(r"\bshared\s+", "", source)
    # FX9 binds independent vertex/pixel c-register files. FX5 packs global
    # values into constant buffers, so the old explicit register assignments
    # collide. The native renderer addresses parameters by reflected name.
    source = re.sub(r":\s*register\s*\([^)]*\)", "", source)
    # FX5 ignores the original FX9 min/mag/mip sampler properties. The game
    # uses either point for all three or linear for all three in every sampler.
    # The old sRGB flag belongs on the DX11 texture view, so keep a sidecar
    # recording it for the native texture binding path.
    sampler_records: list[str] = []
    sampler_block = re.compile(
        r"\bsampler\s+([A-Za-z_]\w*)\s*=\s*sampler_state\s*\{([^{}]*)\}",
        re.DOTALL,
    )

    def convert_sampler(match: re.Match[str]) -> str:
        body = match[2]
        values = {key.lower(): value.strip() for key, value in re.findall(
            r"(?m)^\s*([A-Za-z_]\w*)\s*=\s*([^;]+);", body)}
        filters = {values.get(key, "LINEAR").upper()
                   for key in ("minfilter", "magfilter", "mipfilter")}
        if len(filters) != 1 or next(iter(filters)) not in ("POINT", "LINEAR"):
            raise ValueError(f"sampler filter needs native port: {match[1]} in {args.source}")
        filter_name = "MIN_MAG_MIP_" + next(iter(filters))
        texture = values.get("texture", "").strip("< >")
        sampler_records.append(
            f"{match[1]}\t{texture}\t{int(values.get('srgbtexture', 'FALSE').upper() == 'TRUE')}"
            f"\t{filter_name}\n"
        )
        body = re.sub(r"(?m)^\s*(?:MinFilter|MagFilter|MipFilter|SRGBTEXTURE)\s*=\s*[^;]+;", "", body)
        # FX9 defaults to wrapping texture coordinates. FX11's legacy sampler
        # syntax defaults to CLAMP, so preserve each omitted PC address mode.
        for axis in "UVW":
            if "address" + axis.lower() not in values:
                body += f"\nAddress{axis} = WRAP;"
        return (f"sampler {match[1]} = sampler_state {{{body}\n"
                f"Filter = {filter_name};\n}}")

    source = sampler_block.sub(convert_sampler, source)
    output.with_suffix(".samplers").write_text("".join(sampler_records), encoding="utf-8")
    source = re.sub(r"\btechnique\b", "technique11", source)
    shader_assignment = re.compile(
        r"\b(Vertex|Pixel)Shader\s*=\s*compile\s+(vs_5_0|ps_5_0)\s+"
        r"([A-Za-z_]\w*)\s*\(\s*\)\s*;"
    )
    source = shader_assignment.sub(
        lambda match: f"Set{match[1]}Shader(CompileShader({match[2]}, {match[3]}()));",
        source,
    )
    # FX9 fixed-function pass assignments need native D3D11 state objects.
    # Alpha test is implemented by injecting a pixel shader discard below.
    pass_block = re.compile(r"\bpass\s+([A-Za-z_]\w*)\s*\{([^{}]*)\}", re.DOTALL)
    state_assignment = re.compile(r"(?m)^\s*([A-Za-z_]\w*)\s*=\s*([^;]+);\s*$")
    technique_names = [(m.start(), m[1]) for m in re.finditer(
        r"\btechnique11\s+([A-Za-z_]\w*)", source)]
    native_states: list[str] = []
    state_records: list[str] = []

    def convert_pass(match: re.Match[str]) -> str:
        original = match[2]
        states = {key: value.strip() for key, value in state_assignment.findall(original)}
        technique = next((name for pos, name in reversed(technique_names)
                          if pos < match.start()), "")
        for key, value in states.items():
            state_records.append(f"{technique}\t{match[1]}\t{key}\t{value}\n")
        body = state_assignment.sub("", original)
        serial = len(native_states)
        calls: list[str] = []
        if any(k in states for k in ("AlphaBlendEnable", "SrcBlend", "DestBlend", "BlendOp")):
            enabled = states.get("AlphaBlendEnable", "FALSE").upper()
            blend_names = {"one": "ONE", "zero": "ZERO",
                           "srcalpha": "SRC_ALPHA", "invsrcalpha": "INV_SRC_ALPHA"}
            source_blend = blend_names[states.get("SrcBlend", "ONE").lower()]
            dest_blend = blend_names[states.get("DestBlend", "ZERO").lower()]
            operation = states.get("BlendOp", "ADD").upper()
            blend_name = f"BlurNativeBlend{serial}"
            native_states.append(
                f"BlendState {blend_name} {{ BlendEnable[0] = {enabled}; "
                f"SrcBlend[0] = {source_blend}; DestBlend[0] = {dest_blend}; "
                f"BlendOp[0] = {operation}; SrcBlendAlpha[0] = {source_blend}; "
                f"DestBlendAlpha[0] = {dest_blend}; BlendOpAlpha[0] = {operation}; "
                "RenderTargetWriteMask[0] = 15; };\n"
            )
            calls.append(f"SetBlendState({blend_name}, float4(1,1,1,1), 0xffffffff);")
        if "ZWriteEnable" in states:
            write_mask = "ALL" if states["ZWriteEnable"].upper() == "TRUE" else "ZERO"
            depth_name = f"BlurNativeDepth{serial}"
            native_states.append(
                f"DepthStencilState {depth_name} {{ DepthEnable = TRUE; "
                f"DepthWriteMask = {write_mask}; DepthFunc = LESS_EQUAL; }};\n"
            )
            calls.append(f"SetDepthStencilState({depth_name}, 0);")
        if "CullMode" in states:
            cull = states["CullMode"].upper()
            if cull not in ("NONE", "CW", "CCW"):
                raise ValueError(f"unsupported authored cull mode: {cull}")
            raster_name = f"BlurNativeRaster{serial}"
            native_states.append(
                f"RasterizerState {raster_name} {{ FillMode = SOLID; "
                f"CullMode = {'NONE' if cull == 'NONE' else 'BACK'}; "
                f"FrontCounterClockwise = {'TRUE' if cull == 'CW' else 'FALSE'}; "
                "DepthClipEnable = TRUE; };\n"
            )
            calls.append(f"SetRasterizerState({raster_name});")
        return f"pass {match[1]} {{{body}\n{' '.join(calls)}\n}}"

    source = pass_block.sub(convert_pass, source)
    source = inject_effect_alpha_test(source)
    output.with_suffix(".states").write_text("".join(state_records), encoding="utf-8")
    source = "".join(native_states) + source
    transformed.write_text(source, encoding="utf-8")
    return subprocess.run(
        [str(args.fxc), "/nologo", "/Gec", "/Zpr", "/T", "fx_5_0", "/Fo", str(output),
         str(transformed)],
    ).returncode


if __name__ == "__main__":
    raise SystemExit(main())
