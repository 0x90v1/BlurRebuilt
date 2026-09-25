@echo off
rem Blur1Multi launcher (phase 2: Blur 1 from any platform's data). The game selects its data
rem with --source=<dir> (or BLUR_SOURCE); a PC install becomes the working directory as retail.
rem Set BLUR_SOURCE to point at another data set (a Blur PC install, or an Xbox 360 disc root once
rem the 360 data layer exists).
rem Optional: BLUR2PC_FRAMELIMIT=<fps> caps rendered frames; BLUR2PC_DECOUPLE=0 disables interpolation.
setlocal
set "EXE=%~dp0_work\run\Amax.exe"
if "%BLUR_SOURCE%"=="" set "BLUR_SOURCE=C:\Program Files (x86)\Activision\Blur(TM)"
if not exist "%EXE%" (
  echo Game not built: "%EXE%" is missing. See AGENTS.md, Building.
  pause
  exit /b 1
)
if not exist "%BLUR_SOURCE%\cache\root.pak" (
  echo Blur data not found in "%BLUR_SOURCE%". Set BLUR_SOURCE to a Blur data folder.
  pause
  exit /b 1
)
set "BLUR2PC_AUTOMATION="
cd /d "%BLUR_SOURCE%"
start "" "%EXE%" "--source=%BLUR_SOURCE%"
