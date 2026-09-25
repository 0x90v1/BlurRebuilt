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
    ${BLUR_OVERLAY}/code/mercury/library/util/x360convert/source/XenosTexture.cpp)
target_include_directories(x360convert PUBLIC ${BLUR_OVERLAY}/code/mercury/library/util/x360convert/include)
set_target_properties(x360convert PROPERTIES FOLDER "code/mercury/library/util")
target_sources(Amax PRIVATE ${BLUR_OVERLAY}/code/mercury/library/util/x360convert/engine/X360ConvertFs.cpp)
target_include_directories(Amax PRIVATE ${BLUR_OVERLAY}/code/mercury/library/util/x360convert/engine)
target_link_libraries(Amax PRIVATE x360convert)
