@echo off


REM call "D:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x64


REM add current directory to path
set path=%cd%;%path%



doskey g=google $*
doskey e=explorer $*

REM doskey np="D:\Program Files (x86)\Notepad++\notepad++.exe" $*


