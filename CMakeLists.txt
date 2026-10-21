cmake_minimum_required(VERSION 3.25)
project(Blur1PC LANGUAGES C CXX)

# One CMake project per phase branch (docs/decisions.md D16): this file is the
# same on blur1pc, blur1multi and blurx except for the phase lines marked
# below, which each branch adds and the ones above it never touch. Run from
# the worktree root (see AGENTS.md "Building"); the generated description lives
# in cmake/targets.cmake and every build product goes under _work/ here.
#   python tools/jam2cmake/jamextract.py --top . --root code/amax/game/jamfile.jam --target WIN32 --platform PC --build Speed --out _work/build/targets_pc_speed.json
#   python tools/jam2cmake/cpgen.py <all .cp/.cv/.sp>
#   python tools/jam2cmake/shadergen.py --json _work/build/targets_pc_speed.json --out _work/build/gen/shaders
#   python tools/jam2cmake/luabundle.py --json _work/build/targets_pc_speed.json --out _work/build/gen/blobs/amax_lua_bundle.cpp
#   python tools/jam2cmake/gen_cmake.py --json _work/build/targets_pc_speed.json --out cmake/targets.cmake
# Configure: cmake -S . -B _work/build/vs -G "Visual Studio 18 2026" -A Win32

set(BLUR_TOP ${CMAKE_SOURCE_DIR})
set(BLUR_PROJECT ${CMAKE_SOURCE_DIR})
# The container that holds the phase worktrees (Blur1PC, Blur1Multi, BlurX):
# blur_common.cmake finds the shared _work/vendor through it.
get_filename_component(BLUR_WORKSPACE ${CMAKE_SOURCE_DIR}/.. ABSOLUTE)

# ---- phase defines (each phase branch adds its own line here) ----------------

include(${BLUR_TOP}/cmake/blur_common.cmake)
include(${BLUR_PROJECT}/cmake/targets.cmake)
include(${BLUR_TOP}/cmake/overrides.cmake)

# ---- phase additions (each phase branch includes its own cmake file here) ----
