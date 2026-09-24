#| T009 | M1 | Retail PC UI aspect: variable 2D screen width (width*480/height), UI camera rebuild (retail 0x00c358c0, 0x00799710), Lua ChangeResolution layout half, 4:3 bars only off 16:9 | session 2 (Claude) | review (menus verified at 16:9 and 16:10; race HUD and 4:3 not yet) | ui Simple2d/UICamera, amax UiManager, lua_Amax Blur2PCRefreshUIForBackBuffer, GUIBank.lua | T008 |
 Board

Claim a task before working on it. One task per agent. Keep rows short; details go in the ledger. Statuses: `open`, `in progress`, `blocked`, `review`, `done`. Add new tasks at the bottom of the right milestone; never renumber.

| ID | Milestone | Task | Owner | Status | Files / scope | Blocked on |
|---|---|---|---|---|---|---|
| T001 | M1 | Decide D08 (toolchain) with the user | session 1 | done | docs/decisions.md | — |
| T002 | M1 | Extract per-target source lists, include paths and defines from the jamfiles into a machine-readable form | session 1 | done | code/tools/jam2cmake/jamextract.py → _work/build/targets_pc_{speed,debug}.json | — |
| T003 | M1 | Generate CMake from the T002 JSON (incl. the `.cp/.cv/.sp` → `.h` generator and built-in shader blobs) and get `amax` WIN32 x86 compiling with VS 18 | session 1 | done | code/tools/jam2cmake/{cpgen,shadergen,gen_cmake,errsum}.py, CMakeLists.txt, cmake/{targets,overrides}.cmake, code/vendor/{efx-util,dxerr}, compile fixes in code/ | — |
| T004 | M1 | Remove Bink (movie player returns finished), leave Massive undefined, stub or disable Demonware; provide OpenAL 1.1, libjpeg 6b and d3dx9 (DirectX SDK June 2010 or NuGet `Microsoft.DXSDK.D3DX`) | session 1 | done (Demonware vs8 libs link as-is; runtime behaviour untested) | code/mercury/library/graphics/movie, gamesys/advertising, util/JpegHelper, network | T003 |
| T005 | M1 | Run Blur 1 with retail PC data: front end, Showroom, one race | session 1 | in progress (menus, Garage, race and driving work; paint, shadows, HUD-on-road and interpolation jitter fixed; open: showroom re-spray not visible, AI car models not drawn, per-part race damage, collision stall) | runtime fixes; pak v2 reader in util/logicalFs/pakfs | T004 |
| T006 | M1 | Add a data-root command-line override and enable FileMappingDebug/logfs in Debug | — | open | code/amax/game/Main.cpp, GameSystems.cpp | T003 |
| T010 | M2 | Inventory tool v0: walk discs + pak FATs (via Blur2Test catalogues), content hashes, validity checks, one table | — | open | code/tools/kanzen_inventory/ (or _work bootstrap script) | — |
| T011 | M2 | Serving-policy manifest format + resolver | — | open | code/tools/kanzen_inventory/ | T010 |
| T012 | M2 | Engine `I_FileSystem` layer that serves from the manifest | — | open | code/mercury/library/util/logicalFs/inventoryfs/ (new), GameSystems.cpp | T003 |
| T020 | M3 | Chunk-file byte-swap framework + `.texture` converter, verified against retail X360→PC | — | open | code/tools/kanzen_convert/ | T003 |
| T021 | M3 | `.model` converter (Gen A, then Gen B) | — | open | code/tools/kanzen_convert/ | T020 |
| T022 | M3 | Level file converters (Gen A) | — | open | code/tools/kanzen_convert/ | T020 |
| T023 | M3 | XT flask + `.loc` converter | — | open | code/tools/kanzen_convert/ | T020 |
| T024 | M3 | Audio `.baf` path: determine what retail PC consumes, then convert | — | open | analysis first | — |
| T030 | M4 | Gen B `.levelrender` (`Scen`) reader | — | open | graphics/world_render | T022 |
| T031 | M4 | Gen B `.texture` v1 and `.model` 5-chunk readers | — | open | display/texture, game_resource | T020, T021 |
| T032 | M4 | `.levelanim` loader | — | open | game_resource (new) | T030 |
| T033 | M4 | Load Tracks.bin / Levels.*.bin / Ui.bin; reconcile XT schema drift | — | open | amax/game/GameLogic/GameData | T023 |
| T040 | M5 | Shader cache pipeline: microcode → HLSL → SM3 → PC .fxb, per disc | — | open | code/tools/kanzen_shaders/, _work/shaders/ | T003 |
| T050 | M7 | Frame-rate independence: 60 Hz update, uncapped interpolated render | session 1 | done 2026-09-24 (car shadows interpolated; open: particles, headlight projections, HUD markers stay at tick positions) | ApplicationPC.cpp, ApplicationBase.cpp, Timer.cpp, world_render ModelInstance.cpp, camera2 Feed/Camera, GameInput.cpp | T005 |
| T060 | M8 | Design the platform data layer interface (container, byte order, texture, shader, audio) and the game-profile switch (B1/B2 scene + mechanics); proposal in decisions.md | — | open | docs/decisions.md, docs/analysis.md | D09 |
| T061 | M8 | Keep a Blur 1 retail-PC regression run (`_work/run/drive.ps1` boot → Garage → race) and run it before closing each milestone | — | open | _work/run/, docs | T005 |
| T007 | M1 | Single-instance guard (match retail's "BlurMutex" named mutex; plain return 0 like retail; launch-flash traced to the play.bat console host, Blur.lnk added) | session 1 | done 2026-09-24 | code/mercury/library/gamesys/framework/include/framework/application/ApplicationMain.inl, code/amax/game/GameLogic/Replays/ReplayDirector.cpp, Blur.lnk | — |
| T008 | M1 | Port retail PC graphics initialisation and the graphics options (decompiled from retail Blur.exe) | session 2 (Claude) | review (settings.xml, window/device init, apply/reset, options screen done; quality preset is T013, UI aspect is T009) | framework ApplicationPC.{h,cpp}, ApplicationBase.{h,cpp}; display D3D9Manager, DeviceDebug, D3D9DeviceParams, PCD3D9Device, DeviceTypesEnums, WindowBase.h; amax lua_Amax.cpp, GameProfile.cpp, GameDataFinder.cpp, GameApp; Ui lua_scripts Options*/GUIBank/UIEnums/CustomPopupData; ghidra_retail new scripts; docs/analysis.md section 'Graphics options (PC)' | — |
| T009 | M1 | Retail PC UI aspect: variable 2D screen width (width*480/height), UI camera rebuild (retail 0x00c358c0, 0x00799710), Lua ChangeResolution layout half, 4:3 bars only off 16:9 | session 2 (Claude) | review (menus verified at 16:9 and 16:10; race HUD and 4:3 not yet) | ui Simple2d/UICamera, amax UiManager, lua_Amax Blur2PCRefreshUIForBackBuffer, GUIBank.lua | T008 |
| T013 | M1 | Retail graphical detail preset 1-4 (C_GameWorld::SetGraphicalDetail 0x0041b9e0 and the view-manager, main view, mirror, reflection, cube map, blob shadow and render-pass changes it drives; see analysis.md 'Graphics options (PC)') | — | open | amax GameApp, GameWorld, View/*, ViewManager; gamesys blobshadows; scenegraph | T008 |
