# BlurX additions to the generated build (hand-maintained). cmake/targets.cmake
# already compiles BlurX copies of Blur1PC files and of Blur1Multi overlay
# copies. New files and new libraries are declared here with a comment saying
# why; keep every path under ${BLUR_OVERLAY}/code so the layout mirrors
# ../Blur1PC/code.

# A source that Blur1Multi/cmake/multi.cmake declares by path (a Blur1Multi-only
# file, or a Blur1PC file the jamfiles do not compile for PC, such as the XMem pak
# codecs) is replaced by a BlurX copy at the same relative path in every target
# that compiles it. Recorded in overlay.json like any other copy (tools/overlay.py take).
function(blurx_take_multi_sources)
    get_property(targets DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY BUILDSYSTEM_TARGETS)
    foreach(t IN LISTS targets)
        get_target_property(srcs ${t} SOURCES)
        if(NOT srcs)
            continue()
        endif()
        set(changed FALSE)
        set(out "")
        foreach(s IN LISTS srcs)
            foreach(base IN ITEMS "${BLUR1MULTI_DIR}" "${BLUR_TOP}")
                string(FIND "${s}" "${base}/code/" at)
                if(at EQUAL 0)
                    string(REPLACE "${base}/" "${BLUR_OVERLAY}/" mine "${s}")
                    if(EXISTS "${mine}")
                        set(s "${mine}")
                        set(changed TRUE)
                    endif()
                endif()
            endforeach()
            list(APPEND out "${s}")
        endforeach()
        if(changed)
            set_property(TARGET ${t} PROPERTY SOURCES ${out})
        endif()
    endforeach()
endfunction()

blurx_take_multi_sources()

# Asset catalog (D12): which disc serves each file. The catalog and profile
# reader has no engine dependencies; the file-system layer (engine/CatalogFs)
# is compiled into Amax beside Blur1Multi's X360ConvertFs, and the overlay copy
# of amax/game/GameSystems.cpp mounts it in catalog mode (--profile=<name>).
add_library(assetcatalog STATIC
    ${BLUR_OVERLAY}/code/mercury/library/util/assetcatalog/source/AssetCatalog.cpp
    ${BLUR_OVERLAY}/code/mercury/library/util/assetcatalog/source/KanzenNormalise.cpp)
target_include_directories(assetcatalog PUBLIC ${BLUR_OVERLAY}/code/mercury/library/util/assetcatalog/include)
set_target_properties(assetcatalog PROPERTIES FOLDER "code/mercury/library/util")
target_sources(Amax PRIVATE
    ${BLUR_OVERLAY}/code/mercury/library/util/assetcatalog/engine/CatalogFs.cpp
    ${BLUR_OVERLAY}/code/mercury/library/util/assetcatalog/engine/KanzenFs.cpp)
target_include_directories(Amax PRIVATE ${BLUR_OVERLAY}/code/mercury/library/util/assetcatalog/engine)
target_link_libraries(Amax PRIVATE assetcatalog)

# BlurX copies of Blur1Multi converter sources (x360convert/source) include that
# folder's private headers (ChunkFile.h) by quoted name; give the copies its path.
target_include_directories(x360convert PRIVATE ${BLUR1MULTI_DIR}/code/mercury/library/util/x360convert/source)

# The BlurX copies of the scene transcoder (Kanzen scene archives, T031):
# Blur1Multi's target/archive.cpp copy includes SceneTranscodeInstantiate.inl by
# quoted name through the include path, so BlurX's engine folder goes first.
target_include_directories(RenderingData BEFORE PRIVATE ${BLUR_OVERLAY}/code/mercury/library/util/x360convert/engine)
target_include_directories(Amax BEFORE PRIVATE ${BLUR_OVERLAY}/code/mercury/library/util/x360convert/engine)

# Kanzen compression type 3 textures (field planes, a port of Blur2Test's decoder):
# a new converter in x360convert, read by the scene transcoder copies.
target_sources(x360convert PRIVATE ${BLUR_OVERLAY}/code/mercury/library/util/x360convert/source/KanzenTexture.cpp)
target_link_libraries(x360convert PRIVATE xmem_pc)
target_include_directories(RenderingData PRIVATE ${BLUR_OVERLAY}/code/mercury/library/util/x360convert/source)
