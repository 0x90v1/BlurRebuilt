cmake_minimum_required(VERSION 3.25)
project(Blur1Multi LANGUAGES C CXX)

# Phase 2: Blur 1 from any platform's data (PC now, Xbox 360 next, PS3 later).
# This project owns no copy of the engine: it builds ../Blur1PC/code with the
# files under ./code laid over it (same relative path = replaces the base file;
# new path = new file, listed in cmake/multi.cmake). See README.md here and
# AGENTS.md "Building". Run from this directory:
#   python ../Blur1PC/code/tools/jam2cmake/jamextract.py --top ../Blur1PC --root code/amax/game/jamfile.jam --target WIN32 --platform PC --build Speed --out _work/build/targets_pc_speed.json
#   python ../Blur1PC/code/tools/jam2cmake/shadergen.py --json _work/build/targets_pc_speed.json --out _work/build/gen/shaders --overlay .
#   python ../Blur1PC/code/tools/jam2cmake/luabundle.py --json _work/build/targets_pc_speed.json --out _work/build/gen/blobs/amax_lua_bundle.cpp --overlay .
#   python ../Blur1PC/code/tools/jam2cmake/gen_cmake.py --json _work/build/targets_pc_speed.json --out cmake/targets.cmake --project-dir . --overlay .
# Configure: cmake -S . -B _work/build/vs -G "Visual Studio 18 2026" -A Win32
# (cpgen.py must have been run in ../Blur1PC: its headers are generated beside the .cp files there.)

get_filename_component(BLUR_TOP ${CMAKE_SOURCE_DIR}/../Blur1PC ABSOLUTE)
set(BLUR_PROJECT ${CMAKE_SOURCE_DIR})
set(BLUR_OVERLAY ${CMAKE_SOURCE_DIR})
get_filename_component(BLUR_WORKSPACE ${CMAKE_SOURCE_DIR}/.. ABSOLUTE)

# Every translation unit, base or overlay, can tell it is the phase 2 build.
add_compile_definitions(BLUR1MULTI=1)

include(${BLUR_TOP}/cmake/blur_common.cmake)
include(${BLUR_PROJECT}/cmake/targets.cmake)
include(${BLUR_TOP}/cmake/overrides.cmake)
include(${BLUR_PROJECT}/cmake/multi.cmake)
