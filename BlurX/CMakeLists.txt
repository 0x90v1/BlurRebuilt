cmake_minimum_required(VERSION 3.25)
project(BlurX LANGUAGES C CXX)

# Phase 3: the Blur 2 (Kanzen) prototypes. This project owns no copy of the
# engine or of phase 2: it builds ../Blur1PC/code with ../Blur1Multi/code laid
# over it and ./code laid over both (same relative path = replaces the lower
# file; new path = new file, listed in cmake/blurx.cmake). See README.md here
# and AGENTS.md "Building". Run from this directory:
#   python ../Blur1PC/code/tools/jam2cmake/jamextract.py --top ../Blur1PC --root code/amax/game/jamfile.jam --target WIN32 --platform PC --build Speed --out _work/build/targets_pc_speed.json
#   python ../Blur1PC/code/tools/jam2cmake/shadergen.py --json _work/build/targets_pc_speed.json --out _work/build/gen/shaders --overlay _work/base/Blur1Multi --overlay .
#   python ../Blur1PC/code/tools/jam2cmake/luabundle.py --json _work/build/targets_pc_speed.json --out _work/build/gen/blobs/amax_lua_bundle.cpp --overlay _work/base/Blur1Multi --overlay .
#   python ../Blur1PC/code/tools/jam2cmake/gen_cmake.py --json _work/build/targets_pc_speed.json --out cmake/targets.cmake --project-dir . --overlay _work/base/Blur1Multi=BLUR1MULTI_DIR --overlay .
# (First: python tools/sync_base.py, which exports the committed Blur1Multi into _work/base.)
# Configure: cmake -S . -B _work/build/vs -G "Visual Studio 18 2026" -A Win32
# (cpgen.py must have been run in ../Blur1PC: its headers are generated beside the .cp files there.)

get_filename_component(BLUR_TOP ${CMAKE_SOURCE_DIR}/../Blur1PC ABSOLUTE)
# Phase 2 as committed: tools/sync_base.py exports it (git archive) into
# _work/base/Blur1Multi, so unfinished edits in ../Blur1Multi never reach this build.
if(EXISTS ${CMAKE_SOURCE_DIR}/_work/base/Blur1Multi/cmake/multi.cmake)
    set(BLUR1MULTI_DIR ${CMAKE_SOURCE_DIR}/_work/base/Blur1Multi)
else()
    message(WARNING "BlurX: no Blur1Multi snapshot (python tools/sync_base.py); building on ../Blur1Multi, the working tree")
    get_filename_component(BLUR1MULTI_DIR ${CMAKE_SOURCE_DIR}/../Blur1Multi ABSOLUTE)
endif()
set(BLUR_PROJECT ${CMAKE_SOURCE_DIR})
set(BLUR_OVERLAY ${CMAKE_SOURCE_DIR})
get_filename_component(BLUR_WORKSPACE ${CMAKE_SOURCE_DIR}/.. ABSOLUTE)

# Every translation unit is part of the phase 2 build (its code tests BLUR1MULTI)
# and can tell it is the phase 3 build.
add_compile_definitions(BLUR1MULTI=1 BLURX=1)

include(${BLUR_TOP}/cmake/blur_common.cmake)
include(${BLUR_PROJECT}/cmake/targets.cmake)
include(${BLUR_TOP}/cmake/overrides.cmake)

# Phase 2's additions, as its own project declares them. multi.cmake names its
# files ${BLUR_OVERLAY}/code/..., so it is read with BLUR_OVERLAY pointing at
# Blur1Multi; blurx_take_multi_sources() then swaps in any BlurX copy of a
# Blur1Multi-only file.
set(BLUR_OVERLAY ${BLUR1MULTI_DIR})
include(${BLUR1MULTI_DIR}/cmake/multi.cmake)
set(BLUR_OVERLAY ${CMAKE_SOURCE_DIR})

include(${BLUR_PROJECT}/cmake/blurx.cmake)
