@echo off
rem BlurX launcher (phase 3: the Blur 2 prototypes). Runs the game from several
rem Xbox 360 discs at once through the asset catalog: each file comes from the disc
rem the profile picks (catalog\profiles.txt; local changes in catalog\profiles.local.txt).
rem Profile: first argument, else BLURX_PROFILE, else nov10-b1rules: Nov 10 content with retail
rem Blur 1 gameplay databases, the stopgap of D13 that boots to the front end. "play.bat nov10"
rem runs Nov 10 faithfully (Kanzen gameplay data; stops in the race book until D13 step 2).
rem Discs are looked for where catalog\catalog.tsv was built from; point one elsewhere
rem with BLURX_DISC_<ID>=<dir> (e.g. BLURX_DISC_NOV10).
rem Without a profile (set BLURX_PROFILE= and pass --source=<dir>) the exe behaves as
rem Blur1Multi's: one data source, PC or Xbox 360.
setlocal
set "EXE=%~dp0_work\run\Amax.exe"
set "PROFILE=%~1"
if "%PROFILE%"=="" set "PROFILE=%BLURX_PROFILE%"
if "%PROFILE%"=="" set "PROFILE=nov10-b1rules"
if not exist "%EXE%" (
  echo Game not built: "%EXE%" is missing. See AGENTS.md, Building, and BlurX\README.md.
  pause
  exit /b 1
)
rem The game allows one running copy (retail's BlurMutex, any phase's exe): a second one exits silently.
tasklist /FI "IMAGENAME eq Amax.exe" 2>nul | find /I "Amax.exe" >nul && (
  echo Another Amax.exe is already running ^(any phase's^); the game allows one copy. Close it and try again.
  pause
  exit /b 1
)
set "BLUR2PC_AUTOMATION="
cd /d "%~dp0_work\run"
start "" "%EXE%" "--profile=%PROFILE%" "--catalog=%~dp0catalog"
