@echo off
color 70

title Update Mercury Minimal Data

set SRC1=\\Bizgame02\shitstorm\Samples\Data\Mercury\ALL\Data
set SRC2=\\Bizgame02\shitstorm\Samples\Data\Mercury\360\Data
set DST=DEVKIT:\mercury\minimal_xenon

set CPY="%XEDK%\bin\win32\xbcp.exe" /D /R /Y /T

@echo Performing copy [%CPY%] [%SRC1%] [%DST%]
%CPY% %SRC1% %DST%

@echo Performing copy [%CPY%] [%SRC2%] [%DST%]
%CPY% %SRC2% %DST%

pause