@echo off

REM google the contents of the clipboard


REM requires clop.exe, a little thing to paste clipboard to stdout
set CLOP_PATH=D:\~phil\apps\clop.exe

REM basically this takes the stdout of clop.exe and sets it to the variable
FOR /F "tokens=*" %%i in ('%CLOP_PATH%') do SET TMPARGS=%%i

google %TMPARGS%