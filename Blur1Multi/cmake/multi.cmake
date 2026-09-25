# Blur1Multi additions to the generated build (hand-maintained; the generated
# cmake/targets.cmake already compiles overlay copies of base files in place
# of the originals). New files and new libraries are declared here with a
# comment saying why. Keep every path under ${BLUR_OVERLAY}/code so the
# layout mirrors ../Blur1PC/code.

# Game source: which directory the data comes from and what platform wrote it
# (docs/decisions.md D10/D11). Engine-level utility beside logicalFs; no engine
# dependencies so it can run before the file system exists.
add_library(gamesource STATIC
    ${BLUR_OVERLAY}/code/mercury/library/util/gamesource/source/GameSource.cpp)
target_include_directories(gamesource PUBLIC
    ${BLUR_OVERLAY}/code/mercury/library/util/gamesource/include)
set_target_properties(gamesource PROPERTIES FOLDER "code/mercury/library/util")

# The overlay copy of amax/game/GameSystems.cpp selects the source in CreateFsLayers.
# GameSourceDump.cpp: BLUR1MULTI_PAKDUMP verification hook (reads through the engine's file system).
target_sources(Amax PRIVATE ${BLUR_OVERLAY}/code/amax/game/GameSourceDump.cpp)
# Console sources: clip z / w for the blob and drop shadow passes from the console prepass target (T076).
target_sources(Amax PRIVATE ${BLUR_OVERLAY}/code/amax/game/Game/GameWorld/View/ConsoleDepth.cpp)
target_link_libraries(Amax PRIVATE gamesource)
# The overlay copy of amax/game/GameLogic/Layers/BootLayer.cpp mounts the Xenon boot paks for 360 sources.
target_link_libraries(amax_gamelogic PRIVATE gamesource)

# XMem (Xbox 360 LZX) decompression on PC: the XDK xcompress API the engine is
# written against, implemented over libmspack's LZX decoder (LGPL 2.1). It takes
# the place of the XDK's Win32 Xcompress.lib that Bizarre's PC tool builds
# linked (misc_shared/jamfile.jam). Decoder verified byte for byte against
# Blur2Test's extraction of every Aug 31 archive (code/tools/xmem-verify).
add_library(xmem_pc STATIC
    ${BLUR_OVERLAY}/code/vendor/xmem-pc/XMemPc.cpp
    ${BLUR_OVERLAY}/code/vendor/libmspack/lzxd.c)
target_include_directories(xmem_pc PUBLIC ${BLUR_OVERLAY}/code/vendor/xmem-pc)
target_include_directories(xmem_pc PRIVATE ${BLUR_OVERLAY}/code/vendor/libmspack)
target_compile_definitions(xmem_pc PRIVATE BLUR_XMEM_LZX)
set_target_properties(xmem_pc PROPERTIES FOLDER "vendor")

# misc_shared: Compression::XMemDecompress and the per-worker contexts, as the
# Xenon and PC tool builds compiled it (jamfile: Define USE_XMEM_COMPRESSION).
target_compile_definitions(misc_shared PRIVATE USE_XMEM_COMPRESSION)
target_include_directories(misc_shared PRIVATE ${BLUR_OVERLAY}/code/vendor/xmem-pc)

# logicalFs: the XMem pak codecs, compiled by the base jamfile only for Xenon
# and converter builds; the overlay PakEntryFile.cpp selects them.
target_sources(logicalfs PRIVATE
    ${BLUR_TOP}/code/mercury/library/util/logicalFs/source/pakfs/impl/PakCompressXMem.cpp
    ${BLUR_TOP}/code/mercury/library/util/logicalFs/source/pakfs/impl/PakCompressPacketXMem.cpp)
target_include_directories(logicalfs PRIVATE ${BLUR_OVERLAY}/code/vendor/xmem-pc)

target_link_libraries(Amax PRIVATE xmem_pc)

# Xbox 360 -> PC asset conversion (T072, D11): engine-independent converters
# (also built into code/tools/x360convert-verify) and the file-system layer that
# serves their output to the engine for a 360 source.
add_library(x360convert STATIC
    ${BLUR_OVERLAY}/code/mercury/library/util/x360convert/source/X360Convert.cpp
    ${BLUR_OVERLAY}/code/mercury/library/util/x360convert/source/TextureConvert.cpp
    ${BLUR_OVERLAY}/code/mercury/library/util/x360convert/source/FontConvert.cpp
    ${BLUR_OVERLAY}/code/mercury/library/util/x360convert/source/ParticleMeshConvert.cpp
    ${BLUR_OVERLAY}/code/mercury/library/util/x360convert/source/FlaskConvert.cpp
    ${BLUR_OVERLAY}/code/mercury/library/util/x360convert/source/BundleConvert.cpp
    ${BLUR_OVERLAY}/code/mercury/library/util/x360convert/source/LocConvert.cpp
    ${BLUR_OVERLAY}/code/mercury/library/util/x360convert/source/BafConvert.cpp
    ${BLUR_OVERLAY}/code/mercury/library/util/x360convert/source/AobjConvert.cpp
    ${BLUR_OVERLAY}/code/mercury/library/util/x360convert/source/XenosTexture.cpp
    ${BLUR_OVERLAY}/code/mercury/library/util/x360convert/source/Ps3Texture.cpp
    ${BLUR_OVERLAY}/code/mercury/library/util/x360convert/source/EffectConvert.cpp
    ${BLUR_OVERLAY}/code/mercury/library/util/x360convert/source/Ps3EffectConvert.cpp
    ${BLUR_OVERLAY}/code/mercury/library/util/x360convert/source/RsxTranslate.cpp)
target_include_directories(x360convert PUBLIC ${BLUR_OVERLAY}/code/mercury/library/util/x360convert/include)

# Xenos shader microcode -> Direct3D 9 shader model 3 HLSL (T074, D03): the
# recompiler of hedge-dev/XenosRecomp (MIT), vendored with a D3D9 output mode
# (code/vendor/xenosrecomp/BLUR2PC.md). C++20 (std::format stands in for {fmt}).
add_library(xenosrecomp STATIC
    ${BLUR_OVERLAY}/code/vendor/xenosrecomp/xenosrecomp_d3d9.cpp
    ${BLUR_OVERLAY}/code/vendor/xenosrecomp/shader_recompiler.cpp)
set_target_properties(xenosrecomp PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON FOLDER "vendor")
target_compile_definitions(xenosrecomp PRIVATE XENOS_RECOMP_D3D9)
target_compile_options(xenosrecomp PRIVATE /FIpch.h /wd4200)
target_include_directories(xenosrecomp PUBLIC ${BLUR_OVERLAY}/code/vendor/xenosrecomp)
target_link_libraries(x360convert PRIVATE xenosrecomp)
target_include_directories(x360convert PRIVATE ${BLUR_OVERLAY}/code/vendor/xenosrecomp)
set_target_properties(x360convert PROPERTIES FOLDER "code/mercury/library/util")
target_sources(Amax PRIVATE
    ${BLUR_OVERLAY}/code/mercury/library/util/x360convert/engine/X360ConvertFs.cpp
    ${BLUR_OVERLAY}/code/mercury/library/util/x360convert/engine/EngineConverters.cpp
    ${BLUR_OVERLAY}/code/mercury/library/util/x360convert/engine/LevelConvert.cpp)

# Xbox 360 audio banks (T075, D15): XMA2 rows decoded with FFmpeg's XMA2 decoder
# (vgmstream's n5.1.2 Win32 DLLs, loaded at run time from the exe's folder: copy
# avcodec-vgmstream-59.dll and avutil-vgmstream-57.dll from BLUR_FFMPEG_ROOT/bin-win32
# into _work/run), into the per-source cache by AudioCacheFs.
set(BLUR_FFMPEG_ROOT ${BLUR_VENDOR}/ffmpeg-5.1.2 CACHE PATH "FFmpeg 5.1.2 headers (include/) and vgmstream's Win32 DLLs (bin-win32/)")
target_sources(x360convert PRIVATE ${BLUR_OVERLAY}/code/mercury/library/util/x360convert/source/XmaBankConvert.cpp)
set_source_files_properties(${BLUR_OVERLAY}/code/mercury/library/util/x360convert/source/XmaBankConvert.cpp PROPERTIES
    INCLUDE_DIRECTORIES ${BLUR_FFMPEG_ROOT}/include)
target_sources(Amax PRIVATE ${BLUR_OVERLAY}/code/mercury/library/util/x360convert/engine/AudioCacheFs.cpp)
target_include_directories(Amax PRIVATE ${BLUR_OVERLAY}/code/mercury/library/util/x360convert/engine)
# LevelConvert.cpp runs the level block readers (as game_resource does, which has these paths).
target_include_directories(Amax PRIVATE
    ${BLUR_TOP}/Code/Mercury/Library/graphics/portal/include
    ${BLUR_TOP}/Code/Mercury/Library/gamesys/crowd_system/include
    ${BLUR_TOP}/Code/Mercury/Library/gamesys/advertising/include
    ${BLUR_TOP}/Code/Mercury/Library/ai/navmesh/include)
target_link_libraries(Amax PRIVATE x360convert)

# Scene archives (.model Render/Scene, later level render files): the engine's
# scene reader runs over a converting archive. The overlay copy of
# renderingdata/source/noninterface/target/archive.cpp includes
# SceneTranscodeInstantiate.inl to instantiate the reader for it;
# SceneTranscode.cpp holds the payload conversions (vertices, strips, normals).
target_sources(RenderingData PRIVATE
    ${BLUR_OVERLAY}/code/mercury/library/util/x360convert/engine/SceneTranscode.cpp)
target_include_directories(RenderingData PRIVATE ${BLUR_OVERLAY}/code/mercury/library/util/x360convert/engine)
# PS3 scene archives: ZLIB_COMPRESSION blocks are inflated with the engine's zlib (T076)
target_include_directories(RenderingData PRIVATE ${BLUR_TOP}/code/vendor/zlib/current_mercury)
target_link_libraries(RenderingData PRIVATE x360convert)

# PS3 sources: the vehicle body's stream 3 (T076). The PS3 vehicle effects read the
# body's position and normal from a stream the PS3 build filled with an SPU job;
# Ps3DamageStream.cpp is its CPU port, used by the overlay copies of VehicleModel.cpp
# and damage/DamageLayer.cpp when the game source is a PS3 disc.
target_sources(vehicle PRIVATE
    ${BLUR_OVERLAY}/code/mercury/library/graphics/vehicle/source/damage/Ps3DamageStream.cpp)
target_link_libraries(vehicle PRIVATE gamesource)
