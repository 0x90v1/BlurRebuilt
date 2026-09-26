@echo off
color 70

title Update Minimal Data

set MKDIR=mkdir 

set SRC1="\\bizgame02\shitstorm\Samples\Data\Mercury\ALL\Data"
set SRC2="\\bizgame02\shitstorm\Samples\Data\Mercury\PC\Data"

set DST="..\..\Data\PC"

%MKDIR% %DST%

set COPY_COMMAND=xcopy /d /s /f /y /k /r /c

@echo Performing copy [%COPY_COMMAND%] [%SR1C%] [%DST%]
%COPY_COMMAND% %SRC1%\*.* %DST%

@echo Performing copy [%COPY_COMMAND%] [%SRC2%] [%DST%]
%COPY_COMMAND% %SRC2%\*.* %DST%

pause