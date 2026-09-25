@echo off

set COMP="%DXSDK_DIR%Utilities\Bin\x86\fxc" /nologo
set COPY=xcopy /d /f /y /k /r /c 
set ODIR=Z:\shitstorm\working\PC\data\Image\Shaders\World
set CDIR=Z:\shitstorm\game\PC\data\Image\Shaders\World
rem set CDIR=Z:\devroot\code\genesis\applications\Minimal\Data\Shaders\World

set PLATFORM_NAME=PC

set DEFINES=
set DEFINES=%DEFINES% /D PLATFORM_PC
set DEFINES=%DEFINES% /D PCD3D9
rem set DEFINES=%DEFINES% /D FX_DEBUG
