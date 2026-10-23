# Decision log

Numbered, append-only. Status: `proposed` (needs the user), `accepted`, `superseded by Dnn`. Never edit an accepted decision; add a new one that supersedes it.

## D04 — Toolchain: VS2008 + Jam as shipped
**Status:** superseded by D08

## D08 — Toolchain: CMake generated from the jamfiles, built with VS 18
**Status:** accepted (user delegated the choice to Claude, 2026-09-23; option 2 below)
Found after D04: the machine has no VS2008 and no DirectX SDK; the tree lacks the custom Jam rules every jamfile depends on (`SupportedTargets`, `IncludePath`, `Define`, …) and the `libs/vendor` tree (Bink, OpenAL, Massive). "As shipped" therefore cannot build anywhere without reconstructing Bizarre's Jambase and installing 2008-era SDKs. Options:
1. Install VS2008 + DirectX SDK (June 2010) and reconstruct the Jam rules (they are simple: include paths, defines, source globs). Closest to shipped; old compiler.
2. Generate a CMake (or vcxproj) description from the jamfile source lists and build with the installed VS 18 toolset, D3D9 headers from the Windows SDK, d3dx9 from the DirectX SDK redistributable or DXVK-native shims. Modern debugger and sanitizers; expect a wave of compile fixes.
Recommendation: option 2, because option 1 still requires reconstructing the rules by hand and then leaves us on a compiler nobody supports. Bink and Massive get stubbed either way; OpenAL 1.1 headers/libs are freely available.

## D16 — Phases as git branches in worktrees; the untouched source as the root
**Status:** accepted (user, 2026-09-25: "make Blur1PC a branch, then Blur1Multi a branch of it, and BlurX a branch of Blur1Multi"; the untouched source as the base of Blur1PC; docs and tools inside Blur1PC so the others inherit them). Supersedes the mechanism of D10; D10's goals (independent, removable layers; Blur1PC fixes flow up, nothing flows down) stand.
The repository is a bare store at `C:\AIProjects\Blur2PC\.git` with one worktree per branch beside it: `Blur1PCSource/` = `master` (the leaked source exactly as at `C:\AIProjects\Blur\code`, 20,770 files, never edited), `Blur1PC/` = `blur1pc`, `Blur1Multi/` = `blur1multi`, `BlurX/` = `blurx`. Each branch is the one below plus its own commits: `blur1pc` is the port of `master` (`git diff master blur1pc` is every port change; `master` was joined to the existing history by an ours-merge, so hashes before the split are unchanged). Every file sits at its real path in every branch, so the build is the Blur1PC build in every worktree, and `docs/`, `AGENTS.md`, the generators and `tools/` are on `blur1pc` and reach the upper branches by merge.
**Retired:** the shared root `_work/` (now `Blur1PC/_work`; `blur_common.cmake` resolves the vendor SDKs through the worktree container since `_work/` is untracked and cannot be inherited).
**Why:** three trees stacked at build time needed a hand-written merge tool and three-tree generator logic that git already provides; two sessions' uncommitted edits in one working tree reached each other's builds and deliveries (2026-09-25, T088/T092). One worktree per phase ends the collisions, and a base that is the untouched leak makes the port auditable line by line (`git log master..blur1pc -- <file>`).
**Costs accepted:** `master` on `origin` is replaced (its old history lives on in `blur1pc`, so the first push of `master` is a force-push and clones re-fetch); a Blur1PC fault found in an upper phase is fixed in `Blur1PC/` and merged down, never patched where it was found (a local patch conflicts with the real fix when it merges); two agents on the same phase still share a worktree (a short-lived feature branch in its own worktree if that needs solving).

## D17 � T231 provisional letterbox stencil reset
**Status:** rejected by user (2026-10-07); removed and replaced by the exact retail T232 mechanism.
The results cutout retains previous frames' stencil marks; a per-render stencil clear before PC 2DBG restores the bars at 60 Hz and unlocked. Geometry and stencil operations were checked against retail, but the retail main-stencil reset location is still unknown. T232 owns recovering it and removing the provisional clear. This does not replace the recovered display-window rendering mechanism or restore the snapshot's back-buffer colour clear. See analysis.md T231 for evidence and verification limits.


D17 evidence addendum (2026-10-07, T232): stock retail runtime capture confirms geometry can overlap the purple bars, matching Retail.jpg. Its final bound stencil surface is cleared in an earlier off-screen pass. The Render2DBG clear remains a proposed provisional substitute; clean bars alone do not establish parity. No accepted rendering decision was reversed during verification. Recover the surface restoration before promoting or replacing this implementation.

D17 direction confirmed by user (2026-10-07, T232): the user rejects any stopgap and requires a 1:1 retail implementation. The provisional Render2DBG clear is removed from source. T232 must recover and port the actual surface-binding lifecycle; delivery remains pending until rebuilt and verified.

D17 replacement delivered and verified (T232): the runtime-decoded retail EndRenderSplitscreenWithoutAA callback restores prepass view-1 depth/stencil for one view at graphical detail > 1 after the retail view-dependent FX refresh and post-processing. That sequence is now implemented exactly, with its recovered getter, and passes delivered 60-Hz/unlocked intro/results checks. No new clear or workaround. See analysis.md T232.

## D18 — Project-authored tools live outside the leaked source

Status: accepted (user, 2026-10-20; T252). New project tools belong in root `tools/<name>/`. `code/tools/` is reserved for tools shipped with the leaked Blur source. Relocate our existing additions and update commands, imports, project-root calculations, CMake dependencies and documentation across all working branches; preserve each phase's own tools and local edits. This supersedes the former `code/tools/<name>/` placement convention, without moving engine code or the original leaked tools.

## D32 — Default DirectSound ring: 3 / 4 / 5 blocks at 24 / 36 / 48 kHz

Status: accepted (user, 2026-10-22; T255). Retail sizes the PC DirectSound ring at rate / 1500 blocks of 256 frames (16 / 24 / 32 blocks: 171 ms queued at High). Players do not set debug values, so the default departs from retail: 3 / 4 / 5 blocks at 24 / 36 / 48 kHz (32 / 28 / 27 ms), the counts of the community "v1.3 in-place" patch (retail VA 0x00546c23), chosen by ear there; 4 blocks at 48 kHz crackled. Other debug rates take the next quality's count. `Audio.PC.Engine.NumberOfBuffersOverride` still restores any count. The engine also holds timeBeginPeriod(1) from device start to shutdown: at the default 15.6 ms timer, the writer's Sleep(AudioSleep) poll fed only 5-20% of realtime into these rings (the user heard it as very slow, broken sound). Everything else in the engine stays retail (AudioSleep, rates, mixer, thread).
