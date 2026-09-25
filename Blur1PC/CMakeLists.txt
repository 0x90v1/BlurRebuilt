cmake_minimum_required(VERSION 3.25)
project(Blur1PC LANGUAGES C CXX)

# Phase 1: Blur 1 from retail PC data (see AGENTS.md "Building"). Run from this
# directory; the generated description lives in cmake/targets.cmake and every
# build product goes under _work/ here. Phase 2+ projects (../Blur1Multi) reuse
# cmake/blur_common.cmake and this tree's code/ as their base.
#   python code/tools/jam2cmake/jamextract.py --top . --root code/amax/game/jamfile.jam --target WIN32 --platform PC --build Speed --out _work/build/targets_pc_speed.json
#   python code/tools/jam2cmake/cpgen.py <all .cp/.cv/.sp>
#   python code/tools/jam2cmake/shadergen.py --json _work/build/targets_pc_speed.json --out _work/build/gen/shaders
#   python code/tools/jam2cmake/gen_cmake.py --json _work/build/targets_pc_speed.json --out cmake/targets.cmake
# Configure: cmake -S . -B _work/build/vs -G "Visual Studio 18 2026" -A Win32

set(BLUR_TOP ${CMAKE_SOURCE_DIR})
set(BLUR_PROJECT ${CMAKE_SOURCE_DIR})
get_filename_component(BLUR_WORKSPACE ${CMAKE_SOURCE_DIR}/.. ABSOLUTE)

include(${BLUR_TOP}/cmake/blur_common.cmake)
include(${BLUR_PROJECT}/cmake/targets.cmake)
include(${BLUR_TOP}/cmake/overrides.cmake)
