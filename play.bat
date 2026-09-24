@echo off
rem Blur2PC launcher. The game reads its data from the working directory, so this starts it
rem inside the retail Blur install. Set BLUR_DIR to point at another copy of the game data.
rem Optional: BLUR2PC_FRAMELIMIT=<fps> caps rendered frames; BLUR2PC_DECOUPLE=0 disables interpolation.
setlocal
set "EXE=%~dp0_work\run\Amax.exe"
if "%BLUR_DIR%"=="" set "BLUR_DIR=C:\Program Files (x86)\Activision\Blur(TM)"
if not exist "%EXE%" (
  echo Game not built: "%EXE%" is missing. See AGENTS.md, Building.
  pause
  exit /b 1
)
if not exist "%BLUR_DIR%\cache\root.pak" (
  echo Blur data not found in "%BLUR_DIR%". Set BLUR_DIR to the Blur install folder.
  pause
  exit /b 1
)
set "BLUR2PC_AUTOMATION="
cd /d "%BLUR_DIR%"
start "" "%EXE%"
