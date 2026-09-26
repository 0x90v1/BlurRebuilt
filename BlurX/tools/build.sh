#!/bin/sh
# [BlurX] Regenerate from the jamfiles and build Release (Blur1PC's working tree
# and the committed Blur1Multi snapshot, see README.md "Building"). Run from BlurX/.
# Blur1PC jamfile edits (new shader variants, sources) need the jamextract step.
set -e
J=../Blur1PC/code/tools/jam2cmake
CM="/c/Program Files/Microsoft Visual Studio/18/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe"
python $J/jamextract.py --top ../Blur1PC --root code/amax/game/jamfile.jam --target WIN32 --platform PC --build Speed --out _work/build/targets_pc_speed.json | tail -1
python $J/shadergen.py --json _work/build/targets_pc_speed.json --out _work/build/gen/shaders --overlay _work/base/Blur1Multi --overlay . | tail -1
python $J/luabundle.py --json _work/build/targets_pc_speed.json --out _work/build/gen/blobs/amax_lua_bundle.cpp --overlay _work/base/Blur1Multi --overlay . | tail -1
python $J/gen_cmake.py --json _work/build/targets_pc_speed.json --out cmake/targets.cmake --project-dir . --overlay _work/base/Blur1Multi=BLUR1MULTI_DIR --overlay . | tail -1
"$CM" -S . -B _work/build/vs -G "Visual Studio 18 2026" -A Win32 | tail -1
"$CM" --build _work/build/vs --config Release --target Amax -- -m -v:m -nologo > _work/build/amax.log 2>&1 || true
python $J/errsum.py _work/build/amax.log | head -6
