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
target_link_libraries(Amax PRIVATE gamesource)
