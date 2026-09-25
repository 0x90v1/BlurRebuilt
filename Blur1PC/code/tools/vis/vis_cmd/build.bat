cscript.exe //nologo Z:\tools\code_tools\scripts\build_server\jam_build.wsf BUILD WIN32_TOOL Release 
if not %errorlevel% == 0 goto fail
cscript.exe //nologo Z:\tools\code_tools\scripts\build_server\jam_build.wsf BUILD WIN64_TOOL Release
if not %errorlevel% == 0 goto fail

goto eof

:eof
exit /b 0

:fail
exit /b 1
