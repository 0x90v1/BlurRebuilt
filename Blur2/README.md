# Blur2 — phase 3: the Blur 2 (Kanzen) prototypes

Reserved for phase 3 (AGENTS.md "Project stages/layers"): running the engine
on the five Xbox 360 Blur 2 prototype discs through the asset catalog
(AGENTS.md "Asset catalog", decisions D02/D05) and the Generation B loaders
(board milestones M2-M5).

Nothing is built from here yet. When work starts, this folder follows the
Blur1Multi pattern: a CMake project that takes `../Blur1PC/code` as its base
with `../Blur1Multi/code` and `./code` laid over it (in that order), its own
`cmake/targets.cmake` generated with `--overlay`, its own `_work/` and
`play.bat`. Deleting this folder must leave Blur1Multi and Blur1PC untouched.
