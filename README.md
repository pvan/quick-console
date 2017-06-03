
quick fps-style drop-down cmd, accessible via win+~ shortcut

see notes in source code

compile with msvc using build.bat


when launching the exe, it will execute shell.bat,
if it exists in the same directory as the exe

can add a different path with optional command line argument, e.g.:
"D:\apps\phil's quick console.exe" D:/misc/shell.bat


font/colors/size can be tweaked by changing the registry settings in HKEY_CURRENT_USER\Console

settings are stored by the name of the cmd window, so set them to match the name used by the program, by default "phil's quick console"

but easier is to create a cmd shortcut with that name
and change the settings in the properties of the shortcut
the settings will be saved to registry when hitting "apply"


the alpha is currently hard-coded but can be changed in the code and re-compiled




