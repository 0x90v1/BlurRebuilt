@echo off
rem Packages the player build for redistribution as _work\BlurRebuilt.zip:
rem BlurRebuilt.exe (the Master Amax.exe from _work\run), README.md and the fx11
rem effects the game loads (effects.index, effects.samplers and the .fxo files).
setlocal
set "RUN=%~dp0_work\run"
set "STAGE=%~dp0_work\package"
set "ZIP=%~dp0_work\BlurRebuilt.zip"

for %%F in ("%RUN%\Amax.exe" "%RUN%\README.md" "%RUN%\fx11\effects.index" "%RUN%\fx11\effects.samplers") do (
  if not exist "%%~F" (
    echo Missing %%~F
    exit /b 1
  )
)

if exist "%STAGE%" rmdir /s /q "%STAGE%"
mkdir "%STAGE%" || exit /b 1
copy /y "%RUN%\Amax.exe" "%STAGE%\BlurRebuilt.exe" >nul || exit /b 1
copy /y "%RUN%\README.md" "%STAGE%\README.md" >nul || exit /b 1
robocopy "%RUN%\fx11" "%STAGE%\fx11" effects.index effects.samplers *.fxo /s /njh /njs /nfl /ndl /np >nul
if errorlevel 8 (
  echo Copying fx11 failed.
  exit /b 1
)

if exist "%ZIP%" del /q "%ZIP%"
tar -a -c -f "%ZIP%" -C "%STAGE%" BlurRebuilt.exe README.md fx11 || exit /b 1
rmdir /s /q "%STAGE%"
echo Created %ZIP%
