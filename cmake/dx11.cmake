# [Blur1PCDX11] Native D3D11 foundation, separate from the D3D9 game path.
# This target can be built and checked while the engine's display types are ported.
find_package(Python3 COMPONENTS Interpreter REQUIRED)
file(GLOB _fxc_bins LIST_DIRECTORIES true
    "C:/Program Files (x86)/Windows Kits/10/bin/*/x86")
list(SORT _fxc_bins COMPARE NATURAL ORDER DESCENDING)
find_program(BLUR_FXC_EXE NAMES fxc.exe PATHS ${_fxc_bins} REQUIRED)
set(_fx11_dir ${BLUR_TOP}/code/vendor/fx11)
add_library(blur_fx11 STATIC
    ${_fx11_dir}/d3dxGlobal.cpp
    ${_fx11_dir}/EffectAPI.cpp
    ${_fx11_dir}/EffectLoad.cpp
    ${_fx11_dir}/EffectNonRuntime.cpp
    ${_fx11_dir}/EffectReflection.cpp
    ${_fx11_dir}/EffectRuntime.cpp)
target_include_directories(blur_fx11 PUBLIC ${_fx11_dir}/inc PRIVATE ${_fx11_dir}/Binary)
target_compile_features(blur_fx11 PRIVATE cxx_std_17)
target_compile_definitions(blur_fx11 PRIVATE _WIN32_WINNT=0x0601)
target_precompile_headers(blur_fx11 PRIVATE ${_fx11_dir}/pchfx.h)
target_link_libraries(blur_fx11 PUBLIC d3d11 d3dcompiler)
set_target_properties(blur_fx11 PROPERTIES FOLDER "dx11/vendor")

file(GLOB_RECURSE _dx11_fx_includes CONFIGURE_DEPENDS
    ${BLUR_TOP}/code/amax/game/Shaders/*.h
    ${BLUR_TOP}/code/mercury/library/graphics/shader/include/*.h
    ${BLUR_TOP}/code/mercury/library/graphics/vehicle/include/*.h)
# Recreate the PC effect permutation list from the original Jam rules. This
# produces the native SM5 variant set under the branch's ignored build tree.
set(_dx11_fx_manifest ${BLUR_TOP}/_work/build/targets_fx_pc_speed.json)
set(_dx11_fx_stamp ${BLUR_TOP}/_work/build/gen/fx11/effects.stamp)
file(GLOB_RECURSE _dx11_fx_sources CONFIGURE_DEPENDS
    ${BLUR_TOP}/code/amax/game/Shaders/*.fx
    ${BLUR_TOP}/code/mercury/library/graphics/shader/*.fx
    ${BLUR_TOP}/code/mercury/library/graphics/vehicle/*.fx)
file(GLOB_RECURSE _dx11_fx_jam CONFIGURE_DEPENDS
    ${BLUR_TOP}/code/amax/game/Shaders/*.jam
    ${BLUR_TOP}/code/mercury/library/graphics/shader/*.jam
    ${BLUR_TOP}/code/mercury/library/graphics/vehicle/*.jam)
add_custom_command(OUTPUT ${_dx11_fx_manifest}
    COMMAND ${Python3_EXECUTABLE} ${BLUR_TOP}/code/tools/jam2cmake/jamextract.py
        --top ${BLUR_TOP}
        --root code/amax/game/Shaders/jamfile.jam
        --target WIN32 --platform PC --build Speed
        --out ${_dx11_fx_manifest}
    DEPENDS ${_dx11_fx_jam} ${BLUR_TOP}/code/tools/jam2cmake/jamextract.py
    VERBATIM)
add_custom_command(OUTPUT ${_dx11_fx_stamp}
    COMMAND ${Python3_EXECUTABLE} ${BLUR_TOP}/code/tools/dx11_fxcompile/compile_manifest.py
        --top ${BLUR_TOP} --manifest ${_dx11_fx_manifest}
        --fxc ${BLUR_FXC_EXE}
        --out ${BLUR_TOP}/_work/build/gen/fx11/effects
        --jobs 4
    COMMAND ${CMAKE_COMMAND} -E touch ${_dx11_fx_stamp}
    DEPENDS ${_dx11_fx_manifest} ${_dx11_fx_sources} ${_dx11_fx_includes}
        ${BLUR_TOP}/code/tools/dx11_fxcompile/compile_fx.py
        ${BLUR_TOP}/code/tools/dx11_fxcompile/compile_manifest.py
    VERBATIM)
add_custom_target(dx11_fx_all DEPENDS ${_dx11_fx_stamp})
set_target_properties(dx11_fx_all PROPERTIES FOLDER "dx11")

set(_dx11_shader_dir ${BLUR_TOP}/code/mercury/library/graphics/display/include/display/shaders)
add_custom_command(
    OUTPUT ${_dx11_shader_dir}/psConstants.h ${_dx11_shader_dir}/vsConstants.h
    COMMAND ${Python3_EXECUTABLE} ${BLUR_TOP}/code/tools/jam2cmake/cpgen.py
        ${_dx11_shader_dir}/psConstants.cp ${_dx11_shader_dir}/vsConstants.cv
    DEPENDS ${BLUR_TOP}/code/tools/jam2cmake/cpgen.py
        ${_dx11_shader_dir}/psConstants.cp ${_dx11_shader_dir}/vsConstants.cv
    VERBATIM)
add_custom_target(dx11_shader_headers DEPENDS
    ${_dx11_shader_dir}/psConstants.h ${_dx11_shader_dir}/vsConstants.h)

# Build each Jam-selected display shader as a separate SM5 program. Keep these
# binary files separate from the base branch's SM2/SM3 C arrays.
set(_dx11_builtin_manifest ${BLUR_TOP}/_work/build/targets_pc_speed.json)
set(_dx11_builtin_report ${BLUR_TOP}/_work/build/gen/dx11_builtins/report.json)
file(GLOB_RECURSE _dx11_builtin_jams CONFIGURE_DEPENDS
    ${BLUR_TOP}/code/amax/game/*.jam
    ${BLUR_TOP}/code/mercury/library/*.jam)
file(GLOB_RECURSE _dx11_builtin_sources CONFIGURE_DEPENDS
    ${BLUR_TOP}/code/amax/game/*.vsh
    ${BLUR_TOP}/code/amax/game/*.psh
    ${BLUR_TOP}/code/amax/game/*.h
    ${BLUR_TOP}/code/mercury/library/graphics/*.vsh
    ${BLUR_TOP}/code/mercury/library/graphics/*.psh
    ${BLUR_TOP}/code/mercury/library/graphics/*.h
    ${BLUR_TOP}/code/mercury/library/ui/*.vsh
    ${BLUR_TOP}/code/mercury/library/ui/*.psh
    ${BLUR_TOP}/code/mercury/library/ui/*.h)
add_custom_command(OUTPUT ${_dx11_builtin_manifest}
    COMMAND ${Python3_EXECUTABLE} ${BLUR_TOP}/code/tools/jam2cmake/jamextract.py
        --top ${BLUR_TOP} --root code/amax/game/jamfile.jam
        --target WIN32 --platform PC --build Speed
        --out ${_dx11_builtin_manifest}
    DEPENDS ${_dx11_builtin_jams} ${BLUR_TOP}/code/tools/jam2cmake/jamextract.py
    VERBATIM)
add_custom_command(OUTPUT ${_dx11_builtin_report}
    COMMAND ${Python3_EXECUTABLE}
        ${BLUR_TOP}/code/tools/dx11_shadercompile/compile_builtins.py
        --manifest ${_dx11_builtin_manifest} --top ${BLUR_TOP}
        --fxc ${BLUR_FXC_EXE}
        --out ${BLUR_TOP}/_work/build/gen/dx11_builtins --jobs 4
    DEPENDS ${_dx11_builtin_manifest} ${_dx11_builtin_sources}
        ${BLUR_TOP}/code/tools/dx11_shadercompile/compile_builtins.py
    VERBATIM)
add_custom_target(dx11_shaders_all DEPENDS ${_dx11_builtin_report})
add_dependencies(dx11_shaders_all dx11_shader_headers)
set_target_properties(dx11_shaders_all PROPERTIES FOLDER "dx11")
set(_dx11_builtin_embedded ${BLUR_TOP}/_work/build/gen/dx11_builtins/embedded.cpp)
add_custom_command(OUTPUT ${_dx11_builtin_embedded}
    COMMAND ${Python3_EXECUTABLE}
        ${BLUR_TOP}/code/tools/dx11_shadercompile/embed_builtins.py
        --manifest ${_dx11_builtin_manifest}
        --compiled ${BLUR_TOP}/_work/build/gen/dx11_builtins
        --out ${_dx11_builtin_embedded}
    DEPENDS ${_dx11_builtin_report}
        ${BLUR_TOP}/code/tools/dx11_shadercompile/embed_builtins.py
    VERBATIM)

add_library(blur_dx11 STATIC
    ${BLUR_TOP}/code/mercury/library/graphics/display/source/device/d3d11/D3D11Device.cpp
    ${BLUR_TOP}/code/mercury/library/graphics/display/source/device/d3d11/D3D11Formats.cpp
    ${BLUR_TOP}/code/mercury/library/graphics/display/source/device/d3d11/D3D11States.cpp
    ${BLUR_TOP}/code/mercury/library/graphics/display/source/device/d3d11/D3D11InputLayout.cpp
    ${BLUR_TOP}/code/mercury/library/graphics/display/source/device/d3d11/D3D11Constants.cpp)
target_sources(blur_dx11 PRIVATE
    ${BLUR_TOP}/code/mercury/library/graphics/display/source/device/d3d11/D3D11Effect.cpp
    ${BLUR_TOP}/code/mercury/library/graphics/display/source/device/d3d11/D3D11EffectLibrary.cpp)
target_include_directories(blur_dx11 PUBLIC
    ${BLUR_TOP}/code/mercury/library/graphics/display/include)
target_compile_features(blur_dx11 PRIVATE cxx_std_17)
target_link_libraries(blur_dx11 PUBLIC d3d11 dxgi d3dcompiler blur_fx11)
set_target_properties(blur_dx11 PROPERTIES FOLDER "dx11")

if(TARGET fx AND BLUR_USE_DX11)
    target_sources(fx PRIVATE
        ${BLUR_TOP}/code/mercury/library/graphics/fx/source/NativeFX11.cpp)
    target_include_directories(fx PRIVATE ${_fx11_dir}/inc)
endif()

# [Blur1PCDX11] Compile the native Display2 implementation against the game's
# actual headers. Keep it as an object target until its remaining device and
# context methods are ported and the generated game target can link it.
if(TARGET display2 AND BLUR_USE_DX11)
    add_library(display2_dx11_backend OBJECT
        ${BLUR_TOP}/code/mercury/library/graphics/display2/source/context/D3D11Context.cpp
        ${BLUR_TOP}/code/mercury/library/graphics/display2/source/device/D3D11Device.cpp
        ${BLUR_TOP}/code/mercury/library/graphics/display2/source/device/PCD3D11Device.cpp)
    target_include_directories(display2_dx11_backend PRIVATE
        $<TARGET_PROPERTY:display2,INCLUDE_DIRECTORIES>)
    target_compile_definitions(display2_dx11_backend PRIVATE BUILD_CONFIG_SPEED BLUR_USE_DX11=1)
    set_target_properties(display2_dx11_backend PROPERTIES FOLDER "dx11")

    # The generated Jam list is the retail D3D9 selection. Exclude its device
    # implementations in this branch; Display2 must resolve only native calls.
    get_target_property(_display2_sources display2 SOURCES)
    list(FILTER _display2_sources EXCLUDE REGEX
        "(D3D9Context|D3D9Device|PCD3D9Device|PCD3D9Memory|PCD3D9Utils)\\.cpp$")
    set_property(TARGET display2 PROPERTY SOURCES "${_display2_sources}")
    target_sources(display2 PRIVATE
        $<TARGET_OBJECTS:display2_dx11_backend>
        ${BLUR_TOP}/code/mercury/library/graphics/display2/source/memory/PCD3D11Memory.cpp)
endif()

if(TARGET display AND BLUR_USE_DX11)
    add_library(display_dx11_context_backend OBJECT
        ${_dx11_builtin_embedded}
        ${BLUR_TOP}/code/mercury/library/graphics/display/source/context/Context_D3D11.cpp
        ${BLUR_TOP}/code/mercury/library/graphics/display/source/context/Display2D3D11Bridge.cpp
        ${BLUR_TOP}/code/mercury/library/graphics/display/source/device/d3d11/D3D11GameDevice.cpp
        ${BLUR_TOP}/code/mercury/library/graphics/display/source/device/d3d11/D3D11GameDeviceNative.cpp
        ${BLUR_TOP}/code/mercury/library/graphics/display/source/device/d3d11/PCD3D11GameDevice.cpp
        ${BLUR_TOP}/code/mercury/library/graphics/display/source/window/d3d11/D3D11Window.cpp
        ${BLUR_TOP}/code/mercury/library/graphics/display/source/manager/d3d11/D3D11Manager.cpp)
    target_include_directories(display_dx11_context_backend PRIVATE
        $<TARGET_PROPERTY:display,INCLUDE_DIRECTORIES>)
    target_compile_definitions(display_dx11_context_backend PRIVATE BUILD_CONFIG_SPEED BLUR_USE_DX11=1)
    set_target_properties(display_dx11_context_backend PROPERTIES FOLDER "dx11")

    get_target_property(_display_sources display SOURCES)
    list(FILTER _display_sources EXCLUDE REGEX "/[dD]3[dD]9/")
    list(FILTER _display_sources EXCLUDE REGEX "Context_D3D9\\.cpp$")
    set_property(TARGET display PROPERTY SOURCES "${_display_sources}")
    target_sources(display PRIVATE
        $<TARGET_OBJECTS:display_dx11_context_backend>
        ${BLUR_TOP}/code/mercury/library/graphics/display/source/occlusion/d3d11/D3D11OcclusionQuery.cpp
        ${BLUR_TOP}/code/mercury/library/graphics/display/source/texture/d3d11/D3D11TextureLoad.cpp)
endif()

if(TARGET movie AND BLUR_USE_DX11)
    # [Blur1PCDX11] The installed Bink SDK is a decoder stub; exclude the old
    # D3D9 texture renderer rather than linking any D3D9 runtime into Amax.
    get_target_property(_movie_sources movie SOURCES)
    list(FILTER _movie_sources EXCLUDE REGEX "/w32/bink/binktextures\\.cpp$")
    set_property(TARGET movie PROPERTY SOURCES "${_movie_sources}")
endif()

add_executable(dx11_probe
    ${BLUR_TOP}/code/tools/dx11_probe/main.cpp)
target_compile_features(dx11_probe PRIVATE cxx_std_17)
target_link_libraries(dx11_probe PRIVATE blur_dx11 blur_fx11)
add_dependencies(dx11_probe dx11_shaders_all dx11_fx_all)
set_target_properties(dx11_probe PROPERTIES FOLDER "dx11")
