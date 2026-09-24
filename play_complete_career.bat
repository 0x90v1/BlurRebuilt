@echo off
rem Starts Blur with every career event at gold, every boss beaten and the top rank, applied to
rem the profile save when it loads. The game saves it at its next save point; afterwards use play.bat.
setlocal
set "BLUR2PC_COMPLETE_CAREER=1"
call "%~dp0play.bat"
