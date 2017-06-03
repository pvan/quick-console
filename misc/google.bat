@echo off

REM google things from the command line


set QUERY=""

:loop
if not "%1"=="" (
  set QUERY=%QUERY%+%1
  shift
  goto :loop
)


start chrome /new-window www.google.com/search?q=%QUERY%